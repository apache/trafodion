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

package org.apache.hadoop.hbase.client.transactional;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Collection;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Map;
import java.util.concurrent.Callable;
import java.util.concurrent.CompletionService;
import java.util.concurrent.ExecutorCompletionService;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ConcurrentHashMap;
import java.util.HashMap;

import org.apache.commons.codec.binary.Hex;

import org.apache.hadoop.hbase.ServerName;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.MasterNotRunningException;
import org.apache.hadoop.hbase.TableNotFoundException;
import org.apache.hadoop.hbase.TableNotEnabledException;
import org.apache.hadoop.hbase.TableNotDisabledException;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.ZooKeeperConnectionException;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.HConnection;
import org.apache.hadoop.hbase.client.HConnectionManager;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.coprocessor.Batch;
import org.apache.hadoop.hbase.client.Durability;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.AbortTransactionRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.AbortTransactionResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequestRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequestResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.RecoveryRequestRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.RecoveryRequestResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TrxRegionService;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitMultipleRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitMultipleResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.AbortTransactionMultipleRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.AbortTransactionMultipleResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequestMultipleRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequestMultipleResponse;

import org.apache.hadoop.hbase.exceptions.DeserializationException;
import org.apache.hadoop.hbase.io.compress.Compression.Algorithm;
import org.apache.hadoop.hbase.io.encoding.DataBlockEncoding;
import org.apache.hadoop.hbase.ipc.BlockingRpcCallback;
import org.apache.hadoop.hbase.ipc.ServerRpcController;
import org.apache.hadoop.hbase.ipc.CoprocessorRpcChannel;
import org.apache.hadoop.hbase.regionserver.BloomType;
import org.apache.hadoop.hbase.regionserver.KeyPrefixRegionSplitPolicy;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.Pair;
import org.apache.hadoop.hbase.util.EnvironmentEdgeManager;

import org.apache.hadoop.ipc.RemoteException;

import com.google.protobuf.ByteString;

import org.apache.hadoop.hbase.client.transactional.TmDDL;
import org.apache.hadoop.hbase.regionserver.transactional.IdTm;
import org.apache.hadoop.hbase.regionserver.transactional.IdTmException;
import org.apache.hadoop.hbase.regionserver.transactional.IdTmId;

// Sscc imports
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccRegionService;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccBeginTransactionRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccBeginTransactionResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccAbortTransactionRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccAbortTransactionResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitRequestRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitRequestResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitIfPossibleRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitIfPossibleResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccGetTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccGetTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPerformScanRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPerformScanResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccOpenScannerRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccOpenScannerResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalRequest;
import org.apache.hadoop.hbase.client.transactional.SsccUpdateConflictException;

/**
 * Transaction Manager. Responsible for committing transactions.
 */
public class TransactionManager {

  // Singleton TransactionManager class
  private static TransactionManager g_TransactionManager = null;
  static final Log LOG = LogFactory.getLog(TransactionManager.class);
  public AlgorithmType TRANSACTION_ALGORITHM;

  private boolean batchRegionServer = false;
  private int RETRY_ATTEMPTS;
  private final HConnection connection;
  private final TransactionLogger transactionLogger;
  private JtaXAResource xAResource;
  private HBaseAdmin hbadmin;
  private TmDDL tmDDL;
  private boolean batchRSMetricsFlag = false;
  Configuration     config;

  public static final int HBASE_NAME = 0;
  public static final int HBASE_MAX_VERSIONS = 1;
  public static final int HBASE_MIN_VERSIONS = 2;
  public static final int HBASE_TTL = 3;
  public static final int HBASE_BLOCKCACHE = 4;
  public static final int HBASE_IN_MEMORY = 5;
  public static final int HBASE_COMPRESSION = 6;
  public static final int HBASE_BLOOMFILTER = 7;
  public static final int HBASE_BLOCKSIZE = 8;
  public static final int HBASE_DATA_BLOCK_ENCODING = 9;
  public static final int HBASE_CACHE_BLOOMS_ON_WRITE = 10;
  public static final int HBASE_CACHE_DATA_ON_WRITE = 11;
  public static final int HBASE_CACHE_INDEXES_ON_WRITE = 12;
  public static final int HBASE_COMPACT_COMPRESSION = 13;
  public static final int HBASE_PREFIX_LENGTH_KEY = 14;
  public static final int HBASE_EVICT_BLOCKS_ON_CLOSE = 15;
  public static final int HBASE_KEEP_DELETED_CELLS = 16;
  public static final int HBASE_REPLICATION_SCOPE = 17;
  public static final int HBASE_MAX_FILESIZE = 18;
  public static final int HBASE_COMPACT = 19;
  public static final int HBASE_DURABILITY = 20;
  public static final int HBASE_MEMSTORE_FLUSH_SIZE = 21;
  public static final int HBASE_SPLIT_POLICY = 22;

  public static final int TM_COMMIT_FALSE = 0;
  public static final int TM_COMMIT_READ_ONLY = 1;
  public static final int TM_COMMIT_TRUE = 2;
  public static final int TM_COMMIT_FALSE_CONFLICT = 3;

  public static final int TM_SLEEP = 1000;      // One second
  public static final int TM_SLEEP_INCR = 5000; // Five seconds
  public static final int TM_RETRY_ATTEMPTS = 5;

  private IdTm idServer;
  private static final int ID_TM_SERVER_TIMEOUT = 1000;

  private Map<String,Long> batchRSMetrics = new ConcurrentHashMap<String, Long>();
  private long regions = 0;
  private long regionServers = 0;
  private int metricsCount = 0;

  static ExecutorService    cp_tpe;

  public enum AlgorithmType{
    MVCC, SSCC
  }

  // getInstance to return the singleton object for TransactionManager
  public synchronized static TransactionManager getInstance(final Configuration conf) throws ZooKeeperConnectionException, IOException {
    if (g_TransactionManager == null) {
      g_TransactionManager = new TransactionManager(conf);
    }
    return g_TransactionManager;
  }


  /* increment/deincrement for positive value */
  /* This method copied from o.a.h.h.utils.Bytes */
  public static byte [] binaryIncrementPos(byte [] value, long amount) {
    long amo = amount;
    int sign = 1;
    if (amount < 0) {
      amo = -amount;
      sign = -1;
    }
    for(int i=0;i<value.length;i++) {
      int cur = ((int)amo % 256) * sign;
      amo = (amo >> 8);
      int val = value[value.length-i-1] & 0x0ff;
      int total = val + cur;
      if(total > 255) {
        amo += sign;
        total %= 256;
      } else if (total < 0) {
        amo -= sign;
      }
      value[value.length-i-1] = (byte)total;
      if (amo == 0) return value;
    }
    return value;
  }

  public void init(final TmDDL tmddl) throws IOException {
    this.config = HBaseConfiguration.create();
    this.tmDDL = tmddl;
    try {
      hbadmin = new HBaseAdmin(config);
    }
    catch(Exception e) {
      System.out.println("ERROR: Unable to obtain HBase accessors, Exiting");
      e.printStackTrace();
      System.exit(1);
    }
  }

  /**
   * TransactionManagerCallable  :  inner class for creating asynchronous requests
   */
  private abstract class TransactionManagerCallable implements Callable<Integer>{
        TransactionState transactionState;
        TransactionRegionLocation  location;
        HTable table;
        byte[] startKey;
        byte[] endKey_orig;
        byte[] endKey;

        TransactionManagerCallable(TransactionState txState, TransactionRegionLocation location, HConnection connection) {
        transactionState = txState;
        this.location = location;
        try {
        table = new HTable(location.getRegionInfo().getTable(), connection, cp_tpe);
        } catch(IOException e) {
          e.printStackTrace();
          LOG.error("Error obtaining HTable instance");
          table = null;
        }
        startKey = location.getRegionInfo().getStartKey();
        endKey_orig = location.getRegionInfo().getEndKey();
        endKey = TransactionManager.binaryIncrementPos(endKey_orig, -1);
    }

    /**
     * Method  : doCommitX
     * Params  : regionName - name of Region
     *           transactionId - transaction identifier
     * Return  : Always 0, can ignore
     * Purpose : Call commit for a given regionserver
     */
  public Integer doCommitX(final byte[] regionName, final long transactionId, final long commitId, final boolean ignoreUnknownTransactionException) throws CommitUnsuccessfulException, IOException {
        boolean retry = false;
        boolean refresh = false;

        int retryCount = 0;
        int retrySleep = TM_SLEEP;

        if( TRANSACTION_ALGORITHM == AlgorithmType.MVCC){
        do {
          try {

            if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- ENTRY txid: " + transactionId +
                                                  " ignoreUnknownTransactionException: " + ignoreUnknownTransactionException);
            Batch.Call<TrxRegionService, CommitResponse> callable =
               new Batch.Call<TrxRegionService, CommitResponse>() {
                 ServerRpcController controller = new ServerRpcController();
                 BlockingRpcCallback<CommitResponse> rpcCallback =
                    new BlockingRpcCallback<CommitResponse>();

                    @Override
                    public CommitResponse call(TrxRegionService instance) throws IOException {
                      org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequest.Builder builder = CommitRequest.newBuilder();
                      builder.setTransactionId(transactionId);
                      builder.setRegionName(ByteString.copyFromUtf8(Bytes.toString(regionName))); //ByteString.copyFromUtf8(Bytes.toString(regionName)));
                      builder.setIgnoreUnknownTransactionException(ignoreUnknownTransactionException);

                      instance.commit(controller, builder.build(), rpcCallback);
                      return rpcCallback.get();
                  }
               };

               Map<byte[], CommitResponse> result = null;
               try {
                 if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- before coprocessorService txid: " + transactionId +
                        " ignoreUnknownTransactionException: " + ignoreUnknownTransactionException + " table: " +
                        table.toString() + " startKey: " + new String(startKey, "UTF-8") + " endKey: " + new String(endKey, "UTF-8"));
                 result = table.coprocessorService(TrxRegionService.class, startKey, endKey, callable);
               } catch (Throwable e) {
                  String msg = "ERROR occurred while calling doCommitX coprocessor service in doCommitX";
                  LOG.error(msg + ":" + e);
                  throw new Exception(msg);
               }
               if(result.size() != 1) {
                  LOG.error("doCommitX, received incorrect result size: " + result.size() + " txid: " + transactionId);
                  refresh = true;
                  retry = true;
               }
               else {
                  // size is 1
                  for (CommitResponse cresponse : result.values()){
                    if(cresponse.getHasException()) {
                      String exceptionString = new String (cresponse.getException().toString());
                      if (exceptionString.contains("UnknownTransactionException")) {
                        if (ignoreUnknownTransactionException == true) {
                          if (LOG.isTraceEnabled()) LOG.trace("doCommitX, ignoring UnknownTransactionException in cresponse");
                        }
                        else {
                          LOG.error("doCommitX, coprocessor UnknownTransactionException: " + cresponse.getException());
                          throw new UnknownTransactionException();
                        }
                      }
                      else {
                        if (LOG.isTraceEnabled()) LOG.trace("doCommitX coprocessor exception: " + cresponse.getException());
                        throw new Exception(cresponse.getException());
                      }
                  }
               }
               retry = false;
             }
          }
          catch (UnknownTransactionException ute) {
              LOG.error("Got unknown exception in doCommitX for transaction: " + transactionId + " " + ute);
              transactionState.requestPendingCountDec(true);
              throw new UnknownTransactionException();
          }
          catch (Exception e) {
             LOG.error("doCommitX retrying due to Exception: " + e);
             refresh = true;
             retry = true;
          }
          if (refresh) {

             HRegionLocation lv_hrl = table.getRegionLocation(startKey);
             HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
             String          lv_node = lv_hrl.getHostname();
             int             lv_length = lv_node.indexOf('.');

             if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- location being refreshed : " + location.getRegionInfo().getRegionNameAsString() + " endKey: "
                     + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " for transaction: " + transactionId);
             if(retryCount == RETRY_ATTEMPTS) {
                LOG.error("Exceeded retry attempts (" + retryCount + ") in doCommitX for transaction: " + transactionId);
                // We have received our reply in the form of an exception,
                // so decrement outstanding count and wake up waiters to avoid
                // getting hung forever
                transactionState.requestPendingCountDec(true);
                throw new CommitUnsuccessfulException("Exceeded retry attempts (" + retryCount + ") in doCommitX for transaction: " + transactionId);
             }

//             if ((location.getRegionInfo().getEncodedName().compareTo(lv_hri.getEncodedName()) != 0) ||  // Encoded name is different
//                 (location.getHostname().regionMatches(0, lv_node, 0, lv_length) == false)) {            // Node is different
                if (LOG.isWarnEnabled()) LOG.warn("doCommitX -- " + table.toString() + " location being refreshed");
                if (LOG.isWarnEnabled()) LOG.warn("doCommitX -- lv_hri: " + lv_hri);
                if (LOG.isWarnEnabled()) LOG.warn("doCommitX -- location.getRegionInfo(): " + location.getRegionInfo());
                table.getRegionLocation(startKey, true);
//             }
             if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- setting retry, count: " + retryCount);
             refresh = false;
           }

           retryCount++;

