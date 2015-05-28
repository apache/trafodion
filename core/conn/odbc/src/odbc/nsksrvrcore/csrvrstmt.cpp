/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2014 Hewlett-Packard Development Company, L.P.
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
********************************************************************/
//
// MODULE: CSrvrStmt.cpp
//
// PURPOSE: Implements the member functions of CSrvrStmt class
//
//

#include <platform_ndcs.h>
#include "CSrvrStmt.h"
#include "sqlinterface.h"
#include "srvrkds.h"
#include "srvrcommon.h"
#include "tdm_odbcSrvrMsg.h"
#include "CommonDiags.h"
#include "commonFunctions.h"

//extern "C" Int32   random(void);

//Map<int, long> 	SRVR_STMT_HDL::stmtHandleMapping;
IDL_long SRVR_STMT_HDL::globalKey = 0;

using namespace SRVR;

SRVR_STMT_HDL::SRVR_STMT_HDL()
{
	SRVRTRACE_ENTER(FILE_CSTMT+1);
	cursorNameLen = 0;
	cursorName[0] = '\0';
	previousCursorName[0] = '\0';
	stmtNameLen = 0;
	stmtName[0] = '\0';
	paramCount = 0;
	columnCount = 0;
	stmtType = EXTERNAL_STMT;
	inputDescVarBuffer = NULL;
	outputDescVarBuffer = NULL;
	inputDescVarBufferLen = 0;
	outputDescVarBufferLen = 0;
	inputDescIndBufferLen = 0;
	outputDescIndBufferLen = 0;
	SpjProxySyntaxString = NULL;
	SpjProxySyntaxStringLen = 0;

	currentMethod = UNKNOWN_METHOD;
	asyncThread = 0;
	queryTimeoutThread = 0;
	threadStatus = SQL_SUCCESS;
	threadId = 0;
	threadReturnCode = SQL_SUCCESS;

	sqlAsyncEnable = SQL_ASYNC_ENABLE_OFF;
	queryTimeout = 0;
	sqlStringLen = 0;
	sqlString = NULL;
	sqlPlan = NULL;
	sqlPlanLen = 0;
	inputRowCnt = 0;
	maxRowsetSize = ROWSET_NOT_DEFINED;
	maxRowCnt = 0;
	sqlStmtType = TYPE_UNKNOWN;
	sqlQueryType = SQL_UNKNOWN;
	inputDescBufferLength = 0;
	inputDescBuffer = NULL;
	outputDescBufferLength = 0;
	outputDescBuffer = NULL;
	sqlWarningOrErrorLength = 0;
	sqlWarningOrError = NULL;
	delayedSqlWarningOrErrorLength = 0;
	delayedSqlWarningOrError = NULL;
	cliStartTime = 0;
	cliEndTime = 0;
	cliElapseTime = 0;
	sqlBulkFetchPossible = false;
	bFirstSqlBulkFetch = false;
	bLowCost = false;

	sqlUniqueQueryIDLen = 0;
	sqlUniqueQueryID[0] = '\0';
	freeResourceOpt = SQL_CLOSE;
	inputValueList._length = 0;
	inputValueList._buffer = NULL;

	bzero(&cost_info, sizeof(cost_info));
	bzero(&comp_stats_info, sizeof(comp_stats_info));
	rowsAffected = 0;
	delayedRowsAffected = 0;
	rowsAffectedHigherBytes = 0;
	inputDescList._length = 0;
	inputDescList._buffer = NULL;
	outputDescList._length = 0;
	outputDescList._buffer = NULL;
	sqlWarning._length	= 0;
	sqlWarning._buffer = NULL;
	sqlError.errorList._length = 0;
	sqlError.errorList._buffer = NULL;
	outputValueList._length = 0;
	outputValueList._buffer = NULL;
	outputValueVarBuffer = NULL;
	inputValueVarBuffer = NULL;
	clientLCID = srvrGlobal->clientLCID;
	isReadFromModule = FALSE;
	moduleName[0] = '\0';
	inputDescName[0] = '\0';
	outputDescName[0] = '\0';
	isClosed = TRUE;
	IPD = NULL;
	IRD = NULL;
	outputDataValue._length = 0;
	outputDataValue._buffer = NULL;
	outputDataValue.pad_to_offset_8_ = {'\0', '\0', '\0', '\0'};
	inputDataValue._length = 0;
	inputDataValue._buffer = NULL;
	delayedOutputDataValue._length = 0;
	delayedOutputDataValue._buffer = NULL;
	estRowLength = 0;
	bSQLMessageSet = false;
	bSQLValueListSet = false;
	bFetchStarted = false;
	
	SqlDescInfo = NULL;

	GN_SelectParamQuadFields = NULL;
	GN_stmtDrvrHandle = 0;
	GN_maxRowsetSize = 0;
	GN_currentRowsetSize = 0;

    numResultSets  = 0;
    previousSpjRs  = NULL;
    nextSpjRs      = NULL;
    callStmtId     = NULL;
    resultSetIndex = 0;
	callStmtHandle = NULL;

	NA_supported = false;

        preparedWithRowsets = false;  
	inputQuadList       = NULL;
        inputQuadList_recover = NULL;
        transportBuffer     = NULL;
	transportBufferLen  = 0;

        outputQuadList      = NULL;
        outputBuffer        = NULL;
        outputBufferLength  = 0;
	returnCodeForDelayedError = SQL_SUCCESS;

	m_isAdaptiveSegmentAllocated = false;
	m_need_21036_end_msg = false;
	m_adaptive_segment = -1;

	m_internal_Id = 0;
	m_aggPriority = 0;
	m_bSkipWouldLikeToExecute = false;
	m_bDoneWouldLikeToExecute = false;
	m_bqueryFinish = false;

	m_result_set = NULL;
	m_curRowsFetched = 0;
	inState = STMTSTAT_NONE;
	exPlan = UNAVAILABLE;

	current_holdableCursor = SQL_NONHOLDABLE;
	holdableCursor = SQL_NONHOLDABLE;

	sqlNewQueryType = SQL_UNKNOWN;

	bzero(m_con_rule_name, sizeof(m_con_rule_name));
	bzero(m_cmp_rule_name, sizeof(m_cmp_rule_name));
	bzero(m_exe_rule_name, sizeof(m_exe_rule_name));
	//ssd overflow
	memset(&m_execOverflow, '\0', sizeof(EXEC_OVERFLOW));

	m_suspended_ts = 0;
	m_released_ts = 0;
	m_cancelled_ts = 0;

//
// AGGREGATION
//
//
	m_query_rejected = false;
//
//
	m_lastQueryStartTime = 0;
	m_lastQueryEndTime = 0;
	m_lastQueryStartCpuTime = 0;
	m_lastQueryEndCpuTime = 0;
//
	m_mxsrvr_substate = NDCS_INIT;

	m_flushQuery = NULL;

//
	m_bNewQueryId = false;
	bzero(m_shortQueryText, sizeof(m_shortQueryText));
	m_rmsSqlSourceLen = 0;

//
	querySpl = SPEC_QUERY_INIT;
//
	m_pertable_stats = false;
//
	// 64bit work
	globalKey++;
	//globalKey %= 64000;
	myKey = globalKey;	
	srvrGlobal->stmtHandleMap[myKey] = (Long)this;
	//stmtHandleMapping.insert(pair<int, long>(myKey, this));	
	
	reprepareWarn = false;

	//for publishing compiler error to repository
	queryStartTime = 0;

	SRVRTRACE_EXIT(FILE_CSTMT+1);
}

