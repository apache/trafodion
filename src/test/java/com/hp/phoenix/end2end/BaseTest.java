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
import java.io.*;
import java.lang.Thread.*;

import com.google.common.collect.ImmutableMap;

public abstract class BaseTest {

    // target is phoenix, TRAF, or SQ?
    protected enum TargetType {PHOENIX, TRAF, SQ};
    protected static TargetType targetType = TargetType.TRAF; 
    protected static void setTgtPH() { targetType = TargetType.PHOENIX; }
    protected static void setTgtTR() { targetType = TargetType.TRAF; }
    protected static void setTgtSQ() { targetType = TargetType.SQ; }

    protected static boolean tgtPH() { return targetType == TargetType.PHOENIX; }
    protected static boolean tgtTR() { return targetType == TargetType.TRAF; }
    protected static boolean tgtSQ() { return targetType == TargetType.SQ; }

    // Do you want to print the test info while Junit runs?
    private final boolean print_testinfo = false;

    private static String my_catalog = null;
    private static String my_schema = null;

    protected static Connection conn = null;
    /* List all of the object names being used in this entire class.
     * The objects are dropped with errors ignored, so it is OK if the
     * object does not exist for a particular test.
     */
    protected static ArrayList<String> objDropList = null;

    // This is really a @BeforeClass, but the child needs to initialize
    // objDropList before we can call this function, so the acutal
    // @BeforeClass is defined in the child and then it calls this
    // one after objDropList is defined.
    protected static void doBaseTestSuiteSetup() throws Exception {
        // shouldn't have any, but just in case
        dropTestObjects();
        String name = Thread.currentThread().getStackTrace()[2].toString();
        name = name.substring(0, name.indexOf(".doTestSuiteSetup")); 
        System.out.println(name);
    }

    @AfterClass
    public static void doBaseTestSuiteCleanup() throws Exception {
        // do nothing for now
    }

    @Before
    public void doBaseTestSetup() throws Exception {
        conn = getConnection();
        if (tgtSQ())
           createCatalogSchemaIfNotExist();
    }

    @After
    public void doBaseTestCleanup() throws Exception {
        conn.close();
        dropTestObjects();
    }

    private static Connection baseConn = null;

    // For SQ only, create catalog and schema in case they don't exist.
    private void createCatalogSchemaIfNotExist() throws Exception {
        assertNotNull(conn);
         
        try {
            conn.createStatement().execute("create catalog " + my_catalog);
        } catch (Exception e) {
            // Do nothing, the catalog may already exist.
        }

        try {
            conn.createStatement().execute("create schema " + my_catalog + "." + my_schema);
        } catch (Exception e) {
            // Do nothing, the schema may already exist.
        }
    }

    protected static void dropTestObjects() throws Exception {

        // Use our own conn.  Who knows what the tests have been doing with
        // auto commit of the conn that it has been using.
        if (baseConn == null)
            baseConn = getConnection();

        if (objDropList == null)
            return;

        for (String objname : objDropList) {
            for (int i = 0; i < 3; i++) {
                try {
                    // shouldn't make any difference, but some people
                    // insist dropping object should use executeUpdate()
                    // instead of execute()
                    baseConn.createStatement().executeUpdate("drop " + objname + " cascade");
                    break; // no execption, break out here
                } catch (Exception e) {
                    String msg = e.getMessage();
                    if ((msg.contains("ERROR[1002]") &&
                         msg.contains("does not exist")) ||
                        (msg.contains("ERROR[1003]") && 
                         msg.contains("does not exist")) ||
                        (msg.contains("ERROR[1004]") &&
                         msg.contains("does not exist"))) {
                        // ERROR[1002]: catalog does not exist in SQ, 
                        // ERROR[1003]: schema does not exist in SQ,
                        // ERROR[1004]: schema does not exist in SQ,
                        // we are done these cases.
                        break;
                    } 
                    else if (msg.contains("ERROR[1389]") &&
                        msg.contains("does not exist")) {
                        // object does not exist in TRAF, we are done.
                        break;
                    }
                    else if (i < 2 &&
                        msg.contains("ERROR[1183]") &&
                        msg.contains("Error 73")) {
                        // error 73 should be reried up to 3 times.
                        Thread.sleep(2000); // 2 secs
                        System.out.println("see error 73, retrying...");
                    } else {
                        // all rest are bad.
                        System.out.println(msg);
                        fail("Failed to drop object: " + objname);
                    }
                }
            }
        }
    }


    protected static final String tenantId = "TRAF";

    protected static final String CF_NAME = "a";
    // protected static final byte[] CF = Bytes.toBytes(CF_NAME);

    protected static final String CF2_NAME = "b";

    protected final static String A_VALUE = "a";
    // protected final static byte[] A = Bytes.toBytes(A_VALUE);
    protected final static String B_VALUE = "b";
    // protected final static byte[] B = Bytes.toBytes(B_VALUE);
    protected final static String C_VALUE = "c";
    // protected final static byte[] C = Bytes.toBytes(C_VALUE);
    protected final static String D_VALUE = "d";
    // protected final static byte[] D = Bytes.toBytes(D_VALUE);
    protected final static String E_VALUE = "e";
    // protected final static byte[] E = Bytes.toBytes(E_VALUE);

    protected final static String ROW1 = "00A123122312312";
    protected final static String ROW2 = "00A223122312312";
    protected final static String ROW3 = "00A323122312312";
    protected final static String ROW4 = "00A423122312312";
    protected final static String ROW5 = "00B523122312312";
    protected final static String ROW6 = "00B623122312312";
    protected final static String ROW7 = "00B723122312312";
    protected final static String ROW8 = "00B823122312312";
    protected final static String ROW9 = "00C923122312312";

    protected static final long MILLIS_IN_DAY = 1000 * 60 * 60 * 24;

