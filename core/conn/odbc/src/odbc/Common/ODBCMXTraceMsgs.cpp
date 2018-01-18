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

#include "ODBCMXTraceMsgs.h"

char charTable[] = {
//
//  00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 00
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 01
   ' ','!','"','#','$','%','&',39 ,'(',')','*','+',',','-','.','/',	// 02
   '0','1','2','3','4','5','6','7','8','9',':',';','<','=','>','?',	// 03
   '@','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',	// 04
   'P','Q','R','S','T','U','V','W','X','Y','Z','[',92 ,']','^','_',	// 05
   '`','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',	// 06
   'p','q','r','s','t','u','v','w','x','y','z','{','|','}','~','.',	// 07
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 08
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 09
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 0A
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 0B
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 0C
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 0D
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 0E
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'	// 0F
//  00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F
};

// Constructor for ODBC/MXTraceMsg class
ODBCMXTraceMsg::ODBCMXTraceMsg(DWORD processId, IDL_OBJECT_def objRef)
{
	collectorOpenError 	= false;
	pid			= processId;
	strcpy(objectRef, objRef);
	memset(collector, 0, EXT_FILENAME_LEN);

	msgBuffer = new char[MAX_MSG_LENGTH+10];

	temp = new char[200];
}

// Descructor for ODBC/MXTraceMsg class
ODBCMXTraceMsg::~ODBCMXTraceMsg()
{
	if(msgBuffer) delete[] msgBuffer;
	if(temp) delete[] temp;
}

// Opens EMS collector for ODBC/MX Server Trace
void ODBCMXTraceMsg::OpenTraceCollector(char* collectorName)
{
	short error = -1;
	char  errorMsg[100];
	char  errorStr[10];
	short retems;
}

// Closes collector for ODBC/MX Server Trace
void ODBCMXTraceMsg::CloseTraceCollector()
{
}

