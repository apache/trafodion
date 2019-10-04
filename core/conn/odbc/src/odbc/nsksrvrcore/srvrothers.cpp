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
//
// MODULE: SrvrOther.cpp
//
// PURPOSE: Implements the following methods
//		odbc_SQLSvc_Prepare_sme_
//		odbc_SQLSvc_ExecuteN_sme_
//		odbc_SQLSvc_Close_sme_
//		odbc_SQLSvc_FetchN_sme_
//		odbc_SQLSvc_EndTransaction_sme_
//		odbc_SQLSvc_ExecuteCall_sme_
//
#include <platform_ndcs.h>
#include <platform_utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>

//  Used to disable and enable dumps via setrlimit in a release env.
#ifndef _DEBUG
#include <sys/resource.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include "odbcCommon.h"
#include "odbc_sv.h"
#include "srvrcommon.h"
#include "sqlinterface.h"
#include "srvrkds.h"
#include "SQLWrapper.h"
#include "CommonDiags.h"
#include "tdm_odbcSrvrMsg.h"
#include "ResStatistics.h"
#include "ResStatisticsSession.h"
#include "ResStatisticsStatement.h"
#include "NskUtil.h"

// reserved names for seabase metadata where SQL table information is kept
#ifndef SEABASE_MD_SCHEMA
#define SEABASE_MD_SCHEMA     "\"_MD_\""
#define SEABASE_MD_CATALOG     "TRAFODION"
#define SEABASE_COLUMNS         "COLUMNS"
#define SEABASE_DEFAULTS        "DEFAULTS"
#define SEABASE_INDEXES          "INDEXES"
#define SEABASE_KEYS                "KEYS"
#define SEABASE_OBJECTS          "OBJECTS"
#define SEABASE_OBJECTUID      "OBJECTUID"
#define SEABASE_TABLES            "TABLES"
#define SEABASE_VIEWS              "VIEWS"
#define SEABASE_VIEWS_USAGE  "VIEWS_USAGE"
#define SEABASE_VERSIONS        "VERSIONS"
#endif

//#include "zmxocfg.h"                //aruna
#include <dlfcn.h>
#include "sqlcli.h"

#include "TransportBase.h"
// #ifdef _TMP_SQ_SECURITY
#include "secsrvrmxo.h"
#include "dbUserAuth.h"
// #endif

#define SQL_API_TBLSYNONYM		1917
#define SQL_API_TBLMVS			1918

using namespace SRVR;

bool rePrepare2WouldLikeToExecute(Long stmtHandle
				, Int32 *returnCode
				, Int32 *sqlWarningOrErrorLength
				, char *&sqlWarningOrError);

short qrysrvcExecuteFinished(
				  const char *stmtLabel
				, const Long stmtHandle
				, const bool bCheckSqlQueryType
				, const short error_code
				, const bool bFetch
				, const bool bException
				, const bool bErase);

void AllocateAdaptiveSegment(SRVR_STMT_HDL *pSrvrStmt);
void DeallocateAdaptiveSegment(SRVR_STMT_HDL *pSrvrStmt);
void ClearAdaptiveSegment(short adapiveSeg = -1);

static void setAuthenticationError(
   bool & bSQLMessageSet, 
   odbc_SQLSvc_SQLError * SQLError,
   const char * externalUsername,
   bool isInternalError);

// Internal calls - Defined in libcli.so

int SQL_EXEC_AssignParserFlagsForExSqlComp_Internal( /*IN*/ unsigned int flagbits);

int SQL_EXEC_GetParserFlagsForExSqlComp_Internal( /*IN*/ unsigned int &flagbits);

void SQL_EXEC_SetParserFlagsForExSqlComp_Internal( /*IN*/ unsigned int flagbits);

#define INTERNAL_QUERY_FROM_EXEUTIL 0x20000

//#define SRVR_PERFORMANCE

SMD_SELECT_TABLE SQLCommitStmt[] = {
	{ STRING_TYPE, "COMMIT WORK"},
	{ END_OF_TABLE}
};

SMD_SELECT_TABLE SQLRollbackStmt[] = {
	{ STRING_TYPE, "ROLLBACK WORK"},
	{ END_OF_TABLE}
};

SMD_QUERY_TABLE tranQueryTable[] = {
	{"SQL_COMMIT", SQLCommitStmt, TYPE_UNKNOWN, FALSE, FALSE},
	{"SQL_ROLLBACK", SQLRollbackStmt, TYPE_UNKNOWN, FALSE, FALSE},
	{NULL}
};

#define SQL_API_JDBC					9999
#define SQL_API_SQLTABLES_JDBC			SQL_API_SQLTABLES + SQL_API_JDBC
#define SQL_API_SQLGETTYPEINFO_JDBC		SQL_API_SQLGETTYPEINFO + SQL_API_JDBC
#define SQL_API_SQLCOLUMNS_JDBC			SQL_API_SQLCOLUMNS + SQL_API_JDBC
#define SQL_API_SQLSPECIALCOLUMNS_JDBC	SQL_API_SQLSPECIALCOLUMNS + SQL_API_JDBC
#define SQL_API_SQLPROCEDURES_JDBC     SQL_API_SQLPROCEDURES + SQL_API_JDBC
#define SQL_API_SQLPROCEDURECOLUMNS_JDBC      SQL_API_SQLPROCEDURECOLUMNS  + SQL_API_JDBC
// The value represents SQL version, MXCS module major version and MXCS module minor version.
#define MODULE_RELEASE_VERSION			200
#define	MODULE_MAJOR_VERSION			400
#define	MODULE_MINOR_VERSION			000

#define SQL_INVALID_USER_CODE -8837

// ResStatistics
ResStatisticsSession    *resStatSession;
ResStatisticsStatement  *resStatStatement;
BOOL					resStatCollectorError = false;
struct					collect_info setinit;
Int32					inState = STMTSTAT_NONE;
short					inSqlStmtType = TYPE_UNKNOWN;
double					inEstimatedCost = 0;
char					*inQueryId = NULL;
char					*inSqlString = NULL;
Int32					inErrorStatement = 0;
Int32					inWarningStatement = 0;
int64					inRowCount = 0;
Int32					inErrorCode = 0;
Int32					inSqlQueryType = 0;
Int32					inSqlNewQueryType = 0;
char					*inSqlError = NULL;
Int32					inSqlErrorLength = 0;
bool					setStatisticsFlag = FALSE;
// end ResStatistics

char b[317];

bool securitySetup = false;

//char		QueryQueue[1024];
//int64	queryId;

/*
 * Synchronous method function  for
 * operation 'odbc_SQLSvc_Prepare'
 */
extern "C" void
odbc_SQLSvc_Prepare_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLSvc_Prepare_exc_ *exception_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ const IDL_char *stmtExplainLabel
  , /* In    */ IDL_short stmtType
  , /* In    */ IDL_string sqlString
  , /* In    */ IDL_short sqlAsyncEnable
  , /* In    */ Int32 queryTimeout
  , /* Out   */ Int32 *estimatedCost
  , /* Out   */ SQLItemDescList_def *inputDesc
  , /* Out   */ SQLItemDescList_def *outputDesc
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning)
{
	SRVRTRACE_ENTER(FILE_SME+1)

	SRVR_STMT_HDL *pSrvrStmt = NULL;
	SQL_QUERY_COST_INFO cost_info;
	SQL_QUERY_COMPILER_STATS_INFO comp_stats_info;
	SQLRETURN rc = PROGRAM_ERROR;
    Int32 holdestimatedCost;
	bool flag_21036 = false;

	if (sqlString ==  NULL)
	{
		exception_->exception_nr = odbc_SQLSvc_Prepare_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_NULL_SQL_STMT;
	}

	// resource statistics
	if (resStatStatement != NULL && stmtType == EXTERNAL_STMT)
	{
		inState = STMTSTAT_PREPARE;
		inSqlStmtType = TYPE_UNKNOWN;
		inEstimatedCost = 0;
		inQueryId = NULL;
		inSqlString = NULL;
		inErrorStatement = 0;
		inWarningStatement = 0;
		inRowCount = 0;
		inErrorCode = 0;
		inSqlQueryType = SQL_UNKNOWN;
		inSqlNewQueryType = SQL_UNKNOWN;
		inSqlError = NULL;
		inSqlErrorLength = 0;
		bzero(&cost_info, sizeof(cost_info));
   		resStatStatement->start(inState,
					inSqlQueryType,
					stmtLabel,
					NULL,
					inEstimatedCost,
					&flag_21036,
					sqlString);
	}

	if (exception_->exception_nr == 0)
	{
		if (stmtType != TYPE_SMD)
		{

			if ((pSrvrStmt = getSrvrStmt(stmtLabel, FALSE)) != NULL)
			{
				pSrvrStmt->cleanupAll();
				pSrvrStmt->currentMethod = odbc_SQLSvc_Close_ldx_;
				pSrvrStmt->freeResourceOpt = SQL_DROP;
				FREESTATEMENT(pSrvrStmt);
			}
			// Need to validate the stmtLabel
			// Given a label find out the SRVR_STMT_HDL
			if ((pSrvrStmt = getSrvrStmt(stmtLabel, TRUE)) == NULL)
			{
				exception_->exception_nr = odbc_SQLSvc_Prepare_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
			}
			if (exception_->exception_nr == 0)
			{
				if (resStatStatement != NULL && stmtType == EXTERNAL_STMT)
					pSrvrStmt->inState = inState;
				rc = SQL_SUCCESS;
				if (!pSrvrStmt->isReadFromModule)
				{
					// cleanup all memory allocated in the previous operations
					pSrvrStmt->cleanupAll();
					pSrvrStmt->sqlStringLen = strlen(sqlString);
					pSrvrStmt->sqlString  = new char[pSrvrStmt->sqlStringLen+1];
					if (pSrvrStmt->sqlString == NULL)
					{
						SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
									srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
									srvrGlobal->srvrObjRef, 1, "Prepare");
						exit(0);
					}

					strcpy(pSrvrStmt->sqlString, sqlString);
					pSrvrStmt->stmtType = stmtType;
					pSrvrStmt->currentMethod = odbc_SQLSvc_Prepare_ldx_;
					rc = PREPARE(pSrvrStmt);
					switch (rc)
					{
					case SQL_SUCCESS:
						break;
					case SQL_SUCCESS_WITH_INFO:
						GETSQLWARNING(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlWarning);
						break;
					case SQL_ERROR:
						GETSQLERROR(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlError);
						break;
					case ODBC_RG_WARNING:
						// if there is RG_WARNING, we don't pass SQL Warning to the application
						// Hence, we need to clear any warnings
						// call SQL_EXEC_ClearDiagnostics
//						CLEARDIAGNOSTICS(pSrvrStmt);
						rc = SQL_SUCCESS_WITH_INFO;
					case ODBC_SERVER_ERROR:
					case ODBC_RG_ERROR:
					default:
						break;
					}
				}

			}
		}
	/*	else 	//	added for user module support
		{
			char	*sqltoken;
			char	sqlseps[]   = " \t\n{(";
			int		ch = '.';
			int		TknLen = 0;
			int		SStrLen = 0;
			char	VariableValue[1000];

			VariableValue[0] = '\0';
			strcpy(VariableValue,sqlString);
			sqltoken = strtok(VariableValue, sqlseps);
			if (_stricmp(sqltoken, "SMD") == 0)
			{
				sqltoken = strtok(NULL, sqlseps);
				if (sqltoken != NULL)
				{
					UserModuleName[0] = '\0';
					UserStatementName[0] = '\0';
					TknLen = strrchr(sqltoken, ch) - sqltoken + 1;
					SStrLen = strlen(sqltoken);
					strncpy(UserModuleName,sqltoken, TknLen-1);
					UserModuleName[TknLen-1] = '\0';
					strncpy(UserStatementName,sqltoken + TknLen, SStrLen);
					UserStatementName[SStrLen-TknLen+1] = '\0';
					// Need to validate the stmtLabel
					// Given a label find out the SRVR_STMT_HDL
					pSrvrStmt = getSrvrStmt(UserStatementName, TRUE);
					strcpy(pSrvrStmt->stmtName, UserStatementName);
					strcpy(pSrvrStmt->cursorName, UserStatementName);
					pSrvrStmt->stmtType = stmtType;
					user_module.module_name = UserModuleName;
					user_module.module_name_len = strlen(UserModuleName);
					user_module.charset = "ISO88591";
					user_module.creation_timestamp = 1234567890;

					rc = pSrvrStmt->PrepareUserModule();
				}
				else
				{
					// Return Error Invalid Module Name.
				}
				sqltoken = strtok(NULL, sqlseps);
				if (sqltoken != NULL)
				{
					// Return Error Invalid Module Name.
				}
			}
			else
			{
				// Return Error Invalid Call.
			}
		}
		*/
		if (exception_->exception_nr == 0)
		{
			switch (rc)
			{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
				exception_->exception_nr = 0;
				// Copy all the output parameters
				// Vijay - Changes to support not to parse tokens for statement type SELECT
				holdestimatedCost = (Int32)pSrvrStmt->cost_info.totalTime; // SQL returns cost in a strcuture - cost.totalTime
				if ((pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE)  || (pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE))
					pSrvrStmt->sqlStmtType = TYPE_SELECT;
				*estimatedCost = pSrvrStmt->sqlQueryType;
				inputDesc->_length = pSrvrStmt->inputDescList._length;
				inputDesc->_buffer = pSrvrStmt->inputDescList._buffer;
				outputDesc->_length = pSrvrStmt->outputDescList._length;
				outputDesc->_buffer = pSrvrStmt->outputDescList._buffer;
				sqlWarning->_length = pSrvrStmt->sqlWarning._length;
				sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
				break;
			case SQL_STILL_EXECUTING:
				exception_->exception_nr = odbc_SQLSvc_Prepare_SQLStillExecuting_exn_;
				break;

			case ODBC_RG_ERROR:
			case SQL_ERROR:
				ERROR_DESC_def *error_desc_def;
				error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
				if (pSrvrStmt->sqlError.errorList._length != 0 && error_desc_def->sqlcode == -8007)
				{
					exception_->exception_nr = odbc_SQLSvc_Prepare_SQLQueryCancelled_exn_;
					exception_->u.SQLQueryCancelled.sqlcode = error_desc_def->sqlcode;
				}
				else
				{
					exception_->exception_nr = odbc_SQLSvc_Prepare_SQLError_exn_;
					exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
					exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
				}
				break;
			case PROGRAM_ERROR:
				exception_->exception_nr = odbc_SQLSvc_Prepare_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_PREPARE_FAILED;
			default:
				break;
			}
		}
	}

	//  resource statistics
	if (resStatStatement != NULL && stmtType == EXTERNAL_STMT)
	{
		if (exception_->exception_nr != 0 && exception_->u.SQLError.errorList._buffer != NULL)
		{
			inErrorStatement ++;

			ERROR_DESC_def *p_buffer = exception_->u.SQLError.errorList._buffer;
			inErrorCode = p_buffer->sqlcode;
			inSqlError = p_buffer->errorText;
			inSqlErrorLength = strlen(p_buffer->errorText);
		}

		if (sqlWarning->_length != 0)
			inWarningStatement ++;

		if (sqlString == NULL)
			sqlString = "";

		inSqlString = new char[strlen(sqlString) + 1];
		if (inSqlString == NULL)
		{
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
					srvrGlobal->srvrObjRef, 1, "inSqlString");
			exit(0);
		}
		strcpy(inSqlString,sqlString);
		if (pSrvrStmt != NULL)
		{
			inEstimatedCost = pSrvrStmt->cost_info.totalTime;  // res stat reports estimated cost as double
			inQueryId = pSrvrStmt->sqlUniqueQueryID;
			inSqlQueryType = pSrvrStmt->sqlQueryType;
			inSqlNewQueryType = pSrvrStmt->sqlNewQueryType;
		}
		resStatStatement->end(inState,
							  inSqlQueryType,
							  inSqlStmtType,
							  inQueryId,
							  inEstimatedCost,
							  inSqlString,
							  inErrorStatement,
							  inWarningStatement,
							  inRowCount,
							  inErrorCode,
							  resStatSession,
							  inSqlErrorLength,
							  inSqlError,
							  pSrvrStmt,
							  &flag_21036,
							  inSqlNewQueryType);
		delete inSqlString;
	}
	//end rs
	pSrvrStmt->m_need_21036_end_msg = flag_21036;
	SRVRTRACE_EXIT(FILE_SME+1);
	return;
}

/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_ExecuteN'
 */
extern "C" void
odbc_SQLSvc_ExecuteN_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLSvc_ExecuteN_exc_ *exception_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_string cursorName
  , /* In    */ IDL_short sqlStmtType
  , /* In    */ Int32 inputRowCnt
  , /* In    */ const SQLValueList_def *inputValueList
  , /* In    */ IDL_short sqlAsyncEnable
  , /* In    */ Int32 queryTimeout
  , /* Out   */ Int32 *rowsAffected
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning)
{
	SRVRTRACE_ENTER(FILE_SME+2);

	SRVR_STMT_HDL *pSrvrStmt = NULL;
	SQLRETURN rc = SQL_SUCCESS;

	bool bWMAutoCommitOff = false;

	if (inputRowCnt < 0)
	{
		exception_->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_ROW_COUNT;
	}
	else
	{
		if (sqlStmtType == TYPE_SELECT && inputRowCnt > 1)
		{
			exception_->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_ROW_COUNT_AND_SELECT;
		}
		else
		{
			if ((pSrvrStmt = getSrvrStmt(stmtLabel, FALSE)) == NULL)
			{
				exception_->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
			}
		}
	}

	if (exception_->exception_nr == 0)
	{
		// resource statistics
		if (resStatStatement != NULL && pSrvrStmt->stmtType == EXTERNAL_STMT)
		{
			pSrvrStmt->inState = inState = STMTSTAT_EXECUTE;
			inSqlStmtType = sqlStmtType;
			inEstimatedCost = pSrvrStmt->cost_info.totalTime;
			inQueryId = NULL;
			inSqlString = NULL;
			inErrorStatement = 0;
			inWarningStatement = 0;
			inRowCount = 0;
			inErrorCode = 0;
			inSqlError = NULL;
			inSqlErrorLength = 0;

			/*resStatStatement->start(inState,
									pSrvrStmt->sqlQueryType,
									stmtLabel,
									pSrvrStmt->sqlUniqueQueryID,
						pSrvrStmt->cost_info,
						pSrvrStmt->comp_stats_info,
						inEstimatedCost,
						&pSrvrStmt->m_need_21036_end_msg,
						false,
						pSrvrStmt->sqlString); */
			resStatStatement->start(inState,
						pSrvrStmt->sqlQueryType,
						stmtLabel,
						pSrvrStmt,
									inEstimatedCost,
									&pSrvrStmt->m_need_21036_end_msg,
									pSrvrStmt->sqlString);
		}
		//end rs

		if (pSrvrStmt->bSQLMessageSet)
			pSrvrStmt->cleanupSQLMessage();
		if(pSrvrStmt->bSQLValueListSet)
			pSrvrStmt->cleanupSQLValueList();
		pSrvrStmt->inputRowCnt = inputRowCnt;
		pSrvrStmt->sqlStmtType = sqlStmtType;

		if (cursorName != NULL && cursorName[0] != '\0')
		{
			pSrvrStmt->cursorNameLen = strlen(cursorName);
			pSrvrStmt->cursorNameLen = pSrvrStmt->cursorNameLen < sizeof(pSrvrStmt->cursorName)? pSrvrStmt->cursorNameLen : sizeof(pSrvrStmt->cursorName);
			strncpy(pSrvrStmt->cursorName, cursorName, sizeof(pSrvrStmt->cursorName));
			pSrvrStmt->cursorName[sizeof(pSrvrStmt->cursorName)-1] = 0;
		}
		else
			pSrvrStmt->cursorName[0] = '\0';

		pSrvrStmt->inputValueList._buffer = inputValueList->_buffer;
		pSrvrStmt->inputValueList._length = inputValueList->_length;

		pSrvrStmt->currentMethod = odbc_SQLSvc_ExecuteN_ldx_;

		// batch job support for T4
		// Fix for transaction issue 20/09/06
		if ((WSQL_EXEC_Xact(SQLTRANS_STATUS,NULL) != 0) && srvrGlobal->bAutoCommitOn == TRUE && sqlStmtType != TYPE_SELECT)
		{
			bWMAutoCommitOff = true;

			SRVR_STMT_HDL	*TranOffSrvrStmt;
			if ((TranOffSrvrStmt = getSrvrStmt("STMT_TRANS_OFF_1", FALSE)) == NULL)
			{
				exception_->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
			}
			else
			{
				SQLValueList_def *inValueList;
				markNewOperator,inValueList = new SQLValueList_def();
				if (inValueList == NULL)
				{
					SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
							srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
							srvrGlobal->srvrObjRef, 1, "odbc_SQLSvc_ExecuteN_sme_");
					exit(0);
				}
				inValueList->_buffer = NULL;
				inValueList->_length = 0;
				rc = TranOffSrvrStmt->Execute(NULL,1,TYPE_UNKNOWN,inValueList,SQL_ASYNC_ENABLE_OFF,0);
				if (rc == SQL_ERROR)
				{
					exception_->exception_nr = odbc_SQLSvc_ExecuteN_SQLError_exn_;
					exception_->u.SQLError.errorList._length = TranOffSrvrStmt->sqlError.errorList._length;
					exception_->u.SQLError.errorList._buffer = TranOffSrvrStmt->sqlError.errorList._buffer;
				}
				else if (rc != SQL_SUCCESS)
				{
					exception_->exception_nr = 0;
					sqlWarning->_length = TranOffSrvrStmt->sqlWarning._length;
					sqlWarning->_buffer = TranOffSrvrStmt->sqlWarning._buffer;
				}
				delete inValueList;
			}
		}
		if (exception_->exception_nr == 0)
		{
			rc = EXECUTE(pSrvrStmt);
			switch (rc)
			{
			case ODBC_RG_WARNING:
				// if there is RG_WARNING, we don't pass SQL Warning to the application
				// Hence, we need to clear any warnings
				// call SQL_EXEC_ClearDiagnostics
				// CLEARDIAGNOSTICS(pSrvrStmt);
				rc = SQL_SUCCESS_WITH_INFO;
			case SQL_SUCCESS_WITH_INFO:
				GETSQLWARNING(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlWarning);
			case SQL_SUCCESS:
				exception_->exception_nr = 0;
				// Copy the output values
				*rowsAffected = pSrvrStmt->rowsAffected;
				sqlWarning->_length = pSrvrStmt->sqlWarning._length;
				sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
				break;
			case SQL_STILL_EXECUTING:
				exception_->exception_nr = odbc_SQLSvc_ExecuteN_SQLStillExecuting_exn_;
				break;
			case SQL_INVALID_HANDLE:
				exception_->exception_nr = odbc_SQLSvc_ExecuteN_SQLInvalidHandle_exn_;
				break;
			case SQL_NEED_DATA:
				exception_->exception_nr = odbc_SQLSvc_ExecuteN_SQLNeedData_exn_;
				break;
			case SQL_ERROR:
				GETSQLERROR(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlError);
			case ODBC_SERVER_ERROR:
				if (rc == ODBC_SERVER_ERROR)
				{
					// Allocate Error Desc
					kdsCreateSQLErrorException(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlError, 1);
					// Add SQL Error
					kdsCopySQLErrorException(&pSrvrStmt->sqlError, NULL_VALUE_ERROR, NULL_VALUE_ERROR_SQLCODE,
							NULL_VALUE_ERROR_SQLSTATE);
				}
				ERROR_DESC_def *error_desc_def;
				error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
				if (pSrvrStmt->sqlError.errorList._length != 0 && error_desc_def->sqlcode == -8007)
				{
					exception_->exception_nr = odbc_SQLSvc_ExecuteN_SQLQueryCancelled_exn_;
					exception_->u.SQLQueryCancelled.sqlcode = error_desc_def->sqlcode;
				}
				else
				{
					exception_->exception_nr = odbc_SQLSvc_ExecuteN_SQLError_exn_;
					exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
					exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
				}
				break;
			case -8814:
			case 8814:
				rc = SQL_RETRY_COMPILE_AGAIN;
				exception_->exception_nr = odbc_SQLSvc_ExecuteN_SQLRetryCompile_exn_;
				break;
			case PROGRAM_ERROR:
				exception_->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECUTE_FAILED;
			default:
				break;
			}
			if (resStatStatement != NULL)
			{
				resStatStatement->setStatistics(pSrvrStmt);
			}

			// batch job support for T4
			// Fix for transaction issue 20/09/06
			if ( bWMAutoCommitOff )
			{
				SQLValueList_def *inValueList;

				markNewOperator,inValueList = new SQLValueList_def();
				if (inValueList == NULL)
				{
					SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
							srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
							srvrGlobal->srvrObjRef, 1, "odbc_SQLSvc_ExecuteN_sme_");
					exit(0);
				}
				inValueList->_buffer = NULL;
				inValueList->_length = 0;

				if(WSQL_EXEC_Xact(SQLTRANS_STATUS,NULL) == 0)
				{
					SRVR_STMT_HDL	*RbwSrvrStmt;
					SRVR_STMT_HDL	*CmwSrvrStmt;

					if (exception_->exception_nr != 0)
					{
						RbwSrvrStmt = getSrvrStmt("STMT_ROLLBACK_1", FALSE);
						rc = RbwSrvrStmt->Execute(NULL,1,TYPE_UNKNOWN,inValueList,SQL_ASYNC_ENABLE_OFF,0);
					}
					else
					{
						CmwSrvrStmt = getSrvrStmt("STMT_COMMIT_1", FALSE);
						rc = CmwSrvrStmt->Execute(NULL,1,TYPE_UNKNOWN,inValueList,SQL_ASYNC_ENABLE_OFF,0);
						if (rc == SQL_ERROR)
						{
							ERROR_DESC_def *error_desc_def = CmwSrvrStmt->sqlError.errorList._buffer;
							if (CmwSrvrStmt->sqlError.errorList._length != 0 )
							{
								if(error_desc_def->sqlcode != -8605 )
								{
									exception_->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
									if (CEE_TMP_ALLOCATE(call_id_, 50, (void **)&(exception_->u.ParamError.ParamDesc)) == CEE_SUCCESS)
										sprintf(exception_->u.ParamError.ParamDesc, "%s (%d)",SQLSVC_EXCEPTION_EXECUTE_FAILED, error_desc_def->sqlcode);
									else
										exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECUTE_FAILED;
								}
								else
									rc = SQL_SUCCESS;
							}
							else
							{
								exception_->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
								exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECUTE_FAILED;
							}
						}
						else if (rc != SQL_SUCCESS)
						{
							exception_->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
							exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECUTE_FAILED;
						}
					}

				}

				// reset back to original setting
				SRVR_STMT_HDL	*TranOnSrvrStmt;
				if ((TranOnSrvrStmt = getSrvrStmt("STMT_TRANS_ON_1", FALSE)) == NULL)
				{
					exception_->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
					exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
				}
				rc = TranOnSrvrStmt->Execute(NULL,1,TYPE_UNKNOWN,inValueList,SQL_ASYNC_ENABLE_OFF,0);
				if (rc == SQL_ERROR)
				{
					exception_->exception_nr = odbc_SQLSvc_ExecuteN_SQLError_exn_;
					exception_->u.SQLError.errorList._length = TranOnSrvrStmt->sqlError.errorList._length;
					exception_->u.SQLError.errorList._buffer = TranOnSrvrStmt->sqlError.errorList._buffer;
				}
				else if (rc != SQL_SUCCESS)
				{
					exception_->exception_nr = 0;
					sqlWarning->_length = TranOnSrvrStmt->sqlWarning._length;
					sqlWarning->_buffer = TranOnSrvrStmt->sqlWarning._buffer;
				}

				delete inValueList;
			}
		}
	}

	//  resource statistics
	if (resStatStatement != NULL && pSrvrStmt != NULL  && pSrvrStmt->stmtType == EXTERNAL_STMT)
	{
		if (exception_->exception_nr != 0 && exception_->u.SQLError.errorList._buffer != NULL)
		{
			inErrorStatement ++;

			ERROR_DESC_def *p_buffer = exception_->u.SQLError.errorList._buffer;
			inErrorCode = p_buffer->sqlcode;
			inSqlError = p_buffer->errorText;
			inSqlErrorLength = strlen(p_buffer->errorText);
		}
		if (sqlWarning->_length != 0)
			inWarningStatement ++;
		inRowCount = *rowsAffected;
		inQueryId = pSrvrStmt->sqlUniqueQueryID;
		inSqlQueryType = pSrvrStmt->sqlQueryType;
		resStatStatement->end(inState,
							  inSqlQueryType,
							  inSqlStmtType,
							  inQueryId,
							  inEstimatedCost,
							  inSqlString,
							  inErrorStatement,
							  inWarningStatement,
							  inRowCount,
							  inErrorCode,
							  resStatSession,
							  inSqlErrorLength,
							  inSqlError,
							  pSrvrStmt,
							  &pSrvrStmt->m_need_21036_end_msg,
							  pSrvrStmt->sqlNewQueryType,
							  pSrvrStmt->isClosed);
	}
	//end rs
	SRVRTRACE_EXIT(FILE_SME+2);
	return;
}

/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_Prepare2'
 */
extern "C" void
odbc_SQLSvc_Prepare2_sme_(
    /* In    */ Int32 inputRowCnt
  , /* In    */ Int32 sqlStmtType
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_string sqlString
  , /* In    */ Int32 holdableCursor
  , /* In    */ Int32 queryTimeout
  , /* Out   */ Int32 *returnCode
  , /* Out   */ Int32 *sqlWarningOrErrorLength
  , /* Out   */ BYTE *&sqlWarningOrError
  , /* Out   */ Int32 *sqlQueryType
  , /* Out   */ Long *stmtHandle
  , /* Out   */ Int32 *estimatedCost
  , /* Out   */ Int32 *inputDescLength
  , /* Out   */ BYTE *&inputDesc
  , /* Out   */ Int32 *outputDescLength
  , /* Out   */ BYTE *&outputDesc
  , /* In    */ bool isFromExecDirect = false)
{
	SRVRTRACE_ENTER(FILE_SME+18);

	SRVR_STMT_HDL *pSrvrStmt = NULL;
	SQL_QUERY_COST_INFO cost_info;
	SQL_QUERY_COMPILER_STATS_INFO comp_stats_info;
	SQLRETURN rc = SQL_SUCCESS;

	bool bSkipWouldLikeToExecute = false; // some queries have to skip Would Like To Execute
	bool flag_21036 = false;

	if (sqlString ==  NULL)
	{
		*returnCode = SQL_ERROR;
		GETMXCSWARNINGORERROR(-1, "HY090", "Invalid SQL String.", sqlWarningOrErrorLength, sqlWarningOrError);
	}
	else
	{

		// resource statistics
		if (resStatStatement != NULL)
		{
			inState = STMTSTAT_PREPARE;
			inSqlStmtType = TYPE_UNKNOWN;
			inEstimatedCost = 0;
			inQueryId = NULL;
			inSqlString = NULL;
			inErrorStatement = 0;
			inWarningStatement = 0;
			inRowCount = 0;
			inErrorCode = 0;
			inSqlQueryType = SQL_UNKNOWN;
			inSqlNewQueryType = SQL_UNKNOWN;
			inSqlError = NULL;
			inSqlErrorLength = 0;
			bzero(&cost_info, sizeof(cost_info));
   			resStatStatement->start(inState,
									inSqlQueryType,
									stmtLabel,
									NULL,
									inEstimatedCost,
									&flag_21036,
									sqlString);
		}

		if ((pSrvrStmt = getSrvrStmt(stmtLabel, FALSE)) != NULL)
		{
			bSkipWouldLikeToExecute = pSrvrStmt->m_bSkipWouldLikeToExecute;

			if (pSrvrStmt->bSQLMessageSet)
				pSrvrStmt->cleanupSQLMessage();
			if(pSrvrStmt->bSQLValueListSet)
				pSrvrStmt->cleanupSQLValueList();
			pSrvrStmt->currentMethod = odbc_SQLSvc_Close_ldx_;
			pSrvrStmt->freeResourceOpt = SQL_DROP;
			FREESTATEMENT(pSrvrStmt);
		}

		// Need to validate the stmtLabel
		// Given a label find out the SRVR_STMT_HDL
		if ((pSrvrStmt = getSrvrStmt(stmtLabel, TRUE)) == NULL)
		{
				*returnCode = SQL_ERROR;
				GETMXCSWARNINGORERROR(-1, "HY000", "Statement Label could not be allocated.", sqlWarningOrErrorLength, sqlWarningOrError);
		}
	}

	if (*returnCode == 0)
	{
		pSrvrStmt->m_bSkipWouldLikeToExecute = bSkipWouldLikeToExecute;

		*stmtHandle = (Long)pSrvrStmt;
		// cleanup all memory allocated in the previous operations
		pSrvrStmt->cleanupAll();
		if (resStatStatement != NULL)
			pSrvrStmt->inState = inState;
		pSrvrStmt->sqlStringLen = strlen(sqlString) + 1;
		pSrvrStmt->sqlString  = new char[pSrvrStmt->sqlStringLen];
		if (pSrvrStmt->sqlString == NULL)
		{

				SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
						srvrGlobal->srvrObjRef, 1, "Prepare2");
			exit(0);
		}

		strncpy(pSrvrStmt->sqlString, sqlString, pSrvrStmt->sqlStringLen);
		pSrvrStmt->sqlStmtType = (short)sqlStmtType;
		pSrvrStmt->maxRowsetSize = inputRowCnt;
		if (pSrvrStmt->maxRowsetSize == ROWSET_NOT_DEFINED) pSrvrStmt->maxRowsetSize = DEFAULT_ROWSET_SIZE;

		if (srvrGlobal->srvrType == CORE_SRVR)
			AllocateAdaptiveSegment(pSrvrStmt);

		pSrvrStmt->currentMethod = odbc_SQLSvc_PrepareRowset_ldx_;
		pSrvrStmt->holdableCursor = holdableCursor;
		rc = PREPARE2(pSrvrStmt,isFromExecDirect);

		if (srvrGlobal->srvrType == CORE_SRVR && rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
			DeallocateAdaptiveSegment(pSrvrStmt);

		switch (rc)
		{
		case ODBC_RG_WARNING:
		case SQL_SHAPE_WARNING:
		case SQL_SUCCESS_WITH_INFO:
			*returnCode = SQL_SUCCESS_WITH_INFO;
			*estimatedCost = (Int32)pSrvrStmt->cost_info.totalTime; // SQL returns cost in a strcuture - cost.totalTime == estimatedCost
			*sqlQueryType = pSrvrStmt->sqlQueryType;
			*inputDescLength = pSrvrStmt->inputDescBufferLength;
			inputDesc = pSrvrStmt->inputDescBuffer;
			*outputDescLength = pSrvrStmt->outputDescBufferLength;
			outputDesc = pSrvrStmt->outputDescBuffer;
			if (rc == SQL_SUCCESS_WITH_INFO)
			{
				GETSQLWARNINGORERROR2(pSrvrStmt);
				*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
				sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
			}
			else if (rc == SQL_SHAPE_WARNING)
			{
				*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
				sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
			}
			else
			{
				char *RGWarningOrError;
				RGWarningOrError = new char[256];
				sprintf(b,"%lf",pSrvrStmt->cost_info.totalTime);
				sprintf(RGWarningOrError, "The query's estimated cost: %.50s exceeded resource management attribute limit set.", b);
				GETMXCSWARNINGORERROR(1, "01000", RGWarningOrError, sqlWarningOrErrorLength, sqlWarningOrError);
				delete RGWarningOrError;
			}
		break;
		case SQL_SUCCESS:
			*estimatedCost = (Int32)pSrvrStmt->cost_info.totalTime; // SQL returns cost in a strcuture - cost.totalTime == estimatedCost
			*sqlQueryType = pSrvrStmt->sqlQueryType;
			*inputDescLength = pSrvrStmt->inputDescBufferLength;
			inputDesc = pSrvrStmt->inputDescBuffer;
			*outputDescLength = pSrvrStmt->outputDescBufferLength;
			outputDesc = pSrvrStmt->outputDescBuffer;
			break;
		case SQL_ERROR:
		case ODBC_RG_ERROR:
			*returnCode = SQL_ERROR;
			if (rc == SQL_ERROR)
			{
				GETSQLWARNINGORERROR2(pSrvrStmt);
				*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
				sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
			}
			else
			{
				char *RGWarningOrError;
				RGWarningOrError = new char[256];
				sprintf(b,"%lf",pSrvrStmt->cost_info.totalTime);
				sprintf(RGWarningOrError, "The query's estimated cost: %.50s exceeded resource management attribute limit set.", b);
				GETMXCSWARNINGORERROR(-1, "HY000", RGWarningOrError, sqlWarningOrErrorLength, sqlWarningOrError);
				delete RGWarningOrError;
			}
			break;
		case PROGRAM_ERROR:
				GETMXCSWARNINGORERROR(-1, "HY000", SQLSVC_EXCEPTION_PREPARE_FAILED, sqlWarningOrErrorLength, sqlWarningOrError);
			break;
		case INFOSTATS_SYNTAX_ERROR:
		case INFOSTATS_STMT_NOT_FOUND:
			*returnCode = SQL_ERROR;
			*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
			sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
			break;

		default:
			break;
		}
	}

	//  resource statistics
	if (resStatStatement != NULL)
	{
		if (*returnCode == SQL_ERROR && pSrvrStmt != NULL && pSrvrStmt->sqlWarningOrError != NULL)
		{
			inErrorCode = *(Int32 *)(pSrvrStmt->sqlWarningOrError+8);
			inErrorStatement ++;
			inSqlError = (char*)pSrvrStmt->sqlWarningOrError + 16;
			inSqlErrorLength =*(Int32 *)(pSrvrStmt->sqlWarningOrError + 12);
		}
		if (*returnCode == SQL_SUCCESS_WITH_INFO)
			inWarningStatement ++;

		if (sqlString == NULL)
			sqlString = "";


		if (pSrvrStmt != NULL)
		{
			inSqlString = new char[pSrvrStmt->sqlStringLen];
			if (inSqlString == NULL)
			{
				SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
						srvrGlobal->srvrObjRef, 1, "inSqlString");
				exit(0);
			}
			strncpy(inSqlString,sqlString,pSrvrStmt->sqlStringLen);

			inEstimatedCost = pSrvrStmt->cost_info.totalTime; // res stat reports estimated cost as double
			inQueryId = pSrvrStmt->sqlUniqueQueryID;
			inSqlQueryType = pSrvrStmt->sqlQueryType;
			inSqlNewQueryType = pSrvrStmt->sqlNewQueryType;
		}

		resStatStatement->end(inState,
							  inSqlQueryType,
							  inSqlStmtType,
							  inQueryId,
							  inEstimatedCost,
							  inSqlString,
							  inErrorStatement,
							  inWarningStatement,
							  inRowCount,
							  inErrorCode,
							  resStatSession,
							  inSqlErrorLength,
							  inSqlError,
							  pSrvrStmt,
							  &flag_21036,
							  inSqlNewQueryType);
		if(inSqlString != NULL)
			delete inSqlString;
	}
	//end rs


	if(pSrvrStmt != NULL)
		pSrvrStmt->m_need_21036_end_msg = flag_21036;

	SRVRTRACE_EXIT(FILE_SME+18);
	return;
}

/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_Prepare2withRowsets'
 */
extern "C" void
odbc_SQLSvc_Prepare2withRowsets_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ Int32 sqlAsyncEnable
  , /* In    */ Int32 queryTimeout
  , /* In    */ Int32 inputRowCnt
  , /* In    */ Int32 sqlStmtType
  , /* In    */ Int32 stmtLength
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ Int32 stmtLabelCharset
  , /* In    */ Int32 cursorLength
  , /* In    */ IDL_string cursorName
  , /* In    */ Int32 cursorCharset
  , /* In    */ Int32 moduleNameLength
  , /* In    */ const IDL_char *moduleName
  , /* In    */ Int32 moduleCharset
  , /* In    */ int64 moduleTimestamp
  , /* In    */ Int32 sqlStringLength
  , /* In    */ IDL_string sqlString
  , /* In    */ Int32 sqlStringCharset
  , /* In    */ Int32 setStmtOptionsLength
  , /* In    */ IDL_string setStmtOptions
  , /* In    */ Int32 holdableCursor
  , /* Out   */ Int32 *returnCode
  , /* Out   */ Int32 *sqlWarningOrErrorLength
  , /* Out   */ BYTE *&sqlWarningOrError
  , /* Out   */ Int32 *sqlQueryType
  , /* Out   */ Long *stmtHandle
  , /* Out   */ Int32 *estimatedCost
  , /* Out   */ Int32 *inputDescLength
  , /* Out   */ BYTE *&inputDesc
  , /* Out   */ Int32 *outputDescLength
  , /* Out   */ BYTE *&outputDesc)
{
	SRVRTRACE_ENTER(FILE_SME+18);

	SRVR_STMT_HDL *pSrvrStmt = NULL;
	SQL_QUERY_COST_INFO cost_info;
	SQL_QUERY_COMPILER_STATS_INFO comp_stats_info;
	SQLRETURN rc = SQL_SUCCESS;
	bool flag_21036 = false;

	if (sqlString ==  NULL)
	{
		*returnCode = SQL_ERROR;
		GETMXCSWARNINGORERROR(-1, "HY090", "Invalid SQL String.", sqlWarningOrErrorLength, sqlWarningOrError);
	}
	else
	{

		// resource statistics
		if (resStatStatement != NULL)
		{
			inState = STMTSTAT_PREPARE;
			inSqlStmtType = TYPE_UNKNOWN;
			inEstimatedCost = 0;
			inQueryId = NULL;
			inSqlString = NULL;
			inErrorStatement = 0;
			inWarningStatement = 0;
			inRowCount = 0;
			inErrorCode = 0;
			inSqlQueryType = SQL_UNKNOWN;
			inSqlNewQueryType = SQL_UNKNOWN;
			inSqlError = NULL;
			inSqlErrorLength = 0;
			bzero(&cost_info, sizeof(cost_info));
   			resStatStatement->start(inState,
									inSqlQueryType,
									stmtLabel,
									NULL,
									inEstimatedCost,
									&flag_21036,
									sqlString);
		}

		if ((pSrvrStmt = getSrvrStmt(stmtLabel, FALSE)) != NULL)
		{
			if (pSrvrStmt->bSQLMessageSet)
				pSrvrStmt->cleanupSQLMessage();
			if(pSrvrStmt->bSQLValueListSet)
				pSrvrStmt->cleanupSQLValueList();
			pSrvrStmt->currentMethod = odbc_SQLSvc_Close_ldx_;
			pSrvrStmt->freeResourceOpt = SQL_DROP;
			FREESTATEMENT(pSrvrStmt);
		}

		// Need to validate the stmtLabel
		// Given a label find out the SRVR_STMT_HDL
		if ((pSrvrStmt = getSrvrStmt(stmtLabel, TRUE)) == NULL)
		{
				*returnCode = SQL_ERROR;
				GETMXCSWARNINGORERROR(-1, "HY000", "Statement Label not found.", sqlWarningOrErrorLength, sqlWarningOrError);
		}
	}
	if (*returnCode == 0)
	{
		*stmtHandle = (Long)pSrvrStmt;
		// cleanup all memory allocated in the previous operations
		pSrvrStmt->cleanupAll();
		if (resStatStatement != NULL)
			pSrvrStmt->inState = inState;
		pSrvrStmt->sqlStringLen = sqlStringLength;
		pSrvrStmt->sqlString  = new char[sqlStringLength];
		if (pSrvrStmt->sqlString == NULL)
		{

				SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
						srvrGlobal->srvrObjRef, 1, "Prepare2");
			exit(0);
		}

		strncpy(pSrvrStmt->sqlString, sqlString, sqlStringLength);
		pSrvrStmt->sqlStmtType = (short)sqlStmtType;
		pSrvrStmt->maxRowsetSize = inputRowCnt;
		if (pSrvrStmt->maxRowsetSize == ROWSET_NOT_DEFINED) pSrvrStmt->maxRowsetSize = DEFAULT_ROWSET_SIZE;

// this part is for NAR (not Atomic Rowset Recovery)
		if (pSrvrStmt->maxRowsetSize > 1)
//            && (pSrvrStmt->sqlStmtType == TYPE_INSERT_PARAM))
//	        || pSrvrStmt->sqlStmtType == TYPE_UPDATE
//	        || pSrvrStmt->sqlStmtType == TYPE_DELETE))
		{
			if (srvrGlobal->EnvironmentType & MXO_ROWSET_ERROR_RECOVERY)
				rc = WSQL_EXEC_SetStmtAttr(&pSrvrStmt->stmt, SQL_ATTR_ROWSET_ATOMICITY, SQL_NOT_ATOMIC,0);
			else
				rc = WSQL_EXEC_SetStmtAttr(&pSrvrStmt->stmt, SQL_ATTR_ROWSET_ATOMICITY, SQL_ATOMIC, 0);
			if (rc < 0)
			{
				GETSQLERROR(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlError);
				return;
			}
		}

		if (srvrGlobal->srvrType == CORE_SRVR)
			AllocateAdaptiveSegment(pSrvrStmt);

		pSrvrStmt->currentMethod = odbc_SQLSvc_PrepareRowset_ldx_;
		pSrvrStmt->holdableCursor = holdableCursor;
		rc = PREPARE2withRowsets(pSrvrStmt);

		if (srvrGlobal->srvrType == CORE_SRVR && rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
			DeallocateAdaptiveSegment(pSrvrStmt);

		switch (rc)
		{
		case ODBC_RG_WARNING:
		case SQL_SHAPE_WARNING:
		case SQL_SUCCESS_WITH_INFO:
			*returnCode = SQL_SUCCESS_WITH_INFO;
			*estimatedCost = (Int32)pSrvrStmt->cost_info.totalTime; // SQL returns cost in a strcuture - cost.totalTime == estimatedCost
			if (pSrvrStmt->sqlBulkFetchPossible && pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE)
				*sqlQueryType = 10000;
			else
				*sqlQueryType = pSrvrStmt->sqlQueryType;
			*inputDescLength = pSrvrStmt->inputDescBufferLength;
			inputDesc = pSrvrStmt->inputDescBuffer;
			*outputDescLength = pSrvrStmt->outputDescBufferLength;
			outputDesc = pSrvrStmt->outputDescBuffer;
			if (rc == SQL_SUCCESS_WITH_INFO)
			{
				GETSQLWARNINGORERROR2(pSrvrStmt);
				*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
				sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
			}
			else if (rc == SQL_SHAPE_WARNING)
			{
				*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
				sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
			}
			else
			{
				char RGWarningOrError[256];

				sprintf(b,"%lf",pSrvrStmt->cost_info.totalTime);
				sprintf(RGWarningOrError, "The query's estimated cost: %.50s exceeded resource management attribute limit set.", b);
				GETMXCSWARNINGORERROR(1, "01000", RGWarningOrError, sqlWarningOrErrorLength, sqlWarningOrError);
			}
		break;
		case SQL_SUCCESS:
			*estimatedCost = (Int32)pSrvrStmt->cost_info.totalTime; // SQL returns cost in a strcuture - cost.totalTime == estimatedCost
			if (pSrvrStmt->sqlBulkFetchPossible && pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE)
				*sqlQueryType = 10000;
			else
				*sqlQueryType = pSrvrStmt->sqlQueryType;
			*inputDescLength = pSrvrStmt->inputDescBufferLength;
			inputDesc = pSrvrStmt->inputDescBuffer;
			*outputDescLength = pSrvrStmt->outputDescBufferLength;
			outputDesc = pSrvrStmt->outputDescBuffer;
			break;
		case SQL_ERROR:
		case ODBC_RG_ERROR:
			*returnCode = SQL_ERROR;
			if (rc == SQL_ERROR)
			{
				GETSQLWARNINGORERROR2(pSrvrStmt);
				*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
				sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
			}
			else
			{
				char *RGWarningOrError;
				RGWarningOrError = new char[256];
				sprintf(b,"%lf",pSrvrStmt->cost_info.totalTime);
				sprintf(RGWarningOrError, "The query's estimated cost: %.50s exceeded resource management attribute limit set.", b);
				GETMXCSWARNINGORERROR(-1, "HY000", RGWarningOrError, sqlWarningOrErrorLength, sqlWarningOrError);
				delete RGWarningOrError;
			}
			break;
		case PROGRAM_ERROR:
				GETMXCSWARNINGORERROR(-1, "HY000", SQLSVC_EXCEPTION_PREPARE_FAILED, sqlWarningOrErrorLength, sqlWarningOrError);
			break;
		default:
			break;
		}
	}

	//  resource statistics
	if (resStatStatement != NULL)
	{
		if (*returnCode == SQL_ERROR && pSrvrStmt != NULL && pSrvrStmt->sqlWarningOrError != NULL)
		{
			inErrorCode = *(Int32 *)(pSrvrStmt->sqlWarningOrError+8);
			inErrorStatement ++;
			inSqlError = (char*)pSrvrStmt->sqlWarningOrError + 16;
			inSqlErrorLength =*(Int32 *)(pSrvrStmt->sqlWarningOrError + 12);
		}
		if (*returnCode == SQL_SUCCESS_WITH_INFO)
			inWarningStatement ++;

		if (sqlString == NULL)
			sqlString = "";

		inSqlString = new char[sqlStringLength];
		if (inSqlString == NULL)
		{
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
					srvrGlobal->srvrObjRef, 1, "inSqlString");
			exit(0);
		}
		strncpy(inSqlString,sqlString,sqlStringLength);
		if (pSrvrStmt != NULL)
		{
			inEstimatedCost = pSrvrStmt->cost_info.totalTime; // res stat reports estimated cost as double
			inQueryId = pSrvrStmt->sqlUniqueQueryID;
			inSqlQueryType = pSrvrStmt->sqlQueryType;
			inSqlNewQueryType = pSrvrStmt->sqlNewQueryType;
		}
		resStatStatement->end(inState,
							  inSqlQueryType,
							  inSqlStmtType,
							  inQueryId,
							  inEstimatedCost,
							  inSqlString,
							  inErrorStatement,
							  inWarningStatement,
							  inRowCount,
							  inErrorCode,
							  resStatSession,
							  inSqlErrorLength,
							  inSqlError,
							  pSrvrStmt,
							  &flag_21036,
							  inSqlNewQueryType);
		delete inSqlString;
	}
	//end rs
	pSrvrStmt->m_need_21036_end_msg = flag_21036;
	SRVRTRACE_EXIT(FILE_SME+18);
	return;
}  // end SQLSvc_Prepare2withRowsets_sme

/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_Execute2'
 */
extern "C" void
odbc_SQLSvc_Execute2_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ Int32 sqlAsyncEnable
  , /* In    */ Int32 queryTimeout
  , /* In    */ Int32 inputRowCnt
  , /* In    */ Int32 sqlStmtType
  , /* In    */ Long stmtHandle
  , /* In    */ Int32 cursorLength
  , /* In    */ IDL_string cursorName
  , /* In    */ Int32 cursorCharset
  , /* In    */ Int32 holdableCursor
  , /* In    */ Int32 inValuesLength
  , /* In    */ BYTE *inValues
  , /* Out   */ Int32 *returnCode
  , /* Out   */ Int32 *sqlWarningOrErrorLength
  , /* Out   */ BYTE *&sqlWarningOrError
  , /* Out   */ Int32 *rowsAffected
  , /* Out   */ Int32 *outValuesLength
  , /* Out   */ BYTE *&outValues)
{
	SRVRTRACE_ENTER(FILE_SME+19);

	bool bRePrepare2 = false;
	SRVR_STMT_HDL *pSrvrStmt = NULL;
	SQLRETURN rc = SQL_SUCCESS;

	if ((pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle) == NULL)
	{
		*returnCode = SQL_ERROR;
		GETMXCSWARNINGORERROR(-1, "HY000", "Statement Label not found.", sqlWarningOrErrorLength, sqlWarningOrError);
	}
	else
	{
		if (pSrvrStmt->current_holdableCursor != holdableCursor)
		{
			rePrepare2( pSrvrStmt
				  , sqlStmtType
				  , inputRowCnt
				  , holdableCursor
                                  , queryTimeout
				  , &rc
				  , returnCode
				  , sqlWarningOrErrorLength
				  , sqlWarningOrError
				  );
			bRePrepare2 = true;
		}

		if (*returnCode == 0 || *returnCode == 1)
		{
		// resource statistics
			// generate the actual start message after reprepare, if any.
		if (resStatStatement != NULL && pSrvrStmt->stmtType == EXTERNAL_STMT)
		{
			pSrvrStmt->inState = inState = STMTSTAT_EXECUTE;
			inSqlStmtType = sqlStmtType;
			inEstimatedCost = pSrvrStmt->cost_info.totalTime;
			inQueryId = NULL;
			inSqlString = NULL;
			inErrorStatement = 0;
			inWarningStatement = 0;
			inRowCount = 0;
			inErrorCode = 0;
				inSqlError = NULL;
				inSqlErrorLength = 0;
			// For UPSERT statements force it as INSERT if the driver has sent a unknown type.
			if( inSqlStmtType == TYPE_UNKNOWN && (pSrvrStmt->sqlQueryType == SQL_INSERT_UNIQUE || pSrvrStmt->sqlQueryType == SQL_INSERT_NON_UNIQUE) )
				inSqlStmtType = TYPE_INSERT;
		}
		//end rs

		if (inputRowCnt < 0)
		{
			*returnCode = SQL_ERROR;
			GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Row Count.", sqlWarningOrErrorLength, sqlWarningOrError);
		}
		else
		{
			if (sqlStmtType == TYPE_SELECT && inputRowCnt > 1)
			{
				*returnCode = SQL_ERROR;
				GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Row Count.", sqlWarningOrErrorLength, sqlWarningOrError);
			}
		}

		if (*returnCode == 0 || *returnCode == 1)
		{
			if ((*returnCode == 0) && (pSrvrStmt->sqlWarningOrErrorLength > 0)) // To preserve warning returned at prepare time
			{
				if (pSrvrStmt->sqlWarningOrError != NULL)
					delete pSrvrStmt->sqlWarningOrError;
				pSrvrStmt->sqlWarningOrErrorLength = 0;
				pSrvrStmt->sqlWarningOrError = NULL;
			}
			pSrvrStmt->inputRowCnt = inputRowCnt;
			pSrvrStmt->sqlStmtType = (short)sqlStmtType;

			if (cursorLength > 0)
			{
				pSrvrStmt->cursorNameLen = cursorLength;
				memcpy(pSrvrStmt->cursorName, cursorName, cursorLength);
				pSrvrStmt->cursorName[cursorLength] = '\0';
			}
			else
				pSrvrStmt->cursorName[0] = '\0';

			if (pSrvrStmt->sqlQueryType == SQL_RWRS_SPECIAL_INSERT)
			{
				//memcpy(pSrvrStmt->inputDescVarBuffer, (void *)&pSrvrStmt->inputRowCnt , sizeof(pSrvrStmt->inputRowCnt) );
				//memcpy(pSrvrStmt->inputDescVarBuffer+4, (void *)&pSrvrStmt->maxRowLen, sizeof(pSrvrStmt->maxRowLen) );
				*((Int32 *)pSrvrStmt->inputDescVarBuffer) = pSrvrStmt->inputRowCnt;
				*((Int32 *)(pSrvrStmt->inputDescVarBuffer+4)) = pSrvrStmt->maxRowLen;
				//*((Int32)pSrvrStmt->inputDescVarBuffer+8) = inValues  ;
				*((BYTE **)(pSrvrStmt->inputDescVarBuffer+8)) = inValues  ;

			}
			else
			{

				if (pSrvrStmt->inputDescVarBufferLen == inValuesLength)
					memcpy(pSrvrStmt->inputDescVarBuffer, inValues, inValuesLength);
				else
				{
					*returnCode = SQL_ERROR;
					GETMXCSWARNINGORERROR(-1, "HY090", "Invalid param Values.", sqlWarningOrErrorLength, sqlWarningOrError);
				}
			}

				if (bRePrepare2)
				{
					rc = rePrepare2WouldLikeToExecute((Long)pSrvrStmt, (Int32*)returnCode, (Int32*)sqlWarningOrErrorLength, (char*&)sqlWarningOrError);
					if (rc == false)
					{
						*rowsAffected = -1;
						if ((resStatStatement != NULL) && (pSrvrStmt->stmtType == EXTERNAL_STMT))
						{
							// generate 21036 start message
							resStatStatement->start(inState,
											pSrvrStmt->sqlQueryType,
											pSrvrStmt->stmtName,
											pSrvrStmt,
											inEstimatedCost,
											&pSrvrStmt->m_need_21036_end_msg,
											pSrvrStmt->sqlString);
						}
						goto out0;
					}
				}

				if (resStatStatement != NULL && pSrvrStmt->stmtType == EXTERNAL_STMT)
				{
					resStatStatement->start(inState,
											pSrvrStmt->sqlQueryType,
											pSrvrStmt->stmtName,
											pSrvrStmt,
											inEstimatedCost,
											&pSrvrStmt->m_need_21036_end_msg,
											pSrvrStmt->sqlString);
				}

			pSrvrStmt->currentMethod = odbc_SQLSvc_ExecuteN_ldx_;
			rc = EXECUTE2(pSrvrStmt);

//			char tmpString[32];
//			tmpString[0] = '\0';
//			sprintf(tmpString, "e: %Ld", pSrvrStmt->cliElapseTime);
//			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
//					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
//					srvrGlobal->srvrObjRef, 1, tmpString);

			switch (rc)
			{
			case SQL_SUCCESS_WITH_INFO:
				*returnCode = SQL_SUCCESS_WITH_INFO;
				*rowsAffected = pSrvrStmt->rowsAffected;
				if (   pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE
				    || pSrvrStmt->sqlStmtType  == TYPE_CALL)
				{
					*outValuesLength = pSrvrStmt->outputDescVarBufferLen;
					outValues = pSrvrStmt->outputDescVarBuffer;
				}
				else
				{
					if (pSrvrStmt->sqlQueryType == SQL_RWRS_SPECIAL_INSERT)
					{
						if (inValues != NULL)
							*pSrvrStmt->inputDescVarBuffer = NULL;
					}
					*outValuesLength = 0;
					outValues = 0;
				}

				if (pSrvrStmt->sqlWarningOrErrorLength > 0) // overwriting warning returned at prepare time
				{
					if (pSrvrStmt->sqlWarningOrError != NULL)
						delete pSrvrStmt->sqlWarningOrError;
					pSrvrStmt->sqlWarningOrErrorLength = 0;
					pSrvrStmt->sqlWarningOrError = NULL;
				}

				GETSQLWARNINGORERROR2(pSrvrStmt);
				*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
				sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
				break;
			case SQL_SUCCESS:
				*returnCode = SQL_SUCCESS;
				*rowsAffected = pSrvrStmt->rowsAffected;
				if (   pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE
				    || pSrvrStmt->sqlStmtType  == TYPE_CALL)
				{
					*outValuesLength = pSrvrStmt->outputDescVarBufferLen;
					outValues = pSrvrStmt->outputDescVarBuffer;
				}
				else
				{
					if (pSrvrStmt->sqlQueryType == SQL_RWRS_SPECIAL_INSERT)
					{
						if (inValues != NULL)
							*pSrvrStmt->inputDescVarBuffer = NULL;
					}
					*outValuesLength = 0;
					outValues = 0;
				}
				break;
			case SQL_NO_DATA_FOUND:
				*returnCode = SQL_NO_DATA_FOUND;
				break;
			case SQL_INVALID_HANDLE:
				*returnCode = SQL_ERROR;
				GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Statement Handle.", sqlWarningOrErrorLength, sqlWarningOrError);
				break;
			case SQL_ERROR:
				if (pSrvrStmt->sqlWarningOrErrorLength > 0) // Overwriting warning returned at prepare time
				{
					if (pSrvrStmt->sqlWarningOrError != NULL)
						delete pSrvrStmt->sqlWarningOrError;
					pSrvrStmt->sqlWarningOrErrorLength = 0;
					pSrvrStmt->sqlWarningOrError = NULL;
				}

				GETSQLWARNINGORERROR2(pSrvrStmt);
				*returnCode = SQL_ERROR;
				*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
				sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
				break;
			default:
				break;
			}
			if (resStatStatement != NULL)
			{
				// Should avoid collecting statistics for
				// statement types that do not fetch until end of data or close the stmt
				// during execute since it's causing issues in RMS.
					if( pSrvrStmt->sqlQueryType != SQL_SELECT_NON_UNIQUE && pSrvrStmt->sqlQueryType != SQL_CALL_WITH_RESULT_SETS)
						resStatStatement->setStatistics(pSrvrStmt);
				}
			}
out0:
		//  resource statistics
		if (resStatStatement != NULL  && pSrvrStmt->stmtType == EXTERNAL_STMT)
		{
			if (*returnCode == SQL_ERROR && pSrvrStmt->sqlWarningOrError != NULL)
			{
				inErrorCode = *(Int32 *)(pSrvrStmt->sqlWarningOrError+8);
				inErrorStatement ++;
					inSqlError = (char*)pSrvrStmt->sqlWarningOrError + 16;
					inSqlErrorLength =*(Int32 *)(pSrvrStmt->sqlWarningOrError + 12);
			}
			if (*returnCode == SQL_SUCCESS_WITH_INFO)
				inWarningStatement ++;
			if (pSrvrStmt->rowsAffectedHigherBytes != 0)
				inRowCount = -1;
			else
				inRowCount = *rowsAffected;
			inQueryId = pSrvrStmt->sqlUniqueQueryID;
			inSqlQueryType = pSrvrStmt->sqlQueryType;
			resStatStatement->end(inState,
								  inSqlQueryType,
								  inSqlStmtType,
								  inQueryId,
								  inEstimatedCost,
								  inSqlString,
								  inErrorStatement,
								  inWarningStatement,
								  inRowCount,
								  inErrorCode,
								  resStatSession,
									  inSqlErrorLength,
									  inSqlError,
									  pSrvrStmt,
								  &pSrvrStmt->m_need_21036_end_msg,
									  pSrvrStmt->sqlNewQueryType,
								  pSrvrStmt->isClosed);
		}
		//end rs
	}
	}
	SRVRTRACE_EXIT(FILE_SME+19);
	return;
}

//--------------------------------------------------------------------------------
/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_Execute2withRowsets'
 */
extern "C" void
odbc_SQLSvc_Execute2withRowsets_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ Int32 sqlAsyncEnable
  , /* In    */ Int32 queryTimeout
  , /* In    */ Int32 inputRowCnt
  , /* In    */ Int32 sqlStmtType
  , /* In    */ Long stmtHandle
  , /* In    */ Int32 cursorLength
  , /* In    */ IDL_string cursorName
  , /* In    */ Int32 cursorCharset
  , /* In    */ Int32 holdableCursor
  , /* In    */ Int32 inValuesLength
  , /* In    */ BYTE *inValues
  , /* Out   */ Int32 *returnCode
  , /* Out   */ Int32 *sqlWarningOrErrorLength
  , /* Out   */ BYTE *&sqlWarningOrError
  , /* Out   */ Int32 *rowsAffected
  , /* Out   */ Int32 *outValuesLength
  , /* Out   */ BYTE *&outValues)
{
	SRVRTRACE_ENTER(FILE_SME+19);

	bool bRePrepare2 = false;

       /*
        * The performance team wanted to be able to stub out the actual inserts
        * to measure the contributions of individual components to the overall
        * load times. If the env variable mxosrvr-stubout-EXECUTE2withRowsets
        * is set in ms.env, we will skip over the call to EXECUTE2withRowsets
        * and return sql_success, and rowsAffected = input row count
        */
        static bool bCheckStubExecute2WithRowsets = true;
        static bool bStubExecute2WithRowsets = false;

        if(bCheckStubExecute2WithRowsets)
        {
           char *env = getenv("mxosrvr-stubout-EXECUTE2withRowsets");
           if (env != NULL && strcmp(env,"true") == 0)
              bStubExecute2WithRowsets = true;
           bCheckStubExecute2WithRowsets = false;
        }



	SRVR_STMT_HDL *pSrvrStmt = NULL;
	SQLRETURN rc = SQL_SUCCESS;

	if ((pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle) == NULL)
	{
		*returnCode = SQL_ERROR;
		GETMXCSWARNINGORERROR(-1, "HY000", "Statement Label not found.", sqlWarningOrErrorLength, sqlWarningOrError);
	}
	else
	{
		*returnCode = SQL_SUCCESS;
		if (inputRowCnt < 0)
		{
		}
		else if (sqlStmtType == TYPE_SELECT && inputRowCnt > 1)
		{
		}
		else if ((pSrvrStmt->maxRowsetSize < inputRowCnt)  || (pSrvrStmt->current_holdableCursor != holdableCursor))
		{
			rePrepare2( pSrvrStmt
				  , sqlStmtType
				  , inputRowCnt
				  , holdableCursor
				  , queryTimeout
				  ,&rc
				  , returnCode
				  , sqlWarningOrErrorLength
				  , sqlWarningOrError
				  );
			bRePrepare2 = true;
		}

		if (*returnCode == 0 || *returnCode == 1)
		{
			// resource statistics
			// generate the actual start message after reprepare, if any.
			if (resStatStatement != NULL && pSrvrStmt->stmtType == EXTERNAL_STMT)
			{
				pSrvrStmt->inState = inState = STMTSTAT_EXECUTE;
				inSqlStmtType = sqlStmtType;
				inEstimatedCost = pSrvrStmt->cost_info.totalTime;
				inQueryId = NULL;
				inSqlString = NULL;
				inErrorStatement = 0;
				inWarningStatement = 0;
				inRowCount = 0;
				inErrorCode = 0;
				inSqlError = NULL;
				inSqlErrorLength = 0;
			}
			//end rs

			if (inputRowCnt < 0)
			{
				*returnCode = SQL_ERROR;
				GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Row Count.", sqlWarningOrErrorLength, sqlWarningOrError);
			}
			else
			{
				if (sqlStmtType == TYPE_SELECT && inputRowCnt > 1)
				{
					*returnCode = SQL_ERROR;
					GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Row Count.", sqlWarningOrErrorLength, sqlWarningOrError);
				}
			}

			if (*returnCode == 0 || *returnCode == 1)
			{
				// Fix for CR 5763/6389 - added additional checks to make sure the warnings, if any, are not lost from the
				// rePrepare2() call in SrvrConnect.cpp (returnCode could be 0).
				if ((*returnCode == 0) && (pSrvrStmt->sqlWarningOrErrorLength > 0) && pSrvrStmt->reprepareWarn == FALSE) // To preserve warning returned at prepare time
				{
					if (pSrvrStmt->sqlWarningOrError != NULL)
						delete pSrvrStmt->sqlWarningOrError;
					pSrvrStmt->sqlWarningOrErrorLength = 0;
					pSrvrStmt->sqlWarningOrError = NULL;
				}

				if (pSrvrStmt->bSQLMessageSet)
					pSrvrStmt->cleanupSQLMessage();
				if(pSrvrStmt->bSQLValueListSet)
					pSrvrStmt->cleanupSQLValueList();


				if ( (*returnCode == 0 && rc == 0) || (*returnCode == 1 && rc == 1) )
				{
					pSrvrStmt->inputRowCnt = inputRowCnt;
					pSrvrStmt->sqlStmtType = (short)sqlStmtType;

					if (cursorLength > 0)
					{
						pSrvrStmt->cursorNameLen = cursorLength;
						memcpy(pSrvrStmt->cursorName, cursorName, cursorLength);
					}
					else
						pSrvrStmt->cursorName[0] = '\0';


					if (pSrvrStmt->preparedWithRowsets == TRUE)
					{
						pSrvrStmt->transportBuffer = inValues;
						pSrvrStmt->transportBufferLen = inValuesLength;
					}
					else if (pSrvrStmt->inputDescVarBufferLen == inValuesLength)
						memcpy(pSrvrStmt->inputDescVarBuffer, inValues, inValuesLength);
					else
					{
						*returnCode = SQL_ERROR;
						GETMXCSWARNINGORERROR( -1
							 , "HY090"
							 , "Invalid param Values."
								 , sqlWarningOrErrorLength
								 , sqlWarningOrError
								 );
						goto out;
					}

					if (bRePrepare2)
					{
						// Note: The below method ends in a dummy call in CommonNSKFunctions.cpp. CR 5763 takes care of this.
						rc = rePrepare2WouldLikeToExecute((Long)pSrvrStmt, (Int32*)returnCode, (Int32*)sqlWarningOrErrorLength, (char*&)sqlWarningOrError);
						if (rc == false)
						{
							*rowsAffected = -1;
							if ((resStatStatement != NULL) && (pSrvrStmt->stmtType == EXTERNAL_STMT))
							{
								// generate 21036 start message
								resStatStatement->start(inState,
												pSrvrStmt->sqlQueryType,
												pSrvrStmt->stmtName,
												pSrvrStmt,
												inEstimatedCost,
												&pSrvrStmt->m_need_21036_end_msg,
												pSrvrStmt->sqlString);
							}
							goto out;
						}
					}

					// resource statistics
					if (resStatStatement != NULL && pSrvrStmt->stmtType == EXTERNAL_STMT)
					{
						resStatStatement->start(inState,
												pSrvrStmt->sqlQueryType,
												pSrvrStmt->stmtName,
												pSrvrStmt,
												inEstimatedCost,
												&pSrvrStmt->m_need_21036_end_msg,
												pSrvrStmt->sqlString);
					}
					//end rs

					pSrvrStmt->currentMethod = odbc_SQLSvc_ExecuteN_ldx_;
                                        if(!bStubExecute2WithRowsets)
					   rc = EXECUTE2withRowsets(pSrvrStmt);
                                        else {
                                           rc = SQL_SUCCESS;
                                           pSrvrStmt->rowsAffected = inputRowCnt;
                                        }

					switch (rc)
					{
					case ROWSET_SQL_ERROR:
					// Copy the output values
						*rowsAffected = -1;

						if (pSrvrStmt->sqlWarningOrErrorLength > 0) // Overwriting warning returned at prepare time
						{
							if (pSrvrStmt->sqlWarningOrError != NULL)
								delete pSrvrStmt->sqlWarningOrError;
							pSrvrStmt->sqlWarningOrErrorLength = 0;
							pSrvrStmt->sqlWarningOrError = NULL;
						}

						GETSQLWARNINGORERROR2forRowsets(pSrvrStmt);
						*returnCode = SQL_ERROR;
						*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
						sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
						  break;
					case SQL_SUCCESS_WITH_INFO:
						*returnCode = SQL_SUCCESS_WITH_INFO;
						*rowsAffected = pSrvrStmt->rowsAffected;
						if (   pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE
							|| pSrvrStmt->sqlStmtType  == TYPE_CALL)
						{
							*outValuesLength = pSrvrStmt->outputDescVarBufferLen;
							outValues = pSrvrStmt->outputDescVarBuffer;
						}
						else
						{
							*outValuesLength = 0;
							outValues = 0;
						}
						if (pSrvrStmt->sqlWarningOrErrorLength > 0) // Overwriting warning returned at prepare time
						{
							if (pSrvrStmt->sqlWarningOrError != NULL)
								delete pSrvrStmt->sqlWarningOrError;
							pSrvrStmt->sqlWarningOrErrorLength = 0;
							pSrvrStmt->sqlWarningOrError = NULL;
						}

						if (pSrvrStmt->sqlWarning._length > 0)
							GETSQLWARNINGORERROR2forRowsets(pSrvrStmt);
						else
							GETSQLWARNINGORERROR2(pSrvrStmt);
						*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
						sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
						break;
					case SQL_SUCCESS:
						*returnCode = SQL_SUCCESS;
						*rowsAffected = pSrvrStmt->rowsAffected;
						if (pSrvrStmt->sqlWarning._length > 0)
						{
							if (pSrvrStmt->sqlWarningOrErrorLength > 0) // Overwriting warning returned at prepare time
							{
								if (pSrvrStmt->sqlWarningOrError != NULL)
									delete pSrvrStmt->sqlWarningOrError;
								pSrvrStmt->sqlWarningOrErrorLength = 0;
								pSrvrStmt->sqlWarningOrError = NULL;
							}

							GETSQLWARNINGORERROR2forRowsets(pSrvrStmt);
							*returnCode = SQL_SUCCESS_WITH_INFO;  // We have warnings so return success witn info.
							*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
							sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
						}

						if (pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE
							|| pSrvrStmt->sqlStmtType  == TYPE_CALL)
						{
							*outValuesLength = pSrvrStmt->outputDescVarBufferLen;
							outValues = pSrvrStmt->outputDescVarBuffer;
						}
						else
						{
							*outValuesLength = 0;
							outValues = 0;
						}
						break;
					case SQL_NO_DATA_FOUND:
						*returnCode = SQL_NO_DATA_FOUND;
						break;
					case SQL_INVALID_HANDLE:
						*returnCode = SQL_ERROR;
						GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Statement Handle.", sqlWarningOrErrorLength, sqlWarningOrError);
						break;
					case SQL_ERROR:
						if (pSrvrStmt->sqlWarningOrErrorLength > 0) // Overwriting warning returned at prepare time
						{
							if (pSrvrStmt->sqlWarningOrError != NULL)
								delete pSrvrStmt->sqlWarningOrError;
							pSrvrStmt->sqlWarningOrErrorLength = 0;
							pSrvrStmt->sqlWarningOrError = NULL;
						}

						GETSQLWARNINGORERROR2(pSrvrStmt);
						*returnCode = SQL_ERROR;
						*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
						sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
						break;
					default:
						break;
					}
					if (resStatStatement != NULL)
					{
						// We don't need a check here similar to odbc_SQLSvc_Execute2_sme_()
						// since for rowsets we don't call
						// WSQL_EXEC_Exec().
						resStatStatement->setStatistics(pSrvrStmt);
					}
							  //
							  // The following transaction check was taken from Zbig.
							  //

					if (  sqlStmtType == TYPE_INSERT_PARAM
									  && (srvrGlobal->EnvironmentType & MXO_ROWSET_ERROR_RECOVERY)
									  && pSrvrStmt->NA_supported == false
									  && srvrGlobal->bAutoCommitOn == true)
					{
						if (SQL_EXEC_Xact(SQLTRANS_STATUS,NULL) == 0)
						{
								  // transaction is running - do commit/rollback
							SQLValueList_def inValueList;
							inValueList._buffer = NULL;
							inValueList._length = 0;

							if (rc == ROWSET_SQL_ERROR)
							{
								SRVR_STMT_HDL *RbwSrvrStmt = getSrvrStmt("STMT_ROLLBACK_1", FALSE);
								RbwSrvrStmt->Execute(NULL,1,TYPE_UNKNOWN,&inValueList,SQL_ASYNC_ENABLE_OFF,0);
							}
							else
							{
								SRVR_STMT_HDL *CmwSrvrStmt = getSrvrStmt("STMT_COMMIT_1", FALSE);
								CmwSrvrStmt->Execute(NULL,1,TYPE_UNKNOWN,&inValueList,SQL_ASYNC_ENABLE_OFF,0);
							}
						  }
					  }

					}
				}  // end if (*returnCode == 0 && rc == 0)
				//  resource statistics
out:
				if (resStatStatement != NULL && pSrvrStmt->stmtType == EXTERNAL_STMT)
				{
					if (*returnCode == SQL_ERROR && pSrvrStmt->sqlWarningOrError != NULL)
					{
						inErrorCode = *(Int32 *)(pSrvrStmt->sqlWarningOrError+8);
						inErrorStatement ++;
						inSqlError = (char*)pSrvrStmt->sqlWarningOrError + 16;
						inSqlErrorLength =*(Int32 *)(pSrvrStmt->sqlWarningOrError + 12);
					}
					if (*returnCode == SQL_SUCCESS_WITH_INFO)
						inWarningStatement ++;
					if (pSrvrStmt->rowsAffectedHigherBytes != 0)
						inRowCount = -1;
					else
						inRowCount = *rowsAffected;
					inQueryId = pSrvrStmt->sqlUniqueQueryID;
					inSqlQueryType = pSrvrStmt->sqlQueryType;
					resStatStatement->end(inState,
										  inSqlQueryType,
										  inSqlStmtType,
										  inQueryId,
										  inEstimatedCost,
										  inSqlString,
										  inErrorStatement,
										  inWarningStatement,
										  inRowCount,
										  inErrorCode,
										  resStatSession,
										  inSqlErrorLength,
										  inSqlError,
										  pSrvrStmt,
										  &pSrvrStmt->m_need_21036_end_msg,
										  pSrvrStmt->sqlNewQueryType,
										  pSrvrStmt->isClosed);
				}
			}
	}  // end else

outout:
	pSrvrStmt->returnCodeForDelayedError = *returnCode;
	SRVRTRACE_EXIT(FILE_SME+19);
	return;
}  // end odbc_SQLSvc_Execute2withRowsets_sme_

//------------------------------------------------------------------------------
extern "C" void
rePrepare2( SRVR_STMT_HDL *pSrvrStmt
			, Int32			sqlStmtType
			, Int32			inputRowCnt
			, Int32		holdableCursor
			, Int32 queryTimeout
			, SQLRETURN     *rc
			, Int32          *returnCode
			, Int32      *sqlWarningOrErrorLength
			, BYTE          *&sqlWarningOrError
			)
{
	UInt32	tmpSqlStringLen  = pSrvrStmt->sqlStringLen;
	char	*tmpSqlString;
	short	tmpStmtType      = pSrvrStmt->stmtType;
	short	tmpSqlStmtType   = sqlStmtType; // need to do this since PREPARE does not pass this from driver
	Int32	tmpMaxRowsetSize = pSrvrStmt->maxRowsetSize;
	Int32	sqlQueryType;
	Int32	estimatedCost;

	if (pSrvrStmt->sqlWarningOrErrorLength > 0) // To preserve warning returned at prepare time
	{
		if (pSrvrStmt->sqlWarningOrError != NULL)
			delete pSrvrStmt->sqlWarningOrError;
		pSrvrStmt->sqlWarningOrErrorLength = 0;
		pSrvrStmt->sqlWarningOrError = NULL;
	}

	if (pSrvrStmt->bSQLMessageSet)
		pSrvrStmt->cleanupSQLMessage();
	if(pSrvrStmt->bSQLValueListSet)
		pSrvrStmt->cleanupSQLValueList();

	tmpSqlString = new char[tmpSqlStringLen+1];
	if (tmpSqlString == NULL)
    {

			SendEventMsg( MSG_MEMORY_ALLOCATION_ERROR
                                   , EVENTLOG_ERROR_TYPE
									, srvrGlobal->nskProcessInfo.processId
                                   , ODBCMX_SERVER
                                   , srvrGlobal->srvrObjRef
                                   , 1
                                   , "Execute2"
                                   );
		exit(0);
	}
	strcpy(tmpSqlString, pSrvrStmt->sqlString);

  // cleanup all memory allocated in the previous operations
	pSrvrStmt->cleanupAll();
	pSrvrStmt->sqlStringLen = tmpSqlStringLen;

	pSrvrStmt->sqlString  = new char[pSrvrStmt->sqlStringLen+1];
	if (pSrvrStmt->sqlString == NULL)
    {

			SendEventMsg( MSG_MEMORY_ALLOCATION_ERROR
                                   , EVENTLOG_ERROR_TYPE
                                   , srvrGlobal->nskProcessInfo.processId
                                   , ODBCMX_SERVER
                                   , srvrGlobal->srvrObjRef
                                   , 1
                                   , "Execute2"
                                   );
		exit(0);
	}
	strcpy(pSrvrStmt->sqlString, tmpSqlString);

	pSrvrStmt->stmtType      = tmpStmtType;
	pSrvrStmt->sqlStmtType   = tmpSqlStmtType;
	pSrvrStmt->maxRowsetSize = inputRowCnt;
	pSrvrStmt->holdableCursor= holdableCursor;

	if (pSrvrStmt->maxRowsetSize == ROWSET_NOT_DEFINED)
		pSrvrStmt->maxRowsetSize = DEFAULT_ROWSET_SIZE;

	// resource statistics
	if (resStatStatement != NULL && pSrvrStmt->stmtType == EXTERNAL_STMT)
	{
		pSrvrStmt->inState = inState = STMTSTAT_PREPARE;
		inSqlStmtType = TYPE_UNKNOWN;
		inEstimatedCost = 0;
		inQueryId = NULL;
		inSqlString = NULL;
		inErrorStatement = 0;
		inWarningStatement = 0;
		inRowCount = 0;
		inErrorCode = 0;
		inSqlQueryType = SQL_UNKNOWN;
		inSqlNewQueryType = SQL_UNKNOWN;
		inSqlError = NULL;
		inSqlErrorLength = 0;
   		/*resStatStatement->start(inState,
								inSqlQueryType,
								pSrvrStmt->stmtName,
								NULL,
					pSrvrStmt->cost_info,
					pSrvrStmt->comp_stats_info,
					inEstimatedCost,
					&pSrvrStmt->m_need_21036_end_msg,
					false,
					tmpSqlString);*/
   		resStatStatement->start(inState,
					inSqlQueryType,
					pSrvrStmt->stmtName,
					pSrvrStmt,
								inEstimatedCost,
								&pSrvrStmt->m_need_21036_end_msg,
								tmpSqlString);
	}

	*rc = REALLOCSQLMXHDLS(pSrvrStmt); // This is a workaround for executor when we switch between OLTP vs NON-OLTP
	if (*rc < 0)
    {
		GETSQLWARNINGORERROR2(pSrvrStmt);
		*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
		sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
		goto out;
	}

	if (pSrvrStmt->sqlStmtType == TYPE_INSERT_PARAM)
    {
		if (srvrGlobal->EnvironmentType & MXO_ROWSET_ERROR_RECOVERY)
			*rc = WSQL_EXEC_SetStmtAttr(&pSrvrStmt->stmt, SQL_ATTR_ROWSET_ATOMICITY, SQL_NOT_ATOMIC,0);
		else
			*rc = WSQL_EXEC_SetStmtAttr(&pSrvrStmt->stmt, SQL_ATTR_ROWSET_ATOMICITY, SQL_ATOMIC, 0);
		if (*rc < 0)
		{
			GETSQLWARNINGORERROR2(pSrvrStmt);
			goto out;
		}
		WSQL_EXEC_ClearDiagnostics(&pSrvrStmt->stmt);
	}
	if (srvrGlobal->srvrType == CORE_SRVR)
		AllocateAdaptiveSegment(pSrvrStmt);

	pSrvrStmt->currentMethod = odbc_SQLSvc_PrepareRowset_ldx_; // KAS - bug. This should be Prepare2.
	if(pSrvrStmt->maxRowsetSize > 1)
		*rc = PREPARE2withRowsets(pSrvrStmt);
	else
		*rc = PREPARE2(pSrvrStmt);

	if (srvrGlobal->srvrType == CORE_SRVR && *rc != SQL_SUCCESS && *rc != SQL_SUCCESS_WITH_INFO)
		DeallocateAdaptiveSegment(pSrvrStmt);

	switch (*rc)
    {
	case ODBC_RG_WARNING:
    case SQL_SUCCESS_WITH_INFO:
		*returnCode = SQL_SUCCESS_WITH_INFO;
        estimatedCost = (Int32)pSrvrStmt->cost_info.totalTime; // change to double in future
        sqlQueryType = pSrvrStmt->sqlQueryType;

		if (*rc == SQL_SUCCESS_WITH_INFO)
		{
			GETSQLWARNINGORERROR2(pSrvrStmt);
			*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
			sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
		}
		else
		{
			char RGWarningOrError[256];

			sprintf(b,"%lf",pSrvrStmt->cost_info.totalTime);
			sprintf( RGWarningOrError
                  , "The query's estimated cost: %.50s exceeded resource management attribute limit set."
                  , b
                  );
			GETMXCSWARNINGORERROR(1, "01000", RGWarningOrError, sqlWarningOrErrorLength, sqlWarningOrError);
		}
		break;
	case SQL_SUCCESS:
		WSQL_EXEC_ClearDiagnostics(&pSrvrStmt->stmt);
		estimatedCost = (Int32)pSrvrStmt->cost_info.totalTime; // change to double in future
		sqlQueryType = pSrvrStmt->sqlQueryType;
		break;
    case SQL_ERROR:
    case ODBC_RG_ERROR:
		*returnCode = SQL_ERROR;
		if (*rc == SQL_ERROR)
		{
			GETSQLWARNINGORERROR2(pSrvrStmt);
			*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
			sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
		}
		else
		{
			char *RGWarningOrError;

			RGWarningOrError = new char[256];
			sprintf(b,"%lf",pSrvrStmt->cost_info.totalTime);
			sprintf( RGWarningOrError
                  , "The query's estimated cost: %.50s exceeded resource management attribute limit set."
                  , b
                  );
			GETMXCSWARNINGORERROR( -1
                               , "HY000"
                               , RGWarningOrError
                               , sqlWarningOrErrorLength
                               , sqlWarningOrError
                               );
			delete RGWarningOrError;
		}
		break;
	case PROGRAM_ERROR:
		GETMXCSWARNINGORERROR( -1
                              , "HY000"
                              , SQLSVC_EXCEPTION_PREPARE_FAILED
                              , sqlWarningOrErrorLength
                              , sqlWarningOrError
                              );
		break;
	default:
		break;
	}  // end switch
out:
	//  resource statistics
	if (resStatStatement != NULL && pSrvrStmt->stmtType == EXTERNAL_STMT)
	{
		if (*returnCode == SQL_ERROR && pSrvrStmt != NULL && pSrvrStmt->sqlWarningOrError != NULL)
		{
			inErrorCode = *(Int32 *)(pSrvrStmt->sqlWarningOrError+8);
			inErrorStatement ++;
			inSqlError = (char*)pSrvrStmt->sqlWarningOrError + 16;
			inSqlErrorLength =*(Int32 *)(pSrvrStmt->sqlWarningOrError + 12);
		}
		if (*returnCode == SQL_SUCCESS_WITH_INFO)
			inWarningStatement ++;

		if (tmpSqlString == NULL)
			tmpSqlString = "";

		inSqlString = new char[pSrvrStmt->sqlStringLen];
		if (inSqlString == NULL)
		{
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
					srvrGlobal->srvrObjRef, 1, "inSqlString");
			exit(0);
		}
		strncpy(inSqlString,tmpSqlString,pSrvrStmt->sqlStringLen);
		if (pSrvrStmt != NULL)
		{
			inEstimatedCost = pSrvrStmt->cost_info.totalTime; // res stat reports estimated cost as double
			inQueryId = pSrvrStmt->sqlUniqueQueryID;
			inSqlQueryType = pSrvrStmt->sqlQueryType;
			inSqlNewQueryType = pSrvrStmt->sqlNewQueryType;
		}
		resStatStatement->end(inState,
							  inSqlQueryType,
							  inSqlStmtType,
							  inQueryId,
							  inEstimatedCost,
							  inSqlString,
							  inErrorStatement,
							  inWarningStatement,
							  inRowCount,
							  inErrorCode,
							  resStatSession,
							  inSqlErrorLength,
							  inSqlError,
							  pSrvrStmt,
							  &pSrvrStmt->m_need_21036_end_msg,
							  inSqlNewQueryType);
		delete inSqlString;
		delete tmpSqlString;
	}
	//end rs
}  // end rePrepare2

//------------------------------------------------------------------------------
//LCOV_EXCL_START
/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_Fetch2'
 */
extern "C" void
odbc_SQLSvc_Fetch2_sme_(
    /* In    */ CEE_tag_def           objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def       dialogueId
  , /* In    */ Int32              sqlAsyncEnable
  , /* In    */ Int32              queryTimeout
  , /* In    */ Long              stmtHandle
  , /* In    */ Int32              maxRowCnt
  , /* In    */ Int32              cursorLength
  , /* In    */ IDL_string            cursorName
  , /* In    */ Int32              cursorCharset
  , /* Out   */ Int32             *returnCode
  , /* Out   */ Int32             *sqlWarningOrErrorLength
  , /* Out   */ BYTE                *&sqlWarningOrError
  , /* Out   */ Int32             *rowsAffected
  , /* Out   */ Int32             *outValuesFormat
  , /* Out   */ Int32             *outValuesLength
  , /* Out   */ BYTE                 *&outValues)
  {
  SRVRTRACE_ENTER(FILE_SME+20);

  SRVR_STMT_HDL      *pSrvrStmt;

  SQLRETURN rc = SQL_SUCCESS;

  if ((pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle) == NULL)
    {
    *returnCode = SQL_ERROR;
    GETMXCSWARNINGORERROR(-1, "HY000", "Statement Label not found.", sqlWarningOrErrorLength, sqlWarningOrError);
    }
  else if (maxRowCnt < 0)
    {
    *returnCode = SQL_ERROR;
    GETMXCSWARNINGORERROR(-1, "HY000", "Max row count < 0.", sqlWarningOrErrorLength, sqlWarningOrError);
    }
  else if (pSrvrStmt->isClosed)
    *returnCode = SQL_NO_DATA_FOUND;
  else
    {
    pSrvrStmt->maxRowCnt = maxRowCnt;
    // resource statistics
    if (resStatStatement != NULL && pSrvrStmt->stmtType == EXTERNAL_STMT)
      {
      pSrvrStmt->inState = inState = STMTSTAT_FETCH;
      inSqlStmtType      = TYPE_UNKNOWN;
      inEstimatedCost    = 0;
      inQueryId          = NULL;
      inSqlString        = NULL;
      inErrorStatement   = 0;
      inWarningStatement = 0;
      inRowCount         = 0;
      inErrorCode        = 0;
      inSqlError         = NULL;
      inSqlErrorLength   = 0;
      /* resStatStatement->start(inState,
							  pSrvrStmt->sqlQueryType,
							  pSrvrStmt->stmtName,
							  pSrvrStmt->sqlUniqueQueryID,
			  pSrvrStmt->cost_info,
			  pSrvrStmt->comp_stats_info,
			  inEstimatedCost,
			  &pSrvrStmt->m_need_21036_end_msg);*/
      resStatStatement->start(inState,
			  pSrvrStmt->sqlQueryType,
			  pSrvrStmt->stmtName,
			  pSrvrStmt,
							  inEstimatedCost,
							  &pSrvrStmt->m_need_21036_end_msg);
      }  // end if resource statistics

    if (pSrvrStmt->sqlWarningOrErrorLength > 0)
      {
      if (pSrvrStmt->sqlWarningOrError != NULL)
	delete pSrvrStmt->sqlWarningOrError;
      pSrvrStmt->sqlWarningOrErrorLength = 0;
      pSrvrStmt->sqlWarningOrError = NULL;
      }

    if (cursorLength > 0)
      {
      pSrvrStmt->cursorNameLen = cursorLength;
      memcpy(pSrvrStmt->cursorName, cursorName, cursorLength);
      }
    else
      pSrvrStmt->cursorName[0] = '\0';

    pSrvrStmt->currentMethod = odbc_SQLSvc_FetchPerf_ldx_;  // KAS - bug. Should be  Fetch2

    //
    // We will either use fetch bulk (also known as fetch "row wise rowsets") or fetch rowsets
    // (also known as fetch "column wise rowsets").
    //
    if (pSrvrStmt->sqlBulkFetchPossible && pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE)
      {
      if (pSrvrStmt->outputDataValue._buffer != NULL)
	delete pSrvrStmt->outputDataValue._buffer;
      pSrvrStmt->outputDataValue._buffer = NULL;
      pSrvrStmt->outputDataValue._length = 0;
      rc = FETCH2bulk(pSrvrStmt);
      *outValuesFormat = ROWWISE_ROWSETS;
      if (pSrvrStmt->rowsAffected > 0)
	{
        if (pSrvrStmt->outputDataValue._length == 0 && pSrvrStmt->outputDataValue._buffer == NULL)
	  {
	  outValues       = pSrvrStmt->outputDescVarBuffer;
	  *outValuesLength = (Int32)(pSrvrStmt->outputDescVarBufferLen * pSrvrStmt->rowsAffected);
	  }
        else
	  {
	  outValues       = pSrvrStmt->outputDataValue._buffer;
	  *outValuesLength = (Int32)(pSrvrStmt->outputDataValue._length);
          }
	}
      else
        {
	outValues       = NULL;
	*outValuesLength = 0;
	}
      }
    else
      rc = FETCH2(pSrvrStmt, outValuesFormat, outValuesLength, outValues);

    switch (rc)
      {
      case SQL_SUCCESS_WITH_INFO:
	   *returnCode = SQL_SUCCESS_WITH_INFO;
           *rowsAffected = pSrvrStmt->rowsAffected;
	   GETSQLWARNINGORERROR2(pSrvrStmt);
	   *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
	   sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
	   break;
      case SQL_SUCCESS:
	   *returnCode = SQL_SUCCESS;
	   *rowsAffected = pSrvrStmt->rowsAffected;
	   break;
      case SQL_NO_DATA_FOUND:
	   *returnCode = SQL_NO_DATA_FOUND;
	   break;
      case SQL_INVALID_HANDLE:
	   *returnCode = SQL_ERROR;
	   GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Statement Handle.", sqlWarningOrErrorLength, sqlWarningOrError);
	   break;
      case SQL_ERROR:
	   GETSQLWARNINGORERROR2(pSrvrStmt);
	   *returnCode = SQL_ERROR;
	   *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
	   sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
	   break;
      default:
	   break;
      }  // end switch

    if (resStatStatement != NULL && (rc == SQL_NO_DATA_FOUND || (rc == SQL_SUCCESS && *rowsAffected < maxRowCnt)))
      {
      resStatStatement->setStatistics(pSrvrStmt);
      }

    //  resource statistics
    if (resStatStatement != NULL && pSrvrStmt->stmtType == EXTERNAL_STMT)
      {
      if (*returnCode == SQL_ERROR && pSrvrStmt->sqlWarningOrError != NULL)
      {
         inErrorCode = *(Int32 *)(pSrvrStmt->sqlWarningOrError+8);
         inErrorStatement ++;
	     inSqlError = (char*)pSrvrStmt->sqlWarningOrError + 16;
	     inSqlErrorLength =*(Int32 *)(pSrvrStmt->sqlWarningOrError + 12);
      }
      if (*returnCode == SQL_SUCCESS_WITH_INFO)
        inWarningStatement ++;
      inRowCount = *rowsAffected;
      inQueryId = pSrvrStmt->sqlUniqueQueryID;
	  inSqlQueryType = pSrvrStmt->sqlQueryType;
      resStatStatement->end(inState,
							inSqlQueryType,
							inSqlStmtType,
							inQueryId,
							inEstimatedCost,
							inSqlString,
							inErrorStatement,
							inWarningStatement,
							inRowCount,
							inErrorCode,
							resStatSession,
							inSqlErrorLength,
							inSqlError,
							pSrvrStmt,
							&pSrvrStmt->m_need_21036_end_msg,
							pSrvrStmt->sqlNewQueryType,
							pSrvrStmt->isClosed);
      }  // end resStatStatement != NULL
    }  // end if ((pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle) == NULL) else

  SRVRTRACE_EXIT(FILE_SME+20);

  return;

  }  // end odbc_SQLSvc_Fetch2_sme_
//LCOV_EXCL_STOP

/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_Close'
 */
extern "C" void
odbc_SQLSvc_Close_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLSvc_Close_exc_ *exception_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_unsigned_short freeResourceOpt
  , /* Out   */ Int32 *rowsAffected
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
  )
{
	SRVRTRACE_ENTER(FILE_SME+3);

	SRVR_STMT_HDL *pSrvrStmt = NULL;
	SQLRETURN rc = SQL_SUCCESS;

	if (freeResourceOpt != SQL_CLOSE && freeResourceOpt != SQL_DROP &&
		freeResourceOpt != SQL_UNBIND && freeResourceOpt != SQL_RESET_PARAMS)
	{
		exception_->exception_nr = odbc_SQLSvc_Close_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_RESOURCE_OPT_CLOSE;
	}
	else
	{
		if ((pSrvrStmt = getSrvrStmt(stmtLabel, FALSE)) == NULL)
			goto ret; // Statement was never allocated.
		if (freeResourceOpt == SQL_CLOSE && pSrvrStmt->isClosed)
			goto ret;
	}

	if (exception_->exception_nr == 0)
	{
		// resource statistics
		if (resStatStatement != NULL && pSrvrStmt->stmtType == EXTERNAL_STMT)
		{
			pSrvrStmt->inState = inState = STMTSTAT_CLOSE;
			inSqlStmtType = TYPE_UNKNOWN;
			inEstimatedCost = 0;
			inQueryId = NULL;
			inSqlString = NULL;
			inErrorStatement = 0;
			inWarningStatement = 0;
			inRowCount = 0;
			inErrorCode = 0;
			inSqlError = NULL;
			inSqlErrorLength = 0;
			/*resStatStatement->start(inState,
									pSrvrStmt->sqlQueryType,
									stmtLabel,
									pSrvrStmt->sqlUniqueQueryID,
						pSrvrStmt->cost_info,
						pSrvrStmt->comp_stats_info,
						inEstimatedCost,
						&pSrvrStmt->m_need_21036_end_msg);*/
			resStatStatement->start(inState,
						pSrvrStmt->sqlQueryType,
						stmtLabel,
						pSrvrStmt,
									inEstimatedCost,
									&pSrvrStmt->m_need_21036_end_msg);
			// if SQLClose is called after SQLExecute/SQLExecdirect of select stmt,
			// END:SQLFetch/END:QueryExecution still needs to be generated
//			if (pSrvrStmt->isClosed == FALSE) pSrvrStmt->bFetchStarted = TRUE;

			if (pSrvrStmt->isClosed == FALSE)
			{
				pSrvrStmt->bFetchStarted = FALSE;
				if (exception_->exception_nr != 0 && exception_->u.SQLError.errorList._buffer != NULL)
				{
					inErrorStatement ++;

					ERROR_DESC_def *p_buffer = exception_->u.SQLError.errorList._buffer;
					inErrorCode = p_buffer->sqlcode;
					inSqlError = p_buffer->errorText;
					inSqlErrorLength = strlen(p_buffer->errorText);
				}
				inQueryId=pSrvrStmt->sqlUniqueQueryID;
				inSqlQueryType = pSrvrStmt->sqlQueryType;
				resStatStatement->end(inState,
									  inSqlQueryType,
									  inSqlStmtType,
									  inQueryId,
									  inEstimatedCost,
									  inSqlString,
									  inErrorStatement,
									  inWarningStatement,
									  inRowCount,
									  inErrorCode,
									  resStatSession,
									  inSqlErrorLength,
									  inSqlError,
									  pSrvrStmt,
									  &pSrvrStmt->m_need_21036_end_msg,
									  pSrvrStmt->sqlNewQueryType,
									  pSrvrStmt->isClosed);
			}
		}
		//end rs
		rc = SQL_SUCCESS;
		if (pSrvrStmt->stmtType != INTERNAL_STMT)
		{
			if (pSrvrStmt->bSQLMessageSet)
				pSrvrStmt->cleanupSQLMessage();
			if(pSrvrStmt->bSQLValueListSet)
				pSrvrStmt->cleanupSQLValueList();
			pSrvrStmt->freeResourceOpt = freeResourceOpt;
			pSrvrStmt->currentMethod = odbc_SQLSvc_Close_ldx_;

            if (pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
			{
				// The result set can not be re-used, so remove it completely.
			  	freeResourceOpt = SQL_DROP;
			  	pSrvrStmt->freeResourceOpt = freeResourceOpt;
				delete [] pSrvrStmt->SpjProxySyntaxString;
				pSrvrStmt->SpjProxySyntaxString = NULL;
				pSrvrStmt->SpjProxySyntaxStringLen = 0;

			  	// remove the result set from the call statement
                SRVR_STMT_HDL *prev = pSrvrStmt->previousSpjRs;
                SRVR_STMT_HDL *next = pSrvrStmt->nextSpjRs;

                prev->nextSpjRs = next;
                if (next != NULL)
                   next->previousSpjRs = prev;

                // If prev is the call statement itself, and the call statement has no more
			  	// result sets, then close the call statement.
                if (prev->sqlQueryType == SQL_CALL_WITH_RESULT_SETS && prev->nextSpjRs == NULL)
			    	rc = FREESTATEMENT(prev);
			}

			rc = FREESTATEMENT(pSrvrStmt);
			// Return back immediately since the pSrvrStmt is deleted and return SQL_SUCCESS always
			if (freeResourceOpt == SQL_DROP)
			{
				rc = SQL_SUCCESS;
			}
			else
			{
				switch (rc)
				{
				case SQL_SUCCESS:
					break;
				case SQL_SUCCESS_WITH_INFO:
					GETSQLWARNING(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlWarning);
					break;
				case SQL_ERROR:
					GETSQLERROR(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlError);
					break;
				case ODBC_RG_WARNING:
					// if there is RG_WARNING, we don't pass SQL Warning to the application
					// Hence, we need to clear any warnings
					// call SQL_EXEC_ClearDiagnostics
					// CLEARDIAGNOSTICS(pSrvrStmt);
					rc = SQL_SUCCESS_WITH_INFO;
				case ODBC_SERVER_ERROR:
				case ODBC_RG_ERROR:
				default:
					break;
				}
			}
		}

		switch (rc)
		{
		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO:
			exception_->exception_nr = 0;
			if (freeResourceOpt != SQL_DROP)
			{
				*rowsAffected = pSrvrStmt->rowsAffected;
				sqlWarning->_length = pSrvrStmt->sqlWarning._length;
				sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
			}
			else
			{
				*rowsAffected = 0;
				sqlWarning->_length = 0;
				sqlWarning->_buffer = NULL;
			}
			break;
		case SQL_ERROR:
			exception_->exception_nr = odbc_SQLSvc_Close_SQLError_exn_;
			exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
			exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
			break;
		case PROGRAM_ERROR:
			exception_->exception_nr = odbc_SQLSvc_Close_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_CLOSE_FAILED;
		default:
			break;
		}
	}

ret:
/* This code is moved to the begining of this method since pSrvrStmt is deleted in case of a
   SQL_DROP.

	// resource statistics
	if (resStatStatement != NULL && pSrvrStmt != NULL && pSrvrStmt->isClosed == TRUE && pSrvrStmt->bFetchStarted == TRUE && pSrvrStmt->stmtType == EXTERNAL_STMT)
	{
		pSrvrStmt->bFetchStarted = FALSE;
		if (exception_->exception_nr != 0 && exception_->u.SQLError.errorList._buffer != NULL)
		{
			inErrorStatement ++;
			inErrorCode = exception_->u.SQLError.errorList._buffer->sqlcode;
		}
		inQueryId=pSrvrStmt->sqlUniqueQueryID;
		inSqlQueryType = pSrvrStmt->sqlQueryType;
		resStatStatement->end(inState,inSqlQueryType,inSqlStmtType,inQueryId,inEstimatedCost,inSqlString,inErrorStatement,inWarningStatement,inRowCount,inErrorCode,resStatSession,pSrvrStmt->isClosed);
	}
*/
	SRVRTRACE_EXIT(FILE_SME+3);
	return;
}

//------------------------------------------------------------------------------
/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_Close'
 */
extern "C" void
odbc_SQLSrvr_Close_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_unsigned_short freeResourceOpt
  , /* Out   */ Int32 *rowsAffected
  , /* Out   */ Int32 *returnCode
  , /* Out   */ Int32 *sqlWarningOrErrorLength
  , /* Out   */ BYTE *&sqlWarningOrError
  )
{
	SRVRTRACE_ENTER(FILE_SME+3);

	SRVR_STMT_HDL *pSrvrStmt = NULL;
	SQLRETURN rc = SQL_SUCCESS;

	if (freeResourceOpt != SQL_CLOSE && freeResourceOpt != SQL_DROP &&
		freeResourceOpt != SQL_UNBIND && freeResourceOpt != SQL_RESET_PARAMS)
	{
		*returnCode = SQL_ERROR;
		GETMXCSWARNINGORERROR(-1, "HY000", SQLSVC_EXCEPTION_INVALID_RESOURCE_OPT_CLOSE, sqlWarningOrErrorLength, sqlWarningOrError);
	}
	else
	{
		pSrvrStmt = getSrvrStmt(stmtLabel, FALSE);
		if(pSrvrStmt == NULL)
			goto ret; // Statement was never allocated.
		else
		{
			if (pSrvrStmt->sqlWarningOrErrorLength > 0 &&
				pSrvrStmt->sqlWarningOrError != NULL)
			{
	           delete pSrvrStmt->sqlWarningOrError;
			}
            pSrvrStmt->sqlWarningOrErrorLength = 0;
            pSrvrStmt->sqlWarningOrError = NULL;
		}

		if (freeResourceOpt == SQL_CLOSE && pSrvrStmt->isClosed)
			goto ret;
	}

	if (*returnCode == SQL_SUCCESS)
	{
		// resource statistics
        if (resStatStatement != NULL && pSrvrStmt->stmtType == EXTERNAL_STMT)
		{
			pSrvrStmt->inState = inState = STMTSTAT_CLOSE;
			pSrvrStmt->m_bqueryFinish = true;
			inSqlStmtType = TYPE_UNKNOWN;
			inEstimatedCost = 0;
			inQueryId = NULL;
			inSqlString = NULL;
			inErrorStatement = 0;
			inWarningStatement = 0;
			inRowCount = 0;
			inErrorCode = 0;
			inSqlError = NULL;
			inSqlErrorLength = 0;
			/*resStatStatement->start(inState,
									pSrvrStmt->sqlQueryType,
									stmtLabel,
									pSrvrStmt->sqlUniqueQueryID,
						pSrvrStmt->cost_info,
						pSrvrStmt->comp_stats_info,
						inEstimatedCost,
						&pSrvrStmt->m_need_21036_end_msg);*/
			resStatStatement->start(inState,
						pSrvrStmt->sqlQueryType,
						stmtLabel,
						pSrvrStmt,
									inEstimatedCost,
									&pSrvrStmt->m_need_21036_end_msg);

			if (pSrvrStmt->isClosed == FALSE)
			{
				pSrvrStmt->bFetchStarted = FALSE;
				if (*returnCode == SQL_ERROR && pSrvrStmt != NULL && pSrvrStmt->sqlWarningOrError != NULL)
				{
					inErrorStatement ++;
					inErrorCode = *(Int32 *)(pSrvrStmt->sqlWarningOrError+8);
					inSqlError = (char*)pSrvrStmt->sqlWarningOrError + 16;
					inSqlErrorLength =*(Int32 *)(pSrvrStmt->sqlWarningOrError + 12);
				}
				inQueryId=pSrvrStmt->sqlUniqueQueryID;
				inSqlQueryType = pSrvrStmt->sqlQueryType;

				if (pSrvrStmt->m_need_21036_end_msg)
				{
				resStatStatement->end(inState,
									  inSqlQueryType,
									  inSqlStmtType,
									  inQueryId,
									  inEstimatedCost,
									  inSqlString,
									  inErrorStatement,
									  inWarningStatement,
									  inRowCount,
									  inErrorCode,
									  resStatSession,
										inSqlErrorLength,
										inSqlError,
										pSrvrStmt,
									  &pSrvrStmt->m_need_21036_end_msg,
										pSrvrStmt->sqlNewQueryType,
									  pSrvrStmt->isClosed);
			}

			}
		}
		//end rs

		qrysrvcExecuteFinished(NULL, (Long)pSrvrStmt, false, *returnCode, false, false, true);

		if ((resStatStatement != NULL) && (pSrvrStmt->stmtType == EXTERNAL_STMT)) // if statement is on
		{

			resStatStatement->endRepository(pSrvrStmt,
						inSqlErrorLength,
						(BYTE*)inSqlError,
						true);
		}

		rc = SQL_SUCCESS;

		if (pSrvrStmt->stmtType != INTERNAL_STMT)
		{
			if (pSrvrStmt->bSQLMessageSet)
				pSrvrStmt->cleanupSQLMessage();
			if(pSrvrStmt->bSQLValueListSet)
				pSrvrStmt->cleanupSQLValueList();
			pSrvrStmt->freeResourceOpt = freeResourceOpt;
			pSrvrStmt->currentMethod = odbc_SQLSvc_Close_ldx_;

            if (pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
			{
				// The result set can not be re-used, so remove it completely.
			  	freeResourceOpt = SQL_DROP;
			  	pSrvrStmt->freeResourceOpt = freeResourceOpt;

				// Added deletion of proxy syntax
				delete [] pSrvrStmt->SpjProxySyntaxString;
				pSrvrStmt->SpjProxySyntaxString = NULL;
				pSrvrStmt->SpjProxySyntaxStringLen = 0;

			  	// remove the result set from the call statement
                SRVR_STMT_HDL *prev = pSrvrStmt->previousSpjRs;
                SRVR_STMT_HDL *next = pSrvrStmt->nextSpjRs;

                prev->nextSpjRs = next;
                if (next != NULL)
                   next->previousSpjRs = prev;

                // If prev is the call statement itself, and the call statement has no more
			  	// result sets, then close the call statement.
                if (prev->sqlQueryType == SQL_CALL_WITH_RESULT_SETS && prev->nextSpjRs == NULL)
			    	rc = FREESTATEMENT(prev);
			}

			rc = FREESTATEMENT(pSrvrStmt);
			// Return back immediately since the pSrvrStmt is deleted and return SQL_SUCCESS always
			if (freeResourceOpt == SQL_DROP)
			{
				rc = SQL_SUCCESS;
			}
			else
			{
				if(rc == SQL_SUCCESS_WITH_INFO ||
				   rc == SQL_ERROR )
				{
 		           GETSQLWARNINGORERROR2(pSrvrStmt);
  	               *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
	               sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
				}
			}
		}

		switch (rc)
		{
		   case SQL_SUCCESS:
		      *returnCode = SQL_SUCCESS;
			  if (freeResourceOpt != SQL_DROP)
			     *rowsAffected = pSrvrStmt->rowsAffected;
			  else
				  *rowsAffected = 0;
			  break;

		   case SQL_SUCCESS_WITH_INFO:
			  *returnCode = SQL_SUCCESS_WITH_INFO;
			  if (freeResourceOpt != SQL_DROP)
			     *rowsAffected = pSrvrStmt->rowsAffected;
			  else
				*rowsAffected = 0;
 			 break;

		   case SQL_ERROR:
		      *returnCode = SQL_ERROR;
			  break;

		   default:
		      break;
		}
	}

ret:
/* This code is moved to the begining of this method since pSrvrStmt is deleted in case of a
   SQL_DROP.

	// resource statistics
	if (resStatStatement != NULL && pSrvrStmt != NULL && pSrvrStmt->isClosed == TRUE && pSrvrStmt->bFetchStarted == TRUE && pSrvrStmt->stmtType == EXTERNAL_STMT)
	{
		pSrvrStmt->bFetchStarted = FALSE;
		if (*returnCode == SQL_ERROR && pSrvrStmt != NULL && pSrvrStmt->sqlWarningOrError != NULL)
		{
			inErrorStatement ++;
			inErrorCode = *(Int32 *)(pSrvrStmt->sqlWarningOrError+8);
		}
		inQueryId=pSrvrStmt->sqlUniqueQueryID;
		inSqlQueryType = pSrvrStmt->sqlQueryType;
		resStatStatement->end(inState,inSqlQueryType,inSqlStmtType,inQueryId,inEstimatedCost,inSqlString,inErrorStatement,inWarningStatement,inRowCount,inErrorCode,resStatSession,pSrvrStmt->isClosed);
	}
*/
	SRVRTRACE_EXIT(FILE_SME+3);
	return;

} /* odbc_SQLSrvr_Close_sme_() */


/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_FetchN'
 */
extern "C" void
odbc_SQLSvc_FetchN_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLSvc_FetchN_exc_ *exception_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ Int32 maxRowCnt
  , /* In    */ Int32 maxRowLen
  , /* In    */ IDL_short sqlAsyncEnable
  , /* In    */ Int32 queryTimeout
  , /* Out   */ Int32 *rowsAffected
  , /* Out   */ SQLValueList_def *outputValueList
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
  )
{
	SRVRTRACE_ENTER(FILE_SME+4);

	SRVR_STMT_HDL *pSrvrStmt = NULL;
	SQLRETURN rc = SQL_SUCCESS;

	if (maxRowCnt < 0)
	{
		exception_->exception_nr = odbc_SQLSvc_FetchN_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_ROW_COUNT;
	}
	else
	{
		if ((pSrvrStmt = getSrvrStmt(stmtLabel, FALSE)) == NULL)
		{
			exception_->exception_nr = odbc_SQLSvc_FetchN_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
		}
	}
	if (exception_->exception_nr == 0)
	{
		// resource statistics
		if (resStatStatement != NULL && pSrvrStmt->isClosed == FALSE && pSrvrStmt->bFetchStarted == FALSE && pSrvrStmt->stmtType == EXTERNAL_STMT)
		{
			pSrvrStmt->bFetchStarted = TRUE;
			pSrvrStmt->inState = inState = STMTSTAT_FETCH;
			inSqlStmtType = TYPE_UNKNOWN;
			inEstimatedCost = 0;
			inQueryId = NULL;
			inSqlString = NULL;
			inErrorStatement = 0;
			inWarningStatement = 0;
			inRowCount = 0;
			inErrorCode = 0;
			inSqlError = NULL;
			inSqlErrorLength = 0;
			/*resStatStatement->start(inState,
									pSrvrStmt->sqlQueryType,
									stmtLabel,
									pSrvrStmt->sqlUniqueQueryID,
						pSrvrStmt->cost_info,
						pSrvrStmt->comp_stats_info,
						inEstimatedCost,
						&pSrvrStmt->m_need_21036_end_msg);*/
			resStatStatement->start(inState,
						pSrvrStmt->sqlQueryType,
						stmtLabel,
						pSrvrStmt,
									inEstimatedCost,
									&pSrvrStmt->m_need_21036_end_msg);
		}
		 //end rs
		rc = SQL_SUCCESS;
		if (pSrvrStmt->bSQLMessageSet)
			pSrvrStmt->cleanupSQLMessage();
		if (pSrvrStmt->outputValueList._buffer == NULL  || pSrvrStmt->maxRowCnt < maxRowCnt)
		{
			if(pSrvrStmt->bSQLValueListSet)
				pSrvrStmt->cleanupSQLValueList();
			rc = AllocAssignValueBuffer(pSrvrStmt->bSQLValueListSet,&pSrvrStmt->outputDescList,
				&pSrvrStmt->outputValueList, pSrvrStmt->outputDescVarBufferLen,
				maxRowCnt, pSrvrStmt->outputValueVarBuffer);
		}
		else
			// Reset the length to 0, but the _buffer points to array of required SQLValue_defs
			pSrvrStmt->outputValueList._length = 0;

		if (rc == SQL_SUCCESS)
		{
			pSrvrStmt->maxRowCnt = maxRowCnt;
			pSrvrStmt->maxRowLen = maxRowLen;
			pSrvrStmt->currentMethod = odbc_SQLSvc_FetchN_ldx_;
			rc = FETCH(pSrvrStmt);
			switch (rc)
			{
			case SQL_SUCCESS:
				break;
			case SQL_SUCCESS_WITH_INFO:
				GETSQLWARNING(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlWarning);
				break;
			case SQL_ERROR:
				GETSQLERROR(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlError);
				break;
			case ODBC_RG_WARNING:
				// if there is RG_WARNING, we don't pass SQL Warning to the application
				// Hence, we need to clear any warnings
				// call SQL_EXEC_ClearDiagnostics
				// CLEARDIAGNOSTICS(pSrvrStmt);
				rc = SQL_SUCCESS_WITH_INFO;
			case ODBC_SERVER_ERROR:
			case ODBC_RG_ERROR:
			default:
				break;
			}
		}
		switch (rc)
		{
		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO:
			exception_->exception_nr = 0;
			*rowsAffected = pSrvrStmt->rowsAffected;
			outputValueList->_length = pSrvrStmt->outputValueList._length;
			outputValueList->_buffer = pSrvrStmt->outputValueList._buffer;
			sqlWarning->_length = pSrvrStmt->sqlWarning._length;
			sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
			break;
		case SQL_STILL_EXECUTING:
			exception_->exception_nr = odbc_SQLSvc_FetchN_SQLStillExecuting_exn_;
			break;
		case SQL_INVALID_HANDLE:
			exception_->exception_nr = odbc_SQLSvc_FetchN_SQLInvalidHandle_exn_;
			break;
		case SQL_NO_DATA_FOUND:
			exception_->exception_nr = odbc_SQLSvc_FetchN_SQLNoDataFound_exn_;
			break;
		case SQL_ERROR:
			ERROR_DESC_def *error_desc_def;
			error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
			if (pSrvrStmt->sqlError.errorList._length != 0 && error_desc_def->sqlcode == -8007)
			{
				exception_->exception_nr = odbc_SQLSvc_FetchN_SQLQueryCancelled_exn_;
				exception_->u.SQLQueryCancelled.sqlcode = error_desc_def->sqlcode;
			}
			else
			{
				exception_->exception_nr = odbc_SQLSvc_FetchN_SQLError_exn_;
				exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
				exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
			}
			break;
		case PROGRAM_ERROR:
			exception_->exception_nr = odbc_SQLSvc_FetchN_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_FETCH_FAILED;
		default:
			break;
		}
		if (resStatStatement != NULL && (rc == SQL_NO_DATA_FOUND || rc == SQL_ERROR || ((rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) && *rowsAffected < maxRowCnt)))
			resStatStatement->setStatistics(pSrvrStmt);
	}

	//resource statistics
	if (resStatStatement != NULL && pSrvrStmt != NULL && pSrvrStmt->isClosed == TRUE && pSrvrStmt->bFetchStarted == TRUE && pSrvrStmt->stmtType == EXTERNAL_STMT)
	{
		if (rc == SQL_ERROR  && exception_->u.SQLError.errorList._buffer != NULL)
		{
			ERROR_DESC_def *p_buffer = exception_->u.SQLError.errorList._buffer;
			inErrorCode = p_buffer->sqlcode;
			inSqlError = p_buffer->errorText;
			inSqlErrorLength = strlen(p_buffer->errorText);
		}
		pSrvrStmt->bFetchStarted = FALSE;
		Int32 inMaxRowCnt = 0;
		Int32 inMaxRowLen = 0;

		inMaxRowCnt = maxRowCnt;
		inMaxRowLen = maxRowLen;

		if (exception_->exception_nr != 0)
		  inErrorStatement ++;
		else
		  setStatisticsFlag = FALSE;

		if (sqlWarning->_length != 0)
			inWarningStatement ++;
		if (exception_->exception_nr == 5)
		{
			inErrorStatement = 0;
			inWarningStatement = 0;
			setStatisticsFlag = TRUE;
		}
		inQueryId = pSrvrStmt->sqlUniqueQueryID;
		inSqlQueryType = pSrvrStmt->sqlQueryType;
		resStatStatement->setStatisticsFlag(setStatisticsFlag);
		resStatStatement->end(inState,
							  inSqlQueryType,
							  inSqlStmtType,
							  inQueryId,
							  inEstimatedCost,
							  inSqlString,
							  inErrorStatement,
							  inWarningStatement,
							  inRowCount,
							  inErrorCode,
							  resStatSession,
							  inSqlErrorLength,
							  inSqlError,
							  pSrvrStmt,
							  &pSrvrStmt->m_need_21036_end_msg,
							  pSrvrStmt->sqlNewQueryType,
							  pSrvrStmt->isClosed);
	}
	// end rs
	SRVRTRACE_EXIT(FILE_SME+4);
	return;
}

/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_EndTransaction'
 */
extern "C" void
odbc_SQLSvc_EndTransaction_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLSvc_EndTransaction_exc_ *exception_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_unsigned_short transactionOpt
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
 )
{
	SRVRTRACE_ENTER(FILE_SME+5);

	char stmtLabel[MAX_STMT_LABEL_LEN+1];
	SRVR_STMT_HDL	*pSrvrStmt = NULL;
	bool isTransPending = (WSQL_EXEC_Xact(SQLTRANS_STATUS, 0) == 0);
	Int32 rc = SQL_SUCCESS;

	exception_->exception_nr = 0;
	sqlWarning->_buffer = NULL;
	sqlWarning->_length = 0;

	switch (transactionOpt) {
	case SQL_COMMIT:
		if (isTransPending)
			pSrvrStmt = getSrvrStmt("STMT_COMMIT_1", FALSE);
		else
			return;
		break;
	case SQL_ROLLBACK:
		if (isTransPending)
			pSrvrStmt = getSrvrStmt("STMT_ROLLBACK_1", FALSE);
		else
			return;
		break;
	default:
		exception_->exception_nr = odbc_SQLSvc_EndTransaction_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_TRANSACT_OPT;
		return;
	}

	if (pSrvrStmt == NULL)
	{
		exception_->exception_nr = odbc_SQLSvc_EndTransaction_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
	}
	else
	{
		if (pSrvrStmt->bSQLMessageSet)
			pSrvrStmt->cleanupSQLMessage();
		if(pSrvrStmt->bSQLValueListSet)
			pSrvrStmt->cleanupSQLValueList();
		pSrvrStmt->inputRowCnt = 1;
		pSrvrStmt->sqlStmtType = TYPE_UNKNOWN;
		pSrvrStmt->cursorName[0] = '\0';
		pSrvrStmt->cursorNameLen = 0;

		pSrvrStmt->inputValueList._buffer = NULL;
		pSrvrStmt->inputValueList._length = 0;

		pSrvrStmt->currentMethod = odbc_SQLSvc_ExecuteN_ldx_;

		rc = EXECUTE(pSrvrStmt);
		if (rc == SQL_ERROR)
		{
			GETSQLERROR(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlError);
			ERROR_DESC_def *error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
			if (pSrvrStmt->sqlError.errorList._length != 0 )
			{
				if (error_desc_def != NULL && (error_desc_def->sqlcode == -8605 || error_desc_def->sqlcode == -8607 ||	error_desc_def->sqlcode == -8609))
				{
					exception_->exception_nr = 0;
					sqlWarning->_length = 0;
					sqlWarning->_buffer = NULL;
				}
				else
				{
					exception_->exception_nr = odbc_SQLSvc_EndTransaction_SQLError_exn_;
					exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
					exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
				}
			}
			else
			{
				exception_->exception_nr = odbc_SQLSvc_EndTransaction_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_TRANSACT_OPT;
			}
		}
		else if (rc != SQL_SUCCESS)
		{
			GETSQLWARNING(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlWarning);
			exception_->exception_nr = 0;
			sqlWarning->_length = pSrvrStmt->sqlWarning._length;
			sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
		}
	}

	SRVRTRACE_EXIT(FILE_SME+5);
	return;
}
//LCOV_EXCL_START
/*
 * Synchronous method function prototype for
 * operation 'odbc_SQLSvc_ExecDirect'
 */
extern "C" void
odbc_SQLSvc_ExecDirect_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLSvc_ExecDirect_exc_ *exception_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_string cursorName
  , /* In    */ const IDL_char *stmtExplainLabel
  , /* In    */ IDL_short stmtType
  , /* In    */ IDL_short sqlStmtType
  , /* In    */ IDL_string sqlString
  , /* In    */ IDL_short sqlAsyncEnable
  , /* In    */ Int32 queryTimeout
  , /* Out   */ Int32 *estimatedCost
  , /* Out   */ SQLItemDescList_def *outputDesc
  , /* Out   */ Int32 *rowsAffected
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning)
{
	SRVRTRACE_ENTER(FILE_SME+6);

	SRVR_STMT_HDL *pSrvrStmt = NULL;
	SQLRETURN rc = SQL_SUCCESS;
	Int32 holdestimatedCost;

	if (sqlString ==  NULL)
	{
		exception_->exception_nr = odbc_SQLSvc_ExecDirect_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_NULL_SQL_STMT;
	}
	else
	{
		// resource statistics
		if (resStatStatement != NULL && stmtType == EXTERNAL_STMT)
		{
			inState = STMTSTAT_EXECDIRECT;
			inSqlStmtType = sqlStmtType;
			inEstimatedCost = 0;
			inQueryId = NULL;
			inSqlString = NULL;
			inErrorStatement = 0;
			inWarningStatement = 0;
			inRowCount = 0;
			inErrorCode = 0;
			inSqlError = NULL;
			inSqlErrorLength = 0;
//			resStatStatement->start(inState, stmtLabel, NULL, inEstimatedCost, sqlString); called in EXECDIRECT
		}
		//end rs

		if ((pSrvrStmt = getSrvrStmt(stmtLabel, FALSE)) != NULL)
		{
			pSrvrStmt->cleanupAll();
			pSrvrStmt->currentMethod = odbc_SQLSvc_Close_ldx_;
			pSrvrStmt->freeResourceOpt = SQL_DROP;
			FREESTATEMENT(pSrvrStmt);
		}
		// Need to validate the stmtLabel
		// Given a label find out the SRVR_STMT_HDL
		if ((pSrvrStmt = getSrvrStmt(stmtLabel, TRUE)) == NULL)
		{
			exception_->exception_nr = odbc_SQLSvc_ExecDirect_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
		}
	}

	if (exception_->exception_nr == 0)
	{
		pSrvrStmt->cleanupAll();
		if (resStatStatement != NULL && stmtType == EXTERNAL_STMT)
			pSrvrStmt->inState = inState;
		pSrvrStmt->sqlStringLen = strlen(sqlString);
		pSrvrStmt->sqlString  = new char[pSrvrStmt->sqlStringLen+1];
		if (pSrvrStmt->sqlString == NULL)
		{

				SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
				srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
				srvrGlobal->srvrObjRef, 1, "ExecDirect");
			exit(0);
		}
		strcpy(pSrvrStmt->sqlString, sqlString);
		pSrvrStmt->sqlStmtType = sqlStmtType;

		if (exception_->exception_nr == 0)
		{
			pSrvrStmt->stmtType = stmtType;
			if (cursorName != NULL && cursorName[0] != '\0')
			{
				pSrvrStmt->cursorNameLen = strlen(cursorName);
				pSrvrStmt->cursorNameLen = pSrvrStmt->cursorNameLen < sizeof(pSrvrStmt->cursorName)? pSrvrStmt->cursorNameLen : sizeof(pSrvrStmt->cursorName);
				strncpy(pSrvrStmt->cursorName, cursorName, sizeof(pSrvrStmt->cursorName));
				pSrvrStmt->cursorName[sizeof(pSrvrStmt->cursorName)-1] = 0;
			}
			else
				pSrvrStmt->cursorName[0] = '\0';

			pSrvrStmt->currentMethod = odbc_SQLSvc_ExecDirect_ldx_;
			rc = EXECDIRECT(pSrvrStmt);
			switch (rc)
			{
			case SQL_SUCCESS:
				break;
			case SQL_SUCCESS_WITH_INFO:
				GETSQLWARNING(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlWarning);
				break;
			case SQL_ERROR:
				GETSQLERROR(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlError);
				break;
			case ODBC_RG_WARNING:
				// if there is RG_WARNING, we don't pass SQL Warning to the application
				// Hence, we need to clear any warnings
				// call SQL_EXEC_ClearDiagnostics
				// CLEARDIAGNOSTICS(pSrvrStmt);
				rc = SQL_SUCCESS_WITH_INFO;
			case ODBC_SERVER_ERROR:
			case ODBC_RG_ERROR:
			default:
				break;
			}
			switch (rc)
			{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
				break;
			default:
				break;
			}
			switch (rc)
			{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
				exception_->exception_nr = 0;
				// Vijay - Changes to support not to parse tokens for statement type SELECT
				holdestimatedCost = (Int32)pSrvrStmt->cost_info.totalTime; // SQL returns cost in a strcuture - cost.totalTime
				if ((pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE)  || (pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE))
					pSrvrStmt->sqlStmtType = TYPE_SELECT;
				*estimatedCost = pSrvrStmt->sqlQueryType;
				*rowsAffected = pSrvrStmt->rowsAffected;
				outputDesc->_length = pSrvrStmt->outputDescList._length;
				outputDesc->_buffer = pSrvrStmt->outputDescList._buffer;
				sqlWarning->_length = pSrvrStmt->sqlWarning._length;
				sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
				break;
			case SQL_STILL_EXECUTING:
				exception_->exception_nr = odbc_SQLSvc_ExecDirect_SQLStillExecuting_exn_;
				break;
			case ODBC_RG_ERROR:
			case SQL_ERROR:
			case INFOSTATS_STMT_NOT_FOUND:
				ERROR_DESC_def *error_desc_def;
				error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
				if (pSrvrStmt->sqlError.errorList._length != 0 && error_desc_def->sqlcode == -8007)
				{
					exception_->exception_nr = odbc_SQLSvc_ExecDirect_SQLQueryCancelled_exn_;
					exception_->u.SQLQueryCancelled.sqlcode = error_desc_def->sqlcode;
				}
				else
				{
					exception_->exception_nr = odbc_SQLSvc_ExecDirect_SQLError_exn_;
					exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
					exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
				}
				break;
			case PROGRAM_ERROR:
				exception_->exception_nr = odbc_SQLSvc_ExecDirect_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECDIRECT_FAILED;
				break;
			default:
				break;
			}
			if (resStatStatement != NULL)
				resStatStatement->setStatistics(pSrvrStmt);
		}
	}
	// resource statistics
	if (resStatStatement != NULL && stmtType == EXTERNAL_STMT)
	{
		if (exception_->exception_nr != 0 && exception_->u.SQLError.errorList._buffer != NULL)
		{
			inErrorStatement ++;

			ERROR_DESC_def *p_buffer = exception_->u.SQLError.errorList._buffer;
			inErrorCode = p_buffer->sqlcode;
			inSqlError = p_buffer->errorText;
			inSqlErrorLength = strlen(p_buffer->errorText);
		}

		if (sqlWarning->_length != 0)
			inWarningStatement ++;
		inRowCount = *rowsAffected;
		if (sqlString == NULL)
			sqlString = "";

		inSqlString = new char[strlen(sqlString) + 1];
		if (inSqlString == NULL)
		{
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
					srvrGlobal->srvrObjRef, 1, "inSqlString");
			exit(0);
		}
		strcpy(inSqlString,sqlString);
		if (pSrvrStmt != NULL)
		{
			if (holdestimatedCost == -1)
				inEstimatedCost = 0;
			else
				inEstimatedCost = pSrvrStmt->cost_info.totalTime; // res stat reports estimated cost as double
			inQueryId = pSrvrStmt->sqlUniqueQueryID;
			inSqlQueryType = pSrvrStmt->sqlQueryType;
			inSqlNewQueryType = pSrvrStmt->sqlNewQueryType;
		}
		resStatStatement->end(inState,
							  inSqlQueryType,
							  inSqlStmtType,
							  inQueryId,
							  inEstimatedCost,
							  inSqlString,
							  inErrorStatement,
							  inWarningStatement,
							  inRowCount,
							  inErrorCode,
							  resStatSession,
							  inSqlErrorLength,
							  inSqlError,
							  pSrvrStmt,
							  &pSrvrStmt->m_need_21036_end_msg,
							  inSqlNewQueryType,
							  pSrvrStmt->isClosed);
		delete inSqlString;
	}
	//end rs
	SRVRTRACE_EXIT(FILE_SME+6);
	return;
}
//LCOV_EXCL_STOP


// Cut extra parts of varchar in outputDataValue  to make data compaction
long long  clipVarchar(SRVR_STMT_HDL *pSrvrStmt )
{
    if(srvrGlobal->clipVarchar == 0)
    {
        return pSrvrStmt->outputDescVarBufferLen*pSrvrStmt->rowsAffected;
    }
    BYTE * desc = pSrvrStmt->outputDescBuffer; 
    BYTE *VarPtr = pSrvrStmt->outputDescVarBuffer;
    long long remainLen = 0;
    int numEntries = pSrvrStmt->columnCount;
    int rowsAffected = pSrvrStmt->rowsAffected ;
    int bufferRowLen = pSrvrStmt->outputDescVarBufferLen;
    long long * colBuferLen = new long long [numEntries] ;
    long startOffset = 0;
    long varcharCount = 0; //counter of varchar column per row 
    BYTE * cpStart = VarPtr;
    BYTE * cpEnd   = cpStart;
    BYTE * colEnd  = cpStart;
    BYTE * bufferOffset  = cpStart;
    int IndBuf,VarBuf;
    int i = 0,j = 0;
    for(j = 0 ; j < numEntries ; j++)//Calculate the data length of each column
    {
        if(pSrvrStmt->SqlDescInfo[j].DataType ==SQLTYPECODE_VARCHAR_WITH_LENGTH)
            varcharCount ++;
        if(j == numEntries -1 ){
            colBuferLen[j] = bufferRowLen - remainLen;
            if(varcharCount == 0)
            {
                // if there is no varchar in the data do nothing and  return
                delete [] colBuferLen;
                return pSrvrStmt->outputDescVarBufferLen*pSrvrStmt->rowsAffected;
            }
        }
        else
        {
            IndBuf = *(int*)(desc+pSrvrStmt->SqlDescInfo[j+1].IndBuf) ;
            VarBuf = *(int*)(desc+pSrvrStmt->SqlDescInfo[j+1].VarBuf);
            if(IndBuf == -1)
            {
                colBuferLen[j] = VarBuf - startOffset;
                startOffset = VarBuf;
            }
            else
            {
                colBuferLen[j] = IndBuf - startOffset;
                startOffset = IndBuf;
            }
            remainLen += colBuferLen[j];
        }
    }
    for( i = 0; i < rowsAffected ; i++){//do clip 
        for (j = 0 ; j < numEntries ; j++)
        {
            switch (pSrvrStmt->SqlDescInfo[j].DataType)
            {
                case SQLTYPECODE_VARCHAR_WITH_LENGTH:
                    /*
                                             |--------------column 1 ------|--------------column 2 ------|
                     column format nullable: |align_1|indPtr|align_2|varPtr|align_1|indPtr|align_2|varPtr|

                                             |--column 1--|--column 2--|
                     column format not null: |align|varPtr|align|varPtr|
                     indPtr is the pointer to store nullable info ,if column value is null indPtr = -1
                     if varPtr is pointer to store column value 
                     IndBuf is the offset of indPtr
                     VarBuf is the offset of varPtr

                    */
                    IndBuf = *(int*)(desc+pSrvrStmt->SqlDescInfo[j].IndBuf);
                    VarBuf = *(int*)(desc+pSrvrStmt->SqlDescInfo[j].VarBuf);
                    if(IndBuf != -1 && (*(short*)(VarPtr+IndBuf + i*bufferRowLen) == -1))
                    {
                        cpEnd = cpStart+VarBuf-IndBuf;
                        colEnd = cpStart + colBuferLen[j];
                        break;
                    }
                    if( pSrvrStmt->SqlDescInfo[j].Length > SHRT_MAX )
                    {
                        cpEnd = VarPtr+VarBuf + i*bufferRowLen + 4 + *(int*)(VarPtr+VarBuf+i*bufferRowLen);
                    }
                    else
                    {
                        cpEnd =  VarPtr+VarBuf + i*bufferRowLen + 2 + *(short*)(VarPtr+VarBuf+i*bufferRowLen);

                    }
                    colEnd = cpStart + colBuferLen[j];
                    break;
                default:
                    cpEnd = cpStart + colBuferLen[j];
                    colEnd = cpEnd;
                    break;
            }
            if(cpStart != bufferOffset)
            {
                memcpy(bufferOffset,cpStart,cpEnd-cpStart);
            }

            bufferOffset += cpEnd-cpStart;
            cpStart = colEnd;
        }
    }

    delete [] colBuferLen;
    return bufferOffset-VarPtr;
}
//for setting in the indicator and Varpointers.


//LCOV_EXCL_START
/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_FetchPerf'
 */
extern "C" void
odbc_SQLSvc_FetchPerf_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLSvc_FetchPerf_exc_ *exception_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ Int32 maxRowCnt
  , /* In    */ Int32 maxRowLen
  , /* In    */ IDL_short sqlAsyncEnable
  , /* In    */ Int32 queryTimeout
  , /* Out   */ Int32 *rowsAffected
  , /* Out   */ SQL_DataValue_def *outputDataValue
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning)
{
	SRVRTRACE_ENTER(FILE_SME+8);

	SRVR_STMT_HDL *pSrvrStmt = NULL;
	SQLRETURN rc = SQL_SUCCESS;

	if (maxRowCnt < 0)
	{
		exception_->exception_nr = odbc_SQLSvc_FetchPerf_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_ROW_COUNT;
	}
	else
	{
		if ((pSrvrStmt = getSrvrStmt(stmtLabel, FALSE)) == NULL)
		{
			exception_->exception_nr = odbc_SQLSvc_FetchPerf_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
		}
	}
	if (exception_->exception_nr == 0)
	{
		// resource statistics
		if (resStatStatement != NULL && pSrvrStmt->isClosed == FALSE && pSrvrStmt->bFetchStarted == FALSE && pSrvrStmt->stmtType == EXTERNAL_STMT)
		{
			pSrvrStmt->bFetchStarted = TRUE;
			pSrvrStmt->inState = inState = STMTSTAT_FETCH;
			inSqlStmtType = TYPE_UNKNOWN;
			inEstimatedCost = 0;
			inQueryId = NULL;
			inSqlString = NULL;
			inErrorStatement = 0;
			inWarningStatement = 0;
			inRowCount = 0;
			inErrorCode = 0;
			inSqlError = NULL;
			inSqlErrorLength = 0;
			/*resStatStatement->start(inState,
									pSrvrStmt->sqlQueryType,
									stmtLabel,
									pSrvrStmt->sqlUniqueQueryID,
						pSrvrStmt->cost_info,
						pSrvrStmt->comp_stats_info,
						inEstimatedCost,
						&pSrvrStmt->m_need_21036_end_msg);*/
			resStatStatement->start(inState,
						pSrvrStmt->sqlQueryType,
						stmtLabel,
						pSrvrStmt,
									inEstimatedCost,
									&pSrvrStmt->m_need_21036_end_msg);
		}
		// end rs
		if (pSrvrStmt->sqlStmtType != TYPE_SELECT_CATALOG)
		{
			if (pSrvrStmt->bSQLMessageSet)
				pSrvrStmt->cleanupSQLMessage();
			pSrvrStmt->outputDataValue._length = 0;
			pSrvrStmt->outputDataValue._buffer = 0;

			if (pSrvrStmt->isClosed)
			{
				exception_->exception_nr = odbc_SQLSvc_FetchPerf_SQLNoDataFound_exn_;
				goto ret;
			}


			pSrvrStmt->currentMethod = odbc_SQLSvc_FetchPerf_ldx_;
			if (pSrvrStmt->sqlBulkFetchPossible && pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE)
			{
				if (pSrvrStmt->outputDataValue._buffer != NULL)
					delete pSrvrStmt->outputDataValue._buffer;
				pSrvrStmt->outputDataValue._buffer = NULL;
				pSrvrStmt->outputDataValue._length = 0;
				rc = FETCH2bulk(pSrvrStmt);
				if (pSrvrStmt->rowsAffected > 0)
				{
					if(pSrvrStmt->outputDataValue._length == 0 && pSrvrStmt->outputDataValue._buffer == NULL)
					{
						outputDataValue->_buffer = pSrvrStmt->outputDescVarBuffer;
						outputDataValue->_length = pSrvrStmt->outputDescVarBufferLen*pSrvrStmt->rowsAffected;
					}
					else
					{
						outputDataValue->_buffer = pSrvrStmt->outputDataValue._buffer;
						outputDataValue->_length = pSrvrStmt->outputDataValue._length;
					}
				}
				else
				{
					outputDataValue->_buffer = NULL;
					outputDataValue->_length = 0;
				}
//				if (pSrvrStmt->PerfFetchRetcode == SQL_NO_DATA_FOUND)
//				{
//					char tmpString[32];
//					tmpString[0] = '\0';
//					sprintf(tmpString, "f: %Ld %d", pSrvrStmt->cliElapseTime, pSrvrStmt->rowsAffected);
//					SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
//							srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
//							srvrGlobal->srvrObjRef, 1, tmpString);
//				}
			}
			else
			{
				pSrvrStmt->maxRowCnt = maxRowCnt;
				pSrvrStmt->maxRowLen = maxRowLen;
				rc = FETCHPERF(pSrvrStmt, outputDataValue);
			}

			switch (rc)
			{
			case ODBC_SERVER_ERROR:
			case ODBC_RG_ERROR:
			default:
				break;
			}
			switch (rc)
			{
			case ODBC_RG_WARNING:
				// if there is RG_WARNING, we don't pass SQL Warning to the application
				// Hence, we need to clear any warnings
				// call SQL_EXEC_ClearDiagnostics
				// CLEARDIAGNOSTICS(pSrvrStmt);
				rc = SQL_SUCCESS_WITH_INFO;
			case SQL_SUCCESS_WITH_INFO:
				GETSQLWARNING(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlWarning);
			case SQL_SUCCESS:
				exception_->exception_nr = 0;
				*rowsAffected = pSrvrStmt->rowsAffected;
				sqlWarning->_length = pSrvrStmt->sqlWarning._length;
				sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
				break;
			case SQL_STILL_EXECUTING:
				exception_->exception_nr = odbc_SQLSvc_FetchPerf_SQLStillExecuting_exn_;
				break;
			case SQL_INVALID_HANDLE:
				exception_->exception_nr = odbc_SQLSvc_FetchPerf_SQLInvalidHandle_exn_;
				break;
			case SQL_NO_DATA_FOUND:
				exception_->exception_nr = odbc_SQLSvc_FetchPerf_SQLNoDataFound_exn_;
				break;
			case SQL_ERROR:
				GETSQLERROR(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlError);
				ERROR_DESC_def *error_desc_def;
				error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
				if (pSrvrStmt->sqlError.errorList._length != 0 && error_desc_def->sqlcode == -8007)
				{
					exception_->exception_nr = odbc_SQLSvc_FetchPerf_SQLQueryCancelled_exn_;
					exception_->u.SQLQueryCancelled.sqlcode = error_desc_def->sqlcode;
				}
				else
				{
					exception_->exception_nr = odbc_SQLSvc_FetchPerf_SQLError_exn_;
					exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
					exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
				}
				break;
			case PROGRAM_ERROR:
				exception_->exception_nr = odbc_SQLSvc_FetchPerf_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_FETCH_FAILED;
			default:
				break;
			}
		 }
		else
		{ // Catalog APIs

			pSrvrStmt->maxRowCnt = maxRowCnt;
			pSrvrStmt->maxRowLen = maxRowLen;
			rc = FETCHPERF(pSrvrStmt, outputDataValue);

			if (pSrvrStmt->sqlError.errorList._buffer != NULL)
			{
				rc = SQL_ERROR;
				ERROR_DESC_def *error_desc_def;
				error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
				if (pSrvrStmt->sqlError.errorList._length != 0 && error_desc_def->sqlcode == -8007)
				{
					exception_->exception_nr = odbc_SQLSvc_FetchPerf_SQLQueryCancelled_exn_;
					exception_->u.SQLQueryCancelled.sqlcode = error_desc_def->sqlcode;
				}
				else
				{
					exception_->exception_nr = odbc_SQLSvc_FetchPerf_SQLError_exn_;
					exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
					exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
				}
			}
			else if (pSrvrStmt->rowsAffected == 0 || pSrvrStmt->rowsAffected == -1)
			{
				if (pSrvrStmt->bSQLMessageSet)
						pSrvrStmt->cleanupSQLMessage();
				pSrvrStmt->outputDataValue._length = 0;
				pSrvrStmt->outputDataValue._buffer = 0;
				pSrvrStmt->InternalStmtClose(SQL_CLOSE);
				rc = SQL_NO_DATA_FOUND;
				exception_->exception_nr = odbc_SQLSvc_FetchPerf_SQLNoDataFound_exn_;
			}
			else
			{
				exception_->exception_nr = 0;
				*rowsAffected = pSrvrStmt->rowsAffected;
				outputDataValue->_length = pSrvrStmt->outputDataValue._length;
				outputDataValue->_buffer = pSrvrStmt->outputDataValue._buffer;
				sqlWarning->_length = pSrvrStmt->sqlWarning._length;
				sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
				if (pSrvrStmt->sqlWarning._length != 0)
					rc = SQL_SUCCESS_WITH_INFO;
				else
					rc = SQL_SUCCESS;
				pSrvrStmt->rowsAffected = 0;
			}
		}

ret:
		if (exception_->exception_nr != 0)
		{
			if (pSrvrStmt->outputDataValue._buffer != NULL)
				delete pSrvrStmt->outputDataValue._buffer;
			pSrvrStmt->outputDataValue._length = 0;
			pSrvrStmt->outputDataValue._buffer = NULL;
		}
		if (resStatStatement != NULL && pSrvrStmt->bFetchStarted == TRUE &&
						(rc == SQL_NO_DATA_FOUND || rc == SQL_ERROR ||
						((rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) && *rowsAffected < maxRowCnt)))
		{
			resStatStatement->setStatistics(pSrvrStmt);
		}
	}

	// resource statistics
	if (resStatStatement != NULL && pSrvrStmt != NULL && pSrvrStmt->isClosed == TRUE && pSrvrStmt->bFetchStarted == TRUE && pSrvrStmt->stmtType == EXTERNAL_STMT)
	{
		if (rc == SQL_ERROR  && exception_->u.SQLError.errorList._buffer != NULL)
		{
			ERROR_DESC_def *p_buffer = exception_->u.SQLError.errorList._buffer;
			inErrorCode = p_buffer->sqlcode;
			inSqlError = p_buffer->errorText;
			inSqlErrorLength = strlen(p_buffer->errorText);
		}
		pSrvrStmt->bFetchStarted = FALSE;
		Int32 inMaxRowCnt = 0;
		Int32 inMaxRowLen = 0;

		inMaxRowCnt = maxRowCnt;
		inMaxRowLen = maxRowLen;

		if (exception_->exception_nr != 0)
			inErrorStatement ++;
		else
			setStatisticsFlag  = FALSE;

		if (sqlWarning->_length != 0)
			inWarningStatement ++;
		if (exception_->exception_nr == 5)
		{
			inErrorStatement = 0;
			inWarningStatement = 0;
			setStatisticsFlag = TRUE;

		}
		inQueryId = pSrvrStmt->sqlUniqueQueryID;
		inSqlQueryType = pSrvrStmt->sqlQueryType;
		resStatStatement->setStatisticsFlag(setStatisticsFlag);
		resStatStatement->end(inState,
							  inSqlQueryType,
							  inSqlStmtType,
							  inQueryId,
							  inEstimatedCost,
							  inSqlString,
							  inErrorStatement,
							  inWarningStatement,
							  inRowCount,
							  inErrorCode,
							  resStatSession,
							  inSqlErrorLength,
							  inSqlError,
							  pSrvrStmt,
							  &pSrvrStmt->m_need_21036_end_msg,
							  pSrvrStmt->sqlNewQueryType,
							  pSrvrStmt->isClosed);
	}
	//end rs

	SRVRTRACE_EXIT(FILE_SME+8);

	return;
}

/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_ExecuteCall'
 */
extern "C" void
odbc_SQLSvc_ExecuteCall_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLSvc_ExecuteCall_exc_ *exception_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_string cursorName
  , /* In    */ IDL_short sqlStmtType
  , /* In    */ Int32 inputRowCnt
  , /* In    */ const SQLValueList_def *inputValueList
  , /* In    */ IDL_short sqlAsyncEnable
  , /* In    */ Int32 queryTimeout
  , /* Out   */ SQLValueList_def *outputValueList
  , /* Out   */ Int32 *rowsAffected
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning)
{
	SRVRTRACE_ENTER(FILE_SME+13);

	SRVR_STMT_HDL *pSrvrStmt = NULL;
	SQLRETURN rc = SQL_SUCCESS;

	/*
	// resource statistics
	inState = STMTSTAT_EXECUTE;
	inSqlStmtType = sqlStmtType;
	inEstimatedCost = 0;
	inSqlString = NULL;
	inErrorStatement = 0;
	inWarningStatement = 0;
	inRowCount = 0;
	inErrorCode = 0;
	if (resStatStatement != NULL)
		resStatStatement->start(inState, stmtLabel);
	//end rs
	*/

	if ((pSrvrStmt = getSrvrStmt(stmtLabel, FALSE)) == NULL)
	{
		exception_->exception_nr = odbc_SQLSvc_ExecuteCall_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
		return;
	}
	if (exception_->exception_nr == 0)
	{
		rc = SQL_SUCCESS;
		if (pSrvrStmt->bSQLMessageSet)
			pSrvrStmt->cleanupSQLMessage();
		if(pSrvrStmt->bSQLValueListSet)
			pSrvrStmt->cleanupSQLValueList();
		pSrvrStmt->inputValueList._buffer = inputValueList->_buffer;
		pSrvrStmt->inputValueList._length = inputValueList->_length;

		if (pSrvrStmt->outputValueList._buffer == NULL)
		{
			rc = AllocAssignValueBuffer(pSrvrStmt->bSQLValueListSet,&pSrvrStmt->outputDescList,
				&pSrvrStmt->outputValueList, pSrvrStmt->outputDescVarBufferLen,
				1, pSrvrStmt->outputValueVarBuffer);
		}
		else
			pSrvrStmt->outputValueList._length = 0;

		if (rc == SQL_SUCCESS)
		{
			pSrvrStmt->currentMethod = odbc_SQLSvc_ExecuteCall_ldx_;
			 rc = EXECUTECALL(pSrvrStmt);
			switch (rc)
			{
			case SQL_SUCCESS:
				break;
			case SQL_SUCCESS_WITH_INFO:
				GETSQLWARNING(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlWarning);
				break;
			case SQL_ERROR:
				GETSQLERROR(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlError);
				break;
			case ODBC_RG_WARNING:
				// if there is RG_WARNING, we don't pass SQL Warning to the application
				// Hence, we need to clear any warnings
				// call SQL_EXEC_ClearDiagnostics
				// CLEARDIAGNOSTICS(pSrvrStmt);
				rc = SQL_SUCCESS_WITH_INFO;
			case ODBC_SERVER_ERROR:
			case ODBC_RG_ERROR:
			default:
				break;
			}

			switch (rc)
			{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
				break;
			case ODBC_SERVER_ERROR:
				// Allocate Error Desc
				kdsCreateSQLErrorException(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlError, 1);
				// Add SQL Error
				kdsCopySQLErrorException(&pSrvrStmt->sqlError, NULL_VALUE_ERROR, NULL_VALUE_ERROR_SQLCODE,
						NULL_VALUE_ERROR_SQLSTATE);
				break;
			case -8814:
			case 8814:
				rc = SQL_RETRY_COMPILE_AGAIN;
				break;
			default:
				break;
			}
		}

		switch (rc)
		{
		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO:
			exception_->exception_nr = 0;
			// Copy the output values
			*rowsAffected = 0;
			outputValueList->_length = pSrvrStmt->outputValueList._length;
			outputValueList->_buffer = pSrvrStmt->outputValueList._buffer;
			sqlWarning->_length = pSrvrStmt->sqlWarning._length;
			sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
			break;
		case SQL_STILL_EXECUTING:
			exception_->exception_nr = odbc_SQLSvc_ExecuteCall_SQLStillExecuting_exn_;
			break;
		case SQL_INVALID_HANDLE:
			exception_->exception_nr = odbc_SQLSvc_ExecuteCall_SQLInvalidHandle_exn_;
			break;
		case SQL_NO_DATA_FOUND:
			// Added this for JDBC T4 driver since its uses this interface to get zero or one row
			// for select statement besides CALL statement.
			exception_->exception_nr = 100;
			break;
		case SQL_NEED_DATA:
			exception_->exception_nr = odbc_SQLSvc_ExecuteCall_SQLNeedData_exn_;
			break;
		case ODBC_SERVER_ERROR:
		case SQL_ERROR:
			ERROR_DESC_def *error_desc_def;
			error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
			if (pSrvrStmt->sqlError.errorList._length != 0 && error_desc_def->sqlcode == -8007)
			{
				exception_->exception_nr = odbc_SQLSvc_ExecuteCall_SQLQueryCancelled_exn_;
				exception_->u.SQLQueryCancelled.sqlcode = error_desc_def->sqlcode;
			}
			else
			{
				exception_->exception_nr = odbc_SQLSvc_ExecuteCall_SQLError_exn_;
				exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
				exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
			}
			break;
		case -8814:
		case 8814:
			exception_->exception_nr = odbc_SQLSvc_ExecuteCall_SQLRetryCompile_exn_;
			break;
		case PROGRAM_ERROR:
			exception_->exception_nr = odbc_SQLSvc_ExecuteCall_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECUTE_FAILED;
		default:
			break;
		}

		/* SQL doesn't return statistics for Call statement?
		if (resStatStatement != NULL && (srvrGlobal->resourceStatistics & STMTSTAT_EXECUTE))
			resStatStatement->setStatistics(pSrvrStmt);
		*/
	}

	/* SQL doesn't return statistics for Call statement?
	//  resource statistics
	if (exception_.exception_nr != 0 && exception_.u.SQLError.errorList._buffer != NULL)
	{
		inErrorCode = exception_.u.SQLError.errorList._buffer->sqlcode;
		inErrorStatement ++;
	}
	if (sqlWarning._length != 0)
		inWarningStatement ++;
	inRowCount = rowsAffected;
	if (resStatStatement != NULL)
		resStatStatement->end(inState,inSqlStmtType,inEstimatedCost,inSqlString,inErrorStatement,inWarningStatement,inRowCount,inErrorCode, resStatSession);
	//end rs
	*/

	SRVRTRACE_EXIT(FILE_SME+13);
	return;
}
//LCOV_EXCL_STOP


/*
 * * Synchronous method function prototype for
 * * operation 'odbc_SQLSvc_GetSQLCatalogs'
 * */
extern "C" void
odbc_SQLSvc_GetSQLCatalogs_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLSvc_GetSQLCatalogs_exc_ *exception_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_short APIType
  , /* In    */ const IDL_char *catalogNm
  , /* In    */ const IDL_char *schemaNm
  , /* In    */ const IDL_char *tableNm
  , /* In    */ const IDL_char *tableTypeList
  , /* In    */ const IDL_char *columnNm
  , /* In    */ Int32 columnType
  , /* In    */ Int32 rowIdScope
  , /* In    */ Int32 nullable
  , /* In    */ Int32 uniqueness
  , /* In    */ Int32 accuracy
  , /* In    */ IDL_short sqlType
  , /* In    */ UInt32 metadataId
  , /* In    */ const IDL_char *fkcatalogNm
  , /* In    */ const IDL_char *fkschemaNm
  , /* In    */ const IDL_char *fktableNm
  , /* Out   */ IDL_char *catStmtLabel
  , /* Out   */ SQLItemDescList_def *outputDesc
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
  )
{
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

        char                          CatalogQuery[20000];
        SRVR_STMT_HDL                 *QryCatalogSrvrStmt = NULL;
        odbc_SQLSvc_Prepare_exc_      prepareException;
        odbc_SQLSvc_ExecuteN_exc_     executeException;
        char                          *inputParam[16];
        char                          *tableParam[20];

        short                         retCode;
        char                          RequestError[200 + 1];
	char ConvertAPITypeToString[30];

	Int32 curRowNo = 0;
	Int32 numOfCols = 0;
	Int32 curColNo = 0;
	Int32 rowsAffected = 0;
	Int32 rowsFetched;
	char SQLObjType[2];
	short EnvSetting = 0;
	char MapDataType[2] = "0";
	short retcode = 0;
	char tmpBuf[20];
	char odbcAppVersion[20];
        char lc_tableTypeList[MAX_ANSI_NAME_LEN+1];
	char *token;
	char* saveptr;

	odbc_SQLSvc_FetchN_exc_ fetchException;
	odbc_SQLSvc_Close_exc_ CloseException;
	CloseException.exception_nr=0;
	fetchException.exception_nr=0;

	if (resStatSession != NULL)
		resStatSession->totalCatalogStatements++;
	char catalogNmNoEsc[MAX_ANSI_NAME_LEN+1];
	char schemaNmNoEsc[MAX_ANSI_NAME_LEN+1];
	char tableNmNoEsc[MAX_ANSI_NAME_LEN+1];
	char columnNmNoEsc[MAX_ANSI_NAME_LEN+1];

	char expCatalogNm[MAX_ANSI_NAME_LEN+1];
	char expSchemaNm[MAX_ANSI_NAME_LEN+1];
	char expTableNm[MAX_ANSI_NAME_LEN+1];
	char expColumnNm[MAX_ANSI_NAME_LEN+1];

	char tableName1[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3];
	char tableName2[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3];
	char tableName3[MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+MAX_ANSI_NAME_LEN+3];

	_itoa(srvrGlobal->appVersion.majorVersion, odbcAppVersion, 10);

        if(diagnostic_flags)
        {
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
                              case SQL_API_SQLPROCEDURES:
                                      sprintf(ConvertAPITypeToString, "SQLProcedures (%d)", SQL_API_SQLPROCEDURES);
                                      break;
                              case SQL_API_SQLPROCEDURECOLUMNS:
                                      sprintf(ConvertAPITypeToString, "SQLProcedureColumns (%d)", SQL_API_SQLPROCEDURECOLUMNS);
                                      break;
                              case SQL_API_TBLSYNONYM:
                                      sprintf(ConvertAPITypeToString, "SQLTblsynonym (%d)", SQL_API_TBLSYNONYM);
                                      break;
                              case SQL_API_TBLMVS:
                                      sprintf(ConvertAPITypeToString, "SQLTblMvs (%d)", SQL_API_TBLMVS);
                                      break;
                              default:
                                      sprintf(ConvertAPITypeToString, "Invalid Catalog API (%d)", APIType);
                                      break;
                       }
                       TraceOut(TR_SRVR_KRYPTON_API,"odbc_SQLSvc_GetSQLCatalogs_sme_(%#x, %#x, %#x, %ld, %s, %s, %s, %s, %s, %s, %s, %ld, %ld, %ld, %ld, %ld, %d, %s, %#x, %#x)",
                              objtag_,
                              call_id_,
                              exception_,
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
                              fktableNm,
                              catStmtLabel,
                              outputDesc,
                              sqlWarning);
        }

        if (stmtLabel != NULL)
               odbc_SQLSvc_Close_sme_(objtag_, call_id_, &CloseException, dialogueId, stmtLabel,
                                             SQL_DROP, &rowsAffected, sqlWarning);

        if ((tableNm[0] == '\0') && (columnNm[0] == '\0') && (metadataId == 1))
                metadataId = 0;

        if (APIType != SQL_API_SQLTABLES && APIType != SQL_API_SQLTABLES_JDBC)
        {
                if (tableNm[0] == '\0')
                        strcpy((char *)tableNm,"%");
                if (columnNm[0] == '\0')
                        strcpy((char *)columnNm,"%");
        }

        exception_->exception_nr = 0;
        CatalogQuery[0] = '\0';
        strcpy(catStmtLabel, stmtLabel);
        switch(APIType)
        {
               case SQL_API_SQLTABLES :
               case SQL_API_SQLTABLES_JDBC :
                       if ((strcmp(catalogNm,"%") == 0) && (strcmp(schemaNm,"") == 0) && (strcmp(tableNm,"") == 0))
                       {
                              strcpy(catalogNmNoEsc, SEABASE_MD_CATALOG);
                              inputParam[0] = catalogNmNoEsc;
                              inputParam[1] = inputParam[0];
                              inputParam[2] = NULL;

                              if (APIType == SQL_API_SQLTABLES)
                              {
                                 // strcpy((char *)catStmtLabel, "SQL_TABLES_ANSI_Q1");
                                 snprintf(CatalogQuery, sizeof(CatalogQuery),
"select distinct(cast('%s' as varchar(128))) TABLE_CAT, "
  "cast(NULL as varchar(128) ) TABLE_SCHEM, "
  "cast(NULL as varchar(128) ) TABLE_NAME, "
  "cast(NULL as varchar(128) ) TABLE_TYPE, "
  "cast(NULL as varchar(128)) REMARKS "
"from TRAFODION.\"_MD_\".objects "
"where CATALOG_NAME = '%s' "
"FOR READ UNCOMMITTED ACCESS ORDER BY 4,1,2,3;", 
                                inputParam[0], inputParam[1]);
                              }
                              else
                              {
                                // strcpy((char *)catStmtLabel, "SQL_JAVA_TABLES_ANSI_Q1");
                                snprintf(CatalogQuery, sizeof(CatalogQuery),
"select distinct(cast('%s' as varchar(128) )) TABLE_CAT "
"from TRAFODION.\"_MD_\".objects "
"where CATALOG_NAME = '%s' "
"FOR READ UNCOMMITTED ACCESS ORDER BY 1;", 
                                inputParam[0], inputParam[1]);
                              }
                       }
                       else
                       if ((strcmp(catalogNm,"") == 0) && (strcmp(schemaNm,"%") == 0) && (strcmp(tableNm,"") == 0))
                       {
                              convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
                              strcpy(catalogNmNoEsc, SEABASE_MD_CATALOG);
                              inputParam[0] = catalogNmNoEsc;
                              inputParam[1] = inputParam[0];
                              inputParam[2] = (char*) schemaNm;
                              inputParam[3] = expSchemaNm;
                              inputParam[4] = NULL;

                              if (APIType == SQL_API_SQLTABLES)
                              {
                                 // strcpy((char *)catStmtLabel, "SQL_TABLES_ANSI_Q2");
                                 snprintf(CatalogQuery,sizeof(CatalogQuery),
 "select distinct cast(NULL as varchar(128) ) TABLE_CAT, "
   "cast(trim(SCHEMA_NAME) as varchar(128) ) TABLE_SCHEM, "
   "cast(NULL as varchar(128) ) TABLE_NAME, "
   "cast(NULL as varchar(128) ) TABLE_TYPE, "
   "cast(NULL as varchar(128)) REMARKS "
 "from TRAFODION.\"_MD_\".objects "
 "where "
   "(CATALOG_NAME = '%s' or "
   " CATALOG_NAME LIKE '%s' ESCAPE '\\') "
   "and (SCHEMA_NAME = '%s' or "
   "SCHEMA_NAME LIKE '%s' ESCAPE '\\') "
 "FOR READ UNCOMMITTED ACCESS ORDER BY 4,1,2,3;", 
                                 inputParam[0], inputParam[1], inputParam[2],
                                 inputParam[3]);
                              }
                              else
                              {
                                      // strcpy((char *)catStmtLabel, "SQL_JAVA_TABLES_ANSI_Q2");
                                 snprintf(CatalogQuery,sizeof(CatalogQuery),
 "select distinct "
   "cast(trim(SCHEMA_NAME) as varchar(128) ) TABLE_SCHEM, "
   "cast(trim(CATALOG_NAME) as varchar(128) ) TABLE_CATALOG "
 "from TRAFODION.\"_MD_\".objects "
 "where "
   "(CATALOG_NAME = '%s' or "
   " CATALOG_NAME LIKE '%s' ESCAPE '\\') "
   "and (SCHEMA_NAME = '%s' or "
   "SCHEMA_NAME LIKE '%s' ESCAPE '\\') "
 "FOR READ UNCOMMITTED ACCESS ORDER BY 2;", 
                                 inputParam[0], inputParam[1], inputParam[2],
                                 inputParam[3]);
                              }
                       }
                        else
                        if ((strcmp(catalogNm,"") == 0) && (strcmp(schemaNm,"") 
== 0) && (strcmp(tableNm,"") == 0) && (strcmp(tableTypeList,"%") == 0))
                        {
                                strcpy(catalogNmNoEsc, "%");
                                strcpy(schemaNmNoEsc, "%");
                                strcpy(tableNmNoEsc, "%");
                                // strcpy((char *)catStmtLabel, "SQL_TABLES_ANSI_Q4");
                                tableParam[0] = NULL;
                                inputParam[0] = NULL;
                                snprintf(CatalogQuery,sizeof(CatalogQuery),
 "select cast(NULL as varchar(128) ) TABLE_CAT,"
    "cast(NULL as varchar(128) ) TABLE_SCHEM, "
    "cast(NULL as varchar(128) ) TABLE_NAME,"
    "trim(TABLE_TYPE) TABLE_TYPE,"
    "cast(NULL as varchar(128)) REMARKS " 
 " from (VALUES "
         "('TABLE'),"
         "('SYSTEM TABLE'),"
         "('VIEW'))"
     " tp (\"TABLE_TYPE\")"
 " FOR READ UNCOMMITTED ACCESS ORDER BY 4,1,2,3;");
                        }
                        else
                        {
                                if (tableNm[0] == '\0')
                                        strcpy((char *)tableNmNoEsc,"%");
                                if (! checkIfWildCard(catalogNm, expCatalogNm))
                                {
                                        exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
                                        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
                                        goto MapException;
                                }
                                if (strcmp(catalogNm,"") == 0) // If catalog empty default to system catalog
                                        strcpy(tableName1,SEABASE_MD_CATALOG);
                                else
                                {
                                        strncpy(tableName1,catalogNm, sizeof(tableName1));
                                        tableName1[sizeof(tableName1)-1] = 0;
                                }
                                tableParam[0] = tableName1;
                                tableParam[1] = NULL;
                                convertWildcardNoEsc(metadataId, TRUE, schemaNm,schemaNmNoEsc);
                                convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
                                if (tableNm[0] == '\0')
                                {
                                convertWildcardNoEsc(metadataId, TRUE, tableNmNoEsc, tableNmNoEsc);
                                convertWildcard(metadataId, TRUE, tableNmNoEsc, expTableNm);
                                }
                                else
                                {
                                convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
                                convertWildcard(metadataId, TRUE, tableNm, expTableNm);
                                }

                                inputParam[0] = schemaNmNoEsc;
                                inputParam[1] = expSchemaNm;
                                inputParam[2] = tableNmNoEsc;
                                inputParam[3] = expTableNm;
                                if (tableTypeList == NULL || strlen(tableTypeList) == 0 || strcmp(tableTypeList,"%") == 0)
                                {
                                        inputParam[4]  = "UT"; // User Table
                                        inputParam[5]  = "BT";
                                        inputParam[6]  = "VI"; 
                                        inputParam[7]  = "SM"; // System MetaData
                                        inputParam[8]  = "BT";
                                        inputParam[9] = NULL;
                                }
                                else
                                {
                                        inputParam[4]  = "";
                                        inputParam[5]  = "";
                                        inputParam[6]  = "";
                                        inputParam[7]  = "";
                                        inputParam[8]  = "";
                                        inputParam[9] = NULL;

                                        strncpy(lc_tableTypeList, tableTypeList, sizeof(lc_tableTypeList));
                                        lc_tableTypeList[sizeof(lc_tableTypeList)-1] = 0;
                                        token = strtok_r(lc_tableTypeList, " ,'", &saveptr);
                                        while (token != NULL)
                                        {
                                                if (strcmp(token, "SYSTEM") == 0
)
                                                {
                                                        token = strtok_r(NULL, ",'", &saveptr);
                                                        if (token != NULL && strcmp(token, "TABLE") == 0)
                                                        {
                                                                inputParam[7] = "SM";
                                                                inputParam[8] = "BT";
                                                        }
                                                        else
                                                                continue;
                                                }
                                                else
                                                if (strcmp(token, "TABLE") == 0)                                                {
                                                        inputParam[4]  = "UT";
                                                        inputParam[5]  = "BT";
                                                }
                                                else
                                                if (strcmp(token, "VIEW") == 0)
                                                {
                                                        inputParam[4]  = "UT";
                                                        inputParam[6]  = "VI";
                                                }

                                                token = strtok_r(NULL, " ,'", &saveptr);
                                        }
                                }

                                if (APIType == SQL_API_SQLTABLES)
                                {
                                   // strcpy((char *)catStmtLabel, "SQL_TABLES_ANSI_Q7");
                                   snprintf(CatalogQuery,sizeof(CatalogQuery),
 "select cast('%s' as varchar(128) ) TABLE_CAT,"
   "cast(trim(SCHEMA_NAME) as varchar(128) ) TABLE_SCHEM,"
   "cast(trim(OBJECT_NAME) as varchar(128) ) TABLE_NAME,"
   "trim(case OBJECT_TYPE "
       "when 'BT' then 'TABLE' "
       "when 'VI' then 'VIEW' "
      "end) TABLE_TYPE,"
   "cast(NULL as varchar(128)) REMARKS "
 " from TRAFODION.\"_MD_\".OBJECTS "
 " where (SCHEMA_NAME = '%s' or "
 " trim(SCHEMA_NAME) LIKE '%s' ESCAPE '\\')"
 " and (OBJECT_NAME = '%s' or"
 " trim(OBJECT_NAME) LIKE '%s' ESCAPE '\\')"
 "  and ((SCHEMA_NAME <> '_MD_' and '%s'='UT' and OBJECT_TYPE in ('%s', '%s'))"
 "  or   (SCHEMA_NAME = '_MD_' and '%s'='SM' and OBJECT_TYPE in ('%s')))"
 " FOR READ UNCOMMITTED ACCESS ORDER BY 4, 1, 2, 3 ;", 
                                   tableParam[0], inputParam[0], inputParam[1],
                                   inputParam[2], inputParam[3], inputParam[4],
                                   inputParam[5], inputParam[6], inputParam[7],
                                   inputParam[8]);

                                }
                                else
                                {
                                   // strcpy((char *)catStmtLabel, "SQL_JAVA_TABLES_ANSI_Q7");
                                   snprintf(CatalogQuery,sizeof(CatalogQuery),
 "select cast('%s' as varchar(128) ) TABLE_CAT,"
  "cast(trim(SCHEMA_NAME) as varchar(128) ) TABLE_SCHEM,"
  "cast(trim(OBJECT_NAME) as varchar(128) ) TABLE_NAME,"
  "trim(case OBJECT_TYPE "
       "when 'BT' then 'TABLE' "
       "when 'VI' then 'VIEW' "
       "end) TABLE_TYPE,"
  "cast(NULL as varchar(128)) REMARKS, "
  "cast(NULL as varchar(128)) TYPE_CAT,"
  "cast(NULL as varchar(128)) TYPE_SCHEM, "
  "cast(NULL as varchar(128)) TYPE_NAME,"
  "cast(NULL as varchar(128)) SELF_REFERENCING_COL_NAME, "
  "cast(NULL as varchar(128)) REF_GENERATION"
 " from TRAFODION.\"_MD_\".OBJECTS "
 " where (SCHEMA_NAME = '%s' or "
 " trim(SCHEMA_NAME) LIKE '%s' ESCAPE '\\')"
 " and (OBJECT_NAME = '%s' or"
 " trim(OBJECT_NAME) LIKE '%s' ESCAPE '\\')"
 "  and ((SCHEMA_NAME <> '_MD_' and '%s'='UT' and OBJECT_TYPE in ('%s', '%s'))"
 "  or   (SCHEMA_NAME = '_MD_' and '%s'='SM' and OBJECT_TYPE in ('%s')))"
 " FOR READ UNCOMMITTED ACCESS ORDER BY 4, 1, 2, 3 ;", 
                                   tableParam[0], inputParam[0], inputParam[1],
                                   inputParam[2], inputParam[3], inputParam[4],
                                   inputParam[5], inputParam[6], inputParam[7],
                                   inputParam[8]);

                                }

                       }
                       break;
                case SQL_API_SQLGETTYPEINFO :
                     {
                         char condExpression[20] = {0};
                        
                         switch(sqlType) {
                             case SQL_DATE:
                                 sqlType == SQL_TYPE_DATE;
                                 break;
                             case SQL_TIME:
                                 sqlType == SQL_TYPE_TIME;
                                 break;
                             case SQL_TIMESTAMP:
                                 sqlType == SQL_TYPE_TIMESTAMP;
                                 break;
                             default:
                                 break;
                         }

                         if(sqlType == SQL_ALL_TYPES)
                             sprintf(condExpression, "1=1");
                         else
                             sprintf(condExpression, "DATA_TYPE=%d", sqlType);

                         snprintf(CatalogQuery,sizeof(CatalogQuery),
                                 "select distinct TYPE_NAME TYPE_NAME,"
                                 "DATA_TYPE DATA_TYPE,PREC COLUMN_SIZE,"
                                 "LITERAL_PREFIX LITERAL_PREFIX,"
                                 "LITERAL_SUFFIX LITERAL_SUFFIX,"
                                 "CREATE_PARAMS CREATE_PARAMS,"
                                 "IS_NULLABLE NULLABLE,"
                                 "CASE_SENSITIVE CASE_SENSITIVE,"
                                 "SEARCHABLE SEARCHABLE,"
                                 "UNSIGNED_ATTRIBUTE UNSIGNED_ATTRIBUTE,"
                                 "FIXED_PREC_SCALE FIXED_PREC_SCALE,"
                                 "AUTO_UNIQUE_VALUE AUTO_UNIQUE_VALUE,"
                                 "LOCAL_TYPE_NAME LOCAL_TYPE_NAME,"
                                 "MINIMUM_SCALE MINIMUM_SCALE,"
                                 "MAXIMUM_SCALE MAXIMUM_SCALE,"
                                 "SQL_DATA_TYPE SQL_DATA_TYPE,"
                                 "SQL_DATETIME_SUB SQL_DATETIME_SUB,"
                                 "NUM_PREC_RADIX NUM_PREC_RADIX,"
                                 "INTERVAL_PRECISION INTERVAL_PRECISION "
                                 " from "
                                 " (VALUES "
                                 "(cast('BIGINT' as varchar(128)),cast(-5 as smallint), cast(19 as integer), cast (NULL as varchar(128)), cast (NULL as varchar(128)),"
                                 "cast (NULL as varchar(128)), cast(1 as smallint), cast(0 as smallint), cast(2 as smallint) , cast(0 as smallint), cast(0 as smallint),"
                                 "cast(0 as smallint), cast('LARGEINT' as varchar(128)), cast(NULL as smallint), cast(NULL as smallint), cast('LARGEINT' as varchar(128)),"
                                 "cast(10 as smallint), cast(19 as integer), cast(20 as integer), cast(-402 as smallint), cast(NULL as smallint), cast(NULL as smallint),"
                                 "cast(0 as smallint), cast(0 as smallint), cast(3 as smallint), cast(0 as smallint)),"
                                 "('BIGINT SIGNED', -5, 19, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'LARGEINT', NULL, NULL, 'SIGNED LARGEINT', 19, 19, -1, -5, NULL, NULL, 0, 0, 3, 0),"
                                 "('BIGINT UNSIGNED', -5, 20, NULL, NULL, NULL, 1, 0, 2, 1, 0, 0, 'LARGEINT', NULL, NULL, 'UNSIGNED LARGEINT', 20, 20, -1, -405, NULL, NULL, 0, 0, 3, 0),"
                                 "('CHAR', 1, 32000, '''', '''', 'max length', 1, 1, 3, NULL, 0, NULL, 'CHARACTER', NULL, NULL, 'CHARACTER', NULL, -1, -1, 1, NULL, NULL, 0, 0, 3, 0),"
                                 "('NCHAR', -8, 32000, '''', '''', 'max length', 1, 1, 3, NULL, 0, NULL, 'WCHAR', NULL, NULL, 'WCHAR', NULL, -1, -1, -8, NULL, NULL, 0, 0, 3, 0),"
                                 "('NCHAR VARYING', -9, 32000, '''', '''', 'max length', 1, 1, 3, NULL, 0, NULL, 'WCHAR VARYING', NULL, NULL, 'VARWCHAR', NULL, -1, -1, -9, NULL, NULL, 0, 0, 3, 0),"
                                 "('DATE', 91, 10, '{d ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'DATE', NULL, NULL, 'DATE', NULL, 10, 6, 9, 1, NULL, 1, 3, 3, 0),"
                                 "('DECIMAL', 3, 18, NULL, NULL, 'precision,scale', 1, 0, 2, 0, 0, 0, 'DECIMAL', 0, 18, 'DECIMAL', 10, -2, -3, 3, NULL, NULL, 0, 0, 3, 0),"
                                 "('DECIMAL SIGNED', 3, 18, NULL, NULL, 'precision,scale', 1, 0, 2, 0, 0, 0, 'DECIMAL', 0, 18, 'SIGNED DECIMAL', 10, -2, -3, 3, NULL, NULL, 0, 0, 3, 0),"
                                 "('DECIMAL UNSIGNED', 3, 18, NULL, NULL, 'precision,scale', 1, 0, 2, 1, 0, 0, 'DECIMAL', 0, 18, 'UNSIGNED DECIMAL', 10, -2, -3, -301, NULL, NULL, 0, 0, 3, 0),"
                                 "('DOUBLE PRECISION', 8, 15, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'DOUBLE', NULL, NULL, 'DOUBLE PRECISION', 2, 54, -1, 8, NULL, NULL, 0, 0, 3, 0),"
                                 "('DOUBLE PRECISION', 8, 15, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'DOUBLE', NULL, NULL, 'DOUBLE', 2, 54, -1, 8, NULL, NULL, 0, 0, 3, 0),"
                                 "('FLOAT', 6, 15, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'FLOAT', NULL, NULL, 'FLOAT', 2, -2, -1, 6, NULL, NULL, 0, 0, 3, 0),"
                                 "('INTEGER', 4, 10, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'INTEGER', NULL, NULL, 'INTEGER', 10, 10, -1, 4, NULL, NULL, 0, 0, 3, 0),"
                                 "('INTEGER SIGNED', 4, 10, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'INTEGER', NULL, NULL, 'SIGNED INTEGER', 10, 10, -1, 4, NULL, NULL, 0, 0, 3, 0),"
                                 "('INTEGER UNSIGNED', 4, 10, NULL, NULL, NULL, 1, 0, 2, 1, 0, 0, 'INTEGER', NULL, NULL, 'UNSIGNED INTEGER', 10, 10, -1, -401, NULL, NULL, 0, 0, 3, 0),"
                                 "('INTERVAL', 113, 0, '{INTERVAL ''', ''' MINUTE TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 3, 34, 100, 13, 2, 5, 6, 3, 0),"
                                 "('INTERVAL', 105, 0, '{INTERVAL ''', ''' MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 0, 34, 100, 5, 2, 5, 5, 3, 0),"
                                 "('INTERVAL', 101, 0, '{INTERVAL ''', ''' YEAR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 0, 34, 100, 1, 2, 1, 1, 3, 0),"
                                 "('INTERVAL', 106, 0, '{INTERVAL ''', ''' SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 0, 34, 100, 6, 2, 6, 6, 3, 0),"
                                 "('INTERVAL', 104, 0, '{INTERVAL ''', ''' HOUR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 0, 34, 100, 4, 2, 4, 4, 3, 0),"
                                 "('INTERVAL', 107, 0, '{INTERVAL ''', ''' YEAR TO MONTH}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 3, 34, 100, 7, 2, 1, 2, 3, 0),"
                                 "('INTERVAL', 108, 0, '{INTERVAL ''', ''' DAY TO HOUR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 3, 34, 100, 8, 2, 3, 4, 3, 0),"
                                 "('INTERVAL', 102, 0, '{INTERVAL ''', ''' MONTH}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 0, 34, 100, 2, 2, 2, 2, 3, 0),"
                                 "('INTERVAL', 111, 0, '{INTERVAL ''', ''' HOUR TO MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 3, 34, 100, 11, 2, 4, 5, 3, 0),"
                                 "('INTERVAL', 112, 0, '{INTERVAL ''', ''' HOUR TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 6, 34, 100, 12, 2, 4, 6, 3, 0),"
                                 "('INTERVAL', 110, 0, '{INTERVAL ''', ''' DAY TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 9, 34, 100, 10, 2, 3, 6, 3, 0),"
                                 "('INTERVAL', 109, 0, '{INTERVAL ''', ''' DAY TO MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 6, 34, 100, 9, 2, 3, 5, 3, 0),"
                                 "('INTERVAL', 103, 0, '{INTERVAL ''', ''' DAY}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', NULL, 0, 34, 100, 3, 2, 3, 3, 3, 0),"
                                 "('NUMERIC', 2, 128, NULL, NULL, 'precision,scale', 1, 0, 2, 0, 0, 0, 'NUMERIC', 0, 128, 'NUMERIC', 10, -2, -3, 2, NULL, NULL, 0, 0, 3, 0),"
                                 "('NUMERIC SIGNED', 2, 128, NULL, NULL, 'precision,scale', 1, 0, 2, 0, 0, 0, 'NUMERIC', 0, 128, 'SIGNED NUMERIC', 10, -2, -3, 2, NULL, NULL, 0, 0, 3, 0),"
                                 "('NUMERIC UNSIGNED', 2, 128, NULL, NULL, 'precision,scale', 1, 0, 2, 1, 0, 0, 'NUMERIC', 0, 128, 'UNSIGNED NUMERIC', 10, -2, -3, 2, NULL, NULL, 0, 0, 3, 0),"
                                 "('REAL', 7, 7, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'REAL', NULL, NULL, 'REAL', 2, 22, -1, 7, NULL, NULL, 0, 0, 3, 0),"
                                 "('SMALLINT', 5, 5, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'SMALLINT', NULL, NULL, 'SMALLINT', 5, 5, -1, 5, NULL, NULL, 0, 0, 3, 0),"
                                 "('SMALLINT SIGNED', 5, 5, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'SMALLINT', NULL, NULL, 'SIGNED SMALLINT', 5, 5, -1, 5, NULL, NULL, 0, 0, 3, 0),"
                                 "('SMALLINT UNSIGNED', 5, 5, NULL, NULL, NULL, 1, 0, 2, 1, 0, 0, 'SMALLINT', NULL, NULL, 'UNSIGNED SMALLINT', 5, 5, -1, -502, NULL, NULL, 0, 0, 3, 0),"
                                 "('TIME', 92, 8, '{t ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'TIME', NULL, NULL, 'TIME', NULL, 8, 6, 9, 2, NULL, 4, 6, 3, 0),"
                                 "('TIMESTAMP', 93, 26, '{ts ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'TIMESTAMP', 0, 6, 'TIMESTAMP', NULL, 19, 16, 9, 3, NULL, 1, 6, 3, 0),"
                                 "('VARCHAR', 12, 32000, '''', '''', 'max length', 1, 1, 3, NULL, 0, NULL, 'VARCHAR', NULL, NULL, 'VARCHAR', NULL, -1, -1, 12, NULL, NULL, 0, 0, 3, 0),"
                                 "('LONG VARCHAR', -1, 2000, '''', '''', 'max length', 1, 1, 3, NULL, 0, NULL, 'LONG VARCHAR', NULL, NULL, 'VARCHAR', NULL, -1, -1, -1, NULL, NULL, 0, 0, 3, 0),"
                                 "('TINYINT', -6, 3, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'TINYINT', NULL, NULL, 'TINYINT', 3, 3, -1, 4, NULL, NULL, 0, 0, 3, 0),"
                                 "('TINYINT SIGNED', -6, 3, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'TINYINT', NULL, NULL, 'SIGNED TINYINT', 3, 3, -1, 4, NULL, NULL, 0, 0, 3, 0),"
                                 "('TINYINT UNSIGNED', -6, 3, NULL, NULL, NULL, 1, 0, 2, 1, 0, 0, 'TINYINT', NULL, NULL, 'UNSIGNED TINYINT', 3, 3, -1, -404, NULL, NULL, 0, 0, 3, 0)"
                                 " ) "
                                 " dt(\"TYPE_NAME\", \"DATA_TYPE\", \"PREC\", \"LITERAL_PREFIX\", \"LITERAL_SUFFIX\", \"CREATE_PARAMS\", \"IS_NULLABLE\", \"CASE_SENSITIVE\", \"SEARCHABLE\","
                                 "\"UNSIGNED_ATTRIBUTE\", \"FIXED_PREC_SCALE\", \"AUTO_UNIQUE_VALUE\", \"LOCAL_TYPE_NAME\", \"MINIMUM_SCALE\", \"MAXIMUM_SCALE\", \"SQL_TYPE_NAME\","
                                 "\"NUM_PREC_RADIX\", \"USEPRECISION\", \"USELENGTH\", \"SQL_DATA_TYPE\", \"SQL_DATETIME_SUB\", \"INTERVAL_PRECISION\", \"DATETIMESTARTFIELD\","
                                 "\"DATETIMEENDFIELD\", \"APPLICATION_VERSION\", \"TRANSLATION_ID\")"
                                 " WHERE %s" 
                                 " ORDER BY 2,1 FOR READ UNCOMMITTED ACCESS ;", condExpression);
                           break;
                     }

		case SQL_API_SQLPROCEDURES :
		case SQL_API_SQLPROCEDURES_JDBC:
		//	strcpy((char *)catStmtLabel, "SQL_PROCEDURES_ANSI_Q4");

			if (!checkIfWildCard(catalogNm, expCatalogNm) && !metadataId)
			{
				exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
				goto MapException;
			}
			if (strcmp(catalogNm,"") == 0)
				strcpy(tableName1,SEABASE_MD_CATALOG);
			else
				strcpy(tableName1,catalogNm);
			tableParam[0] = tableName1;
                        tableParam[1] = NULL;
			convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
			convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
			convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
			convertWildcard(metadataId, TRUE, tableNm, expTableNm);
			inputParam[0] = schemaNmNoEsc;
			inputParam[1] = expSchemaNm;
			inputParam[2] = tableNmNoEsc;
			inputParam[3] = expTableNm;
			inputParam[4] = NULL;

			if( APIType == SQL_API_SQLPROCEDURES )
			{
                        	snprintf(CatalogQuery,sizeof(CatalogQuery),
 "select "
  "cast('%s' as varchar(128) ) PROCEDURE_CAT, "
  "cast(trim(SCHEMA_NAME) as varchar(128) ) PROCEDURE_SCHEM, "
  "cast(trim(OBJECT_NAME)   as varchar(128) ) PROCEDURE_NAME, "
  "cast (NULL as smallint) NUM_INPUT_PARAMS, "
  "cast (NULL as smallint) NUM_OUTPUT_PARAMS, "
  "cast (NULL as smallint) NUM_RESULT_SETS, "
  "cast (NULL as varchar(128)) REMARKS, "
  "(case OBJECT_TYPE "
        "when 'UR' then cast(1 as smallint) "
        "else cast(0 as smallint) end) PROCEDURE_TYPE "
 "from "
 "TRAFODION.\"_MD_\".OBJECTS "
 "where (SCHEMA_NAME = '%s' "
        "or trim(SCHEMA_NAME) LIKE '%s' ESCAPE '\\') "
  "and (OBJECT_NAME = '%s' "
        "or trim(OBJECT_NAME) LIKE '%s' ESCAPE '\\') "
  "and OBJECT_TYPE = 'UR' "
 "FOR READ UNCOMMITTED ACCESS ORDER BY 4, 1, 2, 3 ;",
                               tableParam[0], inputParam[0], inputParam[1],
                               inputParam[2], inputParam[3]);
			}
			else
			{
				snprintf(CatalogQuery,sizeof(CatalogQuery),
"select "
"obj.CATALOG_NAME PROCEDURE_CAT, obj.SCHEMA_NAME PROCEDURE_SCHEMA,"
"obj.OBJECT_NAME PROCEDURE_NAME, cast(NULL as varchar(10)) R1,cast(NULL as varchar(10)) R2,"
"cast(NULL as varchar(10)) R3, cast(NULL as varchar(10)) REMARKS,"
"cast(case when routines.UDR_TYPE = 'P' then 1"
"    when routines.UDR_TYPE = 'F' or routines.UDR_TYPE = 'T'"
"    then 2 else 0 end as smallint) PROCEDURE_TYPE,"
"obj.OBJECT_NAME SPECIFIC_NAME "
"from "
"TRAFODION.\"_MD_\".OBJECTS obj "
"left join TRAFODION.\"_MD_\".ROUTINES routines on obj.OBJECT_UID = routines.UDR_UID "
"where (obj.SCHEMA_NAME = '%s' "
"or trim(obj.SCHEMA_NAME) LIKE '%s' ESCAPE '\\') "
"and (obj.OBJECT_NAME = '%s' "
"or trim(obj.OBJECT_NAME) LIKE '%s' ESCAPE '\\') "
"and obj.OBJECT_TYPE = 'UR' "
"FOR READ UNCOMMITTED ACCESS ORDER BY 4, 1, 2, 3 ;",
                              inputParam[0], inputParam[1],
                              inputParam[2], inputParam[3]);
			}
			break;
		case SQL_API_SQLPROCEDURECOLUMNS:
		case SQL_API_SQLPROCEDURECOLUMNS_JDBC:
			if (!checkIfWildCard(catalogNm, expCatalogNm) && !metadataId)
			{
			exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
			goto MapException;
			}
			if (strcmp(catalogNm,"") == 0)
			strcpy(tableName1,SEABASE_MD_CATALOG);
			else
			strcpy(tableName1,catalogNm);
			tableParam[0] = tableName1;
				tableParam[1] = NULL;
			convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
			convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
			convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
			convertWildcard(metadataId, TRUE, tableNm, expTableNm);
			convertWildcardNoEsc(metadataId, TRUE, columnNm, columnNmNoEsc);
			convertWildcard(metadataId, TRUE, columnNm, expColumnNm);
			inputParam[0] = schemaNmNoEsc;
			inputParam[1] = expSchemaNm;
			inputParam[2] = tableNmNoEsc;
			inputParam[3] = expTableNm;
			inputParam[4] = columnNmNoEsc;
			inputParam[5] = expColumnNm;
			inputParam[6] = NULL;
			if( APIType == SQL_API_SQLPROCEDURECOLUMNS )
			{
				snprintf(CatalogQuery,sizeof(CatalogQuery),
"select obj.CATALOG_NAME PROCEDURE_CAT, obj.SCHEMA_NAME PROCEDURE_SCHEM,"
"obj.OBJECT_NAME PROCEDURE_NAME, cols.COLUMN_NAME COLUMN_NAME,"
"cast((case when cols.DIRECTION='I' then 1 when cols.DIRECTION='N' "
"then 2 when cols.DIRECTION='O' then 3 else 0 end) as smallint) COLUMN_TYPE,"
"cols.FS_DATA_TYPE DATA_TYPE, cols.SQL_DATA_TYPE TYPE_NAME,"
"cols.COLUMN_PRECISION \"PRECISION\", cols.COLUMN_SIZE LENGTH, cols.COLUMN_SCALE SCALE,"
"cast(1 as smallint) RADIX, cols.NULLABLE NULLABLE, cast(NULL as varchar(10)) REMARKS,"
"cols.DEFAULT_VALUE COLUMN_DEF, cols.FS_DATA_TYPE SQL_DATA_TYPE, cast(0 as smallint) SQL_DATETIME_SUB,"
"cols.COLUMN_SIZE CHAR_OCTET_LENGTH, cols.COLUMN_NUMBER ORDINAL_POSITION,"
"cols.NULLABLE IS_NULLABLE"
" from TRAFODION.\"_MD_\".OBJECTS obj"
" left join TRAFODION.\"_MD_\".COLUMNS cols on obj.OBJECT_UID=cols.OBJECT_UID"
" where"
" (obj.SCHEMA_NAME = '%s' or trim(obj.SCHEMA_NAME) LIKE '%s' ESCAPE '\\') "
" and (obj.OBJECT_NAME = '%s' or trim(obj.OBJECT_NAME) LIKE '%s' ESCAPE '\\')"
" and (cols.COLUMN_NAME = '%s' or trim(cols.COLUMN_NAME) LIKE '%s' ESCAPE '\\')"
" order by PROCEDURE_CAT, PROCEDURE_SCHEM, PROCEDURE_NAME, ORDINAL_POSITION"
" FOR READ UNCOMMITTED ACCESS",
			       inputParam[0], inputParam[1],
                               inputParam[2], inputParam[3],
                               inputParam[4], inputParam[5]);
			}
            else
            {
				snprintf(CatalogQuery,sizeof(CatalogQuery),
"select obj.CATALOG_NAME PROCEDURE_CAT, obj.SCHEMA_NAME PROCEDURE_SCHEM,"
"obj.OBJECT_NAME PROCEDURE_NAME, cols.COLUMN_NAME COLUMN_NAME,"
"cast((case when cols.DIRECTION='I' then 1 when cols.DIRECTION='N' then 2 when cols.DIRECTION='O' then 3 else 0 end) as smallint) COLUMN_TYPE,"
"cols.FS_DATA_TYPE DATA_TYPE, cols.SQL_DATA_TYPE TYPE_NAME,"
"cols.COLUMN_PRECISION \"PRECISION\", cols.COLUMN_SIZE LENGTH, cols.COLUMN_SCALE SCALE,"
"cast(1 as smallint) RADIX, cols.NULLABLE NULLABLE, cast(NULL as varchar(10)) REMARKS,"
"cols.DEFAULT_VALUE COLUMN_DEF, cols.FS_DATA_TYPE SQL_DATA_TYPE, cast(0 as smallint) SQL_DATETIME_SUB,"
"cols.COLUMN_SIZE CHAR_OCTET_LENGTH, cols.COLUMN_NUMBER ORDINAL_POSITION,"
"cols.NULLABLE IS_NULLABLE, cols.COLUMN_NAME SPECIFIC_NAME"
" from TRAFODION.\"_MD_\".OBJECTS obj"
" left join TRAFODION.\"_MD_\".COLUMNS cols on obj.OBJECT_UID=cols.OBJECT_UID"
" where"
" (obj.SCHEMA_NAME = '%s' or trim(obj.SCHEMA_NAME) LIKE '%s' ESCAPE '\\') "
" and (obj.OBJECT_NAME = '%s' or trim(obj.OBJECT_NAME) LIKE '%s' ESCAPE '\\')"
" and (cols.COLUMN_NAME = '%s' or trim(cols.COLUMN_NAME) LIKE '%s' ESCAPE '\\')"
" order by PROCEDURE_CAT, PROCEDURE_SCHEM, PROCEDURE_NAME, ORDINAL_POSITION"
" FOR READ UNCOMMITTED ACCESS",
			       inputParam[0], inputParam[1],
                               inputParam[2], inputParam[3],
                               inputParam[4], inputParam[5]);
			}
			break;
		case SQL_API_SQLCOLUMNS :
			if (!checkIfWildCard(catalogNm, catalogNmNoEsc) && !metadataId)
			{
				exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
				goto MapException;
			}
			if (tableNm[0] != '$' && tableNm[0] != '\\')
			{
				if (strcmp(catalogNm,"") == 0)
					strcpy(tableName1,SEABASE_MD_CATALOG);
				else
					strcpy(tableName1, catalogNm);

/*
				if (APIType == SQL_API_SQLCOLUMNS)
						strcpy((char *)catStmtLabel, "SQL_COLUMNS_UNICODE_Q4");

				else
					strcpy((char *)catStmtLabel, "SQL_JAVA_COLUMNS_ANSI_Q4");
*/
				tableParam[0] = tableName1;
				convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
				convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
				convertWildcard(metadataId, TRUE, tableNm, expTableNm);
				convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
				convertWildcard(metadataId, TRUE, columnNm, expColumnNm);
				convertWildcardNoEsc(metadataId, TRUE, columnNm, columnNmNoEsc);
				inputParam[0] = schemaNmNoEsc;
				inputParam[1] = expSchemaNm;
				inputParam[2] = tableNmNoEsc;
				inputParam[3] = expTableNm;
				inputParam[4] = columnNmNoEsc;
				inputParam[5] = expColumnNm;
				inputParam[6] = odbcAppVersion;
				inputParam[7] = NULL;

                if (strncmp(odbcAppVersion, "3", 1) == 0)
                {
                                snprintf(CatalogQuery,sizeof(CatalogQuery),
"select "
  "cast('%s' as varchar(128) ) TABLE_CAT, "
"cast(trim(ob.SCHEMA_NAME) as varchar(128) ) TABLE_SCHEM, "
  "cast(trim(ob.OBJECT_NAME) as varchar(128) ) TABLE_NAME, "
  "cast(trim(co.COLUMN_NAME) as varchar(128) ) COLUMN_NAME, "
   "cast((case when co.FS_DATA_TYPE = 0 and co.character_set = 'UCS2' then -8 "
     "when co.FS_DATA_TYPE = 64 and co.character_set = 'UCS2' then -9 else dt.DATA_TYPE end) as smallint) DATA_TYPE, "
  "trim(dt.TYPE_NAME) TYPE_NAME, "
  "cast((case when co.FS_DATA_TYPE = 0 and co.character_set = 'UCS2' then co.COLUMN_SIZE/2 "
    "when co.FS_DATA_TYPE = 64 and co.character_set = 'UCS2' then co.COLUMN_SIZE/2 "
    "when dt.USEPRECISION = -1 then co.COLUMN_SIZE when dt.USEPRECISION = -2 then co.COLUMN_PRECISION "
    "when co.FS_DATA_TYPE = 192 then dt.USEPRECISION "
    "when co.FS_DATA_TYPE >= 195 and co.FS_DATA_TYPE <= 207 then dt.USEPRECISION + 1 "
    "else dt.USEPRECISION end) as integer) COLUMN_SIZE, "
"cast((case when dt.USELENGTH = -1 then co.COLUMN_SIZE when dt.USELENGTH = -2 then co.COLUMN_PRECISION "
   "when dt.USELENGTH = -3 then co.COLUMN_PRECISION + 2  "
   "else dt.USELENGTH end) as integer) BUFFER_LENGTH, "
  "cast(co.COLUMN_SCALE as smallint) DECIMAL_DIGITS, "
"cast(dt.NUM_PREC_RADIX as smallint) NUM_PREC_RADIX, "
 "cast(co.NULLABLE as smallint) NULLABLE, "
 "cast('' as varchar(128)) REMARKS, "
"trim(co.DEFAULT_VALUE) COLUMN_DEF, "
  "cast((case when co.FS_DATA_TYPE = 0 and co.character_set = 'UCS2' then -8 "
     "when co.FS_DATA_TYPE = 64 and co.character_set = 'UCS2' then -9 else dt.SQL_DATA_TYPE end) as smallint) SQL_DATA_TYPE, "
  "cast(dt.SQL_DATETIME_SUB as smallint) SQL_DATETIME_SUB, cast((case dt.DATA_TYPE when 1 then co.COLUMN_SIZE "
    "when -1 then co.COLUMN_SIZE when 12 then co.COLUMN_SIZE else NULL end) as integer) CHAR_OCTET_LENGTH, "
  "cast((case when (trim(co1.COLUMN_CLASS) <> 'S') then co.column_number+1 else "
    "co.column_number end) as integer) ORDINAL_POSITION, "
  "cast((case when co.NULLABLE = 0 then 'NO' else 'YES' end) as varchar(3)) IS_NULLABLE  "
"from  "
"TRAFODION.\"_MD_\".objects ob, "
"TRAFODION.\"_MD_\".columns co, "
"TRAFODION.\"_MD_\".columns co1, "
  "(VALUES ("
  "cast('BIGINT' as varchar(128)),cast(-5 as smallint), cast(19 as integer), cast (NULL as varchar(128)), cast (NULL as varchar(128)), "
  "cast (NULL as varchar(128)), cast(1 as smallint), cast(0 as smallint), cast(2 as smallint) , cast(0 as smallint), cast(0 as smallint), "
  "cast(0 as smallint), cast('LARGEINT' as varchar(128)), cast(NULL as smallint), cast(NULL as smallint), cast('SIGNED LARGEINT' as varchar(128)), cast(134 as integer), "
  "cast(10 as smallint), cast(19 as integer), cast(20 as integer), cast(-402 as smallint), cast(NULL as smallint), cast(NULL as smallint), "
  "cast(0 as smallint), cast(0 as smallint), cast(3 as smallint), cast(0 as smallint)), "
  "('CHAR', 1, 32000, '''', '''', 'max length', 1, 1, 3, NULL, 0, NULL, 'CHARACTER', NULL, NULL, 'CHARACTER', 0, NULL, -1, -1, 1, NULL, NULL, 0, 0, 3, 0), "
  "('DATE', 91, 10, '{d ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'DATE', NULL, NULL, 'DATE', 192, NULL, 10, 6, 9, 1, NULL, 1, 3, 3, 0), "
  "('DECIMAL', 3, 18, NULL, NULL, 'precision,scale', 1, 0, 2, 0, 0, 0, 'DECIMAL', 0, 18, 'SIGNED DECIMAL', 152, 10, -2, -3, 3, NULL, NULL, 0, 0, 3, 0), "
  "('DECIMAL UNSIGNED', 3, 18, NULL, NULL, 'precision,scale', 1, 0, 2, 1, 0, 0, 'DECIMAL', 0, 18, 'UNSIGNED DECIMAL', 150, 10, -2, -3, -301, NULL, NULL, 0, 0, 3, 0), "
  "('DOUBLE PRECISION', 8, 15, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'DOUBLE', NULL, NULL, 'DOUBLE', 143, 2, 54, -1, 8, NULL, NULL, 0, 0, 3, 0), "
  "('INTEGER', 4, 10, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'INTEGER', NULL, NULL, 'SIGNED INTEGER', 132, 10, 10, -1, 4, NULL, NULL, 0, 0, 3, 0), "
  "('INTEGER UNSIGNED', 4, 10, NULL, NULL, NULL, 1, 0, 2, 1, 0, 0, 'INTEGER', NULL, NULL, 'UNSIGNED INTEGER', 133, 10, 10, -1, -401, NULL, NULL, 0, 0, 3, 0), "
  "('INTERVAL', 113, 0, '{INTERVAL ''', ''' MINUTE TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 205, NULL, 3, 34, 100, 13, 2, 5, 6, 3, 0), "
  "('INTERVAL', 105, 0, '{INTERVAL ''', ''' MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 201, NULL, 0, 34, 100, 5, 2, 5, 5, 3, 0), "
  "('INTERVAL', 101, 0, '{INTERVAL ''', ''' YEAR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 195, NULL, 0, 34, 100, 1, 2, 1, 1, 3, 0), "
  "('INTERVAL', 106, 0, '{INTERVAL ''', ''' SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 204, NULL, 0, 34, 100, 6, 2, 6, 6, 3, 0), "
  "('INTERVAL', 104, 0, '{INTERVAL ''', ''' HOUR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 199, NULL, 0, 34, 100, 4, 2, 4, 4, 3, 0), "
  "('INTERVAL', 107, 0, '{INTERVAL ''', ''' YEAR TO MONTH}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 197, NULL, 3, 34, 100, 7, 2, 1, 2, 3, 0), "
  "('INTERVAL', 108, 0, '{INTERVAL ''', ''' DAY TO HOUR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 200, NULL, 3, 34, 100, 8, 2, 3, 4, 3, 0), "
  "('INTERVAL', 102, 0, '{INTERVAL ''', ''' MONTH}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 196, NULL, 0, 34, 100, 2, 2, 2, 2, 3, 0), "
  "('INTERVAL', 111, 0, '{INTERVAL ''', ''' HOUR TO MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 202, NULL, 3, 34, 100, 11, 2, 4, 5, 3, 0), "
  "('INTERVAL', 112, 0, '{INTERVAL ''', ''' HOUR TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 206, NULL, 6, 34, 100, 12, 2, 4, 6, 3, 0), "
  "('INTERVAL', 110, 0, '{INTERVAL ''', ''' DAY TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 207, NULL, 9, 34, 100, 10, 2, 3, 6, 3, 0), "
  "('INTERVAL', 109, 0, '{INTERVAL ''', ''' DAY TO MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 203, NULL, 6, 34, 100, 9, 2, 3, 5, 3, 0), "
  "('INTERVAL', 103, 0, '{INTERVAL ''', ''' DAY}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 198, NULL, 0, 34, 100, 3, 2, 3, 3, 3, 0), "
  "('NUMERIC', 2, 128, NULL, NULL, 'precision,scale', 1, 0, 2, 0, 0, 0, 'NUMERIC', 0, 128, 'SIGNED NUMERIC', 156, 10, -2, -3, 2, NULL, NULL, 0, 0, 3, 0), "
  "('NUMERIC UNSIGNED', 2, 128, NULL, NULL, 'precision,scale', 1, 0, 2, 1, 0, 0, 'NUMERIC', 0, 128, 'UNSIGNED NUMERIC', 155, 10, -2, -3, 2, NULL, NULL, 0, 0, 3, 0), "
  "('REAL', 7, 7, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'REAL', NULL, NULL, 'REAL', 142, 2, 22, -1, 7, NULL, NULL, 0, 0, 3, 0), "
  "('SMALLINT', 5, 5, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'SMALLINT', NULL, NULL, 'SIGNED SMALLINT', 130, 10, 5, -1, 5, NULL, NULL, 0, 0, 3, 0), "
  "('SMALLINT UNSIGNED', 5, 5, NULL, NULL, NULL, 1, 0, 2, 1, 0, 0, 'SMALLINT', NULL, NULL, 'UNSIGNED SMALLINT', 131, 10, 5, -1, -502, NULL, NULL, 0, 0, 3, 0), "
  "('TIME', 92, 8, '{t ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'TIME', NULL, NULL, 'TIME', 192, NULL, 8, 6, 9, 2, NULL, 4, 6, 3, 0), "
  "('TIMESTAMP', 93, 26, '{ts ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'TIMESTAMP', 0, 6, 'TIMESTAMP', 192, NULL, 19, 16, 9, 3, NULL, 1, 6, 3, 0), "
  "('VARCHAR', 12, 32000, '''', '''', 'max length', 1, 1, 3, NULL, 0, NULL, 'VARCHAR', NULL, NULL, 'VARCHAR', 64, NULL, -1, -1, 12, NULL, NULL, 0, 0, 3, 0) "
  " ) "
  "dt(\"TYPE_NAME\", \"DATA_TYPE\", \"PREC\", \"LITERAL_PREFIX\", \"LITERAL_SUFFIX\", \"CREATE_PARAMS\", \"IS_NULLABLE\", \"CASE_SENSITIVE\", \"SEARCHABLE\", "
     "\"UNSIGNED_ATTRIBUTE\", \"FIXED_PREC_SCALE\", \"AUTO_UNIQUE_VALUE\", \"LOCAL_TYPE_NAME\", \"MINIMUM_SCALE\", \"MAXIMUM_SCALE\", \"SQL_TYPE_NAME\", \"FS_DATA_TYPE\", "
     "\"NUM_PREC_RADIX\", \"USEPRECISION\", \"USELENGTH\", \"SQL_DATA_TYPE\", \"SQL_DATETIME_SUB\", \"INTERVAL_PRECISION\", \"DATETIMESTARTFIELD\", "
     "\"DATETIMEENDFIELD\", \"APPLICATION_VERSION\", \"TRANSLATION_ID\") "
"where  ob.OBJECT_UID = co.OBJECT_UID "
  "and dt.FS_DATA_TYPE = co.FS_DATA_TYPE "
  "and co.OBJECT_UID = co1.OBJECT_UID and co1.COLUMN_NUMBER = 0 "
  "and (dt.DATETIMESTARTFIELD = co.DATETIME_START_FIELD) "
  "and (dt.DATETIMEENDFIELD = co.DATETIME_END_FIELD) "
  "and (ob.SCHEMA_NAME = '%s' or trim(ob.SCHEMA_NAME) LIKE '%s' ESCAPE '\\') "
  "and (ob.OBJECT_NAME = '%s' or trim(ob.OBJECT_NAME) LIKE '%s' ESCAPE '\\') "
  "and (co.COLUMN_NAME = '%s' or trim(co.COLUMN_NAME) LIKE '%s' ESCAPE '\\')  "
  "and (ob.OBJECT_TYPE in ('BT' , 'VI') ) "
  "and (trim(co.COLUMN_CLASS) not in ('S', 'M')) "
  "and dt.APPLICATION_VERSION = %s "
"FOR READ UNCOMMITTED ACCESS order by 1, 2, 3, co.COLUMN_NUMBER ; ",
                               tableParam[0], inputParam[0], inputParam[1],
                               inputParam[2], inputParam[3], inputParam[4],
                               inputParam[5], inputParam[6]);
                }
                else
                {
                                snprintf(CatalogQuery,sizeof(CatalogQuery),
"select "
  "cast('%s' as varchar(128) ) TABLE_CAT, "
"cast(trim(ob.SCHEMA_NAME) as varchar(128) ) TABLE_SCHEM, "
  "cast(trim(ob.OBJECT_NAME) as varchar(128) ) TABLE_NAME, "
  "cast(trim(co.COLUMN_NAME) as varchar(128) ) COLUMN_NAME, "
   "cast((case when co.FS_DATA_TYPE = 0 and co.character_set = 'UCS2' then -8 "
     "when co.FS_DATA_TYPE = 64 and co.character_set = 'UCS2' then -9 else dt.DATA_TYPE end) as smallint) DATA_TYPE, "
  "trim(dt.TYPE_NAME) TYPE_NAME, "
  "cast((case when co.FS_DATA_TYPE = 0 and co.character_set = 'UCS2' then co.COLUMN_SIZE/2 "
    "when co.FS_DATA_TYPE = 64 and co.character_set = 'UCS2' then co.COLUMN_SIZE/2 "
    "when dt.USEPRECISION = -1 then co.COLUMN_SIZE when dt.USEPRECISION = -2 then co.COLUMN_PRECISION "
    "when co.FS_DATA_TYPE = 192 then dt.USEPRECISION + 1 "
    "when co.FS_DATA_TYPE >= 195 and co.FS_DATA_TYPE <= 207 then dt.USEPRECISION + 1 "
    "else dt.USEPRECISION end) as integer) COLUMN_SIZE, "
"cast((case when dt.USELENGTH = -1 then co.COLUMN_SIZE when dt.USELENGTH = -2 then co.COLUMN_PRECISION "
   "when dt.USELENGTH = -3 then co.COLUMN_PRECISION + 2  "
   "else dt.USELENGTH end) as integer) BUFFER_LENGTH, "
  "cast(co.COLUMN_SCALE as smallint) DECIMAL_DIGITS, "
"cast(dt.NUM_PREC_RADIX as smallint) NUM_PREC_RADIX, "
 "cast(co.NULLABLE as smallint) NULLABLE, "
 "cast('' as varchar(128)) REMARKS "
"from  "
"TRAFODION.\"_MD_\".objects ob, "
"TRAFODION.\"_MD_\".columns co, "
"TRAFODION.\"_MD_\".columns co1, "
  "(VALUES ("
  "cast('BIGINT' as varchar(128)),cast(-5 as smallint), cast(19 as integer), cast (NULL as varchar(128)), cast (NULL as varchar(128)), "
  "cast (NULL as varchar(128)), cast(1 as smallint), cast(0 as smallint), cast(2 as smallint) , cast(0 as smallint), cast(0 as smallint), "
  "cast(0 as smallint), cast('LARGEINT' as varchar(128)), cast(NULL as smallint), cast(NULL as smallint), cast('SIGNED LARGEINT' as varchar(128)), cast(134 as integer), "
  "cast(10 as smallint), cast(19 as integer), cast(20 as integer), cast(-402 as smallint), cast(NULL as smallint), cast(NULL as smallint), "
  "cast(0 as smallint), cast(0 as smallint), cast(3 as smallint), cast(0 as smallint)), "
  "('CHAR', 1, 32000, '''', '''', 'max length', 1, 1, 3, NULL, 0, NULL, 'CHARACTER', NULL, NULL, 'CHARACTER', 0, NULL, -1, -1, 1, NULL, NULL, 0, 0, 3, 0), "
  "('DATE', 91, 10, '{d ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'DATE', NULL, NULL, 'DATE', 192, NULL, 10, 6, 9, 1, NULL, 1, 3, 3, 0), "
  "('DECIMAL', 3, 18, NULL, NULL, 'precision,scale', 1, 0, 2, 0, 0, 0, 'DECIMAL', 0, 18, 'SIGNED DECIMAL', 152, 10, -2, -3, 3, NULL, NULL, 0, 0, 3, 0), "
  "('DECIMAL UNSIGNED', 3, 18, NULL, NULL, 'precision,scale', 1, 0, 2, 1, 0, 0, 'DECIMAL', 0, 18, 'UNSIGNED DECIMAL', 150, 10, -2, -3, -301, NULL, NULL, 0, 0, 3, 0), "
  "('DOUBLE PRECISION', 8, 15, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'DOUBLE', NULL, NULL, 'DOUBLE', 143, 2, 54, -1, 8, NULL, NULL, 0, 0, 3, 0), "
  "('INTEGER', 4, 10, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'INTEGER', NULL, NULL, 'SIGNED INTEGER', 132, 10, 10, -1, 4, NULL, NULL, 0, 0, 3, 0), "
  "('INTEGER UNSIGNED', 4, 10, NULL, NULL, NULL, 1, 0, 2, 1, 0, 0, 'INTEGER', NULL, NULL, 'UNSIGNED INTEGER', 133, 10, 10, -1, -401, NULL, NULL, 0, 0, 3, 0), "
  "('INTERVAL', 113, 0, '{INTERVAL ''', ''' MINUTE TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 205, NULL, 3, 34, 100, 13, 2, 5, 6, 3, 0), "
  "('INTERVAL', 105, 0, '{INTERVAL ''', ''' MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 201, NULL, 0, 34, 100, 5, 2, 5, 5, 3, 0), "
  "('INTERVAL', 101, 0, '{INTERVAL ''', ''' YEAR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 195, NULL, 0, 34, 100, 1, 2, 1, 1, 3, 0), "
  "('INTERVAL', 106, 0, '{INTERVAL ''', ''' SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 204, NULL, 0, 34, 100, 6, 2, 6, 6, 3, 0), "
  "('INTERVAL', 104, 0, '{INTERVAL ''', ''' HOUR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 199, NULL, 0, 34, 100, 4, 2, 4, 4, 3, 0), "
  "('INTERVAL', 107, 0, '{INTERVAL ''', ''' YEAR TO MONTH}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 197, NULL, 3, 34, 100, 7, 2, 1, 2, 3, 0), "
  "('INTERVAL', 108, 0, '{INTERVAL ''', ''' DAY TO HOUR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 200, NULL, 3, 34, 100, 8, 2, 3, 4, 3, 0), "
  "('INTERVAL', 102, 0, '{INTERVAL ''', ''' MONTH}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 196, NULL, 0, 34, 100, 2, 2, 2, 2, 3, 0), "
  "('INTERVAL', 111, 0, '{INTERVAL ''', ''' HOUR TO MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 202, NULL, 3, 34, 100, 11, 2, 4, 5, 3, 0), "
  "('INTERVAL', 112, 0, '{INTERVAL ''', ''' HOUR TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 206, NULL, 6, 34, 100, 12, 2, 4, 6, 3, 0), "
  "('INTERVAL', 110, 0, '{INTERVAL ''', ''' DAY TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 207, NULL, 9, 34, 100, 10, 2, 3, 6, 3, 0), "
  "('INTERVAL', 109, 0, '{INTERVAL ''', ''' DAY TO MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 203, NULL, 6, 34, 100, 9, 2, 3, 5, 3, 0), "
  "('INTERVAL', 103, 0, '{INTERVAL ''', ''' DAY}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 198, NULL, 0, 34, 100, 3, 2, 3, 3, 3, 0), "
  "('NUMERIC', 2, 128, NULL, NULL, 'precision,scale', 1, 0, 2, 0, 0, 0, 'NUMERIC', 0, 128, 'SIGNED NUMERIC', 156, 10, -2, -3, 2, NULL, NULL, 0, 0, 3, 0), "
  "('NUMERIC UNSIGNED', 2, 128, NULL, NULL, 'precision,scale', 1, 0, 2, 1, 0, 0, 'NUMERIC', 0, 128, 'UNSIGNED NUMERIC', 155, 10, -2, -3, 2, NULL, NULL, 0, 0, 3, 0), "
  "('REAL', 7, 7, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'REAL', NULL, NULL, 'REAL', 142, 2, 22, -1, 7, NULL, NULL, 0, 0, 3, 0), "
  "('SMALLINT', 5, 5, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'SMALLINT', NULL, NULL, 'SIGNED SMALLINT', 130, 10, 5, -1, 5, NULL, NULL, 0, 0, 3, 0), "
  "('SMALLINT UNSIGNED', 5, 5, NULL, NULL, NULL, 1, 0, 2, 1, 0, 0, 'SMALLINT', NULL, NULL, 'UNSIGNED SMALLINT', 131, 10, 5, -1, -502, NULL, NULL, 0, 0, 3, 0), "
  "('TIME', 92, 8, '{t ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'TIME', NULL, NULL, 'TIME', 192, NULL, 8, 6, 9, 2, NULL, 4, 6, 3, 0), "
  "('TIMESTAMP', 93, 26, '{ts ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'TIMESTAMP', 0, 6, 'TIMESTAMP', 192, NULL, 19, 16, 9, 3, NULL, 1, 6, 3, 0), "
  "('VARCHAR', 12, 32000, '''', '''', 'max length', 1, 1, 3, NULL, 0, NULL, 'VARCHAR', NULL, NULL, 'VARCHAR', 64, NULL, -1, -1, 12, NULL, NULL, 0, 0, 3, 0) "
  " ) "
  "dt(\"TYPE_NAME\", \"DATA_TYPE\", \"PREC\", \"LITERAL_PREFIX\", \"LITERAL_SUFFIX\", \"CREATE_PARAMS\", \"IS_NULLABLE\", \"CASE_SENSITIVE\", \"SEARCHABLE\", "
     "\"UNSIGNED_ATTRIBUTE\", \"FIXED_PREC_SCALE\", \"AUTO_UNIQUE_VALUE\", \"LOCAL_TYPE_NAME\", \"MINIMUM_SCALE\", \"MAXIMUM_SCALE\", \"SQL_TYPE_NAME\", \"FS_DATA_TYPE\", "
     "\"NUM_PREC_RADIX\", \"USEPRECISION\", \"USELENGTH\", \"SQL_DATA_TYPE\", \"SQL_DATETIME_SUB\", \"INTERVAL_PRECISION\", \"DATETIMESTARTFIELD\", "
     "\"DATETIMEENDFIELD\", \"APPLICATION_VERSION\", \"TRANSLATION_ID\") "
"where  ob.OBJECT_UID = co.OBJECT_UID "
  "and dt.FS_DATA_TYPE = co.FS_DATA_TYPE "
  "and co.OBJECT_UID = co1.OBJECT_UID and co1.COLUMN_NUMBER = 0 "
  "and (dt.DATETIMESTARTFIELD = co.DATETIME_START_FIELD) "
  "and (dt.DATETIMEENDFIELD = co.DATETIME_END_FIELD) "
  "and (ob.SCHEMA_NAME = '%s' or trim(ob.SCHEMA_NAME) LIKE '%s' ESCAPE '\\') "
  "and (ob.OBJECT_NAME = '%s' or trim(ob.OBJECT_NAME) LIKE '%s' ESCAPE '\\') "
  "and (co.COLUMN_NAME = '%s' or trim(co.COLUMN_NAME) LIKE '%s' ESCAPE '\\')  "
  "and (ob.OBJECT_TYPE in ('BT' , 'VI') ) "
  "and (trim(co.COLUMN_CLASS) not in ('S', 'M')) "
"FOR READ UNCOMMITTED ACCESS order by 1, 2, 3, co.COLUMN_NUMBER ; ",
                               tableParam[0], inputParam[0], inputParam[1],
                               inputParam[2], inputParam[3], inputParam[4],
                               inputParam[5]);
                }
			}
			break;
        case SQL_API_SQLCOLUMNS_JDBC :

            if (!checkIfWildCard(catalogNm, catalogNmNoEsc) && !metadataId)
            {
                exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
                exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
                goto MapException;
            }
            if (tableNm[0] != '$' && tableNm[0] != '\\')
            {
                if (strcmp(catalogNm,"") == 0)
                    strcpy(tableName1,SEABASE_MD_CATALOG);
                else
                    strcpy(tableName1, catalogNm);

                /*
                   if (APIType == SQL_API_SQLCOLUMNS)
                   strcpy((char *)catStmtLabel, "SQL_COLUMNS_UNICODE_Q4");

                   else
                   strcpy((char *)catStmtLabel, "SQL_JAVA_COLUMNS_ANSI_Q4");
                   */
                tableParam[0] = tableName1;
                convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
                convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
                convertWildcard(metadataId, TRUE, tableNm, expTableNm);
                convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
                convertWildcard(metadataId, TRUE, columnNm, expColumnNm);
                convertWildcardNoEsc(metadataId, TRUE, columnNm, columnNmNoEsc);
                inputParam[0] = schemaNmNoEsc;
                inputParam[1] = expSchemaNm;
                inputParam[2] = tableNmNoEsc;
                inputParam[3] = expTableNm;
                inputParam[4] = columnNmNoEsc;
                inputParam[5] = expColumnNm;
                inputParam[6] = odbcAppVersion;
                inputParam[7] = NULL;

                snprintf(CatalogQuery,sizeof(CatalogQuery),
                        "select "
                        "cast('%s' as varchar(128) ) TABLE_CAT, "
                        "cast(trim(ob.SCHEMA_NAME) as varchar(128) ) TABLE_SCHEM, "
                        "cast(trim(ob.OBJECT_NAME) as varchar(128) ) TABLE_NAME, "
                        "cast(trim(co.COLUMN_NAME) as varchar(128) ) COLUMN_NAME, "
                        "cast((case when co.FS_DATA_TYPE = 0 and co.character_set = 'UCS2' then -8 "
                        "when co.FS_DATA_TYPE = 64 and co.character_set = 'UCS2' then -9 else dt.DATA_TYPE end) as smallint) DATA_TYPE, "
                        "trim(dt.TYPE_NAME) TYPE_NAME, "
                        "cast((case when co.FS_DATA_TYPE = 0 and co.character_set = 'UCS2' then co.COLUMN_SIZE/2 "
                        "when co.FS_DATA_TYPE = 64 and co.character_set = 'UCS2' then co.COLUMN_SIZE/2 "
                        "when dt.USEPRECISION = -1 then co.COLUMN_SIZE when dt.USEPRECISION = -2 then co.COLUMN_PRECISION "
                        "when co.FS_DATA_TYPE = 192 then dt.USEPRECISION "
                        "when co.FS_DATA_TYPE >= 195 and co.FS_DATA_TYPE <= 207 then dt.USEPRECISION + 1 "
                        "else dt.USEPRECISION end) as integer) COLUMN_SIZE, "
                        "cast((case when dt.USELENGTH = -1 then co.COLUMN_SIZE when dt.USELENGTH = -2 then co.COLUMN_PRECISION "
                        "when dt.USELENGTH = -3 then co.COLUMN_PRECISION + 2  "
                        "else dt.USELENGTH end) as integer) BUFFER_LENGTH, "
                        "cast(co.COLUMN_SCALE as smallint) DECIMAL_DIGITS, "
                        "cast(dt.NUM_PREC_RADIX as smallint) NUM_PREC_RADIX, "
                        "cast(co.NULLABLE as smallint) NULLABLE, "
                        "cast('' as varchar(128)) REMARKS, "
                        "trim(co.DEFAULT_VALUE) COLUMN_DEF, "
                        "cast((case when co.FS_DATA_TYPE = 0 and co.character_set = 'UCS2' then -8 "
                        "when co.FS_DATA_TYPE = 64 and co.character_set = 'UCS2' then -9 else dt.SQL_DATA_TYPE end) as smallint) SQL_DATA_TYPE, "
                        "cast(dt.SQL_DATETIME_SUB as smallint) SQL_DATETIME_SUB, cast((case dt.DATA_TYPE when 1 then co.COLUMN_SIZE "
                        "when -1 then co.COLUMN_SIZE when 12 then co.COLUMN_SIZE else NULL end) as integer) CHAR_OCTET_LENGTH, "
                        "cast((case when (trim(co1.COLUMN_CLASS) <> 'S') then co.column_number+1 else "
                        "co.column_number end) as integer) ORDINAL_POSITION, "
                        "cast((case when co.NULLABLE = 0 then 'NO' else 'YES' end) as varchar(3)) IS_NULLABLE,  "
                        "cast(NULL as varchar(128)) SCOPE_CATALOG, "
                        "cast(NULL as varchar(128)) SCOPE_SCHEMA, "
                        "cast(NULL as varchar(128)) SCOPE_TABLE, "
                        "cast(NULL as smallint) SOURCE_DATA_TYPE, "
                        "cast((case when co.DEFAULT_CLASS = 6 then 'YES' else 'NO' end) as varchar(8)) IS_AUTOINCREMENT, "
                        "cast((case when co.DEFAULT_CLASS = 5 then 'YES' else 'NO' end) as varchar(8)) IS_GENERATEDCOLUMN "
                        "from  "
                        "TRAFODION.\"_MD_\".objects ob, "
                        "TRAFODION.\"_MD_\".columns co, "
                        "TRAFODION.\"_MD_\".columns co1, "
                        "(VALUES ("
                        "cast('BIGINT' as varchar(128)),cast(-5 as smallint), cast(19 as integer), cast (NULL as varchar(128)), cast (NULL as varchar(128)), "
                        "cast (NULL as varchar(128)), cast(1 as smallint), cast(0 as smallint), cast(2 as smallint) , cast(0 as smallint), cast(0 as smallint), "
                        "cast(0 as smallint), cast('LARGEINT' as varchar(128)), cast(NULL as smallint), cast(NULL as smallint), cast('SIGNED LARGEINT' as varchar(128)), cast(134 as integer), "
                        "cast(10 as smallint), cast(19 as integer), cast(20 as integer), cast(-402 as smallint), cast(NULL as smallint), cast(NULL as smallint), "
                        "cast(0 as smallint), cast(0 as smallint), cast(3 as smallint), cast(0 as smallint)), "
                        "('CHAR', 1, 32000, '''', '''', 'max length', 1, 1, 3, NULL, 0, NULL, 'CHARACTER', NULL, NULL, 'CHARACTER', 0, NULL, -1, -1, 1, NULL, NULL, 0, 0, 3, 0), "
                        "('DATE', 91, 10, '{d ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'DATE', NULL, NULL, 'DATE', 192, NULL, 10, 6, 9, 1, NULL, 1, 3, 3, 0), "
                        "('DECIMAL', 3, 18, NULL, NULL, 'precision,scale', 1, 0, 2, 0, 0, 0, 'DECIMAL', 0, 18, 'SIGNED DECIMAL', 152, 10, -2, -3, 3, NULL, NULL, 0, 0, 3, 0), "
                        "('DECIMAL UNSIGNED', 3, 18, NULL, NULL, 'precision,scale', 1, 0, 2, 1, 0, 0, 'DECIMAL', 0, 18, 'UNSIGNED DECIMAL', 150, 10, -2, -3, -301, NULL, NULL, 0, 0, 3, 0), "
                        "('DOUBLE PRECISION', 8, 15, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'DOUBLE', NULL, NULL, 'DOUBLE', 143, 2, 54, -1, 8, NULL, NULL, 0, 0, 3, 0), "
                        "('INTEGER', 4, 10, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'INTEGER', NULL, NULL, 'SIGNED INTEGER', 132, 10, 10, -1, 4, NULL, NULL, 0, 0, 3, 0), "
                        "('INTEGER UNSIGNED', 4, 10, NULL, NULL, NULL, 1, 0, 2, 1, 0, 0, 'INTEGER', NULL, NULL, 'UNSIGNED INTEGER', 133, 10, 10, -1, -401, NULL, NULL, 0, 0, 3, 0), "
                        "('INTERVAL', 113, 0, '{INTERVAL ''', ''' MINUTE TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 205, NULL, 3, 34, 100, 13, 2, 5, 6, 3, 0), "
                        "('INTERVAL', 105, 0, '{INTERVAL ''', ''' MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 201, NULL, 0, 34, 100, 5, 2, 5, 5, 3, 0), "
                        "('INTERVAL', 101, 0, '{INTERVAL ''', ''' YEAR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 195, NULL, 0, 34, 100, 1, 2, 1, 1, 3, 0), "
                        "('INTERVAL', 106, 0, '{INTERVAL ''', ''' SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 204, NULL, 0, 34, 100, 6, 2, 6, 6, 3, 0), "
                        "('INTERVAL', 104, 0, '{INTERVAL ''', ''' HOUR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 199, NULL, 0, 34, 100, 4, 2, 4, 4, 3, 0), "
                        "('INTERVAL', 107, 0, '{INTERVAL ''', ''' YEAR TO MONTH}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 197, NULL, 3, 34, 100, 7, 2, 1, 2, 3, 0), "
                        "('INTERVAL', 108, 0, '{INTERVAL ''', ''' DAY TO HOUR}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 200, NULL, 3, 34, 100, 8, 2, 3, 4, 3, 0), "
                        "('INTERVAL', 102, 0, '{INTERVAL ''', ''' MONTH}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 196, NULL, 0, 34, 100, 2, 2, 2, 2, 3, 0), "
                        "('INTERVAL', 111, 0, '{INTERVAL ''', ''' HOUR TO MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 202, NULL, 3, 34, 100, 11, 2, 4, 5, 3, 0), "
                        "('INTERVAL', 112, 0, '{INTERVAL ''', ''' HOUR TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 206, NULL, 6, 34, 100, 12, 2, 4, 6, 3, 0), "
                        "('INTERVAL', 110, 0, '{INTERVAL ''', ''' DAY TO SECOND}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 207, NULL, 9, 34, 100, 10, 2, 3, 6, 3, 0), "
                        "('INTERVAL', 109, 0, '{INTERVAL ''', ''' DAY TO MINUTE}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 203, NULL, 6, 34, 100, 9, 2, 3, 5, 3, 0), "
                        "('INTERVAL', 103, 0, '{INTERVAL ''', ''' DAY}', NULL, 1, 0, 2, 0, 0, NULL, 'INTERVAL', 0, 0, 'INTERVAL', 198, NULL, 0, 34, 100, 3, 2, 3, 3, 3, 0), "
                        "('NUMERIC', 2, 128, NULL, NULL, 'precision,scale', 1, 0, 2, 0, 0, 0, 'NUMERIC', 0, 128, 'SIGNED NUMERIC', 156, 10, -2, -3, 2, NULL, NULL, 0, 0, 3, 0), "
                        "('NUMERIC UNSIGNED', 2, 128, NULL, NULL, 'precision,scale', 1, 0, 2, 1, 0, 0, 'NUMERIC', 0, 128, 'UNSIGNED NUMERIC', 155, 10, -2, -3, 2, NULL, NULL, 0, 0, 3, 0), "
                        "('REAL', 7, 7, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'REAL', NULL, NULL, 'REAL', 142, 2, 22, -1, 7, NULL, NULL, 0, 0, 3, 0), "
                        "('SMALLINT', 5, 5, NULL, NULL, NULL, 1, 0, 2, 0, 0, 0, 'SMALLINT', NULL, NULL, 'SIGNED SMALLINT', 130, 10, 5, -1, 5, NULL, NULL, 0, 0, 3, 0), "
                        "('SMALLINT UNSIGNED', 5, 5, NULL, NULL, NULL, 1, 0, 2, 1, 0, 0, 'SMALLINT', NULL, NULL, 'UNSIGNED SMALLINT', 131, 10, 5, -1, -502, NULL, NULL, 0, 0, 3, 0), "
                        "('TIME', 92, 8, '{t ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'TIME', NULL, NULL, 'TIME', 192, NULL, 8, 6, 9, 2, NULL, 4, 6, 3, 0), "
                        "('TIMESTAMP', 93, 26, '{ts ''', '''}', NULL, 1, 0, 2, NULL, 0, NULL, 'TIMESTAMP', 0, 6, 'TIMESTAMP', 192, NULL, 19, 16, 9, 3, NULL, 1, 6, 3, 0), "
                        "('VARCHAR', 12, 32000, '''', '''', 'max length', 1, 1, 3, NULL, 0, NULL, 'VARCHAR', NULL, NULL, 'VARCHAR', 64, NULL, -1, -1, 12, NULL, NULL, 0, 0, 3, 0) "
                        " ) "
                        "dt(\"TYPE_NAME\", \"DATA_TYPE\", \"PREC\", \"LITERAL_PREFIX\", \"LITERAL_SUFFIX\", \"CREATE_PARAMS\", \"IS_NULLABLE\", \"CASE_SENSITIVE\", \"SEARCHABLE\", "
                        "\"UNSIGNED_ATTRIBUTE\", \"FIXED_PREC_SCALE\", \"AUTO_UNIQUE_VALUE\", \"LOCAL_TYPE_NAME\", \"MINIMUM_SCALE\", \"MAXIMUM_SCALE\", \"SQL_TYPE_NAME\", \"FS_DATA_TYPE\", "
                        "\"NUM_PREC_RADIX\", \"USEPRECISION\", \"USELENGTH\", \"SQL_DATA_TYPE\", \"SQL_DATETIME_SUB\", \"INTERVAL_PRECISION\", \"DATETIMESTARTFIELD\", "
                        "\"DATETIMEENDFIELD\", \"APPLICATION_VERSION\", \"TRANSLATION_ID\") "
                        "where  ob.OBJECT_UID = co.OBJECT_UID "
                        "and dt.FS_DATA_TYPE = co.FS_DATA_TYPE "
                        "and co.OBJECT_UID = co1.OBJECT_UID and co1.COLUMN_NUMBER = 0 "
                        "and (dt.DATETIMESTARTFIELD = co.DATETIME_START_FIELD) "
                        "and (dt.DATETIMEENDFIELD = co.DATETIME_END_FIELD) "
                        "and (ob.SCHEMA_NAME = '%s' or trim(ob.SCHEMA_NAME) LIKE '%s' ESCAPE '\\') "
                        "and (ob.OBJECT_NAME = '%s' or trim(ob.OBJECT_NAME) LIKE '%s' ESCAPE '\\') "
                        "and (co.COLUMN_NAME = '%s' or trim(co.COLUMN_NAME) LIKE '%s' ESCAPE '\\')  "
                        "and (ob.OBJECT_TYPE in ('BT' , 'VI') ) "
                        "and (trim(co.COLUMN_CLASS) not in ('S', 'M')) "
                        "and dt.APPLICATION_VERSION = 3 "
                        "FOR READ UNCOMMITTED ACCESS order by 1, 2, 3, co.COLUMN_NUMBER ; ",
                    tableParam[0], inputParam[0], inputParam[1],
                    inputParam[2], inputParam[3], inputParam[4],
                    inputParam[5], inputParam[6]);
            }
            break;
		case SQL_API_SQLPRIMARYKEYS :
			if ((!checkIfWildCard(catalogNm, catalogNmNoEsc) || !checkIfWildCard(schemaNm, schemaNmNoEsc) || !checkIfWildCard(tableNm, tableNmNoEsc)) && !metadataId)
			{
				exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
				goto MapException;
			}
			// strcpy((char *)catStmtLabel, "SQL_PRIMARYKEYS_ANSI_Q4");

			if (strcmp(catalogNm,"") == 0)
                                strcpy(tableName1,SEABASE_MD_CATALOG);
			else
				strcpy(tableName1, catalogNm);
			tableParam[0] = tableName1;
/*
			strcpy(tableName2, SEABASE_MD_CATALOG);
			strcat(tableName2, ".");
			strcat(tableName2, SEABASE_MD_SCHEMA);
			strcat(tableName2, ".");
			strcat(tableName2, smdCatAPITablesList[OBJECTS]);
			tableParam[1] = tableName2;
			strcpy(tableName3, SEABASE_MD_CATALOG);
			strcat(tableName3, ".");
			strcat(tableName3, SEABASE_MD_SCHEMA);
			strcat(tableName3, ".");
			strcat(tableName3, smdCatAPITablesList[KEYS]);
			tableParam[2] = tableName3;
			tableParam[3] = NULL;
*/
			convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
			convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
			convertWildcard(metadataId, TRUE, tableNm, expTableNm);
			convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
			inputParam[0] = schemaNmNoEsc;
			inputParam[1] = expSchemaNm;
			inputParam[2] = tableNmNoEsc;
			inputParam[3] = expTableNm;
			inputParam[4] = NULL;

                        snprintf(CatalogQuery,sizeof(CatalogQuery),
 "select "
 "cast('%s' as varchar(128) ) TABLE_CAT,"
 "cast(trim(ob.SCHEMA_NAME) as varchar(128) ) TABLE_SCHEM,"
 "cast(trim(ob.OBJECT_NAME) as varchar(128) ) TABLE_NAME,"
 "trim(ky.COLUMN_NAME) COLUMN_NAME,"
 "cast((ky.keyseq_number) as smallint) KEY_SEQ,"
 "trim(ob.OBJECT_NAME) PK_NAME "
 " from TRAFODION.\"_MD_\".OBJECTS ob, "
       "TRAFODION.\"_MD_\".KEYS ky "
 " where (ob.SCHEMA_NAME = '%s' or "
 " trim(ob.SCHEMA_NAME) LIKE '%s' ESCAPE '\\') "
 " and (ob.OBJECT_NAME = '%s' or "
 " trim(ob.OBJECT_NAME) LIKE '%s' ESCAPE '\\') "
  " and ob.OBJECT_UID = ky.OBJECT_UID and ky.COLUMN_NAME <> '_SALT_' "
  " FOR READ UNCOMMITTED ACCESS order by 1, 2, 3, 5 ;",
                        tableParam[0], inputParam[0], inputParam[1],
                        inputParam[2], inputParam[3]);


			break;

        case SQL_API_SQLFOREIGNKEYS:
            if ((!checkIfWildCard(catalogNm, catalogNmNoEsc) ||
                 !checkIfWildCard(schemaNm, schemaNmNoEsc)  ||
                 !checkIfWildCard(tableNm, tableNmNoEsc))    &&
                 !metadataId)
            {
                exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
                exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
                goto MapException;
            }

            convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
            convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
            convertWildcard(metadataId, TRUE, tableNm, expTableNm);
            convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);

            char fkcatalogNmNoEsc[MAX_ANSI_NAME_LEN + 1];
            char fkschemaNmNoEsc[MAX_ANSI_NAME_LEN + 1];
            char fktableNmNoEsc[MAX_ANSI_NAME_LEN + 1];
            char fkexpCatalogNm[MAX_ANSI_NAME_LEN + 1];
            char fkexpSchemaNm[MAX_ANSI_NAME_LEN + 1];
            char fkexpTableNm[MAX_ANSI_NAME_LEN + 1];

            if (strcmp(fktableNm, "") == 0)
                strcpy((char *)fktableNm, "%");
            if (strcmp(fkschemaNm, "") == 0)
                strcpy((char *)fkschemaNm, "%");

            convertWildcard(metadataId, TRUE, fkcatalogNm, fkexpCatalogNm);
            convertWildcardNoEsc(metadataId, TRUE, fkcatalogNm, fkcatalogNmNoEsc);

            convertWildcard(metadataId, TRUE, fkschemaNm, fkexpSchemaNm);
            convertWildcardNoEsc(metadataId, TRUE, fkschemaNm, fkschemaNmNoEsc);

            convertWildcard(metadataId, TRUE, fktableNm, fkexpTableNm);
            convertWildcardNoEsc(metadataId, TRUE, fktableNm, fktableNmNoEsc);

            snprintf(CatalogQuery, sizeof(CatalogQuery),
                    "select "
                    "cast(PKCO.CATALOG_NAME as varchar(128)) PKTABLE_CAT, "
                    "cast(PKCO.SCHEMA_NAME as varchar(128)) PKTABLE_SCHEM, "
                    "cast(PKCO.TABLE_NAME as varchar(128)) PKTABLE_NAME, "
                    "cast(PKCO.COLUMN_NAME as varchar(128)) PKCOLUMN_NAME, "
                    "cast(FKCO.CATALOG_NAME as varchar(128)) FKTABLE_CAT, "
                    "cast(PKCO.SCHEMA_NAME as varchar(128)) FKTABLE_SCHEM, "
                    "cast(FKCO.TABLE_NAME as varchar(128)) FKTABLE_NAME, "
                    "cast(FKCO.COLUMN_NAME as varchar(128)) FKCOLUMN_NAME, "
                    "cast(FKKV.ORDINAL_POSITION as smallint) KEY_SEQ, "
                    "cast(0 as smallint) update_rule, " // not support
                    "cast(0 as smallint) delete_rule, " // not support
                    "cast(FKKV.CONSTRAINT_NAME as varchar(128)) FK_NAME, "
                    "cast(PKKV.CONSTRAINT_NAME as varchar(128)) PK_NAME, "
                    "cast(0 as smallint) DEFERRABILITY " // not support
                    "from "
                    "TRAFODION.\"_MD_\".REF_CONSTRAINTS_VIEW rcv, "
                    "TRAFODION.\"_MD_\".KEYS_VIEW PKKV, "
                    "TRAFODION.\"_MD_\".KEYS_VIEW FKKV, "
                    "TRAFODION.\"_MD_\".COLUMNS_VIEW PKCO, "
                    "TRAFODION.\"_MD_\".COLUMNS_VIEW FKCO "
                    "where "
                    "FKKV.CONSTRAINT_NAME = rcv.CONSTRAINT_NAME "
                    "and PKKV.CONSTRAINT_NAME = rcv.UNIQUE_CONSTRAINT_NAME "
                    "and PKCO.TABLE_NAME = PKKV.TABLE_NAME "
                    "and FKCO.TABLE_NAME = FKKV.TABLE_NAME "
                    "and PKCO.COLUMN_NAME = PKKV.COLUMN_NAME "
                    "and FKCO.COLUMN_NAME = FKKV.COLUMN_NAME "
                    "and (PKCO.SCHEMA_NAME = '%s' or trim(PKCO.SCHEMA_NAME) LIKE '%s' ESCAPE '\\') "
                    "and (PKCO.TABLE_NAME = '%s' or trim(PKCO.TABLE_NAME) LIKE '%s' ESCAPE '\\') "
                    "and (FKCO.SCHEMA_NAME = '%s' or trim(FKCO.SCHEMA_NAME) LIKE '%s' ESCAPE '\\') "
                    "and (FKCO.TABLE_NAME = '%s' or trim(FKCO.TABLE_NAME) LIKE '%s' ESCAPE '\\') "
                    "FOR READ UNCOMMITTED ACCESS ORDER BY 1, 2, 3, 5, 6, 7, 9;",
                schemaNmNoEsc, expSchemaNm,
                tableNmNoEsc, expTableNm,
                fkschemaNmNoEsc, fkexpSchemaNm,
                fktableNmNoEsc, fkexpTableNm
                    );
            break;
        case SQL_API_SQLSTATISTICS:
            if (!checkIfWildCard(catalogNm, catalogNmNoEsc) && !metadataId)
            {
                exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
                exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED;
            }
            if (tableNm[0] != '$' && tableNm[0] != '\\')
            {
                if (strcmp(catalogNm, "") == 0)
                    strcpy(tableName1, SEABASE_MD_CATALOG);
                else
                    strcpy(tableName1, catalogNm);
            }

            tableParam[0] = tableName1;
            convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
            convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
            convertWildcard(metadataId, TRUE, tableNm, expTableNm);
            convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
            inputParam[0] = schemaNmNoEsc;
            inputParam[1] = expSchemaNm;
            inputParam[2] = tableNmNoEsc;
            inputParam[3] = expTableNm;

            snprintf(CatalogQuery, sizeof(CatalogQuery),
                    "select "
                    "cast('%s' as varchar(128)) TABLE_CAT, "
                    "cast(trim(ob.SCHEMA_NAME) as varchar(128)) TABLE_SCHEM, "
                    "cast(trim(ob.OBJECT_NAME) as varchar(128)) TABLE_NAME, "
                    "cast(NULL as smallint) NON_UNIQUE, " // return NULL if TYPE is SQL_TABLE_STAT
                    "cast(NULL as varchar(128)) INDEX_QUALIFIER, " // return NULL if TYPE is SQL_TABLE_STAT
                    "cast(NULL as varchar(128)) INDEX_NAME, " // return NULL if TYPE is SQL_TABLE_STAT
                    "cast(0 as smallint) TYPE, " // TYPE is SQL_TABLE_STAT
                    "cast(NULL as smallint) ORDINAL_POSITION, " // return NULL if TYPE is SQL_TABLE_STAT
                    "cast(trim(co.COLUMN_NAME) as varchar(128)) COLUMN_NAME, "
                    "cast(NULL as char(1)) ASC_OR_DESC, " // return NULL if TYPE is SQL_TABLE_STAT
                    "cast(sb.rowcount as integer) CARDINALITY, " // number of rows
                    "cast(NULL as integer) PAGES, " // not support
                    "cast(NULL as varchar(128)) FILTER_CONDITION " // not support
                    "from "
                    "TRAFODION.\"_MD_\".OBJECTS ob, "
                    "TRAFODION.\"_MD_\".COLUMNS co, "
                    "TRAFODION.%s.sb_histograms sb "
                    "where "
                    "ob.OBJECT_UID = co.OBJECT_UID "
                    "and co.OBJECT_UID = sb.TABLE_UID "
                    "and co.COLUMN_NUMBER = sb.COLUMN_NUMBER "
                    "and sb.colcount = 1 "
                    "and (ob.SCHEMA_NAME = '%s' or trim(ob.SCHEMA_NAME) LIKE '%s' ESCAPE '\\')  "
                    "and (ob.OBJECT_NAME = '%s' or trim(ob.OBJECT_NAME) LIKE '%s' ESCAPE '\\') "
                    "and (ob.OBJECT_TYPE in ('BT', 'VI')) "
                    "and (trim(co.COLUMN_CLASS) not in('S', 'M')) "
                    "union "
                    "select "
                    "cast('%s' as varchar(128)) TABLE_CAT, "
                    "cast(trim(ob_table.SCHEMA_NAME) as varchar(128)) TABLE_SCHEM, "
                    "cast(trim(ob_table.OBJECT_NAME) as varchar(128)) TABLE_NAME, "
                    "cast(case when idx.is_unique = 1 then 0 else 1 end as smallint) NON_UNIQUE, "
                    "cast(NULL as varchar(128)) INDEX_QUALIFIER, " // not support
                    "cast(trim(ob.OBJECT_NAME) as varchar(128)) INDEX_NAME, "
                    "cast(3 as smallint) TYPE, " // SQL_INDEX_OTHER
                    "cast(0 as smallint) ORDINAL_POSITION, "
                    "cast('' as varchar(128)) COLUMN_NAME, " // return an empty string if the expression cannot be determined.
                    "cast(NULL as char(1)) ASC_OR_DESC, " // not subsequent
                    "cast(NULL as integer) CARDINALITY, "
                    "cast(NULL as integer) PAGES, "
                    "cast(NULL as varchar(128)) FILTER_CONDITION "
                    "from "
                    "TRAFODION.\"_MD_\".OBJECTS ob, "
                    "TRAFODION.\"_MD_\".INDEXES idx, "
                    "TRAFODION.\"_MD_\".OBJECTS ob_table, "
                    "TRAFODION.\"_MD_\".TABLES tb "
                    "where "
                    "idx.BASE_TABLE_UID=tb.TABLE_UID "
                    "and idx.INDEX_UID=ob.OBJECT_UID "
                    "and idx.BASE_TABLE_UID=ob_table.OBJECT_UID "
                    "and (ob_table.SCHEMA_NAME = '%s' or trim(ob_table.SCHEMA_NAME) LIKE '%s' ESCAPE '\\') "
                    "and (ob_table.OBJECT_NAME = '%s' or trim(ob_table.OBJECT_NAME) LIKE '%s' ESCAPE '\\') "
                    "and (ob_table.OBJECT_TYPE in ('BT', 'VI')) "
                    "%s "
                    "ORDER BY 1, 2, 3, 7, 9, 6 ;",
                    tableParam[0],
                    inputParam[0],
                    inputParam[0], inputParam[1],
                    inputParam[2], inputParam[3],
                    tableParam[0],
                    inputParam[0], inputParam[1],
                    inputParam[2], inputParam[3],
                    uniqueness == 1 ? "" : "and idx.is_unique=1"
                    );
            break;
        case SQL_API_SQLTABLEPRIVILEGES:
            convertWildcard(metadataId, TRUE, catalogNm, expCatalogNm);
            convertWildcardNoEsc(metadataId, TRUE, catalogNm, catalogNmNoEsc);
            convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
            convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
            convertWildcard(metadataId, TRUE, tableNm, expTableNm);
            convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
            inputParam[0] = catalogNmNoEsc;
            inputParam[1] = expCatalogNm;
            inputParam[2] = schemaNmNoEsc;
            inputParam[3] = expSchemaNm;
            inputParam[4] = tableNmNoEsc;
            inputParam[5] = expTableNm;
            snprintf(CatalogQuery, sizeof(CatalogQuery), "select cast(ob.CATALOG_NAME as varchar(128) ) TABLE_CAT,"
                    "cast(trim(ob.SCHEMA_NAME) as varchar(128) ) TABLE_SCHEM,"
                    "cast(trim(ob.OBJECT_NAME) as varchar(128) ) TABLE_NAME,"
                    "cast(trim(op.GRANTOR_NAME) as varchar(128) ) GRANTOR,"
                    "cast(trim(op.GRANTEE_NAME) as varchar(128) ) GRANTEE,"
                    "cast(trim(case op.PRIVILEGES_BITMAP "
                    "when 1 then 'SELECT' "
                    "when 2 then 'INSERT' "
                    "when 3 then 'SELECT,INSERT' "
                    "when 4 then 'DELETE' "
                    "when 5 then 'SELECT,DELETE' "
                    "when 6 then 'INSERT,DELETE' "
                    "when 7 then 'SELECT,INSERT,DELETE' "
                    "when 8 then 'UPDATE' "
                    "when 9 then 'SELECT,UPDATE' "
                    "when 10 then 'INSERT,UPDATE' "
                    "when 11 then 'SELECT,INSERT,UPDATE' "
                    "when 12 then 'DELETE,UPDATE' "
                    "when 13 then 'SELECT,DELETE,UPDATE' "
                    "when 14 then 'INSERT,DELETE,UPDATE' "
                    "when 15 then 'SELECT,INSERT,DELETE,UPDATE' "
                    "when 32 then 'REFERENCES' "
                    "when 33 then 'SELECT,REFERENCES' "
                    "when 34 then 'INSERT,REFERENCES' "
                    "when 35 then 'SELECT,INSERT,REFERENCES' "
                    "when 36 then 'DELETE,REFERENCES' "
                    "when 37 then 'SELECT,DELETE,REFERENCES' "
                    "when 38 then 'INSERT,DELETE,REFERENCES' "
                    "when 39 then 'SELECT,INSERT,DELETE,REFERENCES' "
                    "when 40 then 'UPDATE,REFERENCES' "
                    "when 41 then 'SELECT,UPDATE,REFERENCES' "
                    "when 42 then 'INSERT,UPDATE,REFERENCES' "
                    "when 43 then 'SELECT,INSERT,UPDATE,REFERENCES' "
                    "when 44 then 'DELETE,UPDATE,REFERENCES' "
                    "when 45 then 'SELECT,DELETE,UPDATE,REFERENCES' "
                    "when 46 then 'INSERT,DELETE,UPDATE,REFERENCES' "
                    "when 47 then 'SELECT,INSERT,DELETE,UPDATE,REFERENCES' end) as varchar(128)) PRIVILEGE,"
                    "cast (trim(case op.GRANTABLE_BITMAP "
                    "when 0 then 'NO' else 'YES' "
                    "end) as varchar(128)) IS_GRANTABLE "
                    "from TRAFODION.\"_PRIVMGR_MD_\".OBJECT_PRIVILEGES as op, TRAFODION.\"_MD_\".OBJECTS as ob "
                    "where ob.OBJECT_UID = op.OBJECT_UID "
                    "%s"
                    "and (ob.CATALOG_NAME = '%s' or trim(ob.CATALOG_NAME) LIKE '%s' ESCAPE '\\') "
                    "and (ob.SCHEMA_NAME = '%s' or trim(ob.SCHEMA_NAME) LIKE '%s' ESCAPE '\\') "
                    "and (ob.OBJECT_NAME = '%s' or trim(ob.OBJECT_NAME) LIKE '%s' ESCAPE '\\') "
                    , (strlen(inputParam[4]) == 0 || inputParam[4][0] == '%') ? " " : "and op.GRANTEE_NAME <> 'DB__ROOT' "
                    , inputParam[0], inputParam[1]
                    , inputParam[2], inputParam[3]
                    , inputParam[4], inputParam[5]
                    );
            break;
        case SQL_API_SQLCOLUMNPRIVILEGES:
            convertWildcard(metadataId, TRUE, catalogNm, expCatalogNm);
            convertWildcardNoEsc(metadataId, TRUE, catalogNm, catalogNmNoEsc);
            convertWildcard(metadataId, TRUE, schemaNm, expSchemaNm);
            convertWildcardNoEsc(metadataId, TRUE, schemaNm, schemaNmNoEsc);
            convertWildcard(metadataId, TRUE, tableNm, expTableNm);
            convertWildcardNoEsc(metadataId, TRUE, tableNm, tableNmNoEsc);
            convertWildcard(metadataId, TRUE, columnNm, expColumnNm);
            convertWildcardNoEsc(metadataId, TRUE, columnNm, columnNmNoEsc);
            inputParam[0] = catalogNmNoEsc;
            inputParam[1] = expCatalogNm;
            inputParam[2] = schemaNmNoEsc;
            inputParam[3] = expSchemaNm;
            inputParam[4] = tableNmNoEsc;
            inputParam[5] = expTableNm;
            inputParam[6] = columnNmNoEsc;
            inputParam[7] = expColumnNm;
            snprintf(CatalogQuery, sizeof(CatalogQuery), "select cast(mob.CATALOG_NAME as varchar(128)) TABLE_CAT,"
                    "cast(mob.SCHEMA_NAME as varchar(128)) TABLE_SCHEM,"
                    "cast(mob.OBJECT_NAME as varchar(128)) TABLE_NAME,"
                    "cast(mco.COLUMN_NAME as varchar(128)) COLUMN_NAME,"
                    "cast(pmcp.GRANTOR_NAME as varchar(128)) GRANTOR,"
                    "cast(pmcp.GRANTEE_NAME as varchar(128)) GRANTEE,"
                    "cast(trim(case pmcp.PRIVILEGES_BITMAP "
                    "when 1 then 'SELECT' "
                    "when 2 then 'INSERT' "
                    "when 3 then 'SELECT,INSERT' "
                    "when 4 then 'DELETE' "
                    "when 5 then 'SELECT,DELETE' "
                    "when 6 then 'INSERT,DELETE' "
                    "when 7 then 'SELECT,INSERT,DELETE' "
                    "when 8 then 'UPDATE' "
                    "when 9 then 'SELECT,UPDATE' "
                    "when 10 then 'INSERT,UPDATE' "
                    "when 11 then 'SELECT,INSERT,UPDATE' "
                    "when 12 then 'DELETE,UPDATE' "
                    "when 13 then 'SELECT,DELETE,UPDATE' "
                    "when 14 then 'INSERT,DELETE,UPDATE' "
                    "when 15 then 'SELECT,INSERT,DELETE,UPDATE' "
                    "when 32 then 'REFERENCES' "
                    "when 33 then 'SELECT,REFERENCES' "
                    "when 34 then 'INSERT,REFERENCES' "
                    "when 35 then 'SELECT,INSERT,REFERENCES' "
                    "when 36 then 'DELETE,REFERENCES' "
                    "when 37 then 'SELECT,DELETE,REFERENCES' "
                    "when 38 then 'INSERT,DELETE,REFERENCES' "
                    "when 39 then 'SELECT,INSERT,DELETE,REFERENCES' "
                    "when 40 then 'UPDATE,REFERENCES' "
                    "when 41 then 'SELECT,UPDATE,REFERENCES' "
                    "when 42 then 'INSERT,UPDATE,REFERENCES' "
                    "when 43 then 'SELECT,INSERT,UPDATE,REFERENCES' "
                    "when 44 then 'DELETE,UPDATE,REFERENCES' "
                    "when 45 then 'SELECT,DELETE,UPDATE,REFERENCES' "
                    "when 46 then 'INSERT,DELETE,UPDATE,REFERENCES' "
                    "when 47 then 'SELECT,INSERT,DELETE,UPDATE,REFERENCES' end) as varchar(128)) PRIVILEGE, "
                    "cast (trim(case pmcp.GRANTABLE_BITMAP "
                    "when 0 then 'NO' else 'YES' "
                    "end) as varchar(128)) IS_GRANTABLE "
                    "from TRAFODION.\"_MD_\".OBJECTS as mob, TRAFODION.\"_MD_\".COLUMNS as mco, TRAFODION.\"_PRIVMGR_MD_\".COLUMN_PRIVILEGES as pmcp "
                    "where pmcp.OBJECT_UID = mob.OBJECT_UID "
                    "and pmcp.OBJECT_UID = mco.OBJECT_UID "
                    "and pmcp.COLUMN_NUMBER = mco.COLUMN_NUMBER "
                    "and mco.COLUMN_NAME <> 'SYSKEY' "
                    "and (mob.CATALOG_NAME = '%s' or trim(mob.CATALOG_NAME) LIKE '%s' ESCAPE '\\') "
                    "and (mob.SCHEMA_NAME = '%s' or trim(mob.SCHEMA_NAME) LIKE '%s' ESCAPE '\\') "
                    "and (mob.OBJECT_NAME = '%s' or trim(mob.OBJECT_NAME) LIKE '%s' ESCAPE '\\') "
                    "and (mco.COLUMN_NAME = '%s' or trim(mco.COLUMN_NAME) LIKE '%s' ESCAPE '\\') "
                    "order by pmcp.COLUMN_NUMBER;",
                inputParam[0], inputParam[1],
                inputParam[2], inputParam[3],
                inputParam[4], inputParam[5],
                inputParam[6], inputParam[7]);

            break;
               default :
                       exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
                       exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNSUPPORTED_SMD_API_TYPE;
                       break;
        }

        if (exception_->exception_nr == 0)
        {
           if ((QryCatalogSrvrStmt = getSrvrStmt(catStmtLabel, TRUE)) == NULL)
           {
              SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR,
                           EVENTLOG_ERROR_TYPE,
                           srvrGlobal->nskProcessInfo.processId,
                           ODBCMX_SERVER,
                           srvrGlobal->srvrObjRef,
                           2,
                           "CATALOG APIs",
                           "Allocate Statement");
              exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
              exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
           }
           else
           {
              // Temporary solution - bypass checks on metadata tables
              unsigned int savedParserFlags = 0;


              SQL_EXEC_GetParserFlagsForExSqlComp_Internal(savedParserFlags);

              try
              {
                 SQL_EXEC_SetParserFlagsForExSqlComp_Internal(INTERNAL_QUERY_FROM_EXEUTIL);

                 retcode = QryCatalogSrvrStmt->ExecDirect(NULL, CatalogQuery, EXTERNAL_STMT, TYPE_SELECT, SQL_ASYNC_ENABLE_OFF, 0);

                 SQL_EXEC_AssignParserFlagsForExSqlComp_Internal(savedParserFlags);
                 if (retcode == SQL_ERROR)
                 {
                    ERROR_DESC_def *p_buffer = QryCatalogSrvrStmt->sqlError.errorList._buffer;
                    char             errNumStr[128] = {0};
                    sprintf(errNumStr, "%d", (int)p_buffer->sqlcode);
                    strncpy(RequestError, p_buffer->errorText,sizeof(RequestError) -1);
                    RequestError[sizeof(RequestError) - 1] = '\0';

                    SendEventMsg(MSG_SQL_ERROR,
                                 EVENTLOG_ERROR_TYPE,
                                 srvrGlobal->nskProcessInfo.processId,
                                 ODBCMX_SERVER,
                                 srvrGlobal->srvrObjRef,
                                 2,
                                 errNumStr,
                                 RequestError);

                    exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
                    exception_->u.SQLError.errorList._length = QryCatalogSrvrStmt->sqlError.errorList._length;
                    exception_->u.SQLError.errorList._buffer = QryCatalogSrvrStmt->sqlError.errorList._buffer;
                    exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECDIRECT_FAILED;
                 }

              } //try
              catch (...)
              {
                 SQL_EXEC_AssignParserFlagsForExSqlComp_Internal(savedParserFlags);
                 SendEventMsg(MSG_PROGRAMMING_ERROR,
                              EVENTLOG_ERROR_TYPE,
                              srvrGlobal->nskProcessInfo.processId,
                              ODBCMX_SERVER,
                              srvrGlobal->srvrObjRef,
                              1,
                              "Exception in executing Catalog API");

                 exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
                 exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECDIRECT_FAILED;
              } // catch
           }
        }

        if (exception_->exception_nr == 0)
        {
           QryCatalogSrvrStmt->sqlStmtType = TYPE_SELECT_CATALOG;
           outputDesc->_length = QryCatalogSrvrStmt->outputDescList._length;
           outputDesc->_buffer = QryCatalogSrvrStmt->outputDescList._buffer;
        }

MapException:
        // resource statistics
        if (resStatSession != NULL)
        {
               if (exception_->exception_nr != 0)
                              resStatSession->totalCatalogErrors ++;
               if (sqlWarning->_length != 0)
                              resStatSession->totalCatalogWarnings ++;
        }
        if (resStatStatement != NULL)
               resStatStatement->catFlagOn = FALSE;
        if (exception_->exception_nr != 0)
        {
               SRVR_STMT_HDL *pSrvrStmt = NULL;
               if ((pSrvrStmt = getSrvrStmt(catStmtLabel, FALSE)) != NULL)
               {
                       exception_->exception_nr = 0;
                       exception_->exception_detail = 0;
                       pSrvrStmt->rowsAffected = 0;
                       // CLEARDIAGNOSTICS(pSrvrStmt);
                       if (pSrvrStmt->bSQLMessageSet)
                              pSrvrStmt->cleanupSQLMessage();
               }
        }

        SRVRTRACE_EXIT(FILE_SME+14);
        return;
}

/*
 * Synchronous method function prototype for
 * operation 'odbc_SQLSvc_InitializeDialogue'
 */
extern "C" void
odbc_SQLSvc_InitializeDialogue_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLSvc_InitializeDialogue_exc_ *exception_
  , /* In    */ const USER_DESC_def *userDesc
  , /* In    */ const CONNECTION_CONTEXT_def *inContext
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* Out	 */ OUT_CONNECTION_CONTEXT_def *outContext
  )
{
	SRVRTRACE_ENTER(FILE_SME+15);

	exception_->exception_nr = 0;
	Int32 retcode = SQL_SUCCESS;
	char userSid[MAX_TEXT_SID_LEN+1];
	char pwd[387];
	pwd[0] = '\0';

	__int64			julian = 0;
	char primary_rolename[MAX_ROLENAME_LEN+1];
	char logon_rolename[MAX_ROLENAME_LEN+1];
	short logon_roleLen = 0;
        __int64 redefTime = 0;
	Int32  authIDType = 0;
	int encryption = 1;
	int refresh = 0;
	char* pPWD = (char*)userDesc->password._buffer;

	bzero(primary_rolename, sizeof(primary_rolename));

//	volatile int done = 0;
//	while (!done) {
//		sleep(10);
//	}



	userSid[0]  = 0;

// #ifdef _TMP_SQ_SECURITY

UA_Status       authErrorDetail;
DBUserAuth     *userSession = DBUserAuth::GetInstance();
size_t          passwordLen = 0;
CLIENT_INFO     client_info;
PERFORMANCE_INFO performanceInfo;
AUTHENTICATION_INFO authenticationInfo;

	if (userDesc->userDescType != AUTHENTICATED_USER_TYPE)
	{
	        srvrGlobal->QSRoleName[0] = '\0';
		int64 tempStartTime, tempEndTime = 0;
		int64 loginStartTime = JULIANTIMESTAMP();
		client_info.client_name = setinit.clientId;
		client_info.client_user_name = setinit.clientUserName;
		client_info.application_name = setinit.applicationId;

#ifndef _DEBUG
		// Disable generation of cores in release version
		struct rlimit rlim;
		int limit, ret_rlimit;

		// Get the current limit and save.
		ret_rlimit = getrlimit(RLIMIT_CORE, &rlim);
		limit = rlim.rlim_cur;

		// Set the current limit to 0, disabling generation of core
		rlim.rlim_cur = 0;
		ret_rlimit = setrlimit(RLIMIT_CORE, &rlim);
#endif

		retcode = decrypt_message(pPWD,primary_rolename);

		tempStartTime = JULIANTIMESTAMP() ;

		if (retcode == SECMXO_NO_ERROR)
         	{
                        size_t length = strlen(userDesc->userName);
 	                for (size_t i = 0; i < length; i++)
	                   userDesc->userName[i] = toupper(userDesc->userName[i]);

	                retcode = userSession->verify(userDesc->userName
						      ,pPWD
                                                      ,authErrorDetail
						      ,authenticationInfo
						      ,client_info
						      ,performanceInfo
						      );
		}
		if (retcode == 0)
		{
			srvrGlobal->redefTime = authenticationInfo.usersInfo.redefTime;
			strcpy(srvrGlobal->QSRoleName, "NONE");
		}

#ifndef _DEBUG
		// Revert to the previous setting for core generation
		rlim.rlim_cur = limit;
		ret_rlimit = setrlimit(RLIMIT_CORE, &rlim);
#endif

		tempEndTime = JULIANTIMESTAMP();
		setinit.ldapLoginTime = tempEndTime - tempStartTime;
		setinit.totalLoginTime = tempEndTime - loginStartTime;
                setinit.sqlUserTime = performanceInfo.sqlUserTime;
                setinit.searchConnectionTime = performanceInfo.searchConnectionTime;
                setinit.searchTime = performanceInfo.searchTime;
                setinit.authenticationConnectionTime = performanceInfo.authenticationConnectionTime;
                setinit.authenticationTime = performanceInfo.authenticationTime;
		
		if (retcode != SECMXO_NO_ERROR)
		{
			exception_->exception_detail = retcode;
			exception_->exception_nr = odbc_SQLSvc_InitializeDialogue_SQLError_exn_;
			srvrGlobal->QSRoleName[0] = '\0';
			SETSECURITYERROR(retcode, &exception_->u.SQLError.errorList);
			SRVRTRACE_EXIT(FILE_SME+15);
			if (retcode == SECMXO_INTERNAL_ERROR_FATAL)
			{
				SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
					1, "Security layer returned fatal error. Server exiting.");
				exit(0);
			}
			return;
		}

		tempEndTime = JULIANTIMESTAMP();
		setinit.totalLoginTime = tempEndTime - loginStartTime;
                
		if (authenticationInfo.error == 0)
          		retcode = WSQL_EXEC_SetAuthID(userDesc->userName,
                                                      authenticationInfo.usersInfo.databaseUsername, 
                                                      authenticationInfo.tokenKey,
                                                      authenticationInfo.tokenKeySize,
                                                      authenticationInfo.usersInfo.effectiveUserID,
                                                      authenticationInfo.usersInfo.sessionUserID);
                else
		{
			bool bSQLMessageSet;
			exception_->exception_detail = -8837;
			if (authenticationInfo.errorDetail == 1)
                        {
                           setAuthenticationError(bSQLMessageSet,&(exception_->u.SQLError),userDesc->userName,false);
			   exception_->exception_nr = odbc_SQLSvc_InitializeDialogue_InvalidUser_exn_;
                        }
			else
			{
                           setAuthenticationError(bSQLMessageSet,&(exception_->u.SQLError),userDesc->userName,true);
			   exception_->exception_nr = odbc_SQLSvc_InitializeDialogue_SQLError_exn_;
			   ERROR_DESC_def *sqlError = exception_->u.SQLError.errorList._buffer;
			   SendEventMsg(MSG_SQL_ERROR, EVENTLOG_ERROR_TYPE,
			     	srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			     	3, ODBCMX_SERVER, sqlError->sqlstate, sqlError->errorText);
			}
		}
	}

	SRVRTRACE_EXIT(FILE_SME+15);
	return;
}

/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_TerminateDialogue'
 */
extern "C" void
odbc_SQLSvc_TerminateDialogue_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLSvc_TerminateDialogue_exc_ *exception_
  , /* In    */ DIALOGUE_ID_def dialogueId
  )
{
	SRVRTRACE_ENTER(FILE_SME+16);

	exception_->exception_nr = 0;
	//odbc_SQLSvc_EndTransaction_exc_ endTransactionException;
	//ERROR_DESC_LIST_def	sqlWarning;


	// Rollback the transaction, Don't bother to check if autocommit is on or off, since SQL
	// doesn't check for it
	// When there is no transaction outstanding, SQL would give an error and ignore this error.

	if (WSQL_EXEC_Xact(SQLTRANS_STATUS,NULL) == 0)
		EXECDIRECT("ROLLBACK WORK");
	//odbc_SQLSvc_EndTransaction_sme_(objtag_, call_id_, &endTransactionException,
	//								dialogueId, SQL_ROLLBACK
	//								,&sqlWarning
	//									);

	// resource statistics resStatSession->end() is called in SRVR::BreakDialogue()
	//if (resStatSession != NULL)
	//	resStatSession->end();
	//end rs

	SRVRTRACE_EXIT(FILE_SME+16);
	return;
}

/*
 *
 * Synchronous method function for
 * operation 'odbc_SQLSvc_SetConnectionOption'
 */
extern "C" void
odbc_SQLSvc_SetConnectionOption_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLSvc_SetConnectionOption_exc_ *exception_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_short connectionOption
  , /* In    */ Int32  optionValueNum
  , /* In    */ IDL_string optionValueStr
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
 )
{
	SRVRTRACE_ENTER(FILE_SME+17);

	char stmtLabel[MAX_STMT_LABEL_LEN+1];
	char sqlString[256];
	Int32 retcode = 0;
	bool sqlStringNeedsExecution = true;

	SRVR_STMT_HDL *pSrvrStmt = NULL;
	SRVR_STMT_HDL *resStmt;

	int64 	local_xid;
	UINT	xid_length = sizeof(local_xid);
	char buffer[100];
	char valueStr[MAX_SQL_IDENTIFIER_LEN+1];
	char schemaValueStr[MAX_SQL_IDENTIFIER_LEN+MAX_SQL_IDENTIFIER_LEN+5+1]; // 5 for quotes + dot
	char	*CatalogNameTypeStr = '\0';
	char *endPtr;

	exception_->exception_nr = CEE_SUCCESS;
	sqlWarning->_length = 0;
	sqlWarning->_buffer = NULL;
	memset(sqlString, 0, 256);

	// Given a label find out the SRVR_STMT_HDL
	if ((pSrvrStmt = getSrvrStmt("STMT_INTERNAL_1", TRUE)) == NULL)
	{
		exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
		return;
	}
	// clientLCID = srvrGlobal->clientLCID, is done during the creation of statement
	// handle. However, the in InitializeDialog(), the correct value of
	// srvrGlobal->clientLCID is set only after the creation of statement handle.
	// This causes an incorrect clientLCID value (the initalized value zero) to
	// be set on pSrvrStmt. This value dectates the character set translations.
	// An incorrect value of clientLCID causes incorrect translations for all the
	// executions on that statement. The fix is to set the correct value after the
	// creation of the statement.
	pSrvrStmt->setclientLCID(srvrGlobal->clientLCID);

	switch (connectionOption) {
//Special Case//
   
	case SQL_ACCESSMODE_AND_ISOLATION:
			switch (optionValueNum) {
		case SQL_TXN_READ_UNCOMMITTED:
			//Modified it, since on setting at DS level to uncommitted the subsequent connections were not going through.
			strcpy(sqlString, "SET TRANSACTION READ ONLY, ISOLATION LEVEL READ UNCOMMITTED");
			break;
		case SQL_TXN_READ_COMMITTED:
			//Modified it, since on setting at DS level to uncommitted the subsequent connections were not going through.
			strcpy(sqlString, "SET TRANSACTION READ WRITE, ISOLATION LEVEL READ COMMITTED");
			break;
		default:
			exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_OPTION_VALUE_NUM;
			return;
		}
		break;
//Special Case//
	case SQL_ACCESS_MODE:
		strcpy(sqlString, "SET TRANSACTION ");
		switch (optionValueNum) {
		case SQL_MODE_READ_WRITE:
			strcat(sqlString, "READ WRITE");
			break;
		case SQL_MODE_READ_ONLY:
			if ((srvrGlobal->EnvironmentType & MXO_MSACCESS_1997) || (srvrGlobal->EnvironmentType & MXO_MSACCESS_2000)) // change this to fix MS Access problem. Since it insert, update and delete while SQL_ACCESS_MODE is SQL_MODE_READ_ONLY.
				strcat(sqlString, "READ WRITE");
			else
				strcat(sqlString, "READ ONLY");
			break;
		case SQL_MODE_NULL:
		// Need to find out from profile what is the access Mode
		// at present return ParamError. Note there is no break
		default:
			exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_OPTION_VALUE_NUM;
			return;
		}
		break;
	case SQL_TXN_ISOLATION:
		strcpy(sqlString, "SET TRANSACTION ISOLATION LEVEL ");
			switch (optionValueNum) {
		case SQL_TXN_READ_UNCOMMITTED:
			strcat(sqlString, "READ UNCOMMITTED");
			break;
		case SQL_TXN_READ_COMMITTED:
			strcat(sqlString, "READ COMMITTED");
			break;
		case SQL_TXN_REPEATABLE_READ:
			strcat(sqlString, "REPEATABLE READ");
			break;
		case SQL_TXN_SERIALIZABLE:
			strcat(sqlString, "SERIALIZABLE");
			break;
		default:
			exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_OPTION_VALUE_NUM;
			return;
		}
		break;
	case SQL_ATTR_ENLIST_IN_DTC:

#ifdef TIP_DEFINED
		if (srvrGlobal->tip_gateway != NULL) {
			tip_close(srvrGlobal->tip_gateway);
			srvrGlobal->tip_gateway = NULL;
		}

		// Check for non-DTC transaction
		if (optionValueNum == NULL){
			SetTipUrl((IDL_char *)NULL);
			SetLocalXid(NULL, 0);
			exception_->exception_nr = 0;
			sqlWarning->_length = 0;
			sqlWarning->_buffer = NULL;
			return;
		}

		// Check for previous DTC transaction
		if(GetTipUrl() != (IDL_char *)NULL){
			SetTipUrl((IDL_char *)NULL);
			SetLocalXid(NULL, 0);
		}

		retcode = tip_open(&(srvrGlobal->tip_gateway));

		if (retcode != TIPOK){
			exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_OPEN_TIP_GATEWAY_FAILED;

			switch (retcode) {
			case TIPNOTCONNECTED:

					SendEventMsg (MSG_SRVR_DTC_TIP_NOTCONNECTED,
									EVENTLOG_WARNING_TYPE,
									srvrGlobal->nskProcessInfo.processId,
									ODBCMX_SERVER,
									srvrGlobal->srvrObjRef,
									0);
				break;
			case TIPNOTCONFIGURED:

					SendEventMsg (MSG_SRVR_DTC_TIP_NOTCONFIGURED,
									EVENTLOG_WARNING_TYPE,
									srvrGlobal->nskProcessInfo.processId,
									ODBCMX_SERVER,
									srvrGlobal->srvrObjRef,
									0);
				break;
			default:

					SendEventMsg (MSG_SRVR_DTC_TIP_ERROR,
									EVENTLOG_WARNING_TYPE,
									srvrGlobal->nskProcessInfo.processId,
									ODBCMX_SERVER,
									srvrGlobal->srvrObjRef,
									0);
				break;
			}
			return;
		}

		strncpy(buffer, optionValueStr, sizeof(buffer));
		buffer[sizeof(buffer)-1] = 0;

		retcode = tip_pull(srvrGlobal->tip_gateway,
							(IDL_char *)buffer,
							&local_xid,
							xid_length);

		if (retcode != TIPOK){
			exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_PULL_TIP_FAILED;

			switch (retcode) {
			case TIPNOTCONNECTED:

					SendEventMsg (MSG_SRVR_DTC_TIP_NOTCONNECTED,
									EVENTLOG_WARNING_TYPE,
									srvrGlobal->nskProcessInfo.processId,
									ODBCMX_SERVER,
									srvrGlobal->srvrObjRef,
									0);
				break;
			case TIPNOTCONFIGURED:

					SendEventMsg (MSG_SRVR_DTC_TIP_NOTCONFIGURED,
									EVENTLOG_WARNING_TYPE,
									srvrGlobal->nskProcessInfo.processId,
									ODBCMX_SERVER,
									srvrGlobal->srvrObjRef,
									0);
			break;
			default:

					SendEventMsg (MSG_SRVR_DTC_TIP_ERROR,
									EVENTLOG_WARNING_TYPE,
									srvrGlobal->nskProcessInfo.processId,
									ODBCMX_SERVER,
									srvrGlobal->srvrObjRef,
									0);
				break;
			}
		return;
		}

		SetTipUrl((IDL_char *)buffer);
		SetLocalXid(local_xid,
					xid_length);

		exception_->exception_nr = 0;
		sqlWarning->_length = 0;
		sqlWarning->_buffer = NULL;
#else
		// RS we'll return an error just in case a user tries to use TIP until we get the libraries
		sqlWarning->_length = 0;
		sqlWarning->_buffer = NULL;

		exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_OPEN_TIP_GATEWAY_FAILED;
		SendEventMsg (MSG_SRVR_DTC_TIP_ERROR,
						EVENTLOG_WARNING_TYPE,
						srvrGlobal->nskProcessInfo.processId,
						ODBCMX_SERVER,
						srvrGlobal->srvrObjRef,
						0);

#endif
		return;

	case SET_AUTOBEGIN:
		strcpy(sqlString, "SET TRANSACTION AUTOBEGIN ON");
		break;
	case SQL_AUTOCOMMIT:
		// if a change is required
		sqlStringNeedsExecution = false;
		if ( ((srvrGlobal->bAutoCommitOn == TRUE) && (optionValueNum == 0 ))
			|| ((srvrGlobal->bAutoCommitOn == FALSE) && (optionValueNum == 1 )) )
		{
			//check for active txn, if yes commit them
			if ((srvrGlobal->bAutoCommitOn == FALSE) && (WSQL_EXEC_Xact(SQLTRANS_STATUS,NULL) == 0))
				retcode = pSrvrStmt->ExecDirect(NULL, "COMMIT WORK", INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
			if (retcode != SQL_ERROR)
			{
				if (optionValueNum)
				{
					if( SQL_SUCCESS == (retcode = pSrvrStmt->ExecDirect(NULL, "SET TRANSACTION AUTOCOMMIT ON", INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0)))
						srvrGlobal->bAutoCommitOn = TRUE;
				}
				else
				{
					if( SQL_SUCCESS == (retcode = pSrvrStmt->ExecDirect(NULL, "SET TRANSACTION AUTOCOMMIT OFF", INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0)))
						srvrGlobal->bAutoCommitOn = FALSE;
				}
		    }
		}
		else
			return;
		break;
	case SET_CATALOG:
	case SQL_ATTR_CURRENT_CATALOG:
	{
		sqlStringNeedsExecution = false;
		int len = 0;
		bool defaultCatalog = false;
		bool isDoubleQuoted = false;
		char* tempSqlString = NULL;

		if (optionValueStr == NULL || (optionValueStr != NULL && optionValueStr[0] == '\0'))
		{
			len = strlen(ODBCMX_DEFAULT_CATALOG);
			defaultCatalog = true;
		}
		else
			len = strlen(optionValueStr);

		tempSqlString = new char[len+20];
		if (tempSqlString == NULL)
		{
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
						srvrGlobal->srvrObjRef, 1, "SET CATALOG");
			exit(0);
		}
		if (!defaultCatalog && optionValueStr[0] == '"' && optionValueStr[len-1] == '"')
			isDoubleQuoted = true;

		strcpy(tempSqlString, "SET CATALOG ");
		if (!isDoubleQuoted)
			strcat(tempSqlString, "'");
		if (defaultCatalog)
			strcat(tempSqlString, ODBCMX_DEFAULT_CATALOG);
		else
			strcat(tempSqlString, optionValueStr);
		if (!isDoubleQuoted)
			strcat(tempSqlString, "'");

		if( SQL_SUCCESS == (retcode = pSrvrStmt->ExecDirect(NULL, tempSqlString, INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0)))
		{
			if (defaultCatalog)
				strcpy(srvrGlobal->DefaultCatalog, ODBCMX_DEFAULT_CATALOG);
			else if (isDoubleQuoted)
			{
				strncpy(srvrGlobal->DefaultCatalog, optionValueStr+1, len-2);
				srvrGlobal->DefaultCatalog[len-2] = '\0';
			}
			else
				strcpy(srvrGlobal->DefaultCatalog, optionValueStr);
		}
		delete [] tempSqlString;
	}
		break;
	case SET_SCHEMA:
	{
		if (optionValueStr == NULL || (optionValueStr != NULL && optionValueStr[0] == '\0'))
			sprintf(schemaValueStr, "%s.%s", ODBCMX_DEFAULT_CATALOG, ODBCMX_DEFAULT_SCHEMA);
		else
		{
			if (strlen(optionValueStr) < sizeof(schemaValueStr))
				strcpy(schemaValueStr, optionValueStr);
			else
			{
				exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_OPTION_VALUE_STR;
				return;
			}
		}
		strcpy(sqlString, "SET SCHEMA ");
		strncat(sqlString, schemaValueStr, sizeof(sqlString));
		sqlString[sizeof(sqlString)-1] = 0;
	}
		break;
	case RESET_DEFAULTS:
		strcpy(sqlString, "CONTROL QUERY DEFAULT * RESET");
		break;
	case RESET_RESET_DEFAULTS:
		strcpy(sqlString, "CONTROL QUERY DEFAULT * RESET RESET");
		break;
	case CUT_CONTROLQUERYSHAPE:
		strcpy(sqlString, "CONTROL QUERY SHAPE CUT");
		break;
	case BEGIN_SESSION:
		if(optionValueStr != NULL && strlen(optionValueStr) > 0)
			sprintf(sqlString,"SET SESSION DEFAULT SQL_SESSION 'BEGIN:%0.200s';",optionValueStr);
		else
			strcpy(sqlString, "SET SESSION DEFAULT SQL_SESSION 'BEGIN';");
		break;
	// Added the below workaround for volatile table SQL problem
	// Not called any more from initailizeDialog since executor fixed it.
	case SET_SESSION_USERNAME:
		sprintf( sqlString, "CONTROL QUERY DEFAULT session_id '%s'", srvrGlobal->sessionId );
		break;
	case END_SESSION:
		strcpy(sqlString, "SET SESSION DEFAULT SQL_SESSION 'END'");
		break;
	case SET_CATALOGNAMETYPE:
		strcpy(sqlString, "SET NAMETYPE ANSI");
		break;
	case SET_SETANDCONTROLSTMTS:
		break;
	case SET_ODBC_PROCESS:
		strcpy(sqlString, "CONTROL QUERY DEFAULT ODBC_PROCESS 'TRUE'");
		break;
	case SET_JDBC_PROCESS:
		strcpy(sqlString, "CONTROL QUERY DEFAULT JDBC_PROCESS 'TRUE'");
		break;
	case SET_INFER_NCHAR:
		strcpy(sqlString, "CONTROL QUERY DEFAULT INFER_CHARSET 'ON'");
		break;
	case SET_EXPLAIN_PLAN:
		strcpy(sqlString,"CONTROL QUERY DEFAULT GENERATE_EXPLAIN 'ON'");
		break;
	case SQL_ATTR_ROWSET_RECOVERY:
		if (optionValueNum)
			srvrGlobal->EnvironmentType |= MXO_ROWSET_ERROR_RECOVERY;
		else
			srvrGlobal->EnvironmentType &= (0xFFFF-MXO_ROWSET_ERROR_RECOVERY);
		return;
	case SQL_ATTR_CONCURRENCY:
		strcpy(sqlString, "CONTROL QUERY DEFAULT READONLY_CURSOR ");
		switch (optionValueNum)
		{
			case SQL_CONCUR_READ_ONLY:
				strcat(sqlString, "'TRUE'");
				break;
			case SQL_CONCUR_LOCK:
			case SQL_CONCUR_ROWVER:
			case SQL_CONCUR_VALUES:
				strcat(sqlString, "'FALSE'");
				break;
			default:
				exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_OPTION_VALUE_NUM;
				return;
		}
		break;
	case JDBC_ATTR_CONN_IDLE_TIMEOUT:
		if (srvrGlobal->drvrVersion.componentId == JDBC_DRVR_COMPONENT)
		{
			if (optionValueNum > JDBC_DATASOURCE_CONN_IDLE_TIMEOUT)
				srvrGlobal->javaConnIdleTimeout = optionValueNum;
			else
				srvrGlobal->javaConnIdleTimeout = JDBC_DATASOURCE_CONN_IDLE_TIMEOUT;
		}
		else
		{
			exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_CONNECTION_OPTION;
		}
		return;
	case CONN_IDLE_TIMER_RESET:
		// this connection attribute is JDBC exclusive, NDCS just need to recognize it and does nothing
		return;
		break;
	// Set priority of DP2 relative to master executor/ESPs
	case CONTROL_TABLE_PRIORITY:
		sprintf(sqlString,"CONTROL TABLE * PRIORITY_DELTA '%d'", optionValueNum);
		break;
	case SET_STATISTICS:
		strcpy(sqlString,"CONTROL QUERY DEFAULT detailed_statistics 'PERTABLE'");
		break;
	// JDBC sets this connection attribute to set the Proxy syntax for SPJ result sets
	case SET_SPJ_ENABLE_PROXY:
		if(optionValueNum)
			srvrGlobal->bSpjEnableProxy = true;
		else
			srvrGlobal->bSpjEnableProxy = false;
		return;
		break;
	case SQL_ATTR_JOIN_UDR_TRANSACTION:
		errno = 0;
		endPtr = NULL;
		srvrGlobal->spjTxnId = strtoll(optionValueStr, &endPtr, 10);
		//srvrGlobal->spjTxnId = atoll(optionValueStr);
		if( errno == 0 )
		{
#ifdef SPJ_TXN_TEST
			sprintf(msg, "SQL_ATTR_JOIN_UDR_TRANSACTION %lld", srvrGlobal->spjTxnId);
			SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
					srvrGlobal->srvrObjRef, 1, msg);
#endif

			retcode = JOINTRANSACTION( srvrGlobal->spjTxnId );
			if(retcode != 0)
			{
				sprintf(buffer,"Transaction join failed with error %d",retcode);
				exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = buffer;
			}
			else
				srvrGlobal->bspjTxnJoined = TRUE;
		}
		else
		{
			sprintf(buffer,"Unable to retrieve transaction ID. Error %d",errno);
			exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = buffer;
		}
		return;
		break;
	case SQL_ATTR_SUSPEND_UDR_TRANSACTION:
		endPtr = NULL;
		errno = 0;
		srvrGlobal->spjTxnId = strtoll(optionValueStr, &endPtr, 10);
		//srvrGlobal->spjTxnId = atoll(optionValueStr);
		if( errno == 0 )
		{
#ifdef SPJ_TXN_TEST
			sprintf(msg, "SQL_ATTR_SUSPEND_UDR_TRANSACTION %lld", srvrGlobal->spjTxnId);
			SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
					srvrGlobal->srvrObjRef, 1, msg);
#endif

			retcode = SUSPENDTRANSACTION( (short*)&(srvrGlobal->spjTxnId) );
			if(retcode != 0)
			{
				sprintf(buffer,"Transaction suspend failed with error %d",retcode);
				exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = buffer;
			}
			else
				srvrGlobal->bspjTxnJoined = FALSE;
		}
		else
		{
			sprintf(buffer,"Unable to retrieve transaction ID. Error %d",errno);
			exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = buffer;
		}
		return;
		break;
	case SET_INPUT_CHARSET:
		snprintf(sqlString,200,"CONTROL QUERY DEFAULT input_charset '%s'", getCharsetStr(srvrGlobal->clientLCID));
		break;
	case SET_TERMINAL_CHARSET:
		snprintf(sqlString,200,"CONTROL QUERY DEFAULT terminal_charset '%s'", getCharsetStr(srvrGlobal->clientErrorLCID));
		break;
	case SET_NVCI_PROCESS:
		sprintf(sqlString, "CONTROL QUERY DEFAULT NVCI_PROCESS 'ON'");
		break;
	case WMS_QUERY_MONITORING:
		strcpy(sqlString, "CONTROL QUERY DEFAULT WMS_QUERY_MONITORING 'OFF'");
		break;
    case SQL_ATTR_CLIPVARCHAR:
        srvrGlobal->clipVarchar = optionValueNum;
		sqlStringNeedsExecution = false;
        break;
	default:
		exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_CONNECTION_OPTION;
		return;
	}

	if(sqlStringNeedsExecution)
	{
		if (connectionOption == SET_SETANDCONTROLSTMTS)
			retcode = pSrvrStmt->ExecDirect(NULL, optionValueStr, INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
		else
			retcode = pSrvrStmt->ExecDirect(NULL, sqlString, INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
	}
	switch (retcode)
		{
			case SQL_SUCCESS:
				exception_->exception_nr = 0;
				// Ignore estimatedCost and rowsAffected
				sqlWarning->_length = pSrvrStmt->sqlWarning._length;
				sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
			break;
			case SQL_ERROR:
				if(pSrvrStmt->sqlError.errorList._buffer->sqlcode == -15371) // Executor dev should be treated as warning.
				{
					exception_->exception_nr = 0;
					SendEventMsg(MSG_SQL_WARNING, EVENTLOG_WARNING_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
					srvrGlobal->srvrObjRef, 3, "SQL/MX", "15371", sqlString);
				}
				else
				{
					exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_SQLError_exn_;
					exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
					exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
				}
			break;
			case PROGRAM_ERROR:
				exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
				exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_SETCONNECTOPTION_FAILED;
			default:
			break;
	}

	SRVRTRACE_EXIT(FILE_SME+17);
	return;
}

extern "C" void
odbc_SQLSrvr_FetchPerf_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ Int32 *returnCode
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ Int32 maxRowCnt
  , /* In    */ Int32 maxRowLen
  , /* In    */ IDL_short sqlAsyncEnable
  , /* In    */ Int32 queryTimeout
  , /* Out   */ Int32 *rowsAffected
  , /* Out   */ Int32 *outValuesFormat
  , /* Out   */ SQL_DataValue_def *outputDataValue
  , /* Out   */ Int32 *sqlWarningOrErrorLength
  , /* Out   */ BYTE     *&sqlWarningOrError
)
{
	SRVRTRACE_ENTER(FILE_SME+8);

	SRVR_STMT_HDL *pSrvrStmt = NULL;
	SQLRETURN rc = SQL_SUCCESS;
	int outputDataOffset = 0;

	*returnCode = SQL_SUCCESS;
    long long tmpLength;
	if (maxRowCnt < 0)
	{
		*returnCode = SQL_ERROR;
		GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Row Count", sqlWarningOrErrorLength, sqlWarningOrError);

	}
	else
	{
		pSrvrStmt = getSrvrStmt(stmtLabel, FALSE);

		if (pSrvrStmt == NULL)
		{
			*returnCode = SQL_ERROR;
			GETMXCSWARNINGORERROR(-1, "HY000", "Statement Label not found", sqlWarningOrErrorLength, sqlWarningOrError);
		}
		else
		{
			if (pSrvrStmt->sqlWarningOrErrorLength > 0 &&
				pSrvrStmt->sqlWarningOrError != NULL)
			{
	           delete pSrvrStmt->sqlWarningOrError;
			}
            pSrvrStmt->sqlWarningOrErrorLength = 0;
            pSrvrStmt->sqlWarningOrError = NULL;
		}

	}

	if (*returnCode == SQL_SUCCESS)
	{
// limit on maxRowsFetched from WMS
		if (pSrvrStmt->sqlStmtType != TYPE_SELECT_CATALOG)
		{
			if (srvrGlobal->maxRowsFetched != 0 && pSrvrStmt->m_bDoneWouldLikeToExecute)
			{
				if (srvrGlobal->maxRowsFetched <= pSrvrStmt->m_curRowsFetched)
				{
					WSQL_EXEC_CloseStmt(&pSrvrStmt->stmt);
					WSQL_EXEC_ClearDiagnostics(&pSrvrStmt->stmt);
					pSrvrStmt->isClosed = true;
					pSrvrStmt->bFirstSqlBulkFetch = false;
					pSrvrStmt->m_curRowsFetched = 0;
//					*returnCode = SQL_NO_DATA_FOUND;
					*returnCode = SQL_ERROR;
					GETMXCSWARNINGORERROR(-1, "HY000", "The limit for maximum rows to be returned for a query, as set by the administrator, was exceeded", sqlWarningOrErrorLength, sqlWarningOrError);
					goto ret;
				}
				else
				{
					if (pSrvrStmt->bFirstSqlBulkFetch == true)
						pSrvrStmt->m_curRowsFetched = 0;

					if (pSrvrStmt->m_curRowsFetched + maxRowCnt <= srvrGlobal->maxRowsFetched )
						pSrvrStmt->maxRowCnt = maxRowCnt;
					else
					{
						pSrvrStmt->maxRowCnt = srvrGlobal->maxRowsFetched - pSrvrStmt->m_curRowsFetched;
						if (pSrvrStmt->maxRowCnt <= 0)
							pSrvrStmt->maxRowCnt = 1;
					}
				}
			}
			else
				pSrvrStmt->maxRowCnt = maxRowCnt;
		}
		else
			pSrvrStmt->maxRowCnt = maxRowCnt;
		pSrvrStmt->maxRowLen = maxRowLen;

		// resource statistics
		if (resStatStatement != NULL && pSrvrStmt->isClosed == FALSE && pSrvrStmt->bFetchStarted == FALSE && pSrvrStmt->stmtType == EXTERNAL_STMT)
		{
			pSrvrStmt->bFetchStarted = TRUE;
			pSrvrStmt->inState = inState = STMTSTAT_FETCH;
			inSqlStmtType = TYPE_UNKNOWN;
			inEstimatedCost = 0;
			inQueryId = NULL;
			inSqlString = NULL;
			inErrorStatement = 0;
			inWarningStatement = 0;
			inRowCount = 0;
			inErrorCode = 0;
			inSqlError = NULL;
			inSqlErrorLength = 0;
			/*resStatStatement->start(inState,
									pSrvrStmt->sqlQueryType,
									stmtLabel,
									pSrvrStmt->sqlUniqueQueryID,
						pSrvrStmt->cost_info,
						pSrvrStmt->comp_stats_info,
						inEstimatedCost,
						&pSrvrStmt->m_need_21036_end_msg);*/
			resStatStatement->start(inState,
						pSrvrStmt->sqlQueryType,
						stmtLabel,
						pSrvrStmt,
									inEstimatedCost,
									&pSrvrStmt->m_need_21036_end_msg);
		}
		// end rs
		if (pSrvrStmt->sqlStmtType != TYPE_SELECT_CATALOG)
		{
			if (pSrvrStmt->bSQLMessageSet)
				pSrvrStmt->cleanupSQLMessage();

			if(pSrvrStmt->outputDataValue._length > 0 &&
			   pSrvrStmt->outputDataValue._buffer != NULL)
			   delete pSrvrStmt->outputDataValue._buffer;

			pSrvrStmt->outputDataValue._length = 0;
			pSrvrStmt->outputDataValue._buffer = NULL;

			if (pSrvrStmt->isClosed)
			{
				pSrvrStmt->m_curRowsFetched = 0;
				pSrvrStmt->bFirstSqlBulkFetch = false;
				*returnCode = SQL_NO_DATA_FOUND;
				goto ret;
			}


			pSrvrStmt->currentMethod = odbc_SQLSvc_FetchPerf_ldx_;
//			if (pSrvrStmt->sqlBulkFetchPossible && (pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE || pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET))
			if (srvrGlobal->drvrVersion.buildId & ROWWISE_ROWSET)
			{
				*outValuesFormat = ROWWISE_ROWSETS;

				rc = FETCH2bulk(pSrvrStmt);
				tmpLength = clipVarchar(pSrvrStmt);
				if (pSrvrStmt->rowsAffected > 0){
					if(pSrvrStmt->outputDataValue._length == 0 && pSrvrStmt->outputDataValue._buffer == NULL)
					{
						outputDataValue->_buffer = pSrvrStmt->outputDescVarBuffer;
						outputDataValue->_length = tmpLength;
					}
					else
					{
						outputDataValue->_buffer = pSrvrStmt->outputDataValue._buffer;
						outputDataValue->_length = pSrvrStmt->outputDataValue._length;
					}
				}
				else
				{
					outputDataValue->_buffer = NULL;
					outputDataValue->_length = 0;
				}
			}
			else
			{
				*outValuesFormat = COLUMNWISE_ROWSETS;

				//pSrvrStmt->maxRowCnt = maxRowCnt;
				//pSrvrStmt->maxRowLen = maxRowLen;
                rc = FETCHPERF(pSrvrStmt, outputDataValue);
			}

			switch (rc)
			{
			   case ODBC_RG_WARNING:
			   case SQL_SUCCESS_WITH_INFO:
				   *returnCode = SQL_SUCCESS_WITH_INFO;
				   GETSQLWARNINGORERROR2(pSrvrStmt);
	               *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
	               sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
				   *rowsAffected = pSrvrStmt->rowsAffected;
				   if (*rowsAffected > 0)
						pSrvrStmt->m_curRowsFetched += *rowsAffected;
				   break;

			   case SQL_SUCCESS:
				   *returnCode = SQL_SUCCESS;
				   *rowsAffected = pSrvrStmt->rowsAffected;
				   if (*rowsAffected > 0)
						pSrvrStmt->m_curRowsFetched += *rowsAffected;
				   break;

			   case SQL_STILL_EXECUTING:
				   *returnCode = SQL_STILL_EXECUTING;
				   break;

			   case SQL_INVALID_HANDLE:
				   *returnCode = SQL_INVALID_HANDLE;
				   break;

			   case SQL_NO_DATA_FOUND:
				   pSrvrStmt->bFirstSqlBulkFetch = false;
				   *returnCode = SQL_NO_DATA_FOUND;
				   break;

			   case SQL_ERROR:
				   pSrvrStmt->bFirstSqlBulkFetch = false;
	               *returnCode = SQL_ERROR;
	               GETSQLWARNINGORERROR2(pSrvrStmt);
	               *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
	               sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
	               break;

			   case PROGRAM_ERROR:
				   pSrvrStmt->bFirstSqlBulkFetch = false;
				   *returnCode = SQL_ERROR;
				   GETMXCSWARNINGORERROR(-1, "HY000", "Fetch Failed", sqlWarningOrErrorLength, sqlWarningOrError);
				   break;

			   default:
				   break;
			}
		}
		else
		{ // Catalog APIs
			outputDataOffset  = *(int*)pSrvrStmt->outputDataValue.pad_to_offset_8_;

			*outValuesFormat = COLUMNWISE_ROWSETS;
			rc = FETCHPERF(pSrvrStmt, &pSrvrStmt->outputDataValue);
			if (pSrvrStmt->sqlError.errorList._buffer != NULL)
			{
				*returnCode = SQL_ERROR;
				GETSQLWARNINGORERROR2(pSrvrStmt);
				*sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
				sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
				if (pSrvrStmt->outputDataValue._buffer != NULL)
					delete pSrvrStmt->outputDataValue._buffer;
				pSrvrStmt->outputDataValue._buffer = NULL;
				pSrvrStmt->outputDataValue._length = 0;

			}
			else if (pSrvrStmt->rowsAffected == 0 || pSrvrStmt->rowsAffected == -1)
			{
				if (pSrvrStmt->bSQLMessageSet)
						pSrvrStmt->cleanupSQLMessage();
				pSrvrStmt->outputDataValue._buffer = NULL;
				pSrvrStmt->outputDataValue._length = 0;
				*(int*)pSrvrStmt->outputDataValue.pad_to_offset_8_=0;
				outputDataOffset = 0;
				pSrvrStmt->InternalStmtClose(SQL_CLOSE);
				*returnCode = SQL_NO_DATA_FOUND;
			}
			else
			{
				*rowsAffected = pSrvrStmt->rowsAffected;

				if (pSrvrStmt->sqlWarning._length != 0)
				{
					*returnCode = SQL_SUCCESS_WITH_INFO;
                    GETSQLWARNINGORERROR2(pSrvrStmt);
	                *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
	                sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
				}
				else
				{
					char *tmpByte = (char*)&pSrvrStmt->outputDataValue._length;
					for(int i=0; i<sizeof(pSrvrStmt->outputDataValue.pad_to_offset_8_); i++) {
						pSrvrStmt->outputDataValue.pad_to_offset_8_[i] = *tmpByte;
						tmpByte++;
					}
					
					*returnCode = SQL_SUCCESS;
				}

				pSrvrStmt->rowsAffected = 0;
			}

			outputDataValue->_length = pSrvrStmt->outputDataValue._length - outputDataOffset;
			outputDataValue->_buffer = pSrvrStmt->outputDataValue._buffer + outputDataOffset;
		}

ret:

		if (*returnCode != SQL_SUCCESS &&
				*returnCode != SQL_SUCCESS_WITH_INFO)
		{
			if (pSrvrStmt->outputDataValue._buffer != NULL)
				delete pSrvrStmt->outputDataValue._buffer;
			pSrvrStmt->outputDataValue._length = 0;
			pSrvrStmt->outputDataValue._buffer = NULL;
		}

		if (pSrvrStmt->sqlNewQueryType == SQL_SP_RESULT_SET)
		{
			if (pSrvrStmt->callStmtHandle->isClosed == true && *returnCode == SQL_NO_DATA_FOUND || *returnCode == SQL_ERROR)
			{
				pSrvrStmt->callStmtHandle->inState = STMTSTAT_CLOSE;
				// Fix for CR 6059
				if( resStatStatement != NULL )
					resStatStatement->setStatistics(pSrvrStmt->callStmtHandle);
			}
		}
		else if (resStatStatement != NULL && pSrvrStmt->bFetchStarted == TRUE &&
						(*returnCode == SQL_NO_DATA_FOUND || *returnCode == SQL_ERROR ||
						((*returnCode == SQL_SUCCESS || *returnCode == SQL_SUCCESS_WITH_INFO) && *rowsAffected < maxRowCnt)))

		{
			resStatStatement->setStatistics(pSrvrStmt);
		}
	}

	// resource statistics
	if (resStatStatement != NULL && pSrvrStmt != NULL && pSrvrStmt->isClosed == TRUE && pSrvrStmt->bFetchStarted == TRUE && pSrvrStmt->stmtType == EXTERNAL_STMT)
	{
		if (*returnCode == SQL_ERROR && pSrvrStmt->sqlWarningOrError != NULL)
		{
			inErrorCode = *(Int32 *)(pSrvrStmt->sqlWarningOrError+8);
			inSqlError = (char*)pSrvrStmt->sqlWarningOrError + 16;
			inSqlErrorLength =*(Int32 *)(pSrvrStmt->sqlWarningOrError + 12);
		}
		pSrvrStmt->bFetchStarted = FALSE;
		Int32 inMaxRowCnt = 0;
		Int32 inMaxRowLen = 0;

		inMaxRowCnt = maxRowCnt;
		inMaxRowLen = maxRowLen;

		if (*returnCode != SQL_SUCCESS &&
			*returnCode != SQL_SUCCESS_WITH_INFO)
			inErrorStatement ++;
		else
			setStatisticsFlag  = FALSE;

		if (*returnCode == SQL_SUCCESS_WITH_INFO)
			inWarningStatement ++;

		if (*returnCode == SQL_NO_DATA_FOUND)
		{
			inErrorStatement = 0;
			inWarningStatement = 0;
			setStatisticsFlag = TRUE;

		}
		inQueryId = pSrvrStmt->sqlUniqueQueryID;
		inSqlQueryType = pSrvrStmt->sqlQueryType;
		resStatStatement->setStatisticsFlag(setStatisticsFlag);
		resStatStatement->end(inState,
							  inSqlQueryType,
							  inSqlStmtType,
							  inQueryId,
							  inEstimatedCost,
							  inSqlString,
							  inErrorStatement,
							  inWarningStatement,
							  inRowCount,
							  inErrorCode,
							  resStatSession,
							  inSqlErrorLength,
							  inSqlError,
							  pSrvrStmt,
							  &pSrvrStmt->m_need_21036_end_msg,
							  pSrvrStmt->sqlNewQueryType,
							  pSrvrStmt->isClosed);
	}
	//end rs

	SRVRTRACE_EXIT(FILE_SME+8);

	return;

} // odbc_SQLSrvr_FetchPerf_sme_()


extern "C" void
odbc_SQLSrvr_ExtractLob_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLsrvr_ExtractLob_exc_ *exception_
  , /* In    */ IDL_short extractLobAPI
  , /* In    */ IDL_string lobHandle
  , /* In    */ IDL_long_long &lobLength
  , /* Out   */ IDL_long_long &extractLen
  , /* Out   */ BYTE *& extractData
  )
{
    char LobExtractQuery[1000] = {0};
    char RequestError[200] = {0};
    SRVR_STMT_HDL  *QryLobExtractSrvrStmt = NULL;

    if ((QryLobExtractSrvrStmt = getSrvrStmt("MXOSRVR_EXTRACRTLOB", TRUE)) == NULL)
    {
        SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR,
                     EVENTLOG_ERROR_TYPE,
                     srvrGlobal->nskProcessInfo.processId,
                     ODBCMX_SERVER,
                     srvrGlobal->srvrObjRef,
                     2,
                     "EXTRACT LOB APIs",
                     "Allocate Statement");
        exception_->exception_nr = odbc_SQLsrvr_ExtractLob_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
    }
    switch (extractLobAPI) {
    case 0:
        extractData = NULL;
        snprintf(LobExtractQuery, sizeof(LobExtractQuery), "EXTRACT LOBLENGTH(LOB'%s') LOCATION %Ld", lobHandle, (Int64)&lobLength);
        break;
    case 1:
        extractData = new BYTE[extractLen + 1];
        if (extractData == NULL)
        {
            exception_->exception_nr = odbc_SQLsrvr_ExtractLob_ParamError_exn_;
            exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_BUFFER_ALLOC_FAILED;
        }

        snprintf(LobExtractQuery, sizeof(LobExtractQuery), "EXTRACT LOBTOBUFFER(LOB'%s', LOCATION %Ld, SIZE %Ld)", lobHandle, (Int64)extractData, &extractLen);
        break;
    case 102:
        extractLen = 0;
        extractData = NULL;
        snprintf(LobExtractQuery, sizeof(LobExtractQuery), "EXTRACT LOBTOBUFFER(LOB'%s', LOCATION %Ld, SIZE %Ld)", lobHandle, (Int64)extractData, &extractLen);
        break;
    default:
        return ;
    }

    try
    {
        short retcode = QryLobExtractSrvrStmt->ExecDirect(NULL, LobExtractQuery, EXTERNAL_STMT, TYPE_CALL, SQL_ASYNC_ENABLE_OFF, 0);

        if (retcode == SQL_ERROR)
        {
            ERROR_DESC_def *p_buffer = QryLobExtractSrvrStmt->sqlError.errorList._buffer;
            char            errNumStr[128] = {0};
            sprintf(errNumStr, "%d", (int)p_buffer->sqlcode);
            strncpy(RequestError, p_buffer->errorText, sizeof(RequestError) - 1);

            SendEventMsg(MSG_SQL_ERROR,
                         EVENTLOG_ERROR_TYPE,
                         srvrGlobal->nskProcessInfo.processId,
                         ODBCMX_SERVER,
                         srvrGlobal->srvrObjRef,
                         2,
                         errNumStr,
                         RequestError);

            exception_->exception_nr = odbc_SQLsrvr_ExtractLob_ParamError_exn_;
            exception_->u.SQLError.errorList._length = QryLobExtractSrvrStmt->sqlError.errorList._length;
            exception_->u.SQLError.errorList._buffer = QryLobExtractSrvrStmt->sqlError.errorList._buffer;
            exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECUTE_FAILED;
        }
    }
    catch (...)
    {
        SendEventMsg(MSG_PROGRAMMING_ERROR,
                EVENTLOG_ERROR_TYPE,
                srvrGlobal->nskProcessInfo.processId,
                ODBCMX_SERVER,
                srvrGlobal->srvrObjRef,
                1,
                    //"Exception in executing EXTRACT LOBTOBUFFER");
                "Exception in executing EXTRACT LOBLENGTH");

        exception_->exception_nr = odbc_SQLsrvr_ExtractLob_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECDIRECT_FAILED;
    }

}

extern "C" void
odbc_SQLSrvr_UpdateLob_sme_(
    /* In   */ CEE_tag_def objtag_
  , /* In   */ const CEE_handle_def * call_id_
  , /* In   */ odbc_SQLSvc_UpdateLob_exc_ * exception_
  , /* In   */ IDL_short lobUpdateType
  , /* In   */ IDL_string lobHandle
  , /* In   */ IDL_long_long totalLength
  , /* In   */ IDL_long_long offset
  , /* In   */ IDL_long_long length
  , /* In   */ BYTE * data)
{
	char lobUpdateQuery[1000] = {0};
	char RequestError[200] = {0};

	SRVR_STMT_HDL * QryLobUpdateSrvrStmt = NULL;

	if ((QryLobUpdateSrvrStmt = getSrvrStmt("MXOSRVR_UPDATELOB", TRUE)) == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR,
			         EVENTLOG_ERROR_TYPE,
			         srvrGlobal->nskProcessInfo.processId,
			         ODBCMX_SERVER,
			         srvrGlobal->srvrObjRef,
			         2,
			         "LOB UPDATE APIs",
			         "Allocate Statement");

		exception_->exception_nr = odbc_SQLSvc_UpdateLob_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
	}

    if (offset == 0)
    {
        snprintf(lobUpdateQuery, sizeof(lobUpdateQuery),  "UPDATE LOB (LOB'%s', LOCATION %Ld, SIZE %Ld)", lobHandle, (Int64)data, length);
    }
    else
    {
        snprintf(lobUpdateQuery, sizeof(lobUpdateQuery),  "UPDATE LOB (LOB'%s', LOCATION %Ld, SIZE %Ld, APPEND)", lobHandle, (Int64)data, length);
    }

    short retcode = 0;
	try
	{
        retcode = QryLobUpdateSrvrStmt->ExecDirect(NULL, lobUpdateQuery, INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);

        if (retcode == SQL_ERROR)
        {
            ERROR_DESC_def * p_buffer = QryLobUpdateSrvrStmt->sqlError.errorList._buffer;
            char             errNumStr[128] = {0};
            sprintf(errNumStr, "%d", (int)p_buffer->sqlcode);

            strncpy(RequestError, p_buffer->errorText, sizeof(RequestError) - 1);

            SendEventMsg(MSG_SQL_ERROR,
                         EVENTLOG_ERROR_TYPE,
                         srvrGlobal->nskProcessInfo.processId,
                         ODBCMX_SERVER,
                         srvrGlobal->srvrObjRef,
                         2,
                         errNumStr,
                         RequestError);
            exception_->exception_nr = odbc_SQLSvc_UpdateLob_ParamError_exn_;
            exception_->u.SQLError.errorList._length = QryLobUpdateSrvrStmt->sqlError.errorList._length;
            exception_->u.SQLError.errorList._buffer = QryLobUpdateSrvrStmt->sqlError.errorList._buffer;
            exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECUTE_FAILED;
        }
    }
    catch (...)
    {
        SendEventMsg(MSG_PROGRAMMING_ERROR,
                     EVENTLOG_ERROR_TYPE,
                     srvrGlobal->nskProcessInfo.processId,
                     ODBCMX_SERVER,
                     srvrGlobal->srvrObjRef,
                     1,
                     "Exception in executing UPDATE_LOB");

        exception_->exception_nr = odbc_SQLSvc_UpdateLob_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECUTE_FAILED;
    }

    if (QryLobUpdateSrvrStmt != NULL) {
        QryLobUpdateSrvrStmt->Close(SQL_DROP);
    }
}

//========================================================================
//LCOV_EXCL_START
short qrysrvc_GetAdaptiveSegment()
{
	static ADAPTIVE_SEGMENT_DATA asd = {CATCHER, ASOPER_ALLOCATE, -1};

	_cc_status cc;
	short error;
	//short pHandle[10];
   TPT_DECL(pHandle);
	unsigned short wcount;
	SB_Tag_Type tag;
	Int32 timeout = AS_TIMEOUT;
	int size;

	if (srvrGlobal->bWMS_AdaptiveSegment == false)
		return -1;


	if ((error = getProcessHandle(srvrGlobal->QSProcessName,
                                  TPT_REF(pHandle))) != 0)
	{
		if (srvrGlobal->fnumAS != -1)
		{
			FILE_CLOSE_(srvrGlobal->fnumAS);
			srvrGlobal->fnumAS = -1;
		}
		return -1;
	}


	if (srvrGlobal->fnumAS == -1 || 2 != PROCESSHANDLE_COMPARE_(TPT_REF(pHandle), TPT_REF(srvrGlobal->pASHandle)))
	{
		if (srvrGlobal->fnumAS != -1)
		{
			FILE_CLOSE_(srvrGlobal->fnumAS);
			srvrGlobal->fnumAS = -1;
		}
		// bits <1> ON - nowait
		short option = 0x4000;

		error = FILE_OPEN_(srvrGlobal->QSProcessName
						, strlen(srvrGlobal->QSProcessName)
						, &srvrGlobal->fnumAS
						, 0			//access
						, 0			//exclusion
						, 1			//nowait_depth
						, 0			//sync-or-receive-depth
						, option	//options
						);
		if (error == 0)
		{
			cc = AWAITIOX(&srvrGlobal->fnumAS,OMITREF,OMITREF,OMITREF,timeout);
			if (_status_lt(cc))
				FILE_GETINFO_ (srvrGlobal->fnumAS, &error);
			else
				error = 0;
		}
		if (error == 0)
		{

			if ((error = getProcessHandle(srvrGlobal->QSProcessName,
                                  TPT_REF(srvrGlobal->pASHandle))) != 0)
					error = 1;
		}
		if (error)
		{
			if (srvrGlobal->fnumAS != -1)	//timeout
				FILE_CLOSE_(srvrGlobal->fnumAS);
			srvrGlobal->fnumAS = -1;
			return -1;
		}
	}

	size = sizeof(asd);
	short fnum = srvrGlobal->fnumAS;

	cc = WRITEREADX( fnum,
		(char*)&asd,
		size,
		size,
		&wcount,
		tag);
	if (_status_lt(cc))
	{
		FILE_CLOSE_(srvrGlobal->fnumAS);
		srvrGlobal->fnumAS = -1;
		return -1;
	}

	cc = AWAITIOX(&fnum,OMITREF,&wcount,&tag,timeout);
	if (_status_lt(cc))
	{
		FILE_CLOSE_(srvrGlobal->fnumAS);
		srvrGlobal->fnumAS = -1;
		return -1;
	}

	return asd.segment;
}

void AllocateAdaptiveSegment(SRVR_STMT_HDL *pSrvrStmt)
{
	if (srvrGlobal->bWMS_AdaptiveSegment == false)
		return;

	if (pSrvrStmt->m_isAdaptiveSegmentAllocated)
	{
		if (pSrvrStmt->m_adaptive_segment != -1)
			ClearAdaptiveSegment(pSrvrStmt->m_adaptive_segment);
	}
	pSrvrStmt->m_isAdaptiveSegmentAllocated = false;

	pSrvrStmt->m_adaptive_segment = qrysrvc_GetAdaptiveSegment();
	if (pSrvrStmt->m_adaptive_segment != -1)
		pSrvrStmt->m_isAdaptiveSegmentAllocated = true;

	if (pSrvrStmt->m_adaptive_segment != srvrGlobal->lastCQDAdaptiveSegment)
	{
		char AffinityCQD[64];
		sprintf(AffinityCQD, "CONTROL QUERY DEFAULT AFFINITY_VALUE '%d'", pSrvrStmt->m_adaptive_segment);
		EXECDIRECT(AffinityCQD);
		srvrGlobal->lastCQDAdaptiveSegment = pSrvrStmt->m_adaptive_segment;
	}
}

void DeallocateAdaptiveSegment(SRVR_STMT_HDL *pSrvrStmt)
{
	if (srvrGlobal->bWMS_AdaptiveSegment == false)
		return;

	if (pSrvrStmt->m_isAdaptiveSegmentAllocated)
	{
		if (pSrvrStmt->m_adaptive_segment != -1)
			ClearAdaptiveSegment(pSrvrStmt->m_adaptive_segment);
		pSrvrStmt->m_isAdaptiveSegmentAllocated = false;
	}
}

void ClearAdaptiveSegment(short adapiveSeg)
{
	static ADAPTIVE_SEGMENT_DATA asd = {CATCHER, ASOPER_INIT, -1};

	_cc_status cc;
	short error;
	//short pHandle[10];
   TPT_DECL(pHandle);
	unsigned short wcount;
	SB_Tag_Type tag;
	Int32 timeout = AS_TIMEOUT;

	if (srvrGlobal->bWMS_AdaptiveSegment == false)
		return;

	if ((error = getProcessHandle(srvrGlobal->QSProcessName,
                                  TPT_REF(pHandle))) != 0)
	{
		if (srvrGlobal->fnumAS != -1)
		{
			FILE_CLOSE_(srvrGlobal->fnumAS);
			srvrGlobal->fnumAS = -1;
		}
		return;
	}

	if (adapiveSeg == -1)
	{
		asd.operation = ASOPER_DEALLOCATE_ALL;
	}
	else
	{
		asd.operation = ASOPER_DEALLOCATE;
		asd.segment = adapiveSeg;
	}

	if (srvrGlobal->fnumAS == -1 || 2 != PROCESSHANDLE_COMPARE_(TPT_REF(pHandle),TPT_REF(srvrGlobal->pASHandle)))
	{
		if (srvrGlobal->fnumAS != -1)
		{
			FILE_CLOSE_(srvrGlobal->fnumAS);
			srvrGlobal->fnumAS = -1;
		}
		// bits <1> ON - nowait
		short option = 0x4000;

		error = FILE_OPEN_(srvrGlobal->QSProcessName
						, strlen(srvrGlobal->QSProcessName)
						, &srvrGlobal->fnumAS
						, 0			//access
						, 0			//exclusion
						, 1			//nowait_depth
						, 0			//sync-or-receive-depth
						, option	//options
						);
		if (error == 0)
		{
			cc = AWAITIOX(&srvrGlobal->fnumAS,OMITREF,OMITREF,OMITREF,timeout);
			if (_status_lt(cc))
				FILE_GETINFO_ (srvrGlobal->fnumAS, &error);
			else
				error = 0;
		}
		if (error == 0)
		{

			if ((error = getProcessHandle(srvrGlobal->QSProcessName,
                                  TPT_REF(srvrGlobal->pASHandle))) != 0)
					error = 1;
		}
		if (error)
		{
			if (srvrGlobal->fnumAS != -1)	//timeout
				FILE_CLOSE_(srvrGlobal->fnumAS);
			srvrGlobal->fnumAS = -1;
		}
	}

	if (srvrGlobal->fnumAS != -1)
	{
		cc = WRITEREADX( srvrGlobal->fnumAS,
			(char*)&asd,
			sizeof(asd),
			sizeof(asd),
			&wcount,
			tag);

		if (_status_lt(cc))
		{
			FILE_CLOSE_(srvrGlobal->fnumAS);
			srvrGlobal->fnumAS = -1;
			return;
		}

		cc = AWAITIOX(&(srvrGlobal->fnumAS),OMITREF,&wcount,&tag,timeout);

		if (_status_lt(cc))
		{
			FILE_CLOSE_(srvrGlobal->fnumAS);
			srvrGlobal->fnumAS = -1;
		}
	}
}
//LCOV_EXCL_STOP
//==========================================================

static void setAuthenticationError(
   bool & bSQLMessageSet, 
   odbc_SQLSvc_SQLError * SQLError,
   const char * externalUsername,
   bool isInternalError)

{

const char authErrorMessageHeader[] = "*** ERROR[8837] Invalid username or password";
const char authInternalErrorMessageHeader[] = "*** ERROR[8837] Internal error occurred";
char  strNow[TIMEBUFSIZE + 1];

   kdsCreateSQLErrorException(bSQLMessageSet,SQLError,1);
   
size_t messageHeaderLength;

   if (isInternalError)
      messageHeaderLength = strlen(authInternalErrorMessageHeader);
   else
      messageHeaderLength = strlen(authErrorMessageHeader);

size_t messageLength = (messageHeaderLength + 1) * 4 + TIMEBUFSIZE;

char *message = new char[messageLength];

   if (isInternalError)
      strcpy(message,authInternalErrorMessageHeader);
   else   
      strcpy(message,authErrorMessageHeader);
      
   strcat(message,".  User: ");
   strcat(message,externalUsername);
   time_t now = time(NULL);
   bzero(strNow,sizeof(strNow));
   strftime(strNow,sizeof(strNow)," [%Y-%m-%d %H:%M:%S]", localtime(&now));
   strcat(message,strNow);

   kdsCopySQLErrorExceptionAndRowCount(SQLError,message,-8837,"    ",-1);
   delete message;

}
