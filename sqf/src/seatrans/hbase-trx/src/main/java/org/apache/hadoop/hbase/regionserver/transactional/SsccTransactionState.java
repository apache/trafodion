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

package org.apache.hadoop.hbase.regionserver.transactional;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.NavigableSet;
import java.util.Set;
import java.util.Arrays;
import java.util.HashMap;

import org.apache.commons.codec.binary.Hex;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hbase.Cell;
import org.apache.hadoop.hbase.CellUtil;
import org.apache.hadoop.hbase.Tag;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.KeyValueUtil;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.KeyValue.Type;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.regionserver.InternalScanner;
import org.apache.hadoop.hbase.regionserver.KeyValueScanner;
import org.apache.hadoop.hbase.regionserver.ScanQueryMatcher;
import org.apache.hadoop.hbase.regionserver.ScanType;
import org.apache.hadoop.hbase.regionserver.ScanInfo;
import org.apache.hadoop.hbase.regionserver.wal.WALEdit;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.EnvironmentEdgeManager;
import org.apache.hadoop.io.DataInputBuffer;
import org.apache.hadoop.hbase.regionserver.wal.HLog;
import org.apache.hadoop.hbase.client.SsccConst;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Holds the state of a SSCC transaction. 
 */
public class SsccTransactionState extends TransactionState{

    private static final Log LOG = LogFactory.getLog(SsccTransactionState.class);
    private List<byte[]>  putRows =  Collections.synchronizedList(new LinkedList<byte[]>());
    private List<Delete>  delRows =  Collections.synchronizedList(new LinkedList<Delete>());
    private List<Scan> scans = Collections.synchronizedList(new LinkedList<Scan>());
    private Map<String, byte[]> colUpdatedByTransaction = new HashMap<String, byte[]>();  

    private long commitSequenceId;
    private long startId_;

    public SsccTransactionState(final long transactionId, final long rLogStartSequenceId, AtomicLong hlogSeqId, final HRegionInfo regionInfo,
                                                 HTableDescriptor htd, HLog hLog, boolean logging, long SsccSequenceId) {

        super(transactionId,rLogStartSequenceId,hlogSeqId,regionInfo,htd,hLog,logging);
        setStartId(SsccSequenceId);
        LOG.info("SsccTransactionState : new state object for transid: " + transactionId + " with sequence: " + SsccSequenceId + " complete");
    }

    public void addToColList(byte[] rowkey, byte[] collist)
    {
        synchronized(colUpdatedByTransaction){
            String keystr= new String(rowkey);
            if(colUpdatedByTransaction.containsKey(keystr) ==true)
            {
                colUpdatedByTransaction.remove(keystr);
                colUpdatedByTransaction.put(keystr, collist);
            }
            else
            {
                colUpdatedByTransaction.put(keystr, collist);
            }
        }
    }
    
    public byte[] getColList(byte[] rowkey)
    {
        String keystr= new String(rowkey);
        synchronized(colUpdatedByTransaction){
            if(colUpdatedByTransaction == null) return null;
            if(colUpdatedByTransaction.containsKey(keystr) ==true)
            {
                return colUpdatedByTransaction.get(keystr);
            }
            else
                return null;
        }
    }
    /**
     * Get the puts rowkeys in transaction order.
     * 
     * @return Return the putRows.
     */
    public List<byte[]> getPutRows() {
        return putRows;
    }

    /**
     * Get the del rowkeys in transaction order.
     * 
     * @return Return the putRows.
     */
    public List<Delete> getDelRows() {
        return delRows;
    }
    
    /**
     * add a rowkey into putRows
     */
    public void addToPutList(byte[] rowkey) {
       putRows.add(rowkey);
    }

    /**
     * add a rowkey into delRows
     */
    public void addToDelList(Delete del) {
       delRows.add(del);
    }

    public void clearStateResource()
    {
        //TODO, clear all resources here
        //should be invoked by retireTransaction
        delRows.clear();
        putRows.clear();
    }

    /**
     * Get the commitId for this transaction.
     * 
     * @return Return the commitSequenceId.
     */
    public synchronized long getCommitId() {
        return commitSequenceId;
    }

