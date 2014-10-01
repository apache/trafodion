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
import java.util.Map;

import junit.framework.Assert;

import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.HBaseTestingUtility;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.transactional.TransactionLogger;
import org.apache.hadoop.hbase.regionserver.wal.WALEdit;
import org.apache.hadoop.hbase.test.HBaseTrxTestUtil;
import org.apache.hadoop.hbase.util.Bytes;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

/** JUnit test case for HLog */
public class TestTHLog {

    private final static HBaseTestingUtility TEST_UTIL = new HBaseTestingUtility();

    private Path dir;
    private Path oldDir;
    private FileSystem fs;

    final byte[] tableName = Bytes.toBytes("tablename");
    final HTableDescriptor tableDesc = new HTableDescriptor(tableName);    
    final HRegionInfo regionInfo = new HRegionInfo(tableName, HConstants.EMPTY_START_ROW, HConstants.EMPTY_END_ROW);
    final byte[] row1 = Bytes.toBytes("row1");
    final byte[] val1 = Bytes.toBytes("val1");
    final byte[] row2 = Bytes.toBytes("row2");
    final byte[] val2 = Bytes.toBytes("val2");
    final byte[] row3 = Bytes.toBytes("row3");
    final byte[] val3 = Bytes.toBytes("val3");
    final byte[] family = Bytes.toBytes("family");
    final byte[] column = Bytes.toBytes("a");

    @BeforeClass
    public static void setUpClass() throws Exception {
        HBaseTrxTestUtil.configureForIndexingAndTransactions(TEST_UTIL.getConfiguration());
        TEST_UTIL.startMiniDFSCluster(3);
    }

    @Before
    public void setUp() throws Exception {
        fs = TEST_UTIL.getDFSCluster().getFileSystem();
        this.dir = new Path("/trxLog");
        this.oldDir = new Path("/oldTrxLog");
        if (fs.exists(dir)) {
            fs.delete(dir, true);
        }
    }

    @After
    public void tearDown() throws Exception {
        if (this.fs.exists(this.dir)) {
            this.fs.delete(this.dir, true);
        }
    }

    @AfterClass
    public static void tearDownClass() throws Exception {
        TEST_UTIL.shutdownMiniDFSCluster();
    }

    @Test
    public void testSingleCommit() throws IOException {

        THLog log = THLog.createTHLog(fs, dir, oldDir, TEST_UTIL.getConfiguration(), null);

        // Write columns named 1, 2, 3, etc. and then values of single byte
        // 1, 2, 3...
        long transactionId = 1;
        TransactionState state = new TransactionState(transactionId, 0, regionInfo,tableDesc);
        state.addWrite(new Put(row1).add(family, column, val1));
        state.addWrite(new Put(row1).add(family, column, val2));
        state.addWrite(new Put(row1).add(family, column, val3));
        log.writeCommitRequestToLog(regionInfo, state);
        log.writeCommitToLog(regionInfo, transactionId, tableDesc);

        // log.completeCacheFlush(regionName, tableName, logSeqId);

        log.close();
        Path filename = log.computeFilename();
        // The "null" as first argument will probably not work, just making this compile for now...
        THLogRecoveryManager logRecoveryMangaer = new THLogRecoveryManager(null, fs, regionInfo, TEST_UTIL.getConfiguration());
        Map<Long, WALEdit> commits = logRecoveryMangaer.getCommitsFromLog(filename, -1, null);
        Assert.assertNull(commits);
    }

