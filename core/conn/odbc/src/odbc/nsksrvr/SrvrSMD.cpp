/**********************************************************************
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
********************************************************************/
/**************************************************************************
**************************************************************************/
//
// MODULE: SrvrSMD.cpp
//
// PURPOSE: Implements the following methods
//	odbc_SQLSvc_GetSQLCatalogs_sme_
//
// MODIFICATION: Fixed a set of catalog API column header problems (11/24/97)
//
// MODIFICATION: Add server trace messages -- 4/30/98

// Changed DEFINITION SCHEMA version from 1000 to 1200 -- 08/31/99

#include <platform_ndcs.h>
#include <sql.h>
#include <sqlext.h>
#include "DrvrSrvr.h"
#include "odbcCommon.h"
#include "odbc_sv.h"                                               
#include "srvrcommon.h"
#include "srvrfunctions.h"
#include "srvrkds.h"
#include "sqlinterface.h"
#include "CommonDiags.h"
#include "ODBCMXTraceMsgs.h"
#include "NskUtil.h"
#include "SrvrConnect.h"

using namespace SRVR;

#include "ceercv.h"
#include "FileSystemSrvr.h"
#include "odbcs_srvr_res.h"

char smdSchemaList[3][50] = {0};
char *smdTablesList[] = {0};

/*
 * Asynchronous method function prototype for
 * operation 'odbc_SQLSrvr_GetSQLCatalogs_ame_'
 */
extern "C" void
odbc_SQLSrvr_GetSQLCatalogs_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_short APIType
  , /* In    */ const IDL_char *catalogNm
  , /* In    */ const IDL_char *schemaNm
  , /* In    */ const IDL_char *tableNm
  , /* In    */ const IDL_char *tableTypeList
  , /* In    */ const IDL_char *columnNm
  , /* In    */ IDL_long columnType
  , /* In    */ IDL_long rowIdScope
  , /* In    */ IDL_long nullable
  , /* In    */ IDL_long uniqueness
  , /* In    */ IDL_long accuracy
  , /* In    */ IDL_short sqlType
  , /* In    */ IDL_unsigned_long metadataId
  , /* In    */ const IDL_char *fkcatalogNm
  , /* In    */ const IDL_char *fkschemaNm
  , /* In    */ const IDL_char *fktableNm
  )
{
	const IDL_char* pcatalogNm = catalogNm;
	const IDL_char* pschemaNm = schemaNm;
	const IDL_char* ptableNm = tableNm;
	const IDL_char* pfkcatalogNm = fkcatalogNm;
	const IDL_char* pfkschemaNm = fkschemaNm;
	const IDL_char* pfktableNm = fktableNm;

	if (pcatalogNm == NULL)
		pcatalogNm = srvrGlobal->DefaultCatalog;
	if (pschemaNm == NULL)
		pschemaNm = srvrGlobal->DefaultSchema;
	if (tableNm == NULL && APIType == SQL_API_SQLFOREIGNKEYS)
		tableNm = "";

	if (pfkcatalogNm == NULL)
		pfkcatalogNm = srvrGlobal->DefaultCatalog;

	if (pfkschemaNm == NULL)
		pfkschemaNm = srvrGlobal->DefaultSchema;
	
	if (pfktableNm == NULL)
		pfktableNm = "";

	odbc_SQLSvc_GetSQLCatalogs_exc_ exception_;
	STMT_NAME_def	catStmtLabel;
	SQLItemDescList_def outputDesc;
	ERROR_DESC_LIST_def sqlWarning={0,0};
	char ConvertAPITypeToString[30];
	
	memset(&sqlWarning, 0, sizeof(sqlWarning));
	outputDesc._length = 0;
	exception_.exception_nr = 0;
	exception_.exception_detail = 0;
	exception_.u.SQLError.errorList._buffer = NULL;
	exception_.u.SQLError.errorList._length = 0;

	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceGetSQLCatalogsEnter(dialogueId, stmtLabel, APIType, 
								  catalogNm, schemaNm, tableNm,
								  tableTypeList, columnNm, columnType,
								  rowIdScope, nullable, uniqueness,
								  accuracy, sqlType, metadataId, 
								  fkcatalogNm, fkschemaNm, fktableNm);
	}

