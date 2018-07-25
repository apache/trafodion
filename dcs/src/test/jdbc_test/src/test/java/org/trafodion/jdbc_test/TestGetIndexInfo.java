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

import java.sql.Connection;
import java.sql.DatabaseMetaData;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.Statement;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import static org.junit.Assert.*;

public class TestGetIndexInfo {
	
	private static final String INDEX_INFO_TEST_TABLE = "INDEX_INFO_TEST_TABLE";
	
	private static final String strCreateTableQuery = "CREATE TABLE " + INDEX_INFO_TEST_TABLE + "(C1 INT, C2 INT)";
	private static final String strInsertQuery = "INSERT INTO " + INDEX_INFO_TEST_TABLE + " (C1, C2) VALUES (?, ?)";
	private static final String strUpdateStatisticsQuery = "UPDATE STATISTICS FOR TABLE " + INDEX_INFO_TEST_TABLE + " ON (C1, C2)";
	private static final String strDropTableQuery = "DROP TABLE " + INDEX_INFO_TEST_TABLE;
	private static final String INDEX_C1_NAME = INDEX_INFO_TEST_TABLE + "_INDEX";
	
	private static final String strCreateIndexQuery = "CREATE INDEX " + INDEX_C1_NAME +" on " + INDEX_INFO_TEST_TABLE + "(C1)";
	
	private static Connection _conn = null;

	@BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        try {
            _conn = Utils.getUserConnection();
        } catch (Exception e) {
            fail("failed to create connection" + e.getMessage());
        }

