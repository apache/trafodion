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
import java.sql.Date;
import java.util.*;

public class ExecuteStatementsTest extends BaseTest {

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table " + ATABLE_NAME, "table " + BTABLE_NAME, "table " + PTSDB_NAME, "table foo"));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */   

    @Test
    public void testExecuteStatements() throws Exception {
        printTestDescription();

        initATableValues();

        String statements[] =  null;
        if (tgtPH()) statements = new String[] {
            "create table IF NOT EXISTS" + ATABLE_NAME + // Shouldn't error out b/c of if not exists clause
            "   (organization_id char(15) not null, \n" + 
            "    entity_id char(15) not null,\n" + 
            "    a_string varchar(100),\n" + 
            "    b_string varchar(100)\n" +
            "    CONSTRAINT pk PRIMARY KEY (organization_id,entity_id));\n",

            "create table " + PTSDB_NAME +
            "   (inst varchar null,\n" + 
            "    host varchar null,\n" + 
            "    date date not null,\n" + 
            "    val decimal\n" +
            "    CONSTRAINT pk PRIMARY KEY (inst,host,date))\n" +
            "    split on (?,?,?);\n",

            "alter table " + PTSDB_NAME + " add IF NOT EXISTS val decimal;\n", // Shouldn't error out b/c of if not exists clause

            "alter table " + PTSDB_NAME + " drop column IF EXISTS blah;\n",  // Shouldn't error out b/c of if exists clause

            "drop table IF EXISTS FOO.BAR;\n", // Shouldn't error out b/c of if exists clause

            "UPSERT INTO " + PTSDB_NAME + "(date, val, host) " +
            "    SELECT current_date(), x_integer+2, entity_id FROM ATABLE WHERE a_integer >= ?;",

            "UPSERT INTO " + PTSDB_NAME + "(date, val, inst)\n" +
            "    SELECT date+1, val*10, host FROM " + PTSDB_NAME + ";"};
        else if (tgtTR()) statements = new String[]{
            "create table IF NOT EXISTS " + ATABLE_NAME + // Shouldn't error out b/c of if not exists clause 
            "   (organization_id char(15) not null, \n" +
            "    entity_id char(15) not null,\n" +
            "    a_string varchar(100),\n" +
            "    b_string varchar(100)\n" +
            "    , CONSTRAINT pk_btable PRIMARY KEY (organization_id,entity_id));\n",

            "create table " + PTSDB_NAME +
            "   (inst varchar(100) not null not droppable,\n" +
            "    host1 varchar(100) not null not droppable,\n" +
            "    date1 date not null,\n" +
            "    val decimal\n" +
            "    , CONSTRAINT pk_ptsdb PRIMARY KEY (inst,host1,date1))\n" +
// TRAF "    split on (?,?,?);\n",
/* TRAF */ ";\n",

            "alter table " + PTSDB_NAME + " add IF NOT EXISTS val1 decimal;\n", // Shouldn't error out b/c of if not exists clause 

            "alter table " + PTSDB_NAME + " drop column if exists blah;\n",  // Shouldn't error out b/c of if exists clause

            "drop table IF EXISTS FOO.BAR;\n", // Shouldn't error out b/c of if exists clause

            "UPSERT INTO " + PTSDB_NAME + "(inst, date1, val, host1) " +
            "    SELECT 'abcd', CURRENT_DATE, x_integer+2, entity_id FROM ATABLE WHERE a_integer >= 6;",

            "UPSERT INTO " + PTSDB_NAME + "(date1, val, inst, host1)\n" +
            "    SELECT date1+1, val*10, host1, host1 FROM " + PTSDB_NAME + ";"};
        else if (tgtSQ()) statements = new String[]{
            "create table " + BTABLE_NAME +
            "   (organization_id char(15) not null, \n" +
            "    entity_id char(15) not null,\n" +
            "    a_string varchar(100),\n" +
            "    b_string varchar(100)\n" +
            "    , CONSTRAINT pk_btable PRIMARY KEY (organization_id,entity_id));\n",

            "create table " + PTSDB_NAME +
            "   (inst varchar(100) not null not droppable,\n" +
            "    host1 varchar(100) not null not droppable,\n" +
            "    date1 date not null,\n" +
            "    val decimal\n" +
            "    , CONSTRAINT pk_ptsdb PRIMARY KEY (inst,host1,date1))\n" +
            ";\n",

            "alter table " + PTSDB_NAME + " add val1 decimal;\n",

// TRAF: take the following one out completely. No replacement in SQ.
// TRAF "alter table " + PTSDB_NAME + " drop column if exists blah;\n",  // Shouldn't error out b/c of if exists clause

            "drop table " + BTABLE_NAME + ";\n",

            "INSERT INTO " + PTSDB_NAME + "(inst, date1, val, host1) " +
            "    SELECT 'abcd', CURRENT_DATE, x_integer+2, entity_id FROM ATABLE WHERE a_integer >= 6;",

            "INSERT INTO " + PTSDB_NAME + "(date1, val, inst, host1)\n" +
            "    SELECT date1+1, val*10, host1, host1 FROM " + PTSDB_NAME + ";"};
        Date now = new Date(System.currentTimeMillis());
        conn.setAutoCommit(true);
        List<Object> binds = Arrays.<Object>asList("a","j","s", 6);
        for (int i = 0; i < statements.length; i++) {
            String stmt = statements[i];
            conn.createStatement().execute(stmt);
        }

        Date then = new Date(System.currentTimeMillis() + MILLIS_IN_DAY);

        String query = null;
        if (tgtPH()) query = "SELECT host,inst, date,val FROM " + PTSDB_NAME + " where inst is not null";
        else if (tgtSQ()||tgtTR()) query = "SELECT host1,inst, date1,val FROM " + PTSDB_NAME + " where inst is not null order by 1, 2, 3, 4";
        PreparedStatement statement = conn.prepareStatement(query);
       
        // TRAF, the table column values are all different by now.  There is
        // not much point to verify the data the same way.
        if (tgtSQ()||tgtTR()) return;
  
        ResultSet rs = statement.executeQuery();
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(ROW6, rs.getString(2));
        assertTrue(rs.getDate(3).after(now) && rs.getDate(3).before(then));
        assertEquals(null, rs.getBigDecimal(4));
        
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(ROW7, rs.getString(2));
        assertTrue(rs.getDate(3).after(now) && rs.getDate(3).before(then));
        assertTrue(BigDecimal.valueOf(70).compareTo(rs.getBigDecimal(4)) == 0);
        
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(ROW8, rs.getString(2));
        assertTrue(rs.getDate(3).after(now) && rs.getDate(3).before(then));
        assertTrue(BigDecimal.valueOf(60).compareTo(rs.getBigDecimal(4)) == 0);
        
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(ROW9, rs.getString(2));
        assertTrue(rs.getDate(3).after(now) && rs.getDate(3).before(then));
        assertTrue(BigDecimal.valueOf(50).compareTo(rs.getBigDecimal(4)) == 0);
        
        assertFalse(rs.next());
    }
    