// Traces entry of odbc_SQLSvc_InitializeDialogue_ame
void ODBCMXTraceMsg::TraceConnectEnter(const USER_DESC_def	    *userDesc,
				       const CONNECTION_CONTEXT_def *inContext,
				       DIALOGUE_ID_def		     dialogueId)
{
	int   length     = 0;
	int   tempStrlen = 0;
	int   availBuf   = 0;
	char *pBuffer    = msgBuffer;
	seqNumber        = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterConnect");

	CheckCollectorError();
	
	pBuffer = printUSER_DESC_def(userDesc, pBuffer);
	pBuffer = printCONNECTION_CONTEXT_def(inContext, pBuffer);

	tempStrlen = sprintf(temp, "DialogueId:%d ", dialogueId);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_InitializeDialogue_ame
void ODBCMXTraceMsg::TraceConnectExit(odbc_SQLSvc_InitializeDialogue_exc_ exception, 
				      OUT_CONNECTION_CONTEXT_def	  outContext)
{
	int          length     = 0;
	int	     tempStrlen = 0;
	int          availBuf   = 0;
	char        *pBuffer = msgBuffer;
	VERSION_def *clientVersionPtr;
	seqNumber            = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitConnect");

	CheckCollectorError();
	
	length   = sprintf(pBuffer, "ExceptionNr:%d ExceptionDetail:%d ", 
			   exception.exception_nr, exception.exception_detail); 
	pBuffer += length;
	
	for (int i=0; i<outContext.versionList._length; i++) 
	{
		clientVersionPtr = outContext.versionList._buffer + i;

		tempStrlen = sprintf(temp, "Component%d:%d ", i, clientVersionPtr->componentId);
		pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "Major%d:%d ", i, clientVersionPtr->majorVersion);
		pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "Minor%d:%d ", i, clientVersionPtr->minorVersion);
		pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "Build%d:%d ", i, clientVersionPtr->buildId);
		pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	}

	tempStrlen = sprintf(temp, "Catalog:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer    = printLongString(pBuffer, (char*)outContext.catalog);

	tempStrlen = sprintf(temp, "Schema:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer    = printLongString(pBuffer, (char*)outContext.schema);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);	
}

// Traces entry of odbc_SQLSvc_TerminateDialogue_ame		
void ODBCMXTraceMsg::TraceDisconnectEnter(DIALOGUE_ID_def dialogueId)
{
	int length = 0;
	seqNumber  = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterDisconnect");

	CheckCollectorError();
	
	length = sprintf(msgBuffer, "DialogueId:%d ", dialogueId);
	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_TerminateDialogue_ame
void ODBCMXTraceMsg::TraceDisconnectExit(odbc_SQLSvc_TerminateDialogue_exc_ exception)
{
	int   length = 0;
	seqNumber    = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitDisconnect");

	CheckCollectorError();
		
	length = sprintf(msgBuffer, "ExceptionNr:%d ExceptionDetail:%d", 
			 exception.exception_nr, exception.exception_detail); 
	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}
//LCOV_EXCL_START
// Traces entry of odbc_SQLSvc_SetConnectionOption_ame
void ODBCMXTraceMsg::TraceConnectOptionEnter(DIALOGUE_ID_def dialogueId,
				             IDL_short	     connectionOption,
					     IDL_long	     optionValueNum,
					     IDL_string	     optionValueStr)
{
	int   length  = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterConnectOption");

	CheckCollectorError();
		
	length   = sprintf(pBuffer,"DialogueId:%d ", dialogueId);
	pBuffer += length;

	length   = sprintf(pBuffer,"ConnectOption:%d ", connectionOption);
	pBuffer += length;

	length   = sprintf(pBuffer,"OptionValNum:%d OptionValStr:", optionValueNum);
	pBuffer += length;

	pBuffer  = printLongString(pBuffer, optionValueStr);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);

}

// Traces exit of odbc_SQLSvc_SetConnectionOption_ame
void ODBCMXTraceMsg::TraceConnectOptionExit(odbc_SQLSvc_SetConnectionOption_exc_ exception,
				            ERROR_DESC_LIST_def			 sqlWarning)
{
	int   length  = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitConnectOption");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ExceptionNr:%d ExceptionDetail:%d ", 
			   exception.exception_nr, exception.exception_detail); 
	pBuffer += length;
	
	pBuffer  = printSQLWarning(sqlWarning, pBuffer);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces entry of odbc_SQLSvc_EndTransaction_ame
void ODBCMXTraceMsg::TraceEndTransactEnter(DIALOGUE_ID_def    dialogueId,
					   IDL_unsigned_short transactionOpt)
{
	int   length  = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterEndTransaction");

	CheckCollectorError();
	
	length   = sprintf(pBuffer,"DialogueId:%d ", dialogueId);
	pBuffer += length;

	length   = sprintf(pBuffer,"TransactOpt:%d ", transactionOpt);
	pBuffer += length;

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_EndTransaction_ame
void ODBCMXTraceMsg::TraceEndTransactExit(odbc_SQLSvc_EndTransaction_exc_ exception,
					  ERROR_DESC_LIST_def		  sqlWarning)
{
	int   length  = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitEndTransaction");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ExceptionNr:%d ExceptionDetail:%d ", 
			   exception.exception_nr, exception.exception_detail); 
	pBuffer += length;

	pBuffer  = printSQLWarning(sqlWarning, pBuffer);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces entry of odbc_SQLSvc_Prepare_ame
void ODBCMXTraceMsg::TracePrepareEnter(DIALOGUE_ID_def dialogueId,
				       const IDL_char *stmtLabel,
				       const IDL_char *stmtExplainLabel,
				       IDL_short       stmtType,
				       IDL_string      sqlString,
				       IDL_short       sqlAsyncEnable,
				       IDL_long	       queryTimeout)
{
	int   length     = 0;
	int   tempStrlen = 0;
	int   availBuf   = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterPrepare");

	CheckCollectorError();
	
	length   = sprintf(pBuffer,"DialogueId:%d StmtLabel:", dialogueId);
	pBuffer += length;
	pBuffer  = printLongString(pBuffer, (char*)stmtLabel);

	tempStrlen = sprintf(temp,"StmtExLabel:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)stmtExplainLabel);

	tempStrlen = sprintf(temp,"StmtType:%d SqlString:", stmtType);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)sqlString);

	tempStrlen = sprintf(temp,"SqlAsyncEnable:%d QueryTimeout:%d", sqlAsyncEnable, queryTimeout);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_Prepare_ame
void ODBCMXTraceMsg::TracePrepareExit(odbc_SQLSvc_Prepare_exc_ exception, 
				      IDL_long		       estimatedCost, 
				      SQLItemDescList_def      inputDesc, 
				      SQLItemDescList_def      outputDesc, 
				      ERROR_DESC_LIST_def      sqlWarning)
{
	int   length  = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitPrepare");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ExceptionNr:%d ExceptionDetail:%d EstCost:%d ", 
			   exception.exception_nr, exception.exception_detail, estimatedCost); 
	pBuffer += length;
	pBuffer  = printSQLItemDesc(inputDesc,  pBuffer);
	pBuffer  = printSQLItemDesc(outputDesc, pBuffer);
	pBuffer  = printSQLWarning(sqlWarning, pBuffer);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}
//LCOV_EXCL_STOP
// Traces entry of odbc_SQLSvc_Prepare2_ame
void ODBCMXTraceMsg::TracePrepare2Enter(DIALOGUE_ID_def dialogueId,
				IDL_long sqlAsyncEnable,
				IDL_long queryTimeout,
				IDL_long inputRowCnt,
				IDL_long sqlStmtType,
				IDL_long stmtLength,	
				const IDL_char *stmtLabel,
				IDL_long stmtLabelCharset,
				IDL_long cursorLength,
				IDL_string cursorName,
				IDL_long cursorCharset,
				IDL_long moduleNameLength,
				const IDL_char *moduleName,
				IDL_long moduleCharset,
				IDL_long_long moduleTimestamp,
				IDL_long sqlStringLength,
				IDL_string sqlString,
				IDL_long sqlStringCharset,
				IDL_long setStmtOptionsLength,
				IDL_string setStmtOptions,
				IDL_long_long txnId,
				IDL_long holdableCursor)
{
	int   length     = 0;
	int   tempStrlen = 0;
	int   availBuf   = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterPrepare2");

	CheckCollectorError();

	length   = sprintf(pBuffer,"DialogueId:%d SqlAsyncEnable:%d TransactionID:%d SqlCursorHoldable:%d QueryTimeout:%d InputRowCnt:%d SqlStmtType:%d StmtLabel:", 
				dialogueId, sqlAsyncEnable, txnId, holdableCursor, queryTimeout, inputRowCnt, sqlStmtType);
	pBuffer += length;	
	if (stmtLength > 0) pBuffer = printLongString(pBuffer, (char*)stmtLabel);

	tempStrlen = sprintf(temp,"StmtLabelCharset:%d CursorName:", stmtLabelCharset);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	if (cursorLength > 0) pBuffer  = printLongString(pBuffer, cursorName);

	tempStrlen = sprintf(temp,"CursorCharset:%d ModuleName:", cursorCharset);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	if (moduleNameLength > 0) pBuffer  = printLongString(pBuffer, (char*)moduleName);

	tempStrlen = sprintf(temp,"ModuleCharset:%d ModuleTimestamp:%x: SqlString:", moduleCharset, moduleTimestamp);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	if (sqlStringLength > 0) pBuffer  = printLongString(pBuffer, sqlString);

	tempStrlen = sprintf(temp,"SqlStringCharset:%d SetStmtOptions:", sqlStringCharset);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	if (setStmtOptionsLength > 0) pBuffer  = printLongString(pBuffer, setStmtOptions);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_Prepare2_ame
void ODBCMXTraceMsg::TracePrepare2Exit(IDL_long returnCode,
				IDL_long sqlWarningOrErrorLength,
				BYTE *sqlWarningOrError,
				IDL_long sqlQueryType,
				Long stmtHandle,
				IDL_long estimatedCost,
				IDL_long inputDescLength,
				BYTE *inputDesc,
				IDL_long outputDescLength,
				BYTE *outputDesc)
{
	int   length     = 0;
	int   tempStrlen = 0;
	char *pBuffer    = msgBuffer;
	seqNumber        = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitPrepare2");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ReturnCode:%d SqlWarningOrError:", returnCode); 
	pBuffer += length;
	if (sqlWarningOrErrorLength > 0) pBuffer = printHex(pBuffer, sqlWarningOrError, sqlWarningOrErrorLength);

	tempStrlen = sprintf(temp, "SqlQueryType:%d StmtHandle:%d EstCost:%d InputDesc:",
				sqlQueryType, stmtHandle, estimatedCost);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	if (inputDescLength > 0) pBuffer = printHex(pBuffer, inputDesc, inputDescLength);

	tempStrlen = sprintf(temp, "OutputDesc:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	if (outputDescLength > 0) pBuffer = printHex(pBuffer, outputDesc, outputDescLength);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces entry of odbc_SQLSvc_ExecDirect_ame
void ODBCMXTraceMsg::TraceExecDirectEnter(DIALOGUE_ID_def dialogueId,
					  const IDL_char *stmtLabel,
					  IDL_string	  cursorName,
					  const IDL_char *stmtExplainLabel,
					  IDL_short	  stmtType,
					  IDL_short	  sqlStmtType,
					  IDL_string	  sqlString,
					  IDL_short	  sqlAsyncEnable,
					  IDL_long	  queryTimeout)
{
	int   length     = 0;
	int   tempStrlen = 0;
	int   availBuf   = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterExecDirect");

	CheckCollectorError();
	
	length   = sprintf(pBuffer,"DialogueId:%d StmtLabel:", dialogueId);
	pBuffer += length;
	pBuffer  = printLongString(pBuffer, (char*)stmtLabel);
	
	tempStrlen = sprintf(temp,"CursorName:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)cursorName);

	tempStrlen = sprintf(temp,"StmtExLabel:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)stmtExplainLabel);

	tempStrlen = sprintf(temp,"StmtType:%d SqlStmtType:%d SqlStr:", stmtType, sqlStmtType);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)sqlString);

	tempStrlen = sprintf(temp,"SqlAsyncEnable:%d QueryTimeout:%d", sqlAsyncEnable, queryTimeout);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_ExecDirect_ame
void ODBCMXTraceMsg::TraceExecDirectExit(odbc_SQLSvc_ExecDirect_exc_ exception, 
					 IDL_long		     estimatedCost, 
					 SQLItemDescList_def	     outputDesc, 
					 IDL_long		     rowsAffected,
					 ERROR_DESC_LIST_def	     sqlWarning)
{
	int   length     = 0;
	int   tempStrlen = 0;
	int   availBuf   = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitExecDirect");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ExceptionNr:%d ExceptionDetail:%d EstCost:%d ", 
			   exception.exception_nr, exception.exception_detail, estimatedCost); 
	pBuffer += length;
	pBuffer  = printSQLItemDesc(outputDesc, pBuffer);
	
	tempStrlen = sprintf(temp,"RowsAffected:%d ", rowsAffected);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer    = printSQLWarning(sqlWarning, pBuffer);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}
//LCOV_EXCL_START
// Traces entry of odbc_SQLSvc_ExecuteN_ame
void ODBCMXTraceMsg::TraceExecuteEnter(DIALOGUE_ID_def	       dialogueId,
				       const IDL_char	      *stmtLabel,
				       IDL_string	       cursorName,
				       IDL_short	       sqlStmtType,
				       IDL_long		       inputRowCnt,
				       const SQLValueList_def *inputValueList,
				       IDL_short	       sqlAsyncEnable,
				       IDL_long		       queryTimeout)
{
	int                length     = 0;
	int                tempStrlen = 0;
	int                availBuf   = 0;
	char              *pBuffer    = msgBuffer;
	SQLValue_def      *sqlval;
	SQL_DataValue_def  dataval;
	seqNumber                     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterExecute");

	CheckCollectorError();
	
	length   = sprintf(pBuffer,"DialogueId:%d StmtLabel:", dialogueId);
	pBuffer += length;
	pBuffer  = printLongString(pBuffer, (char*)stmtLabel);

	tempStrlen = sprintf(temp,"CursorName:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer    = printLongString(pBuffer, (char*)cursorName);

	tempStrlen = sprintf(temp,"SqlStmtType:%d InputRowCnt:%d", sqlStmtType, inputRowCnt);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

	for (int i=0; i<inputValueList->_length; i++)
	{
		tempStrlen = sprintf(temp," InputVal%d:", i);
		pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

		sqlval  = (SQLValue_def *)inputValueList->_buffer + i;
		dataval = sqlval->dataValue;
		pBuffer = printHex(pBuffer, dataval._buffer, dataval._length);
	}

	tempStrlen = sprintf(temp," SqlAsyncEnable:%d QueryTimeout:%d", sqlAsyncEnable, queryTimeout);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	
	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_ExecuteN_ame
void ODBCMXTraceMsg::TraceExecuteExit(odbc_SQLSvc_ExecuteN_exc_ exception, 
				      IDL_long		        rowsAffected, 
				      ERROR_DESC_LIST_def	sqlWarning)
{
	int   length  = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitExecute");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ExceptionNr:%d ExceptionDetail:%d RowsAffected:%d ", 
			   exception.exception_nr, exception.exception_detail, rowsAffected); 
	pBuffer += length;
	pBuffer  = printSQLWarning(sqlWarning, pBuffer);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}
//LCOV_EXCL_STOP
// Traces entry of odbc_SQLSvc_Execute2_ame
void ODBCMXTraceMsg::TraceExecute2Enter(DIALOGUE_ID_def dialogueId,
				IDL_long sqlAsyncEnable,
				IDL_long queryTimeout,
				IDL_long inputRowCnt,
				IDL_long sqlStmtType,
				Long stmtHandle,
				IDL_long cursorLength,
				IDL_string cursorName,
				IDL_long cursorCharset,
				IDL_long inValuesLength,
				BYTE *inValues)
{
	int                length     = 0;
	int                tempStrlen = 0;
	int                availBuf   = 0;
	char              *pBuffer    = msgBuffer;
	SQLValue_def      *sqlval;
	SQL_DataValue_def  dataval;
	seqNumber                     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterExecute2");

	CheckCollectorError();

	length   = sprintf(pBuffer,"DialogueId:%d SqlAsyncEnable:%d QueryTimeout:%d InputRowCnt:%d SqlStmtType:%d StmtHandle:%d CursorName:", 
				dialogueId, sqlAsyncEnable, queryTimeout, inputRowCnt, sqlStmtType, stmtHandle);
	pBuffer += length;
	if (cursorLength > 0) pBuffer  = printLongString(pBuffer, (char*)cursorName);

	tempStrlen = sprintf(temp,"CursorCharset:%d InValues:", cursorCharset);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	if (inValuesLength > 0)	pBuffer    = printHex(pBuffer, inValues, inValuesLength);
	
	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_Execute2_ame
void ODBCMXTraceMsg::TraceExecute2Exit(IDL_long returnCode,
				IDL_long sqlWarningOrErrorLength,
				BYTE *sqlWarningOrError,
				IDL_long rowsAffected,
				IDL_long outValuesLength,
				BYTE *outValues)
{
	int  length     = 0;
	int  tempStrlen = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitExecute2");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ReturnCode:%d sqlWarningOrError:", returnCode); 
	pBuffer += length;
	if (sqlWarningOrErrorLength > 0) pBuffer  = printHex(pBuffer, sqlWarningOrError, sqlWarningOrErrorLength);

	tempStrlen = sprintf(temp, "RowsAffected:%d OutValues:", rowsAffected);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	if (outValuesLength > 0) pBuffer = printHex(pBuffer, outValues, outValuesLength);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}
//LCOV_EXCL_START
// Traces entry of odbc_SQLSvc_ExecuteCall_ame
void ODBCMXTraceMsg::TraceExecuteCallEnter(DIALOGUE_ID_def	       dialogueId,
				       const IDL_char	      *stmtLabel,
				       IDL_string	       cursorName,
				       IDL_short	       sqlStmtType,
				       IDL_long		       inputRowCnt,
				       const SQLValueList_def *inputValueList,
				       IDL_short	       sqlAsyncEnable,
				       IDL_long		       queryTimeout)
{
	int                length     = 0;
	int                tempStrlen = 0;
	int                availBuf   = 0;
	char              *pBuffer    = msgBuffer;
	SQLValue_def      *sqlval;
	SQL_DataValue_def  dataval;
	seqNumber                     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterExecuteCall");

	CheckCollectorError();
	
	length   = sprintf(pBuffer,"DialogueId:%d StmtLabel:", dialogueId);
	pBuffer += length;
	pBuffer  = printLongString(pBuffer, (char*)stmtLabel);

	tempStrlen = sprintf(temp,"CursorName:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer    = printLongString(pBuffer, (char*)cursorName);

	tempStrlen = sprintf(temp,"SqlStmtType:%d InputRowCnt:%d", sqlStmtType, inputRowCnt);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

	for (int i=0; i<inputValueList->_length; i++)
	{
		tempStrlen = sprintf(temp," InputVal%d:", i);
		pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

		sqlval  = (SQLValue_def *)inputValueList->_buffer + i;
		dataval = sqlval->dataValue;
		pBuffer = printHex(pBuffer, dataval._buffer, dataval._length);
	}

	tempStrlen = sprintf(temp," SqlAsyncEnable:%d QueryTimeout:%d", sqlAsyncEnable, queryTimeout);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	
	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_ExecuteCall_ame
void ODBCMXTraceMsg::TraceExecuteCallExit(odbc_SQLSvc_ExecuteCall_exc_ exception, 
				      IDL_long		        rowsAffected, 
					  SQLValueList_def	    *outputValueList, 
				      ERROR_DESC_LIST_def	sqlWarning)
{
	int		length  = 0;
	int		tempStrlen = 0;
	char	*pBuffer = msgBuffer;
	seqNumber     = 0;
	SQLValue_def		*sqlval;
	SQL_DataValue_def	dataval;

	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitExecuteCall");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ExceptionNr:%d ExceptionDetail:%d RowsAffected:%d ", 
			   exception.exception_nr, exception.exception_detail, rowsAffected); 
	pBuffer += length;
	for (int i=0; i<outputValueList->_length; i++)
	{
		tempStrlen = sprintf(temp," OutputVal%d:", i);
		pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

		sqlval  = (SQLValue_def *)outputValueList->_buffer + i;
		dataval = sqlval->dataValue;
		pBuffer = printHex(pBuffer, dataval._buffer, dataval._length);
	}
	pBuffer  = printSQLWarning(sqlWarning, pBuffer);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces entry of odbc_SQLSvc_FetchPerf_ame
void ODBCMXTraceMsg::TraceFetchPerfEnter(DIALOGUE_ID_def dialogueId,
					 const IDL_char *stmtLabel,
					 IDL_long        maxRowCnt,
					 IDL_long        maxRowLen,
					 IDL_short       sqlAsyncEnable,
					 IDL_long        queryTimeout)
{
	int   length     = 0;
	int   tempStrlen = 0;
	int   availBuf   = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterFetchPerf");

	CheckCollectorError();
	
	length   = sprintf(pBuffer,"DialogueId:%d StmtLabel:", dialogueId);
	pBuffer += length;
	pBuffer  = printLongString(pBuffer, (char*)stmtLabel);

	tempStrlen = sprintf(temp,"MaxRowCnt:%d MaxRowLen:%d SqlAsyncEnable:%d QueryTimeout:%d", 
		     maxRowCnt, maxRowLen, sqlAsyncEnable, queryTimeout);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_FetchPerf_ame
void ODBCMXTraceMsg::TraceFetchPerfExit(odbc_SQLSvc_FetchPerf_exc_     exception, 
					IDL_long		       rowsAffected, 
					SQL_DataValue_def	       outputDataValue, 
					ERROR_DESC_LIST_def	       sqlWarning)
{
	int   length  = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitFetchPerf");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ExceptionNr:%d ExceptionDetail:%d RowsAffected:%d OutputData:", 
			   exception.exception_nr, exception.exception_detail, rowsAffected); 
	pBuffer += length;
	pBuffer  = printHex(pBuffer, outputDataValue._buffer, outputDataValue._length);

	length   = sprintf(pBuffer, " ");
	pBuffer += length;

	pBuffer  = printSQLWarning(sqlWarning, pBuffer);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces entry of odbc_SQLSvc_FetchN_ame
void ODBCMXTraceMsg::TraceFetchEnter(DIALOGUE_ID_def dialogueId,
				     const IDL_char *stmtLabel,
				     IDL_long        maxRowCnt,
				     IDL_long        maxRowLen,
				     IDL_short       sqlAsyncEnable,
				     IDL_long        queryTimeout)
{
	int   length     = 0;
	int   tempStrlen = 0;
	int   availBuf   = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterFetch");

	CheckCollectorError();
	
	length   = sprintf(pBuffer,"DialogueId:%d StmtLabel:", dialogueId);
	pBuffer += length;
	pBuffer  = printLongString(pBuffer, (char*)stmtLabel);

	tempStrlen = sprintf(temp,"MaxRowCnt:%d MaxRowLen:%d SqlAsyncEnable:%d QueryTimeout:%d", 
		     maxRowCnt, maxRowLen, sqlAsyncEnable, queryTimeout);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_FetchN_ame
void ODBCMXTraceMsg::TraceFetchExit(odbc_SQLSvc_FetchN_exc_    exception, 
				    IDL_long		       rowsAffected, 
				    SQLValueList_def	      *outputValueList, 
				    ERROR_DESC_LIST_def	       sqlWarning)
{
	int		   length     = 0;
	int		   tempStrlen = 0;
	int		   availBuf   = 0;
	char		  *pBuffer    = msgBuffer;
	seqNumber		      = 0;
	SQLValue_def	  *sqlval;
	SQL_DataValue_def  dataval;

	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitFetch");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ExceptionNr:%d ExceptionDetail:%d RowsAffected:%d ", 
			   exception.exception_nr, exception.exception_detail, rowsAffected); 
	pBuffer += length;

	for (int i=0; i<outputValueList->_length; i++)
	{
		tempStrlen = sprintf(temp," OutputVal%d:", i);
		pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

		sqlval  = (SQLValue_def *)outputValueList->_buffer + i;
		dataval = sqlval->dataValue;
		pBuffer = printHex(pBuffer, dataval._buffer, dataval._length);
	}

	pBuffer  = printSQLWarning(sqlWarning, pBuffer);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}
//LCOV_EXCL_STOP
// Traces entry of odbc_SQLSvc_Close_ame
void ODBCMXTraceMsg::TraceCloseEnter(DIALOGUE_ID_def	 dialogueId,
				     const IDL_char     *stmtLabel,
				     IDL_unsigned_short  freeResourceOpt)
{
	int   length     = 0;
	int   tempStrlen = 0;
	int   availBuf   = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterClose");

	CheckCollectorError();
	
	length   = sprintf(pBuffer,"DialogueId:%d StmtLabel:", dialogueId);
	pBuffer += length;
	pBuffer  = printLongString(pBuffer, (char*)stmtLabel);

	tempStrlen = sprintf(temp,"FreeRSOpt:%d", freeResourceOpt);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}
//LCOV_EXCL_START
// Traces exit of odbc_SQLSvc_Close_ame
void ODBCMXTraceMsg::TraceCloseExit(odbc_SQLSvc_Close_exc_ exception, 
				    IDL_long		   rowsAffected, 
				    ERROR_DESC_LIST_def	   sqlWarning)
{
	int   length  = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitClose");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ExceptionNr:%d ExceptionDetail:%d RowsAffected:%d ", 
			   exception.exception_nr, exception.exception_detail, rowsAffected); 
	pBuffer += length;
	pBuffer  = printSQLWarning(sqlWarning, pBuffer);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}
//LCOV_EXCL_STOP
// Traces exit of odbc_SQLSvc_Close_ame
void ODBCMXTraceMsg::TraceClose2Exit(IDL_long returnCode, 
				    IDL_long		   rowsAffected, 
					IDL_long sqlWarningOrErrorLength,
					BYTE *sqlWarningOrError)
{
	int   length  = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitClose");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ReturnCode:%d RowsAffected: %d sqlWarningOrError:", returnCode, rowsAffected); 
	pBuffer += length;
	if (sqlWarningOrErrorLength > 0) pBuffer  = printHex(pBuffer, sqlWarningOrError, sqlWarningOrErrorLength);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces entry of odbc_SQLSvc_GetSQLCatalogs_ame
void ODBCMXTraceMsg::TraceGetSQLCatalogsEnter(DIALOGUE_ID_def  dialogueId,
					      const IDL_char   *stmtLabel,
					      IDL_short         APIType,
					      const IDL_char   *catalogNm,
					      const IDL_char   *schemaNm,
					      const IDL_char   *tableNm,
					      const IDL_char   *tableTypeList,
					      const IDL_char   *columnNm,
					      IDL_long	        columnType,
					      IDL_long	        rowIdScope,
					      IDL_long	        nullable,
					      IDL_long	        uniqueness,
					      IDL_long	        accuracy,
					      IDL_short	        sqlType,
					      IDL_unsigned_long metadataId,
					      const IDL_char   *fkcatalogNm,
					      const IDL_char   *fkschemaNm,
					      const IDL_char   *fktableNm)
{
	int   length     = 0;
	int   tempStrlen = 0;
	int   availBuf   = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterGetSQLCatalogs");

	CheckCollectorError();
	
	length   = sprintf(pBuffer,"DialogueId:%d StmtLabel:", dialogueId);
	pBuffer += length;
	pBuffer  = printLongString(pBuffer, (char*)stmtLabel);

	tempStrlen = sprintf(temp,"APIType:%d Catalog:", APIType);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)catalogNm);

	tempStrlen = sprintf(temp,"Schema:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)schemaNm);

	tempStrlen = sprintf(temp,"Table:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)tableNm);

	tempStrlen = sprintf(temp,"TableTypeList:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)tableTypeList);

	tempStrlen = sprintf(temp,"Column:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)columnNm);

	tempStrlen = sprintf(temp,"ColumnType:%d RowIDScope:%d Nullable:%d ", columnType, rowIdScope, nullable);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

	tempStrlen = sprintf(temp,"Uniqueness:%d Accuracy:%d SqlType:%d MetadataId:%d", uniqueness, accuracy, sqlType, metadataId);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

	tempStrlen = sprintf(temp,"FKCatalog:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)fkcatalogNm);

	tempStrlen = sprintf(temp,"FKSchema:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)fkschemaNm);

	tempStrlen = sprintf(temp,"FKTable:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)fktableNm);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_GetSQLCatalogs_ame
void ODBCMXTraceMsg::TraceGetSQLCatalogsExit(odbc_SQLSvc_GetSQLCatalogs_exc_ exception, 
					     STMT_NAME_def		     catStmtLabel,
					     SQLItemDescList_def	     outputDesc,
					     ERROR_DESC_LIST_def	     sqlWarning)
{
	int   length  = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitGetSQLCatalogs");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ExceptionNr:%d ExceptionDetail:%d CatStmtLabel:%s ", 
			   exception.exception_nr, exception.exception_detail, catStmtLabel); 
	pBuffer += length;
	pBuffer  = printSQLItemDesc(outputDesc, pBuffer);
	pBuffer  = printSQLWarning(sqlWarning, pBuffer);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

#ifdef __OBSOLETE
// Traces entry of odbc_SQLSvc_CancelStatement_ame
void ODBCMXTraceMsg::TraceCancelStatementEnter(DIALOGUE_ID_def  dialogueId,
					       const IDL_char  *stmtLabel)
{
	int   length  = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterCancelStmt");

	CheckCollectorError();
	
	length   = sprintf(pBuffer,"DialogueId:%d StmtLabel:", dialogueId);
	pBuffer += length;
	pBuffer  = printLongString(pBuffer, (char*)stmtLabel);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}
#endif
//LCOV_EXCL_START
// Traces exit of odbc_SQLSvc_CancelStatement_ame
void ODBCMXTraceMsg::TraceCancelStatementExit(odbc_SQLSvc_CancelStatement_exc_ exception, 
					      ERROR_DESC_LIST_def	       sqlWarning)
{
	int   length  = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitCancelStatement");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ExceptionNr:%d ExceptionDetail:%d ", 
			   exception.exception_nr, exception.exception_detail); 
	pBuffer += length;
	pBuffer  = printSQLWarning(sqlWarning, pBuffer);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces entry of odbc_SQLSvc_StopServer_ame
void ODBCMXTraceMsg::TraceStopServerEnter(DIALOGUE_ID_def dialogueId,
					  IDL_long	  StopType,
					  IDL_string	  ReasonText)
{
	int length    = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterStopSrvr");

	CheckCollectorError();	

	length   = sprintf(pBuffer,"DialogueId:%d StopType:%d Reason:", dialogueId, StopType);
	pBuffer += length;
	pBuffer  = printLongString(pBuffer, (char*)ReasonText);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_StopServer_ame
void ODBCMXTraceMsg::TraceStopServerExit(odbc_SQLSvc_StopServer_exc_ exception)
{
	int   length  = 0;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitStopServer");

	CheckCollectorError();
		
	length   = sprintf(msgBuffer, "ExceptionNr:%d ExceptionDetail:%d ", 
			   exception.exception_nr, exception.exception_detail); 

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces entry of odbc_SQLSvc_FetchRowset_ame
void ODBCMXTraceMsg::TraceFetchRowsetEnter(DIALOGUE_ID_def dialogueId,
					   const IDL_char *stmtLabel,
					   IDL_long	   maxRowCnt,
					   IDL_long	   maxRowLen,
					   IDL_short	   sqlAsyncEnable,
					   IDL_long	   queryTimeout)
{
	int   length     = 0;
	int   tempStrlen = 0;
	int   availBuf   = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterFetchRowset");

	CheckCollectorError();
	
	length   = sprintf(pBuffer,"DialogueId:%d StmtLabel:", dialogueId);
	pBuffer += length;
	pBuffer  = printLongString(pBuffer, (char*)stmtLabel);

	tempStrlen = sprintf(temp,"MaxRowCnt:%d MaxRowLen:%d SqlAsyncEnable:%d QueryTimeout:%d", 
		     maxRowCnt, maxRowLen, sqlAsyncEnable, queryTimeout);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_FetchRowset_ame
void ODBCMXTraceMsg::TraceFetchRowsetExit(odbc_SQLSvc_FetchRowset_exc_ exception, 
					  IDL_long		       rowsAffected, 
					  SQL_DataValue_def	       outputDataValue, 
					  ERROR_DESC_LIST_def	       sqlWarning)
{
	int   length  = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitFetchRowset");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ExceptionNr:%d ExceptionDetail:%d RowsAffected:%d OutputData:", 
			   exception.exception_nr, exception.exception_detail, rowsAffected); 
	pBuffer += length;
	pBuffer  = printHex(pBuffer, outputDataValue._buffer, outputDataValue._length);

	length   = sprintf(pBuffer, " ");
	pBuffer += length;

	pBuffer  = printSQLWarning(sqlWarning, pBuffer);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}
//LCOV_EXCL_STOP
//---------------------------------------------------------------------------------------
// Traces entry of odbc_SQLSvc_Fetch2_ame
void ODBCMXTraceMsg::TraceSrvrFetchEnter(DIALOGUE_ID_def dialogueId,
			                             IDL_long	     sqlAsyncEnable,
			                             IDL_long	     queryTimeout,
                                        		 Long        stmtHandle,			     
			                             IDL_long	     maxRowCnt,
			                             IDL_long        maxRowLen,
										 IDL_long		 fetchAhead)
{
   char *pBuffer    = msgBuffer;
   seqNumber        = 0;
   int   length     = 0;

   sprintf(seqNumStr, "%d", seqNumber);
   strcpy(funcName, "EnterSrvrFetch");

   CheckCollectorError();

   length   = sprintf(  pBuffer
                    , "DialogueId:%d SqlAsyncEnable:%d QueryTimeout:%d StmtHandle:%d maxRowCnt:%d maxRowLen:%d fetchAhead:%d"
		            ,  dialogueId
                    ,  sqlAsyncEnable
                    ,  queryTimeout
                    ,  stmtHandle
                    ,  maxRowCnt
					,  maxRowLen
					,  fetchAhead);
	
   SendEventMsg(MSG_SERVER_TRACE_INFO, 
	            EVENTLOG_INFORMATION_TYPE, 
				pid, 
				ODBCMX_SERVICE, 
		        objectRef, 
				4, 
				srvrGlobal->sessionId, 
				funcName, 
				seqNumStr, 
				msgBuffer);

}  // end ODBCMXTraceMsg::TraceSrvrFetchEnter

//---------------------------------------------------------------------------------------
// Traces exit of odbc_SQLSvc_Fetch2_ame
void ODBCMXTraceMsg::TraceSrvrFetchExit( IDL_long  returnCode,
				                         IDL_long  sqlWarningOrErrorLength,
				                         BYTE     *sqlWarningOrError,
				                         IDL_long  rowsAffected,
				                         IDL_long  outValuesLength,
				                         BYTE     *outValues)
{
  int   length     = 0;
  int   tempStrlen = 0;
  char *pBuffer    = msgBuffer;
  seqNumber        = 0;

  sprintf(seqNumStr, "%d", seqNumber);
  strcpy(funcName, "ExitFetch2");

  CheckCollectorError();
		
  length   = sprintf(pBuffer, "ReturnCode:%d sqlWarningOrError:", returnCode); 
  pBuffer += length;
  if (sqlWarningOrErrorLength > 0) 
    pBuffer  = printHex(pBuffer, sqlWarningOrError, sqlWarningOrErrorLength);

  tempStrlen = sprintf(temp, "RowsAffected:%d OutValues:", rowsAffected);
  pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
  if (outValuesLength > 0)
     pBuffer = printHex(pBuffer, outValues, outValuesLength);

  SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
	       objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);

}  // end ODBCMXTraceMsg::TraceFetch2Exit
//LCOV_EXCL_START
void ODBCMXTraceMsg::TraceFetch2Enter(DIALOGUE_ID_def dialogueId,
			              IDL_long	      sqlAsyncEnable,
			              IDL_long	      queryTimeout,
                                   Long        stmtHandle,			     
			              IDL_long	      maxRowCnt,
			              IDL_long        cursorLength,
			              IDL_string      cursorName,
			              IDL_long        cursorCharset)
  {
  int                length     = 0;
  int                tempStrlen = 0;
  int                availBuf   = 0;
  char              *pBuffer    = msgBuffer;
  SQLValue_def      *sqlval;
  SQL_DataValue_def  dataval;
  seqNumber                     = 0;

  sprintf(seqNumStr, "%d", seqNumber);
  strcpy(funcName, "EnterFetch2");

  CheckCollectorError();

  length   = sprintf(  pBuffer
                    , "DialogueId:%d SqlAsyncEnable:%d QueryTimeout:%d StmtHandle:%d maxRowCnt:%d CursorName:"
		    ,  dialogueId
                    ,  sqlAsyncEnable
                    ,  queryTimeout
                    ,  stmtHandle
                    ,  maxRowCnt);
  pBuffer += length;
  if (cursorLength > 0) 
    pBuffer  = printLongString(pBuffer, (char*)cursorName);

  tempStrlen = sprintf(temp,"CursorCharset:%d InValues:", cursorCharset);
  pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	
  SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);

}  // end ODBCMXTraceMsg::TraceFetch2Enter

//---------------------------------------------------------------------------------------
// Traces exit of odbc_SQLSvc_Fetch2_ame
void ODBCMXTraceMsg::TraceFetch2Exit( IDL_long  returnCode,
				      IDL_long  sqlWarningOrErrorLength,
				      BYTE     *sqlWarningOrError,
				      IDL_long  rowsAffected,
				      IDL_long  outValuesLength,
				      BYTE     *outValues)
  {
  int   length     = 0;
  int   tempStrlen = 0;
  char *pBuffer    = msgBuffer;
  seqNumber        = 0;

  sprintf(seqNumStr, "%d", seqNumber);
  strcpy(funcName, "ExitFetch2");

  CheckCollectorError();
		
  length   = sprintf(pBuffer, "ReturnCode:%d sqlWarningOrError:", returnCode); 
  pBuffer += length;
  if (sqlWarningOrErrorLength > 0) 
    pBuffer  = printHex(pBuffer, sqlWarningOrError, sqlWarningOrErrorLength);

  tempStrlen = sprintf(temp, "RowsAffected:%d OutValues:", rowsAffected);
  pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
  if (outValuesLength > 0)
     pBuffer = printHex(pBuffer, outValues, outValuesLength);

  SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
	       objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);

}  // end ODBCMXTraceMsg::TraceFetch2Exit

//---------------------------------------------------------------------------------------

// Traces entry of odbc_SQLSvc_PrepareRowset_ame
void ODBCMXTraceMsg::TracePrepareRowsetEnter(DIALOGUE_ID_def dialogueId,
					     const IDL_char *stmtLabel,
					     const IDL_char *stmtExplainLabel,
					     IDL_short	     stmtType,
					     IDL_short	     sqlStmtType,
					     IDL_string	     sqlString,
					     IDL_short	     sqlAsyncEnable,
					     IDL_long	     queryTimeout,
					     IDL_long	     maxRowCnt)
{
	int   length     = 0;
	int   tempStrlen = 0;
	int   availBuf   = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterPrepareRowset");

	CheckCollectorError();
	
	length   = sprintf(pBuffer,"DialogueId:%d StmtLabel:", dialogueId);
	pBuffer += length;
	pBuffer  = printLongString(pBuffer, (char*)stmtLabel);

	tempStrlen = sprintf(temp,"StmtExLabel:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)stmtExplainLabel);

	tempStrlen = sprintf(temp,"StmtType:%d SqlStmtType:%d SqlString:", stmtType, sqlStmtType);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)sqlString);

	tempStrlen = sprintf(temp,"SqlAsyncEnable:%d QueryTimeout:%d MaxRowCnt:%d", 
			     sqlAsyncEnable, queryTimeout, maxRowCnt);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_PrepareRowset_ame
void ODBCMXTraceMsg::TracePrepareRowsetExit(odbc_SQLSvc_PrepareRowset_exc_ exception, 
					    IDL_long			   estimatedCost, 
					    SQLItemDescList_def		   inputDesc, 
					    SQLItemDescList_def		   outputDesc, 
					    ERROR_DESC_LIST_def		   sqlWarning)
{
	int   length  = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitPrepareRowset");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ExceptionNr:%d ExceptionDetail:%d EstCost:%d ", 
			   exception.exception_nr, exception.exception_detail, estimatedCost); 
	pBuffer += length;
	pBuffer  = printSQLItemDesc(inputDesc,  pBuffer);
	pBuffer  = printSQLItemDesc(outputDesc, pBuffer);
	pBuffer  = printSQLWarning(sqlWarning, pBuffer);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces entry of odbc_SQLSvc_ExecDirectRowset_ame
void ODBCMXTraceMsg::TraceExecDirectRowsetEnter(DIALOGUE_ID_def dialogueId,
						const IDL_char *stmtLabel,
						IDL_string	cursorName,
						const IDL_char *stmtExplainLabel,
						IDL_short	stmtType,
						IDL_short	sqlStmtType,
						IDL_string	sqlString,
						IDL_short	sqlAsyncEnable,
						IDL_long	queryTimeout,
						IDL_long	maxRowCnt)
{
	int   length     = 0;
	int   tempStrlen = 0;
	int   availBuf   = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterExecDirectRowset");

	CheckCollectorError();
	
	length   = sprintf(pBuffer,"DialogueId:%d StmtLabel:", dialogueId);
	pBuffer += length;
	pBuffer  = printLongString(pBuffer, (char*)stmtLabel);
	
	tempStrlen = sprintf(temp,"CursorName:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)cursorName);

	tempStrlen = sprintf(temp,"StmtExLabel:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)stmtExplainLabel);

	tempStrlen = sprintf(temp,"StmtType:%d SqlStmtType:%d SqlStr:", stmtType, sqlStmtType);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer  = printLongString(pBuffer, (char*)sqlString);

	tempStrlen = sprintf(temp,"SqlAsyncEnable:%d QueryTimeout:%d MaxRowCnt:%d", 
				   sqlAsyncEnable, queryTimeout, maxRowCnt);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_ExecDirectRowset_ame
void ODBCMXTraceMsg::TraceExecDirectRowsetExit(odbc_SQLSvc_ExecDirectRowset_exc_ exception,
					       IDL_long				 estimatedCost, 
					       SQLItemDescList_def		 outputDesc, 
					       IDL_long				 rowsAffected,
					       ERROR_DESC_LIST_def		 sqlWarning)
{
	int   length     = 0;
	int   tempStrlen = 0;
	int   availBuf   = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitExecDirectRowset");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ExceptionNr:%d ExceptionDetail:%d EstCost:%d ", 
			   exception.exception_nr, exception.exception_detail, estimatedCost); 
	pBuffer += length;
	pBuffer  = printSQLItemDesc(outputDesc, pBuffer);
	
	tempStrlen = sprintf(temp,"RowsAffected:%d ", rowsAffected);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer    = printSQLWarning(sqlWarning, pBuffer);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces entry of odbc_SQLSvc_ExecuteRowset_ame
void ODBCMXTraceMsg::TraceExecuteRowsetEnter(DIALOGUE_ID_def	      dialogueId,
					     const IDL_char	     *stmtLabel,
					     IDL_string		      cursorName,
					     IDL_short		      sqlStmtType,
					     IDL_long		      inputRowCnt,
					     const SQL_DataValue_def *inputDataValue,
					     IDL_short		      sqlAsyncEnable,
					     IDL_long		      queryTimeout)
{
	int                length     = 0;
	int                tempStrlen = 0;
	int                availBuf   = 0;
	char              *pBuffer    = msgBuffer;
	seqNumber                     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterExecuteRowset");

	CheckCollectorError();
	
	length   = sprintf(pBuffer,"DialogueId:%d StmtLabel:", dialogueId);
	pBuffer += length;
	pBuffer  = printLongString(pBuffer, (char*)stmtLabel);

	tempStrlen = sprintf(temp,"CursorName:");
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	pBuffer    = printLongString(pBuffer, (char*)cursorName);

	tempStrlen = sprintf(temp,"SqlStmtType:%d InputRowCnt:%d InputDataVal:", sqlStmtType, inputRowCnt);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);

	pBuffer    = printHex(pBuffer, inputDataValue->_buffer, inputDataValue->_length);

	tempStrlen = sprintf(temp," SqlAsyncEnable:%d QueryTimeout:%d", sqlAsyncEnable, queryTimeout);
	pBuffer    = writeTraceMsg(pBuffer, temp, tempStrlen);
	
	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of odbc_SQLSvc_ExecuteRowset_ame
void ODBCMXTraceMsg::TraceExecuteRowsetExit(odbc_SQLSvc_ExecuteRowset_exc_ exception,
					    IDL_long			   rowsAffected, 
					    ERROR_DESC_LIST_def		   sqlWarning)
{
	int   length  = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitExecuteRowset");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "ExceptionNr:%d ExceptionDetail:%d RowsAffected:%d ", 
			   exception.exception_nr, exception.exception_detail, rowsAffected); 
	pBuffer += length;
	pBuffer  = printSQLWarning(sqlWarning, pBuffer);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}
//LCOV_EXCL_STOP
// Traces enter of ImplInit
void ODBCMXTraceMsg::TraceImplInitEnter(SRVR_INIT_PARAM_Def *initParam, long initParamLen)
{
	int   length  = 0;
	char *pBuffer = msgBuffer;
	seqNumber     = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "EnterImplInit");

	CheckCollectorError();
		
	length   = sprintf(pBuffer, "DebugFlag:%d EventFlag:%d ASObjRef:%s ", 
			   initParam->debugFlag, initParam->eventFlag, initParam->asSrvrObjRef);
	pBuffer += length;

	length  = sprintf(temp,"SrvrType:%d StartType:%d ", initParam->srvrType, initParam->startType);
	pBuffer = writeTraceMsg(pBuffer, temp, length);

	length  = sprintf(temp,"NumServers:%d CfgSrvrTimeout:%d ", initParam->noOfServers, initParam->cfgSrvrTimeout);
	pBuffer = writeTraceMsg(pBuffer, temp, length);
	
	length  = sprintf(temp,"DSID:%d Transport:%d ", initParam->DSId, initParam->transport);
	pBuffer = writeTraceMsg(pBuffer, temp, length);

	length  = sprintf(temp,"PortNum:%d IpPortRange:%d ", initParam->portNumber, initParam->IpPortRange);
	pBuffer = writeTraceMsg(pBuffer, temp, length);

	length  = sprintf(temp,"TcpProcess:%.80s ASProcess:%.80s ", initParam->TcpProcessName, initParam->ASProcessName);
	pBuffer = writeTraceMsg(pBuffer, temp, length);

	length  = sprintf(temp,"EMSName:%s EmsTimeout:%d ", initParam->EmsName, initParam->EmsTimeout);
	pBuffer = writeTraceMsg(pBuffer, temp, length);

	length  = sprintf(temp,"TraceCollector:%s RSCollector:%s ", initParam->TraceCollector, initParam->RSCollector);
	pBuffer = writeTraceMsg(pBuffer, temp, length);

	length  = sprintf(temp,"InitIncSrvr:%d InitIncTime:%d DSG:%d SrvrTrace:%d ", 
			  initParam->initIncSrvr, initParam->initIncTime, initParam->DSG, initParam->srvrTrace);
	pBuffer = writeTraceMsg(pBuffer, temp, length);
	length  = sprintf(temp,"MajorVersion:%d ", initParam->majorVersion);
	pBuffer = writeTraceMsg(pBuffer, temp, length);

	length  = sprintf(temp,"DSName:%s", initParam->DSName);
	pBuffer = writeTraceMsg(pBuffer, temp, length);

	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Traces exit of ImplInit
void ODBCMXTraceMsg::TraceImplInitExit(CEE_status returnSts) 
{
	seqNumber = 0;
	sprintf(seqNumStr, "%d", seqNumber);
	strcpy(funcName, "ExitImplInit");

	CheckCollectorError();

	sprintf(msgBuffer, "ReturnStatus:%d", returnSts);
	SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, ODBCMX_SERVICE, 
		     objectRef, 4, srvrGlobal->sessionId, funcName, seqNumStr, msgBuffer);
}

// Prints long string
char *ODBCMXTraceMsg::printLongString(char *buffer,
				      char *longStr)
{
	int length  = 0;
	int size    = 0;
	int strSize = 0;
	char *p;
	
	if (longStr==NULL) return buffer;

	size    = MAX_MSG_LENGTH-(buffer-msgBuffer);
	strSize = strlen(longStr);
	p       = longStr;

	if(strSize > size)
	{	
		while (strSize > size)
		{
			strncpy(buffer, p, size);
			buffer      += size;
			p           += size;
			strSize     -= size;
			SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, 
			             ODBCMX_SERVICE, objectRef, 4, srvrGlobal->sessionId, 
				     funcName, seqNumStr, msgBuffer);
			buffer = msgBuffer;
			seqNumber++;
			sprintf(seqNumStr, "%d", seqNumber);
			size   = MAX_MSG_LENGTH;
		}
	}
	length  = sprintf(buffer,"%s ", p);
	buffer += length;

	return buffer;
}

// Prints USER_DESC_def structure
char *ODBCMXTraceMsg::printUSER_DESC_def(const USER_DESC_def *value, 
				         char		     *buffer)
{
	int length = 0;

	length  = sprintf(buffer,"UserDescType:%d UserName:", value->userDescType);
	buffer += length;
	buffer  = printLongString(buffer, value->userName);
	
	return buffer;
}

// Prints CONNECTION_CONTEXT_def structure
char *ODBCMXTraceMsg::printCONNECTION_CONTEXT_def(const CONNECTION_CONTEXT_def *value,
						  char			       *buffer)
{
	int   length     = 0;
	int   tempStrlen = 0;
	int   availBuf   = 0;
	VERSION_def *clientVersionPtr;

	tempStrlen = sprintf(temp, "Datasource:");
	buffer     = writeTraceMsg(buffer, temp, tempStrlen);
	buffer     = printLongString(buffer, (char*)value->datasource);

	tempStrlen = sprintf(temp, "Catalog:");
	buffer     = writeTraceMsg(buffer, temp, tempStrlen);
	buffer     = printLongString(buffer, (char*)value->catalog);

	tempStrlen = sprintf(temp, "Schema:");
	buffer     = writeTraceMsg(buffer, temp, tempStrlen);
	buffer     = printLongString(buffer, (char*)value->schema);

	tempStrlen = sprintf(temp, "AccessMode:%d ", value->accessMode);
	buffer     = writeTraceMsg(buffer, temp, tempStrlen);

	tempStrlen = sprintf(temp, "AutoCommit:%d ", value->autoCommit);
	buffer     = writeTraceMsg(buffer, temp, tempStrlen);

	tempStrlen = sprintf(temp, "IsolationLevel:%d ", value->txnIsolationLevel);
	buffer     = writeTraceMsg(buffer, temp, tempStrlen);

	tempStrlen = sprintf(temp, "RowSetSize:%d ", value->rowSetSize);
	buffer     = writeTraceMsg(buffer, temp, tempStrlen);

	for (int i=0; i<value->clientVersionList._length; i++) 
	{
		clientVersionPtr = value->clientVersionList._buffer + i;

		tempStrlen = sprintf(temp, "Component%d:%d ", i, clientVersionPtr->componentId);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "Major%d:%d ", i, clientVersionPtr->majorVersion);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "Minor%d:%d ", i, clientVersionPtr->minorVersion);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "Build%d:%d ", i, clientVersionPtr->buildId);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);
	}

	return buffer;
}

// Prints ERROR_DESC_LIST_def structure
char *ODBCMXTraceMsg::printSQLWarning (ERROR_DESC_LIST_def  sqlWarning,
				       char		   *buffer)
{
	int   length     = 0;
	int   tempStrlen = 0;
	int   availBuf   = 0;
	ERROR_DESC_def *error;

	for (int i=0; i<sqlWarning._length; i++)
	{
		error = sqlWarning._buffer + i;

		if (error == NULL) break;			

		tempStrlen = sprintf(temp, "RowId%d:%d ", i, error->rowId);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "ErrDiagId%d:%d ", i, error->errorDiagnosticId);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "SqlCode%d:%d ", i, error->sqlcode);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "SqlState%d:%s ErrText%d:", i, error->sqlstate, i);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);
		buffer     = printLongString(buffer, error->errorText);
		
		tempStrlen = sprintf(temp, "OpAbortId%d:%d ", i, error->operationAbortId);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "ErrCodeType%d:%d ", i, error->errorCodeType);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);
	}

	return buffer;
}

// prints SQLItemDescList_def structure
char *ODBCMXTraceMsg::printSQLItemDesc(SQLItemDescList_def  list,
				       char		   *buffer)
{
	int   length     = 0;
	int   tempStrlen = 0;
	int   availBuf   = 0;
	SQLItemDesc_def *item;

	for (int i=0; i<list._length; i++)
	{
		item = list._buffer + i;

		tempStrlen = sprintf(temp, "Version%d:%d DataType%d:%d ", i, item->version, i, item->dataType);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "DateTimeCode%d:%d MaxLen%d:%d ", i, item->datetimeCode, i, item->maxLen);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "Precision%d:%d Scale%d:%d ", i, item->precision, i, item->scale);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "NullInfo%d:%d SignType%d:%d ", i, item->nullInfo, i, item->signType);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "ColHeadingNm%d:%s ", i, item->colHeadingNm);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "ODBCDataType%d:%d ODBCPrecision%d:%d ", i, item->ODBCDataType, i, item->ODBCPrecision);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "SQLCharset%d:%d ODBCCharset%d:%d ", i, item->SQLCharset, i, item->ODBCCharset);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "TableName%d:%s ", i, item->TableName);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "CatalogName%d:%s ", i, item->CatalogName);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "SchemaName%d:%s ", i, item->SchemaName);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);

		tempStrlen = sprintf(temp, "Heading%d:%s ", i, item->Heading);
		buffer     = writeTraceMsg(buffer, temp, tempStrlen);
	}

	return buffer;
}