        try (Statement stmt = _conn.createStatement()
        ) {
            stmt.execute(strCreateTableQuery);
        }
        catch (Exception e) {
            fail("failed to create the table : " + e.getMessage());
        }
        try (
    		Statement stmt = _conn.createStatement();
                PreparedStatement pstmt = _conn.prepareStatement(strInsertQuery);
        ) {
    		int[][] testValues = {
    				{1, 2},
    				{10, 3},
    				{2, 2}
    		};   

    		for (int i = 0; i < testValues.length; i++) {
    			pstmt.setInt(1, testValues[i][0]);
    			pstmt.setInt(2, testValues[i][1]);
    			pstmt.addBatch();
    		}
    		pstmt.executeBatch();

    		// create index
    		stmt.execute(strCreateIndexQuery);
    		
    		// update statistics on the table
    		stmt.execute(strUpdateStatisticsQuery);
    		
    		stmt.close();
    	}
    	catch (Exception e) {
    		System.out.println(e.getMessage());
    		e.printStackTrace();
    	} finally {
    	}
    }
	
	@Test
    public void testGetNoneUniqueIndexInfo() {
		IndexInfo[] expIndexInfo = {
				new IndexInfo("TRAFODION", "SEABASE", INDEX_INFO_TEST_TABLE, false, (String)null, (String)null, (short)0, (short)0, "C1", 0, 3, (short)0, (String)null),
				new IndexInfo("TRAFODION", "SEABASE", INDEX_INFO_TEST_TABLE, false, (String)null, (String)null, (short)0, (short)0, "C2", 0, 3, (short)0, (String)null),
				new IndexInfo("TRAFODION", "SEABASE", INDEX_INFO_TEST_TABLE, true, (String)null, INDEX_C1_NAME, (short)3, (short)0, "", 0, 0, (short)0, (String)null)
		};
		
		try {
			DatabaseMetaData meta = _conn.getMetaData();
			ResultSet indexInformation = meta.getIndexInfo(_conn.getCatalog(), null, INDEX_INFO_TEST_TABLE, false, false);
			
			int rowNum = 0;
			while(indexInformation.next()) {
				compareInfoWithExp("testGetUniqueIndexInfo", rowNum + 1, indexInformation, expIndexInfo[rowNum]);
				rowNum += 1;
			}
			assertEquals(rowNum, 3);
		} catch(Exception e) {
			System.out.println(e.getMessage());
			e.printStackTrace();
		}
    }

	@AfterClass
    public static void cleanTable() {
    	try ( Statement stmt = _conn.createStatement() ) {
    		stmt.execute(strDropTableQuery);
    	} catch(Exception e) {
    		// do nothing
    	}
    	
    	try {
    		_conn.close();
    	} catch(Exception e) {
    		
    	}
    }
	
	private class IndexInfo {
		public String dbCatalog = "TRAFODION";
		public String dbSchema = "SEABASE";
		public String dbTableName;
		public boolean dbNoneUnique = false;
	    public String dbIndexQualifier;
	    public String dbIndexName;
	    public short dbType;
	    public short dbOrdinalPosition;
	    public String dbColumnName;
	    public String dbAscOrDesc;
	    public int dbCardinality;
	    public int dbPages;
	    public String dbFilterCondition;
	    
	    public IndexInfo(String dbCatalog, String dbSchema, String dbTableName, boolean dbNoneUnique, String dbIndexQualifier, String dbIndexName,
	    		short dbType, short dbOrdinalPosition, String dbColumnName, int dbAscOrdesc, int dbCardinality, int dbPages, String dbFilterCondition) {
	    	this.dbCatalog = dbCatalog;
	    	this.dbSchema = dbSchema;
	    	this.dbTableName = dbTableName;
	    	this.dbNoneUnique = dbNoneUnique;
	    	this.dbIndexQualifier = dbIndexQualifier;
	    	this.dbIndexName = dbIndexName;
	    	this.dbType = dbType;
	    	this.dbOrdinalPosition = dbOrdinalPosition;
	    	this.dbColumnName = dbColumnName;
	    	this.dbCardinality = dbCardinality;
	    	this.dbPages = dbPages;
	    	this.dbFilterCondition = dbFilterCondition;
	    }
	}
	
	private void compareInfoWithExp(String methondName, int rowNum, ResultSet rs, IndexInfo indexInfo) {
		try {
			assertEquals(methondName + " rowNum " + Integer.toString(rowNum) + " dbCatalog ", indexInfo.dbCatalog, rs.getString("TABLE_CAT"));
			String currentSchema = _conn.getSchema();
			
			// Since we may run the test in different Schema, so compare the result to the current schema of the connection here.
			assertEquals(methondName + " rowNUm " + Integer.toString(rowNum) + " dbSchem ", currentSchema, rs.getString("TABLE_SCHEM"));
			
			assertEquals(methondName + " rowNum " + Integer.toString(rowNum) + " dbTableName ", indexInfo.dbTableName, rs.getString("TABLE_NAME"));
			assertEquals(methondName + " rowNum " + Integer.toString(rowNum) + " dbNoneUnique ", indexInfo.dbNoneUnique, rs.getBoolean("NON_UNIQUE"));
			// By the Document, dbNoneUnique will return null if the type is SQL_TABLE_STAT
			System.out.println(rs.wasNull());
			if (indexInfo.dbType == DatabaseMetaData.tableIndexStatistic)
				assertTrue(rs.wasNull());

			assertEquals(methondName + " rowNum " + Integer.toString(rowNum) + " dbIndexQualifier ", indexInfo.dbIndexQualifier, rs.getString("INDEX_QUALIFIER"));
			assertEquals(methondName + " rowNum " + Integer.toString(rowNum) + " dbIndexName ", indexInfo.dbIndexName, rs.getString("INDEX_NAME"));
			assertEquals(methondName + " rowNum " + Integer.toString(rowNum) + " dbType ", indexInfo.dbType, rs.getShort("TYPE"));
			assertEquals(methondName + " rowNum " + Integer.toString(rowNum) + " dbOridinalPosition ", indexInfo.dbOrdinalPosition, rs.getShort("ORDINAL_POSITION"));
			// By the Document, it return NULL when the type is SQL_TABLE_STAT
			if (indexInfo.dbType == DatabaseMetaData.tableIndexStatistic)
				assertTrue(rs.wasNull());

			assertEquals(methondName + " rowNum " + Integer.toString(rowNum) + " dbColumnName ", indexInfo.dbColumnName, rs.getString("COLUMN_NAME"));
			assertEquals(methondName + " rowNum " + Integer.toString(rowNum) + " dbAscOrDesc ", indexInfo.dbAscOrDesc, rs.getString("ASC_OR_DESC"));
			assertEquals(methondName + " rowNum " + Integer.toString(rowNum) + " dbCardinality ", indexInfo.dbCardinality, rs.getInt("CARDINALITY"));
			// When the type is not SQL_TABLE_STAT, dbCardinality will be NULL
			if (indexInfo.dbType != DatabaseMetaData.tableIndexStatistic)
				assertTrue(rs.wasNull());
			assertEquals(methondName + " rowNum " + Integer.toString(rowNum) + " dbPages ", indexInfo.dbPages, rs.getInt("PAGES"));
			// Since dbPages is not supported now, it always return NULL
			// here check if it is real NULL
			assertTrue(rs.wasNull());

			assertEquals(methondName + " rowNum " + Integer.toString(rowNum) + " dbFilterCondition ", indexInfo.dbFilterCondition, rs.getString("FILTER_CONDITION"));
		} catch (Exception e) {
			System.out.println(e.getMessage());
			e.printStackTrace();
		}
	}
}
