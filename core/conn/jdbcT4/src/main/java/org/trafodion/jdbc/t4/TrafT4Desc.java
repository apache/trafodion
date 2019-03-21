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

package org.trafodion.jdbc.t4;

import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.Types;
import java.util.Locale;

class TrafT4Desc {

	String getColumnClassName() throws SQLException {
		switch (dataType_) {
		case Types.TINYINT:
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
                case Types.BINARY:
                    return "java.sql.Binary";
                case Types.VARBINARY:
                    return "java.sql.Varbinary";
		case Types.BIT:
		default:
			return null;
		}
	} // end getColumnClassName

	// ---------------------------------------------------------------
	String getColumnTypeName(Locale locale) throws SQLException {
		switch (dataType_) {
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
                case Types.BINARY:
                    return "BINARY";
                case Types.VARBINARY:
                    return "VARBINARY";
		case Types.OTHER:
			if (sqlDataType_ == SQLTYPECODE_INTERVAL) {
				return "INTERVAL";
			} else {
				return "UNKNOWN";
			}
		case Types.BIT:
		case Types.TINYINT:
		default:
			return null;
		}
	} // end getColumnTypeName

	// ---------------------------------------------------------------
	void checkValidNumericConversion(Locale locale) throws SQLException {
		switch (dataType_) {
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
                case Types.BINARY:
                case Types.VARBINARY:
			break;
		default:
                        System.out.println("Data type is " + dataType_);
			throw TrafT4Messages.createSQLException(null, locale, "restricted_data_type", null);
		}
		return;
	} // end checkValidNumericConversion

