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

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.Reader;
import java.sql.Clob;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

public class TestClob {
	private static final String tableName = "CLOBTEST";
	private static final String colName = "C1";
	private static final String strCreateTable = "CREATE TABLE " + Utils.schema + "." + tableName
			+ "(C1 int, C2 CLOB);";
	private static final String strCleanTable = "DELETE FROM " + Utils.schema + "." + tableName;
	private static final String strDropTable = "DROP TABLE " + Utils.schema + "." + tableName;

	private static Connection _conn = null;

	@BeforeClass
	public static void doTestSuiteSetup() throws Exception {
	    try {
            _conn = Utils.getUserConnection();
	    }
	    catch (SQLException se) {
            se.printStackTrace();
	        fail("failed to create connection : " + se.getMessage());
	    }
	    try (
	            Statement stmt = _conn.createStatement();
	            )
        {
			// use CQD to enable CLOB support
			stmt.execute("CQD TRAF_CLOB_AS_VARCHAR 'OFF'");
			// stmt.execute(strDropTable);
			stmt.execute(strCreateTable);
		} catch (Exception e) {
			System.out.println(e.getMessage());
			e.printStackTrace();
            fail("failed to set CQD for CLOB");
		}
	}

	@Test
	public void readCLOBData() throws Exception {
		// USE built-in function stringtolob to insert data
		String data = "this is a CLOB String";

		try (Statement stmt = _conn.createStatement()) {
			stmt.execute(strCleanTable);
			stmt.executeUpdate(
					"INSERT INTO " + Utils.schema + "." + tableName + " values (1, stringtolob('" + data + "'))");

			ResultSet rs = stmt.executeQuery("SELECT * FROM " + Utils.schema + "." + tableName);
			int rowNum = 0;
			while (rs.next()) {
				rowNum++;
				Clob clob = rs.getClob(2);
				assertEquals("clob data ", data, clob.getSubString(1, (int) clob.length()));
			}
			assertEquals("Row count read from server", 1, rowNum);
		} catch (Exception e) {
			System.out.println(e.getMessage());
			e.printStackTrace();
		}
	}

