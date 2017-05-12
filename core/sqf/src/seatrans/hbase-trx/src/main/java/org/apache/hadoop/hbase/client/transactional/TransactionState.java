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
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.HConstants;

/**
 * Holds client-side transaction information. Client's use them as opaque objects passed around to transaction
 * operations.
 */
public class TransactionState {

    static final Log LOG = LogFactory.getLog(TransactionState.class);

    // Current transactionId has the following composition:
    //  int   sequenceId
    //  short nodeId
    //  short clusterId
    private final long transactionId;
    private TransState status;
    private long startEpoch;
    private long startId;
    private long commitId;
    private long recoveryASN;

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
    private Throwable hasError;
    private boolean localTransaction;
    private static boolean envLocalTransaction;
    private boolean ddlTrans;
    private static boolean useConcurrentHM = false;
    private boolean hasRetried = false;
    private boolean exceptionLogged = false;
    private long nodeId;
    private long clusterId;
    private String recordException;
    private static long TM_MAX_REGIONSERVER_STRING = 3072;
    public Set<String> tableNames = Collections.synchronizedSet(new HashSet<String>());
    public Set<TransactionRegionLocation> participatingRegions;
    /**
     * Regions to ignore in the twoPase commit protocol. They were read only, or already said to abort.
     */
    public Set<TransactionRegionLocation> regionsToIgnore = Collections.synchronizedSet(new HashSet<TransactionRegionLocation>());
    private Set<TransactionRegionLocation> retryRegions = Collections.synchronizedSet(new HashSet<TransactionRegionLocation>());

    private native void registerRegion(long transid, long startid, int port, byte[] hostname, long startcode, byte[] regionInfo);

    public boolean islocalTransaction() {
      return localTransaction;
    }

    static {
       String concurrentHM = System.getenv("DTM_USE_CONCURRENTHM");
       if (concurrentHM != null) 
           useConcurrentHM = (Integer.parseInt(concurrentHM) == 0) ? false : true;
       else
           useConcurrentHM = false;
       if (useConcurrentHM) 
          LOG.info("Using ConcurrentHashMap synchronization to synchronize participatingRegions");
       else 
          LOG.info("Using synchronizedSet to synchronize participatingRegions");
       String localTxns = System.getenv("DTM_LOCAL_TRANSACTIONS");
       if (localTxns != null) 
          envLocalTransaction = (Integer.parseInt(localTxns) == 0) ? false : true;
       else
          envLocalTransaction = false; 
    }

    public TransactionState(final long transactionId) {
        this.transactionId = transactionId;
        setStatus(TransState.STATE_ACTIVE);
        countLock = new Object();
        commitSendLock = new Object();
        requestPendingCount = 0;
        requestReceivedCount = 0;
        commitSendDone = false;
        hasError = null;
        ddlTrans = false;
        recoveryASN = -1;
        setNodeId();
        setClusterId();
        recordException = null;

        if(useConcurrentHM) {
          participatingRegions = Collections.newSetFromMap((new ConcurrentHashMap<TransactionRegionLocation, Boolean>()));
        }
        else {
          participatingRegions = Collections.synchronizedSet(new HashSet<TransactionRegionLocation>());
        }

        localTransaction = envLocalTransaction;
        if (localTransaction) {
          if (LOG.isTraceEnabled()) LOG.trace("TransactionState local transaction begun: " + transactionId);
        }
        else {
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
            hasError = null;  // reset, just in case
            requestPendingCount = count;
        }
    }

    /**
     * 
     * method  : requestpendingcountdec
     * params  : none
     * return  : void
     * purpose : decrease number of outstanding replies needed and wake up any waiters
     *           if we receive the last one 
     */
    public void  requestPendingCountDec(Throwable exception)
    {
       synchronized (countLock)
       {
          requestReceivedCount++;
          if (exception != null && hasError == null)
             hasError = exception;
          if (requestReceivedCount == requestPendingCount)
             countLock.notify();
       }
    }

    /**
     * 
     * Method  : completeRequest
     * Params  : None
     * Return  : Void
     * Purpose : Hang thread until all replies have been received
     */
    public void completeRequest() throws InterruptedException, IOException
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

        if (hasError != null)  {
            hasError.fillInStackTrace();
            throw new IOException("Exception at completeRequest()", hasError);
        }
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
    
    /**
    *
    * Method  : recordException
    * Params  : 
    * Return  : void
    * Purpose : Record an exception encountered by executor threads. 
    */
    public synchronized void recordException(String exception)
    {
      if (exception != null && recordException == null)
        recordException = exception;
    }
    
