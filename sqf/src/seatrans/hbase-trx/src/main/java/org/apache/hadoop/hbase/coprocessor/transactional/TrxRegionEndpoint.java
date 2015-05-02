// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
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
import java.lang.management.ManagementFactory;
import java.lang.management.MemoryUsage;
import java.lang.management.MemoryMXBean;
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
import java.util.ListIterator;
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
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;
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
import org.apache.hadoop.hbase.client.transactional.MemoryUsageException;
import org.apache.hadoop.hbase.client.transactional.OutOfOrderProtocolException;
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
import org.apache.hadoop.hbase.regionserver.transactional.MemoryUsageChore;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionalRegion;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionalRegionScannerHolder;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionState;
import org.apache.hadoop.hbase.regionserver.transactional.TrxTransactionState;
import org.apache.hadoop.hbase.regionserver.transactional.TrxTransactionState.TransactionScanner;
import org.apache.hadoop.hbase.regionserver.transactional.TrxTransactionState.WriteAction;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionState.CommitProgress;
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
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.AbortTransactionMultipleRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.AbortTransactionMultipleResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitMultipleRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitMultipleResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequestMultipleRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequestMultipleResponse;


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
  protected ConcurrentHashMap<String, TrxTransactionState> transactionsById = new ConcurrentHashMap<String, TrxTransactionState>();

  // Map of recent transactions that are COMMIT_PENDING or COMMITED keyed 
  // by their sequence number

  private SortedMap<Long, TrxTransactionState> commitedTransactionsBySequenceNumber = Collections.synchronizedSortedMap(new TreeMap<Long, TrxTransactionState>());

  // Collection of transactions that are COMMIT_PENDING
  private Set<TrxTransactionState> commitPendingTransactions = Collections.synchronizedSet(new HashSet<TrxTransactionState>());

  // an in-doubt transaction list during recovery WALEdit replay
  private Map<Long, List<WALEdit>> indoubtTransactionsById = new TreeMap<Long, List<WALEdit>>();

  // list of transactions to check for stale scanners
     private List<Long> cleanScannersForTransactions = Collections.synchronizedList(new LinkedList<Long>());
    
  // an in-doubt transaction list count by TM id
  private Map<Integer, Integer> indoubtTransactionsCountByTmid = new TreeMap<Integer,Integer>();

  // Concurrent map for transactional region scanner holders
  // Protected by synchronized methods
  final ConcurrentHashMap<Long,
                          TransactionalRegionScannerHolder> scanners =
      new ConcurrentHashMap<Long, TransactionalRegionScannerHolder>();

  // Atomic values to manage region scanners
  private AtomicLong performScannerId = new AtomicLong(0);
  private AtomicLong nextSequenceId = new AtomicLong(0);

  private Object commitCheckLock = new Object();
  private Object recoveryCheckLock = new Object();
  private Object editReplay = new Object();
  private static Object stoppableLock = new Object();
  private int reconstructIndoubts = 0; 
  //temporary THLog getSequenceNumber() replacement
  private AtomicLong nextLogSequenceId = new AtomicLong(0);
  public AtomicLong controlPointEpoch = new AtomicLong(1);
  private final int oldTransactionFlushTrigger = 0;
  private final Boolean splitDelayEnabled = false;
  private final Boolean doWALHlog = false;
  static Leases transactionLeases = null;
  // Joanie: commenting out scanner leases for now
  //static Leases scannerLeases = null;
  CleanOldTransactionsChore cleanOldTransactionsThread;
  static MemoryUsageChore memoryUsageThread = null;
  Stoppable stoppable = new StoppableImplementation();
  static Stoppable stoppable2 = new StoppableImplementation();
  private int cleanTimer = 5000; // Five minutes
  private int memoryUsageTimer = 60000; // One minute   
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
  private AtomicBoolean closing = new AtomicBoolean(false);
  private boolean fullEditInCommit = true;
  private boolean configuredEarlyLogging = false;
  private boolean configuredConflictReinstate = false;
  private static Object zkRecoveryCheckLock = new Object();
  private static ZooKeeperWatcher zkw1 = null;
  String lv_hostName;
  int lv_port;
  private static String zNodePath = "/hbase/Trafodion/recovery/";

  private static final int MINIMUM_LEASE_TIME = 7200 * 1000;
  private static final int LEASE_CHECK_FREQUENCY = 1000;
  private static final int DEFAULT_SLEEP = 60 * 1000;
  private static final int DEFAULT_MEMORY_THRESHOLD = 100; // 100% memory used
  private static final int DEFAULT_MEMORY_SLEEP = 15 * 1000;
  private static final boolean DEFAULT_MEMORY_WARN_ONLY = true;        
  private static final boolean DEFAULT_MEMORY_PERFORM_GC = false;
  private static final boolean DEFAULT_SUPPRESS_OOP = false;
  private static final String SLEEP_CONF = "hbase.transaction.clean.sleep";
  private static final String LEASE_CONF  = "hbase.transaction.lease.timeout";
  private static final String MEMORY_THRESHOLD = "hbase.transaction.memory.threshold";
  private static final String MEMORY_WARN_ONLY = "hbase.transaction.memory.warn.only";
  private static final String MEMORY_CONF = "hbase.transaction.memory.sleep";
  private static final String MEMORY_PERFORM_GC = "hbase.transaction.memory.perform.GC";
  private static final String SUPPRESS_OOP = "hbase.transaction.suppress.OOP.exception";
  protected static int transactionLeaseTimeout = 0;
  private static int scannerLeaseTimeoutPeriod = 0;
  private static int scannerThreadWakeFrequency = 0;
  private static int memoryUsageThreshold = DEFAULT_MEMORY_THRESHOLD;
  private static boolean memoryUsagePerformGC = DEFAULT_MEMORY_PERFORM_GC;
  private static boolean memoryUsageWarnOnly = DEFAULT_MEMORY_WARN_ONLY;
  private static MemoryMXBean memoryBean = null;
  private static float memoryPercentage = 0;
  private static boolean memoryThrottle = false;
  private static boolean suppressOutOfOrderProtocolException = DEFAULT_SUPPRESS_OOP;

  // Transaction state defines
  private static final int COMMIT_OK = 1;
  private static final int COMMIT_OK_READ_ONLY = 2;
  private static final int COMMIT_UNSUCCESSFUL_FROM_COPROCESSOR = 3;
  private static final int COMMIT_CONFLICT = 5;

  private static final int CLOSE_WAIT_ON_COMMIT_PENDING = 1000;
  private static final int MAX_COMMIT_PENDING_WAITS = 10;
  private Thread ChoreThread = null;
  private static Thread ChoreThread2 = null;
  //private static Thread ScannerLeasesThread = null;
  private static Thread TransactionalLeasesThread = null;

  public static final int TS_ACTIVE = 0;
  public static final int TS_COMMIT_REQUEST = 1;
  public static final int TS_COMMIT = 2;
  public static final int TS_ABORT = 3;
  public static final int TS_CONTROL_POINT_COMMIT = 4;

  public static final int REGION_STATE_RECOVERING = 0;
  public static final int REGION_STATE_START = 2;

  // TrxRegionService methods
    
  @Override
  public void abortTransaction(RpcController controller,
                                AbortTransactionRequest request,
      RpcCallback<AbortTransactionResponse> done) {
    AbortTransactionResponse response = AbortTransactionResponse.getDefaultInstance();

    long transactionId = request.getTransactionId();

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: abortTransaction - txId " + transactionId + ", regionName " + regionInfo.getRegionNameAsString());

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
        abortTransaction(transactionId);
      } catch (UnknownTransactionException u) {
        if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor:abort - txId " + transactionId + ", Caught UnknownTransactionException after internal abortTransaction call - " + u.getMessage() + " " + stackTraceToString(u));
       ute = u;
      } catch (IOException e) {
        if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor:abort - txId " + transactionId + ", Caught IOException after internal abortTransaction call - " + e.getMessage() + " " + stackTraceToString(e));
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

    Throwable t = null;
    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();
    WrongRegionException wre = null;
    MemoryUsageException mue = null;
    long transactionId = request.getTransactionId();

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: beginTransaction - txId "  + transactionId + ", regionName " + regionInfo.getRegionNameAsString());

    // First test if this region matches our region name

    /* commenting it out for the time-being
    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:beginTransaction threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
     }else 
    */
    {
      if (memoryThrottle == true) {
        if(memoryUsageWarnOnly == true)  {
          LOG.warn("TrxRegionEndpoint coprocessor: beginTransaction - performing memoryPercentage " + memoryPercentage + ", warning memory usage exceeds indicated percentage");
        }
        else {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: beginTransaction - performing memoryPercentage " + memoryPercentage + ", generating memory usage exceeds indicated percentage");
          mue = new MemoryUsageException("beginTransaction memory usage exceeds " + memoryUsageThreshold + " percent, trxId is " + transactionId);
        }
      }
      else
      {
        try {
          beginTransaction(transactionId);
        } catch (Throwable e) {
           if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: beginTransaction - txId " + transactionId + ", Caught exception after internal beginTransaction call "
                           + e.getMessage() + " " + stackTraceToString(e));
           t = e;
        }        
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

    if (mue != null)
    {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: beginTransaction - performing memoryPercentage " + memoryPercentage + ", posting memory usage exceeds indicated percentage");
      beginTransactionResponseBuilder.setHasException(true);
      beginTransactionResponseBuilder.setException(mue.toString());
    }

    BeginTransactionResponse bresponse = beginTransactionResponseBuilder.build();

    done.run(bresponse);
  }

  @Override
  public void commit(RpcController controller,
                     CommitRequest request,
      RpcCallback<CommitResponse> done) {
    CommitResponse response = CommitResponse.getDefaultInstance();

    Throwable t = null;
    WrongRegionException wre = null;
    long transactionId = request.getTransactionId();

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit - txId "  + transactionId + ", regionName " + regionInfo.getRegionNameAsString());

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
        commit(transactionId, request.getIgnoreUnknownTransactionException());
      } catch (Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit - txId " + transactionId + ", Caught exception after internal commit call "
                     + e.getMessage() + " " + stackTraceToString(e));
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
    long transactionId = request.getTransactionId();
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
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitIfPossible - txId "  + transactionId + ", regionName, " + regionInfo.getRegionNameAsString() + "calling internal commitIfPossible");
         reply = commitIfPossible(transactionId);
       } catch (Throwable e) {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitIfPossible - txId " + transactionId + ", Caught exception after internal commitIfPossible call "
                   + e.getMessage() + " " + stackTraceToString(e));
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

    int status = 0;
    IOException ioe = null;
    UnknownTransactionException ute = null;
    Throwable t = null;
    WrongRegionException wre = null;
    long transactionId = request.getTransactionId();

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitRequest - txId "  + transactionId + ", regionName " + regionInfo.getRegionNameAsString());

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
        status = commitRequest(transactionId);
      } catch (UnknownTransactionException u) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitRequest - txId " + transactionId + ", Caught UnknownTransactionException after internal commitRequest call - " + u.toString());
        ute = u;
      } catch (IOException e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitRequest - txId " + transactionId + ", Caught IOException after internal commitRequest call - "+ e.toString());
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
    MutationProto proto = request.getDelete();
    MutationType type = proto.getMutateType();
    Delete delete = null;
    Throwable t = null;
    MemoryUsageException mue = null;
    WrongRegionException wre = null;
    boolean result = false;
    long transactionId = request.getTransactionId();

    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();

    // First test if this region matches our region name

    /* commenting it out for the time-being

    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:checkAndDelete threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
   }
    */

    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CheckAndDeleteResponse.Builder checkAndDeleteResponseBuilder = CheckAndDeleteResponse.newBuilder();

      if (memoryThrottle == true) {
        if(memoryUsageWarnOnly == true)  {
          LOG.warn("TrxRegionEndpoint coprocessor: checkAndDelete - performing memoryPercentage " + memoryPercentage + ", warning memory usage exceeds indicated percentage");
        }
        else {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndDelete - performing memoryPercentage " + memoryPercentage + ", generating memory usage exceeds indicated percentage");
          mue = new MemoryUsageException("checkAndDelete memory usage exceeds " + memoryUsageThreshold + " percent, trxId is " + transactionId);
        }
      }

    if (mue == null && 
        wre == null && 
        type == MutationType.DELETE && 
        proto.hasRow())
    {
      try {
          delete = ProtobufUtil.toDelete(proto);
      } catch (Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndDelete - txId " + transactionId + ", Caught exception " + e.getMessage() + " " + stackTraceToString(e));
        t = e;
      }

      // Process in local memory
      if (delete != null && t == null)
      {
        if (request.hasRow()) {

        if (!Bytes.equals(proto.getRow().toByteArray(), request.getRow().toByteArray()))
          t = new org.apache.hadoop.hbase.DoNotRetryIOException("Action's " +
          "Delete row must match the passed row");
        }

        if (t == null) {
      
          try {
           result = checkAndDelete(transactionId,
               request.getRow().toByteArray(),
               request.getFamily().toByteArray(),
               request.getQualifier().toByteArray(),
               request.getValue().toByteArray(),
               delete);
           } catch (Throwable e) {
             if (LOG.isInfoEnabled()) LOG.info("TrxRegionEndpoint coprocessor: checkAndDelete - txId " + transactionId + ", Caught exception after internal checkAndDelete call - "+ e.getMessage() + " " + stackTraceToString(e));
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

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndDelete - txId " + transactionId + ", result is " + result);

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

    if (mue != null)
    {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndDelete - performing memoryPercentage " + memoryPercentage + ", posting memory usage exceeds indicated percentage");
      checkAndDeleteResponseBuilder.setHasException(true);
      checkAndDeleteResponseBuilder.setException(mue.toString());
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
    MemoryUsageException mue = null;
    WrongRegionException wre = null;
    Throwable t = null;
    boolean result = false;
    long transactionId = request.getTransactionId();

    /* commenting it out for the time-being
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
    */

    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CheckAndPutResponse.Builder checkAndPutResponseBuilder = CheckAndPutResponse.newBuilder();

    if (memoryThrottle == true) {
        if(memoryUsageWarnOnly == true)  {
          LOG.warn("TrxRegionEndpoint coprocessor: checkAndPut - performing memoryPercentage " + memoryPercentage + ", warning memory usage exceeds indicated percentage");
        }
        else {
          mue = new MemoryUsageException("checkAndPut memory usage exceeds " + memoryUsageThreshold + " percent, trxId is " + transactionId);
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndPut - performing memoryPercentage " + memoryPercentage + ", generating memory usage exceeds indicated percentage exception");
        }
    }

    if (mue == null &&
        wre == null && 
        type == MutationType.PUT && 
        proto.hasRow())
    {
      rowArray = proto.getRow().toByteArray();

      try {
          put = ProtobufUtil.toPut(proto);
      } catch (Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndPut - txId " + transactionId + ", Caught exception " + e.getMessage() + " " + stackTraceToString(e));
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
           result = checkAndPut(transactionId,
               request.getRow().toByteArray(),
               request.getFamily().toByteArray(),
               request.getQualifier().toByteArray(),
               request.getValue().toByteArray(),
               put);
           } catch (Throwable e) {
             if (LOG.isInfoEnabled()) LOG.info("TrxRegionEndpoint coprocessor: checkAndPut - txId " + transactionId + ", Caught exception after internal checkAndPut call - "
                          + e.getMessage() + " " + stackTraceToString(e));
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

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndPut - txId " + transactionId + ", result is " + result);

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

    if (mue != null)
    {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndPut - performing memoryPercentage " + memoryPercentage + ", posting memory usage exceeds indicated percentage exception");
      checkAndPutResponseBuilder.setHasException(true);
      checkAndPutResponseBuilder.setException(mue.toString());
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
    OutOfOrderProtocolException oop = null;
    WrongRegionException wre = null;
    Exception ce = null;
    long transId = request.getTransactionId();
    long scannerId = request.getScannerId();

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: closeScanner - txId " + transId + ", scanner id " + scannerId + ", regionName " + regionInfo.getRegionNameAsString());

    /* commenting it out for the time-being
    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();

    // First test if this region matches our region name
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

    // There should be a matching key in the transactionsById map
    // associated with this transaction id.  If there is not
    // one, then the initial openScanner call for the transaction
    // id was not called.  This is a protocol error requiring
    // openScanner, performScan followed by a closeScanner.

    String key = getTransactionalUniqueId(transId);
    boolean keyFound = transactionsById.containsKey(key);

    if (keyFound != true)
    {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: closeScanner - Unknown transaction [" + transId
             + "] in region ["
             + m_Region.getRegionInfo().getRegionNameAsString()
             + "], will create an OutOfOrderProtocol exception ");
      oop = new OutOfOrderProtocolException("closeScanner does not have an active transaction with an open scanner, txId: " + transId);
    }

    if (oop == null) {
      try {
         scanner = removeScanner(scannerId);

         if (scanner != null) { 
             scanner.close();
         }
         else
           if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: closeScanner - txId " + transId + ", scanner was null for scanner id " + scannerId);

/*
         try {
           scannerLeases.cancelLease(getScannerLeaseId(scannerId));
         } catch (LeaseException le) {
           // ignore
           if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: closeScanner failed to get a lease " + scannerId);
         }
*/

      } catch(Exception e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: closeScanner - txId " + transId + ", Caught exception " + e.getMessage() + " " + stackTraceToString(e));
        ce = e;
      } catch(Throwable e) {
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: closeScanner - txId " + transId + ", Caught exception " + e.getMessage() + " " + stackTraceToString(e));
         t = e;
      }
    }

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

    if (oop != null)
    {
      if (this.suppressOutOfOrderProtocolException == false)
      {
        closeResponseBuilder.setHasException(true);
        closeResponseBuilder.setException(oop.toString());
        LOG.warn("TrxRegionEndpoint coprocessor: closeScanner - OutOfOrderProtocolException, transaction was not found, txId: " + transId + ",returned exception" + ", regionName " + regionInfo.getRegionNameAsString());
      }
      else
        LOG.warn("TrxRegionEndpoint coprocessor: closeScanner - suppressing OutOfOrderProtocolException, transaction was not found, txId: " + transId + ", regionName " + regionInfo.getRegionNameAsString());
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
   MemoryUsageException mue = null;
   WrongRegionException wre = null;
   long transactionId = request.getTransactionId();

    /* commenting it out for the time-being
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
    */
   if (memoryThrottle == true) {
        if(memoryUsageWarnOnly == true)  {
          LOG.warn("TrxRegionEndpoint coprocessor: deleteMultiple - performing memoryPercentage " + memoryPercentage + ", warning memory usage exceeds indicated percentage");
        }
        else {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: deleteMultiple - performing memoryPercentage " + memoryPercentage + ", generating memory usage exceeds indicated percentage exception");
          mue = new MemoryUsageException("deleteMultiple memory usage exceeds " + memoryUsageThreshold + " percent, trxId is " + transactionId);
       }
   }

   if (mue == null && wre == null) {
     for (org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto proto : results)
     { 
       delete = null;

       if (proto != null)
       {
         type = proto.getMutateType();

         if (type == MutationType.DELETE && proto.hasRow())
         {
           try {
               delete = ProtobufUtil.toDelete(proto);
           } catch (Throwable e) {
             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:deleteMultiple - txId " + transactionId + ", Caught exception after protobuf conversion delete"
                        + e.getMessage() + " " + stackTraceToString(e));
             t = e;
           }

           // Process in local memory
           if (delete != null)
           {
             try {
               delete(transactionId, delete);
             } catch (Throwable e) {
               if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:deleteMultiple - txId " + transactionId + ", Caught exception after internal delete - "
                         + e.getMessage() + " " + stackTraceToString(e));
             t = e;
             }

             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: deleteMultiple - txId "  + transactionId + ", regionName " + regionInfo.getRegionNameAsString() + ", type " + type + ", row " + Bytes.toStringBinary(proto.getRow().toByteArray()) + ", row in hex " + Hex.encodeHexString(proto.getRow().toByteArray()));
           }
         }
       }
       else
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: deleteMultiple - txId "  + transactionId + ", regionName " + regionInfo.getRegionNameAsString() + ", delete proto was null");

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

    if (mue != null)
    {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: deleteMultiple - performing memoryPercentage " + memoryPercentage + ", posting memory usage exceeds indicated percentage");
      deleteMultipleTransactionalResponseBuilder.setHasException(true);
      deleteMultipleTransactionalResponseBuilder.setException(mue.toString());
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
    MemoryUsageException mue = null;
    WrongRegionException wre = null;
    long transactionId = request.getTransactionId();

    /* commenting it out for the time-being
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
    */

    if (memoryThrottle == true) {
        if(memoryUsageWarnOnly == true)  {
          LOG.warn("TrxRegionEndpoint coprocessor: delete - performing memoryPercentage " + memoryPercentage + ", warning memory usage exceeds indicated percentage");
        }
        else {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: delete - performing memoryPercentage " + memoryPercentage + ", generating memory usage exceeds indicated percentage");
          mue = new MemoryUsageException("delete memory usage exceeds " + memoryUsageThreshold + " percent, trxId is " + transactionId);
        }
    }
    else
    {
      try {
          delete = ProtobufUtil.toDelete(proto); 
      } catch (Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:delete - txId " + transactionId + ", Caught exception " + e.getMessage() + " " + stackTraceToString(e));
        t = e;
      }

      // Process in local memory
      try {
        delete(transactionId, delete);
      } catch (Throwable e) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:delete - txId " + transactionId + ", Caught exception after internal delete - "
             + e.getMessage() + " " + stackTraceToString(e));
        t = e;
      }

      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: delete - txId "  + transactionId + ", regionName " + regionInfo.getRegionNameAsString() + ", type " + type + ", row " + Bytes.toStringBinary(proto.getRow().toByteArray()) + ", row in hex " + Hex.encodeHexString(proto.getRow().toByteArray()));
    }

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

    if (mue != null)
    {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: delete - performing memoryPercentage " + memoryPercentage + ", posting memory usage exceeds indicated percentage");
      deleteTransactionalResponseBuilder.setHasException(true);
      deleteTransactionalResponseBuilder.setException(mue.toString());
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
    IOException gioe = null;
    MemoryUsageException mue = null;
    WrongRegionException wre = null;
    org.apache.hadoop.hbase.client.Result result2 = null;
    long transactionId = request.getTransactionId();

    /* commenting it out for the time-being
    java.lang.String name = ((com.google.protobuf.ByteString) request.getRegionName()).toStringUtf8();

    // First test if this region matches our region name
    if (!name.equals(regionInfo.getRegionNameAsString())) {
       wre = new WrongRegionException("Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:get threw WrongRegionException" +
      "Request Region Name, " +
        name + ",  does not match this region, " +
        regionInfo.getRegionNameAsString());
    } else { */

      if (memoryThrottle == true) {
        if(memoryUsageWarnOnly == true)  
          LOG.warn("TrxRegionEndpoint coprocessor: get - performing memoryPercentage " + memoryPercentage + ", warning memory usage exceeds indicated percentage");
        else {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: get - performing memoryPercentage " + memoryPercentage + ", generating memory usage exceeds indicated percentage exception");
          mue = new MemoryUsageException("get memory usage exceeds " + memoryUsageThreshold + " percent, trxId is " + transactionId);
        }
      }
      else
      {
        try {
          get = ProtobufUtil.toGet(proto);
        } catch (Throwable e) {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:get - txId " + transactionId + ", Caught exception " + e.getMessage() + " " + stackTraceToString(e));
          t = e;
        }

        Scan scan = new Scan(get);
        List<Cell> results = new ArrayList<Cell>();

        try {
        
          if (LOG.isTraceEnabled()) {
            byte[] row = proto.getRow().toByteArray();
            byte[] getrow = get.getRow();
            String rowKey = Bytes.toString(row);
            String getRowKey = Bytes.toString(getrow);

            LOG.trace("TrxRegionEndpoint coprocessor: get - txId " + transactionId + ", Calling getScanner for regionName " + regionInfo.getRegionNameAsString() + ", row = " + Bytes.toStringBinary(row) + ", row in hex " + Hex.encodeHexString(row) + ", getrow = " + Bytes.toStringBinary(getrow) + ", getrow in hex " + Hex.encodeHexString(getrow));
          }

          scanner = getScanner(transactionId, scan);

          if (scanner != null)
            scanner.next(results);
         
          result2 = Result.create(results);

          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: get - txId " + transactionId + ", getScanner result2 isEmpty is " 
		   + result2.isEmpty() 
		   + ", row " 
		   + Bytes.toStringBinary(result2.getRow())
		   + " result length: "
		   + result2.size()); 

        } catch(Throwable e) {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: get - txId " + transactionId + ", Caught exception " + e.getMessage() + " " + stackTraceToString(e));
          t = e;
        }
        finally {
          if (scanner != null) {
            try {
              scanner.close();
            } catch(Exception e) {
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: get - txId " + transactionId + ", Caught exception " + e.getMessage() + " " + stackTraceToString(e));
              ge = e;
            }
          }
        }
      } // End of MemoryUsageCheck
  //}  // End of WrongRegionCheck

    org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.GetTransactionalResponse.Builder getResponseBuilder = GetTransactionalResponse.newBuilder();

   if (result2 != null)
   {
     getResponseBuilder.setResult(ProtobufUtil.toResult(result2));
   }
   else
   {
     if (t == null && wre == null && ge == null)
       gioe = new IOException("TrxRegionEndpoint coprocessor: get - result2 was null");
     if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: get - txId " + transactionId + ", result2 was null ");
   }
      
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
      
   if (gioe != null)
   {
     getResponseBuilder.setHasException(true);
     getResponseBuilder.setException(gioe.toString());
   }

   if (mue != null)
   {
     if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: get - performing memoryPercentage " + memoryPercentage + ", posting memory usage exceeds indicated percentage exception");
     getResponseBuilder.setHasException(true);
     getResponseBuilder.setException(mue.toString());
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
    MemoryUsageException mue = null;
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

    /* commenting it out for the time-being
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
    } else 
    */
    {
    
      if (memoryThrottle == true) {
        if(memoryUsageWarnOnly == true)  {
          LOG.warn("TrxRegionEndpoint coprocessor: openScanner - performing memoryPercentage " + memoryPercentage + ", warning memory usage exceeds indicated percentage");
        }
        else {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: openScanner - performing memoryPercentage " + memoryPercentage + ", generating memory usage exceeds indicated percentage");
          exceptionThrown = true;
          mue = new MemoryUsageException("openScanner memory usage exceeds " + memoryUsageThreshold + " percent, trxId is " + transId);
        }
      }
      else
      {
        try {
            scan = ProtobufUtil.toScan(request.getScan());
          if (scan == null)
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: openScanner - txId " + transId + ", scan was null");
        } catch (Throwable e) {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: openScanner - txId " + transId + ", Caught exception " + e.getMessage() + " " + stackTraceToString(e));
          t = e;
          exceptionThrown = true;
        }

        if (!exceptionThrown) {
          if (scan == null) {
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: openScanner - txId " + transId + ", scan is null");
            npe = new NullPointerException("TrxRegionEndpoint coprocessor: openScanner - txId " + transId + ", scan is null ");
            ioe =  new IOException("Invalid arguments to openScanner", npe);
            exceptionThrown = true;
          }
          else
          {
            try {
              scan.getAttribute(Scan.SCAN_ATTRIBUTES_METRICS_ENABLE);
              prepareScanner(scan);
            } catch (Throwable e) {
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: openScanner - txId " + transId + ", scan Caught exception " + e.getMessage() + " " + stackTraceToString(e));
              t = e;
              exceptionThrown = true;
            }
          }
        }

        List<Cell> results = new ArrayList<Cell>();

        if (!exceptionThrown) {
          try {
            scanner = getScanner(transId, scan);
        
            if (scanner != null) {
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: openScanner - txId " + transId + ", called getScanner, scanner is " + scanner);
              // Add the scanner to the map
              scannerId = addScanner(transId, scanner, this.m_Region);
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: openScanner - txId " + transId + ", called addScanner, scanner id " + scannerId + ", regionName " + regionInfo.getRegionNameAsString());
            }
            else
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: openScanner - txId " + transId + ", getScanner returned null, scanner id " + scannerId + ", regionName " + regionInfo.getRegionNameAsString());
       
          } catch (LeaseStillHeldException llse) {
/*
            try {
                scannerLeases.cancelLease(getScannerLeaseId(scannerId));
              } catch (LeaseException le) {
                  if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getScanner failed to get a lease " + scannerId);
              }
*/
            LOG.error("TrxRegionEndpoint coprocessor: openScanner - txId " + transId + ", getScanner Error opening scanner, " + llse.toString());
            exceptionThrown = true;
            lse = llse;
          } catch (IOException e) {
            LOG.error("TrxRegionEndpoint coprocessor: openScanner - txId " + transId + ", getScanner Error opening scanner, " + e.toString());
            exceptionThrown = true;
          }
        }

        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: openScanner - txId " + transId + ", scanner id " + scannerId + ", regionName " + regionInfo.getRegionNameAsString());
      }
    }

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

    if (mue != null)
    {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: openScanner - performing memoryPercentage " + memoryPercentage + ", posting memory usage exceeds indicated percentage");
      openResponseBuilder.setHasException(true);
      openResponseBuilder.setException(mue.toString());
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
    OutOfOrderProtocolException oop = null;
    OutOfOrderScannerNextException ooo = null;
    UnknownScannerException use = null;
    MemoryUsageException mue = null;
    WrongRegionException wre = null;
    Exception ne = null;
    Scan scan = null;
    List<Cell> cellResults = new ArrayList<Cell>();
    List<Result> results = new ArrayList<Result>();
    org.apache.hadoop.hbase.client.Result result = null;

    long scannerId = request.getScannerId();
    long transId = request.getTransactionId();
    int numberOfRows = request.getNumberOfRows();
    boolean closeScanner = request.getCloseScanner();
    long nextCallSeq = request.getNextCallSeq();
    long count = 0L;
    boolean shouldContinue = true;
    TransactionalRegionScannerHolder rsh = null;

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - txId " + transId + ", scanner id " + scannerId + ", numberOfRows " + numberOfRows + ", nextCallSeq " + nextCallSeq + ", closeScanner is " + closeScanner + ", region is " + regionInfo.getRegionNameAsString());

    /* commenting it out for the time-being
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
    } else 
    */
    {
    
      if (memoryThrottle == true) {
        if(memoryUsageWarnOnly == true)  {
          LOG.warn("TrxRegionEndpoint coprocessor: performScan - performing memoryPercentage " + memoryPercentage + ", warning memory usage exceeds indicated percentage");
        }
        else {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - performing memoryPercentage " + memoryPercentage + ", generating memory usage exceeds indicated percentage");
          mue = new MemoryUsageException("performScan memory usage exceeds " + memoryUsageThreshold + " percent, trxId is " + transId);
        }
      }
      else
      {
        // There should be a matching key in the transactionsById map
        // associated with this transaction id.  If there is not
        // one, then the initial openScanner call for the transaction
        // id was not called.  This is a protocol error requiring
        // openScanner, performScan followed by a closeScanner.

        String key = getTransactionalUniqueId(transId);
        boolean keyFound = transactionsById.containsKey(key);

        if (keyFound != true)
        {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - Unknown transaction [" + transId
                 + "] in region ["
                 + m_Region.getRegionInfo().getRegionNameAsString()
                 + "], will create an OutOfOrderProtocol exception ");
          oop = new OutOfOrderProtocolException("performScan does not have an active transaction with an open scanner, txId: " + transId);
         }

        if (oop == null) {
          try {

            scanner = getScanner(scannerId, nextCallSeq);

            if (scanner != null)
            {
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - txId " + transId + ", scanner id " + scannerId + ", scanner is not null");
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
              }
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - txId " + transId + ", scanner id " + scannerId + ", count is " + count + ", hasMore is " + hasMore + ", result " + result.isEmpty());
            }
            else
            {
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - txId " + transId + ", scanner id " + scannerId + transId + ", scanner is null");
            }
         } catch(OutOfOrderScannerNextException ooone) {
           if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - txId " + transId + ", scanner id " + scannerId + " Caught OutOfOrderScannerNextException  " + ooone.getMessage() + " " + stackTraceToString(ooone));
           ooo = ooone;
           } catch(ScannerTimeoutException cste) {
           if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - txId " + transId + ", scanner id " + scannerId + " Caught ScannerTimeoutException  " + cste.getMessage() + " " + stackTraceToString(cste));
           ste = cste;
         } catch(Throwable e) {
           if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - txId " + transId + ", scanner id " + scannerId + " Caught throwable exception " + e.getMessage() + " " + stackTraceToString(e));
           t = e;
         }
         finally {
           if (scanner != null) {
             try {
               if (closeScanner) {
                 if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - txId " + transId + ", scanner id " + scannerId + ", close scanner was true, closing the scanner" + ", closeScanner is " + closeScanner + ", region is " + regionInfo.getRegionNameAsString());
                 removeScanner(scannerId);
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
               if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan -  transaction id " + transId + ", Caught general exception " + e.getMessage() + " " + stackTraceToString(e));
               ne = e;
             }
           }
         }

         rsh = scanners.get(scannerId);

         nextCallSeq++;

         if (rsh == null)
         {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan rsh is null");
            use =  new UnknownScannerException(
              "ScannerId: " + scannerId + ", already closed?");
         }
         else
         {
           rsh.nextCallSeq = nextCallSeq;

           if (rsh == null)
           {
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - txId " + transId + ", performScan rsh is null, UnknownScannerException for scannerId: " + scannerId + ", nextCallSeq was " + nextCallSeq + ", for region " + regionInfo.getRegionNameAsString());
              use =  new UnknownScannerException(
                 "ScannerId: " + scannerId + ", was scanner already closed?, transaction id " + transId + ", nextCallSeq was " + nextCallSeq + ", for region " + regionInfo.getRegionNameAsString());
           }
           else
           {
             rsh.nextCallSeq = nextCallSeq;

             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - txId " + transId + ", scanner id " + scannerId + ", regionName " + regionInfo.getRegionNameAsString() +
", nextCallSeq " + nextCallSeq + ", rsh.nextCallSeq " + rsh.nextCallSeq + ", close scanner is " + closeScanner);

          }
         }
        }
       }
     }

   org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PerformScanResponse.Builder performResponseBuilder = PerformScanResponse.newBuilder();
   performResponseBuilder.setHasMore(hasMore);
   performResponseBuilder.setNextCallSeq(nextCallSeq);
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
      performResponseBuilder.setHasMore(false);
      performResponseBuilder.setHasException(true);
      performResponseBuilder.setException(t.toString());
    }

    if (ste != null)
    {
      performResponseBuilder.setHasMore(false);
      performResponseBuilder.setHasException(true);
      performResponseBuilder.setException(ste.toString());
    }

    if (wre != null)
    {
      performResponseBuilder.setHasMore(false);
      performResponseBuilder.setHasException(true);
      performResponseBuilder.setException(wre.toString());
    }

    if (ne != null)
    {
      performResponseBuilder.setHasMore(false);
      performResponseBuilder.setHasException(true);
      performResponseBuilder.setException(ne.toString());
    }

    if (ooo != null)
    {
      performResponseBuilder.setHasMore(false);
      performResponseBuilder.setHasException(true);
      performResponseBuilder.setException(ooo.toString());
    }

    if (use != null)
    {
      performResponseBuilder.setHasMore(false);
      performResponseBuilder.setHasException(true);
      performResponseBuilder.setException(use.toString());
    }

    if (oop != null)
    {
      performResponseBuilder.setHasMore(false);
      if (this.suppressOutOfOrderProtocolException == false)
      {
        performResponseBuilder.setHasException(true);
        performResponseBuilder.setException(oop.toString());
        LOG.warn("TrxRegionEndpoint coprocessor: performScan - OutOfOrderProtocolException, transaction was not found, txId: " + transId + ", return exception" + ", regionName " + regionInfo.getRegionNameAsString());
      }
      else
        LOG.warn("TrxRegionEndpoint coprocessor: performScan - suppressing OutOfOrderProtocolException, transaction was not found, txId: " + transId + ", regionName " + regionInfo.getRegionNameAsString());
    }

    if (mue != null)
    {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: performScan - performing memoryPercentage " + memoryPercentage + ", posting memory usage exceeds indicated percentage");
      performResponseBuilder.setHasMore(false);
      performResponseBuilder.setHasException(true);
      performResponseBuilder.setException(mue.toString());
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
    MemoryUsageException mue = null;
    WrongRegionException wre = null;
    long transactionId = request.getTransactionId();

    /* commenting it out for the time-being
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
    } else 
    */
      if (memoryThrottle == true) {
        if(memoryUsageWarnOnly == true)  {
          LOG.warn("TrxRegionEndpoint coprocessor: put - performing memoryPercentage " + memoryPercentage + ", warning memory usage exceeds indicated percentage");
        }
        else {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: put - performing memoryPercentage " + memoryPercentage + ", generating memory usage exceeds indicated percentage exception");
          mue = new MemoryUsageException("put memory usage exceeds " + memoryUsageThreshold + " percent, trxId is " + transactionId);
        }
      }
      else
      {
        try {
            put = ProtobufUtil.toPut(proto);
        } catch (Throwable e) {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:put - txId " + transactionId + ", Caught exception " + e.getMessage() + " " + stackTraceToString(e));
          t = e;
        }

      if (mue == null && type == MutationType.PUT && proto.hasRow())
      {
        // Process in local memory
        try {   
          put(transactionId, put);
        } catch (Throwable e) {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:put - txId " + transactionId + ", Caught exception after  internal put - "
                         + e.getMessage() + " " + stackTraceToString(e));
          t = e;
        }

        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: put - txId "  + transactionId + ", regionName " + regionInfo.getRegionNameAsString() + ", type " + type + ", row " + Bytes.toStringBinary(proto.getRow().toByteArray()) + ", row in hex " + Hex.encodeHexString(proto.getRow().toByteArray()));
      }
      else
      {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: put - txId "  + transactionId + ", regionName " + regionInfo.getRegionNameAsString() + "- no valid PUT type or does not contain a row");
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

    if (mue != null)
    {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: put - performing memoryPercentage " + memoryPercentage + ", posting memory usage exceeds indicated percentage exception");
      putTransactionalResponseBuilder.setHasException(true);
      putTransactionalResponseBuilder.setException(mue.toString());
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
   MemoryUsageException mue = null;
   WrongRegionException wre = null;
   long transactionId = request.getTransactionId();
    /* commenting it out for the time-being
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
   } else 
    */
   {
      if (memoryThrottle == true) {
        if(memoryUsageWarnOnly == true)  {
          LOG.warn("TrxRegionEndpoint coprocessor: putMultiple - performing memoryPercentage " + memoryPercentage + ", warning memory usage exceeds indicated percentage");
        }
        else { 
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: putMultiple - performing memoryPercentage " + memoryPercentage + ", generating memory usage exceeds indicated percentage");
          mue = new MemoryUsageException("putMultiple memory usage exceeds " + memoryUsageThreshold + " percent, trxId is " + transactionId);
        }
      }
      else
      {
         for (org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto proto : results)
         { 
           put = null;

           if (proto != null)
           {
             type = proto.getMutateType();

             if (type == MutationType.PUT && proto.hasRow())
             {
               try {
                   put = ProtobufUtil.toPut(proto);
               } catch (Throwable e) {
                 if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:putMultiple - txId " + transactionId + ", Caught exception after protobuf conversion put"
                         + e.getMessage() + " " + stackTraceToString(e));
                 t = e;
               }

               // Process in local memory
               if (put != null)
               {
                 try {
                   put(transactionId, put);
                 } catch (Throwable e) {
                   if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:putMultiple - txId " + transactionId + ", Caught exception after  internal put - "
                            + e.getMessage() + " " + stackTraceToString(e));
                   t = e;
                 }

                 if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: putMultiple - txId "  + transactionId + ", regionName " + regionInfo.getRegionNameAsString() + ", type " + type + ", row " + Bytes.toStringBinary(proto.getRow().toByteArray()) + ", row in hex " + Hex.encodeHexString(proto.getRow().toByteArray()));
               }
             }
           }
            else
             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: putMultiple - txId "  + transactionId + ", regionName " + regionInfo.getRegionNameAsString() + ", put proto was null");

          }
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

    if (mue != null)
    {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: putMultiple - performing memoryPercentage " + memoryPercentage + ", posting memory usage exceeds indicated percentage");
      putMultipleTransactionalResponseBuilder.setHasException(true);
      putMultipleTransactionalResponseBuilder.setException(mue.toString());
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
      long transactionId = request.getTransactionId();

      if (reconstructIndoubts == 0) {
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: recoveryRequest - txId " + transactionId + ", RECOV");
         constructIndoubtTransactions();
      }

      // Placeholder for real work when recovery is added
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: recoveryRequest - txId "  + transactionId + ", regionName " + regionInfo.getRegionNameAsString() + ", tmId" + tmId);

    /* commenting it out for the time-being
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
    */

      org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.RecoveryRequestResponse.Builder recoveryResponseBuilder = RecoveryRequestResponse.newBuilder();

      List<Long> indoubtTransactions = new ArrayList<Long>();
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: recoveryRequest Trafodion Recovery: region " + regionInfo.getEncodedName() + " receives recovery request from TM " + tmId  + " with region state " + regionState);
      switch(regionState) {
              case REGION_STATE_RECOVERING: // RECOVERING, already create a list of in-doubt txn, but still in the state of resolving them,
                           // retrieve all in-doubt txn from rmid and return them into a long a
                    if (LOG.isInfoEnabled()) LOG.info("TRAF RCOV:recoveryRequest in region starting" + regionInfo.getEncodedName() + " has in-doubt transaction " + indoubtTransactionsById.size());
                    for (Entry<Long, List<WALEdit>> entry : indoubtTransactionsById.entrySet()) {
                          long tid = entry.getKey();
                          if ((int) (tid >> 32) == tmId) {
                              indoubtTransactions.add(tid);
                              if (LOG.isInfoEnabled()) LOG.info("TrxRegionEndpoint coprocessor: recoveryRequest - txId " + transactionId + ", Trafodion Recovery: region " + regionInfo.getEncodedName() + " in-doubt transaction " + tid + " has been added into the recovery reply to TM " + tmId + " during recovery ");
                          }
                     }
                     if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: recoveryRequest " + indoubtTransactions.size());
                     if (indoubtTransactions.size() == 0) {
                       String lv_encoded = m_Region.getRegionInfo().getEncodedName();
                       try {
                           if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: recoveryRequest - Trafodion Recovery: delete recovery zNode TM " + tmId + " region encoded name " + lv_encoded + " for 0 in-doubt transaction");
                           deleteRecoveryzNode(tmId, lv_encoded);
                       } catch (IOException e) {
                          LOG.error("TrxRegionEndpoint coprocessor: recoveryRequest - Trafodion Recovery: delete recovery zNode failed");
                       }
                     }
                     break;
              case REGION_STATE_START: // START
                     List<TrxTransactionState> commitPendingCopy = new ArrayList<TrxTransactionState>(commitPendingTransactions);
                     if (LOG.isInfoEnabled()) LOG.info("TRAF RCOV:recoveryRequest in region started" + regionInfo.getEncodedName() + " has in-doubt transaction " + commitPendingCopy.size());
                     for (TrxTransactionState commitPendingTS : commitPendingCopy) {
                        long tid = commitPendingTS.getTransactionId();
                          if ((int) (tid >> 32) == tmId) {
                              indoubtTransactions.add(tid);
                              if (LOG.isInfoEnabled()) LOG.info("TrxRegionEndpoint coprocessor: recoveryRequest - Trafodion Recovery: region " + regionInfo.getEncodedName() + " in-doubt transaction " + tid + " has been added into the recovery reply to TM " + tmId + " during start ");
                          }
                     }
                     // now remove the ZK node after TM has initiated the ecovery request   
                    String lv_encoded = m_Region.getRegionInfo().getEncodedName();
                    try {
                         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: recoveryRequest - Trafodion Recovery: delete recovery zNode TM " + tmId + " region encoded name " + lv_encoded + " for 0 in-doubt transaction");
                        deleteRecoveryzNode(tmId, lv_encoded);
                    } catch (IOException e) {
                        LOG.error("TrxRegionEndpoint coprocessor: recoveryRequest - Trafodion Recovery: delete recovery zNode failed");
                    }
                    break;
                default:
                    LOG.error("Trafodion Recovery: encounter incorrect region state " + regionState);
                    break;
      }

      // Placeholder response forced to zero for now
      for (Long transactionInDoubtId:indoubtTransactions) {
         recoveryResponseBuilder.addResult(transactionInDoubtId);
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
    long transactionId = request.getTransactionId();
    T max = null;
    try {
      ColumnInterpreter<T, S, P, Q, R> ci = constructColumnInterpreterFromRequest(request);
      T temp;
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
    if (LOG.isInfoEnabled()) LOG.info("TrxRegionEndpoint coprocessor: getMax - txId " + transactionId + ", Maximum from this region is "
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
    long transactionId = request.getTransactionId();
    T min = null;
    try {
      ColumnInterpreter<T, S, P, Q, R> ci = constructColumnInterpreterFromRequest(request);
      T temp;
      Scan scan = ProtobufUtil.toScan(request.getScan());
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
    if (LOG.isInfoEnabled()) LOG.info("TrxRegionEndpoint coprocessor: getMin - txId " + transactionId + ", Minimum from this region is "
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
    long transactionId = request.getTransactionId();
    try {
      ColumnInterpreter<T, S, P, Q, R> ci = constructColumnInterpreterFromRequest(request);
      S sumVal = null;
      T temp;
      Scan scan = ProtobufUtil.toScan(request.getScan());
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
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getSum - txId " + transactionId + ", Sum from this region is "
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

    if (LOG.isInfoEnabled()) LOG.info(String.format("Row counter for txId %d from this region: %s is %d, startKey is [%s], endKey is [%s]",
        transactionId, env.getRegion().getRegionNameAsString(), counter, 
        env.getRegion().getStartKey() == null ? "null" : Bytes.toStringBinary(env.getRegion().getStartKey()), 
        env.getRegion().getEndKey() == null ? "null" : Bytes.toStringBinary(env.getRegion().getEndKey())));

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

    org.apache.hadoop.conf.Configuration conf = tmp_env.getConfiguration(); 
    
    synchronized (stoppableLock) {
      try {
        this.transactionLeaseTimeout = conf.getInt(LEASE_CONF, MINIMUM_LEASE_TIME);
        if (this.transactionLeaseTimeout < MINIMUM_LEASE_TIME) {
          if (LOG.isWarnEnabled()) LOG.warn("Transaction lease time: " + this.transactionLeaseTimeout + ", was less than the minimum lease time.  Now setting the timeout to the minimum default value: " + MINIMUM_LEASE_TIME);
          this.transactionLeaseTimeout = MINIMUM_LEASE_TIME;
        }

        this.scannerLeaseTimeoutPeriod = HBaseConfiguration.getInt(conf,
          HConstants.HBASE_CLIENT_SCANNER_TIMEOUT_PERIOD,
          HConstants.HBASE_REGIONSERVER_LEASE_PERIOD_KEY,
          HConstants.DEFAULT_HBASE_CLIENT_SCANNER_TIMEOUT_PERIOD);
        this.scannerThreadWakeFrequency = conf.getInt(HConstants.THREAD_WAKE_FREQUENCY, 10 * 1000);

        this.cleanTimer = conf.getInt(SLEEP_CONF, DEFAULT_SLEEP);
        this.memoryUsageThreshold = conf.getInt(MEMORY_THRESHOLD, DEFAULT_MEMORY_THRESHOLD);
        this.memoryUsagePerformGC = conf.getBoolean(MEMORY_PERFORM_GC, DEFAULT_MEMORY_PERFORM_GC);
        this.memoryUsageWarnOnly = conf.getBoolean(MEMORY_WARN_ONLY, DEFAULT_MEMORY_WARN_ONLY);
        this.memoryUsageTimer = conf.getInt(MEMORY_CONF, DEFAULT_MEMORY_SLEEP);
        this.memoryUsageTimer = conf.getInt(MEMORY_CONF, DEFAULT_MEMORY_SLEEP);

        this.suppressOutOfOrderProtocolException = conf.getBoolean(SUPPRESS_OOP, DEFAULT_SUPPRESS_OOP);
	if (this.transactionLeases == null)  
	    this.transactionLeases = new Leases(LEASE_CHECK_FREQUENCY);

	//if (this.scannerLeases == null)  
	 //   this.scannerLeases = new Leases(scannerThreadWakeFrequency);

        if (LOG.isTraceEnabled()) LOG.trace("Transaction lease time: "
            + this.transactionLeaseTimeout
            + " Scanner lease time: "
            + this.scannerThreadWakeFrequency
            + ", Scanner lease timeout period: "
            + this.scannerLeaseTimeoutPeriod
            + ", Clean timer: "
            + this.cleanTimer
            + ", MemoryUsage timer: "
            + this.memoryUsageTimer
            + ", MemoryUsageThreshold: "
            + this.memoryUsageThreshold
            + ", MemoryUsagePerformGC: "
            + this.memoryUsagePerformGC
            + ", MemoryUsageWarnOnly: "
            + this.memoryUsageWarnOnly
            + ", Suppress OutOfOrderProtocolException: "
            + this.suppressOutOfOrderProtocolException);

        // Start the clean core thread
          
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

        // Start the memory usage chore thread if the threshold
        // selected is greater than the default of 100%.   

        if (memoryUsageThreshold < DEFAULT_MEMORY_THRESHOLD &&
            memoryUsageThread == null) {
          LOG.warn("TrxRegionEndpoint coprocessor: start - starting memoryUsageThread");

          memoryUsageThread = new MemoryUsageChore(this, memoryUsageTimer, stoppable2);

          UncaughtExceptionHandler handler2 = new UncaughtExceptionHandler() {

            public void uncaughtException(final Thread t, final Throwable e)
            {
              LOG.fatal("MemoryUsageChore uncaughtException: " + t.getName(), e);
            }
          };

          String n2 = Thread.currentThread().getName();

          ChoreThread2 = new Thread(memoryUsageThread);
          Threads.setDaemonThreadRunning(ChoreThread2, n2 + ".memoryUsage", handler2);
        }

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
        throw new CoprocessorException("TrxRegionEndpoint coprocessor: start - Caught exception " + e);
      }
    }

    this.t_Region = (TransactionalRegion) tmp_env.getRegion();
    this.fs = this.m_Region.getFilesystem();
    tHLog = this.m_Region.getLog();

    RegionServerServices rss = tmp_env.getRegionServerServices();
    ServerName sn = rss.getServerName();
    lv_hostName = sn.getHostname();
    lv_port = sn.getPort();
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: hostname " + lv_hostName + " port " + lv_port);
    this.regionInfo = this.m_Region.getRegionInfo();
    this.nextLogSequenceId = this.m_Region.getSequenceId();
    this.t_Region = (TransactionalRegion) tmp_env.getRegion();
    zkw1 = rss.getZooKeeper();

    this.configuredEarlyLogging = tmp_env.getConfiguration().getBoolean("hbase.regionserver.region.transactional.earlylogging", false);
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: early logging setting is " + this.configuredEarlyLogging +
                  "\nTrxRegionEndpoint coprocessor: get the reference from Region CoprocessorEnvironment ");

    this.configuredConflictReinstate = tmp_env.getConfiguration().getBoolean("hbase.regionserver.region.transactional.conflictreinstate", false);
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: conflict reinstate  setting is " + this.configuredConflictReinstate +
                  "\nTrxRegionEndpoint coprocessor: get the reference from Region CoprocessorEnvironment ");

    if (tmp_env.getSharedData().isEmpty())
       if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: shared map is empty ");
    else
       if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: shared map is NOT empty");

    transactionsByIdTestz = TrxRegionObserver.getRefMap();

    if (transactionsByIdTestz.isEmpty()) {
       if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: reference map is empty ");
    }
    else  {
       if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: reference map is NOT empty ");
    }
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: Region " + this.m_Region.getRegionNameAsString() + " check indoubt list from reference map ");

    Map<Long, List<WALEdit>> indoubtTransactionsByIdCheck = (TreeMap<Long, List<WALEdit>>)transactionsByIdTestz.get(
            this.m_Region.getRegionNameAsString()+TrxRegionObserver.trxkeypendingTransactionsById);
    if(indoubtTransactionsByIdCheck != null) {
        this.indoubtTransactionsById = indoubtTransactionsByIdCheck;
    }
    else {
        transactionsByIdTestz.put(this.m_Region.getRegionNameAsString()+TrxRegionObserver.trxkeypendingTransactionsById,
                                  this.indoubtTransactionsById);
    }

    Map<Integer, Integer> indoubtTransactionsCountByTmidCheck = (TreeMap<Integer,Integer>)transactionsByIdTestz.get(
            this.m_Region.getRegionNameAsString()+TrxRegionObserver.trxkeyindoubtTransactionsCountByTmid);
    if(indoubtTransactionsCountByTmidCheck != null) {
        this.indoubtTransactionsCountByTmid = indoubtTransactionsCountByTmidCheck;
    }
    else {
        transactionsByIdTestz.put(this.m_Region.getRegionNameAsString()+TrxRegionObserver.trxkeyindoubtTransactionsCountByTmid,
                                  this.indoubtTransactionsCountByTmid);
    }

    Set<TrxTransactionState> commitPendingTransactionsCheck = (Set<TrxTransactionState>)transactionsByIdTestz.get(
            this.m_Region.getRegionNameAsString()+TrxRegionObserver.trxkeycommitPendingTransactions);
    if(commitPendingTransactionsCheck != null) {
        this.commitPendingTransactions = commitPendingTransactionsCheck;
    }
    else {
        transactionsByIdTestz.put(this.m_Region.getRegionNameAsString()+TrxRegionObserver.trxkeycommitPendingTransactions,
                this.commitPendingTransactions);
    }

    ConcurrentHashMap<String, TrxTransactionState> transactionsByIdCheck = (ConcurrentHashMap<String, TrxTransactionState>) transactionsByIdTestz.get(
            this.m_Region.getRegionNameAsString()+TrxRegionObserver.trxkeytransactionsById);
    if(transactionsByIdCheck != null) {
        this.transactionsById = transactionsByIdCheck;
    }
    else {
        transactionsByIdTestz.put(this.m_Region.getRegionNameAsString()+TrxRegionObserver.trxkeytransactionsById,
                                  this.transactionsById);
    }

    AtomicBoolean closingCheck = (AtomicBoolean)transactionsByIdTestz
            .get(this.m_Region.getRegionNameAsString()+TrxRegionObserver.trxkeyClosingVar);
    if(closingCheck != null) {
        this.closing = closingCheck;
    }
    else {
        transactionsByIdTestz.put(this.m_Region.getRegionNameAsString()+TrxRegionObserver.trxkeyClosingVar,
                                  this.closing);
    }

    // Set up the memoryBean from the ManagementFactory
    if (memoryUsageThreshold < DEFAULT_MEMORY_THRESHOLD) 
      memoryBean = ManagementFactory.getMemoryMXBean();

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: start");
  }

  @Override
  public void stop(CoprocessorEnvironment env) throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: stop ");
    stoppable.stop("stop() TrxRegionEndpoint");
  }

  // Internal support methods

  /**
   * Checks if the region is closing
   * @param long transactionId
   * @return String 
   * @throws IOException 
   */
  private void checkClosing(final long transactionId) throws IOException {
    if (closing.get()) {
      if(LOG.isWarnEnabled()) LOG.warn("TrxRegionEndpoint coprocessor: checkClosing - txId " + transactionId + ", Trafodion Recovery: Raising exception. no more new transactions allowed.");
      throw new IOException("closing region, no more new transactions allowed. Region: " + regionInfo.getRegionNameAsString());
    }
  }

  /**
   * Gets the transaction state                   
   * @param long transactionId
   * @return TrxTransactionState
   * @throws UnknownTransactionException
   */
  protected TrxTransactionState getTransactionState(final long transactionId)
   throws UnknownTransactionException {
    TrxTransactionState state = null;
    boolean throwUTE = false;

    String key = getTransactionalUniqueId(transactionId);
    state = transactionsById.get(key);

    if (state == null) 
    {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getTransactionState Unknown transaction: [" + transactionId + "], throwing UnknownTransactionException");
        throwUTE = true;
    }
    else {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getTransactionState Found transaction: [" + transactionId + "]");

      try {
         transactionLeases.renewLease(key);
      } catch (LeaseException e) {
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getTransactionState renewLease failed will try to createLease for transaction: [" + transactionId + "]");
         try {
            transactionLeases.createLease(key, transactionLeaseTimeout, new TransactionLeaseListener(transactionId));
         } catch (LeaseStillHeldException lshe) {
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getTransactionState renewLeasefollowed by createLease failed throwing original LeaseException for transaction: [" + transactionId + "]");
            throw new RuntimeException(e);
         }
      }
    }

    if (throwUTE)
      throw new UnknownTransactionException();

    return state;
  }

  /**
   * Retires the transaction                        
   * @param TrxTransactionState state
   */
  private void retireTransaction(final TrxTransactionState state, final boolean clear) {
    String key = getTransactionalUniqueId(state.getTransactionId());
    long transId = state.getTransactionId();

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: retireTransaction: [" 
             + transId + "]");              

    try {
      transactionLeases.cancelLease(key);
    } catch (LeaseException le) {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: retireTransaction: [" 
               + transId + "] LeaseException");
      // Ignore
    } catch (Exception e) {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: retireTransaction: [" 
               + transId + "] General Lease exception" + e.getMessage() + " " + stackTraceToString(e));
      // Ignore
    }

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: retireTransaction clearTransactionsToCheck for: " + key + " from all its TrxTransactionState lists");

    // Clear out transaction state
      
    if (clear == true) {
      state.clearState();
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: retireTransaction clearState for: " + key + " from all its TrxTransactionState lists");
    }
    else {
      state.clearTransactionsToCheck();	
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: retireTransaction clearTransactionsToCheck for: " + key);
    }

    synchronized (cleanScannersForTransactions) {
      cleanScannersForTransactions.add(transId);
    }

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: retireTransaction calling remove entry for: " + key + " , from transactionById map ");
      transactionsById.remove(key);

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:retireTransaction " + key + ", looking for retire transaction id " + transId + ", transactionsById " + transactionsById.size() + ", commitedTransactionsBySequenceNumber " + commitedTransactionsBySequenceNumber.size() + ", commitPendingTransactions " + commitPendingTransactions.size());

  }

  public void choreThreadDetectStaleTransactionBranch() {

      List<Integer> staleBranchforTMId = new ArrayList<Integer>();
      List<TrxTransactionState> commitPendingCopy = new ArrayList<TrxTransactionState>(commitPendingTransactions);
      Map<Long, List<WALEdit>> indoubtTransactionsMap = new TreeMap<Long, List<WALEdit>>(indoubtTransactionsById);
      int tmid, tm;

      // selected printout for CP
      long currentEpoch = controlPointEpoch.get();
      if ((currentEpoch < 10) || ((currentEpoch % 10) == 1)) {
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:choreThreadDetectStaleTransactionBranch: Region "
             + regionInfo.getRegionNameAsString() + " ChoreThread CP Epoch " + controlPointEpoch.get());
      }

      byte [] lv_byte_region_info = regionInfo.toByteArray();
      String lv_encoded = regionInfo.getEncodedName();

      long transactionId;
      if ((indoubtTransactionsById != null) && (indoubtTransactionsById.size() > 0) && (this.regionState != REGION_STATE_START)) {
         for (Entry<Long, List<WALEdit>> entry : indoubtTransactionsMap.entrySet()) {
               transactionId = entry.getKey();
               if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:choreThreadDetectStaleTransactionBranch: indoubt branch Txn id "
                       + transactionId + " region info bytes " + new String(lv_byte_region_info));
               tmid = (int) (transactionId >> 32);
               if (!staleBranchforTMId.contains(tmid)) {staleBranchforTMId.add(tmid);}
         }
      }
      else { // region has started
         for (TrxTransactionState commitPendingTS : commitPendingCopy) {
            if (commitPendingTS.getCPEpoch() < (controlPointEpoch.get() - 1)) {
               transactionId = commitPendingTS.getTransactionId();
               if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:choreThreadDetectStaleTransactionBranch: stale branch Txn id "
                  + transactionId + " region info bytes " + new String(lv_byte_region_info));
               tmid = (int) (transactionId >> 32);
               if (!staleBranchforTMId.contains(tmid)) {staleBranchforTMId.add(tmid);}
            }
         }
      }

      if (!staleBranchforTMId.isEmpty()) {
            for (int i = 0; i < staleBranchforTMId.size(); i++) {
               try {
                   tm = staleBranchforTMId.get(i);
                   if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:choreThreadDetectStaleTransactionBranch: ZKW Create Recovery zNode TM "
                            + tm + " region encoded name " + lv_encoded + " region info bytes " + new String(lv_byte_region_info));
                   createRecoveryzNode(tm, lv_encoded, lv_byte_region_info);
                   } catch (IOException exp) {
                   LOG.error("TrxRegionEndpoint coprocessor:choreThreadDetectStaleTransactionBranch: ZKW Create recovery zNode failed " + exp);
               }
            } // for
      } // if block

      controlPointEpoch.getAndIncrement();
      commitPendingCopy.clear();
      indoubtTransactionsMap.clear();
      staleBranchforTMId.clear();
  }

  public void createRecoveryzNode(int node, String encodedName, byte [] data) throws IOException {

       synchronized(zkRecoveryCheckLock) {
         // default zNodePath for recovery
         String zNodeKey = lv_hostName + "," + lv_port + "," + encodedName;

         StringBuilder sb = new StringBuilder();
         sb.append("TM");
         sb.append(node);
         String str = sb.toString();
         String zNodePathTM = zNodePath + str;
         String zNodePathTMKey = zNodePathTM + "/" + zNodeKey;
         if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: ZKW Post region recovery znode" + node + " zNode Path " + zNodePathTMKey);
          // create zookeeper recovery zNode, call ZK ...
         try {
                if (ZKUtil.checkExists(zkw1, zNodePathTM) == -1) {
                   // create parent nodename
                   if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: ZKW create parent zNodes " + zNodePathTM);
                   ZKUtil.createWithParents(zkw1, zNodePathTM);
                }
                ZKUtil.createAndFailSilent(zkw1, zNodePathTMKey, data);
          } catch (KeeperException e) {
          throw new IOException("Trafodion Recovery Region Observer CP: ZKW Unable to create recovery zNode to TM, throw IOException " + node, e);
          }
       }
  } // end ogf createRecoveryzNode

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
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: Trafodion Recovery:  Flushing cache in startRegionAfterRecovery " + m_Region.getRegionInfo().getRegionNameAsString());
          m_Region.flushcache();
          //if (!m_Region.flushcache().isFlushSucceeded()) { 
          //   LOG.trace("TrxRegionEndpoint coprocessor: Trafodion Recovery:  Flushcache returns false !!! " + m_Region.getRegionInfo().getRegionNameAsString());
          //}
     } catch (IOException e) {
     LOG.error("TrxRegionEndpoint coprocessor: Trafodion Recovery: Flush failed after replay edits" + m_Region.getRegionInfo().getRegionNameAsString() + ", Caught exception " + e.toString());
     return;
     }


    //FileSystem fileSystem = m_Region.getFilesystem();
    //Path archiveTHLog = new Path (recoveryTrxPath.getParent(),"archivethlogfile.log");
    //if (fileSystem.exists(archiveTHLog)) fileSystem.delete(archiveTHLog, true);
    //if (fileSystem.exists(recoveryTrxPath))fileSystem.rename(recoveryTrxPath,archiveTHLog);
    if (indoubtTransactionsById != null)
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: Trafodion Recovery: region " + recoveryTrxPath + " has " + indoubtTransactionsById.size() + " in-doubt transactions and edits are archived.");
    else
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: Trafodion Recovery: region " + recoveryTrxPath + " has 0 in-doubt transactions and edits are archived.");
    regionState = REGION_STATE_START; 
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: Trafodion Recovery: region " + m_Region.getRegionInfo().getEncodedName() + " is STARTED.");
  }

  /**
   * Commits the transaction
   * @param TrxTransactionState state
   * @throws IOException
   */
  private void commit(final TrxTransactionState state) throws IOException {
    long txid = 0;
    WALEdit b = null;
    int num = 0;
    Tag commitTag;
    ArrayList<WALEdit> editList;
    long transactionId = state.getTransactionId();

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:commit - txId " + transactionId + ", region " + m_Region.getRegionInfo().getRegionNameAsString() + ", transactionsById " + transactionsById.size() + ", commitedTransactionsBySequenceNumber " + commitedTransactionsBySequenceNumber.size() + ", commitPendingTransactions " + commitPendingTransactions.size());
     
    if (state.isReinstated() && !this.configuredConflictReinstate) {
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit Trafodion Recovery: commit reinstated indoubt transactions " + transactionId + 
                              " in region " + m_Region.getRegionInfo().getRegionNameAsString());
      synchronized (indoubtTransactionsById) {  
        editList = (ArrayList<WALEdit>) indoubtTransactionsById.get(transactionId);
      }
      num  = editList.size();
       if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit - txId " + transactionId + ", region " + regionInfo.getRegionNameAsString() + 
       ", Redrive commit with number of edit kvs list size " + num);
      for ( int i = 0; i < num; i++){
         b = editList.get(i);
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit - txId " + transactionId + ", Writing " + b.size() + " updates for reinstated transaction");
         for (KeyValue kv : b.getKeyValues()) {
           synchronized (editReplay) {
             Put put;
             Delete del;
             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:commit - txId " + transactionId + ", Trafodion Recovery: region " + m_Region.getRegionInfo().getRegionNameAsString() + ", Replay commit for transaction with Op " + kv.getType());
             if (kv.getTypeByte() == KeyValue.Type.Put.getCode()) {
		put = new Put(CellUtil.cloneRow(kv)); // kv.getRow()
                put.add(CellUtil.cloneFamily(kv), CellUtil.cloneQualifier(kv), kv.getTimestamp(), CellUtil.cloneValue(kv));
                //state.addWrite(put); // no need to add since add has been done in constructInDoubtTransactions
              try {
                m_Region.put(put);
              }
              catch (Exception e) {
                 if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit - txId " + transactionId + ", Trafodion Recovery: Executing put caught an exception " + e.toString());
                 state.setStatus(Status.ABORTED);
                 retireTransaction(state, true);
                 throw new IOException(e.toString());
              }
     	     } else if (CellUtil.isDelete(kv))  {
	   	del = new Delete(CellUtil.cloneRow(kv));
	       	if (CellUtil.isDeleteFamily(kv)) {
	 	     del.deleteFamily(CellUtil.cloneFamily(kv));
	        } else if (kv.isDeleteType()) {
	             del.deleteColumn(CellUtil.cloneFamily(kv), CellUtil.cloneQualifier(kv));
	        }
                //state.addDelete(del);  // no need to add since add has been done in constructInDoubtTransactions
               try {
                 m_Region.delete(del);
                  }
               catch (Exception e) {
                 if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit - txId " + transactionId + ", Trafodion Recovery: Executing delete caught an exception " + e.toString());
                 state.setStatus(Status.ABORTED);
                 retireTransaction(state, true);
                 throw new IOException(e.toString());
                }
   	     }
          } // synchronized reply edits
        } // for WALEdit
      } // for ediList
    }  // reinstated transactions
    else { // either non-reinstated transaction, or reinstate transaction with conflict reinstate TRUE (write from TS write ordering)
      // Perform write operations timestamped to right now
      // maybe we can turn off WAL here for HLOG since THLOG has contained required edits in phase 1
        
      ListIterator<WriteAction> writeOrderIter = null;
      for (writeOrderIter = state.getWriteOrderingIter();
             writeOrderIter.hasNext();) {
         WriteAction action =(WriteAction) writeOrderIter.next();
         // Process Put
         Put put = action.getPut();

         if (null != put) {
          put.setDurability(Durability.SKIP_WAL);
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit - txId " + transactionId + ", Executing put directly to m_Region");
           try {
             m_Region.put(put);
           }
           catch (Exception e) {
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit - txId " + transactionId + ", Executing put caught an exception " + e.toString());
              state.setStatus(Status.ABORTED);
              retireTransaction(state, true);
              throw new IOException(e.toString());
           }
         }

         // Process Delete
         Delete delete = action.getDelete();

         if (null != delete){
          delete.setDurability(Durability.SKIP_WAL);
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit - txId " + transactionId + ", Executing delete directly to m_Region");
           try {
             m_Region.delete(delete);
           }
           catch (Exception e) {
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit  - txId " + transactionId + ", Executing delete caught an exception " + e.toString());
              state.setStatus(Status.ABORTED);
              retireTransaction(state, true);
              throw new IOException(e.toString());
           }
         }
       }
    } // normal transactions

    // Now write a commit edit to HLOG

    List<Tag> tagList = new ArrayList<Tag>();
    if (state.hasWrite() || state.isReinstated()) {
         if (!state.getFullEditInCommit()) {
            commitTag = state.formTransactionalContextTag(TS_COMMIT);
            tagList.add(commitTag);
            WALEdit e1 = state.getEdit();
            WALEdit e = new WALEdit();
            if (e1.isEmpty() || e1.getKeyValues().size() <= 0) {
               if (LOG.isInfoEnabled()) LOG.info("TRAF RCOV EPCP: commit - txId " + transactionId + ", Encountered empty TS WAL Edit list during commit, HLog txid " + txid);
               }
            else {
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
                    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit - txId " + transactionId + ", Write commit HLOG seq " + txid);
                 }
                 catch (IOException exp1) {
                    if (LOG.isTraceEnabled()) LOG.trace("TRAF RCOV EPCP: commit - txId " + transactionId + ", Writing to HLOG : Threw an exception " + exp1.toString());
                    throw exp1;
                 }
            } // e1 is not empty
         } // not full edit write in commit record during phase 2
        else { //do this for  rollover case
           if (LOG.isTraceEnabled()) LOG.trace("TRAF RCOV EPCP:commit -- HLOG rollover txId: " + transactionId);
           commitTag = state.formTransactionalContextTag(TS_CONTROL_POINT_COMMIT);
           tagList.add(commitTag);
           WALEdit e1 = state.getEdit();
           WALEdit e = new WALEdit();

           for (Cell c : e1.getKeyValues()) {
              KeyValue kv = new KeyValue(c.getRowArray(), c.getRowOffset(), (int)c.getRowLength(),
					c.getFamilyArray(), c.getFamilyOffset(), (int)c.getFamilyLength(),
					c.getQualifierArray(), c.getQualifierOffset(), (int) c.getQualifierLength(),
					c.getTimestamp(), Type.codeToType(c.getTypeByte()), c.getValueArray(), c.getValueOffset(),
					c.getValueLength(), tagList);
              e.add(kv);
            }
            try {
                txid = this.tHLog.appendNoSync(this.regionInfo, this.regionInfo.getTable(),
                     e, new ArrayList<UUID>(), EnvironmentEdgeManager.currentTimeMillis(), this.m_Region.getTableDesc(),
                     nextLogSequenceId, false, HConstants.NO_NONCE, HConstants.NO_NONCE);
                if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit - txId " + transactionId + ", Y11 write commit HLOG seq " + txid);
            }
            catch (IOException exp1) {
               if (LOG.isTraceEnabled()) LOG.trace("TRAF RCOV EPCP: commit - txId " + transactionId + ", Writing to HLOG : Threw an exception " + exp1.toString());
               throw exp1;
             }
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:commit -- EXIT txId: " + transactionId + " HLog seq " + txid);
            if (this.fullEditInCommit) this.fullEditInCommit = false;
        } // else -- full edit write in commit record during phase 2
    } // write or reinstated

    state.setStatus(Status.COMMITED);
    if (state.hasWrite() || state.isReinstated()) {
      synchronized (commitPendingTransactions) {
      if (!commitPendingTransactions.remove(state)) {
          LOG.fatal("TrxRegionEndpoint coprocessor: commit - txid: " + transactionId + ", Commiting a non-query transaction that is not in commitPendingTransactions");
          // synchronized statements are cleared for a throw
        throw new IOException("commit failure");
      }
    }
    }

    if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: commit(tstate) -- EXIT TrxTransactionState: " + 
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
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit  - txId " + transactionId + ", Trafodion Recovery: delete in commit recovery zNode TM " + tmid + " region encoded name " + lv_encoded + " for 0 in-doubt transaction");
              deleteRecoveryzNode(tmid, lv_encoded);
            } catch (IOException e) {
            LOG.error("TrxRegionEndpoint coprocessor: commit - txId " + transactionId + ", Trafodion Recovery: delete recovery zNode failed. Caught exception " + e.toString());
            }
        }

        if ((indoubtTransactionsById == null) || (indoubtTransactionsById.size() == 0)) {
          if (indoubtTransactionsById == null) 
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit - txId " + transactionId + ", Trafodion Recovery: start region in commit with indoubtTransactionsById null");
          else
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit - txId " + transactionId + ", Trafodion Recovery: start region in commit with indoubtTransactionsById size " + indoubtTransactionsById.size());
          startRegionAfterRecovery();
        }
      }
    }
    state.setCommitProgress(CommitProgress.COMMITED);
    retireTransaction(state, false);
  }

  /**
   * Rssolves the transaction from the log
   * @param TrxTransactionState transactionState
   * @throws IOException 
   */
  private void resolveTransactionFromLog(
    final TrxTransactionState transactionState) throws IOException {
    LOG.error("TrxRegionEndpoint coprocessor: Global transaction log is not Implemented. (Optimisticly) assuming transaction commit!");
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
   long transactionId = 0L;
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: leaseExpired Transaction [" + this.transactionName
              + "] expired in region ["
              + m_Region.getRegionInfo().getRegionNameAsString() + "]");
   TrxTransactionState s = null;
   synchronized (transactionsById) {
     s = transactionsById.remove(transactionName);
     if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: leaseExpired Removing transaction: " + this.transactionName + " from list");
   }
   if (s == null) {
     LOG.warn("leaseExpired Unknown transaction expired " + this.transactionName);
     return;
   }

   transactionId = s.getTransactionId(); 

   switch (s.getStatus()) {
     case PENDING:
       if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: leaseExpired transaction " + transactionId + " was PENDING, calling retireTransaction");
       s.setStatus(Status.ABORTED);  
       retireTransaction(s, true);
       break;
      case COMMIT_PENDING:
       if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: leaseExpired  Transaction " + transactionId
                                + " expired in COMMIT_PENDING state");

        String key = getTransactionalUniqueId(transactionId);
        try {
          if (s.getCommitPendingWaits() > MAX_COMMIT_PENDING_WAITS) {
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: leaseExpired  Checking transaction status in transaction log");
            resolveTransactionFromLog(s);
            break;
          }
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: leaseExpired  renewing lease and hoping for commit");
          s.incrementCommitPendingWaits();
          synchronized (transactionsById) {
            transactionsById.put(key, s);
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: leaseExpired  Adding transaction: " + transactionId + " to list");
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
    TrxTransactionState state = this.beginTransIfNotExist(transactionId);
    state.addDelete(delete);
  }


  /**
   * Processes multiple transactional deletes    
   * @param long transactionId
   * @param Delete[] deletes   
   * @throws IOException 
   */
  public synchronized void delete(final long transactionId, Delete[] deletes)
    throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("Enter TrxRegionEndpoint coprocessor: deletes[], txid: " + transactionId);

    TrxTransactionState state = this.beginTransIfNotExist(transactionId);

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
  public boolean checkAndDelete(final long transactionId, 
                                byte[] row, byte[] family,
                                byte[] qualifier, byte[] value, Delete delete)
    throws IOException {

    if (LOG.isTraceEnabled()) LOG.trace("Enter TrxRegionEndpoint coprocessor: checkAndDelete, txid: "
                + transactionId + ", on HRegion " + this);

    TrxTransactionState state = this.beginTransIfNotExist(transactionId);
    boolean result = false;
    byte[] rsValue = null;
    byte[] startKey = null;
    byte[] endKey = null;

    if (!this.m_Region.rowIsInRange(this.regionInfo, row)) {
      startKey = this.regionInfo.getStartKey();
      endKey = this.regionInfo.getEndKey();
      LOG.error("Requested row out of range for " +
       "checkAndDelete for txid " + transactionId + ", on HRegion " +
       this + ", startKey=[" + Bytes.toStringBinary(startKey) +
       "], startKey in hex[" + Hex.encodeHexString(startKey) +
       "], endKey [" + Bytes.toStringBinary(endKey) +
       "], endKey in hex[" + Hex.encodeHexString(endKey) + "]" +
       "], row=[" + Bytes.toStringBinary(row) + "]");
     }

    try {

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
      }
    } catch (Exception e) {
      if (LOG.isWarnEnabled()) LOG.warn("TrxRegionEndpoint coprocessor: checkAndDelete - txid " + transactionId + ", Caught internal exception " + e.toString() + ", returning false");
     throw new IOException("TrxRegionEndpoint coprocessor: checkAndDelete - " + e.toString());
    }

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndDelete EXIT - returns " + result + ", transId " + transactionId + ", row " + Bytes.toStringBinary(row) + ", row in hex " + Hex.encodeHexString(row));

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
  public boolean checkAndPut(final long transactionId, byte[] row, byte[] family,
                            byte[] qualifier, byte[] value, Put put)
    throws IOException {

    if (LOG.isTraceEnabled()) LOG.trace("Enter TrxRegionEndpoint coprocessor: checkAndPut, txid: "
                + transactionId + ", on HRegion " + this);
    TrxTransactionState state = this.beginTransIfNotExist(transactionId);
    boolean result = false;
    byte[] rsValue = null;
    byte[] startKey = null;
    byte[] endKey = null;

    if (!this.m_Region.rowIsInRange(this.regionInfo, row)) {
      startKey = this.regionInfo.getStartKey();
      endKey = this.regionInfo.getEndKey();
      LOG.error("Requested row out of range for " +
       "checkAndPut for txid " + transactionId + ", on HRegion " +
       this + ", startKey=[" + Bytes.toStringBinary(startKey) +
       "], startKey in hex[" + Hex.encodeHexString(startKey) +
       "], endKey [" + Bytes.toStringBinary(endKey) +
       "], endKey in hex[" + Hex.encodeHexString(endKey) + "]" +
       "], row=[" + Bytes.toStringBinary(row) + "]");
     }

    try {
      Get get = new Get(row);
      get.addColumn(family, qualifier);
    
      Result rs = this.get(transactionId, get);

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
        else {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndPut - txid " + transactionId + ", row " + Bytes.toStringBinary(row) + ", row in hex " + Hex.encodeHexString(row) + ", first check setting result to false");
          result = false;
        }
      } else if ((!rs.isEmpty()) && !valueIsNull   
                && (Bytes.equals(rs.getValue(family, qualifier), value))) {
         this.put(transactionId, put);
         result = true;
      } else {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndPut - txid " + transactionId + ", row " + Bytes.toStringBinary(row) + ", row in hex " + Hex.encodeHexString(row) + ", second check setting result to false");
        result = false;
      }
    } catch (Exception e) {
      if (LOG.isWarnEnabled()) LOG.warn("TrxRegionEndpoint coprocessor: checkAndPut - txid " + transactionId + ", Caught internal exception " + e.toString() + ", returning false");
      throw new IOException("TrxRegionEndpoint coprocessor: checkAndPut - " + e.toString());
    }

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkAndPut EXIT - returns " + result + ", transId " + transactionId + ", row " + Bytes.toStringBinary(row) + ", row in hex " + Hex.encodeHexString(row));

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
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: get --  ENTRY txId: " + transactionId );
    
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
      if (scanner != null)
        scanner.next(results);       
    } catch(Exception e) {
      LOG.warn("TrxRegionEndpoint coprocessor: get - txId " + transactionId + ", Caught internal exception " + e.getMessage() + " " + stackTraceToString(e));
    }
    finally {
      if (scanner != null) {
        scanner.close();
      }
    }

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: get -- EXIT txId: " + transactionId);
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

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: RegionScanner getScanner -- ENTRY txId: " + transactionId );

    TrxTransactionState state = this.beginTransIfNotExist(transactionId);     

    state.addScan(scan);

    List<KeyValueScanner> scanners = new ArrayList<KeyValueScanner>(1);     

    scanners.add(state.getScanner(scan));

    Scan deleteWrapScan = wrapWithDeleteFilter(scan, state);
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: RegionScanner getScanner -- Calling t_Region.getScanner txId: " + transactionId );
    RegionScanner gotScanner =  this.t_Region.getScanner(deleteWrapScan, scanners); 
    if (gotScanner != null)
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: RegionScanner getScanner -- obtained scanner was not null,  txId: " + transactionId );
    else
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: RegionScanner getScanner -- obtained scanner was null,  txId: " + transactionId );
    return gotScanner;
  }

  /**
   * Wraps the transactional scan with a delete filter
   * @param Scan scan
   * @param TrxTransactionState state
   * @return Scan 
   */
  private Scan wrapWithDeleteFilter(final Scan scan,
                                    final TrxTransactionState state) {
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: wrapWithDeleteFilter -- ENTRY");
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
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: no previous filter, wrapWithDeleteFilter -- EXIT");
      return scan;
    }

    FilterList wrappedFilter = new FilterList(Arrays.asList(deleteFilter,
                                             scan.getFilter()));
    scan.setFilter(wrappedFilter);
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: new filter array, wrapWithDeleteFilter -- EXIT");

    return scan;
  }

  /**
   * Add a write to the transaction. Does not get applied until commit
   * process.
   * @param long transactionId
   * @param Put put
   * @throws IOException
   */

  public void put(final long transactionId, final Put put)
    throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("Enter TrxRegionEndpoint coprocessor: put, txid: " + transactionId);
    TrxTransactionState state = this.beginTransIfNotExist(transactionId);

    state.addWrite(put);
  }

  public void constructIndoubtTransactions() {

      synchronized (recoveryCheckLock) {
            if ((indoubtTransactionsById == null) || (indoubtTransactionsById.size() == 0)) {
              if (LOG.isInfoEnabled()) LOG.info("TRAF RCOV Endpoint Coprocessor: Region " + regionInfo.getRegionNameAsString() + " has no in-doubt transaction, set region START ");
              regionState = REGION_STATE_START; // region is started for transactional access
              reconstructIndoubts = 1; 
              try {
              startRegionAfterRecovery();
               } catch (IOException exp1) {
                    if (LOG.isDebugEnabled()) LOG.debug("Trafodion Recovery: flush error during region start");
               }
              return;
            }

            if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Endpoint Coprocessor: Trafodion Recovery RegionObserver to Endpoint coprocessor " +
                     "data exchange test try to access indoubt transaction list with size " + indoubtTransactionsById.size());

            if (reconstructIndoubts == 0) {
            //Retrieve (tid,Edits) from indoubt Transaction and construct/add into desired transaction data list
            for (Entry<Long, List<WALEdit>> entry : indoubtTransactionsById.entrySet()) {
                      long txid = 0;
                      long transactionId = entry.getKey();
		      String key = String.valueOf(transactionId);
                      ArrayList<WALEdit> editList = (ArrayList<WALEdit>) entry.getValue();
                      //editList = (ArrayList<WALEdit>) indoubtTransactionsById.get(transactionId);
                      if (LOG.isTraceEnabled()) LOG.trace("TrafodionEPCP: reconstruct transaction in Region " + regionInfo.getRegionNameAsString() + " process in-doubt transaction " + transactionId);
		      TrxTransactionState state = new TrxTransactionState(transactionId, /* 1L my_Region.getLog().getSequenceNumber()*/
                                                                                nextLogSequenceId.getAndIncrement(), nextLogSequenceId, 
                                                                                regionInfo, m_Region.getTableDesc(), tHLog, false);

                      if (LOG.isTraceEnabled()) LOG.trace("TrafodionEPCP: reconstruct transaction in Region " + regionInfo.getRegionNameAsString() + " create transaction state for " + transactionId);

                      state.setFullEditInCommit(true);
		      state.setStartSequenceNumber(nextSequenceId.get());
    		      transactionsById.put(getTransactionalUniqueId(transactionId), state);

                      // Re-establish write ordering (put and get) for in-doubt transactional
                     int num  = editList.size();
                     if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEPCP: reconstruct transaction " + transactionId + ", region " + regionInfo.getRegionNameAsString() +
                               " with number of edit list kvs size " + num);
                    for (int i = 0; i < num; i++){
                          WALEdit b = editList.get(i);
                          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEPCP: reconstruction transaction " + transactionId + ", region " + regionInfo.getRegionNameAsString() +
                               " with " + b.size() + " kv in WALEdit " + i);
                          for (KeyValue kv : b.getKeyValues()) {
                             Put put;
                             Delete del;
                             synchronized (editReplay) {
                             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEPCP:reconstruction transaction " + transactionId + ", region " + regionInfo.getRegionNameAsString() +
                               " re-establish write ordering Op Code " + kv.getType());
                             if (kv.getTypeByte() == KeyValue.Type.Put.getCode()) {
                               put = new Put(CellUtil.cloneRow(kv)); // kv.getRow()
                                put.add(CellUtil.cloneFamily(kv), CellUtil.cloneQualifier(kv), kv.getTimestamp(), CellUtil.cloneValue(kv));
                                state.addWrite(put);
                             }
                             else if (CellUtil.isDelete(kv))  {
                               del = new Delete(CellUtil.cloneRow(kv));
                                if (CellUtil.isDeleteFamily(kv)) {
                                   del.deleteFamily(CellUtil.cloneFamily(kv));
                                } else if (kv.isDeleteType()) {
                                   del.deleteColumn(CellUtil.cloneFamily(kv), CellUtil.cloneQualifier(kv));
                                }
                                 state.addDelete(del);

                             } // handle put/delete op code
                             } // sync editReplay
                          } // for all kv in edit b
                    } // for all edit b in ediList 
                      state.setReinstated();
		      state.setStatus(Status.COMMIT_PENDING);
		      commitPendingTransactions.add(state);
		      state.setSequenceNumber(nextSequenceId.getAndIncrement());
		      commitedTransactionsBySequenceNumber.put(state.getSequenceNumber(), state);
                    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEPCP: reconstruct transaction " + transactionId + ", region " + regionInfo.getRegionNameAsString() +
                              " complete in prepared state");

                     // Rewrite HLOG for prepared edit (this method should be invoked in postOpen Observer ??
                    try {
                       txid = this.tHLog.appendNoSync(this.regionInfo, this.regionInfo.getTable(),
                       state.getEdit(), new ArrayList<UUID>(), EnvironmentEdgeManager.currentTimeMillis(), this.m_Region.getTableDesc(),
                       nextLogSequenceId, false, HConstants.NO_NONCE, HConstants.NO_NONCE);
                       if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitRequest COMMIT_OK -- EXIT txId: " + transactionId + " HLog seq " + txid);
                       this.tHLog.sync(txid);
                    }
                    catch (IOException exp) {
                       LOG.warn("TrxRegionEPCP: reconstruct transaction - Caught IOException in HLOG appendNoSync -- EXIT txId: " + transactionId + " HLog seq " + txid);
                       //throw exp;
                    }   
                    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEPCP: reconstruct transaction: rewrite to HLOG CR edit for transaction " + transactionId);
                      int tmid = (int) (transactionId >> 32);
                    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEPCP " + regionInfo.getRegionNameAsString() + " reconstruct transaction " + transactionId + " for TM " + tmid);
             } // for all txns in indoubt transcation list
             } // not reconstruct indoubtes yet
             reconstructIndoubts = 1;
             if (this.configuredConflictReinstate) {
	         regionState = REGION_STATE_START; // set region state START , so new transaction can start with conflict re-established
             }
        } // synchronized
  }

  /**
   * Begin a transaction
   * @param longtransactionId
   * @throws IOException
   */

  public void beginTransaction(final long transactionId)
     throws IOException {

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: beginTransaction -- ENTRY txId: " + transactionId);
    checkClosing(transactionId);

    // TBD until integration with recovery 
    if (reconstructIndoubts == 0) {
       if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: RECOV beginTransaction -- ENTRY txId: " + transactionId);
       constructIndoubtTransactions();
    }

    if (regionState != REGION_STATE_START) {
        //print out all the in-doubt transaction at this moment
        if ((indoubtTransactionsById == null) || (indoubtTransactionsById.size() == 0))
           regionState = REGION_STATE_START;
        else {
           LOG.warn("TRAF RCOV coprocessor: RECOVERY WARN beginTransaction while the region is still in recovering state " +  regionState + " indoubt tx size " + indoubtTransactionsById.size() );
           for (Entry<Long, List<WALEdit>> entry : indoubtTransactionsById.entrySet()) {
               long tid = entry.getKey();
               if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery: region " + regionInfo.getEncodedName() + " still has in-doubt transaction " + tid + " when new transaction arrives ");
           }
           throw new IOException("NewTransactionStartedBeforeRecoveryCompleted");
        }
    }

    TrxTransactionState state;
    synchronized (transactionsById) {
//      if (transactionsById.get(getTransactionalUniqueId(transactionId)) != null) {
//        TrxTransactionState alias = getTransactionState(transactionId);

//        LOG.error("TrxRegionEndpoint coprocessor: beginTransaction - Ignoring - Existing transaction with id ["
//                   + transactionId + "] in region [" + m_Region.getRegionInfo().getRegionNameAsString() + "]");

//        if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: beginTransaction -- EXIT txId: " + transactionId);

//        return;
//      }

      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: beginTransaction -- creating new TrxTransactionState without coprocessorHost txId: " + transactionId);

      state = new TrxTransactionState(transactionId,
                                //this.m_Region.getLog().getSequenceNumber(),
                                nextLogSequenceId.getAndIncrement(),
                                nextLogSequenceId,
                                m_Region.getRegionInfo(),
                                m_Region.getTableDesc(), tHLog, configuredEarlyLogging);

      state.setFullEditInCommit(this.fullEditInCommit);
      state.setStartSequenceNumber(nextSequenceId.get());
    }

    List<TrxTransactionState> commitPendingCopy = 
        new ArrayList<TrxTransactionState>(commitPendingTransactions);

    for (TrxTransactionState commitPending : commitPendingCopy) {
            state.addTransactionToCheck(commitPending);
    }

    String key = getTransactionalUniqueId(transactionId);
    synchronized (transactionsById) {
      transactionsById.put(key, state);
    }

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: beginTransaction - Adding transaction: [" + transactionId + "] in region ["
                + m_Region.getRegionInfo().getRegionNameAsString() + "]" +
                 " to list");
    try {
      transactionLeases.createLease(key, transactionLeaseTimeout, new TransactionLeaseListener(transactionId));
    } catch (LeaseStillHeldException e) {
      LOG.error("TrxRegionEndpoint coprocessor: beginTransaction - Lease still held for [" + transactionId + "] in region ["
                + m_Region.getRegionInfo().getRegionNameAsString() + "]");
        throw new RuntimeException(e);
    }

    if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: beginTransaction -- EXIT txId: " + transactionId + " transactionsById size: " + transactionsById.size()
    		 + " region ID: " + this.regionInfo.getRegionId());
  }

  /**
   * Obtains a scanner lease id                            
   * @param long scannerId
   * @return String 
   */
  private String getScannerLeaseId(final long scannerId) {
    String lstring = m_Region.getRegionInfo().getRegionNameAsString() + scannerId;

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getScannerLeaseId -- EXIT txId: " 
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
	LOG.trace("TrxRegionEndpoint coprocessor: getTransactionalUniqueId -- EXIT txId: " 
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
  private TrxTransactionState beginTransIfNotExist(final long transactionId) throws IOException{

    if (LOG.isTraceEnabled()) LOG.trace("Enter TrxRegionEndpoint coprocessor: beginTransIfNotExist, txid: "
              + transactionId + " transactionsById size: "
              + transactionsById.size());

    String key = getTransactionalUniqueId(transactionId);
    synchronized (transactionsById) {
      TrxTransactionState state = transactionsById.get(key);

      if (state == null) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: Begin transaction in beginTransIfNotExist beginning the transaction internally as state was null");
        this.beginTransaction(transactionId);
        state =  transactionsById.get(key);
      }
      return state;
    }
  }


  /**
   * Commits the transaction
   * @param long TransactionId
   * @throws IOException
   */
  public void commit(final long transactionId) throws IOException {
     commit(transactionId, false /* IgnoreUnknownTransactionException */);
  }

  /**
   * Commits the transaction                        
   * @param long TransactionId
   * @param boolean ignoreUnknownTransactionException
   * @throws IOException 
   */
  public void commit(final long transactionId, final boolean ignoreUnknownTransactionException) throws IOException {
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit(txId) -- ENTRY txId: " + transactionId +
              " ignoreUnknownTransactionException: " + ignoreUnknownTransactionException);
    CommitProgress commitStatus = CommitProgress.NONE;
    TrxTransactionState state;
    try {
      state = getTransactionState(transactionId);
    } catch (UnknownTransactionException e) {
      if (ignoreUnknownTransactionException == true) {
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: ignoring UnknownTransactionException in commit : " + transactionId
                + " in region "
                + m_Region.getRegionInfo().getRegionNameAsString());
         return;
      }
      LOG.fatal("TrxRegionEndpoint coprocessor: Asked to commit unknown transaction: " + transactionId
                + " in region "
                + m_Region.getRegionInfo().getRegionNameAsString());
      throw new IOException("UnknownTransactionException");
    }

    if (!state.getStatus().equals(Status.COMMIT_PENDING)) {
      LOG.fatal("TrxRegionEndpoint coprocessor: commit - Asked to commit a non pending transaction ");

      throw new IOException("Asked to commit a non-pending transaction");
    }
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit(txId) -- EXIT txId: " + transactionId);

    // manage concurrent duplicate commit requests through TS.xaOperation object

    synchronized(state.getXaOperationObject()) {
        commitStatus = state.getCommitProgress();
        //if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit HHH " + commitStatus);
        if (commitStatus == CommitProgress.COMMITED) { // already committed, this is likely unnecessary due to Status check above
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit - duplicate commit for committed transaction ");
        }
        else if (commitStatus == CommitProgress.COMMITTING) {
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit - duplicate commit during committing transaction ");
            try {
                  Thread.sleep(1000);          ///1000 milliseconds is one second.
            } catch(InterruptedException ex) {
                  Thread.currentThread().interrupt();
            }
        }
        else if (commitStatus == CommitProgress.NONE) {
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit " + commitStatus);
            state.setCommitProgress(CommitProgress.COMMITTING);
    commit(state);

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commit(txId) -- EXIT txId: " + transactionId);
  } 
    }

  } 

  /**
   * @param transactionId
   * @return TransactionRegionInterface commit code
   * @throws IOException
   */
  public int commitRequest(final long transactionId) throws IOException {
    long txid = 0;
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitRequest -- ENTRY txId: " + transactionId);
    TrxTransactionState state;

    int lv_totalCommits = 0;
    int lv_timeIndex = 0;
    if (LOG.isInfoEnabled()) {
      synchronized (totalCommits){
         lv_totalCommits = totalCommits.incrementAndGet();
         lv_timeIndex = (timeIndex.getAndIncrement() % 50 );
      }
    }

    boolean returnPending = false;
    long commitCheckEndTime = 0;
    long hasConflictStartTime = 0;
    long hasConflictEndTime = 0;
    long putBySequenceStartTime = 0;
    long putBySequenceEndTime = 0;
    long writeToLogEndTime = 0;
    long commitCheckStartTime = System.nanoTime();

    try {
      state = getTransactionState(transactionId);
    } catch (UnknownTransactionException e) {
      if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: commitRequest Unknown transaction [" + transactionId
                 + "] in region [" 
                 + m_Region.getRegionInfo().getRegionNameAsString()
                 + "], ignoring");
     state = null;
    }
      // may change to indicate a NOTFOUND case  then depends on the TM ts state, if reinstated tx, ignore the exception
      if (state == null) {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitRequest encountered unknown transactionID txId: " + transactionId + " returning COMMIT_UNSUCCESSFUL_FROM_COPROCESSOR");
        return COMMIT_UNSUCCESSFUL_FROM_COPROCESSOR;
      }

    if (LOG.isInfoEnabled()) 
      hasConflictStartTime = System.nanoTime();

    synchronized (commitCheckLock) {
      if (hasConflict(state)) {
        if (LOG.isInfoEnabled()) {
          hasConflictEndTime = System.nanoTime();
          hasConflictTimes[lv_timeIndex] = hasConflictEndTime - hasConflictStartTime;
          totalConflictTime += hasConflictTimes[lv_timeIndex];
        }
        state.setStatus(Status.ABORTED);
        retireTransaction(state, true);
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitRequest encountered conflict txId: " + transactionId + "returning COMMIT_CONFLICT");
        return COMMIT_CONFLICT;
      }

      if (LOG.isInfoEnabled()) 
          hasConflictEndTime = System.nanoTime();

      // No conflicts, we can commit.
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: No conflicts for transaction " + transactionId
		+ " found in region "
		+ m_Region.getRegionInfo().getRegionNameAsString()
		+ ". Votes to commit");

      // If there are writes we must keep record of the transaction
      putBySequenceStartTime = System.nanoTime();
      if (state.hasWrite()) {
        if (LOG.isInfoEnabled())
          putBySequenceOperations.getAndIncrement();
        // Order is important
	state.setStatus(Status.COMMIT_PENDING);
        state.setCPEpoch(controlPointEpoch.get());
	commitPendingTransactions.add(state);
	state.setSequenceNumber(nextSequenceId.getAndIncrement());
	commitedTransactionsBySequenceNumber.put(state.getSequenceNumber(), state);
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: Transaction " + transactionId
		+ " found in region "
		+ m_Region.getRegionInfo().getRegionNameAsString()
		+ ". Adding to commitedTransactionsBySequenceNumber for sequence number " + state.getSequenceNumber());

      }
      commitCheckEndTime = putBySequenceEndTime = System.nanoTime();
    } // exit sync block of commitCheckLock
                
    if (state.hasWrite()) {
      if (LOG.isTraceEnabled()) LOG.trace("write commitRequest edit to HLOG");
      //call HLog by passing tagged WALEdits and associated fields
      try {

      // Once we append edit into HLOG during DML operation, there is no need to do any HLOG write in phase 1.
      // Likely all the edits have been synced, so just do a sync(state.getLargestFlushTxId()) --> most likely, it is an no-op
      // This is to leverage the time between last DML and phase 1, so there is no need to do any logging (and waited) in phase 1.
      // And there is no need to write "prepared" record since we try to optimize normal running mode (99.99% commit)
      // We can append "commit" or "abort" into HLOG in phase 2 in a no-sync mode, but it can also be eliminated if necessary.
      // All the transaction reinstated during recovery will be treated as "in-doubt" and need to use TLOG to resolve. 
      // So TLOG must be kept for a while (likely a few CP), besides, HRegion chore thread can perform ~ CP action by writing
      //  the smallest sequenceId (not logSeqid), so we don't need to deal with commite case (but we did write abort edit since
      //  the number of abort is < 0.1% -- we can set this as a configurable property).

            if (!state.getEarlyLogging()) {
                  txid = this.tHLog.appendNoSync(this.regionInfo, this.regionInfo.getTable(),
                  state.getEdit(), new ArrayList<UUID>(), EnvironmentEdgeManager.currentTimeMillis(), this.m_Region.getTableDesc(),
                  nextLogSequenceId, false, HConstants.NO_NONCE, HConstants.NO_NONCE);
                  if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitRequest COMMIT_OK -- EXIT txId: " + transactionId + " HLog seq " + txid);
                  this.tHLog.sync(txid);
            }
            else {
                 //if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: YYY0 commitRequest just SYNC -- EXIT txId: " + transactionId + " MAX HLog seq " + state.getFlushTxId());
                 this.tHLog.sync(state.getFlushTxId());
            }
            if (LOG.isInfoEnabled()) {
                     writeToLogEndTime = System.nanoTime();
                     writeToLogTimes[lv_timeIndex] = writeToLogEndTime - commitCheckEndTime;
                    writeToLogOperations.getAndIncrement();
            }
            //if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:commitRequest COMMIT_OK -- EXIT txId: " + transactionId + " HLog seq " + txid);
      } catch (IOException exp) {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitRequest - Caught IOException in HLOG appendNoSync -- EXIT txId: " + transactionId + " HLog seq " + txid);
          throw exp;
      }      
      if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: commitRequest COMMIT_OK -- EXIT txId: " + transactionId);
      returnPending = true;
    }
    // No write pending
    else {
      if (LOG.isInfoEnabled())
        writeToLogTimes[lv_timeIndex] = 0;
    }

    if (LOG.isInfoEnabled()) {
      commitCheckTimes[lv_timeIndex] = commitCheckEndTime - commitCheckStartTime;
      hasConflictTimes[lv_timeIndex] = hasConflictEndTime - hasConflictStartTime;
      putBySequenceTimes[lv_timeIndex] = putBySequenceEndTime - putBySequenceStartTime;
      totalCommitCheckTime += commitCheckTimes[lv_timeIndex];
      totalConflictTime += hasConflictTimes[lv_timeIndex];
      totalPutTime += putBySequenceTimes[lv_timeIndex];
      totalWriteToLogTime += writeToLogTimes[lv_timeIndex];
      if (commitCheckTimes[lv_timeIndex] > maxCommitCheckTime) {
         maxCommitCheckTime = commitCheckTimes[lv_timeIndex];
      }
      if (commitCheckTimes[lv_timeIndex] < minCommitCheckTime) {
         minCommitCheckTime = commitCheckTimes[lv_timeIndex];
      }
      if (hasConflictTimes[lv_timeIndex] > maxConflictTime) {
         maxConflictTime = hasConflictTimes[lv_timeIndex];
      }
      if (hasConflictTimes[lv_timeIndex] < minConflictTime) {
         minConflictTime = hasConflictTimes[lv_timeIndex];
      }
      if (putBySequenceTimes[lv_timeIndex] > maxPutTime) {
         maxPutTime = putBySequenceTimes[lv_timeIndex];
      }
      if (putBySequenceTimes[lv_timeIndex] < minPutTime) {
         minPutTime = putBySequenceTimes[lv_timeIndex];
      }
      if (writeToLogTimes[lv_timeIndex] > maxWriteToLogTime) {
         maxWriteToLogTime = writeToLogTimes[lv_timeIndex];
      }
      if (writeToLogTimes[lv_timeIndex] < minWriteToLogTime) {
         minWriteToLogTime = writeToLogTimes[lv_timeIndex];
      }

      if (lv_timeIndex == 49) {
         timeIndex.set(1);  // Start over so we don't exceed the array size
      }

      if (lv_totalCommits == 9999) {
       avgCommitCheckTime = (double) (totalCommitCheckTime/lv_totalCommits);
       avgConflictTime = (double) (totalConflictTime/lv_totalCommits);
       avgPutTime = (double) (totalPutTime/lv_totalCommits);
       avgWriteToLogTime = (double) ((double)totalWriteToLogTime/(double)lv_totalCommits);
       if (LOG.isInfoEnabled()) LOG.info("commitRequest Report\n" +
                      "  Region: " + m_Region.getRegionInfo().getRegionNameAsString() + "\n" +
                      "                        Total commits: "
                         + lv_totalCommits + "\n" +
                      "                        commitCheckLock time:\n" +
                      "                                     Min:  "
                         + minCommitCheckTime / 1000 + " microseconds\n" +
                      "                                     Max:  "
                         + maxCommitCheckTime / 1000 + " microseconds\n" +
                      "                                     Avg:  "
                         + avgCommitCheckTime / 1000 + " microseconds\n" +
                      "                        hasConflict time:\n" +
                      "                                     Min:  "
                         + minConflictTime / 1000 + " microseconds\n" +
                      "                                     Max:  "
                         + maxConflictTime / 1000 + " microseconds\n" +
                      "                                     Avg:  "
                         + avgConflictTime / 1000 + " microseconds\n" +
                      "                        putBySequence time:\n" +
                      "                                     Min:  "
                         + minPutTime / 1000 + " microseconds\n" +
                      "                                     Max:  "
                         + maxPutTime / 1000 + " microseconds\n" +
                      "                                     Avg:  "
                         + avgPutTime / 1000 + " microseconds\n" +
                      "                                     Ops:  "
                         + putBySequenceOperations.get() + "\n" +
                      "                        writeToLog time:\n" +
                      "                                     Min:  "
                         + minWriteToLogTime / 1000 + " microseconds\n" +
                      "                                     Max:  "
                         + maxWriteToLogTime / 1000 + " microseconds\n" +
                      "                                     Avg:  "
                         + avgWriteToLogTime / 1000 + " microseconds\n" +
                      "                                     Ops:  "
                         + writeToLogOperations.get() + "\n\n");
                   totalCommits.set(0);
                   writeToLogOperations.set(0);
                   putBySequenceOperations.set(0);
                   totalCommitCheckTime    =    0;
                   totalConflictTime =    0;
                   totalPutTime      =    0;
                   totalWriteToLogTime     =    0;
                   minCommitCheckTime      =    1000000000;
                   maxCommitCheckTime      =    0;
                   avgCommitCheckTime      =    0;
                   minConflictTime   =    1000000000;
                   maxConflictTime   =    0;
                   avgConflictTime   =    0;
                   minPutTime        =    1000000000;
                   maxPutTime        =    0;
                   avgPutTime        =    0;
                   minWriteToLogTime =    1000000000;
                   maxWriteToLogTime =    0;
                   avgWriteToLogTime =    0;
                }
    } // end of LOG.Info

    if (returnPending) {
      return COMMIT_OK;
    }

    // Otherwise we were read-only and commitable, so we can forget it.
    state.setStatus(Status.COMMITED);
    retireTransaction(state, true);
    if (LOG.isDebugEnabled()) LOG.debug("TrxRegionEndpoint coprocessor: commitRequest READ ONLY -- EXIT txId: " + transactionId);
    return COMMIT_OK_READ_ONLY;
  }

  /**
   * Determines if the transaction has any conflicts
   * @param TrxTransactionState state
   * @return boolean
   */
  private boolean hasConflict(final TrxTransactionState state) {
    // Check transactions that were committed while we were running
      
    synchronized (commitedTransactionsBySequenceNumber) {
      for (long i = state.getStartSequenceNumber(); i < nextSequenceId.get(); i++)
      {
        TrxTransactionState other = commitedTransactionsBySequenceNumber.get(i);
        if (other == null) {
          continue;
        }

        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: hasConflict state.getStartSequenceNumber  is " + i + ", nextSequenceId.get() is " + nextSequenceId.get() + ", state object is " + state.toString() + ", calling addTransactionToCheck");

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

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: abort transactionId: " + transactionId + " " + m_Region.getRegionInfo().getRegionNameAsString());

    TrxTransactionState state;
    try {
      state = getTransactionState(transactionId);
    } catch (UnknownTransactionException e) {
      IOException ioe = new IOException("UnknownTransactionException");
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: abortTransaction - Unknown transaction [" + transactionId
                 + "] in region [" 
                 + m_Region.getRegionInfo().getRegionNameAsString()
                 + "], " + ioe.toString());
      
      throw new IOException("UnknownTransactionException");
    }

    synchronized(state.getXaOperationObject()) {
        if (state.getStatus().equals(Status.ABORTED)) { // already aborted, duplicate abort requested
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: abortTransaction - duplicate abort transaction Id: " + transactionId + " " + m_Region.getRegionInfo().getRegionNameAsString());
            return;
        }
    state.setStatus(Status.ABORTED);
    }

    if (state.hasWrite()) {
    // TODO log
    //  this.transactionLog.writeAbortToLog(m_Region.getRegionInfo(),
    //                                      state.getTransactionId(),
    //                                    m_Region.getTableDesc());
       if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: abortTransaction - abort write to HLOG");
       Tag abortTag = state.formTransactionalContextTag(TS_ABORT);
       List<Tag> tagList = new ArrayList<Tag>();
       tagList.add(abortTag);

      WALEdit e1 = state.getEdit();
      WALEdit e = new WALEdit();

      if (e1.getKeyValues().size() > 0) {
         // get 1st Cell to associated with the abort record as a workaround through HLOG async append
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
             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: Y99 write abort HLOG " + transactionId + " HLog seq " + txid);
         }
         catch (IOException exp1) {
           if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: abortTransaction - abort writing to HLOG : Caught an exception " + exp1.toString());
           throw exp1;
          }
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: abortTransaction -- EXIT txId: " + transactionId + " HLog seq " + txid);
      }
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
             if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: Trafodion Recovery: start region in abort with indoubtTransactionsById null");
            else
              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: Trafodion Recovery: start region in abort with indoubtTransactionsById size " + indoubtTransactionsById.size());
            startRegionAfterRecovery();
         }
       }
     }

   retireTransaction(state, true);

   if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: abortTransaction looking for abort transaction " + transactionId + ", transactionsById " + transactionsById.size() + ", commitedTransactionsBySequenceNumber " + commitedTransactionsBySequenceNumber.size() + ", commitPendingTransactions" + commitPendingTransactions.size());
   }

  /**
   * Determines if the transaction can be committed, and if possible commits the transaction.
   * @param long transactionId
   * @return boolean
   * @throws IOException
   */
  public boolean commitIfPossible(final long transactionId)
    throws IOException {

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitIfPossible -- ENTRY txId: "
               + transactionId);
    int status = commitRequest(transactionId);
  
    if (status == COMMIT_OK) {

       // Process local memory
       try {
         commit(transactionId);
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitIfPossible -- ENTRY txId: " + transactionId + " COMMIT_OK");
         return true;
       } catch (Throwable e) {
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitIfPossible - txId " + transactionId + ", Caught exception after internal commit call "
                    + e.getMessage() + " " + stackTraceToString(e));
        throw new IOException(e.toString());
       }
    } else if (status == COMMIT_OK_READ_ONLY) {
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitIfPossible -- ENTRY txId: " 
            + transactionId + " COMMIT_OK_READ_ONLY");
            return true;
    }
    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: commitIfPossible -- ENTRY txId: " 
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
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: cleanup - NotServingRegionException; " +  t.getMessage());
      return t;
    }
    if (msg == null) {
      LOG.error("TrxRegionEndpoint coprocessor: cleanup - cleanup message was null");
    } else {
      LOG.error("TrxRegionEndpoint coprocessor: cleanup - cleanup message was " + msg);
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
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkFileSystemAvailable - File System not available threw IOException " + e.getMessage());
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

      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getScanner - scanner id " + scannerId + ", count is "  + scanners.size());

      TransactionalRegionScannerHolder rsh = 
        scanners.get(scannerId);

      if (rsh != null)
      {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  getScanner - rsh is " + rsh + "rsh.s is "  + rsh.s );
      }
      else
      {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor:  getScanner - rsh is null");
          throw new UnknownScannerException(
            "TrxRegionEndpoint getScanner - scanner id " + scannerId + ", already closed?");
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
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getScanner - scanner id " + scannerId + ", calling OutOfOrderScannerNextException, nextCallSeq is " + nextCallSeq + " rsh.nextCallSeq is " + rsh.nextCallSeq);
        throw new OutOfOrderScannerNextException(
        "TrxRegionEndpoint coprocessor: getScanner - scanner id " + scannerId + ", Expected nextCallSeq: " + rsh.nextCallSeq +
        ", But the nextCallSeq received from client: " + nextCallSeq); 
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

      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: removeScanner - scanner id " + scannerId + ", before count is "  + scanners.size());
      TransactionalRegionScannerHolder rsh = 
        scanners.remove(scannerId);
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: removeScanner - scanner id " + scannerId + ", after count is "  + scanners.size());
      if (rsh != null)
      {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: removeScanner - scanner id " + scannerId + ", rsh is " + rsh + ", rsh.s is " + rsh.s );
        RegionScanner s = rsh.s;
        rsh.cleanHolder();
        return s;
      }
      else
      {
        if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: removeScanner - scanner id " + scannerId + ", rsh is null");
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
      new TransactionalRegionScannerHolder(transId, scannerId, s, r);

    if (rsh != null)
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: addScanner - scanner id " + scannerId + ", rsh is " + rsh);
    else
      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: addScanner - scanner id " + scannerId + ", rsh is null");
  
    TransactionalRegionScannerHolder existing =
      scanners.putIfAbsent(scannerId, rsh);

    if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: addScanner - scanner id " + scannerId + ", count is " + scanners.size());

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
      TrxTransactionState state = null;
      int numRemoved = 0;
      long key = 0;

      if (minStartSeqNumber == null) {
        minStartSeqNumber = Long.MAX_VALUE;  
      }

      synchronized (commitedTransactionsBySequenceNumber) {
      for (Entry<Long, TrxTransactionState> entry : new LinkedList<Entry<Long, TrxTransactionState>>(
        commitedTransactionsBySequenceNumber.entrySet())) {
          key = entry.getKey();
          if (key >= minStartSeqNumber) {
            break;
          }

          state = commitedTransactionsBySequenceNumber.remove(key);
  
          if (state != null) {
            state.clearState();
            numRemoved++;
            //if (LOG.isTraceEnabled()) LOG.trace("removeUnNeededCommitedTransactions: Transaction - entry key " + key + ", " + state.toString());
          }
        }
      }

      if (LOG.isTraceEnabled()) {
         StringBuilder traceMessage = new StringBuilder();
         if (numRemoved > 0) {
            traceMessage.append("TrxRegionEndpoint coprocessor: removeUnNeededCommitedTransactions: Removed [").append(numRemoved).append("] commited transactions");

            if (minStartSeqNumber == Long.MAX_VALUE) {
              traceMessage.append(" with any sequence number.");
            } else {
               traceMessage.append(" with sequence lower than [").append(minStartSeqNumber).append("].");
            }

             if (!commitedTransactionsBySequenceNumber.isEmpty()) {
                traceMessage.append(" Still have [").append(commitedTransactionsBySequenceNumber.size())
                          .append("] left.");
             } else {
                traceMessage.append(" None left.");
             }
             LOG.trace(traceMessage.toString());
         } else if (commitedTransactionsBySequenceNumber.size() > 0) {
            traceMessage.append("Could not remove any transactions, and still have ")
                         .append(commitedTransactionsBySequenceNumber.size())
                         .append(" left");
            LOG.trace(traceMessage.toString());
         }
      }
  }

  /**
   * Removes unneeded TransactionalRegionScannerHolder objects             
   */

  synchronized public void removeUnNeededStaleScanners() {

    long scannerId = 0L;
    long transId = 0L;
    long listSize = 0L;
    long scannerSize = 0L;
    Long transactionId = 0L;
    TransactionalRegionScannerHolder rsh = null;
    Iterator<Long> transIter = null;
    Iterator<Map.Entry<Long, TransactionalRegionScannerHolder>> scannerIter = null;
    synchronized (cleanScannersForTransactions) {

      listSize = cleanScannersForTransactions.size();

      if (listSize == 0)
        return;

      if (scanners == null || scanners.isEmpty())
        return;

      scannerSize = scanners.size();
      scannerIter = scanners.entrySet().iterator();

      if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: removeUnNeededStaleScanners - transactions list count is " + listSize + ", scanners size is " + scannerSize);

      for (transIter = cleanScannersForTransactions.iterator(); transIter.hasNext();) {

        transactionId = transIter.next();

        while(scannerIter.hasNext()){

          Map.Entry<Long, TransactionalRegionScannerHolder> entry = scannerIter.next();
          rsh = entry.getValue();

          if (rsh != null) {
            transId = rsh.transId;
            scannerId = rsh.scannerId;

            if (transId == transactionId) {

              if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: removeUnNeededStaleScanners - txId " + transactionId + ", scannerId " + scannerId + ", Removing stale scanner ");

              try {
                if (rsh.s != null) {
                  if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: removeUnNeededStaleScanners - txId " + transactionId + ", scannerId " + scannerId + ", Scanner was not previously closed ");
                  rsh.s.close();
                }
                rsh.s = null;
                rsh.r = null;
                scannerIter.remove();
              }
              catch (Exception e) {
                if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: removeUnNeededStaleScanners - txId " + transactionId + ", scannerId " + scannerId + ", Caught exception " + e.toString());
              }
            }
          }
        }
      }  // End of for loop

      cleanScannersForTransactions.clear();

    }  // End of synchronization

  }

  /**
   * Returns the minimum start sequence number
   * @return Integer
   */
  private Long getMinStartSequenceNumber() {

    List<TrxTransactionState> transactionStates;

    synchronized (transactionsById) {
      transactionStates = new ArrayList<TrxTransactionState>(
      transactionsById.values());
    }

    Long min = null;

    for (TrxTransactionState transactionState : transactionStates) {
      try {
         if (min == null || transactionState.getStartSequenceNumber() < min) {
            min = transactionState.getStartSequenceNumber();
         }
      }
      catch(NullPointerException npe){
         if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: getMinStartSequenceNumber ignoring NullPointerException ");
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

  synchronized public void checkMemoryUsage() {

    long memUsed = 0L;
    long memMax = 0L;

    if (memoryUsageThreshold < DEFAULT_MEMORY_THRESHOLD &&
        memoryBean != null) {
      memUsed = memoryBean.getHeapMemoryUsage().getUsed();
      memMax = memoryBean.getHeapMemoryUsage().getMax();

      memoryPercentage = 0L;

      if (memMax != 0) {
        memoryPercentage = (memUsed * 100) / memMax;
      }

      memoryThrottle = false;
      if (memoryPercentage > memoryUsageThreshold) {
        // If configured to perform a garbage collection,
        // try to release memory before throttling the queries.
        if (memoryUsagePerformGC == true) {
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkMemoryUsage - before GC, memoryPercentage is " + memoryPercentage);
          System.gc();
          // Calculate the memory usage again before
          // setting the throttle value or post a warning.
          memUsed = memoryBean.getHeapMemoryUsage().getUsed();
          memMax = memoryBean.getHeapMemoryUsage().getMax();
          memoryPercentage = 0L;

          if (memMax != 0) {
            memoryPercentage = (memUsed * 100) / memMax;
          }

          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkMemoryUsage - after GC, memoryPercentage is " + memoryPercentage);

          if (memoryPercentage > memoryUsageThreshold) {
            if(memoryUsageWarnOnly == false)
              memoryThrottle = true;
            if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkMemoryUsage - memoryPercentage is " + memoryPercentage + ", memoryThrottle is "+ memoryThrottle);
          }
        } else {
          if(memoryUsageWarnOnly == false)
            memoryThrottle = true;
          if (LOG.isTraceEnabled()) LOG.trace("TrxRegionEndpoint coprocessor: checkMemoryUsage - memoryPercentage is " + memoryPercentage + ", memoryThrottle is "+ memoryThrottle);
        }
      }
    }
  }
  @Override
  public void abortTransactionMultiple(RpcController controller,
      AbortTransactionMultipleRequest request, RpcCallback<AbortTransactionMultipleResponse> done) {
    // TODO Auto-generated method stub

  }

  @Override
  public void commitRequestMultiple(RpcController controller, CommitRequestMultipleRequest request,
      RpcCallback<CommitRequestMultipleResponse> done) {
    // TODO Auto-generated method stub

  }

  @Override
  public void commitMultiple(RpcController controller, CommitMultipleRequest request,
      RpcCallback<CommitMultipleResponse> done) {
    // TODO Auto-generated method stub

  }
}

//1}