    /**
     * Set the commitId for this transaction.
     * 
     */
    public synchronized void setCommitId(final long Id) {
        this.commitSequenceId = Id;
    }

    /**
     * return true if the transaction perform write operations: put or delete.
     * 
     */    
    public boolean hasWrite() {
        return ( putRows.size() > 0  || delRows.size() > 0 );
    }
    
    /**
    */
    public static boolean isSelfDelete(Cell status, long startId)
    {
        return status.getTimestamp() == startId
            && SsccConst.isDeleteStatus(CellUtil.cloneValue(status));        
    }
    
    public void setStartId(long startId)
    {
        startId_ = startId;
    }
    
    public long getStartId()
    {
        return startId_;
    }

    /**
      each row contains two metadata column in _mt CF: status and versions
      A row can be visible must meet following rules (SSCC algorithm):
      1. the current startID is in status list as key (timestamp of Cell), and its value in status indicate it is not deleted
         This is because the row is updated by this own transaction, so must be visible. A delete flag will be 
         set in status value if the operation is DELETE.
      2. the current startID is not in status list as key, but it is in the version list
         This is the case the row is a commited row, updated by other transactions
         we need to find the last transaction do commit.
         If its startId is smaller than current transaction's startId, then this row is also visible. Otherwise, still 
         not visible.

    */
    public boolean handleResult(Cell inputCell,List<Cell> statusList,List<Cell> versionList,List<Cell> colList, long gTransId) 
    {
        byte[] status = null;
        byte[] version = null;
        long startId = getStartId();
        long commitId = 0;
        long maxCommitId = 0;
        long maxStartId = 0;
        boolean finalret = false;// (maxStartId == thisTs);
        long thisTs = inputCell.getTimestamp();

        byte[] cNm = CellUtil.cloneQualifier(inputCell);  //column name

        byte[] matcher = byteMerger("|".getBytes(),null);
        matcher = byteMerger(matcher, cNm);
        matcher = byteMerger(matcher,"|".getBytes());        
        try{
            LOG.info("handleResult : ENTER: matcher: " + matcher);//statusList size is " + statusList.size() + " versionList size is " + versionList.size() + "gid is " + gTransId);
            /*
            if(statusList==null && versionList == null) //non-transactional data, user put it directly
            {
                 //maybe we can set a CQD for this?
                 //Now, I make this one visible.
                return true;
            }   
            */        
            if(thisTs > startId)  //only for debug checking, like an assert
            {
                LOG.error("SHOULD NOT HAPPEN handleResult thisTs " + thisTs+ " > startId " + startId);
                return false;  
            }

            if (statusList != null)  
            {
                byte[] lv_colList = null;
                if(colList != null)
                {
                    //get the collist for this startId
                    for( Cell c : colList )
                    {
                        if( c.getTimestamp() == startId )
                            lv_colList = byteMerger(lv_colList,CellUtil.cloneValue(c) );
                    }
                }
                if(statusList.size() > 0) {
                    // This row is currently being modified by some transaction(s).  Check to see if our transaction is responsible.
                    boolean updateByMe = false;
                    List<byte[]> statusValList =  new LinkedList<byte[]>();
                    for (Cell c : statusList)
                    {
                        // Check the transactionId responsible for this row
                        LOG.info("handleResult check status item if transactionId is self ? gTransId is " + gTransId + " to compare to " + SsccConst.getTransactionId( CellUtil.cloneValue(c) ) );
                        if( gTransId == SsccConst.getTransactionId( CellUtil.cloneValue(c) ))
                        {
                            // This row is updated by the current transaction
                            if(SsccConst.isDeleteStatus(CellUtil.cloneValue(c)) == true ) //this row is delete
                            {
                                //In Trafodion, we have to delete the whole row, instead of individual cell
                                LOG.info("handleResult row is deleted by this transaction");
                                return false;
                            }
                            if(indexOf(lv_colList,matcher) != -1) // This cell is updated by me
                            {
                                statusValList.add(CellUtil.cloneValue(c));
                                LOG.info("handleResult check status item if transactionId is self update");
                                updateByMe = true;                           
                            }
                            else
                                LOG.info("this cell is not updated by me , matcher " + Bytes.toString(matcher) +  " lv_colList is " + Bytes.toString(lv_colList) );
                        }
                        else
                            LOG.info("handleResult check status item if transactionId is NOT self gTransId is " + gTransId + " to compare to " + SsccConst.getTransactionId( CellUtil.cloneValue(c) ) );
                    }
                    if (updateByMe == true)  // I am updating this cell, but is this the version update by me?
                    {
                        if(thisTs == startId)
                        {
                            boolean isit = SsccConst.isSelfUpdate(statusValList,startId);
                            LOG.info("handleResult this is updated by me " + isit);
                            return isit;  
                        }
                        else
                        {
                            LOG.info("handleResult is not update by me for this row");
                            return false;
                        }
                    }
                }
            }
            if(versionList == null){
                LOG.info("handleResult versionList is null, so return false, invisible row");
                return false;
            }

            // Status is empty: no active update of this row, so need to check the commit version
            // or status is not empty but current transaction not update it, still need to check the commit version
            for(Cell v: versionList) {
                    version=CellUtil.cloneValue(v);
                    commitId=v.getTimestamp();
                    long itsStartId = SsccConst.getVersionStartID(version);

                    if (commitId > startId) {
                       LOG.info("handleResult cell committed after our startId, ignoring");
                       continue;
                    }

                    //check if this col is updated by this commit?
                    boolean sameCommit = false;
                    byte[] allcollist = null;
                    if(colList != null)
                    {
                        for( Cell colc : colList)
                        {
                            LOG.info("colc list, itstartid " + itsStartId + " colc.timestamp " + colc.getTimestamp() );
                            if(itsStartId == colc.getTimestamp() )
                            {
                                byte[] colVBytes = CellUtil.cloneValue(colc);
                                allcollist = byteMerger(allcollist,colVBytes);
                            }
                        }

                        if(indexOf(allcollist,matcher) != -1) 
                        {
                            sameCommit = true;
                            LOG.info("handleResult: check version item cid: " + commitId );
                            //find the last commited row, and its time must be before this transaction start
                            if(commitId > maxCommitId && commitId < getStartId()) 
                            {
                                maxCommitId= commitId;
                                maxStartId = SsccConst.getVersionStartID(version);
                            }
                        }
                        else {
                            LOG.info("this cell is not updated by this commit version, matcher " + Arrays.toString(matcher) +  " collist is " + Arrays.toString(allcollist) );
                        }
                    }
                    else {
                      LOG.info("handleResult colList is null");
                    }
            }
            if(maxStartId == 0 && commitId < getStartId()) // out of MAX_VERSION window, this row must be visible
            {
               if (commitId > getStartId()) {
                  LOG.info("handleResult : this cell committed after our startId");
               }
               LOG.info("handleResult : this cell is out of MAX_VERSION window check");
               return true;
            }
            finalret = (maxStartId == thisTs);
            LOG.info("handleResult: finally return " + finalret + " maxId is " + maxStartId + " thisTs is " + thisTs);
        }
        catch( Exception e) {
           LOG.error("handleResult:  get Caught exception " + e.getMessage() + ""  );
        }
        return finalret;
    }

