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
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class SSccTableClient5 {
    protected final Log log = LogFactory.getLog(getClass());
    // ------ for concurrent ------
    private static CountDownLatch cdl = new CountDownLatch(2);
    private static CountDownLatch cdl2 = new CountDownLatch(2);
    private static CountDownLatch cdl3 = new CountDownLatch(2);
    private static CountDownLatch cdl4 = new CountDownLatch(2);
    private static CountDownLatch cdl5 = new CountDownLatch(2);
    private static CountDownLatch cdl6 = new CountDownLatch(2);
    private static CountDownLatch cdl7 = new CountDownLatch(2);
    private static CountDownLatch cdl8 = new CountDownLatch(2);
    private static Lock lock = new ReentrantLock();
    private static Condition t1Condition = lock.newCondition();
    private static Condition t2Condition = lock.newCondition();

    private static int successNum = 0;

    /**
     * concurrency writes
     * 
     * @param args
     * @throws Exception
     */
    static public void main(String[] args) throws Exception {
        SSccTableClient5 cilent = new SSccTableClient5();

        cilent.concurrencyWrites1();
        cilent.concurrencyWrites2();
        cilent.concurrencyWrites3();
        cilent.concurrencyWrites4();
        cilent.concurrencyWrites5();
        cilent.concurrencyWrites6();
        cilent.concurrencyWrites7();
        cilent.concurrencyWrites8();

        System.out.println("=========================================");
        System.out.println(" ");
        System.out.println("TOTAL : 8 . SUCCESS : " + successNum + " FAILURE : " + (8 - successNum));
        System.out.println(" ");
        System.out.println("=========================================");
    }

    /**
     * main--put[v1]<br/>
     * t1-----------beginTrans---put[v2]-------------------------------commit<br/>
     * t2------------------------------------------beginTrans---put[v3]--------
     * -----commit<br/>
     * main---get[v2]
     */
    private void concurrencyWrites1() {
        try {
            System.out.println("Starting SsccTableClient5: concurrencyWrites1");
            System.out.println("main--put[v1]");
            System.out.println("t1-----------beginTrans---put[v2]---------------------------commit");
            System.out.println("t2-------------------------------------beginTrans---put[v3]----------commit");
            System.out.println("main---get[v2]");

            SsccTableClientUtils.initialize();
            putValue();

            final P0Sscc01 t1 = new P0Sscc01();
            final P0Sscc02 t2 = new P0Sscc02();

            new Thread(new Runnable() {
                @Override
                public void run() {
                    t1.doWork();
                    System.out.println("  Sscc1 finished");
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
                    System.out.println("  Sscc2 finished");
                    cdl.countDown();
                }
            }, "Sscc2").start();

            cdl.await();
            SsccTableClientUtils.testSsccBeginTransaction();
            Integer[] result = SsccTableClientUtils.testSsccGet();
            SsccTableClientUtils.testSsccCommitIfPossible();
            System.out.println("Finish SsccTableClient5: concurrencyWrites1");

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

    private void putValue() throws IOException {
        SsccTableClientUtils.testSsccBeginTransaction();
        SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE1);
        SsccTableClientUtils.testSsccCommitIfPossible();

    }

    static class P0Sscc01 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);

                System.out
                        .println("Sscc1 had begun transaction & wroten v2 and waits for Sscc2 to begin transaction & write v3");

                t2Condition.signal();
                t1Condition.await();

                // commit
                SsccTableClientUtils.testSsccCommitIfPossible();

            } catch (Exception e) {
                System.out.println("=======Error in Sscc1: =======");
                e.printStackTrace();
                System.out.println("=======Error in Sscc1: =======");
            } finally {
                t2Condition.signal();
                lock.unlock();
            }
        }
    }

    static class P0Sscc02 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
                System.out.println("  Sscc2 had begun transaction & wroten v3 and waits for Sscc1 to commit");

                t1Condition.signal();
                t2Condition.await();

                // commit
                SsccTableClientUtils.testSsccCommitIfPossible();
            } catch (Exception e) {
                System.out.println("=======Error in Sscc2: =======");
                e.printStackTrace();
                System.out.println("=======Error in Sscc2: =======");
            } finally {
                lock.unlock();
            }
        }
    }

    /**
     * main--put[v1]<br/>
     * t1-----------beginTrans---put[v2]-------------------------------commit<br/>
     * t2------------------------------------------beginTrans---put[v3]--------
     * -----abort<br/>
     * main---get[v2]
     */
    private void concurrencyWrites2() {
        try {
            System.out.println("Starting SsccTableClient5: concurrencyWrites2");
            System.out.println("main--put[v1]");
            System.out.println("t1-----------beginTrans---put[v2]---------------------------commit");
            System.out.println("t2-------------------------------------beginTrans---put[v3]----------abort");
            System.out.println("main---get[v2]");
            SsccTableClientUtils.initialize();
            putValue();

            final P0Sscc11 t1 = new P0Sscc11();
            final P0Sscc12 t2 = new P0Sscc12();

            new Thread(new Runnable() {
                @Override
                public void run() {
                    t1.doWork();
                    System.out.println("  Sscc1 finished");
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
                    System.out.println("  Sscc2 finished");
                    cdl2.countDown();
                }
            }, "Sscc2").start();

            cdl2.await();

            SsccTableClientUtils.testSsccBeginTransaction();
            Integer[] result = SsccTableClientUtils.testSsccGet();
            SsccTableClientUtils.testSsccCommitIfPossible();
            System.out.println("Finish SsccTableClient5: concurrencyWrites2");

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

    static class P0Sscc11 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);

                System.out
                        .println("  Sscc1 had begun transaction & wroten v2 and waits for Sscc2 to begin transaction & write v3");

                t2Condition.signal();
                t1Condition.await();

                // commit
                SsccTableClientUtils.testSsccCommitIfPossible();

            } catch (Exception e) {
                System.out.println("=======Error in Sscc1: =======");
                e.printStackTrace();
                System.out.println("=======Error in Sscc1: =======");
            } finally {
                t2Condition.signal();
                lock.unlock();
            }
        }
    }

    static class P0Sscc12 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
                System.out.println("  Sscc2 had begun transaction & wroten v3 and waits for Sscc1 to commit");

                t1Condition.signal();
                t2Condition.await();

                // commit
                SsccTableClientUtils.testSsccAbortTransaction();
            } catch (Exception e) {
                System.out.println("=======Error in Sscc2: =======");
                e.printStackTrace();
                System.out.println("=======Error in Sscc2: =======");
            } finally {
                lock.unlock();
            }
        }
    }

    /**
     * main--put[v1]<br/>
     * t1-----------beginTrans---put[v2]-------------------------------abort<br/>
     * t2------------------------------------------beginTrans---put[v3]--------
     * -----commit<br/>
     * main---get[v1]
     */
    private void concurrencyWrites3() {
        try {
            System.out.println("Starting SsccTableClient5: concurrencyWrites3");
            System.out.println("main--put[v1]");
            System.out.println("t1-----------beginTrans---put[v2]---------------------------abort");
            System.out.println("t2-------------------------------------beginTrans---put[v3]----------commit");
            System.out.println("main---get[v1]");
            SsccTableClientUtils.initialize();
            putValue();

            final P0Sscc21 t1 = new P0Sscc21();
            final P0Sscc22 t2 = new P0Sscc22();

            new Thread(new Runnable() {
                @Override
                public void run() {
                    t1.doWork();
                    System.out.println("  Sscc1 finished");
                    cdl3.countDown();
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
                    System.out.println("  Sscc2 finished");
                    cdl3.countDown();
                }
            }, "Sscc2").start();

            cdl3.await();

            SsccTableClientUtils.testSsccBeginTransaction();
            Integer[] result = SsccTableClientUtils.testSsccGet();
            SsccTableClientUtils.testSsccCommitIfPossible();
            System.out.println("Finish SsccTableClient5: concurrencyWrites3");

            if (result.length == 1 && result[0] == 1) {
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

    static class P0Sscc21 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);

                System.out
                        .println("  Sscc1 had begun transaction & wroten v2 and waits for Sscc2 to begin transaction & write v3");

                t2Condition.signal();
                t1Condition.await();

                // commit
                SsccTableClientUtils.testSsccAbortTransaction();

            } catch (Exception e) {
                System.out.println("=======Error in Sscc1: =======");
                e.printStackTrace();
                System.out.println("=======Error in Sscc1: =======");
            } finally {
                t2Condition.signal();
                lock.unlock();
            }
        }
    }

    static class P0Sscc22 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
                System.out.println("  Sscc2 had begun transaction & wroten v3 and waits for Sscc1 to commit");

                t1Condition.signal();
                t2Condition.await();

                // commit
                SsccTableClientUtils.testSsccCommitIfPossible();
            } catch (Exception e) {
                System.out.println("=======Error in Sscc2: =======");
                e.printStackTrace();
                System.out.println("=======Error in Sscc2: =======");
            } finally {
                lock.unlock();
            }
        }
    }

    /**
     * main--put[v1]<br/>
     * t1-----------beginTrans---put[v2]-------------------------------abort<br/>
     * t2------------------------------------------beginTrans---put[v3]--------
     * -----abort<br/>
     * main---get[v1]
     */
    private void concurrencyWrites4() {
        try {
            System.out.println("Starting SsccTableClient5: concurrencyWrites4");
            System.out.println("main--put[v1]");
            System.out.println("t1-----------beginTrans---put[v2]---------------------------abort");
            System.out.println("t2-------------------------------------beginTrans---put[v3]----------abort");
            System.out.println("main---get[v1]");
            SsccTableClientUtils.initialize();
            putValue();

            final P0Sscc31 t1 = new P0Sscc31();
            final P0Sscc32 t2 = new P0Sscc32();

            new Thread(new Runnable() {
                @Override
                public void run() {
                    t1.doWork();
                    System.out.println("  Sscc1 finished");
                    cdl4.countDown();
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
                    System.out.println("  Sscc2 finished");
                    cdl4.countDown();
                }
            }, "Sscc2").start();

            cdl4.await();

            SsccTableClientUtils.testSsccBeginTransaction();
            Integer[] result = SsccTableClientUtils.testSsccGet();
            SsccTableClientUtils.testSsccCommitIfPossible();
            System.out.println("Finish SsccTableClient5: concurrencyWrites4");

            if (result.length == 1 && result[0] == 1) {
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

    static class P0Sscc31 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);

                System.out
                        .println("  Sscc1 had begun transaction & wroten v2 and waits for Sscc2 to begin transaction & write v3");

                t2Condition.signal();
                t1Condition.await();

                // commit
                SsccTableClientUtils.testSsccAbortTransaction();

            } catch (Exception e) {
                System.out.println("=======Error in Sscc1: =======");
                e.printStackTrace();
                System.out.println("=======Error in Sscc1: =======");
            } finally {
                t2Condition.signal();
                lock.unlock();
            }
        }
    }

    static class P0Sscc32 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
                System.out.println("  Sscc2 had begun transaction & wroten v3 and waits for Sscc1 to commit");

                t1Condition.signal();
                t2Condition.await();

                // commit
                SsccTableClientUtils.testSsccAbortTransaction();
            } catch (Exception e) {
                System.out.println("=======Error in Sscc2: =======");
                e.printStackTrace();
                System.out.println("=======Error in Sscc2: =======");
            } finally {
                lock.unlock();
            }
        }
    }

    /**
     * main--put[v1]<br/>
     * t1-----------beginTrans------------------------------------------put[v2]
     * ---commit<br/>
     * t2-------------------------beginTrans---put[v3]---commit <br/>
     * main---get[v3]
     */
    private void concurrencyWrites5() {
        try {
            System.out.println("Starting SsccTableClient5: concurrencyWrites5");
            System.out.println("main--put[v1]");
            System.out.println("t1-----------beginTrans------------------------------------------put[v2]---commit");
            System.out.println("t2-------------------------beginTrans---put[v3]---commit");
            System.out.println("main---get[v3]");

            SsccTableClientUtils.initialize();
            putValue();

            final P0Sscc41 t1 = new P0Sscc41();
            final P0Sscc42 t2 = new P0Sscc42();

            new Thread(new Runnable() {
                @Override
                public void run() {
                    t1.doWork();
                    System.out.println("  Sscc1 finished");
                    cdl5.countDown();
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
                    System.out.println("  Sscc2 finished");
                    cdl5.countDown();
                }
            }, "Sscc2").start();

            cdl5.await();

            SsccTableClientUtils.testSsccBeginTransaction();
            Integer[] result = SsccTableClientUtils.testSsccGet();
            SsccTableClientUtils.testSsccCommitIfPossible();
            System.out.println("Finish SsccTableClient5: concurrencyWrites5");

            if (result.length == 1 && result[0] == 3) {
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

    static class P0Sscc41 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                System.out
                        .println("  Sscc1 had begun transaction and waits for Sscc2 to begin transaction & write v3 & commit");

                t2Condition.signal();
                t1Condition.await();
                SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);

                // commit
                SsccTableClientUtils.testSsccCommitIfPossible();

            } catch (Exception e) {
                System.out.println("=======Error in Sscc1: =======");
                e.printStackTrace();
                System.out.println("=======Error in Sscc1: =======");
            } finally {
                lock.unlock();
            }
        }
    }

    static class P0Sscc42 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
                // commit
                SsccTableClientUtils.testSsccCommitIfPossible();
                System.out.println("  Sscc2 had begun transaction & wroten v3 & commit");

                t1Condition.signal();

            } catch (Exception e) {
                System.out.println("=======Error in Sscc2: =======");
                e.printStackTrace();
                System.out.println("=======Error in Sscc2: =======");
            } finally {
                lock.unlock();
            }
        }
    }

    /**
     * main--put[v1]<br/>
     * t1-----------beginTrans------------------------------------------put[v2]
     * ---abort<br/>
     * t2-------------------------beginTrans---put[v3]---commit <br/>
     * main---get[v3]
     */
    private void concurrencyWrites6() {
        try {
            System.out.println("Starting SsccTableClient5: concurrencyWrites6");
            System.out.println("main--put[v1]");
            System.out.println("t1-----------beginTrans------------------------------------------put[v2]---abort");
            System.out.println("t2-------------------------beginTrans---put[v3]---commit");
            System.out.println("main---get[v3]");

            SsccTableClientUtils.initialize();
            putValue();

            final P0Sscc51 t1 = new P0Sscc51();
            final P0Sscc52 t2 = new P0Sscc52();

            new Thread(new Runnable() {
                @Override
                public void run() {
                    t1.doWork();
                    System.out.println("  Sscc1 finished");
                    cdl6.countDown();
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
                    System.out.println("  Sscc2 finished");
                    cdl6.countDown();
                }
            }, "Sscc2").start();

            cdl6.await();

            SsccTableClientUtils.testSsccBeginTransaction();
            Integer[] result = SsccTableClientUtils.testSsccGet();
            SsccTableClientUtils.testSsccCommitIfPossible();
            System.out.println("Finish SsccTableClient5: concurrencyWrites6");

            if (result.length == 1 && result[0] == 3) {
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

    static class P0Sscc51 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                System.out
                        .println("  Sscc1 had begun transaction and waits for Sscc2 to begin transaction & write v3 & commit");

                t2Condition.signal();
                t1Condition.await();
                SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);

                // commit
                SsccTableClientUtils.testSsccAbortTransaction();

            } catch (Exception e) {
                System.out.println("=======Error in Sscc1: =======");
                e.printStackTrace();
                System.out.println("=======Error in Sscc1: =======");
            } finally {
                lock.unlock();
            }
        }
    }

    static class P0Sscc52 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
                // commit
                SsccTableClientUtils.testSsccCommitIfPossible();
                System.out.println("  Sscc2 had begun transaction & wroten v3 & commit");

                t1Condition.signal();

            } catch (Exception e) {
                System.out.println("=======Error in Sscc2: =======");
                e.printStackTrace();
                System.out.println("=======Error in Sscc2: =======");
            } finally {
                lock.unlock();
            }
        }
    }

    /**
     * main--put[v1]<br/>
     * t1-----------beginTrans------------------------------------------put[v2]
     * ---commit<br/>
     * t2-------------------------beginTrans---put[v3]---abort<br/>
     * main---get[v1]
     */
    private void concurrencyWrites7() {
        try {
            System.out.println("Starting SsccTableClient5: concurrencyWrites7");
            System.out.println("main--put[v1]");
            System.out.println("t1-----------beginTrans------------------------------------------put[v2]---commit");
            System.out.println("t2-------------------------beginTrans---put[v3]---abort");
            System.out.println("main---get[v2]");
            SsccTableClientUtils.initialize();
            putValue();

            final P0Sscc61 t1 = new P0Sscc61();
            final P0Sscc62 t2 = new P0Sscc62();

            new Thread(new Runnable() {
                @Override
                public void run() {
                    t1.doWork();
                    System.out.println("  Sscc1 finished");
                    cdl7.countDown();
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
                    System.out.println("  Sscc2 finished");
                    cdl7.countDown();
                }
            }, "Sscc2").start();

            cdl7.await();

            SsccTableClientUtils.testSsccBeginTransaction();
            Integer[] result = SsccTableClientUtils.testSsccGet();
            SsccTableClientUtils.testSsccCommitIfPossible();
            System.out.println("Finish SsccTableClient5: concurrencyWrites7");

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

    static class P0Sscc61 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                System.out
                        .println("  Sscc1 had begun transaction and waits for Sscc2 to begin transaction & write v3 & commit");

                t2Condition.signal();
                t1Condition.await();
                SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);

                // commit
                SsccTableClientUtils.testSsccCommitIfPossible();

            } catch (Exception e) {
                System.out.println("=======Error in Sscc1: =======");
                e.printStackTrace();
                System.out.println("=======Error in Sscc1: =======");
            } finally {
                lock.unlock();
            }
        }
    }

    static class P0Sscc62 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
                // commit
                SsccTableClientUtils.testSsccAbortTransaction();
                System.out.println("  Sscc2 had begun transaction & wroten v3 & commit");

                t1Condition.signal();

            } catch (Exception e) {
                System.out.println("Error in Sscc2: ");
                e.printStackTrace();
            } finally {
                lock.unlock();
            }
        }
    }

    /**
     * main--put[v1]<br/>
     * t1-----------beginTrans-----------------------------------put[v2]---abort<br/>
     * t2-------------------------beginTrans---put[v3]---abort<br/>
     * main---get[v1]
     */
    private void concurrencyWrites8() {
        try {
            System.out.println("Starting SsccTableClient5: concurrencyWrites8");
            System.out.println("main--put[v1]");
            System.out.println("t1-----------beginTrans------------------------------------------put[v2]---abort");
            System.out.println("t2-------------------------beginTrans---put[v3]---abort");
            System.out.println("main---get[v1]");
            SsccTableClientUtils.initialize();
            putValue();

            final P0Sscc71 t1 = new P0Sscc71();
            final P0Sscc72 t2 = new P0Sscc72();

            new Thread(new Runnable() {
                @Override
                public void run() {
                    t1.doWork();
                    System.out.println("  Sscc1 finished");
                    cdl8.countDown();
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
                    System.out.println("  Sscc2 finished");
                    cdl8.countDown();
                }
            }, "Sscc2").start();

            cdl8.await();

            SsccTableClientUtils.testSsccBeginTransaction();
            Integer[] result = SsccTableClientUtils.testSsccGet();
            SsccTableClientUtils.testSsccCommitIfPossible();
            System.out.println("Finish SsccTableClient5: concurrencyWrites8");

            if (result.length == 1 && result[0] == 1) {
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

    static class P0Sscc71 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                System.out
                        .println("  Sscc1 had begun transaction and waits for Sscc2 to begin transaction & write v3 & commit");

                t2Condition.signal();
                t1Condition.await();
                SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);

                // commit
                SsccTableClientUtils.testSsccAbortTransaction();

            } catch (Exception e) {
                System.out.println("=======Error in Sscc1: =======");
                e.printStackTrace();
                System.out.println("=======Error in Sscc1: =======");
            } finally {
                lock.unlock();
            }
        }
    }

    static class P0Sscc72 {
        protected final Log log = LogFactory.getLog(getClass());

        public void doWork() {
            try {
                lock.lock();

                // start transaction
                SsccTableClientUtils.testSsccBeginTransaction();
                SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
                // commit
                SsccTableClientUtils.testSsccAbortTransaction();
                System.out.println("  Sscc2 had begun transaction & wroten v3 & commit");

                t1Condition.signal();

            } catch (Exception e) {
                System.out.println("=======Error in Sscc2: =======");
                e.printStackTrace();
                System.out.println("=======Error in Sscc2: =======");
            } finally {
                lock.unlock();
            }
        }
    }

}
