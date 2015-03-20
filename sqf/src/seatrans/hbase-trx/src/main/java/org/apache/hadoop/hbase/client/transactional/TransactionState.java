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
package org.apache.hadoop.hbase.client.transactional;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.client.transactional.TransState;

/**
 * Holds client-side transaction information. Client's use them as opaque objects passed around to transaction
 * operations.
 */
public class TransactionState {

    static final Log LOG = LogFactory.getLog(TransactionState.class);

    private final long transactionId;
    private TransState status;
    
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
    private boolean localTransaction;
    private boolean ddlTrans;
    
    public Set<String> tableNames = Collections.synchronizedSet(new HashSet<String>());
    public Set<TransactionRegionLocation> participatingRegions = Collections.synchronizedSet(new HashSet<TransactionRegionLocation>());
    /**
     * Regions to ignore in the twoPase commit protocol. They were read only, or already said to abort.
     */
    private Set<TransactionRegionLocation> regionsToIgnore = Collections.synchronizedSet(new HashSet<TransactionRegionLocation>());

    private native void registerRegion(long transid, int port, byte[] hostname, long startcode, byte[] regionInfo);

    
    public boolean islocalTransaction() {
      return localTransaction;
    }
    
    public TransactionState(final long transactionId) {
        this.transactionId = transactionId;
        setStatus(TransState.STATE_ACTIVE);
        countLock = new Object();
        commitSendLock = new Object();
        requestPendingCount = 0;
        requestReceivedCount = 0;
        commitSendDone = false;
        hasError = false;
        ddlTrans = false;
        String localTxns = System.getenv("DTM_LOCAL_TRANSACTIONS");
        if (localTxns != null) {
          localTransaction = (Integer.parseInt(localTxns)==0)?false:true;
          //System.out.println("TS begin local txn id " + transactionId);
          if (LOG.isTraceEnabled()) LOG.trace("TransactionState local transaction begun: " + transactionId);
        }
        else {
          localTransaction = false;
          if (LOG.isTraceEnabled()) LOG.trace("TransactionState global transaction begun: " + transactionId);
        }
    }
    
    public boolean addTableName(final String table) {
        boolean added =  tableNames.add(table);
        if (added) {
            if (LOG.isTraceEnabled()) LOG.trace("Adding new table name [" + table + "] to transaction state ["
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
    
      public void registerLocation(final HRegionLocation location) throws IOException {
        byte [] lv_hostname = location.getHostname().getBytes();
        int lv_port = location.getPort();
	    long lv_startcode = location.getServerName().getStartcode();
  
	    /*        ByteArrayOutputStream lv_bos = new ByteArrayOutputStream();
        DataOutputStream lv_dos = new DataOutputStream(lv_bos);
        location.getRegionInfo().write(lv_dos);
        lv_dos.flush(); */
        byte [] lv_byte_region_info = location.getRegionInfo().toByteArray();
        if(LOG.isTraceEnabled()) LOG.trace("TransactionState.registerLocation: [" + location.getRegionInfo().getRegionNameAsString() + 
          "], transaction [" + transactionId + "]");
        
        if (islocalTransaction()) {
	    if(LOG.isTraceEnabled()) LOG.trace("TransactionState.registerLocation local transaction not sending registerRegion.");
        }
        else {
	    if(LOG.isTraceEnabled()) LOG.trace("TransactionState.registerLocation global transaction registering region.");
          registerRegion(transactionId, lv_port, lv_hostname, lv_startcode, lv_byte_region_info);
        }
      }
    
    public boolean addRegion(final TransactionRegionLocation trRegion) {        

// Save hregion for now
        boolean added = participatingRegions.add(trRegion);

        if (added) {
            if (LOG.isTraceEnabled()) LOG.trace("Adding new hregion [" + trRegion.getRegionInfo().getRegionNameAsString() + "] to transaction ["
                    + transactionId + "]");
        }

        return added;
    }

    public boolean addRegion(final HRegionLocation hregion) {        

        TransactionRegionLocation trRegion   = new TransactionRegionLocation(hregion.getRegionInfo(), 
                                                                             hregion.getServerName());
// Save hregion for now
        boolean added = participatingRegions.add(trRegion);

        if (added) {
            if (LOG.isTraceEnabled()) LOG.trace("Adding new hregion [" + trRegion.getRegionInfo().getRegionNameAsString() + "] to transaction ["
                    + transactionId + "]");
        }
        else {
            if (LOG.isTraceEnabled()) LOG.trace("HRegion already added [" + hregion.getRegionInfo().getRegionNameAsString() + "] to transaction ["
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

    Set<TransactionRegionLocation> getRegionsToIgnore() {
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
      return status.toString();
    }

    public void setStatus(final TransState status) {
      this.status = status;
    }

    public boolean getDDLTxStatus() {
        return ddlTrans;   
    }

    public void setDDLTxStatus(final boolean status) {
        this.ddlTrans = status;
    }
}
