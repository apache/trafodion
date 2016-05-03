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

import java.io.PrintWriter;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.logging.Level;

public abstract class PreparedStatementManager extends HPT4Handle {

	boolean isStatementCachingEnabled() {
		if (maxStatements_ < 1) {
			return false;
		} else {
			return true;
		}
	}

	boolean makeRoom() throws SQLException {
		if (out_ != null) {
			if (traceLevel_ != Level.OFF) {
				out_.println(traceId_ + "makeRoom()");
			}
		}

		Iterator i;
		CachedPreparedStatement cs;
		long oldest;
		long stmtTime;
		String key;

		i = (prepStmtsInCache_.values()).iterator();
		if (!i.hasNext()) {
			return false;
		}
		cs = (CachedPreparedStatement) i.next();
		stmtTime = cs.getLastUsedTime();
		key = cs.getLookUpKey();
		oldest = stmtTime;

		for (; i.hasNext();) {
			cs = (CachedPreparedStatement) i.next();
			stmtTime = cs.getLastUsedTime();
			if (oldest > stmtTime) {
				oldest = stmtTime;
				key = cs.getLookUpKey();
			}
		}
		cs = (CachedPreparedStatement) prepStmtsInCache_.remove(key);
		if (cs != null) {
			if (cs.inUse_ == false) // if the user has already closed the
									// statement, hard close it
				cs.close(true);

			return true;
		} else {
			return false;
		}
	}

	void closePreparedStatementsAll() throws SQLException {

		if (out_ != null) {
			if (traceLevel_ != Level.OFF) {
				out_.println(traceId_ + "closePreparedStatementsAll()");
			}
		}

		Object[] csArray;

		CachedPreparedStatement cs;
		int i = 0;

		csArray = (prepStmtsInCache_.values()).toArray();
		for (i = 0; i < csArray.length; i++) {
			cs = (CachedPreparedStatement) csArray[i];
			if (cs != null) {
				cs.close(false);
			}
		}
	}

	private String createKey(TrafT4Connection connect, String sql, int resultSetHoldability) throws SQLException {
		String lookupKey = sql + connect.getCatalog() + connect.getSchema() + connect.getTransactionIsolation()
				+ resultSetHoldability;

		return lookupKey;
	}

	boolean closePreparedStatement(TrafT4Connection connect, String sql, int resultSetType, int resultSetConcurrency,
			int resultSetHoldability) throws SQLException {
		if (out_ != null) {
			if (traceLevel_ != Level.OFF) {
				out_.println(traceId_ + "closePreparedStatement(" + connect + ",\"" + sql + "\"," + resultSetType + ","
						+ resultSetConcurrency + "," + resultSetHoldability + ")");
			}
		}

		CachedPreparedStatement cs;

		String lookupKey = createKey(connect, sql, resultSetHoldability);

		cs = (CachedPreparedStatement) prepStmtsInCache_.get(lookupKey);
		if (cs != null) {
			cs.inUse_ = false;
			return true;
		}

		return false;
	}

	void clearPreparedStatementsAll() {
		if (out_ != null) {
			if (traceLevel_ != Level.OFF) {
				out_.println(traceId_ + "clearPreparedStatementsAll()");
			}
		}
		if (prepStmtsInCache_ != null) {
			prepStmtsInCache_.clear();
		}
		count_ = 0;
	}

	void addPreparedStatement(TrafT4Connection connect, String sql, PreparedStatement pStmt, int resultSetType,
			int resultSetConcurrency, int resultSetHoldability) throws SQLException {
		if (out_ != null) {
			if (traceLevel_ != Level.OFF) {
				out_.println(traceId_ + "addPreparedStatement(" + connect + ",\"" + sql + "\"," + pStmt + ","
						+ resultSetType + "," + resultSetConcurrency + "," + resultSetHoldability + ")");
			}
		}

		CachedPreparedStatement cachedStmt;

		String lookupKey = createKey(connect, sql, resultSetHoldability);

		cachedStmt = (CachedPreparedStatement) prepStmtsInCache_.get(lookupKey);
		if (cachedStmt != null) {
			// Update the last use time
			cachedStmt.setLastUsedInfo();
		} else {
			if (count_ < maxStatements_) {
				cachedStmt = new CachedPreparedStatement(pStmt, lookupKey);
				prepStmtsInCache_.put(lookupKey, cachedStmt);
				count_++;
			} else {
				if (makeRoom()) {
					cachedStmt = new CachedPreparedStatement(pStmt, lookupKey);
					prepStmtsInCache_.put(lookupKey, cachedStmt);
				}
			}
		}
	}

	PreparedStatement getPreparedStatement(TrafT4Connection connect, String sql, int resultSetType,
			int resultSetConcurrency, int resultSetHoldability) throws SQLException {
		if (out_ != null) {
			if (traceLevel_ != Level.OFF) {
				out_.println(traceId_ + "getPreparedStatement(" + connect + ",\"" + sql + "\"," + resultSetType + ","
						+ resultSetConcurrency + "," + resultSetHoldability + ")");
			}
		}

		PreparedStatement pStmt = null;
		CachedPreparedStatement cachedStmt;

		String lookupKey = createKey(connect, sql, resultSetHoldability);

		if (prepStmtsInCache_ != null) {
			cachedStmt = (CachedPreparedStatement) prepStmtsInCache_.get(lookupKey);
			if (cachedStmt != null) {
				if (!cachedStmt.inUse_) {
					pStmt = cachedStmt.getPreparedStatement();
					((org.trafodion.jdbc.t4.TrafT4PreparedStatement) pStmt).reuse(connect, resultSetType, resultSetConcurrency,
							resultSetHoldability);
				} else {
					pStmt = null;
				}
			}
		}
		return pStmt;
	}

	void setLogInfo(Level traceLevel, PrintWriter out) {
		this.traceLevel_ = traceLevel;
		this.out_ = out;

	}

	PreparedStatementManager() {
		super();
		String className = getClass().getName();
		traceId_ = "jdbcTrace:[" + Thread.currentThread() + "]:[" + hashCode() + "]:" + className + ".";
	}

	PreparedStatementManager(T4Properties t4props) {
		super();

		String className = getClass().getName();

		String tmp;

		if (t4props != null) {
			maxStatements_ = t4props.getMaxStatements();

		}
		if (maxStatements_ > 0) {
			prepStmtsInCache_ = new Hashtable();
		}
		traceId_ = "jdbcTrace:[" + Thread.currentThread() + "]:[" + hashCode() + "]:" + className + ".";
	}

	private Hashtable prepStmtsInCache_;
	private int maxStatements_;
	private int count_;

	Level traceLevel_;
	PrintWriter out_;
	String traceId_;
}
