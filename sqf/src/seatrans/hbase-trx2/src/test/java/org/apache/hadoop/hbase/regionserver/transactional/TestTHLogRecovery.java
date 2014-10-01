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
import java.io.PrintWriter;
import java.util.Collection;
import java.util.List;

import junit.framework.Assert;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseTestingUtility;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.transactional.CommitUnsuccessfulException;
import org.apache.hadoop.hbase.client.transactional.HBaseBackedTransactionLogger;
import org.apache.hadoop.hbase.client.transactional.TransactionManager;
import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.client.transactional.TransactionalTable;
import org.apache.hadoop.hbase.regionserver.HRegionServer;
import org.apache.hadoop.hbase.test.HBaseTrxTestUtil;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.EnvironmentEdgeManager;
import org.apache.hadoop.hbase.util.JVMClusterUtil;
import org.apache.hadoop.util.ReflectionUtils;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

public class TestTHLogRecovery {

    private static final Log LOG = LogFactory.getLog(TestTHLogRecovery.class);
    private static HBaseTestingUtility TEST_UTIL = new HBaseTestingUtility();

    private static final String TABLE_NAME = "table1";

    private static final byte[] FAMILY = Bytes.toBytes("family");
    private static final byte[] QUAL_A = Bytes.toBytes("a");

    private static final byte[] ROW1 = Bytes.toBytes("row1");
    private static final byte[] ROW2 = Bytes.toBytes("row2");
    private static final byte[] ROW3 = Bytes.toBytes("row3");
    private static final int TOTAL_VALUE = 10;

    private TransactionManager transactionManager;
    private TransactionalTable table;

    @BeforeClass
    public static void setUpClass() throws Exception {

        Configuration conf = TEST_UTIL.getConfiguration();
        HBaseTrxTestUtil.configureForIndexingAndTransactions(conf);

        // TEST_UTIL.getTestFileSystem().delete(new Path(conf.get(HConstants.HBASE_DIR)), true);

        // Set flush params so we don't get any
        // FIXME (defaults are probably fine)

        TEST_UTIL.startMiniCluster(3);

        HTableDescriptor desc = new HTableDescriptor(TABLE_NAME);
        desc.addFamily(new HColumnDescriptor(FAMILY));
        HBaseAdmin admin = TEST_UTIL.getHBaseAdmin();

        admin.createTable(desc);
        HBaseBackedTransactionLogger.createTable(conf);
    }

    @AfterClass
    public static void tearDownClass() throws Throwable {
        TEST_UTIL.shutdownMiniCluster();
    }

    @Before
    public void setUp() throws Exception {
        Configuration conf = TEST_UTIL.getConfiguration();

        table = new TransactionalTable(conf, TABLE_NAME);
        transactionManager = new TransactionManager(new HBaseBackedTransactionLogger(conf), conf);
        writeInitalRows();

        TEST_UTIL.getHBaseCluster().startRegionServer();
    }

    private void writeInitalRows() throws IOException {

        table.put(new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(TOTAL_VALUE)));
        table.put(new Put(ROW2).add(FAMILY, QUAL_A, Bytes.toBytes(0)));
        table.put(new Put(ROW3).add(FAMILY, QUAL_A, Bytes.toBytes(0)));

