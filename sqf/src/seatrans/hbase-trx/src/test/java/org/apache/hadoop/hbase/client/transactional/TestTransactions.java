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

import java.io.IOException;

import junit.framework.Assert;

import org.apache.hadoop.hbase.HBaseTestingUtility;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.regionserver.transactional.SingleVersionDeleteNotSupported;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionalRegionServer;
import org.apache.hadoop.hbase.test.HBaseTrxTestUtil;
import org.apache.hadoop.hbase.util.Bytes;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;

/**
 * Test the transaction functionality. This requires to run an {@link TransactionalRegionServer}.
 */
public class TestTransactions {

    private final static HBaseTestingUtility TEST_UTIL = new HBaseTestingUtility();

    private static final String TABLE_NAME = "table1";

    private static final byte[] FAMILY = Bytes.toBytes("family");
    private static final byte[] QUAL_A = Bytes.toBytes("a");
    private static final byte[] QUAL_B = Bytes.toBytes("b");

    private static final byte[] ROW1 = Bytes.toBytes("row1");
    private static final byte[] ROW2 = Bytes.toBytes("row2");
    private static final byte[] ROW3 = Bytes.toBytes("row3");

    private static HBaseAdmin admin;
    private static TransactionalTable table;
    private static TransactionManager transactionManager;

    /**
     * @throws java.lang.Exception
     */
    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
        HBaseTrxTestUtil.configureForIndexingAndTransactions(TEST_UTIL.getConfiguration());

