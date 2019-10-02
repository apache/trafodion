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
 * Filename    : SQLMXDatabaseMetaData.java
 * Description :
 *
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.RowIdLifetime;
import java.sql.SQLException;
import java.sql.Types;

/**
 * Comprehensive information about the database as a whole.
 * 
 * <p>
 * This interface is implemented by driver vendors to let users know the
 * capabilities of a Database Management System (DBMS) in combination with the
 * driver based on JDBCTM technology ("JDBC driver") that is used with it.
 * Different relational DBMSs often support different features, implement
 * features in different ways, and use different data types. In addition, a
 * driver may implement a feature on top of what the DBMS offers. Information
 * returned by methods in this interface applies to the capabilities of a
 * particular driver and a particular DBMS working together. Note that as used
 * in this documentation, the term "database" is used generically to refer to
 * both the driver and DBMS.
 * </p>
 * 
 */
/*
 * Methods Changed: getTxid() changes 
 */

public class SQLMXDatabaseMetaData extends SQLMXHandle implements
		java.sql.DatabaseMetaData {
	/**
	 * Retrieves whether the current user can call all the procedures returned
	 * by the method getProcedures.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean allProceduresAreCallable() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_allProceduresAreCallable].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_allProceduresAreCallable].methodExit();
		}
	}

	/**
	 * Retrieves whether the current user can use all the tables returned by the
	 * method getTables in a SELECT statement.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean allTablesAreSelectable() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_allTablesAreSelectable].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_allTablesAreSelectable].methodExit();
		}
	}

	/**
	 * Retrieves the URL for this DBMS.
	 * 
	 * @return the url or null if it can't be generated
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getURL() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getURL].methodEntry();
		try {
			return connection_.url_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getURL].methodExit();
		}
	}

	/**
	 * Retrieves the user name as known to this database.
	 * 
	 * @return the database user name
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getUserName() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getUserName].methodEntry();
		try {
			return connection_.uid_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getUserName].methodExit();
		}
	}

	/**
	 * Retrieves whether this database is in read-only mode.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean isReadOnly() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_isReadOnly].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_isReadOnly].methodExit();
		}
	}

	/**
	 * Retrieves whether NULL values are sorted high. Sorted high means that
	 * NULL values sort higher than any other value in a domain. In an ascending
	 * order, if this method returns true, NULL values will appear at the end.
	 * By contrast, the method nullsAreSortedAtEnd indicates whether NULL values
	 * are sorted at the end regardless of sort order.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean nullsAreSortedHigh() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_nullsAreSortedHigh].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_nullsAreSortedHigh].methodExit();
		}
	}

	/**
	 * Retrieves whether NULL values are sorted low. Sorted low means that NULL
	 * values sort lower than any other value in a domain. In an ascending
	 * order, if this method returns true, NULL values will appear at the
	 * beginning. By contrast, the method nullsAreSortedAtStart indicates
	 * whether NULL values are sorted at the beginning regardless of sort order.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean nullsAreSortedLow() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_nullsAreSortedLow].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_nullsAreSortedLow].methodExit();
		}
	}

	/**
	 * Retrieves whether NULL values are sorted low. Sorted low means that NULL
	 * values sort lower than any other value in a domain. In an ascending
	 * order, if this method returns true, NULL values will appear at the
	 * beginning. By contrast, the method nullsAreSortedAtStart indicates
	 * whether NULL values are sorted at the beginning regardless of sort order.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean nullsAreSortedAtStart() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_nullsAreSortedAtStart].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_nullsAreSortedAtStart].methodExit();
		}
	}

	/**
	 * Retrieves whether NULL values are sorted at the end regardless of sort
	 * order.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean nullsAreSortedAtEnd() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_nullsAreSortedAtEnd].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_nullsAreSortedAtEnd].methodExit();
		}
	}

	/**
	 * Retrieves the name of this database product.
	 * 
	 * @return Trafodion
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	// Note: Any changes to this string must be coordinated with SQLJ
	public String getDatabaseProductName() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getDatabaseProductName].methodEntry();
		try {
			return new String("Trafodion");
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getDatabaseProductName].methodExit();
		}
	}

	/**
	 * Retrieves the version number of this database product.
	 * 
	 * @return Trafodion release number
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getDatabaseProductVersion() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getDatabaseProductVersion].methodEntry();
		try {
			return new String(Integer.toString(T2Driver
					.getDatabaseMajorVersion())
					+ "."
					+ Integer.toString(T2Driver.getDatabaseMinorVersion()));
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getDatabaseProductVersion].methodExit();
		}
	}

	/**
	 * Retrieves the name of this JDBC driver.
	 * 
	 * @return org.apache.trafodion.jdbc.t2.T2Driver
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getDriverName() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getDriverName].methodEntry();
		try {
			return new String("org.apache.trafodion.jdbc.t2.T2Driver");
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getDriverName].methodExit();
		}
	}

	/**
	 * Retrieves the version number of this JDBC driver as a String.
	 * 
	 * @return TppppVnn where pppp is the product number and nn is the major and
	 *         minor number of the version
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getDriverVersion() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getDriverVersion].methodEntry();
		try {
			return new String("Trafodion JDBC T2 driver Version "
					+ DriverInfo.driverVproc);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getDriverVersion].methodExit();
		}
	}

	/**
	 * Retrieves this JDBC driver's major version number.
	 * 
	 * @return 3
	 **/
	public int getDriverMajorVersion() {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getDriverMajorVersion].methodEntry();
		try {
			return DriverInfo.JdbcMajorVersion;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getDriverMajorVersion].methodExit();
		}
	}

	/**
	 * Retrieves this JDBC driver's minor version number.
	 * 
	 * @return 2
	 **/
	public int getDriverMinorVersion() {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getDriverMinorVersion].methodEntry();
		try {
			return DriverInfo.JdbcMinorVersion;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getDriverMinorVersion].methodExit();
		}
	}

	/**
	 * Retrieves whether this database stores tables in a local file.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean usesLocalFiles() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_usesLocalFiles].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_usesLocalFiles].methodExit();
		}
	}

	/**
	 * Retrieves whether this database uses a file for each table.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean usesLocalFilePerTable() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_usesLocalFilePerTable].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_usesLocalFilePerTable].methodExit();
		}
	}

	/**
	 * Retrieves whether this database treats mixed case unquoted SQL
	 * identifiers as case sensitive and as a result stores them in mixed case.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsMixedCaseIdentifiers() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsMixedCaseIdentifiers].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsMixedCaseIdentifiers].methodExit();
		}
	}

	/**
	 * Retrieves whether this database treats mixed case unquoted SQL
	 * identifiers as case insensitive and stores them in upper case.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean storesUpperCaseIdentifiers() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_storesUpperCaseIdentifiers].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_storesUpperCaseIdentifiers].methodExit();
		}
	}

	/**
	 * Retrieves whether this database treats mixed case unquoted SQL
	 * identifiers as case insensitive and stores them in lower case.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean storesLowerCaseIdentifiers() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_storesLowerCaseIdentifiers].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_storesLowerCaseIdentifiers].methodExit();
		}
	}

	/**
	 * Retrieves whether this database treats mixed case unquoted SQL
	 * identifiers as case insensitive and stores them in mixed case.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean storesMixedCaseIdentifiers() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_storesMixedCaseIdentifiers].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_storesMixedCaseIdentifiers].methodExit();
		}
	}

	/**
	 * Retrieves whether this database treats mixed case quoted SQL identifiers
	 * as case sensitive and as a result stores them in mixed case.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsMixedCaseQuotedIdentifiers() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsMixedCaseQuotedIdentifiers].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsMixedCaseQuotedIdentifiers].methodExit();
		}
	}

	/**
	 * Retrieves whether this database treats mixed case quoted SQL identifiers
	 * as case insensitive and stores them in upper case.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean storesUpperCaseQuotedIdentifiers() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_storesUpperCaseQuotedIdentifiers].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_storesUpperCaseQuotedIdentifiers].methodExit();
		}
	}

	/**
	 * Retrieves whether this database treats mixed case quoted SQL identifiers
	 * as case insensitive and stores them in lower case.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean storesLowerCaseQuotedIdentifiers() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_storesLowerCaseQuotedIdentifiers].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_storesLowerCaseQuotedIdentifiers].methodExit();
		}
	}

	/**
	 * Retrieves whether this database treats mixed case quoted SQL identifiers
	 * as case insensitive and stores them in mixed case.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean storesMixedCaseQuotedIdentifiers() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_storesMixedCaseQuotedIdentifiers].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_storesMixedCaseQuotedIdentifiers].methodExit();
		}
	}

	/**
	 * Retrieves the string used to quote SQL identifiers. This method returns a
	 * space " " if identifier quoting is not supported.
	 * 
	 * @return "\"
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getIdentifierQuoteString() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getIdentifierQuoteString].methodEntry();
		try {
			return new String("\"");
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getIdentifierQuoteString].methodExit();
		}
	}

	/**
	 * Retrieves a comma-separated list of all of this database's SQL keywords
	 * that are NOT also SQL92 keywords.
	 * 
	 * @return DATETIME,FRACTION,PROTOTYPE,SQL_CHAR,SQL_DATE,SQL_DECIMAL,
	 *         SQL_DOUBLE,SQL_FLOAT,SQL_INT,SQL_INTEGER,SQL_REAL,SQL_SMALLINT,
	 *         SQL_TIME,SQL_TIMESTAMP,SQL_VARCHAR,TRANSPOSE,UPSHIFT
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getSQLKeywords() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getSQLKeywords].methodEntry();
		try {
			return "DATETIME,FRACTION,PROTOTYPE,SQL_CHAR,SQL_DATE,SQL_DECIMAL,SQL_DOUBLE,SQL_FLOAT"
					+ ",SQL_INT,SQL_INTEGER,SQL_REAL,SQL_SMALLINT,SQL_TIME,SQL_TIMESTAMP,SQL_VARCHAR"
					+ ",TRANSPOSE,UPSHIFT";
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getSQLKeywords].methodExit();
		}
	}

	/**
	 * Retrieves a comma-separated list of math functions available with this
	 * database. These are the Open /Open CLI math function names used in the
	 * JDBC function escape clause.
	 * 
	 * @return ABS,ACOS,ASIN,ATAN,ATAN2,CEILING,COS,COSH,DEGREES,EXP,FLOOR,
	 *         LOG,LOG10,MOD,PI,POWER,RADIANS,SIGN,SIN,SINH,SQRT,TAN,TANH
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getNumericFunctions() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getNumericFunctions].methodEntry();
		try {
			return new String(
					"ABS,ACOS,ASIN,ATAN,ATAN2,CEILING,COS,COSH,DEGREES,EXP,FLOOR,LOG,LOG10,MOD,PI,POWER,RADIANS,"
							+ "SIGN,SIN,SINH,SQRT,TAN,TANH");
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getNumericFunctions].methodExit();
		}
	}

	/**
	 * Retrieves a comma-separated list of string functions available with this
	 * database. These are the Open Group CLI string function names used in the
	 * JDBC function escape clause.
	 * 
	 * @return 
	 *         ASCII,CHAR,CHAR_LENGTH,CODE_VALUE,CONCAT,INSERT,LCASE,LEFT,LOCATE,
	 *         LOWER,LPAD,LTRIM,OCTET_LENGTH,POSITION,REPEAT,REPLACE,RIGHT,
	 *         RPAD,RTRIM,SPACE,SUBSTRING,TRANSLATE,TRIM,UCASE,UPPER,UPSHIFT
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getStringFunctions() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getStringFunctions].methodEntry();
		try {
			return new String(
					"ASCII,CHAR,CHAR_LENGTH,CODE_VALUE,CONCAT,INSERT,LCASE,LEFT,LOCATE,LOWER,LPAD,LTRIM,OCTET_LENGTH,"
							+ "POSITION,REPEAT,REPLACE,RIGHT,RPAD,RTRIM,SPACE,SUBSTRING,TRANSLATE,TRIM,UCASE,UPPER,UPSHIFT");
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getStringFunctions].methodExit();
		}
	}

	/**
	 * Retrieves a comma-separated list of system functions available with this
	 * database. These are the Open Group CLI system function names used in the
	 * JDBC function escape clause.
	 * 
	 * @return CURRENT_USER,SESSION_USER,USER
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getSystemFunctions() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getSystemFunctions].methodEntry();
		try {
			return new String("CURRENT_USER,SESSION_USER,USER");
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getSystemFunctions].methodExit();
		}
	}

	/**
	 * Retrieves a comma-separated list of the time and date functions available
	 * with this database.
	 * 
	 * @return CONVERTTIMESTAMP,CURRENT,CURRENT_DATE,CURRENT_TIME,
	 *         CURRENT_TIMESTAMP,DATEFORMAT,DAY,DAYNAME,DAYOFMONTH,DAYOFWEEK,
	 *         DAYOFYEAR,EXTRACT,HOUR,JULIANTIMESTAMP,MINUTE,MONTH,MONTHNAME,
	 *         QUARTER,SECOND,WEEK,YEAR
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getTimeDateFunctions() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getTimeDateFunctions].methodEntry();
		try {
			return new String(
					"CONVERTTIMESTAMP,CURRENT,CURRENT_DATE,CURRENT_TIME,CURRENT_TIMESTAMP,"
							+ "DATEFORMAT,DAY,DAYNAME,DAYOFMONTH,DAYOFWEEK,DAYOFYEAR,EXTRACT,HOUR,JULIANTIMESTAMP,MINUTE,"
							+ "MONTH,MONTHNAME,QUARTER,SECOND,WEEK,YEAR");
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getTimeDateFunctions].methodExit();
		}
	}

	/**
	 * Retrieves the string that can be used to escape '_' or '%' in the string
	 * pattern style catalog search parameters.
	 * 
	 * <P>
	 * The '_' character represents any single character.
	 * <P>
	 * The '%' character represents any sequence of zero or more characters.
	 * 
	 * @return "\\"
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getSearchStringEscape() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getSearchStringEscape].methodEntry();
		try {
			return new String("\\");
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getSearchStringEscape].methodExit();
		}
	}

	/**
	 * Retrieves all the "extra" characters that can be used in unquoted
	 * identifier names (those beyond a-z, A-Z, 0-9 and _).
	 * 
	 * @return null
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getExtraNameCharacters() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getExtraNameCharacters].methodEntry();
		try {
			return null;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getExtraNameCharacters].methodExit();
		}
	}

	// --------------------------------------------------------------------
	// Functions describing which features are supported.

	/**
	 * Retrieves whether this database supports ALTER TABLE with add column.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsAlterTableWithAddColumn() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsAlterTableWithAddColumn].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsAlterTableWithAddColumn].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports ALTER TABLE with drop column.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsAlterTableWithDropColumn() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsAlterTableWithDropColumn].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsAlterTableWithDropColumn].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports column aliasing.
	 * 
	 * <P>
	 * If so, the SQL AS clause can be used to provide names for computed
	 * columns or to provide alias names for columns as required.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsColumnAliasing() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsColumnAliasing].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsColumnAliasing].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports concatenations between NULL and
	 * non-NULL values being NULL.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean nullPlusNonNullIsNull() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_nullPlusNonNullIsNull].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_nullPlusNonNullIsNull].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports the CONVERT function between SQL
	 * types.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsConvert() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsConvert_V].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsConvert_V].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports the CONVERT for two given SQL
	 * types.
	 * 
	 * @param fromType
	 *            the type to convert from
	 * @param toType
	 *            the type to convert to
	 * 
	 * @return true if<BR>
	 *         <UL>
	 *         <B>fromType:</B>
	 *         BIGINT,DECIMAL,DOUBLE,FLOAT,INTEGER,NUMERIC,REAL,SMALLINT<BR>
	 *         <B>toType:</B>
	 *         CHAR,NUMERIC,DECIMAL,INTEGER,SMALLINT,FLOAT,REAL,DOUBLE,
	 *         VARCHAR,LONGVARCHAR,BIGINT
	 *         <P>
	 *         <B>fromType:</B> CHAR,VARCHAR,LONGVARCHAR<BR>
	 *         <B>toType:</B>
	 *         CHAR,NUMERIC,DECIMAL,INTEGER,SMALLINT,FLOAT,REAL,DOUBLE,
	 *         VARCHAR,LONGVARCHAR,BIGINT,DATE,TIME,TIMESTAMP
	 *         <P>
	 *         <B>fromType:</B> DATE<BR>
	 *         <B>toType:</B> CHAR,VARCHAR,LONGVARCHAR,DATE,TIMESTAMP
	 *         <P>
	 *         <B>fromType:</B> TIME<BR>
	 *         <B>toType:</B> CHAR,VARCHAR,LONGVARCHAR,TIME,TIMESTAM
	 *         <P>
	 *         <B>fromType:</B> TIMESTAMP<BR>
	 *         <B>toType:</B> CHAR,VARCHAR,LONGVARCHAR,DATE,TIME,TIMESTAMP
	 *         </UL>
	 *         <P>
	 *         false - All other fromType, toType conversions
	 *         </P>
	 * 
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @see Types
	 */
	public boolean supportsConvert(int fromType, int toType)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsConvert_II].methodEntry();
		try {
			switch (fromType) {
			case Types.BIGINT:
			case Types.DECIMAL:
			case Types.DOUBLE:
			case Types.FLOAT:
			case Types.INTEGER:
			case Types.NUMERIC:
			case Types.REAL:
			case Types.SMALLINT:
				switch (toType) {
				case Types.CHAR:
				case Types.NUMERIC:
				case Types.DECIMAL:
				case Types.INTEGER:
				case Types.SMALLINT:
				case Types.FLOAT:
				case Types.REAL:
				case Types.DOUBLE:
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
				case Types.BIGINT:
					return true;
				}
				return false;
			case Types.CHAR:
			case Types.VARCHAR:
			case Types.LONGVARCHAR:
				switch (toType) {
				case Types.CHAR:
				case Types.NUMERIC:
				case Types.DECIMAL:
				case Types.INTEGER:
				case Types.SMALLINT:
				case Types.FLOAT:
				case Types.REAL:
				case Types.DOUBLE:
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
				case Types.BIGINT:
				case Types.DATE:
				case Types.TIME:
				case Types.TIMESTAMP:
					return true;
				}
				return false;
			case Types.DATE:
				switch (toType) {
				case Types.CHAR:
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
				case Types.DATE:
				case Types.TIMESTAMP:
					return true;
				}
				return false;
			case Types.TIME:
				switch (toType) {
				case Types.CHAR:
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
				case Types.TIME:
				case Types.TIMESTAMP:
					return true;
				}
				return false;
			case Types.TIMESTAMP:
				switch (toType) {
				case Types.CHAR:
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
				case Types.DATE:
				case Types.TIME:
				case Types.TIMESTAMP:
					return true;
				}
				return false;
			case Types.BIT:
			case Types.BINARY:
			case Types.VARBINARY:
			case Types.LONGVARBINARY:
			case Types.TINYINT:
				return false;
			}
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsConvert_II].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports table correlation names.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsTableCorrelationNames() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsTableCorrelationNames].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsTableCorrelationNames].methodExit();
		}
	}

	/**
	 * Retrieves whether, when table correlation names are supported, they are
	 * restricted to being different from the names of the tables.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsDifferentTableCorrelationNames() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsDifferentTableCorrelationNames]
					.methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsDifferentTableCorrelationNames]
						.methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports expressions in ORDER BY lists.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsExpressionsInOrderBy() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsExpressionsInOrderBy].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsExpressionsInOrderBy].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports using a column that is not in
	 * the SELECT statement in an ORDER BY clause.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsOrderByUnrelated() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsOrderByUnrelated].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsOrderByUnrelated].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports some form of GROUP BY clause.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsGroupBy() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsGroupBy].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsGroupBy].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports using a column that is not in
	 * the SELECT statement in a GROUP BY clause.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsGroupByUnrelated() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsGroupByUnrelated].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsGroupByUnrelated].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports using columns not included in
	 * the SELECT statement in a GROUP BY clause provided that all of the
	 * columns in the SELECT statement are included in the GROUP BY clause.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsGroupByBeyondSelect() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsGroupByBeyondSelect].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsGroupByBeyondSelect].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports specifying a LIKE escape clause.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsLikeEscapeClause() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsLikeEscapeClause].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsLikeEscapeClause].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports getting multiple ResultSet
	 * objects from a single call to the method execute.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsMultipleResultSets() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsMultipleResultSets].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsMultipleResultSets].methodExit();
		}
	}

	/**
	 * Retrieves whether this database allows having multiple transactions open
	 * at once (on different connections).
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsMultipleTransactions() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsMultipleTransactions].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsMultipleTransactions].methodExit();
		}
	}

	/**
	 * Retrieves whether columns in this database may be defined as
	 * non-nullable.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsNonNullableColumns() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsNonNullableColumns].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsNonNullableColumns].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports the ODBC Minimum SQL grammar.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsMinimumSQLGrammar() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsMinimumSQLGrammar].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsMinimumSQLGrammar].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports the ODBC Core SQL grammar.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsCoreSQLGrammar() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsCoreSQLGrammar].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsCoreSQLGrammar].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports the ODBC Extended SQL grammar.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsExtendedSQLGrammar() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsExtendedSQLGrammar].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsExtendedSQLGrammar].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports the ANSI92 entry level SQL
	 * grammar.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsANSI92EntryLevelSQL() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsANSI92EntryLevelSQL].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsANSI92EntryLevelSQL].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports the ANSI92 intermediate SQL
	 * grammar supported.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsANSI92IntermediateSQL() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsANSI92IntermediateSQL].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsANSI92IntermediateSQL].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports the ANSI92 full SQL grammar
	 * supported.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsANSI92FullSQL() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsANSI92FullSQL].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsANSI92FullSQL].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports the SQL Integrity Enhancement
	 * Facility.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsIntegrityEnhancementFacility() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsIntegrityEnhancementFacility].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsIntegrityEnhancementFacility]
						.methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports some form of outer join.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsOuterJoins() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsOuterJoins].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsOuterJoins].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports full nested outer joins.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsFullOuterJoins() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsFullOuterJoins].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsFullOuterJoins].methodExit();
		}
	}

	/**
	 * Retrieves whether this database provides limited support for outer joins.
	 * (This will be true if the method supportsFullOuterJoins returns true).
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsLimitedOuterJoins() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsLimitedOuterJoins].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsLimitedOuterJoins].methodExit();
		}
	}

	/**
	 * Retrieves the database vendor's preferred term for "schema".
	 * 
	 * @return SCHEMA
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getSchemaTerm() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getSchemaTerm].methodEntry();
		try {
			return new String("SCHEMA");
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getSchemaTerm].methodExit();
		}
	}

	/**
	 * Retrieves the database vendor's preferred term for "procedure".
	 * 
	 * @return PROCEDURE
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getProcedureTerm() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getProcedureTerm].methodEntry();
		try {
			return new String("PROCEDURE");
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getProcedureTerm].methodExit();
		}
	}

	/**
	 * Retrieves the database vendor's preferred term for "catalog".
	 * 
	 * @return CATALOG
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getCatalogTerm() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getCatalogTerm].methodEntry();
		try {
			return new String("CATALOG");
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getCatalogTerm].methodExit();
		}
	}

	/**
	 * Retrieves whether a catalog appears at the start of a fully qualified
	 * table name. If not, the catalog appears at the end.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean isCatalogAtStart() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_isCatalogAtStart].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_isCatalogAtStart].methodExit();
		}
	}

	/**
	 * Retrieves the String that this database uses as the separator between a
	 * catalog and table name.
	 * 
	 * @return "."
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public String getCatalogSeparator() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getCatalogSeparator].methodEntry();
		try {
			return new String(".");
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getCatalogSeparator].methodExit();
		}
	}

	/**
	 * Retrieves whether a schema name can be used in a data manipulation
	 * statement.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsSchemasInDataManipulation() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsSchemasInDataManipulation].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsSchemasInDataManipulation].methodExit();
		}
	}

	/**
	 * Retrieves whether a schema name can be used in a procedure call
	 * statement.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsSchemasInProcedureCalls() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsSchemasInProcedureCalls].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsSchemasInProcedureCalls].methodExit();
		}
	}

	/**
	 * Retrieves whether a schema name can be used in a table definition
	 * statement.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsSchemasInTableDefinitions() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsSchemasInTableDefinitions].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsSchemasInTableDefinitions].methodExit();
		}
	}

	/**
	 * Retrieves whether a schema name can be used in an index definition
	 * statement.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsSchemasInIndexDefinitions() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsSchemasInIndexDefinitions].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsSchemasInIndexDefinitions].methodExit();
		}
	}

	/**
	 * Retrieves whether a schema name can be used in a privilege definition
	 * statement.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsSchemasInPrivilegeDefinitions() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsSchemasInPrivilegeDefinitions].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsSchemasInPrivilegeDefinitions]
						.methodExit();
		}
	}

	/**
	 * Retrieves whether a catalog name can be used in a data manipulation
	 * statement.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsCatalogsInDataManipulation() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsCatalogsInDataManipulation].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsCatalogsInDataManipulation].methodExit();
		}
	}

	/**
	 * Retrieves whether a catalog name can be used in a procedure call
	 * statement.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsCatalogsInProcedureCalls() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsCatalogsInProcedureCalls].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsCatalogsInProcedureCalls].methodExit();
		}
	}

	/**
	 * Retrieves whether a catalog name can be used in a table definition
	 * statement.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsCatalogsInTableDefinitions() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsCatalogsInTableDefinitions].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsCatalogsInTableDefinitions].methodExit();
		}
	}

	/**
	 * Retrieves whether a catalog name can be used in an index definition
	 * statement.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsCatalogsInIndexDefinitions() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsCatalogsInIndexDefinitions].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsCatalogsInIndexDefinitions].methodExit();
		}
	}

	/**
	 * Retrieves whether a catalog name can be used in a privilege definition
	 * statement.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsCatalogsInPrivilegeDefinitions() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsCatalogsInPrivilegeDefinitions]
					.methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsCatalogsInPrivilegeDefinitions]
						.methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports positioned DELETE statements.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsPositionedDelete() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsPositionedDelete].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsPositionedDelete].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports positioned UPDATE statements.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsPositionedUpdate() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsPositionedUpdate].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsPositionedUpdate].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports SELECT FOR UPDATE statements.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsSelectForUpdate() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsSelectForUpdate].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsSelectForUpdate].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports stored procedure calls that use
	 * the stored procedure escape syntax.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsStoredProcedures() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsStoredProcedures].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsStoredProcedures].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports subqueries in comparison
	 * expressions.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsSubqueriesInComparisons() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsSubqueriesInComparisons].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsSubqueriesInComparisons].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports subqueries in EXISTS
	 * expressions.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsSubqueriesInExists() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsSubqueriesInExists].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsSubqueriesInExists].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports subqueries in IN statements.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsSubqueriesInIns() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsSubqueriesInIns].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsSubqueriesInIns].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports subqueries in quantified
	 * expressions.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsSubqueriesInQuantifieds() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsSubqueriesInQuantifieds].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsSubqueriesInQuantifieds].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports correlated subqueries.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsCorrelatedSubqueries() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsCorrelatedSubqueries].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsCorrelatedSubqueries].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports SQL UNION.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsUnion() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsUnion].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsUnion].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports SQL UNION ALL.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsUnionAll() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsUnionAll].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsUnionAll].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports keeping cursors open across
	 * commits.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsOpenCursorsAcrossCommit() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsOpenCursorsAcrossCommit].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsOpenCursorsAcrossCommit].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports keeping cursors open across
	 * rollbacks.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsOpenCursorsAcrossRollback() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsOpenCursorsAcrossRollback].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsOpenCursorsAcrossRollback].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports keeping statements open across
	 * commits.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsOpenStatementsAcrossCommit() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsOpenStatementsAcrossCommit].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsOpenStatementsAcrossCommit].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports keeping statements open across
	 * rollbacks.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsOpenStatementsAcrossRollback() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsOpenStatementsAcrossRollback].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsOpenStatementsAcrossRollback]
						.methodExit();
		}
	}

	// ----------------------------------------------------------------------
	// The following group of methods exposes various limitations
	// based on the target database with the current driver.
	// Unless otherwise specified, a result of zero means there is no
	// limit, or the limit is not known.

	/**
	 * Retrieves the maximum number of hex characters this database allows in an
	 * inline binary literal.
	 * 
	 * @return 4050
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxBinaryLiteralLength() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxBinaryLiteralLength].methodEntry();
		try {
			return 4050;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxBinaryLiteralLength].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of characters this database allows for a
	 * character literal.
	 * 
	 * @return 4050
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxCharLiteralLength() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxCharLiteralLength].methodEntry();
		try {
			return 4050;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxCharLiteralLength].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of characters this database allows for a
	 * column name.
	 * 
	 * @return 128
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxColumnNameLength() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxColumnNameLength].methodEntry();
		try {
			return 128;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxColumnNameLength].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of columns this database allows in a GROUP
	 * BY clause.
	 * 
	 * @return 0 - a result of zero means there is no limit, or the limit is not
	 *         known.
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxColumnsInGroupBy() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxColumnsInGroupBy].methodEntry();
		try {
			return 0;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxColumnsInGroupBy].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of columns this database allows in an index.
	 * 
	 * @return 0 - a result of zero means there is no limit, or the limit is not
	 *         known.
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxColumnsInIndex() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxColumnsInIndex].methodEntry();
		try {
			return 0;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxColumnsInIndex].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of columns this database allows in an ORDER
	 * BY clause.
	 * 
	 * @return 0- a result of zero means there is no limit, or the limit is not
	 *         known.
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxColumnsInOrderBy() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxColumnsInOrderBy].methodEntry();
		try {
			return 0;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxColumnsInOrderBy].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of columns this database allows in a SELECT
	 * list.
	 * 
	 * @return 0 - a result of zero means there is no limit, or the limit is not
	 *         known.
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxColumnsInSelect() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxColumnsInSelect].methodEntry();
		try {
			return 0;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxColumnsInSelect].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of columns this database allows in a table.
	 * 
	 * @return 0 - a result of zero means there is no limit, or the limit is not
	 *         known.
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxColumnsInTable() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxColumnsInTable].methodEntry();
		try {
			return 0;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxColumnsInTable].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of concurrent connections to this database
	 * that are possible.
	 * 
	 * @return 0 - a result of zero means there is no limit, or the limit is not
	 *         known.
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxConnections() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxConnections].methodEntry();
		try {
			return 0;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxConnections].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of characters that this database allows in a
	 * cursor name.
	 * 
	 * @return 128
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxCursorNameLength() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxCursorNameLength].methodEntry();
		try {
			return 128;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxCursorNameLength].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of bytes this database allows for an index,
	 * including all of the parts of the index.
	 * 
	 * @return 4050
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxIndexLength() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxIndexLength].methodEntry();
		try {
			return 4050;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxIndexLength].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of characters that this database allows in a
	 * schema name.
	 * 
	 * @return 128
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxSchemaNameLength() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxSchemaNameLength].methodEntry();
		try {
			return 128;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxSchemaNameLength].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of characters that this database allows in a
	 * procedure name.
	 * 
	 * @return 128
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxProcedureNameLength() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxProcedureNameLength].methodEntry();
		try {
			return 128;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxProcedureNameLength].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of characters that this database allows in a
	 * catalog name.
	 * 
	 * @return 128
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxCatalogNameLength() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxCatalogNameLength].methodEntry();
		try {
			return 128;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxCatalogNameLength].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of bytes this database allows in a single
	 * row.
	 * 
	 * @return 4050
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxRowSize() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxRowSize].methodEntry();
		try {
			return 4050;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxRowSize].methodExit();
		}
	}

	/**
	 * Retrieves whether the return value for the method getMaxRowSize includes
	 * the SQL data types LONGVARCHAR and LONGVARBINARY.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean doesMaxRowSizeIncludeBlobs() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_doesMaxRowSizeIncludeBlobs].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_doesMaxRowSizeIncludeBlobs].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of characters this database allows in an SQL
	 * statement.
	 * 
	 * @return 0 - a result of zero means there is no limit, or the limit is not
	 *         known.
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxStatementLength() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxStatementLength].methodEntry();
		try {
			return 0;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxStatementLength].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of active statements to this database that
	 * can be open at the same time.
	 * 
	 * @return 0 - a result of zero means there is no limit, or the limit is not
	 *         known.
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxStatements() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxStatements].methodEntry();
		try {
			return 0;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxStatements].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of characters this database allows in a
	 * table name.
	 * 
	 * @return 128
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxTableNameLength() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxTableNameLength].methodEntry();
		try {
			return 128;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxTableNameLength].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of tables this database allows in a SELECT
	 * statement.
	 * 
	 * @return 0 - a result of zero means there is no limit, or the limit is not
	 *         known.
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxTablesInSelect() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxTablesInSelect].methodEntry();
		try {
			return 0;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxTablesInSelect].methodExit();
		}
	}

	/**
	 * Retrieves the maximum number of characters this database allows in a user
	 * name.
	 * 
	 * @return 32 (in bytes)
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getMaxUserNameLength() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMaxUserNameLength].methodEntry();
		try {
			return 32;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMaxUserNameLength].methodExit();
		}
	}

	// ----------------------------------------------------------------------

	/**
	 * Retrieves this database's default transaction isolation level. The
	 * possible values are defined in java.sql.Connection.
	 * 
	 * @return Connection.TRANSACTION_READ_COMMITTED
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public int getDefaultTransactionIsolation() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getDefaultTransactionIsolation].methodEntry();
		try {
			return Connection.TRANSACTION_READ_COMMITTED;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getDefaultTransactionIsolation].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports transactions. If not, invoking
	 * the method commit is a noop, and the isolation level is TRANSACTION_NONE.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsTransactions() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsTransactions].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsTransactions].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports the given transaction isolation
	 * level.
	 * 
	 * @param level
	 *            the values are defined in java.sql.Connection
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsTransactionIsolationLevel(int level)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsTransactionIsolationLevel].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsTransactionIsolationLevel].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports both data definition and data
	 * manipulation statements within a transaction.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsDataDefinitionAndDataManipulationTransactions()
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsDataDefinitionAndDataManipulationTransactions]
					.methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsDataDefinitionAndDataManipulationTransactions]
						.methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports only data manipulation
	 * statements within a transaction.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean supportsDataManipulationTransactionsOnly()
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsDataManipulationTransactionsOnly]
					.methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsDataManipulationTransactionsOnly]
						.methodExit();
		}
	}

	/**
	 * Retrieves whether a data definition statement within a transaction forces
	 * the transaction to commit.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean dataDefinitionCausesTransactionCommit() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_dataDefinitionCausesTransactionCommit].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_dataDefinitionCausesTransactionCommit]
						.methodExit();
		}
	}

	/**
	 * Retrieves whether this database ignores a data definition statement
	 * within a transaction.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public boolean dataDefinitionIgnoredInTransactions() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_dataDefinitionIgnoredInTransactions].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_dataDefinitionIgnoredInTransactions]
						.methodExit();
		}
	}

	/**
	 * Get a description of stored procedures available in a catalog.
	 * 
	 * <P>
	 * Only procedure descriptions matching the schema and procedure name
	 * criteria are returned. They are ordered by PROCEDURE_SCHEM, and
	 * PROCEDURE_NAME.
	 * 
	 * <P>
	 * Each procedure description has the following columns:
	 * <OL>
	 * <LI><B>PROCEDURE_CAT</B> String => procedure catalog (may be null)
	 * <LI><B>PROCEDURE_SCHEM</B> String => procedure schema (may be null)
	 * <LI><B>PROCEDURE_NAME</B> String => procedure name
	 * <LI>reserved for future use
	 * <LI>reserved for future use
	 * <LI>reserved for future use
	 * <LI><B>REMARKS</B> String => explanatory comment on the procedure
	 * <LI><B>PROCEDURE_TYPE</B> short => kind of procedure:
	 * <UL>
	 * <LI>procedureResultUnknown - May return a result
	 * <LI>procedureNoResult - Does not return a result
	 * <LI>procedureReturnsResult - Returns a result
	 * </UL>
	 * </OL>
	 * 
	 * @param catalog
	 *            a catalog name; "" retrieves those without a catalog; null
	 *            means drop catalog name from the selection criteria
	 * @param schemaPattern
	 *            a schema name pattern; "" retrieves those without a schema
	 * @param procedureNamePattern
	 *            a procedure name pattern
	 * @return ResultSet - each row is a procedure description
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @see #getSearchStringEscape
	 **/
	public java.sql.ResultSet getProcedures(String catalog,
			String schemaPattern, String procedureNamePattern)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getProcedures].methodEntry();
		try {
			SQLMXResultSet resultSet;
			String catalogNm;
			String schemaNm;

			int i;

			clearWarnings();
			/*
			 * RFE: Connection synchronization Connection object is now
			 * synchronized.
			 */
			synchronized (connection_) {
				if (connection_.isClosed_)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_connection", null);

				if (catalog == null)
					catalogNm = connection_.catalog_;
				else
					catalogNm = catalog;

				if (schemaPattern == null)
					schemaNm = connection_.schema_;
				else
					schemaNm = schemaPattern;
				resultSet = getProcedures(connection_.server_,
						connection_.getDialogueId(), connection_.getTxid(),
						connection_.autoCommit_, connection_.transactionMode_,
						catalogNm, schemaNm, procedureNamePattern);
			}// End sync
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getProcedures].methodExit();
		}
	}

	/**
	 * Get a description of a catalog's stored procedure parameters and result
	 * columns.
	 * 
	 * <P>
	 * Only descriptions matching the schema, procedure and parameter name
	 * criteria are returned. They are ordered by PROCEDURE_SCHEM and
	 * PROCEDURE_NAME. Within this, the return value, if any, is first. Next are
	 * the parameter descriptions in call order. The column descriptions follow
	 * in column number order.
	 * 
	 * <P>
	 * Each row in the ResultSet is a parameter description or column
	 * description with the following fields:
	 * <OL>
	 * <LI><B>PROCEDURE_CAT</B> String => procedure catalog (may be null)
	 * <LI><B>PROCEDURE_SCHEM</B> String => procedure schema (may be null)
	 * <LI><B>PROCEDURE_NAME</B> String => procedure name
	 * <LI><B>COLUMN_NAME</B> String => column/parameter name
	 * <LI><B>COLUMN_TYPE</B> Short => kind of column/parameter:
	 * <UL>
	 * <LI>procedureColumnUnknown - nobody knows
	 * <LI>procedureColumnIn - IN parameter
	 * <LI>procedureColumnInOut - INOUT parameter
	 * <LI>procedureColumnOut - OUT parameter
	 * <LI>procedureColumnReturn - procedure return value
	 * <LI>procedureColumnResult - result column in ResultSet
	 * </UL>
	 * <LI><B>DATA_TYPE</B> short => SQL type from java.sql.Types
	 * <LI><B>TYPE_NAME</B> String => SQL type name
	 * <LI><B>PRECISION</B> int => precision
	 * <LI><B>LENGTH</B> int => length in bytes of data
	 * <LI><B>SCALE</B> short => scale
	 * <LI><B>RADIX</B> short => radix
	 * <LI><B>NULLABLE</B> short => can it contain NULL?
	 * <UL>
	 * <LI>procedureNoNulls - does not allow NULL values
	 * <LI>procedureNullable - allows NULL values
	 * <LI>procedureNullableUnknown - nullability unknown
	 * </UL>
	 * <LI><B>REMARKS</B> String => comment describing parameter/column
	 * </OL>
	 * 
	 * <P>
	 * <B>Note:</B> Some databases may not return the column descriptions for a
	 * procedure. Additional columns beyond REMARKS can be defined by the
	 * database.
	 * 
	 * @param catalog
	 *            a catalog name; "" retrieves those without a catalog; null
	 *            means drop catalog name from the selection criteria
	 * @param schemaPattern
	 *            a schema name pattern; "" retrieves those without a schema
	 * @param procedureNamePattern
	 *            a procedure name pattern
	 * @param columnNamePattern
	 *            a column name pattern
	 * @return ResultSet - each row is a stored procedure parameter or column
	 *         description
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @see #getSearchStringEscape
	 **/
	public java.sql.ResultSet getProcedureColumns(String catalog,
			String schemaPattern, String procedureNamePattern,
			String columnNamePattern) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getProcedureColumns].methodEntry();
		try {
			SQLMXResultSet resultSet;
			String catalogNm;
			String schemaNm;

			int i;

			clearWarnings();
			/*
			 * RFE: Connection synchronization Connection object is now
			 * synchronized.
			 */
			synchronized (connection_) {
				if (connection_.isClosed_)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_connection", null);

				if (catalog == null)
					catalogNm = connection_.catalog_;
				else
					catalogNm = catalog;

				if (schemaPattern == null)
					schemaNm = connection_.schema_;
				else
					schemaNm = schemaPattern;
				resultSet = getProcedureColumns(connection_.server_,
						connection_.getDialogueId(), connection_.getTxid(),
						connection_.autoCommit_, connection_.transactionMode_,
						catalogNm, schemaNm, procedureNamePattern,
						columnNamePattern);
			}// End sync
			// path column Names as per JDBC specification
			resultSet.setColumnName(8, "PRECISION");
			resultSet.setColumnName(9, "LENGTH");
			resultSet.setColumnName(10, "SCALE");
			resultSet.setColumnName(11, "RADIX");
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getProcedureColumns].methodExit();
		}
	}

	/**
	 * Get a description of tables available in a catalog.
	 * 
	 * <P>
	 * Only table descriptions matching the catalog, schema, table name and type
	 * criteria are returned. They are ordered by TABLE_TYPE, TABLE_SCHEM and
	 * TABLE_NAME.
	 * 
	 * <P>
	 * Each table description has the following columns:
	 * <OL>
	 * <LI><B>TABLE_CAT</B> String => table catalog (may be null)
	 * <LI><B>TABLE_SCHEM</B> String => table schema (may be null)
	 * <LI><B>TABLE_NAME</B> String => table name
	 * <LI><B>TABLE_TYPE</B> String => table type. Typical types are "TABLE",
	 * "VIEW", "SYSTEM TABLE", "GLOBAL TEMPORARY", "LOCAL TEMPORARY", "ALIAS",
	 * "SYNONYM".
	 * <LI><B>REMARKS</B> String => explanatory comment on the table
	 * <LI><B>TYPE_CAT</B> String => the types catalog (may be null)
	 * <LI><B>TYPE_SCHEM</B> String => the types schema (may be null)
	 * <LI><B>TYPE_NAME</B> String => type name (may be null)
	 * <LI><B>SELF_REFERENCING_COL_NAME</B> String => name of the designated
	 * "identifier" column of a typed table (may be null)
	 * <LI><B>REF_GENERATION</B> String => specifies how values in
	 * SELF_REFERENCING_COL_NAME are created. Values are "SYSTEM", "USER",
	 * "DERIVED". (may be null)
	 * </OL>
	 * 
	 * <P>
	 * <B>Note:</B> Some databases may not return information for all tables.
	 * 
	 * @param catalog
	 *            a catalog name; "" retrieves those without a catalog; null
	 *            means drop catalog name from the selection criteria
	 * @param schemaPattern
	 *            a schema name pattern; "" retrieves those without a schema
	 * @param tableNamePattern
	 *            a table name pattern
	 * @param types
	 *            a list of table types to include; null returns all types
	 * @return ResultSet - each row is a table description
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @see #getSearchStringEscape
	 **/

	public java.sql.ResultSet getTables(String catalog, String schemaPattern,
			String tableNamePattern, String types[]) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getTables].methodEntry();
		try {
			SQLMXResultSet resultSet;
			StringBuffer tableType;
			String tableTypeList;
			String catalogNm;
			String schemaNm;
			String tableNm;

			int i;

			clearWarnings();
			/*
			 * RFE: Connection synchronization Connection object is now
			 * synchronized.
			 */
			synchronized (connection_) {
				if (connection_.isClosed_)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_connection", null);

				if (catalog == null)
					catalogNm = connection_.catalog_;
				else
					catalogNm = catalog;

				if (schemaPattern == null)
					schemaNm = connection_.schema_;
				else
					schemaNm = schemaPattern;

				if (tableNamePattern == null)
					tableNm = "%";
				else
					tableNm = tableNamePattern;

				if (types != null) {
					if (types.length != 0) {
						tableType = new StringBuffer(types.length * 10);
						for (i = 0; i < types.length; i++) {
							tableType.append(types[i]);
							tableType.append(',');
						}
						tableTypeList = tableType.toString();
					} else
						tableTypeList = null;
				} else
					tableTypeList = null;
				resultSet = getTables(connection_.server_,
						connection_.getDialogueId(), connection_.getTxid(),
						connection_.autoCommit_, connection_.transactionMode_,
						catalogNm, schemaNm, tableNm, tableTypeList);
			}// End sync
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getTables].methodExit();
		}
	}

	/**
	 * Get the schema names available in this database. The results are ordered
	 * by schema name.
	 * 
	 * <P>
	 * The schema column is:
	 * <OL>
	 * <LI><B>TABLE_SCHEM</B> String => schema name
	 * <LI><B>TABLE_CATALOG</B> String => catalog name (may be null)
	 * </OL>
	 * 
	 * @return ResultSet - each row has a single String column that is a schema
	 *         name
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public java.sql.ResultSet getSchemas() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getSchemas].methodEntry();
		try {
			SQLMXResultSet resultSet;

			clearWarnings();
			/*
			 * RFE: Connection synchronization Connection object is now
			 * synchronized.
			 */
			synchronized (connection_) {
				if (connection_.isClosed_)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_connection", null);
				resultSet = getSchemas(connection_.server_,
						connection_.getDialogueId(), connection_.getTxid(),
						connection_.autoCommit_, connection_.transactionMode_,
						"%");
			}// End sync
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getSchemas].methodExit();
		}
	}

	/**
	 * Get the catalog names available in this database. The results are ordered
	 * by catalog name.
	 * 
	 * <P>
	 * The catalog column is:
	 * <OL>
	 * <LI><B>TABLE_CAT</B> String => catalog name
	 * </OL>
	 * 
	 * @return ResultSet - each row has a single String column that is a catalog
	 *         name
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public java.sql.ResultSet getCatalogs() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getCatalogs].methodEntry();
		try {
			SQLMXResultSet resultSet;

			clearWarnings();
			/*
			 * RFE: Connection synchronization Connection object is now
			 * synchronized.
			 */
			synchronized (connection_) {
				if (connection_.isClosed_)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_connection", null);
				resultSet = getCatalogs(connection_.server_,
						connection_.getDialogueId(), connection_.getTxid(),
						connection_.autoCommit_, connection_.transactionMode_,
						"%");
			}// End sync
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getCatalogs].methodExit();
		}
	}

	/**
	 * Get the table types available in this database. The results are ordered
	 * by table type.
	 * 
	 * <P>
	 * The table type is:
	 * <OL>
	 * <LI><B>TABLE_TYPE</B> String => table type. Typical types are "TABLE",
	 * "VIEW", "SYSTEM TABLE", "GLOBAL TEMPORARY", "LOCAL TEMPORARY", "ALIAS",
	 * "SYNONYM".
	 * </OL>
	 * 
	 * @return ResultSet - each row has a single String column that is a table
	 *         type
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public java.sql.ResultSet getTableTypes() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getTableTypes].methodEntry();
		try {
			SQLMXResultSet resultSet;
			SQLMXDesc[] outputDesc;
			DataWrapper[] rows;

			clearWarnings();
			if (connection_.isClosed_)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_connection", null);

			outputDesc = new SQLMXDesc[1];
			outputDesc[0] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "TABLE_TYPE",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);

			resultSet = new SQLMXResultSet(this, outputDesc, connection_
					.getTxid(), 0);
			rows = new DataWrapper[3];

			// Populate the rows
			rows[0] = new DataWrapper(1);
			rows[0].setString(1, new String("SYSTEM TABLE"));

			rows[1] = new DataWrapper(1);
			rows[1].setString(1, new String("TABLE"));

			rows[2] = new DataWrapper(1);
			rows[2].setString(1, new String("VIEW"));

			resultSet.setFetchOutputs(rows, 3, true, connection_.getTxid());
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getTableTypes].methodExit();
		}
	}

	/**
	 * Get a description of table columns available in a catalog.
	 * 
	 * <P>
	 * Only column descriptions matching the catalog, schema, table and column
	 * name criteria are returned. They are ordered by TABLE_SCHEM, TABLE_NAME
	 * and ORDINAL_POSITION.
	 * 
	 * <P>
	 * Each column description has the following columns:
	 * <OL>
	 * <LI><B>TABLE_CAT</B> String => table catalog (may be null)
	 * <LI><B>TABLE_SCHEM</B> String => table schema (may be null)
	 * <LI><B>TABLE_NAME</B> String => table name
	 * <LI><B>COLUMN_NAME</B> String => column name
	 * <LI><B>DATA_TYPE</B> short => SQL type from java.sql.Types
	 * <LI><B>TYPE_NAME</B> String => Data source dependent type name
	 * <LI><B>COLUMN_SIZE</B> int => column size. For char or date types this is
	 * the maximum number of characters, for numeric or decimal types this is
	 * precision.
	 * <LI><B>BUFFER_LENGTH</B> is not used.
	 * <LI><B>DECIMAL_DIGITS</B> int => the number of fractional digits
	 * <LI><B>NUM_PREC_RADIX</B> int => Radix (typically either 10 or 2)
	 * <LI><B>NULLABLE</B> int => is NULL allowed?
	 * <UL>
	 * <LI>columnNoNulls - might not allow NULL values
	 * <LI>columnNullable - definitely allows NULL values
	 * <LI>columnNullableUnknown - nullability unknown
	 * </UL>
	 * <LI><B>REMARKS</B> String => comment describing column (may be null)
	 * <LI><B>COLUMN_DEF</B> String => default value (may be null)
	 * <LI><B>SQL_DATA_TYPE</B> int => unused
	 * <LI><B>SQL_DATETIME_SUB</B> int => unused
	 * <LI><B>CHAR_OCTET_LENGTH</B> int => for char types the maximum number of
	 * bytes in the column
	 * <LI><B>ORDINAL_POSITION</B> int => index of column in table (starting at
	 * 1)
	 * <LI><B>IS_NULLABLE</B> String => "NO" means column definitely does not
	 * allow NULL values; "YES" means the column might allow NULL values. An
	 * empty string means nobody knows.
	 * <LI><B>SCOPE_CATLOG</B> String => catalog of table that is the scope of a
	 * reference attribute (null if DATA_TYPE isn't REF)
	 * <LI><B>SCOPE_SCHEMA</B> String => schema of table that is the scope of a
	 * reference attribute (null if the DATA_TYPE isn't REF)
	 * <LI><B>SCOPE_TABLE</B> String => table name that this the scope of a
	 * reference attribure (null if the DATA_TYPE isn't REF)
	 * <LI><B>SOURCE_DATA_TYPE</B> short => source type of a distinct type or
	 * user-generated Ref type, SQL type from java.sql.Types (null if DATA_TYPE
	 * isn't DISTINCT or user-generated REF)
	 * </OL>
	 * 
	 * @param catalog
	 *            a catalog name; "" retrieves those without a catalog; null
	 *            means drop catalog name from the selection criteria
	 * @param schemaPattern
	 *            a schema name pattern; "" retrieves those without a schema
	 * @param tableNamePattern
	 *            a table name pattern
	 * @param columnNamePattern
	 *            a column name pattern
	 * @return ResultSet - each row is a column description
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @see #getSearchStringEscape
	 **/
	public java.sql.ResultSet getColumns(String catalog, String schemaPattern,
			String tableNamePattern, String columnNamePattern)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getColumns].methodEntry();
		try {
			SQLMXResultSet resultSet;
			String catalogNm;
			String schemaNm;
			String tableNm;
			String columnNm;

			clearWarnings();
			/*
			 * RFE: Connection synchronization Connection object is now
			 * synchronized.
			 */
			synchronized (connection_) {
				if (connection_.isClosed_)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_connection", null);
				if (catalog == null)
					catalogNm = connection_.getCatalog();
				else
					catalogNm = catalog;

				if (schemaPattern == null)
					schemaNm = connection_.getSchema();
				else
					schemaNm = schemaPattern;
				if (tableNamePattern == null)
					tableNm = "%";
				else
					tableNm = tableNamePattern;
				if (columnNamePattern == null)
					columnNm = "%";
				else
					columnNm = columnNamePattern;

				resultSet = getColumns(connection_.server_,
						connection_.getDialogueId(), connection_.getTxid(),
						connection_.autoCommit_, connection_.transactionMode_,
						catalogNm, schemaNm, tableNm, columnNm);
			}// End sync
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getColumns].methodExit();
		}
	}

	/**
	 * Get a description of the access rights for a table's columns.
	 * 
	 * <P>
	 * Only privileges matching the column name criteria are returned. They are
	 * ordered by COLUMN_NAME and PRIVILEGE.
	 * 
	 * <P>
	 * Each privilige description has the following columns:
	 * <OL>
	 * <LI><B>TABLE_CAT</B> String => table catalog (may be null)
	 * <LI><B>TABLE_SCHEM</B> String => table schema (may be null)
	 * <LI><B>TABLE_NAME</B> String => table name
	 * <LI><B>COLUMN_NAME</B> String => column name
	 * <LI><B>GRANTOR</B> => grantor of access (may be null)
	 * <LI><B>GRANTEE</B> String => grantee of access
	 * <LI><B>PRIVILEGE</B> String => name of access (SELECT, INSERT, UPDATE,
	 * REFRENCES, ...)
	 * <LI><B>IS_GRANTABLE</B> String => "YES" if grantee is permitted to grant
	 * to others; "NO" if not; null if unknown
	 * </OL>
	 * 
	 * @param catalog
	 *            a catalog name; "" retrieves those without a catalog; null
	 *            means drop catalog name from the selection criteria
	 * @param schema
	 *            a schema name; "" retrieves those without a schema
	 * @param table
	 *            a table name
	 * @param columnNamePattern
	 *            a column name pattern
	 * @return ResultSet - each row is a column privilege description
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @see #getSearchStringEscape
	 **/
	public java.sql.ResultSet getColumnPrivileges(String catalog,
			String schema, String table, String columnNamePattern)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getColumnPrivileges].methodEntry();
		try {
			SQLMXResultSet resultSet;
			String catalogNm;
			String schemaNm;

			int i;

			clearWarnings();
			/*
			 * RFE: Connection synchronization Connection object is now
			 * synchronized.
			 */
			synchronized (connection_) {
				if (connection_.isClosed_)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_connection", null);
				if ((table == null) || (columnNamePattern == null)) {
					throw Messages.createSQLException(connection_.locale_,
							"invalid_use_of_null", null);
				}

				if (catalog == null)
					catalogNm = connection_.catalog_;
				else
					catalogNm = catalog;

				if (schema == null)
					schemaNm = connection_.schema_;
				else
					schemaNm = schema;

				resultSet = getColumnPrivileges(connection_.server_,
						connection_.getDialogueId(), connection_.getTxid(),
						connection_.autoCommit_, connection_.transactionMode_,
						catalogNm, schemaNm, table, columnNamePattern);
			}// End sync
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getColumnPrivileges].methodExit();
		}
	}

	/**
	 * Get a description of the access rights for each table available in a
	 * catalog. Note that a table privilege applies to one or more columns in
	 * the table. It would be wrong to assume that this priviledge applies to
	 * all columns (this may be true for some systems but is not true for all.)
	 * 
	 * <P>
	 * Only privileges matching the schema and table name criteria are returned.
	 * They are ordered by TABLE_SCHEM, TABLE_NAME, and PRIVILEGE.
	 * 
	 * <P>
	 * Each privilige description has the following columns:
	 * <OL>
	 * <LI><B>TABLE_CAT</B> String => table catalog (may be null)
	 * <LI><B>TABLE_SCHEM</B> String => table schema (may be null)
	 * <LI><B>TABLE_NAME</B> String => table name
	 * <LI><B>GRANTOR</B> => grantor of access (may be null)
	 * <LI><B>GRANTEE</B> String => grantee of access
	 * <LI><B>PRIVILEGE</B> String => name of access (SELECT, INSERT, UPDATE,
	 * REFRENCES, ...)
	 * <LI><B>IS_GRANTABLE</B> String => "YES" if grantee is permitted to grant
	 * to others; "NO" if not; null if unknown
	 * </OL>
	 * 
	 * @param catalog
	 *            a catalog name; "" retrieves those without a catalog; null
	 *            means drop catalog name from the selection criteria
	 * @param schemaPattern
	 *            a schema name pattern; "" retrieves those without a schema
	 * @param tableNamePattern
	 *            a table name pattern
	 * @return ResultSet - each row is a table privilege description
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @see #getSearchStringEscape
	 **/
	public java.sql.ResultSet getTablePrivileges(String catalog, String schema,
			String table) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getTablePrivileges].methodEntry();
		try {
			SQLMXResultSet resultSet;
			String catalogNm;
			String schemaNm;

			clearWarnings();
			/*
			 * RFE: Connection synchronization Connection object is now
			 * synchronized.
			 */
			synchronized (connection_) {
				if (connection_.isClosed_)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_connection", null);
				if (table == null)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_use_of_null", null);

				if (catalog == null)
					catalogNm = connection_.catalog_;
				else
					catalogNm = catalog;

				if (schema == null)
					schemaNm = connection_.schema_;
				else
					schemaNm = schema;

				resultSet = getTablePrivileges(connection_.server_,
						connection_.getDialogueId(), connection_.getTxid(),
						connection_.autoCommit_, connection_.transactionMode_,
						catalogNm, schemaNm, table);
			}// End sync

			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getTablePrivileges].methodExit();
		}
	}

	/**
	 * Get a description of a table's optimal set of columns that uniquely
	 * identifies a row. They are ordered by SCOPE.
	 * 
	 * <P>
	 * Each column description has the following columns:
	 * <OL>
	 * <LI><B>SCOPE</B> short => actual scope of result
	 * <UL>
	 * <LI>bestRowTemporary - very temporary, while using row
	 * <LI>bestRowTransaction - valid for remainder of current transaction
	 * <LI>bestRowSession - valid for remainder of current session
	 * </UL>
	 * <LI><B>COLUMN_NAME</B> String => column name
	 * <LI><B>DATA_TYPE</B> short => SQL data type from java.sql.Types
	 * <LI><B>TYPE_NAME</B> String => Data source dependent type name
	 * <LI><B>COLUMN_SIZE</B> int => precision
	 * <LI><B>BUFFER_LENGTH</B> int => not used
	 * <LI><B>DECIMAL_DIGITS</B> short => scale
	 * <LI><B>PSEUDO_COLUMN</B> short => is this a pseudo column like an Oracle
	 * ROWID
	 * <UL>
	 * <LI>bestRowUnknown - may or may not be pseudo column
	 * <LI>bestRowNotPseudo - is NOT a pseudo column
	 * <LI>bestRowPseudo - is a pseudo column
	 * </UL>
	 * </OL>
	 * 
	 * @param catalog
	 *            a catalog name; "" retrieves those without a catalog; null
	 *            means drop catalog name from the selection criteria
	 * @param schema
	 *            a schema name; "" retrieves those without a schema
	 * @param table
	 *            a table name
	 * @param scope
	 *            the scope of interest; use same values as SCOPE
	 * @param nullable
	 *            include columns that are nullable?
	 * @return ResultSet - each row is a column description
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public java.sql.ResultSet getBestRowIdentifier(String catalog,
			String schema, String table, int scope, boolean nullable)
			throws SQLException

	{
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getBestRowIdentifier].methodEntry();
		try {
			SQLMXResultSet resultSet;
			String catalogNm;
			String schemaNm;

			clearWarnings();
			/*
			 * RFE: Connection synchronization Connection object is now
			 * synchronized.
			 */
			synchronized (connection_) {
				if (connection_.isClosed_)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_connection", null);
				if (table == null)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_use_of_null", null);
				if (catalog == null)
					catalogNm = connection_.catalog_;
				else
					catalogNm = catalog;

				if (schema == null)
					schemaNm = connection_.schema_;
				else
					schemaNm = schema;
				resultSet = getBestRowIdentifier(connection_.server_,
						connection_.getDialogueId(), connection_.getTxid(),
						connection_.autoCommit_, connection_.transactionMode_,
						catalogNm, schemaNm, table, scope, nullable);
			}// End sync
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getBestRowIdentifier].methodExit();
		}
	}

	/**
	 * Get a description of a table's columns that are automatically updated
	 * when any value in a row is updated. They are unordered.
	 * 
	 * <P>
	 * Each column description has the following columns:
	 * <OL>
	 * <LI><B>SCOPE</B> short => is not used
	 * <LI><B>COLUMN_NAME</B> String => column name
	 * <LI><B>DATA_TYPE</B> short => SQL data type from java.sql.Types
	 * <LI><B>TYPE_NAME</B> String => Data source dependent type name
	 * <LI><B>COLUMN_SIZE</B> int => precision
	 * <LI><B>BUFFER_LENGTH</B> int => length of column value in bytes
	 * <LI><B>DECIMAL_DIGITS</B> short => scale
	 * <LI><B>PSEUDO_COLUMN</B> short => is this a pseudo column like an Oracle
	 * ROWID
	 * <UL>
	 * <LI>versionColumnUnknown - may or may not be pseudo column
	 * <LI>versionColumnNotPseudo - is NOT a pseudo column
	 * <LI>versionColumnPseudo - is a pseudo column
	 * </UL>
	 * </OL>
	 * 
	 * @param catalog
	 *            a catalog name; "" retrieves those without a catalog; null
	 *            means drop catalog name from the selection criteria
	 * @param schema
	 *            a schema name; "" retrieves those without a schema
	 * @param table
	 *            a table name
	 * @return ResultSet - each row is a column description
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public java.sql.ResultSet getVersionColumns(String catalog, String schema,
			String table) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getVersionColumns].methodEntry();
		try {
			SQLMXResultSet resultSet;
			String catalogNm;
			String schemaNm;

			clearWarnings();
			/*
			 * RFE: Connection synchronization Connection object is now
			 * synchronized.
			 */
			synchronized (connection_) {
				if (connection_.isClosed_)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_connection", null);
				if (table == null)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_use_of_null", null);

				if (catalog == null)
					catalogNm = connection_.catalog_;
				else
					catalogNm = catalog;

				if (schema == null)
					schemaNm = connection_.schema_;
				else
					schemaNm = schema;
				resultSet = getVersionColumns(connection_.server_,
						connection_.getDialogueId(), connection_.getTxid(),
						connection_.autoCommit_, connection_.transactionMode_,
						catalogNm, schemaNm, table);
			}// End sync
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getVersionColumns].methodExit();
		}
	}

	/**
	 * Get a description of a table's primary key columns. They are ordered by
	 * COLUMN_NAME.
	 * 
	 * <P>
	 * Each primary key column description has the following columns:
	 * <OL>
	 * <LI><B>TABLE_CAT</B> String => table catalog (may be null)
	 * <LI><B>TABLE_SCHEM</B> String => table schema (may be null)
	 * <LI><B>TABLE_NAME</B> String => table name
	 * <LI><B>COLUMN_NAME</B> String => column name
	 * <LI><B>KEY_SEQ</B> short => sequence number within primary key
	 * <LI><B>PK_NAME</B> String => primary key name (may be null)
	 * </OL>
	 * 
	 * @param catalog
	 *            a catalog name; "" retrieves those without a catalog; null
	 *            means drop catalog name from the selection criteria
	 * @param schema
	 *            a schema name pattern; "" retrieves those without a schema
	 * @param table
	 *            a table name
	 * @return ResultSet - each row is a primary key column description
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public java.sql.ResultSet getPrimaryKeys(String catalog, String schema,
			String table) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getPrimaryKeys].methodEntry();
		try {
			SQLMXResultSet resultSet;
			String catalogNm;
			String schemaNm;

			clearWarnings();
			/*
			 * RFE: Connection synchronization Connection object is now
			 * synchronized.
			 */
			synchronized (connection_) {
				if (connection_.isClosed_)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_connection", null);
				if (table == null)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_use_of_null", null);

				if (catalog == null)
					catalogNm = connection_.catalog_;
				else
					catalogNm = catalog;
				if (schema == null)
					schemaNm = connection_.schema_;
				else
					schemaNm = schema;
				resultSet = getPrimaryKeys(connection_.server_,
						connection_.getDialogueId(), connection_.getTxid(),
						connection_.autoCommit_, connection_.transactionMode_,
						catalogNm, schemaNm, table);
			}// End sync
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getPrimaryKeys].methodExit();
		}
	}

	/**
	 * Get a description of the primary key columns that are referenced by a
	 * table's foreign key columns (the primary keys imported by a table). They
	 * are ordered by PKTABLE_CAT, PKTABLE_SCHEM, PKTABLE_NAME, and KEY_SEQ.
	 * 
	 * <P>
	 * Each primary key column description has the following columns:
	 * <OL>
	 * <LI><B>PKTABLE_CAT</B> String => primary key table catalog being imported
	 * (may be null)
	 * <LI><B>PKTABLE_SCHEM</B> String => primary key table schema being
	 * imported (may be null)
	 * <LI><B>PKTABLE_NAME</B> String => primary key table name being imported
	 * <LI><B>PKCOLUMN_NAME</B> String => primary key column name being imported
	 * <LI><B>FKTABLE_CAT</B> String => foreign key table catalog (may be null)
	 * <LI><B>FKTABLE_SCHEM</B> String => foreign key table schema (may be null)
	 * <LI><B>FKTABLE_NAME</B> String => foreign key table name
	 * <LI><B>FKCOLUMN_NAME</B> String => foreign key column name
	 * <LI><B>KEY_SEQ</B> short => sequence number within foreign key
	 * <LI><B>UPDATE_RULE</B> short => What happens to foreign key when primary
	 * is updated:
	 * <UL>
	 * <LI>importedNoAction - do not allow update of primary key if it has been
	 * imported
	 * <LI>importedKeyCascade - change imported key to agree with primary key
	 * update
	 * <LI>importedKeySetNull - change imported key to NULL if its primary key
	 * has been updated
	 * <LI>importedKeySetDefault - change imported key to default values if its
	 * primary key has been updated
	 * <LI>importedKeyRestrict - same as importedKeyNoAction (for ODBC 2.x
	 * compatibility)
	 * </UL>
	 * <LI><B>DELETE_RULE</B> short => What happens to the foreign key when
	 * primary is deleted.
	 * <UL>
	 * <LI>importedKeyNoAction - do not allow delete of primary key if it has
	 * been imported
	 * <LI>importedKeyCascade - delete rows that import a deleted key
	 * <LI>importedKeySetNull - change imported key to NULL if its primary key
	 * has been deleted
	 * <LI>importedKeyRestrict - same as importedKeyNoAction (for ODBC 2.x
	 * compatibility)
	 * <LI>importedKeySetDefault - change imported key to default if its primary
	 * key has been deleted
	 * </UL>
	 * <LI><B>FK_NAME</B> String => foreign key name (may be null)
	 * <LI><B>PK_NAME</B> String => primary key name (may be null)
	 * <LI><B>DEFERRABILITY</B> short => can the evaluation of foreign key
	 * constraints be deferred until commit
	 * <UL>
	 * <LI>importedKeyInitiallyDeferred - see SQL92 for definition
	 * <LI>importedKeyInitiallyImmediate - see SQL92 for definition
	 * <LI>importedKeyNotDeferrable - see SQL92 for definition
	 * </UL>
	 * </OL>
	 * 
	 * @param catalog
	 *            a catalog name; "" retrieves those without a catalog; null
	 *            means drop catalog name from the selection criteria
	 * @param schema
	 *            a schema name pattern; "" retrieves those without a schema
	 * @param table
	 *            a table name
	 * @return ResultSet - each row is a primary key column description
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @see #getExportedKeys
	 **/
	public java.sql.ResultSet getImportedKeys(String catalog, String schema,
			String table) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getImportedKeys].methodEntry();
		try {
			SQLMXResultSet resultSet;
			String catalogNm;
			String schemaNm;

			clearWarnings();
			/*
			 * RFE: Connection synchronization Connection object is now
			 * synchronized.
			 */
			synchronized (connection_) {
				if (connection_.isClosed_)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_connection", null);
				if (table == null)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_use_of_null", null);

				if (catalog == null)
					catalogNm = connection_.catalog_;
				else
					catalogNm = catalog;

				if (schema == null)
					schemaNm = connection_.schema_;
				else
					schemaNm = schema;

				resultSet = getImportedKeys(connection_.server_,
						connection_.getDialogueId(), connection_.getTxid(),
						connection_.autoCommit_, connection_.transactionMode_,
						catalogNm, schemaNm, table);
			}// End sync

			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getImportedKeys].methodExit();
		}
	}

	/**
	 * Get a description of the foreign key columns that reference a table's
	 * primary key columns (the foreign keys exported by a table). They are
	 * ordered by FKTABLE_CAT, FKTABLE_SCHEM, FKTABLE_NAME, and KEY_SEQ.
	 * 
	 * <P>
	 * Each foreign key column description has the following columns:
	 * <OL>
	 * <LI><B>PKTABLE_CAT</B> String => primary key table catalog (may be null)
	 * <LI><B>PKTABLE_SCHEM</B> String => primary key table schema (may be null)
	 * <LI><B>PKTABLE_NAME</B> String => primary key table name
	 * <LI><B>PKCOLUMN_NAME</B> String => primary key column name
	 * <LI><B>FKTABLE_CAT</B> String => foreign key table catalog (may be null)
	 * being exported (may be null)
	 * <LI><B>FKTABLE_SCHEM</B> String => foreign key table schema (may be null)
	 * being exported (may be null)
	 * <LI><B>FKTABLE_NAME</B> String => foreign key table name being exported
	 * <LI><B>FKCOLUMN_NAME</B> String => foreign key column name being exported
	 * <LI><B>KEY_SEQ</B> short => sequence number within foreign key
	 * <LI><B>UPDATE_RULE</B> short => What happens to foreign key when primary
	 * is updated:
	 * <UL>
	 * <LI>importedNoAction - do not allow update of primary key if it has been
	 * imported
	 * <LI>importedKeyCascade - change imported key to agree with primary key
	 * update
	 * <LI>importedKeySetNull - change imported key to NULL if its primary key
	 * has been updated
	 * <LI>importedKeySetDefault - change imported key to default values if its
	 * primary key has been updated
	 * <LI>importedKeyRestrict - same as importedKeyNoAction (for ODBC 2.x
	 * compatibility)
	 * </UL>
	 * <LI><B>DELETE_RULE</B> short => What happens to the foreign key when
	 * primary is deleted.
	 * <UL>
	 * <LI>importedKeyNoAction - do not allow delete of primary key if it has
	 * been imported
	 * <LI>importedKeyCascade - delete rows that import a deleted key
	 * <LI>importedKeySetNull - change imported key to NULL if its primary key
	 * has been deleted
	 * <LI>importedKeyRestrict - same as importedKeyNoAction (for ODBC 2.x
	 * compatibility)
	 * <LI>importedKeySetDefault - change imported key to default if its primary
	 * key has been deleted
	 * </UL>
	 * <LI><B>FK_NAME</B> String => foreign key name (may be null)
	 * <LI><B>PK_NAME</B> String => primary key name (may be null)
	 * <LI><B>DEFERRABILITY</B> short => can the evaluation of foreign key
	 * constraints be deferred until commit
	 * <UL>
	 * <LI>importedKeyInitiallyDeferred - see SQL92 for definition
	 * <LI>importedKeyInitiallyImmediate - see SQL92 for definition
	 * <LI>importedKeyNotDeferrable - see SQL92 for definition
	 * </UL>
	 * </OL>
	 * 
	 * @param catalog
	 *            a catalog name; "" retrieves those without a catalog; null
	 *            means drop catalog name from the selection criteria
	 * @param schema
	 *            a schema name pattern; "" retrieves those without a schema
	 * @param table
	 *            a table name
	 * @return ResultSet - each row is a foreign key column description
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @see #getImportedKeys
	 **/
	public java.sql.ResultSet getExportedKeys(String catalog, String schema,
			String table) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getExportedKeys].methodEntry();
		try {
			SQLMXResultSet resultSet;
			String catalogNm;
			String schemaNm;

			clearWarnings();
			/*
			 * RFE: Connection synchronization Connection object is now
			 * synchronized.
			 */
			synchronized (connection_) {
				if (connection_.isClosed_)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_connection", null);
				if (table == null)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_use_of_null", null);

				if (catalog == null)
					catalogNm = connection_.catalog_;
				else
					catalogNm = catalog;

				if (schema == null)
					schemaNm = connection_.schema_;
				else
					schemaNm = schema;

				resultSet = getExportedKeys(connection_.server_,
						connection_.getDialogueId(), connection_.getTxid(),
						connection_.autoCommit_, connection_.transactionMode_,
						catalogNm, schemaNm, table);
			}// End sync

			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getExportedKeys].methodExit();
		}
	}

	/**
	 * Retrieves a description of the foreign key columns in the given foreign
	 * key table that reference the primary key columns of the given primary key
	 * table (describe how one table imports another's key). This should
	 * normally return a single foreign key/primary key pair because most tables
	 * import a foreign key from a table only once. They are ordered by
	 * FKTABLE_CAT, FKTABLE_SCHEM, FKTABLE_NAME, and KEY_SEQ.
	 * 
	 * <P>
	 * Each foreign key column description has the following columns:
	 * <OL>
	 * <LI><B>PKTABLE_CAT</B> String => primary key table catalog (may be null)
	 * <LI><B>PKTABLE_SCHEM</B> String => primary key table schema (may be null)
	 * <LI><B>PKTABLE_NAME</B> String => primary key table name
	 * <LI><B>PKCOLUMN_NAME</B> String => primary key column name
	 * <LI><B>FKTABLE_CAT</B> String => foreign key table catalog (may be null)
	 * being exported (may be null)
	 * <LI><B>FKTABLE_SCHEM</B> String => foreign key table schema (may be null)
	 * being exported (may be null)
	 * <LI><B>FKTABLE_NAME</B> String => foreign key table name being exported
	 * <LI><B>FKCOLUMN_NAME</B> String => foreign key column name being exported
	 * <LI><B>KEY_SEQ</B> short => sequence number within foreign key
	 * <LI><B>UPDATE_RULE</B> short => What happens to foreign key when primary
	 * is updated:
	 * <UL>
	 * <LI>importedNoAction - do not allow update of primary key if it has been
	 * imported
	 * <LI>importedKeyCascade - change imported key to agree with primary key
	 * update
	 * <LI>importedKeySetNull - change imported key to NULL if its primary key
	 * has been updated
	 * <LI>importedKeySetDefault - change imported key to default values if its
	 * primary key has been updated
	 * <LI>importedKeyRestrict - same as importedKeyNoAction (for ODBC 2.x
	 * compatibility)
	 * </UL>
	 * <LI><B>DELETE_RULE</B> short => What happens to the foreign key when
	 * primary is deleted.
	 * <UL>
	 * <LI>importedKeyNoAction - do not allow delete of primary key if it has
	 * been imported
	 * <LI>importedKeyCascade - delete rows that import a deleted key
	 * <LI>importedKeySetNull - change imported key to NULL if its primary key
	 * has been deleted
	 * <LI>importedKeyRestrict - same as importedKeyNoAction (for ODBC 2.x
	 * compatibility)
	 * <LI>importedKeySetDefault - change imported key to default if its primary
	 * key has been deleted
	 * </UL>
	 * <LI><B>FK_NAME</B> String => foreign key name (may be null)
	 * <LI><B>PK_NAME</B> String => primary key name (may be null)
	 * <LI><B>DEFERRABILITY</B> short => can the evaluation of foreign key
	 * constraints be deferred until commit
	 * <UL>
	 * <LI>importedKeyInitiallyDeferred - see SQL92 for definition
	 * <LI>importedKeyInitiallyImmediate - see SQL92 for definition
	 * <LI>importedKeyNotDeferrable - see SQL92 for definition
	 * </UL>
	 * </OL>
	 * 
	 * @param primaryCatalog
	 *            a catalog name; "" retrieves those without a catalog; null
	 *            means drop catalog name from the selection criteria
	 * @param primarySchema
	 *            a schema name pattern; "" retrieves those without a schema
	 * @param primaryTable
	 *            the table name that exports the key
	 * @param foreignCatalog
	 *            a catalog name; "" retrieves those without a catalog; null
	 *            means drop catalog name from the selection criteria
	 * @param foreignSchema
	 *            a schema name pattern; "" retrieves those without a schema
	 * @param foreignTable
	 *            the table name that imports the key
	 * @return ResultSet - each row is a foreign key column description
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @see #getImportedKeys
	 **/
	public java.sql.ResultSet getCrossReference(String primaryCatalog,
			String primarySchema, String primaryTable, String foreignCatalog,
			String foreignSchema, String foreignTable) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getCrossReference].methodEntry();
		try {
			SQLMXResultSet resultSet;
			String primaryCatalogNm;
			String primarySchemaNm;
			String foreignCatalogNm;
			String foreignSchemaNm;

			clearWarnings();
			/*
			 * RFE: Connection synchronization Connection object is now
			 * synchronized.
			 */
			synchronized (connection_) {
				if (connection_.isClosed_)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_connection", null);
				if ((primaryTable == null) || (foreignTable == null))
					throw Messages.createSQLException(connection_.locale_,
							"invalid_use_of_null", null);

				if (primaryCatalog == null)
					primaryCatalogNm = connection_.catalog_;
				else
					primaryCatalogNm = primaryCatalog;

				if (primarySchema == null)
					primarySchemaNm = connection_.schema_;
				else
					primarySchemaNm = primarySchema;

				if (foreignCatalog == null)
					foreignCatalogNm = connection_.catalog_;
				else
					foreignCatalogNm = foreignCatalog;

				if (foreignSchema == null)
					foreignSchemaNm = connection_.schema_;
				else
					foreignSchemaNm = foreignSchema;

				resultSet = getCrossReference(connection_.server_,
						connection_.getDialogueId(), connection_.getTxid(),
						connection_.autoCommit_, connection_.transactionMode_,
						primaryCatalogNm, primarySchemaNm, primaryTable,
						foreignCatalogNm, foreignSchemaNm, foreignTable);
			}// End sync

			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getCrossReference].methodExit();
		}
	}

	/**
	 * Get a description of all the standard SQL types supported by this
	 * database. They are ordered by DATA_TYPE and then by how closely the data
	 * type maps to the corresponding JDBC SQL type.
	 * 
	 * <P>
	 * Each type description has the following columns:
	 * <OL>
	 * <LI><B>TYPE_NAME</B> String => Type name
	 * <LI><B>DATA_TYPE</B> short => SQL data type from java.sql.Types
	 * <LI><B>PRECISION</B> int => maximum precision
	 * <LI><B>LITERAL_PREFIX</B> String => prefix used to quote a literal (may
	 * be null)
	 * <LI><B>LITERAL_SUFFIX</B> String => suffix used to quote a literal (may
	 * be null)
	 * <LI><B>CREATE_PARAMS</B> String => parameters used in creating the type
	 * (may be null)
	 * <LI><B>NULLABLE</B> short => can you use NULL for this type?
	 * <UL>
	 * <LI>typeNoNulls - does not allow NULL values
	 * <LI>typeNullable - allows NULL values
	 * <LI>typeNullableUnknown - nullability unknown
	 * </UL>
	 * <LI><B>CASE_SENSITIVE</B> boolean=> is it case sensitive?
	 * <LI><B>SEARCHABLE</B> short => can you use "WHERE" based on this type:
	 * <UL>
	 * <LI>typePredNone - No support
	 * <LI>typePredChar - Only supported with WHERE .. LIKE
	 * <LI>typePredBasic - Supported except for WHERE .. LIKE
	 * <LI>typeSearchable - Supported for all WHERE ..
	 * </UL>
	 * <LI><B>UNSIGNED_ATTRIBUTE</B> boolean => is it unsigned?
	 * <LI><B>FIXED_PREC_SCALE</B> boolean => can it be a money value?
	 * <LI><B>AUTO_INCREMENT</B> boolean => can it be used for an auto-increment
	 * value?
	 * <LI><B>LOCAL_TYPE_NAME</B> String => localized version of type name (may
	 * be null)
	 * <LI><B>MINIMUM_SCALE</B> short => minimum scale supported
	 * <LI><B>MAXIMUM_SCALE</B> short => maximum scale supported
	 * <LI><B>SQL_DATA_TYPE</B> int => unused
	 * <LI><B>SQL_DATETIME_SUB</B> int => unused
	 * <LI><B>NUM_PREC_RADIX</B> int => usually 2 or 10
	 * </OL>
	 * 
	 * @return ResultSet - each row is a SQL type description
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public java.sql.ResultSet getTypeInfo() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getTypeInfo].methodEntry();
		try {
			SQLMXResultSet resultSet;

			clearWarnings();
			/*
			 * RFE: Connection synchronization Connection object is now
			 * synchronized.
			 */
			synchronized (connection_) {
				if (connection_.isClosed_)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_connection", null);
				resultSet = getTypeInfo(connection_.server_,
						connection_.getDialogueId(), connection_.getTxid(),
						connection_.autoCommit_, connection_.transactionMode_);
			}// End sync
			// Patch the column names as per JDBC specification
			resultSet.setColumnName(3, "PRECISION");
			resultSet.setColumnName(12, "AUTO_INCREMENT");
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getTypeInfo].methodExit();
		}
	}

	/**
	 * Get a description of a table's indices and statistics. They are ordered
	 * by NON_UNIQUE, TYPE, INDEX_NAME, and ORDINAL_POSITION.
	 * 
	 * <P>
	 * Each index column description has the following columns:
	 * <OL>
	 * <LI><B>TABLE_CAT</B> String => table catalog (may be null)
	 * <LI><B>TABLE_SCHEM</B> String => table schema (may be null)
	 * <LI><B>TABLE_NAME</B> String => table name
	 * <LI><B>NON_UNIQUE</B> boolean => Can index values be non-unique? false
	 * when TYPE is tableIndexStatistic
	 * <LI><B>INDEX_QUALIFIER</B> String => index catalog (may be null); null
	 * when TYPE is tableIndexStatistic
	 * <LI><B>INDEX_NAME</B> String => index name; null when TYPE is
	 * tableIndexStatistic
	 * <LI><B>TYPE</B> short => index type:
	 * <UL>
	 * <LI>tableIndexStatistic - this identifies table statistics that are
	 * returned in conjuction with a table's index descriptions
	 * <LI>tableIndexClustered - this is a clustered index
	 * <LI>tableIndexHashed - this is a hashed index
	 * <LI>tableIndexOther - this is some other style of index
	 * </UL>
	 * <LI><B>ORDINAL_POSITION</B> short => column sequence number within index;
	 * zero when TYPE is tableIndexStatistic
	 * <LI><B>COLUMN_NAME</B> String => column name; null when TYPE is
	 * tableIndexStatistic
	 * <LI><B>ASC_OR_DESC</B> String => column sort sequence, "A" => ascending,
	 * "D" => descending, may be null if sort sequence is not supported; null
	 * when TYPE is tableIndexStatistic
	 * <LI><B>CARDINALITY</B> int => When TYPE is tableIndexStatistic, then this
	 * is the number of rows in the table; otherwise, it is the number of unique
	 * values in the index.
	 * <LI><B>PAGES</B> int => When TYPE is tableIndexStatisic then this is the
	 * number of pages used for the table, otherwise it is the number of pages
	 * used for the current index.
	 * <LI><B>FILTER_CONDITION</B> String => Filter condition, if any. (may be
	 * null)
	 * </OL>
	 * 
	 * @param catalog
	 *            a catalog name; "" retrieves those without a catalog; null
	 *            means drop catalog name from the selection criteria
	 * @param schema
	 *            a schema name pattern; "" retrieves those without a schema
	 * @param table
	 *            a table name
	 * @param unique
	 *            when true, return only indices for unique values; when false,
	 *            return indices regardless of whether unique or not
	 * @param approximate
	 *            when true, result is allowed to reflect approximate or out of
	 *            data values; when false, results are requested to be accurate
	 * @return ResultSet - each row is an index column description
	 * @throws SQLException
	 *             - if a database access error occurs
	 **/
	public java.sql.ResultSet getIndexInfo(String catalog, String schema,
			String table, boolean unique, boolean approximate)
			throws SQLException

	{
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getIndexInfo].methodEntry();
		try {
			SQLMXResultSet resultSet;
			String catalogNm;
			String schemaNm;

			clearWarnings();
			/*
			 * RFE: Connection synchronization Connection object is now
			 * synchronized.
			 */
			synchronized (connection_) {
				if (connection_.isClosed_)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_connection", null);
				if (table == null)
					throw Messages.createSQLException(connection_.locale_,
							"invalid_use_of_null", null);

				if (catalog == null)
					catalogNm = connection_.catalog_;
				else
					catalogNm = catalog;
				if (schema == null)
					schemaNm = connection_.schema_;
				else
					schemaNm = schema;
				resultSet = getIndexInfo(connection_.server_,
						connection_.getDialogueId(), connection_.getTxid(),
						connection_.autoCommit_, connection_.transactionMode_,
						catalogNm, schemaNm, table, unique, approximate);
			}// End sync
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getIndexInfo].methodExit();
		}
	}

	/*
	 * ------------------------------------------------------------ JDBC 2.0
	 * -----------------------------------------------------------
	 */

	/**
	 * Retrieves whether or not a visible row delete can be detected by calling
	 * the method ResultSet.rowDeleted. If the method deletesAreDetected returns
	 * false, it means that deleted rows are removed from the result set.
	 * 
	 * @param type
	 *            - the ResultSet type; one of ResultSet.TYPE_FORWARD_ONLY,
	 *            ResultSet.TYPE_SCROLL_INSENSITIVE, or
	 *            ResultSet.TYPE_SCROLL_SENSITIVE
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.2
	 **/
	public boolean deletesAreDetected(int type) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_deletesAreDetected].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_deletesAreDetected].methodExit();
		}
	}

	/**
	 * Retrieves the connection that produced this metadata object.
	 * 
	 * @return the connection that produced this metadata object
	 * @since 1.2
	 **/
	public java.sql.Connection getConnection() {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getConnection].methodEntry();
		try {
			return (connection_);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getConnection].methodExit();
		}
	}

	/**
	 * <P>
	 * Retrieves a description of the user-defined types (UDTs) defined in a
	 * particular schema. Schema-specific UDTs may have type JAVA_OBJECT,
	 * STRUCT, or DISTINCT. Only types matching the catalog, schema, type name
	 * and type criteria are returned. They are ordered by DATA_TYPE, TYPE_SCHEM
	 * and TYPE_NAME. The type name parameter may be a fully-qualified name. In
	 * this case, the catalog and schemaPattern parameters are ignored.
	 * </P>
	 * 
	 * <P>
	 * Each type description has the following columns:
	 * </P>
	 * <OL>
	 * <LI><B>TYPE_CAT</B> String => the type's catalog (may be null)
	 * <LI><B>TYPE_SCHEM</B> String => type's schema (may be null)
	 * <LI><B>TYPE_NAME</B> String => type name
	 * <LI><B>CLASS_NAME</B> String => Java class name
	 * <LI><B>DATA_TYPE</B> int => type value defined in java.sql.Types. One of
	 * JAVA_OBJECT, STRUCT, or DISTINCT
	 * <LI><B>REMARKS</B> String => explanatory comment on the type
	 * <LI><B>BASE_TYPE</B> short => type code of the source type of a DISTINCT
	 * type or the type that implements the user-generated reference type of the
	 * SELF_REFERENCING_COLUMN of a structured type as defined in java.sql.Types
	 * (null if DATA_TYPE is not DISTINCT or not STRUCT with
	 * REFERENCE_GENERATION = USER_DEFINED)
	 * </OL>
	 * <P>
	 * <B>Note:</B> If the driver does not support UDTs, an empty result set is
	 * returned.
	 * </P>
	 * 
	 * @param catalog
	 *            - a catalog name; must match the catalog name as it is stored
	 *            in the database; "" retrieves those without a catalog; null
	 *            means that the catalog name should not be used to narrow the
	 *            search
	 * @param schemaPattern
	 *            - a schema pattern name; must match the schema name as it is
	 *            stored in the database; "" retrieves those without a schema;
	 *            null means that the schema name should not be used to narrow
	 *            the search
	 * @param typeNamePattern
	 *            - a type name pattern; must match the type name as it is
	 *            stored in the database; may be a fully qualified name
	 * @param types
	 *            - a list of user-defined types (JAVA_OBJECT, STRUCT, or
	 *            DISTINCT) to include; null returns all types
	 * @return ResultSet object in which each row describes a UDT
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.2
	 **/
	public java.sql.ResultSet getUDTs(String catalog, String schemaPattern,
			String typeNamePattern, int[] types) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getUDTs].methodEntry();
		try {
			SQLMXResultSet resultSet;
			SQLMXDesc[] outputDesc;
			DataWrapper[] rows;

			clearWarnings();
			if (connection_.isClosed_)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_connection", null);

			outputDesc = new SQLMXDesc[7];
			outputDesc[0] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "TYPE_CAT",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[1] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "TYPE_SCHEM",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[2] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "TYPE_NAME",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[3] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "CLASS_NAME",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[4] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "DATA_TYPE",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[5] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "REMARKS",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[6] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_SMALLINT,
					(short) 0, 2, (short) 0, (short) 0, false, "BASE_TYPE",
					false, Types.SMALLINT, (short) 0, (short) 0, 0, null, null,
					null, 130, 0, 0, null);

			resultSet = new SQLMXResultSet(this, outputDesc, connection_
					.getTxid(), 0);
			rows = new DataWrapper[0];

			// Populate the rows
			resultSet.setFetchOutputs(rows, 0, true, connection_.getTxid());
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getUDTs].methodExit();
		}
	}

	/**
	 * Retrieves whether or not a visible row insert can be detected by calling
	 * the method ResultSet.rowInserted.
	 * 
	 * @param type
	 *            - the ResultSet type; one of ResultSet.TYPE_FORWARD_ONLY,
	 *            ResultSet.TYPE_SCROLL_INSENSITIVE, or
	 *            ResultSet.TYPE_SCROLL_SENSITIVE
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.2
	 **/
	public boolean insertsAreDetected(int type) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_insertsAreDetected].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_insertsAreDetected].methodExit();
		}
	}

	/**
	 * Retrieves whether or not a visible row update can be detected by calling
	 * the method ResultSet.rowUpdated.
	 * 
	 * @param type
	 *            - the ResultSet type; one of ResultSet.TYPE_FORWARD_ONLY,
	 *            ResultSet.TYPE_SCROLL_INSENSITIVE, or
	 *            ResultSet.TYPE_SCROLL_SENSITIVE
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.2
	 **/
	public boolean updatesAreDetected(int type) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_updatesAreDetected].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_updatesAreDetected].methodExit();
		}
	}

	/**
	 * Retrieves whether deletes made by others are visible.
	 * 
	 * @param type
	 *            - the ResultSet type; one of ResultSet.TYPE_FORWARD_ONLY,
	 *            ResultSet.TYPE_SCROLL_INSENSITIVE, or
	 *            ResultSet.TYPE_SCROLL_SENSITIVE
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.2
	 **/
	public boolean othersDeletesAreVisible(int type) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_othersDeletesAreVisible].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_othersDeletesAreVisible].methodExit();
		}
	}

	/**
	 * Retrieves whether inserts made by others are visible.
	 * 
	 * @param type
	 *            - the ResultSet type; one of ResultSet.TYPE_FORWARD_ONLY,
	 *            ResultSet.TYPE_SCROLL_INSENSITIVE, or
	 *            ResultSet.TYPE_SCROLL_SENSITIVE
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.2
	 **/
	public boolean othersInsertsAreVisible(int type) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_othersInsertsAreVisible].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_othersInsertsAreVisible].methodExit();
		}
	}

	/**
	 * Retrieves whether updates made by others are visible.
	 * 
	 * @param type
	 *            - the ResultSet type; one of ResultSet.TYPE_FORWARD_ONLY,
	 *            ResultSet.TYPE_SCROLL_INSENSITIVE, or
	 *            ResultSet.TYPE_SCROLL_SENSITIVE
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.2
	 **/
	public boolean othersUpdatesAreVisible(int type) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_othersUpdatesAreVisible].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_othersUpdatesAreVisible].methodExit();
		}
	}

	/**
	 * Retrieves whether a result set's own deletes are visible.
	 * 
	 * @param type
	 *            - the ResultSet type; one of ResultSet.TYPE_FORWARD_ONLY,
	 *            ResultSet.TYPE_SCROLL_INSENSITIVE, or
	 *            ResultSet.TYPE_SCROLL_SENSITIVE
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.2
	 **/
	public boolean ownDeletesAreVisible(int type) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_ownDeletesAreVisible].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_ownDeletesAreVisible].methodExit();
		}
	}

	/**
	 * Retrieves whether a result set's own inserts are visible.
	 * 
	 * @param type
	 *            - the ResultSet type; one of ResultSet.TYPE_FORWARD_ONLY,
	 *            ResultSet.TYPE_SCROLL_INSENSITIVE, or
	 *            ResultSet.TYPE_SCROLL_SENSITIVE
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.2
	 **/
	public boolean ownInsertsAreVisible(int type) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_ownInsertsAreVisible].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_ownInsertsAreVisible].methodExit();
		}
	}

	/**
	 * Retrieves whether for the given type of ResultSet object, the result
	 * set's own updates are visible.
	 * 
	 * @param type
	 *            - the ResultSet type; one of ResultSet.TYPE_FORWARD_ONLY,
	 *            ResultSet.TYPE_SCROLL_INSENSITIVE, or
	 *            ResultSet.TYPE_SCROLL_SENSITIVE
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.2
	 **/
	public boolean ownUpdatesAreVisible(int type) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_ownUpdatesAreVisible].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_ownUpdatesAreVisible].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports batch updates.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.2
	 **/
	public boolean supportsBatchUpdates() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsBatchUpdates].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsBatchUpdates].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports the given result set type.
	 * 
	 * @param type
	 *            - the ResultSet type; one of ResultSet.TYPE_FORWARD_ONLY,
	 *            ResultSet.TYPE_SCROLL_INSENSITIVE, or
	 *            ResultSet.TYPE_SCROLL_SENSITIVE
	 * @return true for TYPE_FORWARD_ONLY and TYPE_SCROLL_INSENSITIVE, otherwise
	 *         false.
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.2
	 **/
	public boolean supportsResultSetType(int type) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsResultSetType].methodEntry();
		try {
			switch (type) {
			case ResultSet.TYPE_FORWARD_ONLY:
			case ResultSet.TYPE_SCROLL_INSENSITIVE:
				return true;
			}
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsResultSetType].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports the given concurrency type in
	 * combination with the given result set type.
	 * 
	 * @param type
	 *            - the ResultSet type; one of ResultSet.TYPE_FORWARD_ONLY,
	 *            ResultSet.TYPE_SCROLL_INSENSITIVE, or
	 *            ResultSet.TYPE_SCROLL_SENSITIVE
	 * @return true for TYPE_FORWARD_ONLY and TYPE_SCROLL_INSENSITIVE, otherwise
	 *         false.
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.2
	 **/
	public boolean supportsResultSetConcurrency(int type, int concurrency)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsResultSetConcurrency].methodEntry();
		try {
			switch (type) {
			case ResultSet.TYPE_FORWARD_ONLY:
			case ResultSet.TYPE_SCROLL_INSENSITIVE:
				return true; // We support both ResultSet.CONCUR_READ_ONLY and
								// ResultSet.CONCUR_UPDATABLE
			case ResultSet.TYPE_SCROLL_SENSITIVE:
				return false;
			}
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsResultSetConcurrency].methodExit();
		}
	}

	// jdk 1.4
	/**
	 * Retrieves whether this database supports savepoints.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.4
	 **/
	public boolean supportsSavepoints() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsSavepoints].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsSavepoints].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports named parameters to callable
	 * statements.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.4
	 **/
	public boolean supportsNamedParameters() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsNamedParameters].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsNamedParameters].methodExit();
		}
	}

	/**
	 * Retrieves whether it is possible to have multiple ResultSet objects
	 * returned from a CallableStatement object simultaneously.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.4
	 **/
	public boolean supportsMultipleOpenResults() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsMultipleOpenResults].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsMultipleOpenResults].methodExit();
		}
	}

	/**
	 * Retrieves whether auto-generated keys can be retrieved after a statement
	 * has been executed.
	 * 
	 * @return false
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.4
	 **/
	public boolean supportsGetGeneratedKeys() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsGetGeneratedKeys].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsGetGeneratedKeys].methodExit();
		}
	}

	/**
	 * <P>
	 * Retrieves a description of the user-defined type (UDT) hierarchies
	 * defined in a particular schema in this database. Only the immediate super
	 * type/ sub type relationship is modeled. Only supertype information for
	 * UDTs matching the catalog, schema, and type name is returned. The type
	 * name parameter may be a fully-qualified name. When the UDT name supplied
	 * is a fully-qualified name, the catalog and schemaPattern parameters are
	 * ignored.
	 * </P>
	 * 
	 * <P>
	 * If a UDT does not have a direct super type, it is not listed here. A row
	 * of the ResultSet object returned by this method describes the designated
	 * UDT and a direct supertype. A row has the following columns:
	 * </P>
	 * 
	 * <OL>
	 * <LI><B>TYPE_CAT</B> String => the UDT's catalog (may be null)
	 * <LI><B>TYPE_SCHEM</B> String => UDT's schema (may be null)
	 * <LI><B>TYPE_NAME</B> String => type name of the UDT
	 * <LI><B>SUPERTYPE_CAT</B> String => the direct super type's catalog (may
	 * be null)
	 * <LI><B>SUPERTYPE_SCHEM</B> String => the direct super type's schema (may
	 * be null)
	 * <LI><B>SUPERTYPE_NAME</B> String => the direct super type's name
	 * </OL>
	 * <P>
	 * <B>Note:</B> If the driver does not support type hierarchies, an empty
	 * result set is returned.
	 * </P>
	 * 
	 * @param catalog
	 *            - a catalog name; "" retrieves those without a catalog; null
	 *            means drop catalog name from the selection criteria
	 * @param schemaPattern
	 *            - a schema name pattern; "" retrieves those without a schema
	 * @param typeNamePattern
	 *            - a UDT name pattern; may be a fully-qualified name
	 * 
	 * @return a ResultSet object in which a row gives information about the
	 *         designated UDT
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.4
	 **/
	public ResultSet getSuperTypes(String catalog, String schemaPattern,
			String typeNamePattern) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getSuperTypes].methodEntry();
		try {
			SQLMXResultSet resultSet;
			SQLMXDesc[] outputDesc;
			DataWrapper[] rows;

			clearWarnings();
			if (connection_.isClosed_)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_connection", null);

			outputDesc = new SQLMXDesc[6];
			outputDesc[0] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "TYPE_CAT",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[1] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "TYPE_SCHEM",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[2] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "TYPE_NAME",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[3] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false,
					"SUPERTYPE_CAT", false, Types.VARCHAR, (short) 0,
					(short) 0, 0, null, null, null, 100, 0, 0, null);
			outputDesc[4] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false,
					"SUPERTYPE_SCHEM", false, Types.VARCHAR, (short) 0,
					(short) 0, 0, null, null, null, 100, 0, 0, null);
			outputDesc[5] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false,
					"SUPERTYPE_NAME", false, Types.VARCHAR, (short) 0,
					(short) 0, 0, null, null, null, 100, 0, 0, null);

			resultSet = new SQLMXResultSet(this, outputDesc, connection_
					.getTxid(), 0);
			rows = new DataWrapper[0];

			// Populate the rows
			resultSet.setFetchOutputs(rows, 0, true, connection_.getTxid());
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getSuperTypes].methodExit();
		}
	}

	/**
	 * <P>
	 * Retrieves a description of the table hierarchies defined in a particular
	 * schema in this database.
	 * </P>
	 * 
	 * <P>
	 * Only supertable information for tables matching the catalog, schema and
	 * table name are returned. The table name parameter may be a fully-
	 * qualified name, in which case, the catalog and schemaPattern parameters
	 * are ignored. If a table does not have a super table, it is not listed
	 * here. Supertables have to be defined in the same catalog and schema as
	 * the sub tables. Therefore, the type description does not need to include
	 * this information for the supertable.
	 * </P>
	 * 
	 * <P>
	 * Each type description has the following columns:
	 * </P>
	 * <OL>
	 * <LI><B>TABLE_CAT</B> String => the type's catalog (may be null)
	 * <LI><B>TABLE_SCHEM</B> String => type's schema (may be null)
	 * <LI><B>TABLE_NAME</B> String => type name
	 * <LI><B>SUPERTABLE_NAME</B> String => the direct super type's name
	 * </OL>
	 * <P>
	 * <B>Note:</B> If the driver does not support type hierarchies, an empty
	 * result set is returned.
	 * </P>
	 * 
	 * @param catalog
	 *            - a catalog name; "" retrieves those without a catalog; null
	 *            means drop catalog name from the selection criteria
	 * @param schemaPattern
	 *            - a schema name pattern; "" retrieves those without a schema
	 * @param typeNamePattern
	 *            - a UDT name pattern; may be a fully-qualified name
	 * 
	 * @return a ResultSet object in which each row is a type description
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.4
	 **/
	public ResultSet getSuperTables(String catalog, String schemaPattern,
			String tableNamePattern) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getSuperTables].methodEntry();
		try {
			SQLMXResultSet resultSet;
			SQLMXDesc[] outputDesc;
			DataWrapper[] rows;

			clearWarnings();
			if (connection_.isClosed_)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_connection", null);

			outputDesc = new SQLMXDesc[4];
			outputDesc[0] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "TYPE_CAT",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[1] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "TYPE_SCHEM",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[2] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "TYPE_NAME",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[3] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false,
					"SUPERTABLE_NAME", false, Types.VARCHAR, (short) 0,
					(short) 0, 0, null, null, null, 100, 0, 0, null);

			resultSet = new SQLMXResultSet(this, outputDesc, connection_
					.getTxid(), 0);
			rows = new DataWrapper[0];

			// Populate the rows
			resultSet.setFetchOutputs(rows, 0, true, connection_.getTxid());
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getSuperTables].methodExit();
		}
	}

	/**
	 * <P>
	 * Retrieves a description of the given attribute of the given type for a
	 * user-defined type (UDT) that is available in the given schema and
	 * catalog.
	 * </P>
	 * 
	 * <P>
	 * Descriptions are returned only for attributes of UDTs matching the
	 * catalog, schema, type, and attribute name criteria. They are ordered by
	 * TYPE_SCHEM, TYPE_NAME and ORDINAL_POSITION. This description does not
	 * contain inherited attributes.
	 * </P>
	 * 
	 * <P>
	 * The ResultSet object that is returned has the following columns:
	 * </P>
	 * <OL>
	 * <LI><B>TYPE_CAT</B> String => type catalog (may be null)
	 * <LI><B>TYPE_SCHEM</B> String => type schema (may be null)
	 * <LI><B>TYPE_NAME</B> String => type name
	 * <LI><B>ATTR_NAME</B> String => attribute name
	 * <LI><B>DATA_TYPE</B> int => attribute type SQL type from java.sql.Types
	 * <LI><B>ATTR_TYPE_NAME</B> String => Data source dependent type name. For
	 * a UDT, the type name is fully qualified. For a REF, the type name is
	 * fully qualified and represents the target type of the reference type.
	 * <LI><B>ATTR_SIZE</B> int => column size. For char or date types this is
	 * the maximum number of characters; for numeric or decimal types this is
	 * precision.
	 * <LI><B>DECIMAL_DIGITS</B> int => the number of fractional digits
	 * <LI><B>NUM_PREC_RADIX</B> int => Radix (typically either 10 or 2)
	 * <LI><B>NULLABLE</B> int => whether NULL is allowed
	 * <ul>
	 * <li>attributeNoNulls - might not allow NULL values
	 * <li>attributeNullable - definitely allows NULL values
	 * <li>attributeNullableUnknown - nullability unknown
	 * </ul>
	 * <LI><B>REMARKS</B> String => comment describing column (may be null)
	 * <LI><B>ATTR_DEF</B> String => default value (may be null)
	 * <LI><B>SQL_DATA_TYPE</B> int => unused
	 * <LI><B>SQL_DATETIME_SUB</B> int => unused
	 * <LI><B>CHAR_OCTET_LENGTH</B> int => for char types the maximum number of
	 * bytes in the column
	 * <LI><B>ORDINAL_POSITION</B> int => index of column in table (starting at
	 * 1)
	 * <LI><B>IS_NULLABLE</B> String => "NO" means column definitely does not
	 * allow NULL values; "YES" means the column might allow NULL values. An
	 * empty string means unknown.
	 * <LI><B>SCOPE_SCHEMA</B> String => schema of table that is the scope of a
	 * reference attribute (null if DATA_TYPE isn't REF)
	 * <LI><B>SCOPE_TABLE</B> String => table name that is the scope of a
	 * reference attribute (null if the DATA_TYPE isn't REF)
	 * </OL>
	 * 
	 * @param catalog
	 *            - a catalog name; must match the catalog name as it is stored
	 *            in the database; "" retrieves those without a catalog; null
	 *            means that the catalog name should not be used to narrow the
	 *            search
	 * @param schemaPattern
	 *            - a schema name pattern; must match the schema name as it is
	 *            stored in the database; "" retrieves those without a schema;
	 *            null means that the schema name should not be used to narrow
	 *            the search
	 * @param typeNamePattern
	 *            - a type name pattern; must match the type name as it is
	 *            stored in the database
	 * @param attributeNamePattern
	 *            - an attribute name pattern; must match the attribute name as
	 *            it is declared in the database
	 * @return a ResultSet object in which each row is an attribute description
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.4
	 **/
	public ResultSet getAttributes(String catalog, String schemaPattern,
			String typeNamePattern, String attributeNamePattern)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getAttributes].methodEntry();
		try {
			SQLMXResultSet resultSet;
			SQLMXDesc[] outputDesc;
			DataWrapper[] rows;

			clearWarnings();
			if (connection_.isClosed_)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_connection", null);

			outputDesc = new SQLMXDesc[21];
			outputDesc[0] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "TYPE_CAT",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[1] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "TYPE_SCHEM",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[2] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "TYPE_NAME",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[3] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "ATTR_NAME",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[4] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_SMALLINT,
					(short) 0, 2, (short) 0, (short) 0, false, "DATA_TYPE",
					false, Types.SMALLINT, (short) 0, (short) 0, 0, null, null,
					null, 130, 0, 0, null);
			outputDesc[5] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false,
					"ATTR_TYPE_NAME", false, Types.VARCHAR, (short) 0,
					(short) 0, 0, null, null, null, 100, 0, 0, null);
			outputDesc[6] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_INTEGER,
					(short) 0, 4, (short) 0, (short) 0, false, "ATTR_SIZE",
					false, Types.INTEGER, (short) 0, (short) 0, 0, null, null,
					null, 132, 0, 0, null);
			outputDesc[7] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_INTEGER,
					(short) 0, 4, (short) 0, (short) 0, false,
					"DECIMAL_DIGITS ", false, Types.INTEGER, (short) 0,
					(short) 0, 0, null, null, null, 132, 0, 0, null);
			outputDesc[8] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_INTEGER,
					(short) 0, 4, (short) 0, (short) 0, false,
					"NUM_PREC_RADIX", false, Types.INTEGER, (short) 0,
					(short) 0, 0, null, null, null, 132, 0, 0, null);
			outputDesc[9] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_INTEGER,
					(short) 0, 4, (short) 0, (short) 0, false, "NULLABLE ",
					false, Types.INTEGER, (short) 0, (short) 0, 0, null, null,
					null, 132, 0, 0, null);
			outputDesc[10] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "REMARKS",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[11] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "ATTR_DEF",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[12] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_INTEGER,
					(short) 0, 4, (short) 0, (short) 0, false, "SQL_DATA_TYPE",
					false, Types.INTEGER, (short) 0, (short) 0, 0, null, null,
					null, 132, 0, 0, null);
			outputDesc[13] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_INTEGER,
					(short) 0, 4, (short) 0, (short) 0, false,
					"SQL_DATETIME_SUB", false, Types.INTEGER, (short) 0,
					(short) 0, 0, null, null, null, 132, 0, 0, null);
			outputDesc[14] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_INTEGER,
					(short) 0, 4, (short) 0, (short) 0, false,
					"CHAR_OCTET_LENGTH", false, Types.INTEGER, (short) 0,
					(short) 0, 0, null, null, null, 132, 0, 0, null);
			outputDesc[15] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_INTEGER,
					(short) 0, 4, (short) 0, (short) 0, false,
					"ORDINAL_POSITION", false, Types.INTEGER, (short) 0,
					(short) 0, 0, null, null, null, 132, 0, 0, null);
			outputDesc[16] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "IS_NULLABLE",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[17] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false,
					"SCOPE_CATALOG", false, Types.VARCHAR, (short) 0,
					(short) 0, 0, null, null, null, 100, 0, 0, null);
			outputDesc[18] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false,
					"SCOPE_SCHEMA", false, Types.VARCHAR, (short) 0, (short) 0,
					0, null, null, null, 100, 0, 0, null);
			outputDesc[19] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_VARCHAR,
					(short) 0, 128, (short) 0, (short) 0, false, "SCOPE_TABLE",
					false, Types.VARCHAR, (short) 0, (short) 0, 0, null, null,
					null, 100, 0, 0, null);
			outputDesc[20] = new SQLMXDesc(SQLMXDesc.SQLTYPECODE_SMALLINT,
					(short) 0, 2, (short) 0, (short) 0, false,
					"SOURCE_DATA_TYPE", false, Types.SMALLINT, (short) 0,
					(short) 0, 0, null, null, null, 130, 0, 0, null);

			resultSet = new SQLMXResultSet(this, outputDesc, connection_
					.getTxid(), 0);
			rows = new DataWrapper[0];

			// Populate the rows
			resultSet.setFetchOutputs(rows, 0, true, connection_.getTxid());
			return resultSet;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getAttributes].methodExit();
		}
	}

	/**
	 * Retrieves the major JDBC version number for this driver.
	 * 
	 * @return Major version number for JDBC/MX driver
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.4
	 **/
	public int getJDBCMajorVersion() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getJDBCMajorVersion].methodEntry();
		try {
			return DriverInfo.JdbcMajorVersion;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getJDBCMajorVersion].methodExit();
		}
	}

	/**
	 * Retrieves the minor JDBC version number for this driver.
	 * 
	 * @return Minor version number for JDBC/MX driver
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.4
	 **/
	public int getJDBCMinorVersion() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getJDBCMinorVersion].methodEntry();
		try {
			return DriverInfo.JdbcMinorVersion;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getJDBCMinorVersion].methodExit();
		}
	}

	/**
	 * Indicates whether the SQLSTATE returned by SQLException.getSQLState is
	 * X/Open (now known as Open Group) SQL CLI or SQL99.
	 * 
	 * <P>
	 * <B>Note:</B> This method is not supported.
	 * </P>
	 * 
	 * @return 1
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.4
	 **/
	public int getSQLStateType() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getSQLStateType].methodEntry();
		try {
			// ToDO:
			return 1;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getSQLStateType].methodExit();
		}
	}

	/**
	 * Indicates whether updates made to a LOB are made on a copy or directly to
	 * the LOB.
	 * 
	 * @return false - updates are made directly to the LOB
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.4
	 **/
	public boolean locatorsUpdateCopy() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_locatorsUpdateCopy].methodEntry();
		try {
			return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_locatorsUpdateCopy].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports statement pooling.
	 * 
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.4
	 **/
	public boolean supportsStatementPooling() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsStatementPooling].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsStatementPooling].methodExit();
		}
	}

	/**
	 * Retrieves whether this database supports the given result set
	 * holdability.
	 * 
	 * @param holdability
	 *            - one of the following constants:
	 *            ResultSet.HOLD_CURSORS_OVER_COMMIT or
	 *            ResultSet.CLOSE_CURSORS_AT_COMMIT
	 * @return true
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.4
	 **/
	public boolean supportsResultSetHoldability(int holdability)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_supportsResultSetHoldability].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_supportsResultSetHoldability].methodExit();
		}
	}

	/**
	 * Retrieves the default holdability of this ResultSet object.
	 * 
	 * @return SQLMXResultSet.CLOSE_CURSORS_AT_COMMIT
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.4
	 **/
	public int getResultSetHoldability() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getResultSetHoldability].methodEntry();
		try {
			return SQLMXResultSet.CLOSE_CURSORS_AT_COMMIT;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getResultSetHoldability].methodExit();
		}
	}

	/**
	 * Retrieves the major version number of the underlying database.
	 * 
	 * @return Trafodion release major number
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.4
	 **/
	public int getDatabaseMajorVersion() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getDatabaseMajorVersion].methodEntry();
		try {
			return T2Driver.getDatabaseMajorVersion();
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getDatabaseMajorVersion].methodExit();
		}
	}

	/**
	 * Retrieves the minor version number of the underlying database.
	 * 
	 * @return Trafodion release minor number
	 * @throws SQLException
	 *             - if a database access error occurs
	 * @since 1.4
	 **/
	public int getDatabaseMinorVersion() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getDatabaseMinorVersion].methodEntry();
		try {
			return T2Driver.getDatabaseMinorVersion();
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getDatabaseMinorVersion].methodExit();
		}
	}

	void setCurrentTxid(int txid) {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setCurrentTxid].methodEntry();
		try {
			connection_.setTxid_(txid);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setCurrentTxid].methodExit();
		}
	}

	// Constructors
	SQLMXDatabaseMetaData(SQLMXConnection connection) {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_SQLMXDatabaseMetaData].methodEntry();
		try {
			connection_ = connection;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_SQLMXDatabaseMetaData].methodExit();
		}
	}

	// native methods
	private native SQLMXResultSet getCatalogs(String server, long dialogueId,
			int txid, boolean autocommit, int txnMode, String catalogPattern);

	private native SQLMXResultSet getSchemas(String server, long dialogueId,
			int txid, boolean autocommit, int txnMode, String schemaPattern);

	private native SQLMXResultSet getTables(String server, long dialogueId,
			int txid, boolean autocommit, int txnMode, String catalog,
			String schemaPattern, String tableNamePattern, String tableType);

	private native SQLMXResultSet getColumns(String server, long dialogueId,
			int txid, boolean autocommit, int txnMode, String catalog,
			String schemaPattern, String tableNamePattern,
			String columnNamePattern);

	private native SQLMXResultSet getPrimaryKeys(String server, long dialogueId,
			int txid, boolean autocommit, int txnMode, String catalog,
			String schema, String table);

	private native SQLMXResultSet getIndexInfo(String server, long dialogueId,
			int txid, boolean autocommit, int txnMode, String catalog,
			String schema, String table, boolean unique, boolean approximate);

	private native SQLMXResultSet getTypeInfo(String server, long dialogueId,
			int txid, boolean autocommit, int txnMode);

	private native SQLMXResultSet getTablePrivileges(String server,
			long dialogueId, int txid, boolean autocommit, int txnMode,
			String catalog, String schema, String table);

	private native SQLMXResultSet getColumnPrivileges(String server,
			long dialogueId, int txid, boolean autocommit, int txnMode,
			String catalog, String schema, String table,
			String columnNamePattern);

	private native SQLMXResultSet getImportedKeys(String server,
			long dialogueId, int txid, boolean autocommit, int txnMode,
			String catalog, String schema, String table);

	private native SQLMXResultSet getExportedKeys(String server,
			long dialogueId, int txid, boolean autocommit, int txnMode,
			String catalog, String schema, String table);

	private native SQLMXResultSet getCrossReference(String server,
			long dialogueId, int txid, boolean autocommit, int txnMode,
			String catalog, String schema, String table, String fkcatalog,
			String fkschema, String fktable);

	private native SQLMXResultSet getBestRowIdentifier(String server,
			long dialogueId, int txid, boolean autocommit, int txnMode,
			String catalog, String schema, String table, int scope,
			boolean nullable);

	private native SQLMXResultSet getVersionColumns(String server,
			long dialogueId, int txid, boolean autocommit, int txnMode,
			String catalog, String schema, String table);

	private native SQLMXResultSet getProcedures(String server, long dialogueId,
			int txid, boolean autocommit, int txnMode, String catalog,
			String schemaPattern, String procPattern);

	private native SQLMXResultSet getProcedureColumns(String server,
			long dialogueId, int  txid, boolean autocommit, int txnMode,
			String catalog, String schemaPattern, String procPattern,
			String columnNamePattern);

	// fields
	SQLMXConnection connection_;

	private static int methodId_allProceduresAreCallable = 0;
	private static int methodId_allTablesAreSelectable = 1;
	private static int methodId_getURL = 2;
	private static int methodId_getUserName = 3;
	private static int methodId_isReadOnly = 4;
	private static int methodId_nullsAreSortedHigh = 5;
	private static int methodId_nullsAreSortedLow = 6;
	private static int methodId_nullsAreSortedAtStart = 7;
	private static int methodId_nullsAreSortedAtEnd = 8;
	private static int methodId_getDatabaseProductName = 9;
	private static int methodId_getDatabaseProductVersion = 10;
	private static int methodId_getDriverName = 11;
	private static int methodId_getDriverVersion = 12;
	private static int methodId_getDriverMajorVersion = 13;
	private static int methodId_getDriverMinorVersion = 14;
	private static int methodId_usesLocalFiles = 15;
	private static int methodId_usesLocalFilePerTable = 16;
	private static int methodId_supportsMixedCaseIdentifiers = 17;
	private static int methodId_storesUpperCaseIdentifiers = 18;
	private static int methodId_storesLowerCaseIdentifiers = 19;
	private static int methodId_storesMixedCaseIdentifiers = 20;
	private static int methodId_supportsMixedCaseQuotedIdentifiers = 21;
	private static int methodId_storesUpperCaseQuotedIdentifiers = 22;
	private static int methodId_storesLowerCaseQuotedIdentifiers = 23;
	private static int methodId_storesMixedCaseQuotedIdentifiers = 24;
	private static int methodId_getIdentifierQuoteString = 25;
	private static int methodId_getSQLKeywords = 26;
	private static int methodId_getNumericFunctions = 27;
	private static int methodId_getStringFunctions = 28;
	private static int methodId_getSystemFunctions = 29;
	private static int methodId_getTimeDateFunctions = 30;
	private static int methodId_getSearchStringEscape = 31;
	private static int methodId_getExtraNameCharacters = 32;
	private static int methodId_supportsAlterTableWithAddColumn = 33;
	private static int methodId_supportsAlterTableWithDropColumn = 34;
	private static int methodId_supportsColumnAliasing = 35;
	private static int methodId_nullPlusNonNullIsNull = 36;
	private static int methodId_supportsConvert_V = 37;
	private static int methodId_supportsConvert_II = 38;
	private static int methodId_supportsTableCorrelationNames = 39;
	private static int methodId_supportsDifferentTableCorrelationNames = 40;
	private static int methodId_supportsExpressionsInOrderBy = 41;
	private static int methodId_supportsOrderByUnrelated = 42;
	private static int methodId_supportsGroupBy = 43;
	private static int methodId_supportsGroupByUnrelated = 44;
	private static int methodId_supportsGroupByBeyondSelect = 45;
	private static int methodId_supportsLikeEscapeClause = 46;
	private static int methodId_supportsMultipleResultSets = 47;
	private static int methodId_supportsMultipleTransactions = 48;
	private static int methodId_supportsNonNullableColumns = 49;
	private static int methodId_supportsMinimumSQLGrammar = 50;
	private static int methodId_supportsCoreSQLGrammar = 51;
	private static int methodId_supportsExtendedSQLGrammar = 52;
	private static int methodId_supportsANSI92EntryLevelSQL = 53;
	private static int methodId_supportsANSI92IntermediateSQL = 54;
	private static int methodId_supportsANSI92FullSQL = 55;
	private static int methodId_supportsIntegrityEnhancementFacility = 56;
	private static int methodId_supportsOuterJoins = 57;
	private static int methodId_supportsFullOuterJoins = 58;
	private static int methodId_supportsLimitedOuterJoins = 59;
	private static int methodId_getSchemaTerm = 60;
	private static int methodId_getProcedureTerm = 61;
	private static int methodId_getCatalogTerm = 62;
	private static int methodId_isCatalogAtStart = 63;
	private static int methodId_getCatalogSeparator = 64;
	private static int methodId_supportsSchemasInDataManipulation = 65;
	private static int methodId_supportsSchemasInProcedureCalls = 66;
	private static int methodId_supportsSchemasInTableDefinitions = 67;
	private static int methodId_supportsSchemasInIndexDefinitions = 68;
	private static int methodId_supportsSchemasInPrivilegeDefinitions = 69;
	private static int methodId_supportsCatalogsInDataManipulation = 70;
	private static int methodId_supportsCatalogsInProcedureCalls = 71;
	private static int methodId_supportsCatalogsInTableDefinitions = 72;
	private static int methodId_supportsCatalogsInIndexDefinitions = 73;
	private static int methodId_supportsCatalogsInPrivilegeDefinitions = 74;
	private static int methodId_supportsPositionedDelete = 75;
	private static int methodId_supportsPositionedUpdate = 76;
	private static int methodId_supportsSelectForUpdate = 77;
	private static int methodId_supportsStoredProcedures = 78;
	private static int methodId_supportsSubqueriesInComparisons = 79;
	private static int methodId_supportsSubqueriesInExists = 80;
	private static int methodId_supportsSubqueriesInIns = 81;
	private static int methodId_supportsSubqueriesInQuantifieds = 82;
	private static int methodId_supportsCorrelatedSubqueries = 83;
	private static int methodId_supportsUnion = 84;
	private static int methodId_supportsUnionAll = 85;
	private static int methodId_supportsOpenCursorsAcrossCommit = 86;
	private static int methodId_supportsOpenCursorsAcrossRollback = 87;
	private static int methodId_supportsOpenStatementsAcrossCommit = 88;
	private static int methodId_supportsOpenStatementsAcrossRollback = 89;
	private static int methodId_getMaxBinaryLiteralLength = 90;
	private static int methodId_getMaxCharLiteralLength = 91;
	private static int methodId_getMaxColumnNameLength = 92;
	private static int methodId_getMaxColumnsInGroupBy = 93;
	private static int methodId_getMaxColumnsInIndex = 94;
	private static int methodId_getMaxColumnsInOrderBy = 95;
	private static int methodId_getMaxColumnsInSelect = 96;
	private static int methodId_getMaxColumnsInTable = 97;
	private static int methodId_getMaxConnections = 98;
	private static int methodId_getMaxCursorNameLength = 99;
	private static int methodId_getMaxIndexLength = 100;
	private static int methodId_getMaxSchemaNameLength = 101;
	private static int methodId_getMaxProcedureNameLength = 102;
	private static int methodId_getMaxCatalogNameLength = 103;
	private static int methodId_getMaxRowSize = 104;
	private static int methodId_doesMaxRowSizeIncludeBlobs = 105;
	private static int methodId_getMaxStatementLength = 106;
	private static int methodId_getMaxStatements = 107;
	private static int methodId_getMaxTableNameLength = 108;
	private static int methodId_getMaxTablesInSelect = 109;
	private static int methodId_getMaxUserNameLength = 110;
	private static int methodId_getDefaultTransactionIsolation = 111;
	private static int methodId_supportsTransactions = 112;
	private static int methodId_supportsTransactionIsolationLevel = 113;
	private static int methodId_supportsDataDefinitionAndDataManipulationTransactions = 114;
	private static int methodId_supportsDataManipulationTransactionsOnly = 115;
	private static int methodId_dataDefinitionCausesTransactionCommit = 116;
	private static int methodId_dataDefinitionIgnoredInTransactions = 117;
	private static int methodId_getProcedures = 118;
	private static int methodId_getProcedureColumns = 119;
	private static int methodId_getTables = 120;
	private static int methodId_getSchemas = 121;
	private static int methodId_getCatalogs = 122;
	private static int methodId_getTableTypes = 123;
	private static int methodId_getColumns = 124;
	private static int methodId_getColumnPrivileges = 125;
	private static int methodId_getTablePrivileges = 126;
	private static int methodId_getBestRowIdentifier = 127;
	private static int methodId_getVersionColumns = 128;
	private static int methodId_getPrimaryKeys = 129;
	private static int methodId_getImportedKeys = 130;
	private static int methodId_getExportedKeys = 131;
	private static int methodId_getCrossReference = 132;
	private static int methodId_getTypeInfo = 133;
	private static int methodId_getIndexInfo = 134;
	private static int methodId_deletesAreDetected = 135;
	private static int methodId_getConnection = 136;
	private static int methodId_getUDTs = 137;
	private static int methodId_insertsAreDetected = 138;
	private static int methodId_updatesAreDetected = 139;
	private static int methodId_othersDeletesAreVisible = 140;
	private static int methodId_othersInsertsAreVisible = 141;
	private static int methodId_othersUpdatesAreVisible = 142;
	private static int methodId_ownDeletesAreVisible = 143;
	private static int methodId_ownInsertsAreVisible = 144;
	private static int methodId_ownUpdatesAreVisible = 145;
	private static int methodId_supportsBatchUpdates = 146;
	private static int methodId_supportsResultSetType = 147;
	private static int methodId_supportsResultSetConcurrency = 148;
	private static int methodId_supportsSavepoints = 149;
	private static int methodId_supportsNamedParameters = 150;
	private static int methodId_supportsMultipleOpenResults = 151;
	private static int methodId_supportsGetGeneratedKeys = 152;
	private static int methodId_getSuperTypes = 153;
	private static int methodId_getSuperTables = 154;
	private static int methodId_getAttributes = 155;
	private static int methodId_getJDBCMajorVersion = 156;
	private static int methodId_getJDBCMinorVersion = 157;
	private static int methodId_getSQLStateType = 158;
	private static int methodId_locatorsUpdateCopy = 159;
	private static int methodId_supportsStatementPooling = 160;
	private static int methodId_supportsResultSetHoldability = 161;
	private static int methodId_getResultSetHoldability = 162;
	private static int methodId_getDatabaseMajorVersion = 163;
	private static int methodId_getDatabaseMinorVersion = 164;
	private static int methodId_setCurrentTxid = 165;
	private static int methodId_SQLMXDatabaseMetaData = 166;
	private static int totalMethodIds = 167;
	private static JdbcDebug[] debug;

	static {
		String className = "SQLMXDatabaseMetaData";
		if (JdbcDebugCfg.entryActive) {
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_allProceduresAreCallable] = new JdbcDebug(className,
					"allProceduresAreCallable");
			debug[methodId_allTablesAreSelectable] = new JdbcDebug(className,
					"allTablesAreSelectable");
			debug[methodId_getURL] = new JdbcDebug(className, "getURL");
			debug[methodId_getUserName] = new JdbcDebug(className,
					"getUserName");
			debug[methodId_isReadOnly] = new JdbcDebug(className, "isReadOnly");
			debug[methodId_nullsAreSortedHigh] = new JdbcDebug(className,
					"nullsAreSortedHigh");
			debug[methodId_nullsAreSortedLow] = new JdbcDebug(className,
					"nullsAreSortedLow");
			debug[methodId_nullsAreSortedAtStart] = new JdbcDebug(className,
					"nullsAreSortedAtStart");
			debug[methodId_nullsAreSortedAtEnd] = new JdbcDebug(className,
					"nullsAreSortedAtEnd");
			debug[methodId_getDatabaseProductName] = new JdbcDebug(className,
					"getDatabaseProductName");
			debug[methodId_getDatabaseProductVersion] = new JdbcDebug(
					className, "getDatabaseProductVersion");
			debug[methodId_getDriverName] = new JdbcDebug(className,
					"getDriverName");
			debug[methodId_getDriverVersion] = new JdbcDebug(className,
					"getDriverVersion");
			debug[methodId_getDriverMajorVersion] = new JdbcDebug(className,
					"getDriverMajorVersion");
			debug[methodId_getDriverMinorVersion] = new JdbcDebug(className,
					"getDriverMinorVersion");
			debug[methodId_usesLocalFiles] = new JdbcDebug(className,
					"usesLocalFiles");
			debug[methodId_usesLocalFilePerTable] = new JdbcDebug(className,
					"usesLocalFilePerTable");
			debug[methodId_supportsMixedCaseIdentifiers] = new JdbcDebug(
					className, "supportsMixedCaseIdentifiers");
			debug[methodId_storesUpperCaseIdentifiers] = new JdbcDebug(
					className, "storesUpperCaseIdentifiers");
			debug[methodId_storesLowerCaseIdentifiers] = new JdbcDebug(
					className, "storesLowerCaseIdentifiers");
			debug[methodId_storesMixedCaseIdentifiers] = new JdbcDebug(
					className, "storesMixedCaseIdentifiers");
			debug[methodId_supportsMixedCaseQuotedIdentifiers] = new JdbcDebug(
					className, "supportsMixedCaseQuotedIdentifiers");
			debug[methodId_storesUpperCaseQuotedIdentifiers] = new JdbcDebug(
					className, "storesUpperCaseQuotedIdentifiers");
			debug[methodId_storesLowerCaseQuotedIdentifiers] = new JdbcDebug(
					className, "storesLowerCaseQuotedIdentifiers");
			debug[methodId_storesMixedCaseQuotedIdentifiers] = new JdbcDebug(
					className, "storesMixedCaseQuotedIdentifiers");
			debug[methodId_getIdentifierQuoteString] = new JdbcDebug(className,
					"getIdentifierQuoteString");
			debug[methodId_getSQLKeywords] = new JdbcDebug(className,
					"getSQLKeywords");
			debug[methodId_getNumericFunctions] = new JdbcDebug(className,
					"getNumericFunctions");
			debug[methodId_getStringFunctions] = new JdbcDebug(className,
					"getStringFunctions");
			debug[methodId_getSystemFunctions] = new JdbcDebug(className,
					"getSystemFunctions");
			debug[methodId_getTimeDateFunctions] = new JdbcDebug(className,
					"getTimeDateFunctions");
			debug[methodId_getSearchStringEscape] = new JdbcDebug(className,
					"getSearchStringEscape");
			debug[methodId_getExtraNameCharacters] = new JdbcDebug(className,
					"getExtraNameCharacters");
			debug[methodId_supportsAlterTableWithAddColumn] = new JdbcDebug(
					className, "supportsAlterTableWithAddColumn");
			debug[methodId_supportsAlterTableWithDropColumn] = new JdbcDebug(
					className, "supportsAlterTableWithDropColumn");
			debug[methodId_supportsColumnAliasing] = new JdbcDebug(className,
					"supportsColumnAliasing");
			debug[methodId_nullPlusNonNullIsNull] = new JdbcDebug(className,
					"nullPlusNonNullIsNull");
			debug[methodId_supportsConvert_V] = new JdbcDebug(className,
					"supportsConvert[V]");
			debug[methodId_supportsConvert_II] = new JdbcDebug(className,
					"supportsConvert[II]");
			debug[methodId_supportsTableCorrelationNames] = new JdbcDebug(
					className, "supportsTableCorrelationNames");
			debug[methodId_supportsDifferentTableCorrelationNames] = new JdbcDebug(
					className, "supportsDifferentTableCorrelationNames");
			debug[methodId_supportsExpressionsInOrderBy] = new JdbcDebug(
					className, "supportsExpressionsInOrderBy");
			debug[methodId_supportsOrderByUnrelated] = new JdbcDebug(className,
					"supportsOrderByUnrelated");
			debug[methodId_supportsGroupBy] = new JdbcDebug(className,
					"supportsGroupBy");
			debug[methodId_supportsGroupByUnrelated] = new JdbcDebug(className,
					"supportsGroupByUnrelated");
			debug[methodId_supportsGroupByBeyondSelect] = new JdbcDebug(
					className, "supportsGroupByBeyondSelect");
			debug[methodId_supportsLikeEscapeClause] = new JdbcDebug(className,
					"supportsLikeEscapeClause");
			debug[methodId_supportsMultipleResultSets] = new JdbcDebug(
					className, "supportsMultipleResultSets");
			debug[methodId_supportsMultipleTransactions] = new JdbcDebug(
					className, "supportsMultipleTransactions");
			debug[methodId_supportsNonNullableColumns] = new JdbcDebug(
					className, "supportsNonNullableColumns");
			debug[methodId_supportsMinimumSQLGrammar] = new JdbcDebug(
					className, "supportsMinimumSQLGrammar");
			debug[methodId_supportsCoreSQLGrammar] = new JdbcDebug(className,
					"supportsCoreSQLGrammar");
			debug[methodId_supportsExtendedSQLGrammar] = new JdbcDebug(
					className, "supportsExtendedSQLGrammar");
			debug[methodId_supportsANSI92EntryLevelSQL] = new JdbcDebug(
					className, "supportsANSI92EntryLevelSQL");
			debug[methodId_supportsANSI92IntermediateSQL] = new JdbcDebug(
					className, "supportsANSI92IntermediateSQL");
			debug[methodId_supportsANSI92FullSQL] = new JdbcDebug(className,
					"supportsANSI92FullSQL");
			debug[methodId_supportsIntegrityEnhancementFacility] = new JdbcDebug(
					className, "supportsIntegrityEnhancementFacility");
			debug[methodId_supportsOuterJoins] = new JdbcDebug(className,
					"supportsOuterJoins");
			debug[methodId_supportsFullOuterJoins] = new JdbcDebug(className,
					"supportsFullOuterJoins");
			debug[methodId_supportsLimitedOuterJoins] = new JdbcDebug(
					className, "supportsLimitedOuterJoins");
			debug[methodId_getSchemaTerm] = new JdbcDebug(className,
					"getSchemaTerm");
			debug[methodId_getProcedureTerm] = new JdbcDebug(className,
					"getProcedureTerm");
			debug[methodId_getCatalogTerm] = new JdbcDebug(className,
					"getCatalogTerm");
			debug[methodId_isCatalogAtStart] = new JdbcDebug(className,
					"isCatalogAtStart");
			debug[methodId_getCatalogSeparator] = new JdbcDebug(className,
					"getCatalogSeparator");
			debug[methodId_supportsSchemasInDataManipulation] = new JdbcDebug(
					className, "supportsSchemasInDataManipulation");
			debug[methodId_supportsSchemasInProcedureCalls] = new JdbcDebug(
					className, "supportsSchemasInProcedureCalls");
			debug[methodId_supportsSchemasInTableDefinitions] = new JdbcDebug(
					className, "supportsSchemasInTableDefinitions");
			debug[methodId_supportsSchemasInIndexDefinitions] = new JdbcDebug(
					className, "supportsSchemasInIndexDefinitions");
			debug[methodId_supportsSchemasInPrivilegeDefinitions] = new JdbcDebug(
					className, "supportsSchemasInPrivilegeDefinitions");
			debug[methodId_supportsCatalogsInDataManipulation] = new JdbcDebug(
					className, "supportsCatalogsInDataManipulation");
			debug[methodId_supportsCatalogsInProcedureCalls] = new JdbcDebug(
					className, "supportsCatalogsInProcedureCalls");
			debug[methodId_supportsCatalogsInTableDefinitions] = new JdbcDebug(
					className, "supportsCatalogsInTableDefinitions");
			debug[methodId_supportsCatalogsInIndexDefinitions] = new JdbcDebug(
					className, "supportsCatalogsInIndexDefinitions");
			debug[methodId_supportsCatalogsInPrivilegeDefinitions] = new JdbcDebug(
					className, "supportsCatalogsInPrivilegeDefinitions");
			debug[methodId_supportsPositionedDelete] = new JdbcDebug(className,
					"supportsPositionedDelete");
			debug[methodId_supportsPositionedUpdate] = new JdbcDebug(className,
					"supportsPositionedUpdate");
			debug[methodId_supportsSelectForUpdate] = new JdbcDebug(className,
					"supportsSelectForUpdate");
			debug[methodId_supportsStoredProcedures] = new JdbcDebug(className,
					"supportsStoredProcedures");
			debug[methodId_supportsSubqueriesInComparisons] = new JdbcDebug(
					className, "supportsSubqueriesInComparisons");
			debug[methodId_supportsSubqueriesInExists] = new JdbcDebug(
					className, "supportsSubqueriesInExists");
			debug[methodId_supportsSubqueriesInIns] = new JdbcDebug(className,
					"supportsSubqueriesInIns");
			debug[methodId_supportsSubqueriesInQuantifieds] = new JdbcDebug(
					className, "supportsSubqueriesInQuantifieds");
			debug[methodId_supportsCorrelatedSubqueries] = new JdbcDebug(
					className, "supportsCorrelatedSubqueries");
			debug[methodId_supportsUnion] = new JdbcDebug(className,
					"supportsUnion");
			debug[methodId_supportsUnionAll] = new JdbcDebug(className,
					"supportsUnionAll");
			debug[methodId_supportsOpenCursorsAcrossCommit] = new JdbcDebug(
					className, "supportsOpenCursorsAcrossCommit");
			debug[methodId_supportsOpenCursorsAcrossRollback] = new JdbcDebug(
					className, "supportsOpenCursorsAcrossRollback");
			debug[methodId_supportsOpenStatementsAcrossCommit] = new JdbcDebug(
					className, "supportsOpenStatementsAcrossCommit");
			debug[methodId_supportsOpenStatementsAcrossRollback] = new JdbcDebug(
					className, "supportsOpenStatementsAcrossRollback");
			debug[methodId_getMaxBinaryLiteralLength] = new JdbcDebug(
					className, "getMaxBinaryLiteralLength");
			debug[methodId_getMaxCharLiteralLength] = new JdbcDebug(className,
					"getMaxCharLiteralLength");
			debug[methodId_getMaxColumnNameLength] = new JdbcDebug(className,
					"getMaxColumnNameLength");
			debug[methodId_getMaxColumnsInGroupBy] = new JdbcDebug(className,
					"getMaxColumnsInGroupBy");
			debug[methodId_getMaxColumnsInIndex] = new JdbcDebug(className,
					"getMaxColumnsInIndex");
			debug[methodId_getMaxColumnsInOrderBy] = new JdbcDebug(className,
					"getMaxColumnsInOrderBy");
			debug[methodId_getMaxColumnsInSelect] = new JdbcDebug(className,
					"getMaxColumnsInSelect");
			debug[methodId_getMaxColumnsInTable] = new JdbcDebug(className,
					"getMaxColumnsInTable");
			debug[methodId_getMaxConnections] = new JdbcDebug(className,
					"getMaxConnections");
			debug[methodId_getMaxCursorNameLength] = new JdbcDebug(className,
					"getMaxCursorNameLength");
			debug[methodId_getMaxIndexLength] = new JdbcDebug(className,
					"getMaxIndexLength");
			debug[methodId_getMaxSchemaNameLength] = new JdbcDebug(className,
					"getMaxSchemaNameLength");
			debug[methodId_getMaxProcedureNameLength] = new JdbcDebug(
					className, "getMaxProcedureNameLength");
			debug[methodId_getMaxCatalogNameLength] = new JdbcDebug(className,
					"getMaxCatalogNameLength");
			debug[methodId_getMaxRowSize] = new JdbcDebug(className,
					"getMaxRowSize");
			debug[methodId_doesMaxRowSizeIncludeBlobs] = new JdbcDebug(
					className, "doesMaxRowSizeIncludeBlobs");
			debug[methodId_getMaxStatementLength] = new JdbcDebug(className,
					"getMaxStatementLength");
			debug[methodId_getMaxStatements] = new JdbcDebug(className,
					"getMaxStatements");
			debug[methodId_getMaxTableNameLength] = new JdbcDebug(className,
					"getMaxTableNameLength");
			debug[methodId_getMaxTablesInSelect] = new JdbcDebug(className,
					"getMaxTablesInSelect");
			debug[methodId_getMaxUserNameLength] = new JdbcDebug(className,
					"getMaxUserNameLength");
			debug[methodId_getDefaultTransactionIsolation] = new JdbcDebug(
					className, "getDefaultTransactionIsolation");
			debug[methodId_supportsTransactions] = new JdbcDebug(className,
					"supportsTransactions");
			debug[methodId_supportsTransactionIsolationLevel] = new JdbcDebug(
					className, "supportsTransactionIsolationLevel");
			debug[methodId_supportsDataDefinitionAndDataManipulationTransactions] = new JdbcDebug(
					className,
					"supportsDataDefinitionAndDataManipulationTransactions");
			debug[methodId_supportsDataManipulationTransactionsOnly] = new JdbcDebug(
					className, "supportsDataManipulationTransactionsOnly");
			debug[methodId_dataDefinitionCausesTransactionCommit] = new JdbcDebug(
					className, "dataDefinitionCausesTransactionCommit");
			debug[methodId_dataDefinitionIgnoredInTransactions] = new JdbcDebug(
					className, "dataDefinitionIgnoredInTransactions");
			debug[methodId_getProcedures] = new JdbcDebug(className,
					"getProcedures");
			debug[methodId_getProcedureColumns] = new JdbcDebug(className,
					"getProcedureColumns");
			debug[methodId_getTables] = new JdbcDebug(className, "getTables");
			debug[methodId_getSchemas] = new JdbcDebug(className, "getSchemas");
			debug[methodId_getCatalogs] = new JdbcDebug(className,
					"getCatalogs");
			debug[methodId_getTableTypes] = new JdbcDebug(className,
					"getTableTypes");
			debug[methodId_getColumns] = new JdbcDebug(className, "getColumns");
			debug[methodId_getColumnPrivileges] = new JdbcDebug(className,
					"getColumnPrivileges");
			debug[methodId_getTablePrivileges] = new JdbcDebug(className,
					"getTablePrivileges");
			debug[methodId_getBestRowIdentifier] = new JdbcDebug(className,
					"getBestRowIdentifier");
			debug[methodId_getVersionColumns] = new JdbcDebug(className,
					"getVersionColumns");
			debug[methodId_getPrimaryKeys] = new JdbcDebug(className,
					"getPrimaryKeys");
			debug[methodId_getImportedKeys] = new JdbcDebug(className,
					"getImportedKeys");
			debug[methodId_getExportedKeys] = new JdbcDebug(className,
					"getExportedKeys");
			debug[methodId_getCrossReference] = new JdbcDebug(className,
					"getCrossReference");
			debug[methodId_getTypeInfo] = new JdbcDebug(className,
					"getTypeInfo");
			debug[methodId_getIndexInfo] = new JdbcDebug(className,
					"getIndexInfo");
			debug[methodId_deletesAreDetected] = new JdbcDebug(className,
					"deletesAreDetected");
			debug[methodId_getConnection] = new JdbcDebug(className,
					"getConnection");
			debug[methodId_getUDTs] = new JdbcDebug(className, "getUDTs");
			debug[methodId_insertsAreDetected] = new JdbcDebug(className,
					"insertsAreDetected");
			debug[methodId_updatesAreDetected] = new JdbcDebug(className,
					"updatesAreDetected");
			debug[methodId_othersDeletesAreVisible] = new JdbcDebug(className,
					"othersDeletesAreVisible");
			debug[methodId_othersInsertsAreVisible] = new JdbcDebug(className,
					"othersInsertsAreVisible");
			debug[methodId_othersUpdatesAreVisible] = new JdbcDebug(className,
					"othersUpdatesAreVisible");
			debug[methodId_ownDeletesAreVisible] = new JdbcDebug(className,
					"ownDeletesAreVisible");
			debug[methodId_ownInsertsAreVisible] = new JdbcDebug(className,
					"ownInsertsAreVisible");
			debug[methodId_ownUpdatesAreVisible] = new JdbcDebug(className,
					"ownUpdatesAreVisible");
			debug[methodId_supportsBatchUpdates] = new JdbcDebug(className,
					"supportsBatchUpdates");
			debug[methodId_supportsResultSetType] = new JdbcDebug(className,
					"supportsResultSetType");
			debug[methodId_supportsResultSetConcurrency] = new JdbcDebug(
					className, "supportsResultSetConcurrency");
			debug[methodId_supportsSavepoints] = new JdbcDebug(className,
					"supportsSavepoints");
			debug[methodId_supportsNamedParameters] = new JdbcDebug(className,
					"supportsNamedParameters");
			debug[methodId_supportsMultipleOpenResults] = new JdbcDebug(
					className, "supportsMultipleOpenResults");
			debug[methodId_supportsGetGeneratedKeys] = new JdbcDebug(className,
					"supportsGetGeneratedKeys");
			debug[methodId_getSuperTypes] = new JdbcDebug(className,
					"getSuperTypes");
			debug[methodId_getSuperTables] = new JdbcDebug(className,
					"getSuperTables");
			debug[methodId_getAttributes] = new JdbcDebug(className,
					"getAttributes");
			debug[methodId_getJDBCMajorVersion] = new JdbcDebug(className,
					"getJDBCMajorVersion");
			debug[methodId_getJDBCMinorVersion] = new JdbcDebug(className,
					"getJDBCMinorVersion");
			debug[methodId_getSQLStateType] = new JdbcDebug(className,
					"getSQLStateType");
			debug[methodId_locatorsUpdateCopy] = new JdbcDebug(className,
					"locatorsUpdateCopy");
			debug[methodId_supportsStatementPooling] = new JdbcDebug(className,
					"supportsStatementPooling");
			debug[methodId_supportsResultSetHoldability] = new JdbcDebug(
					className, "supportsResultSetHoldability");
			debug[methodId_getResultSetHoldability] = new JdbcDebug(className,
					"getResultSetHoldability");
			debug[methodId_getDatabaseMajorVersion] = new JdbcDebug(className,
					"getDatabaseMajorVersion");
			debug[methodId_getDatabaseMinorVersion] = new JdbcDebug(className,
					"getDatabaseMinorVersion");
			debug[methodId_setCurrentTxid] = new JdbcDebug(className,
					"setCurrentTxid");
			debug[methodId_SQLMXDatabaseMetaData] = new JdbcDebug(className,
					"SQLMXDatabaseMetaData");
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

	public RowIdLifetime getRowIdLifetime() throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public ResultSet getSchemas(String catalog, String schemaPattern)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public boolean supportsStoredFunctionsUsingCallSyntax() throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	public boolean autoCommitFailureClosesAllResultSets() throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	public ResultSet getClientInfoProperties() throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public ResultSet getFunctions(String catalog, String schemaPattern,
			String functionNamePattern) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public ResultSet getFunctionColumns(String catalog, String schemaPattern,
			String functionNamePattern, String columnNamePattern)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public ResultSet getPseudoColumns(String catalog, String schemaPattern,
			String tableNamePattern, String columnNamePattern)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public boolean generatedKeyAlwaysReturned() throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}
}