SRVR_STMT_HDL::~SRVR_STMT_HDL()
{
	SRVRTRACE_ENTER(FILE_CSTMT+2);

	if (m_flushQuery != NULL)
		(*m_flushQuery)((intptr_t)this);
	cleanupAll();
	inState = STMTSTAT_NONE;
	
	// Added for 64bit work
	srvrGlobal->stmtHandleMap.erase(myKey);
	
	SRVRTRACE_EXIT(FILE_CSTMT+2);
}

SQLRETURN SRVR_STMT_HDL::Prepare(const char *inSqlString, short inStmtType, short inSqlAsyncEnable, 
								 Int32 inQueryTimeout)
{
	SRVRTRACE_ENTER(FILE_CSTMT+3);

	SQLRETURN rc;
	
	if (isReadFromModule)	// Already SMD label is found
		return SQL_SUCCESS;
	// cleanup all memory allocated in the previous operations
	cleanupAll();
	sqlStringLen = strlen(inSqlString);
	markNewOperator,sqlString  = new char[sqlStringLen+1];
	if (sqlString == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "Prepare");
		exit(0);
	}

	strcpy(sqlString, inSqlString);
	stmtType = inStmtType;
	currentMethod = odbc_SQLSvc_Prepare_ldx_;
	rc = (SQLRETURN)ControlProc(this);

	SRVRTRACE_EXIT(FILE_CSTMT+3);
	return rc;
}

