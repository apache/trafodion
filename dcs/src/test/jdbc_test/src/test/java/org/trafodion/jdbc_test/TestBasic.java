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

public class TestBasic 
{
	/* Test basic insert and update sql statements */
	@Test
	public void JDBCBasic1() throws InterruptedException, SQLException
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
            stmt.executeUpdate("insert into " + tableName + " values(1, 'Moe', 100, 223), (2, 'Curly', 934, 444), (3, 'Larry', 1000, 999), (4, 'Moe', 12, 333), (5, 'Moe', 98, 987), (6, 'Moe', 54, 212)");
	        String updqry = "update " + tableName + " set b = ? where c > ?";              
	        long startTime = System.currentTimeMillis();
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
				fail("Exception in pstmt in test JDBC TestBasic1 - Update.." + ex.getMessage());	        	
	        }
	        long endTime = System.currentTimeMillis();
	        long elapsedTime = (endTime - startTime)/1000; //in seconds
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
			System.out.println("JDBC TestBasic1 - Update : Passed");
		} 
	    catch (Exception ex) 
	    {
            ex.printStackTrace();
			fail("Exception in test JDBC TestBasic1 - Update.." + ex.getMessage());
	 	}
		System.out.println("JDBC TestBasic1 - Update : Done");
	}
	
	/* Test Select Count * */
	@Test
	public void JDBCBasic2() throws InterruptedException, SQLException
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
            stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 100, 223)");
            stmt.executeUpdate("upsert into " + tableName + " values(2, 'Curly', 34, 444)");
            stmt.executeUpdate("upsert into " + tableName + " values(3, 'Larry', 100, 999)");
            stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 12, 333)");
            stmt.executeUpdate("upsert into " + tableName + " values(2, 'Moe', 98, 987)");
            stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 54, 212)");
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
    			fail("Exception in resultset in test JDBC TestBasic2 - SelectCount*.." + ex.getMessage());
            }
         	//rs.close();
	        long endTime = System.currentTimeMillis();
	        long elapsedTime = (endTime - startTime)/1000; //in seconds
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
			System.out.println("JDBC TestBasic2 - SelectCount* : Passed");
		} 
	    catch (Exception ex) 
	    {
            ex.printStackTrace();
			fail("Exception in test JDBC TestBasic2 - SelectCount*.." + ex.getMessage());
	 	}
		System.out.println("JDBC TestBasic2 - SelectCount* : Done");
	}

	/* Test fetch of resultset metadata */
	@Test
	public void JDBCBasic3() throws InterruptedException, SQLException 
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
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 100, 223), ('Curly', 34, 444), ('Larry', 100, 999), ('Moe', 12, 333), ('Moe', 98, 987), ('Moe', 54, 212)");
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
    			fail("Exception in resultset in test JDBC TestBasic3 - Metadata.." + ex.getMessage());
            }
		
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
			System.out.println("JDBC TestBasic3 - Metadata : Passed");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in test JDBC TestBasic3 - Metadata.." + ex.getMessage());
		}
		System.out.println("JDBC TestBasic3 - Metadata : Done");
    }

	/* Test delete sql stmt */
	@Test
	public void JDBCBasic4 () throws InterruptedException, SQLException
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
			stmt.executeUpdate("insert into " + tableName + " values (223, 'Moe', 100, 223), (444, 'Curly', 34, 444), (999, 'Larry', 100, 999), (333, 'Moe', 12, 333), (987, 'Moe', 98, 987), (212, 'Moe', 54, 212)");
			String updqry = "delete from " + tableName + " where a > ?";              
			long startTime = System.currentTimeMillis();
			PreparedStatement pstmt = connection.prepareStatement(updqry);
			pstmt.setInt(1,900);
			int rowsDeleted = pstmt.executeUpdate();
			 
			assertEquals("Rows Deleted", 2, rowsDeleted);
			
			long endTime = System.currentTimeMillis();
			long elapsedTime = (endTime - startTime)/1000; //in seconds
			//System.out.println("Total time required to delete records using PreparedStatement using executeUpdate() is :" + elapsedTime + " sec(s)");
			//log.info("Total time required to delete records using PreparedStatement using executeUpdate() is :" + elapsedTime + " sec(s)");			      
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
			System.out.println("JDBC TestBasic4 - Delete : Passed");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in test JDBC TestBasic4 - Delete.." + ex.getMessage());
		}
		System.out.println("JDBC TestBasic4 - Delete : Done");
	}

	/* Test delimited names */
	@Test
	public void JDBCBasic5 () throws SQLException, ClassNotFoundException 
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
       	   	stmt.executeUpdate("insert into " + tableName + " values(1, 200, 300, 400)");
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
       	   	} catch (Exception ex) {
       	   	    ex.printStackTrace();
       	   	    fail("Exception in resultset JDBC TestBasic5 .." + ex.getMessage());
    		}
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
   	   		//conn.close();
   	   		System.out.println("JDBC TestBasic5 - Delim : Passed");
        } catch (Exception ex)
        {
        	ex.printStackTrace();
        	fail("Exception in test JDBC TestBasic5 - Delim.." + ex.getMessage());
        } 
	   		System.out.println("JDBC TestBasic5 - Delim : Done");
	}
	
	/* Test insert and update */
	@Test
	public void JDBCBasic6() throws SQLException, ClassNotFoundException 
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
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 100, 223)");
            stmt.executeUpdate("insert into " + tableName + " values('Curly', 34, 444)");
            stmt.executeUpdate("insert into " + tableName + " values('Larry', 100, 999)");
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 12, 333)");
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 98, 987)");
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 54, 212)");
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
            } catch (Exception ex) {
                ex.printStackTrace();
                fail("Exception in resultset JDBC TestBasic6 .." + ex.getMessage());
    		}
            rowsUpdated = stmt.executeUpdate("update " + tableName + " set c2 = 8787 where c3 > 300");
         	assertEquals("Rows Updated", 4, rowsUpdated);
	        rowsDeleted = stmt.executeUpdate("delete from " + tableName + " where c2 < 200");
         	assertEquals("Rows deleted", 2, rowsDeleted);
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
			System.out.println("JDBC TestBasic6 - Insert : Passed");
        } catch (Exception ex)
        {
            ex.printStackTrace();
            fail("Exception in test JDBC TestBasic6 - Insert.." + ex.getMessage());
        } 
		System.out.println("JDBC TestBasic6 - Insert : Done");
	}

	/* Test upsert */
	@Test
    public void TestBasic7() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBCUpsert..");
        String sql = null;
        int rowNo = 0, rowsDeleted = 0, rowsUpdated=0, rowsUpserted = 0;
        String tableName="qatabU";
        String tableName2="qatabU2";

        try 
        (
        	Connection conn = Utils.getUserConnection();
           	Statement stmt = conn.createStatement();
        )
        {
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	        stmt.executeUpdate("drop table if exists " + tableName + " cascade");
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");
           	stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 100, 223)");
           	stmt.executeUpdate("upsert into " + tableName + " values(2, 'Curly', 34, 444)");
           	stmt.executeUpdate("upsert into " + tableName + " values(3, 'Larry', 100, 999)");
           	stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 12, 333)");
           	stmt.executeUpdate("upsert into " + tableName + " values(2, 'Moe', 98, 987)");
           	stmt.executeUpdate("upsert into " + tableName + " values(3, 'Moe', 54, 212)");
           	try
           	(
           		ResultSet rs = stmt.executeQuery("select * from " + tableName);
           	)
           	{
	           	rowNo = 0;
	           	rs.next();
	        	assertEquals("Column 1 data", (Integer)1, (Integer)rs.getObject(1));
	        	assertEquals("Column 2 data", "Moe", rs.getObject(2).toString().trim());
	        	Short exp = 12;
	        	assertEquals("Column 3 data", (Short)exp, (Short)rs.getShort(3));
	        	assertEquals("Column 4 data", (Integer)333, (Integer)rs.getObject(4));
			   
	        	rs.next();
	        	assertEquals("Column 1 data", (Integer)2, (Integer)rs.getObject(1));
	        	assertEquals("Column 2 data", "Moe", rs.getObject(2).toString().trim());
	        	exp = 98;
	        	assertEquals("Column 3 data", (Short)exp, (Short)rs.getShort(3));
	        	assertEquals("Column 4 data", (Integer)987, (Integer)rs.getObject(4));
	
	        	rs.next();
	        	assertEquals("Column 1 data", (Integer)3, (Integer)rs.getObject(1));
	        	assertEquals("Column 2 data", "Moe", rs.getObject(2).toString().trim());
	        	exp = 54;
	        	assertEquals("Column 3 data", (Short)exp, (Short)rs.getShort(3));
	        	assertEquals("Column 4 data", (Integer)212, (Integer)rs.getObject(4));
           	} catch (Exception ex) {
                ex.printStackTrace();
                fail("Exception in resultset JDBC TestBasic7 .." + ex.getMessage());
    		}
        	
	        stmt.executeUpdate("drop table if exists " + tableName2 + " cascade");
            stmt.executeUpdate("create table " + tableName2 + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");
            rowsUpserted = stmt.executeUpdate("upsert into " + tableName2 + " select * from " + tableName);
        	assertEquals("Rows upserted", (Integer)3, (Integer)rowsUpserted);
			System.out.println("JDBC TestBasic7 - Upsert : Passed");
        } catch (Exception ex)
       	{
           	ex.printStackTrace();
           	fail("Exception in test JDBC TestBasic7 - Upsert.." + ex.getMessage());
       	} 
        System.out.println("JDBC TestBasic7 - Upsert : Done");
     }

	/* Test Get User Connection */
	@Test
    public void TestBasic17() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get User Connection..");

        try
        (
        	Connection conn = Utils.getUserConnection();
        )
        {
			//System.out.println("Connected using getUserConnection");
			System.out.println("JDBC TestBasic17 - Get User Connection : Passed");
        } catch (Exception ex)
       	{
           	ex.printStackTrace();
           	fail("Exception in test JDBC TestBasic17 - Get User Connection.." + ex.getMessage());
       	} 
		System.out.println("JDBC TestBasic17 - Get User Connection: Done");
    }

	/* Test Get Properties Cnnection */	
	@Test
    public void TestBasic18() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Properties Connection..");

        try
        (
        	Connection conn = Utils.getPropertiesConnection();
        )
        {
			//System.out.println("Connected using getPropertiesConnection");
			System.out.println("JDBC TestBasic18 - Get Properties Connection : Passed");
        } catch (Exception ex)
       	{
           	ex.printStackTrace();
           	fail("Exception in test JDBC TestBasic18 - Get Properties Connection.." + ex.getMessage());
       	} 
		System.out.println("JDBC TestBasic18 - Get Properties Connection : Done");
    }

	/* Test all datatypes, insert using params*/
	@Test
	public void JDBCBasic19() throws InterruptedException, SQLException 
	{
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "TABALL";
        int total = 1;
        long startTime = 0;
        long endTime = 0;
        int TOTALUPDATECOUNT = total;
        boolean pass = true;
        String IN1 = "TABALL";
        String IN2 = null;
        int insertCount;
        
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

           	insert = "insert into " + IN1 + " values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
	        pStmt = conn.prepareStatement(insert);
	        startTime = System.currentTimeMillis();
        	pStmt.setInt(1, 1);
        	
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
        	
        	insertCount = pStmt.executeUpdate();

        	int updateRowCount = pStmt.getUpdateCount();
        	if (updateRowCount != TOTALUPDATECOUNT) {
        		System.out.println("JDBC Basic 19 : ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT + " but got " + updateRowCount);
        		pass = false;
        	}

        	// Verify the inserted rows
        	int totalI = 0;
        	rs = stmt.executeQuery("select * from " + IN1);
        	if (rs != null) {
        		ResultSetMetaData rsMD = rs.getMetaData();
        		while (rs.next()) {
        			totalI++;
        			// for (int j=1; j <= rsMD.getColumnCount(); j++)
        			// System.out.println("Column " + j + ":" +
        			// rs.getObject(j));
        		}
        		rs.close();
        	}
        	if (totalI != TOTALUPDATECOUNT) {
        		pass = false;
        		System.out.println("JDBC Basic 19 : ERROR: Expecting total of " + totalI + " rows inserted");
        	}
        	pStmt.close();
        	pStmt = null;
        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	fail("Exception in JDBC TestBasic19 .." + e.getMessage());
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
		System.out.println("JDBCBasic19 - Param All : Done");
	}

	public static void handleBatchUpdateException(BatchUpdateException e) 
	{
      int[] bueUpdateCount = e.getUpdateCounts();

      Throwable cause = e.getCause();
      while (cause != null) {
      	cause = cause.getCause();
      }

      SQLException nextException = e.getNextException();

      while (nextException != null) {

      	cause = nextException.getCause();
      	while (cause != null) {
      		cause = cause.getCause();
      	}
      	nextException = nextException.getNextException();
      }
  }
}

