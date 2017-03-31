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
 * Filename    : SQLMXHandle.java
 * Description : 
 */
package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.util.Locale;

 
public abstract class SQLMXHandle
{
	SQLWarning sqlWarning_;
 	
	public void clearWarnings()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_clearWarnings].methodEntry();
		try
		{
			sqlWarning_ = null;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_clearWarnings].methodExit();
		}
	}
 		
	public SQLWarning getWarnings() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getWarnings].methodEntry();
		try
		{
			return sqlWarning_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getWarnings].methodExit();
		}
	}
 		
	void setSQLWarning(Locale msgLocale, String messageId, Object[] messageArguments)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setSQLWarning_LLL].methodEntry();
		try
		{
			SQLWarning sqlWarningLeaf =	Messages.createSQLWarning(msgLocale, messageId, messageArguments);
			if (sqlWarning_ == null)
				sqlWarning_ = sqlWarningLeaf;
			else
				sqlWarning_.setNextWarning(sqlWarningLeaf);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setSQLWarning_LLL].methodExit();
		}
	}
 	
	// Method used by JNI layer to set the warning
	void setSqlWarning(SQLWarning sqlWarning)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setSqlWarning_L].methodEntry();
		try
		{
			if (sqlWarning_ == null)
				sqlWarning_ = sqlWarning;
			else
				sqlWarning_.setNextWarning(sqlWarning);	
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setSqlWarning_L].methodExit();
		}
	}

	private static int methodId_clearWarnings		= 0;
	private static int methodId_getWarnings			= 1;
	private static int methodId_setSQLWarning_LLL	= 2;
	private static int methodId_setSqlWarning_L		= 3;
	private static int totalMethodIds				= 4;
	private static JdbcDebug[] debug;

	static
	{
		String className = "SQLMXHandle";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_clearWarnings] = new JdbcDebug(className,"clearWarnings"); 
			debug[methodId_getWarnings] = new JdbcDebug(className,"getWarnings"); 
			debug[methodId_setSQLWarning_LLL] = new JdbcDebug(className,"setSQLWarning[LLL]"); 
			debug[methodId_setSqlWarning_L] = new JdbcDebug(className,"setSqlWarning[L]"); 
		}
	}
}