SQLRETURN SRVR_STMT_HDL::Execute(const char *inCursorName, IDL_long inInputRowCnt, IDL_short inSqlStmtType, 
								 const SQLValueList_def *inValueList, 
								 short inSqlAsyncEnable, Int32 inQueryTimeout)
{
	SRVRTRACE_ENTER(FILE_CSTMT+4);
	SQLRETURN rc;
	
	if(bSQLMessageSet)
		cleanupSQLMessage();
	if(bSQLValueListSet)
		cleanupSQLValueList();
	inputRowCnt = inInputRowCnt;
	sqlStmtType = inSqlStmtType;

	if (inCursorName != NULL && inCursorName[0] != '\0')
	{
		cursorNameLen = strlen(inCursorName);
		cursorNameLen = cursorNameLen < sizeof(cursorName)? cursorNameLen : sizeof(cursorName);
		strcpyUTF8(cursorName, inCursorName, sizeof(cursorName));
	}
	else
		cursorName[0] = '\0';


	inputValueList._buffer = inValueList->_buffer;
	inputValueList._length = inValueList->_length;

	currentMethod = odbc_SQLSvc_ExecuteN_ldx_;
//	rc = (SQLRETURN)ControlProc(this);

	rc = EXECUTE(this);

	switch (rc)
	{
	case SQL_SUCCESS:
		break;
	case ODBC_RG_WARNING:
		rc = SQL_SUCCESS_WITH_INFO;
	case SQL_SUCCESS_WITH_INFO:
		GETSQLERROR(bSQLMessageSet, &sqlError);
		break;
	case SQL_ERROR:
		GETSQLERROR(bSQLMessageSet, &sqlError);
		break;
	case ODBC_SERVER_ERROR:
		// Allocate Error Desc
		kdsCreateSQLErrorException(bSQLMessageSet, &sqlError, 1);
		// Add SQL Error
		kdsCopySQLErrorException(&sqlError, NULL_VALUE_ERROR, NULL_VALUE_ERROR_SQLCODE,
				NULL_VALUE_ERROR_SQLSTATE);
		break;
	case -8814:
	case 8814:
		rc = SQL_RETRY_COMPILE_AGAIN;
		break;
	default:
		break;
	}
	SRVRTRACE_EXIT(FILE_CSTMT+4);
	return rc;
}

SQLRETURN SRVR_STMT_HDL::Close(unsigned short inFreeResourceOpt)
{
	SRVRTRACE_ENTER(FILE_CSTMT+5);

	SQLRETURN rc;

	if (bSQLMessageSet)
		cleanupSQLMessage();
	if(bSQLValueListSet)
		cleanupSQLValueList();
	freeResourceOpt = inFreeResourceOpt;
	currentMethod = odbc_SQLSvc_Close_ldx_;
	rc = (SQLRETURN)ControlProc(this);
	SRVRTRACE_EXIT(FILE_CSTMT+5);
	return rc;
}

SQLRETURN SRVR_STMT_HDL::InternalStmtClose(unsigned short inFreeResourceOpt)
{
	SRVRTRACE_ENTER(FILE_CSTMT+6);

	SQLRETURN rc;

	if (bSQLMessageSet)
		cleanupSQLMessage();
	if(bSQLValueListSet)
		cleanupSQLValueList();
	freeResourceOpt = inFreeResourceOpt;
	currentMethod = odbc_SQLSvc_Close_ldx_;
	rc = (SQLRETURN)ControlProc(this);
	SRVRTRACE_EXIT(FILE_CSTMT+6);
	return rc;
}