    public synchronized void addScan(final Scan scan) {
        if (LOG.isTraceEnabled()) LOG.trace(String.format("Adding scan for transaction [%s], from [%s] to [%s]", transactionId,
            scan.getStartRow(),scan.getStopRow()));
        scans.add(scan);
    }

    public static int indexOf(byte[] source, byte[] find) {
       if (source == null) {
          return -1;
       }
       if (find == null) {
          return -1;
       }
       if (find.length > source.length) {
          return -1;
       }
       final int maxIndex = source.length - find.length;
       final int maxLength = find.length;
       final byte firstByte = find[0];
       int index = 0;
       Loop:
       do {
          if (source[index] == firstByte) {
            for (int i = 1; i < maxLength; i++) {
              if (source[index + i] != find[i]) {
                 continue Loop;
              }
            }
            return index;
          }
       } while (++index <= maxIndex);
       return -1;
    }

    public boolean hasConflict(List<Cell> statusList,List<Cell> versionList, boolean stateless,long startId,long gTransId) {
        LOG.info("Entry hasConflict, stateless: " + stateless + ", startId: " + startId + ", transId: " + gTransId );

        /*
        if(statusList==null && versionList == null) //non-transactional data, user put it directly
        {
             //maybe we can set a CQD for this?
             //Now, I make this one visible.
            return true;
        }
        */
        //if the status list is empty
        // no other update at this point
        if((statusList == null) && (stateless == true)){
            // Cannot have a conflict
            LOG.info("hasConflict statusList is null for stateless update: no conflict");
            return false;
        }
        if (statusList != null) {
           //status is not null
           if((statusList.size() == 0) && (stateless == true)){
              // Cannot have a conflict with an empty statusList
              LOG.info("hasConflict statusList size 0 for stateless update: no conflict");
              return false;
           }
           else {
              // StatusList > 0
              LOG.info("hasConflict statusList size > 0");
              if(checkStatusListForConflict( statusList,stateless,startId,gTransId) == true) {
                 LOG.info("hasConflict checkStatusListForConflict returned true");
                 return true;
              }
              else {
                 // If we are a stateless update we don't need to check the version
                 if (stateless) {
                    LOG.info("hasConflict checkStatusListForConflict returned false and this is a stateless update:  Skipping version checks");
                    return false;
                 }
              }
           } //  statusList.size() > 0
        }  // statusList == null
        LOG.info("hasConflict checking versionList for conflict");
        return checkVersionListForConflict( versionList,startId);
    }