	// ---------------------------------------------------------------
	// Constructors
	TrafT4Desc(int dataType, short datetimeCode, int maxLen, short precision, short scale, boolean nullInfo,
			String colName, boolean signType, int odbcDataType, short odbcPrecision, int sqlCharset, int odbcCharset,
			String catalogName, String schemaName, String tableName, int fsDataType, int intLeadPrec, int paramMode) {
		catalogName_ = catalogName;
		schemaName_ = schemaName;
		tableName_ = tableName;
		name_ = colName;
		if (nullInfo) {
			isNullable_ = ResultSetMetaData.columnNullable;
		} else {
			isNullable_ = ResultSetMetaData.columnNoNulls;

		}

		sqlDataType_ = dataType;
		dataType_ = odbcDataType;
		sqlDatetimeCode_ = datetimeCode;
		sqlCharset_ = sqlCharset;
		odbcCharset_ = odbcCharset;
		isSigned_ = signType;
		sqlOctetLength_ = maxLen;
		scale_ = scale;
		sqlPrecision_ = precision;

		//
		// Convert ODBC type to equivalent JDBC type when necessary.
		//
		// From SqlUcode.h
		//
		// #define SQL_WCHAR (-8)
		// #define SQL_WVARCHAR (-9)
		// #define SQL_WLONGVARCHAR (-10)
		//
		if (odbcDataType == -8) {

			// ODBC's SQL_WCHAR becomes a Types.CHAR
			dataType_ = Types.CHAR;
		} else if (odbcDataType == -9) {

			// ODBC's SQL_WVARCHAR becomes a Types.VARCHAR
			dataType_ = Types.VARCHAR;
		} else if (odbcDataType == -10) {

			// ODBC's SQL_WLONGVARCHAR becomes a Types.LONGVARCHAR
			dataType_ = Types.LONGVARCHAR;

		}
		if (sqlDataType_ == InterfaceResultSet.SQLTYPECODE_DATETIME) // 9
		{
			switch (dataType_) { // ODBC conversion to ODBC2.0
			case 9: // ODBC2 Date

				// check the datetime code and set appropriately
				switch (sqlDatetimeCode_) {
				case SQLDTCODE_YEAR:
				case SQLDTCODE_YEAR_TO_MONTH:
				case SQLDTCODE_MONTH:
				case SQLDTCODE_MONTH_TO_DAY:
				case SQLDTCODE_DAY:
					dataType_ = Types.OTHER;
					precision_ = odbcPrecision;
					displaySize_ = maxLen;
					sqlOctetLength_ = maxLen;
					break;
				default:
					dataType_ = Types.DATE;
					break;
				}
				break;
			case 10: // ODBC2 TIME
				switch (sqlDatetimeCode_) {
				case SQLDTCODE_HOUR:
				case SQLDTCODE_HOUR_TO_MINUTE:
				case SQLDTCODE_MINUTE:
				case SQLDTCODE_MINUTE_TO_SECOND:
				case SQLDTCODE_SECOND:
					dataType_ = Types.OTHER;
					precision_ = odbcPrecision;
					displaySize_ = maxLen;
					sqlOctetLength_ = maxLen;
					break;
				default:
					dataType_ = Types.TIME;
					break;
				}
				break;
			case 11: // ODBC2 TIMESTAMP
				switch (sqlDatetimeCode_) {
				case SQLDTCODE_YEAR_TO_HOUR:
				case SQLDTCODE_YEAR_TO_MINUTE:
				case SQLDTCODE_MONTH_TO_HOUR:
				case SQLDTCODE_MONTH_TO_MINUTE:
				case SQLDTCODE_MONTH_TO_SECOND:
					// case SQLDTCODE_MONTH_TO_FRACTION:
				case SQLDTCODE_DAY_TO_HOUR:
				case SQLDTCODE_DAY_TO_MINUTE:
				case SQLDTCODE_DAY_TO_SECOND:
					// case SQLDTCODE_DAY_TO_FRACTION:
				case SQLDTCODE_HOUR_TO_FRACTION: // note: Database 
					// maps to TIME(6)
					// NCS maps to TIMESTAMP
				case SQLDTCODE_MINUTE_TO_FRACTION:
				case SQLDTCODE_SECOND_TO_FRACTION:
					dataType_ = Types.OTHER;
					precision_ = odbcPrecision;
					displaySize_ = maxLen;
					sqlOctetLength_ = maxLen;
					break;
				default:
					dataType_ = Types.TIMESTAMP;
					break;
				}
				break;
			default:
				dataType_ = Types.TIMESTAMP;
				break;
			}
		}

		switch (dataType_) {
		case Types.NUMERIC:
		case Types.DECIMAL:
			precision_ = odbcPrecision;
			displaySize_ = precision_ + 2; // 1 for dot and 1 for sign
			// if (scale != 0) // ODBC2.0
			// isCurrency_ = true;
			break;
		case Types.SMALLINT:
			precision_ = odbcPrecision;
			if (isSigned_) {
				displaySize_ = 6;
			} else {
				dataType_ = Types.INTEGER;
				displaySize_ = 5;
			}
			break;
		case Types.INTEGER:
			precision_ = odbcPrecision;
			if (isSigned_) {
				displaySize_ = 11;
			} else {
				dataType_ = Types.BIGINT;
				displaySize_ = 10;
			}
			break;
		case Types.TINYINT:
			precision_ = odbcPrecision;
			if (isSigned_) {
				displaySize_ = 4;
			} else {
				displaySize_ = 3;
			}
			break;
		case Types.BIGINT:
			precision_ = odbcPrecision;
			if (isSigned_) {
				displaySize_ = 20;
			} else {
				displaySize_ = 19;
			}
			break;
		case Types.REAL:
			precision_ = odbcPrecision;
			displaySize_ = 15;
			break;
		case Types.DOUBLE:
		case Types.FLOAT:
			precision_ = odbcPrecision;
			displaySize_ = 24;
			break;
		case Types.DATE:
			sqlOctetLength_ = maxLen + 3;
			displaySize_ = 10;
			precision_ = 10; // ODBC2.0
			break;
		case Types.TIME:
			sqlOctetLength_ = maxLen + 3;
			displaySize_ = (precision == 0)?8: precision + 9;
			precision_ = 8; // ODBC2.0
			break;
		case Types.TIMESTAMP:
			sqlOctetLength_ = maxLen + 3;
			precision_ = odbcPrecision;
			displaySize_ = precision_;
			if (sqlDatetimeCode_ > 3) // if it is more than 3, it is one of
			// SQL/MP Datetime columns
			{
				// like YEAR, YEAR TO MONTH, YEAR TO DAY ...see dfs2rec.h
				dataType_ = Types.OTHER;
			}
			break;
		case Types.CHAR:
                case Types.BINARY:
			// sqlOctetLength_ = maxLen+1;
			sqlOctetLength_ = maxLen;
			displaySize_ = maxLen;
			precision_ = maxLen; // ODBC2.0
			break;
		case Types.VARCHAR:
		case Types.LONGVARCHAR:
		case Types.BLOB:
		case Types.CLOB:
                case Types.VARBINARY:
                        boolean shortLength = maxLen < Math.pow(2, 15);
                        int dataOffset = ((shortLength) ? 2 : 4);
			if (sqlDataType_ == SQLTYPECODE_VARCHAR) {
				sqlOctetLength_ = maxLen + 1;
			} else {
				sqlOctetLength_ = maxLen + dataOffset + 1;
			}
			displaySize_ = maxLen;
			precision_ = maxLen; // ODBC2.0
			break;
		default:
			if (sqlDataType_ == SQLTYPECODE_INTERVAL) {
				dataType_ = Types.OTHER;
				precision_ = odbcPrecision;
				displaySize_ = maxLen; // Make sure maxLen returns the right
				// display size for interval
				// sqlOctetLength_ = maxLen+3;
				// Swastik - commented above line 02/10/2005 for Interval Data
				// Type support
				// 3 was added earlier because interval datatype was handled in
				// the same
				// way as varchar and varchar-long were handled. Since we are
				// separating it
				// we don't need to add the additional 3
				sqlOctetLength_ = maxLen;
			}
			break;
		}
		if (sqlDataType_ == SQLTYPECODE_CHAR || sqlDataType_ == SQLTYPECODE_VARCHAR
				|| sqlDataType_ == SQLTYPECODE_BLOB || sqlDataType_ == SQLTYPECODE_CLOB
				|| sqlDataType_ == SQLTYPECODE_VARCHAR_LONG || sqlDataType_ == SQLTYPECODE_VARCHAR_WITH_LENGTH) {
			isCaseSensitive_ = true;
		}
		isSearchable_ = true;
		fsDataType_ = fsDataType;
		intLeadPrec_ = intLeadPrec;
		paramMode_ = paramMode;
	} // end TrafT4Desc

