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

public class AlterTableTest extends BaseTest {

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

    @Test
    public void testAlterTableWithVarBinaryKey() throws Exception {
        printTestDescription();

        conn.setAutoCommit(true);
        try {
            String ddl = null;
            if (tgtPH()) ddl = "CREATE TABLE test_table " +
                                    "  (a_string varchar not null, a_binary varbinary not null, col1 integer" +
                                    "  CONSTRAINT pk PRIMARY KEY (a_string, a_binary))\n";
            else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE test_table " +
                                    "  (a_string varchar(128) not null, a_binary varbinary not null, col1 integer" +
                                    "  CONSTRAINT pk PRIMARY KEY (a_string, a_binary))\n";
            conn.createStatement().execute(ddl);
            if (tgtPH()) ddl = "ALTER TABLE test_table ADD b_string VARCHAR NULL PRIMARY KEY";
            else if (tgtSQ()||tgtTR()) ddl = "ALTER TABLE test_table ADD b_string VARCHAR(128) NULL PRIMARY KEY";
            PreparedStatement stmt = conn.prepareStatement(ddl);
            stmt.execute();
            fail("Should have caught bad alter.");
        } catch (Exception e) {
            assertTrue(e.getMessage(), e.getMessage().contains("*** ERROR[15001] A syntax error occurred at or before"));
        } finally {
        }
    }


    @Test
    public void testAddVarCharColToPK() throws Exception {
        printTestDescription();

        conn.setAutoCommit(true);
        try {
            String ddl = null;
            if (tgtPH()) ddl = "CREATE TABLE test_table " +
                               "  (a_string varchar not null, col1 integer" +
                               "  CONSTRAINT pk PRIMARY KEY (a_string))\n";
            else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE test_table " +
                                    "  (a_string varchar(128) not null, col1 integer, " +
                                    "  CONSTRAINT pk1 PRIMARY KEY (a_string))\n";
            conn.createStatement().execute(ddl);
            
            String dml = null;
            if (tgtPH()) dml = "UPSERT INTO test_table VALUES(?)";
            else if (tgtTR()) dml = "UPSERT INTO test_table VALUES(?, ?)";
            else if (tgtSQ()) dml = "INSERT INTO test_table VALUES(?, ?)";  
            PreparedStatement stmt = conn.prepareStatement(dml);
            if (tgtPH()) {
                stmt.setString(1, "b");
                stmt.execute();
                stmt.setString(1, "a");
                stmt.execute();
            } else if (tgtSQ()||tgtTR()) {
                stmt.setString(1, "b");
                stmt.setNull(2, java.sql.Types.INTEGER);
                stmt.execute();
                stmt.setString(1, "a");
                stmt.setNull(2, java.sql.Types.INTEGER);
                stmt.execute();
            }
           
            String query = null; 
            if (tgtPH()) query = "SELECT * FROM test_table";
            else if (tgtSQ()||tgtTR()) query = "SELECT * FROM test_table order by 1"; 
            ResultSet rs = conn.createStatement().executeQuery(query);
            assertTrue(rs.next());
            assertEquals("a",rs.getString(1));
            assertTrue(rs.next());
            assertEquals("b",rs.getString(1));
            assertFalse(rs.next());
            
            if (tgtPH()) ddl = "ALTER TABLE test_table ADD b_string VARCHAR NULL PRIMARY KEY";
            else if (tgtSQ()||tgtTR()) ddl = "ALTER TABLE test_table ADD b_string VARCHAR(128) DEFAULT NULL";
            conn.createStatement().execute(ddl);
            
            query = "SELECT * FROM test_table WHERE a_string = 'a' AND b_string IS NULL";
            rs = conn.createStatement().executeQuery(query);
            assertTrue(rs.next());
            assertEquals("a",rs.getString(1));
            assertFalse(rs.next());
            
            if (tgtPH()) dml = "UPSERT INTO test_table VALUES(?)";
            else if (tgtTR()) dml = "UPSERT INTO test_table VALUES(?,?,?)";
            else if (tgtSQ()) dml = "INSERT INTO test_table VALUES(?,?,?)";
            stmt = conn.prepareStatement(dml);
            if (tgtPH()) {
                stmt.setString(1, "c");
            } else if (tgtSQ()||tgtTR()) {
                stmt.setString(1, "c");
                stmt.setNull(2, java.sql.Types.INTEGER);
                stmt.setNull(3, java.sql.Types.VARCHAR);
            }
            stmt.execute();
           
            query = "SELECT * FROM test_table WHERE a_string = 'c' AND b_string IS NULL";
            rs = conn.createStatement().executeQuery(query);
            assertTrue(rs.next());
            assertEquals("c",rs.getString(1));
            assertFalse(rs.next());
            
            if (tgtPH()||tgtTR()) dml = "UPSERT INTO test_table(a_string,col1) VALUES(?,?)";
            else if (tgtSQ()) dml = "INSERT INTO test_table(a_string,col1) VALUES(?,?)";
            stmt = conn.prepareStatement(dml);
            if (tgtPH()) stmt.setString(1, "a");
            else if (tgtSQ()||tgtTR()) stmt.setString(1, "d"); // a_string is primary key
            stmt.setInt(2, 5);
            stmt.execute();
          
            if (tgtPH()) { 
                query = "SELECT a_string,col1 FROM test_table WHERE a_string = 'a' AND b_string IS NULL";
                rs = conn.createStatement().executeQuery(query);
                assertTrue(rs.next());
                assertEquals("a",rs.getString(1));
            } else if (tgtSQ()||tgtTR()) {
                query = "SELECT a_string,col1 FROM test_table WHERE a_string = 'd' AND b_string IS NULL";
                rs = conn.createStatement().executeQuery(query);
                assertTrue(rs.next());
                assertEquals("d",rs.getString(1));
            }
            assertEquals(5,rs.getInt(2)); // TODO: figure out why this flaps
            assertFalse(rs.next());
            
        } finally {
        }
    }
}
