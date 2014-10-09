/**
l * Copyright 2009 The Apache Software Foundation Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements. See the NOTICE file distributed with this work for additional information regarding
 * copyright ownership. The ASF licenses this file to you under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0 Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and limitations under the
 * License.
 */
package org.apache.hadoop.hbase.client.transactional;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Callable;
import java.util.concurrent.CompletionService;
import java.util.concurrent.ExecutorCompletionService;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.ZooKeeperConnectionException;
import org.apache.hadoop.hbase.client.HConnection;
import org.apache.hadoop.hbase.client.HConnectionManager;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.coprocessor.Batch;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.AbortTransactionRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.AbortTransactionResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequestRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequestResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.RecoveryRequestRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.RecoveryRequestResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TrxRegionService;
import org.apache.hadoop.hbase.ipc.BlockingRpcCallback;
import org.apache.hadoop.hbase.ipc.ServerRpcController;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.EnvironmentEdgeManager;
import org.apache.hadoop.ipc.RemoteException;

import com.google.protobuf.ByteString;

/**
 * Transaction Manager. Responsible for committing transactions.
 */
public class TransactionManager {

  static final Log LOG = LogFactory.getLog(TransactionManager.class);
 
  private int RETRY_ATTEMPTS;
  private final HConnection connection;
  private final TransactionLogger transactionLogger;
  private JtaXAResource xAResource;

  public static final int TM_COMMIT_FALSE = 0;     
  public static final int TM_COMMIT_READ_ONLY = 1; 
  public static final int TM_COMMIT_TRUE = 2;
  public static final int TM_COMMIT_FALSE_CONFLICT = 3;    
  
  static ExecutorService    cp_tpe;

  /* increment/deincrement for positive value */
  /* This method copied from o.a.h.h.utils.Bytes */ 
  public static byte [] binaryIncrementPos(byte [] value, long amount) {
    long amo = amount;
    int sign = 1;
    if (amount < 0) {
      amo = -amount;
      sign = -1;
    }
    for(int i=0;i<value.length;i++) {
      int cur = ((int)amo % 256) * sign;
      amo = (amo >> 8);
      int val = value[value.length-i-1] & 0x0ff;
      int total = val + cur;
      if(total > 255) {
        amo += sign;
        total %= 256;
      } else if (total < 0) {
        amo -= sign;
      }
      value[value.length-i-1] = (byte)total;
      if (amo == 0) return value;
    }
    return value;
  }

  /** 
   * TransactionManagerCallable  :  inner class for creating asynchronous requests
   */
  private abstract class TransactionManagerCallable implements Callable<Integer>{    
    	TransactionState transactionState;
    	TransactionRegionLocation  location;
    	HTable table;
    	byte[] startKey;
    	byte[] endKey_orig;
    	byte[] endKey;
    	
    	TransactionManagerCallable(TransactionState txState, TransactionRegionLocation location, HConnection connection) { 
    	transactionState = txState;
    	this.location = location;
    	try {
	    table = new HTable(location.getRegionInfo().getTable(), connection, cp_tpe);
    	} catch(IOException e) {
    	  e.printStackTrace();
    	  LOG.error("Error obtaining HTable instance");
    	  table = null;
    	}
    	startKey = location.getRegionInfo().getStartKey();
    	endKey_orig = location.getRegionInfo().getEndKey();
	endKey = TransactionManager.binaryIncrementPos(endKey_orig, -1);

	}    	
	
