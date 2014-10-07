// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2014 Hewlett-Packard Development Company, L.P.
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

package org.trafodion.dtm;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;

import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.transactional.TransactionManager;
import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.client.transactional.CommitUnsuccessfulException;
import org.apache.hadoop.hbase.client.transactional.UnknownTransactionException;
import org.apache.hadoop.hbase.client.transactional.HBaseBackedTransactionLogger;
import org.apache.hadoop.hbase.client.transactional.TransactionRegionLocation;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.LocalHBaseCluster;
import org.apache.hadoop.hbase.ServerName;
import org.apache.hadoop.hbase.TableExistsException;
import org.apache.hadoop.hbase.util.Bytes;

import org.apache.hadoop.hbase.regionserver.RegionSplitPolicy;

import java.util.Arrays;
import java.util.ArrayList;
import java.util.Collections;
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
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.RejectedExecutionException;

public class TmAuditTlog {

   static final Log LOG = LogFactory.getLog(TmAuditTlog.class);
   private static HBaseAdmin admin;
   private static Configuration config;
   private static String TLOG_TABLE_NAME;
   private static final byte[] TLOG_FAMILY = Bytes.toBytes("tf");
   private static final byte[] ASN_STATE = Bytes.toBytes("as");
   private static final byte[] QUAL_TX_STATE = Bytes.toBytes("tx");
   private static HTable[] table;
   private static HBaseAuditControlPoint tLogControlPoint;
   private static long tLogControlPointNum;
   private static long tLogHashKey;
   private static int  tLogHashShiftFactor;
   private static int dtmid;

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
   private static boolean distributedFS;
   private static boolean useAutoFlush;
   private static boolean ageCommitted;
   private static boolean forceControlPoint;
   private static boolean disableBlockCache;
 
   private static AtomicLong asn;  // Audit sequence number is the monotonic increasing value of the tLog write

   private static Object tlogAuditLock[];        // Lock for synchronizing access via regions.

   private static Object tablePutLock;            // Lock for synchronizing table.put operations
                                                  // to avoid ArrayIndexOutOfBoundsException
   private static byte filler[];

   public static final int TM_TX_STATE_NOTX = 0; //S0 - NOTX
   public static final int TM_TX_STATE_ACTIVE = 1; //S1 - ACTIVE
   public static final int TM_TX_STATE_FORGOTTEN = 2; //N/A
   public static final int TM_TX_STATE_COMMITTED = 3; //N/A
   public static final int TM_TX_STATE_ABORTING = 4; //S4 - ROLLBACK
   public static final int TM_TX_STATE_ABORTED = 5; //S4 - ROLLBACK
   public static final int TM_TX_STATE_COMMITTING = 6; //S3 - PREPARED
   public static final int TM_TX_STATE_PREPARING = 7; //S2 - IDLE
   public static final int TM_TX_STATE_FORGETTING = 8; //N/A
   public static final int TM_TX_STATE_PREPARED = 9; //S3 - PREPARED XARM Branches only!
   public static final int TM_TX_STATE_FORGETTING_HEUR = 10; //S5 - HEURISTIC
   public static final int TM_TX_STATE_BEGINNING = 11; //S1 - ACTIVE
   public static final int TM_TX_STATE_HUNGCOMMITTED = 12; //N/A
   public static final int TM_TX_STATE_HUNGABORTED = 13; //S4 - ROLLBACK
   public static final int TM_TX_STATE_IDLE = 14; //S2 - IDLE XARM Branches only!
   public static final int TM_TX_STATE_FORGOTTEN_HEUR = 15; //S5 - HEURISTIC - Waiting Superior TM xa_forget request
   public static final int TM_TX_STATE_ABORTING_PART2 = 16; // Internal State
   public static final int TM_TX_STATE_TERMINATING = 17;
   public static final int TM_TX_STATE_LAST = 17;

   private class AuditBuffer{
      private ArrayList<Put> buffer;           // Each Put is an audit record

      private AuditBuffer () {
         buffer = new  ArrayList<Put>();
         buffer.clear();

      }

      private void bufferAdd(Put localPut) throws Exception {
         long threadId = Thread.currentThread().getId();
         if (LOG.isTraceEnabled()) LOG.trace("BufferAdd start in thread " + threadId );
         try {
            buffer.add(localPut);
         }
         catch (Exception e) {
            if (LOG.isDebugEnabled()) LOG.debug("AuditBuffer Exception trying bufferAdd" + e);
            throw e;
         }
         if (LOG.isTraceEnabled()) LOG.trace("BufferAdd end in thread " + threadId );
      }

      private int bufferSize() throws Exception {
         int lvSize;
         long threadId = Thread.currentThread().getId();
         if (LOG.isTraceEnabled()) LOG.trace("BufferSize start in thread " + threadId );
         try {
            lvSize = buffer.size();
         }
         catch (Exception e) {
            if (LOG.isDebugEnabled()) LOG.debug("AuditBuffer Exception trying bufferSize" + e);
            throw e;
         }
         if (LOG.isTraceEnabled()) LOG.trace("AuditBuffer bufferSize end; returning " + lvSize + " in thread " 
                    +  Thread.currentThread().getId());
         return lvSize;
      }

