// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
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


public class TopNTest extends BaseTest {

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

    @Test
    public void testMultiOrderByExpr() throws Exception {
        printTestDescription();

        initATableValues();
        String query = null;
        if (tgtPH()||tgtTR()) query = "SELECT entity_id FROM aTable ORDER BY b_string, entity_id LIMIT 5";
        else if (tgtSQ()) query = "SELECT [first 5] entity_id FROM aTable ORDER BY b_string, entity_id";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW1);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW4);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW7);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW2);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW5);

            assertFalse(rs.next());
        } finally {
        }
    }
    

    @Test
    public void testDescMultiOrderByExpr() throws Exception {
        printTestDescription();

        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable ORDER BY b_string || entity_id desc LIMIT 5";
        else if (tgtTR()) query = "SELECT entity_id, CONCAT(b_string, entity_id) as c FROM aTable ORDER BY c desc LIMIT 5";
        else if (tgtSQ()) query = "SELECT [first 5] entity_id, CONCAT(b_string, entity_id) as c FROM aTable ORDER BY c desc";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW9);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW6);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW3);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW8);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW5);

            assertFalse(rs.next());
        } finally {
        }
    }
    

    @Test
    public void testTopNDeleteAutoCommitOn() throws Exception {
        printTestDescription();

        testTopNDelete(true);
    }
    
    @Test
    public void testTopNDeleteAutoCommitOff() throws Exception {
        printTestDescription();

        testTopNDelete(false);
    }
    
    private void testTopNDelete(boolean autoCommit) throws Exception {
        initATableValues();
        String query = null;
        if (tgtPH()) query = "DELETE FROM aTable ORDER BY b_string, entity_id LIMIT 5";
        // TRAF: Do not support ORDER BY in DELETE, nor in a subquery.
        // So there is no way we can do [first N] delete and be sure which
        // rows are deleted.
        else if (tgtSQ()||tgtTR()) query = "DELETE FROM aTable where entity_id in ('" + ROW1 + "','" + ROW2 + "','" + ROW4 + "','" + ROW5 + "','" + ROW7 + "')";
        conn.setAutoCommit(autoCommit);
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.execute();
            assertEquals(5,statement.getUpdateCount());
            if (!autoCommit) {
                conn.commit();
            }
        } finally {
            conn.close();
        }

        conn = getConnection();
        if (tgtPH()) query = "SELECT entity_id FROM aTable ORDER BY b_string, x_decimal nulls last, 8-a_integer LIMIT 5";
        else if (tgtTR()) query = "SELECT entity_id FROM aTable ORDER BY b_string, x_decimal, a_integer desc LIMIT 5";
        else if (tgtSQ()) query = "SELECT [first 5] entity_id FROM aTable ORDER BY b_string, x_decimal, a_integer desc";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW8, rs.getString(1));
            assertTrue (rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertTrue (rs.next());
            assertEquals(ROW6, rs.getString(1));
            assertTrue (rs.next());
            assertEquals(ROW3, rs.getString(1));

            assertFalse(rs.next());
        } finally {
        }
    }
    

}
