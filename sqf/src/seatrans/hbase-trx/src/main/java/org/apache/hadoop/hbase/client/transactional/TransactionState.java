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

import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hbase.HRegionLocation;

/**
 * Holds client-side transaction information. Client's use them as opaque objects passed around to transaction
 * operations.
 */
public class TransactionState {

    static final Log LOG = LogFactory.getLog(TransactionState.class);

/** Current status. */
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

    private final long transactionId;
    private int status;
    
    /**
     * 
     * requestPendingCount - how many requests send
     * requestReceivedCount - how many replies received
     * countLock - synchronize access to above
     * 
     * commitSendDone - did we sent all requests yet
     * commitSendLock - synchronize access to above
     */
    private int requestPendingCount;
    private int requestReceivedCount;
    private Object countLock;
    private boolean commitSendDone;
    private Object commitSendLock;
    private boolean hasError;
    
    public Set<String> tableNames = Collections.synchronizedSet(new HashSet<String>());
    public Set<TransactionRegionLocation> participatingRegions = Collections.synchronizedSet(new HashSet<TransactionRegionLocation>());
    /**
     * Regions to ignore in the twoPase commit protocol. They were read only, or already said to abort.
     */
    private Set<TransactionRegionLocation> regionsToIgnore = Collections.synchronizedSet(new HashSet<TransactionRegionLocation>());

    public TransactionState(final long transactionId) {
        this.transactionId = transactionId;
        setStatus(TM_TX_STATE_ACTIVE);
	countLock = new Object();
	commitSendLock = new Object();
	requestPendingCount = 0;
	requestReceivedCount = 0;
	commitSendDone = false;
        hasError = false;
    }
    
    public boolean addTableName(final String table) {
        boolean added =  tableNames.add(table);
        if (added) {
            LOG.trace("Adding new table name [" + table + "] to transaction state ["
                    + transactionId + "]");
        }
        return added;
    }

    /**
     * 
     * Method  : requestAllComplete
     * Params  : None
     * Return  : True if all replies received, False if not
     * Purpose : Make sure all requests have been sent, then determine
     *           if we've received all the replies.  Non blocking version
     */
    public boolean requestAllComplete() {
 
    	
    	// Make sure that we've completed sending the requests
    	synchronized (commitSendLock)
	   	{
	   		if (commitSendDone != true)
	   			return false;
	   	}
    	
    	synchronized (countLock)
	   	{
    	    if (requestPendingCount == requestReceivedCount)
       		    return true;
    	    return false;
	   	}
    }
    
    /**
     * 
     * Method  : requestPendingcount
     * Params  : count - how many requests were sent
     * Return  : void
     * Purpose : Record how many requests were sent
     */
	public void requestPendingCount(int count)
	{
	 	synchronized (countLock)
	   	{
	 	    hasError = false;  // reset, just in case
	   	    requestPendingCount = count;
	   	}
	}

	/**
	 * 
	 * Method  : requestPendingCountDec
	 * Params  : None
	 * Return  : void
	 * Purpose : Decrease number of outstanding replies needed and wake up any waiters
	 *           if we receive the last one or if the wakeUp value is true (which means
	 *           we received an exception)
	 */
	public void  requestPendingCountDec(boolean wakeUp)
	{
	   	synchronized (countLock)
	   	{
	   	    requestReceivedCount++;
	   	    if ((requestReceivedCount == requestPendingCount) || (wakeUp == true))
	   	    {
	   	    	//Signal waiters that an error occurred
	   	    	if (wakeUp == true)
	   	            hasError = true;
	   	    	
	   	    	countLock.notify();
	   	}
	}
	}
	    
	/**
	 * 
	 * Method  : completeRequest
	 * Params  : None
	 * Return  : Void
	 * Purpose : Hang thread until all replies have been received
	 */
	public void completeRequest() throws InterruptedException, CommitUnsuccessfulException
	{
		// Make sure we've completed sending all requests first, if not, then wait
	   	synchronized (commitSendLock)
	   	{
	   		if (commitSendDone == false)
	   		{
	   		    commitSendLock.wait();
	   		}
	   	}
	   	
	   	// if we haven't received all replies, then wait
	   	synchronized (countLock)
	   	{
	   	    if (requestPendingCount > requestReceivedCount)
	   	    	countLock.wait();
	   	}
	   	
	   	if (hasError)
	   	    throw new CommitUnsuccessfulException();
	   	
	   	return;
	   
	}
	
	/**
	 * 
	 * Method  : completeSendInvoke
	 * Params  : count : number of requests sent
	 * Return  : void
	 * Purpose : wake up waiter that are waiting on completion of sending requests 
	 */
	public void completeSendInvoke(int count)
	{
		
		// record how many requests sent
		requestPendingCount(count);
		
		// wake up waiters and record that we've sent all requests
	   	synchronized (commitSendLock)
	   	{
	   		commitSendDone = true;
	   		commitSendLock.notify();
	   	}
	}
    
    public boolean addRegion(final HRegionLocation hregion) {        
        TransactionRegionLocation trRegion   = new TransactionRegionLocation(hregion.getRegionInfo(), 
                                                                             hregion.getServerName());

// Save hregion for now
        boolean added = participatingRegions.add(trRegion);

        if (added) {
            LOG.trace("Adding new hregion [" + hregion.getRegionInfo().getRegionNameAsString() + "] to transaction ["
                    + transactionId + "]");
        }

        return added;
    }

    public Set<TransactionRegionLocation> getParticipatingRegions() {
        return participatingRegions;
    }

    public void clearParticipatingRegions() {
        participatingRegions.clear();
    }

    Set<TransactionRegionLocation> getRegionsToIngore() {
        return regionsToIgnore;
    }

    void addRegionToIgnore(final TransactionRegionLocation region) {
        // Changing to an HRegionLocation for now
        regionsToIgnore.add(region);
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
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        return "id: " + transactionId + ", participants: " + participatingRegions.size() + ", ignoring: " + regionsToIgnore.size();
    }

    public int getParticipantCount() {
        return participatingRegions.size();
    }

    public int getRegionsToIgnoreCount() {
        return regionsToIgnore.size();
    }

    public String getStatus() {
       String localStatus;
      switch (status) {
         case TM_TX_STATE_COMMITTED:
            localStatus = new String("COMMITTED");
            break;
         case TM_TX_STATE_ABORTED:
            localStatus = new String("ABORTED");
            break;
         default:
            localStatus = new String("ACTIVE");
            break;
      }
      return localStatus;
 
    }

    public void setStatus(final int status) {
      this.status = status;
    }

}
