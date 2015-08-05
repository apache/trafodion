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
import java.text.*;

public class UpsertValuesTest extends BaseTest {

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table IntKeyTest", "table upsertDateTest", "table phoenix_uuid_mac", "table UpsertWithDesc", "table " + PTSDB_NAME));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    @Test
    public void testUpsertDateValues() throws Exception {
        printTestDescription();

        Date now = new Date(System.currentTimeMillis());
        createTestTable(PTSDB_NAME);
        String dateString = "1999-01-01 02:00:00";
        PreparedStatement upsertStmt = null;
        if (tgtPH()) upsertStmt = conn.prepareStatement("upsert into ptsdb(inst,host,date) values('aaa','bbb',to_date('" + dateString + "'))");
        else if (tgtTR()) upsertStmt = conn.prepareStatement("upsert into ptsdb(inst,host1,date1) values('aaa','bbb', TIMESTAMP '" + dateString + "')");
        else if (tgtSQ()) upsertStmt = conn.prepareStatement("insert into ptsdb(inst,host1,date1) values('aaa','bbb', TIMESTAMP '" + dateString + "')");
        int rowsInserted = upsertStmt.executeUpdate();
        assertEquals(1, rowsInserted);
        if (tgtPH()) upsertStmt = conn.prepareStatement("upsert into ptsdb(inst,host,date) values('ccc','ddd',current_date())");
        else if (tgtTR()) upsertStmt = conn.prepareStatement("upsert into ptsdb(inst,host1,date1) values('ccc','ddd',CURRENT_TIMESTAMP)");
        else if (tgtSQ()) upsertStmt = conn.prepareStatement("insert into ptsdb(inst,host1,date1) values('ccc','ddd',CURRENT_TIMESTAMP)");
        rowsInserted = upsertStmt.executeUpdate();
        assertEquals(1, rowsInserted);
        
        String select = null;
        if (tgtPH()) select = "SELECT date,current_date() FROM ptsdb";
        else if (tgtSQ()||tgtTR()) select = "SELECT date1,CURRENT_TIMESTAMP FROM ptsdb order by 1"; 
        ResultSet rs = conn.createStatement().executeQuery(select);
        Date then = new Date(System.currentTimeMillis());
        assertTrue(rs.next());
        // TRAF Date date = DateUtil.parseDate(dateString);
        if (tgtPH()) assertEquals(dateString,rs.getDate(1).toString());
        else if (tgtSQ()||tgtTR()) assertEquals(dateString, new SimpleDateFormat("yyyy-MM-dd hh:mm:ss").format(rs.getTimestamp(1)));
        assertTrue(rs.next());
        if (tgtPH()) assertTrue(rs.getDate(1).after(now) && rs.getDate(1).before(then));
        else if (tgtSQ()||tgtTR()) {
            /* Most java time function returns UTC.  We also run phoenix_test
             * with -Duser.timezone=UTC to force UTC.  But CURRENT_TIMESTAMP 
             * uses the local machine time zone (say PST), which is not always
             * UTC.  Skip this comparision.
             */
            // assertTrue(rs.getTimestamp(1).after(now) && rs.getTimestamp(1).before(then));
        }
        assertFalse(rs.next());
    }
    
    @Test
    public void testUpsertValuesWithExpression() throws Exception {
        printTestDescription();

        createTestTable("IntKeyTest");
        String upsert = null;
        if (tgtPH()||tgtTR()) upsert = "UPSERT INTO IntKeyTest VALUES(-1)";
        else if (tgtSQ()) upsert = "INSERT INTO IntKeyTest VALUES(-1)";
        PreparedStatement upsertStmt = conn.prepareStatement(upsert);
        int rowsInserted = upsertStmt.executeUpdate();
        assertEquals(1, rowsInserted);
        if (tgtPH()||tgtTR()) upsert = "UPSERT INTO IntKeyTest VALUES(1+2)";
        else if (tgtSQ()) upsert = "INSERT INTO IntKeyTest VALUES(1+2)";
        upsertStmt = conn.prepareStatement(upsert);
        rowsInserted = upsertStmt.executeUpdate();
        assertEquals(1, rowsInserted);
        
        String select = null;
        if (tgtPH()) select = "SELECT i FROM IntKeyTest";
        else if (tgtSQ()||tgtTR()) select = "SELECT i FROM IntKeyTest order by 1";
        ResultSet rs = conn.createStatement().executeQuery(select);
        assertTrue(rs.next());
        assertEquals(-1,rs.getInt(1));
        assertTrue(rs.next());
        assertEquals(3,rs.getInt(1));
        assertFalse(rs.next());
    }
    
