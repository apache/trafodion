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

import com.google.common.collect.Lists;
import com.google.common.collect.Ordering;

public class ProductMetricsTest extends BaseTest {
    private static final String PRODUCT_METRICS_NAME = "PRODUCT_METRICS";

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table " + PRODUCT_METRICS_NAME));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    private static final String DS1 = "1970-01-01 00:58:00";
    private static final String DS2 = "1970-01-01 01:02:00";
    private static final String DS3 = "1970-01-01 01:30:00";
    private static final String DS4 = "1970-01-01 01:45:00";
    private static final String DS5 = "1970-01-01 02:00:00";
    private static final String DS6 = "1970-01-01 04:00:00";
    private static final Date D1 = toDate(DS1);
    private static final Date D2 = toDate(DS2);
    private static final Date D3 = toDate(DS3);
    private static final Date D4 = toDate(DS4);
    private static final Date D5 = toDate(DS5);
    private static final Date D6 = toDate(DS6);
    private static final Object ROUND_1HR = toDate("1970-01-01 01:00:00");
    private static final Object ROUND_2HR = toDate("1970-01-01 02:00:00");
    private static final String F1 = "A";
    private static final String F2 = "B";
    private static final String F3 = "C";
    private static final String R1 = "R1";
    private static final String R2 = "R2";
    
    private static Date toDate(String dateString) {
        try {
            java.util.Date utilDate = new SimpleDateFormat("yyyy-MM-dd hh:mm:ss", Locale.ENGLISH).parse(dateString);
            java.sql.Date sqlDate = new java.sql.Date(utilDate.getTime());
            return sqlDate;
        } catch (ParseException e) {
            throw new RuntimeException(e);
        }
    }
    
    private void initTable() throws Exception {
        createTestTable(PRODUCT_METRICS_NAME);
    }

