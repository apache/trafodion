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

public class TestTrx 
{
	@Test
	public void JDBCTrx1() throws InterruptedException, SQLException
	{
		String tableName="qatabupd";
	    try
	    (
	     	Connection  connection = Utils.getUserConnection();
	        Statement stmt = connection.createStatement();            
	    )
	    {
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b char(20), c smallint, d integer, primary key(a))");
	        connection.setAutoCommit(false);
            stmt.executeUpdate("insert into " + tableName + " values(1, 'Moe', 100, 223), (2, 'Curly', 934, 444), (3, 'Larry', 1000, 999), (4, 'Moe', 12, 333), (5, 'Moe', 98, 987), (6, 'Moe', 54, 212)");
            connection.commit();
	        String updqry = "update " + tableName + " set b = ? where c > ?";              
	        long startTime = System.currentTimeMillis();
	        connection.setAutoCommit(false);
	        try 
	        (
	        	PreparedStatement pstmt = connection.prepareStatement(updqry);
	        )
	        {
		        pstmt.setString(1, "Moe");
		        pstmt.setInt(2, 890);
	         	assertEquals("Rows updated", 2, pstmt.executeUpdate());
	        }
	        catch (SQLException ex)
	        {
	            ex.printStackTrace();
				fail("Exception in pstmt in test JDBCTrx1 - Update.." + ex.getMessage());	        	
	        }
	        connection.commit();
	        connection.setAutoCommit(true);
	        long endTime = System.currentTimeMillis();
	        long elapsedTime = (endTime - startTime)/1000; //in seconds
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
			System.out.println("JDBC Trx1 - Update : Passed");
		} 
	    catch (Exception ex) 
	    {
            ex.printStackTrace();
			fail("Exception in test JDBC TestTrx1 - Update.." + ex.getMessage());
	 	}
		System.out.println("JDBC TestTrx1 - Update : Done");
	}
	
	@Test
	public void JDBCTrx2() throws InterruptedException, SQLException
	{
		String tableName="qatabups";
	    try
	    (
		    Connection  connection = Utils.getUserConnection();
		    Statement stmt = connection.createStatement();            
	    )
	    {
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        long startTime = System.currentTimeMillis();
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b char(20), c smallint, d integer, primary key(a))");
	        connection.setAutoCommit(false);
            stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 100, 223)");
            stmt.executeUpdate("upsert into " + tableName + " values(2, 'Curly', 34, 444)");
            stmt.executeUpdate("upsert into " + tableName + " values(3, 'Larry', 100, 999)");
            stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 12, 333)");
            stmt.executeUpdate("upsert into " + tableName + " values(2, 'Moe', 98, 987)");
            stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 54, 212)");
	        connection.commit();
	        connection.setAutoCommit(true);
            try
            (
            		ResultSet rs = stmt.executeQuery("select count(*) from " + tableName);
            )
            {
            	rs.next();
            	assertEquals("Upsert row count", 3, rs.getInt(1));
            }
            catch(SQLException ex)
            {
                ex.printStackTrace();
    			fail("Exception in resultset in test JDBC TestTrx2 - Update.." + ex.getMessage());
            }
         	//rs.close();
	        long endTime = System.currentTimeMillis();
	        long elapsedTime = (endTime - startTime)/1000; //in seconds
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
			System.out.println("JDBC TestTrx2 - SelectCount* : Passed");
		} 
	    catch (Exception ex) 
	    {
            ex.printStackTrace();
			fail("Exception in test JDBC TestTrx2 - SelectCount*.." + ex.getMessage());
	 	}
		System.out.println("JDBC TestTrx2 - SelectCount* : Done");
	}

	@Test
	public void JDBCTrx3() throws InterruptedException, SQLException 
	{
		String tableName="qatabmeta";
		try 
		(
		    Connection conn = Utils.getUserConnection();
		    Statement stmt = conn.createStatement();
		)
		{
    		stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
            stmt.executeUpdate("create table " + tableName + " (c1 char(20), c2 smallint, c3 integer)");
	        conn.setAutoCommit(false);
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 100, 223), ('Curly', 34, 444), ('Larry', 100, 999), ('Moe', 12, 333), ('Moe', 98, 987), ('Moe', 54, 212)");
	        conn.commit();
	        conn.setAutoCommit(true);
            try 
            (
            	ResultSet rs = stmt.executeQuery("select * from " + tableName);
            )
            {
	            ResultSetMetaData rsMD;
	         	rsMD = rs.getMetaData();
	         	assertEquals("No. of Columns ", 3, rsMD.getColumnCount());
	         	assertEquals("Column 1 column datatype", "CHAR", rsMD.getColumnTypeName(1));
	         	assertEquals("Column 2 column datatype", "SMALLINT", rsMD.getColumnTypeName(2));
	         	assertEquals("Column 3 column datatype", "INTEGER", rsMD.getColumnTypeName(3));
			
	         	assertEquals("Column 1 column name", "C1", rsMD.getColumnName(1));
	         	assertEquals("Column 2 column name", "C2", rsMD.getColumnName(2));
	         	assertEquals("Column 3 column name", "C3", rsMD.getColumnName(3));
			   
	         	rs.next();
	         	assertEquals("Column 1 data", "Moe", rs.getString("C1").trim());
	         	assertEquals("Column 2 data", 100, rs.getInt("C2"));
	         	assertEquals("Column 3 data", 223, rs.getInt("C3"));
			   
	         	rs.next();
	         	assertEquals("Column 1 data", "Curly", rs.getString(1).trim());
	         	assertEquals("Column 2 data", 34, rs.getInt(2));
	         	assertEquals("Column 3 data", 444, rs.getInt(3));
			}
            catch(SQLException ex)
            {
                ex.printStackTrace();
    			fail("Exception in resultset in test JDBCTrx3 - Metadata.." + ex.getMessage());
            }
		
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
			System.out.println("JDBCTrx3 - Metadata : Passed");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in test JDBCTrx3 - Metadata.." + ex.getMessage());
		}
		System.out.println("JDBCTrx3 - Metadata : Done");
    }

	@Test
	public void JDBCTrx4 () throws InterruptedException, SQLException
	{
		try
		(
			Connection  connection = Utils.getUserConnection();
			Statement stmt = connection.createStatement();            
		)
		{
			stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	        //int randomNumber = ( int )( Math.random() * 9999 );
		  	//String tableName="qatabdel" + randomNumber;
			String tableName="qatabdel";
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
			stmt.executeUpdate("create table " + tableName + "(a int not null, b char(20), c smallint, d integer, primary key(a))");
	        connection.setAutoCommit(false);
			stmt.executeUpdate("insert into " + tableName + " values (223, 'Moe', 100, 223), (444, 'Curly', 34, 444), (999, 'Larry', 100, 999), (333, 'Moe', 12, 333), (987, 'Moe', 98, 987), (212, 'Moe', 54, 212)");
			String updqry = "delete from " + tableName + " where a > ?";              
			long startTime = System.currentTimeMillis();
			PreparedStatement pstmt = connection.prepareStatement(updqry);
			pstmt.setInt(1,900);
			int rowsDeleted = pstmt.executeUpdate();
	        connection.commit();
	        connection.setAutoCommit(true);
			 
			assertEquals("Rows Deleted", 2, rowsDeleted);
			
			long endTime = System.currentTimeMillis();
			long elapsedTime = (endTime - startTime)/1000; //in seconds
			//System.out.println("Total time required to delete records using PreparedStatement using executeUpdate() is :" + elapsedTime + " sec(s)");
			//log.info("Total time required to delete records using PreparedStatement using executeUpdate() is :" + elapsedTime + " sec(s)");			      
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
			System.out.println("JDBCTrx4 - Delete : Passed");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in test JDBCTrx4 - Delete.." + ex.getMessage());
		}
		System.out.println("JDBCTrx4 - Delete : Done");
	}

	@Test
	public void JDBCTrx5 () throws SQLException, ClassNotFoundException 
	{
//		System.out.println("JDBCDelim..");
        //int randomNumber = ( int )( Math.random() * 9999 );
	  	//String tableName="qatabdelim" + randomNumber;
		String tableName="qatabdelim";

        try 
        (
        	Connection conn = Utils.getUserConnection();
        	Statement stmt = conn.createStatement();
        )
        {
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
       	   	stmt.executeUpdate("create table " + tableName + " (c0 int not null, \"value\" int, \"Value\" smallint, \"VALUE\" integer, primary key(c0))");
	        conn.setAutoCommit(false);
       	   	stmt.executeUpdate("insert into " + tableName + " values(1, 200, 300, 400)");
	        conn.commit();
	        conn.setAutoCommit(true);
       	   	try
       	   	(
       	   		ResultSet rs = stmt.executeQuery("select * from " + tableName);
       	   	)
       	   	{
	       	   	ResultSetMetaData rsMD = rs.getMetaData();
	       	   	assertEquals("No. of Columns ", 4, rsMD.getColumnCount());
	
	       	   	assertEquals("Column 1 column datatype", "INTEGER", rsMD.getColumnTypeName(1));
	       	   	assertEquals("Column 2 column datatype", "INTEGER", rsMD.getColumnTypeName(2));
	       	   	assertEquals("Column 3 column datatype", "SMALLINT", rsMD.getColumnTypeName(3));
	       	   	assertEquals("Column 4 column datatype", "INTEGER", rsMD.getColumnTypeName(4));
	
	       	   	assertEquals("Column 1 column name", "C0", rsMD.getColumnName(1));
	       	   	assertEquals("Column 2 column name", "value", rsMD.getColumnName(2));
	       	   	assertEquals("Column 3 column name", "Value", rsMD.getColumnName(3));
	       	   	assertEquals("Column 4 column name", "VALUE", rsMD.getColumnName(4));
	      	   
	       	   	rs.next();
	
	       	   	assertEquals("Column 1 data", 1, rs.getInt("C0"));
	       	   	//OUR JDBC DRIVER DOES NOT SUPPORT CASE SENSITIVE DELIMITED COLUMN NAMES IN GETDATA METHODS
	       	   	//assertEquals("Column 2 data", 200, rs.getInt("\"value\""));
	       	   	//AssertEquals("Column 3 data", 300, rs.getInt("\"Value\""));
	       	   	//AssertEquals("Column 4 data", 400, rs.getInt(\"VALUE\"));
	       	   
	       	   	assertEquals("Column 1 data", 1, rs.getInt(1));
	       	   	assertEquals("Column 2 data", 200, rs.getInt(2));
	       	   	assertEquals("Column 3 data", 300, rs.getInt(3));
	       	   	assertEquals("Column 4 data", 400, rs.getInt(4));
       	   	}
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
   	   		//conn.close();
   	   		System.out.println("JDBCTrx5 - Delim : Passed");
        } catch (Exception ex)
        {
        	ex.printStackTrace();
        	fail("Exception in test JDBC TestBasic5 - Delim.." + ex.getMessage());
        } 
	   		System.out.println("JDBCTrx5 - Delim : Done");
	}
	
	@Test
	public void JDBCTrx6() throws SQLException, ClassNotFoundException 
	{
//		System.out.println("JDBCInsert..");
	    int rowNo = 0, rowsDeleted = 0, rowsUpdated=0;
        //int randomNumber = ( int )( Math.random() * 9999 );
	  	//String tableName="qatabins" + randomNumber;
	    String tableName="qatabins";

		try 
		(
	        Connection conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();
		)
		{
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
            stmt.executeUpdate("create table " + tableName + " (c1 char(20), c2 smallint, c3 integer)");
	        conn.setAutoCommit(false);
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 100, 223)");
            stmt.executeUpdate("insert into " + tableName + " values('Curly', 34, 444)");
            stmt.executeUpdate("insert into " + tableName + " values('Larry', 100, 999)");
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 12, 333)");
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 98, 987)");
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 54, 212)");
	        conn.commit();
	        conn.setAutoCommit(true);
            try
            (
            	ResultSet rs = stmt.executeQuery("select * from " + tableName);
            )
            {
	         	rs.next();
	         	assertEquals("Column 1 data", "Moe", rs.getObject(1).toString().trim());
	         	Short exp = 100;
	         	assertEquals("Column 2 data", (Short)exp, (Short)rs.getShort(2));
	         	assertEquals("Column 3 data", (Integer)223, (Integer)rs.getObject(3));
			   
	         	rs.next();
	         	assertEquals("Column 1 data", "Curly", rs.getObject(1).toString().trim());
	         	exp = 34;
	         	assertEquals("Column 2 data", (Short)exp, (Short)rs.getShort(2));
	         	assertEquals("Column 3 data", (Integer)444, (Integer)rs.getObject(3));
            }
            rowsUpdated = stmt.executeUpdate("update " + tableName + " set c2 = 8787 where c3 > 300");
         	assertEquals("Rows Updated", 4, rowsUpdated);
	        rowsDeleted = stmt.executeUpdate("delete from " + tableName + " where c2 < 200");
         	assertEquals("Rows deleted", 2, rowsDeleted);
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
			System.out.println("JDBCTrx6 - Insert : Passed");
        } catch (Exception ex)
        {
            ex.printStackTrace();
            fail("Exception in test JDBCTrx6 - Insert.." + ex.getMessage());
        } 
		System.out.println("JDBCTrx6 - Insert : Done");
	}

	@Test
	public void JDBCTrxBatch1() throws InterruptedException, SQLException 
	{
		System.out.println("JDBC Trx batch test 1..");

        Statement stmt = null;
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTRX1";
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
        String IN1 = "BATCHTRX1";
        String IN2 = null;
		try
		(
			Connection conn = Utils.getUserConnection();
		)
		{
	        Utils.dropTable(conn, IN1);
	        Utils.createTable(conn, IN1);

	        conn.setAutoCommit(false);
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
        				System.out.println("Trx Batch1 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("Trx Batch1 : ERROR: Exception in insert batch mode.....");
        		fail("Exception in test JDBCTrxBatch1 - insert batch mode.." + bue.getMessage());
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
        		System.out.println("Trx Batch1: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
        		pass = false;
        	}
        	conn.commit();
        	conn.setAutoCommit(true);
        	// Verify the inserted rows
        	stmt = conn.createStatement();
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
        		System.out.println("Trx Batch1 : ERROR: Expecting total of " + totalI + " rows inserted");
        	}
        	pStmt.close();
        	pStmt = null;
        	
        	conn.setAutoCommit(false);
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
        				System.out.println("Trx Batch1 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + updateCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("Trx Batch1 : ERROR: Exception in update batch mode.....");
        		fail("Exception in test JDBCTrxBatch1 - update batch mode.." + bue.getMessage());
        		handleBatchUpdateException(bue);
        	}

        	updateRowCount = pStmt.getUpdateCount();
        	if (updateRowCount != totalMgr) {
        		System.out.println("Trx Batch1: ERROR: Expecting updateRowCount to be " + totalMgr);
        		pass = false;
        	}
        	conn.commit();
        	conn.setAutoCommit(true);
        	
        	int ret = 0;
        	rs = stmt.executeQuery("select POINTS from " + IN1 + " where TITLE = 'MGR'");
        	if (rs != null) {
        		while (rs.next()) {
        			ret = rs.getInt("POINTS");
        			if (ret != 9) {
        				pass = false;
        				System.out.println("Trx Batch1: ERROR: points for MGR is not updated: " + ret);
        			}
        		}
        		rs.close();
        	}
        	pStmt.close();
        	stmt.close();
        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	System.out.println(e.getMessage());
        	fail("Exception in test JDBCTrxBatch1 .." + e.getMessage());
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
		System.out.println("JDBCTrxBatch1 : Done");
	}

	@Test
	public void JDBCTrxBatch2() throws InterruptedException, SQLException 
	{
		System.out.println("JDBC Trx batch test 2..");

        Statement stmt = null;
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTRX2";
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
        String IN1 = "BATCHTRX2";
        String IN2 = null;
		try
		(
			Connection conn = Utils.getUserConnection();
		)
		{
	        Utils.dropTable(conn, IN1);
	        Utils.createTable(conn, IN1);
	        conn.setAutoCommit(false);
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
        				System.out.println("Trx Batch2 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("Trx Batch2 : ERROR: Exception in insert batch mode.....");
        		fail("Exception in test JDBCTrxBatch2 - insert batch mode.." + bue.getMessage());
        		handleBatchUpdateException(bue);
        	}
        	conn.commit();
        	conn.setAutoCommit(true);
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
        		System.out.println("Trx Batch2: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
        		pass = false;
        	}

        	stmt = conn.createStatement();
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
        		System.out.println("Trx Batch2 : ERROR: Expecting total of " + totalI + " rows inserted");
        	}
        	pStmt.close();
        	pStmt = null;
        	conn.setAutoCommit(false);
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
        				System.out.println("Trx Batch2 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + updateCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("Trx Batch2 : ERROR: Exception in update batch mode.....");
        		fail("Exception in test JDBCTrxBatch2 - update batch mode.." + bue.getMessage());
        		handleBatchUpdateException(bue);
        	}

        	updateRowCount = pStmt.getUpdateCount();
        	if (updateRowCount != totalMgr) {
        		System.out.println("Trx Batch2: ERROR: Expecting updateRowCount to be " + totalMgr);
        		pass = false;
        	}
        	conn.commit();
        	conn.setAutoCommit(true);
        	int ret = 0;
        	rs = stmt.executeQuery("select POINTS from " + IN1 + " where TITLE = 'MGR'");
        	if (rs != null) {
        		while (rs.next()) {
        			ret = rs.getInt("POINTS");
        			if (ret != 9) {
        				pass = false;
        				System.out.println("Trx Batch2: ERROR: points for MGR is not updated: " + ret);
        			}
        		}
        		rs.close();
        	}
        	pStmt.close();
        	stmt.close();
        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	System.out.println(e.getMessage());
        	fail("Exception in test JDBCTrxBatch2 .." + e.getMessage());
        } finally {
        	if (pass)
        		System.out.println("JDBCTrxBatch2 :  Passed");
        	else
        		System.out.println("JDBCTrxBatch2 : FAILED");
        	assertEquals(true, pass);
        }
		System.out.println("JDBCTrxBatch2 :  Done");
	}

	@Test
	public void JDBCTrxBatch3() throws InterruptedException, SQLException 
	{
		System.out.println("JDBC Trx batch test 3..");

		Statement stmt = null;
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
		)
		{
           	stmt = conn.createStatement();
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

	        conn.setAutoCommit(false);
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
        				System.out.println("Trx Batch3 : ERROR: Expecting return code for " + i + "th command: -2 or 1, but got : " + insertCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("Trx Batch3 : ERROR: Exception in insert batch mode.....");
            	bue.printStackTrace();
            	System.out.println(bue.getMessage());
            	fail("Exception in test JDBCTrxBatch3 - insert batch mode.." + bue.getMessage());
        		handleBatchUpdateException(bue);
        	}
        	conn.commit();
        	conn.setAutoCommit(true);
        	
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
        		System.out.println("Trx Batch3: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT + " but got " + updateRowCount);
        		pass = false;
        	}

        	// Verify the inserted rows
        	stmt = conn.createStatement();
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
        		System.out.println("Trx Batch3 : ERROR: Expecting total of " + totalI + " rows inserted");
        	}
        	pStmt.close();
        	pStmt = null;

        	stmt.close();
        	conn.close();

        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	System.out.println(e.getMessage());
        	fail("Exception in test JDBCTrxBatch3 .." + e.getMessage());
        } finally {
        	/**/
        	if (pass)
        		System.out.println("JDBCTrxBatch3 :  Passed");
        	else
        		System.out.println("JDBCTrxBatch3 : FAILED");
        	/**/
        	assertEquals(true, pass);
        }
		System.out.println("JDBCTrxBatch3 : Done");
	}

	//BATCH DELETE
	@Test
	public void JDBCTrxBatch4() throws InterruptedException, SQLException 
	{
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String delete = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTRX4";
        int totalQA = 100;
        int totalPoints = 99999;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] deleteCount = null;
        int[] updateCount = null;
        int TOTALUPDATECOUNT = totalQA;
        boolean pass = true;
        String IN1 = "BATCHTRX4";
        String IN2 = null;

		System.out.println("JDBC Trx batch test 4..");

        try
        {
			Connection conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            

	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	stmt.executeUpdate("drop table if exists " + ISTBL + " cascade");
	        stmt.executeUpdate("create table " + ISTBL + "(a int not null, b varchar(2000), c int, d varchar(400), primary key(a))");
	        conn.setAutoCommit(false);
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
	        conn.commit();
	        conn.setAutoCommit(true);
    		for (int i = 0; i < insertCount.length; i++) {
    			if ((insertCount[i] != -2) && (insertCount[i] != 1)) {
    				pass = false;
    				System.out.println("JDBCTrxBatch4 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
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
        		System.out.println("JDBCTrxBatch4: 1 : ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT + " but was " + updateRowCount);
        		pass = false;
        	}

        	//DELETE BATCH
	        //create table " + ISTBL + "(a int not null, b varchar(200000), c int, d varchar(40000), primary key(a))");
	        delete = "delete from " + ISTBL + " where d = ?";
	        conn.setAutoCommit(false);
	        pStmt = conn.prepareStatement(delete);
	        pStmt.setString(1, "DBADMIN");
	        pStmt.addBatch();
	        pStmt.setString(1, "ODBC");
	        pStmt.addBatch();

	        deleteCount = pStmt.executeBatch();
    		for (int i = 0; i < deleteCount.length; i++) {
    			if ((deleteCount[i] != -2) && (deleteCount[i] != 1)) {
    				pass = false;
    				System.out.println("JDBCTrxBatch4 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + deleteCount[i]);
    			}
    		}
    		conn.commit();
    		conn.setAutoCommit(true);
        	sw = pStmt.getWarnings();
        	totalW = 0;
        	while (sw != null) {
        		totalW++;
        		sw = sw.getNextWarning();
        	}
        	
        	int expDeletedRows = 66;
        	updateRowCount = pStmt.getUpdateCount();
        	if (updateRowCount != expDeletedRows) {
        		System.out.println("JDBCTrxBatch4: 2 : ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT + " but was " + updateRowCount);
        		pass = false;
        	}

        	// Verify the deleted rows
        	stmt = conn.createStatement();
        	int totalI = 0;
        	rs = stmt.executeQuery("select * from " + IN1);
        	int rowNo = 0;
    		while (rs.next()) {
    			rowNo++;
    			if (!rs.getString(4).equals("JDBC"))
    				System.out.println("JDBCTrxBatch4 failed : Col d should have JDBC in row " + rowNo);
    		}
    		rs.close();
        	pStmt.close();
        	pStmt = null;
        	stmt.close();
        	conn.close();
        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	System.out.println(e.getMessage());
        	fail("Exception in test JDBCTrxBatch4 .." + e.getMessage());
        } finally {
        	if (pass)
        		System.out.println("JDBC Trx Batch4 :  Passed");
        	else
        		System.out.println("JDBC Trx Batch4 : FAILED");
        	assertEquals(true, pass);
        }
	}

	//BATCH UPSERT/UPDATE
	@Test
	public void JDBCTrxBatch5() throws InterruptedException, SQLException 
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

		System.out.println("JDBC Trx batch test 5..");

        try
        {
			Connection conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            

	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	stmt.executeUpdate("drop table if exists " + ISTBL + " cascade");
	        stmt.executeUpdate("create table " + ISTBL + "(a int not null, b varchar(2000), c int, d varchar(400), primary key(a))");
	        conn.setAutoCommit(false);
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
    				System.out.println("JDBCTrxBatch5 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
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
        		System.out.println("JDBCTrxBatch5: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
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
    				System.out.println("JDBCTrxBatch5 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + updateCount[i]);
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
        		System.out.println("JDBCTrxBatch5: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
        		pass = false;
        	}
        	conn.commit();
        	conn.setAutoCommit(true);

        	// Verify the updated rows
        	stmt = conn.createStatement();
        	int totalI = 0;
        	rs = stmt.executeQuery("select * from " + IN1);
        	int rowNo = 0;
    		while (rs.next()) {
    			rowNo++;
    			if (!rs.getString(2).equals("Quality"))
    				System.out.println("JDBCTrxBatch5 failed : Col d should have JDBC in row " + rowNo);
    		}
    		rs.close();
        	pStmt.close();
        	pStmt = null;
        	stmt.close();
        	conn.close();
        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	System.out.println(e.getMessage());
        	fail("Exception in test JDBCTrxBatch5 .." + e.getMessage());
        } finally {
        	if (pass)
        		System.out.println("JDBC Trx Batch5 :  Passed");
        	else
        		System.out.println("JDBC Trx Batch5 : FAILED");
        	assertEquals(true, pass);
        }
	}

	@Test
	public void JDBCTrxBatch6() throws InterruptedException, SQLException 
	{
		System.out.println("JDBC Trx batch test 6..");

		Statement stmt = null;
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "TRXALL6";
        int total = 10000;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] updateCount = null;
        int TOTALUPDATECOUNT = total;
        boolean pass = true;
        String IN1 = "TRXALL6";
        String IN2 = null;
        int insertCountTemplate = -2;
        int[] updateCountTemplate = { -2 };
		try
		(
			Connection conn = Utils.getUserConnection();
		)
		{
           	stmt = conn.createStatement();
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
           	conn.setAutoCommit(false);
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
        				System.out.println("JDBC Trx Batch6 : ERROR: Expecting return code for " + i + "th command: -2 or 1, but got : " + insertCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("JDBC Trx Batch6 : ERROR: Exception in insert batch mode.....");
        		fail("BatchUpdateException in test Batch6 - insert batch mode .." + bue.getMessage());
            	bue.printStackTrace();
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
        		System.out.println("JDBC Trx Batch6: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT + " but got " + updateRowCount);
        		pass = false;
        	}
        	conn.commit();
        	conn.setAutoCommit(true);

        	// Verify the inserted rows
        	stmt = conn.createStatement();
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
        		System.out.println("JDBC Trx Batch6 : ERROR: Expecting total of " + totalI + " rows inserted");
        	}
        	pStmt.close();
        	pStmt = null;

        	stmt.close();
        	conn.close();

        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	System.out.println(e.getMessage());
        	fail("Exception in test JDBCTrxBatch6 .." + e.getMessage());
        } finally {
        	/**/
        	if (pass)
        		System.out.println("JDBCTrxBatch6 :  Passed");
        	else
        		System.out.println("JDBCTrxBatch6 : FAILED");
        	/**/
        	assertEquals(true, pass);
        }
		System.out.println("JDBCTrxBatch6 : Done");
	}

	@Test
	public void JDBCRollBackTrx1() throws InterruptedException, SQLException
	{
		String tableName="qatabupd";
	    try
	    (
	     	Connection  connection = Utils.getUserConnection();
	        Statement stmt = connection.createStatement();            
	    )
	    {
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b char(20), c smallint, d integer, primary key(a))");
	        connection.setAutoCommit(false);
            stmt.executeUpdate("insert into " + tableName + " values(1, 'Moe', 100, 223), (2, 'Curly', 934, 444), (3, 'Larry', 1000, 999), (4, 'Moe', 12, 333), (5, 'Moe', 98, 987), (6, 'Moe', 54, 212)");
            connection.commit();
	        String updqry = "update " + tableName + " set b = ? where c > ?";              
	        long startTime = System.currentTimeMillis();
	        connection.setAutoCommit(false);
	        try 
	        (
	        	PreparedStatement pstmt = connection.prepareStatement(updqry);
	        )
	        {
		        pstmt.setString(1, "Moe");
		        pstmt.setInt(2, 890);
	         	assertEquals("Rows updated", 2, pstmt.executeUpdate());
	        }
	        catch (SQLException ex)
	        {
	            ex.printStackTrace();
		     fail("Exception in pstmt in test JDBCRollBackTrx1 - Update.." + ex.getMessage());	        	
	        }
	        connection.rollback();
	        connection.setAutoCommit(true);
	        ResultSet rs = stmt.executeQuery("select count(*) from " + tableName + " where b = 'Moe'");
	        rs.next();
	        assertEquals ("Count after rollback", 4, rs.getInt(1));
	        long endTime = System.currentTimeMillis();
	        long elapsedTime = (endTime - startTime)/1000; //in seconds
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
			System.out.println("JDBC RollBack Trx1 - Update : Passed");
		} 
	    catch (Exception ex) 
	    {
            ex.printStackTrace();
			fail("Exception in test JDBCRollBackTrx1 - Update.." + ex.getMessage());
	 	}
		System.out.println("JDBCRollBackTrx1 - Update : Done");
	}
	
	@Test
	public void JDBCRollBackTrx2() throws InterruptedException, SQLException
	{
		String tableName="qatabups";
	    try
	    (
		    Connection  connection = Utils.getUserConnection();
		    Statement stmt = connection.createStatement();            
	    )
	    {
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        long startTime = System.currentTimeMillis();
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b char(20), c smallint, d integer, primary key(a))");
	        connection.setAutoCommit(false);
            stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 100, 223)");
            stmt.executeUpdate("upsert into " + tableName + " values(2, 'Curly', 34, 444)");
            stmt.executeUpdate("upsert into " + tableName + " values(3, 'Larry', 100, 999)");
            stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 12, 333)");
            stmt.executeUpdate("upsert into " + tableName + " values(2, 'Moe', 98, 987)");
            stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 54, 212)");
	        connection.rollback();
	        connection.setAutoCommit(true);
            try
            (
            		ResultSet rs = stmt.executeQuery("select count(*) from " + tableName);
            )
            {
            	rs.next();
            	assertEquals("Upsert row count", 0, rs.getInt(1));
            }
            catch(SQLException ex)
            {
                ex.printStackTrace();
    			fail("Exception in resultset in test JDBCRollBackTrx2 - Update.." + ex.getMessage());
            }
         	//rs.close();
	        long endTime = System.currentTimeMillis();
	        long elapsedTime = (endTime - startTime)/1000; //in seconds
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
			System.out.println("JDBCRollBackTrx2 - SelectCount* : Passed");
		} 
	    catch (Exception ex) 
	    {
            ex.printStackTrace();
			fail("Exception in test JDBCRollBackTrx2 - SelectCount*.." + ex.getMessage());
	 	}
		System.out.println("JDBCRollBackTrx2 - SelectCount* : Done");
	}

	@Test
	public void JDBCRollBackTrx4 () throws InterruptedException, SQLException
	{
		try
		(
			Connection  connection = Utils.getUserConnection();
			Statement stmt = connection.createStatement();            
		)
		{
			stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	        //int randomNumber = ( int )( Math.random() * 9999 );
		  	//String tableName="qatabdel" + randomNumber;
			String tableName="qatabdel";
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
			stmt.executeUpdate("create table " + tableName + "(a int not null, b char(20), c smallint, d integer, primary key(a))");
	        connection.setAutoCommit(false);
			stmt.executeUpdate("insert into " + tableName + " values (223, 'Moe', 100, 223), (444, 'Curly', 34, 444), (999, 'Larry', 100, 999), (333, 'Moe', 12, 333), (987, 'Moe', 98, 987), (212, 'Moe', 54, 212)");
	        connection.commit();
	        connection.setAutoCommit(false);
			String updqry = "delete from " + tableName + " where a > ?";
			long startTime = System.currentTimeMillis();
			PreparedStatement pstmt = connection.prepareStatement(updqry);
			pstmt.setInt(1,900);
			int rowsDeleted = pstmt.executeUpdate();
			connection.rollback();
	        connection.setAutoCommit(true);
			 
			assertEquals("Rows Deleted", 2, rowsDeleted);
			
			long endTime = System.currentTimeMillis();
			long elapsedTime = (endTime - startTime)/1000; //in seconds
			//System.out.println("Total time required to delete records using PreparedStatement using executeUpdate() is :" + elapsedTime + " sec(s)");
			//log.info("Total time required to delete records using PreparedStatement using executeUpdate() is :" + elapsedTime + " sec(s)");			      
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
			System.out.println("JDBCRollBackTrx4 - Delete : Passed");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in test JDBCRollBackTrx4 - Delete.." + ex.getMessage());
		}
		System.out.println("JDBCRollBackTrx4 - Delete : Done");
	}

	@Test
	public void JDBCRollBackTrx6() throws SQLException, ClassNotFoundException 
	{
//		System.out.println("JDBCInsert..");
	    int rowNo = 0, rowsDeleted = 0, rowsUpdated=0;
        //int randomNumber = ( int )( Math.random() * 9999 );
	  	//String tableName="qatabins" + randomNumber;
	    String tableName="qatabins";

		try 
		(
	        Connection conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();
		)
		{
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
            stmt.executeUpdate("create table " + tableName + " (c1 char(20), c2 smallint, c3 integer)");
	        conn.setAutoCommit(false);
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 100, 223)");
            stmt.executeUpdate("insert into " + tableName + " values('Curly', 34, 444)");
            stmt.executeUpdate("insert into " + tableName + " values('Larry', 100, 999)");
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 12, 333)");
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 98, 987)");
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 54, 212)");
	        conn.commit();
	        conn.setAutoCommit(false);
            rowsUpdated = stmt.executeUpdate("update " + tableName + " set c2 = 8787 where c3 > 300");
         	assertEquals("Rows Updated", 4, rowsUpdated);
	        rowsDeleted = stmt.executeUpdate("delete from " + tableName + " where c2 < 200");
         	assertEquals("Rows deleted", 2, rowsDeleted);
         	conn.rollback();
            try
            (
            	ResultSet rs = stmt.executeQuery("select count(*) from " + tableName);
            )
            {
	         	rs.next();
	         	assertEquals("Count after rollback", 6, rs.getInt(1));
            }
            conn.setAutoCommit(true);
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
			System.out.println("JDBCRollBackTrx6 - Insert : Passed");
        } catch (Exception ex)
        {
            ex.printStackTrace();
            fail("Exception in test JDBCRollBackTrx6 - Insert.." + ex.getMessage());
        } 
		System.out.println("JDBCRollBackTrx6 - Insert : Done");
	}
	
	@Test
	public void JDBCRollBackTrxBatch1() throws InterruptedException, SQLException 
	{
		System.out.println("JDBC RollBack Trx batch test 1..");

        Statement stmt = null;
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTRX1";
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
        String IN1 = "BATCHTRX1";
        String IN2 = null;
	 try
	 (
		Connection conn = Utils.getUserConnection();
	 )
	 {
	        Utils.dropTable(conn, IN1);
	        Utils.createTable(conn, IN1);

	        conn.setAutoCommit(false);
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
        				System.out.println("RollBack Trx Batch1 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("RollBack Trx Batch1 : ERROR: Exception in insert batch mode.....");
        		fail("BatchUpdateException in test JDBCRollBackTrxBatch1 - insert batch mode.." + bue.getMessage());
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
        		System.out.println("RollBack Trx Batch1: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
        		pass = false;
        	}
        	conn.commit();
        	conn.setAutoCommit(true);
        	// Verify the inserted rows
        	stmt = conn.createStatement();
        	int totalI = 0;
        	rs = stmt.executeQuery("select count(*) from " + IN1);
        	if (rs != null) {
        		rs.next();
        		totalI = rs.getInt(1);
        	}
        	rs.close();
        	if (totalI != TOTALUPDATECOUNT) {
        		pass = false;
        		System.out.println("RollBack Trx Batch1 : ERROR: Expecting total of " + totalI + " rows inserted");
        	}
        	pStmt.close();
        	pStmt = null;
        	
        	conn.setAutoCommit(false);
        	update = "delete from " + IN1 + " where TITLE = ? ";
        	pStmt = conn.prepareStatement(update);
        	pStmt.setString(1, "MGR");
        	pStmt.addBatch();

        	try {
        		updateCount = pStmt.executeBatch();

        		for (int i = 0; i < updateCount.length; i++) {
        			if ((updateCount[i] != -2) && (updateCount[i] != 2)) {
        				pass = false;
        				System.out.println("RollBack Trx Batch1 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + updateCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("RollBack Trx Batch1 : ERROR: Exception in update batch mode.....");
        		fail("BatchUpdateException in test JDBCRollBackTrxBatch1 - update batch mode.." + bue.getMessage());
        		handleBatchUpdateException(bue);
        	}

        	updateRowCount = pStmt.getUpdateCount();
        	if (updateRowCount != totalMgr) {
        		System.out.println("RollBack Trx Batch1: ERROR: Expecting updateRowCount to be " + totalMgr);
        		pass = false;
        	}
        	conn.rollback();
        	conn.setAutoCommit(true);
        	
        	int ret = 0;
        	rs = stmt.executeQuery("select count(*) from " + IN1 + " where TITLE = 'MGR'");
        	if (rs != null) 
        	{
        		rs.next();
        		ret = rs.getInt(1);
        		if (ret == 0) {
        			pass = false;
        			System.out.println("RollBack Trx Batch1: ERROR: There should be " + totalMgr + " rows with MGR title after rollback, but got " + ret);
        		}
        	}
        	rs.close();
        	pStmt.close();
        	stmt.close();
		
        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	System.out.println(e.getMessage());
        	fail("Exception in test JDBCRollBackTrxBatch1 .." + e.getMessage());
        } finally {
        	/**/
        	if (pass)
        		System.out.println("JDBCRollBackTrxBatch1 :  Passed");
        	else
        		System.out.println("JDBCRollBackTrxBatch1 : FAILED");
        	/**/
        	assertEquals(true, pass);
        }
		System.out.println("JDBCRollBackTrxBatch1 : Done");
	}

	//BATCH DELETE
	@Test
	public void JDBCRollBackTrxBatch4() throws InterruptedException, SQLException 
	{
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String delete = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTRX4";
        int totalQA = 100;
        int totalPoints = 99999;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] deleteCount = null;
        int[] updateCount = null;
        int TOTALUPDATECOUNT = totalQA;
        boolean pass = true;
        String IN1 = "BATCHTRX4";
        String IN2 = null;

		System.out.println("JDBC RollBack Trx batch test 4..");

        try
        {
			Connection conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            

	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	stmt.executeUpdate("drop table if exists " + ISTBL + " cascade");
	        stmt.executeUpdate("create table " + ISTBL + "(a int not null, b varchar(2000), c int, d varchar(400), primary key(a))");
	        conn.setAutoCommit(false);
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
	        conn.commit();
	        conn.setAutoCommit(true);
    		for (int i = 0; i < insertCount.length; i++) {
    			if ((insertCount[i] != -2) && (insertCount[i] != 1)) {
    				pass = false;
    				System.out.println("JDBCRollBackTrxBatch4 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
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
        		System.out.println("JDBCRollBackTrxBatch4: 1 : ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT + " but was " + updateRowCount);
        		pass = false;
        	}

        	//DELETE BATCH
	        //create table " + ISTBL + "(a int not null, b varchar(200000), c int, d varchar(40000), primary key(a))");
	        delete = "delete from " + ISTBL + " where d = ?";
	        conn.setAutoCommit(false);
	        pStmt = conn.prepareStatement(delete);
	        pStmt.setString(1, "DBADMIN");
	        pStmt.addBatch();
	        pStmt.setString(1, "ODBC");
	        pStmt.addBatch();

	        deleteCount = pStmt.executeBatch();
    		for (int i = 0; i < deleteCount.length; i++) {
    			if ((deleteCount[i] != -2) && (deleteCount[i] != 1)) {
    				pass = false;
    				System.out.println("JDBCRollBackTrxBatch4 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + deleteCount[i]);
    			}
    		}
    		conn.rollback();
    		conn.setAutoCommit(true);
        	sw = pStmt.getWarnings();
        	totalW = 0;
        	while (sw != null) {
        		totalW++;
        		sw = sw.getNextWarning();
        	}
        	
        	// Verify the deleted rows
        	stmt = conn.createStatement();
        	int totalI = 0;
        	rs = stmt.executeQuery("select count(*) from " + IN1);
        	int rowNo = 0;
    		rs.next();
    		rowNo = rs.getInt(1);
    		if (rowNo != TOTALUPDATECOUNT)
    			System.out.println("JDBCRollBackTrxBatch4 failed : Rows expected " + TOTALUPDATECOUNT + " but got " + rowNo);
    		
    		rs.close();
        	pStmt.close();
        	pStmt = null;
        	stmt.close();
        	conn.close();
        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	System.out.println(e.getMessage());
        	fail("Exception in test JDBCRollBackTrxBatch4 .." + e.getMessage());
        } finally {
        	if (pass)
        		System.out.println("JDBC RollBack Trx Batch4 :  Passed");
        	else
        		System.out.println("JDBC RollBack Trx Batch4 : FAILED");
        	assertEquals(true, pass);
        }
	}

	//BATCH UPSERT/UPDATE
	@Test
	public void JDBCRollBackBatch5() throws InterruptedException, SQLException 
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

		System.out.println("JDBC RollBack Trx batch test 5..");

        try
        {
			Connection conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            

	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	stmt.executeUpdate("drop table if exists " + ISTBL + " cascade");
	        stmt.executeUpdate("create table " + ISTBL + "(a int not null, b varchar(2000), c int, d varchar(400), primary key(a))");
	        conn.setAutoCommit(false);
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
    				System.out.println("JDBCRollBackTrxBatch5 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
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
        		System.out.println("JDBCRollBackTrxBatch5: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
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
    				System.out.println("JDBCRollBackTrxBatch5 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + updateCount[i]);
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
        		System.out.println("JDBCRollBackTrxBatch5: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
        		pass = false;
        	}
        	conn.rollback();
        	conn.setAutoCommit(true);

        	// Verify the updated rows
        	stmt = conn.createStatement();
        	int totalI = 0;
        	rs = stmt.executeQuery("select count(*) from " + IN1 + " where b = 'Quality'");
        	int rowNo = 0;
    		rs.next();
    		rowNo = rs.getInt(1);
    		if (rowNo != 0)
    				System.out.println("JDBCRollBackTrxBatch5 failed : Col b should not have Quality, but found " + rowNo + " rows, rollback failed.");
    		
    		rs.close();
        	pStmt.close();
        	pStmt = null;
        	stmt.close();
        	conn.close();
        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	System.out.println(e.getMessage());
        	fail("Exception in test JDBCRollBackTrxBatch5 .." + e.getMessage());
        } finally {
        	if (pass)
        		System.out.println("JDBC RollBack Trx Batch5 :  Passed");
        	else
        		System.out.println("JDBC RollBack Trx Batch5 : FAILED");
        	assertEquals(true, pass);
        }
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

