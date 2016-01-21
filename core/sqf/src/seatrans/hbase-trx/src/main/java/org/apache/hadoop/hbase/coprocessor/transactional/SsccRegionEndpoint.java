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

package org.apache.hadoop.hbase.coprocessor.transactional;

import java.io.IOException;

import org.apache.commons.codec.binary.Hex;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hbase.Coprocessor;
import org.apache.hadoop.hbase.CoprocessorEnvironment;
import org.apache.hadoop.hbase.coprocessor.ColumnInterpreter;
import org.apache.hadoop.hbase.coprocessor.CoprocessorException;
import org.apache.hadoop.hbase.coprocessor.CoprocessorService;
import org.apache.hadoop.hbase.coprocessor.RegionCoprocessorEnvironment;

import java.io.*;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.lang.reflect.InvocationTargetException;
import java.lang.StringBuilder;
import java.lang.StringBuilder;
import java.lang.Thread.UncaughtExceptionHandler;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.Map.Entry;
import java.util.NavigableSet;
import java.util.NavigableMap;
import java.util.Set;
import java.util.SortedMap;
import java.util.SortedSet;
import java.util.TreeMap;
import java.util.TreeSet;
import java.util.UUID;
import java.lang.reflect.InvocationTargetException;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;


import org.apache.hadoop.classification.InterfaceAudience;
import org.apache.hadoop.classification.InterfaceStability;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.Cell;
import org.apache.hadoop.hbase.CellUtil;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Durability;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Increment;
import org.apache.hadoop.hbase.client.Mutation;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.ScannerTimeoutException;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.transactional.UnknownTransactionException;

import org.apache.hadoop.classification.InterfaceAudience;
import org.apache.hadoop.classification.InterfaceStability;
import org.apache.hadoop.hbase.Cell;
import org.apache.hadoop.hbase.CellUtil;
import org.apache.hadoop.hbase.filter.FilterBase;
import org.apache.hadoop.hbase.filter.FilterList;
import org.apache.hadoop.hbase.KeyValueUtil;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.KeyValue.Type;
import org.apache.hadoop.hbase.Coprocessor;
import org.apache.hadoop.hbase.CoprocessorEnvironment;
import org.apache.hadoop.hbase.coprocessor.CoprocessorException;
import org.apache.hadoop.hbase.coprocessor.CoprocessorService;
import org.apache.hadoop.hbase.coprocessor.RegionCoprocessorEnvironment;
import org.apache.hadoop.hbase.coprocessor.RegionObserver;
import org.apache.hadoop.hbase.exceptions.OutOfOrderScannerNextException;
import org.apache.hadoop.hbase.filter.FilterBase;
import org.apache.hadoop.hbase.coprocessor.CoprocessorService;
import org.apache.hadoop.hbase.coprocessor.RegionCoprocessorEnvironment;
import org.apache.hadoop.hbase.coprocessor.RegionObserver;
import org.apache.hadoop.hbase.filter.FilterList;
import org.apache.hadoop.hbase.filter.FirstKeyOnlyFilter;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HBaseInterfaceAudience;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.ServerName;
import org.apache.hadoop.hbase.Tag;
import org.apache.hadoop.hbase.client.Mutation;
import org.apache.hadoop.hbase.regionserver.RegionServerServices;
import org.apache.hadoop.hbase.Stoppable;
import org.apache.hadoop.hbase.NotServingRegionException;
import org.apache.hadoop.hbase.UnknownScannerException;
import org.apache.hadoop.hbase.regionserver.HRegion;
import org.apache.hadoop.hbase.regionserver.KeyValueScanner;
import org.apache.hadoop.hbase.regionserver.LeaseException;
import org.apache.hadoop.hbase.regionserver.LeaseListener;
import org.apache.hadoop.hbase.regionserver.Leases;
import org.apache.hadoop.hbase.regionserver.Leases.LeaseStillHeldException;
import org.apache.hadoop.hbase.regionserver.MultiVersionConsistencyControl;
import org.apache.hadoop.hbase.regionserver.RegionCoprocessorHost;
import org.apache.hadoop.hbase.regionserver.RegionScanner;
import org.apache.hadoop.hbase.regionserver.WrongRegionException;
import org.apache.hadoop.hbase.regionserver.wal.HLog;
import org.apache.hadoop.hbase.regionserver.wal.HLogKey;
import org.apache.hadoop.hbase.regionserver.wal.HLogUtil;
import org.apache.hadoop.hbase.regionserver.wal.WALEdit;
import org.apache.hadoop.hbase.regionserver.transactional.CleanOldTransactionsChore;
import org.apache.hadoop.hbase.regionserver.transactional.SsccTransactionState;
import org.apache.hadoop.hbase.regionserver.transactional.IdTm;
import org.apache.hadoop.hbase.regionserver.transactional.IdTmException;
import org.apache.hadoop.hbase.regionserver.transactional.IdTmId;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionalRegion;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionalRegionScannerHolder;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionState.Status;
import org.apache.hadoop.hbase.util.EnvironmentEdgeManager;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.FSUtils;
import org.apache.hadoop.hbase.util.Threads;
import org.apache.hadoop.hbase.protobuf.ProtobufUtil;
import org.apache.hadoop.hbase.protobuf.ResponseConverter;
import org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto;
import org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto.MutationType;
import org.apache.hadoop.hbase.coprocessor.transactional.TrxRegionObserver;

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
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCloseScannerRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCloseScannerResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndDeleteRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndDeleteResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndPutRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndPutResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteMultipleTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteMultipleTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutMultipleTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutMultipleTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccRecoveryRequestRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccRecoveryRequestResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccTransactionalAggregateRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccTransactionalAggregateResponse;

import org.apache.hadoop.hbase.client.DtmConst;
import org.apache.hadoop.hbase.client.SsccConst;
import org.apache.hadoop.hbase.zookeeper.ZooKeeperWatcher;
import org.apache.hadoop.hbase.zookeeper.ZKUtil;
import org.apache.zookeeper.KeeperException;

import com.google.protobuf.ByteString;
import com.google.protobuf.Message;
import com.google.protobuf.RpcCallback;
import com.google.protobuf.RpcController;
import com.google.protobuf.Service;
import com.google.protobuf.ServiceException;

@InterfaceAudience.LimitedPrivate(HBaseInterfaceAudience.COPROC)
@InterfaceStability.Evolving
public class SsccRegionEndpoint<T, S, P extends Message, Q extends Message, R extends Message> extends SsccRegionService implements
CoprocessorService, Coprocessor {

  private static final Log LOG = LogFactory.getLog(SsccRegionEndpoint.class);

  private RegionCoprocessorEnvironment env;

  ConcurrentHashMap<String, Object> transactionsByIdTestz = null;

  // Collection of active transactions (PENDING) keyed by id.
  protected ConcurrentHashMap<String, SsccTransactionState> transactionsById = new ConcurrentHashMap<String, SsccTransactionState>();

  // Map of recent transactions that are COMMIT_PENDING or COMMITED keyed by
  // their sequence number
  private SortedMap<Long, SsccTransactionState> commitedTransactionsBySequenceNumber = Collections.synchronizedSortedMap(new TreeMap<Long, SsccTransactionState>());

  // Collection of transactions that are COMMIT_PENDING
  private Set<SsccTransactionState> commitPendingTransactions = Collections
                        .synchronizedSet(new HashSet<SsccTransactionState>());

  // an in-doubt transaction list during recovery WALEdit replay
  private Map<Long, WALEdit> indoubtTransactionsById = new TreeMap<Long, WALEdit>();

  // an in-doubt transaction list count by TM id
  private Map<Integer, Integer> indoubtTransactionsCountByTmid = new TreeMap<Integer,Integer>();

  // Concurrent map for transactional region scanner holders
  // Protected by synchronized methods
  final ConcurrentHashMap<Long,
                          TransactionalRegionScannerHolder> scanners =
      new ConcurrentHashMap<Long, TransactionalRegionScannerHolder>();

  // Atomic values to manage region scanners
  private AtomicLong performScannerId = new AtomicLong(0);
//  private AtomicInteger nextSequenceId = new AtomicInteger(0);

  private Object commitCheckLock = new Object();
  private Object recoveryCheckLock = new Object();
  private Object editReplay = new Object();
  private Object stoppableLock = new Object();
  private int reconstructIndoubts = 0; 
  //temporary THLog getSequenceNumber() replacement
  private AtomicLong nextLogSequenceId = new AtomicLong(0);
  private final int oldTransactionFlushTrigger = 0;
  private final Boolean splitDelayEnabled = false;
  private final Boolean doWALHlog = false;
  static Leases transactionLeases = null;
//  CleanOldTransactionsChore cleanOldTransactionsThread;
//  CompleteTransactionsChore completeTransactionsThread;
  static Stoppable stoppable = new StoppableImplementation();
  private int cleanTimer = 5000; // Five minutes
  private int regionState = 0; 
  private Path recoveryTrxPath = null;
  private int cleanAT = 0;
  private long[] commitCheckTimes   = new long[50];
  private long[] hasConflictTimes   = new long[50];
  private long[] putBySequenceTimes = new long[50];
  private long[] writeToLogTimes    = new long[50];

  private AtomicInteger  timeIndex               =    new AtomicInteger (0);
  private AtomicInteger  totalCommits            =    new AtomicInteger (0);
  private AtomicInteger  writeToLogOperations    =    new AtomicInteger (0);
  private AtomicInteger  putBySequenceOperations =    new AtomicInteger (0);
  private long   totalCommitCheckTime =    0;
  private long   totalConflictTime    =    0;
  private long   totalPutTime         =    0;
  private long   totalWriteToLogTime  =    0;
  private long   minCommitCheckTime   =    1000000000;
  private long   maxCommitCheckTime   =    0;
  private double avgCommitCheckTime   =    0;
  private long   minConflictTime      =    1000000000;
  private long   maxConflictTime      =    0;
  private double avgConflictTime      =    0;
  private long   minPutTime           =    1000000000;
  private long   maxPutTime           =    0;
  private double avgPutTime           =    0;
  private long   minWriteToLogTime    =    1000000000;
  private long   maxWriteToLogTime    =    0;
  private double avgWriteToLogTime    =    0;

  private HRegionInfo regionInfo = null;
  private HRegion m_Region = null;
  private TransactionalRegion t_Region = null;
  private FileSystem fs = null;
  private RegionCoprocessorHost rch = null;
  private HLog tHLog = null;
  boolean closing = false;
  private boolean fullEditInCommit = true;
  private boolean configuredEarlyLogging = false;
  private static Object zkRecoveryCheckLock = new Object();
  private static ZooKeeperWatcher zkw1 = null;
  String lv_hostName;
  int lv_port;
  private static String zNodePath = "/hbase/Trafodion/recovery/";

  private static final int DEFAULT_LEASE_TIME = 7200 * 1000;
  private static final int LEASE_CHECK_FREQUENCY = 1000;
  private static final String SLEEP_CONF = "hbase.transaction.clean.sleep";
  private static final int DEFAULT_SLEEP = 60 * 1000;
  protected static int transactionLeaseTimeout = 0;
  private static int scannerLeaseTimeoutPeriod = 0;
  private static int scannerThreadWakeFrequency = 0;

  // Transaction state defines
  private static final int COMMIT_OK = 1;
  private static final int COMMIT_OK_READ_ONLY = 2;
  private static final int COMMIT_UNSUCCESSFUL = 3;
  private static final int COMMIT_CONFLICT = 5;

  // update return code defines
  private static final int STATEFUL_UPDATE_OK = 1;
  private static final int STATEFUL_UPDATE_CONFLICT = 2;
  private static final int STATELESS_UPDATE_OK = 3;
  private static final int STATELESS_UPDATE_CONFLICT = 5;

  private static final int CLOSE_WAIT_ON_COMMIT_PENDING = 1000;
  private static final int MAX_COMMIT_PENDING_WAITS = 10;
  private Thread ChoreThread = null;
  //private static Thread ScannerLeasesThread = null;
  private static Thread TransactionalLeasesThread = null;
  private static boolean editGenerated = false;

  private static final int COMMIT_REQUEST = 1;
  private static final int COMMIT = 2;
  private static final int ABORT = 3;
  private static final int CONTROL_POINT_COMMIT = 4;

  private Map<Long, Long> updateTsToStartId = new TreeMap<Long, Long>();
  private AtomicLong nextSsccSequenceId;

  private IdTm idServer;
  private static final int ID_TM_SERVER_TIMEOUT = 1000;
  // SsccRegionService methods
  @Override
  public void beginTransaction(RpcController controller,
                                SsccBeginTransactionRequest request,
      RpcCallback<SsccBeginTransactionResponse> done) {
    SsccBeginTransactionResponse response = SsccBeginTransactionResponse.getDefaultInstance();

    long transId = request.getTransactionId();
    long startId = request.getStartId();

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: beginTransaction - id " + transId + ", regionName " + regionInfo.getRegionNameAsString());

    Throwable t = null;
    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();
    WrongRegionException wre = null;
    try {
      beginTransaction(transId, startId);
    } catch (Throwable e) {
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:beginTransaction threw exception after internal begin");
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
      t = e;
    }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccBeginTransactionResponse.Builder SsccBeginTransactionResponseBuilder = SsccBeginTransactionResponse.newBuilder();

    SsccBeginTransactionResponseBuilder.setHasException(false);

    if (t != null)
    {
      SsccBeginTransactionResponseBuilder.setHasException(true);
      SsccBeginTransactionResponseBuilder.setException(t.toString());
    }

    if (wre != null)
    {
      SsccBeginTransactionResponseBuilder.setHasException(true);
      SsccBeginTransactionResponseBuilder.setException(wre.toString());
    }

    SsccBeginTransactionResponse bresponse = SsccBeginTransactionResponseBuilder.build();

    done.run(bresponse);
  }

  /**
   * Begin a transaction
   * @param longtransactionId
   * @throws IOException
   */

  public void beginTransaction(final long transactionId, final long startId) throws IOException {

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  beginTransaction -- ENTRY txId: " + transactionId + " startId: " + startId);
    checkClosing(transactionId);

    // TBD until integration with recovery
    if (reconstructIndoubts == 0) {
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  RECOV beginTransaction -- ENTRY txId: " + transactionId);
//       try {
          constructIndoubtTransactions();
//       }
//       catch (IdTmException e){
//          LOG.error("SsccRegionEndpoint coprocessor:  RECOV beginTransaction exception" + e);
//          throw new IOException("SsccRegionEndpoint coprocessor:  RECOV beginTransaction " + e);
//       }
//       catch (Exception e2) {
//          LOG.error("SsccRegionEndpoint coprocessor:  RECOV beginTransaction exception" + e2);
//          throw new IOException("SsccRegionEndpoint coprocessor:  RECOV beginTransaction exception " + e2);
//       }
    }
    if (regionState != 2) {
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: Trafodion Recovery: RECOVERY WARN beginTransaction while the region is still in recovering state " +  regionState);
    }

    String key = getTransactionalUniqueId(transactionId);
    SsccTransactionState state;
    synchronized (transactionsById) {
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  beginTransaction -- creating new SsccTransactionState without coprocessorHost txId: " + transactionId);

      state = new SsccTransactionState(transactionId,
                                nextLogSequenceId.getAndIncrement(),
                                nextLogSequenceId,
                                m_Region.getRegionInfo(),
                                m_Region.getTableDesc(), tHLog, configuredEarlyLogging,
                                startId);
//                                nextSsccSequenceId.getAndIncrement());


      state.setStartSequenceNumber(state.getStartId());

//    List<TransactionState> commitPendingCopy =
//        new ArrayList<SsccTransactionState>(commitPendingTransactions);

//    for (SsccTransactionState commitPending : commitPendingCopy) {
//            state.addTransactionToCheck(commitPending);
//    }

      transactionsById.put(key, state);

      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  beginTransaction - Adding transaction: [" + transactionId + "] in region ["
                + m_Region.getRegionInfo().getRegionNameAsString() + "]" +
                 " to list");
    }

    try {
       transactionLeases.createLease(key, transactionLeaseTimeout, new TransactionLeaseListener(transactionId));
    } catch (LeaseStillHeldException e) {
       LOG.error("SsccRegionEndpoint coprocessor:  beginTransaction - Lease still held for [" + transactionId + "] in region ["
                + m_Region.getRegionInfo().getRegionNameAsString() + "]");
       throw new RuntimeException(e);
    }

     if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  beginTransaction -- EXIT txId: " + transactionId
          + " transactionsById size: " + transactionsById.size() + " region ID: " + this.regionInfo.getRegionId());
  }

  /**begin transaction if not yet
    * @param transactionId
    * @return true: begin; false: not necessary to begin
    * @throws IOException
   */
  private SsccTransactionState beginTransIfNotExist(final long transactionId, final long startId) throws IOException{

    if (LOG.isTraceEnabled()) LOG.trace("Enter SsccRegionEndpoint coprocessor: beginTransIfNotExist, txid: "
              + transactionId + " startId: " + startId + " transactionsById size: "
              + transactionsById.size());

    String key = getTransactionalUniqueId(transactionId);
    synchronized (transactionsById) {
      SsccTransactionState state = transactionsById.get(key);
      if (state == null) {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Begin transaction in beginTransIfNotExist beginning the transaction internally as state was null");
        this.beginTransaction(transactionId, startId);
        state =  transactionsById.get(key);
      }
      return state;
    }
  }

  /**
   * Gets the transaction state
   * @param long transactionId
   * @return SsccTransactionState
   * @throws UnknownTransactionException
   */
  protected SsccTransactionState getTransactionState(final long transactionId)
   throws UnknownTransactionException {
    SsccTransactionState state = null;
    boolean throwUTE = false;

    String key = getTransactionalUniqueId(transactionId);
    state = transactionsById.get(key);
    if (state == null)
    {
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getTransactionState Unknown transaction: [" + transactionId + "], throwing UnknownTransactionException");
        throwUTE = true;
    }
    else {
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getTransactionState Found transaction: [" + transactionId + "]");

      try {
         transactionLeases.renewLease(key);
      } catch (LeaseException e) {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getTransactionState renewLease failed will try to createLease for transaction: [" + transactionId + "]");
        try {
           transactionLeases.createLease(key, transactionLeaseTimeout, new TransactionLeaseListener(transactionId));
        } catch (LeaseStillHeldException lshe) {
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getTransactionState renewLeasefollowed by createLease failed throwing original LeaseException for transaction: [" + transactionId + "]");
          throw new RuntimeException(e);
        }
      }
    }

    if (throwUTE)
    {
      throw new UnknownTransactionException();
    }

    return state;
  }

  @Override
  public void commit(RpcController controller,
                     SsccCommitRequest request,
      RpcCallback<SsccCommitResponse> done) {
    SsccCommitResponse response = SsccCommitResponse.getDefaultInstance();
    long transId = request.getTransactionId();
    long commitId = request.getCommitId();

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: commit - id " + transId + ", regionName " + regionInfo.getRegionNameAsString());

    Throwable t = null;
    WrongRegionException wre = null;

    {
     // Process local memory
      try {
        commit(transId, commitId, request.getIgnoreUnknownTransactionException());
      } catch (Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:commit threw exception after internal commit");
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
        t = e;
      }
    }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitResponse.Builder commitResponseBuilder = SsccCommitResponse.newBuilder();

    commitResponseBuilder.setHasException(false);

    if (t != null)
    {
      commitResponseBuilder.setHasException(true);
      commitResponseBuilder.setException(t.toString());
    }

    if (wre != null)
    {
      commitResponseBuilder.setHasException(true);
      commitResponseBuilder.setException(wre.toString());
    }

    SsccCommitResponse cresponse = commitResponseBuilder.build();

    done.run(cresponse);
  }

  @Override
  public void commitIfPossible(RpcController controller,
                                SsccCommitIfPossibleRequest request,
      RpcCallback<SsccCommitIfPossibleResponse> done) {
    SsccCommitIfPossibleResponse response = SsccCommitIfPossibleResponse.getDefaultInstance();

     boolean reply = false;
     Throwable t = null;
    WrongRegionException wre = null;
    long transId = request.getTransactionId();

     {
       // Process local memory
       try {
         if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: commitIfPossible - id " + transId + ", regionName, " + regionInfo.getRegionNameAsString() + "calling internal commitIfPossible");
         reply = commitIfPossible(transId);
       } catch (Throwable e) {
         if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:commitIfPossible threw exception after internal commitIfPossible");
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
          t = e;
       }
     }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitIfPossibleResponse.Builder commitIfPossibleResponseBuilder = SsccCommitIfPossibleResponse.newBuilder();

    commitIfPossibleResponseBuilder.setHasException(false);

    if (t != null)
    {
      commitIfPossibleResponseBuilder.setHasException(true);
      commitIfPossibleResponseBuilder.setException(t.toString());
    }

    if (wre != null)
    {
      commitIfPossibleResponseBuilder.setHasException(true);
      commitIfPossibleResponseBuilder.setException(wre.toString());
    }

    commitIfPossibleResponseBuilder.setWasSuccessful(reply);
    SsccCommitIfPossibleResponse cresponse = commitIfPossibleResponseBuilder.build();
    done.run(cresponse);
  }


  @Override
  public void commitRequest(RpcController controller,
                            SsccCommitRequestRequest request,
                            RpcCallback<SsccCommitRequestResponse> done) {

    SsccCommitRequestResponse response = SsccCommitRequestResponse.getDefaultInstance();
    long transId = request.getTransactionId();

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: commitRequest - id " + transId +
                      ", regionName " + regionInfo.getRegionNameAsString());

    int status = 0;
    IOException ioe = null;
    UnknownTransactionException ute = null;
    Throwable t = null;
    WrongRegionException wre = null;

    {
      // Process local memory
      try {
        status = commitRequest(transId);
      } catch (UnknownTransactionException u) {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:commitRequest threw exception after internal commit" + u.toString());
        ute = u;
      } catch (IOException e) {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:commitRequest threw exception after internal commit" + e.toString());
        ioe = e;
      }
    }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCommitRequestResponse.Builder commitRequestResponseBuilder = SsccCommitRequestResponse.newBuilder();

    commitRequestResponseBuilder.setHasException(false);

    if (t != null)
    {
      commitRequestResponseBuilder.setHasException(true);
      commitRequestResponseBuilder.setException(t.toString());
    }

    if (wre != null)
    {
      commitRequestResponseBuilder.setHasException(true);
      commitRequestResponseBuilder.setException(wre.toString());
    }

    if (ioe != null)
    {
      commitRequestResponseBuilder.setHasException(true);
      commitRequestResponseBuilder.setException(ioe.toString());
    }

    if (ute != null)
    {
      commitRequestResponseBuilder.setHasException(true);
      commitRequestResponseBuilder.setException(ute.toString());
    }

    commitRequestResponseBuilder.setResult(status);

    SsccCommitRequestResponse cresponse = commitRequestResponseBuilder.build();
    done.run(cresponse);
  }

  
  /**
   * Commits the transaction
   * @param SsccTransactionState state
   * @throws IOException
   */
  private void commit(final SsccTransactionState state) throws IOException {
    long txid = 0;

    //if state is in ABORTED status, return do nothing
    if(state.getStatus() == Status.ABORTED || state.getStatus() == Status.COMMITED) return;

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Commiting transaction: " + state.toString() + " to "
     + m_Region.getRegionInfo().getRegionNameAsString());
    long inTransactionId = state.getTransactionId();
    long startId = state.getStartId();

    if (state.isReinstated()) {
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  commit Trafodion Recovery: commit reinstated indoubt transactions " + inTransactionId + 
                              " in region " + m_Region.getRegionInfo().getRegionNameAsString());
      if (false) //Somthing wrong
      {
              state.setStatus(Status.ABORTED);
              retireTransaction(state);
      }
    }  // reinstated transactions
    else {
       //get a commid ID  
       //  This commitId must be comparable with the startId
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: commit : try to update the status and version ");

//       long commitId = nextSsccSequenceId.getAndIncrement();
       long commitId = state.getCommitId();

       //update the putlist
       List<byte[]> putToDoList = state.getPutRows();
       List<Delete> delToDoList = state.getDelRows();
       //List<Mutation> mutList= new ArrayList<Mutation>();
       try {
         for( byte[] rowkey : putToDoList )
         {
           // delete the status item from status column for this transactin
           Delete statusDelete = new Delete(rowkey, startId);
           statusDelete.deleteColumn(DtmConst.TRANSACTION_META_FAMILY , SsccConst.STATUS_COL );
           //statusDelete.deleteColumn(DtmConst.TRANSACTION_META_FAMILY , SsccConst.COLUMNS_COL );
           m_Region.delete(statusDelete);
           //mutList.add(statusDelete);

           // insert a new item into version column
//           Put verPut = new Put(rowkey, commitId.val);
           Put verPut = new Put(rowkey, commitId);
           verPut.add(DtmConst.TRANSACTION_META_FAMILY , SsccConst.VERSION_COL,SsccConst.generateVersionValue(startId,false));
           m_Region.put(verPut);
           //mutList.add(verPut);
         }
         ListIterator<Delete> deletesIter = null;
         for (deletesIter = delToDoList.listIterator(); deletesIter.hasNext();) {
           Delete d = deletesIter.next();

           //mutList.add(d);
           // insert a new item into version column
           // delete the status item from status column for this transactin
           byte[] dKey = d.getRow();
           Delete statusDelete = new Delete(dKey, startId);
           statusDelete.deleteColumn(DtmConst.TRANSACTION_META_FAMILY , SsccConst.STATUS_COL );
           //statusDelete.deleteColumn(DtmConst.TRANSACTION_META_FAMILY , SsccConst.COLUMNS_COL );
           m_Region.delete(statusDelete);

//           Put verPut = new Put(dKey, commitId.val);
           Put verPut = new Put(dKey, commitId);
           verPut.add(DtmConst.TRANSACTION_META_FAMILY , SsccConst.VERSION_COL,SsccConst.generateVersionValue(startId,true));
           m_Region.put(verPut);
//           m_Region.delete(d);
         }
         //DO a batch mutation
         //Mutation[] m = (Mutation[])mutList.toArray();
         //m_Region.batchMutate(m);

         //set the commitId of the transaction
//         state.setCommitId(commitId.val);
//         state.setCommitId(commitId);
       }
       catch (Exception e) //something wrong
       {
            LOG.error("SsccRegionEndpoint Commit get exception " +  e.toString());
           state.setStatus(Status.ABORTED);
           retireTransaction(state);
           throw new IOException(e.toString());
       }
    } // normal transactions

    // Now the transactional writes live in the core WAL, we can write a commit to the log
    // so we don't have to recover it from the transactional WAL.
    if (state.hasWrite() || state.isReinstated()) {
       // comment out for now
      if (LOG.isTraceEnabled()) LOG.trace("write commit edit to HLOG");
      if (LOG.isTraceEnabled()) LOG.trace("BBB write commit edit to HLOG after appendNoSync");
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:commit -- EXIT txId: " + inTransactionId + " HLog seq " + txid);
      if (!editGenerated) editGenerated = true;
    }

    state.setStatus(Status.COMMITED);
