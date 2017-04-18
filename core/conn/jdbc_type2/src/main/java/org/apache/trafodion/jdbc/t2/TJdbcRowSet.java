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
 * Filename		: TJdbcRowSet.java
 * Description	:
 *
 */


package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.InputStream;
import java.io.Reader;
import java.io.PrintWriter;
import java.math.BigDecimal;
import java.net.URL;
import java.util.Calendar;
import java.util.Date;
import java.util.Map;
import org.apache.trafodion.jdbc.t2.SQLMXJdbcRowSet;

import javax.sql.RowSetListener;
import javax.sql.rowset.JdbcRowSet;
import javax.sql.rowset.RowSetWarning;
import javax.sql.rowset.Joinable;

public class TJdbcRowSet implements JdbcRowSet, Joinable
{
	// javax.sql.RowSet.JdbcRowSet interface methods
	public void commit() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "commit()");
		jrs_.commit();
	}
	
	public boolean getAutoCommit() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "v()");
		boolean retValue;

		retValue = jrs_.getAutoCommit();
		return retValue;
	}
	
	public RowSetWarning getRowSetWarnings()  throws SQLException
	{
		RowSetWarning retValue;

		if (out_ != null)
			out_.println(getTraceId() + "getRowSetWarnings()");
		
		retValue = jrs_.getRowSetWarnings();
		
		if (out_ != null)
			out_.println(getTraceId() + "getRowSetWarnings() returns RowSetWarning [" + System.identityHashCode(retValue) + "]");

		return retValue;
	}

	public boolean getShowDeleted() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getShowDeleted()");
		boolean retValue;

		retValue = jrs_.getShowDeleted();
		return retValue;
	}

	public void rollback()throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "rollback()");
		jrs_.rollback();
	}

	public void rollback(Savepoint savepoint) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "rollback(" + savepoint + ")");
		jrs_.rollback(savepoint);
	}

	public void setAutoCommit(boolean autoCommit)throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setAutoCommit(" + autoCommit + ")");
		jrs_.setAutoCommit(autoCommit);
	}

	public void setShowDeleted(boolean b) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setShowDeleted(" + b + ")");
		jrs_.setShowDeleted(b);
	}
	// End of javax.sql.JdbcRowSet method block

	// ******************************************************
	// The following methods are from the java.sql.RowSet API
	// ******************************************************
	public void addRowSetListener(RowSetListener listener)
	{
		if (out_ != null)
			out_.println(getTraceId() + "addRowSetListener(" + listener + ")");
		jrs_.addRowSetListener(listener);
	}
	
	public void clearParameters() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "clearParameters()");
		jrs_.clearParameters();
	}
	
	public void execute() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "execute()");
		jrs_.execute();
	}
	
	public String getCommand()
	{
		if (out_ != null)
			out_.println(getTraceId() + "getCommand()");
		String retValue;

		retValue = jrs_.getCommand();
		return retValue;
	}

	public String getDataSourceName()
	{
		if (out_ != null)
			out_.println(getTraceId() + "getDataSourceName()");
		String retValue;

		retValue = jrs_.getDataSourceName();
		return retValue;
	}

	public boolean getEscapeProcessing() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getEscapeProcessing()");
		boolean retValue;

		retValue = jrs_.getEscapeProcessing();
		return retValue;
	}

	public int getMaxFieldSize() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getMaxFieldSize()");
		int retValue;
		
		retValue = jrs_.getMaxFieldSize();
		return retValue;
	}
	
	public int getMaxRows() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getMaxRows()");
		int retValue;
		
		retValue = jrs_.getMaxRows();
		return retValue;
	}

	public String getPassword()
	{
		if (out_ != null)
			out_.println(getTraceId() + "getPassword()");
		String retValue;

		retValue = jrs_.getPassword();
		return retValue;
	}
	
	public int getQueryTimeout() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getQueryTimeout()");
		int retValue;
		
		retValue = jrs_.getQueryTimeout();
		return retValue;
	}

	public int getTransactionIsolation()
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTransactionIsolation()");
		int retValue;
		
		retValue = jrs_.getTransactionIsolation();
		return retValue;
	}

	public Map<String,Class<?>> getTypeMap() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTypeMap()");
		Map<String,Class<?>> retValue;

		retValue = jrs_.getTypeMap();
		return retValue;
	}

	public String getUrl() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getUrl()");
		String retValue;

		retValue = jrs_.getUrl();
		return retValue;
	}

	public String getUsername()
	{
		if (out_ != null)
			out_.println(getTraceId() + "getUsername()");
		String retValue;

		retValue = jrs_.getUsername();
		return retValue;
	}

	public boolean isReadOnly()
	{
		if (out_ != null)
			out_.println(getTraceId() + "isReadOnly()");
		boolean retValue;
		
		retValue = jrs_.isReadOnly();
		return retValue;
	}

	public void removeRowSetListener(RowSetListener listener)
	{
		if (out_ != null)
			out_.println(getTraceId() + "removeRowSetListener(" + listener + ")");
		jrs_.removeRowSetListener(listener);
	}
	
	public void setArray(int i, Array x)  throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setArray(" + i + "," + x + ")");
		jrs_.setArray(i, x);
	}
	
	public void setAsciiStream(int parameterIndex, InputStream x, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setAsciiStream(" + parameterIndex + "," + x + "," + length + ")");
		jrs_.setAsciiStream(parameterIndex, x, length);
	}
    
	public void setBigDecimal(int parameterIndex, BigDecimal x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setBigDecimal(" + parameterIndex + "," + x + ")");
		jrs_.setBigDecimal(parameterIndex, x);
	}
    
	public void setBinaryStream(int parameterIndex, InputStream x, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setBinaryStream(" + parameterIndex + "," + x + "," + length + ")");
		jrs_.setBinaryStream(parameterIndex, x, length);
	}
	
	public void setBlob(int parameterIndex, Blob x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setBlob(" + parameterIndex + "," + x + ")");
		jrs_.setBlob(parameterIndex, x);
	}
    
	public void setBoolean(int parameterIndex, boolean x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setBoolean(" + parameterIndex + "," + x + ")");
		jrs_.setBoolean(parameterIndex, x);
	}
    
	public void setByte(int parameterIndex, byte x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setByte(" + parameterIndex + "," + x + ")");
		jrs_.setByte(parameterIndex, x);
	}
    
	public void setBytes(int parameterIndex, byte[] x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setBytes(" + parameterIndex + "," + x + ")");
		jrs_.setBytes(parameterIndex, x);
	}
    
	public void setCharacterStream(int parameterIndex, Reader x, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setCharacterStream(" + parameterIndex + "," + x + "," + length + ")");
		jrs_.setCharacterStream(parameterIndex, x, length);
	}

	public void setClob(int parameterIndex, Clob x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setClob(" + parameterIndex + "," + x + ")");
		jrs_.setClob(parameterIndex, x);
	}

	public void setCommand(String cmd) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setCommand(" + cmd + ")");
		jrs_.setCommand(cmd);
	}

	public void setConcurrency(int concurrency) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setConcurrency(" + concurrency + ")");
		jrs_.setConcurrency(concurrency);
	}

	public void setDataSourceName(String name) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setDataSourceName(" + name + ")");
		jrs_.setDataSourceName(name);
	}

	public void setDate(int parameterIndex, java.sql.Date x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setDate(" + parameterIndex + "," + x + ")");
		jrs_.setDate(parameterIndex, x);
	}
    
	public void setDate(int parameterIndex, java.sql.Date x, Calendar cal) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setDate(" + parameterIndex + "," + x + "," + cal + ")");
		jrs_.setDate(parameterIndex, x, cal);
	}
    
	public void setDouble(int parameterIndex, double x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setDouble(" + parameterIndex + "," + x + ")");
		jrs_.setDouble(parameterIndex, x);
	}

	public void setEscapeProcessing(boolean enable) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setEscapeProcessing(" + enable + ")");
		jrs_.setEscapeProcessing(enable);
	}

	public void setFloat(int parameterIndex, float x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setFloat(" + parameterIndex + "," + x + ")");
		jrs_.setFloat(parameterIndex, x);
	}
    
	public void setInt(int parameterIndex, int x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setInt(" + parameterIndex + "," + x + ")");
		jrs_.setInt(parameterIndex, x);
	}
    
	public void setLong(int parameterIndex, long x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setLong(" + parameterIndex + "," + x + ")");
		jrs_.setLong(parameterIndex, x);
	}

	public void setMaxFieldSize(int max) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setMaxFieldSize(" + max + ")");
		jrs_.setMaxFieldSize(max);
	}

	public void setMaxRows(int max) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setMaxRows(" + max + ")");
		jrs_.setMaxRows(max);
	}

	public void setNull(int parameterIndex, int sqlType) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setNull(" + parameterIndex + "," + sqlType + ")");
		jrs_.setNull(parameterIndex, sqlType);
	}
    
	public void setNull(int parameterIndex, int sqlType, String typeName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setNull(" + parameterIndex + "," + sqlType + "," + typeName + ")");
		jrs_.setNull(parameterIndex, sqlType, typeName);
	}
    
	public void setObject(int parameterIndex, Object x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setObject(" + parameterIndex + "," + x + ")");
		jrs_.setObject(parameterIndex, x);
	}

	public void setObject(int parameterIndex, Object x, int targetSqlType) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setObject(" + parameterIndex + "," + x + "," + targetSqlType + ")");
		jrs_.setObject(parameterIndex, x, targetSqlType);
	}
    
	public void setObject(int parameterIndex, Object x, int targetSqlType, int scale) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setObject(" + parameterIndex + "," + x + "," +
				targetSqlType + "," + scale + ")");
		jrs_.setObject(parameterIndex, x, targetSqlType, scale);
	}

	public void setPassword(String password) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setPassword(" + password + ")");
		jrs_.setPassword(password);
	}

	public void setQueryTimeout(int seconds) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setQueryTimeout(" + seconds + ")");
		jrs_.setQueryTimeout(seconds);
	}

	public void setReadOnly(boolean value) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setReadOnly(" + value + ")");
		jrs_.setReadOnly(value);
	}

	public void setRef(int parameterIndex, Ref x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setRef(" + parameterIndex + "," + x + ")");
		jrs_.setRef(parameterIndex, x);
	}
    
	public void setShort(int parameterIndex, short x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setShort(" + parameterIndex + "," + x + ")");
		jrs_.setShort(parameterIndex, x);
	}
    
	public void setString(int parameterIndex, String x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setString(" + parameterIndex + "," + x + ")");
		jrs_.setString(parameterIndex, x);
	}
    
	public void setTime(int parameterIndex, Time x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setTime(" + parameterIndex + "," + x + ")");
		jrs_.setTime(parameterIndex, x);
	}
    
	public void setTime(int parameterIndex, Time x, Calendar cal) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setTime(" + parameterIndex + "," + x + "," + cal + ")");
		jrs_.setTime(parameterIndex, x, cal);
	}

	public void setTimestamp(int parameterIndex, Timestamp x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setTimestamp(" + parameterIndex + "," + x + ")");
		jrs_.setTimestamp(parameterIndex, x);
	}
    
	public void setTimestamp(int parameterIndex, Timestamp x, Calendar cal) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setTimestamp(" + parameterIndex + "," + x + "," + cal + ")");
		jrs_.setTimestamp(parameterIndex, x, cal);
	}

	public void setTransactionIsolation(int level) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setTransactionIsolation(" + level + ")");
		jrs_.setTransactionIsolation(level);
	}

	public void setType(int type) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setType(" + type + ")");
		jrs_.setType(type);
	}

	public void setTypeMap(Map<String,Class<?>> map) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setTypeMap(" + map + ")");
		jrs_.setTypeMap(map);
	}

	public void setUrl(String url) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setUrl(" + url + ")");
		jrs_.setUrl(url);
	}
	
	public void setUsername(String name) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setUsername(" + name + ")");
		jrs_.setUsername(name);
	}
    // End of java.sql.RowSet method block

	// *********************************************************
	// The following methods are from the java.sql.ResultSet API
	// *********************************************************
	public boolean absolute(int row) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "absolute(" + row + ")");
		boolean retValue;

		retValue = jrs_.absolute(row);
		return retValue;
	}

	public void afterLast() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "afterLast()");
		jrs_.afterLast();
	}

	public void beforeFirst() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "beforeFirst()");
		jrs_.beforeFirst();
	}

	public void cancelRowUpdates() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "cancelRowUpdates()");
		jrs_.cancelRowUpdates();
	}

	public void clearWarnings() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "clearWarnings()");
		jrs_.clearWarnings();
	}

	public void close() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "close()");
		jrs_.close();
	}

	public void deleteRow() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "deleteRow()");
		jrs_.deleteRow();
	}

	public int findColumn(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "findColumn(" + columnName + ")");
		int retValue;
		
		retValue = jrs_.findColumn(columnName);
		return retValue;
	}

	public boolean first() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "first()");
		boolean retValue;

		retValue = jrs_.first();
		return retValue;
	}

	// JDK 1.2
	public Array getArray(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getArray(" + columnIndex + ")");
		Array retValue;
		
		retValue = jrs_.getArray(columnIndex);
		return retValue;
	}
	
	// JDK 1.2
	public Array getArray(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getArray(" + columnName + ")");
		Array retValue;
		
		retValue = jrs_.getArray(columnName);
		return retValue;
	}
	
	public InputStream getAsciiStream(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getAsciiStream(" + columnIndex + ")");
		InputStream	retValue;
		
		retValue = jrs_.getAsciiStream(columnIndex);
		return retValue;
	}
	
	public InputStream getAsciiStream(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getAsciiStream(" + columnName + ")");
		InputStream	retValue;
		
		retValue = jrs_.getAsciiStream(columnName);
		return retValue;
	}
	
	public BigDecimal getBigDecimal(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBigDecimal(" + columnIndex + ")");
		BigDecimal retValue;
		
		retValue = jrs_.getBigDecimal(columnIndex);
		return retValue;
	}
	
	@Deprecated public BigDecimal getBigDecimal(int columnIndex, int scale) throws SQLException
				{
					if (out_ != null)
						out_.println(getTraceId() + "getBigDecimal(" + columnIndex + "," + scale + ")");
					BigDecimal retValue;
		
					retValue = jrs_.getBigDecimal(columnIndex, scale);
					return retValue;
				}
	
	public BigDecimal getBigDecimal(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBigDecimal(" + columnName + ")");
		BigDecimal retValue;
		
		retValue = jrs_.getBigDecimal(columnName);
		return retValue;
	}
	
	@Deprecated public BigDecimal getBigDecimal(String columnName, int scale) throws SQLException
				{
					if (out_ != null)
						out_.println(getTraceId() + "getBigDecimal(" + columnName + "," + scale + ")");
					BigDecimal retValue;
		
					retValue = jrs_.getBigDecimal(columnName, scale);
					return retValue;
				}
	
	public InputStream getBinaryStream(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBinaryStream(" + columnIndex + ")");
		InputStream retValue;
		
		retValue = jrs_.getBinaryStream(columnIndex);
		return retValue;
	}
	
	public InputStream getBinaryStream(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBinaryStream(" + columnName + ")");
		InputStream retValue;
		
		retValue = jrs_.getBinaryStream(columnName);
		return retValue;
	}
	
	// JDK 1.2
	public Blob getBlob(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBlob(" + columnIndex + ")");
		Blob retValue;
		
		retValue = jrs_.getBlob(columnIndex);
		if (retValue != null)
			return new TBlob(retValue, out_);
		else
			return retValue;
	}
	
	// JDK 1.2
	public Blob getBlob(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBlob(" + columnName + ")");
		Blob retValue;
		
		retValue = jrs_.getBlob(columnName);
		if (retValue != null)
			return new TBlob(retValue, out_);
		else
			return retValue;
	}
	
	public boolean getBoolean(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBoolean(" + columnIndex + ")");
		boolean retValue;
		
		retValue = jrs_.getBoolean(columnIndex);
		return retValue;
	}
	
	public boolean getBoolean(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBoolean(" + columnName + ")");
		boolean retValue;
		
		retValue = jrs_.getBoolean(columnName);
		return retValue;
	}
	
	public byte getByte(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getByte(" + columnIndex + ")");
		byte retValue;
		
		retValue = jrs_.getByte(columnIndex);
		return retValue;
	}
	
	public byte getByte(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getByte(" + columnName + ")");
		byte retValue;
		
		retValue = jrs_.getByte(columnName);
		return retValue;
	}
	
	public byte[] getBytes(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBytes(" + columnIndex + ")");
		byte[] retValue;
		
		retValue = jrs_.getBytes(columnIndex);
		return retValue;
	}
	
	public byte[] getBytes(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBytes(" + columnName + ")");
		byte[] retValue;
		
		retValue = jrs_.getBytes(columnName);
		return retValue;
	}
	
	public Reader getCharacterStream(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getCharacterStream(" + columnIndex + ")");
		Reader retValue;
		
		retValue = jrs_.getCharacterStream(columnIndex);
		return retValue;
	}
	
	public Reader getCharacterStream(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getCharacterStream(" + columnName + ")");
		Reader retValue;
		
		retValue = jrs_.getCharacterStream(columnName);
		return retValue;
	}
	
	// JDK 1.2
	public Clob getClob(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getClob(" + columnIndex + ")");
		Clob retValue;
		
		retValue = jrs_.getClob(columnIndex);
		if (retValue != null)
			return new TClob(retValue, out_);
		else
			return retValue;
	}
	
	// JDK 1.2
	public Clob getClob(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getClob(" + columnName + ")");
		Clob retValue;
		
		retValue = jrs_.getClob(columnName);
		if (retValue != null)
			return new TClob(retValue, out_);
		else
			return retValue;
	}
	
	public int getConcurrency() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getConcurrency()");
		int retValue;
		
		retValue = jrs_.getConcurrency();
		return retValue;
	}
	
	public String getCursorName() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getCursorName()");
		String retValue;
		
		retValue = jrs_.getCursorName();
		return retValue;
	}
	
	public java.sql.Date getDate(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getDate(" + columnIndex + ")");
		java.sql.Date retValue;
		
		retValue = jrs_.getDate(columnIndex);
		return retValue;
	}
	
	public java.sql.Date getDate(int columnIndex, Calendar cal) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getDate(" + columnIndex + "," + cal + ")");
		java.sql.Date retValue;
		
		retValue = jrs_.getDate(columnIndex, cal);
		return retValue;
	}
	
	public java.sql.Date getDate(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getDate(" + columnName + ")");
		java.sql.Date retValue;
		
		retValue = jrs_.getDate(columnName);
		return retValue;
	}
	
	public java.sql.Date getDate(String columnName, Calendar cal) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getDate(" + columnName + "," + cal + ")");
		java.sql.Date retValue;
		
		retValue = jrs_.getDate(columnName, cal);
		return retValue;
	}
	
	public double getDouble(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getDouble(" + columnIndex + ")");
		double retValue;
		
		retValue = jrs_.getDouble(columnIndex);
		return retValue;
	}
	
	public double getDouble(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getDouble(" + columnName + ")");
		double retValue;
		
		retValue = jrs_.getDouble(columnName);
		return retValue;
	}
	
	public int getFetchDirection() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getFetchDirection()");
		int retValue;
		
		retValue = jrs_.getFetchDirection();
		return retValue;
	}
	
	public int getFetchSize() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getFetchSize()");
		int retValue;
		
		retValue = jrs_.getFetchSize();
		return retValue;
	}
	
	public float getFloat(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getFloat(" + columnIndex + ")");
		float retValue;
		
		retValue = jrs_.getFloat(columnIndex);
		return retValue;
	}
	
	public float getFloat(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getFloat(" + columnName + ")");
		float retValue;
		
		retValue = jrs_.getFloat(columnName);
		return retValue;
	}
	
	public int getInt(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getInt(" + columnIndex + ")");
		int retValue;
		
		retValue = jrs_.getInt(columnIndex);
		return retValue;
	}
	
	public int getInt(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getInt(" + columnName + ")");
		int retValue;
		
		retValue = jrs_.getInt(columnName);
		return retValue;
	}
	
	public long getLong(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getLong(" + columnIndex + ")");
		long retValue;
		
		retValue = jrs_.getLong(columnIndex);
		return retValue;
	}
	
	public long getLong(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getLong(" + columnName + ")");
		long retValue;
		
		retValue = jrs_.getLong(columnName);
		return retValue;
	}
	
	public ResultSetMetaData getMetaData() throws SQLException
	{
		ResultSetMetaData rsMD;
		
		if (out_ != null)
			out_.println(getTraceId() + "getMetaData()");

		rsMD = jrs_.getMetaData();

		if (out_ != null)
			out_.println(getTraceId() + "getMetaData() returns ResultSetMetaData [" + System.identityHashCode(rsMD) + "]");

		return new TResultSetMetaData(rsMD, out_);
	}

  	public ParameterMetaData getParameterMetaData() throws SQLException
	{
		ParameterMetaData pMD;
		
		if (out_ != null)
			out_.println(getTraceId() + "getParameterMetaData()");

		pMD = ((SQLMXJdbcRowSet)jrs_).getParameterMetaData();

		if (out_ != null)
			out_.println(getTraceId() + "getParameterMetaData() returns ParameterMetaData [" + System.identityHashCode(pMD) + "]");

		return pMD;
	}

	public DatabaseMetaData getDatabaseMetaData() throws SQLException
	{
		DatabaseMetaData dbMD;
		
		if (out_ != null)
			out_.println(getTraceId() + "getDatabaseMetaData()");

		dbMD = ((SQLMXJdbcRowSet)jrs_).getDatabaseMetaData();

		if (out_ != null)
			out_.println(getTraceId() + "getDatabaseMetaData() returns DatabaseMetaData [" + System.identityHashCode(dbMD) + "]");

		return dbMD;
	}
	
	public Object getObject(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getObject(" + columnIndex + ")");
		Object retValue;
		
		retValue = jrs_.getObject(columnIndex);
		return retValue;
	}
	
	// JDK 1.2
	public Object getObject(int columnIndex, Map<String,Class<?>> map) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getObject(" + columnIndex + "," + map + ")");
		Object retValue;
		
		retValue = jrs_.getObject(columnIndex, map);
		return retValue;
	}
	
	public Object getObject(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getObject(" + columnName + ")");
		Object retValue;
		
		retValue = jrs_.getObject(columnName);
		return retValue;
	}
	
	// JDK 1.2
	public Object getObject(String columnName, Map<String,Class<?>> map) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getObject(" + columnName + "," + map + ")");
		Object retValue;
		
		retValue = jrs_.getObject(columnName, map);
		return retValue;
	}
	
	// JDK 1.2
	public Ref getRef(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getRef(" + columnIndex + ")");
		Ref retValue;
		
		retValue = jrs_.getRef(columnIndex);
		return retValue;
	}
	
	// JDK 1.2
	public Ref getRef(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getRef(" + columnName + ")");
		Ref retValue;
		
		retValue = jrs_.getRef(columnName);
		return retValue;
	}
	
	public int getRow() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getRow()");
		int retValue;
		
		retValue = jrs_.getRow();
		return retValue;
	}
	
	public short getShort(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getShort(" + columnIndex + ")");
		short retValue;
		
		retValue = jrs_.getShort(columnIndex);
		return retValue;
	}
	
	public short getShort(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getShort(" + columnName + ")");
		short retValue;
		
		retValue = jrs_.getShort(columnName);
		return retValue;
	}
	
	public Statement getStatement() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getStatement()");
		Statement stmt;
		
		stmt = jrs_.getStatement();
		return stmt;
	}
	
	public String getString(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getString(" + columnIndex + ")");
		String retValue;
		
		retValue = jrs_.getString(columnIndex);
		return retValue;
	}
	
	public String getString(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getString(" + columnName + ")");
		String retValue;
		
		retValue = jrs_.getString(columnName);
		return retValue;
	}
	
	public Time getTime(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTime(" + columnIndex + ")");
		Time retValue;
		
		retValue = jrs_.getTime(columnIndex);
		return retValue;
	}
	
	public Time getTime(int columnIndex, Calendar cal) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTime(" + columnIndex + "," + cal + ")");
		Time retValue;
		
		retValue = jrs_.getTime(columnIndex, cal);
		return retValue;
	}
	
	public Time getTime(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTime(" + columnName + ")");
		Time retValue;
		
		retValue = jrs_.getTime(columnName);
		return retValue;
	}
	
	public Time getTime(String columnName, Calendar cal) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTime(" + columnName + "," + cal + ")");
		Time retValue;
		
		retValue = jrs_.getTime(columnName, cal);
		return retValue;
	}
	
	public Timestamp getTimestamp(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTimestamp(" + columnIndex + ")");
		Timestamp retValue;
		
		retValue = jrs_.getTimestamp(columnIndex);
		return retValue;
	}
	
	public Timestamp getTimestamp(int columnIndex, Calendar cal) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTimestamp(" + columnIndex + "," + cal + ")");
		Timestamp retValue;
		
		retValue = jrs_.getTimestamp(columnIndex, cal);
		return retValue;
	}
	
	public Timestamp getTimestamp(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTimestamp(" + columnName + ")");
		Timestamp retValue;
		
		retValue = jrs_.getTimestamp(columnName);
		return retValue;
	}
	
	public Timestamp getTimestamp(String columnName, Calendar cal) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTimestamp(" + columnName + "," + cal + ")");
		Timestamp retValue;
		
		retValue = jrs_.getTimestamp(columnName, cal);
		return retValue;
	}
	
	public int getType() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getType()");
		int retValue;
		
		retValue = jrs_.getType();
		return retValue;
	}
	
	@Deprecated public InputStream getUnicodeStream(int columnIndex) throws SQLException
				{
					if (out_ != null)
						out_.println(getTraceId() + "getUnicodeStream(" + columnIndex + ")");
					InputStream retValue;
		
					retValue = jrs_.getUnicodeStream(columnIndex);
					return retValue;
				}
	
	@Deprecated public InputStream getUnicodeStream(String columnName) throws SQLException
				{
					if (out_ != null)
						out_.println(getTraceId() + "getUnicodeStream(" + columnName + ")");
					InputStream retValue;
		
					retValue = jrs_.getUnicodeStream(columnName);
					return retValue;
				}
	
	public URL getURL(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getURL(" + columnIndex + ")");
		URL retValue;
		
		retValue = jrs_.getURL(columnIndex);
		return retValue;
	}
	
	public URL getURL(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getURL(\"" + columnName + "\")");
		URL retValue;
		
		retValue = jrs_.getURL(columnName);
		return retValue;
	}
	
	public SQLWarning getWarnings() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getWarnings()");
		SQLWarning retValue;
		
		retValue = jrs_.getWarnings();
		return retValue;
	}
	
	public void insertRow() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "insertRow()");
		jrs_.insertRow();
	}
	
	public boolean isAfterLast() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "isAfterLast()");
		boolean retValue;
		
		retValue = jrs_.isAfterLast();
		return retValue;
	}
	
	public boolean isBeforeFirst() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "isBeforeFirst()");
		boolean retValue;
		
		retValue = jrs_.isBeforeFirst();
		return retValue;
	}
	
	public boolean isFirst() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "isFirst()");
		boolean retValue;
		
		retValue = jrs_.isFirst();
		return retValue;
	}
	
	public boolean isLast() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "isLast()");
		boolean retValue;
		
		retValue = jrs_.isLast();
		return retValue;
	}
	
	public boolean last() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "last()");
		boolean retValue;
		
		retValue = jrs_.last();
		return retValue;
	}
	
	public void moveToCurrentRow() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "moveToCurrentRow()");
		jrs_.moveToCurrentRow();
	}
	
	public void moveToInsertRow() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "moveToInsertRow()");
		jrs_.moveToInsertRow();
	}
	
	public boolean next() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "next()");
		boolean retValue;
		
		retValue = jrs_.next();
		return retValue;
	}
	
	public boolean previous() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "previous()");
		boolean retValue;
		
		retValue = jrs_.previous();
		return retValue;
	}
	
	public void refreshRow() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "refreshRow()");
		jrs_.refreshRow();
	}
	
	public boolean relative(int row) throws SQLException
	{
		boolean retValue;
		if (out_ != null)
			out_.println(getTraceId() + "relative(" + row + ")");
		
		retValue = jrs_.relative(row);
		return retValue;
	}
	
	public boolean rowDeleted() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "rowDeleted()");
		boolean retValue;
		
		retValue = jrs_.rowDeleted();
		return retValue;
	}
	
	public boolean rowInserted() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "rowInserted()");
		boolean retValue;
		
		retValue = jrs_.rowInserted();
		return retValue;
	}
	
	public boolean rowUpdated() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "rowUpdated()");
		boolean retValue;
		
		retValue = jrs_.rowUpdated();
		return retValue;
	}
	
	public void setFetchDirection(int direction) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setFetchDirection(" + direction + ")");
		jrs_.setFetchDirection(direction);
	}
	
	public void setFetchSize(int rows) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setFetchSize(" + rows + ")");
		jrs_.setFetchSize(rows);
	}
	
	public void updateArray(String str, Array array) throws SQLException
	{
		
		if (out_ != null)
		{
			out_.println(getTraceId() + "updateArray(" + str + "," + array + ")");
		}
		jrs_.updateArray(str, array);
	}
	
	public void updateArray(int param, Array array) throws SQLException
	{
		
		if (out_ != null)
		{
			out_.println(getTraceId() + "updateArray(" + param + "," + array + ")");
		}
		jrs_.updateArray(param, array);
	}
	
	public void updateAsciiStream(int columnIndex, InputStream x, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateAsciiStream(" + columnIndex + "," + x + "," + length + ")");
		jrs_.updateAsciiStream(columnIndex, x, length);
	}
	
	public void updateAsciiStream(String columnName, InputStream x, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateAsciiStream(" + columnName + "," + x + "," + length + ")");
		jrs_.updateAsciiStream(columnName, x, length);
	}
	
	public void updateBigDecimal(int columnIndex, BigDecimal x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateBigDecimal(" + columnIndex + "," + x + ")");
		jrs_.updateBigDecimal(columnIndex, x);
	}
	
	public void updateBigDecimal(String columnName, BigDecimal x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateBigDecimal(" + columnName + "," + x + ")");
		jrs_.updateBigDecimal(columnName, x);
	}
	
	public void updateBinaryStream(int columnIndex, InputStream x, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateBinaryStream(" + columnIndex + "," + x +
				"," + length + ")");
		jrs_.updateBinaryStream(columnIndex, x, length);
	}
	
	public void updateBinaryStream(String columnName, InputStream x, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateBinaryStream(" + columnName + "," + x +
				"," + length + ")");
		jrs_.updateBinaryStream(columnName, x, length);
	}
	
	public void updateBlob(int param, Blob blob) throws SQLException
	{
		
		if (out_ != null)
		{
			out_.println(getTraceId() + "updateBlob(" + param + "," + blob + ")");
		}
		jrs_.updateBlob(param, blob);
	}
	
	public void updateBlob(String str, Blob blob) throws SQLException
	{
		
		if (out_ != null)
		{
			out_.println(getTraceId() + "updateBlob(" + str + "," + blob + ")");
		}
		jrs_.updateBlob(str, blob);
	}

	public void updateBoolean(int columnIndex, boolean x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateBoolean(" + columnIndex + "," + x + ")");
		jrs_.updateBoolean(columnIndex, x);
	}
	
	public void updateBoolean(String columnName, boolean x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateBoolean(" + columnName + "," + x + ")");
		jrs_.updateBoolean(columnName, x);
	}
	
	public void updateByte(int columnIndex, byte x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateByte(" + columnIndex + "," + x + ")");
		jrs_.updateByte(columnIndex, x);
	}
	
	public void updateByte(String columnName, byte x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateByte(" + columnName + "," + x + ")");
		jrs_.updateByte(columnName, x);
	}
	
	public void updateBytes(int columnIndex, byte[] x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateBytes(" + columnIndex + "," + x + ")");
		jrs_.updateBytes(columnIndex, x);
	}
	
	public void updateBytes(String columnName, byte[] x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateBytes(" + columnName + "," + x + ")");
		jrs_.updateBytes(columnName, x);
	}
	
	public void updateCharacterStream(int columnIndex, Reader x, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateCharacterStream(" + columnIndex + "," + x +
				"," + length + ")");
		jrs_.updateCharacterStream(columnIndex, x, length);
	}
	
	public void updateCharacterStream(String columnName, Reader x, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateCharacterStream(" + columnName + "," + x +
				"," + length + ")");
		jrs_.updateCharacterStream(columnName, x, length);
	}
	
	public void updateClob(String str, Clob clob) throws SQLException
	{
		
		if (out_ != null)
		{
			out_.println(getTraceId() + "updateClob(" + str + "," + clob + ")");
		}
		jrs_.updateClob(str, clob);
	}
	
	public void updateClob(int param, Clob clob) throws SQLException
	{
		
		if (out_ != null)
		{
			out_.println(getTraceId() + "updateClob(" + param + "," + clob + ")");
		}
		jrs_.updateClob(param, clob);
	}

	public void updateDate(int columnIndex, java.sql.Date x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateDate(" + columnIndex + "," + x + ")");
		jrs_.updateDate(columnIndex, x);
	}
	
	public void updateDate(String columnName, java.sql.Date x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateDate(" + columnName + "," + x + ")");
		jrs_.updateDate(columnName, x);
	}
	
	public void updateDouble(int columnIndex, double x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateDouble(" + columnIndex + "," + x + ")");
		jrs_.updateDouble(columnIndex, x);
	}
	
	public void updateDouble(String columnName, double x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateDouble(" + columnName + "," + x + ")");
		jrs_.updateDouble(columnName, x);
	}
	
	public void updateFloat(int columnIndex, float x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateFloat(" + columnIndex + "," + x + ")");
		jrs_.updateFloat(columnIndex, x);
	}
	
	public void updateFloat(String columnName, float x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateFloat(" + columnName + "," + x + ")");
		jrs_.updateFloat(columnName, x);
	}
	
	public void updateInt(int columnIndex, int x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateInt(" + columnIndex + "," + x + ")");
		jrs_.updateInt(columnIndex, x);
	}
	
	public void updateInt(String columnName, int x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateInt(" + columnName + "," + x + ")");
		jrs_.updateInt(columnName, x);
	}
	
	public void updateLong(int columnIndex, long x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateLong(" + columnIndex + "," + x + ")");
		jrs_.updateLong(columnIndex, x);
	}
	
	public void updateLong(String columnName, long x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateLong(" + columnName + "," + x + ")");
		jrs_.updateLong(columnName, x);
	}
	
	public void updateNull(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateNull(" + columnIndex + ")");
		jrs_.updateNull(columnIndex);
	}
	
	public void updateNull(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateNull(" + columnName + ")");
		jrs_.updateNull(columnName);
	}
	
	public void updateObject(int columnIndex, Object x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateObject(" + columnIndex + "," + x + ")");
		jrs_.updateObject(columnIndex, x);
	}
	
	public void updateObject(int columnIndex, Object x, int scale) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateObject(" + columnIndex + "," + x +
				"," + scale + ")");
		jrs_.updateObject(columnIndex, x, scale);
	}
	
	public void updateObject(String columnName, Object x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateObject(" + columnName + "," + x + ")");
		jrs_.updateObject(columnName, x);
	}
	
	public void updateObject(String columnName, Object x, int scale) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateObject(" + columnName + "," + x +
				"," + scale + ")");
		jrs_.updateObject(columnName, x, scale);
	}
	
	public void updateRef(int param, Ref ref) throws SQLException
	{
		if (out_ != null)
		{
			out_.println(getTraceId() + "updateRef(" + param + "," + ref + ")");
		}
		jrs_.updateRef(param, ref);
	}
	
	public void updateRef(String str, Ref ref) throws SQLException
	{		
		if (out_ != null)
		{
			out_.println(getTraceId() + "updateRef(" + str + "," + ref + ")");
		}
		jrs_.updateRef(str, ref);
	}

	public void updateRow() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateRow()");
		jrs_.updateRow();
	}
	
	public void updateShort(int columnIndex, short x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateShort(" + columnIndex + "," + x + ")");
		jrs_.updateShort(columnIndex, x);
	}
	
	public void updateShort(String columnName, short x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateShort(" + columnName + "," + x + ")");
		jrs_.updateShort(columnName, x);
	}
	
	public void updateString(int columnIndex, String x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateString(" + columnIndex + "," + x + ")");
		jrs_.updateString(columnIndex, x);
	}
	
	public void updateString(String columnName, String x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateString(" + columnName + "," + x + ")");
		jrs_.updateString(columnName, x);
	}
	
	public void updateTime(int columnIndex, Time x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateTime(" + columnIndex + "," + x + ")");
		jrs_.updateTime(columnIndex, x);
	}
	
	public void updateTime(String columnName, Time x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateTime(" + columnName + "," + x + ")");
		jrs_.updateTime(columnName, x);
	}
	
	public void updateTimestamp(int columnIndex, Timestamp x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateTimestamp(" + columnIndex + "," + x + ")");
		jrs_.updateTimestamp(columnIndex, x);
	}
	
	public void updateTimestamp(String columnName, Timestamp x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateTimestamp(" + columnName + "," + x + ")");
		jrs_.updateTimestamp(columnName, x);
	}
	
	public boolean wasNull() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "wasNull()");
		boolean retValue;
		
		retValue = jrs_.wasNull();
		return retValue;
	}
	// End of java.sql.ResultSet method block

	// ****************************************************************
	// The following methods are from the javax.sql.rowset.Joinable API
	// ****************************************************************
	public int[] getMatchColumnIndexes() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getMatchColumnIndexes()");
		int[] retValue;
		
		retValue = jrs_.getMatchColumnIndexes();
		return retValue;
	}

	public String[] getMatchColumnNames() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getMatchColumnNames()");
		String[] retValue;
		
		retValue = jrs_.getMatchColumnNames();
		return retValue;
	}

	public void setMatchColumn(int columnIdx) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setMatchColumn(" + columnIdx + ")");
		jrs_.setMatchColumn(columnIdx);
	}

	public void setMatchColumn(int[] columnIdxes) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setMatchColumn(" + columnIdxes + ")");
		jrs_.setMatchColumn(columnIdxes);
	}

	public void setMatchColumn(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setMatchColumn(" + columnName + ")");
		jrs_.setMatchColumn(columnName);
	}

	public void setMatchColumn(String[] columnNames) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setMatchColumn(" + columnNames + ")");
		jrs_.setMatchColumn(columnNames);
	}

	public void unsetMatchColumn(int columnIdx) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "unsetMatchColumn(" + columnIdx + ")");
		jrs_.unsetMatchColumn(columnIdx);
	}

	public void unsetMatchColumn(int[] columnIdxes) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "unsetMatchColumn(" + columnIdxes + ")");
		jrs_.unsetMatchColumn(columnIdxes);
	}

	public void unsetMatchColumn(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "unsetMatchColumn(" + columnName + ")");
		jrs_.unsetMatchColumn(columnName);
	}

	public void unsetMatchColumn(String[] columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "unsetMatchColumn(" + columnName + ")");
		jrs_.unsetMatchColumn(columnName);
	}
	// End of javax.sql.rowset.Joinable method block

	// Constructor
	public TJdbcRowSet() throws SQLException
	{
		

		jrs_ = new SQLMXJdbcRowSet();
		out_ = ((SQLMXJdbcRowSet)jrs_).traceWriter_;

		
	}
	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		String className = null;
		if (jrs_ != null)
			className = jrs_.getClass().getName();

		// Build up jdbcTrace output entry
		setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText
			+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
			+ "]:[" + Thread.currentThread() + "]:["
			+ System.identityHashCode(jrs_) +  "]:"
			+ jrs_.getClass().getName().substring(org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME,className.length()) 
			+ ".");
		return traceId_;
	}
	// Fields
	JdbcRowSet		jrs_;
	private String			traceId_;
	PrintWriter		out_;

         public void setURL(int parameterIndex, URL x) throws SQLException {
         }

	public void setNClob(int parameterIndex, Reader reader) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNClob(int parameterIndex, NClob value) throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setNClob(int parameterIndex, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setNClob(String parameterName, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setNClob(String parameterName, NClob value) throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setNClob(String parameterName, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setNCharacterStream(String parameterName, Reader value,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNCharacterStream(String parameterName, Reader value)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNCharacterStream(int parameterIndex, Reader value,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNCharacterStream(int parameterIndex, Reader value)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNString(String parameterName, String value)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setNString(int parameterIndex, String value)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setRowId(int parameterIndex, RowId x) throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setRowId(String parameterName, RowId x) throws SQLException {
		// TODO Auto-generated method stub
		
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
	public void setSQLXML(int parameterIndex, SQLXML xmlObject)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setTimestamp(String str, Timestamp timestamp, Calendar calendar) throws SQLException {
        }

	public void setTimestamp(String str, Timestamp timestamp)
			throws SQLException {
        }
	public void setTime(String str, Time time, Calendar calendar)
			throws SQLException {
        }
	public void setTime(String str, Time time)
			throws SQLException {
        }
	public void setDate(String str, java.sql.Date date)
			throws SQLException {
        }
	public void setDate(String str, java.sql.Date dt, Calendar cal)
			throws SQLException {
        }
	public void setClob(String parameterName, Clob clob)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setClob(String parameterName, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
	}

	public void setBlob(String parameterName, Blob blob)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setBlob(String parameterName, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setBlob(String parameterName, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
        }
	public void setBlob(int parameterIndex, InputStream inputStream)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setBlob(String parameterName, InputStream inputStream)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setBlob(String parameterName, InputStream inputStream, long index)
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
	public void setClob(int parameterIndex, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setClob(int parameterIndex, Reader reader,long length)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setClob(String parameterName, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setBlob(int parameterIndex, InputStream inps, long length)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setObject(String parameterName, Object x) throws SQLException {}
	public void setObject(String parameterName, Object x, int k) throws SQLException {}
	public void setObject(String parameterName, Object x, int k , int length) throws SQLException {}
	public void setCharacterStream(String parameterString, Reader x) throws SQLException {}
	public void setCharacterStream(String parameterString, Reader x, int index) throws SQLException {}
	public void setCharacterStream(int parameterIndex, Reader x) throws SQLException {}
	public void setBinaryStream(String parameterString, InputStream x) throws SQLException{}
	public void setBinaryStream(String parameterString, InputStream x, int index) throws SQLException{}
	public void setBinaryStream(int parameterIndex, InputStream x) throws SQLException{}
	public void setAsciiStream(int parameterIndex, InputStream x) throws SQLException{}
	public void setAsciiStream(String parameterName, InputStream x) throws SQLException{}
	public void setAsciiStream(String parameterName, InputStream x, int index) throws SQLException{}
	public void setBytes(String parameterName, byte[] x) throws SQLException{}
	public void setString(String parameterName, String x) throws SQLException{}
	public void setString(String parameterName, byte x) throws SQLException{}

	public void setBigDecimal(String parameterName, BigDecimal x) throws SQLException{}
	public void setDouble(String parameterIndex, double x) throws SQLException{}
	public void setFloat(String parameterIndex, float x) throws SQLException{}
	public void setLong(String parameterIndex, long x) throws SQLException{}
	public void setInt(String parameterIndex, int x) throws SQLException{}
	public void setShort(String parameterIndex, short x) throws SQLException{}
	public void setByte(String parameterName, byte x) throws SQLException{}
	public void setBoolean(String parameterName, boolean x) throws SQLException{}
	public void setNull(String parameterName, int t, String k) throws SQLException{}
	public void setNull(String parameterName, int t) throws SQLException{}
	public void updateNClob(int columnIndex, Reader reader) throws SQLException {
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

        public void updateClob(int columnIndex, Reader reader )
                        throws SQLException {
                // TODO Auto-generated method stub

        }
	public void updateClob(String columnLabel, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void updateNClob(String columnLabel, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
	}

        public void updateBlob(int columnIndex, InputStream inputStream)
                        throws SQLException {
                // TODO Auto-generated method stub
        }
        public void updateBlob(int columnIndex, InputStream inputStream, long length)
                        throws SQLException {
                // TODO Auto-generated method stub
        }

        public void updateBlob(String columnLabel, InputStream inputStream,
                        long length) throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateBlob(String columnLabel, InputStream inputStream) throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateCharacterStream(String columnLabel, Reader reader) throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateAsciiStream(String columnLabel, InputStream x)
                        throws SQLException {
                // TODO Auto-generated method stub

        }

        public void updateBinaryStream(String columnLabel, InputStream x) throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateCharacterStream(int columnIndex, Reader x)
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

        public void updateNClob(int columnIndex, Reader reader, long length)
                        throws SQLException {
                // TODO Auto-generated method stub

        }

        public void updateNClob(String columnLabel, Reader reader, long length)
                        throws SQLException {
                // TODO Auto-generated method stub

        }

        public void updateNCharacterStream(String columnLabel, Reader reader)
                        throws SQLException {
                // TODO Auto-generated method stub

        }

        public void updateNCharacterStream(int columnLabel, Reader reader)
                        throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateCharacterStream(String columnLabel, Reader reader, long s) throws SQLException {
                // TODO Auto-generated method stub

        }

       public void updateBinaryStream(String columnLabel, InputStream x, long t) throws SQLException {
                // TODO Auto-generated method stub

        }
       public void updateAsciiStream(String columnLabel, InputStream x, long t) throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateCharacterStream(int columnIndex, Reader reader, long length)
                        throws SQLException {
                // TODO Auto-generated method stub

        }
       public void updateBinaryStream(int columnLabel, InputStream x, long t) throws SQLException {
                // TODO Auto-generated method stub

        }
       public void updateAsciiStream(int columnLabel, InputStream x, long t) throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateNCharacterStream(int columnLabel, Reader reader, long s) throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateNCharacterStream(String columnLabel, Reader reader, long s) throws SQLException {
                // TODO Auto-generated method stub

        }
	public Reader getNCharacterStream(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Reader getNCharacterStream(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
	public String getNString(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public String getNString(String columnLabel) throws SQLException {
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
	public NClob getNClob(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
	public NClob getNClob(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
	public void updateNClob(int columnIndex, NClob nClob) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNClob(String columnLabel, NClob nClob)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void updateNString(int columnIndex, String nString)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNString(String columnLabel, String nString)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
        public boolean isClosed() throws SQLException {
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
}

