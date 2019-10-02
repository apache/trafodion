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
 * Filename		: SQLMXLobInputStream.java
 * Description	: This program implements the InputStream interface.
 *      This object returned to the application when Clob.getInputStream()
 *      method or Blob.getInputStream is called. The application can use
 *      this object to read the clob/blob data
 *
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.InputStream;
import java.io.IOException;
import java.util.Date;
import java.io.PrintWriter;
import java.nio.ByteBuffer;

public class SQLMXLobInputStream extends InputStream
{
	public int available() throws IOException
	{
		int remainLen;

		if (eos_)
			remainLen = 0;
		else {
			remainLen = bytesRead_ - currentPos_;
			if (remainLen == 0) // 0 would mean all the bytes are read from chunk_
				remainLen = lob_.chunkSize_;
		}
		return remainLen;
	}

	public void close() throws IOException
	{
		isClosed_ = true;
	}

	public void mark(int readlimit)
	{
	}

	public boolean markSupported()
	{
		return false;
	}

	public int read() throws IOException
	{	
		if (JdbcDebugCfg.entryActive) debug[methodId_read_V].methodEntry();
		try
		{
			int retValue = 0;

			if (eos_)
				return -1;
			if (currentPos_ == bytesRead_)
				retValue = readChunkThrowIO();
			if (retValue != -1)
			{
				retValue = chunk_.get();
				currentPos_++;
			}
			return retValue;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_read_V].methodExit();
		}
	}

	public int read(byte[] b) throws IOException
	{
		return read(b, 0, b.length);
	}

	public int read(byte[] b, int off, int len) throws IOException
	{
		return read(b, off, len, false);
	}

	public int read(byte[] b, int off, int len, boolean skip) throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_read_BII].methodEntry();
		try
		{
			int remainLen;
			int copyLen;
			int copyOffset;
			int retLen;
			int availableLen;
			int copiedLen = 0;

			if (b == null)
				throw new IOException("Invalid input value");
			if (eos_)
				return -1;		
			remainLen = len;
			copyOffset = off;
			
			while (remainLen > 0) {
				availableLen = bytesRead_ - currentPos_;
				if (availableLen > remainLen)	
					copyLen = remainLen;
				else
					copyLen = availableLen;
				if (copyLen > 0) {
					if (! skip)
						chunk_.get(b, copyOffset, copyLen);			
					else
						chunk_.position(currentPos_+copyLen);
					currentPos_ += copyLen;
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
			if (JdbcDebugCfg.entryActive) debug[methodId_read_BII].methodExit();
		}
	}

	public void reset() throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_reset].methodEntry();
		try
		{
			currentPos_ = 0;
			bytesRead_ = 0;
			return;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_reset].methodExit();
		}
	}

	public long skip(long n) throws IOException
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
		if (JdbcDebugCfg.entryActive) debug[methodId_readChunkThrowIO].methodEntry();
		try
		{
			int readLen;

			try
			{
				readLen = readChunk();
			}
			catch (SQLException e)
			{
				throw new IOException(SQLMXLob.convSQLExceptionToIO(e));
			}
			return readLen;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_readChunkThrowIO].methodExit();
		}
	}

	int readChunk() throws SQLException
	{
		int extractMode = 1; // get the lob data
		chunk_.clear(); 
		if (eos_)
			return -1;
		if (bytesRead_ > 0 && (bytesRead_ < lob_.chunkSize_)) {
			eos_ = true;
			extractMode = 2; // Close the extract
		}
		bytesRead_ = readChunk(conn_.server_, conn_.getDialogueId(), conn_.getTxid(), extractMode, lob_.lobLocator_, chunk_); 
		if (bytesRead_ == -1) {
			extractMode = 2; // close the extract
		 	readChunk(conn_.server_, conn_.getDialogueId(), conn_.getTxid(), extractMode, lob_.lobLocator_, chunk_); 
			eos_ = true;
			chunk_.limit(0);
		} else if (bytesRead_ == 0) {
			bytesRead_ = -1;
			eos_ = true;
			chunk_.limit(0);
		} else
			chunk_.limit(bytesRead_);
		return bytesRead_;
	}

        native int readChunk(String server, long dialogueId, long txid,  int extractMode, String lobLocator, ByteBuffer buffer);

	// Constructor
	SQLMXLobInputStream(SQLMXConnection connection, SQLMXLob lob)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXLobInputStream].methodEntry();
		try
		{
			lob_ = lob;
			conn_ = connection;
			chunk_ = ByteBuffer.allocateDirect(lob_.chunkSize_);
			bytesRead_ = 0;
			currentPos_ = 0;
			eos_ = false;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXLobInputStream].methodExit();
		}
	}
	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		traceWriter_ = SQLMXDataSource.traceWriter_;
		
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
	SQLMXLob		lob_;
	SQLMXConnection		conn_;
	boolean			isClosed_;
	ByteBuffer		chunk_;
	int			currentPos_;
	int			bytesRead_;
	boolean 		eos_;

	private static int methodId_available			=  0;
	private static int methodId_close				=  1;
	private static int methodId_mark				=  2;
	private static int methodId_markSupported		=  3;
	private static int methodId_read_V				=  4;
	private static int methodId_read_B				=  5;
	private static int methodId_read_BII			=  6;
	private static int methodId_reset				=  7;
	private static int methodId_skip				=  8;
	private static int methodId_readChunkThrowIO	=  9;
	private static int methodId_readChunk			= 10;
	private static int methodId_SQLMXLobInputStream	= 11;
	private static int totalMethodIds				= 12;
	private static JdbcDebug[] debug;
	
	static
	{
		String className = "SQLMXLobInputStream";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_available] = new JdbcDebug(className,"available");
			debug[methodId_close] = new JdbcDebug(className,"close");
			debug[methodId_markSupported] = new JdbcDebug(className,"markSupported");
			debug[methodId_read_V] = new JdbcDebug(className,"read[V]");
			debug[methodId_read_B] = new JdbcDebug(className,"read[B]");
			debug[methodId_read_BII] = new JdbcDebug(className,"read[BII]");
			debug[methodId_reset] = new JdbcDebug(className,"reset");
			debug[methodId_skip] = new JdbcDebug(className,"skip");
			debug[methodId_readChunkThrowIO] = new JdbcDebug(className,"readChunkThrowIO");
			debug[methodId_readChunk] = new JdbcDebug(className,"readChunk");
			debug[methodId_SQLMXLobInputStream] = new JdbcDebug(className,"SQLMXLobInputStream");
		}
	}
}
