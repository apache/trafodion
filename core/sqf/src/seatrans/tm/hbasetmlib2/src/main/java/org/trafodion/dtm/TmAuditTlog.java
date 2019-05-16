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
import org.apache.hadoop.hbase.client.coprocessor.Batch;
import org.apache.hadoop.hbase.client.Admin;
import org.apache.hadoop.hbase.client.BufferedMutator;
import org.apache.hadoop.hbase.client.Durability;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.RegionLocator;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.RetriesExhaustedWithDetailsException;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.client.transactional.CommitUnsuccessfulException;
import org.apache.hadoop.hbase.client.transactional.HBaseBackedTransactionLogger;
import org.apache.hadoop.hbase.client.transactional.TransactionManager;
import org.apache.hadoop.hbase.client.transactional.TransactionRegionLocation;
import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.client.transactional.TransState;
import org.apache.hadoop.hbase.client.transactional.TrafodionLocationList;
import org.apache.hadoop.hbase.client.transactional.UnknownTransactionException;
import org.apache.hadoop.hbase.client.coprocessor.Batch;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TlogDeleteRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TlogDeleteResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TlogTransactionStatesFromIntervalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TlogTransactionStatesFromIntervalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TlogWriteRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TlogWriteResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TrxRegionService;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HConstants;
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
import org.apache.hadoop.hbase.ipc.FailedServerException;
import org.apache.hadoop.hbase.ipc.ServerRpcController;
import org.apache.hadoop.hbase.protobuf.ProtobufUtil;
import org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto;
import org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto.MutationType;

import org.apache.hadoop.hbase.ZooKeeperConnectionException;

import org.apache.hadoop.hbase.ipc.BlockingRpcCallback;
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

import org.trafodion.dtm.HBaseAuditControlPoint;

public class TmAuditTlog {

   static final Log LOG = LogFactory.getLog(TmAuditTlog.class);
   private Configuration config;
   private static String TLOG_TABLE_NAME;
   private static final byte[] TLOG_FAMILY = Bytes.toBytes("tf");
   private static final byte[] ASN_STATE = Bytes.toBytes("as");
   private static final byte[] QUAL_TX_STATE = Bytes.toBytes("tx");
   private Table table;
   private static Connection connection;
   private HBaseAuditControlPoint tLogControlPoint;
   private long tLogControlPointNum;
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
   private static boolean ageCommitted;
   private static boolean forceControlPoint;
   private boolean disableBlockCache;
   private boolean controlPointDeferred;
   private int TlogRetryDelay;
   private int TlogRetryCount;

   private static AtomicLong asn;  // Audit sequence number is the monotonic increasing value of the tLog write

   private static byte filler[];
   public static final int TLOG_SLEEP = 1000;      // One second
   public static final int TLOG_SLEEP_INCR = 5000; // Five seconds
   public static final int TLOG_RETRY_ATTEMPTS = 5;

   private static int myClusterId = 0;

   /**
    * tlogThreadPool - pool of thread for asynchronous requests
    */
   ExecutorService tlogThreadPool;

   private abstract class TlogCallable implements Callable<Integer>{
      HRegionLocation  location;
      Table table;
      byte[] startKey;
      byte[] endKey_orig;
      byte[] endKey;

     TlogCallable(HRegionLocation location, Connection connection) throws IOException {
        this.location = location;
        try {
           this.table = connection.getTable(location.getRegionInfo().getTable());
        } catch(IOException e) {
           LOG.error("Error obtaining Table instance ", e);
           this.table = null;
        }
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
       scan.setCacheBlocks(false);

       int retryCount = 0;
       int retrySleep = TLOG_SLEEP;
       do {
          try {
             if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASNX -- ENTRY ASN: " + auditSeqNum);
                 Map<byte[], TlogDeleteResponse> result = null;
                 try {
                   if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASNX -- before coprocessorService ASN: " + auditSeqNum
                         + " startKey: " + new String(startKey, "UTF-8") + " endKey: " + new String(endKey, "UTF-8"));
                   result = table.coprocessorService(TrxRegionService.class,
                      startKey,
                      endKey,
                      new Batch.Call<TrxRegionService, TlogDeleteResponse>() {

                      @Override
                      public TlogDeleteResponse call(TrxRegionService instance) throws IOException {
                         BlockingRpcCallback<TlogDeleteResponse> rpcCallback =
                         new BlockingRpcCallback<TlogDeleteResponse>();

                         org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TlogDeleteRequest.Builder
                         builder = TlogDeleteRequest.newBuilder();
                         builder.setAuditSeqNum(auditSeqNum);
                         builder.setScan(ProtobufUtil.toScan(scan));
                         builder.setRegionName(ByteString.copyFromUtf8(Bytes.toString(regionName))); //ByteString.copyFromUtf8(Bytes.toString(regionName)));
                         builder.setAgeCommitted(pv_ageCommitted);

                         instance.deleteTlogEntries(null, builder.build(), rpcCallback);
                         return rpcCallback.get();
                      }
                   });
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
              } catch (Exception e) {
                 LOG.error("deleteEntriesOlderThanASNX retrying due to Exception: ", e);
                 refresh = true;
                 retry = true;
              }
              if (refresh) {

               RegionLocator   rl = connection.getRegionLocator(table.getName());
               location = rl.getRegionLocation(startKey, true);

               if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASNX -- location refreshed : "
                    + location.getRegionInfo().getRegionNameAsString() + "endKey: "
                    + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " for ASN: " + auditSeqNum);
               if(retryCount == TLOG_RETRY_ATTEMPTS) {
                  LOG.error("Exceeded retry attempts (" + retryCount + ") in deleteEntriesOlderThanASNX for ASN: " + auditSeqNum);
                  IOException ie = new IOException("Exceeded retry attempts (" + retryCount + ") in deleteEntriesOlderThanASNX for ASN: " + auditSeqNum);
                  throw ie;
               }
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

       if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASNX -- EXIT ASN: " + auditSeqNum);
       return 0;
     } //deleteEntriesOlderThanASNX
   } // TlogCallable

   /**
    * TlogCallable1  :  inner class for creating asynchronous requests
    */
   private abstract class TlogCallable1 implements Callable<Integer>{
      TransactionState transactionState;
      HRegionLocation  location;
      Table table;
      byte[] startKey;
      byte[] endKey_orig;
      byte[] endKey;

      TlogCallable1(TransactionState txState, HRegionLocation location, Connection connection) {
         transactionState = txState;
         this.location = location;
         try {
             this.table = connection.getTable(location.getRegionInfo().getTable());
         } catch(IOException e) {
            LOG.error("Error obtaining Table instance ", e);
            this.table = null;
         }
         startKey = location.getRegionInfo().getStartKey();
         endKey_orig = location.getRegionInfo().getEndKey();
         endKey = TransactionManager.binaryIncrementPos(endKey_orig, -1);
      }