	/**
	 * Method  : doCommitX 
	 * Params  : regionName - name of Region
	 *           transactionId - transaction identifier
	 * Return  : Always 0, can ignore
	 * Purpose : Call commit for a given regionserver  
	 */
  public Integer doCommitX(final byte[] regionName, final long transactionId, final boolean ignoreUnknownTransactionException) throws CommitUnsuccessfulException, IOException {
        boolean retry = false;
        int retryCount = 0;
        do {
          try {

            if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- ENTRY txid: " + transactionId +
                                                  " ignoreUnknownTransactionException: " + ignoreUnknownTransactionException);
              Batch.Call<TrxRegionService, CommitResponse> callable =
                  new Batch.Call<TrxRegionService, CommitResponse>() {
                ServerRpcController controller = new ServerRpcController();
                BlockingRpcCallback<CommitResponse> rpcCallback =
                new BlockingRpcCallback<CommitResponse>();

                @Override
                public CommitResponse call(TrxRegionService instance) throws IOException {
                  org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequest.Builder builder = CommitRequest.newBuilder();
                  builder.setTransactionId(transactionId);
	          builder.setRegionName(ByteString.copyFromUtf8(Bytes.toString(regionName))); //ByteString.copyFromUtf8(Bytes.toString(regionName)));
                  builder.setIgnoreUnknownTransactionException(ignoreUnknownTransactionException);

                  instance.commit(controller, builder.build(), rpcCallback);
                  return rpcCallback.get();
                }
             };

            Map<byte[], CommitResponse> result = null;
            try {
              HRegionLocation lv_hrl = table.getRegionLocation(startKey);
              HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
              if ((location.getRegionInfo().compareTo(lv_hri) != 0)) {
                 LOG.info("doCommitX -- " + table.toString() + " location being refreshed");
                 if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- lv_hri: " + lv_hri);
                 if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- location.getRegionInfo(): " + location.getRegionInfo());
                 table.getRegionLocation(startKey, true);
              }
              if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- before coprocessorService txid: " + transactionId +
                        " ignoreUnknownTransactionException: " + ignoreUnknownTransactionException);
	      if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- " + table.toString() + " startKey: " + new String(startKey, "UTF-8") + " endKey: " + new String(endKey, "UTF-8"));
              result = table.coprocessorService(TrxRegionService.class, startKey, endKey, callable);
            } catch (Throwable e) {
               String msg = "ERROR occurred while calling coprocessor service";
               LOG.error(msg + ":" + e);
               throw new Exception(msg);
            }
            if(result.size() != 1) {
              LOG.error("doCommitX, result size: " + result.size());
              throw new IOException("ERROR Received incorrect number of results from coprocessor call");
            }
            for (CommitResponse cresponse : result.values())
            {
              if(cresponse.getHasException()) {
                String exceptionString = new String (cresponse.getException().toString());
                if (exceptionString.contains("UnknownTransactionException")) {
                   if (ignoreUnknownTransactionException == true) {
                      if (LOG.isTraceEnabled()) LOG.trace("doCommitX, ignoring UnknownTransactionException in cresponse");
                   }
                   else {
                      LOG.error("doCommitX, coprocessor UnknownTransactionException: " + cresponse.getException());
                      throw new UnknownTransactionException();
                   }
                }
                else {
                   if (LOG.isTraceEnabled()) LOG.trace("doCommitX coprocessor exception: " + cresponse.getException());
                   throw new Exception(cresponse.getException());
                }
              }
            }
            retry = false;
          }
          catch (UnknownTransactionException ute) {
               LOG.error("exception in doCommitX : " + ute);
               LOG.info("Got unknown exception during commit. Transaction: [" + transactionState.getTransactionId() + "]");
               transactionState.requestPendingCountDec(true);
               throw new UnknownTransactionException();
          }

          catch (Exception e) {
             HRegionLocation lv_hrl = table.getRegionLocation(startKey);
             HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
             if ((location.getRegionInfo().compareTo(lv_hri) != 0) ||
                 (location.getServerName().compareTo(lv_hrl.getServerName()) != 0)) {
                 LOG.info("doCommitX -- " + table.toString() + " location being refreshed");
                 if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- lv_hri: " + lv_hri);
                 if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- location.getRegionInfo(): " + location.getRegionInfo());
                 table.getRegionLocation(startKey, true);
             }
             retry = true;
             retryCount++;
             if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- setting retry, count: " + retryCount);
             if(retryCount == RETRY_ATTEMPTS) {
                LOG.error("exception in doCommitX: " + e);
                // We have received our reply in the form of an exception,
                // so decrement outstanding count and wake up waiters to avoid
                // getting hung forever
                transactionState.requestPendingCountDec(true);
                throw new CommitUnsuccessfulException(e);
             }
           }
        } while (retryCount < RETRY_ATTEMPTS && retry == true); 

      	// We have received our reply so decrement outstanding count
      	transactionState.requestPendingCountDec(false);
         			