    protected static final String ATABLE_NAME = "ATABLE";
    protected static final String SUM_DOUBLE_NAME = "SumDoubleTest";
    protected static final String ATABLE_SCHEMA_NAME = "";
    protected static final String BTABLE_NAME = "BTABLE";
    protected static final String STABLE_NAME = "STABLE";
    protected static final String STABLE_SCHEMA_NAME = "";
    protected static final String GROUPBYTEST_NAME = "GROUPBYTEST";
    protected static final String CUSTOM_ENTITY_DATA_FULL_NAME = "CUSTOM_ENTITY_DATA";
    protected static final String CUSTOM_ENTITY_DATA_NAME = "CUSTOM_ENTITY_DATA";
    protected static final String CUSTOM_ENTITY_DATA_SCHEMA_NAME = "CORE";
    protected static final String HBASE_NATIVE = "HBASE_NATIVE";
    protected static final String HBASE_DYNAMIC_COLUMNS = "HBASE_DYNAMIC_COLUMNS";
    protected static final String PRODUCT_METRICS_NAME = "PRODUCT_METRICS";
    protected static final String PTSDB_NAME = "PTSDB";
    protected static final String PTSDB2_NAME = "PTSDB2";
    protected static final String PTSDB3_NAME = "PTSDB3";
    protected static final String PTSDB_SCHEMA_NAME = "";
    protected static final String FUNKY_NAME = "FUNKY_NAMES";
    protected static final String MULTI_CF_NAME = "MULTI_CF";
    protected static final String MDTEST_NAME = "MDTEST";
    protected static final String KEYONLY_NAME = "KEYONLY";
    protected static final String TABLE_WITH_SALTING = "TABLE_WITH_SALTING";
    protected static final String INDEX_DATA_SCHEMA = "INDEX_TEST";
    protected static final String INDEX_DATA_TABLE = "INDEX_DATA_TABLE";

    protected static final String NAME_SEPARATOR = ".";

