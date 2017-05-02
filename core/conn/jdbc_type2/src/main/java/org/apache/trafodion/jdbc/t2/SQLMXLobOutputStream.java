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
			if (! isFlushed_)
				writeChunkThrowIO();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_flush].methodExit();
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
			int	srcOffset;
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
			while (true)
			{
				if ((copyLen+currentByte_) < lob_.chunkSize_)
				{
					System.arraycopy(b, srcOffset, chunk_, currentByte_, copyLen);
					currentByte_ += copyLen;
					isFlushed_ = false;
					break;
				}
				else
				{
					tempLen = lob_.chunkSize_-currentByte_;		
					System.arraycopy(b, srcOffset, chunk_, currentByte_, tempLen);
					currentByte_ += tempLen;
					writeChunkThrowIO();
					copyLen -= tempLen;
					srcOffset += tempLen;
					currentByte_ = 0;
				}
				
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_write_BII].methodExit();
		}
	}
	
	public void write(int b)
		throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_write_I].methodEntry();
		try
		{
			if (isClosed_)
				throw new IOException("Output stream is in closed state");
			chunk_[currentByte_] = (byte)b;
			isFlushed_ = false;
			currentByte_++;
			if (currentByte_ == lob_.chunkSize_)
				writeChunkThrowIO();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_write_I].methodExit();
		}
	}

	void writeChunk() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_writeChunk].methodEntry();
		try
		{
			byte[] tempChunk;

			if (currentChunkNo_ > updChunkNo_)
			{
				lob_.prepareInsLobDataStmt();
				PreparedStatement InsLobStmt = lob_.getInsLobDataStmt();

				synchronized (InsLobStmt)
				{
					InsLobStmt.setString(1, lob_.tableName_);
					InsLobStmt.setLong(2, lob_.dataLocator_);
					InsLobStmt.setInt(3, currentChunkNo_);
					if (currentByte_ != lob_.chunkSize_)
					{
						tempChunk = new byte[currentByte_];
						System.arraycopy(chunk_, 0, tempChunk, 0, currentByte_);
					}
					else
						tempChunk = chunk_;	
					InsLobStmt.setBytes(4, tempChunk);
					InsLobStmt.executeUpdate();
					currentChunkNo_++;
					currentByte_ = 0;
				}
			}
			else
			{
				lob_.prepareUpdLobDataStmt();
				PreparedStatement UpdLobStmt = lob_.getUpdLobDataStmt();

				synchronized (UpdLobStmt)
				{
					UpdLobStmt.setString(4, lob_.tableName_);
					UpdLobStmt.setLong(5, lob_.dataLocator_);
					UpdLobStmt.setInt(6, currentChunkNo_);
					UpdLobStmt.setInt(1, updOffset_);
					if (updOffset_ != 0 || currentByte_ != lob_.chunkSize_)
					{
						tempChunk = new byte[currentByte_-updOffset_];
						System.arraycopy(chunk_, updOffset_, tempChunk, 0, currentByte_-updOffset_);
					}
					else
						tempChunk = chunk_;	
					UpdLobStmt.setInt(3, currentByte_+1);
					UpdLobStmt.setBytes(2, tempChunk);
					UpdLobStmt.executeUpdate();
					currentChunkNo_++;
					currentByte_ = 0;
					updOffset_ = 0;
				}
			}
			isFlushed_ = true;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_writeChunk].methodExit();
		}
	}

	void writeChunkThrowIO() throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_writeChunkThrowIO].methodEntry();
		try
		{
			try
			{
				writeChunk();
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

	void populate(InputStream is, int length) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_populate].methodEntry();
		try
		{
			int tempLen;
			int readLen;
			int retLen=0;
				
			readLen = length;
			try
			{
				while (readLen > 0)
				{
					if (readLen <= lob_.chunkSize_)
						tempLen = readLen;
					else
						tempLen = lob_.chunkSize_;
					retLen = is.read(chunk_, 0, tempLen);
					if (retLen == -1)
						break;
					currentByte_ = retLen;

					if ((traceWriter_ != null) && 
						((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL)))
					{
						// For tracing, only print the 1st and last LOB data chunk write info to limit 
						// potential overflow of buffer for trace output.
						if (readLen==length) 			// 1st writeChunk
						{
							traceWriter_.println(getTraceId()
								+ "populate() -  First writeChunk data: tableName_=" + lob_.tableName_
								+ " dataLocator_=" + lob_.dataLocator_ + " length=" + length 
								+ " currentChunkNo_=" + currentChunkNo_ + " updChunkNo_=" + updChunkNo_ + " retLen=" + retLen);
						}
						if (readLen<=lob_.chunkSize_)	// last writeChunk (NOTE: last chunk can be exactly chunkSize_)
						{
							traceWriter_.println(getTraceId()
								+ "populate() -  Last writeChunk data: tableName_=" + lob_.tableName_
								+ " dataLocator_=" + lob_.dataLocator_ + " length=" + length 
								+ " currentChunkNo_=" + currentChunkNo_ + " updChunkNo_=" + updChunkNo_ + " retLen=" + retLen);
						}
					}

					writeChunk();
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
	SQLMXLobOutputStream(SQLMXConnection connection, SQLMXLob lob, long pos) throws 
		SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXLobOutputStream].methodEntry();
		try
		{
			long length;

			lob_ = lob;
			length = lob_.length();
			conn_ = connection;
			if (pos < 1 || pos > length+1)
				throw Messages.createSQLException(conn_.locale_,"invalid_position_value", null);
			startingPos_ = pos;
			chunk_ = new byte[lob_.chunkSize_];
			isFlushed_ = false;
			if (length == 0)
				updChunkNo_ = -1;
			else
			{
				if ((length % lob_.chunkSize_) == 0)
					updChunkNo_ = (int)(length / lob_.chunkSize_)-1;
				else
					updChunkNo_ = (int)(length / lob_.chunkSize_);
			}
			currentChunkNo_ = (int)((pos-1)/ lob_.chunkSize_);
			currentByte_ = (int)((pos-1) % lob_.chunkSize_);
			updOffset_ = (int)((pos-1) % lob_.chunkSize_);

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
	private String				traceId_;
	static PrintWriter	traceWriter_;
	static int			traceFlag_;
	SQLMXLob			lob_;
	long				startingPos_;
	SQLMXConnection		conn_;
	boolean				isClosed_;
	byte[]				chunk_;
	int					currentByte_;
	int					currentChunkNo_;
	boolean				isFlushed_;
	int					updChunkNo_;
	int					updOffset_;

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
}
