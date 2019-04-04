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
import java.math.BigDecimal;
import java.net.URL;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.UnsupportedCharsetException;
import java.sql.Array;
import java.sql.Blob;
import java.sql.Clob;
import java.sql.DatabaseMetaData;
import java.sql.Date;
import java.sql.NClob;
import java.sql.Ref;
import java.sql.ResultSet;
import java.sql.RowId;
import java.sql.SQLException;
import java.sql.SQLXML;
import java.sql.Time;
import java.sql.Timestamp;
import java.sql.Types;
import java.util.Calendar;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.LogRecord;

public class TrafT4CallableStatement extends TrafT4PreparedStatement implements java.sql.CallableStatement {
	public Array getArray(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getArray", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getArray");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		clearWarnings();
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "getArray()");
		return null;
	}

	public Array getArray(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getArray", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getArray");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getArray(parameterIndex);
	}

	public BigDecimal getBigDecimal(int parameterIndex) throws SQLException {
		BigDecimal retValue;
		String data;

		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getBigDecimal", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getBigDecimal");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		validateGetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		// String returned may not be numeric in case of SQL_CHAR, SQL_VARCHAR
		// and SQL_LONGVARCHAR
		// fields. Hoping that java might throw invalid value exception
		// For LOB Support - SB
		// data = inputDesc_[parameterIndex-1].paramValue_;
		data = getString(parameterIndex);
		if (data == null) {
			wasNull_ = true;
			return null;
		} else {
			wasNull_ = false;
			try {
				retValue = new BigDecimal(data);
			} catch (NumberFormatException e) {
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
						"invalid_cast_specification", null);
			}
			return retValue;
		}
	}

	public BigDecimal getBigDecimal(int parameterIndex, int scale) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, scale);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getBigDecimal", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, scale);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getBigDecimal");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		BigDecimal retValue;

		retValue = getBigDecimal(parameterIndex);
		if (retValue != null) {
			return retValue.setScale(scale);
		} else {
			return null;
		}
	}

	public BigDecimal getBigDecimal(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getBigDecimal", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getBigDecimal");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getBigDecimal(parameterIndex);
	}

	public BigDecimal getBigDecimal(String parameterName, int scale) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, scale);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getBigDecimal", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, scale);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getBigDecimal");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getBigDecimal(parameterIndex, scale);
	}


	public boolean getBoolean(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getBoolean", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getBoolean");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;

		validateGetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		// For LOB Support - SB
		// data = inputDesc_[parameterIndex-1].paramValue_;
		data = getString(parameterIndex);

		if (data != null) {
			wasNull_ = false;
			return (!data.equals("0"));
		} else {
			wasNull_ = true;
			return false;
		}
	}

	public boolean getBoolean(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getBoolean", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getBoolean");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getBoolean(parameterIndex);
	}

	public byte getByte(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getByte", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getByte");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;

		validateGetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		// For LOB Support - SB
		// data = inputDesc_[parameterIndex-1].paramValue_;
		data = getString(parameterIndex);

		if (data != null) {
			wasNull_ = false;
			return Byte.parseByte(data);
		} else {
			wasNull_ = true;
			return 0;
		}
	}

	public byte getByte(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getByte", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getByte");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getByte(parameterIndex);
	}

	public byte[] getBytes(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getBytes", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getBytes");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;

		validateGetInvocation(parameterIndex);
		dataType = inputDesc_[parameterIndex - 1].dataType_;
		if (dataType != Types.BINARY && dataType != Types.VARBINARY && dataType != Types.LONGVARBINARY) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}
		return getBytes(parameterIndex);
	}

	public byte[] getBytes(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getBytes", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getBytes");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getBytes(parameterIndex);
	}

	public Date getDate(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getDate", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getDate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;
		String dateStr;
		Date retValue;

		validateGetInvocation(parameterIndex);
		dataType = inputDesc_[parameterIndex - 1].dataType_;
		if (dataType != Types.CHAR && dataType != Types.VARCHAR && dataType != Types.LONGVARCHAR
				&& dataType != Types.DATE && dataType != Types.TIMESTAMP) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}
		// For LOB Support - SB
		// dateStr = inputDesc_[parameterIndex-1].paramValue_;
		dateStr = getString(parameterIndex);

		if (dateStr != null) {
			wasNull_ = false;
			try {
				boolean convertDate = connection_.getDateConversion();

				if (convertDate) {
					String dt = TrafT4ResultSet.convertDateFormat(dateStr);
					retValue = TrafT4ResultSet.valueOf(dt);
				} else {
					retValue = Date.valueOf(dateStr);
				}
			} catch (IllegalArgumentException e) {
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
						"invalid_cast_specification", null);
			}
			return retValue;
		} else {
			wasNull_ = true;
			return null;
		}
	}

	public Date getDate(int parameterIndex, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getDate", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getDate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		Date sqlDate;
		java.util.Date d;

		sqlDate = getDate(parameterIndex);
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

	public Date getDate(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getDate", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getDate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getDate(parameterIndex);
	}

	public Date getDate(String parameterName, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getDate", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getDate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getDate(parameterIndex, cal);
	}

	public double getDouble(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getDouble", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getDouble");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;

		validateGetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		// For LOB Support - SB
		// data = inputDesc_[parameterIndex-1].paramValue_;
		data = getString(parameterIndex);
		if (data != null) {
			wasNull_ = false;
			return Double.parseDouble(data);
		} else {
			wasNull_ = true;
			return 0;
		}
	}

	public double getDouble(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getDouble", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getDouble");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getDouble(parameterIndex);
	}

	public float getFloat(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getFloat", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getFloat");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;

		validateGetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		// For LOB Support - SB
		// data = inputDesc_[parameterIndex-1].paramValue_;
		data = getString(parameterIndex);

		if (data != null) {
			wasNull_ = false;
			return Float.parseFloat(data);
		} else {
			wasNull_ = true;
			return 0;
		}
	}

	public float getFloat(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getFloat", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getFloat");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getFloat(parameterIndex);
	}

	public int getInt(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getInt", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getInt");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;

		validateGetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		// For LOB Support - SB
		// data = inputDesc_[parameterIndex-1].paramValue_;
		data = getString(parameterIndex);
		if (data != null) {
			wasNull_ = false;
			return Integer.parseInt(data);
		} else {
			wasNull_ = true;
			return 0;
		}
	}

	public int getInt(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getInt", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getInt");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getInt(parameterIndex);
	}

	public long getLong(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getLong", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getLong");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;

		validateGetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		// For LOB Support - SB
		// data = inputDesc_[parameterIndex-1].paramValue_;
		data = getString(parameterIndex);
		if (data != null) {
			wasNull_ = false;
			return Long.parseLong(data);
		} else {
			wasNull_ = true;
			return 0;
		}
	}

	public long getLong(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getLong", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getLong");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getLong(parameterIndex);
	}

	public Object getObject(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getObject", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;
		byte byteValue;
		short shortValue;
		int intValue;
		long longValue;
		float floatValue;
		double doubleValue;
		boolean booleanValue;

		validateGetInvocation(parameterIndex);
		dataType = inputDesc_[parameterIndex - 1].dataType_;
		switch (dataType) {
		case Types.TINYINT:
			byteValue = getByte(parameterIndex);
			if (wasNull_) {
				return null;
			} else {
				return new Byte(byteValue);
			}
		case Types.SMALLINT:
			intValue = getShort(parameterIndex);
			if (wasNull_) {
				return null;
			} else {
				return new Integer(intValue);
			}
		case Types.INTEGER:
			intValue = getInt(parameterIndex);
			if (wasNull_) {
				return null;
			} else {
				return new Integer(intValue);
			}
		case Types.BIGINT:
			longValue = getLong(parameterIndex);
			if (wasNull_) {
				return null;
			} else {
				return new Long(longValue);
			}
		case Types.REAL:
			floatValue = getFloat(parameterIndex);
			if (wasNull_) {
				return null;
			} else {
				return new Float(floatValue);
			}
		case Types.FLOAT:
		case Types.DOUBLE:
			doubleValue = getDouble(parameterIndex);
			if (wasNull_) {
				return null;
			} else {
				return new Double(doubleValue);
			}
		case Types.DECIMAL:
		case Types.NUMERIC:
			return getBigDecimal(parameterIndex);
		case Types.BIT:
			booleanValue = getBoolean(parameterIndex);
			if (wasNull_) {
				return null;
			} else {
				return new Boolean(booleanValue);
			}
		case Types.CHAR:
		case Types.VARCHAR:
		case Types.LONGVARCHAR:
			return getString(parameterIndex);
		case Types.BINARY:
		case Types.VARBINARY:
		case Types.LONGVARBINARY:
			return getBytes(parameterIndex);
		case Types.DATE:
			return getDate(parameterIndex);
		case Types.TIME:
			return getTime(parameterIndex);
		case Types.TIMESTAMP:
			return getTimestamp(parameterIndex);
		default:
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}
	}

	public Object getObject(int parameterIndex, Map map) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, map);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getObject", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, map);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "getObject()");
		return null;
	}

	public Object getObject(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getOjbect", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getOjbect");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getObject(parameterIndex);
	}

	public Object getObject(String parameterName, Map map) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, map);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getObject", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, map);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getObject(parameterIndex, map);
	}

	public Ref getRef(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getRef", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getRef");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "getRef()");
		return null;
	}

	public Ref getRef(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getRef", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getRef");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getRef(parameterIndex);
	}

	public short getShort(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getShort", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getShort");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;

		validateGetInvocation(parameterIndex);
		inputDesc_[parameterIndex - 1].checkValidNumericConversion(connection_.getLocale());
		// For LOB Support - SB
		// data = inputDesc_[parameterIndex-1].paramValue_;
		data = getString(parameterIndex);
		if (data != null) {
			wasNull_ = false;
			return Short.parseShort(data);
		} else {
			wasNull_ = true;
			return 0;
		}
	}

	public short getShort(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getShort", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getShort");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getShort(parameterIndex);
	}

	public String getString(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getString", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getString");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		String data;
		// For LOB Support - SB 10/8/2004
		Object x;
		int targetSqlType;
		int sqlCharset;

		validateGetInvocation(parameterIndex);

		targetSqlType = inputDesc_[parameterIndex - 1].dataType_;
		sqlCharset = inputDesc_[parameterIndex - 1].sqlCharset_;
		x = inputDesc_[parameterIndex - 1].paramValue_;

		if (x == null) {
			wasNull_ = true;
			data = null;
		} else {
			if (x instanceof byte[]) {
				try {
					if (this.ist_.ic_.getISOMapping() == InterfaceUtilities.SQLCHARSETCODE_ISO88591
							&& !this.ist_.ic_.getEnforceISO()
							&& sqlCharset == InterfaceUtilities.SQLCHARSETCODE_ISO88591)
						data = new String((byte[]) x, ist_.ic_.t4props_.getISO88591());
					else
						data = this.ist_.ic_.decodeBytes((byte[]) x, sqlCharset);

					wasNull_ = false;
				} catch (CharacterCodingException e) {
					SQLException se = TrafT4Messages.createSQLException(this.connection_.ic_.t4props_, this.connection_
							.getLocale(), "translation_of_parameter_failed", "getLocalString", e.getMessage());
					se.initCause(e);
					throw se;
				} catch (UnsupportedCharsetException e) {
					SQLException se = TrafT4Messages.createSQLException(this.connection_.ic_.t4props_, this.connection_
							.getLocale(), "unsupported_encoding", e.getCharsetName());
					se.initCause(e);
					throw se;
				} catch (UnsupportedEncodingException e) {
					SQLException se = TrafT4Messages.createSQLException(this.connection_.ic_.t4props_, this.connection_
							.getLocale(), "unsupported_encoding", e.getMessage());
					se.initCause(e);
					throw se;
				}
			} else {
				data = x.toString();
				wasNull_ = false;
			}
		}
		return data;
	}

	public String getString(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getString", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getString");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getString(parameterIndex);
	}

	public Time getTime(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getTime", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getTime");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;
		String timeStr;
		Time retValue;

		validateGetInvocation(parameterIndex);
		dataType = inputDesc_[parameterIndex - 1].dataType_;
		if (dataType != Types.CHAR && dataType != Types.VARCHAR && dataType != Types.LONGVARCHAR
				&& dataType != Types.TIME && dataType != Types.TIMESTAMP) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}

		// For LOB Support - SB 10/8/2004
		// timeStr = inputDesc_[parameterIndex-1].paramValue_;
		timeStr = getString(parameterIndex);
		if (timeStr != null) {
			try {
				wasNull_ = false;
				retValue = Time.valueOf(timeStr);
			} catch (IllegalArgumentException e) {
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
						"invalid_cast_specification", null);
			}
			return retValue;
		} else {
			wasNull_ = true;
			return null;
		}
	}

	public Time getTime(int parameterIndex, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getTime", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getTime");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		Time sqlTime;
		java.util.Date d;

		sqlTime = getTime(parameterIndex);
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

	public Time getTime(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getTime", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getTime");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getTime(parameterIndex);
	}

	public Time getTime(String parameterName, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getTime", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getTime");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getTime(parameterIndex, cal);
	}

	public Timestamp getTimestamp(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getTimestamp", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getTimestamp");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int dataType;
		String timestampStr;
		Timestamp retValue;

		validateGetInvocation(parameterIndex);
		dataType = inputDesc_[parameterIndex - 1].dataType_;
		if (dataType != Types.CHAR && dataType != Types.VARCHAR && dataType != Types.LONGVARCHAR
				&& dataType != Types.DATE && dataType != Types.TIMESTAMP) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "restricted_data_type",
					null);
		}

		// For LOB Support - SB 10/8/2004
		// timestampStr = inputDesc_[parameterIndex - 1].paramValue_;
		timestampStr = getString(parameterIndex);
		if (timestampStr != null) {
			try {
				wasNull_ = false;
				retValue = Timestamp.valueOf(timestampStr);
			} catch (IllegalArgumentException e) {
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
						"invalid_cast_specification", null);
			}
			return retValue;
		} else {
			wasNull_ = true;
			return null;
		}
	}

	public Timestamp getTimestamp(int parameterIndex, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getTimestamp", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getTimestamp");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		Timestamp sqlTimestamp;
		java.util.Date d;
		int nanos;

		sqlTimestamp = getTimestamp(parameterIndex);
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

	public Timestamp getTimestamp(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getTimestamp", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getTimestamp");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getTimestamp(parameterIndex);
	}

	public Timestamp getTimestamp(String parameterName, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getTimestamp", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getTimestamp");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getTimestamp(parameterIndex, cal);
	}

	public URL getURL(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getURL", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getURL");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		clearWarnings();
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "getURL()");
		return null;
	}

	public URL getURL(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "getURL", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("getURL");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		return getURL(parameterName);
	}

	public void registerOutParameter(int parameterIndex, int sqlType) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, sqlType);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "registerOutParameter", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, sqlType);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("registerOutParameter");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		// Ignoring sqlType and scale
		validateGetInvocation(parameterIndex);
		if (inputDesc_[parameterIndex - 1].paramMode_ == DatabaseMetaData.procedureColumnOut) {
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		}
	}

	public void registerOutParameter(int parameterIndex, int sqlType, int scale) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, sqlType, scale);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "registerOutParameter", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, sqlType, scale);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("registerOutParameter");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		// Ignoring sqlType and scale
		validateGetInvocation(parameterIndex);
		if (inputDesc_[parameterIndex - 1].paramMode_ == DatabaseMetaData.procedureColumnOut) {
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		}
	}

	public void registerOutParameter(int parameterIndex, int sqlType, String typeName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, sqlType, typeName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "registerOutParameter", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex, sqlType, typeName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("registerOutParameter");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		// Ignoring sqlType and typeName
		validateGetInvocation(parameterIndex);
		if (inputDesc_[parameterIndex - 1].paramMode_ == DatabaseMetaData.procedureColumnOut) {
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		}
	}

	public void registerOutParameter(String parameterName, int sqlType) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, sqlType);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "registerOutParameter", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, sqlType);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("registerOutParameter");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		registerOutParameter(parameterIndex, sqlType);
	}

	public void registerOutParameter(String parameterName, int sqlType, int scale) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, sqlType, scale);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "registerOutParameter", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, sqlType, scale);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("registerOutParameter");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		registerOutParameter(parameterIndex, sqlType, scale);
	}

	public void registerOutParameter(String parameterName, int sqlType, String typeName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, sqlType, typeName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "registerOutParameter", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, sqlType, typeName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("registerOutParameter");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateGetInvocation(parameterName);
		registerOutParameter(parameterIndex, sqlType, typeName);
	}

	public void setAsciiStream(String parameterName, InputStream x, int length) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x, length);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setAsciiStream", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x, length);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setAsciiStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setAsciiStream(parameterIndex, x, length);
	}

	public void setBigDecimal(String parameterName, BigDecimal x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setBigDecimal", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setBigDecimal");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setBigDecimal(parameterIndex, x);
	}

	public void setBinaryStream(String parameterName, InputStream x, int length) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x, length);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setBinaryStream", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x, length);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setBinaryStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setBinaryStream(parameterIndex, x, length);
	}

	public void setBoolean(String parameterName, boolean x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setBoolean", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setBoolean");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setBoolean(parameterIndex, x);
	}

	public void setByte(String parameterName, byte x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setByte", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setByte");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setByte(parameterIndex, x);
	}

	public void setBytes(String parameterName, byte[] x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setBytes", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setBytes");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setBytes(parameterIndex, x);
	}

	public void setCharacterStream(String parameterName, Reader reader, int length) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, reader, length);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setCharacterStream", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, reader, length);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setCharacterStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setCharacterStream(parameterIndex, reader, length);
	}

	public void setDate(String parameterName, Date x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setDate", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setDate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setDate(parameterIndex, x);
	}

	public void setDate(String parameterName, Date x, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setDate", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setDate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setDate(parameterIndex, x, cal);
	}

	public void setDouble(String parameterName, double x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setDouble", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setDouble");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setDouble(parameterIndex, x);
	}

	public void setFloat(String parameterName, float x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setFloat", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setFloat");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setFloat(parameterIndex, x);
	}

	public void setInt(String parameterName, int x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setInt", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setInt");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setInt(parameterIndex, x);
	}

	public void setLong(String parameterName, long x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setLong", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setLong");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setLong(parameterIndex, x);
	}

	public void setNull(String parameterName, int sqlType) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, sqlType);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setNull", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, sqlType);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setNull");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setNull(parameterIndex, sqlType);
	}

	public void setNull(String parameterName, int sqlType, String typeName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, sqlType, typeName);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setNull", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, sqlType, typeName);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setNull");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setNull(parameterIndex, sqlType, typeName);
	}

	public void setObject(String parameterName, Object x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setObject", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setObject(parameterIndex, x);
	}

	public void setObject(String parameterName, Object x, int targetSqlType) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x, targetSqlType);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setObject", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x, targetSqlType);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setObject(parameterIndex, x, targetSqlType);
	}

	public void setObject(String parameterName, Object x, int targetSqlType, int scale) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x, targetSqlType, scale);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setObject", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x, targetSqlType, scale);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setObject");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setObject(parameterIndex, x, targetSqlType, scale);
	}

	public void setShort(String parameterName, short x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setShort", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setShort");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setShort(parameterIndex, x);
	}

	public void setString(String parameterName, String x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setString", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setString");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setString(parameterIndex, x);
	}

	public void setTime(String parameterName, Time x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setTime", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setTime");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setTime(parameterIndex, x);
	}

	public void setTime(String parameterName, Time x, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setTime", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setTime");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setTime(parameterIndex, x, cal);
	}

	public void setTimestamp(String parameterName, Timestamp x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setTimestamp", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setTimestamp");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setTimestamp(parameterIndex, x);
	}

	public void setTimestamp(String parameterName, Timestamp x, Calendar cal) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x, cal);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setTimestamp", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x, cal);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setTimestamp");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setTimestamp(parameterIndex, x, cal);
	}

	public void setUnicodeStream(String parameterName, InputStream x, int length) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x, length);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setUnicodeStream", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x, length);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setUnicodeStream");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setUnicodeStream(parameterIndex, x, length);
	}

	public void setURL(String parameterName, URL x) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "setURL", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName, x);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("setURL");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		int parameterIndex = validateSetInvocation(parameterName);
		setURL(parameterIndex, x);
	}

	public boolean wasNull() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "wasNull", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("wasNull");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		return wasNull_;
	}

	public boolean execute() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "execute", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("execute");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		Object[] valueArray = null;
		int inDescLength = 0;
		if (inputDesc_ != null) {
			valueArray = getValueArray();
			inDescLength = inputDesc_.length;
		}

		validateExecuteInvocation();

		valueArray = getValueArray();
		ist_.execute(TRANSPORT.SRVR_API_SQLEXECUTE2, paramRowCount_, inDescLength, valueArray, queryTimeout_, null,
				this);

		// SPJ: 5-18-2007
		// if (resultSet_[result_set_offset] != null)
		if (resultSet_[result_set_offset] != null && resultSet_[result_set_offset].spj_rs_) {
			return true;
		} else {
			return false;
		}
	}

	public int[] executeBatch() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "executeBatch", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("executeBatch");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if ((batchCommands_ == null) || (paramRowCount_ < 1)) {
			return new int[] {};
		}

		if (batchCommands_.isEmpty()) {
			return new int[] {};
		}

		clearWarnings();
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "executeBatch()");
		return null;
	}

	public ResultSet executeQuery() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "executeQuery", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("executeQuery");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		Object[] valueArray = null;
		int inDescLength = 0;
		if (inputDesc_ != null) {
			valueArray = getValueArray();
			inDescLength = inputDesc_.length;
		}

		validateExecuteInvocation();

		ist_.execute(TRANSPORT.SRVR_API_SQLEXECUTE2, paramRowCount_, inDescLength, valueArray, queryTimeout_, null,
				this);

		return resultSet_[result_set_offset];
	}

	public int executeUpdate() throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "executeUpdate", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("executeUpdate");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		Object[] valueArray = null;
		int inDescLength = 0;
		if (inputDesc_ != null) {
			valueArray = getValueArray();
			inDescLength = inputDesc_.length;
		}

		validateExecuteInvocation();
		valueArray = getValueArray();
		ist_.execute(TRANSPORT.SRVR_API_SQLEXECUTE2, paramRowCount_, inDescLength, valueArray, queryTimeout_, null,
				this);

		return (1);
	}

	// Other methods
	protected void validateGetInvocation(int parameterIndex) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterIndex);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4CallableStatement", "validateGetInvocation", "", p);
		}
		clearWarnings();
		// connection_.getServerHandle().isConnectionOpen();
		connection_.isConnectionOpen();
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_statement",
					null);
		}
		if (inputDesc_ == null) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"not_a_output_parameter", null);
		}
		if (parameterIndex < 1 || parameterIndex > inputDesc_.length) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_parameter_index", null);
		}
		if (inputDesc_[parameterIndex - 1].paramMode_ != DatabaseMetaData.procedureColumnInOut
				&& inputDesc_[parameterIndex - 1].paramMode_ != DatabaseMetaData.procedureColumnOut) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"not_a_output_parameter", null);
		}
	}

	protected int validateGetInvocation(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4CallableStatement", "validateGetInvocation", "", p);
		}
		int i;

		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_statement",
					null);
		}
		if (inputDesc_ == null) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"not_a_output_parameter", null);
		}
		for (i = 0; i < inputDesc_.length; i++) {
			if (parameterName.equalsIgnoreCase(inputDesc_[i].name_)) {
				return i + 1;
			}
		}
		throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_parameter_name",
				null);
	}

	private int validateSetInvocation(String parameterName) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, parameterName);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4CallableStatement", "validateSetInvocation", "", p);
		}
		int i;

		if (isClosed_) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "stmt_closed", null);
		}
		if (inputDesc_ == null) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
					"invalid_parameter_index", null);
		}
		for (i = 0; i < inputDesc_.length; i++) {
			if (parameterName.equalsIgnoreCase(inputDesc_[i].name_)) {
				return i + 1;
			}
		}
		throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_parameter_name",
				null);
	}

	void setExecuteCallOutputs(Object[] outputValues, short rowsAffected) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, outputValues, rowsAffected);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4CallableStatement", "setExecuteCallOutputs", "", p);
		}
		if (outputValues != null) {
			for (int i = 0; i < outputValues.length; i++) {
				if (outputValues[i] == null) {
					inputDesc_[i].paramValue_ = null;
				} else if (outputValues[i] instanceof byte[]) {
					inputDesc_[i].paramValue_ = outputValues[i];
				} else {
					inputDesc_[i].paramValue_ = outputValues[i].toString();
				}
			}
		}
		returnResultSet_ = rowsAffected;
	}

	// Constructors with access specifier as "default"
	TrafT4CallableStatement(TrafT4Connection connection, String sql) throws SQLException {
		super(connection, sql);
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
	}

	TrafT4CallableStatement(TrafT4Connection connection, String sql, String stmtLabel) throws SQLException {
		super(connection, sql, stmtLabel);
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
	}

	TrafT4CallableStatement(TrafT4Connection connection, String sql, int resultSetType, int resultSetConcurrency)
			throws SQLException {
		super(connection, sql, resultSetType, resultSetConcurrency);
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql, resultSetType);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql, resultSetType);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
	}

	TrafT4CallableStatement(TrafT4Connection connection, String sql, int resultSetType, int resultSetConcurrency,
			int resultSetHoldability) throws SQLException {
		super(connection, sql, resultSetType, resultSetConcurrency, resultSetHoldability);
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql, resultSetType,
					resultSetConcurrency, resultSetHoldability);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4CallableStatement", "", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, sql, resultSetType,
					resultSetConcurrency, resultSetHoldability);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4CallableStatement");
			lr.setSourceMethodName("");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
	}
	
	// Interface methods
	void prepareCall(String sql, int queryTimeout, int holdability) throws SQLException {
		super.ist_.prepare(sql, queryTimeout, this);
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, sql, queryTimeout, holdability);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4CallableStatement", "prepareCall", "", p);
		}
	};

	void executeCall(int inputParamCount, Object[] inputParamValues, int queryTimeout) throws SQLException {
		/*
		 * super.ist_.execute( inputParamCount, inputParamValues, queryTimeout,
		 * this);
		 */
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, inputParamCount, inputParamValues,
					queryTimeout);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4CallableStatement", "ececuteCall", "", p);
		}
	};

	void cpqPrepareCall(String moduleName, int moduleVersion, long moduleTimestamp, String stmtName, int queryTimeout,
			int holdability) throws SQLException {
		if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, moduleName, moduleVersion, moduleTimestamp,
					stmtName, queryTimeout, holdability);
			connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4CallableStatement", "cpqPrepareCall", "", p);
		}
		throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "unsupported_feature",
				new Object[] { "cpqPrepareCall" });
	};

	// fields
	boolean wasNull_;
	short returnResultSet_;

	public RowId getRowId(int parameterIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public RowId getRowId(String parameterName) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public void setRowId(String parameterName, RowId x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNString(String parameterName, String value)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNCharacterStream(String parameterName, Reader value,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNClob(String parameterName, NClob value) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setClob(String parameterName, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setBlob(String parameterName, InputStream inputStream,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNClob(String parameterName, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	
	/**/
	public Blob getBlob(int parameterIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
	
	public Blob getBlob(String parameterName) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
	
	public Clob getClob(int parameterIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
	
	public Clob getClob(String parameterName) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
	/**/

	public NClob getNClob(int parameterIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public NClob getNClob(String parameterName) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public void setSQLXML(String parameterName, SQLXML xmlObject)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public SQLXML getSQLXML(int parameterIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public SQLXML getSQLXML(String parameterName) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public String getNString(int parameterIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public String getNString(String parameterName) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Reader getNCharacterStream(int parameterIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Reader getNCharacterStream(String parameterName) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Reader getCharacterStream(int parameterIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Reader getCharacterStream(String parameterName) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public void setBlob(String parameterName, Blob x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setClob(String parameterName, Clob x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setAsciiStream(String parameterName, InputStream x, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setBinaryStream(String parameterName, InputStream x, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setCharacterStream(String parameterName, Reader reader,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setAsciiStream(String parameterName, InputStream x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setBinaryStream(String parameterName, InputStream x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setCharacterStream(String parameterName, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNCharacterStream(String parameterName, Reader value)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setClob(String parameterName, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setBlob(String parameterName, InputStream inputStream)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNClob(String parameterName, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public Object getObject(int parameterIndex, Class type)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Object getObject(String parameterName, Class type)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
}
