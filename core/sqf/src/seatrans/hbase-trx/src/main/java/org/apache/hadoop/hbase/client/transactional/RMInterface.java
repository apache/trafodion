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
import java.util.ArrayList;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.InterruptedIOException;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.commons.codec.binary.Hex;

import org.apache.hadoop.fs.Path;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.client.RegionLocator;
import org.apache.hadoop.hbase.ZooKeeperConnectionException;
import org.apache.hadoop.hbase.client.coprocessor.Batch;
import org.apache.hadoop.hbase.client.Connection;
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

import java.util.Map;
import java.util.HashMap;
import java.util.List;
import java.util.Iterator;
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
    private TransactionalTable ttable = null;
    private boolean bRegisterRegionsAtTransEnd;
    private ExecutorService threadPool;
    private CompletionService<Integer> compPool;
    private int intThreads = 16;
    private Connection connection;
    static TransactionManager txm;
    private static List<String> createdTables;
    private static boolean envBroadcastMode;

    static {
        System.loadLibrary("stmlib");
        String envset = System.getenv("TM_USE_SSCC");
        if (envset != null)
           envTransactionAlgorithm = (Integer.parseInt(envset) == 1) ? AlgorithmType.SSCC : AlgorithmType.MVCC;
        else
           envTransactionAlgorithm = AlgorithmType.MVCC;
        String useBroadcastMode = System.getenv("TM_USE_BROADCAST_MODE");
        if (useBroadcastMode != null)
          envBroadcastMode = (Integer.parseInt(useBroadcastMode) == 1) ? true : false;

        createdTables = new ArrayList<String>();
    }

    private native String createTableReq(byte[] lv_byte_htabledesc, byte[][] keys, int numSplits, int keyLength, long transID, byte[] tblName);
    private native String dropTableReq(byte[] lv_byte_tblname, long transID);
    private native String truncateOnAbortReq(byte[] lv_byte_tblName, long transID); 
    private native String alterTableReq(byte[] lv_byte_tblname, Object[] tableOptions, long transID);

    public static void main(String[] args) {
      System.out.println("MAIN ENTRY");
    }

    protected static IdTm idServer;
    private static final int ID_TM_SERVER_TIMEOUT = 1000; // 1 sec 

    public enum AlgorithmType {
       MVCC, SSCC
    }

    protected static AlgorithmType envTransactionAlgorithm;
    private AlgorithmType transactionAlgorithm;

    public RMInterface(final String tableName, Connection connection) throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("RMInterface constructor:"
					    + " tableName: " + tableName);
        bRegisterRegionsAtTransEnd = true; 

        this.connection = connection;
        transactionAlgorithm = envTransactionAlgorithm;
        if( transactionAlgorithm == AlgorithmType.MVCC) //MVCC
        {
           if (LOG.isTraceEnabled()) LOG.trace("Algorithm type: MVCC"
						+ " tableName: " + tableName);
            ttable = new TransactionalTable(Bytes.toBytes(tableName), connection);
        }
        else if(transactionAlgorithm == AlgorithmType.SSCC)
        {
            ttable = new SsccTransactionalTable( Bytes.toBytes(tableName), connection);
        }
        idServer = new IdTm(false);
        if (LOG.isTraceEnabled()) LOG.trace("RMInterface constructor exit");
    }

    public RMInterface(Connection connection) throws IOException {
       this.connection = connection;

    }

    public void pushRegionEpoch (HTableDescriptor desc, final TransactionState ts) throws IOException {
       if (LOG.isDebugEnabled()) LOG.debug("pushRegionEpoch start; transId: " + ts.getTransactionId());

       TransactionalTable ttable1 = new TransactionalTable(Bytes.toBytes(desc.getNameAsString()), connection);
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
          compPool.submit(new RMCallable2(ts, lv_location, connection ) {
             public Integer call() throws IOException {
                return pushRegionEpochX(ts, lv_location, connection);
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
       Connection connection;
       HTable table;
       byte[] startKey;
       byte[] endKey_orig;
       byte[] endKey;

       RMCallable2(TransactionState txState, HRegionLocation location, Connection connection) {
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
                          final HRegionLocation location, Connection connection) throws IOException {
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

    public TransactionState registerTransaction(final TransactionalTable pv_table, 
							     final long transactionID,
							     final byte[] startRow) throws IOException 
    {
       return registerTransaction (pv_table, transactionID, startRow, null, 
                                 false, 0, false );
    }

    public TransactionState registerTransaction(final TransactionalTable pv_table, 
							     final long transactionID, 
							     final byte[] startRow,
							     final byte[] endRow,			
							     final boolean pv_skipConflictCheck,
							     int pv_tmFlags) throws IOException 
    {
       return registerTransaction (pv_table, transactionID, startRow, endRow, 
                                 pv_skipConflictCheck, 0, true );
    }

    public TransactionState registerTransaction(final TransactionalTable pv_table, 
							     final long transactionID, 
							     final byte[] startRow,
							     final byte[] endRow,			
							     final boolean pv_skipConflictCheck,
							     int pv_tmFlags, boolean scanRange) 
                                                                   throws IOException 
    {
        if (LOG.isTraceEnabled()) LOG.trace("Enter registerTransaction, table: " + getTableNameAsString() + " transaction ID: " + transactionID
             + " startRow " + Hex.encodeHexString(startRow)
             + " endRow " + Hex.encodeHexString(endRow));
        boolean register = false;
        
        TransactionState ts = mapTransactionStates.get(transactionID);

        if (LOG.isTraceEnabled()) 
           LOG.trace("mapTransactionStates " + mapTransactionStates + " entries " + mapTransactionStates.size());

        // if we don't have a TransactionState for this ID we need to register it with the TM
        if (ts == null) { 
           ts = TransactionState.getInstance(transactionID);
        }
        HRegionLocation locationRow;
        boolean refresh = false;
        if (createdTables.size() > 0 && createdTables.contains(Bytes.toString(pv_table.getTableName()))){
            if (LOG.isTraceEnabled()) LOG.trace("Locations being refreshed : "
                + Bytes.toString(pv_table.getTableName()) + " for transaction " + transactionID
                + "  CreatedTables size " + createdTables.size());
            refresh = true;
            createdTables.remove(Bytes.toString(pv_table.getTableName()));
        }

        HRegionLocation location = null;
        List<HRegionLocation> scanRegionLocations = null;
        Iterator<HRegionLocation> scanRegionLocationsIter = null;
        
        // set keys to infinite
        if ((envBroadcastMode == true) && (pv_skipConflictCheck == true))
        { 
            locationRow = pv_table.getRegionLocation(startRow, refresh);
            if (LOG.isTraceEnabled()) LOG.trace("RMInterface:registerTransaction - setting keys to infinite for location " + locationRow);
            HRegionInfo regionInfo = new HRegionInfo (locationRow.getRegionInfo().getTable(), HConstants.EMPTY_START_ROW, HConstants.EMPTY_END_ROW);
            location = new HRegionLocation(regionInfo, locationRow.getServerName());
        }
        if (location == null) {
           if (scanRange) {
              scanRegionLocations = pv_table.getRegionsInRange(startRow, endRow, refresh);
              scanRegionLocationsIter = scanRegionLocations.iterator();
              location = scanRegionLocationsIter.next();
           }
           else 
              location = pv_table.getRegionLocation(startRow, refresh);
        }
        do {
           TransactionRegionLocation trLocation = new TransactionRegionLocation(location.getRegionInfo(),
                                                                             location.getServerName());
        if (LOG.isTraceEnabled()) LOG.trace("RMInterface:registerTransaction, created TransactionRegionLocation [" + trLocation.getRegionInfo().getRegionNameAsString() + "], endKey: "
                  + Hex.encodeHexString(trLocation.getRegionInfo().getEndKey()) + " and transaction [" + transactionID + "]");
           // if this region hasn't been registered as participating in the transaction, we need to register it
           if (ts.addRegion(trLocation)) {
              register = true;
          if (LOG.isTraceEnabled()) LOG.trace("RMInterface:registerTransaction, added TransactionRegionLocation ["
                  + trLocation.getRegionInfo().getRegionNameAsString() + "], endKey: "
                  + Hex.encodeHexString(trLocation.getRegionInfo().getEndKey()) + " to transaction " + transactionID
                        + " with " + ts.getParticipantCount() + " participants");
           }

           // register region with TM.
           if (register) {
            ts.registerLocation(location);
               if (LOG.isTraceEnabled()) LOG.trace("RMInterface:registerTransaction, called registerLocation TransactionRegionLocation [" + trLocation.getRegionInfo().getRegionNameAsString() +  "\nEncodedName: [" + trLocation.getRegionInfo().getEncodedName() + "], endKey: "
                  + Hex.encodeHexString(trLocation.getRegionInfo().getEndKey()) + " to transaction [" + transactionID + "]");
           }
           else {
              if (LOG.isTraceEnabled()) LOG.trace("RMInterface:registerTransaction did not send registerRegion for transaction " + transactionID);
           }
           if (scanRange && scanRegionLocationsIter.hasNext())
              location = scanRegionLocationsIter.next();
           else
              break;
           register = false;
        } while (true);
        if (LOG.isTraceEnabled()) LOG.trace("Exit registerTransaction, transaction ID: " + transactionID + ", startId: " + ts.getStartId());
        return ts;
    }

    public TransactionState registerTransaction(final long transactionID,
							     final byte[] row) throws IOException {

       if (LOG.isTraceEnabled()) LOG.trace("Enter registerTransaction,"
					    + " transaction ID: " + transactionID);

       TransactionState ts = registerTransaction(ttable, transactionID, row);

       if (LOG.isTraceEnabled()) LOG.trace("Exit registerTransaction, transaction ID: " + transactionID);
       return ts;
    }

    protected static long getTmId() throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("Enter getTmId");

        long IdVal = -1;
        IdTmId Id;
        try {
           Id = new IdTmId();
           if (LOG.isTraceEnabled()) LOG.trace("getTmId getting new Id with timeout " + ID_TM_SERVER_TIMEOUT);
           idServer.id(ID_TM_SERVER_TIMEOUT, Id);
           if (LOG.isTraceEnabled()) LOG.trace("getTmId idServer.id returned: " + Id.val);
        } catch (IdTmException exc) {
           LOG.error("getTmId: IdTm threw exception " , exc);
           throw new IOException("getTmId: IdTm threw exception ", exc);
        }
        IdVal = Id.val;

        if (LOG.isTraceEnabled()) LOG.trace("Exit getTmId, ID: " + IdVal);
        return IdVal;
    }

    public void createTable(HTableDescriptor desc, byte[][] keys, int numSplits, int keyLength, long transID) throws IOException {
    	if (LOG.isTraceEnabled()) LOG.trace("Enter createTable, txid: " + transID + " Table: " + desc.getNameAsString());
        byte[] lv_byte_desc = desc.toByteArray();
        byte[] lv_byte_tblname = desc.getNameAsString().getBytes();
        if (LOG.isTraceEnabled()) LOG.trace("createTable: htabledesc bytearray: " + lv_byte_desc + "desc in hex: " + Hex.encodeHexString(lv_byte_desc));
        String retstr = createTableReq(lv_byte_desc, keys, numSplits, keyLength, transID, lv_byte_tblname);
        if(retstr != null)
        {
        	LOG.error("createTable exception. Unable to create table " + desc.getNameAsString() + " txid " + transID + " exception " + retstr);
        	throw new IOException("createTable exception. Unable to create table " + desc.getNameAsString() + " Reason: " + retstr);
        }
        if (LOG.isTraceEnabled()) LOG.trace("Adding \'" + desc.getNameAsString() + "\' to createdTables");
        createdTables.add(desc.getNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("Exit createTable, txid: " + transID
            + " Table: " + desc.getNameAsString() + " createdTables size: " + createdTables.size());
    }

    public void truncateTableOnAbort(String tblName, long transID) throws IOException {
		if (LOG.isTraceEnabled()) LOG.trace("Enter truncateTableOnAbort, txid: " + transID + " Table: " + tblName);
        byte[] lv_byte_tblName = tblName.getBytes();
        String retstr = truncateOnAbortReq(lv_byte_tblName, transID);
        if(retstr != null)
        {
        	LOG.error("truncateTableOnAbort exception. Unable to truncate table" + tblName + " txid " + transID + " exception " + retstr);
        	throw new IOException("truncateTableOnAbort exception. Unable to truncate table " + tblName + " Reason: " + retstr);
        }
        if (LOG.isTraceEnabled()) LOG.trace("Exit truncateTableOnAbort, txid: " + transID + " Table: " + tblName);
    }

    public void dropTable(String tblName, long transID) throws IOException {
    	if (LOG.isTraceEnabled()) LOG.trace("Enter dropTable, txid: " + transID + " Table: " + tblName);
        byte[] lv_byte_tblname = tblName.getBytes();
        String retstr = dropTableReq(lv_byte_tblname, transID);
        if(retstr != null)
        {
        	LOG.error("dropTable exception. Unable to drop table" + tblName + " txid " + transID + " exception " + retstr);
        	throw new IOException("dropTable exception. Unable to drop table" + tblName + " Reason: " + retstr);
        }
        if (LOG.isTraceEnabled()) LOG.trace("Exit dropTable, txid: " + transID + " Table: " + tblName);
    }

    public void alter(String tblName, Object[] tableOptions, long transID) throws IOException {
    	if (LOG.isTraceEnabled()) LOG.trace("Enter alterTable, txid: " + transID + " Table: " + tblName);
        byte[] lv_byte_tblname = tblName.getBytes();
        String retstr = alterTableReq(lv_byte_tblname, tableOptions, transID);
        if(retstr != null)
        {
        	LOG.error("alter Table exception. Unable to alter table" + tblName + " txid " + transID + " exception " + retstr);
        	throw new IOException("alter Table exception. Unable to alter table" + tblName + " Reason: " + retstr);
        }
        if (LOG.isTraceEnabled()) LOG.trace("Exit alterTable, txid: " + transID + " Table: " + tblName);
    }   

    static public void clearTransactionStates(final long transactionID) {
      if (LOG.isTraceEnabled()) LOG.trace("clearTransactionStates enter txid: " + transactionID);

      unregisterTransaction(transactionID);

      if (LOG.isTraceEnabled()) LOG.trace("clearTransactionStates exit txid: " + transactionID);
    }

    static public void unregisterTransaction(final long transactionID) {
      TransactionState ts = null;
      if (LOG.isTraceEnabled()) LOG.trace("Enter unregisterTransaction txid: " + transactionID);
      ts = mapTransactionStates.remove(transactionID);
      if (ts == null) {
        LOG.warn("mapTransactionStates.remove did not find transid " + transactionID);
      }
      if (LOG.isTraceEnabled()) LOG.trace("Exit unregisterTransaction txid: " + transactionID);
    }

    // Not used?
    static public void unregisterTransaction(TransactionState ts) {
        if (LOG.isTraceEnabled()) LOG.trace("Enter unregisterTransaction ts: " + ts.getTransactionId());
        mapTransactionStates.remove(ts.getTransactionId());
        if (LOG.isTraceEnabled()) LOG.trace("Exit unregisterTransaction ts: " + ts.getTransactionId());
    }

    public TransactionState getTransactionState(final long transactionID) throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("getTransactionState txid: " + transactionID);
        TransactionState ts = mapTransactionStates.get(transactionID);
        if (ts == null) {
            if (LOG.isTraceEnabled()) LOG.trace("TransactionState for txid: " + transactionID + " not found; throwing IOException");
            throw new IOException("TransactionState for txid: " + transactionID + " not found" );
        }
        if (LOG.isTraceEnabled()) LOG.trace("EXIT getTransactionState");
        return ts;
    }
    public Result get(final long transactionID, final Get get) throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("get txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, get.getRow());
        Result res = ttable.get(ts, get, false);
        if (LOG.isTraceEnabled()) LOG.trace("EXIT get -- result: " + res.toString());
        return res;	
    }

    public void delete(final long transactionID, final Delete delete) throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("delete txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, delete.getRow());
        ttable.delete(ts, delete, false);
    }

    public void deleteRegionTx(final Delete delete, final boolean autoCommit) throws IOException {
        long tid = getTmId();
        if (LOG.isTraceEnabled()) LOG.trace("deleteRegionTx tid: " + tid  + " autoCommit " + autoCommit);
        ttable.deleteRegionTx(tid, delete, autoCommit);
        if (LOG.isTraceEnabled()) LOG.trace("deleteRegionTx EXIT tid: " + tid);
    }

    public void delete(final long transactionID, final List<Delete> deletes) throws IOException {
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

    public ResultScanner getScanner(final long transactionID, final Scan scan) throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("getScanner txid: " + transactionID
           + " scan startRow=" + (Bytes.equals(scan.getStartRow(), HConstants.EMPTY_START_ROW) ?
                   "INFINITE" : Hex.encodeHexString(scan.getStartRow())) + ", endRow="
           + (Bytes.equals(scan.getStopRow(), HConstants.EMPTY_END_ROW) ?
                   "INFINITE" : Hex.encodeHexString(scan.getStopRow())));

        TransactionState ts = registerTransaction(ttable, transactionID, scan.getStartRow(), scan.getStopRow(), false, 0);
        ResultScanner res = ttable.getScanner(ts, scan);
        if (LOG.isTraceEnabled()) LOG.trace("EXIT getScanner");
        return res;
    }

    public void putRegionTx(final Put put, final boolean autoCommit) throws IOException {
        long tid = getTmId();
        if (LOG.isTraceEnabled()) LOG.trace("Enter putRegionTx, autoCommit: " + autoCommit
               + ", tid " + tid);
        ttable.putRegionTx(tid, put, autoCommit);
        if (LOG.isTraceEnabled()) LOG.trace("putRegionTx Exit tid: " + tid);
    }

    public void put(final long transactionID, final Put put) throws IOException {
        if (LOG.isTraceEnabled()) LOG.trace("Enter Put txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, put.getRow());
        ttable.put(ts, put, false);
        if (LOG.isTraceEnabled()) LOG.trace("Exit Put txid: " + transactionID);
    }

    public void put(final long transactionID, final List<Put> puts) throws IOException {
         if (LOG.isTraceEnabled()) LOG.trace("Enter put (list of puts) txid: " + transactionID);
        TransactionState ts = null;
        if ((envBroadcastMode == false))  //  || (skipConflictCheck == false))
        {
      	   for (Put put : puts) {
      	      ts = registerTransaction(transactionID, put.getRow());
      	   }
        }
        // The only way to get here is if both envBroadcastMode AND skipConflictCheck are true
        else
        {
             if (puts.size() > 0)
                 ts = registerTransaction(transactionID, puts.get(0).getRow());
        }
        if (ts == null){
           ts = mapTransactionStates.get(transactionID);
        }
        ttable.put(ts, puts);
        if (LOG.isTraceEnabled()) LOG.trace("Exit put (list of puts) txid: " + transactionID);
    }

    public boolean checkAndPut(final long transactionID,
                                            final byte[] row,
                                            final byte[] family,
                                            final byte[] qualifier,
                                            final byte[] value,
                                            final Put put) throws IOException {

        if (LOG.isTraceEnabled()) LOG.trace("Enter checkAndPut txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, row);
        return ttable.checkAndPut(ts, row, family, qualifier, value, put);
    }

    public synchronized boolean checkAndPutRegionTx(byte[] row, byte[] family,
    		byte[] qualifier, byte[] value, Put put, final boolean autoCommit) throws IOException {

        long tid = getTmId();
        if (LOG.isTraceEnabled()) LOG.trace("checkAndPutRegionTx, autoCommit: " + autoCommit
                           + ", tid " + tid);
        return ttable.checkAndPutRegionTx(tid, row, family, qualifier, value,
                   put, autoCommit);
    }

    public boolean checkAndDelete(final long transactionID,
                                               final byte[] row,
                                               final byte[] family,
                                               final byte[] qualifier,
                                               final byte[] value,
                                               final Delete delete) throws IOException {

        if (LOG.isTraceEnabled()) LOG.trace("Enter checkAndDelete txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, row);
        return ttable.checkAndDelete(ts, row, family, qualifier, value, delete);
    }

    public synchronized boolean checkAndDeleteRegionTx(byte[] row, byte[] family, byte[] qualifier,
            byte[] value, Delete delete, final boolean autoCommit) throws IOException {
       long tid = getTmId();
       if (LOG.isTraceEnabled()) LOG.trace("checkAndDeleteRegionTx, autoCommit: " + autoCommit
               + ", tid " + tid);
       return ttable.checkAndDeleteRegionTx(tid, row, family, qualifier, value, delete, autoCommit);
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

    private String getTableNameAsString()
    {
        return Bytes.toString(ttable.getTableName());
    }

}
