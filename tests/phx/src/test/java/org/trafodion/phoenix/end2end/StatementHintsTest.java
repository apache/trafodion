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
 * End-to-End tests on various statement hints.
 */
public class StatementHintsTest extends BaseTest {

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table test_table"));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    private static void initTableValues() throws Exception {
        
        try {
            String ddl = null;
            if (tgtPH()) ddl = "CREATE TABLE test_table" +
                    "   (a_integer integer not null, \n" +
                    "    a_string varchar not null, \n" +
                    "    a_id char(3) not null,\n" +
                    "    b_string varchar \n" +
                    "    CONSTRAINT pk PRIMARY KEY (a_integer, a_string, a_id))\n";
            else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE test_table" +
                    "   (a_integer integer not null, \n" +
                    "    a_string varchar(128) not null, \n" +
                    "    a_id char(3) not null,\n" +
                    "    b_string varchar(128) \n" +
                    "    , CONSTRAINT pk PRIMARY KEY (a_integer, a_string, a_id))\n";

            conn.createStatement().execute(ddl);
       
            conn.setAutoCommit(false);
     
            String query = null;
            PreparedStatement stmt;
            
            if (tgtPH()||tgtTR()) query = "UPSERT INTO test_table"
                    + "(a_integer, a_string, a_id, b_string) "
                    + "VALUES(?,?,?,?)";
            else if (tgtSQ()) query = "INSERT INTO test_table"
                    + "(a_integer, a_string, a_id, b_string) "
                    + "VALUES(?,?,?,?)";
            stmt = conn.prepareStatement(query);
            
            stmt.setInt(1, 1);
            stmt.setString(2, "ab");
            stmt.setString(3, "123");
            stmt.setString(4, "abc");
            stmt.execute();
            
            stmt.setInt(1, 1);
            stmt.setString(2, "abc");
            stmt.setString(3, "456");
            stmt.setString(4, "abc");
            stmt.execute();
            
            stmt.setInt(1, 1);
            stmt.setString(2, "de");
            stmt.setString(3, "123");
            stmt.setString(4, "abc");
            stmt.execute();
            
            stmt.setInt(1, 2);
            stmt.setString(2, "abc");
            stmt.setString(3, "123");
            stmt.setString(4, "def");
            stmt.execute();

            stmt.setInt(1, 3);
            stmt.setString(2, "abc");
            stmt.setString(3, "123");
            stmt.setString(4, "ghi");
            stmt.execute();

            stmt.setInt(1, 4);
            stmt.setString(2, "abc");
            stmt.setString(3, "123");
            stmt.setString(4, "jkl");
            stmt.execute();
            conn.commit();
        } finally {
        }
    }

    @Test
    public void testSelectForceRangeScan() throws Exception {
        printTestDescription();

        try {
            initTableValues();
            String query = null;
            if (tgtPH()) query = "SELECT /*+ RANGE_SCAN */ * FROM test_table WHERE a_integer IN (1, 2, 3, 4)";
            else if (tgtSQ()||tgtTR()) query = "SELECT * FROM test_table WHERE a_integer IN (1, 2, 3, 4) order by 1";
            PreparedStatement stmt = conn.prepareStatement(query);
            ResultSet rs = stmt.executeQuery();
            
            assertTrue(rs.next());
            assertEquals(1, rs.getInt(1));
            assertEquals("ab", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertEquals("abc", rs.getString(4));
            
            assertTrue(rs.next());
            assertTrue(rs.next());
            
            assertTrue(rs.next());
            assertEquals(2, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertEquals("def", rs.getString(4));
            
            assertTrue(rs.next());
            assertTrue(rs.next());
            assertEquals(4, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testSelectForceSkipScan() throws Exception {
        printTestDescription();

        try {
            initTableValues();
            // second slot on the 
            String query = null; 
            if (tgtPH()) query = "SELECT /*+ SKIP_SCAN */ * FROM test_table WHERE a_string = 'abc'";
            else if (tgtSQ()||tgtTR()) query = "SELECT * FROM test_table WHERE a_string = 'abc' order by 1"; 
            PreparedStatement stmt = conn.prepareStatement(query);
            ResultSet rs = stmt.executeQuery();
            
            assertTrue(rs.next());
            assertEquals(1, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            assertEquals("456", rs.getString(3));
            
            assertTrue(rs.next());
            assertTrue(rs.next());
            
            assertTrue(rs.next());
            assertEquals(4, rs.getInt(1));
            assertEquals("abc", rs.getString(2));
            assertEquals("123", rs.getString(3));
            assertFalse(rs.next());
        } finally {
        }
    }
}
