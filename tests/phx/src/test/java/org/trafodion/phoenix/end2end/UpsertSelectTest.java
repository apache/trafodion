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


public class UpsertSelectTest extends BaseTest {

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table IntKeyTest", "table phoenix_test", "table " + ATABLE_NAME, "table " + CUSTOM_ENTITY_DATA_FULL_NAME, "table " + PTSDB_NAME));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */


    @Test
    public void testUpsertSelect() throws Exception {
        printTestDescription();

        initATableValues();
        createTestTable(CUSTOM_ENTITY_DATA_FULL_NAME);
        conn.setAutoCommit(true);
        String upsert = null;
        if (tgtPH()||tgtTR()) upsert = "UPSERT INTO " + CUSTOM_ENTITY_DATA_FULL_NAME + "(custom_entity_data_id, key_prefix, organization_id, created_by) " +
            "SELECT substr(entity_id, 4), substr(entity_id, 1, 3), organization_id, a_string  FROM ATABLE WHERE a_string = ?";
        else if (tgtSQ()) upsert = "INSERT INTO " + CUSTOM_ENTITY_DATA_FULL_NAME + "(custom_entity_data_id, key_prefix, organization_id, created_by) " +
            "SELECT substr(entity_id, 4), substr(entity_id, 1, 3), organization_id, a_string  FROM ATABLE WHERE a_string = ?";
        PreparedStatement upsertStmt = conn.prepareStatement(upsert);
        upsertStmt.setString(1, A_VALUE);
        int rowsInserted = upsertStmt.executeUpdate();
        assertEquals(4, rowsInserted);
       
        String query = null;
        if (tgtPH()) query = "SELECT key_prefix, substr(custom_entity_data_id, 1, 1), created_by FROM " + CUSTOM_ENTITY_DATA_FULL_NAME + " WHERE organization_id = ?";
        else if (tgtSQ()||tgtTR()) query = "SELECT key_prefix, substr(custom_entity_data_id, 1, 1), created_by FROM " + CUSTOM_ENTITY_DATA_FULL_NAME + " WHERE organization_id = ? order by 2";
        PreparedStatement statement = conn.prepareStatement(query);
        statement.setString(1, tenantId);
        ResultSet rs = statement.executeQuery();
        
        assertTrue (rs.next());
        assertEquals("00A", rs.getString(1));
        assertEquals("1", rs.getString(2));
        assertEquals(A_VALUE, rs.getString(3));
        
        assertTrue (rs.next());
        assertEquals("00A", rs.getString(1));
        assertEquals("2", rs.getString(2));
        assertEquals(A_VALUE, rs.getString(3));
        
        assertTrue (rs.next());
        assertEquals("00A", rs.getString(1));
        assertEquals("3", rs.getString(2));
        assertEquals(A_VALUE, rs.getString(3));
        
        assertTrue (rs.next());
        assertEquals("00A", rs.getString(1));
        assertEquals("4", rs.getString(2));
        assertEquals(A_VALUE, rs.getString(3));

        assertFalse(rs.next());

        // Test UPSERT through coprocessor
        conn.setAutoCommit(true);
        if (tgtPH()||tgtTR()) upsert = "UPSERT INTO " + CUSTOM_ENTITY_DATA_FULL_NAME + "(custom_entity_data_id, key_prefix, organization_id, last_update_by, division) " +
            "SELECT custom_entity_data_id, key_prefix, organization_id, created_by, 1.0  FROM " + CUSTOM_ENTITY_DATA_FULL_NAME + " WHERE organization_id = ?";
        else if (tgtSQ()) upsert = "UPDATE " + CUSTOM_ENTITY_DATA_FULL_NAME + " SET (last_update_by, division) = (created_by, 1.0) WHERE organization_id = ?";
        upsertStmt = conn.prepareStatement(upsert);
        upsertStmt.setString(1, tenantId);
        assertEquals(4, upsertStmt.executeUpdate());

        if (tgtPH()) query = "SELECT key_prefix, substr(custom_entity_data_id, 1, 1), created_by, last_update_by, division FROM " + CUSTOM_ENTITY_DATA_FULL_NAME + " WHERE organization_id = ?";
        else if (tgtSQ()||tgtTR()) query = "SELECT key_prefix, substr(custom_entity_data_id, 1, 1), created_by, last_update_by, division FROM " + CUSTOM_ENTITY_DATA_FULL_NAME + " WHERE organization_id = ? order by 2";
        statement = conn.prepareStatement(query);
        statement.setString(1, tenantId);
        rs = statement.executeQuery();
       
        assertTrue (rs.next());
        assertEquals("00A", rs.getString(1));
        assertEquals("1", rs.getString(2));
        assertEquals(A_VALUE, rs.getString(3));
        assertEquals(A_VALUE, rs.getString(4));
        assertTrue(BigDecimal.valueOf(1.0).compareTo(rs.getBigDecimal(5)) == 0);
        
        assertTrue (rs.next());
        assertEquals("00A", rs.getString(1));
        assertEquals("2", rs.getString(2));
        assertEquals(A_VALUE, rs.getString(3));
        assertEquals(A_VALUE, rs.getString(4));
        assertTrue(BigDecimal.valueOf(1.0).compareTo(rs.getBigDecimal(5)) == 0);
        
        assertTrue (rs.next());
        assertEquals("00A", rs.getString(1));
        assertEquals("3", rs.getString(2));
        assertEquals(A_VALUE, rs.getString(3));
        assertEquals(A_VALUE, rs.getString(4));
        assertTrue(BigDecimal.valueOf(1.0).compareTo(rs.getBigDecimal(5)) == 0);
        
        assertTrue (rs.next());
        assertEquals("00A", rs.getString(1));
        assertEquals("4", rs.getString(2));
        assertEquals(A_VALUE, rs.getString(3));
        assertEquals(A_VALUE, rs.getString(4));
        assertTrue(BigDecimal.valueOf(1.0).compareTo(rs.getBigDecimal(5)) == 0);

        assertFalse(rs.next());
    }

