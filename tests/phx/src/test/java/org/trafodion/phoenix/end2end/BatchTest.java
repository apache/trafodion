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
    private int[] statusArray = null;

    private static final String strInsert = "INSERT INTO " + BATCH_TEST_TABLE + " VALUES (?,?)";
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
        statusArray = upsertStmt.executeBatch();
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
    public void testBatchInsertDuplicate() throws Exception {
        printTestDescription();

        createTestTable(BATCH_TEST_TABLE);

        // Initialize the expected status array for executeBatch() success for all rows,
        // except for row 2 and row 6, to which we will pass duplicate ID values on
        // purpose in actual insert later to make an unique value conflict error. The
        // returned status value should be
        String nameSuffix = "Traf The World ";
        expectedStatusArray = new int[10];
        
        int[] expectedIdArray = new int[8];
        String[] expectedNameArray = new String[8];
        int[] idArray = new int[10];
        String[] nameArray = new String[10];
        
        for(int i=0, j=0; i < 10; ++i) {
            expectedStatusArray[i] = -2;
            idArray[i] = i;
            nameArray[i] = nameSuffix + i;
            expectedIdArray[j] = i;
            expectedNameArray[j] = nameArray[i];
            switch(i) {
                case 1:
                case 5:
                    idArray[i] = i-1;
                    expectedStatusArray[i] = -3;
                    break;
                default:
                    j++;
                    break;
            }
        }
        int expectedRowCount = 8;

        // Start to prepare and execute the batch upsert
        PreparedStatement insertStmt = conn.prepareStatement(strInsert);
        for(int i=0; i < 10; ++i) {
            insertStmt.setInt(1, idArray[i]);
            insertStmt.setString(2, nameArray[i]);
            insertStmt.addBatch();
        }
        
        try {
            statusArray = insertStmt.executeBatch();
        } catch(SQLException sqle) {
            assertEquals("Batch update failed. See next exception for details", sqle.getMessage());
            SQLException e = null;
            e = sqle.getNextException();
            do {
                assertTrue(e.getMessage().contains("ERROR[8102] The operation is prevented by a unique constraint"));
            } while((e = e.getNextException()) != null);
        }
        
        // Commented out, because right now, the entire batch will
        // fail. Trafodion cannot detect duplicate rows in a batch
        // and process the remaining rows. See TRAFODION-1701.

        //assertArrayEquals(expectedStatusArray, statusArray);

        int rowCount = 0;
        ResultSet rs = conn.createStatement().executeQuery(strSelect);
        while(rs.next()) {
            // Same as above, see TRAFODION-1701.
            //System.out.println("ID = " + rs.getString(1) + ", Name = " + rs.getString(2));
            //assertEquals(expectedIdArray[rs.getRow()-1], rs.getInt(1));
            //assertEquals(expectedNameArray[rs.getRow()-1], rs.getString(2));
            rowCount++;
        }
        rs.close();
        insertStmt.close();
    }

    @Test
    public void testBatchUpsertDuplicate() throws Exception {
        printTestDescription();

        createTestTable(BATCH_TEST_TABLE);

        // Initialize the expected status array for executeBatch() success for all rows,
        // except for row 2 and row 6, to which we will pass duplicate ID values on
        // purpose in actual insert later to make an unique value conflict error. The
        // returned status value should be
        String nameSuffix = "Traf The World ";
        expectedStatusArray = new int[10];
        int[] expectedIdArray = new int[8];
        String[] expectedNameArray = new String[8];
        int[] idArray = new int[10];
        String[] nameArray = new String[10];
        
        for(int i=0, j=0; i < 10; ++i) {
            expectedStatusArray[i] = -2;
            idArray[i] = i;
            nameArray[i] = nameSuffix + i;
            expectedIdArray[j] = i;
            expectedNameArray[j] = nameArray[i];
            switch(i) {
                case 1:
                case 5:
                    idArray[i] = i-1;
                    expectedNameArray[j-1] = nameArray[i];
                    break;
                default:
                    j++;
                    break;
            }
        }
        int expectedRowCount = 8;

        // Start to prepare and execute the batch upsert
        PreparedStatement upsertStmt = conn.prepareStatement(strUpsert);
        for(int i=0; i < 10; ++i) {
            upsertStmt.setInt(1, idArray[i]);
            upsertStmt.setString(2, nameArray[i]);
            upsertStmt.addBatch();
        }
        
        statusArray = upsertStmt.executeBatch();
        
        assertArrayEquals(expectedStatusArray, statusArray);

        int rowCount = 0;
        ResultSet rs = conn.createStatement().executeQuery(strSelect);
        while(rs.next()) {
            //System.out.println("ID = " + rs.getString(1) + ", Name = " + rs.getString(2));
            assertEquals(expectedIdArray[rs.getRow()-1], rs.getInt(1));
            assertEquals(expectedNameArray[rs.getRow()-1], rs.getString(2));
            rowCount++;
        }
        assertEquals(rowCount, expectedRowCount);
        rs.close();
        upsertStmt.close();
    }

}
