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
 * Filename    : TParameterMetaData.java
 * Description :
 *
 * --------------------------------------------------------------------
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.PrintWriter;
import java.util.Date;

public class TParameterMetaData implements java.sql.ParameterMetaData
{

	public String getParameterClassName(int param) throws SQLException
 	{
		if (out_ != null)
			out_.println(getTraceId() + "getParameterClassName(" + param + ")");
		String retValue;

		retValue = pmd_.getParameterClassName(param);
		return retValue;
	}
	
 	public int getParameterCount() throws SQLException
 	{
 		if (out_ != null)
			out_.println(getTraceId() + "getParameterCount()");
		int retValue;

		retValue = pmd_.getParameterCount();
		return retValue;
 	}
 	
 	public int getParameterMode(int param) throws SQLException
 	{
 		if (out_ != null)
			out_.println(getTraceId() + "getParameterMode(" + param + ")");
		int retValue;

		retValue = pmd_.getParameterMode(param);
		return retValue;
	}
 	
 	public int getParameterType(int param) throws SQLException
 	{
 		if (out_ != null)
			out_.println(getTraceId() + "getParameterType(" + param + ")");
		int retValue;

		retValue = pmd_.getParameterType(param);
		return retValue;
	}
 	
 	public String getParameterTypeName(int param) throws SQLException
 	{
		if (out_ != null)
			out_.println(getTraceId() + "getParameterTypeName(" + param + ")");
		String retValue;

		retValue = pmd_.getParameterTypeName(param);
		return retValue;
	}
	
 	public int getPrecision(int param) throws SQLException
 	{
		if (out_ != null)
			out_.println(getTraceId() + "getPrecision(" + param + ")");
		int retValue;

		retValue = pmd_.getPrecision(param);
		return retValue;
 	}
 	
 	public int getScale(int param) throws SQLException
 	{
		if (out_ != null)
			out_.println(getTraceId() + "getScale(" + param + ")");
		int retValue;

		retValue = pmd_.getScale(param);
		return retValue;
	}
 	
 	public int isNullable(int param) throws SQLException
 	{
		if (out_ != null)
			out_.println(getTraceId() + "isNullable(" + param + ")");
		int retValue;

		retValue = pmd_.isNullable(param);
		return retValue;
 	}
 	
 	public boolean isSigned(int param) throws SQLException
 	{
		if (out_ != null)
			out_.println(getTraceId() + "isSigned(" + param + ")");
		boolean retValue;

		retValue = pmd_.isSigned(param);
		return retValue;
 	}
 	
 	// Constructors
    TParameterMetaData(ParameterMetaData pmd, PrintWriter out) throws SQLException
	{
		
		pmd_ = pmd;
		out_ = out;
		
	}
    public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		String className = null;
		
		if (pmd_ != null)
			className = pmd_.getClass().getName();

		// Build up jdbcTrace output entry
		setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText
			+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
			+ "]:[" + Thread.currentThread() + "]:["
			+ System.identityHashCode(pmd_) +  "]:" 
			+ className.substring(org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME,className.length()) 
			+ ".");

		return traceId_;
	}
	// Fields
	ParameterMetaData	pmd_;
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
	
    public SQLMXParameterMetaData getSqlParameterMetaData(){
        SQLMXParameterMetaData pmd = (SQLMXParameterMetaData)pmd_;
        return pmd;
    }
}
