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
import java.text.*;

/**
 * Tests for the TO_CHAR built-in function.
 * 
 * @see ToCharFunction
 * @author elevine
 * @since 0.1
 */
public class ToCharFunctionTest extends BaseTest {
    
    public static final String TO_CHAR_TABLE_NAME = "TO_CHAR_TABLE";
   
    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table " + TO_CHAR_TABLE_NAME));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */
 
    private Date row1Date;
    private Time row1Time;
    private Timestamp row1Timestamp;
    private Integer row1Integer;
    private BigDecimal row1Decimal;
    private Date row2Date;
    private Time row2Time;
    private Timestamp row2Timestamp;
    private Integer row2Integer;
    private BigDecimal row2Decimal;
    
/* TRAF 
    @Before
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(
            value="DMI_BIGDECIMAL_CONSTRUCTED_FROM_DOUBLE", 
            justification="Test code.")
*/
    public void initTable() throws Exception {

        String TO_CHAR_TABLE_DDL = null;
        if (tgtPH()) TO_CHAR_TABLE_DDL = "create table " + TO_CHAR_TABLE_NAME +
            "(pk integer not null, \n" +
            "col_date date not null, \n" +
            "col_time date not null, \n" +
            "col_timestamp timestamp not null, \n" +
            "col_integer integer not null, \n" +
            "col_decimal decimal not null \n" +
            "CONSTRAINT my_pk PRIMARY KEY (pk))";
        else if (tgtSQ()||tgtTR()) TO_CHAR_TABLE_DDL = "create table " + TO_CHAR_TABLE_NAME +
            "(pk integer not null, \n" +
            "col_date date not null, \n" +
            "col_time time not null, \n" +
            "col_timestamp timestamp not null, \n" +
            "col_integer integer not null, \n" +
            "col_decimal decimal(18,3) not null \n" +
            ", CONSTRAINT pk_tocharfunc PRIMARY KEY (pk))"; 

        conn.createStatement().execute(TO_CHAR_TABLE_DDL);
        conn.setAutoCommit(false);
        
        PreparedStatement stmt = null;
        if (tgtPH()||tgtTR()) stmt = conn.prepareStatement(
                "upsert into " + TO_CHAR_TABLE_NAME +
                "    (pk, " +
                "    col_date," +
                "    col_time," +
                "    col_timestamp," +
                "    col_integer," +
                "    col_decimal)" +
                "VALUES (?, ?, ?, ?, ?, ?)");
        else if (tgtSQ()) stmt = conn.prepareStatement(
                "insert into " + TO_CHAR_TABLE_NAME +
                "    (pk, " +
                "    col_date," +
                "    col_time," +
                "    col_timestamp," +
                "    col_integer," +
                "    col_decimal)" +
                "VALUES (?, ?, ?, ?, ?, ?)");

        row1Date = new Date(System.currentTimeMillis() - 10000);
        row1Time = new Time(System.currentTimeMillis() - 1000);
        row1Timestamp = new Timestamp(System.currentTimeMillis() + 10000);
        row1Integer = 666;
        row1Decimal = new BigDecimal(33.333);
        
        stmt.setInt(1, 1);
        stmt.setDate(2, row1Date);
        stmt.setTime(3, row1Time);
        stmt.setTimestamp(4, row1Timestamp);
        stmt.setInt(5, row1Integer);
        stmt.setBigDecimal(6, row1Decimal);
        stmt.execute();
        
        row2Date = new Date(System.currentTimeMillis() - 1234567);
        row2Time = new Time(System.currentTimeMillis() - 1234);
        row2Timestamp = new Timestamp(System.currentTimeMillis() + 1234567);
        row2Integer = 10011;
        if (tgtPH()) row2Decimal = new BigDecimal(123456789.66);
        else if (tgtSQ()||tgtTR()) row2Decimal = new BigDecimal("123456789.66");
        
        stmt.setInt(1, 2);
        stmt.setDate(2, row2Date);
        stmt.setTime(3, row2Time);
        stmt.setTimestamp(4, row2Timestamp);
        stmt.setInt(5, row2Integer);
        stmt.setBigDecimal(6, row2Decimal);
        
        stmt.execute();
        conn.commit();
    }
    
    @Test
    public void testDateProjection() throws Exception {
        printTestDescription();

        initTable();
        String pattern = null;
        String query = null; 
        if (tgtPH()) {
            pattern = "yyyy.MM.dd G HH:mm:ss z";
            query = "select to_char(col_date, '" + pattern + "') from " + TO_CHAR_TABLE_NAME + " WHERE pk = 1";
        } else if (tgtSQ()||tgtTR()) {
            pattern = "yyyy-MM-dd";
            query = "select RTRIM(CAST(col_date as char(128))) from " + TO_CHAR_TABLE_NAME + " WHERE pk = 1";
        }
        String expectedString = getGMTDateFormat(pattern).format(row1Date);
        runOneRowProjectionQuery(query, expectedString);
    }
    
