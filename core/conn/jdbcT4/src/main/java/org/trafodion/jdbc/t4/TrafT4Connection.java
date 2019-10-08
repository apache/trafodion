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

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;
import java.sql.Array;
import java.sql.Blob;
import java.sql.CallableStatement;
import java.sql.Clob;
import java.sql.DatabaseMetaData;
import java.sql.NClob;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLClientInfoException;
import java.sql.SQLException;
import java.sql.SQLXML;
import java.sql.Savepoint;
import java.sql.Statement;
import java.sql.Struct;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Locale;
import java.util.Map;
import java.util.Properties;
import java.util.concurrent.Executor;
import java.util.logging.FileHandler;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;
import java.util.HashMap;
import javax.sql.PooledConnection;

/**
 * <p>
 * JDBC Type 4 TrafT4Connection class.
 * </p>
 * <p>
 * Description: The <code>TrafT4Connection</code> class is an implementation of
 * the <code>java.sql.Connection</code> interface.
 * </p>
 *
 */
public class TrafT4Connection extends PreparedStatementManager implements java.sql.Connection {

	private T4DatabaseMetaData metaData_;
	/**
	 * Validates the connection by clearing warnings and verifying that the
	 * Connection is still open.
	 * 
	 * @throws SQLException
	 *             If the Connection is not valid
	 */
	private void validateConnection() throws SQLException {
		clearWarnings();

		if (this.ic_ == null || this.ic_.isClosed()) {
			throw TrafT4Messages.createSQLException(this.props_, this.getLocale(), "invalid_connection", null);
		}
	}
	
	public String getRemoteProcess() throws SQLException {
		return this.ic_.getRemoteProcess();
	}

