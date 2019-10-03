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

// -*-java-*-
// Filename :	SQLMXResultSetMetaData.java
//

package org.apache.trafodion.jdbc.t2;

import java.sql.*;

public class SQLMXResultSetMetaData implements java.sql.ResultSetMetaData
{

	public String getCatalogName(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getCatalogName].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].catalogName_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getCatalogName].methodExit();
		}
	}
	
	public String getColumnClassName(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getColumnClassName].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].getColumnClassName();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getColumnClassName].methodExit();
		}
	}
	
	public int getColumnCount() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getColumnCount].methodEntry();
		try
		{
			return outputDesc_.length;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getColumnCount].methodExit();
		}
	}
 	
	public int getColumnDisplaySize(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getColumnDisplaySize].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].displaySize_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getColumnDisplaySize].methodExit();
		}
	}
 	
	public String getColumnLabel(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getColumnLabel].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			
			// If the column heading/label is not set, return the column name.
			if (outputDesc_[column-1].columnLabel_.equals("")) 
				return outputDesc_[column-1].name_;
			else
				return outputDesc_[column-1].columnLabel_; 
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getColumnLabel].methodExit();
		}
	}
	
	public String getColumnName(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getColumnName].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].name_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getColumnName].methodExit();
		}
	}
	
	public int getColumnType(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getColumnType].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].dataType_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getColumnType].methodExit();
		}
	}
 	
	public String getColumnTypeName(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getColumnTypeName].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].getColumnTypeName(connection_.locale_);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getColumnTypeName].methodExit();
		}
	}
	
	public int getPrecision(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getPrecision].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].precision_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getPrecision].methodExit();
		}
	}
 	
	public int getScale(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getScale].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].scale_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getScale].methodExit();
		}
	}
 	
	public String getSchemaName(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getSchemaName].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].schemaName_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getSchemaName].methodExit();
		}
	}
	
	public String getTableName(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getTableName].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].tableName_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getTableName].methodExit();
		}
	}
	
	public boolean isAutoIncrement(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isAutoIncrement].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].isAutoIncrement_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isAutoIncrement].methodExit();
		}
	}
 	
	public boolean isCaseSensitive(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isCaseSensitive].methodEntry();
		try
		{
			if (column > outputDesc_.length) 
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].isCaseSensitive_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isCaseSensitive].methodExit();
		}
	}
 	
	public boolean isCurrency(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isCurrency].methodEntry();
		try
		{
			if (column > outputDesc_.length) 
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].isCurrency_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isCurrency].methodExit();
		}
	}
 	
	public boolean isDefinitelyWritable(int column) throws SQLException 
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isDefinitelyWritable].methodEntry();
		try
		{
			return true;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isDefinitelyWritable].methodExit();
		}
	}
 	
	public int isNullable(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isNullable].methodEntry();
		try
		{
			if (column > outputDesc_.length) 
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].isNullable_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isNullable].methodExit();
		}
	}
 	
	public boolean isReadOnly(int column)  throws SQLException 
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isReadOnly].methodEntry();
		try
		{
			return false;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isReadOnly].methodExit();
		}
	}
 	
	public boolean isSearchable(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isSearchable].methodEntry();
		try
		{
			if (column > outputDesc_.length) 
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].isSearchable_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isSearchable].methodExit();
		}
	}
 	
	public boolean isSigned(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isSigned].methodEntry();
		try
		{
			if (column > outputDesc_.length) 
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].isSigned_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isSigned].methodExit();
		}
	}
 	
	public boolean isWritable(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isWritable].methodEntry();
		try
		{
			return true;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isWritable].methodExit();
		}
	}

	// Constructors
	SQLMXResultSetMetaData(SQLMXStatement stmt, SQLMXDesc[] outputDesc)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXResultSetMetaData_LL_stmt].methodEntry();
		try
		{
			connection_ = stmt.connection_;
			outputDesc_ = outputDesc;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXResultSetMetaData_LL_stmt].methodExit();
		}
	}
  
	SQLMXResultSetMetaData(SQLMXResultSet resultSet, SQLMXDesc[] outputDesc)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXResultSetMetaData_LL_rs].methodEntry();
		try
		{
			resultSet_ = resultSet;
			connection_ = resultSet_.connection_;
			outputDesc_ = outputDesc;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXResultSetMetaData_LL_rs].methodExit();
		}
	}

	// Fields
	SQLMXResultSet	resultSet_;
	SQLMXConnection	connection_;
	SQLMXDesc[]		outputDesc_;
	
	private static int methodId_getCatalogName					=  0;
	private static int methodId_getColumnClassName					=  1;
	private static int methodId_getColumnCount					=  2;
	private static int methodId_getColumnDisplaySize				=  3;
	private static int methodId_getColumnLabel					=  4;
	private static int methodId_getColumnName					=  5;
	private static int methodId_getColumnType					=  6;
	private static int methodId_getColumnTypeName					=  7;
	private static int methodId_getPrecision					=  8;
	private static int methodId_getScale						=  9;
	private static int methodId_getSchemaName					= 10;
	private static int methodId_getTableName					= 11;
	private static int methodId_isAutoIncrement					= 12;
	private static int methodId_isCaseSensitive					= 13;
	private static int methodId_isCurrency						= 14;
	private static int methodId_isDefinitelyWritable				= 15;
	private static int methodId_isNullable						= 16;
	private static int methodId_isReadOnly						= 17;
	private static int methodId_isSearchable					= 18;
	private static int methodId_isSigned						= 19;
	private static int methodId_isWritable						= 20;
	private static int methodId_SQLMXResultSetMetaData_LL_stmt			= 21;
	private static int methodId_SQLMXResultSetMetaData_LL_rs			= 22;
	private static int totalMethodIds						= 23;
	private static JdbcDebug[] debug;
	
	static
	{
		String className = "SQLMXResultSetMetaData";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_getCatalogName] = new JdbcDebug(className,"getCatalogName"); 
			debug[methodId_getColumnClassName] = new JdbcDebug(className,"getColumnClassName"); 
			debug[methodId_getColumnCount] = new JdbcDebug(className,"getColumnCount"); 
			debug[methodId_getColumnDisplaySize] = new JdbcDebug(className,"getColumnDisplaySize"); 
			debug[methodId_getColumnLabel] = new JdbcDebug(className,"getColumnLabel"); 
			debug[methodId_getColumnName] = new JdbcDebug(className,"getColumnName"); 
			debug[methodId_getColumnType] = new JdbcDebug(className,"getColumnType"); 
			debug[methodId_getColumnTypeName] = new JdbcDebug(className,"getColumnTypeName"); 
			debug[methodId_getPrecision] = new JdbcDebug(className,"getPrecision"); 
			debug[methodId_getScale] = new JdbcDebug(className,"getScale"); 
			debug[methodId_getSchemaName] = new JdbcDebug(className,"getSchemaName"); 
			debug[methodId_getTableName] = new JdbcDebug(className,"getTableName"); 
			debug[methodId_isAutoIncrement] = new JdbcDebug(className,"isAutoIncrement"); 
			debug[methodId_isCaseSensitive] = new JdbcDebug(className,"isCaseSensitive"); 
			debug[methodId_isCurrency] = new JdbcDebug(className,"isCurrency"); 
			debug[methodId_isDefinitelyWritable] = new JdbcDebug(className,"isDefinitelyWritable"); 
			debug[methodId_isNullable] = new JdbcDebug(className,"isNullable"); 
			debug[methodId_isReadOnly] = new JdbcDebug(className,"isReadOnly"); 
			debug[methodId_isSearchable] = new JdbcDebug(className,"isSearchable"); 
			debug[methodId_isSigned] = new JdbcDebug(className,"isSigned"); 
			debug[methodId_isWritable] = new JdbcDebug(className,"isWritable"); 
			debug[methodId_SQLMXResultSetMetaData_LL_stmt] = new JdbcDebug(className,"SQLMXResultSetMetaData[LL_stmt]"); 
			debug[methodId_SQLMXResultSetMetaData_LL_rs] = new JdbcDebug(className,"SQLMXResultSetMetaData[LL_rs]"); 
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
    public int getSqlCharset(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].sqlCharset_;
        }
        finally
        {
        }
    }
    public int getOdbcCharset(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].odbcCharset_;
        }
        finally
        {
        }
    }
    public int getSqlDataType(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].sqlDataType_;
        }
        finally
        {
        }
    }
    public int getDataType(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].dataType_;
        }
        finally
        {
        }
    }
    public short getSqlPrecision(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].sqlPrecision_;       
        }
        finally
        {
        }
    }
    public int getOdbcPrecision(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].odbcPrecision_;
        }
        finally
        {
        }
    }
    public short getSqlDatetimeCode(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].sqlDatetimeCode_;        
        }
        finally
        {
        }
    }
    public int getSqlOctetLength(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].sqlOctetLength_;     
        }
        finally
        {
        }
    }
    public int getMaxLen(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].maxLen_;     
        }
        finally
        {
        }
    }
    public int getIsNullable(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].isNullable_;     
        }
        finally
        {
        }
    }
    public boolean getIsSigned(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].isSigned_;       
        }
        finally
        {
        }
    }
    public boolean getIsCurrency(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].isCurrency_;     
        }
        finally
        {
        }
    }
    public boolean getIsCaseSensitive(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].isCaseSensitive_;        
        }
        finally
        {
        }
    }
    public int getFsDataType(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].fsDataType_;     
        }
        finally
        {
        }
    }
    public int getIntLeadPrec(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].intLeadPrec_;        
        }
        finally
        {
        }
    }
    public int getMode(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].paramMode_;      
        }
        finally
        {
        }
    }
    public int getIndex(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].paramIndex_;     
        }
        finally
        {
        }
    }
    public int getPos(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].paramPos_;       
        }
        finally
        {
        }
    }
    public int getDisplaySize(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].displaySize_;        
        }
        finally
        {
        }
    }
    public String getName(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].name_;
        }
        finally
        {
        }
    }
    public String getLabel(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].columnLabel_;
        }
        finally
        {
        }
    }
    public String getClassName(int column) throws SQLException {
        try
        {
            if (column > outputDesc_.length) 
                throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
            return outputDesc_[column - 1].columnClassName_;
        }
        finally
        {
        }
    }
//---------------------------------------------------------
}
