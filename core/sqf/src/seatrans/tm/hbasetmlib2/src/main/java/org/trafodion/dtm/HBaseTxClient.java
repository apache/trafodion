// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

package org.trafodion.dtm;

import java.util.Collections;
import java.util.Set;
import java.util.HashSet;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;

import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.commons.codec.binary.Hex;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.transactional.TransactionManager;
import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.client.transactional.CommitUnsuccessfulException;
import org.apache.hadoop.hbase.client.transactional.UnsuccessfulDDLException;
import org.apache.hadoop.hbase.client.transactional.UnknownTransactionException;
import org.apache.hadoop.hbase.client.transactional.HBaseBackedTransactionLogger;
import org.apache.hadoop.hbase.client.transactional.TransactionRegionLocation;
import org.apache.hadoop.hbase.client.transactional.TransState;
import org.apache.hadoop.hbase.client.transactional.TransReturnCode;
import org.apache.hadoop.hbase.client.transactional.TransactionMap;
import org.apache.hadoop.hbase.client.transactional.TransactionalReturn;
import org.apache.hadoop.hbase.client.transactional.TmDDL;
import org.apache.hadoop.hbase.exceptions.DeserializationException;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.NotServingRegionException;
import org.apache.hadoop.hbase.ServerName;
import org.apache.hadoop.hbase.TableNotFoundException;
import org.apache.hadoop.hbase.util.Bytes;
import org.trafodion.dtm.HBaseTmZK;
import org.trafodion.dtm.TmAuditTlog;

import org.apache.hadoop.hbase.regionserver.transactional.IdTm;
import org.apache.hadoop.hbase.regionserver.transactional.IdTmException;
import org.apache.hadoop.hbase.regionserver.transactional.IdTmId;

import java.util.Map;
import java.util.HashMap;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentHashMap;

public class HBaseTxClient {

   static final Log LOG = LogFactory.getLog(HBaseTxClient.class);
   private static TmAuditTlog tLog;
   private static HBaseTmZK tmZK;
   private static RecoveryThread recovThread;
   private static TmDDL tmDDL;
   private short dtmID;
   private int stallWhere;
   private IdTm idServer;
   private static final int ID_TM_SERVER_TIMEOUT = 1000;

   public enum AlgorithmType{
     MVCC, SSCC
   }
   public AlgorithmType TRANSACTION_ALGORITHM;

   boolean useTlog;
   boolean useForgotten;
   boolean forceForgotten;
   boolean useRecovThread;
   boolean useDDLTrans;

   private static Configuration config;
   TransactionManager trxManager;
   static Map<Long, TransactionState> mapTransactionStates = TransactionMap.getInstance();
   Map<Integer, RecoveryThread> mapRecoveryThreads = new HashMap<Integer, org.trafodion.dtm.HBaseTxClient.RecoveryThread>();
   static final Object mapLock = new Object();

   void setupLog4j() {
        //System.out.println("In setupLog4J");
        System.setProperty("trafodion.root", System.getenv("MY_SQROOT"));
        String confFile = System.getenv("MY_SQROOT")
            + "/conf/log4j.dtm.config";
        PropertyConfigurator.configure(confFile);
    }

   public boolean init(String hBasePath, String zkServers, String zkPort) throws Exception {
      //System.out.println("In init - hbp");
      setupLog4j();
      if (LOG.isDebugEnabled()) LOG.debug("Enter init, hBasePath:" + hBasePath);
      if (LOG.isTraceEnabled()) LOG.trace("mapTransactionStates " + mapTransactionStates + " entries " + mapTransactionStates.size());
      config = HBaseConfiguration.create();

      config.set("hbase.zookeeper.quorum", zkServers);
      config.set("hbase.zookeeper.property.clientPort",zkPort);
      config.set("hbase.rootdir", hBasePath);
      config.set("dtmid", "0");
      this.dtmID = 0;
      this.useRecovThread = false;
      this.stallWhere = 0;

      useForgotten = true;
      try {
         String useAuditRecords = System.getenv("TM_ENABLE_FORGOTTEN_RECORDS");
         if (useAuditRecords != null) {
            useForgotten = (Integer.parseInt(useAuditRecords) != 0);
         }
      }
      catch (Exception e) {
         if (LOG.isDebugEnabled()) LOG.debug("TM_ENABLE_FORGOTTEN_RECORDS is not in ms.env");
      }
      LOG.info("useForgotten is " + useForgotten);

      forceForgotten = false;
      try {
         String forgottenForce = System.getenv("TM_TLOG_FORCE_FORGOTTEN");
         if (forgottenForce != null){
            forceForgotten = (Integer.parseInt(forgottenForce) != 0);
            if (LOG.isDebugEnabled()) LOG.debug("forgottenForce != null");
         }
      }
      catch (Exception e) {
         if (LOG.isDebugEnabled()) LOG.debug("TM_TLOG_FORCE_FORGOTTEN is not in ms.env");
      }
      LOG.info("forceForgotten is " + forceForgotten);

      useTlog = false;
      useRecovThread = false;
      try {
         String useAudit = System.getenv("TM_ENABLE_TLOG_WRITES");
         if (useAudit != null) {
            useTlog = useRecovThread = (Integer.parseInt(useAudit) != 0);
         }
      }
      catch (Exception e) {
         if (LOG.isDebugEnabled()) LOG.debug("TM_ENABLE_TLOG_WRITES is not in ms.env");
      }

      if (useTlog) {
         try {
            tLog = new TmAuditTlog(config);
         } catch (Exception e ){
            LOG.error("Unable to create TmAuditTlog, throwing exception");
            throw new RuntimeException(e);
         }
      }
      try {
        trxManager = TransactionManager.getInstance(config);
      } catch (IOException e ){
          LOG.error("Unable to create TransactionManager, throwing exception");
          throw new RuntimeException(e);
      }

      if (useRecovThread) {
         if (LOG.isDebugEnabled()) LOG.debug("Starting recovery thread for tm ID: " + dtmID);
          try {
              tmZK = new HBaseTmZK(config, dtmID);
          }catch (IOException e ){
              LOG.error("Unable to create HBaseTmZK TM-zookeeper class, throwing exception");
              throw new RuntimeException(e);
          }
          recovThread = new RecoveryThread(tLog, tmZK, trxManager);
          recovThread.start();
      }
      if (LOG.isDebugEnabled()) LOG.debug("Exit init(String, String, String)");
      return true;
   }

