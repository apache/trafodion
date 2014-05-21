/**************************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
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
**************************************************************************/
//
// MODULE: SrvrSMD.cpp
//
// PURPOSE: Implements the following methods
//	odbc_SQLSvc_GetSQLCatalogs_sme_
/**************************************************************************/
#include <windows.h>
#ifdef NSK_PLATFORM
	#include <sqlWin.h>
#else
	#include <sql.h>
#endif
#include <sqlext.h>
#include <stdlib.h>
#include "CoreCommon.h"
#include "SrvrCommon.h"
#include "SrvrFunctions.h"
#include "SrvrKds.h"
#include "sqlInterface.h"
#include "CommonDiags.h"
#ifdef NSK_PLATFORM
	#include "NskUtil.h"
#endif
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

static short executeAndFetchSMDQuery(
	/* In	*/ void * objtag_
  , /* In	*/ const CEE_handle_def *call_id_
  , /* In	*/ long dialogueId
  , /* In	*/ const char *stmtLabel
  , /* In	*/ short sqlStmtType
  , /* In	*/ char *tableParam[]
  , /* In	*/ char *inputParam[]
  , /* Out   */ SQLItemDescList_def *outputDesc
  , /* Out   */ ExceptionStruct *executeException
  , /* Out   */ ExceptionStruct *fetchException
  , /* Out   */ ERROR_DESC_LIST_def	*sqlWarning
  , /* Out   */ long *rowsAffected
  , /* Out   */ SQLValueList_def *outputValueList
  , /* Out   */ long *stmtId
  )
{
	FUNCTION_ENTRY("executeAndFetchSMDQuery",("..."));
	short retCode;

	retCode = do_ExecSMD(objtag_, call_id_, executeException, sqlWarning, dialogueId,
				stmtLabel, sqlStmtType, tableParam, inputParam, outputDesc, stmtId);
	if(retCode != CEE_SUCCESS)
		FUNCTION_RETURN_NUMERIC(retCode,("do_ExecSMD() Failed"));

	long rowsRead;
	SQLValueList_def fetchOutputValueList;
	memset(&fetchOutputValueList, 0, sizeof (fetchOutputValueList));
	*rowsAffected = 0;
	do
	{
		odbc_SQLSvc_FetchN_sme_(objtag_, call_id_, fetchException, dialogueId, *stmtId, SQL_MAX_COLUMNS_IN_SELECT,
								SQL_ASYNC_ENABLE_OFF, 0, &rowsRead, &fetchOutputValueList, sqlWarning);

		if (fetchException->exception_nr != CEE_SUCCESS)
			FUNCTION_RETURN_NUMERIC(FETCH_EXCEPTION,("FETCH_EXCEPTION - odbc_SQLSvc_FetchN_sme_() Failed"));

		*rowsAffected += rowsRead;
		if (rowsRead==SQL_MAX_COLUMNS_IN_SELECT) appendOutputValueList(outputValueList,&fetchOutputValueList,true);
	} while (rowsRead==SQL_MAX_COLUMNS_IN_SELECT);
	appendOutputValueList(outputValueList,&fetchOutputValueList,true);

	FUNCTION_RETURN_NUMERIC(CEE_SUCCESS,("CEE_SUCCESS"));
}

/*
 * Synchronous method function prototype for
 * operation 'odbc_SQLSvc_GetSQLCatalogs'
 */
