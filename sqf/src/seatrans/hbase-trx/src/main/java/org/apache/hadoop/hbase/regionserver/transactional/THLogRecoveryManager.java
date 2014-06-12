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

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.Map;
import java.util.Map.Entry;
import java.util.SortedMap;
import java.util.TreeMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.transactional.HBaseBackedTransactionLogger;
import org.apache.hadoop.hbase.client.transactional.TransactionLogger;
import org.apache.hadoop.hbase.regionserver.wal.HLog;
import org.apache.hadoop.hbase.regionserver.wal.HLog.Reader;
import org.apache.hadoop.hbase.regionserver.wal.WALEdit;
import org.apache.hadoop.hbase.util.CancelableProgressable;

/**
 * Responsible recovering transactional information from the HLog.
 */
class THLogRecoveryManager {

    private static final Log LOG = LogFactory.getLog(THLogRecoveryManager.class);

    private final TransactionalRegion tregion;
    private final FileSystem fileSystem;
    private final HRegionInfo regionInfo;
    private final Configuration conf;

    /**
     * @param region
     */
    public THLogRecoveryManager(final TransactionalRegion region) {
    	LOG.trace("THLogRecoveryManager Construct");
        this.tregion = region;
        this.fileSystem = region.getFilesystem();
        this.regionInfo = region.getRegionInfo();
        this.conf = region.getConf();
    }

    // For Testing
    THLogRecoveryManager(final TransactionalRegion region, final FileSystem fileSystem, final HRegionInfo regionInfo, final Configuration conf) {
    	LOG.trace("THLogRecoveryManager Construct");
        this.tregion = region;
        this.fileSystem = fileSystem;
        this.regionInfo = regionInfo;
        this.conf = conf;
    }

    /**
     * Go through the WAL, and look for transactions that were started, but never completed. If the transaction was
     * committed, then those edits will need to be applied.
     * 
     * @param reconstructionLog
     * @param minSeqID that needs to be examined
     * @param reporter
     * @return map of transaction that should be commited
     * @throws UnsupportedEncodingException
     * @throws IOException
     */
    public Map<Long, WALEdit> getCommitsFromLog(final Path reconstructionLog, final long minSeqID,
            final CancelableProgressable reporter) throws UnsupportedEncodingException, IOException {

    	LOG.trace("getCommitsFromLog -- ENTRY" + reconstructionLog);
    	if (reconstructionLog == null || !fileSystem.exists(reconstructionLog)) {
            // Nothing to do.
                LOG.debug("Trafodion Recovery: passed reconstruction log " + reconstructionLog + " does not exist");
    		LOG.trace("getCommitsFromLog -- EXIT");
            return null;
        }
        // Check its not empty.
        FileStatus[] stats = fileSystem.listStatus(reconstructionLog);
        if (stats == null || stats.length == 0) {
            LOG.warn("Trafodion Recovery: passed reconstruction log " + reconstructionLog + " is zero-length");
            LOG.trace("getCommitsFromLog -- EXIT");
            return null;
        }

        // the map contains all the in-doubt transaction from edits replay, needs to be cleared or entry-deleted later
        SortedMap<Long, WALEdit> pendingTransactionsById = new TreeMap<Long, WALEdit>();

        //
        //   * This constructor allows a specific HLogKey implementation to override that
        //   * which would otherwise be chosen via configuration property.
        //   *
        //   * @param keyClass
        //   */
        //public SequenceFileLogReader(Class<? extends HLogKey> keyClass) {
        //this.keyClass = keyClass;
        //  }
        //

        LOG.debug("Trafodion Recovery: " + regionInfo.getRegionNameAsString() + " Setup THLOG reader");
        Reader logReader = THLog.getReader(fileSystem, reconstructionLog, conf);

        try {
            long skippedEdits = 0;
            long totalEdits = 0;
            long commitRequestCount = 0;
            long abortCount = 0;
            long commitCount = 0;
            long flushCount = 0;
            // How many edits to apply before we send a progress report.
            int reportInterval = conf.getInt("hbase.hstore.report.interval.edits", 2000);
            HLog.Entry entry;
            // TBD: Need to add an exception handler around logReader.next.
            //
            // A transaction now appears as a single edit. If logReader.next()
            // returns an exception, then it must be a incomplete/partial
            // transaction at the end of the file. Rather than bubble up
            // the exception, we should catch it and simply ignore the
            // partial transaction during this recovery phase.
            //

           LOG.debug("Trafodion Recovery:  " + regionInfo.getRegionNameAsString() + " Start processing THLOG edits with minSeqId " + minSeqID + " from path " + reconstructionLog);

            while ((entry = logReader.next()) != null) {
                THLogKey key = (THLogKey) entry.getKey();
                WALEdit val = entry.getEdit();
                if (LOG.isTraceEnabled())
                    switch (key.getTrxOp()) {
                       case FORCED_FLUSH_FILLER:
                           break;
                       default:
                    LOG.trace("Processing edit: key: " + key.toString() + " val: " + val.toString());
                }

               //LOG.debug("RRR read edits " + key + " - " + key.getLogSeqNum());

                // "-1" is used for all commit/abort/commitRequest cases
                // therefore any non-negative minSeqId could be used to retrieve all necessary WALEdits from trx-log
                // optimization may be needed later. Need to handle this situation to avoid failure during recovery
                //if (key.getLogSeqNum() < minSeqID) {
                //    skippedEdits++;
                //    continue;
                //}
                
                //if (key.getLogSeqNum() < minSeqID) {
                //    LOG.debug("Trafodion Recovery:  " + regionInfo.getRegionNameAsString() + " process edits " + key + " - " + key.getTransactionId() + " - " + key.getTrxOp() + " with LogSeqNum " + key.getLogSeqNum()
                //                           + " < " + minSeqID + " skip edits");
                //    skippedEdits++;
                //    continue;
                //}

                LOG.debug("Trafodion Recovery:   " + regionInfo.getRegionNameAsString() + " process edits " + key + " - " + key.getTransactionId() + " - " + key.getTrxOp() + " with LogSeqNum " + key.getLogSeqNum()
                                       + " and " + minSeqID + " process edits");
                long transactionId = key.getTransactionId();
                switch (key.getTrxOp()) {

                    case COMMIT_REQUEST:
                        if (pendingTransactionsById.containsKey(transactionId)) {
                            LOG.error("Trafodion Recovery:   " + regionInfo.getRegionNameAsString() + " processing commit request for transaction: " + transactionId
                                    + ", already have a pending trx with that id");
                            //throw new IOException("Corrupted transaction log");
                        }
                        else {
                        pendingTransactionsById.put(transactionId, val);
                        commitRequestCount++;
                         LOG.debug("Trafodion Recovery:   " + regionInfo.getRegionNameAsString() + " find in-doubt transaction " + transactionId + " in prepared state");
                        }
                        break;

                    case ABORT:
                        if (!pendingTransactionsById.containsKey(transactionId)) {
                            LOG.error("Trafodion Recovery:   " + regionInfo.getRegionNameAsString() + " processing abort for transaction: " + transactionId
                                    + ", but don't have  a pending trx with that id");
                            abortCount++;
                            break;
                            //throw new IOException("Corrupted transaction log");
                        }
                        pendingTransactionsById.remove(transactionId);
                        abortCount++;
                        break;

                    case COMMIT:
                        if (!pendingTransactionsById.containsKey(transactionId)) {
                            LOG.error("Trafodion Recovery:   " + regionInfo.getRegionNameAsString() + " processing commit for transaction: " + transactionId
                                    + ", but don't have  a pending trx with that id");
                            commitCount++;
                            break;
                            //throw new IOException("Corrupted transaction log");
                        }
                        WALEdit b = pendingTransactionsById.get(transactionId);
                        tregion.replayCommittedTransaction(transactionId, b);
                        pendingTransactionsById.remove(transactionId);
                        commitCount++;
                        break;

                   case FORCED_FLUSH_FILLER:
                        LOG.trace("Log Entry FORCED_FLUSH_FILLER, ignored ");
                        flushCount++;
                        break;

                    default:
                        throw new IllegalStateException("Trafodion Recovery: Unexpected log entry type detected in audit replay");
                }
                totalEdits++;

                if (reporter != null && (totalEdits % reportInterval) == 0) {
                    reporter.progress();
                }
            }
            if (LOG.isDebugEnabled()) {
                LOG.debug("Trafodion Recovery:   " + regionInfo.getRegionNameAsString() + " Edits replay read " + totalEdits + " transactional operations (skipped " + skippedEdits
                        + " because sequence id <= " + minSeqID + "): " + commitRequestCount + " commitRequests, "
                        + abortCount + " aborts, " + commitCount + " commits, and " + flushCount + " flushes.");
            }
        } finally {
            logReader.close();
        }

        LOG.debug("Trafodion Recovery:   " + regionInfo.getRegionNameAsString() + " region recovered edits path " + reconstructionLog + " has " + pendingTransactionsById.size() +
                             " in-doubt transactions after replying edits");

        LOG.trace("getCommitsFromLog -- EXIT");

        if (pendingTransactionsById.size() > 0)
            return pendingTransactionsById;

        return null;
    }