// prints data in hex
char *ODBCMXTraceMsg::hexOut(char *buffer, unsigned char *data, int size)
{
	int length = 0;

	for (int i=0; i<size; i++)
	{
		length  = sprintf(buffer, "%02X", data[i]);
		buffer += length;
	}

	length  = sprintf(buffer, "    ");
	buffer += length;

	for (int j=0; j<size; j++)
	{
		length  = sprintf(buffer, "%c", charTable[*data++]);
		buffer += length;
	}

	return buffer;
}

// prints data in hex
char *ODBCMXTraceMsg::printHex(char *buffer, unsigned char *data, int strSize) 
{
	int length = 0;
	int size   = 0;
	unsigned char *p;

	size = MAX_MSG_LENGTH-(buffer-msgBuffer);
	p    = data;

	if(strSize > size/4)
	{	
		while (strSize > size/4)
		{
			buffer = hexOut(buffer, p, size/4);
			p           += size;
			strSize     -= size;
			SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, 
			             ODBCMX_SERVICE, objectRef, 4, srvrGlobal->sessionId, 
				     funcName, seqNumStr, msgBuffer);
			buffer = msgBuffer;
			seqNumber++;
			sprintf(seqNumStr, "%d", seqNumber);
			size   = MAX_MSG_LENGTH-(buffer-msgBuffer);
		}
	}
	buffer = hexOut(buffer, p, strSize);

	return buffer;
}

// writes trace message to msgBuffer
char *ODBCMXTraceMsg::writeTraceMsg(char *buffer, char *data, int dataSize)
{
	int length   = 0;
	int availBuf = 0;

	availBuf   = MAX_MSG_LENGTH-(buffer-msgBuffer);
  	if(dataSize > availBuf)
	{
		SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE, pid, 
			     ODBCMX_SERVICE, objectRef, 4, srvrGlobal->sessionId, 
			     funcName, seqNumStr, msgBuffer);
		buffer = msgBuffer;
		seqNumber++;
		sprintf(seqNumStr, "%d", seqNumber);
	}
	length   = sprintf(buffer, "%s", data);
	buffer  += length;

	return buffer;
}

void ODBCMXTraceMsg::CheckCollectorError()
{
	char  errorMsg[100];
	char  errorStr[10];
	short retems = 0;
}
