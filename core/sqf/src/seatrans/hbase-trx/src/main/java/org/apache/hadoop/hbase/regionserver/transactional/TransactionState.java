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

package org.apache.hadoop.hbase.regionserver.transactional;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.NavigableSet;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicLong;

import org.apache.commons.codec.binary.Hex;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hbase.Cell;
import org.apache.hadoop.hbase.Tag;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.KeyValueUtil;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.KeyValue.Type;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.regionserver.wal.HLog;
import org.apache.hadoop.hbase.regionserver.InternalScanner;
import org.apache.hadoop.hbase.regionserver.KeyValueScanner;
import org.apache.hadoop.hbase.regionserver.ScanQueryMatcher;
import org.apache.hadoop.hbase.regionserver.ScanType;
import org.apache.hadoop.hbase.regionserver.ScanInfo;
import org.apache.hadoop.hbase.regionserver.wal.WALEdit;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.EnvironmentEdgeManager;
import org.apache.hadoop.io.DataInputBuffer;

/**
 * Holds the state of a transaction. This includes a buffer of all writes, a record of all reads / scans, and
 * information about which other transactions we need to check against.
 */
public class TransactionState {

    protected static final Log LOG = LogFactory.getLog(TransactionState.class);

    /** Current commit progress */
    public enum CommitProgress {
        /** Initial status, still performing operations. */
        NONE,
        COMMITTING,
        COMMITED,
    }

    /** Current status */
    public enum Status {
        /** Initial status, still performing operations. */
        PENDING,
        /**
         * Checked if we can commit, and said yes. Still need to determine the global decision.
         */
        COMMIT_PENDING,
        /** Committed. */
        COMMITED,
        /** Aborted. */
        ABORTED
    }

    protected final HRegionInfo regionInfo;
    protected final long hLogStartSequenceId;
    protected final long transactionId;
    protected AtomicLong logSeqId; 
    public Status status;
    protected long startSequenceNumber;
    protected Long sequenceNumber;
    protected int commitPendingWaits = 0;
    protected HTableDescriptor tabledescriptor;
    protected long controlPointEpochAtPrepare = 0;
    protected int reInstated = 0;
    protected long flushTxId = 0;

    protected boolean earlyLogging = false;
    protected boolean commit_TS_CC = false;
    protected HLog tHLog = null;
    protected Object xaOperation = new Object();;
    protected CommitProgress commitProgress = CommitProgress.NONE; // 0 is no commit yet, 1 is a commit is under way, 2 is committed
    protected List<Tag> tagList = Collections.synchronizedList(new ArrayList<Tag>());

    public static final int TS_ACTIVE = 0;
    public static final int TS_COMMIT_REQUEST = 1;
    public static byte TS_TRAFODION_TXN_TAG_TYPE = 41;

    public TransactionState(final long transactionId, final long rLogStartSequenceId, AtomicLong hlogSeqId, final HRegionInfo regionInfo,
                                                 HTableDescriptor htd, HLog hLog, boolean logging) {
        Tag transactionalTag = null;
        if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery: create TS object for " + transactionId + " early logging " + logging);
        this.transactionId = transactionId;
        this.hLogStartSequenceId = rLogStartSequenceId;
        this.logSeqId = hlogSeqId;
        this.regionInfo = regionInfo;
        this.status = Status.PENDING;
        this.tabledescriptor = htd;
        this.earlyLogging = logging;
        this.tHLog = hLog;
        if (earlyLogging) {
           transactionalTag = this.formTransactionalContextTag(TS_ACTIVE);
        }
        else {
           transactionalTag = this.formTransactionalContextTag(TS_COMMIT_REQUEST);
        }
        tagList.add(transactionalTag);
    }

    public HTableDescriptor getTableDesc() {
        return this.tabledescriptor;
    }

