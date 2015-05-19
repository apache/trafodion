package org.apache.hadoop.hbase.coprocessor.transactional;

import java.io.IOException;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class SsccTableClient10 {
	// ------ for concurrent ------
	private static CountDownLatch cdl = new CountDownLatch(2);
	private static Lock lock = new ReentrantLock();
	private static Condition t1Condition = lock.newCondition();
	private static Condition t2Condition = lock.newCondition();
	protected final Log log = LogFactory.getLog(getClass());

	private boolean first = false;
	private boolean second = false;
	private static int successNum = 0;

	/**
	 * concurrency writes
	 * 
	 * @param args
	 * @throws Exception
	 */
	static public void main(String[] args) throws Exception {
		SsccTableClient10 cilent = new SsccTableClient10();

		cilent.concurrencyWrites1();

		cilent.log.info("=========================================");
		cilent.log.info(" ");
		cilent.log.info("TOTAL : 1 . SUCCESS : " + successNum + " FAILURE : " + (1 - successNum));
		cilent.log.info(" ");
		cilent.log.info("=========================================");

	}

	/**
	 * main--put[v1]<br/>
	 * t1-----------begin---get[v1]--------------------------------------------
	 * -------------------------------------------------------------get[v1]<br/>
	 * t2------------------------------begin---put[v2]-commit---begin---put[v3]
	 * ---commit---begin---put[v4]---commit¡ªget[v4]<br/>
	 * main---get[v4]
	 */
	private void concurrencyWrites1() {
		try {
			log.info("Starting TrxTableClient10: concurrencyWrites1");
			log.info("main--put[v1]");
			log.info("t1-----------begin---get[v1]---------------------------------------------------------------------------------------------------------get[v1]");
			log.info("t2------------------------------begin---put[v2]-commit---begin---put[v3]---commit---begin---put[v4]---commit¡ªget[v4]");
			log.info("main--get[v4]");
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
					second = t2.doWork();
					log.info("Trx2 finished");
					cdl.countDown();
				}
			}, "Trx2").start();

			cdl.await();

			SsccTableClientUtils.testSsccBeginTransaction();
			Integer[] result = SsccTableClientUtils.testSsccGet();
			SsccTableClientUtils.testSsccCommitIfPossible();
			log.info("Finish TrxTableClient10: concurrencyWrites1");

			if (result.length == 1 && result[0] == 4 && first == true && second == true) {
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
				log.info("Trx1 had begun transaction & read v1 and waits for Trx2 to do multiple write.");

				t2Condition.signal();
				t1Condition.await();

				// log.info("================");Thread.sleep(5000);

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

		public boolean doWork() {
			boolean second = false;
			try {
				lock.lock();

				// put v2
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE2);
				SsccTableClientUtils.testSsccCommitIfPossible();
				// put v3
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE3);
				SsccTableClientUtils.testSsccCommitIfPossible();
				// put v4
				SsccTableClientUtils.testSsccBeginTransaction();
				SsccTableClientUtils.testSsccPut(SsccTableClientUtils.VALUE4);
				SsccTableClientUtils.testSsccCommitIfPossible();
				// get v4
				SsccTableClientUtils.testSsccBeginTransaction();
				Integer[] r1 = SsccTableClientUtils.testSsccGet();
				if (r1.length == 1 && r1[0] == 4) {
					second = true;
				}
				SsccTableClientUtils.testSsccCommitIfPossible();
				log.info("Trx2 had put multiple value & commited and waits for Trx1 to commit");

			} catch (Exception e) {
				log.info("Error in Trx2: ");
				e.printStackTrace();
			} finally {
				t1Condition.signal();
				lock.unlock();
			}
			return second;
		}
	}
}