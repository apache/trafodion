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
 * Filename    : TClobReader.java
 * Description :
 *
 * --------------------------------------------------------------------
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.*;
import java.util.Date;

public class TClobReader extends java.io.Reader
{
	public void close() throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"close()");
		cr_.close();
	}

	public void mark(int readlimit) throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"mark("+ readlimit + ")");
		cr_.mark(readlimit);
	}

	public boolean markSupported()
	{
		if (out_ != null)
			out_.println(getTraceId()+"markSupported()");
		return cr_.markSupported();
	}

	public int read() throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"read()");
		return cr_.read();
	}

	public int read(char[] cbuf)
         throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"read(" + cbuf + ")");
		return cr_.read(cbuf);
	}

	public int read(char[] cbuf,
                int off,
                int len)
         throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"read(" + cbuf + ", " + off + ", " + len + ")");
		return cr_.read(cbuf, off, len);
	}

	public boolean ready() throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"ready()");
		return cr_.ready();
	}

	public void reset() throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"reset()");
		cr_.reset();
	}

	public long skip(long n)
          throws IOException
	{
		if (out_ != null)
			out_.println(getTraceId()+"skip(" + n + ")");
		return cr_.skip(n);
	}
	
	// Constructors with access specifier as "default"
    TClobReader(Reader cr, PrintWriter out)
    {
        
        
        cr_ = cr;
        out_ = out;
        
    }
	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		String className = null;
		if (cr_ != null)
            className = cr_.getClass().getName();

        // Build up jdbcTrace output entry
		setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText
			+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
			+ "]:[" + Thread.currentThread() + "]:["
			+ System.identityHashCode(cr_) +  "]:"
			+ className.substring(org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME,className.length()) 
			+ ".");
		return traceId_;
	}
	//fields
	Reader cr_;
	PrintWriter out_;
	private String traceId_;
}