	@Test
	public void testGetString() throws SQLException {
		try (Statement stmt = _conn.createStatement()) {
			stmt.execute(strCleanTable);
			stmt.execute("insert into " + Utils.schema + "." + tableName
					+ " values (1, stringtolob('this is a CLOB object'))");

			ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");

			assertTrue(rs.next());

			Clob clob = rs.getClob(1);
			String str = clob.getSubString(1, (int) clob.length());
			assertEquals(str, "this is a CLOB object");
			assertFalse(rs.next());
			rs.close();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@Test
	public void testGetCharacterStream() throws SQLException {
		try (Statement stmt = _conn.createStatement()) {
			stmt.execute(strCleanTable);
			stmt.execute("insert into " + Utils.schema + "." + tableName
					+ " values (1, stringtolob('this is a CLOB object'))");

			ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
			assertTrue(rs.next());

			Clob clob = rs.getClob(1);
			Reader reader = clob.getCharacterStream();
			char c[] = new char[(int) clob.length()];
			reader.read(c);
			// expected
			assertArrayEquals("this is a CLOB object".toCharArray(), c);
			reader.close();

			// Test getCharacterStream with length
			reader = clob.getCharacterStream(2, 3);
			c = new char[3];
			reader.read(c);
			assertArrayEquals("his".toCharArray(), c);
			reader.close();
			rs.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	@Test
	public void testGetAsciiStream() throws Exception {
		try (Statement stmt = _conn.createStatement()) {
			stmt.execute(strCleanTable);
			stmt.execute("insert into " + Utils.schema + "." + tableName
					+ " values (1, stringtolob('this is a CLOB object'))");

			ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");

			assertTrue(rs.next());

			Clob clob = rs.getClob(1);
			InputStream is = clob.getAsciiStream();
			byte[] b = new byte[(int) clob.length()];
			is.read(b);

			assertArrayEquals("this is a CLOB object".getBytes("utf8"), b);
			assertFalse(rs.next());
			rs.close();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@Test
	public void testSetCharacterStream() {
		try (Statement stmt = _conn.createStatement()) {
			stmt.execute("delete from " + Utils.schema + "." + tableName);
			stmt.execute("insert into " + Utils.schema + "." + tableName + " values (1, empty_clob())");

			ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
			assertTrue(rs.next());

			Clob clob = rs.getClob(1);
			OutputStream os = clob.setAsciiStream(1);
			byte[] b = "this is a CLOB object".getBytes("utf8");
			os.write(b);

			os.close();
			rs.close();

			rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
			assertTrue(rs.next());
			clob = rs.getClob(1);
			String str = clob.getSubString(1, (int) clob.length());
			assertArrayEquals(b, str.getBytes("utf8"));
			assertFalse(rs.next());
			rs.close();
		} catch (Exception e) {

		}
	}

	@Test
	public void testSetString() throws Exception {
		try (Statement stmt = _conn.createStatement()) {
			stmt.execute("delete from " + Utils.schema + "." + tableName);
			stmt.execute("insert into " + Utils.schema + "." + tableName + " values (1, empty_clob())");

			ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
			assertTrue(rs.next());

			Clob clob = rs.getClob(1);
			clob.setString(1, "this is a CLOB object");
			assertFalse(rs.next());

			rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
			assertTrue(rs.next());
			clob = rs.getClob(1);
			String str = clob.getSubString(1, (int) clob.length());
			System.out.println(str);
			assertEquals("this is a CLOB object", str);
			assertFalse(rs.next());
			rs.close();
		} catch (Exception e) {
			System.out.println(e.getMessage());
			e.printStackTrace();
		}
	}

	@Test
	public void testSetStringWithLength() throws Exception {
		try (Statement stmt = _conn.createStatement()) {
			stmt.execute("delete from " + Utils.schema + "." + tableName);
			stmt.execute("insert into " + Utils.schema + "." + tableName + " values (1, empty_clob())");

			ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
			assertTrue(rs.next());

			Clob clob = rs.getClob(1);
			clob.setString(1, "this is a CLOB object", 2, 3);
			assertFalse(rs.next());

			rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
			assertTrue(rs.next());
			clob = rs.getClob(1);
			String str = clob.getSubString(1, (int) clob.length());
			System.out.println(str);
			assertEquals("is ", str);
			assertFalse(rs.next());
			rs.close();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@Test
	public void testTruncate() throws Exception {
		try (Statement stmt = _conn.createStatement()) {
			stmt.execute("delete from " + Utils.schema + "." + tableName);
			stmt.execute("insert into " + Utils.schema + "." + tableName
					+ " values (1, stringtolob('this is a CLOB object'))");

			ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
			assertTrue(rs.next());

			int lenLeft = 10;
			Clob clob = rs.getClob(1);
			clob.truncate(lenLeft);
			clob.free();

			assertFalse(rs.next());

			rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
			assertTrue(rs.next());
			clob = rs.getClob(1);

			assertEquals("The length of CLOB ", lenLeft, clob.length());
		} catch (Exception e) {
			e.printStackTrace();
			assertTrue(false);
		}
	}

	@Test
	public void testReadtoString() throws Exception {
		try (Statement stmt = _conn.createStatement()) {
			stmt.execute("delete from " + Utils.schema + "." + tableName);
			stmt.execute("insert into " + Utils.schema + "." + tableName
					+ " values (1, stringtolob('this is a CLOB object'))");

			ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
			assertTrue(rs.next());

			String str = rs.getString(1);
			System.out.println(str);
			assertEquals("getString for CLB", "this is a CLOB object", str);
			rs.close();

		} catch (Exception e) {
			e.printStackTrace();
			assertTrue(false);
		}
	}

	@Test
	public void testGetCharacterStreamFromRS() throws Exception {
		try (Statement stmt = _conn.createStatement()) {
			stmt.execute("delete from " + Utils.schema + "." + tableName);
			stmt.execute("insert into " + Utils.schema + "." + tableName
					+ " values (1, stringtolob('this is a CLOB object'))");

			ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
			assertTrue(rs.next());

			Reader reader = rs.getCharacterStream(1);

			String b = "this is a CLOB object";
			char[] cbuf = new char[b.length()];

			reader.read(cbuf, 0, b.length());

			assertArrayEquals(b.toCharArray(), cbuf);
			rs.close();
		} catch (Exception e) {
			e.printStackTrace();
			assertTrue(false);
		}
	}

	@Test
	public void testPosition() throws Exception {
		try (Statement stmt = _conn.createStatement()) {
			stmt.execute("delete from " + Utils.schema + "." + tableName);
			stmt.execute("insert into " + Utils.schema + "." + tableName
					+ " values (1, stringtolob('this is a CLOB object'))");

			ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
			assertTrue(rs.next());

			Clob clob = rs.getClob(1);

			long pos = clob.position("CLOB", 1);
			assertEquals(10, pos);
			rs.close();
		} catch (Exception e) {
			e.printStackTrace();
			assertTrue(false);
		}
	}

	@Test
	public void testFree() throws Exception {
		try (Statement stmt = _conn.createStatement()) {
			stmt.execute("delete from " + Utils.schema + "." + tableName);
			stmt.execute("insert into " + Utils.schema + "." + tableName
					+ " values (1, stringtolob('this is a CLOB object'))");

			ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
			assertTrue(rs.next());

			Clob clob = rs.getClob(1);
			clob.free();

			boolean lob_freed_excption = false;
			try {
				long pos = clob.position("his", 1);
			} catch (Exception e) {
				lob_freed_excption = true;
			}

			assertTrue(lob_freed_excption);
		} catch (Exception e) {
			e.printStackTrace();
			assertTrue(false);
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
