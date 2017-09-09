/**
 * * @@@ START COPYRIGHT @@@
 * *
 * * Licensed to the Apache Software Foundation (ASF) under one
 * * or more contributor license agreements.  See the NOTICE file
 * * distributed with this work for additional information
 * * regarding copyright ownership.  The ASF licenses this file
 * * to you under the Apache License, Version 2.0 (the
 * * "License"); you may not use this file except in compliance
 * * with the License.  You may obtain a copy of the License at
 * *
 * *   http://www.apache.org/licenses/LICENSE-2.0
 * *
 * * Unless required by applicable law or agreed to in writing,
 * * software distributed under the License is distributed on an
 * * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * * KIND, either express or implied.  See the License for the
 * * specific language governing permissions and limitations
 * * under the License.
 * *
 * * @@@ END COPYRIGHT @@@
 * **/

package org.apache.hadoop.hbase.coprocessor.transactional;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Iterator;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.regionserver.HRegion;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionalRegionScannerHolder;
import org.apache.hadoop.hbase.regionserver.transactional.TrxTransactionState;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionState;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.zookeeper.ZKUtil;
import org.apache.hadoop.hbase.zookeeper.ZooKeeperWatcher;
import org.apache.zookeeper.KeeperException;

public class SplitBalanceHelper {
    private static final Log LOG = LogFactory.getLog(SplitBalanceHelper.class);

    private Path flushPath;

    private static String zkTable;
    private static String zSplitBalPath = TrxRegionObserver.zTrafPath + "splitbalance/";
    private static String zSplitBalPathNoSlash = TrxRegionObserver.zTrafPath + "splitbalance";
    private static String SPLIT = "SPLIT";
    private static String BALANCE = "BALANCE";
    private static final String FLUSH_PATH = "traf.txn.out";
    private static AtomicBoolean needsCleanup = new AtomicBoolean(true);
    private String balancePath;
    private String splitPath;
    private String regionPath;

    private ZooKeeperWatcher zkw;
    private HRegionInfo hri;
    private HRegion region;
    private String tablename;

    public SplitBalanceHelper(HRegion my_Region, ZooKeeperWatcher zkw, Configuration conf) {

        String parentZNode = conf.get(HConstants.ZOOKEEPER_ZNODE_PARENT,
                                      HConstants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);
        SplitBalanceHelper.zkTable = parentZNode + "/table";
        if(LOG.isDebugEnabled()) LOG.debug("zkTable value: " + SplitBalanceHelper.zkTable);

        String fileName = FLUSH_PATH + getTimeStamp();
        this.region = my_Region;
        this.hri = my_Region.getRegionInfo();
        this.zkw = zkw;
        this.tablename = my_Region.getTableDesc().getNameAsString();
        try {
            if (ZKUtil.checkExists(zkw, zSplitBalPathNoSlash) == -1) {
                if (LOG.isDebugEnabled()) LOG.debug("HELPER create with parents");
                ZKUtil.createWithParents(zkw, zSplitBalPathNoSlash);
            }
        } catch (KeeperException ke) {
            LOG.error("ERROR: Zookeeper exception: " + ke);
        }
        this.flushPath = new Path(region.getRegionFileSystem().getRegionDir(), fileName);
        regionPath = zSplitBalPath + this.tablename + "/" + hri.getEncodedName();
        balancePath = regionPath + "/" + BALANCE + "/";
        splitPath = regionPath + "/" + SPLIT + "/";

        if (SplitBalanceHelper.needsCleanup.compareAndSet(true, false)) {
            zkCleanup();
        }
    }

    public Path getPath() {
        return flushPath;
    }

    public boolean getSplit() {
        return getSplit(null);
    }

    public boolean getSplit(StringBuilder path) {
        try {
            byte[] splPath = ZKUtil.getData(zkw, splitPath.substring(0, splitPath.length() - 1));
            if (splPath == null) {
                return false;
            } else {
                if (path != null)
                    path.append(splPath.toString());
                if (LOG.isDebugEnabled()) LOG.debug("Split information retrieved, path is: " + splPath.toString());
                return true;
            }
        } catch (Exception e) {
            if (LOG.isErrorEnabled()) LOG.error("Keeper exception: " + e);
            return false;
        }
    }

