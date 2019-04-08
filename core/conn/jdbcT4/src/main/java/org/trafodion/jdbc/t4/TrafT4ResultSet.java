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
import java.io.UnsupportedEncodingException;
import java.lang.reflect.Method;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.net.URL;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.UnsupportedCharsetException;
import java.sql.Array;
import java.sql.Blob;
import java.sql.Clob;
import java.sql.Connection;
import java.sql.DataTruncation;
import java.sql.DatabaseMetaData;
import java.sql.Date;
import java.sql.NClob;
import java.sql.PreparedStatement;
import java.sql.Ref;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.RowId;
import java.sql.SQLException;
import java.sql.SQLXML;
import java.sql.Statement;
import java.sql.Time;
import java.sql.Timestamp;
import java.sql.Types;
import java.util.ArrayList;
import java.util.BitSet;
import java.util.Calendar;
import java.util.Map;
import java.util.HashMap;
import java.util.logging.Level;
import java.util.logging.LogRecord;

// ----------------------------------------------------------------------------
// This class partially implements the result set class as defined in 
// java.sql.ResultSet.  
// ----------------------------------------------------------------------------
public class TrafT4ResultSet extends TrafT4Handle implements java.sql.ResultSet {

	// java.sql.ResultSet interface methods
	public boolean absolute(int row) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, row);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "absolute", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, row);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("absolute");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		boolean flag = false;
		int absRow;

		clearWarnings();
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		if (getType() == ResultSet.TYPE_FORWARD_ONLY) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "forward_only_cursor",
					null);
		}
		if (row == 0) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_row_number",
					null);
		}


		if (row > 0) {
			if (row <= numRows_) {
				currentRow_ = row;
				isBeforeFirst_ = false;
				isAfterLast_ = false;
				onInsertRow_ = false;
				flag = true;
			} else {
				do {
					flag = next();
					if (!flag) {
						break;
					}
				} while (currentRow_ < row);
			}
		} else {
			absRow = -row;
			afterLast();
			if (absRow <= numRows_) {
				currentRow_ = numRows_ - absRow + 1;
				isAfterLast_ = false;
				isBeforeFirst_ = false;
				onInsertRow_ = false;
				flag = true;
			} else {
				beforeFirst();
			}
		}
		return flag;
	}

	public void afterLast() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "afterLast", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("afterLast");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		clearWarnings();
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		if (getType() == ResultSet.TYPE_FORWARD_ONLY) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "forward_only_cursor",
					null);
		}
		last();
		// currentRow_++;
		isAfterLast_ = true;
		isBeforeFirst_ = false;
	}

	public void beforeFirst() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "beforeFirst", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("beforeFirst");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		clearWarnings();
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		if (getType() == ResultSet.TYPE_FORWARD_ONLY) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "forward_only_cursor",
					null);
		}


		currentRow_ = 0;
		isBeforeFirst_ = true;
		isAfterLast_ = false;
		onInsertRow_ = false;
	}

        // Method not implemented
	public void cancelRowUpdates() throws SQLException {
             throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
                  "cancelRowUpdates - not supported", null);
	}

	/**
	 * Close the resultSet. This method is synchronized to prevent many threads
	 * talking to the same server after close().
	 * 
	 * @throws SQLException
	 */
	synchronized public void close() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "close", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("close");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		clearWarnings();
		if (isClosed_) {
			return;
		}
		if (connection_._isClosed()) {
			connection_.closeErroredConnection(null);
			return;
		}


		if (stmt_ instanceof org.trafodion.jdbc.t4.TrafT4PreparedStatement) {
			close(false);
		} else {
			close(true);
		}
	}

        // Method not implemented
	public void deleteRow() throws SQLException {
            throw TrafT4Messages.createSQLException(connection_.props_, 
                                                  connection_.getLocale(),
                                                  "deleteRow - not supported", 
                                                  null);
        }

	public int findColumn(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "findColumn", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("findColumn");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int i;

		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		for (i = 0; i < outputDesc_.length; i++) {
			if (columnName.equalsIgnoreCase(outputDesc_[i].name_)) {
				return i + 1;
			}
		}
		throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_column_name", null);
	}

	public boolean first() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "first", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("first");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		boolean flag = true;

		clearWarnings();
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		if (getType() == ResultSet.TYPE_FORWARD_ONLY) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "forward_only_cursor",
					null);
		}


		if (isBeforeFirst_) {
			flag = next();
		}
		if (numRows_ > 0) {
			currentRow_ = 1;
			isAfterLast_ = false;
			isBeforeFirst_ = false;
			onInsertRow_ = false;
		}
		return flag;
	}

	// JDK 1.2
	public Array getArray(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getArray", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getArray");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateGetInvocation(columnIndex);
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "getArray()");
		return null;
	}

	// JDK 1.2
	public Array getArray(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getArray", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getArray");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getArray(columnIndex);
	}

	public InputStream getAsciiStream(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getAsciiStream", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getAsciiStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;
		// For LOB Support - SB 10/8/2004
		int dataType;


		dataType = outputDesc_[columnIndex - 1].dataType_;
		switch (dataType) {


		case Types.CHAR:
		case Types.VARCHAR:
		case Types.LONGVARCHAR:
		case Types.BINARY:
		case Types.VARBINARY:
		case Types.LONGVARBINARY:
		case Types.BLOB:
		case Types.CLOB:
			data = getLocalString(columnIndex);
			if (data != null) {
				try {
					return new java.io.DataInputStream(new java.io.ByteArrayInputStream(data.getBytes("ASCII")));
				} catch (java.io.UnsupportedEncodingException e) {
					Object[] messageArguments = new Object[1];
					messageArguments[0] = e.getMessage();
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"unsupported_encoding", messageArguments);
				}
			} else {
				return null;
			}
		default:
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}

	}

	public InputStream getAsciiStream(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getAsciiStream", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getAsciiStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getAsciiStream(columnIndex);
	}

	public BigDecimal getBigDecimal(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getBigDecimal", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getBigDecimal");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;

		String data;
		BigDecimal retValue;
		Double d;

		validateGetInvocation(columnIndex);
		outputDesc_[columnIndex - 1].checkValidNumericConversion(connection_.getLocale());
		// String returned may not be numeric in case of SQL_CHAR, SQL_VARCHAR
		// and SQL_LONGVARCHAR
		// fields. Hoping that java might throw invalid value exception
		data = getLocalString(columnIndex);
		if (data != null) {
			data = data.trim();
			try {
				retValue = new BigDecimal(data);
			} catch (NumberFormatException e) {
				try {
					d = new Double(data);
				} catch (NumberFormatException e1) {
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"invalid_cast_specification", null);
				}
				retValue = new BigDecimal(d.doubleValue());
			}
			return retValue;
		} else {
			return null;
		}
	}

	public BigDecimal getBigDecimal(int columnIndex, int scale) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, scale);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getBigDecimal", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, scale);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getBigDecimal");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		BigDecimal retValue;

		retValue = getBigDecimal(columnIndex);
		if (retValue != null) {
			return retValue.setScale(scale);
		} else {
			return null;
		}
	}

	public BigDecimal getBigDecimal(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getBigDecimal", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getBigDecimal");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getBigDecimal(columnIndex);
	}

	public BigDecimal getBigDecimal(String columnName, int scale) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, scale);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getBigDecimal", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, scale);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getBigDecimal");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getBigDecimal(columnIndex, scale);
	}

	public InputStream getBinaryStream(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getBinaryStream", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getBinaryStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateGetInvocation(columnIndex);
		byte[] data;

		// For LOB Support - SB 10/8/2004
		int dataType;

		
		dataType = outputDesc_[columnIndex - 1].dataType_;
		switch (dataType) {
		

		case Types.CHAR:
		case Types.VARCHAR:
		case Types.LONGVARCHAR:
		case Types.BINARY:
		case Types.VARBINARY:
		case Types.LONGVARBINARY:
		case Types.BLOB:
		case Types.CLOB:
			data = getBytes(columnIndex);
			if (data != null) {
				return new java.io.ByteArrayInputStream(data);
			} else {
				return null;
			}
		default:
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}
	}

	public InputStream getBinaryStream(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getBinaryStream", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getBinaryStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getBinaryStream(columnIndex);
	}


	public boolean getBoolean(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getBoolean", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getBoolean");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;
		short shortValue;

		validateGetInvocation(columnIndex);
		outputDesc_[columnIndex - 1].checkValidNumericConversion(connection_.getLocale());
		data = getLocalString(columnIndex);
		if (data != null) {
			data = data.trim();
			if ((data.equalsIgnoreCase("true")) || (data.equalsIgnoreCase("1"))) {
				return true;
			} else if ((data.equalsIgnoreCase("false")) || (data.equalsIgnoreCase("false"))) {
				return false;
			} else {
				try {
					shortValue = getShort(columnIndex);
				} catch (NumberFormatException e) {
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"invalid_cast_specification", null);
				}
				switch (shortValue) {
				case 0:
					return false;
				case 1:
					return true;
				default:
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"numeric_out_of_range", shortValue);
				}
			}
		} else {
			return false;
		}
	}

	public boolean getBoolean(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getBoolean", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getBoolean");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getBoolean(columnIndex);
	}

	public byte getByte(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getByte", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getByte");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;
		byte retValue;
		Double d;
		double d1;

		validateGetInvocation(columnIndex);
		outputDesc_[columnIndex - 1].checkValidNumericConversion(connection_.getLocale());
		data = getLocalString(columnIndex);
		if (data != null) {
			try {
				retValue = Byte.parseByte(data);
			} catch (NumberFormatException e) {
				try {
					d = new Double(data);
				} catch (NumberFormatException e1) {
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"invalid_cast_specification", null);
				}

				d1 = d.doubleValue();
				// To allow -128.999.. and 127.999...
				if (d1 > (double) Byte.MIN_VALUE - 1 && d1 < (double) Byte.MAX_VALUE + 1) {
					retValue = d.byteValue();
					if ((double) retValue != d1) {
						setSQLWarning(null, "data_truncation", null);
					}
				} else {
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"numeric_out_of_range", d1);
				}
			}
			return retValue;
		} else {
			return 0;
		}
	}

	public byte getByte(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getByte", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getByte");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getByte(columnIndex);
	}

	public byte[] getBytes(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getBytes", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getBytes");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateGetInvocation(columnIndex);
		int dataType;


		dataType = outputDesc_[columnIndex - 1].dataType_;

		switch (dataType) {
		case Types.BINARY:
		case Types.VARBINARY:
		case Types.LONGVARBINARY:
		case Types.CHAR:
		case Types.VARCHAR: // Extension allows varchar and
		case Types.LONGVARCHAR: // longvarchar data types

			Object x = getCurrentRow().getUpdatedArrayElement(columnIndex);
			if (x == null) {
				wasNull_ = true;
				return null;
			} else {
				wasNull_ = false;
				if (x instanceof byte[]) {
					return (byte[]) x;
				} else if (x instanceof String) {
					return ((String) x).getBytes();
				} else {
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"invalid_cast_specification", null);
				}
			}
                case Types.BLOB:
                case Types.CLOB:
                    x = getLocalString(columnIndex);
                    Blob blob = new TrafT4Blob(connection_, (String) x, null);
                    return blob.getBytes(1, (int)blob.length());
		default:
                    throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
                                                            null);
		}
	}

	public byte[] getBytes(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getBytes", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getBytes");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getBytes(columnIndex);
	}

	public Reader getCharacterStream(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getCharacterStream", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getCharacterStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;
		int dataType;


		validateGetInvocation(columnIndex);
		dataType = outputDesc_[columnIndex - 1].dataType_;
		switch (dataType) {

			
		case Types.CHAR:
		case Types.VARCHAR:
		case Types.LONGVARCHAR:
		case Types.BINARY:
		case Types.VARBINARY:
		case Types.LONGVARBINARY:
			data = getString(columnIndex);
			if (data != null) {
				return new java.io.StringReader(data);
			} else {
				return null;
			}
                case Types.BLOB:
                case Types.CLOB:
                    Clob clob = getClob(columnIndex);
                    return clob.getCharacterStream(); 
		default:
                    throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}

	}

	public Reader getCharacterStream(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getCharacterStream", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getCharacterStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getCharacterStream(columnIndex);
	}

	public int getConcurrency() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getConcurrency", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getConcurrency");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		if (stmt_ != null) {
			return stmt_.resultSetConcurrency_;
		} else {
			return ResultSet.CONCUR_READ_ONLY;
		}
	}

	public String getCursorName() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getCursorName", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getCursorName");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		if (spj_rs_ && stmtLabel_ != null) {
			return stmtLabel_;
		} else if (stmt_ != null) {
			String cursorName;
			cursorName = stmt_.cursorName_;
			if (cursorName == null || cursorName.trim().equals("")) {
				cursorName = stmt_.stmtLabel_;
			}
			return cursorName;
		} else {
			return null;
		}
	}

	// wm_merge - AM
	static String convertDateFormat(String dt) {
		String tokens[] = dt.split("[/]", 3);

		if (tokens.length != 3) {
			return dt;
		}
		StringBuffer sb = new StringBuffer();
		sb.append(tokens[0]).append("-").append(tokens[1]).append("-").append(tokens[2]);
		return sb.toString();
	}

	public Date getDate(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getDate", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getDate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;
		String data;
		Date retValue;
		int endIndex;

		validateGetInvocation(columnIndex);
		dataType = outputDesc_[columnIndex - 1].dataType_;
		if (dataType != Types.CHAR && dataType != Types.VARCHAR && dataType != Types.LONGVARCHAR
				&& dataType != Types.DATE && dataType != Types.TIMESTAMP) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}

		data = getLocalString(columnIndex);
		if (data != null) {
			try {
				boolean convertDate = connection_.getDateConversion();

				if (connection_.props_.t4Logger_.isLoggable(Level.FINEST) == true) {
					Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
					String temp = "Convert Date=" + convertDate;
					connection_.props_.t4Logger_.logp(Level.FINEST, "TrafT4ResultSet", "getDate", temp, p);
				}
				if (convertDate) {
					String dt = convertDateFormat(data);
					retValue = valueOf(dt);
				} else {
					retValue = Date.valueOf(data);
				}
			} catch (IllegalArgumentException e) {
				data = data.trim();
				if ((endIndex = data.indexOf(' ')) != -1) {
					data = data.substring(0, endIndex);
				}
				try {
					retValue = Date.valueOf(data);
					setSQLWarning(null, "data_truncation", null);

				} catch (IllegalArgumentException ex) {
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"invalid_cast_specification", null);
				}
			}
			return retValue;
		} else {
			return null;
		}
	}

	/* TODO: this is a horrible hack but what else can be done with random 2 digit/4 digit years for dates?
	 * Note: The date constructor wants (year-1900) as a parameter
	 * We use the following table for conversion:
	 * 
	 * 		Year Value		Assumed Year		Action
	 * 		<50 			Value + 2000		must add 100
	 * 		>=100			Value 				must subtract 1900
	 * 		>=50 			Value + 1900		no change in value needed
	 * 
	 */
	static Date valueOf(String s) {
		int year;
		int month;
		int day;
		int firstDash;
		int secondDash;

		if (s == null)
			throw new java.lang.IllegalArgumentException();

		firstDash = s.indexOf('-');
		secondDash = s.indexOf('-', firstDash + 1);
		if ((firstDash > 0) & (secondDash > 0) & (secondDash < s.length() - 1)) {
			year = Integer.parseInt(s.substring(0, firstDash));
			
			if (year < 50) {//handles 2 digit years: <50 assume 2000, >=50 assume 1900
				year += 100;
			}
			else if(year >= 100) { //handles 4 digit years
				year -= 1900;
			}
			
			month = Integer.parseInt(s.substring(firstDash + 1, secondDash)) - 1;
			day = Integer.parseInt(s.substring(secondDash + 1));
		} else {
			throw new java.lang.IllegalArgumentException();
		}

		return new Date(year, month, day);
	}

	public Date getDate(int columnIndex, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getDate", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getDate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		Date sqlDate;
		java.util.Date d;

		sqlDate = getDate(columnIndex);
		if (sqlDate != null) {
			if (cal != null) {
				cal.setTime(sqlDate);
				d = cal.getTime();
				sqlDate = new Date(d.getTime());
			}
			return sqlDate;
		} else {
			return (sqlDate);
		}
	}

	public Date getDate(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getDate", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getDate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getDate(columnIndex);
	}

	public Date getDate(String columnName, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getDate", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getDate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getDate(columnIndex, cal);
	}

	public double getDouble(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getDouble", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getDouble");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;

		validateGetInvocation(columnIndex);
		outputDesc_[columnIndex - 1].checkValidNumericConversion(connection_.getLocale());
		data = getLocalString(columnIndex);
		if (data != null) {
			try {
				return Double.parseDouble(data);
			} catch (NumberFormatException e1) {
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
						"invalid_cast_specification", null);
			}
		} else {
			return 0;
		}
	}

	public double getDouble(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getDouble", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getDouble");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getDouble(columnIndex);
	}

	public int getFetchDirection() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getFetchDirection", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getFetchDirection");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		return fetchDirection_;
	}

	public int getFetchSize() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getFetchSize", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getFetchSize");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		return fetchSize_;
	}

	public float getFloat(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getFloat", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getFloat");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		double data;
		validateGetInvocation(columnIndex);

		outputDesc_[columnIndex - 1].checkValidNumericConversion(connection_.getLocale());
		// parseFloat doesn't return error when
		// the value exceds the float max
		data = getDouble(columnIndex);
		if (data >= Float.NEGATIVE_INFINITY && data <= Float.POSITIVE_INFINITY) {
			return (float) data;
		} else {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "numeric_out_of_range",
			        data);
		}
	}

	public float getFloat(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getFloat", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getFloat");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getFloat(columnIndex);
	}

	public int getInt(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getInt", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getInt");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;
		int retValue;
		double d;

		validateGetInvocation(columnIndex);
		outputDesc_[columnIndex - 1].checkValidNumericConversion(connection_.getLocale());
		data = getLocalString(columnIndex);
		if (data != null) {
			try {
				retValue = Integer.parseInt(data);
			} catch (NumberFormatException e) {
				try {
					d = new Double(data).doubleValue();
				} catch (NumberFormatException e1) {
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"invalid_cast_specification", null);
				}

				if (d > (double) Integer.MIN_VALUE - 1 && d < (double) Integer.MAX_VALUE + 1) {
					retValue = (int) d;
					if ((double) retValue != d) {
						setSQLWarning(null, "data_truncation", null);
					}
				} else {
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"numeric_out_of_range", d);
				}
			}
		} else {
			retValue = 0;
		}

		return retValue;
	}

	public int getInt(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getInt", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getInt");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getInt(columnIndex);
	}

	public long getLong(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getLong", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getLong");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;
		long retValue;
		double d;

		BigDecimal bd;

		validateGetInvocation(columnIndex);
		outputDesc_[columnIndex - 1].checkValidNumericConversion(connection_.getLocale());
		data = getLocalString(columnIndex);

		if (data != null) {
			try {
				retValue = Long.parseLong(data);
			} catch (NumberFormatException e) {
				try {
					bd = new BigDecimal(data);
					retValue = bd.longValue();
					if (bd.compareTo(BigDecimal.valueOf(Long.MAX_VALUE)) <= 0
							&& bd.compareTo(BigDecimal.valueOf(Long.MIN_VALUE)) >= 0) {
						
						if (bd.compareTo(BigDecimal.valueOf(retValue)) != 0) {
							setSQLWarning(null, "data_truncation", null);
						}
					} else {
						throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
								"numeric_out_of_range", bd);
					}
				} catch (NumberFormatException e2) {

					try {
						d = new Double(data).doubleValue();
					} catch (NumberFormatException e1) {
						throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
								"invalid_cast_specification", null);
					}

					if (d >= Long.MIN_VALUE && d <= Long.MAX_VALUE) {
						retValue = (long) d;
						
						if ((double) retValue != d) {
							setSQLWarning(null, "data_truncation", null);
						}
					} else {
						throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
								"numeric_out_of_range", d);
					}
				}
			}
		} else {
			retValue = 0;
		}

		return retValue;
	}

	public long getLong(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getLong", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getLong");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getLong(columnIndex);
	}

	public ResultSetMetaData getMetaData() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getMetaData", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getMetaData");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		return new TrafT4ResultSetMetaData(this, outputDesc_);
	}

	public Object getObject(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getObject", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;
		int precision;
		byte byteValue;
		short shortValue;
		int intValue;
		long longValue;
		float floatValue;
		double doubleValue;
		boolean booleanValue;

		validateGetInvocation(columnIndex);
		dataType = outputDesc_[columnIndex - 1].dataType_;
		precision = outputDesc_[columnIndex - 1].sqlPrecision_;
		int sqltype = outputDesc_[columnIndex - 1].sqlDataType_;
		switch (dataType) {
		case Types.TINYINT:
			if (sqltype == InterfaceResultSet.SQLTYPECODE_TINYINT_UNSIGNED) {
				short s = getShort(columnIndex);
				if (wasNull_) {
					return null;
				}
				return s;
			} else {
				byte b = getByte(columnIndex);
				if (wasNull_) {
					return null;
				}
				return b;
			}
		case Types.SMALLINT:
			shortValue = getShort(columnIndex);
			if (wasNull_) {
				return null;
			} else {
				return new Short(shortValue);
			}
		case Types.INTEGER:
			intValue = getInt(columnIndex);
			if (wasNull_) {
				return null;
			} else {
				return new Integer(intValue);
			}
		case Types.BIGINT:
			if (sqltype == InterfaceResultSet.SQLTYPECODE_LARGEINT_UNSIGNED) {
				BigDecimal bd = getBigDecimal(columnIndex);
				if (wasNull_) {
					return null;
				}
				return bd;
			} else {
				long l = getLong(columnIndex);
				return l;
			}
		case Types.REAL:
			floatValue = getFloat(columnIndex);
			if (wasNull_) {
				return null;
			} else {
				return new Float(floatValue);
			}
		case Types.FLOAT:
		case Types.DOUBLE:
			doubleValue = getDouble(columnIndex);
			if (wasNull_) {
				return null;
			} else {
				return new Double(doubleValue);
			}
		case Types.DECIMAL:
		case Types.NUMERIC:
			return getBigDecimal(columnIndex);
		case Types.BIT:
			booleanValue = getBoolean(columnIndex);
			if (wasNull_) {
				return null;
			} else {
				return new Boolean(booleanValue);
			}
		case Types.CHAR:
			if (sqltype == InterfaceResultSet.SQLTYPECODE_BOOLEAN) {
				boolean b = getBoolean(columnIndex);
				if (wasNull_) {
					return null;
				}
				return b;
			}
		case Types.VARCHAR:
		case Types.LONGVARCHAR:
		case Types.BLOB:
		case Types.CLOB:
			return getString(columnIndex);
		case Types.BINARY:
		case Types.VARBINARY:
			return getBytes(columnIndex);
		case Types.LONGVARBINARY:
			return getBinaryStream(columnIndex);
		case Types.DATE:
			return getDate(columnIndex);
		case Types.TIME:
			if (precision > 0)
				return getString(columnIndex);

			return getTime(columnIndex);
		case Types.TIMESTAMP:
			return getTimestamp(columnIndex);
			// For LOB Support - SB 10/8/2004


		case Types.OTHER:
			return getString(columnIndex);
		default:
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}
	}

	// JDK 1.2
	public Object getObject(int columnIndex, Map map) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getObject", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateGetInvocation(columnIndex);
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "getObject()");
		return null;
	}

	public Object getObject(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getObject", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getObject(columnIndex);
	}

	// JDK 1.2
	public Object getObject(String columnName, Map map) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, map);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getObject", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, map);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getObject(columnIndex, map);
	}

	// JDK 1.2
	public Ref getRef(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getRef", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getRef");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateGetInvocation(columnIndex);
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "getRef()");
		return null;
	}

	// JDK 1.2
	public Ref getRef(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getRef", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getRef");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getRef(columnIndex);
	}

	public int getRow() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getRow", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getRow");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		clearWarnings();
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		if (isBeforeFirst_ || isAfterLast_ || onInsertRow_) {
			return 0;
		}

		if ((getType() == ResultSet.TYPE_FORWARD_ONLY) && (getConcurrency() == ResultSet.CONCUR_UPDATABLE)) {
			return currentRowCount_;
		}
		return currentRow_;
	}

	public short getShort(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getShort", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getShort");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		String data;
		short retValue;
		double d;

		validateGetInvocation(columnIndex);
		outputDesc_[columnIndex - 1].checkValidNumericConversion(connection_.getLocale());

		data = getLocalString(columnIndex);
		if (data != null) {
			try {
				retValue = Short.parseShort(data);
			} catch (NumberFormatException e) {
				try {
					d = new Double(data).doubleValue();
				} catch (NumberFormatException e1) {
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"invalid_cast_specification", null);
				}

				if (d > (double) Short.MIN_VALUE - 1 && d < (double) Short.MAX_VALUE + 1) {
					retValue = (short) d;
					if ((double) retValue != d)

					{
						setSQLWarning(null, "data_truncation", null);
					}
				} else {
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"numeric_out_of_range", d);
				}
			}

		} else {
			retValue = 0;
		}

		return retValue;
	}

	public short getShort(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getShort", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getShort");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getShort(columnIndex);
	}

	public Statement getStatement() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getStatement", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getStatement");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		return stmt_;
	}

	public String getString(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getString", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getString");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;
		int targetSqlType;
		int precision;
		Object x;
		ObjectArray currentRow;

		validateGetInvocation(columnIndex);
		currentRow = getCurrentRow();
		x = currentRow.getUpdatedArrayElement(columnIndex);

		if (x == null) {
			wasNull_ = true;
			return null;
		}

		wasNull_ = false;
		targetSqlType = outputDesc_[columnIndex - 1].dataType_;
		precision = outputDesc_[columnIndex - 1].sqlPrecision_;
		int sqltype = outputDesc_[columnIndex - 1].sqlDataType_;
		switch (targetSqlType) {


		case Types.CHAR:
			if (sqltype == InterfaceResultSet.SQLTYPECODE_BOOLEAN) {
				return String.valueOf(getBoolean(columnIndex));
			}
		case Types.VARCHAR:
		case Types.LONGVARCHAR:
			data = getLocalString(columnIndex);
			if (stmt_ != null && stmt_.maxFieldSize_ != 0) {
				if (data.length() > stmt_.maxFieldSize_) {
					data = data.substring(0, stmt_.maxFieldSize_);
				}
			}
			break;
        case Types.BLOB:
            data = getLocalString(columnIndex);
            if ( !connection_.props_.getUseLobHandle() && data != null) {
                Blob blob = new TrafT4Blob(connection_, data, null);
                data = new String((blob.getBytes(1, (int) blob.length())));
            }
            break;
        case Types.CLOB:
            data = getLocalString(columnIndex);
            if ( !connection_.props_.getUseLobHandle() && data != null) {
                Clob clob = new TrafT4Clob(connection_, data, null);
                data =  clob.getSubString(1, (int)clob.length());
            }
            break;
		case Types.VARBINARY:
		case Types.BINARY:
		case Types.LONGVARBINARY:
			data = String.valueOf(getBytes(columnIndex));
			break;
		case Types.TIMESTAMP:
			Timestamp t = getTimestamp(columnIndex);
			data = "" + t.getNanos();
			int l = data.length();
			data = t.toString();
			
			if(precision > 0) {
				for(int i=0;i<precision-l;i++)
					data += '0';
			} else {
				data = data.substring(0,data.lastIndexOf('.'));
			}

			break;
		case Types.TIME:
			if (precision > 0)
				data = x.toString();
			else
				data = String.valueOf(getTime(columnIndex));
			break;
		case Types.DATE:
			data = String.valueOf(getDate(columnIndex));
			break;
		case Types.BOOLEAN:
			data = String.valueOf(getBoolean(columnIndex));
			break;
		case Types.SMALLINT:
			data = String.valueOf(getShort(columnIndex));
			break;
		case Types.TINYINT:
			if (sqltype == InterfaceResultSet.SQLTYPECODE_TINYINT_UNSIGNED) {
				data = String.valueOf(getShort(columnIndex));
			} else {
				data = String.valueOf(getByte(columnIndex));
			}
			break;
		case Types.REAL:
			data = String.valueOf(getFloat(columnIndex));
			break;
		case Types.DOUBLE:
		case Types.FLOAT:
			data = String.valueOf(getDouble(columnIndex));
			break;
		case Types.DECIMAL:
		case Types.NUMERIC:
	        BigDecimal bd = getBigDecimal(columnIndex);
	        if (_javaVersion >= 1.5) {
	            // as of Java 5.0 and above, BigDecimal.toPlainString() should be used.
	            try {
	                data = (String) _toPlainString.invoke(bd, (Object[]) null);
	            } catch (Exception e) {
	            	data = bd.toString();
	            }
	        } else {
	        	data = bd.toString();
	        }			
			break;
		case Types.BIGINT:
			if (sqltype == InterfaceResultSet.SQLTYPECODE_LARGEINT_UNSIGNED) {
				data = String.valueOf(getBigDecimal(columnIndex));
			} else {
				data = String.valueOf(getLong(columnIndex));
			}
			break;
		case Types.INTEGER:
			data = String.valueOf(getInt(columnIndex));
			break;
		case Types.OTHER: {
			if (x instanceof byte[]) {
				try {
					data = new String((byte[]) x, "ASCII");
				} catch (Exception e) {
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"unsupported_encoding", "ASCII");
				}
			} else {
				data = x.toString();
			}
			// only 2 supported today
			// 1. SQLTYPECODE_INTERVAL
			// 2. SQLTYPECODE_DATETIME
			// Within DATETIME we check for only the SQL/MP specific data types
			// in another switch-case statement
			switch (outputDesc_[columnIndex - 1].fsDataType_) {
			case InterfaceResultSet.SQLTYPECODE_INTERVAL: {
				// if data does no start with a hyphen (representing a negative
				// sign)
				// then send back data without the byte that holds the hyphen
				// Reason: for Interval data types first byte is holding either
				// the
				// a negative sign or if number is positive, it is just an extra
				// space
				data = Utility.trimRightZeros(data);
				if (!data.startsWith(hyphen_string)) {
					data = data.substring(1);
				}
			}
				break;
			case InterfaceResultSet.SQLTYPECODE_DATETIME: {
				switch (outputDesc_[columnIndex - 1].sqlDatetimeCode_) {
				case TrafT4Desc.SQLDTCODE_YEAR:
				case TrafT4Desc.SQLDTCODE_YEAR_TO_MONTH:
				case TrafT4Desc.SQLDTCODE_MONTH:
				case TrafT4Desc.SQLDTCODE_MONTH_TO_DAY:
				case TrafT4Desc.SQLDTCODE_DAY:
				case TrafT4Desc.SQLDTCODE_HOUR:
				case TrafT4Desc.SQLDTCODE_HOUR_TO_MINUTE:
				case TrafT4Desc.SQLDTCODE_MINUTE:
				case TrafT4Desc.SQLDTCODE_MINUTE_TO_SECOND:
					// case TrafT4Desc.SQLDTCODE_MINUTE_TO_FRACTION:
				case TrafT4Desc.SQLDTCODE_SECOND:
					// case TrafT4Desc.SQLDTCODE_SECOND_TO_FRACTION:
				case TrafT4Desc.SQLDTCODE_YEAR_TO_HOUR:
				case TrafT4Desc.SQLDTCODE_YEAR_TO_MINUTE:
				case TrafT4Desc.SQLDTCODE_MONTH_TO_HOUR:
				case TrafT4Desc.SQLDTCODE_MONTH_TO_MINUTE:
				case TrafT4Desc.SQLDTCODE_MONTH_TO_SECOND:
					// case TrafT4Desc.SQLDTCODE_MONTH_TO_FRACTION:
				case TrafT4Desc.SQLDTCODE_DAY_TO_HOUR:
				case TrafT4Desc.SQLDTCODE_DAY_TO_MINUTE:
				case TrafT4Desc.SQLDTCODE_DAY_TO_SECOND:
					// case TrafT4Desc.SQLDTCODE_DAY_TO_FRACTION:
				case TrafT4Desc.SQLDTCODE_HOUR_TO_FRACTION:
					break;
				default:
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"object_type_not_supported", null);
				}
			}
				break;
			default:
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
						"object_type_not_supported", null);
			}
		}
			break;
		case Types.ARRAY:
		case Types.BIT:
		case Types.REF:
		case Types.DATALINK:
		case Types.DISTINCT:
		case Types.JAVA_OBJECT:
		case Types.STRUCT:
		default:
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"object_type_not_supported", null);
		}
		return data;
	}

	public String getString(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getString", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getString");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getString(columnIndex);
	}

	public Time getTime(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getTime", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getTime");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;
		String data;
		Time retValue;
		Timestamp timestamp;

		validateGetInvocation(columnIndex);
		dataType = outputDesc_[columnIndex - 1].dataType_;
		if (dataType != Types.CHAR && dataType != Types.VARCHAR && dataType != Types.LONGVARCHAR
				&& dataType != Types.TIME && dataType != Types.TIMESTAMP) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}
		data = getLocalString(columnIndex);
		if (data != null) {
			switch (dataType) {
			case Types.TIMESTAMP:
				try {
					timestamp = Timestamp.valueOf(data);
					retValue = new Time(timestamp.getTime());
				} catch (IllegalArgumentException e) {
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"invalid_cast_specification", null);
				}
				break;
			case Types.CHAR:
			case Types.VARCHAR:
			case Types.LONGVARCHAR:
		        case Types.BLOB:
		        case Types.CLOB:
				data = data.trim(); // Fall Thru
			case Types.TIME:
				try {
					retValue = Time.valueOf(data);
				} catch (IllegalArgumentException e) {
					try {
						timestamp = Timestamp.valueOf(data);
						retValue = new Time(timestamp.getTime());
					} catch (IllegalArgumentException ex) {
						throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
								"invalid_cast_specification", null);
					}
				}
				break;
			default:
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
						"restricted_data_type", null);
			}
			return retValue;
		} else {
			return null;
		}
	}

	public Time getTime(int columnIndex, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getTime", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getTime");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		Time sqlTime;
		java.util.Date d;

		sqlTime = getTime(columnIndex);
		if (sqlTime != null) {
			if (cal != null) {
				cal.setTime(sqlTime);
				d = cal.getTime();
				sqlTime = new Time(d.getTime());
			}
			return sqlTime;
		} else {
			return (sqlTime);
		}
	}

	public Time getTime(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getTime", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getTime");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getTime(columnIndex);
	}

	public Time getTime(String columnName, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getTime", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getTime");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getTime(columnIndex, cal);
	}

	public Timestamp getTimestamp(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getTimestamp", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getTimestamp");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;
		String data;
		Timestamp retValue;
		Date dateValue;
		Time timeValue;

		validateGetInvocation(columnIndex);
		dataType = outputDesc_[columnIndex - 1].dataType_;
		if (dataType != Types.CHAR && dataType != Types.VARCHAR && dataType != Types.LONGVARCHAR
				&& dataType != Types.DATE && dataType != Types.TIME && dataType != Types.TIMESTAMP) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}
		data = getLocalString(columnIndex);
		if (data != null) {
			switch (dataType) {
			case Types.DATE:
				try {
					dateValue = Date.valueOf(data);
					retValue = new Timestamp(dateValue.getTime());
				} catch (IllegalArgumentException e) {
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"invalid_cast_specification", null);
				}
				break;
			case Types.CHAR:
			case Types.VARCHAR:
			case Types.LONGVARCHAR:
		        case Types.BLOB:
		        case Types.CLOB:
				data = data.trim();
			case Types.TIMESTAMP:
			case Types.TIME:
				try {
					retValue = Timestamp.valueOf(data);
				} catch (IllegalArgumentException e) {
					try {
						dateValue = Date.valueOf(data);
						retValue = new Timestamp(dateValue.getTime());
					} catch (IllegalArgumentException e1) {
						try {
							int nano = 0;
							if (outputDesc_[columnIndex - 1].sqlPrecision_ > 0) {
								nano = Integer.parseInt(data.substring(data.indexOf(".") + 1));
								nano *= Math.pow(10, 9 - outputDesc_[columnIndex - 1].sqlPrecision_);
								data = data.substring(0, data.indexOf("."));
							}

							timeValue = Time.valueOf(data);
							retValue = new Timestamp(timeValue.getTime());
							retValue.setNanos(nano);
						} catch (IllegalArgumentException e2) {
							throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
									"invalid_cast_specification", null);
						}

					}
				}
				break;
			default:
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
						"restricted_data_type", null);
			}
			return retValue;
		} else {
			return null;
		}
	}

	public Timestamp getTimestamp(int columnIndex, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getTimestamp", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getTimestamp");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		Timestamp sqlTimestamp;
		java.util.Date d;
		int nanos;

		sqlTimestamp = getTimestamp(columnIndex);
		if (sqlTimestamp != null) {
			if (cal != null) {
				nanos = sqlTimestamp.getNanos();
				cal.setTime(sqlTimestamp);
				d = cal.getTime();
				sqlTimestamp = new Timestamp(d.getTime());
				sqlTimestamp.setNanos(nanos);
			}
			return sqlTimestamp;
		} else {
			return (sqlTimestamp);
		}
	}

	public Timestamp getTimestamp(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getTimestamp", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getTimestamp");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getTimestamp(columnIndex);
	}

	public Timestamp getTimestamp(String columnName, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getTimestamp", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getTimestamp");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getTimestamp(columnIndex, cal);
	}

	public int getType() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getType", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getType");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		if (stmt_ != null) {
			return stmt_.resultSetType_;
		} else {
			return ResultSet.TYPE_FORWARD_ONLY;
		}

	}

	public InputStream getUnicodeStream(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getUnicodeStream", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getUnicodeStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;

		validateGetInvocation(columnIndex);
		data = getLocalString(columnIndex);
		if (data != null) {
			try {
				return new java.io.ByteArrayInputStream(data.getBytes((String) InterfaceUtilities
						.getCharsetName(InterfaceUtilities.SQLCHARSETCODE_UNICODE)));
			} catch (java.io.UnsupportedEncodingException e) {
				Object[] messageArguments = new Object[1];
				messageArguments[0] = e.getMessage();
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
						"unsupported_encoding", messageArguments);
			}
		} else {
			return null;
		}

	}

	public InputStream getUnicodeStream(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getUnicodeStream", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getUnicodeStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getUnicodeStream(columnIndex);
	}

	public URL getURL(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getURL", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getURL");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateGetInvocation(columnIndex);
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "getURL()");
		return null;
	}

	public URL getURL(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getURL", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getURL");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getURL(columnIndex);
	}

	public byte[] getRawBytes(int columnIndex) throws SQLException {
		TrafT4Desc desc;
		byte[] ret;

		if (!keepRawBuffer_) // if you dont set the property, we will not
			// support the call
			TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "getRawBytes()");

		validateGetInvocation(columnIndex); // verify columnIndex and that we
		// are not closed

		desc = outputDesc_[columnIndex - 1];
		int rowOffset = (currentRow_ - 1) * desc.rowLength_;

		if (desc.nullValue_ != -1
				&& Bytes
						.extractShort(rawBuffer_, desc.nullValue_ + rowOffset, this.stmt_.connection_.ic_.getByteSwap()) == -1) {
			ret = null;
		} else {
                       boolean shortLength = desc.maxLen_ < Math.pow(2, 15);
                       int dataOffset = ((shortLength) ? 2 : 4);
		       int maxLen = (desc.sqlDataType_ != InterfaceResultSet.SQLTYPECODE_VARCHAR_WITH_LENGTH && desc.sqlDataType_ != InterfaceResultSet.SQLTYPECODE_BLOB && desc.sqlDataType_ != InterfaceResultSet.SQLTYPECODE_CLOB) ? desc.maxLen_ : desc.maxLen_ + dataOffset;
	               ret = new byte[maxLen];
	               System.arraycopy(rawBuffer_, desc.noNullValue_ + rowOffset, ret, 0, maxLen);
		}
		return ret;
	}

	// ------------------------------------------------------------------
	/**
	 * This method will get the next available set of rows in rowwise rowset
	 * format.
	 * 
	 * @exception A
	 *                SQLException is thrown
	 * 
	 * @return A byte buffer containing the rows. Null is returned there are no
	 *         more rows.
	 * 
	 * @exception A
	 *                SQLException is thrown
	 */
	public byte[] getNextFetchBuffer() throws SQLException {
		boolean done = false;
		byte[] retValue = null;

		keepRawBuffer_ = true;
		while (fetchComplete_ == false && done == false)
			done = next();
		fetchComplete_ = false;

		if (done == false)
			retValue = null;
		else
			retValue = rawBuffer_;

		return retValue;
	}

	// ------------------------------------------------------------------
        // Method not implemented
	public void insertRow() throws SQLException {
           throw TrafT4Messages.createSQLException(connection_.props_, 
                                                 connection_.getLocale(),
                                                 "insertRow - not supported", 
                                                 null);
        }

	public boolean isAfterLast() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "isAfterLast", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("isAfterLast");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		return isAfterLast_;
	}

	public boolean isBeforeFirst() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "isBeforeFirst", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("isBeforeFirst");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		return isBeforeFirst_;
	}

	public boolean isFirst() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "isFirst", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("isFirst");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		if ((getType() == ResultSet.TYPE_FORWARD_ONLY) && (getConcurrency() == ResultSet.CONCUR_UPDATABLE)) {
			if (!onInsertRow_ && currentRowCount_ == 1) {
				return true;
			} else {
				return false;
			}
		} else {
			if (!onInsertRow_ && currentRow_ == 1) {
				return true;
			} else {
				return false;
			}
		}
	}

	public boolean isLast() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "isLast", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("isLast");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		/*
		 */
		if (!onInsertRow_ && endOfData_) {
			if (currentRow_ == numRows_) {
				// return true;
				return (isAfterLast_ ? false : true);
			}
			return false;
		}

		boolean found = next();
		if (found) {
			// previous();
			--currentRow_;
		} else {
			isAfterLast_ = false;
		}

		return (!found);
	}

	public boolean last() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "last", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("last");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		clearWarnings();
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		if (getType() == ResultSet.TYPE_FORWARD_ONLY) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "forward_only_cursor",
					null);
		}


		onInsertRow_ = false;
		if (endOfData_) {
			currentRow_ = numRows_;
			isBeforeFirst_ = false;
			isAfterLast_ = false;
		} else {
			while (next()) {
				;
			}
		}
		if (currentRow_ != 0) {
			isAfterLast_ = false;
			return true;
		} else {
			return false;
		}
	}

	public void moveToCurrentRow() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "moveToCurrentRow", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("moveToCurrentRow");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		clearWarnings();
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		if (!onInsertRow_) {
			return;
		} else {



			currentRow_ = savedCurrentRow_;
			onInsertRow_ = false;
			return;
		}
	}

        // Method not implemented
	public void moveToInsertRow() throws SQLException {
           throw TrafT4Messages.createSQLException(connection_.props_, 
                                                 connection_.getLocale(),
                                                 "moveToInsertRow - not supported", 
                                                 null);
	}

	public boolean next() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "next", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("next");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		boolean validRow = false;

		int maxRowCnt;
		int maxRows;
		int queryTimeout;

		clearWarnings();
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}


		onInsertRow_ = false;
		if (currentRow_ < numRows_) {
			validRow = true;
			currentRow_++;
			isBeforeFirst_ = false;
		} else {
			if (endOfData_) {
				isAfterLast_ = true;
				isBeforeFirst_ = false;
			} else {
				if (stmt_ != null) {
					maxRows = stmt_.maxRows_;
					queryTimeout = stmt_.queryTimeout_;
				} else {
					maxRows = 0;
					queryTimeout = 0;
				}

				if (maxRows == 0 || maxRows > totalRowsFetched_ + fetchSize_) {
					maxRowCnt = fetchSize_;
				} else {
					maxRowCnt = maxRows - totalRowsFetched_;
				}

				if (maxRowCnt == 0) {
					validRow = false;
				} else {
					try {
						validRow = irs_.fetch(stmtLabel_, maxRowCnt, queryTimeout, holdability_, this);
						fetchComplete_ = true;
					} catch (SQLException e) {
						performConnectionErrorChecks(e);
						throw e;
					}
				}
				if (validRow) {
					currentRow_++;
					isAfterLast_ = false;
					isBeforeFirst_ = false;
				} else {
					// In some cases endOfData_ is reached when fetch returns
					// false;
					endOfData_ = true;
					isAfterLast_ = true;
					isBeforeFirst_ = false;
				}
			}
		}
		if (validRow) {
			currentRowCount_++;
		} else {
			currentRowCount_ = 0;
		}

		return validRow;
	}

	public boolean previous() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "previous", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("previous");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		boolean validRow = false;
		clearWarnings();
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		if (getType() == ResultSet.TYPE_FORWARD_ONLY) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "forward_only_cursor",
					null);
		}


		onInsertRow_ = false;
		if (currentRow_ > 1) {
			// --currentRow_;
			currentRow_ = isAfterLast_ ? currentRow_ : --currentRow_;
			validRow = true;
			isBeforeFirst_ = false;
			isAfterLast_ = false;
		} else {
			currentRow_ = 0;
			isBeforeFirst_ = true;
			isAfterLast_ = false;
		}
		return validRow;
        }

        // Method not implemented
	public void refreshRow() throws SQLException {
           throw TrafT4Messages.createSQLException(connection_.props_, 
                                                 connection_.getLocale(),
                                                 "refreshRow - not supported", 
                                                 null);
	}

	public boolean relative(int row) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, row);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "relative", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, row);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("relative");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int absRow;
		int rowInc;
		boolean flag = false;

		clearWarnings();
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		if (getType() == ResultSet.TYPE_FORWARD_ONLY) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "forward_only_cursor",
					null);
		}
		


		onInsertRow_ = false;
		if (row > 0) {
			rowInc = 0;
			do {
				flag = next();
				if (!flag) {
					break;
				}
			} while (++rowInc < row);
		} else {
			absRow = -row;
			if (absRow < currentRow_) {
				currentRow_ -= absRow;
				isAfterLast_ = false;
				isBeforeFirst_ = false;
				flag = true;
			} else {
				beforeFirst();
			}
		}
		return flag;
	}

        // Method not implemented
	public boolean rowDeleted() throws SQLException {
           throw TrafT4Messages.createSQLException(connection_.props_, 
                                                 connection_.getLocale(),
                                                 "rowDeleted - not supported", 
                                                  null);
	}

        // Method not implemented
	public boolean rowInserted() throws SQLException {
           throw TrafT4Messages.createSQLException(connection_.props_, 
                                                 connection_.getLocale(),
                                                 "rowInserted - not supported", 
                                                  null);
	}

        // Method not implemented
	public boolean rowUpdated() throws SQLException {
           throw TrafT4Messages.createSQLException(connection_.props_, 
                                                 connection_.getLocale(),
                                                 "rowUpdated - not supported", 
                                                  null);
	}

	public void setFetchDirection(int direction) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, direction);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "setFetchDirection", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, direction);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("setFetchDirection");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
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
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "setFetchSize", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, rows);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("setFetchSize");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		if (rows < 0) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_fetch_size",
					null);
		} else if (rows == 0) {
			fetchSize_ = DEFAULT_FETCH_SIZE;
		} else {
			fetchSize_ = rows;
		}
	}

	public void updateArray(int columnIndex, Array x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateArray", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateArray");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "updateArray()");
	}

	public void updateArray(String columnName, Array x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateArray", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateArray");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateArray(columnIndex, x);
	}

	public void updateAsciiStream(int columnIndex, InputStream x, int length) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x, length);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateAsciiStream", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x, length);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateAsciiStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		byte value[];

		validateUpdInvocation(columnIndex);
		value = new byte[length];
		try {
			int k = 0;
			int bytesRead = 0;
			do {
				bytesRead = x.read(value, k, length - k);
				k += bytesRead > 0 ? bytesRead : 0;
			} while (bytesRead != -1 && k < length);
		} catch (java.io.IOException e) {
			Object[] messageArguments = new Object[1];
			messageArguments[0] = e.getMessage();
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "io_exception",
					messageArguments);
		}
		try {
			getCurrentRow().updateArrayElement(columnIndex, new String(value, "ASCII"));
		} catch (java.io.UnsupportedEncodingException e) {
			Object[] messageArguments = new Object[1];
			messageArguments[0] = e.getMessage();
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "unsupported_encoding",
					messageArguments);
		}
	}

	public void updateAsciiStream(String columnName, InputStream x, int length) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x, length);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateAsciiStream", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x, length);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateAsciiStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateAsciiStream(columnIndex, x, length);
	}

	public void updateBigDecimal(int columnIndex, BigDecimal x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateBigDecimal", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateBigDecimal");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		getCurrentRow().updateArrayElement(columnIndex, x);
	}

	public void updateBigDecimal(String columnName, BigDecimal x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateBigDecimal", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateBigDecimal");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateBigDecimal(columnIndex, x);
	}

	public void updateBinaryStream(int columnIndex, InputStream x, int length) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x, length);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateBinaryStream", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x, length);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateBinaryStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		byte value[];

		validateUpdInvocation(columnIndex);
		value = new byte[length];
		try {
			int k = 0;
			int bytesRead = 0;
			do {
				bytesRead = x.read(value, k, length - k);
				k += bytesRead > 0 ? bytesRead : 0;
			} while (bytesRead != -1 && k < length);
		} catch (java.io.IOException e) {
			Object[] messageArguments = new Object[1];
			messageArguments[0] = e.getMessage();
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "io_exception",
					messageArguments);
		}
		getCurrentRow().updateArrayElement(columnIndex, value);
	}

	public void updateBinaryStream(String columnName, InputStream x, int length) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x, length);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateBinaryStream", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x, length);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateBinaryStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateBinaryStream(columnIndex, x, length);
	}


	public void updateBoolean(int columnIndex, boolean x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateBoolean", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateBoolean");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		getCurrentRow().updateArrayElement(columnIndex, new Boolean(x));
	}

	public void updateBoolean(String columnName, boolean x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateBoolean", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateBoolean");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateBoolean(columnIndex, x);
	}

	public void updateByte(int columnIndex, byte x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateByte", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateByte");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		getCurrentRow().updateArrayElement(columnIndex, new Byte(x));
	}

	public void updateByte(String columnName, byte x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateByte", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateByte");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateByte(columnIndex, x);
	}

	public void updateBytes(int columnIndex, byte[] x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateBytes", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateBytes");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		getCurrentRow().updateArrayElement(columnIndex, x);
	}

	public void updateBytes(String columnName, byte[] x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateBytes", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateBytes");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateBytes(columnIndex, x);
	}

	public void updateCharacterStream(int columnIndex, Reader reader, int length) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, reader, length);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateCharacterStream", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, reader, length);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateCharacterStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		char value[] = new char[length];
		try {
			int valuePos = reader.read(value);
			if (valuePos < 1) {
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "No data to read from the Reader";
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "io_exception",
						messageArguments);
			}

			int tempReadLen = 0;
			while (valuePos < length && tempReadLen != -1) {
				char temp[] = new char[length - valuePos];
				tempReadLen = reader.read(temp, 0, length - valuePos);
				System.arraycopy(temp, 0, value, valuePos, tempReadLen);
				valuePos += tempReadLen > 0 ? tempReadLen : 0;
			}

		} catch (java.io.IOException e) {
			Object[] messageArguments = new Object[1];
			messageArguments[0] = e.getMessage();
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "io_exception",
					messageArguments);
		}
		getCurrentRow().updateArrayElement(columnIndex, new String(value));
	}

	public void updateCharacterStream(String columnName, Reader x, int length) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x, length);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateCharacterStream", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x, length);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateCharacterStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateCharacterStream(columnIndex, x, length);
	}

	public void updateDate(int columnIndex, Date x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateDate", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateDate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		getCurrentRow().updateArrayElement(columnIndex, x);
	}

	public void updateDate(String columnName, Date x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateDate", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateDate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateDate(columnIndex, x);
	}

	public void updateDouble(int columnIndex, double x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateDouble", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateDouble");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		getCurrentRow().updateArrayElement(columnIndex, new Double(x));
	}

	public void updateDouble(String columnName, double x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateDouble", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateDouble");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateDouble(columnIndex, x);
	}

	public void updateFloat(int columnIndex, float x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateFloat", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateFloat");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		getCurrentRow().updateArrayElement(columnIndex, new Float(x));
	}

	public void updateFloat(String columnName, float x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateFloat", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateFloat");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateDouble(columnIndex, x);
	}

	public void updateInt(int columnIndex, int x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateInt", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateInt");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		getCurrentRow().updateArrayElement(columnIndex, new Integer(x));
	}

	public void updateInt(String columnName, int x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateInt", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateInt");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateDouble(columnIndex, x);
	}

	public void updateLong(int columnIndex, long x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateLong", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateLong");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		getCurrentRow().updateArrayElement(columnIndex, new Long(x));
	}

	public void updateLong(String columnName, long x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateLong", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateLong");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateLong(columnIndex, x);
	}

	public void updateNull(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateNull", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateNull");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		getCurrentRow().updateArrayElement(columnIndex, null);
	}

	public void updateNull(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateNull", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateNull");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateNull(columnIndex);
	}

	public void updateObject(int columnIndex, Object x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateObject", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		updateObject(columnIndex, x, 0);
	}

	public void updateObject(int columnIndex, Object x, int scale) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x, scale);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateObject", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x, scale);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		getCurrentRow().updateArrayElement(columnIndex, x);
	}

	public void updateObject(String columnName, Object x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateObject", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateObject(columnIndex, x);
	}

	public void updateObject(String columnName, Object x, int scale) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x, scale);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateObject", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x, scale);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateObject(columnIndex, x, scale);
	}

	public void updateRef(int columnIndex, Ref x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateRef", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateRef");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "updateRef()");
	}

	public void updateRef(String columnName, Ref x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateRef", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateRef");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateRef(columnIndex, x);
	}

        // Method not implemented
	public void updateRow() throws SQLException {
           throw TrafT4Messages.createSQLException(connection_.props_, 
                                                 connection_.getLocale(),
                                                 "updateRow - not supported", null);
	}

	public void updateShort(int columnIndex, short x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateShort", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateShort");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		getCurrentRow().updateArrayElement(columnIndex, new Short(x));
	}

	public void updateShort(String columnName, short x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateShort", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateShort");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateShort(columnIndex, x);
	}

	public void updateString(int columnIndex, String x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateString", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateString");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		getCurrentRow().updateArrayElement(columnIndex, x);
	}

	public void updateString(String columnName, String x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateString", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateString");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateString(columnIndex, x);
	}

	public void updateTime(int columnIndex, Time x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateTime", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateTime");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		getCurrentRow().updateArrayElement(columnIndex, x);
	}

	public void updateTime(String columnName, Time x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateTime", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateTime");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateTime(columnIndex, x);
	}

	public void updateTimestamp(int columnIndex, Timestamp x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateTimestamp", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateTimestamp");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateUpdInvocation(columnIndex);
		getCurrentRow().updateArrayElement(columnIndex, x);
	}

	public void updateTimestamp(String columnName, Timestamp x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "updateTimestamp", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("updateTimestamp");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int columnIndex = validateUpdInvocation(columnName);
		updateTimestamp(columnIndex, x);
	}

	public boolean wasNull() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "wasNull", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("wasNull");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		return wasNull_;
	}

	void setColumnName(int columnIndex, String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "setColumnName", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("setColumnName");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (columnIndex < 1 || columnIndex > outputDesc_.length) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_column_index",
					null);
		}
		outputDesc_[columnIndex - 1].name_ = columnName;
	}

	// Other methods
	void close(boolean dropStmt) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, dropStmt);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "close", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, dropStmt);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("close");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (!isClosed_) {
			clearWarnings();
			try {
				irs_.close();
			} finally {
				isClosed_ = true;
				irs_ = null;

				if (stmt_ != null) {
					if (dropStmt) {
						for (int i = 0; i < stmt_.num_result_sets_; i++)
							stmt_.resultSet_[i] = null;
						stmt_.result_set_offset = 0;
					}
				}
			}
		}
	}

	private int validateGetInvocation(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "validateGetInvocation", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("validateGetInvocation");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int i;
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		for (i = 0; i < outputDesc_.length; i++) {
			if (columnName.equalsIgnoreCase(outputDesc_[i].name_)) {
				return i + 1;
			}
		}
		throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_column_name", null);
	}

	private void validateGetInvocation(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "validateGetInvocation", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("validateGetInvocation");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_cursor_state",
					null);
		}
		if (columnIndex < 1 || columnIndex > outputDesc_.length) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_column_index",
					null);
		}
	}

	public Connection getConnection() {
		return this.connection_;
	}

	private int validateUpdInvocation(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "validateUpdInvocation", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("validateUpdInvocation");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int i;
		if (getConcurrency() == ResultSet.CONCUR_READ_ONLY) {
			throw TrafT4Messages
					.createSQLException(connection_.props_, connection_.getLocale(), "read_only_concur", null);
		}
		for (i = 0; i < outputDesc_.length; i++) {
			if (columnName.equalsIgnoreCase(outputDesc_[i].name_)) {
				return i + 1;
			}
		}
		throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_column_name", null);
	}

	private void validateUpdInvocation(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "validateUpdInvocation", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("validateUpdInvocation");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (getConcurrency() == ResultSet.CONCUR_READ_ONLY) {
			throw TrafT4Messages
					.createSQLException(connection_.props_, connection_.getLocale(), "read_only_concur", null);
		}
		if (columnIndex < 1 || columnIndex > outputDesc_.length) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_column_index",
					null);
		}
	}

	private ObjectArray getCurrentRow() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getCurrentRow", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getCurrentRow");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (onInsertRow_) {
			return insertRow_;
		} else {
			if (isBeforeFirst_) {
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "cursor_is_before_first_row", null);
			}
			if (isAfterLast_) {
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "cursor_after_last_row", null);
			}
			return (ObjectArray) cachedRows_.get(currentRow_ - 1);
		}
	}

	private int getRowCount() {
		return numRows_;
	}

	void getKeyColumns() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getKeyColumns", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getKeyColumns");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String colName;
		int index = 0;
		int rowCount = 0;
		int colNo;
		int columnCount;
		StringBuffer insertColsStr;
		StringBuffer insertValueStr;
		StringBuffer updateStr;
		StringBuffer whereClause;
		StringBuffer deleteWhereClause;
		StringBuffer selectWhereClause;
		StringBuffer selectClause;
		String keyCatalogNm;
		String keySchemaNm;
		String keyTableNm;
		ArrayList keyColumns = null;
		String columnName;

		if (noKeyFound_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "no_primary_key", null);
		}

		try {
			DatabaseMetaData dbmd = connection_.getMetaData();
			ResultSetMetaData rsmd = getMetaData();

			// The table of the first column in the select list is being treated
			// as the table to be updated. Columns from the other tables are
			// ignored
			columnCount = rsmd.getColumnCount();
			keyCatalogNm = rsmd.getCatalogName(1);
			keySchemaNm = rsmd.getSchemaName(1);
			keyTableNm = rsmd.getTableName(1);

			if (this.stmt_ instanceof TrafT4PreparedStatement) {
				keyColumns = ((TrafT4PreparedStatement) this.stmt_).getKeyColumns();
			}

			if (keyColumns == null) {
				ResultSet rs = dbmd.getPrimaryKeys(keyCatalogNm, keySchemaNm, keyTableNm);

				keyColumns = new ArrayList();
				while (rs.next()) {
					rowCount++;
					colName = rs.getString(4);
					keyColumns.add(index++, colName);
				}

				rowCount = ((TrafT4ResultSet) rs).getRowCount();
				if (rowCount == 0) {
					noKeyFound_ = true;
					stmt_.resultSetConcurrency_ = ResultSet.CONCUR_READ_ONLY;
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"no_primary_key", null);
				}

				if (this.stmt_ instanceof TrafT4PreparedStatement) {
					((TrafT4PreparedStatement) this.stmt_).setKeyColumns(keyColumns);
				}
			}

			// Commented out since rs.close() will end the transaction and hence
			// the application
			// may not be able to fetch its next row. It is ok not to close,
			// since it is an internal
			// stmt
			// rs.close();
			// Check if Key columns are there in the result Set
			keyCols_ = new BitSet(columnCount);
			for (index = 0; index < keyColumns.size(); index++) {
				for (colNo = 1; colNo <= columnCount; colNo++) {
					if (rsmd.getColumnName(colNo).equals((String) keyColumns.get(index))
							&& rsmd.getTableName(colNo).equals(keyTableNm)
							&& rsmd.getSchemaName(colNo).equals(keySchemaNm)
							&& rsmd.getCatalogName(colNo).equals(keyCatalogNm)) {
						keyCols_.set(colNo - 1);
						break;
					}
				}
				if (colNo > columnCount) {
					noKeyFound_ = true;
					stmt_.resultSetConcurrency_ = ResultSet.CONCUR_READ_ONLY;
					throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
							"no_primary_key", null);
				}
			}
			// Create a Update, Insert, Delete and Select statements
			// Select statement where clause has only primary keys
			// Update and Delete statments where clause has all columns of the
			// table being modified
			// Update statement set clause doesn't contain the primary keys,
			// since it can't be updated
			// Insert statement has all the columns of the table in the value
			// list
			paramCols_ = new BitSet(columnCount);
			whereClause = new StringBuffer(2048).append(" where ");
			deleteWhereClause = new StringBuffer(2048).append(" where ");
			insertColsStr = new StringBuffer(2048).append("(");
			insertValueStr = new StringBuffer(2048).append(" values (");
			updateStr = new StringBuffer(2048).append(" set ");
			selectWhereClause = new StringBuffer(2048).append(" where ");
			selectClause = new StringBuffer(2048).append("select ");
			for (colNo = 1; colNo < columnCount; colNo++) {
				if (rsmd.getTableName(colNo).equals(keyTableNm) && rsmd.getSchemaName(colNo).equals(keySchemaNm)
						&& rsmd.getCatalogName(colNo).equals(keyCatalogNm)) {
					paramCols_.set(colNo - 1);
					columnName = rsmd.getColumnName(colNo);
					insertColsStr = insertColsStr.append(columnName).append(", ");
					insertValueStr = insertValueStr.append("?, ");
					if (!keyCols_.get(colNo - 1)) // do not include key
					// columns
					{
						updateStr = updateStr.append(columnName).append(" = ?, ");
					} else {
						selectWhereClause = selectWhereClause.append(columnName).append(" = ? and ");
						whereClause = whereClause.append(columnName).append(" = ? and ");
					}
					deleteWhereClause = deleteWhereClause.append(columnName).append(" = ? and ");
					selectClause = selectClause.append(columnName).append(", ");
				}
			}
			paramCols_.set(colNo - 1);
			columnName = rsmd.getColumnName(colNo);
			insertColsStr = insertColsStr.append(columnName).append(")");
			insertValueStr = insertValueStr.append("?)");
			if (!keyCols_.get(colNo - 1)) { // do not include key columns
				updateStr = updateStr.append(columnName).append(" = ? ");

				// We do not want non-key columns in the where clause, but we
				// added an extra
				// " and " above
				int selectWhereClause_len = selectWhereClause.length();
				selectWhereClause.delete(selectWhereClause_len - 5, selectWhereClause_len);

				int whereClause_len = whereClause.length();
				whereClause.delete(whereClause_len - 5, whereClause_len);
			} else {
				updateStr.setCharAt(updateStr.length() - 2, ' ');
				selectWhereClause = selectWhereClause.append(columnName).append(" = ? ");
				whereClause = whereClause.append(columnName).append(" = ? ");
			}
			deleteWhereClause = deleteWhereClause.append(columnName).append(" = ? ");
			selectClause = selectClause.append(columnName).append(" from ");
			selectCmd_ = new StringBuffer(2048).append(selectClause).append(keyCatalogNm).append(".").append(
					keySchemaNm).append(".").append(keyTableNm).append(selectWhereClause);
			deleteCmd_ = new StringBuffer(2048).append("delete from ").append(keyCatalogNm).append(".").append(
					keySchemaNm).append(".").append(keyTableNm).append(deleteWhereClause);
			insertCmd_ = new StringBuffer(2048).append("insert into ").append(keyCatalogNm).append(".").append(
					keySchemaNm).append(".").append(keyTableNm).append(insertColsStr).append(insertValueStr);
			updateCmd_ = new StringBuffer(2048).append("update ").append(keyCatalogNm).append(".").append(keySchemaNm)
					.append(".").append(keyTableNm).append(updateStr).append(whereClause);
		} catch (SQLException e) {
			performConnectionErrorChecks(e);
			throw e;
		}
	}

	void prepareDeleteStmt() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "prepareDeleteStmt", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("prepareDeleteStmt");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (deleteStmt_ == null) {
			if (deleteCmd_ == null) {
				getKeyColumns();
			}
			deleteStmt_ = connection_.prepareStatement(deleteCmd_.toString());
		}
	}

	void prepareInsertStmt() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "prepareInsertStmt", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("prepareInsertStmt");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (insertStmt_ == null) {
			if (insertCmd_ == null) {
				getKeyColumns();
			}
			insertStmt_ = connection_.prepareStatement(insertCmd_.toString());
		}
	}

	void prepareUpdateStmt() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "prepareUpdateStmt", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("prepareUpdateStmt");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (updateStmt_ == null) {
			if (updateCmd_ == null) {
				getKeyColumns();
			}
			updateStmt_ = connection_.prepareStatement(updateCmd_.toString());
		}
	}

	void prepareSelectStmt() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "prepareSelectStmt", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("prepareSelectStmt");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (selectStmt_ == null) {
			if (selectCmd_ == null) {
				getKeyColumns();
			}
			selectStmt_ = connection_.prepareStatement(selectCmd_.toString());
		}
	}


	// This method is called from the JNI method Fetch for user given SQL
	// statements. For
	// DatabaseMetaData catalog APIs also call this method to update the rows
	// fetched
	//
	void setFetchOutputs(ObjectArray[] row, int rowsFetched, boolean endOfData) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, row, rowsFetched, endOfData, 0);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "setFetchOutputs", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, row, rowsFetched, endOfData, 0);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("setFetchOutputs");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		// numRows_ contain the number of rows in the ArrayList. In case of
		// TYPE_FORWARD_ONLY,
		// the numRows_ is reset to 0 whenever this method is called.
		// totalRowsFetched_ contain the total number of rows fetched

		if (getType() == ResultSet.TYPE_FORWARD_ONLY) {
			cachedRows_.clear();
			numRows_ = 0;
			currentRow_ = 0;
		}
		for (int i = 0; i < row.length; i++) {
			cachedRows_.add(row[i]);
			// add to the totalRowsFetched
		}
		totalRowsFetched_ += rowsFetched;
		numRows_ += rowsFetched;
		endOfData_ = endOfData;
		isBeforeFirst_ = false;
	}

	// Method used by JNI layer to set the Data Truncation warning
	void setDataTruncation(int index, boolean parameter, boolean read, int dataSize, int transferSize) {

		DataTruncation dtLeaf = new DataTruncation(index, parameter, read, dataSize, transferSize);
		if (sqlWarning_ == null) {
			sqlWarning_ = dtLeaf;
		} else {
			sqlWarning_.setNextWarning(dtLeaf);
		}
	}

	int getFSDataType(int ColumnCount) throws SQLException { // 0 -based
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, ColumnCount);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getFSDataType", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, ColumnCount);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getFSDataType");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (ColumnCount >= outputDesc_.length) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_column_index",
					null);
		} else {
			return outputDesc_[ColumnCount].fsDataType_;
		}
	}

	int getSQLDataType(int ColumnCount) throws SQLException { // 0 -based
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, ColumnCount);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getSQLDataType", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, ColumnCount);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getSQLDataType");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (ColumnCount >= outputDesc_.length) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_column_index",
					null);
		} else {
			return outputDesc_[ColumnCount].sqlDataType_;
		}
	}

	int getPrecision(int ColumnCount) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, ColumnCount);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getPrecision", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, ColumnCount);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getPrecision");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (ColumnCount >= outputDesc_.length) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_column_index",
					null);
		} else {
			return outputDesc_[ColumnCount].precision_;
		}
	}

	int getScale(int ColumnCount) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, ColumnCount);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getScale", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, ColumnCount);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getScale");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (ColumnCount >= outputDesc_.length) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_column_index",
					null);
		} else {
			return outputDesc_[ColumnCount].scale_;
		}
	}

	int getSqlDatetimeCode(int ColumnCount) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, ColumnCount);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getSqlDatetimeCode", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, ColumnCount);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getSqlDatetimeCode");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (ColumnCount >= outputDesc_.length) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_column_index",
					null);
		} else {
			return outputDesc_[ColumnCount].sqlDatetimeCode_;
		}
	}

	int getNoOfColumns() {
		return outputDesc_.length;
	}

	int getSqlOctetLength(int ColumnCount) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, ColumnCount);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getSqlOctetLength", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, ColumnCount);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getSqlOctetLength");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (ColumnCount >= outputDesc_.length) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_column_index",
					null);
		} else {
			return outputDesc_[ColumnCount].sqlOctetLength_;
		}
	}

	boolean getSigned(int ColumnCount) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, ColumnCount);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getSigned", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, ColumnCount);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getSigned");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (ColumnCount >= outputDesc_.length) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_column_index",
					null);
		} else {
			return outputDesc_[ColumnCount].isSigned_;
		}
	}

	private String getLocalString(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getLocalString", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getLocalString");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		Object obj = null;
		String data = null;
		int sqlCharset = outputDesc_[columnIndex - 1].sqlCharset_;

		wasNull_ = true;
		ObjectArray row = getCurrentRow();
		if (row != null) {
			obj = row.getUpdatedArrayElement(columnIndex);
			if (obj != null) {
				if (obj instanceof byte[]) {
					try {
						data = this.irs_.ic_.decodeBytes((byte[]) obj, this.irs_.ic_.getTerminalCharset());

						wasNull_ = false;
					} catch (CharacterCodingException e) {
						SQLException se = TrafT4Messages.createSQLException(this.connection_.ic_.t4props_,
								this.connection_.getLocale(), "translation_of_parameter_failed", "getLocalString", e
										.getMessage());
						se.initCause(e);
						throw se;
					} catch (UnsupportedCharsetException e) {
						SQLException se = TrafT4Messages.createSQLException(this.connection_.ic_.t4props_,
								this.connection_.getLocale(), "unsupported_encoding", e.getCharsetName());
						se.initCause(e);
						throw se;
					}
				} else {
					data = obj.toString();
					wasNull_ = false;
				}
			}
		}

		return data;
	}

	public String getProxySyntax() {
		return proxySyntax_;
	}

	public boolean isClosed() throws SQLException {
		return (isClosed_ || connection_.isClosed());
	}

	public boolean hasLOBColumns() {
		boolean ret = false;
		for (int i = 0; i < this.outputDesc_.length; i++) {
			if (outputDesc_[i].dataType_ == Types.CLOB || outputDesc_[i].dataType_ == Types.BLOB) {
				ret = true;
				break;
			}
		}
		return ret;
	}

	public long getSequenceNumber() {
		return seqNum_;
	}

	public boolean useOldDateFormat() {
		return useOldDateFormat_;
	}

	void closeErroredConnection(TrafT4Exception sme) {
		connection_.closeErroredConnection(sme);
	}

	// Constructors - used in TrafT4Statement
	TrafT4ResultSet(TrafT4Statement stmt, TrafT4Desc[] outputDesc) throws SQLException {
		this(stmt, outputDesc, stmt.stmtLabel_, false);
	}

	// Constructors - used for SPJ ResultSets
	TrafT4ResultSet(TrafT4Statement stmt, TrafT4Desc[] outputDesc, String stmt_label, boolean spj_result_set)
			throws SQLException {
		if (stmt.connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(stmt.connection_.props_, stmt, outputDesc, stmt_label,
					spj_result_set);
			stmt.connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "", "", p);
		}
		if (stmt.connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(stmt.connection_.props_, stmt, outputDesc, stmt_label,
					spj_result_set);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			stmt.connection_.props_.getLogWriter().println(temp);
		}
		stmt_ = stmt;
		outputDesc_ = outputDesc;
		// update all the resultSet fields from stmt
		connection_ = stmt.connection_;
		keepRawBuffer_ = connection_.props_.getKeepRawFetchBuffer();
		fetchSize_ = stmt.fetchSize_;
		fetchDirection_ = stmt.fetchDirection_;
		// Don't use the statement label from the statement object
		// Instead, use it from the one provided (added for SPJ RS support) - SB
		// 11/21/2005
		stmtLabel_ = stmt_label;
		cachedRows_ = new ArrayList();
		isBeforeFirst_ = true;
		holdability_ = stmt.resultSetHoldability_;
		spj_rs_ = spj_result_set;
		fetchComplete_ = false;
		
		seqNum_ = seqCount_++;
		try {
			irs_ = new InterfaceResultSet(this);
		} catch (SQLException e) {
			performConnectionErrorChecks(e);
			throw e;
		}
	}

	// Constructor - used in T4DatabaseMetaData
	// ResultSet is not created on Java side in
	// T4DatanaseMetaData due to threading issues
	TrafT4ResultSet(T4DatabaseMetaData dbMetaData, TrafT4Desc[] outputDesc, String stmtLabel, boolean oldDateFormat)
			throws SQLException {
		if (dbMetaData.connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(dbMetaData.connection_.props_, dbMetaData, outputDesc, 0,
					stmtLabel);
			dbMetaData.connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "", "", p);
		}
		if (dbMetaData.connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(dbMetaData.connection_.props_, dbMetaData, outputDesc, 0,
					stmtLabel);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			dbMetaData.connection_.props_.getLogWriter().println(temp);
		}
		connection_ = dbMetaData.connection_;
		outputDesc_ = outputDesc;
		keepRawBuffer_ = connection_.props_.getKeepRawFetchBuffer();
		stmtLabel_ = stmtLabel;
		fetchSize_ = DEFAULT_FETCH_SIZE;
		fetchDirection_ = ResultSet.FETCH_FORWARD;
		cachedRows_ = new ArrayList();
		isBeforeFirst_ = true;
		holdability_ = CLOSE_CURSORS_AT_COMMIT;
		spj_rs_ = false;
		useOldDateFormat_ = oldDateFormat;
		fetchComplete_ = false;
		
		seqNum_ = seqCount_++;
		try {
			irs_ = new InterfaceResultSet(this);
		} catch (SQLException e) {
			performConnectionErrorChecks(e);
			throw e;
		}
	}
	
	// Fields
	InterfaceResultSet irs_;
	TrafT4Desc[] outputDesc_;
	TrafT4Statement stmt_;
	TrafT4Connection connection_;
	boolean isClosed_;
	int currentRow_;
	boolean endOfData_;
	// Fetch outputs updated by JNI Layer
	boolean wasNull_;
	int totalRowsFetched_;
	int fetchSize_;
	int fetchDirection_;
	String stmtLabel_;

	ArrayList cachedRows_;
	boolean onInsertRow_;
	ObjectArray insertRow_;
	int savedCurrentRow_;
	boolean showInserted_;
	int numRows_;
	boolean isAfterLast_;
	boolean isBeforeFirst_;
	private static float _javaVersion;
	private static Method _toPlainString;

	boolean noKeyFound_;
	StringBuffer deleteCmd_;
	StringBuffer insertCmd_;
	StringBuffer updateCmd_;
	StringBuffer selectCmd_;
	PreparedStatement deleteStmt_;
	PreparedStatement insertStmt_;
	PreparedStatement updateStmt_;
	PreparedStatement selectStmt_;
	BitSet paramCols_;
	BitSet keyCols_;
	int holdability_;

	int currentRowCount_ = 0;
	static final int DEFAULT_FETCH_SIZE = 100;
	static final String hyphen_string = new String("-");

	static long seqCount_ = 0;
	long seqNum_ = 0;
	String proxySyntax_ = "";
	boolean useOldDateFormat_ = false;

	// For LOB Support - SB 10/8/2004
	boolean isAnyLob_;

	// For SPJ RS support - SB 11/21/2005
	boolean spj_rs_;

	boolean keepRawBuffer_;
	byte[] rawBuffer_;
	boolean fetchComplete_;
	HashMap<String, Integer> colMap_;

	static {
		_javaVersion = Float.parseFloat(System.getProperty("java.specification.version"));
		if (_javaVersion >= 1.5) {
			try {
				_toPlainString = java.math.BigDecimal.class.getMethod("toPlainString", (Class[]) null);
			} catch(Exception e) {}
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

	public RowId getRowId(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public RowId getRowId(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public void updateRowId(int columnIndex, RowId x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateRowId(String columnLabel, RowId x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public int getHoldability() throws SQLException {
		// TODO Auto-generated method stub
		return 0;
	}

	public void updateNString(int columnIndex, String nString)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNString(String columnLabel, String nString)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNClob(int columnIndex, NClob nClob) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNClob(String columnLabel, NClob nClob)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	/**/
	public Clob getClob(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getClob", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getClob");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		String lobHandle;

		validateGetInvocation(columnIndex);
		outputDesc_[columnIndex - 1].checkValidNumericConversion(connection_.getLocale());

		lobHandle = getLocalString(columnIndex);
        int dataType = outputDesc_[columnIndex - 1].dataType_;
        switch (dataType) {
        case Types.CLOB:
        case Types.BLOB:
            lobHandle = getLocalString(columnIndex);
            if (lobHandle != null) {
                return new TrafT4Clob(connection_, lobHandle, null);
            }
            break;

        case Types.VARCHAR:
        case Types.CHAR:
        case Types.LONGVARCHAR:
            String data = getString(columnIndex);
            if (data != null) {
                return new TrafT4Clob(connection_, null, data);
            }

        default:
            return null;
		}
		return null;
	}
	
	public Clob getClob(String columnName)  throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getClob", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getClob");
			T4LogFormatter lf = new T4LogFormatter();
			String tmp = lf.format(lr);
			connection_.props_.getLogWriter().println(tmp);
		}
		int columnIndex = validateGetInvocation(columnName);
		return getClob(columnIndex);
	}
	
	public Blob getBlob(int columnIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getClob", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getClob");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		String lobHandle;

		validateGetInvocation(columnIndex);
		outputDesc_[columnIndex - 1].checkValidNumericConversion(connection_.getLocale());

		lobHandle = getLocalString(columnIndex);

		if (lobHandle != null) {
            return new TrafT4Blob(connection_, lobHandle, null);
		}
		return null;
	}

	public Blob getBlob(String columnName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getBlob", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, columnName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getBlob");
			T4LogFormatter lf = new T4LogFormatter();
			String tmp = lf.format(lr);
			connection_.props_.getLogWriter().println(tmp);
		}
		int columnIndex = validateGetInvocation(columnName);

		return getBlob(columnIndex);
	}
    /**/
	public NClob getNClob(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public NClob getNClob(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public SQLXML getSQLXML(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public SQLXML getSQLXML(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public void updateSQLXML(int columnIndex, SQLXML xmlObject)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateSQLXML(String columnLabel, SQLXML xmlObject)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public String getNString(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public String getNString(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Reader getNCharacterStream(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Reader getNCharacterStream(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public void updateNCharacterStream(int columnIndex, Reader x, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNCharacterStream(String columnLabel, Reader reader,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateAsciiStream(int columnIndex, InputStream x, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBinaryStream(int columnIndex, InputStream x, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateCharacterStream(int columnIndex, Reader x, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateAsciiStream(String columnLabel, InputStream x, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBinaryStream(String columnLabel, InputStream x,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateCharacterStream(String columnLabel, Reader reader,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}
	/**/
	public void updateBlob(int columnIndex, Blob x) throws SQLException {
		// TODO Auto-generated method stub
	}
	
	public void updateBlob(String columnName, Blob x) throws SQLException {
		// TODO Auto-generated method stub
	}
	
	public void updateClob(String columnName, Clob x) throws SQLException {
		// TODO Auto-generated method stub
	}
	
	public void updateClob(int columnIndex, Clob x) throws SQLException {
		// TODO Auto-generated method stub
	}
    /**/
	public void updateBlob(int columnIndex, InputStream inputStream, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBlob(String columnLabel, InputStream inputStream,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateClob(int columnIndex, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateClob(String columnLabel, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNClob(int columnIndex, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNClob(String columnLabel, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNCharacterStream(int columnIndex, Reader x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNCharacterStream(String columnLabel, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateAsciiStream(int columnIndex, InputStream x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBinaryStream(int columnIndex, InputStream x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateCharacterStream(int columnIndex, Reader x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateAsciiStream(String columnLabel, InputStream x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBinaryStream(String columnLabel, InputStream x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateCharacterStream(String columnLabel, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBlob(int columnIndex, InputStream inputStream)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBlob(String columnLabel, InputStream inputStream)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateClob(int columnIndex, Reader reader) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateClob(String columnLabel, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNClob(int columnIndex, Reader reader) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNClob(String columnLabel, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public Object getObject(int columnIndex, Class type) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Object getObject(String columnLabel, Class type)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
}
