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
#include "SQLConnect.h"
#include "CConnect.h"
#include "tdm_odbcDrvMsg.h"


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

	pConnect = (CConnect *)ConnectionHandle;
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		rc = pConnect->Connect(ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3,
			TRUE);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN ODBC::Disconnect(SQLHDBC ConnectionHandle)
{

	SQLRETURN rc;
	CConnect	*pConnect;

	pConnect = (CConnect *)ConnectionHandle;
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		rc = pConnect->Disconnect();
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN ODBC::SetConnectAttr(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	SQLRETURN	rc;
	CConnect	*pConnect;

	pConnect = (CConnect *)ConnectionHandle;
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		rc = pConnect->SetConnectAttr(Attribute, Value, StringLength);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
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

	pConnect = (CConnect *)ConnectionHandle;
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		rc = pConnect->GetConnectAttr(Attribute, ValuePtr, BufferLength, StringLengthPtr);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
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

	pConnect = (CConnect *)ConnectionHandle;

	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		rc = pConnect->GetInfo(InfoType, InfoValuePtr, BufferLength, StringLengthPtr);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
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

	pConnect = (CConnect *)ConnectionHandle;
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		rc = pConnect->BrowseConnect(InConnectionString, StringLength1, OutConnectionString,
				BufferLength, StringLength2Ptr);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
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

	pConnect = (CConnect *)ConnectionHandle;
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		rc = pConnect->DriverConnect(WindowHandle, InConnectionString, StringLength1, OutConnectionString,
				BufferLength, StringLength2Ptr, DriverCompletion);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

extern SQLRETURN ODBC::NativeSql(
    SQLHDBC            ConnectionHandle,
    SQLWCHAR 		  *InStatementTextW,
    SQLINTEGER         TextLength1,
    SQLWCHAR 		  *OutStatementTextW,
    SQLINTEGER         BufferLength,
    SQLINTEGER 		  *TextLength2Ptr)
{
	SQLRETURN		rc = SQL_SUCCESS;
	CConnect		*pConnect;


	pConnect = (CConnect *)ConnectionHandle;
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pConnect->NativeSql(InStatementTextW,TextLength1,OutStatementTextW,BufferLength,TextLength2Ptr);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}
