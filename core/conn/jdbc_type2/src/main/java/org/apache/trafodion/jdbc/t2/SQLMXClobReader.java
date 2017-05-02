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

			if (isClosed_)
				throw new IOException("Reader is in closed state");
			if (currentChar_ == charsRead_)
				retValue = readChunkThrowIO(null, 0, clob_.chunkSize_);
			if (retValue != -1)
			{
				retValue = chunk_[currentChar_];
				if (currentChar_ != charsRead_)
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
		if (JdbcDebugCfg.entryActive) debug[methodId_read_CII].methodEntry();
		try
		{
			int readLen;
			int copyLen;
			int copyOffset;
			int tempLen = 0;
			int rowsToRead;
			int retLen;

			if (isClosed_)
				throw new IOException("Reader is in closed state");
			if (cbuf == null)
				throw new IOException("Invalid input value");
			copyLen = len;
			copyOffset = off;
			readLen = 0;
			if (currentChar_ < charsRead_)
			{
				if (copyLen+currentChar_ <= charsRead_)
				{
					System.arraycopy(chunk_, currentChar_, cbuf, copyOffset, copyLen);
					currentChar_ += copyLen;
					readLen = copyLen;
					return readLen;
				}
				else
				{
					tempLen = charsRead_- currentChar_;
					System.arraycopy(chunk_, currentChar_, cbuf, copyOffset, tempLen);
					copyOffset += tempLen;
					copyLen -= tempLen;
					currentChar_ += tempLen;
				}
			}
			readLen = readChunkThrowIO(cbuf, copyOffset, copyLen);
			if (readLen != -1)
				retLen = readLen + tempLen;
			else
				retLen = tempLen;
			if (retLen == 0)
				return -1;
			else
				return retLen;
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
			if (isClosed_)
				throw new IOException("Reader is in closed state");
			currentChar_ = 0;
			currentChunkNo_ = 0;
			charsRead_ = 0;
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
		if (JdbcDebugCfg.entryActive) debug[methodId_skip].methodEntry();
		try
		{
			long charsToSkip;
			int noOfChunks = 0;
			int remChars;
			long retLen = 0;
			long charsSkipped = 0;
			int oldChunkNo;
			int readLen;

			if (isClosed_)
				throw new IOException("Reader is in closed state");
			if (n <= 0)
				return 0;
			if (currentChar_ + n > charsRead_)
			{
				charsSkipped = charsRead_ - currentChar_;
				charsToSkip = n - charsSkipped;
				currentChar_ += charsSkipped;
			}
			else
			{
				currentChar_ += n;
				return n;
			}
			noOfChunks += (int)((charsToSkip-1) / clob_.chunkSize_);
			if ((charsToSkip % clob_.chunkSize_) == 0)
				remChars = clob_.chunkSize_;
			else
				remChars = (int)(charsToSkip % clob_.chunkSize_);
			oldChunkNo = currentChunkNo_;	// Which is already 1 more
			currentChunkNo_ = currentChunkNo_ + noOfChunks;
			retLen = readChunkThrowIO(null, 0, clob_.chunkSize_);
			if (retLen != -1)
			{
				charsSkipped += (currentChunkNo_ - oldChunkNo -1) * clob_.chunkSize_;
				if (retLen < remChars)
					remChars= (int)retLen;
				currentChar_ = remChars;
				charsSkipped += remChars;
			}
			else
			{
				if (currentChunkNo_ > 0)
					readLen = ((currentChunkNo_-1) * clob_.chunkSize_) + currentChar_;
				else
					readLen = currentChar_;
				try
				{
					charsSkipped = charsSkipped + clob_.length() - readLen;
				}
				catch (SQLException e)
				{
					throw new IOException(SQLMXLob.convSQLExceptionToIO(e));
				}
				// Exclude the bytes that are in chunk already
				remChars = (int)(charsSkipped - (charsRead_ - currentChar_));
				noOfChunks += (int)((remChars-1) / clob_.chunkSize_);
				currentChunkNo_ = oldChunkNo + noOfChunks;
				//calculate the bytes in the chunk and set currentChar and charsRead
				//to reach EOD
				if (remChars == 0)
				{
					currentChar_ = 0;
					charsRead_ = 0;
				}
				else
				{
					if ((remChars % clob_.chunkSize_) == 0)
						currentChar_ = clob_.chunkSize_;
					else
						currentChar_ = (int)(remChars % clob_.chunkSize_);
					charsRead_ = currentChar_;
				}
			}
			return charsSkipped;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_skip].methodExit();
		}
	}

	int readChunkThrowIO(char[] c, int off, int len) throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_readChunkThrowIO].methodEntry();
		try
		{
			int readLen;

			try
			{
				readLen = readChunk(c, off, len);
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

	int readChunk(char[] c, int off, int len) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_readChunk].methodEntry();
		try
		{
			int rowsToRead;
			String	data;
			int copyLen;
			int	copyOffset;
			int readLen = 0;
			int dataLen;

			rowsToRead = (len-1)/clob_.chunkSize_;
			clob_.prepareGetLobDataStmt();
			PreparedStatement GetClobDataStmt = clob_.getGetLobDataStmt();

			if ((traceWriter_ != null) && 
				((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL)))
			{
				traceWriter_.println(getTraceId() 
					+ "readChunk(<char>," + off + "," + len + ") - GetLobDataStmt params: tableName_=" + clob_.tableName_ 
					+ " dataLocator_=" + clob_.dataLocator_
					+ " currentChunkNo_=" + currentChunkNo_
					+ " currentChunkNo_+rowsToRead=" + (currentChunkNo_+rowsToRead));
			}

			synchronized (GetClobDataStmt)
			{
				GetClobDataStmt.setString(1, clob_.tableName_);
				GetClobDataStmt.setLong(2, clob_.dataLocator_);
				GetClobDataStmt.setInt(3, currentChunkNo_);
				GetClobDataStmt.setInt(4, currentChunkNo_+rowsToRead);
				ResultSet rs = GetClobDataStmt.executeQuery();
				copyLen = len;
				copyOffset = off;
				try 
				{
					while (rs.next())
					{
						data = rs.getString(1);
						currentChunkNo_++;
						charsRead_ = data.length();
						dataLen = charsRead_;
						if (c == null)
						{
							data.getChars(0, dataLen, chunk_, 0);
							readLen += dataLen;
							currentChar_ = 0;
							break;
						}
						else
						{
							if (copyLen >= dataLen)
							{
								data.getChars(0, dataLen, c, copyOffset);
								copyLen -= dataLen;
								readLen += dataLen;
								copyOffset += dataLen;
								currentChar_ = dataLen;
							} 
							else
							{
								data.getChars(0, copyLen, c, copyOffset);
								// copy the rest of data to chunk
								data.getChars(copyLen, dataLen,  chunk_, copyLen);
								readLen += copyLen;
								currentChar_ = copyLen;
								break;
							}
						}
					}
				} 
				finally 
				{
					rs.close();
				}
			}
			
			if ((traceWriter_ != null) && 
				((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL)))
			{
				traceWriter_.println(getTraceId() 
					+ "readChunk(<char>," + off + "," + len + ") - LOB data read: charsRead_=" + charsRead_ 
					+ " readLen=" + readLen + " copyLen=" + copyLen + " currentChunkNo_=" + currentChunkNo_);
			}

			if (readLen == 0)
				return -1;
			else
				return readLen;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_readChunk].methodExit();
		}
	}

	// constructors
	SQLMXClobReader(SQLMXConnection connection, SQLMXClob clob)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXClobReader].methodEntry();
		try
		{
			clob_ = clob;
			conn_ = connection;
			chunk_ = new char[clob_.chunkSize_];

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
	private String				traceId_;
	static PrintWriter	traceWriter_;
	static int			traceFlag_;
	SQLMXClob			clob_;
	SQLMXConnection		conn_;
	boolean				isClosed_;
	char[]				chunk_;
	int					currentChar_;
	int					currentChunkNo_;
	int					charsRead_;

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