    private boolean checkVersionListForConflict(List<Cell> versionList,long startId)
    {
        boolean ret = false;
        LOG.info("checkVersionListForConflict : ENTER startId is " + startId );
        if(versionList == null)
        {
            LOG.info("checkVersionListForConflict: versionList is null, return false");
            return false;
        }
        for(Cell v : versionList)
        {
            LOG.info("checkVersionListForConflict : iterate the version list v.timestamp is " + v.getTimestamp() );
            if(startId < v.getTimestamp() ) //some transaction commit after I beginTransaction
            {
                ret = true;
                break;
            }
        LOG.info("checkVersionListForConflict : 3");
        }
        LOG.info("checkVersionListForConflict : 4");
        return ret;
    }

    private boolean checkStatusListForConflict(List<Cell> statusList, boolean stateless,long startId,long gTransId)
    {
        LOG.info("checkStatusListForConflict: ENTER with startId " + startId );
        if(statusList == null) return false;
        LOG.info("checkStatusListForConflict: statusList is not null");
        int count = statusList.size();

        if(count == 0) {
           return false;
        }

        if(count == 1)
        {
            //get the status Cell
            Cell c = statusList.get(0);
            LOG.info("startId is " + startId + " c.timestamp is " + c.getTimestamp() + " c.value is " + Arrays.toString(CellUtil.cloneValue(c)));
            if(c.getTypeByte() == KeyValue.Type.DeleteFamilyVersion.getCode() )
            {
                LOG.info("the status cell is deleted");
                return false;
            }
            LOG.info("checkStatusListForConflict : check gTransId " + gTransId + " with status's transactionId " + SsccConst.getTransactionId( CellUtil.cloneValue(c) ) );
            return !(gTransId==SsccConst.getTransactionId( CellUtil.cloneValue(c) ));
        }

        if(count > 1){
           LOG.info("checkStatusListForConflict: statusList counter > 1, stateless: " + stateless);
           if (stateless != true) {
             return true; //there are more than 1 update, and this is a stateful update
           }
           for (Cell c : statusList) {
              if (SsccConst.getStatusValue(CellUtil.cloneValue(c)) == SsccConst.S_STATEFUL_BYTE) {
                 LOG.info("checkStatusListForConflict: stateful update already pending");
                 return true; //there is already a stateful update
              }
           }
        }

        return false;
    }

    public static byte[] byteMerger(byte[] byte_1, byte[] byte_2){
        if (byte_1 == null) 
        {
            if (byte_2 == null)
                return null;
            else
                return byte_2;
        }
        else
        {
            if (byte_2 == null)
                return byte_1;
        }
        
		byte[] byte_3 = new byte[byte_1.length+byte_2.length];
		System.arraycopy(byte_1, 0, byte_3, 0, byte_1.length);
		System.arraycopy(byte_2, 0, byte_3, byte_1.length, byte_2.length);
		return byte_3;
	}
}
