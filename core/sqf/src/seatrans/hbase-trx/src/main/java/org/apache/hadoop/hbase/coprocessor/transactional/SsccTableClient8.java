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

import java.io.IOException;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class SsccTableClient8 {
    // ------ for concurrent ------
    private static CountDownLatch cdl = new CountDownLatch(2);
    private static CountDownLatch cdl2 = new CountDownLatch(2);
    private static Lock lock = new ReentrantLock();
    private static Condition t1Condition = lock.newCondition();
    private static Condition t2Condition = lock.newCondition();
    protected final Log log = LogFactory.getLog(getClass());

    private static int successNum = 0;
    private boolean first = false;

    /**
     * concurrency writes
     * 
     * @param args
     * @throws Exception
     */
    static public void main(String[] args) throws Exception {
        SsccTableClient8 cilent = new SsccTableClient8();
        cilent.concurrencyWrites1();
        cilent.concurrencyWrites2();

        System.out.println("=========================================");
        System.out.println(" ");
        System.out.println("TOTAL : 2 . SUCCESS : " + successNum + " FAILURE : " + (2 - successNum));
        System.out.println(" ");
        System.out.println("=========================================");
    }

    private static boolean compareScanResult(List<Object[]> befores, List<Object[]> laters) {
        if (befores.size() != laters.size()) {
            return false;
        }
        int k = 0;
        for (int i = 0; i < befores.size(); i++) {
            for (int j = 0; j < laters.size(); j++) {
                if (Arrays.equals((byte[]) laters.get(j)[0], (byte[]) befores.get(i)[0])
                        && ((Long) laters.get(j)[4]).longValue() == ((Long) befores.get(i)[4]).longValue()) {
                    k++;
                }
            }
        }
        return befores.size() == k;
    }

    /**
     * main--put[v1,v2,v3]<br/>
     * t1-----------beginTrans---scan[v1,v2,v3]--------------------------------
     * ----------scan[v1,v2,v3]<br/>
     * t2---------------------------------------------beginTrans---put[v4]---
     * commit<br/>
     * main--- scan
     */
    private void concurrencyWrites1() {
        try {
            System.out.println("Starting SsccTableClient8: concurrencyWrites1");
            System.out.println("main--put[v1,v2,v3]");
            System.out
                    .println("t1-----------beginTrans---scan[v1,v2,v3]------------------------------------------scan[v1,v2,v3]");
            System.out.println("t2---------------------------------------------beginTrans---put[v4]---commit");
            System.out.println("main--- scan");

            SsccTableClientUtils.initialize();
            putValue();

            final P3Sscc01 t1 = new P3Sscc01();
            final P3Sscc02 t2 = new P3Sscc02();

            new Thread(new Runnable() {
                @Override
                public void run() {
                    first = t1.doWork();
                    System.out.println("Sscc1 finished");
                    cdl.countDown();
                }
            }, "Sscc1").start();

            // to make sure t2 is later than t1
            try {
                Thread.sleep(1000);

            } catch (InterruptedException e1) {
                e1.printStackTrace();
            }

            new Thread(new Runnable() {
                @Override
                public void run() {
                    t2.doWork();
                    System.out.println("Sscc2 finished");
                    cdl.countDown();
                }
            }, "Sscc2").start();

            cdl.await();

            SsccTableClientUtils.testSsccOpenScanner();
            List<Object[]> result = SsccTableClientUtils.testSsccPerformScan();
            SsccTableClientUtils.testSsccCloseScanner();

            System.out.println("Finish SsccTableClient5: concurrencyWrites1");

            if (result.size() == 4 && first == true) {
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

    private void putValue() throws IOException {
        SsccTableClientUtils.testSsccBeginTransaction();
        SsccTableClientUtils.testSsccPutRow(SsccTableClientUtils.ROW1);
        SsccTableClientUtils.testSsccPutRow(SsccTableClientUtils.ROW2);
        SsccTableClientUtils.testSsccPutRow(SsccTableClientUtils.ROW3);
        SsccTableClientUtils.testSsccCommitIfPossible();

    }

    static class P3Sscc01 {
        protected final Log log = LogFactory.getLog(getClass());

        public boolean doWork() {
            boolean first = false;
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                SsccTableClientUtils.testSsccOpenScanner();
                List<Object[]> before = SsccTableClientUtils.testSsccPerformScan();
                SsccTableClientUtils.testSsccCloseScanner();
                System.out
                        .println("Sscc1 had begun transaction & read v1,v2,v3 and waits for Sscc2 to begin transaction & write v4");
                t2Condition.signal();
                t1Condition.await();
                SsccTableClientUtils.testSsccOpenScanner();
                List<Object[]> later = SsccTableClientUtils.testSsccPerformScan();
                SsccTableClientUtils.testSsccCloseScanner();
                // commit
                SsccTableClientUtils.testSsccCommitIfPossible();
                first = compareScanResult(before, later);

            } catch (Exception e) {
                System.out.println("Error in Sscc1: ");
                e.printStackTrace();
            } finally {
                lock.unlock();
            }
            return first;
        }

    }

    static class P3Sscc02 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                SsccTableClientUtils.testSsccPutRow(SsccTableClientUtils.ROW4);

                // commit
                SsccTableClientUtils.testSsccCommitIfPossible();
                System.out
                        .println("Sscc2 had begun transaction & wroten v4 & commited and waits for Sscc1 to scan v1,v2,v3 & commit");

            } catch (Exception e) {
                System.out.println("Error in Sscc2: ");
                e.printStackTrace();
            } finally {
                t1Condition.signal();
                lock.unlock();
            }
        }
    }

    /**
     * main--put[v1,v2,v3]<br/>
     * t1-----------beginTrans---scan[v1,v2,v3]--------------------------------
     * ----------scan[v1,v2,v3]<br/>
     * t2---------------------------------------------beginTrans---put[v4]---
     * abort<br/>
     * main--- scan
     */
    private void concurrencyWrites2() {
        try {
            System.out.println("Starting SsccTableClient8: concurrencyWrites2");
            System.out.println("main--put[v1,v2,v3]");
            System.out
                    .println("t1-----------beginTrans---scan[v1,v2,v3]------------------------------------------scan[v1,v2,v3]");
            System.out.println("t2---------------------------------------------beginTrans---put[v4]---abort");
            System.out.println("main---scan");

            SsccTableClientUtils.initialize();
            putValue();

            final P3Sscc11 t1 = new P3Sscc11();
            final P3Sscc12 t2 = new P3Sscc12();

            new Thread(new Runnable() {
                @Override
                public void run() {
                    first = t1.doWork();
                    System.out.println("Sscc1 finished");
                    cdl2.countDown();
                }
            }, "Sscc1").start();

            // to make sure t2 is later than t1
            try {
                Thread.sleep(1000);

            } catch (InterruptedException e1) {
                e1.printStackTrace();
            }

            new Thread(new Runnable() {
                @Override
                public void run() {
                    t2.doWork();
                    System.out.println("Sscc2 finished");
                    cdl2.countDown();
                }
            }, "Sscc2").start();

            cdl2.await();

            SsccTableClientUtils.testSsccOpenScanner();
            List<Object[]> result = SsccTableClientUtils.testSsccPerformScan();
            SsccTableClientUtils.testSsccCloseScanner();

            System.out.println("Finish SsccTableClient5: concurrencyWrites2");

            if (result.size() == 3 && first == true) {
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

    static class P3Sscc11 {
        protected final Log log = LogFactory.getLog(getClass());

        public boolean doWork() {
            boolean first = false;
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                SsccTableClientUtils.testSsccOpenScanner();
                List<Object[]> before = SsccTableClientUtils.testSsccPerformScan();
                SsccTableClientUtils.testSsccCloseScanner();
                System.out
                        .println("Sscc1 had begun transaction & read v1,v2,v3 and waits for Sscc2 to begin transaction & write v4");

                t2Condition.signal();
                t1Condition.await();
                SsccTableClientUtils.testSsccOpenScanner();
                List<Object[]> later = SsccTableClientUtils.testSsccPerformScan();
                SsccTableClientUtils.testSsccCloseScanner();
                // commit
                SsccTableClientUtils.testSsccCommitIfPossible();

                first = compareScanResult(before, later);
            } catch (Exception e) {
                System.out.println("Error in Sscc1: ");
                e.printStackTrace();
            } finally {
                lock.unlock();
            }
            return first;
        }
    }

    static class P3Sscc12 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                SsccTableClientUtils.testSsccPutRow(SsccTableClientUtils.ROW4);
                // commit
                SsccTableClientUtils.testSsccAbortTransaction();
                System.out
                        .println("Sscc2 had begun transaction & wroten v4 & rollback and waits for Sscc1 to scan v1,v2,v3 & commit");

            } catch (Exception e) {
                System.out.println("Error in Sscc2: ");
                e.printStackTrace();
            } finally {
                t1Condition.signal();
                lock.unlock();
            }
        }
    }

}