      private void bufferClear() throws Exception {
         long threadId = Thread.currentThread().getId();
         if (LOG.isTraceEnabled()) LOG.trace("AuditBuffer bufferClear start in thread " + threadId);
         try {
            buffer.clear();
         }
         catch (Exception e) {
            if (LOG.isDebugEnabled()) LOG.debug("Exception trying bufferClear.clear" + e);
            throw e;
         }
         if (LOG.isTraceEnabled()) LOG.trace("AuditBuffer bufferClear end in thread " + threadId);
      }

      private ArrayList<Put> getBuffer() throws Exception {
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

   public TmAuditTlog (Configuration config) throws IOException, RuntimeException {

      this.config = config;
      this.dtmid = Integer.parseInt(config.get("dtmid"));
      if (LOG.isTraceEnabled()) LOG.trace("Enter TmAuditTlog constructor for dtmid " + dtmid);
      TLOG_TABLE_NAME = config.get("TLOG_TABLE_NAME");
      int fillerSize = 2;

      if (LocalHBaseCluster.isLocal(config)) {
         distributedFS = false;
      }
      else {
         distributedFS = true;
      }
      LOG.info("distributedFS is " + distributedFS);

      forceControlPoint = false;
      try {
         String controlPointFlush = System.getenv("TM_TLOG_FLUSH_CONTROL_POINT");
         if (controlPointFlush != null){
            forceControlPoint = (Integer.parseInt(controlPointFlush) != 0);
            if (LOG.isDebugEnabled()) LOG.debug("controlPointFlush != null");
         }
      }
      catch (Exception e) {
         if (LOG.isDebugEnabled()) LOG.debug("TM_TLOG_FLUSH_CONTROL_POINT is not in ms.env");
      }
      LOG.info("forceControlPoint is " + forceControlPoint);

      useAutoFlush = true;
      try {
         String autoFlush = System.getenv("TM_TLOG_AUTO_FLUSH");
         if (autoFlush != null){
            useAutoFlush = (Integer.parseInt(autoFlush) != 0);
            if (LOG.isDebugEnabled()) LOG.debug("autoFlush != null");
         }
      }
      catch (Exception e) {
         if (LOG.isDebugEnabled()) LOG.debug("TM_TLOG_AUTO_FLUSH is not in ms.env");
      }
      LOG.info("useAutoFlush is " + useAutoFlush);

      ageCommitted = false;
      try {
         String ageCommittedRecords = System.getenv("TM_TLOG_AGE_COMMITTED_RECORDS");
         if (ageCommittedRecords != null){
            ageCommitted = (Integer.parseInt(ageCommittedRecords) != 0);
            if (LOG.isDebugEnabled()) LOG.debug("ageCommittedRecords != null");
         }
      }
      catch (Exception e) {
         if (LOG.isDebugEnabled()) LOG.debug("TM_TLOG_AGE_COMMITTED_RECORDS is not in ms.env");
      }
      LOG.info("ageCommitted is " + ageCommitted);

      versions = 5;
      try {
         String maxVersions = System.getenv("TM_TLOG_MAX_VERSIONS");
         if (maxVersions != null){
            versions = (Integer.parseInt(maxVersions) > versions ? Integer.parseInt(maxVersions) : versions);
         }
      }
      catch (Exception e) {
         if (LOG.isDebugEnabled()) LOG.debug("TM_TLOG_MAX_VERSIONS is not in ms.env");
      }

      tlogNumLogs = 1;
      try {
         String numLogs = System.getenv("TM_TLOG_NUM_LOGS");
         if (numLogs != null) {
            tlogNumLogs = Math.max( 1, Integer.parseInt(numLogs));
         }
      }
      catch (Exception e) {
         if (LOG.isDebugEnabled()) LOG.debug("TM_TLOG_NUM_LOGS is not in ms.env");
      }
      disableBlockCache = false;
      try {
         String blockCacheString = System.getenv("TM_TLOG_DISABLE_BLOCK_CACHE");
         if (blockCacheString != null){
            disableBlockCache = (Integer.parseInt(blockCacheString) != 0);
            if (LOG.isDebugEnabled()) LOG.debug("disableBlockCache != null");
         }
      }
      catch (Exception e) {
         if (LOG.isDebugEnabled()) LOG.debug("TM_TLOG_DISABLE_BLOCK_CACHE is not in ms.env");
      }
      LOG.info("disableBlockCache is " + disableBlockCache);

      switch (tlogNumLogs) {
        case 1:
          tLogHashKey = 0b0;
          tLogHashShiftFactor = 63;
          break;
        case 2:
          tLogHashKey = 0b1;
          tLogHashShiftFactor = 63;
          break;
        case 4:
          tLogHashKey = 0b11;
          tLogHashShiftFactor = 62;
          break;
        case 8:
          tLogHashKey = 0b111;
          tLogHashShiftFactor = 61;
          break;
        case 16:
          tLogHashKey = 0b1111;
          tLogHashShiftFactor = 60;
          break;
        case 32:
          tLogHashKey = 0b11111;
          tLogHashShiftFactor = 59;
          break;
        default : {
          LOG.error("TM_TLOG_NUM_LOGS must b 1 or a power of 2 in the range 2-32");
          throw new RuntimeException();
        }
      }
      if (LOG.isDebugEnabled()) LOG.debug("TM_TLOG_NUM_LOGS is " + tlogNumLogs);

      HColumnDescriptor hcol = new HColumnDescriptor(TLOG_FAMILY);
      if (disableBlockCache) {
         hcol.setBlockCacheEnabled(false);
      }
      hcol.setMaxVersions(versions);
      admin = new HBaseAdmin(config);

      if (distributedFS) {
         fillerSize = 2;
      }
      else {
         fillerSize = 4097;
      }
      filler = new byte[fillerSize];
      Arrays.fill(filler, (byte) ' ');
      startTimes      =    new long[1000];
      endTimes        =    new long[1000];
      synchTimes      =    new long[1000];
      bufferSizes     =    new long[1000];
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

      try {
         if (LOG.isDebugEnabled()) LOG.debug("try new HBaseAuditControlPoint");
         tLogControlPoint = new HBaseAuditControlPoint(config);
      }
      catch (Exception e) {
         LOG.error("Unable to create new HBaseAuditControlPoint object " + e);
      }

      tlogAuditLock =    new Object[tlogNumLogs];
      table = new HTable[tlogNumLogs];

      try {
         // Get the asn from the last control point.  This ignores 
         // any asn increments between the last control point
         // write and a system crash and could result in asn numbers
         // being reused.  However this would just mean that some old 
         // records are held onto a bit longer before cleanup and is safe.
         asn.set(tLogControlPoint.getStartingAuditSeqNum());
      }
      catch (Exception e2){
         if (LOG.isDebugEnabled()) LOG.debug("Exception setting the ASN " + e2);
         if (LOG.isDebugEnabled()) LOG.debug("Setting the ASN to 1");
         asn.set(1L);  // Couldn't read the asn so start asn at 1
      }

      for (int i = 0 ; i < tlogNumLogs; i++) {
         tlogAuditLock[i]      = new Object();
         String lv_tLogName = new String(TLOG_TABLE_NAME + "_LOG_" + Integer.toHexString(i));
         boolean lvTlogExists = admin.tableExists(lv_tLogName);
         if (LOG.isDebugEnabled()) LOG.debug("Tlog table " + lv_tLogName + (lvTlogExists? " exists" : " does not exist" ));
         HTableDescriptor desc = new HTableDescriptor(lv_tLogName);
         desc.addFamily(hcol);

          if (lvTlogExists == false) {
            // Need to prime the asn for future writes
            try {
               if (LOG.isDebugEnabled()) LOG.debug("Creating the table " + lv_tLogName);
               admin.createTable(desc);
               asn.set(1L);  // TLOG didn't exist previously, so start asn at 1
            }
            catch (TableExistsException e) {
               LOG.error("Table " + lv_tLogName + " already exists");
            }
         }
         try {
            if (LOG.isDebugEnabled()) LOG.debug("try new HTable index " + i);
            table[i] = new HTable(config, desc.getName());
         }
         catch(Exception e){
            LOG.error("TmAuditTlog Exception on index " + i + "; " + e);
            throw new RuntimeException(e);
         }

         table[i].setAutoFlush(this.useAutoFlush);

      }

      lvAsn = asn.get();
      // This control point write needs to be delayed until after recovery completes, 
      // but is here as a placeholder
      if (LOG.isDebugEnabled()) LOG.debug("Starting a control point with asn value " + lvAsn);
      tLogControlPointNum = tLogControlPoint.doControlPoint(lvAsn);

      if (LOG.isTraceEnabled()) LOG.trace("Exit constructor()");
      return;
   }

   public static long asnGetAndIncrement () {
      if (LOG.isTraceEnabled()) LOG.trace("asnGetAndIncrement");
      return asn.getAndIncrement();
   }

   public void putSingleRecord(final long lvTransid, final String lvTxState, final Set<TransactionRegionLocation> regions, boolean forced) throws Exception {
      long threadId = Thread.currentThread().getId();
      if (LOG.isTraceEnabled()) LOG.trace("putSingleRecord start in thread " + threadId);
      StringBuilder tableString = new StringBuilder();
      String transidString = new String(String.valueOf(lvTransid));
      boolean lvResult = true;
      long lvAsn;
      long startSynch = 0;
      long endSynch = 0;
      int lv_lockIndex = 0;
      int lv_TimeIndex = (timeIndex.getAndIncrement() % 500 );
      long lv_TotalWrites = totalWrites.incrementAndGet();
      long lv_TotalRecords = totalRecords.incrementAndGet();
      if (regions != null) {
         // Regions passed in indicate a state record where recovery might be needed following a crash.
         // To facilitate branch notification we translate the regions into table names that can then
         // be translated back into new region names following a restart.  THis allows us to ensure all
         // branches reply prior to cleanup
         Iterator<TransactionRegionLocation> it = regions.iterator();
         while (it.hasNext()) {
	     String name = new String(it.next().getRegionInfo().getTable().getNameAsString());
            if (name.length() > 0){
               tableString.append(",");
               tableString.append(name);
            }
         }
         if (LOG.isDebugEnabled()) LOG.debug("table names: " + tableString.toString());
      }
      //Create the Put as directed by the hashed key boolean
      Put p;

      //create our own hashed key
      long key = (((lvTransid & tLogHashKey) << tLogHashShiftFactor) + (lvTransid & 0xFFFFFFFF));
      lv_lockIndex = (int)(lvTransid & tLogHashKey);
      if (LOG.isDebugEnabled()) LOG.debug("key: " + key + ", hex: " + Long.toHexString(key) + ", transid: " +  lvTransid);
      p = new Put(Bytes.toBytes(key));
 
      lvAsn = asn.getAndIncrement();
      if (LOG.isDebugEnabled()) LOG.debug("transid: " + lvTransid + " state: " + lvTxState + " ASN: " + lvAsn);
      p.add(TLOG_FAMILY, ASN_STATE, Bytes.toBytes(String.valueOf(lvAsn) + "," 
                       + transidString + "," + lvTxState 
                       + "," + Bytes.toString(filler)  
                       +  "," + tableString.toString()));

      if (LOG.isDebugEnabled()) LOG.debug("TLOG putSingleRecord synchronizing tlogAuditLock[" + lv_lockIndex + "] in thread " + threadId );
      startSynch = System.nanoTime();
      try {
         synchronized (tlogAuditLock[lv_lockIndex]) {
            endSynch = System.nanoTime();
            try {
               if (LOG.isDebugEnabled()) LOG.debug("try table.put " + p );
               startTimes[lv_TimeIndex] = System.nanoTime();
               table[lv_lockIndex].put(p);
               if ((forced) && (useAutoFlush == false)) {
                  if (LOG.isDebugEnabled()) LOG.debug("flushing commits");
                  table[lv_lockIndex].flushCommits();
               }
               endTimes[lv_TimeIndex] = System.nanoTime();
            }
            catch (Exception e2){
               // create record of the exception
               LOG.error("putSingleRecord Exception " + e2);
               e2.printStackTrace();
               throw e2;
            }
         } // End global synchronization
      }
      catch (Exception e) {
         // create record of the exception
         LOG.error("Synchronizing on tlogAuditLock[" + lv_lockIndex + "] Exception " + e);
         e.printStackTrace();
         throw e;
      }
      if (LOG.isDebugEnabled()) LOG.debug("TLOG putSingleRecord synchronization complete in thread " + threadId );

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
      if (lv_TimeIndex == 499) {
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
 
      if (LOG.isTraceEnabled()) LOG.trace("putSingleRecord exit");
   }

   //   public static Put formatRecord(final long lvTransid, final String lvTxState, final Set<TransactionRegionLocation> regions) throws Exception {
   public static Put formatRecord(final long lvTransid, final TransactionState lvTx) throws Exception {
      if (LOG.isTraceEnabled()) LOG.trace("formatRecord start");
      StringBuilder tableString = new StringBuilder();
      String transidString = new String(String.valueOf(lvTransid));
      String lvTxState;
      Set<TransactionRegionLocation> regions = lvTx.getParticipatingRegions();
      Iterator<TransactionRegionLocation> it = regions.iterator();
      long lvAsn;
      long threadId = Thread.currentThread().getId();
      while (it.hasNext()) {
         String name = new String(it.next().getRegionInfo().getTable().getNameAsString());
         if (name.length() > 0){
            tableString.append(",");
            tableString.append(name);
         }
      }
      if (LOG.isDebugEnabled()) LOG.debug("formatRecord table names " + tableString.toString());
      Put p;

      //create our own hashed key
      long key = (((lvTransid & tLogHashKey) << tLogHashShiftFactor) + (lvTransid & 0xFFFFFFFF));
      if (LOG.isDebugEnabled()) LOG.debug("key: " + key + " hex: " + Long.toHexString(key));
      p = new Put(Bytes.toBytes(key));
      lvAsn = asn.getAndIncrement();
      lvTxState = lvTx.getStatus();
      if (LOG.isDebugEnabled()) LOG.debug("formatRecord transid: " + lvTransid + " state: " + lvTxState + " ASN: " + lvAsn);
      p.add(TLOG_FAMILY, ASN_STATE, Bytes.toBytes(String.valueOf(lvAsn) + "," + transidString + "," + 
            lvTxState + tableString.toString()));

      if (LOG.isTraceEnabled()) LOG.trace("formatRecord exit");
      return p;
   }

   public boolean putBuffer(ArrayList<Put> buffer, int lv_lockIndex) throws Exception {
      long threadId = Thread.currentThread().getId();
      if (LOG.isTraceEnabled()) LOG.trace("putBuffer start in thread " + threadId);
      boolean lvResult = true;
      long prepTime = 0;
      long synchTime = 0;
      long writeTime = 0;
      long totalRecords = 0;

      try {
         if (LOG.isTraceEnabled()) LOG.trace("putBuffer synchronizing on tlogAuditLock[lv_lockIndex] " + lv_lockIndex + " in thread " + threadId);
         synchronized(tlogAuditLock[lv_lockIndex]) {
            synchTime = System.nanoTime() - prepTime;
            table[lv_lockIndex].put(buffer);
            writeTime = System.nanoTime() - synchTime;
         }
         if (LOG.isTraceEnabled()) LOG.trace("putBuffer tlogAuditLock[lv_lockIndex] " + lv_lockIndex + " synchronization complete in thread " + threadId);
         LOG.info("TLog Control Point Write Report\n" + 
                  "                        Total records: " + buffer.size() + "\n" +
                  "                        Prep time: " + prepTime + "\n" +
                  "                        Synch time: " + synchTime + "\n" +
                  "                        Write time: " + writeTime + "\n");
         
      }
      catch (Exception e) {
         if (LOG.isDebugEnabled()) LOG.debug("putBuffer table.put Exception " + e);
         lvResult = false;
      }

      if (LOG.isTraceEnabled()) LOG.trace("putBuffer exit in thread " + threadId + " with result " + lvResult);
      return lvResult;
   }

   public static int getRecord(final long lvTransid) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("getRecord start");
      int lvTxState = -1;
      String stateString;
      int lv_lockIndex = (int)(lvTransid & tLogHashKey);
      try {
         String transidString = new String(String.valueOf(lvTransid));
         Get g;
         //create our own hashed key
         long key = (((lvTransid & tLogHashKey) << tLogHashShiftFactor) + (lvTransid & 0xFFFFFFFF));
         if (LOG.isDebugEnabled()) LOG.debug("key: " + key + " hex: " + Long.toHexString(key));
         g = new Get(Bytes.toBytes(key));
         try {
            Result r = table[lv_lockIndex].get(g);
            byte [] value = r.getValue(TLOG_FAMILY, ASN_STATE);
            stateString =  new String (Bytes.toString(value));
            if (LOG.isDebugEnabled()) LOG.debug("stateString is " + stateString);
            if (stateString.compareTo("COMMITTED") == 0){
               lvTxState = TM_TX_STATE_COMMITTED;
            }
            else if (stateString.compareTo("ABORTED") == 0){
               lvTxState = TM_TX_STATE_ABORTED;
            }
            else if (stateString.compareTo("ACTIVE") == 0){
               lvTxState = TM_TX_STATE_ACTIVE;
            }
            else if (stateString.compareTo("PREPARED") == 0){
               lvTxState = TM_TX_STATE_PREPARED;
            }
            else if (stateString.compareTo("NOTX") == 0){
               lvTxState = TM_TX_STATE_NOTX;
            }
            else if (stateString.compareTo("FORGOTTEN") == 0){
               lvTxState = TM_TX_STATE_FORGOTTEN;
            }
            else if (stateString.compareTo("ABORTING") == 0){
               lvTxState = TM_TX_STATE_ABORTING;
            }
            else if (stateString.compareTo("COMMITTING") == 0){
               lvTxState = TM_TX_STATE_COMMITTING;
            }
            else if (stateString.compareTo("PREPARING") == 0){
               lvTxState = TM_TX_STATE_PREPARING;
            }
            else if (stateString.compareTo("FORGETTING") == 0){
               lvTxState = TM_TX_STATE_FORGETTING;
            }
            else if (stateString.compareTo("FORGETTING_HEUR") == 0){
               lvTxState = TM_TX_STATE_FORGETTING_HEUR;
            }
            else if (stateString.compareTo("BEGINNING") == 0){
               lvTxState = TM_TX_STATE_BEGINNING;
            }
            else if (stateString.compareTo("HUNGCOMMITTED") == 0){
              lvTxState = TM_TX_STATE_HUNGCOMMITTED;
            }
            else if (stateString.compareTo("HUNGABORTED") == 0){
               lvTxState = TM_TX_STATE_HUNGABORTED;
            }
            else if (stateString.compareTo("IDLE") == 0){
               lvTxState = TM_TX_STATE_IDLE;
            }
            else if (stateString.compareTo("FORGOTTEN_HEUR") == 0){
               lvTxState = TM_TX_STATE_FORGOTTEN_HEUR;
            }
            else if (stateString.compareTo("ABORTING_PART2") == 0){
               lvTxState = TM_TX_STATE_ABORTING_PART2;
            }
            else if (stateString.compareTo("TERMINATING") == 0){
               lvTxState = TM_TX_STATE_TERMINATING;
            }
            else {
               lvTxState = -1;
            }

            if (LOG.isDebugEnabled()) LOG.debug("transid: " + lvTransid + " state: " + lvTxState);
         }
         catch (IOException e){
             LOG.error("getRecord IOException");
             throw e;
         }
         catch (Exception e){
             LOG.error("getRecord Exception");
             throw e;
         }
      }
      catch (Exception e2) {
            LOG.error("getRecord Exception2 " + e2);
            e2.printStackTrace();
      }

      if (LOG.isTraceEnabled()) LOG.trace("getRecord end; returning " + lvTxState);
      return lvTxState;
   }

   public static String getRecord(final String transidString) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("getRecord start");
      long lvTransid = Long.parseLong(transidString, 10);
      int lv_lockIndex = (int)(lvTransid & tLogHashKey);
      String lvTxState = new String("NO RECORD");
      try {
         Get g;
         //create our own hashed key
         long key = (((lvTransid & tLogHashKey) << tLogHashShiftFactor) + (lvTransid & 0xFFFFFFFF));
         if (LOG.isDebugEnabled()) LOG.debug("key: " + key + " hex: " + Long.toHexString(key));
         g = new Get(Bytes.toBytes(key));
         try {
            Result r = table[lv_lockIndex].get(g);
            byte [] value = r.getValue(TLOG_FAMILY, ASN_STATE);
            StringTokenizer st = new StringTokenizer(value.toString(), ",");
            String asnToken = st.nextElement().toString();
            String transidToken = st.nextElement().toString();
            lvTxState = st.nextElement().toString();
            if (LOG.isDebugEnabled()) LOG.debug("transid: " + transidToken + " state: " + lvTxState);
         } catch (IOException e){
             LOG.error("getRecord IOException");
             throw e;
         }
      } catch (Exception e){
             LOG.error("getRecord Exception " + e);
             throw e;
      }
      if (LOG.isTraceEnabled()) LOG.trace("getRecord end; returning String:" + lvTxState);
      return lvTxState;
   }
      

