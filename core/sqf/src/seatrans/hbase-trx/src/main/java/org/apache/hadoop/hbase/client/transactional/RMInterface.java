/**
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**/


package org.apache.hadoop.hbase.client.transactional;

import java.io.IOException;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.PrintWriter;
import java.io.StringWriter;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.commons.codec.binary.Hex;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.ZooKeeperConnectionException;
import org.apache.hadoop.hbase.client.coprocessor.Batch;
import org.apache.hadoop.hbase.client.HConnectionManager;
import org.apache.hadoop.hbase.client.HConnection;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.RegionLocator;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.client.transactional.TransactionManager;
import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.client.transactional.CommitUnsuccessfulException;
import org.apache.hadoop.hbase.client.transactional.UnknownTransactionException;
import org.apache.hadoop.hbase.client.transactional.HBaseBackedTransactionLogger;
import org.apache.hadoop.hbase.client.transactional.TransactionalTableClient;
import org.apache.hadoop.hbase.client.transactional.TransactionalTable;
import org.apache.hadoop.hbase.client.transactional.SsccTransactionalTable;

import org.apache.hadoop.hbase.exceptions.DeserializationException;
import org.apache.hadoop.hbase.client.RetriesExhaustedWithDetailsException;
import java.io.InterruptedIOException;

import org.apache.hadoop.hbase.client.transactional.TransState;
import org.apache.hadoop.hbase.client.transactional.TransReturnCode;
import org.apache.hadoop.hbase.client.transactional.TransactionMap;

import org.apache.hadoop.hbase.regionserver.transactional.IdTm;
import org.apache.hadoop.hbase.regionserver.transactional.IdTmException;
import org.apache.hadoop.hbase.regionserver.transactional.IdTmId;

import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TrxRegionService;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PushEpochRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PushEpochResponse;

import org.apache.hadoop.hbase.ipc.BlockingRpcCallback;
import org.apache.hadoop.hbase.ipc.ServerRpcController;

import org.apache.zookeeper.KeeperException;

import com.google.protobuf.ByteString;

import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.HashSet;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorCompletionService;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Callable;
import java.util.concurrent.CompletionService;

public class RMInterface {
    static final Log LOG = LogFactory.getLog(RMInterface.class);
    static Map<Long, TransactionState> mapTransactionStates = TransactionMap.getInstance();
    static final Object mapLock = new Object();

    public AlgorithmType TRANSACTION_ALGORITHM;
    static Map<Long, Set<RMInterface>> mapRMsPerTransaction = new HashMap<Long,  Set<RMInterface>>();
    private TransactionalTableClient ttable = null;
    private ExecutorService threadPool;
    private CompletionService<Integer> compPool;
    private int intThreads = 16;
    static {
        System.loadLibrary("stmlib");
    }

    private native void registerRegion(int port, byte[] hostname, long startcode, byte[] regionInfo);
    private native void createTableReq(byte[] lv_byte_htabledesc, byte[][] keys, int numSplits, int keyLength, long transID, byte[] tblName);
    private native void dropTableReq(byte[] lv_byte_tblname, long transID);
    private native void truncateOnAbortReq(byte[] lv_byte_tblName, long transID); 
    private native void alterTableReq(byte[] lv_byte_tblname, Object[] tableOptions, long transID);

    public static void main(String[] args) {
      System.out.println("MAIN ENTRY");
    }

    private IdTm idServer;
    private static final int ID_TM_SERVER_TIMEOUT = 1000;

    public enum AlgorithmType {
       MVCC, SSCC
    }

    private AlgorithmType transactionAlgorithm;

    public RMInterface(final String tableName) throws IOException {
        //super(conf, Bytes.toBytes(tableName));
        transactionAlgorithm = AlgorithmType.MVCC;
        String envset = System.getenv("TM_USE_SSCC");
        if( envset != null)
        {
            transactionAlgorithm = (Integer.parseInt(envset) == 1) ? AlgorithmType.SSCC : AlgorithmType.MVCC;
        }
        if( transactionAlgorithm == AlgorithmType.MVCC) //MVCC
        {
            ttable = new TransactionalTable(Bytes.toBytes(tableName));
        }
        else if(transactionAlgorithm == AlgorithmType.SSCC)
        {
            ttable = new SsccTransactionalTable( Bytes.toBytes(tableName));
        }

        try {
           idServer = new IdTm(false);
        }
        catch (Exception e){
           LOG.error("RMInterface: Exception creating new IdTm: " + e);
        }
        if (LOG.isTraceEnabled()) LOG.trace("RMInterface constructor exit");
    }

