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
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.client.Connection;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
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

import org.apache.hadoop.hbase.ipc.BlockingRpcCallback;
import org.apache.hadoop.hbase.ipc.ServerRpcController;

import org.apache.zookeeper.KeeperException;

import com.google.protobuf.ByteString;

import java.util.Map;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.HashSet;
import java.util.concurrent.ConcurrentHashMap;

public class RMInterface {
    static final Log LOG = LogFactory.getLog(RMInterface.class);
    static Map<Long, TransactionState> mapTransactionStates = TransactionMap.getInstance();
    static final Object mapLock = new Object();

    public AlgorithmType TRANSACTION_ALGORITHM;
    static Map<Long, Set<RMInterface>> mapRMsPerTransaction = new HashMap<Long,  Set<RMInterface>>();
    private TransactionalTableClient ttable = null;
    private Connection connection;
    static {
        System.loadLibrary("stmlib");
        String envset = System.getenv("TM_USE_SSCC");
        if (envset != null)
           envTransactionAlgorithm = (Integer.parseInt(envset) == 1) ? AlgorithmType.SSCC : AlgorithmType.MVCC;
        else
           envTransactionAlgorithm = AlgorithmType.MVCC;
    }

    private native void registerRegion(int port, byte[] hostname, long startcode, byte[] regionInfo);
    private native String createTableReq(byte[] lv_byte_htabledesc, byte[][] keys, int numSplits, int keyLength, long transID, byte[] tblName);
    private native String dropTableReq(byte[] lv_byte_tblname, long transID);
    private native String truncateOnAbortReq(byte[] lv_byte_tblName, long transID); 
    private native String alterTableReq(byte[] lv_byte_tblname, Object[] tableOptions, long transID);

    public static void main(String[] args) {
      System.out.println("MAIN ENTRY");
    }

    private IdTm idServer;
    private static final int ID_TM_SERVER_TIMEOUT = 1000; // 1 sec 

    public enum AlgorithmType {
       MVCC, SSCC
    }

    private static AlgorithmType envTransactionAlgorithm;
    private AlgorithmType transactionAlgorithm;

    public RMInterface(final String tableName, Connection connection) throws IOException {
        //super(conf, Bytes.toBytes(tableName));
        this.connection = connection;
        transactionAlgorithm = envTransactionAlgorithm;
        if( transactionAlgorithm == AlgorithmType.MVCC) //MVCC
        {
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
                 if (LOG.isTraceEnabled()) LOG.trace("registerTransaction getting new startId with timeout " + ID_TM_SERVER_TIMEOUT);
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

    public long getTmId() throws IOException {
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
        if (LOG.isTraceEnabled()) LOG.trace("Exit createTable, txid: " + transID + " Table: " + desc.getNameAsString());
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
      if (LOG.isTraceEnabled()) LOG.trace("cts1 Enter txid: " + transactionID);

      unregisterTransaction(transactionID);

      if (LOG.isTraceEnabled()) LOG.trace("cts2 txid: " + transactionID);
    }

    static public synchronized void unregisterTransaction(final long transactionID) {
      TransactionState ts = null;
      if (LOG.isTraceEnabled()) LOG.trace("Enter unregisterTransaction txid: " + transactionID);
      ts = mapTransactionStates.remove(transactionID);
      if (ts == null) {
        LOG.warn("mapTransactionStates.remove did not find transid " + transactionID);
      }
      if (LOG.isTraceEnabled()) LOG.trace("Exit unregisterTransaction txid: " + transactionID);
    }

    // Not used?
    static public synchronized void unregisterTransaction(TransactionState ts) {
        if (LOG.isTraceEnabled()) LOG.trace("Enter unregisterTransaction ts: " + ts.getTransactionId());
        mapTransactionStates.remove(ts.getTransactionId());
        if (LOG.isTraceEnabled()) LOG.trace("Exit unregisterTransaction ts: " + ts.getTransactionId());
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

    public synchronized void deleteRegionTx(final Delete delete, final boolean autoCommit) throws IOException {
        long tid = getTmId();
        if (LOG.isTraceEnabled()) LOG.trace("deleteRegionTx tid: " + tid);
        ttable.deleteRegionTx(tid, delete, autoCommit);
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

    public synchronized void putRegionTx(final Put put, final boolean autoCommit) throws IOException {
        long tsId = getTmId();
        if (LOG.isTraceEnabled()) LOG.trace("Enter putRegionTx, autoCommit: " + autoCommit + ", tsId " + tsId);
        ttable.putRegionTx(tsId, put, autoCommit);
        if (LOG.isTraceEnabled()) LOG.trace("Exit putRegionTx tsId: " + tsId);
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

    public synchronized boolean checkAndPutRegionTx(byte[] row, byte[] family,
    		byte[] qualifier, byte[] value, Put put, final boolean autoCommit) throws IOException {

        long tsId = getTmId();
        if (LOG.isTraceEnabled()) LOG.trace("Enter checkAndPutRegionTx tsId: " + tsId
        		           + ": autoCommit " + autoCommit );
        return ttable.checkAndPutRegionTx(tsId, row, family, qualifier, value,
        		                          put, autoCommit);
    }

    public synchronized boolean checkAndDelete(final long transactionID, byte[] row, byte[] family, byte[] qualifier,
                       byte[] value, Delete delete) throws IOException {

        if (LOG.isTraceEnabled()) LOG.trace("Enter checkAndDelete txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, row);
        return ttable.checkAndDelete(ts, row, family, qualifier, value, delete);
    }

    public synchronized boolean checkAndDeleteRegionTx(byte[] row, byte[] family, byte[] qualifier,
            byte[] value, Delete delete, final boolean autoCommit) throws IOException {
       long tid = getTmId();
       if (LOG.isTraceEnabled()) LOG.trace("Enter checkAndDeleteRegionTx tid: " + tid);
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
}