SQLRETURN SRVR_STMT_HDL::Fetch(Int32 inMaxRowCnt, Int32 inMaxRowLen, short inSqlAsyncEnable, Int32 inQueryTimeout)
{
	SRVRTRACE_ENTER(FILE_CSTMT+7);

	SQLRETURN rc;

	if (bSQLMessageSet)
		cleanupSQLMessage();
	if (outputValueList._buffer == NULL  || maxRowCnt < inMaxRowCnt)
	{
		if(bSQLValueListSet)
			cleanupSQLValueList();
		rc = AllocAssignValueBuffer(bSQLValueListSet,&outputDescList, &outputValueList, outputDescVarBufferLen,
						inMaxRowCnt, outputValueVarBuffer);
		if (rc != SQL_SUCCESS)
			return rc;
	}
	else
		// Reset the length to 0, but the _buffer points to array of required SQLValue_defs
		outputValueList._length = 0;

	maxRowCnt = inMaxRowCnt;
	maxRowLen = inMaxRowLen;
	currentMethod = odbc_SQLSvc_FetchN_ldx_;
	rc = (SQLRETURN)ControlProc(this);
	SRVRTRACE_EXIT(FILE_CSTMT+7);
	return rc;
}

SQLRETURN SRVR_STMT_HDL::ExecDirect(const char *inCursorName, const char *inSqlString, short inStmtType, IDL_short inSqlStmtType, 
									short inSqlAsyncEnable, Int32 inQueryTimeout)
{
	SRVRTRACE_ENTER(FILE_CSTMT+8);

	SQLRETURN rc;

	cleanupAll();
	sqlStringLen = strlen(inSqlString);
	markNewOperator,sqlString  = new char[sqlStringLen+1];
	if (sqlString == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "ExecDirect");
		exit(0);
	}

	strcpy(sqlString, inSqlString);
	stmtType = inStmtType;
	sqlStmtType = inSqlStmtType;

	if (inCursorName != NULL && inCursorName[0] != '\0')
	{
		cursorNameLen = strlen(inCursorName);
		cursorNameLen = cursorNameLen < sizeof(cursorName)? cursorNameLen : sizeof(cursorName);
		strcpyUTF8(cursorName, inCursorName, sizeof(cursorName));
	}
	else
		cursorName[0] = '\0';

	currentMethod = odbc_SQLSvc_ExecDirect_ldx_;
	rc = (SQLRETURN)ControlProc(this);
	switch (rc)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		break;
	default:
		break;
	}
	SRVRTRACE_EXIT(FILE_CSTMT+8);
	return rc;
}

void  SRVR_STMT_HDL::cleanupSQLMessage()
{
	SRVRTRACE_ENTER(FILE_CSTMT+10);

	UInt32 i;
	ERROR_DESC_def *errorDesc;
	// Cleanup SQLWarning
	bSQLMessageSet = false;
	if (sqlWarning._buffer)
	{
		int len_length = sqlWarning._length;
		ERROR_DESC_def *p_buffer = (ERROR_DESC_def *)sqlWarning._buffer;

		for (i = 0 ; i < len_length ; i++)
		{

			errorDesc = p_buffer + i;
			if (errorDesc->errorText != NULL)
			{
				delete errorDesc->errorText;

			}
		}
		delete sqlWarning._buffer;
		sqlWarning._buffer = NULL;
		sqlWarning._length = 0;
	}
	
	// Cleanup sqlErrror
	if (sqlError.errorList._buffer != NULL)
	{
		int len_length = sqlError.errorList._length;
		ERROR_DESC_def *p_buffer = (ERROR_DESC_def *)sqlError.errorList._buffer;

		for (i = 0 ; i < len_length ; i++)
		{

			errorDesc = p_buffer + i;
			if (errorDesc->errorText != NULL)
			{
				delete errorDesc->errorText;
			}

			if (errorDesc->Param1 != NULL)
			{
				delete errorDesc->Param1;
			}

			if (errorDesc->Param2 != NULL)
			{
				delete errorDesc->Param2;
			}

			if (errorDesc->Param3 != NULL)
			{
				delete errorDesc->Param3;
			}

			if (errorDesc->Param4 != NULL)
			{
				delete errorDesc->Param4;
			}

			if (errorDesc->Param5 != NULL)
			{
				delete errorDesc->Param5;
			}

			if (errorDesc->Param6 != NULL)
			{
				delete errorDesc->Param6;
			}

			if (errorDesc->Param7 != NULL)
			{
				delete errorDesc->Param7;
			}

		}
		delete sqlError.errorList._buffer;
		sqlError.errorList._buffer = NULL;
		sqlError.errorList._length = 0;
	}

	// Cleanup sqlErrorOrWarnings
	if (sqlWarningOrError != NULL)
		delete sqlWarningOrError;
	sqlWarningOrError = NULL;
	sqlWarningOrErrorLength = 0;


	SRVRTRACE_EXIT(FILE_CSTMT+10);
	return;
}

