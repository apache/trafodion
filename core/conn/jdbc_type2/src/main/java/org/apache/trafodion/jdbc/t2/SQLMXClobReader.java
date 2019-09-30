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
 * Filename    : SQLMXClobReader.java
 * Description : This program implements the Reader interface. This object
 *               returned to the application when Clob.getAsciiStream() 
 *               method is called. The application can use this object
 *               to read the clob data
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.Reader;
import java.io.IOException;
import java.util.Date;
import java.io.PrintWriter;
import java.nio.CharBuffer;

public class SQLMXClobReader extends Reader
{

	public void close() throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_close].methodEntry();
		try
		{
			isClosed_ = true;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_close].methodExit();
		}
	}

	public void mark(int readAheadLimit) throws IOException
	{
		if (JdbcDebugCfg.entryActive) 
		{
			debug[methodId_mark].methodEntry();
			debug[methodId_mark].methodExit();
		}
	}

	public boolean markSupported()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_markSupported].methodEntry();
		try
		{
			return false;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_markSupported].methodExit();
		}
	}

	public int read() throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_read_V].methodEntry();
		try
		{
			int retValue = 0;

			if (eor_)
				return -1;
			if (currentChar_ == charsRead_)
				retValue = readChunkThrowIO();
			if (retValue != -1)
			{
				retValue = chunk_.get();
				currentChar_++;
			}
			return retValue;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_read_V].methodExit();
		}
	}

	public int read(char[] cbuf) throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_read_C].methodEntry();
		try
		{
			if (cbuf == null)
				throw new IOException("Invalid input value");
			return read(cbuf, 0, cbuf.length);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_read_C].methodExit();
		}
	}

	public int read(char[] cbuf,
		int off,
		int len)
		throws IOException
	{
		return read(cbuf, off, len, false);
	}

	public int read(char[] cbuf,
		int off,
		int len,
		boolean skip)
		throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_read_CII].methodEntry();
		try
		{
			int remainLen;
			int copyLen;
			int copyOffset;
			int retLen;
			int availableLen;
			int copiedLen = 0;

			if (cbuf == null)
				throw new IOException("Invalid input value");
                        if (eor_)
				return -1;
			remainLen = len;
			copyOffset = off;
			while (remainLen > 0) {
				availableLen = charsRead_ - currentChar_;
				if (availableLen > remainLen)	
					copyLen = remainLen;
				else
					copyLen = availableLen;
				if (copyLen > 0) { 
					if (! skip) 
						System.arraycopy(chunk_, currentChar_, cbuf, copyOffset, copyLen);
					currentChar_ += copyLen;
					copyOffset += copyLen;
					copiedLen += copyLen;
					remainLen -= copyLen;
				}
				if (remainLen > 0) {
					retLen = readChunkThrowIO();
					if (retLen == -1)
						break;
				}
			}
			return copiedLen;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_read_CII].methodExit();
		}
	}

	public void reset()
		throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_reset].methodEntry();
		try
		{
			currentChar_ = 0;
			charsRead_ = 0;
			eor_ = false;
			return;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_reset].methodExit();
		}
	}

	public long skip(long n)
		throws IOException
	{
		long totalSkippedLen = 0;
		long skipRemain = n;
		int skipLen;
		int skippedLen;
		while (skipRemain > 0) {
			if (skipRemain <= Integer.MAX_VALUE)
				skipLen = (int)skipRemain;
			else
				skipLen = Integer.MAX_VALUE;	
			skippedLen = read(null, 0, skipLen, true); 
			if (skippedLen == -1)
				break;
			skipRemain -= skippedLen;
			totalSkippedLen += skippedLen;
		}	
		return totalSkippedLen;
	}

	int readChunkThrowIO() throws IOException
	{
		int readLen;
		try {
			readLen = readChunk();
		}
		catch (SQLException e) {
			throw new IOException(SQLMXLob.convSQLExceptionToIO(e));
		}
		return readLen;
	}

	int readChunk() throws SQLException
	{
		int extractMode = 1; // get the lob data
		chunk_.clear(); 
		if (eor_)
			return -1;
		if (charsRead_ != 0 && (charsRead_ < clob_.chunkSize_)) {
			eor_ = true;
			extractMode = 2; // Close the extract
		}
		charsRead_ = readChunk(conn_.server_, conn_.getDialogueId(), conn_.getTxid(), extractMode, clob_.lobLocator_, chunk_); 
		if (charsRead_ == -1) {
			extractMode = 2; // close the extract
		 	readChunk(conn_.server_, conn_.getDialogueId(), conn_.getTxid(), extractMode, clob_.lobLocator_, chunk_); 
			eor_ = true;
			chunk_.limit(0);
		} else if (charsRead_ == 0) {
			charsRead_ = -1;
			eor_ = true;
			chunk_.limit(0);
		} else
			chunk_.limit(charsRead_);
		return charsRead_;
	}

        native int readChunk(String server, long dialogueId, long txid,  int extractMode, String lobLocator, CharBuffer buffer);

	// constructors
	SQLMXClobReader(SQLMXConnection connection, SQLMXClob clob)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXClobReader].methodEntry();
		try
		{
			clob_ = clob;
			conn_ = connection;
			chunk_ = CharBuffer.allocate(clob_.chunkSize_);
			currentChar_ = 0;
			charsRead_ = 0;
			eor_ = false;
			isClosed_ = false;
			traceWriter_ = SQLMXDataSource.traceWriter_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXClobReader].methodExit();
		}
	}

	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		// Build up template portion of jdbcTrace output. Pre-appended to jdbcTrace entries.
		// jdbcTrace:[XXXX]:[Thread[X,X,X]]:[XXXXXXXX]:ClassName.
		if (traceWriter_ != null) 
		{
			traceFlag_ = T2Driver.traceFlag_;
			String className = getClass().getName();
			setTraceId(T2Driver.traceText + T2Driver.dateFormat.format(new Date()) 
				+ "]:[" + Thread.currentThread() + "]:[" + hashCode() +  "]:" 
				+ className.substring(T2Driver.REMOVE_PKG_NAME,className.length()) 
				+ ".");
		}
		return traceId_;
	}

	// Fields 
	private String		traceId_;
	static PrintWriter	traceWriter_;
	static int		traceFlag_;
	SQLMXClob		clob_;
	SQLMXConnection		conn_;
	boolean			isClosed_;
	CharBuffer		chunk_;
	int			currentChar_;
	int			charsRead_;
	boolean			eor_;
	

	private static int methodId_close				=  0;
	private static int methodId_mark				=  1;
	private static int methodId_markSupported		=  2;
	private static int methodId_read_V				=  3;
	private static int methodId_read_C				=  4;
	private static int methodId_read_CII			=  5;
	private static int methodId_reset				=  6;
	private static int methodId_skip				=  7;
	private static int methodId_readChunkThrowIO	=  8;
	private static int methodId_readChunk			=  9;
	private static int methodId_SQLMXClobReader		= 10;
	private static int totalMethodIds				= 11;
	private static JdbcDebug[] debug;
	
	static
	{
		String className = "SQLMXClobReader";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_close] = new JdbcDebug(className,"close");
			debug[methodId_mark] = new JdbcDebug(className,"mark");
			debug[methodId_markSupported] = new JdbcDebug(className,"markSupported");
			debug[methodId_read_V] = new JdbcDebug(className,"read[V]");
			debug[methodId_read_C] = new JdbcDebug(className,"read[C]");
			debug[methodId_read_CII] = new JdbcDebug(className,"read[CII]");
			debug[methodId_reset] = new JdbcDebug(className,"reset");
			debug[methodId_skip] = new JdbcDebug(className,"skip");
			debug[methodId_readChunkThrowIO] = new JdbcDebug(className,"readChunkThrowIO");
			debug[methodId_readChunk] = new JdbcDebug(className,"readChunk");
			debug[methodId_SQLMXClobReader] = new JdbcDebug(className,"SQLMXClobReader");
		}
	}
}
