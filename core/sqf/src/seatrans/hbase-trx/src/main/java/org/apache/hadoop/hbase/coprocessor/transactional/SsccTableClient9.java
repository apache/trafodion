/**
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**/

package org.apache.hadoop.hbase.coprocessor.transactional;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class SsccTableClient9 {
    // ------ for concurrent ------
    protected final Log log = LogFactory.getLog(getClass());
    private static int successNum = 0;

    /**
     * concurrency writes
     * 
     * @param args
     * @throws Exception
     */
    static public void main(String[] args) throws Exception {
        SsccTableClient9 cilent = new SsccTableClient9();
        cilent.concurrencyWrites1();

        System.out.println("=========================================");
        System.out.println(" ");
        System.out.println("TOTAL : 1 . SUCCESS : " + successNum + " FAILURE : " + (1 - successNum));
        System.out.println(" ");
        System.out.println("=========================================");
    }

    /**
     * main--put[v1]--del[v1]--put[v2]--get[v2]<br/>
     */
    private void concurrencyWrites1() {
        try {
            System.out.println("Starting SsccTableClient9: concurrencyWrites1");

            System.out.println("main--put[v1]--del[v1]--put[v2]--get[v2]");

            SsccTableClientUtils.initialize();
            SsccTableClientUtils.testSsccBeginTransaction();

            SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE1);
            SsccTableClientUtils.testSsccDelete(SsccTableClientUtils.VALUE1);
            SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);
            SsccTableClientUtils.testSsccCommitIfPossible();
            SsccTableClientUtils.testSsccBeginTransaction();
            Integer[] result = SsccTableClientUtils.testSsccGet();
            SsccTableClientUtils.testSsccCommitIfPossible();
            System.out.println("Finish SsccTableClient9: concurrencyWrites1");

            if (result.length == 1 && result[0] == 2) {
                successNum++;
                System.out.println("=========================================");
                System.out.println(" ");
                System.out.println("SUCCESS");
                System.out.println(" ");
                System.out.println("=========================================");
            } else {
                System.out.println("=========================================");
                System.out.println(" ");
                System.out.println("FAILURE");
                System.out.println(" ");
                System.out.println("=========================================");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}