    private void initTableValues() throws Exception {
        initTable();

        PreparedStatement stmt = null;
        if (tgtPH()) stmt = conn.prepareStatement(
            "upsert into " +
            "PRODUCT_METRICS(" +
            "    ORGANIZATION_ID, " +
            "    DATE, " +
            "    FEATURE, " +
            "    UNIQUE_USERS, " +
            "    TRANSACTIONS, " +
            "    CPU_UTILIZATION, " +
            "    DB_UTILIZATION, " +
            "    REGION, " +
            "    IO_TIME)" +
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
        else if (tgtTR()) stmt = conn.prepareStatement(
            "upsert into " +
            "PRODUCT_METRICS(" +
            "    ORGANIZATION_ID, " +
            "    DATE1, " +
            "    FEATURE, " +
            "    UNIQUE_USERS, " +
            "    TRANSACTIONS, " +
            "    CPU_UTILIZATION, " +
            "    DB_UTILIZATION, " +
            "    REGION, " +
            "    IO_TIME)" +
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
        else if (tgtSQ()) stmt = conn.prepareStatement(
            "insert into " +
            "PRODUCT_METRICS(" +
            "    ORGANIZATION_ID, " +
            "    DATE1, " +
            "    FEATURE, " +
            "    UNIQUE_USERS, " +
            "    TRANSACTIONS, " +
            "    CPU_UTILIZATION, " +
            "    DB_UTILIZATION, " +
            "    REGION, " +
            "    IO_TIME)" +
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
 
        stmt.setString(1, tenantId);
        stmt.setDate(2, D1);
        stmt.setString(3, F1);
        stmt.setInt(4, 10);
        stmt.setLong(5, 100L);
        stmt.setBigDecimal(6, BigDecimal.valueOf(0.5));
        stmt.setBigDecimal(7, BigDecimal.valueOf(0.2));
        stmt.setString(8, R2);
        stmt.setNull(9, Types.BIGINT);
        stmt.execute();
        
        stmt.setString(1, tenantId);
        stmt.setDate(2, D2);
        stmt.setString(3, F1);
        stmt.setInt(4, 20);
        stmt.setLong(5, 200);
        stmt.setBigDecimal(6, BigDecimal.valueOf(1.0));
        stmt.setBigDecimal(7, BigDecimal.valueOf(0.4));
        stmt.setString(8, null);
        stmt.setLong(9, 2000);
        stmt.execute();
        
        stmt.setString(1, tenantId);
        stmt.setDate(2, D3);
        stmt.setString(3, F1);
        stmt.setInt(4, 30);
        stmt.setLong(5, 300);
        stmt.setBigDecimal(6, BigDecimal.valueOf(2.5));
        stmt.setBigDecimal(7, BigDecimal.valueOf(0.6));
        stmt.setString(8, R1);
        stmt.setNull(9, Types.BIGINT);
        stmt.execute();
        
        stmt.setString(1, tenantId);
        stmt.setDate(2, D4);
        stmt.setString(3, F2);
        stmt.setInt(4, 40);
        stmt.setLong(5, 400);
        stmt.setBigDecimal(6, BigDecimal.valueOf(3.0));
        stmt.setBigDecimal(7, BigDecimal.valueOf(0.8));
        stmt.setString(8, R1);
        stmt.setLong(9, 4000);
        stmt.execute();
        
        stmt.setString(1, tenantId);
        stmt.setDate(2, D5);
        stmt.setString(3, F3);
        stmt.setInt(4, 50);
        stmt.setLong(5, 500);
        stmt.setBigDecimal(6, BigDecimal.valueOf(3.5));
        stmt.setBigDecimal(7, BigDecimal.valueOf(1.2));
        stmt.setString(8, R2);
        stmt.setLong(9, 5000);
        stmt.execute();
        
        stmt.setString(1, tenantId);
        stmt.setDate(2, D6);
        stmt.setString(3, F1);
        stmt.setInt(4, 60);
        stmt.setLong(5, 600);
        stmt.setBigDecimal(6, BigDecimal.valueOf(4.0));
        stmt.setBigDecimal(7, BigDecimal.valueOf(1.4));
        stmt.setString(8, null);
        stmt.setNull(9, Types.BIGINT);
        stmt.execute();
    }
        
    private void initDateTableValues(Date startDate) throws Exception {
        initDateTableValues(startDate, 2.0);
    }

    private void initDateTableValues(Date startDate, double dateIncrement) throws Exception {
        initTable();

        PreparedStatement stmt = null;
        if (tgtPH()) stmt = conn.prepareStatement(
            "upsert into " +
            "PRODUCT_METRICS(" +
            "    ORGANIZATION_ID, " +
            "    DATE, " +
            "    FEATURE, " +
            "    UNIQUE_USERS, " +
            "    TRANSACTIONS, " +
            "    CPU_UTILIZATION, " +
            "    DB_UTILIZATION, " +
            "    REGION, " +
            "    IO_TIME)" +
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
        else if (tgtTR()) stmt = conn.prepareStatement(
            "upsert into " +
            "PRODUCT_METRICS(" +
            "    ORGANIZATION_ID, " +
            "    DATE1, " +
            "    FEATURE, " +
            "    UNIQUE_USERS, " +
            "    TRANSACTIONS, " +
            "    CPU_UTILIZATION, " +
            "    DB_UTILIZATION, " +
            "    REGION, " +
            "    IO_TIME)" +
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
        else if (tgtSQ()) stmt = conn.prepareStatement(
            "insert into " +
            "PRODUCT_METRICS(" +
            "    ORGANIZATION_ID, " +
            "    DATE1, " +
            "    FEATURE, " +
            "    UNIQUE_USERS, " +
            "    TRANSACTIONS, " +
            "    CPU_UTILIZATION, " +
            "    DB_UTILIZATION, " +
            "    REGION, " +
            "    IO_TIME)" +
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");

        stmt.setString(1, tenantId);
        stmt.setDate(2, startDate);
        stmt.setString(3, "A");
        stmt.setInt(4, 10);
        stmt.setLong(5, 100L);
        stmt.setBigDecimal(6, BigDecimal.valueOf(0.5));
        stmt.setBigDecimal(7, BigDecimal.valueOf(0.2));
        stmt.setString(8, R2);
        stmt.setNull(9, Types.BIGINT);
        stmt.execute();
        
        startDate = new Date(startDate.getTime() + (long)(MILLIS_IN_DAY * dateIncrement));
        stmt.setString(1, tenantId);
        stmt.setDate(2, startDate);
        stmt.setString(3, "B");
        stmt.setInt(4, 20);
        stmt.setLong(5, 200);
        stmt.setBigDecimal(6, BigDecimal.valueOf(1.0));
        stmt.setBigDecimal(7, BigDecimal.valueOf(0.4));
        stmt.setString(8, null);
        stmt.setLong(9, 2000);
        stmt.execute();
        
        startDate = new Date(startDate.getTime() + (long)(MILLIS_IN_DAY * dateIncrement));
        stmt.setString(1, tenantId);
        stmt.setDate(2, startDate);
        stmt.setString(3, "C");
        stmt.setInt(4, 30);
        stmt.setLong(5, 300);
        stmt.setBigDecimal(6, BigDecimal.valueOf(2.5));
        stmt.setBigDecimal(7, BigDecimal.valueOf(0.6));
        stmt.setString(8, R1);
        stmt.setNull(9, Types.BIGINT);
        stmt.execute();
        
        startDate = new Date(startDate.getTime() + (long)(MILLIS_IN_DAY * dateIncrement));
        stmt.setString(1, tenantId);
        stmt.setDate(2, startDate);
        stmt.setString(3, "D");
        stmt.setInt(4, 40);
        stmt.setLong(5, 400);
        stmt.setBigDecimal(6, BigDecimal.valueOf(3.0));
        stmt.setBigDecimal(7, BigDecimal.valueOf(0.8));
        stmt.setString(8, R1);
        stmt.setLong(9, 4000);
        stmt.execute();
        
        startDate = new Date(startDate.getTime() + (long)(MILLIS_IN_DAY * dateIncrement));
        stmt.setString(1, tenantId);
        stmt.setDate(2, startDate);
        stmt.setString(3, "E");
        stmt.setInt(4, 50);
        stmt.setLong(5, 500);
        stmt.setBigDecimal(6, BigDecimal.valueOf(3.5));
        stmt.setBigDecimal(7, BigDecimal.valueOf(1.2));
        stmt.setString(8, R2);
        stmt.setLong(9, 5000);
        stmt.execute();
        
        startDate = new Date(startDate.getTime() + (long)(MILLIS_IN_DAY * dateIncrement));
        stmt.setString(1, tenantId);
        stmt.setDate(2, startDate);
        stmt.setString(3, "F");
        stmt.setInt(4, 60);
        stmt.setLong(5, 600);
        stmt.setBigDecimal(6, BigDecimal.valueOf(4.0));
        stmt.setBigDecimal(7, BigDecimal.valueOf(1.4));
        stmt.setString(8, null);
        stmt.setNull(9, Types.BIGINT);
        stmt.execute();
    }
    
    @Test
    public void testDateRangeAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT count(1), feature f FROM PRODUCT_METRICS WHERE organization_id=? AND date >= to_date(?) AND date <= to_date(?) GROUP BY f";
        else if (tgtSQ()||tgtTR()) query = "SELECT count(1), feature f FROM PRODUCT_METRICS WHERE organization_id=? AND date1 >= ? AND date1 <= ? GROUP BY feature order by 1 desc";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, DS2);
            statement.setString(3, DS4);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(2, rs.getLong(1));
            assertEquals(F1, rs.getString(2));
            assertTrue(rs.next());
            assertEquals(1, rs.getLong(1));
            assertEquals(F2, rs.getString(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testPartiallyEvaluableAnd() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT date FROM PRODUCT_METRICS WHERE organization_id=? AND unique_users >= 30 AND transactions >= 300 AND cpu_utilization > 2 AND db_utilization > 0.5 AND io_time = 4000";
        else if (tgtSQ()||tgtTR()) query = "SELECT date1 FROM PRODUCT_METRICS WHERE organization_id=? AND unique_users >= 30 AND transactions >= 300 AND cpu_utilization > 2 AND db_utilization > 0.5 AND io_time = 4000";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(D4, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(D4, rs.getTimestamp(1));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testPartiallyEvaluableOr() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT date FROM PRODUCT_METRICS WHERE organization_id=? AND (transactions = 10000 OR unset_column = 5 OR io_time = 4000)";
        else if (tgtSQ()||tgtTR()) query = "SELECT date1 FROM PRODUCT_METRICS WHERE organization_id=? AND (transactions = 10000 OR unset_column = 5 OR io_time = 4000)";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(D4, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(D4, rs.getTimestamp(1));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testConstantTrueHaving() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT count(1), feature FROM PRODUCT_METRICS WHERE organization_id=? AND date >= to_date(?) AND date <= to_date(?) GROUP BY feature HAVING 1=1";
        else if (tgtSQ()||tgtTR()) query = "SELECT count(1), feature FROM PRODUCT_METRICS WHERE organization_id=? AND date1 >= ? AND date1 <= ? GROUP BY feature HAVING 1=1";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, DS2);
            statement.setString(3, DS4);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(2, rs.getLong(1));
            assertEquals(F1, rs.getString(2));
            assertTrue(rs.next());
            assertEquals(1, rs.getLong(1));
            assertEquals(F2, rs.getString(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testConstantFalseHaving() throws Exception {
        printTestDescription();
        String query = null;
        if (tgtPH()) query = "SELECT count(1), feature FROM PRODUCT_METRICS WHERE organization_id=? AND date >= to_date(?) AND date <= to_date(?) GROUP BY feature HAVING 1=1 and 0=1";
        else if (tgtSQ()||tgtTR()) query = "SELECT count(1), feature FROM PRODUCT_METRICS WHERE organization_id=? AND date1 >= ? AND date1 <= ? GROUP BY feature HAVING 1=1 and 0=1";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, DS2);
            statement.setString(3, DS4);
            ResultSet rs = statement.executeQuery();
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testDateRangeHavingAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT count(1), feature FROM PRODUCT_METRICS WHERE organization_id=? AND date >= to_date(?) AND date <= to_date(?) GROUP BY feature HAVING count(1) >= 2";
        else if (tgtSQ()||tgtTR()) query = "SELECT count(1), feature FROM PRODUCT_METRICS WHERE organization_id=? AND date1 >= ? AND date1 <= ? GROUP BY feature HAVING count(1) >= 2";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, DS2);
            statement.setString(3, DS4);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(2, rs.getLong(1));
            assertEquals(F1, rs.getString(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testDateRangeSumLongAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT sum(transactions), feature FROM PRODUCT_METRICS WHERE organization_id=? AND date >= to_date(?) AND date <= to_date(?) GROUP BY feature";
        else if (tgtSQ()||tgtTR()) query = "SELECT sum(transactions), feature FROM PRODUCT_METRICS WHERE organization_id=? AND date1 >= ? AND date1 <= ? GROUP BY feature";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, DS2);
            statement.setString(3, DS4);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(500, rs.getLong(1));
            assertEquals(F1, rs.getString(2));
            assertTrue(rs.next());
            assertEquals(400, rs.getLong(1));
            assertEquals(F2, rs.getString(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testRoundAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT round(date,'hour',1) r,count(1) FROM PRODUCT_METRICS WHERE organization_id=? GROUP BY r";
        else if (tgtSQ()||tgtTR()) query = "SELECT date_trunc('hour',date_add(date1,interval '30' minute)) r,count(1) FROM PRODUCT_METRICS WHERE organization_id=? GROUP BY date_trunc('hour',date_add(date1,interval '30' minute)) order by 1";

        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            Date d = null;
            Timestamp t = null;
            int c;
            ResultSet rs = statement.executeQuery();
            
            assertTrue(rs.next());
            if (tgtPH()) d = rs.getDate(1);
            else if (tgtSQ()||tgtTR()) t = rs.getTimestamp(1);
            c = rs.getInt(2);
            if (tgtPH()) assertEquals(1 * 60 * 60 * 1000, d.getTime()); // Date bucketed into 1 hr
            else if (tgtSQ()||tgtTR()) assertEquals(1 * 60 * 60 * 1000, t.getTime());
            assertEquals(2, c);
            
            assertTrue(rs.next());
            if (tgtPH()) d = rs.getDate(1);
            else if (tgtSQ()||tgtTR()) t = rs.getTimestamp(1);
            c = rs.getInt(2);
            if (tgtPH()) assertEquals(2 * 60 * 60 * 1000, d.getTime()); // Date bucketed into 2 hr
            else if (tgtSQ()||tgtTR()) assertEquals(2 * 60 * 60 * 1000, t.getTime());
            assertEquals(3, c);
            
            assertTrue(rs.next());
            if (tgtPH()) d = rs.getDate(1);
            else if (tgtSQ()||tgtTR()) t = rs.getTimestamp(1);
            c = rs.getInt(2);
            if (tgtPH()) assertEquals(4 * 60 * 60 * 1000, d.getTime()); // Date bucketed into 4 hr
            else if (tgtSQ()||tgtTR()) assertEquals(4 * 60 * 60 * 1000, t.getTime());
            assertEquals(1, c);
            
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testRoundScan() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT round(date,'hour') FROM PRODUCT_METRICS WHERE organization_id=?";
        else if (tgtSQ()||tgtTR()) query = "SELECT date_trunc('hour',date_add(date1,interval '30' minute)) FROM PRODUCT_METRICS WHERE organization_id=? order by 1";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            long t;
            ResultSet rs = statement.executeQuery();
            
            t = 1 * 60 * 60 * 1000;

            assertTrue(rs.next());
            if (tgtPH()) assertEquals(t, rs.getDate(1).getTime()); // Date bucketed into 1 hr
            else if (tgtSQ()||tgtTR()) assertEquals(t, rs.getTimestamp(1).getTime());

            assertTrue(rs.next());
            if (tgtPH()) assertEquals(t, rs.getDate(1).getTime()); // Date bucketed into 1 hr
            else if (tgtSQ()||tgtTR()) assertEquals(t, rs.getTimestamp(1).getTime()); 

            t = 2 * 60 * 60 * 1000;

            assertTrue(rs.next());
            if (tgtPH()) assertEquals(t, rs.getDate(1).getTime()); // Date bucketed into 2 hr
            else if (tgtSQ()||tgtTR()) assertEquals(t, rs.getTimestamp(1).getTime());

            assertTrue(rs.next());
            if (tgtPH()) assertEquals(t, rs.getDate(1).getTime()); // Date bucketed into 2 hr
            else if (tgtSQ()||tgtTR()) assertEquals(t, rs.getTimestamp(1).getTime());

            assertTrue(rs.next());
            if (tgtPH()) assertEquals(t, rs.getDate(1).getTime()); // Date bucketed into 2 hr
            else if (tgtSQ()||tgtTR()) assertEquals(t, rs.getTimestamp(1).getTime());
 
            t = 4 * 60 * 60 * 1000;
 
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(t, rs.getDate(1).getTime()); // Date bucketed into 4 hr
            else if (tgtSQ()||tgtTR()) assertEquals(t, rs.getTimestamp(1).getTime());
            
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testTruncAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT trunc(date,'hour'),count(1) FROM PRODUCT_METRICS WHERE organization_id=? GROUP BY trunc(date,'hour')";
        else if (tgtSQ()||tgtTR()) query = "SELECT date_trunc('hour',date1),count(1) FROM PRODUCT_METRICS WHERE organization_id=? GROUP BY date_trunc('hour',date1) order by 1";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            Date d = null;
            Timestamp t = null;
            int c;
            ResultSet rs = statement.executeQuery();
            
            assertTrue(rs.next());
            if (tgtPH()) d = rs.getDate(1);
            else if (tgtSQ()||tgtTR()) t = rs.getTimestamp(1);
            c = rs.getInt(2);
            if (tgtPH()) assertEquals(0, d.getTime()); // Date bucketed into 0 hr
            else if (tgtSQ()||tgtTR()) assertEquals(0, t.getTime());
            assertEquals(1, c);
            
            assertTrue(rs.next());
            if (tgtPH()) d = rs.getDate(1); 
            else if (tgtSQ()||tgtTR()) t = rs.getTimestamp(1); 
            c = rs.getInt(2);
            if (tgtPH()) assertEquals(1 * 60 * 60 * 1000, d.getTime()); // Date bucketed into 1 hr
            else if (tgtSQ()||tgtTR()) assertEquals(1 * 60 * 60 * 1000, t.getTime());
            assertEquals(3, c);
            
            assertTrue(rs.next());
            if (tgtPH()) d = rs.getDate(1);
            else if (tgtSQ()||tgtTR()) t = rs.getTimestamp(1);
            c = rs.getInt(2);
            if (tgtPH()) assertEquals(2 * 60 * 60 * 1000, d.getTime()); // Date bucketed into 2 hr
            else if (tgtSQ()||tgtTR()) assertEquals(2 * 60 * 60 * 1000, t.getTime());
            assertEquals(1, c);
            
            assertTrue(rs.next());
            if (tgtPH()) d = rs.getDate(1);
            else if (tgtSQ()||tgtTR()) t = rs.getTimestamp(1);
            c = rs.getInt(2);
            if (tgtPH()) assertEquals(4 * 60 * 60 * 1000, d.getTime()); // Date bucketed into 4 hr
            else if (tgtSQ()||tgtTR()) assertEquals(4 * 60 * 60 * 1000, t.getTime());
            assertEquals(1, c);

            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testAggregation() throws Exception {
        printTestDescription();

        String query = "SELECT feature,sum(unique_users) FROM PRODUCT_METRICS WHERE organization_id=? AND transactions > 0 GROUP BY feature";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertTrue(rs.next());
            assertTrue(rs.next());
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testHavingAggregation() throws Exception {
        printTestDescription();

        String query = "SELECT feature,sum(unique_users) FROM PRODUCT_METRICS WHERE organization_id=? AND transactions > 0 GROUP BY feature HAVING feature=?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, F1);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testConstantSumAggregation() throws Exception {
        printTestDescription();

        String query = "SELECT sum(1),sum(unique_users) FROM PRODUCT_METRICS WHERE organization_id=?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(6,rs.getInt(1));
            assertEquals(210,rs.getInt(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test 
    public void testMultiDimAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT feature,region,sum(unique_users) FROM PRODUCT_METRICS WHERE organization_id=? GROUP BY feature,region";
        else if (tgtSQ()||tgtTR()) query = "SELECT feature,region,sum(unique_users) FROM PRODUCT_METRICS WHERE organization_id=? GROUP BY feature,region order by 1, 3 desc";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            
            assertTrue(rs.next());
            assertEquals(F1,rs.getString(1));
            assertEquals(null,rs.getString(2));
            assertEquals(80,rs.getInt(3));
            assertTrue(rs.next());
            assertEquals(F1,rs.getString(1));
            assertEquals(R1,rs.getString(2));
            assertEquals(30,rs.getInt(3));
            assertTrue(rs.next());
            assertEquals(F1,rs.getString(1));
            assertEquals(R2,rs.getString(2));
            assertEquals(10,rs.getInt(3));

            assertTrue(rs.next());
            assertEquals(F2,rs.getString(1));
            assertEquals(R1,rs.getString(2));
            assertEquals(40,rs.getInt(3));
            
            assertTrue(rs.next());
            assertEquals(F3,rs.getString(1));
            assertEquals(R2,rs.getString(2));
            assertEquals(50,rs.getInt(3));
            
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testMultiDimRoundAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT round(date,'hour',1),feature,sum(unique_users) FROM PRODUCT_METRICS WHERE organization_id=? GROUP BY round(date,'hour',1),feature";
        else if (tgtSQ()||tgtTR()) query = "SELECT date_trunc('hour',date_add(date1,interval '30' minute)),feature,sum(unique_users) FROM PRODUCT_METRICS WHERE organization_id=? GROUP BY date_trunc('hour',date_add(date1,interval '30' minute)),feature order by 3";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            
            Date bucket1 = new Date(1 * 60 * 60 * 1000);
            Date bucket2 = new Date(2 * 60 * 60 * 1000);
            Date bucket3 = new Date(4 * 60 * 60 * 1000);

            assertTrue(rs.next());
            if (tgtPH()) assertEquals(bucket1, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(bucket1, rs.getTimestamp(1));
            assertEquals(F1,rs.getString(2));
            assertEquals(30,rs.getInt(3));
            
            
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(bucket2, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(bucket2, rs.getTimestamp(1));
            assertEquals(F1,rs.getString(2));
            assertEquals(30,rs.getInt(3));
            
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(bucket2.getTime(), rs.getDate(1).getTime());
            else if (tgtSQ()||tgtTR()) assertEquals(bucket2.getTime(), rs.getTimestamp(1).getTime());
            assertEquals(F2,rs.getString(2));
            assertEquals(40,rs.getInt(3));
            
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(bucket2, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(bucket2, rs.getTimestamp(1));
            assertEquals(F3,rs.getString(2));
            assertEquals(50,rs.getInt(3));
            
            
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(bucket3, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(bucket3, rs.getTimestamp(1));
            assertEquals(F1,rs.getString(2));
            assertEquals(60,rs.getInt(3));
            
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test 
    public void testDateRangeSumNumberUngroupedAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT sum(cpu_utilization) FROM PRODUCT_METRICS WHERE organization_id=? AND date >= to_date(?) AND date <= to_date(?)";
        else if (tgtSQ()||tgtTR()) query = "SELECT sum(cpu_utilization) FROM PRODUCT_METRICS WHERE organization_id=? AND date1 >= ? AND date1 <= ?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, DS2);
            statement.setString(3, DS4);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(BigDecimal.valueOf(6.5), rs.getBigDecimal(1));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(6.5), rs.getBigDecimal(1).setScale(1));
            assertFalse(rs.next());
        } finally {
            
        }
    }

    @Test
    public void testSumUngroupedAggregation() throws Exception {
        printTestDescription();

        String query = "SELECT sum(unique_users),sum(cpu_utilization),sum(transactions),sum(db_utilization),sum(response_time) FROM PRODUCT_METRICS WHERE organization_id=?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(210, rs.getInt(1));
            if (tgtPH()) assertEquals(BigDecimal.valueOf(14.5), rs.getBigDecimal(2));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(14.5), rs.getBigDecimal(2).setScale(1));
            assertEquals(2100L, rs.getLong(3));
            if (tgtPH()) assertEquals(BigDecimal.valueOf(4.6), rs.getBigDecimal(4));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(4.6), rs.getBigDecimal(4).setScale(1));
            assertEquals(0, rs.getLong(5));
            assertEquals(true, rs.wasNull());
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testResetColumnInSameTxn() throws Exception {
        printTestDescription();

        String query = "SELECT sum(transactions) FROM PRODUCT_METRICS WHERE organization_id=?";
        try {
            initTableValues();
            PreparedStatement stmt = null;
            if (tgtPH()) {
                stmt = conn.prepareStatement(
                    "upsert into " +
                    "PRODUCT_METRICS(" +
                    "    ORGANIZATION_ID, " +
                    "    DATE, " +
                    "    FEATURE, " +
                    "    UNIQUE_USERS," +
                    "    TRANSACTIONS) " +
                    "VALUES (?, ?, ?, ?, ?)");
                stmt.setString(1, tenantId);
                stmt.setDate(2, D1);
                stmt.setString(3, F1);
                stmt.setInt(4, 10);
                stmt.setInt(5, 200); // Change TRANSACTIONS from 100 to 200
            } else if (tgtTR()) {
                stmt = conn.prepareStatement(
                    "upsert into " +
                    "PRODUCT_METRICS(" +
                    "    ORGANIZATION_ID, " +
                    "    DATE1, " +
                    "    FEATURE, " +
                    "    UNIQUE_USERS," +
                    "    TRANSACTIONS) " +
                    "VALUES (?, ?, ?, ?, ?)");
                stmt.setString(1, tenantId);
                stmt.setDate(2, D1);
                stmt.setString(3, F1);
                stmt.setInt(4, 10);
                stmt.setInt(5, 200); // Change TRANSACTIONS from 100 to 200
            } else if (tgtSQ()) {
                stmt = conn.prepareStatement(
                    "update PRODUCT_METRICS set TRANSACTIONS = ? " +
                    "where ORGANIZATION_ID = ? and DATE1 = ? and " + 
                    "FEATURE = ? and UNIQUE_USERS = ?");
                stmt.setInt(1, 200); // Change TRANSACTIONS from 100 to 200
                stmt.setString(2, tenantId);
                stmt.setDate(3, D1);
                stmt.setString(4, F1);
                stmt.setInt(5, 10);
            }
            stmt.execute();

            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(2200, rs.getInt(1));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testSumUngroupedHavingAggregation() throws Exception {
        printTestDescription();

        String query = "SELECT sum(unique_users),sum(cpu_utilization),sum(transactions),sum(db_utilization),sum(response_time) FROM PRODUCT_METRICS WHERE organization_id=? HAVING sum(unique_users) > 200 AND sum(db_utilization) > 4.5";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(210, rs.getInt(1));
            if (tgtPH()) assertEquals(BigDecimal.valueOf(14.5), rs.getBigDecimal(2));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(14.5), rs.getBigDecimal(2).setScale(1));
            assertEquals(2100L, rs.getLong(3));
            if (tgtPH()) assertEquals(BigDecimal.valueOf(4.6), rs.getBigDecimal(4));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(4.6), rs.getBigDecimal(4).setScale(1));
            assertEquals(0, rs.getLong(5));
            assertEquals(true, rs.wasNull());
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testSumUngroupedHavingAggregation2() throws Exception {
        printTestDescription();

        String query = "SELECT sum(unique_users),sum(cpu_utilization),sum(transactions),sum(db_utilization),sum(response_time) FROM PRODUCT_METRICS WHERE organization_id=? HAVING sum(unique_users) > 200 AND sum(db_utilization) > 5";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testMinUngroupedAggregation() throws Exception {
        printTestDescription();
        String query = "SELECT min(unique_users),min(cpu_utilization),min(transactions),min(db_utilization),min('X'),min(response_time) FROM PRODUCT_METRICS WHERE organization_id=?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(10, rs.getInt(1));
            if (tgtPH()) assertEquals(BigDecimal.valueOf(0.5), rs.getBigDecimal(2));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(0.5), rs.getBigDecimal(2).setScale(1));
            assertEquals(100L, rs.getLong(3));
            if (tgtPH()) assertEquals(BigDecimal.valueOf(0.2), rs.getBigDecimal(4));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(0.2), rs.getBigDecimal(4).setScale(1));
            assertEquals("X", rs.getString(5));            
            assertEquals(0, rs.getLong(6));
            assertEquals(true, rs.wasNull());
            assertFalse(rs.next());
        } finally {
            
        }
    }

    @Test
    public void testMinUngroupedAggregation1() throws Exception {
        printTestDescription();

        String query = "SELECT min(cpu_utilization) FROM PRODUCT_METRICS WHERE organization_id=?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(BigDecimal.valueOf(0.5), rs.getBigDecimal(1));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(0.5), rs.getBigDecimal(1).setScale(1));
            assertFalse(rs.next());
        } finally {
            
        }
    }

    @Test
    public void testMaxUngroupedAggregation() throws Exception {
        printTestDescription();

        String query = "SELECT max(unique_users),max(cpu_utilization),max(transactions),max(db_utilization),max('X'),max(response_time) FROM PRODUCT_METRICS WHERE organization_id=?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(60, rs.getInt(1));
            if (tgtPH()) assertEquals(BigDecimal.valueOf(4), rs.getBigDecimal(2));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(4), rs.getBigDecimal(2).setScale(0));
            assertEquals(600L, rs.getLong(3));
            if (tgtPH()) assertEquals(BigDecimal.valueOf(1.4), rs.getBigDecimal(4));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(1.4), rs.getBigDecimal(4).setScale(1));
            assertEquals("X", rs.getString(5));            
            assertEquals(0, rs.getLong(6));
            assertEquals(true, rs.wasNull());
            assertFalse(rs.next());
        } finally {
            
        }
    }

    @Test
    public void testMaxGroupedAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT feature,max(transactions) FROM PRODUCT_METRICS WHERE organization_id=? GROUP BY feature";
        else if (tgtSQ()||tgtTR()) query = "SELECT feature,max(transactions) FROM PRODUCT_METRICS WHERE organization_id=? GROUP BY feature order by 1";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(F1,rs.getString(1));
            assertEquals(600,rs.getInt(2));
            assertTrue(rs.next());
            assertEquals(F2,rs.getString(1));
            assertEquals(400,rs.getInt(2));
            assertTrue(rs.next());
            assertEquals(F3,rs.getString(1));
            assertEquals(500,rs.getInt(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }

    @Test
    public void testCountUngroupedAggregation() throws Exception {
        printTestDescription();

        String query = "SELECT count(1) FROM PRODUCT_METRICS";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(6, rs.getLong(1));
            assertFalse(rs.next());
        } finally {
            
        }
    }

    @Test
    public void testCountColumnUngroupedAggregation() throws Exception {
        printTestDescription();

        String query = "SELECT count(io_time),sum(io_time),avg(io_time) FROM PRODUCT_METRICS WHERE organization_id=?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(3, rs.getLong(1));
            assertEquals(11000, rs.getLong(2));
            // Scale is automatically capped at 4 if no scale is specified. 
            if (tgtPH()) assertEquals(new BigDecimal("3666.6666"), rs.getBigDecimal(3));
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("3666"), rs.getBigDecimal(3).setScale(0));
            assertFalse(rs.next());
        } finally {
            
        }
    }

    @Test
    public void testNoRowsUngroupedAggregation() throws Exception {
        printTestDescription();

        String query = "SELECT count(io_time),sum(io_time),avg(io_time),count(1) FROM PRODUCT_METRICS WHERE organization_id=? AND feature > ?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2,F3);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(0, rs.getLong(1));
            assertFalse(rs.wasNull());
            assertEquals(0, rs.getLong(2));
            assertTrue(rs.wasNull());
            assertEquals(null, rs.getBigDecimal(3));
            assertEquals(0, rs.getLong(4));
            assertFalse(rs.wasNull());
            assertFalse(rs.next());
        } finally {
            
        }
    }

    @Test
    public void testAvgUngroupedAggregation() throws Exception {
        printTestDescription();

        String query = "SELECT avg(unique_users) FROM PRODUCT_METRICS WHERE organization_id=?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(BigDecimal.valueOf(35), rs.getBigDecimal(1));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testAvgUngroupedAggregationOnValueField() throws Exception {
        printTestDescription();

        String query = "SELECT AVG(DB_UTILIZATION) FROM PRODUCT_METRICS WHERE organization_id=?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);

            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            // The column is defined as decimal(31,10), so the value is capped at 10 decimal points.
            assertEquals(new BigDecimal("0.7666666666"), rs.getBigDecimal(1));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    /**
     * Test aggregate query with rownum limit that does not explicity contain a count(1) as a select expression
     * @throws Exception
     */
    @Test
    public void testLimitSumUngroupedAggregation() throws Exception {
        printTestDescription();

        // No count(1) aggregation, so it will get added automatically
        // LIMIT has no effect, since it's applied at the end and we'll always have a single row for ungrouped aggregation
        String query = null;
        if (tgtPH()||tgtTR()) query = "SELECT sum(unique_users),sum(cpu_utilization),sum(transactions),sum(db_utilization),sum(response_time) feature FROM PRODUCT_METRICS WHERE organization_id=? LIMIT 3";
        else if (tgtSQ()) query = "SELECT [first 3] sum(unique_users),sum(cpu_utilization),sum(transactions),sum(db_utilization),sum(response_time) feature FROM PRODUCT_METRICS WHERE organization_id=?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(210, rs.getInt(1));
            if (tgtPH()) assertEquals(BigDecimal.valueOf(14.5), rs.getBigDecimal(2));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(14.5), rs.getBigDecimal(2).setScale(1));
            assertEquals(2100L, rs.getLong(3));
            if (tgtPH()) assertEquals(BigDecimal.valueOf(4.6), rs.getBigDecimal(4));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(4.6), rs.getBigDecimal(4).setScale(1));
            assertEquals(0, rs.getLong(5));
            assertEquals(true, rs.wasNull());
            assertFalse(rs.next());
        } finally {
            
        }
    }

    /**
     * Test grouped aggregation query with a mix of aggregated data types
     * @throws Exception
     */
    @Test
    public void testSumGroupedAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT feature,sum(unique_users),sum(cpu_utilization),sum(transactions),sum(db_utilization),sum(response_time),count(1) c FROM PRODUCT_METRICS WHERE organization_id=? AND feature < ? GROUP BY feature";
        else if (tgtSQ()||tgtTR()) query = "SELECT feature,sum(unique_users) sumcol,sum(cpu_utilization),sum(transactions),sum(db_utilization),sum(response_time),count(1) c FROM PRODUCT_METRICS WHERE organization_id=? AND feature < ? GROUP BY feature";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, F3);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(F1, rs.getString("feature"));
            if (tgtPH()) assertEquals(120, rs.getInt("sum(unique_users)"));
            else if (tgtSQ()||tgtTR()) assertEquals(120, rs.getInt("sumcol"));
            if (tgtPH()) assertEquals(BigDecimal.valueOf(8), rs.getBigDecimal(3));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(8), rs.getBigDecimal(3).setScale(0));
            assertEquals(1200L, rs.getLong(4));
            if (tgtPH()) assertEquals(BigDecimal.valueOf(2.6), rs.getBigDecimal(5));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(2.6), rs.getBigDecimal(5).setScale(1));
            assertEquals(0, rs.getLong(6));
            assertEquals(true, rs.wasNull());
            assertEquals(4, rs.getLong("c"));
            assertTrue(rs.next());
            assertEquals(F2, rs.getString("feature"));
            assertEquals(40, rs.getInt(2));
            if (tgtPH()) assertEquals(BigDecimal.valueOf(3), rs.getBigDecimal(3));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(3), rs.getBigDecimal(3).setScale(0));
            assertEquals(400L, rs.getLong(4));
            if (tgtPH()) assertEquals(BigDecimal.valueOf(0.8), rs.getBigDecimal(5));
            else if (tgtSQ()||tgtTR()) assertEquals(BigDecimal.valueOf(0.8), rs.getBigDecimal(5).setScale(1));
            assertEquals(0, rs.getLong(6));
            assertEquals(true, rs.wasNull());
            assertEquals(1, rs.getLong("c"));
            assertFalse(rs.next());
        } finally {
            
        }
    }

