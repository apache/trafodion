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
//
//
#include "DrvrGlobal.h"
#include "SQLHandle.h"
#include "SQLConnect.h"
#include "SQLEnv.h"
#include "SQLStmt.h"
#include "SQLDesc.h"
#include "DiagFunctions.h"
#include "DrvrSrvr.h"
#include "charsetconv.h"
#include "tdm_odbcDrvMsg.h"
//
#include <Math.h>
//

using namespace ODBC;

#define SEH

#define TRACE_RETURN(retHandle, rc)\
if (retHandle)\
{\
	if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))\
	{\
		if (fpTracePrintMarker)\
			(fpTracePrintMarker)();\
		if (fpTraceReturn)\
			(fpTraceReturn)(retHandle, rc);\
	}\
}

SQLRETURN  SQL_API SQLAllocHandle(SQLSMALLINT HandleType,
				SQLHANDLE InputHandle, 
				SQLHANDLE *OutputHandle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLAllocHandle)
				retHandle = (fpTraceSQLAllocHandle)(HandleType, InputHandle, OutputHandle);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = AllocHandle(HandleType, InputHandle, OutputHandle);
#else
	__try{
		rc = AllocHandle(HandleType, InputHandle, OutputHandle);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (InputHandle) ((CHandle*)InputHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}


SQLRETURN  SQL_API SQLFreeHandle(SQLSMALLINT HandleType, 
				SQLHANDLE Handle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLFreeHandle)
				retHandle = (fpTraceSQLFreeHandle)(HandleType, Handle);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = FreeHandle(HandleType, Handle);
#else
	__try{
		rc = FreeHandle(HandleType, Handle);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (Handle)	((CHandle*)Handle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API SQLEndTran(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT CompletionType)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLEndTran)
				retHandle = (fpTraceSQLEndTran)(HandleType, Handle, CompletionType);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = EndTran(HandleType, Handle, CompletionType);
#else
	__try{
		rc = EndTran(HandleType, Handle, CompletionType);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (Handle) ((CHandle*)Handle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API SQLDisconnect(SQLHDBC ConnectionHandle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLDisconnect)
				retHandle = (fpTraceSQLDisconnect)(ConnectionHandle);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = Disconnect(ConnectionHandle);
#else
	__try{
		rc = Disconnect(ConnectionHandle);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (ConnectionHandle) ((CHandle*)ConnectionHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}
 
SQLRETURN  SQL_API SQLSetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLSetEnvAttr)
				retHandle = (fpTraceSQLSetEnvAttr)(EnvironmentHandle, Attribute, Value, StringLength);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = SetEnvAttr(EnvironmentHandle, Attribute, Value, StringLength);
#else
	__try{
		rc = SetEnvAttr(EnvironmentHandle, Attribute, Value, StringLength);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (EnvironmentHandle) ((CHandle*)EnvironmentHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLGetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetEnvAttr)
				retHandle = (fpTraceSQLGetEnvAttr)(EnvironmentHandle, Attribute, Value, BufferLength, StringLength);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetEnvAttr(EnvironmentHandle, Attribute, Value, BufferLength, StringLength);
#else
	__try{
		rc = GetEnvAttr(EnvironmentHandle, Attribute, Value, BufferLength, StringLength);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (EnvironmentHandle) ((CHandle*)EnvironmentHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLBindCol(SQLHSTMT StatementHandle, 
		   SQLUSMALLINT ColumnNumber, 
		   SQLSMALLINT TargetType, 
		   SQLPOINTER TargetValue, 
		   SQLLEN BufferLength, 
	   	   SQLLEN *StrLen_or_IndPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLBindCol)
				retHandle = (fpTraceSQLBindCol)(StatementHandle, ColumnNumber, TargetType, 
							TargetValue, BufferLength, StrLen_or_IndPtr);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = BindCol(StatementHandle, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_IndPtr);
#else
	__try{
		rc = BindCol(StatementHandle, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_IndPtr);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
	if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API SQLBindParameter(
			SQLHSTMT StatementHandle,
			SQLUSMALLINT ParameterNumber, 
			SQLSMALLINT InputOutputType,
			SQLSMALLINT ValueType,
			SQLSMALLINT ParameterType, 
			SQLULEN		ColumnSize,
			SQLSMALLINT DecimalDigits,
			SQLPOINTER  ParameterValuePtr,
			SQLLEN		BufferLength,
			SQLLEN *StrLen_or_IndPtr)		   
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLBindParameter)
				retHandle = (fpTraceSQLBindParameter)(StatementHandle, ParameterNumber, InputOutputType, ValueType, ParameterType,
				ColumnSize, DecimalDigits,ParameterValuePtr, BufferLength, StrLen_or_IndPtr);
		}
		HexOut(TR_ODBC_API, StrLen_or_IndPtr, ParameterValuePtr, "ParameterValuePtr");
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = BindParameter(StatementHandle, ParameterNumber, InputOutputType, ValueType, ParameterType,
				ColumnSize, DecimalDigits,ParameterValuePtr, BufferLength, StrLen_or_IndPtr);
#else
	__try{
		rc = BindParameter(StatementHandle, ParameterNumber, InputOutputType, ValueType, ParameterType,
				ColumnSize, DecimalDigits,ParameterValuePtr, BufferLength, StrLen_or_IndPtr);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLNumResultCols(SQLHSTMT StatementHandle,
           SQLSMALLINT *ColumnCountPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLNumResultCols)
				retHandle = (fpTraceSQLNumResultCols)(StatementHandle, ColumnCountPtr);		
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = getDescSize(StatementHandle, SQL_API_SQLNUMRESULTCOLS, ColumnCountPtr);
#else
	__try{
		rc = getDescSize(StatementHandle, SQL_API_SQLNUMRESULTCOLS, ColumnCountPtr);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLNumParams(SQLHSTMT StatementHandle,
           SQLSMALLINT *ParameterCountPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLNumParams)
				retHandle = (fpTraceSQLNumParams)(StatementHandle, ParameterCountPtr);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = getDescSize(StatementHandle, SQL_API_SQLNUMPARAMS, ParameterCountPtr);
#else
	__try{
		rc = getDescSize(StatementHandle, SQL_API_SQLNUMPARAMS, ParameterCountPtr);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API SQLDescribeParam(
	SQLHSTMT           StatementHandle,
	SQLUSMALLINT       ParameterNumber,
    SQLSMALLINT 	  *DataTypePtr,
    SQLULEN      	  *ParameterSizePtr,
    SQLSMALLINT 	  *DecimalDigitsPtr,
    SQLSMALLINT 	  *NullablePtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLDescribeParam)
				retHandle = (fpTraceSQLDescribeParam)(StatementHandle, ParameterNumber, DataTypePtr,
						ParameterSizePtr, DecimalDigitsPtr, NullablePtr);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = getDescRec(StatementHandle, SQL_API_SQLDESCRIBEPARAM, ParameterNumber, NULL, 0, NULL,
			DataTypePtr, ParameterSizePtr, DecimalDigitsPtr, NullablePtr);
#else
	__try{
		rc = getDescRec(StatementHandle, SQL_API_SQLDESCRIBEPARAM, ParameterNumber, NULL, 0, NULL,
			DataTypePtr, ParameterSizePtr, DecimalDigitsPtr, NullablePtr);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;

}

SQLRETURN SQL_API SQLFreeStmt(SQLHSTMT StatementHandle,
        SQLUSMALLINT Option)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLFreeStmt)
				retHandle = (fpTraceSQLFreeStmt)(StatementHandle, Option);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = FreeStmt(StatementHandle, SQL_API_SQLFREESTMT, Option);
#else
	__try{
		rc = FreeStmt(StatementHandle, SQL_API_SQLFREESTMT, Option);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}


SQLRETURN SQL_API SQLCloseCursor(SQLHSTMT StatementHandle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLCloseCursor)
				retHandle = (fpTraceSQLCloseCursor)(StatementHandle);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = FreeStmt(StatementHandle, SQL_API_SQLCLOSECURSOR, SQL_CLOSE);
#else
	__try{
		rc = FreeStmt(StatementHandle, SQL_API_SQLCLOSECURSOR, SQL_CLOSE);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLGetTypeInfoW(SQLHSTMT StatementHandle,
           SQLSMALLINT DataType)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetTypeInfo)
				retHandle = (fpTraceSQLGetTypeInfo)(StatementHandle, DataType);
		}
	}
	else
		RESET_TRACE(); 
#ifndef SEH
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLGETTYPEINFO, NULL, 0, 
				NULL, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, 0, DataType);
#else
	__try{
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLGETTYPEINFO, NULL, 0, 
				NULL, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, 0, DataType);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLRowCount(
	   SQLHSTMT StatementHandle, 
	   SQLLEN	*RowCountPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLRowCount)
				retHandle = (fpTraceSQLRowCount)(StatementHandle, RowCountPtr);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = RowCount(StatementHandle, RowCountPtr);
#else
	__try{
		rc = RowCount(StatementHandle, RowCountPtr);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}


SQLRETURN SQL_API SQLMoreResults(
    SQLHSTMT           StatementHandle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLMoreResults)
				retHandle = (fpTraceSQLMoreResults)(StatementHandle);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = FreeStmt(StatementHandle, SQL_API_SQLMORERESULTS, SQL_CLOSE);
#else
	__try{
		rc = FreeStmt(StatementHandle, SQL_API_SQLMORERESULTS, SQL_CLOSE);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API SQLCancel(SQLHSTMT StatementHandle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLCancel)
				retHandle = (fpTraceSQLCancel)(StatementHandle);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = Cancel(StatementHandle);
#else
	__try{
		rc = Cancel(StatementHandle);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API SQLExecute(SQLHSTMT StatementHandle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLExecute)
				retHandle = (fpTraceSQLExecute)(StatementHandle);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = Execute(StatementHandle);
#else
	__try{
		rc = Execute(StatementHandle);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLParamData(SQLHSTMT StatementHandle,
           SQLPOINTER *ValuePtrPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLParamData)
				retHandle = (fpTraceSQLParamData)(StatementHandle, ValuePtrPtr);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = ParamData(StatementHandle, ValuePtrPtr);
#else
	__try{
		rc = ParamData(StatementHandle, ValuePtrPtr);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLPutData(
		   SQLHSTMT StatementHandle,
           SQLPOINTER DataPtr, 
		   SQLLEN StrLen_or_Ind)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLPutData)
				retHandle = (fpTraceSQLPutData)(StatementHandle, DataPtr, StrLen_or_Ind);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = PutData(StatementHandle, DataPtr, StrLen_or_Ind);
#else
	__try{
		rc = PutData(StatementHandle, DataPtr, StrLen_or_Ind);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API SQLFetch(SQLHSTMT StatementHandle)
{

	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLFetch)
				retHandle = (fpTraceSQLFetch)(StatementHandle);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = Fetch(StatementHandle);
#else
	__try{
		rc = Fetch(StatementHandle);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API SQLExtendedFetch(
			SQLHSTMT StatementHandle,
			SQLUSMALLINT FetchOrientation,
			SQLLEN FetchOffset,
			SQLULEN* RowCountPtr,
			SQLUSMALLINT* RowStatusArray)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLFetch)
				retHandle = (fpTraceSQLExtendedFetch)(StatementHandle,FetchOrientation,
						FetchOffset,RowCountPtr,RowStatusArray);
		}
	}
	else
		RESET_TRACE();
	rc = ExtendedFetch(StatementHandle,FetchOrientation,FetchOffset,RowCountPtr,RowStatusArray);
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API SQLFetchScroll(
			SQLHSTMT StatementHandle,
			SQLSMALLINT FetchOrientation,
			SQLLEN FetchOffset)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLFetch)
				retHandle = (fpTraceSQLFetchScroll)(StatementHandle,FetchOrientation,FetchOffset);
		}
	}
	else
		RESET_TRACE();

	rc = FetchScroll(StatementHandle,FetchOrientation,FetchOffset);
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API SQLGetData(
		   SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLSMALLINT TargetType,
           SQLPOINTER TargetValuePtr, 
		   SQLLEN BufferLength,
           SQLLEN *StrLen_or_IndPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetData)
				retHandle = (fpTraceSQLGetData)(StatementHandle, ColumnNumber, TargetType, 
						TargetValuePtr, BufferLength, StrLen_or_IndPtr);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetData(StatementHandle, ColumnNumber, TargetType, TargetValuePtr, BufferLength, 
							StrLen_or_IndPtr);
#else
	__try{
		rc = GetData(StatementHandle, ColumnNumber, TargetType, TargetValuePtr, BufferLength, 
							StrLen_or_IndPtr);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API SQLSetPos(
    SQLHSTMT        StatementHandle,
    SQLSETPOSIROW	RowNumber,
    SQLUSMALLINT    Operation,
    SQLUSMALLINT    LockType)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLSetPos)
				retHandle = (fpTraceSQLSetPos)(StatementHandle, RowNumber, Operation, LockType);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = SetPos(StatementHandle, RowNumber, Operation, LockType);
#else
	__try{
		rc = SetPos(StatementHandle, RowNumber, Operation, LockType);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API SQLCopyDesc(SQLHDESC SourceDescHandle,
		SQLHDESC TargetDescHandle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLCopyDesc)
				retHandle = (fpTraceSQLCopyDesc)(SourceDescHandle, TargetDescHandle);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = CopyDesc(SourceDescHandle, TargetDescHandle);
#else
	__try{
		rc = CopyDesc(SourceDescHandle, TargetDescHandle);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLSetDescRec(
		   SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber,
		   SQLSMALLINT Type,
           SQLSMALLINT SubType, 
		   SQLLEN Length,
           SQLSMALLINT Precision, 
		   SQLSMALLINT Scale,
           SQLPOINTER Data, 
		   SQLLEN *StringLengthPtr,
           SQLLEN *IndicatorPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLSetDescRec)
				retHandle = (fpTraceSQLSetDescRec)(DescriptorHandle, RecNumber, Type, SubType, Length, 
							Precision, Scale, Data, StringLengthPtr, IndicatorPtr);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = SetDescRec(DescriptorHandle, RecNumber, Type, SubType, Length, Precision, Scale, Data,
			StringLengthPtr, IndicatorPtr);
#else
	__try{
		rc = SetDescRec(DescriptorHandle, RecNumber, Type, SubType, Length, Precision, Scale, Data,
			StringLengthPtr, IndicatorPtr);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (DescriptorHandle) ((CHandle*)DescriptorHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}


/*============================================================================================================*/
/* WIDE FUNCTION CALLS for UNICODE Support                                                                                        */
/*============================================================================================================*/

SQLRETURN SQL_API SQLGetDiagRecW(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber,
				SQLWCHAR *Sqlstate,
				SQLINTEGER *NativeError, 
				SQLWCHAR *MessageText,
				SQLSMALLINT BufferLength, 
				SQLSMALLINT *TextLength)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetDiagRecW)
				retHandle = (fpTraceSQLGetDiagRecW)(HandleType, Handle, RecNumber, Sqlstate,
							NativeError, MessageText, BufferLength, TextLength);
		}
	}
	else
		RESET_TRACE();

#ifndef SEH
		rc = GetDiagRec(HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength,
			TextLength);
#else
	__try{
		rc = GetDiagRec(HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength,
			TextLength);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (Handle) ((CHandle*)Handle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif

	TRACE_RETURN(retHandle, rc);

	return rc;

}

SQLRETURN SQL_API SQLGetDiagFieldW(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber, 
				SQLSMALLINT DiagIdentifier,
				SQLPOINTER DiagInfo, 
				SQLSMALLINT BufferLength,
				SQLSMALLINT *StringLength)
{	
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetDiagFieldW)
				retHandle = (fpTraceSQLGetDiagFieldW)(HandleType, Handle, RecNumber, DiagIdentifier,
							DiagInfo, BufferLength, StringLength);
		}
	}
	else
		RESET_TRACE();

#ifndef SEH
		rc = GetDiagField(HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);
#else
	__try{
		rc = GetDiagField(HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (Handle) ((CHandle*)Handle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API SQLConnectW(SQLHDBC ConnectionHandle,
           SQLWCHAR *ServerNameW, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *UserNameW, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *AuthenticationW, 
		   SQLSMALLINT NameLength3)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	SQLCHAR ServerName[MAX_SQL_IDENTIFIER_LEN+1];
	SQLCHAR UserName[MAX_SQL_IDENTIFIER_LEN+1];
	SQLCHAR Authentication[3*MAX_SQL_IDENTIFIER_LEN+3];
	int  TransStringLength1 = 0;
	int  TransStringLength2 = 0;
	int  TransStringLength3 = 0;
	char error[64];

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLConnectW)
				retHandle = (fpTraceSQLConnectW)(ConnectionHandle, ServerNameW, NameLength1, UserNameW, 
						NameLength2, AuthenticationW, NameLength3);
		}
	}
	else
		RESET_TRACE();

	if(WCharToUTF8(ServerNameW, NameLength1, (char*)ServerName, sizeof(ServerName), &TransStringLength1, error) != SQL_SUCCESS)
	{
		((CHandle*)ConnectionHandle)->setWcharConvError(error); 
		return SQL_ERROR;
	}
	if(WCharToUTF8(UserNameW, NameLength2, (char*)UserName, sizeof(UserName), &TransStringLength2, error) != SQL_SUCCESS)
	{
		((CHandle*)ConnectionHandle)->setWcharConvError(error); 
		return SQL_ERROR;
	}
	if(WCharToUTF8(AuthenticationW, NameLength3, (char*)Authentication, sizeof(Authentication), &TransStringLength3, error) != SQL_SUCCESS)
	{
		((CHandle*)ConnectionHandle)->setWcharConvError(error); 
		return SQL_ERROR;
	}
	
#ifndef SEH
		rc = Connect(ConnectionHandle, ServerName, TransStringLength1, UserName, TransStringLength2, 
				Authentication, TransStringLength3);
#else
	__try{
		rc = Connect(ConnectionHandle, ServerName, TransStringLength1, UserName, TransStringLength2, 
				Authentication, TransStringLength3);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (ConnectionHandle) ((CHandle*)ConnectionHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLSetConnectAttrW(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLSetConnectAttrW)
				retHandle = (fpTraceSQLSetConnectAttrW)(ConnectionHandle, Attribute, Value, StringLength);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = SetConnectAttr(ConnectionHandle, Attribute, Value, StringLength);
#else
	__try{
		rc = SetConnectAttr(ConnectionHandle, Attribute, Value, StringLength);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (ConnectionHandle) ((CHandle*)ConnectionHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLGetConnectAttrW(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetConnectAttrW)
				retHandle = (fpTraceSQLGetConnectAttrW)(ConnectionHandle, Attribute, Value, BufferLength,
							StringLength);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetConnectAttr(ConnectionHandle, Attribute, Value, BufferLength, StringLength);
#else
	__try{
		rc = GetConnectAttr(ConnectionHandle, Attribute, Value, BufferLength, StringLength);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (ConnectionHandle) ((CHandle*)ConnectionHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLSetStmtAttrW(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLSetStmtAttrW)
				retHandle = (fpTraceSQLSetStmtAttrW)(StatementHandle, Attribute, Value, StringLength);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = SetStmtAttr(StatementHandle, Attribute, Value, StringLength);
#else
	__try{
		rc = SetStmtAttr(StatementHandle, Attribute, Value, StringLength);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;

}

SQLRETURN  SQL_API SQLGetStmtAttrW(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetStmtAttrW)
				retHandle = (fpTraceSQLGetStmtAttrW)(StatementHandle, Attribute, Value, BufferLength, 
				StringLength);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetStmtAttr(StatementHandle, Attribute, Value, BufferLength, StringLength);
#else
	__try{
		rc = GetStmtAttr(StatementHandle, Attribute, Value, BufferLength, StringLength);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLGetInfoW(SQLHDBC ConnectionHandle,
           SQLUSMALLINT InfoType, 
		   SQLPOINTER InfoValuePtr,
           SQLSMALLINT BufferLength,
		   SQLSMALLINT *StringLengthPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetInfoW)
				retHandle = (fpTraceSQLGetInfoW)(ConnectionHandle, InfoType, InfoValuePtr, BufferLength, StringLengthPtr);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetInfo(ConnectionHandle, InfoType, InfoValuePtr, BufferLength, StringLengthPtr);
#else
	__try{
		rc = GetInfo(ConnectionHandle, InfoType, InfoValuePtr, BufferLength, StringLengthPtr);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (ConnectionHandle) ((CHandle*)ConnectionHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}


SQLRETURN  SQL_API SQLSetDescFieldW(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLSetDescFieldW)
				retHandle = (fpTraceSQLSetDescFieldW)(DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = SetDescField(DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength);
#else
	__try{
		rc = SetDescField(DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (DescriptorHandle) ((CHandle*)DescriptorHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLGetDescFieldW(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength,
           SQLINTEGER *StringLengthPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetDescFieldW)
				retHandle = (fpTraceSQLGetDescFieldW)(DescriptorHandle, RecNumber, FieldIdentifier,
						ValuePtr, BufferLength, StringLengthPtr);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetDescField(DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength, 
					StringLengthPtr);
#else
	__try{
		rc = GetDescField(DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength, 
					StringLengthPtr);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (DescriptorHandle) ((CHandle*)DescriptorHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}


SQLRETURN  SQL_API SQLGetDescRecW(
		   SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLWCHAR *NameW,
           SQLSMALLINT BufferLength, 
		   SQLSMALLINT *StringLengthPtr,
           SQLSMALLINT *TypePtr, 
		   SQLSMALLINT *SubTypePtr, 
           SQLLEN     *LengthPtr, 
		   SQLSMALLINT *PrecisionPtr, 
           SQLSMALLINT *ScalePtr, 
		   SQLSMALLINT *NullablePtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetDescRecW)
				retHandle = (fpTraceSQLGetDescRecW)(DescriptorHandle, RecNumber, NameW, BufferLength, StringLengthPtr, TypePtr, 
						SubTypePtr, LengthPtr, PrecisionPtr, ScalePtr, NullablePtr);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetDescRec(DescriptorHandle, RecNumber, Name, BufferLength, StringLengthPtr, TypePtr, 
				SubTypePtr, LengthPtr, PrecisionPtr, ScalePtr, NullablePtr);
#else
	__try{
		rc = GetDescRec(DescriptorHandle, RecNumber, NameW, BufferLength, StringLengthPtr, TypePtr, 
				SubTypePtr, LengthPtr, PrecisionPtr, ScalePtr, NullablePtr);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (DescriptorHandle) ((CHandle*)DescriptorHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API SQLBrowseConnectW(
    SQLHDBC            ConnectionHandle,
    SQLWCHAR 		  *InConnectionStringW,
    SQLSMALLINT        StringLength1,
    SQLWCHAR 		  *OutConnectionStringW,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT       *StringLength2Ptr)
{
	SQLRETURN	rc, rc1;
	RETCODE		retHandle = 0;

	SQLCHAR InConnectionString[2048];
	SQLCHAR OutConnectionString[2048];
	SQLCHAR tmpOutConnectionString[2048];
	int  TransStringLength1 = 0;
	int  TransStringLength2 = 0;
	char error[64];

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLBrowseConnectW)
				retHandle = (fpTraceSQLBrowseConnectW)(ConnectionHandle, InConnectionStringW, StringLength1,
					OutConnectionStringW, BufferLength, StringLength2Ptr);
		}
	}
	else
		RESET_TRACE();

	if(WCharToUTF8(InConnectionStringW, StringLength1, (char*)InConnectionString, sizeof(InConnectionString), &TransStringLength1, error) != SQL_SUCCESS)
	{
		((CHandle*)ConnectionHandle)->setWcharConvError(error); 
		return SQL_ERROR;
	}

#ifndef SEH
		rc = BrowseConnect(ConnectionHandle, InConnectionString, TransStringLength1,
					OutConnectionString, BufferLength, StringLength2Ptr);
#else
	__try{
		rc = BrowseConnect(ConnectionHandle, InConnectionString, TransStringLength1,
					OutConnectionString, sizeof(OutConnectionString), StringLength2Ptr);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (ConnectionHandle) ((CHandle*)ConnectionHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);

	if(rc != SQL_ERROR)
	{
		if((rc1 = UTF8ToWChar((char*)OutConnectionString, strlen((const char*)OutConnectionString), (wchar_t *)tmpOutConnectionString, 
							  sizeof(tmpOutConnectionString)/2, &TransStringLength2, error)) != SQL_SUCCESS)
			((CHandle*)ConnectionHandle)->setWcharConvError(error); 
		if (OutConnectionStringW != NULL)
		{
			//check whether we had been given enough buffer (BufferLength)
			if( TransStringLength2 > BufferLength - 1 )
			{
				wcsncpy(OutConnectionStringW, (const wchar_t *)tmpOutConnectionString, BufferLength - 1);
				*((wchar_t *)OutConnectionString + (BufferLength - 1)) = L'\0' ;
				((CHandle*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_01_004);
				rc1 = SQL_SUCCESS_WITH_INFO;
			}
			else //Copy it
				wcscpy(OutConnectionStringW, (const wchar_t *)tmpOutConnectionString);
		}
	}
	else if(OutConnectionStringW != NULL)
			*OutConnectionStringW = '\0';
	if (StringLength2Ptr != NULL)
		*StringLength2Ptr = TransStringLength2;
	if((rc1 == SQL_ERROR) || (rc1 == SQL_SUCCESS_WITH_INFO)) 
		rc = rc1 ; 
	return rc;
}

SQLRETURN SQL_API SQLDriverConnectW(SQLHDBC  ConnectionHandle,
    SQLHWND            WindowHandle,
    SQLWCHAR 		  *InConnectionStringW,
    SQLSMALLINT        StringLength1,
    SQLWCHAR           *OutConnectionStringW,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT 	  *StringLength2Ptr,
    SQLUSMALLINT       DriverCompletion)
{
	SQLRETURN	rc, rc1;
	RETCODE		retHandle = 0;
	
	SQLCHAR InConnectionString[2048];
	SQLCHAR OutConnectionString[2048];
	SQLCHAR tmpOutConnectionString[2048];
	int  TransStringLength1 = 0;
	int  TransStringLength2 = 0;
	char error[64];
	//There should not be any data truncation for the following call. So just check for SQL_ERROR

		
	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLDriverConnectW)
				retHandle = (fpTraceSQLDriverConnectW)(ConnectionHandle, WindowHandle, InConnectionStringW, TransStringLength1,
					OutConnectionStringW, BufferLength, StringLength2Ptr, DriverCompletion);

		}
	}
	else
		RESET_TRACE();

	if(WCharToUTF8(InConnectionStringW, StringLength1, (char*)InConnectionString, sizeof(InConnectionString), &TransStringLength1, error) != SQL_SUCCESS)
	{
		((CHandle*)ConnectionHandle)->setWcharConvError(error); 
		return SQL_ERROR;
	}
#ifndef SEH
		rc = DriverConnect(ConnectionHandle, WindowHandle, InConnectionString, TransStringLength1,
					OutConnectionString, sizeof(OutConnectionString), StringLength2Ptr, DriverCompletion);
#else
	__try{
		rc = DriverConnect(ConnectionHandle, WindowHandle, InConnectionString, TransStringLength1,
					OutConnectionString, sizeof(OutConnectionString), StringLength2Ptr, DriverCompletion);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		((CHandle*)ConnectionHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);

	if(rc != SQL_ERROR)
	{
		if((rc1 = UTF8ToWChar((char*)OutConnectionString, strlen((const char*)OutConnectionString), (wchar_t *)tmpOutConnectionString, 
							  sizeof(tmpOutConnectionString)/2, &TransStringLength2, error)) != SQL_SUCCESS)
			((CHandle*)ConnectionHandle)->setWcharConvError(error);
		
		if (OutConnectionStringW != NULL)
		{
			//check whether we had been given enough buffer (BufferLength)
			if( TransStringLength2 > BufferLength - 1 )
			{
				wcsncpy(OutConnectionStringW, (const wchar_t *)tmpOutConnectionString, BufferLength - 1);
				*((wchar_t *)OutConnectionString + (BufferLength - 1)) = L'\0' ;
				((CHandle*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_01_004);
				rc1 = SQL_SUCCESS_WITH_INFO;
			}
			else //Copy it
				wcscpy(OutConnectionStringW, (const wchar_t *)tmpOutConnectionString);
		}
	}
	else if(OutConnectionStringW != NULL)
			*OutConnectionStringW = '\0';
	if (StringLength2Ptr != NULL)
		*StringLength2Ptr = TransStringLength2;
	if((rc1 == SQL_ERROR) || (rc1 == SQL_SUCCESS_WITH_INFO))
		rc = rc1 ;
	return rc;
}

SQLRETURN SQL_API SQLPrepareW(SQLHSTMT StatementHandle,
           SQLWCHAR *StatementTextW,
		   SQLINTEGER TextLength)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;
	int sqlStringLen, TransStringLength = 0;
	char error[64];

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLPrepareW)
				retHandle = (fpTraceSQLPrepareW)(StatementHandle, StatementTextW, TextLength);
		}
	}
	else
		RESET_TRACE();

	if (TextLength == SQL_NTS)
		sqlStringLen = wcslen((const wchar_t *)StatementTextW);
	else
		sqlStringLen = TextLength;
	unsigned char* StatementText = new unsigned char[sqlStringLen*4];
	if(WCharToUTF8(StatementTextW, sqlStringLen, (char*)StatementText, sqlStringLen*4, &TransStringLength, error) != SQL_SUCCESS)
	{	
		delete[] StatementText;
		((CHandle*)StatementHandle)->setWcharConvError(error); 
		return SQL_ERROR;
	}
#ifndef SEH
		rc = Prepare(StatementHandle, StatementText, TransStringLength);
#else
	__try{
		rc = Prepare(StatementHandle, StatementText, TransStringLength);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		delete[] StatementText;
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
		return rc;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	delete[] StatementText;
	return rc;
}

SQLRETURN SQL_API SQLExecDirectW(SQLHSTMT StatementHandle,
           SQLWCHAR *StatementTextW,
		   SQLINTEGER TextLength)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;
	int sqlStringLen, TransStringLength = 0;
	char error[64];

	if (TextLength == SQL_NTS)
		sqlStringLen = wcslen((const wchar_t *)StatementTextW);
	else
		sqlStringLen = TextLength;
	unsigned char* StatementText = new unsigned char[sqlStringLen*4];

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLExecDirectW)
				retHandle = (fpTraceSQLExecDirectW)(StatementHandle, StatementTextW, TextLength);
		}
	}
	else
		RESET_TRACE();

	if(WCharToUTF8(StatementTextW, sqlStringLen, (char *)StatementText, sqlStringLen*4, &TransStringLength, error) != SQL_SUCCESS)
	{
		delete[] StatementText;
		((CHandle*)StatementHandle)->setWcharConvError(error); 
		return SQL_ERROR;
	}
#ifndef SEH
		rc = ExecDirect(StatementHandle, StatementText, TransStringLength);
#else
	__try{
		rc = ExecDirect(StatementHandle, StatementText, TransStringLength);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		delete[] StatementText;
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
		return rc;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	delete[] StatementText;
	return rc;
}

SQLRETURN  SQL_API SQLDescribeColW(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLWCHAR *ColumnNameW,
           SQLSMALLINT BufferLength, 
		   SQLSMALLINT *NameLengthPtr,
           SQLSMALLINT *DataTypePtr, 
		   SQLULEN *ColumnSizePtr,
           SQLSMALLINT *DecimalDigitsPtr,
		   SQLSMALLINT *NullablePtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLDescribeColW)
				retHandle = (fpTraceSQLDescribeColW)(StatementHandle, ColumnNumber, (SQLWCHAR *)ColumnNameW, 
						BufferLength, NameLengthPtr, DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, NullablePtr);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = getDescRec(StatementHandle, SQL_API_SQLDESCRIBECOL, ColumnNumber, ColumnNameW, BufferLength, NameLengthPtr,
			DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, NullablePtr);
#else
	__try{
		rc = getDescRec(StatementHandle, SQL_API_SQLDESCRIBECOL, ColumnNumber, ColumnNameW, BufferLength, NameLengthPtr,
			DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, NullablePtr);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLTablesW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogNameW, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaNameW, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableNameW, 
		   SQLSMALLINT NameLength3,
           SQLWCHAR *TableTypeW, 
		   SQLSMALLINT NameLength4)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLTablesW)
				retHandle = (fpTraceSQLTablesW)(StatementHandle, CatalogNameW, NameLength1, SchemaNameW, NameLength2, 
						TableNameW, NameLength3, TableTypeW, NameLength4);
		}
	}
	else
		RESET_TRACE();

#ifndef SEH
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLTABLES, CatalogNameW, NameLength1, SchemaNameW, NameLength2, 
				TableNameW, NameLength3, NULL, SQL_NTS, TableTypeW, NameLength4);
#else
	__try{
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLTABLES, CatalogNameW, NameLength1, SchemaNameW, NameLength2, 
				TableNameW, NameLength3, NULL, SQL_NTS, TableTypeW, NameLength4);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc); 
	return rc;
}

SQLRETURN  SQL_API SQLColumnsW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogNameW, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaNameW, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableNameW, 
		   SQLSMALLINT NameLength3,
           SQLWCHAR *ColumnNameW, 
		   SQLSMALLINT NameLength4)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLColumnsW)
				retHandle = (fpTraceSQLColumnsW)(StatementHandle, CatalogNameW, NameLength1, SchemaNameW, 
						NameLength2, TableNameW, NameLength3,  ColumnNameW, NameLength4);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLCOLUMNS, CatalogName, 
				NameLength1, SchemaName, NameLength2, 
				TableName, NameLength3,  ColumnName, 
				NameLength4);
#else
	__try{
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLCOLUMNS, CatalogNameW, 
				NameLength1, SchemaNameW, NameLength2, 
				TableNameW, NameLength3,  ColumnNameW, 
				NameLength4);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLSpecialColumnsW(SQLHSTMT StatementHandle,
		   SQLUSMALLINT IdentifierType,
           SQLWCHAR *CatalogNameW, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaNameW, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableNameW, 
		   SQLSMALLINT NameLength3,
		   SQLUSMALLINT Scope,
		   SQLUSMALLINT Nullable)
{
 	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLSpecialColumnsW)
				retHandle = (fpTraceSQLSpecialColumnsW)(StatementHandle, IdentifierType, CatalogNameW, NameLength1, SchemaNameW, 
						NameLength2, TableNameW, NameLength3,  Scope, Nullable);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLSPECIALCOLUMNS, CatalogName, 
				NameLength1, SchemaName, NameLength2, 
				TableName, NameLength3,  NULL, SQL_NTS, NULL, SQL_NTS, IdentifierType, Scope, Nullable);
