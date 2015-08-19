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
import java.text.*;

/**
 * Tests for the TO_NUMBER built-in function.
 * 
 * @see ToNumberFunction
 * @author elevine
 * @since 0.1
 */
public class ToNumberFunctionTest extends BaseTest {
    
    public static final String TO_NUMBER_TABLE_NAME = "TO_NUMBER_TABLE";

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table " + TO_NUMBER_TABLE_NAME));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */
    
    private Date row1Date;
    private Date row2Date;
    private Date row3Date;
    private Time row1Time;
    private Time row2Time;
    private Time row3Time;
    private Timestamp row1Timestamp;
    private Timestamp row2Timestamp;
    private Timestamp row3Timestamp;
    
    void initTable() throws Exception {

        String TO_NUMBER_TABLE_DDL = null;
        if (tgtPH()) TO_NUMBER_TABLE_DDL = "create table " + TO_NUMBER_TABLE_NAME +
            "(a_id integer not null, \n" +
            "a_string char(4) not null, \n" +
            "b_string char(4) not null, \n" +
            "a_date date not null, \n" +
            "a_time date not null, \n" +
            "a_timestamp timestamp not null \n" +
            "CONSTRAINT my_pk PRIMARY KEY (a_id, a_string))";
        else if (tgtSQ()||tgtTR()) TO_NUMBER_TABLE_DDL = "create table " + TO_NUMBER_TABLE_NAME +
            "(a_id integer not null, \n" +
            "a_string char(4) not null, \n" +
            "b_string char(4) not null, \n" +
            "a_date date not null, \n" +
            "a_time time not null, \n" +
            "a_timestamp timestamp not null \n" +
            ", CONSTRAINT pk_tonumber PRIMARY KEY (a_id, a_string))";

        conn.createStatement().execute(TO_NUMBER_TABLE_DDL);
        conn.setAutoCommit(false);
        
        PreparedStatement stmt = null;
        if (tgtPH()||tgtTR()) stmt = conn.prepareStatement(
                "upsert into " + TO_NUMBER_TABLE_NAME +
                "    (a_id, " +
                "    a_string," +
                "    b_string," +
                "    a_date," +
                "    a_time," +
                "    a_timestamp)" +
                "VALUES (?, ?, ?, ?, ?, ?)");
        else if (tgtSQ()) stmt = conn.prepareStatement(
                "insert into " + TO_NUMBER_TABLE_NAME +
                "    (a_id, " +
                "    a_string," +
                "    b_string," +
                "    a_date," +
                "    a_time," +
                "    a_timestamp)" +
                "VALUES (?, ?, ?, ?, ?, ?)");
 
        stmt.setInt(1, 1);
        stmt.setString(2, "   1");
        stmt.setString(3, "   1");
        row1Date = new Date(System.currentTimeMillis() - 1000);
        row1Time = new Time(System.currentTimeMillis() - 1000);
        row1Timestamp = new Timestamp(System.currentTimeMillis() + 10000);
        stmt.setDate(4, row1Date);
        stmt.setTime(5, row1Time);
        stmt.setTimestamp(6, row1Timestamp);
        stmt.execute();
        
        stmt.setInt(1, 2);
        stmt.setString(2, " 2.2");
        stmt.setString(3, " 2.2");
        row2Date = new Date(System.currentTimeMillis() - 10000);
        row2Time = new Time(System.currentTimeMillis() - 1234);
        row2Timestamp = new Timestamp(System.currentTimeMillis() + 1234567);
        stmt.setDate(4, row2Date);
        stmt.setTime(5, row2Time);
        stmt.setTimestamp(6, row2Timestamp);
        stmt.execute();
        
        stmt.setInt(1, 3);
        if (tgtPH()) {
            stmt.setString(2, "$3.3");
            stmt.setString(3, "$3.3");
        } else if (tgtSQ()||tgtTR()) {
            stmt.setString(2, "3.3");
            stmt.setString(3, "3.3");
        }
        row3Date = new Date(System.currentTimeMillis() - 100);
        row3Time = new Time(System.currentTimeMillis() - 789);
        row3Timestamp = new Timestamp(System.currentTimeMillis() + 78901);
        stmt.setDate(4, row3Date);
        stmt.setTime(5, row3Time);
        stmt.setTimestamp(6, row3Timestamp);
        stmt.execute();
        
        conn.commit();
    }

