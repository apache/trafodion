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
 * Filename		: SQLMXStatement.java
 * Description	:
 */

package org.apache.trafodion.jdbc.t2;

import java.lang.ref.WeakReference;
import java.sql.BatchUpdateException;
import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;

public class SQLMXStatement extends SQLMXHandle implements java.sql.Statement {
	// java.sql.Statement interface Methods
	public void addBatch(String sql) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_addBatch].methodEntry();
		try {
			clearWarnings();

			if (batchCommands_ == null)
				batchCommands_ = new ArrayList<String>();
			batchCommands_.add(sql);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_addBatch].methodExit();
		}
	}

	public void cancel() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_cancel].methodEntry();
		try {
			// Donot clear warning, since the warning may pertain to previous
			// opertation
			// and it is not yet seen by the application

			synchronized (connection_) {
				cancel(connection_.server_, connection_.getDialogueId(), stmtId_);
			}
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_cancel].methodExit();
		}
	}

	public void clearBatch() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_clearBatch].methodEntry();
		try {
			clearWarnings();
			/*
			 * current batch elements list after execution
			 */
			if (batchCommands_ != null) {
				batchCommands_.clear();
				batchCommands_ = null;
			}
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_clearBatch].methodExit();
		}
	}

	public void close() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_close].methodEntry();
		try {
			clearWarnings();
			if (isClosed_)
				return;
			try {
				if ((!connection_.isClosed_) && resultSet_ != null) {
					resultSet_.close(true);
					resultSet_ = null;
					connection_.removeElement(this);

				}
			} finally {
				isClosed_ = true;
				this.stmtId_ = 0;
			}
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_close].methodExit();
		}
	}

	public boolean execute(String sql) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_execute_L].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_execute_L].traceOut(JdbcDebug.debugLevelCLI,
					"sql = " + sql);
		try {
			validateExecDirectInvocation(sql);
			try {
				// Correlate JDBC SQL statements and STMTIDs if enableLog
				// property is enabled
				if (connection_.t2props.getEnableLog().equalsIgnoreCase("ON"))
					printSeqNoIdMapEntry();

				// Allocate a result set in case the execute returns data
				resultSet_ = new SQLMXResultSet(this, null);

				long beginTime=0,endTime,timeTaken;
//				if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
				if(connection_.t2props.getQueryExecuteTime() > 0){
				beginTime=System.currentTimeMillis();
				}
				synchronized (connection_) {
					executeDirect(connection_.server_, connection_.getDialogueId(),
							connection_.getTxid(), connection_.autoCommit_,
							connection_.transactionMode_, getStmtLabel_(),
							cursorName_, sql_.trim(), isSelect_, queryTimeout_,
							resultSetHoldability_, resultSet_,this.stmtId_);
				}
//				if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
				if(connection_.t2props.getQueryExecuteTime() > 0){
					endTime = System.currentTimeMillis();
					timeTaken = endTime - beginTime;
					printQueryExecuteTimeTrace(timeTaken);
				}

				// If there is no data in the result set, throw away result set
				if ((resultSet_ != null)
						&& (resultSet_.getOutputDesc() == null))
					resultSet_ = null;

				// SQL/MX statement are not deallocated when there is a
				// SQLWarning or SQLException or
				// when a resultSet is produced
				if (sqlWarning_ != null) {
					if (resultSet_ == null) {
						try {
							internalClose();
						} catch (SQLException closeException1) {
						}
					}
				}

			} catch (SQLException se) {
				try {
					if (resultSet_ == null)
						internalClose();
				} catch (SQLException closeException) {
					se.setNextException(closeException);
				}
				throw se;
			}
			if (resultSet_ != null)
				return true;
			else
				return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_execute_L].methodExit();
		}
	}

	public boolean execute(String sql, int autoGeneratedKeys)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_execute_LI_auto].methodEntry();
		try {
			boolean ret;

			if (autoGeneratedKeys == NO_GENERATED_KEYS)
				ret = execute(sql);
			else
				throw Messages.createSQLException(connection_.locale_,
						"auto_generated_keys_not_supported", null);
			return ret;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_execute_LI_auto].methodExit();
		}
	}

	public boolean execute(String sql, int[] columnIndexes) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_execute_LI_col].methodEntry();
		try {
			boolean ret;

			if (columnIndexes == null)
				ret = execute(sql);
			else if (columnIndexes.length == 0)
				ret = execute(sql);
			else
				throw Messages.createSQLException(connection_.locale_,
						"auto_generated_keys_not_supported", null);
			return ret;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_execute_LI_col].methodExit();
		}
	}

	public boolean execute(String sql, String[] columnNames)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_execute_LL].methodEntry();
		try {
			boolean ret;

			if (columnNames == null)
				ret = execute(sql);
			else if (columnNames.length == 0)
				ret = execute(sql);
			else
				throw Messages.createSQLException(connection_.locale_,
						"auto_generated_keys_not_supported", null);
			return ret;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_execute_LL].methodExit();
		}
	}

	public int[] executeBatch() throws SQLException, BatchUpdateException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_executeBatch].methodEntry();
		try {

			boolean isSelect;
			int i = 0;
			SQLException se;
			boolean contBatchOnError = false;

			validateExecDirectInvocation();

			if (connection_.contBatchOnErrorval_ == true)
				contBatchOnError = true;

			if (batchCommands_ == null) {
				se = Messages.createSQLException(connection_.locale_,
						"batch_command_failed", null);
				throw new BatchUpdateException(se.getMessage(), se
						.getSQLState(), new int[0]);
			}

			for (i = 0; i < batchCommands_.size(); i++) {
				isSelect = getStmtSqlType((String) batchCommands_.get(i));
				if (isSelect) {
					se = Messages.createSQLException(connection_.locale_,
							"select_in_batch_not_supported", null);
					throw new BatchUpdateException(se.getMessage(), se
							.getSQLState(), new int[0]);
				}
			}

			batchRowCount_ = new int[batchCommands_.size()];
			for (i = 0; i < batchRowCount_.length; i++)
				batchRowCount_[i] = EXECUTE_FAILED;

			try {
				// Batched statements should never return data, so do not need a
				// result set
				resultSet_ = null;

				long beginTime=0,endTime,timeTaken;
//				if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
				if(connection_.t2props.getQueryExecuteTime() > 0){
				beginTime=System.currentTimeMillis();
				}
				synchronized (connection_) {
					executeBatch(connection_.server_, connection_.getDialogueId(),
							connection_.getTxid(), connection_.autoCommit_,
							connection_.transactionMode_, getStmtLabel_(),
							cursorName_, batchCommands_.toArray(), isSelect_,
							queryTimeout_, contBatchOnError,this.stmtId_);
				}
				
//				if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
				if(connection_.t2props.getQueryExecuteTime() > 0){
					endTime = System.currentTimeMillis();
					timeTaken = endTime - beginTime;
					printQueryExecuteTimeTrace(timeTaken);
				}

				// SQL/MX statement are not deallocated when there is a
				// SQLWarning or SQLException or
				// when a resultSet is produced
				if (sqlWarning_ != null) {
					if (resultSet_ == null)
						try {
							internalClose();
						} catch (SQLException closeException) {
						}
				}
			} catch (SQLException e) {
				BatchUpdateException be;

				int[] batch_update_exception_update_count;
				int error_row = 0;
				while ((error_row < batchRowCount_.length)
						&& (batchRowCount_[error_row] != EXECUTE_FAILED || contBatchOnError == true))
					error_row++;
				if (error_row == batchRowCount_.length)
					batch_update_exception_update_count = batchRowCount_;
				else {
					batch_update_exception_update_count = new int[error_row];
					for (i = 0; i < batch_update_exception_update_count.length; i++)
						batch_update_exception_update_count[i] = batchRowCount_[i];
				}
				se = Messages.createSQLException(connection_.locale_,
						"batch_command_failed", null);
				be = new BatchUpdateException(se.getMessage(),
						se.getSQLState(), batch_update_exception_update_count);
				be.setNextException(e);
				try {
					if (resultSet_ == null)
						internalClose();
				} catch (SQLException closeException) {
					be.setNextException(closeException);
				}
				throw be;
			}

			if (batchRowCount_ == null) {
				se = Messages.createSQLException(connection_.locale_,
						"batch_command_failed", null);
				throw new BatchUpdateException(se.getMessage(), se
						.getSQLState(), new int[0]);
			}
			return batchRowCount_;
		} finally {
			if (batchCommands_ != null) {
				batchCommands_.clear();
				batchCommands_ = null;
			}
			if (JdbcDebugCfg.entryActive)
				debug[methodId_executeBatch].methodExit();
		}
	}

	public ResultSet executeQuery(String sql) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_executeQuery].methodEntry();
		try {
			validateExecDirectInvocation(sql);
			if (!isSelect_){
				throw Messages.createSQLException(connection_.locale_,
						"non_select_invalid", null);
			}
			try {
				// Correlate JDBC SQL statements and STMTIDs if enableLog
				// property is enabled
				if (connection_.t2props.getEnableLog().equalsIgnoreCase("ON"))
					printSeqNoIdMapEntry();

				// Allocate a result set in case the execute returns data
				resultSet_ = new SQLMXResultSet(this, null);

				long beginTime=0,endTime,timeTaken;
//				if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
				if(connection_.t2props.getQueryExecuteTime() > 0){
				beginTime=System.currentTimeMillis();
				}
				synchronized (connection_) {
					executeDirect(connection_.server_, connection_.getDialogueId(),
							connection_.getTxid(), connection_.autoCommit_,
							connection_.transactionMode_, getStmtLabel_(),
							cursorName_, sql_.trim(), isSelect_, queryTimeout_,
							resultSetHoldability_, resultSet_,this.stmtId_);
				}
				
				
//				if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
				if(connection_.t2props.getQueryExecuteTime() > 0){
					endTime = System.currentTimeMillis();
					timeTaken = endTime - beginTime;
					printQueryExecuteTimeTrace(timeTaken);
				}

				// If there is no data in the result set, throw away result set
				if ((resultSet_ != null)
						&& (resultSet_.getOutputDesc() == null))
					resultSet_ = null;

				// SQL/MX statement are not deallocated when there is a
				// SQLWarning or SQLException or
				// when a resultSet is produced
				if (resultSet_ == null) {
					try {
						internalClose();
					} catch (SQLException closeException1) {
					}
				}
			} catch (SQLException se) {
				try {
					if (resultSet_ == null)
						internalClose();
				} catch (SQLException closeException) {
					se.setNextException(closeException);
				}
				throw se;
			}
			return resultSet_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_executeQuery].methodExit();
		}
	}

	public int executeUpdate(String sql) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_executeUpdate_L].methodEntry();
		try {
			validateExecDirectInvocation(sql);
			if (isSelect_)
				throw Messages.createSQLException(connection_.locale_,
						"select_invalid", null);
			try {
				// Correlate JDBC SQL statements and STMTIDs if enableLog
				// property is enabled
				if (connection_.t2props.getEnableLog().equalsIgnoreCase("ON"))
					printSeqNoIdMapEntry();

				// Allocate a result set in case the execute returns data
				resultSet_ = new SQLMXResultSet(this, null);

				long beginTime=0,endTime,timeTaken;
//				if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
				if(connection_.t2props.getQueryExecuteTime() > 0){
				beginTime=System.currentTimeMillis();
				}
				synchronized (connection_) {
					executeDirect(connection_.server_, connection_.getDialogueId(),
							connection_.getTxid(), connection_.autoCommit_,
							connection_.transactionMode_, getStmtLabel_(),
							cursorName_, sql_.trim(), isSelect_, queryTimeout_,
							resultSetHoldability_, resultSet_,this.stmtId_);
				}
//				if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
				if(connection_.t2props.getQueryExecuteTime() > 0){
					endTime = System.currentTimeMillis();
					timeTaken = endTime - beginTime;
					printQueryExecuteTimeTrace(timeTaken);
				}
				// If there is no data in the result set, throw away result set
				if ((resultSet_ != null)
						&& (resultSet_.getOutputDesc() == null))
					resultSet_ = null;

				// SQL/MX statement are not deallocated when there is a
				// SQLWarning or SQLException or
				// when a resultSet is produced
				if (sqlWarning_ != null) {
					if (resultSet_ == null) {
						try {
							internalClose();
						} catch (SQLException closeException1) {
						}
					}
				}
			} catch (SQLException se) {
				try {
					if (resultSet_ == null)
						internalClose();
				} catch (SQLException closeException) {
					se.setNextException(closeException);
				}
				throw se;
			}
			return rowCount_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_executeUpdate_L].methodExit();
		}
	}

	public int executeUpdate(String sql, int autoGeneratedKeys)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_executeUpdate_LI].methodEntry();
		try {
			int ret;

			if (autoGeneratedKeys == NO_GENERATED_KEYS)
				ret = executeUpdate(sql);
			else
				throw Messages.createSQLException(connection_.locale_,
						"auto_generated_keys_not_supported", null);
			return ret;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_executeUpdate_LI].methodExit();
		}
	}

	public int executeUpdate(String sql, int[] columnIndexes)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_executeUpdate_LI_array].methodEntry();
		try {
			int ret;

			if (columnIndexes == null)
				ret = executeUpdate(sql);
			else if (columnIndexes.length == 0)
				ret = executeUpdate(sql);
			else
				throw Messages.createSQLException(connection_.locale_,
						"auto_generated_keys_not_supported", null);
			return ret;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_executeUpdate_LI_array].methodExit();
		}
	}

	public int executeUpdate(String sql, String[] columnNames)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_executeUpdate_LL].methodEntry();
		try {
			int ret;

			if (columnNames == null)
				ret = executeUpdate(sql);
			else if (columnNames.length == 0)
				ret = executeUpdate(sql);
			else
				throw Messages.createSQLException(connection_.locale_,
						"auto_generated_keys_not_supported", null);
			return ret;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_executeUpdate_LL].methodExit();
		}
	}

	public Connection getConnection() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getConnection].methodEntry();
		try {
			clearWarnings();
			return connection_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getConnection].methodExit();
		}
	}

	public int getFetchDirection() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getFetchDirection].methodEntry();
		try {
			clearWarnings();
			return fetchDirection_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getFetchDirection].methodExit();
		}
	}

	public int getFetchSize() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getFetchSize].methodEntry();
		try {
			clearWarnings();
			return fetchSize_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getFetchSize].methodExit();
		}
	}

	public ResultSet getGeneratedKeys() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getGeneratedKeys].methodEntry();
		try {
			clearWarnings();
			throw Messages.createSQLException(connection_.locale_,
					"auto_generated_keys_not_supported", null);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getGeneratedKeys].methodExit();
		}
	}

	public int getMaxFieldSize() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxFieldSize].methodEntry();
		try {
			clearWarnings();
			return maxFieldSize_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxFieldSize].methodExit();
		}
	}

	public int getMaxRows() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxRows].methodEntry();
		try {
			clearWarnings();
			return maxRows_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxRows].methodExit();
		}
	}

	public boolean getMoreResults() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMoreResults_V].methodEntry();
		try {
			return getMoreResults(Statement.CLOSE_CURRENT_RESULT);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMoreResults_V].methodExit();
		}
	}

	public boolean getMoreResults(int current) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMoreResults_I].methodEntry();
		try {
			// Just call getMoreResults since we currently do not support
			// multiple result sets
			/* return getMoreResults(); */
			clearWarnings();
			if (!isSPJResultSet_)
				return false;
			// increment rs index
			resultSetIndex_++; 
			if (current == Statement.CLOSE_ALL_RESULTS) {
				closeAllResultSets(false);
			} else if (current == Statement.CLOSE_CURRENT_RESULT) {
				if (resultSet_ != null) {
					resultSet_.close();

				}
			}
			// if(resultSetIndex_ >= resultSetMax_) 
			if (resultSetIndex_ > resultSetMax_) { 
				resultSet_.isClosed_ = false;
				closeAllResultSets(true);
				return false;
			}

			// increment rs index
			// resultSetIndex_++; 

			// set resultSet_ to spjResultSets_ element
			Integer rsKey = new Integer(resultSetIndex_);

			// Create a new result set and set variables for executeRS call
			isSelect_ = true;
			resultSet_ = new SQLMXResultSet(this, null);

			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMoreResults_I]
						.methodParameters("Pre-dialogueId_= "
								+ connection_.getDialogueId() + "  stmtId_= "
								+ stmtId_ + "  resultSetMax_= " + resultSetMax_
								+ "  resultSetIndex_= " + resultSetIndex_
								+ "  isSPJResultSet_= " + isSPJResultSet_
								+ "  resultSet_= " + resultSet_);

			// stmtLabel_ = ... append RSx to existing base RS stmtLabel_
			SPJRSstmtLabel_ = SPJRSbaseStmtLabel_ + resultSetIndex_;

			executeRS(connection_.server_, connection_.getDialogueId(), connection_
					.getTxid(), connection_.autoCommit_,
					connection_.transactionMode_, getStmtLabel_(),
					SPJRSstmtLabel_, isSelect_, stmtId_, resultSetIndex_,
					resultSet_);

			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMoreResults_I]
						.methodParameters("Post-dialogueId_= "
								+ connection_.getDialogueId() + "  stmtId_= "
								+ stmtId_ + "  resultSetMax_= " + resultSetMax_
								+ "  resultSetIndex_= " + resultSetIndex_
								+ "  isSPJResultSet_= " + isSPJResultSet_
								+ "  resultSet_= " + resultSet_);
			spjResultSets_.put(rsKey, resultSet_);

			if (resultSet_ == null)
				return false;
			else
				return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMoreResults_I].methodExit();
		}
	}

	public int getQueryTimeout() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getQueryTimeout].methodEntry();
		try {
			clearWarnings();
			return queryTimeout_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getQueryTimeout].methodExit();
		}
	}

	/**
	 * Returns the current result set. If the result set is from an SPJ call,
	 * resultSet_ is set from the SPJResultSets container using ResultSetIndex_
	 * for an index. If the result set has not been accessed before, a new
	 * SQLMXResultSet is created.
	 */
	public ResultSet getResultSet() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getResultSet].methodEntry();
		try {
			clearWarnings();
			/* return resultSet_; */
			// if statment is closed return null for RS
			if (isClosed_)
				return null;

			// check for SPJ result set type
			if (isSPJResultSet_) {
				// set resultSet_ to spjResultSets_ element if it already
				// exists, otherwise ...
				Integer rsKey = new Integer(resultSetIndex_);
				if (spjResultSets_.containsKey(rsKey)) {
					if (JdbcDebugCfg.traceActive)
						debug[methodId_getResultSet]
								.methodParameters("  ResultSetIndex_= "
										+ resultSetIndex_
										+ "  already created ");
					resultSet_ = (SQLMXResultSet) spjResultSets_.get(rsKey);
				}
				// Create a new result set and set variables for executeRS call
				else {
					isSelect_ = true;
					resultSet_ = new SQLMXResultSet(this, null);
					spjResultSets_.put(rsKey, resultSet_);

					if (JdbcDebugCfg.traceActive)
						debug[methodId_getResultSet]
								.methodParameters("Pre-dialogueId_= "
										+ connection_.getDialogueId()
										+ "  stmtId_= " + stmtId_
										+ "  resultSetMax_= " + resultSetMax_
										+ "  resultSetIndex_= "
										+ resultSetIndex_
										+ "  isSPJResultSet_= "
										+ isSPJResultSet_ + "  resultSet_= "
										+ resultSet_);

					// Create unique SPJRS stmt label by appending RS index
					// to existing base SPJRS stmt label
					SPJRSstmtLabel_ = SPJRSbaseStmtLabel_ + resultSetIndex_;

					executeRS(connection_.server_, connection_.getDialogueId(),
							connection_.getTxid(), connection_.autoCommit_,
							connection_.transactionMode_, getStmtLabel_(),
							SPJRSstmtLabel_, isSelect_, stmtId_,
							resultSetIndex_, resultSet_);

					// if executeRS->setExecRSOutputs set resultSet_ to null
					// set spjResultSets object to null also
					if (resultSet_ == null)
						spjResultSets_.put(rsKey, resultSet_);

					if (JdbcDebugCfg.traceActive)
						debug[methodId_getResultSet]
								.methodParameters("Post-dialogueId_= "
										+ connection_.getDialogueId()
										+ "  stmtId_= " + stmtId_
										+ "  resultSetMax_= " + resultSetMax_
										+ "  resultSetIndex_= "
										+ resultSetIndex_
										+ "  isSPJResultSet_= "
										+ isSPJResultSet_ + "  resultSet_= "
										+ resultSet_);
				}
			}
			return resultSet_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getResultSet].methodExit();
		}
	}

	public int getResultSetConcurrency() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getResultSetConcurrency].methodEntry();
		try {
			clearWarnings();
			return resultSetConcurrency_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getResultSetConcurrency].methodExit();
		}
	}

	public int getResultSetHoldability() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getResultSetHoldability].methodEntry();
		try {
			clearWarnings();
			return resultSetHoldability_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getResultSetHoldability].methodExit();
		}
	}

	public int getResultSetType() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getResultSetType].methodEntry();
		try {
			clearWarnings();
			if (JdbcDebugCfg.traceActive)
				debug[methodId_getResultSetType]
						.methodParameters("resultSetType_ = " + resultSetType_);

			return resultSetType_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getResultSetType].methodExit();
		}
	}

	public int getUpdateCount() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getUpdateCount].methodEntry();
		try {
			// Returns the current result as an update count.
			// If the result is a ResultSet object or there are
			// no more results, -1 is returned.
			// Note: For PreparedStatement batch execution, this
			// method will return the total rows affected.
			clearWarnings();
			if (JdbcDebugCfg.traceActive)
				debug[methodId_getUpdateCount].methodParameters("rowCount_ = "
						+ rowCount_);
			if (rowCount_ < 0)
				return (-1);
			return rowCount_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getUpdateCount].methodExit();
		}
	}

	public void setCursorName(String name) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setCursorName].methodEntry();
		try {
			clearWarnings();
			// TODO: May need to check the Statement STATE
			cursorName_ = name;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setCursorName].methodExit();
		}
	}

	public void setEscapeProcessing(boolean enable) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setEscapeProcessing].methodEntry();
		try {
			clearWarnings();
			escapeProcess_ = enable;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setEscapeProcessing].methodExit();
		}
	}

	public void setFetchDirection(int direction) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setFetchDirection].methodEntry();
		try {
			clearWarnings();
			switch (direction) {
			case ResultSet.FETCH_FORWARD:
				fetchDirection_ = direction;
				break;
			case ResultSet.FETCH_REVERSE:
			case ResultSet.FETCH_UNKNOWN:
				fetchDirection_ = ResultSet.FETCH_FORWARD;
				break;
			default:
				throw Messages.createSQLException(connection_.locale_,
						"invalid_fetch_direction", null);
			}
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setFetchDirection].methodExit();
		}
	}

	public void setFetchSize(int rows) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setFetchSize].methodEntry();
		try {
			clearWarnings();
			if (rows < 0)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_fetchSize_value", null);
			// Fetch size of zero means to take best guess.
			// Since we have no statistics, just leave as is.
			if (rows != 0) {
				fetchSize_ = rows;

				synchronized (connection_) {
					// Pass the fetch size change to the driver
					resetFetchSize(connection_.getDialogueId(), stmtId_, fetchSize_);
				}
			}
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setFetchSize].methodExit();
		}
	}

	public void setMaxFieldSize(int max) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setMaxFieldSize].methodEntry();
		try {
			clearWarnings();
			if (max < 0)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_maxFieldSize_value", null);
			maxFieldSize_ = max;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setMaxFieldSize].methodExit();
		}
	}

	public void setMaxRows(int max) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setMaxRows].methodEntry();
		try {
			clearWarnings();
			if (max < 0)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_maxRows_value", null);
			maxRows_ = max;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setMaxRows].methodExit();
		}
	}

	public void setQueryTimeout(int seconds) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setQueryTimeout].methodEntry();
		try {
			clearWarnings();
			if (seconds < 0)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_queryTimeout_value", null);
			queryTimeout_ = seconds;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setQueryTimeout].methodExit();
		}
	}

	// other Methods
	boolean getStmtSqlType(String sql) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getStmtSqlType].methodEntry();
		try {
			String tempStr;
			int lengStr;
			char find;
			isSelect_ = true;

			tempStr = sql.trim();
			/*
			 * isSelect_ = tempStr.regionMatches(true, 0, "SELECT", 0, 6); if (
			 * ! isSelect_) isSelect_ = tempStr.regionMatches(true, 0,
			 * "SHOWSHAPE", 0, 9);
			 * 
			 */
			if (!isSelect_)
				isSelect_ = tempStr.regionMatches(true, 0, "INVOKE", 0, 6); 
			/*
			 * The tempStr is parsed from the beginning.
			 * Blanks,tabs and '(' are ignored. If the next character is 'S' we
			 * match for SELECT or SHOWSHAPE. else if the next character is 'I'
			 * we match for INVOKE, else we consider this query as non-select.
			 */
			lengStr = tempStr.length();
			for (int i = 0; (i < lengStr) && (isSelect_ == true); i++) {
				find = tempStr.charAt(i);
				if (find == ' ' || find == '(' || find == '\t') {
					// ignore these characters
				} else if (find == 'S' || find == 's') {
					isSelect_ = tempStr.regionMatches(true, i, "SELECT", 0, 6);
					if (isSelect_ == false)
						isSelect_ = tempStr.regionMatches(true, i, "SHOWSHAPE",
								0, 9);
					if (isSelect_ == false)
						isSelect_ = tempStr.regionMatches(true, i, "SHOWCONTROL",
								0, 9);
					if (isSelect_ == false)
						isSelect_ = tempStr.regionMatches(true, i, "SHOWDDL",
								0, 9);
					break;
				} else if (find == 'I' || find == 'i') {
					isSelect_ = tempStr.regionMatches(true, 0, "INVOKE", 0, 6);
					break;
				}else if (find == 'E' || find == 'e'){
					isSelect_ = tempStr.regionMatches(true, 0, "EXPLAIN", 0, 6);
					break;
				} else if (find == 'V' || find == 'v'){
					isSelect_ = tempStr.regionMatches(true, 0, "VALUES", 0, 6);
					break;
				}
				else {
					isSelect_ = false;
					break;
				}
			}
			return isSelect_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getStmtSqlType].methodExit();
		}
	}

	void validateExecDirectInvocation(String sql) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_validateExecDirectInvocation_L].methodEntry();
		try {
			validateExecDirectInvocation();
			isSelect_ = getStmtSqlType(sql);
			sql_ = sql;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_validateExecDirectInvocation_L].methodExit();
		}
	}

	void validateExecDirectInvocation() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_validateExecDirectInvocation_V].methodEntry();
		try {
			clearWarnings();
			if (isClosed_)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_statement", null);
			if (connection_.isClosed_)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_connection", null);

			// close the previous resultset, if any
			if (resultSet_ != null)
				resultSet_.close();
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_validateExecDirectInvocation_V].methodExit();
		}
	}

	// This functions ensure that SQL/MX Resources are cleaned up, but leave the
	// java Statement object
	// intact.
	public void internalClose() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_internalClose].methodEntry();
		try {
			synchronized (connection_) {
				if (!connection_.isClosed_) {
					if (stmtId_ != 0)
						close(connection_.server_, connection_.getDialogueId(),
								stmtId_, true);
					else
						closeUsingLabel(connection_.server_,
								connection_.getDialogueId(), getStmtLabel_(), true);
				}
			}// End sync
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_internalClose].methodExit();
		}
	}

	// Method used by JNI Layer to update the results of ExecDirect
	void setExecDirectOutputs(SQLMXDesc[] outputDesc, int rowCount,
			DataWrapper[] fetchedRows, int fetchedRowCount, int txid,
			long stmtId, int stmtType) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setExecDirectOutputs].methodEntry();
		try {
			outputDesc_ = outputDesc;
			if (outputDesc != null) {
				stmtId_ = stmtId;
				// Update the result set information with the statement info
				resultSet_.setupResultSet(outputDesc, stmtId_);
				// Add stmt that produces result set to the weak Reference list
				// to release SQL/MX Resources
				// For, rest of the stmts, SQL/MX resources are already
				// reduced
				connection_.addElement(this);
				// Check if the transaction is started by this Select statement
				if (connection_.getTxid() == 0 && txid != 0)
					resultSet_.txnStarted_ = true;
				rowCount_ = -1;
			} else {
				resultSet_ = null;
				stmtId_ = stmtId;
				rowCount_ = rowCount;
			}
			connection_.setTxid_(txid);
			// Populate the result set if row information was returned on the
			// execute
			if ((fetchedRows != null) && (resultSet_ != null)) {
				resultSet_.setFetchOutputs(fetchedRows, fetchedRowCount, true,
						txid);
			}

			// Update connection attribute changes as required
			// for SQLMX versions above release 2.0
			if ((T2Driver.getDatabaseMajorVersion() != T2Driver.DATABASE_MAJOR_VERSION_R20)
					|| (T2Driver.getDatabaseMinorVersion() != T2Driver.DATABASE_MINOR_VERSION_R20)) {
				connection_.updateConnectionReusability(stmtType);
			}
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setExecDirectOutputs].methodExit();
		}
	}

	// Method used by JNI Layer to update the SPJ resultSets (via executeRS)
	void setExecRSOutputs(SQLMXDesc[] outputDesc, int txid, long RSstmtId,
			int RSIndex) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setExecRSOutputs].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setExecRSOutputs].methodParameters("  txid = "
					+ txid + "  RSstmtId = 0x" + Long.toHexString(RSstmtId)
					+ "  RSIndex = " + RSIndex);
		try {
			currentResultSetIndex_ = RSIndex; 
			spjRSCommitCount_++; 
			outputDesc_ = outputDesc;
			// connection_.txid_ = txid;

			if (outputDesc != null) {
				// Update the result set information
				resultSet_.setupResultSet(outputDesc, RSstmtId);

				// Check if the transaction is started by this Select statement
				// if (connection_.txid_ == 0 && txid != 0) 
				if (connection_.getTxid() == txid) 
					resultSet_.txnStarted_ = true;
				rowCount_ = -1;
			} else {
				resultSet_ = null;
			}
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setExecRSOutputs].methodExit();
		}
	}

	void setExecDirectBatchOutputs(int commandIndex, int rowCount, int txid) {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setExecDirectBatchOutputs].methodEntry();
		try {
			batchRowCount_[commandIndex] = rowCount;
			connection_.setTxid_(txid);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setExecDirectBatchOutputs].methodExit();
		}
	}

	void setCurrentTxid(int txid) {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setCurrentTxid].methodEntry();
		try {
			if (JdbcDebugCfg.traceActive)
				debug[methodId_setCurrentTxid].traceOut(
						JdbcDebug.debugLevelTxn,
						"Connection transaction set to " + txid);
			connection_.setTxid_(txid);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setCurrentTxid].methodExit();
		}
	}

	void setCurrentStmtId(long stmtId) {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setCurrentTxid].methodEntry();
		try {
			if (JdbcDebugCfg.traceActive)
				debug[methodId_setCurrentStmtId].traceOut(
						JdbcDebug.debugLevelTxn,
						"Connection transaction set to " + stmtId);
			this.stmtId_ =  stmtId;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setCurrentStmtId].methodExit();
		}
	}
	void closeAllResultSets(boolean reset) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_closeAllResultSets].methodEntry();
		try {
			Iterator resultSetIterator = spjResultSets_.values().iterator();
			int i = 0;
			while (resultSetIterator.hasNext()) {
				SQLMXResultSet rs = (SQLMXResultSet) resultSetIterator.next();
				if (rs != null) {
					isClosed_ = false;
					rs.close();
				}
			}
			if (reset)
				resultSetIndex_ = 0;
			spjResultSets_.clear();
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_closeAllResultSets].methodExit();
		}
	}

	boolean firstResultSetExists() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_firstResultSetExists].methodEntry();
		try {
			// set resultSet_ to spjResultSets_ element
			Integer rsKey = new Integer(resultSetIndex_);

			// Create a new result set and set variables for executeRS call
			isSelect_ = true;
			resultSet_ = new SQLMXResultSet(this, null);
			spjResultSets_.put(rsKey, resultSet_);

			if (JdbcDebugCfg.traceActive)
				debug[methodId_firstResultSetExists]
						.methodParameters("Pre-dialogueId_= "
								+ connection_.getDialogueId() + "  stmtId_= "
								+ stmtId_ + "  resultSetMax_= " + resultSetMax_
								+ "  resultSetIndex_= " + resultSetIndex_
								+ "  isSPJResultSet_= " + isSPJResultSet_
								+ "  resultSet_= " + resultSet_);

			// stmtLabel_ = ... append RSx to existing base RS stmtLabel_
			SPJRSstmtLabel_ = SPJRSbaseStmtLabel_ + resultSetIndex_;

			executeRS(connection_.server_, connection_.getDialogueId(), connection_
					.getTxid(), connection_.autoCommit_,
					connection_.transactionMode_, getStmtLabel_(),
					SPJRSstmtLabel_, isSelect_, stmtId_, resultSetIndex_,
					resultSet_);

			if (JdbcDebugCfg.traceActive)
				debug[methodId_firstResultSetExists]
						.methodParameters("Post-dialogueId_= "
								+ connection_.getDialogueId() + "  stmtId_= "
								+ stmtId_ + "  resultSetMax_= " + resultSetMax_
								+ "  resultSetIndex_= " + resultSetIndex_
								+ "  isSPJResultSet_= " + isSPJResultSet_
								+ "  resultSet_= " + resultSet_);
			if (resultSet_ == null) {
				spjResultSets_.put(rsKey, resultSet_);
				return false;
			} else
				return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_firstResultSetExists].methodExit();
		}
	}

	// Log the JDBC SQL statement and the STMTID to the idMapFile if the
	// enableLog_ property is set.
	private void printSeqNoIdMapEntry() {
		synchronized (this) {
			seqno_++;
			setStmtLabel_(stmtLabelBase_ + "_" + seqno_);
		}
		SQLMXDataSource.prWriter_.println("["
				+ T2Driver.dateFormat.format(new java.util.Date()) + "] "
				+ getStmtLabel_() + " (" + sql_.trim() + ")\n");
	}

	// Constructors with access specifier as "default"
	SQLMXStatement() {
		if (JdbcDebugCfg.entryActive) {
			debug[methodId_SQLMXStatement_V].methodEntry();
			debug[methodId_SQLMXStatement_V].methodExit();
		}
	}

	SQLMXStatement(SQLMXConnection connection) throws SQLException {
		this(connection, ResultSet.TYPE_FORWARD_ONLY,
				ResultSet.CONCUR_READ_ONLY,
				SQLMXResultSet.CLOSE_CURSORS_AT_COMMIT);
	}

	SQLMXStatement(SQLMXConnection connection, int resultSetType,
			int resultSetConcurrency) throws SQLException {
		this(connection, resultSetType, resultSetConcurrency,
				SQLMXResultSet.CLOSE_CURSORS_AT_COMMIT);
	}

	SQLMXStatement(SQLMXConnection connection, int resultSetType,
			int resultSetConcurrency, int resultSetHoldability)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_SQLMXStatement_LIII].methodEntry();
		try {
			// int hashcode;

			connection_ = connection;
			if (resultSetType != ResultSet.TYPE_FORWARD_ONLY
					&& resultSetType != ResultSet.TYPE_SCROLL_INSENSITIVE
					&& resultSetType != ResultSet.TYPE_SCROLL_SENSITIVE)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_resultset_type", null);
			if (resultSetType == ResultSet.TYPE_SCROLL_SENSITIVE) {
				resultSetType_ = ResultSet.TYPE_SCROLL_INSENSITIVE;
				connection_.setSQLWarning(null, "scrollResultSetChanged", null);
			} else
				resultSetType_ = resultSetType;
			if (resultSetConcurrency != ResultSet.CONCUR_READ_ONLY
					&& resultSetConcurrency != ResultSet.CONCUR_UPDATABLE)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_resultset_concurrency", null);
			resultSetConcurrency_ = resultSetConcurrency;
			resultSetHoldability_ = resultSetHoldability;
			queryTimeout_ = connection_.queryTimeout_;
			/*
			 * hashcode = hashCode(); // SQL/MX doesn't like '-' character in
			 * CursorName if (hashcode < 0) hashcode = -hashcode; stmtLabel_ =
			 * "STMT" + hashcode; if (stmtLabel_.length() > 128) stmtLabel_ =
			 * stmtLabel_.substring(0,128);
			 */
			/*
			 * Replacing hashCode() implementation, as it creates duplicate
			 * codes with a static variable initialized and incremented for each
			 * stmtLabel_.
			 */
			synchronized (connection_) {
				label++;
				setStmtLabel_("STMT" + label);
				if (label == 9223372036854775807l) {
					throw Messages.createSQLException(connection_.locale_,
							"statement_labels_out_of_range", null);
				}
			}
			stmtLabelBase_ = getStmtLabel_();
			SPJRSbaseStmtLabel_ = getStmtLabel_() + "RS";
			fetchSize_ = SQLMXResultSet.DEFAULT_FETCH_SIZE;
			maxRows_ = 0;
			fetchDirection_ = ResultSet.FETCH_FORWARD;
			pRef_ = new WeakReference<SQLMXStatement>(this, connection_.refQ_);
			rowCount_ = -1;
			resultSetIndex_ = 0;
			spjResultSets_ = new HashMap<Integer, SQLMXResultSet>();
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_SQLMXStatement_LIII].methodExit();
		}
	}

	// Native methods
	private native void executeDirect(String server, long dialogueId, int txid,
			boolean autocommit, int txnMode, String stmtLabel,
			String cursorName, String sql, boolean isSelect, int queryTimeout,
			int holdability, SQLMXResultSet resultSet,long stmtId) throws SQLException;

	private native void executeBatch(String server, long dialogueId, int txid,
			boolean autocommit, int txnMode, String stmtLabel,
			String cursorName, Object[] comamnds, boolean isSelect,
			int queryTimeout, boolean contBatchOnError, long stmtId) throws SQLException;
	native static int close(String server, long dialogueId, long stmtId,
			boolean dropStmt) throws SQLException;

	native static void closeUsingLabel(String server, long dialogueId,
			String stmtLabel, boolean dropStmt);
	private native  void cancel(String server, long dialogueId, long stmtId);
	private native void resetFetchSize(long dialogueId, long stmtId, int fetchSize);

	// SPJRS Native methods
	private native void executeRS(String server, long dialogueId, int txid,
			boolean autocommit, int txnMode, String stmtLabel,
			String RSstmtLabel, boolean isSelect, long stmtId,
			int ResultSetIndex, SQLMXResultSet resultSet) throws SQLException;

	public void setStmtLabel_(String stmtLabel_) {
		this.stmtLabel_ = stmtLabel_;
	}

	public String getStmtLabel_() {
		return stmtLabel_;
	}
	
	public String getStmtLabel() {
    return this.getStmtLabel_();
	}
	
	void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	String getTraceId() {
	
		String className = this.getClass().getName();

		// Build up jdbcTrace output entry
		// Build up jdbcTrace output entry
		setTraceId(T2Driver.traceText
				+ T2Driver.dateFormat.format(new Date())
				+ "]:["
				+ Thread.currentThread()
				+ "]:["
				+ connection_.toString()
				+ "]:"
				+ className.substring(T2Driver.REMOVE_PKG_NAME, className.length()) + ".");
		return traceId_;
	}
	
	void printQueryExecuteTimeTrace(long timeTaken){
		
		if ((connection_.t2props.getQueryExecuteTime()!=0 && timeTaken > connection_.t2props.getQueryExecuteTime())) {
			
			if (SQLMXDataSource.queryExecuteTraceWriter_ != null) {
				SQLMXDataSource.queryExecuteTraceWriter_
						.println(this.getStmtLabel() + ":"
								+ this.sql_ + ": TIME TAKEN "
								+ timeTaken + " ms");
			}
		}
		
	}

	public void setStmtLabel(String stmtLabel) {
      this.setStmtLabel_(stmtLabel);
	}

	// static fields from Statement. Passed to JNI.
	public static final int JNI_SUCCESS_NO_INFO = SUCCESS_NO_INFO;
	public static final int JNI_EXECUTE_FAILED = EXECUTE_FAILED;

	static long label = 0;

	// Fields
	SQLMXConnection connection_;
	int resultSetType_;
	int resultSetConcurrency_;
	String sql_;
	int queryTimeout_;
	int maxRows_;
	int maxFieldSize_;
	int fetchSize_;
	int fetchDirection_;
	boolean escapeProcess_;
	String cursorName_;
	int rowCount_;
	SQLMXResultSet resultSet_;
	private String stmtLabel_;
	String stmtLabelBase_;
	String SPJRSstmtLabel_;
	String SPJRSbaseStmtLabel_;
	boolean isSelect_;
	boolean isClosed_;
	ArrayList<String> batchCommands_;
	int[] batchRowCount_;
	SQLMXDesc[] outputDesc_;
	WeakReference<SQLMXStatement> pRef_;
	long stmtId_; // Pointer to SRVR_STMT_HDL structure in native mode
	int resultSetHoldability_;
	int seqno_;
	boolean isSPJResultSet_; // is query SPJRS (determined by the cli stmt query
	// type)
	int resultSetMax_; // Max number of RS's returned from
	// GetStmtAttr(.,SQL_ATTR_MAX_RESULT_SETS,....)
	int resultSetIndex_; // Returned in the ResultSetInfo class
	HashMap<Integer, SQLMXResultSet> spjResultSets_; // Container of Result Sets
	// (keyed by
	// ResultSetIndex_)
	int currentResultSetIndex_; 
	int spjRSCommitCount_ = 0;
	
	private String traceId_;

	private static int methodId_addBatch = 0;
	private static int methodId_cancel = 1;
	private static int methodId_clearBatch = 2;
	private static int methodId_close = 3;
	private static int methodId_execute_L = 4;
	private static int methodId_execute_LI_auto = 5;
	private static int methodId_execute_LI_col = 6;
	private static int methodId_execute_LL = 7;
	private static int methodId_executeBatch = 8;
	private static int methodId_executeQuery = 9;
	private static int methodId_executeUpdate_L = 10;
	private static int methodId_executeUpdate_LI = 11;
	private static int methodId_executeUpdate_LI_array = 12;
	private static int methodId_executeUpdate_LL = 13;
	private static int methodId_getConnection = 14;
	private static int methodId_getFetchDirection = 15;
	private static int methodId_getFetchSize = 16;
	private static int methodId_getGeneratedKeys = 17;
	private static int methodId_getMaxFieldSize = 18;
	private static int methodId_getMaxRows = 19;
	private static int methodId_getMoreResults_V = 20;
	private static int methodId_getMoreResults_I = 21;
	private static int methodId_getQueryTimeout = 22;
	private static int methodId_getResultSet = 23;
	private static int methodId_getResultSetConcurrency = 24;
	private static int methodId_getResultSetHoldability = 25;
	private static int methodId_getResultSetType = 26;
	private static int methodId_getUpdateCount = 27;
	private static int methodId_setCursorName = 28;
	private static int methodId_setEscapeProcessing = 29;
	private static int methodId_setFetchDirection = 30;
	private static int methodId_setFetchSize = 31;
	private static int methodId_setMaxFieldSize = 32;
	private static int methodId_setMaxRows = 33;
	private static int methodId_setQueryTimeout = 34;
	private static int methodId_getStmtSqlType = 35;
	private static int methodId_validateExecDirectInvocation_L = 37;
	private static int methodId_validateExecDirectInvocation_V = 38;
	private static int methodId_internalClose = 39;
	private static int methodId_setExecDirectOutputs = 40;
	private static int methodId_setExecDirectBatchOutputs = 41;
	private static int methodId_setCurrentTxid = 42;
	private static int methodId_SQLMXStatement_V = 43;
	private static int methodId_SQLMXStatement_LIII = 44;
	private static int methodId_setExecRSOutputs = 45;
	private static int methodId_closeAllResultSets = 46;
	private static int methodId_firstResultSetExists = 47;
	private static int methodId_setCurrentStmtId = 48;
	private static int totalMethodIds = 49;
	private static JdbcDebug[] debug;

	static {
		String className = "SQLMXStatement";
		if (JdbcDebugCfg.entryActive) {
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_addBatch] = new JdbcDebug(className, "addBatch");
			debug[methodId_cancel] = new JdbcDebug(className, "cancel");
			debug[methodId_clearBatch] = new JdbcDebug(className, "clearBatch");
			debug[methodId_close] = new JdbcDebug(className, "close");
			debug[methodId_execute_L] = new JdbcDebug(className, "execute[L]");
			debug[methodId_execute_LI_auto] = new JdbcDebug(className,
					"execute[LI_auto]");
			debug[methodId_execute_LI_col] = new JdbcDebug(className,
					"execute[LI_col]");
			debug[methodId_execute_LL] = new JdbcDebug(className, "execute[LL]");
			debug[methodId_executeBatch] = new JdbcDebug(className,
					"executeBatch");
			debug[methodId_executeQuery] = new JdbcDebug(className,
					"executeQuery");
			debug[methodId_executeUpdate_L] = new JdbcDebug(className,
					"executeUpdate[L]");
			debug[methodId_executeUpdate_LI] = new JdbcDebug(className,
					"executeUpdate[LI]");
			debug[methodId_executeUpdate_LI_array] = new JdbcDebug(className,
					"executeUpdate[LI_array]");
			debug[methodId_executeUpdate_LL] = new JdbcDebug(className,
					"executeUpdate[LL]");
			debug[methodId_getConnection] = new JdbcDebug(className,
					"getConnection");
			debug[methodId_getFetchDirection] = new JdbcDebug(className,
					"getFetchDirection");
			debug[methodId_getFetchSize] = new JdbcDebug(className,
					"getFetchSize");
			debug[methodId_getGeneratedKeys] = new JdbcDebug(className,
					"getGeneratedKeys");
			debug[methodId_getMaxFieldSize] = new JdbcDebug(className,
					"getMaxFieldSize");
			debug[methodId_getMaxRows] = new JdbcDebug(className, "getMaxRows");
			debug[methodId_getMoreResults_V] = new JdbcDebug(className,
					"getMoreResults[V]");
			debug[methodId_getMoreResults_I] = new JdbcDebug(className,
					"getMoreResults[I]");
			debug[methodId_getQueryTimeout] = new JdbcDebug(className,
					"getQueryTimeout");
			debug[methodId_getResultSet] = new JdbcDebug(className,
					"getResultSet");
			debug[methodId_getResultSetConcurrency] = new JdbcDebug(className,
					"getResultSetConcurrency");
			debug[methodId_getResultSetHoldability] = new JdbcDebug(className,
					"getResultSetHoldability");
			debug[methodId_getResultSetType] = new JdbcDebug(className,
					"getResultSetType");
			debug[methodId_getUpdateCount] = new JdbcDebug(className,
					"getUpdateCount");
			debug[methodId_setCursorName] = new JdbcDebug(className,
					"setCursorName");
			debug[methodId_setEscapeProcessing] = new JdbcDebug(className,
					"setEscapeProcessing");
			debug[methodId_setFetchDirection] = new JdbcDebug(className,
					"setFetchDirection");
			debug[methodId_setFetchSize] = new JdbcDebug(className,
					"setFetchSize");
			debug[methodId_setMaxFieldSize] = new JdbcDebug(className,
					"setMaxFieldSize");
			debug[methodId_setMaxRows] = new JdbcDebug(className, "setMaxRows");
			debug[methodId_setQueryTimeout] = new JdbcDebug(className,
					"setQueryTimeout");
			debug[methodId_getStmtSqlType] = new JdbcDebug(className,
					"getStmtSqlType");
			debug[methodId_validateExecDirectInvocation_L] = new JdbcDebug(
					className, "validateExecDirectInvocation[L]");
			debug[methodId_validateExecDirectInvocation_V] = new JdbcDebug(
					className, "validateExecDirectInvocation[V]");
			debug[methodId_internalClose] = new JdbcDebug(className,
					"internalClose");
			debug[methodId_setExecDirectOutputs] = new JdbcDebug(className,
					"setExecDirectOutputs");
			debug[methodId_setExecRSOutputs] = new JdbcDebug(className,
					"setExecRSOutputs");
			debug[methodId_setExecDirectBatchOutputs] = new JdbcDebug(
					className, "setExecDirectBatchOutputs");
			debug[methodId_setCurrentTxid] = new JdbcDebug(className,
					"setCurrentTxid");
			debug[methodId_SQLMXStatement_V] = new JdbcDebug(className,
					"SQLMXStatement_V");
			debug[methodId_SQLMXStatement_LIII] = new JdbcDebug(className,
					"SQLMXStatement_LIII");
			debug[methodId_closeAllResultSets] = new JdbcDebug(className,
					"closeAllResultSets");
			debug[methodId_firstResultSetExists] = new JdbcDebug(className,
					"firstResultSetExists");
			debug[methodId_setCurrentStmtId] = new JdbcDebug(className,
			"setCurrentstmtId");
		}
	}
        public Object unwrap(Class iface) throws SQLException {
                // TODO Auto-generated method stub
                return null;
        }

        public boolean isWrapperFor(Class iface) throws SQLException {
                // TODO Auto-generated method stub
                return false;
        }

        public boolean isClosed() throws SQLException {
                // TODO Auto-generated method stub
                return isClosed_;
        }

        public void setPoolable(boolean poolable) throws SQLException {
                // TODO Auto-generated method stub

        }
        public boolean isPoolable() throws SQLException {
                // TODO Auto-generated method stub
                return false;
        }

        public void closeOnCompletion() throws SQLException {
                // TODO Auto-generated method stub

        }

        public boolean isCloseOnCompletion() throws SQLException {
                // TODO Auto-generated method stub
                return false;
        }


}