#else
	__try{
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLSPECIALCOLUMNS, CatalogNameW, 
				NameLength1, SchemaNameW, NameLength2, 
				TableNameW, NameLength3,  NULL, SQL_NTS, NULL, SQL_NTS, IdentifierType, Scope, Nullable);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLPrimaryKeysW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableName, 
		   SQLSMALLINT NameLength3)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLPrimaryKeysW)
				retHandle = (fpTraceSQLPrimaryKeysW)(StatementHandle, CatalogName, NameLength1, SchemaName, 
						NameLength2, TableName, NameLength3);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLPRIMARYKEYS, CatalogName, 
				NameLength1, SchemaName, NameLength2, 
				TableName, NameLength3);
#else
	__try{
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLPRIMARYKEYS, CatalogName, 
				NameLength1, SchemaName, NameLength2, 
				TableName, NameLength3);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLStatisticsW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   SQLUSMALLINT Unique,
		   SQLUSMALLINT Reserved)
{
 	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLStatisticsW)
				retHandle = (fpTraceSQLStatisticsW)(StatementHandle, CatalogName, NameLength1, SchemaName, 
						NameLength2, TableName, NameLength3, Unique, Reserved);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLSTATISTICS, CatalogName, 
				NameLength1, SchemaName, NameLength2, 
				TableName, NameLength3,  NULL, SQL_NTS, NULL, SQL_NTS, 0, 0, 0, 0, Unique, Reserved);
