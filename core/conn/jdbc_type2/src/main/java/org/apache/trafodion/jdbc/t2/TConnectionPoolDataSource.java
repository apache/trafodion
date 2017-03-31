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
 * Filename    : TConnectionPoolDataSource.java
 * Description :
 *
 * --------------------------------------------------------------------
 */

package org.apache.trafodion.jdbc.t2;

import java.io.PrintWriter;
import java.sql.*;
import javax.sql.*;
import java.util.Properties;
import javax.naming.Referenceable;
import javax.naming.NamingException;
import javax.naming.Reference;
import java.util.Date;
import javax.naming.StringRefAddr;
import java.util.logging.Logger;



public class TConnectionPoolDataSource implements javax.sql.ConnectionPoolDataSource, java.io.Serializable, Referenceable

{
	// javax.sql.ConnectionPoolDataSource interface methods
	public PooledConnection getPooledConnection() throws SQLException
	{	
		PooledConnection pc;
		if (cpDS_ != null)
		{
			if (out_ != null)
				out_.println(getTraceId() + "getPooledConnection()");

			pc = cpDS_.getPooledConnection();

			if (out_ != null)
				out_.println(getTraceId() + "getPooledConnection() returns PooledConnection [" + System.identityHashCode(pc) + "]");

			return new TPooledConnection(pc, out_);
		}
		else
			throw new SQLException("Trace Data Source" + traceDataSource_ + "Not found");
	}
	
	public PooledConnection getPooledConnection(String username, String password) throws SQLException
	{
		PooledConnection pc;
		if (cpDS_ != null)
		{
			if (out_ != null)
				out_.println(getTraceId() + "getPooledConnection(\"" + username + "\", \"****\")");			

			pc = cpDS_.getPooledConnection(username, password);

			if (out_ != null)
				out_.println(getTraceId() + "getPooledConnection(\"" + username 
					+ "\", \"****\") returns PooledConnection [" + System.identityHashCode(pc) + "]");			

			return new TPooledConnection(pc, out_);
		}
		else
			throw new SQLException("Trace Data Source" + traceDataSource_ + "Not found");
	}

	public int getLoginTimeout() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getLoginTimeout()");			
		int	retValue;

		if (cpDS_ != null)
		{
			retValue = cpDS_.getLoginTimeout();
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

		if (cpDS_ != null)
		{
			retValue = cpDS_.getLogWriter();
			return retValue;
		}
		else
			throw new SQLException("Trace Data Source" + traceDataSource_ + "Not found");
	}

	public void setLoginTimeout(int seconds) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setLoginTimeout("+ seconds + ")");			
		if (cpDS_ != null)
			cpDS_.setLoginTimeout(seconds);
		else
			throw new SQLException("Trace Data Source" + traceDataSource_ + "Not found");
	}

	public void setLogWriter(PrintWriter out) throws SQLException
	{
		out_ = out;
		if (out_ != null)
			out_.println(getTraceId() + "setLogWriter("+ out + ")");			
		if (cpDS_ != null)
			cpDS_.setLogWriter(out);
		else
			throw new SQLException("Trace Data Source" + traceDataSource_ + "Not found");
	}
	
	public Reference getReference() throws NamingException
	{
	
		if (out_ != null)
			out_.println(getTraceId() + "getReference()");			
		Reference ref;
		
		ref = new Reference(this.getClass().getName(), "org.apache.trafodion.jdbc.t2.TConnectionPoolDataSourceFactory", null);
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
	
	TConnectionPoolDataSource(String traceDataSource, ConnectionPoolDataSource cpDS)
	{
		String className = null;

		traceDataSource_ = traceDataSource;
		cpDS_	= cpDS;
		if (cpDS_ != null)
			className = cpDS_.getClass().getName();

		// Build up jdbcTrace output entry
		setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText
			+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
			+ "]:[" + Thread.currentThread() + "]:["
			+ System.identityHashCode(cpDS_) +  "]:" 
			+ className.substring(org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME,className.length()) 
			+ ".");
	}

	public TConnectionPoolDataSource()
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
	ConnectionPoolDataSource	cpDS_;
	private String		traceId_;
	String		dataSourceName_;
	String		description_;

	// serialVersionUID set to resolve javax -Xlint:serial definition warning.
	private static final long serialVersionUID = 4L;

	public Logger getParentLogger() throws SQLFeatureNotSupportedException {
		// TODO Auto-generated method stub
		return null;
	}
}
