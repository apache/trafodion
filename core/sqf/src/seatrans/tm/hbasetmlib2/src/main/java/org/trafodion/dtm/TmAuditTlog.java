// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

package org.trafodion.dtm;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;

import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;

import org.apache.commons.codec.binary.Hex;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.Cell;
import org.apache.hadoop.hbase.CellUtil;
import org.apache.hadoop.hbase.client.Connection;
import org.apache.hadoop.hbase.client.Admin;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.RetriesExhaustedWithDetailsException;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.transactional.TransactionManager;
import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.client.transactional.CommitUnsuccessfulException;
import org.apache.hadoop.hbase.client.transactional.UnknownTransactionException;
import org.apache.hadoop.hbase.client.transactional.HBaseBackedTransactionLogger;
import org.apache.hadoop.hbase.client.transactional.TransactionRegionLocation;
import org.apache.hadoop.hbase.client.transactional.TransState;
import org.apache.hadoop.hbase.client.transactional.UnknownTransactionException;
import org.apache.hadoop.hbase.client.coprocessor.Batch;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TlogDeleteRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TlogDeleteResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TrxRegionService;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.LocalHBaseCluster;
import org.apache.hadoop.hbase.ServerName;
import org.apache.hadoop.hbase.TableExistsException;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.util.Bytes;

import org.apache.hadoop.hbase.ZooKeeperConnectionException;

import org.apache.hadoop.hbase.ipc.BlockingRpcCallback;
import org.apache.hadoop.hbase.ipc.ServerRpcController;
import org.apache.hadoop.hbase.protobuf.ProtobufUtil;
import org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto;
import org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto.MutationType;

import org.apache.hadoop.hbase.regionserver.RegionSplitPolicy;

import com.google.protobuf.ByteString;
import com.google.protobuf.HBaseZeroCopyByteString;

import java.util.Arrays;
import java.util.ArrayList;
import java.util.Collections;
import java.util.ConcurrentModificationException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.NavigableMap;
import java.util.Set;
import java.util.StringTokenizer;

import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.Callable;
import java.util.concurrent.CompletionService;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executor;
import java.util.concurrent.ExecutorCompletionService;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.RejectedExecutionException;

public class TmAuditTlog {

   static final Log LOG = LogFactory.getLog(TmAuditTlog.class);
   private Configuration config;
   private static String TLOG_TABLE_NAME;
   private static final byte[] TLOG_FAMILY = Bytes.toBytes("tf");
   private static final byte[] ASN_STATE = Bytes.toBytes("as");
   private static final byte[] QUAL_TX_STATE = Bytes.toBytes("tx");
   private static HTable[] table;
   private static Connection connection;
   private static HBaseAuditControlPoint tLogControlPoint;
   private static long tLogControlPointNum;
   private static long tLogHashKey;
   private static int  tLogHashShiftFactor;
   private int dtmid;

   // For performance metrics
   private static long[] startTimes;
   private static long[] endTimes;
   private static long[] synchTimes;
   private static long[] bufferSizes;
   private static AtomicInteger  timeIndex;
   private static long   totalWriteTime;
   private static long   totalSynchTime;
   private static long   totalPrepTime;
   private static AtomicLong  totalWrites;
   private static AtomicLong  totalRecords;
   private static long   minWriteTime;
   private static long   minWriteTimeBuffSize;
   private static long   maxWriteTime; 
   private static long   maxWriteTimeBuffSize;
   private static double avgWriteTime;
   private static long   minPrepTime;
   private static long   maxPrepTime;
   private static double avgPrepTime;
   private static long   minSynchTime;
   private static long   maxSynchTime;
   private static double avgSynchTime;
   private static long   minBufferSize;
   private static long   maxBufferSize;
   private static double avgBufferSize;

   private static int     versions;
   private static int     tlogNumLogs;
   private boolean useAutoFlush;
   private static boolean ageCommitted;
   private static boolean forceControlPoint;
   private boolean disableBlockCache;
   private boolean controlPointDeferred;
   private int TlogRetryDelay;
   private int TlogRetryCount;

   private static AtomicLong asn;  // Audit sequence number is the monotonic increasing value of the tLog write

   private static Object tlogAuditLock[];        // Lock for synchronizing access via regions.

   private static Object tablePutLock;            // Lock for synchronizing table.put operations
                                                  // to avoid ArrayIndexOutOfBoundsException
   private static byte filler[];
   public static final int TLOG_SLEEP = 1000;      // One second
   public static final int TLOG_SLEEP_INCR = 5000; // Five seconds
   public static final int TLOG_RETRY_ATTEMPTS = 5;

   /**
    * tlogThreadPool - pool of thread for asynchronous requests
    */
   ExecutorService tlogThreadPool;

   private abstract class TlogCallable implements Callable<Integer>{
      TransactionState transactionState;
      HRegionLocation  location;
      HTable table;
      byte[] startKey;
      byte[] endKey_orig;
      byte[] endKey;

     TlogCallable(TransactionState txState, HRegionLocation location, Connection connection) throws IOException {
        transactionState = txState;
        this.location = location;
        table = new HTable(location.getRegionInfo().getTable(), connection, tlogThreadPool);
        startKey = location.getRegionInfo().getStartKey();
        endKey_orig = location.getRegionInfo().getEndKey();
        endKey = TransactionManager.binaryIncrementPos(endKey_orig, -1);
    }

    public Integer deleteEntriesOlderThanASNX(final byte[] regionName, final long auditSeqNum, final boolean pv_ageCommitted) throws IOException {
       long threadId = Thread.currentThread().getId();
       if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASNX -- ENTRY auditSeqNum: "
            + auditSeqNum + ", thread " + threadId);
       boolean retry = false;
       boolean refresh = false;
       final Scan scan = new Scan(startKey, endKey);