     /**
      * Method  : doTlogWriteX
      * Params  : regionName - name of Region
      *           transactionId - transaction identifier
      * Return  : Always 0, can ignore
      * Purpose : write commit/abort state record for a given transaction
      */
      public Integer doTlogWriteX(final byte[] regionName, final long transactionId, final long commitId, final Put put) throws IOException {
         long threadId = Thread.currentThread().getId();
         if (LOG.isTraceEnabled()) LOG.trace("doTlogWriteX -- ENTRY txid: " + transactionId + ", clusterId: " + myClusterId + ", thread " + threadId
        		             + ", put: " + put.toString());
         boolean retry = false;
         boolean refresh = false;

         int retryCount = 0;
         int retrySleep = TLOG_SLEEP;

         do {
            try {
              if (LOG.isTraceEnabled()) LOG.trace("doTlogWriteX -- try txid: " + transactionId + " in thread " + threadId);
              Batch.Call<TrxRegionService, TlogWriteResponse> callable =
                 new Batch.Call<TrxRegionService, TlogWriteResponse>() {
                    ServerRpcController controller = new ServerRpcController();
                    BlockingRpcCallback<TlogWriteResponse> rpcCallback = new BlockingRpcCallback<TlogWriteResponse>();

                    @Override
                    public TlogWriteResponse call(TrxRegionService instance) throws IOException {
                       org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TlogWriteRequest.Builder builder = TlogWriteRequest.newBuilder();
                       builder.setTransactionId(transactionId);
                       builder.setCommitId(commitId);
                       builder.setRegionName(ByteString.copyFromUtf8(Bytes.toString(regionName))); //ByteString.copyFromUtf8(Bytes.toString(regionName)));
                       builder.setFamily(HBaseZeroCopyByteString.wrap(TLOG_FAMILY));
                       builder.setQualifier(HBaseZeroCopyByteString.wrap(ASN_STATE));
                       MutationProto m1 = ProtobufUtil.toMutation(MutationType.PUT, put);
                       builder.setPut(m1);

                       instance.putTlog(controller, builder.build(), rpcCallback);
                       long threadId = Thread.currentThread().getId();
                       if (LOG.isTraceEnabled()) LOG.trace("TlogWrite -- sent for txid: " + transactionId + " in thread " + threadId);
                       TlogWriteResponse response = rpcCallback.get();
                       if (LOG.isTraceEnabled()) LOG.trace("TlogWrite -- response received (" + response + ") for txid: "
                               + transactionId + " in thread " + threadId );
                       return response;
                    }
                 };

              Map<byte[], TlogWriteResponse> result = null;
              try {
                 if (LOG.isTraceEnabled()) LOG.trace("doTlogWriteX -- before coprocessorService txid: " + transactionId + " table: "
                             + table.toString() + " startKey: " + new String(startKey, "UTF-8") + " endKey: " + new String(endKey, "UTF-8"));
                 result = table.coprocessorService(TrxRegionService.class, startKey, endKey, callable);
                 if (LOG.isTraceEnabled()) LOG.trace("doTlogWriteX -- after coprocessorService txid: " + transactionId);
              } catch (Throwable e) {
                 String msg = "ERROR occurred while calling doTlogWriteX coprocessor service in doTlogWriteX";
                 LOG.error(msg + ":" , e);
                 throw new IOException(e);
              }
              if(result.size() != 1) {
                 LOG.error("doTlogWriteX, received incorrect result size: " + result.size() + " txid: " + transactionId);
                 throw new IOException("Wrong result size in doWriteTlogX");
              }
              else {
                 // size is 1
                 for (TlogWriteResponse tlw_response : result.values()){
                    if(tlw_response.getHasException()) {
                       String exceptionString = new String (tlw_response.getException().toString());
                       if (LOG.isTraceEnabled()) LOG.trace("doTlogWriteX coprocessor exception: " + tlw_response.getException());
                       throw new Exception(tlw_response.getException());
                    }
                 }
                 retry = false;
              }
            }
            catch (Exception e) {
              LOG.error("doTlogWriteX retrying due to Exception: " + e);
              refresh = true;
              retry = true;
            }
            if (refresh) {

               RegionLocator   rl = connection.getRegionLocator(table.getName());
               location = rl.getRegionLocation(startKey, true);

               if (LOG.isTraceEnabled()) LOG.trace("doTlogWriteX -- location refreshed : "
            		   + location.getRegionInfo().getRegionNameAsString() + "endKey: "
                       + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " for transaction: " + transactionId);
               if(retryCount == TLOG_RETRY_ATTEMPTS) {
                  LOG.error("Exceeded retry attempts (" + retryCount + ") in doTlogWriteX for transaction: " + transactionId);
                  // We have received our reply in the form of an exception,
                  // so decrement outstanding count and wake up waiters to avoid
                  // getting hung forever
                  IOException ie = new IOException("Exceeded retry attempts (" + retryCount + ") in doTlogWriteX for transaction: " + transactionId);
                  transactionState.requestPendingCountDec(ie);
                  throw ie;
               }

               if (LOG.isTraceEnabled()) LOG.trace("doTlogWriteX -- setting retry, count: " + retryCount);
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
         transactionState.requestPendingCountDec(null);

         if (LOG.isTraceEnabled()) LOG.trace("doTlogWriteX -- EXIT txid: " + transactionId);
         return 0;
      }//doTlogWriteX
   }//TlogCallable1

   private abstract class TlogCallable2 implements Callable<ArrayList<TransactionState>>{
      Table table;
      byte[] startKey = HConstants.EMPTY_BYTE_ARRAY;
      byte[] endKey = HConstants.EMPTY_BYTE_ARRAY;

      TlogCallable2(Table targetTable, Connection connection) {
        this.table = targetTable;
      }

      public ArrayList<TransactionState> getTransactionStatesFromIntervalX(final long clusterId, final long auditSeqNum) throws IOException {
         boolean retry = false;
         boolean refresh = false;
         final Scan scan = new Scan(startKey, endKey); // Null start and end keys covers all regions
         scan.setCaching(100);
         scan.setCacheBlocks(false);

         int retryCount = 0;
         int retrySleep = TLOG_SLEEP;
         ArrayList<TransactionState> transList = new ArrayList<TransactionState>();
         do {
           try {
              if (LOG.isTraceEnabled()) LOG.trace("getTransactionStatesFromIntervalX -- ENTRY ASN: " + auditSeqNum);
              Batch.Call<TrxRegionService, TlogTransactionStatesFromIntervalResponse> request =
                 new Batch.Call<TrxRegionService, TlogTransactionStatesFromIntervalResponse>() {
                   BlockingRpcCallback<TlogTransactionStatesFromIntervalResponse> rpcCallback =
                      new BlockingRpcCallback<TlogTransactionStatesFromIntervalResponse>();

                      @Override
                      public TlogTransactionStatesFromIntervalResponse call(TrxRegionService instance) throws IOException {
                        org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TlogTransactionStatesFromIntervalRequest.Builder builder =
                                TlogTransactionStatesFromIntervalRequest.newBuilder();
                        builder.setClusterId(clusterId);
                        builder.setAuditSeqNum(auditSeqNum);
                        builder.setScan(ProtobufUtil.toScan(scan));
                        instance.getTransactionStatesPriorToAsn(null, builder.build(), rpcCallback);
                        return rpcCallback.get();
                    }
                 };

                 Map<byte[], TlogTransactionStatesFromIntervalResponse> result = null;
                 try {
                   if (LOG.isTraceEnabled()) LOG.trace("getTransactionStatesFromIntervalX -- before coprocessorService ASN: " + auditSeqNum
                                       + " startKey: " + new String(startKey, "UTF-8") + " endKey: " + new String(endKey, "UTF-8"));
                   result = table.coprocessorService(TrxRegionService.class, startKey, endKey, request); // null keys cover all regions
                 } catch (Throwable e) {
                    String msg = "ERROR occurred while calling getTransactionStatesFromIntervalX coprocessor service in getTransactionStatesFromIntervalX";
                    LOG.error(msg + ":" + e);
                    throw new Exception(msg);
                 }
                 if (LOG.isTraceEnabled()) LOG.trace("getTransactionStatesFromIntervalX -- after coprocessorService ASN: " + auditSeqNum
                         + " startKey: " + new String(startKey, "UTF-8") + " result size: " + result.size());

                 if(result.size() >= 1) {
                    LOG.info("getTransactionStatesFromInterval: result size is " + result.size());
                    org.apache.hadoop.hbase.protobuf.generated.ClientProtos.Result row = null;
                    for (TlogTransactionStatesFromIntervalResponse TSFI_response : result.values()){

                       if(TSFI_response.getHasException()) {
                          if (LOG.isTraceEnabled()) LOG.trace("getTransactionStatesFromIntervalX coprocessor exception: "
                               + TSFI_response.getException());
                          throw new Exception(TSFI_response.getException());
                       }

                       long count = TSFI_response.getCount();
                       LOG.info("getTransactionStatesFromInterval: count is " + count);
                       for (int i = 0; i < count; i++){

                          // Here we get the transaction records returned and create new TransactionState objects
                          // Each 'row' is a return result from a region
                          row = TSFI_response.getResult(i);
                          Result rowResult = ProtobufUtil.toResult(row);
                          if (!rowResult.isEmpty()) {
                             byte [] value = rowResult.getValue(TLOG_FAMILY, ASN_STATE);
                             if (value == null) {
                                if (LOG.isTraceEnabled()) LOG.trace("getTransactionStatesFromInterval: tLog value is null, continuing");
                                continue;
                             }
                             if (value.length == 0) {
                                if (LOG.isTraceEnabled()) LOG.trace("getTransactionStatesFromInterval: tLog value.length is 0, continuing");
                                continue;
                             }
                             TransactionState ts;
                             TransState lvTxState = TransState.STATE_NOTX;
                             StringTokenizer st = new StringTokenizer(Bytes.toString(value), ",");
                             String stateString = new String("NOTX");
                             String transidToken;
                             if (! st.hasMoreElements()) {
                                continue;
                             }
                             String asnToken = st.nextElement().toString();
                             transidToken = st.nextElement().toString();
                             stateString = st.nextElement().toString();
                             long lvTransid = Long.parseLong(transidToken, 10);
                             ts =  new TransactionState(lvTransid);
                             ts.setRecoveryASN(Long.parseLong(asnToken, 10));
                             ts.clearParticipatingRegions();

                             if (LOG.isTraceEnabled()) LOG.trace("getTransactionStatesFromInterval: transaction: "
                                                 + transidToken + " stateString is: " + stateString);

                             if (stateString.equals(TransState.STATE_COMMITTED.toString())){
                                lvTxState = TransState.STATE_COMMITTED;
                             }
                             else if (stateString.equals(TransState.STATE_ABORTED.toString())){
                                lvTxState = TransState.STATE_ABORTED;
                             }
                             else if (stateString.equals(TransState.STATE_ACTIVE.toString())){
                                lvTxState = TransState.STATE_ACTIVE;
                             }
                             else if (stateString.equals(TransState.STATE_PREPARED.toString())){
                                lvTxState = TransState.STATE_PREPARED;
                             }
                             else if (stateString.equals(TransState.STATE_FORGOTTEN.toString())){
                                lvTxState = TransState.STATE_FORGOTTEN;
                             }
                             else if (stateString.equals(TransState.STATE_FORGOTTEN_COMMITTED.toString())){
                                lvTxState = TransState.STATE_FORGOTTEN_COMMITTED;
                             }
                             else if (stateString.equals(TransState.STATE_FORGOTTEN_ABORT.toString())){
                                lvTxState = TransState.STATE_FORGOTTEN_ABORT;
                             }
                             else if (stateString.equals(TransState.STATE_RECOVERY_COMMITTED.toString())){
                                lvTxState = TransState.STATE_RECOVERY_COMMITTED;
                             }
                             else if (stateString.equals(TransState.STATE_RECOVERY_ABORT.toString())){
                                lvTxState = TransState.STATE_RECOVERY_ABORT;
                             }
                             else {
                                lvTxState = TransState.STATE_BAD;
                             }

                             // get past the filler
                             st.nextElement();

                             // get past the place holder
                             st.nextElement();
                             String startIdToken = st.nextElement().toString();
                             ts.setStartId(Long.parseLong(startIdToken));
                             String commitIdToken = st.nextElement().toString();
                             ts.setCommitId(Long.parseLong(commitIdToken));

                             // Load the TransactionState object up with regions
                             while (st.hasMoreElements()) {
                                String tableNameToken = st.nextToken();
                                Table tmpTable = connection.getTable(TableName.valueOf(tableNameToken));
                                RegionLocator   rl = connection.getRegionLocator(table.getName());
                                List<HRegionLocation> regions = rl.getAllRegionLocations();
                                Iterator<HRegionLocation> it =  regions.iterator();
                                while(it.hasNext()) { // iterate entries.
                                   HRegionLocation loc = it.next();
                                   HRegionInfo regionKey = loc.getRegionInfo();
                                   if (LOG.isTraceEnabled()) LOG.trace("getTransactionStatesFromInterval: transaction: " + transidToken + " adding region: " + regionKey.getRegionNameAsString());
                                   ServerName serverValue = loc.getServerName();
                                   TransactionRegionLocation tloc = new TransactionRegionLocation(regionKey, serverValue);
                                   ts.addRegion(tloc);
                                }
                             }
                             ts.setStatus(lvTxState);

                             if (LOG.isTraceEnabled()) LOG.trace("getTransactionStatesFromInterval: adding transid: "
                                            + ts.getTransactionId() + " state: " + lvTxState + " to transList");
                             transList.add(ts);
                          } // if (! rowResult,isEmpty()))
                       } // for (int i = 0; i < count
                    } // TlogTransactionStatesFromIntervalResponse TSFI_response : result.values()
                 } // if(result.size() >= 1)
                 retry = false;
              } catch (Exception e) {
                 LOG.error("getTransactionStatesFromIntervalX retrying due to Exception: " + e);
                 refresh = true;
                 retry = true;
              }
              if (refresh) {

               RegionLocator   rl = connection.getRegionLocator(table.getName());
               rl.getAllRegionLocations();

               if (LOG.isTraceEnabled()) LOG.trace("getTransactionStatesFromIntervalX -- locations being refreshed : " + table + " for ASN: " + auditSeqNum);
               if(retryCount == TLOG_RETRY_ATTEMPTS) {
                  LOG.error("Exceeded retry attempts (" + retryCount + ") in getTransactionStatesFromIntervalX for ASN: " + auditSeqNum);
                     // We have received our reply in the form of an exception,
                     // so decrement outstanding count and wake up waiters to avoid
                     // getting hung forever
                  IOException ie = new IOException("Exceeded retry attempts (" + retryCount + ") in getTransactionStatesFromIntervalX for ASN: " + auditSeqNum);
                  throw ie;
               }

               if (LOG.isTraceEnabled()) LOG.trace("getTransactionStatesFromIntervalX -- setting retry, count: " + retryCount);
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

          if (LOG.isTraceEnabled()) LOG.trace("getTransactionStatesFromIntervalX -- EXIT ASN: " + auditSeqNum);
          return transList;
      } //getTransactionStatesFromIntervalX
   } // TlogCallable2  

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

   /**
   * Method  : getTransactionStatesFromInterval
   * Params  : ClusterId - Trafodion clusterId that was assigned to the beginner of the transaction.
   *                       Transactions that originate from other clsters will be filtered out from the response.
   *           nodeId    - Trafodion nodeId of the Tlog set that is to be read.  Typically this
   *                       id is mapped to the Tlog set as follows Tlog<nodeId>
   *           pvASN     - ASN after which all audit records will be returned
   * Return  : ArrayList<TransactionState> 
   * Purpose : Retrieve list of transactions from an interval
   */
   public ArrayList<TransactionState>  getTransactionStatesFromInterval(final long pv_clusterId, final long pv_nodeId, final long pv_ASN) throws IOException {

     long threadId = Thread.currentThread().getId();
     CompletionService<ArrayList<TransactionState>> compPool = new ExecutorCompletionService<ArrayList<TransactionState>>(tlogThreadPool);

     try {
        if (LOG.isTraceEnabled()) LOG.trace("getTransactionStatesFromInterval node: " + pv_nodeId
                      + ", asn: " + pv_ASN + " in thread " + threadId);

        Table targetTable;

        String lv_tLogName = new String("TRAFODION._DTM_.TLOG" + String.valueOf(pv_nodeId));
        targetTable = connection.getTable(TableName.valueOf(lv_tLogName));
           
        compPool.submit(new TlogCallable2(table, connection) {
           public ArrayList<TransactionState> call() throws IOException {
              if (LOG.isTraceEnabled()) LOG.trace("before getTransactionStatesFromIntervalX() ASN: "
                        + pv_ASN + ", clusterId: " + pv_clusterId + " and node: " + pv_nodeId);
              return getTransactionStatesFromIntervalX(pv_clusterId, pv_ASN);
           }
        });
     } catch (Exception e) {
        LOG.error("exception in getTransactionStatesFromInterval for interval ASN: " + pv_ASN
                    + ", node: " + pv_nodeId + " " + e);
        throw new IOException(e);
     }
     // all requests sent at this point, can record the count
     if (LOG.isTraceEnabled()) LOG.trace("getTransactionStatesFromInterval tlog callable requests sent in thread " + threadId);
     ArrayList<TransactionState> results = new ArrayList<TransactionState>();
     try {
        ArrayList<TransactionState> partialResult = compPool.take().get();
        for (TransactionState ts : partialResult) {
           results.add(ts);
        }
     }
     catch (Exception e2) {
       LOG.error("exception retrieving replies in getTransactionStatesFromInterval for interval ASN: " + pv_ASN
                   + ", node: " + pv_nodeId + " " + e2);
       throw new IOException(e2);
     }
     if (LOG.isTraceEnabled()) LOG.trace("getTransactionStatesFromInterval tlog callable requests completed in thread "
         + threadId + ".  " + results.size() + " results returned.");
     return results;
   }

   /**
   * Method  : doTlogWrite
   * Params  : regionName - name of Region
   *           transactionId - transaction identifier
   *           commitId - commitId for the transaction
   *           put - record representing the commit/abort record for the transaction
   * Return  : void
   * Purpose : write commit/abort for a given transaction
   */
   public void doTlogWrite(final TransactionState transactionState, final String lvTxState, final TrafodionLocationList regions, final boolean hasPeer, boolean forced, long recoveryASN) throws IOException {
     int loopCount = 0;
     long threadId = Thread.currentThread().getId();
     final long lvTransid = transactionState.getTransactionId();
     if (LOG.isTraceEnabled()) LOG.trace("doTlogWrite for " + transactionState.getTransactionId() + " in thread " + threadId);
     StringBuilder tableString = new StringBuilder();
     final long lvCommitId = transactionState.getCommitId();
     if (regions != null) {
        // Regions passed in indicate a state record where recovery might be needed following a crash.
        // To facilitate branch notification we translate the regions into table names that can then
        // be translated back into new region names following a restart.  THis allows us to ensure all
        // branches reply prior to cleanup
        boolean addComma = false;
        for (String tableName : regions.getList().keySet()) {
           if (addComma)
              tableString.append(",");
           else
              addComma = true;
           tableString.append(tableName); 
           
        }
/*
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
*/
        if (LOG.isTraceEnabled()) LOG.trace("table names: " + tableString.toString() + " in thread " + threadId);
     }
     long key = transactionState.getTransSeqNum();
     if (LOG.isTraceEnabled()) LOG.trace("key: " + key + ", hex: " + Long.toHexString(key) + ", transid: " +  lvTransid
   		  + " in thread " + threadId);
     if (LOG.isTraceEnabled()) LOG.trace("transid: " +  lvTransid + " in thread " + threadId);
     Put p = new Put(Bytes.toBytes(key));
     String hasPeerS;
     if (hasPeer) {
        hasPeerS = new String ("1");
     }
     else {
        hasPeerS = new String ("0");
     }
     long lvAsn;
     if (recoveryASN == -1){
        // This is a normal audit record so we manage the ASN
        lvAsn = asn.get();
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
                       + "," + hasPeerS
                       + "," + String.valueOf(lvCommitId)
                       + "," + tableString.toString()));

     try {
        if (LOG.isTraceEnabled()) LOG.trace("doTlogWrite [" + lvTransid + "] in thread " + threadId);

        Table table = connection.getTable(TableName.valueOf(getTlogTableNameBase()));
        RegionLocator locator = connection.getRegionLocator(table.getName());
        HRegionLocation location = locator.getRegionLocation(p.getRow());
        CompletionService<Integer> compPool = new ExecutorCompletionService<Integer>(tlogThreadPool);

        if (LOG.isTraceEnabled()) LOG.trace("doTlogWrite submitting tlog callable in thread " + threadId);
        final Put p2 = new Put(p);

        compPool.submit(new TlogCallable1(transactionState, location, connection) {
           public Integer call() throws IOException {
              if (LOG.isTraceEnabled()) LOG.trace("before doTlogWriteX() [" + transactionState.getTransactionId() + "]" );
              return doTlogWriteX(location.getRegionInfo().getRegionName(), lvTransid,
//                         transactionState.getCommitId(), p2, index);
                         transactionState.getCommitId(), p2);
           }
        });
     } catch (IOException e) {
        LOG.error("exception in doTlogWrite for transaction: " + lvTransid + " "  + e);
        //throw new CommitUnsuccessfulException(e);
        throw e;
     }
     // all requests sent at this point, can record the count
     if (LOG.isTraceEnabled()) LOG.trace("doTlogWrite tlog callable setting requests sent to 1 in thread " + threadId);
     transactionState.completeSendInvoke(1);

   }

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

      // Get the asn from the last control point.  This ignores
      // any asn increments between the last control point
      // write and a system crash and could result in asn numbers
      // being reused.  However this would just mean that some old
      // records are held onto a bit longer before cleanup and is safe.
      lvAsn = tLogControlPoint.getStartingAuditSeqNum(myClusterId); 
      asn.set(lvAsn);

      LOG.info("Starting Audit Sequence Number is " + lvAsn);

      Admin admin  = connection.getAdmin();
      boolean lvTlogExists = admin.tableExists(TableName.valueOf(getTlogTableNameBase()));
      if (LOG.isTraceEnabled()) LOG.trace("Tlog table " + getTlogTableNameBase() + (lvTlogExists? " exists" : " does not exist" ));
      HTableDescriptor desc = new HTableDescriptor(TableName.valueOf(getTlogTableNameBase()));
      desc.setRegionSplitPolicyClassName("org.apache.hadoop.hbase.regionserver.DisabledRegionSplitPolicy");
      desc.addCoprocessor("org.apache.hadoop.hbase.coprocessor.transactional.TrxRegionEndpoint");
      desc.addFamily(hcol);

      if (lvTlogExists == false) {
         // Need to prime the asn for future writes
         try {
            if (LOG.isTraceEnabled()) LOG.trace("Creating the table " + getTlogTableNameBase());
            admin.createTable(desc);
            asn.set(1L);  // TLOG didn't exist previously, so start asn at 1
         }
         catch (TableExistsException e) {
            LOG.error("Table " + getTlogTableNameBase() + " already exists");
         }
      }
      try {
         if (LOG.isTraceEnabled()) LOG.trace("try new Table ");
         table = connection.getTable(TableName.valueOf(getTlogTableNameBase()));
      }
      catch(Exception e){
         LOG.error("TmAuditTlog Exception; ", e);
         throw new RuntimeException(e);
      }
      admin.close();

      lvAsn = asn.get();
      // This control point write needs to be delayed until after recovery completes, 
      // but is here as a placeholder
      if (LOG.isTraceEnabled()) LOG.trace("Starting a control point with asn value " + lvAsn);
      tLogControlPointNum = tLogControlPoint.doControlPoint(myClusterId, lvAsn, true);

      if (LOG.isTraceEnabled()) LOG.trace("Exit constructor()");
      return;
   }

   public long bumpControlPoint(final int clusterId, final int count) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("bumpControlPoint clusterId: " + clusterId + " count: " + count);
      // Bump the bump the control point as requested, but make sure our asn is still set properly 
      // reflecting what is stored in the table.  This ignores 
      // any asn increments between the last control point
      // write and a system crash and could result in asn numbers
      // being reused.  However this would just mean that some old 
      // records are held onto a bit longer before cleanup and is safe.
      long lvReturn = tLogControlPoint.bumpControlPoint(clusterId, count);
      asn.set(lvReturn);
      if (LOG.isTraceEnabled()) LOG.trace("bumpControlPoint resetting asn to: " + lvReturn);
      return lvReturn;
   }

