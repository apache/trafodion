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
import java.util.*;

public class CompareDecimalToLongTest extends BaseTest {

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table LongInKeyTest"));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    protected void initTableValues() throws Exception {
        createTestTable("LongInKeyTest");
        
        conn.setAutoCommit(true);
        PreparedStatement stmt = null;
        if (tgtPH()||tgtTR()) stmt = conn.prepareStatement(
                "upsert into " +
                "LongInKeyTest VALUES(?)");
        else if (tgtSQ()) stmt = conn.prepareStatement(
                "insert into " +
                "LongInKeyTest VALUES(?)");
        stmt.setLong(1, 2);
        stmt.execute();
    }

    @Test
    public void testCompareLongGTDecimal() throws Exception {
        printTestDescription();

        initTableValues();
        String query = "SELECT l FROM LongInKeyTest where l > 1.5";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            assertTrue (rs.next());
            assertEquals(2, rs.getLong(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testCompareLongGTEDecimal() throws Exception {
        printTestDescription();

        initTableValues();
        String query = "SELECT l FROM LongInKeyTest where l >= 1.5";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            /*
             *  Failing because we're not converting the constant to the type of the RHS
             *  when forming the start/stop key.
             *  For this case, 1.5 -> 1L
             *  if where l < 1.5 then 1.5 -> 1L and then to 2L because it's not inclusive
             *  
             */
            assertTrue (rs.next());
            assertEquals(2, rs.getLong(1));
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testCompareLongLTDecimal() throws Exception {
        printTestDescription();

        initTableValues();
        String query = "SELECT l FROM LongInKeyTest where l < 1.5";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            /*
             *  Failing because we're not converting the constant to the type of the RHS
             *  when forming the start/stop key.
             *  For this case, 1.5 -> 1L
             *  if where l < 1.5 then 1.5 -> 1L and then to 2L because it's not inclusive
             *  
             */
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testCompareLongLTEDecimal() throws Exception {
        printTestDescription();

        initTableValues();
        String query = "SELECT l FROM LongInKeyTest where l <= 1.5";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            /*
             *  Failing because we're not converting the constant to the type of the RHS
             *  when forming the start/stop key.
             *  For this case, 1.5 -> 1L
             *  if where l < 1.5 then 1.5 -> 1L and then to 2L because it's not inclusive
             *  
             */
            assertFalse(rs.next());
        } finally {
        }
    }
    @Test
    public void testCompareLongGTDecimal2() throws Exception {
        printTestDescription();

        initTableValues();
        String query = "SELECT l FROM LongInKeyTest where l > 2.5";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            /*
             *  Failing because we're not converting the constant to the type of the RHS
             *  when forming the start/stop key.
             *  For this case, 1.5 -> 1L
             *  if where l < 1.5 then 1.5 -> 1L and then to 2L because it's not inclusive
             *  
             */
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testCompareLongGTEDecimal2() throws Exception {
        printTestDescription();

        initTableValues();
        String query = "SELECT l FROM LongInKeyTest where l >= 2.5";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            /*
             *  Failing because we're not converting the constant to the type of the RHS
             *  when forming the start/stop key.
             *  For this case, 1.5 -> 1L
             *  if where l < 1.5 then 1.5 -> 1L and then to 2L because it's not inclusive
             *  
             */
            assertFalse(rs.next());
        } finally {
        }
    }
    
    @Test
    public void testCompareLongLTDecimal2() throws Exception {
        printTestDescription();

        initTableValues();
        String query = "SELECT l FROM LongInKeyTest where l < 2.5";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            /*
             *  Failing because we're not converting the constant to the type of the RHS
             *  when forming the start/stop key.
             *  For this case, 1.5 -> 1L
             *  if where l < 1.5 then 1.5 -> 1L and then to 2L because it's not inclusive
             *  
             */
            assertTrue (rs.next());
            assertEquals(2, rs.getLong(1));
            assertFalse(rs.next());
        } finally {
        }
    }

    @Test
    public void testCompareLongLTEDecimal2() throws Exception {
        printTestDescription();

        initTableValues();
        String query = "SELECT l FROM LongInKeyTest where l <= 2.5";
        try {
            PreparedStatement statement = conn.prepareStatement(query);
            ResultSet rs = statement.executeQuery();
            /*
             *  Failing because we're not converting the constant to the type of the RHS
             *  when forming the start/stop key.
             *  For this case, 1.5 -> 1L
             *  if where l < 1.5 then 1.5 -> 1L and then to 2L because it's not inclusive
             *  
             */
            assertTrue (rs.next());
            assertEquals(2, rs.getLong(1));
            assertFalse(rs.next());
        } finally {
        }
    }
}