    // TODO: more tests - nullable fixed length last PK column
    @Test
    public void testUpsertSelectEmptyPKColumn() throws Exception {
        printTestDescription();

        initATableValues();
        createTestTable(PTSDB_NAME);
        conn.setAutoCommit(false);
        String upsert = null;
        if (tgtPH()) upsert = "UPSERT INTO " + PTSDB_NAME + "(date, val, host) " +
            "SELECT current_date(), x_integer+2, entity_id FROM ATABLE WHERE a_integer >= ?";
        else if (tgtTR()) upsert = "UPSERT INTO " + PTSDB_NAME + "(date1, val, host1) " +
            "SELECT CURRENT_TIMESTAMP, x_integer+2, entity_id FROM ATABLE WHERE a_integer >= ?";
        else if (tgtSQ()) upsert = "INSERT INTO " + PTSDB_NAME + "(date1, val, host1) " +
            "SELECT CURRENT_TIMESTAMP, x_integer+2, entity_id FROM ATABLE WHERE a_integer >= ?";

        PreparedStatement upsertStmt = conn.prepareStatement(upsert);
        upsertStmt.setInt(1, 6);
        int rowsInserted = upsertStmt.executeUpdate();
        assertEquals(4, rowsInserted);
        conn.commit();
        
        String query = null;
        if (tgtPH()) query = "SELECT inst,host,date,val FROM " + PTSDB_NAME;
        else if (tgtSQ()||tgtTR()) query = "SELECT inst,host1,date1,val FROM " + PTSDB_NAME + " order by 2";
        PreparedStatement statement = conn.prepareStatement(query);
        ResultSet rs = statement.executeQuery();
        
        Date now = null;
        Timestamp now1 = null;
        if (tgtPH()) now = new Date(System.currentTimeMillis());
        else if (tgtSQ()||tgtTR()) now1 = new Timestamp(System.currentTimeMillis());

        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(ROW6, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).before(now) );
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).before(now1) );
        assertEquals(null, rs.getBigDecimal(4));
        
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(ROW7, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).before(now) );
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).before(now1) );
        assertTrue(BigDecimal.valueOf(7).compareTo(rs.getBigDecimal(4)) == 0);
        
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(ROW8, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).before(now) );
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).before(now1) );
        assertTrue(BigDecimal.valueOf(6).compareTo(rs.getBigDecimal(4)) == 0);
        
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(ROW9, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).before(now) );
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).before(now1) );
        assertTrue(BigDecimal.valueOf(5).compareTo(rs.getBigDecimal(4)) == 0);

        assertFalse(rs.next());
        
        conn.setAutoCommit(true);
        if (tgtPH()) upsert = "UPSERT INTO " + PTSDB_NAME + "(date, val, inst) " +
            "SELECT date+1, val*10, host FROM " + PTSDB_NAME;
        else if (tgtTR()) upsert = "UPSERT INTO " + PTSDB_NAME + "(date1, val, inst) " +
            // +1 would mean +1 seconds to the timestamp. If the test runs
            // too fast, the selected value would not become bigger than now1
            // change it to 10 minutes instead
            "SELECT date1+60*10, val*10, host1 FROM " + PTSDB_NAME;
        else if (tgtSQ()) upsert = "INSERT INTO " + PTSDB_NAME + "(date1, val, inst) " +
            // +1 would mean +1 seconds to the timestamp. If the test runs
            // too fast, the selected value would not become bigger than now1
            // change it to 10 minutes instead
            "SELECT date1+60*10, val*10, host1 FROM " + PTSDB_NAME;
        upsertStmt = conn.prepareStatement(upsert);
        rowsInserted = upsertStmt.executeUpdate();
        assertEquals(4, rowsInserted);
        
        Date then = null;
        Timestamp then1 = null;
        if (tgtPH()) then = new Date(now.getTime() + MILLIS_IN_DAY);
        else if (tgtSQ()||tgtTR()) then1 = new Timestamp(now1.getTime() + MILLIS_IN_DAY);

        if (tgtPH()) query = "SELECT host,inst, date,val FROM " + PTSDB_NAME + " where inst is not null";
        else if (tgtSQ()||tgtTR()) query = "SELECT host1,inst, date1,val FROM " + PTSDB_NAME + " where inst is not null order by 2";
        statement = conn.prepareStatement(query);
        
        rs = statement.executeQuery();
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(ROW6, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).after(now) && rs.getDate(3).before(then));
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).after(now1) && rs.getTimestamp(3).before(then1));
        assertEquals(null, rs.getBigDecimal(4));
        
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(ROW7, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).after(now) && rs.getDate(3).before(then));
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).after(now1) && rs.getTimestamp(3).before(then1));
        assertTrue(BigDecimal.valueOf(70).compareTo(rs.getBigDecimal(4)) == 0);
        
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(ROW8, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).after(now) && rs.getDate(3).before(then));
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).after(now1) && rs.getTimestamp(3).before(then1));
        assertTrue(BigDecimal.valueOf(60).compareTo(rs.getBigDecimal(4)) == 0);
        
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(ROW9, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).after(now) && rs.getDate(3).before(then));
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtPH()) assertTrue(rs.getTimestamp(3).after(now1) && rs.getTimestamp(3).before(then1));
        assertTrue(BigDecimal.valueOf(50).compareTo(rs.getBigDecimal(4)) == 0);
        
        assertFalse(rs.next());
        
        // Should just update all values with the same value, essentially just updating the timestamp
        conn.setAutoCommit(true);
        // TRAF: we don't have the timestamp concept, the table will just
        // remain the same.
        if (tgtPH()) {
            upsert = "UPSERT INTO " + PTSDB_NAME + " SELECT * FROM " + PTSDB_NAME;
            upsertStmt = conn.prepareStatement(upsert);
            rowsInserted = upsertStmt.executeUpdate();
            assertEquals(8, rowsInserted);
        }
 
        if (tgtPH()) query = "SELECT * FROM " + PTSDB_NAME ;
        else if (tgtSQ()||tgtTR()) query = "SELECT * FROM " + PTSDB_NAME + " order by 2, 1";
        statement = conn.prepareStatement(query);
        
        rs = statement.executeQuery();
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(ROW6, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).before(now) );
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).before(now1) );
        assertEquals(null, rs.getBigDecimal(4));
        
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(ROW7, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).before(now) );
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).before(now1) );
        assertTrue(BigDecimal.valueOf(7).compareTo(rs.getBigDecimal(4)) == 0);
        
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(ROW8, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).before(now) );
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).before(now1) );
        assertTrue(BigDecimal.valueOf(6).compareTo(rs.getBigDecimal(4)) == 0);
        
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(ROW9, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).before(now) );
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).before(now1) );
        assertTrue(BigDecimal.valueOf(5).compareTo(rs.getBigDecimal(4)) == 0);

        assertTrue (rs.next());
        assertEquals(ROW6, rs.getString(1));
        assertEquals(null, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).after(now) && rs.getDate(3).before(then));
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).after(now1) && rs.getTimestamp(3).before(then1));

        assertEquals(null, rs.getBigDecimal(4));
        
        assertTrue (rs.next());
        assertEquals(ROW7, rs.getString(1));
        assertEquals(null, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).after(now) && rs.getDate(3).before(then));
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).after(now1) && rs.getTimestamp(3).before(then1));
        assertTrue(BigDecimal.valueOf(70).compareTo(rs.getBigDecimal(4)) == 0);
        
        assertTrue (rs.next());
        assertEquals(ROW8, rs.getString(1));
        assertEquals(null, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).after(now) && rs.getDate(3).before(then));
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).after(now1) && rs.getTimestamp(3).before(then1));
        assertTrue(BigDecimal.valueOf(60).compareTo(rs.getBigDecimal(4)) == 0);
        
        assertTrue (rs.next());
        assertEquals(ROW9, rs.getString(1));
        assertEquals(null, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).after(now) && rs.getDate(3).before(then));
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).after(now1) && rs.getTimestamp(3).before(then1));
        assertTrue(BigDecimal.valueOf(50).compareTo(rs.getBigDecimal(4)) == 0);
        
        assertFalse(rs.next());
    }

    @Test
    public void testUpsertSelectForAggAutoCommit() throws Exception {
        printTestDescription();

        testUpsertSelectForAgg(true);
    }
    
    @Test
    public void testUpsertSelectForAgg() throws Exception {
        printTestDescription();

        testUpsertSelectForAgg(false);
    }
    
    private void testUpsertSelectForAgg(boolean autoCommit) throws Exception {
        initATableValues();
        createTestTable(PTSDB_NAME);
        conn.setAutoCommit(autoCommit);
        String upsert = null;
        if (tgtPH()) upsert = "UPSERT INTO " + PTSDB_NAME + "(date, val, host) " +
            "SELECT current_date(), sum(a_integer), a_string FROM ATABLE GROUP BY a_string";
        else if (tgtTR()) upsert = "UPSERT INTO " + PTSDB_NAME + "(date1, val, host1) " +
            "SELECT CURRENT_TIMESTAMP, sum(a_integer), a_string FROM ATABLE GROUP BY a_string";
        else if (tgtSQ()) upsert = "INSERT INTO " + PTSDB_NAME + "(date1, val, host1) " +
            "SELECT CURRENT_TIMESTAMP, sum(a_integer), a_string FROM ATABLE GROUP BY a_string";
        PreparedStatement upsertStmt = conn.prepareStatement(upsert);
        int rowsInserted = upsertStmt.executeUpdate();
        assertEquals(3, rowsInserted);
        if (!autoCommit) {
            conn.commit();
        }
        
        String query = null;
        if (tgtPH()) query = "SELECT inst,host,date,val FROM " + PTSDB_NAME;
        else if (tgtSQ()||tgtTR()) query = "SELECT inst,host1,date1,val FROM " + PTSDB_NAME + " order by 2";
        PreparedStatement statement = conn.prepareStatement(query);
        ResultSet rs = statement.executeQuery();
        Date now = null;
        Timestamp now1 = null;
        if (tgtPH()) now = new Date(System.currentTimeMillis());
        else if (tgtSQ()||tgtTR()) now1 = new Timestamp(System.currentTimeMillis());
 
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(A_VALUE, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).before(now) );
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).before(now1) );
        assertTrue(BigDecimal.valueOf(10).compareTo(rs.getBigDecimal(4)) == 0);
        
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(B_VALUE, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).before(now) );
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).before(now1) );
        assertTrue(BigDecimal.valueOf(26).compareTo(rs.getBigDecimal(4)) == 0);
        
        assertTrue (rs.next());
        assertEquals(null, rs.getString(1));
        assertEquals(C_VALUE, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).before(now) );
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).before(now1) );
        assertTrue(BigDecimal.valueOf(9).compareTo(rs.getBigDecimal(4)) == 0);
        assertFalse(rs.next());
        
        conn.setAutoCommit(autoCommit);
        if (tgtPH()) upsert = "UPSERT INTO " + PTSDB_NAME + "(date, val, host, inst) " +
            "SELECT current_date(), max(val), max(host), 'x' FROM " + PTSDB_NAME;
        else if (tgtTR()) upsert = "UPSERT INTO " + PTSDB_NAME + "(date1, val, host1, inst) " +
            "SELECT CURRENT_TIMESTAMP, max(val), max(host1), 'x' FROM " + PTSDB_NAME;
        else if (tgtSQ()) upsert = "INSERT INTO " + PTSDB_NAME + "(date1, val, host1, inst) " +
            "SELECT CURRENT_TIMESTAMP, max(val), max(host1), 'x' FROM " + PTSDB_NAME;
        upsertStmt = conn.prepareStatement(upsert);
        rowsInserted = upsertStmt.executeUpdate();
        assertEquals(1, rowsInserted);
        if (!autoCommit) {
            conn.commit();
        }
        
        if (tgtPH()) query = "SELECT inst,host,date,val FROM " + PTSDB_NAME + " WHERE inst='x'";
        else if (tgtSQ()||tgtTR()) query = "SELECT inst,host1,date1,val FROM " + PTSDB_NAME + " WHERE inst='x'";
        statement = conn.prepareStatement(query);
        rs = statement.executeQuery();
        if (tgtPH()) now = new Date(System.currentTimeMillis());
        else if (tgtSQ()||tgtTR()) now1 = new Timestamp(System.currentTimeMillis()); 

        assertTrue (rs.next());
        assertEquals("x", rs.getString(1));
        assertEquals(C_VALUE, rs.getString(2));
        if (tgtPH()) assertTrue(rs.getDate(3).before(now) );
        // Skip checking this, target machine time and client machine time are
        // not always in sync.
        // else if (tgtSQ()||tgtTR()) assertTrue(rs.getTimestamp(3).before(now1) );
        assertTrue(BigDecimal.valueOf(26).compareTo(rs.getBigDecimal(4)) == 0);
        assertFalse(rs.next());
        
    }

    @Test
    public void testUpsertSelectLongToInt() throws Exception {
        printTestDescription();

        createTestTable("IntKeyTest");
        String upsert = null;
        if (tgtPH()||tgtTR()) upsert = "UPSERT INTO IntKeyTest VALUES(1)";
        else if (tgtSQ()) upsert = "INSERT INTO IntKeyTest VALUES(1)";
        PreparedStatement upsertStmt = conn.prepareStatement(upsert);
        int rowsInserted = upsertStmt.executeUpdate();
        assertEquals(1, rowsInserted);
        
        if (tgtPH()||tgtTR()) upsert = "UPSERT INTO IntKeyTest select i+1 from IntKeyTest";
        else if (tgtSQ()) upsert = "INSERT INTO IntKeyTest select i+1 from IntKeyTest";
        upsertStmt = conn.prepareStatement(upsert);
        rowsInserted = upsertStmt.executeUpdate();
        assertEquals(1, rowsInserted);
        
        String select = null;
        if (tgtPH()) select = "SELECT i FROM IntKeyTest";
        else if (tgtSQ()||tgtTR()) select = "SELECT i FROM IntKeyTest order by 1";
        ResultSet rs = conn.createStatement().executeQuery(select);
        assertTrue(rs.next());
        assertEquals(1,rs.getInt(1));
        assertTrue(rs.next());
        assertEquals(2,rs.getInt(1));
        assertFalse(rs.next());
    }

    @Test
    public void testUpsertSelectRunOnServer() throws Exception {
        printTestDescription();

        conn.createStatement().execute("create table IntKeyTest (i integer not null primary key desc, j integer)");
        String upsert = null;
        if (tgtPH()||tgtTR()) upsert = "UPSERT INTO IntKeyTest VALUES(1, 1)";
        else if (tgtSQ()) upsert = "INSERT INTO IntKeyTest VALUES(1, 1)";
        PreparedStatement upsertStmt = conn.prepareStatement(upsert);
        int rowsInserted = upsertStmt.executeUpdate();
        assertEquals(1, rowsInserted);
        
        String select = "SELECT i,j+1 FROM IntKeyTest";
        ResultSet rs = conn.createStatement().executeQuery(select);
        assertTrue(rs.next());
        assertEquals(1,rs.getInt(1));
        assertEquals(2,rs.getInt(2));
        assertFalse(rs.next());

        if (tgtPH()||tgtTR()) upsert = "UPSERT INTO IntKeyTest(i,j) select i, j+1 from IntKeyTest";
        else if (tgtSQ()) upsert = "UPDATE IntKeyTest SET j=j+1 where i in (SELECT i from IntKeyTest)";
        upsertStmt = conn.prepareStatement(upsert);
        rowsInserted = upsertStmt.executeUpdate();
        assertEquals(1, rowsInserted);
        
        select = "SELECT j FROM IntKeyTest";
        rs = conn.createStatement().executeQuery(select);
        assertTrue(rs.next());
        assertEquals(2,rs.getInt(1));
        assertFalse(rs.next());
        
        if (tgtPH()||tgtTR()) upsert = "UPSERT INTO IntKeyTest(i,j) select i, i from IntKeyTest";
        else if (tgtSQ())  upsert = "UPDATE IntKeyTest SET j=i where i in (SELECT i from IntKeyTest)";
        upsertStmt = conn.prepareStatement(upsert);
        rowsInserted = upsertStmt.executeUpdate();
        assertEquals(1, rowsInserted);
        
        select = "SELECT j FROM IntKeyTest";
        rs = conn.createStatement().executeQuery(select);
        assertTrue(rs.next());
        assertEquals(1,rs.getInt(1));
        assertFalse(rs.next());
    }

    @Test
    public void testUpsertSelectOnDescToAsc() throws Exception {
        printTestDescription();

        conn.createStatement().execute("create table IntKeyTest (i integer not null primary key desc, j integer)");
        String upsert = null;
        if (tgtPH()||tgtTR()) upsert = "UPSERT INTO IntKeyTest VALUES(1, 1)";
        else if (tgtSQ()) upsert = "INSERT INTO IntKeyTest VALUES(1, 1)";
        PreparedStatement upsertStmt = conn.prepareStatement(upsert);
        int rowsInserted = upsertStmt.executeUpdate();
        assertEquals(1, rowsInserted);
        
        if (tgtPH()||tgtTR()) upsert = "UPSERT INTO IntKeyTest(i,j) select i+1, j+1 from IntKeyTest";
        else if (tgtSQ()) upsert = "INSERT INTO IntKeyTest(i,j) select i+1, j+1 from IntKeyTest";
        upsertStmt = conn.prepareStatement(upsert);
        rowsInserted = upsertStmt.executeUpdate();
        assertEquals(1, rowsInserted);
        
        String select = null;
        if (tgtPH()) select = "SELECT i,j FROM IntKeyTest";
        else if (tgtSQ()||tgtTR()) select = "SELECT i,j FROM IntKeyTest order by 1 desc";
        ResultSet rs = conn.createStatement().executeQuery(select);
        assertTrue(rs.next());
        assertEquals(2,rs.getInt(1));
        assertEquals(2,rs.getInt(2));
        assertTrue(rs.next());
        assertEquals(1,rs.getInt(1));
        assertEquals(1,rs.getInt(2));
        assertFalse(rs.next());
    }

    @Test
    public void testUpsertSelectRowKeyMutationOnSplitedTable() throws Exception {
        printTestDescription();

        createTestTable("IntKeyTest");
        String upsert = null;
        if (tgtPH()||tgtTR()) upsert = "UPSERT INTO IntKeyTest VALUES(?)";
        else if (tgtSQ()) upsert = "INSERT INTO IntKeyTest VALUES(?)";
        PreparedStatement upsertStmt = conn.prepareStatement(upsert);
        upsertStmt.setInt(1, 1);
        upsertStmt.executeUpdate();
        upsertStmt.setInt(1, 3);
        upsertStmt.executeUpdate();
        
        // Normally this would force a server side update. But since this changes the PK column, it would
        // for to run on the client side.
        if (tgtPH()||tgtTR()) upsert = "UPSERT INTO IntKeyTest(i) SELECT i+1 from IntKeyTest";
        else if (tgtSQ()) upsert = "INSERT INTO IntKeyTest(i) SELECT i+1 from IntKeyTest";
        upsertStmt = conn.prepareStatement(upsert);
        int rowsInserted = upsertStmt.executeUpdate();
        assertEquals(2, rowsInserted);
        
        String select = null;
        if (tgtPH()) select = "SELECT i FROM IntKeyTest";
        else if (tgtSQ()||tgtTR()) select = "SELECT i FROM IntKeyTest order by 1";
        ResultSet rs = conn.createStatement().executeQuery(select);
        assertTrue(rs.next());
        assertEquals(1,rs.getInt(1));
        assertTrue(rs.next());
        assertTrue(rs.next());
        assertTrue(rs.next());
        assertEquals(4,rs.getInt(1));
        assertFalse(rs.next());
    }
    
    @Test
    public void testUpsertSelectWithLimit() throws Exception {
        printTestDescription();

        conn.createStatement().execute("create table phoenix_test (id varchar(10) not null primary key, val varchar(10), ts timestamp)");

        if (tgtPH()) {
            conn.createStatement().execute("upsert into phoenix_test values ('aaa', 'abc', current_date())");
            conn.createStatement().execute("upsert into phoenix_test values ('bbb', 'bcd', current_date())");
            conn.createStatement().execute("upsert into phoenix_test values ('ccc', 'cde', current_date())");
        }  else if (tgtTR()) {
            conn.createStatement().execute("upsert into phoenix_test values ('aaa', 'abc', CURRENT_TIMESTAMP)");
            conn.createStatement().execute("upsert into phoenix_test values ('bbb', 'bcd', CURRENT_TIMESTAMP)");
            conn.createStatement().execute("upsert into phoenix_test values ('ccc', 'cde', CURRENT_TIMESTAMP)");
        } else if (tgtSQ()) {
            conn.createStatement().execute("insert into phoenix_test values ('aaa', 'abc', CURRENT_TIMESTAMP)");
            conn.createStatement().execute("insert into phoenix_test values ('bbb', 'bcd', CURRENT_TIMESTAMP)");
            conn.createStatement().execute("insert into phoenix_test values ('ccc', 'cde', CURRENT_TIMESTAMP)");
        }

        ResultSet rs = null;
        if (tgtPH()) rs = conn.createStatement().executeQuery("select * from phoenix_test");
        else if (tgtSQ()||tgtTR()) rs = conn.createStatement().executeQuery("select * from phoenix_test order by 1");

        assertTrue(rs.next());
        assertEquals("aaa",rs.getString(1));
        assertEquals("abc",rs.getString(2));
        assertNotNull(rs.getDate(3));

        assertTrue(rs.next());
        assertEquals("bbb",rs.getString(1));
        assertEquals("bcd",rs.getString(2));
        assertNotNull(rs.getDate(3));

        assertTrue(rs.next());
        assertEquals("ccc",rs.getString(1));
        assertEquals("cde",rs.getString(2));
        assertNotNull(rs.getDate(3));

        if (tgtPH()) conn.createStatement().execute("upsert into phoenix_test (id, ts) select id, null from phoenix_test where id <= 'bbb' limit 1");
        else if (tgtTR()) conn.createStatement().execute("upsert into phoenix_test (id, ts) select id, null from phoenix_test where id < 'bbb'"); // TRAF: subquery does not support [first 1] or limit 1. Changed id <= 'bbb' to id < 'bbb'
        else if (tgtSQ()) conn.createStatement().execute("update phoenix_test set ts = null where id in (select id from phoenix_test where id < 'bbb')"); // TRAF: subquery does not support [first 1].  Changed id <= 'bbb' to id < 'bbb'

        if (tgtPH()) rs = conn.createStatement().executeQuery("select * from phoenix_test");
        else if (tgtSQ()||tgtTR()) rs = conn.createStatement().executeQuery("select * from phoenix_test order by 1");
 
        assertTrue(rs.next());
        assertEquals("aaa",rs.getString(1));
        assertEquals("abc",rs.getString(2));
        assertNull(rs.getDate(3));
        
        assertTrue(rs.next());
        assertEquals("bbb",rs.getString(1));
        assertEquals("bcd",rs.getString(2));
        assertNotNull(rs.getDate(3));
        
        assertTrue(rs.next());
        assertEquals("ccc",rs.getString(1));
        assertEquals("cde",rs.getString(2));
        assertNotNull(rs.getDate(3));
        
        assertFalse(rs.next());

    }
}
