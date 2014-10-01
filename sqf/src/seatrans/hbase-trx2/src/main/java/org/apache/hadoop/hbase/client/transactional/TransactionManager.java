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
package org.apache.hadoop.hbase.client.transactional;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.CompletionService;
import java.util.concurrent.ExecutorCompletionService;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.io.PrintWriter;
import java.io.StringWriter;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.NotServingRegionException;
import org.apache.hadoop.hbase.ZooKeeperConnectionException;
import org.apache.hadoop.hbase.client.HConnection;
import org.apache.hadoop.hbase.client.HConnectionManager;
import org.apache.hadoop.hbase.ipc.TransactionalRegionInterface;
import org.apache.hadoop.hbase.util.EnvironmentEdgeManager;

/**
 * Transaction Manager. Responsible for committing transactions.
 */
public class TransactionManager {

    static final Log LOG = LogFactory.getLog(TransactionManager.class);

    private final HConnection connection;
    private final TransactionLogger transactionLogger;
    private JtaXAResource xAResource;

    public static final int TM_COMMIT_FALSE = 0;     
    public static final int TM_COMMIT_READ_ONLY = 1; 
    public static final int TM_COMMIT_TRUE = 2;
    public static final int TM_COMMIT_FALSE_CONFLICT = 3; 
    
    /** 
     * TransactionManagerCallable  :  inner class for creating asynchronous requests
     */
    private abstract class TransactionManagerCallable implements Callable<Integer>{
    	
	TransactionState transactionState;
	TransactionalRegionInterface transactionRS;
	
	TransactionManagerCallable(TransactionState txState, TransactionalRegionInterface txRegionServer) {
	transactionState = txState;
	transactionRS = txRegionServer;
   }
		
	/**
	 * Method  : doCommitX 
	 * Params  : regionName - name of Region
	 *           transactionId - transaction identifier
	 * Return  : Always 0, can ignore
	 * Purpose : Call commit for a given regionserver  
	 */
    public Integer doCommitX(final byte[] regionName, final long transactionId) throws CommitUnsuccessfulException {
        try {
	    transactionRS.commit(regionName,transactionId);
	}catch (Exception e) { 
	        LOG.error("exception in doCommitX for transaciton: " + transactionId + ": " + e);
		// We have received our reply in the form of an exception,
	        // so decrement outstanding count and wake up waiters to avoid
	        // getting hung forever
		transactionState.requestPendingCountDec(true);
                throw new CommitUnsuccessfulException(e);
        }
			
	// We have received our reply so decrement outstanding count
	transactionState.requestPendingCountDec(false);
   			
	// forget the transaction if all replies have been received. otherwise another thread
	// will do it.
	if (transactionState.requestAllComplete())
	{
		transactionLogger.forgetTransaction(transactionState.getTransactionId());
	}
	return 0;
    }
		
	/**
	 * Method  : doPrepareX 
	 * Params  : regionName - name of Region
	 *           transactionId - transaction identifier
	 *           location - 
	 * Return  : Commit vote (yes, no, read only)
	 * Purpose : Call prepare for a given regionserver  
	 */
    public Integer doPrepareX(final byte[] regionName, final long transactionId, final HRegionLocation location) 
	throws IOException, CommitUnsuccessfulException {

        int commitStatus;
 
        try {			
	    commitStatus = transactionRS.commitRequest(regionName,transactionId);
        }
        catch(IOException e) {
           LOG.warn("Received IOException " + e + " from commitRequest for transaction " + transactionId + " rethrowing exception");
           e.printStackTrace();
           throw e;
        }

	boolean canCommit = true;
    boolean readOnly = false;
              
        switch (commitStatus) {
            case TransactionalRegionInterface.COMMIT_OK:
                break;
            case TransactionalRegionInterface.COMMIT_OK_READ_ONLY:
                transactionState.addRegionToIgnore(location); // No need to doCommit for read-onlys
                readOnly = true;
                 break;
            case TransactionalRegionInterface.COMMIT_CONFLICT:
            case TransactionalRegionInterface.COMMIT_UNSUCCESSFUL:
                 canCommit = false;
                 transactionState.addRegionToIgnore(location); // No need to re-abort.
                 break;
	    default:
                 LOG.warn("Received invalid return code from requestCommit " + commitStatus + " for transaction " + transactionId + " throwing CommitUnsuccessfulException");

                 throw new CommitUnsuccessfulException("Unexpected return code from prepareCommit: "
                  + commitStatus);
         }

              
         if (LOG.isTraceEnabled()) {
             LOG.trace("Region [" + location.getRegionInfo().getRegionNameAsString() + "] votes "
                 + (canCommit ? "to commit" : "to abort") + (readOnly ? " Read-only ":"") + " transaction "
                 + transactionState.getTransactionId());
         }
              
         if (!canCommit) {
        	 // track regions which indicate they could not commit for better diagnostics
             LOG.warn("Region [" + location.getRegionInfo().getRegionNameAsString() + "] votes "
                     +  "to abort" + (readOnly ? " Read-only ":"") + " transaction "
                     + transactionState.getTransactionId());
             //System.out.println("Region [" + location.getRegionInfo().getRegionNameAsString() + "] votes "
             //        +  "to abort" + (readOnly ? " Read-only ":"") + " transaction "
             //        + transactionState.getTransactionId());
             if(commitStatus == TransactionalRegionInterface.COMMIT_CONFLICT)
                  return TM_COMMIT_FALSE_CONFLICT;
             return TM_COMMIT_FALSE;
         }
              
         if (readOnly)
             return TM_COMMIT_READ_ONLY;
              
  	 return TM_COMMIT_TRUE;
			
    }
		
