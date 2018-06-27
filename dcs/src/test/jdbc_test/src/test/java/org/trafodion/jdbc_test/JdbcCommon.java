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

import static org.junit.Assert.*;
import org.junit.*;

import java.sql.DriverManager;
import java.sql.Connection;
import java.sql.Statement;
import java.sql.PreparedStatement;
import java.sql.Date;

import java.util.Map;
import java.util.HashMap;
import java.util.Properties;
import java.util.Arrays;
import java.util.ArrayList;

import java.io.File;
import java.io.FileInputStream;
import java.lang.Thread.*;


public class JdbcCommon {

    protected static final String BATCH_TEST_TABLE = "BATCH_TEST_TABLE";
    protected static final String BATCH_TEST_TABLE_FK = "BATCH_TEST_TABLE_FK";
    protected static ArrayList<String> objDropList = null;

    private final boolean print_testinfo = false;
    
    private static Map<String,String> _tableDDLMap = null;

    private static String _catalog = null;
    private static String _schema = null;
    private static String _url = null;

    protected static Connection _conn = null;
    private static Connection commConn = null;
    
    protected static String getUrl() { return _url; }

    private static boolean _driverRegistered = false;

    static {
        if(_tableDDLMap == null)
            _tableDDLMap = new HashMap();
        
        _tableDDLMap.put(BATCH_TEST_TABLE, "CREATE TABLE BATCH_TEST_TABLE(DEPT_ID INT NOT NULL PRIMARY KEY, DEPT_NAME VARCHAR(128))");
        
        _tableDDLMap.put(BATCH_TEST_TABLE_FK, "CREATE TABLE BATCH_TEST_TABLE_FK(EID INT NOT NULL PRIMARY KEY, DEPT_ID INT, E_NAME VARCHAR(128)," +
                " FOREIGN KEY(DEPT_ID) REFERENCES BATCH_TEST_TABLE(DEPT_ID))");
    }

    protected void printTestDescription() {
        if (print_testinfo) {
            System.out.println(Thread.currentThread().getStackTrace()[2]);
        }
    }

    protected static Connection getConnection() throws Exception {
        Connection conn = null;
        try {
            String propFile = System.getProperty("trafjdbc.properties");
            assertNotNull(propFile);
            FileInputStream fs = new FileInputStream(new File(propFile));
            Properties props = new Properties();
            props.load(fs);

            _url = props.getProperty("url");
            _catalog = props.getProperty("catalog");
            _schema = props.getProperty("schema");
           
            // Reigser our JDBC driver if this is the first call.           
            if (! _driverRegistered) {
                if (_url.contains("t4jdbc"))
                    Class.forName("org.trafodion.jdbc.t4.T4Driver");   // T4 driver
                else
                    Class.forName("org.apache.trafodion.jdbc.t2.T2Driver"); // T2 driver
                _driverRegistered = true;
            }

            conn = DriverManager.getConnection(_url, props);
        } catch (Exception e) {
            conn = null;
            fail(e.getMessage());
        }

        assertNotNull(conn);

        return conn;
    }

    protected void createTestTable(String tableName) {
        assertNotNull(_conn);
        String ddl = null;
        ddl = _tableDDLMap.get(tableName);
        assertNotNull(ddl);
        StringBuilder buf = new StringBuilder(ddl);
        ddl = buf.toString();

        try (
            Statement stmt = _conn.createStatement();
        )
        {
            stmt.execute(ddl);
        } catch (Exception e) { 
            System.out.println(e.getMessage());
            fail("Failed to create table");
        }
    }

    protected static void doBaseTestSuiteSetup() throws Exception {
        createSchemaIfNotExist();

        String name = Thread.currentThread().getStackTrace()[2].toString();
        name = name.substring(0, name.indexOf(".doTestSuiteSetup")); 
        System.out.println(name);
    }

    @AfterClass
    public static void doBaseTestSuiteCleanup() throws Exception {
        dropSchemaIfExist();
    }

    @Before
    public void doBaseTestSetup() throws Exception {
        _conn = getConnection();
    }

    @After
    public void doBaseTestCleanup() throws Exception {
        _conn.close();
        dropTestObjects();
    }

    // create schema if it does exist.
    private static void createSchemaIfNotExist() throws Exception {
        if (commConn == null)
            commConn = getConnection();

        try (
            Statement stmt = commConn.createStatement();
        )
        {
            stmt.execute("create schema " + _catalog + "." + _schema);
        } catch (Exception e) {
            // Do nothing, the schema may already exist.
        }
    }

    private static void dropSchemaIfExist() throws Exception {
        if (commConn == null)
            commConn = getConnection();

        try (
            Statement stmt = commConn.createStatement();
        )
        {
            stmt.execute("drop schema " + _catalog + "." + _schema + " cascade");
        } catch (Exception e) {
            // Do nothing, the schema may not exist.  
        }
    }

    protected static void dropTestObjects() throws Exception {

        // Use our own conn.  Who knows what the tests have been doing with
        // auto commit of the conn that it has been using.
        if (commConn == null)
            commConn = getConnection();

        if (objDropList == null)
            return;

        for (String objname : objDropList) {
            for (int i = 0; i < 3; i++) {
                try (
                    Statement stmt = commConn.createStatement();
                ){
                    stmt.executeUpdate("drop " + objname + " cascade");
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
}
