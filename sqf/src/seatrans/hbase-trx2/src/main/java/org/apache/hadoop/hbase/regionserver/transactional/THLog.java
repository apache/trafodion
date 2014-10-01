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
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.lang.reflect.Constructor;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.LocalHBaseCluster;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionState.WriteAction;
import org.apache.hadoop.hbase.regionserver.wal.HLog;
import org.apache.hadoop.hbase.regionserver.wal.SequenceFileLogReader;
//Using custom SequenceFileLogWriter 
//import org.apache.hadoop.hbase.regionserver.wal.SequenceFileLogWriter;
import org.apache.hadoop.hbase.regionserver.wal.WALActionsListener;
import org.apache.hadoop.hbase.regionserver.wal.WALEdit;
import org.apache.hadoop.hbase.util.EnvironmentEdgeManager;

/**
 * Add support for transactional operations to the regionserver's write-ahead-log.
 */
public class THLog extends HLog {
    static final Log LOG = LogFactory.getLog(THLog.class);

    static final String THLOG_DATFILE = "thlog.dat.";

    boolean distributedMode = true;
    boolean doTlog = false;

    /** Name of old log file for reconstruction */
    static final String HREGION_OLD_THLOGFILE_NAME = "oldthlogfile.log";

    private THLog(final FileSystem fs, final Path dir, final Path oldLogDir, final Configuration conf,
            final List<WALActionsListener> listeners) throws IOException {

    	super(fs, dir, oldLogDir, conf, listeners, false, null
	//      , false // required for Hortonworks and MapR
	      );
        if (LocalHBaseCluster.isLocal(conf)) distributedMode = false;
        if (distributedMode) LOG.debug("Trafodion Recovery: cluster is distributed mode");
        else LOG.debug("Trafodion Recovery: cluster is local mode");

        this.doTlog = conf.getBoolean("hbase.regionserver.region.transactional.tlog", false);
        LOG.debug("Trafodion Recovery: TM TLOG setting " + this.doTlog);
    }

    // constructor used for Hortonworks and MapR
    // (compiles only because we put an edited HLog.java file into the classpath,
    // but this file will not be compiled into the packaged jar)
    private THLog(final FileSystem fs, final Path dir, final Path oldLogDir, final Configuration conf,
                 final List<WALActionsListener> listeners, boolean dummy) throws IOException {

    	super(fs, dir, oldLogDir, conf, listeners, false, null
	      , false // required for Hortonworks and MapR
	      );
    }

    public static THLog createTHLog(final FileSystem fs, final Path dir, final Path oldLogDir,
                                    final Configuration conf,
                                    final List<WALActionsListener> listeners) throws IOException {
        boolean dummyParam = false;
        Constructor c7 = null;

        // use reflection to find out whethe the relevant constructor
        // is the one with 7 or 8 parameters, this tests for the one with 7
        try {
            c7 = HLog.class.getConstructor(
                                           new Class [] {
                                               FileSystem.class,
                                               Path.class,
                                               Path.class,
                                               Configuration.class,
                                               List.class,
                                               Boolean.TYPE,
                                               String.class });
        } catch (NoSuchMethodException nsm) {
            c7 = null;
        }

        if (c7 != null)
            return new THLog(fs, dir, oldLogDir, conf, listeners);
        else
            return new THLog(fs, dir, oldLogDir, conf, listeners, dummyParam);
    }

    /**
     * Get a writer for the WAL.
     * 
     * @param path
     * @param conf
     * @return A WAL writer. Close when done with it.
     * @throws IOException
     */
    public static Writer createWriter(final FileSystem fs, final Path path, final Configuration conf) throws IOException {
        try {
            LOG.trace("createWriter -- ENTRY");
            HLog.Writer writer = new SequenceFileLogWriter(THLogKey.class);
            //conf.s
            writer.init(fs, path, conf);
            LOG.trace("createWriter -- EXIT");
            return writer;
        } catch (Exception e) {
            IOException ie = new IOException("cannot get log writer");
            ie.initCause(e);
            throw ie;
        }
    }

    @Override
    protected Writer createWriterInstance(final FileSystem fs, final Path path, final Configuration conf)
            throws IOException {
        return createWriter(fs, path, conf);
    }

    /**
     * This is a convenience method that computes a new filename with a given file-number.
     * 
     * @param fn
     * @return Path
     */
    @Override
    protected Path computeFilename() {
        // REVIEW : Use prefix ?
        return new Path(getDir(), THLOG_DATFILE + getFilenum());
    }

    /**
     * Get a reader for the WAL.
     * 
     * @param fs
     * @param path
     * @param conf
     * @return A WAL reader. Close when done with it.
     * @throws IOException
     */
    public static Reader getReader(final FileSystem fs, final Path path, final Configuration conf) throws IOException {
        try {
            LOG.trace("getReader -- ENTRY");
            HLog.Reader reader = new SequenceFileLogReader(THLogKey.class);
            reader.init(fs, path, conf);
            LOG.trace("getReader -- EXIT");
            return reader;
        } catch (Exception e) {
            IOException ie = new IOException("cannot get log reader");
            ie.initCause(e);
            throw ie;
        }
    }

    @Override
    protected THLogKey makeKey(final byte[] regionName, final byte[] tableName, final long seqnum, final long now, final UUID clusterId) {
    	return new THLogKey(regionName, tableName, seqnum, now, null, -1, clusterId);
    }

    /**
     * Write a transactional state to the log after we have decide that it can be committed. At this time we are still
     * waiting for the final vote (from other regions), so the commit may not be processed.
     */
    public void writeCommitRequestToLog(final HRegionInfo regionInfo, final TransactionState transactionState)
            throws IOException {
        this.appendCommitRequest(regionInfo, EnvironmentEdgeManager.currentTimeMillis(), transactionState);
    }