#else
	__try{
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLSTATISTICS, CatalogName, 
				NameLength1, SchemaName, NameLength2, 
				TableName, NameLength3,  NULL, SQL_NTS, NULL, SQL_NTS, 0, 0, 0, 0, Unique, Reserved);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLGetCursorNameW(SQLHSTMT StatementHandle,
           SQLWCHAR *CursorNameW, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLengthPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetCursorNameW)
				retHandle = (fpTraceSQLGetCursorNameW)(StatementHandle, CursorNameW, BufferLength, 
						NameLengthPtr);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetCursorName(StatementHandle, CursorName, BufferLength, NameLengthPtr);
#else
	__try{
		rc = GetCursorName(StatementHandle, CursorNameW, BufferLength, NameLengthPtr);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLSetCursorNameW(SQLHSTMT StatementHandle,
           SQLWCHAR *CursorNameW, 
		   SQLSMALLINT NameLength)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLSetCursorNameW)
				retHandle = (fpTraceSQLSetCursorNameW)(StatementHandle, CursorNameW,  NameLength);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = SetCursorName(StatementHandle, CursorName,  NameLength);
#else
	__try{
		rc = SetCursorName(StatementHandle, CursorNameW,  NameLength);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}
		
SQLRETURN SQL_API SQLNativeSqlW(
    SQLHDBC            ConnectionHandle,
    SQLWCHAR 		  *InStatementTextW,
    SQLINTEGER         TextLength1,
    SQLWCHAR 		  *OutStatementTextW,
    SQLINTEGER         BufferLength,
    SQLINTEGER 		  *TextLength2Ptr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLNativeSqlW)
				retHandle = (fpTraceSQLNativeSqlW)(ConnectionHandle, InStatementTextW, TextLength1, OutStatementTextW, 
					BufferLength, TextLength2Ptr);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = NativeSql(ConnectionHandle, InStatementText, TextLength1, OutStatementText, 
					BufferLength, TextLength2Ptr);