       int retryCount = 0;
       int retrySleep = TLOG_SLEEP;
       do {
             if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASNX -- ENTRY ASN: " + auditSeqNum);
             Batch.Call<TrxRegionService, TlogDeleteResponse> callable =
                new Batch.Call<TrxRegionService, TlogDeleteResponse>() {
                  ServerRpcController controller = new ServerRpcController();
                  BlockingRpcCallback<TlogDeleteResponse> rpcCallback =
                      new BlockingRpcCallback<TlogDeleteResponse>();

                     @Override
                     public TlogDeleteResponse call(TrxRegionService instance) throws IOException {
                        org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TlogDeleteRequest.Builder
                        builder = TlogDeleteRequest.newBuilder();
                        builder.setAuditSeqNum(auditSeqNum);
                        builder.setTransactionId(transactionState.getTransactionId());
                        builder.setScan(ProtobufUtil.toScan(scan));
                        builder.setRegionName(ByteString.copyFromUtf8(Bytes.toString(regionName))); //ByteString.copyFromUtf8(Bytes.toString(regionName)));
                        builder.setAgeCommitted(pv_ageCommitted); 

                        instance.deleteTlogEntries(controller, builder.build(), rpcCallback);
                        return rpcCallback.get();
                    }
                 };

                 Map<byte[], TlogDeleteResponse> result = null;
                 try {
                   if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASNX -- before coprocessorService ASN: " + auditSeqNum
                         + " startKey: " + new String(startKey, "UTF-8") + " endKey: " + new String(endKey, "UTF-8"));
                   result = table.coprocessorService(TrxRegionService.class, startKey, endKey, callable);
                 } catch (Throwable e) {
                   String msg = new String("ERROR occurred while calling deleteTlogEntries coprocessor service in deleteEntriesOlderThanASNX: " + e);
                   LOG.error(msg, e);
                   throw new IOException(msg, e);
                 }
                 if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASNX -- after coprocessorService ASN: " + auditSeqNum
                         + " startKey: " + new String(startKey, "UTF-8") + " result size: " + result.size());