    public RMInterface() throws IOException {

    }

    public void pushRegionEpoch (HTableDescriptor desc, final TransactionState ts) throws IOException {
       LOG.info("pushRegionEpoch start; transId: " + ts.getTransactionId());

       TransactionalTable ttable1 = new TransactionalTable(Bytes.toBytes(desc.getNameAsString()));
       HConnection connection = ttable1.getConnection();
       long lvTransid = ts.getTransactionId();
       RegionLocator rl = connection.getRegionLocator(desc.getTableName());
       List<HRegionLocation> regionList = rl.getAllRegionLocations();

       boolean complete = false;
       int loopCount = 0;
       int result = 0;

       for (HRegionLocation location : regionList) {
          final byte[] regionName = location.getRegionInfo().getRegionName();
          if (compPool == null){
              LOG.info("pushRegionEpoch compPool is null");
              threadPool = Executors.newFixedThreadPool(intThreads);
              compPool = new ExecutorCompletionService<Integer>(threadPool);
          }

          final HRegionLocation lv_location = location;
          final HConnection lv_connection = connection;
          compPool.submit(new RMCallable2(ts, lv_location, lv_connection ) {
             public Integer call() throws IOException {
                return pushRegionEpochX(ts, lv_location, lv_connection);
             }
          });
          try {
            result = compPool.take().get();
          } catch(Exception ex) {
            throw new IOException(ex);
          }
          if ( result != 0 ){
             LOG.error("pushRegionEpoch result " + result + " returned from region "
                          + location.getRegionInfo().getRegionName());
             throw new IOException("pushRegionEpoch result " + result + " returned from region "
                      + location.getRegionInfo().getRegionName());
          }
       }
       if (LOG.isTraceEnabled()) LOG.trace("pushRegionEpoch end transid: " + ts.getTransactionId());
       return;
    }

    private abstract class RMCallable2 implements Callable<Integer>{
       TransactionState transactionState;
       HRegionLocation  location;
       HConnection connection;
       HTable table;
       byte[] startKey;
       byte[] endKey_orig;
       byte[] endKey;

       RMCallable2(TransactionState txState, HRegionLocation location, HConnection connection) {
          this.transactionState = txState;
          this.location = location;
          this.connection = connection;
          try {
             table = new HTable(location.getRegionInfo().getTable(), connection);
          } catch(IOException e) {
             LOG.error("Error obtaining HTable instance " + e);
             table = null;
          }
          startKey = location.getRegionInfo().getStartKey();
          endKey_orig = location.getRegionInfo().getEndKey();
          endKey = TransactionManager.binaryIncrementPos(endKey_orig, -1);

       }

       public Integer pushRegionEpochX(final TransactionState txState,
        		           final HRegionLocation location, HConnection connection) throws IOException {
          if (LOG.isTraceEnabled()) LOG.trace("pushRegionEpochX -- Entry txState: " + txState
                   + " location: " + location);
        	
          Batch.Call<TrxRegionService, PushEpochResponse> callable =
              new Batch.Call<TrxRegionService, PushEpochResponse>() {
                 ServerRpcController controller = new ServerRpcController();
                 BlockingRpcCallback<PushEpochResponse> rpcCallback =
                    new BlockingRpcCallback<PushEpochResponse>();

                 @Override
                 public PushEpochResponse call(TrxRegionService instance) throws IOException {
                    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PushEpochRequest.Builder
                    builder = PushEpochRequest.newBuilder();
                    builder.setTransactionId(txState.getTransactionId());
                    builder.setEpoch(txState.getStartEpoch());
                    builder.setRegionName(ByteString.copyFromUtf8(Bytes.toString(location.getRegionInfo().getRegionName())));
                    instance.pushOnlineEpoch(controller, builder.build(), rpcCallback);
                    return rpcCallback.get();
                 }
              };

              Map<byte[], PushEpochResponse> result = null;
              try {
                 if (LOG.isTraceEnabled()) LOG.trace("pushRegionEpochX -- before coprocessorService: startKey: "
                     + new String(startKey, "UTF-8") + " endKey: " + new String(endKey, "UTF-8"));
                 result = table.coprocessorService(TrxRegionService.class, startKey, endKey, callable);
              } catch (Throwable e) {
                 String msg = "ERROR occurred while calling pushRegionEpoch coprocessor service in pushRegionEpochX";
                 LOG.error(msg + ":" + e);
                 throw new IOException(msg);
              }

              if(result.size() == 1){
                 // size is 1
                 for (PushEpochResponse eresponse : result.values()){
                   if(eresponse.getHasException()) {
                     String exceptionString = new String (eresponse.getException().toString());
                     LOG.error("pushRegionEpochX - coprocessor exceptionString: " + exceptionString);
                     throw new IOException(eresponse.getException());
                   }
                 }
              }
              else {
                  LOG.error("pushRegionEpochX, received incorrect result size: " + result.size() + " txid: "
                          + txState.getTransactionId() + " location: " + location.getRegionInfo().getRegionNameAsString());
                  return 1;
              }
              if (LOG.isTraceEnabled()) LOG.trace("pushRegionEpochX -- Exit txState: " + txState
                      + " location: " + location);
              return 0;       
       }
    }