   public long getNextAuditSeqNum(int clusterId, int nid) throws IOException{
      if (LOG.isTraceEnabled()) LOG.trace("getNextAuditSeqNum clusterId: " + clusterId + " node: " + nid);
      return tLogControlPoint.getNextAuditSeqNum(clusterId, nid);
   }

   public static long asnGetAndIncrement () {
      if (LOG.isTraceEnabled()) LOG.trace("asnGetAndIncrement");
      return asn.getAndIncrement();
   }

   public void putSingleRecord(final long lvTransid, final long lvStartId, final long lvCommitId, final String lvTxState, 
         final TrafodionLocationList regions, final boolean hasPlaceHolder, boolean forced) throws IOException {
      putSingleRecord(lvTransid, lvStartId, lvCommitId, lvTxState, regions, hasPlaceHolder, forced, -1);
   }

   public void putSingleRecord(final long lvTransid, final long lvStartId, final long lvCommitId, final String lvTxState, 
         final TrafodionLocationList regions, final boolean hasPlaceHolder, boolean forced, long recoveryASN) throws IOException {

      long threadId = Thread.currentThread().getId();
      if (LOG.isTraceEnabled()) LOG.trace("putSingleRecord start in thread " + threadId);
      StringBuilder tableString = new StringBuilder();
      String transidString = new String(String.valueOf(lvTransid));
      String commitIdString = new String(String.valueOf(lvCommitId));
      boolean lvResult = true;
      long lvAsn;
      long startSynch = 0;
      long endSynch = 0;
      int lv_TimeIndex = (timeIndex.getAndIncrement() % 50 );
      long lv_TotalWrites = totalWrites.incrementAndGet();
      long lv_TotalRecords = totalRecords.incrementAndGet();
      Table putTable;
      putTable = connection.getTable(TableName.valueOf(getTlogTableNameBase()));

      if (regions != null) {
         // Regions passed in indicate a state record where recovery might be needed following a crash.
         // To facilitate branch notification we translate the regions into table names that can then
         // be translated back into new region names following a restart.  THis allows us to ensure all
         // branches reply prior to cleanup
        boolean addComma = false;
        for ( String tableName : regions.getList().keySet()) {
           if (addComma)
              tableString.append(",");
           else
              addComma = true;
           tableString.append(tableName); 
        }
/*   
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
*/
         if (LOG.isTraceEnabled()) LOG.trace("table names: " + tableString.toString() + " in thread " + threadId);
      }
      long key = lvTransid;
      if (LOG.isTraceEnabled()) LOG.trace("key: " + key + ", hex: " + Long.toHexString(key) + ", transid: " +  lvTransid
    		  + " in thread " + threadId);
      Put p = new Put(Bytes.toBytes(key));
      String hasPlaceHolderS;
      if (hasPlaceHolder) {
         hasPlaceHolderS = new String ("1");
      }
      else {
         hasPlaceHolderS = new String ("0");
      }
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
                       + "," + hasPlaceHolderS
                       + "," + String.valueOf(lvStartId)
                       + "," + String.valueOf(lvCommitId)
                       + "," + tableString.toString()));
      if (! forced){
         p.setDurability(Durability.ASYNC_WAL);
      }