void  SRVR_STMT_HDL::cleanupSQLValueList()
{
	SRVRTRACE_ENTER(FILE_CSTMT+11);

	bSQLValueListSet = false;

	if (outputValueList._buffer != NULL)
	{
		delete outputValueList._buffer;
		outputValueList._buffer = NULL;
		outputValueList._length = 0;
	}
	if (outputValueVarBuffer != NULL)
	{
		delete outputValueVarBuffer;
		outputValueVarBuffer = NULL;
	}
	maxRowCnt = 0;
	SRVRTRACE_EXIT(FILE_CSTMT+11);
	return;
}

void  SRVR_STMT_HDL::cleanupSQLDescList()
{
	SRVRTRACE_ENTER(FILE_CSTMT+12);

	if (inputDescList._buffer != NULL)
	{
		delete inputDescList._buffer;
		inputDescList._length = 0;
		inputDescList._buffer = NULL;
	}
	if (inputDescVarBuffer != NULL)
	{
		delete inputDescVarBuffer;
		inputDescVarBuffer = NULL;
		inputDescVarBufferLen = 0;
	}

	if (outputDescList._buffer != NULL)
	{
		delete outputDescList._buffer;
		outputDescList._length = 0;
		outputDescList._buffer = 0;
	}
	if (outputDescVarBuffer != NULL)
	{
		delete outputDescVarBuffer;
		outputDescVarBuffer = NULL;
		outputDescVarBufferLen = 0;
	}
	if (inputDescBuffer != NULL)
	{
		delete inputDescBuffer;
		inputDescBuffer = NULL;
		inputDescBufferLength = 0;
	}
	if (outputDescBuffer != NULL)
	{
		delete outputDescBuffer;
		outputDescBuffer = NULL;
		outputDescBufferLength = 0;
	}
	if (SqlDescInfo != NULL)
	{
		delete SqlDescInfo;
		SqlDescInfo = NULL;	
	}
	

	SRVRTRACE_EXIT(FILE_CSTMT+12);
	return;
}

void  SRVR_STMT_HDL::cleanupAll()
{
	SRVRTRACE_ENTER(FILE_CSTMT+13);

	if (sqlString != NULL)
	{
		delete[] sqlString;
		sqlString = NULL;
	}
	if (sqlPlan != NULL)
	{
		delete sqlPlan;
		sqlPlan = NULL;
		sqlPlanLen = 0;
	}

	if (bSQLMessageSet)
		cleanupSQLMessage();
	cleanupSQLDescList();
	if(bSQLValueListSet)
		cleanupSQLValueList();
	if (stmtType != EXTERNAL_STMT && inputValueList._buffer != NULL)
	{
		delete inputValueList._buffer;
		inputValueList._buffer = NULL;
		inputValueList._length = 0;
	}
	if (inputValueVarBuffer != NULL)
	{
		delete inputValueVarBuffer;
		inputValueVarBuffer = NULL;
	}
	if (IPD != NULL)
	{
		delete IPD;
		IPD = NULL;
	}
	if (IRD != NULL)
	{
		delete IRD;
		IRD = NULL;
	}
	if (SqlDescInfo != NULL)
	{
		delete SqlDescInfo;
		SqlDescInfo = NULL;	
	}
	
	if (GN_SelectParamQuadFields != NULL)
	{
		delete[] GN_SelectParamQuadFields;
		GN_SelectParamQuadFields = NULL;
	}
        if (inputQuadList != NULL)
          {
	  delete[] inputQuadList;
          inputQuadList = NULL;
	  }
        if (inputQuadList_recover != NULL)
          {
	  delete[] inputQuadList_recover;
          inputQuadList_recover = NULL;
	  }
	if (sqlWarningOrErrorLength > 0)
	  sqlWarningOrErrorLength = 0;
	if (sqlWarningOrError != NULL)
	  {
	  delete sqlWarningOrError;
	  sqlWarningOrError = NULL;
	  }
        if (outputQuadList != NULL)
          {
	  delete[] outputQuadList;
          outputQuadList = NULL;
	  }

	exPlan = UNAVAILABLE;
	if(SpjProxySyntaxString != NULL)
		delete[] SpjProxySyntaxString;
	SpjProxySyntaxString = NULL;
	SpjProxySyntaxStringLen = 0;

	current_holdableCursor = SQL_NONHOLDABLE;
	holdableCursor = SQL_NONHOLDABLE;

	SRVRTRACE_EXIT(FILE_CSTMT+13);
	return;
}