    public synchronized TransactionState registerTransaction(final long transactionID, final byte[] row) throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("Enter registerTransaction, transaction ID: " + transactionID);
        boolean register = false;
        short ret = 0;

        TransactionState ts = mapTransactionStates.get(transactionID);

        if (LOG.isTraceEnabled()) LOG.trace("mapTransactionStates " + mapTransactionStates + " entries " + mapTransactionStates.size());

        // if we don't have a TransactionState for this ID we need to register it with the TM
        if (ts == null) {
           if (LOG.isTraceEnabled()) LOG.trace("registerTransaction transactionID (" + transactionID +
                    ") not found in mapTransactionStates of size: " + mapTransactionStates.size());
           ts = new TransactionState(transactionID);

           long startIdVal = -1;

           // Set the startid
           if (transactionAlgorithm == AlgorithmType.SSCC) {
              IdTmId startId;
              try {
                 startId = new IdTmId();
                 if (LOG.isTraceEnabled()) LOG.trace("registerTransaction getting new startId");
                 idServer.id(ID_TM_SERVER_TIMEOUT, startId);
                 if (LOG.isTraceEnabled()) LOG.trace("registerTransaction idServer.id returned: " + startId.val);
              } catch (IdTmException exc) {
                 LOG.error("registerTransaction: IdTm threw exception " , exc);
                 throw new IOException("registerTransaction: IdTm threw exception ", exc);
              }
              startIdVal = startId.val;
           }
           ts.setStartId(startIdVal);

           synchronized (mapTransactionStates) {
              TransactionState ts2 = mapTransactionStates.get(transactionID);
              if (ts2 != null) {
                 // Some other thread added the transaction while we were creating one.  It's already in the
                 // map, so we can use the existing one.
                 if (LOG.isTraceEnabled()) LOG.trace("RMInterface:registerTransaction, found TransactionState object while creating a new one " + ts2);
                 ts = ts2;
              }
              else {
                 if (LOG.isTraceEnabled()) LOG.trace("RMInterface:registerTransaction, adding new TransactionState to map " + ts);
                 mapTransactionStates.put(transactionID, ts);
              }
           }// end synchronized
           register = true;
        }
        else {
            if (LOG.isTraceEnabled()) LOG.trace("RMInterface:registerTransaction - Found TS in map for tx " + ts);
        }
        HRegionLocation location = ttable.getRegionLocation(row, false /*reload*/);

        TransactionRegionLocation trLocation = new TransactionRegionLocation(location.getRegionInfo(),
                                                                             location.getServerName());
        if (LOG.isTraceEnabled()) LOG.trace("RMInterface:registerTransaction, created TransactionRegionLocation [" + trLocation.getRegionInfo().getRegionNameAsString() + "], endKey: "
                  + Hex.encodeHexString(trLocation.getRegionInfo().getEndKey()) + " and transaction [" + transactionID + "]");

        // if this region hasn't been registered as participating in the transaction, we need to register it
        if (ts.addRegion(trLocation)) {
          register = true;
          if (LOG.isTraceEnabled()) LOG.trace("RMInterface:registerTransaction, added TransactionRegionLocation [" + trLocation.getRegionInfo().getRegionNameAsString() +  "\nEncodedName: [" + trLocation.getRegionInfo().getEncodedName() + "], endKey: "
                  + Hex.encodeHexString(trLocation.getRegionInfo().getEndKey()) + " to transaction [" + transactionID + "]");
        }

