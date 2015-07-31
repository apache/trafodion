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

#include <windows.h>
#include <cee.h>
#include <idltype.h>
#include "transport.h"
#include "inoutparams.h"
#include "marshalingdrvr_drvr.h"
#include "swapdrvr_drvr.h"

#ifndef unixcli
void PROCESS_swap(char* buffer, int odbcAPI)
{
	switch (odbcAPI)
	{
	case AS_API_GETOBJREF:
		GETOBJREF_swap(buffer);
		break;
	case AS_API_UPDATESRVRSTATE:
		UPDATESRVRSTATE_swap(buffer);
		break;
	case AS_API_STOPSRVR:
		STOPSRVR_swap(buffer);
		break;
	case SRVR_API_SQLCONNECT:
		SQLCONNECT_swap(buffer);
		break;
	case SRVR_API_SQLDISCONNECT:
		SQLDISCONNECT_swap(buffer);
		break;
	case SRVR_API_SQLSETCONNECTATTR:
		SQLSETCONNECTATTR_swap(buffer);
		break;
	case SRVR_API_SQLENDTRAN:
		SQLENDTRAN_swap(buffer);
		break;
	case SRVR_API_SQLPREPARE:
		SQLPREPARE_swap(buffer);
		break;
	case SRVR_API_SQLPREPARE_ROWSET:
		SQLPREPAREROWSET_swap(buffer);
		break;
	case SRVR_API_SQLEXECDIRECT_ROWSET:
		SQLEXECDIRECTROWSET_swap(buffer);
		break;
	case SRVR_API_SQLEXECUTE_ROWSET:
		SQLEXECUTEROWSET_swap(buffer);
		break;
	case SRVR_API_SQLFETCH_ROWSET:
		SQLFETCHROWSET_swap(buffer);
		break;
	case SRVR_API_SQLEXECDIRECT:
		SQLEXECDIRECT_swap(buffer);
		break;
	case SRVR_API_SQLEXECUTE:
		SQLEXECUTE_swap(buffer);
		break;
	case SRVR_API_SQLFETCH_PERF:
		SQLFETCHPERF_swap(buffer);
		break;
	case SRVR_API_SQLFREESTMT:
		SQLFREESTMT_swap(buffer);
		break;
	case SRVR_API_GETCATALOGS:
		SQLGETCATALOGS_swap(buffer);
		break;
	case SRVR_API_SQLEXECUTECALL:
		SQLEXECUTECALL_swap(buffer);
		break;
	default:
		break;
	}
}

void GETOBJREF_swap(char* buffer)
{
	short number_of_param = GetObjRefHdl_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;

	pointers += number_of_param;

	CONNECTION_CONTEXT_def *inContext; 
	USER_DESC_def *userDesc;
	IDL_long *srvrType;
	IDL_short *retryCount;

	inContext = (CONNECTION_CONTEXT_def *)(param[0] + (long)buffer); 
	userDesc = (USER_DESC_def *)(param[1] + (long)buffer);
	srvrType = (IDL_long *)(param[2] + (long)buffer);
	retryCount = (IDL_short *)(param[3] + (long)buffer);
//
// swap CONNECTION_CONTEXT_def
//
	SHORT_swap(&inContext->accessMode);
	SHORT_swap(&inContext->autoCommit);
	SHORT_swap(&inContext->queryTimeoutSec);
	SHORT_swap(&inContext->idleTimeoutSec);
	SHORT_swap(&inContext->loginTimeoutSec);
	SHORT_swap(&inContext->txnIsolationLevel);
	SHORT_swap(&inContext->rowSetSize);
	LONG_swap(&inContext->diagnosticFlag);
	ULONG_swap(&inContext->processId);
	ULONG_swap(&inContext->ctxACP);
	ULONG_swap(&inContext->ctxDataLang);
	ULONG_swap(&inContext->ctxErrorLang);
	SHORT_swap(&inContext->ctxCtrlInferNCHAR);
	VERSION_LIST_swap(buffer, &inContext->clientVersionList);
//
// swap userDesc
//
	LONG_swap(&userDesc->userDescType);
	ULONG_swap(&userDesc->userSid._length);
	ULONG_swap(&userDesc->password._length);
//
// swap srvrType
//
	LONG_swap(srvrType);
//
// swap retryCount
//
	SHORT_swap(retryCount);

	swapPointers(buffer, number_of_param);
	
}


