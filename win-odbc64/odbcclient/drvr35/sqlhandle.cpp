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
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400 
#endif
#include "DrvrGlobal.h"
#include "SQLHandle.h"
#include "CEnv.h"
#include "CConnect.h"
#include "CStmt.h"
#include "CDesc.h"
#include "tdm_odbcDrvMsg.h"

SQLRETURN ODBC::AllocHandle(SQLSMALLINT HandleType,
			SQLHANDLE InputHandle, 
			SQLHANDLE *OutputHandle)
{
	SQLRETURN	rc = SQL_SUCCESS;
	CEnv* pEnv;
	CConnect* pConnect;

	switch (HandleType)
	{
	case SQL_HANDLE_ENV:
		*OutputHandle = new CEnv(InputHandle);
		if (*OutputHandle == NULL)
			rc = SQL_ERROR;
		break;
	case SQL_HANDLE_DBC:
		pEnv = (CEnv*)InputHandle;
		EnterCriticalSection(&pEnv->m_CSObject);
		rc = pEnv->AllocHandle(HandleType,InputHandle,OutputHandle);
		LeaveCriticalSection(&pEnv->m_CSObject);
		break;
	case SQL_HANDLE_STMT:
		pConnect = (CConnect*)InputHandle;
		EnterCriticalSection(&pConnect->m_CSObject);
		rc = pConnect->AllocHandle(HandleType,InputHandle,OutputHandle);
		LeaveCriticalSection(&pConnect->m_CSObject);
		break;
	case SQL_HANDLE_DESC:
		pConnect = (CConnect*)InputHandle;
		EnterCriticalSection(&pConnect->m_CSObject);
		rc = pConnect->AllocHandle(HandleType,InputHandle,OutputHandle);
		LeaveCriticalSection(&pConnect->m_CSObject);
		break;
	default:
		*OutputHandle = SQL_NULL_HANDLE;
		break;
	}
	return rc;
}


SQLRETURN ODBC::FreeHandle(SQLSMALLINT HandleType, 
			SQLHANDLE Handle)
{
	CEnv* pEnv;
	CConnect* pConnect;
	CStmt* pStmt;
	CDesc* pDesc;

	switch (HandleType)
	{
	case SQL_HANDLE_ENV:
		delete (CEnv *)Handle;
		break;
	case SQL_HANDLE_DBC:
		pConnect = (CConnect *)Handle;
		pEnv = pConnect->getEnvHandle();
		if(TryEnterCriticalSection(&pEnv->m_CSObject))
		{
			if(pEnv->m_CSObject.RecursionCount > 1)
			{
  			    pConnect->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, "The handle is in use and cannot be freed.");
				LeaveCriticalSection(&pEnv->m_CSObject);
				return SQL_ERROR;
			}
		}
		else
		{
		   EnterCriticalSection(&pEnv->m_CSObject);
		}
		delete pConnect;
		LeaveCriticalSection(&pEnv->m_CSObject);
		break;
	case SQL_HANDLE_STMT:
		pStmt = (CStmt *)Handle;
		pConnect = pStmt->getConnectHandle();
		if(TryEnterCriticalSection(&pConnect->m_CSObject))
		{
			if(pConnect->m_CSObject.RecursionCount > 1)
			{
  			    pStmt->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, "The handle is in use and cannot be freed.");
				LeaveCriticalSection(&pConnect->m_CSObject);
				return SQL_ERROR;
			}
		}
		else
		{
		   EnterCriticalSection(&pConnect->m_CSObject);
		}
		delete pStmt;
		LeaveCriticalSection(&pConnect->m_CSObject);
		break;
	case SQL_HANDLE_DESC:
		pDesc = (CDesc *)Handle;
		pConnect = pDesc->getDescConnect();
		if(TryEnterCriticalSection(&pConnect->m_CSObject))
		{
			if(pConnect->m_CSObject.RecursionCount > 1)
			{
  			    pDesc->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, "The handle is in use and cannot be freed.");
				LeaveCriticalSection(&pConnect->m_CSObject);
				return SQL_ERROR;
			}
		}
		else
		{
		   EnterCriticalSection(&pConnect->m_CSObject);
		}
		delete pDesc;
		LeaveCriticalSection(&pConnect->m_CSObject);
		break;
	default:
		return SQL_INVALID_HANDLE;
	}
	return SQL_SUCCESS;

}

