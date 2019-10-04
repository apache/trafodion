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

import java.io.InputStream;
import java.io.Reader;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.net.URL;
import java.nio.ByteBuffer;
import java.sql.Array;
import java.sql.BatchUpdateException;
import java.sql.Blob;
import java.sql.Clob;
import java.sql.DatabaseMetaData;
import java.sql.Date;
import java.sql.NClob;
import java.sql.ParameterMetaData;
import java.sql.Ref;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.RowId;
import java.sql.SQLException;
import java.sql.SQLXML;
import java.sql.Time;
import java.sql.Timestamp;
import java.sql.Types;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Locale;
import java.util.logging.Level;
import java.util.logging.LogRecord;

public class TrafT4PreparedStatement extends TrafT4Statement implements java.sql.PreparedStatement {
	// java.sql.PreparedStatement interface methods
	public void addBatch() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "addBatch", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("addBatch");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		if (inputDesc_ == null) {
			return;
		}

		// Check if all parameters are set for current set
		checkIfAllParamsSet();
		// Add to the number of Rows Count
		if (rowsValue_ == null) {
			rowsValue_ = new ArrayList();
		}
		rowsValue_.add(paramsValue_);
		paramRowCount_++;
		paramsValue_ = new Object[inputDesc_.length];
		if (isAnyLob_ && (lobObjects_ == null)) {
			lobObjects_ = new ArrayList();
			// Clear the isValueSet_ flag in inputDesc_ and add the lob objects
			// to the lobObject List
		}
		for (int i = 0; i < inputDesc_.length; i++) {
			// If isAnyLob_ is false: inputDesc_.paramValue_ for all
			// parameters should be null
			// If isAnyLob_ is true: one or more inputDesc_.parmValue will not
			// be null, based on the number of LOB columns in the query
			if (inputDesc_[i].paramValue_ != null) {
				lobObjects_.add(inputDesc_[i].paramValue_);
				inputDesc_[i].paramValue_ = null;
			}
			inputDesc_[i].isValueSet_ = false;
		}
	}

	public void clearBatch() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "clearBatch", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("clearBatch");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		if (inputDesc_ == null) {
			return;
		}
		if (rowsValue_ != null) {
			rowsValue_.clear();
		}
		if (lobObjects_ != null) {
			lobObjects_.clear();
		}
		paramRowCount_ = 0;
		// Clear the isValueSet_ flag in inputDesc_
		for (int i = 0; i < inputDesc_.length; i++) {
			inputDesc_[i].isValueSet_ = false;
			paramsValue_[i] = null;
			inputDesc_[i].paramValue_ = null;
		}
		isAnyLob_ = false;
		batchRowCount_ = new int[] {};
	}

	public void clearParameters() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "clearParameters", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("clearParameters");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		// Clear the isValueSet_ flag in inputDesc_
		if (inputDesc_ == null) {
			return;
		}

		for (int i = 0; i < inputDesc_.length; i++) {
			inputDesc_[i].isValueSet_ = false;
			paramsValue_[i] = null;
			inputDesc_[i].paramValue_ = null;
		}
		isAnyLob_ = false;
	}

	public void close() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "close", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
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
				if (!connection_.isStatementCachingEnabled()) {
					super.close();
				} else {
					logicalClose();
				}
			}
		} catch (SQLException e) {
			performConnectionErrorChecks(e);
			throw e;
		} finally {
			isClosed_ = true;
			if (!connection_.isStatementCachingEnabled()) {
				connection_.removeElement(pRef_);
			}
		}

	}

	public boolean execute() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "execute", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("execute");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		Object[] valueArray = null;
		int inDescLength = 0;

		validateExecuteInvocation();

		// *******************************************************************
		// * If LOB is involved with autocommit enabled we throw an exception
		// *******************************************************************
		if (isAnyLob_ && (connection_.getAutoCommit())) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_lob_commit_state", null);
		}
		if (inputDesc_ != null) {
			if (!usingRawRowset_)
				valueArray = getValueArray();
			inDescLength = inputDesc_.length;
		}

		execute(paramRowCount_ + 1, inDescLength, valueArray, queryTimeout_, isAnyLob_); // LOB
		// Support
		// - SB
		// 9/28/04

		// if (resultSet_[result_set_offset] != null)
		if (resultSet_ != null && resultSet_[result_set_offset] != null) {
			return true;
		} else {
			if (isAnyLob_) {


			}
			return false;
		}
	}

	public int[] executeBatch() throws SQLException, BatchUpdateException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "executeBatch", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("executeBatch");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		try {
			clearWarnings();
			TrafT4Exception se;
			Object[] valueArray = null;

			if (inputDesc_ == null) {
				se = TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
						"batch_command_failed", null);
				throw new BatchUpdateException(se.getMessage(), se.getSQLState(), new int[0]);
			}
			if (connection_._isClosed()) {
				se = TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_connection",
						null);
				connection_.closeErroredConnection(se);
				throw new BatchUpdateException(se.getMessage(), se.getSQLState(), new int[0]);
			}
			if (isAnyLob_ && (connection_.getAutoCommit())) {
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
						"invalid_lob_commit_state", null);
			}

			int prc = usingRawRowset_ ? (paramRowCount_ + 1) : paramRowCount_;

			if (paramRowCount_ < 1) {
				if (!connection_.props_.getDelayedErrorMode()) {
					return (new int[] {});
				}
			}

			try {
				if (!usingRawRowset_)
					valueArray = getValueArray();

				execute(prc, inputDesc_.length, valueArray, queryTimeout_, lobObjects_ != null);


			} catch (SQLException e) {
				BatchUpdateException be;
				se = TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
						"batch_command_failed", e.getMessage());
				if (batchRowCount_ == null) // we failed before execute
				{
					batchRowCount_ = new int[paramRowCount_];
					Arrays.fill(batchRowCount_, -3);
				}
				be = new BatchUpdateException(se.getMessage(), se.getSQLState(), batchRowCount_);
				be.setNextException(e);

				throw be;
			}

			if (connection_.props_.getDelayedErrorMode()) {
				_lastCount = paramRowCount_;
			}

			return batchRowCount_;
		} finally {
			clearBatch();
		}
	}

	public ResultSet executeQuery() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "executeQuery", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("executeQuery");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		Object[] valueArray = null;
		int inDescLength = 0;

		validateExecuteInvocation();
		if (inputDesc_ != null) {
			if (!usingRawRowset_)
				valueArray = getValueArray();
			inDescLength = inputDesc_.length;
		}
		execute(paramRowCount_ + 1, inDescLength, valueArray, queryTimeout_, isAnyLob_); // LOB
		// Support
		// - SB
		// 9/28/04
		return resultSet_[result_set_offset];
	}

	public int executeUpdate() throws SQLException {
		long count = executeUpdate64();

		if (count > Integer.MAX_VALUE)
			this.setSQLWarning(null, "numeric_out_of_range", null);

		return (int) count;
	}

	public long executeUpdate64() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "executeUpdate", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("executeUpdate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		Object[] valueArray = null;
		int inDescLength = 0;

		validateExecuteInvocation();
		if (usingRawRowset_ == false) {
			if (inputDesc_ != null) {
				if (!usingRawRowset_)
					valueArray = getValueArray();
				inDescLength = inputDesc_.length;
			}
		} else {
			valueArray = this.paramsValue_; // send it along raw in case we need
			// it
			paramRowCount_ -= 1; // we need to make sure that paramRowCount
			// stays exactly what we set it to since we
			// add one during execute
		}

		// *******************************************************************
		// * If LOB is involved with autocommit enabled we throw an exception
		// *******************************************************************
		if (isAnyLob_ && (connection_.getAutoCommit())) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_lob_commit_state", null);
		}

		execute(paramRowCount_ + 1, inDescLength, valueArray, queryTimeout_, isAnyLob_);
		if (isAnyLob_) {


		}
		return ist_.getRowCount();
	}

	public ResultSetMetaData getMetaData() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "getMetaData", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("getMetaData");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		if (outputDesc_ != null) {
			return new TrafT4ResultSetMetaData(this, outputDesc_);
		} else {
			return null;
		}
	}

	public ParameterMetaData getParameterMetaData() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "getParameterMetaData", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("getParameterMetaData");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (inputDesc_ != null) {
			return new TrafT4ParameterMetaData(this, inputDesc_);
		} else {
			return null;
		}
	}

	// JDK 1.2
	public void setArray(int parameterIndex, Array x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setArray", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setArray");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateSetInvocation(parameterIndex);
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "setArray()");
	}

	public void setAsciiStream(int parameterIndex, InputStream x, int length) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x, length);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setAsciiStream", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x, length);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setAsciiStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;

		validateSetInvocation(parameterIndex);

		dataType = inputDesc_[parameterIndex - 1].dataType_;

		switch (dataType) {


		case Types.CHAR:
		case Types.VARCHAR:
		case Types.LONGVARCHAR:
		case Types.CLOB:
		case Types.BLOB:
		case Types.BINARY: 
		case Types.VARBINARY: 
		case Types.LONGVARBINARY:  // At this time Database does not
			// have this column data type
			byte[] buffer = new byte[length];
			try {
				x.read(buffer);
			} catch (java.io.IOException e) {
				Object[] messageArguments = new Object[1];
				messageArguments[0] = e.getMessage();
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "io_exception",
						messageArguments);
			}

			try {
				addParamValue(parameterIndex, new String(buffer, "ASCII"));
			} catch (java.io.UnsupportedEncodingException e) {
				Object[] messageArguments = new Object[1];
				messageArguments[0] = e.getMessage();
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
						"unsupported_encoding", messageArguments);
			}
			break;
		default:
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_datatype_for_column", null);
		}
	}

	public void setBigDecimal(int parameterIndex, BigDecimal x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setBigDecimal", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setBigDecimal");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		validateSetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		int sqltype = inputDesc_[parameterIndex - 1].sqlDataType_;

		if (x != null) {
			if (sqltype == InterfaceResultSet.SQLTYPECODE_LARGEINT){
				Utility.checkDecimalTruncation(parameterIndex, connection_.getLocale(), x,
						inputDesc_[parameterIndex - 1].precision_, inputDesc_[parameterIndex - 1].scale_);
			}
			addParamValue(parameterIndex, x.toString());
		} else {
			addParamValue(parameterIndex, null);
		}
	}

	public void setBinaryStream(int parameterIndex, InputStream x, int length) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x, length);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setBinaryStream",
					"setBinaryStream", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x, length);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setBinaryStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;

		validateSetInvocation(parameterIndex);

		dataType = inputDesc_[parameterIndex - 1].dataType_;

		switch (dataType) {


		case Types.DOUBLE:
		case Types.DECIMAL:
		case Types.NUMERIC:
		case Types.FLOAT:
		case Types.BIGINT:
		case Types.INTEGER:
		case Types.SMALLINT:
		case Types.TINYINT:
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_datatype_for_column", null);
		case Types.CHAR:
		case Types.VARCHAR:
		case Types.LONGVARCHAR:
		case Types.BLOB:
		case Types.CLOB:
		case Types.BINARY:
		case Types.VARBINARY:
		case Types.LONGVARBINARY:  // At this time Database does not
			// have this column data type
			byte[] buffer2 = new byte[length];

			try {
				int temp = x.read(buffer2);
			} catch (java.io.IOException e) {
				Object[] messageArguments = new Object[1];
				messageArguments[0] = e.getMessage();
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "io_exception",
						messageArguments);
			}
			addParamValue(parameterIndex, buffer2);
			break;
		default:
			byte[] buffer = new byte[length];

			try {
				x.read(buffer);
			} catch (java.io.IOException e) {
				Object[] messageArguments = new Object[1];
				messageArguments[0] = e.getMessage();
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "io_exception",
						messageArguments);
			}

			// addParamValue(parameterIndex, new String(buffer));
			// just pass the raw buffer.
			addParamValue(parameterIndex, buffer);
		}
	}

	/*
	 * Sets the designated parameter to the given <tt>Blob</tt> object. The
	 * driver converts this to an SQL <tt>BLOB</tt> value when it sends it to
	 * the database.
	 *
	 * @param i the first parameter is 1, the second is 2, ... @param x a <tt>Blob</tt>
	 * object that maps an SQL <tt>BLOB</tt> value
	 *
	 * @throws SQLException invalid data type for column
	 */
	public void setBlob(int parameterIndex, Blob x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setBlob", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setBlob");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;

		validateSetInvocation(parameterIndex);
		dataType = inputDesc_[parameterIndex - 1].dataType_;
		switch (dataType) {
                case Types.CHAR:
                case Types.VARCHAR:
                case Types.LONGVARCHAR:
		case Types.BLOB:
		case Types.CLOB:
                case Types.BINARY:
                case Types.VARBINARY:
            addParamValue(parameterIndex, x.getBytes(1, (int) x.length()));
		    break;

		default:
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_datatype_for_column", null);
		}
	}

	public void setBoolean(int parameterIndex, boolean x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setBoolean", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setBoolean");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateSetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		int sqltype = inputDesc_[parameterIndex - 1].sqlDataType_;
		Object valueObj = null;
		if (sqltype == InterfaceResultSet.SQLTYPECODE_BOOLEAN) {
			if (x) {
				valueObj = 1;
			} else {
				valueObj = 0;
			}
		} else {
			if (x) {
				valueObj = "1";
			} else {
				valueObj = "0";
			}
		}
		addParamValue(parameterIndex, valueObj);
	}

	public void setByte(int parameterIndex, byte x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setByte", "setByte", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setByte");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateSetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		addParamValue(parameterIndex, Byte.toString(x));
	}

	public void setBytes(int parameterIndex, byte[] x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setBytes", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setBytes");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;
		long dataLocator;

		byte[] tmpArray = new byte[x.length];
		System.arraycopy(x, 0, tmpArray, 0, x.length);
		validateSetInvocation(parameterIndex);
		dataType = inputDesc_[parameterIndex - 1].dataType_;
		switch (dataType) {


		case Types.CHAR:
		case Types.VARCHAR:
		case Types.LONGVARCHAR:
		case Types.BINARY:
		case Types.VARBINARY:
		case Types.LONGVARBINARY:
		case Types.BLOB:
		case Types.CLOB:
			addParamValue(parameterIndex, tmpArray);
			break;
		default:
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}
	}

	public void setCharacterStream(int parameterIndex, Reader reader, int length) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, reader, length);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setCharacterStream", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, reader, length);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setCharacterStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		char[] value;
		int dataType;

		validateSetInvocation(parameterIndex);
		dataType = inputDesc_[parameterIndex - 1].dataType_;
		switch (dataType) {


		case Types.DECIMAL:
		case Types.DOUBLE:
		case Types.FLOAT:
		case Types.NUMERIC:
		case Types.BIGINT:
		case Types.INTEGER:
		case Types.SMALLINT:
		case Types.TINYINT:
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_datatype_for_column", null);

		default:
			value = new char[length];
			try {
				int valuePos = reader.read(value);
				if (valuePos < 1) {
					Object[] messageArguments = new Object[1];
					messageArguments[0] = "No data to read from the Reader";
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "io_exception",
							messageArguments);
				}

				while (valuePos < length) {
					char temp[] = new char[length - valuePos];
					int tempReadLen = reader.read(temp, 0, length - valuePos);
					System.arraycopy(temp, 0, value, valuePos, tempReadLen);
					valuePos += tempReadLen;
				}
			} catch (java.io.IOException e) {
				Object[] messageArguments = new Object[1];
				messageArguments[0] = e.getMessage();
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "io_exception",
						messageArguments);
			}
			addParamValue(parameterIndex, new String(value));
			break;
		}
	}

	/**
	 * Sets the designated parameter to the given <tt>Clob</tt> object. The
	 * driver converts this to an SQL <tt>CLOB</tt> value when it sends it to
	 * the database.
	 *
	 * @param parameterIndex
	 *            the first parameter is 1, the second is 2, ...
	 * @param x
	 *            a <tt>Clob</tt> object that maps an SQL <tt>CLOB</tt>
	 *
	 * @throws SQLException
	 *             invalid data type for column, or restricted data type.
	 */
	public void setClob(int parameterIndex, Clob x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setClob", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setClob");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;
		long dataLocator;

		validateSetInvocation(parameterIndex);
		dataType = inputDesc_[parameterIndex - 1].dataType_;
		switch (dataType) {
                case Types.CHAR:
                case Types.VARCHAR:
                case Types.LONGVARCHAR:
                case Types.BLOB:
                case Types.CLOB:
                case Types.BINARY:
                case Types.VARBINARY:
            addParamValue(parameterIndex, x.getSubString(1, (int) x.length()));
            break;
		default:
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}
	}

	public void setDate(int parameterIndex, Date x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setDate", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setDate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;
		Timestamp t1;

		validateSetInvocation(parameterIndex);
		dataType = inputDesc_[parameterIndex - 1].dataType_;
		if (dataType != Types.CHAR && dataType != Types.VARCHAR && dataType != Types.LONGVARCHAR
				&& dataType != Types.DATE && dataType != Types.TIMESTAMP) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}
		if (x != null) {
			if (dataType == Types.TIMESTAMP) {
				t1 = new Timestamp(x.getTime());
				addParamValue(parameterIndex, t1.toString());
			} else {
				addParamValue(parameterIndex, x.toString());
			}
		} else {
			addParamValue(parameterIndex, null);
		}
	}

	public void setDate(int parameterIndex, Date x, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setDate", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setDate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;
		long dateValue;
		Date adjustedDate;
		Timestamp t1;

		validateSetInvocation(parameterIndex);
		dataType = inputDesc_[parameterIndex - 1].dataType_;
		if (dataType != Types.CHAR && dataType != Types.VARCHAR && dataType != Types.LONGVARCHAR
				&& dataType != Types.DATE && dataType != Types.TIME && dataType != Types.TIMESTAMP) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}
		// Ignore the cal, since SQL would expect it to store it in the local
		// time zone
		if (x != null) {
			if (dataType == Types.TIMESTAMP) {
				t1 = new Timestamp(x.getTime());
				addParamValue(parameterIndex, t1.toString());
			} else {
				addParamValue(parameterIndex, x.toString());
			}
		} else {
			addParamValue(parameterIndex, null);

		}
	}

	public void setDouble(int parameterIndex, double x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setDouble", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setDouble");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateSetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		addParamValue(parameterIndex, Double.toString(x));
		inputDesc_[parameterIndex - 1].isValueSet_ = true;
	}

	public void setFloat(int parameterIndex, float x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setFloat", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setFloat");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateSetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		addParamValue(parameterIndex, Float.toString(x));
	}

	public void setInt(int parameterIndex, int x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setInt", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setInt");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateSetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		addParamValue(parameterIndex, Integer.toString(x));
	}

	public void setLong(int parameterIndex, long x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setLong", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setLong");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateSetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		addParamValue(parameterIndex, Long.toString(x));
	}

	private void setLong(int parameterIndex, BigDecimal x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setLong", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setLong");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateSetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		addParamValue(parameterIndex, x);
	}

	public void setNull(int parameterIndex, int sqlType) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, sqlType);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setNull", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, sqlType);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setNull");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateSetInvocation(parameterIndex);
		addParamValue(parameterIndex, null);
	}

	public void setNull(int paramIndex, int sqlType, String typeName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, paramIndex, sqlType, typeName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setNull", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, paramIndex, sqlType, typeName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setNull");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		setNull(paramIndex, sqlType);
	}

	public void setObject(int parameterIndex, Object x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setObject", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (x == null) {
			setNull(parameterIndex, Types.NULL);
		} else if (x instanceof BigDecimal) {
			setBigDecimal(parameterIndex, (BigDecimal) x);
		} else if (x instanceof java.sql.Date) {
			setDate(parameterIndex, (Date) x);
		} else if (x instanceof java.sql.Time) {
			setTime(parameterIndex, (Time) x);
		} else if (x instanceof java.sql.Timestamp) {
			setTimestamp(parameterIndex, (Timestamp) x);
		} else if (x instanceof Double) {
			setDouble(parameterIndex, ((Double) x).doubleValue());
		} else if (x instanceof Float) {
			setFloat(parameterIndex, ((Float) x).floatValue());
		} else if (x instanceof Long) {
			setLong(parameterIndex, ((Long) x).longValue());
		} else if (x instanceof Integer) {
			setInt(parameterIndex, ((Integer) x).intValue());
		} else if (x instanceof Short) {
			setShort(parameterIndex, ((Short) x).shortValue());
		} else if (x instanceof Byte) {
			setByte(parameterIndex, ((Byte) x).byteValue());
		} else if (x instanceof Boolean) {
			setBoolean(parameterIndex, ((Boolean) x).booleanValue());
		} else if (x instanceof String) {
			setString(parameterIndex, x.toString());
		} else if (x instanceof byte[]) {
			setBytes(parameterIndex, (byte[]) x);
		} else if (x instanceof Clob) {
			setClob(parameterIndex, (Clob) x);
		} else if (x instanceof Blob) {
			setBlob(parameterIndex, (Blob) x);
			/*
			 * else if (x instanceof DataWrapper) {
			 * validateSetInvocation(parameterIndex); setObject(parameterIndex,
			 * x, inputDesc_[parameterIndex - 1].dataType_); }
			 */
		} else if (x instanceof BigInteger) {
			setBigDecimal(parameterIndex, new BigDecimal((BigInteger) x));
		} else {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"object_type_not_supported", null);
		}
	}

	public void setObject(int parameterIndex, Object x, int targetSqlType) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x, targetSqlType);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setObject", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x, targetSqlType);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		setObject(parameterIndex, x, targetSqlType, -1);
	}

	public void setObject(int parameterIndex, Object x, int targetSqlType, int scale) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x, targetSqlType, scale);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setObject", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x, targetSqlType, scale);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		BigDecimal tmpbd;
		int precision;
		Locale locale = connection_.getLocale();

		if (x == null) {
			setNull(parameterIndex, Types.NULL);
		} else {
			int type = inputDesc_[parameterIndex - 1].sqlDataType_;
			switch (targetSqlType) {
			case Types.CHAR:
			case Types.VARCHAR:
			case Types.LONGVARCHAR:
			case Types.CLOB:
				setString(parameterIndex, x.toString());
				break;
			case Types.NCHAR:
			case Types.NVARCHAR:
			    setNString(parameterIndex, x.toString());
			    break;
			case Types.VARBINARY:
			case Types.BINARY:
			case Types.LONGVARBINARY:
			case Types.BLOB:
				setBytes(parameterIndex, (byte[]) x);
				break;
			case Types.TIMESTAMP:
				if (x instanceof Timestamp) {
					setTimestamp(parameterIndex, (Timestamp) x);
				} else if (x instanceof Date) {
					setTimestamp(parameterIndex, Timestamp.valueOf(x.toString() + " 00:00:00.0"));
				} else {
					setString(parameterIndex, x.toString());
				}
				break;
			case Types.TIME:
				if (x instanceof Time) {
					setTime(parameterIndex, (Time) x);
				} else if (x instanceof Date) {
					setTime(parameterIndex, new Time(((Date) x).getTime()));
				} else if (x instanceof Timestamp) {
					setTime(parameterIndex, new Time(((Timestamp) x).getTime()));
				} else {
					setString(parameterIndex, x.toString());
				}
				break;
			case Types.DATE:
				try {
					if (x instanceof Date) {
						setDate(parameterIndex, (Date) x);
					} else if (x instanceof Time) {
						setDate(parameterIndex, new Date(((Time) x).getTime()));
					} else if (x instanceof Timestamp) {
						setDate(parameterIndex, new Date(((Timestamp) x).getTime()));
					} else {
						setDate(parameterIndex, Date.valueOf(x.toString()));
					}
				} catch (IllegalArgumentException iex) {
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"invalid_parameter_value", x.toString());
				}
				break;
			case Types.BOOLEAN:
				setBoolean(parameterIndex, (Boolean.valueOf(x.toString())).booleanValue());
				break;
			case Types.SMALLINT:
				tmpbd = Utility.getBigDecimalValue(locale, x);
				Utility.checkShortBoundary(locale, tmpbd);
				//Utility.checkLongTruncation(parameterIndex, tmpbd);
				setShort(parameterIndex, tmpbd.shortValue());
				break;
			case Types.INTEGER:
				tmpbd = Utility.getBigDecimalValue(locale, x);
				//Utility.checkLongTruncation(parameterIndex, tmpbd);
				Utility.checkIntegerBoundary(locale, tmpbd);
				setInt(parameterIndex, tmpbd.intValue());
				break;
			case Types.BIGINT:
				tmpbd = Utility.getBigDecimalValue(locale, x);
				//Utility.checkLongTruncation(parameterIndex, tmpbd);
				if (type == InterfaceResultSet.SQLTYPECODE_LARGEINT_UNSIGNED){
                	Utility.checkUnsignedLongBoundary(locale, tmpbd);
					setLong(parameterIndex, tmpbd);
				} else if (type == InterfaceResultSet.SQLTYPECODE_INTEGER_UNSIGNED) {
				    // if data is unsigned int ,the java.sql.type is -5 (bigint), our sql type is -401
				    Utility.checkUnsignedIntegerBoundary(locale, tmpbd);
				    setLong(parameterIndex, tmpbd);
				} else{
					Utility.checkLongBoundary(locale, tmpbd);
					setLong(parameterIndex, tmpbd.longValue());
				}
				break;
			case Types.DECIMAL:
				// precision = getPrecision(parameterIndex - 1);
				tmpbd = Utility.getBigDecimalValue(locale, x);
				tmpbd = Utility.setScale(tmpbd, scale, BigDecimal.ROUND_HALF_EVEN);
				// Utility.checkDecimalBoundary(locale, tmpbd, precision);
				setBigDecimal(parameterIndex, tmpbd);
				break;
			case Types.NUMERIC:
				// precision = getPrecision(parameterIndex - 1);
				tmpbd = Utility.getBigDecimalValue(locale, x);
				tmpbd = Utility.setScale(tmpbd, scale, BigDecimal.ROUND_HALF_EVEN);
				// Utility.checkDecimalBoundary(locale, tmpbd, precision);
				setBigDecimal(parameterIndex, tmpbd);
				break;
			case Types.TINYINT:
				tmpbd = Utility.getBigDecimalValue(locale, x);
				tmpbd = Utility.setScale(tmpbd, scale, roundingMode_);
				if (type == InterfaceResultSet.SQLTYPECODE_TINYINT_UNSIGNED) {
					Utility.checkUnsignedTinyintBoundary(locale, tmpbd);
				} else {
					Utility.checkSignedTinyintBoundary(locale, tmpbd);
				}
				setShort(parameterIndex, tmpbd.shortValue());
				break;
			case Types.FLOAT:
				tmpbd = Utility.getBigDecimalValue(locale, x);
				Utility.checkFloatBoundary(locale, tmpbd);
				setDouble(parameterIndex, tmpbd.doubleValue());
				break;
			case Types.DOUBLE:
				tmpbd = Utility.getBigDecimalValue(locale, x);
				Utility.checkDoubleBoundary(locale, tmpbd);
				setDouble(parameterIndex, tmpbd.doubleValue());
				break;
			case Types.REAL:
				tmpbd = Utility.getBigDecimalValue(locale, x);
				setFloat(parameterIndex, tmpbd.floatValue());
				break;
			case Types.OTHER:
				if (inputDesc_[parameterIndex].fsDataType_ == InterfaceResultSet.SQLTYPECODE_INTERVAL) {
					if (x instanceof byte[]) {
						addParamValue(parameterIndex, x);
					} else if (x instanceof String) {
						addParamValue(parameterIndex, x);
					} else {
						throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
								"conversion_not_allowed", null);
					}
					break;
				}
			case Types.ARRAY:
			case Types.BIT:
			case Types.DATALINK:
			case Types.DISTINCT:
			case Types.JAVA_OBJECT:
			case Types.STRUCT:
			default:
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
						"object_type_not_supported", null);
			}
		}
	}

	// JDK 1.2
	public void setRef(int i, Ref x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, i, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setRef", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, i, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setRef");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateSetInvocation(i);
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "setRef()");
	}

	public void setShort(int parameterIndex, short x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setShort", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setShort");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateSetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		addParamValue(parameterIndex, Short.toString(x));
	}

	public void setString(int parameterIndex, String x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setString", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setString");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateSetInvocation(parameterIndex);
		int dataType = inputDesc_[parameterIndex - 1].dataType_;

		switch (dataType) {
		case Types.CHAR:
		case Types.VARCHAR:
		case Types.LONGVARCHAR:
		case Types.CLOB:
		case Types.DATE:
		case Types.TIME:
		case Types.TIMESTAMP:
                    //case Types.VARBINARY:
		case Types.OTHER: // This type maps to the Database
			// INTERVAL
			addParamValue(parameterIndex, x);
			break;


		case Types.ARRAY:
		case Types.BIT:
		case Types.DATALINK:
		case Types.JAVA_OBJECT:
		case Types.REF:
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"datatype_not_supported", null);
		case Types.BIGINT:
		case Types.INTEGER:
		case Types.SMALLINT:
		case Types.TINYINT:
		case Types.DECIMAL:
		case Types.NUMERIC:
			if (x != null) {
				x = x.trim(); // SQLJ is using numeric string with
				// leading/trailing whitespace
			}
			setObject(parameterIndex, x, dataType);
			break;
		case Types.BLOB:
		case Types.BOOLEAN:
		case Types.DOUBLE:
		case Types.FLOAT:
		case Types.LONGVARBINARY:
		case Types.NULL:
		case Types.REAL:
		case Types.BINARY:
		case Types.VARBINARY:
			setObject(parameterIndex, x, dataType);
			break;
		default:
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"fetch_output_inconsistent", null);
		}

	}

	public void setTime(int parameterIndex, Time x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setTime", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setTime");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;
		Timestamp t1;

		validateSetInvocation(parameterIndex);
		dataType = inputDesc_[parameterIndex - 1].dataType_;
		if (dataType != Types.CHAR && dataType != Types.VARCHAR && dataType != Types.LONGVARCHAR
				&& dataType != Types.TIME && dataType != Types.TIMESTAMP) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}
		if (x != null) {
			if (dataType == Types.TIMESTAMP) {
				t1 = new Timestamp(x.getTime());
				addParamValue(parameterIndex, t1.toString());
			} else {
				addParamValue(parameterIndex, x.toString());
			}
		} else {
			addParamValue(parameterIndex, null);
		}
	}

	public void setTime(int parameterIndex, Time x, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setTime", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setTime");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;
		long timeValue;
		Time adjustedTime;
		Timestamp t1;

		validateSetInvocation(parameterIndex);
		dataType = inputDesc_[parameterIndex - 1].dataType_;
		if (dataType != Types.CHAR && dataType != Types.VARCHAR && dataType != Types.LONGVARCHAR
				&& dataType != Types.TIME && dataType != Types.TIMESTAMP) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}
		// Ignore the cal, since SQL would expect it to store it in the local
		// time zone
		if (x != null) {
			if (dataType == Types.TIMESTAMP) {
				t1 = new Timestamp(x.getTime());
				addParamValue(parameterIndex, t1.toString());
			} else {
				addParamValue(parameterIndex, x.toString());
			}
		} else {
			addParamValue(parameterIndex, null);
		}
	}

	public void setTimestamp(int parameterIndex, Timestamp x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setTimestamp", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setTimestamp");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;
		Date d1;
		Time t1;

		validateSetInvocation(parameterIndex);
		dataType = inputDesc_[parameterIndex - 1].dataType_;
		if (dataType != Types.CHAR && dataType != Types.VARCHAR && dataType != Types.LONGVARCHAR
				&& dataType != Types.DATE && dataType != Types.TIME && dataType != Types.TIMESTAMP) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}
		if (x != null) {
			switch (dataType) {
			case Types.DATE:
				d1 = new Date(x.getTime());
				addParamValue(parameterIndex, d1.toString());
				break;
			case Types.TIME:
				t1 = new Time(x.getTime());
				addParamValue(parameterIndex, t1.toString());
				break;
			default:
				addParamValue(parameterIndex, x.toString());
				break;
			}
		} else {
			addParamValue(parameterIndex, null);
		}
	}

	public void setTimestamp(int parameterIndex, Timestamp x, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setTimestamp", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setTimestamp");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;
		long timeValue;
		Timestamp adjustedTime;
		Date d1;
		Time t1;

		validateSetInvocation(parameterIndex);
		dataType = inputDesc_[parameterIndex - 1].dataType_;
		if (dataType != Types.CHAR && dataType != Types.VARCHAR && dataType != Types.LONGVARCHAR
				&& dataType != Types.DATE && dataType != Types.TIME && dataType != Types.TIMESTAMP) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}
		// Ignore the cal, since SQL would expect it to store it in the local
		// time zone
		if (x != null) {
			switch (dataType) {
			case Types.DATE:
				d1 = new Date(x.getTime());
				addParamValue(parameterIndex, d1.toString());
				break;
			case Types.TIME:
				t1 = new Time(x.getTime());
				addParamValue(parameterIndex, t1.toString());
				break;
			default:
				addParamValue(parameterIndex, x.toString());
				break;
			}
		} else {
			addParamValue(parameterIndex, null);
		}
	}

	public void setUnicodeStream(int parameterIndex, InputStream x, int length) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x, length);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setUnicodeStream", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x, length);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setUnicodeStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		byte[] buffer = new byte[length]; // length = number of bytes in
		// stream
		validateSetInvocation(parameterIndex);
		String s;

		if (x == null) {
			addParamValue(parameterIndex, null);
		} else {
			int dataType = inputDesc_[parameterIndex - 1].dataType_;
			switch (dataType) {
			case Types.DECIMAL:
			case Types.DOUBLE:
			case Types.FLOAT:
			case Types.NUMERIC:
			case Types.SMALLINT:
			case Types.INTEGER:
			case Types.BIGINT:
			case Types.TINYINT:
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
						"invalid_datatype_for_column", null);
			default:
				try {
					x.read(buffer, 0, length);
				} catch (java.io.IOException e) {
					Object[] messageArguments = new Object[1];
					messageArguments[0] = e.getMessage();
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "io_exception",
							messageArguments);
				}
				try {
					s = new String(buffer, "UnicodeBig");
					addParamValue(parameterIndex, s);
				} catch (java.io.UnsupportedEncodingException e) {
					Object[] messageArguments = new Object[1];
					messageArguments[0] = e.getMessage();
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"unsupported_encoding", messageArguments);
				}
				break;
			}
		}
	}

	public void setURL(int parameterIndex, URL x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setURL", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setURL");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateSetInvocation(parameterIndex);
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "setURL()");
	} // end setURL

	// -------------------------------------------------------------------------------------------
	/**
	 * This method will associate user defined data with the prepared statement.
	 * The user defined data must be in SQL/MX rowwise rowset format.
	 *
	 * @param numRows
	 *            the number of rows contained in buffer
	 * @param buffer
	 *            a buffer containing the rows
	 *
	 * @exception A
	 *                SQLException is thrown
	 */
	public void setDataBuffer(int numRows, ByteBuffer buffer) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, numRows, buffer);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setDataBuffer", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, numRows, buffer);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("setDataBuffer");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		usingRawRowset_ = true;
		paramRowCount_ = numRows;
		rowwiseRowsetBuffer_ = buffer;
	} // end setDataBufferBuffer

	// -------------------------------------------------------------------------------------------

	// Other methods
	protected void validateExecuteInvocation() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4PreparedStatement", "validateExecuteInvocation", "", p);
		}
		clearWarnings();
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "stmt_closed", null);
		}
		// connection_.getServerHandle().isConnectionOpen();
		connection_.isConnectionOpen();
		// close the previous resultset, if any
		for (int i = 0; i < num_result_sets_; i++) {
			if (resultSet_[i] != null) {
				resultSet_[i].close();
			}
		}
		if (paramRowCount_ > 0 && usingRawRowset_ == false) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"function_sequence_error", null);
		}

		if (usingRawRowset_ == false)
			checkIfAllParamsSet();

	}

	private void checkIfAllParamsSet() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4PreparedStatement", "checkIfAllParamsSet", "", p);
		}
		int paramNumber;

		if (inputDesc_ == null) {
			return;
		}
		for (paramNumber = 0; paramNumber < inputDesc_.length; paramNumber++) {
			if (!inputDesc_[paramNumber].isValueSet_) {
				Object[] messageArguments = new Object[2];
				messageArguments[0] = new Integer(paramNumber + 1);
				messageArguments[1] = new Integer(paramRowCount_ + 1);
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "parameter_not_set",
						messageArguments);
			}
		}
	}

	private void validateSetInvocation(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4PreparedStatement", "validateSetInvocation", "", p);
		}
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "stmt_closed", null);
		}
		// connection_.getServerHandle().isConnectionOpen();
		connection_.isConnectionOpen();
		if (inputDesc_ == null) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_parameter_index", null);
		}
		if (parameterIndex < 1 || parameterIndex > inputDesc_.length) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_parameter_index", null);
		}
		if (inputDesc_[parameterIndex - 1].paramMode_ == DatabaseMetaData.procedureColumnOut) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "is_a_output_parameter",
					null);
		}
	}

	void addParamValue(int parameterIndex, Object x) {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4PreparedStatement", "addParamValue", "", p);
		}

		paramsValue_[parameterIndex - 1] = x;
		inputDesc_[parameterIndex - 1].isValueSet_ = true;
	}

	Object[] getValueArray() {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4PreparedStatement", "getValueArray", "", p);
		}
		Object[] valueArray;
		int length;
		int i;
		int j;
		int index;
		Object[] rows;

		if (paramRowCount_ > 0) {
			valueArray = new Object[(paramRowCount_ + 1) * inputDesc_.length];
			length = rowsValue_.size();
			for (i = 0, index = 0; i < length; i++) {
				rows = (Object[]) rowsValue_.get(i);
				for (j = 0; j < rows.length; j++, index++) {
					valueArray[index] = rows[j];
				}
			}
		} else {
			valueArray = paramsValue_;
		}
		return valueArray;
	}

	void logicalClose() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4PreparedStatement", "logicalClose", "", p);
		}
		isClosed_ = true;
		if (rowsValue_ != null) {
			rowsValue_.clear();

		}
		if (lobObjects_ != null) {
			lobObjects_.clear();

		}
		paramRowCount_ = 0;
		for (int i = 0; i < num_result_sets_; i++) {
			if (resultSet_[i] != null) {
				resultSet_[i].close();
				// Clear the isValueSet_ flag in inputDesc_
			}
		}
		result_set_offset = 0;
		resultSet_[result_set_offset] = null;
		if (inputDesc_ != null) {
			for (int i = 0; i < inputDesc_.length; i++) {
				inputDesc_[i].isValueSet_ = false;
				paramsValue_[i] = null;
			}
		}
		isAnyLob_ = false;
		if (!connection_.closePreparedStatement(connection_, sql_, resultSetType_, resultSetConcurrency_,
				resultSetHoldability_)) {
			this.close(true); // if the statement is not in the cache
			// hardclose it afterall
		}

	}

	// ----------------------------------------------------------------------------------
	// Method used by JNI Layer to update the results of Prepare
	void setPrepareOutputs(TrafT4Desc[] inputDesc, TrafT4Desc[] outputDesc, int inputParamCount, int outputParamCount)
			throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, inputDesc, outputDesc, inputParamCount,
					outputParamCount);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4PreparedStatement", "setPrepareOutputs", "", p);
		}
		inputDesc_ = inputDesc;
		outputDesc_ = outputDesc;
		paramRowCount_ = 0;

		// Prepare updares inputDesc_ and outputDesc_
		if (inputDesc_ != null) {
			paramsValue_ = new Object[inputDesc_.length];
		} else {
			paramsValue_ = null;
		}
	} // end setPrepareOutputs

	// ----------------------------------------------------------------------------------
	void setPrepareOutputs2(TrafT4Desc[] inputDesc, TrafT4Desc[] outputDesc, int inputParamCount, int outputParamCount,
			int inputParamsLength, int outputParamsLength, int inputDescLength, int outputDescLength)
			throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, inputDesc, outputDesc, inputParamCount,
					outputParamCount, inputParamsLength, outputParamsLength, inputDescLength, outputDescLength);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4PreparedStatement", "setPrepareOutputs2", "", p);
		}
		inputParamCount_ = inputParamCount;
		outputParamCount_ = outputParamCount;
		inputParamsLength_ = inputParamsLength;
		outputParamsLength_ = outputParamsLength;
		inputDescLength_ = inputDescLength;
		outputDescLength_ = outputDescLength;
		setPrepareOutputs(inputDesc, outputDesc, inputParamCount, outputParamCount);
	} // end setPrepareOutputs2

	// ----------------------------------------------------------------------------------
	// Method used by JNI layer to update the results of Execute
	void setExecuteOutputs(int rowCount) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, rowCount);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4PreparedStatement", "setExecuteOutputs", "", p);
		}
		batchRowCount_ = new int[1];
		batchRowCount_[0] = rowCount;
		num_result_sets_ = 1;
		result_set_offset = 0;
		if (outputDesc_ != null) {
			resultSet_[result_set_offset] = new TrafT4ResultSet(this, outputDesc_);
		} else {
			resultSet_[result_set_offset] = null;
		}
	}

	/*
	 * //----------------------------------------------------------------------------------
	 * void setExecuteSingletonOutputs(SQLValue_def[] sqlValue_def_array, short
	 * rowsAffected) throws SQLException { batchRowCount_ = new int[1];
	 * batchRowCount_[0] = rowsAffected; if (outputDesc_ != null) { resultSet_ =
	 * new TrafT4ResultSet(this, outputDesc_); } else { resultSet_ = null; } if
	 * (rowsAffected == 0) { resultSet_.setFetchOutputs(new ObjectRow[0], 0, true, 0); }
	 * else { resultSet_.irs_.setSingletonFetchOutputs(resultSet_, rowsAffected,
	 * true, 0, sqlValue_def_array); } }
	 */

	// ----------------------------------------------------------------------------------
	// Method used by JNI layer to update the results of Execute
	void setExecuteBatchOutputs(int[] rowCount) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, rowCount);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4PreparedStatement", "setExecuteBatchOutputs", "", p);
		}
		num_result_sets_ = 1;
		result_set_offset = 0;
		if (outputDesc_ != null) {
			resultSet_[result_set_offset] = new TrafT4ResultSet(this, outputDesc_);
		} else {
			resultSet_[result_set_offset] = null;
		}
		batchRowCount_ = rowCount;
	}

	void reuse(TrafT4Connection connection, int resultSetType, int resultSetConcurrency, int resultSetHoldability)
			throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, resultSetType,
					resultSetConcurrency, resultSetHoldability);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4PreparedStatement", "reuse", "", p);
		}
		if (resultSetType != ResultSet.TYPE_FORWARD_ONLY && resultSetType != ResultSet.TYPE_SCROLL_INSENSITIVE
				&& resultSetType != ResultSet.TYPE_SCROLL_SENSITIVE) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_resultset_type", null);
		}
		if (resultSetType == ResultSet.TYPE_SCROLL_SENSITIVE) {
			resultSetType_ = ResultSet.TYPE_SCROLL_INSENSITIVE;
			setSQLWarning(null, "scrollResultSetChanged", null);
		} else {
			resultSetType_ = resultSetType;
		}
		if (resultSetConcurrency != ResultSet.CONCUR_READ_ONLY && resultSetConcurrency != ResultSet.CONCUR_UPDATABLE) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_resultset_concurrency", null);
		}
		resultSetConcurrency_ = resultSetConcurrency;
		resultSetHoldability_ = resultSetHoldability;
		queryTimeout_ = connection_.getServerHandle().getQueryTimeout();
		fetchSize_ = TrafT4ResultSet.DEFAULT_FETCH_SIZE;
		maxRows_ = 0;
		fetchDirection_ = ResultSet.FETCH_FORWARD;
		isClosed_ = false;
	}

	public void close(boolean hardClose) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, hardClose);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "close", "", p);
		}

		if (connection_._isClosed()) {
			return;
		}
		try {
			if (hardClose) {
				ist_.close();
			} else {
				logicalClose();
			}
		} catch (SQLException e) {
			performConnectionErrorChecks(e);
			throw e;
		} finally {
			isClosed_ = true;
			if (hardClose) {
				connection_.removeElement(pRef_);
			}
		}

	}


	TrafT4PreparedStatement(TrafT4Connection connection, String sql, String stmtLabel) throws SQLException {
		this(connection, sql, ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY, connection.holdability_,
				stmtLabel);
		connection.ic_.t4props_.setUseArrayBinding(false);
		connection.ic_.t4props_.setBatchRecovery(false);
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "",
					"Note, this call is before previous constructor call.", p);
		}
        if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
	}

	// Constructors with access specifier as "default"
	TrafT4PreparedStatement(TrafT4Connection connection, String sql) throws SQLException {
		this(connection, sql, ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY, connection.holdability_);
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "",
					"Note, this call is before previous constructor call.", p);
		}
        if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
	}

	TrafT4PreparedStatement(TrafT4Connection connection, String sql, int resultSetType, int resultSetConcurrency)
			throws SQLException {
		this(connection, sql, resultSetType, resultSetConcurrency, connection.holdability_);
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql, resultSetType,
					resultSetConcurrency);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "",
					"Note, this call is before previous constructor call.", p);
		}
        if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql, resultSetType,
					resultSetConcurrency);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

	}
	TrafT4PreparedStatement(TrafT4Connection connection, String sql, int resultSetType, int resultSetConcurrency,
			int resultSetHoldability, String stmtLabel) throws SQLException {
		super(connection, resultSetType, resultSetConcurrency, resultSetHoldability, stmtLabel);
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql, resultSetType,
					resultSetConcurrency, resultSetHoldability);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "",
					"Note, this call is before previous constructor call.", p);
		}
        if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql, resultSetType,
					resultSetConcurrency, resultSetHoldability);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		// connection_.getServerHandle().isConnectionOpen();
		connection_.isConnectionOpen();
		sqlStmtType_ = ist_.getSqlStmtType(sql);
		if (sqlStmtType_ == TRANSPORT.TYPE_STATS) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"infostats_invalid_error", null);
		} else if (sqlStmtType_ == TRANSPORT.TYPE_CONFIG) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"config_cmd_invalid_error", null);
		}
		sql_ = sql;


		// stmtLabel_ = generateStmtLabel();
		stmtLabel_ = stmtLabel;
		// System.out.println("TrafT4PreparedStatement stmtLabel_ " + stmtLabel_);

		usingRawRowset_ = false;
	}

	TrafT4PreparedStatement(TrafT4Connection connection, String sql, int resultSetType, int resultSetConcurrency,
			int resultSetHoldability) throws SQLException {
		super(connection, resultSetType, resultSetConcurrency, resultSetHoldability);
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql, resultSetType,
					resultSetConcurrency, resultSetHoldability);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "",
					"Note, this call is before previous constructor call.", p);
		}
        if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql, resultSetType,
					resultSetConcurrency, resultSetHoldability);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		// connection_.getServerHandle().isConnectionOpen();
		connection_.isConnectionOpen();
		sqlStmtType_ = TRANSPORT.TYPE_UNKNOWN;
		sql_ = sql;


		//stmtLabel_ = generateStmtLabel();

		usingRawRowset_ = false;
	}

	// Interface methods
	public void prepare(String sql, int queryTimeout, int holdability) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql, queryTimeout, holdability);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "prepare", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql, queryTimeout, holdability);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4PreparedStatement");
			lr.setSourceMethodName("prepare");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		try {
			super.ist_.prepare(sql, queryTimeout, this);
                        sqlStmtType_ = ist_.getSqlStmtType(sql);
		} catch (SQLException e) {
			performConnectionErrorChecks(e);
			throw e;
		}
	};

	public void setFetchSize(int rows) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, rows);
			connection_.props_.t4Logger_.logp(Level.FINE, "SQLMXPreparedStatement", "setFetchSize", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, rows);
			lr.setParameters(p);
			lr.setSourceClassName("SQLMXPreparedStatement");
			lr.setSourceMethodName("setFetchSize");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		if (rows < 0) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_fetchSize_value", null);
		}
		if (rows > 0) {
			fetchSize_ = rows;
		}
		// If the value specified is zero, then the hint is ignored.
	}

	private void execute(int paramRowCount, int paramCount, Object[] paramValues, int queryTimeout, boolean isAnyLob
	) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, paramRowCount, paramCount, paramValues,
					queryTimeout, isAnyLob);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4PreparedStatement", "execute", "", p);
		}
		try {
			ist_.execute(TRANSPORT.SRVR_API_SQLEXECUTE2, paramRowCount, paramCount, paramValues, queryTimeout, null,
					this);
		} catch (SQLException e) {
			performConnectionErrorChecks(e);
			throw e;
		}
	};

	/*
	 * protected void setSingleton(boolean value) { singleton_ = value; }
	 * protected boolean getSingleton() { return singleton_; }
	 */

	/**
	 * Use this method to retrieve the statement type that was used when
	 * creating the statement through the connectivity service. ie. SELECT,
	 * UPDATE, DELETE, INSERT.
	 */
	public String getStatementType() {
		String stmtType = "";
		switch (sqlStmtType_) {
		case TRANSPORT.TYPE_SELECT:
			stmtType = "SELECT";
			break;
		case TRANSPORT.TYPE_UPDATE:
			stmtType = "UPDATE";
			break;
		case TRANSPORT.TYPE_DELETE:
			stmtType = "DELETE";
			break;
		case TRANSPORT.TYPE_INSERT:
		case TRANSPORT.TYPE_INSERT_PARAM:
			stmtType = "INSERT";
			break;
		case TRANSPORT.TYPE_CREATE:
			stmtType = "CREATE";
			break;
		case TRANSPORT.TYPE_GRANT:
			stmtType = "GRANT";
			break;
		case TRANSPORT.TYPE_DROP:
			stmtType = "DROP";
			break;
		case TRANSPORT.TYPE_CALL:
			stmtType = "CALL";
			break;
		case TRANSPORT.TYPE_EXPLAIN:
			stmtType = "EXPLAIN";
			break;
		case TRANSPORT.TYPE_STATS:
			stmtType = "INFOSTATS";
			break;
		case TRANSPORT.TYPE_CONFIG:
			stmtType = "CONFIG";
			break;
		default:
			stmtType = "";
			break;
		}

		return stmtType;
	}

	/**
	 * Use this method to retrieve the statement type that was used when
	 * creating the statement through the connectivity service. ie. SELECT,
	 * UPDATE, DELETE, INSERT.
	 */
	public short getStatementTypeShort() {
		return sqlStmtType_;
	}

	/**
	 * Use this method to retrieve the statement type that was used when
	 * creating the statement through the connectivity service. ie. SELECT,
	 * UPDATE, DELETE, INSERT.
	 */
	public int getStatementTypeInt() {
		return ist_.getSqlQueryType();
	}

	ArrayList getKeyColumns() {
		return keyColumns;
	}

	void setKeyColumns(ArrayList keyColumns) {
		this.keyColumns = keyColumns;
	}

	ArrayList keyColumns = null;

	int paramRowCount_;
	String moduleName_;
	int moduleVersion_;
	long moduleTimestamp_;
	boolean isAnyLob_;
	ArrayList lobObjects_;

	ArrayList rowsValue_;
	Object[] paramsValue_;

	// boolean singleton_ = false;

	// ================ SQL Statement type ====================
	public static final short TYPE_UNKNOWN = 0;
	public static final short TYPE_SELECT = 0x0001;
	public static final short TYPE_UPDATE = 0x0002;
	public static final short TYPE_DELETE = 0x0004;
	public static final short TYPE_INSERT = 0x0008;
	public static final short TYPE_EXPLAIN = 0x0010;
	public static final short TYPE_CREATE = 0x0020;
	public static final short TYPE_GRANT = 0x0040;
	public static final short TYPE_DROP = 0x0080;
	public static final short TYPE_INSERT_PARAM = 0x0100;
	public static final short TYPE_SELECT_CATALOG = 0x0200;
	public static final short TYPE_SMD = 0x0400;
	public static final short TYPE_CALL = 0x0800;
	public static final short TYPE_STATS = 0x1000;
	public static final short TYPE_CONFIG = 0x2000;

	// =================== SQL Query ===================
	public static final int SQL_OTHER = -1;
	public static final int SQL_UNKNOWN = 0;
	public static final int SQL_SELECT_UNIQUE = 1;
	public static final int SQL_SELECT_NON_UNIQUE = 2;
	public static final int SQL_INSERT_UNIQUE = 3;
	public static final int SQL_INSERT_NON_UNIQUE = 4;
	public static final int SQL_UPDATE_UNIQUE = 5;
	public static final int SQL_UPDATE_NON_UNIQUE = 6;
	public static final int SQL_DELETE_UNIQUE = 7;
	public static final int SQL_DELETE_NON_UNIQUE = 8;
	public static final int SQL_CONTROL = 9;
	public static final int SQL_SET_TRANSACTION = 10;
	public static final int SQL_SET_CATALOG = 11;
	public static final int SQL_SET_SCHEMA = 12;

	// =================== new identifiers ===================
	public static final int SQL_CREATE_TABLE = SQL_SET_SCHEMA + 1;
	public static final int SQL_CREATE_VIEW = SQL_CREATE_TABLE + 1;
	public static final int SQL_CREATE_INDEX = SQL_CREATE_VIEW + 1;
	public static final int SQL_CREATE_UNIQUE_INDEX = SQL_CREATE_INDEX + 1;
	public static final int SQL_CREATE_SYNONYM = SQL_CREATE_UNIQUE_INDEX + 1;
	public static final int SQL_CREATE_VOLATILE_TABLE = SQL_CREATE_SYNONYM + 1;;
	public static final int SQL_CREATE_MV = SQL_CREATE_VOLATILE_TABLE + 1;
	public static final int SQL_CREATE_MVG = SQL_CREATE_MV + 1;
	public static final int SQL_CREATE_MP_ALIAS = SQL_CREATE_MVG + 1;
	public static final int SQL_CREATE_PROCEDURE = SQL_CREATE_MP_ALIAS + 1;
	public static final int SQL_CREATE_TRIGGER = SQL_CREATE_PROCEDURE + 1;
	public static final int SQL_CREATE_SET_TABLE = SQL_CREATE_TRIGGER + 1;
	public static final int SQL_CREATE_MULTISET_TABLE = SQL_CREATE_SET_TABLE + 1;

	public static final int SQL_DROP_TABLE = SQL_CREATE_MULTISET_TABLE + 1;
	public static final int SQL_DROP_VIEW = SQL_DROP_TABLE + 1;
	public static final int SQL_DROP_INDEX = SQL_DROP_VIEW + 1;
	public static final int SQL_DROP_SYNONYM = SQL_DROP_INDEX + 1;
	public static final int SQL_DROP_VOLATILE_TABLE = SQL_DROP_SYNONYM + 1;;
	public static final int SQL_DROP_MV = SQL_DROP_VOLATILE_TABLE + 1;
	public static final int SQL_DROP_MVG = SQL_DROP_MV + 1;
	public static final int SQL_DROP_MP_ALIAS = SQL_DROP_MVG + 1;
	public static final int SQL_DROP_PROCEDURE = SQL_DROP_MP_ALIAS + 1;
	public static final int SQL_DROP_TRIGGER = SQL_DROP_PROCEDURE + 1;
	public static final int SQL_DROP_SET_TABLE = SQL_DROP_TRIGGER + 1;
	public static final int SQL_DROP_MULTISET_TABLE = SQL_DROP_SET_TABLE + 1;

	public static final int SQL_ALTER_TABLE = SQL_DROP_MULTISET_TABLE + 1;
	public static final int SQL_ALTER_INDEX = SQL_ALTER_TABLE + 1;
	public static final int SQL_ALTER_TRIGGER = SQL_ALTER_INDEX + 1;
	public static final int SQL_ALTER_MP_ALIAS = SQL_ALTER_TRIGGER + 1;
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

	public Object unwrap(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public boolean isWrapperFor(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	public void setRowId(int parameterIndex, RowId x) throws SQLException {
		// TODO Auto-generated method stub

	}

	public void setNString(int parameterIndex, String value)
			throws SQLException {
	    if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
            Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, value);
            connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4PreparedStatement", "setNString", "", p);
        }

        validateSetInvocation(parameterIndex);
        int dataType = inputDesc_[parameterIndex - 1].dataType_;

        switch (dataType) {
        case Types.CHAR:
        case Types.VARCHAR:
            addParamValue(parameterIndex, value);
            break;
        default:
            throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
                    "fetch_output_inconsistent", null);
        }
	}

	public void setNCharacterStream(int parameterIndex, Reader value,
			long length) throws SQLException {
		// TODO Auto-generated method stub

	}

	public void setNClob(int parameterIndex, NClob value) throws SQLException {
		// TODO Auto-generated method stub

	}

	public void setClob(int parameterIndex, Reader reader, long length)
			throws SQLException {
	    setCharacterStream(parameterIndex, reader, length);
	}

	public void setBlob(int parameterIndex, InputStream inputStream, long length)
			throws SQLException {
		setBinaryStream(parameterIndex, inputStream, (int) length);
	}

	public void setNClob(int parameterIndex, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub

	}

	public void setSQLXML(int parameterIndex, SQLXML xmlObject)
			throws SQLException {
		// TODO Auto-generated method stub

	}

	public void setAsciiStream(int parameterIndex, InputStream x, long length)
			throws SQLException {
		// TODO Auto-generated method stub

	}

	public void setBinaryStream(int parameterIndex, InputStream x, long length)
			throws SQLException {
		setBinaryStream(parameterIndex, x, (int)length);
	}

	public void setCharacterStream(int parameterIndex, Reader reader,
			long length) throws SQLException {
		try {
		    char[] c = null;
		    if (length > 0) {
		        c = new char[(int)length];
		        int numRead = 0;

		        while (numRead < length) {
		            int count = reader.read(c, numRead, (int) (length - numRead));

		            if (count < 0) {
		                break;
		            }
		            numRead += count;
		        }
		    }
		    setString(parameterIndex, new String(c, 0, (int)length));
		}
		catch (Exception e) {

		}

	}

	public void setAsciiStream(int parameterIndex, InputStream x)
			throws SQLException {
		// TODO Auto-generated method stub

	}

	public void setBinaryStream(int parameterIndex, InputStream x)
			throws SQLException {
		addParamValue(parameterIndex, x);
	}

	public void setCharacterStream(int parameterIndex, Reader reader)
			throws SQLException {
	    try {
	        char[] c = null;
	        int len = 0;

	        c = new char[4096];
	        StringBuilder builder = new StringBuilder();

	        while ((len = reader.read(c)) != -1) {
	            builder.append(c, 0, len);
	        }

	        setString(parameterIndex, builder.toString());
	    }
	    catch (Exception e) {

	    }
	}

	public void setNCharacterStream(int parameterIndex, Reader value)
			throws SQLException {
		// TODO Auto-generated method stub

	}

	public void setClob(int parameterIndex, Reader reader) throws SQLException {
		setCharacterStream(parameterIndex, reader);
	}

	public void setBlob(int parameterIndex, InputStream inputStream)
			throws SQLException {
		setBinaryStream(parameterIndex, inputStream);
	}

	public void setNClob(int parameterIndex, Reader reader) throws SQLException {
		// TODO Auto-generated method stub

	}

}
