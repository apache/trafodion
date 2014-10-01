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
import org.apache.hadoop.hbase.regionserver.InternalScanner;
import org.apache.hadoop.hbase.regionserver.KeyValueScanner;
import org.apache.hadoop.hbase.regionserver.ScanQueryMatcher;
import org.apache.hadoop.hbase.regionserver.ScanType;
import org.apache.hadoop.hbase.regionserver.ScanInfo;
import org.apache.hadoop.hbase.regionserver.wal.WALEdit;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.EnvironmentEdgeManager;

/**
 * Holds the state of a transaction. This includes a buffer of all writes, a record of all reads / scans, and
 * information about which other transactions we need to check against.
 */
public class TransactionState {

    private static final Log LOG = LogFactory.getLog(TransactionState.class);

    /** Current status. */
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

    /**
     * Simple container of the range of the scanners we've opened. Used to check for conflicting writes.
     */
    private static class ScanRange {

        protected byte[] startRow;
        protected byte[] endRow;

        public ScanRange(final byte[] startRow, final byte[] endRow) {
            this.startRow = startRow == HConstants.EMPTY_START_ROW ? null : startRow;
            this.endRow = endRow == HConstants.EMPTY_END_ROW ? null : endRow;
        }

        /**
         * Check if this scan range contains the given key.
         * 
         * @param rowKey
         * @return boolean
         */
        public boolean contains(final byte[] rowKey) {
            if (startRow != null && Bytes.compareTo(rowKey, startRow) < 0) {
                return false;
            }
            if (endRow != null && Bytes.compareTo(endRow, rowKey) < 0) {
                return false;
            }
            return true;
        }

        @Override
        public String toString() {
            return "startRow: " + (startRow == null ? "null" : Bytes.toString(startRow)) + ", endRow: "
                    + (endRow == null ? "null" : Bytes.toString(endRow));
        }
    }

    private final HRegionInfo regionInfo;
    private final long hLogStartSequenceId;
    private final long transactionId;
    public Status status;
    private List<ScanRange> scans = Collections.synchronizedList(new LinkedList<ScanRange>());
    private List<Delete> deletes = Collections.synchronizedList(new LinkedList<Delete>());
    private List<WriteAction> writeOrdering = Collections.synchronizedList(new LinkedList<WriteAction>());
    private Set<TransactionState> transactionsToCheck = Collections.synchronizedSet(new HashSet<TransactionState>());
    private int startSequenceNumber;
    private Integer sequenceNumber;
    private int commitPendingWaits = 0;
    private HTableDescriptor tabledescriptor;
    private int reInstated = 0;
    private WALEdit e;
    private int transactionEditsLen;
    private List<Tag> tagList = Collections.synchronizedList(new ArrayList<Tag>());
    //private List<Tag> tagList = new ArrayList<Tag>();

    public TransactionState(final long transactionId, final long rLogStartSequenceId, final HRegionInfo regionInfo, HTableDescriptor htd) {
        LOG.trace("Trafodion Recovery: create TS object for " + transactionId);
        this.transactionId = transactionId;
        this.hLogStartSequenceId = rLogStartSequenceId;
        this.regionInfo = regionInfo;
        this.status = Status.PENDING;
        this.tabledescriptor = htd;
        this.e = new WALEdit();
        this.transactionEditsLen = 0;
        Tag transactionalTag = this.formTransactionalContextTag(1);
        tagList.add(transactionalTag );
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
        byte tagType = 41;
        Tag tag = new Tag(tagType, tagBytes);
        return tag;
    }    

    public synchronized void addRead(final byte[] rowKey) {
        scans.add(new ScanRange(rowKey, rowKey));
    }