	// ---------------------------------------------------------------
	// Constructors

	TrafT4Desc(
			int noNullValue // Descriptor2 only
			,
			int nullValue // Descriptor2 only
			,
			int version // Descriptor2 only
			, int dataType, short datetimeCode, int maxLen, short precision, short scale, boolean nullInfo,
			boolean signType // same as signe
			, int odbcDataType, int odbcPrecision, int sqlCharset, int odbcCharset, String colName // same
																									// as
																									// colHeadingNm
			, String tableName, String catalogName, String schemaName, String headingName // Descriptor2
																							// only
			, int intLeadPrec, int paramMode, int fsDataType // fsDataType
			// seems to be
			// the same as
			// dataType (see
			// old
			// descriptor)
			, int rowLength) {

		//
		// Call the old constructor to set the items that are
		// in both the old descriptor and the new descriptor.
		//
		this(dataType, datetimeCode, maxLen, precision, scale, nullInfo, colName, signType, odbcDataType,
				(short) odbcPrecision, sqlCharset, odbcCharset, catalogName, schemaName, tableName, fsDataType,
				intLeadPrec, paramMode);
		//
		// Set the items specific to the new descriptor.
		//
		noNullValue_ = noNullValue;
		nullValue_ = nullValue;
		version_ = version;
		headingName_ = headingName;
		rowLength_ = rowLength;

		maxLen_ = maxLen;

	} // end TrafT4Desc

	// ---------------------------------------------------------------
	/***************************************************************************
	 * Returns encoding type for character data types from Database
	 * COLS table.
	 */
	String getCharacterSetName() throws SQLException {
		switch (dataType_) {
		case Types.CHAR:
		case Types.VARCHAR:
		case Types.LONGVARCHAR:
		case Types.BLOB:
		case Types.CLOB:
			return (String) InterfaceUtilities.getCharsetName(sqlCharset_);
		default:
			return null;
		}
	}

