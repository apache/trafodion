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

public class TestBigCol
{
	/* Test varchar col size 200k, insert 200k long data */
	@Test
	public void JDBCBigCol1() throws InterruptedException, SQLException
	{
		System.out.println("JDBC varchar 200k test 1..");
		String tableName="bigcoltab1";
	    String longstr = "a";

	    while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";
	 	    
	    try
	 	{
	     	Connection  conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b char(20), c varchar(200000), d integer, primary key(a))");
            stmt.executeUpdate("insert into " + tableName + " values (1, 'Zoey', '" + longstr + "', 111)");
            stmt.close();
        	stmt = conn.createStatement();
        	ResultSet rs = stmt.executeQuery("select * from " + tableName);
        	int rowNo = 0;
    		while (rs.next()) 
    		{
    			rowNo++;
    			if (!rs.getString(3).endsWith("ZZ"))
    				System.out.println("Test failed : ZZ at the end missing in row " + rowNo);
    		}
    		rs.close();
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
            conn.close();
			System.out.println("JDBC varchar 200k test 1 : Done");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in JDBC varchar 200k test 1.." + ex.getMessage());
	 	}
	}
	
	/* Test 200k long varchar col, data < and > 32k length */
	@Test
	public void JDBCBigCol2() throws InterruptedException, SQLException
	{
		System.out.println("JDBC varchar 200k test 2..");
		String tableName="bigcoltab2";
	    String longstr = "a";

	    while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";
	 	    
	    try
	 	{
	     	Connection  conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b char(20), c varchar(200000), d integer, primary key(a))");
            stmt.executeUpdate("insert into " + tableName + " values(1, 'Moe', 'aaaZZ', 111), (2, 'Curly', 'bbbZZ', 222), (3, 'Zoey', '" + longstr + "', 333)");
            stmt.close();
        	stmt = conn.createStatement();
        	ResultSet rs = stmt.executeQuery("select * from " + tableName);
        	int rowNo = 0;
    		while (rs.next()) 
    		{
    			rowNo++;
    			if (!rs.getString(3).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k test 2 failed : ZZ at the end missing in row " + rowNo);
    		}
    		rs.close();
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
            conn.close();
			System.out.println("JDBC varchar 200k test 2 : Done");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in JDBC varchar 200k test 2.." + ex.getMessage());
	 	}
	}
	
	/* Test 200k long varchar col, using bound params */
	@Test
	public void JDBCBigCol3() throws InterruptedException, SQLException
	{
		System.out.println("JDBC varchar 200k test 3..");
		String tableName="bigcoltab3";
	    //String longstr = "a";

	    //while (longstr.length() < (200000-2))
        	//longstr = longstr + "a";
	    //longstr = longstr + "ZZ";
	 	    
	    try
	 	{
	     	Connection  conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b varchar(200000), primary key(a))");
            PreparedStatement pstmt = conn.prepareStatement("insert into " + tableName + " values(?, ?)");
            int i;
            for (i = 1; i < 10; i++)
            {
            	pstmt.setInt(1, i);
            	pstmt.setString(2, "aaaZZ");
            	pstmt.executeUpdate();
            }
        	//pstmt.setInt(1, i);
        	//pstmt.setString(2, longstr);
        	//pstmt.executeUpdate();
            
            stmt.close();
        	stmt = conn.createStatement();
        	ResultSet rs = stmt.executeQuery("select * from " + tableName);
        	int rowNo = 0;
    		while (rs.next()) 
    		{
    			rowNo++;
    			if (!rs.getString(2).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k test 3 failed : ZZ at the end missing in row " + rowNo);
    		}
    		rs.close();
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
            conn.close();
			System.out.println("JDBC varchar 200k test 3 : Done");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in JDBC varchar 200k test 3.." + ex.getMessage());
	 	}
	}
	
	/* Test 200k varchar col, data < 32k, using bound params */
	@Test
	public void JDBCBigCol3a() throws InterruptedException, SQLException
	{
		System.out.println("JDBC varchar 200k test 3a..");
		String tableName="bigcoltab3a";
	    //String longstr = "a";

	    //while (longstr.length() < (200000-2))
        	//longstr = longstr + "a";
	    //longstr = longstr + "ZZ";
	 	    
	    try
	 	{
	     	Connection  conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b varchar(20000), primary key(a))");
            PreparedStatement pstmt = conn.prepareStatement("insert into " + tableName + " values(?, ?)");
            int i;
            for (i = 1; i < 10; i++)
            {
            	pstmt.setInt(1, i);
            	pstmt.setString(2, "aaaZZ");
            	pstmt.executeUpdate();
            }
        	//pstmt.setInt(1, i);
        	//pstmt.setString(2, longstr);
        	//pstmt.executeUpdate();
            
            stmt.close();
        	stmt = conn.createStatement();
        	ResultSet rs = stmt.executeQuery("select * from " + tableName);
        	int rowNo = 0;
    		while (rs.next()) 
    		{
    			rowNo++;
    			if (!rs.getString(2).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k test 3a failed : ZZ at the end missing in row " + rowNo);
    		}
    		rs.close();
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
            conn.close();
			System.out.println("JDBC varchar 200k test 3a : Done");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in JDBC varchar 200k test 3a.." + ex.getMessage());
	 	}
	}
	
	/* Test varchar col size 200k, bound param, data 200k length */
	@Test
	public void JDBCBigCol4() throws InterruptedException, SQLException
	{
		System.out.println("JDBC varchar 200k test 4..");
		String tableName="bigcoltab4";
	    String longstr = "a";

	    while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";
	 	    
	    try
	 	{
	     	Connection  conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b varchar(200000), primary key(a))");
            PreparedStatement pstmt = conn.prepareStatement("insert into " + tableName + " values(?, ?)");
        	pstmt.setInt(1, 1);
        	pstmt.setString(2, longstr);
        	pstmt.executeUpdate();
            
            stmt.close();
        	stmt = conn.createStatement();
        	ResultSet rs = stmt.executeQuery("select * from " + tableName);
        	int rowNo = 0;
    		while (rs.next()) 
    		{
    			rowNo++;
    			if (!rs.getString(2).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k test 4 failed : ZZ at the end missing in row " + rowNo);
    		}
    		rs.close();
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
            conn.close();
			System.out.println("JDBC varchar 200k test 4 : Done");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in JDBC varchar 200k test 4.." + ex.getMessage());
	 	}
	}
	
	/* Test varchar col 200k size, upsert, data 200k length*/
	@Test
	public void JDBCBigCol5() throws InterruptedException, SQLException
	{
		System.out.println("JDBC varchar 200k test 5..");
		String tableName="bigcoltab5";
	    String longstr = "a";

	    while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";
	 	    
	    try
	 	{
	     	Connection  conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b char(20), c varchar(200000), d integer, primary key(a))");
            stmt.executeUpdate("upsert into " + tableName + " values (1, 'Zoey', '" + longstr + "', 111)");
            stmt.executeUpdate("upsert into " + tableName + " values (2, 'Joey', '" + longstr + "', 222)");
            stmt.close();
        	stmt = conn.createStatement();
        	ResultSet rs = stmt.executeQuery("select * from " + tableName);
        	int rowNo = 0;
    		while (rs.next()) 
    		{
    			rowNo++;
    			if (!rs.getString(3).endsWith("ZZ"))
    				System.out.println("Test failed : ZZ at the end missing in row " + rowNo);
    		}
    		rs.close();
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
            conn.close();
			System.out.println("JDBC varchar 200k test 5 : Done");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in JDBC varchar 200k test 5.." + ex.getMessage());
	 	}
	}
	
	/* Test varchar 200k col size, upsert, data < and > 32k */
	@Test
	public void JDBCBigCol6() throws InterruptedException, SQLException
	{
		System.out.println("JDBC varchar 200k test 6..");
		String tableName="bigcoltab6";
	    String longstr = "a";

	    while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";
	 	    
	    try
	 	{
	     	Connection  conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b char(20), c varchar(200000), d integer, primary key(a))");
            stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 'aaaZZ', 111), (2, 'Curly', 'bbbZZ', 222), (3, 'Zoey', '" + longstr + "', 333)");
            stmt.close();
        	stmt = conn.createStatement();
        	ResultSet rs = stmt.executeQuery("select * from " + tableName);
        	int rowNo = 0;
    		while (rs.next()) 
    		{
    			rowNo++;
    			if (!rs.getString(3).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k test 6 failed : ZZ at the end missing in row " + rowNo);
    		}
    		rs.close();
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
            conn.close();
			System.out.println("JDBC varchar 200k test 6 : Done");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in JDBC varchar 200k test 6.." + ex.getMessage());
	 	}
	}
	
	/* Test varchar col size 200k size, upsert, bound param, data < 32k length, multiple rows */
	@Test
	public void JDBCBigCol7() throws InterruptedException, SQLException
	{
		System.out.println("JDBC varchar 200k test 7..");
		String tableName="bigcoltab7";
	    //String longstr = "a";

	    //while (longstr.length() < (200000-2))
        	//longstr = longstr + "a";
	    //longstr = longstr + "ZZ";
	 	    
	    try
	 	{
	     	Connection  conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b varchar(200000), primary key(a))");
            PreparedStatement pstmt = conn.prepareStatement("upsert into " + tableName + " values(?, ?)");
            int i;
            for (i = 1; i < 10; i++)
            {
            	pstmt.setInt(1, i);
            	pstmt.setString(2, "aaaZZ");
            	pstmt.executeUpdate();
            }
        	//pstmt.setInt(1, i);
        	//pstmt.setString(2, longstr);
        	//pstmt.executeUpdate();
            
            stmt.close();
        	stmt = conn.createStatement();
        	ResultSet rs = stmt.executeQuery("select * from " + tableName);
        	int rowNo = 0;
    		while (rs.next()) 
    		{
    			rowNo++;
    			if (!rs.getString(2).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k test 7 failed : ZZ at the end missing in row " + rowNo);
    		}
    		rs.close();
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
            conn.close();
			System.out.println("JDBC varchar 200k test 7 : Done");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in JDBC varchar 200k test 7.." + ex.getMessage());
	 	}
	}
	
	/* Test varchar col size 200k, insert data using bound params, data < and > 32k length */
	@Test
	public void JDBCBigCol8() throws InterruptedException, SQLException
	{
		System.out.println("JDBC varchar 200k test 8..");
		String tableName="bigcoltab8";
	    String longstr = "a";

	    while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";
	 	    
	    try
	 	{
	     	Connection  conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b varchar(200000), primary key(a))");
	        conn.setAutoCommit(false);
            PreparedStatement pstmt = conn.prepareStatement("insert into " + tableName + " values(?, ?)");
        	pstmt.setInt(1, 1);
        	pstmt.setString(2, longstr);
        	pstmt.executeUpdate();
            
        	pstmt.setInt(1, 2);
        	pstmt.setString(2, "bbbbbZZ");
        	pstmt.executeUpdate();
        	conn.commit ();
	        conn.setAutoCommit(true);
        	
        	stmt.close();
        	stmt = conn.createStatement();
        	ResultSet rs = stmt.executeQuery("select * from " + tableName);
        	int rowNo = 0;
    		while (rs.next()) 
    		{
    			rowNo++;
    			if (!rs.getString(2).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k test 8 failed : ZZ at the end missing in row " + rowNo);
    		}
    		rs.close();
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
            conn.close();
			System.out.println("JDBC varchar 200k test 8 : Done");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in JDBC varchar 200k test 8.." + ex.getMessage());
	 	}
	}
	
	/* Test varchar col size 200k, upsert using bound params, single row, 200k long data */
	@Test
	public void JDBCBigCol9() throws InterruptedException, SQLException
	{
		System.out.println("JDBC varchar 200k test 9..");
		String tableName="bigcoltab9";
	    String longstr = "a";

	    while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";
	 	    
	    try
	 	{
	     	Connection  conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b varchar(200000), primary key(a))");
            PreparedStatement pstmt = conn.prepareStatement("upsert into " + tableName + " values(?, ?)");
        	pstmt.setInt(1, 1);
        	pstmt.setString(2, longstr);
        	pstmt.executeUpdate();
            
            stmt.close();
        	stmt = conn.createStatement();
        	ResultSet rs = stmt.executeQuery("select * from " + tableName);
        	int rowNo = 0;
    		while (rs.next()) 
    		{
    			rowNo++;
    			if (!rs.getString(2).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k test 9 failed : ZZ at the end missing in row " + rowNo);
    		}
    		rs.close();
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
            conn.close();
			System.out.println("JDBC varchar 200k test 9 : Done");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in JDBC varchar 200k test 9.." + ex.getMessage());
	 	}
	}
	
	/* Test varchar col size 200k, insert using bound params, single row, 200k long data */
	@Test
	public void JDBCBigCol10() throws InterruptedException, SQLException
	{
		System.out.println("JDBC varchar 200k test 10..");
		String tableName="bigcoltab10";
	    String longstr = "a";

	    while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";
	 	    
	    try
	 	{
	     	Connection  conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b varchar(200000), primary key(a))");
            PreparedStatement pstmt = conn.prepareStatement("insert into " + tableName + " values(?, ?)");
        	pstmt.setInt(1, 1);
        	pstmt.setString(2, longstr);
        	pstmt.executeUpdate();
            
            stmt.close();
        	stmt = conn.createStatement();
        	ResultSet rs = stmt.executeQuery("select * from " + tableName);
        	int rowNo = 0;
    		while (rs.next()) 
    		{
    			rowNo++;
    			if (!rs.getString(2).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k test 10 failed : ZZ at the end missing in row " + rowNo);
    		}
    		rs.close();
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
            conn.close();
			System.out.println("JDBC varchar 200k test 10 : Done");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in JDBC varchar 200k test 10.." + ex.getMessage());
	 	}
	}

	/* Test varchar col size 200k, insert, delete using bound params, multiple rows, 200k long data */
	@Test
	public void JDBCBigCol11() throws InterruptedException, SQLException
	{
		System.out.println("JDBC varchar 200k test 11..");
		String tableName="bigcoltab11";
	    String longstr = "a";

	    while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";
	 	    
	    try
	 	{
	     	Connection  conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b varchar(200000), primary key(a))");
        	stmt.close();
	        conn.setAutoCommit(false);
            PreparedStatement pstmt = conn.prepareStatement("insert into " + tableName + " values(?, ?)");
        	pstmt.setInt(1, 1);
        	pstmt.setString(2, longstr);
        	pstmt.executeUpdate();
            
        	pstmt.setInt(1, 2);
        	pstmt.setString(2, "bbbbbZZ");
        	pstmt.executeUpdate();
        	conn.commit ();

	        conn.setAutoCommit(false);
            PreparedStatement pstmt1 = conn.prepareStatement("delete from " + tableName + " where b = ?");
        	pstmt1.setString(1, longstr);
        	pstmt1.executeUpdate();
        	conn.commit ();
	        conn.setAutoCommit(true);
	        
        	stmt = conn.createStatement();
        	ResultSet rs = stmt.executeQuery("select * from " + tableName);
        	int rowNo = 0;
    		while (rs.next()) 
    		{
    			rowNo++;
    			if (!rs.getString(2).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k test 11 failed : ZZ at the end missing in row " + rowNo);
    		}
    		assertEquals("Row count after delete", 1, rowNo);
    		rs.close();
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
            conn.close();
			System.out.println("JDBC varchar 200k test 11 : Done");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in JDBC varchar 200k test 11.." + ex.getMessage());
	 	}
	}

	/* Test varchar col size 200k, insert, update using bound params, multiple rows, 200k long data */	
	public void JDBCBigCol12() throws InterruptedException, SQLException
	{
		System.out.println("JDBC varchar 200k test 12..");
		String tableName="bigcoltab12";
	    String longstr = "a";
	    String updtstr = "b";

	    while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";

	    while (updtstr.length() < (200000-2))
        	updtstr = updtstr + "b";
	    updtstr = updtstr + "XX";

	    try
	 	{
	     	Connection  conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b varchar(200000), primary key(a))");
        	stmt.close();
	        conn.setAutoCommit(false);
            PreparedStatement pstmt = conn.prepareStatement("insert into " + tableName + " values(?, ?)");
        	pstmt.setInt(1, 1);
        	pstmt.setString(2, longstr);
        	pstmt.executeUpdate();
            
        	pstmt.setInt(1, 2);
        	pstmt.setString(2, "bbbbbZZ");
        	pstmt.executeUpdate();
        	conn.commit ();

	        conn.setAutoCommit(false);
            PreparedStatement pstmt1 = conn.prepareStatement("update " + tableName + " set b = ? where a = ?");
        	pstmt.setString(1, updtstr);
        	pstmt.setInt(2, 1);
        	pstmt.executeUpdate();
        	pstmt.setString(1, updtstr);
        	pstmt.setInt(2, 2);
        	pstmt.executeUpdate();
        	conn.commit ();
	        conn.setAutoCommit(true);
	        
        	stmt = conn.createStatement();
        	ResultSet rs = stmt.executeQuery("select * from " + tableName);
        	int rowNo = 0;
    		while (rs.next()) 
    		{
    			rowNo++;
    			if (!rs.getString(2).endsWith("XX"))
    				System.out.println("JDBC varchar 200k test 12 failed : XX at the end missing in row " + rowNo);
    		}
    		assertEquals("Row count after update", 2, rowNo);
    		rs.close();
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
            conn.close();
			System.out.println("JDBC varchar 200k test 12 : Done");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in JDBC varchar 200k test 12.." + ex.getMessage());
	 	}
	}
	
	/* Test varchar col size 200k, using batch insert, 200k long data */
	@Test
	public void JDBCBigColBatch1() throws InterruptedException, SQLException 
	{
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTBL1";
        int totalQA = 20;
        int totalDev = 20;
        int totalPoints = 99999;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] updateCount = null;
        int TOTALUPDATECOUNT = totalQA + totalDev;
        boolean pass = true;
        String IN1 = "BATCHTBL1";
        String IN2 = null;
	    String longstr = "a";

		System.out.println("JDBC varchar 200k batch test 1..");

		while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";

        try
        {
			Connection conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            

	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	stmt.executeUpdate("drop table if exists " + ISTBL + " cascade");
	        stmt.executeUpdate("create table " + ISTBL + "(a int not null, b varchar(200000), c char(200), primary key(a))");
	
	        insert = "insert into " + ISTBL + " values(?,?,?)";
	        pStmt = conn.prepareStatement(insert);
	
	        startTime = System.currentTimeMillis();
	
	        for (int j = 1; j <= totalQA; j++) {
	        	pStmt.setInt(1, j);
	        	pStmt.setString(2, "bbbbbZZ");
	        	pStmt.setString(3, "cccccZZ");
	        	pStmt.addBatch();
	        }

	    	for (int j = (totalQA + 1); j <= (totalDev + totalQA); j++) {
	    		pStmt.setInt(1, j);
	    		pStmt.setString(2, longstr);
	    		pStmt.setString(3, "whatever");
	    		pStmt.addBatch();
	    	}
	    	
    		insertCount = pStmt.executeBatch();
    		for (int i = 0; i < insertCount.length; i++) {
    			if ((insertCount[i] != -2) && (insertCount[i] != 1)) {
    				pass = false;
    				System.out.println("JDBC varchar 200k batch test 1 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
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
        		System.out.println("JDBC varchar 200k batch test 1: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
        		pass = false;
        	}

        	// Verify the inserted rows
        	stmt = conn.createStatement();
        	int totalI = 0;
        	rs = stmt.executeQuery("select * from " + IN1);
        	int rowNo = 0;
    		while (rs.next()) {
    			totalI++;
    			rowNo++;
    			if (!rs.getString(2).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k batch test 1 failed : ZZ at the end missing in row " + rowNo);
    		}
    		rs.close();
        	if (totalI != TOTALUPDATECOUNT) {
        		pass = false;
        		System.out.println("JDBC varchar 200k batch test 1 : ERROR: Expecting total of " + totalI + " rows inserted");
        	}
        	pStmt.close();
        	pStmt = null;

        	stmt.close();
        	conn.close();

        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	fail("Exception in JDBCBigColBatch1.." + e.getMessage());
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
	}

	/* Test varchar col size 200k, upsert using batch, 200k long data */
	@Test
	public void JDBCBigColBatch2() throws InterruptedException, SQLException 
	{
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTBL2";
        int totalQA = 20;
        int totalDev = 20;
        int totalPoints = 99999;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] updateCount = null;
        int TOTALUPDATECOUNT = totalQA + totalDev;
        boolean pass = true;
        String IN1 = "BATCHTBL2";
        String IN2 = null;
	    String longstr = "a";

		System.out.println("JDBC varchar 200k batch test 2..");

		while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";

        try
        {
			Connection conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            

	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	stmt.executeUpdate("drop table if exists " + ISTBL + " cascade");
	        stmt.executeUpdate("create table " + ISTBL + "(a int not null, b varchar(200000), c varchar(40000), d char(200), primary key(a))");
	
	        insert = "upsert into " + ISTBL + " values(?,?,?,?)";
	        pStmt = conn.prepareStatement(insert);
	
	        startTime = System.currentTimeMillis();
	
	        for (int j = 1; j <= totalQA; j++) {
	        	pStmt.setInt(1, j);
	        	pStmt.setString(2, "bbbbbZZ");
	        	pStmt.setString(3, "cccccZZ");
	        	pStmt.setString(4, "cccccZZ");
	        	pStmt.addBatch();
	        }

	    	for (int j = (totalQA + 1); j <= (totalDev + totalQA); j++) {
	    		pStmt.setInt(1, j);
	    		pStmt.setString(2, longstr);
	        	pStmt.setString(3, "cccccZZ");
	    		pStmt.setString(4, "whatever");
	    		pStmt.addBatch();
	    	}
	    	
    		insertCount = pStmt.executeBatch();
    		for (int i = 0; i < insertCount.length; i++) {
    			if ((insertCount[i] != -2) && (insertCount[i] != 1)) {
    				pass = false;
    				System.out.println("JDBC varchar 200k batch test 2 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
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
        		System.out.println("JDBC varchar 200k batch test 2: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
        		pass = false;
        	}

        	// Verify the inserted rows
        	stmt = conn.createStatement();
        	int totalI = 0;
        	rs = stmt.executeQuery("select * from " + IN1);
        	int rowNo = 0;
    		while (rs.next()) {
    			totalI++;
    			rowNo++;
    			if (!rs.getString(2).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k batch test 2 failed : ZZ at the end missing in row " + rowNo);
    			if (!rs.getString(3).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k batch test 2 failed : ZZ at the end missing in row " + rowNo);
    		}
    		rs.close();
        	if (totalI != TOTALUPDATECOUNT) {
        		pass = false;
        		System.out.println("JDBC varchar 200k batch test 2 : ERROR: Expecting total of " + totalI + " rows inserted");
        	}
        	pStmt.close();
        	pStmt = null;

        	stmt.close();
        	conn.close();

        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	fail("Exception in JDBCBigColBatch2.." + e.getMessage());
        	System.out.println(e.getMessage());
        } finally {
        	/**/
        	if (pass)
        		System.out.println("Big Col Batch2 :  Passed");
        	else
        		System.out.println("Big Col Batch2 : FAILED");
        	/**/
        	assertEquals(true, pass);
        }
	}

	/* Test varchar col size 200k, insert/delete using batch, 200k long data */
	@Test
	public void JDBCBigColBatch3() throws InterruptedException, SQLException 
	{
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTBL3";
        int totalQA = 20;
        int totalDev = 20;
        int totalPoints = 99999;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] updateCount = null;
        int[] deleteCount = null;
        int TOTALUPDATECOUNT = totalQA + totalDev;
        boolean pass = true;
        String IN1 = "BATCHTBL3";
        String IN2 = null;
	    String longstr = "a";

		System.out.println("JDBC varchar 200k batch test 3..");

		while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";

        try
        {
			Connection conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            

	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	stmt.executeUpdate("drop table if exists " + ISTBL + " cascade");
	        stmt.executeUpdate("create table " + ISTBL + "(a int not null, b varchar(200000), c char(200), primary key(a))");
	        conn.setAutoCommit(false);
	        insert = "insert into " + ISTBL + " values(?,?,?)";
	        pStmt = conn.prepareStatement(insert);
	
	        startTime = System.currentTimeMillis();
	
	        for (int j = 1; j <= totalQA; j++) {
	        	pStmt.setInt(1, j);
	        	pStmt.setString(2, "bbbbbZZ");
	        	pStmt.setString(3, "cccccZZ");
	        	pStmt.addBatch();
	        }

	    	for (int j = (totalQA + 1); j <= (totalDev + totalQA); j++) {
	    		pStmt.setInt(1, j);
	    		pStmt.setString(2, longstr);
	    		pStmt.setString(3, "whatever");
	    		pStmt.addBatch();
	    	}
	    	
    		insertCount = pStmt.executeBatch();
    		conn.commit();
	        conn.setAutoCommit(true);
    		for (int i = 0; i < insertCount.length; i++) {
    			if ((insertCount[i] != -2) && (insertCount[i] != 1)) {
    				pass = false;
    				System.out.println("JDBC varchar 200k batch test 3 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
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
        		System.out.println("JDBC varchar 200k batch test 3: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
        		pass = false;
        	}

	        conn.setAutoCommit(false);
	        String delete = "delete from " + ISTBL + " where b = ?";
	        pStmt = conn.prepareStatement(delete);
	
	        startTime = System.currentTimeMillis();
	
	        pStmt.setString(1, "bbbbbZZ");
	        pStmt.addBatch();

	    	pStmt.setString(1, longstr);
	    	pStmt.addBatch();
	    	
    		deleteCount = pStmt.executeBatch();
    		conn.commit();
	        conn.setAutoCommit(true);
    		for (int i = 0; i < deleteCount.length; i++) {
    			if ((deleteCount[i] != -2) && (deleteCount[i] != 1)) {
    				pass = false;
    				System.out.println("JDBC varchar 200k batch test 3 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
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
        		System.out.println("JDBC varchar 200k batch test 3: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
        		pass = false;
        	}

        	// Verify the inserted rows
        	stmt = conn.createStatement();
        	int totalI = 0;
        	rs = stmt.executeQuery("select * from " + IN1);
        	int rowNo = 0;
    		while (rs.next()) {
    			totalI++;
    			rowNo++;
    			if (!rs.getString(2).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k batch test 3 failed : ZZ at the end missing in row " + rowNo);
    		}
    		rs.close();
        	if (totalI != 0) {
        		pass = false;
        		System.out.println("JDBC varchar 200k batch test 3 : ERROR: Expecting 0 rows");
        	}
        	pStmt.close();
        	pStmt = null;

        	stmt.close();
        	conn.close();

        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	fail("Exception in JDBCBigColBatch3.." + e.getMessage());
        	System.out.println(e.getMessage());
        } finally {
        	/**/
        	if (pass)
        		System.out.println("Big Col Batch3 :  Passed");
        	else
        		System.out.println("Big Col Batch3 : FAILED");
        	/**/
        	assertEquals(true, pass);
        }
	}

	/* Test varchar col size 200k, insert using batch, multiple >32k varchar cols, 200k long data */
	@Test
	public void JDBCBigColBatch4() throws InterruptedException, SQLException 
	{
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTBL4";
        int totalQA = 1;
        int totalPoints = 99999;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] updateCount = null;
        int TOTALUPDATECOUNT = 1;
        boolean pass = true;
        String IN1 = "BATCHTBL4";
        String IN2 = null;
	    String longstr = "a";

		System.out.println("JDBC varchar 200k batch test 4..");

		while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";

        try
        {
			Connection conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            

	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	stmt.executeUpdate("drop table if exists " + ISTBL + " cascade");
	        stmt.executeUpdate("create table " + ISTBL + "(a int not null, b varchar(200000), c char(200), d varchar(40000), primary key(a))");
	
	        insert = "insert into " + ISTBL + " values(?,?,?,?)";
	        pStmt = conn.prepareStatement(insert);
	
	        startTime = System.currentTimeMillis();
	
	        pStmt.setInt(1, 1);
	        pStmt.setString(2, longstr);
	        pStmt.setString(3, "cccccZZ");
	        pStmt.setString(4, "bbbbbZZ");
	        pStmt.addBatch();

	        insertCount = pStmt.executeBatch();
    		for (int i = 0; i < insertCount.length; i++) {
    			if ((insertCount[i] != -2) && (insertCount[i] != 1)) {
    				pass = false;
    				System.out.println("JDBC varchar 200k batch test 4 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
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
        		System.out.println("JDBC varchar 200k batch test 4: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
        		pass = false;
        	}

        	// Verify the inserted rows
        	stmt = conn.createStatement();
        	int totalI = 0;
        	rs = stmt.executeQuery("select * from " + IN1);
        	int rowNo = 0;
    		while (rs.next()) {
    			totalI++;
    			rowNo++;
    			if (!rs.getString(2).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k batch test 4 failed : ZZ at the end missing in row " + rowNo);
    			if (!rs.getString(4).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k batch test 4 failed : ZZ at the end missing in row " + rowNo);
    		}
    		rs.close();
        	if (totalI != TOTALUPDATECOUNT) {
        		pass = false;
        		System.out.println("JDBC varchar 200k batch test 4 : ERROR: Expecting total of " + totalI + " rows inserted");
        	}
        	pStmt.close();
        	pStmt = null;

        	stmt.close();
        	conn.close();

        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	fail("Exception in JDBCBigColBatch4.." + e.getMessage());
        	System.out.println(e.getMessage());
        } finally {
        	/**/
        	if (pass)
        		System.out.println("Big Col Batch4 :  Passed");
        	else
        		System.out.println("Big Col Batch4 : FAILED");
        	/**/
        	assertEquals(true, pass);
        }
	}

	/* Test varchar col size 200k, batch insert and delete, multiple >32k varchar cols, 200k long data */
	@Test
	public void JDBCBigColBatch5() throws InterruptedException, SQLException 
	{
        ResultSet rs = null;
        String DEPT = null;
        String JOB = null;
        String insert = null;
        String delete = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTBL5";
        int totalQA = 100;
        int totalPoints = 99999;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] deleteCount = null;
        int[] updateCount = null;
        int TOTALUPDATECOUNT = totalQA;
        boolean pass = true;
        String IN1 = "BATCHTBL5";
        String IN2 = null;

	    String longstr = "a";

		System.out.println("JDBC big col batch test 5..");

		while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";
		
        try
        {
			Connection conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            

	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	stmt.executeUpdate("drop table if exists " + ISTBL + " cascade");
	        stmt.executeUpdate("create table " + ISTBL + "(a int not null, b varchar(200000), c int, d varchar(40000), primary key(a))");
	
	        insert = "insert into " + ISTBL + " values(?,?,?,?)";
	        pStmt = conn.prepareStatement(insert);
	
	        startTime = System.currentTimeMillis();
	
	        int points = totalPoints;
	        int toggle = 1;
	        for (int j = 1; j <= totalQA; j++) {
	        	if (toggle == 1)
	        	{
	        		JOB = "QA";
	        		DEPT = "JDBC";
	        	}
	        	else if (toggle == 2)
	        	{
	        		JOB = longstr;
	        		DEPT = "ODBC";
	        	}
	        	else
	        	{
	        		JOB = "Whatever";
	        		DEPT = "DBADMIN";
	        	}
	
	        	pStmt.setInt(1, j);
	        	pStmt.setString(2, JOB);
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
    				System.out.println("JDBCBigColBatch5 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
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
        		System.out.println("JDBCBigColBatch5: 1 : ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT + " but was " + updateRowCount);
        		pass = false;
        	}

        	//DELETE BATCH
	        //create table " + ISTBL + "(a int not null, b varchar(200000), c int, d varchar(40000), primary key(a))");
	        delete = "delete from " + ISTBL + " where b = ?";
	        pStmt = conn.prepareStatement(delete);
	        pStmt.setString(1, longstr);
	        pStmt.addBatch();
	        pStmt.setString(1, "Whatever");
	        pStmt.addBatch();

	        deleteCount = pStmt.executeBatch();
    		for (int i = 0; i < deleteCount.length; i++) {
    			if ((deleteCount[i] != -2) && (deleteCount[i] != 1)) {
    				pass = false;
    				System.out.println("JDBCBigColBatch5 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + deleteCount[i]);
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
        		System.out.println("JDBCBigColBatch5: 2 : ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT + " but was " + updateRowCount);
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
    				System.out.println("JDBCBigColBatch5 failed : Col d should have JDBC in row " + rowNo);
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
        	fail("Exception in JDBCBigColBatch5.." + e.getMessage());
        	System.out.println(e.getMessage());
        } finally {
        	if (pass)
        		System.out.println("JDBCBigColBatch5 :  Passed");
        	else
        		System.out.println("JDBCBigColBatch5 : FAILED");
        	assertEquals(true, pass);
        }
	}

	/* Test varchar col size 200k, batch upsert/update, multiple >32k varchar cols, 200k long data */
	@Test
	public void JDBCBigColBatch6() throws InterruptedException, SQLException 
	{
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTBL6";
        int totalQA = 100;
        int totalPoints = 99999;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] updateCount = null;
        int TOTALUPDATECOUNT = 100;
        boolean pass = true;
        String IN1 = "BATCHTBL6";
        String IN2 = null;

	    String longstr = "a";

		System.out.println("JDBC big col batch test 6..");

		while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";
		
        try
        {
			Connection conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            

	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	stmt.executeUpdate("drop table if exists " + ISTBL + " cascade");
	        stmt.executeUpdate("create table " + ISTBL + "(a int not null, b varchar(200000), c int, d varchar(40000), primary key(a))");
	
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
    				System.out.println("JDBCBigColBatch6 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
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
        		System.out.println("JDBCBigColBatch6: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
        		pass = false;
        	}

        	//UPDATE BATCH
	        //create table " + ISTBL + "(a int not null, b varchar(200000), c int, d varchar(40000), primary key(a))");
	        update = "update " + ISTBL + " set b = ? where d = ?";
	        pStmt = conn.prepareStatement(update);
	        pStmt.setString(1, longstr);
	        pStmt.setString(2, "JDBC");
	        pStmt.addBatch();
	        pStmt.setString(1, longstr);
	        pStmt.setString(2, "ODBC");
	        pStmt.addBatch();
	        pStmt.setString(1, longstr);
	        pStmt.setString(2, "DBADMIN");
	        pStmt.addBatch();

	        updateCount = pStmt.executeBatch();
    		for (int i = 0; i < updateCount.length; i++) {
    			if ((updateCount[i] != -2) && (updateCount[i] != 1)) {
    				pass = false;
    				System.out.println("JDBCBigColBatch6 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + updateCount[i]);
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
        		System.out.println("JDBCBigColBatch6: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
        		pass = false;
        	}

        	// Verify the updated rows
        	stmt = conn.createStatement();
        	int totalI = 0;
        	rs = stmt.executeQuery("select * from " + IN1);
        	int rowNo = 0;
    		while (rs.next()) {
    			rowNo++;
    			if (!rs.getString(2).equals(longstr))
    				System.out.println("JDBCBigColBatch6 failed : Col d should have 200k long string in row " + rowNo);
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
        	fail("Exception in JDBCBigColBatch6.." + e.getMessage());
        	System.out.println(e.getMessage());
        } finally {
        	if (pass)
        		System.out.println("JDBCBigColBatch6 :  Passed");
        	else
        		System.out.println("JDBCBigColBatch6 : FAILED");
        	assertEquals(true, pass);
        }
	}
	
	/* DUPLICATE ROWS, NEED NAR SUPPORT
	@Test
	public void JDBCBigColBatch5() throws InterruptedException, SQLException 
	{
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTBL5";
        int totalQA = 2;
        int totalPoints = 99999;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] updateCount = null;
        int TOTALUPDATECOUNT = 1;
        boolean pass = true;
        String IN1 = "BATCHTBL5";
        String IN2 = null;
	    String longstr = "a";

		System.out.println("JDBC varchar 200k batch test 5..");

		while (longstr.length() < (200000-2))
        	longstr = longstr + "a";
	    longstr = longstr + "ZZ";

        try
        {
			Connection conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();            

	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	stmt.executeUpdate("drop table if exists " + ISTBL + " cascade");
	        stmt.executeUpdate("create table " + ISTBL + "(a int not null, b varchar(200000), c char(200), d varchar(40000), primary key(a))");
	
	        insert = "insert into " + ISTBL + " values(?,?,?,?)";
	        pStmt = conn.prepareStatement(insert);
	
	        startTime = System.currentTimeMillis();
	
	        pStmt.setInt(1, 1);
	        pStmt.setString(2, longstr);
	        pStmt.setString(3, "cccccZZ");
	        pStmt.setString(4, "bbbbbZZ");
	        pStmt.addBatch();
	        
	        //Duplicate row
	        pStmt.setInt(1, 1);
	        pStmt.setString(2, longstr);
	        pStmt.setString(3, "cccccZZ");
	        pStmt.setString(4, "bbbbbZZ");
	        pStmt.addBatch();

	       insertCount = pStmt.executeBatch();
    		for (int i = 0; i < insertCount.length; i++) {
    			if ((insertCount[i] != -2) && (insertCount[i] != 1)) {
    				pass = false;
    				System.out.println("JDBC varchar 200k batch test 5 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
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
        		System.out.println("JDBC varchar 200k batch test 5: ERROR: Expecting updateRowCount to be " + TOTALUPDATECOUNT);
        		pass = false;
        	}

        	// Verify the inserted rows
        	stmt = conn.createStatement();
        	int totalI = 0;
        	rs = stmt.executeQuery("select * from " + IN1);
        	int rowNo = 0;
    		while (rs.next()) {
    			totalI++;
    			rowNo++;
    			if (!rs.getString(2).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k batch test 5 failed : ZZ at the end missing in row " + rowNo);
    			if (!rs.getString(4).endsWith("ZZ"))
    				System.out.println("JDBC varchar 200k batch test 5 failed : ZZ at the end missing in row " + rowNo);
    		}
    		rs.close();
        	if (totalI != TOTALUPDATECOUNT) {
        		pass = false;
        		System.out.println("JDBC varchar 200k batch test 5 : ERROR: Expecting total of " + totalI + " rows inserted");
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
        } finally {
        	if (pass)
        		System.out.println("Big Col Batch5 :  Passed");
        	else
        		System.out.println("Big Col Batch5 : FAILED");
        	assertEquals(true, pass);
        }
	}
/*/
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