   public boolean init(short dtmid) throws Exception {
      //System.out.println("In init - dtmId" + dtmid);

      setupLog4j();
      if (LOG.isDebugEnabled()) LOG.debug("Enter init(" + dtmid + ")");
      config = HBaseConfiguration.create();
      config.set("hbase.hregion.impl", "org.apache.hadoop.hbase.regionserver.transactional.TransactionalRegion");
      config.set("hbase.hlog.splitter.impl", "org.apache.hadoop.hbase.regionserver.transactional.THLogSplitter");
      config.set("dtmid", String.valueOf(dtmid));
      config.set("CONTROL_POINT_TABLE_NAME", "TRAFODION._DTM_.TLOG" + String.valueOf(dtmid) + "_CONTROL_POINT");
      config.set("TLOG_TABLE_NAME", "TRAFODION._DTM_.TLOG" + String.valueOf(dtmid));

      this.dtmID = dtmid;
      this.useRecovThread = false;
      this.stallWhere = 0;
      this.useDDLTrans = false;

      String useSSCC = System.getenv("TM_USE_SSCC");
      TRANSACTION_ALGORITHM = AlgorithmType.MVCC;
      if (useSSCC != null)
         TRANSACTION_ALGORITHM = (Integer.parseInt(useSSCC) == 1) ? AlgorithmType.SSCC :AlgorithmType.MVCC ;

      try {
         idServer = new IdTm(false);
      }
      catch (Exception e){
         LOG.error("Exception creating new IdTm: " + e);
      }

      try {
         String useDDLTransactions = System.getenv("TM_ENABLE_DDL_TRANS");
         if (useDDLTransactions != null) {
             useDDLTrans = (Integer.parseInt(useDDLTransactions) != 0);
         }
      }
      catch (Exception e) {
         if (LOG.isDebugEnabled()) LOG.debug("TM_ENABLE_DDL_TRANS is not in ms.env");
      }

      if(useDDLTrans){
         try {
            tmDDL = new TmDDL(config);
         }
         catch (Exception e) {
            LOG.error("Unable to create TmDDL, throwing exception " + e);
            e.printStackTrace();
            throw new RuntimeException(e);
         }
      }

      useForgotten = true;
      try {
         String useAuditRecords = System.getenv("TM_ENABLE_FORGOTTEN_RECORDS");
         if (useAuditRecords != null) {
            useForgotten = (Integer.parseInt(useAuditRecords) != 0);
         }
      }
      catch (Exception e) {
         if (LOG.isDebugEnabled()) LOG.debug("TM_ENABLE_FORGOTTEN_RECORDS is not in ms.env");
      }
      LOG.info("useForgotten is " + useForgotten);

      forceForgotten = false;
      try {
         String forgottenForce = System.getenv("TM_TLOG_FORCE_FORGOTTEN");
         if (forgottenForce != null){
            forceForgotten = (Integer.parseInt(forgottenForce) != 0);
            if (LOG.isDebugEnabled()) LOG.debug("forgottenForce != null");
         }
      }
      catch (Exception e) {
         if (LOG.isDebugEnabled()) LOG.debug("TM_TLOG_FORCE_FORGOTTEN is not in ms.env");
      }
      LOG.info("forceForgotten is " + forceForgotten);

      useTlog = false;
      useRecovThread = false;
      try {
         String useAudit = System.getenv("TM_ENABLE_TLOG_WRITES");
         if (useAudit != null){
            useTlog = useRecovThread = (Integer.parseInt(useAudit) != 0);
         }
      }
      catch (Exception e) {
         if (LOG.isDebugEnabled()) LOG.debug("TM_ENABLE_TLOG_WRITES is not in ms.env");
      }
      if (useTlog) {
         try {
            tLog = new TmAuditTlog(config);
         } catch (Exception e ){
            LOG.error("Unable to create TmAuditTlog, throwing exception " + e);
            e.printStackTrace();
            throw new RuntimeException(e);
         }
      }
      try {
          trxManager = TransactionManager.getInstance(config);
      } catch (IOException e ){
            LOG.error("Unable to create TransactionManager, Exception: " + e + "throwing new RuntimeException");
            throw new RuntimeException(e);
      }

      if(useDDLTrans)
          trxManager.init(tmDDL);

      if (useRecovThread) {
         if (LOG.isDebugEnabled()) LOG.debug("Entering recovThread Usage");
          try {
              tmZK = new HBaseTmZK(config, dtmID);
          }catch (IOException e ){
              LOG.error("Unable to create HBaseTmZK TM-zookeeper class, throwing exception");
              throw new RuntimeException(e);
          }
          recovThread = new RecoveryThread(tLog,
                                           tmZK,
                                           trxManager,
                                           this,
                                           useForgotten,
                                           forceForgotten,
                                           useTlog);
          recovThread.start();
      }
      if (LOG.isTraceEnabled()) LOG.trace("Exit init()");
      return true;
   }

   public TmDDL getTmDDL() {
        return tmDDL;
   }

   public void nodeDown(int nodeID) throws IOException {
       if(LOG.isTraceEnabled()) LOG.trace("nodeDown -- ENTRY node ID: " + nodeID);

       RecoveryThread newRecovThread;
       if(dtmID == nodeID)
           throw new IOException("Down node ID is the same as current dtmID, Incorrect parameter");

       try {
           if(mapRecoveryThreads.containsKey(nodeID)) {
               if(LOG.isDebugEnabled()) LOG.debug("nodeDown called on a node that already has RecoveryThread running node ID: " + nodeID);
           }
           else {
               newRecovThread = new RecoveryThread(tLog,
                                                   new HBaseTmZK(config, (short) nodeID),
                                                   trxManager,
                                                   this,
                                                   useForgotten,
                                                   forceForgotten,
                                                   useTlog);
               newRecovThread.start();
               mapRecoveryThreads.put(nodeID, recovThread);
               if(LOG.isTraceEnabled()) LOG.trace("nodeDown -- mapRecoveryThreads size: " + mapRecoveryThreads.size());
           }
       }
       catch(Exception e) {
           LOG.error("Unable to create rescue recovery thread for TM" + dtmID);
       }
       if(LOG.isTraceEnabled()) LOG.trace("nodeDown -- EXIT node ID: " + nodeID);
   }

   public void nodeUp(int nodeID) throws IOException {
       if(LOG.isTraceEnabled()) LOG.trace("nodeUp -- ENTRY node ID: " + nodeID);
       RecoveryThread rt = mapRecoveryThreads.get(nodeID);
       if(rt == null) {
           if(LOG.isWarnEnabled()) LOG.warn("nodeUp called on a node that has RecoveryThread removed already, node ID: " + nodeID);
           if(LOG.isTraceEnabled()) LOG.trace("nodeUp -- EXIT node ID: " + nodeID);
           return;
       }
       rt.stopThread();
       try {
           rt.join();
       } catch (Exception e) { LOG.warn("Problem while waiting for the recovery thread to stop for node ID: " + nodeID); }
       mapRecoveryThreads.remove(nodeID);
       if(LOG.isTraceEnabled()) LOG.trace("nodeUp -- mapRecoveryThreads size: " + mapRecoveryThreads.size());
       if(LOG.isTraceEnabled()) LOG.trace("nodeUp -- EXIT node ID: " + nodeID);
   }

   public short stall (int where) {
      if (LOG.isDebugEnabled()) LOG.debug("Entering stall with parameter " + where);
      this.stallWhere = where;
      return TransReturnCode.RET_OK.getShort();
   }

   public long beginTransaction(final long transactionId) throws Exception {

      if (LOG.isTraceEnabled()) LOG.trace("Enter beginTransaction, txid: " + transactionId);
      TransactionState tx = trxManager.beginTransaction(transactionId);
      if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient:beginTransaction new transactionState created: " + tx);
      if(tx == null) {
         LOG.error("null Transaction State returned by the Transaction Manager, txid: " + transactionId);
         throw new Exception("TransactionState is null");
      }

      synchronized(mapLock) {
         TransactionState tx2 = mapTransactionStates.get(transactionId);
         if (tx2 != null) {
            // Some other thread added the transaction while we were creating one.  It's already in the
            // map, so we can use the existing one.
            if (LOG.isDebugEnabled()) LOG.debug("HBaseTxClient:beginTransaction, found TransactionState object while creating a new one " + tx2);
            tx = tx2;
         }
         else {
            if (LOG.isDebugEnabled()) LOG.debug("HBaseTxClient:beginTransaction, adding new TransactionState to map " + tx);
            mapTransactionStates.put(transactionId, tx);
         }
      }

      if (LOG.isDebugEnabled()) LOG.debug("Exit beginTransaction, Transaction State: " + tx + " mapsize: " + mapTransactionStates.size());
      return transactionId;
   }

