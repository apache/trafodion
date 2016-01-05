/**************************************************************************
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
**************************************************************************/
//
// MODULE: SrvrSMD.cpp
//
// PURPOSE: Implements the following methods
//	odbc_SQLSvc_GetSQLCatalogs_sme_
/**************************************************************************/

#include <sql.h>
#include <stdlib.h>
#include "CoreCommon.h"
#include "SrvrCommon.h"
#include "SrvrFunctions.h"
#include "SrvrKds.h"
#include "SrvrOthers.h"
#include "SqlInterface.h"
#include "CommonDiags.h"
#include "Debug.h"

enum TABLE_INDEX {
	CATSYS = 0,
	SCHEMATA,
	ACCESS_PATHS,
	ACCESS_PATH_COLS,
	COLS,
	COL_PRIVILEGES,
	KEY_COL_USAGE,
	OBJECTS,
	REF_CONSTRAINTS,
	TBL_PRIVILEGES,
	SCH_PRIVILEGES, //added for R3.0 - senthil
	VW_COL_TBLS,//added for R3.0
	SYNONYM_USAGE, //added for r3.0
	TBL_CONSTRAINTS,
	DATATYPES,
	MP_PARTITIONS,		// Used for SQL/MX 2.0 Metadata
	RI_UNIQUE_USAGE,	// Used for Foreign Keys Metadata
	MPALIAS,
	ZODT,
	COLUMNS,
	KEYS,
	INDEXES,
	TABLES,
	CATALOGS,
	PROCS,
	PARAMS
};

enum SCHEMA_INDEX {
	DEF_SCH,
	SYS_SCH,
	ODBC_SCH
};

// String list
char *smdSchemaList[] = {
"DEFINITION_SCHEMA_VERSION_", // SCHEMA_VERSION appended define in srvrfunctions.h
"SYSTEM_SCHEMA.",
"MXCS_SCHEMA."
};

char *smdTablesList[] = {
	"CATSYS",
	"SCHEMATA",
	"ACCESS_PATHS",
	"ACCESS_PATH_COLS",
	"COLS",
	"COL_PRIVILEGES",
	"KEY_COL_USAGE",
	"OBJECTS",
	"REF_CONSTRAINTS",
	"TBL_PRIVILEGES",
	"SCH_PRIVILEGES", //added for R3.0 - senthil
	"VW_COL_TBLS",  // added for R3.0
	"SYNONYM_USAGE", // "
	"TBL_CONSTRAINTS",
	"DATATYPES",
	"MP_PARTITIONS",	// Used for SQL/MX 2.0 Metadata
	"RI_UNIQUE_USAGE",	// Used for Foreign Keys Metadata
	".MPALIAS",
	".ZODT",
	".COLUMNS",
	".KEYS",
	".INDEXES",
	".TABLES",
	".CATALOGS",
	".PROCS",
	".PARAMS"
};

SMD_QUERY_TABLE smdQueryTable[] = {
	{NULL}
};

/*
 * Synchronous method function prototype for
 * operation 'odbc_SQLSvc_GetSQLCatalogs'
 */
