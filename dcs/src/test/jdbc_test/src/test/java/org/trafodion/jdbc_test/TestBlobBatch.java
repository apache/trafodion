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

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.sql.Blob;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.Statement;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

public class TestBlobBatch {
    private static final String tableName = "BLOBTEST";
    private static final String strCreateTable = "CREATE TABLE " + Utils.schema + "." + tableName + "(C1 int, c2 BLOB, c3 CHAR(4));";
    private static final String strDropTable = "DROP TABLE " + Utils.schema + "." + tableName;

    private static Connection _conn = null;
 
    @SuppressWarnings("finally")
    private boolean assertResult() throws Exception {
        Statement stmt = null;
        ResultSet rs = null;
        boolean result = false;
        try {
            stmt = _conn.createStatement();
            
            rs = stmt.executeQuery("select c1, c2, c3 from " + Utils.schema + "." + tableName + " order by c1");
            
            assertTrue(rs.next());
            assertEquals(1, rs.getInt(1));
            assertArrayEquals("this is a BLOB1 object".getBytes("UTF-8"), rs.getBytes(2));
            assertEquals("aaaa", rs.getString(3));
            assertTrue(rs.next());
            
            assertEquals(2,rs.getInt(1));
            assertArrayEquals("this is a BLOB2 object".getBytes("UTF-8"), rs.getBytes(2));
            assertEquals("aaaa", rs.getString(3));
            assertFalse(rs.next());
            result = true;
        }catch (Exception e) {
            e.printStackTrace();
        }
        finally {
            stmt.close();
            rs.close();
            return result;
        }
    }
    @Test
    public void testSetBlob() throws Exception {
        try (Statement stmt = _conn.createStatement()) {
            stmt.execute("delete from " + Utils.schema + "." + tableName);

            PreparedStatement pstmt = _conn.prepareStatement("insert into " + Utils.schema + "." + tableName + " values (?, ?, ?)");
            
            Blob blob1 = _conn.createBlob();
            Blob blob2 = _conn.createBlob();
            
            blob1.setBytes(1, "this is a BLOB1 object".getBytes("UTF-8"));
            blob2.setBytes(1, "this is a BLOB2 object".getBytes("UTF-8"));
            pstmt.setInt(1, 1);
            pstmt.setBlob(2, blob1);
            pstmt.setString(3, "aaaa");
            pstmt.addBatch();
            pstmt.setInt(1, 2);
            pstmt.setBlob(2, blob2);
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
    public void testSetBytes() throws Exception {
        try (Statement stmt = _conn.createStatement()) {
            stmt.execute("delete from " + Utils.schema + "." + tableName);

            PreparedStatement pstmt = _conn.prepareStatement("insert into " + Utils.schema + "." + tableName + " values (?, ?, ?)");
            
            pstmt.setInt(1, 1);
            pstmt.setBytes(2,"this is a BLOB1 object".getBytes("UTF-8"));
            pstmt.setString(3, "aaaa");
            pstmt.addBatch();
            
            pstmt.setInt(1, 2);
            pstmt.setBytes(2, "this is a BLOB2 object".getBytes("UTF-8"));
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

            PreparedStatement pstmt = _conn.prepareStatement("insert into " + Utils.schema + "." + tableName + " values (?, ?, ?)");
            
            byte[] b1 = "this is a BLOB1 object".getBytes("UTF-8");
            InputStream is = new ByteArrayInputStream(b1);
            
            byte[] b2 = "this is a BLOB2 object".getBytes("UTF-8");
            InputStream is2 = new ByteArrayInputStream(b2);
           
            pstmt.setInt(1, 1);
            pstmt.setBinaryStream(2, is, b1.length);
            pstmt.setString(3, "aaaa");
            pstmt.addBatch();
            pstmt.setInt(1, 2);
            pstmt.setBinaryStream(2, is2, b2.length);
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
            stmt.execute("CQD TRAF_BLOB_AS_VARCHAR 'OFF'");
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
