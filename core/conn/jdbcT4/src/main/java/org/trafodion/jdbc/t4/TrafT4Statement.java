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

import java.lang.ref.WeakReference;
import java.math.BigDecimal;
import java.nio.ByteBuffer;
import java.sql.BatchUpdateException;
import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.logging.Level;
import java.util.logging.LogRecord;

/**
 * <p>
 * JDBC Type 4 TrafT4Statement class.
 * </p>
 * <p>
 * Description: The <code>TrafT4Statement</code> class is an implementation of
 * the <code>java.sql.Statement</code> interface.
 * </p>
 */
public class TrafT4Statement extends TrafT4Handle implements java.sql.Statement {
	// java.sql.Statement interface Methods
	T4Properties getT4props() {
		return connection_.props_;
	}


	public void addBatch(String sql) throws SQLException {
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("addBatch");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "addBatch", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("addBatch");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		if (batchCommands_ == null) {
			batchCommands_ = new ArrayList();
		}


		batchCommands_.add(sql);
	}

	public void cancel() throws SQLException 
        {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "cancel", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("cancel");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
        	}
		// Donot clear warning, since the warning may pertain to
		// previous opertation and it is not yet seen by the application
		//

		// if the statement is already closed
		// No need to cancel the statement
		
		if (isClosed_) 
			return;

