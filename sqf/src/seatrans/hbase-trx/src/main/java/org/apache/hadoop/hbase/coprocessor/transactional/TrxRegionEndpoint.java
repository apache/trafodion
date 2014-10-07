/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
import java.util.Map;
import java.util.Map.Entry;
import java.util.NavigableSet;
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

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
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
import org.apache.hadoop.hbase.exceptions.OutOfOrderScannerNextException;
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
import org.apache.hadoop.hbase.regionserver.transactional.TransactionalRegion;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionalRegionScannerHolder;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionState;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionState.TransactionScanner;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionState.WriteAction;
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
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.AbortTransactionRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.AbortTransactionResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.BeginTransactionRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.BeginTransactionResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CloseScannerRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CloseScannerResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitIfPossibleRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitIfPossibleResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequestRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequestResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CheckAndDeleteRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CheckAndDeleteResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CheckAndPutRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CheckAndPutResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.DeleteMultipleTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.DeleteMultipleTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.DeleteTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.DeleteTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.GetTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.GetTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PerformScanRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PerformScanResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.OpenScannerRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.OpenScannerResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PutTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PutTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PutMultipleTransactionalRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PutMultipleTransactionalResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.RecoveryRequestRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.RecoveryRequestResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TransactionalAggregateRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TransactionalAggregateResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TrxRegionService;
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
public class TrxRegionEndpoint<T, S, P extends Message, Q extends Message, R extends Message> extends TrxRegionService implements
CoprocessorService, Coprocessor {

  private static final Log LOG = LogFactory.getLog(TrxRegionEndpoint.class);

  private RegionCoprocessorEnvironment env;

  protected Map<Long, Long> transactionsByIdTest = null;
  ConcurrentHashMap<String, Object> transactionsByIdTestz = null;

  // Collection of active transactions (PENDING) keyed by id.
  protected ConcurrentHashMap<String, TransactionState> transactionsById = new ConcurrentHashMap<String, TransactionState>();

  // Map of recent transactions that are COMMIT_PENDING or COMMITED keyed by
  // their sequence number
  private SortedMap<Integer, TransactionState> commitedTransactionsBySequenceNumber = Collections.synchronizedSortedMap(new TreeMap<Integer, TransactionState>());

  // Collection of transactions that are COMMIT_PENDING
  private Set<TransactionState> commitPendingTransactions = Collections
                        .synchronizedSet(new HashSet<TransactionState>());

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
  private AtomicInteger nextSequenceId = new AtomicInteger(0);

  private Object commitCheckLock = new Object();
  private Object recoveryCheckLock = new Object();
  private Object editReplay = new Object();
  private int reconstructIndoubts = 0; 
  //temporary THLog getSequenceNumber() replacement
  private AtomicLong nextLogSequenceId = new AtomicLong(0);
  private final int oldTransactionFlushTrigger = 0;
  private final Boolean splitDelayEnabled = false;
  private final Boolean doWALHlog = false;
  static Leases transactionLeases = null;
  // Joanie: commenting out scanner leases for now
  //static Leases scannerLeases = null;
  CleanOldTransactionsChore cleanOldTransactionsThread;
  static Stoppable stoppable = new StoppableImplementation();
  private int cleanTimer = 5000; // Five minutes
  private int regionState = 0; 
  private Path recoveryTrxPath = null;
  private int cleanAT = 0;
  private HRegionInfo regionInfo = null;
  private HRegion m_Region = null;
  private TransactionalRegion t_Region = null;
  private FileSystem fs = null;
  private RegionCoprocessorHost rch = null;
  private HLog tHLog = null;
  boolean closing = false;
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

  // TrxRegionService methods
    
  @Override
  public void abortTransaction(RpcController controller,
                                AbortTransactionRequest request,
      RpcCallback<AbortTransactionResponse> done) {
    AbortTransactionResponse response = AbortTransactionResponse.getDefaultInstance();

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: abortTransaction - id " + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString());

    IOException ioe = null;
    UnknownTransactionException ute = null;
    WrongRegionException wre = null;
    Throwable t = null;

    /*  commenting out for the time being
    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();

    // First test if this region matches our region name
    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:abortTransaction threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
    } else 
    */
    {
      // Process in local memory
      try {
        abortTransaction(request.getTransactionId());
      } catch (UnknownTransactionException u) {
        if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor:abort threw UnknownTransactionException after internal abort");
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + u.getMessage() + "" + stackTraceToString(u));
       ute = u;
      } catch (IOException e) {
        if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor:abort threw UnknownTransactionException after internal abort");
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
        ioe = e;
      }
    }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.AbortTransactionResponse.Builder abortTransactionResponseBuilder = AbortTransactionResponse.newBuilder();

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

    AbortTransactionResponse aresponse = abortTransactionResponseBuilder.build();

    done.run(aresponse);
  }

  @Override
  public void beginTransaction(RpcController controller,
                                BeginTransactionRequest request,
      RpcCallback<BeginTransactionResponse> done) {
    BeginTransactionResponse response = BeginTransactionResponse.getDefaultInstance();

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: beginTransaction - id " + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString());

    Throwable t = null;
    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();
    WrongRegionException wre = null;
    // First test if this region matches our region name
    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:beginTransaction threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
     }else {
      try {
        beginTransaction(request.getTransactionId());
      } catch (Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:beginTransaction threw exception after internal begin");
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
         t = e;
      }        
    }        
     
    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.BeginTransactionResponse.Builder beginTransactionResponseBuilder = BeginTransactionResponse.newBuilder();

    beginTransactionResponseBuilder.setHasException(false);

    if (t != null)
    {
      beginTransactionResponseBuilder.setHasException(true);
      beginTransactionResponseBuilder.setException(t.toString());
    }

    if (wre != null)
    {
      beginTransactionResponseBuilder.setHasException(true);
      beginTransactionResponseBuilder.setException(wre.toString());
    }

    BeginTransactionResponse bresponse = beginTransactionResponseBuilder.build();

    done.run(bresponse);
  }

  @Override
  public void commit(RpcController controller,
                     CommitRequest request,
      RpcCallback<CommitResponse> done) {
    CommitResponse response = CommitResponse.getDefaultInstance();

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit - id " + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString());

    Throwable t = null;
    WrongRegionException wre = null;

    /*  commenting out for the time being
    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();
    // First test if this region matches our region name
    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:commit threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
    } else 
    */
    {
     // Process local memory
      try {
        commit(request.getTransactionId());
      } catch (Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:commit threw exception after internal commit");
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
        t = e;
      }
    }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitResponse.Builder commitResponseBuilder = CommitResponse.newBuilder();

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

    CommitResponse cresponse = commitResponseBuilder.build();

    done.run(cresponse);
  }

  @Override
  public void commitIfPossible(RpcController controller,
                                CommitIfPossibleRequest request,
      RpcCallback<CommitIfPossibleResponse> done) {
    CommitIfPossibleResponse response = CommitIfPossibleResponse.getDefaultInstance();

     boolean reply = false;
     Throwable t = null;
    WrongRegionException wre = null;

    /*  commenting out for the time being
    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();
    // First test if this region matches our region name
    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:commitIfPossible threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
     } else 
    */
     {
       // Process local memory
       try {
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitIfPossible - id " + request.getTransactionId() + ", regionName, " + regionInfo.getRegionNameAsString() + "calling internal commitIfPossible");
         reply = commitIfPossible(request.getTransactionId());
       } catch (Throwable e) {
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:commitIfPossible threw exception after internal commitIfPossible");
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
          t = e;
       }
     }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitIfPossibleResponse.Builder commitIfPossibleResponseBuilder = CommitIfPossibleResponse.newBuilder();

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

    CommitIfPossibleResponse cresponse = commitIfPossibleResponseBuilder.build();
    done.run(cresponse);
  }

  @Override
  public void commitRequest(RpcController controller,
                            CommitRequestRequest request,
                            RpcCallback<CommitRequestResponse> done) {

    CommitRequestResponse response = CommitRequestResponse.getDefaultInstance();

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitRequest - id " + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString());

    int status = 0;
    IOException ioe = null;
    UnknownTransactionException ute = null;
    Throwable t = null;
    WrongRegionException wre = null;

    /*  commenting out for the time being
    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();
    // First test if this region matches our region name
    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:commitRequest threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
    } else
	*/
    {
      // Process local memory
      try {
        status = commitRequest(request.getTransactionId());
      } catch (UnknownTransactionException u) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:commitRequest threw exception after internal commit" + u.toString());
        ute = u;
      } catch (IOException e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:commitRequest threw exception after internal commit" + e.toString());
        ioe = e;
      }
    }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequestResponse.Builder commitRequestResponseBuilder = CommitRequestResponse.newBuilder();

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

    CommitRequestResponse cresponse = commitRequestResponseBuilder.build();
    done.run(cresponse);
  }

  @Override
  public void checkAndDelete(RpcController controller,
                          CheckAndDeleteRequest request,
                          RpcCallback<CheckAndDeleteResponse> done) {

    CheckAndDeleteResponse response = CheckAndDeleteResponse.getDefaultInstance();

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
    boolean result = false;

    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();

    // First test if this region matches our region name
    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:checkAndDelete threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
   }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CheckAndDeleteResponse.Builder checkAndDeleteResponseBuilder = CheckAndDeleteResponse.newBuilder();

    if (wre == null && type == MutationType.DELETE && proto.hasRow())
    {
      rowArray = proto.getRow().toByteArray();

      try {
          delete = ProtobufUtil.toDelete(proto);
      } catch (Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndDelete caught exception " + e.getMessage() + "" + stackTraceToString(e));
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
           result = checkAndDelete(request.getTransactionId(),
               request.getRow().toByteArray(),
               request.getFamily().toByteArray(),
               request.getQualifier().toByteArray(),
               request.getValue().toByteArray(),
               delete);
           } catch (Throwable e) {
             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndDelete caught exception " + e.getMessage() + "" + stackTraceToString(e));
             t = e;
           }
         }

       checkAndDeleteResponseBuilder.setResult(result);
     }
    }
    else
    {
      result = false;
      checkAndDeleteResponseBuilder.setResult(result);
    }

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  checkAndDelete result is " + result);
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


    CheckAndDeleteResponse checkAndDeleteResponse = checkAndDeleteResponseBuilder.build();

    done.run(checkAndDeleteResponse);
  }

  @Override
  public void checkAndPut(RpcController controller,
                          CheckAndPutRequest request,
                          RpcCallback<CheckAndPutResponse> done) {

    CheckAndPutResponse response = CheckAndPutResponse.getDefaultInstance();

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

    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();

    // First test if this region matches our region name
    if (!name.equals(regionInfo.getRegionNameAsString())) {
      wre =  new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:checkAndPut threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
   }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CheckAndPutResponse.Builder checkAndPutResponseBuilder = CheckAndPutResponse.newBuilder();

    if (wre == null && type == MutationType.PUT && proto.hasRow())
    {
      rowArray = proto.getRow().toByteArray();

      try {
          put = ProtobufUtil.toPut(proto);
      } catch (Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndPut caught exception " + e.getMessage() + "" + stackTraceToString(e));
        t = e;
      }

      // Process in local memory
      if (put != null)
      {
        if (request.hasRow()) {
          row = request.getRow();

        if (!Bytes.equals(rowArray, request.getRow().toByteArray()))
          t = new org.apache.hadoop.hbase.DoNotRetryIOException("Action's " +
          "Put row must match the passed row");
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
           result = checkAndPut(request.getTransactionId(),
               request.getRow().toByteArray(),
               request.getFamily().toByteArray(),
               request.getQualifier().toByteArray(),
               request.getValue().toByteArray(),
               put);
           } catch (Throwable e) {
             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndPut threw exception after internal checkAndPut");
             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAnd Put caught exception " + e.getMessage() + "" + stackTraceToString(e));
             t = e;
           }
         }

       checkAndPutResponseBuilder.setResult(result);
     }
    }
    else
    {
      result = false;
      checkAndPutResponseBuilder.setResult(result);
    }

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  checkAndPut result is " + result);

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

    CheckAndPutResponse checkAndPutResponse = checkAndPutResponseBuilder.build();

    done.run(checkAndPutResponse);
  }

  @Override
  public void closeScanner(RpcController controller,
                           CloseScannerRequest request,
                           RpcCallback<CloseScannerResponse> done) {

    RegionScanner scanner = null;
    Throwable t = null;
    WrongRegionException wre = null;
    Exception ce = null;

    long scannerId = request.getScannerId();

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: closeScanner - id " + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString() +
", scannerId " + scannerId);

    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();

    // First test if this region matches our region name
    /*
    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: closeScanner threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
    } else {
    */
      try {
         scanner = removeScanner(scannerId);

         if (scanner != null) { 
             scanner.close();
         }
         else
           if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  closeScanner scanner was null for scannerId " + scannerId);

/*
         try {
           scannerLeases.cancelLease(getScannerLeaseId(scannerId));
         } catch (LeaseException le) {
           // ignore
           if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: closeScanner failed to get a lease " + scannerId);
         }
*/

      } catch(Exception e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  closeScanner caught exception " + e.getMessage() + "" + stackTraceToString(e));
        ce = e;
      } catch(Throwable e) {
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: closeScanner - id Caught exception  " + e.getMessage() + " " + stackTraceToString(e));
         t = e;
      }

    //Commenting out for now
    //}

    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CloseScannerResponse.Builder closeResponseBuilder = CloseScannerResponse.newBuilder();

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

    CloseScannerResponse cresponse = closeResponseBuilder.build();
    done.run(cresponse);
  }

  @Override
  public void deleteMultiple(RpcController controller,
                                DeleteMultipleTransactionalRequest request,
      RpcCallback<DeleteMultipleTransactionalResponse> done) {
    DeleteMultipleTransactionalResponse response = DeleteMultipleTransactionalResponse.getDefaultInstance();

   java.util.List<org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto> results;
   results = request.getDeleteList();
   int resultCount = request.getDeleteCount();
   byte [] row = null;
   Delete delete = null;
   MutationType type;
   Throwable t = null;
   WrongRegionException wre = null;

    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();

    // First test if this region matches our region name
    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:deleteMultiple threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
   } 

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
             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:delete threw exception after protobuf conversion delete");
             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
             t = e;
           }

           // Process in local memory
           if (delete != null)
           {
             try {
               delete(request.getTransactionId(), delete);
             } catch (Throwable e) {
               if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:delete threw exception after internal delete");
               if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
             t = e;
             }

             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: deleteMultiple - id " + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString() + ", type " + type + ", row " + Bytes.toString(row));
           }
         }
       }
       else
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: deleteMultiple - id " + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString() + ", delete proto was null");

      }
    }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.DeleteMultipleTransactionalResponse.Builder deleteMultipleTransactionalResponseBuilder = DeleteMultipleTransactionalResponse.newBuilder();

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

    DeleteMultipleTransactionalResponse dresponse = deleteMultipleTransactionalResponseBuilder.build();
      
    done.run(dresponse);
  }

  @Override
  public void delete(RpcController controller,
                                DeleteTransactionalRequest request,
      RpcCallback<DeleteTransactionalResponse> done) {
    DeleteTransactionalResponse response = DeleteTransactionalResponse.getDefaultInstance();

    byte [] row = null;
    MutationProto proto = request.getDelete();
    MutationType type = proto.getMutateType();
    Delete delete = null;
    Throwable t = null;
   WrongRegionException wre = null;

    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();

    // First test if this region matches our region name
    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:delete threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
    }

    if (wre == null && type == MutationType.DELETE && proto.hasRow())
      row = proto.getRow().toByteArray();
    try {
        delete = ProtobufUtil.toDelete(proto); 
    } catch (Throwable e) {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:delete threw exception");
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
      t = e;
    }

    // Process in local memory
    try {
      delete(request.getTransactionId(), delete);
    } catch (Throwable e) {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:delete threw exception after internal delete");
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
      t = e;
    }

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: delete - id " + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString() + ", type " + type + ", row " + Bytes.toString(row));

    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.DeleteTransactionalResponse.Builder deleteTransactionalResponseBuilder = DeleteTransactionalResponse.newBuilder();

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

    DeleteTransactionalResponse dresponse = deleteTransactionalResponseBuilder.build();
    done.run(dresponse);
  }

  @Override
  public void get(RpcController controller,
                  GetTransactionalRequest request,
                  RpcCallback<GetTransactionalResponse> done) {
    GetTransactionalResponse response = GetTransactionalResponse.getDefaultInstance();

    org.apache.hadoop.hbase.protobuf.generated.ClientProtos.Get proto = request.getGet();
    Get get = null;
    RegionScanner scanner = null;
    Throwable t = null;
    Exception ge = null;
    WrongRegionException wre = null;
    org.apache.hadoop.hbase.client.Result result2 = null;

    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();

    // First test if this region matches our region name
    /*
    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:get threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
    } else { */
      try {
        get = ProtobufUtil.toGet(proto);
      } catch (Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:get threw exception");
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
        t = e;
      }

      byte[] row = proto.getRow().toByteArray();
      byte[] getrow = get.getRow();
      String rowKey = Bytes.toString(row);
      String getRowKey = Bytes.toString(getrow);

      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: get - id " + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString() + ", row = " + rowKey + ", getRowKey = " + getRowKey);

/*
 *    TODO: Disabled to resolve array out of bounds in certain cases
      if (LOG.isTraceEnabled()) {
	  Map lv_getmap = get.toMap(1000);
	  Map<byte[],NavigableSet<byte[]>> lv_fm = get.getFamilyMap();
	  byte [][] lv_fms = lv_fm.keySet().toArray(new byte[0][0]);
	  byte[] lv_f = lv_fms[0];
	  if (LOG.isTraceEnabled()) LOG.trace("family: " + new String(lv_f));
	  NavigableSet<byte []> lv_set = lv_fm.get(lv_f);
	  if (LOG.isTraceEnabled()) LOG.trace("get - lv_set size: " + lv_set.size() + " Map size: " + lv_getmap.size());
	  //	  if (LOG.isTraceEnabled()) LOG.trace("get -  map contents: " + lv_getmap);
      }
*/

      Scan scan = new Scan(get);
      List<Cell> results = new ArrayList<Cell>();

      try {
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: get - id Calling getScanner id/scan" + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString() + ", row = " + rowKey + ", getRowKey = " + getRowKey);
         scanner = getScanner(request.getTransactionId(), scan);

         if (scanner != null)
            scanner.next(results);
         
      	 result2 = Result.create(results);

         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: get - id No exception, result2 isEmpty is " 
		   + result2.isEmpty() 
		   + ", row " 
		   + result2.getRow()
		   + " result length: "
		   + result2.size()); 
	 /*
	 if (LOG.isTraceEnabled()) {
	     if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: get - result contents: " + result2);
	 }
	 */

      } catch(Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: get - id Caught exception  " + e.getMessage() + " " + stackTraceToString(e));
        t = e;
      }
      finally {
        if (scanner != null) {
          try {
            scanner.close();
          } catch(Exception e) {
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
            ge = e;
          }
        }
      }
    //}

    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.GetTransactionalResponse.Builder getResponseBuilder = GetTransactionalResponse.newBuilder();

   getResponseBuilder.setResult(ProtobufUtil.toResult(result2));
      
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

   GetTransactionalResponse gresponse = getResponseBuilder.build();

   done.run(gresponse);

  }

  @Override
  public void openScanner(RpcController controller,
                          OpenScannerRequest request,
                          RpcCallback<OpenScannerResponse> done) {
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

    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();

    // First test if this region matches our region name
    
    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:openScanner threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        exceptionThrown = true;
    } else {
    
      try {
        scan = ProtobufUtil.toScan(request.getScan());
        if (scan == null)
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  openScanner scan was null");
      //isLoadingCfsOnDemandSet = scan.getLoadColumnFamiliesOnDemandValue();
      //if (!isLoadingCfsOnDemandSet) 
       //scan.setLoadColumnFamiliesOnDemand(this.m_Region.isLoadingCfsOnDemandDefault());
      } catch (Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  openScanner Caught exception " + e.getMessage() + "" + stackTraceToString(e));
        t = e;
        exceptionThrown = true;
      }
    }

    if (!exceptionThrown) {
      if (scan == null) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:openScanner scan is null");
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
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:openScanner scan threw exception");
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
          t = e;
          exceptionThrown = true;
        }
      }
    }