void UPDATESRVRSTATE_swap(char* buffer)
{
	short number_of_param = UpdateSrvrState_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;

	pointers += number_of_param;

	IDL_long *srvrType; 
//	IDL_char *srvrObjRef;
	IDL_long *srvrState;

	srvrType = (IDL_long *)(param[0] + (long)buffer);
	srvrState = (IDL_long *)(param[2] + (long)buffer);
//
// swap srvrType
//
	LONG_swap(srvrType);
//
// swap srvrState
//
	LONG_swap(srvrState);

	swapPointers(buffer, number_of_param);
	
}

void STOPSRVR_swap(char* buffer)
{
	short number_of_param = StopSrvr_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;

	pointers += number_of_param;

	DIALOGUE_ID_def *dialogueId;
	IDL_long *srvrType; 
//	IDL_char *srvrObjRef;
	IDL_long *StopType;

	dialogueId = (IDL_long *)(param[0] + (long)buffer);
	srvrType = (IDL_long *)(param[1] + (long)buffer);
	StopType = (IDL_long *)(param[3] + (long)buffer);
//
// swap dialogueId
//
	LONG_swap(dialogueId);
//
// swap srvrType
//
	LONG_swap(srvrType);
//
// swap StopType
//
	LONG_swap(StopType);

	swapPointers(buffer, number_of_param);
}

void SQLCONNECT_swap(char* buffer)
{
	short number_of_param = InitializeDialogue_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;

	pointers += number_of_param;

	USER_DESC_def *userDesc; 
	CONNECTION_CONTEXT_def *inContext;
	DIALOGUE_ID_def *dialogueId;

	userDesc = (USER_DESC_def *)(param[0] + (long)buffer);
	inContext = (CONNECTION_CONTEXT_def *)(param[1] + (long)buffer); 
	dialogueId = (IDL_long *)(param[2] + (long)buffer);
//
// swap userDesc
//
	LONG_swap(&userDesc->userDescType);
	ULONG_swap(&userDesc->userSid._length);
	ULONG_swap(&userDesc->password._length);
//
// swap CONNECTION_CONTEXT_def
//
	SHORT_swap(&inContext->accessMode);
	SHORT_swap(&inContext->autoCommit);
	SHORT_swap(&inContext->queryTimeoutSec);
	SHORT_swap(&inContext->idleTimeoutSec);
	SHORT_swap(&inContext->loginTimeoutSec);
	SHORT_swap(&inContext->txnIsolationLevel);
	SHORT_swap(&inContext->rowSetSize);
	LONG_swap(&inContext->diagnosticFlag);
	ULONG_swap(&inContext->processId);
	ULONG_swap(&inContext->ctxACP);
	ULONG_swap(&inContext->ctxDataLang);
	ULONG_swap(&inContext->ctxErrorLang);
	SHORT_swap(&inContext->ctxCtrlInferNCHAR);
	VERSION_LIST_swap(buffer, &inContext->clientVersionList);
//
// swap DIALOGUE_ID_def
//
	LONG_swap(dialogueId);

	swapPointers(buffer, number_of_param);
}

void SQLDISCONNECT_swap(char* buffer)
{
	short number_of_param = TerminateDialogue_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;

	pointers += number_of_param;

	DIALOGUE_ID_def *dialogueId;

	dialogueId = (IDL_long *)(param[0] + (long)buffer);
//
// swap DIALOGUE_ID_def
//
	LONG_swap(dialogueId);

	swapPointers(buffer, number_of_param);
}

void SQLSETCONNECTATTR_swap(char* buffer)
{
	short number_of_param = SetConnectionOption_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;

	pointers += number_of_param;

	DIALOGUE_ID_def *dialogueId;
	IDL_short *connectionOption;
	IDL_long *optionValueNum;
//	IDL_string optionValueStr;

	dialogueId = (IDL_long *)(param[0] + (long)buffer);
	connectionOption = (IDL_short *)(param[1] + (long)buffer);;
	optionValueNum = (IDL_long *)(param[2] + (long)buffer);;
//
// swap
//
	LONG_swap(dialogueId);
	SHORT_swap(connectionOption);
	LONG_swap(optionValueNum);

	swapPointers(buffer, number_of_param);
}


