// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.transactional.TransactionManager;
import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.client.transactional.CommitUnsuccessfulException;
import org.apache.hadoop.hbase.client.transactional.UnknownTransactionException;
import org.apache.hadoop.hbase.client.transactional.HBaseBackedTransactionLogger;
import org.apache.hadoop.hbase.client.transactional.TransactionRegionLocation;
import org.apache.hadoop.hbase.client.transactional.TransactionalReturn;
import org.apache.hadoop.hbase.exceptions.DeserializationException;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.NotServingRegionException;
import org.apache.hadoop.hbase.ServerName;
import org.apache.hadoop.hbase.util.Bytes;
import org.trafodion.dtm.HBaseTmZK;
import org.trafodion.dtm.TmAuditTlog;

import java.util.Map;
import java.util.HashMap;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentHashMap;

public class HBaseTxClient {

   static final Log LOG = LogFactory.getLog(HBaseTxClient.class);
   private static TmAuditTlog tLog;
   private static HBaseTmZK tmZK;
   private static RecoveryThread recovThread;
   private short dtmID;
   private static int stallWhere;

   boolean useTlog;
   boolean useForgotten;
   boolean forceForgotten;
   boolean useRecovThread;

   private static Configuration config;
   TransactionManager trxManager;
   Map<Long, TransactionState> mapTransactionStates = new HashMap<Long, TransactionState>();
   private final Object mapLock = new Object();

   public static final int RET_OK = 0;
   public static final int RET_EXCEPTION = 4;
   public static final int RET_READONLY = 2;
   public static final int RET_NOTX = 1;
   public static final int RET_PARAMERR = 3;
   public static final int RET_HASCONFLICT = 5;
   public static final int RET_IOEXCEPTION = 6;
   public static final int RET_NOCOMMITEX  = 7;
   public static final int TM_TX_STATE_NOTX = 0; //S0 - NOTX
   public static final int TM_TX_STATE_ACTIVE = 1; //S1 - ACTIVE
   public static final int TM_TX_STATE_IDLE = 14; //S2 - IDLE XARM Branches only!
   public static final int TM_TX_STATE_FORGOTTEN = 2; //N/A
   public static final int TM_TX_STATE_COMMITTED = 3; //N/A
   public static final int TM_TX_STATE_ABORTING = 4; //S4 - ROLLBACK
   public static final int TM_TX_STATE_ABORTED = 5; //S4 - ROLLBACK
   public static final int TM_TX_STATE_COMMITTING = 6; //S3 - PREPARED
   public static final int TM_TX_STATE_PREPARING = 7; //S2 - IDLE
   public static final int TM_TX_STATE_PREPARED = 9; //S3 - PREPARED XARM Branches only!
   public static final int TM_TX_STATE_FORGETTING = 8; //N/A
   public static final int TM_TX_STATE_FORGETTING_HEUR = 10; //S5 - HEURISTIC
   public static final int TM_TX_STATE_FORGOTTEN_HEUR = 15; //S5 - HEURISTIC - Waiting Superior TM xa_forget request
   public static final int TM_TX_STATE_BEGINNING = 11; //S1 - ACTIVE
   public static final int TM_TX_STATE_HUNGCOMMITTED = 12; //N/A
   public static final int TM_TX_STATE_HUNGABORTED = 13; //S4 - ROLLBACK
   public static final int TM_TX_STATE_ABORTING_PART2 = 16; // Internal State
   public static final int TM_TX_STATE_TERMINATING = 17;
   public static final int TM_TX_STATE_LAST = 17;

    void setupLog4j() {
       System.out.println("In setupLog4J");
        String confFile = System.getenv("MY_SQROOT")
            + "/logs/log4j.dtm.config";
        PropertyConfigurator.configure(confFile);
    }

   public boolean init(String hBasePath, String zkServers, String zkPort) throws Exception {
      System.out.println("In init - hbp");
      setupLog4j();
      if (LOG.isDebugEnabled()) LOG.debug("Enter init, hBasePath:" + hBasePath);
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
            trxManager = new TransactionManager(config);
      } catch (IOException e ){
            LOG.error("Unable to create TransactionManager, throwing exception");
            throw new RuntimeException(e);
      }

