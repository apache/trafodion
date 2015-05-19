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

		cilent.log.info("=========================================");
		cilent.log.info(" ");
		cilent.log.info("TOTAL : 8 . SUCCESS : " + successNum + " FAILURE : " + (8 - successNum));
		cilent.log.info(" ");
		cilent.log.info("=========================================");
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
			log.info("Starting TrxTableClient5: concurrencyWrites1");
			log.info("main--put[v1]");
			log.info("t1-----------beginTrans---put[v2]---------------------------commit");
			log.info("t2-------------------------------------beginTrans---put[v3]----------commit");
			log.info("main---get[v2]");

			SsccTableClientUtils.initialize();
			putValue();

			final P0Trx01 t1 = new P0Trx01();
			final P0Trx02 t2 = new P0Trx02();

			new Thread(new Runnable() {
				@Override
				public void run() {
					t1.doWork();
					log.info("  Trx1 finished");
					cdl.countDown();
				}
			}, "Trx1").start();

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
					log.info("  Trx2 finished");
					cdl.countDown();
				}
			}, "Trx2").start();

			cdl.await();
			SsccTableClientUtils.testSsccBeginTransaction();
			Integer[] result = SsccTableClientUtils.testSsccGet();
			SsccTableClientUtils.testSsccCommitIfPossible();
			log.info("Finish TrxTableClient5: concurrencyWrites1");

			if (result.length == 1 && result[0] == 2) {
				successNum++;
				log.info("=========================================");
				log.info(" ");
				log.info("SUCCESS");
				log.info(" ");
				log.info("=========================================");
			} else {
				log.info("=========================================");
				log.info(" ");
				log.info("FAILURE");
				log.info(" ");
				log.info("=========================================");
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

	static class P0Trx01 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);

				log.info("Trx1 had begun transaction & wroten v2 and waits for Trx2 to begin transaction & write v3");

				t2Condition.signal();
				t1Condition.await();

				// commit
				SsccTableClientUtils.testSsccCommitIfPossible();

			} catch (Exception e) {
				log.info("=======Error in Trx1: =======");
				e.printStackTrace();
				log.info("=======Error in Trx1: =======");
			} finally {
				t2Condition.signal();
				lock.unlock();
			}
		}
	}

	static class P0Trx02 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
				log.info("  Trx2 had begun transaction & wroten v3 and waits for Trx1 to commit");

				t1Condition.signal();
				t2Condition.await();

				// commit
				SsccTableClientUtils.testSsccCommitIfPossible();
			} catch (Exception e) {
				log.info("=======Error in Trx2: =======");
				e.printStackTrace();
				log.info("=======Error in Trx2: =======");
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
			log.info("Starting TrxTableClient5: concurrencyWrites2");
			log.info("main--put[v1]");
			log.info("t1-----------beginTrans---put[v2]---------------------------commit");
			log.info("t2-------------------------------------beginTrans---put[v3]----------abort");
			log.info("main---get[v2]");
			SsccTableClientUtils.initialize();
			putValue();

			final P0Trx11 t1 = new P0Trx11();
			final P0Trx12 t2 = new P0Trx12();

			new Thread(new Runnable() {
				@Override
				public void run() {
					t1.doWork();
					log.info("  Trx1 finished");
					cdl2.countDown();
				}
			}, "Trx1").start();

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
					log.info("  Trx2 finished");
					cdl2.countDown();
				}
			}, "Trx2").start();

			cdl2.await();

			SsccTableClientUtils.testSsccBeginTransaction();
			Integer[] result = SsccTableClientUtils.testSsccGet();
			SsccTableClientUtils.testSsccCommitIfPossible();
			log.info("Finish TrxTableClient5: concurrencyWrites2");

			if (result.length == 1 && result[0] == 2) {
				successNum++;
				log.info("=========================================");
				log.info(" ");
				log.info("SUCCESS");
				log.info(" ");
				log.info("=========================================");
			} else {
				log.info("=========================================");
				log.info(" ");
				log.info("FAILURE");
				log.info(" ");
				log.info("=========================================");
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	static class P0Trx11 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);

				log.info("  Trx1 had begun transaction & wroten v2 and waits for Trx2 to begin transaction & write v3");

				t2Condition.signal();
				t1Condition.await();

				// commit
				SsccTableClientUtils.testSsccCommitIfPossible();

			} catch (Exception e) {
				log.info("=======Error in Trx1: =======");
				e.printStackTrace();
				log.info("=======Error in Trx1: =======");
			} finally {
				t2Condition.signal();
				lock.unlock();
			}
		}
	}

	static class P0Trx12 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
				log.info("  Trx2 had begun transaction & wroten v3 and waits for Trx1 to commit");

				t1Condition.signal();
				t2Condition.await();

				// commit
				SsccTableClientUtils.testSsccAbortTransaction();
			} catch (Exception e) {
				log.info("=======Error in Trx2: =======");
				e.printStackTrace();
				log.info("=======Error in Trx2: =======");
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
			log.info("Starting TrxTableClient5: concurrencyWrites3");
			log.info("main--put[v1]");
			log.info("t1-----------beginTrans---put[v2]---------------------------abort");
			log.info("t2-------------------------------------beginTrans---put[v3]----------commit");
			log.info("main---get[v1]");
			SsccTableClientUtils.initialize();
			putValue();

			final P0Trx21 t1 = new P0Trx21();
			final P0Trx22 t2 = new P0Trx22();

			new Thread(new Runnable() {
				@Override
				public void run() {
					t1.doWork();
					log.info("  Trx1 finished");
					cdl3.countDown();
				}
			}, "Trx1").start();

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
					log.info("  Trx2 finished");
					cdl3.countDown();
				}
			}, "Trx2").start();

			cdl3.await();

			SsccTableClientUtils.testSsccBeginTransaction();
			Integer[] result = SsccTableClientUtils.testSsccGet();
			SsccTableClientUtils.testSsccCommitIfPossible();
			log.info("Finish TrxTableClient5: concurrencyWrites3");

			if (result.length == 1 && result[0] == 1) {
				successNum++;
				log.info("=========================================");
				log.info(" ");
				log.info("SUCCESS");
				log.info(" ");
				log.info("=========================================");
			} else {
				log.info("=========================================");
				log.info(" ");
				log.info("FAILURE");
				log.info(" ");
				log.info("=========================================");
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	static class P0Trx21 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);

				log.info("  Trx1 had begun transaction & wroten v2 and waits for Trx2 to begin transaction & write v3");

				t2Condition.signal();
				t1Condition.await();

				// commit
				SsccTableClientUtils.testSsccAbortTransaction();

			} catch (Exception e) {
				log.info("=======Error in Trx1: =======");
				e.printStackTrace();
				log.info("=======Error in Trx1: =======");
			} finally {
				t2Condition.signal();
				lock.unlock();
			}
		}
	}

	static class P0Trx22 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
				log.info("  Trx2 had begun transaction & wroten v3 and waits for Trx1 to commit");

				t1Condition.signal();
				t2Condition.await();

				// commit
				SsccTableClientUtils.testSsccCommitIfPossible();
			} catch (Exception e) {
				log.info("=======Error in Trx2: =======");
				e.printStackTrace();
				log.info("=======Error in Trx2: =======");
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
			log.info("Starting TrxTableClient5: concurrencyWrites4");
			log.info("main--put[v1]");
			log.info("t1-----------beginTrans---put[v2]---------------------------abort");
			log.info("t2-------------------------------------beginTrans---put[v3]----------abort");
			log.info("main---get[v1]");
			SsccTableClientUtils.initialize();
			putValue();

			final P0Trx31 t1 = new P0Trx31();
			final P0Trx32 t2 = new P0Trx32();

			new Thread(new Runnable() {
				@Override
				public void run() {
					t1.doWork();
					log.info("  Trx1 finished");
					cdl4.countDown();
				}
			}, "Trx1").start();

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
					log.info("  Trx2 finished");
					cdl4.countDown();
				}
			}, "Trx2").start();

			cdl4.await();

			SsccTableClientUtils.testSsccBeginTransaction();
			Integer[] result = SsccTableClientUtils.testSsccGet();
			SsccTableClientUtils.testSsccCommitIfPossible();
			log.info("Finish TrxTableClient5: concurrencyWrites4");

			if (result.length == 1 && result[0] == 1) {
				successNum++;
				log.info("=========================================");
				log.info(" ");
				log.info("SUCCESS");
				log.info(" ");
				log.info("=========================================");
			} else {
				log.info("=========================================");
				log.info(" ");
				log.info("FAILURE");
				log.info(" ");
				log.info("=========================================");
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	static class P0Trx31 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);

				log.info("  Trx1 had begun transaction & wroten v2 and waits for Trx2 to begin transaction & write v3");

				t2Condition.signal();
				t1Condition.await();

				// commit
				SsccTableClientUtils.testSsccAbortTransaction();

			} catch (Exception e) {
				log.info("=======Error in Trx1: =======");
				e.printStackTrace();
				log.info("=======Error in Trx1: =======");
			} finally {
				t2Condition.signal();
				lock.unlock();
			}
		}
	}

	static class P0Trx32 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
				log.info("  Trx2 had begun transaction & wroten v3 and waits for Trx1 to commit");

				t1Condition.signal();
				t2Condition.await();

				// commit
				SsccTableClientUtils.testSsccAbortTransaction();
			} catch (Exception e) {
				log.info("=======Error in Trx2: =======");
				e.printStackTrace();
				log.info("=======Error in Trx2: =======");
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
			log.info("Starting TrxTableClient5: concurrencyWrites5");
			log.info("main--put[v1]");
			log.info("t1-----------beginTrans------------------------------------------put[v2]---commit");
			log.info("t2-------------------------beginTrans---put[v3]---commit");
			log.info("main---get[v3]");

			SsccTableClientUtils.initialize();
			putValue();

			final P0Trx41 t1 = new P0Trx41();
			final P0Trx42 t2 = new P0Trx42();

			new Thread(new Runnable() {
				@Override
				public void run() {
					t1.doWork();
					log.info("  Trx1 finished");
					cdl5.countDown();
				}
			}, "Trx1").start();

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
					log.info("  Trx2 finished");
					cdl5.countDown();
				}
			}, "Trx2").start();

			cdl5.await();

			SsccTableClientUtils.testSsccBeginTransaction();
			Integer[] result = SsccTableClientUtils.testSsccGet();
			SsccTableClientUtils.testSsccCommitIfPossible();
			log.info("Finish TrxTableClient5: concurrencyWrites5");

			if (result.length == 1 && result[0] == 3) {
				successNum++;
				log.info("=========================================");
				log.info(" ");
				log.info("SUCCESS");
				log.info(" ");
				log.info("=========================================");
			} else {
				log.info("=========================================");
				log.info(" ");
				log.info("FAILURE");
				log.info(" ");
				log.info("=========================================");
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	static class P0Trx41 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				log.info("  Trx1 had begun transaction and waits for Trx2 to begin transaction & write v3 & commit");

				t2Condition.signal();
				t1Condition.await();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);

				// commit
				SsccTableClientUtils.testSsccCommitIfPossible();

			} catch (Exception e) {
				log.info("=======Error in Trx1: =======");
				e.printStackTrace();
				log.info("=======Error in Trx1: =======");
			} finally {
				lock.unlock();
			}
		}
	}

	static class P0Trx42 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
				// commit
				SsccTableClientUtils.testSsccCommitIfPossible();
				log.info("  Trx2 had begun transaction & wroten v3 & commit");

				t1Condition.signal();

			} catch (Exception e) {
				log.info("=======Error in Trx2: =======");
				e.printStackTrace();
				log.info("=======Error in Trx2: =======");
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
			log.info("Starting TrxTableClient5: concurrencyWrites6");
			log.info("main--put[v1]");
			log.info("t1-----------beginTrans------------------------------------------put[v2]---abort");
			log.info("t2-------------------------beginTrans---put[v3]---commit");
			log.info("main---get[v3]");

			SsccTableClientUtils.initialize();
			putValue();

			final P0Trx51 t1 = new P0Trx51();
			final P0Trx52 t2 = new P0Trx52();

			new Thread(new Runnable() {
				@Override
				public void run() {
					t1.doWork();
					log.info("  Trx1 finished");
					cdl6.countDown();
				}
			}, "Trx1").start();

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
					log.info("  Trx2 finished");
					cdl6.countDown();
				}
			}, "Trx2").start();

			cdl6.await();

			SsccTableClientUtils.testSsccBeginTransaction();
			Integer[] result = SsccTableClientUtils.testSsccGet();
			SsccTableClientUtils.testSsccCommitIfPossible();
			log.info("Finish TrxTableClient5: concurrencyWrites6");

			if (result.length == 1 && result[0] == 3) {
				successNum++;
				log.info("=========================================");
				log.info(" ");
				log.info("SUCCESS");
				log.info(" ");
				log.info("=========================================");
			} else {
				log.info("=========================================");
				log.info(" ");
				log.info("FAILURE");
				log.info(" ");
				log.info("=========================================");
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	static class P0Trx51 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				log.info("  Trx1 had begun transaction and waits for Trx2 to begin transaction & write v3 & commit");

				t2Condition.signal();
				t1Condition.await();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);

				// commit
				SsccTableClientUtils.testSsccAbortTransaction();

			} catch (Exception e) {
				log.info("=======Error in Trx1: =======");
				e.printStackTrace();
				log.info("=======Error in Trx1: =======");
			} finally {
				lock.unlock();
			}
		}
	}

	static class P0Trx52 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
				// commit
				SsccTableClientUtils.testSsccCommitIfPossible();
				log.info("  Trx2 had begun transaction & wroten v3 & commit");

				t1Condition.signal();

			} catch (Exception e) {
				log.info("=======Error in Trx2: =======");
				e.printStackTrace();
				log.info("=======Error in Trx2: =======");
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
			log.info("Starting TrxTableClient5: concurrencyWrites7");
			log.info("main--put[v1]");
			log.info("t1-----------beginTrans------------------------------------------put[v2]---commit");
			log.info("t2-------------------------beginTrans---put[v3]---abort");
			log.info("main---get[v2]");
			SsccTableClientUtils.initialize();
			putValue();

			final P0Trx61 t1 = new P0Trx61();
			final P0Trx62 t2 = new P0Trx62();

			new Thread(new Runnable() {
				@Override
				public void run() {
					t1.doWork();
					log.info("  Trx1 finished");
					cdl7.countDown();
				}
			}, "Trx1").start();

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
					log.info("  Trx2 finished");
					cdl7.countDown();
				}
			}, "Trx2").start();

			cdl7.await();

			SsccTableClientUtils.testSsccBeginTransaction();
			Integer[] result = SsccTableClientUtils.testSsccGet();
			SsccTableClientUtils.testSsccCommitIfPossible();
			log.info("Finish TrxTableClient5: concurrencyWrites7");

			if (result.length == 1 && result[0] == 2) {
				successNum++;
				log.info("=========================================");
				log.info(" ");
				log.info("SUCCESS");
				log.info(" ");
				log.info("=========================================");
			} else {
				log.info("=========================================");
				log.info(" ");
				log.info("FAILURE");
				log.info(" ");
				log.info("=========================================");
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	static class P0Trx61 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				log.info("  Trx1 had begun transaction and waits for Trx2 to begin transaction & write v3 & commit");

				t2Condition.signal();
				t1Condition.await();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);

				// commit
				SsccTableClientUtils.testSsccCommitIfPossible();

			} catch (Exception e) {
				log.info("=======Error in Trx1: =======");
				e.printStackTrace();
				log.info("=======Error in Trx1: =======");
			} finally {
				lock.unlock();
			}
		}
	}

	static class P0Trx62 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
				// commit
				SsccTableClientUtils.testSsccAbortTransaction();
				log.info("  Trx2 had begun transaction & wroten v3 & commit");

				t1Condition.signal();

			} catch (Exception e) {
				log.info("Error in Trx2: ");
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
			log.info("Starting TrxTableClient5: concurrencyWrites8");
			log.info("main--put[v1]");
			log.info("t1-----------beginTrans------------------------------------------put[v2]---abort");
			log.info("t2-------------------------beginTrans---put[v3]---abort");
			log.info("main---get[v1]");
			SsccTableClientUtils.initialize();
			putValue();

			final P0Trx71 t1 = new P0Trx71();
			final P0Trx72 t2 = new P0Trx72();

			new Thread(new Runnable() {
				@Override
				public void run() {
					t1.doWork();
					log.info("  Trx1 finished");
					cdl8.countDown();
				}
			}, "Trx1").start();

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
					log.info("  Trx2 finished");
					cdl8.countDown();
				}
			}, "Trx2").start();

			cdl8.await();

			SsccTableClientUtils.testSsccBeginTransaction();
			Integer[] result = SsccTableClientUtils.testSsccGet();
			SsccTableClientUtils.testSsccCommitIfPossible();
			log.info("Finish TrxTableClient5: concurrencyWrites8");

			if (result.length == 1 && result[0] == 1) {
				successNum++;
				log.info("=========================================");
				log.info(" ");
				log.info("SUCCESS");
				log.info(" ");
				log.info("=========================================");
			} else {
				log.info("=========================================");
				log.info(" ");
				log.info("FAILURE");
				log.info(" ");
				log.info("=========================================");
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	static class P0Trx71 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				log.info("  Trx1 had begun transaction and waits for Trx2 to begin transaction & write v3 & commit");

				t2Condition.signal();
				t1Condition.await();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);

				// commit
				SsccTableClientUtils.testSsccAbortTransaction();

			} catch (Exception e) {
				log.info("=======Error in Trx1: =======");
				e.printStackTrace();
				log.info("=======Error in Trx1: =======");
			} finally {
				lock.unlock();
			}
		}
	}

	static class P0Trx72 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
				// commit
				SsccTableClientUtils.testSsccAbortTransaction();
				log.info("  Trx2 had begun transaction & wroten v3 & commit");

				t1Condition.signal();

			} catch (Exception e) {
				log.info("=======Error in Trx2: =======");
				e.printStackTrace();
				log.info("=======Error in Trx2: =======");
			} finally {
				lock.unlock();
			}
		}
	}

}
