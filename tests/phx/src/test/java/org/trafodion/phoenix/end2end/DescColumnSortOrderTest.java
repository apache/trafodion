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
import java.math.*;

import org.apache.commons.lang.ArrayUtils;
import com.google.common.collect.Lists;

/**
 * @author simontoens
 * @since 1.2
 */
public class DescColumnSortOrderTest extends BaseTest {

    private static final String TABLE = "DescColumnSortOrderTest";

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table " + TABLE));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    private static boolean SQ_order_by_desc = false;
    
    @Test
    public void noOder() throws Exception {
        printTestDescription();
        String ddl = null;
   
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (pk VARCHAR NOT NULL PRIMARY KEY)";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (pk VARCHAR(128) NOT NULL PRIMARY KEY)";
        runQueryTest(ddl, "pk", new Object[][]{{"a"}, {"b"}, {"c"}}, new Object[][]{{"a"}, {"b"}, {"c"}});
    }                                                           

    @Test
    public void noOrderCompositePK() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (oid CHAR(2) NOT NULL, code INTEGER NOT NULL constraint pk primary key (oid, code))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (ooid CHAR(2) NOT NULL, code INTEGER NOT NULL , constraint pk primary key (ooid, code))";
        Object[][] rows = new Object[][]{{"o1", 1}, {"o2", 2}, {"o3", 3}};
        if (tgtPH()) runQueryTest(ddl, upsert("oid", "code"), rows, rows);
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("ooid", "code"), rows, rows);
    }
    
    @Test
    public void ascOrderInlinePK() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (pk VARCHAR NOT NULL PRIMARY KEY ASC)";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (pk VARCHAR(128) NOT NULL PRIMARY KEY ASC)";
        runQueryTest(ddl, "pk", new Object[][]{{"a"}, {"b"}, {"c"}}, new Object[][]{{"a"}, {"b"}, {"c"}});
    }
    
    @Test
    public void ascOrderCompositePK() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (oid CHAR(2) NOT NULL, code INTEGER NOT NULL constraint pk primary key (oid ASC, code DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (ooid CHAR(2) NOT NULL, code INTEGER NOT NULL , constraint pk primary key (ooid ASC, code DESC))";
        Object[][] insertedRows = new Object[][]{{"o1", 1}, {"o1", 2}, {"o1", 3}};
        Object[][] expectedRows = new Object[][]{{"o1", 3}, {"o1", 2}, {"o1", 1}};
        if (tgtPH()) runQueryTest(ddl, upsert("oid", "code"), insertedRows, expectedRows);
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("ooid", "code"), insertedRows, expectedRows);              
    }

    @Test
    public void descOrderInlinePK() throws Exception {
        printTestDescription();
        String[] testTypes = null;
        if (tgtPH()) testTypes = new String[]{"CHAR(2)", "VARCHAR"};
        else if (tgtSQ()||tgtTR()) testTypes = new String[]{"CHAR(2)", "VARCHAR(128)"};

        SQ_order_by_desc = true;
        for (String type : testTypes) {
            dropTestObjects();
            String ddl = "CREATE TABLE " + TABLE + " (pk ${type} NOT NULL PRIMARY KEY DESC)".replace("${type}", type);
            runQueryTest(ddl, "pk", new Object[][]{{"aa"}, {"bb"}, {"cc"}}, new Object[][]{{"cc"}, {"bb"}, {"aa"}});
        }
        SQ_order_by_desc = false;
    }
    
    @Test
    public void descOrderCompositePK1() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (oid CHAR(2) NOT NULL, code INTEGER NOT NULL constraint pk primary key (oid DESC, code))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (ooid CHAR(2) NOT NULL, code INTEGER NOT NULL, constraint pk primary key (ooid DESC, code))";

        Object[][] insertedRows = new Object[][]{{"o1", 1}, {"o2", 2}, {"o3", 3}};
        Object[][] expectedRows = new Object[][]{{"o3", 3}, {"o2", 2}, {"o1", 1}};

        SQ_order_by_desc = true;
        if (tgtPH()) runQueryTest(ddl, upsert("oid", "code"), insertedRows, expectedRows);        
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("ooid", "code"), insertedRows, expectedRows);
        SQ_order_by_desc = false;
    }
    
    @Test
    public void descOrderCompositePK2() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (oid CHAR(2) NOT NULL, code INTEGER NOT NULL constraint pk primary key (oid DESC, code DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (ooid CHAR(2) NOT NULL, code INTEGER NOT NULL, constraint pk primary key (ooid DESC, code DESC))";

        Object[][] insertedRows = new Object[][]{{"o1", 1}, {"o1", 2}, {"o1", 3}};
        Object[][] expectedRows = new Object[][]{{"o1", 3}, {"o1", 2}, {"o1", 1}};

        SQ_order_by_desc = true;
        if (tgtPH()) runQueryTest(ddl, upsert("oid", "code"), insertedRows, expectedRows);
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("ooid", "code"), insertedRows, expectedRows); 
        SQ_order_by_desc = false;       
    }    

    @Test
    public void equalityDescInlinePK() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (pk VARCHAR NOT NULL PRIMARY KEY DESC)";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (pk VARCHAR(1) NOT NULL PRIMARY KEY DESC)";
        runQueryTest(ddl, upsert("pk"), new Object[][]{{"a"}, {"b"}, {"c"}}, new Object[][]{{"b"}}, new WhereCondition("pk", "=", "'b'"));
    }
    
    @Test
    public void equalityDescCompositePK1() throws Exception {
        printTestDescription();
        String ddl = null;

        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (oid CHAR(2) NOT NULL, code INTEGER NOT NULL constraint pk primary key (oid DESC, code DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (ooid CHAR(2) NOT NULL, code INTEGER NOT NULL,  constraint pk primary key (ooid DESC, code DESC))";

        Object[][] insertedRows = new Object[][]{{"o1", 1}, {"o2", 2}, {"o3", 3}};
        if (tgtPH()) runQueryTest(ddl, upsert("oid", "code"), insertedRows, new Object[][]{{"o2", 2}}, new WhereCondition("oid", "=", "'o2'"));        
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("ooid", "code"), insertedRows, new Object[][]{{"o2", 2}}, new WhereCondition("ooid", "=", "'o2'"));
    }
    
    @Test
    public void equalityDescCompositePK2() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (oid CHAR(2) NOT NULL, code INTEGER NOT NULL constraint pk primary key (oid DESC, code DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (ooid CHAR(2) NOT NULL, code INTEGER NOT NULL, constraint pk primary key (ooid DESC, code DESC))";

        Object[][] insertedRows = new Object[][]{{"o1", 1}, {"o1", 2}, {"o1", 3}};
        if (tgtPH()) runQueryTest(ddl, upsert("oid", "code"), insertedRows, new Object[][]{{"o1", 2}}, new WhereCondition("code", "=", "2"));        
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("ooid", "code"), insertedRows, new Object[][]{{"o1", 2}}, new WhereCondition("code", "=", "2"));
    }
    
    @Test
    public void inDescCompositePK1() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (oid CHAR(2) NOT NULL, code INTEGER NOT NULL constraint pk primary key (oid DESC, code DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (ooid CHAR(2) NOT NULL, code INTEGER NOT NULL, constraint pk primary key (ooid DESC, code DESC))";

        Object[][] insertedRows = new Object[][]{{"o1", 1}, {"o1", 2}, {"o1", 3}};
        if (tgtPH()) runQueryTest(ddl, upsert("oid", "code"), insertedRows, new Object[][]{{"o1", 2}}, new WhereCondition("code", "IN", "(2)"));        
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("ooid", "code"), insertedRows, new Object[][]{{"o1", 2}}, new WhereCondition("code", "IN", "(2)"));
    }
    
    @Test
    public void inDescCompositePK2() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (oid CHAR(2) NOT NULL, code INTEGER NOT NULL constraint pk primary key (oid DESC, code DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (ooid CHAR(2) NOT NULL, code INTEGER NOT NULL, constraint pk primary key (ooid DESC, code DESC))";

        Object[][] insertedRows = new Object[][]{{"o1", 1}, {"o2", 2}, {"o3", 3}};
        if (tgtPH()) runQueryTest(ddl, upsert("oid", "code"), insertedRows, new Object[][]{{"o2", 2}}, new WhereCondition("oid", "IN", "('o2')"));       
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("ooid", "code"), insertedRows, new Object[][]{{"o2", 2}}, new WhereCondition("ooid", "IN", "('o2')")); 
    }
    
    @Test
    public void likeDescCompositePK1() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (oid CHAR(2) NOT NULL, code INTEGER NOT NULL constraint pk primary key (oid DESC, code DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (ooid CHAR(2) NOT NULL, code INTEGER NOT NULL, constraint pk primary key (ooid DESC, code DESC))";

        Object[][] insertedRows = new Object[][]{{"a1", 1}, {"b2", 2}, {"c3", 3}};
        if (tgtPH()) runQueryTest(ddl, upsert("oid", "code"), insertedRows, new Object[][]{{"b2", 2}}, new WhereCondition("oid", "LIKE", "('b%')"));        
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("ooid", "code"), insertedRows, new Object[][]{{"b2", 2}}, new WhereCondition("ooid", "LIKE", "('b%')"));
    }
    
    @Test
    public void likeDescCompositePK2() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (oid CHAR(2) NOT NULL, code CHAR(2) NOT NULL constraint pk primary key (oid DESC, code DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (ooid CHAR(2) NOT NULL, code CHAR(2) NOT NULL, constraint pk primary key (ooid DESC, code DESC))";

        Object[][] insertedRows = new Object[][]{{"a1", "11"}, {"b2", "22"}, {"c3", "33"}};

        if (tgtPH()) runQueryTest(ddl, upsert("oid", "code"), insertedRows, new Object[][]{{"b2", "22"}}, new WhereCondition("code", "LIKE", "('2%')"));        
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("ooid", "code"), insertedRows, new Object[][]{{"b2", "22"}}, new WhereCondition("code", "LIKE", "('2%')"));
    }
    
    @Test
    public void greaterThanDescCompositePK3() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (oid CHAR(2) NOT NULL, code INTEGER NOT NULL constraint pk primary key (oid DESC, code DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (ooid CHAR(2) NOT NULL, code INTEGER NOT NULL, constraint pk primary key (ooid DESC, code DESC))";

        Object[][] insertedRows = new Object[][]{{"o1", 1}, {"o1", 2}, {"o1", 3}};
        Object[][] expectedRows = new Object[][]{{"o1", 2}, {"o1", 1}};

        SQ_order_by_desc = true;
        if (tgtPH()) runQueryTest(ddl, upsert("oid", "code"), insertedRows, expectedRows, new WhereCondition("code", "<", "3"));        
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("ooid", "code"), insertedRows, expectedRows, new WhereCondition("code", "<", "3"));
        SQ_order_by_desc = false;
    }
    
    @Test
    public void substrDescCompositePK1() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (oid CHAR(3) NOT NULL, code INTEGER NOT NULL constraint pk primary key (oid DESC, code ASC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (ooid CHAR(3) NOT NULL, code INTEGER NOT NULL, constraint pk primary key (ooid DESC, code ASC))";

        Object[][] insertedRows = new Object[][]{{"ao1", 1}, {"bo2", 2}, {"co3", 3}};
        Object[][] expectedRows = new Object[][]{{"co3", 3}, {"bo2", 2}};

        SQ_order_by_desc = true;
        if (tgtPH()) runQueryTest(ddl, upsert("oid", "code"), insertedRows, expectedRows, new WhereCondition("SUBSTR(oid, 3, 1)", ">", "'1'"));
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("ooid", "code"), insertedRows, expectedRows, new WhereCondition("SUBSTR(ooid, 3, 1)", ">", "'1'"));
        SQ_order_by_desc = false;
    }
        
    @Test
    public void substrDescCompositePK2() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (oid CHAR(4) NOT NULL, code INTEGER NOT NULL constraint pk primary key (oid DESC, code ASC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (ooid CHAR(4) NOT NULL, code INTEGER NOT NULL, constraint pk primary key (ooid DESC, code ASC))";

        Object[][] insertedRows = new Object[][]{{"aaaa", 1}, {"bbbb", 2}, {"cccd", 3}};
        Object[][] expectedRows = new Object[][]{{"cccd", 3}};

        if (tgtPH()) runQueryTest(ddl, upsert("oid", "code"), insertedRows, expectedRows, new WhereCondition("SUBSTR(oid, 4, 1)", "=", "'d'"));
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("ooid", "code"), insertedRows, expectedRows, new WhereCondition("SUBSTR(ooid, 4, 1)", "=", "'d'"));
    }    
    
    @Test
    public void lTrimDescCompositePK() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (oid VARCHAR NOT NULL, code INTEGER NOT NULL constraint pk primary key (oid DESC, code DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (ooid VARCHAR(128) NOT NULL, code INTEGER NOT NULL, constraint pk primary key (ooid DESC, code DESC))";

        Object[][] insertedRows = new Object[][]{{" o1 ", 1}, {"  o2", 2}, {"  o3", 3}};
        Object[][] expectedRows = new Object[][]{{"  o2", 2}};

        if (tgtPH()) runQueryTest(ddl, upsert("oid", "code"), insertedRows, expectedRows, new WhereCondition("LTRIM(oid)", "=", "'o2'"));
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("ooid", "code"), insertedRows, expectedRows, new WhereCondition("LTRIM(ooid)", "=", "'o2'"));
    }

    @Test
    public void countDescCompositePK() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (oid CHAR(2) NOT NULL, code INTEGER NOT NULL constraint pk primary key (oid DESC, code ASC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (ooid CHAR(2) NOT NULL, code INTEGER NOT NULL, constraint pk primary key (ooid DESC, code ASC))";

        Object[][] insertedRows = new Object[][]{{"o1", 1}, {"o2", 2}, {"o3", 3}};
        Object[][] expectedRows = new Object[][]{{3l}};

        if (tgtPH()) runQueryTest(ddl, upsert("oid", "code"), select("COUNT(oid)"), insertedRows, expectedRows);
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("ooid", "code"), select("COUNT(ooid)"), insertedRows, expectedRows);
    }
    
    @Test
    public void sumDescCompositePK() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (n1 INTEGER NOT NULL, n2 DECIMAL(10, 2) NOT NULL, n3 BIGINT NOT NULL " + 
            "constraint pk primary key (n1 DESC, n2 DESC, n3 DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (n1 INTEGER NOT NULL, n2 DECIMAL(10, 1) NOT NULL, n3 BIGINT NOT NULL " +
            ", constraint pk primary key (n1 DESC, n2 DESC, n3 DESC))";

        Object[][] insertedRows = new Object[][]{{10, bdec(10.2), 21l}, {20, bdec(20.2), 32l}, {30, bdec(30.2), 43l}};
        Object[][] expectedRows = new Object[][]{{60l, bdec(60.6), 96l}};

        runQueryTest(ddl, upsert("n1", "n2", "n3"), select("SUM(n1), SUM(n2), SUM(n3)"), insertedRows, expectedRows);
    }    
    
    @Test
    public void avgDescCompositePK() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (n1 INTEGER NOT NULL, n2 DECIMAL(10, 2) NOT NULL, n3 BIGINT NOT NULL " + 
            "constraint pk primary key (n1 DESC, n2 DESC, n3 DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (n1 INTEGER NOT NULL, n2 DECIMAL(10, 1) NOT NULL, n3 BIGINT NOT NULL " +
            ", constraint pk primary key (n1 DESC, n2 DESC, n3 DESC))";

        Object[][] insertedRows = new Object[][]{{10, bdec(10.2), 21l}, {20, bdec(20.2), 32l}, {30, bdec(30.2), 43l}};
        Object[][] expectedRows = null;
        if (tgtPH()) expectedRows = new Object[][]{{new BigDecimal(bint(2), -1), bdec(20.2), BigDecimal.valueOf(32)}};
        else if (tgtSQ()||tgtTR()) expectedRows = new Object[][]{{(new BigDecimal(bint(2), -1)).longValue(), bdec(20.2), BigDecimal.valueOf(32)}};

        runQueryTest(ddl, upsert("n1", "n2", "n3"), select("AVG(n1), AVG(n2), AVG(n3)"), insertedRows, expectedRows);

    }
    
    @Test
    public void minDescCompositePK() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (n1 INTEGER NOT NULL, n2 DECIMAL(10, 2) NOT NULL, n3 BIGINT NOT NULL " + 
            "constraint pk primary key (n1 DESC, n2 DESC, n3 DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (n1 INTEGER NOT NULL, n2 DECIMAL(10, 1) NOT NULL, n3 BIGINT NOT NULL " +
            ", constraint pk primary key (n1 DESC, n2 DESC, n3 DESC))";

        Object[][] insertedRows = new Object[][]{{10, bdec(10.2), 21l}, {20, bdec(20.2), 32l}, {30, bdec(30.2), 43l}};
        Object[][] expectedRows = new Object[][]{{10, bdec(10.2), 21l}};
        runQueryTest(ddl, upsert("n1", "n2", "n3"), select("MIN(n1), MIN(n2), MIN(n3)"), insertedRows, expectedRows);
    }
    
    @Test
    public void maxDescCompositePK() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (n1 INTEGER NOT NULL, n2 DECIMAL(10, 2) NOT NULL, n3 BIGINT NOT NULL " + 
            "constraint pk primary key (n1 DESC, n2 DESC, n3 DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (n1 INTEGER NOT NULL, n2 DECIMAL(10, 1) NOT NULL, n3 BIGINT NOT NULL " +
            ", constraint pk primary key (n1 DESC, n2 DESC, n3 DESC))";

        Object[][] insertedRows = new Object[][]{{10, bdec(10.2), 21l}, {20, bdec(20.2), 32l}, {30, bdec(30.2), 43l}};
        Object[][] expectedRows = new Object[][]{{30, bdec(30.2), 43l}};
        runQueryTest(ddl, upsert("n1", "n2", "n3"), select("MAX(n1), MAX(n2), MAX(n3)"), insertedRows, expectedRows);
    }
    
    @Test
    public void havingSumDescCompositePK() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (name CHAR(1) NOT NULL, code INTEGER NOT NULL " + 
            "constraint pk primary key (name DESC, code DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (name CHAR(1) NOT NULL, code INTEGER NOT NULL " +
            ", constraint pk primary key (name DESC, code DESC))";

        Object[][] insertedRows = new Object[][]{{"a", 10}, {"a", 20}, {"b", 100}}; 
        Object[][] expectedRows = new Object[][]{{"a", 30l}};
        runQueryTest(ddl, upsert("name", "code"), select("name", "SUM(code)"), insertedRows, expectedRows, 
            new HavingCondition("name", "SUM(code) = 30"));
    }
    
    @Test
    public void queryDescDateWithExplicitOrderBy() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (c1 CHAR(1) NOT NULL, c2 CHAR(1) NOT NULL, d1 DATE NOT NULL, c3 CHAR(1) NOT NULL " + 
            "constraint pk primary key (c1, c2, d1 DESC, c3))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (c1 CHAR(1) NOT NULL, c2 CHAR(1) NOT NULL, d1 DATE NOT NULL, c3 CHAR(1) NOT NULL " +
            ", constraint pk primary key (c1, c2, d1 DESC, c3))";

        Object[] row1 = {"1", "2", date(10, 11, 2001), "3"};
        Object[] row2 = {"1", "2", date(10, 11, 2003), "3"};
        Object[][] insertedRows = new Object[][]{row1, row2};
        runQueryTest(ddl, upsert("c1", "c2", "d1", "c3"), select("c1, c2, d1", "c3"), insertedRows, new Object[][]{row2, row1},
            null, null, new OrderBy("d1", OrderBy.Direction.DESC));
    }    
    
    @Test
    public void additionOnDescCompositePK() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (n1 INTEGER NOT NULL, n2 DECIMAL(10, 2) NOT NULL, n3 BIGINT NOT NULL, d1 DATE NOT NULL " + 
            "constraint pk primary key (n1 DESC, n2 DESC, n3 DESC, d1 DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (n1 INTEGER NOT NULL, n2 DECIMAL(10, 1) NOT NULL, n3 BIGINT NOT NULL, d1 DATE NOT NULL " +
            ", constraint pk primary key (n1 DESC, n2 DESC, n3 DESC, d1 DESC))";

        Object[][] insertedRows = new Object[][]{
            {10, bdec(10.2), 21l, date(1, 10, 2001)}, {20, bdec(20.2), 32l, date(2, 6, 2001)}, {30, bdec(30.2), 43l, date(3, 1, 2001)}};
        Object[][] expectedRows = new Object[][]{
            {31l, bdec(32.2), 46l, date(3, 5, 2001)}, {21l, bdec(22.2), 35l, date(2, 10, 2001)}, {11l, bdec(12.2), 24l, date(1, 14, 2001)}};

        SQ_order_by_desc = true;
        runQueryTest(ddl, upsert("n1", "n2", "n3", "d1"), select("n1+1, n2+2, n3+3", "d1+4"), insertedRows, expectedRows);
        SQ_order_by_desc = false;
    }
    
    @Test 
    public void subtractionOnDescCompositePK() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (n1 INTEGER NOT NULL, n2 DECIMAL(10, 2) NOT NULL, n3 BIGINT NOT NULL, d1 DATE NOT NULL " + 
            "constraint pk primary key (n1 DESC, n2 DESC, n3 DESC, d1 DESC))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (n1 INTEGER NOT NULL, n2 DECIMAL(10, 1) NOT NULL, n3 BIGINT NOT NULL, d1 DATE NOT NULL " +
            ", constraint pk primary key (n1 DESC, n2 DESC, n3 DESC, d1 DESC))";

        Object[][] insertedRows = new Object[][]{
            {10, bdec(10.2), 21l, date(1, 10, 2001)}, {20, bdec(20.2), 32l, date(2, 6, 2001)}, {30, bdec(30.2), 43l, date(3, 10, 2001)}};
        Object[][] expectedRows = new Object[][]{
            {29l, bdec(28.2), 40l, date(3, 6, 2001)}, {19l, bdec(18.2), 29l, date(2, 2, 2001)}, {9l, bdec(8.2), 18l, date(1, 6, 2001)}};

        SQ_order_by_desc = true;
        runQueryTest(ddl, upsert("n1", "n2", "n3", "d1"), select("n1-1, n2-2, n3-3", "d1-4"), insertedRows, expectedRows);
        SQ_order_by_desc = false;
    }
    
    @Test
    public void lessThanLeadingDescCompositePK() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (id INTEGER NOT NULL, date DATE NOT NULL constraint pk primary key (id DESC, date))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (id INTEGER NOT NULL, date1 DATE NOT NULL, constraint pk primary key (id DESC, date1))";

        Object[][] insertedRows = new Object[][]{{1, date(1, 1, 2012)}, {3, date(1, 1, 2013)}, {2, date(1, 1, 2011)}};
        Object[][] expectedRows = new Object[][]{{1, date(1, 1, 2012)}};
        if (tgtPH()) runQueryTest(ddl, upsert("id", "date"), insertedRows, expectedRows, new WhereCondition("id", "<", "2"));
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("id", "date1"), insertedRows, expectedRows, new WhereCondition("id", "<", "2"));
    }
    
    @Test
    public void lessThanTrailingDescCompositePK() throws Exception {
        printTestDescription();
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE " + TABLE + " (id INTEGER NOT NULL, date DATE NOT NULL constraint pk primary key (id DESC, date))";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE " + TABLE + " (id INTEGER NOT NULL, date1 DATE NOT NULL, constraint pk primary key (id DESC, date1))";

        Object[][] insertedRows = new Object[][]{{1, date(1, 1, 2002)}, {3, date(1, 1, 2003)}, {2, date(1, 1, 2001)}};
        Object[][] expectedRows = new Object[][]{{2, date(1, 1, 2001)}};

        if (tgtPH()) runQueryTest(ddl, upsert("id", "date"), insertedRows, expectedRows, new WhereCondition("date", "<", "TO_DATE('02-02-2001','mm-dd-yyyy')"));
        else if (tgtSQ()||tgtTR()) runQueryTest(ddl, upsert("id", "date1"), insertedRows, expectedRows, new WhereCondition("date1", "<", "DATE '02-02-2001'"));
    }
    
    private void runQueryTest(String ddl, String columnName, Object[][] rows, Object[][] expectedRows) throws Exception {
        runQueryTest(ddl, new String[]{columnName}, rows, expectedRows, null);
    }
    
    private void runQueryTest(String ddl, String[] columnNames, Object[][] rows, Object[][] expectedRows) throws Exception {
        runQueryTest(ddl, columnNames, rows, expectedRows, null);
    }
    
    private void runQueryTest(String ddl, String[] columnNames, Object[][] rows, Object[][] expectedRows, WhereCondition condition) throws Exception {
        runQueryTest(ddl, columnNames, columnNames, rows, expectedRows, condition, null, null);
    }
    
    private void runQueryTest(String ddl, String[] columnNames, String[] projections, Object[][] rows, Object[][] expectedRows) throws Exception {
        runQueryTest(ddl, columnNames, projections, rows, expectedRows, null, null, null);
    }
    
    private void runQueryTest(String ddl, String[] columnNames, String[] projections, Object[][] rows, Object[][] expectedRows, HavingCondition havingCondition) throws Exception {
        runQueryTest(ddl, columnNames, projections, rows, expectedRows, null, havingCondition, null);
    }
    

    private static void runQueryTest(
        String ddl, 
        String[] columnNames, 
        String[] projections, 
        Object[][] rows, Object[][] expectedRows, 
        WhereCondition whereCondition, 
        HavingCondition havingCondition,
        OrderBy orderBy) 
        throws Exception 
    {
        try {
            conn.createStatement().execute(ddl);

            String columns = appendColumns(columnNames);
            String placeholders = appendPlaceholders(columnNames);
            String dml = null;
            if (tgtPH()||tgtTR()) dml = "UPSERT INTO " + TABLE + " (" + columns + ") VALUES(" + placeholders +")";
            else if (tgtSQ()) dml = "INSERT INTO " + TABLE + " (" + columns + ") VALUES(" + placeholders +")";

            PreparedStatement stmt = conn.prepareStatement(dml);

            for (int row = 0; row < rows.length; row++) {
                for (int col = 0; col < rows[row].length; col++) {
                    Object value = rows[row][col];
                    stmt.setObject(col + 1, value);
                }
                stmt.execute();
            }
            
            String selectClause = "SELECT " + appendColumns(projections) + " FROM " + TABLE;

            for (WhereCondition whereConditionClause : new WhereCondition[]{whereCondition, WhereCondition.reverse(whereCondition)}) {
                String query = WhereCondition.appendWhere(whereConditionClause, selectClause);
                query = HavingCondition.appendHaving(havingCondition, query);
                query = OrderBy.appendOrderBy(orderBy, query);
                runQuery(conn, query, expectedRows);
            }
            
            if (orderBy != null) {
                orderBy = OrderBy.reverse(orderBy);
                String query = WhereCondition.appendWhere(whereCondition, selectClause);
                query = HavingCondition.appendHaving(havingCondition, query);
                query = OrderBy.appendOrderBy(orderBy, query);
                runQuery(conn, query, reverse(expectedRows));
            }
        } finally {
        }
    }
    
    private static String appendColumns(String[] columnNames) {
        String appendedColumns = "";
        for (int i = 0; i < columnNames.length; i++) {                
            appendedColumns += columnNames[i];
            if (i < columnNames.length - 1) {
                appendedColumns += ",";
            }
        }
        return appendedColumns;
    }
    
    private static String appendPlaceholders(String[] columnNames) {
        String placeholderList = "";
        for (int i = 0; i < columnNames.length; i++) {                
            placeholderList += "?";
            if (i < columnNames.length - 1) {
                placeholderList += ",";
            }
        }
        return placeholderList;        
    }
    
    private static void runQuery(Connection connection, String query, Object[][] expectedValues) throws Exception {

        if (tgtSQ()||tgtTR()) {
            if (! query.toLowerCase().contains("order by")) {
                query += " order by 1";
                if (SQ_order_by_desc)
                    query += " desc";
            }
        }

        PreparedStatement stmt = connection.prepareStatement(query);
        ResultSet rs = stmt.executeQuery();
        int rowCounter = 0;
        while (rs.next()) {
            if (rowCounter == expectedValues.length) {
                Assert.assertEquals("Exceeded number of expected rows for query" + query, expectedValues.length, rowCounter+1);
            }
            Object[] cols = new Object[expectedValues[rowCounter].length];
            for (int colCounter = 0; colCounter < expectedValues[rowCounter].length; colCounter++) {
                cols[colCounter] = rs.getObject(colCounter+1);
            }
            /* TRAF: For some reason, the following code does not work for
             * DATE data type.  The comparision returns false even if 2
             * dates are the same.  I think it has something to do with 
             * Equals() acutally compares the object references when it comes
             * to an object?  Replace it with the new comparision method.
            Assert.assertArrayEquals("Unexpected result for query " + query, expectedValues[rowCounter], cols);
             */
            /* TRAF */
            for (int colCounter = 0; colCounter < expectedValues[rowCounter].length; colCounter++) {
                /* Do string comparision. */
                Assert.assertTrue("Unexpected result for query " + query + " expected " + expectedValues[rowCounter][colCounter].toString() + ", was " + cols[colCounter].toString(), expectedValues[rowCounter][colCounter].toString().equals(cols[colCounter].toString()));
            }
            /* end of TRAF */

            rowCounter++;
        }
        Assert.assertEquals("Unexpected number of rows for query " + query, expectedValues.length, rowCounter);
    }
    
    private static Object[][] reverse(Object[][] rows) {
        Object[][] reversedArray = new Object[rows.length][];
        System.arraycopy(rows, 0, reversedArray, 0, rows.length);
        ArrayUtils.reverse(reversedArray);
        return reversedArray;
    }
    
    private Date date(int month, int day, int year) {
        Calendar cal = new GregorianCalendar();
        cal.set(Calendar.MONTH, month-1);
        cal.set(Calendar.DAY_OF_MONTH, day);
        cal.set(Calendar.YEAR, year);
        cal.set(Calendar.HOUR_OF_DAY, 10);
        cal.set(Calendar.MINUTE, 2);
        cal.set(Calendar.SECOND, 5);
        cal.set(Calendar.MILLISECOND, 101);
        Date d = new Date(cal.getTimeInMillis()); 
        return d;
    }
        
    private String[] upsert(String...args) {
        return args;
    }
    
    private String[] select(String...args) {
        return args;
    }
    
    private BigDecimal bdec(double d) {
        return BigDecimal.valueOf(d);
    }
    
    private BigInteger bint(long l) {
        return BigInteger.valueOf(l);
    }    
    
    private static class WhereCondition {
        final String lhs;
        final String operator;
        final String rhs;
    
        WhereCondition(String lhs, String operator, String rhs) {
            this.lhs = lhs;
            this.operator = operator;
            this.rhs = rhs;
        }
        
        static WhereCondition reverse(WhereCondition whereCondition) {
            
            if (whereCondition == null) {
                return null; 
            }
            
            if (whereCondition.operator.equalsIgnoreCase("IN") || whereCondition.operator.equalsIgnoreCase("LIKE")) {
                return whereCondition;
            } else {
                return new WhereCondition(whereCondition.rhs, whereCondition.getReversedOperator(), whereCondition.lhs);
            }
        }
        
        static String appendWhere(WhereCondition whereCondition, String query) {
             if (whereCondition == null) {
                 return query;
             }
            return query + " WHERE " + whereCondition.lhs + " " + whereCondition.operator + " " + whereCondition.rhs;
        }
        
        private String getReversedOperator() {
            if (operator.equals("<")) {
                return ">";
            } else if (operator.equals(">")) {
                return "<";
            }
            return operator;
        }
    }
    
    private static class HavingCondition {
        
        private String groupby;
        private String having;
        
        HavingCondition(String groupby, String having) {
            this.groupby = groupby;
            this.having = having;
        }
        
        static String appendHaving(HavingCondition havingCondition, String query) {
            if (havingCondition == null) {
                return query;
            }
            return query + " GROUP BY " + havingCondition.groupby + " HAVING " + havingCondition.having + " ";
        }
    }
    
    private static class OrderBy {
        
        enum Direction {
            
            ASC, DESC;
            
            Direction reverse() {
                if (this == ASC) {
                    return DESC;
                }
                return ASC;
            }
        }
        
        private List<String> columnNames = Lists.newArrayList();
        private List<Direction> directions = Lists.newArrayList();
        
        OrderBy() {            
        }
        
        OrderBy(String columnName, Direction orderBy) {
            add(columnName, orderBy);
        }
        
        void add(String columnName, Direction direction) {
            columnNames.add(columnName);
            directions.add(direction);
        }
        
        static OrderBy reverse(OrderBy orderBy) {
            
            if (orderBy == null) {
                return null;
            }
            
            List<Direction> reversedDirections = Lists.newArrayList();
            for (Direction dir : orderBy.directions) {
                reversedDirections.add(dir.reverse());
            }
            OrderBy reversedOrderBy = new OrderBy();
            reversedOrderBy.columnNames = orderBy.columnNames;
            reversedOrderBy.directions = reversedDirections;
            return reversedOrderBy;
        }
        
        static String appendOrderBy(OrderBy orderBy, String query) {
            if (orderBy == null || orderBy.columnNames.isEmpty()) {
                return query;
            }
            query += " ORDER BY ";
            for (int i = 0; i < orderBy.columnNames.size(); i++) {
                query += orderBy.columnNames.get(i) + " " + orderBy.directions.get(i).toString() + " ";
            }
          
            if (tgtPH() || tgtTR()) 
                query += " LIMIT 1000 ";

            return query;
        }        
    }
}