   public static boolean deleteRecord(final long lvTransid) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("deleteRecord start " + lvTransid);
      String transidString = new String(String.valueOf(lvTransid));
      int lv_lockIndex = (int)(lvTransid & tLogHashKey);
      try {
         Delete d;
         //create our own hashed key
         long key = (((lvTransid & tLogHashKey) << tLogHashShiftFactor) + (lvTransid & 0xFFFFFFFF));
         if (LOG.isDebugEnabled()) LOG.debug("key: " + key + " hex: " + Long.toHexString(key));
         d = new Delete(Bytes.toBytes(key));
         if (LOG.isDebugEnabled()) LOG.debug("deleteRecord  (" + lvTransid + ") ");
         table[lv_lockIndex].delete(d);
      }
      catch (Exception e) {
         LOG.error("deleteRecord Exception " + e );
      }
      if (LOG.isTraceEnabled()) LOG.trace("deleteRecord - exit");
      return true;
   }

   public static boolean deleteAgedEntries(final long lvAsn) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("deleteAgedEntries start:  Entries older than " + lvAsn + " will be removed");
      for (int i = 0; i < tlogNumLogs; i++) {
         try {
            Scan s = new Scan();
            s.setCaching(500);
            s.setCacheBlocks(false);
            ArrayList<Delete> deleteList = new ArrayList<Delete>();
            ResultScanner ss = table[i].getScanner(s);

            try {
               for (Result r : ss) {
                  for (KeyValue kv : r.raw()) {
                     String valueString = new String(kv.getValue());
                     StringTokenizer st = new StringTokenizer(valueString, ",");
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
                              String rowKey = new String(r.getRow());
                              Delete del = new Delete(r.getRow());
                              if (LOG.isTraceEnabled()) LOG.trace("adding transid: " + transidToken + " to delete list");
                              deleteList.add(del);
                           }
                           else {
                              String key = new String(r.getRow());
                              Get get = new Get(r.getRow());
                              get.setMaxVersions(versions);  // will return last n versions of row
                              Result lvResult = table[i].get(get);
                             // byte[] b = lvResult.getValue(TLOG_FAMILY, ASN_STATE);  // returns current version of value
                              List<KeyValue> list = lvResult.getColumn(TLOG_FAMILY, ASN_STATE);  // returns all versions of this column
                              for (KeyValue element : list) {
                                 String value = new String(element.getValue());
                                 StringTokenizer stok = new StringTokenizer(value, ",");
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
           } finally {
              ss.close();
           }
           if (LOG.isDebugEnabled()) LOG.debug("attempting to delete list with " + deleteList.size() + " elements");
           synchronized(tlogAuditLock[i]) {
              table[i].delete(deleteList);
           }
        }
        catch (IOException e) {
           LOG.error("deleteAgedEntries IOException on table index " + i);
           e.printStackTrace();
        }
     }
     if (LOG.isTraceEnabled()) LOG.trace("deleteAgedEntries - exit");
     return true;
   }

   public long addControlPoint (final Map<Long, TransactionState> map) throws IOException, Exception {
      if (LOG.isTraceEnabled()) LOG.trace("addControlPoint start with map size " + map.size());
      long startTime = System.nanoTime();
      long endTime;
      long lvCtrlPt = 0L;
      long agedAsn;  // Writes older than this audit seq num will be deleted
      long lvAsn;    // local copy of the asn
      long key;
      boolean success = false;
      int cpWrites = 0;
      int lv_lockIndex;

      for (Map.Entry<Long, TransactionState> e : map.entrySet()) {
         try {
            Long transid = e.getKey();
            lv_lockIndex = (int)(transid & tLogHashKey);
            TransactionState value = e.getValue();
            if (value.getStatus().equals("COMMITTED")){
               if (LOG.isDebugEnabled()) LOG.debug("addControlPoint adding record for trans (" + transid + ") : state is " + value.getStatus());
               cpWrites++;
               if (forceControlPoint) {
                  putSingleRecord(transid, value.getStatus(), value.getParticipatingRegions(), true);
               }
               else {
                  putSingleRecord(transid, value.getStatus(), value.getParticipatingRegions(), false);
               }
            }
         }
         catch (Exception ex) {
            LOG.error("formatRecord Exception " + ex);
            ex.printStackTrace();
            throw ex;
         }
      }

      endTime = System.nanoTime();
      LOG.info("TLog Control Point Write Report\n" + 
                   "                        Total records: " 
                       +  map.size() + " in " + cpWrites + " write operations\n" +
                   "                        Write time: " + (endTime - startTime) / 1000 + " microseconds\n" );

      try {
         lvAsn = asn.getAndIncrement();

         // Write the control point interval and the ASN to the control point table
         lvCtrlPt = tLogControlPoint.doControlPoint(lvAsn); 

         if ((lvCtrlPt - 5) > 0){  // We'll keep 5 control points of audit
            try {
               agedAsn = tLogControlPoint.getRecord(String.valueOf(lvCtrlPt - 5));
               if ((agedAsn > 0) && (lvCtrlPt % 5 == 0)){
                  try {
                     if (LOG.isDebugEnabled()) LOG.debug("Attempting to remove TLOG writes older than asn " + agedAsn);
                     deleteAgedEntries(agedAsn);
                  }
                  catch (Exception e){
                     LOG.error("deleteAgedEntries Exception " + e);
                     throw e;
                  }
               }
               try {
                  tLogControlPoint.deleteAgedRecords(lvCtrlPt - 5);
               }
               catch (Exception e){
                  if (LOG.isDebugEnabled()) LOG.debug("addControlPoint - control point record not found ");
               }
            }
            catch (IOException e){
               LOG.error("addControlPoint IOException");
               e.printStackTrace();
               throw e;
            }
         }
      } catch (IOException e){
          LOG.error("addControlPoint IOException 2");
          e.printStackTrace();
          throw e;
      }
      if (LOG.isTraceEnabled()) LOG.trace("addControlPoint returning " + lvCtrlPt);
      return lvCtrlPt;
   } 

   public void getTransactionState (TransactionState ts) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("getTransactionState start; transid: " + ts.getTransactionId());

      try {
         long lvTransid = ts.getTransactionId();
         String transidString = new String(String.valueOf(lvTransid));
         Get g;
         long key = (((lvTransid & tLogHashKey) << tLogHashShiftFactor) + (lvTransid & 0xFFFFFFFF));
         if (LOG.isDebugEnabled()) LOG.debug("key: " + key + ", hexkey: " + Long.toHexString(key) + ", transid: " +  lvTransid);
         g = new Get(Bytes.toBytes(key));
         int lvTxState = TM_TX_STATE_NOTX;
         String stateString = "";
         String transidToken = "";
         int    lv_lockIndex = (int)(lvTransid & tLogHashKey);
         try {
            Result r = table[lv_lockIndex].get(g);
            if (r == null) {
               if (LOG.isDebugEnabled()) LOG.debug("getTransactionState: tLog result is null: " + transidString);
            }
            if (r.isEmpty()) {
               if (LOG.isDebugEnabled()) LOG.debug("getTransactionState: tLog empty result: " + transidString);
            }
            byte [] value = r.getValue(TLOG_FAMILY, ASN_STATE);
            if (value == null) {
               ts.setStatus(TM_TX_STATE_NOTX);
               if (LOG.isDebugEnabled()) LOG.debug("getTransactionState: tLog value is null: " + transidString);
               return;
            }
            if (value.length == 0) {
               ts.setStatus(TM_TX_STATE_NOTX);
               if (LOG.isDebugEnabled()) LOG.debug("getTransactionState: tLog transaction not found: " + transidString);
               return;
            }
            String recordString =  new String (Bytes.toString(value));
            StringTokenizer st = new StringTokenizer(recordString, ",");
            if (st.hasMoreElements()) {
               String asnToken = st.nextElement().toString() ;
               transidToken = st.nextElement().toString() ;
               stateString = st.nextElement().toString() ;
               if (LOG.isDebugEnabled()) LOG.debug("getTransactionState: stateString is " + stateString);
            }
            if (stateString.compareTo("COMMITTED") == 0){
               lvTxState = TM_TX_STATE_COMMITTED;
            }
            else if (stateString.compareTo("ABORTED") == 0){
               lvTxState = TM_TX_STATE_ABORTED;
            }
            else if (stateString.compareTo("ACTIVE") == 0){
               lvTxState = TM_TX_STATE_ACTIVE;
            }
            else if (stateString.compareTo("PREPARED") == 0){
               lvTxState = TM_TX_STATE_PREPARED;
            }
            else if (stateString.compareTo("NOTX") == 0){
               lvTxState = TM_TX_STATE_NOTX;
            }
            else if (stateString.compareTo("FORGOTTEN") == 0){
               // Need to get the previous state record so we know how to drive the regions
               String keyS = new String(r.getRow());
               Get get = new Get(r.getRow());
               get.setMaxVersions(versions);  // will return last n versions of row
               Result lvResult = table[lv_lockIndex].get(get);
               // byte[] b = lvResult.getValue(TLOG_FAMILY, ASN_STATE);  // returns current version of value
               List<KeyValue> list = lvResult.getColumn(TLOG_FAMILY, ASN_STATE);  // returns all versions of this column
               for (KeyValue element : list) {
                  String stringValue = new String(element.getValue());
                  st = new StringTokenizer(stringValue, ",");
                  if (st.hasMoreElements()) {
                     if (LOG.isDebugEnabled()) LOG.debug("Performing secondary search on (" + transidToken + ")");
                     String asnToken = st.nextElement().toString() ;
                     transidToken = st.nextElement().toString() ;
                     String stateToken = st.nextElement().toString() ;
                     if ((stateToken.compareTo("COMMITTED") == 0) || (stateToken.compareTo("ABORTED") == 0)) {
                         String rowKey = new String(r.getRow());
                         if (LOG.isDebugEnabled()) LOG.debug("Secondary search found record for (" + transidToken + ") with state: " + stateToken);
                         lvTxState = (stateToken.compareTo("COMMITTED") == 0 ) ? TM_TX_STATE_COMMITTED : TM_TX_STATE_ABORTED;
                         break;
                     }
                     else {
                         if (LOG.isDebugEnabled()) LOG.debug("Secondary search skipping entry for (" + 
                                    transidToken + ") with state: " + stateToken );
                     }
                  }
               }
            }
            else if (stateString.compareTo("ABORTING") == 0){
               lvTxState = TM_TX_STATE_ABORTING;
            }
            else if (stateString.compareTo("COMMITTING") == 0){
               lvTxState = TM_TX_STATE_COMMITTING;
            }
            else if (stateString.compareTo("PREPARING") == 0){
               lvTxState = TM_TX_STATE_PREPARING;
            }
            else if (stateString.compareTo("FORGETTING") == 0){
               lvTxState = TM_TX_STATE_FORGETTING;
            }
            else if (stateString.compareTo("FORGETTING_HEUR") == 0){
               lvTxState = TM_TX_STATE_FORGETTING_HEUR;
            }
            else if (stateString.compareTo("BEGINNING") == 0){
               lvTxState = TM_TX_STATE_BEGINNING;
            }
            else if (stateString.compareTo("HUNGCOMMITTED") == 0){
               lvTxState = TM_TX_STATE_HUNGCOMMITTED;
            }
            else if (stateString.compareTo("HUNGABORTED") == 0){
               lvTxState = TM_TX_STATE_HUNGABORTED;
            }
            else if (stateString.compareTo("IDLE") == 0){
               lvTxState = TM_TX_STATE_IDLE;
            }
            else if (stateString.compareTo("FORGOTTEN_HEUR") == 0){
               lvTxState = TM_TX_STATE_FORGOTTEN_HEUR;
            }
            else if (stateString.compareTo("ABORTING_PART2") == 0){
               lvTxState = TM_TX_STATE_ABORTING_PART2;
            }
            else if (stateString.compareTo("TERMINATING") == 0){
               lvTxState = TM_TX_STATE_TERMINATING;
            }
            else {
               lvTxState = -1;
            }

            // get past the filler
            st.nextElement();

            // Load the TransactionState object up with regions
            while (st.hasMoreElements()) {
               String tableNameToken = st.nextToken();
               HTable table = new HTable(config, tableNameToken);
               NavigableMap<HRegionInfo, ServerName> regions = table.getRegionLocations();
               Iterator it =  regions.entrySet().iterator();
               while(it.hasNext()) { // iterate entries.
                  NavigableMap.Entry pairs = (NavigableMap.Entry)it.next();
                  HRegionInfo regionKey = (HRegionInfo) pairs.getKey();
                  if (LOG.isDebugEnabled()) LOG.debug("getTransactionState: region: " + regionKey.getRegionNameAsString());
                  ServerName serverValue = (ServerName) regions.get(regionKey);
                  String hostAndPort = new String(serverValue.getHostAndPort());
                  StringTokenizer tok = new StringTokenizer(hostAndPort, ":");
                  String hostName = new String(tok.nextElement().toString());
                  int portNumber = Integer.parseInt(tok.nextElement().toString());
                  TransactionRegionLocation loc = new TransactionRegionLocation(regionKey, serverValue);
                  ts.addRegion(loc);
              }
            }
            ts.setStatus(lvTxState);

            if (LOG.isDebugEnabled()) LOG.debug("getTransactionState: returning transid: " + ts.getTransactionId() + " state: " + lvTxState);
         } catch (Exception e){
             LOG.error("getTransactionState Exception " + Arrays.toString(e.getStackTrace()));
             throw e;
         }
      }
      catch (Exception e2) {
            LOG.error("getTransactionState Exception2 " + e2);
            e2.printStackTrace();
      }
      if (LOG.isTraceEnabled()) LOG.trace("getTransactionState end transid: " + ts.getTransactionId());
      return;
   } 
}

