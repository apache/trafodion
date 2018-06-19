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

#include "drvrglobal.h"
#include "chandle.h"
#include "cconnect.h"
#include "mxomsg.h"
#include "csconvert.h"
#include "sqlcli.h"
#include "diagfunctions.h" //10-080124-0030 
#include "cstmt.h" //10-080201-0266
//
// Implements the member functions of CHandle

CHandle::CHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle)
{

//============= for Driver Manager ==================

	type = HandleType;
	state = 0;
	cursor_state = 0;
	prep_state = 0;
	asyn_on = 0;
	need_on = 0;
	stmt_cip = 0;
	dbc_cip = 0;
	desc_cip = 0;
	returncode = 0;

//====================================================

	m_HandleType = HandleType;
	m_InputHandle = InputHandle;
	m_ExceptionNr = CEE_SUCCESS;
	m_HeartBeatEnable = TRUE;
	m_SqlWarning = FALSE;
	m_CurrentOdbcAPI = 0;
	InitializeCriticalSection2(&m_CSObject);

	EnterCriticalSection2(&gDrvrGlobal.gHandleCSObject);
	m_HandleNumber = ++(gDrvrGlobal.gHandle.m_HandleSeq);
	gDrvrGlobal.gHandle.m_HandleCollect.push_back(this);
	LeaveCriticalSection2(&gDrvrGlobal.gHandleCSObject);
}

CHandle::~CHandle()
{

	CHandlePtr		pHandle;

	DeleteCriticalSection2(&m_CSObject);


	EnterCriticalSection2(&gDrvrGlobal.gHandleCSObject);
	std::list<CHandlePtr>::iterator i = gDrvrGlobal.gHandle.m_HandleCollect.begin();

	// i() not available in STL
	for( ;i !=  gDrvrGlobal.gHandle.m_HandleCollect.end(); ++i)
		if ((pHandle = (CHandlePtr)(*i)) !=  NULL)
	{
		if (pHandle == this)
		{	
			gDrvrGlobal.gHandle.m_HandleCollect.erase(i);
			break;
		}
	}

	LeaveCriticalSection2(&gDrvrGlobal.gHandleCSObject);

	return;
}

// virtual function - note this has been overriden in CStmt
void CHandle::clearError()
{
	EnterCriticalSection2(&m_CSObject);

	if (m_DiagRec.getClearDiagRec())
	{
		m_DiagRec.clear();
		m_SqlWarning = FALSE;
		m_ExceptionNr = CEE_SUCCESS;
	}

	LeaveCriticalSection2(&m_CSObject);
}

void CHandle::purgeError()
{
	EnterCriticalSection2(&m_CSObject);

	m_DiagRec.clear();
	m_SqlWarning = FALSE;
	m_ExceptionNr = CEE_SUCCESS;

	LeaveCriticalSection2(&m_CSObject);
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

#ifdef __DEBUGASYNCIO
        printf("ECS2 CHandle::setDiagRec(): Acquirng m_CSObject Lock for handle %x\n",this);
#endif 

	EnterCriticalSection2(&m_CSObject);

#ifdef __DEBUGASYNCIO
        printf("ECS2 CHandle::setDiagRec(): Acquired m_CSObject Lock for handle %x\n",this);
#endif
	m_DiagRec.setDiagRec(diagComponentCode, diagErrorCode, diagNative,
			diagMessageText, diagSqlState, diagRowNumber, diagColumnNumber, diagNoParams, marker);

#ifdef __DEBUGASYNCIO
        printf("LCS2 CHandle::setDiagRec(): Releasing m_CSObject Lock for handle %x\n",this);
#endif 

	LeaveCriticalSection2(&m_CSObject);

#ifdef __DEBUGASYNCIO
        printf("LCS2 CHandle::setDiagRec(): Released m_CSObject Lock for handle %x\n",this);
#endif
}

