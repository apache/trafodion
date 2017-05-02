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
 * Filename    : TLobOutputStream.java
 * Description :
 *
 * --------------------------------------------------------------------
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.*;
import java.util.Date;

public class TLobOutputStream extends java.io.OutputStream
{
	public void close() throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"close()");
		os_.close();
	}

	public void flush()
           throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"flush()");
		os_.flush();
	}

	public void write(int b) throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"write(" + b + ")");
		os_.write(b);
	}

	public void write(byte[] b)
         throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"write(" + b + ")");
		os_.write(b);
	}

	public void write(byte[] b,
                int off,
                int len)
         throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"write(" + b + ", " + off + ", " + len + ")");
		os_.write(b, off, len);
	}


	// Constructors with access specifier as "default"
    TLobOutputStream(OutputStream os, PrintWriter out)
    {
       
        
        os_ = os;
        out_ = out;
        
    }
	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		 String className = null;
		 if (os_ != null)
	            className = os_.getClass().getName();

	        // Build up jdbcTrace output entry
			setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText
				+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
				+ "]:[" + Thread.currentThread() + "]:["
				+ System.identityHashCode(os_) +  "]:"
				+ className.substring(org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME,className.length()) 
				+ ".");
		return traceId_;
	}
	//fields
	OutputStream os_;
	PrintWriter out_;
	private String traceId_;
}