		// Check if the statement is stuck in reading from socket connection to mxosrvr
		// If not, just do the internal close to free up the statement resources on the server side
		// else let the query time mechanism in the socket connection read take care of 
		// cancelling the query
		if ((ist_.getT4statement() != null && (! ist_.getT4statement().isProcessing()))
				|| (resultSet_ != null && resultSet_[result_set_offset] != null
						&& resultSet_[result_set_offset].irs_ != null
						&& resultSet_[result_set_offset].irs_.t4resultSet_ != null && 
						( ! resultSet_[result_set_offset].irs_.t4resultSet_.m_processing)))
			internalClose();
	}

	public void clearBatch() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "clearBatch", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("clearBatch");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		batchCommands_.clear();
	}

	/**
	 * Closes the statement object. Synchronized to prevent the same resource
	 * issued free command twice on the server.
	 * 
	 * @throws SQLException
	 */
	synchronized public void close() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "close", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("close");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		if (isClosed_) {
			return;
		}

		try {
			if (connection_._isClosed() == false) {
				for (int i = 0; i < num_result_sets_; i++) {
					if (resultSet_[i] != null) {
						resultSet_[i].close(false);
					}
				}
				ist_.close();
			}
		} finally {
			isClosed_ = true;
			connection_.removeElement(pRef_);
			initResultSets();
		}
	}

	void initResultSets() {
		num_result_sets_ = 1;
		result_set_offset = 0;
		resultSet_[result_set_offset] = null;
	}

	// ------------------------------------------------------------------
	/**
	 * This method will execute an operation.
	 * 
	 * @return true
	 * 
	 * @exception A
	 *                SQLException is thrown
	 */

	public boolean execute() throws SQLException {
		try {
			ist_.executeDirect(queryTimeout_, this);
		} catch (SQLException se) {
			performConnectionErrorChecks(se);
			throw se;
		}
		return true;
	} // end execute

	// ------------------------------------------------------------------

	public boolean execute(String sql) throws SQLException {
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("execute");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "execute", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("execute");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateExecDirectInvocation(sql);
		try {
			ist_.execute(TRANSPORT.SRVR_API_SQLEXECDIRECT, 0, 0, null, queryTimeout_, sql_, this);

			checkSQLWarningAndClose();
		} catch (SQLException se) {
			try {
				if (num_result_sets_ == 1 && resultSet_[result_set_offset] == null)
					;
				{
					internalClose();
				}
			} catch (SQLException closeException) {
				se.setNextException(closeException);
			}
			performConnectionErrorChecks(se);
			throw se;
		}
		if (resultSet_[result_set_offset] != null) {
			return true;
		} else {
			return false;
		}
	}

	public boolean execute(String sql, int autoGeneratedKeys) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql, autoGeneratedKeys);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "execute", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql, autoGeneratedKeys);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("execute");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		boolean ret;

		if (autoGeneratedKeys == TrafT4Statement.NO_GENERATED_KEYS) {
			ret = execute(sql);
		} else {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"auto_generated_keys_not_supported", null);
		}
		return ret;
	}

	public boolean execute(String sql, int[] columnIndexes) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql, columnIndexes);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "execute", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql, columnIndexes);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("execute");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		boolean ret;

		if (columnIndexes == null) {
			ret = execute(sql);
		} else if (columnIndexes.length == 0) {
			ret = execute(sql);
		} else {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"auto_generated_keys_not_supported", null);
		}
		return ret;
	}

	public boolean execute(String sql, String[] columnNames) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql, columnNames);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "execute", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql, columnNames);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("execute");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		boolean ret;

		if (columnNames == null) {
			ret = execute(sql);
		} else if (columnNames.length == 0) {
			ret = execute(sql);
		} else {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"auto_generated_keys_not_supported", null);
		}
		return ret;
	}

	public int[] executeBatch() throws SQLException, BatchUpdateException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "executeBatch", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("executeBatch");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		try {
			int i = 0;
			SQLException se;

			validateExecDirectInvocation();
			if ((batchCommands_ == null) || (batchCommands_.isEmpty())) {
				return new int[]

				{};
			}

			for (i = 0; i < batchCommands_.size(); i++) {
				String sql = (String) batchCommands_.get(i);

				if (sql == null) {
					se = TrafT4Messages.createSQLException(connection_.props_, this.ist_.ic_.getLocale(),
							"batch_command_failed", "Invalid SQL String");
					throw new BatchUpdateException(se.getMessage(), se.getSQLState(), new int[0]);
				}

				sqlStmtType_ = ist_.getSqlStmtType(sql);
			}

			Object[] commands = batchCommands_.toArray();
			int[] batchRowCount = new int[commands.length];
			String sql;
			int rowCount = 0;

			try {
				for (i = 0; i < commands.length; i++) {
					sql = String.valueOf(commands[i]);

					validateExecDirectInvocation(sql);

					ist_.execute(TRANSPORT.SRVR_API_SQLEXECDIRECT, 0, 0, null, queryTimeout_, sql_, this);

					checkSQLWarningAndClose();

					batchRowCount[i] = (int)ist_.getRowCount(); // the member will
					// be set by
					// execute...keep
					// them in our local
					// array
					rowCount += ist_.getRowCount();
				}
				// CTS requirement.
				if (commands.length < 1) {
					batchRowCount = new int[] {};
				}
			} catch (SQLException e) {
				ist_.setRowCount(rowCount);
				batchRowCount_ = new int[i];
				System.arraycopy(batchRowCount, 0, batchRowCount_, 0, i);

				BatchUpdateException be;

				se = TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
						"batch_command_failed", e.getMessage());
				be = new BatchUpdateException(se.getMessage(), se.getSQLState(), batchRowCount_);
				be.setNextException(e);

				try {
					if (resultSet_[result_set_offset] == null) {
						internalClose();
					}
				} catch (SQLException closeException) {
					be.setNextException(closeException);
				}
				performConnectionErrorChecks(e);

				throw be;
			}

			ist_.setRowCount(rowCount);
			batchRowCount_ = new int[i];
			System.arraycopy(batchRowCount, 0, batchRowCount_, 0, i);
			return batchRowCount_;
		} finally {
			clearBatch();
		}

	}

	public ResultSet executeQuery(String sql) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "executeQuery", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("executeQuery");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		validateExecDirectInvocation(sql);
		try {
			ist_.execute(TRANSPORT.SRVR_API_SQLEXECDIRECT, 0, 0, null, queryTimeout_, sql_, this);

			checkSQLWarningAndClose();
		} catch (SQLException se) {
			try {
				if (resultSet_[result_set_offset] == null) {
					internalClose();
				}
			} catch (SQLException closeException) {
				se.setNextException(closeException);
			}
			performConnectionErrorChecks(se);
			throw se;
		}
		return resultSet_[result_set_offset];
	}

	public int executeUpdate(String sql) throws SQLException {
		long count = executeUpdate64(sql);

		if (count > Integer.MAX_VALUE)
			this.setSQLWarning(null, "numeric_out_of_range", null);

		return (int) count;
	}

	public long executeUpdate64(String sql) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "executeUpdate", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("executeUpdate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateExecDirectInvocation(sql);
		try {
			ist_.execute(TRANSPORT.SRVR_API_SQLEXECDIRECT, 0, 0, null, queryTimeout_, sql_, this);

			checkSQLWarningAndClose();
		} catch (SQLException se) {
			try {
				if (resultSet_[result_set_offset] == null) {
					internalClose();
				}
			} catch (SQLException closeException) {
				se.setNextException(closeException);
			}
			performConnectionErrorChecks(se);
			throw se;
		}
		return ist_.getRowCount();
	}

	public int executeUpdate(String sql, int autoGeneratedKeys) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql, autoGeneratedKeys);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "executeUpdate", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql, autoGeneratedKeys);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("executeUpdate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int ret;

		if (autoGeneratedKeys == TrafT4Statement.NO_GENERATED_KEYS) {
			ret = executeUpdate(sql);
		} else {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"auto_generated_keys_not_supported", null);
		}
		return ret;
	}

	public int executeUpdate(String sql, int[] columnIndexes) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql, columnIndexes);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "executeUpdate", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql, columnIndexes);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("executeUpdate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int ret;

		if (columnIndexes == null) {
			ret = executeUpdate(sql);
		} else if (columnIndexes.length == 0) {
			ret = executeUpdate(sql);
		} else {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"auto_generated_keys_not_supported", null);
		}
		return ret;
	}

	public int executeUpdate(String sql, String[] columnNames) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql, columnNames);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "executeUpdate", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql, columnNames);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("executeUpdate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int ret;

		if (columnNames == null) {
			ret = executeUpdate(sql);
		} else if (columnNames.length == 0) {
			ret = executeUpdate(sql);
		} else {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"auto_generated_keys_not_supported", null);
		}
		return ret;
	}

	public Connection getConnection() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "getConnection", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("getConnection");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		return connection_;
	}

	public int getFetchDirection() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "getFetchDirection", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("getFetchDirection");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		return fetchDirection_;
	}

	public int getFetchSize() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "getFetchSize", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("getFetchSize");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		return fetchSize_;
	}

	public ResultSet getGeneratedKeys() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "getGeneratedKeys", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("getGeneratedKeys");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
				"auto_generated_keys_not_supported", null);
	}

	public int getMaxFieldSize() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "getMaxFieldSize", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("getMaxFieldSize");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		return maxFieldSize_;
	}

	public int getMaxRows() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "getMaxRows", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("getMaxRows");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		return maxRows_;
	}

	public boolean getMoreResults() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "getMoreResults", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("getMoreResults");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		return getMoreResults(Statement.CLOSE_CURRENT_RESULT);
	}

	public boolean getMoreResults(int current) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, current);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "getMoreResults", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, current);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("getMoreResults");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		switch (current) {
		case Statement.CLOSE_ALL_RESULTS:
			for (int i = 0; i <= result_set_offset; i++) {
				if (resultSet_[i] != null) {
					resultSet_[i].close();
				}
			}
			break;
		case Statement.KEEP_CURRENT_RESULT:
			break;
		case Statement.CLOSE_CURRENT_RESULT: // this is the default action
		default:
			if (resultSet_[result_set_offset] != null) {
				resultSet_[result_set_offset].close();
			}
			break;
		}
		ist_.setRowCount(-1);
		if (result_set_offset < num_result_sets_ - 1) {
			result_set_offset++;
			return true;
		}
		return false;
	}

	public int getQueryTimeout() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "getQueryTimeout", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("getQueryTimeout");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		return queryTimeout_;
	}

	public ResultSet getResultSet() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "getResultSet", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("getResultSet");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		return resultSet_[result_set_offset];
	}

	public int getResultSetConcurrency() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "getResultSetConcurrency", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("getResultSetConcurrency");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		return resultSetConcurrency_;
	}

	public int getResultSetHoldability() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "getResultSetHoldability", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("getResultSetHoldability");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		return resultSetHoldability_;
	}

	public int getResultSetType() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "getResultSetType", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("getResultSetType");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		return resultSetType_;
	}

	public int getUpdateCount() throws SQLException {
		long count = getUpdateCount64();

		if (count > Integer.MAX_VALUE)
			this.setSQLWarning(null, "numeric_out_of_range", null);

		return (int) count;
	}

	public long getUpdateCount64() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "getUpdateCount", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("getUpdateCount");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		if (ist_ == null) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_statement_handle", null);
		}

		// Spec wants -1 when the resultset is current and no more rows.
		long count = ist_.getRowCount();
		if ((count == 0) && resultSet_ != null && resultSet_[result_set_offset] != null) {
			count = -1;
		}

		return count;
	}

	// ------------------------------------------------------------------
	/**
	 * This method will get the operation ID for this statement. -1 is returned
	 * if the operation ID has not been set.
	 * 
	 * @retrun The operation ID or -1 if the operation ID has not been set.
	 * 
	 * @exception A
	 *                SQLException is thrown
	 */
	public short getOperationID() throws SQLException {
		return operationID_;
	} // end getOperationID

	// ------------------------------------------------------------------
	/**
	 * This method will get the operation buffer for this statement. Null is
	 * returned if the operation buffer has not been set.
	 * 
	 * @retrun The operation buffer or null if the operation ID has not been
	 *         set.
	 * 
	 * @exception A
	 *                SQLException is thrown
	 */
	public byte[] getOperationBuffer() throws SQLException {
		// System.out.println("in getOperation");
		return operationBuffer_;
	}

	// ------------------------------------------------------------------
	/**
	 * This method will get the operation reply buffer for this statement. Null
	 * is returned if the operation reply buffer has not been set.
	 * 
	 * @retrun The operation reply buffer or null.
	 * 
	 * @exception A
	 *                SQLException is thrown
	 */
	public byte[] getOperationReplyBuffer() throws SQLException {
		// System.out.println("in getOperationReplyBuffer");
		return operationReply_;
	}

	// ------------------------------------------------------------------

	public void setCursorName(String name) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, name);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "setCursorName", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, name);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("setCursorName");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		// TODO: May need to check the Statement STATE
		cursorName_ = name;
	}

	public void setEscapeProcessing(boolean enable) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, enable);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "setEscapeProcessing", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, enable);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("setEscapeProcessing");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		escapeProcess_ = enable;

	}

	public void setFetchDirection(int direction) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, direction);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "setFetchDirection", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, direction);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("setFetchDirection");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		switch (direction) {
		case ResultSet.FETCH_FORWARD:
			fetchDirection_ = direction;
			break;
		case ResultSet.FETCH_REVERSE:
		case ResultSet.FETCH_UNKNOWN:
			fetchDirection_ = ResultSet.FETCH_FORWARD;
			break;
		default:
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_fetch_direction", null);
		}
	}

	public void setFetchSize(int rows) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, rows);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "setFetchSize", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, rows);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("setFetchSize");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		if (rows < 0) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_fetch_size",
					null);
		} else if (rows == 0) {
			fetchSize_ = TrafT4ResultSet.DEFAULT_FETCH_SIZE;
		} else {
			fetchSize_ = rows;
		}
	}

	public void setMaxFieldSize(int max) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, max);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "setMaxFieldSize", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, max);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("setMaxFieldSize");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		if (max < 0) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_maxFieldSize_value", null);
		}
		maxFieldSize_ = max;
	}

	public void setMaxRows(int max) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, max);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "setMaxRows", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, max);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("setMaxRows");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		if (max < 0) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_maxRows_value",
					null);
		}
		maxRows_ = max;
	}

	public void setQueryTimeout(int seconds) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, seconds);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "setQueryTimeout", "", p);
		}
		if ( getT4props().t4Logger_.isLoggable(Level.FINE) && getT4props().getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, seconds);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("setQueryTimeout");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		//TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "setQueryTimeout()");
		
		if (seconds < 0) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_queryTimeout_value", null);
		}
		queryTimeout_ = seconds;
	}

	// ------------------------------------------------------------------
	/**
	 * This method will set the operation ID for this statement.
	 * 
	 * @param opID
	 *            the operation ID to associate with this statement.
	 * 
	 * @exception A
	 *                SQLException is thrown
	 */
	public void setOperationID(short opID) throws SQLException {
		operationID_ = opID;
	} // end setOperationID

	// ------------------------------------------------------------------
	/**
	 * This method will set the operation buffer for this statement.
	 * 
	 * @param The
	 *            operation buffer.
	 * 
	 * @exception A
	 *                SQLException is thrown
	 */
	public void setOperationBuffer(byte[] opBuffer) throws SQLException {
		operationBuffer_ = opBuffer;
	}


	void validateExecDirectInvocation(String sql) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4Statement", "validateExecDirectInvocation", "", p);
		}

		validateExecDirectInvocation();
		sqlStmtType_ = ist_.getSqlStmtType(sql);
                sql_ = sql;

	}

	void validateExecDirectInvocation() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4Statement", "validateExecDirectInvocation", "", p);
		}
		ist_.setRowCount(-1);
		clearWarnings();
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_statement",
					null);
		}
		try {
			// connection_.getServerHandle().isConnectionOpen();
			connection_.isConnectionOpen();

			// close the previous resultset, if any
			for (int i = 0; i < num_result_sets_; i++) {
				if (resultSet_[i] != null) {
					resultSet_[i].close();
				}
			}
		} catch (SQLException se) {
			performConnectionErrorChecks(se);
			throw se;
		}
	}
	
	// This functions ensure that Database Resources are cleaned up,
	// but leave the java Statement object
	// intact.
	void internalClose() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4Statement", "internalClose", "", p);
		}
		if (connection_._isClosed() == false) {
			ist_.close();
		}
	}

	private void setResultSet(TrafT4Desc[] outputDesc) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, outputDesc);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4Statement", "setResultSet", "", p);
		}
		initResultSets();
		if (outputDesc != null) {
			resultSet_[result_set_offset] = new TrafT4ResultSet(this, outputDesc);
		} else {
			resultSet_[result_set_offset] = null;
		}
	}

	public void setTransactionToJoin(byte[] txid) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, txid);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4Statement", "setTransactionToJoin", "", p);
		}

		this.transactionToJoin = txid;
	}

	void setMultipleResultSets(int num_result_sets, TrafT4Desc[][] output_descriptors, String[] stmt_labels,
			String[] proxySyntax) throws SQLException {
		if (num_result_sets < 1)
			return;

		resultSet_ = new TrafT4ResultSet[num_result_sets];
		num_result_sets_ = num_result_sets;
		for (int i = 0; i < num_result_sets; i++) {
			TrafT4Desc[] desc = output_descriptors[i];
			if (desc == null) {
				resultSet_[i] = null;
			} else {
				resultSet_[i] = new TrafT4ResultSet(this, desc, stmt_labels[i], 
                                        (ist_.getSqlQueryType() == TRANSPORT.SQL_CALL_WITH_RESULT_SETS ||
                                         ist_.getSqlQueryType() == TRANSPORT.SQL_CALL_NO_RESULT_SETS));
				resultSet_[i].proxySyntax_ = proxySyntax[i];
			}
		}
	}

	// ----------------------------------------------------------------------------------
	void setExecute2Outputs(byte[] values, short rowsAffected, boolean endOfData, String[] proxySyntax, TrafT4Desc[] desc)
			throws SQLException {
		num_result_sets_ = 1;
		result_set_offset = 0;

		// if NO DATA FOUND is returned from the server, desc = null but
		// we still want to save our descriptors from PREPARE
		if (desc != null)
			outputDesc_ = desc;

		resultSet_ = new TrafT4ResultSet[num_result_sets_];

		if (outputDesc_ != null) {
			resultSet_[result_set_offset] = new TrafT4ResultSet(this, outputDesc_);
			resultSet_[result_set_offset].proxySyntax_ = proxySyntax[result_set_offset];

			if (rowsAffected == 0) {
				if (endOfData == true) {
					resultSet_[result_set_offset].setFetchOutputs(new ObjectArray[0], 0, true);
				}
			} else {
				 if(resultSet_[result_set_offset].keepRawBuffer_ == true)
			          resultSet_[result_set_offset].rawBuffer_ = values;
				//if setExecute2FetchOutputs is not called by fetch set flag to false , data has not been clipped 
				resultSet_[result_set_offset].irs_.setExecute2FetchOutputs(resultSet_[result_set_offset], 1, true,
						values,false);
			}
		} else {
			resultSet_[result_set_offset] = null;
		}
	}

	// Constructors with access specifier as "default"
	TrafT4Statement() {
		if (T4Properties.t4GlobalLogger.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(null);
			T4Properties.t4GlobalLogger.logp(Level.FINE, "TrafT4Statement", "<init>", "", p);
		}
		resultSet_ = new TrafT4ResultSet[1];
		initResultSets();
	}

	/*
	 * * For closing statements using label.
	 */
	TrafT4Statement(TrafT4Connection connection, String stmtLabel) throws SQLException {
		if (connection.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection.props_, connection, stmtLabel);
			connection.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "<init>", "", p);
		}
		if ( connection.props_.t4Logger_.isLoggable(Level.FINE) && connection.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection.props_, connection);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("Note, this constructor was called before the previous constructor");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection.props_.getLogWriter().println(temp);
		}
		int hashcode;

		connection_ = connection;
		operationID_ = -1;

		resultSetType_ = ResultSet.TYPE_FORWARD_ONLY;
		resultSetConcurrency_ = ResultSet.CONCUR_READ_ONLY;
		resultSetHoldability_ = TrafT4ResultSet.CLOSE_CURSORS_AT_COMMIT;
		queryTimeout_ = connection_.getServerHandle().getQueryTimeout();

		stmtLabel_ = stmtLabel;
		fetchSize_ = TrafT4ResultSet.DEFAULT_FETCH_SIZE;
		maxRows_ = 0;
		fetchDirection_ = ResultSet.FETCH_FORWARD;
		pRef_ = new WeakReference(this, connection_.refStmtQ_);
		ist_ = new InterfaceStatement(this);
		connection_.addElement(pRef_, stmtLabel_);

		resultSet_ = new TrafT4ResultSet[1];
		initResultSets();
	}

	TrafT4Statement(TrafT4Connection connection) throws SQLException {
		this(connection, ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY, TrafT4ResultSet.CLOSE_CURSORS_AT_COMMIT);
		if (connection.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection.props_, connection);
			connection.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "<init>",
					"Note, this constructor was called before the previous constructor", p);
		}
		if ( connection.props_.t4Logger_.isLoggable(Level.FINE) && connection.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection.props_, connection);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("<init>");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection.props_.getLogWriter().println(temp);
		}
		resultSet_ = new TrafT4ResultSet[1];
		roundingMode_ = connection_.props_.getRoundingMode();
		initResultSets();
	}

	TrafT4Statement(TrafT4Connection connection, int resultSetType, int resultSetConcurrency) throws SQLException {
		this(connection, resultSetType, resultSetConcurrency, TrafT4ResultSet.CLOSE_CURSORS_AT_COMMIT);
		if (connection.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection.props_, connection, resultSetType,
					resultSetConcurrency);
			connection.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "<init>",
					"Note, this constructor was called before the previous constructor", p);
		}
		if ( connection.props_.t4Logger_.isLoggable(Level.FINE) && connection.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection.props_, connection, resultSetType,
					resultSetConcurrency);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("<init>");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection.props_.getLogWriter().println(temp);
		}
		resultSet_ = new TrafT4ResultSet[1];
		roundingMode_ = connection_.props_.getRoundingMode();
		initResultSets();
	}
	TrafT4Statement(TrafT4Connection connection, int resultSetType, int resultSetConcurrency, int resultSetHoldability,
			String stmtLabel) throws SQLException {
		if (connection.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection.props_, connection, resultSetType,
					resultSetConcurrency, resultSetHoldability);
			connection.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "<init>", "", p);
		}
		if ( connection.props_.t4Logger_.isLoggable(Level.FINE) && connection.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection.props_, connection, resultSetType,
					resultSetConcurrency, resultSetHoldability);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("<init>");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection.props_.getLogWriter().println(temp);
		}
		int hashcode;

		connection_ = connection;
		operationID_ = -1;

		if (resultSetType != ResultSet.TYPE_FORWARD_ONLY && resultSetType != ResultSet.TYPE_SCROLL_INSENSITIVE
				&& resultSetType != ResultSet.TYPE_SCROLL_SENSITIVE) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_resultset_type", null);
		}

		if (resultSetType == ResultSet.TYPE_SCROLL_SENSITIVE) {
			resultSetType_ = ResultSet.TYPE_SCROLL_INSENSITIVE;
			connection_.setSQLWarning(null, "scrollResultSetChanged", null);
			//setSQLWarning(null, "scrollResultSetChanged", null);
		} else {
			resultSetType_ = resultSetType;
		}
		if (resultSetConcurrency != ResultSet.CONCUR_READ_ONLY && resultSetConcurrency != ResultSet.CONCUR_UPDATABLE) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_resultset_concurrency", null);
		}

		if ((resultSetHoldability != 0) && (resultSetHoldability != ResultSet.CLOSE_CURSORS_AT_COMMIT)
				&& (resultSetHoldability != ResultSet.HOLD_CURSORS_OVER_COMMIT)) {
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_holdability",
					null);
		}

		resultSetConcurrency_ = resultSetConcurrency;
		resultSetHoldability_ = resultSetHoldability;
		queryTimeout_ = connection_.getServerHandle().getQueryTimeout();

		stmtLabel_ = stmtLabel;
		fetchSize_ = TrafT4ResultSet.DEFAULT_FETCH_SIZE;
		maxRows_ = 0;
		fetchDirection_ = ResultSet.FETCH_FORWARD;

		connection_.gcStmts();
		pRef_ = new WeakReference(this, connection_.refStmtQ_);
		ist_ = new InterfaceStatement(this);
		connection_.addElement(pRef_, stmtLabel_);
		roundingMode_ = connection_.props_.getRoundingMode();

		resultSet_ = new TrafT4ResultSet[1];
	}

	TrafT4Statement(TrafT4Connection connection, int resultSetType, int resultSetConcurrency, int resultSetHoldability)
			throws SQLException {
		if (connection.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection.props_, connection, resultSetType,
					resultSetConcurrency, resultSetHoldability);
			connection.props_.t4Logger_.logp(Level.FINE, "TrafT4Statement", "<init>", "", p);
		}
		if ( connection.props_.t4Logger_.isLoggable(Level.FINE) && connection.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection.props_, connection, resultSetType,
					resultSetConcurrency, resultSetHoldability);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Statement");
			lr.setSourceMethodName("<init>");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection.props_.getLogWriter().println(temp);
		}
		int hashcode;

		connection_ = connection;
		operationID_ = -1;

		if (resultSetType != ResultSet.TYPE_FORWARD_ONLY && resultSetType != ResultSet.TYPE_SCROLL_INSENSITIVE
				&& resultSetType != ResultSet.TYPE_SCROLL_SENSITIVE) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_resultset_type", null);
		}

		if (resultSetType == ResultSet.TYPE_SCROLL_SENSITIVE) {
			resultSetType_ = ResultSet.TYPE_SCROLL_INSENSITIVE;
			connection_.setSQLWarning(null, "scrollResultSetChanged", null);
			//setSQLWarning(null, "scrollResultSetChanged", null);
		} else {
			resultSetType_ = resultSetType;
		}
		if (resultSetConcurrency != ResultSet.CONCUR_READ_ONLY && resultSetConcurrency != ResultSet.CONCUR_UPDATABLE) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_resultset_concurrency", null);
		}

		if ((resultSetHoldability != 0) && (resultSetHoldability != ResultSet.CLOSE_CURSORS_AT_COMMIT)
				&& (resultSetHoldability != ResultSet.HOLD_CURSORS_OVER_COMMIT)) {
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_holdability",
					null);
		}

		resultSetConcurrency_ = resultSetConcurrency;
		resultSetHoldability_ = resultSetHoldability;
		queryTimeout_ = connection_.getServerHandle().getQueryTimeout();

		stmtLabel_ = generateStmtLabel();
		fetchSize_ = TrafT4ResultSet.DEFAULT_FETCH_SIZE;
		maxRows_ = 0;
		fetchDirection_ = ResultSet.FETCH_FORWARD;

		connection_.gcStmts();
		pRef_ = new WeakReference(this, connection_.refStmtQ_);
		ist_ = new InterfaceStatement(this);
		connection_.addElement(pRef_, stmtLabel_);

		resultSet_ = new TrafT4ResultSet[1];
		roundingMode_ = connection_.props_.getRoundingMode();
		initResultSets();
	}

	//max length for a label is 32 characters.
	String generateStmtLabel() {
		String id = String.valueOf(this.connection_.ic_.getSequenceNumber());
		if(id.length() > 24) {
			id = id.substring(id.length()-24);
		}
	
		return "SQL_CUR_" + id;
	}
	
	// Database statement are not deallocated when there is a
	// SQLWarning or SQLException or when a resultSet is produced
	void checkSQLWarningAndClose() {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4Statement", "checkSQLWarningAndClose", "", p);
		}
		if (sqlWarning_ != null) {
			if (resultSet_[result_set_offset] == null) {
				try {
					internalClose();
				} catch (SQLException closeException1) {
				}
			}
		}
	}

	public void setRoundingMode(int roundingMode) {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, roundingMode);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setRoundingMode", "", p);
		}
		roundingMode_ = Utility.getRoundingMode(roundingMode);
	}

	public void setRoundingMode(String roundingMode) {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, roundingMode);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setRoundingMode", "", p);
		}
		roundingMode_ = Utility.getRoundingMode(roundingMode);
	}

	void closeErroredConnection(TrafT4Exception sme) {
		connection_.closeErroredConnection(sme);
	}

	/**
	 * Use this method to retrieve the statement-label name that was used when
	 * creating the statement through the Trafodion connectivity service. You can
	 * subsequently use the name retrieved as the cursor name when invoking
	 * INFOSTATS to gather resource statistics through either the
	 * <code>executeQuery(String sql)</code> or
	 * <code>execute(String sql)</code> methods.
	 */
	public String getStatementLabel() {
		return new String(stmtLabel_);
	}

	/**
	 * Returns the raw SQL associated with the statement
	 * 
	 * @return the SQL text
	 */
	public String getSQL() {
		return this.sql_;
	}

	/**
	 * Returns the MXCS statement handle
	 * 
	 * @return the MXCS statement handle
	 */
	public int getStmtHandle() {
		return this.ist_.stmtHandle_;
	}

	// static fields
	public static final int NO_GENERATED_KEYS = 2;
	// Fields
	TrafT4Connection connection_;
	int resultSetType_;
	int resultSetConcurrency_;
	String sql_;
	int queryTimeout_;
	int maxRows_;
	int maxFieldSize_;
	int fetchSize_;
	int fetchDirection_;
	boolean escapeProcess_;
	String cursorName_ = "";
	TrafT4ResultSet[] resultSet_; // Added for SPJ RS - SB 11/21/2005
	int num_result_sets_; // Added for SPJ RS - SB 11/21/2005
	int result_set_offset; // Added for SPJ RS - SB 11/21/2005
	String stmtLabel_;
	short sqlStmtType_;
	boolean isClosed_;
	ArrayList batchCommands_;
	int[] batchRowCount_;
	WeakReference pRef_;
	int resultSetHoldability_;
	InterfaceStatement ist_;

	int inputParamsLength_;
	int outputParamsLength_;
	int inputDescLength_;
	int outputDescLength_;

	int inputParamCount_;
	int outputParamCount_;

	int roundingMode_ = BigDecimal.ROUND_HALF_EVEN;

	TrafT4Desc[] inputDesc_, outputDesc_;

	short operationID_;
	byte[] operationBuffer_;
	byte[] operationReply_;

	boolean usingRawRowset_;
	ByteBuffer rowwiseRowsetBuffer_;

	byte[] transactionToJoin;
	
	int _lastCount = -1;

	/**
	 * @return the inputParamsLength_
	 */
	public int getInputParamsLength_() {
		return inputParamsLength_;
	}

	/**
	 * @return the outputParamsLength_
	 */
	public int getOutputParamsLength_() {
		return outputParamsLength_;
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