//LCOV_EXCL_START
	if(diagnostic_flags){
			switch (APIType)
			{
				case SQL_API_SQLTABLES:
					sprintf(ConvertAPITypeToString, "SQLTables (%d)", SQL_API_SQLTABLES);
					break;
				case SQL_API_SQLCOLUMNS:
					sprintf(ConvertAPITypeToString, "SQLColumns (%d)", SQL_API_SQLCOLUMNS);
					break;
				case SQL_API_SQLCOLUMNPRIVILEGES:
					sprintf(ConvertAPITypeToString, "SQLColumnPrivileges (%d)", SQL_API_SQLCOLUMNPRIVILEGES);
					break;
				case SQL_API_SQLFOREIGNKEYS:
					sprintf(ConvertAPITypeToString, "SQLForeignKeys (%d)", SQL_API_SQLFOREIGNKEYS);
					break;
				case SQL_API_SQLPRIMARYKEYS:
					sprintf(ConvertAPITypeToString, "SQLPrimaryKeys (%d)", SQL_API_SQLPRIMARYKEYS);
					break;
				case SQL_API_SQLSPECIALCOLUMNS:
					sprintf(ConvertAPITypeToString, "SQLSpecialColumns (%d)", SQL_API_SQLSPECIALCOLUMNS);
					break;
				case SQL_API_SQLSTATISTICS:
					sprintf(ConvertAPITypeToString, "SQLStatistics (%d)", SQL_API_SQLSTATISTICS);
					break;
				case SQL_API_SQLTABLEPRIVILEGES:
					sprintf(ConvertAPITypeToString, "SQLTablePrivileges (%d)", SQL_API_SQLTABLEPRIVILEGES);
					break;
				case SQL_API_SQLGETTYPEINFO:
					sprintf(ConvertAPITypeToString, "SQLGetTypeInfo (%d)", SQL_API_SQLGETTYPEINFO);
					break;
				default:
					sprintf(ConvertAPITypeToString, "Invalid Catalog API (%d)", APIType);
					break;
			}
			TraceOut(TR_SRVR_KRYPTON_API,"odbc_SQLSvc_GetSQLCatalogs_sme_(%#x, %#x, %ld, %s, %s, %s, %s, %s, %s, %s, %ld, %ld, %ld, %ld, %ld, %d)",
				objtag_,
				call_id_,
				dialogueId,
				stmtLabel,
				ConvertAPITypeToString,
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
				fkcatalogNm,
				fkschemaNm,
				fktableNm);
	}
//LCOV_EXCL_STOP
		
	if (srvrGlobal != NULL && srvrGlobal->srvrType == CORE_SRVR)
	{
		if (srvrGlobal->srvrState == SRVR_CONNECTED)
		{
			if (dialogueId != srvrGlobal->dialogueId)
				exception_.exception_nr = odbc_SQLSvc_GetSQLCatalogs_InvalidConnection_exn_;
			else
				odbc_SQLSvc_GetSQLCatalogs_sme_(objtag_,call_id_,&exception_,
					dialogueId,stmtLabel,APIType,pcatalogNm,pschemaNm,ptableNm,tableTypeList,
					columnNm,columnType,rowIdScope,nullable,uniqueness,accuracy, sqlType,
					metadataId, pfkcatalogNm, pfkschemaNm, pfktableNm, (char *)&catStmtLabel,
					&outputDesc,&sqlWarning);
		}
		else
			exception_.exception_nr = odbc_SQLSvc_GetSQLCatalogs_InvalidConnection_exn_;
	}
	else
		odbc_SQLSvc_GetSQLCatalogs_sme_(objtag_,call_id_,&exception_,
					dialogueId,stmtLabel,APIType,pcatalogNm,pschemaNm,ptableNm,tableTypeList,
					columnNm,columnType,rowIdScope,nullable,uniqueness,accuracy,
					sqlType, metadataId, pfkcatalogNm, pfkschemaNm, pfktableNm, 
					(char *)&catStmtLabel,&outputDesc,&sqlWarning);
	
//LCOV_EXCL_START
	if(diagnostic_flags){
		TraceOut(TR_SRVR_KRYPTON_API,"odbc_SQLSvc_GetSQLCatalogs_res_(%#x, %#x(%ld), %#x, %#x)",
				call_id_,&exception_, exception_.exception_nr, &outputDesc,&sqlWarning);
	} 
//LCOV_EXCL_STOP
	SRVR_STMT_HDL *pSrvrStmt = getSrvrStmt(catStmtLabel, FALSE);
	odbc_SQLSrvr_GetSQLCatalogs_ts_res_(objtag_, call_id_,&exception_, (char *)&catStmtLabel,&outputDesc,&sqlWarning,pSrvrStmt);
	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceGetSQLCatalogsExit(exception_, catStmtLabel, 
								 outputDesc, sqlWarning);
	}
	
} // odbc_SQLSrvr_GetSQLCatalogs_ame_

