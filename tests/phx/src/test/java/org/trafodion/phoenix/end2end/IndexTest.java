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

/*******************************************************************************
 * Copyright (c) 2013, Salesforce.com, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *     Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *     Neither the name of Salesforce.com nor the names of its contributors may
 *     be used to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/
package test.java.org.trafodion.phoenix.end2end;

import static org.junit.Assert.*;
import org.junit.*;
import java.math.*;
import java.sql.*;
import java.util.*;


public class IndexTest extends BaseTest{
    private static final int TABLE_SPLITS = 3;
    private static final int INDEX_SPLITS = 4;
 
    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table t", "table " + INDEX_DATA_TABLE));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    // Populate the test table with data.
    private void populateTestTable() throws SQLException {
        try {
            String upsert = null;
            if (tgtPH()) upsert = "UPSERT INTO " + INDEX_DATA_SCHEMA + NAME_SEPARATOR + INDEX_DATA_TABLE
                    + " VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            else if (tgtTR()) upsert = "UPSERT INTO " + INDEX_DATA_TABLE
                    + " VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            else if (tgtSQ()) upsert = "INSERT INTO " + INDEX_DATA_TABLE
                    + " VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            PreparedStatement stmt = conn.prepareStatement(upsert);
            stmt.setString(1, "varchar1");
            stmt.setString(2, "char1");
            stmt.setInt(3, 1);
            stmt.setLong(4, 1L);
            stmt.setBigDecimal(5, new BigDecimal(1.0));
            stmt.setString(6, "varchar_a");
            stmt.setString(7, "chara");
            stmt.setInt(8, 2);
            stmt.setLong(9, 2L);
            stmt.setBigDecimal(10, new BigDecimal(2.0));
            stmt.setString(11, "varchar_b");
            stmt.setString(12, "charb");
            stmt.setInt(13, 3);
            stmt.setLong(14, 3L);
            stmt.setBigDecimal(15, new BigDecimal(3.0));
            stmt.executeUpdate();
            
            stmt.setString(1, "varchar2");
            stmt.setString(2, "char2");
            stmt.setInt(3, 2);
            stmt.setLong(4, 2L);
            stmt.setBigDecimal(5, new BigDecimal(2.0));
            stmt.setString(6, "varchar_a");
            stmt.setString(7, "chara");
            stmt.setInt(8, 3);
            stmt.setLong(9, 3L);
            stmt.setBigDecimal(10, new BigDecimal(3.0));
            stmt.setString(11, "varchar_b");
            stmt.setString(12, "charb");
            stmt.setInt(13, 4);
            stmt.setLong(14, 4L);
            stmt.setBigDecimal(15, new BigDecimal(4.0));
            stmt.executeUpdate();
            
            stmt.setString(1, "varchar3");
            stmt.setString(2, "char3");
            stmt.setInt(3, 3);
            stmt.setLong(4, 3L);
            stmt.setBigDecimal(5, new BigDecimal(3.0));
            stmt.setString(6, "varchar_a");
            stmt.setString(7, "chara");
            stmt.setInt(8, 4);
            stmt.setLong(9, 4L);
            stmt.setBigDecimal(10, new BigDecimal(4.0));
            stmt.setString(11, "varchar_b");
            stmt.setString(12, "charb");
            stmt.setInt(13, 5);
            stmt.setLong(14, 5L);
            stmt.setBigDecimal(15, new BigDecimal(5.0));
            stmt.executeUpdate();
        } finally {
        }
    }
    
    @Test
    public void testImmutableTableIndexMaintanenceSaltedSalted() throws Exception {
        printTestDescription();
        testImmutableTableIndexMaintanence(TABLE_SPLITS, INDEX_SPLITS);
    }

    @Test
    public void testImmutableTableIndexMaintanenceSalted() throws Exception {
        printTestDescription();
        testImmutableTableIndexMaintanence(null, INDEX_SPLITS);
    }

    @Test
    public void testImmutableTableIndexMaintanenceUnsalted() throws Exception {
        printTestDescription();
        testImmutableTableIndexMaintanence(null, null);
    }

    private void testImmutableTableIndexMaintanence(Integer tableSaltBuckets, Integer indexSaltBuckets) throws Exception {
        try {
            String query;
            ResultSet rs;
            
            if (tgtPH()) conn.createStatement().execute("CREATE TABLE t (k VARCHAR NOT NULL PRIMARY KEY, v VARCHAR) IMMUTABLE_ROWS=true " +  (tableSaltBuckets == null ? "" : ", SALT_BUCKETS=" + tableSaltBuckets));
            else if (tgtSQ()||tgtTR()) conn.createStatement().execute("CREATE TABLE t (k VARCHAR(1) NOT NULL PRIMARY KEY, v VARCHAR(1))");
            query = "SELECT * FROM t";
            rs = conn.createStatement().executeQuery(query);
            assertFalse(rs.next());
            
            if (tgtPH()) {
                conn.createStatement().execute("CREATE INDEX i ON t (v DESC)" + (indexSaltBuckets == null ? "" : " SALT_BUCKETS=" + indexSaltBuckets));
                query = "SELECT * FROM i";
                rs = conn.createStatement().executeQuery(query);
                assertFalse(rs.next());
            } else if (tgtSQ()||tgtTR()) {
                conn.createStatement().execute("CREATE INDEX i ON t (v DESC)");
                // TRAF: does not allow direct access to indexes
            }

            conn.setAutoCommit(false);
 
            PreparedStatement stmt = null;
            if (tgtPH()||tgtTR()) stmt = conn.prepareStatement("UPSERT INTO t VALUES(?,?)");
            else if (tgtSQ()) stmt = conn.prepareStatement("INSERT INTO t VALUES(?,?)");
            stmt.setString(1,"a");
            stmt.setString(2, "x");
            stmt.execute();
            stmt.setString(1,"b");
            stmt.setString(2, "y");
            stmt.execute();
            conn.commit();
         
            // TRAF: does not allow direct access to indexes
            if (tgtPH()) { 
                query = "SELECT * FROM i";
                rs = conn.createStatement().executeQuery(query);
                assertTrue(rs.next());
                assertEquals("y",rs.getString(1));
                assertEquals("b",rs.getString(2));
                assertTrue(rs.next());
                assertEquals("x",rs.getString(1));
                assertEquals("a",rs.getString(2));
                assertFalse(rs.next());
            }
 
            query = "SELECT k,v FROM t WHERE v = 'y'";
            rs = conn.createStatement().executeQuery(query);
            assertTrue(rs.next());
            assertEquals("b",rs.getString(1));
            assertEquals("y",rs.getString(2));
            assertFalse(rs.next());
            
            String expectedPlan;
            if (tgtPH()) {
                rs = conn.createStatement().executeQuery("EXPLAIN " + query);
                expectedPlan = indexSaltBuckets == null ? 
                     "CLIENT PARALLEL 1-WAY RANGE SCAN OVER I 'y'" : 
                    ("CLIENT PARALLEL 4-WAY SKIP SCAN ON 4 KEYS OVER I 0...3,'y'\n" + 
                     "CLIENT MERGE SORT");
                assertEquals(expectedPlan,getExplainPlan(rs));
            } else if (tgtTR()) {
                 // TRAF: For now, only make sure that scan is in there.
                 rs = conn.createStatement().executeQuery("EXPLAIN options 'f' " + query);
                 assertTrue(getExplainPlan(rs).contains("scan"));
            } else if (tgtSQ()) {
                 // TRAF: For now, only make sure that index_scan is in there.
                 rs = conn.createStatement().executeQuery("EXPLAIN options 'f' " + query);
                 assertTrue(getExplainPlan(rs).contains("index_scan"));
            }
 
            // Will use index, so rows returned in DESC order.
            // This is not a bug, though, because we can
            // return in any order.
            if (tgtPH()) query = "SELECT k,v FROM t WHERE v >= 'x'";
            else if (tgtSQ()||tgtTR()) query = "SELECT k,v FROM t WHERE v >= 'x' order by 1 desc";
            rs = conn.createStatement().executeQuery(query);
            assertTrue(rs.next());
            assertEquals("b",rs.getString(1));
            assertEquals("y",rs.getString(2));
            assertTrue(rs.next());
            assertEquals("a",rs.getString(1));
            assertEquals("x",rs.getString(2));
            assertFalse(rs.next());
            if (tgtPH()) {
                rs = conn.createStatement().executeQuery("EXPLAIN " + query);
                expectedPlan = indexSaltBuckets == null ? 
                    "CLIENT PARALLEL 1-WAY RANGE SCAN OVER I (*-'x']" :
                    ("CLIENT PARALLEL 4-WAY SKIP SCAN ON 4 RANGES OVER I 0...3,(*-'x']\n" + 
                     "CLIENT MERGE SORT");
                assertEquals(expectedPlan,getExplainPlan(rs));
            } else if (tgtTR()) {
                 // TRAF: For now, only make sure that scan is in there.
                 rs = conn.createStatement().executeQuery("EXPLAIN options 'f' " + query);
                 assertTrue(getExplainPlan(rs).contains("scan"));
            } else if (tgtSQ()) {
                 // TRAF: For now, only make sure that index_scan is in there.
                 rs = conn.createStatement().executeQuery("EXPLAIN options 'f' " + query);
                 // TRAF DEBUG: Unfortunately, this query does not use
                 // index anymore
                 // TRAF assertTrue(getExplainPlan(rs).contains("index_scan"));
                 assertTrue(getExplainPlan(rs).contains("file_scan"));
            }

            // Will still use index, since there's no LIMIT clause
            query = "SELECT k,v FROM t WHERE v >= 'x' ORDER BY k";
            rs = conn.createStatement().executeQuery(query);
            assertTrue(rs.next());
            assertEquals("a",rs.getString(1));
            assertEquals("x",rs.getString(2));
            assertTrue(rs.next());
            assertEquals("b",rs.getString(1));
            assertEquals("y",rs.getString(2));
            assertFalse(rs.next());
            if (tgtPH()) {
                rs = conn.createStatement().executeQuery("EXPLAIN " + query);
                // Turns into an ORDER BY, which could be bad if lots of data is
                // being returned. Without stats we don't know. The alternative
                // would be a full table scan.
                expectedPlan = indexSaltBuckets == null ? 
                    ("CLIENT PARALLEL 1-WAY RANGE SCAN OVER I (*-'x']\n" + 
                     "    SERVER TOP -1 ROWS SORTED BY [K]\n" + 
                     "CLIENT MERGE SORT") :
                    ("CLIENT PARALLEL 4-WAY SKIP SCAN ON 4 RANGES OVER I 0...3,(*-'x']\n" + 
                     "    SERVER TOP -1 ROWS SORTED BY [K]\n" + 
                     "CLIENT MERGE SORT");
                assertEquals(expectedPlan,getExplainPlan(rs));
            } else if (tgtTR()) {
                 // TRAF: For now, only make sure that scan is in there.
                 rs = conn.createStatement().executeQuery("EXPLAIN options 'f' " + query);
                 assertTrue(getExplainPlan(rs).contains("scan"));
            } else if (tgtSQ()) {
                 // TRAF: For now, only make sure that index_scan is in there.
                 rs = conn.createStatement().executeQuery("EXPLAIN options 'f' " + query);
                 // TRAF DEBUG: Unfortunately, this query does not use
                 // index anymore
                 // TRAF assertTrue(getExplainPlan(rs).contains("index_scan"));
                 assertTrue(getExplainPlan(rs).contains("file_scan"));
            }
 
            // Will use data table now, since there's a LIMIT clause and
            // we're able to optimize out the ORDER BY, unless the data
            // table is salted.
            if (tgtPH()||tgtTR()) query = "SELECT k,v FROM t WHERE v >= 'x' ORDER BY k LIMIT 2";
            else if (tgtSQ()) query = "SELECT [first 2] k,v FROM t WHERE v >= 'x' ORDER BY k";
            rs = conn.createStatement().executeQuery(query);
            assertTrue(rs.next());
            assertEquals("a",rs.getString(1));
            assertEquals("x",rs.getString(2));
            assertTrue(rs.next());
            assertEquals("b",rs.getString(1));
            assertEquals("y",rs.getString(2));
            assertFalse(rs.next());
            if (tgtPH()) {
                rs = conn.createStatement().executeQuery("EXPLAIN " + query);
                expectedPlan = tableSaltBuckets == null ? 
                    "CLIENT PARALLEL 1-WAY FULL SCAN OVER T\n" + 
                    "    SERVER FILTER BY V >= 'x'\n" + 
                    "    SERVER 2 ROW LIMIT\n" + 
                    "CLIENT 2 ROW LIMIT" :
                    "CLIENT PARALLEL 4-WAY SKIP SCAN ON 4 RANGES OVER I 0...3,(*-'x']\n" + 
                    "    SERVER TOP 2 ROWS SORTED BY [K]\n" + 
                    "CLIENT MERGE SORT";
                assertEquals(expectedPlan,getExplainPlan(rs));
            } else if (tgtTR()) {
                 // TRAF: For now, only make sure that scan is in there.
                 rs = conn.createStatement().executeQuery("EXPLAIN options 'f' " + query);
                 assertTrue(getExplainPlan(rs).contains("scan"));
            } else if (tgtSQ()) {
                 // TRAF: For now, only make sure that index_scan is in there.
                 rs = conn.createStatement().executeQuery("EXPLAIN options 'f' " + query);
                 // TRAF DEBUG: Unfortunately, this query does not use
                 // index anymore
                 // TRAF assertTrue(getExplainPlan(rs).contains("index_scan"));
                 assertTrue(getExplainPlan(rs).contains("file_scan"));
            }
        } finally {
        }
    }

    @Test
    public void testIndexWithNullableFixedWithCols() throws Exception {
        printTestDescription();

        if (tgtPH()||tgtSQ())
    	    conn.setAutoCommit(false);
        else if (tgtTR())
            conn.setAutoCommit(true);
 
    	try {
            createTestTable(INDEX_DATA_TABLE);
            populateTestTable();
            String ddl = null;
            if (tgtPH()) ddl = "CREATE INDEX IDX ON " + INDEX_DATA_SCHEMA + NAME_SEPARATOR + INDEX_DATA_TABLE
                    + " (char_col1 ASC, int_col1 ASC)"
                    + " INCLUDE (long_col1, long_col2)";
            else if (tgtSQ()||tgtTR()) ddl = "CREATE INDEX IDX ON " + INDEX_DATA_TABLE
                    + " (char_col1 ASC, int_col1 ASC)";
            PreparedStatement stmt = conn.prepareStatement(ddl);
            stmt.execute();
            
            String query = null;
            if (tgtPH()) query = "SELECT char_col1, int_col1 from " + INDEX_DATA_SCHEMA + NAME_SEPARATOR + INDEX_DATA_TABLE;
            else if (tgtSQ()||tgtTR()) query = "SELECT char_col1, int_col1 from " + INDEX_DATA_TABLE + " order by 2";
            ResultSet rs = null;
            if (tgtPH()) {
                rs = conn.createStatement().executeQuery("EXPLAIN " + query);
                assertEquals("CLIENT PARALLEL 1-WAY FULL SCAN OVER INDEX_TEST.IDX", getExplainPlan(rs));
            } else if (tgtTR()) {
                 // TRAF: For now, only make sure that scan is in there.
                 rs = conn.createStatement().executeQuery("EXPLAIN options 'f' " + query);
                 assertTrue(getExplainPlan(rs).contains("scan"));
            } else if (tgtSQ()) {
                 // TRAF: For now, only make sure that index_scan is in there.
                 rs = conn.createStatement().executeQuery("EXPLAIN options 'f' " + query);
                 assertTrue(getExplainPlan(rs).contains("index_scan"));
            }
 
            rs = conn.createStatement().executeQuery(query);
            assertTrue(rs.next());
            assertEquals("chara", rs.getString(1));
            assertEquals(2, rs.getInt(2));
            assertTrue(rs.next());
            assertEquals("chara", rs.getString(1));
            assertEquals(3, rs.getInt(2));
            assertTrue(rs.next());
            assertEquals("chara", rs.getString(1));
            assertEquals(4, rs.getInt(2));
            assertFalse(rs.next());
    	} finally {
    	}
    }
}
