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
import java.sql.*;
import java.util.*;


/**
 * Tests for table with transparent salting.
 */
public class SaltedTableTest extends BaseTest {

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table salted_table", "table " + TABLE_WITH_SALTING));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    private void initTableValues() throws Exception {
        // Rows we inserted:
        // 1ab123abc111
        // 1abc456abc111
        // 1de123abc111
        // 2abc123def222 
        // 3abc123ghi333
        // 4abc123jkl444
        try {
            // Upsert with no column specifies.
            createTestTable(TABLE_WITH_SALTING);
            String query = null;
            if (tgtPH()||tgtTR()) query = "UPSERT INTO " + TABLE_WITH_SALTING + " VALUES(?,?,?,?,?)";
            else if (tgtSQ()) query = "INSERT INTO " + TABLE_WITH_SALTING + " VALUES(?,?,?,?,?)";
            PreparedStatement stmt = conn.prepareStatement(query);
            stmt.setInt(1, 1);
            stmt.setString(2, "ab");
            stmt.setString(3, "123");
            stmt.setString(4, "abc");
            stmt.setInt(5, 111);
            stmt.execute();
            
            stmt.setInt(1, 1);
            stmt.setString(2, "abc");
            stmt.setString(3, "456");
            stmt.setString(4, "abc");
            stmt.setInt(5, 111);
            stmt.execute();
            
            // Test upsert when statement explicitly specifies the columns to upsert into.
            if (tgtPH()||tgtTR()) query = "UPSERT INTO " + TABLE_WITH_SALTING +
                    " (a_integer, a_string, a_id, b_string, b_integer) " + 
                    " VALUES(?,?,?,?,?)";
            else if (tgtSQ()) query = "INSERT INTO " + TABLE_WITH_SALTING +
                    " (a_integer, a_string, a_id, b_string, b_integer) " +
                    " VALUES(?,?,?,?,?)";
            stmt = conn.prepareStatement(query);
            
            stmt.setInt(1, 1);
            stmt.setString(2, "de");
            stmt.setString(3, "123");
            stmt.setString(4, "abc");
            stmt.setInt(5, 111);
            stmt.execute();
            
            stmt.setInt(1, 2);
            stmt.setString(2, "abc");
            stmt.setString(3, "123");
            stmt.setString(4, "def");
            stmt.setInt(5, 222);
            stmt.execute();
            
            // Test upsert when order of column is shuffled.
            if (tgtPH()||tgtTR()) query = "UPSERT INTO " + TABLE_WITH_SALTING +
                    " (a_string, a_integer, a_id, b_string, b_integer) " + 
                    " VALUES(?,?,?,?,?)";
            else if (tgtSQ()) query = "INSERT INTO " + TABLE_WITH_SALTING +
                    " (a_string, a_integer, a_id, b_string, b_integer) " +
                    " VALUES(?,?,?,?,?)";
            stmt = conn.prepareStatement(query);
            stmt.setString(1, "abc");
            stmt.setInt(2, 3);
            stmt.setString(3, "123");
            stmt.setString(4, "ghi");
            stmt.setInt(5, 333);
            stmt.execute();
            
            stmt.setString(1, "abc");
            stmt.setInt(2, 4);
            stmt.setString(3, "123");
            stmt.setString(4, "jkl");
            stmt.setInt(5, 444);
            stmt.execute();
        } finally {
        }
    }

    @Test
    public void testTableWithInvalidBucketNumber() throws Exception {
        printTestDescription();
        if (tgtSQ()||tgtTR()) return; // TRAF: does not support SALT_BUCKETS

        try {
            String query = "create table salted_table (a_integer integer not null CONSTRAINT pk PRIMARY KEY (a_integer)) SALT_BUCKETS = 257";
            PreparedStatement stmt = conn.prepareStatement(query);
            stmt.execute();
            fail("Should have caught exception");
        } catch (SQLException e) {
            assertTrue(e.getMessage(), e.getMessage().contains("ERROR 1021 (42Y80): Salt bucket numbers should be with 1 and 256."));
        } finally {
        }
    }

    @Test
    public void testTableWithSplit() throws Exception {
        printTestDescription();

        if (tgtSQ()||tgtTR()) return; // TRAF: does not support SALT_BUCKETS or split points

        try {
            // TRAF createTestTable(getUrl(), "create table salted_table (a_integer integer not null primary key) SALT_BUCKETS = 4",
            // TRAF         new byte[][] {{1}, {2,3}, {2,5}, {3}}, nextTimestamp());

            conn.createStatement().execute("create table salted_table (a_integer integer not null primary key) SALT_BUCKETS = 4");
            fail("Should have caught exception");
        } catch (SQLException e) {
            assertTrue(e.getMessage(), e.getMessage().contains("ERROR 1022 (42Y81): Should not specify split points on salted table with default row key order."));
        }
    }
    
    @Test
    public void testSelectValueNoWhereClause() throws Exception {
        printTestDescription();

        try {
            initTableValues();
            
            String query = null;
            if (tgtPH()) query = "SELECT * FROM " + TABLE_WITH_SALTING;
            else if (tgtSQ()||tgtTR()) query = "SELECT * FROM " + TABLE_WITH_SALTING + " order by 1, 2";
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            
            assertTrue(rs.next());
            assertEquals(1, rs.getInt(1));
            assertEquals("ab", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertEquals("abc", rs.getString(4));
            assertEquals(111, rs.getInt(5));
            
            assertTrue(rs.next());
            assertEquals(1, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            assertEquals("456", rs.getString(3));
            assertEquals("abc", rs.getString(4));
            assertEquals(111, rs.getInt(5));
            
            assertTrue(rs.next());
            assertEquals(1, rs.getInt(1));
            assertEquals("de", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertEquals("abc", rs.getString(4));
            assertEquals(111, rs.getInt(5));
            
            assertTrue(rs.next());
            assertEquals(2, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertEquals("def", rs.getString(4));
            assertEquals(222, rs.getInt(5));
            
            assertTrue(rs.next());
            assertEquals(3, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertEquals("ghi", rs.getString(4));
            assertEquals(333, rs.getInt(5));
            
            assertTrue(rs.next());
            assertEquals(4, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertEquals("jkl", rs.getString(4));
            assertEquals(444, rs.getInt(5));
            
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testSelectValueWithFullyQualifiedWhereClause() throws Exception {
        printTestDescription();

        try {
            initTableValues();
            String query;
            PreparedStatement stmt;
            ResultSet rs;
            
            // Variable length slot with bounded ranges.
            query = "SELECT * FROM " + TABLE_WITH_SALTING + 
                    " WHERE a_integer = 1 AND a_string >= 'ab' AND a_string < 'de' AND a_id = '123'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals(1, rs.getInt(1));
            assertEquals("ab", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertEquals("abc", rs.getString(4));
            assertEquals(111, rs.getInt(5));
            assertFalse(rs.next());

            // all single slots with one value.
            query = "SELECT * FROM " + TABLE_WITH_SALTING + 
                    " WHERE a_integer = 1 AND a_string = 'ab' AND a_id = '123'";
            stmt = conn.prepareStatement(query);
            
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals(1, rs.getInt(1));
            assertEquals("ab", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertEquals("abc", rs.getString(4));
            assertEquals(111, rs.getInt(5));
            assertFalse(rs.next());
            
            // all single slots with multiple values.
            if (tgtPH()) query = "SELECT * FROM " + TABLE_WITH_SALTING + 
                    " WHERE a_integer in (2, 4) AND a_string = 'abc' AND a_id = '123'";
            else if (tgtSQ()||tgtTR()) query = "SELECT * FROM " + TABLE_WITH_SALTING +
                    " WHERE a_integer in (2, 4) AND a_string = 'abc' AND a_id = '123' order by 1";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            
            assertTrue(rs.next());
            assertEquals(2, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertEquals("def", rs.getString(4));
            assertEquals(222, rs.getInt(5));
            
            assertTrue(rs.next());
            assertEquals(4, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertEquals("jkl", rs.getString(4));
            assertEquals(444, rs.getInt(5));
            assertFalse(rs.next());
            
            if (tgtPH()) query = "SELECT a_integer, a_string FROM " + TABLE_WITH_SALTING +
                    " WHERE a_integer in (1,2,3,4) AND a_string in ('a', 'abc', 'de') AND a_id = '123'";
            else if (tgtSQ()||tgtTR()) query = "SELECT a_integer, a_string FROM " + TABLE_WITH_SALTING +
                    " WHERE a_integer in (1,2,3,4) AND a_string in ('a', 'abc', 'de') AND a_id = '123' order by 1";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            
            assertTrue(rs.next());
            assertEquals(1, rs.getInt(1));
            assertEquals("de", rs.getString(2));
            
            assertTrue(rs.next());
            assertEquals(2, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            
            assertTrue(rs.next());
            assertEquals(3, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            
            assertTrue(rs.next());
            assertEquals(4, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            assertFalse(rs.next());
            
            // fixed length slot with bounded ranges.
            query = "SELECT a_string, a_id FROM " + TABLE_WITH_SALTING + 
                    " WHERE a_integer > 1 AND a_integer < 4 AND a_string = 'abc' AND a_id = '123'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals("abc", rs.getString(1));
            assertEquals("123", rs.getString(2));
            
            assertTrue(rs.next());
            assertEquals("abc", rs.getString(1));
            assertEquals("123", rs.getString(2));
            assertFalse(rs.next());
            
            // fixed length slot with unbound ranges.
            if (tgtPH()) query = "SELECT b_string, b_integer FROM " + TABLE_WITH_SALTING + 
                    " WHERE a_integer > 1 AND a_string = 'abc' AND a_id = '123'";
            else if (tgtSQ()||tgtTR()) query = "SELECT b_string, b_integer FROM " + TABLE_WITH_SALTING +
                    " WHERE a_integer > 1 AND a_string = 'abc' AND a_id = '123' order by 1";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals("def", rs.getString(1));
            assertEquals(222, rs.getInt(2));
            
            assertTrue(rs.next());
            assertEquals("ghi", rs.getString(1));
            assertEquals(333, rs.getInt(2));
            
            assertTrue(rs.next());
            assertEquals("jkl", rs.getString(1));
            assertEquals(444, rs.getInt(2));
            assertFalse(rs.next());
            
            // Variable length slot with unbounded ranges.
            query = "SELECT * FROM " + TABLE_WITH_SALTING + 
                    " WHERE a_integer = 1 AND a_string > 'ab' AND a_id = '123'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals(1, rs.getInt(1));
            assertEquals("de", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertEquals("abc", rs.getString(4));
            assertEquals(111, rs.getInt(5));
            assertFalse(rs.next());

        } finally {
        }
    }

    @Test
    public void testSelectValueWithNotFullyQualifiedWhereClause() throws Exception {
        printTestDescription();

        try {
            initTableValues();
            
            // Where without fully qualified key, point query.
            String query = "SELECT * FROM " + TABLE_WITH_SALTING + " WHERE a_integer = ? AND a_string = ?";
            PreparedStatement stmt = conn.prepareStatement(query);
            
            stmt.setInt(1, 1);
            stmt.setString(2, "abc");
            ResultSet rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals(1, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            assertEquals("456", rs.getString(3));
            assertEquals("abc", rs.getString(4));
            assertEquals(111, rs.getInt(5));
            assertFalse(rs.next());
            
            // Where without fully qualified key, range query.
            if (tgtPH()) query = "SELECT * FROM " + TABLE_WITH_SALTING + " WHERE a_integer >= 2";
            else if (tgtSQ()||tgtTR()) query = "SELECT * FROM " + TABLE_WITH_SALTING + " WHERE a_integer >= 2 order by 1";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals(2, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertEquals("def", rs.getString(4));
            assertEquals(222, rs.getInt(5));
            
            assertTrue(rs.next());
            assertEquals(3, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertEquals("ghi", rs.getString(4));
            assertEquals(333, rs.getInt(5));
            
            assertTrue(rs.next());
            assertEquals(4, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertEquals("jkl", rs.getString(4));
            assertEquals(444, rs.getInt(5));
            assertFalse(rs.next());
            
            // With point query.
            query = "SELECT a_string FROM " + TABLE_WITH_SALTING + " WHERE a_string = ?";
            stmt = conn.prepareStatement(query);
            stmt.setString(1, "de");
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals("de", rs.getString(1));
            assertFalse(rs.next());
            
            query = "SELECT a_id FROM " + TABLE_WITH_SALTING + " WHERE a_id = ?";
            stmt = conn.prepareStatement(query);
            stmt.setString(1, "456");
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals("456", rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testSelectWithGroupBy() throws Exception {
        printTestDescription();

        try {
            initTableValues();
            
            String query = null;
            if (tgtPH()) query = "SELECT a_integer FROM " + TABLE_WITH_SALTING + " WHERE a_string='abc' LIMIT 1";
            else if (tgtTR()) query = "SELECT a_integer FROM " + TABLE_WITH_SALTING + " WHERE a_string='abc' order by 1 LIMIT 1";
            else if (tgtSQ()) query = "SELECT [first 1] a_integer FROM " + TABLE_WITH_SALTING + " WHERE a_string='abc' order by 1";
            PreparedStatement stmt = conn.prepareStatement(query);
            ResultSet rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals(1, rs.getInt(1));
            assertFalse(rs.next());
        } finally {
        }
    }
}
