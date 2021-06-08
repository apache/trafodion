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

public class BatchTest extends JdbcCommon {
    
    public  static final int ROW_COUNT = 10000;
    private int[] expectedStatusArray = null;
    private int[] statusArray = null;

    private static final String strInsert = "INSERT INTO " + BATCH_TEST_TABLE + " VALUES (?,?)";
    private static final String strInsertFk = "INSERT INTO " + BATCH_TEST_TABLE_FK + " VALUES (?,?,?)";
    private static final String strUpsert = "UPSERT USING LOAD INTO " + BATCH_TEST_TABLE + " VALUES (?,?)";
    private static final String strUpdate = "UPDATE " + BATCH_TEST_TABLE + " SET (DEPT_ID,DEPT_NAME) = (?,?) WHERE DEPT_ID=?";
    private static final String strDelete = "DELETE FROM " + BATCH_TEST_TABLE;
    private static final String strSelect = "SELECT * FROM " + BATCH_TEST_TABLE + " ORDER BY DEPT_ID ASCENDING";
    private static final String strSelectFk = "SELECT * FROM " + BATCH_TEST_TABLE_FK + " ORDER BY EID ASCENDING";

    private void cleanTable() {
        try {
            Statement stmt = _conn.createStatement();
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
        System.out.println("Running testBatchInsertPositive......");

        createTestTable(BATCH_TEST_TABLE);

        // Initialize the expected status array for executeBatch() success
        expectedStatusArray = new int[ROW_COUNT];
        for(int i=0; i < ROW_COUNT; ++i) {
            expectedStatusArray[i] = -2;
        }

        // Start to prepare and execute the batch insert
        long startTime = System.currentTimeMillis();
        PreparedStatement upsertStmt = _conn.prepareStatement(strUpsert);
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
        PreparedStatement selectStmt = _conn.prepareStatement(strSelect);
        ResultSet rs = selectStmt.executeQuery();
        int rowCount = 0;
        while(rs.next()) {
            assertEquals(rowCount, rs.getInt(1));
            assertEquals("Traf The World " + String.valueOf(rowCount), rs.getString(2));
            rowCount++;
        }
        assertEquals(rowCount, ROW_COUNT);
    }

    /* Currently SQL does not have the ability to dump individual row, which
     * violates a contraint in a rowsets, and not able to return status error
     * for the specific illegal row. The whole batch will be failed. Thus, I
     * just comment out the expected error value in the expected status array
     * temporarily and does not assert the status array because there is no
     * value inserted into the table as the whole batch fails.
     * JIRA 1701 was filed for this issue 
     * */
    @Test
    public void testBatchInsertPkDuplicate() throws Exception {

        printTestDescription();
        System.out.println("Running testBatchInsertPkDuplicate......");

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
                    //expectedStatusArray[i] = -3;
                    break;
                default:
                    j++;
                    break;
            }
        }
        int expectedRowCount = 8;

