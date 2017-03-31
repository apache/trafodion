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
	// public methods
	public long length() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_length].methodEntry();
		try
		{
			long length = 0;

			checkIfCurrent();
			prepareGetLobLenStmt();
			PreparedStatement GetLobLenStmt = getGetLobLenStmt();

			if ((traceWriter_ != null) &&
				((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL)))
			{
				traceWriter_.println(getTraceId()
					+ "length() - GetLobLenStmt params: tableName_=" + tableName_
					+ " dataLocator_=" + dataLocator_);
			}

			synchronized (GetLobLenStmt)
			{
				GetLobLenStmt.setString(1, tableName_);
				GetLobLenStmt.setLong(2, dataLocator_);
				ResultSet rs = GetLobLenStmt.executeQuery();
				try
				{
					if (rs.next())
						length = rs.getLong(1);
				}
				finally
				{
					rs.close();
				}
			}
			return length;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_length].methodExit();
		}
	}

	public void truncate(long len) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_truncate].methodEntry();
		try
		{
			int chunkNo;
			int offset;
			byte[] chunk;

			if (len < 0)
			{
				Object[] messageArguments = new Object[1];
				messageArguments[0] = "SQLMXLob.truncate(long)";
				throw Messages.createSQLException(conn_.locale_,"invalid_input_value", messageArguments);
			}
			checkIfCurrent();
			chunkNo = (int)(len / chunkSize_);
			offset = (int)(len % chunkSize_);
			prepareDelLobDataStmt();
			PreparedStatement DelLobStmt = getDelLobDataStmt();

			if ((traceWriter_ != null) &&
				((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL)))
			{
				traceWriter_.println(getTraceId()
					+ "truncate(" + len + ") - DelLobStmt params: tableName_=" + tableName_
					+ " dataLocator_=" + dataLocator_
					+ " chunkNo+1=" + chunkNo+1);
			}

			synchronized (DelLobStmt)
			{
				DelLobStmt.setString(1, tableName_);
				DelLobStmt.setLong(2, dataLocator_);
				DelLobStmt.setInt(3, chunkNo+1);
				DelLobStmt.setInt(4, Integer.MAX_VALUE);
				DelLobStmt.executeUpdate();
			}
			if (offset != 0)
			{
				prepareTrunLobDataStmt();
				PreparedStatement TrunLobStmt = getTrunLobDataStmt();

				if ((traceWriter_ != null) &&
					((traceFlag_ == T2Driver.LOB_LVL) || (traceFlag_ == T2Driver.ENTRY_LVL)))
				{
					traceWriter_.println(getTraceId()
						+ "truncate(" + len + ") - TrunLobStmt params: offset=" + offset
						+ " tableName_=" + tableName_
						+ " dataLocator_=" + dataLocator_
						+ " chunkNo=" + chunkNo);
				}

				synchronized (TrunLobStmt)
				{
					TrunLobStmt.setInt(1, offset);
					TrunLobStmt.setString(2, tableName_);
					TrunLobStmt.setLong(3, dataLocator_);
					TrunLobStmt.setInt(4, chunkNo);
					TrunLobStmt.executeUpdate();
				}
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_truncate].methodExit();
		}
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

	OutputStream setOutputStream(long pos) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setOutputStream].methodEntry();
		try
		{
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
			outputStream_ = new SQLMXLobOutputStream(conn_, this, pos);
			return outputStream_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setOutputStream].methodExit();
		}
	}


	void close()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_close].methodEntry();
		try
		{
			isCurrent_ = false;
			try
			{
				if (inputStream_ != null)
					inputStream_.close();
				if (outputStream_ != null)
					outputStream_.close();
			}
			catch (IOException e)
			{
			}
			finally
			{
				inputStream_ = null;
				outputStream_ = null;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_close].methodExit();
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

    // Declare the following abstract methods to resolve symbols
	abstract void prepareGetLobLenStmt() throws SQLException;
	abstract void prepareDelLobDataStmt()  throws SQLException;
	abstract void prepareGetLobDataStmt() throws SQLException;
	abstract void prepareUpdLobDataStmt() throws SQLException;
	abstract void prepareInsLobDataStmt() throws SQLException;
	abstract void prepareTrunLobDataStmt() throws SQLException;
	abstract PreparedStatement getGetLobLenStmt();
	abstract PreparedStatement getDelLobDataStmt();
	abstract PreparedStatement getTrunLobDataStmt();
	abstract PreparedStatement getInsLobDataStmt();
	abstract PreparedStatement getUpdLobDataStmt();
	abstract PreparedStatement getGetLobDataStmt();



	// Constructors
	SQLMXLob(SQLMXConnection connection, String tableName, long dataLocator, String lobTableName, boolean isBlob)
		throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXLob_LLJL].methodEntry();
		try
		{
			conn_ = connection;
			tableName_ = tableName;
			isCurrent_ = true;
			dataLocator_ = dataLocator;
			if (lobTableName != null)
			{
				lobTableName_ = lobTableName;
				SQLMXDataLocator tempLoc = (SQLMXDataLocator)SQLMXConnection.lobTableToDataLoc_.get(lobTableName);
				if (tempLoc == null)
				{
					dataLocator = conn_.getDataLocator(lobTableName_, isBlob);
				}
				SQLMXDataLocator dataLoc = (SQLMXDataLocator)SQLMXConnection.lobTableToDataLoc_.get(lobTableName);
				chunkSize_ = dataLoc.chunkSize_;
			}


		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXLob_LLJL].methodExit();
		}
	}

	SQLMXLob(SQLMXConnection connection, String tableName, long dataLocator, InputStream x,
		int length, String lobTableName, boolean isBlob) throws SQLException
	{
		this(connection, tableName, dataLocator, lobTableName, isBlob);

		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXLob_LLJLIL].methodEntry();
		try
		{
			is_ = x;
			isLength_ = length;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXLob_LLJLIL].methodExit();
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
	SQLMXConnection			conn_;
	String					tableName_;
	long					dataLocator_;
	SQLMXLobInputStream		inputStream_;
	SQLMXLobOutputStream	outputStream_;
	boolean					isCurrent_;
	InputStream				is_;
	int						isLength_;
	String					lobTableName_;
	int						chunkSize_;

	private static int methodId_length					=  0;
	private static int methodId_truncate				=  1;
	private static int methodId_getInputStream			=  2;
	private static int methodId_setOutputStream			=  3;
	private static int methodId_close					=  4;
	private static int methodId_convSQLExceptionToIO	=  5;
	private static int methodId_checkIfCurrent			=  6;
	private static int methodId_checkAutoCommitExtTxn	=  7;
	private static int methodId_SQLMXLob_LLJL			=  8;
	private static int methodId_SQLMXLob_LLJLIL			=  9;
	private static int totalMethodIds					= 10;
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
