/**
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**/

package org.apache.hadoop.hbase.client.transactional;

import java.io.IOException;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.codec.binary.Hex;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.util.Bytes;

/**
 * Holds client-side transaction information. Client's use them as opaque objects passed around to transaction
 * operations.
 */
public class TransactionState {

    static final Log LOG = LogFactory.getLog(TransactionState.class);

    private final long transactionId;
    private TransState status;
    private long startEpoch;
    private long startId;
    private long commitId;

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
    private static boolean useConcurrentHM = false;
    private static boolean getCHMVariable = true;
    private boolean hasRetried = false;

    public Set<String> tableNames = Collections.synchronizedSet(new HashSet<String>());
    public Set<TransactionRegionLocation> participatingRegions;
    /**
     * Regions to ignore in the twoPase commit protocol. They were read only, or already said to abort.
     */
    public Set<TransactionRegionLocation> regionsToIgnore = Collections.synchronizedSet(new HashSet<TransactionRegionLocation>());
    private Set<TransactionRegionLocation> retryRegions = Collections.synchronizedSet(new HashSet<TransactionRegionLocation>());

    private native void registerRegion(long transid, long startId, int port, byte[] hostname, long startcode, byte[] regionInfo);

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

        if(getCHMVariable) {
          String concurrentHM = System.getenv("DTM_USE_CONCURRENTHM");
          if (concurrentHM != null) {
            useConcurrentHM = (Integer.parseInt(concurrentHM)==0)?false:true;
          }
          if(LOG.isTraceEnabled()) {
            if(useConcurrentHM) {
              LOG.trace("Using ConcurrentHashMap synchronization to synchronize participatingRegions");
            }
            else {
              LOG.trace("Using synchronizedSet to synchronize participatingRegions");
            }
          }
          getCHMVariable = false;
        }
        if(useConcurrentHM) {
          participatingRegions = Collections.newSetFromMap((new ConcurrentHashMap<TransactionRegionLocation, Boolean>()));
        }
        else {
          participatingRegions = Collections.synchronizedSet(new HashSet<TransactionRegionLocation>());
        }

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

      public void registerLocation(final TransactionRegionLocation location) throws IOException {
        byte [] lv_hostname = location.getHostname().getBytes();
        int lv_port = location.getPort();
        long lv_startcode = location.getServerName().getStartcode();

        /*        ByteArrayOutputStream lv_bos = new ByteArrayOutputStream();
        DataOutputStream lv_dos = new DataOutputStream(lv_bos);
        location.getRegionInfo().write(lv_dos);
        lv_dos.flush(); */
        byte [] lv_byte_region_info = location.getRegionInfo().toByteArray();
        if (LOG.isTraceEnabled()) LOG.trace("TransactionState.registerLocation: [" + location.getRegionInfo().getEncodedName() +
          "], endKey: " + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " transaction [" + transactionId + "]");

        if (islocalTransaction()) {
          if(LOG.isTraceEnabled()) LOG.trace("TransactionState.registerLocation local transaction not sending registerRegion.");
        }
        else {
          if (LOG.isTraceEnabled()) LOG.trace("TransactionState.registerLocation global transaction registering region for ts: " + transactionId + " and startId: " + startId);
          registerRegion(transactionId, startId, lv_port, lv_hostname, lv_startcode, lv_byte_region_info);
        }
      }

