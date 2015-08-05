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

public class CreateTableTest  extends BaseTest {

    @BeforeClass
    public static void doTestSuiteSetup() throws Exception {
        /* List all of the object names being used in this entire class.
         * The objects are dropped with errors ignored, so it is OK if the
         * object does not exist for a particular test.
         */
        objDropList = new ArrayList<String>(
            Arrays.asList("table m_interface_job"));
        doBaseTestSuiteSetup();
    }
    /* @AfterClass, @Before, @After are defined in BaseTest */

    @Test
    public void testCreateTable() throws Exception {
        printTestDescription();

        if (tgtPH()) conn.createStatement().execute("CREATE TABLE m_interface_job(                data.addtime VARCHAR ,\n" + 
        		"                data.dir VARCHAR ,\n" + 
        		"                data.end_time VARCHAR ,\n" + 
        		"                data.file VARCHAR ,\n" + 
        		"                data.fk_log VARCHAR ,\n" + 
        		"                data.host VARCHAR ,\n" + 
        		"                data.row VARCHAR ,\n" + 
        		"                data.size VARCHAR ,\n" + 
        		"                data.start_time VARCHAR ,\n" + 
        		"                data.stat_date DATE ,\n" + 
        		"                data.stat_hour VARCHAR ,\n" + 
        		"                data.stat_minute VARCHAR ,\n" + 
        		"                data.state VARCHAR ,\n" + 
        		"                data.title VARCHAR ,\n" + 
        		"                data.user VARCHAR ,\n" + 
        		"                data.inrow VARCHAR ,\n" + 
        		"                data.jobid VARCHAR ,\n" + 
        		"                data.jobtype VARCHAR ,\n" + 
        		"                data.level VARCHAR ,\n" + 
        		"                data.msg VARCHAR ,\n" + 
        		"                data.outrow VARCHAR ,\n" + 
        		"                data.pass_time VARCHAR ,\n" + 
        		"                data.type VARCHAR ,\n" + 
        		"                id INTEGER not null primary key desc\n" + 
        		"                ) ");
        else if (tgtSQ()||tgtTR()) conn.createStatement().execute("CREATE TABLE m_interface_job(                addtime VARCHAR(128) ,\n" +
                        "                dir VARCHAR(128) ,\n" +
                        "                end_time VARCHAR(128) ,\n" +
                        "                file VARCHAR(128) ,\n" +
                        "                fk_log VARCHAR(128) ,\n" +
                        // TRAF "                host VARCHAR(128) ,\n" +
                        // TRAF "                row VARCHAR(128) ,\n" +
                        // TRAF "                size VARCHAR(128) ,\n" +
                        "                start_time VARCHAR(128) ,\n" +
                        "                stat_date DATE ,\n" +
                        "                stat_hour VARCHAR(128) ,\n" +
                        "                stat_minute VARCHAR(128) ,\n" +
                        "                state VARCHAR(128) ,\n" +
                        "                title VARCHAR(128) ,\n" +
                        // TRAF "                user VARCHAR(128) ,\n" +
                        "                inrow VARCHAR(128) ,\n" +
                        "                jobid VARCHAR(128) ,\n" +
                        "                jobtype VARCHAR(128) ,\n" +
                        // TRAF "                level VARCHAR(128) ,\n" +
                        "                msg VARCHAR(128) ,\n" +
                        "                outrow VARCHAR(128) ,\n" +
                        "                pass_time VARCHAR(128) ,\n" +
                        "                type VARCHAR(128) ,\n" +
                        "                id INTEGER not null primary key desc\n" +
                        "                ) ");


        conn.createStatement().execute("DROP TABLE m_interface_job");
    }
}