    private static final Map<String,String> tableDDLMap_PH;
    private static final Map<String,String> tableDDLMap_TR;
    private static final Map<String,String> tableDDLMap_SQ;
    static {
        ImmutableMap.Builder<String,String> builder_PH = ImmutableMap.builder();
        ImmutableMap.Builder<String,String> builder_TR = ImmutableMap.builder();
        ImmutableMap.Builder<String,String> builder_SQ = ImmutableMap.builder();

        builder_PH.put(ATABLE_NAME,"create table " + ATABLE_NAME +
                "   (organization_id char(15) not null, \n" +
                "    entity_id char(15) not null,\n" +
                "    a_string varchar(100),\n" +
                "    b_string varchar(100),\n" +
                "    a_integer integer,\n" +
                "    a_date date,\n" +
                "    a_time time,\n" +
                "    a_timestamp timestamp,\n" +
                "    x_decimal decimal(31,10),\n" +
                "    x_long bigint,\n" +
                "    x_integer integer,\n" +
                "    y_integer integer,\n" +
                "    a_byte tinyint,\n" +
                "    a_short smallint,\n" +
                "    a_float float,\n" +
                "    a_double double,\n" +
                "    a_unsigned_float unsigned_float,\n" +
                "    a_unsigned_double unsigned_double\n" +
                "    CONSTRAINT pk PRIMARY KEY (organization_id, entity_id)\n" +
                ")");
        builder_TR.put(ATABLE_NAME,"create table " + ATABLE_NAME +
                "   (organization_id char(15) not null, \n" +
                "    entity_id char(15) not null,\n" +
                "    a_string varchar(100),\n" +
                "    b_string varchar(100),\n" +
                "    a_integer integer,\n" +
                "    a_date date,\n" +
                "    a_time time,\n" +
                "    a_timestamp timestamp,\n" +
/* TRAF */      "    x_decimal decimal(18,10),\n" +
                "    x_long bigint,\n" +
                "    x_integer integer,\n" +
                "    y_integer integer,\n" +
                "    a_byte tinyint,\n" +
                "    a_short smallint,\n" +
                "    a_float float,\n" +
/* TRAF */      "    a_double double precision,\n" +
/* TRAF */      "    a_unsigned_float float,\n" +
/* TRAF */      "    a_unsigned_double double precision\n" +
                "    , CONSTRAINT pk_atable PRIMARY KEY (organization_id, entity_id)\n" +
                ")");
        builder_SQ.put(ATABLE_NAME,"create table " + ATABLE_NAME +
                "   (organization_id char(15) not null, \n" +
                "    entity_id char(15) not null,\n" +
                "    a_string varchar(100),\n" +
                "    b_string varchar(100),\n" +
                "    a_integer integer,\n" +
                "    a_date date,\n" +
                "    a_time time,\n" +
                "    a_timestamp timestamp,\n" +
/* TRAF */      "    x_decimal decimal(18,10),\n" +
                "    x_long bigint,\n" +
                "    x_integer integer,\n" +
                "    y_integer integer,\n" +
                "    a_byte tinyint,\n" +
                "    a_short smallint,\n" +
                "    a_float float,\n" +
/* TRAF */      "    a_double double precision,\n" +
/* TRAF */      "    a_unsigned_float float,\n" +
/* TRAF */      "    a_unsigned_double double precision\n" +
                "    , CONSTRAINT pk_atable PRIMARY KEY (organization_id, entity_id)\n" +
                ")");
        builder_PH.put(BTABLE_NAME,"create table " + BTABLE_NAME +
                "   (a_string varchar not null, \n" +
                "    a_id char(3) not null,\n" +
                "    b_string varchar not null, \n" +
                "    a_integer integer not null, \n" +
                "    c_string varchar(2) null,\n" +
                "    b_integer integer,\n" +
                "    c_integer integer,\n" +
                "    d_string varchar(3),\n" +
                "    e_string char(10)\n" +
                "    CONSTRAINT my_pk PRIMARY KEY (a_string,a_id,b_string,a_integer,c_string))");
        builder_TR.put(BTABLE_NAME,"create table " + BTABLE_NAME +
/* TRAF */      "   (a_string varchar(128) not null, \n" +
                "    a_id char(3) not null,\n" +
/* TRAF */      "    b_string varchar(128) not null, \n" +
                "    a_integer integer not null, \n" +
/* TRAF */      "    c_string varchar(2) not null,\n" +
                "    b_integer integer,\n" +
                "    c_integer integer,\n" +
                "    d_string varchar(3),\n" +
                "    e_string char(10)\n" +
/* TRAF */      "   , CONSTRAINT pk_btable PRIMARY KEY (a_string,a_id,b_string,a_integer,c_string))");
        builder_SQ.put(BTABLE_NAME,"create table " + BTABLE_NAME +
/* TRAF */      "   (a_string varchar(128) not null, \n" +
                "    a_id char(3) not null,\n" +
/* TRAF */      "    b_string varchar(128) not null, \n" +
                "    a_integer integer not null, \n" +
/* TRAF */      "    c_string varchar(2) not null,\n" +
                "    b_integer integer,\n" +
                "    c_integer integer,\n" +
                "    d_string varchar(3),\n" +
                "    e_string char(10)\n" +
/* TRAF */      "   , CONSTRAINT pk_btable PRIMARY KEY (a_string,a_id,b_string,a_integer,c_string))");
        builder_PH.put(TABLE_WITH_SALTING,"create table " + TABLE_WITH_SALTING +
                "   (a_integer integer not null, \n" +
                "    a_string varchar not null, \n" +
                "    a_id char(3) not null,\n" +
                "    b_string varchar, \n" +
                "    b_integer integer \n" +
                "    CONSTRAINT pk PRIMARY KEY (a_integer, a_string, a_id))\n" +
                "    SALT_BUCKETS = 4");
        builder_TR.put(TABLE_WITH_SALTING,"create table " + TABLE_WITH_SALTING +
                "   (a_integer integer not null, \n" +
/* TRAF */      "    a_string varchar(128) not null, \n" +
                "    a_id char(3) not null,\n" +
/* TRAF */      "    b_string varchar(128), \n" +
                "    b_integer integer \n" +
/* TRAF */      "    , CONSTRAINT pk_table_with_salting PRIMARY KEY (a_integer, a_string, a_id))\n" +
/* TRAF */      "");
        builder_SQ.put(TABLE_WITH_SALTING,"create table " + TABLE_WITH_SALTING +
                "   (a_integer integer not null, \n" +
/* TRAF */      "    a_string varchar(128) not null, \n" +
                "    a_id char(3) not null,\n" +
/* TRAF */      "    b_string varchar(128), \n" +
                "    b_integer integer \n" +
/* TRAF */      "    , CONSTRAINT pk_table_with_salting PRIMARY KEY (a_integer, a_string, a_id))\n" +
// TRAF         "    SALT_BUCKETS = 4");
/* TRAF */      "");
        builder_PH.put(PTSDB_NAME,"create table " + PTSDB_NAME +
                "   (inst varchar null,\n" +
                "    host varchar null,\n" +
                "    date date not null,\n" +
                "    val decimal(31,10)\n" +
                "    CONSTRAINT pk PRIMARY KEY (inst, host, date))");
        builder_TR.put(PTSDB_NAME,"create table " + PTSDB_NAME +
/* TRAF */      "   (inst varchar(128) default null,\n" +
/* TRAF */      "    host1 varchar(128) default null,\n" +
/* TRAF */      "    date1 timestamp not null,\n" +
/* TRAF */      "    val decimal(18,10)\n" +
// TRAF: change the primary keys to clustering key.  Some of the columns allow null, which can't be used as primary keys in SQ.  date1 alone is not unique enough to be a primary key.
/* TRAF */      "    ) STORE BY (date1)");
        builder_SQ.put(PTSDB_NAME,"create table " + PTSDB_NAME +
/* TRAF */      "   (inst varchar(128) default null,\n" +
/* TRAF */      "    host1 varchar(128) default null,\n" +
/* TRAF */      "    date1 timestamp not null,\n" +
/* TRAF */      "    val decimal(18,10)\n" +
// TRAF: change the primary keys to clustering key.  Some of the columns allow null, which can't be used as primary keys in SQ.  date1 alone is not unique enough to be a primary key.
/* TRAF */      "    ) STORE BY (date1)");
        builder_PH.put(PTSDB2_NAME,"create table " + PTSDB2_NAME +
                "   (inst varchar(10) not null,\n" +
                "    date date not null,\n" +
                "    val1 decimal,\n" +
                "    val2 decimal(31,10),\n" +
                "    val3 decimal\n" +
                "    CONSTRAINT pk PRIMARY KEY (inst, date))");
        builder_TR.put(PTSDB2_NAME,"create table " + PTSDB2_NAME +
                "   (inst varchar(10) not null,\n" +
/* TRAF */      "    date1 timestamp not null,\n" +
                "    val1 decimal,\n" +
/* TRAF */      "    val2 decimal(18,10),\n" +
                "    val3 decimal\n" +
/* TRAF */      "    , CONSTRAINT pk PRIMARY KEY (inst, date1))");
        builder_SQ.put(PTSDB2_NAME,"create table " + PTSDB2_NAME +
                "   (inst varchar(10) not null,\n" +
/* TRAF */      "    date1 timestamp not null,\n" +
                "    val1 decimal,\n" +
/* TRAF */      "    val2 decimal(18,10),\n" +
                "    val3 decimal\n" +
/* TRAF */      "    , CONSTRAINT pk PRIMARY KEY (inst, date1))");
        builder_PH.put(PTSDB3_NAME,"create table " + PTSDB3_NAME +
                "   (host varchar(10) not null,\n" +
                "    date date not null,\n" +
                "    val1 decimal,\n" +
                "    val2 decimal(31,10),\n" +
                "    val3 decimal\n" +
                "    CONSTRAINT pk PRIMARY KEY (host DESC, date DESC))");
        builder_TR.put(PTSDB3_NAME,"create table " + PTSDB3_NAME +
/* TRAF */      "   (host1 varchar(10) not null,\n" +
/* TRAF */      "    date1 timestamp not null,\n" +   
                "    val1 decimal,\n" +
/* TRAF */      "    val2 decimal(18,10),\n" + 
                "    val3 decimal\n" +
/* TRAF */      "    , CONSTRAINT pk PRIMARY KEY (host1 DESC, date1 DESC))");
        builder_SQ.put(PTSDB3_NAME,"create table " + PTSDB3_NAME +
/* TRAF */      "   (host1 varchar(10) not null,\n" +
/* TRAF */      "    date1 timestamp not null,\n" +
                "    val1 decimal,\n" +
/* TRAF */      "    val2 decimal(18,10),\n" +
                "    val3 decimal\n" +
/* TRAF */      "    , CONSTRAINT pk PRIMARY KEY (host1 DESC, date1 DESC))");
        builder_PH.put(FUNKY_NAME,"create table " + FUNKY_NAME +
                "   (\"foo!\" varchar not null primary key,\n" +
                "    \"1\".\"#@$\" varchar, \n" +
                "    \"1\".\"foo.bar-bas\" varchar, \n" +
                "    \"1\".\"Value\" integer,\n" +
                "    \"1\".\"VALUE\" integer,\n" +
                "    \"1\".\"value\" integer,\n" +
                "    \"1\".\"_blah^\" varchar)"
                ); 
        builder_TR.put(FUNKY_NAME,"create table " + FUNKY_NAME +
/* TRAF */      "   (\"foo!\" varchar(128) not null primary key,\n" +
/* TRAF */      "    \"#@$\" varchar(128), \n" +
/* TRAF */      "    \"foo.bar-bas\" varchar(128), \n" +
/* TRAF */      "    \"Value\" integer,\n" +
/* TRAF */      "    \"VALUE\" integer,\n" +
/* TRAF */      "    \"value\" integer,\n" +
/* TRAF */      "    \"_blah^\" varchar(128))"
                );
        builder_SQ.put(FUNKY_NAME,"create table " + FUNKY_NAME +
/* TRAF */      "   (\"foo!\" varchar(128) not null primary key,\n" +
/* TRAF */      "    \"#@$\" varchar(128), \n" +
/* TRAF */      "    \"foo.bar-bas\" varchar(128), \n" +
/* TRAF */      "    \"Value\" integer,\n" +
/* TRAF */      "    \"VALUE\" integer,\n" +
/* TRAF */      "    \"value\" integer,\n" +
/* TRAF */      "    \"_blah^\" varchar(128))"
                );
        builder_PH.put(KEYONLY_NAME,"create table " + KEYONLY_NAME +
                "   (i1 integer not null, i2 integer not null\n" +
                "    CONSTRAINT pk PRIMARY KEY (i1,i2))");
        builder_TR.put(KEYONLY_NAME,"create table " + KEYONLY_NAME +
                "   (i1 integer not null, i2 integer not null\n" +
                "    , CONSTRAINT pk_keyonly PRIMARY KEY (i1,i2))");
        builder_SQ.put(KEYONLY_NAME,"create table " + KEYONLY_NAME +
                "   (i1 integer not null, i2 integer not null\n" +
                "    , CONSTRAINT pk_keyonly PRIMARY KEY (i1,i2))");
        builder_PH.put(MULTI_CF_NAME,"create table " + MULTI_CF_NAME +
                "   (id char(15) not null primary key,\n" +
                "    a.unique_user_count integer,\n" +
                "    b.unique_org_count integer,\n" +
                "    c.db_cpu_utilization decimal(31,10),\n" +
                "    d.transaction_count bigint,\n" +
                "    e.cpu_utilization decimal(31,10),\n" +
                "    f.response_time bigint,\n" +
                "    g.response_time bigint)");
        builder_TR.put(MULTI_CF_NAME,"create table " + MULTI_CF_NAME +
                "   (id char(15) not null primary key,\n" +
/* TRAF */      "    unique_user_count integer,\n" +
/* TRAF */      "    unique_org_count integer,\n" +
/* TRAF */      "    db_cpu_utilization decimal(18,10),\n" +
/* TRAF */      "    transaction_count bigint,\n" +
/* TRAF */      "    cpu_utilization decimal(18,10),\n" +
/* TRAF */      "    f_response_time bigint,\n" +
/* TRAF */      "    g_response_time bigint)");
        builder_SQ.put(MULTI_CF_NAME,"create table " + MULTI_CF_NAME +
                "   (id char(15) not null primary key,\n" +
/* TRAF */      "    unique_user_count integer,\n" +
/* TRAF */      "    unique_org_count integer,\n" +
/* TRAF */      "    db_cpu_utilization decimal(18,10),\n" +
/* TRAF */      "    transaction_count bigint,\n" +
/* TRAF */      "    cpu_utilization decimal(18,10),\n" +
/* TRAF */      "    f_response_time bigint,\n" +
/* TRAF */      "    g_response_time bigint)");
        builder_PH.put(GROUPBYTEST_NAME,"create table " + GROUPBYTEST_NAME +
                "   (id varchar not null primary key,\n" +
                "    uri varchar, appcpu integer)");
        builder_TR.put(GROUPBYTEST_NAME,"create table " + GROUPBYTEST_NAME +
/* TRAF */      "   (id varchar(128) not null primary key,\n" +
/* TRAF */      "    uri varchar(128), appcpu integer)");
        builder_SQ.put(GROUPBYTEST_NAME,"create table " + GROUPBYTEST_NAME +
/* TRAF */      "   (id varchar(128) not null primary key,\n" +
/* TRAF */      "    uri varchar(128), appcpu integer)");
        builder_PH.put(PRODUCT_METRICS_NAME,"create table " + PRODUCT_METRICS_NAME +
                "   (organization_id char(15) not null," +
                "    date date not null," +
                "    feature char(1) not null," +
                "    unique_users integer not null,\n" +
                "    db_utilization decimal(31,10),\n" +
                "    transactions bigint,\n" +
                "    cpu_utilization decimal(31,10),\n" +
                "    response_time bigint,\n" +
                "    io_time bigint,\n" +
                "    region varchar,\n" +
                "    unset_column decimal(31,10)\n" +
                "    CONSTRAINT pk PRIMARY KEY (organization_id, DATe, feature, UNIQUE_USERS))"); 
        builder_TR.put(PRODUCT_METRICS_NAME,"create table " + PRODUCT_METRICS_NAME +
                "   (organization_id char(15) not null," +
/* TRAF */      "    date1 timestamp not null," +
                "    feature char(1) not null," +
                "    unique_users integer not null,\n" +
/* TRAF */      "    db_utilization decimal(18,10),\n" +
                "    transactions bigint,\n" +
/* TRAF */      "    cpu_utilization decimal(18,10),\n" +
                "    response_time bigint,\n" +
                "    io_time bigint,\n" +
/* TRAF */      "    region varchar(128),\n" +
/* TRAF */      "    unset_column decimal(18,10)\n" +
/* TRAF */      "    , CONSTRAINT pk_product_matrics PRIMARY KEY (organization_id, DATe1, feature, UNIQUE_USERS))"); 
        builder_SQ.put(PRODUCT_METRICS_NAME,"create table " + PRODUCT_METRICS_NAME +
                "   (organization_id char(15) not null," +
/* TRAF */      "    date1 timestamp not null," +
                "    feature char(1) not null," +
                "    unique_users integer not null,\n" +
/* TRAF */      "    db_utilization decimal(18,10),\n" +
                "    transactions bigint,\n" +
/* TRAF */      "    cpu_utilization decimal(18,10),\n" +
                "    response_time bigint,\n" +
                "    io_time bigint,\n" +
/* TRAF */      "    region varchar(128),\n" +
/* TRAF */      "    unset_column decimal(18,10)\n" +
/* TRAF */      "    , CONSTRAINT pk_product_matrics PRIMARY KEY (organization_id, DATe1, feature, UNIQUE_USERS))");
        builder_PH.put(CUSTOM_ENTITY_DATA_FULL_NAME,"create table " + CUSTOM_ENTITY_DATA_FULL_NAME +
                "   (organization_id char(15) not null, \n" +
                "    key_prefix char(3) not null,\n" +
                "    custom_entity_data_id char(12) not null,\n" +
                "    created_by varchar,\n" +
                "    created_date date,\n" +
                "    currency_iso_code char(3),\n" +
                "    deleted char(1),\n" +
                "    division decimal(31,10),\n" +
                "    last_activity date,\n" +
                "    last_update date,\n" +
                "    last_update_by varchar,\n" +
                "    name varchar(240),\n" +
                "    owner varchar,\n" +
                "    record_type_id char(15),\n" +
                "    setup_owner varchar,\n" +
                "    system_modstamp date,\n" +
                "    b.val0 varchar,\n" +
                "    b.val1 varchar,\n" +
                "    b.val2 varchar,\n" +
                "    b.val3 varchar,\n" +
                "    b.val4 varchar,\n" +
                "    b.val5 varchar,\n" +
                "    b.val6 varchar,\n" +
                "    b.val7 varchar,\n" +
                "    b.val8 varchar,\n" +
                "    b.val9 varchar\n" +
                "    CONSTRAINT pk PRIMARY KEY (organization_id, key_prefix, custom_entity_data_id))");
        builder_TR.put(CUSTOM_ENTITY_DATA_FULL_NAME,"create table " + CUSTOM_ENTITY_DATA_FULL_NAME +
                "   (organization_id char(15) not null, \n" +
                "    key_prefix char(3) not null,\n" +
                "    custom_entity_data_id char(12) not null,\n" +
/* TRAF */      "    created_by varchar(128),\n" +
                "    created_date date,\n" +
                "    currency_iso_code char(3),\n" +
                "    deleted char(1),\n" +
/* TRAF */      "    division decimal(18,10),\n" +
                "    last_activity date,\n" +
                "    last_update date,\n" +
/* TRAF */      "    last_update_by varchar(128),\n" +
                "    name varchar(240),\n" +
/* TRAF */      "    owner varchar(128),\n" +
                "    record_type_id char(15),\n" +
/* TRAF */      "    setup_owner varchar(128),\n" +
                "    system_modstamp date,\n" +
/* TRAF */      "    val0 varchar(128),\n" +
/* TRAF */      "    val1 varchar(128),\n" +
/* TRAF */      "    val2 varchar(128),\n" +
/* TRAF */      "    val3 varchar(128),\n" +
/* TRAF */      "    val4 varchar(128),\n" +
/* TRAF */      "    val5 varchar(128),\n" +
/* TRAF */      "    val6 varchar(128),\n" +
/* TRAF */      "    val7 varchar(128),\n" +
/* TRAF */      "    val8 varchar(128),\n" +
/* TRAF */      "    val9 varchar(128)\n" +
/* TRAF */      "    , CONSTRAINT pk_custom_entity_data PRIMARY KEY (organization_id, key_prefix, custom_entity_data_id))");
        builder_SQ.put(CUSTOM_ENTITY_DATA_FULL_NAME,"create table " + CUSTOM_ENTITY_DATA_FULL_NAME +
                "   (organization_id char(15) not null, \n" +
                "    key_prefix char(3) not null,\n" +
                "    custom_entity_data_id char(12) not null,\n" +
/* TRAF */      "    created_by varchar(128),\n" +
                "    created_date date,\n" +
                "    currency_iso_code char(3),\n" +
                "    deleted char(1),\n" +
/* TRAF */      "    division decimal(18,10),\n" +
                "    last_activity date,\n" +
                "    last_update date,\n" +
/* TRAF */      "    last_update_by varchar(128),\n" +
                "    name varchar(240),\n" +
/* TRAF */      "    owner varchar(128),\n" +
                "    record_type_id char(15),\n" +
/* TRAF */      "    setup_owner varchar(128),\n" +
                "    system_modstamp date,\n" +
/* TRAF */      "    val0 varchar(128),\n" +
/* TRAF */      "    val1 varchar(128),\n" +
/* TRAF */      "    val2 varchar(128),\n" +
/* TRAF */      "    val3 varchar(128),\n" +
/* TRAF */      "    val4 varchar(128),\n" +
/* TRAF */      "    val5 varchar(128),\n" +
/* TRAF */      "    val6 varchar(128),\n" +
/* TRAF */      "    val7 varchar(128),\n" +
/* TRAF */      "    val8 varchar(128),\n" +
/* TRAF */      "    val9 varchar(128)\n" +
/* TRAF */      "    , CONSTRAINT pk_custom_entity_data PRIMARY KEY (organization_id, key_prefix, custom_entity_data_id))");
        builder_PH.put("IntKeyTest","create table IntKeyTest" +
                "   (i integer not null primary key)");
        builder_TR.put("IntKeyTest","create table IntKeyTest" +
                "   (i integer not null primary key)");
        builder_SQ.put("IntKeyTest","create table IntKeyTest" +
                "   (i integer not null primary key)");
        builder_PH.put("IntIntKeyTest","create table IntIntKeyTest" +
                "   (i integer not null primary key, j integer)");
        builder_TR.put("IntIntKeyTest","create table IntIntKeyTest" +
                "   (i integer not null primary key, j integer)");
        builder_SQ.put("IntIntKeyTest","create table IntIntKeyTest" +
                "   (i integer not null primary key, j integer)");
        builder_PH.put("LongInKeyTest","create table LongInKeyTest" +
                "   (l bigint not null primary key)");
        builder_TR.put("LongInKeyTest","create table LongInKeyTest" +
                "   (l bigint not null primary key)");
        builder_SQ.put("LongInKeyTest","create table LongInKeyTest" +
                "   (l bigint not null primary key)");
        builder_PH.put("PKIntValueTest", "create table PKIntValueTest" +
                "   (pk integer not null primary key)");
        builder_TR.put("PKIntValueTest", "create table PKIntValueTest" +
                "   (pk integer not null primary key)");
        builder_SQ.put("PKIntValueTest", "create table PKIntValueTest" +
                "   (pk integer not null primary key)");
        builder_PH.put("PKBigIntValueTest", "create table PKBigIntValueTest" +
                "   (pk bigint not null primary key)");
        builder_TR.put("PKBigIntValueTest", "create table PKBigIntValueTest" +
                "   (pk bigint not null primary key)");
        builder_SQ.put("PKBigIntValueTest", "create table PKBigIntValueTest" +
                "   (pk bigint not null primary key)");
        builder_PH.put("PKUnsignedIntValueTest", "create table PKUnsignedIntValueTest" +
                "   (pk unsigned_int not null primary key)");
        builder_TR.put("PKUnsignedIntValueTest", "create table PKUnsignedIntValueTest" +
                "   (pk unsigned_int not null primary key)");
        builder_SQ.put("PKUnsignedIntValueTest", "create table PKUnsignedIntValueTest" +
                "   (pk unsigned_int not null primary key)");
        builder_PH.put("PKUnsignedLongValueTest", "create table PKUnsignedLongValueTest" +
                "   (pk unsigned_long not null\n" +
                "    CONSTRAINT pk PRIMARY KEY (pk))");
        builder_TR.put("PKUnsignedLongValueTest", "create table PKUnsignedLongValueTest" +
                "   (pk unsigned_long not null\n" +
                "    CONSTRAINT pk PRIMARY KEY (pk))");
        builder_SQ.put("PKUnsignedLongValueTest", "create table PKUnsignedLongValueTest" +
                "   (pk unsigned_long not null\n" +
                "    CONSTRAINT pk PRIMARY KEY (pk))");
        builder_PH.put("KVIntValueTest", "create table KVIntValueTest" +
                "   (pk integer not null primary key,\n" +
                "    kv integer)\n");
        builder_TR.put("KVIntValueTest", "create table KVIntValueTest" +
                "   (pk integer not null primary key,\n" +
                "    kv integer)\n");
        builder_SQ.put("KVIntValueTest", "create table KVIntValueTest" +
                "   (pk integer not null primary key,\n" +
                "    kv integer)\n");
        builder_PH.put("KVBigIntValueTest", "create table KVBigIntValueTest" +
                "   (pk integer not null primary key,\n" +
                "    kv bigint)\n");
        builder_TR.put("KVBigIntValueTest", "create table KVBigIntValueTest" +
                "   (pk integer not null primary key,\n" +
                "    kv bigint)\n");
        builder_SQ.put("KVBigIntValueTest", "create table KVBigIntValueTest" +
                "   (pk integer not null primary key,\n" +
                "    kv bigint)\n");
        builder_PH.put(INDEX_DATA_TABLE, "create table " + INDEX_DATA_SCHEMA + NAME_SEPARATOR + INDEX_DATA_TABLE + "(" +
                "   varchar_pk VARCHAR NOT NULL, " +
                "   char_pk CHAR(5) NOT NULL, " +
                "   int_pk INTEGER NOT NULL, "+
                "   long_pk BIGINT NOT NULL, " +
                "   decimal_pk DECIMAL(31, 10) NOT NULL, " +
                "   a.varchar_col1 VARCHAR, " +
                "   a.char_col1 CHAR(5), " +
                "   a.int_col1 INTEGER, " +
                "   a.long_col1 BIGINT, " +
                "   a.decimal_col1 DECIMAL(31, 10), " +
                "   b.varchar_col2 VARCHAR, " +
                "   b.char_col2 CHAR(5), " +
                "   b.int_col2 INTEGER, " +
                "   b.long_col2 BIGINT, " +
                "   b.decimal_col2 DECIMAL(31, 10) " +
                "   CONSTRAINT pk PRIMARY KEY (varchar_pk, char_pk, int_pk, long_pk DESC, decimal_pk)) " +
                "IMMUTABLE_ROWS=true");
        builder_TR.put(INDEX_DATA_TABLE, "create table " + INDEX_DATA_TABLE + "(" +
/* TRAF */      "   varchar_pk VARCHAR(128) NOT NULL, " +
                "   char_pk CHAR(5) NOT NULL, " +
                "   int_pk INTEGER NOT NULL, "+
                "   long_pk BIGINT NOT NULL, " +
/* TRAF */      "   decimal_pk DECIMAL(18, 10) NOT NULL, " +
/* TRAF */      "   varchar_col1 VARCHAR(128), " +
/* TRAF */      "   char_col1 CHAR(5), " +
/* TRAF */      "   int_col1 INTEGER, " +
/* TRAF */      "   long_col1 BIGINT, " +
/* TRAF */      "   decimal_col1 DECIMAL(18, 10), " +
/* TRAF */      "   varchar_col2 VARCHAR(128), " +
/* TRAF */      "   char_col2 CHAR(5), " +
/* TRAF */      "   int_col2 INTEGER, " +
/* TRAF */      "   long_col2 BIGINT, " +
/* TRAF */      "   decimal_col2 DECIMAL(18, 10) " +
/* TRAF */      "   , CONSTRAINT pk PRIMARY KEY (varchar_pk, char_pk, int_pk, long_pk DESC, decimal_pk)) " +
/* TRAF */      "");
        builder_SQ.put(INDEX_DATA_TABLE, "create table " + INDEX_DATA_TABLE + "(" +
/* TRAF */      "   varchar_pk VARCHAR(128) NOT NULL, " +
                "   char_pk CHAR(5) NOT NULL, " +
                "   int_pk INTEGER NOT NULL, "+
                "   long_pk BIGINT NOT NULL, " +
/* TRAF */      "   decimal_pk DECIMAL(18, 10) NOT NULL, " +
/* TRAF */      "   varchar_col1 VARCHAR(128), " +
/* TRAF */      "   char_col1 CHAR(5), " +
/* TRAF */      "   int_col1 INTEGER, " +
/* TRAF */      "   long_col1 BIGINT, " +
/* TRAF */      "   decimal_col1 DECIMAL(18, 10), " +
/* TRAF */      "   varchar_col2 VARCHAR(128), " +
/* TRAF */      "   char_col2 CHAR(5), " +
/* TRAF */      "   int_col2 INTEGER, " +
/* TRAF */      "   long_col2 BIGINT, " +
/* TRAF */      "   decimal_col2 DECIMAL(18, 10) " +
/* TRAF */      "   , CONSTRAINT pk PRIMARY KEY (varchar_pk, char_pk, int_pk, long_pk DESC, decimal_pk)) " +
/* TRAF */      "");
        builder_PH.put("SumDoubleTest","create table SumDoubleTest" +
                "   (id varchar not null primary key, d DOUBLE, f FLOAT, ud UNSIGNED_DOUBLE, uf UNSIGNED_FLOAT, i integer, de decimal)");
        builder_TR.put("SumDoubleTest","create table SumDoubleTest" +
/* TRAF */      "   (id varchar(128) not null primary key, d DOUBLE PRECISION, f FLOAT, ud DOUBLE PRECISION, uf FLOAT, i integer, de decimal)");
        builder_SQ.put("SumDoubleTest","create table SumDoubleTest" +
/* TRAF */      "   (id varchar(128) not null primary key, d DOUBLE PRECISION, f FLOAT, ud DOUBLE PRECISION, uf FLOAT, i integer, de decimal)");

        tableDDLMap_PH = builder_PH.build();
        tableDDLMap_TR = builder_TR.build();
        tableDDLMap_SQ = builder_SQ.build();
    }

