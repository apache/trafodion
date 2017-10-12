/*
/* @@@ START COPYRIGHT @@@
/*
/*
Licensed to the Apache Software Foundation (ASF) under one
/*
or more contributor license agreements.  See the NOTICE file
/*
distributed with this work for additional information
/*
regarding copyright ownership.  The ASF licenses this file
/*
to you under the Apache License, Version 2.0 (the
/*
"License"); you may not use this file except in compliance
/*
with the License.  You may obtain a copy of the License at
/*
/*
  http://www.apache.org/licenses/LICENSE-2.0
/*
/*
Unless required by applicable law or agreed to in writing,
/*
software distributed under the License is distributed on an
/*
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
/*
KIND, either express or implied.  See the License for the
/*
specific language governing permissions and limitations
/*
under the License.
/*
/* @@@ END COPYRIGHT @@@
/*/

import org.junit.Test;
import static org.junit.Assert.*;

import java.sql.*;
import java.io.*;
import java.lang.*;

public class TestBatch 
{
	/* Test Batch insert and update*/
	@Test
	public void JDBCBatch1() throws InterruptedException, SQLException 
	{
		System.out.println("JDBC batch test 1..");

        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTBL1";
        int totalQA = 2000;
        int totalDev = 2000;
        int totalMgr = 2000;
        int totalPoints = 99999;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] updateCount = null;
        int TOTALUPDATECOUNT = 6000;
        boolean pass = true;
        String IN1 = "BATCHTBL1";
        String IN2 = null;
		try
		(
			Connection conn = Utils.getUserConnection();
			Statement stmt = conn.createStatement();
		)
		{
	        Utils.dropTable(conn, IN1);
	        Utils.createTable(conn, IN1);
	
	        insert = "insert into " + IN1 + " values(?,?,?,?)";
	        pStmt = conn.prepareStatement(insert);
	
	        startTime = System.currentTimeMillis();
	
	        int points = totalPoints;
	        int toggle = 1;
	        for (int j = 1; j <= totalQA; j++) {
	        	if (toggle == 1)
	        		DEPT = "JDBC";
	        	else if (toggle == 2)
	        		DEPT = "ODBC";
	        	else
	        		DEPT = "DBADMIN";
	
	        	pStmt.setString(1, "QA");
	        	pStmt.setInt(2, j);
	        	pStmt.setInt(3, points);
	        	pStmt.setString(4, DEPT);
	        	points--;
	        	toggle++;
	        	if (toggle > 3)
	        		toggle = 1;
	        	pStmt.addBatch();
	        }

	        points = totalPoints;
	    	toggle = 1;
	    	for (int j = (totalQA + 1); j <= (totalDev + totalQA); j++) {
	    		if (toggle == 1)
	    			DEPT = "JDBC";
	    		else if (toggle == 2)
	    			DEPT = "ODBC";
	    		else
	    			DEPT = "DBADMIN";
	    		pStmt.setString(1, "DEV");
	    		pStmt.setInt(2, j);
	    		pStmt.setInt(3, points);
	    		pStmt.setString(4, DEPT);
	    		points--;
	    		points--;
	    		toggle++;
	    		if (toggle > 3)
	    			toggle = 1;
	    		pStmt.addBatch();
	    	}

        	points = totalPoints;
        	for (int j = (totalQA + totalDev + 1); j <= (totalDev + totalQA + totalMgr); j++) {
        		if (toggle == 1)
        			DEPT = "JDBC";
        		else if (toggle == 2)
        			DEPT = "ODBC";
        		else
        			DEPT = "DBADMIN";
        		pStmt.setString(1, "MGR");
        		pStmt.setInt(2, j);
        		pStmt.setInt(3, points);
        		pStmt.setString(4, DEPT);
        		points--;
        		toggle++;
        		if (toggle > 3)
        			toggle = 1;
        		pStmt.addBatch();
        	}

        	try {
        		insertCount = pStmt.executeBatch();
        		for (int i = 0; i < insertCount.length; i++) {
        			if ((insertCount[i] != -2) && (insertCount[i] != 1)) {
        				pass = false;
        				System.out.println("Batch1 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("Batch1 : ERROR: Exception in insert batch mode.....");
        		fail("exception in test JDBCBatch1 in TestBatch -insert batch mode .." + bue.getMessage());
        		handleBatchUpdateException(bue);
        	}

        	java.sql.SQLWarning sw = pStmt.getWarnings();
        	int totalW = 0;
        	while (sw != null) {
        		System.out.println("SQLMessage : " + sw.getMessage());
          		System.out.println("SQLState : " + sw.getSQLState());
        		System.out.println("ErrorCode : " + sw.getErrorCode());
        		totalW++;
        		sw = sw.getNextWarning();
        	}
        	// Check the update count
        	int updateRowCount = pStmt.getUpdateCount();
        	if (updateRowCount != TOTALUPDATECOUNT) {
        		System.out.println("Batch1: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
        		pass = false;
        	}

        	// Verify the inserted rows
        	int totalI = 0;
        	rs = stmt.executeQuery("select * from " + IN1);
        	if (rs != null) {
        		ResultSetMetaData rsMD = rs.getMetaData();
        		while (rs.next()) {
        			totalI++;
        		}
        		rs.close();
        	}
        	if (totalI != TOTALUPDATECOUNT) {
        		pass = false;
        		System.out.println("Batch1 : ERROR: Expecting total of " + totalI + " rows inserted");
        	}
        	pStmt.close();
        	pStmt = null;
        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	fail("exception in test JDBCBatch1 in TestBatch .." + e.getMessage());
        	System.out.println(e.getMessage());
        } finally {
        	/**/
/* @@@ START COPYRIGHT @@@

        	Licensed to the Apache Software Foundation (ASF) under one
        	or more contributor license agreements.  See the NOTICE file
        	distributed with this work for additional information
        	regarding copyright ownership.  The ASF licenses this file
        	to you under the Apache License, Version 2.0 (the
        	"License"); you may not use this file except in compliance
        	with the License.  You may obtain a copy of the License at

        	  http://www.apache.org/licenses/LICENSE-2.0

        	Unless required by applicable law or agreed to in writing,
        	software distributed under the License is distributed on an
        	"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
        	KIND, either express or implied.  See the License for the
        	specific language governing permissions and limitations
        	under the License.

/* @@@ END COPYRIGHT @@@
        	/**/
        	assertEquals(true, pass);
        }
		System.out.println("JDBCBatch1 : Done");
	}

	/* Test Batch insert and update */
	@Test
	public void JDBCBatch2() throws InterruptedException, SQLException 
	{
		System.out.println("JDBC batch test 2..");

        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTBL2";
        int totalQA = 2000;
        int totalDev = 2000;
        int totalMgr = 2000;
        int totalPoints = 999999;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] updateCount = null;
        int TOTALUPDATECOUNT = totalQA+totalDev+totalMgr;
        boolean pass = true;
        String IN1 = "BATCHTBL2";
        String IN2 = null;
		try
		(
			Connection conn = Utils.getUserConnection();
			Statement stmt = conn.createStatement();
		)
		{
	        Utils.dropTable(conn, IN1);
	        Utils.createTable(conn, IN1);
	
	        insert = "insert into " + IN1 + " values(?,?,?,?)";
	        pStmt = conn.prepareStatement(insert);
	
	        startTime = System.currentTimeMillis();
	
	        int points = totalPoints;
	        int toggle = 1;
	        for (int j = 1; j <= totalQA; j++) {
	        	if (toggle == 1)
	        		DEPT = "JDBC";
	        	else if (toggle == 2)
	        		DEPT = "ODBC";
	        	else
	        		DEPT = "DBADMIN";
	
	        	pStmt.setString(1, "QA");
	        	pStmt.setInt(2, j);
	        	pStmt.setInt(3, points);
	        	pStmt.setString(4, DEPT);
	        	points--;
	        	toggle++;
	        	if (toggle > 3)
	        		toggle = 1;
	        	pStmt.addBatch();
	        }

	        points = totalPoints;
	    	toggle = 1;
	    	for (int j = (totalQA + 1); j <= (totalDev + totalQA); j++) {
	    		if (toggle == 1)
	    			DEPT = "JDBC";
	    		else if (toggle == 2)
	    			DEPT = "ODBC";
	    		else
	    			DEPT = "DBADMIN";
	    		pStmt.setString(1, "DEV");
	    		pStmt.setInt(2, j);
	    		pStmt.setInt(3, points);
	    		pStmt.setString(4, DEPT);
	    		points--;
	    		points--;
	    		toggle++;
	    		if (toggle > 3)
	    			toggle = 1;
	    		pStmt.addBatch();
	    	}

        	points = totalPoints;
        	for (int j = (totalQA + totalDev + 1); j <= (totalDev + totalQA + totalMgr); j++) {
        		if (toggle == 1)
        			DEPT = "JDBC";
        		else if (toggle == 2)
        			DEPT = "ODBC";
        		else
        			DEPT = "DBADMIN";
        		pStmt.setString(1, "MGR");
        		pStmt.setInt(2, j);
        		pStmt.setInt(3, points);
        		pStmt.setString(4, DEPT);
        		points--;
        		toggle++;
        		if (toggle > 3)
        			toggle = 1;
        		pStmt.addBatch();
        	}

        	try {
        		insertCount = pStmt.executeBatch();
        		for (int i = 0; i < insertCount.length; i++) {
        			if ((insertCount[i] != -2) && (insertCount[i] != 1))
        			{
        				pass = false;
        				System.out.println("Batch2 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("Batch2 : ERROR: Exception in insert batch mode.....");
        		fail("exception in test JDBCBatch2 in TestBatch -insert batch mode .." + bue.getMessage());
        		handleBatchUpdateException(bue);
        	}

        	java.sql.SQLWarning sw = pStmt.getWarnings();
        	int totalW = 0;
        	while (sw != null) {
        		System.out.println("SQLMessage : " + sw.getMessage());
        		System.out.println("SQLState : " + sw.getSQLState());
        		System.out.println("ErrorCode : " + sw.getErrorCode());
        		totalW++;
        		sw = sw.getNextWarning();
        	}
        	int updateRowCount = pStmt.getUpdateCount();
        	if (updateRowCount != TOTALUPDATECOUNT) {
        		System.out.println("Batch2: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
        		pass = false;
        	}

        	int totalI = 0;
        	rs = stmt.executeQuery("select * from " + IN1);
        	if (rs != null) {
        		ResultSetMetaData rsMD = rs.getMetaData();
        		while (rs.next()) {
        			totalI++;
        		}
        		rs.close();
        	}
        	if (totalI != TOTALUPDATECOUNT) {
        		pass = false;
        		System.out.println("Batch2 : ERROR: Expecting total of " + totalI + " rows inserted");
        	}
        	pStmt.close();
        	pStmt = null;

        	update = "update " + IN1 + " set points  = ? where TITLE = ? ";
        	pStmt = conn.prepareStatement(update);
        	pStmt.setInt(1, 9);
        	pStmt.setString(2, "MGR");
        	pStmt.addBatch();

        	try {
        		updateCount = pStmt.executeBatch();
        		for (int i = 0; i < updateCount.length; i++) {
        			if ((updateCount[i] != -2) && (updateCount[i] != 2)) {
        				pass = false;
        				System.out.println("Batch2 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + updateCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("Batch2 : ERROR: Exception in update batch mode.....");
        		fail("exception in test JDBCBatch2 in TestBatch -update batch mode .." + bue.getMessage());
        		handleBatchUpdateException(bue);
        	}

        	updateRowCount = pStmt.getUpdateCount();
        	if (updateRowCount != totalMgr) {
        		System.out.println("Batch2: ERROR: Expecting updateRowCount to be " + totalMgr);
        		pass = false;
        	}

        	int ret = 0;
        	rs = stmt.executeQuery("select POINTS from " + IN1 + " where TITLE = 'MGR'");
        	if (rs != null) {
        		while (rs.next()) {
        			ret = rs.getInt("POINTS");
        			if (ret != 9) {
        				pass = false;
        				System.out.println("Batch2: ERROR: points for MGR is not updated: " + ret);
        			}
        		}
        		rs.close();
        	}
        	pStmt.close();
        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	fail("exception in test JDBCBatch2 in TestBatch .." + e.getMessage());
        	System.out.println(e.getMessage());
        } finally {
        	if (pass)
        		System.out.println("JDBCBatch2 :  Passed");
        	else
        		System.out.println("JDBCBatch2 : FAILED");
        	assertEquals(true, pass);
        }
		System.out.println("JDBCBatch2 :  Done");
	}

	/* Test batch insert with all datatypes */
	@Test
	public void JDBCBatch3() throws InterruptedException, SQLException 
	{
		System.out.println("JDBC batch test 3..");

        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "TABALL";
        int total = 10000;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] updateCount = null;
        int TOTALUPDATECOUNT = total;
        boolean pass = true;
        String IN1 = "TABALL";
        String IN2 = null;
        int insertCountTemplate = -2;
        int[] updateCountTemplate = { -2 };
		try
		(
			Connection conn = Utils.getUserConnection();
			Statement stmt = conn.createStatement();
		)
		{
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	        stmt.executeUpdate("drop table if exists " + IN1 + " cascade");
           	
           	stmt.executeUpdate("create table " + IN1 + "(c1 LARGEINT NOT NULL, C2 LARGEINT SIGNED, C3 CHAR (100)," +
           			" C4 NUMERIC(18,4), C5 NUMERIC(6) SIGNED, C6 NUMERIC(128,30) UNSIGNED, C7 DECIMAL, C8 DECIMAL(18,4) SIGNED, C9 DECIMAL(9,5) UNSIGNED," +
           			" C10 INTEGER, C11 INTEGER SIGNED, c12 INTEGER UNSIGNED, C13 SMALLINT, C14 SMALLINT SIGNED, C15 SMALLINT UNSIGNED, C16 FLOAT(5), C17 REAL," +
           			" C18 DOUBLE PRECISION, C19 VARCHAR (1000), C20 DATE, C21 TIME, C22 TIMESTAMP, C23 INTERVAL YEAR TO MONTH, C24 INTERVAL YEAR, C25 INTERVAL MONTH," +
           			" C26 INTERVAL DAY, C27 INTERVAL YEAR TO MONTH, C28 INTERVAL DAY TO HOUR, C29 INTERVAL HOUR, C30 INTERVAL MINUTE, C31 INTERVAL SECOND," +
           			" C32 INTERVAL HOUR TO MINUTE, C33 INTERVAL HOUR TO SECOND, C34 INTERVAL MINUTE TO SECOND, PRIMARY KEY (C1)) SALT USING 6 PARTITIONS");

	        insert = "insert into " + IN1 + " values(?,-2**45,'abc',1234.5678,-1234.5678,123456789.123456789,12,-123,12.56,\n" 
	        			+ "123456,-123456,123456,123,-123,123,0.1234,123.0989,12345.67890,'abcdefghi',DATE '2001-03-22',TIME '13:40:30.666666',TIMESTAMP '1997-09-03 09:33:30.555555',\n" 
	        			+ "INTERVAL '2-7' YEAR TO MONTH,INTERVAL '96' YEAR,INTERVAL '9' MONTH,INTERVAL '9' DAY,INTERVAL '4-5' YEAR TO MONTH,\n"
	        			+ "INTERVAL '7 8' DAY TO HOUR,INTERVAL '12' HOUR,INTERVAL '30' MINUTE,INTERVAL '60' SECOND,INTERVAL '2:5' HOUR TO MINUTE,\n" 
	        			+ "INTERVAL '5:4:3' HOUR TO SECOND,INTERVAL '9:2' MINUTE TO SECOND)";
			
	        pStmt = conn.prepareStatement(insert);
	
	        startTime = System.currentTimeMillis();
	
        	try {
    	        for (int j = 1; j <= total; j++) {
    	        	pStmt.setInt(1, j);
    	        	pStmt.addBatch();
    	        }
        		insertCount = pStmt.executeBatch();
        		for (int i = 0; i < insertCount.length; i++) 
        		{
        			if ((insertCount[i] != -2) && (insertCount[i] != 1))
        			{
        				pass = false;
        				System.out.println("Batch3 : ERROR: Expecting return code for " + i + "th command: -2 or 1, but got : " + insertCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("Batch3 : ERROR: Exception in insert batch mode.....");
            	bue.printStackTrace();
            	fail("exception in test JDBCBatch3 in TestBatch -insert batch mode .." + bue.getMessage());
            	System.out.println(bue.getMessage());
        		handleBatchUpdateException(bue);
        	}

        	// Check the SQLWarning
        	java.sql.SQLWarning sw = pStmt.getWarnings();
        	int totalW = 0;
        	while (sw != null) {
        		System.out.println("SQLMessage : " + sw.getMessage());
        		System.out.println("SQLState : " + sw.getSQLState());
        		System.out.println("ErrorCode : " + sw.getErrorCode());
        		totalW++;
        		sw = sw.getNextWarning();
        	}
        	int updateRowCount = pStmt.getUpdateCount();
        	if (updateRowCount != TOTALUPDATECOUNT) {
        		System.out.println("Batch3: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT + " but got " + updateRowCount);
        		pass = false;
        	}

        	// Verify the inserted rows
        	int totalI = 0;
        	rs = stmt.executeQuery("select * from " + IN1);
        	if (rs != null) {
        		ResultSetMetaData rsMD = rs.getMetaData();
        		while (rs.next()) {
        			totalI++;
        		}
        		rs.close();
        	}
        	if (totalI != TOTALUPDATECOUNT) {
        		pass = false;
        		System.out.println("Batch3 : ERROR: Expecting total of " + totalI + " rows inserted");
        	}
        	pStmt.close();
        	pStmt = null;

        	stmt.close();
        	conn.close();

        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	fail("exception in test JDBCBatch3 in TestBatch .." + e.getMessage());
        	System.out.println(e.getMessage());
        } finally {
        	/**/
        	if (pass)
        		System.out.println("JDBCBatch3 :  Passed");
        	else
        		System.out.println("JDBCBatch3 : FAILED");
        	/**/
        	assertEquals(true, pass);
        }
		System.out.println("JDBCBatch3 : Done");
	}

	/* Test Batch Delete */
	@Test
	public void JDBCBatch4() throws InterruptedException, SQLException 
	{
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String delete = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTBL4";
        int totalQA = 100;
        int totalPoints = 99999;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] deleteCount = null;
        int[] updateCount = null;
        int TOTALUPDATECOUNT = totalQA;
        boolean pass = true;
        String IN1 = "BATCHTBL4";
        String IN2 = null;

		System.out.println("JDBC batch test 4..");

        try (
                Connection conn = Utils.getUserConnection();
                Statement stmt = conn.createStatement();
                )
        {
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	stmt.executeUpdate("drop table if exists " + ISTBL + " cascade");
	        stmt.executeUpdate("create table " + ISTBL + "(a int not null, b varchar(2000), c int, d varchar(400), primary key(a))");
	
	        insert = "insert into " + ISTBL + " values(?,?,?,?)";
	        pStmt = conn.prepareStatement(insert);
	
	        startTime = System.currentTimeMillis();
	
	        int points = totalPoints;
	        int toggle = 1;
	        for (int j = 1; j <= totalQA; j++) {
	        	if (toggle == 1)
	        		DEPT = "JDBC";
	        	else if (toggle == 2)
	        		DEPT = "ODBC";
	        	else
	        		DEPT = "DBADMIN";
	
	        	pStmt.setInt(1, j);
	        	pStmt.setString(2, "QA");
	        	pStmt.setInt(3, points);
	        	pStmt.setString(4, DEPT);
	        	points--;
	        	toggle++;
	        	if (toggle > 3)
	        		toggle = 1;
	        	pStmt.addBatch();
	        }

	        insertCount = pStmt.executeBatch();
    		for (int i = 0; i < insertCount.length; i++) {
    			if ((insertCount[i] != -2) && (insertCount[i] != 1)) {
    				pass = false;
    				System.out.println("JDBCBatch4 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
    			}
    		}

        	java.sql.SQLWarning sw = pStmt.getWarnings();
        	int totalW = 0;
        	while (sw != null) {
        		totalW++;
        		sw = sw.getNextWarning();
        	}
        	int updateRowCount = pStmt.getUpdateCount();
        	if (updateRowCount != TOTALUPDATECOUNT) {
        		System.out.println("JDBCBatch4: 1 : ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT + " but was " + updateRowCount);
        		pass = false;
        	}

        	//DELETE BATCH
	        //create table " + ISTBL + "(a int not null, b varchar(200000), c int, d varchar(40000), primary key(a))");
	        delete = "delete from " + ISTBL + " where d = ?";
	        pStmt = conn.prepareStatement(delete);
	        pStmt.setString(1, "DBADMIN");
	        pStmt.addBatch();
	        pStmt.setString(1, "ODBC");
	        pStmt.addBatch();

	        deleteCount = pStmt.executeBatch();
    		for (int i = 0; i < deleteCount.length; i++) {
    			if ((deleteCount[i] != -2) && (deleteCount[i] != 1)) {
    				pass = false;
    				System.out.println("JDBCBatch4 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + deleteCount[i]);
    			}
    		}

        	sw = pStmt.getWarnings();
        	totalW = 0;
        	while (sw != null) {
        		totalW++;
        		sw = sw.getNextWarning();
        	}
        	
        	int expDeletedRows = 66;
        	updateRowCount = pStmt.getUpdateCount();
        	if (updateRowCount != expDeletedRows) {
        		System.out.println("JDBCBatch4: 2 : ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT + " but was " + updateRowCount);
        		pass = false;
        	}

        	// Verify the deleted rows
        	int totalI = 0;
        	rs = stmt.executeQuery("select * from " + IN1);
        	int rowNo = 0;
    		while (rs.next()) {
    			rowNo++;
    			if (!rs.getString(4).equals("JDBC"))
    				System.out.println("JDBCBatch4 failed : Col d should have JDBC in row " + rowNo);
    		}
    		rs.close();
        	pStmt.close();
        	pStmt = null;
        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	fail("exception in test JDBCBatch4 in TestBatch .." + e.getMessage());
        	System.out.println(e.getMessage());
        } finally {
        	if (pass)
        		System.out.println("JDBC Batch4 :  Passed");
        	else
        		System.out.println("JDBC Batch4 : FAILED");
        	assertEquals(true, pass);
        }
	}

	/* Test BATCH UPSERT/UPDATE */
	@Test
	public void JDBCBatch5() throws InterruptedException, SQLException 
	{
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTBL5";
        int totalQA = 100;
        int totalPoints = 99999;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] updateCount = null;
        int TOTALUPDATECOUNT = 100;
        boolean pass = true;
        String IN1 = "BATCHTBL5";
        String IN2 = null;

		System.out.println("JDBC batch test 5..");

        try (
                Connection conn = Utils.getUserConnection();
                Statement stmt = conn.createStatement();
                )
        {
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	stmt.executeUpdate("drop table if exists " + ISTBL + " cascade");
	        stmt.executeUpdate("create table " + ISTBL + "(a int not null, b varchar(2000), c int, d varchar(400), primary key(a))");
	
	        insert = "upsert into " + ISTBL + " values(?,?,?,?)";
	        pStmt = conn.prepareStatement(insert);
	
	        startTime = System.currentTimeMillis();
	
	        int points = totalPoints;
	        int toggle = 1;
	        for (int j = 1; j <= totalQA; j++) {
	        	if (toggle == 1)
	        		DEPT = "JDBC";
	        	else if (toggle == 2)
	        		DEPT = "ODBC";
	        	else
	        		DEPT = "DBADMIN";
	
	        	pStmt.setInt(1, j);
	        	pStmt.setString(2, "QA");
	        	pStmt.setInt(3, points);
	        	pStmt.setString(4, DEPT);
	        	points--;
	        	toggle++;
	        	if (toggle > 3)
	        		toggle = 1;
	        	pStmt.addBatch();
	        }

	        insertCount = pStmt.executeBatch();
    		for (int i = 0; i < insertCount.length; i++) {
    			if ((insertCount[i] != -2) && (insertCount[i] != 1)) {
    				pass = false;
    				System.out.println("JDBCBatch5 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
    			}
    		}

        	java.sql.SQLWarning sw = pStmt.getWarnings();
        	int totalW = 0;
        	while (sw != null) {
        		totalW++;
        		sw = sw.getNextWarning();
        	}
        	int updateRowCount = pStmt.getUpdateCount();
        	if (updateRowCount != TOTALUPDATECOUNT) {
        		System.out.println("JDBCBatch5: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
        		pass = false;
        	}

        	//UPDATE BATCH
	        //create table " + ISTBL + "(a int not null, b varchar(200000), c int, d varchar(40000), primary key(a))");
	        update = "update " + ISTBL + " set b = ? where d = ?";
	        pStmt = conn.prepareStatement(update);
	        pStmt.setString(1, "Quality");
	        pStmt.setString(2, "JDBC");
	        pStmt.addBatch();
	        pStmt.setString(1, "Quality");
	        pStmt.setString(2, "ODBC");
	        pStmt.addBatch();
	        pStmt.setString(1, "Quality");
	        pStmt.setString(2, "DBADMIN");
	        pStmt.addBatch();

	        updateCount = pStmt.executeBatch();
    		for (int i = 0; i < updateCount.length; i++) {
    			if ((updateCount[i] != -2) && (updateCount[i] != 1)) {
    				pass = false;
    				System.out.println("JDBCBatch5 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + updateCount[i]);
    			}
    		}

        	sw = pStmt.getWarnings();
        	totalW = 0;
        	while (sw != null) {
        		totalW++;
        		sw = sw.getNextWarning();
        	}
        	updateRowCount = pStmt.getUpdateCount();
        	if (updateRowCount != TOTALUPDATECOUNT) {
        		System.out.println("JDBCBatch5: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
        		pass = false;
        	}

        	// Verify the updated rows
        	int totalI = 0;
        	rs = stmt.executeQuery("select * from " + IN1);
        	int rowNo = 0;
    		while (rs.next()) {
    			rowNo++;
    			if (!rs.getString(2).equals("Quality"))
    				System.out.println("JDBCBatch5 failed : Col d should have JDBC in row " + rowNo);
    		}
    		rs.close();
        	pStmt.close();
        	pStmt = null;
        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	fail("exception in test JDBCBatch5 in TestBatch .." + e.getMessage());
        	System.out.println(e.getMessage());
        } finally {
        	if (pass)
        		System.out.println("JDBC Batch5 :  Passed");
        	else
        		System.out.println("JDBC Batch5 : FAILED");
        	assertEquals(true, pass);
        }
	}

	/* Test Batch insert with all datatypes, using params for all datatypes */
	@Test
	public void JDBCBatch6() throws InterruptedException, SQLException 
	{
		System.out.println("JDBC batch test 6..");

        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "TABALL6";
        int total = 10000;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] updateCount = null;
        int TOTALUPDATECOUNT = total;
        boolean pass = true;
        String IN1 = "TABALL6";
        String IN2 = null;
        int insertCountTemplate = -2;
        int[] updateCountTemplate = { -2 };
		try
		(
			Connection conn = Utils.getUserConnection();
			Statement stmt = conn.createStatement();
		)
		{
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	        stmt.executeUpdate("drop table if exists " + IN1 + " cascade");
           	
           	stmt.executeUpdate("create table " + IN1 + "(c1 LARGEINT NOT NULL, C2 LARGEINT SIGNED, C3 CHAR (100)," +
           			" C4 NUMERIC(18,4), C5 NUMERIC(6) SIGNED, C6 NUMERIC(128,30) UNSIGNED, C7 DECIMAL, C8 DECIMAL(18,4) SIGNED, C9 DECIMAL(9,5) UNSIGNED," +
           			" C10 INTEGER, C11 INTEGER SIGNED, c12 INTEGER UNSIGNED, C13 SMALLINT, C14 SMALLINT SIGNED, C15 SMALLINT UNSIGNED, C16 FLOAT(5), C17 REAL," +
           			" C18 DOUBLE PRECISION, C19 VARCHAR (1000), C20 DATE, C21 TIME, C22 TIMESTAMP, C23 INTERVAL YEAR TO MONTH, C24 INTERVAL YEAR, C25 INTERVAL MONTH," +
           			" C26 INTERVAL DAY, C27 INTERVAL YEAR TO MONTH, C28 INTERVAL DAY TO HOUR, C29 INTERVAL HOUR, C30 INTERVAL MINUTE, C31 INTERVAL SECOND," +
           			" C32 INTERVAL HOUR TO MINUTE, C33 INTERVAL HOUR TO SECOND, C34 INTERVAL MINUTE TO SECOND, PRIMARY KEY (C1)) SALT USING 6 PARTITIONS");

	        insert = "insert into " + IN1 + " values(?,-2**45,'abc',1234.5678,-1234.5678,123456789.123456789,12,-123,12.56,\n" 
	        			+ "123456,-123456,123456,123,-123,123,0.1234,123.0989,12345.67890,'abcdefghi',DATE '2001-03-22',TIME '13:40:30.666666',TIMESTAMP '1997-09-03 09:33:30.555555',\n" 
	        			+ "INTERVAL '2-7' YEAR TO MONTH,INTERVAL '96' YEAR,INTERVAL '9' MONTH,INTERVAL '9' DAY,INTERVAL '4-5' YEAR TO MONTH,\n"
	        			+ "INTERVAL '7 8' DAY TO HOUR,INTERVAL '12' HOUR,INTERVAL '30' MINUTE,INTERVAL '60' SECOND,INTERVAL '2:5' HOUR TO MINUTE,\n" 
	        			+ "INTERVAL '5:4:3' HOUR TO SECOND,INTERVAL '9:2' MINUTE TO SECOND)";
			
	        pStmt = conn.prepareStatement(insert);
	
	        startTime = System.currentTimeMillis();
	
           	insert = "insert into " + IN1 + " values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
	        pStmt = conn.prepareStatement(insert);
	        startTime = System.currentTimeMillis();

	        try 
	        {
    	        for (int j = 1; j <= total; j++) 
    	        {
    	        	pStmt.setInt(1, j);
            	
	            	long ix = -2*45;
	            	pStmt.setLong(2, ix);
	            	
	            	pStmt.setString(3, "abc");
	            	
	            	float fx = 1234.5678f;
	            	pStmt.setFloat(4, fx); //NUMERIC(18,4)
	            	
	            	fx = -1234.5678f;
	            	pStmt.setFloat(5, fx); //NUMERIC(6) SIGNED
	            	pStmt.setDouble(6, 123456789.123456789); //NUMERIC(128,30) UNSIGNED
	            	pStmt.setInt(7, 12); //DECIMAL
	            	pStmt.setInt(8, -123); //DECIMAL(18,4) SIGNED
	            	
	            	fx = 12.56f;
	            	pStmt.setFloat(9, fx); //DECIMAL(9,5) UNSIGNED
	            	
	            	pStmt.setInt(10, 123456); //INTEGER
	            	pStmt.setInt(11, -123456); //INTEGER SIGNED
	            	pStmt.setInt(12, 123456); //INTEGER UNSIGNED
	            	pStmt.setInt(13, 123); //SMALLINT
	            	pStmt.setInt(14, -123); //SMALLINT SIGNED
	            	pStmt.setInt(15, 123); //SMALLINT UNSIGNED
	            	
	            	fx = 0.1234f;
	            	pStmt.setFloat(16, fx); //FLOAT(5)
	            	
	            	fx = 123.0989f;
	            	pStmt.setFloat(17, fx); //REAL
	            	
	            	pStmt.setDouble(18, 12345.67890); //DOUBLE PRECISION
	            	pStmt.setString(19, "abcdefghi"); //VARCHAR(1000)
	            	
	            	Date x = new Date(2000,12,8);
	            	pStmt.setDate(20, x); //DATE
	            	
	            	Time t = new Time(6, 30, 45);
	            	pStmt.setTime(21, t); //TIME
	            	
	            	Timestamp ts = new Timestamp(2000, 12, 8, 6, 30, 45, 10);
	            	pStmt.setTimestamp(22, ts); //TIMESTAMP
	            	pStmt.setString(23, "2-7"); //INTERVAL YEAR TO MONTH
	            	pStmt.setString(24, "96"); //INTERVAL YEAR
	            	pStmt.setString(25, "9"); //INTERVAL MONTH
	            	pStmt.setString(26, "9"); //INTERVAL DAY
	            	pStmt.setString(27, "4-5"); //INTERVAL YEAR TO MONTH
	            	pStmt.setString(28, "7 8"); //INTERVAL DAY TO HOUR
	            	pStmt.setString(29, "12"); //INTERVAL HOUR
	            	pStmt.setString(30, "30"); //INTERVAL MINUTE
	            	pStmt.setString(31, "60"); //INTERVAL SECOND
	            	pStmt.setString(32, "2:5"); //INTERVAL HOUR TO MINUTE
	            	pStmt.setString(33, "5:4:3"); //INTERVAL HOUR TO SECOND
	            	pStmt.setString(34, "9:2"); //INTERVAL MINUTE TO SECOND
		        	pStmt.addBatch();
		        }
        		insertCount = pStmt.executeBatch();
        		for (int i = 0; i < insertCount.length; i++) 
        		{
        			if ((insertCount[i] != -2) && (insertCount[i] != 1))
        			{
        				pass = false;
        				System.out.println("Batch6 : ERROR: Expecting return code for " + i + "th command: -2 or 1, but got : " + insertCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("Batch6 : ERROR: Exception in insert batch mode.....");
            	bue.printStackTrace();
            	fail("exception in test JDBCBatch6 in TestBatch - insert batch mode .." + bue.getMessage());
            	System.out.println(bue.getMessage());
        		handleBatchUpdateException(bue);
        	}

        	// Check the SQLWarning
        	java.sql.SQLWarning sw = pStmt.getWarnings();
        	int totalW = 0;
        	while (sw != null) {
        		System.out.println("SQLMessage : " + sw.getMessage());
        		System.out.println("SQLState : " + sw.getSQLState());
        		System.out.println("ErrorCode : " + sw.getErrorCode());
        		totalW++;
        		sw = sw.getNextWarning();
        	}
        	int updateRowCount = pStmt.getUpdateCount();
        	if (updateRowCount != TOTALUPDATECOUNT) {
        		System.out.println("Batch6: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT + " but got " + updateRowCount);
        		pass = false;
        	}

        	// Verify the inserted rows
        	int totalI = 0;
        	rs = stmt.executeQuery("select * from " + IN1);
        	if (rs != null) {
        		ResultSetMetaData rsMD = rs.getMetaData();
        		while (rs.next()) {
        			totalI++;
        		}
        		rs.close();
        	}
        	if (totalI != TOTALUPDATECOUNT) {
        		pass = false;
        		System.out.println("Batch6 : ERROR: Expecting total of " + totalI + " rows inserted");
        	}
        	pStmt.close();
        	pStmt = null;

        	stmt.close();
        	conn.close();

        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	fail("exception in test JDBCBatch6 in TestBatch .." + e.getMessage());
        	System.out.println(e.getMessage());
        } finally {
        	/**/
        	if (pass)
        		System.out.println("JDBCBatch6 :  Passed");
        	else
        		System.out.println("JDBCBatch6 : FAILED");
        	/**/
        	assertEquals(true, pass);
        }
		System.out.println("JDBCBatch6 : Done");
	}

	public static void handleBatchUpdateException(BatchUpdateException e) 
	{
		int[] bueUpdateCount = e.getUpdateCounts();

		Throwable cause = e.getCause();
		while (cause != null) 
			cause = cause.getCause();

		SQLException nextException = e.getNextException();

		while (nextException != null) 
		{
			cause = nextException.getCause();
			while (cause != null) 
				cause = cause.getCause();
			nextException = nextException.getNextException();
		}
	}
}
