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

public class AutoCommitTest extends BaseTest {

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table test_table"));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    @Test
    public void testMutationJoin() throws Exception {
        printTestDescription();        

        conn.setAutoCommit(true);
        
        String ddl = null;
        if (tgtPH()) ddl = "CREATE TABLE test_table " +
                           "  (row varchar not null, col1 integer" +
                           "  CONSTRAINT pk PRIMARY KEY (row))\n";
        else if (tgtSQ()||tgtTR()) ddl = "CREATE TABLE test_table " +
                                "  (row1 varchar(128) not null, col1 integer" +
                                "  , CONSTRAINT pk PRIMARY KEY (row1))\n";
        conn.createStatement().execute(ddl);
        
        String query = null;
        if (tgtPH()) query = "UPSERT INTO test_table(row, col1) VALUES('row1', 1)";
        else if (tgtTR()) query = "UPSERT INTO test_table(row1, col1) VALUES('row1', 1)";
        else if (tgtSQ()) query = "INSERT INTO test_table(row1, col1) VALUES('row1', 1)";
        PreparedStatement statement = conn.prepareStatement(query);
        statement.executeUpdate();
        
        conn.setAutoCommit(false);
        if (tgtPH()) query = "UPSERT INTO test_table(row, col1) VALUES('row1', 2)";
        else if (tgtTR()) query = "UPSERT INTO test_table(row1, col1) VALUES('row1', 2)";
        else if (tgtSQ()) query = "UPDATE test_table set col1=2 where row1='row1'";
        statement = conn.prepareStatement(query);
        statement.executeUpdate();
        
        if (tgtPH()) query = "DELETE FROM test_table WHERE row='row1'";
        else if (tgtSQ()||tgtTR()) query = "DELETE FROM test_table WHERE row1='row1'";
        statement = conn.prepareStatement(query);
        statement.executeUpdate();
        conn.commit();
        
        query = "SELECT * FROM test_table";
        statement = conn.prepareStatement(query);
        ResultSet rs = statement.executeQuery();
        assertFalse(rs.next());

        if (tgtPH()) query = "DELETE FROM test_table WHERE row='row1'";
        else if (tgtSQ()||tgtTR()) query = "DELETE FROM test_table WHERE row1='row1'";
        statement = conn.prepareStatement(query);
        statement.executeUpdate();

        if (tgtPH()) query = "UPSERT INTO test_table(row, col1) VALUES('row1', 3)";
        else if (tgtTR()) query = "UPSERT INTO test_table(row1, col1) VALUES('row1', 3)";
        else if (tgtSQ()) query = "INSERT INTO test_table(row1, col1) VALUES('row1', 3)";
        statement = conn.prepareStatement(query);
        statement.executeUpdate();
        conn.commit();
        
        query = "SELECT * FROM test_table";
        statement = conn.prepareStatement(query);
        rs = statement.executeQuery();
        assertTrue(rs.next());
        assertEquals("row1", rs.getString(1));
        assertEquals(3, rs.getInt(2));
    }
}