    @Test
    public void testDegenerateAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT count(1), feature FROM PRODUCT_METRICS WHERE organization_id=? AND date >= to_date(?) AND date <= to_date(?) GROUP BY feature";
        else if (tgtSQ()||tgtTR()) query = "SELECT count(1), feature FROM PRODUCT_METRICS WHERE organization_id=? AND date1 >= ? AND date1 <= ? GROUP BY feature";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            // Start date larger than end date
            statement.setString(2, DS4);
            statement.setString(3, DS2);
            ResultSet rs = statement.executeQuery();
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    /**
     * Query with multiple > expressions on continquous PK columns
     * @throws Exception
     */
    @Test
    public void testFeatureDateRangeAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT feature,unique_users FROM PRODUCT_METRICS WHERE organization_id=? AND date >= to_date(?) AND feature > ?";
        else if (tgtSQ()||tgtTR()) query = "SELECT feature,unique_users FROM PRODUCT_METRICS WHERE organization_id=? AND date1 >= ? AND feature > ?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, DS2);
            statement.setString(3, F2);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(F3, rs.getString(1));
            assertEquals(50, rs.getInt(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    /**
     * Query with non contiguous PK column expressions (i.e. no expresion for DATE)
     * @throws Exception
     */
    @Test
    public void testFeatureGTAggregation() throws Exception {
        printTestDescription();

        String query = "SELECT feature,unique_users FROM PRODUCT_METRICS WHERE organization_id=? AND feature > ?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, F2);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(F3, rs.getString(1));
            assertEquals(50, rs.getInt(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testFeatureGTEAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT feature,unique_users FROM PRODUCT_METRICS WHERE organization_id=? AND feature >= ?";
        else if (tgtSQ()||tgtTR()) query = "SELECT feature,unique_users FROM PRODUCT_METRICS WHERE organization_id=? AND feature >= ? order by 1";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, F2);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(F2, rs.getString(1));
            assertEquals(40, rs.getInt(2));
            assertTrue(rs.next());
            assertEquals(F3, rs.getString(1));
            assertEquals(50, rs.getInt(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testFeatureEQAggregation() throws Exception {
        printTestDescription();

        String query = "SELECT feature,unique_users FROM PRODUCT_METRICS WHERE organization_id=? AND feature = ?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, F2);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(F2, rs.getString(1));
            assertEquals(40, rs.getInt(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }

    
    @Test
    public void testFeatureLTAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT feature,unique_users FROM PRODUCT_METRICS WHERE organization_id=? AND feature < ?";
        else if (tgtSQ()||tgtTR()) query = "SELECT feature,unique_users FROM PRODUCT_METRICS WHERE organization_id=? AND feature < ? order by 2";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, F2);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(F1, rs.getString(1));
            assertEquals(10, rs.getInt(2));
            assertTrue(rs.next());
            assertEquals(F1, rs.getString(1));
            assertEquals(20, rs.getInt(2));
            assertTrue(rs.next());
            assertEquals(F1, rs.getString(1));
            assertEquals(30, rs.getInt(2));
            assertTrue(rs.next());
            assertEquals(F1, rs.getString(1));
            assertEquals(60, rs.getInt(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }

    @Test
    public void testFeatureLTEAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT feature,unique_users FROM PRODUCT_METRICS WHERE organization_id=? AND feature <= ?";
        else if (tgtSQ()||tgtTR()) query = "SELECT feature,unique_users FROM PRODUCT_METRICS WHERE organization_id=? AND feature <= ? order by 2";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setString(2, F2);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(F1, rs.getString(1));
            assertEquals(10, rs.getInt(2));
            assertTrue(rs.next());
            assertEquals(F1, rs.getString(1));
            assertEquals(20, rs.getInt(2));
            assertTrue(rs.next());
            assertEquals(F1, rs.getString(1));
            assertEquals(30, rs.getInt(2));
            assertTrue(rs.next());
            assertEquals(F2, rs.getString(1));
            assertEquals(40, rs.getInt(2));
            assertTrue(rs.next());
            assertEquals(F1, rs.getString(1));
            assertEquals(60, rs.getInt(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    @Test
    public void testOrderByNonAggregation() throws Exception {
        printTestDescription();

        initTableValues();
        String query = null;
        if (tgtPH()) query = "SELECT date, transactions t FROM PRODUCT_METRICS WHERE organization_id=? AND unique_users <= 30 ORDER BY t DESC LIMIT 2";
        else if (tgtTR()) query = "SELECT date1, transactions t FROM PRODUCT_METRICS WHERE organization_id=? AND unique_users <= 30 ORDER BY t DESC LIMIT 2";
        else if (tgtSQ()) query = "SELECT [first 2] date1, transactions t FROM PRODUCT_METRICS WHERE organization_id=? AND unique_users <= 30 ORDER BY t DESC";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(D3.getTime(), rs.getDate(1).getTime());
            else if (tgtSQ()||tgtTR()) assertEquals(D3.getTime(), rs.getTimestamp(1).getTime());
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(D2.getTime(), rs.getDate(1).getTime());
            else if (tgtSQ()||tgtTR()) assertEquals(D2.getTime(), rs.getTimestamp(1).getTime());
            assertFalse(rs.next());
        } finally {
            
        }
    }

    @Test
    public void testOrderByUngroupedAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT sum(unique_users) sumUsers,count(feature) " +
                       "FROM PRODUCT_METRICS " + 
                       "WHERE organization_id=? " +
                       "ORDER BY 100000-sumUsers";
        else if (tgtSQ()||tgtTR()) query = "SELECT sum(unique_users) sumUsers,count(feature) " +
                       "FROM PRODUCT_METRICS " +
                       "WHERE organization_id=? " +
                       "ORDER BY sumUsers";

        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(210, rs.getInt(1));
            assertEquals(6, rs.getInt(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testOrderByGroupedAggregation() throws Exception {        
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT feature,sum(unique_users) s,count(feature),round(date,'hour',1) r " +
                       "FROM PRODUCT_METRICS " + 
                       "WHERE organization_id=? " +
                       "GROUP BY feature, r " +
                       "ORDER BY 1 desc,feature desc,r,feature,s";
        else if (tgtSQ()||tgtTR()) query = "SELECT feature,sum(unique_users) s,count(feature),date_trunc('hour',date_add(date1,interval '30' minute)) r " +
                       "FROM PRODUCT_METRICS " +
                       "WHERE organization_id=? " +
                       "GROUP BY feature, date_trunc('hour',date_add(date1,interval '30' minute)) " +
                       "ORDER BY 1 desc,feature desc,r,feature,s";

        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            
            Object[][] expected = {
                    {F3, 50, 1},
                    {F2, 40, 1},
                    {F1, 30, 2},
                    {F1, 30, 1},
                    {F1, 60, 1},
            };

            for (int i = 0; i < expected.length; i++) {
                assertTrue(rs.next());
                assertEquals(expected[i][0], rs.getString(1));
                assertEquals(expected[i][1], rs.getInt(2));
                assertEquals(expected[i][2], rs.getInt(3));
            }
            assertFalse(rs.next());
        } finally {
            
        }
    }

    @Test
    public void testOrderByUnprojected() throws Exception {        
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT sum(unique_users), count(feature) c " +
                       "FROM PRODUCT_METRICS " + 
                       "WHERE organization_id=? " +
                       "GROUP BY feature " +
                       "ORDER BY 100-c,feature";
        // TRAF: (1) 100-c is not allowed in ORDER BY. (2) When there is a GROUP BY, DISTINCT, or aggregate function, ORDER BY can only use columns explicitly SELECTed, otherwise it is a 4120 error. We need to put feature in the SELECT list.
        else if (tgtSQ()||tgtTR()) query = "SELECT sum(unique_users), count(feature) c, feature " +
                       "FROM PRODUCT_METRICS " +
                       "WHERE organization_id=? " +
                       "GROUP BY feature " +
                       "ORDER BY c,feature"; 

        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            
            int[] expected = null;
            if (tgtPH()) expected = new int[]{120, 40, 50};
            else if (tgtSQ()||tgtTR()) expected = new int[]{40, 50, 120};
            for (int i = 0; i < expected.length; i++) {
                assertTrue(rs.next());
                assertEquals(expected[i], rs.getInt(1));
            }
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testOrderByNullColumns__nullsFirst() throws Exception {
        printTestDescription();

        helpTestOrderByNullColumns(true);
    }
    
    @Test
    public void testOrderByNullColumns__nullsLast() throws Exception {
        printTestDescription();

        helpTestOrderByNullColumns(false);
    }
    
    private void helpTestOrderByNullColumns(boolean nullsFirst) throws Exception {
        String query = null;
        if (tgtPH()) query = "SELECT region " +
                       "FROM PRODUCT_METRICS " + 
                       "WHERE organization_id=? " +
                       "GROUP BY region " +
                       "ORDER BY region nulls " + (nullsFirst ? "first" : "last");
        else if (tgtSQ()||tgtTR()) query = "SELECT region " +
                       "FROM PRODUCT_METRICS " +
                       "WHERE organization_id=? " +
                       "GROUP BY region " +
                       "ORDER BY region " + (nullsFirst ? "desc" : "asc");

        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            
            List<String> expected = null;
            if (tgtPH()) {
                expected = Lists.newArrayList(null, R1, R2);
                Ordering<String> regionOrdering = Ordering.natural();
                regionOrdering = nullsFirst ? regionOrdering.nullsFirst() : regionOrdering.nullsLast();
                Collections.sort(expected, regionOrdering);
            } else {
                expected = (nullsFirst ? Lists.newArrayList(null, R2, R1)
                            : Lists.newArrayList(R1, R2, null));
            }

            for (String region : expected) {
                assertTrue(rs.next());
                assertEquals(region, rs.getString(1));
            }
            assertFalse(rs.next());
        } finally {
            
        }
    }

    /**
     * Test to repro ArrayIndexOutOfBoundException that happens during filtering in BinarySubsetComparator
     * only after a flush is performed
     * @throws Exception
     */
    @Test
    public void testFilterOnTrailingKeyColumn() throws Exception {
        printTestDescription();

        // TRAF HBaseAdmin admin = null;
        try {
            initTableValues();
            // TRAF admin = conn.unwrap(PhoenixConnection.class).getQueryServices().getAdmin();
            // TRAF admin.flush(SchemaUtil.getTableName(Bytes.toBytes(PRODUCT_METRICS_NAME)));
            String query = "SELECT SUM(TRANSACTIONS) FROM " + PRODUCT_METRICS_NAME + " WHERE FEATURE=?";
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, F1);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(1200, rs.getInt(1));
        } finally {
            // TRAF if (admin != null) admin.close();
        }	
    }
    
    @Test
    public void testFilterOnTrailingKeyColumn2() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT organization_id, date, feature FROM PRODUCT_METRICS WHERE substr(organization_id,1,3)=? AND date > to_date(?)";
        else if (tgtSQ()||tgtTR()) query = "SELECT organization_id, date1, feature FROM PRODUCT_METRICS WHERE substr(organization_id,1,3)=? AND date1 > ? order by 1";

        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId.substring(0,3));
            statement.setString(2, DS4);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(tenantId, rs.getString(1));
            else if (tgtSQ()||tgtTR()) assertEquals(tenantId, rs.getString(1).trim());
            if (tgtPH()) assertEquals(D5.getTime(), rs.getDate(2).getTime());
            else if (tgtSQ()||tgtTR()) assertEquals(D5.getTime(), rs.getTimestamp(2).getTime());
            assertEquals(F3, rs.getString(3));
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(tenantId, rs.getString(1));
            else if (tgtSQ()||tgtTR()) assertEquals(tenantId, rs.getString(1).trim());
            if (tgtPH()) assertEquals(D6.getTime(), rs.getDate(2).getTime());
            else if (tgtSQ()||tgtTR()) assertEquals(D6.getTime(), rs.getTimestamp(2).getTime());
            assertEquals(F1, rs.getString(3));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testSubstringNotEqual() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT organization_id, date, feature FROM PRODUCT_METRICS WHERE organization_id=? AND date > to_date(?)";
        else if (tgtSQ()||tgtTR()) query = "SELECT organization_id, date1, feature FROM PRODUCT_METRICS WHERE organization_id=? AND date1 > ?";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId.substring(0,3));
            statement.setString(2, DS4);
            ResultSet rs = statement.executeQuery();
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testKeyOrderedAggregation1() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT date, sum(UNIQUE_USERS) FROM PRODUCT_METRICS WHERE date > to_date(?) GROUP BY organization_id, date";
        else if (tgtSQ()||tgtTR()) query = "SELECT date1, sum(UNIQUE_USERS) FROM PRODUCT_METRICS WHERE date1 > ? GROUP BY organization_id, date1 order by 2";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, DS4);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(D5, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(D5, rs.getTimestamp(1));
            assertEquals(50, rs.getInt(2));
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(D6, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(D6, rs.getTimestamp(1));
            assertEquals(60, rs.getInt(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testKeyOrderedAggregation2() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT date, sum(UNIQUE_USERS) FROM PRODUCT_METRICS WHERE date < to_date(?) GROUP BY organization_id, date";
        else if (tgtSQ()||tgtTR()) query = "SELECT date1, sum(UNIQUE_USERS) FROM PRODUCT_METRICS WHERE date1 < ? GROUP BY organization_id, date1 order by 2";

        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, DS4);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(D1, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(D1, rs.getTimestamp(1));
            assertEquals(10, rs.getInt(2));
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(D2, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(D2, rs.getTimestamp(1));
            assertEquals(20, rs.getInt(2));
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(D3, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(D3, rs.getTimestamp(1));
            assertEquals(30, rs.getInt(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testKeyOrderedRoundAggregation1() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT round(date,'HOUR'), sum(UNIQUE_USERS) FROM PRODUCT_METRICS WHERE date < to_date(?) GROUP BY organization_id, round(date,'HOUR')";
        else if (tgtSQ()||tgtTR()) query = "SELECT date_trunc('hour',date_add(date1,interval '30' minute)), sum(UNIQUE_USERS) FROM PRODUCT_METRICS WHERE date1 < ? GROUP BY organization_id, date_trunc('hour',date_add(date1,interval '30' minute)) order by 1";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, DS4);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(ROUND_1HR, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(ROUND_1HR, rs.getTimestamp(1));
            assertEquals(30, rs.getInt(2));
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(ROUND_2HR, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(ROUND_2HR, rs.getTimestamp(1));
            assertEquals(30, rs.getInt(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testKeyOrderedRoundAggregation2() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT round(date,'HOUR'), sum(UNIQUE_USERS) FROM PRODUCT_METRICS WHERE date <= to_date(?) GROUP BY organization_id, round(date,'HOUR')";
        else if (tgtSQ()||tgtTR()) query = "SELECT date_trunc('hour',date_add(date1,interval '30' minute)), sum(UNIQUE_USERS) FROM PRODUCT_METRICS WHERE date1 <= ? GROUP BY organization_id, date_trunc('hour',date_add(date1,interval '30' minute)) order by 1";

        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, DS4);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(ROUND_1HR, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(ROUND_1HR, rs.getTimestamp(1));
            assertEquals(30, rs.getInt(2));
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(ROUND_2HR, rs.getDate(1));
            else if (tgtSQ()||tgtTR()) assertEquals(ROUND_2HR, rs.getTimestamp(1));
            assertEquals(70, rs.getInt(2));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testEqualsRound() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT feature FROM PRODUCT_METRICS WHERE organization_id = ? and trunc(date,'DAY')=?"; 
        else if (tgtSQ()||tgtTR()) query = "SELECT feature FROM PRODUCT_METRICS WHERE organization_id = ? and date_trunc('DAY', date1)=?";
        try {
            Date startDate = new Date(System.currentTimeMillis());
            Date equalDate = new Date((startDate.getTime() + 2 * MILLIS_IN_DAY)/ MILLIS_IN_DAY*MILLIS_IN_DAY);
            initDateTableValues(startDate, 1.0);
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setDate(2, equalDate);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("C", rs.getString(1));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testDateSubtractionCompareNumber() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT feature FROM PRODUCT_METRICS WHERE organization_id = ? and ? - date > 3"; 
        else if (tgtSQ()||tgtTR()) query = "SELECT feature FROM PRODUCT_METRICS WHERE organization_id = ? and ? - date1 > INTERVAL '3' DAY order by 1";
        try {
            Date startDate = new Date(System.currentTimeMillis());
            Date endDate = new Date(startDate.getTime() + 6 * MILLIS_IN_DAY);
            initDateTableValues(startDate);
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setDate(2, endDate);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("A", rs.getString(1));
            assertTrue(rs.next());
            assertEquals("B", rs.getString(1));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testDateSubtractionLongToDecimalCompareNumber() throws Exception {
        printTestDescription();
        String query = null;
        if (tgtPH()) query = "SELECT feature FROM PRODUCT_METRICS WHERE organization_id = ? and ? - date - 1.5 > 3"; 
        else if (tgtSQ()||tgtTR()) query = "SELECT feature FROM PRODUCT_METRICS WHERE organization_id = ? and ? - date1 - (INTERVAL '1' DAY + INTERVAL '12' HOUR) > INTERVAL '3' DAY order by 1";
        try {
            Date startDate = new Date(System.currentTimeMillis());
            Date endDate = new Date(startDate.getTime() + 9 * MILLIS_IN_DAY);
            initDateTableValues(startDate);
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setDate(2, endDate);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("A", rs.getString(1));
            assertTrue(rs.next());
            assertEquals("B", rs.getString(1));
            assertTrue(rs.next());
            assertEquals("C", rs.getString(1));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testDateSubtractionCompareDate() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT feature FROM PRODUCT_METRICS WHERE organization_id = ? and date - 1 >= ?"; 
        else if (tgtSQ()||tgtTR()) query = "SELECT feature FROM PRODUCT_METRICS WHERE organization_id = ? and date1 - INTERVAL '1' DAY >= ?";
        try {
            Date startDate = new Date(System.currentTimeMillis());
            Date endDate = new Date(startDate.getTime() + 9 * MILLIS_IN_DAY);
            initDateTableValues(startDate);
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setDate(2, endDate);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("F", rs.getString(1));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testDateAddCompareDate() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT feature FROM PRODUCT_METRICS WHERE organization_id = ? and date + 1 >= ?"; 
        else if (tgtSQ()||tgtTR()) query = "SELECT feature FROM PRODUCT_METRICS WHERE organization_id = ? and date1 + INTERVAL '1' DAY >= ? order by 1";
        try {
            Date startDate = new Date(System.currentTimeMillis());
            Date endDate = new Date(startDate.getTime() + 8 * MILLIS_IN_DAY);
            initDateTableValues(startDate);
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setDate(2, endDate);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("E", rs.getString(1));
            assertTrue(rs.next());
            assertEquals("F", rs.getString(1));
            assertFalse(rs.next());
        } finally {
            
        }
    }
    
    @Test
    public void testCurrentDate() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT feature FROM PRODUCT_METRICS WHERE organization_id = ? and date - current_date() > 8"; 
        else if (tgtSQ()||tgtTR()) query = "SELECT feature FROM PRODUCT_METRICS WHERE organization_id = ? and date1 - CURRENT_TIMESTAMP > INTERVAL '8' DAY";
        try {
            Date startDate = new Date(System.currentTimeMillis());
            initDateTableValues(startDate);
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) {
                assertTrue(rs.next());
                assertEquals("F", rs.getString(1));
                assertFalse(rs.next());
            } else if (tgtSQ()||tgtTR()) {
                /* Most java time function returns UTC.  We also run 
                 * phoenix_test with -Duser.timezone=UTC to force UTC.  But 
                 * CURRENT_TIMESTAMP uses the local machine time zone (say 
                 * PST), which is not always UTC.  Skip this comparision.
                 */
            }
        } finally {
            
        }
    }
    
    @Test
    public void testCurrentTime() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT feature FROM PRODUCT_METRICS WHERE organization_id = ? and date - current_time() > 8"; 
        else if (tgtSQ()||tgtTR()) query = "SELECT feature FROM PRODUCT_METRICS WHERE organization_id = ? and date1 - CURRENT_TIMESTAMP > INTERVAL '8' DAY";
        try {
            Date startDate = new Date(System.currentTimeMillis());
            initDateTableValues(startDate);
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            if (tgtPH()) {
                assertTrue(rs.next());
                assertEquals("F", rs.getString(1));
                assertFalse(rs.next());
            } else if (tgtSQ()||tgtTR()) {
                /* Most java time function returns UTC.  We also run
                 * phoenix_test with -Duser.timezone=UTC to force UTC.  But
                 * CURRENT_TIMESTAMP uses the local machine time zone (say
                 * PST), which is not always UTC.  Skip this comparision.
                 */
            }
        } finally {
            
        }
    }
    
    @Test
    public void testTruncateNotTraversableToFormScanKey() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT feature FROM PRODUCT_METRICS WHERE organization_id = ? and TRUNC(date,'DAY') <= ?";
        else if (tgtSQ()||tgtTR()) query = "SELECT feature FROM PRODUCT_METRICS WHERE organization_id = ? and DATE_TRUNC('DAY', date1) <= ? order by 1"; 
        try {
            Date startDate = toDate("2013-01-01 00:00:00");
            initDateTableValues(startDate, 0.5);
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            statement.setDate(2, new Date(startDate.getTime() + (long)(MILLIS_IN_DAY * 0.25)));
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals("A", rs.getString(1));
            assertTrue(rs.next());
            assertEquals("B", rs.getString(1));
            assertFalse(rs.next());
        } finally {
            
        }
    }
}