Int32 SRVR_STMT_HDL::GetCharset()
{
	SRVRTRACE_ENTER(FILE_CSTMT+14);

	Int32 rcharset = SQLCHARSETCODE_ISO88591;

	if (clientLCID != SQLCHARSETCODE_UNKNOWN)
		return clientLCID;

	SRVRTRACE_EXIT(FILE_CSTMT+14);
	return rcharset;
}

DWORD WINAPI SRVR::ControlProc(LPVOID pParam)
{
	SRVRTRACE_ENTER(FILE_CSTMT+15);

	SQLRETURN rc = SQL_SUCCESS;
	SRVR_STMT_HDL *pSrvrStmt;

	pSrvrStmt = (SRVR_STMT_HDL *)pParam;
	unsigned short freeResourceOpt;

	switch (pSrvrStmt->currentMethod)
	{
	case odbc_SQLSvc_Prepare_ldx_:
		rc = PREPARE (pSrvrStmt);
		break;
	case odbc_SQLSvc_ExecuteN_ldx_:
		rc = EXECUTE(pSrvrStmt);
		break;
	case odbc_SQLSvc_Close_ldx_:
		// Copy freeresourceOpt since pSrvrStmt is deleted when it is drop
		// In this case pSrvrStmt becomes invalid.
		freeResourceOpt = pSrvrStmt->freeResourceOpt;
		rc = FREESTATEMENT(pSrvrStmt);
		// Return back immediately since the pSrvrStmt is deleted and return SQL_SUCCESS always
		if (freeResourceOpt == SQL_DROP)
		{
			rc = SQL_SUCCESS;
			return (rc);
		}
		break;
	case odbc_SQLSvc_FetchN_ldx_:
		rc = FETCH(pSrvrStmt);
		break;
	case odbc_SQLSvc_ExecDirect_ldx_:
		rc = EXECDIRECT(pSrvrStmt);
		break;
	case odbc_SQLSvc_FetchPerf_ldx_:
		rc = FETCHPERF(pSrvrStmt, &pSrvrStmt->outputDataValue);
		break;
	case Allocate_Statement_Handle:
		rc = ALLOCSQLMXHDLS(pSrvrStmt);
		break;
	case odbc_SQLSvc_GetSQLCatalogs_ldx_:
		rc = PREPARE_FROM_MODULE(pSrvrStmt, &pSrvrStmt->inputDescList, &pSrvrStmt->outputDescList);
		break;
	case odbc_SQLSvc_ExecuteCall_ldx_:
		rc = EXECUTECALL(pSrvrStmt);
		break;
	case Fetch_Catalog_Rowset:
		rc = FETCHCATALOGPERF(pSrvrStmt, pSrvrStmt->maxRowCnt, pSrvrStmt->maxRowLen, &pSrvrStmt->rowsAffected, &pSrvrStmt->outputDataValue);
		break;
	default:
		break;
	}

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
		rc = SQL_SUCCESS_WITH_INFO;
	case ODBC_SERVER_ERROR:
	case ODBC_RG_ERROR:
	default:
		break;
	}
	pSrvrStmt->threadReturnCode = rc;
	SRVRTRACE_EXIT(FILE_CSTMT+15);
	return rc;
}

//======================= Performance ==========================================

SQLRETURN SRVR_STMT_HDL::FetchPerf(Int32 inMaxRowCnt, Int32 inMaxRowLen, short inSqlAsyncEnable, Int32 inQueryTimeout)
{
	SRVRTRACE_ENTER(FILE_CSTMT+16);

	SQLRETURN rc;

	if (bSQLMessageSet)
		cleanupSQLMessage();

	outputDataValue._length = 0;
	outputDataValue._buffer = 0;

	maxRowCnt = inMaxRowCnt;
	maxRowLen = inMaxRowLen;
	currentMethod = odbc_SQLSvc_FetchPerf_ldx_;
	rc = (SQLRETURN)ControlProc(this);

	switch (rc)
	{
	case SQL_ERROR:
		break;
	default:
		break;
	}
	SRVRTRACE_EXIT(FILE_CSTMT+16);
	return rc;
}

