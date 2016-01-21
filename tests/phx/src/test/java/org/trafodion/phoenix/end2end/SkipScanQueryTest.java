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
import java.util.*;


public class SkipScanQueryTest extends BaseTest {
   
    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table inTest", "table inVarTest"));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */
 
    private void initIntInTable(List<Integer> data) throws SQLException {
        String ddl = null;
        if (tgtPH()||tgtTR()) ddl = "CREATE TABLE IF NOT EXISTS inTest (" + 
                     "  i INTEGER NOT NULL PRIMARY KEY)";
        else if (tgtSQ()) ddl = "CREATE TABLE inTest (" +
                     "  i INTEGER NOT NULL PRIMARY KEY)";
        conn.createStatement().executeUpdate(ddl);
        
        // Test upsert correct values 
        String query = null;
        if (tgtPH()||tgtTR()) query = "UPSERT INTO inTest VALUES(?)";
        else if (tgtSQ()) query = "INSERT INTO inTest VALUES(?)";
        PreparedStatement stmt = conn.prepareStatement(query);
        for (Integer i : data) {
            stmt.setInt(1, i);
            stmt.execute();
        }
    }
    
    private void initVarCharCrossProductInTable(List<String> c1, List<String> c2) throws SQLException {
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE IF NOT EXISTS inVarTest (" + 
                     "  s1 VARCHAR, s2 VARCHAR CONSTRAINT pk PRIMARY KEY (s1,s2))";
        else if (tgtTR()) ddl = "CREATE TABLE IF NOT EXISTS inVarTest (" +
                     "  s1 VARCHAR(128) not null not droppable, s2 VARCHAR(128) not null not droppable, CONSTRAINT pk PRIMARY KEY (s1,s2))";
        else if (tgtSQ()) ddl = "CREATE TABLE inVarTest (" +
                     "  s1 VARCHAR(128) not null not droppable, s2 VARCHAR(128) not null not droppable, CONSTRAINT pk PRIMARY KEY (s1,s2))";
        conn.createStatement().executeUpdate(ddl);
        
        // Test upsert correct values 
        String query = null;
        if (tgtPH()||tgtTR()) query = "UPSERT INTO inVarTest VALUES(?,?)";
        else if (tgtSQ()) query = "INSERT INTO inVarTest VALUES(?,?)";
        PreparedStatement stmt = conn.prepareStatement(query);
        for (String s1 : c1) {
            for (String s2 : c2) {
                stmt.setString(1, s1);
                stmt.setString(2, s2);
                stmt.execute();
            }
        }
    }
    
    private void initVarCharParallelListInTable(List<String> c1, List<String> c2) throws SQLException {
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE IF NOT EXISTS inVarTest (" + 
                     "  s1 VARCHAR, s2 VARCHAR CONSTRAINT pk PRIMARY KEY (s1,s2))";
        else if (tgtTR()) ddl = "CREATE TABLE IF NOT EXISTS inVarTest (" +
                     "  s1 VARCHAR(128) not null not droppable, s2 VARCHAR(128) not null not droppable, CONSTRAINT pk PRIMARY KEY (s1,s2))";
        else if (tgtSQ()) ddl = "CREATE TABLE inVarTest (" +
                     "  s1 VARCHAR(128) not null not droppable, s2 VARCHAR(128) not null not droppable, CONSTRAINT pk PRIMARY KEY (s1,s2))";

        conn.createStatement().executeUpdate(ddl);
        
        // Test upsert correct values 
        String query = null;
        if (tgtPH()||tgtTR()) query = "UPSERT INTO inVarTest VALUES(?,?)";
        else if (tgtSQ()) query = "INSERT INTO inVarTest VALUES(?,?)";
        PreparedStatement stmt = conn.prepareStatement(query);
        for (int i = 0; i < c1.size(); i++) {
            stmt.setString(1, c1.get(i));
            stmt.setString(2, i < c2.size() ? c2.get(i) : null);
            stmt.execute();
        }
    }
    
    @Test
    public void testInQuery() throws Exception {
        printTestDescription();

        initIntInTable(Arrays.asList(2,7,10));
        conn.setAutoCommit(false);
        try {
            String query = null;
            if (tgtPH()) query = "SELECT i FROM inTest WHERE i IN (1,2,4,5,7,8,10)";
            else if (tgtSQ()||tgtTR()) query = "SELECT i FROM inTest WHERE i IN (1,2,4,5,7,8,10) order by 1";
            ResultSet rs = conn.createStatement().executeQuery(query);
            assertTrue(rs.next());
            assertEquals(2, rs.getInt(1));
            assertTrue(rs.next());
            assertEquals(7, rs.getInt(1));
            assertTrue(rs.next());
            assertEquals(10, rs.getInt(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testVarCharParallelListInQuery() throws Exception {
        printTestDescription();

        initVarCharParallelListInTable(Arrays.asList("d","da","db"),Arrays.asList("m","mc","tt"));
        conn.setAutoCommit(false);
        try {
            String query;
            query = "SELECT s1,s2 FROM inVarTest WHERE s1 IN ('a','b','da','db') AND s2 IN ('c','ma','m','mc','ttt','z')";
            ResultSet rs = conn.createStatement().executeQuery(query);
            assertTrue(rs.next());
            assertEquals("da", rs.getString(1));
            assertEquals("mc", rs.getString(2));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testVarCharXInQuery() throws Exception {
        printTestDescription();

        initVarCharCrossProductInTable(Arrays.asList("d","da","db"),Arrays.asList("m","mc","tt"));
        conn.setAutoCommit(false);
        try {
            String query = null;
            if (tgtPH()) query = "SELECT s1,s2 FROM inVarTest WHERE s1 IN ('a','b','da','db') AND s2 IN ('c','ma','m','mc','ttt','z')";
            else if (tgtSQ()||tgtTR()) query = "SELECT s1,s2 FROM inVarTest WHERE s1 IN ('a','b','da','db') AND s2 IN ('c','ma','m','mc','ttt','z') order by 1, 2";

            ResultSet rs = conn.createStatement().executeQuery(query);
            assertTrue(rs.next());
            assertEquals("da", rs.getString(1));
            assertEquals("m", rs.getString(2));
            assertTrue(rs.next());
            assertEquals("da", rs.getString(1));
            assertEquals("mc", rs.getString(2));
            assertTrue(rs.next());
            assertEquals("db", rs.getString(1));
            assertEquals("m", rs.getString(2));
            assertTrue(rs.next());
            assertEquals("db", rs.getString(1));
            assertEquals("mc", rs.getString(2));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testVarCharXIntInQuery() throws Exception {
        printTestDescription();

        initVarCharCrossProductInTable(Arrays.asList("d","da","db"),Arrays.asList("m","mc","tt"));
        conn.setAutoCommit(false);
        try {
            String query;
            query = "SELECT s1,s2 FROM inVarTest " +
                    "WHERE s1 IN ('a','b','da','db') AND s2 IN ('c','ma','m','mc','ttt','z') " +
                    "AND s1 > 'd' AND s1 < 'db' AND s2 > 'm'";
            ResultSet rs = conn.createStatement().executeQuery(query);
            assertTrue(rs.next());
            assertEquals("da", rs.getString(1));
            assertEquals("mc", rs.getString(2));
            assertFalse(rs.next());
        } finally {
        }
    }
    
}