    public String getRecordedException()
    {
      return recordException;
    }
    public void resetRecordedException()
    {
      recordException = null;
    }

    // Used at the client end - the one performing the mutation - e.g. the SQL process
    public void registerLocation(final HRegionLocation location) throws IOException {
        byte [] lv_hostname = location.getHostname().getBytes();
        int lv_port = location.getPort();
        long lv_startcode = location.getServerName().getStartcode();
        HRegionInfo regionInfo = location.getRegionInfo();

        byte [] lv_byte_region_info = location.getRegionInfo().toByteArray();
        if (LOG.isTraceEnabled()) LOG.trace("TransactionState.registerLocation: [" + location.getRegionInfo().getEncodedName() +
          "], endKey: " + Hex.encodeHexString(location.getRegionInfo().getEndKey()) + " transaction [" + transactionId + "]");

        if (islocalTransaction()) {
          if(LOG.isTraceEnabled()) LOG.trace("TransactionState.registerLocation local transaction not sending registerRegion.");
        }
        else {
          if (lv_byte_region_info.length > TM_MAX_REGIONSERVER_STRING){
             String skey = (Bytes.equals(location.getRegionInfo().getStartKey(), HConstants.EMPTY_START_ROW)) ? "skey=null" : ("skey=" + Hex.encodeHexString(location.getRegionInfo().getStartKey()));
             String ekey = (Bytes.equals(location.getRegionInfo().getEndKey(), HConstants.EMPTY_END_ROW)) ? "ekey=null" : ("ekey=" + Hex.encodeHexString(location.getRegionInfo().getEndKey()));
             IOException ioe = new IOException("RegionInfo is too large (" + lv_byte_region_info.length
                     + "), try reducing table keys for "
                     + location.getRegionInfo().getTable().getNameAsString()
                     + " skey: " + skey + " ekey: " + ekey);
             LOG.error("RegionInfo is too large (" + lv_byte_region_info.length
                     + "), try reducing table keys for "
                     + location.getRegionInfo().getTable().getNameAsString()
                     + " skey: " + skey + " ekey: " + ekey, ioe);
             throw ioe;
          }
          if (LOG.isDebugEnabled()){
              String skey = (Bytes.equals(location.getRegionInfo().getStartKey(), HConstants.EMPTY_START_ROW)) ? "skey=null" : ("skey=" + Hex.encodeHexString(location.getRegionInfo().getStartKey()));
              String ekey = (Bytes.equals(location.getRegionInfo().getEndKey(), HConstants.EMPTY_END_ROW)) ? "ekey=null" : ("ekey=" + Hex.encodeHexString(location.getRegionInfo().getEndKey()));
              LOG.debug("TransactionState.registerLocation global transaction registering region "
                   + location.getRegionInfo().getTable().getNameAsString()
                   + " using byte array size: " + lv_byte_region_info.length + " for ts: "
                   + transactionId + " and startId: " + startId + " skey: " + skey + " ekey: " + ekey + " ");
          }
          registerRegion(transactionId, startId, lv_port, lv_hostname, lv_startcode, lv_byte_region_info);
        }
      }

    public boolean addRegion(final TransactionRegionLocation trRegion) {
        if (LOG.isDebugEnabled()) LOG.debug("addRegion ENTRY with trRegion [" + trRegion.getRegionInfo().getRegionNameAsString()
                + "], startKey: " + Hex.encodeHexString(trRegion.getRegionInfo().getStartKey()) + "], endKey: "
                  + Hex.encodeHexString(trRegion.getRegionInfo().getEndKey()) + " and transaction [" + transactionId + "]");

        boolean added = participatingRegions.add(trRegion);

        if (added) {
           if (LOG.isDebugEnabled()) LOG.debug("Added new trRegion (#" + participatingRegions.size()
                        + ") to participatingRegions ["        + trRegion.getRegionInfo().getRegionNameAsString()
                        + "], startKey: "      + Hex.encodeHexString(trRegion.getRegionInfo().getStartKey())
                        + "], endKey: "        + Hex.encodeHexString(trRegion.getRegionInfo().getEndKey())
                        + " and transaction [" + transactionId + "]");
        }
        else {
           if (LOG.isDebugEnabled()) LOG.debug("trRegion already present in (" + participatingRegions.size()
                       + ") participatinRegions ["    + trRegion.getRegionInfo().getRegionNameAsString()
                       + "], startKey: "      + Hex.encodeHexString(trRegion.getRegionInfo().getStartKey())
                       + "], endKey: "        + Hex.encodeHexString(trRegion.getRegionInfo().getEndKey())
                       + " and transaction [" + transactionId + "]");
        }

        return added;
    }