void SQLPREPARE_swap(char* buffer)
{
	short number_of_param = Prepare_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;
	pointers += number_of_param;

	DIALOGUE_ID_def *dialogueId;
//	IDL_char *stmtLabel;
//	IDL_char *stmtExplainLabel;
	IDL_short *stmtType;
//	IDL_string sqlString;
	IDL_short *sqlAsyncEnable;
	IDL_long *queryTimeout;

	dialogueId = (IDL_long *)(param[0] + (long)buffer);
	stmtType = (IDL_short *)(param[3] + (long)buffer);;
	sqlAsyncEnable = (IDL_short *)(param[5] + (long)buffer);;
	queryTimeout = (IDL_long *)(param[6] + (long)buffer);;
//
// swap
//
	LONG_swap(dialogueId);
	SHORT_swap(stmtType);
	SHORT_swap(sqlAsyncEnable);
	LONG_swap(queryTimeout);

	swapPointers(buffer, number_of_param);
}

void SQLPREPAREROWSET_swap(char* buffer)
{
	short number_of_param = PrepareRowset_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;

	pointers += number_of_param;

	DIALOGUE_ID_def *dialogueId;
//	IDL_char *stmtLabel;
//	IDL_char *stmtExplainLabel;
	IDL_short *stmtType;
	IDL_short *sqlStmtType;
//	IDL_string sqlString;
	IDL_short *sqlAsyncEnable;
	IDL_long *queryTimeout;
	IDL_long *maxRowsetSize;

	dialogueId = (IDL_long *)(param[0] + (long)buffer);
	stmtType = (IDL_short *)(param[3] + (long)buffer);;
	sqlStmtType = (IDL_short *)(param[4] + (long)buffer);;
	sqlAsyncEnable = (IDL_short *)(param[6] + (long)buffer);;
	queryTimeout = (IDL_long *)(param[7] + (long)buffer);;
	maxRowsetSize = (IDL_long *)(param[8] + (long)buffer);;
//
// swap
//
	LONG_swap(dialogueId);
	SHORT_swap(stmtType);
	SHORT_swap(sqlStmtType);
	SHORT_swap(sqlAsyncEnable);
	LONG_swap(queryTimeout);
	LONG_swap(maxRowsetSize);

	swapPointers(buffer, number_of_param);
}

void SQLEXECDIRECTROWSET_swap(char* buffer)
{
	short number_of_param = ExecDirectRowset_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;

	pointers += number_of_param;

	DIALOGUE_ID_def *dialogueId;
//	IDL_char *stmtLabel;
//	IDL_string cursorName;
//	IDL_char *stmtExplainLabel;
	IDL_short* stmtType;
	IDL_short *sqlStmtType;
//	IDL_string sqlString;
	IDL_short *sqlAsyncEnable;
	IDL_long *queryTimeout;
	IDL_long *maxRowsetSize;

	dialogueId = (IDL_long *)(param[0] + (long)buffer);
	stmtType = (IDL_short *)(param[4] + (long)buffer);
	sqlStmtType = (IDL_short *)(param[5] + (long)buffer);
	sqlAsyncEnable = (IDL_short *)(param[7] + (long)buffer);
	queryTimeout = (IDL_long *)(param[8] + (long)buffer);
	maxRowsetSize = (IDL_long *)(param[9] + (long)buffer);
//
// swap
//
	LONG_swap(dialogueId);
	SHORT_swap(stmtType);
	SHORT_swap(sqlStmtType);
	SHORT_swap(sqlAsyncEnable);
	LONG_swap(queryTimeout);
	LONG_swap(maxRowsetSize);

	swapPointers(buffer, number_of_param);
}

void SQLEXECUTEROWSET_swap(char* buffer)
{
	short number_of_param = ExecuteRowset_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;

	pointers += number_of_param;

	DIALOGUE_ID_def *dialogueId;
//	IDL_char *stmtLabel;
//	IDL_string cursorName;
	IDL_short *sqlStmtType;
	IDL_long *inputRowCnt;
	SQL_DataValue_def *inputDataValue;
	IDL_short *sqlAsyncEnable;
	IDL_long *queryTimeout;

	dialogueId = (IDL_long *)(param[0] + (long)buffer);
	sqlStmtType = (IDL_short *)(param[3] + (long)buffer);
	inputRowCnt = (IDL_long *)(param[4] + (long)buffer);
	inputDataValue = (SQL_DataValue_def *)(param[5] + (long)buffer);
	sqlAsyncEnable = (IDL_short *)(param[6] + (long)buffer);
	queryTimeout = (IDL_long *)(param[7] + (long)buffer);
//
// swap
//
	LONG_swap(dialogueId);
	SHORT_swap(sqlStmtType);
	LONG_swap(inputRowCnt);
	ULONG_swap(&inputDataValue->_length);
	SHORT_swap(sqlAsyncEnable);
	LONG_swap(queryTimeout);

	swapPointers(buffer, number_of_param);
}

