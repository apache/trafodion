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
 * Filename    : SQLMXLobOutputStream.java
 * Description : This program implements the OutputStream interface.
 *      This object returned to the application when Clob.setOutputStream()
 *      method or Blob.setOutputStream is called. The application can use 
 *      this object to write the clob/blob data
 */


package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.OutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Date;
import java.io.PrintWriter;

public class SQLMXLobOutputStream extends OutputStream
{
	public void close() throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_close].methodEntry();
		try
		{
			if (! isClosed_)
			{
				flush();
				isClosed_ = true;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_close].methodExit();
		}
	}

	public void flush() throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_flush].methodEntry();
		try
		{
			if (isClosed_)
				throw new IOException("Output stream is in closed state");
			if (! isFlushed_) {
				writeChunkThrowIO(chunk_, 0, currentByte_);
				currentByte_ = 0;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_flush].methodExit();
		}
	}

	public void write(int b) throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_write_I].methodEntry();
		try
		{
			if (isClosed_)
				throw new IOException("Output stream is in closed state");
			chunk_[currentByte_] = (byte)b;
			isFlushed_ = false;
			currentByte_++;
			if (currentByte_ == lob_.chunkSize_) {
				writeChunkThrowIO(chunk_, 0, currentByte_);
				currentByte_ = 0;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_write_I].methodExit();
		}
	}


	public void write(byte[] b) throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_write_B].methodEntry();
		try
		{
			if (b == null)
				throw new IOException("Invalid input value");
			write(b, 0, b.length);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_write_B].methodExit();
		}
	}

	public void write(byte[] b, int off, int len) throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_write_BII].methodEntry();
		try
		{
			int copyLen;
			int srcOffset;
			int tempLen;

			if (isClosed_)
				throw new IOException("Output stream is in closed state");
			if (b == null)
				throw new IOException("Invalid input value");
			if (off < 0 || len < 0 || off > b.length)
				throw new IndexOutOfBoundsException(
					"length or offset is less than 0 or offset is greater than the length of array");
			srcOffset = off;
			copyLen = len;
			while (true) {
				if ((copyLen+currentByte_) < lob_.chunkSize_) {
					System.arraycopy(b, srcOffset, chunk_, currentByte_, copyLen);
					currentByte_ += copyLen;
					isFlushed_ = false;
					break;
				} else {
					if (currentByte_ != 0) {
						tempLen = lob_.chunkSize_-currentByte_;		
						System.arraycopy(b, srcOffset, chunk_, currentByte_, tempLen);
						currentByte_ += tempLen;
						writeChunkThrowIO(chunk_, 0, currentByte_);
						currentByte_ = 0;
					} else {
						tempLen = lob_.chunkSize_;
						writeChunkThrowIO(b, srcOffset, tempLen);
					}
					copyLen -= tempLen;
					srcOffset += tempLen;
				}
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_write_BII].methodExit();
		}
	}

        void writeChunkThrowIO(byte[] chunk, int off, int len) throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_writeChunkThrowIO].methodEntry();
		try
		{
			try
			{
				writeChunk(chunk, off, len);
			}
			catch (SQLException e)
			{
				throw new IOException(SQLMXLob.convSQLExceptionToIO(e));
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_writeChunkThrowIO].methodExit();
		}
	}

	
	void writeChunk(byte[] chunk, int off, int len) throws SQLException
	{
		writeChunk(conn_.server_, conn_.getDialogueId(), conn_.getTxid(),
				lob_.lobLocator_, chunk, off, len, startingPos_-1+off);
	}

	void populate(InputStream is, long length) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_populate].methodEntry();
		try
		{
			int tempLen;
			long readLen;
			int retLen=0;
				
			readLen = length;
			try
			{
				while (true)
				{
					if (readLen <= lob_.chunkSize_)
						tempLen = (int)readLen;
					else
						tempLen = lob_.chunkSize_;
					retLen = is.read(chunk_, 0, tempLen);
					if (retLen == -1 || (length != 0 && readLen == 0))
						break;
					writeChunk(chunk_, 0, retLen);
					if (length > 0)
						readLen -= retLen;
				}
			}
			catch (IOException e)
			{
				Object[] messageArguments = new Object[1];
				messageArguments[0] = e.getMessage();
				throw Messages.createSQLException(conn_.locale_, "io_exception", 
					messageArguments);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_populate].methodExit();
		}
	}

	// constructors
	SQLMXLobOutputStream(SQLMXConnection connection, long startingPos, SQLMXLob lob) throws 
		SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXLobOutputStream].methodEntry();
		try
		{
			long length;

			lob_ = lob;
			length = lob_.inLength();
			conn_ = connection;
			chunk_ = new byte[lob_.chunkSize_];
			isFlushed_ = false;
			startingPos_ = startingPos;
			traceWriter_ = SQLMXDataSource.traceWriter_;
			currentByte_ = 0;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXLobOutputStream].methodExit();
		}
	}

	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		return traceId_;
	}
	// Fields
	private String			traceId_;
	static PrintWriter		traceWriter_;
	static int			traceFlag_;
	SQLMXLob			lob_;
	long				startingPos_;
	SQLMXConnection			conn_;
	boolean				isClosed_;
	byte[]				chunk_;
	int				currentByte_;
	int				currentChunkNo_;
	boolean				isFlushed_;
	int				updChunkNo_;
	int				updOffset_;

	private static int methodId_close					= 0;
	private static int methodId_flush					= 1;
	private static int methodId_write_B					= 2;
	private static int methodId_write_BII				= 3;
	private static int methodId_write_I					= 4;
	private static int methodId_writeChunk				= 5;
	private static int methodId_writeChunkThrowIO		= 6;
	private static int methodId_populate				= 7;
	private static int methodId_SQLMXLobOutputStream	= 8;
	private static int totalMethodIds					= 9;
	private static JdbcDebug[] debug;
	
	static
	{
		String className = "SQLMXLobOutputStream";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_close] = new JdbcDebug(className,"close");
			debug[methodId_flush] = new JdbcDebug(className,"flush");
			debug[methodId_write_B] = new JdbcDebug(className,"write[B]");
			debug[methodId_write_BII] = new JdbcDebug(className,"write[BII]");
			debug[methodId_write_I] = new JdbcDebug(className,"write[I]");
			debug[methodId_writeChunk] = new JdbcDebug(className,"writeChunk");
			debug[methodId_writeChunkThrowIO] = new JdbcDebug(className,"writeChunkThrowIO");
			debug[methodId_populate] = new JdbcDebug(className,"populate");
			debug[methodId_SQLMXLobOutputStream] = new JdbcDebug(className,"SQLMXLobOutputStream");
		}
	}
	native void writeChunk(String server, long dialogueId, long txid, String lobLocator, byte[] chunk, int off, int writeLength, long pos);
}
