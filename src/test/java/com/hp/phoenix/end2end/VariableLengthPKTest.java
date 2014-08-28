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
package test.java.com.hp.phoenix.end2end;

import static org.junit.Assert.*;
import org.junit.*;
import java.math.*;
import java.sql.*;
import java.sql.Date;
import java.util.*;
import java.text.*;

public class VariableLengthPKTest extends BaseTest {

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table " + PTSDB_NAME, "table " + PTSDB2_NAME, "table VarcharKeyTest", "table BTABLE", "table substr_test"));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */


// TRAF private static Format format = DateUtil.getDateParser(DateUtil.DEFAULT_DATE_FORMAT);
    private DateFormat format = new SimpleDateFormat("yyyy-MM-dd hh:mm:ss");
    private final String DS1 = "1970-01-01 00:58:00";
    private final Date D1 = toDate(DS1);
    private final Timestamp T1 = toTimestamp(DS1);

    private Date toDate(String dateString) {
        try {
            // TRAF return (Date)format.parseObject(dateString);
            return new Date(format.parse(dateString).getTime());
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

   private Timestamp toTimestamp(String dateString) {
        try {
            return new Timestamp(format.parse(dateString).getTime());
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }


    private void initGroupByRowKeyColumns() throws Exception {
        createTestTable(PTSDB_NAME);

        // Insert all rows at ts
        PreparedStatement stmt = null;
        if (tgtPH()) stmt = conn.prepareStatement(
                "upsert into " +
                "PTSDB(" +
                "    INST, " +
                "    HOST," +
                "    DATE)" +
                "VALUES (?, ?, CURRENT_DATE())");
        else if (tgtTR()) stmt = conn.prepareStatement(
                "upsert into " +
                "PTSDB(" +
                "    INST, " +
                "    HOST1," +
                "    DATE1)" +
                "VALUES (?, ?, CURRENT_TIMESTAMP)");
        else if (tgtSQ()) stmt = conn.prepareStatement(
                "insert into " +
                "PTSDB(" +
                "    INST, " +
                "    HOST1," +
                "    DATE1)" +
                "VALUES (?, ?, CURRENT_TIMESTAMP)");
        stmt.setString(1, "ab");
        stmt.setString(2, "a");
        stmt.execute();
        stmt.setString(1, "ac");
        stmt.setString(2, "b");
        stmt.execute();
        stmt.setString(1, "ad");
        stmt.setString(2, "a");
        stmt.execute();
    }

    private void initTableValues() throws Exception {
        createTestTable(PTSDB_NAME);

        // Insert all rows at ts
        conn.setAutoCommit(true);
        PreparedStatement stmt = null;
        if (tgtPH()) stmt = conn.prepareStatement(
                "upsert into " +
                "PTSDB(" +
                "    INST, " +
                "    HOST," +
                "    DATE," +
                "    VAL)" +
                "VALUES (?, ?, ?, ?)");
        else if (tgtTR()) stmt = conn.prepareStatement(
                "upsert into " +
                "PTSDB(" +
                "    INST, " +
                "    HOST1," +
                "    DATE1," +
                "    VAL)" +
                "VALUES (?, ?, ?, ?)");
        else if (tgtSQ()) stmt = conn.prepareStatement(
                "insert into " +
                "PTSDB(" +
                "    INST, " +
                "    HOST1," +
                "    DATE1," +
                "    VAL)" +
                "VALUES (?, ?, ?, ?)");
        stmt.setString(1, "abc");
        stmt.setString(2, "abc-def-ghi");
        if (tgtPH()) stmt.setDate(3, new Date(System.currentTimeMillis()));
        else if (tgtSQ()||tgtTR()) stmt.setTimestamp(3, new Timestamp(System.currentTimeMillis()));
        stmt.setBigDecimal(4, new BigDecimal(.5));
        stmt.execute();

        createTestTable(BTABLE_NAME);
        if (tgtPH()||tgtSQ())
            conn.setAutoCommit(false);
        else if (tgtTR())
            conn.setAutoCommit(true);

        // Insert all rows at ts
        if (tgtPH()) stmt = conn.prepareStatement(
                "upsert into " +
                "BTABLE(" +
                "    A_STRING, " +
                "    A_ID," +
                "    B_STRING," +
                "    A_INTEGER," +
                "    B_INTEGER," +
                "    C_INTEGER," +
                "    D_STRING," +
                "    E_STRING)" +
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
        else if (tgtTR()) stmt = conn.prepareStatement(
                "upsert into " +
                "BTABLE(" +
                "    A_STRING, " +
                "    A_ID," +
                "    B_STRING," +
                "    A_INTEGER," +
                "    B_INTEGER," +
                "    C_INTEGER," +
                "    D_STRING," +
                "    E_STRING," +
                "    C_STRING)" + // TRAF: C_STRING has been changed to not null and no default for it to be a primary key.
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, '')");
        else if (tgtSQ()) stmt = conn.prepareStatement(
                "insert into " +
                "BTABLE(" +
                "    A_STRING, " +
                "    A_ID," +
                "    B_STRING," +
                "    A_INTEGER," +
                "    B_INTEGER," +
                "    C_INTEGER," +
                "    D_STRING," +
                "    E_STRING," +
                "    C_STRING)" + // TRAF: C_STRING has been changed to not null and no default for it to be a primary key.
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, '')");
        stmt.setString(1, "abc");
        stmt.setString(2, "111");
        stmt.setString(3, "x");
        stmt.setInt(4, 1);
        stmt.setInt(5, 10);
        stmt.setInt(6, 1000);
        stmt.setString(7, null);
        stmt.setString(8, "0123456789");
        stmt.execute();

        stmt.setString(1, "abcd");
        stmt.setString(2, "222");
        stmt.setString(3, "xy");
        stmt.setInt(4, 2);
        stmt.setNull(5, Types.INTEGER);
        stmt.setNull(6, Types.INTEGER);
        stmt.execute();

        stmt.setString(3, "xyz");
        stmt.setInt(4, 3);
        stmt.setInt(5, 10);
        stmt.setInt(6, 1000);
        stmt.setString(7, "efg");
        stmt.execute();

        stmt.setString(3, "xyzz");
        stmt.setInt(4, 4);
        stmt.setInt(5, 40);
        stmt.setNull(6, Types.INTEGER);
        stmt.setString(7, null);
        stmt.execute();

        String ddl = null;
        if (tgtPH()) ddl = "create table VarcharKeyTest" +
            "   (pk varchar not null primary key)";
        else if (tgtSQ()||tgtTR()) ddl = "create table VarcharKeyTest" +
            "   (pk varchar(128) not null primary key)";
        conn.createStatement().execute(ddl);
        if (tgtPH()||tgtTR()) stmt = conn.prepareStatement(
                "upsert into " +
                "VarcharKeyTest(pk) " +
                "VALUES (?)");
        else if (tgtSQ()) stmt = conn.prepareStatement(
                "insert into " +
                "VarcharKeyTest(pk) " +
                "VALUES (?)");
        stmt.setString(1, "   def");
        stmt.execute();
        stmt.setString(1, "jkl   ");
        stmt.execute();
        stmt.setString(1, "   ghi   ");
        stmt.execute();

        if (tgtPH()||tgtSQ())
            conn.commit();
    }

    @Test
    public void testSingleColumnScanKey() throws Exception {
        printTestDescription();

        String query = "SELECT A_STRING,substr(a_id,1,1),B_STRING,A_INTEGER,B_INTEGER FROM BTABLE WHERE A_STRING=?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, "abc");
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("abc", rs.getString(1));
            assertEquals("1", rs.getString(2));
            assertEquals("x", rs.getString(3));
            assertEquals(1, rs.getInt(4));
            assertEquals(10, rs.getInt(5));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testSingleColumnGroupBy() throws Exception {
        printTestDescription();

        String query = "SELECT INST FROM PTSDB GROUP BY INST";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("abc", rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testNonfirstColumnGroupBy() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT HOST FROM PTSDB WHERE INST='abc' GROUP BY HOST";
        else if (tgtSQ()||tgtTR()) query = "SELECT HOST1 FROM PTSDB WHERE INST='abc' GROUP BY HOST1";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("abc-def-ghi", rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testGroupByRowKeyColumns() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT SUBSTR(INST,1,1),HOST FROM PTSDB GROUP BY SUBSTR(INST,1,1),HOST";
        else if (tgtSQ()||tgtTR()) query = "SELECT SUBSTR(INST,1,1),HOST1 FROM PTSDB GROUP BY SUBSTR(INST,1,1),HOST1 order by 1,2";
        try {
            initGroupByRowKeyColumns();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("a", rs.getString(1));
            assertEquals("a", rs.getString(2));
            assertTrue(rs.next());
            assertEquals("a", rs.getString(1));
            assertEquals("b", rs.getString(2));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testSkipScan() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT HOST FROM PTSDB WHERE INST='abc' AND DATE>=TO_DATE('1970-01-01 00:00:00') AND DATE <TO_DATE('2015-01-01 00:00:00')";
        else if (tgtSQ()||tgtTR()) query = "SELECT HOST1 FROM PTSDB WHERE INST='abc' AND DATE1>=TIMESTAMP '1970-01-01 00:00:00' AND DATE1 <TIMESTAMP '2015-01-01 00:00:00'";

        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("abc-def-ghi", rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testSkipMax() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT MAX(INST),MAX(DATE) FROM PTSDB WHERE INST='abc' AND DATE>=TO_DATE('1970-01-01 00:00:00') AND DATE <TO_DATE('2171-01-01 00:00:00')";
        else if (tgtSQ()||tgtTR()) query = "SELECT MAX(INST),MAX(DATE1) FROM PTSDB WHERE INST='abc' AND DATE1>=TIMESTAMP '1970-01-01 00:00:00' AND DATE1 <TIMESTAMP '2171-01-01 00:00:00'";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("abc", rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testSkipMaxWithLimit() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT MAX(INST),MAX(DATE) FROM PTSDB WHERE INST='abc' AND DATE>=TO_DATE('1970-01-01 00:00:00') AND DATE <TO_DATE('2171-01-01 00:00:00') LIMIT 2";
        else if (tgtTR()) query = "SELECT MAX(INST),MAX(DATE1) FROM PTSDB WHERE INST='abc' AND DATE1>=TIMESTAMP '1970-01-01 00:00:00' AND DATE1 <TIMESTAMP '2171-01-01 00:00:00' LIMIT 2";
        else if (tgtSQ()) query = "SELECT [FIRST 2] MAX(INST),MAX(DATE1) FROM PTSDB WHERE INST='abc' AND DATE1>=TIMESTAMP '1970-01-01 00:00:00' AND DATE1 <TIMESTAMP '2171-01-01 00:00:00'";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("abc", rs.getString(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testSingleColumnKeyFilter() throws Exception {
        printTestDescription();

        // Requires not null column to be projected, since the only one projected in the query is
        // nullable and will cause the no key value to be returned if it is the only one projected.
        String query = "SELECT A_STRING,substr(a_id,1,1),B_STRING,A_INTEGER,B_INTEGER FROM BTABLE WHERE B_STRING=?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, "xy");
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("abcd", rs.getString(1));
            assertEquals("2", rs.getString(2));
            assertEquals("xy", rs.getString(3));
            assertEquals(2, rs.getInt(4));
            assertEquals(0, rs.getInt(5));
            assertTrue(rs.wasNull());
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testMultiColumnEqScanKey() throws Exception {
        printTestDescription();

        // TODO: add compile test to confirm start/stop scan key
        String query = "SELECT A_STRING,substr(a_id,1,1),B_STRING,A_INTEGER,B_INTEGER FROM BTABLE WHERE A_STRING=? AND A_ID=? AND B_STRING=?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, "abcd");
            statement.setString(2, "222");
            statement.setString(3, "xy");
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("abcd", rs.getString(1));
            assertEquals("2", rs.getString(2));
            assertEquals("xy", rs.getString(3));
            assertEquals(2, rs.getInt(4));
            assertEquals(0, rs.getInt(5));
            assertTrue(rs.wasNull());
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testMultiColumnGTScanKey() throws Exception {
        printTestDescription();

        // TODO: add compile test to confirm start/stop scan key
        String query = null;
        if (tgtPH()) query = "SELECT A_STRING,substr(a_id,1,1),B_STRING,A_INTEGER,B_INTEGER FROM BTABLE WHERE A_STRING=? AND A_ID=? AND B_STRING>?";
        else if (tgtSQ()||tgtTR()) query = "SELECT A_STRING,substr(a_id,1,1),B_STRING,A_INTEGER,B_INTEGER FROM BTABLE WHERE A_STRING=? AND A_ID=? AND B_STRING>? order by 3 ";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, "abcd");
            statement.setString(2, "222");
            statement.setString(3, "xy");
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("abcd", rs.getString(1));
            assertEquals("2", rs.getString(2));
            assertEquals("xyz", rs.getString(3));
            assertEquals(3, rs.getInt(4));
            assertEquals(10, rs.getInt(5));
            assertTrue(rs.next());
            assertEquals("abcd", rs.getString(1));
            assertEquals("2", rs.getString(2));
            assertEquals("xyzz", rs.getString(3));
            assertEquals(4, rs.getInt(4));
            assertEquals(40, rs.getInt(5));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testMultiColumnGTKeyFilter() throws Exception {
        printTestDescription();

        // TODO: add compile test to confirm start/stop scan key
        String query = "SELECT A_STRING,substr(a_id,1,1),B_STRING,A_INTEGER,B_INTEGER FROM BTABLE WHERE A_STRING>? AND A_INTEGER>=?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, "abc");
            statement.setInt(2, 4);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("abcd", rs.getString(1));
            assertEquals("2", rs.getString(2));
            assertEquals("xyzz", rs.getString(3));
            assertEquals(4, rs.getInt(4));
            assertEquals(40, rs.getInt(5));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testNullValueEqualityScan() throws Exception {
        printTestDescription();

        createTestTable(PTSDB_NAME);

        // Insert all rows at ts
        conn.setAutoCommit(true);
        PreparedStatement stmt = null;
        if (tgtPH()||tgtTR()) stmt = conn.prepareStatement("upsert into PTSDB VALUES ('', '', ?, 0.5)");
        else if (tgtSQ()) stmt = conn.prepareStatement("insert into PTSDB VALUES ('', '', ?, 0.5)");
        if (tgtPH()) stmt.setDate(1, D1);
        else if (tgtSQ()||tgtTR()) stmt.setTimestamp(1, T1);
        stmt.execute();

        // Comparisons against null are always false.
        String query = null;
        if (tgtPH()) query = "SELECT HOST,DATE FROM PTSDB WHERE HOST='' AND INST=''";
        else if (tgtSQ()||tgtTR()) query = "SELECT HOST1,DATE1 FROM PTSDB WHERE HOST1='' AND INST=''"; 
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            if (tgtPH()) assertFalse(rs.next());
            else if (tgtSQ()||tgtTR()) assertTrue(rs.next()); // TRAF, the insert did put in '' and '' for HOST1 and INST
        } finally {
        }
    }

    @Test
    public void testVarLengthPKColScan() throws Exception {
        printTestDescription();

        createTestTable(PTSDB_NAME);

        conn.setAutoCommit(true);
        PreparedStatement stmt = null;
        if (tgtPH()||tgtTR()) stmt = conn.prepareStatement("upsert into PTSDB VALUES (?, 'y', ?, 0.5)");
        else if (tgtSQ()) stmt = conn.prepareStatement("insert into PTSDB VALUES (?, 'y', ?, 0.5)");
        stmt.setString(1, "x");
        if (tgtPH()) stmt.setDate(2, D1);
        else if (tgtSQ()||tgtTR()) stmt.setTimestamp(2, T1);
        stmt.execute();
        stmt.setString(1, "xy");
        stmt.execute();

        String query = null;
        if (tgtPH()) query = "SELECT HOST,DATE FROM PTSDB WHERE INST='x' AND HOST='y'";
        else if (tgtSQ()||tgtTR()) query = "SELECT HOST1,DATE1 FROM PTSDB WHERE INST='x' AND HOST1='y'";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(D1, rs.getDate(2));
            else if (tgtSQ()||tgtTR()) assertEquals(T1, rs.getTimestamp(2));
        } finally {
        }
    }

    @Test
    public void testEscapedQuoteScan() throws Exception {
        printTestDescription();

        createTestTable(PTSDB_NAME);
        conn.setAutoCommit(true);
        PreparedStatement stmt = null;
        if (tgtPH()||tgtTR()) stmt = conn.prepareStatement("upsert into PTSDB VALUES (?, 'y', ?, 0.5)");
        else if (tgtSQ()) stmt = conn.prepareStatement("insert into PTSDB VALUES (?, 'y', ?, 0.5)");
        stmt.setString(1, "x'y");
        if (tgtPH()) stmt.setDate(2, D1);
        else if (tgtSQ()||tgtTR()) stmt.setTimestamp(2, T1);
        stmt.execute();
        stmt.setString(1, "x");
        stmt.execute();

        String query1 = null;
        if (tgtPH()) query1 = "SELECT INST,DATE FROM PTSDB WHERE INST='x''y'";
        else if (tgtSQ()||tgtTR()) query1 = "SELECT INST,DATE1 FROM PTSDB WHERE INST='x''y'";
        String query2 = null;
        if (tgtPH()) query2 = "SELECT INST,DATE FROM PTSDB WHERE INST='x\\\'y'";
        else if (tgtSQ()||tgtTR()) query2 = "SELECT INST,DATE1 FROM PTSDB WHERE INST='x''y'";
        try {
            PreparedStatement statement = conn.prepareStatement(query1);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("x'y", rs.getString(1));
            if (tgtPH()) assertEquals(D1, rs.getDate(2));
            else if (tgtSQ()||tgtTR()) assertEquals(T1, rs.getTimestamp(2));
            assertFalse(rs.next());

            statement = conn.prepareStatement(query2);
            rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("x'y", rs.getString(1));
            if (tgtPH()) assertEquals(D1, rs.getDate(2));
            else if (tgtSQ()||tgtTR()) assertEquals(T1, rs.getTimestamp(2));
            assertFalse(rs.next());
        } finally {
        }
    }

    private void initPtsdbTableValues() throws Exception {
        createTestTable(PTSDB_NAME);

        conn.setAutoCommit(true);
        PreparedStatement stmt = null;
        if (tgtPH()||tgtTR()) stmt = conn.prepareStatement("upsert into PTSDB VALUES ('x', 'y', ?, 0.5)");
        else if (tgtSQ()) stmt = conn.prepareStatement("insert into PTSDB VALUES ('x', 'y', ?, 0.5)");
        if (tgtPH()) stmt.setDate(1, D1);
        else if (tgtSQ()||tgtTR()) stmt.setTimestamp(1, T1);
        stmt.execute();
    }

    @Test
    public void testToStringOnDate() throws Exception {
        printTestDescription();

        initPtsdbTableValues();

        String query = null;
        if (tgtPH()) query = "SELECT HOST,DATE FROM PTSDB WHERE INST='x' AND HOST='y'";
        else if (tgtSQ()||tgtTR()) query = "SELECT HOST1,CAST(DATE1 as TIMESTAMP(1)) FROM PTSDB WHERE INST='x' AND HOST1='y'";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(D1.toString(), rs.getString(2));
            else if (tgtSQ()||tgtTR()) assertEquals(T1.toString(), rs.getString(2));
            assertFalse(rs.next());
        } finally {
        }
    }

    private void initPtsdbTableValues2(Date d) throws Exception {
        createTestTable(PTSDB2_NAME);

        conn.setAutoCommit(true);
        PreparedStatement stmt = null;
        if (tgtPH()) stmt = conn.prepareStatement("upsert into "+PTSDB2_NAME+"(inst,date,val2) VALUES (?, ?, ?)");
        else if (tgtTR()) stmt = conn.prepareStatement("upsert into "+PTSDB2_NAME+"(inst,date1,val2) VALUES (?, ?, ?)");
        else if (tgtSQ()) stmt = conn.prepareStatement("insert into "+PTSDB2_NAME+"(inst,date1,val2) VALUES (?, ?, ?)");
        stmt.setString(1, "a");
        stmt.setDate(2, d);
        stmt.setDouble(3, 101.3);
        stmt.execute();
        stmt.setString(1, "a");
        stmt.setDate(2, new Date(d.getTime() + 1 * MILLIS_IN_DAY));
        stmt.setDouble(3, 99.7);
        stmt.execute();
        stmt.setString(1, "a");
        stmt.setDate(2, new Date(d.getTime() - 1 * MILLIS_IN_DAY));
        stmt.setDouble(3, 105.3);
        stmt.execute();
        stmt.setString(1, "b");
        stmt.setDate(2, d);
        stmt.setDouble(3, 88.5);
        stmt.execute();
        stmt.setString(1, "b");
        stmt.setDate(2, new Date(d.getTime() + 1 * MILLIS_IN_DAY));
        stmt.setDouble(3, 89.7);
        stmt.execute();
        stmt.setString(1, "b");
        stmt.setDate(2, new Date(d.getTime() - 1 * MILLIS_IN_DAY));
        stmt.setDouble(3, 94.9);
        stmt.execute();
    }

    // TRAF: a new override function for Timestamp
    private void initPtsdbTableValues2(Timestamp d) throws Exception {
        createTestTable(PTSDB2_NAME);

        conn.setAutoCommit(true);
        PreparedStatement stmt = null;
        if (tgtPH()) stmt = conn.prepareStatement("upsert into "+PTSDB2_NAME+"(inst,date,val2) VALUES (?, ?, ?)");
        else if (tgtTR()) stmt = conn.prepareStatement("upsert into "+PTSDB2_NAME+"(inst,date1,val2) VALUES (?, ?, ?)");
        else if (tgtSQ()) stmt = conn.prepareStatement("insert into "+PTSDB2_NAME+"(inst,date1,val2) VALUES (?, ?, ?)");
        stmt.setString(1, "a");
        stmt.setTimestamp(2, d);
        stmt.setDouble(3, 101.3);
        stmt.execute();
        stmt.setString(1, "a");
        stmt.setTimestamp(2, new Timestamp(d.getTime() + 1 * MILLIS_IN_DAY));
        stmt.setDouble(3, 99.7);
        stmt.execute();
        stmt.setString(1, "a");
        stmt.setTimestamp(2, new Timestamp(d.getTime() - 1 * MILLIS_IN_DAY));
        stmt.setDouble(3, 105.3);
        stmt.execute();
        stmt.setString(1, "b");
        stmt.setTimestamp(2, d);
        stmt.setDouble(3, 88.5);
        stmt.execute();
        stmt.setString(1, "b");
        stmt.setTimestamp(2, new Timestamp(d.getTime() + 1 * MILLIS_IN_DAY));
        stmt.setDouble(3, 89.7);
        stmt.execute();
        stmt.setString(1, "b");
        stmt.setTimestamp(2, new Timestamp(d.getTime() - 1 * MILLIS_IN_DAY));
        stmt.setDouble(3, 94.9);
        stmt.execute();
    }


    @Test
    public void testRoundOnDate() throws Exception {
        printTestDescription();

        if (tgtPH()) {
            Date date = new Date(System.currentTimeMillis());
            initPtsdbTableValues2(date);
        } else if (tgtSQ()||tgtTR()) {
            Timestamp tsp = new Timestamp(System.currentTimeMillis());
            initPtsdbTableValues2(tsp);
        }

        String query = null;
        if (tgtPH()) query = "SELECT MAX(val2)"
        + " FROM "+PTSDB2_NAME
        + " WHERE inst='a'"
        + " GROUP BY ROUND(date,'day',1)"
        + " ORDER BY MAX(val2)"; // disambiguate row order
        // TRAF: (1) ORDER BY does not take a function.  (2) GROUP BY only takes a function when the function is part of the SELECTed list.
        else if (tgtSQ()||tgtTR()) query = "SELECT MAX(val2), DATE_TRUNC('hour',DATE_ADD(date1,interval '30' minute))"
        + " FROM "+PTSDB2_NAME
        + " WHERE inst='a'"
        + " GROUP BY DATE_TRUNC('hour',DATE_ADD(date1,interval '30' minute)) "
        + " ORDER BY 1"; // disambiguate row order
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(99.7, rs.getDouble(1), 1e-6);
            assertTrue(rs.next());
            assertEquals(101.3, rs.getDouble(1), 1e-6);
            assertTrue(rs.next());
            assertEquals(105.3, rs.getDouble(1), 1e-6);
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testOrderBy() throws Exception {
        printTestDescription();

        if (tgtPH()) {
            Date date = new Date(System.currentTimeMillis());
            initPtsdbTableValues2(date);
        } else if (tgtSQ()||tgtTR()) {
            Timestamp tsp = new Timestamp(System.currentTimeMillis());
            initPtsdbTableValues2(tsp);
        }

        String query = null;
        if (tgtPH()) query = "SELECT inst,MAX(val2),MIN(val2)"
        + " FROM "+PTSDB2_NAME
        + " GROUP BY inst,ROUND(date,'day',1)"
        + " ORDER BY inst,ROUND(date,'day',1)"
        ;
        // TRAF: (1) ORDER BY does not take a function.  (2) GROUP BY only takes a function when the function is part of the SELECTed list.
        else if (tgtSQ()||tgtTR()) query = "SELECT inst,MAX(val2),MIN(val2),DATE_TRUNC('hour',DATE_ADD(date1,interval '30' minute))"
        + " FROM "+PTSDB2_NAME
        + " GROUP BY inst,DATE_TRUNC('hour',DATE_ADD(date1,interval '30' minute))"
        + " ORDER BY 1, 2"
        ;
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            if (tgtPH()) {
                assertTrue(rs.next());
                assertEquals("a", rs.getString(1));
                assertEquals(105.3, rs.getDouble(2), 1e-6);
                assertEquals(105.3, rs.getDouble(3), 1e-6);
                assertTrue(rs.next());
                assertEquals("a", rs.getString(1));
                assertEquals(101.3, rs.getDouble(2), 1e-6);
                assertEquals(101.3, rs.getDouble(3), 1e-6);
                assertTrue(rs.next());
                assertEquals("a", rs.getString(1));
                assertEquals(99.7, rs.getDouble(2), 1e-6);
                assertEquals(99.7, rs.getDouble(3), 1e-6);
                assertTrue(rs.next());
                assertEquals("b", rs.getString(1));
                assertEquals(94.9, rs.getDouble(2), 1e-6);
                assertEquals(94.9, rs.getDouble(3), 1e-6);
                assertTrue(rs.next());
                assertEquals("b", rs.getString(1));
                assertEquals(88.5, rs.getDouble(2), 1e-6);
                assertEquals(88.5, rs.getDouble(3), 1e-6);
                assertTrue(rs.next());
                assertEquals("b", rs.getString(1));
                assertEquals(89.7, rs.getDouble(2), 1e-6);
                assertEquals(89.7, rs.getDouble(3), 1e-6);
                assertFalse(rs.next());
            } else if (tgtSQ() || tgtPH()) {
                assertTrue(rs.next());
                assertEquals("a", rs.getString(1));
                assertEquals(99.7, rs.getDouble(2), 1e-6);
                assertEquals(99.7, rs.getDouble(3), 1e-6);
                assertTrue(rs.next());
                assertEquals("a", rs.getString(1));
                assertEquals(101.3, rs.getDouble(2), 1e-6);
                assertEquals(101.3, rs.getDouble(3), 1e-6);
                assertTrue(rs.next());
                assertEquals("a", rs.getString(1));
                assertEquals(105.3, rs.getDouble(2), 1e-6);
                assertEquals(105.3, rs.getDouble(3), 1e-6);
                assertTrue(rs.next());
                assertEquals("b", rs.getString(1));
                assertEquals(88.5, rs.getDouble(2), 1e-6);
                assertEquals(88.5, rs.getDouble(3), 1e-6);
                assertTrue(rs.next());
                assertEquals("b", rs.getString(1));
                assertEquals(89.7, rs.getDouble(2), 1e-6);
                assertEquals(89.7, rs.getDouble(3), 1e-6);
                assertTrue(rs.next());
                assertEquals("b", rs.getString(1));
                assertEquals(94.9, rs.getDouble(2), 1e-6);
                assertEquals(94.9, rs.getDouble(3), 1e-6);
                assertFalse(rs.next());
            } 
        } finally {
        }
    }

    @Test
    public void testSelectCount() throws Exception {
        printTestDescription();

        if (tgtPH()) {
            Date date = new Date(System.currentTimeMillis());
            initPtsdbTableValues2(date);
        } else if (tgtSQ()||tgtTR()) {
            Timestamp tsp = new Timestamp(System.currentTimeMillis());
            initPtsdbTableValues2(tsp);
        }

        String query = "SELECT COUNT(*)"
        + " FROM "+PTSDB2_NAME
        + " WHERE inst='a'";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(3, rs.getInt(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testBatchUpsert() throws Exception {
        printTestDescription();

        createTestTable(PTSDB2_NAME);
        Date d = null;
        Timestamp t = null;
        if (tgtPH()) d = new Date(1); 
        else if (tgtSQ()||tgtTR()) t = new Timestamp(1);

        String query = "SELECT SUM(val1),SUM(val2),SUM(val3) FROM "+PTSDB2_NAME;
        String sql1 = null;
        String upt1 = null;
        if (tgtPH()) sql1 = "UPSERT INTO "+PTSDB2_NAME+"(inst,date,val1) VALUES (?, ?, ?)";
        else if (tgtTR()) sql1 = "UPSERT INTO "+PTSDB2_NAME+"(inst,date1,val1) VALUES (?, ?, ?)";
        else if (tgtSQ()) { 
            sql1 = "INSERT INTO "+PTSDB2_NAME+"(inst,date1,val1) VALUES (?, ?, ?)";
            upt1 = "UPDATE "+PTSDB2_NAME+" SET val1 = ? WHERE (inst, date1) = (?, ?)";
        }

        String sql2 = null;
        if (tgtPH()) sql2 = "UPSERT INTO "+PTSDB2_NAME+"(inst,date,val2) VALUES (?, ?, ?)";
        else if (tgtTR()) sql2 = "UPSERT INTO "+PTSDB2_NAME+"(inst,date1,val2) VALUES (?, ?, ?)";
        else if (tgtSQ()) sql2 = "INSERT INTO "+PTSDB2_NAME+"(inst,date1,val2) VALUES (?, ?, ?)";

        String sql3 = null;
        if (tgtPH()) sql3 = "UPSERT INTO "+PTSDB2_NAME+"(inst,date,val3) VALUES (?, ?, ?)";
        else if (tgtTR()) sql3 = "UPSERT INTO "+PTSDB2_NAME+"(inst,date1,val3) VALUES (?, ?, ?)"; 
        else if (tgtSQ()) sql3 = "INSERT INTO "+PTSDB2_NAME+"(inst,date1,val3) VALUES (?, ?, ?)";

        conn.setAutoCommit(false);
        // conn.setAutoCommit(true);

        {
            // verify precondition: SUM(val{1,2,3}) are null
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertNull(rs.getBigDecimal(1));
            assertNull(rs.getBigDecimal(2));
            assertNull(rs.getBigDecimal(3));
            assertFalse(rs.next());
            statement.close();
        }

        {
            PreparedStatement s = conn.prepareStatement(sql1);
            s.setString(1, "a");
            if (tgtPH()) s.setDate(2, d);
            else if (tgtSQ()||tgtTR()) s.setTimestamp(2, t);
            s.setInt(3, 1);
            assertEquals(1, s.executeUpdate());
            s.close();
        }
        {
            PreparedStatement s = conn.prepareStatement(sql2);
            s.setString(1, "b");
            if (tgtPH()) s.setDate(2, d);
            else if (tgtSQ()||tgtTR()) s.setTimestamp(2, t);
            s.setInt(3, 1);
            assertEquals(1, s.executeUpdate());
            s.close();
        }
        {
            PreparedStatement s = conn.prepareStatement(sql3);
            s.setString(1, "c");
            if (tgtPH()) s.setDate(2, d);
            else if (tgtSQ()||tgtTR()) s.setTimestamp(2, t);
            s.setInt(3, 1);
            assertEquals(1, s.executeUpdate());
            s.close();
        }
        {
            PreparedStatement s = null;
            if (tgtPH()) {
                s = conn.prepareStatement(sql1);
                s.setString(1, "a");
                s.setDate(2, d);
                s.setInt(3, 5);
            } else if (tgtTR()) {
                s = conn.prepareStatement(sql1);
                s.setString(1, "a");
                s.setTimestamp(2, t);
                s.setInt(3, 5);
            } else if (tgtSQ()) {
                s = conn.prepareStatement(upt1);
                s.setInt(1, 5);
                s.setString(2, "a");
                s.setTimestamp(3, t);
            }
            assertEquals(1, s.executeUpdate());
            s.close();
        }
        {
            PreparedStatement s = null;
            if (tgtPH()) {
                s = conn.prepareStatement(sql1);
                s.setString(1, "b");
                s.setDate(2, d);
                s.setInt(3, 5);
            } else if (tgtTR()) {
                s = conn.prepareStatement(sql1);
                s.setString(1, "b");
                s.setTimestamp(2, t);
                s.setInt(3, 5);
            } else if (tgtSQ()) {
                s = conn.prepareStatement(upt1);
                s.setInt(1, 5);
                s.setString(2, "b");
                s.setTimestamp(3, t);
            }
            assertEquals(1, s.executeUpdate());
            s.close();
        }
        {
            PreparedStatement s = null;
            if (tgtPH()) {
                s = conn.prepareStatement(sql1);
                s.setString(1, "c");
                s.setDate(2, d);
                s.setInt(3, 5);
            } else if (tgtTR()) {
                s = conn.prepareStatement(sql1);
                s.setString(1, "c");
                s.setTimestamp(2, t);
                s.setInt(3, 5);
            } else if (tgtSQ()) {
                s = conn.prepareStatement(upt1);
                s.setInt(1, 5);
                s.setString(2, "c");
                s.setTimestamp(3, t);
            }
            assertEquals(1, s.executeUpdate());
            s.close();
        }
        conn.commit();
       
        // Query at a time after the upsert to confirm they took place
        {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(15, rs.getDouble(1), 1e-6);
            assertEquals(1, rs.getDouble(2), 1e-6);
            assertEquals(1, rs.getDouble(3), 1e-6);
            assertFalse(rs.next());
            statement.close();
        }
    }

    @Test
    public void testSelectStar() throws Exception {
        printTestDescription();

        initPtsdbTableValues();

        String query = null;
        if (tgtPH()) query = "SELECT * FROM PTSDB WHERE INST='x' AND HOST='y'";
        else if (tgtSQ()||tgtTR()) query = "SELECT * FROM PTSDB WHERE INST='x' AND HOST1='y'";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("x",rs.getString("inst"));
            if (tgtPH()) assertEquals("y",rs.getString("host"));
            else if (tgtSQ()||tgtTR()) assertEquals("y",rs.getString("host1"));
            if (tgtPH()) assertEquals(D1, rs.getDate("date"));
            else if (tgtSQ()||tgtTR()) assertEquals(T1, rs.getTimestamp("date1"));
            if (tgtPH()) assertEquals(BigDecimal.valueOf(0.5), rs.getBigDecimal("val"));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(0.5).setScale(10), rs.getBigDecimal("val"));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test 
    public void testToCharOnDate() throws Exception {
        printTestDescription();

        initPtsdbTableValues();

        String query = null;
        if (tgtPH()) query = "SELECT HOST,TO_CHAR(DATE) FROM PTSDB WHERE INST='x' AND HOST='y'";
        else if (tgtSQ()||tgtTR()) query = "SELECT HOST1,RTRIM(CAST(CAST(DATE1 as TIMESTAMP(1)) as CHAR(128))) FROM PTSDB WHERE INST='x' AND HOST1='y'";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(D1.toString(), rs.getString(2));
            else if (tgtSQ()||tgtTR()) assertEquals(T1.toString(), rs.getString(2));
        } finally {
        }
    }

    @Test
    public void testToCharWithFormatOnDate() throws Exception {
        printTestDescription();

        initPtsdbTableValues();

        String format = "HH:mm:ss";
        String query = null;
        if (tgtPH()) query = "SELECT HOST,TO_CHAR(DATE,'" + format + "') FROM PTSDB WHERE INST='x' AND HOST='y'";
        else if (tgtSQ()||tgtTR()) query = "SELECT HOST1,RTRIM(CAST(CAST(DATE1 as timestamp(1)) as CHAR(128))) FROM PTSDB WHERE INST='x' AND HOST1='y'";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(D1.toString(), rs.getString(2));
            else if (tgtSQ()||tgtTR()) assertEquals(T1.toString(), rs.getString(2));
        } finally {
        }
    }

    @Test
    public void testToDateWithFormatOnDate() throws Exception {
        printTestDescription();

        initPtsdbTableValues();

        String format = "yyyy-MM-dd HH:mm:ss.S";
        String query = null;
        if (tgtPH()) query = "SELECT HOST,TO_CHAR(DATE,'" + format + "') FROM PTSDB WHERE INST='x' AND HOST='y' and DATE=TO_DATE(?,'" + format + "')";
        else if (tgtSQ()||tgtTR()) query = "SELECT HOST1,RTRIM(CAST(CAST(DATE1 as timestamp(1)) as CHAR(128))) FROM PTSDB WHERE INST='x' AND HOST1='y' and DATE1=?";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            if (tgtPH()) statement.setString(1, D1.toString());
            else if (tgtSQ()||tgtTR()) statement.setString(1, T1.toString());
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(D1.toString(), rs.getString(2));
            else if (tgtSQ()||tgtTR()) assertEquals(T1.toString(), rs.getString(2));

        } finally {
        }
    }

    @Test
    public void testMissingPKColumn() throws Exception {
        printTestDescription();

        createTestTable(PTSDB_NAME);

        conn.setAutoCommit(true);
        Statement stmt = conn.createStatement();
        try {
            if (tgtPH()) stmt.execute("upsert into PTSDB(INST,HOST,VAL) VALUES ('abc', 'abc-def-ghi', 0.5)");
            else if (tgtTR()) stmt.execute("upsert into PTSDB(INST,HOST1,VAL,DATE1) VALUES ('abc', 'abc-def-ghi', 0.5, null)");
            else if (tgtSQ()) stmt.execute("insert into PTSDB(INST,HOST1,VAL,DATE1) VALUES ('abc', 'abc-def-ghi', 0.5, null)");
            fail();
        } catch (Exception e) {
            if (tgtPH()) assertTrue(e.getMessage().contains("may not be null"));
            else if (tgtSQ()||tgtTR()) assertTrue(e.getMessage().contains("*** ERROR[4122] NULL cannot be assigned to NOT NULL column"));
        } finally {
        }
    }

    @Test
    public void testNoKVColumn() throws Exception {
        printTestDescription();

        createTestTable(BTABLE_NAME);

        conn.setAutoCommit(true);
        PreparedStatement stmt = null;
        if (tgtPH()) stmt = conn.prepareStatement(
                "upsert into BTABLE VALUES (?, ?, ?, ?, ?)");
        else if (tgtTR()) stmt = conn.prepareStatement(
                "upsert into BTABLE (a_string, a_id, b_string, a_integer, c_string) VALUES (?, ?, ?, ?, ?)");
        else if (tgtSQ()) stmt = conn.prepareStatement(
                "insert into BTABLE (a_string, a_id, b_string, a_integer, c_string) VALUES (?, ?, ?, ?, ?)");
        stmt.setString(1, "abc");
        stmt.setString(2, "123");
        stmt.setString(3, "x");
        stmt.setInt(4, 1);
        stmt.setString(5, "ab");
        // Succeeds since we have an empty KV
        stmt.execute();
    }

    // Broken, since we don't know if insert vs update. @Test
    public void testMissingKVColumn() throws Exception {
        printTestDescription();

        createTestTable(BTABLE_NAME);

        conn.setAutoCommit(true);
        PreparedStatement stmt = conn.prepareStatement(
                "upsert into BTABLE VALUES (?, ?, ?, ?, ?, ?)");
        stmt.setString(1, "abc");
        stmt.setString(2, "123");
        stmt.setString(3, "x");
        stmt.setInt(4, 1);
        stmt.setString(5, "ab");
        stmt.setInt(6, 1);
        try {
            stmt.execute();
            fail();
        } catch (Exception e) {
            // Non nullable key value E_STRING has no value
            assertTrue(e.getMessage().contains("may not be null"));
        } finally {
        }
    }

    @Test
    public void testTooShortKVColumn() throws Exception {
        printTestDescription();

        createTestTable(BTABLE_NAME);

        conn.setAutoCommit(true);
        // Insert all rows at ts
        PreparedStatement stmt = null;
        if (tgtPH()||tgtTR()) stmt = conn.prepareStatement(
                "upsert into " +
                "BTABLE(" +
                "    A_STRING, " +
                "    A_ID," +
                "    B_STRING," +
                "    A_INTEGER," +
                "    C_STRING," +
                "    E_STRING)" +
                "VALUES (?, ?, ?, ?, ?, ?)");
        else if (tgtSQ()) stmt = conn.prepareStatement(
                "insert into " +
                "BTABLE(" +
                "    A_STRING, " +
                "    A_ID," +
                "    B_STRING," +
                "    A_INTEGER," +
                "    C_STRING," +
                "    E_STRING)" +
                "VALUES (?, ?, ?, ?, ?, ?)");
        stmt.setString(1, "abc");
        stmt.setString(2, "123");
        stmt.setString(3, "x");
        stmt.setInt(4, 1);
        stmt.setString(5, "ab");
        stmt.setString(6, "01234");

        try {
            stmt.execute();
        } catch (Exception e) {
            fail("Constraint voilation Exception should not be thrown, the characters have to be padded");
        } finally {
        }
    }

    @Test
    public void testTooShortPKColumn() throws Exception {
        printTestDescription();

        createTestTable(BTABLE_NAME);

        conn.setAutoCommit(true);
        // Insert all rows at ts
        PreparedStatement stmt = null;
        if (tgtPH()||tgtTR()) stmt = conn.prepareStatement(
                "upsert into " +
                "BTABLE(" +
                "    A_STRING, " +
                "    A_ID," +
                "    B_STRING," +
                "    A_INTEGER," +
                "    C_STRING," +
                "    E_STRING)" +
                "VALUES (?, ?, ?, ?, ?, ?)");
        else if (tgtSQ()) stmt = conn.prepareStatement(
                "insert into " +
                "BTABLE(" +
                "    A_STRING, " +
                "    A_ID," +
                "    B_STRING," +
                "    A_INTEGER," +
                "    C_STRING," +
                "    E_STRING)" +
                "VALUES (?, ?, ?, ?, ?, ?)");
        stmt.setString(1, "abc");
        stmt.setString(2, "12");
        stmt.setString(3, "x");
        stmt.setInt(4, 1);
        stmt.setString(5, "ab");
        stmt.setString(6, "0123456789");

        try {
            stmt.execute();
        } catch (Exception e) {
            fail("Constraint voilation Exception should not be thrown, the characters have to be padded");
        } finally {
        }
    }

    @Test
    public void testTooLongPKColumn() throws Exception {
        printTestDescription();

        createTestTable(BTABLE_NAME);

        conn.setAutoCommit(true);
        // Insert all rows at ts
        PreparedStatement stmt = null;
        if (tgtPH()||tgtTR()) stmt = conn.prepareStatement(
                "upsert into " +
                "BTABLE(" +
                "    A_STRING, " +
                "    A_ID," +
                "    B_STRING," +
                "    A_INTEGER," +
                "    C_STRING," +
                "    E_STRING)" +
                "VALUES (?, ?, ?, ?, ?, ?)");
        else if (tgtSQ()) stmt = conn.prepareStatement(
                "insert into " +
                "BTABLE(" +
                "    A_STRING, " +
                "    A_ID," +
                "    B_STRING," +
                "    A_INTEGER," +
                "    C_STRING," +
                "    E_STRING)" +
                "VALUES (?, ?, ?, ?, ?, ?)");
        stmt.setString(1, "abc");
        stmt.setString(2, "123");
        stmt.setString(3, "x");
        stmt.setInt(4, 1);
        stmt.setString(5, "abc");
        stmt.setString(6, "0123456789");

        try {
            stmt.execute();
            fail();
        } catch (Exception e) {
            if (tgtPH()) assertTrue(e.getMessage().contains(" may not exceed 2 bytes"));
            // This error message is different between T2 and T4.
            // Only make sure that we get an exception now.
            // else if (tgtSQ()||tgtTR()) assertTrue(e.getMessage().contains("VARCHAR data longer than column length"));
        } finally {
        }
    }

    @Test
    public void testTooLongKVColumn() throws Exception {
        printTestDescription();

        createTestTable(BTABLE_NAME);

        conn.setAutoCommit(true);
        // Insert all rows at ts
        PreparedStatement stmt = null;
        if (tgtPH()||tgtTR()) stmt = conn.prepareStatement(
                "upsert into " +
                "BTABLE(" +
                "    A_STRING, " +
                "    A_ID," +
                "    B_STRING," +
                "    A_INTEGER," +
                "    C_STRING," +
                "    D_STRING," +
                "    E_STRING)" +
                "VALUES (?, ?, ?, ?, ?, ?, ?)");
        else if (tgtSQ()) stmt = conn.prepareStatement(
                "insert into " +
                "BTABLE(" +
                "    A_STRING, " +
                "    A_ID," +
                "    B_STRING," +
                "    A_INTEGER," +
                "    C_STRING," +
                "    D_STRING," +
                "    E_STRING)" +
                "VALUES (?, ?, ?, ?, ?, ?, ?)");
        stmt.setString(1, "abc");
        stmt.setString(2, "123");
        stmt.setString(3, "x");
        stmt.setInt(4, 1);
        stmt.setString(5, "ab");
        stmt.setString(6,"abcd");
        stmt.setString(7, "0123456789");

        try {
            stmt.execute();
            fail();
        } catch (Exception e) {
            if (tgtPH()) assertTrue(e.getMessage().contains(" may not exceed 3 bytes"));
            // This error message is different between T2 and T4.  
            // Only make sure that we get an exception now. 
            // else if (tgtSQ()||tgtTR()) assertTrue(e.getMessage().contains("VARCHAR data longer than column length"));
        } finally {
        }
    }

    @Test
    public void testMultiFixedLengthNull() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT B_INTEGER,C_INTEGER,COUNT(1) FROM BTABLE GROUP BY C_INTEGER,B_INTEGER";
        else if (tgtSQ()||tgtTR()) query = "SELECT B_INTEGER,C_INTEGER,COUNT(1) FROM BTABLE GROUP BY C_INTEGER,B_INTEGER order by 1";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            if (tgtPH()) {
                assertTrue(rs.next());
                assertEquals(0, rs.getInt(1));
                assertTrue(rs.wasNull());
                assertEquals(0, rs.getInt(2));
                assertTrue(rs.wasNull());
                assertEquals(1, rs.getLong(3));
            }

            assertTrue(rs.next());
            assertEquals(10, rs.getInt(1));
            assertEquals(1000, rs.getInt(2));
            assertEquals(2, rs.getLong(3));

            assertTrue(rs.next());
            assertEquals(40, rs.getInt(1));
            assertEquals(0, rs.getInt(2));
            assertTrue(rs.wasNull());
            assertEquals(1, rs.getLong(3));

            if (tgtSQ()||tgtTR()) {
                assertTrue(rs.next());
                assertEquals(0, rs.getInt(1));
                assertTrue(rs.wasNull());
                assertEquals(0, rs.getInt(2));
                assertTrue(rs.wasNull());
                assertEquals(1, rs.getLong(3));
            }

            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testSingleFixedLengthNull() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT C_INTEGER,COUNT(1) FROM BTABLE GROUP BY C_INTEGER";
        else if (tgtSQ()||tgtTR()) query = "SELECT C_INTEGER,COUNT(1) FROM BTABLE GROUP BY C_INTEGER order by 1 desc";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(0, rs.getInt(1));
            assertTrue(rs.wasNull());
            assertEquals(2, rs.getLong(2));

            assertTrue(rs.next());
            assertEquals(1000, rs.getInt(1));
            assertEquals(2, rs.getLong(2));

            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testMultiMixedTypeGroupBy() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT A_ID, E_STRING, D_STRING, C_INTEGER, COUNT(1) FROM BTABLE GROUP BY A_ID, E_STRING, D_STRING, C_INTEGER";
        else if (tgtSQ()||tgtTR()) query = "SELECT A_ID, E_STRING, D_STRING, C_INTEGER, COUNT(1) FROM BTABLE GROUP BY A_ID, E_STRING, D_STRING, C_INTEGER order by 1, 3 desc";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("111", rs.getString(1));
            assertEquals("0123456789", rs.getString(2));
            assertEquals(null, rs.getString(3));
            assertEquals(1000, rs.getInt(4));
            assertEquals(1, rs.getInt(5));

            assertTrue(rs.next());
            assertEquals("222", rs.getString(1));
            assertEquals("0123456789", rs.getString(2));
            assertEquals(null, rs.getString(3));
            assertEquals(0, rs.getInt(4));
            assertTrue(rs.wasNull());
            assertEquals(2, rs.getInt(5));

            assertTrue(rs.next());
            assertEquals("222", rs.getString(1));
            assertEquals("0123456789", rs.getString(2));
            assertEquals("efg", rs.getString(3));
            assertEquals(1000, rs.getInt(4));
            assertEquals(1, rs.getInt(5));

            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testSubstrFunction() throws Exception {
        printTestDescription();

        String query[] = null;
        if (tgtPH()) query = new String[] {
            "SELECT substr('ABC',-1,1) FROM BTABLE LIMIT 1",
            "SELECT substr('ABC',-4,1) FROM BTABLE LIMIT 1",
            "SELECT substr('ABC',2,4) FROM BTABLE LIMIT 1",
            "SELECT substr('ABC',1,1) FROM BTABLE LIMIT 1",
            "SELECT substr('ABC',0,1) FROM BTABLE LIMIT 1",
            // Test for multibyte characters support.
            "SELECT substr('ĎďĒ',0,1) FROM BTABLE LIMIT 1",
            "SELECT substr('ĎďĒ',0,2) FROM BTABLE LIMIT 1",
            "SELECT substr('ĎďĒ',1,1) FROM BTABLE LIMIT 1",
            "SELECT substr('ĎďĒ',1,2) FROM BTABLE LIMIT 1",
            "SELECT substr('ĎďĒ',2,1) FROM BTABLE LIMIT 1",
            "SELECT substr('ĎďĒ',2,2) FROM BTABLE LIMIT 1",
            "SELECT substr('ĎďĒ',-1,1) FROM BTABLE LIMIT 1",
            "SELECT substr('Ďďɚʍ',2,4) FROM BTABLE LIMIT 1",
            "SELECT pk FROM VarcharKeyTest WHERE substr(pk, 0, 3)='jkl'",
        };
        else if (tgtTR()) query = new String[] {
            "SELECT substr('ABC',-1,1) FROM BTABLE LIMIT 1",
            "SELECT substr('ABC',-4,1) FROM BTABLE LIMIT 1",
            "SELECT substr('ABC',2,4) FROM BTABLE LIMIT 1",
            "SELECT substr('ABC',1,1) FROM BTABLE LIMIT 1",
            "SELECT substr('ABC',0,1) FROM BTABLE LIMIT 1",
            // Test for multibyte characters support.
            // TRAF: use ASCII for now.
            "SELECT substr('ABC',0,2) FROM BTABLE LIMIT 1",
            "SELECT substr('ABC',0,3) FROM BTABLE LIMIT 1",
            "SELECT substr('ABC',1,2) FROM BTABLE LIMIT 1",
            "SELECT substr('ABC',1,3) FROM BTABLE LIMIT 1",
            "SELECT substr('ABC',2,1) FROM BTABLE LIMIT 1",
            "SELECT substr('ABC',2,2) FROM BTABLE LIMIT 1",
            "SELECT substr('ABC',2,4) FROM BTABLE LIMIT 1",
            "SELECT substr('ABC',1,100) FROM BTABLE LIMIT 1",
            "SELECT pk FROM VarcharKeyTest WHERE substr(pk, 1, 3)='jkl'",
         };
        else if (tgtSQ()) query = new String[] {
            "SELECT [FIRST 1] substr('ABC',-1,1) FROM BTABLE",
            "SELECT [FIRST 1] substr('ABC',-4,1) FROM BTABLE",
            "SELECT [FIRST 1] substr('ABC',2,4) FROM BTABLE",
            "SELECT [FIRST 1] substr('ABC',1,1) FROM BTABLE",
            "SELECT [FIRST 1] substr('ABC',0,1) FROM BTABLE",
            // Test for multibyte characters support.
            // TRAF: use ASCII for now.
            "SELECT [FIRST 1] substr('ABC',0,2) FROM BTABLE",
            "SELECT [FIRST 1] substr('ABC',0,3) FROM BTABLE",
            "SELECT [FIRST 1] substr('ABC',1,2) FROM BTABLE",
            "SELECT [FIRST 1] substr('ABC',1,3) FROM BTABLE",
            "SELECT [FIRST 1] substr('ABC',2,1) FROM BTABLE",
            "SELECT [FIRST 1] substr('ABC',2,2) FROM BTABLE",
            "SELECT [FIRST 1] substr('ABC',2,4) FROM BTABLE",
            "SELECT [FIRST 1] substr('ABC',1,100) FROM BTABLE",
            "SELECT pk FROM VarcharKeyTest WHERE substr(pk, 1, 3)='jkl'",
         };
        String result[] = null;
        if (tgtPH()) result = new String[]{
            "C",
            null,
            "BC",
            "A",
            "A",
            "Ď",
            "Ďď",
            "Ď",
            "Ďď",
            "ď",
            "ďĒ",
            "Ē",
            "ďɚʍ",
            "jkl   ",
        };
        else if (tgtSQ()||tgtTR()) result = new String[]{
            "",
            "",
            "BC",
            "A",
            "",
            "A",
            "AB",
            "AB",
            "ABC",
            "B",
            "BC",
            "BC",
            "ABC",
            "jkl   ",
        };
        assertEquals(query.length,result.length);
        try {
            initTableValues();
            for (int i = 0; i < query.length; i++) {
                PreparedStatement statement = conn.prepareStatement(query[i]);
                ResultSet rs = statement.executeQuery();
                assertTrue(rs.next());
                assertEquals(result[i], rs.getString(1));
                assertFalse(rs.next());
            }
        } finally {
        }
    }

    @Test
    public void testRegexReplaceFunction() throws Exception {
        printTestDescription();

        if (tgtSQ()||tgtTR()) return; // TRAF: do not support regexp_replace()

        // NOTE: we need to double escape the "\\" here because conn.prepareStatement would
        // also try to evaluate the escaping. As a result, to represent what normally would be
        // a "\d" in this test, it would become "\\\\d".
        String query[] = {
            "SELECT regexp_replace('', '') FROM BTABLE LIMIT 1",
            "SELECT regexp_replace('', 'abc', 'def') FROM BTABLE LIMIT 1",
            "SELECT regexp_replace('123abcABC', '[a-z]+') FROM BTABLE LIMIT 1",
            "SELECT regexp_replace('123-abc-ABC', '-[a-zA-Z-]+') FROM BTABLE LIMIT 1",
            "SELECT regexp_replace('abcABC123', '\\\\d+', '') FROM BTABLE LIMIT 1",
            "SELECT regexp_replace('abcABC123', '\\\\D+', '') FROM BTABLE LIMIT 1",
            "SELECT regexp_replace('abc', 'abc', 'def') FROM BTABLE LIMIT 1",
            "SELECT regexp_replace('abc123ABC', '\\\\d+', 'def') FROM BTABLE LIMIT 1",
            "SELECT regexp_replace('abc123ABC', '[0-9]+', '#') FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN regexp_replace('abcABC123', '[a-zA-Z]+') = '123' THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT A_STRING FROM BTABLE WHERE A_ID = regexp_replace('abcABC111', '[a-zA-Z]+') LIMIT 1", // 111
            // Test for multibyte characters support.
            "SELECT regexp_replace('Ďď Ēĕ ĜĞ ϗϘϛϢ', '[a-zA-Z]+') from BTABLE LIMIT 1",
            "SELECT regexp_replace('Ďď Ēĕ ĜĞ ϗϘϛϢ', '[Ď-ě]+', '#') from BTABLE LIMIT 1",
            "SELECT regexp_replace('Ďď Ēĕ ĜĞ ϗϘϛϢ', '.+', 'replacement') from BTABLE LIMIT 1",
            "SELECT regexp_replace('Ďď Ēĕ ĜĞ ϗϘϛϢ', 'Ďď', 'DD') from BTABLE LIMIT 1",
        };
        String result[] = {
            null,
            null,
            "123ABC",
            "123",
            "abcABC",
            "123",
            "def",
            "abcdefABC",
            "abc#ABC",
            "1",
            "abc", // the first column
            "Ďď Ēĕ ĜĞ ϗϘϛϢ",
            "# # ĜĞ ϗϘϛϢ",
            "replacement",
            "DD Ēĕ ĜĞ ϗϘϛϢ",
        };
        assertEquals(query.length,result.length);
        try {
            initTableValues();
            for (int i = 0; i < query.length; i++) {
                PreparedStatement statement = conn.prepareStatement(query[i]);
                ResultSet rs = statement.executeQuery();
                assertTrue(rs.next());
                assertEquals(result[i], rs.getString(1));
                assertFalse(rs.next());
            }
        } finally {
        }
    }

    @Test
    public void testRegexpSubstrFunction() throws Exception {
        printTestDescription();

        if (tgtSQ()||tgtTR()) return; // TRAF: do not support regexp_substr()

        String query[] = {
            "SELECT regexp_substr('', '', 0) FROM BTABLE LIMIT 1",
            "SELECT regexp_substr('', '', 1) FROM BTABLE LIMIT 1",
            "SELECT regexp_substr('', 'abc', 0) FROM BTABLE LIMIT 1",
            "SELECT regexp_substr('abc', '', 0) FROM BTABLE LIMIT 1",
            "SELECT regexp_substr('123', '123', 3) FROM BTABLE LIMIT 1",
            "SELECT regexp_substr('123', '123', -4) FROM BTABLE LIMIT 1",
            "SELECT regexp_substr('123ABC', '[a-z]+', 0) FROM BTABLE LIMIT 1",
            "SELECT regexp_substr('123ABC', '[0-9]+', 4) FROM BTABLE LIMIT 1",
            "SELECT regexp_substr('123ABCabc', '\\\\d+', 0) FROM BTABLE LIMIT 1",
            "SELECT regexp_substr('123ABCabc', '\\\\D+', 0) FROM BTABLE LIMIT 1",
            "SELECT regexp_substr('123ABCabc', '\\\\D+', 4) FROM BTABLE LIMIT 1",
            "SELECT regexp_substr('123ABCabc', '\\\\D+', 7) FROM BTABLE LIMIT 1",
            "SELECT regexp_substr('na11-app5-26-sjl', '[^-]+', 0) FROM BTABLE LIMIT 1",
            "SELECT regexp_substr('na11-app5-26-sjl', '[^-]+') FROM BTABLE LIMIT 1",
            // Test for multibyte characters support.
            "SELECT regexp_substr('ĎďĒĕĜĞ', '.+') from BTABLE LIMIT 1",
            "SELECT regexp_substr('ĎďĒĕĜĞ', '.+', 3) from BTABLE LIMIT 1",
            "SELECT regexp_substr('ĎďĒĕĜĞ', '[a-zA-Z]+', 0) from BTABLE LIMIT 1",
            "SELECT regexp_substr('ĎďĒĕĜĞ', '[Ď-ě]+', 3) from BTABLE LIMIT 1",
        };
        String result[] = {
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            "123",
            "ABCabc",
            "ABCabc",
            "abc",
            "na11",
            "na11",
            "ĎďĒĕĜĞ",
            "ĒĕĜĞ",
            null,
            "Ēĕ",
        };
        assertEquals(query.length,result.length);
        try {
            initTableValues();
            for (int i = 0; i < query.length; i++) {
                PreparedStatement statement = conn.prepareStatement(query[i]);
                ResultSet rs = statement.executeQuery();
                assertTrue(rs.next());
                assertEquals(result[i], rs.getString(1));
                assertFalse(rs.next());
            }
        } finally {
        }
    }

    @Test
    public void testLikeConstant() throws Exception {
        printTestDescription();

        String query[] = null;
        if (tgtPH()) query = new String[] {
            "SELECT CASE WHEN 'ABC' LIKE '' THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN 'ABC' LIKE 'A_' THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN 'ABC' LIKE 'A__' THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN 'AB_C' LIKE 'AB\\_C' THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN 'ABC%DE' LIKE 'ABC\\%D%' THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
        };
        else if (tgtTR()) query = new String[] {
            "SELECT CASE WHEN 'ABC' LIKE '' THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN 'ABC' LIKE 'A_' THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN 'ABC' LIKE 'A__' THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN 'AB_C' LIKE 'AB\\_C' ESCAPE '\\' THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN 'ABC%DE' LIKE 'ABC\\%D%' ESCAPE '\\' THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
        };
        else if (tgtSQ()) query = new String[] {
            "SELECT [FIRST 1] CASE WHEN 'ABC' LIKE '' THEN '1' ELSE '2' END FROM BTABLE",
            "SELECT [FIRST 1] CASE WHEN 'ABC' LIKE 'A_' THEN '1' ELSE '2' END FROM BTABLE",
            "SELECT [FIRST 1] CASE WHEN 'ABC' LIKE 'A__' THEN '1' ELSE '2' END FROM BTABLE",
            "SELECT [FIRST 1] CASE WHEN 'AB_C' LIKE 'AB\\_C' ESCAPE '\\' THEN '1' ELSE '2' END FROM BTABLE",
            "SELECT [FIRST 1] CASE WHEN 'ABC%DE' LIKE 'ABC\\%D%' ESCAPE '\\' THEN '1' ELSE '2' END FROM BTABLE",
        };
        String result[] = {
            "2",
            "2",
            "1",
            "1",
            "1",
        };
        assertEquals(query.length,result.length);
        try {
            initTableValues();
            for (int i = 0; i < query.length; i++) {
                PreparedStatement statement = conn.prepareStatement(query[i]);
                ResultSet rs = statement.executeQuery();
                assertTrue(rs.next());
                assertEquals(query[i],result[i], rs.getString(1));
                assertFalse(rs.next());
            }
        } finally {
        }
    }

    @Test
    public void testInListConstant() throws Exception {
        printTestDescription();

        String query[] = null;
        if (tgtPH()) query = new String[] {
            "SELECT CASE WHEN 'a' IN (null,'a') THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN NOT 'a' IN (null,'b') THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN 'a' IN (null,'b') THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN NOT 'a' IN ('c','b') THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN 1 IN ('foo',2,1) THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
        };
        // TRAF: null is not allowed in predicate ('a'=null).  This is a 4099
        // error. (only col IS NULL is supported)  Remove null from IN () predicates.
        else if (tgtTR()) query = new String[] {
            "SELECT CASE WHEN 'a' IN ('a') THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN NOT 'a' IN ('b') THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN 'a' IN ('b') THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN NOT 'a' IN ('c','b') THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
            "SELECT CASE WHEN 1 IN (2,1) THEN '1' ELSE '2' END FROM BTABLE LIMIT 1",
        };
        else if (tgtSQ()) query = new String[] {
            "SELECT [FIRST 1] CASE WHEN 'a' IN ('a') THEN '1' ELSE '2' END FROM BTABLE",
            "SELECT [FIRST 1] CASE WHEN NOT 'a' IN ('b') THEN '1' ELSE '2' END FROM BTABLE",
            "SELECT [FIRST 1] CASE WHEN 'a' IN ('b') THEN '1' ELSE '2' END FROM BTABLE",
            "SELECT [FIRST 1] CASE WHEN NOT 'a' IN ('c','b') THEN '1' ELSE '2' END FROM BTABLE",
            "SELECT [FIRST 1] CASE WHEN 1 IN (2,1) THEN '1' ELSE '2' END FROM BTABLE",
        };

        String result[] = null;
        if (tgtPH()) result = new String[] {
            "1",
            "2",
            "2",
            "1",
            "1",
        };
        else if (tgtSQ()||tgtTR()) result = new String[] {
            "1",
            "1",
            "2",
            "1",
            "1",
        };

        assertEquals(query.length,result.length);
        try {
            initTableValues();
            for (int i = 0; i < query.length; i++) {
                PreparedStatement statement = conn.prepareStatement(query[i]);
                ResultSet rs = statement.executeQuery();
                assertTrue(rs.next());
                assertEquals(query[i],result[i], rs.getString(1));
                assertFalse(rs.next());
            }
        } finally {
        }
    }

    @Test
    public void testLikeOnColumn() throws Exception {
        printTestDescription();

        createTestTable(PTSDB_NAME);

        PreparedStatement stmt = null;
        if (tgtPH()||tgtTR()) stmt = conn.prepareStatement("upsert into PTSDB VALUES (?, ?, ?, 0.5)");
        else if (tgtSQ()) stmt = conn.prepareStatement("insert into PTSDB VALUES (?, ?, ?, 0.5)");
        if (tgtPH()) stmt.setDate(3, D1);
        else if (tgtSQ()||tgtTR()) stmt.setTimestamp(3, T1);

        stmt.setString(1, "a");
        stmt.setString(2, "a");
        stmt.execute();

        stmt.setString(1, "x");
        stmt.setString(2, "a");
        stmt.execute();

        stmt.setString(1, "xy");
        stmt.setString(2, "b");
        stmt.execute();

        stmt.setString(1, "xyz");
        stmt.setString(2, "c");
        stmt.execute();

        stmt.setString(1, "xyza");
        stmt.setString(2, "d");
        stmt.execute();

        stmt.setString(1, "xyzab");
        stmt.setString(2, "e");
        stmt.execute();

        stmt.setString(1, "z");
        stmt.setString(2, "e");
        stmt.execute();

        PreparedStatement statement = null;
        ResultSet rs;
        try {
            // Test 1
            if (tgtPH()) statement = conn.prepareStatement("SELECT INST FROM PTSDB WHERE INST LIKE 'x%'");
            else if (tgtSQ()||tgtTR()) statement = conn.prepareStatement("SELECT INST FROM PTSDB WHERE INST LIKE 'x%' order by 1");

            rs = statement.executeQuery();

            assertTrue(rs.next());
            assertEquals("x", rs.getString(1));

            assertTrue(rs.next());
            assertEquals("xy", rs.getString(1));

            assertTrue(rs.next());
            assertEquals("xyz", rs.getString(1));

            assertTrue(rs.next());
            assertEquals("xyza", rs.getString(1));

            assertTrue(rs.next());
            assertEquals("xyzab", rs.getString(1));

            assertFalse(rs.next());

            // Test 2
            if (tgtPH()) statement = conn.prepareStatement("SELECT INST FROM PTSDB WHERE INST LIKE 'xy_a%'");
            else if (tgtSQ()||tgtTR()) statement = conn.prepareStatement("SELECT INST FROM PTSDB WHERE INST LIKE 'xy_a%' order by 1");

            rs = statement.executeQuery();

            assertTrue(rs.next());
            assertEquals("xyza", rs.getString(1));

            assertTrue(rs.next());
            assertEquals("xyzab", rs.getString(1));

            assertFalse(rs.next());

            // Test 3
            if (tgtPH()) statement = conn.prepareStatement("SELECT INST FROM PTSDB WHERE INST NOT LIKE 'xy_a%'");
            else if (tgtSQ()||tgtTR()) statement = conn.prepareStatement("SELECT INST FROM PTSDB WHERE INST NOT LIKE 'xy_a%' order by 1");

            rs = statement.executeQuery();

            assertTrue(rs.next());
            assertEquals("a", rs.getString(1));

            assertTrue(rs.next());
            assertEquals("x", rs.getString(1));

            assertTrue(rs.next());
            assertEquals("xy", rs.getString(1));

            assertTrue(rs.next());
            assertEquals("xyz", rs.getString(1));

            assertTrue(rs.next());
            assertEquals("z", rs.getString(1));

            assertFalse(rs.next());

            // Test 4
            statement = conn.prepareStatement("SELECT INST FROM PTSDB WHERE 'xzabc' LIKE 'xy_a%'");
            rs = statement.executeQuery();
            assertFalse(rs.next());

        } finally {
        }
    }

    @Test
    public void testIsNullInPK() throws Exception {
        printTestDescription();

        createTestTable(PTSDB_NAME);

        conn.setAutoCommit(true);
        PreparedStatement stmt = null;
        if (tgtPH()) stmt = conn.prepareStatement("upsert into PTSDB VALUES ('', '', ?, 0.5)");
        else if (tgtTR()) stmt = conn.prepareStatement("upsert into PTSDB VALUES (null, null, ?, 0.5)");
        else if (tgtSQ()) stmt = conn.prepareStatement("insert into PTSDB VALUES (null, null, ?, 0.5)");
        stmt.setDate(1, D1);
        stmt.execute();

        String query = null;
        if (tgtPH()) query = "SELECT HOST,INST,DATE FROM PTSDB WHERE HOST IS NULL AND INST IS NULL AND DATE=?";
        else if (tgtSQ()||tgtTR()) query = "SELECT HOST1,INST,DATE1 FROM PTSDB WHERE HOST1 IS NULL AND INST IS NULL AND DATE1=?";
        try { 
            PreparedStatement statement = conn.prepareStatement(query);
            if (tgtPH()) statement.setDate(1, D1);
            else if (tgtSQ()||tgtTR()) statement.setTimestamp(1, T1);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertNull(rs.getString(1));
            assertNull(rs.getString(2));
            if (tgtPH()) assertEquals(D1, rs.getDate(3));
            else if (tgtSQ()||tgtTR()) assertEquals(T1, rs.getTimestamp(3));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testLengthFunction() throws Exception {
        printTestDescription();

        String query[] = null;
        if (tgtPH()) query = new String[] {
            "SELECT length('') FROM BTABLE LIMIT 1",
            "SELECT length(' ') FROM BTABLE LIMIT 1",
            "SELECT length('1') FROM BTABLE LIMIT 1",
            "SELECT length('1234') FROM BTABLE LIMIT 1",
            "SELECT length('ɚɦɰɸ') FROM BTABLE LIMIT 1",
            "SELECT length('ǢǛǟƈ') FROM BTABLE LIMIT 1",
            "SELECT length('This is a test!') FROM BTABLE LIMIT 1",
            "SELECT A_STRING FROM BTABLE WHERE length(A_STRING)=3",
        };
        else if (tgtTR()) query = new String[] {
            "SELECT char_length('') FROM BTABLE LIMIT 1",
            "SELECT char_length(' ') FROM BTABLE LIMIT 1",
            "SELECT char_length('1') FROM BTABLE LIMIT 1",
            "SELECT char_length('1234') FROM BTABLE LIMIT 1",
            "SELECT char_length('ABCD') FROM BTABLE LIMIT 1",
            "SELECT char_length('!@#$') FROM BTABLE LIMIT 1",
            "SELECT char_length('This is a test!') FROM BTABLE LIMIT 1",
            "SELECT A_STRING FROM BTABLE WHERE char_length(A_STRING)=3",
        };
        else if (tgtSQ()) query = new String[] {
            "SELECT [FIRST 1] char_length('') FROM BTABLE",
            "SELECT [FIRST 1] char_length(' ') FROM BTABLE",
            "SELECT [FIRST 1] char_length('1') FROM BTABLE",
            "SELECT [FIRST 1] char_length('1234') FROM BTABLE",
            "SELECT [FIRST 1] char_length('ABCD') FROM BTABLE",
            "SELECT [FIRST 1] char_length('!@#$') FROM BTABLE",
            "SELECT [FIRST 1] char_length('This is a test!') FROM BTABLE",
            "SELECT A_STRING FROM BTABLE WHERE char_length(A_STRING)=3",
        };

        String result[] = null;
        if (tgtPH()) result = new String[] {
            null,
            "1",
            "1",
            "4",
            "4",
            "4",
            "15",
            "abc",
        };
        else if (tgtSQ()||tgtTR()) result = new String[] {
            "0",
            "1",
            "1",
            "4",
            "4",
            "4",
            "15",
            "abc",
        };
        assertEquals(query.length,result.length);
        try {
            initTableValues();
            for (int i = 0; i < query.length; i++) {
                PreparedStatement statement = conn.prepareStatement(query[i]);
                ResultSet rs = statement.executeQuery();
                assertTrue(rs.next());
                assertEquals(query[i],result[i], rs.getString(1));
                assertFalse(rs.next());
            }
        } finally {
        }
    }

    @Test
    public void testUpperFunction() throws Exception {
        printTestDescription();

        String query[] = null;
        if (tgtPH()) query = new String[] {
                "SELECT upper('abc') FROM BTABLE LIMIT 1",
                "SELECT upper('Abc') FROM BTABLE LIMIT 1",
                "SELECT upper('ABC') FROM BTABLE LIMIT 1",
                "SELECT upper('ĎďĒ') FROM BTABLE LIMIT 1",
                "SELECT upper('ß') FROM BTABLE LIMIT 1",
        };
        else if (tgtTR()) query = new String[] {
                "SELECT upper('abc') FROM BTABLE LIMIT 1",
                "SELECT upper('Abc') FROM BTABLE LIMIT 1",
                "SELECT upper('ABC') FROM BTABLE LIMIT 1",
                // TRAF: test only ASCII for now
                "SELECT upper('123') FROM BTABLE LIMIT 1",
                "SELECT upper('#$+') FROM BTABLE LIMIT 1",
        };
        else if (tgtSQ()) query = new String[] {
                "SELECT [FIRST 1] upper('abc') FROM BTABLE",
                "SELECT [FIRST 1] upper('Abc') FROM BTABLE",
                "SELECT [FIRST 1] upper('ABC') FROM BTABLE",
                // TRAF: test only ASCII for now
                "SELECT [FIRST 1] upper('123') FROM BTABLE",
                "SELECT [FIRST 1] upper('#$+') FROM BTABLE",
        };
        String result[] = null;
        if (tgtPH()) result = new String[] {
                "ABC",
                "ABC",
                "ABC",
                "ĎĎĒ",
                "SS",
        };
        else if (tgtSQ()||tgtTR()) result = new String[] {
                "ABC",
                "ABC",
                "ABC",
                "123",
                "#$+",
        };
        assertEquals(query.length, result.length);
        try {
            initTableValues();
            for (int i = 0; i < query.length; i++) {
                PreparedStatement statement = conn.prepareStatement(query[i]);
                ResultSet rs = statement.executeQuery();
                assertTrue(rs.next());
                assertEquals(query[i],result[i], rs.getString(1));
                assertFalse(rs.next());
            }
        } finally {
        }
    }

    @Test
    public void testLowerFunction() throws Exception {
        printTestDescription();

        String query[] = null;
        if (tgtPH()) query = new String[] {
                "SELECT lower('abc') FROM BTABLE LIMIT 1",
                "SELECT lower('Abc') FROM BTABLE LIMIT 1",
                "SELECT lower('ABC') FROM BTABLE LIMIT 1",
                "SELECT lower('ĎďĒ') FROM BTABLE LIMIT 1",
                "SELECT lower('ß') FROM BTABLE LIMIT 1",
                "SELECT lower('SS') FROM BTABLE LIMIT 1",
        };
        else if (tgtTR()) query = new String[] {
                "SELECT lower('abc') FROM BTABLE LIMIT 1",
                "SELECT lower('Abc') FROM BTABLE LIMIT 1",
                "SELECT lower('ABC') FROM BTABLE LIMIT 1",
                // TRAF: test only ASCII for now
                "SELECT upper('123') FROM BTABLE LIMIT 1",
                "SELECT upper('#$+') FROM BTABLE LIMIT 1",
                "SELECT lower('SS') FROM BTABLE LIMIT 1",
        };
        else if (tgtSQ()) query = new String[] {
                "SELECT [FIRST 1] lower('abc') FROM BTABLE",
                "SELECT [FIRST 1] lower('Abc') FROM BTABLE",
                "SELECT [FIRST 1] lower('ABC') FROM BTABLE",
                // TRAF: test only ASCII for now
                "SELECT [FIRST 1] upper('123') FROM BTABLE",
                "SELECT [FIRST 1] upper('#$+') FROM BTABLE",
                "SELECT [FIRST 1] lower('SS') FROM BTABLE",
        };

        String result[] = null;
        if (tgtPH()) result = new String[] {
                "abc",
                "abc",
                "abc",
                "ďďē",
                "ß",
                "ss",
        };
        else if (tgtSQ()||tgtTR()) result = new String[] {
                "abc",
                "abc",
                "abc",
                "123",
                "#$+",
                "ss",
        };

        assertEquals(query.length, result.length);
        try {
            initTableValues();
            for (int i = 0; i < query.length; i++) {
                PreparedStatement statement = conn.prepareStatement(query[i]);
                ResultSet rs = statement.executeQuery();
                assertTrue(rs.next());
                assertEquals(query[i],result[i], rs.getString(1));
                assertFalse(rs.next());
            }
        } finally {
        }
    }

    @Test
    public void testRTrimFunction() throws Exception {
        printTestDescription();

        String query[] = null;
        if (tgtPH()) query = new String[] {
            "SELECT rtrim('') FROM BTABLE LIMIT 1",
            "SELECT rtrim(' ') FROM BTABLE LIMIT 1",
            "SELECT rtrim('   ') FROM BTABLE LIMIT 1",
            "SELECT rtrim('abc') FROM BTABLE LIMIT 1",
            "SELECT rtrim('abc   ') FROM BTABLE LIMIT 1",
            "SELECT rtrim('abc   def') FROM BTABLE LIMIT 1",
            "SELECT rtrim('abc   def   ') FROM BTABLE LIMIT 1",
            "SELECT rtrim('ĎďĒ   ') FROM BTABLE LIMIT 1",
            "SELECT pk FROM VarcharKeyTest WHERE rtrim(pk)='jkl' LIMIT 1",
        };
        else if (tgtTR()) query = new String[] {
            "SELECT rtrim('') FROM BTABLE LIMIT 1",
            "SELECT rtrim(' ') FROM BTABLE LIMIT 1",
            "SELECT rtrim('   ') FROM BTABLE LIMIT 1",
            "SELECT rtrim('abc') FROM BTABLE LIMIT 1",
            "SELECT rtrim('abc   ') FROM BTABLE LIMIT 1",
            "SELECT rtrim('abc   def') FROM BTABLE LIMIT 1",
            "SELECT rtrim('abc   def   ') FROM BTABLE LIMIT 1",
            // TRAF: Test only ASCII for now
            "SELECT rtrim('12 3   ') FROM BTABLE LIMIT 1",
            "SELECT pk FROM VarcharKeyTest WHERE rtrim(pk)='jkl'",
        };
        else if (tgtSQ()) query = new String[] {
            "SELECT [FIRST 1] rtrim('') FROM BTABLE",
            "SELECT [FIRST 1] rtrim(' ') FROM BTABLE",
            "SELECT [FIRST 1] rtrim('   ') FROM BTABLE",
            "SELECT [FIRST 1] rtrim('abc') FROM BTABLE",
            "SELECT [FIRST 1] rtrim('abc   ') FROM BTABLE",
            "SELECT [FIRST 1] rtrim('abc   def') FROM BTABLE",
            "SELECT [FIRST 1] rtrim('abc   def   ') FROM BTABLE",
            // TRAF: Test only ASCII for now
            "SELECT [FIRST 1] rtrim('12 3   ') FROM BTABLE",
            "SELECT [FIRST 1] pk FROM VarcharKeyTest WHERE rtrim(pk)='jkl'",
        };

        String result[] = null;
        if (tgtPH()) result = new String[] {
            null,
            null,
            null,
            "abc",
            "abc",
            "abc   def",
            "abc   def",
            "ĎďĒ",
            "jkl   ",
        };
        else if (tgtSQ()||tgtTR()) result = new String[] {
            "",
            "",
            "",
            "abc",
            "abc",
            "abc   def",
            "abc   def",
            "12 3",
            "jkl   ",
        };

        assertEquals(query.length, result.length);
        try {
            initTableValues();
            for (int i = 0; i < query.length; i++) {
                PreparedStatement statement = conn.prepareStatement(query[i]);
                ResultSet rs = statement.executeQuery();
                assertTrue(rs.next());
                assertEquals(query[i],result[i], rs.getString(1));
                assertFalse(rs.next());
            }
        } finally {
        }
    }

    @Test
    public void testLTrimFunction() throws Exception {
        printTestDescription();

        String query[] = null;
        if (tgtPH()) query = new String[] {
            "SELECT ltrim('') FROM BTABLE LIMIT 1",
            "SELECT ltrim(' ') FROM BTABLE LIMIT 1",
            "SELECT ltrim('   ') FROM BTABLE LIMIT 1",
            "SELECT ltrim('abc') FROM BTABLE LIMIT 1",
            "SELECT ltrim('   abc') FROM BTABLE LIMIT 1",
            "SELECT ltrim('abc   def') FROM BTABLE LIMIT 1",
            "SELECT ltrim('   abc   def') FROM BTABLE LIMIT 1",
            "SELECT ltrim('   ĎďĒ') FROM BTABLE LIMIT 1",
            "SELECT pk FROM VarcharKeyTest WHERE ltrim(pk)='def' LIMIT 1",
        };
        else if (tgtTR()) query = new String[] {
            "SELECT ltrim('') FROM BTABLE LIMIT 1",
            "SELECT ltrim(' ') FROM BTABLE LIMIT 1",
            "SELECT ltrim('   ') FROM BTABLE LIMIT 1",
            "SELECT ltrim('abc') FROM BTABLE LIMIT 1",
            "SELECT ltrim('   abc') FROM BTABLE LIMIT 1",
            "SELECT ltrim('abc   def') FROM BTABLE LIMIT 1",
            "SELECT ltrim('   abc   def') FROM BTABLE LIMIT 1",
            // TRAF: Test ASCII only for now.
            "SELECT ltrim('   12 3') FROM BTABLE LIMIT 1",
            "SELECT pk FROM VarcharKeyTest WHERE ltrim(pk)='def' LIMIT 1",
        };
        else if (tgtSQ()) query = new String[] {
            "SELECT [FIRST 1] ltrim('') FROM BTABLE",
            "SELECT [FIRST 1] ltrim(' ') FROM BTABLE",
            "SELECT [FIRST 1] ltrim('   ') FROM BTABLE",
            "SELECT [FIRST 1] ltrim('abc') FROM BTABLE",
            "SELECT [FIRST 1] ltrim('   abc') FROM BTABLE",
            "SELECT [FIRST 1] ltrim('abc   def') FROM BTABLE",
            "SELECT [FIRST 1] ltrim('   abc   def') FROM BTABLE",
            // TRAF: Test ASCII only for now.
            "SELECT [FIRST 1] ltrim('   12 3') FROM BTABLE",
            "SELECT [FIRST 1] pk FROM VarcharKeyTest WHERE ltrim(pk)='def'",
        };

        String result[] = null;
        if (tgtPH()) result = new String[] {
            null,
            null,
            null,
            "abc",
            "abc",
            "abc   def",
            "abc   def",
            "ĎďĒ",
            "   def",
        };
        else if (tgtSQ()||tgtTR()) result = new String[] {
            "",
            "",
            "",
            "abc",
            "abc",
            "abc   def",
            "abc   def",
            "12 3",
            "   def",
        };

        assertEquals(query.length, result.length);
        try {
            initTableValues();
            for (int i = 0; i < query.length; i++) {
                PreparedStatement statement = conn.prepareStatement(query[i]);
                ResultSet rs = statement.executeQuery();
                assertTrue(rs.next());
                assertEquals(query[i],result[i], rs.getString(1));
                assertFalse(rs.next());
            }
        } finally {
        }
    }

    @Test
    public void testSubstrFunctionOnRowKeyInWhere() throws Exception {
        printTestDescription();

        if (tgtPH()) conn.createStatement().execute("CREATE TABLE substr_test (s1 varchar not null, s2 varchar not null constraint pk primary key(s1,s2))");
        else if (tgtSQ()||tgtTR()) conn.createStatement().execute("CREATE TABLE substr_test (s1 varchar(128) not null, s2 varchar(128) not null, constraint pk_substr_test primary key(s1,s2))");

        if (tgtPH()||tgtTR()) {
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abc','a')");
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abcd','b')");
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abce','c')");
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abcde','d')");
        } else if (tgtSQ()) {
            conn.createStatement().execute("INSERT INTO substr_test VALUES('abc','a')");
            conn.createStatement().execute("INSERT INTO substr_test VALUES('abcd','b')");
            conn.createStatement().execute("INSERT INTO substr_test VALUES('abce','c')");
            conn.createStatement().execute("INSERT INTO substr_test VALUES('abcde','d')");
        }
        ResultSet rs = null;
        if (tgtPH()) rs = conn.createStatement().executeQuery("SELECT s1 from substr_test where substr(s1,1,4) = 'abcd'");
        else if (tgtSQ()||tgtTR()) rs = conn.createStatement().executeQuery("SELECT s1 from substr_test where substr(s1,1,4) = 'abcd' order by 1");
        assertTrue(rs.next());
        assertEquals("abcd",rs.getString(1));
        assertTrue(rs.next());
        assertEquals("abcde",rs.getString(1));
        assertFalse(rs.next());
    }

    @Test
    public void testRTrimFunctionOnRowKeyInWhere() throws Exception {
        printTestDescription();

        if (tgtPH()) conn.createStatement().execute("CREATE TABLE substr_test (s1 varchar not null, s2 varchar not null constraint pk primary key(s1,s2))");
        else if (tgtSQ()||tgtTR()) conn.createStatement().execute("CREATE TABLE substr_test (s1 varchar(128) not null, s2 varchar(128) not null, constraint pk_substr_test primary key(s1,s2))");
      
        if (tgtPH()) {
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abc','a')");
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abcd','b')");
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abcd ','c')");
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abcd  ','c')");
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abcd  a','c')"); // Need TRAVERSE_AND_LEAVE for cases like this
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abcde','d')");
        } else if (tgtTR()) {
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abc','a')");
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abcd','b')");
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abcd ','c')");
            // TRAF: For SQ & TR, it uses 'PAD SPACE' implementation, 'abcd ' 
            // and 'abcd  ' are treated as the same.  It has been decided that
            // this behavior will not change.  Changed the following 'c' to 'd'
            // to make the test work. 
            // conn.createStatement().execute("UPSERT INTO substr_test VALUES('abcd  ','c')");
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abcd  ','d')");
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abcd  a','e')"); // Need TRAVERSE_AND_LEAVE for cases like this
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abcde','f')");
        } else if (tgtSQ()) {
            conn.createStatement().execute("INSERT INTO substr_test VALUES('abc','a')");
            conn.createStatement().execute("INSERT INTO substr_test VALUES('abcd','b')");
            conn.createStatement().execute("INSERT INTO substr_test VALUES('abcd ','c')");
            // TRAF: For SQ & TR, it uses 'PAD SPACE' implementation, 'abcd '
            // and 'abcd  ' are treated as the same.  SQ will see 8102 unique 
            // constraint error in this case. It has been decided that
            // this behavior will not change.  Changed the following 'c' to 'd'
            // to make the test work.
            // conn.createStatement().execute("INSERT INTO substr_test VALUES('abcd  ','c')");
            conn.createStatement().execute("INSERT INTO substr_test VALUES('abcd  ','d')");
            conn.createStatement().execute("INSERT INTO substr_test VALUES('abcd  a','e')"); // Need TRAVERSE_AND_LEAVE for cases like this
            conn.createStatement().execute("INSERT INTO substr_test VALUES('abcde','f')");
        }

        ResultSet rs = null;
        if (tgtPH()) rs = conn.createStatement().executeQuery("SELECT s1 from substr_test where rtrim(s1) = 'abcd'");
        else if (tgtSQ()||tgtTR()) rs = conn.createStatement().executeQuery("SELECT s1 from substr_test where rtrim(s1) = 'abcd' order by s2");
       
        assertTrue(rs.next());
        assertEquals("abcd",rs.getString(1));
        assertTrue(rs.next());
        assertEquals("abcd ",rs.getString(1));
        assertTrue(rs.next());
        assertEquals("abcd  ",rs.getString(1));
        assertFalse(rs.next());
    }

    @Test
    public void testLikeFunctionOnRowKeyInWhere() throws Exception {
        printTestDescription();

        if (tgtPH()) conn.createStatement().execute("CREATE TABLE substr_test (s1 varchar not null, s2 varchar not null constraint pk primary key(s1,s2))");
        else if (tgtSQ()||tgtTR()) conn.createStatement().execute("CREATE TABLE substr_test (s1 varchar(128) not null, s2 varchar(128) not null, constraint pk primary key(s1,s2))");

        if (tgtPH()||tgtTR()) {
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abc','a')");
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abcd','b')");
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abcd-','c')");
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abcd-1','c')");
            conn.createStatement().execute("UPSERT INTO substr_test VALUES('abce','d')");
        } else if (tgtSQ()) {
            conn.createStatement().execute("INSERT INTO substr_test VALUES('abc','a')");
            conn.createStatement().execute("INSERT INTO substr_test VALUES('abcd','b')");
            conn.createStatement().execute("INSERT INTO substr_test VALUES('abcd-','c')");
            conn.createStatement().execute("INSERT INTO substr_test VALUES('abcd-1','c')");
            conn.createStatement().execute("INSERT INTO substr_test VALUES('abce','d')");
        }

        ResultSet rs = conn.createStatement().executeQuery("SELECT s1 from substr_test where s1 like 'abcd%1'");
        assertTrue(rs.next());
        assertEquals("abcd-1",rs.getString(1));
        assertFalse(rs.next());
    }

    @Test
    public void testTrimFunction() throws Exception {
        printTestDescription();

        String query[] = null;
        if (tgtPH()) query = new String[] {
            "SELECT trim('') FROM BTABLE LIMIT 1",
            "SELECT trim(' ') FROM BTABLE LIMIT 1",
            "SELECT trim('   ') FROM BTABLE LIMIT 1",
            "SELECT trim('abc') FROM BTABLE LIMIT 1",
            "SELECT trim('   abc') FROM BTABLE LIMIT 1",
            "SELECT trim('abc   ') FROM BTABLE LIMIT 1",
            "SELECT trim('abc   def') FROM BTABLE LIMIT 1",
            "SELECT trim('   abc   def') FROM BTABLE LIMIT 1",
            "SELECT trim('abc   def   ') FROM BTABLE LIMIT 1",
            "SELECT trim('   abc   def   ') FROM BTABLE LIMIT 1",
            "SELECT trim('   ĎďĒ   ') FROM BTABLE LIMIT 1",
            "SELECT pk FROM VarcharKeyTest WHERE trim(pk)='ghi'",
        };
        else if (tgtTR()) query = new String[] {
            "SELECT trim('') FROM BTABLE LIMIT 1",
            "SELECT trim(' ') FROM BTABLE LIMIT 1",
            "SELECT trim('   ') FROM BTABLE LIMIT 1",
            "SELECT trim('abc') FROM BTABLE LIMIT 1",
            "SELECT trim('   abc') FROM BTABLE LIMIT 1",
            "SELECT trim('abc   ') FROM BTABLE LIMIT 1",
            "SELECT trim('abc   def') FROM BTABLE LIMIT 1",
            "SELECT trim('   abc   def') FROM BTABLE LIMIT 1",
            "SELECT trim('abc   def   ') FROM BTABLE LIMIT 1",
            "SELECT trim('   abc   def   ') FROM BTABLE LIMIT 1",
            // TRAF: Test only ASCII for now.
            "SELECT trim('   12 3        ') FROM BTABLE LIMIT 1",
            "SELECT pk FROM VarcharKeyTest WHERE trim(pk)='ghi'",
        };
        else if (tgtSQ()) query = new String[] {
            "SELECT [FIRST 1] trim('') FROM BTABLE",
            "SELECT [FIRST 1] trim(' ') FROM BTABLE",
            "SELECT [FIRST 1] trim('   ') FROM BTABLE",
            "SELECT [FIRST 1] trim('abc') FROM BTABLE",
            "SELECT [FIRST 1] trim('   abc') FROM BTABLE",
            "SELECT [FIRST 1] trim('abc   ') FROM BTABLE",
            "SELECT [FIRST 1] trim('abc   def') FROM BTABLE",
            "SELECT [FIRST 1] trim('   abc   def') FROM BTABLE",
            "SELECT [FIRST 1] trim('abc   def   ') FROM BTABLE",
            "SELECT [FIRST 1] trim('   abc   def   ') FROM BTABLE",
            // TRAF: Test only ASCII for now.
            "SELECT [FIRST 1] trim('   12 3        ') FROM BTABLE",
            "SELECT pk FROM VarcharKeyTest WHERE trim(pk)='ghi'",
        };

        String result[] = null;
        if (tgtPH()) result = new String[] {
            null,
            null,
            null,
            "abc",
            "abc",
            "abc",
            "abc   def",
            "abc   def",
            "abc   def",
            "abc   def",
            "ĎďĒ",
            "   ghi   ",
        };
        else if (tgtSQ()||tgtTR()) result = new String[] {
            "",
            "",
            "",
            "abc",
            "abc",
            "abc",
            "abc   def",
            "abc   def",
            "abc   def",
            "abc   def",
            "12 3",
            "   ghi   ",
        };

        assertEquals(query.length, result.length);
        try {
            initTableValues();
            for (int i = 0; i < query.length; i++) {
                PreparedStatement statement = conn.prepareStatement(query[i]);
                ResultSet rs = statement.executeQuery();
                assertTrue(rs.next());
                assertEquals(query[i],result[i], rs.getString(1));
                assertFalse(rs.next());
            }
        } finally {
        }
    }
}