/*
      if (!exceptionThrown) {
        if (region.getCoprocessorHost() != null) {
          scanner = region.getCoprocessorHost().preScannerOpen(scan);
        }
*/


    List<Cell> results = new ArrayList<Cell>();

    if (!exceptionThrown) {
      try {
        scanner = getScanner(request.getTransactionId(), scan);
        
        if (scanner != null) {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  openScanner called getScanner, scanner is " + scanner + ", transid " + request.getTransactionId());
          // Add the scanner to the map
          scannerId = addScanner(scanner, this.m_Region);
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  openScanner called addScanner, scannerId is " + scannerId + ", transid " + request.getTransactionId());
        }
        else
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  getScanner returned null, scannerId is " + scannerId + ", transid " + request.getTransactionId());
       
      } catch (LeaseStillHeldException llse) {
/*
        try {
            scannerLeases.cancelLease(getScannerLeaseId(scannerId));
          } catch (LeaseException le) {
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getScanner failed to get a lease " + scannerId);
          }
*/
        LOG.error("TrxRegionEndpoint coprocessor:  getScanner Error opening scanner, " + llse.toString());
        exceptionThrown = true;
        lse = llse;
      } catch (IOException e) {
        LOG.error("TrxRegionEndpoint coprocessor:  getScanner Error opening scanner, "                  + e.toString());
        exceptionThrown = true;
      }
    }

