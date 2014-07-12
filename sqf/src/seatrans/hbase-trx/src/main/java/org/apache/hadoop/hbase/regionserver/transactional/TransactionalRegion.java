/**
 * Copyright 2009 The Apache Software Foundation Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements. See the NOTICE file distributed with this work for additional information regarding
 * copyright ownership. The ASF licenses this file to you under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0 Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and limitations under the
 * License.
 */
package org.apache.hadoop.hbase.regionserver.transactional;

//import java.io.DataInput;
//import java.io.DataOutput;
//import java.io.IOException;
import java.io.*;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.SortedMap;
import java.util.TreeMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.lang.reflect.InvocationTargetException;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.ServerName;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.transactional.HBaseBackedTransactionLogger;
import org.apache.hadoop.hbase.client.transactional.UnknownTransactionException;
import org.apache.hadoop.hbase.filter.FilterBase;
import org.apache.hadoop.hbase.filter.FilterList;
import org.apache.hadoop.hbase.ipc.TransactionalRegionInterface;
import org.apache.hadoop.hbase.monitoring.MonitoredTask;
import org.apache.hadoop.hbase.regionserver.HRegion;
import org.apache.hadoop.hbase.regionserver.HRegionServer;
import org.apache.hadoop.hbase.regionserver.KeyValueScanner;
import org.apache.hadoop.hbase.regionserver.LeaseException;
import org.apache.hadoop.hbase.regionserver.LeaseListener;
import org.apache.hadoop.hbase.regionserver.Leases;
import org.apache.hadoop.hbase.regionserver.Leases.LeaseStillHeldException;
import org.apache.hadoop.hbase.regionserver.RegionScanner;
import org.apache.hadoop.hbase.regionserver.RegionServerServices;
import org.apache.hadoop.hbase.regionserver.StoreFile;
import org.apache.hadoop.hbase.regionserver.WrongRegionException;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionState.Status;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionState.WriteAction;
import org.apache.hadoop.hbase.regionserver.wal.HLog;
import org.apache.hadoop.hbase.regionserver.wal.HLogSplitter;
import org.apache.hadoop.hbase.regionserver.wal.WALEdit;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.CancelableProgressable;
import org.apache.hadoop.hbase.zookeeper.ZooKeeperWatcher;
import org.apache.hadoop.hbase.zookeeper.ZKUtil;
import org.apache.zookeeper.KeeperException;

/**
 * Regionserver which provides transactional support for atomic transactions.
 * This is achieved with optimistic concurrency control (see
 * http://www.seas.upenn.edu/~zives/cis650/papers/opt-cc.pdf). We keep track
 * read and write sets for each transaction, and hold off on processing the
 * writes. To decide to commit a transaction we check its read sets with all
 * transactions that have committed while it was running for overlaps.
 * <p>
 * Because transactions can span multiple regions, all regions must agree to
 * commit a transactions. The client side of this commit protocol is encoded in
 * org.apache.hadoop.hbase.client.transactional.TransactionManger
 * <p>
 * In the event of an failure of the client mid-commit, (after we voted yes), we
 * will have to consult the transaction log to determine the final decision of
 * the transaction. This is not yet implemented.
 */
public class TransactionalRegion extends HRegion {

	private static final String OLD_TRANSACTION_FLUSH = "hbase.transaction.flush";
	private static final int DEFAULT_OLD_TRANSACTION_FLUSH = 100; // Do a flush
																	// if
	// we have this
	// many old
	// transactions..

	static final Log LOG = LogFactory.getLog(TransactionalRegion.class);

	// Collection of active transactions (PENDING) keyed by id.
	protected Map<Long, TransactionState> transactionsById = new HashMap<Long, TransactionState>();

	// Map of recent transactions that are COMMIT_PENDING or COMMITED keyed by
	// their sequence number
	private SortedMap<Integer, TransactionState> commitedTransactionsBySequenceNumber = Collections
			.synchronizedSortedMap(new TreeMap<Integer, TransactionState>());

	// Collection of transactions that are COMMIT_PENDING
	private Set<TransactionState> commitPendingTransactions = Collections
			.synchronizedSet(new HashSet<TransactionState>());

        // an in-doubt transaction list during recovery WALEdit replay
        private Map<Long, WALEdit> indoubtTransactionsById = new TreeMap<Long, WALEdit>();
        private Map<Integer, Integer> indoubtTransactionsCountByTmid = new TreeMap<Integer,Integer>();

	private AtomicInteger nextSequenceId = new AtomicInteger(0);
	private Object commitCheckLock = new Object();
	private Object recoveryCheckLock = new Object();
        private Object editReplay = new Object();
	private THLog transactionLog;
	private final int oldTransactionFlushTrigger;
        private final Boolean splitDelayEnabled;
        private final Boolean doWALHlog;
	private Leases transactionLeases;
        private int regionState = 0;  // 0 is INIT, 1 is RECOVERING, and 2 is START, put into constant later
        private Path recoveryTrxPath;
        private int cleanAT = 0; // 0 is to process edits from split-THLOG and resolve in-doubt transaction, (default, value set in genms)
                                                  // 1 is to process split-THLOG but ignore any in-doubt transaction
                                                  // else is to ignore split-THLOG, so we basically discard all the edits from split-THLOG and returns 

        private static long[] commitCheckTimes;
        private static long[] hasConflictTimes;
        private static long[] putBySequenceTimes;
        private static long[] writeToLogTimes;

        private static AtomicInteger  timeIndex;
        private static AtomicInteger  totalCommits;
        private static AtomicInteger  writeToLogOperations;
        private static AtomicInteger  putBySequenceOperations;
        private static long   totalCommitCheckTime;
        private static long   totalConflictTime;
        private static long   totalPutTime;
        private static long   totalWriteToLogTime;
        private static long   minCommitCheckTime;
        private static long   maxCommitCheckTime;
        private static double avgCommitCheckTime;
        private static long   minConflictTime;
        private static long   maxConflictTime;
        private static double avgConflictTime;
        private static long   minPutTime;
        private static long   maxPutTime;
        private static double avgPutTime;
        private static long   minWriteToLogTime;
        private static long   maxWriteToLogTime;
        private static double avgWriteToLogTime;

	/**
	 * @param basedir
	 * @param log
	 * @param fs
	 * @param conf
	 * @param regionInfo
	 * @param flushListener
	 */
	public TransactionalRegion(final Path basedir, final HLog log,
			final FileSystem fs, final Configuration conf,
			final HRegionInfo regionInfo, final HTableDescriptor htd,
			final RegionServerServices rsServices) {
		super(basedir, log, fs, conf, regionInfo, htd, rsServices);
		oldTransactionFlushTrigger = conf.getInt(OLD_TRANSACTION_FLUSH,
				DEFAULT_OLD_TRANSACTION_FLUSH);
                this.splitDelayEnabled = conf.getBoolean("hbase.regionserver.region.split.delay",
                                                          true);
                this.doWALHlog = conf.getBoolean("hbase.regionserver.region.transactional.hlog", true);
                LOG.debug("Trafodion Recovery: WAL HLOG setting " + this.doWALHlog);

                boolean doTlog = conf.getBoolean("hbase.regionserver.region.transactional.tlog", false);
                LOG.debug("Trafodion Recovery: TM TLOG setting " + doTlog);
                if (doTlog) this.cleanAT = 0;
                else this.cleanAT = 1;

                commitCheckTimes        = new long[1000];
                hasConflictTimes        = new long[1000];
                putBySequenceTimes      = new long[1000];
                writeToLogTimes         = new long[1000];

                timeIndex               =    new AtomicInteger (0);
                totalCommits            =    new AtomicInteger (0);
                writeToLogOperations    =    new AtomicInteger (0);
                putBySequenceOperations =    new AtomicInteger (0);
                totalCommitCheckTime    =    0;
                totalConflictTime       =    0;
                totalPutTime            =    0;
                totalWriteToLogTime     =    0; 
                minCommitCheckTime      =    1000000000;
                maxCommitCheckTime      =    0;
                avgCommitCheckTime      =    0;
                minConflictTime         =    1000000000;
                maxConflictTime         =    0;
                avgConflictTime         =    0;
                minPutTime              =    1000000000;
                maxPutTime              =    0;
                avgPutTime              =    0;
                minWriteToLogTime       =    1000000000;
                maxWriteToLogTime       =    0;
                avgWriteToLogTime       =    0;


	}