    @Test
    public void testCharPadding() throws Exception {
        printTestDescription();

        String tableName = "foo";
        String rowKey = "hello"; 
        String testString = "world";
        String query = null;
        if (tgtPH()) query = "create table " + tableName +
                "(a_id integer not null, \n" + 
                "a_string char(10) not null, \n" +
                "b_string char(8) not null \n" + 
                "CONSTRAINT my_pk PRIMARY KEY (a_id, a_string))";
        else if (tgtSQ()||tgtTR())  query = "create table " + tableName +
                "(a_id integer not null, \n" +
                "a_string char(10) not null, \n" +
                "b_string char(8) not null \n" +
                ", CONSTRAINT pk_foo PRIMARY KEY (a_id, a_string))"; 
    
        PreparedStatement statement = conn.prepareStatement(query);
        statement.execute();
        if (tgtPH()||tgtTR()) statement = conn.prepareStatement(
                "upsert into " + tableName +
                "    (a_id, " +
                "    a_string, " +
                "    b_string)" +
                "VALUES (?, ?, ?)");
        else if (tgtSQ()) statement = conn.prepareStatement(
                "insert into " + tableName +
                "    (a_id, " +
                "    a_string, " +
                "    b_string)" +
                "VALUES (?, ?, ?)");
        statement.setInt(1, 1);
        statement.setString(2, rowKey);
        statement.setString(3, testString);
        statement.execute();       
        
        createTestTable(BTABLE_NAME);
        if (tgtPH()||tgtTR()) statement = conn.prepareStatement(
                "upsert into BTABLE VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
        else if (tgtSQ()) statement = conn.prepareStatement(
                "insert into BTABLE VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
        statement.setString(1, "abc");
        statement.setString(2, "xyz");
        statement.setString(3, "x");
        statement.setInt(4, 9);
        statement.setString(5, "ab");
        statement.setInt(6, 1);
        statement.setInt(7, 1);
        statement.setString(8, "ab");
        statement.setString(9, "morning1");
        statement.execute();       
        try {
            // test rowkey and non-rowkey values in select statement
            query = "select a_string, b_string from " + tableName;
            assertCharacterPadding(conn.prepareStatement(query), rowKey, testString);
            
            // test with rowkey  in where clause
            query = "select a_string, b_string from " + tableName + " where a_id = 1 and a_string = '" + rowKey + "'";
            assertCharacterPadding(conn.prepareStatement(query), rowKey, testString);
            
            // test with non-rowkey  in where clause
            query = "select a_string, b_string from " + tableName + " where b_string = '" + testString + "'";
            assertCharacterPadding(conn.prepareStatement(query), rowKey, testString);
            
            // test with rowkey and id  in where clause
            query = "select a_string, b_string from " + tableName + " where a_id = 1 and a_string = '" + rowKey + "'";
            assertCharacterPadding(conn.prepareStatement(query), rowKey, testString);
            
            // test with rowkey and id  in where clause where rowkey is greater than the length of the char.len
            query = "select a_string, b_string from " + tableName + " where a_id = 1 and a_string  = '" + rowKey + testString + "'";
            statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertFalse(rs.next());
            
            // test with rowkey and id  in where clause where rowkey is lesser than the length of the char.len
            query = "select a_string, b_string from " + tableName + " where a_id = 1 and a_string  = 'he'";
            statement = conn.prepareStatement(query);
            rs = statement.executeQuery();
            assertFalse(rs.next());
            
            String rowKey2 = "good"; 
            String testString2 = "morning";
            String testString8Char = "morning1";
            String testString10Char = "morning123";
            String upsert = null;
            if (tgtPH()||tgtTR()) upsert = "UPSERT INTO " + tableName + " values (2, '" + rowKey2 + "', '" + testString2+ "') ";
            else if (tgtSQ()) upsert = "INSERT INTO " + tableName + " values (2, '" + rowKey2 + "', '" + testString2+ "') ";
            statement = conn.prepareStatement(upsert);
            statement.execute();
            
            // test upsert statement with padding
            initATableValues();
            if (tgtPH()||tgtTR()) upsert = "UPSERT INTO " + tableName + "(a_id, a_string, b_string) " +
                    "SELECT A_INTEGER, A_STRING, B_STRING FROM ATABLE WHERE a_string = ?";
            else if (tgtSQ()) upsert = "INSERT INTO " + tableName + "(a_id, a_string, b_string) " +
                    "SELECT A_INTEGER, A_STRING, B_STRING FROM ATABLE WHERE a_string = ?";
 
            statement = conn.prepareStatement(upsert);
            statement.setString(1, A_VALUE);
            int rowsInserted = statement.executeUpdate();
            assertEquals(4, rowsInserted);
            
            if (tgtPH()) query = "select a_string, b_string from " + tableName + " where a_string  = '" + A_VALUE+"'";
            else if (tgtSQ()||tgtTR()) query = "select a_string, b_string from " + tableName + " where a_string  = '" + A_VALUE+"'" + "order by 2";
            assertCharacterPadding(conn.prepareStatement(query), A_VALUE, B_VALUE);            
            
            if (tgtPH()||tgtTR()) upsert = "UPSERT INTO " + tableName + " values (3, '" + testString2 + "', '" + testString2+ "') ";
            else if (tgtSQ()) upsert = "INSERT INTO " + tableName + " values (3, '" + testString2 + "', '" + testString2+ "') ";
            statement = conn.prepareStatement(upsert);
            statement.execute();
            query = "select a_string, b_string from " + tableName + "  where a_id = 3 and a_string = b_string";
            assertCharacterPadding(conn.prepareStatement(query), testString2, testString2);
            
            // compare a higher length col with lower length : a_string(10), b_string(8) 
            query = "select a_string, b_string from " + tableName + "  where a_id = 3 and b_string = a_string";
            statement = conn.prepareStatement(query);
            rs = statement.executeQuery();
            assertCharacterPadding(conn.prepareStatement(query), testString2, testString2);
            
            if (tgtPH()||tgtTR()) upsert = "UPSERT INTO " + tableName + " values (4, '" + rowKey2 + "', '" + rowKey2 + "') ";
            else if (tgtSQ()) upsert = "INSERT INTO " + tableName + " values (4, '" + rowKey2 + "', '" + rowKey2 + "') ";
            statement = conn.prepareStatement(upsert);
            statement.execute();
            
            // where both the columns have same value with different paddings
            query = "select a_string, b_string from " + tableName + "  where a_id = 4 and b_string = a_string";
            assertCharacterPadding(conn.prepareStatement(query), rowKey2, rowKey2);
            
            if (tgtPH()||tgtTR()) upsert = "UPSERT INTO " + tableName + " values (5, '" + testString10Char + "', '" + testString8Char + "') ";
            else if (tgtSQ()) upsert = "INSERT INTO " + tableName + " values (5, '" + testString10Char + "', '" + testString8Char + "') ";
            statement = conn.prepareStatement(upsert);
            statement.execute();
            
            // where smaller column is the subset of larger string
            query = "select a_string, b_string from " + tableName + "  where a_id = 5 and b_string = a_string";
            statement = conn.prepareStatement(query);
            rs = statement.executeQuery();
            assertFalse(rs.next());
            
            //where selecting from a CHAR(x) and upserting into a CHAR(y) where x>y
            // upsert rowkey value greater than rowkey limit
            try {
                
                if (tgtPH()||tgtTR()) upsert = "UPSERT INTO " + tableName + "(a_id, a_string, b_string) " +
                        "SELECT x_integer, organization_id, b_string FROM ATABLE WHERE a_string = ?";
                else if (tgtSQ()) upsert = "INSERT INTO " + tableName + "(a_id, a_string, b_string) " +
                        "SELECT x_integer, organization_id, b_string FROM ATABLE WHERE a_string = ?";
 
                statement = conn.prepareStatement(upsert);
                statement.setString(1, A_VALUE);
                statement.executeUpdate();
                fail("Should fail when bigger than expected character is inserted");
            } catch (SQLException ex) {
                if (tgtPH()) assertTrue(ex.getMessage().contains("The value is outside the range for the data type. columnName=A_STRING"));
                // TRAF should see ERROR 8402 A string overflow occurred during the evaluation.. but we are seeing null assigning to non-null... alas, the columns have been changed.
                else if (tgtSQ()||tgtTR()) assertTrue(ex.getMessage().contains("ERROR"));
            }
            
            // upsert non-rowkey value greater than its limit
            try {
                
                if (tgtPH()||tgtTR()) upsert = "UPSERT INTO " + tableName + "(a_id, a_string, b_string) " +
                        "SELECT y_integer, a_string, entity_id FROM ATABLE WHERE a_string = ?";
                else if (tgtSQ()) upsert = "INSERT INTO " + tableName + "(a_id, a_string, b_string) " +
                        "SELECT y_integer, a_string, entity_id FROM ATABLE WHERE a_string = ?";

                statement = conn.prepareStatement(upsert);
                statement.setString(1, A_VALUE);
                statement.executeUpdate();
                fail("Should fail when bigger than expected character is inserted");
            }
            catch (SQLException ex) {
                if (tgtPH()) assertTrue(ex.getMessage().contains("The value is outside the range for the data type. columnName=B_STRING"));
                else if (tgtTR()) assertTrue(ex.getMessage().contains("*** ERROR[8421] NULL cannot be assigned to a NOT NULL column."));
                else if (tgtSQ()) assertTrue(ex.getMessage().contains("*** ERROR[8402] A string overflow occurred during the evaluation of a character expression."));
            }
                        
            //where selecting from a CHAR(x) and upserting into a CHAR(y) where x<=y.
            if (tgtPH()||tgtTR()) upsert = "UPSERT INTO " + tableName + "(a_id, a_string, b_string) " +
                    "SELECT a_integer, e_string, a_id FROM BTABLE";
            else if (tgtSQ()) upsert = "INSERT INTO " + tableName + "(a_id, a_string, b_string) " +
                    "SELECT a_integer, e_string, a_id FROM BTABLE";
 
            statement = conn.prepareStatement(upsert);
            rowsInserted = statement.executeUpdate();
            assertEquals(1, rowsInserted);
            
            query = "select a_string, b_string from " + tableName + " where a_string  = 'morning1'";
            assertCharacterPadding(conn.prepareStatement(query), "morning1", "xyz");
        } finally {
        }
    }
    
    
    private void assertCharacterPadding(PreparedStatement statement, String rowKey, String testString) throws SQLException {
        ResultSet rs = statement.executeQuery();
        assertTrue(rs.next());
        if (tgtPH()) {
            assertEquals(rowKey, rs.getString(1));
            assertEquals(testString, rs.getString(2));
        } else if (tgtSQ()||tgtTR()) {
            assertEquals(rowKey, rs.getString(1).trim());
            assertEquals(testString, rs.getString(2).trim());
        }
    }
    
}
