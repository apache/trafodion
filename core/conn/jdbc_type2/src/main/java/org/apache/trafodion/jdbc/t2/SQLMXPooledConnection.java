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
 * Filename	: SQLMXPooledConnection.java
 * Description :
 */
/*
 * Methods Changed:SQLMXPooledConnection(SQLMXConnectionPoolDataSource,Properties)
 * Methods Added:setLastUsedTime(long), getLastUsedTime()
 * 
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import javax.sql.*;
import java.util.Properties;
import java.util.LinkedList;
import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.lang.ref.*;
import java.util.Locale;
import java.lang.ref.*;
import java.util.*;


public class SQLMXPooledConnection extends WeakConnection implements javax.sql.PooledConnection 
{

	public void addConnectionEventListener(ConnectionEventListener listener) 
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_addConnectionEventListener].methodEntry(JdbcDebug.debugLevelPooling);
		if (JdbcDebugCfg.traceActive) debug[methodId_addConnectionEventListener].methodParameters(
										  "listener=" + JdbcDebug.debugObjectStr(listener));
		try
		{
			if (!isClosed_ && (connection_ != null))
			{
				if (JdbcDebugCfg.traceActive) debug[methodId_addConnectionEventListener].traceOut(
												  JdbcDebug.debugLevelPooling,"Adding listener");
				listenerList_.add(listener);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_addConnectionEventListener].methodExit();
		}
	}

	public void close() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_close].methodEntry(JdbcDebug.debugLevelPooling);
		try
		{
			if (!isClosed_) connection_.close(true, true);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_close].methodExit();
		}
	}

	public Connection getConnection() throws SQLException 
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getConnection].methodEntry(JdbcDebug.debugLevelPooling);
		try
		{
			if (isClosed_ || connection_ == null)
				throw Messages.createSQLException(locale_, "invalid_connection", null);
			if (LogicalConnectionInUse_)
				connection_.close(true, false);
			LogicalConnectionInUse_ = true;

			if (connection_.connectInitialized_)
				connection_.reuse();
			else {
				connection_.connectInit();
				connection_.isClosed_=false; 
			}

			return connection_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getConnection].methodExit();
		}
	}

	public void removeConnectionEventListener(ConnectionEventListener listener) 
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_removeConnectionEventListener].methodEntry(JdbcDebug.debugLevelPooling);
		if (JdbcDebugCfg.traceActive) debug[methodId_removeConnectionEventListener].methodParameters(
										  "listener=" + JdbcDebug.debugObjectStr(listener));
		try
		{
			if (!isClosed_ && (connection_ != null))
			{
				if (JdbcDebugCfg.traceActive) debug[methodId_removeConnectionEventListener].traceOut(
												  JdbcDebug.debugLevelPooling,"Removing listener");
				listenerList_.remove(listener);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_removeConnectionEventListener].methodExit();
		}
	}

	// Called by SQLMXConnection when the connection is closed by the application
	void logicalClose(boolean sendEvents)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_logicalClose].methodEntry(JdbcDebug.debugLevelPooling);
		if (JdbcDebugCfg.traceActive) debug[methodId_logicalClose].methodParameters(
										  "sendEvents=" + sendEvents);
		try
		{
			int i;
			int totalListener;
			ConnectionEventListener listener;
	
			LogicalConnectionInUse_ = false;
			
			if (sendEvents)
			{
				totalListener = listenerList_.size();
				ConnectionEvent event = new ConnectionEvent(this);
				for (i = 0; i < totalListener ; i++)
				{
					listener = (ConnectionEventListener)listenerList_.get(i);
					listener.connectionClosed(event);
				}
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_logicalClose].methodExit();
		}
	}

	void sendConnectionErrorEvent(SQLException ex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_sendConnectionErrorEvent].methodEntry(JdbcDebug.debugLevelPooling);
		if (JdbcDebugCfg.traceActive) debug[methodId_sendConnectionErrorEvent].methodParameters(
										  "ex=" + JdbcDebug.debugObjectStr(ex));
		try
		{
			int i;
			int totalListener;
			ConnectionEventListener listener;
	
			LogicalConnectionInUse_ = false;
			totalListener = listenerList_.size();
			ConnectionEvent event = new ConnectionEvent(this, ex);
			for (i = 0; i < totalListener ; i++)
			{
				listener = (ConnectionEventListener)listenerList_.get(i);
				listener.connectionErrorOccurred(event);
			}
			close();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_sendConnectionErrorEvent].methodExit();
		}
	}

	// Constructor
	SQLMXPooledConnection(SQLMXConnectionPoolDataSource pds, T2Properties info) throws
		SQLException
	{
		super();
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXPooledConnection].methodEntry(JdbcDebug.debugLevelPooling);
		if (JdbcDebugCfg.traceActive) debug[methodId_SQLMXPooledConnection].methodParameters(
										  "pds=" + JdbcDebug.debugObjectStr(pds) +
										  ", info=" + JdbcDebug.debugObjectStr(info));
		try
		{
			String lang = null;
			pds_ = pds;
//			info_ = info;
			t2props_ = info;
			
			if (info != null)
				lang = info.getLanguage();
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
			listenerList_ = new LinkedList<ConnectionEventListener>();
			connection_ = new SQLMXConnection(this, info);
			refToDialogueId_.put(connection_.pRef_, new Long(connection_.getDialogueId()));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXPooledConnection].methodExit();
		}
	} 
	public void setLastUsedTime(long lastUsedTime) {
		this.lastUsedTime = lastUsedTime;
	}
	public long getLastUsedTime() {
		return lastUsedTime;
	} 

	private LinkedList<ConnectionEventListener>			listenerList_;
	private boolean							isClosed_ =	false;
	private SQLMXConnectionPoolDataSource	pds_;
	private Properties						info_;
	private T2Properties					t2props_;
	private SQLMXConnection					connection_;
	private Locale							locale_;
	private boolean							LogicalConnectionInUse_ = false;
	private long                            lastUsedTime;
	private static int methodId_addConnectionEventListener		= 0;
	private static int methodId_close							= 1;
	private static int methodId_getConnection					= 2;
	private static int methodId_removeConnectionEventListener	= 3;
	private static int methodId_logicalClose					= 4;
	private static int methodId_sendConnectionErrorEvent		= 5;
	private static int methodId_SQLMXPooledConnection			= 6;
	private static int totalMethodIds							= 7;
	private static JdbcDebug[] debug;

	static
	{
		String className = "SQLMXPooledConnection";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_addConnectionEventListener] = new JdbcDebug(className,"addConnectionEventListener");
			debug[methodId_close] = new JdbcDebug(className,"close");
			debug[methodId_getConnection] = new JdbcDebug(className,"getConnection");
			debug[methodId_removeConnectionEventListener] = new JdbcDebug(className,"removeConnectionEventListener");
			debug[methodId_logicalClose] = new JdbcDebug(className,"logicalClose");
			debug[methodId_sendConnectionErrorEvent] = new JdbcDebug(className,"sendConnectionErrorEvent");
			debug[methodId_SQLMXPooledConnection] = new JdbcDebug(className,"SQLMXPooledConnection");
		}
	}

        public void addStatementEventListener(StatementEventListener listener) {
                // TODO Auto-generated method stub

        }

	public void removeStatementEventListener(StatementEventListener listener) {
		// TODO Auto-generated method stub
		
	}
}
