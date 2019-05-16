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
import java.util.Map;
import java.util.HashMap;
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
import org.apache.hadoop.hbase.client.Connection;
import org.apache.hadoop.hbase.client.ConnectionFactory;
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
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.NotServingRegionException;
import org.apache.hadoop.hbase.ServerName;
import org.apache.hadoop.hbase.NotServingRegionException;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.ByteArrayKey;
import org.trafodion.dtm.HBaseTmZK;
import org.trafodion.dtm.TmAuditTlog;
import org.trafodion.dtm.TransactionManagerException;

import org.apache.hadoop.hbase.regionserver.transactional.IdTm;
import org.apache.hadoop.hbase.regionserver.transactional.IdTmException;
import org.apache.hadoop.hbase.regionserver.transactional.IdTmId;

import org.apache.zookeeper.KeeperException;
import org.trafodion.sql.TrafConfiguration;

import java.util.Map;
import java.util.HashMap;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentHashMap;

public class HBaseTxClient {

   static final Log LOG = LogFactory.getLog(HBaseTxClient.class);
   private static Connection connection;
   private static TmAuditTlog tLog;
   private static HBaseTmZK tmZK;
   private static RecoveryThread recovThread;
   private static TmDDL tmDDL;
   private short dtmID;
   private int stallWhere;
   private IdTm idServer;
   private static final int ID_TM_SERVER_TIMEOUT = 1000;
   private static int myClusterId = 0;

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
        System.setProperty("trafodion.logdir", System.getenv("TRAF_LOG"));
        String confFile = System.getenv("TRAF_CONF")
            + "/log4j.dtm.config";
        PropertyConfigurator.configure(confFile);
    }

   public boolean init(String hBasePath, String zkServers, String zkPort) throws IOException {
      //System.out.println("In init - hbp");
      setupLog4j();
      if (LOG.isDebugEnabled()) LOG.debug("Enter init, hBasePath:" + hBasePath);
      if (LOG.isTraceEnabled()) LOG.trace("mapTransactionStates " + mapTransactionStates + " entries " + mapTransactionStates.size());
      if (config == null) {
         config = TrafConfiguration.create(TrafConfiguration.HBASE_CONF);
         connection = ConnectionFactory.createConnection(config);
      }
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
      catch (NumberFormatException e) {
         LOG.error("TM_ENABLE_FORGOTTEN_RECORDS is not valid in ms.env");
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
      catch (NumberFormatException e) {
         LOG.error("TM_TLOG_FORCE_FORGOTTEN is not valid in ms.env");
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
      catch (NumberFormatException e) {
         LOG.error("TM_ENABLE_TLOG_WRITES is not valid in ms.env");
      }

      if (useTlog) {
            tLog = new TmAuditTlog(config, connection);
      }
      try {
        trxManager = TransactionManager.getInstance(config, connection);
      } catch (IOException e ){
          LOG.error("trxManager Initialization failure throwing exception", e);
          throw e;
      }

      if (useRecovThread) {
         if (LOG.isDebugEnabled()) LOG.debug("Starting recovery thread for tm ID: " + dtmID);
          try {
              tmZK = new HBaseTmZK(config, dtmID);
          } catch (IOException e ){
              LOG.error("Unable to create HBaseTmZK TM-zookeeper class, throwing exception", e);
              throw e;
          }
          recovThread = new RecoveryThread(tLog, tmZK, trxManager);
          recovThread.start();
      }
      if (LOG.isDebugEnabled()) LOG.debug("Exit init(String, String, String)");
      return true;
   }

   public boolean init(short dtmid) throws IOException {
      //System.out.println("In init - dtmId" + dtmid);

      setupLog4j();
      if (LOG.isDebugEnabled()) LOG.debug("Enter init(" + dtmid + ")");
      if (config == null) {
         config = TrafConfiguration.create(TrafConfiguration.HBASE_CONF);
         connection = ConnectionFactory.createConnection(config);
      }
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
      try {
          if (useSSCC != null)
             TRANSACTION_ALGORITHM = (Integer.parseInt(useSSCC) == 1) ? AlgorithmType.SSCC :AlgorithmType.MVCC ;
      } catch (NumberFormatException e) {
         LOG.error("TRANSACTION_ALGORITHM is not valid in ms.env");
      }

      idServer = new IdTm(false);

      String useDDLTransactions = System.getenv("TM_ENABLE_DDL_TRANS");
    
      try {
         if (useDDLTransactions != null) {
             useDDLTrans = (Integer.parseInt(useDDLTransactions) != 0);
         }
      }
      catch (NumberFormatException e) {
         LOG.error("TM_ENABLE_DDL_TRANS is not valid in ms.env");
      }

      if (useDDLTrans)
         tmDDL = new TmDDL(config, connection);

      useForgotten = true;
         String useAuditRecords = System.getenv("TM_ENABLE_FORGOTTEN_RECORDS");
      try {
         if (useAuditRecords != null) {
            useForgotten = (Integer.parseInt(useAuditRecords) != 0);
         }
      }
      catch (NumberFormatException e) {
         LOG.error("TM_ENABLE_FORGOTTEN_RECORDS is not valid in ms.env");
      }
      LOG.info("useForgotten is " + useForgotten);

      forceForgotten = false;
         String forgottenForce = System.getenv("TM_TLOG_FORCE_FORGOTTEN");
      try {
         if (forgottenForce != null){
            forceForgotten = (Integer.parseInt(forgottenForce) != 0);
         }
      }
      catch (NumberFormatException e) {
         LOG.error("TM_TLOG_FORCE_FORGOTTEN is not valid in ms.env");
      }
      LOG.info("forceForgotten is " + forceForgotten);

      useTlog = false;
      useRecovThread = false;
         String useAudit = System.getenv("TM_ENABLE_TLOG_WRITES");
      try {
         if (useAudit != null){
            useTlog = useRecovThread = (Integer.parseInt(useAudit) != 0);
         }
      }
      catch (NumberFormatException e) {
         LOG.error("TM_ENABLE_TLOG_WRITES is not valid in ms.env");
      }
      if (useTlog) {
         tLog = new TmAuditTlog(config, connection);
      }
      trxManager = TransactionManager.getInstance(config, connection);
      if(useDDLTrans)
          trxManager.init(tmDDL);

      if (useRecovThread) {
         if (LOG.isDebugEnabled()) LOG.debug("Entering recovThread Usage");
          try {
              tmZK = new HBaseTmZK(config, dtmID);
          } catch (IOException e ) {
              LOG.error("Unable to create HBaseTmZK TM-zookeeper class, throwing exception", e);
              throw e;
          }
          recovThread = new RecoveryThread(tLog,
                                           tmZK,
                                           trxManager,
                                           this,
                                           useForgotten,
                                           forceForgotten,
                                           useTlog,
                                           false,
                                           false);
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
                                                   useTlog,
                                                   false,
                                                   true);
               newRecovThread.start();
               mapRecoveryThreads.put(nodeID, recovThread);
               if(LOG.isTraceEnabled()) LOG.trace("nodeDown -- mapRecoveryThreads size: " + mapRecoveryThreads.size());
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
       boolean loopBack = false;
       do {
          try {
             loopBack = false;
             rt.join();
          } catch (InterruptedException e) { 
             LOG.warn("Problem while waiting for the recovery thread to stop for node ID: " + nodeID, e);
             loopBack = true; 
          }
       } while (loopBack);
       mapRecoveryThreads.remove(nodeID);
       if(LOG.isTraceEnabled()) LOG.trace("nodeUp -- mapRecoveryThreads size: " + mapRecoveryThreads.size());
       if(LOG.isTraceEnabled()) LOG.trace("nodeUp -- EXIT node ID: " + nodeID);
   }

   public short stall (int where) {
      if (LOG.isDebugEnabled()) LOG.debug("Entering stall with parameter " + where);
      this.stallWhere = where;
      return TransReturnCode.RET_OK.getShort();
   }

   public static Map<Long, TransactionState> getMap() {
     return mapTransactionStates;
   }

   public long beginTransaction(final long transactionId) throws IOException {

      if (LOG.isTraceEnabled()) LOG.trace("Enter beginTransaction, txid: " + transactionId);
      TransactionState tx = null;
      try {
         tx = trxManager.beginTransaction(transactionId);
      } catch (IdTmException ite) {
          LOG.error("Begin Transaction Error caused by : ", ite);
          throw new IOException("Begin Transaction Error caused by :", ite);
      }  
      if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient:beginTransaction new transactionState created: " + tx);

      synchronized(mapLock) {
         TransactionState tx2 = mapTransactionStates.get(transactionId);
         if (tx2 != null) {
            // Some other thread added the transaction while we were creating one.  It's already in the
            // map, so we can use the existing one.
            if (LOG.isDebugEnabled()) LOG.debug("beginTransaction, found TransactionState object while creating a new one " + tx2);
            tx = tx2;
         }
         else {
            if (LOG.isDebugEnabled()) LOG.debug("beginTransaction, adding new TransactionState to map " + tx);
            mapTransactionStates.put(transactionId, tx);
         }
      }

      if (LOG.isDebugEnabled()) LOG.debug("Exit beginTransaction, Transaction State: " + tx + " mapsize: " + mapTransactionStates.size());
      return transactionId;
   }

   public short abortTransaction(final long transactionID) throws IOException {
      if (LOG.isDebugEnabled()) LOG.debug("Enter abortTransaction, txId: " + transactionID);
      TransactionState ts = mapTransactionStates.get(transactionID);

      if(ts == null) {
          LOG.error("Returning from HBaseTxClient:abortTransaction, txid: " + transactionID + " retval: " + TransReturnCode.RET_NOTX.toString());
          return TransReturnCode.RET_NOTX.getShort();
      }

      try {
         ts.setStatus(TransState.STATE_ABORTED);
         if (useTlog) {
            tLog.putSingleRecord(transactionID, ts.getStartId(), -1, TransState.STATE_ABORTED.toString(), ts.getParticipatingRegions(), ts.hasPlaceHolder(), false); //force flush
         }
      }
      catch(IOException e) {
         LOG.error("Returning from HBaseTxClient:abortTransaction, txid: " + transactionID + " tLog.putRecord: EXCEPTION ", e);
         return TransReturnCode.RET_EXCEPTION.getShort();
      }

      if ((stallWhere == 1) || (stallWhere == 3)) {
         LOG.info("Stalling in phase 2 for abortTransaction");
         boolean loopBack = false;
         do {
            try {
               loopBack = false;
               Thread.sleep(300000); // Initially set to run every 5 min
            } catch(InterruptedException ie) {
               loopBack = true;
            }
         } while (loopBack);
      }

      try {
         trxManager.abort(ts);
      } catch (UnsuccessfulDDLException ddle) {
          LOG.error("FATAL DDL Exception from HBaseTxClient:abort, WAITING INDEFINETLY !! retval: " + TransReturnCode.RET_EXCEPTION.toString() + " UnsuccessfulDDLException" + " txid: " + transactionID, ddle);

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
             boolean loopBack = false;
             do {
                try {
                    loopBack = false;
                    commitDDLLock.wait();
                } catch(InterruptedException ie) {
                    LOG.warn("Interrupting commitDDLLock.wait,  but retrying ", ie);
                    loopBack = true;
                }
             } while (loopBack);
          }
          return TransReturnCode.RET_EXCEPTION.getShort();
      } 
      catch(IOException e) {
          synchronized(mapLock) {
             mapTransactionStates.remove(transactionID);
          }
          LOG.error("Returning from HBaseTxClient:abortTransaction, txid: " + transactionID + " retval: EXCEPTION", e);
          return TransReturnCode.RET_EXCEPTION.getShort();
      }

      if (useTlog && useForgotten) {
            tLog.putSingleRecord(transactionID, ts.getStartId(), -1, TransState.STATE_FORGOTTEN_ABORT.toString(), ts.getParticipatingRegions(), ts.hasPlaceHolder(), forceForgotten); // forced flush?
      }
 //     mapTransactionStates.remove(transactionID);

      if (LOG.isTraceEnabled()) LOG.trace("Exit abortTransaction, retval: OK txid: " + transactionID + " mapsize: " + mapTransactionStates.size());
      return TransReturnCode.RET_OK.getShort();
   }

   public short prepareCommit(long transactionId) throws 
                                                 TransactionManagerException,
                                                 IOException{
     if (LOG.isDebugEnabled()) LOG.debug("Enter prepareCommit"
					 + ", txId: " + transactionId
					 + ", #txstate entries " + mapTransactionStates.size()
					 );
     TransactionState ts = mapTransactionStates.get(transactionId);
     
     if (ts == null) {
       LOG.error("Returning from prepareCommit" 
		 + ", txId: " + transactionId 
		 + ", retval: " + TransReturnCode.RET_NOTX.toString()
		 );
       return TransReturnCode.RET_NOTX.getShort();
     }

     try {
        short result = (short) trxManager.prepareCommit(ts);
        if (LOG.isDebugEnabled()) LOG.debug("prepareCommit, [ " + ts + " ], result " + result + ((result == TransactionalReturn.COMMIT_OK_READ_ONLY)?", Read-Only":""));
        switch (result) {
          case TransactionalReturn.COMMIT_OK:
             if (LOG.isTraceEnabled()) LOG.trace("Exit OK prepareCommit, txId: " + transactionId);
             return TransReturnCode.RET_OK.getShort();
          case TransactionalReturn.COMMIT_OK_READ_ONLY:
             synchronized(mapLock) {
                mapTransactionStates.remove(transactionId);
             }
             if (LOG.isTraceEnabled()) LOG.trace("Exit OK_READ_ONLY prepareCommit, txId: " + transactionId);
             return TransReturnCode.RET_READONLY.getShort();
          case TransactionalReturn.COMMIT_UNSUCCESSFUL:
             if(!ts.getRecordedException().isEmpty())
               throw new TransactionManagerException(ts.getRecordedException(),
                                     TransReturnCode.RET_EXCEPTION.getShort());
             else
               throw new TransactionManagerException("Encountered COMMIT_UNSUCCESSFUL Exception, txid:" + transactionId,
                                     TransReturnCode.RET_EXCEPTION.getShort());
          case TransactionalReturn.COMMIT_CONFLICT:
             if(! ts.getRecordedException().isEmpty())
               throw new TransactionManagerException(ts.getRecordedException(),
                                   TransReturnCode.RET_HASCONFLICT.getShort());
             else
               throw new TransactionManagerException("Encountered COMMIT_CONFLICT Exception, txid:" + transactionId,
                                   TransReturnCode.RET_HASCONFLICT.getShort());
          default:
             if(! ts.getRecordedException().isEmpty())
               throw new TransactionManagerException(ts.getRecordedException(),
                                   TransReturnCode.RET_EXCEPTION.getShort());
             else
               throw new TransactionManagerException("Encountered COMMIT_UNSUCCESSFUL Exception, txid:" + transactionId,
                                   TransReturnCode.RET_EXCEPTION.getShort());
             
        }
     }catch (TransactionManagerException t) {
       LOG.error("Returning from HBaseTxClient:prepareCommit, txid: " + transactionId + " retval: " + t.getErrorCode(), t);
       throw t;
     } 
     catch (CommitUnsuccessfulException e) {
       LOG.error("Returning from prepareCommit, txId: " + transactionId + " retval: " + TransReturnCode.RET_NOCOMMITEX.toString() + " CommitUnsuccessfulException");
       throw new TransactionManagerException(e,
                                   TransReturnCode.RET_NOCOMMITEX.getShort());
     }
     catch (IOException e) {
       LOG.error("Returning from prepareCommit, txId: " + transactionId + " retval: " + TransReturnCode.RET_IOEXCEPTION.toString() + " IOException");
       throw new TransactionManagerException(e,
                                   TransReturnCode.RET_IOEXCEPTION.getShort());
     }
   }

   public short doCommit(long transactionId) throws IOException {
      if (LOG.isDebugEnabled()) LOG.debug("Enter doCommit, txId: " + transactionId);
      TransactionState ts = mapTransactionStates.get(transactionId);

      if(ts == null) {
	      LOG.error("Returning from doCommit, (null tx) retval: " 
			+ TransReturnCode.RET_NOTX.toString() 
			+ ", txId: " + transactionId
			);
          return TransReturnCode.RET_NOTX.getShort();
       }

       // Set the commitId
       IdTmId commitId = null;
       long commitIdVal = -1;
       if (TRANSACTION_ALGORITHM == AlgorithmType.SSCC) {
          try {
             commitId = new IdTmId();
             if (LOG.isTraceEnabled()) LOG.trace("doCommit getting new commitId");
             idServer.id(ID_TM_SERVER_TIMEOUT, commitId);
             if (LOG.isTraceEnabled()) LOG.trace("doCommit idServer.id returned: " + commitId.val);
          } catch (IdTmException exc) {
             LOG.error("doCommit: IdTm threw exception " ,  exc);
             throw new CommitUnsuccessfulException("doCommit: IdTm threw exception " ,  exc);
          }
          commitIdVal = commitId.val;
       }
       if (LOG.isTraceEnabled()) LOG.trace("doCommit setting commitId (" + commitIdVal + ") for tx: " + ts.getTransactionId());
       ts.setCommitId(commitIdVal);

       if (stallWhere == 4) {
    	  if (LOG.isInfoEnabled())LOG.info("Stalling in phase 2a (before TLOG write) for doCommit for transaction: " + transactionId);
          boolean loopBack = false;
          do
          {
             try {
                loopBack = false;
                Thread.sleep(600000); // Initially set to run every 5 min
             } catch (InterruptedException ie) {
                loopBack = true;
             }
          } while (loopBack);
       }

       //try {
          ts.setStatus(TransState.STATE_COMMITTING);
          if (useTlog) {
             try {
                tLog.putSingleRecord(transactionId, ts.getStartId(), commitIdVal, TransState.STATE_COMMITTED.toString(), ts.getParticipatingRegions(), ts.hasPlaceHolder(), true);
                ts.setStatus(TransState.STATE_COMMITTED);
             }
             catch (IOException e) {
                 LOG.error("doCommit: Local TLOG write threw exception during commit " , e);
                 throw new RuntimeException(e);
             }
          }
       if ((stallWhere == 2) || (stallWhere == 3)) {
    	  if (LOG.isInfoEnabled())LOG.info("Stalling in phase 2 for doCommit for transaction: " + transactionId);
          boolean loopBack = false;
          do {
             try {
                loopBack = false;
                Thread.sleep(300000); // Initially set to run every 5 min
             } catch(InterruptedException ie) {
                 loopBack = true;
             }
          } while (loopBack);
       }

       try {
          if (LOG.isTraceEnabled()) LOG.trace("doCommit, calling trxManager.doCommit(" + ts.getTransactionId() + ")" );
          trxManager.doCommit(ts);
       } catch (CommitUnsuccessfulException e) {
          LOG.error("Returning from doCommit, transaction: " + transactionId
        		      + ", retval: " + TransReturnCode.RET_EXCEPTION.toString() + " IOException");
          return TransReturnCode.RET_EXCEPTION.getShort();
       }
       catch (UnsuccessfulDDLException ddle) {
          LOG.error("FATAL DDL Exception from doCommit, WAITING INDEFINETLY !! retval: " + TransReturnCode.RET_EXCEPTION.toString() + " UnsuccessfulDDLException" + " txId: " + transactionId);

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
             boolean loopBack = false;
             do {
                try {
                   loopBack = false;
                   commitDDLLock.wait();
                } catch(InterruptedException ie) {
                    LOG.warn("Interrupting commitDDLLock.wait" 
			     + ", txId: " + transactionId
			     + ", retrying ", ie);
                    loopBack = true;
                }
             } while (loopBack);
          }
          return TransReturnCode.RET_EXCEPTION.getShort();
       }
       if (useTlog && useForgotten) {
          tLog.putSingleRecord(transactionId, ts.getStartId(), commitIdVal, TransState.STATE_FORGOTTEN_COMMITTED.toString(), ts.getParticipatingRegions(), ts.hasPlaceHolder(), forceForgotten); // forced flush?
       }
       if (LOG.isTraceEnabled()) LOG.trace("Exit doCommit, retval(ok): " + TransReturnCode.RET_OK.toString() +
                         " txId: " + transactionId + " mapsize: " + mapTransactionStates.size());

       return TransReturnCode.RET_OK.getShort();
   }

    public short completeRequest(long transactionId)
	throws IOException, CommitUnsuccessfulException 
    {
     if (LOG.isDebugEnabled()) LOG.debug("Enter completeRequest" 
					 + ", txId: " + transactionId
					 );
     TransactionState ts = mapTransactionStates.get(transactionId);

     if (ts == null) {
	     LOG.error("Returning from completeRequest, (null tx) retval: " 
		       + TransReturnCode.RET_NOTX.toString() 
		       + ", txId: " + transactionId
		       );
	 return TransReturnCode.RET_NOTX.getShort();
     }

       boolean loopBack = false;
       do {
          try {
             loopBack = false;
             ts.completeRequest();
          } catch(InterruptedException ie) {
              LOG.warn("Interrupting completeRequest but retrying, ts.completeRequest: txid: " + transactionId + ", EXCEPTION: ", ie);
              loopBack = true;
          } 
       } while (loopBack);

     synchronized(mapLock) {
        mapTransactionStates.remove(transactionId);
     }

     if (LOG.isDebugEnabled()) LOG.debug("Exit completeRequest txid: " + transactionId + " mapsize: " + mapTransactionStates.size());
     return TransReturnCode.RET_OK.getShort();
   }

   public short tryCommit(long transactionId) throws IOException, CommitUnsuccessfulException {
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
         // It is a violation of 2 PC protocol to try to abort the transaction after commit write
         return commitErr;
       }

       if (LOG.isTraceEnabled()) LOG.trace("TEMP tryCommit Calling CompleteRequest() Txid :" + transactionId);

       err = completeRequest(transactionId);
       if (err != TransReturnCode.RET_OK.getShort()){
         if (LOG.isDebugEnabled()) LOG.debug("tryCommit completeRequest for transaction " + transactionId + " failed with error " + err);
       }
     } catch(IOException e) {
       mapTransactionStates.remove(transactionId);
       LOG.error("Returning from HBaseTxClient:tryCommit, ts: EXCEPTION" + " txid: " + transactionId, e);
       throw new IOException("Exception during tryCommit, unable to commit.", e);
    }

    synchronized(mapLock) {
       mapTransactionStates.remove(transactionId);
    }

    if (LOG.isDebugEnabled()) LOG.debug("Exit completeRequest txId: " + transactionId + " mapsize: " + mapTransactionStates.size());
    return TransReturnCode.RET_OK.getShort();
  }

   public short callCreateTable(long transactionId, byte[] pv_htbldesc, Object[]  beginEndKeys) throws IOException
   {
      TransactionState ts;
      HTableDescriptor htdesc = null;

      if (LOG.isTraceEnabled()) LOG.trace("Enter callCreateTable, txId: [" + transactionId + "],  htbldesc bytearray: " + pv_htbldesc + "desc in hex: " + Hex.encodeHexString(pv_htbldesc));

      ts = mapTransactionStates.get(transactionId);
      if(ts == null) {
         LOG.error("Returning from callCreateTable, (null tx) retval: " + TransReturnCode.RET_NOTX.getShort()  + " txId: " + transactionId);
         return TransReturnCode.RET_NOTX.getShort();
      }
      try {
         htdesc = HTableDescriptor.parseFrom(pv_htbldesc);
      } catch (DeserializationException de) {
         LOG.error("Error while getting HTableDescriptor caused by : ", de);
         throw new IOException("Error while getting HTableDescriptor caused by : ", de);
      }
      try {
         trxManager.createTable(ts, htdesc, beginEndKeys);
      }
      catch (IOException cte) {
         LOG.error("HBaseTxClient:callCreateTable exception trxManager.createTable, retval: " +
            TransReturnCode.RET_EXCEPTION.toString() +" txid: " + transactionId +" Exception: ", cte);
         throw new IOException("createTable call error", cte);
      }

      
      if (LOG.isTraceEnabled()) LOG.trace("Exit callCreateTable, txid: [" + transactionId + "] returning RET_OK");
      return TransReturnCode.RET_OK.getShort();
   }

   public short callAlterTable(long transactionId, byte[] pv_tblname, Object[] tableOptions) throws IOException
   {
      TransactionState ts;
      String strTblName = new String(pv_tblname, "UTF-8");

      if (LOG.isTraceEnabled()) LOG.trace("Enter callAlterTable, txId: [" + transactionId + "],  tableName: " + strTblName);

      ts = mapTransactionStates.get(transactionId);
      if(ts == null) {
         LOG.error("Returning from callAlterTable, (null tx) retval: " + TransReturnCode.RET_NOTX.getShort()  + " txId: " + transactionId);
         return TransReturnCode.RET_NOTX.getShort();
      }

      trxManager.alterTable(ts, strTblName, tableOptions);
      return TransReturnCode.RET_OK.getShort();
   }

   public short callRegisterTruncateOnAbort(long transactionId, byte[] pv_tblname) throws IOException
   {
      TransactionState ts;
      String strTblName = new String(pv_tblname, "UTF-8");

      if (LOG.isTraceEnabled()) LOG.trace("Enter callRegisterTruncateOnAbort, txId: [" + transactionId + "],  tablename: " + strTblName);

      ts = mapTransactionStates.get(transactionId);
      if(ts == null) {
         LOG.error("Returning from callRegisterTruncateOnAbort, (null tx) retval: " + TransReturnCode.RET_NOTX.getShort()  + " txId: " + transactionId);
         return TransReturnCode.RET_NOTX.getShort();
      }

      trxManager.registerTruncateOnAbort(ts, strTblName);
      return TransReturnCode.RET_OK.getShort();
   }

   public short callDropTable(long transactionId, byte[] pv_tblname) throws IOException
   {
      TransactionState ts;
      String strTblName = new String(pv_tblname, "UTF-8");

      if (LOG.isTraceEnabled()) LOG.trace("Enter callDropTable, txId: [" + transactionId + "],  tablename: " + strTblName);

      ts = mapTransactionStates.get(transactionId);
      if(ts == null) {
         LOG.error("Returning from callDropTable, (null tx) retval: " + TransReturnCode.RET_NOTX.getShort()  + " txId: " + transactionId);
         return TransReturnCode.RET_NOTX.getShort();
      }

      trxManager.dropTable(ts, strTblName);
      return TransReturnCode.RET_OK.getShort();
   }

    public short callRegisterRegion(long transactionId,
                                    long startId,
                                    int  pv_port,
                                    byte[] pv_hostname,
                                    long pv_startcode,
                                    byte[] pv_regionInfo) throws IOException {
       String hostname    = new String(pv_hostname);
       if (LOG.isTraceEnabled()) LOG.trace("Enter callRegisterRegion, "
					   + "txId: [" + transactionId + "]" 
					   + ", startId: " + startId 
					   + ", port: " + pv_port 
					   + ", hostname: " + hostname 
					   + ", startcode: " + pv_startcode 
					   + ", reg info len: " + pv_regionInfo.length 
					   + " " + new String(pv_regionInfo, "UTF-8"));

       HRegionInfo lv_regionInfo;
       try {
          lv_regionInfo = HRegionInfo.parseFrom(pv_regionInfo);
       } catch (DeserializationException de) {
           LOG.error("Error while getting regionInfo caused by : ", de);
           throw new IOException("Error while getting regionInfo caused by : ", de);
       }

       // TODO Not in CDH 5.1       ServerName lv_servername = ServerName.valueOf(hostname, pv_port, pv_startcode);
       String lv_hostname_port_string = hostname + ":" + pv_port;
       String lv_servername_string = ServerName.getServerName(lv_hostname_port_string, pv_startcode);
       ServerName lv_servername = ServerName.parseServerName(lv_servername_string);
       TransactionRegionLocation regionLocation = new TransactionRegionLocation(lv_regionInfo, lv_servername);
       String regionTableName = regionLocation.getRegionInfo().getTable().getNameAsString();

       TransactionState ts = mapTransactionStates.get(transactionId);
       if(ts == null) {
          if (LOG.isTraceEnabled()) LOG.trace("callRegisterRegion transactionId (" + transactionId +
                   ") not found in mapTransactionStates of size: " + mapTransactionStates.size());
          try {
             ts = trxManager.beginTransaction(transactionId);
          } catch (IdTmException ite) {
             LOG.error("Begin Transaction Error caused by : ", ite);
             throw new IOException("Begin Transaction Error caused by :", ite);
          }
          synchronized (mapLock) {
             TransactionState ts2 = mapTransactionStates.get(transactionId);
             if (ts2 != null) {
                // Some other thread added the transaction while we were creating one.  It's already in the
                // map, so we can use the existing one.
                if (LOG.isTraceEnabled()) LOG.trace("callRegisterRegion, found TransactionState object while creating a new one " + ts2);
                ts = ts2;
             }
             else {
                ts.setStartId(startId);
                if (LOG.isTraceEnabled()) LOG.trace("callRegisterRegion new transactionState created: " + ts );
             }
          }// end synchronized
       }
       else {
          if (LOG.isTraceEnabled()) LOG.trace("callRegisterRegion existing transactionState found: " + ts );
          if (ts.getStartId() == -1) {
            ts.setStartId(startId);
            if (LOG.isTraceEnabled()) LOG.trace("callRegisterRegion reset startId for transactionState: " + ts );
          }
       }

       try {
          trxManager.registerRegion(ts, regionLocation);
       } catch (IOException e) {
          LOG.error("callRegisterRegion exception in registerRegion call, txId: " + transactionId +
            " retval: " + TransReturnCode.RET_EXCEPTION.toString() + " IOException " + e);
          return TransReturnCode.RET_EXCEPTION.getShort();
       }

       if (LOG.isDebugEnabled()) LOG.debug("RegisterRegion adding table name " + regionTableName);
       ts.addTableName(regionTableName);

       if (LOG.isTraceEnabled()) LOG.trace("Exit callRegisterRegion, txId: [" + transactionId + "] with mapsize: "
                  + mapTransactionStates.size());
       return TransReturnCode.RET_OK.getShort();
   }

   public int participatingRegions(long transactionId) throws IOException {
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

   public long addControlPoint() throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("Enter addControlPoint");
      long result = 0L;
      if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient calling tLog.addControlPoint with mapsize " + mapTransactionStates.size());
      result = tLog.addControlPoint(myClusterId, mapTransactionStates, true);
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
   /*
    * DDL specific recovery operation use cases are as follows:
    * In case of phase 1 prepare , phase 2 commit, DDL operations are
    * completed once the region operations are complete. However incase of TM
    * going down and restarting, some of the state information is lost.
    * 1. If the DDL operation also involves DML operations, regions that have
    * in doubt transaction request for help. Recovery thread here reconstructs 
    * the TS state and also DDL operation state from TMDDL and redrives the operation.
    * 
    * 2. If all the regions have completed their operations and if only DDL operation
    * is pending and TM goes down and restarted, there are no regions that 
    * would seek help to redrive the operations. if there is pending DDL operation
    * it will be left starving as there are no triggers to redrive the operation. 
    * To handle this case, every time TM starts, as part of recovery thread start
    * a general scan of TMDDL is made to check for owning transIDs and those that 
    * have active DDL is checked against state of transaction and appropriately redriven.
    * 
    * 3. Failure of TM and restart of TM can happen at any state of DDL operation 
    * in progress and before that operation is recorded as complete. One way to 
    * accurately keep note of this operation in progress is to record the operation 
    * before and after the operation. For this, the table against which the operation
    * is being performed would be the key in a new log, we choose another table
    * called TmDDLObject table. This acts as a global semaphore for DDL table 
    * operation. Recovery Thread as part of its startup processing always checks 
    * against TmDDLObject table and if it owns the transaction, continues to
    * recover the DDL operation. 
    *
    */
    /* DDL Use cases and detailed info :

    Use case                Tx result     phase 1       phase 2
    ----------              ----------    -------     ------------
    Create Table            commit        no op         no op 
    Create Table            rollback      no op         disable ,delete             
    
    create table ,insert    commit        no op         no op
    create table, insert    rollback      no op         disable, delete    Notes:need flag for preclose, make this flag persistent in tmddl and region
    
    
    Drop Table              commit        disable       delete                       
    Drop Table              rollback      no op         no op                        
    
    Insert, drop table      commit        disable       delete             Notes: flag for preclose,   make this flag persistent in tmddl and region.
    Insert, drop table      rollback      no op         no op
    
    
    0. Commit thread and recovery thread do that same thing, either commit or abort.
    
    1. Commit thread is aware of other regions. Recovery thread is not aware of other regions.
       Commit thread doing commit or abort will perform end point calls to all regions involved followed by DDL. 
       Recovery thread doing commit or abort will perform DDL operation separate and region operation separate.
       Question is can DDL and DML commit processing be done in parallel or decoupled. Looks like its ok, MD dml 
       and actual DDL have same decision either to commit or abort.  
       One concern is visibility of the table based on corresponding MD changes being recovered( commit or abort)
       at different times. 
       
       With create table scenario, unwinding DDL and MD at different times does not matter since
       creation of new table from a new thread will get errors from Hmaster since table still exists in hmaster.
       One caviot is TM thinking table is created, but before creating TM dies. When recovery thread attempts to recover abort,
       it goes ahead and deletes the table( which some other thread already succeded in creating the table) which is very dangerous. 
       This scenario is handled as part of step 3 below.   
       
       With drop table scenario, unwinding drop DDL and MD at slightly different times is acceptable since:
       
       In general, whether recovery thread or commit thread is driving the commit or abort, visibility of the table
       is the same as originally intended. 
       
       Having DDL table and DML in the same table, having insertsDrop and createInserts flag persistent is needed for 
       redrive of commit or abort.
      
      
    2. Commit thread and recovery thread cannot overlap or do same execution in parallel. 
       Transaction status in Tlog must be the sole decision maker for recovery thread to redrive. If recovery thread
       finds a active transaction state object in memory, it assumes commit thread is handling it.
    
       
    3. Failure of TM and restart of TM can happen at any state of DDL operation in progress and before that operation is recorded as complete. 
       
       In case of create table,  TM dies before create table,  tx is now aborted.  Visibility of table is "Does not exist". However
       the recovery thread should not attempt to delete the table if it really did not create it ( some other thread might have created it in parallel).
       This is where we need a global semaphore on table name.
       
       In case of create table,  TM dies after create table,  tx is now aborted. Visibility of table is "Does not exist". recovery thread
       attempting to delete the table is ok here.
       
       In case of drop table, the visibility of the table at Hmaster prevents other threads from creating a duplicate table. There is scenario of
       accidently dropping a duplicate table.
    
       In general, a global semaphore kind of method helps to reduce the complexity and recover from various stages of failures.  
    
    */
   
     private static class RecoveryThread extends Thread{
             static final int SLEEP_DELAY = 1000; // Initially set to run every 1sec
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
             private boolean leadtm;
             private boolean takeover;
             HBaseTxClient hbtx;
             private static int envSleepTimeInt;
             private boolean ddlOnlyRecoveryCheck = true;

         static {
            String sleepTime = System.getenv("TMRECOV_SLEEP");
            if (sleepTime != null) 
               envSleepTimeInt = Integer.parseInt(sleepTime);
            else
               envSleepTimeInt = SLEEP_DELAY;
            LOG.info("Recovery thread sleep set to: " + envSleepTimeInt + " ms");
         }

         public RecoveryThread(TmAuditTlog audit,
                               HBaseTmZK zookeeper,
                               TransactionManager txnManager,
                               HBaseTxClient hbtx,
                               boolean useForgotten,
                               boolean forceForgotten,
                               boolean useTlog,
                               boolean leadtm,
                               boolean takeover) {
             this(audit, zookeeper, txnManager);
             this.hbtx = hbtx;
             this.useForgotten = useForgotten;
             this.forceForgotten = forceForgotten;
             this.useTlog= useTlog;
             this.leadtm = leadtm;
             this.takeover = takeover;
             if(LOG.isDebugEnabled()) LOG.debug("Traf Recovery Thread starts for DTM " + tmID +
                             " LDTM " + leadtm + " Takeover " + takeover);

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
                          this.sleepTimeInt = envSleepTimeInt;
            }

            public void stopThread() {
                 this.continueThread = false;
            }

            private void addRegionToTS(String hostnamePort, byte[] regionInfo, TransactionState ts) throws IOException{
                 HRegionInfo regionInfoLoc; // = new HRegionInfo();
                 final byte [] delimiter = ",".getBytes();
                 String[] result = hostnamePort.split(new String(delimiter), 3);

                 if (result.length < 2)
                         throw new IllegalArgumentException("Region array format is incorrect");

                 String hostname = result[0];
                 int port = Integer.parseInt(result[1]);
                 try {
                    regionInfoLoc = HRegionInfo.parseFrom(regionInfo);
                 } catch (DeserializationException de) {
                    throw new IOException(de);
                 }

                 String lv_hostname_port_string = hostname + ":" + port;
                 String lv_servername_string = ServerName.getServerName(lv_hostname_port_string, 0);
                 ServerName lv_servername = ServerName.parseServerName(lv_servername_string);

                 TransactionRegionLocation loc = new TransactionRegionLocation(regionInfoLoc, lv_servername);
                 ts.addRegion(loc);
             }

    @Override
    public void run() {
        
        //Start of recovery thread could be due to only two scenarios.
        //1. TM coming up as part of node up. In this case recovery thread 
        //   assigned node id is the same as own node.
        //2. As part of TM going down, LDTM starting a new recovery thread
        //   corresponding to the node id of TM that just went down. In this
        //   case, recovery thread node id is that of remote node that went down.
        //
        //In both scenarios, there may be DDL only recovery or DDL/DML region
        //recovery. DDL/DML region recovery is driven from the affected regions.
        //In the below loop, first DDL only recovery is checked first. This check
        //is performed only once in the beginning. Rest of the recovery is
        //is followed by DDL DML recovery check .
      
                while (this.continueThread) {
                    try {
                        skipSleep = false;
                        Map<String, byte[]> regions = null;
                        Map<Long, TransactionState> transactionStates = null;
                        boolean loopBack = false;
                        if(ddlOnlyRecoveryCheck)
                        {
                          ddlOnlyRecoveryCheck = false;
                          transactionStates = getTransactionsFromTmDDL();
                          if(transactionStates != null)
                            recoverTransactions(transactionStates);
                        }
                        
                        do {
                           try {
                               loopBack = false;
                               regions = zookeeper.checkForRecovery();
                           } catch(InterruptedException ie) {
                              loopBack = true;
                           } catch (KeeperException ze) {
                              loopBack = true;
                           }
                        } while (loopBack);

                        if(regions != null) {
                            skipSleep = true;
                            recoveryIterations++;

                            transactionStates = getTransactionsFromRegions(regions);
                            
                            if(transactionStates != null)
                                recoverTransactions(transactionStates);

                        } // region not null
                        else {
                            if (recoveryIterations > 0) {
                                if(LOG.isDebugEnabled()) LOG.debug("Recovery completed for TM" + tmID);
                            }
                            recoveryIterations = -1;
                        }
                        try {
                            if(continueThread) {
                                if (!skipSleep) {
                                   Thread.sleep(sleepTimeInt);
                                }
                            }
                            retryCount = 0;
                        } catch (InterruptedException e) {
                            LOG.error("Error in recoveryThread: ", e);
                        }
                    } catch (IOException e) {
                        int possibleRetries = 4;
                        LOG.error("Caught recovery thread exception for tmid: " + tmID + " retries: " + retryCount, e);

                        retryCount++;
                        if(retryCount > possibleRetries) {
                            LOG.error("Recovery thread failure, aborting process");
                            System.exit(4);
                        }

                        try {
                            Thread.sleep(SLEEP_DELAY / possibleRetries);
                        } catch(InterruptedException se) {
                        }
                    } catch (KeeperException ze) {
                        int possibleRetries = 4;
                        LOG.error("Caught recovery thread exception for tmid: " + tmID + " retries: " + retryCount, ze);

                        retryCount++;
                        if(retryCount > possibleRetries) {
                            LOG.error("Recovery thread failure, aborting process");
                            System.exit(4);
                        }

                        try {
                            Thread.sleep(SLEEP_DELAY / possibleRetries);
                        } catch(InterruptedException se) {
                        }
                    } catch (DeserializationException de) {
                        int possibleRetries = 4;
                        LOG.error("Caught recovery thread exception for tmid: " + tmID + " retries: " + retryCount, de);

                        retryCount++;
                        if(retryCount > possibleRetries) {
                            LOG.error("Recovery thread failure, aborting process");
                            System.exit(4);
                        }

                        try {
                            Thread.sleep(SLEEP_DELAY / possibleRetries);
                        } catch(InterruptedException se) {
                        }
                    }
                }
                if(LOG.isDebugEnabled()) LOG.debug("Exiting recovery thread for tm ID: " + tmID);
            }
            
    private  Map<Long, TransactionState> getTransactionsFromRegions(
                           Map<String, byte[]> regions)
                           throws IOException, KeeperException,
                           DeserializationException
    {
        if (LOG.isDebugEnabled()) LOG.debug("TRAF RCOV THREAD: in-doubt region size " + regions.size());
        for (Map.Entry<String, byte[]> regionEntry : regions.entrySet()) {
            Map<Long, TransactionState> transactionStates =
                            new HashMap<Long, TransactionState>();
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
            }
            catch (IOException e) {
               // For all cases of Exception, we rely on the region to redrive the request.
               // Likely there is nothing to recover, due to a stale region entry, but it is always safe to redrive.
               // We log a warning event and delete the ZKNode entry.
               LOG.warn("TRAF RCOV THREAD:Exception calling txnManager.recoveryRequest. " + "TM: " +
                          tmID + " regionBytes: [" + regionBytes + "].  Deleting zookeeper region entry. \n exception: ", e);
               zookeeper.deleteRegionEntry(regionEntry);
  
               // In the case of NotServingRegionException we will repost the ZKNode after refreshing the table.
               if ((e instanceof NotServingRegionException) || (e.getCause() instanceof NotServingRegionException)){
                   // Create a local HTable object using the regionInfo
                   HTable table = new HTable(config, HRegionInfo.parseFrom(regionBytes).getTable().getNameAsString());
                   // Repost a zookeeper entry for all current regions in the table
                   zookeeper.postAllRegionEntries(table);
               }
            } // IOException
  
            if (TxRecoverList != null) {
                if (LOG.isDebugEnabled()) LOG.trace("TRAF RCOV THREAD:size of TxRecoverList " + TxRecoverList.size());
                if (TxRecoverList.size() == 0) {
                     // First delete the zookeeper entry
                     LOG.warn("TRAF RCOV THREAD:Leftover Znode  calling txnManager.recoveryRequest. " + "TM: " +
                             tmID + " regionBytes: [" + regionBytes + "].  Deleting zookeeper region entry. ");
                     zookeeper.deleteRegionEntry(regionEntry);
               }
               for (Long txid : TxRecoverList) {
                  TransactionState ts = transactionStates.get(txid);
                  if (ts == null) {
                     ts = new TransactionState(txid);
  
                     //Identify if DDL is part of this transaction and valid
                     if(hbtx.useDDLTrans){
                        TmDDL tmDDL = hbtx.getTmDDL();
                        StringBuilder state = new StringBuilder ();
                        tmDDL.getState(txid,state);
                        if(state.toString().equals("VALID"))
                           ts.setDDLTx(true);
                     }
                  }
                  this.addRegionToTS(hostnamePort, regionBytes, ts);
                  transactionStates.put(txid, ts);
               }
            }
            else if (LOG.isDebugEnabled()) LOG.debug("TRAF RCOV THREAD:size od TxRecoverList is NULL ");
            
            return transactionStates;
        }
        return null;
    }
              
    private  Map<Long, TransactionState> getTransactionsFromTmDDL()
                                                throws IOException
    {
      if (LOG.isDebugEnabled()) LOG.debug("TRAF RCOV THREAD: Checking for DDL only recovery");
    
      //Access TMDDL, return null if not enabled.
      if(! hbtx.useDDLTrans)
        return null;
      
      Map<Long, TransactionState> transactionStates = null;
      TmDDL tmDDL = hbtx.getTmDDL();
      List<Long> txIdList  = tmDDL.getTxIdList(tmID);
      
      //This list of txID is specific to tmID owner.
      //This list may include txId that are:
      //1. currently in ACTIVE state. RecoverTransactions() call takes care of
      //ignoring TxId which are currently actively in progress.
      //2. Txids regions which have not yet requested for help(regions requesting help
      //from zookeeper) , probably will, could be timing. 
      //3. Txids regions which have already requested for help.
      //4. Txids whose regions have already serviced, but only require recovery
      //from DDL perspective.
      //For 2 and 3 use cases above, those regions will ultimately seek help if
      //they need help. So no need to handle those regions here. We are only
      //interested to handle use case 4. If usecase 4 also involves DML regions
      //it is ok to recover the DDL only here and not dependent on DML regions.
      //
      //Note that recoverTransactions() attempts recovery, its a no-op if those
      //txids are completed for some reason, some of the regions might have completed
      //processing, ignoreUnknownTransactionException is enabled.
      if(txIdList != null && txIdList.size() > 0)
      {
        transactionStates = new HashMap<Long, TransactionState>();
        for (Long txid : txIdList)
        {
          //build ts object
          TransactionState  ts = new TransactionState(txid);
          ts.setDDLTx(true);
          transactionStates.put(txid, ts);
        }
      }
      return transactionStates;
    }

    private void recoverTransactions(Map<Long, TransactionState> transactionStates) throws IOException
    {
        if (LOG.isDebugEnabled()) LOG.debug("TRAF RCOV THREAD: in-doubt transaction size " + transactionStates.size());
        
        for (Map.Entry<Long, TransactionState> tsEntry : transactionStates.entrySet()) {
            int isTransactionStillAlive = 0;
           TransactionState ts1 = null;
            TransactionState ts = tsEntry.getValue();
            Long txID = ts.getTransactionId();
            // TransactionState ts = new TransactionState(txID);
            
            //It is possible for long prepare situations that involve multiple DDL
            //operations, multiple prompts from RS is received. Hence check to see if there
            //is a TS object in main TS list and transaction is still active.
            //Note that tsEntry is local TS object. 
            if (hbtx.mapTransactionStates.get(txID) != null) {
              if (hbtx.mapTransactionStates.get(txID).getStatus().toString().contains("ACTIVE")) {
                isTransactionStillAlive = 1;
              }
              if (LOG.isInfoEnabled()) 
              LOG.info("TRAF RCOV THREAD: TID " + txID
                        + " still has TS object in TM memory. TS details: "
                        + hbtx.mapTransactionStates.get(txID).toString() 
                        + " transactionAlive: " + isTransactionStillAlive);
              if(isTransactionStillAlive == 1)
                continue; //for loop
            }
           
            try {
                audit.getTransactionState(ts);
                if (ts.getStatus().equals(TransState.STATE_COMMITTED.toString())) {
                    if (LOG.isDebugEnabled())
                        LOG.debug("TRAF RCOV THREAD:Redriving commit for " + txID + " number of regions " + ts.getParticipatingRegions().size() +
                                " and tolerating UnknownTransactionExceptions");
                    txnManager.doCommit(ts, true /*ignore UnknownTransactionException*/);
                    if(useTlog) {
                        long nextAsn = tLog.getNextAuditSeqNum(myClusterId, (int)TransactionState.getNodeId(txID));
                        tLog.putSingleRecord(txID,
                                             ts.getStartId(),
                                             ts.getCommitId(),
                                             TransState.STATE_RECOVERY_COMMITTED.toString(),
                                             ts.getParticipatingRegions(),
                                             false,
                                             forceForgotten,
                                             nextAsn);
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
  
            } catch (UnsuccessfulDDLException ddle) {
                LOG.error("UnsuccessfulDDLException encountered by Recovery Thread. Registering for retry. txID: " + txID + "Exception " , ddle);
  
                //Do not change the state of txId in tmDDL. Let the recovery thread
                //detect this txID again and redrive. Reset flag to loop back and
                //check for tmDDL again.
                ddlOnlyRecoveryCheck = true;
                
            } catch (CommitUnsuccessfulException cue) {
                LOG.error("CommitUnsuccessfulException encountered by Recovery Thread. Registering for retry. txID: " + txID + "Exception " , cue);
                
                //Do not change the state of txId in tmDDL. Let the recovery thread
                //detect this txID again and redrive. Reset flag to loop back and
                //check for tmDDL again.
                ddlOnlyRecoveryCheck = true;

            }
        }
      }//recoverTransactions()
          
   } //class RecoveryThread


     
     //================================================================================
     // DTMCI Calls
     //================================================================================

     //--------------------------------------------------------------------------------
     // callRequestRegionInfo
     // Purpose: Prepares HashMapArray class to get region information
     //--------------------------------------------------------------------------------
   public HashMapArray callRequestRegionInfo() throws IOException {

      String tablename = null;
      String encoded_region_name = null;
      String region_name = null;
      String is_offline = null;
      String region_id = null;
      String hostname = null;
      String port = null;
      String thn;
      String startkey, endkey;
      long lv_ret = -1;
      Long key;
      TransactionState value;
      int tnum = 0; // Transaction number

      if (LOG.isTraceEnabled()) LOG.trace(":callRequestRegionInfo:: start\n");

      HashMapArray hm = new HashMapArray();

      for (ConcurrentHashMap.Entry<Long, TransactionState> entry : mapTransactionStates.entrySet()) {
         key = entry.getKey();
         value = entry.getValue();
         long id = value.getTransactionId();

         TransactionState ts = mapTransactionStates.get(id);
         for (Map.Entry<String, HashMap<ByteArrayKey,TransactionRegionLocation>> tableMap : 
                        ts.getParticipatingRegions().getList().entrySet()) {
            // TableName
            tablename = tableMap.getKey();
            for (TransactionRegionLocation loc : tableMap.getValue().values()) {
               // Encoded Region Name
               if (encoded_region_name == null)
                  encoded_region_name = loc.getRegionInfo().getEncodedName();
               else
                  encoded_region_name = encoded_region_name + ";" + loc.getRegionInfo().getEncodedName();

               // Region Name
               if (region_name == null)
                  region_name = loc.getRegionInfo().getTable().getNameAsString();
               else
                  region_name = region_name + ";" + loc.getRegionInfo().getTable().getNameAsString();

               // Region Offline
               boolean is_offline_bool = loc.getRegionInfo().isOffline();
               is_offline = String.valueOf(is_offline_bool);

               // Region ID
               if (region_id == null)
                  region_id = String.valueOf(loc.getRegionInfo().getRegionId());
               else
                  region_id = region_id + ";" + loc.getRegionInfo().getRegionId();

               // Hostname
               if (hostname == null) {
                  thn = String.valueOf(loc.getHostname());
                  hostname = thn.substring(0, thn.length()-1);
               }
               else {
                  thn = String.valueOf(loc.getHostname());
                  hostname = hostname + ";" + thn.substring(0, thn.length()-1);
               }
   
               // Port
               if (port == null) 
                  port = String.valueOf(loc.getPort());
               else
                  port = port + ";" + String.valueOf(loc.getPort());
            }
            hm.addElement(tnum, "TableName", tablename);
            hm.addElement(tnum, "EncodedRegionName", encoded_region_name);
            hm.addElement(tnum, "RegionName", region_name);
            hm.addElement(tnum, "RegionOffline", is_offline);
            hm.addElement(tnum, "RegionID", region_id);
            hm.addElement(tnum, "Hostname", hostname);
            hm.addElement(tnum, "Port", port);
            tnum = tnum + 1;
        }
     }
     if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient::callRequestRegionInfo:: end size: " + hm.getSize());
     return hm;
   }
}

