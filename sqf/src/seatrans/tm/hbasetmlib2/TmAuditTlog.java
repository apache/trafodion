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

import java.math.BigInteger;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

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
import org.apache.hadoop.hbase.ipc.TransactionalRegionInterface;
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
   private static HTable table;
   private static HBaseAuditControlPoint tLogControlPoint;
   private static long tLogControlPointNum;
   private static int dtmid;

   // For performance metrics
   private static long[] startTimes;
   private static long[] endTimes;
   private static long[] synchTimes;
   private static long[] bufferSizes;
   private static int    timeIndex;
   private static long   totalWriteTime;
   private static long   totalSynchTime;
   private static long   totalPrepTime;
   private static long   totalWrites;
   private static long   totalRecords;
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

   private static int    versions;
   private static boolean distributedFS;
   private static boolean useHashedKeys;
 
   private static AtomicLong asn;  // Audit sequence number is the monotonic increasing value of the tLog write

   private static ExecutorService auditWriter;
   private static Future<Boolean> auditWriteResult;

   private static ExecutorService controlPointWriter;
   private static Future<Boolean> cntrlPtWriteResult;

   private static TmAuditTlogAuditBuffer auditBuffer[]; // 2 elements for double buffering
   private static TmAuditTlogAuditBuffer currBuffer; // shorthand reference to curent buffer
   private static Integer currIndex;
   private static Integer prevIndex;
   private static Object grossAuditLock;          // Lock for synchronizing curr/prev buffer index
                                                  // and buffer switch   

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

   public class BufferNotUsedException extends Exception {

      public BufferNotUsedException() {}

      //Constructor that accepts a message
      public BufferNotUsedException(String message)
      {
         super(message);
      }
   }

   private class TmAuditTlogAuditBuffer{
      private ArrayList<Put> buffer;           // Each Put is an audit record
      private AtomicInteger writerCount;       // Number of writers intending to add to the buffer
      private AtomicInteger resultWaiterCount; // Number of writers waiting for results
      private Boolean bufferSubmitting;        // State of the buffer
      private Boolean bufferSubmitted;
      private Boolean bufferUsed;
      private Object bufferLock;               // Lock for the above
      
      private Boolean putResult;               // true if write was successful
      private Boolean resultAvailable;         // true when putResult is written
      private Boolean allResultsReceived;      // true when retrievedResultCount == resultWaiterCount
      private AtomicInteger retrievedResultCount; // number of writers that have received the write result
      private Object resultLock;                  // Lock to access the above

      private TmAuditTlogAuditBuffer () {
         buffer = new  ArrayList<Put>();
         buffer.clear();
         bufferSubmitting = false;
         bufferSubmitted = false;
         bufferUsed = false;
         writerCount = new AtomicInteger(0);
         bufferLock = new Object();

         putResult = false;
         resultAvailable = false;
         allResultsReceived = false;
         resultWaiterCount = new AtomicInteger(0);
         retrievedResultCount = new AtomicInteger(0);
         resultLock = new Object();
      }

      private void bufferAdd(Put localPut) throws Exception {
         long threadId = Thread.currentThread().getId();
         LOG.trace("BufferAdd start in thread " + threadId );
         LOG.trace("BufferAdd synchronizing on bufferLock in thread " + threadId );
         synchronized (bufferLock) {
            try {
               buffer.add(localPut);
               if (bufferUsed == false) {
                  bufferUsed = true;
               }
               LOG.debug("BufferAdd notifying bufferLock waiters in thread " + threadId);
               bufferLock.notify();
            }
            catch (Exception e) {
               LOG.debug("TmAuditTlogAuditBuffer Exception trying bufferAdd" + e);
               throw e;
            }
         }
         LOG.trace("BufferAdd bufferLock synchronization complete in thread " + threadId );
         LOG.trace("BufferAdd end in thread " + threadId );
      }

      private int bufferSize() throws Exception {
         int lvSize;
         long threadId = Thread.currentThread().getId();
         LOG.trace("BufferSize start in thread " + threadId );
         LOG.trace("BufferSize synchronizing on bufferLock in thread " + threadId );
         synchronized (bufferLock) {
            try {
               lvSize = buffer.size();
            }
            catch (Exception e) {
               LOG.debug("TmAuditTlogAuditBuffer Exception trying bufferSize" + e);
               throw e;
            }
         }
         LOG.trace("TmAuditTlogAuditBuffer bufferSize end; returning " + lvSize + " in thread " 
                    +  Thread.currentThread().getId());
         return lvSize;
      }

      private boolean getResult() throws Exception{
         boolean lvResult;
         long threadId = Thread.currentThread().getId();
         LOG.trace("TmAuditTlogAuditBuffer getResult start in thread " + threadId);
         if (bufferUsed == false) {
            LOG.debug("TmAuditTlogAuditBuffer throwing BufferNotUsedException in thread " + threadId);
            throw new BufferNotUsedException("Results from unused buffer not available");
         }
         LOG.trace("TmAuditTlogAuditBuffer getResult synchronizing on resultLock in thread " + threadId);
         synchronized (resultLock) {
            while (resultAvailable == false) {
               try {
                  LOG.debug("TmAuditTlogAuditBuffer getResult resultLock.wait in thread " + threadId);
                  resultLock.wait();
               }
               catch (Exception e) {
                  LOG.debug("TmAuditTlogAuditBuffer Exception trying getResult" + e);
                  throw e;
               }
            }
            lvResult = putResult;
            resultLock.notify();
         } // end synchronized
         LOG.trace("TmAuditTlogAuditBuffer getResult synchronization on resultLock complete in thread " 
                    + threadId);
         LOG.trace("TmAuditTlogAuditBuffer getResult end returning " + lvResult + " in thread " + threadId);
         return lvResult;
      }

      private void incRetrievedResult() throws Exception{
         long threadId = Thread.currentThread().getId();
         LOG.trace("TmAuditTlogAuditBuffer incRetrievedResult start in thread " + threadId);
         int lvResultCount = 0;
         try {
            lvResultCount = retrievedResultCount.incrementAndGet();
         }
         catch (Exception e) {
            LOG.debug("incRetrievedResult Exception " + e);
            throw e;
         }
         if (lvResultCount == resultWaiterCount.get()){
            LOG.trace("incRetrievedResult synchronizing on  resultLock in thread " + threadId );
            synchronized (resultLock) {
               LOG.debug("incRetrievedResult all results received");
               allResultsReceived = true;
               resultLock.notify();
            }// end synchronized
            LOG.trace("TmAuditTlogAuditBuffer incRetrievedResult resultLock synchronization complete in thread " 
                     + threadId);
         }
         else {
            LOG.debug("incRetrievedResult more results needed.  Notifying additional waiters.");
         }
         LOG.trace("TmAuditTlogAuditBuffer incRetrievedResult end; " + threadId);
         return;
      }

      private void setResult(boolean pvResult) throws Exception{
         long threadId = Thread.currentThread().getId();
         LOG.trace("TmAuditTlogAuditBuffer setResult start in thread " + threadId);
         LOG.trace("TmAuditTlogAuditBuffer setResult synchronizing on resultLock in thread " + threadId);
         synchronized (resultLock) {
            try {
               putResult = pvResult;
               retrievedResultCount.set(0);
               LOG.debug("TmAuditTlogAuditBuffer notifying resultLock waiters.  buffsize: " + buffer.size() + " thread " 
                           +  threadId);
               resultAvailable = true;
               resultLock.notify();
            }
            catch (Exception e) {
               LOG.debug("TmAuditTlogAuditBuffer Exception trying setResult" + e);
               throw e;
            }
         } // synchronized
         LOG.trace("TmAuditTlogAuditBuffer setResult synchronization on resultLock complete in thread " + threadId);
         LOG.trace("TmAuditTlogAuditBuffer setResult end in thread " + threadId);
         return;
      }

      private void bufferClear() throws Exception {
         long threadId = Thread.currentThread().getId();
         LOG.trace("TmAuditTlogAuditBuffer bufferClear start in thread " + threadId);
         LOG.trace("TmAuditTlogAuditBuffer bufferClear synchronizing on bufferLock in thread " + threadId);
         synchronized (bufferLock) {
            try {
               bufferSubmitting = false;
               bufferSubmitted = false;
               bufferUsed = false;
               buffer.clear();
               writerCount.set(0);
               resultWaiterCount.set(0);
            }
            catch (Exception e) {
               LOG.debug("Exception trying bufferClear.clear" + e);
               throw e;
            }
         }
         LOG.trace("TmAuditTlogAuditBuffer bufferClear synchronization on bufferLock complete in thread " + threadId);
         LOG.trace("TmAuditTlogAuditBuffer bufferClear synchronizing on resultLock in thread " + threadId);
         synchronized (resultLock) {
            putResult = false;
            resultAvailable = false;
            allResultsReceived = false;
            retrievedResultCount.set(0);
         }
         LOG.trace("TmAuditTlogAuditBuffer bufferClear synchronization on resultLock complete in thread " + threadId);
         LOG.trace("TmAuditTlogAuditBuffer bufferClear end in thread " + threadId);
      }
   }// End of class TmAuditTlogAuditBuffer

   private class TmAuditTlogAuditWriter implements Callable<Boolean> {
   
      TmAuditTlogAuditBuffer cvAuditBuffer;
      public TmAuditTlogAuditWriter ( ) {
         long threadId = Thread.currentThread().getId();
         LOG.trace("Enter TmAuditTlogAuditWriter constructor.  thread " +  threadId);
         int lvPrevIndex;
         int lvCurrIndex;
         LOG.trace("TmAuditTlogAuditWriter synchronizing on grossAuditLock in thread " 
                     + threadId);
         synchronized (grossAuditLock) {
            lvPrevIndex = prevIndex; 
            lvCurrIndex = currIndex; 
         }
         LOG.trace("TmAuditTlogAuditWriter grossAuditLock synchronization complete in thread " 
                     + threadId);
         cvAuditBuffer = auditBuffer[lvPrevIndex];
         boolean lvResult;
         try {
            // First we make sure the previous buffer's results are returned before 
            // trying to put another.  This prevents us from having 2 outstanding writes
            // at the same time which might complete in reverse order.
            lvResult =  cvAuditBuffer.getResult();
            LOG.debug("TmAuditTlogAuditWriter() results received " + lvResult + 
                      " constructor synchronizing on buffer[" + lvPrevIndex + "].resultLock (" + 
                        lvPrevIndex + ") in thread " + threadId);

            LOG.trace("TmAuditTlogAuditWriter synchronizing on auditBuffer[" + lvPrevIndex + "].resultLock in thread " 
                     + threadId);
            synchronized (cvAuditBuffer.resultLock) {
               while (cvAuditBuffer.allResultsReceived == false) {
                  try {
                     LOG.debug("resultLock resultLock.wait by thread " + threadId);
                     cvAuditBuffer.resultLock.wait();
                  }
                  catch (InterruptedException e){
                     LOG.debug("TmAuditTlogAuditWriter() InterruptedException caught waiting for additional results" + e);
                     continue;
                  }
               } // while
            } // synchronized
            LOG.trace("TmAuditTlogAuditWriter auditBuffer[" + lvPrevIndex + "].resultLock synchronization complete in thread " 
                     + threadId);

            // All previous results received, so let's clear the buffer
            try {
               cvAuditBuffer.bufferClear();
            }
            catch (Exception e){
               LOG.debug("TmAuditTlogAuditWriter() Exception caught attempting to clear buffer " 
                          + lvPrevIndex + " " + e);
            }
         } // try
         catch (BufferNotUsedException e) {
            LOG.debug("TmAuditTlogAuditWriter() Previous buffer not used");
         }
         catch (Exception e2) {
            LOG.debug("TmAuditTlogAuditWriter() Exception verifying previous results " + e2);
         }
         LOG.trace("TmAuditTlogAuditWriter constructor exit by thread " +  threadId);
      }
 
      @Override
      public Boolean call() throws IOException, InterruptedException {
        long threadId = Thread.currentThread().getId();
        LOG.trace("TmAuditTlogAuditWriter in call() thread " + threadId);
        int lvPrevIndex;
        int lvCurrIndex;
        int currBuffSize = 0;
        long startSynch = 0;
        long endSynch = 0;
        LOG.trace("TmAuditTlogAuditWriter call() synchronizing on grossAuditLock in thread " 
                     + threadId);
        synchronized (grossAuditLock) {
           // Set a shortcut to the current buffer and switch the currIndex and PrevIndex
           // This will force any new writers that arrive to the other buffer, but we can
           // still handle all threads in the middle of the put to the current buffer
           cvAuditBuffer = auditBuffer[currIndex];
           lvPrevIndex = prevIndex = currIndex;
           lvCurrIndex = (currIndex = (prevIndex == 0)? 1 : 0) ;
        } // End synchronized
        LOG.trace("TmAuditTlogAuditWriter call() grossAuditLock synchronization complete in thread " 
                     + threadId);

        try {

           LOG.trace("TmAuditTlogAuditWriter call() synchronizing on auditBuffer[" + lvPrevIndex + "].bufferLock in thread " 
                     + threadId);
           synchronized(cvAuditBuffer.bufferLock) {
              while (cvAuditBuffer.writerCount.get() != cvAuditBuffer.buffer.size()) {
                 // Some other thread intends to add a Put to this buffer, but it's not here yet

                 LOG.debug("TmAuditTlogAuditWriter call() thread " + threadId + 
                        " waiting for additional put in buffer[" + lvPrevIndex + "] of size " 
                        + cvAuditBuffer.buffer.size() + 
                        " and " + cvAuditBuffer.writerCount.get() + " writers" );
                 cvAuditBuffer.bufferLock.wait();
              }
           } // end synchronized
           LOG.trace("TmAuditTlogAuditWriter call() synchronization on auditBuffer[" + lvPrevIndex + 
                     "].bufferLock complete in thread " + threadId);

           currBuffSize = cvAuditBuffer.buffer.size();

           LOG.trace("TmAuditTlogAuditWriter call() synchronizing on tablePutLock in thread " + threadId);
           startSynch = System.nanoTime();
           synchronized(tablePutLock) {
              endSynch = startTimes[timeIndex] = System.nanoTime();
              table.put(cvAuditBuffer.buffer);
              if (! distributedFS) {
                 //extra write to force flush
                 table.put(cvAuditBuffer.buffer);
              }
              endTimes[timeIndex] = System.nanoTime();
           }
           LOG.trace("TmAuditTlogAuditWriter call() tablePutLock synchronization complete in thread " + threadId);

           synchTimes[timeIndex] = endSynch - startSynch;
           totalSynchTime += synchTimes[timeIndex];
           totalWriteTime += (endTimes[timeIndex] - startTimes[timeIndex]);
           totalPrepTime  += (startTimes[timeIndex] - endTimes[timeIndex-1]);
           totalWrites++;
           totalRecords += cvAuditBuffer.buffer.size();
           if (synchTimes[timeIndex] > maxSynchTime) {
              maxSynchTime = synchTimes[timeIndex];
           }
           if (synchTimes[timeIndex] < minSynchTime) {
              minSynchTime = synchTimes[timeIndex];
           }
           if ((endTimes[timeIndex] - startTimes[timeIndex]) > maxWriteTime) {
              maxWriteTime = (endTimes[timeIndex] - startTimes[timeIndex]);
              maxWriteTimeBuffSize = currBuffSize;
           }
           if ((endTimes[timeIndex] - startTimes[timeIndex]) < minWriteTime) {
              minWriteTime = (endTimes[timeIndex] - startTimes[timeIndex]);
              minWriteTimeBuffSize = currBuffSize;
           }
           if ((startTimes[timeIndex] - endTimes[timeIndex-1]) > maxPrepTime) {
              maxPrepTime = (startTimes[timeIndex] - endTimes[timeIndex-1]);
           }
           if ((startTimes[timeIndex] - endTimes[timeIndex-1]) < minPrepTime) {
              minPrepTime = (startTimes[timeIndex] - endTimes[timeIndex-1]);
           }
           if (cvAuditBuffer.buffer.size() > maxBufferSize) {
              maxBufferSize = cvAuditBuffer.buffer.size();
           }
           if (cvAuditBuffer.buffer.size() < minBufferSize) {
              minBufferSize = cvAuditBuffer.buffer.size();
           }
           if ((timeIndex % 100) == 99) {
              avgWriteTime = (double) (totalWriteTime/totalWrites);
              avgPrepTime = (double) (totalPrepTime/totalWrites);
              avgSynchTime = (double) (totalSynchTime/totalWrites);
              avgBufferSize = (double) ((double)totalRecords/(double)totalWrites);
              LOG.info("TLog Audit Write Report\n" + 
                        "                        Total records: " 
                            + totalRecords + " in " + totalWrites + " write operations\n" +
                        "                        Write time:\n" +
                        "                                     Min:  " 
                            + minWriteTime / 1000 + " microseconds    Buff size: " 
                            + minWriteTimeBuffSize + "\n" +
                        "                                     Max:  " 
                            + maxWriteTime / 1000 + " microseconds    Buff size: " +
                            + maxWriteTimeBuffSize + "\n" +
                        "                                     Avg:  " 
                            + avgWriteTime / 1000 + " microseconds\n" +
                        "                        Synch time:\n" +
                        "                                     Min:  " 
                            + minSynchTime / 1000 + " microseconds\n" +
                        "                                     Max:  " 
                            + maxSynchTime / 1000 + " microseconds\n" +
                        "                                     Avg:  " 
                            + avgSynchTime / 1000 + " microseconds\n" +
                        "                        Prep time:\n" +
                        "                                     Min:  " 
                            + minPrepTime / 1000 + " microseconds\n" +
                        "                                     Max:  " 
                            + maxPrepTime / 1000 + " microseconds\n" +
                        "                                     Avg:  " 
                            + avgPrepTime / 1000 + " microseconds\n" +
                        "                        Buffer Size:\n" +
                        "                                     Min:  " 
                            + minBufferSize + "\n" +
                        "                                     Max:  " 
                            + maxBufferSize + "\n" +
                        "                                     Avg:  " 
                            + avgBufferSize + "\n");
              // Start at index 1 since there is no startTimes[0]
              timeIndex            = 1;
              endTimes[0]          = System.nanoTime();
              totalWriteTime       = 0;
              totalSynchTime       = 0;
              totalPrepTime        = 0;
              totalRecords         = 0;
              totalWrites          = 0;
              minWriteTime         = 5 * maxWriteTime;  // Some arbitrary high value
              maxWriteTime         = 0;
              minWriteTimeBuffSize = 0;
              maxWriteTimeBuffSize = 0;
              minSynchTime         = 5 * maxSynchTime;  // Some arbitrary high value
              maxSynchTime         = 0;
              minPrepTime          = 5 * maxPrepTime;    // Some arbitrary high value
              maxPrepTime          = 0;
              minBufferSize        = 1000;             // Some arbitrary high value
              maxBufferSize        = 0;
           }
           else {
              timeIndex++;
           }
        } catch (Exception e) {
           LOG.error("TmAuditTlogAuditWriter call() child put exception " + e);
           return Boolean.FALSE;
        }
        LOG.trace("TmAuditTlogAuditWriter 'call' end; for buffer[" + lvPrevIndex + "] returning success in thread " + threadId);
        return Boolean.TRUE;
      }
   }
   public void stop() { 
      LOG.trace("Entering stop()");
      auditWriter.shutdown(); 
      controlPointWriter.shutdown();
   }

   public class TmAuditTlogRegionSplitPolicy extends RegionSplitPolicy {

      @Override
      protected boolean shouldSplit(){
         return false;
      }
   }

   public TmAuditTlog (Configuration config) throws IOException {

      this.config = config;
      this.dtmid = Integer.parseInt(config.get("dtmid"));
      LOG.trace("Enter TmAuditTlog constructor for dtmid " + dtmid);
      TLOG_TABLE_NAME = config.get("TLOG_TABLE_NAME");
      HTableDescriptor desc = new HTableDescriptor(TLOG_TABLE_NAME);
      HColumnDescriptor hcol = new HColumnDescriptor(TLOG_FAMILY);

      if (LocalHBaseCluster.isLocal(config)) {
         distributedFS = false;
      }
      else {
         distributedFS = true;
      }
      LOG.debug("distributedFS is " + distributedFS);
      String maxVersions = System.getenv("TM_TLOG_MAX_VERSIONS");
      if (maxVersions != null){
         versions = (Integer.parseInt(maxVersions) > versions ? Integer.parseInt(maxVersions) : versions);
      }

      String useHash = System.getenv("TM_TLOG_HASH_KEYS");
      if (useHash != null)
         useHashedKeys = (Integer.parseInt(useHash) != 0);
      LOG.debug("TM_TLOG_HASH_KEYS is " + useHashedKeys);

      if (useHashedKeys ==  false) {
         desc.setValue(HTableDescriptor.SPLIT_POLICY, TmAuditTlogRegionSplitPolicy.class.getName()); // Never split
      }
      hcol.setMaxVersions(versions);
      desc.addFamily(hcol);
      admin = new HBaseAdmin(config);
      boolean lvTlogExists = admin.tableExists(TLOG_TABLE_NAME);
      filler = new byte[4097];
      Arrays.fill(filler, (byte) ' ');
      startTimes      =    new long[1000];
      endTimes        =    new long[1000];
      synchTimes      =    new long[1000];
      bufferSizes     =    new long[1000];
      totalWriteTime  =    0;
      totalSynchTime  =    0;
      totalPrepTime   =    0;
      totalWrites     =    0;
      totalRecords    =    0;
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
      timeIndex       =    1;

      currIndex       =    0;
      prevIndex       =    1;
      auditBuffer     =    new TmAuditTlogAuditBuffer[2];
      auditBuffer[0]  =    new TmAuditTlogAuditBuffer();
      auditBuffer[1]  =    new TmAuditTlogAuditBuffer();

      auditWriteResult   = null;
      cntrlPtWriteResult = null;

      auditWriter        = Executors.newSingleThreadExecutor();
      controlPointWriter = Executors.newSingleThreadExecutor();

      asn = new AtomicLong();  // Monotonically increasing count of write operations

      long lvAsn = 0;

      grossAuditLock = new Object();
      tablePutLock   = new Object();

      try {
         LOG.debug("try new HBaseAuditControlPoint");
         tLogControlPoint = new HBaseAuditControlPoint(config);
      }
      catch (Exception e) {
         LOG.error("Unable to create new HBaseAuditControlPoint object " + e);
      }

      // Need to prime the asn for future writes
      try {
         LOG.debug("Creating the table " + TLOG_TABLE_NAME);
         admin.createTable(desc);
         asn.set(1L);  // TLOG didn't exist previously, so start asn at 1
      }
      catch (TableExistsException e) {
         LOG.error("Table " + TLOG_TABLE_NAME + " already exists");
         try {
            // Get the asn from the last control point.  This ignores 
            // any asn increments between the last control point
            // write and a system crash and could result in asn numbers
            // being reused.  However this would just mean that some old 
            // records are held onto a bit longer before cleanup and is safe.
            asn.set(tLogControlPoint.getStartingAuditSeqNum());
         }
         catch (Exception e2){
            LOG.debug("Exception setting the ASN " + e2);
         }
      }

      try {
         LOG.debug("try new HTable");
         table = new HTable(config, desc.getName());
      }
      catch(IOException e){
         LOG.error("TmAuditTlog IOException " + e);
         throw new RuntimeException(e);
      }

      lvAsn = asn.get();
      // This control point write needs to be delayed until after recovery completes, 
      // but is here as a placeholder
      LOG.debug("Starting a control point with asn value " + lvAsn);
      tLogControlPointNum = tLogControlPoint.doControlPoint(lvAsn);

      LOG.trace("Exit constructor()");
      return;
   }

   public static long asnGetAndIncrement () {
      LOG.trace("asnGetAndIncrement");
      return asn.getAndIncrement();
   }

   public void putRecord(final long lvTransid, final String lvTxState, final Set<TransactionRegionLocation> regions, boolean wait) throws Exception {
      long threadId = Thread.currentThread().getId();
      LOG.trace("putRecord start in thread " + threadId);
      //Create MessageDigest object for MD5
      MessageDigest md = MessageDigest.getInstance("MD5");
      StringBuilder tableString = new StringBuilder();
      String transidString = new String(String.valueOf(lvTransid));
      boolean lvResult = true;
      long lvAsn;
      boolean lvSubmitIt = false;
      int lvMyIndex = -1;
      int lvPrevIndex = -1;
      if (regions != null) {
         // Regions passed in indicate a state record where recovery might be needed following a crash.
         // To facilitate branch notification we translate the regions into table names that can then
         // be translated back into new region names following a restart.  THis allows us to ensure all
         // branches reply prior to cleanup
         Iterator<TransactionRegionLocation> it = regions.iterator();
         while (it.hasNext()) {
            String name = new String(it.next().getRegionInfo().getTableNameAsString());
            if (name.length() > 0){
               tableString.append(",");
               tableString.append(name);
            }
         }
         LOG.debug("table names: " + tableString.toString());
      }

      //Create the Put as directed by the hashed key boolean
      Put p;
      if (useHashedKeys) {
         //Update input string in message digest hashed key
         p = new Put(md.digest(Bytes.toBytes(transidString)));
      }
      else {
         //Straight text key
         p = new Put(Bytes.toBytes(transidString));
      }
      synchronized (grossAuditLock) {
         lvMyIndex = currIndex;
         lvPrevIndex = prevIndex;

         // We need to increment the writer count and potentially the result waiter count
         // here while we have the gross lock because the audit buffer writer might pick up
         // the buffer to put it into the table before we've complted the put.  Incrementing these numbers
         // now signals our intentions and triggers the writer thread to wait until we've finished the put.
         auditBuffer[lvMyIndex].writerCount.getAndIncrement();
         if (wait) { // some writers are not waiters, e.g. not commit
            auditBuffer[lvMyIndex].resultWaiterCount.getAndIncrement();
         }
      } // End global synchronization

      lvAsn = asn.getAndIncrement();
      LOG.debug("transid: " + lvTransid + " state: " + lvTxState + " ASN: " + lvAsn + " buffer: " + lvMyIndex);
      p.add(TLOG_FAMILY, ASN_STATE, Bytes.toBytes(String.valueOf(lvAsn) + "," 
                       + transidString + "," + lvTxState 
                       + (distributedFS == false ? "," + Bytes.toString(filler) : "," ) 
                       +  "," + tableString.toString()));

      LOG.trace("TLOG putRecord synchronizing auditBuffer[" + lvMyIndex + "].bufferLock in thread " + threadId );
      synchronized (auditBuffer[lvMyIndex].bufferLock) {
         if ((auditBuffer[lvMyIndex].bufferSubmitting == false) && (wait == true)) {
            lvSubmitIt = auditBuffer[lvMyIndex].bufferSubmitting = true;
         }
         auditBuffer[lvMyIndex].bufferAdd(p);
      } //end synchronized bufferLock
      LOG.trace("TLOG putRecord auditBuffer[" + lvMyIndex + "].bufferLock synchronization complete in thread " + threadId );

      if (lvSubmitIt) {
         // I'm the first waiting writer so we want to make sure the writer has work pending.
         // Need to submit the write now.  Other threads can continue to add to the buffer until
         // the writer switches the buffer index
         LOG.debug("putRecord write of size: " + auditBuffer[lvMyIndex].buffer.size() + " submitting by thread " 
                    + threadId + " from currIndex " + lvMyIndex);
         auditWriteResult = auditWriter.submit(new TmAuditTlogAuditWriter());
      }
      else {
         // we're just joining a buffer in progress of being written
         LOG.debug("putRecord write of size: " + auditBuffer[lvMyIndex].buffer.size() + " joining write in progress in thread " 
                    + threadId + " from currIndex " + lvMyIndex);
      }

      // If I'm the submitter, then I need to wait for the completion to populate
      // the results in the buffer.  Other threads can just wait on the buffer results.
      if (lvSubmitIt) {
         try {
            if (auditWriteResult == null) {
               LOG.debug("putRecord auditWriteResult is null in thread " + threadId);
            }
            if (lvResult = auditWriteResult.get()) {
               LOG.trace("TLOG putRecord synchronizing auditBuffer[" + lvMyIndex + "].resultLock in thread " + threadId );
               synchronized (auditBuffer[lvMyIndex].resultLock) {
                  // We just got the results back from our audit buffer so we need to populate the results
                  LOG.debug("auditBuffer.setResult in putRecord " + lvResult + " by thread " + threadId + 
                             " from currIndex " + lvMyIndex);
                  auditBuffer[lvMyIndex].setResult(lvResult);
               } // End synchronization
            }
            else {
               LOG.debug("putRecord write auditWriteResult returned false in thread " + threadId);
               throw new RuntimeException();
            }
         }
         catch (Exception e){
            // create record of the exception
            LOG.error("putRecord Exception " + e);
            throw e;
         }
      } // If (lvSubmitIt)
      
      if (wait) {
         // We need to get the result to maintin proper counts
         lvResult = auditBuffer[lvMyIndex].getResult();

         LOG.debug("auditBuffer.incRetrievedResult with retrieved result " + lvResult + (lvSubmitIt ? " by submitter" : " by joiner") + " in thread " + threadId);
         auditBuffer[lvMyIndex].incRetrievedResult();
      }

      if (lvResult != true) {
         throw new Exception("Tlog putRecord unsuccessful");
      }
      LOG.trace("putRecord exit and results received from buffer " + lvMyIndex );
   }

