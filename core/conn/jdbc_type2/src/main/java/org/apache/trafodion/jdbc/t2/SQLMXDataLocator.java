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

/* -*-java-*-
 * Filename    : SQLMXDataLocator.java
 * Description : For each clobTableName or blobTableName property, an 
 *     instance of this object will be created.
 *
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
//import com.tandem.tmf.*;		// Linux port - ToDo
import java.util.Date;
import java.io.PrintWriter;

class SQLMXDataLocator {
	synchronized long getDataLocator(SQLMXConnection conn, boolean isBlob)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getDataLocator].methodEntry();
		try {
			isBlob_ = isBlob;
			// This method is synchronized to ensure that the two different
			// threads will not reserve
			// data locator,
			if (startDataLocator_ == 0
					|| ((currentDataLocator_ - startDataLocator_ + 1) == SQLMXConnection.reserveDataLocator_))
				currentDataLocator_ = getStartDataLocator(conn);
			else
				currentDataLocator_++;
			return currentDataLocator_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getDataLocator].methodExit();
		}
	}

	long getStartDataLocator(SQLMXConnection conn) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getStartDataLocator].methodEntry();

/* Linux port - ToDo tmf.jar related
		try {
			Current tx = null;
			ControlRef cref = null;
			boolean txBegin = false;
			int currentTxid = conn.getTxid_();
			conn.setTxid_(0);

			try {
				tx = new Current();
				// Suspend the existing transaction, suspend returns null if
				// there is none
				cref = tx.suspend();
				tx.begin();
				txBegin = true;

				synchronized (conn) {
					prepareGetStrtDataLocStmt(conn);
					GetStrtDataLoc_.setLong(1,
							SQLMXConnection.reserveDataLocator_);

					ResultSet rs = GetStrtDataLoc_.executeQuery();
					try {
						if (rs.next()) {
							startDataLocator_ = rs.getLong(1);

							if ((traceWriter_ != null)
									&& ((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL))) {
								traceWriter_
										.println(getTraceId()
												+ "getStartDataLocator() - GetStrtDataLocStmt params: reserveDataLocator_="
												+ SQLMXConnection.reserveDataLocator_
												+ " startDataLocator_="
												+ startDataLocator_);
							}
						} else {
							insertDataLocatorRow(conn);
						}
					} finally {
						rs.close();
					}
					tx.commit(false);
					txBegin = false;
				}
				if (cref != null)
					tx.resume(cref);
			}
			catch (com.tandem.util.FSException fe1) {
				SQLException se1 = null;
				SQLException se2;

				try {
					if (txBegin)
						tx.rollback();
					if (cref != null)
						tx.resume(cref);
				} catch (com.tandem.util.FSException fe2) {
					Object[] messageArguments = new Object[2];
					messageArguments[0] = Short.toString(fe2.error);
					messageArguments[1] = fe2.getMessage();
					se1 = Messages.createSQLException(conn.locale_,
							"transaction_error", messageArguments);
				}
				Object[] messageArguments = new Object[2];
				messageArguments[0] = Short.toString(fe1.error);
				messageArguments[1] = fe1.getMessage();
				se2 = Messages.createSQLException(conn.locale_,
						"transaction_error", messageArguments);
				if (se1 != null)
					se2.setNextException(se1);
				throw se2;
			}
			catch (SQLException se) {
				SQLException se1 = null;
				try {
					if (txBegin)
						tx.rollback();
					if (cref != null)
						tx.resume(cref);
				}
				catch (com.tandem.util.FSException fe2) {
					Object[] messageArguments = new Object[2];
					messageArguments[0] = Short.toString(fe2.error);
					messageArguments[1] = fe2.getMessage();
					se1 = Messages.createSQLException(conn.locale_,
							"transaction_error", messageArguments);
				}
				if (se1 != null)
					se.setNextException(se1);
				throw se;
			} finally {
				conn.setTxid_(currentTxid);
			}
			return startDataLocator_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getStartDataLocator].methodExit();
		}
*/
		return 0;	// Linux port - Temp added
	}

	void insertDataLocatorRow(SQLMXConnection conn) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_insertDataLocatorRow].methodEntry();
		try {
			String lobInsDataLocRowSQL;

			if (prepareInsert_) {
				String lobGetMaxDataLocSQL = "select max(data_locator) from "
						+ lobTableName_;
				GetMaxDataLoc_ = conn.prepareLobStatement(lobGetMaxDataLocSQL);
			}

			ResultSet rs = GetMaxDataLoc_.executeQuery();
			try {
				if (rs.next()) {
					startDataLocator_ = rs.getLong(1) + 1;

					if ((traceWriter_ != null)
							&& ((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL))) {
						traceWriter_
								.println(getTraceId()
										+ "insertDataLocatorRow() - Retrieved startDataLocator_="
										+ startDataLocator_ + " from "
										+ lobTableName_);
					}
				}
			} finally {
				rs.close();
			}

			long dataLocVal = startDataLocator_
					+ SQLMXConnection.reserveDataLocator_ - 1;

			if ((traceWriter_ != null)
					&& ((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL))) {
				traceWriter_.println(getTraceId()
						+ "insertDataLocatorRow() - dataLocVal=" + dataLocVal
						+ " startDataLocator_=" + startDataLocator_
						+ " lobCharSet_=" + lobCharSet_ + " prepareInsert_="
						+ prepareInsert_ + " prepDataLocVal_="
						+ prepDataLocVal_);
			}

			// Re-prepare special LOB table row if lobTableName_ has changed
			// (prepareInsert_ flag)
			// or if the data locator value has changed from previous statement
			// prepare.
			if (prepareInsert_ || (dataLocVal != prepDataLocVal_)) {
				if (lobCharSet_.equals(SQLMXConnection.UCS_STR)) // Unicode data
				{
					lobInsDataLocRowSQL = "insert into "
							+ lobTableName_
							+ " (table_name, data_locator, chunk_no, lob_data) values ('ZZDATA_LOCATOR', 0, 0, cast("
							+ dataLocVal
							+ " as VARCHAR(100) character set UCS2))";
				} else // ISO data
				{
					lobInsDataLocRowSQL = "insert into "
							+ lobTableName_
							+ " (table_name, data_locator, chunk_no, lob_data) values ('ZZDATA_LOCATOR', 0, 0, cast("
							+ dataLocVal + " as VARCHAR(100)))";
				}
				InsDataLocRow_ = conn.prepareLobStatement(lobInsDataLocRowSQL);
				prepDataLocVal_ = dataLocVal;
				prepareInsert_ = false;
			}

			InsDataLocRow_.executeUpdate();
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_insertDataLocatorRow].methodExit();
		}
	}

	void prepareGetStrtDataLocStmt(SQLMXConnection conn) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_prepareGetStrtDataLocStmt].methodEntry();
		try {
			/*
			 * Description: JDBC no longer throws
			 * "Connection does not exist" for an active connection.
			 */
			if (conn.prepareGetStrtDataLocStmt(lobTableName_, isBlob_,
					lobCharSet_)
					|| (GetStrtDataLoc_.getConnection() != conn)) {
				if (isBlob_) {
					GetStrtDataLoc_ = conn.LobPrepStmts[SQLMXConnection.BLOB_GET_STRT_DATA_LOC_STMT];
				} else {
					GetStrtDataLoc_ = conn.LobPrepStmts[SQLMXConnection.CLOB_GET_STRT_DATA_LOC_STMT];
				}
				// Set flag to re-prepare next two LOB statements in the path
				// (GetMaxDataLoc_ and InsDataLocRow_)
				prepareInsert_ = true;
			}
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_prepareGetStrtDataLocStmt].methodExit();
		}
	}

	SQLMXDataLocator(SQLMXConnection conn, String lobTableName)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_SQLMXDataLocator].methodEntry();
		try {


			lobTableName_ = lobTableName;
			// Obtain the precision of the lob_data column to set the
			// appropriate
			// chunk size for LOB data operations.
			String s = "select lob_data from " + lobTableName
					+ " where table_name = 'ZZDATA_LOCATOR' ";
			PreparedStatement ps = conn.prepareLobStatement(s);
			ResultSetMetaData rsmd = ps.getMetaData();
			lobCharSet_ = ((SQLMXResultSetMetaData) rsmd).cpqGetCharacterSet(1);

			// Set appropriate chunkSize_ based on character set type
			if (lobCharSet_.equals(SQLMXConnection.UCS_STR)) {
				chunkSize_ = (rsmd.getPrecision(1)) / 2;
			} else {
				chunkSize_ = rsmd.getPrecision(1);
			}

			if ((traceWriter_ != null)
					&& ((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL))) {
				traceWriter_.println(getTraceId()
						+ "SQLMXDataLocator() - constructor chunkSize_="
						+ chunkSize_ + " lobTableName=" + lobTableName);
			}

			ps.close();
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_SQLMXDataLocator].methodExit();
		}
	}
	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		traceWriter_ = SQLMXDataSource.traceWriter_;

		// Build up template portion of jdbcTrace output. Pre-appended to
		// jdbcTrace entries.
		// jdbcTrace:[XXXX]:[Thread[X,X,X]]:[XXXXXXXX]:ClassName.
		if (traceWriter_ != null) {
			traceFlag_ = T2Driver.traceFlag_;
			String className = getClass().getName();
			setTraceId(T2Driver.traceText
					+ T2Driver.dateFormat.format(new Date())
					+ "]:["
					+ Thread.currentThread()
					+ "]:["
					+ hashCode()
					+ "]:"
					+ className.substring(T2Driver.REMOVE_PKG_NAME,
							className.length()) + ".");
		}
		return traceId_;
	}

	// Fields
	private String traceId_;
	static PrintWriter traceWriter_;
	static int traceFlag_;
	String lobTableName_;
	String lobCharSet_;
	long startDataLocator_;
	long currentDataLocator_;
	int chunkSize_;
	long prepDataLocVal_;
	boolean isBlob_;
	boolean prepareInsert_ = false;

	PreparedStatement GetStrtDataLoc_;
	PreparedStatement GetMaxDataLoc_;
	PreparedStatement InsDataLocRow_;

	private static int methodId_getDataLocator = 0;
	private static int methodId_getStartDataLocator = 1;
	private static int methodId_insertDataLocatorRow = 2;
	private static int methodId_prepareGetStrtDataLocStmt = 3;
	private static int methodId_SQLMXDataLocator = 4;
	private static int totalMethodIds = 5;
	private static JdbcDebug[] debug;

	static {
		String className = "SQLMXDataLocator";
		if (JdbcDebugCfg.entryActive) {
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_getDataLocator] = new JdbcDebug(className,
					"getDataLocator");
			debug[methodId_getStartDataLocator] = new JdbcDebug(className,
					"getStartDataLocator");
			debug[methodId_insertDataLocatorRow] = new JdbcDebug(className,
					"insertDataLocatorRow");
			debug[methodId_prepareGetStrtDataLocStmt] = new JdbcDebug(
					className, "prepareGetStrtDataLocStmt");
			debug[methodId_SQLMXDataLocator] = new JdbcDebug(className,
					"SQLMXDataLocator");
		}
	}
}