void SQLFETCHROWSET_swap(char* buffer)
{
	short number_of_param = FetchRowset_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;

	pointers += number_of_param;

	DIALOGUE_ID_def *dialogueId;
//	IDL_char *stmtLabel;
	IDL_long *maxRowCnt;
	IDL_long *maxRowLen;
	IDL_short *sqlAsyncEnable;
	IDL_long *queryTimeout;

	dialogueId = (IDL_long *)(param[0] + (long)buffer);
	maxRowCnt = (IDL_long *)(param[2] + (long)buffer);
	maxRowLen = (IDL_long *)(param[3] + (long)buffer);
	sqlAsyncEnable = (IDL_short *)(param[4] + (long)buffer);
	queryTimeout = (IDL_long *)(param[5] + (long)buffer);
//
// swap
//
	LONG_swap(dialogueId);
	LONG_swap(maxRowCnt);
	LONG_swap(maxRowLen);
	SHORT_swap(sqlAsyncEnable);
	LONG_swap(queryTimeout);

	swapPointers(buffer, number_of_param);
}

void SQLEXECDIRECT_swap(char* buffer)
{
	short number_of_param = ExecDirect_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;
	pointers += number_of_param;

	DIALOGUE_ID_def *dialogueId;
//	IDL_char *stmtLabel;
//	IDL_string cursorName;
//	IDL_char *stmtExplainLabel;
	IDL_short* stmtType;
	IDL_short *sqlStmtType;
//	IDL_string sqlString;
	IDL_short *sqlAsyncEnable;
	IDL_long *queryTimeout;

	dialogueId = (IDL_long *)(param[0] + (long)buffer);
	stmtType = (IDL_short *)(param[4] + (long)buffer);
	sqlStmtType = (IDL_short *)(param[5] + (long)buffer);
	sqlAsyncEnable = (IDL_short *)(param[7] + (long)buffer);
	queryTimeout = (IDL_long *)(param[8] + (long)buffer);
//
// swap
//
	LONG_swap(dialogueId);
	SHORT_swap(stmtType);
	SHORT_swap(sqlStmtType);
	SHORT_swap(sqlAsyncEnable);
	LONG_swap(queryTimeout);

	swapPointers(buffer, number_of_param);
}

void SQLEXECUTE_swap(char* buffer)
{
	short number_of_param = ExecuteN_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;
	pointers += number_of_param;

	DIALOGUE_ID_def *dialogueId;
//	IDL_char *stmtLabel;
//	IDL_string cursorName;
	IDL_short *sqlStmtType;
	IDL_long *inputRowCnt;
	SQLValueList_def *inputValueList;
	IDL_short *sqlAsyncEnable;
	IDL_long *queryTimeout;

	dialogueId = (IDL_long *)(param[0] + (long)buffer);
	sqlStmtType = (IDL_short *)(param[3] + (long)buffer);
	inputRowCnt = (IDL_long *)(param[4] + (long)buffer);
	inputValueList = (SQLValueList_def *)(param[5] + (long)buffer);
	sqlAsyncEnable = (IDL_short *)(param[6] + (long)buffer);
	queryTimeout = (IDL_long *)(param[7] + (long)buffer);
//
// swap
//
	LONG_swap(dialogueId);
	SHORT_swap(sqlStmtType);
	LONG_swap(inputRowCnt);
	SQL_VALUE_LIST_swap(buffer, inputValueList);
	SHORT_swap(sqlAsyncEnable);
	LONG_swap(queryTimeout);

	swapPointers(buffer, number_of_param);
}

void SQLFETCHPERF_swap(char* buffer)
{
	short number_of_param = FetchPerf_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;

	pointers += number_of_param;

	DIALOGUE_ID_def *dialogueId;
//	IDL_char *stmtLabel;
	IDL_long *maxRowCnt;
	IDL_long *maxRowLen;
	IDL_short *sqlAsyncEnable;
	IDL_long *queryTimeout;

	dialogueId = (IDL_long *)(param[0] + (long)buffer);
	maxRowCnt = (IDL_long *)(param[2] + (long)buffer);
	maxRowLen = (IDL_long *)(param[3] + (long)buffer);
	sqlAsyncEnable = (IDL_short *)(param[4] + (long)buffer);
	queryTimeout = (IDL_long *)(param[5] + (long)buffer);
//
// swap
//
	LONG_swap(dialogueId);
	LONG_swap(maxRowCnt);
	LONG_swap(maxRowLen);
	SHORT_swap(sqlAsyncEnable);
	LONG_swap(queryTimeout);

	swapPointers(buffer, number_of_param);
}

