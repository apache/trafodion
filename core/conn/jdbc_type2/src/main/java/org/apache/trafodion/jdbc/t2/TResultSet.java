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
 * Filename		: TResultSet.java
 * Description	:
 *
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.InputStream;
import java.util.Calendar;
import java.math.BigDecimal;
import java.io.Reader;
import java.io.PrintWriter;
import java.net.URL;
import java.util.Date;

// JDK 1.2
import java.util.Map;

public class TResultSet implements java.sql.ResultSet
{
	// java.sql.ResultSet interface methods
	public boolean absolute(int row) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "absolute(" + row + ")");
		boolean retValue;
		
		retValue = rs_.absolute(row);
		return retValue;
	}
	
	public void afterLast() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "afterLast()");
		rs_.afterLast();
	}
	
	public void beforeFirst() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "beforeFirst()");
		rs_.beforeFirst();
	}
	
	public void cancelRowUpdates() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "cancelRowUpdates()");
		rs_.cancelRowUpdates();
	}
	
	public void clearWarnings() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "clearWarnings()");
		rs_.clearWarnings();
	}
	
	public void close() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "close()");
		rs_.close();
	}
	
	public void deleteRow() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "deleteRow()");
		rs_.deleteRow();
	}
	
	public int findColumn(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "findColumn(" + columnName + ")");
		int retValue;
		
		retValue = rs_.findColumn(columnName);
		return retValue;
	}
	
	public boolean first() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "first()");
		boolean retValue;
		
		retValue = rs_.first();
		return retValue;
	}
	
	// JDK 1.2
	public Array getArray(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getArray(" + columnIndex + ")");
		Array retValue;
		
		retValue = rs_.getArray(columnIndex);
		return retValue;
	}
	
	// JDK 1.2
	public Array getArray(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getArray(" + columnName + ")");
		Array retValue;
		
		retValue = rs_.getArray(columnName);
		return retValue;
	}
	
	public InputStream getAsciiStream(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getAsciiStream(" + columnIndex + ")");
		InputStream	retValue;
		
		retValue = rs_.getAsciiStream(columnIndex);
		return retValue;
	}
	
	public InputStream getAsciiStream(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getAsciiStream(" + columnName + ")");
		InputStream	retValue;
		
		retValue = rs_.getAsciiStream(columnName);
		return retValue;
	}
	
	public BigDecimal getBigDecimal(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBigDecimal(" + columnIndex + ")");
		BigDecimal retValue;
		
		retValue = rs_.getBigDecimal(columnIndex);
		return retValue;
	}
	
	@Deprecated public BigDecimal getBigDecimal(int columnIndex, int scale) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBigDecimal(" + columnIndex + "," + scale + ")");
		BigDecimal retValue;
		
		retValue = rs_.getBigDecimal(columnIndex, scale);
		return retValue;
	}
	
	public BigDecimal getBigDecimal(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBigDecimal(" + columnName + ")");
		BigDecimal retValue;
		
		retValue = rs_.getBigDecimal(columnName);
		return retValue;
	}
	
	@Deprecated public BigDecimal getBigDecimal(String columnName, int scale) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBigDecimal(" + columnName + "," + scale + ")");
		BigDecimal retValue;
		
		retValue = rs_.getBigDecimal(columnName, scale);
		return retValue;
	}
	
	public InputStream getBinaryStream(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBinaryStream(" + columnIndex + ")");
		InputStream retValue;
		
		retValue = rs_.getBinaryStream(columnIndex);
		return retValue;
	}
	
	public InputStream getBinaryStream(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBinaryStream(" + columnName + ")");
		InputStream retValue;
		
		retValue = rs_.getBinaryStream(columnName);
		return retValue;
	}
	
	// JDK 1.2
	public Blob getBlob(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBlob(" + columnIndex + ")");
		Blob retValue;
		
		retValue = rs_.getBlob(columnIndex);
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
		
		retValue = rs_.getBlob(columnName);
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
		
		retValue = rs_.getBoolean(columnIndex);
		return retValue;
	}
	
	public boolean getBoolean(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBoolean(" + columnName + ")");
		boolean retValue;
		
		retValue = rs_.getBoolean(columnName);
		return retValue;
	}
	
	public byte getByte(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getByte(" + columnIndex + ")");
		byte retValue;
		
		retValue = rs_.getByte(columnIndex);
		return retValue;
	}
	
	public byte getByte(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getByte(" + columnName + ")");
		byte retValue;
		
		retValue = rs_.getByte(columnName);
		return retValue;
	}
	
	public byte[] getBytes(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBytes(" + columnIndex + ")");
		byte[] retValue;
		
		retValue = rs_.getBytes(columnIndex);
		return retValue;
	}
	
	public byte[] getBytes(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBytes(" + columnName + ")");
		byte[] retValue;
		
		retValue = rs_.getBytes(columnName);
		return retValue;
	}
	
	public Reader getCharacterStream(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getCharacterStream(" + columnIndex + ")");
		Reader retValue;
		
		retValue = rs_.getCharacterStream(columnIndex);
		return retValue;
	}
	
	public Reader getCharacterStream(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getCharacterStream(" + columnName + ")");
		Reader retValue;
		
		retValue = rs_.getCharacterStream(columnName);
		return retValue;
	}
	
	// JDK 1.2
	public Clob getClob(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getClob(" + columnIndex + ")");
		Clob retValue;
		
		retValue = rs_.getClob(columnIndex);
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
		
		retValue = rs_.getClob(columnName);
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
		
		retValue = rs_.getConcurrency();
		return retValue;
	}
	
	public String getCursorName() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getCursorName()");
		String retValue;
		
		retValue = rs_.getCursorName();
		return retValue;
	}
	
	public java.sql.Date getDate(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getDate(" + columnIndex + ")");
		java.sql.Date retValue;
		
		retValue = rs_.getDate(columnIndex);
		return retValue;
	}
	
	public java.sql.Date getDate(int columnIndex, Calendar cal) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getDate(" + columnIndex + "," + cal + ")");
		java.sql.Date retValue;
		
		retValue = rs_.getDate(columnIndex, cal);
		return retValue;
	}
	
	public java.sql.Date getDate(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getDate(" + columnName + ")");
		java.sql.Date retValue;
		
		retValue = rs_.getDate(columnName);
		return retValue;
	}
	
	public java.sql.Date getDate(String columnName, Calendar cal) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getDate(" + columnName + "," + cal + ")");
		java.sql.Date retValue;
		
		retValue = rs_.getDate(columnName, cal);
		return retValue;
	}
	
	public double getDouble(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getDouble(" + columnIndex + ")");
		double retValue;
		
		retValue = rs_.getDouble(columnIndex);
		return retValue;
	}
	
	public double getDouble(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getDouble(" + columnName + ")");
		double retValue;
		
		retValue = rs_.getDouble(columnName);
		return retValue;
	}
	
	public int getFetchDirection() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getFetchDirection()");
		int retValue;
		
		retValue = rs_.getFetchDirection();
		return retValue;
	}
	
	public int getFetchSize() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getFetchSize()");
		int retValue;
		
		retValue = rs_.getFetchSize();
		return retValue;
	}
	
	public float getFloat(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getFloat(" + columnIndex + ")");
		float retValue;
		
		retValue = rs_.getFloat(columnIndex);
		return retValue;
	}
	
	public float getFloat(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getFloat(" + columnName + ")");
		float retValue;
		
		retValue = rs_.getFloat(columnName);
		return retValue;
	}
	
	public int getInt(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getInt(" + columnIndex + ")");
		int retValue;
		
		retValue = rs_.getInt(columnIndex);
		return retValue;
	}
	
	public int getInt(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getInt(" + columnName + ")");
		int retValue;
		
		retValue = rs_.getInt(columnName);
		return retValue;
	}
	
	public long getLong(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getLong(" + columnIndex + ")");
		long retValue;
		
		retValue = rs_.getLong(columnIndex);
		return retValue;
	}
	
	public long getLong(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getLong(" + columnName + ")");
		long retValue;
		
		retValue = rs_.getLong(columnName);
		return retValue;
	}
	
	public ResultSetMetaData getMetaData() throws SQLException
	{
		ResultSetMetaData rsMD;
		
		if (out_ != null)
			out_.println(getTraceId() + "getMetaData()");

		rsMD = rs_.getMetaData();

		if (out_ != null)
			out_.println(getTraceId() + "getMetaData() returns ResultSetMetaData [" + System.identityHashCode(rsMD) + "]");

		return new TResultSetMetaData(rsMD, out_);
	}
	
	public Object getObject(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getObject(" + columnIndex + ")");
		Object retValue;
		
		retValue = rs_.getObject(columnIndex);
		return retValue;
	}
	
	// JDK 1.2
	public Object getObject(int columnIndex, Map<String,Class<?>> map) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getObject(" + columnIndex + "," + map + ")");
		Object retValue;
		
		retValue = rs_.getObject(columnIndex, map);
		return retValue;
	}
	
	public Object getObject(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getObject(" + columnName + ")");
		Object retValue;
		
		retValue = rs_.getObject(columnName);
		return retValue;
	}
	
	// JDK 1.2
	public Object getObject(String columnName, Map<String,Class<?>> map) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getObject(" + columnName + "," + map + ")");
		Object retValue;
		
		retValue = rs_.getObject(columnName, map);
		return retValue;
	}
	
	// JDK 1.2
	public Ref getRef(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getRef(" + columnIndex + ")");
		Ref retValue;
		
		retValue = rs_.getRef(columnIndex);
		return retValue;
	}
	
	// JDK 1.2
	public Ref getRef(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getRef(" + columnName + ")");
		Ref retValue;
		
		retValue = rs_.getRef(columnName);
		return retValue;
	}

	public org.apache.trafodion.jdbc.t2.ResultSetInfo getResultSetInfo() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getResultSetInfo()");
		org.apache.trafodion.jdbc.t2.ResultSetInfo retValue;
		
		retValue = ((org.apache.trafodion.jdbc.t2.SQLMXResultSet)rs_).getResultSetInfo();
		return retValue;
	}

	public int getRow() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getRow()");
		int retValue;
		
		retValue = rs_.getRow();
		return retValue;
	}
	
	public short getShort(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getShort(" + columnIndex + ")");
		short retValue;
		
		retValue = rs_.getShort(columnIndex);
		return retValue;
	}
	
	public short getShort(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getShort(" + columnName + ")");
		short retValue;
		
		retValue = rs_.getShort(columnName);
		return retValue;
	}
	
	public Statement getStatement() throws SQLException
	{
		Statement stmt;
		
		stmt = rs_.getStatement();
		// But return the
		return ts_;
	}
	
	public String getString(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getString(" + columnIndex + ")");
		String retValue;
		
		retValue = rs_.getString(columnIndex);
		return retValue;
	}
	
	public String getString(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getString(" + columnName + ")");
		String retValue;
		
		retValue = rs_.getString(columnName);
		return retValue;
	}
	
	public Time getTime(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTime(" + columnIndex + ")");
		Time retValue;
		
		retValue = rs_.getTime(columnIndex);
		return retValue;
	}
	
	public Time getTime(int columnIndex, Calendar cal) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTime(" + columnIndex + "," + cal + ")");
		Time retValue;
		
		retValue = rs_.getTime(columnIndex, cal);
		return retValue;
	}
	
	public Time getTime(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTime(" + columnName + ")");
		Time retValue;
		
		retValue = rs_.getTime(columnName);
		return retValue;
	}
	
	public Time getTime(String columnName, Calendar cal) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTime(" + columnName + "," + cal + ")");
		Time retValue;
		
		retValue = rs_.getTime(columnName, cal);
		return retValue;
	}
	
	public Timestamp getTimestamp(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTimestamp(" + columnIndex + ")");
		Timestamp retValue;
		
		retValue = rs_.getTimestamp(columnIndex);
		return retValue;
	}
	
	public Timestamp getTimestamp(int columnIndex, Calendar cal) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTimestamp(" + columnIndex + "," + cal + ")");
		Timestamp retValue;
		
		retValue = rs_.getTimestamp(columnIndex, cal);
		return retValue;
	}
	
	public Timestamp getTimestamp(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTimestamp(" + columnName + ")");
		Timestamp retValue;
		
		retValue = rs_.getTimestamp(columnName);
		return retValue;
	}
	
	public Timestamp getTimestamp(String columnName, Calendar cal) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getTimestamp(" + columnName + "," + cal + ")");
		Timestamp retValue;
		
		retValue = rs_.getTimestamp(columnName, cal);
		return retValue;
	}
	
	public int getType() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getType()");
		int retValue;
		
		retValue = rs_.getType();
		return retValue;
	}
	
	@Deprecated public InputStream getUnicodeStream(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getUnicodeStream(" + columnIndex + ")");
		InputStream retValue;
		
		retValue = rs_.getUnicodeStream(columnIndex);
		return retValue;
	}
	
	@Deprecated public InputStream getUnicodeStream(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getUnicodeStream(" + columnName + ")");
		InputStream retValue;
		
		retValue = rs_.getUnicodeStream(columnName);
		return retValue;
	}
	
	public URL getURL(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getURL(" + columnIndex + ")");
		URL retValue;
		
		retValue = rs_.getURL(columnIndex);
		return retValue;
	}
	
	public URL getURL(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getURL(\"" + columnName + "\")");
		URL retValue;
		
		retValue = rs_.getURL(columnName);
		return retValue;
	}
	
	public SQLWarning getWarnings() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getWarnings()");
		SQLWarning retValue;
		
		retValue = rs_.getWarnings();
		return retValue;
	}
	
	public void insertRow() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "insertRow()");
		rs_.insertRow();
	}
	
	public boolean isAfterLast() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "isAfterLast()");
		boolean retValue;
		
		retValue = rs_.isAfterLast();
		return retValue;
	}
	
	public boolean isBeforeFirst() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "isBeforeFirst()");
		boolean retValue;
		
		retValue = rs_.isBeforeFirst();
		return retValue;
	}
	
	public boolean isFirst() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "isFirst()");
		boolean retValue;
		
		retValue = rs_.isFirst();
		return retValue;
	}
	
	public boolean isLast() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "isLast()");
		boolean retValue;
		
		retValue = rs_.isLast();
		return retValue;
	}
	
	public boolean last() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "last()");
		boolean retValue;
		
		retValue = rs_.last();
		return retValue;
	}
	
	public void moveToCurrentRow() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "moveToCurrentRow()");
		rs_.moveToCurrentRow();
	}
	
	public void moveToInsertRow() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "moveToInsertRow()");
		rs_.moveToInsertRow();
	}
	
	public boolean next() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "next()");
		boolean retValue;
		
		retValue = rs_.next();
		return retValue;
	}
	
	public boolean previous() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "previous()");
		boolean retValue;
		
		retValue = rs_.previous();
		return retValue;
	}
	
	public void refreshRow() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "refreshRow()");
		rs_.refreshRow();
	}
	
	public boolean relative(int row) throws SQLException
	{
		boolean retValue;
		if (out_ != null)
			out_.println(getTraceId() + "relative(" + row + ")");
		
		retValue = rs_.relative(row);
		return retValue;
	}
	
	public boolean rowDeleted() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "rowDeleted()");
		boolean retValue;
		
		retValue = rs_.rowDeleted();
		return retValue;
	}
	
	public boolean rowInserted() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "rowInserted()");
		boolean retValue;
		
		retValue = rs_.rowInserted();
		return retValue;
	}
	
	public boolean rowUpdated() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "rowUpdated()");
		boolean retValue;
		
		retValue = rs_.rowUpdated();
		return retValue;
	}
	
	public void setFetchDirection(int direction) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setFetchDirection(" + direction + ")");
		rs_.setFetchDirection(direction);
	}
	
	public void setFetchSize(int rows) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setFetchSize(" + rows + ")");
		rs_.setFetchSize(rows);
	}
	
	public void updateAsciiStream(int columnIndex, InputStream x, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateAsciiStream(" + columnIndex + "," + x + "," + length + ")");
		rs_.updateAsciiStream(columnIndex, x, length);
	}
	
	public void updateAsciiStream(String columnName, InputStream x, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateAsciiStream(" + columnName + "," + x + "," + length + ")");
		rs_.updateAsciiStream(columnName, x, length);
	}
	
	public void updateBigDecimal(int columnIndex, BigDecimal x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateBigDecimal(" + columnIndex + "," + x + ")");
		rs_.updateBigDecimal(columnIndex, x);
	}
	
	public void updateBigDecimal(String columnName, BigDecimal x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateBigDecimal(" + columnName + "," + x + ")");
		rs_.updateBigDecimal(columnName, x);
	}
	
	public void updateBinaryStream(int columnIndex, InputStream x, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateBinaryStream(" + columnIndex + "," + x +
			"," + length + ")");
		rs_.updateBinaryStream(columnIndex, x, length);
	}
	
	public void updateBinaryStream(String columnName, InputStream x, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateBinaryStream(" + columnName + "," + x +
			"," + length + ")");
		rs_.updateBinaryStream(columnName, x, length);
	}
	
	public void updateBoolean(int columnIndex, boolean x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateBoolean(" + columnIndex + "," + x + ")");
		rs_.updateBoolean(columnIndex, x);
	}
	
	public void updateBoolean(String columnName, boolean x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateBoolean(" + columnName + "," + x + ")");
		rs_.updateBoolean(columnName, x);
	}
	
	public void updateByte(int columnIndex, byte x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateByte(" + columnIndex + "," + x + ")");
		rs_.updateByte(columnIndex, x);
	}
	
	public void updateByte(String columnName, byte x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateByte(" + columnName + "," + x + ")");
		rs_.updateByte(columnName, x);
	}
	
	public void updateBytes(int columnIndex, byte[] x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateBytes(" + columnIndex + "," + x + ")");
		rs_.updateBytes(columnIndex, x);
	}
	
	public void updateBytes(String columnName, byte[] x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateBytes(" + columnName + "," + x + ")");
		rs_.updateBytes(columnName, x);
	}
	
	public void updateCharacterStream(int columnIndex, Reader x, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateCharacterStream(" + columnIndex + "," + x +
			"," + length + ")");
		rs_.updateCharacterStream(columnIndex, x, length);
	}
	
	public void updateCharacterStream(String columnName, Reader x, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateCharacterStream(" + columnName + "," + x +
			"," + length + ")");
		rs_.updateCharacterStream(columnName, x, length);
	}
	
	public void updateDate(int columnIndex, java.sql.Date x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateDate(" + columnIndex + "," + x + ")");
		rs_.updateDate(columnIndex, x);
	}
	
	public void updateDate(String columnName, java.sql.Date x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateDate(" + columnName + "," + x + ")");
		rs_.updateDate(columnName, x);
	}
	
	public void updateDouble(int columnIndex, double x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateDouble(" + columnIndex + "," + x + ")");
		rs_.updateDouble(columnIndex, x);
	}
	
	public void updateDouble(String columnName, double x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateDouble(" + columnName + "," + x + ")");
		rs_.updateDouble(columnName, x);
	}
	
	public void updateFloat(int columnIndex, float x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateFloat(" + columnIndex + "," + x + ")");
		rs_.updateFloat(columnIndex, x);
	}
	
	public void updateFloat(String columnName, float x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateFloat(" + columnName + "," + x + ")");
		rs_.updateFloat(columnName, x);
	}
	
	public void updateInt(int columnIndex, int x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateInt(" + columnIndex + "," + x + ")");
		rs_.updateInt(columnIndex, x);
	}
	
	public void updateInt(String columnName, int x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateInt(" + columnName + "," + x + ")");
		rs_.updateInt(columnName, x);
	}
	
	public void updateLong(int columnIndex, long x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateLong(" + columnIndex + "," + x + ")");
		rs_.updateLong(columnIndex, x);
	}
	
	public void updateLong(String columnName, long x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateLong(" + columnName + "," + x + ")");
		rs_.updateLong(columnName, x);
	}
	
	public void updateNull(int columnIndex) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateNull(" + columnIndex + ")");
		rs_.updateNull(columnIndex);
	}
	
	public void updateNull(String columnName) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateNull(" + columnName + ")");
		rs_.updateNull(columnName);
	}
	
	public void updateObject(int columnIndex, Object x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateObject(" + columnIndex + "," + x + ")");
		rs_.updateObject(columnIndex, x);
	}
	
	public void updateObject(int columnIndex, Object x, int scale) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateObject(" + columnIndex + "," + x +
			"," + scale + ")");
		rs_.updateObject(columnIndex, x, scale);
	}
	
	public void updateObject(String columnName, Object x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateObject(" + columnName + "," + x + ")");
		rs_.updateObject(columnName, x);
	}
	
	public void updateObject(String columnName, Object x, int scale) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateObject(" + columnName + "," + x +
			"," + scale + ")");
		rs_.updateObject(columnName, x, scale);
	}
	
	public void updateRow() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateRow()");
		rs_.updateRow();
	}
	
	public void updateShort(int columnIndex, short x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateShort(" + columnIndex + "," + x + ")");
		rs_.updateShort(columnIndex, x);
	}
	
	public void updateShort(String columnName, short x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateShort(" + columnName + "," + x + ")");
		rs_.updateShort(columnName, x);
	}
	
	public void updateString(int columnIndex, String x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateString(" + columnIndex + "," + x + ")");
		rs_.updateString(columnIndex, x);
	}
	
	public void updateString(String columnName, String x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateString(" + columnName + "," + x + ")");
		rs_.updateString(columnName, x);
	}
	
	public void updateTime(int columnIndex, Time x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateTime(" + columnIndex + "," + x + ")");
		rs_.updateTime(columnIndex, x);
	}
	
	public void updateTime(String columnName, Time x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateTime(" + columnName + "," + x + ")");
		rs_.updateTime(columnName, x);
	}
	
	public void updateTimestamp(int columnIndex, Timestamp x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateTimestamp(" + columnIndex + "," + x + ")");
		rs_.updateTimestamp(columnIndex, x);
	}
	
	public void updateTimestamp(String columnName, Timestamp x) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "updateTimestamp(" + columnName + "," + x + ")");
		rs_.updateTimestamp(columnName, x);
	}
	
	public boolean wasNull() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "wasNull()");
		boolean retValue;
		
		retValue = rs_.wasNull();
		return retValue;
	}
	
	public void updateArray(String str, Array array) throws SQLException
	{
		
		if (out_ != null)
		{
			out_.println(getTraceId() + "updateArray(" + str + "," + array + ")");
		}
		rs_.updateArray(str, array);
	}
	
	public void updateArray(int param, Array array) throws SQLException
	{
		
		if (out_ != null)
		{
			out_.println(getTraceId() + "updateArray(" + param + "," + array + ")");
		}
		rs_.updateArray(param, array);
	}
	
	public void updateBlob(int param, Blob blob) throws SQLException
	{
		
		if (out_ != null)
		{
			out_.println(getTraceId() + "updateBlob(" + param + "," + blob + ")");
		}
		rs_.updateBlob(param, blob);
	}
	
	public void updateBlob(String str, Blob blob) throws SQLException
	{
		
		if (out_ != null)
		{
			out_.println(getTraceId() + "updateBlob(" + str + "," + blob + ")");
		}
		rs_.updateBlob(str, blob);
	}
	
	public void updateClob(String str, Clob clob) throws SQLException
	{
		
		if (out_ != null)
		{
			out_.println(getTraceId() + "updateClob(" + str + "," + clob + ")");
		}
		rs_.updateClob(str, clob);
	}
	
	public void updateClob(int param, Clob clob) throws SQLException
	{
		
		if (out_ != null)
		{
			out_.println(getTraceId() + "updateClob(" + param + "," + clob + ")");
		}
		rs_.updateClob(param, clob);
	}
	
	public void updateRef(int param, Ref ref) throws SQLException
	{
		if (out_ != null)
		{
			out_.println(getTraceId() + "updateRef(" + param + "," + ref + ")");
		}
		rs_.updateRef(param, ref);
	}
	
	public void updateRef(String str, Ref ref) throws SQLException
	{		
		if (out_ != null)
		{
			out_.println(getTraceId() + "updateRef(" + str + "," + ref + ")");
		}
		rs_.updateRef(str, ref);
	}
	
	// Constructors - used in SQLMXStatement
	TResultSet(ResultSet rs, PrintWriter out)
	{
		rs_ = rs;
		out_ = out;

		// Build up jdbcTrace output entry
		setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText
			+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
			+ "]:[" + Thread.currentThread() + "]:["
			+ System.identityHashCode(rs_) +  "]:"
			+ rs_.getClass().getName().substring(org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME,rs_.getClass().getName().length()) 
			+ ".");
	}
	
	// Constructors - used in SQLMXStatement
	TResultSet(ResultSet rs, TStatement ts, PrintWriter out)
	{
		
		
		rs_ = rs;
		ts_ = ts;
		out_ = out;
		
	}
	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		String className = null;
		if (rs_ != null)
			className = rs_.getClass().getName();

		// Build up jdbcTrace output entry
		setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText
			+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
			+ "]:[" + Thread.currentThread() + "]:["
			+ System.identityHashCode(rs_) +  "]:"
			+ className.substring(org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME,className.length()) 
			+ ".");
		return traceId_;
	}

	// Fields
	ResultSet		rs_;
	TStatement		ts_;
	private String			traceId_;
	PrintWriter		out_;
	
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

        public boolean isClosed() throws SQLException {
               return false;
        }
}
