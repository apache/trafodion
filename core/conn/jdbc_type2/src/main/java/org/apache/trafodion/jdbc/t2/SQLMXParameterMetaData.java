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
 * Filename    : SQLMXParameterMetaData.java
 */

package org.apache.trafodion.jdbc.t2;

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
	private static int methodId_getPrecision		=  5;
	private static int methodId_getScale			=  6;
	private static int methodId_isNullable			=  7;
	private static int methodId_isSigned			=  8;
	private static int methodId_SQLMXParameterMetaData	= 9;
	private static int totalMethodIds			= 10;
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
    public int getSqlCharset(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].sqlCharset_;
        }
        finally
        {
        }
    }
    public int getOdbcCharset(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].odbcCharset_;
        }
        finally
        {
        }
    }
    public int getSqlDataType(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].sqlDataType_;
        }
        finally
        {
        }
    }
    public int getDataType(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].dataType_;
        }
        finally
        {
        }
    }
    public short getSqlPrecision(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].sqlPrecision_; 
        }
        finally
        {
        }
    }
    public int getOdbcPrecision(int param) throws SQLException{
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].odbcPrecision_;
        }
        finally
        {
        }
    }
    public short getSqlDatetimeCode(int param) throws SQLException{
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].sqlDatetimeCode_;
        }
        finally
        {
        }
    }
    public int getSqlOctetLength(int param) throws SQLException{
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].sqlOctetLength_;
        }
        finally
        {
        }
    }
    public int getMaxLen(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].maxLen_; 
        }
        finally
        {
        }
    }
    public boolean getIsCurrency(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].isCurrency_;
        }
        finally
        {
        }
    }
    public boolean getIsCaseSensitive(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].isCaseSensitive_;
        }
        finally
        {
        }
    }
    public int getFsDataType(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].fsDataType_;
        }
        finally
        {
        }
    }
    public int getIntLeadPrec(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].intLeadPrec_;
        }
        finally
        {
        }
    }
    public int getMode(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].paramMode_; 
        }
        finally
        {
        }
    }
    public int getIndex(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].paramIndex_;
        }
        finally
        {
        }
    }
    public int getPos(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].paramPos_;
        }
        finally
        {
        }
    }
    public int getDisplaySize(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].displaySize_; 
        }
        finally
        {
        }
    }
    public String getCatalogName(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].catalogName_;
        }
        finally
        {
        }
    }
    public String getSchemaName(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].schemaName_;
        }
        finally
        {
        }
    }
    public String getTableName(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].tableName_;
        }
        finally
        {
        }
    }
    public String getName(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].name_;
        }
        finally
        {
        }
    }
    public String getLabel(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].columnLabel_;
        }
        finally
        {
        }
    }
    public String getClassName(int param) throws SQLException {
        try
        {
            if (param > inputDesc_.length)
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return inputDesc_[param - 1].columnClassName_;
        }
        finally
        {
        }
    }
//------------------------------------------------------
}
