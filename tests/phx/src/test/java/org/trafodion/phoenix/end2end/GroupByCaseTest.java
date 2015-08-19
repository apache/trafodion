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

public class GroupByCaseTest extends BaseTest {

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table " + GROUPBYTEST_NAME));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    private void createTable() throws Exception {
        createTestTable(GROUPBYTEST_NAME);
    }

    private void loadData() throws SQLException {
        insertRow("Report1", 10);
        insertRow("Report2", 10);
        insertRow("Report3", 30);
        insertRow("Report4", 30);
        insertRow("SOQL1", 10);
        insertRow("SOQL2", 10);
        insertRow("SOQL3", 30);
        insertRow("SOQL4", 30);
    }

    private int id;

    private void insertRow(String uri, int appcpu) throws SQLException {
        PreparedStatement statement = null;

        if (tgtPH()||tgtTR()) statement = conn.prepareStatement("UPSERT INTO " + GROUPBYTEST_NAME + "(id, uri, appcpu) values (?,?,?)");
        else if (tgtSQ()) statement = conn.prepareStatement("INSERT INTO " + GROUPBYTEST_NAME + "(id, uri, appcpu) values (?,?,?)");
        statement.setString(1, "id" + id);
        statement.setString(2, uri);
        statement.setInt(3, appcpu);
        statement.executeUpdate();
        id++;
    }

    @Test
    public void testGroupByCase() throws Exception {
        printTestDescription();

        String GROUPBY1 = null;
        String GROUPBY2 = null;
        String GROUPBY3 = null;

        if (tgtPH()) {
            GROUPBY1 = "select " +
                "case when uri LIKE 'Report%' then 'Reports' else 'Other' END category" +
                ", avg(appcpu) from " + GROUPBYTEST_NAME +
                " group by category";
            GROUPBY2 = "select " +
                "case uri when 'Report%' then 'Reports' else 'Other' END category" +
                ", avg(appcpu) from " + GROUPBYTEST_NAME +
                " group by appcpu, category";
            GROUPBY3 = "select " +
                "case uri when 'Report%' then 'Reports' else 'Other' END category" +
                ", avg(appcpu) from " + GROUPBYTEST_NAME +
                " group by avg(appcpu), category";
        } else if (tgtSQ()||tgtTR()) {
            GROUPBY1 = "select " +
                "case when uri LIKE 'Report%' then 'Reports' else 'Other' END AS category" +
                ", avg(appcpu) from " + GROUPBYTEST_NAME +
                // TRAF " group by category";
                /* TRAF */ " group by uri";
            GROUPBY2 = "select " +
                "case uri when 'Report%' then 'Reports' else 'Other' END AS category" +
                ", avg(appcpu) from " + GROUPBYTEST_NAME +
                // TRAF " group by appcpu, category";
                /* TRAF */ " group by appcpu, uri";
            GROUPBY3 = "select " +
                "case uri when 'Report%' then 'Reports' else 'Other' END AS category" +
                ", avg(appcpu) from " + GROUPBYTEST_NAME +
                // TRAF " group by avg(appcpu), category";
                /* TRAF */ " group by avg(appcpu), uri";
        }

        GroupByCaseTest gbt = new GroupByCaseTest();
        gbt.createTable();
        gbt.loadData();
        gbt.executeQuery(conn,GROUPBY1);
        gbt.executeQuery(conn,GROUPBY2);
        // TODO: validate query results
        try {
            gbt.executeQuery(conn,GROUPBY3);
            fail();
        } catch (SQLException e) {
            if (tgtPH()) assertTrue(e.getMessage().contains("Aggregate expressions may not be used in GROUP BY"));
            else if (tgtSQ()||tgtTR()) assertTrue(e.getMessage().contains("*** ERROR[4197] This expression cannot be used in the GROUP BY clause."));
        }
     
    }

    @Test
    public void testScanUri() throws Exception {
        printTestDescription();

        GroupByCaseTest gbt = new GroupByCaseTest();
        gbt.createTable();
        gbt.loadData();
        Statement stmt = conn.createStatement();
        ResultSet rs = null;
        if (tgtPH()) rs = stmt.executeQuery("select uri from " + GROUPBYTEST_NAME);
        else if (tgtSQ()||tgtTR()) rs = stmt.executeQuery("select uri from " + GROUPBYTEST_NAME + " order by 1");
        assertTrue(rs.next());
        assertEquals("Report1", rs.getString(1));
        assertTrue(rs.next());
        assertEquals("Report2", rs.getString(1));
        assertTrue(rs.next());
        assertEquals("Report3", rs.getString(1));
        assertTrue(rs.next());
        assertEquals("Report4", rs.getString(1));
        assertTrue(rs.next());
        assertEquals("SOQL1", rs.getString(1));
        assertTrue(rs.next());
        assertEquals("SOQL2", rs.getString(1));
        assertTrue(rs.next());
        assertEquals("SOQL3", rs.getString(1));
        assertTrue(rs.next());
        assertEquals("SOQL4", rs.getString(1));
        assertFalse(rs.next());
    }

    @Test
    public void testCount() throws Exception {
        printTestDescription();

        GroupByCaseTest gbt = new GroupByCaseTest();
        gbt.createTable();
        gbt.loadData();
        Statement stmt = conn.createStatement();
        ResultSet rs = stmt.executeQuery("select count(1) from " + GROUPBYTEST_NAME);
        assertTrue(rs.next());
        assertEquals(8, rs.getInt(1));
        assertFalse(rs.next());
    }

/* TRAF
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(
            value="RV_RETURN_VALUE_IGNORED",
            justification="Test code.")
*/

    private void executeQuery(Connection conn, String query) throws SQLException {
        PreparedStatement st = conn.prepareStatement(query);
        st.executeQuery();
    }
}
