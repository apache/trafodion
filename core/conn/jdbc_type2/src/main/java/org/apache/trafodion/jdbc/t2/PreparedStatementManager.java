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

/*
 * Filename	: PreparedStatementManager.java
 */

 /*
 * Method Changed : addPreparedStatement & getPreparedStatement
 */
package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import javax.sql.*;

import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Properties;
import java.util.Iterator;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Date;

class PreparedStatementManager extends SQLMXHandle {

	boolean isStatementCachingEnabled() {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_isStatementCachingEnabled].methodEntry();
		try {
			if (maxStatements_ < 1)
				return false;
			else
				return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_isStatementCachingEnabled].methodExit();
		}
	}

	boolean makeRoom() throws SQLException {

		if (JdbcDebugCfg.entryActive)
			debug[methodId_makeRoom].methodEntry();
		try {
			if (out_ != null) {
				if ((traceFlag_ == T2Driver.POOLING_LVL)
						|| (traceFlag_ == T2Driver.ENTRY_LVL))
					out_.println(getTraceId() + "makeRoom() - maxStatements = "
							+ maxStatements_);
			}
			Iterator i;
			SQLMXPreparedStatement pStmt;
			CachedPreparedStatement cs;
			long oldest = 0;
			long stmtTime;
			String key = null;

			i = (prepStmtsInCache_.values()).iterator();
			List<CachedPreparedStatement> listOfNotInUse = new ArrayList<CachedPreparedStatement>();

			// Get the all cached statement that is not in use
			for (; i.hasNext();) {
				cs = (CachedPreparedStatement) i.next();
				if (!cs.inUse_) {
					oldest = cs.getLastUsedTime();
					key = cs.getLookUpKey();
					if (out_ != null) {
						if ((traceFlag_ == T2Driver.POOLING_LVL)
								|| (traceFlag_ == T2Driver.ENTRY_LVL))
							out_.println(getTraceId() + "makeroom() "
									+ "Found unused cached statement - " + "\""
									+ key + "\"");
					}
					listOfNotInUse.add(cs);
				}
			}

			// Try to remove the oldest/unused statement only if found one above
			if (!listOfNotInUse.isEmpty()) {
				Collections.sort(listOfNotInUse);
				CachedPreparedStatement csForRemoval = listOfNotInUse.get(0);
				if ((traceFlag_ == T2Driver.POOLING_LVL)
						|| (traceFlag_ == T2Driver.ENTRY_LVL)) {
					out_.println(getTraceId() + "makeroom() "
							+ "Found older unused cached statement - " + "\""
							+ csForRemoval.getLookUpKey() + "\"");
				}
				prepStmtsInCache_.remove(csForRemoval.getLookUpKey());
				((SQLMXPreparedStatement) csForRemoval.getPreparedStatement())
						.close(true);
				count_--;
				if ((traceFlag_ == T2Driver.POOLING_LVL)
						|| (traceFlag_ == T2Driver.ENTRY_LVL)) {
					out_.println(getTraceId() + "makeroom() "
							+ "Removed cached stmt - " + "\""
							+ csForRemoval.getLookUpKey() + "\"");
				}
				listOfNotInUse.clear();
				return true;
			}
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_makeRoom].methodExit();
		}
	}

	void closePreparedStatementsAll() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_closePreparedStatementsAll].methodEntry();
		try {
			if (out_ != null) {
				if ((traceFlag_ == T2Driver.POOLING_LVL)
						|| (traceFlag_ == T2Driver.ENTRY_LVL))
					out_.println(getTraceId() + "closePreparedStatementsAll()");
			}

			Object[] csArray;

			CachedPreparedStatement cs;
			int i = 0;

			csArray = (prepStmtsInCache_.values()).toArray();
			for (i = 0; i < csArray.length; i++) {
				cs = (CachedPreparedStatement) csArray[i];					
				if (cs != null){
						cs.close(false);
				}
			}
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_closePreparedStatementsAll].methodExit();
		}
	}

	/*
	 * Description : Identify and close the duplicate statement. Additional
	 * argument origPStmt introduced.
	 */
	// void closePreparedStatement(SQLMXConnection connect,
	void closePreparedStatement(SQLMXPreparedStatement origPStmt,
			SQLMXConnection connect, String sql, int resultSetType,
			int resultSetConcurrency, int resultSetHoldability)
			throws SQLException {

		if (JdbcDebugCfg.entryActive)
			debug[methodId_closePreparedStatement].methodEntry();
		try {
			if (out_ != null) {
				if ((traceFlag_ == T2Driver.POOLING_LVL)
						|| (traceFlag_ == T2Driver.ENTRY_LVL))
					out_.println(getTraceId() + "closePreparedStatement(" + connect
							+ ",\"" + sql + "\"," + resultSetType + ","
							+ resultSetConcurrency + "," + resultSetHoldability
							+ ")");
			}
			SQLMXPreparedStatement pStmt = null;
			CachedPreparedStatement cs = null;
			;
			String lookupKey;

			Object csArray[];
//			lookupKey = sql;
//			if (connect.catalog_ != null)
//				lookupKey = lookupKey.concat(connect.catalog_);
//			if (connect.schema_ != null)
//				lookupKey = lookupKey.concat(connect.schema_);			
//			lookupKey = lookupKey.concat(String.valueOf(connect.transactionIsolation_))
//					.concat(String.valueOf(resultSetHoldability));
//			if (connect.getMD5HashCode() != null) 
//				lookupKey = lookupKey.concat(connect.getMD5HashCode());
//			
			
//			lookupKey = sql;
//			if (connect.catalog_ != null)
//				lookupKey.concat(connect.catalog_);
//			if (connect.schema_ != null)
//				lookupKey.concat(connect.schema_);
//			lookupKey.concat(String.valueOf(connect.transactionIsolation_))
//					.concat(String.valueOf(resultSetHoldability));
			
			
//			cs = (CachedPreparedStatement) prepStmtsInCache_.get(lookupKey);
			
			csArray = (prepStmtsInCache_.values()).toArray();
			boolean foundFlag = false;
			
			for(int i=0;i<csArray.length;i++){
				cs = (CachedPreparedStatement)csArray[i];

				if(cs != null){
					pStmt = (SQLMXPreparedStatement) cs.getPreparedStatement();
					if (pStmt == origPStmt){
						cs.inUse_ = false;
						cs.setLastUsedInfo();
						foundFlag=true;		
						break;
//						origPStmt.close(true);						
					}						
				}
			}
			
			if(!foundFlag){
				origPStmt.close(true);
				if (out_ != null) {
				if (traceFlag_ >= 1)
					out_.println(getTraceId() + "closePreparedStatement()["
							+origPStmt.getStmtLabel()+ "]: Hard closed duplicate statement - " + "\""
							+ origPStmt + "\"");
			}
			}
			
			/*
			 * Description : Identify and close the duplicate statement.
			 */
//			if (cs != null) {
//				pStmt = (SQLMXPreparedStatement) cs.getPreparedStatement();
//				if (pStmt != origPStmt)
//					cs = null;
//
//			}
//
//			if (cs == null) {
//				origPStmt.close(true);
//				if (out_ != null) {
//					if (traceFlag_ >= 1)
//						out_.println(getTraceId() + "closePreparedStatement() "
//								+ "Hard closed duplicate statement - " + "\""
//								+ lookupKey + "\"");
//				}
//			}
//			// If the number of cached statements exceeds maxStatements,
//			// perform a hard close
//			if (cs != null) {
//				if (count_ > maxStatements_) {
//					// Remove it from cache
//					cs = (CachedPreparedStatement) prepStmtsInCache_
//							.remove(lookupKey);
//					pStmt = (SQLMXPreparedStatement) cs.getPreparedStatement();
//					pStmt.close(true);
//					count_--;
//					if (out_ != null) {
//						if ((traceFlag_ == T2Driver.POOLING_LVL)
//								|| (traceFlag_ == T2Driver.ENTRY_LVL))
//							out_.println(getTraceId() + "closePreparedStatement() "
//									+ "Hard closed cached statement - " + "\""
//									+ lookupKey + "\"");
//					}
//				} else {
//					cs.inUse_ = false;
//					cs.setLastUsedInfo();
//					if (out_ != null) {
//						if ((traceFlag_ == T2Driver.POOLING_LVL)
//								|| (traceFlag_ == T2Driver.ENTRY_LVL))
//							out_.println(getTraceId() + "closePreparedStatement() "
//									+ "Logical closed cached statement - "
//									+ "\"" + lookupKey + "\"");
//					}
//				}
//			}
//			return;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_closePreparedStatement].methodExit();
		}
	}

	void clearPreparedStatementsAll() {

		if (JdbcDebugCfg.entryActive)
			debug[methodId_clearPreparedStatementsAll].methodEntry();
		try {
			if (out_ != null) {
				if ((traceFlag_ == T2Driver.POOLING_LVL)
						|| (traceFlag_ == T2Driver.ENTRY_LVL))
					out_.println(getTraceId() + "clearPreparedStatementsAll()");
			}
			if (prepStmtsInCache_ != null)
				prepStmtsInCache_.clear();
			count_ = 0;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_clearPreparedStatementsAll].methodExit();
		}
	}

	void addPreparedStatement(SQLMXConnection connect, String sql,
			PreparedStatement pStmt, int resultSetType,
			int resultSetConcurrency, int resultSetHoldability)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_addPreparedStatement].methodEntry();
		if (out_ != null) {
			if ((traceFlag_ == T2Driver.POOLING_LVL)
					|| (traceFlag_ == T2Driver.ENTRY_LVL))
				out_.println(getTraceId() + "addPreparedStatement(" + connect
						+ ",\"" + sql + "\"," + pStmt + "," + resultSetType
						+ "," + resultSetConcurrency + ","
						+ resultSetHoldability + ")");
		}

		try {
			CachedPreparedStatement cachedStmt;
			String lookupKey;
			lookupKey = sql;
			if (connect.catalog_ != null)
				lookupKey = lookupKey.concat(connect.catalog_);
			if (connect.schema_ != null)
				lookupKey = lookupKey.concat(connect.schema_);			
			lookupKey = lookupKey.concat(String.valueOf(connect.transactionIsolation_))
					.concat(String.valueOf(resultSetHoldability));
//			lookupKey = sql;
//			if (connect.catalog_ != null)
//				lookupKey.concat(connect.catalog_);
//			if (connect.schema_ != null)
//				lookupKey.concat(connect.schema_);
//			lookupKey.concat(String.valueOf(connect.transactionIsolation_))
//					.concat(String.valueOf(resultSetHoldability));

			boolean hasRoom = true;
			if (count_ >= maxStatements_) {
				hasRoom = makeRoom();
				if (!hasRoom) {
					// Cached statement count has reached maxStatements AND they
					// are all in use
					Object[] warnMsg = new Object[1];
					warnMsg[0] = new Integer(maxStatements_);
					// Set a warning indicating maxStatements have been exceeded
					connect.setSQLWarning(connect.locale_,
							"exceeded_maxStatements", warnMsg);
					return;
				}
			}
			if (count_ < maxStatements_ || hasRoom) {
				if (!prepStmtsInCache_.containsKey(lookupKey)) {
					cachedStmt = new CachedPreparedStatement(pStmt, lookupKey);
					prepStmtsInCache_.put(lookupKey, cachedStmt);
					count_++;
					if (out_ != null
							&& ((traceFlag_ == T2Driver.POOLING_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL))) {
						//R3.2 changes -- start
						out_.println(getTraceId()
								+ "addPreparedStatement() Added stmt to cache "
								+ "\"" + cachedStmt.getPstmt_().stmtId_ + "\" ; "
								+ "cached stmt cnt = " + count_);
						//R3.2 changes -- end
					}
				}
			}
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_addPreparedStatement].methodExit();
		}

	}

	PreparedStatement getPreparedStatement(SQLMXConnection connect, String sql,
			int resultSetType, int resultSetConcurrency,
			int resultSetHoldability) throws SQLException {

		if (JdbcDebugCfg.entryActive)
			debug[methodId_getPreparedStatement].methodEntry();
		try {
			if (out_ != null) {
				if ((traceFlag_ == T2Driver.POOLING_LVL)
						|| (traceFlag_ == T2Driver.ENTRY_LVL))
					out_.println(getTraceId() + "getPreparedStatement(" + connect
							+ ",\"" + sql + "\"," + resultSetType + ","
							+ resultSetConcurrency + "," + resultSetHoldability
							+ ")");
			}

			PreparedStatement pStmt = null;
			CachedPreparedStatement cachedStmt;
			String lookupKey;

			lookupKey = sql;
			if (connect.catalog_ != null)
				lookupKey = lookupKey.concat(connect.catalog_);
			if (connect.schema_ != null)
				lookupKey = lookupKey.concat(connect.schema_);
			lookupKey = lookupKey.concat(String.valueOf(connect.transactionIsolation_))
					.concat(String.valueOf(resultSetHoldability));
//			lookupKey = sql;
//			if (connect.catalog_ != null)
//				lookupKey.concat(connect.catalog_);
//			if (connect.schema_ != null)
//				lookupKey.concat(connect.schema_);
//			lookupKey.concat(String.valueOf(connect.transactionIsolation_))
//					.concat(String.valueOf(resultSetHoldability));
			
			if (prepStmtsInCache_ != null) {
				cachedStmt = (CachedPreparedStatement) prepStmtsInCache_
						.get(lookupKey);
				if (cachedStmt != null) {
					if (!cachedStmt.inUse_) {
						pStmt = cachedStmt.getPreparedStatement();
						((org.apache.trafodion.jdbc.t2.SQLMXPreparedStatement) pStmt)
								.reuse(connect, resultSetType,
										resultSetConcurrency,
										resultSetHoldability);
						cachedStmt.inUse_ = true;
						cachedStmt.setLastUsedInfo();
					} else
						pStmt = null;
				}
			}
			return pStmt;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getPreparedStatement].methodExit();
		}
	}

	void setLogInfo(int traceFlag, PrintWriter out) {

		if (JdbcDebugCfg.entryActive)
			debug[methodId_setLogInfo].methodEntry();
		try {
			traceFlag_ = traceFlag;
			out_ = out;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setLogInfo].methodExit();
		}
	}

	PreparedStatementManager() {
		super();
		if (JdbcDebugCfg.entryActive)
			debug[methodId_PreparedStatementManager_V].methodEntry();
		try {
			String className = getClass().getName();

			// Build up template portion of jdbcTrace output. Pre-appended to
			// jdbcTrace entries.
			// jdbcTrace:[XXXX]:[Thread[X,X,X]]:[XXXXXXXX]:ClassName.
			setTraceId(T2Driver.traceText
					+ T2Driver.dateFormat.format(new Date())
					+ "]:["
					+ Thread.currentThread()
					+ "]:["
					+ hashCode()
					+ "]:"
					+ className.substring(T2Driver.REMOVE_PKG_NAME,
							className.length()) + ".");
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_PreparedStatementManager_V].methodExit();
		}
	}

	PreparedStatementManager(T2Properties info) {
		super();
		if (JdbcDebugCfg.entryActive)
			debug[methodId_PreparedStatementManager_L].methodEntry();
		try {
			

//			String tmp;

			if (info != null) {
//				tmp = info.getProperty("maxStatements");
				maxStatements_ = info.getMaxStatements();
//				if (tmp != null)
//					maxStatements_ = Integer.parseInt(tmp);
				if (maxStatements_ < 1)
					maxStatements_ = 0;
			}
			if (maxStatements_ > 0) {
				prepStmtsInCache_ = new HashMap<String, CachedPreparedStatement>();
			}

			// Build up template portion of jdbcTrace output. Pre-appended to
			// jdbcTrace entries.
			// jdbcTrace:[XXXX]:[Thread[X,X,X]]:[XXXXXXXX]:ClassName.
			
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_PreparedStatementManager_L].methodExit();
		}
	}
	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		String className = getClass().getName();
		setTraceId(T2Driver.traceText
				+ T2Driver.dateFormat.format(new Date())
				+ "]:["
				+ Thread.currentThread()
				+ "]:["
				+ this.toString()
				+ "]:"
				+ className.substring(T2Driver.REMOVE_PKG_NAME,
						className.length()) + ".");
		return traceId_;
	}
	protected HashMap<String, CachedPreparedStatement> prepStmtsInCache_;
	private int maxStatements_;
	private int count_;

	int traceFlag_;
	PrintWriter out_;
	private String traceId_;

	private static int methodId_isStatementCachingEnabled = 0;
	private static int methodId_makeRoom = 1;
	private static int methodId_closePreparedStatementsAll = 2;
	private static int methodId_closePreparedStatement = 3;
	private static int methodId_clearPreparedStatementsAll = 4;
	private static int methodId_addPreparedStatement = 5;
	private static int methodId_getPreparedStatement = 6;
	private static int methodId_setLogInfo = 7;
	private static int methodId_PreparedStatementManager_V = 8;
	private static int methodId_PreparedStatementManager_L = 9;
	private static int totalMethodIds = 10;
	private static JdbcDebug[] debug;

	static {
		String className = "PreparedStatementManager";
		if (JdbcDebugCfg.entryActive) {
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_isStatementCachingEnabled] = new JdbcDebug(
					className, "isStatementCachingEnabled");
			debug[methodId_makeRoom] = new JdbcDebug(className, "makeRoom");
			debug[methodId_closePreparedStatementsAll] = new JdbcDebug(
					className, "closePreparedStatementsAll");
			debug[methodId_closePreparedStatement] = new JdbcDebug(className,
					"closePreparedStatement");
			debug[methodId_clearPreparedStatementsAll] = new JdbcDebug(
					className, "clearPreparedStatementsAll");
			debug[methodId_addPreparedStatement] = new JdbcDebug(className,
					"addPreparedStatement");
			debug[methodId_getPreparedStatement] = new JdbcDebug(className,
					"getPreparedStatement");
			debug[methodId_setLogInfo] = new JdbcDebug(className, "setLogInfo");
			debug[methodId_PreparedStatementManager_V] = new JdbcDebug(
					className, "PreparedStatementManager_V");
			debug[methodId_PreparedStatementManager_L] = new JdbcDebug(
					className, "PreparedStatementManager_L");
		}
	}
}
