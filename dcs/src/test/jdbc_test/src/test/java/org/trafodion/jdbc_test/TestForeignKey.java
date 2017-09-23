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

public class TestForeignKey {
	private static final String PKTABLE1 = "PKTABLE1";
	private static final String PK1 = "PK1";
	private static final String PKTABLE2 = "PKTABLE2";
	private static final String PK2 = "PK2";
	private static final String FKTABLE1 = "FKTABLE1";
	private static final String FK1 ="FK1";
	private static final String FK2 = "FK2";

	private static final String FKTABLE2 = "FKTABLE2";
	private static final String FK21 = "FK21";
	private static final String FK22 = "FK22";

    private static final String strCreatePKTABLE1Query = "CREATE TABLE IF NOT EXISTS " + Utils.schema + "." + PKTABLE1
            + "( "
			+ PK1 + " INT NOT NULL PRIMARY KEY)";
	private static final String strDropPKTABLE1Query = "DROP TABLE " + Utils.schema + "." + PKTABLE1;

    private static final String strCreatePKTABLE2Query = "CREATE TABLE IF NOT EXISTS " + Utils.schema + "." + PKTABLE2
            + "( "
			+ PK2 + " INT NOT NULL PRIMARY KEY)";
	private static final String strDropPKTABLE2Query = "DROP TABLE " + Utils.schema + "." + PKTABLE2;

    private static final String strCreateFKTABLE1Query = "CREATE TABLE IF NOT EXISTS " + Utils.schema + "." + FKTABLE1
            + "( "
			+ FK1 + " INT NOT NULL, "
			+ FK2 + " INT NOT NULL, "
			+ "FOREIGN KEY (" + FK1 + ") REFERENCES " + Utils.schema + "." + PKTABLE1 + "(" + PK1 + "), "
			+ "FOREIGN KEY (" + FK2 + ") REFERENCES " + Utils.schema + "." + PKTABLE2 + "(" + PK2 + "))";
	private static final String strDropFKTABLE1Query = "DROP TABLE " + Utils.schema + "." + FKTABLE1;

    private static final String strCreateFKTABLE2Query = "CREATE TABLE IF NOT EXISTS " + Utils.schema + "." + FKTABLE2
            + "( "
			+ FK21 + " INT NOT NULL, "
			+ FK22 + " INT NOT NULL, "
			+ "FOREIGN KEY (" + FK21 + ") REFERENCES " + Utils.schema + "." + PKTABLE1 + "(" + PK1 + "), "
			+ "FOREIGN KEY (" + FK22 + ") REFERENCES " + Utils.schema + "." + PKTABLE2 + "(" + PK2 + "))";
	private static final String strDropFKTABLE2Query = "DROP TABLE " + Utils.schema + "." + FKTABLE2;

	private static Connection _conn;

	@BeforeClass
	public static void doTestSuiteSetup() throws Exception {
        try {
            _conn = Utils.getUserConnection();
        } catch (Exception e) {
            fail("failed to create connection" + e.getMessage());
        }

        try (Statement stmt = _conn.createStatement();) {
            stmt.execute(strCreatePKTABLE1Query);
            stmt.execute(strCreatePKTABLE2Query);
            stmt.execute(strCreateFKTABLE1Query);
            stmt.execute(strCreateFKTABLE2Query);
        }
		catch (Exception e) {
            fail("failed to create table: " + e.getMessage());
		}
	}
	
