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
 * Filename    : TClobWriter.java
 * Description :
 *
 * --------------------------------------------------------------------
 */
package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.*;
import java.util.Date;

public class TClobWriter extends java.io.Writer
{
	public void close() throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"close()");
		cw_.close();
	}

	public void flush()
           throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"flush()");
		cw_.flush();
	}

	public void write(char c) throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"write(" + c + ")");
		cw_.write(c);
	}

	public void write(char[] cbuf)
         throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"write(" + cbuf + ")");
		cw_.write(cbuf);
	}

	public void write(char[] cbuf,
                int off,
                int len)
         throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"write(" + cbuf + ", " + off + ", " + len + ")");
		cw_.write(cbuf, off, len);
	}

	public void write(String str) throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"write(" + str.getClass().getName() + ")");
		cw_.write(str);
	}
	
	public void write(String str,
                int off,
                int len)
         throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"write(" + str.getClass().getName() + ", " + off + ", " + len + ")");
		cw_.write(str, off, len);
	}

	// Constructors with access specifier as "default"
    TClobWriter(Writer cw, PrintWriter out)
    {
        
        
        cw_ = cw;
        out_ = out;
        
    }
	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		String className = null;
		if (cw_ != null)
            className = cw_.getClass().getName();

        // Build up jdbcTrace output entry
		setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText
			+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
			+ "]:[" + Thread.currentThread() + "]:["
			+ System.identityHashCode(cw_) +  "]:"
			+ className.substring(org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME,className.length()) 
			+ ".");
		return traceId_;
	}
	//fields
	Writer cw_;
	PrintWriter out_;
	private String traceId_;
}