   public short abortTransaction(final long transactionID) throws Exception {
      if (LOG.isDebugEnabled()) LOG.debug("Enter abortTransaction, txid: " + transactionID);
      TransactionState ts = mapTransactionStates.get(transactionID);

      if(ts == null) {
          LOG.error("Returning from HBaseTxClient:abortTransaction, txid: " + transactionID + " retval: " + TransReturnCode.RET_NOTX.toString());
          return TransReturnCode.RET_NOTX.getShort();
      }

      try {
         ts.setStatus(TransState.STATE_ABORTED);
         if (useTlog) {
            tLog.putSingleRecord(transactionID, -1, "ABORTED", ts.getParticipatingRegions(), false);
         }
      } catch(Exception e) {
         LOG.error("Returning from HBaseTxClient:abortTransaction, txid: " + transactionID + " tLog.putRecord: EXCEPTION");
         return TransReturnCode.RET_EXCEPTION.getShort();
      }

      if ((stallWhere == 1) || (stallWhere == 3)) {
         LOG.info("Stalling in phase 2 for abortTransaction");
         Thread.sleep(300000); // Initially set to run every 5 min
      }

      try {
         trxManager.abort(ts);
      } catch(IOException e) {
          synchronized(mapLock) {
             mapTransactionStates.remove(transactionID);
          }
          LOG.error("Returning from HBaseTxClient:abortTransaction, txid: " + transactionID + " retval: EXCEPTION");
          return TransReturnCode.RET_EXCEPTION.getShort();
      }
      catch (UnsuccessfulDDLException ddle) {
          LOG.error("FATAL DDL Exception from HBaseTxClient:abort, WAITING INDEFINETLY !! retval: " + TransReturnCode.RET_EXCEPTION.toString() + " UnsuccessfulDDLException" + " txid: " + transactionID);

          //Reaching here means several attempts to perform the DDL operation has failed in abort phase.
          //Generally if only DML operation is involved, returning error causes TM to call completeRequest()
          //which causes a hang(abort work is outstanding forever) due to doAbortX thread holding the
          //commitSendLock (since doAbortX raised an exception and exited without clearing the commitSendLock count).
          //In the case of DDL exception, no doAbortX thread is involved and commitSendLock is not held. Hence to mimic
          //the same hang behaviour, the current worker thread will be put to wait indefinitely for user intervention.
          //Long Term solution to this behaviour is currently TODO.
          Object commitDDLLock = new Object();
          synchronized(commitDDLLock)
          {
            commitDDLLock.wait();
          }
          return TransReturnCode.RET_EXCEPTION.getShort();
      }
      if (useTlog && useForgotten) {
         if (forceForgotten) {
            tLog.putSingleRecord(transactionID, -1, "FORGOTTEN", null, true);
         }
         else {
            tLog.putSingleRecord(transactionID, -1, "FORGOTTEN", null, false);
         }
      }
 //     mapTransactionStates.remove(transactionID);

      if (LOG.isTraceEnabled()) LOG.trace("Exit abortTransaction, retval: OK txid: " + transactionID + " mapsize: " + mapTransactionStates.size());
      return TransReturnCode.RET_OK.getShort();
   }

   public short prepareCommit(long transactionId) throws Exception {
     if (LOG.isDebugEnabled()) LOG.debug("Enter prepareCommit, txid: " + transactionId);
     if (LOG.isTraceEnabled()) LOG.trace("mapTransactionStates " + mapTransactionStates + " entries " + mapTransactionStates.size());
        TransactionState ts = mapTransactionStates.get(transactionId);
     if(ts == null) {
       LOG.error("Returning from HBaseTxClient:prepareCommit, txid: " + transactionId + " retval: " + TransReturnCode.RET_NOTX.toString());
       return TransReturnCode.RET_NOTX.getShort();
     }

     try {
        short result = (short) trxManager.prepareCommit(ts);
        if (LOG.isDebugEnabled()) LOG.debug("prepareCommit, [ " + ts + " ], result " + result + ((result == TransactionalReturn.COMMIT_OK_READ_ONLY)?", Read-Only":""));
        switch (result) {
          case TransactionalReturn.COMMIT_OK:
             if (LOG.isTraceEnabled()) LOG.trace("Exit OK prepareCommit, txid: " + transactionId);
             return TransReturnCode.RET_OK.getShort();
          case TransactionalReturn.COMMIT_OK_READ_ONLY:
             synchronized(mapLock) {
                mapTransactionStates.remove(transactionId);
             }
             if (LOG.isTraceEnabled()) LOG.trace("Exit OK_READ_ONLY prepareCommit, txid: " + transactionId);
             return TransReturnCode.RET_READONLY.getShort();
          case TransactionalReturn.COMMIT_UNSUCCESSFUL:
             LOG.info("Exit RET_EXCEPTION prepareCommit, txid: " + transactionId);
             return TransReturnCode.RET_EXCEPTION.getShort();
          case TransactionalReturn.COMMIT_CONFLICT:
             LOG.info("Exit RET_HASCONFLICT prepareCommit, txid: " + transactionId);
             return TransReturnCode.RET_HASCONFLICT.getShort();
          default:
             LOG.info("Exit default RET_EXCEPTION prepareCommit, txid: " + transactionId);
             return TransReturnCode.RET_EXCEPTION.getShort();
        }
     } catch (CommitUnsuccessfulException e) {
       LOG.error("Returning from HBaseTxClient:prepareCommit, txid: " + transactionId + " retval: " + TransReturnCode.RET_NOCOMMITEX.toString() + " CommitUnsuccessfulException");
       return TransReturnCode.RET_NOCOMMITEX.getShort();
     }
     catch (IOException e) {
       LOG.error("Returning from HBaseTxClient:prepareCommit, txid: " + transactionId + " retval: " + TransReturnCode.RET_IOEXCEPTION.toString() + " IOException");
       return TransReturnCode.RET_IOEXCEPTION.getShort();
     }
     catch (Exception e) {
           LOG.error("Returning from HBaseTxClient:prepareCommit, txid: " + transactionId + " retval: " + TransReturnCode.RET_NOCOMMITEX.toString() + " Exception " + e);
           return TransReturnCode.RET_NOCOMMITEX.getShort();
     }
   }

