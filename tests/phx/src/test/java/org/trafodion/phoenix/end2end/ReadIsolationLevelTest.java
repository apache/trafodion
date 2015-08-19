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

public class ReadIsolationLevelTest extends BaseTest {

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table " + ATABLE_NAME));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    private static final String ENTITY_ID1= "000000000000001";
    private static final String ENTITY_ID2= "000000000000002";
    private static final String VALUE1 = "a";
    private static final String VALUE2= "b";

    private void initTableValues() throws Exception {
        createTestTable(ATABLE_NAME);

        // Insert all rows at ts
        PreparedStatement stmt = null;
        if (tgtPH()) stmt = conn.prepareStatement(
                "upsert into ATABLE VALUES (?, ?, ?)");
        else if (tgtTR()) stmt = conn.prepareStatement(
                "upsert into ATABLE (ORGANIZATION_ID, ENTITY_ID, A_STRING) VALUES (?, ?, ?)");
        else if (tgtSQ()) stmt = conn.prepareStatement(
                "insert into ATABLE (ORGANIZATION_ID, ENTITY_ID, A_STRING) VALUES (?, ?, ?)");
        stmt.setString(1, tenantId);
        stmt.setString(2, ENTITY_ID1);
        stmt.setString(3, VALUE1);
        stmt.execute(); // should commit too
        
        stmt.setString(2, ENTITY_ID2);
        stmt.setString(3, VALUE2);
        stmt.execute(); // should commit too

    }

    @Test
    public void testStatementReadIsolationLevel() throws Exception {
        printTestDescription();

        initTableValues();
        String query = null;
        if (tgtPH()) query = "SELECT A_STRING FROM ATABLE WHERE ORGANIZATION_ID=? AND ENTITY_ID=?";
        else if (tgtSQ()||tgtTR()) query = "SELECT A_STRING FROM ATABLE WHERE ORGANIZATION_ID=? AND ENTITY_ID=?";

        conn.setAutoCommit(true);

        Connection conn2 = getConnection();
        Connection conn3 = getConnection();
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, ENTITY_ID1);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(VALUE1, rs.getString(1));
            assertFalse(rs.next());

            // Locate existing row and reset one of it's KVs.
            // Insert all rows at ts
            // TRAF: The original test play swith differnt timestamps as SCN
            // in differet conn, this SCN concept does not apply to us.
            if (tgtPH()) {
                PreparedStatement stmt = conn.prepareStatement("upsert into ATABLE VALUES (?, ?, ?)");
                stmt.setString(1, tenantId);
                stmt.setString(2, ENTITY_ID1);
                stmt.setString(3, VALUE2);
                stmt.execute();
            
                PreparedStatement statement2 = conn2.prepareStatement(query);
                statement2.setString(1, tenantId);
                statement2.setString(2, ENTITY_ID1);
                // Run another query through same connection and make sure
                // you can find the new row
                rs = statement2.executeQuery();
                assertTrue(rs.next());
                assertEquals(VALUE2, rs.getString(1));
                assertFalse(rs.next());
            }

            PreparedStatement statement3 = conn3.prepareStatement(query);
            statement3.setString(1, tenantId);
            statement3.setString(2, ENTITY_ID1);
            rs = statement3.executeQuery();
            assertTrue(rs.next());
            assertEquals(VALUE1, rs.getString(1));
            assertFalse(rs.next());
        } finally {
            conn2.close();
            conn3.close();
        }
    }

    @Test
    public void testConnectionReadIsolationLevel() throws Exception {
        printTestDescription();

        initTableValues();
        String query = "SELECT A_STRING FROM ATABLE WHERE ORGANIZATION_ID=? AND ENTITY_ID=?";
        conn.setAutoCommit(true);
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, ENTITY_ID1);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(VALUE1, rs.getString(1));
            assertFalse(rs.next());

            // Locate existing row and reset one of it's KVs.
            // Insert all rows at ts
            // TRAF: this test does upsert using an older ts as SCN, so that it 
            // did not take effect?
            if (tgtPH()) {
                PreparedStatement stmt = conn.prepareStatement("upsert into ATABLE VALUES (?, ?, ?)");
                stmt.setString(1, tenantId);
                stmt.setString(2, ENTITY_ID1);
                stmt.setString(3, VALUE2);
                stmt.execute();
            } 
            // Run another query through same connection and make sure
            // you can't find the new row
            rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(VALUE1, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
}
