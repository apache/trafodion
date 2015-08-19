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

public class BinaryRowKeyTest extends BaseTest {

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

    private void initTableValues() throws SQLException {
        
        try {
            String ddl = null;
            if (tgtPH()) ddl = "CREATE TABLE test_table" +
                    "   (a_binary binary(10) not null, \n" +
                    "    a_string varchar not null, \n" +
                    "    b_binary varbinary \n" +
                    "    CONSTRAINT pk PRIMARY KEY (a_binary, a_string))\n";
            else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE test_table" +
                    "   (a_binary binary(10) not null, \n" +
                    "    a_string varchar(128) not null, \n" +
                    "    b_binary varchar(128) \n" +
                    "    , CONSTRAINT pk PRIMARY KEY (a_binary, a_string))\n";
            conn.createStatement().execute(ddl);
       
            conn.setAutoCommit(false);
     
            String query = null;
            PreparedStatement stmt;
            
            if (tgtPH()||tgtTR()) query = "UPSERT INTO test_table"
                    + "(a_binary, a_string) "
                    + "VALUES(?,?)";
            else if (tgtSQ()) query = "INSERT INTO test_table"
                    + "(a_binary, a_string) "
                    + "VALUES(?,?)";
            stmt = conn.prepareStatement(query);
            
            stmt.setBytes(1, new byte[] {0,0,0,0,0,0,0,0,0,1});
            stmt.setString(2, "a");
            stmt.execute();
            
            stmt.setBytes(1, new byte[] {0,0,0,0,0,0,0,0,0,2});
            stmt.setString(2, "b");
            stmt.execute();
            conn.commit();
            
        } finally {
        }
    }

    @Test
    public void testInsertPaddedBinaryValue() throws SQLException {
        printTestDescription();

        try {
            initTableValues();
            conn.setAutoCommit(true);
            conn.createStatement().execute("DELETE FROM test_table");
           
            String query = null;
            if (tgtPH()||tgtTR()) query = "UPSERT INTO test_table"
                    + "(a_binary, a_string) "
                    + "VALUES(?,?)";
            else if (tgtSQ()) query = "INSERT INTO test_table"
                    + "(a_binary, a_string) "
                    + "VALUES(?,?)";
            PreparedStatement stmt = conn.prepareStatement(query);
            stmt.setBytes(1, new byte[] {0,0,0,0,0,0,0,0,1});
            stmt.setString(2, "a");
            stmt.execute();
            
            ResultSet rs = conn.createStatement().executeQuery("SELECT a_string FROM test_table");
            assertTrue(rs.next());
            assertEquals("a",rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testSelectValues() throws SQLException {
        printTestDescription();
        
        try {
            initTableValues();
            
            String query = null;
            if (tgtPH()) query = "SELECT * FROM test_table";
            else if (tgtSQ()||tgtTR()) query = "SELECT * FROM test_table order by 1";
            PreparedStatement stmt = conn.prepareStatement(query);
            ResultSet rs = stmt.executeQuery();
            
            assertTrue(rs.next());
            assertArrayEquals(new byte[] {0,0,0,0,0,0,0,0,0,1}, rs.getBytes(1));
            assertEquals("a", rs.getString(2));
            
            assertTrue(rs.next());
            assertArrayEquals(new byte[] {0,0,0,0,0,0,0,0,0,2}, rs.getBytes(1));
            assertEquals("b", rs.getString(2));
            
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testUpsertSelectValues() throws SQLException {
        printTestDescription();
        
        try {
            initTableValues();
            
            String query = null;
            if (tgtPH()||tgtTR()) query = "UPSERT INTO test_table (a_binary, a_string, b_binary) "
                    + " SELECT a_binary, a_string, a_binary FROM test_table";
            else if (tgtSQ()) query = "UPDATE test_table tab1 set b_binary=(select a_binary FROM test_table tab2 where tab1.a_string=tab2.a_string)";

            PreparedStatement stmt = conn.prepareStatement(query);
            stmt.execute();
            conn.commit();
            
            if (tgtPH()) query = "SELECT a_binary, b_binary FROM test_table";
            else if (tgtSQ()||tgtTR()) query = "SELECT a_binary, b_binary FROM test_table order by 1";
            stmt = conn.prepareStatement(query);
            ResultSet rs = stmt.executeQuery();
            
            assertTrue(rs.next());
            assertArrayEquals(new byte[] {0,0,0,0,0,0,0,0,0,1}, rs.getBytes(1));
            assertArrayEquals(new byte[] {0,0,0,0,0,0,0,0,0,1}, rs.getBytes(2));
            
            assertTrue(rs.next());
            assertArrayEquals(new byte[] {0,0,0,0,0,0,0,0,0,2}, rs.getBytes(1));
            assertArrayEquals(new byte[] {0,0,0,0,0,0,0,0,0,2}, rs.getBytes(2));
            assertFalse(rs.next());
        } finally {
        }
    }
}
