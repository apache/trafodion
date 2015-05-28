// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2004-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

/* -*-java-*-
 * Filename    : SQLMXParameterMetaData.java
 */

package org.trafodion.jdbc.t2;

import java.sql.*;

public class SQLMXParameterMetaData implements java.sql.ParameterMetaData
{
	public String getParameterClassName(int param) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getParameterClassName].methodEntry();
		try
		{
			if (param > inputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return inputDesc_[param-1].getColumnClassName();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getParameterClassName].methodExit();
		}
	}
	
	public int getParameterCount() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getParameterCount].methodEntry();
		try
		{
			return inputDesc_.length;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getParameterCount].methodExit();
		}
	}
 	
	public int getParameterMode(int param) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getParameterMode].methodEntry();
		try
		{
			if (param > inputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return inputDesc_[param-1].paramMode_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getParameterMode].methodExit();
		}
	}
 	
	public int getParameterType(int param) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getParameterType].methodEntry();
		try
		{
			if (param > inputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return inputDesc_[param-1].dataType_; 
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getParameterType].methodExit();
		}
	}
 	
	public String getParameterTypeName(int param) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getParameterTypeName].methodEntry();
		try
		{
			if (param > inputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return inputDesc_[param-1].getColumnTypeName(connection_.locale_);	
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getParameterTypeName].methodExit();
		}
	}
	
	public int getPrecision(int param) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getPrecision].methodEntry();
		try
		{
			if (param > inputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return inputDesc_[param-1].precision_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getPrecision].methodExit();
		}
	}
 	
	public int getScale(int param) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getScale].methodEntry();
		try
		{
			if (param > inputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return inputDesc_[param-1].scale_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getScale].methodExit();
		}
	}
 	
	/* cpqGetCharacterName (extended) method added to allow SQLJ to           */
	/* pull character data types from SQL/MX encoding info in the COLS table. */  
	public String cpqGetCharacterSet(int param) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_cpqGetCharacterSet].methodEntry();
		try
		{
			if ((param > inputDesc_.length) || (param <= 0))
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return inputDesc_[param-1].getCharacterSetName();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_cpqGetCharacterSet].methodExit();
		}
	}

	public int isNullable(int param) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isNullable].methodEntry();
		try
		{
			if (param > inputDesc_.length) 
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return inputDesc_[param-1].isNullable_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isNullable].methodExit();
		}
	}
 	
	public boolean isSigned(int param) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isSigned].methodEntry();
		try
		{
			if (param > inputDesc_.length) 
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return inputDesc_[param-1].isSigned_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isSigned].methodExit();
		}
	}
 	
	// Constructors
	SQLMXParameterMetaData(SQLMXPreparedStatement stmt, SQLMXDesc[] inputDesc)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXParameterMetaData].methodEntry();
		try
		{
			connection_ = stmt.connection_;
			inputDesc_ = inputDesc;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXParameterMetaData].methodExit();
		}
	}
  
	// Fields
	SQLMXConnection	connection_;
	SQLMXDesc[]		inputDesc_;
	private static int methodId_getParameterClassName	=  0;
	private static int methodId_getParameterCount		=  1;
	private static int methodId_getParameterMode		=  2;
	private static int methodId_getParameterType		=  3;
	private static int methodId_getParameterTypeName	=  4;
	private static int methodId_getPrecision			=  5;
	private static int methodId_getScale				=  6;
	private static int methodId_cpqGetCharacterSet		=  7;
	private static int methodId_isNullable				=  8;
	private static int methodId_isSigned				=  9;
	private static int methodId_SQLMXParameterMetaData	= 10;
	private static int totalMethodIds					= 11;
	private static JdbcDebug[] debug;

	static
	{
		String className = "SQLMXParameterMetaData";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_getParameterClassName] = new JdbcDebug(className,"getParameterClassName");
			debug[methodId_getParameterCount] = new JdbcDebug(className,"getParameterCount");
			debug[methodId_getParameterMode] = new JdbcDebug(className,"getParameterMode");
			debug[methodId_getParameterType] = new JdbcDebug(className,"getParameterType");
			debug[methodId_getParameterTypeName] = new JdbcDebug(className,"getParameterTypeName");
			debug[methodId_getPrecision] = new JdbcDebug(className,"getPrecision");
			debug[methodId_getScale] = new JdbcDebug(className,"getScale");
			debug[methodId_cpqGetCharacterSet] = new JdbcDebug(className,"cpqGetCharacterSet");
			debug[methodId_isNullable] = new JdbcDebug(className,"isNullable");
			debug[methodId_isSigned] = new JdbcDebug(className,"isSigned");
			debug[methodId_SQLMXParameterMetaData] = new JdbcDebug(className,"SQLMXParameterMetaData");
		}
	}

	public Object unwrap(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public boolean isWrapperFor(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}
}