    public boolean addRegion(final TransactionRegionLocation trRegion) {
        if (LOG.isTraceEnabled()) LOG.trace("addRegion ENTRY with trRegion [" + trRegion.getRegionInfo().getEncodedName() + "], endKey: "
                  + Hex.encodeHexString(trRegion.getRegionInfo().getEndKey()) + " and transaction [" + transactionId + "]");

        boolean added = participatingRegions.add(trRegion);

        if (added) {
           if (LOG.isTraceEnabled()) LOG.trace("Added new trRegion (#" + participatingRegions.size()
                        + ") to participatingRegions ["        + trRegion.getRegionInfo().getRegionNameAsString()
                        + "], endKey: "        + Hex.encodeHexString(trRegion.getRegionInfo().getEndKey()) 
                        + " and transaction [" + transactionId + "]");
        }
        else {
           if (LOG.isTraceEnabled()) LOG.trace("trRegion already present in (" + participatingRegions.size()
                       + ") participatinRegions ["    + trRegion.getRegionInfo().getEncodedName()
                       + "], endKey: "        + Hex.encodeHexString(trRegion.getRegionInfo().getEndKey()) 
                       + " and transaction [" + transactionId + "]");
        }

        return added;
    }

    public boolean addRegion(final HRegionLocation hregion) {

        if (LOG.isTraceEnabled()) LOG.trace("Creating new TransactionRegionLocation from HRegionLocation [" + hregion.getRegionInfo().getRegionNameAsString() +
                              " endKey: " + Hex.encodeHexString(hregion.getRegionInfo().getEndKey()) + " for transaction [" + transactionId + "]");
        TransactionRegionLocation trRegion = new TransactionRegionLocation(hregion.getRegionInfo(),
                                                                             hregion.getServerName());
// Save hregion for now
        boolean added = participatingRegions.add(trRegion);

        if (added) {
            if (LOG.isTraceEnabled()) LOG.trace("Added new HRegion to participatingRegions [" + trRegion.getRegionInfo().getRegionNameAsString() + "] to transaction ["
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

    public void clearRetryRegions() {
        retryRegions.clear();
    }

    Set<TransactionRegionLocation> getRegionsToIgnore() {
        return regionsToIgnore;
    }

    void addRegionToIgnore(final TransactionRegionLocation region) {
        // Changing to an HRegionLocation for now
        regionsToIgnore.add(region);
    }

    Set<TransactionRegionLocation> getRetryRegions() {
        return retryRegions;
    }

    void addRegionToRetry(final TransactionRegionLocation region) {
        // Changing to an HRegionLocation for now
        retryRegions.add(region);
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
     * Set the startEpoc.
     *
     */
    public void setStartEpoch(final long epoch) {
        this.startEpoch = epoch;
    }

    /**
     * Get the startEpoch.
     *
     * @return Return the startEpoch.
     */
    public long getStartEpoch() {
        return startEpoch;
    }

    /**
     * Set the startId.
     *
     */
    public void setStartId(final long id) {
        this.startId = id;
    }

    /**
     * Get the startId.
     *
     * @return Return the startId.
     */
    public long getStartId() {
        return startId;
    }

    /**
     * Set the commitId.
     *
     */
    public void setCommitId(final long id) {
        this.commitId = id;
    }

    /**
     * Get the commitId.
     *
     * @return Return the commitId.
     */
    public long getCommitId() {
        return commitId;
    }

    /**
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        return "transactionId: " + transactionId + ", startId: " + startId + ", commitId: " + commitId +
               ", startEpoch: " + startEpoch + ", participants: " + participatingRegions.size()
               + ", ignoring: " + regionsToIgnore.size() + ", hasDDL: " + hasDDLTx()
               + ", state: " + status.toString();
    }

    public int getParticipantCount() {
        return participatingRegions.size();
    }

    public int getRegionsToIgnoreCount() {
        return regionsToIgnore.size();
    }

    public int getRegionsRetryCount() {
        return retryRegions.size();
    }

    public String getStatus() {
      return status.toString();
    }

    public void setStatus(final TransState status) {
      this.status = status;
    }

    public boolean hasDDLTx() {
        return ddlTrans;   
    }

    public void setDDLTx(final boolean status) {
        this.ddlTrans = status;
    }

    public void setRetried(boolean val) {
        this.hasRetried = val;
    }

    public boolean hasRetried() {
      return this.hasRetried;
    }

}
