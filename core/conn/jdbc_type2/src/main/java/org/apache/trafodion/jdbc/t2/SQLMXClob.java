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
import java.util.Arrays;

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
	 	long skippedLen;	
		checkIfCurrent();
		Reader cr = getCharacterStream();
		try {
 			skippedLen = cr.skip(pos);
			if (skippedLen < pos)
				return new String(""); 
			char[] buf = new char[length];
			int retLen = cr.read(buf, 0, length);
			if (retLen < length)
				buf = Arrays.copyOf(buf, retLen);
			return new String(buf);
		} catch (IOException ioe) {
			throw new SQLException(ioe);
		}
	}

	public long position(Clob searchstr, long start) throws SQLException
	{
		throw new SQLFeatureNotSupportedException("Clob.position(Clob, long) not supported");
	}

	public long position(String searchstr, long start) throws SQLException
	{
		throw new SQLFeatureNotSupportedException("Clob.position(String, long) not supported");
	}

	public OutputStream setAsciiStream(long pos) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setAsciiStream].methodEntry();
		try
		{
			if (pos < 0) {
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "Clob.setAsciiStream(long)";
				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
			}
			if (pos > 1)
				throw new SQLFeatureNotSupportedException("Clob.setAsciiStream with position > 1 is not supported");
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
			if (pos < 0) {
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "Clob.setCharacterStream(long)";
				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
			}
			if (pos > 1)
				throw new SQLFeatureNotSupportedException("Clob.setCharacterStream with position > 1 is not supported");

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
			if (str == null || pos < 0) {
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "Clob.setString(long, String)";
				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
			}
			if (pos > 1)
				throw new SQLFeatureNotSupportedException("Clob.setString with position > 1 is not supported");
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
			if (str == null || pos < 0 || len < 0 || offset < 0) {
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "Clob.setString(long, String, int, int)";
				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
			}
			if (pos > 1)
				throw new SQLFeatureNotSupportedException("Clob.setString with position > 1 is not supported");
			inputLobStr_ = str;
			startingPos_ = pos;	
			length_ = len;
			offset_ = offset;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setString_JLII].methodExit();
		}
		return len;
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

	long inLength() throws SQLException 
	{
		if (inputLobStr_ != null && length_ == 0)
			return inputLobStr_.length();
		else
			return super.inLength();
	}

	String getString(int inlineLobLen) throws SQLException 
        {
		long llength  = inLength();
		if (llength > Integer.MAX_VALUE) {
			Object[] messageArguments = new Object[1];
			messageArguments[0] = "Blob.getString(int)";
			throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
		}
		int length = (int)llength;
		if (length == 0) {
			if (inputLobStr_ != null && (inputLobStr_.length() - offset_)  > inlineLobLen)	
				return null;
			else
				return null;
		}
		else if (length_ > inlineLobLen)	
			return null;	
		if (inputLobStr_ != null) {
			if (offset_ == 0)
				return inputLobStr_;
			else
				return inputLobStr_.substring(offset_, length+offset_);
		}
		if (ir_ != null) {
			try {
				char[] cbuf = new char[length];
				int retLen = ir_.read(cbuf, offset_, length);
				if (retLen != length)
					return new String(cbuf, 0, retLen);
				else
					return new String(cbuf);
			} catch (IOException ioe) {
				throw new SQLException(ioe);
			}
		}
		if (is_ != null) {
			try {
				byte buf[] = new byte[length]; 
				int retLen = is_.read(buf, offset_, length);
				if (retLen != length)
					return new String(buf, 0, retLen);
				else
					return new String(buf);
			} catch (IOException ioe) {
				throw new SQLException(ioe);
			}
		}
		return null;
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
				try {
					if (offset_ > 0)
						ir_.skip(offset_);
				} catch (IOException ioe) {
					throw new SQLException(ioe);
				}
			}
			if (is_ != null)
			{
				os = (SQLMXLobOutputStream)setOutputStream(1);
				os.populate(is_, length_);
				close();
			}
			else if (ir_ != null)
			{
				cw = (SQLMXClobWriter)setCharacterStream(1);
				cw.populate(ir_, length_);
				close();
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_populate].methodExit();
		}
	}
	
	// Constructors
	public SQLMXClob(SQLMXConnection connection, String lobLocator) throws SQLException
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
		length_ = length;
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
	private static int totalMethodIds					= 17;
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