        // register region with TM.
        if (register) {
            ts.registerLocation(trLocation);
             if (LOG.isTraceEnabled()) LOG.trace("RMInterface:registerTransaction, called registerLocation TransactionRegionLocation [" + trLocation.getRegionInfo().getRegionNameAsString() +  "\nEncodedName: [" + trLocation.getRegionInfo().getEncodedName() + "], endKey: "
                  + Hex.encodeHexString(trLocation.getRegionInfo().getEndKey()) + " to transaction [" + transactionID + "]");
        }
        else {
          if (LOG.isTraceEnabled()) LOG.trace("RMInterface:registerTransaction did not send registerRegion for transaction " + transactionID);
        }

        if ((ts == null) || (ret != 0)) {
            LOG.error("registerTransaction failed, TransactionState is NULL"); 
            throw new IOException("registerTransaction failed with error.");
        }

        if (LOG.isTraceEnabled()) LOG.trace("Exit registerTransaction, transaction ID: " + transactionID + ", startId: " + ts.getStartId());
        return ts;
    }

    public void createTable(HTableDescriptor desc, byte[][] keys, int numSplits, int keyLength, long transID) throws IOException {
    	if (LOG.isTraceEnabled()) LOG.trace("Enter createTable, txid: " + transID + " Table: " + desc.getNameAsString());
        byte[] lv_byte_desc = desc.toByteArray();
        byte[] lv_byte_tblname = desc.getNameAsString().getBytes();
        if (LOG.isTraceEnabled()) LOG.trace("createTable: htabledesc bytearray: " + lv_byte_desc + "desc in hex: " + Hex.encodeHexString(lv_byte_desc));
        createTableReq(lv_byte_desc, keys, numSplits, keyLength, transID, lv_byte_tblname);
        TransactionState ts = mapTransactionStates.get(transID);
        if (LOG.isTraceEnabled()) LOG.trace("createTable: pushing epoch into regions for : " + desc.getNameAsString());
        if (ts == null){
           if (LOG.isTraceEnabled()) LOG.trace("pushing epoch into regions but unable to get ts object for transID : " + transID);
           throw new IOException("createTable push epoch exception for table " + desc.getNameAsString());
        }
        pushRegionEpoch(desc, ts);
        if (LOG.isTraceEnabled()) LOG.trace("createTable: epoch pushed into regions for : " + desc.getNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("Exit createTable, txid: " + transID + " Table: " + desc.getNameAsString());
    }

    public void truncateTableOnAbort(String tblName, long transID) throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("truncateTableOnAbort ENTER: ");
            byte[] lv_byte_tblName = tblName.getBytes();
            truncateOnAbortReq(lv_byte_tblName, transID);
    }

    public void dropTable(String tblName, long transID) throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("dropTable ENTER: ");

            byte[] lv_byte_tblname = tblName.getBytes();
            dropTableReq(lv_byte_tblname, transID);
    }

    public void alter(String tblName, Object[] tableOptions, long transID) throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("alter ENTER: ");

            byte[] lv_byte_tblname = tblName.getBytes();
            alterTableReq(lv_byte_tblname, tableOptions, transID);
    }   

    static public void clearTransactionStates(final long transactionID) {
      if (LOG.isTraceEnabled()) LOG.trace("cts1 Enter txid: " + transactionID);

      unregisterTransaction(transactionID);

      if (LOG.isTraceEnabled()) LOG.trace("cts2 txid: " + transactionID);
    }

    static public synchronized void unregisterTransaction(final long transactionID) {
      TransactionState ts = null;
      if (LOG.isTraceEnabled()) LOG.trace("Enter unregisterTransaction txid: " + transactionID);
      try {
        ts = mapTransactionStates.remove(transactionID);
      } catch (Exception e) {
        LOG.warn("Ignoring exception. mapTransactionStates.remove for transid " + transactionID + 
                 " failed with exception " + e);
        return;
      }
      if (ts == null) {
        LOG.warn("mapTransactionStates.remove did not find transid " + transactionID);
      }
    }

    // Not used?
    static public synchronized void unregisterTransaction(TransactionState ts) {
        mapTransactionStates.remove(ts.getTransactionId());
    }

    public synchronized Result get(final long transactionID, final Get get) throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("get txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, get.getRow());
        Result res = ttable.get(ts, get, false);
        if (LOG.isTraceEnabled()) LOG.trace("EXIT get -- result: " + res.toString());
        return res;	
    }

    public synchronized void delete(final long transactionID, final Delete delete) throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("delete txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, delete.getRow());
        ttable.delete(ts, delete, false);
    }

    public synchronized void delete(final long transactionID, final List<Delete> deletes) throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("Enter delete (list of deletes) txid: " + transactionID);
        TransactionState ts = null;
	for (Delete delete : deletes) {
	    ts = registerTransaction(transactionID, delete.getRow());
	}
        if (ts == null){
           ts = mapTransactionStates.get(transactionID);
        }
        ttable.delete(ts, deletes);
        if (LOG.isTraceEnabled()) LOG.trace("Exit delete (list of deletes) txid: " + transactionID);
    }

    public synchronized ResultScanner getScanner(final long transactionID, final Scan scan) throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("getScanner txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, scan.getStartRow());
        ResultScanner res = ttable.getScanner(ts, scan);
        if (LOG.isTraceEnabled()) LOG.trace("EXIT getScanner");
        return res;
    }

    public synchronized void put(final long transactionID, final Put put) throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("Enter Put txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, put.getRow());
        ttable.put(ts, put, false);
        if (LOG.isTraceEnabled()) LOG.trace("Exit Put txid: " + transactionID);
    }

    public synchronized void put(final long transactionID, final List<Put> puts) throws IOException {
         if (LOG.isTraceEnabled()) LOG.trace("Enter put (list of puts) txid: " + transactionID);
        TransactionState ts = null;
      	for (Put put : puts) {
      	    ts = registerTransaction(transactionID, put.getRow());
      	}
        if (ts == null){
           ts = mapTransactionStates.get(transactionID);
        }
        ttable.put(ts, puts);
         if (LOG.isTraceEnabled()) LOG.trace("Exit put (list of puts) txid: " + transactionID);
    }

    public synchronized boolean checkAndPut(final long transactionID, byte[] row, byte[] family, byte[] qualifier,
                       byte[] value, Put put) throws IOException {

        if (LOG.isTraceEnabled()) LOG.trace("Enter checkAndPut txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, row);
        return ttable.checkAndPut(ts, row, family, qualifier, value, put);
    }

    public synchronized boolean checkAndDelete(final long transactionID, byte[] row, byte[] family, byte[] qualifier,
                       byte[] value, Delete delete) throws IOException {

        if (LOG.isTraceEnabled()) LOG.trace("Enter checkAndDelete txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, row);
        return ttable.checkAndDelete(ts, row, family, qualifier, value, delete);
    }

    public void close()  throws IOException
    {
        ttable.close();
    }

    public void setAutoFlush(boolean autoFlush, boolean f)
    {
        ttable.setAutoFlush(autoFlush,f);
    }
    public org.apache.hadoop.conf.Configuration getConfiguration()
    {
        return ttable.getConfiguration();
    }
    public void flushCommits() throws IOException {
         ttable.flushCommits();
    }
    public HConnection getConnection()
    {
        return ttable.getConnection();
    }
    public byte[][] getEndKeys()
                    throws IOException
    {
        return ttable.getEndKeys();
    }
    public byte[][] getStartKeys() throws IOException
    {
        return ttable.getStartKeys();
    }
    public void setWriteBufferSize(long writeBufferSize) throws IOException
    {
        ttable.setWriteBufferSize(writeBufferSize);
    }
    public long getWriteBufferSize()
    {
        return ttable.getWriteBufferSize();
    }
    public byte[] getTableName()
    {
        return ttable.getTableName();
    }
    public ResultScanner getScanner(Scan scan, float dopParallelScanner) throws IOException
    {
        return ttable.getScanner(scan, dopParallelScanner);
    }
    public Result get(Get g) throws IOException
    {
        return ttable.get(g);
    }

    public Result[] get( List<Get> g) throws IOException
    {
        return ttable.get(g);
    }
    public void delete(Delete d) throws IOException
    {
        ttable.delete(d);
    }
    public void delete(List<Delete> deletes) throws IOException
    {
        ttable.delete(deletes);
    }
    public boolean checkAndPut(byte[] row, byte[] family, byte[] qualifier, byte[] value, Put put) throws IOException
    {
        return ttable.checkAndPut(row,family,qualifier,value,put);
    }
    public void put(Put p) throws IOException
    {
        ttable.put(p);
    }
    public void put(List<Put> p) throws IOException
    {
        ttable.put(p);
    }
    public boolean checkAndDelete(byte[] row, byte[] family, byte[] qualifier, byte[] value,  Delete delete) throws IOException
    {
        return ttable.checkAndDelete(row,family,qualifier,value,delete);
    }
}
