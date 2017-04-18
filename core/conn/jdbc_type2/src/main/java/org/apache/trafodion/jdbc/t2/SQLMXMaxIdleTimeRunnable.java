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

package org.apache.trafodion.jdbc.t2;

import java.sql.SQLException;
import java.util.Iterator;
import java.util.LinkedList;

import javax.sql.PooledConnection;

/*
 * This Class is the Runnable which will be run by the
 * Deamon thread which scavenges for Pooled Connections
 * whose maxIdleTime is reached.
 */
public class SQLMXMaxIdleTimeRunnable implements Runnable {

	/*
	 * sleepTime is the maxIdleTime
	 */
	private int sleepTime;
	/*
	 * List of SQLMXPooledConnectionManagers.
	 */
	private LinkedList<SQLMXPooledConnectionManager> listOfPCM;
	/*
	 * This is a Singleton Class hence a static variable of the
	 * class.
	 */
	private static SQLMXMaxIdleTimeRunnable maxIdleTimeRunnable = null;
	/*
	 * This List will be cleared after every scavenging run.
	 * This is to hold the removable PooledConnection from
	 * which will remove the scavenged PooledConnections from
	 * SQLMXPooledConnectionManager
	 */
	LinkedList<PooledConnection> listOfClosedPC = new LinkedList<PooledConnection>();

