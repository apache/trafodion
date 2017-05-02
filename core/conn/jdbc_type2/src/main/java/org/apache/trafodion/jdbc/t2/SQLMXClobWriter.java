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
 * Filename    : SQLMXClobWriter.java
 * Description : This program implements the Writer interface. This
 *      object returned to the application when Clob.setAsciiStream() 
 *      method is called. The application can use this object to write 
 *      the clob data
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.Writer;
import java.io.Reader;
import java.io.IOException;
import java.util.Date;
import java.io.PrintWriter;

public class SQLMXClobWriter extends Writer
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
			{
				writeChunkThrowIO(null);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_flush].methodExit();
		}
	}

	public void write(char[] cbuf) throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_write_C].methodEntry();
		try
		{
			if (cbuf == null)
				throw new IOException("Invalid input value");
			write(cbuf, 0, cbuf.length);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_write_C].methodExit();
		}
	}

	public void write(char[] cbuf, int off, int len) throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_write_CII].methodEntry();
		try
		{
			int copyLen;
			int	srcOffset;
			int tempLen;

			if (isClosed_)
				throw new IOException("Writer is in closed state");
			if (cbuf == null)
				throw new IOException("Invalid input value");
			if (off < 0 || len < 0 || off > cbuf.length)
				throw new IndexOutOfBoundsException(
					"length or offset is less than 0 or offset is greater than the length of array");
			srcOffset = off;
			copyLen = len;
			while (true)
			{
				if (copyLen+currentChar_ < (clob_.chunkSize_))
				{
					System.arraycopy(cbuf, srcOffset, chunk_, currentChar_, copyLen);
					currentChar_ += copyLen;
					isFlushed_ = false;
					break;
				}
				else
				{
					if (currentChar_ != 0)
					{
						tempLen = clob_.chunkSize_-currentChar_;
						System.arraycopy(cbuf, srcOffset, chunk_, currentChar_, tempLen);
						currentChar_ += tempLen;
						writeChunkThrowIO(null);
					}
					else
					{
						tempLen = clob_.chunkSize_;
						currentChar_ += tempLen;
						writeChunkThrowIO(new String(cbuf, srcOffset, tempLen));
					}
					copyLen -= tempLen;
					srcOffset += tempLen;
					currentChar_ = 0;
				}
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_write_CII].methodExit();
		}
	}
	
	public void write(int c) throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_write_I].methodEntry();
		try
		{
			if (isClosed_)
				throw new IOException("Writer is in closed state");
			chunk_[currentChar_] = (char)c;
			isFlushed_ = false;
			currentChar_++;
			if (currentChar_ == clob_.chunkSize_)
				writeChunkThrowIO(null);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_write_I].methodExit();
		}
	}

	public void write(String str) throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_write_L].methodEntry();
		try
		{
			if (str == null)
				throw new IOException("Invalid input value");
			write(str, 0, str.length());
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_write_L].methodExit();
		}
	}

	public void write(String str, int off, int len) throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_write_LII].methodEntry();
		try
		{
			int tempLen;
			int writeLen;
			int srcOff;

			writeLen = len;
			srcOff = off;

			if (isClosed_)
				throw new IOException("Writer is in closed state");
			if (str == null)
				throw new IOException("Invalid input value");
			if (currentChar_ != 0)
			{
				tempLen = clob_.chunkSize_ - currentChar_;
				if (writeLen > tempLen)
				{		
					char[] cbuf = new char[tempLen];
					str.getChars(srcOff, srcOff+tempLen, cbuf, 0);
					write(cbuf, 0, cbuf.length);
					writeLen -= tempLen;
					srcOff += tempLen;
				}
			}
			while (writeLen > 0)
			{
				if (writeLen < clob_.chunkSize_)
					break;
				else
				{
					writeChunkThrowIO(str.substring(srcOff, srcOff+clob_.chunkSize_));
					writeLen -= clob_.chunkSize_;
					srcOff += clob_.chunkSize_;
				}
			}
			if (writeLen != 0)
			{
				char[] cbuf = new char[writeLen];
				str.getChars(srcOff, srcOff+writeLen, cbuf, 0);
				write(cbuf, 0, cbuf.length);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_write_LII].methodExit();
		}
	}

	void writeChunk(String str) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_writeChunk].methodEntry();
		try
		{
			String tempStr;
	
			if (currentChunkNo_ > updChunkNo_)
			{
				clob_.prepareInsLobDataStmt();
				PreparedStatement InsClobDataStmt = clob_.getInsLobDataStmt();

				synchronized (InsClobDataStmt)
				{
					InsClobDataStmt.setString(1, clob_.tableName_);
					InsClobDataStmt.setLong(2, clob_.dataLocator_);
					InsClobDataStmt.setInt(3, currentChunkNo_);
					if (str == null)
					{
						if (currentChar_ != clob_.chunkSize_)
							tempStr = new String(chunk_, 0, currentChar_);
						else
							tempStr = new String(chunk_);	
					}
					else
						tempStr = str;
					InsClobDataStmt.setString(4, tempStr);
					InsClobDataStmt.executeUpdate();
					currentChunkNo_++;
					currentChar_ = 0;
				}
			}
			else
			{
				clob_.prepareUpdLobDataStmt();
				PreparedStatement UpdClobDataStmt = clob_.getUpdLobDataStmt();

				synchronized (UpdClobDataStmt)
				{
					UpdClobDataStmt.setString(4, clob_.tableName_);
					UpdClobDataStmt.setLong(5, clob_.dataLocator_);
					UpdClobDataStmt.setInt(6, currentChunkNo_);
					UpdClobDataStmt.setInt(1, updOffset_);
					if (str == null)
					{
						if (updOffset_ != 0 || currentChar_ != clob_.chunkSize_)
							tempStr = new String(chunk_, updOffset_, currentChar_-updOffset_);
						else
							tempStr = new String(chunk_);	
					}
					else
						tempStr = str;		
					UpdClobDataStmt.setInt(3, currentChar_+1);
					UpdClobDataStmt.setString(2, tempStr);
					UpdClobDataStmt.executeUpdate();
					currentChunkNo_++;
					currentChar_ = 0;
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

	void writeChunkThrowIO(String str) throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_writeChunkThrowIO].methodEntry();
		try
		{
			try
			{
				writeChunk(str);
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
	
	void populate(Reader ir, int length) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_populate].methodEntry();
		try
		{
			int tempLen;
			int readLen;
			int retLen;
				
			readLen = length;
			try
			{
				while (readLen > 0)
				{
					if (readLen <= clob_.chunkSize_)
						tempLen = readLen;
					else
						tempLen = clob_.chunkSize_;
					retLen = ir.read(chunk_, 0, tempLen);
					if (retLen == -1)
						break;
					currentChar_ = retLen;

					if ((traceWriter_ != null) && 
						((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL)))
					{
						// For tracing, only print the 1st and last LOB data chunk write info to limit 
						// potential overflow of buffer for trace output.
						if (readLen==length) 			// 1st writeChunk
						{
							traceWriter_.println(getTraceId() 
								+ "populate() -  First writeChunk data: tableName_=" + clob_.tableName_
								+ " dataLocator_=" + clob_.dataLocator_ + " length=" + length 
								+ " currentChunkNo_=" + currentChunkNo_ + " updChunkNo_=" + updChunkNo_ + " retLen=" + retLen);
						}
						if (readLen<=clob_.chunkSize_)	// last writeChunk (NOTE: last chunk can be exactly chunkSize_)
						{
							traceWriter_.println(getTraceId() 
								+ "populate() -  Last writeChunk data: tableName_=" + clob_.tableName_
								+ " dataLocator_=" + clob_.dataLocator_ + " length=" + length 
								+ " currentChunkNo_=" + currentChunkNo_ + " updChunkNo_=" + updChunkNo_ + " retLen=" + retLen);
						}
					}

					writeChunk(null);
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
	SQLMXClobWriter(SQLMXConnection connection, SQLMXClob clob, long pos) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXClobWriter].methodEntry();
		try
		{
			long length;
		
			clob_ = clob;
			length = clob_.length();
			conn_ = connection;
			if (pos < 1 || pos > length+1)
				throw Messages.createSQLException(conn_.locale_,"invalid_position_value", null);
			startingPos_ = pos;
			chunk_ = new char[clob_.chunkSize_];
			isFlushed_ = false;
			if (length == 0)
				updChunkNo_ = -1;
			else
			{
				if ((length % clob_.chunkSize_) == 0)
					updChunkNo_ = (int)(length / clob_.chunkSize_)-1;
				else
					updChunkNo_ = (int)(length / clob_.chunkSize_);
			}
			currentChunkNo_ = (int)((pos-1)/ clob_.chunkSize_);
			currentChar_ = (int)((pos-1) % clob_.chunkSize_);
			updOffset_ = (int)((pos-1) % clob_.chunkSize_);

		
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXClobWriter].methodExit();
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
	private String				traceId_;
	static PrintWriter	traceWriter_;
	static int			traceFlag_;
	SQLMXClob			clob_;
	long				startingPos_;
	SQLMXConnection		conn_;
	boolean				isClosed_;
	char[]				chunk_;
	int					currentChar_;
	int					currentChunkNo_;
	boolean				isFlushed_;
	int					updChunkNo_;
	int					updOffset_;

	private static int methodId_close				=  0;
	private static int methodId_flush				=  1;
	private static int methodId_write_C				=  2;
	private static int methodId_write_CII			=  3;
	private static int methodId_write_I				=  4;
	private static int methodId_write_L				=  5;
	private static int methodId_write_LII			=  6;
	private static int methodId_writeChunk			=  7;
	private static int methodId_writeChunkThrowIO	=  8;
	private static int methodId_populate			=  9;
	private static int methodId_SQLMXClobWriter		= 10;
	private static int totalMethodIds				= 11;
	private static JdbcDebug[] debug;
	
	static
	{
		String className = "SQLMXClobWriter";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_close] = new JdbcDebug(className,"close");
			debug[methodId_flush] = new JdbcDebug(className,"flush");
			debug[methodId_write_C] = new JdbcDebug(className,"write[C]");
			debug[methodId_write_CII] = new JdbcDebug(className,"write[CII]");
			debug[methodId_write_I] = new JdbcDebug(className,"write[I]");
			debug[methodId_write_L] = new JdbcDebug(className,"write[L]");
			debug[methodId_write_LII] = new JdbcDebug(className,"write[LII]");
			debug[methodId_writeChunk] = new JdbcDebug(className,"writeChunk");
			debug[methodId_writeChunkThrowIO] = new JdbcDebug(className,"writeChunkThrowIO");
			debug[methodId_populate] = new JdbcDebug(className,"populate");
			debug[methodId_SQLMXClobWriter] = new JdbcDebug(className,"SQLMXClobWriter");
		}
	}
}