//========================== Rowset =================================================

SQLRETURN SRVR_STMT_HDL::ExecuteRowset(const char *inCursorName, IDL_long inInputRowCnt, IDL_short inSqlStmtType, 
							const SQL_DataValue_def *inDataValue, 
							short inSqlAsyncEnable, Int32 inQueryTimeout)
{
	SRVRTRACE_ENTER(FILE_CSTMT+20);

	SQLRETURN rc;
	
	if (bSQLMessageSet)
		cleanupSQLMessage();
	if(bSQLValueListSet)
		cleanupSQLValueList();
	inputRowCnt = inInputRowCnt;
	sqlStmtType = inSqlStmtType;

	if (inCursorName != NULL && inCursorName[0] != '\0')
	{
		cursorNameLen = strlen(inCursorName);
		cursorNameLen = cursorNameLen < sizeof(cursorName)? cursorNameLen : sizeof(cursorName);
		strcpyUTF8(cursorName, inCursorName, sizeof(cursorName));
	}
	else
		cursorName[0] = '\0';

	inputDataValue._buffer = inDataValue->_buffer;
	inputDataValue._length = inDataValue->_length;

	currentMethod = odbc_SQLSvc_ExecuteRowset_ldx_;
	rc = (SQLRETURN)ControlProc(this);

	switch (rc)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		break;
	case ODBC_SERVER_ERROR:
		// Allocate Error Desc
		kdsCreateSQLErrorException(bSQLMessageSet, &sqlError, 1);
		// Add SQL Error
		kdsCopySQLErrorException(&sqlError, NULL_VALUE_ERROR, NULL_VALUE_ERROR_SQLCODE,
				NULL_VALUE_ERROR_SQLSTATE);
		break;
	case -8814:
	case 8814:
		rc = SQL_RETRY_COMPILE_AGAIN;
		break;
	default:
		break;
	}
	SRVRTRACE_EXIT(FILE_CSTMT+20);
	return rc;
}


SQLRETURN SRVR_STMT_HDL::PrepareFromModule(short inStmtType)
{
	SRVRTRACE_ENTER(FILE_CSTMT+21);

	SQLRETURN rc;
	size_t	len;
	
	if (isReadFromModule)
		return SQL_SUCCESS;
	// cleanup all memory allocated in the previous operations
	cleanupAll();
	stmtType = inStmtType;
	cost_info.totalTime = -1;
	currentMethod = odbc_SQLSvc_GetSQLCatalogs_ldx_;
	rc = (SQLRETURN)ControlProc(this);
	if (rc != SQL_ERROR)
		isReadFromModule = TRUE;
	SRVRTRACE_EXIT(FILE_CSTMT+21);
	return rc;
}

SQLRETURN SRVR_STMT_HDL::freeBuffers(short descType)
{
	SRVRTRACE_ENTER(FILE_CSTMT+22);
	
	SQLRETURN rc = SQL_SUCCESS;

	switch (descType)
	{
	case SQLWHAT_INPUT_DESC:
		if (inputDescVarBuffer != NULL)
		{
			delete inputDescVarBuffer;
			if( trace_memory ) LogDelete("delete VarBuffer;",(void**)&inputDescVarBuffer,inputDescVarBuffer);
		}
		inputDescVarBuffer = NULL;
		inputDescVarBufferLen = 0;
		paramCount = 0;
		break;
	case SQLWHAT_OUTPUT_DESC:
		if (outputDescVarBuffer != NULL)
		{
			delete outputDescVarBuffer;
			if( trace_memory ) LogDelete("delete VarBuffer;",(void**)&outputDescVarBuffer,outputDescVarBuffer);
		}
		outputDescVarBuffer = NULL;
		outputDescVarBufferLen = 0;
		columnCount = 0;
		break;
	default:
		rc = SQL_ERROR;
	}
	SRVRTRACE_EXIT(FILE_CSTMT+22);
	return rc;
}