#else
	__try{
		rc = NativeSql(ConnectionHandle, InStatementTextW, TextLength1, OutStatementTextW, 
					BufferLength, TextLength2Ptr);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (ConnectionHandle) ((CHandle*)ConnectionHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

#ifdef _WIN64
SQLRETURN  SQL_API SQLColAttributeW (SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLength, SQLLEN *NumericAttribute)
#else
SQLRETURN  SQL_API SQLColAttributeW (SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLength, SQLPOINTER NumericAttribute)
#endif
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLColAttributeW)
				retHandle = (fpTraceSQLColAttributeW)(StatementHandle, ColumnNumber, FieldIdentifier,
						CharacterAttribute, BufferLength, StringLength, NumericAttribute);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = ColAttribute(StatementHandle, ColumnNumber, FieldIdentifier, CharacterAttributePtr, BufferLength,
					StringLengthPtr, NumericAttributePtr);
#else
	__try{
		rc = ColAttribute(StatementHandle, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
					StringLength, NumericAttribute);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLProceduresW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogNameW, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaNameW, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *ProcNameW, 
		   SQLSMALLINT NameLength3)
           
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLProceduresW)
				retHandle = (fpTraceSQLProceduresW)(StatementHandle, CatalogNameW, NameLength1, SchemaNameW, 
						NameLength2, ProcNameW, NameLength3);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLPROCEDURES, CatalogNameW, 
				NameLength1, SchemaNameW, NameLength2, 
				ProcNameW, NameLength3);
