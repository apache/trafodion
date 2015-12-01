// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

package test.java.org.trafodion.phoenix.end2end;

import static org.junit.Assert.*;
import org.junit.*;

import java.io.FileInputStream;
import java.io.File;
import java.io.IOException;

import java.sql.DriverManager;
import java.sql.Connection;
import java.sql.Statement;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;

import java.util.Properties;
import java.util.Arrays;
import java.util.ArrayList;

public class BatchTest extends BaseTest {
    
    public  static final int ROW_COUNT = 10000;
    private int[] expectedStatusArray = null;

    private static final String strUpsert = "UPSERT USING LOAD INTO " + BATCH_TEST_TABLE + " VALUES (?,?)";
    private static final String strUpdate = "UPDATE " + BATCH_TEST_TABLE + "SET (ID, NAME) = (?,?)";
    private static final String strDelete = "DELETE FROM " + BATCH_TEST_TABLE;
    private static final String strSelect = "SELECT * FROM " + BATCH_TEST_TABLE;

    private void cleanTable() {
        try {
            Statement stmt = conn.createStatement();
            stmt.execute(strDelete);
        } catch(Exception e) {
            // do nothing
        }
    }

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table " + BATCH_TEST_TABLE));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    @Test
    public void testBatchInsertPositive() throws Exception {
        printTestDescription();

        createTestTable(BATCH_TEST_TABLE);

        // Initialize the expected status array for executeBatch() success
        expectedStatusArray = new int[ROW_COUNT];
        for(int i=0; i < ROW_COUNT; ++i) {
            expectedStatusArray[i] = -2;
        }

        // Start to prepare and execute the batch insert
        long startTime = System.currentTimeMillis();
        PreparedStatement upsertStmt = conn.prepareStatement(strUpsert);
        for(int i=0; i < ROW_COUNT; ++i) {
            upsertStmt.setInt(1, i);
            upsertStmt.setString(2, "Traf The World " + i);
            upsertStmt.addBatch();
        }
        int[] statusArray = upsertStmt.executeBatch();
        long endTime = System.currentTimeMillis();
        System.out.println("Time consumption for batch inserting "
                + ROW_COUNT + " rows is " + (endTime - startTime)  + " milli seconds");
        assertArrayEquals(expectedStatusArray, statusArray);

        // Fetch the data from the table to see if the data inserted succeeded
        PreparedStatement selectStmt = conn.prepareStatement(strSelect);
        ResultSet rs = selectStmt.executeQuery();
        int rowCount = 0;
        while(rs.next()) {
            assertEquals(rowCount, rs.getInt(1));
            assertEquals("Traf The World " + String.valueOf(rowCount), rs.getString(2));
            rowCount++;
        }
        assertEquals(rowCount, ROW_COUNT);
    }

    @Test
    public void testBatchInsertNegative() throws Exception {
        printTestDescription();

        createTestTable(BATCH_TEST_TABLE);

        // Initialize the expected status array for executeBatch() success for all rows,
        // except for row 2 and row 6, to which we will pass duplicate ID values on
        // purpose in actual insert later to make an unique value conflict error. The
        // returned status value should be 
        expectedStatusArray = new int[10];
        int[] expectedIdArray = new int[8];
        String[] expectedNameArray = new String[8];
        for(int i=0, j=0; i < 10; ++i, ++j) {
            expectedStatusArray[i] = -2;
            switch(i) {
                case 8:
                case 9:
                    break;
                case 1:
                case 4:
                    expectedIdArray[i] = ++j;
                    expectedNameArray[i] = "Traf The World " + j;
                    break;
                default:
                    expectedIdArray[i] = j;
                    expectedNameArray[i] = "Traf The World " + j;
                    break;
            }
        }
        int expectedRowCount = 8;

        // Start to prepare and execute the batch insert
        PreparedStatement upsertStmt = conn.prepareStatement(strUpsert);
        for(int i=0; i < 10; ++i) {
            switch(i) {
                case 1:
                    upsertStmt.setInt(1, 0);
                    upsertStmt.setString(2, "Traf The World " + 0);
                    break;
                case 5:
                    upsertStmt.setInt(1, 4);
                    upsertStmt.setString(2, "Traf The World " + 4);
                    break;
                default:
                    upsertStmt.setInt(1, i);
                    upsertStmt.setString(2, "Traf The World " + i);
                    break;
            }
            upsertStmt.addBatch();
        }
        
        int[] statusArray = upsertStmt.executeBatch();
        
        assertEquals(expectedStatusArray.length, statusArray.length);
        for(int i=0; i<10; ++i) {
            if((i != 1) && (i != 5)) {
                assertEquals(expectedStatusArray[i], statusArray[i]);
            }
            else {
                assertTrue(statusArray[i] != expectedStatusArray[i]);
            }
        }

        int rowCount = 0;
        ResultSet rs = conn.createStatement().executeQuery(strSelect);
        while(rs.next()) {
            assertEquals(expectedIdArray[rowCount], rs.getInt(1));
            assertEquals(expectedNameArray[rowCount], rs.getString(2));
            rowCount++;
        }
        assertEquals(rowCount, expectedRowCount);
    }

}
