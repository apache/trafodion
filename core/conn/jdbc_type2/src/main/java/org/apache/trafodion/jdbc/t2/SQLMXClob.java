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
 * Filename    : SQLMXClob.java
 * Description : This program implements the java.sql.Clob interface
 *
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.InputStream;
import java.io.Reader;
import java.io.OutputStream;
import java.io.Writer;
import java.io.IOException;
import java.util.Date;
import java.io.PrintWriter;
import java.io.StringReader;

public class SQLMXClob extends SQLMXLob implements Clob 
{
	public InputStream getAsciiStream() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getAsciiStream].methodEntry();
		try
		{
			checkIfCurrent();
			// Close the reader and inputStream hander over earlier
			if (reader_ != null)
			{
				try
				{
					reader_.close();
				}
				catch (IOException e)
				{
				}
				finally
				{
					reader_ = null;
				}
			}
			return getInputStream();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getAsciiStream].methodExit();
		}
	}

	public Reader getCharacterStream() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getCharacterStream].methodEntry();
		try
		{
			// Close the reader and inputStream hander over earlier
			if (reader_ != null)
			{
				try
				{
					reader_.close();
				}
				catch (IOException e)
				{
				}
				finally
				{
					reader_ = null;
				}
			}
			if (inputStream_ != null)
			{
				try
				{
					inputStream_.close();
				}
				catch (IOException e)
				{
				}
				finally
				{
					inputStream_ = null;
				}
			}
			reader_ = new SQLMXClobReader(conn_, this);
			return reader_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getCharacterStream].methodExit();
		}
	}

	public String getSubString(long pos, int length) throws SQLException
	{
/*
		if (JdbcDebugCfg.entryActive) debug[methodId_getSubString].methodEntry();
		try
		{
			int startChunkNo;
			int endChunkNo;
			int offset;
			int copyLen;
			int dataLength;
			long clobDataLen;
			String data;
			StringBuffer retString;

			if (pos <= 0 || length < 0 )
			{
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "Clob.getSubString(long, int): position is less than or equal to 0, or length is less than 0";
				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
			}
			
			
			// Clob data total length must be larger than pos supplied (used to offset the substring)
//			clobDataLen = length();
//			if (pos > clobDataLen) 
//			{
//				Object[] messageArguments = new Object[1];
//				messageArguments[0] = "Clob.getSubString(long, int): position (" + pos + ") exceeds the Clob data length (" + clobDataLen + ")";
//				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
//			}
			
			
			checkIfCurrent();
			startChunkNo = (int)((pos-1) / chunkSize_);
			endChunkNo = (int)((pos-1+length)/ chunkSize_);
			copyLen = length;
			offset = (int)((pos-1) % chunkSize_);
			retString = new StringBuffer(length);
		
			if ((traceWriter_ != null) && 
				((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL)))
			{
				traceWriter_.println(getTraceId() 
					+ "getSubString(" + pos + "," + length + ") - GetLobDataStmt params: tableName_=" + tableName_ 
					+ " dataLocator_=" + dataLocator_
					+ " startChunkNo=" + startChunkNo
					+ " endChunkNo=" + endChunkNo);
			}

			synchronized (conn_.LobPrepStmts[SQLMXConnection.CLOB_GET_LOB_DATA_STMT])
			{
				conn_.LobPrepStmts[SQLMXConnection.CLOB_GET_LOB_DATA_STMT].setString(1, tableName_);
				conn_.LobPrepStmts[SQLMXConnection.CLOB_GET_LOB_DATA_STMT].setLong(2, dataLocator_);
				conn_.LobPrepStmts[SQLMXConnection.CLOB_GET_LOB_DATA_STMT].setInt(3, startChunkNo);
				conn_.LobPrepStmts[SQLMXConnection.CLOB_GET_LOB_DATA_STMT].setInt(4, endChunkNo);
				ResultSet rs = conn_.LobPrepStmts[SQLMXConnection.CLOB_GET_LOB_DATA_STMT].executeQuery();
				try
				{
					while (rs.next())
					{
						data = rs.getString(1);
						dataLength = data.length()-offset;
						
						if (dataLength >= copyLen)
						{
							retString.append(data.substring(offset, offset+copyLen));
							break;
						} 
						else
						{
							if (offset == 0)
								retString.append(data);
							else
								retString.append(data.substring(offset));
							copyLen -= dataLength;
						}
						offset = 0;	// reset the offset 
					}
				}
				finally
				{
					rs.close();
				}
			}
			return retString.toString();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getSubString].methodExit();
		}
*/
		return null;
	}

	public long position(Clob searchstr, long start) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_position_LJ_clob].methodEntry();
		try
		{
			String searchString;
		
			if (start <= 0 )
			{
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "Clob.position(Clob, long)";
				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
			}
			checkIfCurrent();
			searchString = searchstr.getSubString(1L,(int)searchstr.length());
			return position(searchString, start);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_position_LJ_clob].methodExit();
		}
	}

	public long position(String searchstr, long start) throws SQLException
	{
/*
		if (JdbcDebugCfg.entryActive) debug[methodId_position_LJ_str].methodEntry();
		try
		{
			String clobData;
			long retValue;

			if (start <= 0 )
			{
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "Clob.position(String, long)";
				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
			}
			clobData = getSubString(start, (int)length());
			retValue = clobData.indexOf(searchstr);
			if (retValue != -1)
				retValue += start;
 
			return retValue;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_position_LJ_str].methodExit();
		}
*/
		return 0;
	}

	public OutputStream setAsciiStream(long pos) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setAsciiStream].methodEntry();
		try
		{
			checkIfCurrent();
			// Close the writer and OutputStream hander over earlier
			if (writer_ != null)
			{
				try
				{
					writer_.close();
				}
				catch (IOException e)
				{
				}
				finally
				{
					writer_ = null;
				}
			}
			return setOutputStream(pos);

		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setAsciiStream].methodExit();
		}
	}
	
	public Writer setCharacterStream(long pos) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setCharacterStream].methodEntry();
		try
		{
			// Check if Autocommit is set, and no external transaction exists
			checkAutoCommitExtTxn();
			checkIfCurrent();
			// Close the writer and OutputStream hander over earlier
			if (writer_ != null)
			{
				try
				{
					writer_.close();
				}
				catch (IOException e)
				{
				}
				finally
				{
					writer_ = null;
				}
			}
			if (outputStream_ != null)
			{
				try
				{
					outputStream_.close();
				}
				catch (IOException e)
				{
				}
				finally
				{
					outputStream_ = null;
				}
			}
			writer_ = new SQLMXClobWriter(conn_, this, pos);
			return writer_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setCharacterStream].methodExit();
		}
	}

	public int setString(long pos, String str) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setString_JL].methodEntry();
		try
		{
			if (str == null || pos > 1) {
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "Clob.setString(long, String)";
				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
			}
			return setString(pos, str, 0, str.length());
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setString_JL].methodExit();
		}
	}

	public int setString(long pos, String str, int offset, int len) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setString_JLII].methodEntry();
		try
		{
			checkIfCurrent();
			if (str == null || pos > 1  || len < 0 || offset < 0) {
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "Clob.setString(long, String)";
				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
			}
			ir_ = new StringReader(str);	
			irLength_ = len;
			try {
				ir_.skip(offset);
			} catch (IOException ioe) {
				throw new SQLException(ioe);
			}
			populate();
			return len-offset;   
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setString_JLII].methodExit();
		}
	}

	void close() throws SQLException 
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_close].methodEntry();
		try
		{
			try
			{
				if (reader_ != null)
					reader_.close();
				if (writer_ != null)
					writer_.close();
				super.close();
			}
			catch (IOException e)
			{
				throw new SQLException(e);
			}
			finally
			{
				reader_ = null;
				writer_ = null;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_close].methodExit();
		}
	}

	// This function populates the Clob data from one of the following:
	// 1. InputStream set in PreparedStatement.setAsciiStream 
	// 2. Reader set in PreparedStatement.setCharacterStream
	// 3. From another clob set in PreparedStatement.setClob or ResultSet.updateClob
	// This function is called at the time of PreparedStatement.executeUpdate, execute and 
	// executeBatch

	void populate() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_populate].methodEntry();
		try
		{
			SQLMXLobOutputStream os;
			SQLMXClobWriter cw;

			if (inputLob_ != null) {
				is_ = inputLob_.getAsciiStream();
				ir_ = inputLob_.getCharacterStream();
			}
			else if (inputLobStr_ != null) {
				ir_  = new StringReader(inputLobStr_);
			}
			if (is_ != null)
			{
				os = (SQLMXLobOutputStream)setOutputStream(1);
				os.populate(is_, isLength_);
			}
			else if (ir_ != null)
			{
				cw = (SQLMXClobWriter)setCharacterStream(1);
				cw.populate(ir_, irLength_);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_populate].methodExit();
		}
	}
	
	// Constructors
	SQLMXClob(SQLMXConnection connection, String lobLocator) throws SQLException
	{
		super(connection, lobLocator, false);
	}

	SQLMXClob(SQLMXConnection connection, String lobLocator, InputStream x, long length) throws SQLException
	{
		super(connection, lobLocator, x, length, false);
	}

	SQLMXClob(SQLMXConnection connection, String lobLocator, Reader x, long length) throws SQLException
	{
		super(connection, lobLocator, false);
		ir_ = x;
		irLength_ = length;
	}
	
	SQLMXClob(SQLMXConnection connection, String lobLocator, Clob inputLob) throws SQLException
	{
		super(connection, lobLocator, false);
		inputLob_ = inputLob;
	}

	SQLMXClob(SQLMXConnection connection, String lobLocator, String lobStr) throws SQLException
	{
		super(connection, lobLocator, false);
		inputLobStr_ = lobStr;
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

	//fields
	private String		traceId_;
	static PrintWriter	traceWriter_;
	static int		traceFlag_;
	SQLMXClobReader		reader_;
	SQLMXClobWriter		writer_;
	Reader			ir_;
	long			irLength_;
	Clob			inputLob_;
	String			inputLobStr_;
	long			startingPos_;	

	private static int methodId_getAsciiStream			=  0;
	private static int methodId_getCharacterStream		=  1;
	private static int methodId_getSubString			=  2;
	private static int methodId_position_LJ_clob		=  3;
	private static int methodId_position_LJ_str			=  4;
	private static int methodId_setAsciiStream			=  5;
	private static int methodId_setCharacterStream		=  6;
	private static int methodId_setString_JL			=  7;
	private static int methodId_setString_JLII			=  8;
	private static int methodId_close					=  9;
	private static int methodId_populate				= 10;
	private static int methodId_populateFromClob		= 11;
	private static int methodId_SQLMXClob_LLJ			= 12;
	private static int methodId_SQLMXClob_LLJLI_stream	= 13;
	private static int methodId_SQLMXClob_LLJLI_reader	= 14;
	private static int methodId_SQLMXClob_LLJL_clob		= 15;
	private static int methodId_SQLMXClob_LLJL_string	= 16;
	private static int methodId_prepareGetLobLenStmt	= 17;
	private static int methodId_prepareDelLobDataStmt	= 18;
	private static int methodId_prepareGetLobDataStmt	= 19;
	private static int methodId_prepareUpdLobDataStmt	= 20;
	private static int methodId_prepareInsLobDataStmt	= 21;
	private static int methodId_prepareTrunLobDataStmt	= 22;
	private static int methodId_getGetLobLenStmt		= 23;
	private static int methodId_getDelLobDataStmt		= 24;
	private static int methodId_getTrunLobDataStmt		= 25;
	private static int methodId_getInsLobDataStmt		= 26;
	private static int methodId_getUpdLobDataStmt		= 27;
	private static int methodId_getGetLobDataStmt		= 28;
	private static int totalMethodIds					= 29;
	private static JdbcDebug[] debug;
	
	static
	{
		String className = "SQLMXClob";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_getAsciiStream] = new JdbcDebug(className,"getAsciiStream");
			debug[methodId_getCharacterStream] = new JdbcDebug(className,"getCharacterStream");
			debug[methodId_getSubString] = new JdbcDebug(className,"getSubString");
			debug[methodId_position_LJ_clob] = new JdbcDebug(className,"position[LJ_clob]");
			debug[methodId_position_LJ_str] = new JdbcDebug(className,"position[LJ_str]");
			debug[methodId_setAsciiStream] = new JdbcDebug(className,"setAsciiStream");
			debug[methodId_setCharacterStream] = new JdbcDebug(className,"setCharacterStream");
			debug[methodId_setString_JL] = new JdbcDebug(className,"setString[JL]");
			debug[methodId_setString_JLII] = new JdbcDebug(className,"setString[JLII]");
			debug[methodId_close] = new JdbcDebug(className,"close");
			debug[methodId_populate] = new JdbcDebug(className,"populate");
			debug[methodId_populateFromClob] = new JdbcDebug(className,"populateFromClob");
			debug[methodId_SQLMXClob_LLJ] = new JdbcDebug(className,"SQLMXClob[LLJ]");
			debug[methodId_SQLMXClob_LLJLI_stream] = new JdbcDebug(className,"SQLMXClob[LLJLI_stream]");
			debug[methodId_SQLMXClob_LLJLI_reader] = new JdbcDebug(className,"SQLMXClob[LLJLI_reader]");
			debug[methodId_SQLMXClob_LLJL_clob] = new JdbcDebug(className,"SQLMXClob[LLJL_clob]");
			debug[methodId_SQLMXClob_LLJL_string] = new JdbcDebug(className,"SQLMXClob[LLJL_string]");
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

	public Reader getCharacterStream(long pos, long length) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
}