//   public static Put formatRecord(final long lvTransid, final String lvTxState, final Set<TransactionRegionLocation> regions) throws Exception {
   public static Put formatRecord(final long lvTransid, final TransactionState lvTx) throws Exception {
      LOG.trace("formatRecord start");
      //Create MessageDigest object for MD5
      MessageDigest md = MessageDigest.getInstance("MD5");
      StringBuilder tableString = new StringBuilder();
      String transidString = new String(String.valueOf(lvTransid));
      String lvTxState;
      Set<TransactionRegionLocation> regions = lvTx.getParticipatingRegions();
      Iterator<TransactionRegionLocation> it = regions.iterator();
      long lvAsn;
      long threadId = Thread.currentThread().getId();
      while (it.hasNext()) {
         String name = new String(it.next().getRegionInfo().getTableNameAsString());
         if (name.length() > 0){
            tableString.append(",");
            tableString.append(name);
         }
      }
      LOG.debug("formatRecord table names " + tableString.toString());
      Put p;
      if (useHashedKeys) {
         p = new Put(md.digest(Bytes.toBytes(transidString)));
      }
      else {
         p = new Put(Bytes.toBytes(transidString));
      }
      lvAsn = asn.getAndIncrement();
      lvTxState = lvTx.getStatus();
      LOG.debug("formatRecord transid: " + lvTransid + " state: " + lvTxState + " ASN: " + lvAsn);
      p.add(TLOG_FAMILY, ASN_STATE, Bytes.toBytes(String.valueOf(lvAsn) + "," + transidString + "," + 
            lvTxState + tableString.toString()));

      LOG.trace("formatRecord exit");
      return p;
   }

   public boolean putBuffer(ArrayList<Put> buffer, long startTime) throws Exception {
      long threadId = Thread.currentThread().getId();
      LOG.trace("putBuffer start in thread " + threadId);
      boolean lvResult = true;
      long prepTime = 0;
      long synchTime = 0;
      long writeTime = 0;
      long totalRecords = 0;

      try {
         LOG.trace("putBuffer synchronizing on tablePutLock in thread " + threadId);
         prepTime = System.nanoTime() - startTime;
         synchronized(tablePutLock) {
            synchTime = System.nanoTime() - prepTime;
            table.put(buffer);
            writeTime = System.nanoTime() - synchTime;
         }
         LOG.trace("putBuffer tablePutLock synchronization complete in thread " + threadId);
         LOG.info("TLog Control Point Write Report\n" + 
                  "                        Total records: " + buffer.size() + "\n" +
                  "                        Prep time: " + prepTime + "\n" +
                  "                        Synch time: " + synchTime + "\n" +
                  "                        Write time: " + writeTime + "\n");
         
      }
      catch (Exception e) {
         LOG.debug("putBuffer table.put Exception " + e);
         lvResult = false;
      }

      LOG.trace("putBuffer exit in thread " + threadId + " with result " + lvResult);
      return lvResult;
   }

   public static int getRecord(final long lvTransid) throws IOException {
      LOG.trace("getRecord start");
      int lvTxState = -1;
      String stateString;
      try {
         //Create MessageDigest object for MD5
         MessageDigest md = MessageDigest.getInstance("MD5");
         String transidString = new String(String.valueOf(lvTransid));
         Get g;
         if (useHashedKeys) {
            g = new Get(md.digest(Bytes.toBytes(transidString)));
         }
         else {
            g = new Get(Bytes.toBytes(transidString));
         }
         try {
            Result r = table.get(g);
            byte [] value = r.getValue(TLOG_FAMILY, ASN_STATE);
            stateString =  new String (Bytes.toString(value));
            LOG.debug("stateString is " + stateString);
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

            LOG.debug("transid: " + lvTransid + " state: " + lvTxState);
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
      catch (NoSuchAlgorithmException e) {
            LOG.error("getRecord NoSuchAlgorithmException");
            e.printStackTrace();
      }

      LOG.trace("getRecord end; returning " + lvTxState);
      return lvTxState;
   }

   public static String getRecord(final String lvTransid) throws IOException {
      LOG.trace("getRecord start");
      String transidString = new String(String.valueOf(lvTransid));
      String lvTxState = new String("NO RECORD");
      try {
         //Create MessageDigest object for MD5
         MessageDigest md = MessageDigest.getInstance("MD5");
         Get g;
         if (useHashedKeys) {
            g = new Get(md.digest(Bytes.toBytes(transidString)));
         }
         else {
            g = new Get(Bytes.toBytes(transidString));
         }
         try {
            Result r = table.get(g);
            byte [] value = r.getValue(TLOG_FAMILY, ASN_STATE);
            StringTokenizer st = new StringTokenizer(value.toString(), ",");
            String asnToken = st.nextElement().toString();
            String transidToken = st.nextElement().toString();
            lvTxState = st.nextElement().toString();
            LOG.debug("transid: " + transidToken + " state: " + lvTxState);
         } catch (IOException e){
             LOG.error("getRecord IOException");
             throw e;
         }
      }
      catch (NoSuchAlgorithmException e) {
            LOG.error("getRecord NoSuchAlgorithmException");
            e.printStackTrace();
      }
      LOG.trace("getRecord end; returning String:" + lvTxState);
      return lvTxState;
   }
      

   public static boolean deleteRecord(final long transid) throws IOException {
      LOG.trace("deleteRecord start " + transid);
      String transidString = new String(String.valueOf(transid));

      try {
         //Create MessageDigest object for MD5
         MessageDigest md = MessageDigest.getInstance("MD5");
         Delete d;
         if (useHashedKeys) {
            d = new Delete(md.digest(Bytes.toBytes(transidString)));
         }
         else {
            d = new Delete(Bytes.toBytes(transidString));
         }
         LOG.debug("deleteRecord  (" + transid + ") ");
         table.delete(d);
      }
      catch (NoSuchAlgorithmException e) {
            LOG.error("deleteRecord NoSuchAlgorithmException " + e);
            e.printStackTrace();
      }
      catch (Exception e) {
         LOG.error("deleteRecord Exception " + e );
      }
      LOG.trace("deleteRecord - exit");
      return true;
   }

   public static boolean deleteTxList(final ArrayList<String> txList) throws IOException {
      LOG.trace("deleteTxList start " + txList);
      Iterator it = txList.iterator();
      Object item = new Object();
      while (it.hasNext()) {
         try {
            item = it.next();
            Delete d = new Delete(Bytes.toBytes(item.toString()));
            LOG.debug("deleteRecord  (" + item.toString() + ") ");
            table.delete(d);
         }
         catch (Exception e) {
            LOG.error("deleteRecord IOException");
         }
      }
      LOG.trace("deleteRecord - exit");
      return true;
   }

   public static boolean deleteAgedEntries(final long lvAsn) throws IOException {
      LOG.trace("deleteAgedEntries start:  Entries older than " + lvAsn + " will be removed");
      try {
         Scan s = new Scan();
         ArrayList<Delete> deleteList = new ArrayList<Delete>();
         String asnToken = new String();
         String stateToken = new String();
         String transidToken = new String();
         ResultScanner ss = table.getScanner(s);

         try {
            for (Result r : ss) {
               for (KeyValue kv : r.raw()) {
                  String valueString = new String(kv.getValue());
                  StringTokenizer st = new StringTokenizer(valueString, ",");
                  if (st.hasMoreElements()) {
                     asnToken = st.nextElement().toString() ;
                     transidToken = st.nextElement().toString() ;
                     stateToken = st.nextElement().toString() ;
                     if ((Long.parseLong(asnToken) < lvAsn) && (stateToken.equals("FORGOTTEN"))) {
                        String rowKey = new String(r.getRow());
                        Delete del = new Delete(r.getRow());
                        LOG.debug("adding transid: " + transidToken + " to delete list");
                        deleteList.add(del);
                     }               
                     else if ((Long.parseLong(asnToken) < lvAsn) && 
                             (stateToken.equals("COMMITTED") || stateToken.equals("ABORTED"))) {
                        String key = new String(r.getRow());
                        Get get = new Get(r.getRow());
                        get.setMaxVersions(versions);  // will return last n versions of row
                        Result lvResult = table.get(get);
                       // byte[] b = lvResult.getValue(TLOG_FAMILY, ASN_STATE);  // returns current version of value
                        List<KeyValue> list = lvResult.getColumn(TLOG_FAMILY, ASN_STATE);  // returns all versions of this column
                        for (KeyValue element : list) {
                           String value = new String(element.getValue());
                           StringTokenizer stok = new StringTokenizer(value, ",");
                           if (stok.hasMoreElements()) {
                              LOG.debug("Performing secondary search on (" + transidToken + ")");
                              asnToken = stok.nextElement().toString() ;
                              transidToken = stok.nextElement().toString() ;
                              stateToken = stok.nextElement().toString() ;
                              if ((Long.parseLong(asnToken) < lvAsn) && (stateToken.equals("FORGOTTEN"))) {
                                 String rowKey = new String(r.getRow());
                                 Delete del = new Delete(r.getRow());
                                 LOG.debug("Secondary search found new delete - adding (" + transidToken + ") with asn: " + asnToken + " to delete list");
                                 deleteList.add(del);
                                 break;
                              }
                              else {
                                 LOG.debug("Secondary search skipping entry with asn: " + asnToken + ", state: " 
                                             + stateToken + ", transid: " + transidToken );
                              }
                           }
                        }
                     } else {
                        LOG.debug("deleteAgedEntries skipping asn: " + asnToken + ", transid: " 
                                  + transidToken + ", state: " + stateToken);
                     }
                  }
               }
           }
        } finally {
           ss.close();
        }
        LOG.debug("attempting to delete list with " + deleteList.size() + " elements");
        table.delete(deleteList);
     }
     catch (IOException e) {
            LOG.error("deleteAgedEntries IOException");
        e.printStackTrace();
     }

     LOG.trace("deleteAgedEntries - exit");
     return true;
   }

   public static void addControlPoint (Callable<Boolean> writer, final ConcurrentHashMap<Long, TransactionState> map) throws Exception {
      long threadId = Thread.currentThread().getId();
      LOG.trace("addControlPoint start from thread " + threadId + " with map size " + map.size());

      cntrlPtWriteResult = controlPointWriter.submit(writer);
      try {
         if (cntrlPtWriteResult == null) {
            LOG.debug("cntrlPtWriteResult is null after submit from thread " + threadId);
         }
         if (cntrlPtWriteResult.get() != null) {
            LOG.debug("addControlPoint succeeded " + cntrlPtWriteResult.get() + " from thread " + threadId);
         }
         else {
            LOG.debug("addControlPoint returned null at cntrlPtWriteResult.get() from thread " + threadId);
         }
      } catch (Exception e){
         // create record of the exception
         LOG.error("addControlPoint Exception " + e);
         throw e;
      } 
      LOG.trace("addControlPoint end");
      return;
   } 

   public long addControlPoint (final ConcurrentHashMap<Long, TransactionState> map) throws IOException, Exception {
      LOG.trace("addControlPoint start with map size " + map.size());
      long lvCtrlPt;
      long agedAsn;  // Writes older than this audit seq num will be deleted
      long lvAsn;    // local copy of the asn
      boolean success = false;
      for (Map.Entry<Long, TransactionState> e : map.entrySet()) {
         try {
            Long transid = e.getKey();
            TransactionState value = e.getValue();
            LOG.debug("addControlPoint putting state record for trans (" + transid + ") : state is " + value.getStatus());
            putRecord(transid, value.getStatus(), value.getParticipatingRegions(), false);
         }
         catch (Exception ex) {
            LOG.error("formatRecord Exception");
            throw ex;
         }
      }

      try {
         lvAsn = asn.getAndIncrement();

         // Write the control point interval and the ASN to the control point table
         lvCtrlPt = tLogControlPoint.doControlPoint(lvAsn); 

         if ((lvCtrlPt - 5) > 0){  // We'll keep 5 control points of audit
            try {
               agedAsn = tLogControlPoint.getRecord(String.valueOf(lvCtrlPt - 5));
               if (agedAsn > 0){
                  try {
                     LOG.debug("Attempting to remove TLOG writes older than asn " + agedAsn);
                     deleteAgedEntries(agedAsn);
                  }
                  catch (IOException e){
                     LOG.error("deleteAgedEntries IOException");
                     throw e;
                  }
               }

               try {
                  tLogControlPoint.deleteAgedRecords(lvCtrlPt - 5);
               }
               catch (Exception e){
                  LOG.debug("addControlPoint - control point record not found ");
               }
            }
            catch (IOException e){
               LOG.error("addControlPoint IOException");
               throw e;
            }
         }
      } catch (IOException e){
          LOG.error("addControlPoint IOException");
          throw e;
      }
      LOG.trace("addControlPoint returning " + lvCtrlPt);
      return lvCtrlPt;
   } 

/*   public long doControlPoint (final ConcurrentHashMap<Long, TransactionState> map) throws IOException, Exception {
      LOG.trace("doControlPoint start with map size " + map.size());
      long lvCtrlPt;
      long agedAsn;  // Writes older than this audit seq num will be deleted
      long lvAsn;    // local copy of the asn
      boolean success = false;
      ArrayList<Put> lvBuffer = new ArrayList<Put>();
      for (Map.Entry<Long, TransactionState> e : map.entrySet()) {
         try {
            Long transid = e.getKey();
            TransactionState value = e.getValue();
            LOG.debug("formatting trans state record for trans (" + transid + ") : state is " + value.getStatus());
            Put lvPut = formatRecord(transid, value);
            lvBuffer.add(lvPut);
         }
         catch (Exception ex) {
            LOG.error("formatRecord Exception");
            throw ex;
         }
      }

      try {
         LOG.debug("putBuffer of size " + lvBuffer.size());
         success = putBuffer(lvBuffer);
      }
      catch (Exception e) {
         LOG.error("doControlPoint Exception in Tlog.putBuffer");
         throw e;
      }
      
      try {
         lvAsn = asn.getAndIncrement();

         // Write the control point interval and the ASN to the control point table
         lvCtrlPt = tLogControlPoint.doControlPoint(lvAsn); 
         LOG.debug("doControlPoint returned " + lvCtrlPt + " asn is " + lvAsn);

         if ((lvCtrlPt - 5) > 0){  // We'll keep 5 control points of audit
            try {
               LOG.debug("doControlPoint calling tLogControlPoint.getRecord " + (lvCtrlPt - 5));
               agedAsn = tLogControlPoint.getRecord(String.valueOf(lvCtrlPt - 5));
               if (agedAsn > 0){
                  try {
                     LOG.debug("Attempting to remove TLOG writes older than asn " + agedAsn);
                     deleteAgedEntries(agedAsn);
                  }
                  catch (IOException e){
                     LOG.error("deleteAgedEntries IOException");
                     throw e;
                  }
               }

               try {
                  LOG.debug("doControlPoint - removing control point record " + (lvCtrlPt - 5));
                  tLogControlPoint.deleteAgedRecords(lvCtrlPt - 5);
               }
               catch (Exception e){
                  LOG.debug("doControlPoint - control point record not found ");
               }
            }
            catch (IOException e){
               LOG.error("doControlPoint IOException");
               throw e;
            }
         }
      } catch (IOException e){
          LOG.error("doControlPoint IOException");
          throw e;
      }
      LOG.trace("doControlPoint returning " + lvCtrlPt);
      return lvCtrlPt;
   }
*/
   public void getTransactionState (TransactionState ts) throws IOException {
      LOG.trace("getTransactionState start; transid: " + ts.getTransactionId());

      try {
         //Create MessageDigest object for MD5
         MessageDigest md = MessageDigest.getInstance("MD5");
         String transidString = new String(String.valueOf(ts.getTransactionId()));
         Get g;
         if (useHashedKeys) {
            g = new Get(md.digest(Bytes.toBytes(transidString)));
         }
         else {
            g = new Get(Bytes.toBytes(transidString));
         }
         int lvTxState = TM_TX_STATE_NOTX;
         String recordString;
         String asnToken = new String();
         String stateString = new String();
         String transidToken = new String();
         String tableNameToken = new String();
         try {
            Result r = table.get(g);
            if (r == null) {
               LOG.debug("getTransactionState: tLog result is null: " + transidString);
            }
            if (r.isEmpty()) {
               LOG.debug("getTransactionState: tLog empty result: " + transidString);
            }
            byte [] value = r.getValue(TLOG_FAMILY, ASN_STATE);
            if (value == null) {
               ts.setStatus(TM_TX_STATE_NOTX);
               LOG.debug("getTransactionState: tLog value is null: " + transidString);
               return;
            }
            if (value.length == 0) {
               ts.setStatus(TM_TX_STATE_NOTX);
               LOG.debug("getTransactionState: tLog transaction not found: " + transidString);
               return;
            }
            recordString =  new String (Bytes.toString(value));
            StringTokenizer st = new StringTokenizer(recordString, ",");
            if (st.hasMoreElements()) {
               asnToken = st.nextElement().toString() ;
               transidToken = st.nextElement().toString() ;
               stateString = st.nextElement().toString() ;
               LOG.debug("getTransactionState: stateString is " + stateString);
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
               String stateToken = new String();
               String key = new String(r.getRow());
               Get get = new Get(r.getRow());
               get.setMaxVersions(versions);  // will return last n versions of row
               Result lvResult = table.get(get);
               // byte[] b = lvResult.getValue(TLOG_FAMILY, ASN_STATE);  // returns current version of value
               List<KeyValue> list = lvResult.getColumn(TLOG_FAMILY, ASN_STATE);  // returns all versions of this column
               for (KeyValue element : list) {
                  String stringValue = new String(element.getValue());
                  st = new StringTokenizer(stringValue, ",");
                  if (st.hasMoreElements()) {
                     LOG.debug("Performing secondary search on (" + transidToken + ")");
                     asnToken = st.nextElement().toString() ;
                     transidToken = st.nextElement().toString() ;
                     stateToken = st.nextElement().toString() ;
                     if ((stateToken.compareTo("COMMITTED") == 0) || (stateToken.compareTo("ABORTED") == 0)) {
                         String rowKey = new String(r.getRow());
                         LOG.debug("Secondary search found record for (" + transidToken + ") with state: " + stateToken);
                         lvTxState = (stateToken.compareTo("COMMITTED") == 0 ) ? TM_TX_STATE_COMMITTED : TM_TX_STATE_ABORTED;
                         break;
                     }
                     else {
                         LOG.debug("Secondary search skipping entry for (" + 
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
               tableNameToken = st.nextToken();
               HTable table = new HTable(config, tableNameToken);
               NavigableMap<HRegionInfo, ServerName> regions = table.getRegionLocations();
               Iterator it =  regions.entrySet().iterator();
               while(it.hasNext()) { // iterate entries.
                  NavigableMap.Entry pairs = (NavigableMap.Entry)it.next();
                  HRegionInfo key = (HRegionInfo) pairs.getKey();
                  LOG.debug("getTransactionState: region: " + key.getRegionNameAsString());
                  ServerName serverValue = (ServerName) regions.get(key);
                  String hostAndPort = new String(serverValue.getHostAndPort());
                  StringTokenizer tok = new StringTokenizer(hostAndPort, ":");
                  String hostName = new String(tok.nextElement().toString());
                  int portNumber = Integer.parseInt(tok.nextElement().toString());
                  TransactionRegionLocation loc = new TransactionRegionLocation(key, hostName, portNumber);
                  ts.addRegion(loc);
              }
            }
            ts.setStatus(lvTxState);

            LOG.debug("getTransactionState: returning transid: " + ts.getTransactionId() + " state: " + lvTxState);
         } catch (Exception e){
             LOG.error("getTransactionState Exception " + Arrays.toString(e.getStackTrace()));
             throw e;
         }
      }
      catch (NoSuchAlgorithmException e) {
            LOG.error("getTransactionState NoSuchAlgorithmException " + e);
            e.printStackTrace();
      }
      LOG.trace("getTransactionState end transid: " + ts.getTransactionId());
      return;
   } 
}