    @Test
    public void testTimeProjection() throws Exception {
        printTestDescription();

        initTable();
        String pattern = null;
        String query = null;
        if (tgtPH()) {
            pattern = "HH:mm:ss z";
            query = "select to_char(col_time, '" + pattern + "') from " + TO_CHAR_TABLE_NAME + " WHERE pk = 1";
        } else if (tgtSQ()||tgtTR()) {
            pattern = "HH:mm:ss";
            query = "select RTRIM(CAST(col_time as CHAR(128))) from " + TO_CHAR_TABLE_NAME + " WHERE pk = 1";
        }
        String expectedString = getGMTDateFormat(pattern).format(row1Time);
        runOneRowProjectionQuery(query, expectedString);
    }

    @Test
    public void testTimestampProjection() throws Exception {
        printTestDescription();

        initTable();
        String query = null;
        String expectedString = null;
        if (tgtPH()) {
            String pattern = "yyMMddHHmmssZ";
            query = "select to_char(col_timestamp, '" + pattern + "') from " + TO_CHAR_TABLE_NAME + " WHERE pk = 2";
            expectedString = getGMTDateFormat(pattern).format(row2Timestamp);
        } else if (tgtSQ()||tgtTR()) {
            String pattern0 = "yyyy-MM-dd HH:mm:ss";
            String pattern1 = "S";
            expectedString = getGMTDateFormat(pattern0).format(row2Timestamp) +  String.format(".%03d", Integer.parseInt(getGMTDateFormat(pattern1).format(row2Timestamp)));
            query = "select RTRIM(CAST(CAST(col_timestamp as timestamp(3))as CHAR(128))) from " + TO_CHAR_TABLE_NAME + " WHERE pk = 2";
        }
        runOneRowProjectionQuery(query, expectedString);
    }
    
    @Test
    public void testIntegerProjection() throws Exception {
        printTestDescription();

        initTable();
        String pattern = null;
        String query = null;
        if (tgtPH()) {
            pattern = "00";
            query = "select to_char(col_integer, '" + pattern + "') from " + TO_CHAR_TABLE_NAME + " WHERE pk = 1";
        } else if (tgtSQ()||tgtTR()) {
            pattern = "00";
            query = "select RTRIM(CAST(col_integer as CHAR(128))) from " + TO_CHAR_TABLE_NAME + " WHERE pk = 1";

        }
        String expectedString = new DecimalFormat(pattern).format(row1Integer);
        runOneRowProjectionQuery(query, expectedString);
    }
    
    @Test
    public void testDecimalProjection() throws Exception {
        printTestDescription();

        initTable();
        String pattern = null;
        String query = null;
        if (tgtPH()) {
            pattern = "0.###E0";
            query = "select to_char(col_decimal, '" + pattern + "') from " + TO_CHAR_TABLE_NAME + " WHERE pk = 2";
        } else if (tgtSQ()||tgtTR()) {
            pattern = "#########.##";
            query = "select RTRIM(CAST(CAST(col_decimal as DECIMAL(18,2)) as CHAR(128))) from " + TO_CHAR_TABLE_NAME + " WHERE pk = 2";
        }
        String expectedString = new DecimalFormat(pattern).format(row2Decimal);
        runOneRowProjectionQuery(query, expectedString);
    }
    
    @Test 
    public void testDateFilter() throws Exception {
        printTestDescription();

        initTable();
        String pattern = null;
        String expectedString = null;
        String query = null;
        if (tgtPH()) {
            pattern = "yyyyMMddHHmmssZ";
            expectedString = getGMTDateFormat(pattern).format(row1Date);
            query = "select pk from " + TO_CHAR_TABLE_NAME + " WHERE to_char(col_date, '" + pattern + "') = '" + expectedString + "'";
        } else {
            pattern = "yyyy-MM-dd";
            expectedString = getGMTDateFormat(pattern).format(row1Date);
            query = "select [first 1] pk from " + TO_CHAR_TABLE_NAME + " WHERE RTRIM(CAST(col_date as CHAR(128))) = '" + expectedString + "' order by 1";
        }
        runOneRowFilterQuery(query, 1);
    }
    