    protected void printTestDescription() {
        if (print_testinfo) {
            System.out.println(Thread.currentThread().getStackTrace()[2]);
        }
    }

    private static String url = null;

    protected static String getUrl() { return url; }

    private static boolean driverRegistered = false;

    private static void doConnectionSetup(Connection conn) throws Exception {
        assertNotNull(conn);

        if (tgtTR()) {
            conn.createStatement().execute("control query default mode_seabase 'on'");
        }
    }

    protected static Connection getConnection() throws Exception {
        Connection conn = null;
        try {
            String propFile = System.getProperty("hpjdbc.properties");
            assertNotNull(propFile);
            FileInputStream fs = new FileInputStream(new File(propFile));
            Properties props = new Properties();
            props.load(fs);

            url = props.getProperty("url");
            my_catalog = props.getProperty("catalog");
            my_schema = props.getProperty("schema");
            String tgtType = props.getProperty("targettype");
            assertTrue(tgtType.equals("") || tgtType.equals("SQ") || tgtType.equals("TR"));
            if (tgtType.equals("SQ"))
                setTgtSQ();
            else if (tgtType.equals("TR"))
                setTgtTR();
           
            // Reigser our JDBC driver if this is the first call.           
            if (! driverRegistered) {
                if (url.contains("t4jdbc"))
                    Class.forName("org.trafodion.jdbc.t4.T4Driver");   // T4 driver
                else
                    Class.forName("org.trafodion.jdbc.t2.T2Driver"); // T2 driver
                driverRegistered = true;
            }

            // System.out.println("connecting to " + url);
            conn = DriverManager.getConnection(url, props);
        } catch (Exception e) {
            conn = null;
            fail(e.getMessage());
        }

        doConnectionSetup(conn);

        return conn;
    }

