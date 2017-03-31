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
 * Filename    : TDataSource.java
 * Description :
 *
 * --------------------------------------------------------------------
 */


package org.apache.trafodion.jdbc.t2;

import java.io.PrintWriter;
import java.sql.*;
import javax.sql.DataSource;
import java.util.Properties;
import javax.naming.Referenceable;
import javax.naming.NamingException;
import javax.naming.Reference;
import javax.naming.StringRefAddr;
import java.util.Date;
import java.util.logging.Logger;


public class TDataSource implements javax.sql.DataSource, java.io.Serializable, Referenceable

{
	// javax.sql.DataSource interface methods
	public Connection getConnection() throws SQLException
	{		
		Connection	connect;

		if (ds_ != null)
		{
			if (out_ != null)
				out_.println(getTraceId() + "getConnection()");	

			connect = ds_.getConnection();

			if (out_ != null)
				out_.println(getTraceId() + "getConnection() returns Connection [" + System.identityHashCode(connect) + "]");	

			return new TConnection(connect, out_);
		}
		else
			throw new SQLException("Trace Data Source" + traceDataSource_ + "Not found");
	}
	
	public Connection getConnection(String username, String Password) throws SQLException
	{
		Connection	connect;
		if (ds_ != null)
		{
			if (out_ != null)
				out_.println(getTraceId() + "getConnection(\"" + username + "\", \"****\")");			

			connect = ds_.getConnection();

			if (out_ != null)
				out_.println(getTraceId() + "getConnection(\"" + username + "\", \"****\") returns Connection [" + System.identityHashCode(connect) + "]");			

			return new TConnection(connect, out_);
		}
		else
			throw new SQLException("Trace Data Source" + traceDataSource_ + "Not found");
	}

	public int getLoginTimeout() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getLoginTimeout()");			
		int	retValue;

		if (ds_ != null)
		{
			retValue = ds_.getLoginTimeout();
			return retValue;
		}
		else
			throw new SQLException("Trace Data Source" + traceDataSource_ + "Not found");
	}
	
	public PrintWriter getLogWriter() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getLogWriter()");			
		PrintWriter	retValue;

		if (ds_ != null)
		{
			retValue = ds_.getLogWriter();
			return retValue;
		}
		else
			throw new SQLException("Trace Data Source" + traceDataSource_ + "Not found");
	}

	public void setLoginTimeout(int seconds) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setLoginTimeout("+ seconds + ")");			
		if (ds_ != null)
			ds_.setLoginTimeout(seconds);
		else
			throw new SQLException("Trace Data Source" + traceDataSource_ + "Not found");
	}

	public void setLogWriter(PrintWriter out) throws SQLException
	{
		out_ = out;
		if (out_ != null) 
		{
			out_.println(org.apache.trafodion.jdbc.t2.T2Driver.printTraceVproc);
			out_.println(getTraceId() + "setLogWriter("+ out + ")");			
		}

		if (ds_ != null)
			ds_.setLogWriter(out);
		else
			throw new SQLException("Trace Data Source" + traceDataSource_ + "Not found");
	}
	
	public Reference getReference() throws NamingException
	{
	
		if (out_ != null)
			out_.println(getTraceId() + "getReference()");			
		Reference ref;
		
		ref = new Reference(this.getClass().getName(), "org.apache.trafodion.jdbc.t2.TDataSourceFactory", null);
		ref.add(new StringRefAddr("description", description_));
		ref.add(new StringRefAddr("traceDataSource", traceDataSource_));
		return ref;

	}

	// Get-Set Property methods
	public void setDescription(String description)
	{
		description_ = description;
	}

	public String getDescription()
	{
		return description_;
	}

	public void setTraceDataSource(String traceDataSource)
	{
		traceDataSource_ = traceDataSource;
	}
	
	public String getTraceDataSource()
	{
		return traceDataSource_;
	}
	
	TDataSource(String traceDataSource, DataSource ds)
	{
		String className = null;

		traceDataSource_ = traceDataSource;
		ds_	= ds;
		if (ds_ != null)
			className = ds_.getClass().getName();

		// Build up jdbcTrace output entry
		setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText
			+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
			+ "]:[" + Thread.currentThread() + "]:["
			+ System.identityHashCode(ds_) +  "]:" 
			+ className.substring(org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME,className.length()) 
			+ ".");
	}

	public TDataSource()
	{
		
	}
	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		// Build up jdbcTrace output entry
		setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText
			+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
			+ "]:[" + Thread.currentThread() + "]:["
			+ System.identityHashCode(this) +  "]:" 
			+ getClass().getName().substring(org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME,getClass().getName().length()) 
			+ ".");
		return traceId_;
	}
	// fields
	PrintWriter	out_;
	String		traceDataSource_;
	DataSource	ds_;
	private String		traceId_;
	String		description_;

	// serialVersionUID set to resolve JDK5.0 javax -Xlint:serial definition warning.
	private static final long serialVersionUID = 2L;


	public Logger getParentLogger() throws SQLFeatureNotSupportedException {
		// TODO Auto-generated method stub
		return null;
	}

	public Object unwrap(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public boolean isWrapperFor(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}
}