    @Test 
    public void testTimeFilter() throws Exception {
        printTestDescription();

        initTable();
        String pattern = null;
        String expectedString = null;
        String query = null;
        if (tgtPH()) { 
            pattern = "ddHHmmssSSSZ";
            expectedString = getGMTDateFormat(pattern).format(row1Time);
            query = "select pk from " + TO_CHAR_TABLE_NAME + " WHERE to_char(col_time, '" + pattern + "') = '" + expectedString + "'";
        } else if (tgtSQ()||tgtTR()) {
            pattern = "HH:mm:ss";
            expectedString = getGMTDateFormat(pattern).format(row1Time);
            query = "select [first 1] pk from " + TO_CHAR_TABLE_NAME + " WHERE RTRIM(CAST(col_time as CHAR(128))) = '" + expectedString + "' order by 1";
        }
        runOneRowFilterQuery(query, 1);
    }
    
    @Test 
    public void testTimestampFilter() throws Exception {
        printTestDescription();

        initTable();
        String expectedString = null;
        String query = null;
        if (tgtPH()) {
            String pattern = "yy.MM.dd G HH:mm:ss z";
            expectedString = getGMTDateFormat(pattern).format(row2Timestamp);
            query = "select pk from " + TO_CHAR_TABLE_NAME + " WHERE to_char(col_timestamp, '" + pattern + "') = '" + expectedString + "'";

        } else if (tgtSQ()||tgtTR()) {
            String pattern0 = "yyyy-MM-dd HH:mm:ss";
            String pattern1 = "S";
            expectedString = getGMTDateFormat(pattern0).format(row2Timestamp) +  String.format(".%03d", Integer.parseInt(getGMTDateFormat(pattern1).format(row2Timestamp)));
            query = "select pk from " + TO_CHAR_TABLE_NAME + " WHERE RTRIM(CAST(CAST(col_timestamp as timestamp(3))as CHAR(128))) = '" + expectedString + "'";
        }
        runOneRowFilterQuery(query, 2);
    }
    
    @Test 
    public void testIntegerFilter() throws Exception {
        printTestDescription();

        initTable();
        String pattern = null;
        String expectedString = null;
        String query = null;
        if (tgtPH()) {
            pattern = "000";
            expectedString = new DecimalFormat(pattern).format(row1Integer);
            query = "select pk from " + TO_CHAR_TABLE_NAME + " WHERE to_char(col_integer, '" + pattern + "') = '" + expectedString + "'";
        } else if (tgtSQ()||tgtTR()) {
            pattern = "000";
            expectedString = new DecimalFormat(pattern).format(row1Integer);
            query = "select pk from " + TO_CHAR_TABLE_NAME + " WHERE RTRIM(CAST(col_integer as CHAR(128))) = '" + expectedString + "'";
        }
        runOneRowFilterQuery(query, 1);
    }
    
    @Test 
    public void testDecimalFilter() throws Exception {
        printTestDescription();

        initTable();
        String pattern = null;
        String expectedString = null;
        String query = null;
        if (tgtPH()) {
            pattern = "00.###E0";
            expectedString = new DecimalFormat(pattern).format(row2Decimal);
            query = "select pk from " + TO_CHAR_TABLE_NAME + " WHERE to_char(col_decimal, '" + pattern + "') = '" + expectedString + "'";
        } else if (tgtSQ()||tgtTR()) {
            pattern = "#########.##";
            expectedString = new DecimalFormat(pattern).format(row2Decimal);
            query = "select pk from " + TO_CHAR_TABLE_NAME + " WHERE RTRIM(CAST(CAST(col_decimal as DECIMAL(18,2)) as CHAR(128))) = '" + expectedString + "'";
        }
        runOneRowFilterQuery(query, 2);
    }
    
    private void runOneRowProjectionQuery(String oneRowQuery, String projectedValue) throws Exception {
    	runOneRowQueryTest(oneRowQuery, null, projectedValue);
    }
    
    private void runOneRowFilterQuery(String oneRowQuery, int pkValue) throws Exception {
    	runOneRowQueryTest(oneRowQuery, pkValue, null);
    }
    
    private void runOneRowQueryTest(String oneRowQuery, Integer pkValue, String projectedValue) throws Exception {
        try {
            PreparedStatement statement = conn.prepareStatement(oneRowQuery);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            if (pkValue != null)
            	assertEquals(pkValue.intValue(), rs.getInt(1));
            else 
            	assertEquals(projectedValue, rs.getString(1));
            assertFalse(rs.next());
        }
        finally {
        }
    }
    
    private DateFormat getGMTDateFormat(String pattern) {
        DateFormat result = new SimpleDateFormat(pattern);
        result.setTimeZone(TimeZone.getTimeZone("GMT"));
        return result;
    }
}
