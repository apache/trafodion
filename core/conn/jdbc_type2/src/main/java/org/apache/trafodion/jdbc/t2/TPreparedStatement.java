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
 * Filename    : TPreparedStatement.java
 * Description :
 *
 */
package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.InputStream;
import java.io.Reader;
import java.io.PrintWriter;
import java.util.Calendar;
import java.math.BigDecimal;
import java.net.URL;
import java.util.Date;

import org.apache.trafodion.jdbc.t2.SQLMXResultSet;
import org.apache.trafodion.jdbc.t2.SQLMXStatement;

public class TPreparedStatement extends TStatement implements
		java.sql.PreparedStatement {
	// java.sql.PreparedStatement interface methods
	public void addBatch() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "addBatch()");
		((PreparedStatement) stmt_).addBatch();
	}

	public void clearParameters() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "clearParameters()");
		((PreparedStatement) stmt_).clearParameters();
	}

	public boolean execute() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "execute()");
		boolean retValue;

		retValue = ((PreparedStatement) stmt_).execute();
		return retValue;
	}

	public int[] executeBatch() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "executeBatch()");
		int[] retValue;

		retValue = ((PreparedStatement) stmt_).executeBatch();
		return retValue;
	}

	public ResultSet executeQuery() throws SQLException {
		ResultSet rs;

		if (out_ != null)
			out_.println(getTraceId() + "executeQuery()");

		rs = ((PreparedStatement) stmt_).executeQuery();

		if (out_ != null)
			out_.println(getTraceId() + "executeQuery() returns ResultSet ["
					+ System.identityHashCode(rs) + "]");

		if(rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
			rs_ = rs;
		} else {
			rs_ = new TResultSet(rs, this, out_);
		}
		return rs_;
	}

	public int executeUpdate() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "executeUpdate()");
		int retValue;

		retValue = ((PreparedStatement) stmt_).executeUpdate();
		return retValue;
	}

	public ResultSetMetaData getMetaData() throws SQLException {
		ResultSetMetaData rsMD;

		if (out_ != null)
			out_.println(getTraceId() + "getMetaData()");

		rsMD = ((PreparedStatement) stmt_).getMetaData();

		if (out_ != null)
			out_.println(getTraceId() + "getMetaData() returns ResultSetMetaData ["
					+ System.identityHashCode(rsMD) + "]");

		return new TResultSetMetaData(rsMD, out_);
	}

	// JDK 1.2
	public void setArray(int parameterIndex, Array x) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setArray(" + parameterIndex + "," + x
					+ ")");
		((PreparedStatement) stmt_).setArray(parameterIndex, x);
	}

	public void setAsciiStream(int parameterIndex, InputStream x, int length)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setAsciiStream(" + parameterIndex + ","
					+ x + "," + length + ")");
		((PreparedStatement) stmt_).setAsciiStream(parameterIndex, x, length);
	}

	public void setBigDecimal(int parameterIndex, BigDecimal x)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setBigDecimal(" + parameterIndex + "," + x
					+ ")");
		((PreparedStatement) stmt_).setBigDecimal(parameterIndex, x);
	}

	public void setBinaryStream(int parameterIndex, InputStream x, int length)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setBinaryStream(" + parameterIndex + ","
					+ x + "," + length + ")");
		((PreparedStatement) stmt_).setBinaryStream(parameterIndex, x, length);
	}

	// JDK 1.2
	public void setBlob(int parameterIndex, Blob x) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setBlob(" + parameterIndex + "," + x
							+ ")");
		((PreparedStatement) stmt_).setBlob(parameterIndex, x);
	}

	public void setBoolean(int parameterIndex, boolean x) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setBoolean(" + parameterIndex + "," + x
					+ ")");
		((PreparedStatement) stmt_).setBoolean(parameterIndex, x);
	}

	public void setByte(int parameterIndex, byte x) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setByte(" + parameterIndex + "," + x
							+ ")");
		((PreparedStatement) stmt_).setByte(parameterIndex, x);
	}

	public void setBytes(int parameterIndex, byte[] x) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setBytes(" + parameterIndex + "," + x
					+ ")");
		((PreparedStatement) stmt_).setBytes(parameterIndex, x);
	}

	public void setCharacterStream(int parameterIndex, Reader x, int length)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setCharacterStream(" + parameterIndex
					+ "," + x + "," + length + ")");
		((PreparedStatement) stmt_).setCharacterStream(parameterIndex, x,
				length);
	}

	// JDk 1.2
	public void setClob(int parameterIndex, Clob x) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setClob(" + parameterIndex + "," + x
							+ ")");
		((PreparedStatement) stmt_).setClob(parameterIndex, x);
	}

	public void setDate(int parameterIndex, java.sql.Date x)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setDate(" + parameterIndex + "," + x
							+ ")");
		((PreparedStatement) stmt_).setDate(parameterIndex, x);
	}

	public void setDate(int parameterIndex, java.sql.Date x, Calendar cal)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setDate(" + parameterIndex + "," + x + ","
					+ cal + ")");
		((PreparedStatement) stmt_).setDate(parameterIndex, x, cal);
	}

	public void setDouble(int parameterIndex, double x) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setDouble(" + parameterIndex + "," + x
					+ ")");
		((PreparedStatement) stmt_).setDouble(parameterIndex, x);
	}

	public void setFloat(int parameterIndex, float x) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setFloat(" + parameterIndex + "," + x
					+ ")");
		((PreparedStatement) stmt_).setFloat(parameterIndex, x);
	}

	public void setInt(int parameterIndex, int x) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setInt(" + parameterIndex + "," + x + ")");
		((PreparedStatement) stmt_).setInt(parameterIndex, x);
	}

	public void setLong(int parameterIndex, long x) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setLong(" + parameterIndex + "," + x
							+ ")");
		((PreparedStatement) stmt_).setLong(parameterIndex, x);
	}

	public void setNull(int parameterIndex, int sqlType) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setNull(" + parameterIndex + "," + sqlType
					+ ")");
		((PreparedStatement) stmt_).setNull(parameterIndex, sqlType);
	}

	public void setNull(int parameterIndex, int sqlType, String typeName)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setNull(" + parameterIndex + "," + sqlType
					+ "," + typeName + ")");
		((PreparedStatement) stmt_).setNull(parameterIndex, sqlType, typeName);
	}

	public void setObject(int parameterIndex, Object x) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setObject(" + parameterIndex + "," + x
					+ ")");
		((PreparedStatement) stmt_).setObject(parameterIndex, x);
	}

	public void setObject(int parameterIndex, Object x, int targetSqlType)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setObject(" + parameterIndex + "," + x
					+ "," + targetSqlType + ")");
		((PreparedStatement) stmt_).setObject(parameterIndex, x, targetSqlType);
	}

	public void setObject(int parameterIndex, Object x, int targetSqlType,
			int scale) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setObject(" + parameterIndex + "," + x
					+ "," + targetSqlType + "," + scale + ")");
		((PreparedStatement) stmt_).setObject(parameterIndex, x, targetSqlType,
				scale);
	}

	// JDK 1.2
	public void setRef(int parameterIndex, Ref x) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setRef(" + parameterIndex + "," + x + ")");
		((PreparedStatement) stmt_).setRef(parameterIndex, x);
	}

	public void setShort(int parameterIndex, short x) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setShort(" + parameterIndex + "," + x
					+ ")");
		((PreparedStatement) stmt_).setShort(parameterIndex, x);
	}

	public void setString(int parameterIndex, String x) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setString(" + parameterIndex + "," + x
					+ ")");
		((PreparedStatement) stmt_).setString(parameterIndex, x);
	}

	public void setTime(int parameterIndex, Time x) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setTime(" + parameterIndex + "," + x
							+ ")");
		((PreparedStatement) stmt_).setTime(parameterIndex, x);
	}

	public void setTime(int parameterIndex, Time x, Calendar cal)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setTime(" + parameterIndex + "," + x + ","
					+ cal + ")");
		((PreparedStatement) stmt_).setTime(parameterIndex, x, cal);
	}

	@Deprecated
	public void setUnicodeStream(int parameterIndex, InputStream x, int length)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setUnicodeStream(" + parameterIndex + ","
					+ x + "," + length + ")");
		((PreparedStatement) stmt_).setUnicodeStream(parameterIndex, x, length);
	}

	public void setTimestamp(int parameterIndex, Timestamp x)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setTimestamp(" + parameterIndex + "," + x
					+ ")");
		((PreparedStatement) stmt_).setTimestamp(parameterIndex, x);
	}

	public void setTimestamp(int parameterIndex, Timestamp x, Calendar cal)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setTimestamp(" + parameterIndex + "," + x
					+ "," + cal + ")");
		((PreparedStatement) stmt_).setTimestamp(parameterIndex, x, cal);
	}

	public void setURL(int parameterIndex, URL x) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setURL()");
		((PreparedStatement) stmt_).setURL(parameterIndex, x);
	}

	public ParameterMetaData getParameterMetaData() throws SQLException {
		ParameterMetaData pmd;

		if (out_ != null)
			out_.println(getTraceId() + "getParameterMetaData()");

		if (stmt_ instanceof org.apache.trafodion.jdbc.t2.SQLMXPreparedStatement)
			pmd = ((org.apache.trafodion.jdbc.t2.SQLMXPreparedStatement) stmt_)
					.getParameterMetaData();
		else
			pmd = null;

		if (out_ != null)
			out_.println(getTraceId()
					+ "getParameterMetaData() returns ParameterMetaData ["
					+ System.identityHashCode(pmd) + "]");

		return new TParameterMetaData(pmd, out_);
	}

	// Constructors with access specifier as "default"
	public TPreparedStatement(PreparedStatement stmt, PrintWriter out) {
		super(stmt,out);
	}

	TPreparedStatement() {
	}

	public boolean isClosed() throws SQLException {
		// TODO Auto-generated method stub
		return false;
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
		// TODO Auto-generated method stub
		
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
		// TODO Auto-generated method stub
		
	}

	public void setBlob(int parameterIndex, InputStream inputStream, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
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
		// TODO Auto-generated method stub
		
	}

	public void setCharacterStream(int parameterIndex, Reader reader,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setAsciiStream(int parameterIndex, InputStream x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setBinaryStream(int parameterIndex, InputStream x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setCharacterStream(int parameterIndex, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNCharacterStream(int parameterIndex, Reader value)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setClob(int parameterIndex, Reader reader) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setBlob(int parameterIndex, InputStream inputStream)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNClob(int parameterIndex, Reader reader) throws SQLException {
		// TODO Auto-generated method stub
		
	}
	
	public SQLMXPreparedStatement getSqlPreparedStatement(){
		return (SQLMXPreparedStatement)stmt_;
	}
}
