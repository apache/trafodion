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

public class SQLMXLobInputStream extends InputStream
{
	public int available() throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_available].methodEntry();
		try
		{
			long length;
			long readLength;

			if (isClosed_)
				throw new IOException("Input stream is in closed state");
			try
			{
				length = lob_.length();
				if (currentChunkNo_ > 0)
					readLength = ((currentChunkNo_-1) * lob_.chunkSize_) + currentByte_;
				else
					readLength = currentByte_;
				return (int)(length - readLength);
			}
			catch (SQLException e)
			{
				throw new IOException(SQLMXLob.convSQLExceptionToIO(e));
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_available].methodExit();
		}
	}

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

	public void mark(int readlimit)
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
				throw new IOException("Input stream is in closed state");
			if (currentByte_ == bytesRead_)
				retValue = readChunkThrowIO(null, 0, lob_.chunkSize_);
			if (retValue != -1)
			{
				retValue = chunk_[currentByte_];
				// Should be a value between 0 and 255 
				// -1 is mapped to 255, -2 is 254 etc
				if (retValue < 0)
					retValue = 256 + retValue; 
				if (currentByte_ != bytesRead_)
					currentByte_++;
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
		if (JdbcDebugCfg.entryActive) debug[methodId_read_B].methodEntry();
		try
		{
			if (b == null)
				throw new IOException("Invalid input value");
			return read(b, 0, b.length);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_read_B].methodExit();
		}
	}

	public int read(byte[] b, int off, int len) throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_read_BII].methodEntry();
		try
		{
			int readLen;
			int copyLen;
			int copyOffset;
			int tempLen = 0;
			int retLen;

			if (isClosed_)
				throw new IOException("Input stream is in closed state");
			if (b == null)
				throw new IOException("Invalid input value");
			copyLen = len;
			copyOffset = off;
			readLen = 0;
			if (currentByte_ < bytesRead_)
			{
				if (copyLen+currentByte_ <= bytesRead_)
				{
					System.arraycopy(chunk_, currentByte_, b, copyOffset, copyLen);
					currentByte_ += copyLen;
					readLen = copyLen;
					return readLen;
				}
				else
				{
					tempLen = bytesRead_- currentByte_;
					System.arraycopy(chunk_, currentByte_, b, copyOffset, tempLen);
					copyOffset += tempLen;
					copyLen -= tempLen;
					currentByte_ += tempLen;
				}
			}
			readLen = readChunkThrowIO(b, copyOffset, copyLen);
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
			if (JdbcDebugCfg.entryActive) debug[methodId_read_BII].methodExit();
		}
	}

	public void reset() throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_reset].methodEntry();
		try
		{
			if (isClosed_)
				throw new IOException("Input stream is in closed state");
			currentByte_ = 0;
			currentChunkNo_ = 0;
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
		if (JdbcDebugCfg.entryActive) debug[methodId_skip].methodEntry();
		try
		{
			long bytesToSkip;
			int noOfChunks = 0;
			int remBytes;
			long retLen = -1;
			long bytesSkipped = 0;
			int oldChunkNo;

			if (isClosed_)
				throw new IOException("Input stream is in closed state");
			if (n <= 0)
				throw new IOException("Invalid input Value");
			if (currentByte_ + n > bytesRead_)
			{
				bytesSkipped = bytesRead_ - currentByte_;
				bytesToSkip = n - bytesSkipped;
				currentByte_ += bytesSkipped;
			}
			else
			{
				currentByte_ += n;
				return n;
			}
			noOfChunks += (int)((bytesToSkip-1)/ lob_.chunkSize_);
			if ((bytesToSkip % lob_.chunkSize_) == 0)
				remBytes = lob_.chunkSize_;
			else
				remBytes = (int)(bytesToSkip % lob_.chunkSize_);
			oldChunkNo = currentChunkNo_;	// Which is already 1 more
			currentChunkNo_ = currentChunkNo_ + noOfChunks;
			retLen = readChunkThrowIO(null, 0, lob_.chunkSize_);
			if (retLen != -1)
			{
				bytesSkipped += (currentChunkNo_ - oldChunkNo - 1) * lob_.chunkSize_;
				if (retLen < remBytes)
					remBytes = (int)retLen;
				currentByte_ = remBytes;
				bytesSkipped += remBytes;
			}
			else
			{
				bytesSkipped += available();
				// Exclude the bytes that are in chunk already
				remBytes = (int)(bytesSkipped - (bytesRead_ - currentByte_));
				noOfChunks += (int)((remBytes-1) / lob_.chunkSize_);
				currentChunkNo_ = oldChunkNo + noOfChunks;
				//calculate the bytes in the chunk and set currentByte and bytesRead
				//to reach EOD
				if (remBytes == 0)
				{
					currentByte_ = 0;
					bytesRead_ = 0;
				}
				else
				{
					if ((remBytes % lob_.chunkSize_) == 0)
						currentByte_ = lob_.chunkSize_;
					else
						currentByte_ = (int)(remBytes % lob_.chunkSize_);
					bytesRead_ = currentByte_;
				}
			}
			return bytesSkipped;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_skip].methodExit();
		}
	}

	int readChunkThrowIO(byte[] b, int off, int len) throws IOException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_readChunkThrowIO].methodEntry();
		try
		{
			int readLen;
			try
			{
				readLen = readChunk(b, off, len);
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

	int readChunk(byte[] b, int off, int len) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_readChunk].methodEntry();
		try
		{
			int endChunkNo;
			byte[]	data;
			int copyLen;
			int	copyOffset;
			int readLen = 0;

			// The rows to read is calculated via ((len-1)/lob_.chunkSize_)
			endChunkNo = currentChunkNo_ + ((len-1)/lob_.chunkSize_);
			lob_.prepareGetLobDataStmt();
			PreparedStatement GetLobStmt = lob_.getGetLobDataStmt();

			if ((traceWriter_ != null) && 
				((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL)))
			{
				traceWriter_.println(getTraceId() 
					+ "readChunk(<byte>," + off + "," + len + ") - GetLobDataStmt params: tableName_=" + lob_.tableName_ 
					+ " dataLocator_=" + lob_.dataLocator_
					+ " currentChunkNo_=" + currentChunkNo_
					+ " endChunkNo=" + endChunkNo);
			}

			synchronized (GetLobStmt)
			{
				GetLobStmt.setString(1, lob_.tableName_);
				GetLobStmt.setLong(2, lob_.dataLocator_);
				GetLobStmt.setInt(3, currentChunkNo_);
				GetLobStmt.setInt(4, endChunkNo);
				ResultSet rs = GetLobStmt.executeQuery();
				copyLen = len;
				copyOffset = off;
				try 
				{
					while (rs.next())
					{
						data = rs.getBytes(1);
						currentChunkNo_++;
						bytesRead_ = data.length;
						if (b == null)
						{
							System.arraycopy(data, 0, chunk_, 0, data.length);
							readLen += data.length;
							currentByte_ = 0;
							break;				
						}
						else
						{
							if (copyLen >= data.length)
							{
								System.arraycopy(data, 0, b, copyOffset, data.length);
								copyLen -= data.length;
								readLen += data.length;
								copyOffset += data.length;
								currentByte_ = data.length;
							} 
							else
							{
								System.arraycopy(data, 0, b, copyOffset, copyLen);
								// copy the rest of data to chunk
								System.arraycopy(data, copyLen, chunk_, copyLen, data.length - copyLen);
								readLen += copyLen;
								currentByte_ = copyLen;
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
					+ "readChunk(<byte>," + off + "," + len + ") - LOB data read: bytesRead_=" + bytesRead_ 
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

	// Constructor
	SQLMXLobInputStream(SQLMXConnection connection, SQLMXLob lob)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXLobInputStream].methodEntry();
		try
		{
			lob_ = lob;
			conn_ = connection;
			chunk_ = new byte[lob_.chunkSize_];

			
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
	private String				traceId_;
	static PrintWriter	traceWriter_;
	static int			traceFlag_;
	SQLMXLob			lob_;
	SQLMXConnection		conn_;
	boolean				isClosed_;
	byte[]				chunk_;
	int					currentByte_;
	int					currentChunkNo_;
	int					bytesRead_;

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
