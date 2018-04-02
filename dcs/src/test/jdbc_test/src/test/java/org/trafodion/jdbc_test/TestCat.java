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

public class TestCat
{	
	/* Test Get Catalogs */
	@Test
    public void TestBasic8() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Catalogs..");
        String sql = null;
        int rowNo = 0, rowsDeleted = 0, rowsUpdated=0, rowsUpserted = 0;

        try
        (
        	Connection conn = Utils.getUserConnection();
           	Statement stmt = conn.createStatement();
        )
        {
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	DatabaseMetaData dbmd = conn.getMetaData();
           	//try - THIS SYNTAX DOES NOT WORK FOR CATALOG APIs.
           	//(
           		ResultSet rs = dbmd.getCatalogs();
           	//)
           	//{
	           	while(rs.next()) {
	           		//System.out.println(rs.getString(1));
	            	assertEquals("Traf catalog", "TRAFODION", rs.getString(1));
	           	}
           	//}
			System.out.println("JDBC TestBasic8 - Get Catalogs : Passed");
        } catch (Exception ex)
        {
           	ex.printStackTrace();
           	fail("Exception in test JDBC TestBasic8 - Get Catalogs.." + ex.getMessage());
        } 
		System.out.println("JDBC TestBasic8 - Get Catalogs : Done");
     }

	/* Test Get table Types */
	@Test
    public void TestBasic9() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Table Types..");
        String sql = null;
        int rowNo = 0, rowsDeleted = 0, rowsUpdated=0, rowsUpserted = 0;
        String[] tabTypes = {"SYSTEM TABLE", "TABLE", "VIEW"};

        try
        (
        	Connection conn = Utils.getUserConnection();
           	Statement stmt = conn.createStatement();
        )
        {
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
           	DatabaseMetaData dbmd = conn.getMetaData();
       		ResultSet rs = dbmd.getTableTypes();
           	int i = 0;
           	while(rs.next()) {
           		//System.out.println(rs.getString(1));
            	assertEquals("Traf Table types", tabTypes[i], rs.getString(1));
            	i++;
           	}
			System.out.println("JDBC TestBasic9 - Get Table types : Passed");
        } catch (Exception ex)
        {
           	ex.printStackTrace();
           	fail("Exception in test JDBC TestBasic9 - Get Tables.." + ex.getMessage());
        } 
		System.out.println("JDBC TestBasic9 - Get Table types : Done");
    }
	
	/* Test Get Columns using % for all columns */
	@Test
    public void TestBasic10() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Columns..");
        String sql = null;
        String tableName = "TBLGETCOLS";
        int expCols = 4;
        String[] strexpCols = 
        {
        		"Column 1:TRAFODION." + Utils.schema + ".TBLGETCOLS.C0 Datatype: 4, INTEGER",
        		"Column 2:TRAFODION." + Utils.schema + ".TBLGETCOLS.C1 Datatype: 1, CHAR",
        		"Column 3:TRAFODION." + Utils.schema + ".TBLGETCOLS.C2 Datatype: 5, SMALLINT",
        		"Column 4:TRAFODION." + Utils.schema + ".TBLGETCOLS.C3 Datatype: 4, INTEGER"
        };

        try 
        (
        	Connection conn = Utils.getUserConnection();
           	Statement stmt = conn.createStatement();
        )
        {
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
       		ResultSet rs = dbmd.getColumns("TRAFODION", Utils.schema, "TBLGETCOLS", "%");
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		//System.out.println("Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6));
       		    String strCol = "Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6);
            	assertEquals("getColumns" + i, strexpCols[i-1], strCol);
       		}
			//System.out.println("Number of columns: " + i);
        	assertEquals("Number of Columns", expCols, i);
			System.out.println("JDBC TestBasic10 - Get Columns : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC TestBasic10 - Get Columns.." + ex.getMessage());
       } 
		System.out.println("JDBC TestBasic10 - Get Columns : Done");
     }

	/* Test Get columns with null for catalog, schema and colname */
	@Test
    public void TestBasic11() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Columns 2..");
        String sql = null;
        String tableName = "TBLGETCOLS2";
        int expCols = 4;
        String[] strexpCols = 
        {
        		"Column 1:TRAFODION." + Utils.schema + ".TBLGETCOLS2.C0 Datatype: 4, INTEGER",
        		"Column 2:TRAFODION." + Utils.schema + ".TBLGETCOLS2.C1 Datatype: 1, CHAR",
        		"Column 3:TRAFODION." + Utils.schema + ".TBLGETCOLS2.C2 Datatype: 5, SMALLINT",
        		"Column 4:TRAFODION." + Utils.schema + ".TBLGETCOLS2.C3 Datatype: 4, INTEGER"
        };

        try
        (
        	Connection conn = Utils.getUserConnection();
           	Statement stmt = conn.createStatement();
        )
        {
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
       		ResultSet rs = dbmd.getColumns(null, null, tableName, null);
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		//System.out.println("Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6));
       		    String strCol = "Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6);
            	assertEquals("getColumns" + i, strexpCols[i-1], strCol);
       		}
			//System.out.println("Number of columns: " + i);
        	assertEquals("Number of Columns", expCols, i);
			System.out.println("JDBC TestBasic11 - Get Columns 2 : Passed");
       } catch (Exception ex)
       {
           	ex.printStackTrace();
           	fail("Exception in test JDBC TestBasic11 - Get Columns 2.." + ex.getMessage());
       } 
		System.out.println("JDBC TestBasic11 - Get Columns 2 : Done");
     }
	
	/* Test Get columns by passing calaog, schema, table and null for colname */
	@Test
    public void TestBasic12() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Columns 3..");
        String sql = null;
        String tableName = "TBLGETCOLS3";
        int expCols = 4;
        String[] strexpCols = 
        {
        		"Column 1:TRAFODION." + Utils.schema + ".TBLGETCOLS3.C0 Datatype: 4, INTEGER",
        		"Column 2:TRAFODION." + Utils.schema + ".TBLGETCOLS3.C1 Datatype: 1, CHAR",
        		"Column 3:TRAFODION." + Utils.schema + ".TBLGETCOLS3.C2 Datatype: 5, SMALLINT",
        		"Column 4:TRAFODION." + Utils.schema + ".TBLGETCOLS3.C3 Datatype: 4, INTEGER"
        };

        try
        (
        	Connection conn = Utils.getUserConnection();
           	Statement stmt = conn.createStatement();
        )
        {
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
			ResultSet rs = dbmd.getColumns(Utils.catalog, Utils.schema, tableName, null);
		   	int i = 0;
			while(rs.next()){
			    i++;
		   		//System.out.println("Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6));
			    String strCol = "Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6);
		    	assertEquals("getColumns" + i, strexpCols[i-1], strCol);
			}
			//System.out.println("Number of columns: " + i);
			assertEquals("Number of Columns", expCols, i);
			System.out.println("JDBC TestBasic12 - Get Columns 3 : Passed");
        } catch (Exception ex)
        {
           	ex.printStackTrace();
           	fail("Exception in test JDBC TestBasic12 - Get Columns 3.." + ex.getMessage());
        } 
		System.out.println("JDBC TestBasic12 - Get Columns 3 : Done");
    }
	
	/* Test Get columns by passing current catalog, literal schema/tablename and null colname */
	@Test
    public void TestBasic13() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Columns 4..");
        String sql = null;
        String tableName = "TBLGETCOLS4";
        int expCols = 4;
        String[] strexpCols = 
        {
        		"Column 1:TRAFODION." + Utils.schema + ".TBLGETCOLS4.C0 Datatype: 4, INTEGER",
        		"Column 2:TRAFODION." + Utils.schema + ".TBLGETCOLS4.C1 Datatype: 1, CHAR",
        		"Column 3:TRAFODION." + Utils.schema + ".TBLGETCOLS4.C2 Datatype: 5, SMALLINT",
        		"Column 4:TRAFODION." + Utils.schema + ".TBLGETCOLS4.C3 Datatype: 4, INTEGER"
        };

        try
        (
        	Connection conn = Utils.getUserConnection();
           	Statement stmt = conn.createStatement();
        )
        {
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
       		ResultSet rs = dbmd.getColumns(conn.getCatalog(), Utils.schema, tableName, null);
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		//System.out.println("Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6));
       		    String strCol = "Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6);
            	assertEquals("getColumns" + i, strexpCols[i-1], strCol);
       		}
			//System.out.println("Number of columns: " + i);
        	assertEquals("Number of Columns", expCols, i);
			System.out.println("JDBC TestBasic13 - Get Columns 4 : Passed");
        } catch (Exception ex)
       	{
           	ex.printStackTrace();
           	fail("Exception in test JDBC TestBasic13 - Get Columns 4.." + ex.getMessage());
       	} 
		System.out.println("JDBC TestBasic13 - Get Columns 4 : Done");
    }
	
	/* Test Get columns by passing current catalog, literal schema/table/colname */
	@Test
    public void TestBasic14() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Columns 5..");
        String sql = null;
        String tableName = "TBLGETCOLS5";
        int expCols = 1;
        String[] strexpCols = 
        {
        		"Column 1:TRAFODION." + Utils.schema + ".TBLGETCOLS5.C0 Datatype: 4, INTEGER",
        		"Column 2:TRAFODION." + Utils.schema + ".TBLGETCOLS5.C1 Datatype: 1, CHAR",
        		"Column 3:TRAFODION." + Utils.schema + ".TBLGETCOLS5.C2 Datatype: 5, SMALLINT",
        		"Column 4:TRAFODION." + Utils.schema + ".TBLGETCOLS5.C3 Datatype: 4, INTEGER"
        };

        try
        (
        	Connection conn = Utils.getUserConnection();
           	Statement stmt = conn.createStatement();
        )
        {
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
       		ResultSet rs = dbmd.getColumns(conn.getCatalog(), Utils.schema, tableName, "C0");
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		//System.out.println("Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6));
       		    String strCol = "Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6);
            	assertEquals("getColumns" + i, strexpCols[i-1], strCol);
       		}
			//System.out.println("Number of columns: " + i);
        	assertEquals("Number of Columns", expCols, i);
			System.out.println("JDBC TestBasic14 - Get Columns 5 : Passed");
        } catch (Exception ex)
       	{
           	ex.printStackTrace();
           	fail("Exception in test JDBC TestBasic14 - Get Columns 5.." + ex.getMessage());
       	} 
		System.out.println("JDBC TestBasic14 - Get Columns 5 : Done");
     }
	
	/* Test Get COlumsn for delimited column names */
	@Test
    public void TestBasic14a() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Columns delimited..");
        String sql = null;
        String tableName = "\"TblGetColsD\"";
        int expCols = 4;
        String[] strexpCols = 
        {
        		"Column 1:TRAFODION." + Utils.schema + ".TblGetColsD.ColA Datatype: 4, INTEGER",
        		"Column 2:TRAFODION." + Utils.schema + ".TblGetColsD.ColB Datatype: 1, CHAR",
        		"Column 3:TRAFODION." + Utils.schema + ".TblGetColsD.ColC Datatype: 5, SMALLINT",
        		"Column 4:TRAFODION." + Utils.schema + ".TblGetColsD.ColD Datatype: 4, INTEGER"
        };

        try
        (
        	Connection conn = Utils.getUserConnection();
           	Statement stmt = conn.createStatement();
        )
        {
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
           	stmt.executeUpdate("create table " + tableName + " (\"ColA\" int not null, \"ColB\" char(20), \"ColC\" smallint, \"ColD\" integer, primary key(\"ColA\"))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
       		ResultSet rs = dbmd.getColumns(conn.getCatalog(), Utils.schema, tableName, "%");
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		//System.out.println("Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6));
       		    String strCol = "Column " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4) + " Datatype: " + rs.getInt(5) + ", " + rs.getString(6);
            	assertEquals("getColumns" + i, strexpCols[i-1], strCol);
       		}
			//System.out.println("Number of columns: " + i);
        	assertEquals("Number of Columns", expCols, i);
			System.out.println("JDBC TestBasic14a - Get Columns delimited : Passed");
        } catch (Exception ex)
       	{
           	ex.printStackTrace();
           	fail("Exception in test JDBC TestBasic14a - Get Columns delimited.." + ex.getMessage());
       	} 
		System.out.println("JDBC TestBasic14a - Get Columns delimited : Done");
    }

	/* Test get tables passing current catalog, literal schema/tablename */
	@Test
    public void TestBasic15() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Tables..");
        String sql = null;
        String tableName = "TBLGETTBLS";
        String expTab = "Table 1:TRAFODION." + Utils.schema + ".TBLGETTBLS - TABLE";
        int countTab = 1;

        try
        (
        	Connection conn = Utils.getUserConnection();
           	Statement stmt = conn.createStatement();
        )
        {
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
       		ResultSet rs = dbmd.getTables(conn.getCatalog(), Utils.schema, tableName, null);
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		//System.out.println("Table " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + " - " + rs.getString(4));
       		    String actTab = "Table " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + " - " + rs.getString(4);
            	assertEquals("Get tables", expTab, actTab);
       		}
			//System.out.println("Number of tables: " + i);
        	assertEquals("Number of tables", countTab, i);
			System.out.println("JDBC TestBasic15 - Get Tables : Passed");
        } catch (Exception ex)
       	{
           	ex.printStackTrace();
           	fail("Exception in test JDBC TestBasic15 - Get Tables.." + ex.getMessage());
       	} 
		System.out.println("JDBC TestBasic15 - Get Tables : Done");
    }

	/* Test get tables with delimited tablename */
	@Test
    public void TestBasic15a() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Tables delimited..");
        String sql = null;
        String tableName = "\"TblGetTbls\"";
        String expTab = "Table 1:TRAFODION." + Utils.schema + ".TblGetTbls - TABLE";
        int countTab = 1;

        try
        (
        	Connection conn = Utils.getUserConnection();
           	Statement stmt = conn.createStatement();
        )
        {
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
       		ResultSet rs = dbmd.getTables(conn.getCatalog(), Utils.schema, tableName, null);
           	int i = 0;
       		while(rs.next()){
       		    i++;
       		    String actTab = "Table " + i + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + " - " + rs.getString(4);
            	assertEquals("Get tables", expTab, actTab);
       		}
			//System.out.println("Number of tables: " + i);
        	assertEquals("Number of tables", countTab, i);
			System.out.println("JDBC TestBasic15a - Get Tables delimited : Passed");
        } catch (Exception ex)
       	{
           	ex.printStackTrace();
           	fail("Exception in test JDBC TestBasic15a - Get Tables delimited.." + ex.getMessage());
       	} 
		System.out.println("JDBC TestBasic15a - Get Tables delimited : Done");
    }

	/* Test GetTypeInfo */
	@Test
    public void TestBasic16() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Type Info..");
        String sql = null;
        String tableName = "TBLGETTBLS";
        String[] strExpTypes = 
        {
	        "1: TYPE_NAME - NCHAR VARYING DATA_TYPE - -9 PRECISION - 32000",
	        "2: TYPE_NAME - NCHAR DATA_TYPE - -8 PRECISION - 32000",
	        "3: TYPE_NAME - TINYINT DATA_TYPE - -6 PRECISION - 3",
	        "4: TYPE_NAME - TINYINT SIGNED DATA_TYPE - -6 PRECISION - 3",
	        "5: TYPE_NAME - TINYINT UNSIGNED DATA_TYPE - -6 PRECISION - 3",
	        "6: TYPE_NAME - BIGINT DATA_TYPE - -5 PRECISION - 19",
	        "7: TYPE_NAME - BIGINT SIGNED DATA_TYPE - -5 PRECISION - 19",
	        "8: TYPE_NAME - BIGINT UNSIGNED DATA_TYPE - -5 PRECISION - 20",
	        "9: TYPE_NAME - LONG VARCHAR DATA_TYPE - -1 PRECISION - 2000",
	        "10: TYPE_NAME - CHAR DATA_TYPE - 1 PRECISION - 32000",
	        "11: TYPE_NAME - NUMERIC DATA_TYPE - 2 PRECISION - 128",
	        "12: TYPE_NAME - NUMERIC SIGNED DATA_TYPE - 2 PRECISION - 128",
	        "13: TYPE_NAME - NUMERIC UNSIGNED DATA_TYPE - 2 PRECISION - 128",
	        "14: TYPE_NAME - DECIMAL DATA_TYPE - 3 PRECISION - 18",
	        "15: TYPE_NAME - DECIMAL SIGNED DATA_TYPE - 3 PRECISION - 18",
	        "16: TYPE_NAME - DECIMAL UNSIGNED DATA_TYPE - 3 PRECISION - 18",
	        "17: TYPE_NAME - INTEGER DATA_TYPE - 4 PRECISION - 9",
	        "18: TYPE_NAME - INTEGER SIGNED DATA_TYPE - 4 PRECISION - 9",
	        "19: TYPE_NAME - INTEGER UNSIGNED DATA_TYPE - 4 PRECISION - 10",
	        "20: TYPE_NAME - SMALLINT DATA_TYPE - 5 PRECISION - 5",
	        "21: TYPE_NAME - SMALLINT SIGNED DATA_TYPE - 5 PRECISION - 5",
	        "22: TYPE_NAME - SMALLINT UNSIGNED DATA_TYPE - 5 PRECISION - 5",
	        "23: TYPE_NAME - FLOAT DATA_TYPE - 6 PRECISION - 15",
	        "24: TYPE_NAME - REAL DATA_TYPE - 7 PRECISION - 7",
	        "25: TYPE_NAME - DOUBLE PRECISION DATA_TYPE - 8 PRECISION - 15",
	        "26: TYPE_NAME - VARCHAR DATA_TYPE - 12 PRECISION - 32000",
	        "27: TYPE_NAME - DATE DATA_TYPE - 91 PRECISION - 10",
	        "28: TYPE_NAME - TIME DATA_TYPE - 92 PRECISION - 8",
	        "29: TYPE_NAME - TIMESTAMP DATA_TYPE - 93 PRECISION - 26",
	        "30: TYPE_NAME - INTERVAL DATA_TYPE - 101 PRECISION - 0",
	        "31: TYPE_NAME - INTERVAL DATA_TYPE - 102 PRECISION - 0",
	        "32: TYPE_NAME - INTERVAL DATA_TYPE - 103 PRECISION - 0",
	        "33: TYPE_NAME - INTERVAL DATA_TYPE - 104 PRECISION - 0",
	        "34: TYPE_NAME - INTERVAL DATA_TYPE - 105 PRECISION - 0",
	        "35: TYPE_NAME - INTERVAL DATA_TYPE - 106 PRECISION - 0",
	        "36: TYPE_NAME - INTERVAL DATA_TYPE - 107 PRECISION - 0",
	        "37: TYPE_NAME - INTERVAL DATA_TYPE - 108 PRECISION - 0",
	        "38: TYPE_NAME - INTERVAL DATA_TYPE - 109 PRECISION - 0",
	        "39: TYPE_NAME - INTERVAL DATA_TYPE - 110 PRECISION - 0",
	        "40: TYPE_NAME - INTERVAL DATA_TYPE - 111 PRECISION - 0",
	        "41: TYPE_NAME - INTERVAL DATA_TYPE - 112 PRECISION - 0",
	        "42: TYPE_NAME - INTERVAL DATA_TYPE - 113 PRECISION - 0"
    	};
        int expTypes = 42;

        try
        (
        	Connection conn = Utils.getUserConnection();
           	Statement stmt = conn.createStatement();
        )
        {
           	DatabaseMetaData dbmd = conn.getMetaData();
       		ResultSet rs = dbmd.getTypeInfo();
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		//System.out.println(i + ": " + "TYPE_NAME - " + rs.getString(1) + " DATA_TYPE - " + rs.getInt(2) + " PRECISION - " + rs.getInt(3));
       		    String strType = i + ": " + "TYPE_NAME - " + rs.getString(1) + " DATA_TYPE - " + rs.getInt(2) + " PRECISION - " + rs.getInt(3);
            	assertEquals("getColumns" + i, strExpTypes[i-1], strType);
       		}
        	assertEquals("Number of datatypes", expTypes, i);
			//System.out.println("Number of datatypes: " + i);
			System.out.println("JDBC TestBasic16 - Get Type Info : Passed");
        } catch (Exception ex)
       	{
           	ex.printStackTrace();
           	fail("Exception in test JDBC TestBasic16 - Get Type Info.." + ex.getMessage());
       	} 
		System.out.println("JDBC TestBasic16 - Get Type Info : Done");
    }
	
	/* Test Get Primary Keys */
	@Test
    public void TestBasic19() throws SQLException, ClassNotFoundException 
    {
//		System.out.println("JDBC Get Primary Keys..");
        String sql = null;
        String tableName = "TBLGETPKS";
        String[] strExpKeys = 
        	{
		        "1:TRAFODION." + Utils.schema + ".TBLGETPKS.C0",
		        "2:TRAFODION." + Utils.schema + ".TBLGETPKS.C2",
		        "3:TRAFODION." + Utils.schema + ".TBLGETPKS.C3"
        	};
        int expKeys = 3;

        try
        (
        	Connection conn = Utils.getUserConnection();
           	Statement stmt = conn.createStatement();
        )
        {
           	stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("drop table if exists " + tableName + " cascade");
           	stmt.executeUpdate("create table " + tableName + " (c0 int not null, c1 char(20), c2 smallint not null, c3 integer not null, primary key(c0, c2, c3))");           	
           	DatabaseMetaData dbmd = conn.getMetaData();
       		ResultSet rs = dbmd.getPrimaryKeys(conn.getCatalog(), Utils.schema, tableName);
           	int i = 0;
       		while(rs.next()){
       		    i++;
           		String strActKey = rs.getShort(5) + ":" + rs.getString(1) + "." + rs.getString(2) + "." + rs.getString(3) + "." + rs.getString(4);
           		assertEquals("Primary key " + i, strExpKeys[i-1], strActKey);
       		}
			assertEquals("Number of columns in PK: ", expKeys, i);
			System.out.println("JDBC TestBasic19 - Get Primary Keys : Passed");
        } catch (Exception ex)
       	{
           	ex.printStackTrace();
           	fail("Exception in test JDBC TestBasic19 - Get Primary Keys.." + ex.getMessage());
       	} 
		System.out.println("JDBC TestBasic19 - Get Primary Keys : Done");
    }

	/* Test Get DB Product name */
	@Test
    public void TestBasic20()
        throws SQLException, ClassNotFoundException
    {
        Object obj = null;
        try
        {
            Connection connection = Utils.getUserConnection();
            String s = connection.getMetaData().getDatabaseProductName();
            assertTrue("DB Product Name", s.contains("Traf"));
            connection.close();
            System.out.println("JDBC Get DB product name : Passed");
        }
        catch(Exception exception)
        {
            exception.printStackTrace();
            fail((new StringBuilder()).append("Exception in test JDBC Get DB product name..").append(exception.getMessage()).toString());
        }
    }

	/* Test Get DB Product Version */
    @Test
    public void TestBasic21()
        throws SQLException, ClassNotFoundException
    {
        Object obj = null;
        try
        {
            Connection connection = Utils.getUserConnection();
            String db = Utils.dbmaj_version + "." + Utils.dbmin_version;
            String s = connection.getMetaData().getDatabaseProductVersion();
            System.out.println((new StringBuilder()).append("DB product version : ").append(s).toString());
            assertEquals("DB Product Version", db, s);
            connection.close();
            System.out.println("JDBC Get DB product version : Passed");
        }
        catch(Exception exception)
        {
            exception.printStackTrace();
            fail((new StringBuilder()).append("Exception in test JDBC Get DB product version..").append(exception.getMessage()).toString());
        }
    }

	/* Test Get DB Major version */
    @Test
    public void TestBasic22()
        throws SQLException, ClassNotFoundException
    {
        Object obj = null;
        try
        {
            Connection connection = Utils.getUserConnection();
            int maj = Integer.parseInt(Utils.dbmaj_version);
            int i = connection.getMetaData().getDatabaseMajorVersion();
            assertEquals("DB major Version", maj, i);
            connection.close();
            System.out.println("JDBC Get DB major version : Passed");
        }
        catch(Exception exception)
        {
            exception.printStackTrace();
            fail((new StringBuilder()).append("Exception in test JDBC Get DB major version..").append(exception.getMessage()).toString());
        }
    }

	/* Test Get DB Minor version */
    @Test
    public void TestBasic23()
        throws SQLException, ClassNotFoundException
    {
        Object obj = null;
        try
        {
            Connection connection = Utils.getUserConnection();
            int min = Integer.parseInt(Utils.dbmin_version);
            int i = connection.getMetaData().getDatabaseMinorVersion();
            assertEquals("DB Minor Version", min, i);
            connection.close();
            System.out.println("JDBC Get DB minor version : Passed");
        }
        catch(Exception exception)
        {
            exception.printStackTrace();
            fail((new StringBuilder()).append("Exception in test JDBC Get DB minor version..").append(exception.getMessage()).toString());
        }
    }

	/* Test Get Driver Name */
    @Test
    public void TestBasic24()
        throws SQLException, ClassNotFoundException
    {
        Object obj = null;
        try
        {
            Connection connection = Utils.getUserConnection();
            String s = connection.getMetaData().getDriverName();
            assertTrue("Driver Name", s.contains("org.trafodion.jdbc.t"));
            connection.close();
            System.out.println("JDBC Get Driver Name : Passed");
        }
        catch(Exception exception)
        {
            exception.printStackTrace();
            fail((new StringBuilder()).append("Exception in test JDBC Get Driver Name..").append(exception.getMessage()).toString());
        }
    }

	/* Test Get Driver Version */
    @Test
    public void TestBasic25()
        throws SQLException, ClassNotFoundException
    {
        Object obj = null;
        try
        {
            Connection connection = Utils.getUserConnection();
            String s = connection.getMetaData().getDriverVersion();
            assertTrue("Driver version", s.contains("Traf_JDBC_Type"));
            connection.close();
            System.out.println("JDBC Get Driver version : Passed");
        }
        catch(Exception exception)
        {
            exception.printStackTrace();
            fail((new StringBuilder()).append("Exception in test JDBC Get Driver version..").append(exception.getMessage()).toString());
        }
    }
}