        flushRegionServer();
    }

    @Test
    public void testWithoutFlush() throws IOException, CommitUnsuccessfulException {
        TransactionState state1 = makeTransaction(false);
        transactionManager.tryCommit(state1);

        abortRegionServer();

        Thread t = startVerificationThread(1);
        t.start();

        threadDumpingJoin(t);
        verifyWrites(8, 1, 1);
    }

    @Test
    public void testWithFlushBeforeCommit() throws IOException, CommitUnsuccessfulException {
        TransactionState state1 = makeTransaction(true);
        flushRegionServer();
        transactionManager.tryCommit(state1);
        abortRegionServer();

        Thread t = startVerificationThread(1);
        t.start();
        threadDumpingJoin(t);
        verifyWrites(8, 1, 1);
    }

    @Test
    public void testWithFlushAbort() throws IOException, CommitUnsuccessfulException {
        TransactionState state1 = makeTransaction(true);
        flushRegionServer();
        abortRegionServer();

        Thread t = startVerificationThread(0);
        t.start();
        threadDumpingJoin(t);
        verifyWrites(TOTAL_VALUE, 0, 0);
    }

    @Test
    public void testNoFlushAbort() throws IOException {
        TransactionState state1 = makeTransaction(false);
        abortRegionServer();

        Thread t = startVerificationThread(0);
        t.start();
        threadDumpingJoin(t);
        verifyWrites(TOTAL_VALUE, 0, 0);
    }

    @Test
    public void testWithFlushBeforeCommitThenAnother() throws IOException, CommitUnsuccessfulException {
        TransactionState state1 = makeTransaction(true);
        flushRegionServer();
        transactionManager.tryCommit(state1);

        TransactionState state2 = makeTransaction(false);
        transactionManager.tryCommit(state2);

        abortRegionServer();

        Thread t = startVerificationThread(2);
        t.start();
        threadDumpingJoin(t);
        verifyWrites(6, 2, 2);
    }

    private void flushRegionServer() throws IOException {
        TEST_UTIL.flush(Bytes.toBytes(TABLE_NAME));
    }

    /**
     * Abort (hard) the region server serving TABLE_NAME.
     */
    private void abortRegionServer() {
        List<JVMClusterUtil.RegionServerThread> regionThreads = TEST_UTIL.getHBaseCluster().getRegionServerThreads();

        int server = -1;
        for (int i = 0; i < regionThreads.size(); i++) {
            HRegionServer s = regionThreads.get(i).getRegionServer();
            Collection<HRegionInfo> regions = null;
			try {
				regions = s.getOnlineRegions();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
            LOG.info("server: " + regionThreads.get(i).getName());
            for (HRegionInfo r : regions) {
                LOG.info("region: " + r.getRegionNameAsString());
                if (Bytes.equals(r.getTableDesc().getName(), Bytes.toBytes(TABLE_NAME))) {
                    server = i;
                }
            }
        }
        if (server == -1) {
            LOG.fatal("could not find region server serving table region");
            Assert.fail();
        }

        this.TEST_UTIL.getHBaseCluster().abortRegionServer(server);

        LOG.info(this.TEST_UTIL.getHBaseCluster().waitOnRegionServer(server) + " has been aborted");
    }

    private void verify(final int numRuns) throws IOException {
        // Reads
        int row1 = Bytes.toInt(table.get(new Get(ROW1).addColumn(FAMILY, QUAL_A)).getValue(FAMILY, QUAL_A));
        int row2 = Bytes.toInt(table.get(new Get(ROW2).addColumn(FAMILY, QUAL_A)).getValue(FAMILY, QUAL_A));
        int row3 = Bytes.toInt(table.get(new Get(ROW3).addColumn(FAMILY, QUAL_A)).getValue(FAMILY, QUAL_A));

        Assert.assertEquals(TOTAL_VALUE - 2 * numRuns, row1);
        Assert.assertEquals(numRuns, row2);
        Assert.assertEquals(numRuns, row3);
    }

    // Move 2 out of ROW1 and 1 into ROW2 and 1 into ROW3
    private TransactionState makeTransaction(final boolean flushMidWay) throws IOException {
        TransactionState transactionState = transactionManager.beginTransaction();

        // Reads
        int row1 = Bytes.toInt(table.get(transactionState, new Get(ROW1).addColumn(FAMILY, QUAL_A)).getValue(FAMILY,
            QUAL_A));
        int row2 = Bytes.toInt(table.get(transactionState, new Get(ROW2).addColumn(FAMILY, QUAL_A)).getValue(FAMILY,
            QUAL_A));
        int row3 = Bytes.toInt(table.get(transactionState, new Get(ROW3).addColumn(FAMILY, QUAL_A)).getValue(FAMILY,
            QUAL_A));

        row1 -= 2;
        row2 += 1;
        row3 += 1;

        if (flushMidWay) {
            flushRegionServer();
        }

        // Writes
        Put write = new Put(ROW1);
        write.add(FAMILY, QUAL_A, Bytes.toBytes(row1));
        table.put(transactionState, write);

        write = new Put(ROW2);
        write.add(FAMILY, QUAL_A, Bytes.toBytes(row2));
        table.put(transactionState, write);

        write = new Put(ROW3);
        write.add(FAMILY, QUAL_A, Bytes.toBytes(row3));
        table.put(transactionState, write);

        return transactionState;
    }

    private void verifyWrites(final int expectedRow1, final int expectedRow2, final int expectedRow3) throws IOException {
        Get get = new Get(ROW1).addColumn(FAMILY, QUAL_A);
        Result result = table.get(get);

        int row1 = Bytes.toInt(result.getValue(FAMILY, QUAL_A));
        int row2 = Bytes.toInt(table.get(new Get(ROW2).addColumn(FAMILY, QUAL_A)).getValue(FAMILY, QUAL_A));
        int row3 = Bytes.toInt(table.get(new Get(ROW3).addColumn(FAMILY, QUAL_A)).getValue(FAMILY, QUAL_A));
        Assert.assertEquals(expectedRow1, row1);
        Assert.assertEquals(expectedRow2, row2);
        Assert.assertEquals(expectedRow3, row3);
    }

    /*
     * Run verification in a thread so I can concurrently run a thread-dumper while we're waiting (because in this test
     * sometimes the meta scanner looks to be be stuck). @param tableName Name of table to find. @param row Row we
     * expect to find. @return Verification thread. Caller needs to calls start on it.
     */
    private Thread startVerificationThread(final int numRuns) {
        Runnable runnable = new Runnable() {

            public void run() {
                try {
                    // Now try to open a scanner on the meta table. Should stall until
                    // meta server comes back up.
                    HTable t = new HTable(TEST_UTIL.getConfiguration(), TABLE_NAME);
                    Scan s = new Scan();
                    s.addColumn(FAMILY, QUAL_A);
                    ResultScanner scanner = t.getScanner(s);
                    scanner.next();
                    scanner.next();
                    scanner.next();
                    scanner.close();

                } catch (IOException e) {
                    LOG.fatal("could not re-open meta table because", e);
                    Assert.fail();
                }

                try {
                    verify(numRuns);
                    LOG.info("Success!");
                } catch (Exception e) {
                    e.printStackTrace();
                    Assert.fail();
                }
            }
        };
        return new Thread(runnable, "TestTHLogRecovery verification thread");
    }

    private void threadDumpingJoin(final Thread t) {
        if (t == null) {
            return;
        }
        long startTime = EnvironmentEdgeManager.currentTimeMillis();
        while (t.isAlive()) {
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                LOG.info("Continuing...", e);
            }
            if (EnvironmentEdgeManager.currentTimeMillis() - startTime > 60000) {
                startTime = EnvironmentEdgeManager.currentTimeMillis();
                ReflectionUtils.printThreadInfo(new PrintWriter(System.out),
                    "Automatic Stack Trace every 60 seconds waiting on " + t.getName());
            }
        }
    }
}