   public short doCommit(long transactionId) throws Exception {
       if (LOG.isDebugEnabled()) LOG.debug("Enter doCommit, txid: " + transactionId);
       TransactionState ts = mapTransactionStates.get(transactionId);

       if(ts == null) {
      LOG.error("Returning from HBaseTxClient:doCommit, (null tx) retval: " + TransReturnCode.RET_NOTX.toString() + " txid: " + transactionId);
          return TransReturnCode.RET_NOTX.getShort();
       }

       // Set the commitId
       IdTmId commitId = null;
       if (TRANSACTION_ALGORITHM == AlgorithmType.SSCC) {
          try {
             commitId = new IdTmId();
             if (LOG.isTraceEnabled()) LOG.trace("doCommit getting new commitId");
             idServer.id(ID_TM_SERVER_TIMEOUT, commitId);
             if (LOG.isTraceEnabled()) LOG.trace("doCommit idServer.id returned: " + commitId.val);
          } catch (IdTmException exc) {
             LOG.error("doCommit: IdTm threw exception " + exc);
             throw new CommitUnsuccessfulException("doCommit: IdTm threw exception " + exc);
          }
       }

       final long commitIdVal = (TRANSACTION_ALGORITHM == AlgorithmType.SSCC) ? commitId.val : -1;
       if (LOG.isTraceEnabled()) LOG.trace("doCommit setting commitId (" + commitIdVal + ") for tx: " + ts.getTransactionId());
       ts.setCommitId(commitIdVal);

       try {
          ts.setStatus(TransState.STATE_COMMITTED);
          if (useTlog) {
             tLog.putSingleRecord(transactionId, commitIdVal, "COMMITTED", ts.getParticipatingRegions(), true);
          }
       } catch(Exception e) {
          LOG.error("Returning from HBaseTxClient:doCommit, txid: " + transactionId + " tLog.putRecord: EXCEPTION " + e);
          return TransReturnCode.RET_EXCEPTION.getShort();
       }

       if ((stallWhere == 2) || (stallWhere == 3)) {
          LOG.info("Stalling in phase 2 for doCommit");
          Thread.sleep(300000); // Initially set to run every 5 min
       }

       try {
          trxManager.doCommit(ts);
       } catch (CommitUnsuccessfulException e) {
          LOG.error("Returning from HBaseTxClient:doCommit, retval: " + TransReturnCode.RET_EXCEPTION.toString() + " IOException" + " txid: " + transactionId);
          return TransReturnCode.RET_EXCEPTION.getShort();
       }
       catch (UnsuccessfulDDLException ddle) {
          LOG.error("FATAL DDL Exception from HBaseTxClient:doCommit, WAITING INDEFINETLY !! retval: " + TransReturnCode.RET_EXCEPTION.toString() + " UnsuccessfulDDLException" + " txid: " + transactionId);

          //Reaching here means several attempts to perform the DDL operation has failed in commit phase.
          //Generally if only DML operation is involved, returning error causes TM to call completeRequest()
          //which causes a hang(commit work is outstanding forever) due to doCommitX thread holding the
          //commitSendLock (since doCommitX raised an exception and exited without clearing the commitSendLock count).
          //In the case of DDL exception, no doCommitX thread is involved and commitSendLock is not held. Hence to mimic
          //the same hang behaviour, the current worker thread will be put to wait indefinitely for user intervention.
          //Long Term solution to this behaviour is currently TODO.
          Object commitDDLLock = new Object();
          synchronized(commitDDLLock)
          {
            commitDDLLock.wait();
          }
          return TransReturnCode.RET_EXCEPTION.getShort();
       }
       if (useTlog && useForgotten) {
          if (forceForgotten) {
             tLog.putSingleRecord(transactionId, commitIdVal, "FORGOTTEN", null, true);
          }
          else {
             tLog.putSingleRecord(transactionId, commitIdVal, "FORGOTTEN", null, false);
          }
       }
//       mapTransactionStates.remove(transactionId);

       if (LOG.isTraceEnabled()) LOG.trace("Exit doCommit, retval(ok): " + TransReturnCode.RET_OK.toString() +
                         " txid: " + transactionId + " mapsize: " + mapTransactionStates.size());

       return TransReturnCode.RET_OK.getShort();
   }

   public short completeRequest(long transactionId) throws Exception {
     if (LOG.isDebugEnabled()) LOG.debug("Enter completeRequest, txid: " + transactionId);
     TransactionState ts = mapTransactionStates.get(transactionId);

     if(ts == null) {
          LOG.error("Returning from HBaseTxClient:completeRequest, (null tx) retval: " + TransReturnCode.RET_NOTX.toString() + " txid: " + transactionId);
          return TransReturnCode.RET_NOTX.getShort();
       }

       try {

       if (LOG.isTraceEnabled()) LOG.trace("TEMP completeRequest Calling CompleteRequest() Txid :" + transactionId);

          ts.completeRequest();
       } catch(Exception e) {
          LOG.error("Returning from HBaseTxClient:completeRequest, ts.completeRequest: txid: " + transactionId + ", EXCEPTION: " + e);
       throw new Exception("Exception during completeRequest, unable to commit.  Exception: " + e);
       }

     synchronized(mapLock) {
        mapTransactionStates.remove(transactionId);
     }

     if (LOG.isDebugEnabled()) LOG.debug("Exit completeRequest txid: " + transactionId + " mapsize: " + mapTransactionStates.size());
     return TransReturnCode.RET_OK.getShort();
   }

   public short tryCommit(long transactionId) throws Exception {
     if (LOG.isDebugEnabled()) LOG.debug("Enter tryCommit, txid: " + transactionId);
     short err, commitErr, abortErr = TransReturnCode.RET_OK.getShort();

     try {
       err = prepareCommit(transactionId);
       if (err != TransReturnCode.RET_OK.getShort()) {
         if (LOG.isDebugEnabled()) LOG.debug("tryCommit prepare failed with error " + err);
         return err;
       }
       commitErr = doCommit(transactionId);
       if (commitErr != TransReturnCode.RET_OK.getShort()) {
         LOG.error("doCommit for committed transaction " + transactionId + " failed with error " + commitErr);
         // It is a violation of 2 PC protocol to try to abort the transaction after prepare
         return commitErr;
//         abortErr = abortTransaction(transactionId);
//         if (LOG.isDebugEnabled()) LOG.debug("tryCommit commit failed and was aborted. Commit error " +
//                   commitErr + ", Abort error " + abortErr);
       }

       if (LOG.isTraceEnabled()) LOG.trace("TEMP tryCommit Calling CompleteRequest() Txid :" + transactionId);

       err = completeRequest(transactionId);
       if (err != TransReturnCode.RET_OK.getShort()){
         if (LOG.isDebugEnabled()) LOG.debug("tryCommit completeRequest for transaction " + transactionId + " failed with error " + err);
       }
     } catch(Exception e) {
       mapTransactionStates.remove(transactionId);
       LOG.error("Returning from HBaseTxClient:tryCommit, ts: EXCEPTION" + " txid: " + transactionId);
       throw new Exception("Exception " + e + "during tryCommit, unable to commit.");
    }

    synchronized(mapLock) {
       mapTransactionStates.remove(transactionId);
    }

    if (LOG.isDebugEnabled()) LOG.debug("Exit completeRequest txid: " + transactionId + " mapsize: " + mapTransactionStates.size());
    return TransReturnCode.RET_OK.getShort();
  }

   public short callCreateTable(long transactionId, byte[] pv_htbldesc, Object[]  beginEndKeys) throws Exception
   {
      TransactionState ts;
      HTableDescriptor htdesc;

      if (LOG.isTraceEnabled()) LOG.trace("Enter callCreateTable, txid: [" + transactionId + "],  htbldesc bytearray: " + pv_htbldesc + "desc in hex: " + Hex.encodeHexString(pv_htbldesc));

      ts = mapTransactionStates.get(transactionId);
      if(ts == null) {
         LOG.error("Returning from HBaseTxClient:callCreateTable, (null tx) retval: " + TransReturnCode.RET_NOTX.getShort()  + " txid: " + transactionId);
         return TransReturnCode.RET_NOTX.getShort();
      }

      try {
         htdesc = HTableDescriptor.parseFrom(pv_htbldesc);
      }
      catch(Exception e) {
         if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient:callCreateTable exception in htdesc parseFrom, retval: " +
            TransReturnCode.RET_EXCEPTION.toString() +
            " txid: " + transactionId +
            " DeserializationException: " + e);
         StringWriter sw = new StringWriter();
         PrintWriter pw = new PrintWriter(sw);
         e.printStackTrace(pw);
         LOG.error(sw.toString());

         throw new Exception("DeserializationException in callCreateTable parseFrom, unable to send callCreateTable");
      }