/*
    if (state.hasWrite() || state.isReinstated()) {
      synchronized (commitPendingTransactions) {
      if (!commitPendingTransactions.remove(state)) {
          LOG.fatal("SsccRegionEndpoint coprocessor:  commit Commiting a non-query transaction that is not in commitPendingTransactions");
          // synchronized statements are cleared for a throw
        throw new IOException("commit failure");
      }
    }
    }
*/

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  commit(tstate) -- EXIT SsccTransactionState: " + 
      state.toString());

    if (state.isReinstated()) {
      synchronized(indoubtTransactionsById) {
        indoubtTransactionsById.remove(state.getTransactionId());
        int tmid = (int) (inTransactionId >> 32);
        int count = 0;
        if (indoubtTransactionsCountByTmid.containsKey(tmid)) {
          count =  (int) indoubtTransactionsCountByTmid.get(tmid) - 1;
          if (count > 0) indoubtTransactionsCountByTmid.put(tmid, count);
        }
        if (count == 0) {
          indoubtTransactionsCountByTmid.remove(tmid);
          String lv_encoded = m_Region.getRegionInfo().getEncodedName();
            try {
              if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: commit Trafodion Recovery: delete in commit recovery zNode TM " + tmid + " region encoded name " + lv_encoded + " for 0 in-doubt transaction");
              deleteRecoveryzNode(tmid, lv_encoded);
            } catch (IOException e) {
            LOG.error("Trafodion Recovery: delete recovery zNode failed");
            }
        }

        if ((indoubtTransactionsById == null) || (indoubtTransactionsById.size() == 0)) {
          if (indoubtTransactionsById == null) 
            if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  commit Trafodion Recovery: start region in commit with indoubtTransactionsById null");
          else
            if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  commit Trafodion Recovery: start region in commit with indoubtTransactionsById size " + indoubtTransactionsById.size());
          startRegionAfterRecovery();
        }
      }
    }
    retireTransaction(state);
  }

 /**
   * Commits the transaction
   * @param long TransactionId
   * @param boolean ignoreUnknownTransactionException
   * @throws IOException 
   */
  public void commit(final long transactionId, final long commitId, final boolean ignoreUnknownTransactionException) throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: commit -- ENTRY txId: " + transactionId +
              " commitId: " + commitId + " ignoreUnknownTransactionException: " + ignoreUnknownTransactionException);
    SsccTransactionState state=null;
    try {
      state = getTransactionState(transactionId);
    } catch (UnknownTransactionException e) {
      if (ignoreUnknownTransactionException == true) {
         if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  ignoring UnknownTransactionException in commit : " + transactionId
                + " in region "
                + m_Region.getRegionInfo().getRegionNameAsString());
         return;
      }
      LOG.fatal("SsccRegionEndpoint coprocessor:  Asked to commit unknown transaction: " + transactionId
                + " in region "
                + m_Region.getRegionInfo().getRegionNameAsString());
      throw new IOException("UnknownTransactionException");
    }
/* SSCC don't need a prepare phase I
    if (!state.getStatus().equals(Status.COMMIT_PENDING)) {
      LOG.fatal("SsccRegionEndpoint coprocessor: commit - Asked to commit a non pending transaction ");

      throw new IOException("Asked to commit a non-pending transaction");
    }
*/
   state.setCommitId(commitId);

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: commit -- EXIT " + state);
    commit(state);
  } 

  /**
   * @param transactionId
   * @return TransactionRegionInterface commit code
   * @throws IOException
   */
  public int commitRequest(final long transactionId) throws IOException {
    long txid = 0;

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: commitRequest -- ENTRY txId: " + transactionId);
    checkClosing(transactionId);
    SsccTransactionState state=null;
    try{
       state = getTransactionState(transactionId);
    } catch (UnknownTransactionException e) {
        return COMMIT_UNSUCCESSFUL;
    }

    if (state.hasWrite()) {
       return COMMIT_OK;
    }
    // Otherwise we were read-only and commitable, so we can forget it.
    state.setStatus(Status.COMMITED);
    retireTransaction(state);
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: commitRequest READ ONLY -- EXIT txId: " + transactionId);
    return COMMIT_OK_READ_ONLY;
  }

  /**
   * Commits the transaction
   * @param long TransactionId
   * @throws IOException
   */
  public void commit(final long transactionId, final long commitId) throws IOException {
     commit(transactionId, commitId, false /* IgnoreUnknownTransactionException */);
  }

  @Override
  public void abortTransaction(RpcController controller,
                                SsccAbortTransactionRequest request,
      RpcCallback<SsccAbortTransactionResponse> done) {
    SsccAbortTransactionResponse response = SsccAbortTransactionResponse.getDefaultInstance();
    long transId = request.getTransactionId();

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: abortTransaction - id " + transId + ", regionName " + regionInfo.getRegionNameAsString());

    IOException ioe = null;
    UnknownTransactionException ute = null;
    WrongRegionException wre = null;
    Throwable t = null;

    {
      // Process in local memory
      try {
        abortTransaction(transId);
      } catch (UnknownTransactionException u) {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:abort threw UnknownTransactionException after internal abort");
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Caught exception " + u.getMessage() + "" + stackTraceToString(u));
       ute = u;
      } catch (IOException e) {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:abort threw UnknownTransactionException after internal abort");
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
        ioe = e;
      }
    }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccAbortTransactionResponse.Builder abortTransactionResponseBuilder = SsccAbortTransactionResponse.newBuilder();

    abortTransactionResponseBuilder.setHasException(false);

    if (t != null)
    {
      abortTransactionResponseBuilder.setHasException(true);
      abortTransactionResponseBuilder.setException(t.toString());
    }

    if (wre != null)
    {
      abortTransactionResponseBuilder.setHasException(true);
      abortTransactionResponseBuilder.setException(wre.toString());
    }

    if (ioe != null)
    {
      abortTransactionResponseBuilder.setHasException(true);
      abortTransactionResponseBuilder.setException(ioe.toString());
    }

    if (ute != null)
    {
      abortTransactionResponseBuilder.setHasException(true);
      abortTransactionResponseBuilder.setException(ute.toString());
    }

    SsccAbortTransactionResponse aresponse = abortTransactionResponseBuilder.build();

    done.run(aresponse);
  }

  /**
   * Abort the transaction.
   * 
   * @param transactionId
   * @throws IOException
   * @throws UnknownTransactionException
   */

  public void abortTransaction(final long transactionId) throws IOException, UnknownTransactionException {
    long txid = 0;
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: abort transactionId: " + transactionId + " " + m_Region.getRegionInfo().getRegionNameAsString());

    SsccTransactionState state;
    try {
      state = getTransactionState(transactionId);
    } catch (UnknownTransactionException e) {
      IOException ioe = new IOException("UnknownTransactionException");
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: Unknown transaction [" + transactionId
                 + "] in region ["
                 + m_Region.getRegionInfo().getRegionNameAsString()
                 + "], " + ioe.toString());
      
      throw new IOException("UnknownTransactionException");
    }
    if(state.getStatus() == Status.ABORTED) 
    {
      LOG.error("Transaction " + transactionId + " is already aborted, probably due to write conflict");
      return;
    }
    if(state.getStatus() == Status.COMMITED) //should be programming error in client. Like commmit() then abort()
    {
        LOG.error("Transaction " + transactionId + " is already committed, cannot perform abort anymore");
        return;  
    }

    if (state.hasWrite()) {
       //get put/del list
       //do delet to undo put, do nothiing to undo del
       List<byte[]> putUndoList = ( (SsccTransactionState)state).getPutRows();
       //List<Mutation> mutList = new ArrayList<Mutation>();
       for ( byte[] rowkey : putUndoList )
       {
         long localTransId = state.getStartId();
         Delete d = new Delete(rowkey, localTransId);
         Get forColListGet = new Get(rowkey);
         forColListGet.setTimeStamp(localTransId); //get only those cells affected by the given transaction
         //perform a get first, parse the result and get all column affected by the put 
         Result r = m_Region.get(forColListGet);
         List<Cell> listCells = r.listCells();
         if(listCells != null) 
         {
         for (Cell cell : listCells) {
           d.deleteColumn(CellUtil.cloneFamily(cell),CellUtil.cloneQualifier(cell),localTransId);   //this is the cell that needs to be delete
			}
         }
         m_Region.delete(d);
         //clear the status item
         Delete statusDelete = new Delete(rowkey, localTransId);
         statusDelete.deleteColumn(DtmConst.TRANSACTION_META_FAMILY , SsccConst.STATUS_COL );
         //statusDelete.deleteColumn(DtmConst.TRANSACTION_META_FAMILY , SsccConst.COLUMNS_COL );
         m_Region.delete(statusDelete);
         //mutList.add(d);
       }

       //clear status
       List<Delete> deleteList = state.getDelRows();
       ListIterator<Delete> deletesIter = null;
       for (deletesIter = deleteList.listIterator(); deletesIter.hasNext();) {
         Delete di = deletesIter.next();

         long localTransId = state.getStartId();
         Delete d = new Delete(di.getRow(), localTransId);
         d.deleteColumn(DtmConst.TRANSACTION_META_FAMILY , SsccConst.STATUS_COL );
         m_Region.delete(d); 
       }

       /* not understand how to use batchMutate yet
       try {
         Mutation[] m = (Mutation[])mutList.toArray();
         m_Region.batchMutate(m);
       }
       catch (Exception e) {
         //TODO
         throw new IOException(e.toString());
       }
       */
   
    }
    synchronized (commitPendingTransactions) {
        commitPendingTransactions.remove(state);
    }

    if (state.isReinstated()) {
      synchronized(indoubtTransactionsById) {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: Trafodion Recovery: abort reinstated indoubt transactions " + transactionId);
        indoubtTransactionsById.remove(state.getTransactionId());
        int tmid = (int) (transactionId >> 32);
        int count = 0;

        // indoubtTransactionsCountByTmid protected by 
        // indoubtTransactionsById synchronization
        if (indoubtTransactionsCountByTmid.containsKey(tmid)) {
            count =  (int) indoubtTransactionsCountByTmid.get(tmid) - 1;
            if (count > 0) indoubtTransactionsCountByTmid.put(tmid, count);
        }

        // if all reinstated txns are resolved from a TM, remove it and delete associated zNode
        if (count == 0) {
          indoubtTransactionsCountByTmid.remove(tmid);
          String lv_encoded = m_Region.getRegionInfo().getEncodedName();
          try {
            if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: Trafodion Recovery: delete in abort recovery zNode TM " + tmid + " region encoded name " + lv_encoded + " for 0 in-doubt transaction");
            deleteRecoveryzNode(tmid, lv_encoded);
          } catch (IOException e) {
             LOG.error("SsccRegionEndpoint coprocessor: Trafodion Recovery: delete recovery zNode failed");
          }
         }

         if ((indoubtTransactionsById == null) || 
             (indoubtTransactionsById.size() == 0)) {
           // change region state to STARTED, and archive the split-thlog

           if (indoubtTransactionsById == null)
             if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: Trafodion Recovery: start region in abort with indoubtTransactionsById null");
            else
              if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: Trafodion Recovery: start region in abort with indoubtTransactionsById size " + indoubtTransactionsById.size());
            startRegionAfterRecovery();
         }
       }
     }
    state.setStatus(Status.ABORTED);
    retireTransaction(state);

 }

  /**
   * Determines if the transaction can be committed, and if possible commits the transaction.
   * @param long transactionId
   * @return boolean
   * @throws IOException
   */
  public boolean commitIfPossible(final long transactionId)
    throws IOException {

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  commitIfPossible -- ENTRY txId: "
               + transactionId);
    int status = commitRequest(transactionId);

    if (status == COMMIT_OK) {

       IdTmId seqId;
       try {
          seqId = new IdTmId();
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  commitIfPossible:getting new IdTM sequence ");
          idServer.id(ID_TM_SERVER_TIMEOUT, seqId);
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  commitIfPossible: IdTM sequence is " + seqId.val);
       } catch (IdTmException exc) {
          LOG.error("SsccRegionEndpoint coprocessor:  commitIfPossible: IdTm threw exception 1 " + exc);
          throw new IOException("SsccRegionEndpoint coprocessor:  commitIfPossible: IdTm threw exception 1 " + exc);
       }
       catch (Exception e2) {
          LOG.error("SsccRegionEndpoint coprocessor:  beginTransaction: IdTm threw exception e2 " + e2);
          throw new IOException("SsccRegionEndpoint coprocessor:  beginTransaction: IdTm threw exception e2 " + e2);
       }

       // Process local memory
       try {
         commit(transactionId, seqId.val);
         if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  commitIfPossible -- ENTRY txId: " + transactionId + " COMMIT_OK");
         return true;
       } catch (Throwable e) {
         if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:coprocesor: commitIfPossible threw exception after internal commit");
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
        throw new IOException(e.toString());
       }
    } else if (status == COMMIT_OK_READ_ONLY) {
            if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  commitIfPossible -- ENTRY txId: " 
            + transactionId + " COMMIT_OK_READ_ONLY");
            return true;
    }
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  commitIfPossible -- ENTRY txId: " 
              + transactionId + " Commit Unsuccessful");
    return false;
  }

