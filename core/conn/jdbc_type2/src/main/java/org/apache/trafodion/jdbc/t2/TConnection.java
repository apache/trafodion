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
 * Filename    : TConnection.java
 * Description :
 *
 * --------------------------------------------------------------------
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import javax.sql.PooledConnection;

import org.apache.trafodion.jdbc.t2.SQLMXStatement;

import java.util.Map;
import java.io.PrintWriter;
import java.util.Date;
import java.util.Properties;
import java.util.concurrent.Executor;

public class TConnection implements java.sql.Connection {

	// java.sql.Connection interface methods
	public void close() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "close()");
		connection_.close();
	}

	public void clearWarnings() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "clearWarnings()");
		connection_.clearWarnings();
	}

	public void commit() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "commit()");
		connection_.commit();
	}

	public Statement createStatement() throws SQLException {
		Statement stmt;

		if (out_ != null)
			out_.println(getTraceId() + "createStatement()");

		stmt = connection_.createStatement();

		if (out_ != null)
			out_.println(getTraceId() + "createStatement() returns Statement ["
					+ ((SQLMXStatement)((TStatement)stmt).stmt_).getStmtLabel_() + "]");

		return new TStatement(stmt, out_);
	}

	public Statement createStatement(int resultSetType, int resultSetConcurrency)
			throws SQLException {
		Statement stmt = null;

		if (out_ != null)
			out_.println(getTraceId() + "createStatement(" + resultSetType + ","
					+ resultSetConcurrency + ")");

		stmt = connection_.createStatement(resultSetType, resultSetConcurrency);

		if (out_ != null)
			out_.println(getTraceId() + "createStatement(" + resultSetType + ","
					+ resultSetConcurrency + ") returns Statement ["
					+ ((SQLMXStatement)((TStatement)stmt).stmt_).getStmtLabel_()  + "]");

		return new TStatement(stmt, out_);
	}

	public Statement createStatement(int resultSetType,
			int resultSetConcurrency, int resultSetHoldability)
			throws SQLException {
		Statement stmt = null;

		if (out_ != null)
			out_.println(getTraceId() + "createStatement(" + resultSetType + ","
					+ resultSetConcurrency + "," + resultSetHoldability + ")");

		stmt = connection_.createStatement(resultSetType, resultSetConcurrency,
				resultSetHoldability);

		if (out_ != null)
			out_.println(getTraceId() + "createStatement(" + resultSetType + ","
					+ resultSetConcurrency + "," + resultSetHoldability
					+ ") returns Statement ["
					+ ((SQLMXStatement)((TStatement)stmt).stmt_).getStmtLabel_()  + "]");

		return new TStatement(stmt, out_);
	}

	public boolean getAutoCommit() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getAutoCommit()");
		boolean retValue;

		retValue = connection_.getAutoCommit();
		return retValue;
	}

	public String getCatalog() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getCatalog()");
		String retValue;

		retValue = connection_.getCatalog();
		return retValue;
	}

	public int getHoldability() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getCatalog()");
		int retValue;

		retValue = connection_.getHoldability();
		return retValue;
	}

	public DatabaseMetaData getMetaData() throws SQLException {
		DatabaseMetaData metaData;

		if (out_ != null)
			out_.println(getTraceId() + "getMetaData()");

		metaData = connection_.getMetaData();

		if (out_ != null)
			out_.println(getTraceId() + "getMetaData() returns DatabaseMetaData ["
					+ System.identityHashCode(metaData) + "]");

		return new TDatabaseMetaData(metaData, this, out_);
	}

	public int getTransactionIsolation() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getTransactionIsolation()");
		int retValue;

		retValue = connection_.getTransactionIsolation();
		return retValue;
	}

	// JDK 1.2
	public java.util.Map<String, Class<?>> getTypeMap() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getTypeMap()");
		Map<String, Class<?>> retValue;

		retValue = connection_.getTypeMap();
		return retValue;
	}

	public SQLWarning getWarnings() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "getWarnings()");
		SQLWarning retValue;

		retValue = connection_.getWarnings();
		return retValue;
	}

	public boolean isClosed() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "isClosed()");
		boolean retValue;

		retValue = connection_.isClosed();
		return retValue;
	}

	public boolean isReadOnly() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "isReadOnly()");
		boolean retValue;

		retValue = connection_.isReadOnly();
		return retValue;
	}

	public String nativeSQL(String sql) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "nativeSQL(\"" + sql + "\")");
		String retValue;

		retValue = connection_.nativeSQL(sql);
		return retValue;
	}

	public CallableStatement prepareCall(String sql) throws SQLException {
		CallableStatement stmt;

		if (out_ != null)
			out_.println(getTraceId() + "prepareCall(\"" + sql + "\")");

		stmt = connection_.prepareCall(sql);

		if (out_ != null)
			out_.println(getTraceId() + "prepareCall(\"" + sql
					+ "\") returns CallableStatement ["
					+ ((SQLMXStatement)((TCallableStatement)stmt).stmt_).getStmtLabel_() + "]");

		return new TCallableStatement(stmt, out_);
	}

	public CallableStatement prepareCall(String sql, int resultSetType,
			int resultSetConcurrency) throws SQLException {
		CallableStatement stmt;

		if (out_ != null)
			out_.println(getTraceId() + "prepareCall(\"" + sql + "\","
					+ resultSetType + "," + resultSetConcurrency + ")");

		stmt = connection_
				.prepareCall(sql, resultSetType, resultSetConcurrency);

		if (out_ != null)
			out_.println(getTraceId() + "prepareCall(\"" + sql + "\","
					+ resultSetType + "," + resultSetConcurrency
					+ ") returns CallableStatement ["
					+ ((SQLMXStatement)((TCallableStatement)stmt).stmt_).getStmtLabel_()  + "]");

		return new TCallableStatement(stmt, out_);
	}

	public CallableStatement prepareCall(String sql, int resultSetType,
			int resultConcurrency, int resultHoldability) throws SQLException {
		CallableStatement stmt;

		if (out_ != null)
			out_.println(getTraceId() + "prepareCall(\"" + sql + "\","
					+ resultSetType + "," + resultConcurrency + ","
					+ resultHoldability + ")");

		stmt = connection_.prepareCall(sql, resultSetType, resultConcurrency,
				resultHoldability);

		if (out_ != null)
			out_.println(getTraceId() + "prepareCall(\"" + sql + "\","
					+ resultSetType + "," + resultConcurrency + ","
					+ resultHoldability + ") returns CallableStatement ["
					+ ((SQLMXStatement)((TCallableStatement)stmt).stmt_).getStmtLabel_() + "]");

		return new TCallableStatement(stmt, out_);
	}

	public PreparedStatement prepareStatement(String sql) throws SQLException {
		PreparedStatement stmt;

		if (out_ != null)
			out_.println(getTraceId() + "prepareStatement(\"" + sql + "\")");

		stmt = connection_.prepareStatement(sql);

		if (out_ != null)
			out_.println(getTraceId() + "prepareStatement(\"" + sql
					+ "\") returns PreparedStatement ["
					+ ((SQLMXStatement)((TPreparedStatement)stmt).stmt_).getStmtLabel_()+ "]");

		return new TPreparedStatement(stmt, out_);
	}

	public PreparedStatement prepareStatement(String sql, int resultSetType,
			int resultSetConcurrency) throws SQLException {
		PreparedStatement stmt;

		if (out_ != null)
			out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
					+ resultSetType + "," + resultSetConcurrency + ")");

		stmt = connection_.prepareStatement(sql, resultSetType,
				resultSetConcurrency);

		if (out_ != null)
			out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
					+ resultSetType + "," + resultSetConcurrency
					+ ") returns PreparedStatement ["
					+ ((SQLMXStatement)((TPreparedStatement)stmt).stmt_).getStmtLabel_()  + "]");

		return new TPreparedStatement(stmt, out_);
	}

	public PreparedStatement prepareStatement(String sql, String[] columnNames)
			throws SQLException {
		PreparedStatement stmt;

		if (out_ != null)
			out_.println(getTraceId() + "prepareStatement(\"" + sql + "\")");

		stmt = connection_.prepareStatement(sql, columnNames);

		if (out_ != null)
			out_.println(getTraceId() + "prepareStatement(\"" + sql
					+ "\") returns PreparedStatement ["
					+ ((SQLMXStatement)((TPreparedStatement)stmt).stmt_).getStmtLabel_() + "]");

		return new TPreparedStatement(stmt, out_);
	}

	public PreparedStatement prepareStatement(String sql, int autoGeneratedKeys)
			throws SQLException {
		PreparedStatement stmt;

		if (out_ != null)
			out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
					+ autoGeneratedKeys + ")");

		stmt = connection_.prepareStatement(sql, autoGeneratedKeys);

		if (out_ != null)
			out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
					+ autoGeneratedKeys + ") returns PreparedStatement ["
					+ ((SQLMXStatement)((TPreparedStatement)stmt).stmt_).getStmtLabel_()  + "]");

		return new TPreparedStatement(stmt, out_);
	}

	public PreparedStatement prepareStatement(String sql, int[] columnIndexes)
			throws SQLException {
		PreparedStatement stmt;

		if (out_ != null)
			out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
					+ columnIndexes + ")");

		stmt = connection_.prepareStatement(sql, columnIndexes);

		if (out_ != null)
			out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
					+ columnIndexes + ") returns PreparedStatement ["
					+ ((SQLMXStatement)((TPreparedStatement)stmt).stmt_).getStmtLabel_() + "]");

		return new TPreparedStatement(stmt, out_);
	}

	public PreparedStatement prepareStatement(String sql, int resultSetType,
			int resultSetConcurrency, int resultSetHoldability)
			throws SQLException {
		PreparedStatement stmt;

		if (out_ != null)
			out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
					+ resultSetType + "," + resultSetConcurrency + ","
					+ resultSetHoldability + ")");

		stmt = connection_.prepareStatement(sql, resultSetType,
				resultSetConcurrency, resultSetHoldability);

		if (out_ != null)
			out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
					+ resultSetType + "," + resultSetConcurrency + ","
					+ resultSetHoldability + ") returns PreparedStatement ["
					+ ((SQLMXStatement)((TPreparedStatement)stmt).stmt_).getStmtLabel_() + "]");

		return new TPreparedStatement(stmt, out_);
	}

	public void releaseSavepoint(Savepoint savepoint) throws SQLException {
		if (out_ != null) {
			out_.println(getTraceId() + "releaseSavepoint (" + savepoint + ")");
		}

		connection_.releaseSavepoint(savepoint);
	}

	public void rollback() throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "rollback()");
		connection_.rollback();
	}

	public void rollback(Savepoint savepoint) throws SQLException {
		if (out_ != null) {
			out_.println(getTraceId() + "rollback (" + savepoint + ")");
		}

		connection_.rollback(savepoint);
	}

	public void setAutoCommit(boolean autoCommit) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setAutoCommit(" + autoCommit + ")");
		connection_.setAutoCommit(autoCommit);
	}

	public void setCatalog(String catalog) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setCatalog(\"" + catalog + "\")");
		connection_.setCatalog(catalog);
	}

	public void setHoldability(int holdability) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setHoldability(\"" + holdability + "\")");
		connection_.setHoldability(holdability);
	}

	public void setReadOnly(boolean readOnly) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setReadOnly(" + readOnly + ")");
		connection_.setReadOnly(readOnly);
	}

	public Savepoint setSavepoint() throws SQLException {
		Savepoint result;

		if (out_ != null) {
			out_.println(getTraceId() + "setSavepoint()");
		}

		result = connection_.setSavepoint();
		return result;
	}

	public Savepoint setSavepoint(String name) throws SQLException {
		Savepoint result;

		if (out_ != null) {
			out_.println(getTraceId() + "setSavepoint(\"" + name + "\")");
		}

		result = connection_.setSavepoint(name);
		return result;
	}

	public void setTransactionIsolation(int level) throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setTransactionIsolation(" + level + ")");
		connection_.setTransactionIsolation(level);
	}

	// JDK 1.2
	public void setTypeMap(java.util.Map<String, Class<?>> map)
			throws SQLException {
		if (out_ != null)
			out_.println(getTraceId() + "setTypeMap(" + map + ")");
		connection_.setTypeMap(map);
	}

	// Constructors with access specifier as "default"
	public TConnection(Connection connection, PrintWriter out)
			throws SQLException {
		
		connection_ = connection;
		out_ = out;
		
	}
	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		String className = null;
		if (connection_ != null)
			className = connection_.getClass().getName();

		// Build up template portion of jdbcTrace output. Pre-appended to
		// jdbcTrace entries.
		// jdbcTrace:[XXXX]:[Thread[X,X,X]]:[XXXXXXXX]:ClassName.
		setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText
				+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date())
				+ "]:["
				+ Thread.currentThread()
				+ "]:["
				+ System.identityHashCode(connection_)
				+ "]:"
				+ className.substring(
						org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME, className
								.length()) + ".");

		return traceId_;
	}
	// Fields
	Connection connection_;
	private String traceId_;
	PrintWriter out_;

	public Object unwrap(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public boolean isWrapperFor(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	public Clob createClob() throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Blob createBlob() throws SQLException {
		// TODO Auto-generated method stub
		return null;
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

	public void setClientInfo(String name, String value)
			throws SQLClientInfoException {
		// TODO Auto-generated method stub
		
	}

	public void setClientInfo(Properties properties)
			throws SQLClientInfoException {
		// TODO Auto-generated method stub
		
	}

	public String getClientInfo(String name) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Properties getClientInfo() throws SQLException {
		// TODO Auto-generated method stub
		return null;
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
		// TODO Auto-generated method stub
		
	}

	public void abort(Executor executor) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNetworkTimeout(Executor executor, int milliseconds)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public int getNetworkTimeout() throws SQLException {
		// TODO Auto-generated method stub
		return 0;
	}

        public String getSchema() throws SQLException {
		// TODO Auto-generated method stub
              return null;
        }
}
 