SQLRETURN ODBC::GetDiagRec(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber,
				SQLWCHAR *Sqlstate,
				SQLINTEGER *NativeError, 
				SQLWCHAR *MessageText,
				SQLSMALLINT BufferLength, 
				SQLSMALLINT *TextLength)
{

	SQLRETURN rc;
	DWORD	ErrorMsgLang = 0;
	CConnect* pConnect;
	CStmt* pStmt;
	CDesc* pDesc;
	
	switch (HandleType)
	{
	case SQL_HANDLE_ENV:
		break;
	case SQL_HANDLE_DBC:
		pConnect = (CConnect*)Handle;
		EnterCriticalSection(&pConnect->m_CSObject);
		ErrorMsgLang = pConnect->getErrorMsgLang();
		break;
	case SQL_HANDLE_STMT:
		pStmt = (CStmt *)Handle;
		pConnect = pStmt->getConnectHandle();
		EnterCriticalSection(&pConnect->m_CSObject);
		ErrorMsgLang = pStmt->getErrorMsgLang();
		break;
	case SQL_HANDLE_DESC:
		pDesc = (CDesc *)Handle;
		pConnect = pDesc->getDescConnect();
		EnterCriticalSection(&pConnect->m_CSObject);
		ErrorMsgLang = pDesc->getErrorMsgLang();
		break;
	default:
		return SQL_INVALID_HANDLE;
	}
	__try{
		rc = ((CHandle *)Handle)->GetDiagRec(HandleType, Handle, 
					RecNumber, ErrorMsgLang, Sqlstate, NativeError, MessageText, BufferLength, TextLength);
	}
	__finally {
		switch (HandleType)
		{
			case SQL_HANDLE_ENV:
				break;
			case SQL_HANDLE_DBC:
				LeaveCriticalSection(&pConnect->m_CSObject);
				break;
			case SQL_HANDLE_STMT:
				LeaveCriticalSection(&pConnect->m_CSObject);
				break;
			case SQL_HANDLE_DESC:
				LeaveCriticalSection(&pConnect->m_CSObject);
				break;
		}
	}
	return rc;
}

SQLRETURN ODBC::GetDiagField(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber, 
				SQLSMALLINT DiagIdentifier,
				SQLPOINTER DiagInfo, 
				SQLSMALLINT BufferLength,
				SQLSMALLINT *StringLengthPtr)
{
	SQLRETURN rc;
	DWORD	ErrorMsgLang = 0;
	CConnect* pConnect;
	CStmt* pStmt;
	CDesc* pDesc;
	
	switch (HandleType)
	{
	case SQL_HANDLE_ENV:
		break;
	case SQL_HANDLE_DBC:
		pConnect = (CConnect*)Handle;
		EnterCriticalSection(&pConnect->m_CSObject);
		ErrorMsgLang = pConnect->getErrorMsgLang();
		break;
	case SQL_HANDLE_STMT:
		pStmt = (CStmt *)Handle;
		pConnect = pStmt->getConnectHandle();
		EnterCriticalSection(&pConnect->m_CSObject);
		ErrorMsgLang = pStmt->getErrorMsgLang();
		break;
	case SQL_HANDLE_DESC:
		pDesc = (CDesc *)Handle;
		pConnect = pDesc->getDescConnect();
		EnterCriticalSection(&pConnect->m_CSObject);
		ErrorMsgLang = pDesc->getErrorMsgLang();
		break;
	default:
		return SQL_INVALID_HANDLE;
	}
	__try{
		rc = ((CHandle *)Handle)->GetDiagField(HandleType, Handle, RecNumber, 
					ErrorMsgLang, DiagIdentifier, DiagInfo, BufferLength, StringLengthPtr);
	}
	__finally {
		switch (HandleType)
		{
			case SQL_HANDLE_ENV:
				break;
			case SQL_HANDLE_DBC:
				LeaveCriticalSection(&pConnect->m_CSObject);
				break;
			case SQL_HANDLE_STMT:
				LeaveCriticalSection(&pConnect->m_CSObject);
				break;
			case SQL_HANDLE_DESC:
				LeaveCriticalSection(&pConnect->m_CSObject);
				break;
		}
	}
	return rc;
}

SQLRETURN  ODBC::EndTran(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT CompletionType)
{
	SQLRETURN rc;
	DWORD	ErrorMsgLang = 0;
	CConnect* pConnect;

	switch (HandleType)
	{
	case SQL_HANDLE_ENV:	// DM implments by sending SQLEndTran for every active connection
		rc = SQL_INVALID_HANDLE;
		break;
	case SQL_HANDLE_DBC:
		pConnect = (CConnect*)Handle;
		EnterCriticalSection(&pConnect->m_CSObject);
		ErrorMsgLang = pConnect->getErrorMsgLang();
		rc = pConnect->EndTran(CompletionType);
		LeaveCriticalSection(&pConnect->m_CSObject);
		break;
	default:
		return SQL_INVALID_HANDLE;
	}
	return rc;
}