/***********************************************************************************
*****************  ALL code to support SCAN   ************************************
***********************************************************************************/  
 
  /**
   * Returns the scanner associated with the specified ID.
   *
   * @param long scannerId
   * @param long nextCallSeq
   * @return a Scanner or throws UnknownScannerException
   * @throws NotServingRegionException
   * @throws OutOfOrderscannerNextException
   * @throws UnknownScannerException
   */
    protected synchronized RegionScanner getScanner(long scannerId,
                                                    long nextCallSeq)  
      throws NotServingRegionException,
             OutOfOrderScannerNextException,
             UnknownScannerException {

      RegionScanner scanner = null;

      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getScanner scanners map is " + scanners + ", count is "  + scanners.size() + ", scanner id is " + scannerId);

      TransactionalRegionScannerHolder rsh = 
        scanners.get(scannerId);

      if (rsh != null)
      {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getScanner rsh is " + rsh + "rsh.s is "  + rsh.s );
      }
      else
      {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getScanner rsh is null");
          throw new UnknownScannerException(
            "ScannerId: " + scannerId + ", already closed?");
      }

      scanner = rsh.s;
      if (scanner != null) {
        HRegionInfo hri = scanner.getRegionInfo();
        if (this.m_Region != rsh.r) { // Yes, should be the same instance
          throw new NotServingRegionException("Region was re-opened after the scannerId"
            + scannerId + " was created: " + hri.getRegionNameAsString());
        }
      }

      if (nextCallSeq != rsh.nextCallSeq) {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getScanner calling OutOfOrderScannerNextException, nextCallSeq is " + nextCallSeq + " rsh.nextCallSeq is " + rsh.nextCallSeq);
        throw new OutOfOrderScannerNextException(
        "Expected nextCallSeq: " + rsh.nextCallSeq +
        " But the nextCallSeq got from client: " + nextCallSeq); 
      }

      return scanner;
    }

  @Override
  public void performScan(RpcController controller,
                          SsccPerformScanRequest request,
                          RpcCallback<SsccPerformScanResponse> done) {

    boolean hasMore = true;
    RegionScanner scanner = null;
    Throwable t = null;
    ScannerTimeoutException ste = null;
    OutOfOrderScannerNextException ooo = null;
    WrongRegionException wre = null;
    Exception ne = null;
    Scan scan = null;
    List<Cell> cellResults = new ArrayList<Cell>();
    List<Result> results = new ArrayList<Result>();
    List<Cell> validResults = new ArrayList<Cell>();
    org.apache.hadoop.hbase.client.Result result = null;
    long transId = request.getTransactionId();
    long startId = request.getStartId();

    long scannerId = request.getScannerId();
    int numberOfRows = request.getNumberOfRows();
    boolean closeScanner = request.getCloseScanner();
    long nextCallSeq = request.getNextCallSeq();
    long count = 0L;
    boolean shouldContinue = true;

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: performScan - scannerId " + scannerId + ", numberOfRows " + numberOfRows + ", nextCallSeq " + nextCallSeq);

    //This may be wrong, check later
    Map<String , Cell> tempBuf = new TreeMap<String, Cell>();

      try {

        scanner = getScanner(scannerId, nextCallSeq);
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: performScan - scanner is: " + scanner == null ? "NULL" : "NOT NULL" );

        SsccTransactionState state = this.beginTransIfNotExist(transId, startId);
        Set<byte[]>visitCols = new HashSet<byte[]>();

        if (scanner != null)
        {
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: performScan - id " + scannerId + ", scanner is not null");
          boolean firstCell=true;

          while (shouldContinue) {
            hasMore = scanner.next(cellResults);
            if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: performScan hasMore is: " + hasMore + " cellResults: " + cellResults);
            firstCell=true;
            Result verResult = null;
            Result statusResult = null;
            Result colResult = null;
            tempBuf.clear();

            ListIterator<Cell> cellIter = null;
            for (cellIter = cellResults.listIterator(); cellIter.hasNext();) {
               Cell c = cellIter.next();
               if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: performScan Cell is: " + c);
               if(firstCell == true){
                    if(CellUtil.cloneFamily(c) != DtmConst.TRANSACTION_META_FAMILY){
                    //get the statusList
                    Get statusGet = new Get(c.getRow()); //TODO: deprecated API
                    //statusGet.setTimeStamp(startId);
                    statusGet.addColumn(DtmConst.TRANSACTION_META_FAMILY,SsccConst.STATUS_COL);
                    statusGet.setMaxVersions(DtmConst.SSCC_MAX_VERSION);
                    statusResult = m_Region.get(statusGet);

                    //get the colList
                    Get colGet = new Get(c.getRow()); //TODO: deprecated API
                    //colGet.setTimeStamp(startId);
                    colGet.addColumn(DtmConst.TRANSACTION_META_FAMILY,SsccConst.COLUMNS_COL);
                    colGet.setMaxVersions(DtmConst.SSCC_MAX_VERSION);
                    colResult = m_Region.get(colGet);

                    //get the versionList
                    Get verGet = new Get(c.getRow());//TODO: deprecated API
                    //verGet.setTimeStamp(startId);
                    verGet.addColumn(DtmConst.TRANSACTION_META_FAMILY,SsccConst.VERSION_COL);
                    verGet.setMaxVersions(DtmConst.SSCC_MAX_VERSION);
                    verResult = m_Region.get(verGet);   
                    firstCell = false;
                    }
                }

                //long kvStartId = getStartIdFromTs(thisTs);
                if(firstCell == false) {

                    //long thisTs = c.getTimestamp();
                    if (state.handleResult(c,statusResult.listCells(),verResult.listCells(),colResult.listCells(),transId) == true)
                    {
                        byte[] keyt=CellUtil.cloneQualifier(c);
                        String keys = new String(keyt);
                        if(tempBuf.containsKey(keys) == false)  //only get the first one, suppose first is latest
                            tempBuf.put(keys,c);
                    }
                }
            }
            for(String j: tempBuf.keySet())
            {
                Cell kv = tempBuf.get(j);
                validResults.add(kv);
            }

            result = Result.create(validResults);
            cellResults.clear();
            validResults.clear();
            if (!result.isEmpty()) {
              results.add(result);
              count++;
            }

            if (count == numberOfRows || !hasMore)
              shouldContinue = false;
            if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: performScan - id " + scannerId + ", count is " + count + ", hasMore is " + hasMore +
                      ", result.isEmpty: " + result.isEmpty() + ", row " + result.getRow() + ", shouldContinue: " + shouldContinue);
          }
        }
        else
        {
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: performScan - id " + scannerId+ ", scanner is null");
        }

     } catch(OutOfOrderScannerNextException ooone) {
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: performScan - id " + scannerId + " Caught OutOfOrderScannerNextException  " + ooone.getMessage() + " " + stackTraceToString(ooone));
       ooo = ooone;
     } catch(ScannerTimeoutException cste) {
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: performScan - id " + scannerId + " Caught ScannerTimeoutException  " + cste.getMessage() + " " + stackTraceToString(cste));
       ste = cste;
     } catch(Throwable e) {
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: performScan - id " + scannerId + " Caught exception  " + e.getMessage() + " " + stackTraceToString(e));
       t = e;
     }
     finally {
       if (scanner != null) {
         try {
           if (closeScanner) {
             scanner.close();
/*
             try {
               scannerLeases.cancelLease(getScannerLeaseId(scannerId));
             } catch (LeaseException le) {
               // ignore
               if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: performScan failed to get a lease " + scannerId);
             }
*/
           }
         } catch(Exception e) {
           if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  performScan caught exception " + e.getMessage() + "" + stackTraceToString(e));
           ne = e;
         }
       }
     }

   TransactionalRegionScannerHolder rsh = scanners.get(scannerId);

   nextCallSeq++;

   rsh.nextCallSeq = nextCallSeq;

   if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: performScan - id " + transId + ", regionName " + regionInfo.getRegionNameAsString() +
", scannerId " + scannerId + ", nextCallSeq " + nextCallSeq + ", rsh.nextCallSeq " + rsh.nextCallSeq);
    org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPerformScanResponse.Builder performResponseBuilder = SsccPerformScanResponse.newBuilder();
    performResponseBuilder.setHasMore(hasMore);
    performResponseBuilder.setNextCallSeq(nextCallSeq);
    performResponseBuilder.setCount(count);
    performResponseBuilder.setHasException(false);

    if (results != null)
    {
      if (!results.isEmpty()) {
        ListIterator<Result> resultIter = null;
        for (resultIter = results.listIterator(); resultIter.hasNext();) {
          Result r = resultIter.next();
          performResponseBuilder.addResult(ProtobufUtil.toResult(r));
//          LOG.info("UNIQUE: performScan return row " + Arrays.toString(r.getRow()) );
//          for( Cell c : r.listCells() ) {
//            LOG.info("UNIQUE: performScan return value col : " + Arrays.toString(CellUtil.cloneQualifier(c) )+ " value " + Arrays.toString(CellUtil.cloneValue(c) ) );
//          }
//          LOG.info("UNIQUE : -- ");
        }
      }
    }

    if (t != null)
    {
      performResponseBuilder.setHasException(true);
      performResponseBuilder.setException(t.toString());
    }

    if (ste != null)
    {
      performResponseBuilder.setHasException(true);
      performResponseBuilder.setException(ste.toString());
    }

    if (wre != null)
    {
      performResponseBuilder.setHasException(true);
      performResponseBuilder.setException(wre.toString());
    }

    if (ne != null)
    {
      performResponseBuilder.setHasException(true);
      performResponseBuilder.setException(ne.toString());
    }

    if (ooo != null)
    {
      performResponseBuilder.setHasException(true);
      performResponseBuilder.setException(ooo.toString());
    }
     SsccPerformScanResponse presponse = performResponseBuilder.build();
     done.run(presponse);
   }

  @Override
  public void openScanner(RpcController controller,
                          SsccOpenScannerRequest request,
                          RpcCallback<SsccOpenScannerResponse> done) {
    boolean hasMore = true;
    RegionScanner scanner = null;
    RegionScanner scannert = null;
    Throwable t = null;
    WrongRegionException wre = null;
    boolean exceptionThrown = false;
    NullPointerException npe = null;
    Exception ge = null;
    IOException ioe = null;
    LeaseStillHeldException lse = null;
    Scan scan = null;
    long scannerId = 0L;
    boolean isLoadingCfsOnDemandSet = false;
    long transId = request.getTransactionId();
    long startId = request.getStartId();

    {

      try {
        scan = ProtobufUtil.toScan(request.getScan());
        if (scan == null)
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  openScanner scan was null");
      } catch (Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  openScanner Caught exception " + e.getMessage() + "" + stackTraceToString(e));
        t = e;
        exceptionThrown = true;
      }
    }

    if (!exceptionThrown) {
      if (scan == null) {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:openScanner scan is null");
        npe = new NullPointerException("scan is null");
        ioe =  new IOException("Invalid arguments to openScanner", npe);
        exceptionThrown = true;
      }
      else
      {
        try {
          scan.getAttribute(Scan.SCAN_ATTRIBUTES_METRICS_ENABLE);
          //checkRow(scan.getStartRow(), "Scan");            
          prepareScanner(scan);
        } catch (Throwable e) {
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:openScanner scan threw exception");
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
          t = e;
          exceptionThrown = true;
        }
      }
    }

    List<Cell> results = new ArrayList<Cell>();

    if (!exceptionThrown) {
      try {
          scan.setMaxVersions();
        scanner = getScanner(transId, startId, scan);
        if (scanner != null) {
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  openScanner called getScanner, scanner is " + scanner + ", transid " + transId);
          // Add the scanner to the map
          scannerId = addScanner(transId,scanner, this.m_Region);
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  openScanner called addScanner, scannerId is " + scannerId + ", transid " + transId);
        }
        else {
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  getScanner returned null, scannerId is " + scannerId + ", transid " + transId);
        }
      } catch (LeaseStillHeldException llse) {

        LOG.error("SsccRegionEndpoint coprocessor:  getScanner Error opening scanner, " + llse.toString());
        exceptionThrown = true;
        lse = llse;
      } catch (IOException e) {
        LOG.error("SsccRegionEndpoint coprocessor:  getScanner Error opening scanner, " + e.toString());
        exceptionThrown = true;
      }
    }

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: openScanner - transid " + transId + ", regionName " + regionInfo.getRegionNameAsString());

    org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccOpenScannerResponse.Builder openResponseBuilder = SsccOpenScannerResponse.newBuilder();

    openResponseBuilder.setScannerId(scannerId);
    openResponseBuilder.setHasException(false);

    if (t != null)
    {
      openResponseBuilder.setHasException(true);
      openResponseBuilder.setException(t.toString());
    }

    if (wre != null)
    {
      openResponseBuilder.setHasException(true);
      openResponseBuilder.setException(wre.toString());
    }

    if (ioe != null)
    {
      openResponseBuilder.setHasException(true);
      openResponseBuilder.setException(ioe.toString());
    }

    if (lse != null)
    {
      openResponseBuilder.setHasException(true);
      openResponseBuilder.setException(lse.toString());
    }

    SsccOpenScannerResponse oresponse = openResponseBuilder.build();
    done.run(oresponse);
  }

  /**
   * Obtain a RegionScanner
   * @param long transactionId
   * @param Scan scan
   * @return RegionScanner
   * @throws IOException 
   */
  public RegionScanner getScanner(final long transactionId, final long startId, final Scan scan)
                        throws IOException { 

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  RegionScanner getScanner -- ENTRY txId: " + transactionId );

    SsccTransactionState state = this.beginTransIfNotExist(transactionId, startId);

    //state.addScan(scan);

    List<KeyValueScanner> scanners = new ArrayList<KeyValueScanner>(1);

    //Scan deleteWrapScan = wrapWithDeleteFilter(scan, state);
    //In SSCC, we cannot find a way to do this with Filter...

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  RegionScanner getScanner -- Calling t_Region.getScanner txId: " + transactionId );
    RegionScanner gotScanner =  this.t_Region.getScanner(scan); 
    if (gotScanner != null)
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  RegionScanner getScanner -- obtained scanner was not null,  txId: " + transactionId );
    else
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  RegionScanner getScanner -- obtained scanner was null,  txId: " + transactionId );
    return gotScanner;
  }

  @Override
  public void closeScanner(RpcController controller,
                           SsccCloseScannerRequest request,
                           RpcCallback<SsccCloseScannerResponse> done) {

    RegionScanner scanner = null;
    Throwable t = null;
    WrongRegionException wre = null;
    Exception ce = null;
    long transId = request.getTransactionId();
    long scannerId = request.getScannerId();

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: closeScanner - transId " + transId+ ", regionName "
                                        + regionInfo.getRegionNameAsString() + ", scannerId " + scannerId);

      try {
         scanner = removeScanner(scannerId);

         if (scanner != null) { 
             scanner.close();
         }
         else
           if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  closeScanner scanner was null for scannerId " + scannerId);

/*
         try {
           scannerLeases.cancelLease(getScannerLeaseId(scannerId));
         } catch (LeaseException le) {
           // ignore
           if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: closeScanner failed to get a lease " + scannerId);
         }
*/

      } catch(Exception e) {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  closeScanner caught exception " + e.getMessage() + "" + stackTraceToString(e));
        ce = e;
      } catch(Throwable e) {
         if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: closeScanner - id Caught exception  " + e.getMessage() + " " + stackTraceToString(e));
         t = e;
      }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCloseScannerResponse.Builder closeResponseBuilder = SsccCloseScannerResponse.newBuilder();

    closeResponseBuilder.setHasException(false);

    if (t != null)
    {
      closeResponseBuilder.setHasException(true);
      closeResponseBuilder.setException(t.toString());
    }

    if (wre != null)
    {
      closeResponseBuilder.setHasException(true);
      closeResponseBuilder.setException(wre.toString());
    }

    if (ce != null)
    {
      closeResponseBuilder.setHasException(true);
      closeResponseBuilder.setException(ce.toString());
    }

    SsccCloseScannerResponse cresponse = closeResponseBuilder.build();
    done.run(cresponse);
  }

  
