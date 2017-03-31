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
 * Filename    : TResultSetMetaData.java
 * Description :
 *
 * --------------------------------------------------------------------
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.PrintWriter;
import java.util.Date;

public class TResultSetMetaData implements java.sql.ResultSetMetaData
{

	public String getCatalogName(int column) throws SQLException
	{
	    if (out_ != null)
			out_.println(getTraceId() + "getCatalogName(" + column + ")");
		String retValue;

		retValue = rsMD_.getCatalogName(column);
		return retValue;
	}
	
 	public String getColumnClassName(int column) throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "getColumnClassName(" + column + ")");
		String retValue;

		retValue = rsMD_.getColumnClassName(column);
		return retValue;
	}
	
 	public int getColumnCount() throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "getColumnCount()");
		int retValue;

		retValue = rsMD_.getColumnCount();
		return retValue;
 	}
 	
 	public int getColumnDisplaySize(int column) throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "getColumnDisplaySize(" + column + ")");
		int retValue;

		retValue = rsMD_.getColumnDisplaySize(column);
		return retValue;
 	}
 	
 	public String getColumnLabel(int column) throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "getColumnLabel(" + column + ")");
		String retValue;

		retValue = rsMD_.getColumnLabel(column);
		return retValue;
	}
	
 	public String getColumnName(int column) throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "getColumnName(" + column + ")");
		String retValue;

		retValue = rsMD_.getColumnName(column);
		return retValue;
	}
	
 	public int getColumnType(int column) throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "getColumnType(" + column + ")");
		int retValue;

		retValue = rsMD_.getColumnType(column);
		return retValue;
 	}
 	
 	public String getColumnTypeName(int column) throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "getColumnTypeName(" + column + ")");
		String retValue;

		retValue = rsMD_.getColumnTypeName(column);
		return retValue;
	}
	
 	public int getPrecision(int column) throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "getPrecision(" + column + ")");
		int retValue;

		retValue = rsMD_.getPrecision(column);
		return retValue;
 	}
 	
 	public int getScale(int column) throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "getScale(" + column + ")");
		int retValue;

		retValue = rsMD_.getScale(column);
		return retValue;
 	}
 	
 	public String getSchemaName(int column) throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "getSchemaName(" + column + ")");
		String retValue;

		retValue = rsMD_.getSchemaName(column);
		return retValue;
 	}
	
 	public String getTableName(int column) throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "getTableName(" + column + ")");
		String retValue;

		retValue = rsMD_.getTableName(column);
		return retValue;
 	}
	
 	public boolean isAutoIncrement(int column) throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "isAutoIncrement(" + column + ")");
		boolean retValue;

		retValue = rsMD_.isAutoIncrement(column);
		return retValue;
 	}
 	
 	public boolean isCaseSensitive(int column) throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "isCaseSensitive(" + column + ")");
		boolean retValue;

		retValue = rsMD_.isCaseSensitive(column);
		return retValue;
 	}
 	
 	public boolean isCurrency(int column) throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "isCurrency(" + column + ")");
		boolean retValue;

		retValue = rsMD_.isCurrency(column);
		return retValue;
	}
 	
 	public boolean isDefinitelyWritable(int column) throws SQLException 
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "isDefinitelyWritable(" + column + ")");
		boolean retValue;

		retValue = rsMD_.isDefinitelyWritable(column);
		return retValue;
 	}
 	
 	public int isNullable(int column) throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "isNullable(" + column + ")");
		int retValue;

		retValue = rsMD_.isNullable(column);
		return retValue;
 	}
 	
 	public boolean isReadOnly(int column) throws SQLException 
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "isReadOnly(" + column + ")");
		boolean retValue;

		retValue = rsMD_.isReadOnly(column);
		return retValue;
 	}
 	
 	public boolean isSearchable(int column) throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "isSearchable(" + column + ")");
		boolean retValue;

		retValue = rsMD_.isSearchable(column);
		return retValue;
 	}
 	
 	public boolean isSigned(int column) throws SQLException
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "isSigned(" + column + ")");
		boolean retValue;

		retValue = rsMD_.isSigned(column);
		return retValue;
 	}
 	
 	public boolean isWritable(int column) throws SQLException 
 	{
	    if (out_ != null)
			out_.println(getTraceId() + "isWritable(" + column + ")");
		boolean retValue;

		retValue = rsMD_.isWritable(column);
		return retValue;
 	}

 	// Constructors
    TResultSetMetaData(ResultSetMetaData rsMD, PrintWriter out) throws SQLException
	{
		
		rsMD_ = rsMD;
		out_ = out;
		
	}
	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		String className = null;
		if (rsMD_ != null)
			className = rsMD_.getClass().getName();

		// Build up jdbcTrace output entry
		setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText
			+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
			+ "]:[" + Thread.currentThread() + "]:["
			+ System.identityHashCode(rsMD_) +  "]:" 
			+ className.substring(org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME,className.length()) 
			+ ".");

		return traceId_;
	}
	// Fields
	ResultSetMetaData	rsMD_;
	private String				traceId_;
	PrintWriter			out_;

	public Object unwrap(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public boolean isWrapperFor(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}
    public SQLMXResultSetMetaData getSqlResultSetMetaData(){
        return (SQLMXResultSetMetaData)rsMD_;
    }

}
