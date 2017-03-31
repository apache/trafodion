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
 * Filename    : TCallableStatement.java
 * Description :
 *
 * --------------------------------------------------------------------
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.math.BigDecimal;
import java.util.Calendar;
import java.util.Map;
import java.io.PrintWriter;
import java.net.URL;
import java.io.InputStream;
import java.io.Reader;
import java.util.Date;

import org.apache.trafodion.jdbc.t2.SQLMXStatement;

public class TCallableStatement extends TPreparedStatement implements
		java.sql.CallableStatement {
	public Array getArray(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getArray(" + parameterIndex + ")");
		Array retValue;

		retValue = ((CallableStatement) stmt_).getArray(parameterIndex);
		return retValue;
	}

	public BigDecimal getBigDecimal(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getBigDecimal(" + parameterIndex + ")");
		BigDecimal retValue;

		retValue = ((CallableStatement) stmt_).getBigDecimal(parameterIndex);
		return retValue;
	}

	@Deprecated
	public BigDecimal getBigDecimal(int parameterIndex, int scale)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getBigDecimal(" + parameterIndex + ","
					+ scale + ")");
		BigDecimal retValue;

		retValue = ((CallableStatement) stmt_).getBigDecimal(parameterIndex,
				scale);
		return retValue;
	}

	public Blob getBlob(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getBlob(" + parameterIndex + ")");
		Blob retValue;

		retValue = ((CallableStatement) stmt_).getBlob(parameterIndex);
		return retValue;
	}

	public boolean getBoolean(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getBoolean(" + parameterIndex + ")");
		boolean retValue;

		retValue = ((CallableStatement) stmt_).getBoolean(parameterIndex);
		return retValue;
	}

	public byte getByte(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getByte(" + parameterIndex + ")");
		byte retValue;

		retValue = ((CallableStatement) stmt_).getByte(parameterIndex);
		return retValue;
	}

	public byte[] getBytes(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getBytes(" + parameterIndex + ")");
		byte[] retValue;

		retValue = ((CallableStatement) stmt_).getBytes(parameterIndex);
		return retValue;
	}

	public Clob getClob(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getClob(" + parameterIndex + ")");
		Clob retValue;

		retValue = ((CallableStatement) stmt_).getClob(parameterIndex);
		return retValue;
	}

	public java.sql.Date getDate(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getDate(" + parameterIndex + ")");
		java.sql.Date retValue;

		retValue = ((CallableStatement) stmt_).getDate(parameterIndex);
		return retValue;
	}

	public java.sql.Date getDate(int parameterIndex, Calendar cal)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getDate(" + parameterIndex + "," + cal
					+ ")");
		java.sql.Date retValue;

		retValue = ((CallableStatement) stmt_).getDate(parameterIndex, cal);
		return retValue;
	}

	public double getDouble(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getDouble(" + parameterIndex + ")");
		double retValue;

		retValue = ((CallableStatement) stmt_).getDouble(parameterIndex);
		return retValue;
	}

	public float getFloat(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getFloat(" + parameterIndex + ")");
		float retValue;

		retValue = ((CallableStatement) stmt_).getFloat(parameterIndex);
		return retValue;
	}

	public int getInt(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getInt(" + parameterIndex + ")");
		int retValue;

		retValue = ((CallableStatement) stmt_).getInt(parameterIndex);
		return retValue;
	}

	public long getLong(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getLong(" + parameterIndex + ")");
		long retValue;

		retValue = ((CallableStatement) stmt_).getLong(parameterIndex);
		return retValue;
	}

	public Object getObject(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getObject(" + parameterIndex + ")");
		Object retValue;

		retValue = ((CallableStatement) stmt_).getObject(parameterIndex);
		return retValue;
	}

	public Object getObject(int parameterIndex, Map<String, Class<?>> map)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getObject(" + parameterIndex + "," + map
					+ ")");
		Object retValue;

		retValue = ((CallableStatement) stmt_).getObject(parameterIndex, map);
		return retValue;
	}

	public Ref getRef(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getRef(" + parameterIndex + ")");
		Ref retValue;

		retValue = ((CallableStatement) stmt_).getRef(parameterIndex);
		return retValue;
	}

	public short getShort(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getShort(" + parameterIndex + ")");
		short retValue;

		retValue = ((CallableStatement) stmt_).getShort(parameterIndex);
		return retValue;
	}

	public String getString(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getString(" + parameterIndex + ")");
		String retValue;

		retValue = ((CallableStatement) stmt_).getString(parameterIndex);
		return retValue;
	}

	public Time getTime(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getTime(" + parameterIndex + ")");
		Time retValue;

		retValue = ((CallableStatement) stmt_).getTime(parameterIndex);
		return retValue;
	}

	public Time getTime(int parameterIndex, Calendar cal) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getTime(" + parameterIndex + "," + cal
					+ ")");
		Time retValue;

		retValue = ((CallableStatement) stmt_).getTime(parameterIndex, cal);
		return retValue;
	}

	public Timestamp getTimestamp(int parameterIndex) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getTimestamp(" + parameterIndex + ")");
		Timestamp retValue;

		retValue = ((CallableStatement) stmt_).getTimestamp(parameterIndex);
		return retValue;
	}

	public Timestamp getTimestamp(int parameterIndex, Calendar cal)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getTimestamp(" + parameterIndex + ","
					+ cal + ")");
		Timestamp retValue;

		retValue = ((CallableStatement) stmt_)
				.getTimestamp(parameterIndex, cal);
		return retValue;
	}

	public void registerOutParameter(int parameterIndex, int sqlType)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "registerOutParameter(" + parameterIndex
					+ "," + sqlType + ")");
		((CallableStatement) stmt_).registerOutParameter(parameterIndex,
				sqlType);
	}

	public void registerOutParameter(String parameterName, int sqlType)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "registerOutParameter(" + parameterName
					+ "," + sqlType + ")");
		((CallableStatement) stmt_)
				.registerOutParameter(parameterName, sqlType);
	}

	public void registerOutParameter(int parameterIndex, int sqlType, int scale)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "registerOutParameter(" + parameterIndex
					+ "," + sqlType + "," + scale + ")");
		((CallableStatement) stmt_).registerOutParameter(parameterIndex,
				sqlType, scale);
	}

	public void registerOutParameter(int parameterIndex, int sqlType,
			String typeName) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "registerOutParameter(" + parameterIndex
					+ "," + sqlType + "," + typeName + ")");
		if (out_ != null)
			out_.println(getTraceId() + "getArray(" + parameterIndex + ")");
		((CallableStatement) stmt_).registerOutParameter(parameterIndex,
				sqlType, typeName);
	}

	public boolean wasNull() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "wasNull()");
		boolean retValue;

		retValue = ((CallableStatement) stmt_).wasNull();
		return retValue;
	}

	public boolean execute() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "execute()");
		}

		retValue = ((CallableStatement) stmt_).execute();
		return retValue;
	}

	public int[] executeBatch() throws SQLException {
		int[] result;

		if (out_ != null) {
			out_.println(getTraceId() + "executeBatch()");
		}

		result = ((CallableStatement) stmt_).executeBatch();
		return result;
	}

	public ResultSet executeQuery() throws SQLException {
		ResultSet result;

		if (out_ != null) {
			out_.println(getTraceId() + "executeQuery()");
		}

		result = ((CallableStatement) stmt_).executeQuery();

		if (out_ != null) {
			out_.println(getTraceId() + "executeQuery() returns ResultSet ["
					+ System.identityHashCode(result) + "]");
		}

		return result;
	}

	public int executeUpdate() throws SQLException {
		int result;

		if (out_ != null) {
			out_.println(getTraceId() + "executeUpdate()");
		}

		result = ((CallableStatement) stmt_).executeUpdate();
		return 1;
	}

	public Array getArray(String str) throws SQLException {
		Array result;

		if (out_ != null) {
			out_.println(getTraceId() + "getArray(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getArray(str);
		return result;
	}

	public BigDecimal getBigDecimal(String str) throws SQLException {
		BigDecimal result;

		if (out_ != null) {
			out_.println(getTraceId() + "getBigDecimal(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getBigDecimal(str);
		return result;
	}

	public Blob getBlob(String str) throws SQLException {
		Blob result;

		if (out_ != null) {
			out_.println(getTraceId() + "getBlob(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getBlob(str);
		return result;
	}

	public boolean getBoolean(String str) throws SQLException {
		boolean result;

		if (out_ != null) {
			out_.println(getTraceId() + "getBoolean(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getBoolean(str);
		return true;
	}

	public byte getByte(String str) throws SQLException {
		byte result;

		if (out_ != null) {
			out_.println(getTraceId() + "getByte(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getByte(str);
		return result;
	}

	public byte[] getBytes(String str) throws SQLException {
		byte[] result;

		if (out_ != null) {
			out_.println(getTraceId() + "getBytes(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getBytes(str);
		return result;
	}

	public Clob getClob(String str) throws SQLException {
		Clob result;

		if (out_ != null) {
			out_.println(getTraceId() + "getClob(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getClob(str);
		return result;
	}

	public java.sql.Date getDate(String str) throws SQLException {
		java.sql.Date result;

		if (out_ != null) {
			out_.println(getTraceId() + "getDate(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getDate(str);
		return result;
	}

	public java.sql.Date getDate(String str, Calendar calendar)
			throws SQLException {
		java.sql.Date result;

		if (out_ != null) {
			out_.println(getTraceId() + "getDate(" + str + ", " + calendar + ")");
		}

		result = ((CallableStatement) stmt_).getDate(str, calendar);
		return result;
	}

	public double getDouble(String str) throws SQLException {
		double result;

		if (out_ != null) {
			out_.println(getTraceId() + "getDouble(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getDouble(str);
		return result;
	}

	public float getFloat(String str) throws SQLException {
		float result;

		if (out_ != null) {
			out_.println(getTraceId() + "getFloat(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getFloat(str);
		return result;
	}

	public int getInt(String str) throws SQLException {
		int result;

		if (out_ != null) {
			out_.println(getTraceId() + "getInt(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getInt(str);
		return result;
	}

	public long getLong(String str) throws SQLException {
		long result;

		if (out_ != null) {
			out_.println(getTraceId() + "getLong(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getLong(str);
		return result;
	}

	public Object getObject(String str) throws SQLException {
		Object result;

		if (out_ != null) {
			out_.println(getTraceId() + "getObject(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getObject(str);
		return result;
	}

	public Object getObject(String str, Map<String, Class<?>> map)
			throws SQLException {
		Object result;

		if (out_ != null) {
			out_.println(getTraceId() + "getObject(" + str + ", " + map + ")");
		}

		result = ((CallableStatement) stmt_).getObject(str, map);
		return result;
	}

	public Ref getRef(String str) throws SQLException {
		Ref result;

		if (out_ != null) {
			out_.println(getTraceId() + "getRef(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getRef(str);
		return result;
	}

	public short getShort(String str) throws SQLException {
		short result;

		if (out_ != null) {
			out_.println(getTraceId() + "getShort(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getShort(str);
		return result;
	}

	public String getString(String str) throws SQLException {
		String result;

		if (out_ != null) {
			out_.println(getTraceId() + "getString(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getString(str);
		return result;
	}

	public Time getTime(String str) throws SQLException {
		Time result;

		if (out_ != null) {
			out_.println(getTraceId() + "getTime(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getTime(str);
		return result;
	}

	public Time getTime(String str, Calendar calendar) throws SQLException {
		Time result;

		if (out_ != null) {
			out_.println(getTraceId() + "getTime(" + str + calendar + ")");
		}

		result = ((CallableStatement) stmt_).getTime(str, calendar);
		return result;
	}

	public Timestamp getTimestamp(String str) throws SQLException {
		Timestamp result;

		if (out_ != null) {
			out_.println(getTraceId() + "getTimestamp(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getTimestamp(str);
		return result;
	}

	public Timestamp getTimestamp(String str, Calendar calendar)
			throws SQLException {
		Timestamp result;

		if (out_ != null) {
			out_.println(getTraceId() + "getTimestamp(" + str + ", " + calendar
					+ ")");
		}

		result = ((CallableStatement) stmt_).getTimestamp(str, calendar);
		return result;
	}

	public URL getURL(int param) throws SQLException {
		URL result;

		if (out_ != null) {
			out_.println(getTraceId() + "getURL(" + param + ")");
		}

		result = ((CallableStatement) stmt_).getURL(param);
		return result;
	}

	public URL getURL(String str) throws SQLException {
		URL result;

		if (out_ != null) {
			out_.println(getTraceId() + "getURL(" + str + ")");
		}

		result = ((CallableStatement) stmt_).getURL(str);
		return result;
	}

	public void registerOutParameter(String str, int param, String str2)
			throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "registerOutParameter(" + str + ", "
					+ param + ", " + str2 + ")");
		}

		((CallableStatement) stmt_).registerOutParameter(str, param, str2);
	}

	public void registerOutParameter(String str, int param, int param2)
			throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "registerOutParameter(" + str + ", "
					+ param + ", " + param2 + ")");
		}

		((CallableStatement) stmt_).registerOutParameter(str, param, param2);
	}

	public void setAsciiStream(String str, InputStream inputStream, int param)
			throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setAsciiStream(" + str + ", "
					+ inputStream + ", " + param + ")");
		}

		((CallableStatement) stmt_).setAsciiStream(str, inputStream, param);
	}

	public void setBigDecimal(String str, BigDecimal bigDecimal)
			throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setBigDeciaml(" + str + ", " + bigDecimal
					+ ")");
		}

		((CallableStatement) stmt_).setBigDecimal(str, bigDecimal);
	}

	public void setBigDecimal(int param, BigDecimal bigDecimal)
			throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setBigDecimal(" + param + ", "
					+ bigDecimal + ")");
		}

		((CallableStatement) stmt_).setBigDecimal(param, bigDecimal);
	}

	public void setBinaryStream(String str, InputStream inputStream, int param)
			throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setBinaryStream(" + str + ", "
					+ inputStream + ", " + param + ")");
		}

		((CallableStatement) stmt_).setBinaryStream(str, inputStream, param);
	}

	public void setBoolean(String str, boolean param) throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setBoolean(" + str + ", " + param + ")");
		}

		((CallableStatement) stmt_).setBoolean(str, param);
	}

	public void setByte(String str, byte param) throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setByte(" + str + ", " + param + ")");
		}

		((CallableStatement) stmt_).setByte(str, param);
	}

	public void setBytes(String str, byte[] values) throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setBytes(" + str + ", " + values + ")");
		}

		((CallableStatement) stmt_).setBytes(str, values);
	}

	public void setCharacterStream(String str, Reader reader, int param)
			throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setCharacterStream(" + str + ", " + reader
					+ ", " + param + ")");
		}

		((CallableStatement) stmt_).setCharacterStream(str, reader, param);
	}

	public void setDate(String str, java.sql.Date date) throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setDate(" + str + ", " + date + ")");
		}

		((CallableStatement) stmt_).setDate(str, date);
	}

	public void setDate(String str, java.sql.Date date, Calendar calendar)
			throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setDate(" + str + ", " + date + ", "
					+ calendar + ")");
		}

		((CallableStatement) stmt_).setDate(str, date, calendar);
	}

	public void setDouble(String str, double param) throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setDouble(" + str + ", " + param + ")");
		}

		((CallableStatement) stmt_).setDouble(str, param);
	}

	public void setFloat(String str, float param) throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setFloat(" + str + ", " + param + ")");
		}

		((CallableStatement) stmt_).setFloat(str, param);
	}

	public void setInt(String str, int param) throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setInt(" + str + ", " + param + ")");
		}

		((CallableStatement) stmt_).setInt(str, param);
	}

	public void setLong(String str, long param) throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setLong(" + str + ", " + param + ")");
		}

		((CallableStatement) stmt_).setLong(str, param);
	}

	public void setNull(String str, int param) throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setNull(" + str + ", " + param + ")");
		}

		((CallableStatement) stmt_).setNull(str, param);
	}

	public void setNull(String str, int param, String str2) throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setNull(" + str + ", " + param + ", "
					+ str2 + ")");
		}

		((CallableStatement) stmt_).setNull(str, param, str2);
	}

	public void setObject(String str, Object obj) throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setObject(" + str + ", " + obj + ")");
		}

		((CallableStatement) stmt_).setObject(str, obj);
	}

	public void setObject(String str, Object obj, int param)
			throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setObject(" + str + ", " + obj + ", "
					+ param + ")");
		}

		((CallableStatement) stmt_).setObject(str, obj, param);
	}

	public void setObject(String str, Object obj, int param, int param3)
			throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setObject(" + str + ", " + obj + ", "
					+ param + ", " + param3 + ")");
		}

		((CallableStatement) stmt_).setObject(str, obj, param, param3);
	}

	public void setShort(String str, short param) throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setShort(" + str + ", " + param + ")");
		}

		((CallableStatement) stmt_).setShort(str, param);
	}

	public void setString(String str, String str1) throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setString(" + str + ", " + str1 + ")");
		}

		((CallableStatement) stmt_).setString(str, str1);
	}

	public void setTime(String str, Time time) throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setTime(" + str + ", " + time + ")");
		}

		((CallableStatement) stmt_).setTime(str, time);
	}

	public void setTime(String str, Time time, Calendar calendar)
			throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setTime(" + str + ", " + time + ", "
					+ calendar + ")");
		}

		((CallableStatement) stmt_).setTime(str, time, calendar);
	}

	public void setTimestamp(String str, Timestamp timestamp)
			throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setTimestamp(" + str + ", " + timestamp
					+ ")");
		}

		((CallableStatement) stmt_).setTimestamp(str, timestamp);
	}

	public void setTimestamp(String str, Timestamp timestamp, Calendar calendar)
			throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setTimestamp(" + str + ", " + timestamp
					+ ", " + calendar + ")");
		}

		((CallableStatement) stmt_).setTimestamp(str, timestamp, calendar);
	}

	public void setURL(String str, URL uRL) throws SQLException {

		if (out_ != null) {
			out_.println(getTraceId() + "setURL(" + str + ", " + uRL + ")");
		}

		((CallableStatement) stmt_).setURL(str, uRL);
	}

	// Constructors with access specifier as "default"
	public TCallableStatement(CallableStatement stmt, PrintWriter out) {
	super(stmt,out);
	}


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
