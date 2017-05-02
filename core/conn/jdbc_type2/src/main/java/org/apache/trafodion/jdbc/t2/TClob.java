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
 * Filename    : TClob.java
 * Description :
 *
 * --------------------------------------------------------------------
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.*;
import java.util.Date;

public class TClob implements java.sql.Clob
{
	public InputStream getAsciiStream() throws SQLException
	{
		if (out_ != null)
		    out_.println(getTraceId() + "getAsciiStream()");
		return new TLobInputStream(clob_.getAsciiStream(), out_);
	}

	public Reader getCharacterStream() throws SQLException
	{
		if (out_ != null)
		    out_.println(getTraceId() + "getCharacterStream()");
		return new TClobReader(clob_.getCharacterStream(), out_);
	}

	public String getSubString(long pos, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getSubString(" + pos + ", " + length +")");
		return clob_.getSubString(pos, length);
	}

	public long length() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "length()");
		return clob_.length();
	}

	public long position(Clob searchstr, long start) throws SQLException
	{	
		if (out_ != null)
			out_.println(getTraceId() + "position(" + searchstr + ", " + start + ")");
		return clob_.position(searchstr, start);
	}

	public long position(String searchstr,long start) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "position(\"" + searchstr + "\", " + start + ")");
		return clob_.position(searchstr, start);
	}
	
	public OutputStream setAsciiStream(long pos) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setAsciiStream("+ pos + ")");
		return new TLobOutputStream(clob_.setAsciiStream(pos), out_);
	}

	public Writer setCharacterStream(long pos) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setCharacterStream("+ pos + ")");
		return new TClobWriter(clob_.setCharacterStream(pos), out_);
	}

	public int setString(long pos, String str) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setString(" + pos + ", <String>)");
		return clob_.setString(pos, str);
	}

	public int setString(long pos, String str, int offset, int len)
              throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setString(" + pos + ", <String>, " + offset + ", "
							+ len + ")");
		return clob_.setString(pos, str, offset, len);
	}

	public void truncate(long len) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "truncate("+ len + ")");
		clob_.truncate(len);
	}

	// Constructors with access specifier as "default"
    TClob(Clob clob, PrintWriter out)
    {
        clob_ = clob;
        out_ = out;
        
    }
	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		String className = null;
		if (clob_ != null)
            className = clob_.getClass().getName();

        // Build up jdbcTrace output entry
		setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText
			+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date())
			+ "]:[" + Thread.currentThread() + "]:["
			+ System.identityHashCode(clob_) +  "]:"
			+ className.substring(org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME,className.length()) 
			+ "."); 
		return traceId_;
	}
	//fields
	Clob clob_;
	PrintWriter out_;
	private String traceId_;

	public void free() throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public Reader getCharacterStream(long pos, long length) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
}