/***********************************************************************************
*****************  ALL code to support GET    ************************************
***********************************************************************************/  

  @Override
  public void get(RpcController controller,
                  SsccGetTransactionalRequest request,
                  RpcCallback<SsccGetTransactionalResponse> done) {
    SsccGetTransactionalResponse response = SsccGetTransactionalResponse.getDefaultInstance();

    org.apache.hadoop.hbase.protobuf.generated.ClientProtos.Get proto = request.getGet();
    Get get = null;
    RegionScanner scanner = null;
    Throwable t = null;
    Exception ge = null;
    WrongRegionException wre = null;
    org.apache.hadoop.hbase.client.Result result2 = null;

      try {
        get = ProtobufUtil.toGet(proto);
      } catch (Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:get threw exception");
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
        t = e;
      }

      byte[] row = proto.getRow().toByteArray();
      byte[] getrow = get.getRow();
      String rowKey = Bytes.toString(row);
      String getRowKey = Bytes.toString(getrow);
      long transId = request.getTransactionId();
      long startId = request.getStartId();

      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: get - id " + transId + ", regionName " + regionInfo.getRegionNameAsString() + ", row = " + rowKey + ", getRowKey = " + getRowKey);

      try {

         if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: get - id Calling getScanner id/scan " + transId + ", regionName " + regionInfo.getRegionNameAsString() + ", row = " + rowKey + ", getRowKey = " + getRowKey);

         result2 = get(transId, startId, get);

         if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: get - id No exception, result2 isEmpty is " 
		   + result2.isEmpty() 
		   + ", row " 
		   + result2.getRow()
		   + " result length: "
		   + result2.size()); 

      } catch(Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: get - id Caught exception  " + e.getMessage() + " " + stackTraceToString(e));
        t = e;
      }
      finally {
        if (scanner != null) {
          try {
            scanner.close();
          } catch(Exception e) {
            if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
            ge = e;
          }
        }
      }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccGetTransactionalResponse.Builder getResponseBuilder = SsccGetTransactionalResponse.newBuilder();

    try {
      getResponseBuilder.setResult(ProtobufUtil.toResult(result2));
    }
    catch (Exception e) {
       LOG.error("SsccRegionEndpoint coprocessor:  Caught exception getting result2 " + result2 + " " + stackTraceToString(e));
    }
    /*
    if(result2 !=null){
    LOG.info("UNIQUE: get result for row " + Arrays.toString(result2.getRow() ));

    for( Cell c : result2.listCells() )
    {
            LOG.info("UNIQUE: get return value col : " + Arrays.toString(CellUtil.cloneQualifier(c) )+ " value " + Arrays.toString(CellUtil.cloneValue(c) ) );    
    }
    }
    */

   getResponseBuilder.setHasException(false);

   if (t != null)
   {
     getResponseBuilder.setHasException(true);
     getResponseBuilder.setException(t.toString());
   }

   if (wre != null)
   {
     getResponseBuilder.setHasException(true);
     getResponseBuilder.setException(wre.toString());
   }

   if (ge != null)
   {
     getResponseBuilder.setHasException(true);
     getResponseBuilder.setException(ge.toString());
   }

   SsccGetTransactionalResponse gresponse = getResponseBuilder.build();
    done.run(gresponse);
   }

  /**
   * Obtains a transactional Result for Get          
   * @param long transactionId
   * @param Get get             
   * @return Result 
   * @throws IOException 
   */
  public Result get(final long transactionId, final long startId, final Get get)
                          throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  get --  ENTRY txId: " + transactionId + " startId: " + startId );
    //get thet state object
    SsccTransactionState state = this.beginTransIfNotExist(transactionId, startId);

    Scan scan = new Scan(get);

    List<Cell> results = new ArrayList<Cell>();
    RegionScanner scanner = null;
    long max = scan.getTimeRange().getMax();
    long min = scan.getTimeRange().getMin();

    scan.setTimeRange(0, startId + 1);  //only get data updated before me 
    scan.setMaxVersions(DtmConst.SSCC_MAX_VERSION);

    Map<String , Cell> tempBuf = new TreeMap<String, Cell>();

    try {
      scanner = m_Region.getScanner(scan);

      //get the statusList
      Get statusGet = new Get(get.getRow());//TODO: deprecated API
      //statusGet.setTimeStamp(startId);
      statusGet.addColumn(DtmConst.TRANSACTION_META_FAMILY,SsccConst.STATUS_COL);
      statusGet.setMaxVersions(DtmConst.SSCC_MAX_VERSION);
      Result statusResult = m_Region.get(statusGet);

      //get the versionList
      Get verGet = new Get(get.getRow());//TODO: deprecated API
      //verGet.setTimeStamp(startId);
      verGet.addColumn(DtmConst.TRANSACTION_META_FAMILY,SsccConst.VERSION_COL);
      verGet.setMaxVersions(DtmConst.SSCC_MAX_VERSION);
      Result verResult = m_Region.get(verGet);

      //get the colList
      Get colGet = new Get(get.getRow()); //TODO: deprecated API
      //colGet.setTimeStamp(startId);
      colGet.addColumn(DtmConst.TRANSACTION_META_FAMILY,SsccConst.COLUMNS_COL);
      colGet.setMaxVersions(DtmConst.SSCC_MAX_VERSION);
      Result colResult = m_Region.get(colGet);

      /* go through all the versions and find out correct one */
      boolean hasMore = false;
      List<Cell> eachVersion = new ArrayList<Cell>();
      Set<byte[]>visitCols = new HashSet<byte[]>();

      List<Cell>  sl = statusResult.listCells();
      List<Cell> vl = verResult.listCells();
      List<Cell> cl = colResult.listCells();

      do {
          eachVersion.clear();
          hasMore = scanner.next(eachVersion); 
          tempBuf.clear();
          for( Cell kv : eachVersion) {
              long thisTs = kv.getTimestamp();
              //find out the startId for thisTs
              //long kvStartId = getStartIdFromTs(thisTs);
              if (state.handleResult(kv,sl,vl,cl,transactionId) == true)
              {
                  byte[] keyt=CellUtil.cloneQualifier(kv);
                  String keys = new String(keyt);
                  if(tempBuf.containsKey(keys) == false)
                  	tempBuf.put(keys,kv);
              }
          }
        for(String j: tempBuf.keySet())
        {
            Cell kv = tempBuf.get(j);
            results.add(kv);
        }
      } while (hasMore);
    } catch(Exception e) {
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  get Caught exception " + e.getMessage() + "" + stackTraceToString(e));
      LOG.error("SsccRegionEndpoint coprocessor:  get Caught exception " + e.getMessage() + "" + stackTraceToString(e));
    }
    finally {
      if (scanner != null) {
        scanner.close();
      }
    }
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  get -- EXIT txId: " + transactionId);
    return Result.create(results);       
  }

/***********************************************************************************
*****************  ALL code to support stateless/stateful PUT   ********************
***********************************************************************************/

  @Override
  public void put(RpcController controller,
                  SsccPutTransactionalRequest request,
      RpcCallback<SsccPutTransactionalResponse> done) {
    SsccPutTransactionalResponse response = SsccPutTransactionalResponse.getDefaultInstance();

    boolean stateless = false;
    if (request.hasIsStateless()) {
       stateless = request.getIsStateless();
    }
    byte [] row = null;
    MutationProto proto = request.getPut();
    MutationType type = proto.getMutateType();
    Put put = null;
    Throwable t = null;
    WrongRegionException wre = null;
    int status = 0;
    long transId = request.getTransactionId();
    long startId = request.getStartId();
    try {
       put = ProtobufUtil.toPut(proto);
    } catch (Throwable e) {
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:put threw exception");
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
       LOG.error("UNIQUE coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
       t = e;
    }

    if (type == MutationType.PUT && proto.hasRow()) {
       row = proto.getRow().toByteArray();

       // Process in local memory
       try {
          status = put(transId, startId, put, stateless);
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:put returned status: " + status);
       } catch (Throwable e) {
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:put threw exception after internal put");
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
          t = e;
       }

       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: put - id " + transId + ", regionName " + regionInfo.getRegionNameAsString() + ", type " + type + ", row " + Bytes.toString(row));
    }
    else  {
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: put - id " + transId + ", regionName " + regionInfo.getRegionNameAsString() + "- no valid PUT type or does not contain a row");
    }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutTransactionalResponse.Builder putTransactionalResponseBuilder = SsccPutTransactionalResponse.newBuilder();

    putTransactionalResponseBuilder.setHasException(false);

    if (t != null)
    {
      putTransactionalResponseBuilder.setHasException(true);
      putTransactionalResponseBuilder.setException(t.toString());
    }

    if (wre != null)
    {
      putTransactionalResponseBuilder.setHasException(true);
      putTransactionalResponseBuilder.setException(wre.toString());
    }

    putTransactionalResponseBuilder.setStatus(status);

    SsccPutTransactionalResponse presponse = putTransactionalResponseBuilder.build();
    done.run(presponse);
  }

  /**
   * Add a write to the transaction.
   * process.
   * @param long transactionId
   * @param Put put
   * @param boolean stateless  // Is this a stateless put?
   * @return int
   * @throws IOException
   */

  public int put(final long transactionId, final long startId, final Put put, boolean stateless)
    throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("Enter SsccRegionEndpoint coprocessor: put, txid " + transactionId +
                  ", startId " + startId +", stateless: " + stateless);
    SsccTransactionState state = this.beginTransIfNotExist(transactionId, startId);

    // check whether has del before
    state.removeDelBeforePut(put, stateless);

    /*need to change the timestamp, but HBase API does not support
      At this point, the solution is to create a new Put object
    */
    //So the solution at this point is
    //add a mapping of current timestamp of the put row with the startId
    //mapStartIdFromTs(put.getTimeStamp(),startId);
    // try to use getFamilyCellMap to get out all data from the put object and generate a new one
    byte[] rowkey = put.getRow();
    Put newPut = new Put(rowkey, startId);
    byte[] mergedCols = null;
    byte[] mergedColsV = null;
    byte[] cv = null;
    NavigableMap<byte[], List<Cell>> familyCellMap = put.getFamilyCellMap();
    for (Entry<byte[], List<Cell>> entry : familyCellMap.entrySet()) {
        for (Iterator<Cell> iterator = entry.getValue().iterator(); iterator.hasNext();) {
            Cell cell = iterator.next();
            byte[] family = CellUtil.cloneFamily(cell);
            byte[] qualifier = CellUtil.cloneQualifier(cell);
            mergedCols = null;
            mergedCols = byteMerger("|".getBytes(),qualifier);
            mergedCols = byteMerger(mergedCols,"|".getBytes());
            byte[] value = CellUtil.cloneValue(cell);
            newPut.add(family,qualifier,startId,value);
            byte[] currentCollist =  state.getColList(rowkey);
            if( indexOf(currentCollist,mergedCols) != -1) //already in this list
            {
                mergedColsV = byteMerger(currentCollist,null);
                continue;
            }
            mergedColsV = byteMerger(mergedCols,currentCollist);
            state.addToColList(rowkey,mergedColsV);
        }
    }

    //get the statusList
    Get statusGet = new Get(rowkey);
    //statusGet.setTimeStamp(startId);
    statusGet.addColumn(DtmConst.TRANSACTION_META_FAMILY,SsccConst.STATUS_COL);
    //statusGet.setTimeRange(0, startId + 1);  //only get data updated before me
    //statusGet.setMaxVersions(DtmConst.SSCC_MAX_VERSION);
    statusGet.setMaxVersions();

    Result statusResult = m_Region.get(statusGet);

    List<Cell> sl = null;
    List<Cell> vl = null;

    //get the versionList
    //  If this is a stateless put we don't need the version list
    if (stateless == false) {
       Get verGet = new Get(rowkey);

       //verGet.setTimeStamp(startId);
       verGet.addColumn(DtmConst.TRANSACTION_META_FAMILY,SsccConst.VERSION_COL);
       verGet.setMaxVersions(DtmConst.SSCC_MAX_VERSION);

       Result verResult = m_Region.get(verGet);
       if(verResult != null )  vl = verResult.listCells();
    }

    if(statusResult != null ) sl = statusResult.listCells();
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: put stateless: " + stateless  );
    if(state.hasConflict(sl,vl,stateless,startId,transactionId) == false) {
        state.addToPutList(rowkey);
        //update status metadata
        byte[] statusValue;
        if (stateless){
           statusValue = SsccConst.generateStatusValue(SsccConst.S_STATELESS_BYTE,transactionId); //stateless update
        }
        else {
           statusValue = SsccConst.generateStatusValue(SsccConst.S_STATEFUL_BYTE,transactionId); //stateful update
        }
        newPut.add(DtmConst.TRANSACTION_META_FAMILY ,SsccConst.STATUS_COL,startId , statusValue);
        newPut.add(DtmConst.TRANSACTION_META_FAMILY ,SsccConst.COLUMNS_COL,startId ,mergedColsV);
        //perform the put operation, persistently save the data now.
//        LOG.info("UNIQUE: put ok "   );
        m_Region.put(newPut);
        return stateless ? STATELESS_UPDATE_OK : STATEFUL_UPDATE_OK;

    }
    else { //conflict
        // Return conflict, but don't trigger and abort.  That needs to be triggered from the client, if desired.
        if (LOG.isTraceEnabled()) LOG.trace("UNIQUE: put STATEFUL_UPDATE_CONFLICT " );
        return stateless ? STATELESS_UPDATE_CONFLICT : STATEFUL_UPDATE_CONFLICT;
    }
  }

/***********************************************************************************
*****************  ALL code to support DELETE   ************************************
***********************************************************************************/
  @Override
  public void delete(RpcController controller,
                                SsccDeleteTransactionalRequest request,
    RpcCallback<SsccDeleteTransactionalResponse> done) {
    SsccDeleteTransactionalResponse response = SsccDeleteTransactionalResponse.getDefaultInstance();

    byte [] row = null;
    MutationProto proto = request.getDelete();
    MutationType type = proto.getMutateType();
    Delete delete = null;
    Throwable t = null;
    WrongRegionException wre = null;
    long transId = request.getTransactionId();
    long startId = request.getStartId();

    if (wre == null && type == MutationType.DELETE && proto.hasRow())
      row = proto.getRow().toByteArray();
    try {
        delete = ProtobufUtil.toDelete(proto); 
    } catch (Throwable e) {
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:delete threw exception");
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + " " + stackTraceToString(e));
      t = e;
    }

    // Process in local memory
    int status = 0;
    try {
      status = delete(transId, startId, delete);
    } catch (Throwable e) {
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:delete threw exception after internal delete");
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
      t = e;
    }

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: delete - id " + transId + ", regionName " + regionInfo.getRegionNameAsString() + ", type " + type + ", row " + Bytes.toString(row));

     org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteTransactionalResponse.Builder deleteTransactionalResponseBuilder = SsccDeleteTransactionalResponse.newBuilder();

    deleteTransactionalResponseBuilder.setHasException(false);

    if (t != null)
    {
      deleteTransactionalResponseBuilder.setHasException(true);
      deleteTransactionalResponseBuilder.setException(t.toString());
    }

    if (wre != null)
    {
      deleteTransactionalResponseBuilder.setHasException(true);
      deleteTransactionalResponseBuilder.setException(wre.toString());
    }

    deleteTransactionalResponseBuilder.setStatus(status);

    SsccDeleteTransactionalResponse dresponse = deleteTransactionalResponseBuilder.build();
    done.run(dresponse);
  }

  /**
   * Processes a transactional delete
   * @param long transactionId
   * @param Delete delete
   * @return int
   * @throws IOException
   */
  public int delete(final long transactionId, final long startId, final Delete delete)
    throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: delete -- ENTRY txId: " + transactionId + ", startId:" + startId);
    checkClosing(transactionId);
    SsccTransactionState state = this.beginTransIfNotExist(transactionId, startId);

    //clone the delete
    //just to change the timestamp. But I hope the overhead is not too big a concern here
    byte[] rowkey = delete.getRow();
    Delete newDelete = new Delete( rowkey,startId );
    NavigableMap<byte[], List<Cell>> familyCellMap = delete.getFamilyCellMap();
    byte[] mergedColsV = null;
    byte[] cv = null;
    for (Entry<byte[], List<Cell>> entry : familyCellMap.entrySet()) {
        for (Iterator<Cell> iterator = entry.getValue().iterator(); iterator.hasNext();) {
            Cell cell = iterator.next();
            byte[] family = CellUtil.cloneFamily(cell);
            byte[] qualifier = CellUtil.cloneQualifier(cell);
            cv = null;
            cv = byteMerger("|".getBytes(), null);
            cv = byteMerger(cv,qualifier);
            cv = byteMerger(cv,"|".getBytes());
            byte[] currentCollist =  state.getColList(rowkey);
            newDelete.deleteColumns(family,qualifier,startId);  //NOTE: HBase 1.0 this API will change ...
            //here use deleteColumns with timestamp, so it will delete all history version of this row
            //but the real delete is not done at this point, but when doCommit
            //Only when it goes to doCommit(), it is safe to delete all versions
            //Otherwise, SSCC use MVCC in hbase to save history versions, those versions may need in other transaction
            //  to get a snapshot value in the before.
            //Another choice here is to use deleteColumn instead of using deleteColumns, so it will only delete the specific version
            //  specified by the startId. But I suppose these two methods are same. Need more test maybe.

            if( indexOf(currentCollist,cv) != -1) //already in this list
            {
                mergedColsV = byteMerger(currentCollist,null);
                continue;
            }
            mergedColsV = byteMerger(currentCollist,cv);
            state.addToColList(rowkey,mergedColsV);
        }
    }
    Get statusGet = new Get(rowkey);
    statusGet.addColumn(DtmConst.TRANSACTION_META_FAMILY,SsccConst.STATUS_COL);
    statusGet.setMaxVersions();
    Result statusResult = m_Region.get(statusGet);
    List<Cell> sl = null;
    if(statusResult != null ) sl = statusResult.listCells();

    // All deletes are treated as stateless, so no need to retrieve the versions
    if(state.hasConflict(sl,null,true,startId,transactionId) == false){
       state.addToDelList(newDelete);
       /*update the status metadata*/
       Put statusPut = new Put(rowkey,startId );
       byte[] statusValue;
       statusValue = SsccConst.generateStatusValue(SsccConst.S_DELETE_BYTE,transactionId); //stateless delete
       statusPut.add(DtmConst.TRANSACTION_META_FAMILY ,SsccConst.STATUS_COL, startId , statusValue);
       statusPut.add(DtmConst.TRANSACTION_META_FAMILY ,SsccConst.COLUMNS_COL,startId , mergedColsV);
       m_Region.put(statusPut);
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: delete: STATELESS_UPDATE_OK");
       return STATELESS_UPDATE_OK;
    }
    else
    {
       // Return conflict, but don't trigger and abort.  That needs to be triggered from the client, if desired.
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: delete: STATELESS_UPDATE_CONFLICT");
       return STATELESS_UPDATE_CONFLICT;
    }
 }


