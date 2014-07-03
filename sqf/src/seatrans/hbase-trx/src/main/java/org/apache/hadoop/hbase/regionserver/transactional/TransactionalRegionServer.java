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
package org.apache.hadoop.hbase.regionserver.transactional;

import java.io.IOException;
import java.lang.Thread.UncaughtExceptionHandler;
import java.util.List;
import java.util.Map;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.NotServingRegionException;
import org.apache.hadoop.hbase.ServerName;
import org.apache.hadoop.hbase.RemoteExceptionHandler;
import org.apache.hadoop.hbase.catalog.CatalogTracker;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.ipc.HRegionInterface;
import org.apache.hadoop.hbase.ipc.ProtocolSignature;
import org.apache.hadoop.hbase.ipc.TransactionalRegionInterface;
import org.apache.hadoop.hbase.regionserver.HRegion;
import org.apache.hadoop.hbase.regionserver.HRegionServer;
import org.apache.hadoop.hbase.regionserver.Leases;
import org.apache.hadoop.hbase.regionserver.RegionScanner;
import org.apache.hadoop.hbase.regionserver.wal.HLog;
import org.apache.hadoop.hbase.regionserver.wal.HLogKey;
import org.apache.hadoop.hbase.regionserver.wal.HLogSplitter;
import org.apache.hadoop.hbase.regionserver.wal.WALActionsListener;
import org.apache.hadoop.hbase.regionserver.wal.WALEdit;
import org.apache.hadoop.hbase.util.Threads;
import org.apache.hadoop.io.MapWritable;
import org.apache.zookeeper.KeeperException;
import org.apache.hadoop.hbase.zookeeper.ZooKeeperWatcher;
import org.apache.hadoop.hbase.zookeeper.ZKUtil;
   
/**
 * RegionServer with support for transactions. Transactional logic is at the region level, so we mostly just delegate to
 * the appropriate TransactionalRegion.
 */
public class TransactionalRegionServer extends HRegionServer implements TransactionalRegionInterface {
//implements TransactionalRegionInterface {

    private static final String LEASE_TIME = "hbase.transaction.leasetime";
    private static String zNodePath = "/hbase/Trafodion/recovery/";
    private static final int DEFAULT_LEASE_TIME = 7200 * 1000;
    private static final int LEASE_CHECK_FREQUENCY = 1000;
    private static ZooKeeperWatcher zkw1 = null;
    private static ServerName sn;
    private static Object zkRecoveryCheckLock = new Object();

    static final Log LOG = LogFactory.getLog(TransactionalRegionServer.class);
    private final Leases transactionLeases;
    private final CleanOldTransactionsChore cleanOldTransactionsThread;

    private THLog trxHLog;

    /**
     * @param conf
     * @throws IOException
     */
    public TransactionalRegionServer(final Configuration conf) throws IOException, InterruptedException {
        super(conf);
        sn = super.getServerName();
        LOG.debug("Trafodion Recovery: Get Servername" + sn.getHostname() + " port " + sn.getPort());
        this.getRpcMetrics().createMetrics(new Class< ? >[] {
            TransactionalRegionInterface.class
        });
        cleanOldTransactionsThread = new CleanOldTransactionsChore(this);
        transactionLeases = new Leases(conf.getInt(LEASE_TIME, DEFAULT_LEASE_TIME), LEASE_CHECK_FREQUENCY);
        LOG.info("Transaction lease time: " + conf.getInt(LEASE_TIME, DEFAULT_LEASE_TIME));
    }

    protected THLog getTransactionLog() {
        return trxHLog;
    }

    /**
     * Make sure we add a listener for closing the transactional log when the regular WAL closes.
     */
    @Override
    protected List<WALActionsListener> getWALActionListeners() {
        List<WALActionsListener> listeners = super.getWALActionListeners();
        listeners.add(new WALActionsListener() {

            @Override
            public void logCloseRequested() {
                closeTransactionWAL();
            }

            @Override
            public void logRollRequested() {
                // don't care
            }

            @Override
            public void visitLogEntryBeforeWrite(final HRegionInfo info, final HLogKey logKey, final WALEdit logEdit) {
                // don't care
            }
            
            @Override
            public void preLogRoll(final Path oldPath, Path newPath) {
            	// don't care
            }
            
            @Override
            public void postLogRoll(final Path oldPath, Path newPath) {
            	// don't care
            }
            
            @Override
            public void postLogArchive(Path oldPath, Path newPath) {
            	//don't care
            }
            
            @Override
            public void visitLogEntryBeforeWrite(HTableDescriptor htd, HLogKey logKey, WALEdit logEdit) {
            	//don't care
            }
            
            @Override
            public void preLogArchive(Path oldPath, Path newPath) {
            	//don't care
            }     
            
        });
        return listeners;
    }

