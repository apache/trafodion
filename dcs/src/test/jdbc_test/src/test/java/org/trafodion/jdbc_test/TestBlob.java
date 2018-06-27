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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.InputStream;
import java.io.OutputStream;
import java.sql.Blob;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

public class TestBlob {
    private static final String tableName = "BLOBTEST";
    private static final String strCreateTable = "CREATE TABLE if not exists " + Utils.schema + "." + tableName
            + "(C1 int, c2 BLOB);";
    private static final String strDropTable = "DROP TABLE " + Utils.schema + "." + tableName;

    private static Connection _conn = null;
 
    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        try {
            _conn = Utils.getUserConnection();
        }
        catch (Exception e) {
            fail("failed to create connection" + e.getMessage());
        }
        try (
            Statement stmt = _conn.createStatement();
        ) {
            // use CQD to enable BLOB support
            stmt.execute("CQD TRAF_BLOB_AS_VARCHAR 'OFF'");
            stmt.execute(strCreateTable);
        } catch (Exception e) {
            fail("failed to set CQDs for Blob : " + e.getMessage());
        }
    }

    @Test
    public void testGetBytes() throws SQLException {
        try (Statement stmt = _conn.createStatement()) {
            stmt.execute("delete from " + Utils.schema + "." + tableName);
            stmt.execute("insert into " + Utils.schema + "." + tableName + " values (1, stringtolob('this is a BLOB object'))");

            ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
            assertTrue(rs.next());

            Blob blob = rs.getBlob(1);
            byte[] b = blob.getBytes(1, (int)blob.length());
            assertArrayEquals("this is a BLOB object".getBytes("UTF-8"), b);
            assertFalse(rs.next());
            rs.close();
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    @Test
    public void testGetBinaryStream() throws SQLException {
        try (Statement stmt = _conn.createStatement()) {
            stmt.execute("delete from " + Utils.schema + "." + tableName);
            stmt.execute("insert into " + Utils.schema + "." + tableName + " values (1, stringtolob('this is a BLOB object'))");

            ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
            assertTrue(rs.next());

            Blob blob = rs.getBlob(1);
            InputStream is = blob.getBinaryStream();

            byte[] b = new byte[(int)blob.length()];
            is.read(b);
            assertArrayEquals( "this is a BLOB object".getBytes("UTF-8"), b);
            is.close();

            // Test BinaryStream with length
            is = blob.getBinaryStream(1, 3);
            b = new byte[3];
            is.read(b);
            assertArrayEquals("thi".getBytes("UTF-8"), b);
            is.close();

            rs.close();
        }
        catch (Exception e) {
           
            e.printStackTrace();
            assertTrue(false);
        }
    }

    @Test
    public void testSetBytes() throws SQLException {
        try (Statement stmt = _conn.createStatement()) {
            stmt.execute("delete from " + Utils.schema + "." + tableName);
            stmt.execute("insert into " + Utils.schema + "." + tableName + " values (1, EMPTY_BLOB())");

            ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
            assertTrue(rs.next());
            Blob blob = rs.getBlob(1);
            String testString = "this is a BLOB object";
            byte[] b = testString.getBytes();
            blob.setBytes(1, b);
            rs.close();

            rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
            assertTrue(rs.next());
            Blob blob2 = rs.getBlob(1);
            byte[] b2 = blob2.getBytes(1, (int)blob2.length());
            assertArrayEquals(b, b2);
            assertFalse(rs.next());
            rs.close();
        }
        catch (Exception e)
        {
            
            e.printStackTrace();
            assertTrue(false);
        }
    }
    
    @Test
    public void testSetBytesWithLen() throws SQLException {
        try (Statement stmt = _conn.createStatement()) {
            stmt.execute("delete from " + Utils.schema + "." + tableName);
            stmt.execute("insert into " + Utils.schema + "." + tableName + " values (1, EMPTY_BLOB())");
            
            ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
            assertTrue(rs.next());
            Blob blob = rs.getBlob(1);
            byte[] b = "this is a BLOB object".getBytes();
            blob.setBytes(1, b, 2, 3);
            assertFalse(rs.next());
            rs.close();
            rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
            assertTrue(rs.next());
            blob = rs.getBlob(1);
            b = blob.getBytes(1, (int)blob.length());
            assertArrayEquals(b, "is ".getBytes());
            assertFalse(rs.next());
            rs.close();
            
        }
        catch (Exception e )
        {
            
            e.printStackTrace();
            assertTrue(false);
        }
    }
    
    @Test
    public void testSetBinaryStream() throws SQLException {
        try (Statement stmt = _conn.createStatement()) {
            stmt.execute("delete from " + Utils.schema + "." + tableName);
            stmt.execute("insert into " + Utils.schema + "." + tableName + " values (1, EMPTY_BLOB())");
            
            ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
            assertTrue(rs.next());
            Blob blob = rs.getBlob(1);
            OutputStream os = blob.setBinaryStream(1);
            byte[] b = "this is a BLOB object".getBytes("UTF-8");
            
            os.write(b);
            
            os.close();
            assertFalse(rs.next());
            rs.close();
            
            rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
            assertTrue(rs.next());
            blob = rs.getBlob(1);
            b = blob.getBytes(1, (int)blob.length());
            assertArrayEquals(b, "this is a BLOB object".getBytes("UTF-8"));
            assertFalse(rs.next());
            rs.close();
            
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    @Test
    public void testGetBytesFromRS() throws Exception {
        try (Statement stmt = _conn.createStatement()) {
            stmt.execute("delete from " + Utils.schema + "." + tableName);
            stmt.execute("insert into " + Utils.schema + "." + tableName + " values (1, stringtolob('this is a BLOB object'))");

            ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
            assertTrue(rs.next());
            
            byte[] b = rs.getBytes(1);
            assertArrayEquals(b, "this is a BLOB object".getBytes("UTF-8"));
            rs.close();
            
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    @Test
    public void testGetBinaryStreamfromRS() throws Exception {
        try (Statement stmt = _conn.createStatement()) {
            stmt.execute("delete from " + Utils.schema + "." + tableName);
            stmt.execute("insert into " + Utils.schema + "." + tableName + " values (1, stringtolob('this is a BLOB object'))");

            ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
            assertTrue(rs.next());

            InputStream is = rs.getBinaryStream(1);

            byte[] expected = "this is a BLOB object".getBytes("UTF-8");
            byte[] b = new byte[expected.length];
            is.read(b);
            assertArrayEquals( "this is a BLOB object".getBytes("UTF-8"), b);
            is.close();
        }
        catch (Exception e) {
           
            e.printStackTrace();
            assertTrue(false);
        }
    }
    
    @Test   
    public void testFree() throws Exception {
        try (Statement stmt = _conn.createStatement()) {
            stmt.execute("delete from " + Utils.schema + "." + tableName);
            stmt.execute("insert into " + Utils.schema + "." + tableName + " values (1, stringtolob('this is a BLOB object'))");

            ResultSet rs = stmt.executeQuery("SELECT C2 FROM " + Utils.schema + "." + tableName + " WHERE C1 = 1");
            assertTrue(rs.next());
            
            Blob blob = rs.getBlob(1);
            blob.free();
            
            boolean lob_freed_excption = false;
            try {
                long len = blob.length();
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