/*
    if (region.getCoprocessorHost() != null) {
      scanner = region.getCoprocessorHost().postScannerOpen(scan, scanner);
    }
*/

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: openScanner - id " + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString());

    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.OpenScannerResponse.Builder openResponseBuilder = OpenScannerResponse.newBuilder();

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

    OpenScannerResponse oresponse = openResponseBuilder.build();
    done.run(oresponse);
  }

  @Override
  public void performScan(RpcController controller,
                          PerformScanRequest request,
                          RpcCallback<PerformScanResponse> done) {

    boolean hasMore = true;
    RegionScanner scanner = null;
    Throwable t = null;
    ScannerTimeoutException ste = null;
    WrongRegionException wre = null;
    Exception ne = null;
    Scan scan = null;
    List<Cell> cellResults = new ArrayList<Cell>();
    List<Result> results = new ArrayList<Result>();
    org.apache.hadoop.hbase.client.Result result = null;

    long scannerId = request.getScannerId();
    int numberOfRows = request.getNumberOfRows();
    boolean closeScanner = request.getCloseScanner();
    long nextCallSeq = request.getNextCallSeq();
    long count = 0L;
    boolean shouldContinue = true;

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - scannerId " + scannerId + ", numberOfRows " + numberOfRows + ", nextCallSeq " + nextCallSeq); 

    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();

    // First test if this region matches our region name
    
    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:performScan threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
    } else {
    
      try {

        scanner = getScanner(scannerId, nextCallSeq);

        if (scanner != null)
        {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - id " + scannerId+ ", scanner is not null"); 
          while (shouldContinue) {
            hasMore = scanner.next(cellResults);
            result = Result.create(cellResults);
            cellResults.clear();

            if (!result.isEmpty()) {
              results.add(result);
              count++;
            }

            if (count == numberOfRows || !hasMore)
              shouldContinue = false;
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - id " + scannerId + ", count is " + count + ", hasMore is " + hasMore + ", result " + result.isEmpty() + ", row " + result.getRow()); 
          }
        }
        else
        {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - id " + scannerId+ ", scanner is null"); 
        }

     } catch(ScannerTimeoutException cste) {
       if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - id " + scannerId + " Caught ScannerTimeoutExceptionn  " + cste.getMessage() + " " + stackTraceToString(cste));
       ste = cste;
     } catch(Throwable e) {
       if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - id " + scannerId + " Caught exception  " + e.getMessage() + " " + stackTraceToString(e));
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
               if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan failed to get a lease " + scannerId);
             }
*/
           }
         } catch(Exception e) {
           if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  performScan caught exception " + e.getMessage() + "" + stackTraceToString(e));
           ne = e;
         }
       }
     }
 
   }

   if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - id " + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString() +
