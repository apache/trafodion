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
/**********************************
 *
 * Later modifications to test Trafodion instead of Phoenix were granted to ASF.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
*************************************/

package test.java.org.trafodion.phoenix.end2end;

import static org.junit.Assert.*;
import org.junit.*;
import java.sql.*;
import java.util.*;

public class FunkyNamesTest extends BaseTest {

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table " + FUNKY_NAME));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    protected void initTableValues() throws Exception {
        createTestTable(FUNKY_NAME);

        conn.setAutoCommit(true);
        // Insert all rows at ts
        PreparedStatement stmt = null;
        if (tgtPH()||tgtTR()) stmt = conn.prepareStatement(
                "upsert into " +
                "FUNKY_NAMES(" +
                "    \"foo!\", " +
                "    \"#@$\", " +
                "    \"foo.bar-bas\", " +
                "    \"_blah^\"," +
                "    \"Value\", " +
                "    \"VALUE\", " +
                "    \"value\") " +
                "VALUES (?, ?, ?, ?, ?, ?, ?)");
        else if (tgtSQ()) stmt = conn.prepareStatement(
                "insert into " +
                "FUNKY_NAMES(" +
                "    \"foo!\", " +
                "    \"#@$\", " +
                "    \"foo.bar-bas\", " +
                "    \"_blah^\"," +
                "    \"Value\", " +
                "    \"VALUE\", " +
                "    \"value\") " +
                "VALUES (?, ?, ?, ?, ?, ?, ?)");
        stmt.setString(1, "a");
        stmt.setString(2, "b");
        stmt.setString(3, "c");
        stmt.setString(4, "d");
        stmt.setInt(5, 1);
        stmt.setInt(6, 2);
        stmt.setInt(7, 3);
        stmt.executeUpdate();
    }

    @Test
    public void testUnaliasedFunkyNames() throws Exception {
        printTestDescription();

        String query = "SELECT \"foo!\",\"#@$\",\"foo.bar-bas\",\"_blah^\" FROM FUNKY_NAMES";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("a", rs.getString(1));
            assertEquals("b", rs.getString(2));
            assertEquals("c", rs.getString(3));
            assertEquals("d", rs.getString(4));
            
            assertEquals("a", rs.getString("foo!"));
            assertEquals("b", rs.getString("#@$"));
            assertEquals("c", rs.getString("foo.bar-bas"));
            assertEquals("d", rs.getString("_blah^"));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testCaseSensitive() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT \"Value\",\"VALUE\",\"value\" FROM FUNKY_NAMES";
        else if (tgtSQ()||tgtTR()) query = "SELECT \"Value\",\"VALUE\",\"value\" FROM FUNKY_NAMES";

        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(1, rs.getInt(1));
            assertEquals(2, rs.getInt(2));
            assertEquals(3, rs.getInt(3));
           
            if (tgtPH()) {
                assertEquals(1, rs.getInt("Value"));
                assertEquals(2, rs.getInt("VALUE"));
                assertEquals(3, rs.getInt("value"));
            } else if (tgtSQ()||tgtTR()) {
                // TRAF: In both SQ and TR, delimited identifiers are
                // case-insensitive.  The following 2 are the same as
                // the previous one.  It has been decided that this
                // behavior will remain the same.
                assertEquals(3, rs.getInt("Value"));
                assertEquals(3, rs.getInt("VALUE"));
                assertEquals(3, rs.getInt("value"));
            }
            try {
                if (tgtPH()) {
                   rs.getInt("vAlue");
                   fail();
                } else if (tgtSQ()||tgtTR()) {
                   rs.getInt("NonExistentName");
                   fail();
                }
            } catch (Exception e) {
            }
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testAliasedFunkyNames() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT \"1-3.4$\".\"foo!\" as \"1-2\",\"#@$\" as \"[3]\",\"foo.bar-bas\" as \"$$$\",\"_blah^\" \"0\" FROM FUNKY_NAMES \"1-3.4$\"";
        else if (tgtSQ()||tgtTR()) query = "SELECT \"1-3.4$\".\"foo!\" as \"1-2\",\"#@$\" as \"[3]\",\"foo.bar-bas\" as \"ABC$$$\",\"_blah^\" \"0\" FROM FUNKY_NAMES \"1-3.4$\"";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("a", rs.getString("1-2"));
            assertEquals("b", rs.getString("[3]"));
            if (tgtPH()) assertEquals("c", rs.getString("$$$"));
            else if (tgtSQ()||tgtTR()) assertEquals("c", rs.getString("ABC$$$"));
            assertEquals("d", rs.getString("0"));
            assertFalse(rs.next());
        } finally {
        }
    }
}