	private SQLMXMaxIdleTimeRunnable() {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_SQLMXMaxIdleTimeRunnable].methodEntry();
		try{
			sleepTime = 0;
			listOfPCM = new LinkedList<SQLMXPooledConnectionManager>();
		}finally{
			if (JdbcDebugCfg.entryActive)
				debug[methodId_SQLMXMaxIdleTimeRunnable].methodExit();
		}
	}

	public static SQLMXMaxIdleTimeRunnable getSQLMXMaxIdleTimeRunnable() {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getSQLMXMaxIdleTimeRunnable].methodEntry();
		try{
			if(maxIdleTimeRunnable == null) {
				maxIdleTimeRunnable = new SQLMXMaxIdleTimeRunnable();
			}
			return maxIdleTimeRunnable;
		}finally{
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getSQLMXMaxIdleTimeRunnable].methodExit();
		}
	}
	public void run() {
//		if (JdbcDebugCfg.entryActive)
//			debug[methodId_run].methodEntry();
//		try {
			if (sleepTime == 0) {
				return;
			}
			while (true) {
				/*
				 * Sleep for maxIdleTime
				 */
				try {
					Thread.sleep(sleepTime * 1000);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
				/*
				 * Get the current time.
				 */
				long currentTime = System.currentTimeMillis();
				/*
				 * Go over all the PooledConnectionManagers
				 */
				synchronized (listOfPCM) {

					Iterator<SQLMXPooledConnectionManager> iterOfPCM = listOfPCM
							.iterator();
					while (iterOfPCM.hasNext()) {
						SQLMXPooledConnectionManager pcm = iterOfPCM.next();
						/*
						 * Get the Free Connections list
						 */
						LinkedList<PooledConnection> listOfPC = pcm.getFree_();
						/*
						 * Lock the List of PooledConnections which are free.
						 */
						synchronized (listOfPC) {
							Iterator<PooledConnection> iterOfPC = listOfPC
									.iterator();
							/*
							 * Go over the List of PooledConnections.
							 */
							while (iterOfPC.hasNext()) {

								SQLMXPooledConnection pc = (SQLMXPooledConnection) iterOfPC
										.next();
								long lastUsedTime = pc.getLastUsedTime();
								/*
								 * The Time difference from this time and last
								 * used time of Connection should be positive.
								 */
								if ((currentTime - lastUsedTime) > 0) {
									/*
									 * If this time unit is greater than or
									 * equal to the maxIdleTime seconds
									 */
									if ((currentTime - lastUsedTime) * 1000 >= sleepTime) {
										/*
										 * Mark this connection
										 */
										listOfClosedPC.add(pc);
									}
								}
							}
							/*
							 * Remove the marked connections from the
							 * PoolManagers free pool, but maintain the
							 * minPoolSize.
							 */
							int minPoolSize = pcm.getMinPoolSize_();

							/*
							 * Have to maintain minPoolSize
							 */
							if (minPoolSize < listOfClosedPC.size()) {
								for (int nfor = 0; nfor < (listOfClosedPC
										.size() - minPoolSize); ++nfor) {
									try {
										/*
										 * Physical close of Connection.
										 */
										if (pcm.out_ != null)
										{
											if ((pcm.traceFlag_ == T2Driver.POOLING_LVL) ||
												(pcm.traceFlag_ == T2Driver.ENTRY_LVL))
												pcm.out_.println(pcm.getTraceId() + "MaxIdleTime elapsed, This Connection will be Hard closed");
										}
										listOfClosedPC.get(nfor).close();
										/*
										 * Remove this closed PooledConnection
										 * from the Pool of the
										 * PooledConnectionManager
										 */
										listOfPC.remove(listOfClosedPC
												.get(nfor));

									} catch (SQLException e) {
										// TODO Auto-generated catch block
										e.printStackTrace();
									}
								}
							}
							// Clear the temp list.
							listOfClosedPC.clear();
						}
					}
				}
			}
		 //finally {
//			if (JdbcDebugCfg.entryActive)
//				debug[methodId_getSQLMXMaxIdleTimeRunnable].methodExit();
//		}
	}
	/**
	 * @param sleepTime same as maxIdleTime
	 */
	public void setSleepTime(int sleepTime) {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setSleepTime].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setSleepTime].methodParameters("sleepTime=" + sleepTime);
		try{
			this.sleepTime = sleepTime;
		}finally{
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setSleepTime].methodExit();
		}
	}
	/**
	 * @return the sleepTime
	 */
	public int getSleepTime() {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getSleepTime].methodEntry();
		try{
			if (JdbcDebugCfg.traceActive)
				debug[methodId_getSleepTime].methodReturn("sleepTime="
						+ sleepTime);
			return sleepTime;
		}finally{
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getSleepTime].methodExit();
		}
	}

	/*
	 * This should be called from SQLMXPooledConnectionManager.
	 */
	public void addPoolManager(SQLMXPooledConnectionManager mgr) {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_addPoolManager].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_addPoolManager].methodParameters("mgr=" + JdbcDebug.debugObjectStr(mgr));
		try{
			listOfPCM.add(mgr);
		}finally{
			if (JdbcDebugCfg.entryActive)
				debug[methodId_addPoolManager].methodExit();
		}
	}

	public void removePoolManager(SQLMXPooledConnectionManager mgr) {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_removePoolManager].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_removePoolManager].methodParameters("mgr=" + JdbcDebug.debugObjectStr(mgr));
		try{
			listOfPCM.remove(mgr);
		}finally{
			if (JdbcDebugCfg.entryActive)
				debug[methodId_removePoolManager].methodExit();
		}

	}

	private static int methodId_getSQLMXMaxIdleTimeRunnable = 0;
	private static int methodId_setSleepTime = 1;
	private static int methodId_run = 2;
	private static int methodId_getSleepTime = 3;
	private static int methodId_addPoolManager = 4;
	private static int methodId_removePoolManager = 5;
	private static int methodId_SQLMXMaxIdleTimeRunnable = 6;
	private static int totalMethodIds = 7;
	private static JdbcDebug[] debug;

	static
	{
		String className = "SQLMXMaxIdleTimeRunnable";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_getSQLMXMaxIdleTimeRunnable] = new JdbcDebug(className,"getSQLMXMaxIdleTimeRunnable");
			debug[methodId_setSleepTime] = new JdbcDebug(className,"setSleepTime");
			debug[methodId_run] = new JdbcDebug(className,"run");
			debug[methodId_getSleepTime] = new JdbcDebug(className,"getSleepTime");
			debug[methodId_addPoolManager] = new JdbcDebug(className,"addPoolManager");
			debug[methodId_removePoolManager] = new JdbcDebug(className,"removePoolManager");
			debug[methodId_SQLMXMaxIdleTimeRunnable] = new JdbcDebug(className,"SQLMXMaxIdleTimeRunnable");
		}
	}
}