        TEST_UTIL.startMiniCluster(3);
        setupTables();
    }

    /**
     * @throws java.lang.Exception
     */
    @AfterClass
    public static void tearDownAfterClass() throws Exception {
        TEST_UTIL.shutdownMiniCluster();
    }

    private static void setupTables() throws Exception {

        HTableDescriptor desc = new HTableDescriptor(TABLE_NAME);
        desc.addFamily(new HColumnDescriptor(FAMILY));
        admin = new HBaseAdmin(TEST_UTIL.getConfiguration());
        admin.createTable(desc);
        table = new TransactionalTable(TEST_UTIL.getConfiguration(), desc.getName());

        transactionManager = new TransactionManager(TEST_UTIL.getConfiguration());
    }

    @Before
    public void writeInitalRow() throws IOException {
        table.put(new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(1)));
    }

    @Test
    public void testSimpleTransaction() throws IOException, CommitUnsuccessfulException {
        TransactionState transactionState = makeTransaction1();
        transactionManager.tryCommit(transactionState);
        Result row1_A = table.get(new Get(ROW1).addColumn(FAMILY, QUAL_A));
        Result row2_A = table.get(new Get(ROW2).addColumn(FAMILY, QUAL_A));
        Result row3_A = table.get(new Get(ROW3).addColumn(FAMILY, QUAL_A));
        String expected = Bytes.toString(row1_A.getValue(FAMILY, QUAL_A));
        Assert.assertEquals(expected, Bytes.toString(row2_A.getValue(FAMILY, QUAL_A)));
        Assert.assertEquals(expected, Bytes.toString(row3_A.getValue(FAMILY, QUAL_A)));
    }

    @Test
    public void testTwoTransactionsWithoutConflict() throws IOException, CommitUnsuccessfulException {
        TransactionState transactionState1 = makeTransaction1();
        TransactionState transactionState2 = makeTransaction2();

        transactionManager.tryCommit(transactionState1);
        transactionManager.tryCommit(transactionState2);

        Result row1_A = table.get(new Get(ROW1).addColumn(FAMILY, QUAL_A));
        Assert.assertEquals(2, Bytes.toInt(row1_A.getValue(FAMILY, QUAL_A)));
    }

    // FIXME: We should write this test?
    @Ignore
    @Test
    public void testCompactionSplitOfTrxRegion() throws IOException, CommitUnsuccessfulException {
        Assert.fail("This test needs implementation.");
    }

    @Test
    public void testTwoGetPutTransactionsWithConflict() throws IOException, CommitUnsuccessfulException {
        TransactionState transactionState1 = makeTransaction1();
        TransactionState transactionState2 = makeTransaction2();

        transactionManager.tryCommit(transactionState2);

        try {
            transactionManager.tryCommit(transactionState1);
            Assert.fail();
        } catch (CommitUnsuccessfulException e) {
            // Good
        }
    }

    @Test
    public void testTwoTransactionsWithDeleteConflict() throws IOException, CommitUnsuccessfulException {
        TransactionState transactionState1 = makeTransaction1();
        TransactionState transactionState2 = makeTransaction3();

        transactionManager.tryCommit(transactionState2);

        try {
            transactionManager.tryCommit(transactionState1);
            Assert.fail();
        } catch (CommitUnsuccessfulException e) {
            // Good
        }
    }

    @Test
    public void testGetAfterPut() throws IOException, CommitUnsuccessfulException {
        TransactionState transactionState = transactionManager.beginTransaction();

        int originalValue = Bytes.toInt(table.get(transactionState, new Get(ROW1).addColumn(FAMILY, QUAL_A)).getValue(
            FAMILY, QUAL_A));
        int newValue = originalValue + 1;

        table.put(transactionState, new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(newValue)));

        Result row1_A = table.get(transactionState, new Get(ROW1).addColumn(FAMILY, QUAL_A));
        Assert.assertEquals(newValue, Bytes.toInt(row1_A.getValue(FAMILY, QUAL_A)));

        transactionManager.tryCommit(transactionState);

        row1_A = table.get(new Get(ROW1).addColumn(FAMILY, QUAL_A));
        Assert.assertEquals(newValue, Bytes.toInt(row1_A.getValue(FAMILY, QUAL_A)));
    }

    @Test
    public void testGetAfterDeleteColumnAllVersions() throws IOException, CommitUnsuccessfulException {
        Result row1_A = table.get(new Get(ROW1).addColumn(FAMILY, QUAL_A));
        Assert.assertEquals(1, Bytes.toInt(row1_A.getValue(FAMILY, QUAL_A)));

        TransactionState transactionState = transactionManager.beginTransaction();

        // Delete all versions of the column
        table.delete(transactionState, new Delete(ROW1).deleteColumns(FAMILY, QUAL_A));

        row1_A = table.get(transactionState, new Get(ROW1).addColumn(FAMILY, QUAL_A));
        Assert.assertEquals(null, row1_A.getValue(FAMILY, QUAL_A));

        transactionManager.tryCommit(transactionState);

        row1_A = table.get(new Get(ROW1).addColumn(FAMILY, QUAL_A));
        Assert.assertEquals(null, row1_A.getValue(FAMILY, QUAL_A));
    }

    @Test
    // FIXME: This test is failing because the TransactionState.applyDeletes() method does not adequately handle the
    // deleting of most recent version of a column. It assumes delete of all versions.
    public void testGetAfterDeleteColumnLatestVersion() throws IOException {
        Result row1_A = table.get(new Get(ROW1).addColumn(FAMILY, QUAL_A));
        int oldVersion = Bytes.toInt(row1_A.getValue(FAMILY, QUAL_A));

        table.put(new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(oldVersion + 1)));

        TransactionState transactionState = transactionManager.beginTransaction();

        // Delete most recent version
        try {
            table.delete(transactionState, new Delete(ROW1).deleteColumn(FAMILY, QUAL_A));
            Assert.fail();
        } catch (SingleVersionDeleteNotSupported e) {
            // expected
        }
    }

    @Test
    public void testGetAfterPutPut() throws IOException, CommitUnsuccessfulException {
        TransactionState transactionState = transactionManager.beginTransaction();

        int originalValue = Bytes.toInt(table.get(transactionState, new Get(ROW1).addColumn(FAMILY, QUAL_A)).getValue(
            FAMILY, QUAL_A));
        int newValue = originalValue + 1;

        table.put(transactionState, new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(newValue)));

        newValue = newValue + 1;

        table.put(transactionState, new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(newValue)));

        Result row1_A = table.get(transactionState, new Get(ROW1).addColumn(FAMILY, QUAL_A));
        Assert.assertEquals(newValue, Bytes.toInt(row1_A.getValue(FAMILY, QUAL_A)));

        transactionManager.tryCommit(transactionState);

        row1_A = table.get(new Get(ROW1).addColumn(FAMILY, QUAL_A));
        Assert.assertEquals(newValue, Bytes.toInt(row1_A.getValue(FAMILY, QUAL_A)));
    }

    @Test
    public void testPutDeleteCommitOrdering() throws IOException, CommitUnsuccessfulException {
        TransactionState transactionState = transactionManager.beginTransaction();

        int originalValue = Bytes.toInt(table.get(transactionState, new Get(ROW1).addColumn(FAMILY, QUAL_A)).getValue(
            FAMILY, QUAL_A));
        int newValue = originalValue + 1;

        // Delete column
        table.delete(transactionState, new Delete(ROW1).deleteColumns(FAMILY, QUAL_A));
        Result row1_A = table.get(transactionState, new Get(ROW1).addColumn(FAMILY, QUAL_A));
        Assert.assertEquals(null, row1_A.getValue(FAMILY, QUAL_A));

        // Put
        table.put(transactionState, new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(newValue)));
        row1_A = table.get(transactionState, new Get(ROW1).addColumn(FAMILY, QUAL_A));
        Assert.assertEquals(newValue, Bytes.toInt(row1_A.getValue(FAMILY, QUAL_A)));

        // Delete row
        table.delete(transactionState, new Delete(ROW1));
        row1_A = table.get(transactionState, new Get(ROW1).addColumn(FAMILY, QUAL_A));
        Assert.assertEquals(null, row1_A.getValue(FAMILY, QUAL_A));

        newValue++;

        // Put new value
        table.put(transactionState, new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(newValue)));
        row1_A = table.get(transactionState, new Get(ROW1).addColumn(FAMILY, QUAL_A));
        Assert.assertEquals(newValue, Bytes.toInt(row1_A.getValue(FAMILY, QUAL_A)));

        transactionManager.tryCommit(transactionState);

        // Verify most recent put stored
        row1_A = table.get(new Get(ROW1).addColumn(FAMILY, QUAL_A));
        Assert.assertEquals(newValue, Bytes.toInt(row1_A.getValue(FAMILY, QUAL_A)));
    }

    @Test
    public void testScanAfterUpdatePut() throws IOException {
        TransactionState transactionState = transactionManager.beginTransaction();

        int originalValue = Bytes.toInt(table.get(transactionState, new Get(ROW1).addColumn(FAMILY, QUAL_A)).getValue(
            FAMILY, QUAL_A));
        int newValue = originalValue + 1;

        table.put(transactionState, new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(newValue)));

        ResultScanner scanner = table.getScanner(transactionState, new Scan().addFamily(FAMILY));

        Result result;
        result = scanner.next();
        Assert.assertNotNull(result);
        Assert.assertEquals(Bytes.toString(ROW1), Bytes.toString(result.getRow()));
        Assert.assertEquals(newValue, Bytes.toInt(result.getValue(FAMILY, QUAL_A)));
    }

    @Test
    public void testScanAfterCellDelete() throws IOException, CommitUnsuccessfulException {

        // So that row doesn't get completely empty
        table.put(new Put(ROW1).add(FAMILY, QUAL_B, Bytes.toBytes(1)));

        TransactionState transactionState = transactionManager.beginTransaction();

        table.delete(transactionState, new Delete(ROW1).deleteColumns(FAMILY, QUAL_A));

        ResultScanner scanner = table.getScanner(transactionState, new Scan().addFamily(FAMILY));

        Result result;
        result = scanner.next();
        Assert.assertNotNull(result);
        Assert.assertNull(result.getValue(FAMILY, QUAL_A));

        transactionManager.tryCommit(transactionState);

        scanner = table.getScanner(new Scan().addFamily(FAMILY));
        result = scanner.next();
        Assert.assertNotNull(result);
        Assert.assertNull(result.getValue(FAMILY, QUAL_A));
    }

    @Test
    public void testScanAfterNewPutDeletePut() throws IOException {
        table.delete(new Delete(ROW3));

        table.put(new Put(ROW2).add(FAMILY, QUAL_A, Bytes.toBytes(1)));

        TransactionState transactionState = transactionManager.beginTransaction();

        int row2Value = 199;
        table.put(transactionState, new Put(ROW2).add(FAMILY, QUAL_A, Bytes.toBytes(row2Value)));
        table.delete(transactionState, new Delete(ROW2));

        ResultScanner scanner = table.getScanner(transactionState, new Scan().addFamily(FAMILY));

        Result result = scanner.next();
        Assert.assertNotNull(result);
        Assert.assertEquals(Bytes.toString(ROW1), Bytes.toString(result.getRow()));

        result = scanner.next();
        Assert.assertNull(result);

        table.put(transactionState, new Put(ROW2).add(FAMILY, QUAL_A, Bytes.toBytes(++row2Value)));

        scanner = table.getScanner(transactionState, new Scan().addFamily(FAMILY));

        result = scanner.next();
        Assert.assertNotNull(result);
        Assert.assertEquals(Bytes.toString(ROW1), Bytes.toString(result.getRow()));

        result = scanner.next();
        Assert.assertNotNull(result);
        Assert.assertEquals(Bytes.toString(ROW2), Bytes.toString(result.getRow()));
        Assert.assertEquals(row2Value, Bytes.toInt(result.getValue(FAMILY, QUAL_A)));
    }

    @Test
    public void testPutPutScan() throws IOException, CommitUnsuccessfulException {
        TransactionState transactionState = transactionManager.beginTransaction();

        Put put1 = new Put(ROW2).add(FAMILY, QUAL_A, Bytes.toBytes(199));
        Put put2 = new Put(ROW2).add(FAMILY, QUAL_A, Bytes.toBytes(299));
        table.put(transactionState, put1);
        table.put(transactionState, put2);

        ResultScanner scanner = table.getScanner(transactionState, new Scan().addFamily(FAMILY));

        Result result = scanner.next();
        Assert.assertNotNull(result);
        Assert.assertEquals(Bytes.toString(ROW1), Bytes.toString(result.getRow()));

        result = scanner.next();
        Assert.assertNotNull(result);
        Assert.assertEquals(Bytes.toString(ROW2), Bytes.toString(result.getRow()));
        Assert.assertEquals(299, Bytes.toInt(result.getValue(FAMILY, QUAL_A)));

        transactionManager.tryCommit(transactionState);

        scanner = table.getScanner(new Scan().addFamily(FAMILY));

        result = scanner.next();
        Assert.assertNotNull(result);
        Assert.assertEquals(Bytes.toString(ROW1), Bytes.toString(result.getRow()));

        result = scanner.next();
        Assert.assertNotNull(result);
        Assert.assertEquals(Bytes.toString(ROW2), Bytes.toString(result.getRow()));
        Assert.assertEquals(299, Bytes.toInt(result.getValue(FAMILY, QUAL_A)));
    }

    @Test
    public void testPutPutScanInSameMS() throws IOException, CommitUnsuccessfulException {
        TransactionState transactionState = transactionManager.beginTransaction();

        // Do this test many times to try and hit two puts in the same millisecond
        for (int i = 0; i < 100; i++) {
            testPutPutScan();
        }
    }

    // Read from ROW1,COL_A and put it in ROW2_COLA and ROW3_COLA
    private TransactionState makeTransaction1() throws IOException {
        TransactionState transactionState = transactionManager.beginTransaction();

        Result row1_A = table.get(transactionState, new Get(ROW1).addColumn(FAMILY, QUAL_A));

        table.put(transactionState, new Put(ROW2).add(FAMILY, QUAL_A, row1_A.getValue(FAMILY, QUAL_A)));
        table.put(transactionState, new Put(ROW3).add(FAMILY, QUAL_A, row1_A.getValue(FAMILY, QUAL_A)));

        return transactionState;
    }

    // Read ROW1,COL_A, increment its (integer) value, write back
    private TransactionState makeTransaction2() throws IOException {
        TransactionState transactionState = transactionManager.beginTransaction();

        Result row1_A = table.get(transactionState, new Get(ROW1).addColumn(FAMILY, QUAL_A));

        int value = Bytes.toInt(row1_A.getValue(FAMILY, QUAL_A));

        table.put(transactionState, new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(value + 1)));

        return transactionState;
    }

    // Read ROW1,COL_A and delete it
    private TransactionState makeTransaction3() throws IOException {
        TransactionState transactionState = transactionManager.beginTransaction();

        Result row1_A = table.get(transactionState, new Get(ROW1).addColumn(FAMILY, QUAL_A));

        int value = Bytes.toInt(row1_A.getValue(FAMILY, QUAL_A));

        table.delete(transactionState, new Delete(ROW1).deleteColumns(FAMILY, QUAL_A));

        return transactionState;
    }
}