    // concatenate several byte[]
    byte[] concat(byte[]...arrays) {
       // Determine the length of the result byte array
       int totalLength = 0;
       for (int i = 0; i < arrays.length; i++)  {
           totalLength += arrays[i].length;
       }

       // create the result array
       byte[] result = new byte[totalLength];

       // copy the source arrays into the result array
       int currentIndex = 0;
       for (int i = 0; i < arrays.length; i++)  {
           System.arraycopy(arrays[i], 0, result, currentIndex, arrays[i].length);
           currentIndex += arrays[i].length;
       }
       return result;
    }

    public Tag formTransactionalContextTag(int transactionalOp) {
        byte[] tid = Bytes.toBytes (this.transactionId);
        byte[] logSeqId = Bytes.toBytes(this.hLogStartSequenceId);
        byte[] type = Bytes.toBytes(transactionalOp);
        int vers = 1;
        byte[] version = Bytes.toBytes(vers);

        byte[] tagBytes = concat(version, type, tid, logSeqId);
        byte tagType = TS_TRAFODION_TXN_TAG_TYPE;
        Tag tag = new Tag(tagType, tagBytes);
        return tag;
    }    

   public  static void updateLatestTimestamp(final Collection<List<Cell>> kvsCollection, final long time) {
        byte[] timeBytes = Bytes.toBytes(time);
        // HAVE to manually set the KV timestamps
        for (List<Cell> kvs : kvsCollection) {
            for (Cell cell : kvs) {
              KeyValue kv = KeyValueUtil.ensureKeyValue(cell);
                if (kv.isLatestTimestamp()) {
                    kv.updateLatestStamp(timeBytes);
                }
            }
        }
    }

    /**
     * Get the status.
     * 
     * @return Return the status.
     */
    public Status getStatus() {
        return status;
    }

    public long getFlushTxId() {
       return flushTxId;
    }

    public boolean getEarlyLogging() {
       return earlyLogging;
    }

    public void setFullEditInCommit(boolean fullEdit) {
       this.commit_TS_CC = fullEdit;
    }

    public boolean getFullEditInCommit() {
       return this.commit_TS_CC;
    }

    public Object getXaOperationObject() {
       return xaOperation;
    }

    /**
     * Get the CP epoch at Prepare.
     * 
     * @return Return the status.
     */
    public long getCPEpoch() {
        return controlPointEpochAtPrepare;
    }

    public void setCPEpoch(long epoch) {
        controlPointEpochAtPrepare = epoch;
    }

    public CommitProgress getCommitProgress() {
        return commitProgress;
    }

    public void setCommitProgress(final CommitProgress progress) {
        this.commitProgress = progress;
    }

    /**
     * Set the status.
     * 
     * @param status The status to set.
     */
    public synchronized void setStatus(final Status status) {
        this.status = status;
    }

     public Boolean isReinstated() {
        if (reInstated == 0) return false;
        return true;
    }

    public synchronized void setReinstated() {
        this.reInstated = 1;
    }

    /**
     * Get the startSequenceNumber.
     * 
     * @return Return the startSequenceNumber.
     */
    public synchronized long getStartSequenceNumber() {
        return startSequenceNumber;
    }

    /**
     * Set the startSequenceNumber.
     * 
     * @param startSequenceNumber
     */
    public synchronized void setStartSequenceNumber(final long startSequenceNumber) {
        this.startSequenceNumber = startSequenceNumber;
    }

    /**
     * Get the sequenceNumber.
     * 
     * @return Return the sequenceNumber.
     */
    public synchronized Long getSequenceNumber() {
        return sequenceNumber;
    }

    /**
     * Set the sequenceNumber.
     * 
     * @param sequenceNumber The sequenceNumber to set.
     */
    public synchronized void setSequenceNumber(final Long sequenceNumber) {
        this.sequenceNumber = sequenceNumber;
    }
    /**
     * Get the transactionId.
     * 
     * @return Return the transactionId.
     */
    public long getTransactionId() {
        return transactionId;
    }

    /**
     * Get the startSequenceId.
     * 
     * @return Return the startSequenceId.
     */
    public long getHLogStartSequenceId() {
        return hLogStartSequenceId;
    }

    public int getCommitPendingWaits() {
        return commitPendingWaits;
    }

    public synchronized void incrementCommitPendingWaits() {
        this.commitPendingWaits++;
    }

}