      try {
         trxManager.createTable(ts, htdesc, beginEndKeys);
      }
      catch (Exception cte) {
         if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient:callCreateTable exception trxManager.createTable, retval: " +
            TransReturnCode.RET_EXCEPTION.toString() +" txid: " + transactionId +" Exception: " + cte);
         StringWriter sw = new StringWriter();
         PrintWriter pw = new PrintWriter(sw);
         cte.printStackTrace(pw);
         LOG.error("HBaseTxClient createTable call error: " + sw.toString());

         throw new Exception("createTable call error");
      }
      return TransReturnCode.RET_OK.getShort();
   }

   public short callAlterTable(long transactionId, byte[] pv_tblname, Object[] tableOptions) throws Exception
   {
      TransactionState ts;
      String strTblName = new String(pv_tblname, "UTF-8");

      if (LOG.isTraceEnabled()) LOG.trace("Enter callAlterTable, txid: [" + transactionId + "],  tableName: " + strTblName);

      ts = mapTransactionStates.get(transactionId);
      if(ts == null) {
         LOG.error("Returning from HBaseTxClient:callAlterTable, (null tx) retval: " + TransReturnCode.RET_NOTX.getShort()  + " txid: " + transactionId);
         return TransReturnCode.RET_NOTX.getShort();
      }

      try {
         trxManager.alterTable(ts, strTblName, tableOptions);
      }
      catch (Exception cte) {
         if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient:callAlterTable exception trxManager.alterTable, retval: " +
            TransReturnCode.RET_EXCEPTION.toString() +" txid: " + transactionId +" Exception: " + cte);
         StringWriter sw = new StringWriter();
         PrintWriter pw = new PrintWriter(sw);
         cte.printStackTrace(pw);
         LOG.error("HBaseTxClient alterTable call error: " + sw.toString());

         throw new Exception("alterTable call error");
      }
      return TransReturnCode.RET_OK.getShort();
   }

   public short callRegisterTruncateOnAbort(long transactionId, byte[] pv_tblname) throws Exception
   {
      TransactionState ts;
      String strTblName = new String(pv_tblname, "UTF-8");

      if (LOG.isTraceEnabled()) LOG.trace("Enter callRegisterTruncateOnAbort, txid: [" + transactionId + "],  tablename: " + strTblName);

      ts = mapTransactionStates.get(transactionId);
      if(ts == null) {
         LOG.error("Returning from HBaseTxClient:callRegisterTruncateOnAbort, (null tx) retval: " + TransReturnCode.RET_NOTX.getShort()  + " txid: " + transactionId);
         return TransReturnCode.RET_NOTX.getShort();
      }

      try {
         trxManager.registerTruncateOnAbort(ts, strTblName);
      }
      catch (Exception e) {
         if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient:callRegisterTruncateOnAbort exception trxManager.registerTruncateOnAbort, retval: " +
            TransReturnCode.RET_EXCEPTION.toString() +" txid: " + transactionId +" Exception: " + e);
         StringWriter sw = new StringWriter();
         PrintWriter pw = new PrintWriter(sw);
         e.printStackTrace(pw);
         String msg = "HBaseTxClient registerTruncateOnAbort call error ";
         LOG.error(msg + " : " + sw.toString());
         throw new Exception(msg);
      }
      return TransReturnCode.RET_OK.getShort();
   }

   public short callDropTable(long transactionId, byte[] pv_tblname) throws Exception
   {
      TransactionState ts;
      String strTblName = new String(pv_tblname, "UTF-8");

      if (LOG.isTraceEnabled()) LOG.trace("Enter callDropTable, txid: [" + transactionId + "],  tablename: " + strTblName);

      ts = mapTransactionStates.get(transactionId);
      if(ts == null) {
         LOG.error("Returning from HBaseTxClient:callDropTable, (null tx) retval: " + TransReturnCode.RET_NOTX.getShort()  + " txid: " + transactionId);
         return TransReturnCode.RET_NOTX.getShort();
      }

      try {
         trxManager.dropTable(ts, strTblName);
      }
      catch (Exception cte) {
         if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient:callDropTable exception trxManager.dropTable, retval: " +
            TransReturnCode.RET_EXCEPTION.toString() +" txid: " + transactionId +" Exception: " + cte);
         StringWriter sw = new StringWriter();
         PrintWriter pw = new PrintWriter(sw);
         cte.printStackTrace(pw);
         LOG.error("HBaseTxClient dropTable call error: " + sw.toString());
      }
      return TransReturnCode.RET_OK.getShort();
   }

    public short callRegisterRegion(long transactionId,
                                    long startId,
                                    int  pv_port,
                                    byte[] pv_hostname,
                                    long pv_startcode,
                                    byte[] pv_regionInfo) throws Exception {
       String hostname    = new String(pv_hostname);
       if (LOG.isTraceEnabled()) LOG.trace("Enter callRegisterRegion, txid: [" + transactionId + "], startId: " + startId + ", port: "
           + pv_port + ", hostname: " + hostname + ", reg info len: " + pv_regionInfo.length + " " + new String(pv_regionInfo, "UTF-8"));

       HRegionInfo lv_regionInfo;
       try {
          lv_regionInfo = HRegionInfo.parseFrom(pv_regionInfo);
       }
       catch (Exception de) {
          if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient:callRegisterRegion exception in lv_regionInfo parseFrom, retval: " +
             TransReturnCode.RET_EXCEPTION.toString() + " txid: " + transactionId + " DeserializationException: " + de);
          StringWriter sw = new StringWriter();
          PrintWriter pw = new PrintWriter(sw);
          de.printStackTrace(pw);
          LOG.error(sw.toString());

          throw new Exception("DeserializationException in lv_regionInfo parseFrom, unable to register region");
       }

       // TODO Not in CDH 5.1       ServerName lv_servername = ServerName.valueOf(hostname, pv_port, pv_startcode);
       String lv_hostname_port_string = hostname + ":" + pv_port;
       String lv_servername_string = ServerName.getServerName(lv_hostname_port_string, pv_startcode);
       ServerName lv_servername = ServerName.parseServerName(lv_servername_string);
       TransactionRegionLocation regionLocation = new TransactionRegionLocation(lv_regionInfo, lv_servername);
       String regionTableName = regionLocation.getRegionInfo().getTable().getNameAsString();

       TransactionState ts = mapTransactionStates.get(transactionId);
       if(ts == null) {
          if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient:callRegisterRegion transactionId (" + transactionId +
                   ") not found in mapTransactionStates of size: " + mapTransactionStates.size());
          try {
             ts = trxManager.beginTransaction(transactionId);
          }
          catch (IdTmException exc) {
             LOG.error("HBaseTxClient: beginTransaction for tx (" + transactionId + ") caught exception " + exc);
             throw new IdTmException("HBaseTxClient: beginTransaction for tx (" + transactionId + ") caught exception " + exc);
          }
          synchronized (mapLock) {
             TransactionState ts2 = mapTransactionStates.get(transactionId);
             if (ts2 != null) {
                // Some other thread added the transaction while we were creating one.  It's already in the
                // map, so we can use the existing one.
                if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient:callRegisterRegion, found TransactionState object while creating a new one " + ts2);
                ts = ts2;
             }
             else {
                ts.setStartId(startId);
                if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient:callRegisterRegion new transactionState created: " + ts );
             }
          }// end synchronized
       }
       else {
          if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient:callRegisterRegion existing transactionState found: " + ts );
          if (ts.getStartId() == -1) {
            ts.setStartId(startId);
            if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient:callRegisterRegion reset startId for transactionState: " + ts );
          }
       }

       try {
          trxManager.registerRegion(ts, regionLocation);
       } catch (IOException e) {
          LOG.error("HBaseTxClient:callRegisterRegion exception in registerRegion call, txid: " + transactionId +
            " retval: " + TransReturnCode.RET_EXCEPTION.toString() + " IOException " + e);
          return TransReturnCode.RET_EXCEPTION.getShort();
       }

       if (LOG.isDebugEnabled()) LOG.debug("RegisterRegion adding table name " + regionTableName);
       ts.addTableName(regionTableName);

       // Removing unnecessary put back into the map
       // mapTransactionStates.put(ts.getTransactionId(), ts);

       if (LOG.isTraceEnabled()) LOG.trace("Exit callRegisterRegion, txid: [" + transactionId + "] with mapsize: "
                  + mapTransactionStates.size());
       return TransReturnCode.RET_OK.getShort();
   }

   public int participatingRegions(long transactionId) throws Exception {
       if (LOG.isTraceEnabled()) LOG.trace("Enter participatingRegions, txid: " + transactionId);
       TransactionState ts = mapTransactionStates.get(transactionId);
       if(ts == null) {
         if (LOG.isTraceEnabled()) LOG.trace("Returning from HBaseTxClient:participatingRegions, txid: " + transactionId + " not found returning: 0");
          return 0;
       }
       int participants = ts.getParticipantCount() - ts.getRegionsToIgnoreCount();
       if (LOG.isTraceEnabled()) LOG.trace("Exit participatingRegions , txid: [" + transactionId + "] " + participants + " participants" +
                                            "hasDDL Operation: " + ts.hasDDLTx());
    
       //In some scenarios, it is possible only DDL operation is performed
       //within a transaction, example initialize trafodion, drop; In this
       //scenario, region participation is zero. For the prepareCommit to
       //continue to doCommit, there needs to be atleast one participant.
       if(participants == 0 && ts.hasDDLTx())
           participants++;

       //return (ts.getParticipantCount() - ts.getRegionsToIgnoreCount());
       return participants;
   }

   public long addControlPoint() throws Exception {
      if (LOG.isTraceEnabled()) LOG.trace("Enter addControlPoint");
      long result = 0L;
      try {
         if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient calling tLog.addControlPoint with mapsize " + mapTransactionStates.size());
         result = tLog.addControlPoint(mapTransactionStates);
      }
      catch(IOException e){
          LOG.error("addControlPoint IOException " + e);
          throw e;
      }
      Long lowestStartId = Long.MAX_VALUE;
      for(ConcurrentHashMap.Entry<Long, TransactionState> entry : mapTransactionStates.entrySet()){
          TransactionState value;
          value = entry.getValue();
          long ts = value.getStartId();
          if( ts < lowestStartId) lowestStartId = ts;
      }
      if(lowestStartId < Long.MAX_VALUE)
      {
          tmZK.createGCzNode(Bytes.toBytes(lowestStartId));
      }
      if (LOG.isTraceEnabled()) LOG.trace("Exit addControlPoint, returning: " + result);
      return result;
   }

     /**
      * Thread to gather recovery information for regions that need to be recovered 
      */
     private static class RecoveryThread extends Thread{
             final int SLEEP_DELAY = 1000; // Initially set to run every 1sec
             private int sleepTimeInt = 0;
             private boolean skipSleep = false;
             private TmAuditTlog audit;
             private HBaseTmZK zookeeper;
             private TransactionManager txnManager;
             private short tmID;
             private Set<Long> inDoubtList;
             private boolean continueThread = true;
             private int recoveryIterations = -1;
             private int retryCount = 0;
             private boolean useForgotten;
             private boolean forceForgotten;
             private boolean useTlog;
             HBaseTxClient hbtx;

         public RecoveryThread(TmAuditTlog audit,
                               HBaseTmZK zookeeper,
                               TransactionManager txnManager,
                               HBaseTxClient hbtx,
                               boolean useForgotten,
                               boolean forceForgotten,
                               boolean useTlog) {
             this(audit, zookeeper, txnManager);
             this.hbtx = hbtx;
             this.useForgotten = useForgotten;
             this.forceForgotten = forceForgotten;
             this.useTlog= useTlog;
         }
             /**
              *
              * @param audit
              * @param zookeeper
              * @param txnManager
              */
             public RecoveryThread(TmAuditTlog audit,
                                   HBaseTmZK zookeeper,
                                   TransactionManager txnManager)
             {
                          this.audit = audit;
                          this.zookeeper = zookeeper;
                          this.txnManager = txnManager;
                          this.inDoubtList = new HashSet<Long> ();
                          this.tmID = zookeeper.getTMID();

                          String sleepTime = System.getenv("TMRECOV_SLEEP");
                          if (sleepTime != null) {
                                this.sleepTimeInt = Integer.parseInt(sleepTime);
                                if(LOG.isDebugEnabled()) LOG.debug("Recovery thread sleep set to: " +
                                                                   this.sleepTimeInt + "ms");
                          }
             }

             public void stopThread() {
                 this.continueThread = false;
             }

             private void addRegionToTS(String hostnamePort, byte[] regionInfo, TransactionState ts) throws Exception{
                 HRegionInfo regionInfoLoc; // = new HRegionInfo();
                 final byte [] delimiter = ",".getBytes();
                 String[] result = hostnamePort.split(new String(delimiter), 3);

                 if (result.length < 2)
                         throw new IllegalArgumentException("Region array format is incorrect");

                 String hostname = result[0];
                 int port = Integer.parseInt(result[1]);
                 try {
                                 regionInfoLoc = HRegionInfo.parseFrom(regionInfo);
                 }
                 catch(Exception e) {
                                 LOG.error("Unable to parse region byte array, " + e);
                                 throw e;
                 }
                 /*
                 ByteArrayInputStream lv_bis = new ByteArrayInputStream(regionInfo);
                 DataInputStream lv_dis = new DataInputStream(lv_bis);
                 try {
                         regionInfoLoc.readFields(lv_dis);
                 } catch (Exception e) {
                         throw new Exception();
                 }
                 */
                 //HBase98 TODO: need to set the value of startcode correctly
                 //HBase98 TODO: Not in CDH 5.1:  ServerName lv_servername = ServerName.valueOf(hostname, port, 0);

                 String lv_hostname_port_string = hostname + ":" + port;
                 String lv_servername_string = ServerName.getServerName(lv_hostname_port_string, 0);
                 ServerName lv_servername = ServerName.parseServerName(lv_servername_string);

                 TransactionRegionLocation loc = new TransactionRegionLocation(regionInfoLoc, lv_servername);
                 ts.addRegion(loc);
             }

            @Override
             public void run() {

                while (this.continueThread) {
                    try {
                        skipSleep = false;
                        Map<String, byte[]> regions = null;
                        Map<Long, TransactionState> transactionStates =
                                new HashMap<Long, TransactionState>();
                        try {
                            regions = zookeeper.checkForRecovery();
                        } catch (Exception e) {
                            if (regions != null) { // ignore no object returned by zookeeper.checkForRecovery
                               LOG.error("An ERROR occurred while checking for regions to recover. " + "TM: " + tmID);
                               StringWriter sw = new StringWriter();
                               PrintWriter pw = new PrintWriter(sw);
                               e.printStackTrace(pw);
                               LOG.error(sw.toString());
                            }
                        }

                        if(regions != null) {
                            skipSleep = true;
                            recoveryIterations++;

                            if (LOG.isDebugEnabled()) LOG.debug("TRAF RCOV THREAD: in-doubt region size " + regions.size());
                            for (Map.Entry<String, byte[]> regionEntry : regions.entrySet()) {
                                List<Long> TxRecoverList = new ArrayList<Long>();
                                String hostnamePort = regionEntry.getKey();
                                byte[] regionBytes = regionEntry.getValue();
                                if (LOG.isDebugEnabled())
                                    LOG.debug("TRAF RCOV THREAD:Recovery Thread Processing region: " + new String(regionBytes));
                                if (recoveryIterations == 0) {
                                   if(LOG.isWarnEnabled()) {
                                      //  Let's get the host name
                                      final byte [] delimiter = ",".getBytes();
                                      String[] hostname = hostnamePort.split(new String(delimiter), 3);
                                      if (hostname.length < 2) {
                                         throw new IllegalArgumentException("hostnamePort format is incorrect");
                                      }

                                      LOG.warn ("TRAF RCOV THREAD:Starting recovery with " + regions.size() +
                                           " regions to recover.  First region hostname: " + hostnamePort +
                                           " Recovery iterations: " + recoveryIterations);
                                   }
                                }
                                else {
                                   if(recoveryIterations % 10 == 0) {
                                      if(LOG.isWarnEnabled()) {
                                         //  Let's get the host name
                                         final byte [] delimiter = ",".getBytes();
                                         String[] hostname = hostnamePort.split(new String(delimiter), 3);
                                         if (hostname.length < 2) {
                                            throw new IllegalArgumentException("hostnamePort format is incorrect");
                                         }
                                         LOG.warn("TRAF RCOV THREAD:Recovery thread encountered " + regions.size() +
                                           " regions to recover.  First region hostname: " + hostnamePort +
                                           " Recovery iterations: " + recoveryIterations);
                                      }
                                   }
                                }
                                try {
                                    TxRecoverList = txnManager.recoveryRequest(hostnamePort, regionBytes, tmID);
                                }catch (NotServingRegionException e) {
                                   TxRecoverList = null;
                                   LOG.error("TRAF RCOV THREAD:NotServingRegionException calling recoveryRequest. regionBytes: " + new String(regionBytes) +
                                             " TM: " + tmID + " hostnamePort: " + hostnamePort);

                                   try {
                                      // First delete the zookeeper entry
                                      LOG.error("TRAF RCOV THREAD:recoveryRequest. Deleting region entry Entry: " + regionEntry);
                                      zookeeper.deleteRegionEntry(regionEntry);
                                   }
                                   catch (Exception e2) {
                                      LOG.error("TRAF RCOV THREAD:Error calling deleteRegionEntry. regionEntry key: " + regionEntry.getKey() + " regionEntry value: " +
                                      new String(regionEntry.getValue()) + " exception: " + e2);
                                   }
                                   try {
                                      // Create a local HTable object using the regionInfo
                                      HTable table = new HTable(config, HRegionInfo.parseFrom(regionBytes).getTable().getNameAsString());
                                      try {
                                         // Repost a zookeeper entry for all current regions in the table
                                         zookeeper.postAllRegionEntries(table);
                                      }
                                      catch (Exception e2) {
                                         LOG.error("TRAF RCOV THREAD:Error calling postAllRegionEntries. table: " + new String(table.getTableName()) + " exception: " + e2);
                                      }
                                   }// try
                                   catch (Exception e1) {
                                      LOG.error("TRAF RCOV THREAD:recoveryRequest exception in new HTable " + HRegionInfo.parseFrom(regionBytes).getTable().getNameAsString() + " Exception: " + e1);
                                   }
                                }// NotServingRegionException
                                catch (TableNotFoundException tnfe) {
                                   // In this case there is nothing to recover.  We just need to delete the region entry.
                                   try {
                                      // First delete the zookeeper entry
                                      LOG.warn("TRAF RCOV THREAD:TableNotFoundException calling txnManager.recoveryRequest. " + "TM: " +
                                              tmID + " regionBytes: [" + regionBytes + "].  Deleting zookeeper region entry. \n exception: " + tnfe);
                                      zookeeper.deleteRegionEntry(regionEntry);
                                   }
                                   catch (Exception e2) {
                                      LOG.error("TRAF RCOV THREAD:Error calling deleteRegionEntry. regionEntry key: " + regionEntry.getKey() + " regionEntry value: " +
                                      new String(regionEntry.getValue()) + " exception: " + e2);
                                   }

                                }// TableNotFoundException
                                catch (DeserializationException de) {
                                   // We are unable to parse the region info from ZooKeeper  We just need to delete the region entry.
                                   try {
                                      // First delete the zookeeper entry
                                      LOG.warn("TRAF RCOV THREAD:DeserializationException calling txnManager.recoveryRequest. " + "TM: " +
                                              tmID + " regionBytes: [" + regionBytes + "].  Deleting zookeeper region entry. \n exception: " + de);
                                      zookeeper.deleteRegionEntry(regionEntry);
                                   }
                                   catch (Exception e2) {
                                      LOG.error("TRAF RCOV THREAD:Error calling deleteRegionEntry. regionEntry key: " + regionEntry.getKey() + " regionEntry value: " +
                                      new String(regionEntry.getValue()) + " exception: " + e2);
                                   }

                                }// DeserializationException
                                catch (Exception e) {
                                   LOG.error("TRAF RCOV THREAD:An ERROR occurred calling txnManager.recoveryRequest. " + "TM: " +
                                              tmID + " regionBytes: [" + regionBytes + "] exception: " + e);
                                }
                                if (TxRecoverList != null) {
                                    if (LOG.isDebugEnabled()) LOG.trace("TRAF RCOV THREAD:size of TxRecoverList " + TxRecoverList.size());
                                    if (TxRecoverList.size() == 0) {
                                      try {
                                         // First delete the zookeeper entry
                                         LOG.warn("TRAF RCOV THREAD:Leftover Znode  calling txnManager.recoveryRequest. " + "TM: " +
                                                 tmID + " regionBytes: [" + regionBytes + "].  Deleting zookeeper region entry. ");
                                         zookeeper.deleteRegionEntry(regionEntry);
                                      }
                                      catch (Exception e2) {
                                         LOG.error("TRAF RCOV THREAD:Error calling deleteRegionEntry. regionEntry key: " + regionEntry.getKey() + " regionEntry value: " +
                                         new String(regionEntry.getValue()) + " exception: " + e2);
                                      }
                                   }
                                   for (Long txid : TxRecoverList) {
                                      TransactionState ts = transactionStates.get(txid);
                                      if (ts == null) {
                                         ts = new TransactionState(txid);

                                         //Identify if DDL is part of this transaction and valid
                                         if(hbtx.useDDLTrans){
                                            TmDDL tmDDL = hbtx.getTmDDL();
                                            StringBuilder state = new StringBuilder ();
                                            try {
                                                tmDDL.getState(txid,state);
                                            }
                                            catch(Exception egetState){
                                                LOG.error("TRAF RCOV THREAD:Error calling TmDDL getState." + egetState);
                                            }
                                            if(state.toString().equals("VALID"))
                                               ts.setDDLTx(true);
                                         }
                                      }
                                      try {
                                         this.addRegionToTS(hostnamePort, regionBytes, ts);
                                      } catch (Exception e) {
                                         LOG.error("TRAF RCOV THREAD:Unable to add region to TransactionState, region info: " + new String(regionBytes));
                                         e.printStackTrace();
                                      }
                                      transactionStates.put(txid, ts);
                                   }
                                }
                                else if (LOG.isDebugEnabled()) LOG.debug("TRAF RCOV THREAD:size od TxRecoverList is NULL ");
                            }
                            if (LOG.isDebugEnabled()) LOG.debug("TRAF RCOV THREAD: in-doubt transaction size " + transactionStates.size());
                            for (Map.Entry<Long, TransactionState> tsEntry : transactionStates.entrySet()) {
                                TransactionState ts = tsEntry.getValue();
                                Long txID = ts.getTransactionId();
                                // TransactionState ts = new TransactionState(txID);
                                try {
                                    audit.getTransactionState(ts);
                                    if (ts.getStatus().equals(TransState.STATE_COMMITTED.toString())) {
                                        if (LOG.isDebugEnabled())
                                            LOG.debug("TRAF RCOV THREAD:Redriving commit for " + txID + " number of regions " + ts.getParticipatingRegions().size() +
                                                    " and tolerating UnknownTransactionExceptions");
                                        txnManager.doCommit(ts, true /*ignore UnknownTransactionException*/);
                                        if(useTlog && useForgotten) {
                                            long nextAsn = tLog.getNextAuditSeqNum((int)(txID >> 32));
                                            tLog.putSingleRecord(txID, ts.getCommitId(), "FORGOTTEN", null, forceForgotten, nextAsn);
                                        }
                                    } else if (ts.getStatus().equals(TransState.STATE_ABORTED.toString())) {
                                        if (LOG.isDebugEnabled())
                                            LOG.debug("TRAF RCOV THREAD:Redriving abort for " + txID);
                                        txnManager.abort(ts);
                                    } else {
                                        if (LOG.isDebugEnabled())
                                            LOG.debug("TRAF RCOV THREAD:Redriving abort for " + txID);
                                        LOG.warn("Recovering transaction " + txID + ", status is not set to COMMITTED or ABORTED. Aborting.");
                                        txnManager.abort(ts);
                                    }

                                }catch (UnsuccessfulDDLException ddle) {
                                    LOG.error("UnsuccessfulDDLException encountered by Recovery Thread. Registering for retry. txID: " + txID + "Exception " + ddle);
                                    ddle.printStackTrace();

                                    //Note that there may not be anymore redrive triggers from region server point of view for DDL operation.
                                    //Register this DDL transaction for subsequent redrive from Audit Control Event.
                                    //TODO: Launch a new Redrive Thread out of auditControlPoint().
                                    TmDDL tmDDL = hbtx.getTmDDL();
                                    try {
                                        tmDDL.setState(txID,"REDRIVE");
                                    }
                                    catch(Exception e){
                                         LOG.error("TRAF RCOV THREAD:Error calling TmDDL putRow Redrive" + e);
                                    }
                                }
                                catch (Exception e) {
                                    LOG.error("Unable to get audit record for tx: " + txID + ", audit is throwing exception.");
                                    e.printStackTrace();
                                }
                            }

                        }
                        else {
                            if (recoveryIterations > 0) {
                                if(LOG.isInfoEnabled()) LOG.info("Recovery completed for TM" + tmID);
                            }
                            recoveryIterations = -1;
                        }
                        try {
                            if(continueThread) {
                                if(!skipSleep) {
                                    if (sleepTimeInt > 0)
                                        Thread.sleep(sleepTimeInt);
                                    else
                                        Thread.sleep(SLEEP_DELAY);
                                }
                            }
                            retryCount = 0;
                        } catch (Exception e) {
                            LOG.error("Error in recoveryThread: " + e);
                        }

                    } catch (Exception e) {
                        int possibleRetries = 4;
                        LOG.error("Caught recovery thread exception for tmid: " + tmID + " retries: " + retryCount);
                        StringWriter sw = new StringWriter();
                        PrintWriter pw = new PrintWriter(sw);
                        e.printStackTrace(pw);
                        LOG.error(sw.toString());

                        retryCount++;
                        if(retryCount > possibleRetries) {
                            LOG.error("Recovery thread failure, aborting process");
                            System.exit(4);
                        }

                        try {
                            Thread.sleep(SLEEP_DELAY / possibleRetries);
                        } catch(Exception se) {
                            LOG.error(se);
                        }
                    }
                }
                if(LOG.isDebugEnabled()) LOG.debug("Exiting recovery thread for tm ID: " + tmID);
            }
     }

     //================================================================================
     // DTMCI Calls
     //================================================================================

     //--------------------------------------------------------------------------------
     // callRequestRegionInfo
     // Purpose: Prepares HashMapArray class to get region information
     //--------------------------------------------------------------------------------
      public HashMapArray callRequestRegionInfo() throws Exception {

      String tablename, encoded_region_name, region_name, is_offline, region_id, hostname, port, thn;

      HashMap<String, String> inMap;
      long lv_ret = -1;
      Long key;
      TransactionState value;
      int tnum = 0; // Transaction number

      if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient::callRequestRegionInfo:: start\n");

      HashMapArray hm = new HashMapArray();

      try{
      for(ConcurrentHashMap.Entry<Long, TransactionState> entry : mapTransactionStates.entrySet()){
          key = entry.getKey();
          value = entry.getValue();
          long id = value.getTransactionId();

          TransactionState ts = mapTransactionStates.get(id);
          final Set<TransactionRegionLocation> regions = ts.getParticipatingRegions();

          // TableName
          Iterator<TransactionRegionLocation> it = regions.iterator();
          tablename = it.next().getRegionInfo().getTable().getNameAsString();
          while(it.hasNext()){
              tablename = tablename + ";" + it.next().getRegionInfo().getTable().getNameAsString();
          }
          hm.addElement(tnum, "TableName", tablename);

          // Encoded Region Name
          Iterator<TransactionRegionLocation> it2 = regions.iterator();
          encoded_region_name = it2.next().getRegionInfo().getEncodedName();
          while(it2.hasNext()){
              encoded_region_name = encoded_region_name + ";" + it2.next().getRegionInfo().getTable().getNameAsString();
          }
          hm.addElement(tnum, "EncodedRegionName", encoded_region_name);

          // Region Name
          Iterator<TransactionRegionLocation> it3 = regions.iterator();
          region_name = it3.next().getRegionInfo().getRegionNameAsString();
          while(it3.hasNext()){
              region_name = region_name + ";" + it3.next().getRegionInfo().getTable().getNameAsString();
          }
          hm.addElement(tnum, "RegionName", region_name);

          // Region Offline
          Iterator<TransactionRegionLocation> it4 = regions.iterator();
          boolean is_offline_bool = it4.next().getRegionInfo().isOffline();
          is_offline = String.valueOf(is_offline_bool);
          hm.addElement(tnum, "RegionOffline", is_offline);

          // Region ID
          Iterator<TransactionRegionLocation> it5 = regions.iterator();
          region_id = String.valueOf(it5.next().getRegionInfo().getRegionId());
          while(it5.hasNext()){
              region_id = region_id + ";" + it5.next().getRegionInfo().getRegionId();
          }
          hm.addElement(tnum, "RegionID", region_id);

          // Hostname
          Iterator<TransactionRegionLocation> it6 = regions.iterator();
          thn = String.valueOf(it6.next().getHostname());
          hostname = thn.substring(0, thn.length()-1);
          while(it6.hasNext()){
              thn = String.valueOf(it6.next().getHostname());
              hostname = hostname + ";" + thn.substring(0, thn.length()-1);
          }
          hm.addElement(tnum, "Hostname", hostname);

          // Port
          Iterator<TransactionRegionLocation> it7 = regions.iterator();
          port = String.valueOf(it7.next().getPort());
          while(it7.hasNext()){
              port = port + ";" + String.valueOf(it7.next().getPort());
          }
          hm.addElement(tnum, "Port", port);

          tnum = tnum + 1;
        }
      }catch(Exception e){
         if (LOG.isTraceEnabled()) LOG.trace("Error in getting region info. Map might be empty. Please ensure sqlci insert was done");
      }

      if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient::callRequestRegionInfo:: end size: " + hm.getSize());
      return hm;
   }
}