	/**
	 * Method  : doAbortX 
	 * Params  : regionName - name of Region
	 *           transactionId - transaction identifier
	 * Return  : Ignored
	 * Purpose : Call abort for a given regionserver  
	 */
    public Integer doAbortX(final byte[] regionName, final long transactionId) {
            
	try {
	    transactionRS.abortTransaction(regionName,transactionId);
	} catch (Exception e) //ignore
        {        	
	         LOG.error("exception in doAbortX (ignoring): " + e);
	       	 LOG.info("Got unknown exception during abort. Transaction: ["
                     + transactionState.getTransactionId() + "]");
        }
	// We have received our reply so decrement outstanding count
	transactionState.requestPendingCountDec(false);
						
	// forget the transaction if all replies have been received. 
	//  otherwise another thread will do it
	if (transactionState.requestAllComplete())
	{
		transactionLogger.forgetTransaction(transactionState.getTransactionId());
	}
	return 0;
     }
  }
    
    /**
     * threadPool - pool of thread for asynchronous requests
     */
	ExecutorService threadPool;

	/**
     * @param conf
     * @throws ZooKeeperConnectionException
     */
    public TransactionManager(final Configuration conf) throws ZooKeeperConnectionException {
        this(LocalTransactionLogger.getInstance(), conf);
        int intThreads = 16;
        String numThreads = System.getenv("TM_JAVA_THREAD_POOL_SIZE");
        if (numThreads != null)
            intThreads = Integer.parseInt(numThreads);
        threadPool = Executors.newFixedThreadPool(intThreads);
    }

    /**
     * @param transactionLogger
     * @param conf
     * @throws ZooKeeperConnectionException
     */
    public TransactionManager(final TransactionLogger transactionLogger, final Configuration conf)
            throws ZooKeeperConnectionException {
        this.transactionLogger = transactionLogger;
        connection = HConnectionManager.getConnection(conf);
    }


    /**
     * Called to start a transaction.
     * 
     * @return new transaction state
     */
    public TransactionState beginTransaction() {
        long transactionId = transactionLogger.createNewTransactionLog();
        LOG.trace("Beginning transaction " + transactionId);
        return new TransactionState(transactionId);
    }

    /**
     * Called to start a transaction with transactionID input
     *
     * @return new transaction state
     */
    public TransactionState beginTransaction(long transactionId) {
        //long transactionId =
      LOG.trace("Enter beginTransaction, txid: " + transactionId);
      if(transactionLogger.createNewTransactionLog(transactionId) == 0) {
	      LOG.error("beginTransaction, error in createNewTransactionLog");
              // Error creating new txn log, throw exception
              return null;
      }
        return new TransactionState(transactionId);
    }

