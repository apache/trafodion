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
import java.sql.Date;
import java.util.*;

/**
 * 
 * Extended tests for Phoenix JDBC implementation
 * 
 */
public class ExtendedQueryExecTest extends BaseTest {

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
    public void testToDateFunctionBind() throws Exception {
        printTestDescription();

        Date date = new Date(1); //1970-01-01
        initATableValues(date);

        try {
            String query = null;
            if (tgtPH()) query = "SELECT a_date FROM atable WHERE organization_id='" + tenantId + "' and a_date < TO_DATE(?)";
            else if (tgtSQ()||tgtTR()) query = "SELECT a_date FROM atable WHERE organization_id='" + tenantId + "' and a_date < ?";
            PreparedStatement statement = conn.prepareStatement(query);
            if (tgtPH()) statement.setString(1, "1970-1-1 12:00:00");
            else if (tgtSQ()||tgtTR()) statement.setString(1, "1970-1-2");
            ResultSet rs = statement.executeQuery();
            verifyDateResultSet(rs, date, 3);
        } finally {
        }
    }

    /* TRAF
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(
            value="RV_RETURN_VALUE_IGNORED",
            justification="Test code.")
     */

    @Test
    public void testTypeMismatchToDateFunctionBind() throws Exception {
        printTestDescription();

        initATableValues();

        try {
            String query = null;
            if (tgtPH()) query = "SELECT a_date FROM atable WHERE organization_id='" + tenantId + "' and a_date < TO_DATE(?)";
            else if (tgtSQ()||tgtTR()) query = "SELECT a_date FROM atable WHERE organization_id='" + tenantId + "' and a_date < ?";
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setDate(1, new Date(2));
            statement.executeQuery();
            if (tgtPH()) fail();
        } catch (SQLException e) {
            if (tgtPH()) assertTrue(e.getMessage().contains("Type mismatch. expected: [VARCHAR] but was: DATE at TO_DATE"));
        } finally {
        }
    }

    /**
     * Basic tests for date function
     * Related bug: W-1190856
     * @throws Exception
     */
    @Test
    public void testDateFunctions() throws Exception {
        printTestDescription();

        Date date = new Date(1);
        initATableValues(date);

        try {
            ResultSet rs;
            String queryPrefix = "SELECT a_date FROM atable WHERE organization_id='" + tenantId + "' and ";

            String queryDateArg = null;
            if (tgtPH()) queryDateArg = "a_date < TO_DATE('1970-1-1 12:00:00')";
            else if (tgtSQ()||tgtTR()) queryDateArg = "a_date < DATE '1970-01-02'";
            rs = getResultSet(conn, queryPrefix + queryDateArg);
            verifyDateResultSet(rs, date, 3);

            // TODO: Bug #1 - Result should be the same as the the case above
//          queryDateArg = "a_date < TO_DATE('70-1-1 12:0:0')";
//          rs = getResultSet(conn, queryPrefix + queryDateArg);
//          verifyDateResultSet(rs, date, 3);

            // TODO: Bug #2 - Exception should be generated for invalid date/time
//          queryDateArg = "a_date < TO_DATE('999-13-32 24:60:60')";
//          try {
//              getResultSet(conn, queryPrefix + queryDateArg);
//              fail("Expected SQLException");
//          } catch (SQLException ex) {
//              // expected
//          }
            
            if (tgtPH()) queryDateArg = "a_date >= TO_DATE('1970-1-2 23:59:59') and a_date <= TO_DATE('1970-1-3 0:0:1')";
            else if (tgtSQ()||tgtTR()) queryDateArg = "a_date > DATE '1970-01-02' and a_date <= DATE '1970-01-03'";
            rs = getResultSet(conn, queryPrefix + queryDateArg);
            verifyDateResultSet(rs, new Date(date.getTime() + (2*60*60*24*1000)), 3);

        } finally {
        }
    }
    
    /**
     * aggregation - group by
     * @throws Exception
     */
    @Test
    public void testDateGroupBy() throws Exception {
        printTestDescription();

        Date date = new Date(1);
        initATableValues(date);

        try {
            ResultSet rs;
            String query = null;
            if (tgtPH()) query = "SELECT a_date, count(1) FROM atable WHERE organization_id='" + tenantId + "' group by a_date";
            else if (tgtSQ()||tgtTR()) query = "SELECT a_date, count(1) FROM atable WHERE organization_id='" + tenantId + "' group by a_date order by 1";
            rs = getResultSet(conn, query);
            
            /* 3 rows in expected result:
             * 1969-12-31   3
             * 1970-01-01   3
             * 1970-01-02   3
             * */
                        
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(date, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(date.toString(), rs.getDate(1).toString());
            assertEquals(3, rs.getInt(2));
            
            // the following assertions fails
            assertTrue(rs.next());
            assertEquals(3, rs.getInt(2));
            assertTrue(rs.next());
            assertEquals(3, rs.getInt(2));
            assertFalse(rs.next());
            

        } finally {
        }
    }
    
    private ResultSet getResultSet(Connection conn, String query) throws SQLException {
        PreparedStatement statement = conn.prepareStatement(query);
        ResultSet rs = statement.executeQuery();
        return rs;
    }
    
    private void verifyDateResultSet(ResultSet rs, Date date, int rowCount) throws SQLException {
        for (int i=0; i<rowCount; i++) {
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(date, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(date.toString(), rs.getDate(1).toString());
        }
        assertFalse(rs.next());
    }
}