	synchronized public void close() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "close", "", p);
		}

		if (this.ic_ == null || this.ic_.isClosed())
			return;

		// only hard-close if no pooled connection exists
		close((pc_ == null), true);
	}

	public void commit() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "commit", "", p);
		}

		validateConnection();

		try {
			ic_.commit();
		} catch (SQLException se) {
			performConnectionErrorChecks(se);
			throw se;
		}
	}
	
	public void resetServerIdleTimer() throws SQLException {
		clearWarnings();
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}

		this.setConnectionAttr(InterfaceConnection.RESET_IDLE_TIMER, 0, "0");
	}
	
	public String getApplicationName() throws SQLException {
		validateConnection();
		
		return this.ic_.getApplicationName();
	}
	
	public String getServerDataSource() throws SQLException {
		validateConnection();
		
		return this.ic_.getServerDataSource();
	}

	public boolean getEnforceISO() throws SQLException {
		validateConnection();

		return this.ic_.getEnforceISO();
	}

	public int getISOMapping() throws SQLException {
		validateConnection();

		return this.ic_.getISOMapping();
	}

	public String getRoleName() throws SQLException {
		validateConnection();

		return this.ic_.getRoleName();
	}

	public int getTerminalCharset() throws SQLException {
		validateConnection();

		return this.ic_.getTerminalCharset();
	}

	public T4Properties getT4Properties() throws SQLException {
		validateConnection();

		return this.ic_.t4props_;
	}

	public String getSessionName() throws SQLException {
		validateConnection();

		return this.ic_.getSessionName();
	}

	public Statement createStatement() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "createStatement", "", p);
		}

		validateConnection();

		try {
			return new TrafT4Statement(this);
		} catch (SQLException se) {
			performConnectionErrorChecks(se);
			throw se;
		}
	}

	public Statement createStatement(int resultSetType, int resultSetConcurrency) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, resultSetType, resultSetConcurrency);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "createStatement", "", p);
		}

		validateConnection();

		try {
			return new TrafT4Statement(this, resultSetType, resultSetConcurrency);
		} catch (SQLException se) {
			performConnectionErrorChecks(se);
			throw se;
		}
	}

	public Statement createStatement(int resultSetType, int resultSetConcurrency, int resultSetHoldability)
			throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, resultSetType, resultSetConcurrency,
					resultSetHoldability);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "createStatement", "", p);
		}

		validateConnection();

		try {
			return new TrafT4Statement(this, resultSetType, resultSetConcurrency, resultSetHoldability);
		} catch (SQLException se) {
			performConnectionErrorChecks(se);
			throw se;
		}
	}

	Locale getLocale() {
		if (props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINER, "TrafT4Connection", "getLocale", "", p);
		}
		if (ic_ != null) {
			return ic_.getLocale();
		} else {
			return null;
		}
	}

	public boolean getAutoCommit() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "getAutoCommit", "getAutoCommit", p);
		}

		validateConnection();

		return ic_.getAutoCommit();
	}

	public String getCatalog() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "getCatalog", "", p);
		}

		validateConnection();

		return ic_.getCatalog();
	}

    public String getSchema() throws SQLException {
        if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
            Object p[] = T4LoggingUtilities.makeParams(props_);
            props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "getSchema", "", p);
        }

        validateConnection();

        return ic_.getSchema();
    }

	public int getHoldability() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "getHoldability", "", p);
		}

		validateConnection();

		return holdability_;
	}

	public DatabaseMetaData getMetaData() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "getMetaData", "getMetaData", p);
		}

		validateConnection();

		return this.metaData_;
	}

	public int getTransactionIsolation() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "getTransactionIsolation", "", p);
		}

		validateConnection();

		return ic_.getTransactionIsolation();
	}

	public Map getTypeMap() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "getTypeMap", "", p);
		}

		validateConnection();

		return userMap_;
	}

	void isConnectionOpen() throws SQLException {
		validateConnection();
	}

	public boolean isClosed() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "isClosed", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("isClosed");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}

		boolean rv = true;

		if (ic_ == null) {
			rv = true;
			// return true;
		} else {
			clearWarnings();
			rv = ic_.getIsClosed();
		}
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "isClosed", "At exit return = " + rv, p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("isClosed");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}

		return rv;
		// return ic_.get_isClosed();
	}

	// New method that checks if the connection is closed
	// However, this is to be used only be internal classes
	// It does not clear any warnings associated with the current connection
	// Done for CASE 10_060123_4011 ; Swastik Bihani
	boolean _isClosed() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "_isClosed", "", p);
		}
		if ( props_ != null && props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("_isClosed");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}

		boolean rv = true;

		if (ic_ == null) {
			rv = true;
		} else {
			rv = ic_.getIsClosed();
		}
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "_isClosed", "At exit return = " + rv, p);
		}
		if ( props_ != null && props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("_isClosed");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}

		return rv;
	}

	/**
	 * @deprecated
	 */
	public String getServiceName() throws SQLException {
		return "";
	}

	/**
	 * @deprecated
	 */
	public void setServiceName(String serviceName) throws SQLException {
		
	}

	public boolean isReadOnly() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "isReadOnly", "", p);
		}

		validateConnection();

		return ic_.isReadOnly();
	}

	public String nativeSQL(String sql) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, sql);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "nativeSQL", "", p);
		}

		validateConnection();

		return sql;
	}

	public CallableStatement prepareCall(String sql) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, sql);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "prepareCall", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, sql);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("prepareCall");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}

		TrafT4CallableStatement stmt;

		clearWarnings();
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}

		try {
			if (isStatementCachingEnabled()) {
				stmt = (TrafT4CallableStatement) getPreparedStatement(this, sql, ResultSet.TYPE_FORWARD_ONLY,
						ResultSet.CONCUR_READ_ONLY, holdability_);

				if (stmt != null) {
					return stmt;
				}
			}

			stmt = new TrafT4CallableStatement(this, sql);
			stmt.prepareCall(stmt.sql_, stmt.queryTimeout_, stmt.resultSetHoldability_);

			if (isStatementCachingEnabled()) {
				addPreparedStatement(this, sql, stmt, ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY,
						holdability_);
			}
		} catch (TrafT4Exception se) {
			performConnectionErrorChecks(se);
			throw se;
		}
		return stmt;
	}

	public CallableStatement prepareCall(String sql, String stmtLabel) throws SQLException {
		final String QUOTE = "\"";

		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, sql);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "prepareCall", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, sql);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("prepareCall");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}

		if (stmtLabel == null || stmtLabel.length() == 0) {
			throw TrafT4Messages.createSQLException(props_, null, "null_data", null);
		}

		if (stmtLabel.startsWith(QUOTE) && stmtLabel.endsWith(QUOTE)) {
			int len = stmtLabel.length();
			if (len == 2) {
				throw TrafT4Messages.createSQLException(props_, null, "null_data", null);
			} else {
				stmtLabel = stmtLabel.substring(1, len - 1);
			}
		} else {
			stmtLabel = stmtLabel.toUpperCase();
		}

		TrafT4CallableStatement stmt = null;

		clearWarnings();
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}

		try {
			if (isStatementCachingEnabled()) {
				stmt = (TrafT4CallableStatement) getPreparedStatement(this, sql, ResultSet.TYPE_FORWARD_ONLY,
						ResultSet.CONCUR_READ_ONLY, holdability_);

				if (stmt != null) {
					return stmt;
				}
			}
            if (stmtLabel.equalsIgnoreCase("null")) {
                stmt = new TrafT4CallableStatement(this, sql);
            } else {
                stmt = new TrafT4CallableStatement(this, sql, stmtLabel);
            }
			stmt.prepareCall(stmt.sql_, stmt.queryTimeout_, stmt.resultSetHoldability_);

			if (isStatementCachingEnabled()) {
				addPreparedStatement(this, sql, stmt, ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY,
						holdability_);
			}
		} catch (TrafT4Exception se) {
			performConnectionErrorChecks(se);
			throw se;
		}
		return stmt;
	}

	public CallableStatement prepareCall(String sql, int resultSetType, int resultSetConcurrency) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, sql, resultSetType, resultSetConcurrency);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "prepaseCall", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, sql, resultSetType, resultSetConcurrency);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("prepaseCall");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		TrafT4CallableStatement stmt;

		clearWarnings();
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}

		try {
			if (isStatementCachingEnabled()) {
				stmt = (TrafT4CallableStatement) getPreparedStatement(this, sql, resultSetType, resultSetConcurrency,
						holdability_);
				if (stmt != null) {
					return stmt;
				}
			}

			stmt = new TrafT4CallableStatement(this, sql, resultSetType, resultSetConcurrency);
			stmt.prepareCall(stmt.sql_, stmt.queryTimeout_, stmt.resultSetHoldability_);

			if (isStatementCachingEnabled()) {
				addPreparedStatement(this, sql, stmt, resultSetType, resultSetConcurrency, holdability_);
			}
		} catch (TrafT4Exception se) {
			performConnectionErrorChecks(se);
			throw se;
		}
		return stmt;
	}

	public CallableStatement prepareCall(String sql, int resultSetType, int resultSetConcurrency,
			int resultSetHoldability) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, sql, resultSetType, resultSetConcurrency,
					resultSetHoldability);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "prepareCall", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, sql, resultSetType, resultSetHoldability);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("prepareCall");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		TrafT4CallableStatement stmt;

		clearWarnings();
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}
		try {
			if (isStatementCachingEnabled()) {
				stmt = (TrafT4CallableStatement) getPreparedStatement(this, sql, resultSetType, resultSetConcurrency,
						resultSetHoldability);
				if (stmt != null) {
					return stmt;
				}
			}

			stmt = new TrafT4CallableStatement(this, sql, resultSetType, resultSetConcurrency, resultSetHoldability);

			stmt.prepareCall(stmt.sql_, stmt.queryTimeout_, stmt.resultSetHoldability_);

			if (isStatementCachingEnabled()) {
				addPreparedStatement(this, sql, stmt, resultSetType, resultSetConcurrency, resultSetHoldability);
			}
		} catch (TrafT4Exception se) {
			performConnectionErrorChecks(se);
			throw se;
		}
		return stmt;
	}

	/**
	 * Creates a <code>PreparedStatement</code> object for sending
	 * parameterized SQL statements to the database.
	 * 
	 * @param sql
	 *            SQL statement that might contain one or more '?' IN parameter
	 *            placeholders
	 * @param stmtLabel
	 *            SQL statement label that can be passed to the method instead
	 *            of generated by the database system.
	 * @returns a new default PreparedStatement object containing the
	 *          pre-compiled SQL statement
	 * @throws SQLException
	 *             if a database access error occurs
	 */
	public PreparedStatement prepareStatement(String sql, String stmtLabel) throws SQLException {
		final String QUOTE = "\"";

		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, sql);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "prepareStatement", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, sql);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("prepareStatement");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}

		if (stmtLabel == null || stmtLabel.length() == 0) {
			throw TrafT4Messages.createSQLException(props_, null, "null_data", null);
		}

		if (stmtLabel.startsWith(QUOTE) && stmtLabel.endsWith(QUOTE)) {
			int len = stmtLabel.length();
			if (len == 2) {
				throw TrafT4Messages.createSQLException(props_, null, "null_data", null);
			} else {
				stmtLabel = stmtLabel.substring(1, len - 1);
			}
		} else {
			stmtLabel = stmtLabel.toUpperCase();
		}

		TrafT4PreparedStatement stmt;

		clearWarnings();
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}
		try {
			if (isStatementCachingEnabled()) {
				stmt = (TrafT4PreparedStatement) getPreparedStatement(this, sql, ResultSet.TYPE_FORWARD_ONLY,
						ResultSet.CONCUR_READ_ONLY, holdability_);
				if (stmt != null) {
					return stmt;
				}
			}

			stmt = new TrafT4PreparedStatement(this, sql, stmtLabel);

			stmt.prepare(stmt.sql_, stmt.queryTimeout_, stmt.resultSetHoldability_);

			if (isStatementCachingEnabled()) {
				addPreparedStatement(this, sql, stmt, ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY,
						holdability_);
			}
		} catch (TrafT4Exception se) {
			performConnectionErrorChecks(se);
			throw se;
		}
		return stmt;
	}

	public PreparedStatement prepareStatement(String sql) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, sql);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "prepareStatement", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, sql);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("prepareStatement");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}

		TrafT4PreparedStatement stmt;

		clearWarnings();
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}
		try {
			if (isStatementCachingEnabled()) {
				stmt = (TrafT4PreparedStatement) getPreparedStatement(this, sql, ResultSet.TYPE_FORWARD_ONLY,
						ResultSet.CONCUR_READ_ONLY, holdability_);
				if (stmt != null) {
					return stmt;
				}
			}

			stmt = new TrafT4PreparedStatement(this, sql);

			stmt.prepare(stmt.sql_, stmt.queryTimeout_, stmt.resultSetHoldability_);

			if (isStatementCachingEnabled()) {
				addPreparedStatement(this, sql, stmt, ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY,
						holdability_);
			}
		} catch (TrafT4Exception se) {
			performConnectionErrorChecks(se);
			throw se;
		}
		return stmt;
	}

	// SB 12/02/2004 - only for LOB statements - these will be not added to the
	// statement cache
	PreparedStatement prepareLobStatement(String sql) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, sql);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "prepareLobStatement", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, sql);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("prepareLobStatement");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}

		TrafT4PreparedStatement stmt;

		clearWarnings();
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}

		try {
			stmt = new TrafT4PreparedStatement(this, sql);
			stmt.prepare(stmt.sql_, stmt.queryTimeout_, stmt.resultSetHoldability_);
		} catch (TrafT4Exception se) {
			performConnectionErrorChecks(se);
			throw se;
		}
		return stmt;

	}

	public PreparedStatement prepareStatement(String sql, int autoGeneratedKeys) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, sql, autoGeneratedKeys);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "prepareStatement", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, sql, autoGeneratedKeys);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("prepareStatement");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}

		if (autoGeneratedKeys == TrafT4Statement.NO_GENERATED_KEYS) {
			return prepareStatement(sql);
		} else {
			throw TrafT4Messages.createSQLException(props_, getLocale(), "auto_generated_keys_not_supported", null);
		}
	}

	public PreparedStatement prepareStatement(String sql, int[] columnIndexes) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, sql, columnIndexes);
			props_.t4Logger_.logp(Level.FINE, "SQLConnection", "prepareStatement", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, sql, columnIndexes);
			lr.setParameters(p);
			lr.setSourceClassName("SQLConnection");
			lr.setSourceMethodName("prepareStatement");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}

		if (columnIndexes != null && columnIndexes.length > 0) {
			throw TrafT4Messages.createSQLException(props_, getLocale(), "auto_generated_keys_not_supported", null);
		} else {
			return prepareStatement(sql);
		}
	}

	public PreparedStatement prepareStatement(String sql, int resultSetType, int resultSetConcurrency)
			throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, sql, resultSetType, resultSetConcurrency);
			props_.t4Logger_.logp(Level.FINE, "SQLConnection", "prepareStatement", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, sql, resultSetType, resultSetConcurrency);
			lr.setParameters(p);
			lr.setSourceClassName("SQLConnection");
			lr.setSourceMethodName("prepareStatement");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		TrafT4PreparedStatement stmt;

		clearWarnings();

		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}

		try {
			if (isStatementCachingEnabled()) {
				stmt = (TrafT4PreparedStatement) getPreparedStatement(this, sql, resultSetType, resultSetConcurrency,
						holdability_);
				if (stmt != null) {
					return stmt;
				}
			}

			stmt = new TrafT4PreparedStatement(this, sql, resultSetType, resultSetConcurrency);
			stmt.prepare(stmt.sql_, stmt.queryTimeout_, stmt.resultSetHoldability_);

			if (isStatementCachingEnabled()) {
				addPreparedStatement(this, sql, stmt, resultSetType, resultSetConcurrency, holdability_);
			}
		} catch (TrafT4Exception se) {
			performConnectionErrorChecks(se);
			throw se;
		}
		return stmt;
	}

	public PreparedStatement prepareStatement(String sql, int resultSetType, int resultSetConcurrency,
			int resultSetHoldability) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, sql, resultSetType, resultSetConcurrency,
					resultSetHoldability);
			props_.t4Logger_.logp(Level.FINE, "SQLConnection", "prepareStatement", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, sql, resultSetType, resultSetHoldability);
			lr.setParameters(p);
			lr.setSourceClassName("SQLConnection");
			lr.setSourceMethodName("prepareStatement");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		TrafT4PreparedStatement stmt;

		clearWarnings();
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}

		try {
			if (isStatementCachingEnabled()) {
				stmt = (TrafT4PreparedStatement) getPreparedStatement(this, sql, resultSetType, resultSetConcurrency,
						resultSetHoldability);
				if (stmt != null) {
					return stmt;
				}
			}

			stmt = new TrafT4PreparedStatement(this, sql, resultSetType, resultSetConcurrency, resultSetHoldability);
			stmt.prepare(stmt.sql_, stmt.queryTimeout_, stmt.resultSetHoldability_);

			if (isStatementCachingEnabled()) {
				addPreparedStatement(this, sql, stmt, resultSetType, resultSetConcurrency, resultSetHoldability);
			}
		} catch (TrafT4Exception se) {
			performConnectionErrorChecks(se);
			throw se;
		}
		return stmt;
	}

	public PreparedStatement prepareStatement(String sql, String[] columnNames) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, sql, columnNames);
			props_.t4Logger_.logp(Level.FINE, "SQLConnection", "prepareStatement", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, sql, columnNames);
			lr.setParameters(p);
			lr.setSourceClassName("SQLConnection");
			lr.setSourceMethodName("prepareStatement");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		if (columnNames != null && columnNames.length > 0) {
			throw TrafT4Messages.createSQLException(props_, getLocale(), "auto_generated_keys_not_supported", null);
		} else {
			return prepareStatement(sql);
		}
	}

	public void releaseSavepoint(Savepoint savepoint) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, savepoint);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "releaseSavepoint", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, savepoint);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("releaseSavepoint");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		clearWarnings();
		TrafT4Messages.throwUnsupportedFeatureException(props_, getLocale(), "releaseSavepoint()");
	}

	public void rollback() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "rollback", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("rollback");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		clearWarnings();
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}

		// if (ic_.getTxid() == 0) - XA
		// return;

		// commit the Transaction
		try {
			ic_.rollback();
		} catch (TrafT4Exception se) {
			performConnectionErrorChecks(se);
			throw se;
		}
		// ic_.setTxid(0); - XA
	}

	public void rollback(Savepoint savepoint) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, savepoint);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "rollback", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, savepoint);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("rollback");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		clearWarnings();
		TrafT4Messages.throwUnsupportedFeatureException(props_, getLocale(), "rollback(Savepoint)");
	}

	public void setAutoCommit(boolean autoCommit) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, autoCommit);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "setAutoCommit", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, autoCommit);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("setAutoCommit");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}

		clearWarnings();
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}
		try {
			ic_.setAutoCommit(this, autoCommit);
		} catch (TrafT4Exception se) {
			performConnectionErrorChecks(se);
			throw se;
		}
	}

	public void setCatalog(String catalog) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, catalog);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "setCalalog", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, catalog);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("setCalalog");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		clearWarnings();
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}
		if (catalog != null) {
			try {
				ic_.setCatalog(this, catalog);
			} catch (TrafT4Exception se) {
				performConnectionErrorChecks(se);
				throw se;
			}
		}
	}

	public void setHoldability(int holdability) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, holdability);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "setHoldability", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, holdability);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("setHoldability");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		clearWarnings();
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}

		if (holdability != TrafT4ResultSet.CLOSE_CURSORS_AT_COMMIT)

		{
			throw TrafT4Messages.createSQLException(props_, getLocale(), "invalid_holdability", null);
		}
		holdability_ = holdability;
	}

	public void setReadOnly(boolean readOnly) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, readOnly);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "setReadOnly", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, readOnly);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("setReadOnly");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		clearWarnings();
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}
		try {
			// ic_.setReadOnly(readOnly);
			ic_.setReadOnly(this, readOnly);
		} catch (TrafT4Exception se) {
			performConnectionErrorChecks(se);
			throw se;
		}
	}

	public void setConnectionAttr(short attr, int valueNum, String valueString) throws SQLException {
		ic_.setConnectionAttr(this, attr, valueNum, valueString);
	}

	//3196 - NDCS transaction for SPJ	
	public void joinUDRTransaction(long transId) throws SQLException {
		String sTransid = String.valueOf(transId);
		ic_.setConnectionAttr(this,  InterfaceConnection.SQL_ATTR_JOIN_UDR_TRANSACTION, 0, sTransid);
	}
	
	//3196 - NDCS transaction for SPJ
	public void suspendUDRTransaction() throws SQLException {
		String sTransid = String.valueOf(ic_.transId_);
		ic_.setConnectionAttr(this, InterfaceConnection.SQL_ATTR_SUSPEND_UDR_TRANSACTION, 0, sTransid);
	}

	public Savepoint setSavepoint(String name) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, name);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "setSavepoint", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, name);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("setSavepoint");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		clearWarnings();
		TrafT4Messages.throwUnsupportedFeatureException(props_, getLocale(), "setSavepoint");
		return null;
	}

	public Savepoint setSavepoint() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "setSavepoint", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("setSavepoint");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		clearWarnings();
		TrafT4Messages.throwUnsupportedFeatureException(props_, getLocale(), "setSavepoint");
		return null;
	}

	public void setTransactionIsolation(int level) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, level);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "setTransactionIsolation", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, level);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("setTransactionIsolation");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		clearWarnings();
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}
		try {
			ic_.setTransactionIsolation(this, level);
		} catch (TrafT4Exception se) {
			performConnectionErrorChecks(se);
			throw se;
		}
	}

	// JDK 1.2
	public void setTypeMap(java.util.Map map) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, map);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "setTypeMap", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, map);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("setTypeMap");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		clearWarnings();
		userMap_ = map;
	}

	public void begintransaction() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "begintransaction", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("begintransaction");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}
		try {
			ic_.beginTransaction();

			if (ic_.beginTransaction() == 0) {
				return;
			} else {
				setAutoCommit(false);
			}
		} catch (TrafT4Exception se) {
			performConnectionErrorChecks(se);
			throw se;
		}
	}

	public long getCurrentTransaction() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "getTxid", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("getTxid");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}
		return ic_.getTxid();
	}

	public void setTxid(long txid) throws SQLException {
		setTransactionToJoin(Bytes.createLongBytes(txid, this.ic_.getByteSwap()));
	}

	public void setTransactionToJoin(byte[] txid) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, txid);
			props_.t4Logger_.logp(Level.FINER, "TrafT4Connection", "setTxid", "", p);
		}
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		}

		transactionToJoin = txid;
	}

	void gcStmts() {
		if (props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINER, "TrafT4Connection", "gcStmt", "", p);
		}
		Reference pRef;
		String stmtLabel;

		while ((pRef = refStmtQ_.poll()) != null) {
			stmtLabel = (String) refToStmt_.get(pRef);
			// All PreparedStatement objects are added to HashMap
			// Only Statement objects that produces ResultSet are added to
			// HashMap
			// Hence stmtLabel could be null
			if (stmtLabel != null) {
				try {
					TrafT4Statement stmt = new TrafT4Statement(this, stmtLabel);
					stmt.close();
					stmt = null;
				} catch (SQLException e) {
					performConnectionErrorChecks(e);
				} finally {
					refToStmt_.remove(pRef);
				}
			}
		}
	}

	void removeElement(Reference pRef) {
		if (props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, pRef);
			props_.t4Logger_.logp(Level.FINER, "TrafT4Connection", "removeElement", "", p);
		}

		refToStmt_.remove(pRef);
		pRef.clear();
	}

	void addElement(Reference pRef, String stmtLabel) {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, stmtLabel);
			props_.t4Logger_.logp(Level.FINER, "TrafT4Connection", "addElement", "", p);
		}
		refToStmt_.put(pRef, stmtLabel);
	}

	private void physicalCloseStatements() {
		if (props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINER, "TrafT4Connection", "physicalCloseStatement", "", p);
		}
		// close all the statements
		ArrayList stmts = new ArrayList(refToStmt_.values());
		int size = stmts.size();
		for (int i = 0; i < size; i++) {
			try {
				String stmtLabel = (String) stmts.get(i);
				TrafT4Statement stmt = new TrafT4Statement(this, stmtLabel);
				stmt.close();
				stmt = null;
			} catch (SQLException se) {
				// Ignore any exception and proceed to the next statement
			}
		}
		refToStmt_.clear();

		
	}

	private void rollbackAndIgnoreError() {
		if (props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINER, "TrafT4Connection", "rollbackAndIgnoreError", "", p);
		}
		// Rollback the Transaction when autoCommit mode is OFF
		try {
			if (getAutoCommit() == false ) {    
				rollback();
			}
		} catch (SQLException sqex) {
			if (props_.t4Logger_.isLoggable(Level.FINER) == true) {
				Object p[] = T4LoggingUtilities.makeParams(props_);
				props_.t4Logger_.logp(Level.FINER, "TrafT4Connection", "rollbackAndIgnoreError", "warning: "+sqex.getMessage(), p);
			}
		}
	}


	synchronized void close(boolean hardClose, boolean sendEvents) throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, hardClose, sendEvents);
			props_.t4Logger_.logp(Level.FINER, "TrafT4Connection", "close", "", p);
		}
		clearWarnings();
		try {
			if (!hardClose) {
				if (this.ic_ != null && this.ic_.getIsClosed()) {
					return;
				}
				if (isStatementCachingEnabled()) {
					closePreparedStatementsAll();
				} else {
					physicalCloseStatements();
				}
				rollbackAndIgnoreError();

				/*
				 * //inform the NCS server to disregard the T4 ConnectionTimeout
				 * value try{ if (ic_ != null) {
				 * ic_.disregardT4ConnectionTimeout(this); } }catch(SQLException
				 * e){ //ignore - new property from old MXCS ABD version (now
				 * known as NCS) //ignored for backward compatibility }
				 */

				// Need to logicallcally close the statement
				pc_.logicalClose(sendEvents);
				if (ic_ != null) {
					ic_.setIsClosed(true);
				}
			} else {
				if (getServerHandle() == null) {
					return;
				}

				// close all the statements
				physicalCloseStatements();

				// Need to logicallcally close the statement
				// Rollback the Transaction when autoCommit mode is OFF
				rollbackAndIgnoreError();

				if (isStatementCachingEnabled()) {
					clearPreparedStatementsAll();
				}

				// Close the connection
				try {
					ic_.close();
				} finally {
					if (ic_ != null) {
						ic_.removeElement(this);
					}
					ic_ = null;
				}
			}
		} catch (SQLException e) {
			performConnectionErrorChecks(e);
			throw e;
		} finally {
			// close the timer thread
			if (ic_ != null && ic_.getT4Connection() != null) {
				ic_.getT4Connection().closeTimers();
			}
		}
	}

	protected void finalize() {
		if (ic_ != null && ic_.getT4Connection() != null) {
			ic_.getT4Connection().closeTimers();
		}
	}

	void reuse() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINER, "TrafT4Connection", "resue", "", p);
		}
		ic_.reuse();
		/*
		 * try { ic_.enforceT4ConnectionTimeout(this); } catch (TrafT4Exception
		 * se) { //performConnectionErrorChecks(se); //throw se; //ignore - new
		 * property from old MXCS ABD version (now known as NCS) //ignored for
		 * backward compatibility }
		 */
	}


	// Extension method for WLS, this method gives the pooledConnection object
	// associated with the given connection object.
	public PooledConnection getPooledConnection() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "getPooledConnection", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("getPooledConnection");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		if (pc_ != null) {
			return pc_;
		} else {
			throw TrafT4Messages.createSQLException(props_, getLocale(), "null_pooled_connection", null);
		}
	}

	TrafT4Connection(TrafT4DataSource ds, T4Properties t4props) throws SQLException {
		super(t4props);

		t4props.setConnectionID(Integer.toString(this.hashCode()));
		setupLogging(t4props);
		
		if (t4props.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, ds, t4props);
			t4props.t4Logger_.logp(Level.FINE, "TrafT4Connection", "<init>", "", p);
		}
		if ( t4props.t4Logger_.isLoggable(Level.FINE) && t4props.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, ds, t4props);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("<init>");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			t4props.getLogWriter().println(temp);
		}
		ds_ = ds;


		makeConnection(t4props);
		holdability_ = TrafT4ResultSet.CLOSE_CURSORS_AT_COMMIT;
	}

	TrafT4Connection(TrafT4PooledConnection poolConn, T4Properties t4props) throws SQLException {
		super(t4props);

		t4props.setConnectionID(Integer.toString(this.hashCode()));
		setupLogging(t4props);
		
		if (t4props.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, poolConn, t4props);
			t4props.t4Logger_.logp(Level.FINE, "TrafT4Connection", "<init>", "", p);
		}
		if ( t4props.t4Logger_.isLoggable(Level.FINE) && t4props.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_, poolConn, t4props);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("<init>");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			t4props.getLogWriter().println(temp);
		}

		pc_ = poolConn;


		makeConnection(t4props);
		holdability_ = TrafT4ResultSet.CLOSE_CURSORS_AT_COMMIT;
	}
	
	private void makeConnection(T4Properties t4props) throws SQLException {	
		if (t4props.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_, t4props);
			t4props.t4Logger_.logp(Level.FINER, "TrafT4Connection", "makeConnection", "", p);
		}
		
		ArrayList<String> addresses = createAddressList(t4props);
		SQLException eList = null;
		boolean success = false;
		
		for(int i=0;i<addresses.size();i++) {
			clearWarnings();
			t4props.setUrl(addresses.get(i));
			
			try {
				ic_ = new InterfaceConnection(this, t4props);
				success = true;
				break;
			} catch (SQLException se) {
				boolean connectionError = performConnectionErrorChecks(se);
				if(addresses.size() == 1 || !connectionError) {
					throw se;
				}
				if(eList == null) {
					eList = se;
				}
				else {
					eList.setNextException(se);
				}
			}
		}
				
		if(!success) {
			throw eList;
		}
		
		if (ic_.sqlwarning_ != null) {
			setSqlWarning(ic_.sqlwarning_);
		}
		
		refStmtQ_ = new ReferenceQueue();
		refToStmt_ = new HashMap();
		pRef_ = new WeakReference(this, ic_.refQ_);
		ic_.refTosrvrCtxHandle_.put(pRef_, ic_);
		props_ = t4props;

		ic_.enableNARSupport(this, props_.getBatchRecovery());

		if (props_.getSPJEnv()) {
			ic_.enableProxySyntax(this);
		}
		this.metaData_ = new T4DatabaseMetaData(this);
	}
	
	private ArrayList<String> createAddressList(T4Properties t4props) {
		ArrayList<String> addresses = new ArrayList<String>(); //10 addresses by default
		addresses.add(t4props.getUrl());
		String os = System.getProperty("os.name");
		String enable = System.getProperty("t4jdbc.redirectaddr");
		
		if(enable != null && enable.equals("true") && os != null && os.equals("NONSTOP_KERNEL")) { //  TODO get real name	
			String providedUrl = t4props.getUrl();
			String providedHost = providedUrl.substring(16).toLowerCase();
			String hostPrefix = null;
			try {
				hostPrefix = java.net.InetAddress.getLocalHost().getHostName().substring(0, 5).toLowerCase();
			}catch(Exception e) {
			}
			
			if(hostPrefix != null && providedHost.startsWith(hostPrefix)) {
				File f = new File("/E/" + hostPrefix + "01/usr/t4jdbc/jdbcaddr.txt");
				if(f.exists()) {
					addresses.clear();
					
					String urlSuffix = providedUrl.substring(providedUrl.indexOf("/:"));
					
					try {
				        BufferedReader in = new BufferedReader(new FileReader(f));
				        String host;
				        while ((host = in.readLine()) != null) {
				            if(host.indexOf(':') == -1) {
				            	host += ":18650";
				            }
				            addresses.add(String.format("jdbc:t4jdbc://" + host + urlSuffix));
				        }
				        in.close();
				    } catch (IOException e) {
				    }
				}
			}
		}
		
		return addresses;
	}

	

	// --------------------------------------------------------
	private void setupLogging(T4Properties t4props) {

		String ID = T4LoggingUtilities.getUniqueID();
		String name = T4LoggingUtilities.getUniqueLoggerName(ID);

		if (t4props.getT4LogLevel() == Level.OFF) {
			if (dummyLogger_ == null) {
				dummyLogger_ = Logger.getLogger(name);
			}
			t4props.t4Logger_ = dummyLogger_;
		} else {
			t4props.t4Logger_ = Logger.getLogger(name);
		}

		// t4props.t4Logger_ = Logger.getLogger(name);
		t4props.t4Logger_.setUseParentHandlers(false);
		t4props.t4Logger_.setLevel(t4props.getT4LogLevel());

		if (t4props.getT4LogLevel() != Level.OFF) {
			FileHandler fh1 = t4props.getT4LogFileHandler();
			t4props.t4Logger_.addHandler(fh1);
		}
	} // end setupLogging

	// --------------------------------------------------------

	// Interface Methods
	InterfaceConnection getServerHandle() {
		if (props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINER, "TrafT4Connection", "getServerHandle", "", p);
		}
		return ic_;
	}

	// Interface Methods
	public int getDialogueId() throws SQLException {
		if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(props_);
			props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "getDialogueId", "", p);
		}
		if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4Connection");
			lr.setSourceMethodName("getDialogueId");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			props_.getLogWriter().println(temp);
		}
		return getServerHandle().getDialogueId();
	}

	/**
	 * Returns true if the data format needs to be converted. Used by the
	 * <CODE>TrafT4ResultSet</CODE> class.
	 * 
	 * @return true if conversion is needed; otherwise, false.
	 */
	public boolean getDateConversion() throws SQLException {
		validateConnection();

		return ic_.getDateConversion();
	}

	int getServerMajorVersion() throws SQLException {
		validateConnection();

		return ic_.getServerMajorVersion();
	}

	int getServerMinorVersion() throws SQLException {
		validateConnection();

		return ic_.getServerMinorVersion();
	}


	void closeErroredConnection(TrafT4Exception se) {
		try {
			if (!erroredConnection) { // don't issue close repeatedly
				erroredConnection = true;
				if (pc_ != null) {
					pc_.sendConnectionErrorEvent(se);
				} else {
					// hardclose
					close(true, true);
				}
			}
		} catch (Exception e) {
			// ignore
		}
	}


	boolean erroredConnection = false;

	static Logger dummyLogger_ = null;

	// Fields
	InterfaceConnection ic_;

	// Connection
	Map userMap_;
	ReferenceQueue refStmtQ_;
	HashMap refToStmt_;
	int holdability_;
	TrafT4DataSource ds_;
	TrafT4PooledConnection pc_;
	T4Driver driver_;
	WeakReference pRef_;
	T4Properties props_;

	byte[] transactionToJoin;
	

	public Object unwrap(Class iface) throws SQLException {
		try {
			return iface.cast(this);
		} catch (ClassCastException cce) {
			throw TrafT4Messages.createSQLException(props_, this.getLocale(), "unable_unwrap", iface.toString());
		}
	}

	public boolean isWrapperFor(Class iface) throws SQLException {
		if (_isClosed() == true) {
			throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
		} else {
			return iface.isInstance(this);
		}
	}

	public Clob createClob() throws SQLException {
		return new TrafT4Clob(this, null, null);
	}

	public Blob createBlob() throws SQLException {
		return new TrafT4Blob(this, null, null);
	}

	public NClob createNClob() throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public SQLXML createSQLXML() throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public boolean isValid(int timeout) throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	public void setClientInfo(String name, String value) throws SQLClientInfoException {
		ic_.setClientInfo(name, value);
	}

	public void setClientInfo(Properties properties) throws SQLClientInfoException {
		ic_.setClientInfoProperties( properties);
	}

	public String getClientInfo(String name) throws SQLException {
		validateConnection();
		return ic_.getClientInfoProperties().getProperty(name);
	}

	public Properties getClientInfo() throws SQLException {
		validateConnection();
		return ic_.getClientInfoProperties();
	}

	public Array createArrayOf(String typeName, Object[] elements)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Struct createStruct(String typeName, Object[] attributes)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

    public void setSchema(String schema) throws SQLException {
        if (props_.t4Logger_.isLoggable(Level.FINE) == true) {
            Object p[] = T4LoggingUtilities.makeParams(props_, schema);
            props_.t4Logger_.logp(Level.FINE, "TrafT4Connection", "setSchema", "", p);
        }
        if ( props_.t4Logger_.isLoggable(Level.FINE) && props_.getLogWriter() != null ) {
            LogRecord lr = new LogRecord(Level.FINE, "");
            Object p[] = T4LoggingUtilities.makeParams(props_, schema);
            lr.setParameters(p);
            lr.setSourceClassName("TrafT4Connection");
            lr.setSourceMethodName("setSchema");
            T4LogFormatter lf = new T4LogFormatter();
            String temp = lf.format(lr);
            props_.getLogWriter().println(temp);
        }
        clearWarnings();
        if (_isClosed() == true) {
            throw TrafT4Messages.createSQLException(props_, null, "invalid_connection", null);
        }
        if (schema != null) {
            try {
                ic_.setSchema(this, schema);
            } catch (TrafT4Exception se) {
                performConnectionErrorChecks(se);
                throw se;
            }
        }
    }

	public void abort(Executor executor) throws SQLException {
		if (ic_.getT4Connection().getInputOutput() != null) {
			ic_.getT4Connection().getInputOutput().closeIO();
		}
		ic_.setIsClosed(true);
	}

	public void setNetworkTimeout(Executor executor, int milliseconds)
			throws SQLException {
            validateConnection();
            props_.setNetworkTimeoutInMillis(milliseconds);
	}
	

	public int getNetworkTimeout() throws SQLException {
		validateConnection();
		return props_.getNetworkTimeoutInMillis();
	}

	/*
	 * JDK 1.6 functions public Clob createClob() throws SQLException { return
	 * null; }
	 * 
	 * 
	 * public Blob createBlob() throws SQLException { return null; }
	 * 
	 * 
	 * public NClob createNClob() throws SQLException { return null; }
	 * 
	 * 
	 * public SQLXML createSQLXML() throws SQLException { return null; }
	 * 
	 * 
	 * public boolean isValid(int _int) throws SQLException { return false; }
	 * 
	 * 
	 * public void setClientInfo(String string, String string1) throws
	 * SQLClientInfoException { }
	 * 
	 * 
	 * public void setClientInfo(Properties properties) throws
	 * SQLClientInfoException { }
	 * 
	 * 
	 * public String getClientInfo(String string) throws SQLException { return
	 * ""; }
	 * 
	 * 
	 * public Properties getClientInfo() throws SQLException { return null; }
	 * 
	 * 
	 * public Array createArrayOf(String string, Object[] objectArray) throws
	 * SQLException { return null; }
	 * 
	 * 
	 * public Struct createStruct(String string, Object[] objectArray) throws
	 * SQLException { return null; }
	 * 
	 * 
	 * public Object unwrap(Class _class) throws SQLException { return null; }
	 * 
	 * 
	 * public boolean isWrapperFor(Class _class) throws SQLException { return
	 * false; }
	 */
}