      if (recoveryASN != -1){
         // We need to send this to a remote Tlog, not our local one, so open the appropriate table
         if (LOG.isTraceEnabled()) LOG.trace("putSingleRecord writing to remote Tlog for transid: " + lvTransid + " state: " + lvTxState + " ASN: " + lvAsn
                  + " in thread " + threadId);
         Table recoveryTable;
         int lv_ownerNid = (int)TransactionState.getNodeId(lvTransid);
         recoveryTable = connection.getTable(TableName.valueOf(getTlogTableNameBase()));
         RegionLocator locator = connection.getRegionLocator(recoveryTable.getName());

         try {
            boolean complete = false;
            int retries = 0;
            do {
               try {
                  retries++;
                  if (LOG.isTraceEnabled()) LOG.trace("try recovery table.put in thread " + threadId + ", " + p );
                  recoveryTable.put(p);
                  complete = true;
                  if (retries > 1){
                      if (LOG.isTraceEnabled()) LOG.trace("Retry successful in putSingleRecord for transaction: "
                             + lvTransid + " on recovery table " + getTlogTableNameBase());
                   }
               }
               catch (RetriesExhaustedWithDetailsException rewde){
                   LOG.error("Retrying putSingleRecord on recoveryTable for transaction: " + lvTransid + " on table "
                           + getTlogTableNameBase() + " due to RetriesExhaustedWithDetailsException ", rewde);
                   locator.getRegionLocation(p.getRow(), true);
                   try {
                      Thread.sleep(TlogRetryDelay); // 3 second default
                   } catch (InterruptedException ie) {
                   }
                   if (retries == TlogRetryCount){
                      LOG.error("putSingleRecord aborting due to excessive retries on recoveryTable for transaction: " + lvTransid + " on table "
                               + getTlogTableNameBase() + " due to RetriesExhaustedWithDetailsException; aborting ");
                      System.exit(1);
                   }
               }
               catch (IOException e2){
                   LOG.error("Retrying putSingleRecord on recoveryTable for transaction: " + lvTransid + " on table "
                           + getTlogTableNameBase() + " due to Exception ", e2);
                   locator.getRegionLocation(p.getRow(), true);
                   try {
                      Thread.sleep(TlogRetryDelay); // 3 second default
                   } catch (InterruptedException ie) {
                   }
                   if (retries == TlogRetryCount){
                      LOG.error("putSingleRecord aborting due to excessive retries on recoveryTable for transaction: " + lvTransid + " on table "
                               + getTlogTableNameBase() + " due t Exception; aborting ");
                      System.exit(1);
                   }
               }
            } while (! complete && retries < TlogRetryCount);  // default give up after 5 minutes
         }
         catch (IOException e2){
            // create record of the exception
            LOG.error("putSingleRecord Exception in recoveryTable", e2);
            throw e2;
         }
         finally {
               locator.close();
               recoveryTable.close();
         }
      }
      else {
         // This goes to our local TLOG
         startSynch = System.nanoTime();
         boolean complete = false;
         int retries = 0;
         do {
            retries++;
            try {
               endSynch = System.nanoTime();
               startTimes[lv_TimeIndex] = System.nanoTime();
                  try {
                     if (LOG.isTraceEnabled()) LOG.trace("try table.put in thread " + threadId + ", " + p );
                     putTable.put(p);
                     if (forced){
                       if (LOG.isTraceEnabled()) LOG.trace("flushing commits in thread " + threadId);
                     }
                     endTimes[lv_TimeIndex] = System.nanoTime();
                     complete = true;
                     if (retries > 1){
                        if (LOG.isTraceEnabled()) LOG.trace("Retry successful in putSingleRecord for transaction: " + lvTransid + " on table "
                              + putTable.getName().getNameAsString());
                     }
                  }
                  catch (RetriesExhaustedWithDetailsException rewde){
                     LOG.error("Retry " + retries + " putSingleRecord for transaction: " + lvTransid + " on table "
                           + putTable.getName().getNameAsString() + " due to RetriesExhaustedWithDetailsException ", rewde);

                     RegionLocator locator = connection.getRegionLocator(TableName.valueOf(getTlogTableNameBase()));
                     locator.getRegionLocation(p.getRow(), true);
                     try {
                        Thread.sleep(TlogRetryDelay); // 3 second default
                     } catch (InterruptedException ie) {
                     }
                     if (retries == TlogRetryCount){
                        LOG.error("putSingleRecord aborting due to excessive retries for transaction: " + lvTransid + " on table "
                                 + putTable.getName().getNameAsString() + " due to RetriesExhaustedWithDetailsException; aborting ");
                        System.exit(1);
                     }
                 }
                 catch (Exception e2){
                    LOG.error("Retry " + retries + " putSingleRecord for transaction: " + lvTransid + " on table "
                              + putTable.getName().getNameAsString() + " due to Exception ", e2);
                    RegionLocator locator = connection.getRegionLocator(TableName.valueOf(getTlogTableNameBase()));
                    locator.getRegionLocation(p.getRow(), true);
                    try {
                       Thread.sleep(TlogRetryDelay); // 3 second default
                    } catch (InterruptedException ie) {
                    }
                    if (retries == TlogRetryCount){
                       LOG.error("putSingleRecord aborting due to excessive retries for transaction: " + lvTransid + " on table "
                                  + putTable.getName().getNameAsString() + " due to Exception; aborting ");
                       System.exit(1);
                    }
                 }
              }
              catch (IOException e) {
                 // create record of the exception
                 LOG.error("PutSingleRecord for transaction:" + lvTransid + " Exception ", e);
                 try {
                    Thread.sleep(TlogRetryDelay); // 3 second default
                 } catch (InterruptedException ie) {
                 }
                 RegionLocator locator = connection.getRegionLocator(TableName.valueOf(getTlogTableNameBase()));
                 locator.getRegionLocation(p.getRow(), true);
                 if (retries == TlogRetryCount){
                    LOG.error("putSingleRecord retries exceeded for transaction: " + lvTransid + " on table "
                           + putTable.getName().getNameAsString() + " due to Exception; Throwing exception");
                    throw e;
                 }
              }
           } while (! complete && retries < TlogRetryCount);  // default give up after 5 minutes

           if ( retries > 1){
              LOG.info("putSingleRecord for transaction: " + lvTransid + " on table "
                    + putTable.getName().getNameAsString() + " successful after " + retries + " retries");
           }

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
      Table getTable;
      getTable = connection.getTable(TableName.valueOf(getTlogTableNameBase()));
      try {
         String transidString = new String(String.valueOf(lvTransid));
         Get g;
         long key = lvTransid;
         if (LOG.isTraceEnabled()) LOG.trace("key: " + key + " hex: " + Long.toHexString(key));
         g = new Get(Bytes.toBytes(key));
         try {
            Result r = getTable.get(g);
            byte [] value = r.getValue(TLOG_FAMILY, ASN_STATE);
            stateString =  new String (Bytes.toString(value));
            if (LOG.isTraceEnabled()) LOG.trace("stateString is " + stateString);
            if (stateString.contains("COMMITTED")){
                lvTxState = TransState.STATE_COMMITTED;
            }
            else if (stateString.contains("ABORTED")){
               lvTxState = TransState.STATE_ABORTED;
            }
            else if (stateString.equals(TransState.STATE_ACTIVE.toString())){
               lvTxState = TransState.STATE_ACTIVE;
            }
            else if (stateString.equals(TransState.STATE_PREPARED.toString())){
               lvTxState = TransState.STATE_PREPARED;
            }
            else if (stateString.equals(TransState.STATE_NOTX.toString())){
               lvTxState = TransState.STATE_NOTX;
            }
            else if (stateString.equals(TransState.STATE_FORGOTTEN.toString())){
               lvTxState = TransState.STATE_FORGOTTEN;
            }
            else if (stateString.equals(TransState.STATE_ABORTING.toString())){
               lvTxState = TransState.STATE_ABORTING;
            }
            else if (stateString.equals(TransState.STATE_COMMITTING.toString())){
               lvTxState = TransState.STATE_COMMITTING;
            }
            else if (stateString.equals(TransState.STATE_PREPARING.toString())){
               lvTxState = TransState.STATE_PREPARING;
            }
            else if (stateString.equals(TransState.STATE_FORGETTING.toString())){
               lvTxState = TransState.STATE_FORGETTING;
            }
            else if (stateString.equals(TransState.STATE_FORGETTING_HEUR.toString())){
               lvTxState = TransState.STATE_FORGETTING_HEUR;
            }
            else if (stateString.equals(TransState.STATE_BEGINNING.toString())){
               lvTxState = TransState.STATE_BEGINNING;
            }
            else if (stateString.equals(TransState.STATE_HUNGCOMMITTED.toString())){
              lvTxState = TransState.STATE_HUNGCOMMITTED;
            }
            else if (stateString.equals(TransState.STATE_HUNGABORTED.toString())){
               lvTxState = TransState.STATE_HUNGABORTED;
            }
            else if (stateString.equals(TransState.STATE_IDLE.toString())){
               lvTxState = TransState.STATE_IDLE;
            }
            else if (stateString.equals(TransState.STATE_FORGOTTEN_HEUR.toString())){
               lvTxState = TransState.STATE_FORGOTTEN_HEUR;
            }
            else if (stateString.equals(TransState.STATE_ABORTING_PART2.toString())){
               lvTxState = TransState.STATE_ABORTING_PART2;
            }
            else if (stateString.equals(TransState.STATE_TERMINATING.toString())){
                lvTxState = TransState.STATE_TERMINATING;
            }
            else {
               lvTxState = TransState.STATE_BAD;
            }

            if (LOG.isTraceEnabled()) LOG.trace("transid: " + lvTransid + " state: " + lvTxState);
         }
         catch (IOException e){
             LOG.error("getRecord IOException ", e);
             throw e;
         }
         catch (Exception e){
             LOG.error("getRecord Exception ", e);
             throw e;
         }
      }
      catch (Exception e2) {
            LOG.error("getRecord Exception2 ", e2);
            e2.printStackTrace();
      }

      if (LOG.isTraceEnabled()) LOG.trace("getRecord end; returning " + lvTxState);
      return lvTxState.getValue();
   }