    @Test
    public void testUpsertValuesWithDate() throws Exception {
        printTestDescription();

        if (tgtPH()) conn.createStatement().execute("create table UpsertDateTest (k VARCHAR not null primary key,date DATE)");
        else if (tgtSQ()||tgtTR()) conn.createStatement().execute("create table UpsertDateTest (k VARCHAR(128) not null primary key,date1 TIMESTAMP)");

        if (tgtPH()) conn.createStatement().execute("upsert into UpsertDateTest values ('a',to_date('2013-06-08 00:00:00'))");
        else if (tgtTR()) conn.createStatement().execute("upsert into UpsertDateTest values ('a', TIMESTAMP '2013-06-08 00:00:00')");
        else if (tgtSQ()) conn.createStatement().execute("insert into UpsertDateTest values ('a', TIMESTAMP '2013-06-08 00:00:00')");
        
        ResultSet rs = null;
        if (tgtPH()) rs = conn.createStatement().executeQuery("select k,to_char(date) from UpsertDateTest");
        else if (tgtSQ()||tgtTR()) rs = conn.createStatement().executeQuery("select k,RTRIM(CAST(CAST(date1 as TIMESTAMP(0)) as CHAR(128))) from UpsertDateTest");

        assertTrue(rs.next());

        assertEquals("a", rs.getString(1));
        assertEquals("2013-06-08 00:00:00", rs.getString(2));
    }

    @Test
    public void testUpsertVarCharWithMaxLength() throws Exception {
        printTestDescription();

        if (tgtPH()) conn.createStatement().execute("create table phoenix_uuid_mac (mac_md5 VARCHAR not null primary key,raw_mac VARCHAR)");
        else if (tgtSQ()||tgtTR()) conn.createStatement().execute("create table phoenix_uuid_mac (mac_md5 VARCHAR(128) not null primary key,raw_mac VARCHAR(128))");

        if (tgtPH()||tgtTR()) conn.createStatement().execute("upsert into phoenix_uuid_mac values ('00000000591','a')");
        else if (tgtSQ()) conn.createStatement().execute("insert into phoenix_uuid_mac values ('00000000591','a')");
        if (tgtPH()||tgtTR()) conn.createStatement().execute("upsert into phoenix_uuid_mac values ('000000005919','b')");
        else if (tgtSQ()) conn.createStatement().execute("insert into phoenix_uuid_mac values ('000000005919','b')");       
 
        ResultSet rs = conn.createStatement().executeQuery("select max(mac_md5) from phoenix_uuid_mac");
        assertTrue(rs.next());
        assertEquals("000000005919", rs.getString(1));

        if (tgtPH()||tgtTR()) conn.createStatement().execute("upsert into phoenix_uuid_mac values ('000000005919adfasfasfsafdasdfasfdasdfdasfdsafaxxf1','b')");
        else if (tgtSQ()) conn.createStatement().execute("insert into phoenix_uuid_mac values ('000000005919adfasfasfsafdasdfasfdasdfdasfdsafaxxf1','b')");

        rs = conn.createStatement().executeQuery("select max(mac_md5) from phoenix_uuid_mac");
        assertTrue(rs.next());
        assertEquals("000000005919adfasfasfsafdasdfasfdasdfdasfdsafaxxf1", rs.getString(1));
    }
    
    @Test
    public void testUpsertValuesWithDescExpression() throws Exception {
        printTestDescription();

        if (tgtPH()) conn.createStatement().execute("create table UpsertWithDesc (k VARCHAR not null primary key desc)");
        else if (tgtSQ()||tgtTR()) conn.createStatement().execute("create table UpsertWithDesc (k VARCHAR(128) not null primary key desc)");

        if (tgtPH()) conn.createStatement().execute("upsert into UpsertWithDesc values (to_char(100))");
        else if (tgtTR()) conn.createStatement().execute("upsert into UpsertWithDesc values (CAST(100 as VARCHAR(128)))");
        else if (tgtSQ()) conn.createStatement().execute("insert into UpsertWithDesc values (CAST(100 as VARCHAR(128)))");       
 
        ResultSet rs = null;
        if (tgtPH()) rs = conn.createStatement().executeQuery("select to_number(k) from UpsertWithDesc");
        else if (tgtSQ()||tgtTR()) rs = conn.createStatement().executeQuery("select CAST(k as INT) from UpsertWithDesc");
        assertTrue(rs.next());
        assertEquals(100, rs.getInt(1));
        assertFalse(rs.next());
    }

}