    public synchronized void addWrite(final Put write) {
        LOG.trace("addWrite -- ENTRY: write: " + write.toString());
        WriteAction waction;
        updateLatestTimestamp(write.getFamilyCellMap().values(), EnvironmentEdgeManager.currentTimeMillis());
        // Adding read scan on a write action
	addRead(new WriteAction(write).getRow());
        LOG.trace("writeOrdering size before: " + writeOrdering.size());
        writeOrdering.add(waction = new WriteAction(write));
        LOG.trace("writeOrdering size after: " + writeOrdering.size());
         for (Cell value : waction.getCells()) {
             //KeyValue kv = KeyValueUtil.ensureKeyValue(value);
             //LOG.debug("add tag into edit for put " + this.transactionId);
             KeyValue kv = KeyValue.cloneAndAddTags(value, tagList);

             /* // SST trace print
             LOG.debug("PUT11 KV info length " + kv.getLength() + " " + kv.getKeyLength() + " " + kv.getValueLength() + " " + kv.getTagsLength()); 
             LOG.debug("PUT22 tag " + Hex.encodeHexString( kv.getBuffer()));
             byte[] tagArray = Bytes.copy(kv.getTagsArray(), kv.getTagsOffset(), kv.getTagsLength());
             LOG.debug("PUT33 tag " + Hex.encodeHexString(tagArray));
             byte tagType = 41;
             Tag tag = Tag.getTag(tagArray, 0, kv.getTagsLength(), tagType); //TagType.TRANSACTION_TAG_TYPE
             byte[] b = tag.getBuffer();
             int offset = Tag.TYPE_LENGTH_SIZE + Tag.TAG_LENGTH_SIZE;
             int version = Bytes.toInt(b,offset);
             int op = Bytes.toInt(b,Bytes.SIZEOF_INT+offset);
             long tid = Bytes.toLong(b,Bytes.SIZEOF_INT+Bytes.SIZEOF_INT+offset);
             long logSeqId = Bytes.toLong(b,Bytes.SIZEOF_INT+Bytes.SIZEOF_INT+Bytes.SIZEOF_LONG+offset);
             LOG.debug("PUT44 Find transactional tag within Edits for tid " + tid + " op " + op + " log seq " + logSeqId + " version " + version);
             */

             transactionEditsLen = transactionEditsLen + kv.getLength();
             e.add(kv);
         }

       //  Here, we just append the ACTIVE edit to HLOG in the no-wait form (actively logging) rathen than waiting until phase 1
       //  long txid = this.tHLog.appendNoSync(this.regionInfo, this.regionInfo.getTable(),
       //           state.getEdit(), new ArrayList<UUID>(), EnvironmentEdgeManager.currentTimeMillis(), this.m_Region.getTableDesc(),
       //           nextLogSequenceId, false, HConstants.NO_NONCE, HConstants.NO_NONCE);
       //  if (txid > largestLogSeqId) largestLogSeqId = txid; // save the log txid into TS object, later sync on largestSeqid during phase 1
    
       LOG.trace("addWrite -- EXIT");
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

    public boolean hasWrite() {
        return writeOrdering.size() > 0;
    }

    public synchronized void addDelete(final Delete delete) {
        WriteAction waction;
        long now = EnvironmentEdgeManager.currentTimeMillis();
        updateLatestTimestamp(delete.getFamilyCellMap().values(), now);
        if (delete.getTimeStamp() == HConstants.LATEST_TIMESTAMP) {
            delete.setTimestamp(now);
        }
        deletes.add(delete);
        writeOrdering.add(waction = new WriteAction(delete));

         for (Cell value : waction.getCells()) {
             //KeyValue kv = KeyValueUtil.ensureKeyValue(value);
             // LOG.debug("add tag into edit for put " + this.transactionId);
             KeyValue kv = KeyValue.cloneAndAddTags(value, tagList);

             /* // SST trace print
             LOG.debug("DEL11 KV info length " + kv.getLength() + " " + kv.getKeyLength() + " " + kv.getValueLength() + " " + kv.getTagsLength()); 
             LOG.debug("DEL22 tag " + Hex.encodeHexString( kv.getBuffer()));
             byte[] tagArray = Bytes.copy(kv.getTagsArray(), kv.getTagsOffset(), kv.getTagsLength());
             LOG.debug("DEL33 tag " + Hex.encodeHexString(tagArray));
             byte tagType = 41;
             Tag tag = Tag.getTag(tagArray, 0, kv.getTagsLength(), tagType); //*TagType.TRANSACTION_TAG_TYPE
             byte[] b = tag.getBuffer();
             int offset = Tag.TYPE_LENGTH_SIZE + Tag.TAG_LENGTH_SIZE;
             int version = Bytes.toInt(b,offset);
             int op = Bytes.toInt(b,Bytes.SIZEOF_INT+offset);
             long tid = Bytes.toLong(b,Bytes.SIZEOF_INT+Bytes.SIZEOF_INT+offset);
             long logSeqId = Bytes.toLong(b,Bytes.SIZEOF_INT+Bytes.SIZEOF_INT+Bytes.SIZEOF_LONG+offset);
             LOG.debug("DEL44 Find transactional tag within Edits for tid " + tid + " op " + op + " log seq " + logSeqId + " version " + version);
             */

             transactionEditsLen = transactionEditsLen + kv.getLength();
             e.add(kv);
         }
    }

    public synchronized void applyDeletes(final List<Cell> input, final long minTime, final long maxTime) {
        if (deletes.isEmpty()) {
            return;
        }
        for (Iterator<Cell> itr = input.iterator(); itr.hasNext();) {
            Cell included = applyDeletes(itr.next(), minTime, maxTime);
            if (null == included) {
                itr.remove();
            }
        }
    }

   public synchronized Cell applyDeletes(final Cell kv, final long minTime, final long maxTime) {
        if (deletes.isEmpty()) {
            return kv;
        }

        for (Delete delete : deletes) {
            // Skip if delete should not apply
            if (!Bytes.equals(kv.getRow(), delete.getRow()) || kv.getTimestamp() > delete.getTimeStamp()
                    || delete.getTimeStamp() > maxTime || delete.getTimeStamp() < minTime) {
                continue;
            }

            // Whole-row delete
            if (delete.isEmpty()) {
                return null;
            }

            for (Entry<byte[], List<Cell>> deleteEntry : delete.getFamilyCellMap().entrySet()) {
                byte[] family = deleteEntry.getKey();
                if (!Bytes.equals(kv.getFamilyArray(), family)) {
                    continue;
                }
                List<Cell> familyDeletes = deleteEntry.getValue();
                if (familyDeletes == null) {
                    return null;
                }
                for (Cell keyDeletes : familyDeletes) {
                    byte[] deleteQualifier = keyDeletes.getQualifierArray();
                    byte[] kvQualifier = kv.getQualifierArray();
                    if (keyDeletes.getTimestamp() > kv.getTimestamp() && Bytes.equals(deleteQualifier, kvQualifier)) {
                        return null;
                    }
                }
            }
        }

        return kv;
    }

    public void clearTransactionsToCheck() {
        transactionsToCheck.clear();
    }

    public void addTransactionToCheck(final TransactionState transaction) {
        transactionsToCheck.add(transaction);
    }

    public synchronized boolean hasConflict() {
        for (TransactionState transactionState : transactionsToCheck) {
            if (hasConflict(transactionState)) {
                return true;
            }
        }
        return false;
    }

    private boolean hasConflict(final TransactionState checkAgainst) {
        if (checkAgainst.getStatus().equals(TransactionState.Status.ABORTED)) {
            return false; // Cannot conflict with aborted transactions
        }

        for (WriteAction otherUpdate : checkAgainst.writeOrdering) {
            byte[] row = otherUpdate.getRow();
            if (this.scans != null && !this.scans.isEmpty()) {
              int size = this.scans.size();
              for (int i = 0; i < size; i++) {
                ScanRange scanRange = this.scans.get(i);
                if (scanRange == null)
                    LOG.trace("Transaction [" + this.toString() + "] scansRange is null");
                if (scanRange != null && scanRange.contains(row)) {
                    LOG.warn("Transaction [" + this.toString() + "] has scan which conflicts with ["
                            + checkAgainst.toString() + "]: region [" + regionInfo.getRegionNameAsString()
                            + "], scanRange[" + scanRange.toString() + "] ,row[" + Bytes.toString(row) + "]");
                    return true;
                }
            }
        }
        }
        return false;
    }

    /**
     * Get the status.
     * 
     * @return Return the status.
     */
    public Status getStatus() {
        return status;
    }

    public WALEdit getEdit() {
       return e;
    }

    public int getTransactionEditsLen() {
       return transactionEditsLen;
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
    public synchronized int getStartSequenceNumber() {
        return startSequenceNumber;
    }

    /**
     * Set the startSequenceNumber.
     * 
     * @param startSequenceNumber
     */
    public synchronized void setStartSequenceNumber(final int startSequenceNumber) {
        this.startSequenceNumber = startSequenceNumber;
    }

    /**
     * Get the sequenceNumber.
     * 
     * @return Return the sequenceNumber.
     */
    public synchronized Integer getSequenceNumber() {
        return sequenceNumber;
    }

    /**
     * Set the sequenceNumber.
     * 
     * @param sequenceNumber The sequenceNumber to set.
     */
    public synchronized void setSequenceNumber(final Integer sequenceNumber) {
        this.sequenceNumber = sequenceNumber;
    }

    @Override
    public String toString() {
        StringBuilder result = new StringBuilder();
        result.append("[transactionId: ");
        result.append(transactionId);
        result.append(" status: ");
        result.append(status.name());
        result.append(" scan Size: ");
        result.append(scans.size());
        result.append(" write Size: ");
        result.append(getWriteOrdering().size());
        result.append(" startSQ: ");
        result.append(startSequenceNumber);
        if (sequenceNumber != null) {
            result.append(" commitedSQ:");
            result.append(sequenceNumber);
        }
        result.append("]");

        return result.toString();
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

    public synchronized void addScan(final Scan scan) {
        ScanRange scanRange = new ScanRange(scan.getStartRow(), scan.getStopRow());
        LOG.trace(String.format("Adding scan for transaction [%s], from [%s] to [%s]", transactionId,
            scanRange.startRow == null ? "null" : Bytes.toString(scanRange.startRow), scanRange.endRow == null ? "null"
                    : Bytes.toString(scanRange.endRow)));
        scans.add(scanRange);
    }

    public int getCommitPendingWaits() {
        return commitPendingWaits;
    }

    public synchronized void incrementCommitPendingWaits() {
        this.commitPendingWaits++;
    }

    /**
     * Get deletes.
     * 
     * @return deletes
     */
    public synchronized List<Delete> getDeletes() {
        return deletes;
    }

    /**
     * Get a scanner to go through the puts and deletes from this transaction. Used to weave together the local trx puts
     * with the global state.
     * 
     * @return scanner
     */
    public KeyValueScanner getScanner(final Scan scan) {
        return new TransactionScanner(scan);
    }

    private synchronized Cell[] getAllCells(final Scan scan) {
        LOG.trace("getAllCells -- ENTRY");
        List<Cell> kvList = new ArrayList<Cell>();

        for (WriteAction action : writeOrdering) {
            byte[] row = action.getRow();
            List<Cell> kvs = action.getCells();

            if (scan.getStartRow() != null && !Bytes.equals(scan.getStartRow(), HConstants.EMPTY_START_ROW)
                    && Bytes.compareTo(row, scan.getStartRow()) < 0) {
                continue;
            }
            if (scan.getStopRow() != null && !Bytes.equals(scan.getStopRow(), HConstants.EMPTY_END_ROW)
                    && Bytes.compareTo(row, scan.getStopRow()) > 0) {
                continue;
            }

	    if (!scan.hasFamilies()) {
            kvList.addAll(kvs);
		continue;
	    }
      // Pick only the Cell's that match the 'scan' specifications
      for (Cell lv_kv : kvs) {
    byte[] lv_kv_family = lv_kv.getFamilyArray();
    Map<byte [], NavigableSet<byte []>> lv_familyMap = scan.getFamilyMap();
    NavigableSet<byte []> set = lv_familyMap.get(lv_kv_family);
    if (set == null || set.size() == 0) {
          kvList.add(lv_kv);
        continue;
    }
    if (set.contains(lv_kv.getQualifierArray())) {
        kvList.add(lv_kv);
    }
      }
        }

        LOG.trace("getAllCells -- EXIT kvList size = " + kvList.size());
        return kvList.toArray(new Cell[kvList.size()]);
    }
    
	  private KeyValue[] getAllKVs(final Scan scan) {
        LOG.trace("getAllKVs -- ENTRY");
        List<KeyValue> kvList = new ArrayList<KeyValue>();

        for (WriteAction action : writeOrdering) {
            byte[] row = action.getRow();
            List<KeyValue> kvs = action.getKeyValues();

            if (scan.getStartRow() != null && !Bytes.equals(scan.getStartRow(), HConstants.EMPTY_START_ROW)
                    && Bytes.compareTo(row, scan.getStartRow()) < 0) {
                continue;
            }
            if (scan.getStopRow() != null && !Bytes.equals(scan.getStopRow(), HConstants.EMPTY_END_ROW)
                    && Bytes.compareTo(row, scan.getStopRow()) > 0) {
                continue;
            }

      if (!scan.hasFamilies()) {
            kvList.addAll(kvs);
    continue;
      }

	    // Pick only the Cell's that match the 'scan' specifications
	Map<byte [], NavigableSet<byte []>> lv_familyMap = scan.getFamilyMap();
	    for (KeyValue lv_kv : kvs) {
		byte[] lv_kv_family = lv_kv.getFamily();
		NavigableSet<byte []> set = lv_familyMap.get(lv_kv_family);
		if (set == null || set.size() == 0) {
					kvList.add(lv_kv);
		    continue;
		}
		if (set.contains(lv_kv.getQualifier())) {
		    kvList.add(lv_kv);
		}
	    }
        }

        LOG.trace("getAllKVs -- EXIT kvList size = " + kvList.size());
        return kvList.toArray(new KeyValue[kvList.size()]);
    }

    private synchronized int getTransactionSequenceIndex(final Cell kv) {
        for (int i = 0; i < writeOrdering.size(); i++) {
            WriteAction action = writeOrdering.get(i);
            if (isKvInPut(kv, action.getPut())) {
                return i;
            }
            if (isKvInDelete(kv, action.getDelete())) {
                return i;
            }
        }
        throw new IllegalStateException("Can not find kv in transaction writes");
    }

    private synchronized boolean isKvInPut(final Cell kv, final Put put) {
        if (null != put) {
            for (List<Cell> putKVs : put.getFamilyCellMap().values()) {
                for (Cell putKV : putKVs) {
                    if (putKV == kv) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    private synchronized boolean isKvInDelete(final Cell kv, final Delete delete) {
        if (null != delete) {
            for (List<Cell> putKVs : delete.getFamilyCellMap().values()) {
                for (Cell deleteKv : putKVs) {
                    if (deleteKv == kv) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    /**
     * Scanner of the puts and deletes that occur during this transaction.
     * 
     * @author clint.morgan
     */
    public class TransactionScanner extends KeyValueListScanner implements InternalScanner {

        private ScanQueryMatcher matcher;

        TransactionScanner(final Scan scan) {
            super(new KeyValue.KVComparator() {            	
                @Override
                public int compare(final Cell left, final Cell right) {
                    int result = super.compare(left, right);
                    if (result != 0) {
                        return result;
                    }
                    if (left == right) {
                        return 0;
                    }
                    int put1Number = getTransactionSequenceIndex(left);
                    int put2Number = getTransactionSequenceIndex(right);
                    return put2Number - put1Number;
                }
            }, getAllKVs(scan));
           
            // We want transaction scanner to always take priority over store
            // scanners.
            super.setSequenceID(Long.MAX_VALUE);
            
            //Store.ScanInfo scaninfo = new Store.ScanInfo(null, 0, 1, HConstants.FOREVER, false, 0, Cell.COMPARATOR);
            ScanInfo scaninfo = new ScanInfo(null, 0, 1, HConstants.FOREVER, false, 0, KeyValue.COMPARATOR);
            
            try {
              matcher = new ScanQueryMatcher(scan,
            
                scaninfo,
                null,
                ScanType.USER_SCAN,
                Long.MAX_VALUE,
                HConstants.LATEST_TIMESTAMP,
					     0);
	      //null); # Not with HBase 0.98.1 
            }
            catch (Exception e) {
              LOG.error("error while instantiating the ScanQueryMatcher()" + e);
            }
         
        }

        /**
         * Get the next row of values from this transaction.
         * 
         * @param outResult
         * @param limit
         * @return true if there are more rows, false if scanner is done
         */
        @Override
        public synchronized boolean next(final List<Cell> outResult, final int limit) throws IOException {          	
            Cell peeked = this.peek();            
            if (peeked == null) {            
                close();
                return false;
            }
            
            matcher.setRow(peeked.getRowArray(), peeked.getRowOffset(), peeked.getRowLength());
            
            KeyValue kv;
            List<Cell> results = new ArrayList<Cell>();
            LOOP: while ((kv = this.peek()) != null) {
                ScanQueryMatcher.MatchCode qcode = matcher.match(kv);
                switch (qcode) {
                    case INCLUDE:
                        Cell next = this.next();
                        results.add(next);
                        if (limit > 0 && results.size() == limit) {
                            break LOOP;
                        }
                        continue;

                    case DONE:
                        // copy jazz
                        outResult.addAll(0, results);
                        return true;

                    case DONE_SCAN:
                        close();

                        // copy jazz
                        outResult.addAll(0, results);

                        return false;

                    case SEEK_NEXT_ROW:
                        this.next();
                        break;

                    case SEEK_NEXT_COL:
                        this.next();
                        break;

                    case SKIP:
                        this.next();
                        break;

                    default:
                        throw new RuntimeException("UNEXPECTED");
                }
            }

            if (!results.isEmpty()) {
                // copy jazz
                outResult.addAll(0, results);
                return true;
            }

            // No more keys
            close();
            return false;
        }

        @Override
        /* Commenting out for HBase 0.98
        public boolean next(final List<KeyValue> results) throws IOException {
            return next(results, -1);
        }
       // May need to use metric value
        @Override
        public boolean next(List<KeyValue> results, String metric) throws IOException{          
          return next(results, -1);
        }
        
        // May need to use metric value
        @Override
        public boolean next(List<Cell> results, int limit, String metric) throws IOException {
          
          return next(results,limit);
        }

       */
        
        public synchronized boolean next(final List<Cell> results) throws IOException {
          return next(results, -1);
        }
       
     }

    /**
     * Simple wrapper for Put and Delete since they don't have a common enough interface.
     */
    public class WriteAction {

        private Put put;
        private Delete delete;

        public WriteAction(final Put put) {
            if (null == put) {
                throw new IllegalArgumentException("WriteAction requires a Put or a Delete.");
            }
            this.put = put;
        }

        public WriteAction(final Delete delete) {
            if (null == delete) {
                throw new IllegalArgumentException("WriteAction requires a Put or a Delete.");
            }
            this.delete = delete;
        }

        public Put getPut() {
            return put;
        }

        public Delete getDelete() {
            return delete;
        }

        public synchronized byte[] getRow() {
            if (put != null) {
                return put.getRow();
            } else if (delete != null) {
                return delete.getRow();
            }
            throw new IllegalStateException("WriteAction is invalid");
        }

        synchronized List<Cell> getCells() {
            List<Cell> edits = new ArrayList<Cell>();
            Collection<List<Cell>> kvsList;

            if (put != null) {
                kvsList = put.getFamilyCellMap().values();
            } else if (delete != null) {
                if (delete.getFamilyCellMap().isEmpty()) {
                    // If whole-row delete then we need to expand for each
                    // family
                    kvsList = new ArrayList<List<Cell>>(1);
                    for (byte[] family : tabledescriptor.getFamiliesKeys()) {
                        Cell familyDelete = new KeyValue(delete.getRow(), family, null, delete.getTimeStamp(),
                                KeyValue.Type.DeleteFamily);
                        kvsList.add(Collections.singletonList(familyDelete));
                    }
                } else {
                    kvsList = delete.getFamilyCellMap().values();
                }
            } else {
                throw new IllegalStateException("WriteAction is invalid");
            }

            for (List<Cell> kvs : kvsList) {
                for (Cell kv : kvs) {
                    edits.add(kv);
                    //LOG.debug("Trafodion Recovery:   " + regionInfo.getRegionNameAsString() + " create edits for transaction: "
                    //               + transactionId + " with Op " + kv.getType());
                }
            }
            return edits;
        }
        
        synchronized List<KeyValue> getKeyValues() {
          List<KeyValue> edits = new ArrayList<KeyValue>();
          Collection<List<KeyValue>> kvsList = null;

          if (put != null) {
              if (!put.getFamilyMap().isEmpty()) {
              kvsList = put.getFamilyMap().values();
              }
          } else if (delete != null) {
              if (delete.getFamilyCellMap().isEmpty()) {
                  // If whole-row delete then we need to expand for each
                  // family
                  kvsList = new ArrayList<List<KeyValue>>(1);
                  for (byte[] family : tabledescriptor.getFamiliesKeys()) {
                    KeyValue familyDelete = new KeyValue(delete.getRow(), family, null, delete.getTimeStamp(),
                              KeyValue.Type.DeleteFamily);
                      kvsList.add(Collections.singletonList(familyDelete));
                  }
              } else {
                  kvsList = delete.getFamilyMap().values();
              }
          } else {
              throw new IllegalStateException("WriteAction is invalid");
          }

          if (kvsList != null) {
          for (List<KeyValue> kvs : kvsList) {
              for (KeyValue kv : kvs) {
                  edits.add(kv);
                  LOG.trace("Trafodion getKeyValues:   " + regionInfo.getRegionNameAsString() + " create edits for transaction: "
                                 + transactionId + " with Op " + kv.getType());
              }
              }
          }
          else
            LOG.trace("Trafodion getKeyValues:   " 
                 + regionInfo.getRegionNameAsString() + " kvsList was null");
          return edits;
      }
    }

    /**
     * Get the puts and deletes in transaction order.
     * 
     * @return Return the writeOrdering.
     */
    public List<WriteAction> getWriteOrdering() {
        return writeOrdering;
    }
}
