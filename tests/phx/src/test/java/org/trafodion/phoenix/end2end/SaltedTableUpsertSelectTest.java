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
import java.util.*;


public class SaltedTableUpsertSelectTest extends BaseTest {

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table source", "table target"));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    @Test
    public void testUpsertIntoSaltedTableFromNormalTable() throws Exception {
        printTestDescription();

        try {
            String ddl = null;
            if (tgtPH()) ddl = "CREATE TABLE IF NOT EXISTS source" + 
                    " (pk VARCHAR NOT NULL PRIMARY KEY, col INTEGER)";
            else if (tgtTR()) ddl = "CREATE TABLE IF NOT EXISTS source" +
                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col INTEGER)";
            else if (tgtSQ()) ddl = "CREATE TABLE source" +
                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col INTEGER)";
            conn.createStatement().execute(ddl);
            ddl = null;
            if (tgtPH()) ddl = "CREATE TABLE IF NOT EXISTS target" + 
                    " (pk VARCHAR NOT NULL PRIMARY KEY, col INTEGER) SALT_BUCKETS=4";
            else if (tgtTR()) ddl = "CREATE TABLE IF NOT EXISTS target" +
                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col INTEGER) ";
            else if (tgtSQ()) ddl = "CREATE TABLE target" +
                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col INTEGER) ";
            conn.createStatement().execute(ddl);
        
            conn.setAutoCommit(false);
    
            String query = null;
            if (tgtPH()||tgtTR()) query = "UPSERT INTO source(pk, col) VALUES(?,?)";
            else if (tgtSQ()) query = "INSERT INTO source(pk, col) VALUES(?,?)";
            PreparedStatement stmt = conn.prepareStatement(query);
            stmt.setString(1, "1");
            stmt.setInt(2, 1);
            stmt.execute();
            conn.commit();
            
            if (tgtPH()||tgtTR()) query = "UPSERT INTO target(pk, col) SELECT pk, col from source";
            else if (tgtSQ()) query = "INSERT INTO target(pk, col) SELECT pk, col from source";
            stmt = conn.prepareStatement(query);
            stmt.execute();
            conn.commit();
            
            query = "SELECT * FROM target";
            stmt = conn.prepareStatement(query);
            ResultSet rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals("1", rs.getString(1));
            assertEquals(1, rs.getInt(2));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testUpsertIntoNormalTableFromSaltedTable() throws Exception {
        printTestDescription();

        try {
            String ddl = null;
            if (tgtPH()) ddl = "CREATE TABLE IF NOT EXISTS source" + 
                    " (pk VARCHAR NOT NULL PRIMARY KEY, col INTEGER) SALT_BUCKETS=4";
            else if (tgtTR()) ddl = "CREATE TABLE IF NOT EXISTS source" +
                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col INTEGER)";
            else if (tgtSQ()) ddl = "CREATE TABLE source" +
                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col INTEGER)";
            conn.createStatement().execute(ddl);
            if (tgtPH()) ddl = "CREATE TABLE IF NOT EXISTS target" + 
                    " (pk VARCHAR NOT NULL PRIMARY KEY, col INTEGER)";
            else if (tgtTR()) ddl = "CREATE TABLE IF NOT EXISTS target" +
                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col INTEGER) ";
            else if (tgtSQ()) ddl = "CREATE TABLE target" +
                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col INTEGER) ";
            conn.createStatement().execute(ddl);

            conn.setAutoCommit(false);

            String query = null;
            if (tgtPH()||tgtTR()) query = "UPSERT INTO source(pk, col) VALUES(?,?)";
            else if (tgtSQ()) query = "INSERT INTO source(pk, col) VALUES(?,?)";
            PreparedStatement stmt = conn.prepareStatement(query);
            stmt.setString(1, "1");
            stmt.setInt(2, 1);
            stmt.execute();
            conn.commit();
            
            if (tgtPH()||tgtTR()) query = "UPSERT INTO target(pk, col) SELECT pk, col from source";
            else if (tgtSQ()) query = "INSERT INTO target(pk, col) SELECT pk, col from source";
            stmt = conn.prepareStatement(query);
            stmt.execute();
            conn.commit();
            
            query = "SELECT * FROM target";
            stmt = conn.prepareStatement(query);
            ResultSet rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals("1", rs.getString(1));
            assertEquals(1, rs.getInt(2));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testUpsertSaltedTableIntoSaltedTable() throws Exception {
        printTestDescription();

        try {
            String ddl = null;
            if (tgtPH()) ddl = "CREATE TABLE IF NOT EXISTS source" + 
                    " (pk VARCHAR NOT NULL PRIMARY KEY, col INTEGER) SALT_BUCKETS=4";
            else if (tgtTR())  ddl = "CREATE TABLE IF NOT EXISTS source" +
                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col INTEGER)";
            else if (tgtSQ())  ddl = "CREATE TABLE source" +
                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col INTEGER)";
            conn.createStatement().execute(ddl);
            if (tgtPH()) ddl = "CREATE TABLE IF NOT EXISTS target" + 
                    " (pk VARCHAR NOT NULL PRIMARY KEY, col INTEGER) SALT_BUCKETS=4";
            else if (tgtTR()) ddl = "CREATE TABLE IF NOT EXISTS target" +
                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col INTEGER)";
            else if (tgtSQ()) ddl = "CREATE TABLE target" +
                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col INTEGER)";
            conn.createStatement().execute(ddl);
        
            conn.setAutoCommit(false);
    
            String query = null;
            if (tgtPH()||tgtTR()) query = "UPSERT INTO source(pk, col) VALUES(?,?)";
            else if (tgtSQ()) query = "INSERT INTO source(pk, col) VALUES(?,?)";
            PreparedStatement stmt = conn.prepareStatement(query);
            stmt.setString(1, "1");
            stmt.setInt(2, 1);
            stmt.execute();
            conn.commit();
            
            if (tgtPH()||tgtTR()) query = "UPSERT INTO target(pk, col) SELECT pk, col from source";
            else if (tgtSQ()) query = "INSERT INTO target(pk, col) SELECT pk, col from source";
            stmt = conn.prepareStatement(query);
            stmt.execute();
            conn.commit();
            
            query = "SELECT * FROM target";
            stmt = conn.prepareStatement(query);
            ResultSet rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals("1", rs.getString(1));
            assertEquals(1, rs.getInt(2));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testUpsertSelectOnSameSaltedTable() throws Exception {
        printTestDescription();

        try {
            String ddl = null;
            if (tgtPH()) ddl = "CREATE TABLE IF NOT EXISTS source" + 
                    " (pk VARCHAR NOT NULL PRIMARY KEY, col1 INTEGER, col2 INTEGER) SALT_BUCKETS=4";
            else if (tgtTR()) ddl = "CREATE TABLE IF NOT EXISTS source" +
                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col1 INTEGER, col2 INTEGER)";
            else if (tgtSQ()) ddl = "CREATE TABLE source" +
                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col1 INTEGER, col2 INTEGER)";
            conn.createStatement().execute(ddl);
        
            conn.setAutoCommit(false);
    
            String query = null;
            if (tgtPH()||tgtTR()) query = "UPSERT INTO source(pk, col1) VALUES(?,?)";
            else if (tgtSQ()) query = "INSERT INTO source(pk, col1) VALUES(?,?)";
            PreparedStatement stmt = conn.prepareStatement(query);
            stmt.setString(1, "1");
            stmt.setInt(2, 1);
            stmt.execute();
            conn.commit();
            
            if (tgtPH()||tgtTR()) query = "UPSERT INTO source(pk, col2) SELECT pk, col1 from source";
            else if (tgtSQ()) query = "UPDATE source set col2 = col1 where pk in(SELECT pk from source)";
            stmt = conn.prepareStatement(query);
            stmt.execute();
            conn.commit();
            
            query = "SELECT col2 FROM source";
            stmt = conn.prepareStatement(query);
            ResultSet rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals(1, rs.getInt(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testUpsertSelectOnSameSaltedTableWithEmptyPKColumn() throws Exception {
        printTestDescription();

        try {
            String ddl = null;
            if (tgtPH()) ddl = "CREATE TABLE IF NOT EXISTS source" + 
                    " (pk1 varchar NULL, pk2 varchar NULL, pk3 integer NOT NULL, col1 INTEGER" + 
                    " CONSTRAINT pk PRIMARY KEY (pk1, pk2, pk3)) SALT_BUCKETS=4";
            else if (tgtTR()) ddl = "CREATE TABLE IF NOT EXISTS source" +
                    " (pk1 varchar(128) DEFAULT NULL, pk2 varchar(128) DEFAULT NULL, pk3 integer NOT NULL, col1 INTEGER" +
                    " , CONSTRAINT pk_SaltedTableUpsertSelectSource PRIMARY KEY (pk3))";
            else if (tgtSQ()) ddl = "CREATE TABLE source" +
                    " (pk1 varchar(128) DEFAULT NULL, pk2 varchar(128) DEFAULT NULL, pk3 integer NOT NULL, col1 INTEGER" +
                    " , CONSTRAINT pk_SaltedTableUpsertSelectSource PRIMARY KEY (pk3))";

            conn.createStatement().execute(ddl);
        
            conn.setAutoCommit(false);
    
            String query = null;
            if (tgtPH()||tgtTR()) query = "UPSERT INTO source(pk1, pk2, pk3, col1) VALUES(?,?,?,?)";
            else if (tgtSQ()) query = "INSERT INTO source(pk1, pk2, pk3, col1) VALUES(?,?,?,?)";
            PreparedStatement stmt = conn.prepareStatement(query);
            stmt.setString(1, "1");
            stmt.setString(2, "2");
            stmt.setInt(3, 1);
            stmt.setInt(4, 1);
            stmt.execute();
            conn.commit();
            
            conn.setAutoCommit(true);
            query = null;
            if (tgtPH()||tgtTR()) query = "UPSERT INTO source(pk3, col1, pk1) SELECT pk3+1, col1+1, pk2 from source";
            else if (tgtSQ()) query = "INSERT INTO source(pk3, col1, pk1) SELECT pk3+1, col1+1, pk2 from source";
            stmt = conn.prepareStatement(query);
            stmt.execute();
            
            if (tgtPH()) query = "SELECT col1 FROM source";
            else if (tgtSQ()||tgtTR()) query = "SELECT col1 FROM source order by 1";
            stmt = conn.prepareStatement(query);
            ResultSet rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals(1, rs.getInt(1));
            assertTrue(rs.next());
            assertEquals(2, rs.getInt(1));
            assertFalse(rs.next());
        } finally {
        }
    }
}
