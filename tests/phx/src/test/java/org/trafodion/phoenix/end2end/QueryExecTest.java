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
import java.math.*;
import java.sql.*;
import java.sql.Date;
import java.util.*;
// TRAF import org.apache.hadoop.hbase.filter.CompareFilter.CompareOp;
// TRAF import org.apache.hadoop.hbase.io.ImmutableBytesWritable;
// TRAF import org.apache.hadoop.hbase.util.Bytes;

/**
 * 
 * Basic tests for Phoenix JDBC implementation
 *
 * @author jtaylor
 * @since 0.1
 */
public class QueryExecTest extends BaseTest {
  
    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table " + ATABLE_NAME, "table SumDoubleTest"));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    @Test
    public void testScan() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT a_string, /* comment ok? */ b_string FROM aTable WHERE ?=organization_id and 5=a_integer";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_string, b_string FROM aTable WHERE ?=organization_id and 5=a_integer";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), B_VALUE);
            assertEquals(rs.getString("B_string"), C_VALUE);
            assertFalse(rs.next());
        } finally {
        } 
    }
    
    @Test
    public void testScanByByteValue() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT a_string, /* comment ok? */ b_string, a_byte FROM aTable WHERE ?=organization_id and 1=a_byte";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_string, b_string, a_byte FROM aTable WHERE ?=organization_id and 1=a_byte";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), A_VALUE);
            assertEquals(rs.getString("B_string"), B_VALUE);
            assertEquals(rs.getByte(3), 1);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testScanByShortValue() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT a_string, /* comment ok? */ b_string, a_short FROM aTable WHERE ?=organization_id and 128=a_short";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_string, b_string, a_short FROM aTable WHERE ?=organization_id and 128=a_short";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), A_VALUE);
            assertEquals(rs.getString("B_string"), B_VALUE);
            assertEquals(rs.getShort("a_short"), 128);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testScanByFloatValue() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT a_string, /* comment ok? */ b_string, a_float FROM aTable WHERE ?=organization_id and ?=a_float";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_string, b_string, a_float FROM aTable WHERE ?=organization_id and ?=a_float";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setFloat(2, 0.01f);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), A_VALUE);
            assertEquals(rs.getString("B_string"), B_VALUE);
            assertTrue(Float.compare(rs.getFloat(3), 0.01f) == 0);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testScanByUnsignedFloatValue() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT a_string, /* comment ok? */ b_string, a_unsigned_float FROM aTable WHERE ?=organization_id and ?=a_unsigned_float";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_string, b_string, a_unsigned_float FROM aTable WHERE ?=organization_id and ?=a_unsigned_float";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setFloat(2, 0.01f);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), A_VALUE);
            assertEquals(rs.getString("B_string"), B_VALUE);
            assertTrue(Float.compare(rs.getFloat(3), 0.01f) == 0);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testScanByDoubleValue() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT a_string, /* comment ok? */ b_string, a_double FROM aTable WHERE ?=organization_id and ?=a_double";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_string, b_string, a_double FROM aTable WHERE ?=organization_id and ?=a_double";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setDouble(2, 0.0001);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), A_VALUE);
            assertEquals(rs.getString("B_string"), B_VALUE);
            assertTrue(Double.compare(rs.getDouble(3), 0.0001) == 0);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testScanByUnsigned_DoubleValue() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT a_string, /* comment ok? */ b_string, a_unsigned_double FROM aTable WHERE ?=organization_id and ?=a_unsigned_double";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_string,b_string, a_unsigned_double FROM aTable WHERE ?=organization_id and ?=a_unsigned_double";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setDouble(2, 0.0001);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), A_VALUE);
            assertEquals(rs.getString("B_string"), B_VALUE);
            assertTrue(Double.compare(rs.getDouble(3), 0.0001) == 0);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testAllScan() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT ALL a_string, /* comment ok? */ b_string FROM aTable WHERE ?=organization_id and 5=a_integer";
        else if (tgtSQ()||tgtTR()) query = "SELECT ALL a_string, b_string FROM aTable WHERE ?=organization_id and 5=a_integer";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), B_VALUE);
            assertEquals(rs.getString("B_string"), C_VALUE);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testDistinctScan() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT DISTINCT a_string FROM aTable WHERE organization_id=?";
        else if (tgtSQ()||tgtTR()) query = "SELECT DISTINCT a_string FROM aTable WHERE organization_id=? order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), A_VALUE);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), B_VALUE);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), C_VALUE);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testInListSkipScan() throws Exception {
        printTestDescription();
        initATableValues();
        String query = query = "SELECT entity_id, b_string FROM aTable WHERE organization_id=? and entity_id IN (?,?)";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, ROW2);
            statement.setString(3, ROW4);
            ResultSet rs = statement.executeQuery();
            Set<String> expectedvals = new HashSet<String>();
            expectedvals.add(ROW2+"_"+C_VALUE);
            expectedvals.add(ROW4+"_"+B_VALUE);
            Set<String> vals = new HashSet<String>();
            assertTrue (rs.next());
            vals.add(rs.getString(1) + "_" + rs.getString(2));
            assertTrue (rs.next());
            vals.add(rs.getString(1) + "_" + rs.getString(2));
            assertFalse(rs.next());
            assertEquals(expectedvals, vals);
        } finally {
        }
    }

    @Test
    public void testNotInList() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and entity_id NOT IN (?,?,?,?,?,?)";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and entity_id NOT IN (?,?,?,?,?,?) order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, ROW2);
            statement.setString(3, ROW4);
            statement.setString(4, ROW1);
            statement.setString(5, ROW5);
            statement.setString(6, ROW7);
            statement.setString(7, ROW8);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW3, rs.getString(1));
            assertTrue (rs.next());
            assertEquals(ROW6, rs.getString(1));
            assertTrue (rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test 
    public void testSumDouble() throws Exception {
        printTestDescription();
        initSumDoubleValues();
        String query = "SELECT SUM(d) FROM SumDoubleTest";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertTrue(Double.compare(rs.getDouble(1), 0.015)==0);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testSumUnsignedDouble() throws Exception {
        printTestDescription();
        initSumDoubleValues();
        String query = "SELECT SUM(ud) FROM SumDoubleTest";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertTrue(Double.compare(rs.getDouble(1), 0.015)==0);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testSumFloat() throws Exception {
        printTestDescription();
        initSumDoubleValues();
        String query = "SELECT SUM(f) FROM SumDoubleTest";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertTrue(Float.compare(rs.getFloat(1), 0.15f)==0);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testSumUnsignedFloat() throws Exception {
        printTestDescription();
        initSumDoubleValues();
        String query = "SELECT SUM(uf) FROM SumDoubleTest";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertTrue(Float.compare(rs.getFloat(1), 0.15f)==0);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testNotInListOfFloat() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT a_float FROM aTable WHERE organization_id=? and a_float NOT IN (?,?,?,?,?,?)";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_float FROM aTable WHERE organization_id=? and a_float NOT IN (?,?,?,?,?,?) order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setFloat(2, 0.01f);
            statement.setFloat(3, 0.02f);
            statement.setFloat(4, 0.03f);
            statement.setFloat(5, 0.04f);
            statement.setFloat(6, 0.05f);
            statement.setFloat(7, 0.06f);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertTrue(Float.compare(rs.getFloat(1), 0.07f)==0);
            assertTrue (rs.next());
            assertTrue(Float.compare(rs.getFloat(1), 0.08f)==0);
            assertTrue (rs.next());
            assertTrue(Float.compare(rs.getFloat(1), 0.09f)==0);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testNotInListOfDouble() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT a_double FROM aTable WHERE organization_id=? and a_double NOT IN (?,?,?,?,?,?)";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_double FROM aTable WHERE organization_id=? and a_double NOT IN (?,?,?,?,?,?) order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setDouble(2, 0.0001);
            statement.setDouble(3, 0.0002);
            statement.setDouble(4, 0.0003);
            statement.setDouble(5, 0.0004);
            statement.setDouble(6, 0.0005);
            statement.setDouble(7, 0.0006);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertTrue(Double.compare(rs.getDouble(1), 0.0007)==0);
            assertTrue (rs.next());
            assertTrue(Double.compare(rs.getDouble(1), 0.0008)==0);
            assertTrue (rs.next());
            assertTrue(Double.compare(rs.getDouble(1), 0.0009)==0);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testGroupByPlusOne() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT a_integer+1 FROM aTable WHERE organization_id=? and a_integer = 5 GROUP BY a_integer+1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(6, rs.getInt(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testNoWhereScan() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT y_integer FROM aTable";
        else if (tgtSQ()||tgtTR()) query = "SELECT y_integer FROM aTable order by 1 desc";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            for (int i =0; i < 8; i++) {
                assertTrue (rs.next());
                assertEquals(0, rs.getInt(1));
                assertTrue(rs.wasNull());
            }
            assertTrue (rs.next());
            assertEquals(300, rs.getInt(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testToDateOnString() throws Exception { // TODO: test more conversion combinations
        printTestDescription();
        initATableValues();
        String query = "SELECT a_string FROM aTable WHERE organization_id=? and a_integer = 5";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            rs.getDate(1);
            fail();
// TRAF        } catch (ConstraintViolationException e) { // Expected
        } catch (Exception e) { // Expected
        } finally {
        }
    }

    @Test
    public void testNotEquals() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT entity_id -- and here comment\n" +
        "FROM aTable WHERE organization_id=? and a_integer != 1 and a_integer <= 2";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW2);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testNotEqualsByTinyInt() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT a_byte -- and here comment\n" +
        "FROM aTable WHERE organization_id=? and a_byte != 1 and a_byte <= 2";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getByte(1), 2);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testNotEqualsBySmallInt() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT a_short -- and here comment\n" +
        "FROM aTable WHERE organization_id=? and a_short != 128 and a_short !=0 and a_short <= 129";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getShort(1), 129);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testNotEqualsByFloat() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT a_float -- and here comment\n" +
        "FROM aTable WHERE organization_id=? and a_float != 0.01d and a_float <= 0.02d";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_float -- and here comment\n" +
        "FROM aTable WHERE organization_id=? and a_float != 0.01 and a_float <= 0.02";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertTrue(Float.compare(rs.getFloat(1), 0.02f) == 0);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testNotEqualsByUnsignedFloat() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT a_unsigned_float -- and here comment\n" +
        "FROM aTable WHERE organization_id=? and a_unsigned_float != 0.01d and a_unsigned_float <= 0.02d";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_unsigned_float -- and here comment\n" +
        "FROM aTable WHERE organization_id=? and a_unsigned_float != 0.01 and a_unsigned_float <= 0.02";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertTrue(Float.compare(rs.getFloat(1), 0.02f) == 0);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testNotEqualsByDouble() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT a_double -- and here comment\n" +
        "FROM aTable WHERE organization_id=? and a_double != 0.0001d and a_double <= 0.0002d";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_double -- and here comment\n" +
        "FROM aTable WHERE organization_id=? and a_double != 0.0001 and a_double <= 0.0002";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertTrue(Double.compare(rs.getDouble(1), 0.0002) == 0);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testNotEqualsByUnsignedDouble() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT a_unsigned_double -- and here comment\n" +
        "FROM aTable WHERE organization_id=? and a_unsigned_double != 0.0001d and a_unsigned_double <= 0.0002d";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_unsigned_double -- and here comment\n" +
        "FROM aTable WHERE organization_id=? and a_unsigned_double != 0.0001 and a_unsigned_double <= 0.0002";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertTrue(Double.compare(rs.getDouble(1), 0.0002) == 0);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testNotEquals2() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM // one more comment  \n" +
        "aTable WHERE organization_id=? and not a_integer = 1 and a_integer <= 2";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM -- one more comment  \n" +
        "aTable WHERE organization_id=? and not a_integer = 1 and a_integer <= 2";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW2);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testColumnOnBothSides() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT entity_id FROM aTable WHERE organization_id=? and a_string = b_string";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW7);
            assertFalse(rs.next());
        } finally {
        }
    }

    // This is not a test, it is a helper function.
    public void testNoStringValue(String value) throws Exception {
        initATableValues();
        Connection upsertConn = getConnection();
        upsertConn.setAutoCommit(true); // Test auto commit
        PreparedStatement stmt = null;
        if (tgtPH()) {
            stmt = upsertConn.prepareStatement(
            "upsert into ATABLE VALUES (?, ?, ?)"); // without specifying columns
            stmt.setString(1, tenantId);
            stmt.setString(2, ROW5);
            stmt.setString(3, value);
        } else if (tgtTR()) {
            stmt = upsertConn.prepareStatement(
            "upsert into ATABLE (organization_id, entity_id, a_string) VALUES (?, ?, ?)"); // without specifying columns
            stmt.setString(1, tenantId);
            stmt.setString(2, ROW5);
            stmt.setString(3, value);
        } else if (tgtSQ()) {
            stmt = upsertConn.prepareStatement(
            "update ATABLE set a_string = ? where organization_id = ? and entity_id = ?");
            stmt.setString(1, value);
            stmt.setString(2, tenantId);
            stmt.setString(3, ROW5);
        }
        stmt.execute(); // should commit too
        upsertConn.close();

        String query = "SELECT a_string, b_string FROM aTable WHERE organization_id=? and a_integer = 5";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            if (tgtPH()) {
                assertEquals(null, rs.getString(1));
                assertTrue(rs.wasNull());
            } else if (tgtSQ()||tgtTR()) assertEquals(value, rs.getString(1));
            assertEquals(rs.getString("B_string"), C_VALUE);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test 
    public void testNullStringValue() throws Exception {
        printTestDescription();
        testNoStringValue(null);
    }
  
    @Test
    public void testEmptyStringValue() throws Exception {
        printTestDescription();
        testNoStringValue("");
    }

    @Test
    public void testPointInTimeScan() throws Exception {
        printTestDescription();

        initATableValues(); 
        Connection upsertConn = getConnection();

        String upsertStmt = null;
        if (tgtPH()||tgtTR()) upsertStmt = 
            "upsert into " +
            "ATABLE(" +
            "    ORGANIZATION_ID, " +
            "    ENTITY_ID, " +
            "    A_INTEGER) " +
            "VALUES (?, ?, ?)";
        else if (tgtSQ()) upsertStmt =
            "update ATABLE set A_INTEGER = ? " +
            "where ORGANIZATION_ID = ? and ENTITY_ID = ?";
        upsertConn.setAutoCommit(true); // Test auto commit
        // Insert all rows at ts
        PreparedStatement stmt = upsertConn.prepareStatement(upsertStmt);
        if (tgtPH()||tgtTR()) {
            stmt.setString(1, tenantId);
            stmt.setString(2, ROW4);
            stmt.setInt(3, 5);
        } else if (tgtSQ()) {
            stmt.setInt(1, 5);
            stmt.setString(2, tenantId);
            stmt.setString(3, ROW4);
        }
        stmt.execute(); // should commit too
        upsertConn.close();

        // Override value again, but should be ignored since it's past the SCN
        // TRAF: We don't have SCN concept
        if (tgtPH()) {
            upsertConn = getConnection();
            upsertConn.setAutoCommit(true); // Test auto commit
            // Insert all rows at ts
            stmt = upsertConn.prepareStatement(upsertStmt);
            stmt.setString(1, tenantId);
            stmt.setString(2, ROW4);
            stmt.setInt(3, 9);
            stmt.execute(); // should commit too
            upsertConn.close();
        }

        String query = null;
        if (tgtPH()) query = "SELECT organization_id, a_string AS a FROM atable WHERE organization_id=? and a_integer = 5";
        else if (tgtSQ()||tgtTR()) query = "SELECT organization_id, a_string AS a FROM atable WHERE organization_id=? and a_integer = 5 order by 2";
        PreparedStatement statement = conn.prepareStatement(query);
        statement.setString(1, tenantId);
        ResultSet rs = statement.executeQuery();
        assertTrue(rs.next());
        if (tgtPH()) assertEquals(tenantId, rs.getString(1));
        else if (tgtSQ()||tgtTR()) assertEquals(tenantId, rs.getString(1).trim());
        assertEquals(A_VALUE, rs.getString("a"));
        assertTrue(rs.next());
        if (tgtPH()) assertEquals(tenantId, rs.getString(1));
        else if (tgtSQ()||tgtTR()) assertEquals(tenantId, rs.getString(1).trim());
        assertEquals(B_VALUE, rs.getString(2));
        assertFalse(rs.next());
    }

    @Test
    public void testPointInTimeLimitedScan() throws Exception {
        printTestDescription();

        initATableValues();
        Connection upsertConn = getConnection();

        String upsertStmt = null;
        if (tgtPH()||tgtTR()) upsertStmt = 
            "upsert into " +
            "ATABLE(" +
            "    ORGANIZATION_ID, " +
            "    ENTITY_ID, " +
            "    A_INTEGER) " +
            "VALUES (?, ?, ?)";
        else if (tgtSQ()) upsertStmt =
            "update ATABLE set A_INTEGER = ? " + 
            "where ORGANIZATION_ID = ? and ENTITY_ID = ?";
        upsertConn.setAutoCommit(true); // Test auto commit
        // Insert all rows at ts
        PreparedStatement stmt = upsertConn.prepareStatement(upsertStmt);
        if (tgtPH()||tgtTR()) {
            stmt.setString(1, tenantId);
            stmt.setString(2, ROW1);
            stmt.setInt(3, 6);
        } else if (tgtSQ()) {
            stmt.setInt(1, 6);
            stmt.setString(2, tenantId);
            stmt.setString(3, ROW1);
        }

        stmt.execute(); // should commit too
        upsertConn.close();

        // Override value again, but should be ignored since it's past the SCN
        // TRAF: we don't have this SCN concept.
        if (tgtPH()) {
           upsertConn = getConnection();
           upsertConn.setAutoCommit(true); // Test auto commit
           // Insert all rows at ts
           stmt = upsertConn.prepareStatement(upsertStmt);
           stmt.setString(1, tenantId);
           stmt.setString(2, ROW1);
           stmt.setInt(3, 0);
           stmt.execute(); // should commit too
           upsertConn.close();
        }

        String query = null;
        if (tgtPH()) query = "SELECT a_integer,b_string FROM atable WHERE organization_id=? and a_integer <= 5 limit 2";
        else if (tgtTR()) query = "SELECT a_integer,b_string FROM atable WHERE organization_id=? and a_integer <= 5 order by 1 limit 2";
        else if (tgtSQ()) query = "SELECT [first 2] a_integer,b_string FROM atable WHERE organization_id=? and a_integer <= 5 order by 1";
        PreparedStatement statement = conn.prepareStatement(query);
        statement.setString(1, tenantId);
        ResultSet rs = statement.executeQuery();
        assertTrue(rs.next());
        assertEquals(2, rs.getInt(1));
        assertEquals(C_VALUE, rs.getString(2));
        assertTrue(rs.next());
        assertEquals(3, rs.getInt(1));
        assertEquals(E_VALUE, rs.getString(2));
        assertFalse(rs.next());
    }

    @Test
    public void testUpperLowerBoundRangeScan() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and substr(entity_id,1,3) > '00A' and substr(entity_id,1,3) < '00C'";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and substr(entity_id,1,3) > '00A' and substr(entity_id,1,3) < '00C' order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW5);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW6);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW7);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW8);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testUpperBoundRangeScan() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and substr(entity_id,1,3) >= '00B' ";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and substr(entity_id,1,3) >= '00B' order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW5);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW6);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW7);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW8);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW9);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testLowerBoundRangeScan() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and substr(entity_id,1,3) < '00B' ";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and substr(entity_id,1,3) < '00B' order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW1);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW2);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW3);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW4);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testUnboundRangeScan1() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable WHERE organization_id <= ?";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable WHERE organization_id <= ? order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW1);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW2);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW3);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW4);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW5);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW6);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW7);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW8);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW9);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testUnboundRangeScan2() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable WHERE organization_id >= ?";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable WHERE organization_id >= ? order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW1);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW2);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW3);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW4);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW5);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW6);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW7);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW8);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW9);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    // FIXME: this is flapping with an phoenix.memory.InsufficientMemoryException
    // in the GroupedAggregateRegionObserver. We can work around it by increasing
    // the amount of available memory in QueryServicesTestImpl, but we shouldn't
    // have to. I think something may no be being closed to reclaim the memory.
    @Test
    public void testGroupedAggregation() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT a_string, count(1), 'foo' FROM atable WHERE organization_id=? GROUP BY a_string";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_string, count(1), 'foo' FROM atable WHERE organization_id=? GROUP BY a_string order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(rs.getString(1), A_VALUE);
            assertEquals(rs.getLong(2), 4L);
            assertEquals(rs.getString(3), "foo");
            assertTrue(rs.next());
            assertEquals(rs.getString(1), B_VALUE);
            assertEquals(rs.getLong(2), 4L);
            assertEquals(rs.getString(3), "foo");
            assertTrue(rs.next());
            assertEquals(rs.getString(1), C_VALUE);
            assertEquals(rs.getLong(2), 1L);
            assertEquals(rs.getString(3), "foo");
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testDistinctGroupedAggregation() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT DISTINCT a_string, count(1), 'foo' FROM atable WHERE organization_id=? GROUP BY a_string, b_string ORDER BY a_string, count(1)";
        else if (tgtSQ()||tgtTR()) query = "SELECT DISTINCT a_string, count(1), 'foo' FROM atable WHERE organization_id=? GROUP BY a_string, b_string ORDER BY 1, 2";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            
            assertTrue(rs.next());
            assertEquals(rs.getString(1), A_VALUE);
            assertEquals(rs.getLong(2), 1L);
            assertEquals(rs.getString(3), "foo");
            
            assertTrue(rs.next());
            assertEquals(rs.getString(1), A_VALUE);
            assertEquals(rs.getLong(2), 2L);
            assertEquals(rs.getString(3), "foo");
            
            assertTrue(rs.next());
            assertEquals(rs.getString(1), B_VALUE);
            assertEquals(rs.getLong(2), 1L);
            assertEquals(rs.getString(3), "foo");
            
            assertTrue(rs.next());
            assertEquals(rs.getString(1), B_VALUE);
            assertEquals(rs.getLong(2), 2L);
            assertEquals(rs.getString(3), "foo");
            
            assertTrue(rs.next());
            assertEquals(rs.getString(1), C_VALUE);
            assertEquals(rs.getLong(2), 1L);
            assertEquals(rs.getString(3), "foo");
            
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testDistinctLimitedGroupedAggregation() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT DISTINCT a_string, count(1), 'foo' FROM atable WHERE organization_id=? GROUP BY a_string, b_string ORDER BY count(1) desc,a_string LIMIT 2";
        else if (tgtTR()) query = "SELECT DISTINCT a_string, count(1), 'foo' FROM atable WHERE organization_id=? GROUP BY a_string, b_string ORDER BY 2 desc, 1 LIMIT 2";
        else if (tgtSQ()) query = "SELECT [first 2] DISTINCT a_string, count(1), 'foo' FROM atable WHERE organization_id=? GROUP BY a_string, b_string ORDER BY 2 desc, 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            
            assertTrue(rs.next());
            assertEquals(rs.getString(1), A_VALUE);
            assertEquals(rs.getLong(2), 2L);
            assertEquals(rs.getString(3), "foo");
            
            assertTrue(rs.next());
            assertEquals(rs.getString(1), B_VALUE);
            assertEquals(rs.getLong(2), 2L);
            assertEquals(rs.getString(3), "foo");
            
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testDistinctUngroupedAggregation() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT DISTINCT count(1), 'foo' FROM atable WHERE organization_id=?";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            
            assertTrue(rs.next());
            assertEquals(9L, rs.getLong(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testGroupedLimitedAggregation() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT a_string, count(1) FROM atable WHERE organization_id=? GROUP BY a_string LIMIT 2";
        else if (tgtTR()) query = "SELECT a_string, count(1) FROM atable WHERE organization_id=? GROUP BY a_string order by 1 LIMIT 2";
        else if (tgtSQ()) query = "SELECT [first 2] a_string, count(1) FROM atable WHERE organization_id=? GROUP BY a_string order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(rs.getString(1), A_VALUE);
            assertEquals(4L, rs.getLong(2));
            assertTrue(rs.next());
            assertEquals(rs.getString(1), B_VALUE);
            assertEquals(4L, rs.getLong(2));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testPointInTimeGroupedAggregation() throws Exception {
        printTestDescription();

        initATableValues();
        Connection upsertConn = getConnection();

        String updateStmt = null;
        if (tgtPH()) updateStmt =
            "upsert into " +
            "ATABLE VALUES ('" + tenantId + "','" + ROW5 + "','" + C_VALUE +"')";
        else if (tgtTR()) updateStmt =
            "upsert into " +
            "ATABLE (organization_id, entity_id, a_string) VALUES ('" + tenantId + "','" + ROW5 + "','" + C_VALUE +"')";
        else if (tgtSQ()) updateStmt =
            "update ATABLE set a_string = '" + C_VALUE + "' where organization_id = '" + tenantId + "' and entity_id = '" + ROW5 + "'";

        upsertConn.setAutoCommit(true); // Test auto commit
        // Insert all rows at ts
        Statement stmt = upsertConn.createStatement();
        stmt.execute(updateStmt); // should commit too
        upsertConn.close();

        // Override value again, but should be ignored since it's past the SCN
        // TRAF: we don't have this SCN concept.
        if (tgtPH()) {
            upsertConn = getConnection();
            upsertConn.setAutoCommit(true); // Test auto commit
            updateStmt =
                "upsert into " +
                "ATABLE VALUES (?, ?, ?)";
            // Insert all rows at ts
            PreparedStatement pstmt = upsertConn.prepareStatement(updateStmt);
            pstmt.setString(1, tenantId);
            pstmt.setString(2, ROW5);
            pstmt.setString(3, E_VALUE);
            pstmt.execute(); // should commit too
            upsertConn.close();
        }

        String query = null;
        if (tgtPH()) query = "SELECT a_string, count(1) FROM atable WHERE organization_id='" + tenantId + "' GROUP BY a_string";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_string, count(1) FROM atable WHERE organization_id='" + tenantId + "' GROUP BY a_string";
        Statement statement = conn.createStatement();
        ResultSet rs = statement.executeQuery(query);
        assertTrue(rs.next());
        assertEquals(A_VALUE, rs.getString(1));
        assertEquals(4, rs.getInt(2));
        assertTrue(rs.next());
        assertEquals(B_VALUE, rs.getString(1));
        assertEquals(3, rs.getLong(2));
        assertTrue(rs.next());
        assertEquals(C_VALUE, rs.getString(1));
        assertEquals(2, rs.getInt(2));
        assertFalse(rs.next());
    }

    @Test
    public void testUngroupedAggregation() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT count(1) FROM atable WHERE organization_id=? and a_string = ?";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, B_VALUE);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(4, rs.getLong(1));
            assertFalse(rs.next());
        } finally {
        }
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, B_VALUE);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(4, rs.getLong(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testUngroupedAggregationNoWhere() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT count(*) FROM atable";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(9, rs.getLong(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testPointInTimeUngroupedAggregation() throws Exception {
        printTestDescription();

        initATableValues();
        Connection upsertConn = getConnection();
        String updateStmt = null;
        if (tgtPH()||tgtTR()) updateStmt = 
            "upsert into " +
            "ATABLE(" +
            "    ORGANIZATION_ID, " +
            "    ENTITY_ID, " +
            "    A_STRING) " +
            "VALUES (?, ?, ?)";
        else if (tgtSQ()) updateStmt =
            "update ATABLE set A_STRING = ? " +
            "where ORGANIZATION_ID = ? and ENTITY_ID = ?";
        // Insert all rows at ts
        PreparedStatement stmt = upsertConn.prepareStatement(updateStmt);
        if (tgtPH()||tgtTR()) {
            stmt.setString(1, tenantId);
            stmt.setString(2, ROW5);
            stmt.setString(3, null);
        } else if (tgtSQ()) {
            stmt.setString(1, null);
            stmt.setString(2, tenantId);
            stmt.setString(3, ROW5);
        }

        stmt.execute();
        if (tgtPH()||tgtTR()) stmt.setString(3, C_VALUE);
        else if (tgtSQ()) stmt.setString(1, C_VALUE);
        stmt.execute();
        if (tgtPH()||tgtTR()) {
            stmt.setString(2, ROW7);
            stmt.setString(3, E_VALUE);
        } else if (tgtSQ()) {
            stmt.setString(1, E_VALUE);
            stmt.setString(3, ROW7);
        }
        stmt.execute();
        upsertConn.close();

        // Override value again, but should be ignored since it's past the SCN
        // TRAF: we don't have the SCN concept
        if (tgtPH()) {
            upsertConn = getConnection();
            upsertConn.setAutoCommit(true); // Test auto commit
            stmt = upsertConn.prepareStatement(updateStmt);
            stmt.setString(1, tenantId);
            stmt.setString(2, ROW6);
            stmt.setString(3, E_VALUE);
            stmt.execute();
            upsertConn.close();
        }
 
        String query = "SELECT count(1) FROM atable WHERE organization_id=? and a_string = ?";
        PreparedStatement statement = conn.prepareStatement(query);
        statement.setString(1, tenantId);
        statement.setString(2, B_VALUE);
        ResultSet rs = statement.executeQuery();
        assertTrue(rs.next());
        assertEquals(2, rs.getLong(1));
        assertFalse(rs.next());
    }

    @Test
    public void testPointInTimeUngroupedLimitedAggregation() throws Exception {
        printTestDescription();

        initATableValues();
        String updateStmt = null;
        if (tgtPH()||tgtTR()) updateStmt = 
            "upsert into " +
            "ATABLE(" +
            "    ORGANIZATION_ID, " +
            "    ENTITY_ID, " +
            "    A_STRING) " +
            "VALUES (?, ?, ?)";
        else if (tgtSQ()) updateStmt =
            "update ATABLE set A_STRING = ? " +
            "where ORGANIZATION_ID = ? and ENTITY_ID = ?";
        Connection upsertConn = getConnection();
        upsertConn.setAutoCommit(true); // Test auto commit
        PreparedStatement stmt = upsertConn.prepareStatement(updateStmt);
        if (tgtPH()||tgtTR()) {
            stmt.setString(1, tenantId);
            stmt.setString(2, ROW6);
            stmt.setString(3, C_VALUE);
        } else if (tgtSQ()) {
            stmt.setString(1, C_VALUE);
            stmt.setString(2, tenantId);
            stmt.setString(3, ROW6);
        }
        stmt.execute();
        if (tgtPH()||tgtTR()) stmt.setString(3, E_VALUE);
        else if (tgtSQ()) stmt.setString(1, E_VALUE);
        stmt.execute();
        if (tgtPH()||tgtTR()) stmt.setString(3, B_VALUE);
        else if (tgtSQ()) stmt.setString(1, B_VALUE);
        stmt.execute();
        if (tgtPH()||tgtTR()) stmt.setString(3, B_VALUE);
        else if (tgtSQ()) stmt.setString(1, B_VALUE);
        stmt.execute();
        upsertConn.close();

        // Override value again, but should be ignored since it's past the SCN
        // TRAF: we don't have the SCN concept
        if (tgtPH()) {
           upsertConn = getConnection();
           upsertConn.setAutoCommit(true); // Test auto commit
           stmt = upsertConn.prepareStatement(updateStmt);
           stmt.setString(1, tenantId);
           stmt.setString(2, ROW6);
           stmt.setString(3, E_VALUE);
           stmt.execute();
           upsertConn.close();
        }

        String query = null;
        if (tgtPH()||tgtTR()) query = "SELECT count(1) FROM atable WHERE organization_id=? and a_string = ? LIMIT 3";
        else if (tgtSQ()) query = "SELECT [first 3] count(1) FROM atable WHERE organization_id=? and a_string = ?";
        PreparedStatement statement = conn.prepareStatement(query);
        statement.setString(1, tenantId);
        statement.setString(2, B_VALUE);
        ResultSet rs = statement.executeQuery();
        assertTrue(rs.next());
        assertEquals(4, rs.getLong(1)); // LIMIT applied at end, so all rows would be counted
        assertFalse(rs.next());
    }

    @Test 
    public void testPointInTimeDeleteUngroupedAggregation() throws Exception {
        printTestDescription();

        initATableValues();
        String updateStmt = null;
        if (tgtPH()||tgtTR()) updateStmt =
            "upsert into " +
            "ATABLE(" +
            "    ORGANIZATION_ID, " +
            "    ENTITY_ID, " +
            "    A_STRING) " +
            "VALUES (?, ?, ?)";
        else if (tgtSQ()) updateStmt =
            "update ATABLE set A_STRING = ? " +
            "where ORGANIZATION_ID = ? and ENTITY_ID = ?";
        // Remove column value at ts + 1 (i.e. equivalent to setting the value to null)
        PreparedStatement stmt = conn.prepareStatement(updateStmt);
        if (tgtPH()||tgtTR()) {
            stmt.setString(1, tenantId);
            stmt.setString(2, ROW7);
            stmt.setString(3, null);
        } else if (tgtSQ()) {
            stmt.setString(1, null);
            stmt.setString(2, tenantId);
            stmt.setString(3, ROW7);
        }
        stmt.execute(); 

        // Delete row 
        stmt = conn.prepareStatement("delete from atable where organization_id=? and entity_id=?");
        stmt.setString(1, tenantId);
        stmt.setString(2, ROW5);
        stmt.execute();
        
        // Delete row at timestamp 3. This should not be seen by the query executing
        // Remove column value at ts + 1 (i.e. equivalent to setting the value to null)
        // TRAF: we don't have the SCN concept
        if (tgtPH()) {
            Connection futureConn = getConnection();
            stmt = futureConn.prepareStatement("delete from atable where organization_id=? and entity_id=?");
            stmt.setString(1, tenantId);
            stmt.setString(2, ROW6);
            stmt.execute();
            futureConn.commit();
            futureConn.close();
        }

        String query = "SELECT count(1) FROM atable WHERE organization_id=? and a_string = ?";
        PreparedStatement statement = conn.prepareStatement(query);
        statement.setString(1, tenantId);
        statement.setString(2, B_VALUE);
        ResultSet rs = statement.executeQuery();
        assertTrue(rs.next());
        assertEquals(2, rs.getLong(1));
        assertFalse(rs.next());
    }

    @Test
    public void testIntFilter() throws Exception {
        printTestDescription();

        initATableValues();
        String updateStmt = null;
        if (tgtPH()||tgtTR()) updateStmt = 
            "upsert into " +
            "ATABLE(" +
            "    ORGANIZATION_ID, " +
            "    ENTITY_ID, " +
            "    A_INTEGER) " +
            "VALUES (?, ?, ?)";
        else if (tgtSQ()) updateStmt =
            "update ATABLE set A_INTEGER = ? " +
            "where ORGANIZATION_ID = ? and ENTITY_ID = ?";
        Connection upsertConn = getConnection();
        upsertConn.setAutoCommit(true); // Test auto commit
        PreparedStatement stmt = upsertConn.prepareStatement(updateStmt);
        if (tgtPH()||tgtTR()) {
            stmt.setString(1, tenantId);
            stmt.setString(2, ROW4);
            stmt.setInt(3, -10);
        } else if (tgtSQ()) {
            stmt.setInt(1, -10);
            stmt.setString(2, tenantId);
            stmt.setString(3, ROW4);
        }
        stmt.execute();
        upsertConn.close();

        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and a_integer >= ?";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and a_integer >= ? order by 1";
        PreparedStatement statement = conn.prepareStatement(query);
        statement.setString(1, tenantId);
        statement.setInt(2, 7);
        ResultSet rs = statement.executeQuery();
        assertTrue (rs.next());
        assertEquals(rs.getString(1), ROW7);
        assertTrue (rs.next());
        assertEquals(rs.getString(1), ROW8);
        assertTrue (rs.next());
        assertEquals(rs.getString(1), ROW9);
        assertFalse(rs.next());

        if (tgtPH()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and a_integer < 2";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and a_integer < 2 order by 1";
        statement = conn.prepareStatement(query);
        statement.setString(1, tenantId);
        rs = statement.executeQuery();
        assertTrue (rs.next());
        assertEquals(rs.getString(1), ROW1);
        assertTrue (rs.next());
        assertEquals(rs.getString(1), ROW4);
        assertFalse(rs.next());

        if (tgtPH()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and a_integer <= 2";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and a_integer <= 2 order by 1";
        statement = conn.prepareStatement(query);
        statement.setString(1, tenantId);
        rs = statement.executeQuery();
        assertTrue (rs.next());
        assertEquals(rs.getString(1), ROW1);
        assertTrue (rs.next());
        assertEquals(rs.getString(1), ROW2);
        assertTrue (rs.next());
        assertEquals(rs.getString(1), ROW4);
        assertFalse(rs.next());

        query = "SELECT entity_id FROM aTable WHERE organization_id=? and a_integer >=9";
        statement = conn.prepareStatement(query);
        statement.setString(1, tenantId);
        rs = statement.executeQuery();
        assertTrue (rs.next());
        assertEquals(rs.getString(1), ROW9);
        assertFalse(rs.next());
    }
   
    @Test
    public void testDateAdd() throws Exception {
        printTestDescription();
        initATableValues(new Date(System.currentTimeMillis()));
        String query = null;
        if (tgtPH()) query = "SELECT entity_id, b_string FROM ATABLE WHERE a_date + 0.5d < ?";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id, b_string FROM ATABLE WHERE a_date + 0 < ? order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setDate(1, new Date(System.currentTimeMillis() + MILLIS_IN_DAY));
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(ROW1, rs.getString(1));
            assertTrue(rs.next());
            assertEquals(ROW4, rs.getString(1));
            assertTrue(rs.next());
            assertEquals(ROW7, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testDateSubtract() throws Exception {
        printTestDescription();
        initATableValues(new Date(System.currentTimeMillis()));
        String query = null;
        if (tgtPH()) query = "SELECT entity_id, b_string FROM ATABLE WHERE a_date - 0.5d > ?";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id, b_string FROM ATABLE WHERE a_date - 0 > ? order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setDate(1, new Date(System.currentTimeMillis() + MILLIS_IN_DAY));
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(ROW3, rs.getString(1));
            assertTrue(rs.next());
            assertEquals(ROW6, rs.getString(1));
            assertTrue(rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testTimestamp() throws Exception {
        printTestDescription();

        initATableValues();
        String updateStmt = null;
        if (tgtPH()||tgtTR()) updateStmt = 
            "upsert into " +
            "ATABLE(" +
            "    ORGANIZATION_ID, " +
            "    ENTITY_ID, " +
            "    A_TIMESTAMP) " +
            "VALUES (?, ?, ?)";
        else if (tgtSQ()) updateStmt =
            "update ATABLE set A_TIMESTAMP = ? " +
            "where ORGANIZATION_ID = ? and ENTITY_ID = ?";
        Connection upsertConn = getConnection();
        upsertConn.setAutoCommit(true); // Test auto commit
        PreparedStatement stmt = upsertConn.prepareStatement(updateStmt);
        if (tgtPH()||tgtTR()) {
           stmt.setString(1, tenantId);
           stmt.setString(2, ROW4);
           Timestamp tsValue1 = new Timestamp(5000);
           // TRAF byte[] ts1 = PDataType.TIMESTAMP.toBytes(tsValue1);
           stmt.setTimestamp(3, tsValue1);
        } else if (tgtSQ()) {
           Timestamp tsValue1 = new Timestamp(5000);
           stmt.setTimestamp(1, tsValue1);
           stmt.setString(2, tenantId);
           stmt.setString(3, ROW4);
        }
        stmt.execute();

        if (tgtPH()||tgtTR()) updateStmt = 
            "upsert into " +
            "ATABLE(" +
            "    ORGANIZATION_ID, " +
            "    ENTITY_ID, " +
            "    A_TIMESTAMP," +
            "    A_TIME) " +
            "VALUES (?, ?, ?, ?)";
        else if (tgtSQ()) updateStmt = 
            "update ATABLE set (A_TIMESTAMP, A_TIME) = (?, ?) " +
            "where ORGANIZATION_ID = ? and ENTITY_ID = ?";
        stmt = upsertConn.prepareStatement(updateStmt);
        Timestamp tsValue2 = null;
        if (tgtPH()) {
            stmt.setString(1, tenantId);
            stmt.setString(2, ROW5);
            tsValue2 = new Timestamp(5000);
            tsValue2.setNanos(200);
            // TRAF byte[] ts2 = PDataType.TIMESTAMP.toBytes(tsValue2);
            stmt.setTimestamp(3, tsValue2);
            stmt.setTime(4, new Time(tsValue2.getTime()));
        } else if (tgtTR()) {
            tsValue2 = new Timestamp(5000);
            // TRAF tsValue2.setNanos(200);
            // TRAF a narosecond is 1E-9s, our default is 1E-6s, 200 is too
            // small, need to be at least 2000 to be visible for us.
            tsValue2.setNanos(2000);
            stmt.setString(1, tenantId);
            stmt.setString(2, ROW5);
            stmt.setTimestamp(3, tsValue2);
            stmt.setTime(4, new Time(tsValue2.getTime()));
        } else if (tgtSQ()) {
            tsValue2 = new Timestamp(5000);
            // TRAF tsValue2.setNanos(200);
            // TRAF a narosecond is 1E-9s, our default is 1E-6s, 200 is too
            // small, need to be at least 2000 to be visible for us.
            tsValue2.setNanos(2000);
            // TRAF byte[] ts2 = PDataType.TIMESTAMP.toBytes(tsValue2);
            stmt.setTimestamp(1, tsValue2);
            stmt.setTime(2, new Time(tsValue2.getTime()));
            stmt.setString(3, tenantId);
            stmt.setString(4, ROW5);
        }
        stmt.execute();
        upsertConn.close();
        
        String query = "SELECT entity_id, a_timestamp, a_time FROM aTable WHERE organization_id=? and a_timestamp > ?";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setTimestamp(2, new Timestamp(5000));
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW5);
            assertEquals(rs.getTimestamp("A_TIMESTAMP"), tsValue2);
            assertEquals(rs.getTime("A_TIME"), new Time(tsValue2.getTime()));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testCoerceTinyIntToSmallInt() throws Exception {
        printTestDescription();
        String query = "SELECT entity_id FROM ATABLE WHERE organization_id=? AND a_byte >= a_short";
        try {
            initATableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testCoerceIntegerToLong() throws Exception {
        printTestDescription();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM ATABLE WHERE organization_id=? AND x_long >= x_integer";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM ATABLE WHERE organization_id=? AND x_long >= x_integer order by 1";
        try {
            initATableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(ROW7, rs.getString(1));
            assertTrue(rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
   
    @Test
    public void testCoerceLongToDecimal1() throws Exception {
        printTestDescription();
        String query = "SELECT entity_id FROM ATABLE WHERE organization_id=? AND x_decimal > x_integer";
        try {
            initATableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testCoerceLongToDecimal2() throws Exception {
        printTestDescription();
        String query = "SELECT entity_id FROM ATABLE WHERE organization_id=? AND x_integer <= x_decimal";
        try {
            initATableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testSimpleCaseStatement() throws Exception {
        printTestDescription();
        String query = null;
        if (tgtPH()) query = "SELECT CASE a_integer WHEN 1 THEN 'a' WHEN 2 THEN 'b' WHEN 3 THEN 'c' ELSE 'd' END AS a FROM ATABLE WHERE organization_id=? AND a_integer < 6";
        else if (tgtSQ()||tgtTR()) query = "SELECT CASE a_integer WHEN 1 THEN 'a' WHEN 2 THEN 'b' WHEN 3 THEN 'c' ELSE 'd' END AS a FROM ATABLE WHERE organization_id=? AND a_integer < 6 order by 1";
        try {
            initATableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("a", rs.getString(1));
            assertTrue(rs.next());
            assertEquals("b", rs.getString(1));
            assertTrue(rs.next());
            assertEquals("c", rs.getString(1));
            assertTrue(rs.next());
            assertEquals("d", rs.getString(1));
            assertTrue(rs.next());
            assertEquals("d", rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testMultiCondCaseStatement() throws Exception {
        printTestDescription();
        String query = null;
        if (tgtPH()) query = "SELECT CASE WHEN a_integer <= 2 THEN 1.5 WHEN a_integer = 3 THEN 2 WHEN a_integer <= 6 THEN 4.5 ELSE 5 END AS a FROM ATABLE WHERE organization_id=?";
        else if (tgtSQ()||tgtTR()) query = "SELECT CASE WHEN a_integer <= 2 THEN 1.5 WHEN a_integer = 3 THEN 2 WHEN a_integer <= 6 THEN 4.5 ELSE 5 END AS a FROM ATABLE WHERE organization_id=? order by 1";
        try {
            initATableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            /* TRAF for BigDecimal, really should use compareTo()
            assertEquals(BigDecimal.valueOf(1.5), rs.getBigDecimal(1));
            assertTrue(rs.next());
            assertEquals(BigDecimal.valueOf(1.5), rs.getBigDecimal(1));
            assertTrue(rs.next());
            assertEquals(BigDecimal.valueOf(2), rs.getBigDecimal(1));
            assertTrue(rs.next());
            assertEquals(BigDecimal.valueOf(4.5), rs.getBigDecimal(1));
            assertTrue(rs.next());
            assertEquals(BigDecimal.valueOf(4.5), rs.getBigDecimal(1));
            assertTrue(rs.next());
            assertEquals(BigDecimal.valueOf(4.5), rs.getBigDecimal(1));
            assertTrue(rs.next());
            assertEquals(BigDecimal.valueOf(5), rs.getBigDecimal(1));
            assertTrue(rs.next());
            assertEquals(BigDecimal.valueOf(5), rs.getBigDecimal(1));
            assertTrue(rs.next());
            assertEquals(BigDecimal.valueOf(5), rs.getBigDecimal(1));
            assertFalse(rs.next());
            */
            /* TRAF */
            assertTrue(BigDecimal.valueOf(1.5).compareTo(rs.getBigDecimal(1)) == 0);
            assertTrue(rs.next());
            assertTrue(BigDecimal.valueOf(1.5).compareTo(rs.getBigDecimal(1)) == 0);
            assertTrue(rs.next());
            assertTrue(BigDecimal.valueOf(2).compareTo(rs.getBigDecimal(1)) == 0);
            assertTrue(rs.next());
            assertTrue(BigDecimal.valueOf(4.5).compareTo(rs.getBigDecimal(1)) == 0);
            assertTrue(rs.next());
            assertTrue(BigDecimal.valueOf(4.5).compareTo(rs.getBigDecimal(1)) == 0);
            assertTrue(rs.next());
            assertTrue(BigDecimal.valueOf(4.5).compareTo(rs.getBigDecimal(1)) == 0);
            assertTrue(rs.next());
            assertTrue(BigDecimal.valueOf(5).compareTo(rs.getBigDecimal(1)) == 0);
            assertTrue(rs.next());
            assertTrue(BigDecimal.valueOf(5).compareTo(rs.getBigDecimal(1)) == 0);
            assertTrue(rs.next());
            assertTrue(BigDecimal.valueOf(5).compareTo(rs.getBigDecimal(1)) == 0);
            assertFalse(rs.next());
            /* end of TRAF */
        } finally {
        }
    }
    
    @Test
    public void testPartialEvalCaseStatement() throws Exception {
        printTestDescription();
        String query = "SELECT entity_id FROM ATABLE WHERE organization_id=? and CASE WHEN 1234 = a_integer THEN 1 WHEN x_integer = 5 THEN 2 ELSE 3 END = 2";
        try {
            initATableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(ROW7, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
   
    @Test
    public void testFoundIndexOnPartialEvalCaseStatement() throws Exception {
        printTestDescription();
        String query = "SELECT entity_id FROM ATABLE WHERE organization_id=? and CASE WHEN a_integer = 1234 THEN 1 WHEN x_integer = 3 THEN y_integer ELSE 3 END = 300";
        try {
            initATableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    // TODO: we need some tests that have multiple versions of key values
    @Test
    public void testUnfoundMultiColumnCaseStatement() throws Exception {
        printTestDescription();
        String query = "SELECT entity_id, b_string FROM ATABLE WHERE organization_id=? and CASE WHEN a_integer = 1234 THEN 1 WHEN a_date < ? THEN y_integer WHEN x_integer = 4 THEN 4 ELSE 3 END = 4";
        try {
            initATableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setDate(2, new Date(System.currentTimeMillis()));
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(ROW8, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
   
    @Test
    public void testUnfoundSingleColumnCaseStatement() throws Exception {
        printTestDescription();

        String query = "SELECT entity_id, b_string FROM ATABLE WHERE organization_id=? and CASE WHEN a_integer = 0 or a_integer != 0 THEN 1 ELSE 0 END = 0";
        initATableValues();
        // Set ROW5.A_INTEGER to null so that we have one row
        // where the else clause of the CASE statement will
        // fire.
        Connection upsertConn = getConnection();
        String upsertStmt = null;
        if (tgtPH()||tgtTR()) upsertStmt = 
            "upsert into " +
            "ATABLE(" +
            "    ENTITY_ID, " +
            "    ORGANIZATION_ID, " +
            "    A_INTEGER) " +
            "VALUES ('" + ROW5 + "','" + tenantId + "', null)";
        else if (tgtSQ()) upsertStmt =
            "update ATABLE set A_INTEGER = null " + 
            "where ENTITY_ID = '" + ROW5 +"' and ORGANIZATION_ID = '" + tenantId + "'";

        upsertConn.setAutoCommit(true); // Test auto commit
        // Insert all rows at ts
        PreparedStatement stmt = upsertConn.prepareStatement(upsertStmt);
        stmt.execute(); // should commit too
        upsertConn.close();
        
        PreparedStatement statement = conn.prepareStatement(query);
        statement.setString(1, tenantId);
        ResultSet rs = statement.executeQuery();
        assertTrue(rs.next());
        assertEquals(ROW5, rs.getString(1));
        assertFalse(rs.next());
    }
  
    @Test
    public void testNonNullMultiCondCaseStatement() throws Exception {
        printTestDescription();
        String query = "SELECT CASE WHEN entity_id = '000000000000000' THEN 1 WHEN entity_id = '000000000000001' THEN 2 ELSE 3 END FROM ATABLE WHERE organization_id=?";
        try {
            initATableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            ResultSetMetaData rsm = rs.getMetaData();
            assertEquals(ResultSetMetaData.columnNoNulls,rsm.isNullable(1));
        } finally {
        }
    }
    
    @Test
    public void testNullMultiCondCaseStatement() throws Exception {
        printTestDescription();
        String query = "SELECT CASE WHEN entity_id = '000000000000000' THEN 1 WHEN entity_id = '000000000000001' THEN 2 END FROM ATABLE WHERE organization_id=?";
        try {
            initATableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            ResultSetMetaData rsm = rs.getMetaData();
            assertEquals(ResultSetMetaData.columnNullable,rsm.isNullable(1));
        } finally {
        }
    }
 
    @Test
    public void testNullabilityMultiCondCaseStatement() throws Exception {
        printTestDescription();
        String query = null;
        if (tgtPH()) query = "SELECT CASE WHEN a_integer <= 2 THEN ? WHEN a_integer = 3 THEN ? WHEN a_integer <= ? THEN ? ELSE 5 END AS a FROM ATABLE WHERE organization_id=?";
        // TRAF: In both SQ and TRAF, that 'ELSE 5' will force the parameter
        // replacement to become integer, and the fetching back would be 1
        // instead of 1.5.  This is true for both JDBC and hpdci if using
        // 'set param ?p'  change it to 5.0 to accomodate this behavior
        else if (tgtSQ()||tgtTR()) query = "SELECT CASE WHEN a_integer <= 2 THEN ? WHEN a_integer = 3 THEN ? WHEN a_integer <= ? THEN ? ELSE 5.0 END AS a FROM ATABLE WHERE organization_id=? order by 1";
        try {
            initATableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            // TRAF DEBUG This test fails on SQ right now.  But if you don't
            // do the following parameter subsitute, execute the above 
            // commented-out query directly, it works fine.  There is something
            // strange about our JDBC's parameter subsitute.
            statement.setBigDecimal(1,BigDecimal.valueOf(1.5));
            statement.setInt(2,2);
            statement.setInt(3,6);
            statement.setBigDecimal(4,BigDecimal.valueOf(4.5));
            statement.setString(5, tenantId);
            // TRAF DEBUG end
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) {
                assertEquals(BigDecimal.valueOf(1.5), rs.getBigDecimal(1));
                assertTrue(rs.next());
                assertEquals(BigDecimal.valueOf(1.5), rs.getBigDecimal(1));
                assertTrue(rs.next());
                assertEquals(BigDecimal.valueOf(2), rs.getBigDecimal(1));
                assertTrue(rs.next());
                assertEquals(BigDecimal.valueOf(4.5), rs.getBigDecimal(1));
                assertTrue(rs.next());
                assertEquals(BigDecimal.valueOf(4.5), rs.getBigDecimal(1));
                assertTrue(rs.next());
                assertEquals(BigDecimal.valueOf(4.5), rs.getBigDecimal(1));
                assertTrue(rs.next());
                assertEquals(BigDecimal.valueOf(5), rs.getBigDecimal(1));
                assertTrue(rs.next());
                assertEquals(BigDecimal.valueOf(5), rs.getBigDecimal(1));
                assertTrue(rs.next());
                assertEquals(BigDecimal.valueOf(5), rs.getBigDecimal(1));
                assertFalse(rs.next());
            } else if (tgtSQ()||tgtTR()) {
                assertTrue(BigDecimal.valueOf(1.5).compareTo(rs.getBigDecimal(1)) == 0);
                assertTrue(rs.next());
                assertTrue(BigDecimal.valueOf(1.5).compareTo(rs.getBigDecimal(1)) == 0);
                assertTrue(rs.next());
                assertTrue(BigDecimal.valueOf(2).compareTo(rs.getBigDecimal(1)) == 0);
                assertTrue(rs.next());
                assertTrue(BigDecimal.valueOf(4.5).compareTo(rs.getBigDecimal(1)) == 0);
                assertTrue(rs.next());
                assertTrue(BigDecimal.valueOf(4.5).compareTo(rs.getBigDecimal(1)) == 0);
                assertTrue(rs.next());
                assertTrue(BigDecimal.valueOf(4.5).compareTo(rs.getBigDecimal(1)) == 0);
                assertTrue(rs.next());
                assertTrue(BigDecimal.valueOf(5).compareTo(rs.getBigDecimal(1)) == 0);
                assertTrue(rs.next());
                assertTrue(BigDecimal.valueOf(5).compareTo(rs.getBigDecimal(1)) == 0);
                assertTrue(rs.next());
                assertTrue(BigDecimal.valueOf(5).compareTo(rs.getBigDecimal(1)) == 0);
                assertFalse(rs.next());
            }
        } finally {
        }
    }
   
    @Test
    public void testSimpleInListStatement() throws Exception {
        printTestDescription();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM ATABLE WHERE organization_id=? AND a_integer IN (2,4)";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM ATABLE WHERE organization_id=? AND a_integer IN (2,4) order by 1";
        try {
            initATableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(ROW2, rs.getString(1));
            assertTrue(rs.next());
            assertEquals(ROW4, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    /**
     * Test to repro Null Pointer Exception
     * @throws Exception
     */
    @Test
    public void testInFilterOnKey() throws Exception {
        printTestDescription();
        String query = "SELECT count(entity_id) FROM ATABLE WHERE organization_id IN (?,?)";
        try {
            initATableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(9, rs.getInt(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testOneInListStatement() throws Exception {
        printTestDescription();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM ATABLE WHERE organization_id=? AND b_string IN (?)";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM ATABLE WHERE organization_id=? AND b_string IN (?) order by 1";
        try {
            initATableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, E_VALUE);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(ROW3, rs.getString(1));
            assertTrue(rs.next());
            assertEquals(ROW6, rs.getString(1));
            assertTrue(rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testMixedTypeInListStatement() throws Exception {
        printTestDescription();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM ATABLE WHERE organization_id=? AND x_long IN (5, ?)";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM ATABLE WHERE organization_id=? AND x_long IN (5, ?) order by 1";
        try {
            initATableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            long l = Integer.MAX_VALUE + 1L;
            statement.setLong(2, l);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(ROW7, rs.getString(1));
            assertTrue(rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testIsNull() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable WHERE X_DECIMAL is null";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable WHERE X_DECIMAL is null order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW1);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW2);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW3);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW4);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW5);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW6);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testCountIsNull() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT count(1) FROM aTable WHERE X_DECIMAL is null";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(6, rs.getLong(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testCountIsNotNull() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT count(1) FROM aTable WHERE X_DECIMAL is not null";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(3, rs.getLong(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testIsNotNull() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable WHERE X_DECIMAL is not null";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable WHERE X_DECIMAL is not null order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW7);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW8);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW9);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testIntSubtractionExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable where A_INTEGER - 4  <= 0";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable where A_INTEGER - 4  <= 0 order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW1);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW2);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW3);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW4);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testDecimalSubtraction1Expression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable where A_INTEGER - 3.5  <= 0";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable where A_INTEGER - 3.5  <= 0 order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW1, rs.getString(1));
            assertTrue (rs.next());
            assertEquals(ROW2, rs.getString(1));
            assertTrue (rs.next());
            assertEquals(ROW3, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testDecimalSubtraction2Expression() throws Exception {// check if decimal part makes a difference
        printTestDescription();
        initATableValues();
        String query = "SELECT entity_id FROM aTable where X_DECIMAL - 3.5  > 0";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW8);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testLongSubtractionExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT entity_id FROM aTable where X_LONG - 1  < 0";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW8);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testDoubleSubtractionExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable where a_double - 0.0002d  < 0";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable where a_double - 0.0002  < 0";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW1);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testSmallIntSubtractionExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT entity_id FROM aTable where a_short - 129  = 0";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW2);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testTernarySubtractionExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable where  X_INTEGER - X_LONG - 10  < 0";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable where  X_INTEGER - X_LONG - 10  < 0 order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW7);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW9);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testSelectWithSubtractionExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT entity_id, x_integer - 4 FROM aTable where  x_integer - 4 = 0";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW8);
            assertEquals(rs.getInt(2), 0);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testConstantSubtractionExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT entity_id FROM aTable where A_INTEGER = 5 - 1 - 2";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW2);
            assertFalse(rs.next());
        } finally {
        }
    }
 
    @Test
    public void testIntDivideExpression() throws Exception {
        printTestDescription();
        initATableValues();
        // TRAF: In both SQ and TRAF, SQ treates A_INTEGER/3 as a floating
        // point.  A_INTEGER/3>2 is TRUE for A_INTEGER=7,8,9, so this query 
        // returns 3 rows. Pheonix treats it as an integer and it only returns 
        // 1 row when A_INTEGER=9. It has been confirmed that this behavior
        // will not change in TRAF.  Changed the following query so that it
        // would return the same row. 
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable where A_INTEGER / 3 > 2";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable where A_INTEGER / 3 > 2.9999";
 
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testDoubleDivideExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable where a_double / 3.0d = 0.0003";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable where a_double / 3.0 = 0.0003";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testSmallIntDivideExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT entity_id FROM aTable where a_short / 135 = 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW8, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testIntToDecimalDivideExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable where A_INTEGER / 3.0 > 2";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable where A_INTEGER / 3.0 > 2 order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW7, rs.getString(1));
            assertTrue (rs.next());
            assertEquals(ROW8, rs.getString(1));
            assertTrue (rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testConstantDivideExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT entity_id FROM aTable where A_INTEGER = 9 / 3 / 3";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW1);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testSelectWithDivideExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT entity_id, a_integer/3 FROM aTable where  a_integer = 9";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertEquals(3, rs.getInt(2));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testNegateExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT entity_id FROM aTable where A_INTEGER - 4 = -1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW3, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testIntMultiplyExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT entity_id FROM aTable where A_INTEGER * 2 = 16";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW8, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
   
    @Test
    public void testDoubleMultiplyExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable where A_DOUBLE * 2.0d = 0.0002";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable where A_DOUBLE * 2.0 = 0.0002";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW1, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testLongMultiplyExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = "SELECT entity_id FROM aTable where X_LONG * 2 * 2 = 20";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW7, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testIntToDecimalMultiplyExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable where A_INTEGER * 1.5 > 9";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable where A_INTEGER * 1.5 > 9 order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW7, rs.getString(1));
            assertTrue (rs.next());
            assertEquals(ROW8, rs.getString(1));
            assertTrue (rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testDecimalMultiplyExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable where X_DECIMAL * A_INTEGER > 29.5";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable where X_DECIMAL * A_INTEGER > 29.5 order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW8, rs.getString(1));
            assertTrue (rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testIntAddExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = query = "SELECT entity_id FROM aTable where A_INTEGER + 2 = 4";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW2, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testDecimalAddExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable where A_INTEGER + X_DECIMAL > 11";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable where A_INTEGER + X_DECIMAL > 11 order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW8, rs.getString(1));
            assertTrue (rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testDoubleAddExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable where a_double + a_float > 0.08";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable where a_double + a_float > 0.08 order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW8, rs.getString(1));
            assertTrue (rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testUnsignedDoubleAddExpression() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable where a_unsigned_double + a_unsigned_float > 0.08";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable where a_unsigned_double + a_unsigned_float > 0.08 order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW8, rs.getString(1));
            assertTrue (rs.next());
            assertEquals(ROW9, rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }
   
/*  
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(
            value="RV_RETURN_VALUE_IGNORED",
            justification="Test code.")
*/
    @Test
    public void testValidArithmetic() throws Exception {
        printTestDescription();
        initATableValues();
        String[] queries = null;
        if (tgtPH()) queries = new String[] { 
                "SELECT entity_id,organization_id FROM atable where (A_DATE - A_DATE) * 5 < 0",
                "SELECT entity_id,organization_id FROM atable where 1 + A_DATE  < A_DATE",
                "SELECT entity_id,organization_id FROM atable where A_DATE - 1 < A_DATE",
                "SELECT entity_id,organization_id FROM atable where A_INTEGER - 45 < 0",
                "SELECT entity_id,organization_id FROM atable where X_DECIMAL / 45 < 0", };
        else if (tgtSQ()||tgtTR()) queries = new String[] {
                "SELECT entity_id,organization_id FROM atable where (A_DATE - A_DATE) * 5 < interval '0' day",
                "SELECT entity_id,organization_id FROM atable where interval '1' day + A_DATE  < A_DATE",
                "SELECT entity_id,organization_id FROM atable where A_DATE - 1 < A_DATE",
                "SELECT entity_id,organization_id FROM atable where A_INTEGER - 45 < 0",
                "SELECT entity_id,organization_id FROM atable where X_DECIMAL / 45 < 0", };

        for (String query : queries) {
            try {
                PreparedStatement statement = conn.prepareStatement(query);
                statement.executeQuery();
            }
            finally {
            }
        }
    }
   
    @Test 
    public void testValidStringConcatExpression() throws Exception {//test fails with stack overflow wee
        printTestDescription();

        int counter=0;
        initATableValues();
        String[] answers = null;
        if (tgtPH()) answers = new String[]{"00D300000000XHP5bar","a5bar","5bar","15bar","5bar","5bar"};
        // TRAF in both TRAF and SQ, null||'5'||'bar' is null,
        // not '5bar'.  a_date in the table are all null, so
        // we have to change the 3rd, 5th, and 6th answer to null for
        // this reason. 
        else if (tgtSQ()||tgtTR()) answers = new String[]{"TRAF5bar","a5bar",/* "5bar" */ null,"15bar",/* "5bar" */ null , /* "5bar" */ null};
        String[] queries = null;
        if (tgtPH()) queries = new String[] { 
        		"SELECT  organization_id || 5 || 'bar' FROM atable limit 1",
        		"SELECT a_string || 5 || 'bar' FROM atable limit 1",
        		"SELECT a_date||5||'bar' FROM atable limit 1",
        		"SELECT a_integer||5||'bar' FROM atable limit 1",
        		"SELECT x_decimal||5||'bar' FROM atable limit 1",
        		"SELECT x_long||5||'bar' FROM atable limit 1"
        };
        // TRAF: null||'5'||'bar' in both SQ and TRAF is null, not '5bar'
        else if (tgtTR()) queries = new String[] {
                        "SELECT RTRIM(organization_id) || '5' || 'bar' FROM atable order by 1 limit 1",
                        "SELECT a_string || '5' || 'bar' FROM atable order by 1 limit 1",
                        "SELECT RTRIM(CAST(a_date as CHAR(128)))||'5'||'bar' FROM atable order by 1 limit 1",
                        "SELECT RTRIM(CAST(a_integer as CHAR(128)))||'5'||'bar' FROM atable order by 1 limit 1",
                        "SELECT RTRIM(CAST(x_decimal as CHAR(128)))||'5'||'bar' FROM atable order by 1 desc limit 1",
                        "SELECT RTRIM(CAST(x_long as CHAR(128)))||'5'||'bar' FROM atable order by 1 desc limit 1"
        };
        // TRAF: null||'5'||'bar' in both SQ and TRAF is null, not '5bar'
        else if (tgtSQ()) queries = new String[] {
                        "SELECT [first 1] RTRIM(organization_id) || '5' || 'bar' FROM atable order by 1",
                        "SELECT [first 1] a_string || '5' || 'bar' FROM atable order by 1",
                        "SELECT [first 1] RTRIM(CAST(a_date as CHAR(128)))||'5'||'bar' FROM atable order by 1",
                        "SELECT [first 1] RTRIM(CAST(a_integer as CHAR(128)))||'5'||'bar' FROM atable order by 1",
                        "SELECT [first 1] RTRIM(CAST(x_decimal as CHAR(128)))||'5'||'bar' FROM atable order by 1 desc",
                        "SELECT [first 1] RTRIM(CAST(x_long as CHAR(128)))||'5'||'bar' FROM atable order by 1 desc"
        };

        for (String query : queries) {
        	try {
        		PreparedStatement statement = conn.prepareStatement(query);
        		ResultSet rs=statement.executeQuery();
        		assertTrue(rs.next());
        		assertEquals(answers[counter++],rs.getString(1));
           		assertFalse(rs.next());
        	}
        	finally {
        	}
        }
    }
    
    @Test 
    public void testStartKeyStopKey() throws SQLException {
        printTestDescription(); 
        // TRAF SKIP - SQ does not support SPLIT ON
        if (tgtSQ()||tgtTR()) return;

        conn.createStatement().execute("CREATE TABLE start_stop_test (pk char(2) not null primary key) SPLIT ON ('EA','EZ')");
        
        String query = null;
        if (tgtPH()) query = "select count(*) from start_stop_test where pk >= 'EA' and pk < 'EZ'";
        else if (tgtSQ()||tgtTR()) query = "select count(*) from start_stop_test where pk >= 'EA' and pk < 'EZ'";
        Statement statement = conn.createStatement();
        statement.execute(query);
        // TRAF PhoenixStatement pstatement = statement.unwrap(PhoenixStatement.class);
        // TRAF List<KeyRange>splits = pstatement.getQueryPlan().getSplits();
        // TRAF assertTrue(splits.size() > 0);
    }
    
    @Test
    public void testRowKeySingleIn() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and entity_id IN (?,?,?)";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and entity_id IN (?,?,?) order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, ROW2);
            statement.setString(3, ROW6);
            statement.setString(4, ROW8);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW2);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW6);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW8);
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testRowKeyMultiIn() throws Exception {
        printTestDescription();
        initATableValues();
        String query = null;
        if (tgtPH()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and entity_id IN (?,?,?) and a_string IN (?,?)";
        else if (tgtSQ()||tgtTR()) query = "SELECT entity_id FROM aTable WHERE organization_id=? and entity_id IN (?,?,?) and a_string IN (?,?) order by 1";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, ROW2);
            statement.setString(3, ROW6);
            statement.setString(4, ROW9);
            statement.setString(5, B_VALUE);
            statement.setString(6, C_VALUE);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW6);
            assertTrue (rs.next());
            assertEquals(rs.getString(1), ROW9);
            assertFalse(rs.next());
        } finally {
        }
    }
}
