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

public class MultiCfQueryExecTest extends BaseTest {
    private static final String MULTI_CF = "MULTI_CF";

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table " + MULTI_CF));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */
   
    protected void initTableValues() throws Exception {
        createTestTable(MULTI_CF);

        conn.setAutoCommit(true);
        // Insert all rows at ts
        PreparedStatement stmt = null;
        if (tgtPH()) stmt = conn.prepareStatement(
                "upsert into " +
                "MULTI_CF(" +
                "    ID, " +
                "    TRANSACTION_COUNT, " +
                "    CPU_UTILIZATION, " +
                "    DB_CPU_UTILIZATION," +
                "    UNIQUE_USER_COUNT," +
                "    F.RESPONSE_TIME," +
                "    G.RESPONSE_TIME)" +
                "VALUES (?, ?, ?, ?, ?, ?, ?)");
        else if (tgtTR()) stmt = conn.prepareStatement(
                "upsert into " +
                "MULTI_CF(" +
                "    ID, " +
                "    TRANSACTION_COUNT, " +
                "    CPU_UTILIZATION, " +
                "    DB_CPU_UTILIZATION," +
                "    UNIQUE_USER_COUNT," +
                "    F_RESPONSE_TIME," +
                "    G_RESPONSE_TIME)" +
                "VALUES (?, ?, ?, ?, ?, ?, ?)");
        else if (tgtSQ()) stmt = conn.prepareStatement(
                "insert into " +
                "MULTI_CF(" +
                "    ID, " +
                "    TRANSACTION_COUNT, " +
                "    CPU_UTILIZATION, " +
                "    DB_CPU_UTILIZATION," +
                "    UNIQUE_USER_COUNT," +
                "    F_RESPONSE_TIME," +
                "    G_RESPONSE_TIME)" +
                "VALUES (?, ?, ?, ?, ?, ?, ?)");

        stmt.setString(1, "000000000000001");
        stmt.setInt(2, 100);
        stmt.setBigDecimal(3, BigDecimal.valueOf(0.5));
        stmt.setBigDecimal(4, BigDecimal.valueOf(0.2));
        stmt.setInt(5, 1000);
        stmt.setLong(6, 11111);
        stmt.setLong(7, 11112);
        stmt.execute();
        stmt.setString(1, "000000000000002");
        stmt.setInt(2, 200);
        stmt.setBigDecimal(3, BigDecimal.valueOf(2.5));
        stmt.setBigDecimal(4, BigDecimal.valueOf(2.2));
        stmt.setInt(5, 2000);
        stmt.setLong(6, 2222);
        stmt.setLong(7, 22222);
        stmt.execute();
    }
    
    @Test
    public void testConstantCount() throws Exception {
        printTestDescription();

        String query = "SELECT count(1) from multi_cf";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(2, rs.getLong(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testCFToDisambiguateInSelectOnly1() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT F.RESPONSE_TIME,G.RESPONSE_TIME from multi_cf where ID = '000000000000002'";
        else if (tgtSQ()||tgtTR()) query = "SELECT F_RESPONSE_TIME,G_RESPONSE_TIME from multi_cf where ID = '000000000000002'";

        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(2222, rs.getLong(1));
            assertEquals(22222, rs.getLong(2));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testCFToDisambiguateInSelectOnly2() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT F.RESPONSE_TIME,G.RESPONSE_TIME from multi_cf where TRANSACTION_COUNT = 200";
        else if (tgtSQ()||tgtTR()) query = "SELECT F_RESPONSE_TIME,G_RESPONSE_TIME from multi_cf where TRANSACTION_COUNT = 200";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(2222, rs.getLong(1));
            assertEquals(22222, rs.getLong(2));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testCFToDisambiguate1() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT F.RESPONSE_TIME,G.RESPONSE_TIME from multi_cf where F.RESPONSE_TIME = 2222";
        else if (tgtSQ()||tgtTR()) query = "SELECT F_RESPONSE_TIME,G_RESPONSE_TIME from multi_cf where F_RESPONSE_TIME = 2222";

        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(2222, rs.getLong(1));
            assertEquals(22222, rs.getLong(2));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testCFToDisambiguate2() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT F.RESPONSE_TIME,G.RESPONSE_TIME from multi_cf where G.RESPONSE_TIME-1 = F.RESPONSE_TIME";
        else if (tgtSQ()||tgtTR()) query = "SELECT F_RESPONSE_TIME,G_RESPONSE_TIME from multi_cf where G_RESPONSE_TIME-1 = F_RESPONSE_TIME";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(11111, rs.getLong(1));
            assertEquals(11112, rs.getLong(2));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test 
    public void testDefaultCFToDisambiguate() throws Exception {
        printTestDescription();

        initTableValues();
        String ddl = "ALTER TABLE multi_cf ADD response_time BIGINT";
        conn.createStatement().execute(ddl);

        String dml = null;
        if (tgtPH()||tgtTR()) dml = "upsert into " +
        "MULTI_CF(" +
        "    ID, " +
        "    RESPONSE_TIME)" +
        "VALUES ('000000000000003', 333)";
        else if (tgtSQ()) dml = "insert into " +
        "MULTI_CF(" +
        "    ID, " +
        "    RESPONSE_TIME)" +
        "VALUES ('000000000000003', 333)";
        conn.createStatement().execute(dml);

        String query = "SELECT ID,RESPONSE_TIME from multi_cf where RESPONSE_TIME = 333";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("000000000000003", rs.getString(1));
            assertEquals(333, rs.getLong(2));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testEssentialColumnFamilyForRowKeyFilter() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT F.RESPONSE_TIME,G.RESPONSE_TIME from multi_cf where SUBSTR(ID, 15) = '2'";
        else if (tgtSQ()||tgtTR()) query = "SELECT F_RESPONSE_TIME,G_RESPONSE_TIME from multi_cf where SUBSTR(ID, 15) = '2'";

        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(2222, rs.getLong(1));
            assertEquals(22222, rs.getLong(2));
            assertFalse(rs.next());
        } finally {
        }
    }
    
}
