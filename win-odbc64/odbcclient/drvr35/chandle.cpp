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
/*************************************************************************
**************************************************************************/
//
//
#include "CHandle.h"
#include "DrvrGlobal.h"
#include "CConnect.h"
#include "tdm_odbcDrvMsg.h"
#include "csconvert.h"
#include "SQLCLI.h"
#include "DiagFunctions.h" 
#include "CStmt.h" 
//
// Implements the member functions of CHandle

CHandle::CHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle)
{
	m_HandleNumber = ++(gDrvrGlobal.gHandle.m_HandleSeq);
	m_HandleType = HandleType;
	m_InputHandle = InputHandle;
	m_ExceptionNr = CEE_SUCCESS;
	m_HeartBeatEnable = TRUE;
	m_SqlWarning = FALSE;
	m_CurrentOdbcAPI = 0;
	InitializeCriticalSection(&m_CSObject);
	EnterCriticalSection(&gDrvrGlobal.gHandleCSObject);
	gDrvrGlobal.gHandle.m_HandleCollect.push_back(this);
	LeaveCriticalSection(&gDrvrGlobal.gHandleCSObject);
}

CHandle::~CHandle()
{

	CHANDLECOLLECT::iterator i;

	DeleteCriticalSection(&m_CSObject);
	EnterCriticalSection(&gDrvrGlobal.gHandleCSObject);
	for (i = gDrvrGlobal.gHandle.m_HandleCollect.begin() ; i !=  gDrvrGlobal.gHandle.m_HandleCollect.end() ; ++i)
	{
		if ((*i) == this)
		{
			gDrvrGlobal.gHandle.m_HandleCollect.erase(i);
			break;
		}
	}
	LeaveCriticalSection(&gDrvrGlobal.gHandleCSObject);
	return;
}

void CHandle::clearError()
{
	EnterCriticalSection(&gCollectionCSObject);
	m_DiagRec.clear();
	m_SqlWarning = FALSE;
	m_ExceptionNr = CEE_SUCCESS;
	LeaveCriticalSection(&gCollectionCSObject);
}
	
void CHandle::setDiagRec(short diagComponentCode,
				DWORD		diagErrorCode,
				SQLINTEGER	diagNative,
				char		*diagMessageText,
				char		*diagSqlState,
				SQLINTEGER	diagRowNumber,
				SQLINTEGER	diagColumnNumber,
				short		diagNoParams, ...)
{
	va_list marker;

	va_start( marker, diagNoParams); 

	EnterCriticalSection(&gCollectionCSObject);
	m_DiagRec.setDiagRec(diagComponentCode, diagErrorCode, diagNative,
			diagMessageText, diagSqlState, diagRowNumber, diagColumnNumber, diagNoParams, marker);
	LeaveCriticalSection(&gCollectionCSObject);
}
SQLRETURN CHandle::GetDiagRec(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT	RecNumber, 
				DWORD	ErrorMsgLang,
				SQLWCHAR *SqlState,
				SQLINTEGER	*NativeErrPtr, 
				SQLWCHAR *MessageText,
				SQLSMALLINT	BufferLength,
				SQLSMALLINT *TextLengthPtr)
{
	SQLRETURN rc;

	CStmt *pStmt = (CStmt *)Handle;

	EnterCriticalSection(&gCollectionCSObject);
	rc = m_DiagRec.GetDiagRec(RecNumber, ErrorMsgLang, SqlState, NativeErrPtr, MessageText, 
		BufferLength, TextLengthPtr);
	LeaveCriticalSection(&gCollectionCSObject);
	return rc;
}

SQLRETURN CHandle::GetDiagField(SQLSMALLINT HandleType,
						SQLHANDLE Handle,
						SQLSMALLINT RecNumber,
						DWORD	ErrorMsgLang,
						SQLSMALLINT	DiagIdentifier,
						SQLPOINTER DiagInfoPtr,
						SQLSMALLINT BufferLength,
						SQLSMALLINT *StringLengthPtr)
{
	SQLRETURN rc;
	EnterCriticalSection(&gCollectionCSObject);
	rc = m_DiagRec.GetDiagField(HandleType, Handle, RecNumber, ErrorMsgLang, DiagIdentifier, DiagInfoPtr,
		BufferLength, StringLengthPtr);
	LeaveCriticalSection(&gCollectionCSObject);
	return rc;
}
	