    public void setSplit(HRegion leftRegion, HRegion rightRegion) throws IOException {
        String zLeftKey = zSplitBalPath + leftRegion.getRegionInfo().getEncodedName();
        String zRightKey = zSplitBalPath + rightRegion.getRegionInfo().getEncodedName();

        try {
            if (ZKUtil.checkExists(zkw, balancePath.substring(0, balancePath.length() - 1)) != -1) {
                clearBalance();
            }
            if (LOG.isDebugEnabled()) LOG.debug("Split checking for left key ");
            if (ZKUtil.checkExists(zkw, zLeftKey) == -1) {
                if (LOG.isDebugEnabled()) LOG.debug("Split creating left key with parents");
                ZKUtil.createWithParents(zkw, zLeftKey);
            }
            if (LOG.isDebugEnabled()) LOG.debug("Split checking for right key ");
            if (ZKUtil.checkExists(zkw, zRightKey) == -1) {
                if (LOG.isDebugEnabled()) LOG.debug("Split creating right key with parents");
                ZKUtil.createWithParents(zkw, zRightKey);
            }
            if (LOG.isDebugEnabled()) LOG.debug("Split createAndFailSilent for left key ");
            ZKUtil.createAndFailSilent(zkw, zLeftKey + "/" + SPLIT, Bytes.toBytes(flushPath.toString()));
            if (LOG.isDebugEnabled()) LOG.debug("Split createAndFailSilent for right key ");
            ZKUtil.createAndFailSilent(zkw, zRightKey + "/" + SPLIT, Bytes.toBytes(flushPath.toString()));
            if (LOG.isDebugEnabled()) LOG.debug("Split coordination node written for " + leftRegion.getRegionInfo().getRegionNameAsString() + " and " + rightRegion.getRegionInfo().getRegionNameAsString());
        } catch (KeeperException ke) {
            LOG.error("ERROR: Zookeeper exception: " + ke);
        }
    }

    public void setSplit() {

        try {
            if (ZKUtil.checkExists(zkw, balancePath.substring(0, balancePath.length() - 1)) != -1) {
                clearBalance();
            }
            if (ZKUtil.checkExists(zkw, splitPath.substring(0, splitPath.length() - 1)) == -1) {
                ZKUtil.createWithParents(zkw, splitPath.substring(0, splitPath.length() - 1));
            }
            ZKUtil.createSetData(zkw, splitPath.substring(0, splitPath.length() - 1), Bytes.toBytes(flushPath.toString()));
            if (LOG.isDebugEnabled()) LOG.debug("Setting split coordination node for " + hri.getRegionNameAsString());
        } catch (KeeperException ke) {
            LOG.error("ERROR: Zookeeper exception: " + ke);
        }
    }

    public void clearSplit() {
        if (LOG.isTraceEnabled()) LOG.trace("clearSplit called for region: " + this.hri.getRegionNameAsString());
        try {
            ZKUtil.deleteNodeRecursively(zkw, regionPath);
        } catch (KeeperException ke) {
            LOG.error("Zookeeper exception: " + ke);
        }
    }

    public boolean getBalance(StringBuilder path) {
        try {
            byte[] balPath = ZKUtil.getData(zkw, balancePath.substring(0, balancePath.length() - 1));
            if (balPath == null)
                return false;
            else {
                path.append(new String(balPath));
                if (LOG.isDebugEnabled()) LOG.debug("Balance information retrieved, path is: " + new String(balPath));
                return true;
            }
        } catch (Exception e) {
            if (LOG.isErrorEnabled())
                LOG.error("Keeper exception: " + e);
        }
        return true;
    }

    public void setBalance() throws IOException {

        try {
            if (ZKUtil.checkExists(zkw, splitPath.substring(0, splitPath.length() - 1)) != -1) {
                throw new IOException("SPLIT node already exists when trying to add BALANCE node");
            }

            if (ZKUtil.checkExists(zkw, balancePath.substring(0, balancePath.length() - 1)) == -1) {
                if (LOG.isDebugEnabled()) LOG.debug("setBalance createWithParents balancePath");
                ZKUtil.createWithParents(zkw, balancePath.substring(0, balancePath.length() - 1));
            }
            ZKUtil.createSetData(zkw, balancePath.substring(0, balancePath.length() - 1), Bytes.toBytes(flushPath.toString()));
            if (LOG.isDebugEnabled()) LOG.debug("Setting balance coordination node for " + hri.getRegionNameAsString());

        } catch (KeeperException ke) {
            LOG.error("ERROR: Zookeeper exception: " + ke);
        }
    }