#else
	__try{
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLPROCEDURES, CatalogNameW, 
				NameLength1, SchemaNameW, NameLength2, 
				ProcNameW, NameLength3);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLProcedureColumnsW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogNameW, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaNameW, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *ProcNameW, 
		   SQLSMALLINT NameLength3,
           SQLWCHAR *ColumnNameW, 
		   SQLSMALLINT NameLength4)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLProcedureColumnsW)
				retHandle = (fpTraceSQLProcedureColumnsW)(StatementHandle, CatalogNameW, NameLength1, SchemaNameW, 
						NameLength2, ProcNameW, NameLength3,  ColumnNameW, NameLength4);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLPROCEDURECOLUMNS, CatalogNameW, 
				NameLength1, SchemaNameW, NameLength2, 
				ProcNameW, NameLength3,  ColumnNameW, 
				NameLength4);
#else
	__try{
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLPROCEDURECOLUMNS, CatalogNameW, 
				NameLength1, SchemaNameW, NameLength2, 
				ProcNameW, NameLength3,  ColumnNameW, 
				NameLength4);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLColumnPrivilegesW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogNameW, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaNameW, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableNameW, 
		   SQLSMALLINT NameLength3,
           SQLWCHAR *ColumnNameW, 
		   SQLSMALLINT NameLength4)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLColumnPrivilegesW)
				retHandle = (fpTraceSQLColumnPrivilegesW)(StatementHandle, CatalogNameW, NameLength1, SchemaNameW, 
						NameLength2, TableNameW, NameLength3,  ColumnNameW, NameLength4);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLCOLUMNPRIVILEGES, CatalogNameW, 
				NameLength1, SchemaNameW, NameLength2, 
				TableNameW, NameLength3,  ColumnNameW, 
				NameLength4);
