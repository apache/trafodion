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
 * Filename    : TBlob.java
 * Description :
 *
 * --------------------------------------------------------------------
 */
package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.*;
import java.util.Date;

public class TBlob implements java.sql.Blob
{
	public InputStream getBinaryStream() throws SQLException
	{
		if (out_ != null)
		    out_.println(getTraceId() + "getBinaryStream()");
		return new TLobInputStream(blob_.getBinaryStream(), out_);
	}

	public byte[] getBytes(long pos, int length) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "getBytes(" + pos + ", " + length +")");
		return blob_.getBytes(pos, length);
	}

	public long length() throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "length()");
		return blob_.length();
	}

	public long position(Blob pattern, long start) throws SQLException
	{	
		if (out_ != null)
			out_.println(getTraceId() + "position(" + pattern + ", " + start + ")");
		return blob_.position(pattern, start);
	}

	public long position(byte[] pattern,long start) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "position(\"" + pattern + "\", " + start + ")");
		return blob_.position(pattern, start);
	}
	
	public OutputStream setBinaryStream(long pos) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setBinaryStream("+ pos + ")");
		return new TLobOutputStream(blob_.setBinaryStream(pos), out_);
	}

	public int setBytes(long pos, byte[] bytes) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setBytes(" + pos + ", \"" + bytes + "\")");
		return blob_.setBytes(pos, bytes);
	}

	public int setBytes(long pos, byte[] bytes, int offset, int len)
              throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "setBytes(" + pos + ", \"" + bytes + "\", " + offset + ", "
							+ len + ")");
		return blob_.setBytes(pos, bytes, offset, len);
	}

	public void truncate(long len) throws SQLException
	{
		if (out_ != null)
			out_.println(getTraceId() + "truncate("+ len + ")");
		blob_.truncate(len);
	}

	// Constructors with access specifier as "default"
    TBlob(Blob blob, PrintWriter out)
    {
        
        
        blob_ = blob;
        out_ = out;
        
    }
	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		
		String className = null;
		if (blob_ != null)
            className = blob_.getClass().getName();

        // Build up jdbcTrace output entry
		setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText 
			+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
			+ "]:[" + Thread.currentThread() + "]:["
			+ System.identityHashCode(blob_) +  "]:"
			+ className.substring(org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME,className.length()) 
			+ ".");
        
		return traceId_;
	}

	//fields
	Blob blob_;
	PrintWriter out_;
	private String traceId_;

        public void free() throws SQLException {
                // TODO Auto-generated method stub

        }

        public InputStream getBinaryStream(long pos, long length)
                        throws SQLException {
                // TODO Auto-generated method stub
                return null;
        }

}
