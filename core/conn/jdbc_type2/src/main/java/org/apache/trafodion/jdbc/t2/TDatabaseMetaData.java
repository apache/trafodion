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
 * Filename    : TDatabaseMetaData.java
 * Description :
 *
 * --------------------------------------------------------------------
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.io.PrintWriter;
import java.util.Date;

import org.apache.trafodion.jdbc.t2.SQLMXResultSet;

public class TDatabaseMetaData implements java.sql.DatabaseMetaData {

	public boolean allProceduresAreCallable() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "allProceduresAreCallable()");
		}

		retValue = dbMD_.allProceduresAreCallable();
		return retValue;
	}

	public boolean allTablesAreSelectable() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "allTablesAreSelectable()");
		}

		retValue = dbMD_.allTablesAreSelectable();
		return retValue;
	}

	public String getURL() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getURL()");
		}

		retValue = dbMD_.getURL();
		return retValue;
	}

	public String getUserName() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getUserName()");
		}

		retValue = dbMD_.getUserName();
		return retValue;
	}

	public boolean isReadOnly() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "isReadOnly()");
		}

		retValue = dbMD_.isReadOnly();
		return retValue;
	}

	public boolean nullsAreSortedHigh() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "nullsAreSortedHigh()");
		}

		retValue = dbMD_.nullsAreSortedHigh();
		return retValue;
	}

	public boolean nullsAreSortedLow() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "nullsAreSortedLow()");
		}

		retValue = dbMD_.nullsAreSortedLow();
		return retValue;
	}

	public boolean nullsAreSortedAtStart() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "nullsAreSortedAtStart()");
		}

		retValue = dbMD_.nullsAreSortedAtStart();
		return retValue;
	}

	public boolean nullsAreSortedAtEnd() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "nullsAreSortedAtEnd()");
		}

		retValue = dbMD_.nullsAreSortedAtEnd();
		return retValue;
	}

	public String getDatabaseProductName() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getDatabaseProductName()");
		}

		retValue = dbMD_.getDatabaseProductName();
		return retValue;
	}

	public String getDatabaseProductVersion() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getDatabaseProductVersion()");
		}

		retValue = dbMD_.getDatabaseProductVersion();
		return retValue;
	}

	public String getDriverName() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getDriverName()");
		}

		retValue = dbMD_.getDriverName();
		return retValue;
	}

	public String getDriverVersion() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getDriverVersion()");
		}

		retValue = dbMD_.getDriverVersion();
		return retValue;
	}

	public int getDriverMajorVersion() {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getDriverMajorVersion()");
		}

		retValue = dbMD_.getDriverMajorVersion();
		return retValue;
	}

	public int getDriverMinorVersion() {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getDriverMinorVersion()");
		}

		retValue = dbMD_.getDriverMinorVersion();
		return retValue;
	}

	public boolean usesLocalFiles() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "usesLocalFiles()");
		}

		retValue = dbMD_.usesLocalFiles();
		return retValue;
	}

	public boolean usesLocalFilePerTable() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "usesLocalFilePerTable()");
		}

		retValue = dbMD_.usesLocalFilePerTable();
		return retValue;
	}

	public boolean supportsMixedCaseIdentifiers() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsMixedCaseIdentifiers()");
		}

		retValue = dbMD_.supportsMixedCaseIdentifiers();
		return retValue;
	}

	public boolean storesUpperCaseIdentifiers() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "storesUpperCaseIdentifiers()");
		}

		retValue = dbMD_.storesUpperCaseIdentifiers();
		return retValue;
	}

	public boolean storesLowerCaseIdentifiers() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "storesLowerCaseIdentifiers()");
		}

		retValue = dbMD_.storesLowerCaseIdentifiers();
		return retValue;
	}

	public boolean storesMixedCaseIdentifiers() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "storesMixedCaseIdentifiers()");
		}

		retValue = dbMD_.storesMixedCaseIdentifiers();
		return retValue;
	}

	public boolean supportsMixedCaseQuotedIdentifiers() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsMixedCaseQuotedIdentifiers()");
		}

		retValue = dbMD_.supportsMixedCaseQuotedIdentifiers();
		return retValue;
	}

	public boolean storesUpperCaseQuotedIdentifiers() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "storesUpperCaseQuotedIdentifiers()");
		}

		retValue = dbMD_.storesUpperCaseQuotedIdentifiers();
		return retValue;
	}

	public boolean storesLowerCaseQuotedIdentifiers() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "storesLowerCaseQuotedIdentifiers()");
		}

		retValue = dbMD_.storesLowerCaseQuotedIdentifiers();
		return retValue;
	}

	public boolean storesMixedCaseQuotedIdentifiers() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "storesMixedCaseQuotedIdentifiers()");
		}

		retValue = dbMD_.storesMixedCaseQuotedIdentifiers();
		return retValue;
	}

	public String getIdentifierQuoteString() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getIdentifierQuoteString()");
		}

		retValue = dbMD_.getIdentifierQuoteString();
		return retValue;
	}

	public String getSQLKeywords() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getSQLKeywords()");
		}

		retValue = dbMD_.getSQLKeywords();
		return retValue;
	}

	public String getNumericFunctions() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getNumericFunctions()");
		}

		retValue = dbMD_.getNumericFunctions();
		return retValue;
	}

	public String getStringFunctions() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getStringFunctions()");
		}

		retValue = dbMD_.getStringFunctions();
		return retValue;
	}

	public String getSystemFunctions() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getSystemFunctions()");
		}

		retValue = dbMD_.getSystemFunctions();
		return retValue;
	}

	public String getTimeDateFunctions() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getTimeDateFunctions()");
		}

		retValue = dbMD_.getTimeDateFunctions();
		return retValue;
	}

	public String getSearchStringEscape() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getSearchStringEscape()");
		}

		retValue = dbMD_.getSearchStringEscape();
		return retValue;
	}

	public String getExtraNameCharacters() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getExtraNameCharacters()");
		}

		retValue = dbMD_.getExtraNameCharacters();
		return retValue;
	}

	public boolean supportsAlterTableWithAddColumn() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsAlterTableWithAddColumn()");
		}

		retValue = dbMD_.supportsAlterTableWithAddColumn();
		return retValue;
	}

	public boolean supportsAlterTableWithDropColumn() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsAlterTableWithDropColumn()");
		}

		retValue = dbMD_.supportsAlterTableWithDropColumn();
		return retValue;
	}

	public boolean supportsColumnAliasing() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsColumnAliasing()");
		}

		retValue = dbMD_.supportsColumnAliasing();
		return retValue;
	}

	public boolean nullPlusNonNullIsNull() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "nullPlusNonNullIsNull()");
		}

		retValue = dbMD_.nullPlusNonNullIsNull();
		return retValue;
	}

	public boolean supportsConvert() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsConvert()");
		}

		retValue = dbMD_.supportsConvert();
		return retValue;
	}

	public boolean supportsConvert(int fromType, int toType)
			throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsConvert(" + fromType + ","
					+ toType + ")");
		}

		retValue = dbMD_.supportsConvert(fromType, toType);
		return retValue;
	}

	public boolean supportsTableCorrelationNames() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsTableCorrelationNames()");
		}

		retValue = dbMD_.supportsTableCorrelationNames();
		return retValue;
	}

	public boolean supportsDifferentTableCorrelationNames() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId()
					+ "supportsDifferentTableCorrelationNames()");
		}

		retValue = dbMD_.supportsDifferentTableCorrelationNames();
		return retValue;
	}

	public boolean supportsExpressionsInOrderBy() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsExpressionsInOrderBy()");
		}

		retValue = dbMD_.supportsExpressionsInOrderBy();
		return retValue;
	}

	public boolean supportsOrderByUnrelated() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsOrderByUnrelated()");
		}

		retValue = dbMD_.supportsOrderByUnrelated();
		return retValue;
	}

	public boolean supportsGroupBy() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsGroupBy()");
		}

		retValue = dbMD_.supportsGroupBy();
		return retValue;
	}

	public boolean supportsGroupByUnrelated() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsGroupByUnrelated()");
		}

		retValue = dbMD_.supportsGroupByUnrelated();
		return retValue;
	}

	public boolean supportsGroupByBeyondSelect() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsGroupByBeyondSelect()");
		}

		retValue = dbMD_.supportsGroupByBeyondSelect();
		return retValue;
	}

	public boolean supportsLikeEscapeClause() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsLikeEscapeClause()");
		}

		retValue = dbMD_.supportsLikeEscapeClause();
		return retValue;
	}

	public boolean supportsMultipleResultSets() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsMultipleResultSets()");
		}

		retValue = dbMD_.supportsMultipleResultSets();
		return retValue;
	}

	public boolean supportsMultipleTransactions() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsMultipleTransactions()");
		}

		retValue = dbMD_.supportsMultipleTransactions();
		return retValue;
	}

	public boolean supportsNamedParameters() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsNamedParameters()");
		}

		retValue = dbMD_.supportsNamedParameters();
		return retValue;
	}

	public boolean supportsNonNullableColumns() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsNonNullableColumns()");
		}

		retValue = dbMD_.supportsNonNullableColumns();
		return retValue;
	}

	public boolean supportsMinimumSQLGrammar() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsMinimumSQLGrammar()");
		}

		retValue = dbMD_.supportsMinimumSQLGrammar();
		return retValue;
	}

	public boolean supportsCoreSQLGrammar() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsCoreSQLGrammar()");
		}

		retValue = dbMD_.supportsCoreSQLGrammar();
		return retValue;
	}

	public boolean supportsExtendedSQLGrammar() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsExtendedSQLGrammar()");
		}

		retValue = dbMD_.supportsExtendedSQLGrammar();
		return retValue;
	}

	public boolean supportsANSI92EntryLevelSQL() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsANSI92EntryLevelSQL()");
		}

		retValue = dbMD_.supportsANSI92EntryLevelSQL();
		return retValue;
	}

	public boolean supportsANSI92IntermediateSQL() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsANSI92IntermediateSQL()");
		}

		retValue = dbMD_.supportsANSI92IntermediateSQL();
		return retValue;
	}

	public boolean supportsANSI92FullSQL() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsANSI92FullSQL()");
		}

		retValue = dbMD_.supportsANSI92FullSQL();
		return retValue;
	}

	public boolean supportsIntegrityEnhancementFacility() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId()
					+ "supportsIntegrityEnhancementFacility()");
		}

		retValue = dbMD_.supportsIntegrityEnhancementFacility();
		return retValue;
	}

	public boolean supportsOuterJoins() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsOuterJoins()");
		}

		retValue = dbMD_.supportsOuterJoins();
		return retValue;
	}

	public boolean supportsFullOuterJoins() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsFullOuterJoins()");
		}

		retValue = dbMD_.supportsFullOuterJoins();
		return retValue;
	}

	public boolean supportsLimitedOuterJoins() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsLimitedOuterJoins()");
		}

		retValue = dbMD_.supportsLimitedOuterJoins();
		return retValue;
	}

	public String getSchemaTerm() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getSchemaTerm()");
		}

		retValue = dbMD_.getSchemaTerm();
		return retValue;
	}

	public String getProcedureTerm() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getProcedureTerm()");
		}

		retValue = dbMD_.getProcedureTerm();
		return retValue;
	}

	public String getCatalogTerm() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getCatalogTerm()");
		}

		retValue = dbMD_.getCatalogTerm();
		return retValue;
	}

	public boolean isCatalogAtStart() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "isCatalogAtStart()");
		}

		retValue = dbMD_.isCatalogAtStart();
		return retValue;
	}

	public String getCatalogSeparator() throws SQLException {
		String retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getCatalogSeparator()");
		}

		retValue = dbMD_.getCatalogSeparator();
		return retValue;
	}

	public boolean supportsSchemasInDataManipulation() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsSchemasInDataManipulation()");
		}

		retValue = dbMD_.supportsSchemasInDataManipulation();
		return retValue;
	}

	public boolean supportsSchemasInProcedureCalls() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsSchemasInProcedureCalls()");
		}

		retValue = dbMD_.supportsSchemasInProcedureCalls();
		return retValue;
	}

	public boolean supportsSchemasInTableDefinitions() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsSchemasInTableDefinitions()");
		}

		retValue = dbMD_.supportsSchemasInTableDefinitions();
		return retValue;
	}

	public boolean supportsSchemasInIndexDefinitions() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsSchemasInIndexDefinitions()");
		}

		retValue = dbMD_.supportsSchemasInIndexDefinitions();
		return retValue;
	}

	public boolean supportsSchemasInPrivilegeDefinitions() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId()
					+ "supportsSchemasInPrivilegeDefinitions()");
		}

		retValue = dbMD_.supportsSchemasInPrivilegeDefinitions();
		return retValue;
	}

	public boolean supportsCatalogsInDataManipulation() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsCatalogsInDataManipulation()");
		}

		retValue = dbMD_.supportsCatalogsInDataManipulation();
		return retValue;
	}

	public boolean supportsCatalogsInProcedureCalls() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsCatalogsInProcedureCalls()");
		}

		retValue = dbMD_.supportsCatalogsInProcedureCalls();
		return retValue;
	}

	public boolean supportsCatalogsInTableDefinitions() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsCatalogsInTableDefinitions()");
		}

		retValue = dbMD_.supportsCatalogsInTableDefinitions();
		return retValue;
	}

	public boolean supportsCatalogsInIndexDefinitions() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsCatalogsInIndexDefinitions()");
		}

		retValue = dbMD_.supportsCatalogsInIndexDefinitions();
		return retValue;
	}

	public boolean supportsCatalogsInPrivilegeDefinitions() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId()
					+ "supportsCatalogsInPrivilegeDefinitions()");
		}

		retValue = dbMD_.supportsCatalogsInPrivilegeDefinitions();
		return retValue;
	}

	public boolean supportsPositionedDelete() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsPositionedDelete()");
		}

		retValue = dbMD_.supportsPositionedDelete();
		return retValue;
	}

	public boolean supportsPositionedUpdate() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsPositionedUpdate()");
		}

		retValue = dbMD_.supportsPositionedUpdate();
		return retValue;
	}

	public boolean supportsSelectForUpdate() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsSelectForUpdate()");
		}

		retValue = dbMD_.supportsSelectForUpdate();
		return retValue;
	}

	public boolean supportsStoredProcedures() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsStoredProcedures()");
		}

		retValue = dbMD_.supportsStoredProcedures();
		return retValue;
	}

	public boolean supportsSubqueriesInComparisons() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsSubqueriesInComparisons()");
		}

		retValue = dbMD_.supportsSubqueriesInComparisons();
		return retValue;
	}

	public boolean supportsSubqueriesInExists() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsSubqueriesInExists()");
		}

		retValue = dbMD_.supportsSubqueriesInExists();
		return retValue;
	}

	public boolean supportsSubqueriesInIns() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsSubqueriesInIns()");
		}

		retValue = dbMD_.supportsSubqueriesInIns();
		return retValue;
	}

	public boolean supportsSubqueriesInQuantifieds() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsSubqueriesInQuantifieds()");
		}

		retValue = dbMD_.supportsSubqueriesInQuantifieds();
		return retValue;
	}

	public boolean supportsCorrelatedSubqueries() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsCorrelatedSubqueries()");
		}

		retValue = dbMD_.supportsCorrelatedSubqueries();
		return retValue;
	}

	public boolean supportsUnion() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsUnion()");
		}

		retValue = dbMD_.supportsUnion();
		return retValue;
	}

	public boolean supportsUnionAll() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsUnionAll()");
		}

		retValue = dbMD_.supportsUnionAll();
		return retValue;
	}

	public boolean supportsOpenCursorsAcrossCommit() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsOpenCursorsAcrossCommit()");
		}

		retValue = dbMD_.supportsOpenCursorsAcrossCommit();
		return retValue;
	}

	public boolean supportsOpenCursorsAcrossRollback() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsOpenCursorsAcrossRollback()");
		}

		retValue = dbMD_.supportsOpenCursorsAcrossRollback();
		return retValue;
	}

	public boolean supportsOpenStatementsAcrossCommit() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsOpenStatementsAcrossCommit()");
		}

		retValue = dbMD_.supportsOpenStatementsAcrossCommit();
		return retValue;
	}

	public boolean supportsOpenStatementsAcrossRollback() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId()
					+ "supportsOpenStatementsAcrossRollback()");
		}

		retValue = dbMD_.supportsOpenStatementsAcrossRollback();
		return retValue;
	}

	public int getMaxBinaryLiteralLength() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxBinaryLiteralLength()");
		}

		retValue = dbMD_.getMaxBinaryLiteralLength();
		return retValue;
	}

	public int getMaxCharLiteralLength() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxCharLiteralLength()");
		}

		retValue = dbMD_.getMaxCharLiteralLength();
		return retValue;
	}

	public int getMaxColumnNameLength() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxColumnNameLength()");
		}

		retValue = dbMD_.getMaxColumnNameLength();
		return retValue;
	}

	public int getMaxColumnsInGroupBy() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxColumnsInGroupBy()");
		}

		retValue = dbMD_.getMaxColumnsInGroupBy();
		return retValue;
	}

	public int getMaxColumnsInIndex() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxColumnsInIndex()");
		}

		retValue = dbMD_.getMaxColumnsInIndex();
		return retValue;
	}

	public int getMaxColumnsInOrderBy() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxColumnsInOrderBy()");
		}

		retValue = dbMD_.getMaxColumnsInOrderBy();
		return retValue;
	}

	public int getMaxColumnsInSelect() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxColumnsInSelect()");
		}

		retValue = dbMD_.getMaxColumnsInSelect();
		return retValue;
	}

	public int getMaxColumnsInTable() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxColumnsInTable()");
		}

		retValue = dbMD_.getMaxColumnsInTable();
		return retValue;
	}

	public int getMaxConnections() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxConnections()");
		}

		retValue = dbMD_.getMaxConnections();
		return retValue;
	}

	public int getMaxCursorNameLength() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxCursorNameLength()");
		}

		retValue = dbMD_.getMaxCursorNameLength();
		return retValue;
	}

	public int getMaxIndexLength() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxIndexLength()");
		}

		retValue = dbMD_.getMaxIndexLength();
		return retValue;
	}

	public int getMaxSchemaNameLength() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxSchemaNameLength()");
		}

		retValue = dbMD_.getMaxSchemaNameLength();
		return retValue;
	}

	public int getMaxProcedureNameLength() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxProcedureNameLength()");
		}

		retValue = dbMD_.getMaxProcedureNameLength();
		return retValue;
	}

	public int getMaxCatalogNameLength() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxCatalogNameLength()");
		}

		retValue = dbMD_.getMaxCatalogNameLength();
		return retValue;
	}

	public int getMaxRowSize() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxRowSize()");
		}

		retValue = dbMD_.getMaxRowSize();
		return retValue;
	}

	public boolean doesMaxRowSizeIncludeBlobs() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "doesMaxRowSizeIncludeBlobs()");
		}

		retValue = dbMD_.doesMaxRowSizeIncludeBlobs();
		return retValue;
	}

	public int getMaxStatementLength() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxStatementLength()");
		}

		retValue = dbMD_.getMaxStatementLength();
		return retValue;
	}

	public int getMaxStatements() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxStatements()");
		}

		retValue = dbMD_.getMaxStatements();
		return retValue;
	}

	public int getMaxTableNameLength() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxTableNameLength()");
		}

		retValue = dbMD_.getMaxTableNameLength();
		return retValue;
	}

	public int getMaxTablesInSelect() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxTablesInSelect()");
		}

		retValue = dbMD_.getMaxTablesInSelect();
		return retValue;
	}

	public int getMaxUserNameLength() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getMaxUserNameLength()");
		}

		retValue = dbMD_.getMaxUserNameLength();
		return retValue;
	}

	public int getDefaultTransactionIsolation() throws SQLException {
		int retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "getDefaultTransactionIsolation()");
		}

		retValue = dbMD_.getDefaultTransactionIsolation();
		return retValue;
	}

	public boolean supportsTransactions() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsTransactions()");
		}

		retValue = dbMD_.supportsTransactions();
		return retValue;
	}

	public boolean supportsTransactionIsolationLevel(int level)
			throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsTransactionIsolationLevel("
					+ level + ")");
		}

		retValue = dbMD_.supportsTransactionIsolationLevel(level);
		return retValue;
	}

	public boolean supportsDataDefinitionAndDataManipulationTransactions()
			throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_
					.println(getTraceId()
							+ "supportsDataDefinitionAndDataManipulationTransactions()");
		}

		retValue = dbMD_
				.supportsDataDefinitionAndDataManipulationTransactions();
		return retValue;
	}

	public boolean supportsDataManipulationTransactionsOnly()
			throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId()
					+ "supportsDataManipulationTransactionsOnly()");
		}

		retValue = dbMD_.supportsDataManipulationTransactionsOnly();
		return retValue;
	}

	public boolean dataDefinitionCausesTransactionCommit() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId()
					+ "dataDefinitionCausesTransactionCommit()");
		}

		retValue = dbMD_.dataDefinitionCausesTransactionCommit();
		return retValue;
	}

	public boolean dataDefinitionIgnoredInTransactions() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_
					.println(getTraceId()
							+ "dataDefinitionIgnoredInTransactions()");
		}

		retValue = dbMD_.dataDefinitionIgnoredInTransactions();
		return retValue;
	}

	public java.sql.ResultSet getProcedures(String catalog,
			String schemaPattern, String procedureNamePattern)
			throws SQLException {
		ResultSet rs;

		if (out_ != null) {
			out_.println(getTraceId() + "getProcedures(" + catalog + ","
					+ schemaPattern + "," + procedureNamePattern + ")");
		}

		rs = dbMD_.getProcedures(catalog, schemaPattern, procedureNamePattern);

		if (out_ != null) {
			out_.println(getTraceId() + "getProcedures(" + catalog + ","
					+ schemaPattern + "," + procedureNamePattern
					+ ") returns ResultSet [" + System.identityHashCode(rs)
					+ "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public java.sql.ResultSet getProcedureColumns(String catalog,
			String schemaPattern, String procedureNamePattern,
			String columnNamePattern) throws SQLException {
		ResultSet rs;

		if (out_ != null) {
			out_.println(getTraceId() + "getProcedureColumns(" + catalog + ","
					+ schemaPattern + "," + procedureNamePattern + ","
					+ columnNamePattern + ")");
		}

		rs = dbMD_.getProcedureColumns(catalog, schemaPattern,
				procedureNamePattern, columnNamePattern);

		if (out_ != null) {
			out_.println(getTraceId() + "getProcedureColumns(" + catalog + ","
					+ schemaPattern + "," + procedureNamePattern + ","
					+ columnNamePattern + ") returns ResultSet ["
					+ System.identityHashCode(rs) + "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public java.sql.ResultSet getTables(String catalog, String schemaPattern,
			String tableNamePattern, String types[]) throws SQLException {
		ResultSet rs;

		if (out_ != null) {
			out_.println(getTraceId() + "getTables(" + catalog + ","
					+ schemaPattern + "," + tableNamePattern + "," + types
					+ ")");
		}

		rs = dbMD_.getTables(catalog, schemaPattern, tableNamePattern, types);

		if (out_ != null) {
			out_.println(getTraceId() + "getTables(" + catalog + ","
					+ schemaPattern + "," + tableNamePattern + "," + types
					+ ") returns ResultSet [" + System.identityHashCode(rs)
					+ "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public java.sql.ResultSet getSchemas() throws SQLException {
		ResultSet rs;

		if (out_ != null) {
			out_.println(getTraceId() + "getSchemas()");
		}

		rs = dbMD_.getSchemas();

		if (out_ != null) {
			out_.println(getTraceId() + "getSchemas() returns ResultSet ["
					+ System.identityHashCode(rs) + "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public java.sql.ResultSet getCatalogs() throws SQLException {
		ResultSet rs;

		if (out_ != null) {
			out_.println(getTraceId() + "getCatalogs()");
		}

		rs = dbMD_.getCatalogs();

		if (out_ != null) {
			out_.println(getTraceId() + "getCatalogs() returns ResultSet ["
					+ System.identityHashCode(rs) + "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public java.sql.ResultSet getTableTypes() throws SQLException {
		ResultSet rs;

		if (out_ != null) {
			out_.println(getTraceId() + "getTableTypes()");
		}

		rs = dbMD_.getTableTypes();

		if (out_ != null) {
			out_.println(getTraceId() + "getTableTypes() returns ResultSet ["
					+ System.identityHashCode(rs) + "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public java.sql.ResultSet getColumns(String catalog, String schemaPattern,
			String tableNamePattern, String columnNamePattern)
			throws SQLException {
		ResultSet rs;

		if (out_ != null) {
			out_.println(getTraceId() + "getColumns(" + catalog + ","
					+ schemaPattern + "," + tableNamePattern + ","
					+ columnNamePattern + ")");
		}

		rs = dbMD_.getColumns(catalog, schemaPattern, tableNamePattern,
				columnNamePattern);

		if (out_ != null) {
			out_.println(getTraceId() + "getColumns(" + catalog + ","
					+ schemaPattern + "," + tableNamePattern + ","
					+ columnNamePattern + ") returns ResultSet ["
					+ System.identityHashCode(rs) + "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public java.sql.ResultSet getColumnPrivileges(String catalog,
			String schema, String table, String columnNamePattern)
			throws SQLException {
		ResultSet rs;

		if (out_ != null) {
			out_.println(getTraceId() + "getColumnPrivileges(" + catalog + ","
					+ schema + "," + table + "," + columnNamePattern + ")");
		}

		rs = dbMD_.getColumnPrivileges(catalog, schema, table,
				columnNamePattern);

		if (out_ != null) {
			out_.println(getTraceId() + "getColumnPrivileges(" + catalog + ","
					+ schema + "," + table + "," + columnNamePattern
					+ ") returns ResultSet [" + System.identityHashCode(rs)
					+ "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public java.sql.ResultSet getTablePrivileges(String catalog,
			String schemaPattern, String tableNamePattern) throws SQLException {
		ResultSet rs;

		if (out_ != null) {
			out_.println(getTraceId() + "getTablePrivileges(" + catalog + ","
					+ schemaPattern + "," + tableNamePattern + ")");
		}

		rs = dbMD_.getTablePrivileges(catalog, schemaPattern, tableNamePattern);

		if (out_ != null) {
			out_.println(getTraceId() + "getTablePrivileges(" + catalog + ","
					+ schemaPattern + "," + tableNamePattern
					+ ") returns ResultSet [" + System.identityHashCode(rs)
					+ "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public java.sql.ResultSet getBestRowIdentifier(String catalog,
			String schema, String table, int scope, boolean nullable)
			throws SQLException {

		ResultSet rs;

		if (out_ != null) {
			out_
					.println(getTraceId() + "getBestRowIdentifier(" + catalog
							+ "," + schema + "," + table + "," + scope + ","
							+ nullable + ")");
		}

		rs = dbMD_
				.getBestRowIdentifier(catalog, schema, table, scope, nullable);

		if (out_ != null) {
			out_.println(getTraceId() + "getBestRowIdentifier(" + catalog + ","
					+ schema + "," + table + "," + scope + "," + nullable
					+ ") returns ResultSet [" + System.identityHashCode(rs)
					+ "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public java.sql.ResultSet getVersionColumns(String catalog, String schema,
			String table) throws SQLException {
		ResultSet rs;

		if (out_ != null) {
			out_.println(getTraceId() + "getVersionColumns(" + catalog + ","
					+ schema + "," + table + ")");
		}

		rs = dbMD_.getVersionColumns(catalog, schema, table);

		if (out_ != null) {
			out_.println(getTraceId() + "getVersionColumns(" + catalog + ","
					+ schema + "," + table + ") returns ResultSet ["
					+ System.identityHashCode(rs) + "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public java.sql.ResultSet getPrimaryKeys(String catalog, String schema,
			String table) throws SQLException {

		ResultSet rs;

		if (out_ != null) {
			out_.println(getTraceId() + "getPrimaryKeys(" + catalog + ","
					+ schema + "," + table + ")");
		}

		rs = dbMD_.getPrimaryKeys(catalog, schema, table);

		if (out_ != null) {
			out_.println(getTraceId() + "getPrimaryKeys(" + catalog + ","
					+ schema + "," + table + ") returns ResultSet ["
					+ System.identityHashCode(rs) + "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public java.sql.ResultSet getImportedKeys(String catalog, String schema,
			String table) throws SQLException {
		ResultSet rs;

		if (out_ != null) {
			out_.println(getTraceId() + "getImportedKeys(" + catalog + ","
					+ schema + "," + table + ")");
		}

		rs = dbMD_.getImportedKeys(catalog, schema, table);

		if (out_ != null) {
			out_.println(getTraceId() + "getImportedKeys(" + catalog + ","
					+ schema + "," + table + ") returns ResultSet ["
					+ System.identityHashCode(rs) + "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public java.sql.ResultSet getExportedKeys(String catalog, String schema,
			String table) throws SQLException {
		ResultSet rs;

		if (out_ != null) {
			out_.println(getTraceId() + "getExportedKeys(" + catalog + ","
					+ schema + "," + table + ")");
		}

		rs = dbMD_.getExportedKeys(catalog, schema, table);

		if (out_ != null) {
			out_.println(getTraceId() + "getExportedKeys(" + catalog + ","
					+ schema + "," + table + ") returns ResultSet ["
					+ System.identityHashCode(rs) + "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public java.sql.ResultSet getCrossReference(String primaryCatalog,
			String primarySchema, String primaryTable, String foreignCatalog,
			String foreignSchema, String foreignTable) throws SQLException {

		ResultSet rs;

		if (out_ != null) {
			out_.println(getTraceId() + "getCrossReference(" + primaryCatalog
					+ "," + primarySchema + "," + primaryTable + ","
					+ foreignCatalog + "," + foreignSchema + "," + foreignTable
					+ ")");
		}

		rs = dbMD_.getCrossReference(primaryCatalog, primarySchema,
				primaryTable, foreignCatalog, foreignSchema, foreignTable);

		if (out_ != null) {
			out_.println(getTraceId() + "getCrossReference(" + primaryCatalog
					+ "," + primarySchema + "," + primaryTable + ","
					+ foreignCatalog + "," + foreignSchema + "," + foreignTable
					+ ") returns ResultSet [" + System.identityHashCode(rs)
					+ "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public java.sql.ResultSet getTypeInfo() throws SQLException {
		ResultSet rs;

		if (out_ != null) {
			out_.println(getTraceId() + "getTypeInfo()");
		}

		rs = dbMD_.getTypeInfo();

		if (out_ != null) {
			out_.println(getTraceId() + "getTypeInfo() returns ResultSet ["
					+ System.identityHashCode(rs) + "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public java.sql.ResultSet getIndexInfo(String catalog, String schema,
			String table, boolean unique, boolean approximate)
			throws SQLException {
		ResultSet rs;

		if (out_ != null) {
			out_.println(getTraceId() + "getIndexInfo(" + catalog + ", "
					+ schema + ", " + table + ", " + unique + ", "
					+ approximate + ")");
		}

		rs = dbMD_.getIndexInfo(catalog, schema, table, unique, approximate);

		if (out_ != null) {
			out_.println(getTraceId() + "getIndexInfo(" + catalog + ", "
					+ schema + ", " + table + ", " + unique + ", "
					+ approximate + ") returns ResultSet ["
					+ System.identityHashCode(rs) + "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public boolean deletesAreDetected(int type) throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "deletesAreDetected(" + type + ")");
		}

		retValue = dbMD_.deletesAreDetected(type);
		return retValue;
	}

	public java.sql.Connection getConnection() throws SQLException {
		Connection connect;

		if (out_ != null) {
			out_.println(getTraceId() + "getConnection()");
		}

		connect = dbMD_.getConnection();

		if (out_ != null) {
			out_.println(getTraceId() + "getConnection() returns Connection ["
					+ System.identityHashCode(connect) + "]");
		}

		// But return the existing TConnection
		return connect_;
	}

	public java.sql.ResultSet getUDTs(String catalog, String schemaPattern,
			String typeNamePattern, int[] types) throws SQLException {
		ResultSet rs;

		if (out_ != null) {
			out_
					.println(getTraceId() + "getUDTs(" + catalog + ","
							+ schemaPattern + "," + typeNamePattern + ","
							+ types + ")");
		}

		rs = dbMD_.getUDTs(catalog, schemaPattern, typeNamePattern, types);

		if (out_ != null) {
			out_.println(getTraceId() + "getUDTs(" + catalog + ","
					+ schemaPattern + "," + typeNamePattern + "," + types
					+ ") returns ResultSet [" + System.identityHashCode(rs)
					+ "]");
		}

		if (rs instanceof SQLMXResultSet) {
			((SQLMXResultSet) rs).setTracer(out_);
		}
		return rs;
	}

	public boolean insertsAreDetected(int type) throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "insertsAreDetected(" + type + ")");
		}

		retValue = dbMD_.insertsAreDetected(type);
		return retValue;
	}

	public boolean updatesAreDetected(int type) throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "updatesAreDetected(" + type + ")");
		}

		retValue = dbMD_.updatesAreDetected(type);
		return retValue;
	}

	public boolean othersDeletesAreVisible(int type) throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_
					.println(getTraceId() + "othersDeletesAreVisible(" + type
							+ ")");
		}

		retValue = dbMD_.othersDeletesAreVisible(type);
		return retValue;
	}

	public boolean othersInsertsAreVisible(int type) throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_
					.println(getTraceId() + "othersInsertsAreVisible(" + type
							+ ")");
		}

		retValue = dbMD_.othersInsertsAreVisible(type);
		return retValue;
	}

	public boolean othersUpdatesAreVisible(int type) throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_
					.println(getTraceId() + "othersUpdatesAreVisible(" + type
							+ ")");
		}

		retValue = dbMD_.othersUpdatesAreVisible(type);
		return retValue;
	}

	public boolean ownDeletesAreVisible(int type) throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "ownDeletesAreVisible(" + type + ")");
		}

		retValue = dbMD_.ownDeletesAreVisible(type);
		return retValue;
	}

	public boolean ownInsertsAreVisible(int type) throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "ownInsertsAreVisible(" + type + ")");
		}

		retValue = dbMD_.ownInsertsAreVisible(type);
		return retValue;
	}

	public boolean ownUpdatesAreVisible(int type) throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "ownUpdatesAreVisible(" + type + ")");
		}

		retValue = dbMD_.ownUpdatesAreVisible(type);
		return retValue;
	}

	public boolean supportsBatchUpdates() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsBatchUpdates()");
		}

		retValue = dbMD_.supportsBatchUpdates();
		return retValue;
	}

	public boolean supportsResultSetType(int type) throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsResultSetType(" + type + ")");
		}

		retValue = dbMD_.supportsResultSetType(type);
		return retValue;
	}

	public boolean supportsResultSetConcurrency(int type, int concurrency)
			throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsResultSetConcurrency(" + type
					+ "," + concurrency + ")");
		}

		retValue = dbMD_.supportsResultSetConcurrency(type, concurrency);
		return retValue;
	}

	public boolean supportsSavepoints() throws SQLException {
		boolean retValue;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsSavepoint()");
		}

		retValue = dbMD_.supportsSavepoints();
		return retValue;
	}

	public ResultSet getAttributes(String str, String str1, String str2,
			String str3) throws SQLException {
		ResultSet result;

		if (out_ != null) {
			out_.println(getTraceId() + "getAttributes(" + str + "," + str1
					+ "," + str2 + "," + str3 + ")");
		}

		result = dbMD_.getAttributes(str, str1, str2, str3);

		if (out_ != null) {
			out_.println(getTraceId() + "getAttributes(" + str + "," + str1
					+ "," + str2 + "," + str3 + ") returns ResultSet ["
					+ System.identityHashCode(result) + "]");
		}

		return result;
	}

	public int getDatabaseMajorVersion() throws SQLException {
		int result;

		if (out_ != null) {
			out_.println(getTraceId() + "getDatabaseMajorVersion()");
		}

		result = dbMD_.getDatabaseMajorVersion();

		return result;
	}

	public int getDatabaseMinorVersion() throws SQLException {
		int result;

		if (out_ != null) {
			out_.println(getTraceId() + "getDatabaseMinorVersion()");
		}

		result = dbMD_.getDatabaseMinorVersion();

		return result;
	}

	public int getJDBCMajorVersion() throws SQLException {
		int result;

		if (out_ != null) {
			out_.println(getTraceId() + "getJDBCMajorVersion()");
		}

		result = dbMD_.getJDBCMajorVersion();

		return result;
	}

	public int getJDBCMinorVersion() throws SQLException {
		int result;

		if (out_ != null) {
			out_.println(getTraceId() + "getJDBCMinorVersion()");
		}

		result = dbMD_.getJDBCMinorVersion();

		return result;
	}

	public int getResultSetHoldability() throws SQLException {
		int result;

		if (out_ != null) {
			out_.println(getTraceId() + "getResultSetHoldability()");
		}

		result = dbMD_.getResultSetHoldability();

		return result;
	}

	public int getSQLStateType() throws SQLException {
		int result;

		if (out_ != null) {
			out_.println(getTraceId() + "getSQLStateType()");
		}

		result = dbMD_.getSQLStateType();

		return result;
	}

	public ResultSet getSuperTables(String str, String str1, String str2)
			throws SQLException {
		ResultSet result;

		if (out_ != null) {
			out_.println(getTraceId() + "getSuperTables(" + str + "," + str1
					+ "," + str2 + ")");
		}

		result = dbMD_.getSuperTables(str, str1, str2);

		if (out_ != null) {
			out_.println(getTraceId() + "getSuperTables(" + str + "," + str1
					+ "," + str2 + ") returns ResultSet ["
					+ System.identityHashCode(result) + "]");
		}

		return result;
	}

	public ResultSet getSuperTypes(String str, String str1, String str2)
			throws SQLException {
		ResultSet result;

		if (out_ != null) {
			out_.println(getTraceId() + "getSuperTables(" + str + "," + str1
					+ "," + str2 + ")");
		}

		result = dbMD_.getSuperTables(str, str1, str2);

		if (out_ != null) {
			out_.println(getTraceId() + "getSuperTables(" + str + "," + str1
					+ "," + str2 + ") returns ResultSet ["
					+ System.identityHashCode(result) + "]");
		}

		return result;
	}

	public boolean locatorsUpdateCopy() throws SQLException {
		boolean result;

		if (out_ != null) {
			out_.println(getTraceId() + "locatorsUpdateCopy()");
		}

		result = dbMD_.locatorsUpdateCopy();

		return result;
	}

	public boolean supportsGetGeneratedKeys() throws SQLException {
		boolean result;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsGetGeneratedKeys()");
		}

		result = dbMD_.supportsGetGeneratedKeys();

		return result;
	}

	public boolean supportsMultipleOpenResults() throws SQLException {
		boolean result;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsMultipleOpenResults()");
		}

		result = dbMD_.supportsMultipleOpenResults();

		return result;
	}

	public boolean supportsResultSetHoldability(int param) throws SQLException {
		boolean result;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsResultSetHoldability(" + param
					+ ")");
		}

		result = dbMD_.supportsResultSetHoldability(param);

		return result;
	}

	public boolean supportsStatementPooling() throws SQLException {
		boolean result;

		if (out_ != null) {
			out_.println(getTraceId() + "supportsStatementPooling()");
		}

		result = dbMD_.supportsStatementPooling();

		return result;
	}

	// Constructors
	public TDatabaseMetaData(DatabaseMetaData dbMD, TConnection connect,
			PrintWriter out) {

		dbMD_ = dbMD;
		connect_ = connect;
		out_ = out;

	}

	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		String className = null;
		if (dbMD_ != null)
			className = dbMD_.getClass().getName();

		// Build up jdbcTrace output entry
		setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText
				+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date())
				+ "]:["
				+ Thread.currentThread()
				+ "]:["
				+ System.identityHashCode(dbMD_)
				+ "]:"
				+ className.substring(
						org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME, className
								.length()) + ".");
		return traceId_;
	}

	//Fields
	DatabaseMetaData dbMD_;
	TConnection connect_;
	private String traceId_;
	PrintWriter out_;


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