      if (useRecovThread) {
         if (LOG.isDebugEnabled()) LOG.debug("Entering recovThread Usage");
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
      System.out.println("In init - dtmId" + dtmid);

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
            trxManager = new TransactionManager(config);
      } catch (IOException e ){
            LOG.error("Unable to create TransactionManager, Exception: " + e + "throwing new RuntimeException");
            throw new RuntimeException(e);
      }

      if (useRecovThread) {
         if (LOG.isDebugEnabled()) LOG.debug("Entering recovThread Usage");
          try {                                                                          
              tmZK = new HBaseTmZK(config, dtmID);                              
          }catch (IOException e ){                                                       
              LOG.error("Unable to create HBaseTmZK TM-zookeeper class, throwing exception");
              throw new RuntimeException(e);                                             
          }                                                                              
          recovThread = new RecoveryThread(tLog, tmZK, trxManager);                      
          recovThread.start();                     
      }
      if (LOG.isTraceEnabled()) LOG.trace("Exit init()");
      return true;
   }

   public short stall (int where) {
      if (LOG.isDebugEnabled()) LOG.debug("Entering stall with parameter " + where);
      this.stallWhere = where;
      return RET_OK;
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
          LOG.error("Returning from HBaseTxClient:abortTransaction, txid: " + transactionID + " retval: " + RET_NOTX);
          return RET_NOTX;
      }

      try {
         ts.setStatus(TM_TX_STATE_ABORTED);
         if (useTlog) {
            tLog.putSingleRecord(transactionID, "ABORTED", ts.getParticipatingRegions(), false);
         }
      } catch(Exception e) {
         LOG.error("Returning from HBaseTxClient:abortTransaction, txid: " + transactionID + " tLog.putRecord: EXCEPTION");
         return RET_EXCEPTION;
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
          return RET_EXCEPTION;
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
      return RET_OK;
   }

   public short prepareCommit(long transactionId) throws Exception {
     if (LOG.isDebugEnabled()) LOG.debug("Enter prepareCommit, txid: " + transactionId);
     TransactionState ts = mapTransactionStates.get(transactionId);
     if(ts == null) {
       LOG.error("Returning from HBaseTxClient:prepareCommit, txid: " + transactionId + " retval: " + RET_NOTX);
       return RET_NOTX; 
     }

     try {
        short result = (short) trxManager.prepareCommit(ts);
        if (LOG.isDebugEnabled()) LOG.debug("prepareCommit, [ " + ts + " ], result " + result + ((result == TransactionalReturn.COMMIT_OK_READ_ONLY)?", Read-Only":""));
        switch (result) {
          case TransactionalReturn.COMMIT_OK:
              if (LOG.isTraceEnabled()) LOG.trace("Exit OK prepareCommit, txid: " + transactionId);
              return RET_OK;
          case TransactionalReturn.COMMIT_OK_READ_ONLY:
             synchronized(mapLock) {
                mapTransactionStates.remove(transactionId);
             }
             if (LOG.isTraceEnabled()) LOG.trace("Exit OK_READ_ONLY prepareCommit, txid: " + transactionId);
             return RET_READONLY;
          case TransactionalReturn.COMMIT_CONFLICT:
             LOG.info("Exit RET_HASCONFLICT prepareCommit, txid: " + transactionId);
             return RET_HASCONFLICT;
          default:
             if (LOG.isTraceEnabled()) LOG.trace("Exit RET_PARAMERR prepareCommit, txid: " + transactionId);
             return RET_PARAMERR;
        }
     } catch (IOException e) {
  	   LOG.error("Returning from HBaseTxClient:prepareCommit, txid: " + transactionId + " retval: " + RET_IOEXCEPTION + " IOException");
  	   return RET_IOEXCEPTION;
     } catch (CommitUnsuccessfulException e) {
  	   LOG.error("Returning from HBaseTxClient:prepareCommit, txid: " + transactionId + " retval: " + RET_NOCOMMITEX + " CommitUnsuccessfulException");
  	   return RET_NOCOMMITEX;
     }
     catch (Exception e) {
           LOG.error("Returning from HBaseTxClient:prepareCommit, txid: " + transactionId + " retval: " + RET_NOCOMMITEX + " Exception " + e);
           return RET_NOCOMMITEX;
     }
   }

   public short doCommit(long transactionId) throws Exception {
       if (LOG.isDebugEnabled()) LOG.debug("Enter doCommit, txid: " + transactionId);
       TransactionState ts = mapTransactionStates.get(transactionId);

       if(ts == null) {
	  LOG.error("Returning from HBaseTxClient:doCommit, (null tx) retval: " + RET_NOTX + " txid: " + transactionId);
          return RET_NOTX;
       }

       try {
          ts.setStatus(TM_TX_STATE_COMMITTED);
          if (useTlog) {
             tLog.putSingleRecord(transactionId, "COMMITTED", ts.getParticipatingRegions(), true);
          }
       } catch(Exception e) {
          LOG.error("Returning from HBaseTxClient:doCommit, txid: " + transactionId + " tLog.putRecord: EXCEPTION " + e);
          return RET_EXCEPTION;
       }

       if ((stallWhere == 2) || (stallWhere == 3)) {
          LOG.info("Stalling in phase 2 for doCommit");
          Thread.sleep(300000); // Initially set to run every 5 min                                 
       }

       try {
          trxManager.doCommit(ts);
       } catch (CommitUnsuccessfulException e) {
          LOG.error("Returning from HBaseTxClient:doCommit, retval: " + RET_EXCEPTION + " IOException" + " txid: " + transactionId);
          return RET_EXCEPTION;
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

       if (LOG.isTraceEnabled()) LOG.trace("Exit doCommit, retval(ok): " + RET_OK + " txid: " + transactionId + " mapsize: " + mapTransactionStates.size());

       return RET_OK;
   }

   public short completeRequest(long transactionId) throws Exception {
     if (LOG.isDebugEnabled()) LOG.debug("Enter completeRequest, txid: " + transactionId);
     TransactionState ts = mapTransactionStates.get(transactionId);

     if(ts == null) {
          LOG.error("Returning from HBaseTxClient:completeRequest, (null tx) retval: " + RET_NOTX + " txid: " + transactionId);
          return RET_NOTX;
       }

       try {
          ts.completeRequest();
       } catch(Exception e) {
          LOG.error("Returning from HBaseTxClient:doCommit, ts.completeRequest: EXCEPTION" + " txid: " + transactionId);
       throw new Exception("Exception during completeRequest, unable to commit.");
       }

     synchronized(mapLock) {
        mapTransactionStates.remove(transactionId);
     }

     if (LOG.isDebugEnabled()) LOG.debug("Exit completeRequest txid: " + transactionId + " mapsize: " + mapTransactionStates.size());
     return RET_OK;
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
		     RET_EXCEPTION +
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
		    " retval: " + RET_EXCEPTION + " IOException " + e);
          return RET_EXCEPTION;
       }

       if (LOG.isDebugEnabled()) LOG.debug("RegisterRegion adding table name " + regionTableName);
       ts.addTableName(regionTableName);

       // Removing unnecessary put back into the map
       // mapTransactionStates.put(ts.getTransactionId(), ts);

       if (LOG.isTraceEnabled()) LOG.trace("Exit callRegisterRegion, txid: [" + transactionId + "] with mapsize: "
                  + mapTransactionStates.size());
       return RET_OK;
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
             final int SLEEP_DELAY = 180000; // Initially set to run every 3min
             private TmAuditTlog audit;
             private HBaseTmZK zookeeper;
             private TransactionManager txnManager;
             private short tmID;
             private Set<Long> inDoubtList;

             /**
              * 
              * @param audit
              * @param zookeeper
              * @param txnManager
              */
             public RecoveryThread(TmAuditTlog audit, HBaseTmZK zookeeper,
                             TransactionManager txnManager) {
                          this.audit = audit;
                          this.zookeeper = zookeeper;
                          this.txnManager = txnManager;
                          this.inDoubtList = new HashSet<Long> ();
                          this.tmID = zookeeper.getTMID();
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
                     
                     if (LOG.isDebugEnabled()) LOG.debug("Starting recovery thread for TM" + tmID);
                     while(true) {
                             Map<String, byte []> regions = new HashMap<String, byte []>();
                             Map<Long, TransactionState> transactionStates = 
                            		 new HashMap<Long, TransactionState>();                             		 
                             try {                                    
                                     regions = zookeeper.checkForRecovery();
                                     if(regions != null) 
                                         if (LOG.isTraceEnabled()) LOG.trace("Processing " + regions.size() + " regions");
                             } catch (Exception e) {
                                     LOG.error("An ERROR occurred while checking for regions to recover. " + "TM: " + tmID);
                                     StringWriter sw = new StringWriter();
                                     PrintWriter pw = new PrintWriter(sw);
                                     e.printStackTrace(pw);
                                     LOG.error(sw.toString()); 
                             }
                             
                             if (LOG.isDebugEnabled()) LOG.debug("in-doubt region size " + regions.size());
                             for(Map.Entry<String,byte[]> region : regions.entrySet()) {  
                            	     List<Long> TxRecoverList = new ArrayList<Long>();
                                     if (LOG.isDebugEnabled()) LOG.debug("BBB Processing region: " + new String(region.getValue()));
                                     String hostnamePort = region.getKey();
                                     byte [] regionInfo =  region.getValue();                                    
                                            try {
                                         TxRecoverList = txnManager.recoveryRequest(hostnamePort, regionInfo, tmID);
                                     } catch (Exception e) {
                                         LOG.error("Error calling recoveryRequest " + new String(regionInfo) + " TM " + tmID);
                                         e.printStackTrace();
                                     }
				                     for(Long txid : TxRecoverList) {
				                    	 TransactionState ts = transactionStates.get(txid);
				                    	 if(ts == null) {
				                    		 ts = new TransactionState(txid);
				                    	 }
				                   /* 	 try {
				                    		 this.addRegionToTS(hostnamePort, regionInfo, ts);
				                    	 } catch (Exception e) {
				                    		 LOG.error("Unable to add region to TransactionState" +
				                    		 		"region info: " + new String(regionBytes));
				                    		 e.printStackTrace();
				                    	 } */
				                    	 transactionStates.put(txid, ts);
				                     }
                                     }

                             for(Map.Entry<Long, TransactionState> tsEntry: transactionStates.entrySet()) {
                            	   TransactionState ts = tsEntry.getValue();
                            	   Long txID = tsEntry.getKey();
                                   // TransactionState ts = new TransactionState(txID);
                                   try {
                                           audit.getTransactionState(ts);
                                           if(ts.getStatus().equals("COMMITTED")) {
                                                   if (LOG.isDebugEnabled()) LOG.debug("Redriving commit for " + ts.getTransactionId() + " number of regions " + ts.getParticipatingRegions().size() +
                                                             " and tolerating UnknownTransactionExceptions" );
                                                   txnManager.doCommit(ts, true /*ignore UnknownTransactionException*/);
                                           }
                                           else if(ts.getStatus().equals("ABORTED")) {
                                                   if (LOG.isDebugEnabled()) LOG.debug("Redriving abort for " + ts.getTransactionId());
                                                   txnManager.abort(ts);
                                           }
                                           else {
                                                   if (LOG.isDebugEnabled()) LOG.debug("Redriving abort for " + ts.getTransactionId());
                                                   LOG.warn("Recovering transaction " + txID + ", status is not set to COMMITTED or ABORTED. Aborting.");
                                                   txnManager.abort(ts);
                                           }

                                   }catch (Exception e) {
                                           LOG.error("Unable to get audit record for tx: " + txID + ", audit is throwing exception.");
                                           e.printStackTrace();
                                   }
                             }

                             try {
                            	     if(sleepTimeInt > 0) 
                            	    	 Thread.sleep(sleepTimeInt);
                            	     else
                            	    	 Thread.sleep(SLEEP_DELAY);                                     
                             } catch (Exception e) {
                                     e.printStackTrace();
                             }
                     }
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