/***********************************************************************************
*************************  Helper functions    ************************************
***********************************************************************************/  
    public static int indexOf(byte[] source, byte[] find) {
        if (source == null) {
            return -1;
        }
        if (find == null) {
            return -1;
        }
        if (find.length > source.length) {
            return -1;
        }
        final int maxIndex = source.length - find.length;
        final int maxLength = find.length;
        final byte firstByte = find[0];
        int index = 0;
        Loop:
        do {
            if (source[index] == firstByte ) {
                for (int i = 1; i < maxLength; i++) {
                    if (source[index + i] != find[i]) {
                        continue Loop;
                    }
                }
                return index;
            }
        } while (++index <= maxIndex);
        return -1;
    }
	public static byte[] byteMerger(byte[] byte_1, byte[] byte_2){
        if (byte_1 == null) 
        {
            if (byte_2 == null)
                return null;
            else
                return byte_2;
        }
        else
        {
            if (byte_2 == null)
                return byte_1;
        }

		byte[] byte_3 = new byte[byte_1.length+byte_2.length];
		System.arraycopy(byte_1, 0, byte_3, 0, byte_1.length);
		System.arraycopy(byte_2, 0, byte_3, byte_1.length, byte_2.length);
		return byte_3;
	}
    /** methods to handle the lookup table
    */

    /**
      generate a local transId and update the lookup table
    */