    public static String getRecord(final String transidString) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("getRecord start");
      long lvTransid = Long.parseLong(transidString, 10);
      String lvTxState = new String("NO RECORD");
      Table getTable;
      getTable = connection.getTable(TableName.valueOf(getTlogTableNameBase()));
      try {
         Get g;
         long key = lvTransid;
         if (LOG.isTraceEnabled()) LOG.trace("key: " + key + " hex: " + Long.toHexString(key));
         g = new Get(Bytes.toBytes(key));
         try {
            Result r = getTable.get(g);
            StringTokenizer st = 
                 new StringTokenizer(Bytes.toString(r.getValue(TLOG_FAMILY, ASN_STATE)), ",");
            String asnToken = st.nextElement().toString();
            String transidToken = st.nextElement().toString();
            lvTxState = st.nextElement().toString();
            if (LOG.isTraceEnabled()) LOG.trace("transid: " + transidToken + " state: " + lvTxState);
         } catch (IOException e){
             LOG.error("getRecord IOException: ", e);
             throw e;
         }
      } catch (IOException e){
             LOG.error("getRecord Exception: ", e);
             throw e;
      }
      if (LOG.isTraceEnabled()) LOG.trace("getRecord end; returning String:" + lvTxState);
      return lvTxState;
   }
      

