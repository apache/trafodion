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
 * Filename    : SQLMXLob.java
 * Description : SQLMXClob and SQLMXBlob extends this class. Some of the 
 *     common methods are implemented here
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;
//import com.tandem.tmf.Current;	// Linux port - ToDo
import java.util.Date;
import java.io.PrintWriter;

public abstract class SQLMXLob
{
        long inLength() throws SQLException 
        {
		if (b_ != null && length_ == 0)
			return b_.length;
		else
			return length_;
        }

	public long length() throws SQLException
	{
		throw new SQLFeatureNotSupportedException("Clob or Blob.length() is not supported");
	}

	public void truncate(long len) throws SQLException
	{
		throw new SQLFeatureNotSupportedException("Clob or Blob.truncate(long) is not supported");
	}

	InputStream getInputStream() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getInputStream].methodEntry();
		try
		{
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
			inputStream_ = new SQLMXLobInputStream(conn_, this);
			return inputStream_;

		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getInputStream].methodExit();
		}
	}

	SQLMXLobOutputStream setOutputStream(long startingPos) throws SQLException
	{
		outputStream_ = new SQLMXLobOutputStream(conn_, startingPos, this);
		return outputStream_;
	}


	void close() throws SQLException
	{
		try {
			if (inputStream_ != null)
				inputStream_.close();
			isCurrent_ = false;
		} catch (IOException ioe) {
			throw new SQLException(ioe);
		}
	}


	static String convSQLExceptionToIO(SQLException e)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_convSQLExceptionToIO].methodEntry();
		try
		{
			SQLException e1;
			e1 = e;
			StringBuffer s = new StringBuffer(1000);
			do
			{
				s.append("SQLState :");
				s.append(e1.getSQLState());
				s.append(" ErrorCode :");
				s.append(e1.getErrorCode());
				s.append(" Message:");
				s.append(e1.getMessage());
			}
			while ((e1 = e1.getNextException()) != null);
			return s.toString();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_convSQLExceptionToIO].methodExit();
		}
	}

        void checkIfCurrent() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_checkIfCurrent].methodEntry();
		try
		{
			if (! isCurrent_)
			{
				Object[] messageArguments = new Object[1];
				messageArguments[0] = this;
				throw Messages.createSQLException(conn_.locale_, "lob_not_current",
					messageArguments);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_checkIfCurrent].methodExit();
		}
	}


// *******************************************************************
// * If Autocommit is enabled, and no external transaction exists, an
// * exception will be thrown. In this case, JDBC cannot play the role of
// * Autocommit (updating the base and lob tables in a single unit of work)
// * because we return an OutputStream or Writer object to the application,
// * who could hold it indefinitely. This is the case for
// * Clob.setAsciiStream, Clob.setCharacterStream, and Blob.setBinaryStream.
// *******************************************************************
	void checkAutoCommitExtTxn() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_checkAutoCommitExtTxn].methodEntry();
		try
		{
/* Linux port - ToDo com.tandem.util.FSException in tmf.jar
			Current tx = null;
			int txnState = -1;

			try
			{
				tx = new Current();
				txnState = tx.get_status();

				if (conn_.autoCommit_ && (txnState == tx.StatusNoTransaction))
				{
					throw Messages.createSQLException(conn_.locale_,"invalid_lob_commit_state", null);
				}
			}
			catch (com.tandem.util.FSException fe1)
			{
				Object[] messageArguments = new Object[2];
				messageArguments[0] = Short.toString(fe1.error);
				messageArguments[1] = fe1.getMessage();
				throw Messages.createSQLException(conn_.locale_, "transaction_error_update",
					messageArguments);
			}
*/
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_checkAutoCommitExtTxn].methodExit();
		}
	}

	void setLobLocator(String lobLocator)
	{
		lobLocator_ = lobLocator;
	}

	SQLMXLob(SQLMXConnection connection, String lobLocator, boolean isBlob) throws SQLException
	{
		lobLocator_ = lobLocator;
		conn_ = connection;
		isBlob_ = isBlob;
		chunkSize_ = 16*1024;
		isCurrent_ = true;
	}

	SQLMXLob(SQLMXConnection connection, String lobLocator, InputStream x, long length, boolean isBlob) throws SQLException
	{
		this(connection, lobLocator, isBlob);
		is_ = x;
		length_ = length;
		isCurrent_ = true;
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
	private String			traceId_;
	static PrintWriter		traceWriter_;
	static int			traceFlag_;
	SQLMXConnection			conn_;
	String				lobLocator_;
	SQLMXLobInputStream		inputStream_;
	SQLMXLobOutputStream		outputStream_;
	boolean				isCurrent_;
	InputStream			is_;
	byte[]				b_;
	long				length_;
	int				offset_;
	int				chunkSize_;
	boolean				isBlob_;

	
	private static int methodId_length				=  0;
	private static int methodId_truncate				=  1;
	private static int methodId_getInputStream			=  2;
	private static int methodId_setOutputStream			=  3;
	private static int methodId_close				=  4;
	private static int methodId_convSQLExceptionToIO		=  5;
	private static int methodId_checkIfCurrent			=  6;
	private static int methodId_checkAutoCommitExtTxn		=  7;
	private static int methodId_SQLMXLob_LLJL			=  8;
	private static int methodId_SQLMXLob_LLJLIL			=  9;
	private static int totalMethodIds				= 10;
	private static JdbcDebug[] debug;

	static
	{
		String className = "SQLMXLob";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_length] = new JdbcDebug(className,"length");
			debug[methodId_truncate] = new JdbcDebug(className,"truncate");
			debug[methodId_getInputStream] = new JdbcDebug(className,"getInputStream");
			debug[methodId_setOutputStream] = new JdbcDebug(className,"setOutputStream");
			debug[methodId_close] = new JdbcDebug(className,"close");
			debug[methodId_convSQLExceptionToIO] = new JdbcDebug(className,"convSQLExceptionToIO");
			debug[methodId_checkIfCurrent] = new JdbcDebug(className,"checkIfCurrent");
			debug[methodId_checkAutoCommitExtTxn] = new JdbcDebug(className,"checkAutoCommitExtTxn");
			debug[methodId_SQLMXLob_LLJL] = new JdbcDebug(className,"SQLMXLob[LLJL]");
			debug[methodId_SQLMXLob_LLJLIL] = new JdbcDebug(className,"SQLMXLob[LLJLIL]");
		}
	}
}