    @Override
    public long getProtocolVersion(final String protocol, final long clientVersion) throws IOException {
        if (protocol.equals(TransactionalRegionInterface.class.getName())) {
            return HRegionInterface.VERSION;
        }
        

        return super.getProtocolVersion(protocol, clientVersion);
    }

    @Override
    public ProtocolSignature getProtocolSignature(String protocol,
                                   long clientVersion,
                                   int clientMethodsHash) throws IOException {
       if (protocol.equals(TransactionalRegionInterface.class.getName())) {
          return new ProtocolSignature(HRegionInterface.VERSION, null);
       }
       return super.getProtocolSignature(protocol, clientVersion, clientMethodsHash);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void handleReportForDutyResponse(final MapWritable c) throws IOException {
        super.handleReportForDutyResponse(c);
        initializeTHLog();
        if (zkw1 == null) zkw1 = super.getZooKeeper();
        LOG.debug("Trafodion Recovery: Get ZooKeeper serverName " + sn.getHostname() + " port " + sn.getPort() + " zkw " + zkw1.toString());

        String n = Thread.currentThread().getName();
        UncaughtExceptionHandler handler = new UncaughtExceptionHandler() {

            public void uncaughtException(final Thread t, final Throwable e) {
                abort("Set stop flag in " + t.getName(), e);
                LOG.fatal("Set stop flag in " + t.getName(), e);
            }
        };
        
        Thread ChoreThread = new Thread(this.cleanOldTransactionsThread);
        Thread LeasesThread = new Thread(this.transactionLeases);
        Threads.setDaemonThreadRunning(ChoreThread, n + ".oldTransactionCleaner", handler);
        Threads.setDaemonThreadRunning(LeasesThread, "Transactional leases");
    }

    private void initializeTHLog() throws IOException {
        // We keep in the same directory as the core HLog.
    	
        Path oldLogDir = new Path(getRootDir(), HConstants.HREGION_OLDLOGDIR_NAME);       

        Path logdir = new Path(getRootDir(), HLog.getHLogDirectoryName(this.getServerName().getServerName()));

        trxHLog = THLog.createTHLog(getFileSystem(), logdir, oldLogDir, conf, null);        
    }

    @Override
    public void postOpenDeployTasks(final HRegion r, final CatalogTracker ct, final boolean daughter)
            throws KeeperException, IOException {
        if (r instanceof TransactionalRegion) {
            TransactionalRegion trxRegion = (TransactionalRegion) r;
            trxRegion.setTransactionLog(trxHLog);
            trxRegion.setTransactionalLeases(getTransactionalLeases());
        }
        super.postOpenDeployTasks(r, ct, daughter);
    }

    protected TransactionalRegion getTransactionalRegion(final byte[] regionName) throws NotServingRegionException {
        return (TransactionalRegion) super.getRegion(regionName);
    }

    protected Leases getTransactionalLeases() {
        return this.transactionLeases;
    }

    public static ZooKeeperWatcher getRecoveryzNode() {
         return zkw1;
    }

    public static ServerName getRecoveryRSName() {
         return sn;
    }

    public static void createRecoveryzNode(int node, String encodedName, byte [] data) throws IOException {

       synchronized(zkRecoveryCheckLock) {
         // default zNodePath for recovery
         String zNodeKey = sn.getHostname() + "," + sn.getPort() + "," + encodedName;

         StringBuilder sb = new StringBuilder();
         sb.append("TM");
         sb.append(node);
         String str = sb.toString();
         String zNodePathTM = zNodePath + str;
         String zNodePathTMKey = zNodePathTM + "/" + zNodeKey;
         LOG.trace("Trafodion Recovery: ZKW Post region recovery znode" + node + " zNode Path " + zNodePathTMKey);
          // create zookeeper recovery zNode, call ZK ...
         try {
                if (ZKUtil.checkExists(zkw1, zNodePathTM) == -1) {
                   // create parent nodename
                   LOG.debug("Trafodion Recovery: ZKW create parent zNodes " + zNodePathTM);
                   ZKUtil.createWithParents(zkw1, zNodePathTM);
                }
                ZKUtil.createAndFailSilent(zkw1, zNodePathTMKey, data);
          } catch (KeeperException e) {
          throw new IOException("Trafodion Recovery: ZKW Unable to create recovery zNode to TM, throw IOException " + node, e);
          }
       }
    }

    public static void deleteRecoveryzNode(int node, String encodedName) throws IOException {

       synchronized(zkRecoveryCheckLock) {
         // default zNodePath
         String zNodeKey = sn.getHostname() + "," + sn.getPort() + "," + encodedName;

         StringBuilder sb = new StringBuilder();
         sb.append("TM");
         sb.append(node);
         String str = sb.toString();
         String zNodePathTM = zNodePath + str;
         String zNodePathTMKey = zNodePathTM + "/" + zNodeKey;
         LOG.trace("Trafodion Recovery: ZKW Delete region recovery znode" + node + " zNode Path " + zNodePathTMKey);
          // delete zookeeper recovery zNode, call ZK ...
         try {
                ZKUtil.deleteNodeFailSilent(zkw1, zNodePathTMKey);
          } catch (KeeperException e) {
          throw new IOException("Trafodion Recovery: ZKW Unable to delete recovery zNode to TM " + node, e);
          }
       }
    }

    /**
     * We want to delay the close region for a bit if we have commit pending transactions.
     * 
     * @throws NotServingRegionException
     */
    @Override
    protected boolean closeRegion(final HRegionInfo region, final boolean abort, final boolean zk) {    	
        try {
            getTransactionalRegion(region.getRegionName()).prepareToClose();
        } catch (NotServingRegionException e) {
            LOG.warn("Failed to wait for uncommitted transactions to commit during region close.", e);
        }        
        return super.closeRegion(region, abort, zk);
    }

    /**
     * Close the transaction log.
     */
    private void closeTransactionWAL() {
        if (null != trxHLog) {
            try {
                trxHLog.close();
            } catch (Throwable e) {
                LOG.error("Close and delete of trx WAL failed", RemoteExceptionHandler.checkThrowable(e));
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void abortTransaction(final byte[] regionName, final long transactionId) throws IOException {
        checkOpen();
        super.getRequestCount().incrementAndGet();
        try {
            getTransactionalRegion(regionName).abort(transactionId);
        } catch (NotServingRegionException e) {
            LOG.info("Got not serving region during abort. Ignoring.");
        } catch (IOException e) {
            checkFileSystem();
            throw e;
        }
    }

    /**
     * new Recover request
     */
    //@Override
    public List<Long> recoveryRequest(final byte[] regionName, final int tmId) throws IOException {
        checkOpen();
        try {
            return getTransactionalRegion(regionName).doRecoveryRequest(tmId);
        } catch (IOException e) {
            checkFileSystem();
            throw e;
        }
    }


    /**
     * {@inheritDoc}
     */
    @Override
    public void commit(final byte[] regionName, final long transactionId) throws IOException {
        checkOpen();
        super.getRequestCount().incrementAndGet();
        try {
            getTransactionalRegion(regionName).commit(transactionId);
        } catch (IOException e) {
            checkFileSystem();
            throw e;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int commitRequest(final byte[] regionName, final long transactionId) throws IOException {
        checkOpen();
        super.getRequestCount().incrementAndGet();
        try {
            return getTransactionalRegion(regionName).commitRequest(transactionId);
        } catch (IOException e) {
            checkFileSystem();
            throw e;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean commitIfPossible(final byte[] regionName, final long transactionId) throws IOException {
        checkOpen();
        super.getRequestCount().incrementAndGet();
        try {
            return getTransactionalRegion(regionName).commitIfPossible(transactionId);
        } catch (IOException e) {
            checkFileSystem();
            throw e;
        }
    }

    /**
     * {@inheritDoc}
     */
    
    
    @Override
    public long openScanner(final long transactionId, final byte[] regionName, final Scan scan) throws IOException {   
        LOG.trace("openScanner -- ENTRY txId: " + transactionId);
    	checkOpen();
    	NullPointerException npe = null;        
        if (regionName == null) {
            npe = new NullPointerException("regionName is null");
          } else if (scan == null) {
            npe = new NullPointerException("scan is null");
          }
          if (npe != null) {
            throw new IOException("Invalid arguments to openScanner", npe);
          }
          
          super.getRequestCount().incrementAndGet();
          try {          
            TransactionalRegion r = getTransactionalRegion(regionName);
            //HRegion r = getRegion(regionName);
            r.checkRow(scan.getStartRow(), "Scan");            
            r.prepareScanner(scan);
            RegionScanner s = null;
               
            /* Disabling coprocessor work here, encountering error
            if (r.getCoprocessorHost() != null) {
              s = r.getCoprocessorHost().preScannerOpen(scan);
            }
            */

            if (s == null) {
              s = r.getScanner(transactionId, scan);
            }
            if (r.getCoprocessorHost() != null) {
              RegionScanner savedScanner = r.getCoprocessorHost().postScannerOpen(
                  scan, s);
              if (savedScanner == null) {
                LOG.warn("PostScannerOpen impl returning null. "
                    + "Check the RegionObserver implementation.");
              } else {
                s = savedScanner;
              }
            }
            LOG.trace("openScanner -- EXIT txId: " + transactionId);
            return addScanner(s);
          } catch (IOException e) {
            /*
        	LOG.error("Error opening scanner (fsOk: " + this.fsOk + ")", RemoteExceptionHandler.checkThrowable(t));
            checkFileSystem();
            */
            //throw(t);
            LOG.error("Error opening scanner (fsOk: " + this.fsOk + ")", RemoteExceptionHandler.checkIOException(e));
            checkFileSystem();
            throw e;
          } catch (Throwable t) {
            throw convertThrowableToIOE(cleanup(t, "Failed openScanner"));
          }    
    }

  private Throwable cleanup(final Throwable t, final String msg) {
    // Don't log as error if NSRE; NSRE is 'normal' operation.
    if (t instanceof NotServingRegionException) {
      LOG.debug("NotServingRegionException; " +  t.getMessage());
      return t;
    }
    if (msg == null) {
      LOG.error("", RemoteExceptionHandler.checkThrowable(t));
    } else {
      LOG.error(msg, RemoteExceptionHandler.checkThrowable(t));
    }
    if (!checkOOME(t)) {
      checkFileSystem();
    }
    return t;
  }
  private IOException convertThrowableToIOE(final Throwable t) {
    return convertThrowableToIOE(t, null);
  }

  /*
   * @param t
   *
   * @param msg Message to put in new IOE if passed <code>t</code> is not an IOE
   *
   * @return Make <code>t</code> an IOE if it isn't already.
   */
  private IOException convertThrowableToIOE(final Throwable t, final String msg) {
    return (t instanceof IOException ? (IOException) t : msg == null
        || msg.length() == 0 ? new IOException(t) : new IOException(msg, t));
  }


    /**
     * {@inheritDoc}
     */
    @Override
    public void beginTransaction(final long transactionId, final byte[] regionName) throws IOException {    	 
	LOG.debug("beginTransaction txId: " + transactionId + " regionName: " + new String(regionName));
    	getTransactionalRegion(regionName).beginTransaction(transactionId);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void delete(final long transactionId, final byte[] regionName, final Delete delete) throws IOException {

        SingleVersionDeleteNotSupported.validateDelete(delete);

        getTransactionalRegion(regionName).delete(transactionId, delete);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Result get(final long transactionId, final byte[] regionName, final Get get) throws IOException {
    	return getTransactionalRegion(regionName).get(transactionId, get);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void put(final long transactionId, final byte[] regionName, final Put put) throws IOException {
	LOG.trace("put, txid: " + transactionId);
    	getTransactionalRegion(regionName).put(transactionId, put);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int put(final long transactionId, final byte[] regionName, final Put[] puts) throws IOException {
	LOG.trace("puts[], txid: " + transactionId);
        getTransactionalRegion(regionName).put(transactionId, puts);
        return puts.length; // ??
    }

	@Override
	public int delete(long transactionId, byte[] regionName, final Delete[] deletes) throws NotServingRegionException, IOException {
		LOG.trace("Enter TransactionalRegionServer.deletes[], txid: " + transactionId);
        getTransactionalRegion(regionName).delete(transactionId, deletes);
        return deletes.length; // ??
	}

	@Override
	public boolean checkAndPut(long transactionId, byte[] regionName, byte[] row,
			byte[] family, byte[] qualifier, byte[] value, Put put)
			throws IOException {
		LOG.trace("Enter TransactionalRegionServer.checkAndPut, txid: " + transactionId);
		return getTransactionalRegion(regionName).checkAndPut(transactionId, row,
								      family, qualifier, value, put);
	}

	@Override
	public boolean checkAndDelete(long transactionId, byte[] regionName,
			byte[] row, byte[] family, byte[] qualifier, byte[] value,
			Delete delete) throws IOException {
		LOG.trace("Enter checkAndDelete.checkAndPut, txid: " + transactionId);
		return getTransactionalRegion(regionName).checkAndDelete(transactionId, row,
									 family, qualifier, value, delete);
	}
	
	@Override
	public boolean isMoveable(final byte[] regionName)
		      throws IOException {               
            LOG.trace("isMoveable -- ENTRY");
            try {
                LOG.trace("isMoveable -- EXIT");
                return getTransactionalRegion(regionName).isMoveable();
            } catch (IOException e) {            
                LOG.error("Unable to call isMoveable() on region: " + new String(regionName));
                throw e;
            }		
	}

    /**
    * Pending Transactions request
    */ 
    @Override
    public List<Long> getPendingTrans(byte [] regionName)
            throws IOException {
        return getTransactionalRegion(regionName).doPendingTransRequest();
    }

    /**
    * Committed Transactions request
    */ 
    @Override
    public List<Long> getCommittedTrans(byte [] regionName)
            throws IOException {
        return getTransactionalRegion(regionName).doCommittedTransRequest();
    }

    /**
    * In-doubt Transactions request
    */
    @Override
    public List<Long> getInDoubtTrans(byte [] regionName)
            throws IOException {
        return getTransactionalRegion(regionName).doInDoubtTransRequest();
    }
}
