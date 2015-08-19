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
import java.sql.Date;
import java.util.*;
import java.math.*;

public class CustomEntityDataTest extends BaseTest {

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table " + CUSTOM_ENTITY_DATA_FULL_NAME));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    protected void initTableValues() throws Exception {
        createTestTable(CUSTOM_ENTITY_DATA_FULL_NAME);
            
        // Insert all rows at ts
        PreparedStatement stmt = null;
        if (tgtPH()||tgtTR()) stmt = conn.prepareStatement(
                "upsert into " + CUSTOM_ENTITY_DATA_FULL_NAME + "(" + 
                "    ORGANIZATION_ID, " +
                "    KEY_PREFIX, " +
                "    CUSTOM_ENTITY_DATA_ID, " +
                "    CREATED_BY, " +
                "    CREATED_DATE, " +
                "    CURRENCY_ISO_CODE, " +
                "    DELETED, " +
                "    DIVISION, " +
                "    LAST_UPDATE, " +
                "    LAST_UPDATE_BY," +
                "    NAME," +
                "    OWNER," +
                "    SYSTEM_MODSTAMP," +
                "    VAL0," +
                "    VAL1," +
                "    VAL2," +
                "    VAL3," +
                "    VAL4," +
                "    VAL5," +
                "    VAL6," +
                "    VAL7," +
                "    VAL8," +
                "    VAL9)" +
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            else if (tgtSQ()) stmt = conn.prepareStatement(
                "insert into " + CUSTOM_ENTITY_DATA_FULL_NAME + "(" +
                "    ORGANIZATION_ID, " +
                "    KEY_PREFIX, " +
                "    CUSTOM_ENTITY_DATA_ID, " +
                "    CREATED_BY, " +
                "    CREATED_DATE, " +
                "    CURRENCY_ISO_CODE, " +
                "    DELETED, " +
                "    DIVISION, " +
                "    LAST_UPDATE, " +
                "    LAST_UPDATE_BY," +
                "    NAME," +
                "    OWNER," +
                "    SYSTEM_MODSTAMP," +
                "    VAL0," +
                "    VAL1," +
                "    VAL2," +
                "    VAL3," +
                "    VAL4," +
                "    VAL5," +
                "    VAL6," +
                "    VAL7," +
                "    VAL8," +
                "    VAL9)" +
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    
        // Insert all rows at ts
        stmt.setString(1, tenantId);
        stmt.setString(2, ROW2.substring(0,3));
        stmt.setString(3, ROW2.substring(3));
        stmt.setString(4, "Curly");
        stmt.setDate(5, new Date(1));
        stmt.setString(6, "ISO");
        stmt.setString(7, "0");
        stmt.setBigDecimal(8, new BigDecimal(1));
        stmt.setDate(9, new Date(2));
        stmt.setString(10, "Curly");
        stmt.setString(11, "Curly");
        stmt.setString(12, "Curly");
        stmt.setDate(13, new Date(2));
        stmt.setString(14, "2");
        stmt.setString(15, "2");
        stmt.setString(16, "2");
        stmt.setString(17, "2");
        stmt.setString(18, "2");
        stmt.setString(19, "2");
        stmt.setString(20, "2");
        stmt.setString(21, "2");
        stmt.setString(22, "2");
        stmt.setString(23, "2");
        stmt.execute();

        stmt.setString(1, tenantId);
        stmt.setString(2, ROW5.substring(0,3));
        stmt.setString(3, ROW5.substring(3));
        stmt.setString(4, "Moe");
        stmt.setDate(5, new Date(1));
        stmt.setString(6, "ISO");
        stmt.setString(7, "0");
        stmt.setBigDecimal(8, new BigDecimal(1));
        stmt.setDate(9, new Date(2));
        stmt.setString(10, "Moe");
        stmt.setString(11, "Moe");
        stmt.setString(12, "Moe");
        stmt.setDate(13, new Date(2));
        stmt.setString(14, "5");
        stmt.setString(15, "5");
        stmt.setString(16, "5");
        stmt.setString(17, "5");
        stmt.setString(18, "5");
        stmt.setString(19, "5");
        stmt.setString(20, "5");
        stmt.setString(21, "5");
        stmt.setString(22, "5");
        stmt.setString(23, "5");
        stmt.execute();

        stmt.setString(1, tenantId);
        stmt.setString(2, ROW9.substring(0,3));
        stmt.setString(3, ROW9.substring(3));
        stmt.setString(4, "Larry");
        stmt.setDate(5, new Date(1));
        stmt.setString(6, "ISO");
        stmt.setString(7, "0");
        stmt.setBigDecimal(8, new BigDecimal(1));
        stmt.setDate(9, new Date(2));
        stmt.setString(10, "Larry");
        stmt.setString(11, "Larry");
        stmt.setString(12, "Larry");
        stmt.setDate(13, new Date(2));
        stmt.setString(14, "v9");
        stmt.setString(15, "v9");
        stmt.setString(16, "v9");
        stmt.setString(17, "v9");
        stmt.setString(18, "v9");
        stmt.setString(19, "v9");
        stmt.setString(20, "v9");
        stmt.setString(21, "v9");
        stmt.setString(22, "v9");
        stmt.setString(23, "v9");
        stmt.execute();
    }    

    @Test
    public void testUngroupedAggregation() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT count(1) FROM CORE.CUSTOM_ENTITY_DATA WHERE organization_id=?";
        else if (tgtSQ()||tgtTR()) query = "SELECT count(1) FROM CUSTOM_ENTITY_DATA WHERE organization_id=?";

        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            assertEquals(3, rs.getLong(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testScan() throws Exception {
        printTestDescription();

        String query = null;
        if (tgtPH()) query = "SELECT CREATED_BY,CREATED_DATE,CURRENCY_ISO_CODE,DELETED,DIVISION,LAST_UPDATE,LAST_UPDATE_BY,NAME,OWNER,SYSTEM_MODSTAMP,VAL0,VAL1,VAL2,VAL3,VAL4,VAL5,VAL6,VAL7,VAL8,VAL9 FROM CORE.CUSTOM_ENTITY_DATA WHERE organization_id=?";
        else if (tgtSQ()||tgtTR()) query = "SELECT CREATED_BY,CREATED_DATE,CURRENCY_ISO_CODE,DELETED,DIVISION,LAST_UPDATE,LAST_UPDATE_BY,NAME,OWNER,SYSTEM_MODSTAMP,VAL0,VAL1,VAL2,VAL3,VAL4,VAL5,VAL6,VAL7,VAL8,VAL9 FROM CUSTOM_ENTITY_DATA WHERE organization_id=? order by 1";
        try {
            initTableValues();
            PreparedStatement statement = conn.prepareStatement(query);
            statement.setString(1, tenantId);
            ResultSet rs = statement.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) {
                assertEquals("Curly", rs.getString(1));
                assertTrue(rs.next());
                assertEquals("Moe", rs.getString(1));
                assertTrue(rs.next());
                assertEquals("Larry", rs.getString(1));
            } else if (tgtSQ()||tgtTR()) {
                assertEquals("Curly", rs.getString(1));
                assertTrue(rs.next());
                assertEquals("Larry", rs.getString(1));
                assertTrue(rs.next());
                assertEquals("Moe", rs.getString(1));
            }
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testWhereStringConcatExpression() throws Exception {
        printTestDescription();

        initTableValues();
        String query = null;
        if (tgtPH()) query = "SELECT KEY_PREFIX||CUSTOM_ENTITY_DATA_ID FROM CORE.CUSTOM_ENTITY_DATA where '00A'||val0 LIKE '00A2%'";
        else if (tgtSQ()||tgtTR()) query = "SELECT KEY_PREFIX||CUSTOM_ENTITY_DATA_ID FROM CUSTOM_ENTITY_DATA where '00A'||val0 LIKE '00A2%'";

        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs=statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(ROW2, rs.getString(1));
            assertFalse(rs.next());
        }
        finally {
        }
    }
}
