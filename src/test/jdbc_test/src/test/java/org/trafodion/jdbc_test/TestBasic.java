/*
# @@@ START COPYRIGHT @@@   
#   
# (C) Copyright 2013 Hewlett-Packard Development Company, L.P.   
#   
#  Licensed under the Apache License, Version 2.0 (the "License");   
#  you may not use this file except in compliance with the License.   
#  You may obtain a copy of the License at   
#   
#      http://www.apache.org/licenses/LICENSE-2.0   
#   
#  Unless required by applicable law or agreed to in writing, software   
#  distributed under the License is distributed on an "AS IS" BASIS,   
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   
#  See the License for the specific language governing permissions and   
#  limitations under the License.   
#   
# @@@ END COPYRIGHT @@@   
*/ 

import org.junit.Test;
import static org.junit.Assert.*;

import java.sql.*;
import java.io.*;
import java.lang.*;

public class TestBasic 
{
	@Test
	public void JDBCBasic1() throws InterruptedException, SQLException
	{
//		System.out.println("JDBCUpdate..");
        //int randomNumber = ( int )( Math.random() * 9999 );
	  	//String tableName="qatabupd" + randomNumber;
		String tableName="qatabupd";
	    try{
	     	Connection  connection = Utils.getUserConnection();
	        Statement stmt = connection.createStatement();            
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	        try{
	           	 stmt.executeUpdate("drop table " + tableName);
	        }catch (SQLException e) {}
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b char(20), c smallint, d integer, primary key(a))");
            stmt.executeUpdate("insert into " + tableName + " values(1, 'Moe', 100, 223), (2, 'Curly', 934, 444), (3, 'Larry', 1000, 999), (4, 'Moe', 12, 333), (5, 'Moe', 98, 987), (6, 'Moe', 54, 212)");
	        String updqry = "update " + tableName + " set b = ? where c > ?";              
	        long startTime = System.currentTimeMillis();
	        PreparedStatement pstmt = connection.prepareStatement(updqry);
	        //pstmt.setInt(1,554);
	        pstmt.setString(1,"Moe");
	        pstmt.setInt(2, 890);
         	assertEquals("Rows updated", 2, pstmt.executeUpdate());
	        long endTime = System.currentTimeMillis();
	        long elapsedTime = (endTime - startTime)/1000; //in seconds
   	   		stmt.executeUpdate("drop table " + tableName);
            connection.close();
			System.out.println("JDBCUpdate : Passed");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in test JDBCUpdate.." + ex.getMessage());
	 	}
	}
	
	@Test
	public void JDBCBasic2() throws InterruptedException, SQLException
	{
//		System.out.println("JDBCSelectCount*..");
        //int randomNumber = ( int )( Math.random() * 9999 );
	  	//String tableName="qatabups" + randomNumber;
		String tableName="qatabups";
	    try{
	     	Connection  connection = Utils.getUserConnection();
	        Statement stmt = connection.createStatement();            
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	        try{
	           	 stmt.executeUpdate("drop table " + tableName);
	        }catch (SQLException e) {}
	        long startTime = System.currentTimeMillis();
	        stmt.executeUpdate("create table " + tableName + "(a int not null, b char(20), c smallint, d integer, primary key(a))");
            stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 100, 223)");
            stmt.executeUpdate("upsert into " + tableName + " values(2, 'Curly', 34, 444)");
            stmt.executeUpdate("upsert into " + tableName + " values(3, 'Larry', 100, 999)");
            stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 12, 333)");
            stmt.executeUpdate("upsert into " + tableName + " values(2, 'Moe', 98, 987)");
            stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 54, 212)");
            ResultSet rs = stmt.executeQuery("select count(*) from " + tableName);
         	rs.next();
         	assertEquals("Upsert row count", 3, rs.getInt(1));
         	rs.close();
	        long endTime = System.currentTimeMillis();
	        long elapsedTime = (endTime - startTime)/1000; //in seconds
   	   		stmt.executeUpdate("drop table " + tableName);
            connection.close();
			System.out.println("JDBCSelectCount* : Passed");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in test JDBCSelectCount*.." + ex.getMessage());
	 	}
	}

	@Test
	public void JDBCBasic3() throws InterruptedException, SQLException 
	{
//		System.out.println("JDBCMeta..");
        //int randomNumber = ( int )( Math.random() * 9999 );
	  	//String tableName="qatabmeta" + randomNumber;
		String tableName="qatabmeta";
		try {
		    Connection conn = Utils.getUserConnection();
		    Statement stmt = conn.createStatement();
    		stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	  		try {
	            stmt.executeUpdate("drop table " + tableName);
	  		} catch (SQLException e){}
            stmt.executeUpdate("create table " + tableName + " (c1 char(20), c2 smallint, c3 integer)");
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 100, 223), ('Curly', 34, 444), ('Larry', 100, 999), ('Moe', 12, 333), ('Moe', 98, 987), ('Moe', 54, 212)");
            ResultSet rs = stmt.executeQuery("select * from " + tableName);
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
         	rs.close();
   	   		stmt.executeUpdate("drop table " + tableName);
         	conn.close();
			System.out.println("JDBCMeta : Passed");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in test JDBCMeta.." + ex.getMessage());
		}
    }

	@Test
	public void JDBCBasic4 () throws InterruptedException, SQLException
	{
//		System.out.println("JDBCDelete..");
		try{
			Connection  connection = Utils.getUserConnection();
			Statement stmt = connection.createStatement();            
			stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	        //int randomNumber = ( int )( Math.random() * 9999 );
		  	//String tableName="qatabdel" + randomNumber;
			String tableName="qatabdel";
			try{
				stmt.executeUpdate("drop table " + tableName);
			} catch (SQLException e) {}
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
			      
   	   		stmt.executeUpdate("drop table  " + tableName);
			connection.close();
			System.out.println("JDBCDelete : Passed");
		} catch (Exception ex) {
            ex.printStackTrace();
			fail("Exception in test JDBCDelete.." + ex.getMessage());
		}
	}

	@Test
	public void JDBCBasic5 () throws SQLException, ClassNotFoundException 
	{
//		System.out.println("JDBCDelim..");
        //int randomNumber = ( int )( Math.random() * 9999 );
	  	//String tableName="qatabdelim" + randomNumber;
		String tableName="qatabdelim";

        try {
        	Connection conn = Utils.getUserConnection();
           	Statement stmt = conn.createStatement();
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	try{
        	   	stmt.executeUpdate("drop table " + tableName);
           	} catch (SQLException e) {}
       	   	stmt.executeUpdate("create table " + tableName + " (c0 int not null, \"value\" int, \"Value\" smallint, \"VALUE\" integer, primary key(c0))");
       	   	stmt.executeUpdate("insert into " + tableName + " values(1, 200, 300, 400)");
       	   	ResultSet rs = stmt.executeQuery("select * from " + tableName);
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
			
       	   	rs.close();
   	   		stmt.executeUpdate("drop table " + tableName);
   	   		conn.close();
   	   		System.out.println("JDBCDelim : Passed");
        } catch (Exception ex)
        {
        	ex.printStackTrace();
        	fail("Exception in test JDBCDelim.." + ex.getMessage());
        } 
	}
	
	@Test
	public void JDBCBasic6() throws SQLException, ClassNotFoundException 
	{
//		System.out.println("JDBCInsert..");
	    int rowNo = 0, rowsDeleted = 0, rowsUpdated=0;
        //int randomNumber = ( int )( Math.random() * 9999 );
	  	//String tableName="qatabins" + randomNumber;
	    String tableName="qatabins";

		try {
	        Connection conn = Utils.getUserConnection();
	        Statement stmt = conn.createStatement();
	        stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
	    	try{
	            stmt.executeUpdate("drop table " + tableName);
	    	} catch (SQLException e) {}
            stmt.executeUpdate("create table " + tableName + " (c1 char(20), c2 smallint, c3 integer)");
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 100, 223)");
            stmt.executeUpdate("insert into " + tableName + " values('Curly', 34, 444)");
            stmt.executeUpdate("insert into " + tableName + " values('Larry', 100, 999)");
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 12, 333)");
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 98, 987)");
            stmt.executeUpdate("insert into " + tableName + " values('Moe', 54, 212)");
	        ResultSet rs = stmt.executeQuery("select * from " + tableName);
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
         	rs.close();
            rowsUpdated = stmt.executeUpdate("update " + tableName + " set c2 = 8787 where c3 > 300");
         	assertEquals("Rows Updated", 4, rowsUpdated);
	        rowsDeleted = stmt.executeUpdate("delete from " + tableName + " where c2 < 200");
         	assertEquals("Rows deleted", 2, rowsDeleted);
   	   		stmt.executeUpdate("drop table " + tableName);
            conn.close();
			System.out.println("JDBCInsert : Passed");
        } catch (Exception ex)
        {
            ex.printStackTrace();
            fail("Exception in test JDBCInsert.." + ex.getMessage());
        } 
	}

	@Test
    public void TestBasic7() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBCUpsert..");
        Connection conn = null;
        Statement stmt = null;
        ResultSet rs = null;
        String sql = null;
        int rowNo = 0, rowsDeleted = 0, rowsUpdated=0, rowsUpserted = 0;
        String tableName="qatabU";
        String tableName2="qatabU2";

        try {
        	conn = Utils.getUserConnection();
           	stmt = conn.createStatement();
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	try{
            	stmt.executeUpdate("drop table " + tableName);
           	} catch (SQLException e) {}
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");
           	stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 100, 223)");
           	stmt.executeUpdate("upsert into " + tableName + " values(2, 'Curly', 34, 444)");
           	stmt.executeUpdate("upsert into " + tableName + " values(3, 'Larry', 100, 999)");
           	stmt.executeUpdate("upsert into " + tableName + " values(1, 'Moe', 12, 333)");
           	stmt.executeUpdate("upsert into " + tableName + " values(2, 'Moe', 98, 987)");
           	stmt.executeUpdate("upsert into " + tableName + " values(3, 'Moe', 54, 212)");
           	rs = stmt.executeQuery("select * from " + tableName);
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
        	rs.close();
        	
           	try{
            	stmt.executeUpdate("drop table " + tableName2);
           	} catch (SQLException e) {}
            stmt.executeUpdate("create table " + tableName2 + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");
            rowsUpserted = stmt.executeUpdate("upsert into " + tableName2 + " select * from " + tableName);
        	assertEquals("Rows upserted", (Integer)3, (Integer)rowsUpserted);
  	       	conn.close();
			System.out.println("JDBCUpsert : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBCUpsert.." + ex.getMessage());
       } 
     }

	@Test
    public void TestBasic8() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Catalogs..");
        Connection conn = null;
        Statement stmt = null;
        ResultSet rs = null;
        String sql = null;
        int rowNo = 0, rowsDeleted = 0, rowsUpdated=0, rowsUpserted = 0;

        try {
        	conn = Utils.getUserConnection();
           	stmt = conn.createStatement();
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	DatabaseMetaData dbmd = conn.getMetaData();
           	rs = dbmd.getCatalogs();
           	while(rs.next()) {
           		//System.out.println(rs.getString(1));
            	assertEquals("Traf catalog", "TRAFODION", rs.getString(1));
           	}
  	       	conn.close();
			System.out.println("JDBC get Catalogs : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get Catalogs.." + ex.getMessage());
       } 
     }
	@Test
    public void TestBasic9() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Table Types..");
        Connection conn = null;
        Statement stmt = null;
        ResultSet rs = null;
        String sql = null;
        int rowNo = 0, rowsDeleted = 0, rowsUpdated=0, rowsUpserted = 0;
        String[] tabTypes = {"SYSTEM TABLE", "TABLE", "VIEW"};

        try {
        	conn = Utils.getUserConnection();
           	stmt = conn.createStatement();
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	DatabaseMetaData dbmd = conn.getMetaData();
           	rs = dbmd.getTableTypes();
           	int i = 0;
           	while(rs.next()) {
           		//System.out.println(rs.getString(1));
            	assertEquals("Traf Table types", tabTypes[i], rs.getString(1));
            	i++;
           	}
  	       	conn.close();
			System.out.println("JDBC Get Table types : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get Tables.." + ex.getMessage());
       } 
     }
	@Test
    public void TestBasic10() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Columns..");
        Connection conn = null;
        Statement stmt = null;
        ResultSet rs = null;
        String sql = null;
        String tableName = "TBLGETCOLS";
        int expCols = 4;
        String[] strexpCols = 
        {
        		"Column 1:TRAFODION.T4QA.TBLGETCOLS.C0 Datatype: 4, INTEGER",
        		"Column 2:TRAFODION.T4QA.TBLGETCOLS.C1 Datatype: 1, CHAR",
        		"Column 3:TRAFODION.T4QA.TBLGETCOLS.C2 Datatype: 5, SMALLINT",
        		"Column 4:TRAFODION.T4QA.TBLGETCOLS.C3 Datatype: 4, INTEGER"
        };

        try {
        	conn = Utils.getUserConnection();
           	stmt = conn.createStatement();
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	try{
            	stmt.executeUpdate("drop table " + tableName);
           	} catch (SQLException e) {}
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
           	rs = dbmd.getColumns("TRAFODION", "T4QA", "TBLGETCOLS", "%");
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		//System.out.println("Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6));
       		    String strCol = "Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6);
            	assertEquals("getColumns" + i, strexpCols[i-1], strCol);
       		}
			//System.out.println("Number of columns: " + i);
        	assertEquals("Number of Columns", expCols, i);
  	       	conn.close();
			System.out.println("JDBC Get Columns : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get Columns.." + ex.getMessage());
       } 
     }

	@Test
    public void TestBasic11() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Columns 2..");
        Connection conn = null;
        Statement stmt = null;
        ResultSet rs = null;
        String sql = null;
        String tableName = "TBLGETCOLS2";
        int expCols = 4;
        String[] strexpCols = 
        {
        		"Column 1:TRAFODION.T4QA.TBLGETCOLS2.C0 Datatype: 4, INTEGER",
        		"Column 2:TRAFODION.T4QA.TBLGETCOLS2.C1 Datatype: 1, CHAR",
        		"Column 3:TRAFODION.T4QA.TBLGETCOLS2.C2 Datatype: 5, SMALLINT",
        		"Column 4:TRAFODION.T4QA.TBLGETCOLS2.C3 Datatype: 4, INTEGER"
        };

        try {
        	conn = Utils.getUserConnection();
           	stmt = conn.createStatement();
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	try{
            	stmt.executeUpdate("drop table " + tableName);
           	} catch (SQLException e) {}
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
           	rs = dbmd.getColumns(null, null, tableName, null);
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		//System.out.println("Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6));
       		    String strCol = "Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6);
            	assertEquals("getColumns" + i, strexpCols[i-1], strCol);
       		}
			//System.out.println("Number of columns: " + i);
        	assertEquals("Number of Columns", expCols, i);
  	       	conn.close();
			System.out.println("JDBC Get Columns 2 : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get Columns 2.." + ex.getMessage());
       } 
     }
	
	@Test
    public void TestBasic12() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Columns 3..");
        Connection conn = null;
        Statement stmt = null;
        ResultSet rs = null;
        String sql = null;
        String tableName = "TBLGETCOLS3";
        int expCols = 4;
        String[] strexpCols = 
        {
        		"Column 1:TRAFODION.T4QA.TBLGETCOLS3.C0 Datatype: 4, INTEGER",
        		"Column 2:TRAFODION.T4QA.TBLGETCOLS3.C1 Datatype: 1, CHAR",
        		"Column 3:TRAFODION.T4QA.TBLGETCOLS3.C2 Datatype: 5, SMALLINT",
        		"Column 4:TRAFODION.T4QA.TBLGETCOLS3.C3 Datatype: 4, INTEGER"
        };

        try {
        	conn = Utils.getUserConnection();
           	stmt = conn.createStatement();
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	try{
            	stmt.executeUpdate("drop table " + tableName);
           	} catch (SQLException e) {}
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
           	rs = dbmd.getColumns(Utils.catalog, Utils.schema, tableName, null);
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		//System.out.println("Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6));
       		    String strCol = "Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6);
            	assertEquals("getColumns" + i, strexpCols[i-1], strCol);
       		}
			//System.out.println("Number of columns: " + i);
        	assertEquals("Number of Columns", expCols, i);
  	       	conn.close();
			System.out.println("JDBC Get Columns 3 : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get Columns 3.." + ex.getMessage());
       } 
     }
	
	@Test
    public void TestBasic13() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Columns 4..");
        Connection conn = null;
        Statement stmt = null;
        ResultSet rs = null;
        String sql = null;
        String tableName = "TBLGETCOLS4";
        int expCols = 4;
        String[] strexpCols = 
        {
        		"Column 1:TRAFODION.T4QA.TBLGETCOLS4.C0 Datatype: 4, INTEGER",
        		"Column 2:TRAFODION.T4QA.TBLGETCOLS4.C1 Datatype: 1, CHAR",
        		"Column 3:TRAFODION.T4QA.TBLGETCOLS4.C2 Datatype: 5, SMALLINT",
        		"Column 4:TRAFODION.T4QA.TBLGETCOLS4.C3 Datatype: 4, INTEGER"
        };

        try {
        	conn = Utils.getUserConnection();
           	stmt = conn.createStatement();
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	try{
            	stmt.executeUpdate("drop table " + tableName);
           	} catch (SQLException e) {}
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
           	rs = dbmd.getColumns(conn.getCatalog(), Utils.props.getProperty("schema"), tableName, null);
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		//System.out.println("Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6));
       		    String strCol = "Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6);
            	assertEquals("getColumns" + i, strexpCols[i-1], strCol);
       		}
			//System.out.println("Number of columns: " + i);
        	assertEquals("Number of Columns", expCols, i);
  	       	conn.close();
			System.out.println("JDBC Get Columns 4 : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get Columns 4.." + ex.getMessage());
       } 
     }
	
	@Test
    public void TestBasic14() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Columns 5..");
        Connection conn = null;
        Statement stmt = null;
        ResultSet rs = null;
        String sql = null;
        String tableName = "TBLGETCOLS5";
        int expCols = 1;
        String[] strexpCols = 
        {
        		"Column 1:TRAFODION.T4QA.TBLGETCOLS5.C0 Datatype: 4, INTEGER",
        		"Column 2:TRAFODION.T4QA.TBLGETCOLS5.C1 Datatype: 1, CHAR",
        		"Column 3:TRAFODION.T4QA.TBLGETCOLS5.C2 Datatype: 5, SMALLINT",
        		"Column 4:TRAFODION.T4QA.TBLGETCOLS5.C3 Datatype: 4, INTEGER"
        };

        try {
        	conn = Utils.getUserConnection();
           	stmt = conn.createStatement();
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	try{
            	stmt.executeUpdate("drop table " + tableName);
           	} catch (SQLException e) {}
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
           	rs = dbmd.getColumns(conn.getCatalog(), Utils.props.getProperty("schema"), tableName, "C0");
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		//System.out.println("Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6));
       		    String strCol = "Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6);
            	assertEquals("getColumns" + i, strexpCols[i-1], strCol);
       		}
			//System.out.println("Number of columns: " + i);
        	assertEquals("Number of Columns", expCols, i);
  	       	conn.close();
			System.out.println("JDBC Get Columns 5 : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get Columns 5.." + ex.getMessage());
       } 
     }
	
	@Test
    public void TestBasic14a() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Columns delimited..");
        Connection conn = null;
        Statement stmt = null;
        ResultSet rs = null;
        String sql = null;
        String tableName = "\"TblGetColsD\"";
        int expCols = 4;
        String[] strexpCols = 
        {
        		"Column 1:TRAFODION.T4QA.TblGetColsD.ColA Datatype: 4, INTEGER",
        		"Column 2:TRAFODION.T4QA.TblGetColsD.ColB Datatype: 1, CHAR",
        		"Column 3:TRAFODION.T4QA.TblGetColsD.ColC Datatype: 5, SMALLINT",
        		"Column 4:TRAFODION.T4QA.TblGetColsD.ColD Datatype: 4, INTEGER"
        };

        try {
        	conn = Utils.getUserConnection();
           	stmt = conn.createStatement();
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	try{
            	stmt.executeUpdate("drop table " + tableName);
           	} catch (SQLException e) {}
           	stmt.executeUpdate("create table " + tableName + " (\"ColA\" int not null, \"ColB\" char(20), \"ColC\" smallint, \"ColD\" integer, primary key(\"ColA\"))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
           	rs = dbmd.getColumns(conn.getCatalog(), Utils.props.getProperty("schema"), tableName, "%");
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		//System.out.println("Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6));
       		    String strCol = "Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6);
            	assertEquals("getColumns" + i, strexpCols[i-1], strCol);
       		}
			//System.out.println("Number of columns: " + i);
        	assertEquals("Number of Columns", expCols, i);
  	       	conn.close();
			System.out.println("JDBC Get Columns delimited : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get Columns delimited.." + ex.getMessage());
       } 
     }

	@Test
    public void TestBasic15() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Tables..");
        Connection conn = null;
        Statement stmt = null;
        ResultSet rs = null;
        String sql = null;
        String tableName = "TBLGETTBLS";
        String expTab = "Table 1:TRAFODION.T4QA.TBLGETTBLS - TABLE";
        int countTab = 1;

        try {
        	conn = Utils.getUserConnection();
           	stmt = conn.createStatement();
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	try{
            	stmt.executeUpdate("drop table " + tableName);
           	} catch (SQLException e) {}
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
           	rs = dbmd.getTables(conn.getCatalog(), Utils.props.getProperty("schema"), tableName, null);
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		//System.out.println("Table " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + " - " + rs.getString(4));
       		    String actTab = "Table " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + " - " + rs.getString(4);
            	assertEquals("Get tables", expTab, actTab);
       		}
			//System.out.println("Number of tables: " + i);
        	assertEquals("Number of tables", countTab, i);
  	       	conn.close();
			System.out.println("JDBC Get Tables : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get Tables.." + ex.getMessage());
       } 
     }

	@Test
    public void TestBasic15a() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Tables delimited..");
        Connection conn = null;
        Statement stmt = null;
        ResultSet rs = null;
        String sql = null;
        String tableName = "\"TblGetTbls\"";
        String expTab = "Table 1:TRAFODION.T4QA.TblGetTbls - TABLE";
        int countTab = 1;

        try {
        	conn = Utils.getUserConnection();
           	stmt = conn.createStatement();
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	try{
            	stmt.executeUpdate("drop table " + tableName);
           	} catch (SQLException e) {}
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
           	rs = dbmd.getTables(conn.getCatalog(), Utils.props.getProperty("schema"), tableName, null);
           	int i = 0;
       		while(rs.next()){
       		    i++;
       		    String actTab = "Table " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + " - " + rs.getString(4);
            	assertEquals("Get tables", expTab, actTab);
       		}
			//System.out.println("Number of tables: " + i);
        	assertEquals("Number of tables", countTab, i);
  	       	conn.close();
			System.out.println("JDBC Get Tables delimited : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get Tables delimited.." + ex.getMessage());
       } 
     }

	@Test
    public void TestBasic16() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Type Info..");
        Connection conn = null;
        Statement stmt = null;
        ResultSet rs = null;
        String sql = null;
        String tableName = "TBLGETTBLS";
        String[] strExpTypes = 
        {
	        "1: TYPE_NAME - BIGINT DATA_TYPE - -5 PRECISION - 19",
	        "2: TYPE_NAME - BIGINT SIGNED DATA_TYPE - -5 PRECISION - 19",
	        "3: TYPE_NAME - CHAR DATA_TYPE - 1 PRECISION - 32000",
	        "4: TYPE_NAME - NUMERIC DATA_TYPE - 2 PRECISION - 128",
	        "5: TYPE_NAME - NUMERIC SIGNED DATA_TYPE - 2 PRECISION - 128",
	        "6: TYPE_NAME - NUMERIC UNSIGNED DATA_TYPE - 2 PRECISION - 128",
	        "7: TYPE_NAME - DECIMAL DATA_TYPE - 3 PRECISION - 18",
	        "8: TYPE_NAME - DECIMAL SIGNED DATA_TYPE - 3 PRECISION - 18",
	        "9: TYPE_NAME - DECIMAL UNSIGNED DATA_TYPE - 3 PRECISION - 18",
	        "10: TYPE_NAME - INTEGER DATA_TYPE - 4 PRECISION - 10",
	        "11: TYPE_NAME - INTEGER SIGNED DATA_TYPE - 4 PRECISION - 10",
	        "12: TYPE_NAME - INTEGER UNSIGNED DATA_TYPE - 4 PRECISION - 10",
	        "13: TYPE_NAME - SMALLINT DATA_TYPE - 5 PRECISION - 5",
	        "14: TYPE_NAME - SMALLINT SIGNED DATA_TYPE - 5 PRECISION - 5",
	        "15: TYPE_NAME - SMALLINT UNSIGNED DATA_TYPE - 5 PRECISION - 5",
	        "16: TYPE_NAME - FLOAT DATA_TYPE - 6 PRECISION - 15",
	        "17: TYPE_NAME - REAL DATA_TYPE - 7 PRECISION - 7",
	        "18: TYPE_NAME - DOUBLE PRECISION DATA_TYPE - 8 PRECISION - 15",
	        "19: TYPE_NAME - VARCHAR DATA_TYPE - 12 PRECISION - 32000",
	        "20: TYPE_NAME - DATE DATA_TYPE - 91 PRECISION - 10",
	        "21: TYPE_NAME - TIME DATA_TYPE - 92 PRECISION - 8",
	        "22: TYPE_NAME - TIMESTAMP DATA_TYPE - 93 PRECISION - 26",
	        "23: TYPE_NAME - INTERVAL DATA_TYPE - 101 PRECISION - 0",
	        "24: TYPE_NAME - INTERVAL DATA_TYPE - 102 PRECISION - 0",
	        "25: TYPE_NAME - INTERVAL DATA_TYPE - 103 PRECISION - 0",
	        "26: TYPE_NAME - INTERVAL DATA_TYPE - 104 PRECISION - 0",
	        "27: TYPE_NAME - INTERVAL DATA_TYPE - 105 PRECISION - 0",
	        "28: TYPE_NAME - INTERVAL DATA_TYPE - 106 PRECISION - 0",
	        "29: TYPE_NAME - INTERVAL DATA_TYPE - 107 PRECISION - 0",
	        "30: TYPE_NAME - INTERVAL DATA_TYPE - 108 PRECISION - 0",
	        "31: TYPE_NAME - INTERVAL DATA_TYPE - 109 PRECISION - 0",
	        "32: TYPE_NAME - INTERVAL DATA_TYPE - 110 PRECISION - 0",
	        "33: TYPE_NAME - INTERVAL DATA_TYPE - 111 PRECISION - 0",
	        "34: TYPE_NAME - INTERVAL DATA_TYPE - 112 PRECISION - 0",
	        "35: TYPE_NAME - INTERVAL DATA_TYPE - 113 PRECISION - 0"
    	};
        int expTypes = 35;

        try {
        	conn = Utils.getUserConnection();
           	stmt = conn.createStatement();
           	DatabaseMetaData dbmd = conn.getMetaData();
           	rs = dbmd.getTypeInfo();
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		//System.out.println(i + ": " + "TYPE_NAME - " + rs.getString(1) + " DATA_TYPE - " + rs.getInt(2) + " PRECISION - " + rs.getInt(3));
       		    String strType = i + ": " + "TYPE_NAME - " + rs.getString(1) + " DATA_TYPE - " + rs.getInt(2) + " PRECISION - " + rs.getInt(3);
            	assertEquals("getColumns" + i, strExpTypes[i-1], strType);
       		}
        	assertEquals("Number of datatypes", expTypes, i);
			//System.out.println("Number of datatypes: " + i);
  	       	conn.close();
			System.out.println("JDBC Get Type Info : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get Type Info.." + ex.getMessage());
       } 
     }
	
	@Test
    public void TestBasic17() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get User Connection..");
        Connection conn = null;

        try {
        	conn = Utils.getUserConnection();
			System.out.println("Connected using getUserConnection");
  	       	conn.close();
			System.out.println("JDBC Get User Connection : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get User Connection.." + ex.getMessage());
       } 
     }
	
	@Test
    public void TestBasic18() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Properties Connection..");
        Connection conn = null;

        try {
        	conn = Utils.getPropertiesConnection();
			System.out.println("Connected using getPropertiesConnection");
  	       	conn.close();
			System.out.println("JDBC Get Properties Connection : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get Properties Connection.." + ex.getMessage());
       } 
     }

	@Test
    public void TestBasic19() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Primary Keys..");
        Connection conn = null;
        Statement stmt = null;
        ResultSet rs = null;
        String sql = null;
        String tableName = "TBLGETPKS";
        String[] strExpKeys = 
        	{
		        "1:TRAFODION.T4QA.TBLGETPKS.C0",
		        "2:TRAFODION.T4QA.TBLGETPKS.C2",
		        "3:TRAFODION.T4QA.TBLGETPKS.C3"
        	};
        int expKeys = 3;

        try {
        	conn = Utils.getUserConnection();
           	stmt = conn.createStatement();
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	try{
            	stmt.executeUpdate("drop table " + tableName);
           	} catch (SQLException e) {}
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint not null, c3 integer not null, primary key(c0, c2, c3))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
           	rs = dbmd.getPrimaryKeys(conn.getCatalog(), Utils.props.getProperty("schema"), tableName);
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		String strActKey = rs.getShort(5) + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4);
           		assertEquals("Primary key " + i, strExpKeys[i-1], strActKey);
       		}
			assertEquals("Number of columns in PK: ", expKeys, i);
  	       	conn.close();
			System.out.println("JDBC Get Primary Keys : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get Primary Keys.." + ex.getMessage());
       } 
     }

	@Test
    public void TestBasic20() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get DB product name..");
        Connection conn = null;

        try {
        	conn = Utils.getUserConnection();
			String	prodName = conn.getMetaData().getDatabaseProductName();
			//System.out.println("DB product name : " + prodName);
			assertEquals("DB Product Name", "Trafodion", prodName);
  	       	conn.close();
			System.out.println("JDBC Get DB product name : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get DB product name.." + ex.getMessage());
       } 
     }

	@Test
    public void TestBasic21() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get DB product version..");
        Connection conn = null;

        try {
        	conn = Utils.getUserConnection();
			String	prodVer = conn.getMetaData().getDatabaseProductVersion();
			System.out.println("DB product version : " + prodVer);
			assertEquals("DB Product Version", "1.1", prodVer);
  	       	conn.close();
			System.out.println("JDBC Get DB product version : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get DB product version.." + ex.getMessage());
       } 
     }

	@Test
    public void TestBasic22() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get DB major version..");
        Connection conn = null;

        try {
        	conn = Utils.getUserConnection();
			int	majorVer = conn.getMetaData().getDatabaseMajorVersion();
			//System.out.println("DB major version : " + majorVer);
			assertEquals("DB major Version", 1, majorVer);
  	       	conn.close();
			System.out.println("JDBC Get DB major version : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get DB major version.." + ex.getMessage());
       } 
     }

	@Test
    public void TestBasic23() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get DB minor version..");
        Connection conn = null;

        try {
        	conn = Utils.getUserConnection();
			int	minorVer = conn.getMetaData().getDatabaseMinorVersion();
			//System.out.println("DB minor version : " + minorVer);
			assertEquals("DB Minor Version", 1, minorVer);
  	       	conn.close();
			System.out.println("JDBC Get DB minor version : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get DB minor version.." + ex.getMessage());
       } 
     }

	@Test
    public void TestBasic24() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get driver name..");
        Connection conn = null;

        try {
        	conn = Utils.getUserConnection();
			String driverName = conn.getMetaData().getDriverName();
			//System.out.println("Driver Name : " + driverName);
			//assertEquals("Driver Name", "org.trafodion.jdbc.t4.T4Driver", driverName);
			assertTrue("Driver Name", driverName.contains("org.trafodion.jdbc.t"));
  	       	conn.close();
			System.out.println("JDBC Get Driver Name : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get Driver Name.." + ex.getMessage());
       } 
     }

	@Test
    public void TestBasic25() throws SQLException, ClassNotFoundException 
    {
        Connection conn = null;

        try {
        	conn = Utils.getUserConnection();
			String	driverVer = conn.getMetaData().getDriverVersion();
			//System.out.println("Driver Version : " + driverVer);
			//assertTrue("Driver version", actualString.contains(wantedString));
			assertTrue("Driver version", driverVer.contains("Traf_JDBC_Type"));
			conn.close();
			System.out.println("JDBC Get Driver version : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC Get Driver version.." + ex.getMessage());
       } 
     }

	@Test
	public void JDBCBatch1() throws InterruptedException, SQLException {
        Statement stmt = null;
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTBL1";
        int totalQA = 20;
        int totalDev = 20;
        int totalMgr = 20;
        int totalPoints = 99999;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] updateCount = null;
	//int TOTALUPDATECOUNT = 60; - Calculating this instead of hardcoding.
        int TOTALUPDATECOUNT = totalQA+totalDev+totalMgr; 
        boolean pass = true;
        String IN1 = "BATCHTBL1";
        String IN2 = null;
        /*
        int[] insertCountTemplate = { -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        		-2, -2 };
        int[] updateCountTemplate = { -2 };
        */
		try{
			Connection conn = Utils.getUserConnection();

	        Utils.dropTable(conn, IN1);
	        Utils.createTable(conn, IN1);
	
	        insert = "insert into " + IN1 + " values(?,?,?,?)";
	        pStmt = conn.prepareStatement(insert);
	
	        startTime = System.currentTimeMillis();
	
	        // Insert lines with unique id
	
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
        		// Check the return status code for each row
        		/*
        		System.out.println("insertcount length = " + insertCount.length);
        		for (int i = 0; i < insertCount.length; i++) {
       				System.out.println("insertCount[" + i + "] : " + insertCount[i]);
        		}
        		*/
        		for (int i = 0; i < insertCount.length; i++) {
        			if ((insertCount[i] != -2) && (insertCount[i] != 1)) {
        				pass = false;
        				System.out.println("Batch1 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("Batch1 : ERROR: Exception in insert batch mode.....");
        		handleBatchUpdateException(bue);
        	}

        	// Check the SQLWarning
        	java.sql.SQLWarning sw = pStmt.getWarnings();
        	int totalW = 0;
        	while (sw != null) {
//        		System.out.println("SQLMessage : " + sw.getMessage());
//        		System.out.println("SQLState : " + sw.getSQLState());
//        		System.out.println("ErrorCode : " + sw.getErrorCode());
        		totalW++;
        		sw = sw.getNextWarning();
        	}
//        	System.out.println("Batch1 : Total of " + totalW + " warning retrieved!!");
        	// Check the update count
        	int updateRowCount = pStmt.getUpdateCount();
//        	System.out.println("Batch1 : Update count :" + updateRowCount);
        	if (updateRowCount != TOTALUPDATECOUNT) {
        		System.out.println("Batch1: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT);
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
        			// for (int j=1; j <= rsMD.getColumnCount(); j++)
        			// System.out.println("Column " + j + ":" +
        			// rs.getObject(j));
        		}
        		rs.close();
//        		System.out.println("Batch1 : total of " + totalI + " rows inserted");
        	}
        	if (totalI != TOTALUPDATECOUNT) {
        		pass = false;
        		System.out.println("Batch1 : ERROR: Expecting total of " + totalI + " rows inserted");
        	}
        	pStmt.close();
        	pStmt = null;

        	update = "update " + IN1 + " set points  = ? where TITLE = ? ";
        	pStmt = conn.prepareStatement(update);
        	pStmt.setInt(1, 9);
        	pStmt.setString(2, "MGR");
        	pStmt.addBatch();

 //       	System.out.println("Batch1 : Done adding update to batch.....");

        	try {
        		updateCount = pStmt.executeBatch();

        		// Check the return status code for each row
        		/*
        		System.out.println("updatecount length = " + updateCount.length);
        		for (int i = 0; i < updateCount.length; i++) {
       				System.out.println("updateCount[" + i + "] : " + updateCount[i]);
        		}
        		*/
        		for (int i = 0; i < updateCount.length; i++) {
        			//System.out.println("Batch1 : return code for " + i + "th command: " + updateCount[i]);
        			if ((updateCount[i] != -2) && (updateCount[i] != 2)) {
        				pass = false;
        				System.out.println("Batch1 : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + updateCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("Batch1 : ERROR: Exception in update batch mode.....");
        		handleBatchUpdateException(bue);
        	}

        	// Check the update count
        	updateRowCount = pStmt.getUpdateCount();
//        	System.out.println("Batch1 : Update count :" + updateRowCount);
        	if (updateRowCount != totalMgr) {
        		System.out.println("Batch1: ERROR: Expecting updateRowCount to be " + totalMgr);
        		pass = false;
        	}

        	int ret = 0;
        	rs = stmt.executeQuery("select POINTS from " + IN1 + " where TITLE = 'MGR'");
        	if (rs != null) {
        		while (rs.next()) {
        			ret = rs.getInt("POINTS");
//        			System.out.println("Batch1: points for MGR is : " + ret);
        			if (ret != 9) {
        				pass = false;
        				System.out.println("Batch1: ERROR: points for MGR is not updated: " + ret);
        			}
        		}
        		rs.close();
        	}
        	pStmt.close();
        	stmt.close();
        	conn.close();

        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	System.out.println(e.getMessage());
        } finally {
        	/**/
        	if (pass)
        		System.out.println("Batch1 :  Passed");
        	else
        		System.out.println("Batch1 : FAILED");
        	/**/
        	assertEquals(true, pass);
        }
	}

	@Test
	public void JDBCBatch() throws InterruptedException, SQLException {
        Statement stmt = null;
        ResultSet rs = null;
        String DEPT = null;
        String insert = null;
        String update = null;
        PreparedStatement pStmt = null;
        String ISTBL = "BATCHTBL";
        int totalQA = 2000;
        int totalDev = 2000;
        int totalMgr = 2000;
        int totalPoints = 999999;
        long startTime = 0;
        long endTime = 0;
        int[] insertCount = null;
        int[] updateCount = null;
        //int TOTALUPDATECOUNT = 6000; - CALCULATING THIS INSTEAD OF HARDCODING
        int TOTALUPDATECOUNT = totalQA+totalDev+totalMgr;
        boolean pass = true;
        String IN1 = "BATCHTBL";
        String IN2 = null;
        /*
        int[] insertCountTemplate = { -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        		-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        		-2, -2 };
        int[] updateCountTemplate = { -2 };
        */
		try{
			Connection conn = Utils.getUserConnection();

	        Utils.dropTable(conn, IN1);
	        Utils.createTable(conn, IN1);
	
	        insert = "insert into " + IN1 + " values(?,?,?,?)";
	        pStmt = conn.prepareStatement(insert);
	
	        startTime = System.currentTimeMillis();
	
	        // Insert lines with unique id
	
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
        		// Check the return status code for each row
        		/*
        		System.out.println("insertcount length = " + insertCount.length);
        		for (int i = 0; i < insertCount.length; i++) {
       				System.out.println("insertCount[" + i + "] : " + insertCount[i]);
        		}
        		*/
        		for (int i = 0; i < insertCount.length; i++) {
        			//if (insertCount[i] != insertCountTemplate[i]) 
        			if ((insertCount[i] != -2) && (insertCount[i] != 1))
        			{
        				pass = false;
        				System.out.println("Batch : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + insertCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("Batch : ERROR: Exception in insert batch mode.....");
        		handleBatchUpdateException(bue);
        	}

        	// Check the SQLWarning
        	java.sql.SQLWarning sw = pStmt.getWarnings();
        	int totalW = 0;
        	while (sw != null) {
//        		System.out.println("SQLMessage : " + sw.getMessage());
//        		System.out.println("SQLState : " + sw.getSQLState());
//        		System.out.println("ErrorCode : " + sw.getErrorCode());
        		totalW++;
        		sw = sw.getNextWarning();
        	}
//        	System.out.println("Batch : Total of " + totalW + " warning retrieved!!");
        	// Check the update count
        	int updateRowCount = pStmt.getUpdateCount();
//        	System.out.println("Batch : Update count :" + updateRowCount);
        	if (updateRowCount != TOTALUPDATECOUNT) {
        		System.out.println("Batch: ERROR: Expecting updateRowCount to be " + TOTALUPDATECOUNT + " but it is " + updateRowCount);
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
        			// for (int j=1; j <= rsMD.getColumnCount(); j++)
        			// System.out.println("Column " + j + ":" +
        			// rs.getObject(j));
        		}
        		rs.close();
        		System.out.println("Batch : total of " + totalI + " rows inserted");
        	}
        	if (totalI != TOTALUPDATECOUNT) {
        		pass = false;
        		System.out.println("Batch : ERROR: Expected rows inserted : " + totalI + " actual: " + TOTALUPDATECOUNT);
        	}
        	pStmt.close();
        	pStmt = null;

        	update = "update " + IN1 + " set points  = ? where TITLE = ? ";
        	pStmt = conn.prepareStatement(update);
        	pStmt.setInt(1, 9);
        	pStmt.setString(2, "MGR");
        	pStmt.addBatch();

 //       	System.out.println("Batch : Done adding update to batch.....");

        	try {
        		updateCount = pStmt.executeBatch();
        		/*
        		System.out.println("updatecount length = " + updateCount.length);
        		for (int i = 0; i < updateCount.length; i++) {
       				System.out.println("updateCount[" + i + "] : " + updateCount[i]);
        		}
        		 */
        		// Check the return status code for each row
        		for (int i = 0; i < updateCount.length; i++) {
        			//System.out.println("Batch : return code for " + i + "th command: " + updateCount[i]);
        			if ((updateCount[i] != -2) && (updateCount[i] != 2)) {
        				pass = false;
        				System.out.println("Batch : ERROR: Unexpected return code (should be -2 or 1) for " + i + "th command: " + updateCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("Batch : ERROR: Exception in update batch mode.....");
        		handleBatchUpdateException(bue);
        	}

        	// Check the update count
        	updateRowCount = pStmt.getUpdateCount();
//        	System.out.println("Batch : Update count :" + updateRowCount);
        	if (updateRowCount != totalMgr) {
        		System.out.println("Batch: ERROR: Expecting updateRowCount to be " + totalMgr);
        		pass = false;
        	}

        	int ret = 0;
        	rs = stmt.executeQuery("select POINTS from " + IN1 + " where TITLE = 'MGR'");
        	if (rs != null) {
        		while (rs.next()) {
        			ret = rs.getInt("POINTS");
//        			System.out.println("Batch: points for MGR is : " + ret);
        			if (ret != 9) {
        				pass = false;
        				System.out.println("Batch: ERROR: points for MGR is not updated: " + ret);
        			}
        		}
        		rs.close();
        	}
        	pStmt.close();
        	stmt.close();
        	conn.close();

        }// end of try
        catch (Exception e) {
        	pass = false;
        	e.printStackTrace();
        	System.out.println(e.getMessage());
        } finally {
        	/**/
        	if (pass)
        		System.out.println("Batch :  Passed");
        	else
        		System.out.println("Batch : FAILED");
        	/**/
        	assertEquals(true, pass);
        }
	}

	@Test
	public void JDBCBasic26() throws InterruptedException, SQLException {
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
        int TOTALUPDATECOUNT = 10000;
        boolean pass = true;
        String IN1 = "TABALL";
        String IN2 = null;
        int insertCountTemplate = -2;
        int[] updateCountTemplate = { -2 };
		try{
			Connection conn = Utils.getUserConnection();
           	stmt = conn.createStatement();
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	try{
            	stmt.executeUpdate("drop table " + IN1);
           	} catch (SQLException e) {}
           	
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
	
	        // Insert lines with unique id
	

        	try {
    	        for (int j = 1; j <= total; j++) {
    	        	pStmt.setInt(1, j);
    	        	pStmt.addBatch();
    	        }
        		insertCount = pStmt.executeBatch();
        		// Check the return status code for each row
        		for (int i = 0; i < insertCount.length; i++) 
        		{
        			//if (insertCount[i] != insertCountTemplate[i]) 
        			if ((insertCount[i] != -2) && (insertCount[i] != 1))
        			{
        				pass = false;
        				System.out.println("Batch All : ERROR: Expecting return code for " + i + "th command: -2 or 1, but got : " + insertCount[i]);
        			}
        		}
        	} catch (BatchUpdateException bue) {
        		System.out.println("Batch All : ERROR: Exception in insert batch mode.....");
            	bue.printStackTrace();
            	System.out.println(bue.getMessage());
        		handleBatchUpdateException(bue);
        	}

        	// Check the SQLWarning
        	java.sql.SQLWarning sw = pStmt.getWarnings();
        	int totalW = 0;
        	while (sw != null) {
//        		System.out.println("SQLMessage : " + sw.getMessage());
//        		System.out.println("SQLState : " + sw.getSQLState());
//        		System.out.println("ErrorCode : " + sw.getErrorCode());
        		totalW++;
        		sw = sw.getNextWarning();
        	}
//        	System.out.println("Batch All : Total of " + totalW + " warning retrieved!!");
        	// Check the update count
        	int updateRowCount = pStmt.getUpdateCount();
//        	System.out.println("Batch All : Update count :" + updateRowCount);
        	if (updateRowCount != TOTALUPDATECOUNT) {
        		System.out.println("Batch All: ERROR: Expecting updateRowCount to be "	+ TOTALUPDATECOUNT + " but got " + updateRowCount);
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
        			// for (int j=1; j <= rsMD.getColumnCount(); j++)
        			// System.out.println("Column " + j + ":" +
        			// rs.getObject(j));
        		}
        		rs.close();
//        		System.out.println("Batch All : total of " + totalI + " rows inserted");
        	}
        	if (totalI != TOTALUPDATECOUNT) {
        		pass = false;
        		System.out.println("Batch All : ERROR: Expecting total of " + totalI + " rows inserted");
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
        	/**/
        	if (pass)
        		System.out.println("Batch All :  Passed");
        	else
        		System.out.println("Batch All : FAILED");
        	/**/
        	assertEquals(true, pass);
        }
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