#else
	__try{
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLCOLUMNPRIVILEGES, CatalogNameW, 
				NameLength1, SchemaNameW, NameLength2, 
				TableNameW, NameLength3,  ColumnNameW, 
				NameLength4);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API SQLTablePrivilegesW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogNameW, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaNameW, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableNameW, 
		   SQLSMALLINT NameLength3)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLTablePrivilegesW)
				retHandle = (fpTraceSQLTablePrivilegesW)(StatementHandle, CatalogNameW, NameLength1, SchemaNameW, 
						NameLength2, TableNameW, NameLength3);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLTABLEPRIVILEGES, CatalogNameW, 
				NameLength1, SchemaNameW, NameLength2, 
				TableNameW, NameLength3);
#else
	__try{
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLTABLEPRIVILEGES, CatalogNameW, 
				NameLength1, SchemaNameW, NameLength2, 
				TableNameW, NameLength3);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN	SQL_API SQLForeignKeysW(SQLHSTMT StatementHandle,
			SQLWCHAR *PKCatalogNameW,
			SQLSMALLINT NameLength1,
			SQLWCHAR *PKSchemaNameW,
			SQLSMALLINT	NameLength2,
			SQLWCHAR *PKTableNameW,
			SQLSMALLINT	NameLength3,
			SQLWCHAR *FKCatalogNameW,
			SQLSMALLINT	NameLength4,
			SQLWCHAR *FKSchemaNameW,
			SQLSMALLINT	NameLength5,
			SQLWCHAR *FKTableNameW,
			SQLSMALLINT	NameLength6)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_API))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLForeignKeysW)
				retHandle = (fpTraceSQLForeignKeysW)(StatementHandle, PKCatalogNameW, NameLength1, PKSchemaNameW, 
						NameLength2, PKTableNameW, NameLength3, FKCatalogNameW, NameLength4, FKSchemaNameW, 
						NameLength5, FKTableNameW, NameLength6);
		}
	}
	else
		RESET_TRACE();
#ifndef SEH
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLFOREIGNKEYS, PKCatalogNameW, NameLength1, PKSchemaNameW, 
						NameLength2, PKTableNameW, NameLength3, NULL, SQL_NTS, NULL, SQL_NTS, 0, 0, 0, 0, 0, 0,
						FKCatalogNameW, NameLength4, FKSchemaNameW, NameLength5, FKTableNameW, NameLength6);
#else
	__try{
		rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLFOREIGNKEYS, PKCatalogNameW, NameLength1, PKSchemaNameW, 
						NameLength2, PKTableNameW, NameLength3, NULL, SQL_NTS, NULL, SQL_NTS, 0, 0, 0, 0, 0, 0,
						FKCatalogNameW, NameLength4, FKSchemaNameW, NameLength5, FKTableNameW, NameLength6);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (StatementHandle) ((CHandle*)StatementHandle)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}
#endif
	TRACE_RETURN(retHandle, rc);
	return rc;
}