                 if(result.size() != 1) {
                    LOG.error("deleteEntriesOlderThanASNX, received incorrect result size: " + result.size() + " ASN: " + auditSeqNum);
                    throw new IOException("Wrong result size in deleteEntriesOlderThanASNX");
                 }
                 else {
                    // size is 1
                    for (TlogDeleteResponse TD_response : result.values()){
                       if(TD_response.getHasException()) {
                          if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASNX coprocessor exception: "
                               + TD_response.getException());
                          throw new IOException(TD_response.getException());
                       }
                       if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASNX coprocessor deleted count: "
                               + TD_response.getCount());
                    }
                    retry = false;
                 }
              if (refresh) {

               HRegionLocation lv_hrl = table.getRegionLocation(startKey);
               HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
               String          lv_node = lv_hrl.getHostname();
               int             lv_length = lv_node.indexOf('.');

               if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASNX -- location being refreshed : "
                    + location.getRegionInfo().getRegionNameAsString() + "endKey: "
                    + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " for ASN: " + auditSeqNum);
               if(retryCount == TLOG_RETRY_ATTEMPTS) {
                  LOG.error("Exceeded retry attempts (" + retryCount + ") in deleteEntriesOlderThanASNX for ASN: " + auditSeqNum);
                  // We have received our reply in the form of an exception,
                  // so decrement outstanding count and wake up waiters to avoid
                  // getting hung forever
                  transactionState.requestPendingCountDec(true);
                  throw new IOException("Exceeded retry attempts (" + retryCount + ") in deleteEntriesOlderThanASNX for ASN: " + auditSeqNum);
               }

               if (LOG.isWarnEnabled()) LOG.warn("deleteEntriesOlderThanASNX -- " + table.toString() + " location being refreshed");
               if (LOG.isWarnEnabled()) LOG.warn("deleteEntriesOlderThanASNX -- lv_hri: " + lv_hri);
               if (LOG.isWarnEnabled()) LOG.warn("deleteEntriesOlderThanASNX -- location.getRegionInfo(): " + location.getRegionInfo());
               table.getRegionLocation(startKey, true);

               if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASNX -- setting retry, count: " + retryCount);
               refresh = false;
            }
            retryCount++;

            if (retryCount < TLOG_RETRY_ATTEMPTS && retry == true) {
               try {
                  Thread.sleep(retrySleep);
               } catch(InterruptedException ex) {
                  Thread.currentThread().interrupt();
               }

               retrySleep += TLOG_SLEEP_INCR;
            }
       } while (retryCount < TLOG_RETRY_ATTEMPTS && retry == true);
       // We have received our reply so decrement outstanding count
       transactionState.requestPendingCountDec(false);

       if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASNX -- EXIT ASN: " + auditSeqNum);
       return 0;
     } //getTransactionStatesFromIntervalX
   } // TlogCallable

   private class AuditBuffer{
      private ArrayList<Put> buffer;           // Each Put is an audit record

      private AuditBuffer () {
         buffer = new  ArrayList<Put>();
         buffer.clear();

      }

      private void bufferAdd(Put localPut) {
         long threadId = Thread.currentThread().getId();
         if (LOG.isTraceEnabled()) LOG.trace("BufferAdd start in thread " + threadId );
         buffer.add(localPut);
         if (LOG.isTraceEnabled()) LOG.trace("BufferAdd end in thread " + threadId );
      }

      private int bufferSize() {
         int lvSize;
         long threadId = Thread.currentThread().getId();
         if (LOG.isTraceEnabled()) LOG.trace("BufferSize start in thread " + threadId );
         lvSize = buffer.size();
         if (LOG.isTraceEnabled()) LOG.trace("AuditBuffer bufferSize end; returning " + lvSize + " in thread " 
                    +  Thread.currentThread().getId());
         return lvSize;
      }

      private void bufferClear() {
         long threadId = Thread.currentThread().getId();
         if (LOG.isTraceEnabled()) LOG.trace("AuditBuffer bufferClear start in thread " + threadId);
         buffer.clear();
         if (LOG.isTraceEnabled()) LOG.trace("AuditBuffer bufferClear end in thread " + threadId);
      }

      private ArrayList<Put> getBuffer() {
         long threadId = Thread.currentThread().getId();
         if (LOG.isTraceEnabled()) LOG.trace("getBuffer start in thread " + threadId );
         return this.buffer;
      }
   }// End of class AuditBuffer

   public class TmAuditTlogRegionSplitPolicy extends RegionSplitPolicy {

      @Override
      protected boolean shouldSplit(){
         return false;
      }
   }

   public TmAuditTlog (Configuration config, Connection connection) throws IOException, RuntimeException {

      this.config = config;
      this.connection = connection;
      this.dtmid = Integer.parseInt(config.get("dtmid"));
      if (LOG.isTraceEnabled()) LOG.trace("Enter TmAuditTlog constructor for dtmid " + dtmid);
      TLOG_TABLE_NAME = config.get("TLOG_TABLE_NAME");
      int fillerSize = 2;
      int intThreads = 16;
      String numThreads = System.getenv("TM_JAVA_THREAD_POOL_SIZE");
      if (numThreads != null){
         intThreads = Integer.parseInt(numThreads);
      }
      tlogThreadPool = Executors.newFixedThreadPool(intThreads);
      controlPointDeferred = false;

      forceControlPoint = false;
      try {
         String controlPointFlush = System.getenv("TM_TLOG_FLUSH_CONTROL_POINT");
         if (controlPointFlush != null){
            forceControlPoint = (Integer.parseInt(controlPointFlush) != 0);
            if (LOG.isTraceEnabled()) LOG.trace("controlPointFlush != null");
         }
      }
      catch (NumberFormatException e) {
         LOG.error("TM_TLOG_FLUSH_CONTROL_POINT is not valid in ms.env");
      }
      LOG.info("forceControlPoint is " + forceControlPoint);

      useAutoFlush = true;
      try {
         String autoFlush = System.getenv("TM_TLOG_AUTO_FLUSH");
         if (autoFlush != null){
            useAutoFlush = (Integer.parseInt(autoFlush) != 0);
            if (LOG.isTraceEnabled()) LOG.trace("autoFlush != null");
         }
      }
      catch (NumberFormatException e) {
         LOG.error("TM_TLOG_AUTO_FLUSH is not valid in ms.env");
      }
      LOG.info("useAutoFlush is " + useAutoFlush);

      ageCommitted = false;
      try {
         String ageCommittedRecords = System.getenv("TM_TLOG_AGE_COMMITTED_RECORDS");
         if (ageCommittedRecords != null){
            ageCommitted = (Integer.parseInt(ageCommittedRecords) != 0);
            if (LOG.isTraceEnabled()) LOG.trace("ageCommittedRecords != null");
         }
      }
      catch (NumberFormatException e) {
         LOG.error("TM_TLOG_AGE_COMMITTED_RECORDS is not valid in ms.env");
      }
      LOG.info("ageCommitted is " + ageCommitted);

      versions = 10;
      try {
         String maxVersions = System.getenv("TM_TLOG_MAX_VERSIONS");
         if (maxVersions != null){
            versions = (Integer.parseInt(maxVersions) > versions ? Integer.parseInt(maxVersions) : versions);
         }
      }
      catch (NumberFormatException e) {
         LOG.error("TM_TLOG_MAX_VERSIONS is not valid in ms.env");
      }

      TlogRetryDelay = 5000; // 3 seconds
      try {
         String retryDelayS = System.getenv("TM_TLOG_RETRY_DELAY");
         if (retryDelayS != null){
            TlogRetryDelay = (Integer.parseInt(retryDelayS) > TlogRetryDelay ? Integer.parseInt(retryDelayS) : TlogRetryDelay);
         }
      }
      catch (NumberFormatException e) {
         LOG.error("TM_TLOG_RETRY_DELAY is not valid in ms.env");
      }

      TlogRetryCount = 60;
      try {
         String retryCountS = System.getenv("TM_TLOG_RETRY_COUNT");
         if (retryCountS != null){
            TlogRetryCount = (Integer.parseInt(retryCountS) > TlogRetryCount ? Integer.parseInt(retryCountS) : TlogRetryCount);
         }
      }
      catch (NumberFormatException e) {
         LOG.error("TM_TLOG_RETRY_COUNT is not valid in ms.env");
      }

      tlogNumLogs = 1;
      try {
         String numLogs = System.getenv("TM_TLOG_NUM_LOGS");
         if (numLogs != null) {
            tlogNumLogs = Math.max( 1, Integer.parseInt(numLogs));
         }
      }
      catch (NumberFormatException e) {
         LOG.error("TM_TLOG_NUM_LOGS is not valid in ms.env");
      }
      disableBlockCache = false;
      try {
         String blockCacheString = System.getenv("TM_TLOG_DISABLE_BLOCK_CACHE");
         if (blockCacheString != null){
            disableBlockCache = (Integer.parseInt(blockCacheString) != 0);
            if (LOG.isTraceEnabled()) LOG.trace("disableBlockCache != null");
         }
      }
      catch (NumberFormatException e) {
         LOG.error("TM_TLOG_DISABLE_BLOCK_CACHE is not valid in ms.env");
      }
      LOG.info("disableBlockCache is " + disableBlockCache);

      switch (tlogNumLogs) {
        case 1:
          tLogHashKey = 0; // 0b0;
          tLogHashShiftFactor = 63;
          break;
        case 2:
          tLogHashKey = 1; // 0b1;
          tLogHashShiftFactor = 63;
          break;
        case 4:
          tLogHashKey = 3; // 0b11;
          tLogHashShiftFactor = 62;
          break;
        case 8:
          tLogHashKey = 7; // 0b111;
          tLogHashShiftFactor = 61;
          break;
        case 16:
          tLogHashKey = 15; // 0b1111;
          tLogHashShiftFactor = 60;
          break;
        case 32:
          tLogHashKey = 31; // 0b11111;
          tLogHashShiftFactor = 59;
          break;
        default : {
          LOG.error("TM_TLOG_NUM_LOGS must be 1 or a power of 2 in the range 2-32");
          throw new RuntimeException();
        }
      }
      if (LOG.isDebugEnabled()) LOG.debug("TM_TLOG_NUM_LOGS is " + tlogNumLogs);

      HColumnDescriptor hcol = new HColumnDescriptor(TLOG_FAMILY);
      if (disableBlockCache) {
         hcol.setBlockCacheEnabled(false);
      }
      hcol.setMaxVersions(versions);

      filler = new byte[fillerSize];
      Arrays.fill(filler, (byte) ' ');
      startTimes      =    new long[50];
      endTimes        =    new long[50];
      synchTimes      =    new long[50];
      bufferSizes     =    new long[50];
      totalWriteTime  =    0;
      totalSynchTime  =    0;
      totalPrepTime   =    0;
      totalWrites     =    new AtomicLong(0);
      totalRecords    =    new AtomicLong(0);
      minWriteTime    =    1000000000;
      minWriteTimeBuffSize  =    0;
      maxWriteTime    =    0;
      maxWriteTimeBuffSize  =    0;
      avgWriteTime    =    0;
      minPrepTime     =    1000000000;
      maxPrepTime     =    0;
      avgPrepTime     =    0;
      minSynchTime    =    1000000000;
      maxSynchTime    =    0;
      avgSynchTime    =    0;
      minBufferSize   =    1000;
      maxBufferSize   =    0;
      avgBufferSize   =    0;
      timeIndex       =    new AtomicInteger(1);

      asn = new AtomicLong();  // Monotonically increasing count of write operations

      long lvAsn = 0;

      if (LOG.isTraceEnabled()) LOG.trace("try new HBaseAuditControlPoint");
      tLogControlPoint = new HBaseAuditControlPoint(config, connection);

      tlogAuditLock =    new Object[tlogNumLogs];
      table = new HTable[tlogNumLogs];

         // Get the asn from the last control point.  This ignores 
         // any asn increments between the last control point
         // write and a system crash and could result in asn numbers
         // being reused.  However this would just mean that some old 
         // records are held onto a bit longer before cleanup and is safe.
         asn.set(tLogControlPoint.getStartingAuditSeqNum());

      Admin admin  = connection.getAdmin();
      for (int i = 0 ; i < tlogNumLogs; i++) {
         tlogAuditLock[i]      = new Object();
         String lv_tLogName = new String(TLOG_TABLE_NAME + "_LOG_" + Integer.toHexString(i));
         boolean lvTlogExists = admin.tableExists(TableName.valueOf(lv_tLogName));
         if (LOG.isTraceEnabled()) LOG.trace("Tlog table " + lv_tLogName + (lvTlogExists? " exists" : " does not exist" ));
         HTableDescriptor desc = new HTableDescriptor(TableName.valueOf(lv_tLogName));
         desc.addFamily(hcol);

         if (lvTlogExists == false) {
            // Need to prime the asn for future writes
          try {
              if (LOG.isTraceEnabled()) LOG.trace("Creating the table " + lv_tLogName);
               admin.createTable(desc);
               asn.set(1L);  // TLOG didn't exist previously, so start asn at 1
            }
            catch (TableExistsException e) {
               // TODO: ignoring the exception
               LOG.error("Table " + lv_tLogName + " already exists");
            }
         }
         if (LOG.isTraceEnabled()) LOG.trace("try new HTable index " + i);
         table[i] = new HTable(config, desc.getName());

         table[i].setAutoFlushTo(this.useAutoFlush);

      }
      admin.close();

      lvAsn = asn.get();
      // This control point write needs to be delayed until after recovery completes, 
      // but is here as a placeholder
      if (LOG.isTraceEnabled()) LOG.trace("Starting a control point with asn value " + lvAsn);
      tLogControlPointNum = tLogControlPoint.doControlPoint(lvAsn);

      if (LOG.isTraceEnabled()) LOG.trace("Exit constructor()");
      return;
   }

   public long getNextAuditSeqNum(int nid) throws IOException{
      if (LOG.isTraceEnabled()) LOG.trace("getNextAuditSeqNum node: " + nid);
      return tLogControlPoint.getNextAuditSeqNum(nid);
   }

   public static long asnGetAndIncrement () {
      if (LOG.isTraceEnabled()) LOG.trace("asnGetAndIncrement");
      return asn.getAndIncrement();
   }

   public void putSingleRecord(final long lvTransid, final long lvCommitId, final String lvTxState, final Set<TransactionRegionLocation> regions, boolean forced) throws IOException {
      putSingleRecord(lvTransid, lvCommitId, lvTxState, regions, forced, -1);
   }

   public void putSingleRecord(final long lvTransid, final long lvCommitId, final String lvTxState, final Set<TransactionRegionLocation> regions, boolean forced, long recoveryASN) throws IOException {
      long threadId = Thread.currentThread().getId();
      if (LOG.isTraceEnabled()) LOG.trace("putSingleRecord start in thread " + threadId);
      StringBuilder tableString = new StringBuilder();
      String transidString = new String(String.valueOf(lvTransid));
      String commitIdString = new String(String.valueOf(lvCommitId));
      boolean lvResult = true;
      long lvAsn;
      long startSynch = 0;
      long endSynch = 0;
      int lv_lockIndex = 0;
      int lv_TimeIndex = (timeIndex.getAndIncrement() % 50 );
      long lv_TotalWrites = totalWrites.incrementAndGet();
      long lv_TotalRecords = totalRecords.incrementAndGet();
      if (regions != null) {
         // Regions passed in indicate a state record where recovery might be needed following a crash.
         // To facilitate branch notification we translate the regions into table names that can then
         // be translated back into new region names following a restart.  THis allows us to ensure all
         // branches reply prior to cleanup
         Iterator<TransactionRegionLocation> it = regions.iterator();
         List<String> tableNameList = new ArrayList<String>();
         while (it.hasNext()) {
            String name = new String(it.next().getRegionInfo().getTable().getNameAsString());
            if ((name.length() > 0) && (tableNameList.contains(name) != true)){
               // We have a table name not already in the list
               tableNameList.add(name);
               tableString.append(",");
               tableString.append(name);
            }
         }
         if (LOG.isTraceEnabled()) LOG.trace("table names: " + tableString.toString() + " in thread " + threadId);
      }
      //Create the Put as directed by the hashed key boolean
      //create our own hashed key
      long key = (((lvTransid & tLogHashKey) << tLogHashShiftFactor) + (lvTransid & 0xFFFFFFFF));
      lv_lockIndex = (int)(lvTransid & tLogHashKey);
      if (LOG.isTraceEnabled()) LOG.trace("key: " + key + ", hex: " + Long.toHexString(key) + ", transid: " +  lvTransid);
      Put p = new Put(Bytes.toBytes(key));

      if (recoveryASN == -1){
         // This is a normal audit record so we manage the ASN
         lvAsn = asn.getAndIncrement();
      }
      else {
         // This is a recovery audit record so use the ASN passed in
         lvAsn = recoveryASN;
      }
      if (LOG.isTraceEnabled()) LOG.trace("transid: " + lvTransid + " state: " + lvTxState + " ASN: " + lvAsn
              + " in thread " + threadId);
      p.add(TLOG_FAMILY, ASN_STATE, Bytes.toBytes(String.valueOf(lvAsn) + ","
                       + String.valueOf(lvTransid) + "," + lvTxState
                       + "," + Bytes.toString(filler)
                       + "," + String.valueOf(lvCommitId)
                       + "," + tableString.toString()));


      if (recoveryASN != -1){
         // We need to send this to a remote Tlog, not our local one, so open the appropriate table
         if (LOG.isTraceEnabled()) LOG.trace("putSingleRecord writing to remote Tlog for transid: " + lvTransid + " state: " + lvTxState + " ASN: " + lvAsn
                  + " in thread " + threadId);
         Table recoveryTable;
         int lv_ownerNid = (int)TransactionState.getNodeId(lvTransid);
         String lv_tLogName = new String("TRAFODION._DTM_.TLOG" + String.valueOf(lv_ownerNid) + "_LOG_" + Integer.toHexString(lv_lockIndex));
         recoveryTable = connection.getTable(TableName.valueOf(lv_tLogName));

         try {
            recoveryTable.put(p);
         }
         finally {
            recoveryTable.close();
         }
      }
      else {
         // This goes to our local TLOG
         if (LOG.isTraceEnabled()) LOG.trace("TLOG putSingleRecord synchronizing tlogAuditLock[" + lv_lockIndex + "] in thread " + threadId );
         startSynch = System.nanoTime();
            synchronized (tlogAuditLock[lv_lockIndex]) {
               endSynch = System.nanoTime();
               try {
                  if (LOG.isTraceEnabled()) LOG.trace("try table.put " + p );
                  startTimes[lv_TimeIndex] = System.nanoTime();
                  table[lv_lockIndex].put(p);
                  if ((forced) && (useAutoFlush == false)) {
                     if (LOG.isTraceEnabled()) LOG.trace("flushing commits");
                     table[lv_lockIndex].flushCommits();
                  }
                  endTimes[lv_TimeIndex] = System.nanoTime();
               }
               catch (IOException e2){
                  // create record of the exception
                  LOG.error("putSingleRecord Exception ", e2);
                  throw e2;
               }
            } // End global synchronization

         if (LOG.isTraceEnabled()) LOG.trace("TLOG putSingleRecord synchronization complete in thread " + threadId );

         synchTimes[lv_TimeIndex] = endSynch - startSynch;
         totalSynchTime += synchTimes[lv_TimeIndex];
         totalWriteTime += (endTimes[lv_TimeIndex] - startTimes[lv_TimeIndex]);
         if (synchTimes[lv_TimeIndex] > maxSynchTime) {
            maxSynchTime = synchTimes[lv_TimeIndex];
         }
         if (synchTimes[lv_TimeIndex] < minSynchTime) {
            minSynchTime = synchTimes[lv_TimeIndex];
         }
         if ((endTimes[lv_TimeIndex] - startTimes[lv_TimeIndex]) > maxWriteTime) {
            maxWriteTime = (endTimes[lv_TimeIndex] - startTimes[lv_TimeIndex]);
         }
         if ((endTimes[lv_TimeIndex] - startTimes[lv_TimeIndex]) < minWriteTime) {
            minWriteTime = (endTimes[lv_TimeIndex] - startTimes[lv_TimeIndex]);
         }

         if (lv_TimeIndex == 49) {
            timeIndex.set(1);  // Start over so we don't exceed the array size
         }

         if (lv_TotalWrites == 59999) {
            avgWriteTime = (double) (totalWriteTime/lv_TotalWrites);
            avgSynchTime = (double) (totalSynchTime/lv_TotalWrites);
            LOG.info("TLog Audit Write Report\n" + 
                   "                        Total records: "
                       + lv_TotalRecords + " in " + lv_TotalWrites + " write operations\n" +
                   "                        Write time:\n" +
                   "                                     Min:  " 
                       + minWriteTime / 1000 + " microseconds\n" +
                   "                                     Max:  " 
                       + maxWriteTime / 1000 + " microseconds\n" +
                   "                                     Avg:  " 
                       + avgWriteTime / 1000 + " microseconds\n" +
                   "                        Synch time:\n" +
                   "                                     Min:  " 
                       + minSynchTime / 1000 + " microseconds\n" +
                   "                                     Max:  " 
                       + maxSynchTime / 1000 + " microseconds\n" +
                   "                                     Avg:  " 
                       + avgSynchTime / 1000 + " microseconds\n");

            // Start at index 1 since there is no startTimes[0]
            timeIndex.set(1);
            endTimes[0]          = System.nanoTime();
            totalWriteTime       = 0;
            totalSynchTime       = 0;
            totalPrepTime        = 0;
            totalRecords.set(0);
            totalWrites.set(0);
            minWriteTime         = 50000;             // Some arbitrary high value
            maxWriteTime         = 0;
            minWriteTimeBuffSize = 0;
            maxWriteTimeBuffSize = 0;
            minSynchTime         = 50000;             // Some arbitrary high value
            maxSynchTime         = 0;
            minPrepTime          = 50000;            // Some arbitrary high value
            maxPrepTime          = 0;
            minBufferSize        = 1000;             // Some arbitrary high value
            maxBufferSize        = 0;
         }
      }// End else revoveryASN == -1
      if (LOG.isTraceEnabled()) LOG.trace("putSingleRecord exit");
   }

   public static int getRecord(final long lvTransid) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("getRecord start");
      TransState lvTxState = TransState.STATE_NOTX;
      String stateString;
      int lv_lockIndex = (int)(lvTransid & tLogHashKey);
      if (LOG.isTraceEnabled()) LOG.trace("getRecord end; returning " + lvTxState);
      return lvTxState.getValue();
   }

    public static String getRecord(final String transidString) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("getRecord start");
      long lvTransid = Long.parseLong(transidString, 10);
      int lv_lockIndex = (int)(lvTransid & tLogHashKey);
      String lvTxState = new String("NO RECORD");
         Get g;
         //create our own hashed key
         long key = (((lvTransid & tLogHashKey) << tLogHashShiftFactor) + (lvTransid & 0xFFFFFFFF));
         if (LOG.isTraceEnabled()) LOG.trace("key: " + key + " hex: " + Long.toHexString(key));
         g = new Get(Bytes.toBytes(key));
         try {
            Result r = table[lv_lockIndex].get(g);
            byte [] value = r.getValue(TLOG_FAMILY, ASN_STATE);
            StringTokenizer st = new StringTokenizer(value.toString(), ",");
            String asnToken = st.nextElement().toString();
            String transidToken = st.nextElement().toString();
            lvTxState = st.nextElement().toString();
            if (LOG.isTraceEnabled()) LOG.trace("transid: " + transidToken + " state: " + lvTxState);
         } catch (IOException e){
             LOG.error("getRecord IOException: ", e);
             throw e;
         }
      if (LOG.isTraceEnabled()) LOG.trace("getRecord end; returning String:" + lvTxState);
      return lvTxState;
   }
      

   public static boolean deleteRecord(final long lvTransid) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("deleteRecord start " + lvTransid);
      String transidString = new String(String.valueOf(lvTransid));
      int lv_lockIndex = (int)(lvTransid & tLogHashKey);
      Delete d;
      //create our own hashed key
      long key = (((lvTransid & tLogHashKey) << tLogHashShiftFactor) + (lvTransid & 0xFFFFFFFF));
      if (LOG.isTraceEnabled()) LOG.trace("key: " + key + " hex: " + Long.toHexString(key));
      d = new Delete(Bytes.toBytes(key));
      if (LOG.isTraceEnabled()) LOG.trace("deleteRecord  (" + lvTransid + ") ");
      table[lv_lockIndex].delete(d);
      if (LOG.isTraceEnabled()) LOG.trace("deleteRecord - exit");
      return true;
   }

   public boolean deleteAgedEntries(final long lvAsn) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("deleteAgedEntries start:  Entries older than " + lvAsn + " will be removed");
      Table deleteTable;
      for (int i = 0; i < tlogNumLogs; i++) {
         String lv_tLogName = new String(TLOG_TABLE_NAME + "_LOG_" + Integer.toHexString(i));

         if (LOG.isTraceEnabled()) LOG.trace("delete table is: " + lv_tLogName);

         deleteTable = connection.getTable(TableName.valueOf(lv_tLogName));
         try {
            boolean scanComplete = false;
            Scan s = new Scan();
            s.setCaching(100);
            s.setCacheBlocks(false);
            ArrayList<Delete> deleteList = new ArrayList<Delete>();
            ResultScanner ss = deleteTable.getScanner(s);

            try {
               for (Result r : ss) {
                  for (Cell cell : r.rawCells()) {
                     StringTokenizer st =
                        new StringTokenizer(Bytes.toString(CellUtil.cloneValue(cell)), ",");
                     if (LOG.isTraceEnabled()) LOG.trace("string tokenizer success ");
                     if (st.hasMoreElements()) {
                        String asnToken = st.nextElement().toString() ;
                        String transidToken = st.nextElement().toString() ;
                        String stateToken = st.nextElement().toString() ;
                        if ((Long.parseLong(asnToken) < lvAsn) && (stateToken.equals("FORGOTTEN"))) {
                           String rowKey = new String(r.getRow());
                           Delete del = new Delete(r.getRow());
                           if (LOG.isTraceEnabled()) LOG.trace("adding transid: " + transidToken + " to delete list");
                           deleteList.add(del);
                        }
                        else if ((Long.parseLong(asnToken) < lvAsn) &&
                                (stateToken.equals("COMMITTED") || stateToken.equals("ABORTED"))) {
                           if (ageCommitted) {
                              Delete del = new Delete(r.getRow());
                              if (LOG.isTraceEnabled()) LOG.trace("adding transid: " + transidToken + " to delete list");
                              deleteList.add(del);
                           }
                           else {
                              Get get = new Get(r.getRow());
                              get.setMaxVersions(versions);  // will return last n versions of row
                              Result lvResult = deleteTable.get(get);
                              List<Cell> list = lvResult.getColumnCells(TLOG_FAMILY, ASN_STATE);  // returns all versions of this column
                              for (Cell element : list) {
                                 StringTokenizer stok = new StringTokenizer(Bytes.toString(CellUtil.cloneValue(element)), ",");
                                 if (stok.hasMoreElements()) {
                                    if (LOG.isTraceEnabled()) LOG.trace("Performing secondary search on (" + transidToken + ")");
                                    asnToken = stok.nextElement().toString() ;
                                    transidToken = stok.nextElement().toString() ;
                                    stateToken = stok.nextElement().toString() ;
                                    if ((Long.parseLong(asnToken) < lvAsn) && (stateToken.equals("FORGOTTEN"))) {
                                       Delete del = new Delete(r.getRow());
                                       if (LOG.isTraceEnabled()) LOG.trace("Secondary search found new delete - adding (" + transidToken + ") with asn: " + asnToken + " to delete list");
                                       deleteList.add(del);
                                       break;
                                    }
                                    else {
                                       if (LOG.isTraceEnabled()) LOG.trace("Secondary search skipping entry with asn: " + asnToken + ", state: " 
                                                + stateToken + ", transid: " + transidToken );
                                    }
                                 }
                              }
                           }
                        } else {
                           if (LOG.isTraceEnabled()) LOG.trace("deleteAgedEntries skipping asn: " + asnToken + ", transid: " 
                                     + transidToken + ", state: " + stateToken);
                        }
                     }
                  }
              }
           }
           finally {
              if (LOG.isTraceEnabled()) LOG.trace("deleteAgedEntries closing ResultScanner");
              ss.close();
           }
           if (LOG.isTraceEnabled()) LOG.trace("attempting to delete list with " + deleteList.size()
                   + " elements from table " + lv_tLogName);
           deleteTable.delete(deleteList);
        }
        finally {
              deleteTable.close();
        }
     }
     if (LOG.isTraceEnabled()) LOG.trace("deleteAgedEntries - exit");
     return true;
   }

   public long writeControlPointRecords (final Map<Long, TransactionState> map) throws IOException {
      int lv_lockIndex;
      int cpWrites = 0;
      long startTime = System.nanoTime();
      long endTime;

      if (LOG.isTraceEnabled()) LOG.trace("Tlog " + getTlogTableNameBase()
           + " writeControlPointRecords start with map size " + map.size());

      try {
        for (Map.Entry<Long, TransactionState> e : map.entrySet()) {
         try {
            Long transid = e.getKey();
            lv_lockIndex = (int)(transid & tLogHashKey);
            TransactionState value = e.getValue();
            if (value.getStatus().equals("COMMITTED")){
               if (LOG.isTraceEnabled()) LOG.trace("writeControlPointRecords adding record for trans (" + transid + ") : state is " + value.getStatus());
               cpWrites++;
               putSingleRecord(transid, value.getCommitId(), value.getStatus(), value.getParticipatingRegions(), forceControlPoint);
            }
         }
         catch (IOException ex) {
            LOG.error("formatRecord Exception ", ex);
            throw ex;
         }
        }
      } catch (ConcurrentModificationException cme){
          LOG.info("writeControlPointRecords ConcurrentModificationException;  delaying control point ", cme);
          // Return the current value rather than incrementing this interval.
          controlPointDeferred = true;
          return tLogControlPoint.getCurrControlPt() - 1;
      } 

      endTime = System.nanoTime();
      if (LOG.isDebugEnabled()) LOG.debug("TLog Control Point Write Report\n" + 
                   "                        Total records: " 
                       +  map.size() + " in " + cpWrites + " write operations\n" +
                   "                        Write time: " + (endTime - startTime) / 1000 + " microseconds\n" );
  
      if (LOG.isTraceEnabled()) LOG.trace("writeControlPointRecords exit ");
      return -1L;

   }

   public long addControlPoint (final Map<Long, TransactionState> map) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("addControlPoint start with map size " + map.size());
      long lvCtrlPt = 0L;
      long agedAsn;  // Writes older than this audit seq num will be deleted
      long lvAsn;    // local copy of the asn
      long key;
      boolean success = false;

      if (controlPointDeferred) {
         // We deferred the control point once already due to concurrency.  We'll synchronize this timeIndex
         synchronized (map) {
            if (LOG.isTraceEnabled()) LOG.trace("Writing synchronized control point records");
            lvAsn = writeControlPointRecords(map);
         }

         controlPointDeferred = false;
      }
      else {
         lvAsn = writeControlPointRecords(map);
         if (lvAsn != -1L){
            return lvAsn;
         }
      }

         lvAsn = asn.getAndIncrement();
         if (LOG.isTraceEnabled()) LOG.trace("lvAsn reset to: " + lvAsn);

         // Write the control point interval and the ASN to the control point table
         lvCtrlPt = tLogControlPoint.doControlPoint(lvAsn); 

         if ((lvCtrlPt - 5) > 0){  // We'll keep 5 control points of audit
            try {
               agedAsn = tLogControlPoint.getRecord(String.valueOf(lvCtrlPt - 5));
               if (agedAsn > 0){
                  try {
                     if (LOG.isTraceEnabled()) LOG.trace("Attempting to remove TLOG writes older than asn " + agedAsn);
//                     deleteAgedEntries(agedAsn);
                     deleteEntriesOlderThanASN(agedAsn, ageCommitted);
                  }
                  catch (IOException e){
                     LOG.error("deleteAgedEntries Exception ", e);
                     throw e;
                  }
               }
               try {
                  tLogControlPoint.deleteAgedRecords(lvCtrlPt - 5);
               }
               catch (IOException e){
                  // TODO: ignoring the exception
                  LOG.error("addControlPoint - control point record not found ");
               }
            }
            catch (IOException e){
               LOG.error("addControlPoint IOException ", e);
               throw e;
            }
         }
      if (LOG.isTraceEnabled()) LOG.trace("addControlPoint returning " + lvCtrlPt);
      return lvCtrlPt;
   } 

   public void getTransactionState (TransactionState ts) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("getTransactionState start; transid: " + ts.getTransactionId());

      // This request might be for a transaction not originating on this node, so we need to open
      // the appropriate Tlog
      Table unknownTransactionTable;
      long lvTransid = ts.getTransactionId();
      int lv_ownerNid = (int)TransactionState.getNodeId(lvTransid);
      int lv_lockIndex = (int)(lvTransid & tLogHashKey);
      String lv_tLogName = new String("TRAFODION._DTM_.TLOG" + String.valueOf(lv_ownerNid) + "_LOG_" + Integer.toHexString(lv_lockIndex));
      if (LOG.isTraceEnabled()) LOG.trace("getTransactionState reading from: " + lv_tLogName);

      unknownTransactionTable = connection.getTable(TableName.valueOf(lv_tLogName));

         String transidString = new String(String.valueOf(lvTransid));
         Get g;
         long key = (((lvTransid & tLogHashKey) << tLogHashShiftFactor) + (lvTransid & 0xFFFFFFFF));
         if (LOG.isTraceEnabled()) LOG.trace("key: " + key + ", hexkey: " + Long.toHexString(key) + ", transid: " +  lvTransid);
         g = new Get(Bytes.toBytes(key));
         TransState lvTxState = TransState.STATE_NOTX;
         String stateString = "";
         String transidToken = "";
         String commitIdToken = "";
            Result r = unknownTransactionTable.get(g);
            if (r == null) {
               if (LOG.isTraceEnabled()) LOG.trace("getTransactionState: tLog result is null: " + transidString);
            }
            if (r.isEmpty()) {
               if (LOG.isTraceEnabled()) LOG.trace("getTransactionState: tLog empty result: " + transidString);
            }
            byte [] value = r.getValue(TLOG_FAMILY, ASN_STATE);
            if (value == null) {
               ts.setStatus(TransState.STATE_NOTX);
               if (LOG.isTraceEnabled()) LOG.trace("getTransactionState: tLog value is null: " + transidString);
               return;
            }
            if (value.length == 0) {
               ts.setStatus(TransState.STATE_NOTX);
               if (LOG.isTraceEnabled()) LOG.trace("getTransactionState: tLog transaction not found: " + transidString);
               return;
            }
            ts.clearParticipatingRegions();
            String recordString =  new String (Bytes.toString(value));
            StringTokenizer st = new StringTokenizer(recordString, ",");
            if (st.hasMoreElements()) {
               String asnToken = st.nextElement().toString();
               transidToken = st.nextElement().toString();
               stateString = st.nextElement().toString();
               if (LOG.isTraceEnabled()) LOG.trace("getTransactionState: transaction: " + transidToken + " stateString is: " + stateString);
            }
            if (stateString.compareTo("COMMITTED") == 0){
               lvTxState = TransState.STATE_COMMITTED;
            }
            else if (stateString.compareTo("ABORTED") == 0){
               lvTxState = TransState.STATE_ABORTED;
            }
            else if (stateString.compareTo("ACTIVE") == 0){
               lvTxState = TransState.STATE_ACTIVE;
            }
            else if (stateString.compareTo("PREPARED") == 0){
               lvTxState = TransState.STATE_PREPARED;
            }
            else if (stateString.compareTo("NOTX") == 0){
               lvTxState = TransState.STATE_NOTX;
            }
            else if (stateString.compareTo("FORGOTTEN") == 0){
               // Need to get the previous state record so we know how to drive the regions
               String keyS = new String(r.getRow());
               Get get = new Get(r.getRow());
               get.setMaxVersions(versions);  // will return last n versions of row
               Result lvResult = unknownTransactionTable.get(get);
               // byte[] b = lvResult.getValue(TLOG_FAMILY, ASN_STATE);  // returns current version of value
               List<Cell> list = lvResult.getColumnCells(TLOG_FAMILY, ASN_STATE);  // returns all versions of this column
               for (Cell element : list) {
                  String stringValue = new String(CellUtil.cloneValue(element));
                  st = new StringTokenizer(stringValue, ",");
                  if (st.hasMoreElements()) {
                     if (LOG.isTraceEnabled()) LOG.trace("Performing secondary search on (" + transidToken + ")");
                     String asnToken = st.nextElement().toString() ;
                     transidToken = st.nextElement().toString() ;
                     String stateToken = st.nextElement().toString() ;
                     if ((stateToken.compareTo("COMMITTED") == 0) || (stateToken.compareTo("ABORTED") == 0)) {
                         String rowKey = new String(r.getRow());
                         if (LOG.isTraceEnabled()) LOG.trace("Secondary search found record for (" + transidToken + ") with state: " + stateToken);
                         lvTxState = (stateToken.compareTo("COMMITTED") == 0 ) ? TransState.STATE_COMMITTED : TransState.STATE_ABORTED;
                         break;
                     }
                     else {
                         if (LOG.isTraceEnabled()) LOG.trace("Secondary search skipping entry for (" + 
                                    transidToken + ") with state: " + stateToken );
                     }
                  }
               }
            }
            else if (stateString.compareTo("ABORTING") == 0){
               lvTxState = TransState.STATE_ABORTING;
            }
            else if (stateString.compareTo("COMMITTING") == 0){
               lvTxState = TransState.STATE_COMMITTING;
            }
            else if (stateString.compareTo("PREPARING") == 0){
               lvTxState = TransState.STATE_PREPARING;
            }
            else if (stateString.compareTo("FORGETTING") == 0){
               lvTxState = TransState.STATE_FORGETTING;
            }
            else if (stateString.compareTo("FORGETTING_HEUR") == 0){
               lvTxState = TransState.STATE_FORGETTING_HEUR;
            }
            else if (stateString.compareTo("BEGINNING") == 0){
               lvTxState = TransState.STATE_BEGINNING;
            }
            else if (stateString.compareTo("HUNGCOMMITTED") == 0){
               lvTxState = TransState.STATE_HUNGCOMMITTED;
            }
            else if (stateString.compareTo("HUNGABORTED") == 0){
               lvTxState = TransState.STATE_HUNGABORTED;
            }
            else if (stateString.compareTo("IDLE") == 0){
               lvTxState = TransState.STATE_IDLE;
            }
            else if (stateString.compareTo("FORGOTTEN_HEUR") == 0){
               lvTxState = TransState.STATE_FORGOTTEN_HEUR;
            }
            else if (stateString.compareTo("ABORTING_PART2") == 0){
               lvTxState = TransState.STATE_ABORTING_PART2;
            }
            else if (stateString.compareTo("TERMINATING") == 0){
               lvTxState = TransState.STATE_TERMINATING;
            }
            else {
               lvTxState = TransState.STATE_BAD;
            }

            // get past the filler
            st.nextElement();

            commitIdToken = st.nextElement().toString();
            ts.setCommitId(Long.parseLong(commitIdToken));

            // Load the TransactionState object up with regions
            while (st.hasMoreElements()) {
               String tableNameToken = st.nextToken();
               HTable table = new HTable(config, tableNameToken);
               NavigableMap<HRegionInfo, ServerName> regions = table.getRegionLocations();
               Iterator<Map.Entry<HRegionInfo, ServerName>> it =  regions.entrySet().iterator();
               while(it.hasNext()) { // iterate entries.
                  NavigableMap.Entry<HRegionInfo, ServerName> pairs = it.next();
                  HRegionInfo regionKey = pairs.getKey();
                  if (LOG.isTraceEnabled()) LOG.trace("getTransactionState: transaction: " + transidToken + " adding region: " + regionKey.getRegionNameAsString());
                  ServerName serverValue = regions.get(regionKey);
                  String hostAndPort = new String(serverValue.getHostAndPort());
                  StringTokenizer tok = new StringTokenizer(hostAndPort, ":");
                  String hostName = new String(tok.nextElement().toString());
                  int portNumber = Integer.parseInt(tok.nextElement().toString());
                  TransactionRegionLocation loc = new TransactionRegionLocation(regionKey, serverValue);
                  ts.addRegion(loc);
              }
            }
            ts.setStatus(lvTxState);

            if (LOG.isTraceEnabled()) LOG.trace("getTransactionState: returning transid: " + ts.getTransactionId() + " state: " + lvTxState);
      if (LOG.isTraceEnabled()) LOG.trace("getTransactionState end transid: " + ts.getTransactionId());
      return;
   }

   public String getTlogTableNameBase(){
      return TLOG_TABLE_NAME;
   }

   /**
   * Method  : deleteEntriesOlderThanASN
   * Params  : pv_ASN  - ASN before which all audit records will be deleted
   * Return  : void
   * Purpose : Delete transaction records which are no longer needed
   */
   public void deleteEntriesOlderThanASN(final long pv_ASN, final boolean pv_ageCommitted) throws IOException {
      int loopIndex = 0;
      long threadId = Thread.currentThread().getId();
      // This TransactionState object is just a mechanism to keep track of the asynch rpc calls
      // send to regions in order to retrience the desired set of transactions
      TransactionState transactionState = new TransactionState(0);
      CompletionService<Integer> compPool = new ExecutorCompletionService<Integer>(tlogThreadPool);

         if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASN: "
              + pv_ASN + ", in thread: " + threadId);
         List<HRegionLocation> regionList;

         // For every Tlog table for this node
         for (int index = 0; index < tlogNumLogs; index++) {
            String lv_tLogName = new String("TRAFODION._DTM_.TLOG" + String.valueOf(this.dtmid) + "_LOG_" + Integer.toHexString(index));
            regionList = connection.getRegionLocator(TableName.valueOf(lv_tLogName)).getAllRegionLocations();
            loopIndex++;
            int regionIndex = 0;
            // For every region in this table
            for (HRegionLocation location : regionList) {
               regionIndex++;
               final byte[] regionName = location.getRegionInfo().getRegionName();
               compPool.submit(new TlogCallable(transactionState, location, connection) {
                  public Integer call() throws IOException {
                     if (LOG.isTraceEnabled()) LOG.trace("before deleteEntriesOlderThanASNX() ASN: "
                         + pv_ASN);
                     return deleteEntriesOlderThanASNX(regionName, pv_ASN, pv_ageCommitted);
                  }
               });
               boolean loopBack = false;
               do 
               {
               try {
                   loopBack = false;
                   int partialResult = compPool.take().get();
                   if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASN partial result: " + partialResult
                		      + " loopIndex " + loopIndex + " regionIndex " + regionIndex);
                }
                catch (InterruptedException e2) {
                   LOG.error("exception retieving reply in deleteEntriesOlderThanASN for interval ASN: " + pv_ASN
                           + " ", e2);
                   loopBack = true;
                }
                catch (ExecutionException ee) {
                   LOG.error("Execution exception", ee);
                   throw new IOException(ee);
                }
                } while (loopBack); 
            }
         }

      if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASN tlog callable requests completed in thread "
            + threadId);
      return;
  }
}