    public void clearBalance() {
        if (LOG.isTraceEnabled()) LOG.trace("clearBalance called for region: " + this.hri.getRegionNameAsString());
        try {
            ZKUtil.deleteNodeRecursively(zkw, regionPath);
        } catch (KeeperException ke) {
            LOG.error("Zookeeper exception: " + ke);
        }
    }

    private long getTimeStamp() {
        return System.currentTimeMillis();
    }

    protected boolean pendingListClear(Set<TrxTransactionState> commitPendingTransactions) throws IOException {
        if (commitPendingTransactions.isEmpty()) {
            if (LOG.isDebugEnabled())
                LOG.debug("pendingListClear is true because commitPendingTransactions is empty " + hri.getRegionNameAsString());
            return true;
        } else {
            // Check to see if all of the TrxTransaction state objects
            // have dropTable Recorded, in which case the pending list is
            // considered clear of pending list.
            for (TrxTransactionState transactionState : commitPendingTransactions) {
                // if even one transaction state does not have drop table recorded
                // then pendingList is not yet clear.
                if (!transactionState.dropTableRecorded()) {
                    if (LOG.isDebugEnabled())
                        LOG.debug("pendingListClear is false commitPendingTransactions is not empty "
                                + hri.getRegionNameAsString());
                    return false;
                }
            }
            // Reaching here means pendingListClear.
            LOG.info("pendingListClear is true because dropTableRecorded is true " + hri.getRegionNameAsString());
            return true;
        }
    }

    //Returning true indicates scannerList is Clear.
    protected boolean scannersListClear(ConcurrentHashMap<Long, TransactionalRegionScannerHolder> scanners,
    									ConcurrentHashMap<String, TrxTransactionState> transactionsById) throws IOException {
    	  if(scanners.isEmpty()) 
    	  {
    	  	if (LOG.isDebugEnabled()) LOG.debug("scannersListClear Scanners is empty: " + hri.getRegionNameAsString());
    	  	return true;
    	  }
    	  else
    	  {
    	  	if (LOG.isDebugEnabled()) LOG.debug("scannersListClear Scanners is not empty: " + hri.getRegionNameAsString());
    	  	Iterator<Map.Entry<Long, TransactionalRegionScannerHolder>> scannerIter = scanners.entrySet().iterator();
    	  	TransactionalRegionScannerHolder rsh = null;
          Map.Entry<Long, TransactionalRegionScannerHolder> entry;
    	  	while(scannerIter.hasNext())
    	  	{
    	  		entry = scannerIter.next();
            rsh = entry.getValue();
            if (rsh != null)
            {
            	if (LOG.isDebugEnabled()) LOG.debug("scannersListClear Active Scanner is: "+ rsh.scannerId +
            			" Txid: "+ rsh.transId + " Region: " + hri.getRegionNameAsString());
              String key = hri.getRegionNameAsString() + rsh.transId;
              TrxTransactionState trxState = transactionsById.get(key);
              
              //if trxState is present means there is activity with this region.
              //Hence don't return true.
              if(trxState != null)
              {
             		LOG.info("scannersListClear Active Scanner found, ScannerId: " + 
              				 rsh.scannerId + " Txid: "+ rsh.transId + " Region: " + hri.getRegionNameAsString());
              	return false;
          			
              }
            }
    	  	}
    	  	//Reaching here means, there is no active scanner.
    	  	return true;
    	  }
    }

    protected void pendingWait(Set<TrxTransactionState> commitPendingTransactions, int pendingDelayLen) throws IOException {
        int count = 1;
        while (!pendingListClear(commitPendingTransactions)) {
            try {
                if (LOG.isDebugEnabled()) LOG.debug("pendingWait() delay, count " + count++ + " on: " + hri.getRegionNameAsString());
                Thread.sleep(pendingDelayLen);
            } catch (InterruptedException e) {
                String error = "Problem while calling sleep() on pendingWait delay, " + e;
                if (LOG.isErrorEnabled()) LOG.error("Problem while calling sleep() on preSplit delay, returning. " + e);
                throw new IOException(error);
            }
        }
    }

