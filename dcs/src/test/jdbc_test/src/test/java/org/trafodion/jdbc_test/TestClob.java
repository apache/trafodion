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

import static org.junit.Assert.assertEquals;

import java.sql.Clob;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.Statement;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

public class TestClob {
	private static final String tableName = "CLOBTEST";
	private static final String colName = "C1";
	private static final String strCreateTable = "CREATE TABLE " + Utils.schema + "." + tableName + "(" + colName + " CLOB);";
	private static final String strDropTable = "DROP TABLE " + Utils.schema + "." + tableName;

	private static Connection _conn = null;

	@BeforeClass
	public static void doTestSuiteSetup() throws Exception {
		try {
			_conn = DriverManager.getConnection(Utils.url, Utils.usr, Utils.pwd);
			Statement stmt = _conn.createStatement();

			// use CQD to enable CLOB support
			stmt.execute("CQD TRAF_CLOB_AS_VARCHAR 'OFF'");
			stmt.execute(strCreateTable);
		} catch (Exception e) {
			System.out.println(e.getMessage());
			e.printStackTrace();
		}
	}

	@Test
	public void readCLOBData() throws Exception {
		// USE built-in function stringtolob to insert data
		String data = "this is a CLOB String";

		try (Statement stmt = _conn.createStatement()) {
			stmt.executeUpdate("INSERT INTO " + Utils.schema + "." + tableName + " (" + colName + ") values (stringtolob('" + data + "'))");

			ResultSet rs = stmt.executeQuery("SELECT * FROM " + Utils.schema + "." + tableName);
			int rowNum = 0;
			while (rs.next()) {
				rowNum++;
				Clob clob = rs.getClob(colName);
				assertEquals("clob data ", data, clob.getSubString(1, (int) clob.length()));
			}
			assertEquals("Row count read from server", 1, rowNum);
		} catch (Exception e) {
            System.out.println(e.getMessage());
            e.printStackTrace();
        }
	}

	@AfterClass
	public static void cleanTable() throws Exception {
		try (Statement stmt = _conn.createStatement()) {
			stmt.execute(strDropTable);
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
}