	/**
	 * Open HRegion. Calls initialize and sets sequenceid to both regular WAL
	 * and trx WAL.
	 * 
	 * @param reporter
	 * @return Returns <code>this</code>
	 * @throws IOException
	 */
	@Override
	protected HRegion openHRegion(final CancelableProgressable reporter)
			throws IOException {
                LOG.trace("openHRegion -- ENTRY");
		super.openHRegion(reporter);
		if (this.transactionLog != null) {
			this.transactionLog.setSequenceNumber(super.getLog()
					.getSequenceNumber());
		}
                LOG.trace("openHRegion -- EXIT");
		return this;
	}

        // This is the method used by Cloudera HBase 0.94.6 and possibly other versions
        @Override
	protected long replayRecoveredEditsIfAny(final Path regiondir,
			//final Map<byte[], Long> maxSeqIdInStores,
			final long minSeqId, 
                        final CancelableProgressable reporter,
			final MonitoredTask status) throws UnsupportedEncodingException,
			IOException {

                //          The purpose is to retrieve minSeqId, do we need to perform other task in super
                //          for example, doReconstructionLog call getCommitFromLog will read Edits rather than
                //          super.replayRecoveredEdits
                //
                // We don't use reflection here, since the super class used at compile time
                // matches this signature and we should not invoke this method at runtime
                // on systems that don't have this method
		long maxSeqId = super.replayRecoveredEditsIfAny(regiondir,
                                //maxSeqIdInStores,
                                minSeqId,
				reporter, status);

		Path recoveredEdits = new Path(regiondir, HLogSplitter.RECOVERED_EDITS);

                LOG.trace("Trafodion Recovery: replayRecoveredEditsIfAny -- Path " + recoveredEdits);

		doReconstructionLog(recoveredEdits, 
                                    //maxSeqIdInStores,
                                    minSeqId,
                                    maxSeqId, reporter);

                LOG.trace("Trafodion Recovery: replayRecoveredEditsIfAny -- EXIT");
		return maxSeqId;
	}

        // This is the method used by MapR HBase 0.94.13 and possibly other versions
        @Override
	protected long replayRecoveredEditsIfAny(final Path regiondir,
			final Map<byte[], Long> maxSeqIdInStores,
                        //final long minSeqId, 
                        final CancelableProgressable reporter,
			final MonitoredTask status) throws UnsupportedEncodingException,
			IOException {
                LOG.trace("replayRecoveredEditsIfAny with Map param -- ENTRY");

                // The actual HBase code will have only one of the two
                // replayRecoveredEditsIfAny methods used here and above, but
                // we use a modified HBase source tree that has both methods declared
                long maxSeqId = super.replayRecoveredEditsIfAny(regiondir,
                                maxSeqIdInStores,
                                //minSeqId,
				reporter, status);

		Path recoveredEdits = new Path(regiondir, HLogSplitter.RECOVERED_EDITS);

                LOG.trace("replayRecoveredEditsIfAny -- Path " + recoveredEdits);

		doReconstructionLog(recoveredEdits,
                                    maxSeqIdInStores,
                                    //minSeqId,
                                    maxSeqId, reporter);

                LOG.trace("replayRecoveredEditsIfAny with Map param -- EXIT");
		return maxSeqId;
	}

