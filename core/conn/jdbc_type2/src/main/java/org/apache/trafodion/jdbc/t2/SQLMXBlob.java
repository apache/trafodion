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
 * Filename		: SQLMXBlob.java
 * Description	: This program implements the java.sql.Blob interface
 *
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.InputStream;
import java.io.Reader;
import java.io.OutputStream;
import java.io.Writer;
import java.util.Date;
import java.io.PrintWriter;

public class SQLMXBlob extends SQLMXLob implements Blob 
{
	public InputStream getBinaryStream() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getBinaryStream].methodEntry();
		try
		{
			checkIfCurrent();
			return getInputStream();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getBinaryStream].methodExit();
		}
	}


	public byte[] getBytes(long pos, int length) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getBytes].methodEntry();
		try
		{
			int startChunkNo;
			int endChunkNo;
			int offset;
			int copyLen;
			int copyOffset;
			int dataLength;
			int readLen;
			long blobDataLen;
			byte[] data;
			byte[] b;
			byte[] b1;

			if (pos <= 0 || length < 0 )
			{
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "Blob.getBytes(long, int): position is less than or equal to 0, or length is less than 0";
				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
			}

			// Blob data total length must be larger than pos supplied (used to offset the bytes)
			blobDataLen = length();
			if (pos > blobDataLen) 
			{
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "Blob.getBytes(long, int): position (" + pos + ") exceeds the Blob data length (" + blobDataLen + ")";
				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
			}

			checkIfCurrent();
			startChunkNo = (int)((pos-1) / chunkSize_);
			endChunkNo = (int)((pos-1+length)/ chunkSize_);
			copyLen = length;
			offset = (int)((pos-1) % chunkSize_);
			copyOffset= 0;
			readLen = 0;
			b = new byte[length];
			prepareGetLobDataStmt();

			if ((traceWriter_ != null) && 
				((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL)))
			{
				traceWriter_.println(getTraceId() 
					+ "getBytes(" + pos + "," + length + ") - GetLobDataStmt params: tableName_=" + tableName_ 
					+ " dataLocator_=" + dataLocator_
					+ " startChunkNo=" + startChunkNo
					+ " endChunkNo=" + endChunkNo);
			}

			synchronized (conn_.LobPrepStmts[SQLMXConnection.BLOB_GET_LOB_DATA_STMT])
			{
				conn_.LobPrepStmts[SQLMXConnection.BLOB_GET_LOB_DATA_STMT].setString(1, tableName_);
				conn_.LobPrepStmts[SQLMXConnection.BLOB_GET_LOB_DATA_STMT].setLong(2, dataLocator_);
				conn_.LobPrepStmts[SQLMXConnection.BLOB_GET_LOB_DATA_STMT].setInt(3, startChunkNo);
				conn_.LobPrepStmts[SQLMXConnection.BLOB_GET_LOB_DATA_STMT].setInt(4, endChunkNo);
				ResultSet rs = conn_.LobPrepStmts[SQLMXConnection.BLOB_GET_LOB_DATA_STMT].executeQuery();
				try
				{
					while (rs.next())
					{
						data = rs.getBytes(1);
						dataLength = data.length-offset;
						
						if (dataLength >= copyLen)
						{
							System.arraycopy(data, offset, b, copyOffset, copyLen);
							readLen += copyLen;
							break;
						} 
						else
						{
							System.arraycopy(data, offset, b, copyOffset, dataLength);
							copyLen -= dataLength;
							copyOffset += dataLength;
							readLen += dataLength;
						}
						offset = 0;	// reset the offset 
					}
				}
				finally
				{
					rs.close();
				}
			}
			if (readLen == length)
				return b;
			else
			{
				b1 = new byte[readLen];
				System.arraycopy(b, 0, b1, 0, readLen);
				return b1;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getBytes].methodExit();
		}
	}

	public long position(Blob pattern, long start) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_position_LJ].methodEntry();
		try
		{
			byte[] searchPattern;
		
			if (start <= 0 )
			{
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "Blob.position(Blob, long)";
				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
			}
			checkIfCurrent();
			searchPattern = pattern.getBytes(1L,(int)pattern.length());
			return position(searchPattern, start);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_position_LJ].methodExit();
		}
	}

	public long position(byte[] pattern, long start) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_position_BJ].methodEntry();
		try
		{
			byte[] blobData;
			long retValue;

			if (start <= 0 )
			{
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "Blob.position(byte[], long)";
				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
			}
			checkIfCurrent();
			blobData = getBytes(start, (int)length());
			retValue = findBytes(blobData, 0, blobData.length, pattern);
			if (retValue != -1)
				retValue += start;

			return retValue;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_position_BJ].methodExit();
		}
	}


	public OutputStream setBinaryStream(long pos) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setBinaryStream].methodEntry();
		try
		{
			// Check if Autocommit is set, and no external transaction exists
			checkAutoCommitExtTxn();
			checkIfCurrent();
			return setOutputStream(pos);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setBinaryStream].methodExit();
		}
	}

	public int setBytes(long pos, byte[] bytes) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setBytes_JB].methodEntry();
		try
		{
			if (bytes == null)	
			{
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "Blob.setBytes(long, byte[])";
				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
			}
			return setBytes(pos, bytes, 0, bytes.length);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setBytes_JB].methodExit();
		}
	}

	public int setBytes(long pos, byte[] bytes, int offset, int len) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setBytes_JBII].methodEntry();
		try
		{
			int endChunkNo;
			int updOffset;
			int updLen;
			int	chunkNo;
			long lobLenForUpd;
			int	 byteOffset;
			int retLen;
			int totalRetLen;
			int copyLen;
			long remLen;
			long lobLen;

			byte [] tempChunk = null;

			if (pos <= 0 || len < 0 || offset < 0 || bytes == null) 
			{
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "Blob.setBytes(long, byte[], int, int)";
				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
			}
			checkIfCurrent();
			lobLen = length();
			if (pos > lobLen+1)
				throw Messages.createSQLException(conn_.locale_,"invalid_position_value", null);
			copyLen = len;
			remLen = pos-1+len;	// Length to be either updated or inserted
			byteOffset = offset;
			totalRetLen = 0;
			chunkNo = (int)((pos-1)/ chunkSize_);
			// calculate the length that can be updated rounded to chunk size
			if ((lobLen % chunkSize_) == 0)
				lobLenForUpd = (lobLen / chunkSize_) * chunkSize_;
			else
				lobLenForUpd = ((lobLen / chunkSize_)+1) * chunkSize_;
			if (remLen <= lobLenForUpd)
				updLen	= len;
			else
				updLen = (int)(lobLenForUpd - (pos-1));
			if (updLen > 0)
			{
				updOffset = (int)((pos-1) % chunkSize_);
				prepareUpdLobDataStmt();		

				synchronized (conn_.LobPrepStmts[SQLMXConnection.BLOB_UPD_LOB_DATA_STMT])
				{
					conn_.LobPrepStmts[SQLMXConnection.BLOB_UPD_LOB_DATA_STMT].setString(4, tableName_);
					conn_.LobPrepStmts[SQLMXConnection.BLOB_UPD_LOB_DATA_STMT].setLong(5, dataLocator_);
				
					while (true)
					{
						// String is 0 based while substring in SQL/MX is 1 based, hence +1
						conn_.LobPrepStmts[SQLMXConnection.BLOB_UPD_LOB_DATA_STMT].setInt(6, chunkNo);
						conn_.LobPrepStmts[SQLMXConnection.BLOB_UPD_LOB_DATA_STMT].setInt(1, updOffset);
						if ((updOffset + updLen) <= chunkSize_)
						{
							conn_.LobPrepStmts[SQLMXConnection.BLOB_UPD_LOB_DATA_STMT].setInt(3, updOffset + updLen + 1);
							if ((byteOffset == 0) && (updLen - updOffset == bytes.length))
							{
								conn_.LobPrepStmts[SQLMXConnection.BLOB_UPD_LOB_DATA_STMT].setBytes(2, bytes);
							}
							else
							{
								tempChunk = new byte[updLen];
								System.arraycopy(bytes, byteOffset, tempChunk, 0, updLen);
								conn_.LobPrepStmts[SQLMXConnection.BLOB_UPD_LOB_DATA_STMT].setBytes(2, tempChunk);
							}
							conn_.LobPrepStmts[SQLMXConnection.BLOB_UPD_LOB_DATA_STMT].executeUpdate();
							totalRetLen += updLen;
							byteOffset += updLen;
							chunkNo++;
							break;
						}
						else
						{
							conn_.LobPrepStmts[SQLMXConnection.BLOB_UPD_LOB_DATA_STMT].setInt(3, chunkSize_+1);
							if (tempChunk == null || tempChunk.length != chunkSize_-updOffset)
								tempChunk = new byte[chunkSize_-updOffset];
							System.arraycopy(bytes, byteOffset, tempChunk, 0, chunkSize_-updOffset);
							conn_.LobPrepStmts[SQLMXConnection.BLOB_UPD_LOB_DATA_STMT].setBytes(2, tempChunk);
							conn_.LobPrepStmts[SQLMXConnection.BLOB_UPD_LOB_DATA_STMT].executeUpdate();
							totalRetLen += (chunkSize_-updOffset);
							byteOffset += (chunkSize_-updOffset);
							updLen -= (chunkSize_-updOffset);
							chunkNo++;
						}
						updOffset = 0;
					}
				}
				copyLen = (int)(remLen - lobLenForUpd);
				
				if ((traceWriter_ != null) && 
					((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL)))
				{
					traceWriter_.println(getTraceId() 
						+ "setBytes(" + pos + ",<bytes>," + offset + "," + len 
						+ ") - UpdLobDataStmt params: tableName_=" + tableName_ 
						+ " dataLocator_=" + dataLocator_ + " chunkNo=" + chunkNo
						+ " updOffset=" + updOffset + " updLen=" + updLen
						+ " remLen=" + remLen + " lobLenForUpd=" + lobLenForUpd 
						+ " byteOffset=" + byteOffset + " totalRetLen=" + totalRetLen);
				}
			}

			tempChunk = null;
			if (remLen > lobLenForUpd)
			{
				while (true)
				{
					prepareInsLobDataStmt();

					synchronized (conn_.LobPrepStmts[SQLMXConnection.BLOB_INS_LOB_DATA_STMT])
					{
						conn_.LobPrepStmts[SQLMXConnection.BLOB_INS_LOB_DATA_STMT].setString(1, tableName_);
						conn_.LobPrepStmts[SQLMXConnection.BLOB_INS_LOB_DATA_STMT].setLong(2, dataLocator_);
						conn_.LobPrepStmts[SQLMXConnection.BLOB_INS_LOB_DATA_STMT].setInt(3, chunkNo);
						if (copyLen <= chunkSize_)
						{
							if (byteOffset == 0 && copyLen == bytes.length)
								conn_.LobPrepStmts[SQLMXConnection.BLOB_INS_LOB_DATA_STMT].setBytes(4, bytes);
							else
							{
								tempChunk = new byte[copyLen];
								System.arraycopy(bytes, byteOffset, tempChunk, 0, copyLen);
								conn_.LobPrepStmts[SQLMXConnection.BLOB_INS_LOB_DATA_STMT].setBytes(4, tempChunk);
							}
							conn_.LobPrepStmts[SQLMXConnection.BLOB_INS_LOB_DATA_STMT].executeUpdate();
							totalRetLen += copyLen;
							break;
						}
						else
						{
							if (tempChunk == null)
								tempChunk = new byte[chunkSize_];
							System.arraycopy(bytes, byteOffset, tempChunk, 0, chunkSize_);
							conn_.LobPrepStmts[SQLMXConnection.BLOB_INS_LOB_DATA_STMT].setBytes(4, tempChunk);
							conn_.LobPrepStmts[SQLMXConnection.BLOB_INS_LOB_DATA_STMT].executeUpdate();
							byteOffset += chunkSize_;
							copyLen -= chunkSize_;
							totalRetLen += chunkSize_;
						}
						chunkNo++;
					}
				}
				
				if ((traceWriter_ != null) && 
					((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL)))
				{
					traceWriter_.println(getTraceId() 
						+ "setBytes(" + pos + ",<bytes>," + offset + "," + len 
						+ ") - InsLobDataStmt params: tableName_=" + tableName_ 
						+ " dataLocator_=" + dataLocator_ + " (total)chunkNo=" + chunkNo
						+ " copyLen=" + copyLen + " byteOffset=" + byteOffset 
						+ " totalRetLen=" + totalRetLen);
				}
			}
			return totalRetLen;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setBytes_JBII].methodExit();
		}
	}

	// This function populates the Blob data from one of the following:
	// 1. InputStream set in PreparedStatement.setBinaryStream
	// 2. From another clob set in PreparedStatement.setBlob or ResultSet.updateBlob
	// This function is called at the time of PreparedStatement.executeUpdate, execute and 
	// executeBatch

	void populate() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_populate].methodEntry();
		try
		{
			SQLMXLobOutputStream os;

			if (is_ != null)
			{
				os = (SQLMXLobOutputStream)setOutputStream(1);
				os.populate(is_, isLength_);
				is_ = null;
			}
			else if (inputLob_ != null)
			{	
				populateFromBlob();
				inputLob_ = null;			
			}
			else if (b_ != null)
			{
				setBytes(1, b_);
				b_ = null;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_populate].methodExit();
		}
	}

	void populateFromBlob() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_populateFromBlob].methodEntry();
		try
		{
			long		pos;
			byte[]		b;
			int			ret;
			ResultSet	rs;
			SQLMXBlob	inputBlob;
			int			chunkNo = 0;
		
			pos = 1;
			if (inputLob_ instanceof SQLMXBlob)
			{
				// When SQL/MX supports insert into a table by selecting some other rows in
				// the same table, we should change the code to do so
				// Until then, we read a row and write to the same table with different
				// data locator till all the rows are read 
				inputBlob = (SQLMXBlob)inputLob_;
			
				prepareGetLobDataStmt();
				prepareInsLobDataStmt();

				if ((traceWriter_ != null) && 
					((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL)))
				{
					traceWriter_.println(getTraceId() 
						+ "populateFromBlob() - GetLobDataStmt params: tableName_=" + inputBlob.tableName_ 
						+ " dataLocator_=" + inputBlob.dataLocator_ + " chunkNo=0");
				}

				synchronized (conn_.LobPrepStmts[SQLMXConnection.BLOB_GET_LOB_DATA_STMT])
				{
					conn_.LobPrepStmts[SQLMXConnection.BLOB_GET_LOB_DATA_STMT].setString(1, inputBlob.tableName_);
					conn_.LobPrepStmts[SQLMXConnection.BLOB_GET_LOB_DATA_STMT].setLong(2, inputBlob.dataLocator_);
					conn_.LobPrepStmts[SQLMXConnection.BLOB_GET_LOB_DATA_STMT].setInt(3, 0);	// start ChunkNo
					conn_.LobPrepStmts[SQLMXConnection.BLOB_GET_LOB_DATA_STMT].setInt(4, Integer.MAX_VALUE);
					rs = conn_.LobPrepStmts[SQLMXConnection.BLOB_GET_LOB_DATA_STMT].executeQuery();
					try
					{
						synchronized(conn_.LobPrepStmts[SQLMXConnection.BLOB_INS_LOB_DATA_STMT])
						{
							conn_.LobPrepStmts[SQLMXConnection.BLOB_INS_LOB_DATA_STMT].setString(1, tableName_);
							conn_.LobPrepStmts[SQLMXConnection.BLOB_INS_LOB_DATA_STMT].setLong(2, dataLocator_);
		
							while (rs.next())
							{
								b = rs.getBytes(1);
								conn_.LobPrepStmts[SQLMXConnection.BLOB_INS_LOB_DATA_STMT].setInt(3, chunkNo);
								conn_.LobPrepStmts[SQLMXConnection.BLOB_INS_LOB_DATA_STMT].setBytes(4, b);
								conn_.LobPrepStmts[SQLMXConnection.BLOB_INS_LOB_DATA_STMT].executeUpdate();
								chunkNo++;
							}
						}		
						
						if ((traceWriter_ != null) && 
							((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL)))
						{
							traceWriter_.println(getTraceId() 
								+ "populateFromBlob() - InsLobDataStmt params: tableName_=" + tableName_ 
								+ " dataLocator_=" + dataLocator_ + " (total)chunkNo=" + chunkNo);
						}
					} 
					finally 
					{
						rs.close();
					}
				}
			}
			else
			{
				while (true)
				{
					b = inputLob_.getBytes(pos, chunkSize_);
					if (b.length == 0)
						break;
					ret = setBytes(pos, b);
					pos += b.length;
				}
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_populateFromBlob].methodExit();
		}
	}

	static final int findBytes(byte buf[], int off, int len, byte ptrn[])
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_findBytes].methodEntry();
		try
		{
			int buf_len = off + len;
			int ptrn_len = ptrn.length;
			int i;					   // index into buf
			int j;					   // index into ptrn;
			byte b = ptrn[0];			// next byte of interest

			for (i = off; i < buf_len; )
			{
				j = 0;
				while (i < buf_len && j < ptrn_len && buf[i] == ptrn[j])
				{
					i++;
					j++;
				}
				if (i == buf_len || j == ptrn_len) return i - j;
				else
				{
					// We have to go back a bit as there may be an overlapping
					// match starting a bit later in buf...
					i = i - j + 1;
				}
			}
			return -1;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_findBytes].methodExit();
		}
	}

	// The following methods are used to prepare the LOB statement specific 
	// to BLOB objects, and re-prepares if the lobTableName_ has changed. 
	void prepareGetLobLenStmt() throws SQLException 
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_prepareGetLobLenStmt].methodEntry();
		try
		{
			conn_.prepareGetLobLenStmt(lobTableName_,true);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_prepareGetLobLenStmt].methodExit();
		}
	}

	void prepareDelLobDataStmt() throws SQLException 
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_prepareDelLobDataStmt].methodEntry();
		try
		{
			conn_.prepareDelLobDataStmt(lobTableName_,true);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_prepareDelLobDataStmt].methodExit();
		}
	}
	
	void prepareGetLobDataStmt() throws SQLException 
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_prepareGetLobDataStmt].methodEntry();
		try
		{
			conn_.prepareGetLobDataStmt(lobTableName_,true);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_prepareGetLobDataStmt].methodExit();
		}
	}
	
	void prepareUpdLobDataStmt() throws SQLException 
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_prepareUpdLobDataStmt].methodEntry();
		try
		{
			conn_.prepareUpdLobDataStmt(lobTableName_,true);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_prepareUpdLobDataStmt].methodExit();
		}
	}
	
	void prepareInsLobDataStmt() throws SQLException 
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_prepareInsLobDataStmt].methodEntry();
		try
		{
			conn_.prepareInsLobDataStmt(lobTableName_,true);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_prepareInsLobDataStmt].methodExit();
		}
	}
	
	void prepareTrunLobDataStmt() throws SQLException 
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_prepareTrunLobDataStmt].methodEntry();
		try
		{
			conn_.prepareTrunLobDataStmt(lobTableName_,true);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_prepareTrunLobDataStmt].methodExit();
		}
	}
	
	// The following methods are used to return the BLOB prepared statement 
	// from the connection object PS array for population and execution.
	PreparedStatement getGetLobLenStmt()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getGetLobLenStmt].methodEntry();
		try
		{
			return conn_.LobPrepStmts[SQLMXConnection.BLOB_GET_LOB_LEN_STMT];
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getGetLobLenStmt].methodExit();
		}
	}
	
	PreparedStatement getDelLobDataStmt()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getDelLobDataStmt].methodEntry();
		try
		{
			return conn_.LobPrepStmts[SQLMXConnection.BLOB_DEL_LOB_DATA_STMT];
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getDelLobDataStmt].methodExit();
		}
	}
	
	PreparedStatement getTrunLobDataStmt()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getTrunLobDataStmt].methodEntry();
		try
		{
			return conn_.LobPrepStmts[SQLMXConnection.BLOB_TRUN_LOB_DATA_STMT];
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getTrunLobDataStmt].methodExit();
		}
	}
	
	PreparedStatement getInsLobDataStmt()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getInsLobDataStmt].methodEntry();
		try
		{
			return conn_.LobPrepStmts[SQLMXConnection.BLOB_INS_LOB_DATA_STMT];
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getInsLobDataStmt].methodExit();
		}
	}
	
	PreparedStatement getUpdLobDataStmt()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getUpdLobDataStmt].methodEntry();
		try
		{
			return conn_.LobPrepStmts[SQLMXConnection.BLOB_UPD_LOB_DATA_STMT];
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getUpdLobDataStmt].methodExit();
		}
	}
	
	PreparedStatement getGetLobDataStmt()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getGetLobDataStmt].methodEntry();
		try
		{
			return conn_.LobPrepStmts[SQLMXConnection.BLOB_GET_LOB_DATA_STMT];
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getGetLobDataStmt].methodExit();
		}
	}


	// Constructors
	SQLMXBlob(SQLMXConnection connection, String tableName, long dataLocator) throws SQLException
	{
		super(connection, tableName, dataLocator, connection.blobTableName_, true);
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXBlob_LLJ].methodEntry();
		try
		{
			if (connection.blobTableName_ == null)
				throw Messages.createSQLException(conn_.locale_,"no_blobTableName", null);
		
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXBlob_LLJ].methodExit();
		}
	}

	SQLMXBlob(SQLMXConnection connection, String tableName, long dataLocator, InputStream x, 
			int length) throws SQLException
	{
		super(connection, tableName, dataLocator, x, length, connection.blobTableName_, true);
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXBlob_LLJLI].methodEntry();
		try
		{
			if (connection.blobTableName_ == null)
				throw Messages.createSQLException(conn_.locale_,"no_blobTableName", null);
	
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXBlob_LLJLI].methodExit();
		}
	}

	SQLMXBlob(SQLMXConnection connection, String tableName, long dataLocator, Blob inputLob) throws SQLException
	{
		super(connection, tableName, dataLocator, connection.blobTableName_, true);
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXBlob_LLJL].methodEntry();
		try
		{
			if (connection.blobTableName_ == null)
				throw Messages.createSQLException(conn_.locale_,"no_blobTableName", null);
			inputLob_ = inputLob;
	
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXBlob_LLJL].methodExit();
		}
	}
	
	SQLMXBlob(SQLMXConnection connection, String tableName, long dataLocator, byte[] b)
		throws SQLException
	{
		super(connection, tableName, dataLocator, connection.blobTableName_, true);
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXBlob_LLJB].methodEntry();
		try
		{
			if (connection.blobTableName_ == null)
				throw Messages.createSQLException(conn_.locale_,"no_blobTableName", null);
			b_ = b;
	
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXBlob_LLJB].methodExit();
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

	// fields
	private String					traceId_;
	static PrintWriter		traceWriter_;
	static int				traceFlag_;
	Blob			inputLob_;
	byte[]			b_;

	private static int methodId_getBinaryStream			=  0;
	private static int methodId_getBytes				=  1;
	private static int methodId_position_LJ				=  2;
	private static int methodId_position_BJ				=  3;
	private static int methodId_setBinaryStream			=  4;
	private static int methodId_setBytes_JB				=  5;
	private static int methodId_setBytes_JBII			=  6;
	private static int methodId_populate				=  7;
	private static int methodId_populateFromBlob		=  8;
	private static int methodId_findBytes				=  9;
	private static int methodId_SQLMXBlob_LLJ			= 10;
	private static int methodId_SQLMXBlob_LLJLI			= 11;
	private static int methodId_SQLMXBlob_LLJL			= 12;
	private static int methodId_SQLMXBlob_LLJB			= 13;
	private static int methodId_prepareGetLobLenStmt	= 14;
	private static int methodId_prepareDelLobDataStmt	= 15;
	private static int methodId_prepareGetLobDataStmt	= 16;
	private static int methodId_prepareUpdLobDataStmt	= 17;
	private static int methodId_prepareInsLobDataStmt	= 18;
	private static int methodId_prepareTrunLobDataStmt	= 19;
	private static int methodId_getGetLobLenStmt		= 20;
	private static int methodId_getDelLobDataStmt		= 21;
	private static int methodId_getTrunLobDataStmt		= 22;
	private static int methodId_getInsLobDataStmt		= 23;
	private static int methodId_getUpdLobDataStmt		= 24;
	private static int methodId_getGetLobDataStmt		= 25;
	private static int totalMethodIds					= 26;
	private static JdbcDebug[] debug;
	
	static
	{
		String className = "SQLMXBlob";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_getBinaryStream] = new JdbcDebug(className,"getBinaryStream");
			debug[methodId_getBytes] = new JdbcDebug(className,"getBytes");
			debug[methodId_position_LJ] = new JdbcDebug(className,"position[LJ]");
			debug[methodId_position_BJ] = new JdbcDebug(className,"position[BJ]");
			debug[methodId_setBinaryStream] = new JdbcDebug(className,"setBinaryStream");
			debug[methodId_setBytes_JB] = new JdbcDebug(className,"setBytes[JB]");
			debug[methodId_setBytes_JBII] = new JdbcDebug(className,"setBytes[JBII]");
			debug[methodId_populate] = new JdbcDebug(className,"populate");
			debug[methodId_populateFromBlob] = new JdbcDebug(className,"populateFromBlob");
			debug[methodId_findBytes] = new JdbcDebug(className,"findBytes");
			debug[methodId_SQLMXBlob_LLJ] = new JdbcDebug(className,"SQLMXBlob[LLJ]");
			debug[methodId_SQLMXBlob_LLJLI] = new JdbcDebug(className,"SQLMXBlob[LLJLI]");
			debug[methodId_SQLMXBlob_LLJL] = new JdbcDebug(className,"SQLMXBlob[LLJL]");
			debug[methodId_SQLMXBlob_LLJB] = new JdbcDebug(className,"SQLMXBlob[LLJB]");
			debug[methodId_prepareGetLobLenStmt] = new JdbcDebug(className,"prepareGetLobLenStmt");
			debug[methodId_prepareDelLobDataStmt] = new JdbcDebug(className,"prepareDelLobDataStmt");
			debug[methodId_prepareGetLobDataStmt] = new JdbcDebug(className,"prepareGetLobDataStmt");
			debug[methodId_prepareUpdLobDataStmt] = new JdbcDebug(className,"prepareUpdLobDataStmt");
			debug[methodId_prepareInsLobDataStmt] = new JdbcDebug(className,"prepareInsLobDataStmt");
			debug[methodId_prepareTrunLobDataStmt] = new JdbcDebug(className,"prepareTrunLobDataStmt");
			debug[methodId_getGetLobLenStmt] = new JdbcDebug(className,"getGetLobLenStmt");
			debug[methodId_getDelLobDataStmt] = new JdbcDebug(className,"getDelLobDataStmt");
			debug[methodId_getTrunLobDataStmt] = new JdbcDebug(className,"getTrunLobDataStmt");
			debug[methodId_getInsLobDataStmt] = new JdbcDebug(className,"getInsLobDataStmt");
			debug[methodId_getUpdLobDataStmt] = new JdbcDebug(className,"getUpdLobDataStmt");
			debug[methodId_getGetLobDataStmt] = new JdbcDebug(className,"getGetLobDataStmt");
		}
	}

        public void free() throws SQLException {
                // TODO Auto-generated method stub

        }

        public InputStream getBinaryStream(long pos, long length)
                        throws SQLException {
                // TODO Auto-generated method stub
                return null;
        }

}
