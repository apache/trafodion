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
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.SortedMap;
import java.util.TreeMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

import org.apache.commons.codec.binary.Hex;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hbase.CellUtil;
import org.apache.hadoop.hbase.CoprocessorEnvironment;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.ServerName;
import org.apache.hadoop.hbase.Tag;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.coprocessor.BaseRegionObserver;
import org.apache.hadoop.hbase.coprocessor.ObserverContext;
import org.apache.hadoop.hbase.coprocessor.RegionCoprocessorEnvironment;
import org.apache.hadoop.hbase.regionserver.HRegion;
import org.apache.hadoop.hbase.regionserver.RegionServerServices;
import org.apache.hadoop.hbase.regionserver.transactional.TrxTransactionState;
import org.apache.hadoop.hbase.regionserver.wal.HLog;
import org.apache.hadoop.hbase.regionserver.wal.HLogKey;
import org.apache.hadoop.hbase.regionserver.wal.WALEdit;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.zookeeper.ZKUtil;
import org.apache.hadoop.hbase.zookeeper.ZooKeeperWatcher;
import org.apache.zookeeper.KeeperException;

public class TrxRegionObserver extends BaseRegionObserver {

private static final Log LOG = LogFactory.getLog(TrxRegionObserver.class);
public static final String trxkey = "TRAFODION";
public static final String trxkeytransactionsById = "transactionsById";
public static final String trxkeycommitedTransactionsBySequenceNumber = "commitedTransactionsBySequenceNumber";
public static final String trxkeycommitPendingTransactions = "commitPendingTransactions";
public static final String trxkeypendingTransactionsById = "pendingTransactionsById";
public static final String trxkeyindoubtTransactionsCountByTmid = "indoubtTransactionsCountByTmid";

public static final int TS_ACTIVE = 0;
public static final int TS_COMMIT_REQUEST = 1;
public static final int TS_COMMIT = 2;
public static final int TS_ABORT = 3;
public static final int TS_CONTROL_POINT_COMMIT = 4;
public static final byte TS_TRAFODION_TXN_TAG_TYPE = 41;

// In-doubt transaction list during recovered WALEdit replay
private SortedMap<Long, List<WALEdit>> pendingTransactionsById = new TreeMap<Long, List<WALEdit>>();//WALEdit>();

// Array to hold the number of indoubt transactions for each TM
private Map<Integer, Integer> indoubtTransactionsCountByTmid = new TreeMap<Integer,Integer>();

// Map for Transactional Region to exchange data structures between Region Observer coprocessor and Endpoint Coprocessor
static ConcurrentHashMap<String, Object> transactionsRefMap = new ConcurrentHashMap<String, Object>();

private ConcurrentHashMap<String, TrxTransactionState> transactionsById = new ConcurrentHashMap<String, TrxTransactionState>();
private Set<TrxTransactionState> commitPendingTransactions = Collections.synchronizedSet(new HashSet<TrxTransactionState>());

HRegion my_Region;
HRegionInfo regionInfo;
HLog tHLog;
ServerName sn;
String hostName;
int port;
long activeCount = 0;
long abortCount = 0;
long commitCount = 0;
long commitRequestCount = 0;
long totalEdits = 0;
long skippedEdits = 0;
int cleanAT = 0;
long minSeqID = 0;
int flushCount = 0;
int regionState = 0;
private Object recoveryCheckLock = new Object();
private Object editReplay = new Object();
private AtomicInteger nextSequenceId = new AtomicInteger(0);

private static String zNodePath = "/hbase/Trafodion/recovery/";
private static ZooKeeperWatcher zkw1 = null;
private static Object zkRecoveryCheckLock = new Object();

// Region Observer Coprocessor START
@Override
public void start(CoprocessorEnvironment e) throws IOException {

    RegionCoprocessorEnvironment re = (RegionCoprocessorEnvironment) e;
    
    if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: trxRegionObserver load start ");

    if (!re.getSharedData().containsKey(trxkey)) {
      // there is a short race here, in the worst case we create a watcher that will be notified once
      re.getSharedData().putIfAbsent(trxkey, transactionsRefMap);

      if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: trxRegionObserver put data structure into CoprocessorEnvironment ");
    }
 
   my_Region = re.getRegion();
   regionInfo = my_Region.getRegionInfo();
   tHLog = my_Region.getLog();
   RegionServerServices rss = re.getRegionServerServices();
   sn = rss.getServerName();
   hostName = sn.getHostname();
   port = sn.getPort();
   if (LOG.isTraceEnabled()) LOG.trace("Hostname " + hostName + " port " + port);
   zkw1 = rss.getZooKeeper();

   String regionName = my_Region.getRegionNameAsString();
   if (LOG.isTraceEnabled()) {
       LOG.trace("Trafodion Recovery Region Observer CP: HRegion " + regionName + " starts to put transactional data lists into reference map ... ");
       if(transactionsRefMap.isEmpty()) {
           LOG.trace("Empty Shared Map, will put objects -- TrxRegionObserver.");
       }
   }

   @SuppressWarnings("unchecked")
   SortedMap<Long, List<WALEdit>> pendingTransactionsByIdCheck = (SortedMap<Long, List<WALEdit>>)transactionsRefMap
                                                                 .get(regionName+trxkeypendingTransactionsById);
   if(pendingTransactionsByIdCheck != null) {
       this.pendingTransactionsById = pendingTransactionsByIdCheck;
   }
   else {
       transactionsRefMap.put(regionName+trxkeypendingTransactionsById, this.pendingTransactionsById);
   }

   @SuppressWarnings("unchecked")
   Map<Integer, Integer> indoubtTransactionsCountByTmidCheck = (Map<Integer, Integer>)transactionsRefMap
                                                               .get(regionName+trxkeyindoubtTransactionsCountByTmid);
   if(indoubtTransactionsCountByTmidCheck != null) {
       this.indoubtTransactionsCountByTmid = indoubtTransactionsCountByTmidCheck;
   }
   else {
       transactionsRefMap.put(regionName+trxkeyindoubtTransactionsCountByTmid, this.indoubtTransactionsCountByTmid);
   }

   @SuppressWarnings("unchecked")
   ConcurrentHashMap<String, TrxTransactionState> transactionsByIdCheck = (ConcurrentHashMap<String, TrxTransactionState>)
                                                                       transactionsRefMap
                                                                       .get(regionName+trxkeytransactionsById);
   if(transactionsByIdCheck != null) {
       this.transactionsById = transactionsByIdCheck;
   }
   else {
       transactionsRefMap.put(regionName+trxkeytransactionsById, this.transactionsById);
   }

   @SuppressWarnings("unchecked")
   Set<TrxTransactionState> commitPendingTransactionsCheck = (Set<TrxTransactionState>)transactionsRefMap
                                                          .get(regionName+trxkeycommitPendingTransactions);
   if(commitPendingTransactionsCheck != null) {
       this.commitPendingTransactions = commitPendingTransactionsCheck;
   }
   else {
       transactionsRefMap.put(regionName+trxkeycommitPendingTransactions,
                              this.commitPendingTransactions);
   }

    if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: trxRegionObserver load start complete");

} // end of start


static ConcurrentHashMap<String, Object> getRefMap() {

  return transactionsRefMap;

} // end of getRefmap


// Region Observer Coprocessor preWALRestore, called in HRegion ReplayRecoveredEdits

@Override
  public void preWALRestore(ObserverContext<RegionCoprocessorEnvironment> env, HRegionInfo info,
     HLogKey logKey, WALEdit logEdit) throws IOException {

     if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: preWALRestore coprocessor is invoked ... in table "+ logKey.getTablename().getNameAsString());

     ArrayList<KeyValue> kvs = logEdit.getKeyValues();
     if (kvs.size() <= 0) {
        if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP:PWR00 No KV inside Edits, skip ... ");
        return;
     }

     // Retrieve KV to see if it has the Trafodion Transaction context tag
     KeyValue kv = kvs.get(0); // get the first KV to check the associated transactional tag (all the KV pairs contain the same tag)
     if (LOG.isTraceEnabled()) LOG.trace("KV hex dump " + Hex.encodeHexString(kv.getValueArray() /*kv.getBuffer()*/));
     byte[] tagArray = Bytes.copy(kv.getTagsArray(), kv.getTagsOffset(), kv.getTagsLength());
     byte tagType = TS_TRAFODION_TXN_TAG_TYPE;
     if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: tag array hex dump " + Hex.encodeHexString(tagArray));
     Tag tag = Tag.getTag(tagArray, 0, kv.getTagsLength(), tagType);  //TagType.TS_TRAFODION_TXN_TAG_TYPE

     if (tag == null) {
        if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: there is no desired transactional tag in KV, skip ... ");
        return;
     }
     byte[] b = tag.getBuffer();
     int offset = Tag.TYPE_LENGTH_SIZE + Tag.TAG_LENGTH_SIZE;
     int version = Bytes.toInt(b,offset);
     int op = Bytes.toInt(b,Bytes.SIZEOF_INT+offset);
     long tid = Bytes.toLong(b,Bytes.SIZEOF_INT+Bytes.SIZEOF_INT+offset);
     long logSeqId = Bytes.toLong(b,Bytes.SIZEOF_INT+Bytes.SIZEOF_INT+Bytes.SIZEOF_LONG+offset);
     if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP:PWR01 Find transactional tag within Edits for tid " + tid + " op " + op + " log seq " + logSeqId + " version " + version);

     //Trafodion Recovery : process each edit according to its nature (prepare, commit, abort)

     switch (op) {

             case TS_ACTIVE: 
                   if (pendingTransactionsById.containsKey(tid)) {
                         if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP:   " + regionInfo.getRegionNameAsString() + " processing active edit for transaction: " + tid
                                + ", already find a previous edit with that id");
                        //get the editList from pendingTransactionsById, and add the logEdit into the editList
                        ArrayList <WALEdit> editList = (ArrayList<WALEdit>) (pendingTransactionsById.get(tid));
                        editList.add(logEdit);
                        activeCount++;
                        // no need to do the "put" back to the map since it gets the reference already
                   }
                   else {
                        // create the editList
                        List<WALEdit> editList = new ArrayList<WALEdit>();
                        editList.add(logEdit);
                        pendingTransactionsById.put(tid, editList);
                        activeCount++;
                         if (LOG.isDebugEnabled()) LOG.debug("Trafodion Recovery Region Observer CP:  " + regionInfo.getRegionNameAsString() + " find in-doubt transaction " + tid + " in active state");
                   }
             break;

             case TS_COMMIT_REQUEST: // Replace it to ACTIVE
                   if (pendingTransactionsById.containsKey(tid)) {
                        LOG.info("TRAF RCOV Region Observer CP:   " + regionInfo.getRegionNameAsString() + " processing commit request for transaction: " + tid
                                + ", already find a previous edit with that id");
                        //throw new IOException("Corrupted transaction log");
                        // get the editList from pendingTransactionsById, and add the logEdit into the editList
                        ArrayList<WALEdit> editList = (ArrayList<WALEdit>) (pendingTransactionsById.get(tid));
                        editList.add(logEdit);
                        // no need to do the "put" back to the map since it gets the reference already
                   }
                   else {
                        // create the editList
                        List<WALEdit> editList = new ArrayList<WALEdit>();
                        editList.add(logEdit);
                        pendingTransactionsById.put(tid, editList);
                        //pendingTransactionsById.put(tid, logEdit);
                        commitRequestCount++;
                        if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP:  " + regionInfo.getRegionNameAsString() + " find in-doubt transaction " + tid + " in prepared state");
                   }
             break;

             case TS_ABORT:
                   if (!pendingTransactionsById.containsKey(tid)) {
                        LOG.info("TRAF RCOV Region Observer CP:   " + regionInfo.getRegionNameAsString() + " processing abort for transaction: " + tid
                                 + ", but don't find a previous edit with that id");
                        abortCount++;
                        break;
                        //throw new IOException("Corrupted transaction log");
                   }
                   pendingTransactionsById.remove(tid);
                   abortCount++;
             break;

             case TS_COMMIT:
                   if (!pendingTransactionsById.containsKey(tid)) {
                         LOG.info("TRAF RCOV Region Observer CP:   " + regionInfo.getRegionNameAsString() + " processing commit for transaction: " + tid
                                 + ", but don't find a previous edit with that id");
                         commitCount++;
                         break;
                         //throw new IOException("Corrupted transaction log");
                   }
                   ArrayList<WALEdit> editList = (ArrayList<WALEdit>) (pendingTransactionsById.get(tid));
                   if (LOG.isDebugEnabled()) LOG.debug("TRAF RCOV Region Observer CP:  " + regionInfo.getRegionNameAsString() + " find in-doubt transaction " + tid + " to commit");
                   replayCommittedTransaction(tid, editList);
                   pendingTransactionsById.remove(tid);
                   commitCount++;
             break;

             case TS_CONTROL_POINT_COMMIT:
                      if (LOG.isTraceEnabled()) LOG.trace("AAA3 get CP edit : " + tid + " region " + regionInfo.getRegionNameAsString());
                     ArrayList<WALEdit> tempEditList = new ArrayList<WALEdit>();
                     tempEditList.add(logEdit);
                     replayCommittedTransaction(tid, tempEditList);  
                     if (pendingTransactionsById.containsKey(tid)) pendingTransactionsById.remove(tid);  
                     commitCount++;
             break;

             default:
                        throw new IllegalStateException("Trafodion Recovery Region Observer CP: Unexpected log entry type detected in audit replay");
     } // switch op