        // This is the method used by Cloudera HBase 0.94.6 and possibly other versions
        // @Override
	protected void doReconstructionLog(final Path oldCoreLogFile,
			// final Map<byte[], Long> maxSeqIdInStores,
			final long minSeqId,
                        final long maxSeqId,
			final CancelableProgressable reporter)
			throws UnsupportedEncodingException, IOException {
                LOG.trace("doReconstructionLog -- ENTRY");
		
		//Path trxPath = new Path(oldCoreLogFile.getParent(),
		Path trxPath = new Path(oldCoreLogFile,THLog.HREGION_OLD_THLOGFILE_NAME);
                recoveryTrxPath = trxPath;

		// We can ignore doing anything with the Trx Log table, it is
		// not-transactional.
		if (super.getTableDesc().getNameAsString()
				.equals(HBaseBackedTransactionLogger.TABLE_NAME)) {
                        LOG.trace("doReconstructionLog -- EXIT");
			return;
		}

                String toCleanAT = System.getenv("TM_CLEAN_THLOG_MODE");
                if (toCleanAT != null)
                    cleanAT = Integer.parseInt(toCleanAT);
                LOG.debug("Trafodion Recovery: TM clean AT mode " + cleanAT);

		THLogRecoveryManager recoveryManager = new THLogRecoveryManager(this);
                FileSystem fileSystem = getFilesystem();

                LOG.debug("Trafodion Recovery: begin to recover a region from recovdered edits path " + trxPath);

                // get pendingTransactionById from THLogRecoverymanager or discard split-THLOG
                if ((cleanAT == 0) || (cleanAT == 1))
		     indoubtTransactionsById = recoveryManager.getCommitsFromLog(trxPath, minSeqId, reporter);
                else {
                     indoubtTransactionsById = null;
                     LOG.debug("Trafodion Recovery: TM clean AT mode " + cleanAT + " discards split-THLOG ");
                 }

                // discard any in-doubt transaction and start the region
                if (cleanAT == 1) {
                   if ((indoubtTransactionsById != null) && (indoubtTransactionsById.size() > 0)) {
                      LOG.debug("Trafodion Recovery: TM clean AT mode " + cleanAT + " discards " + indoubtTransactionsById.size() + " in-doubt transaction ");
                      indoubtTransactionsById.clear();
                   }
                }

		//Map<Long, WALEdit> commitedTransactionsById = recoveryManager
		//		.getCommitsFromLog(trxPath, minSeqId, reporter);
		//		.getCommitsFromLog(trxPath, minSeqIdForTheRegion, reporter);

                regionState = 1; // region recovering
                LOG.debug("Trafodion Recovery: Region " + getRegionNameAsString() + " is in state RECOVERING ");
              
                // remove split-log under region dir if TRegion has been recovered competely
                if ((indoubtTransactionsById == null) || (indoubtTransactionsById.size() == 0)) {
                     // call method startRegionAfterRecovery to 1) archive the split-thlog, and 2) set region state = STARTED
                     LOG.debug("Trafodion Recovery: Region " + getRegionNameAsString() + " has no in-doubt transaction, set region START ");
                     startRegionAfterRecovery();
                     return;
                }

                LOG.debug("Trafodion Recovery: Region " + getRegionNameAsString() + " find " + indoubtTransactionsById.size() + 
                                                     " in-doubt transaction during edit replay, now reconstruct transaction state ");

                for (Entry<Long, WALEdit> entry : indoubtTransactionsById.entrySet()) {
                  synchronized (recoveryCheckLock) {
                      long transactionId = entry.getKey();
		      String key = String.valueOf(transactionId);
                      LOG.debug("Trafodion Recovery: Region " + getRegionNameAsString() + " process in-doubt transaction " + transactionId);
		      if (transactionsById.get(transactionId) != null) {
			  LOG.error("Trafodion Recovery: Existing trasaction with id [" + key + "] in region ["
			 		+ super.getRegionInfo().getRegionNameAsString() + "] during transaction reinstatement");
			  throw new IOException("Already exiting transaction id: " + key);
		      }

		      TransactionState state = new TransactionState(transactionId, super
			   .getLog().getSequenceNumber(), super.getRegionInfo(), super.getTableDesc());

                      LOG.debug("Trafodion Recovery: Region " + getRegionNameAsString() + " create transaction state for " + transactionId);

		       state.setStartSequenceNumber(nextSequenceId.get());

		       //synchronized (transactionsById) {
			   transactionsById.put(transactionId, state);
		       //}
         
                        state.setReinstated();
		        state.setStatus(Status.COMMIT_PENDING);
		        commitPendingTransactions.add(state);
		        state.setSequenceNumber(nextSequenceId.getAndIncrement());
		        commitedTransactionsBySequenceNumber.put(state.getSequenceNumber(), state);
                        int tmid = (int) (transactionId >> 32);
                        int count = 1;
                        LOG.debug("Trafodion Recovery: Region " + getRegionNameAsString() + " add prepared " + transactionId + " to TM " + tmid);
                        if (indoubtTransactionsCountByTmid.containsKey(tmid))
                            count =  (int) indoubtTransactionsCountByTmid.get(tmid) + 1;

                        indoubtTransactionsCountByTmid.put(tmid, count);
                        LOG.debug("Trafodion Recovery: Region " + getRegionNameAsString() + " has " + count +
                                                      " in-doubt-transaction from TM " + tmid);
                        // no need to write the LOG again for reinstated txn (redo does not generate edits)
                   } // synchronized
                }

                ServerName sn = TransactionalRegionServer.getRecoveryRSName();
              	String lv_hostName = sn.getHostname();
	        int lv_port = sn.getPort();

                // construct region info into byte[]
	        ByteArrayOutputStream lv_bos = new ByteArrayOutputStream();
	        DataOutputStream lv_dos = new DataOutputStream(lv_bos);
	        super.getRegionInfo().write(lv_dos);
	        lv_dos.flush();
	        byte [] lv_byte_region_info = lv_bos.toByteArray();

                String lv_encoded = super.getRegionInfo().getEncodedName();
   
                // loop for every tm, call TRS.createzNode (tmid, region encoded name, zNodedata)
                for (int node  : indoubtTransactionsCountByTmid.keySet()) {
                      try {
                            LOG.debug("Trafodion Recovery: ZKW Create Recovery zNode TM " + node + " region encoded name " + lv_encoded + " region info bytes " + new String(lv_byte_region_info));
                            //TransactionalRegionServer.createRecoveryzNode(node, lv_encoded, lv_b);
                            TransactionalRegionServer.createRecoveryzNode(node, lv_encoded, lv_byte_region_info);
                      } catch (IOException e) {
                       LOG.error("Trafodion Recovery: ZKW Create recovery zNode failed");
                      }
                }
                LOG.debug("Trafodion Recovery: ZKW Complete post of recovery zNode for region info " + new String(lv_byte_region_info));

                LOG.debug("Trafodion Recovery:  Flushing cache after doReconstructionLog in region " + getRegionInfo().getRegionNameAsString());
                if (!super.flushcache()) {
                   LOG.debug("Trafodion Recovery:  Flushcache returns false !!! " + getRegionInfo().getRegionNameAsString());
                }
                LOG.trace("doReconstructionLog -- EXIT");
	}

        // This is the method used by MapR HBase 0.94.13 and possibly other versions
        // @Override
	protected void doReconstructionLog(final Path oldCoreLogFile,
			final Map<byte[], Long> maxSeqIdInStores,
                        //final long minSeqId,
                        final long maxSeqId,
			final CancelableProgressable reporter)
			throws UnsupportedEncodingException, IOException {
                LOG.trace("doReconstructionLog with Map param -- ENTRY");
		
                long minSeqIdForTheRegion = -1;
                for (Long  maxSeqIdInStore : maxSeqIdInStores.values()) {
                    if (maxSeqIdInStore < minSeqIdForTheRegion || minSeqIdForTheRegion == -1) {
                        minSeqIdForTheRegion = maxSeqIdInStore;
                    }
                }

                doReconstructionLog(oldCoreLogFile,
                                    minSeqIdForTheRegion,
                                    maxSeqId,
                                    reporter);
                LOG.trace("doReconstructionLog with Map param -- EXIT");
	}

        public void startRegionAfterRecovery() throws IOException {
        boolean isFlush = false;
                try {
                      LOG.trace("Trafodion Recovery:  Flushing cache in startRegionAfterRecovery " + getRegionInfo().getRegionNameAsString());
                      if (!super.flushcache()) {
                         LOG.trace("Trafodion Recovery:  Flushcache returns false !!! " + getRegionInfo().getRegionNameAsString());
                      }
                 } catch (IOException e) {
                 LOG.error("Trafodion Recovery: Flush failed after replay edits" + getRegionInfo().getRegionNameAsString());
                 return;
                 }

                FileSystem fileSystem = getFilesystem();
                Path archiveTHLog = new Path (recoveryTrxPath.getParent(),"archivethlogfile.log");
                if (fileSystem.exists(archiveTHLog)) fileSystem.delete(archiveTHLog);
                if (fileSystem.exists(recoveryTrxPath))fileSystem.rename(recoveryTrxPath,archiveTHLog);
                if (indoubtTransactionsById != null) 
                    LOG.trace("Trafodion Recovery: region " + recoveryTrxPath + " has " + indoubtTransactionsById.size() + " in-doubt transactions and edits are archived.");
                else
                    LOG.trace("Trafodion Recovery: region " + recoveryTrxPath + " has 0 in-doubt transactions and edits are archived.");
                regionState = 2; // region started
                LOG.debug("Trafodion Recovery: region " + super.getRegionInfo().getEncodedName() + " is STARTED.");
        }

        public void replayCommittedTransaction(long transactionId, WALEdit val) throws IOException {

                LOG.debug("Trafodion Recovery:   " + getRegionInfo().getRegionNameAsString() + " replay commit for transaction: " + transactionId);
       		for (KeyValue kv : val.getKeyValues()) {
                        synchronized (editReplay) {
                             LOG.debug("Trafodion Recovery:   " + getRegionInfo().getRegionNameAsString() + " replay commit for transaction: "
                                                         + transactionId);
                             LOG.debug("Trafodion Recovery:   " + getRegionInfo().getRegionNameAsString() + " replay commit for transaction: "
                                                         + transactionId + " with Op " + kv.getType());
			     if (kv.getType() == KeyValue.Type.Put.getCode()) {
				Put put = new Put(kv.getRow());
                                put.add(kv.getFamily(), kv.getQualifier(), kv.getTimestamp(), kv.getValue());
				//put.add(kv);
				super.put(put);
			     } else if (kv.isDelete()) {
				Delete del = new Delete(kv.getRow());
			        	if (kv.isDeleteFamily()) {
				 	     del.deleteFamily(kv.getFamily());
				        } else if (kv.isDeleteType()) {
				             del.deleteColumn(kv.getFamily(), kv.getQualifier());
				        }
                                super.delete(del, false);
			     }
                        }
		}
        }