    protected void createTestTable(String tableName) {
        assertNotNull(conn);
        String ddl = null;
        if (tgtPH()) ddl = tableDDLMap_PH.get(tableName);
        else if (tgtTR()) ddl = tableDDLMap_TR.get(tableName);
        else if (tgtSQ()) ddl = tableDDLMap_SQ.get(tableName);
        assertNotNull(ddl);
        StringBuilder buf = new StringBuilder(ddl);
        ddl = buf.toString();

        try {
            conn.createStatement().execute(ddl);
        } catch (Exception e) { 
            System.out.println(e.getMessage());
            fail("Failed to create table");
        }
    }

    protected void initSumDoubleValues() throws Exception {
        assertNotNull(conn);
        createTestTable("SumDoubleTest");
        try {
            // Insert all rows at ts
            PreparedStatement stmt = null;
            if (tgtPH()||tgtTR()) stmt = conn.prepareStatement(
                    "upsert into " +
                    "SumDoubleTest(" +
                    "    id, " +
                    "    d, " +
                    "    f, " +
                    "    ud, " +
                    "    uf) " +
                    "VALUES (?, ?, ?, ?, ?)");
            else if (tgtSQ()) stmt = conn.prepareStatement(
                    "insert into " +
                    "SumDoubleTest(" +
                    "    id, " +
                    "    d, " +
                    "    f, " +
                    "    ud, " +
                    "    uf) " +
                    "VALUES (?, ?, ?, ?, ?)");
            stmt.setString(1, "1");
            stmt.setDouble(2, 0.001);
            stmt.setFloat(3, 0.01f);
            stmt.setDouble(4, 0.001);
            stmt.setFloat(5, 0.01f);
            stmt.execute();

            stmt.setString(1, "2");
            stmt.setDouble(2, 0.002);
            stmt.setFloat(3, 0.02f);
            stmt.setDouble(4, 0.002);
            stmt.setFloat(5, 0.02f);
            stmt.execute();

            stmt.setString(1, "3");
            stmt.setDouble(2, 0.003);
            stmt.setFloat(3, 0.03f);
            stmt.setDouble(4, 0.003);
            stmt.setFloat(5, 0.03f);
            stmt.execute();

            stmt.setString(1, "4");
            stmt.setDouble(2, 0.004);
            stmt.setFloat(3, 0.04f);
            stmt.setDouble(4, 0.004);
            stmt.setFloat(5, 0.04f);
            stmt.execute();

            stmt.setString(1, "5");
            stmt.setDouble(2, 0.005);
            stmt.setFloat(3, 0.05f);
            stmt.setDouble(4, 0.005);
            stmt.setFloat(5, 0.05f);
            stmt.execute();
        } finally {
        }
    }