   public static boolean deleteRecord(final long lvTransid) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("deleteRecord start " + lvTransid);
      Table deleteTable;
      deleteTable = connection.getTable(TableName.valueOf(getTlogTableNameBase()));
      try {
         Delete d;
         long key = lvTransid;
         if (LOG.isTraceEnabled()) LOG.trace("key: " + key + " hex: " + Long.toHexString(key));
         d = new Delete(Bytes.toBytes(key));
         d.setDurability(Durability.SKIP_WAL);
         if (LOG.isTraceEnabled()) LOG.trace("deleteRecord  (" + lvTransid + ") ");
         deleteTable.delete(d);
      }
      catch (Exception e) {
         LOG.error("deleteRecord Exception: ", e );
      }
      if (LOG.isTraceEnabled()) LOG.trace("deleteRecord - exit");
      return true;
   }

   public boolean deleteAgedEntries(final long lvAsn) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("deleteAgedEntries start:  Entries older than " + lvAsn + " will be removed");
      String lv_tLogName = new String(getTlogTableNameBase());
      long deleteCount = 0;

      if (LOG.isTraceEnabled()) LOG.trace("delete table is: " + lv_tLogName);

      // Use a BufferedMutator for client side buffered operations in the same was as autoFlush was used on HTables
      BufferedMutator deleteMutator = connection.getBufferedMutator(TableName.valueOf(lv_tLogName));
      Table deleteTable = connection.getTable(TableName.valueOf(lv_tLogName));
      try {
         boolean scanComplete = false;
         Scan s = new Scan();
         s.setCaching(100);
         s.setCacheBlocks(false);
         ArrayList<Delete> deleteList = new ArrayList<Delete>();
         ResultScanner ss = deleteTable.getScanner(s);

         try {
            for (Result r : ss) {
               if (scanComplete){
                  if (LOG.isTraceEnabled()) LOG.trace("scanComplete");
                  break;
               }
               for (Cell cell : r.rawCells()) {
                  StringTokenizer st =
                        new StringTokenizer(Bytes.toString(CellUtil.cloneValue(cell)), ",");
                  if (LOG.isTraceEnabled()) LOG.trace("string tokenizer success ");
                  if (st.hasMoreElements()) {
                     String asnToken = st.nextElement().toString();
                     if (LOG.isTraceEnabled()) LOG.trace("asnToken: " + asnToken);
                     if (Long.parseLong(asnToken) > lvAsn){
                        if (LOG.isTraceEnabled()) LOG.trace("RawCells asnToken: " + asnToken
                                      + " is greater than: " + lvAsn + ".  Scan complete");
                        scanComplete = true;
                        break;
                     }
                     String transidToken = st.nextElement().toString();
                     String stateToken = st.nextElement().toString();
                     if (LOG.isTraceEnabled()) LOG.trace("Transid: " + transidToken + " has state: " + stateToken);
                     if (LOG.isTraceEnabled()){
                        long tmp_trans = Long.parseLong(transidToken);
                        LOG.trace("Transid: " + transidToken + " has sequence: "
                                  + TransactionState.getTransSeqNum(tmp_trans)
                                  + ", node: " + TransactionState.getNodeId(tmp_trans)
                                  + ", clusterId: " + TransactionState.getClusterId(tmp_trans));
                     }
                     if ((Long.parseLong(asnToken) < lvAsn) && (stateToken.contains(TransState.STATE_FORGOTTEN.toString()))) {
                        Delete del = new Delete(r.getRow());
                        del.setDurability(Durability.SKIP_WAL);
                        if (LOG.isTraceEnabled()) LOG.trace("adding transid: " + transidToken + " to delete list");
//                        deleteList.add(del);
//                        deleteTable.delete(del);
                          deleteCount++;
                          deleteMutator.mutate(del);
                     }
                     else if ((Long.parseLong(asnToken) < lvAsn) &&
                             (stateToken.equals(TransState.STATE_COMMITTED.toString()) || stateToken.equals(TransState.STATE_ABORTED.toString()))) {
                        if (ageCommitted) {
                           Delete del = new Delete(r.getRow());
                           del.setDurability(Durability.SKIP_WAL);
                           if (LOG.isTraceEnabled()) LOG.trace("adding transid: " + transidToken + " to delete list");
//                           deleteList.add(del);
//                           deleteTable.delete(del);
                           deleteCount++;
                           deleteMutator.mutate(del);
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
                                 if ((Long.parseLong(asnToken) < lvAsn) && (stateToken.contains(TransState.STATE_FORGOTTEN.toString()))) {
                                    Delete del = new Delete(r.getRow());
                                    del.setDurability(Durability.SKIP_WAL);
                                    if (LOG.isTraceEnabled()) LOG.trace("Secondary search found new delete - adding (" + transidToken + ") with asn: " + asnToken + " to delete list");
//                                    deleteList.add(del);
//                                    deleteTable.delete(del);
                                    deleteCount++;
                                    deleteMutator.mutate(del);
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
         catch(Exception e){
            LOG.error("deleteAgedEntries Exception getting results for table " + lv_tLogName + "; ", e);
            throw new RuntimeException(e);
         }
         finally {
            if (LOG.isTraceEnabled()) LOG.trace("deleteAgedEntries closing ResultScanner");
            ss.close();
         }
      }
      catch (IOException e) {
         LOG.error("deleteAgedEntries IOException setting up scan on table "
                   + lv_tLogName + ", Exception: ", e);
         e.printStackTrace();
      }
      finally {
         try {
            if (LOG.isTraceEnabled()) LOG.trace("deleteAgedEntries closing table for "
                 + lv_tLogName +"; " + deleteCount + " entries deleted");
            deleteMutator.close();
            deleteTable.close();
         }
         catch (IOException e) {
            LOG.error("deleteAgedEntries IOException closing table " + lv_tLogName + " Exception: " + e);
         }
     }
     if (LOG.isTraceEnabled()) LOG.trace("deleteAgedEntries - exit");
     return true;
   }

   public long writeControlPointRecords (final int clusterId, final Map<Long, TransactionState> map) throws IOException {
      int cpWrites = 0;
      long startTime = System.nanoTime();
      long endTime;

      if (LOG.isTraceEnabled()) LOG.trace("Tlog " + getTlogTableNameBase()
           + " writeControlPointRecords for clusterId " + clusterId + " start with map size " + map.size());

      try {
        for (Map.Entry<Long, TransactionState> e : map.entrySet()) {
         try {
            Long transid = e.getKey();
            TransactionState value = e.getValue();
            if (value.getStatus().equals(TransState.STATE_COMMITTED.toString())){
               if (LOG.isTraceEnabled()) LOG.trace("writeControlPointRecords adding record for trans (" + transid + ") : state is " + value.getStatus());
               cpWrites++;
               putSingleRecord(transid, value.getStartId(), value.getCommitId(), value.getStatus(), value.getParticipatingRegions(), value.hasPlaceHolder(), forceControlPoint);
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
          return tLogControlPoint.getCurrControlPt(clusterId) - 1;
      } 

      endTime = System.nanoTime();
      if (LOG.isDebugEnabled()) LOG.debug("TLog Control Point Write Report\n" + 
                   "                        Total records: " 
                       +  map.size() + " in " + cpWrites + " write operations\n" +
                   "                        Write time: " + (endTime - startTime) / 1000 + " microseconds\n" );
  
      if (LOG.isTraceEnabled()) LOG.trace("writeControlPointRecords exit ");
      return -1L;

   }

   public long addControlPoint (final int clusterId, final Map<Long, TransactionState> map, final boolean incrementCP) throws IOException {
      if (LOG.isDebugEnabled()) LOG.debug("addControlPoint start with map size " + map.size());
      long lvCtrlPt = 0L;
      long agedAsn;  // Writes older than this audit seq num will be deleted
      long lvAsn;    // local copy of the asn
      long key;
      boolean success = false;

      if (controlPointDeferred) {
         // We deferred the control point once already due to concurrency.  We'll synchronize this timeIndex
         synchronized (map) {
            if (LOG.isTraceEnabled()) LOG.trace("Control point was deferred.  Writing synchronized control point records");
            lvCtrlPt = writeControlPointRecords(clusterId, map);
         }

         controlPointDeferred = false;
      }
      else {
         if (LOG.isTraceEnabled()) LOG.trace("Writing asynch control point records");
         lvCtrlPt = writeControlPointRecords(clusterId, map);
         if (controlPointDeferred){
            if (LOG.isTraceEnabled()) LOG.trace("Write asynch control point records did not complete successfully; control point deferred");
            return lvCtrlPt;  // should return -1 indicating the control point didn't complete successfully
         }
      }

      try {
         lvAsn = asn.getAndIncrement();
         if (LOG.isTraceEnabled()) LOG.trace("lvAsn reset to: " + lvAsn);

         // Write the control point interval and the ASN to the control point table
         lvCtrlPt = tLogControlPoint.doControlPoint(clusterId, lvAsn, incrementCP);
         if (LOG.isTraceEnabled()) LOG.trace("Control point record " + lvCtrlPt +
        		 " returned for table " + tLogControlPoint.getTableName());

         long deleteCP = tLogControlPoint.getNthRecord(clusterId, versions - 2);
         if ((deleteCP) > 0){  // We'll keep 5 control points of audit
            try {
               if (LOG.isTraceEnabled()) LOG.trace("Attempting to get control point record from " + 
                         tLogControlPoint.getTableName() + " for control point " + (deleteCP));
               agedAsn = tLogControlPoint.getRecord(clusterId, String.valueOf(deleteCP));
               if (LOG.isTraceEnabled()) LOG.trace("AgedASN from " + 
                       tLogControlPoint.getTableName() + " is " + agedAsn);
               if (agedAsn > 0){
                  try {
                     if (LOG.isTraceEnabled()) LOG.trace("Attempting to remove TLOG writes older than asn " + agedAsn);
                     deleteAgedEntries(agedAsn);
//                     deleteEntriesOlderThanASN(agedAsn, ageCommitted);
                  }
                  catch (IOException e){
                     LOG.error("deleteAgedEntries Exception ", e);
                     throw e;
                  }
               }
               try {
                  tLogControlPoint.deleteAgedRecords(clusterId, lvCtrlPt - versions);
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
      } catch (IOException e){
          LOG.error("addControlPoint Exception ", e);
          throw e;
      }
      if (LOG.isDebugEnabled()) LOG.debug("addControlPoint returning " + lvCtrlPt);
      return lvCtrlPt;
   } 

   public long getStartingAuditSeqNum(final int clusterId) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("getStartingAuditSeqNum for clusterId: " + clusterId);
      long lvAsn = tLogControlPoint.getStartingAuditSeqNum(clusterId);
      if (LOG.isTraceEnabled()) LOG.trace("getStartingAuditSeqNum returning: " + lvAsn);
      return lvAsn;
   }

   public void getTransactionState (TransactionState ts) throws IOException {
      getTransactionState (ts, true);
   }

   public void getTransactionState (TransactionState ts, boolean postAllRegions) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("getTransactionState start; transid: " + ts.getTransactionId());

      // This request might be for a transaction not originating on this node, so we need to open
      // the appropriate Tlog
      Table unknownTransactionTable;
      long lvTransid = ts.getTransactionId();
      int lv_ownerNid = (int)TransactionState.getNodeId(lvTransid);
      String lv_tLogName = new String("TRAFODION._DTM_.TLOG" + String.valueOf(lv_ownerNid));
      if (LOG.isTraceEnabled()) LOG.trace("getTransactionState reading from: " + lv_tLogName);
      unknownTransactionTable = connection.getTable(TableName.valueOf(lv_tLogName));
      RegionLocator rl = connection.getRegionLocator(TableName.valueOf(lv_tLogName));
      rl.getAllRegionLocations();

      boolean complete = false;
      int retries = 0;
      Get g;
      byte [] value;
      String stateString = "";
      String transidToken = "";
      String startIdToken = "";
      String commitIdToken = "";
      TransState lvTxState = TransState.STATE_NOTX;
      Result r;
      StringTokenizer st;
      long key = lvTransid;

      do {
         try {
       	    retries++;
            String transidString = new String(String.valueOf(lvTransid));
            if (LOG.isTraceEnabled()) LOG.trace("key: " + key + ", hexkey: " + Long.toHexString(key) + ", transid: " +  lvTransid);
            g = new Get(Bytes.toBytes(key));
            lvTxState = TransState.STATE_NOTX;
            r = unknownTransactionTable.get(g);
            if (r == null) {
               ts.setStatus(TransState.STATE_NOTX);
               if (LOG.isTraceEnabled()) LOG.trace("getTransactionState: tLog result is null: " + transidString);
            }
            if (r.isEmpty()) {
               ts.setStatus(TransState.STATE_NOTX);
               if (LOG.isTraceEnabled()) LOG.trace("getTransactionState: tLog empty result: " + transidString);
            }
            value = r.getValue(TLOG_FAMILY, ASN_STATE);
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
            try {
               st = new StringTokenizer(Bytes.toString(value), ",");
               if (st.hasMoreElements()) {
                   String asnToken = st.nextElement().toString();
                   transidToken = st.nextElement().toString();
                   stateString = st.nextElement().toString();
                   if (LOG.isTraceEnabled()) LOG.trace("getTransactionState: transaction: " + transidToken + " stateString is: " + stateString);
                }
                if (stateString.contains("COMMITTED")){
                   lvTxState = TransState.STATE_COMMITTED;
                }
                else if (stateString.contains("ABORT")){
                   lvTxState = TransState.STATE_ABORTED;
                }
                else if (stateString.contains("FORGOT")){
                   // Need to get the previous state record so we know how to drive the regions
                   String keyS = new String(r.getRow());
                   Get get = new Get(r.getRow());
                   get.setMaxVersions(versions);  // will return last n versions of row
                   Result lvResult = unknownTransactionTable.get(get);
                   // byte[] b = lvResult.getValue(TLOG_FAMILY, ASN_STATE);  // returns current version of value
                   List<Cell> list = lvResult.getColumnCells(TLOG_FAMILY, ASN_STATE);  // returns all versions of this column
                   for (Cell element : list) {
                      st = new StringTokenizer(Bytes.toString(CellUtil.cloneValue(element)), ",");
                      if (st.hasMoreElements()) {
                         if (LOG.isTraceEnabled()) LOG.trace("Performing secondary search on (" + transidToken + ")");
                         String asnToken = st.nextElement().toString() ;
                         transidToken = st.nextElement().toString() ;
                         String stateToken = st.nextElement().toString() ;
                         if (LOG.isTraceEnabled()) LOG.trace("Trans (" + transidToken + ") has stateToken: " + stateToken);
                         if ((stateToken.contains("COMMITTED")) || (stateToken.contains("ABORT"))) {
                            if (LOG.isTraceEnabled()) LOG.trace("Secondary search found record for (" + transidToken + ") with state: " + stateToken);
                            lvTxState = (stateToken.contains("COMMITTED")) ? TransState.STATE_COMMITTED : TransState.STATE_ABORTED;
                            break;
                         }
                         else {
                            if (LOG.isTraceEnabled()) LOG.trace("Secondary search skipping entry for (" + 
                                         transidToken + ") with state: " + stateToken );
                         }
                      }
                   }
                }
                else {
                   lvTxState = TransState.STATE_BAD;
                }

                // get past the filler
                st.nextElement();

                // get past the place holder
                st.nextElement();

                startIdToken = st.nextElement().toString();
                ts.setStartId(Long.parseLong(startIdToken));
                commitIdToken = st.nextElement().toString();
                ts.setCommitId(Long.parseLong(commitIdToken));

                if (postAllRegions){
                   ts.clearParticipatingRegions();

                   List<HRegionLocation> regionList;
                   // Load the TransactionState object up with regions
                   while (st.hasMoreElements()) {
                      String tableNameToken = st.nextToken();
                      regionList = connection.getRegionLocator(TableName.valueOf(lv_tLogName)).getAllRegionLocations();
                      Iterator<HRegionLocation> it =  regionList.iterator();
                      while(it.hasNext()) { // iterate entries.
                         HRegionLocation hloc = it.next();
                         if (LOG.isTraceEnabled()) LOG.trace("getTransactionState: transaction: " + transidToken +
                                   " adding region: " + hloc.getRegionInfo().getRegionNameAsString());
                         TransactionRegionLocation tloc = new TransactionRegionLocation(hloc.getRegionInfo(),hloc.getServerName());
                         if (postAllRegions) ts.addRegion(tloc); // TBD quick workaround, skip put if noPostAllRegions
                      }
                   }
                }
            }
            catch (Exception ste) {
               LOG.error("getTransactionState found a malformed record for transid: " + lvTransid
               		 + " record: " + Bytes.toString(value) + " on table: "
                         +lv_tLogName + " returning STATE_NOTX ");
               ts.setStatus(TransState.STATE_NOTX);
               return;
            }
            ts.setStatus(lvTxState);

            complete = true;
            if (retries > 1){
               if (LOG.isTraceEnabled()) LOG.trace("Retry successful in getTransactionState for transid: "
                            + lvTransid + " on table " + lv_tLogName);                    	 
            }
         }
         catch (Exception e){
            LOG.error("Retrying getTransactionState for transid: "
                   + lvTransid + " on table " + lv_tLogName + " due to Exception ", e);
            rl.getRegionLocation(Bytes.toBytes(key), true);
            try {
               Thread.sleep(TlogRetryDelay); // 3 second default
            } catch (InterruptedException ie) {
            }
            if (retries == TlogRetryCount){
               LOG.error("getTransactionState aborting due to excessive retries on on table : "
                         +lv_tLogName + " due to Exception; aborting ");
               System.exit(1);
            }
         }
      } while (! complete && retries < TlogRetryCount);  // default give up after 5 minutes
      if (LOG.isTraceEnabled()) LOG.trace("getTransactionState: returning transid: " + ts.getTransactionId() + " state: " + lvTxState);

      if (LOG.isTraceEnabled()) LOG.trace("getTransactionState end transid: " + ts.getTransactionId());
      return;
   }

   public long getAuditCP(int clustertoRetrieve) throws IOException {
      long cp = 0;
      try {
         cp = tLogControlPoint.getCurrControlPt(clustertoRetrieve);
      } catch (IOException e) {
          LOG.error("Get Control Point Exception " + Arrays.toString(e.getStackTrace()));
          throw e;
      }
      return cp;
   }
   
   public static String getTlogTableNameBase(){
      return TLOG_TABLE_NAME;
   }

   /**
   * Method  : deleteEntriesOlderThanASN
   * Params  : pv_ASN  - ASN before which all audit records will be deleted
   *         : pv_ageCommitted  - indicated whether committed transactions should be deleted
   * Return  : void
   * Purpose : Delete transaction records which are no longer needed
   */
   public void deleteEntriesOlderThanASN(final long pv_ASN, final boolean pv_ageCommitted) throws IOException {
      int loopIndex = 0;
      long threadId = Thread.currentThread().getId();
      CompletionService<Integer> compPool = new ExecutorCompletionService<Integer>(tlogThreadPool);

      try {
         if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASN: "
              + pv_ASN + ", in thread: " + threadId);
         List<HRegionLocation> regionList;

         String lv_tLogName = new String("TRAFODION._DTM_.TLOG" + String.valueOf(this.dtmid));
         regionList = connection.getRegionLocator(TableName.valueOf(lv_tLogName)).getAllRegionLocations();
         if (LOG.isTraceEnabled()) LOG.trace("regionList has " + regionList.size() + " elements");
         int regionIndex = 0;
         // For every region in this table
         for (HRegionLocation location : regionList) {
            regionIndex++;
            final byte[] regionName = location.getRegionInfo().getRegionName();
            compPool.submit(new TlogCallable(location, connection) {
               public Integer call() throws IOException {
                   if (LOG.isTraceEnabled()) LOG.trace("before deleteEntriesOlderThanASNX() ASN: " + pv_ASN);
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
      } catch (Exception e) {
         LOG.error("exception in deleteEntriesOlderThanASN for ASN: "
                 + pv_ASN + " ", e);
         throw new IOException(e);
      }

      if (LOG.isTraceEnabled()) LOG.trace("deleteEntriesOlderThanASN tlog callable requests completed in thread "
            + threadId);
      return;
  }
}