    private SortedMap<Long, WALEdit> resolvePendingTransaction(final SortedMap<Long, WALEdit> pendingTransactionsById)
            throws IOException {
    	LOG.trace("resolvePendingTransaction -- ENTRY");
        SortedMap<Long, WALEdit> commitedTransactionsById = new TreeMap<Long, WALEdit>();

        LOG.debug("Region log has " + pendingTransactionsById.size()
                + " unfinished transactions. Going to the transaction log to resolve");

        for (Entry<Long, WALEdit> entry : pendingTransactionsById.entrySet()) {
            TransactionLogger.TransactionStatus transactionStatus;
            transactionStatus = getGlobalTransactionLog().getStatusForTransaction(entry.getKey());

            if (transactionStatus == null) {
                throw new RuntimeException("Cannot resolve tranasction [" + entry.getKey() + "] from global tx log.");
            }
            switch (transactionStatus) {
                case ABORTED:
                    break;
                case COMMITTED:
                    commitedTransactionsById.put(entry.getKey(), entry.getValue());
                    break;
                case PENDING:
                    LOG.warn("Transaction [" + entry.getKey() + "] is still pending. Asumming it will not commit.");
                    // FIXME / REVIEW validate this behavior/assumption
                    // Can safely ignore this because if we don't see a commit or abort in the regions WAL, the we must
                    // not have been
                    // asked to vote yet.
                    break;
            }
        }
        LOG.trace("resolvePendingTransaction -- EXIT");
        return commitedTransactionsById;
    }

    private TransactionLogger globalTransactionLog = null;

    private synchronized TransactionLogger getGlobalTransactionLog() {
    	LOG.trace("getGlobalTransactionLog -- ENTRY");
        if (globalTransactionLog == null) {
            try {
                globalTransactionLog = new HBaseBackedTransactionLogger();
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
        LOG.trace("getGlobalTransactionLog -- EXIT");
        return globalTransactionLog;
    }

    /**
     * Set globalTransactionLog.
     * 
     * @param globalTransactionLog
     */
    public void setGlobalTransactionLog(final TransactionLogger globalTransactionLog) {
    	LOG.trace("setGlobalTransactionLog -- " + globalTransactionLog.toString());
        this.globalTransactionLog = globalTransactionLog;
    }

}