	/**
	 * We need to make sure that we don't complete a cache flush between running
	 * transactions. If we did, then we would not find all log messages needed
	 * to restore the transaction, as some of them would be before the last
	 * "complete" flush id.
	 */
	@Override
	protected long getCompleteCacheFlushSequenceId(final long currentSequenceId) {
		LinkedList<TransactionState> transactionStates;
		synchronized (transactionsById) {
			transactionStates = new LinkedList<TransactionState>(
					transactionsById.values());
		}

		long minPendingStartSequenceId = currentSequenceId;
		for (TransactionState transactionState : transactionStates) {
			minPendingStartSequenceId = Math.min(minPendingStartSequenceId,
					transactionState.getHLogStartSequenceId());
		}
		return minPendingStartSequenceId;
	}

	/**
	 * @param transactionId
	 * @throws IOException
	 */
	public void beginTransaction(final long transactionId) throws IOException {
                LOG.debug("beginTransaction -- ENTRY txId: " + transactionId);
		checkClosing(transactionId);
                // block new transaction to begin if the region has not completed recovery
                if (regionState != 2) {
                   LOG.debug("Trafodion Recovery: RECOVERY WARN beginTransaction while the region is still in recovering state " +  regionState);
                }
		if (transactionsById.get(transactionId) != null) {
			TransactionState alias = getTransactionState(transactionId);
			/* Commented for now
			if (alias != null) {
				alias.setStatus(Status.ABORTED);
				retireTransaction(alias);
			}
			End commented section */
			LOG.error("Ignoring - Existing transaction with id [" + transactionId + "] in region ["
					+ super.getRegionInfo().getRegionNameAsString() + "]");
			//			throw new IOException("Already existing transaction id: " + key);
			// The following is a hack for now
                        LOG.trace("beginTransaction -- EXIT txId: " + transactionId);
			return;
		}

		TransactionState state = new TransactionState(transactionId, super
				.getLog().getSequenceNumber(), super.getRegionInfo(), super.getTableDesc());

		state.setStartSequenceNumber(nextSequenceId.get());
		List<TransactionState> commitPendingCopy = new ArrayList<TransactionState>(
				commitPendingTransactions);
		for (TransactionState commitPending : commitPendingCopy) {
			state.addTransactionToCheck(commitPending);
		}

		synchronized (transactionsById) {
			transactionsById.put(transactionId, state);
                        // Logging to catch error 97
                        LOG.debug("Adding transaction: [" + transactionId + "] in region ["
                             + super.getRegionInfo().getRegionNameAsString() + "]" + " to list");
		}
		try {
			transactionLeases.createLease(getLeaseId(transactionId),
					new TransactionLeaseListener(transactionId));
		} catch (LeaseStillHeldException e) {
			LOG.error("Lease still held for [" + transactionId + "] in region ["
					+ super.getRegionInfo().getRegionNameAsString() + "]");
			throw new RuntimeException(e);
		}

		/* maybeTriggerOldTransactionFlush(); */
                LOG.trace("beginTransaction -- EXIT txId: " + transactionId + " transactionsById size: " + transactionsById.size());
	}

	private String getLeaseId(final long transactionId) {
		return super.getRegionInfo().getRegionNameAsString() + transactionId;
	}

	public Result get(final long transactionId, final Get get)
			throws IOException {
                LOG.trace("get -- ENTRY txId: " + transactionId );
		Scan scan = new Scan(get);
		List<KeyValue> results = new ArrayList<KeyValue>();

		RegionScanner scanner = null;
		try {
			scanner = getScanner(transactionId, scan);
			scanner.next(results);
		} catch(Exception e) {
			e.printStackTrace();
			
		}
		finally {
		
			if (scanner != null) {
				scanner.close();
			}
		}
                LOG.trace("get -- EXIT txId: " + transactionId);
		return new Result(results);
	}

	

	/**
	 * Get a transactional scanner.
	 */
	// public InternalScanner getScanner(final long transactionId, final Scan
	// scan) throws IOException {
	public RegionScanner getScanner(final long transactionId, final Scan scan)
			throws IOException {
                LOG.trace("getScanner -- ENTRY txId: " + transactionId );
		TransactionState state = this.beginTransIfNotExist(transactionId);

		state.addScan(scan);
		List<KeyValueScanner> scanners = new ArrayList<KeyValueScanner>(1);
		scanners.add(state.getScanner(scan));
                LOG.trace("getScanner -- EXIT txId: " + transactionId );
		return super.getScanner(wrapWithDeleteFilter(scan, state), scanners);
	}

	private Scan wrapWithDeleteFilter(final Scan scan,
			final TransactionState state) {
                LOG.trace("wrapWithDeleteFilter -- ENTRY");
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
			public void filterRow(final List<KeyValue> kvs) {
				state.applyDeletes(kvs, scan.getTimeRange().getMin(), scan
						.getTimeRange().getMax());
				rowFiltered = kvs.isEmpty();
			}

			@Override
			public boolean filterRow() {
				return rowFiltered;
			}

			@Override
			public void write(final DataOutput out) throws IOException {
				// does nothing
			}

			@Override
			public void readFields(final DataInput in) throws IOException {
				// does nothing
			}
		};

		if (scan.getFilter() == null) {
			scan.setFilter(deleteFilter);
                        LOG.trace("wrapWithDeleteFilter -- EXIT");
			return scan;
		}

