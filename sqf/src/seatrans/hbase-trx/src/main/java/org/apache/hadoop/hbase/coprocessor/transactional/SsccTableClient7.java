package org.apache.hadoop.hbase.coprocessor.transactional;

import java.io.IOException;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class SsccTableClient7 {
	// ------ for concurrent ------
	private static CountDownLatch cdl = new CountDownLatch(2);
	private static CountDownLatch cdl2 = new CountDownLatch(2);
	private static Lock lock = new ReentrantLock();
	private static Condition t1Condition = lock.newCondition();
	private static Condition t2Condition = lock.newCondition();
	protected final Log log = LogFactory.getLog(getClass());

	private boolean first = false;
	private static int successNum = 0;

	/**
	 * concurrency writes
	 * 
	 * @param args
	 * @throws Exception
	 */
	static public void main(String[] args) throws Exception {
		SsccTableClient7 cilent = new SsccTableClient7();

		cilent.concurrencyWrites1();
		cilent.concurrencyWrites2();

		cilent.log.info("=========================================");
		cilent.log.info(" ");
		cilent.log.info("TOTAL : 2 . SUCCESS : " + successNum + " FAILURE : " + (2 - successNum));
		cilent.log.info(" ");
		cilent.log.info("=========================================");

	}

	/**
	 * main--put[v1]<br/>
	 * t1-----------beginTrans---get[v1]-------------------------------------get
	 * [v1]---commit<br/>
	 * t2------------------------------------beginTrans---put[v2]---commit<br/>
	 * main---get[v2]
	 */
	private void concurrencyWrites1() {
		try {
			log.info("Starting TrxTableClient7: concurrencyWrites1");
			log.info("main--put[v1]");
			log.info("t1-----------beginTrans---get[v1]-------------------------------------get[v1]---commit");
			log.info("t2------------------------------------beginTrans---put[v2]---commit");
			log.info("main---get[v2]");
			SsccTableClientUtils.initialize();
			putValue();

			final P2Trx01 t1 = new P2Trx01();
			final P2Trx02 t2 = new P2Trx02();

			new Thread(new Runnable() {
				@Override
				public void run() {
					first = t1.doWork();
					log.info("Trx1 finished");
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
					log.info("Trx2 finished");
					cdl.countDown();
				}
			}, "Trx2").start();

			cdl.await();

			SsccTableClientUtils.testSsccBeginTransaction();
			Integer[] result = SsccTableClientUtils.testSsccGet();
			SsccTableClientUtils.testSsccCommitIfPossible();
			log.info("Finish TrxTableClient5: concurrencyWrites1");

			if (result.length == 1 && result[0] == 2 && first == true) {
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

	static class P2Trx01 {
		protected final Log log = LogFactory.getLog(getClass());

		public boolean doWork() {
			boolean first = false;
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				Integer[] r1 = SsccTableClientUtils.testSsccGet();
				if (r1.length == 1 && r1[0] == 1) {
					first = true;
				}
				log.info("Trx1 had begun transaction & read v1 and waits for Trx2 to begin transaction & write v2");

				t2Condition.signal();
				t1Condition.await();
				Integer[] r2 = SsccTableClientUtils.testSsccGet();
				if (r2.length != 1 || r2[0] != 1 || first != true) {
					first = false;
				}
				// commit
				SsccTableClientUtils.testSsccCommitIfPossible();

			} catch (Exception e) {
				log.info("Error in Trx1: ");
				e.printStackTrace();
			} finally {
				lock.unlock();
			}

			return first;
		}
	}

	static class P2Trx02 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);
				// commit
				SsccTableClientUtils.testSsccCommitIfPossible();
				log.info("Trx2 had begun transaction & wroten v2 & commited and waits for Trx1 to commit");

			} catch (Exception e) {
				log.info("Error in Trx2: ");
				e.printStackTrace();
			} finally {
				t1Condition.signal();
				lock.unlock();
			}
		}
	}

	/**
	 * main--put[v1]<br/>
	 * t1-----------beginTrans---get[v1]-------------------------------------get
	 * [v1]---commit<br/>
	 * t2------------------------------------beginTrans---put[v2]---abort<br/>
	 * main---get[v1]
	 */
	private void concurrencyWrites2() {
		try {
			log.info("Starting TrxTableClient7: concurrencyWrites2");
			log.info("main--put[v1]");
			log.info("t1-----------beginTrans---get[v1]-------------------------------------get[v1]---commit");
			log.info("t2------------------------------------beginTrans---put[v2]---abort");
			log.info("main---get[v1]");
			SsccTableClientUtils.initialize();
			putValue();

			final P2Trx11 t1 = new P2Trx11();
			final P2Trx12 t2 = new P2Trx12();

			new Thread(new Runnable() {
				@Override
				public void run() {
					first = t1.doWork();
					log.info("Trx1 finished");
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
					log.info("Trx2 finished");
					cdl2.countDown();
				}
			}, "Trx2").start();

			cdl2.await();

			SsccTableClientUtils.testSsccBeginTransaction();
			Integer[] result = SsccTableClientUtils.testSsccGet();
			SsccTableClientUtils.testSsccCommitIfPossible();
			log.info("Finish TrxTableClient5: concurrencyWrites1");

			if (result.length == 1 && result[0] == 1 && first == true) {
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

	static class P2Trx11 {
		protected final Log log = LogFactory.getLog(getClass());

		public boolean doWork() {
			boolean first = false;
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				Integer[] r1 = SsccTableClientUtils.testSsccGet();
				if (r1.length == 1 && r1[0] == 1) {
					first = true;
				}
				log.info("Trx1 had begun transaction & read v1 and waits for Trx2 to begin transaction & write v2");

				t2Condition.signal();
				t1Condition.await();

				Integer[] r2 = SsccTableClientUtils.testSsccGet();
				if (r2.length != 1 || r2[0] != 1 || first != true) {
					first = false;
				}
				// commit
				SsccTableClientUtils.testSsccCommitIfPossible();

			} catch (Exception e) {
				log.info("Error in Trx1: ");
				e.printStackTrace();
			} finally {
				lock.unlock();
			}
			return first;
		}
	}

	static class P2Trx12 {
		protected final Log log = LogFactory.getLog(getClass());

		public void doWork() {
			try {
				lock.lock();

				// start transaction
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);
				// commit
				SsccTableClientUtils.testSsccAbortTransaction();
				log.info("Trx2 had begun transaction & wroten v2 & abort and waits for Trx1 read v1 & commit");

			} catch (Exception e) {
				log.info("Error in Trx2: ");
				e.printStackTrace();
			} finally {
				t1Condition.signal();
				lock.unlock();
			}
		}
	}

}