    /**
     * Prepare to commit a transaction.
     * 
     * @param transactionState
     * @return commitStatusCode (see {@link TransactionalRegionInterface})
     * @throws IOException
     * @throws CommitUnsuccessfulException
     */
    public int prepareCommit(final TransactionState transactionState) throws CommitUnsuccessfulException, IOException {
        boolean allReadOnly = true;
        int loopCount = 0;
        
        // (need one CompletionService per request for thread safety, can share pool of threads
    	CompletionService<Integer> compPool;
    	compPool = new ExecutorCompletionService<Integer>(threadPool); 
        try {
            for (TransactionRegionLocation location : transactionState.getParticipatingRegions()) {

            	loopCount++;
             final TransactionRegionLocation myLocation = location;
             final byte[] regionName = location.getRegionInfo().getRegionName();
             TransactionalRegionInterface transactionalRegionServer = (TransactionalRegionInterface) connection
                        .getHRegionConnection(location.getServerAddress());

             compPool.submit(new TransactionManagerCallable(transactionState, transactionalRegionServer) {
    		public Integer call() throws IOException, CommitUnsuccessfulException {
    		return doPrepareX(regionName, transactionState.getTransactionId(), myLocation);
    	        }
               });
           }
        } catch (Exception e) {        	
	 LOG.error("exception in prepareCommit (during submit to pool): " + e);
         // This happens on a NSRE that is triggered by a split
         try {
             abort(transactionState);
         } catch (Exception abortException) {
             LOG.warn("Exception during abort", abortException);
         }
           throw new CommitUnsuccessfulException(e);
        }

 	    // loop to retrieve replies
        try {
            for (int loopIndex = 0; loopIndex < loopCount; loopIndex ++) {
        	int canCommit = compPool.take().get();
      	
        	if (canCommit == TM_COMMIT_FALSE) {
        		LOG.warn("Aborting [" + transactionState.getTransactionId() + "]");
        		abort(transactionState);
        		return TransactionalRegionInterface.COMMIT_UNSUCCESSFUL;
        	}
        	if(canCommit == TM_COMMIT_FALSE_CONFLICT) {
        		LOG.warn("Aborting [" + transactionState.getTransactionId() + "] due to Conflict");
        		abort(transactionState);
        		return TransactionalRegionInterface.COMMIT_CONFLICT;
        	}
        	else if (canCommit == TM_COMMIT_TRUE)
        		allReadOnly = false;
       	}
        }catch (Exception e) {        	
	    LOG.error("exception in prepareCommit (during completion processing): " + e);
            try {
            	
                abort(transactionState);
            } catch (Exception abortException) {
                LOG.warn("Exception during abort", abortException);
            }
            throw new CommitUnsuccessfulException(e);
        }
        
        return allReadOnly ? TransactionalRegionInterface.COMMIT_OK_READ_ONLY : TransactionalRegionInterface.COMMIT_OK;
    }

    /**
     * Try and commit a transaction. This does both phases of the 2-phase protocol: prepare and commit.
     * 
     * @param transactionState
     * @throws IOException
     * @throws CommitUnsuccessfulException
     */
    public void tryCommit(final TransactionState transactionState) throws CommitUnsuccessfulException, IOException {
        long startTime = EnvironmentEdgeManager.currentTimeMillis();
        LOG.trace("atempting to commit transaction: " + transactionState.toString());
        int status = prepareCommit(transactionState);

        if (status == TransactionalRegionInterface.COMMIT_OK) {
            doCommit(transactionState);
        } else if (status == TransactionalRegionInterface.COMMIT_OK_READ_ONLY) {
        	// no requests sent for fully read only transaction
    		transactionState.completeSendInvoke(0);
            transactionLogger.forgetTransaction(transactionState.getTransactionId());
        } else if (status == TransactionalRegionInterface.COMMIT_UNSUCCESSFUL) {
            // We have already aborted at this point
            throw new CommitUnsuccessfulException();
        }
        LOG.trace("Committed transaction [" + transactionState.getTransactionId() + "] in ["
                + ((EnvironmentEdgeManager.currentTimeMillis() - startTime)) + "]ms");
    }

    /**
     * Do the commit. This is the 2nd phase of the 2-phase protocol.
     * 
     * @param transactionState
     * @throws CommitUnsuccessfulException
     */
    public void doCommit(final TransactionState transactionState) throws CommitUnsuccessfulException {
    	int loopCount = 0;
        try {
            LOG.trace("Committing [" + transactionState.getTransactionId() + "]");

            transactionLogger.setStatusForTransaction(transactionState.getTransactionId(),
                TransactionLogger.TransactionStatus.COMMITTED);

            // (Asynchronously send commit
            for (TransactionRegionLocation location : transactionState.getParticipatingRegions()) {
                if (transactionState.getRegionsToIngore().contains(location)) {
                    continue;
                }

                loopCount++;
            	final byte[] regionName = location.getRegionInfo().getRegionName();              
                TransactionalRegionInterface transactionalRegionServer = (TransactionalRegionInterface) connection
                        .getHRegionConnection(location.getServerAddress());
                    
                threadPool.submit(new TransactionManagerCallable(transactionState, transactionalRegionServer) {
    			public Integer call() throws CommitUnsuccessfulException {
    			return doCommitX(regionName, transactionState.getTransactionId());
    			}
                });
                
                
            }
        } catch (Exception e) {
	    LOG.error("exception in doCommit : " + e);
            LOG.info("Commit of transaction [" + transactionState.getTransactionId() + "] was unsucsessful", e);
            // This happens on a NSRE that is triggered by a split
/*            try {
                abort(transactionState);
            } catch (Exception abortException) {

                LOG.warn("Exeption during abort", abortException);
            }
*/
            throw new CommitUnsuccessfulException(e);
        }
        
        // all requests sent at this point, can record the count
		transactionState.completeSendInvoke(loopCount);

    }

