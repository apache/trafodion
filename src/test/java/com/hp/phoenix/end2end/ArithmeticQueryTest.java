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
import java.sql.*;
import java.util.*;
import java.math.*;

public class ArithmeticQueryTest extends BaseTest {

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table testDecimalArithmetic", "table testDecimalArithmatic", "table source", "table target"));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    @Test
    public void testDecimalUpsertValue() throws Exception {
        printTestDescription();

        try {
            String ddl = null;
            if (tgtPH()) ddl = "CREATE TABLE IF NOT EXISTS testDecimalArithmetic" + 
                               "  (pk VARCHAR NOT NULL PRIMARY KEY, " +
                               "col1 DECIMAL(31,0), col2 DECIMAL(5), col3 DECIMAL(5,2), col4 DECIMAL)";
            else if (tgtTR()) ddl = "CREATE TABLE IF NOT EXISTS testDecimalArithmetic" +
                                    "  (pk VARCHAR(128) NOT NULL PRIMARY KEY, " +
                                    "col1 DECIMAL(18,0), col2 DECIMAL(5), col3 DECIMAL(5,2), col4 DECIMAL)";
            else if (tgtSQ()) ddl = "CREATE TABLE testDecimalArithmetic" +
                                    "  (pk VARCHAR(128) NOT NULL PRIMARY KEY, " +
                                    "col1 DECIMAL(18,0), col2 DECIMAL(5), col3 DECIMAL(5,2), col4 DECIMAL)";

            conn.createStatement().execute(ddl);

            conn.setAutoCommit(false);
 
            // Test upsert correct values 
            String query = null;
            if (tgtPH()||tgtTR()) query = "UPSERT INTO testDecimalArithmetic(pk, col1, col2, col3, col4) VALUES(?,?,?,?,?)";
            else if (tgtSQ()) query = "INSERT INTO testDecimalArithmetic(pk, col1, col2, col3, col4) VALUES(?,?,?,?,?)";

            PreparedStatement stmt = conn.prepareStatement(query);
            stmt.setString(1, "valueOne");
            stmt.setBigDecimal(2, new BigDecimal("123456789123456789"));
            stmt.setBigDecimal(3, new BigDecimal("12345"));
            stmt.setBigDecimal(4, new BigDecimal("12.34"));
            stmt.setBigDecimal(5, new BigDecimal("12345.6789"));
            stmt.execute();
            conn.commit();
           
            query = "SELECT col1, col2, col3, col4 FROM testDecimalArithmetic WHERE pk = 'valueOne'";
            stmt = conn.prepareStatement(query);
            ResultSet rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals(new BigDecimal("123456789123456789"), rs.getBigDecimal(1));
            assertEquals(new BigDecimal("12345"), rs.getBigDecimal(2));
            assertEquals(new BigDecimal("12.34"), rs.getBigDecimal(3));
            if (tgtPH()) assertEquals(new BigDecimal("12345.6789"), rs.getBigDecimal(4));
            else if (tgtSQ()||tgtTR()) assertTrue(rs.getBigDecimal(4).equals(new BigDecimal("12345")) || rs.getBigDecimal(4).equals(new BigDecimal("12346")));
            assertFalse(rs.next());
           
            if (tgtPH()||tgtTR()) query = "UPSERT INTO testDecimalArithmetic(pk, col1, col2, col3) VALUES(?,?,?,?)";
            else if (tgtSQ()) query = "INSERT INTO testDecimalArithmetic(pk, col1, col2, col3) VALUES(?,?,?,?)";
 
            stmt = conn.prepareStatement(query);
            stmt.setString(1, "valueTwo");
            if (tgtPH()) stmt.setBigDecimal(2, new BigDecimal("1234567890123456789012345678901.12345"));
            else if (tgtSQ()||tgtTR()) stmt.setBigDecimal(2, new BigDecimal("123456789012345678.12345"));
            stmt.setBigDecimal(3, new BigDecimal("12345.6789"));
            stmt.setBigDecimal(4, new BigDecimal("123.45678"));
            stmt.execute();
            conn.commit();
            
            query = "SELECT col1, col2, col3 FROM testDecimalArithmetic WHERE pk = 'valueTwo'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            if (tgtPH()) assertEquals(new BigDecimal("1234567890123456789012345678901"), rs.getBigDecimal(1));
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("123456789012345678"), rs.getBigDecimal(1));
            assertTrue(rs.getBigDecimal(2).equals(new BigDecimal("12345")) || rs.getBigDecimal(2).equals(new BigDecimal("12346")));
            assertTrue(rs.getBigDecimal(3).equals(new BigDecimal("123.45")) || rs.getBigDecimal(3).equals(new BigDecimal("123.46")));
            assertFalse(rs.next());
            
            // Test upsert incorrect values and confirm exceptions would be thrown.
            try {
                if (tgtPH()||tgtTR()) query = "UPSERT INTO testDecimalArithmetic(pk, col1, col2, col3) VALUES(?,?,?,?)";
                else if (tgtSQ()) query = "INSERT INTO testDecimalArithmetic(pk, col1, col2, col3) VALUES(?,?,?,?)";
                stmt = conn.prepareStatement(query);
                stmt.setString(1, "badValues");
                // one more than max_precision
                stmt.setBigDecimal(2, new BigDecimal("12345678901234567890123456789012"));
                stmt.setBigDecimal(3, new BigDecimal("12345")); 
                stmt.setBigDecimal(4, new BigDecimal("123.45"));
                stmt.execute();
                conn.commit();
                fail("Should have caught bad values.");
            } catch (Exception e) {
                if (tgtPH()) assertTrue(e.getMessage(), e.getMessage().contains("ERROR 206 (22003): The value is outside the range for the data type. value=12345678901234567890123456789012 columnName=COL1"));
                // This error message is different between T2 and T4.
                // Only make sure that we get an exception now.
                // else if (tgtSQ()||tgtTR()) assertTrue(e.getMessage(), e.getMessage().contains("*** ERROR[29188]"));
            }
            try {
                if (tgtPH()||tgtTR()) query = "UPSERT INTO testDecimalArithmetic(pk, col1, col2, col3) VALUES(?,?,?,?)";
                else if (tgtSQ()) query = "INSERT INTO testDecimalArithmetic(pk, col1, col2, col3) VALUES(?,?,?,?)";
                stmt = conn.prepareStatement(query);
                stmt.setString(1, "badValues");
                stmt.setBigDecimal(2, new BigDecimal("123456"));
                // Exceeds specified precision by 1
                stmt.setBigDecimal(3, new BigDecimal("123456"));
                stmt.setBigDecimal(4, new BigDecimal("123.45"));
                stmt.execute();
                conn.commit();
                fail("Should have caught bad values.");
            } catch (Exception e) {
                if (tgtPH()) assertTrue(e.getMessage(), e.getMessage().contains("ERROR 206 (22003): The value is outside the range for the data type. value=123456 columnName=COL2"));
                // This error message is different between T2 and T4.
                // Only make sure that we get an exception now.
                // else if (tgtSQ()||tgtTR()) assertTrue(e.getMessage(), e.getMessage().contains("*** ERROR[29188]"));
            }
        } finally {
        }
    }

    @Test
    public void testDecimalUpsertSelect() throws Exception {
        printTestDescription();

        try {
            String ddl = null;
            if (tgtPH()) ddl = "CREATE TABLE IF NOT EXISTS source" + 
                               " (pk VARCHAR NOT NULL PRIMARY KEY, col1 DECIMAL(5,2), col2 DECIMAL(5,1), col3 DECIMAL(5,2), col4 DECIMAL(4,4))";
            else if (tgtTR()) ddl = "CREATE TABLE IF NOT EXISTS source" +
                                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col1 DECIMAL(5,2), col2 DECIMAL(5,1), col3 DECIMAL(5,2), col4 DECIMAL(4,4))";
            else if (tgtSQ()) ddl = "CREATE TABLE source" +
                                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col1 DECIMAL(5,2), col2 DECIMAL(5,1), col3 DECIMAL(5,2), col4 DECIMAL(4,4))";

            conn.createStatement().execute(ddl);

            if (tgtPH()) ddl = "CREATE TABLE IF NOT EXISTS target" + 
                               " (pk VARCHAR NOT NULL PRIMARY KEY, col1 DECIMAL(5,1), col2 DECIMAL(5,2), col3 DECIMAL(4,4))";
            else if (tgtTR()) ddl = "CREATE TABLE IF NOT EXISTS target" +
                                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col1 DECIMAL(5,1), col2 DECIMAL(5,2), col3 DECIMAL(4,4))";
            else if (tgtSQ()) ddl = "CREATE TABLE target" +
                                    " (pk VARCHAR(128) NOT NULL PRIMARY KEY, col1 DECIMAL(5,1), col2 DECIMAL(5,2), col3 DECIMAL(4,4))";
            conn.createStatement().execute(ddl);
        
            conn.setAutoCommit(false);
    
            String query = null;
            if (tgtPH()||tgtTR()) query = "UPSERT INTO source(pk, col1) VALUES(?,?)";
            else if (tgtSQ()) query = "INSERT INTO source(pk, col1) VALUES(?,?)";
            PreparedStatement stmt = conn.prepareStatement(query);
            stmt.setString(1, "1");
            stmt.setBigDecimal(2, new BigDecimal("100.12"));
            stmt.execute();
            conn.commit();
            stmt.setString(1, "2");
            stmt.setBigDecimal(2, new BigDecimal("100.34"));
            stmt.execute();
            conn.commit();
            
            // Evaluated on client side.
            // source and target in different tables, values scheme compatible.
            if (tgtPH()||tgtTR()) query = "UPSERT INTO target(pk, col2) SELECT pk, col1 from source";
            else if (tgtSQ()) query = "INSERT INTO target(pk, col2) SELECT pk, col1 from source";
            stmt = conn.prepareStatement(query);
            stmt.execute();
            conn.commit();
            if (tgtPH()) query = "SELECT col2 FROM target";
            else if (tgtSQ()||tgtTR()) query = "SELECT col2 FROM target order by 1";
            stmt = conn.prepareStatement(query);
            ResultSet rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals(new BigDecimal("100.12"), rs.getBigDecimal(1));
            assertTrue(rs.next());
            assertEquals(new BigDecimal("100.34"), rs.getBigDecimal(1));
            assertFalse(rs.next());
            // source and target in different tables, values requires scale chopping.
            if (tgtPH()||tgtTR()) query = "UPSERT INTO target(pk, col1) SELECT pk, col1 from source";
            else if (tgtSQ()) query = "UPDATE target SET col1=(select col1 from source where source.pk=target.pk)";
            stmt = conn.prepareStatement(query);
            stmt.execute();
            conn.commit();
            if (tgtPH()) query = "SELECT col1 FROM target";
            else if (tgtSQ()||tgtTR()) query = "SELECT col1 FROM target order by 1";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            assertEquals(new BigDecimal("100.1"), rs.getBigDecimal(1));
            assertTrue(rs.next());
            assertEquals(new BigDecimal("100.3"), rs.getBigDecimal(1));
            assertFalse(rs.next());
            // source and target in different tables, values scheme incompatible.
            try {
                if (tgtPH()||tgtTR()) query = "UPSERT INTO target(pk, col3) SELECT pk, col1 from source";
                else if (tgtSQ()) query = "INSERT INTO target(pk, col3) SELECT pk, col1 from source";
                stmt = conn.prepareStatement(query);
                stmt.execute();
                conn.commit();
                fail("Should have caught bad upsert.");
            } catch (Exception e) {
                if (tgtPH()) assertTrue(e.getMessage(), e.getMessage().contains("ERROR 206 (22003): The value is outside the range for the data type. columnName=COL3"));
                else if (tgtSQ()||tgtTR()) assertTrue(e.getMessage(), e.getMessage().contains("*** ERROR[8411]"));

            }
           
            if (tgtPH()) { 
                // Evaluate on server side.
                conn.setAutoCommit(true);
                // source and target in same table, values scheme compatible.
                if (tgtPH()||tgtTR()) query = "UPSERT INTO source(pk, col3) SELECT pk, col1 from source";
                else if (tgtSQ()) query = "UPDATE target SET col3=(select col1 from source where source.pk=target.pk)";
                stmt = conn.prepareStatement(query);
                stmt.execute();
                if (tgtPH()) query = "SELECT col3 FROM source";
                else if (tgtSQ()||tgtTR()) query = "SELECT col3 FROM source order by 1";
                stmt = conn.prepareStatement(query);
                rs = stmt.executeQuery();
                assertTrue(rs.next());
                assertEquals(new BigDecimal("100.12"), rs.getBigDecimal(1));
                assertTrue(rs.next());
                assertEquals(new BigDecimal("100.34"), rs.getBigDecimal(1));
                assertFalse(rs.next());
                // source and target in same table, values requires scale chopping.
                if (tgtPH()||tgtTR()) query = "UPSERT INTO source(pk, col2) SELECT pk, col1 from source";
                else if (tgtSQ()) query = "UPDATE target SET col2=(select col1 from source where source.pk=target.pk)";
                stmt = conn.prepareStatement(query);
                stmt.execute();
                if (tgtPH()) query = "SELECT col2 FROM source";
                else if (tgtSQ()||tgtTR()) query = "SELECT col2 FROM source order by 1"; 
                stmt = conn.prepareStatement(query);
                rs = stmt.executeQuery();
                assertTrue(rs.next());
                assertEquals(new BigDecimal("100.1"), rs.getBigDecimal(1));
                assertTrue(rs.next());
                assertEquals(new BigDecimal("100.3"), rs.getBigDecimal(1));
                assertFalse(rs.next());
                // source and target in same table, values scheme incompatible.
                if (tgtPH()||tgtTR()) query = "UPSERT INTO source(pk, col4) SELECT pk, col1 from source";
                else if (tgtSQ()) query = "UPDATE target SET col4=(select col1 from source where source.pk=target.pk)";
                stmt = conn.prepareStatement(query);
                stmt.execute();
                query = "SELECT col4 FROM source";
                stmt = conn.prepareStatement(query);
                rs = stmt.executeQuery();
                assertTrue(rs.next());
                assertNull(rs.getBigDecimal(1));
                assertTrue(rs.next());
                assertNull(rs.getBigDecimal(1));
                assertFalse(rs.next());
            } 
        } finally {
        }
    }

    @Test
    public void testDecimalAveraging() throws Exception {
        printTestDescription();

        try {
            String ddl = null;
            if (tgtPH()) ddl = "CREATE TABLE IF NOT EXISTS testDecimalArithmatic" + 
                                    "  (pk VARCHAR NOT NULL PRIMARY KEY, col1 DECIMAL(31, 11), col2 DECIMAL(31,1), col3 DECIMAL(38,1))";
            else if (tgtTR()) ddl = "CREATE TABLE IF NOT EXISTS testDecimalArithmatic" +
                                    "  (pk VARCHAR(128) NOT NULL PRIMARY KEY, col1 DECIMAL(18, 11), col2 DECIMAL(18,1), col3 DECIMAL(18,1))";
            else if (tgtSQ()) ddl = "CREATE TABLE testDecimalArithmatic" +
                                    "  (pk VARCHAR(128) NOT NULL PRIMARY KEY, col1 DECIMAL(18, 11), col2 DECIMAL(18,1), col3 DECIMAL(18,1))";         
            conn.createStatement().execute(ddl);
        
            conn.setAutoCommit(false);
    
            String query = null;
            if (tgtPH()||tgtTR()) query = "UPSERT INTO testDecimalArithmatic(pk, col1, col2, col3) VALUES(?,?,?,?)";
            else if (tgtSQ()) query = "INSERT INTO testDecimalArithmatic(pk, col1, col2, col3) VALUES(?,?,?,?)";
            PreparedStatement stmt = conn.prepareStatement(query);
            stmt.setString(1, "1");
            if (tgtPH()) {
                stmt.setBigDecimal(2, new BigDecimal("99999999999999999999.1"));
                stmt.setBigDecimal(3, new BigDecimal("99999999999999999999.1"));
                stmt.setBigDecimal(4, new BigDecimal("9999999999999999999999999999999999999.1"));
            } else if (tgtSQ()||tgtTR()) {
                stmt.setBigDecimal(2, new BigDecimal("9999999.11111111111"));
                stmt.setBigDecimal(3, new BigDecimal("99999999999999999.1"));
                stmt.setBigDecimal(4, new BigDecimal("99999999999999999.1"));
            }

            stmt.execute();
            conn.commit();
            stmt.setString(1, "2");
            stmt.setBigDecimal(2, new BigDecimal("0"));
            stmt.setBigDecimal(3, new BigDecimal("0"));
            stmt.setBigDecimal(4, new BigDecimal("0"));
            stmt.execute();
            conn.commit();
            stmt.setString(1, "3");
            stmt.setBigDecimal(2, new BigDecimal("0"));
            stmt.setBigDecimal(3, new BigDecimal("0"));
            stmt.setBigDecimal(4, new BigDecimal("0"));
            stmt.execute();
            conn.commit();
            
            // Averaging
            // result scale should be: max(max(ls, rs), 4).
            // We are not imposing restriction on precisioin.
            query = "SELECT avg(col1) FROM testDecimalArithmatic";
            stmt = conn.prepareStatement(query);
            ResultSet rs = stmt.executeQuery();
            assertTrue(rs.next());
            BigDecimal result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("33333333333333333333.03333333333"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("3333333.03703703703"), result);
            
            query = "SELECT avg(col2) FROM testDecimalArithmatic";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("33333333333333333333.0333"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("33333333333333333.0"), result);
            
            // We cap our decimal to a precision of 38.
            query = "SELECT avg(col3) FROM testDecimalArithmatic";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("3333333333333333333333333333333333333"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("33333333333333333.0"), result);
        } finally {
        }
    }

    @Test
    public void testDecimalArithmeticWithIntAndLong() throws Exception {
        printTestDescription();

        try {
            String ddl = null;
            if (tgtPH()) ddl = "CREATE TABLE IF NOT EXISTS testDecimalArithmatic" + 
                                    "  (pk VARCHAR NOT NULL PRIMARY KEY, " +
                                    "col1 DECIMAL(38,0), col2 DECIMAL(5, 2), col3 INTEGER, col4 BIGINT, col5 DECIMAL)";
            else if (tgtTR()) ddl = "CREATE TABLE IF NOT EXISTS testDecimalArithmatic" +
                                    "  (pk VARCHAR(128) NOT NULL PRIMARY KEY, " +
                                    "col1 DECIMAL(18,0), col2 DECIMAL(5, 2), col3 INTEGER, col4 BIGINT, col5 DECIMAL)";
            else if (tgtSQ()) ddl = "CREATE TABLE testDecimalArithmatic" +
                                    "  (pk VARCHAR(128) NOT NULL PRIMARY KEY, " +
                                    "col1 DECIMAL(18,0), col2 DECIMAL(5, 2), col3 INTEGER, col4 BIGINT, col5 DECIMAL)";
            conn.createStatement().execute(ddl);
        
            conn.setAutoCommit(false);
    
            String query = null;
            if (tgtPH()||tgtTR()) query = "UPSERT INTO testDecimalArithmatic(pk, col1, col2, col3, col4, col5) VALUES(?,?,?,?,?,?)";
            else if (tgtSQ()) query = "INSERT INTO testDecimalArithmatic(pk, col1, col2, col3, col4, col5) VALUES(?,?,?,?,?,?)";
            PreparedStatement stmt = conn.prepareStatement(query);
            stmt.setString(1, "testValueOne");
            if (tgtPH()) stmt.setBigDecimal(2, new BigDecimal("1234567890123456789012345678901"));
            else if (tgtSQ()||tgtTR()) stmt.setBigDecimal(2, new BigDecimal("123456789012345678"));
            stmt.setBigDecimal(3, new BigDecimal("123.45"));
            stmt.setInt(4, 10);
            stmt.setLong(5, 10L);
            stmt.setBigDecimal(6, new BigDecimal("111.111"));
            stmt.execute();
            conn.commit();

            stmt.setString(1, "testValueTwo");
            if (tgtPH()) stmt.setBigDecimal(2, new BigDecimal("12345678901234567890123456789012345678"));
            else if (tgtSQ()||tgtTR()) stmt.setBigDecimal(2, new BigDecimal("123456789012345678"));
            stmt.setBigDecimal(3, new BigDecimal("123.45"));
            stmt.setInt(4, 10);
            stmt.setLong(5, 10L);
            stmt.setBigDecimal(6, new BigDecimal("123456789.0123456789"));
            stmt.execute();
            conn.commit();
            
            // INT has a default precision and scale of (10, 0)
            // LONG has a default precision and scale of (19, 0)
            query = "SELECT col1 + col3 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            ResultSet rs = stmt.executeQuery();
            assertTrue(rs.next());
            BigDecimal result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("1234567890123456789012345678911"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("123456789012345688"), result);
            
            query = "SELECT col1 + col4 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("1234567890123456789012345678911"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("123456789012345688"), result);            
            query = "SELECT col2 + col3 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            assertEquals(new BigDecimal("133.45"), result);
 
            query = "SELECT col2 + col4 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("133.45"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("133"), result);
 
            query = "SELECT col5 + col3 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("121.111"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("121"), result);

            query = "SELECT col5 + col4 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("121.111"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("121"), result);
 
            query = "SELECT col1 - col3 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("1234567890123456789012345678891"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("123456789012345668"), result);
 
            query = "SELECT col1 - col4 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("1234567890123456789012345678891"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("123456789012345668"), result);
 
            query = "SELECT col2 - col3 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            assertEquals(new BigDecimal("113.45"), result);
 
            query = "SELECT col2 - col4 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("113.45"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("113"), result);
 
            query = "SELECT col5 - col3 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("101.111"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("101"), result);
 
            query = "SELECT col5 - col4 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("101.111"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("101"), result);
 
            query = "SELECT col1 * col3 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("1.234567890123456789012345678901E+31"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("1234567890123456780"), result);
 
            query = "SELECT col1 * col4 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("1.234567890123456789012345678901E+31"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("1234567890123456780"), result);

            query = "SELECT col1 * col3 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("1.234567890123456789012345678901E+31"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("1234567890123456780"), result);

            if (tgtPH()) { 
                try {
            	    query = "SELECT col1 * col3 FROM testDecimalArithmatic WHERE pk='testValueTwo'";
            	    stmt = conn.prepareStatement(query);
            	    rs = stmt.executeQuery();
            	    assertTrue(rs.next());
            	    result = rs.getBigDecimal(1);
            	    fail("Should have caught error.");
                } catch (Exception e) {
            	    assertTrue(e.getMessage(), e.getMessage().contains("ERROR 206 (22003): The value is outside the range for the data type. DECIMAL(38,0)"));
                }
            
                try {
            	    query = "SELECT col1 * col4 FROM testDecimalArithmatic WHERE pk='testValueTwo'";
            	    stmt = conn.prepareStatement(query);
            	    rs = stmt.executeQuery();
            	    assertTrue(rs.next());
            	    result = rs.getBigDecimal(1);
            	    fail("Should have caught error.");
                } catch (Exception e) {
            	    assertTrue(e.getMessage(), e.getMessage().contains("ERROR 206 (22003): The value is outside the range for the data type. DECIMAL(38,0)"));
                }
            }
 
            query = "SELECT col4 * col5 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(0, result.compareTo(new BigDecimal("1111.11")));
            else if (tgtSQ()||tgtTR()) assertEquals(0, result.compareTo(new BigDecimal("1110")));

            query = "SELECT col3 * col5 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(0, result.compareTo(new BigDecimal("1111.11")));
            else if (tgtSQ()||tgtTR()) assertEquals(0, result.compareTo(new BigDecimal("1110"))); 

            query = "SELECT col2 * col4 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("1234.5"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("1234"), result); 

            // Result scale has value of 0
            query = "SELECT col1 / col3 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("1.2345678901234567890123456789E+29"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("12345678901234567"), result);

            query = "SELECT col1 / col4 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("1.2345678901234567890123456789E+29"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("12345678901234567"), result);
            
            // Result scale is 2.
            query = "SELECT col2 / col3 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("12.34"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("12.345000000000"), result);
            
            query = "SELECT col2 / col4 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("12.34"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("12.345000000000000"), result);
 
            // col5 has NO_SCALE, so the result's scale is not expected to be truncated to col5 value's scale of 4
            query = "SELECT col5 / col3 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("11.1111"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("11.100000000"), result);
 
            query = "SELECT col5 / col4 FROM testDecimalArithmatic WHERE pk='testValueOne'";
            stmt = conn.prepareStatement(query);
            rs = stmt.executeQuery();
            assertTrue(rs.next());
            result = rs.getBigDecimal(1);
            if (tgtPH()) assertEquals(new BigDecimal("11.1111"), result);
            else if (tgtSQ()||tgtTR()) assertEquals(new BigDecimal("11.100000000"), result);
        } finally {
        }
    }
}