     totalEdits++;

     if (LOG.isDebugEnabled()) LOG.debug("TRAF RCOV PREWAL Region Observer CP:   " + regionInfo.getRegionNameAsString() + " Edits replay read " + totalEdits + " transactional operations (skipped " + skippedEdits
                        + " because sequence id <= " + minSeqID + "): " + commitRequestCount + " commitRequests, "
                        + abortCount + " aborts, " + commitCount + " commits, and " + flushCount + " flushes.");

     //recovery patch

     env.complete(); // do not need to invoke further down coprocessor
     env.bypass();

  } // end of preWALRestore


@Override
public void postOpen(ObserverContext<RegionCoprocessorEnvironment> e) {

   //        Trafodion Recovery : after Open, we should have alreday constructed all the indoubt transactions in
   //        pendingTransactionsById now process it and construct transaction list by TM id. These two data
   //        structures are put in the reference map which is shared with TrxRegionEndpoint coprocessor class per region 

   if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: postOpen coprocessor is invoked ... in region "+ regionInfo.getRegionNameAsString());

   // discard any in-doubt transaction if ENV (likely the property XML) indicates (should be set in start)
   // just ignore it for now
    if (cleanAT == 1) {
	if ((pendingTransactionsById != null) && (pendingTransactionsById.size() > 0)) {
	      LOG.info("Trafodion Recovery Region Observer CP: TM clean AT mode " + cleanAT + " discards " + pendingTransactionsById.size() + " in-doubt transaction ");
	      pendingTransactionsById.clear();
       }
    }

    if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery: Region " + regionInfo.getRegionNameAsString() + " is in state RECOVERING ");
              
    // remove split-log under region dir if TRegion has been recovered competely
    if ((pendingTransactionsById == null) || (pendingTransactionsById.size() == 0)) {
       // call method startRegionAfterRecovery to 1) archive the split-thlog, and 2) set region state = STARTED
       LOG.info("Trafodion Recovery Region Observer CP: Region " + regionInfo.getRegionNameAsString() + " has no in-doubt transaction, set region START ");
       try {       
             startRegionAfterRecovery();
       } catch (IOException exp) {
             LOG.error("Trafodion Recovery Region Observer CP:Flush failed after postOpen flush" + regionInfo.getRegionNameAsString());
       }
       return;
    }

    LOG.info("Trafodion Recovery Region Observer CP: Region " + regionInfo.getRegionNameAsString() + " find " + pendingTransactionsById.size() + 
                         " in-doubt transaction during edit replay, now reconstruct transaction state ");

    //for each indoubt transaction from pendingTransactionsById, build related transaction state object and add it into required lists for endPoint

    for (Entry<Long, List<WALEdit>> entry : pendingTransactionsById.entrySet()) {
        synchronized (recoveryCheckLock) {
                      long transactionId = entry.getKey();
		      String key = String.valueOf(transactionId);
                      if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: Region " + regionInfo.getRegionNameAsString() + " process in-doubt transaction " + transactionId);

                      int tmid = (int) (transactionId >> 32);
                      int count = 1;
                      if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: Region " + regionInfo.getRegionNameAsString() + " add prepared " + transactionId + " to TM " + tmid);
                      if (indoubtTransactionsCountByTmid.containsKey(tmid))
                            count =  (int) indoubtTransactionsCountByTmid.get(tmid) + 1;

                      indoubtTransactionsCountByTmid.put(tmid, count);
                      if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: Region " + regionInfo.getRegionNameAsString() + " has " + count +
                                                    " in-doubt-transaction from TM " + tmid);

                      //TBD may need to write the LOG again for reinstated txn (redo does not generate edits)
                      //since after open, HBase may toss out split-log while there are indoubt list in memory
                      //if the system crash again, those indoubt could be lost (so write them out), a recovery comletion should
                      //lead HBase to flush the memstore and reset the logSequenceId --> need to verify about the failure during recovery case
                      //it should be idempotent

         } // synchronized
     } // for all txns in indoubt transcation list

     // Now we need to inform TM through ZK (TBD here may need to check if 0.98 requires new APIs to construct region info

     byte [] lv_byte_region_info = regionInfo.toByteArray();
     String lv_encoded = regionInfo.getEncodedName();
   
     // loop for every tm, call TRS.createzNode (tmid, region encoded name, zNodedata)
     for (int node  : indoubtTransactionsCountByTmid.keySet()) {
           try {
                 if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: ZKW Create Recovery zNode TM " + node + " region encoded name " + lv_encoded + " region info bytes " + new String(lv_byte_region_info));
                 createRecoveryzNode(node, lv_encoded, lv_byte_region_info);
                  } catch (IOException exp) {
                  LOG.error("Trafodion Recovery Region Observer CP: ZKW Create recovery zNode failed");
            }
      }

      if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: ZKW Complete post of recovery zNode for region info " + new String(lv_byte_region_info));
     
      // Flush the cache (since we don't do it during replay committed transactions) and may need to re-write all the edits for in-doubt txn
      // in case the failure occurred again before the resolution