//10-080201-0266
SQLRETURN CHandle::GetDiagRec(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT	RecNumber, 
				DWORD	ErrorMsgLang,
				SQLCHAR *SqlState,
				SQLINTEGER	*NativeErrPtr, 
				SQLCHAR *MessageText,
				SQLSMALLINT	BufferLength,
				SQLSMALLINT *TextLengthPtr,
				ICUConverter* icuConv)
{
	SQLRETURN rc;
	DWORD translateOption = 0;

#ifdef __DEBUGASYNCIO
        printf("ECS2 CHandle::GetDiagRec(): Acquirng m_CSObject Lock for handle %x\n",this);
#endif 
	EnterCriticalSection2(&m_CSObject);

#ifdef __DEBUGASYNCIO
        printf("ECS2 CHandle::GetDiagRec(): Acquired m_CSObject Lock for handle %x\n",this);
#endif
	rc = m_DiagRec.GetDiagRec(RecNumber, ErrorMsgLang, SqlState, NativeErrPtr, MessageText, 
			BufferLength, TextLengthPtr, icuConv);

#ifdef __DEBUGASYNCIO
        printf("LCS2 CHandle::GetDiagRec(): Releasing m_CSObject Lock for handle %x\n",this);
#endif 
	LeaveCriticalSection2(&m_CSObject);

#ifdef __DEBUGASYNCIO
        printf("LCS2 CHandle::GetDiagRec(): Released m_CSObject Lock for handle %x\n",this);
#endif

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
	DWORD translateOption = 0;

	EnterCriticalSection2(&m_CSObject);
	rc = m_DiagRec.GetDiagField(HandleType, Handle, RecNumber, ErrorMsgLang, DiagIdentifier, DiagInfoPtr,
				BufferLength, StringLengthPtr);  //, getSQLDataSourceToDriverFP(), translateOption);
	LeaveCriticalSection2(&m_CSObject);
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
	default:
		strcpy(sMessage,"EXCEPTION: UNKNOWN");
	}
	setDiagRec(DRIVER_ERROR, IDS_HY_000, subCode, sMessage);
}

bool CHandle::setAppType(int value)
{
	bool rc = true;
/*	if(m_ICUConv->m_AppType == APP_TYPE_NONE) //First time
		m_ICUConv->m_AppType = value;
	else if(value !=  m_ICUConv->m_AppType) //throw an error! We can not mix and match W and A functions!!!
		rc = false;*/
    m_ICUConv->m_AppType = value; // support match W and A functions
	return rc;
}


// Implements the member functions of CHandleGlobal
CHandleGlobal::CHandleGlobal()
{
	m_HandleSeq = 0;
}
BOOL CHandleGlobal::validateHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
//	CHANDLECOLLECT::iterator i;
#ifdef VERSION3
	std::list<CHandlePtr>::iterator i;
#else
	RWTPtrSlistIterator<CHandle> i(m_HandleCollect);
#endif

	BOOL			found = FALSE;
	CHandlePtr		pHandle;
	CHandlePtr		tHandle;

	memcpy(&tHandle, &Handle, sizeof(SQLHANDLE));

	EnterCriticalSection2(&gDrvrGlobal.gHandleCSObject);
//	for (i = m_HandleCollect.begin() ; i !=  m_HandleCollect.end() ; ++i)
//	{
//		if ((*i) == Handle && (*i)->m_HandleType == HandleType)
//		{
//			found = TRUE;
//			break;
//		}
//	}
#ifdef VERSION3
	for (i = m_HandleCollect.begin() ; i !=  m_HandleCollect.end() ; ++i)
		if ((pHandle = (CHandlePtr)(*i)) != NULL)		
#else
			while ((pHandle = i()) !=  NULL)
#endif
	{
		if ((pHandle) == tHandle && pHandle->m_HandleType == HandleType)
		{
			found = TRUE;
			break;
		}
	}
	LeaveCriticalSection2(&gDrvrGlobal.gHandleCSObject);
	return found;
}