", scannerId " + scannerId);

   nextCallSeq++;

    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PerformScanResponse.Builder performResponseBuilder = PerformScanResponse.newBuilder();
    performResponseBuilder.setHasMore(hasMore);
    performResponseBuilder.setNextCallSeq(nextCallSeq++);
    performResponseBuilder.setCount(count);
    performResponseBuilder.setHasException(false);

    if (results != null)
    {
      if (!results.isEmpty()) {
        for (Result r: results) {
          performResponseBuilder.addResult(ProtobufUtil.toResult(r));
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

    PerformScanResponse presponse = performResponseBuilder.build();
    done.run(presponse);
  }

  @Override
  public void put(RpcController controller,
                  PutTransactionalRequest request,
      RpcCallback<PutTransactionalResponse> done) {
    PutTransactionalResponse response = PutTransactionalResponse.getDefaultInstance();

    byte [] row = null;
    MutationProto proto = request.getPut();
    MutationType type = proto.getMutateType();
    Put put = null;
    Throwable t = null;
    WrongRegionException wre = null;

    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();
    // First test if this region matches our region name
    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:put threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
    } else {
      try {
          put = ProtobufUtil.toPut(proto);
      } catch (Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:put threw exception");
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
        t = e;
      }

      if (type == MutationType.PUT && proto.hasRow())
      {
        row = proto.getRow().toByteArray();

        // Process in local memory
        try {   
          put(request.getTransactionId(), put);
        } catch (Throwable e) {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:put threw exception after internal put");
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
          t = e;
        }

        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: put - id " + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString() + ", type " + type + ", row " + Bytes.toString(row));
      }
      else
      {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: put - id " + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString() + "- no valid PUT type or does not contain a row");
      }
    }

    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PutTransactionalResponse.Builder putTransactionalResponseBuilder = PutTransactionalResponse.newBuilder();

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

    PutTransactionalResponse presponse = putTransactionalResponseBuilder.build();
    done.run(presponse);
  }

  @Override
  public void putMultiple(RpcController controller,
                          PutMultipleTransactionalRequest request,
                          RpcCallback<PutMultipleTransactionalResponse> done) {
    PutMultipleTransactionalResponse response = PutMultipleTransactionalResponse.getDefaultInstance();

   java.util.List<org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto> results;
   results = request.getPutList();
   int resultCount = request.getPutCount();
   byte [] row = null;
   Put put = null;
   MutationType type;
   Throwable t = null;
   WrongRegionException wre = null;
   java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();
    // First test if this region matches our region name
    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:putMultiple threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
   } else {

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
               put = ProtobufUtil.toPut(proto);
           } catch (Throwable e) {
             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:put threw exception after protobuf conversion put");
             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
             t = e;
           }

           // Process in local memory
           if (put != null)
           {
             try {
               put(request.getTransactionId(), put);
             } catch (Throwable e) {
               if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:put threw exception after internal put");
             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
               t = e;
             }

             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: putMultiple - id " + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString() + ", type " + type + ", row " + Bytes.toString(row));
           }
         }
       }
        else
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: putMultiple - id " + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString() + ", put proto was null");

      }
    }
      
    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PutMultipleTransactionalResponse.Builder putMultipleTransactionalResponseBuilder = PutMultipleTransactionalResponse.newBuilder();

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

    PutMultipleTransactionalResponse pmresponse = putMultipleTransactionalResponseBuilder.build();
    done.run(pmresponse);
  }

  @Override
  public void recoveryRequest(RpcController controller,
                              RecoveryRequestRequest request,
                              RpcCallback<RecoveryRequestResponse> done) {
      int tmId = request.getTmId();
      Throwable t = null;
      WrongRegionException wre = null;

      //SST TBD until integration with recovery 
      if (reconstructIndoubts == 0) {
         LOG.debug("TrxRegionEndpoint coprocessor:  RECOV recovery Request");
         constructIndoubtTransactions();
      }

      // Placeholder for real work when recovery is added
      if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: recoveryResponse - id " + request.getTransactionId() + ", regionName " + regionInfo.getRegionNameAsString() + ", tmId" + tmId);

      java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();

      // First test if this region matches our region name
      if (!name.equals(regionInfo.getRegionNameAsString())) {
         wre = new WrongRegionException("Request Region Name, " +
          name + ",  does not match this region, " +
          regionInfo.getRegionNameAsString());
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:recoveryResponse threw WrongRegionException" +
      "Request Region Name, " +
          name + ",  does not match this region, " +
          regionInfo.getRegionNameAsString());
      } 

      org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.RecoveryRequestResponse.Builder recoveryResponseBuilder = RecoveryRequestResponse.newBuilder();

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
                              LOG.info("Trafodion Recovery: region " + regionInfo.getEncodedName() + " in-doubt transaction " + tid + " has been added into the recovery repply to TM " + tmId);
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

      RecoveryRequestResponse rresponse = recoveryResponseBuilder.build();
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
  public void getMax(RpcController controller, TransactionalAggregateRequest request,
    RpcCallback<TransactionalAggregateResponse> done) {
    RegionScanner scanner = null;
    TransactionalAggregateResponse response = null;
    T max = null;
    try {
      ColumnInterpreter<T, S, P, Q, R> ci = constructColumnInterpreterFromRequest(request);
      T temp;
      long transactionId = request.getTransactionId();
      Scan scan = ProtobufUtil.toScan(request.getScan());
      scanner = getScanner(transactionId, scan);
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
        TransactionalAggregateResponse.Builder builder = TransactionalAggregateResponse.newBuilder();
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
    LOG.info("TrxRegionEndpoint coprocessor: getMax - Maximum from this region is "
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
  public void getMin(RpcController controller, TransactionalAggregateRequest request,
      RpcCallback<TransactionalAggregateResponse> done) {
    TransactionalAggregateResponse response = null;
    RegionScanner scanner = null;
    T min = null;
    try {
      ColumnInterpreter<T, S, P, Q, R> ci = constructColumnInterpreterFromRequest(request);
      T temp;
      Scan scan = ProtobufUtil.toScan(request.getScan());
      long transactionId = request.getTransactionId();
      scanner = getScanner(transactionId, scan);
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
        response = TransactionalAggregateResponse.newBuilder().addFirstPart( 
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
    LOG.info("Minimum from this region is "
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
  public void getSum(RpcController controller, TransactionalAggregateRequest request,
      RpcCallback<TransactionalAggregateResponse> done) {
    TransactionalAggregateResponse response = null;
    RegionScanner scanner = null;
    long sum = 0l;
    try {
      ColumnInterpreter<T, S, P, Q, R> ci = constructColumnInterpreterFromRequest(request);
      S sumVal = null;
      T temp;
      Scan scan = ProtobufUtil.toScan(request.getScan());
      long transactionId = request.getTransactionId();
      scanner = getScanner(transactionId, scan);
      byte[] colFamily = scan.getFamilies()[0];
      NavigableSet<byte[]> qualifiers = scan.getFamilyMap().get(colFamily);
      byte[] qualifier = null;
      if (qualifiers != null && !qualifiers.isEmpty()) {
        qualifier = qualifiers.pollFirst();
      }
      List<Cell> results = new ArrayList<Cell>();
      boolean hasMoreRows = false;
      do {
        hasMoreRows = scanner.next(results);
        for (Cell kv : results) {
          temp = ci.getValue(colFamily, qualifier, kv);
          if (temp != null)
            sumVal = ci.add(sumVal, ci.castToReturnType(temp));
        }
        results.clear();
      } while (hasMoreRows);
      if (sumVal != null) {
        response = TransactionalAggregateResponse.newBuilder().addFirstPart( 
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
    if (LOG.isDebugEnabled()) LOG.debug("Sum from this region is "
        + env.getRegion().getRegionNameAsString() + ": " + sum);
    done.run(response);
  }

  /**
   * Gives the row count for the given column family and column qualifier, in
   * the given row range as defined in the Scan object.
   * @throws IOException
   */
  @Override
  public void getRowNum(RpcController controller, TransactionalAggregateRequest request,
      RpcCallback<TransactionalAggregateResponse> done) {
    TransactionalAggregateResponse response = null;
    long counter = 0l;
    List<Cell> results = new ArrayList<Cell>();
    RegionScanner scanner = null;
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
      long transactionId = request.getTransactionId();
      scanner = getScanner(transactionId, scan);
      boolean hasMoreRows = false;
      do {
        hasMoreRows = scanner.next(results);
        if (results.size() > 0) {
          counter++;
        }
        results.clear();
      } while (hasMoreRows);
      ByteBuffer bb = ByteBuffer.allocate(8).putLong(counter);
      bb.rewind();
      response = TransactionalAggregateResponse.newBuilder().addFirstPart( 
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
    LOG.info("Row counter from this region is "
        + env.getRegion().getRegionNameAsString() + ": " + counter);
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
  public void getAvg(RpcController controller, TransactionalAggregateRequest request,
      RpcCallback<TransactionalAggregateResponse> done) {
    TransactionalAggregateResponse response = null;
    RegionScanner scanner = null;
    try {
      ColumnInterpreter<T, S, P, Q, R> ci = constructColumnInterpreterFromRequest(request);
      S sumVal = null;
      Long rowCountVal = 0l;
      Scan scan = ProtobufUtil.toScan(request.getScan());
      long transactionId = request.getTransactionId();
      scanner = getScanner(transactionId, scan);
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
        TransactionalAggregateResponse.Builder pair = TransactionalAggregateResponse.newBuilder();
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
  public void getStd(RpcController controller, TransactionalAggregateRequest request,
      RpcCallback<TransactionalAggregateResponse> done) {
    RegionScanner scanner = null;
    TransactionalAggregateResponse response = null;
    try {
      ColumnInterpreter<T, S, P, Q, R> ci = constructColumnInterpreterFromRequest(request);
      S sumVal = null, sumSqVal = null, tempVal = null;
      long rowCountVal = 0l;
      Scan scan = ProtobufUtil.toScan(request.getScan());
      long transactionId = request.getTransactionId();
      scanner = getScanner(transactionId, scan);
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
        TransactionalAggregateResponse.Builder pair = TransactionalAggregateResponse.newBuilder();
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
  public void getMedian(RpcController controller, TransactionalAggregateRequest request,
      RpcCallback<TransactionalAggregateResponse> done) {
    TransactionalAggregateResponse response = null;
    RegionScanner scanner = null;
    try {
      ColumnInterpreter<T, S, P, Q, R> ci = constructColumnInterpreterFromRequest(request);
      S sumVal = null, sumWeights = null, tempVal = null, tempWeight = null;
      Scan scan = ProtobufUtil.toScan(request.getScan());
      long transactionId = request.getTransactionId();
      scanner = getScanner(transactionId, scan);
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
      TransactionalAggregateResponse.Builder pair = TransactionalAggregateResponse.newBuilder();
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
      TransactionalAggregateRequest request) throws IOException {
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
      throw new CoprocessorException("TrxRegionEndpoint coprocessor: start - Must be loaded on a table region!");
    }
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: start");
    RegionCoprocessorEnvironment tmp_env = 
      (RegionCoprocessorEnvironment)env;
    this.m_Region =
       tmp_env.getRegion();
    this.regionInfo = this.m_Region.getRegionInfo();
    this.t_Region = (TransactionalRegion) tmp_env.getRegion();
    this.fs = this.m_Region.getFilesystem();

    org.apache.hadoop.conf.Configuration conf = new org.apache.hadoop.conf.Configuration(); 
    
    synchronized (stoppable) {
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

	//if (this.scannerLeases == null)  
	 //   this.scannerLeases = new Leases(scannerThreadWakeFrequency);

        if (LOG.isTraceEnabled()) LOG.trace("Transaction lease time: " + transactionLeaseTimeout);
        if (LOG.isTraceEnabled()) LOG.trace("Scanner lease time: " + scannerThreadWakeFrequency);

        this.cleanOldTransactionsThread = new CleanOldTransactionsChore(this, cleanTimer, stoppable);

        UncaughtExceptionHandler handler = new UncaughtExceptionHandler() {

          public void uncaughtException(final Thread t, final Throwable e)
          {
            LOG.fatal("CleanOldTransactionChore uncaughtException: " + t.getName(), e);
          }
        };
 
        String n = Thread.currentThread().getName();
	
        ChoreThread = new Thread(this.cleanOldTransactionsThread);
        Threads.setDaemonThreadRunning(ChoreThread, n + ".oldTransactionCleaner", handler);

	if (TransactionalLeasesThread == null) {
	    TransactionalLeasesThread = new Thread(this.transactionLeases);
	    if (TransactionalLeasesThread != null) {
		Threads.setDaemonThreadRunning(TransactionalLeasesThread, "Transactional leases");
	    }
	}

/*
	if (ScannerLeasesThread == null) {
	    ScannerLeasesThread = new Thread(this.scannerLeases);
	    if (ScannerLeasesThread != null) {
		Threads.setDaemonThreadRunning(ScannerLeasesThread, "Scanner leases");
	    }
	}
*/

      } catch (Exception e) {
        throw new CoprocessorException("TrxRegionEndpoint coprocessor: start threw exception " + e);
      }
    }

    this.t_Region = (TransactionalRegion) tmp_env.getRegion();
    this.fs = this.m_Region.getFilesystem();
    tHLog = this.m_Region.getLog();

    RegionServerServices rss = tmp_env.getRegionServerServices();
    ServerName sn = rss.getServerName();
    lv_hostName = sn.getHostname();
    lv_port = sn.getPort();
    if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: Hostname " + lv_hostName + " port " + lv_port);
    this.regionInfo = this.m_Region.getRegionInfo();
    this.nextLogSequenceId = this.m_Region.getSequenceId();
    this.t_Region = (TransactionalRegion) tmp_env.getRegion();
    zkw1 = rss.getZooKeeper();

    if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: get the reference from Region CoprocessorEnvrionment ");

    if (tmp_env.getSharedData().isEmpty())
       if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: shared map is empty ");
    else
       if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: shared map is NOT empty Yes ... ");

    transactionsByIdTestz = TrxRegionObserver.getRefMap();

    if (transactionsByIdTestz.isEmpty())
       if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: reference map is empty ");
    else
       if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: reference map is NOT empty Yes ... ");

    if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: UUU Region " + this.m_Region.getRegionNameAsString() + " check indoubt list from reference map ");
   
    indoubtTransactionsById = (TreeMap<Long, WALEdit>)transactionsByIdTestz.get(
                               this.m_Region.getRegionNameAsString()+TrxRegionObserver.trxkeypendingTransactionsById);

    indoubtTransactionsCountByTmid = (TreeMap<Integer,Integer>)transactionsByIdTestz.get(
                               this.m_Region.getRegionNameAsString()+TrxRegionObserver.trxkeyindoubtTransactionsCountByTmid);
    if (indoubtTransactionsCountByTmid != null) {
       if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor:OOO successfully get the reference from Region CoprocessorEnvrionment ");
    }

    LOG.info("TrxRegionEndpoint coprocessor: start");
  }

  @Override
  public void stop(CoprocessorEnvironment env) throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: stop ");
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
      LOG.error("TrxRegionEndpoint coprocessor:  Trafodion Recovery: checkClosing(" + transactionId + ") - raising exception. no more transaction allowed.");
      throw new IOException("closing region, no more transaction allowed");
    }
  }

  /**
   * Gets the transaction state                   
   * @param long transactionId
   * @return TransactionState
   * @throws UnknownTransactionException
   */
  protected TransactionState getTransactionState(final long transactionId)
   throws UnknownTransactionException {
    TransactionState state = null;
    boolean throwUTE = false;

    synchronized (transactionsById) {
      state = transactionsById.get(getTransactionalUniqueId(transactionId));

    if (state == null) 
    {
      if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: getTransactionState Unknown transaction: [" + transactionId + "], throwing UnknownTransactionException");              
        throwUTE = true;
    }
      else {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getTransactionState Found transaction: [" + transactionId + "]");              

    try {
          transactionLeases.renewLease(getTransactionalUniqueId(transactionId));
    } catch (LeaseException e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getTransactionState renewLease failed will try to createLease for transaction: [" + transactionId + "]");              
          try {
	    transactionLeases.createLease(
                                   getTransactionalUniqueId(transactionId),
				   transactionLeaseTimeout,
				   new TransactionLeaseListener(transactionId));
          } catch (LeaseStillHeldException lshe) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getTransactionState renewLeasefollowed by createLease failed throwing original LeaseException for transaction: [" + transactionId + "]");              
      throw new RuntimeException(e);
    }
        }
      }
    }

    if (throwUTE)
      throw new UnknownTransactionException();

    return state;
  }

  /**
   * Retires the transaction                        
   * @param TransactionState state
   */
  private void retireTransaction(final TransactionState state) {
    //long key = state.getTransactionId();
    String key = getTransactionalUniqueId(state.getTransactionId());

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: retireTransaction: [" 
             + state.getTransactionId() + "]");              

    try {
      transactionLeases.cancelLease(getTransactionalUniqueId(state.getTransactionId()));
    } catch (LeaseException le) {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: retireTransaction: [" 
               + state.getTransactionId() + "] LeaseException");              
      // Ignore
    } catch (Exception e) {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: retireTransaction: [" 
               + state.getTransactionId() + "] General Lease exception");         
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
      // Ignore
    }

    // Clearing transaction conflict check list in case it is holding
    // a reference to a transaction state

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  retireTransaction clearTransactionsById: " + key + " from list");
    state.clearTransactionsToCheck();
    synchronized (transactionsById) {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  retireTransaction calling Removing transaction: " + key + " from list");
      transactionsById.remove(key);
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  retireTransaction Removed transaction: " + key + " from list");
      // Logging to catch error 97
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
         LOG.debug("Trafodion Recovery Region Observer CP: ZKW Delete region recovery znode" + node + " zNode Path " + zNodePathTMKey);
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
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Trafodion Recovery:  Flushing cache in startRegionAfterRecovery " + m_Region.getRegionInfo().getRegionNameAsString());
          m_Region.flushcache();
          //if (!m_Region.flushcache().isFlushSucceeded()) { 
          //   LOG.trace("TrxRegionEndpoint coprocessor:  Trafodion Recovery:  Flushcache returns false !!! " + m_Region.getRegionInfo().getRegionNameAsString());
          //}
     } catch (IOException e) {
     LOG.error("TrxRegionEndpoint coprocessor:  Trafodion Recovery: Flush failed after replay edits" + m_Region.getRegionInfo().getRegionNameAsString());
     return;
     }


    //FileSystem fileSystem = m_Region.getFilesystem();
    //Path archiveTHLog = new Path (recoveryTrxPath.getParent(),"archivethlogfile.log");
    //if (fileSystem.exists(archiveTHLog)) fileSystem.delete(archiveTHLog, true);
    //if (fileSystem.exists(recoveryTrxPath))fileSystem.rename(recoveryTrxPath,archiveTHLog);
    if (indoubtTransactionsById != null)
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Trafodion Recovery: region " + recoveryTrxPath + " has " + indoubtTransactionsById.size() + " in-doubt transactions and edits are archived.");
    else
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Trafodion Recovery: region " + recoveryTrxPath + " has 0 in-doubt transactions and edits are archived.");
    regionState = 2; 
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Trafodion Recovery: region " + m_Region.getRegionInfo().getEncodedName() + " is STARTED.");
  }

  /**
   * Commits the transaction
   * @param TransactionState state
   * @throws IOException 
   */
  private void commit(final TransactionState state) throws IOException {
    long txid = 0;
    WALEdit b = null;
    Tag commitTag;
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Commiting transaction: " + state.toString() + " to "
     + m_Region.getRegionInfo().getRegionNameAsString());
    long transactionId = state.getTransactionId();
    if (state.isReinstated()) {
      if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor:  commit Trafodion Recovery: commit reinstated indoubt transactions " + transactionId + 
                              " in region " + m_Region.getRegionInfo().getRegionNameAsString());
      synchronized (indoubtTransactionsById) {  
        b = indoubtTransactionsById.get(transactionId);
      }
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  commit Writing " + b.size() + " updates for reinstated transaction " + transactionId);
      for (KeyValue kv : b.getKeyValues()) {
        synchronized (editReplay) {
          Put put;
          Delete del;
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Trafodion Recovery:   " + m_Region.getRegionInfo().getRegionNameAsString() + " replay commit for transaction: "   + transactionId + " with Op " + kv.getType());
          if (kv.getTypeByte() == KeyValue.Type.Put.getCode()) {
		put = new Put(CellUtil.cloneRow(kv)); // kv.getRow()
                put.add(CellUtil.cloneFamily(kv), CellUtil.cloneQualifier(kv), kv.getTimestamp(), CellUtil.cloneValue(kv));
                state.addWrite(put);
           try {
             m_Region.put(put);
           }
           catch (Exception e) {
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  commit Trafodion Recovery: Executing put threw an exception");
              state.setStatus(Status.ABORTED);
              retireTransaction(state);
              throw new IOException(e.toString());
           }
  	  } else if (CellUtil.isDelete(kv))  {
		del = new Delete(CellUtil.cloneRow(kv));
	       	if (CellUtil.isDeleteFamily(kv)) {
	 	     del.deleteFamily(CellUtil.cloneFamily(kv));
	        } else if (kv.isDeleteType()) {
	             del.deleteColumn(CellUtil.cloneFamily(kv), CellUtil.cloneQualifier(kv));
	        }
                state.addDelete(del);
            try {
              m_Region.delete(del);
            }
            catch (Exception e) {
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  commit Trafodion Recovery: Executing delete threw an exception");
              state.setStatus(Status.ABORTED);
              retireTransaction(state);
              throw new IOException(e.toString());
            }
   	 }
        } // synchronized reply edits
      }
    }  // reinstated transactions
    else {
      // Perform write operations timestamped to right now
      // maybe we can turn off WAL here for HLOG since THLOG has contained required edits in phase 1
      List<WriteAction> writeOrdering = state.getWriteOrdering();
      for (WriteAction action : writeOrdering) {
         // Process Put
         Put put = action.getPut();

         if (null != put) {
          put.setDurability(Durability.SKIP_WAL);
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  commit Executing put directly to m_Region");
           try {
             m_Region.put(put);
           }
           catch (Exception e) {
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  commit Executing put Threw an exception");
              state.setStatus(Status.ABORTED);
              retireTransaction(state);
              throw new IOException(e.toString());
           }
         }

         // Process Delete
         Delete delete = action.getDelete();

         if (null != delete){
          delete.setDurability(Durability.SKIP_WAL);
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  commit Executing delete directly to m_Region");
           try {
             m_Region.delete(delete);
           }
           catch (Exception e) {
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  commit Executing delete Threw an exception");
              state.setStatus(Status.ABORTED);
              retireTransaction(state);
              throw new IOException(e.toString());
           }
         }
       }
    } // normal transactions

    // Now the transactional writes live in the core WAL, we can write a commit to the log
    // so we don't have to recover it from the transactional WAL.
    if (state.hasWrite() || state.isReinstated()) {
       // comment out for now
      if (LOG.isTraceEnabled()) LOG.trace("write commit edit to HLOG");
      //this.transactionLog.writeCommitToLog(m_Region.getRegionInfo(), state.getTransactionId(), m_Region.getTableDesc()); 
      if (editGenerated) commitTag = state.formTransactionalContextTag(COMMIT);
      else commitTag = state.formTransactionalContextTag(CONTROL_POINT_COMMIT);
      List<Tag> tagList = new ArrayList<Tag>();
      tagList.add(commitTag);

      //KeyValue kv1 = new KeyValue(new byte[10], null, null, HConstants.LATEST_TIMESTAMP, null, tagList);
      WALEdit e1 = state.getEdit();
      WALEdit e = new WALEdit();

      if (editGenerated) {
         // TBD get 1st Cell to associated with the commit record as a workaround through HLOG async append
         Cell c = e1.getKeyValues().get(0);
         KeyValue kv = new KeyValue(c.getRowArray(), c.getRowOffset(), (int)c.getRowLength(),
					c.getFamilyArray(), c.getFamilyOffset(), (int)c.getFamilyLength(),
					c.getQualifierArray(), c.getQualifierOffset(), (int) c.getQualifierLength(),
					c.getTimestamp(), Type.codeToType(c.getTypeByte()), c.getValueArray(), c.getValueOffset(),
					c.getValueLength(), tagList);
      
         e.add(kv);
       }
       else { // TBD to handle 1st edit issue
         for (Cell c : e1.getKeyValues()) {
              KeyValue kv = new KeyValue(c.getRowArray(), c.getRowOffset(), (int)c.getRowLength(),
					c.getFamilyArray(), c.getFamilyOffset(), (int)c.getFamilyLength(),
					c.getQualifierArray(), c.getQualifierOffset(), (int) c.getQualifierLength(),
					c.getTimestamp(), Type.codeToType(c.getTypeByte()), c.getValueArray(), c.getValueOffset(),
					c.getValueLength(), tagList);
              e.add(kv);
          }
       }

    //if (!editGenerated) { // comment out the if (!editGenerated) -- only for DEV test to create in-doubt txn
      try {
           txid = this.tHLog.appendNoSync(this.regionInfo, this.regionInfo.getTable(),
                  e, new ArrayList<UUID>(), EnvironmentEdgeManager.currentTimeMillis(), this.m_Region.getTableDesc(),
                  nextLogSequenceId, false, HConstants.NO_NONCE, HConstants.NO_NONCE);
      }
      catch (IOException exp1) {
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor commit writing to HLOG : Threw an exception");
         throw exp1;
       }
    //}
         if (LOG.isTraceEnabled()) LOG.trace("BBB write commit edit to HLOG after appendNoSync");
         if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor:commit -- EXIT txId: " + transactionId + " HLog seq " + txid);
         if (!editGenerated) editGenerated = true;
      // no need to do this.tHLog.sync(txid) for phase 2 due to TLOG enabled
    }

    state.setStatus(Status.COMMITED);
    if (state.hasWrite() || state.isReinstated()) {
      synchronized (commitPendingTransactions) {
      if (!commitPendingTransactions.remove(state)) {
          LOG.fatal("TrxRegionEndpoint coprocessor:  commit Commiting a non-query transaction that is not in commitPendingTransactions");
          // synchronized statements are cleared for a throw
        throw new IOException("commit failure");
      }
    }
    }

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  commit(tstate) -- EXIT TransactionState: " + 
      state.toString());

    if (state.isReinstated()) {
      synchronized(indoubtTransactionsById) {
        indoubtTransactionsById.remove(state.getTransactionId());
        int tmid = (int) (transactionId >> 32);
        int count = 0;
        if (indoubtTransactionsCountByTmid.containsKey(tmid)) {
          count =  (int) indoubtTransactionsCountByTmid.get(tmid) - 1;
          if (count > 0) indoubtTransactionsCountByTmid.put(tmid, count);
        }
        if (count == 0) {
          indoubtTransactionsCountByTmid.remove(tmid);
          String lv_encoded = m_Region.getRegionInfo().getEncodedName();
            try {
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit Trafodion Recovery: delete in commit recovery zNode TM " + tmid + " region encoded name " + lv_encoded + " for 0 in-doubt transaction");
              deleteRecoveryzNode(tmid, lv_encoded);
            } catch (IOException e) {
            LOG.error("Trafodion Recovery: delete recovery zNode failed");
            }
        }

        if ((indoubtTransactionsById == null) || (indoubtTransactionsById.size() == 0)) {
          if (indoubtTransactionsById == null) 
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  commit Trafodion Recovery: start region in commit with indoubtTransactionsById null");
          else
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  commit Trafodion Recovery: start region in commit with indoubtTransactionsById size " + indoubtTransactionsById.size());
          startRegionAfterRecovery();
        }
      }
    }
    retireTransaction(state);
  }

  /**
   * Rssolves the transaction from the log
   * @param TransactionState transactionState
   * @throws IOException 
   */
  private void resolveTransactionFromLog(
    final TransactionState transactionState) throws IOException {
    LOG.error("TrxRegionEndpoint coprocessor:  Global transaction log is not Implemented. (Optimisticly) assuming transaction commit!");
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
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  leaseExpired Transaction [" + this.transactionName
              + "] expired in region ["
              + m_Region.getRegionInfo().getRegionNameAsString() + "]");
   TransactionState s = null;
   synchronized (transactionsById) {
     s = transactionsById.remove(transactionName);
     if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  leaseExpired Removing transaction: " + this.transactionName + " from list");
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
       if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: leaseExpired  Transaction " + s.getTransactionId()
                                + " expired in COMMIT_PENDING state");

        try {
          if (s.getCommitPendingWaits() > MAX_COMMIT_PENDING_WAITS) {
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: leaseExpired  Checking transaction status in transaction log");
            resolveTransactionFromLog(s);
            break;
          }
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: leaseExpired  renewing lease and hoping for commit");
          s.incrementCommitPendingWaits();
          synchronized (transactionsById) {
            transactionsById.put(getTransactionalUniqueId(s.getTransactionId()), s);
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: leaseExpired  Adding transaction: " + s.getTransactionId() + " to list");
          }
          try {
            transactionLeases.createLease(
            getTransactionalUniqueId(s.getTransactionId()),
                                    transactionLeaseTimeout,
                                    this); 
          } catch (LeaseStillHeldException e) {
            transactionLeases.renewLease(getTransactionalUniqueId(s.getTransactionId()));
          }
          } catch (IOException e) {
            throw new RuntimeException(e);
          }

          break;

       default:
         LOG.warn("TrxRegionEndpoint coprocessor: leaseExpired  Unexpected status on expired lease");
     }
   }
 }

  /**
   * Processes a transactional delete
   * @param long transactionId
   * @param Delete delete      
   * @throws IOException 
   */
  public void delete(final long transactionId, final Delete delete)
    throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: delete -- ENTRY txId: " + transactionId);
    checkClosing(transactionId);
    TransactionState state = this.beginTransIfNotExist(transactionId);
    state.addDelete(delete);
  }


  /**
   * Processes multiple transactional deletes    
   * @param long transactionId
   * @param Delete[] deletes   
   * @throws IOException 
   */
  public synchronized void delete(long transactionId, Delete[] deletes)
    throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("Enter TrxRegionEndpoint coprocessor: deletes[], txid: " + transactionId);
    checkClosing(transactionId);

    TransactionState state = this.beginTransIfNotExist(transactionId);

    for (Delete del : deletes) {
      state.addDelete(del);
    }
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
  public boolean checkAndDelete(long transactionId, 
                                byte[] row, byte[] family,
                                byte[] qualifier, byte[] value, Delete delete)
    throws IOException {

    if (LOG.isTraceEnabled()) LOG.trace("Enter TrxRegionEndpoint coprocessor: checkAndDelete, txid: "
                + transactionId);
    TransactionState state = this.beginTransIfNotExist(transactionId);
    boolean result = false;
    byte[] rsValue = null;

    Get get = new Get(row);
    get.addColumn(family, qualifier);

    Result rs = this.get(transactionId, get);
    
    boolean valueIsNull = value == null ||
                          value.length == 0;

    if (rs.isEmpty() && valueIsNull) {
      this.delete(transactionId, delete);
      result = true;
    } else if (!rs.isEmpty() && valueIsNull) {
      rsValue = rs.getValue(family, qualifier);
      if (rsValue != null && rsValue.length == 0) {
        this.delete(transactionId, delete);
        result = true;
      }
      else
        result = false;
    } else if ((!rs.isEmpty())
              && !valueIsNull
              && (Bytes.equals(rs.getValue(family, qualifier), value))) {
      this.delete(transactionId, delete);
      result = true;
    } else {
      result = false;
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndDelete  setting result is " + result + " row: " + row);
    }

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndDelete  EXIT result is " + result + " row: " + row);

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
  public boolean checkAndPut(long transactionId, byte[] row, byte[] family,
                            byte[] qualifier, byte[] value, Put put)
    throws IOException {

    TransactionState state = this.beginTransIfNotExist(transactionId);
    boolean result = false;
    byte[] rsValue = null;

    Get get = new Get(row);
    get.addColumn(family, qualifier);
    
    Result rs = this.get(transactionId, get);
    if (LOG.isTraceEnabled()) LOG.trace("Enter TrxRegionEndpoint coprocessor: checkAndPut, txid: "
               + transactionId + ", result is empty " + rs.isEmpty() +
               ", value is " + Bytes.toString(value));

    boolean valueIsNull = value == null ||
                          value.length == 0;

    if (rs.isEmpty() && valueIsNull) {
      this.put(transactionId, put);
      result = true;
    } else if (!rs.isEmpty() && valueIsNull) {
      rsValue = rs.getValue(family, qualifier);
      if (rsValue != null && rsValue.length == 0) {
        this.put(transactionId, put);
        result = true;
      }
      else
        result = false;
    } else if ((!rs.isEmpty()) && !valueIsNull   
              && (Bytes.equals(rs.getValue(family, qualifier), value))) {
       this.put(transactionId, put);
       result = true;
    } else {
      result = false;
    }

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  checkAndPut returns " + result + " for row: " + row);

    return result;
  }

  /**
   * Obtains a transactional Result for Get          
   * @param long transactionId
   * @param Get get             
   * @return Result 
   * @throws IOException 
   */
  public Result get(final long transactionId, final Get get)
                          throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  get --  ENTRY txId: " + transactionId );
    
    /*
    if (LOG.isTraceEnabled()) {
	Map<byte[],NavigableSet<byte[]>> lv_fm = get.getFamilyMap();
	byte [][] lv_fms = lv_fm.keySet().toArray(new byte[0][0]);
	byte[] lv_f = lv_fms[0];
	if (LOG.isTraceEnabled()) LOG.trace("family: " + new String(lv_f));
	NavigableSet<byte []> lv_set = lv_fm.get(lv_f);
	if (LOG.isTraceEnabled()) LOG.trace("lv_set size: " + lv_set.size());
    }
    */

    Scan scan = new Scan(get);
    List<Cell> results = new ArrayList<Cell>();

    RegionScanner scanner = null;

    try {
      scanner = getScanner(transactionId, scan);
      scanner.next(results);       
    } catch(Exception e) {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  get Caught exception " + e.getMessage() + "" + stackTraceToString(e));
    }
    finally {
      if (scanner != null) {
        scanner.close();
      }
    }

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  get -- EXIT txId: " + transactionId);
    return Result.create(results);       
  }

  /**
   * Obtain a RegionScanner                        
   * @param long transactionId
   * @param Scan scan             
   * @return RegionScanner
   * @throws IOException 
   */
  public RegionScanner getScanner(final long transactionId, final Scan scan)
                        throws IOException { 

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  RegionScanner getScanner -- ENTRY txId: " + transactionId );

    TransactionState state = this.beginTransIfNotExist(transactionId);     

    state.addScan(scan);

    List<KeyValueScanner> scanners = new ArrayList<KeyValueScanner>(1);     

    scanners.add(state.getScanner(scan));

    Scan deleteWrapScan = wrapWithDeleteFilter(scan, state);
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  RegionScanner getScanner -- Calling t_Region.getScanner txId: " + transactionId );
    RegionScanner gotScanner =  this.t_Region.getScanner(deleteWrapScan, scanners); 
    if (gotScanner != null)
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  RegionScanner getScanner -- obtained scanner was not null,  txId: " + transactionId );
    else
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  RegionScanner getScanner -- obtained scanner was null,  txId: " + transactionId );
    return gotScanner;
  }

  /**
   * Wraps the transactional scan with a delete filter
   * @param Scan scan
   * @param TransactionState state
   * @return Scan 
   */
  private Scan wrapWithDeleteFilter(final Scan scan,
                                    final TransactionState state) {
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  wrapWithDeleteFilter -- ENTRY");
    FilterBase deleteFilter = new FilterBase() {

      private boolean rowFiltered = false;

      @Override
      public void reset() {
        rowFiltered = false;
      }

      @Override
      public boolean hasFilterRow() {
        return true;
      }

      @Override
      public void filterRowCells(final List<Cell> kvs) {
        state.applyDeletes(kvs, scan.getTimeRange().getMin(),
                           scan.getTimeRange().getMax());
        rowFiltered = kvs.isEmpty();
      }

      public boolean filterRow() {
        return rowFiltered;
      }

    };

    if (scan.getFilter() == null) {
        scan.setFilter(deleteFilter);
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  no previous filter, wrapWithDeleteFilter -- EXIT");
      return scan;
    }

    FilterList wrappedFilter = new FilterList(Arrays.asList(deleteFilter,
                                             scan.getFilter()));
    scan.setFilter(wrappedFilter);
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  new filter array, wrapWithDeleteFilter -- EXIT");

    return scan;
  }

  /**
   * Add a write to the transaction. Does not get applied until commit
   * process.
   * @param long transactionId
   * @param Put put
   * @throws IOException
   */

  public synchronized void put(final long transactionId, final Put put)
    throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: put - txid " + transactionId);
    if (LOG.isTraceEnabled()) LOG.trace("Enter TrxRegionEndpoint coprocessor: put, txid: " + transactionId);
    TransactionState state = this.beginTransIfNotExist(transactionId);

    state.addWrite(put);
  }

  public void constructIndoubtTransactions() {

      synchronized (recoveryCheckLock) {
            if ((indoubtTransactionsById == null) || (indoubtTransactionsById.size() == 0)) {
              if (LOG.isDebugEnabled()) LOG.debug("Trafodion Recovery Endpoint Coprocessor: Region " + regionInfo.getRegionNameAsString() + " has no in-doubt transaction, set region START ");
              regionState = 2; // region is started for transactional access
              reconstructIndoubts = 1; 
              try {
              startRegionAfterRecovery();
               } catch (IOException exp1) {
                    LOG.debug("Trafodion Recovery: flush error during region start");
               }
              return;
            }

            if (LOG.isDebugEnabled()) LOG.debug("Trafodion Recovery Endpoint Coprocessor: Trafodion Recovery RegionObserver to Endpoint coprocessor data exchange test");
            if (LOG.isDebugEnabled()) LOG.debug("Trafodion Recovery Endpoint Coprocessor: try to access indoubt transaction list with size " + indoubtTransactionsById.size());

            if (reconstructIndoubts == 0) {
            //Retrieve (tid,Edits) from indoubt Transaction and construct/add into desired transaction data list
            for (Entry<Long, WALEdit> entry : indoubtTransactionsById.entrySet()) {
                      long transactionId = entry.getKey();
		      String key = String.valueOf(transactionId);
                      if (LOG.isDebugEnabled()) LOG.debug("Trafodion Recovery Endpoint Coprocessor:E11 Region " + regionInfo.getRegionNameAsString() + " process in-doubt transaction " + transactionId);

                      //TBD Need to get HLOG ???
		      TransactionState state = new TransactionState(transactionId, 1L /*my_Region.getLog().getSequenceNumber()*/,
                                                                                 regionInfo, m_Region.getTableDesc());

                      if (LOG.isDebugEnabled()) LOG.debug("Trafodion Recovery Endpoint Coprocessor:E22 Region " + regionInfo.getRegionNameAsString() + " create transaction state for " + transactionId);

		      state.setStartSequenceNumber(nextSequenceId.get());
    		      transactionsById.put(getTransactionalUniqueId(transactionId), state);

                      state.setReinstated();
		      state.setStatus(Status.COMMIT_PENDING);
		      commitPendingTransactions.add(state);
		      state.setSequenceNumber(nextSequenceId.getAndIncrement());
		      commitedTransactionsBySequenceNumber.put(state.getSequenceNumber(), state);
                      int tmid = (int) (transactionId >> 32);
                      if (LOG.isDebugEnabled()) LOG.debug("Trafodion Recovery Endpoint Coprocessor:E33 Region " + regionInfo.getRegionNameAsString() + " add prepared " + transactionId + " to TM " + tmid);              
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
   * Begin a transaction
   * @param longtransactionId
   * @throws IOException
   */

  public void beginTransaction(final long transactionId)
     throws IOException {

    TransactionState state = null; 

    if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor:  beginTransaction -- ENTRY txId: " + transactionId);
    checkClosing(transactionId);
                
    // TBD until integration with recovery 
    if (reconstructIndoubts == 0) {
       LOG.debug("TrxRegionEndpoint coprocessor:  RECOV beginTransaction -- ENTRY txId: " + transactionId);
       constructIndoubtTransactions();
    }
    if (regionState != 2) {
       if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: Trafodion Recovery: RECOVERY WARN beginTransaction while the region is still in recovering state " +  regionState);
    }

    synchronized (transactionsById) {
      if (transactionsById.get(getTransactionalUniqueId(transactionId)) != null) {
      TransactionState alias = getTransactionState(transactionId);

      LOG.error("TrxRegionEndpoint coprocessor:  beginTransaction - Ignoring - Existing transaction with id ["
          + transactionId + "] in region ["
          + m_Region.getRegionInfo().getRegionNameAsString() + "]");
                         
      if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor:  beginTransaction -- EXIT txId: " + transactionId);

      return;
    }

      if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor:  beginTransaction -- creating new TransactionState without coprocessorHost txId: " + transactionId);

      state = new TransactionState(transactionId, 
                                //this.m_Region.getLog().getSequenceNumber(),
                                nextLogSequenceId.getAndIncrement(),
                                m_Region.getRegionInfo(),
                                m_Region.getTableDesc());

    state.setStartSequenceNumber(nextSequenceId.get());
    }

    List<TransactionState> commitPendingCopy = 
        new ArrayList<TransactionState>(commitPendingTransactions);

    for (TransactionState commitPending : commitPendingCopy) {
            state.addTransactionToCheck(commitPending);
    }

    synchronized (transactionsById) {
      transactionsById.put(getTransactionalUniqueId(transactionId), state);
                         
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  beginTransaction - Adding transaction: [" + transactionId + "] in region ["
                + m_Region.getRegionInfo().getRegionNameAsString() + "]" +
                 " to list");

    try {
	transactionLeases.createLease(getTransactionalUniqueId(transactionId),
				      transactionLeaseTimeout,
				      new TransactionLeaseListener(transactionId));
    } catch (LeaseStillHeldException e) {
      LOG.error("TrxRegionEndpoint coprocessor:  beginTransaction - Lease still held for [" + transactionId + "] in region ["
                + m_Region.getRegionInfo().getRegionNameAsString() + "]");
        throw new RuntimeException(e);
    }

    if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor:  beginTransaction -- EXIT txId: " + transactionId + " transactionsById size: " + transactionsById.size()
    		 + " region ID: " + this.regionInfo.getRegionId());
  }
  }

  /**
   * Obtains a scanner lease id                            
   * @param long scannerId
   * @return String 
   */
  private String getScannerLeaseId(final long scannerId) {
    String lstring = m_Region.getRegionInfo().getRegionNameAsString() + scannerId;

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  getScannerLeaseId -- EXIT txId: " 
             + scannerId + " lease string " + lstring);
    return m_Region.getRegionInfo().getRegionNameAsString() + scannerId;
  }
                                                             
  /**
   * Obtains a transactional lease id                            
   * @param long transactionId
   * @return String 
   */
  private String getTransactionalUniqueId(final long transactionId) {
    String lstring = m_Region.getRegionInfo().getRegionNameAsString() + transactionId;

    synchronized (transactionsById) {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  getTransactionalUniqueId -- EXIT txId: " 
             + transactionId + " transactionsById size: "
             + transactionsById.size() + " name " + lstring);
    }
    return m_Region.getRegionInfo().getRegionNameAsString() + transactionId;
  }
                                                             

  /**begin transaction if not yet
    * @param transactionId
    * @return true: begin; false: not necessary to begin
    * @throws IOException 
   */
  private TransactionState beginTransIfNotExist(long transactionId)
      throws IOException{

    TransactionState stateR = null;

    synchronized (transactionsById) {
    if (LOG.isTraceEnabled()) LOG.trace("Enter TrxRegionEndpoint coprocessor: beginTransIfNotExist, txid: "
              + transactionId + " transactionsById size: "
              + transactionsById.size());
    TransactionState state = null;

      state = transactionsById.get(getTransactionalUniqueId(transactionId));

    if (state == null) {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Begin transaction in beginTransIfNotExist beginning the transaction internally as state was null");
      this.beginTransaction(transactionId);
    }

      stateR =  transactionsById.get(getTransactionalUniqueId(transactionId));
    }
    return stateR;
  }
 
  /**
   * Commits the transaction                        
   * @param long TransactionId
   * @throws IOException 
   */
  public void commit(final long transactionId) throws IOException {
    if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: commit(txId) -- ENTRY txId: " + transactionId);
    TransactionState state;
    try {
      state = getTransactionState(transactionId);
    } catch (UnknownTransactionException e) {
      LOG.fatal("TrxRegionEndpoint coprocessor:  Asked to commit unknown transaction: " + transactionId
                + " in region "
                + m_Region.getRegionInfo().getRegionNameAsString());
      throw new IOException("UnknownTransactionException");
    }

    if (!state.getStatus().equals(Status.COMMIT_PENDING)) {
      LOG.fatal("TrxRegionEndpoint coprocessor: commit - Asked to commit a non pending transaction ");

      throw new IOException("Asked to commit a non-pending transaction");
    }

    if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: commit(txId) -- EXIT txId: " + transactionId);
    commit(state);
  } 

  /**
   * @param transactionId
   * @return TransactionRegionInterface commit code
   * @throws IOException
   */
  public int commitRequest(final long transactionId) throws IOException {
    long txid = 0;
    if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: commitRequest -- ENTRY txId: " + transactionId);
    checkClosing(transactionId);
    TransactionState state;

    synchronized (commitCheckLock) {
    try {
      state = getTransactionState(transactionId);
    } catch (UnknownTransactionException e) {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: Unknown transaction [" + transactionId
                 + "] in region [" 
                 + m_Region.getRegionInfo().getRegionNameAsString()
                 + "], ignoring");
     state = null;
    }
      // may change to indicate a NOTFOUND case  then depends on the TM ts state, if reinstated tx, ignore the exception
      if (state == null) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitRequest encountered unknown transactionID txId: " + transactionId + " returning COMMIT_UNSUCCESSFUL");
        return COMMIT_UNSUCCESSFUL;
      }

      if (hasConflict(state)) {
        state.setStatus(Status.ABORTED);
        retireTransaction(state);
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitRequest encountered conflict txId: " + transactionId + "returning COMMIT_CONFLICT");
        return COMMIT_CONFLICT;
      }

      // No conflicts, we can commit.
      if (LOG.isTraceEnabled()) LOG.trace("No conflicts for transaction " + transactionId
		+ " found in region "
		+ m_Region.getRegionInfo().getRegionNameAsString()
		+ ". Voting for commit");

      // If there are writes we must keep record of the transaction
      if (state.hasWrite()) {
        // Order is important
	state.setStatus(Status.COMMIT_PENDING);
	commitPendingTransactions.add(state);
	state.setSequenceNumber(nextSequenceId.getAndIncrement());
	commitedTransactionsBySequenceNumber.put(
	state.getSequenceNumber(), state);
      }
    } // exit sync block of commitCheckLock
                
    if (state.hasWrite()) {
      if (LOG.isTraceEnabled()) LOG.trace("write commitRequest edit to HLOG");
      //call HLog by passing tagged WALEdits and associated fields
      try {
             txid = this.tHLog.appendNoSync(this.regionInfo, this.regionInfo.getTable(),
                  state.getEdit(), new ArrayList<UUID>(), EnvironmentEdgeManager.currentTimeMillis(), this.m_Region.getTableDesc(),
                  nextLogSequenceId, false, HConstants.NO_NONCE, HConstants.NO_NONCE);
             this.tHLog.sync(txid);
             if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor:commitRequest COMMIT_OK -- EXIT txId: " + transactionId + " HLog seq " + txid);
      } catch (IOException exp) {
          LOG.info("TrxRegionEndpoint coprocessor:commitRequest IOException caught in HLOG appendNoSync -- EXIT txId: " + transactionId + " HLog seq " + txid);
          throw exp;
      }      
      if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: commitRequest COMMIT_OK -- EXIT txId: " + transactionId);
      return COMMIT_OK;
    }

    // Otherwise we were read-only and commitable, so we can forget it.
    state.setStatus(Status.COMMITED);
    retireTransaction(state);
    if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: commitRequest READ ONLY -- EXIT txId: " + transactionId);
    return COMMIT_OK_READ_ONLY;
  }

  /**
   * Determines if the transaction has any conflicts
   * @param TransactionState state
   * @return boolean
   */
  private boolean hasConflict(final TransactionState state) {
    // Check transactions that were committed while we were running
      
    synchronized (commitedTransactionsBySequenceNumber) {
    for (int i = state.getStartSequenceNumber(); i < nextSequenceId.get(); i++)
    {
      TransactionState other = commitedTransactionsBySequenceNumber.get(i);
      if (other == null) {
        continue;
      }
      state.addTransactionToCheck(other);
    }
    }

    return state.hasConflict();
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
    if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: abort transactionId: " + transactionId + " " + m_Region.getRegionInfo().getRegionNameAsString());

    TransactionState state;
    try {
      state = getTransactionState(transactionId);
    } catch (UnknownTransactionException e) {
      IOException ioe = new IOException("UnknownTransactionException");
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: Unknown transaction [" + transactionId
                 + "] in region [" 
                 + m_Region.getRegionInfo().getRegionNameAsString()
                 + "], " + ioe.toString());
      
      throw new IOException("UnknownTransactionException");
    }

    state.setStatus(Status.ABORTED);

    if (state.hasWrite()) {
    // TODO log
    //  this.transactionLog.writeAbortToLog(m_Region.getRegionInfo(),
    //                                      state.getTransactionId(),
    //                                    m_Region.getTableDesc());
       if (LOG.isTraceEnabled()) LOG.trace("abort write to HLOG");
       Tag abortTag = state.formTransactionalContextTag(ABORT);
       List<Tag> tagList = new ArrayList<Tag>();
       tagList.add(abortTag);

       //e.add(new KeyValue(new byte[0], null, null, HConstants.LATEST_TIMESTAMP, null, tagList)); // Empty KeyValue

      WALEdit e1 = state.getEdit();
      WALEdit e = new WALEdit();

      // SST get 1st Cell to associatyed with the abort record as a workaround through HLOG async append
      Cell c = e1.getKeyValues().get(0);
      KeyValue kv = new KeyValue(c.getRowArray(), c.getRowOffset(), (int)c.getRowLength(),
      c.getFamilyArray(), c.getFamilyOffset(), (int)c.getFamilyLength(),
      c.getQualifierArray(), c.getQualifierOffset(), (int) c.getQualifierLength(),
      c.getTimestamp(), Type.codeToType(c.getTypeByte()), c.getValueArray(), c.getValueOffset(),
      c.getValueLength(), tagList);
      
      e.add(kv);
      try {
             txid = this.tHLog.appendNoSync(this.regionInfo, this.regionInfo.getTable(),
                  e, new ArrayList<UUID>(), EnvironmentEdgeManager.currentTimeMillis(), this.m_Region.getTableDesc(),
                  nextLogSequenceId, false, HConstants.NO_NONCE, HConstants.NO_NONCE);
      }
      catch (IOException exp1) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor abort writing to HLOG : Threw an exception");
        throw exp1;
       }
       if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor:abort -- EXIT txId: " + transactionId + " HLog seq " + txid);
       //TBD: no need to do this.tHLog.sync(txid) for phase 2 due to TLOG enabled
    }

    synchronized (commitPendingTransactions) {
    commitPendingTransactions.remove(state);
    }

    if (state.isReinstated()) {
      synchronized(indoubtTransactionsById) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: Trafodion Recovery: abort reinstated indoubt transactions " + transactionId);
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
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: Trafodion Recovery: delete in abort recovery zNode TM " + tmid + " region encoded name " + lv_encoded + " for 0 in-doubt transaction");
            deleteRecoveryzNode(tmid, lv_encoded);
          } catch (IOException e) {
             LOG.error("TrxRegionEndpoint coprocessor: Trafodion Recovery: delete recovery zNode failed");
          }
         }

         if ((indoubtTransactionsById == null) || 
             (indoubtTransactionsById.size() == 0)) {
           // change region state to STARTED, and archive the split-thlog

           if (indoubtTransactionsById == null)
             if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: Trafodion Recovery: start region in abort with indoubtTransactionsById null");
            else
              if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: Trafodion Recovery: start region in abort with indoubtTransactionsById size " + indoubtTransactionsById.size());
            startRegionAfterRecovery();
         }
       }
     }

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

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  commitIfPossible -- ENTRY txId: "
               + transactionId);
    int status = commitRequest(transactionId);
  
    if (status == COMMIT_OK) {

       // Process local memory
       try {
         commit(transactionId);
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  commitIfPossible -- ENTRY txId: " + transactionId + " COMMIT_OK");
         return true;
       } catch (Throwable e) {
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:coprocesor: commitIfPossible threw exception after internal commit");
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  Caught exception " + e.getMessage() + "" + stackTraceToString(e));
        throw new IOException(e.toString());
       }
    } else if (status == COMMIT_OK_READ_ONLY) {
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  commitIfPossible -- ENTRY txId: " 
            + transactionId + " COMMIT_OK_READ_ONLY");
            return true;
    }
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  commitIfPossible -- ENTRY txId: " 
              + transactionId + " Commit Unsuccessful");
    return false;
  }
  
  /**
   * Formats a cleanup message for a Throwable
   * @param Throwable t
   * @param String msg
   * @return Throwable
   */
  private Throwable cleanup(final Throwable t, final String msg) {
    if (t instanceof NotServingRegionException) {
      if (LOG.isDebugEnabled()) LOG.debug("NotServingRegionException; " +  t.getMessage());
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

      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getScanner scanners map is " + scanners + ", count is "  + scanners.size() + ", scanner id is " + scannerId);

      TransactionalRegionScannerHolder rsh = 
        scanners.get(scannerId);

      if (rsh != null)
      {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getScanner rsh is " + rsh + "rsh.s is "  + rsh.s );
      }
      else
      {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getScanner rsh is null");
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
        throw new OutOfOrderScannerNextException(
        "Expected nextCallSeq: " + rsh.nextCallSeq +
        " But the nextCallSeq got from client: " + nextCallSeq); 
      }

      return scanner;
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

      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: removeScanner scanners map is " + scanners + ", count is "  + scanners.size());
      TransactionalRegionScannerHolder rsh = 
        scanners.remove(scannerId);
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: removeScanner scanners map is " + scanners + ", count is "  + scanners.size());
      if (rsh != null)
      {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: removeScanner rsh is " + rsh + "rsh.s is"  + rsh.s );
        return rsh.s;
      }
      else
      {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: removeScanner rsh is null");
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
  protected synchronized long addScanner(RegionScanner s, HRegion r)
     throws LeaseStillHeldException {
    long scannerId = performScannerId.getAndIncrement();

    TransactionalRegionScannerHolder rsh = 
      new TransactionalRegionScannerHolder(s,r);

    if (rsh != null)
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: scannerId is " + scannerId + ", addScanner rsh is " + rsh);
    else
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: scannerId is " + scannerId + ", addScanner rsh is null");
  
    TransactionalRegionScannerHolder existing =
      scanners.putIfAbsent(scannerId, rsh);

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: addScanner scanners map is " + scanners + ", count is "  + scanners.size());

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

      Integer minStartSeqNumber = getMinStartSequenceNumber();

      if (minStartSeqNumber == null) {
        minStartSeqNumber = Integer.MAX_VALUE;  
      }

      int numRemoved = 0;
	 
      synchronized (commitedTransactionsBySequenceNumber) {
      for (Entry<Integer, TransactionState> entry : new LinkedList<Entry<Integer, TransactionState>>(
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
   * @return Integer
   */
  private Integer getMinStartSequenceNumber() {

    List<TransactionState> transactionStates;

    synchronized (transactionsById) {
      transactionStates = new ArrayList<TransactionState>(
      transactionsById.values());
    }

    Integer min = null;

    for (TransactionState transactionState : transactionStates) {
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