/*
      try {
             if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery:  Flushing cache in postOpen " + regionInfo.getRegionNameAsString());
             HRegion.FlushResult fr = my_Region.flushcache();
             if (!fr.isFlushSucceeded()) {
                 if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery:  Flushcache returns false !!! " + regionInfo.getRegionNameAsString());
             }
       } catch (IOException exp) {
             LOG.error("Trafodion Recovery: Flush failed after replay edits" + regionInfo.getRegionNameAsString());
             return;
      }
*/

} // end of postOpen


public void replayCommittedTransaction(long transactionId, ArrayList<WALEdit> editList) throws IOException {

   if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: " + regionInfo.getRegionNameAsString() + " replay commit for transaction: " + transactionId);
   
   int num  = editList.size();
   if (LOG.isInfoEnabled()) LOG.info("TRAF RCOV Region Observer CP: " + regionInfo.getRegionNameAsString() + " replay commit for transaction: "
                                     + transactionId + " with editList size is " + num);
   for ( int i = 0; i < num; i++){
   WALEdit val = editList.get(i);
   for (KeyValue kv : val.getKeyValues()) {
         synchronized (editReplay) {
             if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: " + regionInfo.getRegionNameAsString() + " replay commit for transaction: "
                                     + transactionId + " edit num " + i + " with Op " + kv.getTypeByte());
	    if (kv.getTypeByte() == KeyValue.Type.Put.getCode()) {
		Put put = new Put(CellUtil.cloneRow(kv)); // kv.getRow()
                put.add(CellUtil.cloneFamily(kv), CellUtil.cloneQualifier(kv), kv.getTimestamp(), CellUtil.cloneValue(kv));
		//put.add(kv);
	        my_Region.put(put); // let it generate new edits at this moment
	    } else if (CellUtil.isDelete(kv)) {
		Delete del = new Delete(CellUtil.cloneRow(kv));
	       	if (CellUtil.isDeleteFamily(kv)) {
	 	     del.deleteFamily(CellUtil.cloneFamily(kv));
	        } else if (kv.isDeleteType()) {
	             del.deleteColumn(CellUtil.cloneFamily(kv), CellUtil.cloneQualifier(kv));
	        }
                my_Region.delete(del);
	    }
        } // synchronized of editReplay
   } // for-loop (KeyValues)
   } // for-loop (edits)

} // end of replayCommittedTransaction