	@Test
	public void testGetImportedKeys() {
		ForeignKeyInfo[] expFkInfo = {
				new ForeignKeyInfo("TRAFODION", Utils.schema, PKTABLE1, PK1, "TRAFODION", Utils.schema, FKTABLE1, FK1, (short) 1),
				new ForeignKeyInfo("TRAFODION", Utils.schema, PKTABLE2, PK2, "TRAFODION", Utils.schema, FKTABLE1, FK2, (short)1)
		};
		
		try {
			DatabaseMetaData metaData = _conn.getMetaData();
			int rowNum = 0;
            try (
                 ResultSet rs = metaData.getImportedKeys("TRAFODION", Utils.schema, FKTABLE1);
            )
            {
                while(rs.next()) {
                    compareForeignkeyWithExp("testGetImportedKeys", rowNum + 1, rs, expFkInfo[rowNum]);
                    rowNum += 1;
                }
            }
            catch (Exception e) {
                fail(e.getMessage());
            }
			assertEquals(rowNum, 2);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	@Test
	public void testGetExportedKeys() {
		ForeignKeyInfo[] expFkInfo = {
				new ForeignKeyInfo("TRAFODION", Utils.schema, PKTABLE1, PK1, "TRAFODION", Utils.schema, FKTABLE1, FK1, (short)1),
				new ForeignKeyInfo("TRAFODION", Utils.schema, PKTABLE1, PK1, "TRAFODION", Utils.schema, FKTABLE2, FK21, (short)1)
		};
		
		try {
			DatabaseMetaData metaData = _conn.getMetaData();
            int rowNum = 0;
            try (
			    ResultSet rs = metaData.getExportedKeys("TRAFODION", Utils.schema, PKTABLE1);
            )
            {
			    while(rs.next()) {
			    	compareForeignkeyWithExp("testGetExportedKeys", rowNum + 1, rs, expFkInfo[rowNum]);
			    	rowNum += 1;
			    }
            }
			assertEquals(rowNum, 2);
		} catch (Exception e) {
			e.printStackTrace();
		}		
	}
	
	@Test
	public void testGetCrossReference() {
		ForeignKeyInfo[] expFkInfo = {
				new ForeignKeyInfo("TRAFODION", Utils.schema, PKTABLE1, PK1, "TRAFODION", Utils.schema, FKTABLE1, FK1, (short)1)
		};
		
		try {
			DatabaseMetaData metaData = _conn.getMetaData();
			int rowNum = 0;
            try (
			    ResultSet rs = metaData.getCrossReference("TRAFODION", Utils.schema, PKTABLE1, "TRAFODION", Utils.schema, FKTABLE1);
            )
            {
			    while(rs.next()) {
			    	compareForeignkeyWithExp("testGetCrossReference", rowNum + 1, rs, expFkInfo[rowNum]);
			    	rowNum += 1;
			    }
            }
			assertEquals(rowNum, 1);
		} catch (Exception e) {
			e.printStackTrace();
		}		
	}

	@AfterClass
	public static void cleanTable() {
		try (Statement stmt = _conn.createStatement()){
			stmt.execute(strDropFKTABLE1Query);
			stmt.execute(strDropFKTABLE2Query);
			stmt.execute(strDropPKTABLE1Query);
			stmt.execute(strDropPKTABLE2Query);
		} catch (Exception e) {
			System.out.println(e.getMessage());
			e.printStackTrace();
		}
		
		try {
			_conn.close();
		} catch (Exception e) {
			System.out.println(e.getMessage());
			e.printStackTrace();
		}
	}
	
	private class ForeignKeyInfo {
		public String pkTableCat;
		public String pkTableSchem;
		public String pkTableName;
		public String pkColumnName;
		public String fkTableCat;
		public String fkTableSchem;
		public String fkTableName;
		public String fkColumnName;
		public short keySeq;
		
		public ForeignKeyInfo (String pkTableCat, String pkTableSchem, String pkTableName, String pkColumnName,
				String fkTableCat, String fkTableSchem, String fkTableName, String fkColumnName, 
				short keySeq) {
			this.pkTableCat = pkTableCat;
			this.pkTableSchem = pkTableSchem;
			this.pkTableName = pkTableName;
			this.pkColumnName = pkColumnName;
			this.fkTableCat = fkTableCat;
			this.fkTableSchem = fkTableSchem;
			this.fkTableName = fkTableName;
			this.fkColumnName = fkColumnName;
			this.keySeq = keySeq;
		}
	}

	private void compareForeignkeyWithExp(String methodName, int rowNum, ResultSet rs, ForeignKeyInfo fkInfo) {
		try {
			assertEquals(methodName + " rowNum " + Integer.toString(rowNum) + " PKTABLE_CAT", fkInfo.pkTableCat,
					rs.getString("PKTABLE_CAT"));
			assertEquals(methodName + " rowNum " + Integer.toString(rowNum) + " PKTABLE_SCHEM", fkInfo.pkTableSchem,
					rs.getString("PKTABLE_SCHEM"));
			assertEquals(methodName + " rowNum " + Integer.toString(rowNum) + " PKTABLE_NAME", fkInfo.pkTableName,
					rs.getString("PKTABLE_NAME"));
			assertEquals(methodName + " rowNum " + Integer.toString(rowNum) + " PKCOLUMNS_NAME", fkInfo.pkColumnName,
					rs.getString("PKCOLUMN_NAME"));
			assertEquals(methodName + " rowNum " + Integer.toString(rowNum) + " FKTABLE_CAT", fkInfo.fkTableCat,
					rs.getString("FKTABLE_CAT"));
			assertEquals(methodName + " rowNum " + Integer.toString(rowNum) + " FKTABLE_SCHEM", fkInfo.fkTableSchem,
					rs.getString("FKTABLE_SCHEM"));
			assertEquals(methodName + " rowNum " + Integer.toString(rowNum) + " FKTABLE_NAME", fkInfo.fkTableName,
					rs.getString("FKTABLE_NAME"));
			assertEquals(methodName + " rowNum " + Integer.toString(rowNum) + " FKCOLUMN_NAME", fkInfo.fkColumnName,
					rs.getString("FKCOLUMN_NAME"));
			// Skip KEY_SEQ, UPDATE_RULE, DELETE_RULE, DEFERRABILITY now , since
			// them was been supported now.

			// Skip PK_NAME and FK_NAME
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
