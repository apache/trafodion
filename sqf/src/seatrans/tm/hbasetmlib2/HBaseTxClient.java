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
import org.apache.hadoop.hbase.client.transactional.UnknownTransactionException;
import org.apache.hadoop.hbase.client.transactional.HBaseBackedTransactionLogger;
import org.apache.hadoop.hbase.client.transactional.TransactionRegionLocation;
import org.apache.hadoop.hbase.client.transactional.TransState;
import org.apache.hadoop.hbase.client.transactional.TransReturnCode;
import org.apache.hadoop.hbase.client.transactional.TransactionMap;
import org.apache.hadoop.hbase.client.transactional.TransactionalReturn;
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
import org.trafodion.dtm.TmDDL;

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

   boolean useTlog;
   boolean useForgotten;
   boolean forceForgotten;
   boolean useRecovThread;
   boolean useDDLTrans;

   private static Configuration config;
   TransactionManager trxManager;
   static Map<Long, TransactionState> mapTransactionStates = TransactionMap.getInstance();
   Map<Integer, RecoveryThread> mapRecoveryThreads = new HashMap<Integer, org.trafodion.dtm.HBaseTxClient.RecoveryThread>();
   private final Object mapLock = new Object();

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
          trxManager.init();

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

   public long beginTransaction(final long transactionId) throws Exception
    {

      if (LOG.isDebugEnabled()) LOG.debug("Enter beginTransaction, txid: " + transactionId);
      TransactionState tx = trxManager.beginTransaction(transactionId);
      if(tx == null) {
    	  LOG.error("null Transaction State returned by the Transaction Manager, txid: " + transactionId);
    	  throw new Exception("TransactionState is null");
      }

      synchronized(mapLock) {
         mapTransactionStates.put(tx.getTransactionId(), tx);
      }

      if (LOG.isDebugEnabled()) LOG.debug("Exit beginTransaction, Transaction State: " + tx + " mapsize: " + mapTransactionStates.size());
     return tx.getTransactionId();
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
            tLog.putSingleRecord(transactionID, "ABORTED", ts.getParticipatingRegions(), false);
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
      if (useTlog && useForgotten) {
         if (forceForgotten) {
            tLog.putSingleRecord(transactionID, "FORGOTTEN", null, true);
         }
         else {
            tLog.putSingleRecord(transactionID, "FORGOTTEN", null, false);
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
             if (LOG.isTraceEnabled()) LOG.trace("Exit default RET_EXCEPTION prepareCommit, txid: " + transactionId);
             return TransReturnCode.RET_EXCEPTION.getShort();
        }
     } catch (IOException e) {
       LOG.error("Returning from HBaseTxClient:prepareCommit, txid: " + transactionId + " retval: " + TransReturnCode.RET_IOEXCEPTION.toString() + " IOException");
       return TransReturnCode.RET_IOEXCEPTION.getShort();
     } catch (CommitUnsuccessfulException e) {
       LOG.error("Returning from HBaseTxClient:prepareCommit, txid: " + transactionId + " retval: " + TransReturnCode.RET_NOCOMMITEX.toString() + " CommitUnsuccessfulException");
       return TransReturnCode.RET_NOCOMMITEX.getShort();
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

       try {
          ts.setStatus(TransState.STATE_COMMITTED);
          if (useTlog) {
             tLog.putSingleRecord(transactionId, "COMMITTED", ts.getParticipatingRegions(), true);
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
       if (useTlog && useForgotten) {
          if (forceForgotten) {
             tLog.putSingleRecord(transactionId, "FORGOTTEN", null, true);
          }
          else {
             tLog.putSingleRecord(transactionId, "FORGOTTEN", null, false);
          }
       }
//       mapTransactionStates.remove(transactionId);

       if (LOG.isTraceEnabled()) LOG.trace("Exit doCommit, retval(ok): " + TransReturnCode.RET_OK.toString() 
	   				+ " txid: " + transactionId + " mapsize: " + mapTransactionStates.size());

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
          ts.completeRequest();
       } catch(Exception e) {
          LOG.error("Returning from HBaseTxClient:completeRequest, ts.completeRequest: EXCEPTION" + " txid: " + transactionId);
       throw new Exception("Exception during completeRequest, unable to commit.");
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
         abortErr = abortTransaction(transactionId);
         if (LOG.isDebugEnabled()) LOG.debug("tryCommit commit failed and was aborted. Commit error " + 
                   commitErr + ", Abort error " + abortErr);
       }
       err = completeRequest(transactionId);
       if (err != TransReturnCode.RET_OK.getShort()) 
         if (LOG.isDebugEnabled()) LOG.debug("tryCommit completeRequest failed with error " + err);
       
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

   public short callCreateTable(long transactionId, byte[] pv_htbldesc) throws Exception
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
         trxManager.createTable(ts, htdesc);
      }
      catch (Exception cte) {
         if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient:callCreateTable exception trxManager.createTable, retval: " +
            TransReturnCode.RET_EXCEPTION.toString() +" txid: " + transactionId +" Exception: " + cte);
         StringWriter sw = new StringWriter();
         PrintWriter pw = new PrintWriter(sw);
         cte.printStackTrace(pw);
         LOG.error("HBaseTxClient createTable call error: " + sw.toString());
      }
      return TransReturnCode.RET_OK.getShort();
   }
   
    public short callRegisterRegion(long transactionId,
				    int  pv_port,
				    byte[] pv_hostname,
				    long pv_startcode,
				    byte[] pv_regionInfo) throws Exception {
 	String hostname    = new String(pv_hostname);
	if (LOG.isDebugEnabled()) LOG.debug("Enter callRegisterRegion, txid: [" + transactionId + "]");
	if (LOG.isTraceEnabled()) LOG.trace("callRegisterRegion, txid: [" + transactionId + "], port: " + pv_port + ", hostname: " + hostname + ", reg info len: " + pv_regionInfo.length + " " + new String(pv_regionInfo, "UTF-8"));

	HRegionInfo lv_regionInfo;
	try {
	    lv_regionInfo = HRegionInfo.parseFrom(pv_regionInfo);
	}
	catch (Exception de) {
           if (LOG.isTraceEnabled()) LOG.trace("HBaseTxClient:callRegisterRegion exception in lv_regionInfo parseFrom, retval: " +
             TransReturnCode.RET_EXCEPTION.toString() +
		     " txid: " + transactionId +
		     " DeserializationException: " + de);
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
          ts = trxManager.beginTransaction(transactionId);
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
       if (LOG.isTraceEnabled()) LOG.trace("Exit participatingRegions , txid: [" + transactionId + "] " + participants + " participants");
       return (ts.getParticipantCount() - ts.getRegionsToIgnoreCount());
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
      if (LOG.isTraceEnabled()) LOG.trace("Exit addControlPoint, returning: " + result);
      return result;
   }
   
     /**
      * Thread to gather recovery information for regions that need to be recovered 
      */
     private static class RecoveryThread extends Thread{
             final int SLEEP_DELAY = 10000; // Initially set to run every 10sec
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
             }

             public void stopThread() {
                 this.continueThread = false;
             }
             
             private void addRegionToTS(String hostnamePort, byte[] regionInfo,
            		                    TransactionState ts) throws Exception{
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

                 TransactionRegionLocation loc = new TransactionRegionLocation(regionInfoLoc,
									       lv_servername);
                 ts.addRegion(loc);
             }

            @Override
             public void run() {
                int sleepTimeInt = 0;
                String sleepTime = System.getenv("TMRECOV_SLEEP");
                if (sleepTime != null)
                    sleepTimeInt = Integer.parseInt(sleepTime);

                while (this.continueThread) {
                    try {
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
                                            tLog.putSingleRecord(txID, "FORGOTTEN", null, forceForgotten, nextAsn);
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

                                } catch (Exception e) {
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
                                if (sleepTimeInt > 0)
                                    Thread.sleep(sleepTimeInt);
                                else
                                    Thread.sleep(SLEEP_DELAY);
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