        PreparedStatement insertStmt =null;
		ResultSet rs = null;
		try {
			// Start to and execute the batch upsert
	        insertStmt = _conn.prepareStatement(strInsert);
	        for(int i=0; i < 10; ++i) {
	            insertStmt.setInt(1, idArray[i]);
				insertStmt.setString(2, nameArray[i]);
	            insertStmt.addBatch();
	        }

			try {
				statusArray = insertStmt.executeBatch();
			} catch(SQLException sqle) {
				assertTrue(sqle.getMessage().toUpperCase().contains("BATCH UPDATE FAILED"));
				SQLException e = null;
				e = sqle.getNextException();
				do {
					assertTrue(e.getMessage().contains("ERROR[8102] The operation is prevented by a unique constraint"));
				} while((e = e.getNextException()) != null);
			}

			//assertArrayEquals(expectedStatusArray, statusArray);

			int rowCount = 0;
			rs = _conn.createStatement().executeQuery(strSelect);
			while(rs.next()) {
				System.out.println("ID = " + rs.getString(1) + ", Name = " + rs.getString(2));
				assertEquals(expectedIdArray[rs.getRow()-1], rs.getInt(1));
				assertEquals(expectedNameArray[rs.getRow()-1], rs.getString(2));
				rowCount++;
			}
			rs.close();
			insertStmt.close();
		 } finally {
            if (rs != null)
                rs.close();
            if (insertStmt != null)
                insertStmt.close();
        }
    }

    /* Currently SQL does not have the ability to dump individual row, which
     * violates a contraint in a rowsets, and not able to return status error
     * for the specific illegal row. The whole batch will be failed. Thus, I
     * just comment out the expected error value in the expected status array
     * temporarily and does not assert the status array because there is no
     * value inserted into the table as the whole batch fails.
     * JIRA 1716 was filed to describ this.
     * */
    @Test
    public void testBatchInsertFKNotExist() throws Exception {
        printTestDescription();
        System.out.println("Running testBatchInsertFKNotExist......");

        createTestTable(BATCH_TEST_TABLE);
        createTestTable(BATCH_TEST_TABLE_FK);

        String[] deptName = {"RD", "Marketing", "Finance"};
        String[] employeeName = {"John", "Michael", "Paul", "Ronan", "Kate"};
        int[] employeeDeptId = {1, 2, 2, 3, 5};

        expectedStatusArray = new int[]{-2, -2, -2, -2, -2};

        // Insert department records
        PreparedStatement pstmt = _conn.prepareStatement(strInsert);
        for(int i=0; i < deptName.length; ++i) {
            pstmt.setInt(1, (i+1));
            pstmt.setString(2, deptName[i]);
            pstmt.addBatch();
        }
        pstmt.executeBatch();
        pstmt.close();

        // Insert employee records, which need to reference id from department
        // as foreign key.
        pstmt = _conn.prepareStatement(strInsertFk);
        for(int i=0; i < employeeName.length; ++i) {
            pstmt.setInt(1, i);
            pstmt.setInt(2, employeeDeptId[i]);
            pstmt.setString(3, employeeName[i]);
            pstmt.addBatch();
        }
        try {
            statusArray = pstmt.executeBatch();
        } catch(SQLException sqle) {
            assertTrue(sqle.getMessage().toUpperCase().contains("BATCH UPDATE FAILED")); 
            System.out.println(sqle.getMessage());
            SQLException e = null;
            e = sqle.getNextException();
            do {
                assertTrue(e.getMessage().contains("operation is prevented by referential integrity constraint"));
                System.out.println(e.getMessage());
                break;
            } while((e = e.getNextException()) != null);
        }
        pstmt.close();

        ResultSet rs = _conn.createStatement().executeQuery(strSelectFk);
        int rowCount = 0;
        while(rs.next()) {
            if(rowCount == 4)
                break;
            assertEquals(rowCount, rs.getInt(1));
            assertEquals(employeeDeptId[rowCount], rs.getInt(2));
            assertEquals(employeeName[rowCount], rs.getString(3));
            rowCount++;
        }
        rs.close();
    }

    /* Normally, when there's invalid value in an individual row of a batch,
     * the insertion of this batch shall be succeeded except the individual
     * row with invalid value(string length larger than the collumn size in
     * this case.), and an error code represent the specific failure will be
     * returned in the status array for this particular row.
     * However, SQL does not return any error code and information for this
     * case, it just returns SQL_SUCCEED.
     * JIRA 1717 was filed to describ this problem
     * */
    @Test
    public void testBatchInsertBufferOverflow() throws Exception {
        printTestDescription();
        System.out.println("Running testBatchInsertBufferOverflow......");

        createTestTable(BATCH_TEST_TABLE);

        // Initialize the expected status array for executeBatch() success for all rows,
        // except for row 3, to which we will pass a string buffer of which the length
        // of the varchar column is longer than the column max length.
        String nameSuffix = "Traf The World ";
        expectedStatusArray = new int[10];
        
        int[] expectedIdArray = new int[9];
        String[] expectedNameArray = new String[9];
        int[] idArray = new int[10];
        String[] nameArray = new String[10];
        
        for(int i=0, j=0; i < 10; ++i) {
            expectedStatusArray[i] = -2;
            idArray[i] = i;
            nameArray[i] = nameSuffix + i;
            expectedIdArray[j] = i;
            expectedNameArray[j] = nameArray[i];
            if(i != 2)
                j++;
        }
        int expectedRowCount = 9;

        // Start to prepare and execute the batch upsert
        PreparedStatement insertStmt = _conn.prepareStatement(strInsert);
        for(int i=0; i < 10; ++i) {
            insertStmt.setInt(1, idArray[i]);
            if(i != 2)
                insertStmt.setString(2, nameArray[i]);
            else
                insertStmt.setString(2, nameArray[i] + nameArray[i]
                        + nameArray[i] + nameArray[i] + nameArray[i]
                        + nameArray[i] + nameArray[i] + nameArray[i]
                        + nameArray[i] + nameArray[i] + nameArray[i] );
            insertStmt.addBatch();
        }
        
        try {
            statusArray = insertStmt.executeBatch();
        } catch(SQLException sqle) {
            assertTrue(sqle.getMessage().toUpperCase().contains("BATCH UPDATE FAILED")); 
            System.out.println(sqle.getMessage());
            SQLException e = null;
            e = sqle.getNextException();
            do {
                assertTrue(e.getMessage().contains("VARCHAR data longer than column length"));
                System.out.println(e.getMessage());
            } while((e = e.getNextException()) != null);
        }
        
        //assertArrayEquals(expectedStatusArray, statusArray);

        int rowCount = 0;
        ResultSet rs = _conn.createStatement().executeQuery(strSelect);
        while(rs.next()) {
            assertEquals(expectedIdArray[rs.getRow()-1], rs.getInt(1));
            assertEquals(expectedNameArray[rs.getRow()-1], rs.getString(2));
            rowCount++;
        }
        rs.close();
        insertStmt.close();
    }

    @Test
    public void testBatchUpsertDuplicate() throws Exception {
        printTestDescription();
        System.out.println("Running testBatchUpsertDuplicate......");

        createTestTable(BATCH_TEST_TABLE);

        // Initialize the expected status array for executeBatch() success for all rows,
        // except for row 2 and row 6, to which we will pass duplicate ID values on
        // purpose in actual insert later to make an unique value conflict error.
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
        PreparedStatement upsertStmt = _conn.prepareStatement(strUpsert);
        for(int i=0; i < 10; ++i) {
            upsertStmt.setInt(1, idArray[i]);
            upsertStmt.setString(2, nameArray[i]);
            upsertStmt.addBatch();
        }
        
        statusArray = upsertStmt.executeBatch();
        
        assertArrayEquals(expectedStatusArray, statusArray);

        int rowCount = 0;
        ResultSet rs = _conn.createStatement().executeQuery(strSelect);
        while(rs.next()) {
            assertEquals(expectedIdArray[rs.getRow()-1], rs.getInt(1));
            assertEquals(expectedNameArray[rs.getRow()-1], rs.getString(2));
            rowCount++;
        }
        assertEquals(rowCount, expectedRowCount);
        rs.close();
        upsertStmt.close();
    }

    @Test
    public void testBatchUpdateOverflow() throws Exception {
        printTestDescription();
        System.out.println("Running testBatchUpdateOverflow......");

        createTestTable(BATCH_TEST_TABLE);

        String nameSuffix = "Traf The World ";
        String uptNameSuffix = "Traf The Galaxy ";
        expectedStatusArray = new int[10];
        
        int[] idArray = new int[10];
        String[] nameArray = new String[10];
        String[] updateNameArray = new String[5];
        String[] expectedNameArray = new String[10];
        
        // Initialize the expected status array for executeBatch() success for all rows,
        // except for 2nd row in update batch, to which we will pass a string buffer of 
        // which the length of the varchar column is longer than the column max length.
        for(int i=0, j=0; i < idArray.length; ++i) {
            idArray[i] = i;
            nameArray[i] = nameSuffix + i;
            expectedNameArray[i] = nameArray[i];
            expectedStatusArray[i] = -2;
         
            if(i > 4) {
                updateNameArray[j] = uptNameSuffix + j;
                expectedNameArray[i] = updateNameArray[j];
                
                if(i == 6) {
                    while(updateNameArray[j].length() < 128) {
                        updateNameArray[j] += uptNameSuffix;
                    }
                    //expectedStatusArray[i] = -3;
                    expectedNameArray[i] = nameArray[i];
                }
                j++;
            }
        }

        // Insert original records
        PreparedStatement pstmt = _conn.prepareStatement(strInsert);
        for(int i=0; i < idArray.length; ++i) {
            pstmt.setInt(1, i);
            pstmt.setString(2, nameArray[i]);
            pstmt.addBatch();
        }
        pstmt.executeBatch();
        pstmt.close();

        // Update existing records
        pstmt = _conn.prepareStatement(strUpdate);
        for(int i=0; i < updateNameArray.length; ++i) {
            pstmt.setInt(1, i+5);
            pstmt.setString(2, updateNameArray[i]);
            pstmt.setInt(3, i+5);
            pstmt.addBatch();
        }

        try {
            statusArray = pstmt.executeBatch();
        } catch(SQLException sqle) {
            assertTrue(sqle.getMessage().toUpperCase().contains("BATCH UPDATE FAILED")); 
            System.out.println(sqle.getMessage());
            SQLException e = null;
            e = sqle.getNextException();
            do {
                assertTrue(e.getMessage().contains("VARCHAR data longer than column length"));
                System.out.println(e.getMessage());
            } while((e = e.getNextException()) != null);
        }
        
        //assertArrayEquals(expectedStatusArray, statusArray);

        int rowCount = 0;
        ResultSet rs = _conn.createStatement().executeQuery(strSelect);
        while(rs.next()) {
            assertEquals(expectedNameArray[rs.getRow()-1], rs.getString(2));
            rowCount++;
        }
        rs.close();
        pstmt.close();
    }

    /* Currently SQL does not have the ability to dump individual row, which
     * violates a contraint in a rowsets, and not able to return status error
     * for the specific illegal row. The whole batch will be failed. Thus, I
     * just comment out the expected error value in the expected status array
     * temporarily and does not assert the status array because there is no
     * value inserted into the table as the whole batch fails.
     * JIRA 1701 was filed for this issue 
     * */
    @Test
    public void testBatchUpdatePkDuplicate() throws Exception {
        printTestDescription();
        System.out.println("Running testBatchUpdatePkDuplicate......");

        createTestTable(BATCH_TEST_TABLE);

        String nameSuffix = "Traf The World ";
        String uptNameSuffix = "Traf The Galaxy ";
        expectedStatusArray = new int[10];
        
        int[] idArray = new int[10];
        String[] nameArray = new String[10];
        int[] updateIdArray = new int[5];
        String[] updateNameArray = new String[5];
        int[] expectedIdArray = new int[10];
        String[] expectedNameArray = new String[10];
        
        // Initialize the expected status array for executeBatch() success for all rows,
        // except for 2nd row in update batch, to which we will pass a string buffer of 
        // which the length of the varchar column is longer than the column max length.
        for(int i=0, j=0; i < idArray.length; ++i) {
            idArray[i] = i;
            nameArray[i] = nameSuffix + i;
            expectedIdArray[i] = idArray[i];
            expectedNameArray[i] = nameArray[i];
            expectedStatusArray[i] = -2;
         
            if(i > 4) {
                updateIdArray[j] = j+15;
                updateNameArray[j] = uptNameSuffix + j;
                
                if(i != 6) {
                    expectedIdArray[i] = updateIdArray[j];
                    expectedNameArray[i] = updateNameArray[j];
                } else {
                    updateIdArray[j] = 4; // Use an existing ID in the records to make unique constraint violation.
                    expectedStatusArray[i] = -3;
                }
                
                j++;
            }
        }

        // Insert original records
        PreparedStatement pstmt = _conn.prepareStatement(strInsert);
        for(int i=0; i < idArray.length; ++i) {
            pstmt.setInt(1, i);
            pstmt.setString(2, nameArray[i]);
            pstmt.addBatch();
        }
        pstmt.executeBatch();
        pstmt.close();

        // Update existing records
        pstmt = _conn.prepareStatement(strUpdate);
        for(int i=0; i < updateNameArray.length; ++i) {
            pstmt.setInt(1, updateIdArray[i]);
            pstmt.setString(2, updateNameArray[i]);
            pstmt.setInt(3, i+5);
            pstmt.addBatch();
        }

        try {
            statusArray = pstmt.executeBatch();
        } catch(SQLException sqle) {
            assertTrue(sqle.getMessage().toUpperCase().contains("BATCH UPDATE FAILED")); 
            System.out.println(sqle.getMessage());
            SQLException e = null;
            e = sqle.getNextException();
            do {
                assertTrue(e.getMessage().contains("The operation is prevented by a unique constraint"));
                System.out.println(e.getMessage());
            } while((e = e.getNextException()) != null);
        }
        
        //assertArrayEquals(expectedStatusArray, statusArray);

        int rowCount = 0;
        ResultSet rs = _conn.createStatement().executeQuery(strSelect);
        while(rs.next()) {
            //assertEquals(expectedIdArray[rs.getRow()-1], rs.getInt(1));
            //assertEquals(expectedNameArray[rs.getRow()-1], rs.getString(2));
            rowCount++;
        }
        rs.close();
        pstmt.close();
    }
}