SQLRETURN SRVR_STMT_HDL::allocSqlmxHdls(const char *inStmtName, const char *inModuleName,
			int64 inModuleTimestamp, Int32 inModuleVersion, const char *inInputDescName, 
				const char *inOutputDescName, short inSqlStmtType)
{
	SRVRTRACE_ENTER(FILE_CSTMT+23);

	SQLRETURN rc;

	stmtNameLen = strlen(inStmtName);
	stmtNameLen = stmtNameLen < sizeof(stmtName)? stmtNameLen : sizeof(stmtName);
	strcpyUTF8(stmtName, inStmtName, sizeof(stmtName));

	if (inModuleName != NULL)
	{
		moduleId.version = inModuleVersion;
		strncpy(moduleName, inModuleName, sizeof(moduleName));
		moduleName[sizeof(moduleName)-1] = 0;
		moduleId.module_name = moduleName;
		moduleId.module_name_len = strlen(moduleName);
		moduleId.charset = "ISO88591";
		moduleId.creation_timestamp = inModuleTimestamp;
	}
	else
	{
		moduleId.version = SQLCLI_ODBC_MODULE_VERSION;
		moduleId.module_name = NULL;
		moduleId.module_name_len = 0;
		moduleId.charset = "ISO88591";
		moduleId.creation_timestamp = 0;
	}
	if (inInputDescName != NULL)
		strcpyUTF8(inputDescName, inInputDescName, sizeof(inputDescName));
	if (inOutputDescName != NULL)
		strcpyUTF8(outputDescName, inOutputDescName, sizeof(outputDescName));

	sqlStmtType = inSqlStmtType;
	currentMethod = 0;
	rc = (SQLRETURN)ControlProc(this);
	SRVRTRACE_EXIT(FILE_CSTMT+23);
	return rc;
}

SQLRETURN SRVR_STMT_HDL::ExecuteCall(const SQLValueList_def *inValueList,short inSqlAsyncEnable, 
		Int32 inQueryTimeout)
{
	SRVRTRACE_ENTER(FILE_CSTMT+24);

	SQLRETURN rc;

	if (bSQLMessageSet)
		cleanupSQLMessage();
	if(bSQLValueListSet)
		cleanupSQLValueList();
	inputValueList._buffer = inValueList->_buffer;
	inputValueList._length = inValueList->_length;

	if (outputValueList._buffer == NULL)
	{
		if ((rc = AllocAssignValueBuffer(bSQLValueListSet, &outputDescList, &outputValueList, outputDescVarBufferLen,
			1, outputValueVarBuffer)) != SQL_SUCCESS)
			return rc;
	}
	else
		outputValueList._length = 0;

	currentMethod = odbc_SQLSvc_ExecuteCall_ldx_;
	rc = (SQLRETURN)ControlProc(this);

	switch (rc)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		break;
	case ODBC_SERVER_ERROR:
		// Allocate Error Desc
		kdsCreateSQLErrorException(bSQLMessageSet, &sqlError, 1);
		// Add SQL Error
		kdsCopySQLErrorException(&sqlError, NULL_VALUE_ERROR, NULL_VALUE_ERROR_SQLCODE,
				NULL_VALUE_ERROR_SQLSTATE);
		break;
	case -8814:
	case 8814:
		rc = SQL_RETRY_COMPILE_AGAIN;
		break;
	default:
		break;
	}
	SRVRTRACE_EXIT(FILE_CSTMT+24);
	return rc;
}

SQLRETURN SRVR_STMT_HDL::FetchCatalogRowset(Int32 inMaxRowCnt, Int32 inMaxRowLen, short inSqlAsyncEnable, Int32 inQueryTimeout, bool resetValues)
{
	SRVRTRACE_ENTER(FILE_CSTMT+25);

	SQLRETURN rc;

	if (resetValues)
	{
		if (bSQLMessageSet)
			cleanupSQLMessage();
		if (outputDataValue._buffer != NULL)
			delete outputDataValue._buffer;
		outputDataValue._length = 0;
		outputDataValue._buffer = 0;
		rowsAffected = 0;
	}

	maxRowCnt = inMaxRowCnt;
	maxRowLen = inMaxRowLen;
	currentMethod = Fetch_Catalog_Rowset;
	rc = (SQLRETURN)ControlProc(this);

	switch (rc)
	{
	case SQL_ERROR:
		break;
	default:
		break;
	}
	SRVRTRACE_EXIT(FILE_CSTMT+25);
	return rc;
}

void  SRVR_STMT_HDL::setclientLCID(UInt32 value)
{
	SRVRTRACE_ENTER(FILE_CSTMT+26);
	clientLCID = value;
	SRVRTRACE_EXIT(FILE_CSTMT+26);
	return;
}