    protected void initATableValues() throws Exception {
        initATableValues(null);
    }
 
    protected void initATableValues(Date date) throws Exception {
        assertNotNull(conn);
        createTestTable(ATABLE_NAME);
        try {
            // Insert all rows at ts
            PreparedStatement stmt = null;
            if (tgtPH()||tgtTR()) stmt = conn.prepareStatement(
                    "upsert into " +
                    "ATABLE(" +
                    "    ORGANIZATION_ID, " +
                    "    ENTITY_ID, " +
                    "    A_STRING, " +
                    "    B_STRING, " +
                    "    A_INTEGER, " +
                    "    A_DATE, " +
                    "    X_DECIMAL, " +
                    "    X_LONG, " +
                    "    X_INTEGER," +
                    "    Y_INTEGER," +
                    "    A_BYTE," +
                    "    A_SHORT," +
                    "    A_FLOAT," +
                    "    A_DOUBLE," +
                    "    A_UNSIGNED_FLOAT," +
                    "    A_UNSIGNED_DOUBLE)" +
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            else if (tgtSQ()) stmt = conn.prepareStatement(
                    "insert into " +
                    "ATABLE(" +
                    "    ORGANIZATION_ID, " +
                    "    ENTITY_ID, " +
                    "    A_STRING, " +
                    "    B_STRING, " +
                    "    A_INTEGER, " +
                    "    A_DATE, " +
                    "    X_DECIMAL, " +
                    "    X_LONG, " +
                    "    X_INTEGER," +
                    "    Y_INTEGER," +
                    "    A_BYTE," +
                    "    A_SHORT," +
                    "    A_FLOAT," +
                    "    A_DOUBLE," +
                    "    A_UNSIGNED_FLOAT," +
                    "    A_UNSIGNED_DOUBLE)" +
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

            stmt.setString(1, tenantId);
            stmt.setString(2, ROW1);
            stmt.setString(3, A_VALUE);
            stmt.setString(4, B_VALUE);
            stmt.setInt(5, 1);
            stmt.setDate(6, date);
            stmt.setBigDecimal(7, null);
            stmt.setNull(8, Types.BIGINT);
            stmt.setNull(9, Types.INTEGER);
            stmt.setNull(10, Types.INTEGER);
            stmt.setByte(11, (byte)1);
            stmt.setShort(12, (short) 128);
            stmt.setFloat(13, 0.01f);
            stmt.setDouble(14, 0.0001);
            stmt.setFloat(15, 0.01f);
            stmt.setDouble(16, 0.0001);
            stmt.execute();

            stmt.setString(1, tenantId);
            stmt.setString(2, ROW2);
            stmt.setString(3, A_VALUE);
            stmt.setString(4, C_VALUE);
            stmt.setInt(5, 2);
            stmt.setDate(6, date == null ? null : new Date(date.getTime() + MILLIS_IN_DAY * 1));
            stmt.setBigDecimal(7, null);
            stmt.setNull(8, Types.BIGINT);
            stmt.setNull(9, Types.INTEGER);
            stmt.setNull(10, Types.INTEGER);
            stmt.setByte(11, (byte)2);
            stmt.setShort(12, (short) 129);
            stmt.setFloat(13, 0.02f);
            stmt.setDouble(14, 0.0002);
            stmt.setFloat(15, 0.02f);
            stmt.setDouble(16, 0.0002);
            stmt.execute();

            stmt.setString(1, tenantId);
            stmt.setString(2, ROW3);
            stmt.setString(3, A_VALUE);
            stmt.setString(4, E_VALUE);
            stmt.setInt(5, 3);
            stmt.setDate(6, date == null ? null : new Date(date.getTime() + MILLIS_IN_DAY * 2));
            stmt.setBigDecimal(7, null);
            stmt.setNull(8, Types.BIGINT);
            stmt.setNull(9, Types.INTEGER);
            stmt.setNull(10, Types.INTEGER);
            stmt.setByte(11, (byte)3);
            stmt.setShort(12, (short) 130);
            stmt.setFloat(13, 0.03f);
            stmt.setDouble(14, 0.0003);
            stmt.setFloat(15, 0.03f);
            stmt.setDouble(16, 0.0003);
            stmt.execute();

            stmt.setString(1, tenantId);
            stmt.setString(2, ROW4);
            stmt.setString(3, A_VALUE);
            stmt.setString(4, B_VALUE);
            stmt.setInt(5, 4);
            stmt.setDate(6, date == null ? null : date);
            stmt.setBigDecimal(7, null);
            stmt.setNull(8, Types.BIGINT);
            stmt.setNull(9, Types.INTEGER);
            stmt.setNull(10, Types.INTEGER);
            stmt.setByte(11, (byte)4);
            stmt.setShort(12, (short) 131);
            stmt.setFloat(13, 0.04f);
            stmt.setDouble(14, 0.0004);
            stmt.setFloat(15, 0.04f);
            stmt.setDouble(16, 0.0004);
            stmt.execute();

            stmt.setString(1, tenantId);
            stmt.setString(2, ROW5);
            stmt.setString(3, B_VALUE);
            stmt.setString(4, C_VALUE);
            stmt.setInt(5, 5);
            stmt.setDate(6, date == null ? null : new Date(date.getTime() + MILLIS_IN_DAY * 1));
            stmt.setBigDecimal(7, null);
            stmt.setNull(8, Types.BIGINT);
            stmt.setNull(9, Types.INTEGER);
            stmt.setNull(10, Types.INTEGER);
            stmt.setByte(11, (byte)5);
            stmt.setShort(12, (short) 132);
            stmt.setFloat(13, 0.05f);
            stmt.setDouble(14, 0.0005);
            stmt.setFloat(15, 0.05f);
            stmt.setDouble(16, 0.0005);
            stmt.execute();

            stmt.setString(1, tenantId);
            stmt.setString(2, ROW6);
            stmt.setString(3, B_VALUE);
            stmt.setString(4, E_VALUE);
            stmt.setInt(5, 6);
            stmt.setDate(6, date == null ? null : new Date(date.getTime() + MILLIS_IN_DAY * 2));
            stmt.setBigDecimal(7, null);
            stmt.setNull(8, Types.BIGINT);
            stmt.setNull(9, Types.INTEGER);
            stmt.setNull(10, Types.INTEGER);
            stmt.setByte(11, (byte)6);
            stmt.setShort(12, (short) 133);
            stmt.setFloat(13, 0.06f);
            stmt.setDouble(14, 0.0006);
            stmt.setFloat(15, 0.06f);
            stmt.setDouble(16, 0.0006);
            stmt.execute();

            stmt.setString(1, tenantId);
            stmt.setString(2, ROW7);
            stmt.setString(3, B_VALUE);
            stmt.setString(4, B_VALUE);
            stmt.setInt(5, 7);
            stmt.setDate(6, date == null ? null : date);
            stmt.setBigDecimal(7, BigDecimal.valueOf(0.1));
            stmt.setLong(8, 5L);
            stmt.setInt(9, 5);
            stmt.setNull(10, Types.INTEGER);
            stmt.setByte(11, (byte)7);
            stmt.setShort(12, (short) 134);
            stmt.setFloat(13, 0.07f);
            stmt.setDouble(14, 0.0007);
            stmt.setFloat(15, 0.07f);
            stmt.setDouble(16, 0.0007);
            stmt.execute();

            stmt.setString(1, tenantId);
            stmt.setString(2, ROW8);
            stmt.setString(3, B_VALUE);
            stmt.setString(4, C_VALUE);
            stmt.setInt(5, 8);
            stmt.setDate(6, date == null ? null : new Date(date.getTime() + MILLIS_IN_DAY * 1));
            stmt.setBigDecimal(7, BigDecimal.valueOf(3.9));
            long l = Integer.MIN_VALUE - 1L;
            assert(l < Integer.MIN_VALUE);
            stmt.setLong(8, l);
            stmt.setInt(9, 4);
            stmt.setNull(10, Types.INTEGER);
            stmt.setByte(11, (byte)8);
            stmt.setShort(12, (short) 135);
            stmt.setFloat(13, 0.08f);
            stmt.setDouble(14, 0.0008);
            stmt.setFloat(15, 0.08f);
            stmt.setDouble(16, 0.0008);
            stmt.execute();

            stmt.setString(1, tenantId);
            stmt.setString(2, ROW9);
            stmt.setString(3, C_VALUE);
            stmt.setString(4, E_VALUE);
            stmt.setInt(5, 9);
            stmt.setDate(6, date == null ? null : new Date(date.getTime() + MILLIS_IN_DAY * 2));
            stmt.setBigDecimal(7, BigDecimal.valueOf(3.3));
            l = Integer.MAX_VALUE + 1L;
            assert(l > Integer.MAX_VALUE);
            stmt.setLong(8, l);
            stmt.setInt(9, 3);
            stmt.setInt(10, 300);
            stmt.setByte(11, (byte)9);
            stmt.setShort(12, (short) 0);
            stmt.setFloat(13, 0.09f);
            stmt.setDouble(14, 0.0009);
            stmt.setFloat(15, 0.09f);
            stmt.setDouble(16, 0.0009);
            stmt.execute();
        } finally {
        }
    }

    protected String getExplainPlan(ResultSet rs) throws SQLException {
        String output = "";
        try {
            while (rs.next()) {
                output += rs.getString(1);
                output += '\n';
            }
        } finally {
        }
        // System.out.println(output);
        return output;
    }
}

