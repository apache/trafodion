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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.Reader;
import java.io.StringReader;
import java.sql.Clob;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.Statement;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

public class TestClobBatch {
	private static final String tableName = "CLOBBATCHTEST";
	private static final String strCreateTable = "CREATE TABLE " + Utils.schema + "." + tableName
			+ "(C1 int, c2 CLOB, c3 CHAR(4));";
	private static final String strDropTable = "DROP TABLE " + Utils.schema + "." + tableName;

	private static Connection _conn = null;

	private boolean assertResult() throws Exception {
		Statement stmt = null;
		ResultSet rs = null;
		try {
			stmt = _conn.createStatement();

			rs = stmt.executeQuery("select c1, c2, c3 from " + Utils.schema + "." + tableName + " order by c1");

			assertTrue(rs.next());
			assertEquals(1, rs.getInt(1));
			assertEquals("this is a clob1 object", rs.getString(2));
			assertEquals("aaaa", rs.getString(3));
			assertTrue(rs.next());

			assertEquals(2, rs.getInt(1));
			assertEquals("this is a clob2 object", rs.getString(2));
			assertEquals("aaaa", rs.getString(3));
			assertFalse(rs.next());

		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			stmt.close();
			rs.close();
		}
		return false;
	}

	@Test
	public void testSetClob() throws Exception {
		try (Statement stmt = _conn.createStatement()) {
			stmt.execute("delete from " + Utils.schema + "." + tableName);

			PreparedStatement pstmt = _conn
					.prepareStatement("insert into " + Utils.schema + "." + tableName + " values (?, ?, ?)");

			Clob clob1 = _conn.createClob();
			Clob clob2 = _conn.createClob();

			clob1.setString(1, "this is a clob1 object");
			clob2.setString(1, "this is a clob2 object");

			pstmt.setInt(1, 1);
			pstmt.setClob(2, clob1);
			pstmt.setString(3, "aaaa");
			pstmt.addBatch();
			pstmt.setInt(1, 2);
			pstmt.setClob(2, clob2);
			pstmt.setString(3, "aaaa");
			pstmt.addBatch();
			pstmt.executeBatch();

			pstmt.close();
			assertResult();

		} catch (Exception e) {
			e.printStackTrace();

			assertTrue(false);
		}
	}

	@Test
	public void testUpdateWithSetClob() throws Exception {
		try (Statement stmt = _conn.createStatement()) {
			stmt.execute("delete from " + Utils.schema + "." + tableName);

			stmt.execute("INSERT INTO " + Utils.schema + "." + tableName + " values (1, empty_blob(), 'aaaa')");
			stmt.execute("INSERT INTO " + Utils.schema + "." + tableName + " values (2, empty_blob(), 'aaaa')");
			PreparedStatement pstmt = _conn
					.prepareStatement("update " + Utils.schema + "." + tableName + " set c2 = ? where c1 = ?");

			Clob clob1 = _conn.createClob();
			Clob clob2 = _conn.createClob();

			clob1.setString(1, "this is a clob1 object");
			clob2.setString(1, "this is a clob2 object");

			pstmt.setInt(2, 1);
			pstmt.setClob(1, clob1);
			// pstmt.setString(3, "aaaa");
			pstmt.addBatch();
			pstmt.setInt(2, 2);
			pstmt.setClob(1, clob2);
			// pstmt.setString(3, "aaaa");
			pstmt.addBatch();
			pstmt.executeBatch();

			pstmt.close();
			assertResult();

		} catch (Exception e) {
			e.printStackTrace();

			assertTrue(false);
		}
	}

	@Test
	public void testSetString() throws Exception {
		try (Statement stmt = _conn.createStatement()) {
			stmt.execute("delete from " + Utils.schema + "." + tableName);

			PreparedStatement pstmt = _conn
					.prepareStatement("insert into " + Utils.schema + "." + tableName + " values (?, ?, ?)");

			pstmt.setInt(1, 1);
			pstmt.setString(2, "this is a clob1 object");
			pstmt.setString(3, "aaaa");
			pstmt.addBatch();

			pstmt.setInt(1, 2);
			pstmt.setString(2, "this is a clob2 object");
			pstmt.setString(3, "aaaa");
			pstmt.addBatch();
			pstmt.executeBatch();

			pstmt.close();
			assertResult();
		} catch (Exception e) {
			e.printStackTrace();
			assertTrue(false);
		}
	}

	@Test
	public void testSetBinaryStream() throws Exception {
		try (Statement stmt = _conn.createStatement()) {
			stmt.execute("delete from " + Utils.schema + "." + tableName);

			PreparedStatement pstmt = _conn
					.prepareStatement("insert into " + Utils.schema + "." + tableName + " values (?, ? ,?)");

			Reader reader1 = new StringReader("this is a clob1 object");
			Reader reader2 = new StringReader("this is a clob2 object");

			int len = "this is a clob1 object".length();
			pstmt.setInt(1, 1);
			pstmt.setCharacterStream(2, reader1, len);
			pstmt.setString(3, "aaaa");
			pstmt.addBatch();
			pstmt.setInt(1, 2);
			pstmt.setCharacterStream(2, reader2, len);
			pstmt.setString(3, "aaaa");
			pstmt.addBatch();
			pstmt.executeBatch();

			assertResult();
		} catch (Exception e) {
			e.printStackTrace();
			assertTrue(false);
		}
	}

	@BeforeClass
	public static void doTestSuiteSetup() throws Exception {
        try {
            _conn = Utils.getUserConnection();
        } catch (Exception e) {
            fail("failed to create connection" + e.getMessage());
        }

        try (
			Statement stmt = _conn.createStatement();
        ) {
			// use CQD to enable BLOB support
			stmt.execute("CQD TRAF_CLOB_AS_VARCHAR 'OFF'");
			stmt.execute(strCreateTable);
		} catch (Exception e) {
            fail("failed to set CQD : " + e.getMessage());
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