      	// forget the transaction if all replies have been received. otherwise another thread
      	// will do it.
      	if (transactionState.requestAllComplete())
      	{
      		transactionLogger.forgetTransaction(transactionState.getTransactionId());
      	}
      	if (LOG.isTraceEnabled()) LOG.trace("doCommitX -- EXIT txid: " + transactionId);
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
  public Integer doPrepareX(final byte[] regionName, final long transactionId, final TransactionRegionLocation location) 
	throws IOException, CommitUnsuccessfulException {

    int commitStatus = 0;
    boolean retry = false;
    int retryCount = 0;
    do {
	    try {			
	      Batch.Call<TrxRegionService, CommitRequestResponse> callable =
	          new Batch.Call<TrxRegionService, CommitRequestResponse>() {
	        ServerRpcController controller = new ServerRpcController();
	        BlockingRpcCallback<CommitRequestResponse> rpcCallback =
	          new BlockingRpcCallback<CommitRequestResponse>();
	
	        @Override
	        public CommitRequestResponse call(TrxRegionService instance) throws IOException {
	          org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CommitRequestRequest.Builder builder = CommitRequestRequest.newBuilder();
	          builder.setTransactionId(transactionId);
		  builder.setRegionName(ByteString.copyFromUtf8(Bytes.toString(regionName)));
	
	          instance.commitRequest(controller, builder.build(), rpcCallback);
	          return rpcCallback.get();
	        }
	      };
	
	     Map<byte[], CommitRequestResponse> result = null;

	     try {
		    if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- before coprocessorService txid: " + transactionId + " table: " + table.toString());
		    if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- txid: " + transactionId + " table: " + table.toString() + " endKey_Orig: " + new String(endKey_orig, "UTF-8"));
		    if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- " + table.toString() + " startKey: " + new String(startKey, "UTF-8") + " endKey: " + new String(endKey, "UTF-8"));

		    HRegionLocation lv_hrl = table.getRegionLocation(startKey);
		    HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
		    if ((location.getRegionInfo().compareTo(lv_hri) != 0)) {
			    	LOG.info("doPrepareX -- " + table.toString() + " location being refreshed");
			    	if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- lv_hri: " + lv_hri);
			    	if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- location.getRegionInfo(): " + location.getRegionInfo());
				
			    	table.getRegionLocation(startKey, true);
			}
		    
		    result = table.coprocessorService(TrxRegionService.class, startKey, endKey, callable);
	        } catch (Throwable e) {	          
	          LOG.error("doPrepareX coprocessor error for " + Bytes.toString(regionName) + " txid: " + transactionId + ":" + e);
	          throw new CommitUnsuccessfulException("Unable to call prepare, coprocessor error");
	          
	        }
	
	        if(result.size() != 1)  {
		    LOG.error("doPrepareX, result size: " + result.size());
		    throw new IOException("ERROR Received incorrect number of results from coprocessor call");
	        }
	        for (CommitRequestResponse cresponse : result.values())
	        {
	          // Should only be one result
	          int value = cresponse.getResult();          
	          commitStatus = value;        
	          if(cresponse.getHasException()) {
	        	if (LOG.isTraceEnabled()) LOG.trace("doPrepareX coprocessor exception: " + cresponse.getException());
	            throw new Exception(cresponse.getException());
	          }
	        }
	        retry = false;
	    }
	    catch(UnknownTransactionException ute) {
	    	LOG.warn("Exception: " + ute);
	    	throw new UnknownTransactionException();
	    }
	    catch(Exception e) {	    	
		    HRegionLocation lv_hrl = table.getRegionLocation(startKey);
		    HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
		    if ((location.getRegionInfo().compareTo(lv_hri) != 0) || 
		        (location.getServerName().compareTo(lv_hrl.getServerName()) != 0)) {
		    	LOG.info("doPrepareX -- " + table.toString() + " location being refreshed");
		    	if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- lv_hri: " + lv_hri);
		    	if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- location.getRegionInfo(): " + location.getRegionInfo());
			
		    	table.getRegionLocation(startKey, true);

		    	LOG.debug("doPrepareX retry count: " + retryCount);
		    }
	    	retry = true;
	    	retryCount++;
	    	if (LOG.isTraceEnabled()) LOG.trace("doPrepareX -- setting retry, count: " + retryCount);
		    if(retryCount == RETRY_ATTEMPTS){
		    	LOG.error("Exception: " + e);
		    	throw new IOException(e);
		    }
	    }
    } while (retryCount < RETRY_ATTEMPTS && retry == true); 
    
    if (LOG.isTraceEnabled()) LOG.trace("commitStatus: " + commitStatus);
  	boolean canCommit = true;
    boolean readOnly = false;
                
    switch (commitStatus) {
        case TransactionalReturn.COMMIT_OK:
            break;
        case TransactionalReturn.COMMIT_OK_READ_ONLY:
            transactionState.addRegionToIgnore(location); // No need to doCommit for read-onlys
            readOnly = true;
             break;
        case TransactionalReturn.COMMIT_CONFLICT:
        case TransactionalReturn.COMMIT_UNSUCCESSFUL:
             canCommit = false;
             transactionState.addRegionToIgnore(location); // No need to re-abort.
             break;
  	    default:
             LOG.warn("Received invalid return code from requestCommit " + commitStatus + " for transaction " + transactionId + " throwing CommitUnsuccessfulException");
  
             throw new CommitUnsuccessfulException("Unexpected return code from prepareCommit: "
              + commitStatus);
     }
          
     if (!canCommit) {
    	 // track regions which indicate they could not commit for better diagnostics
         LOG.warn("Region [" + location.getRegionInfo().getRegionNameAsString() + "] votes "
                 +  "to abort" + (readOnly ? " Read-only ":"") + " transaction "
                 + transactionState.getTransactionId());
         //System.out.println("Region [" + location.getRegionInfo().getRegionNameAsString() + "] votes "
         //        +  "to abort" + (readOnly ? " Read-only ":"") + " transaction "
         //        + transactionState.getTransactionId());
         if(commitStatus == TransactionalReturn.COMMIT_CONFLICT)
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
    public Integer doAbortX(final byte[] regionName, final long transactionId) throws IOException{
	    boolean retry = false;
	    int retryCount = 0;
	    do {
	    	try {

	          Batch.Call<TrxRegionService, AbortTransactionResponse> callable =
	            new Batch.Call<TrxRegionService, AbortTransactionResponse>() {
	          ServerRpcController controller = new ServerRpcController();
	          BlockingRpcCallback<AbortTransactionResponse> rpcCallback =
	            new BlockingRpcCallback<AbortTransactionResponse>();
	
	          @Override
	          public AbortTransactionResponse call(TrxRegionService instance) throws IOException {
	            org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.AbortTransactionRequest.Builder builder = AbortTransactionRequest.newBuilder();
	            builder.setTransactionId(transactionId);
	            builder.setRegionName(ByteString.copyFromUtf8(Bytes.toString(regionName)));
	
	            instance.abortTransaction(controller, builder.build(), rpcCallback);
	            return rpcCallback.get();
	          }
	        };
	          
	        Map<byte[], AbortTransactionResponse> result = null;
	          try {       
	              HRegionLocation lv_hrl = table.getRegionLocation(startKey);
	              HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
	              if ((location.getRegionInfo().compareTo(lv_hri) != 0)) {
	                  LOG.info("doAbortX -- " + table.toString() + " region location being refreshed ");
	                  if (LOG.isTraceEnabled()) LOG.trace("doAbortX -- lv_hri: " + lv_hri);
 	                  if (LOG.isTraceEnabled())  LOG.trace("doAbortX -- location.getRegionInfo(): " + location.getRegionInfo());

	                  table.getRegionLocation(startKey, true);
	              }
		      result = table.coprocessorService(TrxRegionService.class, startKey, endKey, callable);
	          } catch (Throwable e) {
	              String msg = "ERROR occurred while calling coprocessor service";
	              LOG.error(msg + ":" + e);
	              throw new Exception(msg);
	          }
	          
	          if(result.size() != 1) 
	            throw new IOException("ERROR Received incorrect number of results from coprocessor call");
	          
	          for (AbortTransactionResponse cresponse : result.values())
	          {                                                  
	            if(cresponse.getHasException()) {
	              LOG.error("Abort HasException true: " + cresponse.getHasException());
	              LOG.error("Abort HasException true: " + cresponse.getException().toString());
	              throw new Exception(cresponse.getException());
	            }
	          }
	          retry = false;
	      } 
	      catch (UnknownTransactionException ute) {
		         LOG.error("exception in doAbortX (ignoring): " + ute);       
		         LOG.info("Got unknown exception during abort. Transaction: ["
		             + transactionState.getTransactionId() + "]");
	      }
	      catch (Exception e)
	      {        		    	  
              HRegionLocation lv_hrl = table.getRegionLocation(startKey);
              HRegionInfo     lv_hri = lv_hrl.getRegionInfo();
              if ((location.getRegionInfo().compareTo(lv_hri) != 0) ||
                  (location.getServerName().compareTo(lv_hrl.getServerName()) != 0)) {
                  LOG.info("doAbortX -- " + table.toString() + " region location being refreshed ");
                  if (LOG.isTraceEnabled()) LOG.trace("doAbortX -- lv_hri: " + lv_hri);
                  if (LOG.isTraceEnabled()) LOG.trace("doAbortX -- location.getRegionInfo(): " + location.getRegionInfo());

                  table.getRegionLocation(startKey, true);
              }              
              retry = true;
              retryCount++;
              if (LOG.isTraceEnabled()) LOG.trace("doAbortX -- setting retry, count: " + retryCount);
              if (retryCount == RETRY_ATTEMPTS){
		         LOG.error("exception in doAbortX (ignoring): " + e);       
		         LOG.info("Got unknown exception during abort. Transaction: ["
		             + transactionState.getTransactionId() + "]");
              }
	      }   
	  } while (retryCount < RETRY_ATTEMPTS && retry == true);	     
      
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
  } // TransactionManagerCallable
  
    
  /**
   * threadPool - pool of thread for asynchronous requests
   */
	ExecutorService threadPool;

	/**
     * @param conf
     * @throws ZooKeeperConnectionException
     */
    public TransactionManager(final Configuration conf) throws ZooKeeperConnectionException, IOException {
        this(LocalTransactionLogger.getInstance(), conf);

        int intThreads = 16;

        
        String retryAttempts = System.getenv("TMCLIENT_RETRY_ATTEMPTS");
        String numThreads = System.getenv("TM_JAVA_THREAD_POOL_SIZE");
        String numCpThreads = System.getenv("TM_JAVA_CP_THREAD_POOL_SIZE");

        if (retryAttempts != null) 
        	RETRY_ATTEMPTS = Integer.parseInt(retryAttempts);
        else 
        	RETRY_ATTEMPTS = 3;
        
        if (numThreads != null)
            intThreads = Integer.parseInt(numThreads);

        int intCpThreads = intThreads;
        if (numCpThreads != null)
            intCpThreads = Integer.parseInt(numCpThreads);
	
        threadPool = Executors.newFixedThreadPool(intThreads);      

	cp_tpe = Executors.newFixedThreadPool(intCpThreads);

	/* This was in the HTable code - let's use a fixed thread pool instead
	cp_tpe = new ThreadPoolExecutor(1, 
					intCpThreads, 
					30, TimeUnit.SECONDS,
					new SynchronousQueue<Runnable>(),
					Threads.newDaemonThreadFactory("htable_tmcp"));
	*/
    }

    /**
     * @param transactionLogger
     * @param conf
     * @throws ZooKeeperConnectionException
     */
    public TransactionManager(final TransactionLogger transactionLogger, final Configuration conf)
            throws ZooKeeperConnectionException, IOException {
        this.transactionLogger = transactionLogger;        
	conf.setInt("hbase.client.retries.number", 3);
        connection = HConnectionManager.createConnection(conf);        
    }


    /**
     * Called to start a transaction.
     * 
     * @return new transaction state
     */
    public TransactionState beginTransaction() {
        long transactionId = transactionLogger.createNewTransactionLog();
        if (LOG.isTraceEnabled()) LOG.trace("Beginning transaction " + transactionId);
        return new TransactionState(transactionId);
    }

    /**
     * Called to start a transaction with transactionID input
     *
     * @return new transaction state
     */
    public TransactionState beginTransaction(long transactionId) {
        //long transactionId =
      if (LOG.isTraceEnabled()) LOG.trace("Enter beginTransaction, txid: " + transactionId);
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
    	CompletionService<Integer> compPool = new ExecutorCompletionService<Integer>(threadPool); 
        try {
            for (TransactionRegionLocation location : transactionState.getParticipatingRegions()) {

            	loopCount++;
             final TransactionRegionLocation myLocation = location;
             final byte[] regionName = location.getRegionInfo().getRegionName();             

             compPool.submit(new TransactionManagerCallable(transactionState, location, connection) {
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
          		return TransactionalReturn.COMMIT_UNSUCCESSFUL;
          	}
          	if(canCommit == TM_COMMIT_FALSE_CONFLICT) {
          		LOG.warn("Aborting [" + transactionState.getTransactionId() + "] due to Conflict");
          		abort(transactionState);
          		return TransactionalReturn.COMMIT_CONFLICT;
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
        return allReadOnly ? TransactionalReturn.COMMIT_OK_READ_ONLY: 
                             TransactionalReturn.COMMIT_OK;
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
        if (LOG.isTraceEnabled()) LOG.trace("Attempting to commit transaction: " + transactionState.toString());
        int status = prepareCommit(transactionState);

        if (status == TransactionalReturn.COMMIT_OK) {
          if (LOG.isTraceEnabled()) LOG.trace("doCommit txid:" + transactionState.getTransactionId());
          doCommit(transactionState);
        } else if (status == TransactionalReturn.COMMIT_OK_READ_ONLY) {
        	// no requests sent for fully read only transaction
          transactionState.completeSendInvoke(0);
          transactionLogger.forgetTransaction(transactionState.getTransactionId());
        } else if (status == TransactionalReturn.COMMIT_UNSUCCESSFUL) {
          // We have already aborted at this point
          throw new CommitUnsuccessfulException();
        }
        if (LOG.isTraceEnabled()) LOG.trace("Committed transaction [" + transactionState.getTransactionId() + "] in ["
                + ((EnvironmentEdgeManager.currentTimeMillis() - startTime)) + "]ms");
    }

    /**
     * Do the commit. This is the 2nd phase of the 2-phase protocol.
     * 
     * @param transactionState
     * @throws CommitUnsuccessfulException
     */
    public void doCommit(final TransactionState transactionState) throws CommitUnsuccessfulException {
       if (LOG.isTraceEnabled()) LOG.trace("doCommit [" + transactionState.getTransactionId() +
                      "] ignoreUnknownTransactionException not supplied");
       doCommit(transactionState, false);
    }

    /**
     * Do the commit. This is the 2nd phase of the 2-phase protocol.
     *
     * @param transactionState
     * @param ignoreUnknownTransactionException
     * @throws CommitUnsuccessfulException
     */
    public void doCommit(final TransactionState transactionState, final boolean ignoreUnknownTransactionException) throws CommitUnsuccessfulException {
    	int loopCount = 0;
        try {
            if (LOG.isTraceEnabled()) LOG.trace("Committing [" + transactionState.getTransactionId() +
                      "] ignoreUnknownTransactionException: " + ignoreUnknownTransactionException);

            transactionLogger.setStatusForTransaction(transactionState.getTransactionId(),
                TransactionLogger.TransactionStatus.COMMITTED);

            // (Asynchronously send commit
            for (TransactionRegionLocation location : transactionState.getParticipatingRegions()) {
              if (LOG.isTraceEnabled()) LOG.trace("sending commits ... [" + transactionState.getTransactionId() + "]");
                if (transactionState.getRegionsToIngore().contains(location)) {
                    continue;
                }

                loopCount++;
            	final byte[] regionName = location.getRegionInfo().getRegionName();  
          
                //TransactionalRegionInterface transactionalRegionServer = (TransactionalRegionInterface) connection
                  //      .getHRegionConnection(location.getServerName());
                    
                threadPool.submit(new TransactionManagerCallable(transactionState, location, connection) {
                  public Integer call() throws CommitUnsuccessfulException, IOException {
                    if (LOG.isTraceEnabled()) LOG.trace("before doCommit() [" + transactionState.getTransactionId() + "]" +
                                                        " ignoreUnknownTransactionException: " + ignoreUnknownTransactionException);
                    return doCommitX(regionName, transactionState.getTransactionId(), ignoreUnknownTransactionException);
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
        /*
        try {
          Thread.sleep(500);
        } catch(Exception e) {}
        */
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
              
            threadPool.submit(new TransactionManagerCallable(transactionState, location, connection) {
              public Integer call() throws IOException {
                return doAbortX(regionName, transactionState.getTransactionId());
              }
            });
          } catch (Exception e) {
            LOG.error("exception in abort: " + e);
          }
            /*
            } catch (UnknownTransactionException e) {
		LOG.error("exception in abort: " + e);
                LOG.info("Got unknown transaction exception during abort. Transaction: ["
                        + transactionState.getTransactionId() + "], region: ["
                        + location.getRegionInfo().getRegionNameAsString() + "]. Ignoring.");
            } catch (NotServingRegionException e) {
                LOG.info("Got NSRE during abort. Transaction: [" + transactionState.getTransactionId() + "], region: ["
                        + location.getRegionInfo().getRegionNameAsString() + "]. Ignoring.");
            }
            */
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
        if (LOG.isTraceEnabled()) LOG.trace("registerRegion ENTRY, transactioState:" + transactionState);
    	if(transactionState.addRegion(location)){
	    if (LOG.isTraceEnabled()) LOG.trace("registerRegion -- adding region: " + location.getRegionInfo().getRegionNameAsString());
    	}
        if (LOG.isTraceEnabled()) LOG.trace("registerRegion EXIT");
    }
    
    /**
     * @param hostnamePort
     * @param regionArray
     * @param tmid
     * @return
     * @throws Exception
     */
    public List<Long> recoveryRequest (String hostnamePort, byte[] regionArray, int tmid) throws Exception{
        if (LOG.isTraceEnabled()) LOG.trace("recoveryRequest -- ENTRY TM" + tmid);
        HRegionInfo regionInfo = null;
        HTable table = null;
        
        /*
         * hostname and port no longer needed for RPC
        final byte [] delimiter = ",".getBytes();
        String[] result = hostnamePort.split(new String(delimiter), 3);
        
        if (result.length < 2)
                throw new IllegalArgumentException("Region array format is incorrect");
                    	
        String hostname = result[0];        
        int port = Integer.parseInt(result[1]);
        LOG.debug("recoveryRequest regionInfo -- hostname:" + hostname + " port:" + port);
        */
        
        /*
         *  New way of parsing HRegionInfo used instead
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
        */
        
    	try {
    	    regionInfo = HRegionInfo.parseFrom(regionArray);
    	}
    	catch (Exception de) {
           if (LOG.isTraceEnabled()) LOG.trace("TransactionManager:recoveryRequest exception in regionInfo parseFrom, " +
 		     " TM : " + tmid +
 		     " DeserializationException: " + de);
 	        StringWriter sw = new StringWriter();
 	        PrintWriter pw = new PrintWriter(sw);
 	        de.printStackTrace(pw);
 	        LOG.error(sw.toString());  	   
            throw new Exception("DeserializationException in regionInfo parseFrom, unable to complete recoveryRequest " + de);
       } 
        
    	final String regionName = regionInfo.getRegionNameAsString();
    	final int tmID = tmid;
        if (LOG.isTraceEnabled()) LOG.trace("TransactionManager:recoveryRequest regionInfo encoded name: [" + regionInfo.getEncodedName() + "]");
        Batch.Call<TrxRegionService, RecoveryRequestResponse> callable = 
                new Batch.Call<TrxRegionService, RecoveryRequestResponse>() {
              ServerRpcController controller = new ServerRpcController();
              BlockingRpcCallback<RecoveryRequestResponse> rpcCallback = 
                new BlockingRpcCallback<RecoveryRequestResponse>();         

              @Override
              public RecoveryRequestResponse call(TrxRegionService instance) throws IOException {        
                org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.RecoveryRequestRequest.Builder rbuilder = RecoveryRequestRequest.newBuilder();        
                rbuilder.setTransactionId(-1);
                rbuilder.setRegionName(ByteString.copyFromUtf8(regionName));
                rbuilder.setTmId(tmID);
                
                instance.recoveryRequest(controller, rbuilder.build(), rpcCallback);
                return rpcCallback.get();        
              }
            };
            
            // Working out the begin and end keys
            byte[] startKey = regionInfo.getStartKey();
            byte[] endKey = regionInfo.getEndKey();
            
            if(endKey != HConstants.EMPTY_END_ROW)
            	endKey = TransactionManager.binaryIncrementPos(endKey, -1);
            
            table = new HTable(regionInfo.getTable(), connection, cp_tpe);
         
            Map<byte[], RecoveryRequestResponse> rresult = null;   
            try {
              rresult = table.coprocessorService(TrxRegionService.class, startKey, endKey, callable);
            } 
            catch (Throwable e) {
            	LOG.error("Exception thrown when calling coprocessor: " + e.toString());
                e.printStackTrace();     
            }
                                
        Collection<RecoveryRequestResponse> results = rresult.values();
        RecoveryRequestResponse[] resultArray = new RecoveryRequestResponse[results.size()];
        results.toArray(resultArray);
        
        if(resultArray.length == 0) {
        	table.close();
        	throw new IOException("Problem with calling coprocessor, no regions returned result");
        }                      
       
        //return tri.recoveryRequest(regionInfo.getRegionName(), tmid);
        table.close();
        if (LOG.isTraceEnabled()) LOG.trace("recoveryRequest -- EXIT TM" + tmid);
        
        return resultArray[0].getResultList();
    }
}