void odbc_SQLSvc_GetSQLCatalogs_sme_(
	/* In	*/ void * objtag_
  , /* In	*/ const CEE_handle_def *call_id_
  , /* Out   */ ExceptionStruct *exception_
  , /* In	*/ long dialogueId
  , /* In	*/ short APIType
  , /* In	*/ const char *catalogNm
  , /* In	*/ const char *schemaNm
  , /* In	*/ const char *tableNm
  , /* In	*/ const char *tableTypeList
  , /* In	*/ const char *columnNm
  , /* In	*/ long columnType
  , /* In	*/ long rowIdScope
  , /* In	*/ long nullable
  , /* In	*/ long uniqueness
  , /* In	*/ long accuracy
  , /* In	*/ short sqlType
  , /* In	*/ unsigned long metadataId
  , /* Out   */ char *catStmtLabel
  , /* Out   */ SQLItemDescList_def *outputDesc
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
  , /* Out   */ long *rowsAffected
  , /* Out   */ SQLValueList_def *outputValueList
  , /* Out   */ long *stmtId
  , /* In    */ const char *fkcatalogNm
  , /* In    */ const char *fkschemaNm
  , /* In    */ const char *fktableNm)
{
	FUNCTION_ENTRY("odbc_SQLSvc_GetSQLCatalogs_sme_",(""));
	DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  %#x, %#x, %#x, %#x, %d, %s, %s, %s, %s, %s, %ld, %ld, %ld, %ld, %ld, %d, %#x, %#x, %#x",
		objtag_,
		call_id_,
		exception_,
		dialogueId,
		APIType,
		catalogNm,
		schemaNm,
		tableNm,
		tableTypeList,
		columnNm,
		columnType,
		rowIdScope,
		nullable,
		uniqueness,
		accuracy,
		sqlType,
		catStmtLabel,
		outputDesc,
		sqlWarning));

	SRVRTRACE_ENTER(FILE_SME+14);

	enum CATAPI_TABLE_INDEX {
		COLUMNS = 0,
		DEFAULTS,
		INDEXES,
		KEYS,
		OBJECTS,
		OBJECTUID,
		TABLES,
		VIEWS,
		VIEWS_USAGE,
		VERSIONS
		};

	char *smdCatAPITablesList[] = {
		"COLUMNS",
		"DEFAULTS",
		"INDEXES",
		"KEYS",
		"OBJECTS",
		"OBJECTUID",
		"TABLES",
		"VIEWS",
		"VIEWS_USAGE",
		"VERSIONS"
	        };

	const char *inputParam[16];
	const char *tableParam[20];
	short retCode;
    char tmpBuf[20];
	char *odbcAppVersion = "3";
	char *translationId = "3";

	ExceptionStruct					prepareException;
	CLEAR_EXCEPTION(prepareException);

	ExceptionStruct					executeException;
	CLEAR_EXCEPTION(executeException);

	ExceptionStruct					fetchException;
	CLEAR_EXCEPTION(fetchException);

	ExceptionStruct					closeException;
	CLEAR_EXCEPTION(closeException);

	char expCatalogNm[MAX_ANSI_NAME_LEN+1];
	char expSchemaNm[MAX_ANSI_NAME_LEN+1];
	char expTableNm[MAX_ANSI_NAME_LEN+1];
	char expColumnNm[MAX_ANSI_NAME_LEN+1];
	char expProcNm[MAX_ANSI_NAME_LEN+1];

	char catalogNmNoEsc[MAX_ANSI_NAME_LEN+1];
	char schemaNmNoEsc[MAX_ANSI_NAME_LEN+1];
	char tableNmNoEsc[MAX_ANSI_NAME_LEN+1];
	char columnNmNoEsc[MAX_ANSI_NAME_LEN+1];
	char procNmNoEsc[MAX_ANSI_NAME_LEN+1];

	char tableName1[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3];
	char tableName2[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3];
	char tableName3[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3];
	char tableName4[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3];
	char tableName5[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3];
	char tableName6[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3];
	char tableName7[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3];
	char tableName8[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3];

	char SQLObjType[2];
	char inParam1[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3];
	char inParam2[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3];
	char inParam3[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3]; // catalog len + '.' + schema len + '.' + table len +'\0'
	char inParam4[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3]; // catalog len + '.' + schema len + '.' + table len +'\0'

	char schemaVersion[10];					// Holds SQL schema version from SQL_SCHEMAVERSION_ANSI_Q1 module call
	char fkstmtLabel[MAX_STMT_LABEL_LEN+1];	// Used for FK methods
	long rowsMPFetched;					// # of tables to check for MP metadata
	long rowsFKFetched;					// # of rows fetched from foreign keys method query 1
	BOOL queryMP = FALSE;					// Flag whether to pull MP metadata

	SQLValueList_def tempOutputValueList;	// Intermediate and temp output value lists
	// Null out tempOutputValueList
	CLEAR_LIST(tempOutputValueList);

	long curRowNo  = 0;
	long numOfCols = 0;
	long curColNo  = 0;
	char schemaNmAct[MAX_ANSI_NAME_LEN+1];
	char tableNmAct[MAX_ANSI_NAME_LEN+1];
	char colNmAct[MAX_ANSI_NAME_LEN+1];
	char ordinalAct[10];
	char obuidAct[MAX_ANSI_NAME_LEN+1];
	char riuidAct[MAX_ANSI_NAME_LEN+1];

	short			sqlStmtType;
	SQLItemDescList_def lc_outputDesc;
	BOOL tableViewGiven = FALSE;
	BOOL systemTableGiven = FALSE;

	short namelen;

	SQLValue_def *SQLValue;
	char userCatalogNm[MAX_ANSI_NAME_LEN+1];
	char guardianNm[36]; // 8+1+8+1+8+1+8+1
	char inParam[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3]; // catalog len + '.' + schema len + '.' + table len +'\0'

	char MapDataType[2] = "0";
	tmpBuf[0] = '\0';
	fkstmtLabel[0] = '\0';
	userCatalogNm[0] = '\0';

	char catStmtLabelNew[128]  = {'\0'};  // Trying to support max module name length
	
	inputParam[0] = srvrGlobal->SystemCatalog;
	if (catalogNm == NULL)
		strcpy(catalogNmNoEsc,"");
		//strcpy(catalogNmNoEsc,srvrGlobal->DefaultCatalog);		// There is an OR condition with the catalog so it can be ""
	else
		strcpy(catalogNmNoEsc, catalogNm);
	inputParam[1] = catalogNmNoEsc;
	if (schemaNm  == NULL )
		strcpy(schemaNmNoEsc,"%");
		
	else
		strcpy(schemaNmNoEsc, schemaNm);
	if (schemaNm  != NULL )
	{
		convertWildcardNoEsc(metadataId, FALSE, schemaNm, schemaNmNoEsc);
		convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
	}
		inputParam[2] = schemaNmNoEsc;
		inputParam[3] = expSchemaNm;
	    inputParam[4] = NULL;

	sqlStmtType = TYPE_SELECT;

	DEBUG_OUT(DEBUG_LEVEL_METADATA,("SQL_SCHEMAVERSION_NEW_ANSI_Q1  tableParams= |%s|%s| inputParams= |%s|",
		tableParam[0],tableParam[1],
		inputParam[0]));

	if (APIType !=  SQL_TXN_ISOLATION)
	{
	retCode = executeAndFetchSMDQuery(objtag_, call_id_, dialogueId, APIType, "SQL_CATALOG_API",
							sqlStmtType, &tableParam[0], &inputParam[0], catalogNm, schemaNm,
							tableNm, columnNm, tableTypeList, metadataId, outputDesc, &executeException, &fetchException, sqlWarning,
							rowsAffected, outputValueList, stmtId);

	if (retCode != CEE_SUCCESS && writeServerException(retCode,exception_,&prepareException,&executeException,&fetchException) != TRUE)
	{
		odbc_SQLSvc_Close_sme_(objtag_, call_id_, &closeException, dialogueId, *stmtId,
							SQL_DROP, rowsAffected, sqlWarning);
		FUNCTION_RETURN_VOID(("executeAndFetchSMDQuery() and writeServerException() Failed"));
	}

	if (retCode == FETCH_EXCEPTION &&
		fetchException.exception_nr == odbc_SQLSvc_FetchN_SQLNoDataFound_exn_)
	{
		odbc_SQLSvc_Close_sme_(objtag_, call_id_, &closeException, dialogueId, *stmtId,
							SQL_DROP, rowsAffected, sqlWarning);
		FUNCTION_RETURN_VOID(("executeAndFetchSMDQuery() FETCH_EXCEPTION - SQLNoDataFound Expected"));
	}

	SQLValue = (SQLValue_def *)outputValueList->_buffer;
	if (SQLValue->dataInd == -1) //does not come here
	    {
			inputParam[0] = srvrGlobal->SystemCatalog;
			inputParam[2] = "SYSTEM_SCHEMA";

			memset(outputValueList, NULL, sizeof(SQLValueList_def));
			retCode = executeAndFetchSMDQuery(objtag_, call_id_, dialogueId, APIType, "SQL_SCHEMAVERSION_NEW_ANSI_Q1",
				sqlStmtType, &tableParam[0], &inputParam[0], catalogNm, schemaNm,
				tableNm, columnNm, tableTypeList, metadataId, outputDesc, &executeException, &fetchException, sqlWarning,
				rowsAffected, outputValueList, stmtId);

			if (retCode != CEE_SUCCESS && writeServerException(retCode,exception_,&prepareException,&executeException,&fetchException) != TRUE)
			{
				odbc_SQLSvc_Close_sme_(objtag_, call_id_, &closeException, dialogueId, *stmtId,
					SQL_DROP, rowsAffected, sqlWarning);
				FUNCTION_RETURN_VOID(("executeAndFetchSMDQuery() and writeServerException() Failed"));
			}

			if (retCode == FETCH_EXCEPTION &&
				fetchException.exception_nr == odbc_SQLSvc_FetchN_SQLNoDataFound_exn_)
			{
				odbc_SQLSvc_Close_sme_(objtag_, call_id_, &closeException, dialogueId, *stmtId,
					SQL_DROP, rowsAffected, sqlWarning);
				FUNCTION_RETURN_VOID(("executeAndFetchSMDQuery() and writeServerException() Failed"));
			}

			SQLValue = (SQLValue_def *)outputValueList->_buffer;
	    }
	 }

	FUNCTION_RETURN_VOID((NULL));
}