/*    public void generateLocalId(SsccTransactionState state)
    {
        long localId = nextLocalTransId.getAndIncrement();
        state.setStartId(localId);
    }
*/
    public long getStartIdFromTs(long ts)
    {
        long id = 0;
        if(updateTsToStartId.containsKey(ts))
            id= updateTsToStartId.get(ts);
        return id; 
    }

    public void mapStartIdFromTs(long ts, long id)
    {
        updateTsToStartId.put(ts,id);
    }


  /**
   * Retires the transaction
   * @param SsccTransactionState state
   */
  private void retireTransaction(final SsccTransactionState state) {
    //long key = state.getTransactionId();
    String key = getTransactionalUniqueId(state.getTransactionId());

    state.clearStateResource();

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: retireTransaction: ["
             + state.getTransactionId() + "]");

    try {
      transactionLeases.cancelLease(key);
    } catch (LeaseException le) {
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: retireTransaction: ["
               + state.getTransactionId() + "] LeaseException");
      // Ignore
    } catch (Exception e) {
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: retireTransaction: ["
               + state.getTransactionId() + "] General Lease exception");
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
      // Ignore
    }

    // Clearing transaction conflict check list in case it is holding
    // a reference to a transaction state

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  retireTransaction clearTransactionsById: " + key + " from list");
    //state.clearTransactionsToCheck();

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  retireTransaction calling Removing transaction: " + key + " from list");
    transactionsById.remove(key);
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  retireTransaction Removed transaction: " + key + " from list");

   }

  @Override
  public void checkAndDelete(RpcController controller,
                          SsccCheckAndDeleteRequest request,
                          RpcCallback<SsccCheckAndDeleteResponse> done) {

    SsccCheckAndDeleteResponse response = SsccCheckAndDeleteResponse.getDefaultInstance();

    byte [] rowArray = null;
    com.google.protobuf.ByteString row = null;
    com.google.protobuf.ByteString family = null;
    com.google.protobuf.ByteString qualifier = null;
    com.google.protobuf.ByteString value = null;
    MutationProto proto = request.getDelete();
    MutationType type = proto.getMutateType();
    Delete delete = null;
    Throwable t = null;
    WrongRegionException wre = null;
    int status = 0;
    boolean result = false;
    long transId = request.getTransactionId();
    long startId = request.getStartId();

    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();


    org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndDeleteResponse.Builder checkAndDeleteResponseBuilder = SsccCheckAndDeleteResponse.newBuilder();

    if (wre == null && type == MutationType.DELETE && proto.hasRow())
    {
      rowArray = proto.getRow().toByteArray();

      try {
          delete = ProtobufUtil.toDelete(proto);
      } catch (Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndDelete caught exception " + e.getMessage() + "" + stackTraceToString(e));
        t = e;
      }

      // Process in local memory
      if (delete != null && t == null)
      {
        if (request.hasRow()) {
          row = request.getRow();

        if (!Bytes.equals(rowArray, request.getRow().toByteArray()))
          t = new org.apache.hadoop.hbase.DoNotRetryIOException("Action's " +
          "Delete row must match the passed row");
        }

        if (t == null) {
          if (request.hasRow())
            row = request.getRow();
          if (request.hasFamily())
            family = request.getFamily();
          if (request.hasQualifier())
            qualifier = request.getQualifier();
          if (request.hasValue())
            value = request.getValue();

          try {
           result = checkAndDelete(transId, startId,
               request.getRow().toByteArray(),
               request.getFamily().toByteArray(),
               request.getQualifier().toByteArray(),
               request.getValue().toByteArray(),
               delete);
           } catch (Throwable e) {
             if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndDelete caught exception " + e.getMessage() + "" + stackTraceToString(e));
             t = e;
           }
         }

       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndDelete setting result: " + result);
       checkAndDeleteResponseBuilder.setResult(result);
     }
    }
    else
    {
      result = false;
      checkAndDeleteResponseBuilder.setResult(result);
    }

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  checkAndDelete result is " + result);
    checkAndDeleteResponseBuilder.setHasException(false);

    if (t != null)
    {
      checkAndDeleteResponseBuilder.setHasException(true);
      checkAndDeleteResponseBuilder.setException(t.toString());
    }

    if (wre != null)
    {
      checkAndDeleteResponseBuilder.setHasException(true);
      checkAndDeleteResponseBuilder.setException(wre.toString());
    }


    SsccCheckAndDeleteResponse checkAndDeleteResponse = checkAndDeleteResponseBuilder.build();

    done.run(checkAndDeleteResponse);
  }

  @Override
  public void checkAndPut(RpcController controller,
                          SsccCheckAndPutRequest request,
                          RpcCallback<SsccCheckAndPutResponse> done) {

    SsccCheckAndPutResponse response = SsccCheckAndPutResponse.getDefaultInstance();

    byte [] rowArray = null;
    com.google.protobuf.ByteString row = null;
    com.google.protobuf.ByteString family = null;
    com.google.protobuf.ByteString qualifier = null;
    com.google.protobuf.ByteString value = null;
    MutationProto proto = request.getPut();
    MutationType type = proto.getMutateType();
    Put put = null;
    WrongRegionException wre = null;
    Throwable t = null;
    boolean result = false;
    long transId = request.getTransactionId();
    long startId = request.getStartId();

    org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccCheckAndPutResponse.Builder checkAndPutResponseBuilder = SsccCheckAndPutResponse.newBuilder();

    if (wre == null && type == MutationType.PUT && proto.hasRow())
    {
      rowArray = proto.getRow().toByteArray();

      try {
          put = ProtobufUtil.toPut(proto);
      } catch (Throwable e) {
        LOG.error("SsccRegionEndpoint coprocessor: checkAndPut caught exception " + e.getMessage() + "" + stackTraceToString(e));
        t = e;
      }

      // Process in local memory
      if (put != null)
      {
        if (request.hasRow()) {
          row = request.getRow();

          if (!Bytes.equals(rowArray, row.toByteArray())) {
             t = new org.apache.hadoop.hbase.DoNotRetryIOException("Action's Put row must match the passed row");
             if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndPut creating DoNotRetryIOException: setting result: Put row must match the passed row\n" +
                  "rowArray: " + Bytes.toStringBinary(rowArray) + ", rowArray in hex: " + Hex.encodeHexString(rowArray) + ", row: " +
                  Bytes.toStringBinary(request.getRow().toByteArray()) + ", row in hex: " + Hex.encodeHexString(request.getRow().toByteArray()));
          }
        }

        if (t == null) {
          if (request.hasRow())
            row = request.getRow();
          if (request.hasFamily())
            family = request.getFamily();
          if (request.hasQualifier())
            qualifier = request.getQualifier();
          if (request.hasValue())
            value = request.getValue();

          try {
           if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: attempting checkAndPut for trans: " + transId +
                                 " startid: " + startId);
           result = checkAndPut(transId, startId,
               request.getRow().toByteArray(),
               request.getFamily().toByteArray(),
               request.getQualifier().toByteArray(),
               request.getValue().toByteArray(),
               put);
           if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndPut returned " + result);
          } catch (Throwable e) {
             if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndPut threw exception after internal checkAndPut");
             if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAnd Put caught exception " + e.getMessage() + "" + stackTraceToString(e));
             t = e;
          }
       }
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndPut setting result: " + result);
       checkAndPutResponseBuilder.setResult(result);
     }
    }
    else
    {
      result = false;
      checkAndPutResponseBuilder.setResult(result);
    }

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  checkAndPut result is " + result);

    checkAndPutResponseBuilder.setHasException(false);

    if (wre != null)
    {
      checkAndPutResponseBuilder.setHasException(true);
      checkAndPutResponseBuilder.setException(wre.toString());
    }

    if (t != null)
    {
      checkAndPutResponseBuilder.setHasException(true);
      checkAndPutResponseBuilder.setException(t.toString());
    }

    SsccCheckAndPutResponse checkAndPutResponse = checkAndPutResponseBuilder.build();

    done.run(checkAndPutResponse);
  }

  /**
   * Processes a transactional checkAndDelete
   * @param long transactionId
   * @param byte[] row
   * @param byte[] family
   * @param byte[] qualifier
   * @param byte[] value
   * @param Delete delete
   * @return boolean
   * @throws IOException
   */
  public boolean checkAndDelete(long transactionId, long startId,
                                byte[] row, byte[] family,
                                byte[] qualifier, byte[] value, Delete delete)
    throws IOException {

    if (LOG.isTraceEnabled()) LOG.trace("Enter SsccRegionEndpoint coprocessor: checkAndDelete, txid: " + transactionId);
    SsccTransactionState state = this.beginTransIfNotExist(transactionId, startId);
    boolean result = false;
    byte[] rsValue = null;

    Get get = new Get(row);
    get.addColumn(family, qualifier);

    Result rs = this.get(transactionId, startId, get);

    boolean valueIsNull = value == null ||
                          value.length == 0;
    int lv_return = 0;

    if (rs.isEmpty() && valueIsNull) {
      lv_return = this.delete(transactionId, startId, delete);
      result = (lv_return == STATELESS_UPDATE_OK) ? true: false;
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndDelete, txid: "
               + transactionId + " rsValue.length == 0, lv_return: " + lv_return + ", result is " + result);

    } else if (!rs.isEmpty() && valueIsNull) {
      rsValue = rs.getValue(family, qualifier);
      if (rsValue != null && rsValue.length == 0) {
        lv_return = this.delete(transactionId, startId, delete);
        result = (lv_return == STATELESS_UPDATE_OK) ? true: false;
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndDelete, txid: "
               + transactionId + " rsValue.length == 0, lv_return: " + lv_return + ", result is: " + result);
      }
      else {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndDelete, txid: "
              + transactionId + " rsValue.length != 0, result is false");
        result = false;
      }
    } else if ((!rs.isEmpty())
              && !valueIsNull
              && (Bytes.equals(rs.getValue(family, qualifier), value))) {
       lv_return = this.delete(transactionId, startId, delete);
       result = (lv_return == STATELESS_UPDATE_OK) ? true : false;
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndDelete, txid: "
               + transactionId + " rsValue matches the row, lv_return is: " + lv_return + ", result is: " + result);
    } else {
      result = false;
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndDelete  setting result is " + result + " row: " + row);
    }

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndDelete  EXIT result is " + result + " row: " + row);

    return result;
  }

  /**
   * Processes a transactional checkAndPut
   * @param long transactionId
   * @param byte[] row
   * @param byte[] family
   * @param byte[] qualifier
   * @param byte[] value
   * @param Put put
   * @return boolean
   * @throws IOException
   */
  public boolean checkAndPut(long transactionId, long startId, byte[] row, byte[] family,
                            byte[] qualifier, byte[] value, Put put)
    throws IOException {

    SsccTransactionState state = this.beginTransIfNotExist(transactionId, startId);
    boolean result = false;
    byte[] rsValue = null;

    Get get = new Get(row);
    get.addColumn(family, qualifier);
    if (LOG.isTraceEnabled()) LOG.trace("Enter SsccRegionEndpoint coprocessor: checkAndPut, txid: " + transactionId +
                          ", startId: " + startId + ", row " + row);

    Result rs = this.get(transactionId, startId, get);

    if (LOG.isTraceEnabled()) LOG.trace("Enter SsccRegionEndpoint coprocessor: checkAndPut, txid: "
               + transactionId + ", result is empty " + rs.isEmpty() +
               ", value is " + Bytes.toString(value));

    boolean valueIsNull = value == null || value.length == 0;
    int lv_return = 0;
    if (rs.isEmpty() && valueIsNull) {
      lv_return = this.put(transactionId, startId, put, false /* stateful */);
      result = (lv_return == STATEFUL_UPDATE_OK) ? true: false;
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndPut, txid: "
              + transactionId + " valueIsNull, lv_return is: " + lv_return + ", result is: " + result);
    } else if (!rs.isEmpty() && valueIsNull) {
      rsValue = rs.getValue(family, qualifier);
      if (rsValue != null && rsValue.length == 0) {
        lv_return = this.put(transactionId, startId, put, false /* stateful */);
        result = (lv_return == STATEFUL_UPDATE_OK) ? true: false;
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndPut, txid: "
               + transactionId + " rsValue.length == 0, lv_return: " + lv_return + ", result is: " + result);
      }
      else {
        result = false;
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndPut, txid: "
              + transactionId + " rsValue.length != 0, result is false");
      }
    } else if ((!rs.isEmpty()) && !valueIsNull
              && (Bytes.equals(rs.getValue(family, qualifier), value))) {
       lv_return = this.put(transactionId, startId, put, false /* stateful */);
       result = (lv_return == STATEFUL_UPDATE_OK) ? true : false;
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndPut, txid: "
               + transactionId + " rsValue matches the row, lv_return is: " + lv_return + ", result is: " + result);

    } else {
      result = false;
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: checkAndPut, txid: "
              + transactionId + " last case, result is false");
    }

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  checkAndPut returns " + result + " for row: " + row);

    return result;
  }

  @Override
  public void deleteMultiple(RpcController controller,
                                SsccDeleteMultipleTransactionalRequest request,
      RpcCallback<SsccDeleteMultipleTransactionalResponse> done) {
    SsccDeleteMultipleTransactionalResponse response = SsccDeleteMultipleTransactionalResponse.getDefaultInstance();

   java.util.List<org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto> results;
   results = request.getDeleteList();
   int resultCount = request.getDeleteCount();
   byte [] row = null;
   Delete delete = null;
   MutationType type;
   Throwable t = null;
   WrongRegionException wre = null;
   int status = 0;
   long transId = request.getTransactionId();
   long startId = request.getStartId();

   if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor deleteMultiple: Entry");

   if (wre == null) {
     for (org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto proto : results)
     {
       delete = null;

       if (proto != null)
       {
         type = proto.getMutateType();

         if (type == MutationType.DELETE && proto.hasRow())
         {
           row = proto.getRow().toByteArray();

           try {
               delete = ProtobufUtil.toDelete(proto);
           } catch (Throwable e) {
             if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor deleteMultiple:delete Caught exception " +
                            "after protobuf conversion " + e.getMessage() + "" + stackTraceToString(e));
             t = e;
           }

           // Process in local memory
           if (delete != null)
           {
             try {
               status = delete(transId, startId, delete);
             } catch (Throwable e) {
               if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor deleteMultiple:delete Caught exception " +
                            "after internal delete " + e.getMessage() + "" + stackTraceToString(e));
             t = e;
             }

             if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: deleteMultiple - id " + transId +
                ", regionName " + regionInfo.getRegionNameAsString() + ", type " + type + ", row " + Bytes.toString(row));

             if (status != STATELESS_UPDATE_OK) {
                String returnString;

                switch (status){
                  case STATEFUL_UPDATE_OK:
                     returnString = new String("STATEFUL_UPDATE_OK");
                     break;
                  case STATEFUL_UPDATE_CONFLICT:
                     returnString = new String("STATEFUL_UPDATE_CONFLICT");
                     break;
                  case STATELESS_UPDATE_OK:
                     returnString = new String("STATELESS_UPDATE_OK");
                     break;
                  case STATELESS_UPDATE_CONFLICT:
                     returnString = new String("STATELESS_UPDATE_CONFLICT");
                     break;
                  default:
                     returnString = new String("Unknown return value: " + Integer.toString(status));
                     break;
                }
                if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:deleteMultiple returned status: " + returnString);
                break;
           }

          }
         }
       }
       else
         if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: deleteMultiple - id " + transId + ", regionName " + regionInfo.getRegionNameAsString() + ", delete proto was null");

      }
    }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccDeleteMultipleTransactionalResponse.Builder deleteMultipleTransactionalResponseBuilder = SsccDeleteMultipleTransactionalResponse.newBuilder();

    deleteMultipleTransactionalResponseBuilder.setHasException(false);

    if (t != null)
    {
      deleteMultipleTransactionalResponseBuilder.setHasException(true);
      deleteMultipleTransactionalResponseBuilder.setException(t.toString());
    }

    if (wre != null)
    {
      deleteMultipleTransactionalResponseBuilder.setHasException(true);
      deleteMultipleTransactionalResponseBuilder.setException(wre.toString());
    }

    deleteMultipleTransactionalResponseBuilder.setStatus(status);
    SsccDeleteMultipleTransactionalResponse dresponse = deleteMultipleTransactionalResponseBuilder.build();

    done.run(dresponse);
  }

  @Override
  public void putMultiple(RpcController controller,
                          SsccPutMultipleTransactionalRequest request,
                          RpcCallback<SsccPutMultipleTransactionalResponse> done) {
    SsccPutMultipleTransactionalResponse response = SsccPutMultipleTransactionalResponse.getDefaultInstance();

   java.util.List<org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto> results;

   boolean stateless = false;
   if (request.hasIsStateless()) {
      stateless = request.getIsStateless();
   }
   if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor putMultiple: Entry, stateless: " + stateless);

   results = request.getPutList();
   int resultCount = request.getPutCount();
   byte [] row = null;
   Put put = null;
   MutationType type;
   Throwable t = null;
   WrongRegionException wre = null;
   int status = 0;
   long transId = request.getTransactionId();
   long startId = request.getStartId();

   if (wre == null){

     for (org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto proto : results)
     {
       put = null;

       if (proto != null)
       {
         type = proto.getMutateType();

         if (type == MutationType.PUT && proto.hasRow())
         {
           row = proto.getRow().toByteArray();

           try {
               if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor putMultiple: converting put");
               put = ProtobufUtil.toPut(proto);
           } catch (Throwable e) {
             if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor putMultiple:put Caught exception " +
                            "after protobuf conversion " + e.getMessage() + "" + stackTraceToString(e));
             t = e;
           }

           // Process in local memory
           if (put != null)
           {
             try {
               status = put(transId, startId, put, stateless);
               if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:putMultiple returned status: " + status);
             } catch (Throwable e) {
                if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor putMultiple:put Caught exception " +
                            "after internal put " + e.getMessage() + "" + stackTraceToString(e));
               t = e;
             }

             if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: putMultiple - id " + transId + ", regionName " + regionInfo.getRegionNameAsString() + ", type " + type + ", row " + Bytes.toString(row));
           }
           else {
             if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: putMultiple - put is null ");
           }
           if (status != STATEFUL_UPDATE_OK) {
              String returnString;

              switch (status){
                case STATEFUL_UPDATE_OK:
                   returnString = new String("STATEFUL_UPDATE_OK");
                   break;
                case STATEFUL_UPDATE_CONFLICT:
                   returnString = new String("STATEFUL_UPDATE_CONFLICT");
                   break;
                case STATELESS_UPDATE_OK:
                   returnString = new String("STATELESS_UPDATE_OK");
                   break;
                case STATELESS_UPDATE_CONFLICT:
                   returnString = new String("STATELESS_UPDATE_CONFLICT");
                   break;
                default:
                   returnString = new String("Unknown return value: " + Integer.toString(status));
                   break;
              }
              if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:putMultiple returned status: " + returnString);
              break;
           }
         }
       }
        else
         if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: putMultiple - id " + transId + ", regionName " + regionInfo.getRegionNameAsString() + ", proto was null");

      }
    }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccPutMultipleTransactionalResponse.Builder putMultipleTransactionalResponseBuilder = SsccPutMultipleTransactionalResponse.newBuilder();

    putMultipleTransactionalResponseBuilder.setHasException(false);

    if (t != null)
    {
      putMultipleTransactionalResponseBuilder.setHasException(true);
      putMultipleTransactionalResponseBuilder.setException(t.toString());
    }

    if (wre != null)
    {
      putMultipleTransactionalResponseBuilder.setHasException(true);
      putMultipleTransactionalResponseBuilder.setException(wre.toString());
    }

    putMultipleTransactionalResponseBuilder.setStatus(status);
    SsccPutMultipleTransactionalResponse pmresponse = putMultipleTransactionalResponseBuilder.build();
    done.run(pmresponse);
  }

  //****************************************************************
  //
  //  ANYTHING ABOVE THIS LINE HAS BEEN CONVERTED FOR SSCC
  //
  //  ANYTHING BELOW THIS COMMENT HAS NOT BEEN CONVERTED FOR SSCC
  //  AND IS LEFT HERE FOR REFERENCE ONLY.  WILL BE REMOVED AFTER POC
  //
  //****************************************************************

  // TrxRegionService methods




  @Override
  public void recoveryRequest(RpcController controller,
                              SsccRecoveryRequestRequest request,
                              RpcCallback<SsccRecoveryRequestResponse> done) {
      int tmId = request.getTmId();
      Throwable t = null;
      WrongRegionException wre = null;
      long transId = request.getTransactionId();
      long startId = request.getStartId();

      if (reconstructIndoubts == 0) {
         if(LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  RECOV recovery Request");
//         try {
            constructIndoubtTransactions();
//         }
//         catch (IdTmException e){
//            LOG.error("SsccRegionEndpoint coprocessor:  RECOV recovery Request exception " + e);
//            t = new IOException("SsccRegionEndpoint coprocessor:  RECOV recovery Request exception " + e);
//         }
//         catch (Exception e2){
//            LOG.error("SsccRegionEndpoint coprocessor:  RECOV recovery Request exception " + e2);
//            t = new IOException("SsccRegionEndpoint coprocessor:  RECOV recovery Request exception " + e2);
//         }

      }

      // Placeholder for real work when recovery is added
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: recoveryResponse - id " + transId + ", regionName " + regionInfo.getRegionNameAsString() + ", tmId" + tmId);

      org.apache.hadoop.hbase.coprocessor.transactional.generated.SsccRegionProtos.SsccRecoveryRequestResponse.Builder recoveryResponseBuilder = SsccRecoveryRequestResponse.newBuilder();

      List<Long> indoubtTransactions = new ArrayList<Long>();
      if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery: region " + regionInfo.getEncodedName() + " receives recovery request from TM " + tmId  + " with region state " + regionState);
      switch(regionState) {
              case 1: // INIT, assume open the TRegion if necessary
                     regionState = 1;  //Note. ??? should we call openHRegion directly here
                     break;
              case 0: // RECOVERING, already create a list of in-doubt txn, but still in the state of resolving them,
                           // retrieve all in-doubt txn from rmid and return them into a long a
                    for (Entry<Long, WALEdit> entry : indoubtTransactionsById.entrySet()) {
                          long tid = entry.getKey();
                          if ((int) (tid >> 32) == tmId) {
                              indoubtTransactions.add(tid);
                              if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery: region " + regionInfo.getEncodedName() + " in-doubt transaction " + tid + " has been added into the recovery repply to TM " + tmId);
                          }
                     } 
                     break;
              case 2: // START
                     break;
       }

      // Placeholder response forced to zero for now
      for (Long transactionId:indoubtTransactions) {
         recoveryResponseBuilder.addResult(transactionId);
      }
      // Placeholder response forced to zero for now

      recoveryResponseBuilder.setHasException(false);

      if (t != null) 
      {
        recoveryResponseBuilder.setHasException(true);
        recoveryResponseBuilder.setException(t.toString());
      }

      if (wre != null) 
      {
        recoveryResponseBuilder.setHasException(true);
        recoveryResponseBuilder.setException(wre.toString());
      }

      SsccRecoveryRequestResponse rresponse = recoveryResponseBuilder.build();
      done.run(rresponse);
  }

  /**
   * Gives the maximum for a given combination of column qualifier and column
   * family, in the given row range as defined in the Scan object. In its
   * current implementation, it takes one column family and one column qualifier
   * (if provided). In case of null column qualifier, maximum value for the
   * entire column family will be returned.
   */
  @Override
  public void getMax(RpcController controller, SsccTransactionalAggregateRequest request,
    RpcCallback<SsccTransactionalAggregateResponse> done) {
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getMax entry");
    RegionScanner scanner = null;
    SsccTransactionalAggregateResponse response = null;
    T max = null;
    try {
      ColumnInterpreter<T, S, P, Q, R> ci = constructColumnInterpreterFromRequest(request);
      T temp;
      long transactionId = request.getTransactionId();
      long startId = request.getStartId();
      Scan scan = ProtobufUtil.toScan(request.getScan());
      scanner = getScanner(transactionId, startId, scan);
      List<Cell> results = new ArrayList<Cell>();
      byte[] colFamily = scan.getFamilies()[0];
      NavigableSet<byte[]> qualifiers = scan.getFamilyMap().get(colFamily);
      byte[] qualifier = null;
      if (qualifiers != null && !qualifiers.isEmpty()) {
        qualifier = qualifiers.pollFirst();
      }
      // qualifier can be null.
      boolean hasMoreRows = false;
      do {
        hasMoreRows = scanner.next(results);
        for (Cell kv : results) {
          temp = ci.getValue(colFamily, qualifier, kv);
          max = (max == null || (temp != null && ci.compare(temp, max) > 0)) ? temp : max;
        }
        results.clear();
      } while (hasMoreRows);
      if (max != null) {
        SsccTransactionalAggregateResponse.Builder builder = SsccTransactionalAggregateResponse.newBuilder();
        builder.addFirstPart(ci.getProtoForCellType(max).toByteString());
        response = builder.build();
      }
    } catch (IOException e) {
      ResponseConverter.setControllerException(controller, e);
    } finally {
      if (scanner != null) {
        try {
          scanner.close();
        } catch (IOException ignored) {}
      }
    }
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getMax - Maximum from this region is "
        + env.getRegion().getRegionNameAsString() + ": " + max);
    done.run(response);
  }

  /**
   * Gives the minimum for a given combination of column qualifier and column
   * family, in the given row range as defined in the Scan object. In its
   * current implementation, it takes one column family and one column qualifier
   * (if provided). In case of null column qualifier, minimum value for the
   * entire column family will be returned.
   */
  @Override
  public void getMin(RpcController controller, SsccTransactionalAggregateRequest request,
      RpcCallback<SsccTransactionalAggregateResponse> done) {
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getMin entry");
    SsccTransactionalAggregateResponse response = null;
    RegionScanner scanner = null;
    T min = null;
    try {
      ColumnInterpreter<T, S, P, Q, R> ci = constructColumnInterpreterFromRequest(request);
      T temp;
      Scan scan = ProtobufUtil.toScan(request.getScan());
      long transactionId = request.getTransactionId();
      long startId = request.getStartId();
      scanner = getScanner(transactionId, startId, scan);
      List<Cell> results = new ArrayList<Cell>();
      byte[] colFamily = scan.getFamilies()[0];
      NavigableSet<byte[]> qualifiers = scan.getFamilyMap().get(colFamily);
      byte[] qualifier = null;
      if (qualifiers != null && !qualifiers.isEmpty()) {
        qualifier = qualifiers.pollFirst();
      }
      boolean hasMoreRows = false;
      do {
        hasMoreRows = scanner.next(results);
        for (Cell kv : results) {
          temp = ci.getValue(colFamily, qualifier, kv);
          min = (min == null || (temp != null && ci.compare(temp, min) < 0)) ? temp : min;
        }
        results.clear();
      } while (hasMoreRows);
      if (min != null) {
        response = SsccTransactionalAggregateResponse.newBuilder().addFirstPart( 
          ci.getProtoForCellType(min).toByteString()).build();
      }
    } catch (IOException e) {
      ResponseConverter.setControllerException(controller, e);
    } finally {
      if (scanner != null) {
        try {
          scanner.close();
        } catch (IOException ignored) {}
      }
    }
    if (LOG.isTraceEnabled()) LOG.trace("Minimum from this region is "
        + env.getRegion().getRegionNameAsString() + ": " + min);
    done.run(response);
  }

  /**
   * Gives the sum for a given combination of column qualifier and column
   * family, in the given row range as defined in the Scan object. In its
   * current implementation, it takes one column family and one column qualifier
   * (if provided). In case of null column qualifier, sum for the entire column
   * family will be returned.
   */
  @Override
  public void getSum(RpcController controller, SsccTransactionalAggregateRequest request,
      RpcCallback<SsccTransactionalAggregateResponse> done) {
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getSum entry");
    SsccTransactionalAggregateResponse response = null;
    RegionScanner scanner = null;
    long sum = 0L;
    try {
      ColumnInterpreter<T, S, P, Q, R> ci = constructColumnInterpreterFromRequest(request);
      S sumVal = null;
      T temp;
      Scan scan = ProtobufUtil.toScan(request.getScan());
      long transactionId = request.getTransactionId();
      long startId = request.getStartId();
      scanner = getScanner(transactionId, startId, scan);
      SsccTransactionState state = this.beginTransIfNotExist(transactionId, startId);
      byte[] colFamily = scan.getFamilies()[0];
      NavigableSet<byte[]> qualifiers = scan.getFamilyMap().get(colFamily);
      byte[] qualifier = null;
      if (qualifiers != null && !qualifiers.isEmpty()) {
        qualifier = qualifiers.pollFirst();
      }
      List<Cell> results = new ArrayList<Cell>();
      boolean hasMoreRows = false;
      boolean firstCell;
      do {
        hasMoreRows = scanner.next(results);
        firstCell=true;
        Result verResult = null;
        Result statusResult = null;
        Result colResult = null;
        for (Cell c : results) {
           if(firstCell == true){
              if(CellUtil.cloneFamily(c) != DtmConst.TRANSACTION_META_FAMILY){
                 //get the statusList
                 Get statusGet = new Get(c.getRow()); //TODO: deprecated API
                 if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getSum first row:  " + c.getRow());
                 //statusGet.setTimeStamp(startId);
                 statusGet.addColumn(DtmConst.TRANSACTION_META_FAMILY,SsccConst.STATUS_COL);
                 statusGet.setMaxVersions(DtmConst.SSCC_MAX_VERSION);
                 statusResult = m_Region.get(statusGet);

                 //get the colList
                 Get colGet = new Get(c.getRow()); //TODO: deprecated API
                 //colGet.setTimeStamp(startId);
                 colGet.addColumn(DtmConst.TRANSACTION_META_FAMILY,SsccConst.COLUMNS_COL);
                 colGet.setMaxVersions(DtmConst.SSCC_MAX_VERSION);
                 colResult = m_Region.get(colGet);

                 //get the versionList
                 Get verGet = new Get(c.getRow());//TODO: deprecated API
                 //verGet.setTimeStamp(startId);
                 verGet.addColumn(DtmConst.TRANSACTION_META_FAMILY,SsccConst.VERSION_COL);
                 verGet.setMaxVersions(DtmConst.SSCC_MAX_VERSION);
                 verResult = m_Region.get(verGet);
                 firstCell = false;
              }

              if(firstCell == false) {

                 temp = ci.getValue(colFamily, qualifier, c);
                 if (temp != null) {
                    if (state.handleResult(c,statusResult.listCells(),verResult.listCells(),colResult.listCells(),transactionId) == true) {
                       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getSum adding cell: " + c.getRow());
                       sumVal = ci.add(sumVal, ci.castToReturnType(temp));
                       break;
                    }
                 }
              }
           }
        }
        results.clear();
      } while (hasMoreRows);
      if (sumVal != null) {
        response = SsccTransactionalAggregateResponse.newBuilder().addFirstPart( 
          ci.getProtoForPromotedType(sumVal).toByteString()).build();
      }
    } catch (IOException e) {
      ResponseConverter.setControllerException(controller, e);
    } finally {
      if (scanner != null) {
        try {
          scanner.close();
        } catch (IOException ignored) {}
      }
    }
    if (LOG.isTraceEnabled()) LOG.trace("Sum from this region is "
        + env.getRegion().getRegionNameAsString() + ": " + sum);
    done.run(response);
  }

  /**
   * Gives the row count for the given column family and column qualifier, in
   * the given row range as defined in the Scan object.
   * @throws IOException
   */
  @Override
  public void getRowNum(RpcController controller, SsccTransactionalAggregateRequest request,
      RpcCallback<SsccTransactionalAggregateResponse> done) {
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getRowNum entry");
    SsccTransactionalAggregateResponse response = null;
    long counter = 0L;
    List<Cell> results = new ArrayList<Cell>();
    RegionScanner scanner = null;
    long transactionId = 0L;
    try {
      Scan scan = ProtobufUtil.toScan(request.getScan());
      byte[][] colFamilies = scan.getFamilies();
      byte[] colFamily = colFamilies != null ? colFamilies[0] : null;
      NavigableSet<byte[]> qualifiers = colFamilies != null ?
          scan.getFamilyMap().get(colFamily) : null;
      byte[] qualifier = null;
      if (qualifiers != null && !qualifiers.isEmpty()) {
        qualifier = qualifiers.pollFirst();
      }
      if (scan.getFilter() == null && qualifier == null)
        scan.setFilter(new FirstKeyOnlyFilter());
      transactionId = request.getTransactionId();
      long startId = request.getStartId();
      scanner = getScanner(transactionId, startId, scan);
      SsccTransactionState state = this.beginTransIfNotExist(transactionId, startId);
      boolean hasMoreRows = false;
      boolean firstCell;
      do {
        hasMoreRows = scanner.next(results);
        firstCell=true;
        Result verResult = null;
        Result statusResult = null;
        Result colResult = null;
        for (Cell c : results) {
           if(firstCell == true){
              if(CellUtil.cloneFamily(c) != DtmConst.TRANSACTION_META_FAMILY){
                 //get the statusList
                 Get statusGet = new Get(c.getRow()); //TODO: deprecated API
                 if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getRowNum first row:  " + c.getRow());
                 //statusGet.setTimeStamp(startId);
                 statusGet.addColumn(DtmConst.TRANSACTION_META_FAMILY,SsccConst.STATUS_COL);
                 statusGet.setMaxVersions(DtmConst.SSCC_MAX_VERSION);
                 statusResult = m_Region.get(statusGet);

                 //get the colList
                 Get colGet = new Get(c.getRow()); //TODO: deprecated API
                 //colGet.setTimeStamp(startId);
                 colGet.addColumn(DtmConst.TRANSACTION_META_FAMILY,SsccConst.COLUMNS_COL);
                 colGet.setMaxVersions(DtmConst.SSCC_MAX_VERSION);
                 colResult = m_Region.get(colGet);

                 //get the versionList
                 Get verGet = new Get(c.getRow());//TODO: deprecated API
                 //verGet.setTimeStamp(startId);
                 verGet.addColumn(DtmConst.TRANSACTION_META_FAMILY,SsccConst.VERSION_COL);
                 verGet.setMaxVersions(DtmConst.SSCC_MAX_VERSION);
                 verResult = m_Region.get(verGet);
                 firstCell = false;
              }

              if(firstCell == false) {
                 if (state.handleResult(c,statusResult.listCells(),verResult.listCells(),colResult.listCells(),transactionId) == true) {
                    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getRowNum adding cell: " + c.getRow());
                    counter++;
                    break;
                 }
              }
           }
        }
        results.clear();
      } while (hasMoreRows);
      ByteBuffer bb = ByteBuffer.allocate(8).putLong(counter);
      bb.rewind();
      response = SsccTransactionalAggregateResponse.newBuilder().addFirstPart( 
          ByteString.copyFrom(bb)).build();
    } catch (IOException e) {
      ResponseConverter.setControllerException(controller, e);
    } finally {
      if (scanner != null) {
        try {
          scanner.close();
        } catch (IOException ignored) {}
      }
    }
    if (LOG.isTraceEnabled()) LOG.trace("Row counter for transactionId " + transactionId + " from this region: "
        + env.getRegion().getRegionNameAsString() + " is " + counter);
    done.run(response);
  }

  /**
   * Gives a Pair with first object as Sum and second object as row count,
   * computed for a given combination of column qualifier and column family in
   * the given row range as defined in the Scan object. In its current
   * implementation, it takes one column family and one column qualifier (if
   * provided). In case of null column qualifier, an aggregate sum over all the
   * entire column family will be returned.
   * <p>
   * The average is computed in
   * AggregationClient#avg(byte[], ColumnInterpreter, Scan) by
   * processing results from all regions, so its "ok" to pass sum and a Long
   * type.
   */
  @Override
  public void getAvg(RpcController controller, SsccTransactionalAggregateRequest request,
      RpcCallback<SsccTransactionalAggregateResponse> done) {
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getAvg entry");
    SsccTransactionalAggregateResponse response = null;
    RegionScanner scanner = null;
    try {
      ColumnInterpreter<T, S, P, Q, R> ci = constructColumnInterpreterFromRequest(request);
      S sumVal = null;
      Long rowCountVal = 0L;
      Scan scan = ProtobufUtil.toScan(request.getScan());
      long transactionId = request.getTransactionId();
      long startId = request.getStartId();
      scanner = getScanner(transactionId, startId, scan);
      byte[] colFamily = scan.getFamilies()[0];
      NavigableSet<byte[]> qualifiers = scan.getFamilyMap().get(colFamily);
      byte[] qualifier = null;
      if (qualifiers != null && !qualifiers.isEmpty()) {
        qualifier = qualifiers.pollFirst();
      }
      List<Cell> results = new ArrayList<Cell>();
      boolean hasMoreRows = false;

      do {
        results.clear();
        hasMoreRows = scanner.next(results);
        for (Cell kv : results) {
          sumVal = ci.add(sumVal, ci.castToReturnType(ci.getValue(colFamily,
              qualifier, kv)));
        }
        rowCountVal++;
      } while (hasMoreRows);
      if (sumVal != null) {
        ByteString first = ci.getProtoForPromotedType(sumVal).toByteString();
        SsccTransactionalAggregateResponse.Builder pair = SsccTransactionalAggregateResponse.newBuilder();
        pair.addFirstPart(first);
        ByteBuffer bb = ByteBuffer.allocate(8).putLong(rowCountVal);
        bb.rewind();
        pair.setSecondPart(ByteString.copyFrom(bb));
        response = pair.build();
      }
    } catch (IOException e) {
      ResponseConverter.setControllerException(controller, e);
    } finally {
      if (scanner != null) {
        try {
          scanner.close();
        } catch (IOException ignored) {}
      }
    }
    done.run(response);
  }

  /**
   * Gives a Pair with first object a List containing Sum and sum of squares,
   * and the second object as row count. It is computed for a given combination of
   * column qualifier and column family in the given row range as defined in the
   * Scan object. In its current implementation, it takes one column family and
   * one column qualifier (if provided). The idea is get the value of variance first:
   * the average of the squares less the square of the average a standard
   * deviation is square root of variance.
   */
  @Override
  public void getStd(RpcController controller, SsccTransactionalAggregateRequest request,
      RpcCallback<SsccTransactionalAggregateResponse> done) {
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getStd entry");
    RegionScanner scanner = null;
    SsccTransactionalAggregateResponse response = null;
    try {
      ColumnInterpreter<T, S, P, Q, R> ci = constructColumnInterpreterFromRequest(request);
      S sumVal = null, sumSqVal = null, tempVal = null;
      long rowCountVal = 0L;
      Scan scan = ProtobufUtil.toScan(request.getScan());
      long transactionId = request.getTransactionId();
      long startId = request.getStartId();
      scanner = getScanner(transactionId, startId, scan);
      byte[] colFamily = scan.getFamilies()[0];
      NavigableSet<byte[]> qualifiers = scan.getFamilyMap().get(colFamily);
      byte[] qualifier = null;
      if (qualifiers != null && !qualifiers.isEmpty()) {
        qualifier = qualifiers.pollFirst();
      }
      List<Cell> results = new ArrayList<Cell>();

      boolean hasMoreRows = false;

      do {
        tempVal = null;
        hasMoreRows = scanner.next(results);
        for (Cell kv : results) {
          tempVal = ci.add(tempVal, ci.castToReturnType(ci.getValue(colFamily,
              qualifier, kv)));
        }
        results.clear();
        sumVal = ci.add(sumVal, tempVal);
        sumSqVal = ci.add(sumSqVal, ci.multiply(tempVal, tempVal));
        rowCountVal++;
      } while (hasMoreRows);
      if (sumVal != null) {
        ByteString first_sumVal = ci.getProtoForPromotedType(sumVal).toByteString();
        ByteString first_sumSqVal = ci.getProtoForPromotedType(sumSqVal).toByteString();
        SsccTransactionalAggregateResponse.Builder pair = SsccTransactionalAggregateResponse.newBuilder();
        pair.addFirstPart(first_sumVal);
        pair.addFirstPart(first_sumSqVal);
        ByteBuffer bb = ByteBuffer.allocate(8).putLong(rowCountVal);
        bb.rewind();
        pair.setSecondPart(ByteString.copyFrom(bb));
        response = pair.build();
      }
    } catch (IOException e) {
      ResponseConverter.setControllerException(controller, e);
    } finally {
      if (scanner != null) {
        try {
          scanner.close();
        } catch (IOException ignored) {}
      }
    }
    done.run(response);
  }

  /**
   * Gives a List containing sum of values and sum of weights.
   * It is computed for the combination of column
   * family and column qualifier(s) in the given row range as defined in the
   * Scan object. In its current implementation, it takes one column family and
   * two column qualifiers. The first qualifier is for values column and 
   * the second qualifier (optional) is for weight column.
   */
  @Override
  public void getMedian(RpcController controller, SsccTransactionalAggregateRequest request,
      RpcCallback<SsccTransactionalAggregateResponse> done) {
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: getMedian entry");
    SsccTransactionalAggregateResponse response = null;
    RegionScanner scanner = null;
    try {
      ColumnInterpreter<T, S, P, Q, R> ci = constructColumnInterpreterFromRequest(request);
      S sumVal = null, sumWeights = null, tempVal = null, tempWeight = null;
      Scan scan = ProtobufUtil.toScan(request.getScan());
      long transactionId = request.getTransactionId();
      long startId = request.getStartId();
      scanner = getScanner(transactionId, startId, scan);
      byte[] colFamily = scan.getFamilies()[0];
      NavigableSet<byte[]> qualifiers = scan.getFamilyMap().get(colFamily);
      byte[] valQualifier = null, weightQualifier = null;
      if (qualifiers != null && !qualifiers.isEmpty()) {
        valQualifier = qualifiers.pollFirst();
        // if weighted median is requested, get qualifier for the weight column
        weightQualifier = qualifiers.pollLast();
      }
      List<Cell> results = new ArrayList<Cell>();

      boolean hasMoreRows = false;

      do {
        tempVal = null;
        tempWeight = null;
        hasMoreRows = scanner.next(results);
        for (Cell kv : results) {
          tempVal = ci.add(tempVal, ci.castToReturnType(ci.getValue(colFamily,
              valQualifier, kv)));
          if (weightQualifier != null) {
            tempWeight = ci.add(tempWeight,
                ci.castToReturnType(ci.getValue(colFamily, weightQualifier, kv)));
          }
        }
        results.clear();
        sumVal = ci.add(sumVal, tempVal);
        sumWeights = ci.add(sumWeights, tempWeight);
      } while (hasMoreRows);
      ByteString first_sumVal = ci.getProtoForPromotedType(sumVal).toByteString();
      S s = sumWeights == null ? ci.castToReturnType(ci.getMinValue()) : sumWeights;
      ByteString first_sumWeights = ci.getProtoForPromotedType(s).toByteString();
      SsccTransactionalAggregateResponse.Builder pair = SsccTransactionalAggregateResponse.newBuilder();
      pair.addFirstPart(first_sumVal);
      pair.addFirstPart(first_sumWeights); 
      response = pair.build();
    } catch (IOException e) {
      ResponseConverter.setControllerException(controller, e);
    } finally {
      if (scanner != null) {
        try {
          scanner.close();
        } catch (IOException ignored) {}
      }
    }
    done.run(response);
  }

  @SuppressWarnings("unchecked")
  ColumnInterpreter<T,S,P,Q,R> constructColumnInterpreterFromRequest(
      SsccTransactionalAggregateRequest request) throws IOException {
    String className = request.getInterpreterClassName();
    Class<?> cls;
    try {
      cls = Class.forName(className);
      ColumnInterpreter<T,S,P,Q,R> ci = (ColumnInterpreter<T, S, P, Q, R>) cls.newInstance();
      if (request.hasInterpreterSpecificBytes()) {
        ByteString b = request.getInterpreterSpecificBytes();
        P initMsg = ProtobufUtil.getParsedGenericInstance(ci.getClass(), 2, b);
        ci.initialize(initMsg);
      }
      return ci;
    } catch (ClassNotFoundException e) {
      throw new IOException(e);
    } catch (InstantiationException e) {
      throw new IOException(e);
    } catch (IllegalAccessException e) {
      throw new IOException(e);
    }
  }

  @Override
  public Service getService() {
    return this;
  }

  /**
   * Stores a reference to the coprocessor environment provided by the
   * {@link org.apache.hadoop.hbase.regionserver.RegionCoprocessorHost} 
   * from the region where this coprocessor is loaded.
   * Since this is a coprocessor endpoint, it always expects to be loaded
   * on a table region, so always expects this to be an instance of
   * {@link RegionCoprocessorEnvironment}.
   * @param env the environment provided by the coprocessor host
   * @throws IOException if the provided environment is not an instance of
   * {@code RegionCoprocessorEnvironment}
   */
  @Override
  public void start(CoprocessorEnvironment env) throws IOException {
    if (env instanceof RegionCoprocessorEnvironment) {
      this.env = (RegionCoprocessorEnvironment)env;
    } else {
      throw new CoprocessorException("SsccRegionEndpoint coprocessor: start - Must be loaded on a table region!");
    }
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: start");
    RegionCoprocessorEnvironment tmp_env = 
      (RegionCoprocessorEnvironment)env;
    this.m_Region =
       tmp_env.getRegion();
    this.regionInfo = this.m_Region.getRegionInfo();
    this.t_Region = (TransactionalRegion) tmp_env.getRegion();
    this.fs = this.m_Region.getFilesystem();

    org.apache.hadoop.conf.Configuration conf = new org.apache.hadoop.conf.Configuration(); 

    synchronized (stoppableLock) {
      try {
        this.transactionLeaseTimeout = HBaseConfiguration.getInt(conf,
          HConstants.HBASE_CLIENT_SCANNER_TIMEOUT_PERIOD,
          HConstants.HBASE_REGIONSERVER_LEASE_PERIOD_KEY,
          DEFAULT_LEASE_TIME);

        this.scannerLeaseTimeoutPeriod = HBaseConfiguration.getInt(conf,
          HConstants.HBASE_CLIENT_SCANNER_TIMEOUT_PERIOD,
          HConstants.HBASE_REGIONSERVER_LEASE_PERIOD_KEY,
          HConstants.DEFAULT_HBASE_CLIENT_SCANNER_TIMEOUT_PERIOD);

        scannerThreadWakeFrequency = conf.getInt(HConstants.THREAD_WAKE_FREQUENCY, 10 * 1000);

        this.cleanTimer = conf.getInt(SLEEP_CONF, DEFAULT_SLEEP);

        if (this.transactionLeases == null)
           this.transactionLeases = new Leases(LEASE_CHECK_FREQUENCY);

        if (LOG.isTraceEnabled()) LOG.trace("Transaction lease time: " + transactionLeaseTimeout);
        if (LOG.isTraceEnabled()) LOG.trace("Scanner lease time: " + scannerThreadWakeFrequency);

        UncaughtExceptionHandler handler = new UncaughtExceptionHandler() {

          public void uncaughtException(final Thread t, final Throwable e)
          {
            LOG.fatal("CleanOldTransactionChore uncaughtException: " + t.getName(), e);
          }
        };

        String n = Thread.currentThread().getName();

        if (TransactionalLeasesThread == null) {
           TransactionalLeasesThread = new Thread(this.transactionLeases);
           if (TransactionalLeasesThread != null) {
              Threads.setDaemonThreadRunning(TransactionalLeasesThread, "Transactional leases");
           }
        }
      } catch (Exception e) {
        throw new CoprocessorException("SsccRegionEndpoint coprocessor: start threw exception " + e);
      }
    }

    this.t_Region = (TransactionalRegion) tmp_env.getRegion();
    this.fs = this.m_Region.getFilesystem();
    tHLog = this.m_Region.getLog();

    RegionServerServices rss = tmp_env.getRegionServerServices();
    ServerName sn = rss.getServerName();
    lv_hostName = sn.getHostname();
    lv_port = sn.getPort();
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: Hostname " + lv_hostName + " port " + lv_port);
    this.regionInfo = this.m_Region.getRegionInfo();
    this.nextLogSequenceId = this.m_Region.getSequenceId();
    this.t_Region = (TransactionalRegion) tmp_env.getRegion();
    zkw1 = rss.getZooKeeper();

    this.configuredEarlyLogging = conf.getBoolean("hbase.regionserver.region.transactional.earlylogging", false);

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: get the reference from Region CoprocessorEnvrionment ");

    if (tmp_env.getSharedData().isEmpty())
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: shared map is empty ");
    else
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: shared map is NOT empty Yes ... ");

    transactionsByIdTestz = TrxRegionObserver.getRefMap();

    if (transactionsByIdTestz.isEmpty())
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: reference map is empty ");
    else
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: reference map is NOT empty Yes ... ");

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: UUU Region " + this.m_Region.getRegionNameAsString() + " check indoubt list from reference map ");

    indoubtTransactionsById = (TreeMap<Long, WALEdit>)transactionsByIdTestz.get(
                               this.m_Region.getRegionNameAsString()+TrxRegionObserver.trxkeypendingTransactionsById);

    indoubtTransactionsCountByTmid = (TreeMap<Integer,Integer>)transactionsByIdTestz.get(
                               this.m_Region.getRegionNameAsString()+TrxRegionObserver.trxkeyindoubtTransactionsCountByTmid);
    if (indoubtTransactionsCountByTmid != null) {
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:OOO successfully get the reference from Region CoprocessorEnvrionment ");
    }
    try {
       idServer = new IdTm(false);
    }
    catch (Exception e){
       LOG.error("SsccRegionEndpoint coprocessor:  unble to new IdTm " + e);
    }
    long logSeqId = nextLogSequenceId.get();
    long currentTime = System.currentTimeMillis();
    long ssccSeqId = currentTime > logSeqId ? currentTime : logSeqId;
    nextSsccSequenceId = new AtomicLong(ssccSeqId);
    LOG.info("Generate SequenceID start from " + nextSsccSequenceId);

    LOG.info("SsccRegionEndpoint coprocessor: start");
  }

  @Override
  public void stop(CoprocessorEnvironment env) throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: stop ");
    stoppable.stop("stop() SsccRegionEndpoint");
  }

  // Internal support methods

  /**
   * Checks if the region is closing
   * @param long transactionId
   * @return String 
   * @throws IOException 
   */
  private void checkClosing(final long transactionId) throws IOException {
    if (closing) {
      LOG.error("SsccRegionEndpoint coprocessor:  Trafodion Recovery: checkClosing(" + transactionId + ") - raising exception. no more transaction allowed.");
      throw new IOException("closing region, no more transaction allowed");
    }
  }

  public void deleteRecoveryzNode(int node, String encodedName) throws IOException {

       synchronized(zkRecoveryCheckLock) {
         // default zNodePath
         String zNodeKey = lv_hostName + "," + lv_port + "," + encodedName;

         StringBuilder sb = new StringBuilder();
         sb.append("TM");
         sb.append(node);
         String str = sb.toString();
         String zNodePathTM = zNodePath + str;
         String zNodePathTMKey = zNodePathTM + "/" + zNodeKey;
         if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: ZKW Delete region recovery znode" + node + " zNode Path " + zNodePathTMKey);
          // delete zookeeper recovery zNode, call ZK ...
         try {
             ZKUtil.deleteNodeFailSilent(zkw1, zNodePathTMKey);
          } catch (KeeperException e) {
            throw new IOException("Trafodion Recovery Region Observer CP: ZKW Unable to delete recovery zNode to TM " + node, e);
          }
       }
  } // end of deleteRecoveryzNode

  /**
   * Starts the region after a recovery
   */
  public void startRegionAfterRecovery() throws IOException {
    boolean isFlush = false;

    try {
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Trafodion Recovery:  Flushing cache in startRegionAfterRecovery " + m_Region.getRegionInfo().getRegionNameAsString());
          m_Region.flushcache();
          //if (!m_Region.flushcache().isFlushSucceeded()) { 
          //   LOG.trace("SsccRegionEndpoint coprocessor:  Trafodion Recovery:  Flushcache returns false !!! " + m_Region.getRegionInfo().getRegionNameAsString());
          //}
     } catch (IOException e) {
     LOG.error("SsccRegionEndpoint coprocessor:  Trafodion Recovery: Flush failed after replay edits" + m_Region.getRegionInfo().getRegionNameAsString());
     return;
     }


    //FileSystem fileSystem = m_Region.getFilesystem();
    //Path archiveTHLog = new Path (recoveryTrxPath.getParent(),"archivethlogfile.log");
    //if (fileSystem.exists(archiveTHLog)) fileSystem.delete(archiveTHLog, true);
    //if (fileSystem.exists(recoveryTrxPath))fileSystem.rename(recoveryTrxPath,archiveTHLog);
    if (indoubtTransactionsById != null)
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Trafodion Recovery: region " + recoveryTrxPath + " has " + indoubtTransactionsById.size() + " in-doubt transactions and edits are archived.");
    else
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Trafodion Recovery: region " + recoveryTrxPath + " has 0 in-doubt transactions and edits are archived.");
    regionState = 2; 
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  Trafodion Recovery: region " + m_Region.getRegionInfo().getEncodedName() + " is STARTED.");
  }

  /**
   * Rssolves the transaction from the log
   * @param SsccTransactionState transactionState
   * @throws IOException 
   */
  private void resolveTransactionFromLog(final SsccTransactionState transactionState) throws IOException {
    LOG.error("SsccRegionEndpoint coprocessor:  Global transaction log is not Implemented. (Optimisticly) assuming transaction commit!");
    commit(transactionState);
  }

  /**
   * TransactionLeaseListener
   */
  private class TransactionLeaseListener implements LeaseListener {

   //private final long transactionName;
   private final String transactionName;

   TransactionLeaseListener(final long n) {
     this.transactionName = getTransactionalUniqueId(n);
   }

   public void leaseExpired() {
    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  leaseExpired Transaction [" + this.transactionName
              + "] expired in region ["
              + m_Region.getRegionInfo().getRegionNameAsString() + "]");
   SsccTransactionState s = null;
   synchronized (transactionsById) {
     s = transactionsById.remove(transactionName);
     if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  leaseExpired Removing transaction: " + this.transactionName + " from list");

     if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  leaseExpired Removing transaction: " + this.transactionName + " from list");
   }
   if (s == null) {
     LOG.warn("leaseExpired Unknown transaction expired " + this.transactionName);
     return;
   }
   switch (s.getStatus()) {
     case PENDING:
       s.setStatus(Status.ABORTED);  
       break;
      case COMMIT_PENDING:
       if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: leaseExpired  Transaction " + s.getTransactionId()
                                + " expired in COMMIT_PENDING state");
        String key = getTransactionalUniqueId(s.getTransactionId());
        try {
          if (s.getCommitPendingWaits() > MAX_COMMIT_PENDING_WAITS) {
            if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: leaseExpired  Checking transaction status in transaction log");
            resolveTransactionFromLog(s);
            break;
          }
          if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: leaseExpired  renewing lease and hoping for commit");
          s.incrementCommitPendingWaits();
          synchronized (transactionsById) {
            transactionsById.put(key, s);
            if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: leaseExpired  Adding transaction: " + s.getTransactionId() + " to list");
          }
          try {
            transactionLeases.createLease(key, transactionLeaseTimeout, this);
          } catch (LeaseStillHeldException e) {
            transactionLeases.renewLease(key);
          }
          } catch (IOException e) {
            throw new RuntimeException(e);
          }

          break;

       default:
         LOG.warn("SsccRegionEndpoint coprocessor: leaseExpired  Unexpected status on expired lease");
     }
   }
 }

  /**
   * Processes multiple transactional deletes    
   * @param long transactionId
   * @param Delete[] deletes   
   * @throws IOException 
   */
  public synchronized void delete(long transactionId, long startId, Delete[] deletes)
    throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("Enter SsccRegionEndpoint coprocessor: deletes[], txid: " + transactionId);
    checkClosing(transactionId);

    SsccTransactionState state = this.beginTransIfNotExist(transactionId, startId);

    //for (Delete del : deletes) {
    //  state.addDelete(del);
    //}
  }


  public void constructIndoubtTransactions() /*throws IdTmException*/ {

      synchronized (recoveryCheckLock) {
            if ((indoubtTransactionsById == null) || (indoubtTransactionsById.size() == 0)) {
              if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Endpoint Coprocessor: Region " + regionInfo.getRegionNameAsString() + " has no in-doubt transaction, set region START ");
              regionState = 2; // region is started for transactional access
              reconstructIndoubts = 1; 
              try {
              startRegionAfterRecovery();
               } catch (IOException exp1) {
                    LOG.debug("Trafodion Recovery: flush error during region start");
               }
              return;
            }

            if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Endpoint Coprocessor: Trafodion Recovery RegionObserver to Endpoint coprocessor data exchange test");
            if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Endpoint Coprocessor: try to access indoubt transaction list with size " + indoubtTransactionsById.size());

            if (reconstructIndoubts == 0) {
            //Retrieve (tid,Edits) from indoubt Transaction and construct/add into desired transaction data list
            for (Entry<Long, WALEdit> entry : indoubtTransactionsById.entrySet()) {
                      long transactionId = entry.getKey();
		      String key = String.valueOf(transactionId);
                      if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Endpoint Coprocessor:E11 Region " + regionInfo.getRegionNameAsString() + " process in-doubt transaction " + transactionId);

                      IdTmId seqId;
//                      try {
//                         seqId = new IdTmId();
//                         if (LOG.isDebugEnabled()) LOG.debug("Trafodion Recovery Endpoint Coprocessor:getting new IdTM sequence ");
//                         idServer.id(ID_TM_SERVER_TIMEOUT, seqId);
//                         if (LOG.isDebugEnabled()) LOG.debug("Trafodion Recovery Endpoint Coprocessor: IdTM sequence is " + seqId.val);
//                      } catch (IdTmException exc) {
//                         LOG.error("Trafodion Recovery Endpoint Coprocessor: IdTm threw exception exc " + exc);
//                         throw new IdTmException("Trafodion Recovery Endpoint Coprocessor: IdTm threw exception exc " + exc);
//                      } catch (Exception exc2) {
//                         LOG.error("Trafodion Recovery Endpoint Coprocessor: IdTm threw exception exc2 " + exc2);
//                         throw new IdTmException("Trafodion Recovery Endpoint Coprocessor: IdTm threw exception exc2 " + exc2);
//                      }

                      //TBD Need to get HLOG ???
                      if (LOG.isTraceEnabled()) LOG.trace("constructIndoubtTransactions for transId " + transactionId);
		      SsccTransactionState state = new SsccTransactionState(transactionId, /* 1L my_Region.getLog().getSequenceNumber()*/
                                                                                nextLogSequenceId.getAndIncrement(), nextLogSequenceId, 
                                                                                regionInfo, m_Region.getTableDesc(), tHLog, false,
//                                                                                seqId.val);
                                                                                nextSsccSequenceId.getAndIncrement());
                      if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Endpoint Coprocessor:E22 Region " + regionInfo.getRegionNameAsString() + " create transaction state for " + transactionId);

		      state.setStartSequenceNumber(state.getStartId());
    		      transactionsById.put(getTransactionalUniqueId(transactionId), state);

                      state.setReinstated();
		      state.setStatus(Status.COMMIT_PENDING);
		      commitPendingTransactions.add(state);
//                      try {
//                         idServer.id(ID_TM_SERVER_TIMEOUT, seqId);
//                      } catch (IdTmException exc) {
//                         LOG.error("Trafodion Recovery Endpoint Coprocessor: IdTm threw exception 2 " + exc);
//                      }
//		      state.setSequenceNumber(seqId.val);
                      if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Endpoint setting sequenceNumber to commitId: " + state.getCommitId());
                      state.setSequenceNumber(state.getCommitId());
//		      state.setSequenceNumber(nextSsccSequenceId.getAndIncrement());
		      commitedTransactionsBySequenceNumber.put(state.getSequenceNumber(), state);
                      int tmid = (int) (transactionId >> 32);
                      if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Endpoint Coprocessor:E33 Region " + regionInfo.getRegionNameAsString() + " add prepared " + transactionId + " to TM " + tmid);              
             } // for all txns in indoubt transcation list
             } // not reconstruct indoubtes yet
             reconstructIndoubts = 1;
        } // synchronized
        /* //TBD cleanup lists (this is for testing purpose only)
        LOG.debug("Trafodion Recovery Endpoint Coprocessor:EEE Clean up recovery test for indoubt transactions");
        transactionsById.clear();
        commitPendingTransactions.clear();
        commitedTransactionsBySequenceNumber.clear();
        LOG.debug("Trafodion Recovery Endpoint Coprocessor:EEE Clean up indoubt transactions in data lists");
        */
  }


  /**
   * Obtains a scanner lease id
   * @param long scannerId
   * @return String 
   */
  private String getScannerLeaseId(final long scannerId) {
    String lstring = m_Region.getRegionInfo().getRegionNameAsString() + scannerId;

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor:  getScannerLeaseId -- EXIT txId: " 
             + scannerId + " lease string " + lstring);
    return m_Region.getRegionInfo().getRegionNameAsString() + scannerId;
  }

  /**
   * Obtains a transactional lease id
   * @param long transactionId
   * @return String 
   */
  private String getTransactionalUniqueId(final long transactionId) {

    if (LOG.isTraceEnabled()) {
       String lstring = m_Region.getRegionInfo().getRegionNameAsString() + transactionId;
       LOG.trace("SsccRegionEndpoint coprocessor:  getTransactionalUniqueId -- EXIT txId: "
             + transactionId + " transactionsById size: "
             + transactionsById.size() + " name " + lstring);
    }

    return m_Region.getRegionInfo().getRegionNameAsString() + transactionId;
  }

  /**
   * Formats a cleanup message for a Throwable
   * @param Throwable t
   * @param String msg
   * @return Throwable
   */
  private Throwable cleanup(final Throwable t, final String msg) {
    if (t instanceof NotServingRegionException) {
      if (LOG.isTraceEnabled()) LOG.trace("NotServingRegionException; " +  t.getMessage());
      return t;
    }
    if (msg == null) {
      LOG.error("cleanup message was null");
    } else {
      LOG.error("cleanup message was " + msg);
    }
    return t;
  }

  private IOException convertThrowableToIOE(final Throwable t) {
    return convertThrowableToIOE(t, null);
  }

  /*
   * @param t
   *
   * @param msg Message to put in new IOE if passed <code>t</code>
   * is not an IOE
   *
   * @return Make <code>t</code> an IOE if it isn't already.
   */
   private IOException convertThrowableToIOE(final Throwable t, final String msg) {
     return (t instanceof IOException ? (IOException) t : msg == null
      || msg.length() == 0 ? new IOException(t) : new IOException(msg, t));
  }

  /**
   * Checks if the file system is available       
   * @return boolean
   */
  public boolean checkFileSystem() {
    if (this.fs != null) {
      try {
        FSUtils.checkFileSystemAvailable(this.fs);
      } catch (IOException e) {
        if (LOG.isTraceEnabled()) LOG.trace("File System not available threw IOException " + e.getMessage());
        return false;
      }
    }
    return true;
  }

  /**
   * Prepares the family keys if the scan has no families defined
   * @param Scan scan
   * @throws IOException
   */
  public void prepareScanner(Scan scan) throws IOException {
    if(!scan.hasFamilies()) {
      for(byte[] family: this.m_Region.getTableDesc().getFamiliesKeys()){
           scan.addFamily(family);
      }
    }
  }

  /**
   * Checks if the row is within this region's row range
   * @param byte[] row  
   * @param String op
   * @throws IOException
   */
  public void checkRow(final byte [] row, String op) throws IOException {
    if(!this.m_Region.rowIsInRange(this.regionInfo, row)) {
      throw new WrongRegionException("Requested row out of range for " +
       op + " on HRegion " + this + ", startKey='" +
       Bytes.toStringBinary(this.regionInfo.getStartKey()) + "', getEndKey()='" +
       Bytes.toStringBinary(this.regionInfo.getEndKey()) + "', row='" +
       Bytes.toStringBinary(row) + "'");
    }
  }

  /**
   * Removes the scanner associated with the specified ID from the internal
   * id->scanner TransactionalRegionScannerHolder map
   *
   * @param long scannerId
   * @return a Scanner or throws UnknownScannerException
   * @throws UnknownScannerException
   */
    protected synchronized RegionScanner removeScanner(long scannerId) 
      throws UnknownScannerException {

      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: removeScanner scanners map is " + scanners + ", count is "  + scanners.size());
      TransactionalRegionScannerHolder rsh = 
        scanners.remove(scannerId);
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: removeScanner scanners map is " + scanners + ", count is "  + scanners.size());
      if (rsh != null)
      {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: removeScanner rsh is " + rsh + "rsh.s is"  + rsh.s );
        return rsh.s;
      }
      else
      {
        if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: removeScanner rsh is null");
          throw new UnknownScannerException(
            "ScannerId: " + scannerId + ", already closed?");
      }
    }

  /**
   * Adds a region scanner to the TransactionalRegionScannerHolder map
   * @param RegionScanner s
   * @param HRegion r       
   * @return long 
   * @throws LeaseStillHeldException 
   */
  protected synchronized long addScanner(long transId, RegionScanner s, HRegion r)
     throws LeaseStillHeldException {
    long scannerId = performScannerId.getAndIncrement();

    TransactionalRegionScannerHolder rsh = 
      new TransactionalRegionScannerHolder(transId, scannerId, s,r);

    if (rsh != null)
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: scannerId is " + scannerId + ", addScanner rsh is " + rsh);
    else
      if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: scannerId is " + scannerId + ", addScanner rsh is null");
  
    TransactionalRegionScannerHolder existing =
      scanners.putIfAbsent(scannerId, rsh);

    if (LOG.isTraceEnabled()) LOG.trace("SsccRegionEndpoint coprocessor: addScanner scanners map is " + scanners + ", count is "  + scanners.size());

/*
    scannerLeases.createLease(getScannerLeaseId(scannerId),
                              this.scannerLeaseTimeoutPeriod,
                              new TransactionalScannerListener(scannerId));
*/

    return scannerId;
  }