       if (retryCount < RETRY_ATTEMPTS && retry == true) {
             try {
               Thread.sleep(retrySleep);
             } catch(InterruptedException ex) {
               Thread.currentThread().interrupt();
             }

             retrySleep += TM_SLEEP_INCR;
           }

        } while (retryCount < RETRY_ATTEMPTS && retry == true);
        }

        if( TRANSACTION_ALGORITHM == AlgorithmType.SSCC){
        do {
          try {

            if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- ENTRY txid: " + transactionId +
                                                  " ignoreUnknownTransactionException: " + ignoreUnknownTransactionException);
            Batch.Call<SsccRegionService, SsccCommitResponse> callable =
               new Batch.Call<SsccRegionService, SsccCommitResponse>() {
                 ServerRpcController controller = new ServerRpcController();
                 BlockingRpcCallback<SsccCommitResponse> rpcCallback =
                    new BlockingRpcCallback<SsccCommitResponse>();

                    @Override
                    public SsccCommitResponse call(SsccRegionService instance) throws IOException {
                      org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitRequest.Builder builder = SsccCommitRequest.newBuilder();
                      builder.setTransactionId(transactionId);
                      builder.setRegionName(ByteString.copyFromUtf8(Bytes.toString(regionName))); //ByteString.copyFromUtf8(Bytes.toString(regionName)));
                      builder.setCommitId(commitId);
                      builder.setIgnoreUnknownTransactionException(ignoreUnknownTransactionException);

                      instance.commit(controller, builder.build(), rpcCallback);
                      return rpcCallback.get();
                  }
               };

               Map<byte[], SsccCommitResponse> result = null;
               try {
                 if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- before coprocessorService txid: " + transactionId +
                        " ignoreUnknownTransactionException: " + ignoreUnknownTransactionException + " table: " +
                        table.toString() + " startKey: " + new String(startKey, "UTF-8") + " endKey: " + new String(endKey, "UTF-8"));
                 result = table.coprocessorService(SsccRegionService.class, startKey, endKey, callable);
               } catch (Throwable e) {
                  String msg = "ERROR occurred while calling doCommitX coprocessor service in doCommitX";
                  LOG.error(msg + ":" + e);
                  throw new Exception(msg);
               }
               if(result.size() != 1) {
                  LOG.error("doCommitX, received incorrect result size: " + result.size() + " txid: " + transactionId);
                  refresh = true;
                  retry = true;
               }
               else {
                  // size is 1
                  for (SsccCommitResponse cresponse : result.values()){
                    if(cresponse.getHasException()) {
                      String exceptionString = new String (cresponse.getException().toString());
                      if (exceptionString.contains("UnknownTransactionException")) {
                        if (ignoreUnknownTransactionException == true) {
                          if (LOG.isTraceEnabled()) LOG.trace("doCommitX, ignoring UnknownTransactionException in cresponse");
                        }
                        else {
                          LOG.error("doCommitX, coprocessor UnknownTransactionException: " + cresponse.getException());
                          throw new UnknownTransactionException();
                        }
                      }
                      else {
                        if (LOG.isTraceEnabled()) LOG.trace("doCommitX coprocessor exception: " + cresponse.getException());
                        throw new Exception(cresponse.getException());
                      }
                  }
               }
               retry = false;
             }
          }
          catch (UnknownTransactionException ute) {
              LOG.error("Got unknown exception in doCommitX for transaction: " + transactionId + " " + ute);
              transactionState.requestPendingCountDec(true);
              throw new UnknownTransactionException();
          }
          catch (Exception e) {
             LOG.error("doCommitX retrying due to Exception: " + e);
             refresh = true;
             retry = true;
          }
          if (refresh) {

             HRegionLocation lv_hrl = table.getRegionLocation(startKey);
             HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
             String          lv_node = lv_hrl.getHostname();
             int             lv_length = lv_node.indexOf('.');

             if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- location being refreshed : " + location.getRegionInfo().getRegionNameAsString() + "endKey: "
                     + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " for transaction: " + transactionId);
             if(retryCount == RETRY_ATTEMPTS) {
                LOG.error("Exceeded retry attempts (" + retryCount + ") in doCommitX for transaction: " + transactionId);
                // We have received our reply in the form of an exception,
                // so decrement outstanding count and wake up waiters to avoid
                // getting hung forever
                transactionState.requestPendingCountDec(true);
                throw new CommitUnsuccessfulException("Exceeded retry attempts (" + retryCount + ") in doCommitX for transaction: " + transactionId);
             }

//             if ((location.getRegionInfo().getEncodedName().compareTo(lv_hri.getEncodedName()) != 0) ||  // Encoded name is different
//                 (location.getHostname().regionMatches(0, lv_node, 0, lv_length) == false)) {            // Node is different
                if (LOG.isWarnEnabled()) LOG.warn("doCommitX -- " + table.toString() + " location being refreshed");
                if (LOG.isWarnEnabled()) LOG.warn("doCommitX -- lv_hri: " + lv_hri);
                if (LOG.isWarnEnabled()) LOG.warn("doCommitX -- location.getRegionInfo(): " + location.getRegionInfo());
                table.getRegionLocation(startKey, true);
//             }
             if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- setting retry, count: " + retryCount);
             refresh = false;
           }

           retryCount++;

           if (retryCount < RETRY_ATTEMPTS && retry == true) {
             try {
               Thread.sleep(retrySleep);
             } catch(InterruptedException ex) {
               Thread.currentThread().interrupt();
             }

             retrySleep += TM_SLEEP_INCR;
           }

        } while (retryCount < RETRY_ATTEMPTS && retry == true);
        }
        // We have received our reply so decrement outstanding count
        transactionState.requestPendingCountDec(false);

        // forget the transaction if all replies have been received. otherwise another thread
        // will do it.
//        if (transactionState.requestAllComplete()){

