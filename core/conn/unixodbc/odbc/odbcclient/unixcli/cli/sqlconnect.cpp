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

#include "drvrglobal.h"
#include "sqlconnect.h"
#include "cconnect.h"
#include "mxomsg.h"


SQLRETURN ODBC::Connect(SQLHDBC ConnectionHandle,
           SQLCHAR *ServerName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *UserName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *Authentication, 
		   SQLSMALLINT NameLength3)
{
	SQLRETURN	rc;
	CConnect	*pConnect;
	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_DBC, ConnectionHandle))
		return SQL_INVALID_HANDLE;

	pConnect = (CConnect *)ConnectionHandle;
	rc = pConnect->Connect(ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3,
		TRUE);
	return rc;
}

SQLRETURN ODBC::Disconnect(SQLHDBC ConnectionHandle)
{

	SQLRETURN rc;
	CConnect	*pConnect;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_DBC, ConnectionHandle))
		return SQL_INVALID_HANDLE;
	
	pConnect = (CConnect *)ConnectionHandle;
	rc = pConnect->Disconnect();
	return rc;
}

SQLRETURN ODBC::SetConnectAttr(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	SQLRETURN	rc;
	CConnect	*pConnect;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_DBC, ConnectionHandle))
		return SQL_INVALID_HANDLE;
	pConnect = (CConnect *)ConnectionHandle;
	rc = pConnect->SetConnectAttr(Attribute, Value, StringLength);
	return rc;
}

SQLRETURN ODBC::GetConnectAttr(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER ValuePtr,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLengthPtr)
{
	SQLRETURN	rc;
	CConnect	*pConnect;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_DBC, ConnectionHandle))
		return SQL_INVALID_HANDLE;
	pConnect = (CConnect *)ConnectionHandle;
	rc = pConnect->GetConnectAttr(Attribute, ValuePtr, BufferLength, StringLengthPtr);
	return rc;

}

SQLRETURN ODBC::GetInfo(SQLHDBC ConnectionHandle,
           SQLUSMALLINT InfoType, 
		   SQLPOINTER InfoValuePtr,
           SQLSMALLINT BufferLength,
		   SQLSMALLINT *StringLengthPtr)
{
	SQLRETURN	rc;
	CConnect	*pConnect;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_DBC, ConnectionHandle))
		return SQL_INVALID_HANDLE;
	pConnect = (CConnect *)ConnectionHandle;
	rc = pConnect->GetInfo(InfoType, InfoValuePtr, BufferLength, StringLengthPtr);
	return rc;
}
// ODBC Standard
SQLRETURN ODBC::BrowseConnect(SQLHDBC  ConnectionHandle,
    SQLCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLCHAR 		  *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT       *StringLength2Ptr)
{
	SQLRETURN	rc;
	CConnect	*pConnect;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_DBC, ConnectionHandle))
		return SQL_INVALID_HANDLE;
	pConnect = (CConnect *)ConnectionHandle;
	rc = pConnect->BrowseConnect(InConnectionString, StringLength1, OutConnectionString,
			BufferLength, StringLength2Ptr);
	return rc;
}

SQLRETURN ODBC::DriverConnect(SQLHDBC            ConnectionHandle,
    SQLHWND            WindowHandle,
    SQLCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLCHAR           *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT 	  *StringLength2Ptr,
    SQLUSMALLINT       DriverCompletion)
{
	
	SQLRETURN	rc;
	CConnect	*pConnect;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_DBC, ConnectionHandle))
		return SQL_INVALID_HANDLE;
	pConnect = (CConnect *)ConnectionHandle;
	rc = pConnect->DriverConnect(WindowHandle, InConnectionString, StringLength1, OutConnectionString,
			BufferLength, StringLength2Ptr, DriverCompletion);
	return rc;
}

extern SQLRETURN ODBC::NativeSql(
    SQLHDBC            ConnectionHandle,
    SQLCHAR 		  *InStatementText,
    SQLINTEGER         TextLength1,
    SQLCHAR 		  *OutStatementText,
    SQLINTEGER         BufferLength,
    SQLINTEGER 		  *TextLength2Ptr)
{
	SQLRETURN		rc = SQL_SUCCESS;
	CConnect		*pConnect;
	SQLINTEGER		InStatementTextLen;
	int  OutStatementTextTransLen;
	char errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];

	short			InStrLen;
	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_DBC, ConnectionHandle))
		return SQL_INVALID_HANDLE;
	
	pConnect = (CConnect *)ConnectionHandle;
		
	InStatementTextLen = pConnect->m_ICUConv->FindStrLength((const char *)InStatementText, TextLength1);
	
	if(pConnect->m_ICUConv->isAppUTF16())
		BufferLength = BufferLength*2;
	
	if (OutStatementText != NULL)
	{
 //SQLRETURN ICUConverter::OutArgTranslationHelper(SQLCHAR* arg, int argLen, char * dest, int destLen, int *transLen, char *errorMsg, bool LengthInUChars)
		if(pConnect->m_ICUConv->m_AppUnicodeType == APP_UNICODE_TYPE_UTF16)
			rc = pConnect->m_ICUConv->InputArgToWCharHelper(InStatementText, InStatementTextLen,
							(UChar*)OutStatementText, BufferLength, &OutStatementTextTransLen, errorMsg);
		else
			rc = pConnect->m_ICUConv->InArgTranslationHelper(InStatementText, InStatementTextLen,
							(char *)OutStatementText, BufferLength, &OutStatementTextTransLen, errorMsg);
		
		if(rc != SQL_ERROR)
		{
			if(rc == SQL_SUCCESS_WITH_INFO)
				pConnect->setDiagRec(DRIVER_ERROR, IDS_01_004);
		}
	}
	if (TextLength2Ptr != NULL)
		*TextLength2Ptr = OutStatementTextTransLen;
	return rc;
}