	// ---------------------------------------------------------------
	// Constants
	public static final int SQLTYPECODE_CHAR = 1;
	public static final int SQLTYPECODE_VARCHAR = 12;
	public static final int SQLTYPECODE_VARCHAR_LONG = -1;
	public static final int SQLTYPECODE_INTERVAL = 10;
	public static final int SQLTYPECODE_VARCHAR_WITH_LENGTH = -601;
	public static final int SQLTYPECODE_BLOB = -602;
	public static final int SQLTYPECODE_CLOB = -603;
	public static final int SQLTYPECODE_SMALLINT = 5;
	public static final int SQLTYPECODE_INTEGER = 4;
        public static final int SQLTYPECODE_BINARY = 60;
        public static final int SQLTYPECODE_VARBINARY = 61;
	// datetime codes taken from NCS - File ....\....\...\Common\DrvrSrvr.h
	public static final int SQLDTCODE_YEAR = 4;
	public static final int SQLDTCODE_YEAR_TO_MONTH = 5;
	// public static final int SQLDTCODE_YEAR_TO_DAY 1  //Database 
	// DATE
	public static final int SQLDTCODE_YEAR_TO_HOUR = 7; // ODBC TIMESTAMP(0)
	public static final int SQLDTCODE_YEAR_TO_MINUTE = 8;
	// public static final int SQLDTCODE_YEAR_TO_SECOND 3 //
	// DatabaseTIMESTAMP(0)
	// public static final int SQLDTCODE_YEAR_TO_FRACTION 3 // 
	// Database TIMESTAMP(1 - 5)
	public static final int SQLDTCODE_MONTH = 10;
	public static final int SQLDTCODE_MONTH_TO_DAY = 11;
	public static final int SQLDTCODE_MONTH_TO_HOUR = 12;
	public static final int SQLDTCODE_MONTH_TO_MINUTE = 13;
	public static final int SQLDTCODE_MONTH_TO_SECOND = 14;
	public static final int SQLDTCODE_MONTH_TO_FRACTION = 14;
	public static final int SQLDTCODE_DAY = 15;
	public static final int SQLDTCODE_DAY_TO_HOUR = 16;
	public static final int SQLDTCODE_DAY_TO_MINUTE = 17;
	public static final int SQLDTCODE_DAY_TO_SECOND = 18;
	public static final int SQLDTCODE_DAY_TO_FRACTION = 18;
	public static final int SQLDTCODE_HOUR = 19;
	public static final int SQLDTCODE_HOUR_TO_MINUTE = 20;
	// define SQLDTCODE_HOUR_TO_SECOND 2 //Database TIME(0) --> NCS 
	// Maps this to TIME
	public static final int SQLDTCODE_HOUR_TO_FRACTION = 2; // Database TIME(1 -
	// 6) // MXCI Maps
	// this to TIMESTAMP
	public static final int SQLDTCODE_MINUTE = 22;
	public static final int SQLDTCODE_MINUTE_TO_SECOND = 23;
	public static final int SQLDTCODE_MINUTE_TO_FRACTION = 23;
	public static final int SQLDTCODE_SECOND = 24;
	public static final int SQLDTCODE_SECOND_TO_FRACTION = 24;
	public static final int SQLDTCODE_FRACTION_TO_FRACTION = 29;

	// fields
	int sqlCharset_;
	int odbcCharset_;
	int sqlDataType_;
	int dataType_;
	short sqlPrecision_;
	short sqlDatetimeCode_;
	int sqlOctetLength_;
	int isNullable_;
	String name_;
	short scale_;
	int precision_;
	boolean isSigned_;
	boolean isCurrency_;
	boolean isCaseSensitive_;
	String catalogName_;
	String schemaName_;
	String tableName_;
	int fsDataType_;
	int intLeadPrec_;
	int paramMode_;
	int paramIndex_;
	int paramPos_;

	String columnClassName_;
	int displaySize_;
	// fields which are not pouplated now
	String columnLabel_;
	boolean isAutoIncrement_;
	boolean isSearchable_;
	boolean isValueSet_; // To denote if setXXX method is called for this
	// parameter
	// String paramValue_; // Contains the value of output parameter value
	Object paramValue_; // Contains the value of output parameter value

	int noNullValue_; // Descriptor2 only
	int nullValue_; // Descriptor2 only
	int version_; // Descriptor2 only

	String headingName_; // Descriptor2 only

	int rowLength_;
	int maxLen_;

} // end class TrafT4Desc