		FilterList wrappedFilter = new FilterList(Arrays.asList(deleteFilter,
				scan.getFilter()));
		scan.setFilter(wrappedFilter);
                LOG.trace("wrapWithDeleteFilter -- EXIT");
		return scan;
	}

	/**
	 * Add a write to the transaction. Does not get applied until commit
	 * process.
	 * 
	 * @param transactionId
	 * @param put
	 * @throws IOException
	 */
	public synchronized void put(final long transactionId, final Put put) throws IOException {
	    LOG.trace("Enter TransactionalRegion.put, txid: " + transactionId);
	    TransactionState state = this.beginTransIfNotExist(transactionId);

		state.addWrite(put);
	}

	/**
	 * Add multiple writes to the transaction. Does not get applied until commit
	 * process.
	 * 
	 * @param transactionId
	 * @param puts
	 * @throws IOException
	 */
	public void put(final long transactionId, final Put[] puts)
			throws IOException {
	    LOG.trace("Enter TransactionalRegion.puts[], txid: " + transactionId);
	    TransactionState state = this.beginTransIfNotExist(transactionId);

		for (Put put : puts) {
			state.addWrite(put);
		}
	}

	/**
	 * Add a delete to the transaction. Does not get applied until commit
	 * process.
	 * 
	 * @param transactionId
	 * @param delete
	 * @throws IOException
	 */
	public void delete(final long transactionId, final Delete delete)
			throws IOException {
		LOG.trace("delete -- ENTRY txId: " + transactionId);
		checkClosing(transactionId);
		TransactionState state = this.beginTransIfNotExist(transactionId);
		state.addDelete(delete);
	}

        // add the doRecover to reply a list of in-doubt transactions from TM nn
        public List<Long> doRecoveryRequest(final int tmid) throws IOException {
	     //checkClosing();
        
             List<Long> indoubtTransactions = new ArrayList<Long>();

             LOG.trace("Trafodion Recovery: region " + super.getRegionInfo().getEncodedName() + " receives recovery request from TM " + tmid  + " with region state " + regionState);
             switch(regionState) {
                   case 0: // INIT, assume open the TRegion if necessary
                          regionState = 1;  //Note. ??? should we call openHRegion directly here
                          break;
                   case 1: // RECOVERING, already create a list of in-doubt txn, but still in the state of resolving them,
                                // retrieve all in-doubt txn from rmid and return them into a long a
                         for (TransactionState state : commitPendingTransactions) {
                               long tid = state.getTransactionId();
                               if ((int) (tid >> 32) == tmid) {
                                   indoubtTransactions.add(tid);
                                   LOG.debug("Trafodion Recovery: region " + super.getRegionInfo().getEncodedName() + " in-doubt transaction " + tid + "has been added into the recovery repply to TM " + tmid);
                               }
                          } 
                          break;
                   case 2: // START but there are indoubt transactions not ....
                          break;
                    }

               return indoubtTransactions;
        }

	/**
	 * @param transactionId
	 * @return TransactionRegionInterface commit code
	 * @throws IOException
	 */
	public int commitRequest(final long transactionId) throws IOException {
                LOG.debug("commitRequest -- ENTRY txId: " + transactionId);
		checkClosing(transactionId);
                TransactionState state;
                int lv_totalCommits;
                int lv_timeIndex;
                synchronized (totalCommits){
                   lv_totalCommits = totalCommits.incrementAndGet();
                   lv_timeIndex = timeIndex.getAndIncrement();
                }

                long commitCheckStartTime = 0;
                long commitCheckEndTime = 0;
                long hasConflictStartTime = 0;
                long hasConflictEndTime = 0;
                long putBySequenceStartTime = 0;
                long putBySequenceEndTime = 0;
                long writeToLogEndTime = 0;

                boolean returnPending = false;

                LOG.debug("commitRequest timeIndex is " + lv_timeIndex);
                commitCheckStartTime = System.nanoTime();
		synchronized (commitCheckLock) {
                        commitCheckStartTime = System.nanoTime();
                        state = getTransactionState(transactionId);
                        // may change to indicate a NOTFOUND case  then depends on the TM ts state, if reinstated tx, ignore the exception
			if (state == null) {
                                LOG.info("commitRequest encountered unknown transactionID txId: " + transactionId + " returning COMMIT_UNSUCCESSFUL");
				return TransactionalRegionInterface.COMMIT_UNSUCCESSFUL;
			}

                        hasConflictStartTime = System.nanoTime();
			if (hasConflict(state)) {
                           hasConflictEndTime = System.nanoTime();
                           hasConflictTimes[lv_timeIndex] = hasConflictEndTime - hasConflictStartTime;
                           totalConflictTime += hasConflictTimes[lv_timeIndex];
                           state.setStatus(Status.ABORTED);
                           retireTransaction(state);
                           LOG.info("commitRequest encountered conflict txId: " + transactionId + "returning COMMIT_CONFLICT");
                           return TransactionalRegionInterface.COMMIT_CONFLICT;
			}
                        else {
                           hasConflictEndTime = System.nanoTime();
                        }
			// No conflicts, we can commit.
			LOG.trace("No conflicts for transaction " + transactionId
					+ " found in region "
					+ super.getRegionInfo().getRegionNameAsString()
					+ ". Voting for commit");

			// If there are writes we must keep record of the transaction
                        putBySequenceStartTime = System.nanoTime();
			if (state.hasWrite()) {
				// Order is important
                                putBySequenceOperations.getAndIncrement();
				state.setStatus(Status.COMMIT_PENDING);
				commitPendingTransactions.add(state);
				state.setSequenceNumber(nextSequenceId.getAndIncrement());
				commitedTransactionsBySequenceNumber.put(
						state.getSequenceNumber(), state);
                        }
                        commitCheckEndTime = putBySequenceEndTime = System.nanoTime();
                } // exit sync block of commitCheckLock
                
	        if (state.hasWrite()) {
       			transactionLog.writeCommitRequestToLog(getRegionInfo(), state);
                        writeToLogEndTime = System.nanoTime();
                        writeToLogTimes[lv_timeIndex] = writeToLogEndTime - commitCheckEndTime;
                        writeToLogOperations.getAndIncrement();
                        LOG.debug("commitRequest COMMIT_OK -- EXIT txId: " + transactionId);
                        returnPending = true;
//			return TransactionalRegionInterface.COMMIT_OK;
		}
                else {
                   writeToLogTimes[lv_timeIndex] = 0;
                }

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

                if ((lv_timeIndex % 500) == 0) {
                   avgCommitCheckTime = (double) (totalCommitCheckTime/lv_totalCommits);
                   avgConflictTime = (double) (totalConflictTime/lv_totalCommits);
                   avgPutTime = (double) (totalPutTime/lv_totalCommits);
                   avgWriteToLogTime = (double) ((double)totalWriteToLogTime/(double)lv_totalCommits);
                   LOG.info("commitRequest Report\n" + 
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
                   timeIndex.set(1);
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

                if (returnPending) {
                  return TransactionalRegionInterface.COMMIT_OK;
                }

 		// Otherwise we were read-only and commitable, so we can forget it.
		state.setStatus(Status.COMMITED);
		retireTransaction(state);
                LOG.debug("commitRequest READ ONLY -- EXIT txId: " + transactionId);
	        return TransactionalRegionInterface.COMMIT_OK_READ_ONLY;
	}

	/**
	 * @param transactionId
	 * @return true if commit is successful
	 * @throws IOException
	 */
	public boolean commitIfPossible(final long transactionId)
			throws IOException {
                LOG.trace("commitIfPossible -- ENTRY txId: " + transactionId);
		int status = commitRequest(transactionId);

		if (status == TransactionalRegionInterface.COMMIT_OK) {
			commit(transactionId);
                        LOG.trace("commitIfPossible -- ENTRY txId: " + transactionId + " COMMIT_OK");
			return true;
		} else if (status == TransactionalRegionInterface.COMMIT_OK_READ_ONLY) {
                        LOG.trace("commitIfPossible -- ENTRY txId: " + transactionId + " COMMIT_OK_READ_ONLY");
			return true;
		}
                LOG.trace("commitIfPossible -- ENTRY txId: " + transactionId + " Commit Unsuccessful");
		return false;
	}

	private boolean hasConflict(final TransactionState state) {
		// Check transactions that were committed while we were running
		for (int i = state.getStartSequenceNumber(); i < nextSequenceId.get(); i++) {
			TransactionState other = commitedTransactionsBySequenceNumber
					.get(i);
			if (other == null) {
				continue;
			}
			state.addTransactionToCheck(other);
		}

		return state.hasConflict();
	}

	/**
	 * Commit the transaction.
	 * 
	 * @param transactionId
	 * @throws IOException
	 */
	public void commit(final long transactionId) throws IOException {
		LOG.debug("commit(txId) -- ENTRY txId: " + transactionId);
		TransactionState state;
		try {
			state = getTransactionState(transactionId);
		} catch (UnknownTransactionException e) {
			LOG.fatal("Asked to commit unknown transaction: " + transactionId
					+ " in region "
					+ super.getRegionInfo().getRegionNameAsString());
			// TODO. Anything to handle here?
			throw e;
		}

		if (!state.getStatus().equals(Status.COMMIT_PENDING)) {
			LOG.fatal("Asked to commit a non pending transaction");
			// TODO. Anything to handle here?
			throw new IOException("commit failure");
		}
		LOG.debug("commit(txId) -- EXIT txId: " + transactionId);
		commit(state);
	}

	/**
	 * Abort the transaction.
	 * 
	 * @param transactionId
	 * @throws IOException
	 */
	public void abort(final long transactionId) throws IOException {
		LOG.debug("abort transactionId: " + transactionId + " " + super.getRegionInfo().getRegionNameAsString());

		// Not checking closing...
		TransactionState state;
		try {
			state = getTransactionState(transactionId);
		} catch (UnknownTransactionException e) {
			LOG.debug("Asked to abort unknown transaction [" + transactionId
					+ "] in region [" + getRegionInfo().getRegionNameAsString()
					+ "], ignoring");
			return;
		}

		state.setStatus(Status.ABORTED);

		if (state.hasWrite()) {
			this.transactionLog.writeAbortToLog(super.getRegionInfo(),
					state.getTransactionId(), super.getTableDesc());
		}

		// Following removes needed if we have voted
		if (state.getSequenceNumber() != null) {
			commitedTransactionsBySequenceNumber.remove(state
					.getSequenceNumber());
		}
		commitPendingTransactions.remove(state);

                if (state.isReinstated()) {
                    LOG.debug("Trafodion Recovery: abort reinstated indoubt transactions " + transactionId);
                    indoubtTransactionsById.remove(state.getTransactionId());
                    int tmid = (int) (transactionId >> 32);
                    int count = 0;
                    if (indoubtTransactionsCountByTmid.containsKey(tmid)) {
                        count =  (int) indoubtTransactionsCountByTmid.get(tmid) - 1;
                        if (count > 0) indoubtTransactionsCountByTmid.put(tmid, count);
                    }
                    // if all reinstated txns are resolved from a TM, remove it and delete associated zNode
                    if (count == 0) {
                        indoubtTransactionsCountByTmid.remove(tmid);
                        // delete zNode (tmid);
                        String lv_encoded = super.getRegionInfo().getEncodedName();
                        try {
                              LOG.debug("Trafodion Recovery: delete in abort recovery zNode TM " + tmid + " region encoded name " + lv_encoded + " for 0 in-doubt transaction");
                              TransactionalRegionServer.deleteRecoveryzNode(tmid, lv_encoded);
                        } catch (IOException e) {
                        LOG.error("Trafodion Recovery: delete recovery zNode failed");
                        }
                    }
                    if ((indoubtTransactionsById == null) || (indoubtTransactionsById.size() == 0)) {
                        // change region state to STARTED, and archive the split-thlog
                       if (indoubtTransactionsById == null) 
                          LOG.debug("Trafodion Recovery: start region in abort with indoubtTransactionsById null");
                       else
                          LOG.debug("Trafodion Recovery: start region in abort with indoubtTransactionsById size " + indoubtTransactionsById.size());
                       startRegionAfterRecovery();
                    }
                }
		retireTransaction(state);
	}

	private void commit(final TransactionState state) throws IOException {
		LOG.debug("Commiting transaction: " + state.toString() + " to "
				+ super.getRegionInfo().getRegionNameAsString());

                long transactionId = state.getTransactionId();
                if (state.isReinstated()) {
                        LOG.debug("Trafodion Recovery: commit reinstated indoubt transactions " + transactionId);
                        WALEdit b = indoubtTransactionsById.get(transactionId);
			LOG.debug("Writing " + b.size() + " updates for reinstated transaction " + transactionId);
			//WALEdit b = entry.getValue();
			for (KeyValue kv : b.getKeyValues()) {
                             synchronized (editReplay) {
                                  LOG.debug("Trafodion Recovery:   " + getRegionInfo().getRegionNameAsString() + " replay commit for transaction: "
                                                           + transactionId);
                                  LOG.debug("Trafodion Recovery:   " + getRegionInfo().getRegionNameAsString() + " replay commit for transaction: "
                                                              + transactionId + " with Op " + kv.getType());
		                if (kv.getType() == KeyValue.Type.Put.getCode()) {
				     Put put = new Put(kv.getRow());
                                     put.add(kv.getFamily(), kv.getQualifier(), kv.getTimestamp(), kv.getValue());
				     //put.add(kv);
				     super.put(put);
				} else if (kv.isDelete()) {
					Delete del = new Delete(kv.getRow());
					if (kv.isDeleteFamily()) {
						del.deleteFamily(kv.getFamily());
					} else if (kv.isDeleteType()) {
						del.deleteColumn(kv.getFamily(), kv.getQualifier());
					}
                                        super.delete(del, false);
				}
                           } // synchronized reply edits
         		}
                }  // reinstated transactions
                else {
		// Perform write operations timestamped to right now
                // maybe we can turn off WAL here for HLOG since THLOG has contained required edits in phase 1
		List<WriteAction> writeOrdering = state.getWriteOrdering();
		for (WriteAction action : writeOrdering) {
			Put put = action.getPut();
			if (null != put) {
				this.put(put, this.doWALHlog);
			}

			Delete delete = action.getDelete();
			if (null != delete) {
				delete(delete, null, this.doWALHlog);
			}
		}
                
                } // normal transactions

		// Now the transactional writes live in the core WAL, we can write a commit to the log
		// so we don't have to recover it from the transactional WAL.
		if (state.hasWrite() || state.isReinstated()) {
			this.transactionLog.writeCommitToLog(super.getRegionInfo(),
					state.getTransactionId(), super.getTableDesc());
		}

		state.setStatus(Status.COMMITED);
		if (state.hasWrite() || state.isReinstated()) {
                        if (!commitPendingTransactions.remove(state)) {
			   LOG.fatal("Commiting a non-query transaction that is not in commitPendingTransactions");
			   // Something has gone really wrong.
			   throw new IOException("commit failure");
                        }
		}
               LOG.trace("commit(tstate) -- EXIT TransactionState: " + state.toString());
                if (state.isReinstated()) {
                    indoubtTransactionsById.remove(state.getTransactionId());
                    int tmid = (int) (transactionId >> 32);
                    int count = 0;
                    if (indoubtTransactionsCountByTmid.containsKey(tmid)) {
                        count =  (int) indoubtTransactionsCountByTmid.get(tmid) - 1;
                        if (count > 0) indoubtTransactionsCountByTmid.put(tmid, count);
                    }
                    // if all reinstated txns are resolved from a TM, remove it and delete associated zNode
                    if (count == 0) {
                        indoubtTransactionsCountByTmid.remove(tmid);
                        // delete zNode (tmid, encoded name);
                        String lv_encoded = super.getRegionInfo().getEncodedName();
                        try {
                              LOG.debug("Trafodion Recovery: delete in commit recovery zNode TM " + tmid + " region encoded name " + lv_encoded + " for 0 in-doubt transaction");
                              TransactionalRegionServer.deleteRecoveryzNode(tmid, lv_encoded);
                        } catch (IOException e) {
                        LOG.error("Trafodion Recovery: delete recovery zNode failed");
                        }
                    }
                    if ((indoubtTransactionsById == null) || (indoubtTransactionsById.size() == 0)) {
                        // change region state to STARTED, and archive the split-thlog
                       if (indoubtTransactionsById == null) 
                          LOG.debug("Trafodion Recovery: start region in commit with indoubtTransactionsById null");
                       else
                          LOG.debug("Trafodion Recovery: start region in commit with indoubtTransactionsById size " + indoubtTransactionsById.size());
                       startRegionAfterRecovery();
                    }
                }
		retireTransaction(state);
	}

	@Override
	public List<StoreFile> close(final boolean abort) throws IOException {
		prepareToClose();
		if (!commitPendingTransactions.isEmpty()) {
			LOG.warn("Closing transactional region ["
					+ getRegionInfo().getRegionNameAsString()
					+ "], but still have [" + commitPendingTransactions.size()
					+ "] transactions  that are pending commit.");
			// TODO resolve from the Global Trx Log.
		}
		return super.close(abort);
	}

	@Override
	protected void prepareToSplit() {
		if (closing) {
			return;
		}

		LOG.debug("Preparing to split region "
				+ getRegionInfo().getRegionNameAsString());

                if(splitDelayEnabled) {
			while (!commitPendingTransactions.isEmpty() || !transactionsById.isEmpty()) {
				LOG.debug("Preparing to close transactional region ["
						+ getRegionInfo().getRegionNameAsString()
						+ "], but still have [" + commitPendingTransactions.size()
						+ "] transactions that are pending commit. And [ " 
						+ transactionsById.size() + "] active transactions. Sleeping");
				for (TransactionState s : commitPendingTransactions) {
					LOG.debug("commit pending: " + s.toString());
				}
				try {
					Thread.sleep(CLOSE_WAIT_ON_COMMIT_PENDING);
				} catch (InterruptedException e) {
					throw new RuntimeException(e);
				}
	
			}
		}
		else 
		prepareToClose();
	}

	boolean closing = false;
	private static final int CLOSE_WAIT_ON_COMMIT_PENDING = 1000;

	/**
	 * Get ready to close.
	 */
	void prepareToClose() {
		if (closing) {
			return;
		}

		LOG.debug("Preparing to close region "
				+ getRegionInfo().getRegionNameAsString());
		closing = true;

		while (!commitPendingTransactions.isEmpty()) {
			LOG.debug("Preparing to close transactional region ["
					+ getRegionInfo().getRegionNameAsString()
					+ "], but still have [" + commitPendingTransactions.size()
					+ "] transactions that are pending commit. Sleeping");
					 
			for (TransactionState s : commitPendingTransactions) {
				LOG.debug("commit pending: " + s.toString());
			}
			try {
				Thread.sleep(CLOSE_WAIT_ON_COMMIT_PENDING);
			} catch (InterruptedException e) {
				throw new RuntimeException(e);
			}

		}
	}

	private void checkClosing(final long transactionId) throws IOException {
		if (closing) {
		    LOG.error("Trafodion Recovery: checkClosing(" + transactionId + ") - raising exception. no more transaction allowed.");
		    throw new IOException("closing region, no more transaction allowed");
		}
	}

	// Cancel leases, and removed from lease lookup. This transaction may still
	// live in commitedTransactionsBySequenceNumber and
	// commitPendingTransactions
	private void retireTransaction(final TransactionState state) {
		long key = state.getTransactionId();
		try {
			transactionLeases.cancelLease(getLeaseId(state.getTransactionId()));
		} catch (LeaseException e) {
			// Ignore
		}
                // Clearing transaction conflict check list in case it is holding
                // a reference to a transaction state
                state.clearTransactionsToCheck();

		synchronized (transactionsById) {
			transactionsById.remove(key);
                        // Logging to catch error 97
                        LOG.debug("Removing transaction: " + key + " from list");
		}
	}

	protected TransactionState getTransactionState(final long transactionId)
			throws UnknownTransactionException {
		TransactionState state = null;

		state = transactionsById.get(transactionId);

		if (state == null) {
			LOG.debug("Unknown transaction: [" + transactionId + "], region: ["
					+ getRegionInfo().getRegionNameAsString() + "]");
			throw new UnknownTransactionException("transaction: [" + transactionId
					+ "], region: [" + getRegionInfo().getRegionNameAsString()
					+ "]");
		}

		/* commented out this try block		try {
			transactionLeases.renewLease(getLeaseId(transactionId));
		} catch (LeaseException e) {
			throw new RuntimeException(e);
		}
		*/

		return state;
	}

	private void maybeTriggerOldTransactionFlush() {
		if (commitedTransactionsBySequenceNumber.size() > oldTransactionFlushTrigger) {
			removeUnNeededCommitedTransactions();
		}
	}

	/**
	 * Cleanup references to committed transactions that are no longer needed.
	 */
	synchronized void removeUnNeededCommitedTransactions() {
	    LOG.trace("Enter removeUnNeededCommitedTransactions");
		Integer minStartSeqNumber = getMinStartSequenceNumber();
		if (minStartSeqNumber == null) {
			minStartSeqNumber = Integer.MAX_VALUE; // Remove all
		}

		int numRemoved = 0;
		// Copy list to avoid conc update exception
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

		if (LOG.isDebugEnabled()) {
			StringBuilder debugMessage = new StringBuilder();
			if (numRemoved > 0) {
				debugMessage.append("Removed [").append(numRemoved)
						.append("] commited transactions");

				if (minStartSeqNumber == Integer.MAX_VALUE) {
					debugMessage.append(" with any sequence number.");
				} else {
					debugMessage.append(" with sequence lower than [")
							.append(minStartSeqNumber).append("].");
				}
				if (!commitedTransactionsBySequenceNumber.isEmpty()) {
					debugMessage
							.append(" Still have [")
							.append(commitedTransactionsBySequenceNumber.size())
							.append("] left.");
				} else {
					debugMessage.append(" None left.");
				}
				LOG.debug(debugMessage.toString());
			} else if (commitedTransactionsBySequenceNumber.size() > 0) {
				debugMessage
						.append("Could not remove any transactions, and still have ")
						.append(commitedTransactionsBySequenceNumber.size())
						.append(" left");
				LOG.debug(debugMessage.toString());
			}
		}
	    LOG.trace("Exit removeUnNeededCommitedTransactions");
	}

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

	private void resolveTransactionFromLog(
			final TransactionState transactionState) throws IOException {
		LOG.error("Global transaction log is not Implemented. (Optimisticly) assuming transaction commit!");
		commit(transactionState);
		// throw new
		// RuntimeException("Global transaction log is not Implemented");
	}

	private static final int MAX_COMMIT_PENDING_WAITS = 10;

	private class TransactionLeaseListener implements LeaseListener {

		private final long transactionName;

		TransactionLeaseListener(final long n) {
			this.transactionName = n;
		}

		@Override
		public void leaseExpired() {
			LOG.debug("Transaction [" + this.transactionName
					+ "] expired in region ["
					+ getRegionInfo().getRegionNameAsString() + "]");
			TransactionState s = null;
			synchronized (transactionsById) {
				s = transactionsById.remove(transactionName);
                                // Logging to catch error 97
                                LOG.debug("Removing transaction: " + this.transactionName + " from list");
			}
			if (s == null) {
				LOG.warn("Unknown transaction expired " + this.transactionName);
				return;
			}

			switch (s.getStatus()) {
			case PENDING:
				s.setStatus(Status.ABORTED); // Other transactions may have a
												// ref
				break;
			case COMMIT_PENDING:
				LOG.debug("Transaction " + s.getTransactionId()
						+ " expired in COMMIT_PENDING state");

				try {
					if (s.getCommitPendingWaits() > MAX_COMMIT_PENDING_WAITS) {
						LOG.debug("Checking transaction status in transaction log");
						resolveTransactionFromLog(s);
						break;
					}
					LOG.debug("renewing lease and hoping for commit");
					s.incrementCommitPendingWaits();
					transactionsById.put(s.getTransactionId(), s);
                                        // Logging to catch error 97
                                        LOG.debug("Adding transaction: " + s.getTransactionId() + " to list");
					try {
						transactionLeases.createLease(
								getLeaseId(s.getTransactionId()), this);
					} catch (LeaseStillHeldException e) {
						transactionLeases.renewLease(getLeaseId(s
								.getTransactionId()));
					}
				} catch (IOException e) {
					throw new RuntimeException(e);
				}

				break;
			default:
				LOG.warn("Unexpected status on expired lease");
			}
		}
	}

	public void setTransactionLog(final THLog trxHLog) {
		this.transactionLog = trxHLog;
	}

	public void setTransactionalLeases(final Leases transactionalLeases) {
		this.transactionLeases = transactionalLeases;
	}
	
	public void checkRow(final byte [] row, String op) throws IOException {
	    if(!super.rowIsInRange(super.getRegionInfo(), row)) {
	        throw new WrongRegionException("Requested row out of range for " +
	            op + " on HRegion " + this + ", startKey='" +
	            Bytes.toStringBinary(super.getRegionInfo().getStartKey()) + "', getEndKey()='" +
	            Bytes.toStringBinary(super.getRegionInfo().getEndKey()) + "', row='" +
	            Bytes.toStringBinary(row) + "'");
	      }
	}
	public void prepareScanner(Scan scan) throws IOException {
		    if(!scan.hasFamilies()) {
		      // Adding all families to scanner
		      for(byte[] family: super.getTableDesc().getFamiliesKeys()){
		        scan.addFamily(family);
		      }
	   }
	}

	public void delete(long transactionId, Delete[] deletes) throws IOException {
		LOG.trace("Enter TransactionalRegion.deletes[], txid: " + transactionId);
		checkClosing(transactionId);
		TransactionState state = this.beginTransIfNotExist(transactionId);

		for (Delete del : deletes) {
			state.addDelete(del);
		}
		
	}

	public boolean checkAndPut(long transactionId, byte[] row, byte[] family,
			byte[] qualifier, byte[] value, Put put) throws IOException {
		LOG.trace("Enter TransactionalRegion.checkAndPut, txid: "
				+ transactionId);
		TransactionState state = this.beginTransIfNotExist(transactionId);
		Get get = new Get(row);
		get.addColumn(family, qualifier);
		Result rs = this.get(transactionId, get);
		if (rs.isEmpty() && value == null) {
			state.addWrite(put);
		} else if ((!rs.isEmpty()) && (value != null)
				&& (Bytes.equals(rs.getValue(family, qualifier), value))) {
			state.addWrite(put);
		} else {
			LOG.trace("Exit 1 TransactionalTable.checkAndPut row: " + row);
			return false;
		}
		LOG.trace("Exit 2 TransactionalTable.checkAndPut row: " + row);
		return true;
	}

	public boolean checkAndDelete(long transactionId, byte[] row, byte[] family,
			byte[] qualifier, byte[] value, Delete delete) throws IOException {
		LOG.trace("Enter TransactionalRegion.checkAndDelete, txid: "
				+ transactionId);
		TransactionState state = this.beginTransIfNotExist(transactionId);
		Get get = new Get(row);
		get.addColumn(family, qualifier);

		Result rs = this.get(transactionId, get);
		if (rs.isEmpty() && value == null) {
			state.addDelete(delete);
		} else if ((!rs.isEmpty()) && (value != null)
				&& (Bytes.equals(rs.getValue(family, qualifier), value))) {
			state.addDelete(delete);
		} else {
			LOG.trace("Exit 1 TransactionalTable.checkAndDelete row: " + row);
			return false;
		}
		LOG.trace("Exit 2 TransactionalTable.checkAndDelete row: " + row);
		return true;
	}
	
	/**begin transaction if not yet
	 * @param transactionId
	 * @return true: begin; false: not necessary to begin
	 * @throws IOException 
	 */
	private TransactionState beginTransIfNotExist(long transactionId) throws IOException{
		LOG.trace("Enter TransactionalRegion.beginTransIfNotExist, txid: "
			  + transactionId + " transactionsById size: " + transactionsById.size());
		TransactionState state = null;

		state = transactionsById.get(transactionId);

		if (state == null) {
			LOG.trace("Begin transaction in beginTransIfNotExist");
			this.beginTransaction(transactionId);
		}
		return transactionsById.get(transactionId);
	}

        public boolean isMoveable() {
                LOG.trace("isMoveable -- ENTRY");
                if(!commitPendingTransactions.isEmpty() || !transactionsById.isEmpty()) {
                        LOG.trace("Unable to balance transactional region ["
                                        + getRegionInfo().getRegionNameAsString()
                                        + "], still have [" + commitPendingTransactions.size()
                                        + "] transactions that are pending commit. And [ "
                                        + transactionsById.size() + "] active transactions.");
                        LOG.trace("isMoveable -- EXIT -- returning false");
                        return false;
                }
                LOG.trace("isMoveable -- EXIT -- returning true");
                return true;
        }

    /**
    * Request for collection of active pending transactions
    * @return list of active pending transactions
    * @throws IOException
    */
    public List<Long> doPendingTransRequest() throws IOException {
        List<Long> pendingTrans = new ArrayList<Long>();
        Long key;

        for(Map.Entry<Long, TransactionState> entry : transactionsById.entrySet()){
             key = entry.getKey();
             pendingTrans.add(key);
        }
        return  pendingTrans;
    }

    /**
    * Request for collection of committed transactions by sequence number
    * @return list of committed transactions
    * @throws IOException
    */
    public List<Long> doCommittedTransRequest() throws IOException {
        List<Long> committedTrans = new ArrayList<Long>();
        TransactionState tstate;
        Long transid;

        for(Map.Entry<Integer, TransactionState> entry : commitedTransactionsBySequenceNumber.entrySet()){
            tstate = entry.getValue();
            transid = tstate.getTransactionId();
            committedTrans.add(transid);
        }
        return committedTrans;
    }

    /**
    * Request for collection of in doubt transactions
    * @return list of indoubt transactions
    * @throws IOException
    */
    public List<Long> doInDoubtTransRequest() {
        List<Long> inDoubtTrans = new ArrayList<Long>();
        Long key;

        if(indoubtTransactionsById != null){
            for(Entry<Long, WALEdit> entry : indoubtTransactionsById.entrySet()){
                key = entry.getKey();
                inDoubtTrans.add(key);
            }
        }
        return  inDoubtTrans;
    }
}
