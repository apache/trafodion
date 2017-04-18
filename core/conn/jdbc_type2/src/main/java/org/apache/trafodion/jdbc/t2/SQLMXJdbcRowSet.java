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
 * Filename :		SQLMXJdbcRowSet.java
 * Description :	
 */

package org.apache.trafodion.jdbc.t2;

import java.io.*;
import java.math.BigDecimal;
import java.net.URL;

import java.sql.*;

import java.util.Calendar;
import java.util.Hashtable;
import java.util.Locale;
import java.util.Map;
import java.util.Properties;
import java.util.Vector;

import javax.naming.*;
import javax.sql.rowset.*;
import javax.sql.RowSet;
import javax.sql.RowSetEvent;
import javax.sql.RowSetMetaData;
import javax.sql.RowSetListener;

import javax.sql.rowset.serial.SerialArray;
import javax.sql.rowset.serial.SerialBlob;
import javax.sql.rowset.serial.SerialClob;

import java.beans.PropertyChangeSupport;

public class SQLMXJdbcRowSet implements JdbcRowSet, Joinable
{
	public void commit() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_commit].methodEntry();
		try
		{
			checkValidState();
			conn_.commit();
			if(conn_.getHoldability() != ResultSet.HOLD_CURSORS_OVER_COMMIT)
			{
				resultSet_ = null;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_commit].methodExit();
		}
	}

	public boolean getAutoCommit() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getAutoCommit].methodEntry();
		try
		{
			checkValidState();
		    return conn_.getAutoCommit();
		}		
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getAutoCommit].methodExit();
		}
	}

	public RowSetWarning getRowSetWarnings() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getRowSetWarnings].methodEntry();
		try
		{
			// RowSet warnings are currently not supported
			return null;
		}		
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getRowSetWarnings].methodExit();
		}
	}

	public boolean getShowDeleted() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getShowDeleted].methodEntry();
		try
		{
			checkValidState();
			return showDeleted_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getShowDeleted].methodExit();
		}
	}

	public void setShowDeleted(boolean flag) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setShowDeleted].methodEntry();

		showDeleted_ = flag;

		if (JdbcDebugCfg.entryActive) debug[methodId_setShowDeleted].methodExit();
	}

	public SQLWarning getWarnings() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getWarnings].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getWarnings();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getWarnings].methodExit();
		}
	}

	public void rollback() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_rollback_V].methodEntry();
		try
		{
			clearWarnings();
			conn_.rollback();
			resultSet_ = null;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_rollback_V].methodExit();
		}
	}

	public void rollback(Savepoint savepoint) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_rollback_L].methodEntry();
		try
		{
			clearWarnings();
			conn_.rollback(savepoint);
			resultSet_ = null;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_rollback_L].methodExit();
		}
	}

	public void setAutoCommit(boolean autoCommit) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setAutoCommit].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setAutoCommit].methodParameters("autoCommit= " + autoCommit);
		try
		{
			checkValidState();
			conn_.setAutoCommit(autoCommit);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setAutoCommit].methodExit();
		}
	}

	public boolean absolute(int row) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_absolute].methodEntry();
		try
		{
			checkValidState();
			boolean flag = resultSet_.absolute(row);
			notifyCursorMoved();
			return flag;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_absolute].methodExit();
		}
	}

	public void addRowSetListener(RowSetListener rslistener)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_addRowSetListener].methodEntry();
		listeners_.add(rslistener);
		if (JdbcDebugCfg.entryActive) debug[methodId_addRowSetListener].methodExit();
	}

	public void afterLast() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_afterLast].methodEntry();
		try
		{
			checkValidState();
			resultSet_.afterLast();
			notifyCursorMoved();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_afterLast].methodExit();
		}
	}
	
	public void beforeFirst() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_beforeFirst].methodEntry();
		try
		{
			checkValidState();
			resultSet_.beforeFirst();
			notifyCursorMoved();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_beforeFirst].methodExit();
		}
	}

	public void clearParameters() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_clearParameters].methodEntry();
		parameters_.clear();
		if (JdbcDebugCfg.entryActive) debug[methodId_clearParameters].methodExit();
	}

	public void clearWarnings() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_clearWarnings].methodEntry();
		try
		{
			checkValidState();
			resultSet_.clearWarnings();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_clearWarnings].methodExit();
		}
	}

	public void cancelRowUpdates() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_cancelRowUpdates].methodEntry();
		try
		{
			checkValidState();
			resultSet_.cancelRowUpdates();
			notifyRowChanged();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_cancelRowUpdates].methodExit();
		}
	}
	
	public void close() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_close].methodEntry();
		try
		{
			if (resultSet_ != null)
			{
				resultSet_.close();
				resultSet_ = null;
			}
			if (prepStmt_ != null)
			{
				prepStmt_.close();
				prepStmt_ = null;
			}
			if (conn_ != null)
			{
				conn_.close();
				conn_ = null;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_close].methodExit();
		}
	}

	public void deleteRow() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_deleteRow].methodEntry();
		try
		{
			checkValidState();
			resultSet_.deleteRow();
			notifyRowChanged();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_deleteRow].methodExit();
		}
	}

	public void execute() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_execute].methodEntry();
		try
		{
			prepareStmt();
			setProperties(prepStmt_);
			decodeParameters(getParameters(), prepStmt_);
			resultSet_ = prepStmt_.executeQuery();
			notifyRowSetChanged();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_execute].methodExit();
		}
	}

	public int findColumn(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_findColumn].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.findColumn(columnName);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_findColumn].methodExit();
		}
	}
	
	public boolean first() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_first].methodEntry();
		try
		{
			checkValidState();
			boolean flag = resultSet_.first();
			notifyCursorMoved();
			return flag;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_first].methodExit();
		}
	}
	
	public Array getArray(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getArray_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getArray(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getArray_I].methodExit();
		}
	}
	
	public Array getArray(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getArray_L].methodEntry();
		try
		{
			return resultSet_.getArray(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getArray_L].methodExit();
		}
	}
	
	public InputStream getAsciiStream(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getAsciiStream_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getAsciiStream(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getAsciiStream_I].methodExit();
		}
	}
	
	public InputStream getAsciiStream(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getAsciiStream_L].methodEntry();
		try
		{
			return resultSet_.getAsciiStream(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getAsciiStream_L].methodExit();
		}
	}
	
	public BigDecimal getBigDecimal(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getBigDecimal_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getBigDecimal(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getBigDecimal_I].methodExit();
		}
	}

	/**
	 * @deprecated Method getBigDecimal is deprecated
	 */
	@Deprecated public BigDecimal getBigDecimal(int columnIndex, int scale) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getBigDecimal_II].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getBigDecimal(columnIndex, scale);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getBigDecimal_II].methodExit();
		}
	}
  	
	public BigDecimal getBigDecimal(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getBigDecimal_L].methodEntry();
		try
		{
			return resultSet_.getBigDecimal(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getBigDecimal_L].methodExit();
		}
	}

	/**
	 * @deprecated Method getBigDecimal is deprecated
	 */
	@Deprecated public BigDecimal getBigDecimal(String columnName, int scale) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getBigDecimal_LI].methodEntry();
		try
		{
			return resultSet_.getBigDecimal(findColumn(columnName), scale);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getBigDecimal_LI].methodExit();
		}
	}
	
	public InputStream getBinaryStream(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getBinaryStream_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getBinaryStream(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getBinaryStream_I].methodExit();
		}
	}

	public InputStream getBinaryStream(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getBinaryStream_L].methodEntry();
		try
		{
			return resultSet_.getBinaryStream(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getBinaryStream_L].methodExit();
		}
	}

	public Blob getBlob(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getBlob_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getBlob(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getBlob_I].methodExit();
		}
	}

	public Blob getBlob(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getBlob_L].methodEntry();
		try
		{
			return resultSet_.getBlob(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getBlob_L].methodExit();
		}
	}
		
	public boolean getBoolean(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getBoolean_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getBoolean(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getBoolean_I].methodExit();
		}
	}
	
	public boolean getBoolean(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getBoolean_L].methodEntry();
		try
		{
			return resultSet_.getBoolean(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getBoolean_L].methodExit();
		}
	}
	
	public byte getByte(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getByte_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getByte(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getByte_I].methodExit();
		}
	}
  		
	public byte getByte(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getByte_L].methodEntry();
		try
		{
			return resultSet_.getByte(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getByte_L].methodExit();
		}
	}
	
	public byte[] getBytes(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getBytes_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getBytes(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getBytes_I].methodExit();
		}
	}
	
	public byte[] getBytes(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getBytes_L].methodEntry();
		try
		{
			return resultSet_.getBytes(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getBytes_L].methodExit();
		}
	}
	
	public Reader getCharacterStream(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getCharacterStream_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getCharacterStream(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getCharacterStream_I].methodExit();
		}
	}
	
	public Reader getCharacterStream(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getCharacterStream_L].methodEntry();
		try
		{
			return resultSet_.getCharacterStream(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getCharacterStream_L].methodExit();
		}
	}

	public Clob getClob(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getClob_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getClob(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getClob_I].methodExit();
		}
	}
	
	public Clob getClob(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getClob_L].methodEntry();
		try
		{
			return resultSet_.getClob(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getClob_L].methodExit();
		}
	}

	public String getCommand()
	{
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getCommand].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_getCommand].methodParameters("cmd = " + command_);
		try
		{
			return command_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getCommand].methodExit();
		}
	}

	public int getConcurrency() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getConcurrency].methodEntry();
		try
		{     
			/*if(resultSet_ != null)
			concurrency_ = resultSet_.getConcurrency();*/
			return concurrency_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getConcurrency].methodExit();
		}
	}
		  
	public String getCursorName() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getCursorName].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getCursorName();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getCursorName].methodExit();
		}
	}

	//Note: This is not in JdbcRowSet spec. Added for additional functionality
	public DatabaseMetaData getDatabaseMetaData() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getDatabaseMetaData].methodEntry();
		try
		{
			if(conn_ == null)
			{
				Connection connection = connect();
				return connection.getMetaData();
			}
			else
				return conn_.getMetaData();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getDatabaseMetaData].methodExit();
		}
	}

	//Note: This is not in JdbcRowSet spec. Added for additional functionality
	public ParameterMetaData getParameterMetaData() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getParameterMetaData].methodEntry();
		try
		{
			prepareStmt();
			return prepStmt_.getParameterMetaData();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getParameterMetaData].methodExit();
		}
	}

	public String getDataSourceName()
	{
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getDataSourceName].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_getDataSourceName].methodParameters("dataSourceName = " + dataSourceName_);
		try
		{
			return dataSourceName_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getDataSourceName].methodExit();
		}

	}

	public Date getDate(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getDate_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getDate(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getDate_I].methodExit();
		}
	}
		  
	public Date getDate(int columnIndex, Calendar cal) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getDate_IL].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getDate(columnIndex, cal);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getDate_IL].methodExit();
		}
	}

	public Date getDate(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getDate_L].methodEntry();
		try
		{
			return resultSet_.getDate(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getDate_L].methodExit();
		}
	}

	public Date getDate(String columnName, Calendar cal) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getDate_SL].methodEntry();
		try
		{
			return resultSet_.getDate(findColumn(columnName), cal);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getDate_SL].methodExit();
		}
	}

	public double getDouble(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getDouble_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getDouble(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getDouble_I].methodExit();
		}
	}
		
	public double getDouble(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getDouble_L].methodEntry();
		try
		{
			return resultSet_.getDouble(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getDouble_L].methodExit();
		}
	}

	public boolean getEscapeProcessing() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getEscapeProcessing].methodEntry();
		try
		{
			return escapeProcessing_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getEscapeProcessing].methodExit();
		}
	}
 	
	public int getFetchDirection() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getFetchDirection].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getFetchDirection();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getFetchDirection].methodExit();
		}
	}

	public int getFetchSize() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getFetchSize].methodEntry();
		int fetchsize = 0;
		try
		{
			fetchSize_ = resultSet_.getFetchSize();
			return fetchSize_;
		}
		catch(NullPointerException nullpointerexception)
		{
			fetchSize_ = fetchsize;
			return fetchSize_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getFetchSize].methodExit();
		}
	}
  
	public float getFloat(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getFloat_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getFloat(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getFloat_I].methodExit();
		}
	}
  	 
	public float getFloat(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getFloat_L].methodEntry();
		try
		{
			return resultSet_.getFloat(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getFloat_L].methodExit();
		}
	}
  
	public int getInt(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getInt_I].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_getInt_I].methodParameters(
										  "columnIndex = " + columnIndex);
		try
		{
			checkValidState();
			return resultSet_.getInt(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getInt_I].methodExit();
		}
	}
  	 
	public int getInt(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getInt_L].methodEntry();
		try
		{
			return resultSet_.getInt(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getInt_L].methodExit();
		}
	}
  
	public long getLong(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getLong_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getLong(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getLong_I].methodExit();
		}
	}
  	 
	public long getLong(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getLong_L].methodEntry();
		try
		{
			return resultSet_.getLong(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getLong_L].methodExit();
		}
	}

	public int getMaxFieldSize() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getMaxFieldSize].methodEntry();
		try
		{
			return maxFieldSize_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getMaxFieldSize].methodExit();
		}
	}

	public int getMaxRows() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getMaxRows].methodEntry();
		try
		{
			return maxRows_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getMaxRows].methodExit();
		}
	}

	public ResultSetMetaData getMetaData() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getMetaData].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getMetaData();
		}
		catch(SQLException sqlexception)
		{
			prepareStmt();
			return prepStmt_.getMetaData();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getMetaData].methodExit();
		}
	}
  
	public Object getObject(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getObject_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getObject(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getObject_I].methodExit();
		}
	}
  
	public Object getObject(int columnIndex, Map<String,Class<?>> map) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getObject_IL].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getObject(columnIndex, map);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getObject_IL].methodExit();
		}
	}
		
	public Object getObject(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getObject_L].methodEntry();
		try
		{
			return resultSet_.getObject(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getObject_L].methodExit();
		}
	}

	public Object getObject(String columnName, Map<String,Class<?>> map) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getObject_LL].methodEntry();
		try
		{
			return resultSet_.getObject(findColumn(columnName), map);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getObject_LL].methodExit();
		}
	}
  	
	public String getPassword()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getPassword].methodEntry();
		try
		{
			return password_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getPassword].methodExit();
		}
	}

	public int getQueryTimeout() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getQueryTimeout].methodEntry();
		try
		{
			return queryTimeout_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getQueryTimeout].methodExit();
		}
	}

	public Ref getRef(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getRef_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getRef(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getRef_I].methodExit();
		}
	}
		
	public Ref getRef(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getRef_L].methodEntry();
		try
		{
			return resultSet_.getRef(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getRef_L].methodExit();
		}
	}
	
	public int getRow() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getRow].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getRow();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getRow].methodExit();
		}
	}
	  
	public short getShort(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getShort_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getShort(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getShort_I].methodExit();
		}
	}
  	  
	public short getShort(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getShort_L].methodEntry();
		try
		{
			return resultSet_.getShort(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getShort_L].methodExit();
		}
	}
	  
	public Statement getStatement() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getStatement].methodEntry();
		try
		{
			if(resultSet_ != null)
				return resultSet_.getStatement();
			else
				return null;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getStatement].methodExit();
		}
	}
		
	public String getString(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getString_I].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_getString_I].methodParameters(
										  "columnIndex = " + columnIndex);
		try
		{
			checkValidState();
			return resultSet_.getString(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getString_I].methodExit();
		}
	}

	public String getString(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getString_L].methodEntry();
		try
		{
			return resultSet_.getString(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getString_L].methodExit();
		}
	}

	public Time getTime(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getTime_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getTime(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getTime_I].methodExit();
		}
	}

	public Time getTime(int columnIndex, Calendar cal) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getTime_IL].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getTime(columnIndex, cal);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getTime_IL].methodExit();
		}
	}

	public Time getTime(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getTime_L].methodEntry();
		try
		{
			return resultSet_.getTime(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getTime_L].methodExit();
		}
	}

	public Time getTime(String columnName, Calendar cal) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getTime_LL].methodEntry();
		try
		{
			return resultSet_.getTime(findColumn(columnName), cal);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getTime_LL].methodExit();
		}
	}

	public Timestamp getTimestamp(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getTimestamp_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getTimestamp(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getTimestamp_I].methodExit();
		}
	}
	
	public Timestamp getTimestamp(int columnIndex, Calendar cal) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getTimestamp_IL].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getTimestamp(columnIndex, cal);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getTimestamp_IL].methodExit();
		}
	}

	public Timestamp getTimestamp(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getTimestamp_L].methodEntry();
		try
		{
			return resultSet_.getTimestamp(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getTimestamp_L].methodExit();
		}
	}

	public Timestamp getTimestamp(String columnName, Calendar cal) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getTimestamp_LL].methodEntry();
		try
		{
			return resultSet_.getTimestamp(findColumn(columnName), cal);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getTimestamp_LL].methodExit();
		}
	}

	public int getTransactionIsolation()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getTransactionIsolation].methodEntry();
		try
		{
			return isolation_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getTransactionIsolation].methodExit();
		}
	}

	public int getType() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getType].methodEntry();
		try
		{
			checkValidState();
			//rowSetType_ = resultSet_.getType();
			return rowSetType_;
		}
		catch(SQLException sqle)
		{
			return rowSetType_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getType].methodExit();
		}
	}

	public Map<String,Class<?>> getTypeMap() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getTypeMap].methodEntry();
		try
		{
			return map_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getTypeMap].methodExit();
		}
	}

	/**
	 * @deprecated Method getUnicodeStream is deprecated
	 */
	@Deprecated public InputStream getUnicodeStream(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getUnicodeStream_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getUnicodeStream(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getUnicodeStream_I].methodExit();
		}
	}

	/**
	 * @deprecated Method getUnicodeStream is deprecated
	 */
	@Deprecated public InputStream getUnicodeStream(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getUnicodeStream_L].methodEntry();
		try
		{
			return resultSet_.getUnicodeStream(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getUnicodeStream_L].methodExit();
		}
	}

	public String getUrl() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getUrl].methodEntry();
		try
		{
			return URL_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getUrl].methodExit();
		}
	}

	public URL getURL(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getURL_I].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.getURL(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getURL_I].methodExit();
		}
	}

	public URL getURL(String  columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getURL_L].methodEntry();
		try
		{
			return resultSet_.getURL(findColumn(columnName));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getURL_L].methodExit();
		}
	}

	public String getUsername()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getUsername].methodEntry();
		try
		{
			return username_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getUsername].methodExit();
		}
	}

	public void insertRow() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_insertRow].methodEntry();
		try
		{
			checkValidState();
			resultSet_.insertRow();
			notifyRowChanged();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_insertRow].methodExit();
		}
	}
	  
	public boolean isAfterLast() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isAfterLast].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.isAfterLast();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isAfterLast].methodExit();
		}
	}

	public boolean isBeforeFirst() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isBeforeFirst].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.isBeforeFirst();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isBeforeFirst].methodExit();
		}
	}

	public boolean isFirst() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isFirst].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.isFirst();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isFirst].methodExit();
		}
	}

	public boolean isLast() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isLast].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.isLast();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isLast].methodExit();
		}
	}

	public boolean isReadOnly()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isReadOnly].methodEntry();
		try
		{
			return readOnly_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isReadOnly].methodExit();
		}
	}

	public boolean last() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_last].methodEntry();
		try
		{
			checkValidState();
			boolean flag = resultSet_.last();
			notifyCursorMoved();
			return flag;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_last].methodExit();
		}
	}

	public void moveToCurrentRow() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_moveToCurrentRow].methodEntry();
		try
		{
			checkValidState();
			resultSet_.moveToCurrentRow();
			notifyCursorMoved();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_moveToCurrentRow].methodExit();
		}
	}
	  
	public void moveToInsertRow() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_moveToInsertRow].methodEntry();
		try
		{
			checkValidState();
			resultSet_.moveToInsertRow();
			notifyCursorMoved();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_moveToInsertRow].methodExit();
		}
	}
	  
	public boolean next() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_next].methodEntry();
		try
		{
			checkValidState();
			boolean flag = resultSet_.next();
			notifyCursorMoved();
			return flag;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_next].methodExit();
		}
	}

	public boolean previous() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_previous].methodEntry();
		try
		{
			checkValidState();
			boolean flag = resultSet_.previous();
			notifyCursorMoved();
			return flag;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_previous].methodExit();
		}
	}

	public void refreshRow() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_refreshRow].methodEntry();
		try
		{
			checkValidState();
			resultSet_.refreshRow();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_refreshRow].methodExit();
		}
	}

	public boolean relative(int row) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_relative].methodEntry();
		try
		{
			checkValidState();
			boolean flag = resultSet_.relative(row);
			notifyCursorMoved();
			return flag;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_relative].methodExit();
		}
	}

	public void removeRowSetListener(RowSetListener rowsetlistener)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_removeRowSetListener].methodEntry();
		try
		{
			listeners_.remove(rowsetlistener);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_removeRowSetListener].methodExit();
		}
	}
	
	public boolean rowDeleted() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_rowDeleted].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.rowDeleted();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_rowDeleted].methodExit();
		}
	}

	public boolean rowInserted() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_rowInserted].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.rowInserted();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_rowInserted].methodExit();
		}
	}

	public boolean rowUpdated() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_rowUpdated].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.rowUpdated();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_rowUpdated].methodExit();
		}
	}

	public void setFetchDirection(int direction) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setFetchDirection].methodEntry();
		try
		{
			checkValidState();
			resultSet_.setFetchDirection(direction);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setFetchDirection].methodExit();
		}
	}

	public void setFetchSize(int rows) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setFetchSize].methodEntry();
		try
		{
			checkValidState();
			resultSet_.setFetchSize(rows);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setFetchSize].methodExit();
		}
	}

	public void setArray(int parameterIndex, Array x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setArray].methodEntry();
		try
		{
			validateSetInvocation(parameterIndex);
			parameters_.put(parameterIndex - 1, new SerialArray(x));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setArray].methodExit();
		}
	}

	public void setAsciiStream(int parameterIndex, InputStream x, int length) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setAsciiStream].methodEntry();
		try
		{
			validateSetInvocation(parameterIndex);
			Object paramObj[] = new Object[3];
			paramObj[0] = x;
			paramObj[1] = length;
			paramObj[2] = 2;
			parameters_.put(parameterIndex - 1, paramObj);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setAsciiStream].methodExit();
		}
	}

	public void setBigDecimal(int parameterIndex, BigDecimal x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setBigDecimal].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setBigDecimal].methodParameters(
										  Integer.toString(parameterIndex)+",?");
		try
		{
			validateSetInvocation(parameterIndex);
			parameters_.put(parameterIndex - 1, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setBigDecimal].methodExit();
		}
	}

	public void setBinaryStream(int parameterIndex, InputStream x, int length) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setBinaryStream].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setBinaryStream].methodParameters(
										  Integer.toString(parameterIndex)+",?");
		try
		{
			validateSetInvocation(parameterIndex);
			Object paramObj[] = new Object[2];
			paramObj[0] = x;
			paramObj[1] = length;
			parameters_.put(parameterIndex - 1, paramObj);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setBinaryStream].methodExit();
		}
	}

	public void setBlob(int parameterIndex, Blob x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setBlob].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setBlob].methodParameters(
										  Integer.toString(parameterIndex)+",?");
		try
		{
			validateSetInvocation(parameterIndex);
			parameters_.put(parameterIndex - 1, new SerialBlob(x));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setBlob].methodExit();
		}
	}

	public void setBoolean(int parameterIndex, boolean x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setBoolean].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setBoolean].methodParameters(
										  Integer.toString(parameterIndex)+","+Boolean.toString(x));
		try
		{
			validateSetInvocation(parameterIndex);
			parameters_.put(parameterIndex - 1, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setBoolean].methodExit();
		}
	}

	public void setByte(int parameterIndex, byte x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setByte].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setByte].methodParameters(
										  Integer.toString(parameterIndex)+","+Byte.toString(x));
		try
		{
			validateSetInvocation(parameterIndex);
			parameters_.put(parameterIndex - 1, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setByte].methodExit();
		}
	}

	public void setBytes(int parameterIndex, byte[] x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setBytes].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setBytes].methodParameters(
										  Integer.toString(parameterIndex)+",byte[]");
		try
		{
			validateSetInvocation(parameterIndex);
			parameters_.put(parameterIndex - 1, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setBytes].methodExit();
		}
	}

	public void setCharacterStream(int parameterIndex, Reader reader, int length) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setCharacterStream].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setCharacterStream].methodParameters(
										  Integer.toString(parameterIndex)+",?,"+Integer.toString(length));
		try
		{
			validateSetInvocation(parameterIndex);
			Object paramObj[] = new Object[2];
			paramObj[0] = reader;
			paramObj[1] = length;
			parameters_.put(parameterIndex - 1, paramObj);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setCharacterStream].methodExit();
		}
	}

	public void setClob(int parameterIndex, Clob x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setClob].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setClob].methodParameters(
										  Integer.toString(parameterIndex)+",?");
		try
		{
			validateSetInvocation(parameterIndex);
			parameters_.put(parameterIndex - 1, new SerialClob(x));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setClob].methodExit();
		}
	}

	public void setCommand(String cmd) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setCommand].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setCommand].traceOut(
										  JdbcDebug.debugLevelEntry,
										  "cmd = " + cmd);
		try
		{
			String cmd_string = getCommand();

			if(cmd_string != null)
			{
				// Only update if the cmd has changed
				if(!cmd_string.equals(cmd))
				{
					// Save off new command and null existing prepStmt_ and ResultSet_
					command_ = cmd;
					prepStmt_ = null;
					resultSet_ = null;
					clearParameters();
					propertyChangeSupport_.firePropertyChange("command", cmd_string, cmd);
				}
			} 
			else
			{
				command_ = cmd;
				propertyChangeSupport_.firePropertyChange("command", null, cmd);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setCommand].methodExit();
		}
	}

	public void setConcurrency(int concurrency) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setConcurrency].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setConcurrency].methodParameters("concurrency = " + concurrency);
		try
		{
			if (concurrency != ResultSet.CONCUR_READ_ONLY && concurrency != ResultSet.CONCUR_UPDATABLE)
				throw Messages.createSQLException(locale_, "invalid_resultset_concurrency", null);
				if((concurrency_ != concurrency) && (prepStmt_ == null)){
				int current_rs_concurrency = concurrency_;
				concurrency_ = concurrency;
				propertyChangeSupport_.firePropertyChange("concurrency", current_rs_concurrency, concurrency);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setConcurrency].methodExit();
		}
	}

	public void setDataSourceName(String dsname)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setDataSourceName].methodEntry();
		try
		{
			if(getDataSourceName() != null)
			{
				// Check if a new datasource name is defined
				if(!getDataSourceName().equals(dsname))
				{
					String s1 = getDataSourceName();
					dataSourceName_ = dsname;
					conn_ = null;
					prepStmt_ = null;
					resultSet_ = null;
					propertyChangeSupport_.firePropertyChange("dataSourceName", s1, dsname);
				}
			} 
			else
			{
				dataSourceName_ = dsname;
				propertyChangeSupport_.firePropertyChange("dataSourceName", null, dsname);
			}
			// Set URL_ to null such that connections
			// are established using dataSourceName_ 
			URL_ = null;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setDataSourceName].methodExit();
		}
	}

	public void setDate(int parameterIndex, Date date) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setDate_IL].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setDate_IL].methodParameters(
										  Integer.toString(parameterIndex)+",?");
		try
		{
			validateSetInvocation(parameterIndex);
			parameters_.put(parameterIndex - 1, date);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setDate_IL].methodExit();
		}
	}

	public void setDate(int parameterIndex, Date date, Calendar cal) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setDate_ILL].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setDate_ILL].methodParameters(
										  Integer.toString(parameterIndex)+",?,?");
		try
		{
			validateSetInvocation(parameterIndex);
			Object paramObj[] = new Object[2];
			paramObj[0] = date;
			paramObj[1] = cal;
			parameters_.put(parameterIndex - 1, paramObj);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setDate_ILL].methodExit();
		}
	}

	public void setDouble(int parameterIndex, double x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setDouble].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setDouble].methodParameters(
										  Integer.toString(parameterIndex)+","+Double.toString(x));
		try
		{
			validateSetInvocation(parameterIndex);
			parameters_.put(parameterIndex - 1, new Double(x));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setDouble].methodExit();
		}
	}

	public void setEscapeProcessing(boolean enable) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setEscapeProcessing].methodEntry();
		try
		{
			escapeProcessing_ = enable;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setEscapeProcessing].methodExit();
		}
	}

	public void setFloat(int parameterIndex, float x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setFloat].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setFloat].methodParameters(
										  Integer.toString(parameterIndex)+","+Float.toString(x));
		try
		{
			validateSetInvocation(parameterIndex);
			parameters_.put(parameterIndex - 1, new Float(x));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setFloat].methodExit();
		}
	}

	public void setInt(int parameterIndex, int x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setInt].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setInt].methodParameters(
										  Integer.toString(parameterIndex)+","+Integer.toString(x));
		try
		{
			validateSetInvocation(parameterIndex);
			parameters_.put(parameterIndex - 1, new Integer(x));
		}		
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setInt].methodExit();
		}
	}

	public void setLong(int parameterIndex, long x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setLong].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setLong].methodParameters(
										  Integer.toString(parameterIndex)+","+Long.toString(x));
		try
		{
			validateSetInvocation(parameterIndex);
			parameters_.put(parameterIndex - 1, new Long(x));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setLong].methodExit();
		}
	}

	public void setMaxFieldSize(int max) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setMaxFieldSize].methodEntry();
		try
		{
			if (max < 0)
				throw Messages.createSQLException(locale_, "invalid_maxFieldSize_value", null);
			int currentMaxFieldSize;
			currentMaxFieldSize = maxFieldSize_;
			if(currentMaxFieldSize != max)
			{
				maxFieldSize_ = max;
				propertyChangeSupport_.firePropertyChange("maxRows", currentMaxFieldSize, maxFieldSize_);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setMaxFieldSize].methodExit();
		}
	}

	public void setMaxRows(int maxrows) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setMaxRows].methodEntry();
		try
		{
			if(maxrows < 0)
			{
				throw Messages.createSQLException(locale_, "invalid_maxRows_value", null);
			}
			if(maxrows < getFetchSize())
			{
				throw Messages.createSQLException(locale_, "maxRows_too_small_value", null);
			}
			else
			{
				int currentMaxRows;
				try
				{
					currentMaxRows = getMaxRows();
				}
				catch(NullPointerException nullpointerexception)
				{
					currentMaxRows = 0;
				}
				if(currentMaxRows != maxrows)
				{
					maxRows_ = maxrows;
					prepStmt_.setMaxRows(maxRows_);
					propertyChangeSupport_.firePropertyChange("maxRows", currentMaxRows, maxrows);
				}
				return;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setMaxRows].methodExit();
		}
	}

	public void setNull(int parameterIndex, int sqlType) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setNull_II].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setNull_II].methodParameters(
										  Integer.toString(parameterIndex)+","+Integer.toString(sqlType));
		try
		{
			validateSetInvocation(parameterIndex);
			Object paramObj[] = new Object[2];
			paramObj[0] = null;
			paramObj[1] = sqlType;
			parameters_.put(parameterIndex - 1, paramObj);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setNull_II].methodExit();
		}
	}
	
	public void setNull(int parameterIndex, int sqlType, String typeName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setNull_IIL].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setNull_IIL].methodParameters(
										  Integer.toString(parameterIndex)+","+Integer.toString(sqlType)+","+typeName);
		try
		{
			validateSetInvocation(parameterIndex);
			Object paramObj[] = new Object[3];
			paramObj[0] = null;
			paramObj[1] = sqlType;
			paramObj[2] = typeName;
			parameters_.put(parameterIndex - 1, paramObj);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setNull_IIL].methodExit();
		}
	}

	public void setObject(int parameterIndex, Object x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setObject_IL].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setObject_IL].methodParameters(
										  Integer.toString(parameterIndex)+",?");
		try
		{
			if (x == null)
			{
				setNull(parameterIndex, Types.NULL);
			}
			else
			{
				validateSetInvocation(parameterIndex);
				parameters_.put(parameterIndex - 1, x);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setObject_IL].methodExit();
		}
	}

	public void setObject(int parameterIndex, Object x, int targetSqlType) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setObject_ILI].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setObject_ILI].methodParameters(
										  Integer.toString(parameterIndex)+",?,"+Integer.toString(targetSqlType));
		try
		{
			if (x == null)
			{
				setNull(parameterIndex, Types.NULL);
			}
			else
			{
				validateSetInvocation(parameterIndex);
				Object paramObj[] = new Object[2];
				paramObj[0] = x;
				paramObj[1] = targetSqlType;
				parameters_.put(parameterIndex - 1, paramObj);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setObject_ILI].methodExit();
		}
	}

	public void setObject(int parameterIndex, Object x, int targetSqlType, int scale) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setObject_ILII].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setObject_ILII].methodParameters(
										  Integer.toString(parameterIndex)+",?,"+Integer.toString(targetSqlType)+
										  ","+Integer.toString(scale));
		try
		{
			if (x == null)
			{
				setNull(parameterIndex, Types.NULL);
			}
			else
			{
				validateSetInvocation(parameterIndex);
				Object paramObj[] = new Object[3];
				paramObj[0] = x;
				paramObj[1] = targetSqlType;
				paramObj[2] = scale;
				parameters_.put(parameterIndex - 1, paramObj);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setObject_ILII].methodExit();
		}
	}

	public void setPassword(String password)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setPassword].methodEntry();
		try
		{
			String currentPassword = password_;
			if(currentPassword != null)
			{
				if(!currentPassword.equals(password))
				{
					password_ = password;
					conn_ = null;
					prepStmt_ = null;
					resultSet_ = null;
					propertyChangeSupport_.firePropertyChange("password", currentPassword, password);
				}
			} 
			else
			{
				password_ = password;
				propertyChangeSupport_.firePropertyChange("password", currentPassword, password);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setPassword].methodExit();
		}
	}

	public void setQueryTimeout(int seconds) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setQueryTimeout].methodEntry();
		try
		{
			if (seconds < 0)
				throw Messages.createSQLException(locale_, "invalid_queryTimeout_value", null);
			queryTimeout_ = seconds;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setQueryTimeout].methodExit();
		}
	}

	public void setReadOnly(boolean readOnly)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setReadOnly].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setReadOnly].methodParameters(
										  "readOnly=" + readOnly);
		try
		{
			readOnly_ = readOnly;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setReadOnly].methodExit();
		}
	}

	public void setRef(int parameterIndex, Ref x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setRef].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setRef].methodParameters(
										  Integer.toString(parameterIndex)+",?");
		try
		{
			validateSetInvocation(parameterIndex);
			Messages.throwUnsupportedFeatureException(locale_, "setRef()");
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setRef].methodExit();
		}
	}

	public void setShort(int parameterIndex, short x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setShort].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setShort].methodParameters(
										  Integer.toString(parameterIndex)+","+Short.toString(x));
		try
		{
			validateSetInvocation(parameterIndex);
			parameters_.put(parameterIndex - 1, x);
		}		
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setShort].methodExit();
		}
	}

	public void setString(int parameterIndex, String s) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setString].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setString].methodParameters(
										  Integer.toString(parameterIndex)+","+s);
		try
		{
			validateSetInvocation(parameterIndex);
			parameters_.put(parameterIndex - 1, s);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setString].methodExit();
		}
	}

	public void setTime(int parameterIndex, Time x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setTime_IL].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setTime_IL].methodParameters(
										  Integer.toString(parameterIndex)+",?");
		try
		{
			validateSetInvocation(parameterIndex);
			parameters_.put(parameterIndex - 1, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setTime_IL].methodExit();
		}
	}

	public void setTime(int parameterIndex, Time x, Calendar cal) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setTime_ILL].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setTime_ILL].methodParameters(
										  Integer.toString(parameterIndex)+",?,?");
		try
		{
			validateSetInvocation(parameterIndex);
			Object paramObj[] = new Object[2];
			paramObj[0] = x;
			paramObj[1] = cal;
			parameters_.put(parameterIndex - 1, paramObj);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setTime_ILL].methodExit();
		}
	}

	public void setTimestamp(int parameterIndex, Timestamp x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setTimestamp_IL].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setTimestamp_IL].methodParameters(
										  Integer.toString(parameterIndex)+",?");
		try
		{
			validateSetInvocation(parameterIndex);
			parameters_.put(parameterIndex - 1, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setTimestamp_IL].methodExit();
		}
	}

	public void setTimestamp(int parameterIndex, Timestamp x, Calendar cal) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setTimestamp_ILL].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setTimestamp_ILL].methodParameters(
										  Integer.toString(parameterIndex)+",?,?");
		try
		{
			validateSetInvocation(parameterIndex);
			Object paramObj[] = new Object[2];
			paramObj[0] = x;
			paramObj[1] = cal;
			parameters_.put(parameterIndex - 1, paramObj);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setTimestamp_ILL].methodExit();
		}
	}

	public void setTransactionIsolation(int trans_isolation) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setTransactionIsolation].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setTransactionIsolation].methodParameters("trans_isolation = " + trans_isolation);
		try
		{
			if(isolation_ != trans_isolation)
			{
				int currentIsolation = isolation_;
				if (JdbcDebugCfg.traceActive)
					debug[methodId_setTransactionIsolation].methodParameters("prev transaction isolation = " + currentIsolation);

				if(conn_ != null)
				{
					conn_.setTransactionIsolation(trans_isolation);
				}
				isolation_ = trans_isolation;
				propertyChangeSupport_.firePropertyChange("transactionIsolation", currentIsolation, trans_isolation);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setTransactionIsolation].methodExit();
		}
	}

	public void setType(int type) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setType].methodEntry();
		try
		{
		/* Desc : The variable type is validated for the supported resultset types */  
		if(type == ResultSet.TYPE_FORWARD_ONLY || type == ResultSet.TYPE_SCROLL_INSENSITIVE || type== ResultSet.TYPE_SCROLL_SENSITIVE)
		{ 
			int currentRowSetType = rowSetType_;
			if(currentRowSetType != type)
			{
				rowSetType_ = type;
				propertyChangeSupport_.firePropertyChange("type", currentRowSetType, type);
			}
		}
		else
		{
			throw new SQLException("Error : Unsupported SQLMXJdbcRowSet Type");
		} 
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setType].methodExit();
		}
	}

	public void setTypeMap(Map<String,Class<?>> map)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setTypeMap].methodEntry();
		try
		{
			map_ = map;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setTypeMap].methodExit();
		}
	}

	public void setUrl(String url) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setUrl].methodEntry();
		try
		{
			if(URL_ != null)
			{
				if(!URL_.equals(url))
				{
					String currentURL = URL_;
					URL_ = url;
					conn_ = null;
					prepStmt_ = null;
					resultSet_ = null;
					propertyChangeSupport_.firePropertyChange("url", currentURL, url);
				}
			} 
			else
			{
				URL_ = url;
				propertyChangeSupport_.firePropertyChange("url", null, url);
			}
			// Set dataSourceName_ to null such that connections
			// are established using URL_
			dataSourceName_ = null;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setUrl].methodExit();
		}
	}

	public void setUsername(String uname)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setUsername].methodEntry();
		try
		{
			String currentUsername = username_;
			if(currentUsername != null)
			{
				if(!currentUsername.equals(uname))
				{
					username_ = uname;
					conn_ = null;
					prepStmt_ = null;
					resultSet_ = null;
					propertyChangeSupport_.firePropertyChange("username", currentUsername, uname);
				}
			} 
			else
			{
				username_ = uname;
				propertyChangeSupport_.firePropertyChange("password", currentUsername, uname);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setUsername].methodExit();
		}
	}

	public void updateArray(int columnIndex, Array x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateArray_IL].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateArray(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateArray_IL].methodExit();
		}
	}

	public void updateArray(String columnName, Array x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateArray_LL].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateArray(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateArray_LL].methodExit();
		}
	}

	public void updateAsciiStream(int columnIndex, InputStream x, int length) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateAsciiStream_ILI].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateAsciiStream(columnIndex, x, length);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateAsciiStream_ILI].methodExit();
		}
	}
	  
	public void updateAsciiStream(String columnName, InputStream x, int length) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateAsciiStream_LLI].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateAsciiStream(colIndex, x, length);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateAsciiStream_LLI].methodExit();
		}
	}
	  
	public void updateBigDecimal(int columnIndex, BigDecimal x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateBigDecimal_IL].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateBigDecimal(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateBigDecimal_IL].methodExit();
		}
	}
	  
	public void updateBigDecimal(String columnName, BigDecimal x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateBigDecimal_LL].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateBigDecimal(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateBigDecimal_LL].methodExit();
		}
	}
	  
	public void updateBinaryStream(int columnIndex, InputStream x, int length) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateBinaryStream_ILI].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateBinaryStream(columnIndex, x, length);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateBinaryStream_ILI].methodExit();
		}
	}
	  
	public void updateBinaryStream(String columnName, InputStream x, int length) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateBinaryStream_LLI].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateBinaryStream(colIndex, x, length);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateBinaryStream_LLI].methodExit();
		}
	}

	public void updateBlob(int columnIndex, Blob x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateBlob_IL].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateBlob(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateBlob_IL].methodExit();
		}
	}

	public void updateBlob(String columnName, Blob x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateBlob_LL].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateBlob(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateBlob_LL].methodExit();
		}
	}
	  
	public void updateBoolean(int columnIndex, boolean x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateBoolean_IZ].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateBoolean(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateBoolean_IZ].methodExit();
		}
	}
	  
	public void updateBoolean(String columnName, boolean x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateBoolean_LZ].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateBoolean(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateBoolean_LZ].methodExit();
		}
	}
	  
	public void updateByte(int columnIndex, byte x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateByte_IZ].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateByte(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateByte_IZ].methodExit();
		}
	}
	  
	public void updateByte(String columnName, byte x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateByte_LZ].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateByte(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateByte_LZ].methodExit();
		}
	}
	  
	public void updateBytes(int columnIndex, byte[] x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateBytes_IZ].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateBytes(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateBytes_IZ].methodExit();
		}
	}
	  
	public void updateBytes(String columnName, byte[] x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateBytes_LZ].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateBytes(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateBytes_LZ].methodExit();
		}
	}
	  
	public void updateCharacterStream(int columnIndex, Reader x, int length) throws SQLException 
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateCharacterStream_ILI].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateCharacterStream(columnIndex, x, length);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateCharacterStream_ILI].methodExit();
		}
	}
	  
	public void updateCharacterStream(String columnName, Reader x, int length) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateCharacterStream_LLI].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateCharacterStream(colIndex, x, length);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateCharacterStream_LLI].methodExit();
		}
	}
	  
	public void updateClob(int columnIndex, Clob x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateClob_IL].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateClob(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateClob_IL].methodExit();
		}
	}

	public void updateClob(String columnName, Clob x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateClob_LL].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateClob(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateClob_LL].methodExit();
		}
	}
	
	public void updateDate(int columnIndex, Date x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateDate_IL].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateDate(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateDate_IL].methodExit();
		}
	}
	  
	public void updateDate(String columnName, Date x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateDate_LL].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateDate(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateDate_LL].methodExit();
		}
	}
	  
	public void updateDouble(int columnIndex, double x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateDouble_ID].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateDouble(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateDouble_ID].methodExit();
		}
	}
	  
	public void updateDouble(String columnName, double x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateDouble_LD].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateDouble(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateDouble_LD].methodExit();
		}
	}
	  
	public void updateFloat(int columnIndex, float x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateFloat_IF].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateFloat(columnIndex, x);	
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateFloat_IF].methodExit();
		}
	}
	  
	public void updateFloat(String columnName, float x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateFloat_LF].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateFloat(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateFloat_LF].methodExit();
		}
	}
	  
	public void updateInt(int columnIndex, int x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateInt_II].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateInt(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateInt_II].methodExit();
		}
	}
	  
	public void updateInt(String columnName, int x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateInt_LI].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateInt(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateInt_LI].methodExit();
		}
	}
	  
	public void updateLong(int columnIndex, long x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateLong_IJ].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateLong(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateLong_IJ].methodExit();
		}
	}
	  
	public void updateLong(String columnName, long x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateLong_LJ].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateLong(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateLong_LJ].methodExit();
		}
	}
	  
	public void updateNull(int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateNull_I].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateNull(columnIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateNull_I].methodExit();
		}
	}
	  
	public void updateNull(String columnName) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateNull_L].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateNull(colIndex);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateNull_L].methodExit();
		}
	}
	  
	public void updateObject(int columnIndex, Object x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateObject_IL].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateObject(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateObject_IL].methodExit();
		}
	}
	  
	public void updateObject(int columnIndex, Object x, int scale) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateObject_ILI].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateObject(columnIndex, x, scale);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateObject_ILI].methodExit();
		}
	}
	
	public void updateObject(String columnName, Object x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateObject_LL].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateObject(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateObject_LL].methodExit();
		}
	}
	  
	public void updateObject(String columnName, Object x, int scale) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateObject_LLI].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateObject(colIndex, x, scale);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateObject_LLI].methodExit();
		}
	}
	 
	public void updateRef(int columnIndex, Ref x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateRef_IL].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateRef(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateRef_IL].methodExit();
		}
	}

	public void updateRef(String columnName, Ref x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateRef_LL].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateRef(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateRef_LL].methodExit();
		}
	}

	public void updateRow() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateRow].methodEntry();
		try
		{
			checkValidState();
			resultSet_.updateRow();
			notifyRowChanged();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateRow].methodExit();
		}
	}

	public void updateShort(int columnIndex, short x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateShort_IS].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateShort(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateShort_IS].methodExit();
		}
	}
	  
	public void updateShort(String columnName, short x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateShort_LS].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateShort(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateShort_LS].methodExit();
		}
	}
	  
	public void updateString(int columnIndex, String x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateString_IL].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateString(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateString_IL].methodExit();
		}
	}
	  
	public void updateString(String columnName, String x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateString_LL].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateString(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateString_LL].methodExit();
		}
	}
	  
	public void updateTime(int columnIndex, Time x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateTime_IL].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateTime(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateTime_IL].methodExit();
		}
	}
	  
	public void updateTime(String columnName, Time x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateTime_LL].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateTime(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateTime_LL].methodExit();
		}
	}
	  
	public void updateTimestamp(int columnIndex, Timestamp x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateTimestamp_IL].methodEntry();
		try
		{
			checkValidState();
			checkTypeConcurrency();
			resultSet_.updateTimestamp(columnIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateTimestamp_IL].methodExit();
		}
	}
	  
	public void updateTimestamp(String columnName, Timestamp x) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateTimestamp_LL].methodEntry();
		try
		{
			int colIndex = findColumn(columnName);
			checkTypeConcurrency();
			resultSet_.updateTimestamp(colIndex, x);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateTimestamp_LL].methodExit();
		}
	}
	  
	public boolean wasNull() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_wasNull].methodEntry();
		try
		{
			checkValidState();
			return resultSet_.wasNull();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_wasNull].methodExit();
		}
	}

	// Joinable methods
	public void unsetMatchColumn(int ai[]) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_unsetMatchColumn_V].methodEntry();
		try
		{
			for(int j = 0; j < ai.length; j++)
			{
				if(ai[j] != iMatchColumns_.get(j))
				{
					throw Messages.createSQLException(locale_, "match_columns_mismatch" ,null);
				}
			}
			for(int k = 0; k < ai.length; k++)
			{
				iMatchColumns_.set(k, -1);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_unsetMatchColumn_V].methodExit();
		}
	}

	public void unsetMatchColumn(String as[]) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_unsetMatchColumn_L].methodEntry();
		try
		{
			for(int i = 0; i < as.length; i++)
			{
				if(!as[i].equals(strMatchColumns_.get(i)))
				{
					throw Messages.createSQLException(locale_, "match_columns_mismatch" ,null);
				}
			}
			strMatchColumns_.clear();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_unsetMatchColumn_L].methodExit();
		}
	}

	public String[] getMatchColumnNames() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getMatchColumnNames].methodEntry();
		try
		{
			if(strMatchColumns_.get(0) == null)
				throw Messages.createSQLException(locale_, "set_match_columns" ,null);
			else
			{
				String as[] = new String[strMatchColumns_.size()];
				strMatchColumns_.copyInto(as);
				return as;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getMatchColumnNames].methodExit();
		}
	}

	public int[] getMatchColumnIndexes() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getMatchColumnIndexes].methodEntry();
		try
		{
			if(iMatchColumns_.get(0) == -1)
				throw Messages.createSQLException(locale_, "set_match_columns" ,null);

			int ai[] = new int[iMatchColumns_.size()];
			for(int j = 0; j < ai.length; j++)
			{
				ai[j] = iMatchColumns_.get(j);
			}
			return ai;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getMatchColumnIndexes].methodExit();
		}
	}

	public void setMatchColumn(int ai[]) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setMatchColumn_V].methodEntry();
		try
		{
			for(int i = 0; i < ai.length; i++)
			{
				if(ai[i] < 0)
					throw Messages.createSQLException(locale_, "match_columns_nonzero" ,null);
			}
			for(int j = 0; j < ai.length; j++)
			{
				iMatchColumns_.add(j, new Integer(ai[j]));
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setMatchColumn_V].methodExit();
		}
	}

	public void setMatchColumn(String as[]) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setMatchColumn_L].methodEntry();
		try
		{
			for(int i = 0; i < as.length; i++)
			{
				if(as[i] == null || as[i].equals(""))
					throw Messages.createSQLException(locale_, "match_columns_empty" ,null);
			}
			for(int j = 0; j < as.length; j++)
			{
				strMatchColumns_.add(j, as[j]);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setMatchColumn_L].methodExit();
		}
	}

	public void setMatchColumn(int i) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setMatchColumn_I].methodEntry();
		try
		{
			if(i < 0)
				throw Messages.createSQLException(locale_, "match_columns_nonzero" ,null);
			else
				iMatchColumns_.set(0, i);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setMatchColumn_I].methodExit();
		}
	}

	public void setMatchColumn(String s) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setMatchColumn_S].methodEntry();
		try
		{
			if(s == null || s.equals(""))
				throw Messages.createSQLException(locale_, "match_columns_empty" ,null);
			else
			{
				s = s.trim();
				strMatchColumns_.set(0, s);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setMatchColumn_S].methodExit();
		}
	}

	public void unsetMatchColumn(int i) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_unsetMatchColumn_I].methodEntry();
		try
		{
			if(!iMatchColumns_.get(0).equals(i))
				throw Messages.createSQLException(locale_, "unset_match_columns_mismatch" ,null);
			if(strMatchColumns_.get(0) != null)
				throw Messages.createSQLException(locale_, "unset_match_columns_colnames" ,null);
			else
				iMatchColumns_.set(0, new Integer(-1));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_unsetMatchColumn_I].methodExit();
		}
	}

	public void unsetMatchColumn(String s) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_unsetMatchColumn_S].methodEntry();
		try
		{
			s = s.trim();
			if(!strMatchColumns_.get(0).equals(s))
				throw Messages.createSQLException(locale_, "unset_match_columns_mismatch" ,null);
			if(((Integer)iMatchColumns_.get(0)).intValue() > 0)
				throw Messages.createSQLException(locale_, "unset_match_columns_colid" ,null);
			else
				strMatchColumns_.set(0, null);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_unsetMatchColumn_S].methodExit();
		}
	}

	// Private methods

	private Connection connect() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_connect].methodEntry();
		try
		{
			// Return existing connection if one exists
			if (conn_ != null)
				return conn_;

			// Atempt connecting using the DataSourceName, if not null
			if(getDataSourceName() != null)
			{
				try
				{
					InitialContext icontext = new InitialContext();
					SQLMXDataSource ds = (SQLMXDataSource)icontext.lookup(getDataSourceName());
					return (SQLMXConnection)ds.getConnection();
				}
				catch(NamingException ne)
				{
					Object[] errMsg		= new Object[1];
					errMsg[0]			= ne.getMessage();
					throw Messages.createSQLException(locale_, "jdbcrowset_connect_error" ,errMsg);
				}
				catch(SQLException sqle)
				{
					Object[] errMsg		= new Object[1];
					errMsg[0]			= sqle.getMessage();
					throw Messages.createSQLException(locale_, "jdbcrowset_connect_error" ,errMsg);
				}
			}
			// If no DataSourceName, then connect using the DriverManager
			if(URL_ != null)
			{
				try
				{
					return DriverManager.getConnection(URL_); 
				}
				catch(SQLException sqle)
				{
					Object[] errMsg		= new Object[2];
					errMsg[0]			= sqle.getMessage();
					errMsg[1]			= new String("(url='" + URL_ + "')");
					throw Messages.createSQLException(locale_, "jdbcrowset_connect_error" ,errMsg);
				}
			}
			else
				return null;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_connect].methodExit();
		}
	}

	private void decodeParameters(Object aobj[], PreparedStatement pStmt) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_decodeParameters].methodEntry();
		try
		{
			for(int i = 0; i < aobj.length; i++)
			{
				if(aobj[i] instanceof Object[])
				{
					Object aobj1[] = (Object[])aobj[i];
					if(aobj1.length == 2)
					{
						if(aobj1[0] == null)
						{
							pStmt.setNull(i+1, ((Integer)aobj1[1]).intValue());
							continue;
						}
						if(aobj1[1] instanceof Calendar)
						{
							if(aobj1[0] instanceof Date)
							{
								pStmt.setDate(i+1, (Date)aobj1[0], (Calendar)aobj1[1]);
								continue;
							}
							if(aobj1[0] instanceof Time)
							{
								pStmt.setTime(i+1, (Time)aobj1[0], (Calendar)aobj1[1]);
								continue;
							}
							if(aobj1[0] instanceof Timestamp)
							{
								pStmt.setTimestamp(i+1, (Timestamp)aobj1[0], (Calendar)aobj1[1]);
							}
							else
							{
								throw Messages.createSQLException(locale_, "param_type_error" ,null);
							}
							continue;
						}
						if(aobj1[0] instanceof Reader)
						{
							pStmt.setCharacterStream(i+1, (Reader)aobj1[0], ((Integer)aobj1[1]).intValue());
							continue;
						} 
						if(aobj1[1] instanceof Integer)
						{
							pStmt.setObject(i+1, aobj1[0], ((Integer)aobj1[1]).intValue());
						}
						continue;
					} 
					else if(aobj1.length == 3)
					{
						if(aobj1[0] == null)
						{
							pStmt.setNull(i+1, ((Integer)aobj1[1]).intValue(), (String)aobj1[2]);
							continue;
						}
						if(aobj1[0] instanceof InputStream)
						{
							switch(((Integer)aobj1[2]).intValue())
							{
								// Call setCharacterStream instead of setUnicodeStream(), which is a depracated API
								case 0:
									Reader iReader = new InputStreamReader((InputStream)aobj1[0]);
									pStmt.setCharacterStream(i+1, iReader, ((Integer)aobj1[1]).intValue());
									continue;
								case 1:
									pStmt.setBinaryStream(i+1, (InputStream)aobj1[0], ((Integer)aobj1[1]).intValue());
									continue;
								case 2:
									pStmt.setAsciiStream(i+1, (InputStream)aobj1[0], ((Integer)aobj1[1]).intValue());
									continue;
								default:
									throw Messages.createSQLException(locale_, "param_type_error" ,null);
							}
						}
						if((aobj1[1] instanceof Integer) && (aobj1[2] instanceof Integer))
							pStmt.setObject(i+1, aobj1[0], ((Integer)aobj1[1]).intValue(), ((Integer)aobj1[2]).intValue());
						else
							throw Messages.createSQLException(locale_, "param_type_error" ,null);
					}
					else  // array length not 2 or 3
						pStmt.setObject(i+1, aobj[i]);
				}
				else  // paramObj not an array
					pStmt.setObject(i+1, aobj[i]);
			} // end for loop
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_decodeParameters].methodExit();
		}
	}

	private void checkTypeConcurrency() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_checkTypeConcurrency].methodEntry();
		try
		{
			if(resultSet_.getType() == ResultSet.TYPE_FORWARD_ONLY)
				throw Messages.createSQLException(locale_, "forward_only_cursor", null);
			if (resultSet_.getConcurrency() == ResultSet.CONCUR_READ_ONLY)
				throw Messages.createSQLException(locale_, "read_only_concur", null);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_checkTypeConcurrency].methodExit();
		}
	}

	void checkValidState() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_checkValidState].methodEntry();
		try
		{
			//Note: Do not check for prepStmt_ == null as this condition occurs when 
			//      a JdbcRowSet is created with an existing resultSet object
			if(conn_ == null || resultSet_ == null)
			{
				Object[] errMsg	= new Object[2];
				if(conn_ == null)
					errMsg[0] = new String("conn_ is null");
				else
					errMsg[0] = new String("");
				if(resultSet_ == null)
					errMsg[1] = new String("resultSet_ is null");
				else
					errMsg[1] = new String("");
				throw Messages.createSQLException(locale_, "invalid_jdbcrowset_state", errMsg);
			}
			else
				return;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_checkValidState].methodExit();
		}
	}

	protected Object[] getParameters() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getParameters].methodEntry();
		try
		{
			Object paramObj[] = new Object[parameters_.size()];

			for(int i = 0; i < parameters_.size(); i++)
			{
				paramObj[i] = parameters_.get(new Integer(i));
				if(paramObj[i] == null)
				{
					Object[] errData	= new Object[1];
					errData[0]			= new Integer(i+1);
					throw Messages.createSQLException(locale_, "missing_jdbcrowset_param", errData);
				}
			}
			return paramObj;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getParameters].methodExit();
		}
	}

	protected void notifyCursorMoved()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_notifyCursorMoved].methodEntry();
		try
		{
			RowSetEvent rowEvent = new RowSetEvent(this);
			for(RowSetListener rsl : listeners_)
				rsl.cursorMoved(rowEvent);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_notifyCursorMoved].methodExit();
		}
	}

	protected void notifyRowChanged()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_notifyRowChanged].methodEntry();
		try
		{
			RowSetEvent rowEvent = new RowSetEvent(this);
			for(RowSetListener rsl : listeners_)
				rsl.rowChanged(rowEvent);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_notifyRowChanged].methodExit();
		}
	}

	protected void notifyRowSetChanged()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_notifyRowSetChanged].methodEntry();
		try
		{
			RowSetEvent rowEvent = new RowSetEvent(this);
			for(RowSetListener rsl : listeners_)
				rsl.rowSetChanged(rowEvent);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_notifyRowSetChanged].methodExit();
		}
	}

	protected void setParameters(ResultSet rs) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setParameters].methodEntry();
		try
		{
			setType(rs.getType());
			setConcurrency(rs.getConcurrency());
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setParameters].methodExit();
		}
	}

	private void setProperties(PreparedStatement pStmt) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setProperties].methodEntry();
		try
		{
			try
			{
				pStmt.setEscapeProcessing(getEscapeProcessing());
			}
			catch(SQLException sqle)
			{
				Object[] errMsg		= new Object[2];
				errMsg[0]			= new String("setEscapeProcessing failed");
				errMsg[1]			= sqle.getMessage();
				throw Messages.createSQLException(locale_, "jdbcrowset_set_props_error", errMsg);
			}
			try
			{
				pStmt.setMaxFieldSize(getMaxFieldSize());
			}
			catch(SQLException sqle1)
			{
				Object[] errMsg		= new Object[2];
				errMsg[0]			= new String("setMaxFieldSize failed");
				errMsg[1]			= sqle1.getMessage();
				throw Messages.createSQLException(locale_, "jdbcrowset_set_props_error", errMsg);
			}
			try
			{
				pStmt.setMaxRows(getMaxRows());
			}
			catch(SQLException sqle2)
			{
				Object[] errMsg		= new Object[2];
				errMsg[0]			= new String("setMaxRows failed");
				errMsg[1]			= sqle2.getMessage();
				throw Messages.createSQLException(locale_, "jdbcrowset_set_props_error", errMsg);
			}
			try
			{
				pStmt.setQueryTimeout(getQueryTimeout());
			}
			catch(SQLException sqle3)
			{
				Object[] errMsg		= new Object[2];
				errMsg[0]			= new String("setQueryTimeout failed");
				errMsg[1]			= sqle3.getMessage();
				throw Messages.createSQLException(locale_, "jdbcrowset_set_props_error", errMsg);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setProperties].methodExit();
		}
	}

	private void validateSetInvocation(int parameterIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_validateSetInvocation].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_validateSetInvocation].methodParameters(
										  Integer.toString(parameterIndex));
		try
		{
			if (parameterIndex < 1)
				throw Messages.createSQLException(locale_,"invalid_parameter_index", null);
		}		
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_validateSetInvocation].methodExit();
		}
	}

	protected void initMD(RowSetMetaData rowsetmd, ResultSetMetaData resultsetmd) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_initMD].methodEntry();
		try
		{
			int i = resultsetmd.getColumnCount();
			rowsetmd.setColumnCount(i);
			for(int j = 1; j <= i; j++)
			{
				rowsetmd.setAutoIncrement(j, resultsetmd.isAutoIncrement(j));
				rowsetmd.setCaseSensitive(j, resultsetmd.isCaseSensitive(j));
				rowsetmd.setCurrency(j, resultsetmd.isCurrency(j));
				rowsetmd.setNullable(j, resultsetmd.isNullable(j));
				rowsetmd.setSigned(j, resultsetmd.isSigned(j));
				rowsetmd.setSearchable(j, resultsetmd.isSearchable(j));
				rowsetmd.setColumnDisplaySize(j, resultsetmd.getColumnDisplaySize(j));
				rowsetmd.setColumnLabel(j, resultsetmd.getColumnLabel(j));
				rowsetmd.setColumnName(j, resultsetmd.getColumnName(j));
				rowsetmd.setSchemaName(j, resultsetmd.getSchemaName(j));
				rowsetmd.setPrecision(j, resultsetmd.getPrecision(j));
				rowsetmd.setScale(j, resultsetmd.getScale(j));
				rowsetmd.setTableName(j, resultsetmd.getTableName(j));
				rowsetmd.setCatalogName(j, resultsetmd.getCatalogName(j));
				rowsetmd.setColumnType(j, resultsetmd.getColumnType(j));
				rowsetmd.setColumnTypeName(j, resultsetmd.getColumnTypeName(j));
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_initMD].methodExit();
		}
	}

	protected void initJdbcRowSet(ResultSet rs) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_initJdbcRowSet].methodEntry();
		try
		{
			setShowDeleted(false);
			setQueryTimeout(0);
			maxRows_ = 0;
			setMaxFieldSize(0);
			setReadOnly(true);
			setTransactionIsolation(Connection.TRANSACTION_READ_COMMITTED); // 2
			setEscapeProcessing(true);
			setTypeMap(null);
			if (rs != null)
				setParameters(rs);

			iMatchColumns_ = new Vector<Integer>();
			strMatchColumns_ = new Vector<String>();

			// In order to trace JdbcRowSet, check for properties and setup traceFile.
			java.util.Properties system_props = System.getProperties();
			String traceFlag = T2Driver.getPropertyValue(system_props,"traceFlag");
			if (traceFlag != null)
			{
				try
				{
					traceFlag_ = Integer.parseInt(traceFlag);
				}
				catch (NumberFormatException e) { } // Do nothing if unable to parse
													// traceFlag string into an int
			}

			if (traceFlag_ >= T2Driver.ENTRY_LVL) 
			{
				traceFile_ = T2Driver.getPropertyValue(system_props,"traceFile");
				if (traceFile_ != null)
				{
					try
					{
						traceWriter_ = new PrintWriter(new FileOutputStream(traceFile_, true), true);			
					}
					catch (java.io.IOException e)
					{
						traceWriter_ = new PrintWriter(System.err, true);
					}
				}
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_initJdbcRowSet].methodExit();
		}
	}

	protected PreparedStatement prepareStmt() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_prepareStmt].methodEntry();
		try
		{
			if (JdbcDebugCfg.traceActive) debug[methodId_prepareStmt].traceOut(
											  JdbcDebug.debugLevelEntry,
											  "Getting a connection");
			conn_ = connect();

			if(conn_ == null)
			{
				Object[] errMsg		= new Object[1];
				errMsg[0]			= new String("Unable to establish a connection");
				throw Messages.createSQLException(locale_, "jdbcrowset_connect_error", errMsg);
			}

			try
			{
				conn_.setTransactionIsolation(getTransactionIsolation());
			}
			catch(SQLException sqle)
			{
				Object[] errMsg		= new Object[2];
				errMsg[0]			= new String("setTransactionIsolation failed");
				errMsg[1]			= sqle.getMessage();
				throw Messages.createSQLException(locale_, "jdbcrowset_set_props_error", errMsg);
			}

			conn_.setTypeMap(getTypeMap());

			try
			{
				prepStmt_ = conn_.prepareStatement(getCommand(), rowSetType_, concurrency_);
				if (JdbcDebugCfg.traceActive)
					debug[methodId_prepareStmt].methodParameters("prepStmt_ created : rowSetType_ = " + rowSetType_ + " : concurrency = " + concurrency_);
			}
			catch(SQLException sqle1)
			{
				if(prepStmt_ != null)
				{
					prepStmt_.close();
					prepStmt_ = null;
				}
				if(conn_ != null)
				{
					conn_.close();
					conn_ = null;
				}
				Object[] errMsg		= new Object[1];
				errMsg[0]			= sqle1.getMessage();
				throw Messages.createSQLException(locale_, "jdbcrowset_prepare_error", errMsg);
			}
			return prepStmt_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_prepareStmt].methodExit();
		}
	}

	// **************  SQLMXJdbcRowSet Constructors *********************8
	public SQLMXJdbcRowSet() throws SQLException
	{
		conn_		= null;
		prepStmt_	= null;
		resultSet_	= null;

		propertyChangeSupport_ = new PropertyChangeSupport(this);
		parameters_ = new Hashtable<Integer,Object>();
		listeners_ = new Vector<RowSetListener>();

		initJdbcRowSet(resultSet_);
	}

	public SQLMXJdbcRowSet(Connection conn) throws SQLException
	{
		conn_		= conn;
		prepStmt_	= null;
		resultSet_	= null;

		propertyChangeSupport_ = new PropertyChangeSupport(this);
		parameters_ = new Hashtable<Integer,Object>();
		listeners_ = new Vector<RowSetListener>();

		initJdbcRowSet(resultSet_);
	}

	public SQLMXJdbcRowSet(String url, String username, String password) throws SQLException
	{
		conn_		= null;
		prepStmt_	= null;
		resultSet_	= null;

		propertyChangeSupport_ = new PropertyChangeSupport(this);
		parameters_ = new Hashtable<Integer,Object>();
		listeners_ = new Vector<RowSetListener>();

		setUsername(username);
		setPassword(password);
		setUrl(url);

		initJdbcRowSet(resultSet_);
		conn_ = connect();
	}

	public SQLMXJdbcRowSet(ResultSet rs) throws SQLException
	{
		conn_		= null;
		prepStmt_	= null;
		resultSet_	= rs;

		propertyChangeSupport_ = new PropertyChangeSupport(this);
		parameters_ = new Hashtable<Integer,Object>();
		listeners_ = new Vector<RowSetListener>();

		initJdbcRowSet(resultSet_);

		resultSetMD_ = resultSet_.getMetaData();
		rowSetMD_ = new RowSetMetaDataImpl();
		initMD(rowSetMD_, resultSetMD_);

		conn_ = connect();
	}

	// Fields
	private Connection			conn_;
	private PreparedStatement	prepStmt_;
	private ResultSet			resultSet_;

	private Map<String,Class<?>> map_;
	private Vector<RowSetListener> listeners_;
	private Hashtable<Integer,Object> parameters_;

	private RowSetMetaDataImpl		rowSetMD_;
	private ResultSetMetaData		resultSetMD_;

    private PropertyChangeSupport   propertyChangeSupport_;

	private Vector<Integer>			iMatchColumns_;
	private Vector<String>			strMatchColumns_;

	private String command_;
	private String URL_ = "jdbc:sqlmx:";

	private String dataSourceName_;
	private int rowSetType_ = ResultSet.TYPE_SCROLL_INSENSITIVE;
	private boolean showDeleted_;
	private int queryTimeout_;
	private int maxRows_;
	private int maxFieldSize_;
	private int concurrency_ = ResultSet.CONCUR_UPDATABLE;
	private boolean readOnly_;
	private boolean escapeProcessing_;
	private int isolation_ = Connection.TRANSACTION_READ_COMMITTED;
	private int fetchSize_;

	private transient String username_;
	private transient String password_;

	public static PrintWriter	traceWriter_;
	private static String		traceFile_;
	private static int			traceFlag_;

	private static Locale		locale_ = null;

	private static int methodId_commit						=  0;
	private static int methodId_getAutoCommit				=  1;
	private static int methodId_getRowSetWarnings			=  2;
	private static int methodId_getWarnings					=  3;
	private static int methodId_rollback_V					=  4;
	private static int methodId_rollback_L					=  5;
	private static int methodId_setAutoCommit				=  6;
	private static int methodId_absolute					=  7;
	private static int methodId_addRowSetListener			=  8;
	private static int methodId_afterLast					=  9;
	private static int methodId_beforeFirst					= 10;
	private static int methodId_clearParameters				= 11;
	private static int methodId_clearWarnings				= 12;
	private static int methodId_cancelRowUpdates			= 13;
	private static int methodId_close						= 14;
	private static int methodId_deleteRow					= 15;
	private static int methodId_execute						= 16;
	private static int methodId_findColumn					= 17;
	private static int methodId_first						= 18;
	private static int methodId_getArray_I					= 19;
	private static int methodId_getArray_L					= 20;
	private static int methodId_getAsciiStream_I			= 21;
	private static int methodId_getAsciiStream_L			= 22;
	private static int methodId_getBigDecimal_I				= 23;
	private static int methodId_getBigDecimal_II			= 24;
	private static int methodId_getBigDecimal_L				= 25;
	private static int methodId_getBigDecimal_LI			= 26;
	private static int methodId_getBinaryStream_I			= 27;
	private static int methodId_getBinaryStream_L			= 28;
	private static int methodId_getBlob_I					= 29;
	private static int methodId_getBlob_L					= 30;
	private static int methodId_getBoolean_I				= 31;
	private static int methodId_getBoolean_L				= 32;
	private static int methodId_getByte_I					= 33;
	private static int methodId_getByte_L					= 34;
	private static int methodId_getBytes_I					= 35;
	private static int methodId_getBytes_L					= 36;
	private static int methodId_getCharacterStream_I		= 37;
	private static int methodId_getCharacterStream_L		= 38;
	private static int methodId_getClob_I					= 39;
	private static int methodId_getClob_L					= 40;
	private static int methodId_getConcurrency				= 41;
	private static int methodId_getCursorName				= 42;
	private static int methodId_getDatabaseMetaData			= 43;
	private static int methodId_getDataSourceName			= 44;
	private static int methodId_getParameterMetaData		= 45;
	private static int methodId_getDate_I					= 46;
	private static int methodId_getDate_IL					= 47;
	private static int methodId_getDate_L					= 48;
	private static int methodId_getDate_SL					= 49;
	private static int methodId_getDouble_I					= 50;
	private static int methodId_getDouble_L					= 51;
	private static int methodId_getFetchDirection			= 52;
	private static int methodId_getFetchSize				= 53;
	private static int methodId_getFloat_I					= 54;
	private static int methodId_getFloat_L					= 55;
	private static int methodId_getInt_I					= 56;
	private static int methodId_getInt_L					= 57;
	private static int methodId_getLong_I					= 58;
	private static int methodId_getLong_L					= 59;
	private static int methodId_getMaxFieldSize				= 60;
	private static int methodId_getMaxRows					= 61;
	private static int methodId_getMetaData					= 62;
	private static int methodId_getObject_I					= 63;
	private static int methodId_getObject_IL				= 64;
	private static int methodId_getObject_L					= 65;
	private static int methodId_getObject_LL				= 66;
	private static int methodId_getPassword					= 67;
	private static int methodId_getQueryTimeout				= 68;
	private static int methodId_getRef_I					= 69;
	private static int methodId_getRef_L					= 70;
	private static int methodId_getRow						= 71;
	private static int methodId_getShort_I					= 72;
	private static int methodId_getShort_L					= 73;
	private static int methodId_getStatement				= 74;
	private static int methodId_getString_I					= 75;
	private static int methodId_getString_L					= 76;
	private static int methodId_getTime_I					= 77;
	private static int methodId_getTime_IL					= 78;
	private static int methodId_getTime_L					= 79;
	private static int methodId_getTime_LL					= 80;
	private static int methodId_getTimestamp_I				= 81;
	private static int methodId_getTimestamp_IL				= 82;
	private static int methodId_getTimestamp_L				= 83;
	private static int methodId_getTimestamp_LL				= 84;
	private static int methodId_getType						= 85;
	private static int methodId_getTypeMap					= 86;
	private static int methodId_getUnicodeStream_I			= 87;
	private static int methodId_getUnicodeStream_L			= 88;
	private static int methodId_getUrl						= 89;
	private static int methodId_getURL_I					= 90;
	private static int methodId_getURL_L					= 91;
	private static int methodId_getUsername					= 92;
	private static int methodId_insertRow					= 93;
	private static int methodId_isAfterLast					= 94;
	private static int methodId_isBeforeFirst				= 95;
	private static int methodId_isFirst						= 96;
	private static int methodId_isLast						= 97;
	private static int methodId_isReadOnly					= 98;
	private static int methodId_last						= 99;
	private static int methodId_moveToCurrentRow			= 100;
	private static int methodId_moveToInsertRow				= 101;
	private static int methodId_next						= 102;
	private static int methodId_previous					= 103;
	private static int methodId_refreshRow					= 104;
	private static int methodId_relative					= 105;
	private static int methodId_removeRowSetListener		= 106;
	private static int methodId_rowDeleted					= 107;
	private static int methodId_rowInserted					= 108;
	private static int methodId_rowUpdated					= 109;
	private static int methodId_setFetchDirection			= 110;
	private static int methodId_setFetchSize				= 111;
	private static int methodId_setArray					= 112;
	private static int methodId_setAsciiStream				= 113;
	private static int methodId_setBigDecimal				= 114;
	private static int methodId_setBinaryStream				= 115;
	private static int methodId_setBlob						= 116;
	private static int methodId_setBoolean					= 117;
	private static int methodId_setByte						= 118;
	private static int methodId_setBytes					= 119;
	private static int methodId_setCharacterStream			= 120;
	private static int methodId_setClob						= 121;
	private static int methodId_setDate_IL					= 122;
	private static int methodId_setDate_ILL					= 123;
	private static int methodId_setDouble					= 124;
	private static int methodId_setEscapeProcessing			= 125;
	private static int methodId_setFloat					= 126;
	private static int methodId_setInt						= 127;
	private static int methodId_setLong						= 128;
	private static int methodId_setMaxFieldSize				= 129;
	private static int methodId_setMaxRows					= 130;
	private static int methodId_setNull_II					= 131;
	private static int methodId_setNull_IIL					= 132;
	private static int methodId_setObject_IL				= 133;
	private static int methodId_setObject_ILI				= 134;
	private static int methodId_setObject_ILII				= 135;
	private static int methodId_setPassword					= 136;
	private static int methodId_setQueryTimeout				= 137;
	private static int methodId_setReadOnly					= 138;
	private static int methodId_setRef						= 139;
	private static int methodId_setShort					= 140;
	private static int methodId_setString					= 141;
	private static int methodId_setTime_IL					= 142;
	private static int methodId_setTime_ILL					= 143;
	private static int methodId_setTimestamp_IL				= 144;
	private static int methodId_setTimestamp_ILL			= 145;
	private static int methodId_setTransactionIsolation		= 146;
	private static int methodId_setType						= 147;
	private static int methodId_setTypeMap					= 148;
	private static int methodId_setUrl						= 149;
	private static int methodId_setUsername					= 150;
	private static int methodId_updateArray_IL				= 151;
	private static int methodId_updateArray_LL				= 152;
	private static int methodId_updateAsciiStream_ILI		= 153;
	private static int methodId_updateAsciiStream_LLI		= 154;
	private static int methodId_updateBigDecimal_IL			= 155;
	private static int methodId_updateBigDecimal_LL			= 156;
	private static int methodId_updateBinaryStream_ILI		= 157;
	private static int methodId_updateBinaryStream_LLI		= 158;
	private static int methodId_updateBlob_IL				= 159;
	private static int methodId_updateBlob_LL				= 160;
	private static int methodId_updateBoolean_IZ			= 161;
	private static int methodId_updateBoolean_LZ			= 162;
	private static int methodId_updateByte_IZ				= 163;
	private static int methodId_updateByte_LZ				= 164;
	private static int methodId_updateBytes_IZ				= 165;
	private static int methodId_updateBytes_LZ				= 166;
	private static int methodId_updateCharacterStream_ILI	= 167;
	private static int methodId_updateCharacterStream_LLI	= 168;
	private static int methodId_updateClob_IL				= 169;
	private static int methodId_updateClob_LL				= 170;
	private static int methodId_updateDate_IL				= 171;
	private static int methodId_updateDate_LL				= 172;
	private static int methodId_updateDouble_ID				= 173;
	private static int methodId_updateDouble_LD				= 174;
	private static int methodId_updateFloat_IF				= 175;
	private static int methodId_updateFloat_LF				= 176;
	private static int methodId_updateInt_II				= 177;
	private static int methodId_updateInt_LI				= 178;
	private static int methodId_updateLong_IJ				= 179;
	private static int methodId_updateLong_LJ				= 180;
	private static int methodId_updateNull_I				= 181;
	private static int methodId_updateNull_L				= 182;
	private static int methodId_updateObject_IL				= 183;
	private static int methodId_updateObject_ILI			= 184;
	private static int methodId_updateObject_LL				= 185;
	private static int methodId_updateObject_LLI			= 186;
	private static int methodId_updateRef_IL				= 187;
	private static int methodId_updateRef_LL				= 188;
	private static int methodId_updateRow					= 189;
	private static int methodId_updateShort_IS				= 190;
	private static int methodId_updateShort_LS				= 191;
	private static int methodId_updateString_IL				= 192;
	private static int methodId_updateString_LL				= 193;
	private static int methodId_updateTime_IL				= 194;
	private static int methodId_updateTime_LL				= 195;
	private static int methodId_updateTimestamp_IL			= 196;
	private static int methodId_updateTimestamp_LL			= 197;
	private static int methodId_wasNull						= 198;
	private static int methodId_unsetMatchColumn_V			= 199;
	private static int methodId_unsetMatchColumn_L			= 200;
	private static int methodId_getMatchColumnNames			= 201;
	private static int methodId_getMatchColumnIndexes		= 202;
	private static int methodId_setMatchColumn_V			= 203;
	private static int methodId_setMatchColumn_L			= 204;
	private static int methodId_setMatchColumn_I			= 205;
	private static int methodId_setMatchColumn_S			= 206;
	private static int methodId_unsetMatchColumn_I			= 207;
	private static int methodId_unsetMatchColumn_S			= 208;
	private static int methodId_connect						= 209;
	private static int methodId_decodeParameters			= 210;
	private static int methodId_checkTypeConcurrency		= 211;
	private static int methodId_checkValidState				= 212;
	private static int methodId_notifyCursorMoved			= 213;
	private static int methodId_notifyRowChanged			= 214;
	private static int methodId_notifyRowSetChanged			= 215;
	private static int methodId_setParameters				= 216;
	private static int methodId_setProperties				= 217;
	private static int methodId_validateSetInvocation		= 218;
	private static int methodId_initMD						= 219;
	private static int methodId_getEscapeProcessing			= 220;
	private static int methodId_initJdbcRowSet				= 221;
	private static int methodId_prepareStmt					= 222;
	private static int methodId_getCommand					= 223;
	private static int methodId_getTransactionIsolation		= 224;
	private static int methodId_getShowDeleted				= 225;
	private static int methodId_setShowDeleted				= 226;
	private static int methodId_setCommand					= 227;
	private static int methodId_setConcurrency				= 228;
	private static int methodId_setDataSourceName			= 229;
	private static int methodId_getParameters				= 230;

	private static int totalMethodIds						= 231;

	private static JdbcDebug[] debug;

	static
	{
		String className = "SQLMXJdbcRowSet";

		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_commit] = new JdbcDebug(className,"methodId_commit");
			debug[methodId_getAutoCommit] = new JdbcDebug(className,"methodId_getAutoCommit");
			debug[methodId_getRowSetWarnings] = new JdbcDebug(className,"methodId_getRowSetWarnings");
			debug[methodId_getWarnings] = new JdbcDebug(className,"methodId_getWarnings");
			debug[methodId_rollback_V] = new JdbcDebug(className,"methodId_rollback_V"); 
			debug[methodId_rollback_L] = new JdbcDebug(className,"methodId_rollback_L");
			debug[methodId_setAutoCommit] = new JdbcDebug(className,"methodId_setAutoCommit");
			debug[methodId_absolute] = new JdbcDebug(className,"absolute");
			debug[methodId_addRowSetListener] = new JdbcDebug(className,"addRowSetListener");
			debug[methodId_afterLast] = new JdbcDebug(className,"afterLast");
			debug[methodId_beforeFirst] = new JdbcDebug(className,"beforeFirst");
			debug[methodId_clearParameters] = new JdbcDebug(className,"clearParameters");
			debug[methodId_clearWarnings] = new JdbcDebug(className,"clearWarnings");
			debug[methodId_cancelRowUpdates] = new JdbcDebug(className,"cancelRowUpdates");
			debug[methodId_close] = new JdbcDebug(className,"close");
			debug[methodId_deleteRow] = new JdbcDebug(className,"deleteRow");
			debug[methodId_execute] = new JdbcDebug(className,"execute");
			debug[methodId_findColumn] = new JdbcDebug(className,"findColumn");
			debug[methodId_first] = new JdbcDebug(className,"first");
			debug[methodId_getArray_I] = new JdbcDebug(className,"getArray_I");
			debug[methodId_getArray_L] = new JdbcDebug(className,"getArray_L");
			debug[methodId_getAsciiStream_I] = new JdbcDebug(className,"getAsciiStream[I]");
			debug[methodId_getAsciiStream_L] = new JdbcDebug(className,"getAsciiStream[L]");
			debug[methodId_getBigDecimal_I] = new JdbcDebug(className,"getBigDecimal[I]");
			debug[methodId_getBigDecimal_II] = new JdbcDebug(className,"getBigDecimal[II]");
			debug[methodId_getBigDecimal_L] = new JdbcDebug(className,"getBigDecimal[L]");
			debug[methodId_getBigDecimal_LI] = new JdbcDebug(className,"getBigDecimal[LI]");
			debug[methodId_getBinaryStream_I] = new JdbcDebug(className,"getBinaryStream[I]");
			debug[methodId_getBinaryStream_L] = new JdbcDebug(className,"getBinaryStream[L]");
			debug[methodId_getBlob_I] = new JdbcDebug(className,"getBlob[I]");
			debug[methodId_getBlob_L] = new JdbcDebug(className,"getBlob[L]");
			debug[methodId_getBoolean_I] = new JdbcDebug(className,"getBoolean[I]");
			debug[methodId_getBoolean_L] = new JdbcDebug(className,"getBoolean[L]");
			debug[methodId_getByte_I] = new JdbcDebug(className,"getByte[I]");
			debug[methodId_getByte_L] = new JdbcDebug(className,"getByte[L]");
			debug[methodId_getBytes_I] = new JdbcDebug(className,"getBytes[I]");
			debug[methodId_getBytes_L] = new JdbcDebug(className,"getBytes[L]");
			debug[methodId_getCharacterStream_I] = new JdbcDebug(className,"getCharacterStream[I]");
			debug[methodId_getCharacterStream_L] = new JdbcDebug(className,"getCharacterStream[L]");
			debug[methodId_getClob_I] = new JdbcDebug(className,"getClob[I]");
			debug[methodId_getClob_L] = new JdbcDebug(className,"getClob[L]");
			debug[methodId_getConcurrency] = new JdbcDebug(className,"getConcurrency");
			debug[methodId_getCursorName] = new JdbcDebug(className,"getCursorName");
			debug[methodId_getDatabaseMetaData] = new JdbcDebug(className,"getDatabaseMetaData");
			debug[methodId_getParameterMetaData] = new JdbcDebug(className,"getParameterMetaData");
			debug[methodId_getDataSourceName] = new JdbcDebug(className,"getDataSourceName");
			debug[methodId_getDate_I] = new JdbcDebug(className,"getDate[I]");
			debug[methodId_getDate_IL] = new JdbcDebug(className,"getDate[IL]");
			debug[methodId_getDate_L] = new JdbcDebug(className,"getDate[L]");
			debug[methodId_getDate_SL] = new JdbcDebug(className,"getDate[SL]");
			debug[methodId_getDouble_I] = new JdbcDebug(className,"getDouble[I]");
			debug[methodId_getDouble_L] = new JdbcDebug(className,"getDouble[L]");
			debug[methodId_getFetchDirection] = new JdbcDebug(className,"getFetchDirection");
			debug[methodId_getFetchSize] = new JdbcDebug(className,"getFetchSize");
			debug[methodId_getFloat_I] = new JdbcDebug(className,"getFloat[I]");
			debug[methodId_getFloat_L] = new JdbcDebug(className,"getFloat[L]");
			debug[methodId_getInt_I] = new JdbcDebug(className,"getInt[I]");
			debug[methodId_getInt_L] = new JdbcDebug(className,"getInt[L]");
			debug[methodId_getLong_I] = new JdbcDebug(className,"getLong[I]");
			debug[methodId_getLong_L] = new JdbcDebug(className,"getLong[L]");
			debug[methodId_getMaxFieldSize] = new JdbcDebug(className,"getMaxFieldSize");
			debug[methodId_getMaxRows] = new JdbcDebug(className,"getMaxRows");
			debug[methodId_getMetaData] = new JdbcDebug(className,"getMetaData");
			debug[methodId_getObject_I] = new JdbcDebug(className,"getObject[I]");
			debug[methodId_getObject_IL] = new JdbcDebug(className,"getObject[IL]");
			debug[methodId_getObject_L] = new JdbcDebug(className,"getObject[L]");
			debug[methodId_getObject_LL] = new JdbcDebug(className,"getObject[LL]");
			debug[methodId_getPassword] = new JdbcDebug(className,"getPassword");
			debug[methodId_getQueryTimeout] = new JdbcDebug(className,"getQueryTimeeout");
			debug[methodId_getRef_I] = new JdbcDebug(className,"getRef[I]");
			debug[methodId_getRef_L] = new JdbcDebug(className,"getRef[L]");
			debug[methodId_getRow] = new JdbcDebug(className,"getRow");
			debug[methodId_getShort_I] = new JdbcDebug(className,"getShort[I]");
			debug[methodId_getShort_L] = new JdbcDebug(className,"getShort[L]");
			debug[methodId_getStatement] = new JdbcDebug(className,"getStatement");
			debug[methodId_getString_I] = new JdbcDebug(className,"getString[I]");
			debug[methodId_getString_L] = new JdbcDebug(className,"getString[L]");
			debug[methodId_getTime_I] = new JdbcDebug(className,"getTime[I]");
			debug[methodId_getTime_IL] = new JdbcDebug(className,"getTime[IL]");
			debug[methodId_getTime_L] = new JdbcDebug(className,"getTime[L]");
			debug[methodId_getTime_LL] = new JdbcDebug(className,"getTime[LL]");
			debug[methodId_getTimestamp_I] = new JdbcDebug(className,"getTimestamp[I]");
			debug[methodId_getTimestamp_IL] = new JdbcDebug(className,"getTimestamp[IL]");
			debug[methodId_getTimestamp_L] = new JdbcDebug(className,"getTimestamp[L]");
			debug[methodId_getTimestamp_LL] = new JdbcDebug(className,"getTimestamp[LL]");
			debug[methodId_getType] = new JdbcDebug(className,"getType");
			debug[methodId_getTypeMap] = new JdbcDebug(className,"getTypeMap");
			debug[methodId_getUnicodeStream_I] = new JdbcDebug(className,"getUnicodeStream[I]");
			debug[methodId_getUnicodeStream_L] = new JdbcDebug(className,"getUnicodeStream[L]");
			debug[methodId_getUrl] = new JdbcDebug(className,"getUrl");
			debug[methodId_getURL_I] = new JdbcDebug(className,"getURL[I]");
			debug[methodId_getURL_L] = new JdbcDebug(className,"getURL[L]");
			debug[methodId_getUsername] = new JdbcDebug(className,"getUsername");
			debug[methodId_insertRow] = new JdbcDebug(className,"insertRow");
			debug[methodId_isAfterLast] = new JdbcDebug(className,"isAfterLast");
			debug[methodId_isBeforeFirst] = new JdbcDebug(className,"isBeforeFirst");
			debug[methodId_isFirst] = new JdbcDebug(className,"isFirst");
			debug[methodId_isLast] = new JdbcDebug(className,"isLast");
			debug[methodId_isReadOnly] = new JdbcDebug(className,"isReadOnly");
			debug[methodId_last] = new JdbcDebug(className,"last");
			debug[methodId_moveToCurrentRow] = new JdbcDebug(className,"moveToCurrentRow");
			debug[methodId_moveToInsertRow] = new JdbcDebug(className,"moveToInsertRow");
			debug[methodId_next] = new JdbcDebug(className,"next");
			debug[methodId_previous] = new JdbcDebug(className,"previous");
			debug[methodId_refreshRow] = new JdbcDebug(className,"refreshRow");
			debug[methodId_relative] = new JdbcDebug(className,"relative");
			debug[methodId_removeRowSetListener] = new JdbcDebug(className,"removeRowSetListener");
			debug[methodId_rowDeleted] = new JdbcDebug(className,"rowDeleted");
			debug[methodId_rowInserted] = new JdbcDebug(className,"rowInserted");
			debug[methodId_rowUpdated] = new JdbcDebug(className,"rowUpdated");
			debug[methodId_setFetchDirection] = new JdbcDebug(className,"setFetchDirection");
			debug[methodId_setFetchSize] = new JdbcDebug(className,"setFetchSize");
			debug[methodId_setArray] = new JdbcDebug(className,"setArray");
			debug[methodId_setAsciiStream] = new JdbcDebug(className,"setAsciiStream");
			debug[methodId_setBigDecimal] = new JdbcDebug(className,"setBigDecimal");
			debug[methodId_setBinaryStream] = new JdbcDebug(className,"setBinaryStream");
			debug[methodId_setBlob] = new JdbcDebug(className,"setBlob");
			debug[methodId_setBoolean] = new JdbcDebug(className,"setBoolean");
			debug[methodId_setByte] = new JdbcDebug(className,"setByte");
			debug[methodId_setBytes] = new JdbcDebug(className,"setBytes");
			debug[methodId_setCharacterStream] = new JdbcDebug(className,"setCharacterStream");
			debug[methodId_setClob] = new JdbcDebug(className,"setClob");
			debug[methodId_setDate_IL] = new JdbcDebug(className,"setDate_IL");
			debug[methodId_setDate_ILL] = new JdbcDebug(className,"setDate_ILL");
			debug[methodId_setDouble] = new JdbcDebug(className,"setDouble");
			debug[methodId_setEscapeProcessing] = new JdbcDebug(className,"setEscapeProcessing");
			debug[methodId_setFloat] = new JdbcDebug(className,"setFloat");
			debug[methodId_setInt] = new JdbcDebug(className,"setInt");
			debug[methodId_setLong] = new JdbcDebug(className,"setLong");
			debug[methodId_setMaxFieldSize] = new JdbcDebug(className,"setMaxFieldSize");
			debug[methodId_setMaxRows] = new JdbcDebug(className,"setMaxRows");
			debug[methodId_setNull_II] = new JdbcDebug(className,"setNull_II");
			debug[methodId_setNull_IIL] = new JdbcDebug(className,"setNull_IIL");
			debug[methodId_setObject_IL] = new JdbcDebug(className,"setObject_IL");
			debug[methodId_setObject_ILI] = new JdbcDebug(className,"setObject_ILI");
			debug[methodId_setObject_ILII] = new JdbcDebug(className,"setObject_ILII");
			debug[methodId_setPassword] = new JdbcDebug(className,"setPassword");
			debug[methodId_setQueryTimeout] = new JdbcDebug(className,"setQueryTimeout");
			debug[methodId_setReadOnly] = new JdbcDebug(className,"setReadOnly");
			debug[methodId_setRef] = new JdbcDebug(className,"setRef");
			debug[methodId_setShort] = new JdbcDebug(className,"setShort");
			debug[methodId_setString] = new JdbcDebug(className,"setString");
			debug[methodId_setTime_IL] = new JdbcDebug(className,"setTime_IL");
			debug[methodId_setTime_ILL] = new JdbcDebug(className,"setTime_ILL");
			debug[methodId_setTimestamp_IL] = new JdbcDebug(className,"setTimestamp_IL");
			debug[methodId_setTimestamp_ILL] = new JdbcDebug(className,"setTimestamp_ILL");
			debug[methodId_setTransactionIsolation] = new JdbcDebug(className,"setTransactionIsolation");
			debug[methodId_setType] = new JdbcDebug(className,"setType");
			debug[methodId_setTypeMap] = new JdbcDebug(className,"setTypeMap");
			debug[methodId_setUrl] = new JdbcDebug(className,"setUrl");
			debug[methodId_setUsername] = new JdbcDebug(className,"setUsername");
			debug[methodId_updateArray_IL] = new JdbcDebug(className,"updateArray[IL]");
			debug[methodId_updateArray_LL] = new JdbcDebug(className,"updateArray[LL]");
			debug[methodId_updateAsciiStream_ILI] = new JdbcDebug(className,"updateAsciiStream[ILI]");
			debug[methodId_updateAsciiStream_LLI] = new JdbcDebug(className,"updateAsciiStream[LLI]");
			debug[methodId_updateBigDecimal_IL] = new JdbcDebug(className,"updateBigDecimal[IL]");
			debug[methodId_updateBigDecimal_LL] = new JdbcDebug(className,"updateBigDecimal[LL]");
			debug[methodId_updateBinaryStream_ILI] = new JdbcDebug(className,"updateBinaryStream[ILI]");
			debug[methodId_updateBinaryStream_LLI] = new JdbcDebug(className,"updateBinaryStream[LLI]");
			debug[methodId_updateBlob_IL] = new JdbcDebug(className,"updateBlob[IL]");
			debug[methodId_updateBlob_LL] = new JdbcDebug(className,"updateBlob[LL]");
			debug[methodId_updateBoolean_IZ] = new JdbcDebug(className,"updateBoolean[IZ]");
			debug[methodId_updateBoolean_LZ] = new JdbcDebug(className,"updateBoolean[LZ]");
			debug[methodId_updateByte_IZ] = new JdbcDebug(className,"updateByte[IZ]");
			debug[methodId_updateByte_LZ] = new JdbcDebug(className,"updateByte[LZ]");
			debug[methodId_updateBytes_IZ] = new JdbcDebug(className,"updateBytes[IZ]");
			debug[methodId_updateBytes_LZ] = new JdbcDebug(className,"updateBytes[LZ]");
			debug[methodId_updateCharacterStream_ILI] = new JdbcDebug(className,"updateCharacterStream[ILI]");
			debug[methodId_updateCharacterStream_LLI] = new JdbcDebug(className,"updateCharacterStream[LLI]");
			debug[methodId_updateClob_IL] = new JdbcDebug(className,"updateClob[IL]");
			debug[methodId_updateClob_LL] = new JdbcDebug(className,"updateClob[LL]");
			debug[methodId_updateDate_IL] = new JdbcDebug(className,"updateDate[IL]");
			debug[methodId_updateDate_LL] = new JdbcDebug(className,"updateDate[LL]");
			debug[methodId_updateDouble_ID] = new JdbcDebug(className,"updateDouble[ID]");
			debug[methodId_updateDouble_LD] = new JdbcDebug(className,"updateDouble[LD]");
			debug[methodId_updateFloat_IF] = new JdbcDebug(className,"updateFloat[IF]");
			debug[methodId_updateFloat_LF] = new JdbcDebug(className,"updateFloat[LF]");
			debug[methodId_updateInt_II] = new JdbcDebug(className,"updateInt[II]");
			debug[methodId_updateInt_LI] = new JdbcDebug(className,"updateInt[LI]");
			debug[methodId_updateLong_IJ] = new JdbcDebug(className,"updateLong[IJ]");
			debug[methodId_updateLong_LJ] = new JdbcDebug(className,"updateLong[LJ]");
			debug[methodId_updateNull_I] = new JdbcDebug(className,"updateNull[I]");
			debug[methodId_updateNull_L] = new JdbcDebug(className,"updateNull[L]");
			debug[methodId_updateObject_IL] = new JdbcDebug(className,"updateObject[IL]");
			debug[methodId_updateObject_ILI] = new JdbcDebug(className,"updateObject[ILI]");
			debug[methodId_updateObject_LL] = new JdbcDebug(className,"updateObject[LL]");
			debug[methodId_updateObject_LLI] = new JdbcDebug(className,"updateObject[LLI]");
			debug[methodId_updateRef_IL] = new JdbcDebug(className,"updateRef[IL]");
			debug[methodId_updateRef_LL] = new JdbcDebug(className,"updateRef[LL]");
			debug[methodId_updateRow] = new JdbcDebug(className,"updateRow");
			debug[methodId_updateShort_IS] = new JdbcDebug(className,"updateShort[IS]");
			debug[methodId_updateShort_LS] = new JdbcDebug(className,"updateShort[LS]");
			debug[methodId_updateString_IL] = new JdbcDebug(className,"updateString[IL]");
			debug[methodId_updateString_LL] = new JdbcDebug(className,"updateString[LL]");
			debug[methodId_updateTime_IL] = new JdbcDebug(className,"updateTime[IL]");
			debug[methodId_updateTime_LL] = new JdbcDebug(className,"updateTime[LL]");
			debug[methodId_updateTimestamp_IL] = new JdbcDebug(className,"updateTimestamp[IL]");
			debug[methodId_updateTimestamp_LL] = new JdbcDebug(className,"updateTimestamp[LL]");
			debug[methodId_wasNull] = new JdbcDebug(className,"wasNull");
			debug[methodId_unsetMatchColumn_V] = new JdbcDebug(className,"unsetMatchColumn[V]");
			debug[methodId_unsetMatchColumn_L] = new JdbcDebug(className,"unsetMatchColumn[L]");
			debug[methodId_getMatchColumnNames] = new JdbcDebug(className,"getMatchColumnNames");
			debug[methodId_getMatchColumnIndexes] = new JdbcDebug(className,"getMatchColumnIndexes");
			debug[methodId_setMatchColumn_V] = new JdbcDebug(className,"setMatchColumn[V]");
			debug[methodId_setMatchColumn_L] = new JdbcDebug(className,"setMatchColumn[L]");
			debug[methodId_setMatchColumn_I] = new JdbcDebug(className,"setMatchColumn[I]");
			debug[methodId_setMatchColumn_S] = new JdbcDebug(className,"setMatchColumn[S]");
			debug[methodId_unsetMatchColumn_I] = new JdbcDebug(className,"unsetMatchColumn[I]");
			debug[methodId_unsetMatchColumn_S] = new JdbcDebug(className,"unsetMatchColumn[S]");
			debug[methodId_connect] = new JdbcDebug(className,"connect");
			debug[methodId_decodeParameters] = new JdbcDebug(className,"decodeParameters");
			debug[methodId_checkTypeConcurrency] = new JdbcDebug(className,"checkTypeConcurrency");
			debug[methodId_checkValidState] = new JdbcDebug(className,"checkValidState");
			debug[methodId_notifyCursorMoved] = new JdbcDebug(className,"notifyCursorMoved");
			debug[methodId_notifyRowChanged] = new JdbcDebug(className,"notifyRowChanged");
			debug[methodId_notifyRowSetChanged] = new JdbcDebug(className,"notifyRowSetChanged");
			debug[methodId_setParameters] = new JdbcDebug(className,"setParameters");
			debug[methodId_setProperties] = new JdbcDebug(className,"setProperties");
			debug[methodId_validateSetInvocation] = new JdbcDebug(className,"validateSetInvocation");
			debug[methodId_initMD] = new JdbcDebug(className,"initMD");
			debug[methodId_prepareStmt] = new JdbcDebug(className,"prepareStmt");
			debug[methodId_getCommand] = new JdbcDebug(className,"getCommand");
			debug[methodId_getShowDeleted] = new JdbcDebug(className,"getShowDeleted");
			debug[methodId_setShowDeleted] = new JdbcDebug(className,"setShowDeleted");
			debug[methodId_setCommand] = new JdbcDebug(className,"setCommand");
			debug[methodId_setConcurrency] = new JdbcDebug(className,"setConcurrency");
			debug[methodId_setDataSourceName] = new JdbcDebug(className,"setDataSourceName");
			debug[methodId_getParameters] = new JdbcDebug(className,"getParameters");
			debug[methodId_initJdbcRowSet] = new JdbcDebug(className,"initJdbcRowSet");
			debug[methodId_getTransactionIsolation] = new JdbcDebug(className,"getTransactionIsolation");
			debug[methodId_getEscapeProcessing] = new JdbcDebug(className,"getEscapeProcessing");
		}

		java.util.Properties system_props = System.getProperties();
		String lang = T2Driver.getPropertyValue(system_props,"language");
		if (lang == null)
			locale_ = Locale.getDefault();
		else
		{
			if (lang.equalsIgnoreCase("ja"))
				locale_ = new Locale("ja", "", "");
			else
				if (lang.equalsIgnoreCase("en"))
				locale_ = new Locale("en", "", "");
			else
				locale_ = Locale.getDefault();
		}
	}
         public void setURL(int parameterIndex, URL x) throws SQLException {
         }

	public void setNClob(int parameterIndex, Reader reader) throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setNClob(int parameterIndex, NClob value) throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setNClob(int parameterIndex, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setNClob(String parameterName, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setNClob(String parameterName, NClob value) throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setNClob(String parameterName, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setNCharacterStream(String parameterName, Reader value,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNCharacterStream(String parameterName, Reader value)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNCharacterStream(int parameterIndex, Reader value,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNCharacterStream(int parameterIndex, Reader value)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setNString(String parameterName, String value)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setNString(int parameterIndex, String value)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setRowId(int parameterIndex, RowId x) throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setRowId(String parameterName, RowId x) throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setSQLXML(String parameterName, SQLXML xmlObject)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public SQLXML getSQLXML(int parameterIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public SQLXML getSQLXML(String parameterName) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
	public void setSQLXML(int parameterIndex, SQLXML xmlObject)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setTimestamp(String str, Timestamp timestamp, Calendar calendar) throws SQLException {
        }

	public void setTimestamp(String str, Timestamp timestamp)
			throws SQLException {
        }
	public void setTime(String str, Time time, Calendar calendar)
			throws SQLException {
        }
	public void setTime(String str, Time time)
			throws SQLException {
        }
	public void setDate(String str, Date date, Calendar calendar)
			throws SQLException {
        }
	public void setDate(String str, Date date)
			throws SQLException {
        }
	public void setBlob(String parameterName, Blob blob)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setBlob(String parameterName, Blob blob,long length)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setBlob(String parameterName, InputStream inputStream, long length) throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setClob(String parameterName, Clob clob)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void setClob(int parameterIndex, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setClob(int parameterIndex, Reader reader,long length)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setClob(String parameterName, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setClob(String parameterName, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setBlob(int parameterIndex, InputStream inputStream, long length)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setBlob(int parameterIndex, InputStream inputStream)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public void setBlob(String parameterName, InputStream inputStream)
			throws SQLException {
		// TODO Auto-generated method stub
	}
	public Object getObject(int parameterIndex, Class type)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Object getObject(String parameterName, Class type)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
        public void setObject(String str, Object obj) throws SQLException {
        }
        public void setObject(String str, Object obj, int parameterIndex) throws SQLException { }
        public void setObject(String str, Object obj, int parameterIndex, int p1) throws SQLException { }
	public void setCharacterStream(String parameterString, Reader x) throws SQLException {}
	public void setCharacterStream(String parameterString, Reader x, int index) throws SQLException {}
	public void setCharacterStream(int parameterIndex, Reader x) throws SQLException {}
	public void setBinaryStream(String parameterString, InputStream x) throws SQLException{}
	public void setBinaryStream(String parameterString, InputStream x, int index) throws SQLException{}
	public void setBinaryStream(int parameterIndex, InputStream x) throws SQLException{}
	public void setAsciiStream(int parameterIndex, InputStream x) throws SQLException{}
	public void setAsciiStream(String parameterName, InputStream x) throws SQLException{}
	public void setAsciiStream(String parameterName, InputStream x, int index) throws SQLException{}
	public void setBytes(String parameterName, byte[] x) throws SQLException{}
	public void setString(String parameterName, String x) throws SQLException{}

	public void setBigDecimal(String parameterName, BigDecimal x) throws SQLException{}
	public void setDouble(String parameterIndex, double x) throws SQLException{}
	public void setFloat(String parameterIndex, float x) throws SQLException{}
	public void setLong(String parameterIndex, long x) throws SQLException{}
	public void setInt(String parameterIndex, int x) throws SQLException{}
	public void setShort(String parameterIndex, short x) throws SQLException{}
	public void setString(String parameterName, byte x) throws SQLException{}
	public void setByte(String parameterName, byte x) throws SQLException{}
	public void setBoolean(String parameterName, boolean x) throws SQLException{}
	public void setNull(String parameterName, int t, String k) throws SQLException{}
	public void setNull(String parameterName, int t) throws SQLException{}
	public void updateNClob(int columnIndex, Reader reader) throws SQLException {
		// TODO Auto-generated method stub
		
	}

        public void updateClob(int columnIndex, Reader reader )
                        throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateClob(int columnIndex, Reader reader, long length)
                        throws SQLException {
                // TODO Auto-generated method stub

        }

        public void updateClob(String columnLabel, Reader reader, long length)
                        throws SQLException {
                // TODO Auto-generated method stub
        }

        public void updateNClob(String columnLabel, Reader reader, long length)
                        throws SQLException {
                // TODO Auto-generated method stub
        }

	public void updateNClob(String columnLabel, Reader reader) throws SQLException {
		// TODO Auto-generated method stub
        }
	public void updateClob(String columnLabel, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
        }

        public void updateBlob(int columnIndex, InputStream inputStream)
                        throws SQLException {
                // TODO Auto-generated method stub
        }
        public void updateBlob(int columnIndex, InputStream inputStream, long length)
                        throws SQLException {
                // TODO Auto-generated method stub
        }

        public void updateBlob(String columnLabel, InputStream inputStream,
                        long length) throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateBlob(String columnLabel, InputStream inputStream) throws SQLException {
                // TODO Auto-generated method stub

        }

        public void updateCharacterStream(String columnLabel, Reader reader) throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateAsciiStream(String columnLabel, InputStream x)
                        throws SQLException {
                // TODO Auto-generated method stub

        }

        public void updateBinaryStream(String columnLabel, InputStream x) throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateCharacterStream(int columnIndex, Reader x)
                        throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateAsciiStream(int columnIndex, InputStream x)
                        throws SQLException {
                // TODO Auto-generated method stub

        }

        public void updateBinaryStream(int columnIndex, InputStream x)
                        throws SQLException {
                // TODO Auto-generated method stub

        }

        public void updateNCharacterStream(int columnIndex, Reader x)
                        throws SQLException {
                // TODO Auto-generated method stub

        }

        public void updateNCharacterStream(String columnLabel, Reader reader)
                        throws SQLException {
                // TODO Auto-generated method stub

        }

        public void updateNCharacterStream(int columnLabel, Reader reader, long s) throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateCharacterStream(String columnLabel, Reader reader, long s) throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateNCharacterStream(String columnLabel, Reader reader, long s) throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateNClob(int columnIndex, Reader reader, long length)
                        throws SQLException {
                // TODO Auto-generated method stub

        }
       public void updateBinaryStream(String columnLabel, InputStream x, long t) throws SQLException {
                // TODO Auto-generated method stub

        }

       public void updateAsciiStream(String columnLabel, InputStream x, long t) throws SQLException {
                // TODO Auto-generated method stub

        }
        public void updateCharacterStream(int columnIndex, Reader reader, long length)
                        throws SQLException {
                // TODO Auto-generated method stub

        }
       public void updateBinaryStream(int columnLabel, InputStream x, long t) throws SQLException {
                // TODO Auto-generated method stub

        }
       public void updateAsciiStream(int columnLabel, InputStream x, long t) throws SQLException {
                // TODO Auto-generated method stub

        }
	public Reader getNCharacterStream(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Reader getNCharacterStream(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
	public String getNString(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public String getNString(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
	public void updateSQLXML(int columnIndex, SQLXML xmlObject)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateSQLXML(String columnLabel, SQLXML xmlObject)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public NClob getNClob(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
	public NClob getNClob(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
	public void updateNClob(int columnIndex, NClob nClob) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNClob(String columnLabel, NClob nClob)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	public void updateNString(int columnIndex, String nString)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNString(String columnLabel, String nString)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
        public boolean isClosed() throws SQLException {
               return false;
        }
	public Object unwrap(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public boolean isWrapperFor(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}
	public RowId getRowId(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public RowId getRowId(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public void updateRowId(int columnIndex, RowId x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateRowId(String columnLabel, RowId x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public int getHoldability() throws SQLException {
		// TODO Auto-generated method stub
		return 0;
	}
}	
