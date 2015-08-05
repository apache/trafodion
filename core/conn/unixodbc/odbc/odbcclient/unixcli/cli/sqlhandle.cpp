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

#include "drvrglobal.h"
#include "sqlhandle.h"
#include "cenv.h"
#include "cconnect.h"
#include "cstmt.h"
#include "cdesc.h"

SQLRETURN ODBC::AllocHandle(SQLSMALLINT HandleType,
			SQLHANDLE InputHandle, 
			SQLHANDLE *OutputHandle)
{
	SQLRETURN	rc = SQL_SUCCESS; 

	switch (HandleType)
	{
	case SQL_HANDLE_ENV:
		*OutputHandle = (SQLHANDLE)new CEnv(InputHandle);
		if (*OutputHandle == NULL)
			rc = SQL_ERROR;
		break;
	case SQL_HANDLE_DBC:
		rc = ((CEnv*)InputHandle)->AllocHandle(HandleType,InputHandle,OutputHandle);
		break;
	case SQL_HANDLE_STMT:
		rc = ((CConnect*)InputHandle)->AllocHandle(HandleType,InputHandle,OutputHandle);
		break;
	case SQL_HANDLE_DESC:
		rc = ((CConnect*)InputHandle)->AllocHandle(HandleType,InputHandle,OutputHandle);
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
    SQLRETURN rc;
	if (! gDrvrGlobal.gHandle.validateHandle(HandleType, Handle))
		return SQL_INVALID_HANDLE;
	switch (HandleType)
	{
	case SQL_HANDLE_ENV:
		delete (CEnv *)Handle;
		break;
	case SQL_HANDLE_DBC:
		delete (CConnect *)Handle;
		break;
	case SQL_HANDLE_STMT:
	    rc = ((CStmt *)Handle)->Close(SQL_DROP);
		if(rc == SQL_SUCCESS)
		delete (CStmt *)Handle;
		else
			return rc;
		//delete (CStmt *)Handle;
		break;
	case SQL_HANDLE_DESC:
		delete (CDesc *)Handle;
		break;
	default:
		return SQL_INVALID_HANDLE;
	}
	return SQL_SUCCESS;

}

SQLRETURN ODBC::GetDiagRec(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber,
				SQLCHAR *Sqlstate,
				SQLINTEGER *NativeError, 
				SQLCHAR *MessageText,
				SQLSMALLINT BufferLength, 
				SQLSMALLINT *TextLength)
{

	SQLRETURN rc;
	DWORD	ErrorMsgLang = 0;
	ICUConverter* pICUConv;

	if (! gDrvrGlobal.gHandle.validateHandle(HandleType, Handle))
		return SQL_INVALID_HANDLE;
	
	switch (HandleType)
	{
	case SQL_HANDLE_ENV:
			pICUConv = &gDrvrGlobal.ICUConv;
		break;
	case SQL_HANDLE_DBC:
		ErrorMsgLang = ((CConnect *)Handle)->getErrorMsgLang();
		pICUConv = ((CHandle *)Handle)->m_ICUConv;
		break;
	case SQL_HANDLE_STMT:
		ErrorMsgLang = ((CStmt *)Handle)->getErrorMsgLang();
		pICUConv = ((CHandle *)Handle)->m_ICUConv;
		break;
	case SQL_HANDLE_DESC:
		ErrorMsgLang = ((CDesc *)Handle)->getErrorMsgLang();
		pICUConv = ((CHandle *)Handle)->m_ICUConv;
		break;
	default:
		return SQL_INVALID_HANDLE;
	}
		rc = ((CHandle *)Handle)->GetDiagRec(HandleType, Handle, 
					RecNumber, ErrorMsgLang, Sqlstate, NativeError, MessageText, BufferLength, TextLength, pICUConv);
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

	if (! gDrvrGlobal.gHandle.validateHandle(HandleType, Handle))
		return SQL_INVALID_HANDLE;
	
	switch (HandleType)
	{
	case SQL_HANDLE_ENV:
		break;
	case SQL_HANDLE_DBC:
		ErrorMsgLang = ((CConnect *)Handle)->getErrorMsgLang();
		break;
	case SQL_HANDLE_STMT:
		ErrorMsgLang = ((CStmt *)Handle)->getErrorMsgLang();
		break;
	case SQL_HANDLE_DESC:
		ErrorMsgLang = ((CDesc *)Handle)->getErrorMsgLang();
		break;
	default:
		return SQL_INVALID_HANDLE;
	}
	rc = ((CHandle *)Handle)->GetDiagField(HandleType, Handle, RecNumber, 
				ErrorMsgLang, DiagIdentifier, DiagInfo, BufferLength, StringLengthPtr);
	return rc;
}

SQLRETURN  ODBC::EndTran(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT CompletionType)
{
	SQLRETURN rc;
	DWORD	ErrorMsgLang = 0;

	if (! gDrvrGlobal.gHandle.validateHandle(HandleType, Handle))
		return SQL_INVALID_HANDLE;
	
	switch (HandleType)
	{
	case SQL_HANDLE_ENV:	// DM implments by sending SQLEndTran for every active connection
		rc = SQL_INVALID_HANDLE;
		break;
	case SQL_HANDLE_DBC:
		ErrorMsgLang = ((CConnect *)Handle)->getErrorMsgLang();
		rc = ((CConnect *)Handle)->EndTran(CompletionType);
		break;
	default:
		return SQL_INVALID_HANDLE;
	}
	return rc;
}