/**
 *    * Instantiated as a scanner lease. If the lease times out, the scanner is
 *       * closed
 *          */
/*
  private class TransactionalScannerListener implements LeaseListener {
    private final long scannerId;

    TransactionalScannerListener(final long id) {
      this.scannerId = id;
    }

    @Override
    public void leaseExpired() {
      TransactionalRegionScannerHolder rsh = scanners.remove(this.scannerId);
      if (rsh != null) {
        RegionScanner s = rsh.s;
        if (LOG.isTraceEnabled()) LOG.trace("Scanner " + this.scannerId + " lease expired on region "
            + s.getRegionInfo().getRegionNameAsString());
        try {
          HRegion region = rsh.r;

          s.close();
        } catch (IOException e) {
          LOG.error("Closing scanner for "
              + s.getRegionInfo().getRegionNameAsString(), e);
        }
      } else {
        if (LOG.isTraceEnabled()) LOG.trace("Scanner " + this.scannerId + " lease expired");
      }
    }
  }
*/

  /**
   * Formats the throwable stacktrace to a string
   * @param Throwable e
   * @return String 
   */
  public String stackTraceToString(Throwable e) {
    StringBuilder sb = new StringBuilder();
    for (StackTraceElement element : e.getStackTrace()) {
        sb.append(element.toString());
        sb.append("\n");
    }
    return sb.toString();
  }

  /**
   * Returns the Scanner Leases for this coprocessor                       
   * @return Leases 
   */
     //synchronized protected Leases getScannerLeases() {
      //  return this.scannerLeases;
    //}

  /**
   * Returns the Leases for this coprocessor                               
   * @return Leases 
   */
     synchronized protected Leases getTransactionalLeases() {
        return this.transactionLeases;
    }

  /**
   * Removes unneeded committed transactions                               
   */
    synchronized public void removeUnNeededCommitedTransactions() {

      Long minStartSeqNumber = getMinStartSequenceNumber();

      if (minStartSeqNumber == null) {
        minStartSeqNumber = Long.MAX_VALUE;
      }

      int numRemoved = 0;
	 
      synchronized (commitedTransactionsBySequenceNumber) {
      for (Entry<Long, SsccTransactionState> entry : new LinkedList<Entry<Long, SsccTransactionState>>(
        commitedTransactionsBySequenceNumber.entrySet())) {
          if (entry.getKey() >= minStartSeqNumber) {
            break;
	  }
	  numRemoved = numRemoved
			+ (commitedTransactionsBySequenceNumber.remove(entry
			.getKey()) == null ? 0 : 1);
	  numRemoved++;
	}
      }

/*
	StringBuilder traceMessage = new StringBuilder();
	if (numRemoved > 0) {
	  traceMessage.append("Removed [").append(numRemoved)
		      .append("] commited transactions");

          if (minStartSeqNumber == Integer.MAX_VALUE) {
            traceMessage.append(" with any sequence number.");
	  } else {
	    traceMessage.append(" with sequence lower than [")
	                .append(minStartSeqNumber).append("].");
	  }

	  if (!commitedTransactionsBySequenceNumber.isEmpty()) {
	      traceMessage.append(" Still have [")
                          .append(commitedTransactionsBySequenceNumber.size())
                          .append("] left.");
	  } else {
	    traceMessage.append(" None left.");
	  }
	    if (LOG.isTraceEnabled()) LOG.trace(traceMessage.toString());
        } else if (commitedTransactionsBySequenceNumber.size() > 0) {
          traceMessage.append("Could not remove any transactions, and still have ")
		        .append(commitedTransactionsBySequenceNumber.size())
		        .append(" left");
          if (LOG.isTraceEnabled()) LOG.trace(traceMessage.toString());
        }
*/

  }

  /**
   * Returns the minimum start sequence number
   * @return Long
   */
  private Long getMinStartSequenceNumber() {

    List<SsccTransactionState> transactionStates;

    synchronized (transactionsById) {
      transactionStates = new ArrayList<SsccTransactionState>(
      transactionsById.values());
    }

    Long min = null;

    for (SsccTransactionState transactionState : transactionStates) {
      if (min == null || transactionState.getStartSequenceNumber() < min) {
        min = transactionState.getStartSequenceNumber();
      }
    }

    return min;
  }

  /**
   * Returns the region name as a string
   * @return String 
   */
  public String getRegionNameAsString() {
    return this.m_Region.getRegionNameAsString();
  }

/**
 * Simple helper class that just keeps track of whether or not its stopped.
 */
   private static class StoppableImplementation implements Stoppable {
     private volatile boolean stop = false;

     @Override
     public void stop(String why) {
       this.stop = true;
     }

     @Override
     public boolean isStopped() {
       return this.stop;
     }
  }
}

//1}