extern "C" void
odbc_SQLSvc_GetSQLCatalogs_sme_(
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

	char lc_tableTypeList[MAX_ANSI_NAME_LEN+1];
	char *token;

	ExceptionStruct					prepareException;
	CLEAR_EXCEPTION(prepareException);

	ExceptionStruct					executeException;
	CLEAR_EXCEPTION(executeException);

	ExceptionStruct					fetchException;
	CLEAR_EXCEPTION(fetchException);

	ExceptionStruct					closeException;
	CLEAR_EXCEPTION(closeException);

	const char *inputParam[16];
	const char *tableParam[16];
	short retCode;

	char tmpBuf[20];
	char *odbcAppVersion = "3";
	char *translationId = "3";

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

#ifdef NSK_PLATFORM
	SQLValue_def *SQLValue;
	char userCatalogNm[MAX_ANSI_NAME_LEN+1];
	char guardianNm[36]; // 8+1+8+1+8+1+8+1
	char inParam[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3]; // catalog len + '.' + schema len + '.' + table len +'\0'
#endif

	char MapDataType[2] = "0";
	tmpBuf[0] = '\0';
	fkstmtLabel[0] = '\0';
	userCatalogNm[0] = '\0';

	char catStmtLabelNew[128]  = {'\0'};  // Trying to support max module name length
	// Obtain SQL/MX schema version. Setup table and input params and
	// execute SQL_SCHEMAVERSION_ANSI_Q1 module file procedure.
	strcpy(tableName1,srvrGlobal->SystemCatalog);
	strcat(tableName1, ".");
	strcat(tableName1, smdSchemaList[SYS_SCH]);
	strcat(tableName1, smdTablesList[CATSYS]);
	tableParam[0] = tableName1;
	strcpy(tableName2,srvrGlobal->SystemCatalog);
	strcat(tableName2, ".");
	strcat(tableName2, smdSchemaList[SYS_SCH]);
	strcat(tableName2, smdTablesList[SCHEMATA]);
	tableParam[1] = tableName2;
	tableParam[2] = NULL;
	// Arvind: Added for version check 2000
	// Metadata related modifications are done so that the module file
	// Can accept the catalog and schema values to identify the	exact 
	// schema version number when we specify either catalog or schema
	// When not specified it should have the system catalog 
	
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
	// Arvind: Added for version check 2000

	sqlStmtType = TYPE_SELECT;

	DEBUG_OUT(DEBUG_LEVEL_METADATA,("SQL_SCHEMAVERSION_NEW_ANSI_Q1  tableParams= |%s|%s| inputParams= |%s|",
		tableParam[0],tableParam[1],
		inputParam[0]));

	if (APIType !=  SQL_TXN_ISOLATION)
	{
	retCode = executeAndFetchSMDQuery(objtag_, call_id_, dialogueId, "SQL_SCHEMAVERSION_NEW_ANSI_Q1",
							sqlStmtType, &tableParam[0], &inputParam[0], &lc_outputDesc,
							&executeException, &fetchException, sqlWarning, rowsAffected,
							outputValueList, stmtId);

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
	if (SQLValue->dataInd == -1)
	{
		inputParam[0] = srvrGlobal->SystemCatalog;
		inputParam[2] = "SYSTEM_SCHEMA";
		
		memset(outputValueList, NULL, sizeof(SQLValueList_def));
		retCode = executeAndFetchSMDQuery(objtag_, call_id_, dialogueId, "SQL_SCHEMAVERSION_NEW_ANSI_Q1",
			sqlStmtType, &tableParam[0], &inputParam[0], &lc_outputDesc,
			&executeException, &fetchException, sqlWarning, rowsAffected,
			outputValueList, stmtId);
		
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
		
		if (SQLValue->dataInd == -1)
		{
    	exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_SCHEMA_VERSION;
		FUNCTION_RETURN_VOID(("SQLValue->dataInd == -1"));
		} 
		else
		{
			ltoa((long)(*(int *)(SQLValue->dataValue._buffer)), schemaVersion, 10);
		}

	}
	else
	{
		ltoa((long)(*(int *)(SQLValue->dataValue._buffer)), schemaVersion, 10);
	}

	// Close the schemaVersion query
	odbc_SQLSvc_Close_sme_(objtag_, call_id_, &closeException, dialogueId, *stmtId,
							SQL_DROP, rowsAffected, sqlWarning);


	// For methods that could contain MX and MP DB metadata determine
	// 1) if MP is installed (signified by srvrGlobal->NskSystemCatalogName[0] != '\0')
	// AND
	// 2) there are MP tables (aliases) to check.
	// Otherwise we skip the MP metadata check.
	// BTW: "mpQuery" will force us to skip other
	// MP processing points in this routine (initialized to FALSE).
	}

	if ( (srvrGlobal->NskSystemCatalogName[0] != '\0') &&
		 ((APIType == SQL_API_SQLCOLUMNS) || (APIType == SQL_API_SQLPRIMARYKEYS) ||
		  (APIType == SQL_API_SQLSPECIALCOLUMNS) || (APIType == SQL_API_SQLSTATISTICS)) )
	{

		if (!checkIfWildCard(catalogNm, catalogNmNoEsc))
		{
			exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
			FUNCTION_RETURN_VOID(("MP APIType Check - checkIfWildCard() Failed"));
		}

		if (strcmp(catalogNm,"") == 0)
			strcpy(tableName1,srvrGlobal->SystemCatalog);
		else
			strcpy(tableName1, catalogNm);

		strcpy(tableName2, tableName1);
		strcat(tableName2, ".");
		strcat(tableName2, smdSchemaList[DEF_SCH]);
		strcat(tableName2, schemaVersion);
		strcat(tableName2, ".");
		strcat(tableName2, smdTablesList[MP_PARTITIONS]);
		tableParam[0] = tableName2;
		strcpy(tableName3, srvrGlobal->SystemCatalog);
		strcat(tableName3, ".");
		strcat(tableName3, smdSchemaList[SYS_SCH]);
		strcat(tableName3, smdTablesList[SCHEMATA]);
		tableParam[1] = tableName3;
		convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName4);
		strcat(tableName4, ".");
		strcat(tableName4, smdSchemaList[DEF_SCH]);
		strcat(tableName4, schemaVersion);
		strcat(tableName4, ".");
		strcat(tableName4, smdTablesList[OBJECTS]);
		tableParam[2] = tableName4;
		tableParam[3] = NULL;
		sqlStmtType = TYPE_SELECT;
		convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
		convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
		convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
		convertWildcard(metadataId, TRUE, tableNm, expTableNm);

		if (strcmp(schemaNm,"") == 0)
		{	// If schema is null default to wildcard
			inputParam[0] = "%";
			inputParam[1] = "%";
		} else {
			inputParam[0] = strupr(schemaNmNoEsc);
			inputParam[1] = strupr(expSchemaNm);
		}

		inputParam[2] = strupr(tableNmNoEsc);
		inputParam[3] = strupr(expTableNm);
		inputParam[4] = NULL;


			if (strcmp(schemaVersion,"1200")==0 )
						strcpy((char *)catStmtLabelNew, "SQL_GETCATALOGNAME_Q2");
			else
						strcpy((char *)catStmtLabelNew, "SQL_GETCATALOGNAME_SCHVER_3000");


		DEBUG_OUT(DEBUG_LEVEL_METADATA,("catStmtLabelNew=|%s|  tableParams= |%s|%s|%s| inputParams= |%s|%s|%s|%s|",
			tableParam[0],tableParam[1],tableParam[2],
			inputParam[0],inputParam[1],inputParam[2],inputParam[3]));

		retCode = executeAndFetchSMDQuery(objtag_, call_id_, dialogueId, catStmtLabelNew,
						sqlStmtType, &tableParam[0], &inputParam[0], &lc_outputDesc,
						&executeException, &fetchException, sqlWarning, rowsAffected,
						&tempOutputValueList, stmtId);

	    if (retCode != CEE_SUCCESS)
		{
			if (writeServerException(retCode,exception_,&prepareException,&executeException,&fetchException) != TRUE) {
				odbc_SQLSvc_Close_sme_(objtag_, call_id_, &closeException,
					dialogueId, *stmtId, SQL_DROP, rowsAffected, sqlWarning);
				FUNCTION_RETURN_VOID(("executeAndFetchSMDQuery() and writeServerException() Failed"));
			}
		}

		// No data found from SQL_GETCATALOGNAME_Q2 query we do not need to
		// append MP metadata to output. Simply return to only report MX metadata
		if (retCode == FETCH_EXCEPTION &&
			fetchException.exception_nr == odbc_SQLSvc_FetchN_SQLNoDataFound_exn_)
		{
			queryMP = FALSE;
			odbc_SQLSvc_Close_sme_(objtag_, call_id_, &closeException,
				dialogueId, *stmtId, SQL_DROP, rowsAffected, sqlWarning);
		} else {
			queryMP = TRUE;
		}

		if( *rowsAffected > 0 ) {
			numOfCols = (long)tempOutputValueList._length/(*rowsAffected);
			rowsMPFetched = *rowsAffected;
		} else {
			rowsMPFetched = 1;
		}

		DEBUG_OUT(DEBUG_LEVEL_METADATA,("catStmtLabelNew=|%s|  queryMP=%d, rowsMPFetched=%d",
			queryMP,rowsMPFetched));

		// Close the SQL_GETCATALOGNAME_Q2 query
		odbc_SQLSvc_Close_sme_(objtag_, call_id_, &closeException,
			dialogueId, *stmtId, SQL_DROP, rowsAffected, sqlWarning);
	} // end of MP metadata query


   // Setup method tableParam and inputParam lists for retrieving MX metadata
	switch(APIType)
	{
		case SQL_API_SQLTABLES :
			if ((strcmp(catalogNm,"%") == 0) &&
				(strcmp(schemaNm,"") == 0) &&
				(strcmp(tableNm,"") == 0) &&
				(strcmp(tableTypeList,"") == 0))
			{ // getCatalogs() DatabaseMetaData method
				strcpy((char *)catStmtLabel, "SQL_JAVA_TABLES_ANSI_Q1");
				strcpy(tableName1,srvrGlobal->SystemCatalog);
				strcat(tableName1, ".");
				strcat(tableName1, smdSchemaList[SYS_SCH]);
				strcat(tableName1, smdTablesList[CATSYS]);
				tableParam[0] = tableName1;
				tableParam[1] = NULL;
				sqlStmtType = TYPE_SELECT;
				inputParam[0] = catalogNm;
				inputParam[1] = inputParam[0];
				inputParam[2] = NULL;

				DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s| inputParams= |%s|%s|",
					catStmtLabel,
					tableParam[0],
					inputParam[0],inputParam[1]));
			}
			else
			if ((strcmp(catalogNm,"") == 0) &&
				(strcmp(schemaNm,"%") == 0) &&
				(strcmp(tableNm,"") == 0) &&
				(strcmp(tableTypeList,"") == 0))
			{ // getSchemas() DatabaseMetaData method
				strcpy(catalogNmNoEsc, "%");
				strcpy((char *)catStmtLabel, "SQL_JAVA_TABLES_ANSI_Q2");
				strcpy(tableName1,srvrGlobal->SystemCatalog);
				strcat(tableName1, ".");
				strcat(tableName1, smdSchemaList[SYS_SCH]);
				strcat(tableName1, smdTablesList[CATSYS]);
				tableParam[0] = tableName1;
				strcpy(tableName2,srvrGlobal->SystemCatalog);
				strcat(tableName2, ".");
				strcat(tableName2, smdSchemaList[SYS_SCH]);
				strcat(tableName2, smdTablesList[SCHEMATA]);
				tableParam[1] = tableName2;
				tableParam[2] = NULL;
				inputParam[0] = catalogNmNoEsc;
				inputParam[1] = inputParam[1];
				inputParam[2] = schemaNm;
				inputParam[3] = inputParam[3];
				inputParam[4] = NULL;
				sqlStmtType = TYPE_SELECT;

				DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s| inputParams= |%s|%s|%s|%s|%s|",
					catStmtLabel,
					tableParam[0],tableParam[1],
					inputParam[0],inputParam[1],inputParam[2],inputParam[3],
					inputParam[4]));
			}
			else
			{ // getTables() DatabaseMetaData method
				if (tableNm[0] == '\0')
					strcpy((char *)tableNmNoEsc,"%");

				if (strcmp(schemaVersion,"1200") == 0 )
					strcpy((char *)catStmtLabel, "SQL_JAVA_TABLES_ANSI_Q3");				
				else
					strcpy((char *)catStmtLabel, "SQL_JAVA_TABLES_ANSI_Q7");

				if (! checkIfWildCard(catalogNm, expCatalogNm))
				{
					exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
					exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
					FUNCTION_RETURN_VOID(("checkIfWildCard"));
				}

				if (strcmp(catalogNm,"") == 0)
				{	// If catalog empty default to system catalog
					strcpy(tableName1,srvrGlobal->SystemCatalog);
				}	else
					{
						strncpy(tableName1,catalogNm, sizeof(tableName1));
						tableName1[sizeof(tableName1)-1] = 0;
				}

				tableParam[0] = tableName1;
						tableParam[1] = "MV";
				strcpy(tableName2, srvrGlobal->SystemCatalog);
				strcat(tableName2, ".");
				strcat(tableName2, smdSchemaList[SYS_SCH]);
				strcat(tableName2, smdTablesList[SCHEMATA]);
				tableParam[2] = tableName2;
				convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName3);
				strcat(tableName3, ".");
				strcat(tableName3, smdSchemaList[DEF_SCH]);
				strcat(tableName3, schemaVersion);
				strcat(tableName3, ".");
				strcat(tableName3, smdTablesList[OBJECTS]);
					tableParam[3] = tableName3;
					tableParam[4] = NULL;
				sqlStmtType = TYPE_SELECT;
				inputParam[0] = schemaVersion;
				convertWildcardNoEsc(metadataId, TRUE, catalogNm, catalogNmNoEsc);
				convertWildcard(metadataId, TRUE, catalogNm, expCatalogNm);
				convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
				convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
				convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
				convertWildcard(metadataId, TRUE, tableNm, expTableNm);

				if (strcmp(schemaNm,"") == 0)
				{	// If schema is null default to wildcard
					inputParam[1] = "%";
					inputParam[2] = "%";
				} else {
					inputParam[1] = strupr(schemaNmNoEsc);
					inputParam[2] = strupr(expSchemaNm);
				}

				inputParam[3] = strupr(tableNmNoEsc);
				inputParam[4] = strupr(expTableNm);

				if (tableTypeList == NULL || strlen(tableTypeList) == 0 ||
					strcmp(tableTypeList,"%") == 0)
				{
					inputParam[5]  = "UT";
					inputParam[6]  = "SM";
					inputParam[7]  = "BT";
					inputParam[8]  = "MP";
					inputParam[9]  = "BT";
					inputParam[10]  = "VI";
					inputParam[11]  = "PV";
					inputParam[12]  = "SV";
					inputParam[13] = NULL;
				}
				else
				{
					inputParam[5]  = "UT";
					inputParam[6]  = "UT";
					inputParam[7]  = "BT";
					inputParam[8]  = "MP";
					inputParam[9]  = "BT";
					inputParam[10]  = "MP";
					inputParam[11]  = "BT";
					inputParam[12]  = "MP";
					inputParam[13] = NULL;

					strcpy(lc_tableTypeList, tableTypeList);
					token = strtok(lc_tableTypeList, " ,'");
					while (token != NULL)
					{
						if (strcmp(token, "SYSTEM") == 0)
						{
							token = strtok(NULL, " ,'");
							if (token != NULL)
							{
								if (strcmp(token, "VIEW") == 0)
								{
									if (! systemTableGiven)
									{
											inputParam[7]  = "VI";
											inputParam[8]  = "PV";
											inputParam[9]  = "SV";
									}
									inputParam[10]  = "VI";
									inputParam[11]  = "PV";
									inputParam[12]  = "SV";
								}
								else
								{
									if (strcmp(token, "TABLE") == 0)
									{
										systemTableGiven = TRUE;
										inputParam[7]  = "BT";
										inputParam[8]  = "MP";
										inputParam[9]  = "BT";
									}
								}
							}
							else
								break;
						}
						else
						if (strcmp(token, "VIEW") == 0)
						{
							tableViewGiven = TRUE;
							inputParam[5] = "UT";
							inputParam[10]  = "VI";
							inputParam[11]  = "PV";
							inputParam[12]  = "SV";
						}
						else
						if (strcmp(token, "TABLE") == 0)
						{
							tableViewGiven = TRUE;
							inputParam[5] = "UT";
							inputParam[7]  = "BT";
							inputParam[8]  = "MP";
							inputParam[9]  = "BT";
						}
						token = strtok(NULL, " ,'");
					}
				}

				DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s|%s| inputParams= |%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|",
					catStmtLabel,
					tableParam[0],tableParam[1],tableParam[2],
					inputParam[0],inputParam[1],inputParam[2],inputParam[3],
					inputParam[4],inputParam[5],inputParam[6],inputParam[7],
					inputParam[8],inputParam[9],inputParam[10],inputParam[11],
					inputParam[12]));
			}

			break;
		case SQL_API_SQLCOLUMNS :
			// getColumns() DatabaseMetaData method
			if (!checkIfWildCard(catalogNm, catalogNmNoEsc))
			{
				exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
				FUNCTION_RETURN_VOID(("SQL_API_SQLCOLUMNS - checkIfWildCard() Failed"));
			}


			if (strcmp(catalogNm,"") == 0)
				strcpy(tableName1,srvrGlobal->SystemCatalog);
			else
				strcpy(tableName1, catalogNm);

			strcpy(tableName2, srvrGlobal->SystemCatalog);
			strcat(tableName2, ".");
			strcat(tableName2, smdSchemaList[SYS_SCH]);
			strcat(tableName2, smdTablesList[SCHEMATA]);
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName3);
			strcat(tableName3, ".");
			strcat(tableName3, smdSchemaList[DEF_SCH]);
			strcat(tableName3, schemaVersion);
			strcat(tableName3, ".");
			strcat(tableName3, smdTablesList[OBJECTS]);
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName4);
			strcat(tableName4, ".");
			strcat(tableName4, smdSchemaList[DEF_SCH]);
			strcat(tableName4, schemaVersion);
			strcat(tableName4, ".");
			strcat(tableName4, smdTablesList[COLS]);
				convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName5);
				strcat(tableName5, ".");
				strcat(tableName5, smdSchemaList[DEF_SCH]);
				strcat(tableName5, schemaVersion);
				strcat(tableName5, ".");
				if (strcmp(schemaVersion,"1200")==0 )
					strcat(tableName5, smdTablesList[VW_COL_TBLS]);
				else if (strcmp(schemaVersion,"2000")==0 )
					strcat(tableName5, smdTablesList[SYNONYM_USAGE]);
				else
					strcat(tableName5, smdTablesList[SYNONYM_USAGE]);
				if (strcmp(schemaVersion,"1200")==0 )
					strcpy((char *)catStmtLabel, "SQL_JAVA_COLUMNS_ANSI_Q1");				
				else
					strcpy((char *)catStmtLabel, "SQL_JAVA_COLUMNS_ANSI_Q4");

					tableParam[0] = tableName1;
					tableParam[1] = tableName2;
					tableParam[2] = tableName3;
					tableParam[3] = tableName5;
			tableParam[4] = tableName4;
					tableParam[5] = tableParam[4];
					tableParam[6] = NULL;
			sqlStmtType = TYPE_SELECT;
				inputParam[0] = (char *)schemaVersion;
			convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
			convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
			convertWildcard(metadataId, TRUE, tableNm, expTableNm);
			convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
			convertWildcard(metadataId, TRUE, columnNm, expColumnNm);
			convertWildcardNoEsc(metadataId, TRUE, columnNm, columnNmNoEsc);

			if (strcmp(schemaNm,"") == 0)
			{	// If schema is null default to wildcard
				inputParam[1] = "%";
				inputParam[2] = "%";
			} else {
				inputParam[1] = strupr(schemaNmNoEsc);
				inputParam[2] = strupr(expSchemaNm);
			}

			inputParam[3] = strupr(tableNmNoEsc);
			inputParam[4] = strupr(expTableNm);
			inputParam[5] = strupr(columnNmNoEsc);
			inputParam[6] = strupr(expColumnNm);
			/* soln: 10-070301-2970
			 * Desc: Additional Parameter is introduced
		    */
			inputParam[7] = "%";
			// inputParam[7] = "UT";
			inputParam[8] = odbcAppVersion;
			inputParam[9] = NULL;

			DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s|%s|%s|%s| inputParams= |%s|%s|%s|%s|%s|%s|%s|%s|%s|",
				catStmtLabel,
				tableParam[0],tableParam[1],tableParam[2],tableParam[3],tableParam[4],
				inputParam[0],inputParam[1],inputParam[2],inputParam[3],
				inputParam[4],inputParam[5],inputParam[6],inputParam[7],inputParam[8]));

			break;
		case SQL_API_SQLPRIMARYKEYS :
			// getPrimaryKeys() DatabaseMetaData method
			if ((!checkIfWildCard(catalogNm, catalogNmNoEsc) || !checkIfWildCard(schemaNm, schemaNmNoEsc) || !checkIfWildCard(tableNm, tableNmNoEsc)) && !metadataId)
			{
				exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
				FUNCTION_RETURN_VOID(("SQL_API_SQLPRIMARYKEYS - checkIfWildCard() or metadataId Failed"));
			}
			if (strcmp(schemaVersion,"1200") == 0)
			strcpy((char *)catStmtLabel, "SQL_PRIMARYKEYS_ANSI_Q1");			
			else
				strcpy((char *)catStmtLabel, "SQL_PRIMARYKEYS_ANSI_Q4");
			if (strcmp(catalogNm,"") == 0)
				strcpy(tableName1,srvrGlobal->SystemCatalog);
			else
				strcpy(tableName1, catalogNm);

			tableParam[0] = tableName1;
			strcpy(tableName2, srvrGlobal->SystemCatalog);
			strcat(tableName2, ".");
			strcat(tableName2, smdSchemaList[SYS_SCH]);
			strcat(tableName2, smdTablesList[SCHEMATA]);
			tableParam[1] = tableName2;
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName3);
			strcat(tableName3, ".");
			strcat(tableName3, smdSchemaList[DEF_SCH]);
			strcat(tableName3, schemaVersion);
			strcat(tableName3, ".");
			strcat(tableName3, smdTablesList[OBJECTS]);
			tableParam[2] = tableName3;
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName4);
			strcat(tableName4, ".");
			strcat(tableName4, smdSchemaList[DEF_SCH]);
			strcat(tableName4, schemaVersion);
			strcat(tableName4, ".");
			strcat(tableName4, smdTablesList[COLS]);
			tableParam[3] = tableName4;
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName5);
			strcat(tableName5, ".");
			strcat(tableName5, smdSchemaList[DEF_SCH]);
			strcat(tableName5, schemaVersion);
			strcat(tableName5, ".");
			strcat(tableName5, smdTablesList[KEY_COL_USAGE]);
			tableParam[4] = tableName5;
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName6);
			strcat(tableName6, ".");
			strcat(tableName6, smdSchemaList[DEF_SCH]);
			strcat(tableName6, schemaVersion);
			strcat(tableName6, ".");
			strcat(tableName6, smdTablesList[TBL_CONSTRAINTS]);
			tableParam[5] = tableName6;
			tableParam[6] = NULL;
			sqlStmtType = TYPE_SELECT;
			inputParam[0] = (char *)schemaVersion;
			convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
			convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
			convertWildcard(metadataId, TRUE, tableNm, expTableNm);
			convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);

			if (strcmp(schemaNm,"") == 0)
			{	// If schema is null default to wildcard
				inputParam[1] = "%";
				inputParam[2] = "%";
			} else {
				inputParam[1] = strupr(schemaNmNoEsc);
				inputParam[2] = strupr(expSchemaNm);
			}

			inputParam[3] = strupr(tableNmNoEsc);
			inputParam[4] = strupr(expTableNm);
			inputParam[5] = NULL;

			DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s|%s|%s|%s|%s| inputParams= |%s|%s|%s|%s|%s|",
				catStmtLabel,
				tableParam[0],tableParam[1],tableParam[2],tableParam[3],
				tableParam[4],tableParam[5],
				inputParam[0],inputParam[1],inputParam[2],inputParam[3],inputParam[4]));

			break;
		case SQL_API_SQLSPECIALCOLUMNS :
			// getBestRowIdentifier()/getVersionColumns DatabaseMetaData methods
			if ((!checkIfWildCard(catalogNm, catalogNmNoEsc) ||
				 !checkIfWildCard(schemaNm, schemaNmNoEsc) ||
				 !checkIfWildCard(tableNm, tableNmNoEsc))  &&
				 !metadataId)
			{
				exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
				FUNCTION_RETURN_VOID(("SQL_API_SQLSPECIALCOLUMNS - checkIfWildCard() or metadataId Failed"));
			}


			if (strcmp(catalogNm,"") == 0)
			strcpy(tableName1, srvrGlobal->SystemCatalog);
			else
				strcpy(tableName1, catalogNm);
			strcpy(tableName2, srvrGlobal->SystemCatalog);
			strcat(tableName2, ".");
			strcat(tableName2, smdSchemaList[SYS_SCH]);
			strcat(tableName2, smdTablesList[SCHEMATA]);


			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName3);
			strcat(tableName3, ".");
			strcat(tableName3, smdSchemaList[DEF_SCH]);
			strcat(tableName3, schemaVersion);
			strcat(tableName3, ".");
			strcat(tableName3, smdTablesList[OBJECTS]);

			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName4);
			strcat(tableName4, ".");
			strcat(tableName4, smdSchemaList[DEF_SCH]);
			strcat(tableName4, schemaVersion);
			strcat(tableName4, ".");
			strcat(tableName4, smdTablesList[COLS]);

			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName5);
			strcat(tableName5, ".");
			strcat(tableName5, smdSchemaList[DEF_SCH]);
			strcat(tableName5, schemaVersion);
			strcat(tableName5, ".");
			strcat(tableName5, smdTablesList[KEY_COL_USAGE]);

			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName6);
			strcat(tableName6, ".");
			strcat(tableName6, smdSchemaList[DEF_SCH]);
			strcat(tableName6, schemaVersion);
			strcat(tableName6, ".");
			strcat(tableName6, smdTablesList[TBL_CONSTRAINTS]);
				if (strcmp(schemaVersion,"1200") == 0 )
					strcpy((char *)catStmtLabel, "SQL_JAVA_SPECIALCOLUMNS_ANSI_Q1");				
				else
					strcpy((char *)catStmtLabel, "SQL_JAVA_SPECIALCOLUMNS_ANSI_Q4");
				tableParam[0] = tableName2;
				tableParam[1] = tableName3;
				tableParam[2] = tableName4;
				tableParam[3] = tableName5;
			tableParam[4] = tableName6;
			tableParam[5] = NULL;
			sqlStmtType = TYPE_SELECT;
			inputParam[0] = (char *)schemaVersion;
			convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
			convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);

			if (strcmp(schemaNm,"") == 0)
			{	// If schema is null default to wildcard
				inputParam[1] = "%";
			} else {
				inputParam[1] = strupr(schemaNmNoEsc);
			}

			inputParam[2] = strupr(tableNmNoEsc);
			inputParam[3] = odbcAppVersion;
			inputParam[4] = NULL;

			DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s|%s|%s|%s| inputParams= |%s|%s|%s|%s|",
				catStmtLabel,
				tableParam[0],tableParam[1],tableParam[2],tableParam[3],tableParam[4],
				inputParam[0],inputParam[1],inputParam[2],inputParam[3]));

			break;
		case SQL_API_SQLSTATISTICS :
			// getIndexInfo() DatabaseMetaData method
			if ((!checkIfWildCard(catalogNm, catalogNmNoEsc) ||
				 !checkIfWildCard(schemaNm, schemaNmNoEsc))  &&
				 !metadataId)
			{
				exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
				FUNCTION_RETURN_VOID(("SQL_API_SQLSTATISTICS - checkIfWildCard() or metadataId Failed"));
			}
			if (strcmp(schemaVersion,"1200") == 0 )
			strcpy((char *)catStmtLabel, "SQL_STATISTICS_ANSI_Q1");			
			else
				strcpy((char *)catStmtLabel, "SQL_STATISTICS_ANSI_Q4");
			if (strcmp(catalogNm,"") == 0)
				strcpy(tableName1,srvrGlobal->SystemCatalog);
			else
				strcpy(tableName1, catalogNm);
			tableParam[0] = tableName1;
			strcpy(tableName2, srvrGlobal->SystemCatalog);
			strcat(tableName2, ".");
			strcat(tableName2, smdSchemaList[SYS_SCH]);
			strcat(tableName2, smdTablesList[SCHEMATA]);
			tableParam[1] = tableName2;
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName3);
			strcat(tableName3, ".");
			strcat(tableName3, smdSchemaList[DEF_SCH]);
			strcat(tableName3, schemaVersion);
			strcat(tableName3, ".");
			strcat(tableName3, smdTablesList[OBJECTS]);
			tableParam[2] = tableName3;
			tableParam[3] = (char *)schemaVersion;
			convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
			convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
			convertWildcard(metadataId, TRUE, tableNm, expTableNm);
			convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);

			if (strcmp(schemaNm,"") == 0)
			{	// If schema is null default to wildcard
				tableParam[4] = "%";
				tableParam[5] = "%";
			} else {
				tableParam[4] = schemaNmNoEsc;
				tableParam[5] = expSchemaNm;
			}

			tableParam[6] = tableNmNoEsc;
			tableParam[7] = expTableNm;
			tableParam[8] = tableName1;
			tableParam[9] = tableName2;
			tableParam[10] = tableName3;
			tableParam[11] = tableName3;
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName4);
			strcat(tableName4, ".");
			strcat(tableName4, smdSchemaList[DEF_SCH]);
			strcat(tableName4, schemaVersion);
			strcat(tableName4, ".");
			strcat(tableName4, smdTablesList[COLS]);
			tableParam[12] = tableName4;
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName5);
			strcat(tableName5, ".");
			strcat(tableName5, smdSchemaList[DEF_SCH]);
			strcat(tableName5, schemaVersion);
			strcat(tableName5, ".");
			strcat(tableName5, smdTablesList[ACCESS_PATHS]);
			tableParam[13] = tableName5;
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName6);
			strcat(tableName6, ".");
			strcat(tableName6, smdSchemaList[DEF_SCH]);
			strcat(tableName6, schemaVersion);
			strcat(tableName6, ".");
			strcat(tableName6, smdTablesList[ACCESS_PATH_COLS]);
			tableParam[14] = tableName6;
			tableParam[15] = NULL;
			sqlStmtType = TYPE_SELECT;
			inputParam[0] = (char *)schemaVersion;

			if (strcmp(schemaNm,"") == 0)
			{	// If schema is null default to wildcard
				inputParam[1] = "%";
				inputParam[2] = "%";
			} else {
				inputParam[1] = strupr(schemaNmNoEsc);
				inputParam[2] = strupr(expSchemaNm);
			}

			inputParam[3] = strupr(tableNmNoEsc);
			inputParam[4] = strupr(expTableNm);
			if (uniqueness == SQL_INDEX_UNIQUE)
			{
				inputParam[5] = "Y";
				inputParam[6] = "Y";
			}
			else
			{
				inputParam[5] = "Y";
				inputParam[6] = "Y"; 	// Changed param from 'N' to 'Y', Since MS Access doesn't work.
			}
			inputParam[7] = NULL;

			DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|",
				catStmtLabel,
				tableParam[0],tableParam[1],tableParam[2],tableParam[3],tableParam[4],
				tableParam[5],tableParam[6],tableParam[7],tableParam[8],tableParam[9],
				tableParam[10],tableParam[11],tableParam[12],tableParam[13],tableParam[14]));
			DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  inputParams= |%s|%s|%s|%s|%s|%s|%s|",
				catStmtLabel,
				inputParam[0],inputParam[1],inputParam[2],inputParam[3],inputParam[4],
				inputParam[5],inputParam[6]));

			break;
		case SQL_API_SQLCOLUMNPRIVILEGES :
			// getColumnPrivileges() DatabaseMetaData method
			if ((!checkIfWildCard(catalogNm, catalogNmNoEsc) ||
				 !checkIfWildCard(schemaNm, schemaNmNoEsc) ||
				 !checkIfWildCard(tableNm, tableNmNoEsc))  &&
				 !metadataId)
			{
				exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
				FUNCTION_RETURN_VOID(("SQL_API_SQLCOLUMNPRIVILEGES - checkIfWildCard() or metadataId Failed"));
			}
			if (strcmp(schemaVersion,"1200") == 0 )
			strcpy((char *)catStmtLabel, "SQL_COLUMNPRIVILEGES_ANSI_Q1");			
			else
				strcpy((char *)catStmtLabel, "SQL_COLUMNPRIVILEGES_ANSI_Q4");

			if (strcmp(catalogNm,"") == 0)
				strcpy(tableName1,srvrGlobal->SystemCatalog);
			else
				strcpy(tableName1, catalogNm);

			tableParam[0] = tableName1;
			strcpy(tableName2, srvrGlobal->SystemCatalog);
			strcat(tableName2, ".");
			strcat(tableName2, smdSchemaList[SYS_SCH]);
			strcat(tableName2, smdTablesList[SCHEMATA]);
			tableParam[1] = tableName2;
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName3);
			strcat(tableName3, ".");
			strcat(tableName3, smdSchemaList[DEF_SCH]);
			strcat(tableName3, schemaVersion);
			strcat(tableName3, ".");
			strcat(tableName3, smdTablesList[OBJECTS]);
			tableParam[2] = tableName3;
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName4);
			strcat(tableName4, ".");
			strcat(tableName4, smdSchemaList[DEF_SCH]);
			strcat(tableName4, schemaVersion);
			strcat(tableName4, ".");
			strcat(tableName4, smdTablesList[COLS]);
			tableParam[3] = tableName4;
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName5);
			strcat(tableName5, ".");
			strcat(tableName5, smdSchemaList[DEF_SCH]);
			strcat(tableName5, schemaVersion);
			strcat(tableName5, ".");
			strcat(tableName5, smdTablesList[COL_PRIVILEGES]);
			tableParam[4] = tableName5;
			tableParam[5] = NULL;
			sqlStmtType = TYPE_SELECT;
			inputParam[0] = (char *)schemaVersion;
			convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
			convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
			convertWildcard(metadataId, TRUE, columnNm, expColumnNm);
			convertWildcardNoEsc(metadataId, TRUE, columnNm, columnNmNoEsc);

			if (strcmp(schemaNm,"") == 0)
			{	// If schema is null default to wildcard
				inputParam[1] = "%";
			} else {
				inputParam[1] = schemaNmNoEsc;
			}

			inputParam[2] = tableNmNoEsc;
			inputParam[3] = columnNmNoEsc;
			inputParam[4] = expColumnNm;
			inputParam[5] = NULL;

			DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s|%s|%s|%s|  inputParams= |%s|%s|%s|%s|%s|",
				catStmtLabel,
				tableParam[0],tableParam[1],tableParam[2],tableParam[3],tableParam[4],
				inputParam[0],inputParam[1],inputParam[2],inputParam[3],inputParam[4]));

			break;
		case SQL_API_SQLTABLEPRIVILEGES :
			// getTablePrivileges() DatabaseMetaData method
			if (!checkIfWildCard(catalogNm, catalogNmNoEsc) && !metadataId)
			{
				exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
				FUNCTION_RETURN_VOID(("SQL_API_SQLTABLEPRIVILEGES - checkIfWildCard() or metadataId Failed"));
			}
			if (strcmp(schemaVersion,"1200") == 0 )
			strcpy((char *)catStmtLabel, "SQL_TABLEPRIVILEGES_ANSI_Q1");			
			else
				strcpy((char *)catStmtLabel, "SQL_TABLEPRIVILEGES_ANSI_Q4");

			if (strcmp(catalogNm,"") == 0)
				strcpy(tableName1,srvrGlobal->SystemCatalog);
			else
				strcpy(tableName1, catalogNm);

			tableParam[0] = tableName1;
			strcpy(tableName2, srvrGlobal->SystemCatalog);
			strcat(tableName2, ".");
			strcat(tableName2, smdSchemaList[SYS_SCH]);
			strcat(tableName2, smdTablesList[SCHEMATA]);
			tableParam[1] = tableName2;
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName3);
			strcat(tableName3, ".");
			strcat(tableName3, smdSchemaList[DEF_SCH]);
			strcat(tableName3, schemaVersion);
			strcat(tableName3, ".");
			strcat(tableName3, smdTablesList[OBJECTS]);
			tableParam[2] = tableName3;
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName4);
			strcat(tableName4, ".");
			strcat(tableName4, smdSchemaList[DEF_SCH]);
			strcat(tableName4, schemaVersion);
			strcat(tableName4, ".");
			strcat(tableName4, smdTablesList[TBL_PRIVILEGES]);
			tableParam[3] = tableName4;
			if (strcmp(schemaVersion,"1200") == 0 )
			tableParam[4] = NULL;
			else
			{
				convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName5);
				strcat(tableName5, ".");
				strcat(tableName5, smdSchemaList[DEF_SCH]);
				strcat(tableName5, schemaVersion);
				strcat(tableName5, ".");
				strcat(tableName5, smdTablesList[SCH_PRIVILEGES]);
				tableParam[4] = tableName5;
				tableParam[5] = NULL;
			}
			sqlStmtType = TYPE_SELECT;
			inputParam[0] = (char *)schemaVersion;
			convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
			convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
			convertWildcard(metadataId, TRUE, tableNm, expTableNm);
			convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);

			if (strcmp(schemaNm,"") == 0)
			{	// If schema is null default to wildcard
				inputParam[1] = "%";
				inputParam[2] = "%";
			} else {
				inputParam[1] = schemaNmNoEsc;
				inputParam[2] = expSchemaNm;
			}

			inputParam[3] = tableNmNoEsc;
			inputParam[4] = expTableNm;
			inputParam[5] = NULL;

			DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s|%s|%s|  inputParams= |%s|%s|%s|%s|%s|",
				catStmtLabel,
				tableParam[0],tableParam[1],tableParam[2],tableParam[3],
				inputParam[0],inputParam[1],inputParam[2],inputParam[3],inputParam[4]));

			break;
		case SQL_API_SQLGETTYPEINFO :
			// getTypeInfo() DatabaseMetaData method
			tableParam[0] = NULL;
			sqlStmtType = TYPE_SELECT;
			strcpy((char *)catStmtLabel, "SQL_GETTYPEINFO_ANSI_Q1");

			if (sqlType == 0)
			{
				// lowest SQLGetInfo value supported (-92), look in sql.h and sqlext.h
				inputParam[0] = "-92";
				// highest SQLGetInfo value supported (113)
				inputParam[1] = "113";
				inputParam[2] = odbcAppVersion;
				inputParam[3] = NULL;
			}
			else
			{
				itoa(sqlType, tmpBuf, 10);
				inputParam[0] = (char *)tmpBuf;
				inputParam[1] = inputParam[0];
				inputParam[2] = odbcAppVersion;
				inputParam[3] = NULL;
			}

			DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  inputParams= |%s|%s|%s|",
				catStmtLabel,inputParam[0],inputParam[1],inputParam[2]));

			break;
		case SQL_API_SQLPROCEDURES:
			// getProcedures() DatabaseMetaData method
			if (strcmp(schemaVersion,"1200") == 0)
				strcpy((char *)catStmtLabel, "SQL_PROCEDURES_ANSI_Q1");			
			else
				strcpy((char *)catStmtLabel, "SQL_PROCEDURES_ANSI_Q4");

			if (!checkIfWildCard(catalogNm, expCatalogNm))
			{
				exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
				FUNCTION_RETURN_VOID(("SQL_API_SQLPROCEDURES - checkIfWildCard() Failed"));
			}

			if (strcmp(catalogNm,"") == 0)
				strcpy(tableName1,srvrGlobal->SystemCatalog);
			else
				strcpy(tableName1,catalogNm);
			tableParam[0] = tableName1;
			strcpy(tableName2,srvrGlobal->SystemCatalog);
			strcat(tableName2, ".");
			strcat(tableName2, smdSchemaList[SYS_SCH]);
			strcat(tableName2, smdTablesList[SCHEMATA]);
			tableParam[1] = tableName2;
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName3);
			strcat(tableName3, ".");
			strcat(tableName3, smdSchemaList[DEF_SCH]);
			strcat(tableName3, schemaVersion);
			strcat(tableName3, ".");
			strcat(tableName3, smdTablesList[OBJECTS]);
			tableParam[2] = tableName3;
			tableParam[3] = NULL;
			sqlStmtType = TYPE_SELECT;
			inputParam[0] = (char *)schemaVersion;
			convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
			convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
			convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
			convertWildcard(metadataId, TRUE, tableNm, expTableNm);

			if (strcmp(schemaNm,"") == 0)
			{	// If schema is null default to wildcard
				inputParam[1] = "%";
				inputParam[2] = "%";
			} else {
				inputParam[1] = strupr(schemaNmNoEsc);
				inputParam[2] = strupr(expSchemaNm);
			}

			inputParam[3] = strupr(tableNmNoEsc);
			inputParam[4] = strupr(expTableNm);
			inputParam[5] = NULL;

			DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s|%s|  inputParams= |%s|%s|%s|%s|%s|",
				catStmtLabel,
				tableParam[0],tableParam[1],tableParam[2],
				inputParam[0],inputParam[1],inputParam[2],inputParam[3],inputParam[4]));

			break;
		case SQL_API_SQLPROCEDURECOLUMNS:
			// getProcedureColumns() DatabaseMetaData method
			if (!checkIfWildCard(catalogNm, catalogNmNoEsc) && !metadataId)
			{
				exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
				FUNCTION_RETURN_VOID(("SQL_API_SQLPROCEDURECOLUMNS - checkIfWildCard() or metadataId Failed"));
			}
			if (strcmp(schemaVersion,"1200") == 0 )
			strcpy((char *)catStmtLabel, "SQL_PROCEDUREPARAMS_ANSI_Q1");			
			else
				strcpy((char *)catStmtLabel, "SQL_PROCEDUREPARAMS_ANSI_Q4");

			if (strcmp(catalogNm,"") == 0)
				strcpy(tableName1,srvrGlobal->SystemCatalog);
			else
				strcpy(tableName1, catalogNm);

			tableParam[0] = tableName1;
			strcpy(tableName2, srvrGlobal->SystemCatalog);
			strcat(tableName2, ".");
			strcat(tableName2, smdSchemaList[SYS_SCH]);
			strcat(tableName2, smdTablesList[SCHEMATA]);
			tableParam[1] = tableName2;
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName3);
			strcat(tableName3, ".");
			strcat(tableName3, smdSchemaList[DEF_SCH]);
			strcat(tableName3, schemaVersion);
			strcat(tableName3, ".");
			strcat(tableName3, smdTablesList[OBJECTS]);
			tableParam[2] = tableName3;
			convertWildcardNoEsc(FALSE, FALSE, tableName1, tableName4);
			strcat(tableName4, ".");
			strcat(tableName4, smdSchemaList[DEF_SCH]);
			strcat(tableName4, schemaVersion);
			strcat(tableName4, ".");
			strcat(tableName4, smdTablesList[COLS]);
			tableParam[3] = tableName4;
			tableParam[4] = tableName4;
			tableParam[5] = NULL;
			sqlStmtType = TYPE_SELECT;
			inputParam[0] = (char *)schemaVersion;
			convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
			convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
			convertWildcard(metadataId, TRUE, tableNm, expTableNm);
			convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
			convertWildcard(metadataId, TRUE, columnNm, expColumnNm);
			convertWildcardNoEsc(metadataId, TRUE, columnNm, columnNmNoEsc);

			if (strcmp(schemaNm,"") == 0)
			{	// If schema is null default to wildcard
				inputParam[1] = "%";
				inputParam[2] = "%";
			} else {
				inputParam[1] = strupr(schemaNmNoEsc);
				inputParam[2] = strupr(expSchemaNm);
			}

			inputParam[3] = strupr(tableNmNoEsc);
			inputParam[4] = strupr(expTableNm);
			inputParam[5] = strupr(columnNmNoEsc);
			inputParam[6] = strupr(expColumnNm);
			inputParam[7] = odbcAppVersion;
			inputParam[8] = NULL;

			DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s|%s|%s|%s|  inputParams= |%s|%s|%s|%s|%s|%s|%s|%s|",
				catStmtLabel,
				tableParam[0],tableParam[1],tableParam[2],tableParam[3],tableParam[4],
				inputParam[0],inputParam[1],inputParam[2],inputParam[3],
				inputParam[4],inputParam[5],inputParam[6],inputParam[7]));

			break;
		case SQL_API_SQLFOREIGNKEYS:
				if ((strcmp(tableNm,"") != 0) && (strcmp(fktableNm,"") == 0))
				{
					if (strcmp(schemaVersion,"1200") == 0  )
				strcpy((char *)fkstmtLabel, "SQL_FOREIGNKEYS_EXPORT_ANSI_Q1");					
					else
						strcpy((char *)fkstmtLabel, "SQL_FOREIGNKEYS_EXPORT_ANSI_Q7");
				}
				else
				{
					if (strcmp(schemaVersion,"1200") == 0  )
						strcpy((char *)fkstmtLabel, "SQL_FOREIGNKEYS_IMPORT_ANSI_Q1");					
					else
						strcpy((char *)fkstmtLabel, "SQL_FOREIGNKEYS_IMPORT_ANSI_Q7");
			}

			DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s catalogNm=%s schemaNm=%s tableNm=%s fkcatalogNm=%s fkschemaNm=%s fktableNm=%s",
				fkstmtLabel,catalogNm,schemaNm,tableNm,fkcatalogNm,fkschemaNm,fktableNm));

			strcpy(tableName1,srvrGlobal->SystemCatalog);
			strcat(tableName1, ".");
			strcat(tableName1, smdSchemaList[SYS_SCH]);
			strcat(tableName1, smdTablesList[CATSYS]);
			tableParam[0] = tableName1;
			strcpy(tableName2,srvrGlobal->SystemCatalog);
			strcat(tableName2, ".");
			strcat(tableName2, smdSchemaList[SYS_SCH]);
			strcat(tableName2, smdTablesList[SCHEMATA]);
			tableParam[1] = tableName2;
				tableParam[2] = (char *)schemaVersion;
			if ((strcmp(catalogNm,"") == 0) && (strcmp(fkcatalogNm,"") == 0))
				strcpy(tableName3,srvrGlobal->SystemCatalog);
			else
			{
				if ((strcmp(tableNm,"") != 0) && (strcmp(fktableNm,"") == 0))
					strcpy(tableName3, catalogNm);   // getExportedKeys
				else
					strcpy(tableName3, fkcatalogNm); // getImportedKeys
			}
			convertWildcardNoEsc(FALSE, FALSE, tableName3, tableName4);
			strcat(tableName4, ".");
			strcat(tableName4, smdSchemaList[DEF_SCH]);
			strcat(tableName4, schemaVersion);
			strcat(tableName4, ".");
			strcat(tableName4, smdTablesList[OBJECTS]);
			tableParam[3] = tableName4;
			tableParam[4] = tableName4;
			convertWildcardNoEsc(FALSE, FALSE, tableName3, tableName5);
			strcat(tableName5, ".");
			strcat(tableName5, smdSchemaList[DEF_SCH]);
			strcat(tableName5, schemaVersion);
			strcat(tableName5, ".");
			strcat(tableName5, smdTablesList[TBL_CONSTRAINTS]);
			tableParam[5] = tableName5;
			convertWildcardNoEsc(FALSE, FALSE, tableName3, tableName6);
			strcat(tableName6, ".");
			strcat(tableName6, smdSchemaList[DEF_SCH]);
			strcat(tableName6, schemaVersion);
			strcat(tableName6, ".");
			if ((strcmp(tableNm,"") != 0) && (strcmp(fktableNm,"") == 0)) // getExportedKeys()
				strcat(tableName6, smdTablesList[RI_UNIQUE_USAGE]);
			else // getImportedKeys
				strcat(tableName6, smdTablesList[REF_CONSTRAINTS]);
			tableParam[6] = tableName6;
			convertWildcardNoEsc(FALSE, FALSE, tableName3, tableName7);
			strcat(tableName7, ".");
			strcat(tableName7, smdSchemaList[DEF_SCH]);
			strcat(tableName7, schemaVersion);
			strcat(tableName7, ".");
			strcat(tableName7, smdTablesList[COLS]);
			tableParam[7] = tableName7;
			convertWildcardNoEsc(FALSE, FALSE, tableName3, tableName8);
			strcat(tableName8, ".");
			strcat(tableName8, smdSchemaList[DEF_SCH]);
			strcat(tableName8, schemaVersion);
			strcat(tableName8, ".");
			strcat(tableName8, smdTablesList[KEY_COL_USAGE]);
			tableParam[8] = tableName8;
			tableParam[9] = NULL;
			if ((strcmp(tableNm,"") != 0) && (strcmp(fktableNm,"") == 0))
				convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
			else
				convertWildcardNoEsc(metadataId, TRUE, fktableNm, tableNmNoEsc);
			inputParam[0] = tableNmNoEsc;
				inputParam[1] = schemaNmNoEsc; //10-071009-8075
				inputParam[2] = NULL;
			sqlStmtType = TYPE_SELECT;

			DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s|%s|%s|%s|%s|%s|%s|%s|",
				fkstmtLabel,
				tableParam[0],tableParam[1],tableParam[2],tableParam[3],tableParam[4],
				tableParam[5],tableParam[6],tableParam[7],tableParam[8]));
			DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  inputParams= |%s|",
				fkstmtLabel,inputParam[0]));

			retCode = executeAndFetchSMDQuery(objtag_, call_id_, dialogueId, fkstmtLabel,
									sqlStmtType, &tableParam[0], &inputParam[0], &lc_outputDesc,
									&executeException, &fetchException, sqlWarning, rowsAffected,
									&tempOutputValueList, stmtId);

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
			} else {
				SQLValue = (SQLValue_def *)tempOutputValueList._buffer;
				if (SQLValue->dataInd == -1)
				{
					exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
					exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_SCHEMA_VERSION;
					FUNCTION_RETURN_VOID(("SQLSVC_EXCEPTION_INVALID_SCHEMA_VERSION"));
				}
			}

			// Null out some parameters used for 2nd query buildup to ensure it
			// will execute with at least NO_DATA_FOUND, instead of error or
			// incorrect number of cols (7 from 1st query instead of expected 14)
			strcpy(userCatalogNm, srvrGlobal->SystemCatalog);
			schemaNmAct[0]	= '\0';
			colNmAct[0]		= '\0';
			ordinalAct[0]	= '\0';
			tableNmAct[0]	= '\0';
			obuidAct[0]		= '\0';
			riuidAct[0]		= '\0';
			numOfCols		= 0;
			rowsFKFetched	= 1;

			if( *rowsAffected > 0 ) {
				numOfCols = (long)tempOutputValueList._length/(*rowsAffected);
				rowsFKFetched = *rowsAffected;
			} else {
				rowsFKFetched = 1;
			}

			DEBUG_OUT(DEBUG_LEVEL_METADATA,("rowsAffected=%d numOfCols=%d rowsFKFetched=%d",
				*rowsAffected,numOfCols,rowsFKFetched));

			*rowsAffected = 0; // reset rowsAffected for additional loop appends

			for (curRowNo = 0 ; curRowNo < rowsFKFetched ; curRowNo++)
			{
				for( curColNo = 0; curColNo < numOfCols; curColNo++ )
				{
					SQLValue = (SQLValue_def *)tempOutputValueList._buffer + ((curRowNo * numOfCols)+curColNo);
					if (SQLValue->dataInd == -1) { //  We need to handle this case later - NULL value
						continue;
					}
					else
					{
						switch (curColNo)
						{
						case 0: // Guardian name is in VARCHAR format
							namelen = *(short *)(SQLValue->dataValue._buffer);
							strncpy(userCatalogNm, (char *)(SQLValue->dataValue._buffer)+2, namelen);
							userCatalogNm[namelen] = '\0';
							break;
						case 1:
							namelen = *(short *)(SQLValue->dataValue._buffer);
							strncpy(schemaNmAct, (char *)(SQLValue->dataValue._buffer)+2, namelen);
							schemaNmAct[namelen] = '\0';
							break;
						case 2:
							namelen = *(short *)(SQLValue->dataValue._buffer);
							strncpy(colNmAct, (char *)(SQLValue->dataValue._buffer)+2, namelen);
							colNmAct[namelen] = '\0';
							break;
						case 3:
							namelen = *(short *)(SQLValue->dataValue._buffer);
							strncpy(ordinalAct, (char *)(SQLValue->dataValue._buffer)+2, namelen);
							ordinalAct[namelen] = '\0';
							break;
						case 4:
							namelen = *(short *)(SQLValue->dataValue._buffer);
							strncpy(tableNmAct, (char *)(SQLValue->dataValue._buffer)+2, namelen);
							tableNmAct[namelen] = '\0';
							break;
						case 5:
							namelen = *(short *)(SQLValue->dataValue._buffer);
							strncpy(obuidAct, (char *)(SQLValue->dataValue._buffer)+2, namelen);
							obuidAct[namelen] = '\0';
							break;
						case 6:
							namelen = *(short *)(SQLValue->dataValue._buffer);
							strncpy(riuidAct, (char *)(SQLValue->dataValue._buffer)+2, namelen);
							riuidAct[namelen] = '\0';
							break;
						}
					}
				}

				if ((strcmp(tableNm,"") != 0) && (strcmp(fktableNm,"") == 0))
				{
						if (strcmp(schemaVersion,"1200") == 0  )
							strcpy((char *)catStmtLabel, "SQL_FOREIGNKEYS_EXPORT_ANSI_Q2");						
						else
							strcpy((char *)catStmtLabel, "SQL_FOREIGNKEYS_EXPORT_ANSI_Q8");
					if (strcmp(catalogNm,"") == 0)
						strcpy(tableName1,srvrGlobal->SystemCatalog);
					else
						strcpy(tableName1, catalogNm);
					tableParam[0] = tableName1;
					tableParam[1] = schemaNm;
					tableParam[2] = tableNm;
					tableParam[3] = colNmAct;
					tableParam[4] = userCatalogNm;
					tableParam[5] = schemaNmAct;
						tableParam[6] = (char *)ordinalAct;
					tableParam[7] = tableNmAct;
					strcpy(tableName2,srvrGlobal->SystemCatalog);
					strcat(tableName2, ".");
					strcat(tableName2, smdSchemaList[SYS_SCH]);
					strcat(tableName2, smdTablesList[SCHEMATA]);
					tableParam[8] = tableName2;
					convertWildcardNoEsc(FALSE, FALSE, userCatalogNm, tableName3);
					strcat(tableName3, ".");
					strcat(tableName3, smdSchemaList[DEF_SCH]);
					strcat(tableName3, schemaVersion);
					strcat(tableName3, ".");
					strcat(tableName3, smdTablesList[OBJECTS]);
					tableParam[9] = tableName3;
					tableParam[10] = tableName3;
					convertWildcardNoEsc(FALSE, FALSE, userCatalogNm, tableName4);
					strcat(tableName4, ".");
					strcat(tableName4, smdSchemaList[DEF_SCH]);
					strcat(tableName4, schemaVersion);
					strcat(tableName4, ".");
					strcat(tableName4, smdTablesList[TBL_CONSTRAINTS]);
					tableParam[11] = tableName4;
					convertWildcardNoEsc(FALSE, FALSE, userCatalogNm, tableName5);
					strcat(tableName5, ".");
					strcat(tableName5, smdSchemaList[DEF_SCH]);
					strcat(tableName5, schemaVersion);
					strcat(tableName5, ".");
					strcat(tableName5, smdTablesList[REF_CONSTRAINTS]);
					tableParam[12] = tableName5;
					convertWildcardNoEsc(FALSE, FALSE, userCatalogNm, tableName6);
					strcat(tableName6, ".");
					strcat(tableName6, smdSchemaList[DEF_SCH]);
					strcat(tableName6, schemaVersion);
					strcat(tableName6, ".");
					strcat(tableName6, smdTablesList[COLS]);
					tableParam[13] = tableName6;
					convertWildcardNoEsc(FALSE, FALSE, userCatalogNm, tableName7);
					strcat(tableName7, ".");
					strcat(tableName7, smdSchemaList[DEF_SCH]);
					strcat(tableName7, schemaVersion);
					strcat(tableName7, ".");
					strcat(tableName7, smdTablesList[KEY_COL_USAGE]);
					tableParam[14] = tableName7;
					tableParam[15] = NULL;
						inputParam[0] = (char *)schemaVersion;
					inputParam[1] = schemaNmAct;
						inputParam[2] = (char *)riuidAct;
					inputParam[3] = NULL;

					DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|",
						catStmtLabel,
						tableParam[0],tableParam[1],tableParam[2],tableParam[3],tableParam[4],
						tableParam[5],tableParam[6],tableParam[7],tableParam[8],tableParam[9],
						tableParam[10],tableParam[11],tableParam[12],tableParam[13],tableParam[14]));
					DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  inputParams= |%s|%s|%s|",
						catStmtLabel,inputParam[0],inputParam[1],inputParam[2]));
				}
				else
				{
						if (strcmp(schemaVersion,"1200") == 0  )
							strcpy((char *)catStmtLabel, "SQL_FOREIGNKEYS_IMPORT_ANSI_Q2");						
						else
							strcpy((char *)catStmtLabel, "SQL_FOREIGNKEYS_IMPORT_ANSI_Q8");

					tableParam[0] = userCatalogNm;
					tableParam[1] = schemaNmAct;
					if (strcmp(fkcatalogNm,"") == 0)
						strcpy(tableName1,srvrGlobal->SystemCatalog);
					else
						strcpy(tableName1, fkcatalogNm);
					tableParam[2] = tableName1;
					tableParam[3] = fkschemaNm;
					tableParam[4] = fktableNm;
					tableParam[5] = colNmAct;
						tableParam[6] = (char *)ordinalAct;
					tableParam[7] = tableNmAct;
					strcpy(tableName2,srvrGlobal->SystemCatalog);
					strcat(tableName2, ".");
					strcat(tableName2, smdSchemaList[SYS_SCH]);
					strcat(tableName2, smdTablesList[SCHEMATA]);
					tableParam[8] = tableName2;
					convertWildcardNoEsc(FALSE, FALSE, userCatalogNm, tableName3);
					strcat(tableName3, ".");
					strcat(tableName3, smdSchemaList[DEF_SCH]);
					strcat(tableName3, schemaVersion);
					strcat(tableName3, ".");
					strcat(tableName3, smdTablesList[OBJECTS]);
					tableParam[9] = tableName3;
					tableParam[10] = tableName3;
					convertWildcardNoEsc(FALSE, FALSE, userCatalogNm, tableName4);
					strcat(tableName4, ".");
					strcat(tableName4, smdSchemaList[DEF_SCH]);
					strcat(tableName4, schemaVersion);
					strcat(tableName4, ".");
					strcat(tableName4, smdTablesList[TBL_CONSTRAINTS]);
					tableParam[11] = tableName4;
					convertWildcardNoEsc(FALSE, FALSE, userCatalogNm, tableName5);
					strcat(tableName5, ".");
					strcat(tableName5, smdSchemaList[DEF_SCH]);
					strcat(tableName5, schemaVersion);
					strcat(tableName5, ".");
					strcat(tableName5, smdTablesList[RI_UNIQUE_USAGE]);
					tableParam[12] = tableName5;
					convertWildcardNoEsc(FALSE, FALSE, userCatalogNm, tableName6);
					strcat(tableName6, ".");
					strcat(tableName6, smdSchemaList[DEF_SCH]);
					strcat(tableName6, schemaVersion);
					strcat(tableName6, ".");
					strcat(tableName6, smdTablesList[COLS]);
					tableParam[13] = tableName6;
					convertWildcardNoEsc(FALSE, FALSE, userCatalogNm, tableName7);
					strcat(tableName7, ".");
					strcat(tableName7, smdSchemaList[DEF_SCH]);
					strcat(tableName7, schemaVersion);
					strcat(tableName7, ".");
					strcat(tableName7, smdTablesList[KEY_COL_USAGE]);
					tableParam[14] = tableName7;
					tableParam[15] = NULL;
						inputParam[0] = (char *)schemaVersion;
					inputParam[1] = schemaNmAct;
						inputParam[2] = (char *)obuidAct;
					if (strcmp(tableNm,"") != 0)
						inputParam[3] = tableNm;
					else
						inputParam[3] = "%";
					inputParam[4] = NULL;

					DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|",
						catStmtLabel,
						tableParam[0],tableParam[1],tableParam[2],tableParam[3],tableParam[4],
						tableParam[5],tableParam[6],tableParam[7],tableParam[8],tableParam[9],
						tableParam[10],tableParam[11],tableParam[12],tableParam[13],tableParam[14]));
					DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  inputParams= |%s|%s|%s|%s|",
						catStmtLabel,inputParam[0],inputParam[1],inputParam[2],inputParam[3]));

				}
				sqlStmtType = TYPE_SELECT;

				// Only reset outputValueList on first pass. Subsequent do_ExecFetchAppend
				// calls for additional rows should append data to end of buffer
				if (curRowNo == 0) {
					outputValueList->_buffer=NULL;
					outputValueList->_length=0;
				}

				retCode = do_ExecFetchAppend(objtag_, call_id_, &executeException, sqlWarning, dialogueId,
				  catStmtLabel, sqlStmtType, tableParam, inputParam, outputDesc, rowsAffected, outputValueList, stmtId);

				if (retCode != CEE_SUCCESS)
				{
					writeServerException(retCode,exception_,&prepareException,&executeException,NULL);
					MEMORY_DELETE(tempOutputValueList._buffer);
					FUNCTION_RETURN_VOID(("do_ExecFetchAppend() Failed"));
				}
				else
				{
					exception_->exception_nr = 0;
					// The getExportedKeys module file query returns duplicate rows, therefore
					// we want to return for getExportedKeys calls to avoid processing duplicate metadata.
					if (strncmp(catStmtLabel, "SQL_FOREIGNKEYS_EXPORT", 22) == 0)
					{
						FUNCTION_RETURN_VOID(("do_ExecFetchAppend() Passed"));
					}
				}
			} // end of for loop

			FUNCTION_RETURN_VOID(("do_ExecFetchAppend() Passed"));
			break;
		case SQL_TXN_ISOLATION:
			tableParam[0] = NULL;
			sqlStmtType = TYPE_UNKNOWN;
			switch (sqlType)
			{
			case SQL_TXN_READ_UNCOMMITTED:
				strcpy(catStmtLabel, "SQL_ISOLVL_STATEMENT_1");
				break;
			case SQL_TXN_READ_COMMITTED:
				strcpy(catStmtLabel, "SQL_ISOLVL_STATEMENT_2");
				break;
			case SQL_TXN_REPEATABLE_READ:
				strcpy(catStmtLabel, "SQL_ISOLVL_STATEMENT_3");
				break;
			case SQL_TXN_SERIALIZABLE:
				strcpy(catStmtLabel, "SQL_ISOLVL_STATEMENT_4");
				break;
			default:
				strcpy(catStmtLabel, "SQL_ISOLVL_STATEMENT_2");
				break;
			}
			break;
		default :
			exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_DATA_ERROR;
			FUNCTION_RETURN_VOID(("MX APIType switch default case"));
			break;
	}

	if (APIType != SQL_TXN_ISOLATION)
	{
		// Reset outputValueList to avoid appending fetched MX metadata to previous contents
		outputValueList->_buffer=NULL;
		outputValueList->_length=0;

		retCode = do_ExecFetchAppend(objtag_, call_id_, &executeException, sqlWarning, dialogueId,
		  catStmtLabel, sqlStmtType, tableParam, inputParam, outputDesc, rowsAffected, outputValueList, stmtId);

	}
	else
	{
		retCode = do_ExecSMD(objtag_, call_id_, &executeException, sqlWarning, dialogueId,
				catStmtLabel, sqlStmtType, tableParam, inputParam, outputDesc, stmtId);
	}

	if (retCode != CEE_SUCCESS)
	{
		if (retCode == FETCH_EXCEPTION &&
				fetchException.exception_nr == odbc_SQLSvc_FetchN_SQLNoDataFound_exn_)
		{
			exception_->exception_nr = 0;
			*rowsAffected  = 0;
			outputValueList->_buffer = NULL;
			outputValueList->_length = 0;
		}
		else
		{
			writeServerException(retCode,exception_,&prepareException,&executeException,&fetchException);
			FUNCTION_RETURN_VOID(("do_ExecFetchAppend()/do_ExecSMD() Failed"));
		}
	}
	else
	{
		exception_->exception_nr = 0;

		// Do not perform MP metadata check if MX metadata found
		// for any methods but SQL_API_SQLCOLUMNS or SQL_API_SQLSTATISTICS.
		if (APIType != SQL_API_SQLCOLUMNS && *rowsAffected > 0)
			if (APIType != SQL_API_SQLSTATISTICS && *rowsAffected != 1)
				FUNCTION_RETURN_VOID(("No MP Metadata Check Needed"));
	}

    // Need to collect SQL/MP metadata if GET_CATALOGNAME_Q2 query
	// found MP table info. Only check for the valid APITypes (methods)
	if (( (APIType == SQL_API_SQLCOLUMNS)	||
		  (APIType == SQL_API_SQLPRIMARYKEYS) ||
		  (APIType == SQL_API_SQLSPECIALCOLUMNS) ||
		  (APIType == SQL_API_SQLSTATISTICS) ) &&
		 queryMP)
	{
		userCatalogNm[0] = '\0';

		for (curRowNo = 0 ; curRowNo < rowsMPFetched ; curRowNo++)
		{
			for( curColNo = 0; curColNo < numOfCols; curColNo++ )
			{
				SQLValue = (SQLValue_def *)tempOutputValueList._buffer + ((curRowNo * numOfCols)+curColNo);

				if (SQLValue->dataInd == -1)	// NULL value case
				{
					exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
					exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_USER_CATALOG_NAME_ERROR;
					FUNCTION_RETURN_VOID(("SQLValue->dataInd == -1"));
				} else {
					switch (curColNo) {
	   				   case 0: // Guardian name is in VARCHAR format
							namelen = *(short *)(SQLValue->dataValue._buffer);
							strncpy(guardianNm, (char *)(SQLValue->dataValue._buffer)+2, namelen);
							guardianNm[namelen] = '\0';
							if (envGetCatalogName (guardianNm, &userCatalogNm[0]) != TRUE)
							{
								strcpy(userCatalogNm,srvrGlobal->NskSystemCatalogName);
							}
							break;
					   case 1:
							namelen = *(short *)(SQLValue->dataValue._buffer);
							strncpy(schemaNmAct, (char *)(SQLValue->dataValue._buffer)+2, namelen);
							schemaNmAct[namelen] = '\0';
							break;
					   case 2:
							namelen = *(short *)(SQLValue->dataValue._buffer);
							strncpy(tableNmAct, (char *)(SQLValue->dataValue._buffer)+2, namelen);
							tableNmAct[namelen] = '\0';
							break;
					}
				}
			} // for curColNo

			switch(APIType)
			{
			  case SQL_API_SQLCOLUMNS :
				// getColumns() DatabaseMetaData method
				SQLObjType[0] = '\0';
				if (envGetSQLType ((char *)tableNm, SQLObjType) != TRUE)
				{
					strcpy(SQLObjType, "2"); // Set SQL object type to table (2)
				}
				tableParam[0] = catalogNm;
				tableParam[1] = schemaNmAct;
				tableParam[2] = tableNmAct;
				tableParam[3] = SQLObjType;
				strcpy((char *)catStmtLabel, "SQL_JAVA_COLUMNS_ANSI_Q2");
				strcpy(tableName1, userCatalogNm);
				strcat(tableName1, smdTablesList[COLUMNS]);
				tableParam[4] = tableName1;
				tableParam[5] = tableParam[4];
				tableParam[6] = NULL;
				sqlStmtType = TYPE_SELECT;
				convertWildcardNoEsc(metadataId, TRUE, columnNm, columnNmNoEsc);
				convertWildcard(metadataId, TRUE, columnNm, expColumnNm);
				inputParam[0] = strupr(columnNmNoEsc);	// Since SQL/MP always upshifts the column names with or without quotes.
				inputParam[1] = strupr(expColumnNm);	// Since SQL/MP always upshifts the column names with or without quotes.
				inputParam[2] = guardianNm;
				inputParam[3] = SQLObjType;
				inputParam[4] = odbcAppVersion;
				inputParam[5] = translationId;
				inputParam[6] = NULL;

				DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s|%s|%s|%s|%s|  inputParams= |%s|%s|%s|%s|%s|%s|",
					catStmtLabel,
					tableParam[0],tableParam[1],tableParam[2],tableParam[3],
					tableParam[4],tableParam[5],
					inputParam[0],inputParam[1],inputParam[2],inputParam[3],
					inputParam[4],inputParam[5]));

				break;
			  case SQL_API_SQLPRIMARYKEYS :
				// getPrimaryKeys() DatabaseMetaData method
				strcpy((char *)catStmtLabel, "SQL_PRIMARYKEYS_ANSI_Q2");
				tableParam[0] = catalogNm;
				tableParam[1] = schemaNmAct;
				tableParam[2] = tableNmAct;
				strcpy(tableName1, userCatalogNm);
				strcat(tableName1, smdTablesList[COLUMNS]);
				tableParam[3] = tableName1;
				strcpy(tableName2, userCatalogNm);
				strcat(tableName2, smdTablesList[KEYS]);
				tableParam[4] = tableName2;
				strcpy(tableName3, userCatalogNm);
				strcat(tableName3, smdTablesList[INDEXES]);
				tableParam[5] = tableName3;
				tableParam[6] = NULL;
				sqlStmtType = TYPE_SELECT;
				inputParam[0] = guardianNm;
				inputParam[1] = NULL;

				DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s|%s|%s|%s|%s|  inputParams= |%s|",
					catStmtLabel,
					tableParam[0],tableParam[1],tableParam[2],tableParam[3],
					tableParam[4],tableParam[5],
					inputParam[0]));

				break;
			  case SQL_API_SQLSPECIALCOLUMNS :
				// getBestRowIdentifier()/getVersionColumns DatabaseMetaData methods
				strcpy((char *)catStmtLabel, "SQL_JAVA_SPECIALCOLUMNS_ANSI_Q2");
				strcpy(tableName1, userCatalogNm);
				strcat(tableName1, smdTablesList[KEYS]);
				tableParam[0] = tableName1;
				strcpy(tableName2, userCatalogNm);
				strcat(tableName2, smdTablesList[INDEXES]);
				tableParam[1] = tableName2;
				strcpy(tableName3, userCatalogNm);
				strcat(tableName3, smdTablesList[COLUMNS]);
				tableParam[2] = tableName3;
				tableParam[3] = NULL;
				sqlStmtType = TYPE_SELECT;
				inputParam[0] = guardianNm;
				// Note that getBestRowIdentifier sets SQL_NULLABLE, while
				// getVersionColumns does not.
				if (nullable == SQL_NULLABLE)
				{
					inputParam[1] = "Y";
					inputParam[2] = "Y";
				}
				else
				{
					inputParam[1] = "N";
					inputParam[2] = "N";
				}
				inputParam[3] = odbcAppVersion;
				inputParam[4] = translationId;
				inputParam[5] = NULL;

				DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s|%s|  inputParams= |%s|%s|%s|",
					catStmtLabel,
					tableParam[0],tableParam[1],tableParam[2],
					inputParam[0],inputParam[1],inputParam[2]));

				break;
			  case SQL_API_SQLSTATISTICS :
				// getIndexInfo() DatabaseMetaData method
				strcpy((char *)catStmtLabel, "SQL_STATISTICS_ANSI_Q2");
				tableParam[0] = catalogNm;
				tableParam[1] = schemaNmAct;
				tableParam[2] = tableNmAct;
				strcpy(tableName1, userCatalogNm);
				strcat(tableName1, smdTablesList[KEYS]);
				tableParam[3] = tableName1;
				strcpy(tableName2, userCatalogNm);
				strcat(tableName2, smdTablesList[INDEXES]);
				tableParam[4] = tableName2;
				strcpy(tableName3, userCatalogNm);
				strcat(tableName3, smdTablesList[COLUMNS]);
				tableParam[5] = tableName3;
				tableParam[6] = NULL;
				sqlStmtType = TYPE_SELECT;
				inputParam[0] = guardianNm;
				if (uniqueness == SQL_INDEX_UNIQUE) // Return only indices for unique values returned
				{
					inputParam[1] = "Y";
					inputParam[2] = "Y";
				}
				else // Return all indices
				{
					inputParam[1] = "Y";
					inputParam[2] = "N";  // MS Access EnvironmentType's not used in JDBC, return to 'N'.
				}
				inputParam[3] = NULL;

				DEBUG_OUT(DEBUG_LEVEL_METADATA,("%s  tableParams= |%s|%s|%s|%s|%s|%s|  inputParams= |%s|%s|%s|",
					catStmtLabel,
					tableParam[0],tableParam[1],tableParam[2],tableParam[3],
					tableParam[4],tableParam[5],
					inputParam[0],inputParam[1],inputParam[2]));

				break;
			  default :
				exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_DATA_ERROR;
				FUNCTION_RETURN_VOID(("MP APIType switch default case"));
				break;
			} 	// End of MP metadata APIType switch

			retCode = do_ExecFetchAppend(objtag_, call_id_, &executeException, sqlWarning, dialogueId,
		          catStmtLabel, sqlStmtType, tableParam, inputParam, outputDesc, rowsAffected, outputValueList, stmtId);

			if (retCode != CEE_SUCCESS)
			{
				writeServerException(retCode,exception_,&prepareException,&executeException,NULL);
				odbc_SQLSvc_Close_sme_(objtag_, call_id_, &closeException, dialogueId, (long)"SQL_GETCATALOGNAME_Q2",
					SQL_DROP, rowsAffected, sqlWarning);
				FUNCTION_RETURN_VOID(("do_ExecFetchAppend() Failed"));
			}

		} // for curRowNo
	} // APIType == SQL_API_COLUMNS...

	FUNCTION_RETURN_VOID((NULL));
}