    public boolean addRegion(final HRegionLocation hregion) {

        if (LOG.isDebugEnabled()) LOG.debug("Creating new TransactionRegionLocation from HRegionLocation [" + hregion.getRegionInfo().getRegionNameAsString() +
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
     * Get the sequenceNum portion of the transactionId.
     *
     * @return Return the sequenceNum.
     */
    public long getTransSeqNum() {

       return transactionId & 0xFFFFFFFFL;
    }

    /**
     * Get the sequenceNum portion of the passed in transId.
     *
     * @return Return the sequenceNum.
     */
    public static long getTransSeqNum(long transId) {

       return transId & 0xFFFFFFFFL;
    }

    /**
     * Get the originating node of the transaction.
     *
     * @return Return the nodeId.
     */
    public long getNodeId() {
       return nodeId;
    }

    /**
     * Set the originating node of the transaction.
     *
     */
    private void setNodeId() {

       nodeId = ((transactionId >> 32) & 0xFFL);
    }

    /**
     * Get the originating node of the passed in transaction.
     *
     * @return Return the nodeId.
     */
    public static long getNodeId(long transId) {

        return ((transId >> 32) & 0xFFL);
    }

    /**
     * Get the originating clusterId of the transaction.
     *
     * @return Return the clusterId.
     */
    public long getClusterId() {

        return clusterId;

    }


    /**
     * Get the originating clusterId of the passed in transaction.
     *
     * @return Return the clusterId.
     */
    public static long getClusterId(long transId) {

        return (transId >> 48);
    }

    /**
     * Set the originating clusterId of the passed in transaction.
     *
     */
    private void setClusterId() {

        clusterId = (transactionId >> 48);
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
     * Set the recoveryASN.
     *
     */
    public void setRecoveryASN(final long value) {
        this.recoveryASN = value;
    }

    /**
     * Get the recoveryASN.
     *
     * @return Return the recoveryASN.
     */
    public long getRecoveryASN() {
        return recoveryASN;
    }

    /**
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        return "transactionId: " + transactionId + ", startId: " + startId + ", commitId: " + commitId +
               ", startEpoch: " + startEpoch + ", participants: " + participatingRegions.size()
               + ", ignoring: " + regionsToIgnore.size() + ", hasDDL: " + hasDDLTx()
               + ", recoveryASN: " + getRecoveryASN() + ", state: " + status.toString();
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

    public boolean hasPlaceHolder() {
       return false;
    }

    public synchronized void logExceptionDetails(final boolean ute)
    {
       if (exceptionLogged)
          return;
       int participantNum = 0;
       byte[] startKey;
       byte[] endKey_orig;
       byte[] endKey;
       boolean isIgnoredRegion = false;

       LOG.error("Starting " + (ute == true ? "UTE " : "NPTE ") + "for trans: " + this.toString());
       for (TransactionRegionLocation location : getParticipatingRegions()) {
          isIgnoredRegion = getRegionsToIgnore().contains(location);
          if (! isIgnoredRegion) {
             participantNum++;
          }

          startKey = location.getRegionInfo().getStartKey();
          endKey_orig = location.getRegionInfo().getEndKey();
          if (endKey_orig == null || endKey_orig == HConstants.EMPTY_END_ROW)
              endKey = null;
          else
              endKey = TransactionManager.binaryIncrementPos(endKey_orig, -1);

          LOG.error((ute == true ? "UTE " : "NPTE ") + "for transId: " + getTransactionId()
                    + " participantNum " + (isIgnoredRegion ? " Ignored region " : participantNum)
                    + " location " + location.getRegionInfo().getRegionNameAsString()
                    + " startKey " + ((startKey != null)?
                             (Bytes.equals(startKey, HConstants.EMPTY_START_ROW) ? "INFINITE" : Hex.encodeHexString(startKey)) : "NULL")
                    + " endKey " +  ((endKey != null) ?
                             (Bytes.equals(endKey, HConstants.EMPTY_END_ROW) ? "INFINITE" : Hex.encodeHexString(endKey)) : "NULL")
                    + " RegionEndKey " + ((endKey_orig != null) ?
                             (Bytes.equals(endKey_orig, HConstants.EMPTY_END_ROW) ? "INFINITE" : Hex.encodeHexString(endKey_orig)) : "NULL"));
       }
       exceptionLogged = true;
    }
}