void SQLFREESTMT_swap(char* buffer)
{
	short number_of_param = Close_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;

	pointers += number_of_param;

	DIALOGUE_ID_def *dialogueId;
//	IDL_char *stmtLabel
	IDL_unsigned_short *freeResourceOpt;

	dialogueId = (IDL_long *)(param[0] + (long)buffer);
	freeResourceOpt = (IDL_unsigned_short *)(param[2] + (long)buffer);
//
// swap
//
	LONG_swap(dialogueId);
	USHORT_swap(freeResourceOpt);

	swapPointers(buffer, number_of_param);
}

void SQLENDTRAN_swap(char* buffer)
{
	short number_of_param = EndTransaction_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;

	pointers += number_of_param;

	DIALOGUE_ID_def *dialogueId;
	IDL_unsigned_short *transactionOpt;

	dialogueId = (IDL_long *)(param[0] + (long)buffer);
	transactionOpt = (IDL_unsigned_short *)(param[1] + (long)buffer);
//
// swap
//
	LONG_swap(dialogueId);
	USHORT_swap(transactionOpt);

	swapPointers(buffer, number_of_param);
}

void SQLGETCATALOGS_swap(char* buffer)
{
	short number_of_param = GetSQLCatalogs_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;

	pointers += number_of_param;

	DIALOGUE_ID_def *dialogueId;
//	IDL_char *stmtLabel;
	IDL_short *APIType;
//	IDL_char *catalogNm;
//	IDL_char *schemaNm;
//	IDL_char *tableNm;
//	IDL_char *tableTypeList;
//	IDL_char *columnNm;
	IDL_long *columnType;
	IDL_long *rowIdScope;
	IDL_long *nullable;
	IDL_long *uniqueness;
	IDL_long *accuracy;
	IDL_short *sqlType;
	IDL_unsigned_long *metadataId;

	dialogueId = (IDL_long *)(param[0] + (long)buffer);
	APIType = (IDL_short *)(param[2] + (long)buffer);
	columnType = (IDL_long *)(param[8] + (long)buffer);
	rowIdScope = (IDL_long *)(param[9] + (long)buffer);
	nullable = (IDL_long *)(param[10] + (long)buffer);
	uniqueness = (IDL_long *)(param[11] + (long)buffer);
	accuracy = (IDL_long *)(param[12] + (long)buffer);
	sqlType = (IDL_short *)(param[13] + (long)buffer);
	metadataId = (IDL_unsigned_long *)(param[14] + (long)buffer);
//
// swap
//
	LONG_swap(dialogueId);
	SHORT_swap(APIType);
	LONG_swap(columnType);
	LONG_swap(rowIdScope);
	LONG_swap(nullable);
	LONG_swap(uniqueness);
	LONG_swap(accuracy);
	SHORT_swap(sqlType);
	ULONG_swap(metadataId);

	swapPointers(buffer, number_of_param);
}

void SQLEXECUTECALL_swap(char* buffer)
{
	short number_of_param = ExecuteCall_in_params;
	long* param = (long*)buffer;
	long* pointers = (long*)buffer;

	pointers += number_of_param;

	DIALOGUE_ID_def *dialogueId;
//	IDL_char *stmtLabel;
//	IDL_string cursorName;
	IDL_short *sqlStmtType;
	IDL_long *inputRowCnt;
	SQLValueList_def *inputValueList;
	IDL_short *sqlAsyncEnable;
	IDL_long *queryTimeout;

	dialogueId = (IDL_long *)(param[0] + (long)buffer);
	sqlStmtType = (IDL_short *)(param[3] + (long)buffer);
	inputRowCnt = (IDL_long *)(param[4] + (long)buffer);
	inputValueList = (SQLValueList_def *)(param[5] + (long)buffer);
	sqlAsyncEnable = (IDL_short *)(param[6] + (long)buffer);
	queryTimeout = (IDL_long *)(param[7] + (long)buffer);
//
// swap
//
	LONG_swap(dialogueId);
	SHORT_swap(sqlStmtType);
	LONG_swap(inputRowCnt);
	SQL_VALUE_LIST_swap(buffer, inputValueList);
	SHORT_swap(sqlAsyncEnable);
	LONG_swap(queryTimeout);

	swapPointers(buffer, number_of_param);
}

#endif
