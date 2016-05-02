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

package org.trafodion.jdbc.t4;

import java.util.Hashtable;

/*
 Swastik: On 2/24/2005 - this class was changed to NOT inherit from the Thread
 class. The reason is that nothing was dependent on the exact time at
 which the InputOutput object timed out (for which it was used).
 Timeout is always polled from this class. As a result - the timeout is
 is calculated based on current system time.

 Later if some event needs to be triggered at the exact time this Timer
 timed out - then all the commented fields with the comment of 2/24/05
 can be uncommented.
 */
class T4TimerThread
// extends Thread
{
	// Stores a timer thread using the dialogue id as the key
	private static Hashtable threadTable;
	// Stores the idle time out value
	private long sleepTime_;
	// Stores when the absolute time at which timeout will/has to occur
	private long timeoutTime_;

	// Track if the thread timed out
	// private boolean m_timedOut; // SB:2/24/05

	T4TimerThread(int dialogueId, long time) {
		if (time < 0) {
			sleepTime_ = 0;
		} else {
			sleepTime_ = time;
			// Calcluate the absolute time when timeout should occur
		}
		timeoutTime_ = System.currentTimeMillis() + sleepTime_;
		// m_timedOut = false; // SB:2/24/05
		if (threadTable == null) {
			threadTable = new Hashtable();
		}
		threadTable.put(new Integer(dialogueId), this);
	}

	public void reset(long slpTime) {
		// Reset the absolute time when timeout should occur
		if (slpTime > 0) {
			timeoutTime_ = slpTime + System.currentTimeMillis();
		}
	}

	/*
	 * // SB:2/24/05 public void run() { try { while (System.currentTimeMillis() <
	 * timeoutTime_) { //m_timedOut = false; long timeToSleep = timeoutTime_ -
	 * System.currentTimeMillis(); if (timeToSleep > 0) { sleep(timeToSleep); }
	 * else { //m_timedOut = true; //sleep(sleepTime_); } //m_timedOut = true; } }
	 * catch (InterruptedException ie) { return; } }
	 */

	boolean getTimedOut() {
		// return m_timedOut; // SB:2/24/05
		return timeoutTime_ <= System.currentTimeMillis();
	}

	static T4TimerThread getThread(int dialogueId) {
		if (dialogueId <= 0) {
			return null;
		}
		if (threadTable == null) {
			threadTable = new Hashtable();
		}
		Object o = threadTable.get(new Integer(dialogueId));
		if (o == null) {
			return null;
		} else {
			return (T4TimerThread) o;
		}
	}

	static void removeThread(int dialogueId) {
		if (dialogueId <= 0) {
			return;
		}
		if (threadTable == null) {
			return;
		}
		// First check if the thread is active
		Integer key = new Integer(dialogueId);
		/*
		 * // SB:2/24/05 Object o = threadTable.get(key); if (o != null) {
		 * T4TimerThread t = (T4TimerThread) o; if (t.isAlive()) {
		 * t.interrupt(); } }
		 */
		threadTable.remove(key);
	}
}