    /**
     * @param regionInfo
     * @param transactionId
     * @throws IOException
     */
    public void writeCommitToLog(final HRegionInfo regionInfo, final long transactionId, final HTableDescriptor htd) throws IOException {
        this.append(regionInfo, EnvironmentEdgeManager.currentTimeMillis(), THLogKey.TrxOp.COMMIT, transactionId, htd);
    }

    /**
     * @param regionInfo
     * @param transactionId
     * @throws IOException
     */
    public void writeAbortToLog(final HRegionInfo regionInfo, final long transactionId, final HTableDescriptor htd) throws IOException {
        this.append(regionInfo, EnvironmentEdgeManager.currentTimeMillis(), THLogKey.TrxOp.ABORT, transactionId, htd);
    }

    /**
     * Write a general transaction op to the log. This covers: start, commit, and abort.
     * 
     * @param regionInfo
     * @param now
     * @param txOp
     * @param transactionId
     * @throws IOException
     */
    private void append(final HRegionInfo regionInfo, final long now, final THLogKey.TrxOp txOp, final long transactionId, final HTableDescriptor htd)
            throws IOException {
        LOG.trace("append -- ENTRY txId: " + transactionId);
    	UUID id = HConstants.DEFAULT_CLUSTER_ID;
        THLogKey key = new THLogKey(regionInfo.getEncodedNameAsBytes(), regionInfo.getTableName(), -1, now, txOp,
                transactionId, id);
        WALEdit e = new WALEdit();
        e.add(new KeyValue(new byte[0], 0, 0)); // Empty KeyValue

        if (this.doTlog) {
           super.append(regionInfo, key, e, htd, false);
           LOG.trace("append -- EXIT txId: " + transactionId);
           return;
        }

        if (distributedMode) {
           super.append(regionInfo, key, e, htd, true);
           LOG.trace("append -- EXIT txId: " + transactionId);
           return;
        }

        super.append(regionInfo, key, e, htd, false);
        WALEdit e1 = new WALEdit();
        int extra = 4096;
        e1.add(new KeyValue(new byte[extra], 0, extra)); // Extra junk to force thlog.dat flush
        LOG.trace("append CommitAbort --EXTRA: " + transactionId + " with extra log write " + extra);
        THLogKey key1 = new THLogKey(regionInfo.getEncodedNameAsBytes(), regionInfo.getTableName(), -1, now,
                THLogKey.TrxOp.FORCED_FLUSH_FILLER, transactionId, id);
        super.append(regionInfo, key1, e1, htd, true);
        
        LOG.trace("append -- EXIT txId: " + transactionId);
        
    }

    /**
     * Write a transactional state to the log for a commit request.
     * 
     * @param regionInfo
     * @param update
     * @param transactionId
     * @throws IOException
     */
    private void appendCommitRequest(final HRegionInfo regionInfo, final long now,
            final TransactionState transactionState) throws IOException {
        int txnEditsLen = 0;
        LOG.trace("appendCommitRequest -- ENTRY txId: " + transactionState.getTransactionId());
    	UUID id = HConstants.DEFAULT_CLUSTER_ID;
        THLogKey key = new THLogKey(regionInfo.getEncodedNameAsBytes(), regionInfo.getTableName(), -1, now,
                THLogKey.TrxOp.COMMIT_REQUEST, transactionState.getTransactionId(), id);
 
        // move this with wo creation, so drop the prepare time (creation of edits blocks others to prepare due to serialization of commitRequest)
        /*WALEdit e = new WALEdit();
        for (WriteAction write : transactionState.getWriteOrdering()) {
            for (KeyValue value : write.getKeyValues()) {
                txnEditsLen = txnEditsLen + value.getLength();
                e.add(value);
            }
        }
       
        //LOG.debug("GFF 3 " + key.getTablename() + " " + key.getEncodedRegionName());
        LOG.debug("appendCommitRequest -- EXIT txId: " + transactionState.getTransactionId() + " with approx length "
                            + transactionState.getTransactionEditsLen() + " and in THLOG " + txnEditsLen);
        */
        if(distributedMode) {
              long txid = super.append(regionInfo, key, transactionState.getEdit(), transactionState.getTableDesc(), true);
              LOG.trace("appendCommitRequest -- append from super EXIT txId: " + txid);
              return;
        }

        long txid = super.append(regionInfo, key, transactionState.getEdit(), transactionState.getTableDesc(), false);
        LOG.trace("appendCommitRequest -- append from super EXIT txId: " + txid);
        int extra = 4096 - transactionState.getTransactionEditsLen() % 4096;
        WALEdit e1 = new WALEdit();
        e1.add(new KeyValue(new byte[extra], 0, extra)); // Extra junk to force thlog.dat flush
        LOG.trace("appendCommitRequest --EXTRA: " + transactionState.getTransactionId() + " with extra log write "
                            + extra);
        THLogKey key1 = new THLogKey(regionInfo.getEncodedNameAsBytes(), regionInfo.getTableName(), -1, now,
                THLogKey.TrxOp.FORCED_FLUSH_FILLER, transactionState.getTransactionId(), id);
        txid = super.append(regionInfo, key1, e1, transactionState.getTableDesc(), true);

    }

    private List<KeyValue> convertToKeyValues(final Delete delete) {
        LOG.trace("convertToKeyValues -- ENTRY");
        List<KeyValue> edits = new ArrayList<KeyValue>();

        for (List<KeyValue> kvs : delete.getFamilyMap().values()) {
            for (KeyValue kv : kvs) {
                edits.add(kv);
            }
        }
        LOG.trace("convertToKeyValues -- EXIT");
        return edits;
    }
}