    @Test
    public void testSingleAbort() throws IOException {
        THLog log = THLog.createTHLog(fs, dir, oldDir, TEST_UTIL.getConfiguration(), null);

        // Write columns named 1, 2, 3, etc. and then values of single byte
        // 1, 2, 3...
        long transactionId = 1;
        TransactionState state = new TransactionState(transactionId, 0, regionInfo,tableDesc);
        state.addWrite(new Put(row1).add(family, column, val1));
        state.addWrite(new Put(row1).add(family, column, val2));
        state.addWrite(new Put(row1).add(family, column, val3));
        log.writeCommitRequestToLog(regionInfo, state);
        log.writeAbortToLog(regionInfo, transactionId, tableDesc);
        log.close();
        Path filename = log.computeFilename();
        // The "null" as first argument will probably not work, just making this compile for now...
        THLogRecoveryManager logRecoveryMangaer = new THLogRecoveryManager(null, fs, regionInfo, TEST_UTIL.getConfiguration());
        Map<Long, WALEdit> commits = logRecoveryMangaer.getCommitsFromLog(filename, -1, null);
        Assert.assertNull(commits);
    }

    @Test
    public void testWithPendingTransactionToCommit() throws IOException {
        THLog log = THLog.createTHLog(fs, dir, oldDir, TEST_UTIL.getConfiguration(), null);

        // Write columns named 1, 2, 3, etc. and then values of single byte
        // 1, 2, 3...
        long transactionId = 1;
        TransactionState state = new TransactionState(transactionId, 0, regionInfo,tableDesc);
        state.addWrite(new Put(row1).add(family, column, val1));
        state.addWrite(new Put(row1).add(family, column, val2));
        state.addWrite(new Put(row1).add(family, column, val3));
        log.writeCommitRequestToLog(regionInfo, state);
        log.close();
        Path filename = log.computeFilename();
        // The "null" as first argument will probably not work, just making this compile for now...
        THLogRecoveryManager logRecoveryMangaer = new THLogRecoveryManager(null, fs, regionInfo, TEST_UTIL.getConfiguration());
        logRecoveryMangaer.setGlobalTransactionLog(new TransactionLogger() {

            @Override
            public long createNewTransactionLog() {
                return 0;
            }
            
            @Override
            public long createNewTransactionLog(long transactionId) {
                return 0;
            }

            @Override
            public void forgetTransaction(final long transactionId) {}

            @Override
            public TransactionStatus getStatusForTransaction(final long transactionId) throws IOException {
                return TransactionStatus.COMMITTED;
            }

            @Override
            public void setStatusForTransaction(final long transactionId, final TransactionStatus status) {}

        });
        Map<Long, WALEdit> commits = logRecoveryMangaer.getCommitsFromLog(filename, -1, null);
        Assert.assertEquals(1, commits.size());
        Assert.assertEquals(3, commits.values().iterator().next().getKeyValues().size());
    }

    @Test
    public void testWithPendingTransactionToAbort() throws IOException {
        THLog log = THLog.createTHLog(fs, dir, oldDir, TEST_UTIL.getConfiguration(), null);

        // Write columns named 1, 2, 3, etc. and then values of single byte
        // 1, 2, 3...
        long transactionId = 1;
        TransactionState state = new TransactionState(transactionId, 0, regionInfo,tableDesc);
        state.addWrite(new Put(row1).add(family, column, val1));
        state.addWrite(new Put(row1).add(family, column, val2));
        state.addWrite(new Put(row1).add(family, column, val3));
        log.writeCommitRequestToLog(regionInfo, state);
        log.close();
        Path filename = log.computeFilename();
        // The "null" as first argument will probably not work, just making this compile for now...
        THLogRecoveryManager logRecoveryMangaer = new THLogRecoveryManager(null, fs, regionInfo, TEST_UTIL.getConfiguration());
        logRecoveryMangaer.setGlobalTransactionLog(new TransactionLogger() {

            @Override
            public long createNewTransactionLog() {
                return 0;
            }
            
            @Override
            public long createNewTransactionLog(long transactionId) {
                return 0;
            }

            @Override
            public void forgetTransaction(final long transactionId) {}

            @Override
            public TransactionStatus getStatusForTransaction(final long transactionId) throws IOException {
                return TransactionStatus.ABORTED;
            }

            @Override
            public void setStatusForTransaction(final long transactionId, final TransactionStatus status) {}
        });
        Map<Long, WALEdit> commits = logRecoveryMangaer.getCommitsFromLog(filename, -1, null);
        Assert.assertEquals(0, commits.size());
    }
}