    /*
    protected void scannersWait(ConcurrentHashMap<Long, TransactionalRegionScannerHolder> scanners, int pendingDelayLen)
            throws IOException {
        int count = 1;
        while (!scannersListClear(scanners)) {
            try {
                if (LOG.isDebugEnabled()) LOG.debug("scannersWait() delay, count " + count++ + " on: " + hri.getRegionNameAsString());
                Thread.sleep(pendingDelayLen);
            } catch (InterruptedException e) {
                String error = "Problem while calling sleep() on scannersWait delay, " + e;
                if (LOG.isErrorEnabled()) LOG.error("Problem while calling sleep() on preSplit delay, returning. " + e);
                throw new IOException(error);
            }
        }
    }
    */

    protected void pendingAndScannersWait(Set<TrxTransactionState> commitPendingTransactions,
            ConcurrentHashMap<Long, TransactionalRegionScannerHolder> scanners,
            ConcurrentHashMap<String, TrxTransactionState> transactionsById, int pendingDelayLen) throws IOException {
        int count = 1;
        while (!scannersListClear(scanners, transactionsById) || !pendingListClear(commitPendingTransactions)) {
            try {
                if (LOG.isDebugEnabled()) LOG.debug("pendingAndScannersWait() delay, count " + count++ + " on: " + hri.getRegionNameAsString());
                Thread.sleep(pendingDelayLen);
            } catch (InterruptedException e) {
                String error = "Problem while calling sleep() on pendingAndScannersWait delay, " + e;
                if (LOG.isErrorEnabled()) LOG.error("Problem while calling sleep() on pendingAndScannersWait delay, returning. " + e);
                throw new IOException(error);
            }
        }
    }

    protected void activeWait(ConcurrentHashMap<String, TrxTransactionState> transactionsById, int activeDelayLen,
            int splitDelayLimit) throws IOException {
        int counter = 0;
        int minutes = 0;
        int currentMin = 0;

        boolean delayMsg = false;
        while (!transactionsById.isEmpty()) {
            try {
                delayMsg = true;
                Thread.sleep(activeDelayLen);
                counter++;
                currentMin = (counter * activeDelayLen) / 60000;

                if (currentMin > minutes) {
                    minutes = currentMin;
                    if (LOG.isInfoEnabled()) LOG.info("Delaying split due to transactions present. Delayed : " + minutes + " minute(s) on "
                                + hri.getRegionNameAsString());
                }
                if (minutes >= splitDelayLimit) {
                    if (LOG.isWarnEnabled()) LOG.warn("Surpassed split delay limit of " + splitDelayLimit + " minutes. Continuing with split");
                    delayMsg = false;
                    break;
                }
            } catch (InterruptedException e) {
                String error = "Problem while calling sleep() on preSplit delay - activeWait: " + e;
                if (LOG.isErrorEnabled()) LOG.error(error);
                throw new IOException(error);
            }
        }
        if (delayMsg) {
            if (LOG.isWarnEnabled()) LOG.warn("Continuing with split operation, no active transactions on: " + hri.getRegionNameAsString());
        }
    }

    protected void zkCleanup() {
        if (LOG.isTraceEnabled()) LOG.trace("zkCleanup -- ENTRY");
        try {
            List<String> trafTables = ZKUtil.listChildrenNoWatch(zkw, zSplitBalPathNoSlash);
            List<String> hbaseTables = ZKUtil.listChildrenNoWatch(zkw, SplitBalanceHelper.zkTable);
            if(trafTables != null && hbaseTables != null) {
              for (String tableName : trafTables) {
                if (!hbaseTables.contains(tableName)) {
                    if (LOG.isTraceEnabled()) LOG.trace("zkCleanup, removing " + zSplitBalPath + tableName);
                    ZKUtil.deleteNodeRecursively(zkw, zSplitBalPath + tableName);
                }
              }
            }
        } catch (KeeperException ke) {
            if (LOG.isErrorEnabled()) LOG.error("zkCleanup error, please check your ZooKeeper: " + ke);
        }
        if (LOG.isTraceEnabled()) LOG.trace("zkCleanup -- EXIT");
    }

}