    @Test
    public void testKeyFilterWithIntegerValue() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        if (tgtPH()) query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE to_number(a_string) = 1";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE CAST(a_string as INT) = 1";
        int expectedId = 1;
        runOneRowQueryTest(query, expectedId);
    }
    
    @Test
    public void testKeyFilterWithDoubleValue() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        if (tgtPH()) query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE to_number(a_string) = 2.2";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE CAST(a_string as FLOAT) = 2.2";
        int expectedId = 2;
        runOneRowQueryTest(query, expectedId);
    }

    @Test
    public void testNonKeyFilterWithIntegerValue() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        if (tgtPH()) query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE to_number(b_string) = 1";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE CAST(b_string as INT) = 1";
        int expectedId = 1;
        runOneRowQueryTest(query, expectedId);
    }
    
    @Test
    public void testNonKeyFilterWithDoubleValue() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        if (tgtPH()) query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE to_number(b_string) = 2.2";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE CAST(b_string as FLOAT) = 2.2";
        int expectedId = 2;
        runOneRowQueryTest(query, expectedId);
    }

    @Test
    public void testKeyProjectionWithIntegerValue() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        if (tgtPH()) query = "select to_number(a_string) from " + TO_NUMBER_TABLE_NAME + " where a_id = 1";
        else if (tgtSQ()||tgtTR()) query = "select CAST(a_string as INT) from " + TO_NUMBER_TABLE_NAME + " where a_id = 1";
        int expectedIntValue = 1;
        runOneRowQueryTest(query, expectedIntValue);
    }
    
    @Test
    public void testKeyProjectionWithDecimalValue() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        if (tgtPH()) query = "select to_number(a_string) from " + TO_NUMBER_TABLE_NAME + " where a_id = 2";
        else if (tgtSQ()||tgtTR()) query = "select CAST(a_string as decimal(18,1)) from " + TO_NUMBER_TABLE_NAME + " where a_id = 2";
        BigDecimal expectedDecimalValue = new BigDecimal("2.2");
        runOneRowQueryTest(query, expectedDecimalValue);
    }
    
    @Test
    public void testNonKeyProjectionWithIntegerValue() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        if (tgtPH()) query = "select to_number(b_string) from " + TO_NUMBER_TABLE_NAME + " where a_id = 1";
        else if (tgtSQ()||tgtTR()) query = "select CAST(b_string as INT) from " + TO_NUMBER_TABLE_NAME + " where a_id = 1";
        int expectedIntValue = 1;
        runOneRowQueryTest(query, expectedIntValue);
    }
    
    @Test
    public void testNonKeyProjectionWithDecimalValue() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        if (tgtPH()) query = "select to_number(b_string) from " + TO_NUMBER_TABLE_NAME + " where a_id = 2";
        else if (tgtSQ()||tgtTR()) query = "select CAST(b_string as DECIMAL(18,2)) from " + TO_NUMBER_TABLE_NAME + " where a_id = 2";
        BigDecimal expectedDecimalValue = new BigDecimal("2.2");
        runOneRowQueryTest(query, expectedDecimalValue);
    }
    
    @Test
    public void testKeyFilterWithPatternParam() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        if (tgtPH()) query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE to_number(a_string, '\u00A4###.####') = 3.3";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE CAST(a_string as FLOAT) = 3.3";
        int expectedId = 3;
        runOneRowQueryTest(query, expectedId);
    }
    
    @Test
    public void testNonKeyFilterWithPatternParam() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        if (tgtPH()) query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE to_number(b_string, '\u00A4#.#') = 3.3";
        else if (tgtSQ()||tgtTR()) query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE CAST(b_string as FLOAT) = 3.3";
        int expectedId = 3;
        runOneRowQueryTest(query, expectedId);
    }
    
    @Test
    public void testDateFilter() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        if (tgtPH()) {
            String pattern = "yyyyMMddHHmmssZ";
            query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE to_number(a_date, '" + pattern + "') = " + row1Date.getTime() ;
        } else if (tgtSQ()||tgtTR()) {
            query = "SELECT [first 1] a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE (JULIANTIMESTAMP(a_date) - JULIANTIMESTAMP(TIMESTAMP '1970-01-01 00:00:00'))/1000 = " + Date.valueOf(row1Date.toString()).getTime() + " order by 1";
        }
        int expectedId = 1;
        runOneRowQueryTest(query, expectedId);
    }
    
    
    @Test
    public void testTimeFilter() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        if (tgtPH()) {
    	    String pattern = "HH:mm:ss z";
            query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE to_number(a_time, '" + pattern + "') = " + row1Time.getTime() ;
        } else {
            /* Most java time function returns UTC.  We also run phoenix_test
             * with -Duser.timezone=UTC to force UTC.  But when the column type
             * is Time, Julian timestamp may still stick in the local date i
             * part with the machine time zone setting (say, PST).  This query
             * skips the date part, and only compares HH:mm:ss.
             */
            SimpleDateFormat sd = new SimpleDateFormat("HH:mm:ss");
            query = "SELECT [first 1] a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE (JULIANTIMESTAMP(a_time) - JULIANTIMESTAMP(CURRENT_DATE))/1000 = " + (sd.parse(sd.format(row1Time)).getTime()/1000)*1000 + " order by 1";
        }
        int expectedId = 1;
        runOneRowQueryTest(query, expectedId);
    }
    
    @Test 
    public void testDateFilterWithoutPattern() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        if (tgtPH()) query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE to_number(a_date) = " + row2Date.getTime() ;
        else if (tgtSQ()||tgtTR()) query = "SELECT [first 1] a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE (JULIANTIMESTAMP(a_date) - JULIANTIMESTAMP(TIMESTAMP '1970-01-01 00:00:00'))/1000 = " + Date.valueOf(row2Date.toString()).getTime() + " order by a_timestamp desc";
        int expectedId = 2;
        runOneRowQueryTest(query, expectedId);
    }
    
    
    @Test
    public void testTimeFilterWithoutPattern() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        if (tgtPH()) query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE to_number(a_time) = " + row2Time.getTime() ;
        else {
            /* Most java time function returns UTC.  We also run phoenix_test 
             * with -Duser.timezone=UTC to force UTC.  But when the column type
             * is Time, Julian timestamp may still stick in the local date i
             * part with the machine time zone setting (say, PST).  This query 
             * skips the date part, and only compares HH:mm:ss.
             */
            SimpleDateFormat sd = new SimpleDateFormat("HH:mm:ss");
            query = "SELECT [first 1] a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE (JULIANTIMESTAMP(a_time) - JULIANTIMESTAMP(CURRENT_DATE))/1000 = " + (sd.parse(sd.format(row2Time)).getTime()/1000)*1000 + " order by a_timestamp desc" ;
        }
        int expectedId = 2;
        runOneRowQueryTest(query, expectedId);
    }
    
    @Test
    public void testTimeStampFilter() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        if (tgtPH()) {
    	    String pattern = "yyMMddHHmmssZ";
            query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE to_number(a_timestamp, '" + pattern + "') = " + row1Timestamp.getTime() ;
        } else if (tgtSQ()||tgtTR()) {
            query = "SELECT a_id FROM " + TO_NUMBER_TABLE_NAME + " WHERE (JULIANTIMESTAMP(a_timestamp) - JULIANTIMESTAMP(TIMESTAMP '1970-01-01 00:00:00'))/1000 = " + row1Timestamp.getTime() ;
        }
        int expectedId = 1;
        runOneRowQueryTest(query, expectedId);
    }
    
    @Test
    public void testDateProjection() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        BigDecimal expectedDecimalValue = null;
        if (tgtPH()) { 
            query = "select to_number(a_date) from " + TO_NUMBER_TABLE_NAME + " where a_id = 1";
            expectedDecimalValue = new BigDecimal(row1Date.getTime());
        } else if (tgtSQ()||tgtTR()) {
            query = "select (JULIANTIMESTAMP(a_date) - JULIANTIMESTAMP(TIMESTAMP '1970-01-01 00:00:00'))/1000 from " + TO_NUMBER_TABLE_NAME + " where a_id = 1";
            expectedDecimalValue = new BigDecimal(Date.valueOf(row1Date.toString()).getTime());
        }
 
        runOneRowQueryTest(query, expectedDecimalValue);
    }
 
    @Test
    public void testTimeProjection() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        BigDecimal expectedDecimalValue = null;
        if (tgtPH()) {
            query = "select to_number(a_time) from " + TO_NUMBER_TABLE_NAME + " where a_id = 2";
            expectedDecimalValue = new BigDecimal(row2Time.getTime());
        } else if (tgtSQ()||tgtTR()) {
            /* Most java time function returns UTC.  We also run phoenix_test
             * with -Duser.timezone=UTC to force UTC.  But when the column type
             * is Time, Julian timestamp may still stick in the local date i
             * part with the machine time zone setting (say, PST).  This query
             * skips the date part, and only compares HH:mm:ss.
             */
            SimpleDateFormat sd = new SimpleDateFormat("HH:mm:ss");
            query = "select (JULIANTIMESTAMP(a_time) - JULIANTIMESTAMP(CURRENT_DATE))/1000 from " + TO_NUMBER_TABLE_NAME + " where a_id = 2";
            expectedDecimalValue = new BigDecimal((sd.parse(sd.format(row2Time)).getTime()/1000)*1000);
        }
        runOneRowQueryTest(query, expectedDecimalValue);
    }
    
    @Test
    public void testTimeStampProjection() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        BigDecimal expectedDecimalValue = null;
        if (tgtPH()) {
            query = "select to_number(a_timestamp) from " + TO_NUMBER_TABLE_NAME + " where a_id = 3";
            expectedDecimalValue = new BigDecimal(row3Timestamp.getTime());
        } else if (tgtSQ()||tgtTR()) {
            query = "select (JULIANTIMESTAMP(a_timestamp) - JULIANTIMESTAMP(TIMESTAMP '1970-01-01 00:00:00'))/1000 from " + TO_NUMBER_TABLE_NAME + " where a_id = 3";
            expectedDecimalValue = new BigDecimal(row3Timestamp.getTime());
        }
        runOneRowQueryTest(query, expectedDecimalValue);
    }
    
    private void runOneRowQueryTest(String oneRowQuery, BigDecimal expectedDecimalValue) throws Exception {
    	runOneRowQueryTest(oneRowQuery, false, null, expectedDecimalValue);
    }
    
    private void runOneRowQueryTest(String oneRowQuery, int expectedIntValue) throws Exception {
    	runOneRowQueryTest(oneRowQuery, true, expectedIntValue, null);
    }
    
    private void runOneRowQueryTest(String oneRowQuery, boolean isIntegerColumn, Integer expectedIntValue, BigDecimal expectedDecimalValue) throws Exception {
        try {
            PreparedStatement statement = conn.prepareStatement(oneRowQuery);
            ResultSet rs = statement.executeQuery();
            
            assertTrue (rs.next());
            if (isIntegerColumn)
            	assertEquals(expectedIntValue.intValue(), rs.getInt(1));
            else
            	assertTrue(expectedDecimalValue == rs.getBigDecimal(1) || (expectedDecimalValue != null && expectedDecimalValue.compareTo(rs.getBigDecimal(1)) == 0));
            assertFalse(rs.next());
        }
        finally {
        }
    }
}