public void startRegionAfterRecovery() throws IOException {
   boolean isFlush = false;
         //TBD
         // if we have indoubt transaction, do we need to rewrite back to HLOG, otherwise, if the system crash in the middle of recovery
         // we could lose the memory  and HLOG, but the split-log may already be removed after region open
         // if flush succeeds, then it is not necessary

         // regionState = 2; // region started, Endpoint coprocessor can set region state STARTED if it detects there is no indoubt txn
         if (LOG.isTraceEnabled()) LOG.trace("Trafodion Recovery Region Observer CP: Region " + my_Region.getRegionInfo().getEncodedName() + " is STARTED.");
} // end of startRegionAfterRecovery


public void createRecoveryzNode(int node, String encodedName, byte [] data) throws IOException {

       synchronized(zkRecoveryCheckLock) {
         // default zNodePath for recovery
         String zNodeKey = hostName + "," + port + "," + encodedName;

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
} // end of createRecoveryzNode

    @Override
    public void preSplit(ObserverContext<RegionCoprocessorEnvironment> c, byte[] splitRow) throws IOException {
        HRegion region = c.getEnvironment().getRegion();
        int sleepCounter = 0;
        boolean delayed = false;
        if(LOG.isTraceEnabled()) LOG.trace("preSplit -- ENTRY region: " + region.getRegionNameAsString());

        while(!(transactionsById.isEmpty() && commitPendingTransactions.isEmpty())) {
               try {
                       delayed = true;
                       Thread.sleep(60000);
                       sleepCounter++;
                       if(LOG.isDebugEnabled()) LOG.debug("Delaying split due to transactions present. Delayed : " + sleepCounter +
                                       " minute(s) on " + region.getRegionNameAsString());
               } catch(InterruptedException e) {
                       LOG.warn("Problem while calling sleep(), " + e);
               }
        }
        if(delayed) if(LOG.isWarnEnabled()) LOG.warn("Continuing with split operation, no transactions on: " + region.getRegionNameAsString());

        if(LOG.isTraceEnabled()) LOG.trace("preSplit -- EXIT region: " + region.getRegionNameAsString());
    }

} // end of TrxRegionObserver Class