    /**
     * Abort a s transaction.
     * 
     * @param transactionState
     * @throws IOException
     */
    public void abort(final TransactionState transactionState) throws IOException {
    	int loopCount = 0;

        transactionLogger.setStatusForTransaction(transactionState.getTransactionId(),
            TransactionLogger.TransactionStatus.ABORTED);
        
        // (Asynchronously send aborts
        for (TransactionRegionLocation location : transactionState.getParticipatingRegions()) {
            if (transactionState.getRegionsToIngore().contains(location)) {
                continue;
            }
            try {
            	loopCount++;
            	final byte[] regionName = location.getRegionInfo().getRegionName();
                TransactionalRegionInterface transactionalRegionServer = (TransactionalRegionInterface) connection
                        .getHRegionConnection(location.getServerAddress());
                
                threadPool.submit(new TransactionManagerCallable(transactionState, transactionalRegionServer) {
    			public Integer call() throws IOException {
    				return doAbortX(regionName, transactionState.getTransactionId());
    			}
                });

            } catch (UnknownTransactionException e) {
		LOG.error("exception in abort: " + e);
                LOG.info("Got unknown transaction exception during abort. Transaction: ["
                        + transactionState.getTransactionId() + "], region: ["
                        + location.getRegionInfo().getRegionNameAsString() + "]. Ignoring.");
            } catch (NotServingRegionException e) {
                LOG.info("Got NSRE during abort. Transaction: [" + transactionState.getTransactionId() + "], region: ["
                        + location.getRegionInfo().getRegionNameAsString() + "]. Ignoring.");
            }
        }
         
        // all requests sent at this point, can record the count
        transactionState.completeSendInvoke(loopCount);
        
    }

    public synchronized JtaXAResource getXAResource() {
        if (xAResource == null) {
            xAResource = new JtaXAResource(this);
        }
        return xAResource;
    }
    
    public void registerRegion(final TransactionState transactionState, TransactionRegionLocation location)throws IOException{
        LOG.trace("registerRegion ENTRY, transactioState:" + transactionState);
    	if(transactionState.addRegion(location)){
	    LOG.trace("registerRegion -- adding region: " + location.getRegionInfo().getRegionNameAsString());
    	}
        LOG.trace("registerRegion EXIT");
    }
    
    /**
     * @param hostnamePort
     * @param regionArray
     * @param tmid
     * @return
     * @throws Exception
     */
    public List<Long> recoveryRequest (String hostnamePort, byte[] regionArray, int tmid) throws Exception{
        LOG.trace("recoveryRequest -- ENTRY TM" + tmid);
        HRegionInfo regionInfo = new HRegionInfo();
        final byte [] delimiter = ",".getBytes();
        String[] result = hostnamePort.split(new String(delimiter), 3);
        
        if (result.length < 2)
                throw new IllegalArgumentException("Region array format is incorrect");
        
        String hostname = result[0];        
        int port = Integer.parseInt(result[1]);
        LOG.debug("recoveryRequest regionInfo -- hostname:" + hostname + " port:" + port);
        ByteArrayInputStream lv_bis = new ByteArrayInputStream(regionArray);
        DataInputStream lv_dis = new DataInputStream(lv_bis);
        try {
                regionInfo.readFields(lv_dis);
        } catch (Exception e) {                
                StringWriter sw = new StringWriter();
                PrintWriter pw = new PrintWriter(sw);
                e.printStackTrace(pw);
                LOG.error("recoveryRequest exception in regionInfo.readFields() " + sw.toString()); 
                throw new Exception();
        }
        TransactionRegionLocation regionLocation = new TransactionRegionLocation(regionInfo, hostname, port);
        TransactionalRegionInterface tri = (TransactionalRegionInterface) connection.getHRegionConnection(regionLocation.getServerAddress());
        LOG.trace("recoveryRequest -- EXIT TM" + tmid);
        return tri.recoveryRequest(regionInfo.getRegionName(), tmid);
    }
}