//        }
        if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- EXIT txid: " + transactionId);
        return 0;
    }

    /**
      * Method  : doPrepareX
      * Params  : regionName - name of Region
      *           transactionId - transaction identifier
      *           location -
      * Return  : Commit vote (yes, no, read only)
      * Purpose : Call prepare for a given regionserver
     */
    public Integer doPrepareX(final byte[] regionName, final long transactionId, final TransactionRegionLocation location)
          throws IOException, CommitUnsuccessfulException {
       if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- ENTRY txid: " + transactionId );
       int commitStatus = 0;
       boolean refresh = false;
       boolean retry = false;
       int retryCount = 0;
       int retrySleep = TM_SLEEP;

       if( TRANSACTION_ALGORITHM == AlgorithmType.MVCC){
       do {
          try {
             Batch.Call<TrxRegionService, CommitRequestResponse> callable =
                new Batch.Call<TrxRegionService, CommitRequestResponse>() {
                   ServerRpcController controller = new ServerRpcController();
                BlockingRpcCallback<CommitRequestResponse> rpcCallback =
                   new BlockingRpcCallback<CommitRequestResponse>();

                @Override
                public CommitRequestResponse call(TrxRegionService instance) throws IOException {
                   org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequestRequest.Builder builder = CommitRequestRequest.newBuilder();
                   builder.setTransactionId(transactionId);
                   builder.setRegionName(ByteString.copyFromUtf8(Bytes.toString(regionName)));

                   instance.commitRequest(controller, builder.build(), rpcCallback);
                   return rpcCallback.get();
                }
             };

             Map<byte[], CommitRequestResponse> result = null;

             try {
//                if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- before coprocessorService txid: " + transactionId + " table: " + table.toString());
//                if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- txid: " + transactionId + " table: " + table.toString() + " endKey_Orig: " + new String(endKey_orig, "UTF-8"));
//                if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- " + table.toString() + " startKey: " + new String(startKey, "UTF-8") + " endKey: " + new String(endKey, "UTF-8"));

//                HRegionLocation lv_hrl = table.getRegionLocation(startKey);
//                HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
//                String          lv_node = lv_hrl.getHostname();
//                int             lv_length = lv_node.indexOf('.');

//                if ((location.getRegionInfo().getEncodedName().compareTo(lv_hri.getEncodedName()) != 0) ||  // Encoded name is different
//                        (location.getHostname().regionMatches(0, lv_node, 0, lv_length) == false)) {            // Node is different
//                   if (LOG.isInfoEnabled())LOG.info("doPrepareX -- " + table.toString() + " location being refreshed");
//                   if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- lv_hri: " + lv_hri);
//                   if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- location.getRegionInfo(): " + location.getRegionInfo());
//                   if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- lv_node: " + lv_node + " lv_length: " + lv_length);
//                   if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- location.getHostname(): " + location.getHostname());
//                   table.getRegionLocation(startKey, true);
//                }
                result = table.coprocessorService(TrxRegionService.class, startKey, endKey, callable);
             } catch (Throwable e) {
                LOG.error("doPrepareX coprocessor error for " + Bytes.toString(regionName) + " txid: " + transactionId + ":" + e);
                throw new CommitUnsuccessfulException("Unable to call prepare, coprocessor error");
             }

             if(result.size() != 1)  {
                LOG.error("doPrepareX, received incorrect result size: " + result.size());
                refresh = true;
                retry = true;
             }
             else {
                // size is 1
                for (CommitRequestResponse cresponse : result.values()){
                   // Should only be one result
                   int value = cresponse.getResult();
                   commitStatus = value;
                   if(cresponse.getHasException()) {
                      if (LOG.isTraceEnabled()) LOG.trace("doPrepareX coprocessor exception: " + cresponse.getException());
                      throw new Exception(cresponse.getException());
                   }
                }
                retry = false;
             }
          }
          catch(UnknownTransactionException ute) {
             LOG.warn("doPrepareX Exception: " + ute);
             throw new UnknownTransactionException();
          }
          catch(Exception e) {
             LOG.error("doPrepareX retrying due to Exception: " + e);
             refresh = true;
             retry = true;
          }
          if (refresh) {

             HRegionLocation lv_hrl = table.getRegionLocation(startKey);
             HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
             String          lv_node = lv_hrl.getHostname();
             int             lv_length = lv_node.indexOf('.');

             if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- location being refreshed : " + location.getRegionInfo().getRegionNameAsString() + "endKey: "
                     + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " for transaction: " + transactionId);
             if(retryCount == RETRY_ATTEMPTS){
                LOG.error("Exceeded retry attempts in doPrepareX: " + retryCount);
                // We have received our reply in the form of an exception,
                // so decrement outstanding count and wake up waiters to avoid
                // getting hung forever
                transactionState.requestPendingCountDec(true);
                throw new CommitUnsuccessfulException("Exceeded retry attempts in doPrepareX: " + retryCount);
             }
//             if ((location.getRegionInfo().getEncodedName().compareTo(lv_hri.getEncodedName()) != 0) ||  // Encoded name is different
//                 (location.getHostname().regionMatches(0, lv_node, 0, lv_length) == false)) {            // Node is different
                if (LOG.isWarnEnabled()) LOG.warn("doPrepareX -- " + table.toString() + " location being refreshed");
                if (LOG.isWarnEnabled()) LOG.warn("doPrepareX -- lv_hri: " + lv_hri);
                if (LOG.isWarnEnabled()) LOG.warn("doPrepareX -- location.getRegionInfo(): " + location.getRegionInfo());

                table.getRegionLocation(startKey, true);
                LOG.debug("doPrepareX retry count: " + retryCount);
//             }
             if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- setting retry, count: " + retryCount);
             refresh = false;
          }

          retryCount++;

          if (retryCount < RETRY_ATTEMPTS && retry == true) {
            try {
              Thread.sleep(retrySleep);
            } catch(InterruptedException ex) {
              Thread.currentThread().interrupt();
            }

             retrySleep += TM_SLEEP_INCR;
          }

       } while (retryCount < RETRY_ATTEMPTS && retry == true);
       }
       if( TRANSACTION_ALGORITHM == AlgorithmType.SSCC){
       do {
          try {
             Batch.Call<SsccRegionService, SsccCommitRequestResponse> callable =
                new Batch.Call<SsccRegionService, SsccCommitRequestResponse>() {
                   ServerRpcController controller = new ServerRpcController();
                BlockingRpcCallback<SsccCommitRequestResponse> rpcCallback =
                   new BlockingRpcCallback<SsccCommitRequestResponse>();

                @Override
                public SsccCommitRequestResponse call(SsccRegionService instance) throws IOException {
                   org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitRequestRequest.Builder builder = SsccCommitRequestRequest.newBuilder();
                   builder.setTransactionId(transactionId);
                   builder.setRegionName(ByteString.copyFromUtf8(Bytes.toString(regionName)));

                   instance.commitRequest(controller, builder.build(), rpcCallback);
                   return rpcCallback.get();
                }
             };

             Map<byte[], SsccCommitRequestResponse> result = null;

             try {
                result = table.coprocessorService(SsccRegionService.class, startKey, endKey, callable);
             } catch (Throwable e) {
                LOG.error("doPrepareX coprocessor error for " + Bytes.toString(regionName) + " txid: " + transactionId + ":" + e);
                throw new CommitUnsuccessfulException("Unable to call prepare, coprocessor error");
             }

             if(result.size() != 1)  {
                LOG.error("doPrepareX, received incorrect result size: " + result.size());
                refresh = true;
                retry = true;
             }
             else {
                // size is 1
                for (SsccCommitRequestResponse cresponse : result.values()){
                   // Should only be one result
                   int value = cresponse.getResult();
                   commitStatus = value;
                   if(cresponse.getHasException()) {
                      if (LOG.isTraceEnabled()) LOG.trace("doPrepareX coprocessor exception: " + cresponse.getException());
                      throw new Exception(cresponse.getException());
                   }
                }
                retry = false;
             }
          }
          catch(UnknownTransactionException ute) {
             LOG.warn("doPrepareX Exception: " + ute);
             throw new UnknownTransactionException();
          }
          catch(Exception e) {
             LOG.error("doPrepareX retrying due to Exception: " + e);
             refresh = true;
             retry = true;
          }
          if (refresh) {

             HRegionLocation lv_hrl = table.getRegionLocation(startKey);
             HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
             String          lv_node = lv_hrl.getHostname();
             int             lv_length = lv_node.indexOf('.');

             if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- location being refreshed : " + location.getRegionInfo().getRegionNameAsString() + "endKey: "
                     + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " for transaction: " + transactionId);
             if(retryCount == RETRY_ATTEMPTS){
                LOG.error("Exceeded retry attempts in doPrepareX: " + retryCount);
                // We have received our reply in the form of an exception,
                // so decrement outstanding count and wake up waiters to avoid
                // getting hung forever
                transactionState.requestPendingCountDec(true);
                throw new CommitUnsuccessfulException("Exceeded retry attempts in doPrepareX: " + retryCount);
             }
//             if ((location.getRegionInfo().getEncodedName().compareTo(lv_hri.getEncodedName()) != 0) ||  // Encoded name is different
//                 (location.getHostname().regionMatches(0, lv_node, 0, lv_length) == false)) {            // Node is different
                if (LOG.isWarnEnabled()) LOG.warn("doPrepareX -- " + table.toString() + " location being refreshed");
                if (LOG.isWarnEnabled()) LOG.warn("doPrepareX -- lv_hri: " + lv_hri);
                if (LOG.isWarnEnabled()) LOG.warn("doPrepareX -- location.getRegionInfo(): " + location.getRegionInfo());

                table.getRegionLocation(startKey, true);
                LOG.debug("doPrepareX retry count: " + retryCount);
//             }
             if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- setting retry, count: " + retryCount);
             refresh = false;
          }

          retryCount++;

          if (retryCount < RETRY_ATTEMPTS && retry == true) {
            try {
              Thread.sleep(retrySleep);
            } catch(InterruptedException ex) {
              Thread.currentThread().interrupt();
            }

            retrySleep += TM_SLEEP_INCR;
          }

       } while (retryCount < RETRY_ATTEMPTS && retry == true);
       }
       if (LOG.isTraceEnabled()) LOG.trace("commitStatus for transId(" + transactionId + "): " + commitStatus);
       boolean canCommit = true;
       boolean readOnly = false;

       switch (commitStatus) {
          case TransactionalReturn.COMMIT_OK:
            break;
          case TransactionalReturn.COMMIT_OK_READ_ONLY:
            transactionState.addRegionToIgnore(location); // No need to doCommit for read-onlys
            readOnly = true;
            break;
          case TransactionalReturn.COMMIT_UNSUCCESSFUL_FROM_COPROCESSOR:
            if (LOG.isTraceEnabled()) LOG.trace("Received COMMIT_UNSUCCESSFUL_FROM_COPROCESSOR return code from requestCommit " + commitStatus + " for transaction " + transactionId);
          case TransactionalReturn.COMMIT_CONFLICT:
          case TransactionalReturn.COMMIT_UNSUCCESSFUL:
             canCommit = false;
             transactionState.addRegionToIgnore(location); // No need to re-abort.
             break;
          default:
             LOG.warn("Received invalid return code from requestCommit " + commitStatus + " for transaction " + transactionId + " throwing CommitUnsuccessfulException");
             throw new CommitUnsuccessfulException("Unexpected return code from prepareCommit: " + commitStatus);
       }

       if (!canCommit) {
         // track regions which indicate they could not commit for better diagnostics
         LOG.warn("Region [" + location.getRegionInfo().getRegionNameAsString() + "] votes "
                 +  "to abort" + (readOnly ? " Read-only ":"") + " transaction "
                 + transactionState.getTransactionId());
         //System.out.println("Region [" + location.getRegionInfo().getRegionNameAsString() + "] votes "
         //        +  "to abort" + (readOnly ? " Read-only ":"") + " transaction "
         //        + transactionState.getTransactionId());
         if(commitStatus == TransactionalReturn.COMMIT_CONFLICT)
              return TM_COMMIT_FALSE_CONFLICT;
         return TM_COMMIT_FALSE;
       }

       if (readOnly)
         return TM_COMMIT_READ_ONLY;

       return TM_COMMIT_TRUE;
  }

    /**
     * Method  : doAbortX
     * Params  : regionName - name of Region
     *           transactionId - transaction identifier
     * Return  : Ignored
     * Purpose : Call abort for a given regionserver
     */
    public Integer doAbortX(final byte[] regionName, final long transactionId) throws IOException{
        if(LOG.isTraceEnabled()) LOG.trace("doAbortX -- ENTRY txID: " + transactionId);
        boolean retry = false;
            boolean refresh = false;
        int retryCount = 0;
            int retrySleep = TM_SLEEP;

        if( TRANSACTION_ALGORITHM == AlgorithmType.MVCC){
        do {
            try {

              Batch.Call<TrxRegionService, AbortTransactionResponse> callable =
                new Batch.Call<TrxRegionService, AbortTransactionResponse>() {
              ServerRpcController controller = new ServerRpcController();
              BlockingRpcCallback<AbortTransactionResponse> rpcCallback =
                new BlockingRpcCallback<AbortTransactionResponse>();

              @Override
              public AbortTransactionResponse call(TrxRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.AbortTransactionRequest.Builder builder = AbortTransactionRequest.newBuilder();
                builder.setTransactionId(transactionId);
                builder.setRegionName(ByteString.copyFromUtf8(Bytes.toString(regionName)));

                instance.abortTransaction(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
              }
            };

            Map<byte[], AbortTransactionResponse> result = null;
              try {
                      if (LOG.isTraceEnabled()) LOG.trace("doAbortX -- before coprocessorService txid: " + transactionId + " table: " +
                        table.toString() + " startKey: " + new String(startKey, "UTF-8") + " endKey: " + new String(endKey, "UTF-8"));
                      result = table.coprocessorService(TrxRegionService.class, startKey, endKey, callable);
              } catch (Throwable e) {
                  String msg = "ERROR occurred while calling doAbortX coprocessor service";
                  LOG.error(msg + ":" + e);
                  throw new Exception(msg);
              }

              if(result.size() != 1) {
                     LOG.error("doAbortX, received incorrect result size: " + result.size());
                     refresh = true;
                     retry = true;
                  }
                  else {
                     for (AbortTransactionResponse cresponse : result.values()) {
                if(cresponse.getHasException()) {
                  String exceptionString = cresponse.getException().toString();
                  LOG.error("Abort HasException true: " + exceptionString);
                  if(exceptionString.contains("UnknownTransactionException")) {
                         throw new UnknownTransactionException();
                  }
                  throw new Exception(cresponse.getException());
                }
              }
              retry = false;
          }
              }
          catch (UnknownTransactionException ute) {
                 LOG.debug("UnknownTransactionException in doAbortX for transaction: " + transactionId + "(ignoring): " + ute);
          }
          catch (Exception e) {
                LOG.error("doAbortX retrying due to Exception: " + e );
                refresh = true;
                retry = true;
              }
              if (refresh) {

                 HRegionLocation lv_hrl = table.getRegionLocation(startKey);
                 HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
                 String          lv_node = lv_hrl.getHostname();
                 int             lv_length = lv_node.indexOf('.');

                 if (LOG.isTraceEnabled()) LOG.trace("doAbortX -- location being refreshed : " + location.getRegionInfo().getRegionNameAsString() + "endKey: "
                     + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " for transaction: " + transactionId);
                 if(retryCount == RETRY_ATTEMPTS){
                    LOG.error("Exceeded retry attempts in doAbortX: " + retryCount + " (ingoring)");
                 }

//                 if ((location.getRegionInfo().getEncodedName().compareTo(lv_hri.getEncodedName()) != 0) ||  // Encoded name is different
//                     (location.getHostname().regionMatches(0, lv_node, 0, lv_length) == false)) {            // Node is different
                    if (LOG.isWarnEnabled()) LOG.warn("doAbortX -- " + table.toString() + " location being refreshed");
                    if (LOG.isWarnEnabled()) LOG.warn("doAbortX -- lv_hri: " + lv_hri);
                    if (LOG.isWarnEnabled()) LOG.warn("doAbortX -- location.getRegionInfo(): " + location.getRegionInfo());
                    table.getRegionLocation(startKey, true);
//                 }
                 if (LOG.isTraceEnabled()) LOG.trace("doAbortX -- setting retry, count: " + retryCount);
                 refresh = false;
              }

              retryCount++;

          if (retryCount < RETRY_ATTEMPTS && retry == true) {
                try {
                  Thread.sleep(retrySleep);
                } catch(InterruptedException ex) {
                  Thread.currentThread().interrupt();
                }

                retrySleep += TM_SLEEP_INCR;
              }

      } while (retryCount < RETRY_ATTEMPTS && retry == true);
        }
        if( TRANSACTION_ALGORITHM == AlgorithmType.SSCC){
        do {
             try {

              Batch.Call<SsccRegionService, SsccAbortTransactionResponse> callable =
                new Batch.Call<SsccRegionService, SsccAbortTransactionResponse>() {
              ServerRpcController controller = new ServerRpcController();
              BlockingRpcCallback<SsccAbortTransactionResponse> rpcCallback =
                new BlockingRpcCallback<SsccAbortTransactionResponse>();

              @Override
              public SsccAbortTransactionResponse call(SsccRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccAbortTransactionRequest.Builder builder = SsccAbortTransactionRequest.newBuilder();
                builder.setTransactionId(transactionId);
                builder.setRegionName(ByteString.copyFromUtf8(Bytes.toString(regionName)));

                instance.abortTransaction(controller, builder.build(), rpcCallback);
                return rpcCallback.get();
              }
            };

            Map<byte[], SsccAbortTransactionResponse> result = null;
              try {
                      if (LOG.isTraceEnabled()) LOG.trace("doAbortX -- before coprocessorService txid: " + transactionId + " table: " +
                        table.toString() + " startKey: " + new String(startKey, "UTF-8") + " endKey: " + new String(endKey, "UTF-8"));
                      result = table.coprocessorService(SsccRegionService.class, startKey, endKey, callable);
              } catch (Throwable e) {
                  String msg = "ERROR occurred while calling doAbortX coprocessor service";
                  LOG.error(msg + ":" + e);
                  throw new Exception(msg);
              }

              if(result.size() != 1) {
                     LOG.error("doAbortX, received incorrect result size: " + result.size());
                     refresh = true;
                     retry = true;
                  }
                  else {
                     for (SsccAbortTransactionResponse cresponse : result.values()) {
                if(cresponse.getHasException()) {
                  String exceptionString = cresponse.getException().toString();
                  LOG.error("Abort HasException true: " + exceptionString);
                  if(exceptionString.contains("UnknownTransactionException")) {
                         throw new UnknownTransactionException();
                  }
                  throw new Exception(cresponse.getException());
                }
              }
              retry = false;
          }
              }
          catch (UnknownTransactionException ute) {
                 LOG.debug("UnknownTransactionException in doAbortX for transaction: " + transactionId + "(ignoring): " + ute);
          }
          catch (Exception e) {
                LOG.error("doAbortX retrying due to Exception: " + e );
                refresh = true;
                retry = true;
              }
              if (refresh) {

                 HRegionLocation lv_hrl = table.getRegionLocation(startKey);
                 HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
                 String          lv_node = lv_hrl.getHostname();
                 int             lv_length = lv_node.indexOf('.');

                 if (LOG.isTraceEnabled()) LOG.trace("doAbortX -- location being refreshed : " + location.getRegionInfo().getRegionNameAsString() + "endKey: "
                     + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " for transaction: " + transactionId);
                 if(retryCount == RETRY_ATTEMPTS){
                    LOG.error("Exceeded retry attempts in doAbortX: " + retryCount + " (ingoring)");
                 }

//                 if ((location.getRegionInfo().getEncodedName().compareTo(lv_hri.getEncodedName()) != 0) ||  // Encoded name is different
//                     (location.getHostname().regionMatches(0, lv_node, 0, lv_length) == false)) {            // Node is different
                    if (LOG.isWarnEnabled()) LOG.warn("doAbortX -- " + table.toString() + " location being refreshed");
                    if (LOG.isWarnEnabled()) LOG.warn("doAbortX -- lv_hri: " + lv_hri);
                    if (LOG.isWarnEnabled()) LOG.warn("doAbortX -- location.getRegionInfo(): " + location.getRegionInfo());
                    table.getRegionLocation(startKey, true);
//                 }
                 if (LOG.isTraceEnabled()) LOG.trace("doAbortX -- setting retry, count: " + retryCount);
                 refresh = false;
              }

              retryCount++;

          if (retryCount < RETRY_ATTEMPTS && retry == true) {
                try {
                  Thread.sleep(retrySleep);
                } catch(InterruptedException ex) {
                  Thread.currentThread().interrupt();
                }

                retrySleep += TM_SLEEP_INCR;
              }

      } while (retryCount < RETRY_ATTEMPTS && retry == true);
        }
      // We have received our reply so decrement outstanding count
      transactionState.requestPendingCountDec(false);

      // forget the transaction if all replies have been received.
      //  otherwise another thread will do it
//      if (transactionState.requestAllComplete())
//      {
//      }
      if(LOG.isTraceEnabled()) LOG.trace("doAbortX -- EXIT txID: " + transactionId);
      return 0;
    }

    public Integer doCommitX(final List<TransactionRegionLocation> locations, final long transactionId, final long commitId, final boolean ignoreUnknownTransactionException) throws CommitUnsuccessfulException, IOException {
        boolean retry = false;
        boolean refresh = false;

        int retryCount = 0;
        do {
          try {

            if (LOG.isTraceEnabled()) LOG.trace("doCommitX - Batch -- ENTRY txid: " + transactionId +
                                                  " ignoreUnknownTransactionException: " + ignoreUnknownTransactionException);

            TrxRegionProtos.CommitMultipleRequest.Builder builder = CommitMultipleRequest.newBuilder();
            builder.setTransactionId(transactionId);
            for(TransactionRegionLocation location : locations) {
               builder.addRegionName(ByteString.copyFrom(location.getRegionInfo().getRegionName()));
            }
            builder.setIgnoreUnknownTransactionException(ignoreUnknownTransactionException);
            CommitMultipleRequest commitMultipleRequest = builder.build();
            CommitMultipleResponse commitMultipleResponse = null;

            try {
                CoprocessorRpcChannel channel = table.coprocessorService(startKey);
                TrxRegionService.BlockingInterface trxService = TrxRegionService.newBlockingStub(channel);
                commitMultipleResponse = trxService.commitMultiple(null, commitMultipleRequest);
                retry = false;
            } catch (Throwable e) {
              LOG.error("doCommitX coprocessor error for " + Bytes.toString(locations.iterator().next().getRegionInfo().getRegionName()) + " txid: " + transactionId + ":" + e);
              refresh = true;
              retry = true;
          }
          if(!retry) {
              List<String> exceptions = commitMultipleResponse.getExceptionList();

              checkException(transactionState, locations, exceptions);
              if(transactionState.getRegionsRetryCount() > 0) {
                  retryCommit(transactionState, true);
              }
           }
        }
        catch (UnknownTransactionException ute) {
            String errMsg = "Got unknown exception in doCommitX for transaction: " + transactionId + " " + ute;
            LOG.error(errMsg);
            transactionState.requestPendingCountDec(true);
            throw new UnknownTransactionException(errMsg);
        }
        catch (Exception e) {
           LOG.error("doCommitX retrying due to Exception: " + e);
           refresh = true;
           retry = true;
        }
        if (refresh) {

           HRegionLocation lv_hrl = table.getRegionLocation(startKey);
           HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
           String          lv_node = lv_hrl.getHostname();
           int             lv_length = lv_node.indexOf('.');

           if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- location being refreshed : " + location.getRegionInfo().getRegionNameAsString() + "endKey: "
                   + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " for transaction: " + transactionId);
           if(retryCount == RETRY_ATTEMPTS) {
              LOG.error("Exceeded retry attempts (" + retryCount + ") in doCommitX for transaction: " + transactionId);
              transactionState.requestPendingCountDec(true);
              throw new CommitUnsuccessfulException("Exceeded retry attempts (" + retryCount + ") in doCommitX for transaction: " + transactionId);
           }

         if (LOG.isWarnEnabled()) {
           LOG.warn("doCommitX -- " + table.toString() + " location being refreshed");
           LOG.warn("doCommitX -- lv_hri: " + lv_hri);
           LOG.warn("doCommitX -- location.getRegionInfo(): " + location.getRegionInfo());
         }
         table.getRegionLocation(startKey, true);

         if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- setting retry, count: " + retryCount);
         refresh = false;
         retryCount++;
       }
    } while (retryCount < RETRY_ATTEMPTS && retry == true);

    transactionState.requestPendingCountDec(false);

    if (LOG.isTraceEnabled()) LOG.trace("doCommitX - Batch -- EXIT txid: " + transactionId);
    return 0;
  }

  public Integer doPrepareX(final List<TransactionRegionLocation> locations, final long transactionId)
   throws IOException, CommitUnsuccessfulException {
    if (LOG.isTraceEnabled()) LOG.trace("doPrepareX - Batch -- ENTRY txid: " + transactionId );

    boolean refresh = false;
    boolean retry = false;
    int retryCount = 0;
    List<Integer> results = null;
    do {
       try {

          TrxRegionProtos.CommitRequestMultipleRequest.Builder builder = CommitRequestMultipleRequest.newBuilder();
          builder.setTransactionId(transactionId);
          for(TransactionRegionLocation location : locations) {
             builder.addRegionName(ByteString.copyFrom(location.getRegionInfo().getRegionName()));
          }
          TrxRegionProtos.CommitRequestMultipleRequest commitMultipleRequest = builder.build();
          TrxRegionProtos.CommitRequestMultipleResponse commitMultipleResponse = null;

          try {
              CoprocessorRpcChannel channel = table.coprocessorService(startKey);
              TrxRegionService.BlockingInterface trxService = TrxRegionService.newBlockingStub(channel);
              commitMultipleResponse = trxService.commitRequestMultiple(null, commitMultipleRequest);
              retry = false;
          } catch (Throwable e) {
              LOG.error("doPrepareX coprocessor error for " + Bytes.toString(locations.iterator().next().getRegionInfo().getRegionName()) + " txid: " + transactionId + ":" + e);
              refresh = true;
              retry = true;
              //throw new CommitUnsuccessfulException("Unable to call prepare, coprocessor error");
          }
          if(!retry) {
              results = commitMultipleResponse.getResultList();
                    //commitStatus = value;
              List<String> exceptions = commitMultipleResponse.getExceptionList();
              checkException(transactionState, locations, exceptions);

          }
       }
       catch(UnknownTransactionException ute) {
          String warnMsg = "doPrepareX Exception: " + ute;
          LOG.warn(warnMsg);
          throw new UnknownTransactionException(warnMsg);
       }
       catch(Exception e) {
          LOG.error("doPrepareX retrying due to Exception: " + e);
          refresh = true;
          retry = true;
       }
       if (refresh) {
         HRegionLocation lv_hrl = table.getRegionLocation(startKey);
         HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
         String          lv_node = lv_hrl.getHostname();
         int             lv_length = lv_node.indexOf('.');

         if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- location being refreshed : " + location.getRegionInfo().getRegionNameAsString() + "endKey: "
                  + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " for transaction: " + transactionId);
         if(retryCount == RETRY_ATTEMPTS){
            LOG.error("Exceeded retry attempts in doPrepareX: " + retryCount);
            // We have received our reply in the form of an exception,
            // so decrement outstanding count and wake up waiters to avoid
            // getting hung forever
            transactionState.requestPendingCountDec(true);
            throw new CommitUnsuccessfulException("Exceeded retry attempts in doPrepareX: " + retryCount);
         }
         if (LOG.isWarnEnabled()) {
            LOG.warn("doPrepareX -- " + table.toString() + " location being refreshed");
            LOG.warn("doPrepareX -- lv_hri: " + lv_hri);
            LOG.warn("doPrepareX -- location.getRegionInfo(): " + location.getRegionInfo());
         }

         table.getRegionLocation(startKey, true);
         if (LOG.isDebugEnabled()) LOG.debug("doPrepareX retry count: " + retryCount);
         if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- setting retry, count: " + retryCount);
         refresh = false;
         retryCount++;
      }
    } while (retryCount < RETRY_ATTEMPTS && retry == true);

    // Process the results of the list here

    // if (LOG.isTraceEnabled()) LOG.trace("commitStatus: " + commitStatus);
    boolean canCommit = true;
    boolean readOnly = true;
    int commitErr = TransactionalReturn.COMMIT_OK;
    int resultCount = 0;

    for(Integer commitStatus : results) {
       switch (commitStatus) {
          case 0:
            break;
          case TransactionalReturn.COMMIT_OK:
            readOnly = false;
            break;
          case TransactionalReturn.COMMIT_OK_READ_ONLY:
            transactionState.addRegionToIgnore(locations.get(resultCount)); // No need to doCommit for read-onlys
            //readOnly = true;
            break;
          case TransactionalReturn.COMMIT_UNSUCCESSFUL_FROM_COPROCESSOR:
            if (LOG.isTraceEnabled()) LOG.trace("Received COMMIT_UNSUCCESSFUL_FROM_COPROCESSOR return code from requestCommit " + commitStatus + " for transaction " + transactionId);
          case TransactionalReturn.COMMIT_CONFLICT:
             commitErr = TransactionalReturn.COMMIT_CONFLICT;
          case TransactionalReturn.COMMIT_UNSUCCESSFUL:
             if(commitErr == TransactionalReturn.COMMIT_OK)
                 commitErr = TransactionalReturn.COMMIT_UNSUCCESSFUL;
             canCommit = false;
             readOnly = false;
             transactionState.addRegionToIgnore(locations.get(resultCount)); // No need to re-abort.
             break;
          default:
             LOG.warn("Received invalid return code from requestCommit " + commitStatus + " for transaction " + transactionId + " throwing CommitUnsuccessfulException");
             throw new CommitUnsuccessfulException("Unexpected return code from prepareCommit: " + commitStatus);
       }
       resultCount++;
    }

    if (LOG.isTraceEnabled()) LOG.trace("doPrepareX - Batch -- EXIT txid: " + transactionId );
    if (!canCommit) {
     // track regions which indicate they could not commit for better diagnostics
     LOG.warn("Region [" + location.getRegionInfo().getRegionNameAsString() + "] votes "
             +  "to abort" + (readOnly ? " Read-only ":"") + " transaction "
             + transactionState.getTransactionId());
     //System.out.println("Region [" + location.getRegionInfo().getRegionNameAsString() + "] votes "
     //        +  "to abort" + (readOnly ? " Read-only ":"") + " transaction "
     //        + transactionState.getTransactionId());
     if(commitErr == TransactionalReturn.COMMIT_CONFLICT) {
          return TM_COMMIT_FALSE_CONFLICT;
     }

     return TM_COMMIT_FALSE;
    }
    if (readOnly) {

    return TM_COMMIT_READ_ONLY;
    }

    return TM_COMMIT_TRUE;
  }

  public Integer doAbortX(final List<TransactionRegionLocation> locations, final long transactionId) throws IOException{
    if(LOG.isTraceEnabled()) LOG.trace("doAbortX - Batch -- ENTRY txID: " + transactionId);
    boolean retry = false;
    boolean refresh = false;
    int retryCount = 0;
    do {
      try {
          TrxRegionProtos.AbortTransactionMultipleRequest.Builder builder = AbortTransactionMultipleRequest.newBuilder();
          builder.setTransactionId(transactionId);
          for(TransactionRegionLocation location : locations) {
              builder.addRegionName(ByteString.copyFrom(location.getRegionInfo().getRegionName()));
         }
          AbortTransactionMultipleRequest abortTransactionMultipleRequest = builder.build();
          AbortTransactionMultipleResponse abortTransactionMultipleResponse = null;
          try {
              CoprocessorRpcChannel channel = table.coprocessorService(startKey);
              TrxRegionService.BlockingInterface trxService = TrxRegionService.newBlockingStub(channel);
              abortTransactionMultipleResponse = trxService.abortTransactionMultiple(null, abortTransactionMultipleRequest);
              retry = false;
          } catch (Throwable e) {
              LOG.error("doAbortX coprocessor error for " + Bytes.toString(locations.iterator().next().getRegionInfo().getRegionName()) + " txid: " + transactionId + ":" + e);
              refresh = true;
              retry = true;
          }
          if(!retry) {
              List<String> exceptions = abortTransactionMultipleResponse.getExceptionList();
              checkException(transactionState, locations, exceptions);
              if(transactionState.getRegionsRetryCount() > 0) {
                  retryAbort(transactionState);
              }
          }
         }
         catch (UnknownTransactionException ute) {
                LOG.debug("UnknownTransactionException in doAbortX for transaction: " + transactionId + "(ignoring): " + ute);
         }
         catch (Exception e) {
               LOG.error("doAbortX retrying due to Exception: " + e );
               refresh = true;
               retry = true;
         }
         if (refresh) {
            HRegionLocation lv_hrl = table.getRegionLocation(startKey);
            HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
            String          lv_node = lv_hrl.getHostname();
            int             lv_length = lv_node.indexOf('.');

            if (LOG.isTraceEnabled()) LOG.trace("doAbortX -- location being refreshed : " + location.getRegionInfo().getRegionNameAsString() + "endKey: "
                     + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " for transaction: " + transactionId);
            if(retryCount == RETRY_ATTEMPTS){
               LOG.error("Exceeded retry attempts in doAbortX: " + retryCount + " (ingoring)");
            }

           if (LOG.isWarnEnabled()) {
             LOG.warn("doAbortX -- " + table.toString() + " location being refreshed");
             LOG.warn("doAbortX -- lv_hri: " + lv_hri);
             LOG.warn("doAbortX -- location.getRegionInfo(): " + location.getRegionInfo());
           }
           table.getRegionLocation(startKey, true);

            if (LOG.isTraceEnabled()) LOG.trace("doAbortX -- setting retry, count: " + retryCount);
            refresh = false;
            retryCount++;
       }
      } while (retryCount < RETRY_ATTEMPTS && retry == true);

      transactionState.requestPendingCountDec(false);
      if(LOG.isTraceEnabled()) LOG.trace("doAbortX - Batch -- EXIT txID: " + transactionId);
      return 0;
    }
  } // TransactionManagerCallable

  private void checkException(TransactionState ts, List<TransactionRegionLocation> locations, List<String> exceptions) throws IOException {
    if(LOG.isTraceEnabled()) LOG.trace("checkException -- ENTRY txid: " + ts.getTransactionId());
    ts.clearRetryRegions();
    Iterator<String> exceptionIt = exceptions.iterator();
    StringBuilder logException = new StringBuilder();
    for(int i=0; i < exceptions.size(); i++) {
        String exception = exceptionIt.next();
        if(exception.equals(BatchException.EXCEPTION_OK.toString())) {
            continue;
        }
        else if (exception.equals(BatchException.EXCEPTION_UNKNOWNTRX_ERR.toString()) ||
                 exception.equals(BatchException.EXCEPTION_NORETRY_ERR.toString())) {
            // No need to add to retry list, throw exception if not ignoring
            logException.append("Encountered " + exception + " on region: " +
                                 locations.get(i).getRegionInfo().getRegionNameAsString());
        }
        else if (exception.equals(BatchException.EXCEPTION_RETRY_ERR.toString()) ||
                 exception.equals(BatchException.EXCEPTION_REGIONNOTFOUND_ERR.toString())) {
            if(LOG.isWarnEnabled()) LOG.warn("Encountered batch error, adding region to retry list: " +
                                              locations.get(i).getRegionInfo().getRegionNameAsString());
            ts.addRegionToRetry(locations.get(i));
        }
        if(logException.length() > 0) {
            throw new IOException(logException.toString());
        }
    }
    if(LOG.isTraceEnabled()) LOG.trace("checkException -- EXIT txid: " + ts.getTransactionId());

}

  /**
   * threadPool - pool of thread for asynchronous requests
   */
    ExecutorService threadPool;

    /**
     * @param conf
     * @throws ZooKeeperConnectionException
     */
    private TransactionManager(final Configuration conf) throws ZooKeeperConnectionException, IOException {
        this(LocalTransactionLogger.getInstance(), conf);

        int intThreads = 16;
        String retryAttempts = System.getenv("TMCLIENT_RETRY_ATTEMPTS");
        String numThreads = System.getenv("TM_JAVA_THREAD_POOL_SIZE");
        String numCpThreads = System.getenv("TM_JAVA_CP_THREAD_POOL_SIZE");
        String useSSCC = System.getenv("TM_USE_SSCC");
        String batchRSMetricStr = System.getenv("TM_BATCH_RS_METRICS");
        String batchRS = System.getenv("TM_BATCH_REGIONSERVER");

        if (batchRSMetricStr != null)
                batchRSMetricsFlag = (Integer.parseInt(batchRSMetricStr) == 1) ? true : false;

        if (batchRS != null)
            batchRegionServer = (Integer.parseInt(batchRS) == 1)? true : false;

        if (retryAttempts != null)
            RETRY_ATTEMPTS = Integer.parseInt(retryAttempts);
        else
            RETRY_ATTEMPTS = TM_RETRY_ATTEMPTS;

        if (numThreads != null)
            intThreads = Integer.parseInt(numThreads);

        int intCpThreads = intThreads;
        if (numCpThreads != null)
            intCpThreads = Integer.parseInt(numCpThreads);

        TRANSACTION_ALGORITHM = AlgorithmType.MVCC;
        if (useSSCC != null)
           TRANSACTION_ALGORITHM = (Integer.parseInt(useSSCC) == 1) ? AlgorithmType.SSCC :AlgorithmType.MVCC ;

        try {
           idServer = new IdTm(false);
        }
        catch (Exception e){
           LOG.error("Exception creating new IdTm: " + e);
        }

        threadPool = Executors.newFixedThreadPool(intThreads);

    cp_tpe = Executors.newFixedThreadPool(intCpThreads);

    /* This was in the HTable code - let's use a fixed thread pool instead
    cp_tpe = new ThreadPoolExecutor(1,
                    intCpThreads,
                    30, TimeUnit.SECONDS,
                    new SynchronousQueue<Runnable>(),
                    Threads.newDaemonThreadFactory("htable_tmcp"));
    */
    }

    /**
     * @param transactionLogger
     * @param conf
     * @throws ZooKeeperConnectionException
     */
    protected TransactionManager(final TransactionLogger transactionLogger, final Configuration conf)
            throws ZooKeeperConnectionException, IOException {
        this.transactionLogger = transactionLogger;
        conf.setInt("hbase.client.retries.number", 3);
        connection = HConnectionManager.createConnection(conf);
    }


    /**
     * Called to start a transaction.
     *
     * @return new transaction state
     */
    public TransactionState beginTransaction() {
        long transactionId = transactionLogger.createNewTransactionLog();
        if (LOG.isTraceEnabled()) LOG.trace("Beginning transaction " + transactionId);
        return new TransactionState(transactionId);
    }

    /**
     * Called to start a transaction with transactionID input
     *
     * @return new transaction state
     */
    public TransactionState beginTransaction(long transactionId) throws IdTmException {
        //long transactionId =
      if (LOG.isTraceEnabled()) LOG.trace("Enter beginTransaction, txid: " + transactionId);
      TransactionState ts = new TransactionState(transactionId);
      long startIdVal = -1;

      // Set the startid
      if (ts.islocalTransaction() && (TRANSACTION_ALGORITHM == AlgorithmType.SSCC)) {
         IdTmId startId;
         try {
            startId = new IdTmId();
            if (LOG.isTraceEnabled()) LOG.trace("beginTransaction (local) getting new startId");
            idServer.id(ID_TM_SERVER_TIMEOUT, startId);
            if (LOG.isTraceEnabled()) LOG.trace("beginTransaction (local) idServer.id returned: " + startId.val);
         } catch (IdTmException exc) {
            LOG.error("beginTransaction (local) : IdTm threw exception " + exc);
            throw new IdTmException("beginTransaction (local) : IdTm threw exception " + exc);
         }
         startIdVal = startId.val;
      }
      else {
         if (LOG.isTraceEnabled()) LOG.trace("beginTransaction NOT retrieving new startId");
      }
      if (LOG.isTraceEnabled()) LOG.trace("beginTransaction setting transaction: [" + ts.getTransactionId() +
                      "] with startId: " + startIdVal);
      ts.setStartId(startIdVal);
      return ts;
    }

    /**
     * Prepare to commit a transaction.
     *
     * @param transactionState
     * @return commitStatusCode (see {@link TransactionalRegionInterface})
     * @throws IOException
     * @throws CommitUnsuccessfulException
     */
    public int prepareCommit(final TransactionState transactionState) throws CommitUnsuccessfulException, IOException {
       if (LOG.isTraceEnabled()) LOG.trace("Enter prepareCommit, txid: " + transactionState.getTransactionId());

       if (batchRegionServer && (TRANSACTION_ALGORITHM == AlgorithmType.MVCC)) {
         boolean allReadOnly = true;
         int loopCount = 0;
         if (transactionState.islocalTransaction()){
           if(LOG.isTraceEnabled()) LOG.trace("TransactionManager.prepareCommit local transaction " + transactionState.getTransactionId());
         }
         else
           if(LOG.isTraceEnabled()) LOG.trace("TransactionManager.prepareCommit global transaction " + transactionState.getTransactionId());
         // (need one CompletionService per request for thread safety, can share pool of threads
         CompletionService<Integer> compPool = new ExecutorCompletionService<Integer>(threadPool);

         try {
            ServerName servername;
            List<TransactionRegionLocation> regionList;
            Map<ServerName, List<TransactionRegionLocation>> locations = new HashMap<ServerName, List<TransactionRegionLocation>>();
            for (TransactionRegionLocation location : transactionState.getParticipatingRegions()) {
                servername = location.getServerName();
                if(!locations.containsKey(servername)) {
                    regionList = new ArrayList<TransactionRegionLocation>();
                    locations.put(servername, regionList);
                }
                else {
                    regionList = locations.get(servername);
                }
                regionList.add(location);
            }

            for(final Map.Entry<ServerName, List<TransactionRegionLocation>> entry : locations.entrySet()) {
                loopCount++;
                compPool.submit(new TransactionManagerCallable(transactionState, entry.getValue().iterator().next(), connection) {
                    public Integer call() throws CommitUnsuccessfulException, IOException {
                        return doPrepareX(entry.getValue(), transactionState.getTransactionId());
                    }
                });
            }
          } catch (Exception e) {
             throw new CommitUnsuccessfulException(e);
          }

          // loop to retrieve replies
          int commitError = 0;
          try {
            for (int loopIndex = 0; loopIndex < loopCount; loopIndex ++) {
              Integer canCommit = compPool.take().get();
              switch (canCommit) {
                 case TM_COMMIT_TRUE:
                   allReadOnly = false;
                   break;
                 case TM_COMMIT_READ_ONLY:
                   break;
                 case TM_COMMIT_FALSE_CONFLICT:
                   commitError = TransactionalReturn.COMMIT_CONFLICT;
                   break;
                 case TM_COMMIT_FALSE:
                   // Commit conflict takes precedence
                   if(commitError != TransactionalReturn.COMMIT_CONFLICT)
                      commitError = TransactionalReturn.COMMIT_UNSUCCESSFUL;
                   break;
                 default:
                   LOG.error("Unexpected value of canCommit in prepareCommit (during completion processing): " + canCommit);
                   commitError = TransactionalReturn.COMMIT_UNSUCCESSFUL;;
              }
            }
            loopCount = 0;
            if(transactionState.getRegionsRetryCount() > 0) {
                for (TransactionRegionLocation location : transactionState.getRetryRegions()) {
                    loopCount++;
                    compPool.submit(new TransactionManagerCallable(transactionState, location, connection) {
                        public Integer call() throws CommitUnsuccessfulException, IOException {

                            return doPrepareX(location.getRegionInfo().getRegionName(),
                                    transactionState.getTransactionId(),
                                    location);
                        }
                    });
                }
                transactionState.clearRetryRegions();
            }
                for (int loopIndex = 0; loopIndex < loopCount; loopIndex ++) {
                  Integer canCommit = compPool.take().get();
                    switch (canCommit) {
                       case TM_COMMIT_TRUE:
                         allReadOnly = false;
                         break;
                       case TM_COMMIT_READ_ONLY:
                         break;
                       case TM_COMMIT_FALSE_CONFLICT:
                         commitError = TransactionalReturn.COMMIT_CONFLICT;
                         break;
                       case TM_COMMIT_FALSE:
                         // Commit conflict takes precedence
                         if(commitError != TransactionalReturn.COMMIT_CONFLICT)
                            commitError = TransactionalReturn.COMMIT_UNSUCCESSFUL;
                         break;
                       default:
                         commitError = TransactionalReturn.COMMIT_UNSUCCESSFUL;;
                    }
                }
          }catch (Exception e) {
            throw new CommitUnsuccessfulException(e);
          }
          if(commitError != 0)
             return commitError;

          return allReadOnly ? TransactionalReturn.COMMIT_OK_READ_ONLY:
                               TransactionalReturn.COMMIT_OK;
       }
       else {
       boolean allReadOnly = true;
       int loopCount = 0;
       ServerName servername;
       List<TransactionRegionLocation> regionList;
       Map<ServerName, List<TransactionRegionLocation>> locations = null;

       if (transactionState.islocalTransaction()){
         //System.out.println("prepare islocal");
         if(LOG.isTraceEnabled()) LOG.trace("TransactionManager.prepareCommit local transaction " + transactionState.getTransactionId());
       }
       else
         if(LOG.isTraceEnabled()) LOG.trace("TransactionManager.prepareCommit global transaction " + transactionState.getTransactionId());

       // (need one CompletionService per request for thread safety, can share pool of threads
       CompletionService<Integer> compPool = new ExecutorCompletionService<Integer>(threadPool);
       try {
          if(batchRSMetricsFlag)
             locations = new HashMap<ServerName, List<TransactionRegionLocation>>();

          for (TransactionRegionLocation location : transactionState.getParticipatingRegions()) {
             if(batchRSMetricsFlag)  {
                 servername = location.getServerName();
                 if(!locations.containsKey(servername)) {
                     regionList = new ArrayList<TransactionRegionLocation>();
                     locations.put(servername, regionList);
                 }
                 else {
                     regionList = locations.get(servername);
                 }
                 regionList.add(location);
             }


             loopCount++;
             final TransactionRegionLocation myLocation = location;
             final byte[] regionName = location.getRegionInfo().getRegionName();

             compPool.submit(new TransactionManagerCallable(transactionState, location, connection) {
               public Integer call() throws IOException, CommitUnsuccessfulException {
                 return doPrepareX(regionName, transactionState.getTransactionId(), myLocation);
               }
             });
           }

           if(batchRSMetricsFlag)  {
               this.regions += transactionState.getParticipatingRegions().size();
               this.regionServers += locations.size();
               String rsToRegion = locations.size() + " RS / " + transactionState.getParticipatingRegions().size() + " Regions";
               if(batchRSMetrics.get(rsToRegion) == null) {
                   batchRSMetrics.put(rsToRegion, 1L);
               }
               else {
                   batchRSMetrics.put(rsToRegion, batchRSMetrics.get(rsToRegion) + 1);
               }
               if (metricsCount >= 10000) {
                  metricsCount = 0;
                  if(LOG.isInfoEnabled()) LOG.info("---------------------- BatchRS metrics ----------------------");
                  if(LOG.isInfoEnabled()) LOG.info("Number of total Region calls: " + this.regions);
                  if(LOG.isInfoEnabled()) LOG.info("Number of total RegionServer calls: " + this.regionServers);
                  if(LOG.isInfoEnabled()) LOG.info("---------------- Total number of calls by ratio: ------------");
                  for(Map.Entry<String, Long> entry : batchRSMetrics.entrySet()) {
                      if(LOG.isInfoEnabled()) LOG.info(entry.getKey() + ": " + entry.getValue());
                  }
                  if(LOG.isInfoEnabled()) LOG.info("-------------------------------------------------------------");
               }
               metricsCount++;
           }

        } catch (Exception e) {
           LOG.error("exception in prepareCommit (during submit to pool): " + e);
           throw new CommitUnsuccessfulException(e);
        }

        // loop to retrieve replies
        int commitError = 0;
        try {
          for (int loopIndex = 0; loopIndex < loopCount; loopIndex ++) {
            int canCommit = compPool.take().get();
            switch (canCommit) {
               case TM_COMMIT_TRUE:
                 allReadOnly = false;
                 break;
               case TM_COMMIT_READ_ONLY:
                 break;
               case TM_COMMIT_FALSE_CONFLICT:
                 commitError = TransactionalReturn.COMMIT_CONFLICT;
                 break;
               case TM_COMMIT_FALSE:
                 // Commit conflict takes precedence
                 if(commitError != TransactionalReturn.COMMIT_CONFLICT)
                    commitError = TransactionalReturn.COMMIT_UNSUCCESSFUL;
                 break;
               default:
                 LOG.error("Unexpected value of canCommit in prepareCommit (during completion processing): " + canCommit);
                 commitError = TransactionalReturn.COMMIT_UNSUCCESSFUL;;
            }
          }
        }catch (Exception e) {
          LOG.error("exception in prepareCommit (during completion processing): " + e);
          throw new CommitUnsuccessfulException(e);
        }
        if(commitError != 0)
           return commitError;

        //Before replying prepare success, check for DDL transaction.
        //If prepare already has errors (commitError != 0), an abort is automatically
        //triggered by TM which would take care of ddl abort.
        //if prepare is success upto this point, DDL operation needs to check if any
        //drop table requests were recorded as part of phase 0. If any drop table
        //requests is recorded, then those tables need to disabled as part of prepare.
        if(transactionState.hasDDLTx())
        {
            //if tables were created, then nothing else needs to be done.
            //if tables were recorded dropped, then they need to be disabled.
            //Disabled tables will ultimately be deleted in commit phase.
            ArrayList<String> createList = new ArrayList<String>(); //This list is ignored.
            ArrayList<String> dropList = new ArrayList<String>();
            ArrayList<String> truncateList = new ArrayList<String>();
            StringBuilder state = new StringBuilder ();
            try {
                tmDDL.getRow(transactionState.getTransactionId(), state, createList, dropList, truncateList);
            }
            catch(Exception e){
                LOG.error("exception in doPrepare getRow: " + e);
                if(LOG.isTraceEnabled()) LOG.trace("exception in doPrepare getRow: txID: " + transactionState.getTransactionId());
                state.append("INVALID"); //to avoid processing further down this path.
                commitError = TransactionalReturn.COMMIT_UNSUCCESSFUL;
            }

            //Return if error at this point.
            if(commitError != 0)
                return commitError;

            if(state.toString().equals("VALID") && dropList.size() > 0)
            {
                Iterator<String> di = dropList.iterator();
                while (di.hasNext())
                {
                try {
                        //physical drop of table from hbase.
                        disableTable(transactionState, di.next());
                    }
                    catch(Exception e){
                        if(LOG.isTraceEnabled()) LOG.trace("exception in doPrepare disableTable: txID: " + transactionState.getTransactionId());
                        LOG.error("exception in doCommit, Step : DeleteTable: " + e);

                        //Any error at this point should be considered prepareCommit as unsuccessful.
                        //Retry logic can be added only if it is retryable error: TODO.
                        commitError = TransactionalReturn.COMMIT_UNSUCCESSFUL;
                        break;
                    }
                }
            }
        }

        if(commitError != 0)
           return commitError;

        return allReadOnly ? TransactionalReturn.COMMIT_OK_READ_ONLY:
                             TransactionalReturn.COMMIT_OK;
      }
    }

    /**
     * Try and commit a transaction. This does both phases of the 2-phase protocol: prepare and commit.
     *
     * @param transactionState
     * @throws IOException
     * @throws CommitUnsuccessfulException
     */
    public void tryCommit(final TransactionState transactionState)
        throws CommitUnsuccessfulException, UnsuccessfulDDLException, IOException {
        long startTime = EnvironmentEdgeManager.currentTimeMillis();
        if (LOG.isTraceEnabled()) LOG.trace("Attempting to commit transaction: " + transactionState.toString());
        int status = prepareCommit(transactionState);

        if (status == TransactionalReturn.COMMIT_OK) {
          if (LOG.isTraceEnabled()) LOG.trace("doCommit txid:" + transactionState.getTransactionId());

          doCommit(transactionState);
        } else if (status == TransactionalReturn.COMMIT_OK_READ_ONLY) {
            // no requests sent for fully read only transaction
          transactionState.completeSendInvoke(0);
        } else if (status == TransactionalReturn.COMMIT_UNSUCCESSFUL) {
          // We have already aborted at this point
          throw new CommitUnsuccessfulException();
        }
        if (LOG.isTraceEnabled()) LOG.trace("Committed transaction [" + transactionState.getTransactionId() + "] in ["
                + ((EnvironmentEdgeManager.currentTimeMillis() - startTime)) + "]ms");
    }

    public void retryCommit(final TransactionState transactionState, final boolean ignoreUnknownTransactionException) {
      if(LOG.isTraceEnabled()) LOG.trace("retryCommit -- ENTRY -- txid: " + transactionState.getTransactionId());
      synchronized(transactionState.getRetryRegions()) {
          List<TransactionRegionLocation> completedList = new ArrayList<TransactionRegionLocation>();
          final long commitIdVal = (TRANSACTION_ALGORITHM == AlgorithmType.SSCC) ? transactionState.getCommitId() : -1;
          for (TransactionRegionLocation location : transactionState.getRetryRegions()) {
            if(LOG.isTraceEnabled()) LOG.trace("retryAbort retrying abort for: " + location.getRegionInfo().getRegionNameAsString());
            threadPool.submit(new TransactionManagerCallable(transactionState, location, connection) {
                public Integer call() throws CommitUnsuccessfulException, IOException {

                    return doCommitX(location.getRegionInfo().getRegionName(),
                            transactionState.getTransactionId(), commitIdVal,
                            ignoreUnknownTransactionException);
                }
              });
              completedList.add(location);
            }
            transactionState.getRetryRegions().removeAll(completedList);
        }
      if(LOG.isTraceEnabled()) LOG.trace("retryCommit -- EXIT -- txid: " + transactionState.getTransactionId());
    }

    public void retryAbort(final TransactionState transactionState) {
      if(LOG.isTraceEnabled()) LOG.trace("retryAbort -- ENTRY -- txid: " + transactionState.getTransactionId());
      synchronized(transactionState.getRetryRegions()) {
          List<TransactionRegionLocation> completedList = new ArrayList<TransactionRegionLocation>();
          for (TransactionRegionLocation location : transactionState.getRetryRegions()) {
            if(LOG.isTraceEnabled()) LOG.trace("retryAbort retrying abort for: " + location.getRegionInfo().getRegionNameAsString());
              threadPool.submit(new TransactionManagerCallable(transactionState, location, connection) {
                  public Integer call() throws CommitUnsuccessfulException, IOException {

                      return doAbortX(location.getRegionInfo().getRegionName(),
                              transactionState.getTransactionId());
                  }
              });
              completedList.add(location);
          }
          transactionState.getRetryRegions().removeAll(completedList);
      }
      if(LOG.isTraceEnabled()) LOG.trace("retryAbort -- EXIT -- txid: " + transactionState.getTransactionId());
    }
    /**
     * Do the commit. This is the 2nd phase of the 2-phase protocol.
     *
     * @param transactionState
     * @throws CommitUnsuccessfulException
     */
    public void doCommit(final TransactionState transactionState)
        throws CommitUnsuccessfulException, UnsuccessfulDDLException {
       if (LOG.isTraceEnabled()) LOG.trace("doCommit [" + transactionState.getTransactionId() +
                      "] ignoreUnknownTransactionException not supplied");
       doCommit(transactionState, false);
    }

    /**
     * Do the commit. This is the 2nd phase of the 2-phase protocol.
     *
     * @param transactionState
     * @param ignoreUnknownTransactionException
     * @throws CommitUnsuccessfulException
     */
    public void doCommit(final TransactionState transactionState, final boolean ignoreUnknownTransactionException)
                    throws CommitUnsuccessfulException, UnsuccessfulDDLException {
        int loopCount = 0;
        if (batchRegionServer && (TRANSACTION_ALGORITHM == AlgorithmType.MVCC)) {
          try {
        if (LOG.isTraceEnabled()) LOG.trace("Committing [" + transactionState.getTransactionId() +
                      "] ignoreUnknownTransactionException: " + ignoreUnknownTransactionException);
             // Set the commitId
             transactionState.setCommitId(-1); // Dummy for MVCC

             ServerName servername;
             List<TransactionRegionLocation> regionList;
             Map<ServerName, List<TransactionRegionLocation>> locations = new HashMap<ServerName, List<TransactionRegionLocation>>();
             for (TransactionRegionLocation location : transactionState.getParticipatingRegions()) {
                if (transactionState.getRegionsToIgnore().contains(location)) {
                   continue;
                }
                servername = location.getServerName();

                if(!locations.containsKey(servername)) {
                    regionList = new ArrayList<TransactionRegionLocation>();
                    locations.put(servername, regionList);
                }
                else {
                    regionList = locations.get(servername);
           }
                regionList.add(location);
        }

             for(final Map.Entry<ServerName, List<TransactionRegionLocation>> entry : locations.entrySet()) {
                 if (LOG.isTraceEnabled()) LOG.trace("sending commits ... [" + transactionState.getTransactionId() + "]");
                 loopCount++;

                 threadPool.submit(new TransactionManagerCallable(transactionState, entry.getValue().iterator().next(), connection) {
                     public Integer call() throws CommitUnsuccessfulException, IOException {
                        if (LOG.isTraceEnabled()) LOG.trace("before doCommit() [" + transactionState.getTransactionId() + "]" +
                                                            " ignoreUnknownTransactionException: " + ignoreUnknownTransactionException);
                        return doCommitX(entry.getValue(), transactionState.getTransactionId(),
                                      transactionState.getCommitId(), ignoreUnknownTransactionException);
                     }
                  });
             }

          } catch (Exception e) {
            LOG.error("exception in doCommit for transaction: " + transactionState.getTransactionId() + " "  + e);
              // This happens on a NSRE that is triggered by a split
  /*            try {
                  abort(transactionState);
              } catch (Exception abortException) {

                  LOG.warn("Exeption during abort", abortException);
              }
  */
            throw new CommitUnsuccessfulException(e);
          }

          // all requests sent at this point, can record the count
          transactionState.completeSendInvoke(loopCount);
          /*
        try {
            Thread.sleep(500);
          } catch(Exception e) {}
          */
      }
      else {
          // non batch-rs

        if (LOG.isTraceEnabled()) LOG.trace("Committing [" + transactionState.getTransactionId() +
                      "] ignoreUnknownTransactionException: " + ignoreUnknownTransactionException);

        if (LOG.isTraceEnabled()) LOG.trace("sending commits for ts: " + transactionState);
        try {
           int participants = transactionState.participatingRegions.size() - transactionState.regionsToIgnore.size();
           if (LOG.isTraceEnabled()) LOG.trace("Committing [" + transactionState.getTransactionId() + "] with " + participants + " participants" );
           // (Asynchronously send commit
           for (TransactionRegionLocation location : transactionState.getParticipatingRegions()) {
              if (LOG.isTraceEnabled()) LOG.trace("sending commits ... [" + transactionState.getTransactionId() + "]");
              if (transactionState.getRegionsToIgnore().contains(location)) {
                 continue;
              }

              loopCount++;
              final byte[] regionName = location.getRegionInfo().getRegionName();

              //TransactionalRegionInterface transactionalRegionServer = (TransactionalRegionInterface) connection
              //      .getHRegionConnection(location.getServerName());

              threadPool.submit(new TransactionManagerCallable(transactionState, location, connection) {
                 public Integer call() throws CommitUnsuccessfulException, IOException {
                    if (LOG.isTraceEnabled()) LOG.trace("before doCommit() [" + transactionState.getTransactionId() + "]" +
                                                        " ignoreUnknownTransactionException: " + ignoreUnknownTransactionException);
                    return doCommitX(regionName, transactionState.getTransactionId(), transactionState.getCommitId(), ignoreUnknownTransactionException);
                 }
              });
           }
        } catch (Exception e) {
          LOG.error("exception in doCommit for transaction: " + transactionState.getTransactionId() + " "  + e);
            // This happens on a NSRE that is triggered by a split
/*            try {
                abort(transactionState);
            } catch (Exception abortException) {

                LOG.warn("Exeption during abort", abortException);
            }
*/
          throw new CommitUnsuccessfulException(e);
        }

        // all requests sent at this point, can record the count
        transactionState.completeSendInvoke(loopCount);
        /*
        try {
          Thread.sleep(500);
        } catch(Exception e) {}
         */
      }

        //if DDL is involved with this transaction, need to complete it.
        if(transactionState.hasDDLTx())
        {
            //First wait for commit requests sent to all regions is received back.
            //This TM thread gets SUSPENDED until all commit threads complete!!!
            try{
                transactionState.completeRequest();
            }
            catch(Exception e){
                LOG.error("Exception in doCommit completeRequest. txID: " + transactionState.getTransactionId() + "Exception: " + e);
                //return; //Do not return here. This thread should continue servicing DDL operations.
            }

            try{
                doCommitDDL(transactionState);

            } catch (Exception e) {
                LOG.error("FATAL Exception calling doCommitDDL for transaction: " + transactionState.getTransactionId() + "Exception: "  + e);
                throw new UnsuccessfulDDLException(e);
            }
        }
    }

    public void doCommitDDL(final TransactionState transactionState) throws UnsuccessfulDDLException
    {

        //if tables were created, then nothing else needs to be done.
        //if tables were recorded dropped, then they need to be physically dropped.
        //Tables recorded dropped would already be disabled as part of prepare commit.
        ArrayList<String> createList = new ArrayList<String>(); //This list is ignored.
        ArrayList<String> dropList = new ArrayList<String>();
        ArrayList<String> truncateList = new ArrayList<String>(); //This list is ignored.
        StringBuilder state = new StringBuilder ();
        boolean retry = true;
        int retryCount = 0;
        int retrySleep = TM_SLEEP;
        do
        {
            try {
                tmDDL.getRow(transactionState.getTransactionId(), state, createList, dropList, truncateList);
                retry = false;
            }
            catch(Exception e){
                LOG.error("Exception in doCommitDDL, Step: getRow. txID: " + transactionState.getTransactionId() + "Exception: " + e);

                if(retryCount == RETRY_ATTEMPTS)
                {
                    LOG.error("Fatal Exception in doCommitDDL, Step: getRow. Raising CommitUnsuccessfulException txID: " + transactionState.getTransactionId() + "Exception: " + e);

                   //if tmDDL is unreachable at this point, it is fatal.
                    throw new UnsuccessfulDDLException(e);
                }

                retryCount++;
                if (retryCount < RETRY_ATTEMPTS)
                {
                    try {
                        Thread.sleep(retrySleep);
                    } catch(InterruptedException ex) {
                        Thread.currentThread().interrupt();
                    }
                    retrySleep += TM_SLEEP_INCR;
                }
            }
        } while (retryCount < RETRY_ATTEMPTS && retry == true);

        if(state.toString().equals("VALID") && dropList.size() > 0)
        {
            Iterator<String> di = dropList.iterator();
            while (di.hasNext())
            {
                retryCount = 0;
                retrySleep = TM_SLEEP;
                retry = true;
                String tblName = di.next();
                do
                {
                    try {
                        //physical drop of table from hbase.
                        deleteTable(transactionState, tblName);
                        retry = false;
                    }
                    catch(TableNotFoundException t){
                        //Check for TableNotFoundException, if that is the case, no further
                        //processing needed. This is not an error. Possible we are retrying the entire set of DDL changes
                        //because this transaction was pinned for some reason.
                        if(LOG.isTraceEnabled()) LOG.trace(" TableNotFoundException exception in doCommitDDL deleteTable, Continuing: txID: " + transactionState.getTransactionId());
                        retry = false;
                    }
                    catch(Exception e){
                        LOG.error("Fatal exception in doCommitDDL, Step : DeleteTable: TxID:" + transactionState.getTransactionId() + "Exception: " + e);

                        if(retryCount == RETRY_ATTEMPTS)
                        {
                            LOG.error("Fatal Exception in doCommitDDL, Step: DeleteTable. Raising CommitUnsuccessfulException TxID:" + transactionState.getTransactionId() );

                            //Throw this exception after all retry attempts.
                            //Throwing a new exception gets out of the loop.
                            throw new UnsuccessfulDDLException(e);
                        }

                        retryCount++;
                        if (retryCount < RETRY_ATTEMPTS)
                        {
                            try {
                                Thread.sleep(retrySleep);
                            } catch(InterruptedException ex) {
                                Thread.currentThread().interrupt();
                            }
                            retrySleep += TM_SLEEP_INCR;
                        }
                    }
                }while (retryCount < RETRY_ATTEMPTS && retry == true);
            }//while
    }

    //update TDDL post operation, delete the transaction from TmDDL.
    retryCount = 0;
    retrySleep = TM_SLEEP;
    retry = true;
    do
    {
        try{
            tmDDL.deleteRow(transactionState.getTransactionId());
            retry = false;
        }
        catch(Exception e)
        {
            LOG.error("Fatal Exception in doCommitDDL, Step: deleteRow. txID: " + transactionState.getTransactionId() + "Exception: " + e);

            if(retryCount == RETRY_ATTEMPTS)
            {
                LOG.error("Fatal Exception in doCommitDDL, Step: deleteRow. Raising CommitUnsuccessfulException. txID: " + transactionState.getTransactionId());

                //Throw this exception after all retry attempts.
                //Throwing a new exception gets out of the loop.
                throw new UnsuccessfulDDLException(e);
            }

            retryCount++;
            if (retryCount < RETRY_ATTEMPTS)
            {
                try {
                    Thread.sleep(retrySleep);
                } catch(InterruptedException ex) {
                    Thread.currentThread().interrupt();
                }
                retrySleep += TM_SLEEP_INCR;
            }
        }
    }while (retryCount < RETRY_ATTEMPTS && retry == true);
}


    /**
     * Abort a s transaction.
     *
     * @param transactionState
     * @throws IOException
     */
    public void abort(final TransactionState transactionState) throws IOException, UnsuccessfulDDLException {
      if(LOG.isTraceEnabled()) LOG.trace("Abort -- ENTRY txID: " + transactionState.getTransactionId());
        int loopCount = 0;
      /*
      if(transactionState.getStatus().equals("ABORTED")) {
          if(LOG.isTraceEnabled()) LOG.trace("Abort --EXIT already called, ignoring");
          return;
      }
      */

      transactionState.setStatus(TransState.STATE_ABORTED);
      // (Asynchronously send aborts
      if (batchRegionServer && (TRANSACTION_ALGORITHM == AlgorithmType.MVCC)) {
        ServerName servername;
        List<TransactionRegionLocation> regionList;
        Map<ServerName, List<TransactionRegionLocation>> locations = new HashMap<ServerName, List<TransactionRegionLocation>>();

        for (TransactionRegionLocation location : transactionState.getParticipatingRegions()) {
            if (transactionState.getRegionsToIgnore().contains(location)) {
                continue;
            }
            servername = location.getServerName();

            if(!locations.containsKey(servername)) {
                regionList = new ArrayList<TransactionRegionLocation>();
                locations.put(servername, regionList);
            }
            else {
                regionList = locations.get(servername);
            }
            regionList.add(location);
        }
        for(final Map.Entry<ServerName, List<TransactionRegionLocation>> entry : locations.entrySet()) {
            loopCount++;
            threadPool.submit(new TransactionManagerCallable(transactionState, entry.getValue().iterator().next(), connection) {
                public Integer call() throws IOException {
                   if (LOG.isTraceEnabled()) LOG.trace("before abort() [" + transactionState.getTransactionId() + "]");

                   return doAbortX(entry.getValue(), transactionState.getTransactionId());
                }
             });
        }
        transactionState.completeSendInvoke(loopCount);
        /*
        if(transactionState.getRegionsRetryCount() > 0) {
            for (TransactionRegionLocation location : transactionState.getRetryRegions()) {
                loopCount++;
                threadPool.submit(new TransactionManagerCallable(transactionState, location, connection) {
                    public Integer call() throws CommitUnsuccessfulException, IOException {

                        return doAbortX(location.getRegionInfo().getRegionName(),
                                transactionState.getTransactionId());
                    }
                });
            }
        }
        transactionState.clearRetryRegions();
        */
    }
    else {

      for (TransactionRegionLocation location : transactionState.getParticipatingRegions()) {
          if (transactionState.getRegionsToIgnore().contains(location)) {
              continue;
          }
          try {
            loopCount++;
            final byte[] regionName = location.getRegionInfo().getRegionName();

            threadPool.submit(new TransactionManagerCallable(transactionState, location, connection) {
              public Integer call() throws IOException {
                return doAbortX(regionName, transactionState.getTransactionId());
              }
            });
          } catch (Exception e) {
            LOG.error("exception in abort: " + e);
          }
            /*
            } catch (UnknownTransactionException e) {
        LOG.error("exception in abort: " + e);
                LOG.info("Got unknown transaction exception during abort. Transaction: ["
                        + transactionState.getTransactionId() + "], region: ["
                        + location.getRegionInfo().getRegionNameAsString() + "]. Ignoring.");
            } catch (NotServingRegionException e) {
                LOG.info("Got NSRE during abort. Transaction: [" + transactionState.getTransactionId() + "], region: ["
                        + location.getRegionInfo().getRegionNameAsString() + "]. Ignoring.");
            }
            */
        }

        // all requests sent at this point, can record the count
        transactionState.completeSendInvoke(loopCount);
    }

        //if DDL is involved with this transaction, need to unwind it.
        if(transactionState.hasDDLTx())
        {

            //First wait for abort requests sent to all regions is received back.
            //This TM thread gets SUSPENDED until all abort threads complete!!!
            try{
                transactionState.completeRequest();
            }
            catch(Exception e){
                LOG.error("Exception in abort() completeRequest. txID: " + transactionState.getTransactionId() + "Exception: " + e);
                //return; //Do not return here. This thread should continue servicing DDL operations.
            }

            try{
                abortDDL(transactionState);
            }
            catch(Exception e){
                LOG.error("FATAL Exception calling abortDDL for transaction: " + transactionState.getTransactionId() + "Exception: "  + e);
                throw new UnsuccessfulDDLException(e);
            }
        }

        if(LOG.isTraceEnabled()) LOG.trace("Abort -- EXIT txID: " + transactionState.getTransactionId());

    }

    void abortDDL(final TransactionState transactionState) throws UnsuccessfulDDLException
    {
        //if tables were created, then they need to be dropped.
        ArrayList<String> createList = new ArrayList<String>();
        ArrayList<String> dropList = new ArrayList<String>();
        ArrayList<String> truncateList = new ArrayList<String>();
        StringBuilder state = new StringBuilder ();
        boolean retry = true;
        int retryCount = 0;
        int retrySleep = TM_SLEEP;

        do
        {
            try {
                tmDDL.getRow(transactionState.getTransactionId(), state, createList, dropList, truncateList);
                retry = false;
            }
            catch(Exception e){
                LOG.error("Fatal Exception in abortDDL, Step: getRow. txID: " + transactionState.getTransactionId() + "Exception: " + e);

                if(retryCount == RETRY_ATTEMPTS)
                {
                    LOG.error("Fatal Exception in abortDDL, Step: getRow. Raising UnsuccessfulDDLException txID: " + transactionState.getTransactionId() + "Exception: " + e);

                   //if tmDDL is unreachable at this point, it is fatal.
                    throw new UnsuccessfulDDLException(e);
                }

                retryCount++;
                if (retryCount < RETRY_ATTEMPTS)
                {
                    try {
                        Thread.sleep(retrySleep);
                    } catch(InterruptedException ex) {
                        Thread.currentThread().interrupt();
                    }
                    retrySleep += TM_SLEEP_INCR;
                }
            }
        }while (retryCount < RETRY_ATTEMPTS && retry == true);

        // if tables were recorded to be truncated on an upsert using load,
        // then they will be truncated on an abort transaction
        if(state.toString().equals("VALID") && truncateList.size() > 0)
        {
            if(LOG.isTraceEnabled()) LOG.trace("truncateList -- ENTRY txID: " + transactionState.getTransactionId());

            Iterator<String> ci = truncateList.iterator();
            while (ci.hasNext())
            {
                retryCount = 0;
                retrySleep = TM_SLEEP;
                retry = true;
                String tblName = ci.next();
                do
                {
                    try {
                        truncateTable(transactionState, tblName);
                        retry = false;
                    }
                    catch(Exception e){
                        LOG.error("Fatal exception in abortDDL, Step : truncateTable: TxID:" + transactionState.getTransactionId() + "Exception: " + e);

                        if(retryCount == RETRY_ATTEMPTS)
                        {
                            LOG.error("Fatal Exception in abortDDL, Step: truncateTable. Raising UnsuccessfulDDLException TxID:" + transactionState.getTransactionId() );

                            //Throw this exception after all retry attempts.
                            //Throwing a new exception gets out of the loop.
                            throw new UnsuccessfulDDLException(e);
                        }

                        retryCount++;
                        if (retryCount < RETRY_ATTEMPTS)
                        {
                            try {
                                Thread.sleep(retrySleep);
                            } catch(InterruptedException ex) {
                                Thread.currentThread().interrupt();
                            }
                            retrySleep += TM_SLEEP_INCR;
                        }
                    }
                }while(retryCount < RETRY_ATTEMPTS && retry == true);
            }//while
        }

        if(state.toString().equals("VALID") && createList.size() > 0)
        {
            Iterator<String> ci = createList.iterator();
            while (ci.hasNext())
            {
                retryCount = 0;
                retrySleep = TM_SLEEP;
                retry = true;
                String tblName = ci.next();
                do
                {
                    try {
                        deleteTable(transactionState, tblName);
                        retry = false;
                    }
                    catch(TableNotFoundException t){
                        //Check for TableNotFoundException, if that is the case, no further
                        //processing needed. This is not an error. Possible we are retrying the entire set of DDL changes
                        //because this transaction is being redriven for some reason.
                        if(LOG.isTraceEnabled()) LOG.trace(" TableNotFoundException exception in abortDDL deleteTable, Continuing: txID: " + transactionState.getTransactionId());
                        retry = false;
                    }
                    catch(Exception e){
                        LOG.error("Fatal exception in abortDDL, Step : DeleteTable: TxID:" + transactionState.getTransactionId() + "Exception: " + e);

                        if(retryCount == RETRY_ATTEMPTS)
                        {
                            LOG.error("Fatal Exception in abortDDL, Step: DeleteTable. Raising UnsuccessfulDDLException TxID:" + transactionState.getTransactionId() );

                            //Throw this exception after all retry attempts.
                            //Throwing a new exception gets out of the loop.
                            throw new UnsuccessfulDDLException(e);
                        }

                        retryCount++;
                        if (retryCount < RETRY_ATTEMPTS)
                        {
                            try {
                                Thread.sleep(retrySleep);
                            } catch(InterruptedException ex) {
                                Thread.currentThread().interrupt();
                            }
                            retrySleep += TM_SLEEP_INCR;
                        }

                    }
                }while(retryCount < RETRY_ATTEMPTS && retry == true);
            }//while
        }

        //if tables were recorded dropped, then they need to be reinstated,
        //depending on the state of the transaction. The table recorded as dropped in phase 0,
        //will be disabled as part of prepareCommit and physically dropped as part of doCommit.
        if(state.toString().equals("VALID") && dropList.size() > 0)
        {
            Iterator<String> di = dropList.iterator();
            while (di.hasNext())
            {
                retryCount = 0;
                retrySleep = TM_SLEEP;
                retry = true;
                String tblName = di.next();
                do
                {
                    try {
                           enableTable(transactionState, tblName);
                           retry = false;
                    }
                    catch(TableNotDisabledException t){
                        //Check for TableNotDisabledException, if that is the case, no further
                        //processing needed. This is not an error. Possible we are retrying the entire set of DDL changes
                        //because this transaction is being redriven for some reason.
                        if(LOG.isTraceEnabled()) LOG.trace(" TableNotDisabledException exception in abortDDL enableTable, Continuing: txID: " + transactionState.getTransactionId());
                        retry = false;
                    }
                    catch(Exception e){
                        LOG.error("Fatal exception in abortDDL, Step : enableTable: TxID:" + transactionState.getTransactionId() + "Exception: " + e);
                        if(retryCount == RETRY_ATTEMPTS)
                        {
                            LOG.error("Fatal Exception in doCommitDDL, Step: DeleteTable. Raising UnsuccessfulDDLException TxID:" + transactionState.getTransactionId() );

                            //Throw this exception after all retry attempts.
                            //Throwing a new exception gets out of the loop.
                            throw new UnsuccessfulDDLException(e);
                        }

                        retryCount++;
                        if (retryCount < RETRY_ATTEMPTS)
                        {
                            try {
                                Thread.sleep(retrySleep);
                            } catch(InterruptedException ex) {
                                Thread.currentThread().interrupt();
                            }
                            retrySleep += TM_SLEEP_INCR;
                        }
                    }
                }while(retryCount < RETRY_ATTEMPTS && retry == true);
            }//while
        }

        //update TDDL post operation, delete the transaction from TmDDL
        retryCount = 0;
        retrySleep = TM_SLEEP;
        retry = true;
        do
        {
            try{
                tmDDL.deleteRow(transactionState.getTransactionId());
                retry = false;
            }
            catch(Exception e)
            {
                LOG.error("Fatal Exception in abortDDL, Step: deleteRow. txID: " + transactionState.getTransactionId() + "Exception: " + e);

                if(retryCount == RETRY_ATTEMPTS)
                {
                    LOG.error("Fatal Exception in abortDDL, Step: deleteRow. Raising UnsuccessfulDDLException. txID: " + transactionState.getTransactionId());

                    //Throw this exception after all retry attempts.
                    //Throwing a new exception gets out of the loop.
                    throw new UnsuccessfulDDLException(e);
                }

                retryCount++;
                if (retryCount < RETRY_ATTEMPTS)
                {
                    try {
                        Thread.sleep(retrySleep);
                    } catch(InterruptedException ex) {
                        Thread.currentThread().interrupt();
                    }
                    retrySleep += TM_SLEEP_INCR;
                }

            }
        }while(retryCount < RETRY_ATTEMPTS && retry == true);
    }

    public synchronized JtaXAResource getXAResource() {
        if (xAResource == null) {
            xAResource = new JtaXAResource(this);
        }
        return xAResource;
    }

    public void registerRegion(final TransactionState transactionState, TransactionRegionLocation location)throws IOException{
        if (LOG.isTraceEnabled()) LOG.trace("registerRegion ENTRY, transactioState:" + transactionState);
        if(transactionState.addRegion(location)){
           if (LOG.isTraceEnabled()) LOG.trace("registerRegion -- added region: " + location.getRegionInfo().getRegionNameAsString() + " with endKey: "
                      + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " to tx " + transactionState.getTransactionId());
        }
        else {
           if (LOG.isTraceEnabled()) LOG.trace("registerRegion -- region previously added: " + location.getRegionInfo().getRegionNameAsString() + " with endKey: "
                      + Hex.encodeHexString(location.getRegionInfo().getEndKey()));
        }
        if (LOG.isTraceEnabled()) LOG.trace("registerRegion EXIT");
    }

    public void createTable(final TransactionState transactionState, HTableDescriptor desc, Object[]  beginEndKeys)
            throws Exception{
        if (LOG.isTraceEnabled()) LOG.trace("createTable ENTRY, transactionState: " + transactionState.getTransactionId());

        try {
            if (beginEndKeys != null && beginEndKeys.length > 0) {
               byte[][] keys = new byte[beginEndKeys.length][];
               for (int i = 0; i < beginEndKeys.length; i++){
                  keys[i] = (byte[])beginEndKeys[i];
                  if (LOG.isTraceEnabled()) LOG.trace("createTable with key #" + i + "value" + keys[i] + ") called.");
               }
               hbadmin.createTable(desc, keys);
            }
            else {
            hbadmin.createTable(desc);
            }
            hbadmin.close();

            // Set transaction state object as participating in ddl transaction
            transactionState.setDDLTx(true);

            //record this create in TmDDL.
            tmDDL.putRow( transactionState.getTransactionId(), "CREATE", desc.getNameAsString());
        }
        catch (Exception e) {
            LOG.error("createTable Exception TxId: " + transactionState.getTransactionId() + "Exception: " + e);
            throw e;
        }

    }

    private class ChangeFlags {
        boolean tableDescriptorChanged;
        boolean columnDescriptorChanged;

        ChangeFlags() {
           tableDescriptorChanged = false;
           columnDescriptorChanged = false;
        }

        void setTableDescriptorChanged() {
           tableDescriptorChanged = true;
        }

        void setColumnDescriptorChanged() {
           columnDescriptorChanged = true;
       }

       boolean tableDescriptorChanged() {
          return tableDescriptorChanged;
       }

       boolean columnDescriptorChanged() {
          return columnDescriptorChanged;
       }
    }

   private ChangeFlags setDescriptors(Object[] tableOptions,
                                      HTableDescriptor desc,
                                      HColumnDescriptor colDesc,
                                      int defaultVersionsValue) {
       ChangeFlags returnStatus = new ChangeFlags();
       String trueStr = "TRUE";
       for (int i = 0; i < tableOptions.length; i++) {
           if (i == HBASE_NAME)
               continue ;
           String tableOption = (String)tableOptions[i];
           if ((i != HBASE_MAX_VERSIONS) && (tableOption.isEmpty()))
               continue ;
           switch (i) {
           case HBASE_MAX_VERSIONS:
               if (tableOption.isEmpty()) {
                   if (colDesc.getMaxVersions() != defaultVersionsValue) {
                       colDesc.setMaxVersions(defaultVersionsValue);
                       returnStatus.setColumnDescriptorChanged();
                   }
               }
               else {
                   colDesc.setMaxVersions
                       (Integer.parseInt(tableOption));
                   returnStatus.setColumnDescriptorChanged();
               }
               break ;
           case HBASE_MIN_VERSIONS:
               colDesc.setMinVersions
                   (Integer.parseInt(tableOption));
               returnStatus.setColumnDescriptorChanged();
               break ;
           case HBASE_TTL:
               colDesc.setTimeToLive
                   (Integer.parseInt(tableOption));
               returnStatus.setColumnDescriptorChanged();
               break ;
           case HBASE_BLOCKCACHE:
               if (tableOption.equalsIgnoreCase(trueStr))
                   colDesc.setBlockCacheEnabled(true);
               else
                   colDesc.setBlockCacheEnabled(false);
               returnStatus.setColumnDescriptorChanged();
               break ;
           case HBASE_IN_MEMORY:
               if (tableOption.equalsIgnoreCase(trueStr))
                   colDesc.setInMemory(true);
               else
                   colDesc.setInMemory(false);
               returnStatus.setColumnDescriptorChanged();
               break ;
           case HBASE_COMPRESSION:
               if (tableOption.equalsIgnoreCase("GZ"))
                   colDesc.setCompressionType(Algorithm.GZ);
               else if (tableOption.equalsIgnoreCase("LZ4"))
                   colDesc.setCompressionType(Algorithm.LZ4);
               else if (tableOption.equalsIgnoreCase("LZO"))
                   colDesc.setCompressionType(Algorithm.LZO);
               else if (tableOption.equalsIgnoreCase("NONE"))
                   colDesc.setCompressionType(Algorithm.NONE);
               else if (tableOption.equalsIgnoreCase("SNAPPY"))
                   colDesc.setCompressionType(Algorithm.SNAPPY);
               returnStatus.setColumnDescriptorChanged();
               break ;
           case HBASE_BLOOMFILTER:
               if (tableOption.equalsIgnoreCase("NONE"))
                   colDesc.setBloomFilterType(BloomType.NONE);
               else if (tableOption.equalsIgnoreCase("ROW"))
                   colDesc.setBloomFilterType(BloomType.ROW);
               else if (tableOption.equalsIgnoreCase("ROWCOL"))
                   colDesc.setBloomFilterType(BloomType.ROWCOL);
               returnStatus.setColumnDescriptorChanged();
               break ;
           case HBASE_BLOCKSIZE:
               colDesc.setBlocksize
                   (Integer.parseInt(tableOption));
               returnStatus.setColumnDescriptorChanged();
               break ;
           case HBASE_DATA_BLOCK_ENCODING:
               if (tableOption.equalsIgnoreCase("DIFF"))
                   colDesc.setDataBlockEncoding(DataBlockEncoding.DIFF);
               else if (tableOption.equalsIgnoreCase("FAST_DIFF"))
                   colDesc.setDataBlockEncoding(DataBlockEncoding.FAST_DIFF);
               else if (tableOption.equalsIgnoreCase("NONE"))
                   colDesc.setDataBlockEncoding(DataBlockEncoding.NONE);
               else if (tableOption.equalsIgnoreCase("PREFIX"))
                   colDesc.setDataBlockEncoding(DataBlockEncoding.PREFIX);
               else if (tableOption.equalsIgnoreCase("PREFIX_TREE"))
                   colDesc.setDataBlockEncoding(DataBlockEncoding.PREFIX_TREE);
               returnStatus.setColumnDescriptorChanged();
               break ;
           case HBASE_CACHE_BLOOMS_ON_WRITE:
               if (tableOption.equalsIgnoreCase(trueStr))
                   colDesc.setCacheBloomsOnWrite(true);
               else
                   colDesc.setCacheBloomsOnWrite(false);
               returnStatus.setColumnDescriptorChanged();
               break ;
           case HBASE_CACHE_DATA_ON_WRITE:
               if (tableOption.equalsIgnoreCase(trueStr))
                   colDesc.setCacheDataOnWrite(true);
               else
                   colDesc.setCacheDataOnWrite(false);
               returnStatus.setColumnDescriptorChanged();
               break ;
           case HBASE_CACHE_INDEXES_ON_WRITE:
               if (tableOption.equalsIgnoreCase(trueStr))
                   colDesc.setCacheIndexesOnWrite(true);
               else
                   colDesc.setCacheIndexesOnWrite(false);
               returnStatus.setColumnDescriptorChanged();
               break ;
           case HBASE_COMPACT_COMPRESSION:
               if (tableOption.equalsIgnoreCase("GZ"))
                   colDesc.setCompactionCompressionType(Algorithm.GZ);
               else if (tableOption.equalsIgnoreCase("LZ4"))
                   colDesc.setCompactionCompressionType(Algorithm.LZ4);
               else if (tableOption.equalsIgnoreCase("LZO"))
                   colDesc.setCompactionCompressionType(Algorithm.LZO);
               else if (tableOption.equalsIgnoreCase("NONE"))
                   colDesc.setCompactionCompressionType(Algorithm.NONE);
               else if (tableOption.equalsIgnoreCase("SNAPPY"))
                   colDesc.setCompactionCompressionType(Algorithm.SNAPPY);
               returnStatus.setColumnDescriptorChanged();
               break ;
           case HBASE_PREFIX_LENGTH_KEY:
               desc.setValue(KeyPrefixRegionSplitPolicy.PREFIX_LENGTH_KEY,
                             tableOption);
               returnStatus.setTableDescriptorChanged();
               break ;
           case HBASE_EVICT_BLOCKS_ON_CLOSE:
               if (tableOption.equalsIgnoreCase(trueStr))
                   colDesc.setEvictBlocksOnClose(true);
               else
                   colDesc.setEvictBlocksOnClose(false);
               returnStatus.setColumnDescriptorChanged();
               break ;
           case HBASE_KEEP_DELETED_CELLS:
               if (tableOption.equalsIgnoreCase(trueStr))
                   colDesc.setKeepDeletedCells(true);
               else
                   colDesc.setKeepDeletedCells(false);
               returnStatus.setColumnDescriptorChanged();
               break ;
           case HBASE_REPLICATION_SCOPE:
               colDesc.setScope
                   (Integer.parseInt(tableOption));
               returnStatus.setColumnDescriptorChanged();
               break ;
           case HBASE_MAX_FILESIZE:
               desc.setMaxFileSize
                   (Long.parseLong(tableOption));
               returnStatus.setTableDescriptorChanged();
               break ;
           case HBASE_COMPACT:
              if (tableOption.equalsIgnoreCase(trueStr))
                   desc.setCompactionEnabled(true);
               else
                   desc.setCompactionEnabled(false);
               returnStatus.setTableDescriptorChanged();
               break ;
           case HBASE_DURABILITY:
               if (tableOption.equalsIgnoreCase("ASYNC_WAL"))
                   desc.setDurability(Durability.ASYNC_WAL);
               else if (tableOption.equalsIgnoreCase("FSYNC_WAL"))
                   desc.setDurability(Durability.FSYNC_WAL);
               else if (tableOption.equalsIgnoreCase("SKIP_WAL"))
                   desc.setDurability(Durability.SKIP_WAL);
               else if (tableOption.equalsIgnoreCase("SYNC_WAL"))
                   desc.setDurability(Durability.SYNC_WAL);
               else if (tableOption.equalsIgnoreCase("USE_DEFAULT"))
                   desc.setDurability(Durability.USE_DEFAULT);
               returnStatus.setTableDescriptorChanged();
               break ;
           case HBASE_MEMSTORE_FLUSH_SIZE:
               desc.setMemStoreFlushSize
                   (Long.parseLong(tableOption));
               returnStatus.setTableDescriptorChanged();
               break ;
           case HBASE_SPLIT_POLICY:
                  // This method not yet available in earlier versions
                  // desc.setRegionSplitPolicyClassName(tableOption)); 
               desc.setValue(desc.SPLIT_POLICY, tableOption);
               returnStatus.setTableDescriptorChanged();
               break ;
           default:
               break;
           }
       }

       return returnStatus;
   }

    private void waitForCompletion(String tblName,HBaseAdmin admin)
       throws IOException {
       // poll for completion of an asynchronous operation
       boolean keepPolling = true;
       while (keepPolling) {
          // status.getFirst() returns the number of regions yet to be updated
          // status.getSecond() returns the total number of regions
          Pair<Integer,Integer> status = admin.getAlterStatus(tblName.getBytes());

          keepPolling = (status.getFirst() > 0) && (status.getSecond() > 0);
          if (keepPolling) {
          try {
             Thread.sleep(2000); // sleep two seconds or until interrupted
             }
             catch (InterruptedException e) {
                // ignore the interruption and keep going
             }
          }  
       }
    }       

 
    public void alterTable(final TransactionState transactionState, String tblName, Object[]  tableOptions)
           throws Exception {
        if (LOG.isTraceEnabled()) LOG.trace("createTable ENTRY, transactionState: " + transactionState.getTransactionId());
        
        try {
           HTableDescriptor htblDesc = hbadmin.getTableDescriptor(tblName.getBytes());
           HColumnDescriptor[] families = htblDesc.getColumnFamilies();
           HColumnDescriptor colDesc = families[0];  // Trafodion keeps SQL columns only in first column family
           int defaultVersionsValue = colDesc.getMaxVersions();

           ChangeFlags status =
              setDescriptors(tableOptions,htblDesc /*out*/,colDesc /*out*/, defaultVersionsValue);
           
           if (status.tableDescriptorChanged()) {
              hbadmin.modifyTable(tblName,htblDesc);
              waitForCompletion(tblName,hbadmin);
           }
           else if (status.columnDescriptorChanged()) {
              hbadmin.modifyColumn(tblName,colDesc);
              waitForCompletion(tblName,hbadmin);
           }
           hbadmin.close();

           // Set transaction state object as participating in ddl transaction
           transactionState.setDDLTx(true);

           //record this create in TmDDL.
           tmDDL.putRow( transactionState.getTransactionId(), "ALTER", tblName);

        }
        catch (Exception e) {
            LOG.error("createTable Exception TxId: " + transactionState.getTransactionId() + "Exception: " + e);
            throw e;
        }
    }

    public void registerTruncateOnAbort(final TransactionState transactionState, String tblName)
            throws Exception {
        if (LOG.isTraceEnabled()) LOG.trace("registerTruncateOnAbort ENTRY, TxID " + transactionState.getTransactionId() +
            " tableName: " + tblName);

        // register the truncate on abort to TmDDL
        try {
            // add truncate record to TmDDL
            tmDDL.putRow(transactionState.getTransactionId(), "TRUNCATE", tblName);

            // Set transaction state object as participating in ddl transaction.
            transactionState.setDDLTx(true);
        }
        catch (Exception e) {
            LOG.error("registerTruncateOnAbort Exception Txid:" + transactionState.getTransactionId() +"TableName: " + tblName + "Exception:" + e);
            throw e;
        }
    }

    public void dropTable(final TransactionState transactionState, String tblName)
            throws Exception{
        if (LOG.isTraceEnabled()) LOG.trace("dropTable ENTRY, TxId: " + transactionState.getTransactionId() + "TableName: " + tblName);

        //Record this drop table request in TmDDL.
        //Note that physical disable of this table happens in prepare phase.
        //Followed by physical drop of this table in commit phase.
        try {
            // add drop record to TmDDL.
            tmDDL.putRow( transactionState.getTransactionId(), "DROP", tblName);

            // Set transaction state object as participating in ddl transaction.
            transactionState.setDDLTx(true);
        }
        catch (Exception e) {
            LOG.error("dropTable Exception TxId: " + transactionState.getTransactionId() + "TableName:" + tblName + "Exception: " + e);
            throw e;
        }
    }

    //Called only by Abort or Commit processing.
    public void deleteTable(final TransactionState transactionState, final String tblName)
            throws Exception{
        if (LOG.isTraceEnabled()) LOG.trace("deleteTable ENTRY, TxId: " + transactionState.getTransactionId() + "tableName" + tblName);
        try{
            disableTable(transactionState, tblName);
        }
        catch (TableNotEnabledException e) {
            //If table is not enabled, no need to throw exception. Continue.
            if (LOG.isTraceEnabled()) LOG.trace("deleteTable , TableNotEnabledException, Step: disableTable, TxId: " +
                transactionState.getTransactionId() + "TableName" + tblName + "Exception: " + e);
        }
        catch (Exception e) {
            LOG.error("deleteTable Exception TxId: " + transactionState.getTransactionId() + "TableName" + tblName + "Exception: " + e);
            throw e;
        }

        try{
            hbadmin.deleteTable(tblName);
        }
        catch (Exception e) {
            LOG.error("deleteTable Exception TxId: " + transactionState.getTransactionId() + "TableName" + tblName  + "Exception: " + e);
            throw e;
        }
    }

    //Called only by Abort processing.
    public void enableTable(final TransactionState transactionState, String tblName)
            throws Exception{
        if (LOG.isTraceEnabled()) LOG.trace("enableTable ENTRY, TxID: " + transactionState.getTransactionId() + "tableName" + tblName);
        try {
            hbadmin.enableTable(tblName);
        }
        catch (Exception e) {
            LOG.error("enableTable Exception TxId: " + transactionState.getTransactionId() + "TableName" + tblName + "Exception: " + e);
            throw e;
        }
    }

    // Called only by Abort processing to delete data from a table
    public void truncateTable(final TransactionState transactionState, String tblName)
            throws Exception{
        if (LOG.isTraceEnabled()) LOG.trace("truncateTable ENTRY, TxID: " + transactionState.getTransactionId() +
            "table: " + tblName);

        try {
            TableName tablename = TableName.valueOf(tblName);
            HTableDescriptor hdesc = hbadmin.getTableDescriptor(tablename);

            // To be changed in 2.0 for truncate table
            //hbadmin.truncateTable(tablename, true);
            hbadmin.disableTable(tblName);
            hbadmin.deleteTable(tblName);
            hbadmin.createTable(hdesc);
            hbadmin.close();
        }
        catch (Exception e) {
            LOG.error("truncateTable Exception TxId: " + transactionState.getTransactionId() + "TableName" + tblName+  "Exception: " + e);
            throw e;
        }
    }

    //Called only by DoPrepare.
    public void disableTable(final TransactionState transactionState, String tblName)
            throws Exception{
        if (LOG.isTraceEnabled()) LOG.trace("disableTable ENTRY, TxID: " + transactionState.getTransactionId() + "tableName" + tblName);
        try {
            hbadmin.disableTable(tblName);
        }
        catch (Exception e) {
            LOG.error("disableTable Exception TxId: " + transactionState.getTransactionId() + "TableName" + tblName + "Exception: " + e);
            throw e;
        }
    }

    /**
     * @param hostnamePort
     * @param regionArray
     * @param tmid
     * @return
     * @throws Exception
     */
    public List<Long> recoveryRequest (String hostnamePort, byte[] regionArray, int tmid) throws Exception{
        if (LOG.isTraceEnabled()) LOG.trace("recoveryRequest -- ENTRY TM" + tmid);
        HRegionInfo regionInfo = null;
        HTable table = null;

        /*
         * hostname and port no longer needed for RPC
        final byte [] delimiter = ",".getBytes();
        String[] result = hostnamePort.split(new String(delimiter), 3);

        if (result.length < 2)
                throw new IllegalArgumentException("Region array format is incorrect");

        String hostname = result[0];
        int port = Integer.parseInt(result[1]);
        LOG.debug("recoveryRequest regionInfo -- hostname:" + hostname + " port:" + port);
        */

        /*
         *  New way of parsing HRegionInfo used instead
        ByteArrayInputStream lv_bis = new ByteArrayInputStream(regionArray);
        DataInputStream lv_dis = new DataInputStream(lv_bis);
        try {
                regionInfo.readFields(lv_dis);
        } catch (Exception e) {
                StringWriter sw = new StringWriter();
                PrintWriter pw = new PrintWriter(sw);
                e.printStackTrace(pw);
                LOG.error("recoveryRequest exception in regionInfo.readFields() " + sw.toString());
                throw new Exception();
        }
        */

        try {
            regionInfo = HRegionInfo.parseFrom(regionArray);
        }
        catch (Exception de) {
           if (LOG.isTraceEnabled()) LOG.trace("TransactionManager:recoveryRequest exception in regionInfo parseFrom, " +
             " TM : " + tmid +
             " DeserializationException: " + de);
            StringWriter sw = new StringWriter();
            PrintWriter pw = new PrintWriter(sw);
            de.printStackTrace(pw);
               LOG.error("DeserializationException in regionInfo parseFrom, unable to complete recoveryRequest\n" + sw.toString());
            throw new DeserializationException("DeserializationException in regionInfo parseFrom, unable to complete recoveryRequest ");
       }

        final String regionName = regionInfo.getRegionNameAsString();
        final int tmID = tmid;
        if (LOG.isTraceEnabled()) LOG.trace("TransactionManager:recoveryRequest regionInfo encoded name: [" + regionInfo.getEncodedName() + "]" + " hostname " + hostnamePort);
        Batch.Call<TrxRegionService, RecoveryRequestResponse> callable =
                new Batch.Call<TrxRegionService, RecoveryRequestResponse>() {
              ServerRpcController controller = new ServerRpcController();
              BlockingRpcCallback<RecoveryRequestResponse> rpcCallback =
                new BlockingRpcCallback<RecoveryRequestResponse>();

              @Override
              public RecoveryRequestResponse call(TrxRegionService instance) throws IOException {
                org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.RecoveryRequestRequest.Builder rbuilder = RecoveryRequestRequest.newBuilder();
                rbuilder.setTransactionId(-1);
                rbuilder.setRegionName(ByteString.copyFromUtf8(regionName));
                rbuilder.setTmId(tmID);

                instance.recoveryRequest(controller, rbuilder.build(), rpcCallback);
                return rpcCallback.get();
              }
            };

            // Working out the begin and end keys
            byte[] startKey = regionInfo.getStartKey();
            byte[] endKey = regionInfo.getEndKey();

            if(endKey != HConstants.EMPTY_END_ROW)
                endKey = TransactionManager.binaryIncrementPos(endKey, -1);

            table = new HTable(regionInfo.getTable(), connection, cp_tpe);

            Map<byte[], RecoveryRequestResponse> rresult = null;
            try {
              rresult = table.coprocessorService(TrxRegionService.class, startKey, endKey, callable);
            }
            catch (Throwable e) {
                LOG.error("Exception thrown when calling coprocessor: " + e.toString());
                e.printStackTrace();
            }

        Collection<RecoveryRequestResponse> results = rresult.values();
        RecoveryRequestResponse[] resultArray = new RecoveryRequestResponse[results.size()];
        results.toArray(resultArray);

        if(resultArray.length == 0) {
            table.close();
            throw new IOException("Problem with calling coprocessor, no regions returned result");
        }

        //return tri.recoveryRequest(regionInfo.getRegionName(), tmid);
        table.close();
        if (LOG.isTraceEnabled()) LOG.trace("recoveryRequest -- EXIT TM" + tmid);

        return resultArray[0].getResultList();
    }
}

