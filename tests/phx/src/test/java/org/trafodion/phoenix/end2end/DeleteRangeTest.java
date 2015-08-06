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

public class DeleteRangeTest extends BaseTest {

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table IntIntKeyTest"));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    private static final int NUMBER_OF_ROWS = 20;
    private static final int NTH_ROW_NULL = 5;
    
    private void initTableValues() throws SQLException {
        createTestTable("IntIntKeyTest");
        String upsertStmt = null;
        if (tgtPH()||tgtTR()) upsertStmt = "UPSERT INTO IntIntKeyTest VALUES(?,?)";
        else if (tgtSQ()) upsertStmt = "INSERT INTO IntIntKeyTest VALUES(?,?)";

        PreparedStatement stmt = conn.prepareStatement(upsertStmt);
        for (int i = 0; i < NUMBER_OF_ROWS; i++) {
            stmt.setInt(1, i);
            if (i % NTH_ROW_NULL != 0) {
                stmt.setInt(2, i * 10);
            } else {
                stmt.setNull(2, Types.INTEGER);
            }
            stmt.execute();
        }
    }
    
    private void testDeleteRange(boolean autoCommit) throws Exception {
        initTableValues();
        
        ResultSet rs;
        rs = conn.createStatement().executeQuery("SELECT count(*) FROM IntIntKeyTest");
        assertTrue(rs.next());
        assertEquals(NUMBER_OF_ROWS, rs.getInt(1));

        if (tgtPH()) rs = conn.createStatement().executeQuery("SELECT i FROM IntIntKeyTest WHERE j IS NULL");
        else if (tgtSQ()||tgtTR()) rs = conn.createStatement().executeQuery("SELECT i FROM IntIntKeyTest WHERE j IS NULL order by 1");

        int i = 0, count = 0;
        while (rs.next()) {
            assertEquals(i,rs.getInt(1));
            i += NTH_ROW_NULL;
            count++;
        }
        rs = conn.createStatement().executeQuery("SELECT count(*) FROM IntIntKeyTest WHERE j IS NOT NULL");
        assertTrue(rs.next());
        assertEquals(NUMBER_OF_ROWS-count, rs.getInt(1));

        conn.setAutoCommit(autoCommit);
        String deleteStmt = "DELETE FROM IntIntKeyTest WHERE i >= ? and i < ?";
        PreparedStatement stmt = conn.prepareStatement(deleteStmt);
        stmt.setInt(1, 5);
        stmt.setInt(2, 10);
        stmt.execute();
        if (!autoCommit) {
            conn.commit();
        }
        
        rs = conn.createStatement().executeQuery("SELECT count(*) FROM IntIntKeyTest");
        assertTrue(rs.next());
        assertEquals(NUMBER_OF_ROWS - (10-5), rs.getInt(1));
    }
    
    @Test
    public void testDeleteRangeNoAutoCommit() throws Exception {
        printTestDescription();

        testDeleteRange(false);
    }
    
    @Test
    public void testDeleteRangeAutoCommit() throws Exception {
        printTestDescription();

        testDeleteRange(true);
    }
}
