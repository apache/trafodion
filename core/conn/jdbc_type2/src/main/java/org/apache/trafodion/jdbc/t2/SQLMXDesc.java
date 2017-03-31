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
 * Filename    : SQLMXDesc.java
 * Description :
 */

package org.apache.trafodion.jdbc.t2;

import java.util.Locale;
import java.sql.*;

class SQLMXDesc 
{
	
	String getColumnClassName() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getColumnClassName].methodEntry();
		try
		{
			switch (dataType_)
			{
				case Types.SMALLINT:
					return "java.lang.Integer";
				case Types.INTEGER:
					return "java.lang.Integer";
				case Types.BIGINT:
					return "java.lang.Long";
				case Types.REAL:
					return "java.lang.Float";
				case Types.FLOAT:
				case Types.DOUBLE:
					return "java.lang.Double";
				case Types.NUMERIC:
				case Types.DECIMAL:
					return "java.math.BigDecimal";
				case Types.CHAR:
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
					return "java.lang.String";
				case Types.DATE:
					return "java.sql.Date";
				case Types.TIME:
					return "java.sql.Time";
				case Types.TIMESTAMP:
					return "java.sql.Timestamp";
				case Types.OTHER:
					return "java.sql.String";
				case Types.CLOB:
					return "java.sql.Clob";
				case Types.BLOB:
					return "java.sql.Blob";
				case Types.BIT:
				case Types.TINYINT:
				default:
					return null;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getColumnClassName].methodExit();
		}
	}
	
	String getColumnTypeName(Locale locale) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getColumnTypeName].methodEntry();
		try
		{
			switch (dataType_)
			{
				case Types.SMALLINT:
					return "SMALLINT";
				case Types.INTEGER:
					return "INTEGER";
				case Types.BIGINT:
					return "BIGINT";
				case Types.REAL:
					return "REAL";
				case Types.FLOAT:
					return "FLOAT";
				case Types.DOUBLE:
					return "DOUBLE PRECISION";
				case Types.NUMERIC:
					return "NUMERIC";
				case Types.DECIMAL:
					return "DECIMAL";
				case Types.CHAR:
					return "CHAR";
				case Types.VARCHAR:
					return "VARCHAR";
				case Types.LONGVARCHAR:
					return "LONG VARCHAR";
				case Types.DATE:
					return "DATE";
				case Types.TIME:
					return "TIME";
				case Types.TIMESTAMP:
					return "TIMESTAMP";
				case Types.BLOB:
					return "BLOB";
				case Types.CLOB:
					return "CLOB";
				case Types.OTHER:
					if (sqlDataType_ == SQLTYPECODE_INTERVAL)
						return "INTERVAL";
					else
						return "UNKNOWN";
				case Types.BIT:
				case Types.TINYINT:
				default:
					return null;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getColumnTypeName].methodExit();
		}
	}
	
	/* Returns encoding type for character data types from SQL/MX     *
	 * COLS table. Valid encodings are UCS2, ISO88591, KANJI, KSC5601 * 
	 * or UNKNOWN.                                                    */
	String getCharacterSetName() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getCharacterSetName].methodEntry();
		try
		{
			switch (dataType_)
			{
				case Types.CHAR:
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
				case Types.BLOB:
				case Types.CLOB:
				switch (sqlCharset_)
				{
					case SQLCHARSETCODE_ISO88591:
						return SQLCHARSETSTRING_ISO88591;
					case SQLCHARSETCODE_KANJI:
						return SQLCHARSETSTRING_KANJI;
					case SQLCHARSETCODE_KSC5601:
						return SQLCHARSETSTRING_KSC5601;
					case SQLCHARSETCODE_UCS2:
						return SQLCHARSETSTRING_UNICODE;
					case SQLCHARSETCODE_UNKNOWN:
						return SQLCHARSETSTRING_UNKNOWN;
					default:
						return null;
				}
				default:
					return null;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getCharacterSetName].methodExit();
		}
	}

	void checkValidNumericConversion(Locale locale) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_checkValidNumericConversion].methodEntry();
		try
		{
			switch (dataType_)
			{
				case Types.TINYINT:
				case Types.SMALLINT:
				case Types.INTEGER:
				case Types.BIGINT:
				case Types.REAL:
				case Types.FLOAT:
				case Types.DOUBLE:
				case Types.NUMERIC:
				case Types.DECIMAL:
				case Types.BIT:
				case Types.CHAR:
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
				case Types.CLOB:
				case Types.BLOB:
					break;
				default:
					throw Messages.createSQLException(locale, "restricted_data_type", null);
			}
			return;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_checkValidNumericConversion].methodExit();
		}
	}
    
	// Constructors
	SQLMXDesc(int dataType, short datetimeCode, int maxLen, short precision,
		short scale, boolean nullInfo, String colName, boolean signType, int odbcDataType,
		short odbcPrecision, int sqlCharset, int odbcCharset,
		String catalogName, String schemaName, String tableName, int fsDataType,
		int intLeadPrec, int paramMode, String colLabel)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXDesc].methodEntry();
		try
		{
			catalogName_ = catalogName;
			schemaName_ = schemaName;
			tableName_ = tableName;
			if (colLabel != null)
				columnLabel_ = colLabel;
			else
				columnLabel_ = "";
			name_ = colName;
			if (nullInfo)
				isNullable_ = ResultSetMetaData.columnNullable;
			else
				isNullable_ = ResultSetMetaData.columnNoNulls;
			sqlDataType_ = dataType;
			dataType_ = odbcDataType;
            tmpDataType_ = dataType_;           
			sqlDatetimeCode_ = datetimeCode;
			sqlCharset_ = sqlCharset;
			odbcCharset_ = odbcCharset;
			isSigned_ = signType;
			sqlOctetLength_ = maxLen;
			scale_ = scale;
            odbcPrecision_ = odbcPrecision;     
            maxLen_ = maxLen;                   

			switch (dataType_)
			{
				case Types.NUMERIC:
				case Types.DECIMAL:
					precision_ = odbcPrecision;
					displaySize_ = precision_ + 2;   // 1 for dot and 1 for sign
					//if (scale != 0)					// ODBC2.0
					//	isCurrency_ = true;
					break;
				case Types.SMALLINT:
					precision_ = odbcPrecision;
					if (isSigned_)
						displaySize_ = 6;
					else
					{
						displaySize_ = 5;
					}
					break;
				case Types.INTEGER:
					precision_ =  odbcPrecision;
					if (isSigned_)
						displaySize_ = 11;
					else
					{
						displaySize_ = 10;
					}
					break;
				case Types.TINYINT:
					precision_ =  odbcPrecision;
					if (isSigned_)
						displaySize_ = 4;
					else
						displaySize_ = 3;
					break;
				case Types.BIGINT:
					precision_ =  odbcPrecision;
					if (isSigned_)
						displaySize_ = 20;
					else
						displaySize_ = 19;
					break;
				case Types.REAL:
					precision_ =  odbcPrecision;
					displaySize_ = 14;
					break;
				case Types.DOUBLE:
				case Types.FLOAT:
					precision_ =  odbcPrecision;
					displaySize_ = 24;
					break;
				case Types.DATE:
					sqlOctetLength_ = maxLen+3;
					displaySize_ = 10;
					precision_ = 10;				//  ODBC2.0
					break;
				case Types.TIME:
					sqlOctetLength_ = maxLen+3;
					displaySize_ = 8;
					precision_ = 8;					// ODBC2.0
					break;
				case Types.TIMESTAMP:
					sqlOctetLength_ = maxLen+3;
					precision_ = precision; // Should come for Server - Must be changed
					if (precision_ == 0)
						displaySize_ = 19;
					else
						displaySize_ = 20 + precision_;
					precision_ = 23;				// ODBC2.0
					if (sqlDatetimeCode_ > 3)	// if it is more than 3, it is one of SQL/MP Datetime columns
						// like YEAR, YEAR TO MONTH, YEAR TO DAY ...see dfs2rec.h
						dataType_ = Types.OTHER;	
					break;
				case Types.CHAR:
					sqlOctetLength_ = maxLen+1;
					displaySize_ = maxLen;
					precision_ = maxLen;			// ODBC2.0
					break;
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
					if (sqlDataType_ == SQLTYPECODE_VARCHAR)
						sqlOctetLength_ = maxLen+1;
					else
						sqlOctetLength_ = maxLen+3;
					displaySize_ = maxLen;
					precision_ = maxLen;			// ODBC2.0
					break;
				case Types.BLOB:
				case Types.CLOB:
					break;
				default:
					if (sqlDataType_ == SQLTYPECODE_INTERVAL)
					{
						dataType_ = Types.OTHER;
						precision_ = odbcPrecision;
						displaySize_ = maxLen;	// Make sure maxLen returns the right display size for interval
						sqlOctetLength_ = maxLen+3;
					}
					break;
			}
			if (sqlDataType_ == SQLTYPECODE_CHAR || sqlDataType_ == SQLTYPECODE_VARCHAR 
				|| sqlDataType_ == SQLTYPECODE_VARCHAR_LONG || sqlDataType_ == SQLTYPECODE_VARCHAR_WITH_LENGTH)
				isCaseSensitive_ = true;
			isSearchable_ = true;
			fsDataType_ = fsDataType;
			intLeadPrec_ = intLeadPrec_;
			paramMode_ = paramMode;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXDesc].methodExit();
		}
	}
	
	// The enum SQLTYPE_CODE from sqlcli.h. HP extenstion are negative values.

	public static final int SQLTYPECODE_CHAR                   = 1;
	public static final int SQLTYPECODE_NUMERIC                = 2;
	public static final int SQLTYPECODE_NUMERIC_UNSIGNED       = -201;
	public static final int SQLTYPECODE_DECIMAL                = 3;
	public static final int SQLTYPECODE_DECIMAL_UNSIGNED       = -301;
	public static final int SQLTYPECODE_DECIMAL_LARGE          = -302;
	public static final int SQLTYPECODE_DECIMAL_LARGE_UNSIGNED = -303;
	public static final int SQLTYPECODE_INTEGER                = 4;
	public static final int SQLTYPECODE_INTEGER_UNSIGNED       = -401;
	public static final int SQLTYPECODE_LARGEINT               = -402;
	public static final int SQLTYPECODE_SMALLINT               = 5;
	public static final int SQLTYPECODE_SMALLINT_UNSIGNED      = -502;
	public static final int SQLTYPECODE_BPINT_UNSIGNED         = -503;
	public static final int SQLTYPECODE_IEEE_FLOAT             = 6;
	public static final int SQLTYPECODE_IEEE_REAL              = 7;
	public static final int SQLTYPECODE_IEEE_DOUBLE            = 8;
	public static final int SQLTYPECODE_TDM_FLOAT              = -411;
	public static final int SQLTYPECODE_TDM_REAL               = -412;
	public static final int SQLTYPECODE_TDM_DOUBLE             = -413;
	public static final int SQLTYPECODE_DATETIME               = 9;
	public static final int SQLTYPECODE_INTERVAL               = 10;
	public static final int SQLTYPECODE_VARCHAR	               = 12;
	public static final int SQLTYPECODE_VARCHAR_WITH_LENGTH    = -601;
	public static final int SQLTYPECODE_VARCHAR_LONG           = -1;
	public static final int SQLTYPECODE_BIT                    = 14;  // not supported
	public static final int SQLTYPECODE_BITVAR                 = 15;  // not supported 
    
	// type codes from SQL/MP include file sql.h, for TYPE_FS descriptor fields 
	//(with additional SQL/MX datatypes) from sqlcli.h
    
	public static final int SQLDT_16BIT_SIGNED				= 130;
	public static final int SQLDT_16BIT_UNSIGNED			= 131;
	public static final int SQLDT_32BIT_SIGNED				= 132;
	public static final int SQLDT_32BIT_UNSIGNED			= 133;
	public static final int SQLDT_64BIT_SIGNED				= 134;
	// Big Num Changes
	public static final int SQLDT_NUM_BIG_S                 = 156;
	public static final int SQLDT_NUM_BIG_U                 = 155;
	// Big Num Changes

	// enum SQLCHARSET_CODE from sqlcli.h

	public static final int SQLCHARSETCODE_UNKNOWN          = 0;
	public static final int SQLCHARSETCODE_ISO88591         = 1;
	public static final int SQLCHARSETCODE_KANJI            = -1;
	public static final int SQLCHARSETCODE_KSC5601          = -2;
	public static final int SQLCHARSETCODE_SJIS             = 10;
	public static final int SQLCHARSETCODE_UCS2	            = 11;

	public static final String SQLCHARSETSTRING_UNKNOWN     = "UNKNOWN";
	public static final String SQLCHARSETSTRING_ISO88591    = "ISO88591";
	public static final String SQLCHARSETSTRING_KANJI       = "KANJI";
	public static final String SQLCHARSETSTRING_KSC5601     = "KSC5601";
	public static final String SQLCHARSETSTRING_UNICODE	    = "UCS2";


	//fields
	int		sqlCharset_;
	int		odbcCharset_;
	int		sqlDataType_;
	int		dataType_;
	short	sqlPrecision_;
	short	sqlDatetimeCode_;
	int		sqlOctetLength_;
	int		isNullable_;
	String	name_;
	short	scale_;
	int		precision_;
	boolean	isSigned_;
	boolean	isCurrency_;
	boolean	isCaseSensitive_;
	String 	catalogName_;
	String	schemaName_;
	String	tableName_;
	int		fsDataType_;
	int		intLeadPrec_;
	int		paramMode_;
	int		paramIndex_;
	int		paramPos_;
    int     odbcPrecision_;
    int     maxLen_;
    int     tmpDataType_;
	
	String	columnClassName_;
	int		displaySize_;
	String	columnLabel_;

	// fields which are not pouplated now
	boolean	isAutoIncrement_;
	boolean isSearchable_;
	
	boolean isValueSet_; // To denote if setXXX method is called for this parameter
	Object	paramValue_; // Contains the value of output parameter value

	private static int methodId_getColumnClassName			= 0;
	private static int methodId_getColumnTypeName			= 1;
	private static int methodId_getCharacterSetName			= 2;
	private static int methodId_checkValidNumericConversion	= 3;
	private static int methodId_SQLMXDesc					= 4; 
	private static int totalMethodIds						= 5;
	private static JdbcDebug[] debug;
	
	static
	{
		String className = "SQLMXDesc";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_getColumnClassName] = new JdbcDebug(className,"getColumnClassName"); 
			debug[methodId_getColumnTypeName] = new JdbcDebug(className,"getColumnTypeName"); 
			debug[methodId_getCharacterSetName] = new JdbcDebug(className,"getCharacterSetName"); 
			debug[methodId_checkValidNumericConversion] = new JdbcDebug(className,"checkValidNumericConversion"); 
			debug[methodId_SQLMXDesc] = new JdbcDebug(className,"SQLMXDesc"); 
		}
	}

}