long CHandle::sendCDInfo(long exception_nr)
{
	long rc;
	
	switch (m_HandleType) 
	{
	case SQL_HANDLE_DBC:
		rc = ((CConnect *)this)->sendCDInfo(exception_nr);
		break;
	default:
		rc = exception_nr;
		break;
	}
	return rc;

}

DWORD CHandle::getDrvrCharSet()
{
	switch (m_HandleType) 
	{
	case SQL_HANDLE_DBC:
		return ((CConnect *)this)->getDrvrCharSet();
		break;
	case SQL_HANDLE_STMT:
	case SQL_HANDLE_DESC:
		return ((CConnect *)m_InputHandle)->getDrvrCharSet();
		break;
	default:
		break;
	}
	return 0;
}

DWORD CHandle::getSqlCharSet(long inSqlCharSet)
{
	switch (m_HandleType) 
	{
	case SQL_HANDLE_DBC:
		return ((CConnect *)this)->getSqlCharSet(inSqlCharSet);
		break;
	case SQL_HANDLE_STMT:
	case SQL_HANDLE_DESC:
		return ((CConnect *)m_InputHandle)->getSqlCharSet(inSqlCharSet);
		break;
	default:
		break;
	}
	return 0;
}

void CHandle::structExceptionHandling(long exceptionCode)
{
	long subCode = exceptionCode & 0x000000FF;
	char sMessage[100];
	switch (exceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		strcpy(sMessage,"EXCEPTION: ACCESS VIOLATION");
		break;
	case EXCEPTION_BREAKPOINT:
		strcpy(sMessage,"EXCEPTION: BREAKPOINT");
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		strcpy(sMessage,"EXCEPTION: DATATYPE MISALIGNMENT");
		break;
	case EXCEPTION_SINGLE_STEP:
		strcpy(sMessage,"EXCEPTION: SINGLE STEP");
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		strcpy(sMessage,"EXCEPTION: ARRAY BOUNDS EXCEEDED");
		break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		strcpy(sMessage,"EXCEPTION: FLT DENORMAL OPERAND");
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		strcpy(sMessage,"EXCEPTION: FLT DIVIDE BY ZERO");
		break;
	case EXCEPTION_FLT_INEXACT_RESULT:
		strcpy(sMessage,"EXCEPTION: FLT INEXACT RESULT");
		break;
	case EXCEPTION_FLT_INVALID_OPERATION:
		strcpy(sMessage,"EXCEPTION: FLT INVALID OPERATION");
		break;
	case EXCEPTION_FLT_OVERFLOW:
		strcpy(sMessage,"EXCEPTION: FLT OVERFLOW");
		break;
	case EXCEPTION_FLT_STACK_CHECK:
		strcpy(sMessage,"EXCEPTION: FLT STACK CHECK");
		break;
	case EXCEPTION_FLT_UNDERFLOW:
		strcpy(sMessage,"EXCEPTION: FLT UNDERFLOW");
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		strcpy(sMessage,"EXCEPTION: INT DIVIDE BY ZERO");
		break;
	case EXCEPTION_INT_OVERFLOW:
		strcpy(sMessage,"EXCEPTION: INT OVERFLOW");
		break;
	case EXCEPTION_PRIV_INSTRUCTION:
		strcpy(sMessage,"EXCEPTION: PRIV INSTRUCTION");
		break;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		strcpy(sMessage,"EXCEPTION: NONCONTINUABLE EXCEPTION");
		break;
	case STATUS_STACK_OVERFLOW:
		strcpy(sMessage,"EXCEPTION: STACK OVERFLOW");
		break;
	default:
		sprintf(sMessage,"EXCEPTION: UNKNOWN - Exception Code = %x", exceptionCode);
	}
	setDiagRec(DRIVER_ERROR, IDS_HY_000, subCode, sMessage);
}

// Implements the member functions of CHandleGlobal
CHandleGlobal::CHandleGlobal()
{
	m_HandleSeq = 0;
}

BOOL CHandleGlobal::validateHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{

	CHANDLECOLLECT::iterator i;
	BOOL			found = FALSE;

	EnterCriticalSection(&gDrvrGlobal.gHandleCSObject);
	for (i = m_HandleCollect.begin() ; i !=  m_HandleCollect.end() ; ++i)
	{
		if ((*i) == Handle && (*i)->m_HandleType == HandleType)
		{
			found = TRUE;
			break;
		}
	}
	LeaveCriticalSection(&gDrvrGlobal.gHandleCSObject);
	return found;
}

void CHandle::setWcharConvError(char* error)
{
	setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, error);
}
