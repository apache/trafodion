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
 * Filename    : TPooledConnection.java
 * Description :
 *
 * --------------------------------------------------------------------
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import javax.sql.*;
import java.io.PrintWriter;
import java.util.Date;

public class TPooledConnection implements javax.sql.PooledConnection 
{

	public void addConnectionEventListener(ConnectionEventListener listener) 
	{
		if (out_ != null)
			out_.println(getTraceId() + "addConnectionEventListener("+ listener + ")");
		pc_.addConnectionEventListener(listener);
	}

	public void close() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "close()");
		pc_.close();
	}

	public Connection getConnection() throws SQLException 
	{
		if (out_ != null)
			out_.println(getTraceId() + "getConnection()");
		return new TConnection(pc_.getConnection(), out_);

	}

	public void removeConnectionEventListener(ConnectionEventListener listener) 
	{
		if (out_ != null)
			out_.println(getTraceId() + "removeConnectionEventListener("+ listener + ")");
		pc_.removeConnectionEventListener(listener);
	}

	public TPooledConnection(PooledConnection pc, PrintWriter out) throws SQLException
	{
		
		
		pc_ = pc;
		out_ = out;
		
	} 
	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		String  className = null;
		if (pc_ != null)
			className =  pc_.getClass().getName();

		// Build up jdbcTrace output entry
		setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText
			+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
			+ "]:[" + Thread.currentThread() + "]:["
			+ System.identityHashCode(pc_) +  "]:" 
			+ className.substring(org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME,className.length()) 
			+ ".");
		return traceId_;
	}
	// Fields 
	PooledConnection	pc_;
	private String				traceId_ ;
	PrintWriter			out_;		

        public void addStatementEventListener(StatementEventListener listener) {
                // TODO Auto-generated method stub

        }

	public void removeStatementEventListener(StatementEventListener listener) {
		// TODO Auto-generated method stub
		
	}
}

