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

#include "neofunc.h"
#ifndef unixcli
#include "cdnv.h"
#endif
#include "cconnect.h"
#include "cstmt.h"
#include "cdesc.h"
#include "dmfunctions.h"
#include "drvrmanager.h"
#ifdef unixcli
#include "unix_extra.h"
#endif
#include <sqlucode.h>
extern  CRITICAL_SECTION2 g_csWrite;
bool isODBCVersionSet = false;


using namespace ODBC;

SQLRETURN  SQL_API SQLAllocHandle(SQLSMALLINT HandleType,
				SQLHANDLE InputHandle, 
				SQLHANDLE *OutputHandle)
{
	SQLRETURN	rc;

	switch (HandleType)
    {
		case SQL_HANDLE_ENV:
			if (!OutputHandle)
				return SQL_ERROR;
			*OutputHandle = SQL_NULL_HENV;
			if (InputHandle != SQL_NULL_HANDLE)
				return SQL_INVALID_HANDLE;
			break;
		case SQL_HANDLE_DBC:
			{
				GENV (genv, InputHandle);
				ODBC_LOCK ();
				if (!IS_VALID_HENV (genv))
				{
					ODBC_UNLOCK ();
					return SQL_INVALID_HANDLE;
				}
				CLEAR_ERRORS (genv);
				if (!OutputHandle)
				{
					PUSHSQLERR (genv, IDS_S1_009);
					ODBC_UNLOCK ();
					RETURNCODE (genv,SQL_ERROR);
					return SQL_ERROR;
				}
				if (!isODBCVersionSet)  				/* Fix CR 4930: */
				{										/* Check if the SQL_ATTR_ODBC_VERSION has been set or not. */
					PUSHSQLERR (genv, IDS_HY_010);      /* DM does not care what the value is. */
					ODBC_UNLOCK ();
					RETURNCODE (genv,SQL_ERROR);
					return SQL_ERROR;
				}
				ODBC_UNLOCK ();
			}
			break;
		case SQL_HANDLE_STMT:
            if (!IS_VALID_HDBC (InputHandle))
                return SQL_INVALID_HANDLE;
		    rc = NeoAllocHandle(SQL_HANDLE_STMT, InputHandle, (SQLHSTMT*)OutputHandle);
			//rc = SQLAllocStmt(InputHandle,(SQLHSTMT *)OutputHandle);
			RETURNCODE (InputHandle,rc);
			return rc;
			break;
		case SQL_HANDLE_DESC:
			{
				CONN (con, InputHandle);
				ENTER_HDBC (con);
				CLEAR_ERRORS (con);
				if (!OutputHandle)
				{
					PUSHSQLERR (con, IDS_S1_009);
					RETURNCODE (con, SQL_ERROR);
					LEAVE_HDBC (con, SQL_ERROR);
				}
				if (con->getConnectionStatus() == SQL_CD_TRUE)
				{
					PUSHSQLERR (con, IDS_08_003);
					RETURNCODE (con, SQL_ERROR);
					LEAVE_HDBC (con, SQL_ERROR);
				}
				rc = NeoAllocHandle(HandleType,InputHandle,OutputHandle);
				RETURNCODE (con, rc);
				LEAVE_HDBC (con, rc);
			}
			break;
		default:
			ODBC_LOCK ();
			if (IS_VALID_HDBC (InputHandle))
			{
				CONN (con, InputHandle);
				PUSHSQLERR (con, IDS_HY_092);
				RETURNCODE (con, SQL_ERROR);
				ODBC_UNLOCK ();
				return SQL_ERROR;
			}
			else if (IS_VALID_HENV (InputHandle))
			{
				GENV (genv, InputHandle);
				PUSHSQLERR (genv, IDS_HY_092);
				RETURNCODE (genv, SQL_ERROR);
				ODBC_UNLOCK ();
				return SQL_ERROR;
			}
			ODBC_UNLOCK ();
			return SQL_INVALID_HANDLE;
	}
	rc = NeoAllocHandle(HandleType,InputHandle,OutputHandle);
	RETURNCODE (InputHandle,rc);
	return rc;
}


SQLRETURN  SQL_API SQLFreeHandle(SQLSMALLINT HandleType, 
				SQLHANDLE Handle)
{
	SQLRETURN	rc;

	switch (HandleType)
    {
		case SQL_HANDLE_ENV:
			if (IS_VALID_HENV (Handle))
			{
				GENV (env, Handle);
				CLEAR_ERRORS (env);

#ifdef VERSION3 
				std::list<CHandlePtr>::iterator	i = (env->m_ConnectCollect).begin();
				CConnect* pConnect;
				if (i != (env->m_ConnectCollect).end())
					if ((pConnect = (CConnect*)(*i)) != NULL)
					{
						PUSHSQLERR (env, IDS_S1_010);
						RETURNCODE (env, SQL_ERROR);
						return SQL_ERROR;
					}
#else
				RWTPtrSlistIterator<CHandle> i(env->m_ConnectCollect);
				CConnect* pConnect;

				if ((pConnect = (CConnect*)i()) != NULL)
				{
					PUSHSQLERR (env, IDS_S1_010);
					RETURNCODE (env, SQL_ERROR);
					return SQL_ERROR;
				}
#endif
			}
			else
				return SQL_INVALID_HANDLE;
			break;
		case SQL_HANDLE_DBC:
			if (IS_VALID_HDBC (Handle))
			{
				CONN (con, Handle);
				CLEAR_ERRORS (con);
				if (con->state != en_dbc_allocated)
				{
					PUSHSQLERR (con, IDS_S1_010);
					RETURNCODE (con, SQL_ERROR);
					return SQL_ERROR;
				}

			}
			else
				return SQL_INVALID_HANDLE;
			break;
		case SQL_HANDLE_STMT:
			if (IS_VALID_HSTMT (Handle))
			{
				STMT (stmt, Handle);
				CLEAR_ERRORS (stmt);
			}
			else
				return SQL_INVALID_HANDLE;
			break;
		case SQL_HANDLE_DESC:
			if (IS_VALID_HDESC (Handle))
			{
				DESC (pdesc, Handle);
				CONN (pdbc, pdesc->getDescConnect());

				if (IS_VALID_HSTMT (pdesc->getDescStmt()))
				{			/* the desc handle is implicit */
					PUSHSQLERR (pdesc, IDS_HY_017);
					RETURNCODE (pdesc, SQL_ERROR);
					return SQL_ERROR;
				}
				CLEAR_ERRORS (pdesc);
				pdbc->restoreDefaultDescriptor(pdesc);
			}
			else
				return SQL_INVALID_HANDLE;
			break;
		default:
			ODBC_LOCK ();
			if (IS_VALID_HDBC (Handle))
			{
				CONN (con, Handle);
				PUSHSQLERR (con, IDS_HY_092);
				RETURNCODE (con, SQL_ERROR);
				ODBC_UNLOCK ();
				return SQL_ERROR;
			}
			else if (IS_VALID_HENV (Handle))
			{
				GENV (genv, Handle);
				PUSHSQLERR (genv, IDS_HY_092);
				RETURNCODE (genv, SQL_ERROR);
				ODBC_UNLOCK ();
				return SQL_ERROR;
			}
			ODBC_UNLOCK ();
			return SQL_INVALID_HANDLE;
	}
	rc = NeoFreeHandle(HandleType, Handle);
#ifndef unixcli
	RETURNCODE (Handle, rc);
#endif
	return rc;
}


SQLRETURN SQL_API SQLGetDiagRec_common(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber,
				SQLCHAR *Sqlstate,
				SQLINTEGER *NativeError, 
				SQLCHAR *MessageText,
				SQLSMALLINT BufferLength, 
				SQLSMALLINT *TextLength,
				bool isWideCall)
{
	SQLRETURN	rc;

	if (RecNumber < 1)
		return SQL_ERROR;
	if (BufferLength < 0)
		return SQL_ERROR;
	ODBC_LOCK ();
	switch (HandleType)
    {
		case SQL_HANDLE_ENV:
			if (!IS_VALID_HENV (Handle))
			{
				ODBC_UNLOCK ();
				return SQL_INVALID_HANDLE;
			}
			break;
		case SQL_HANDLE_DBC:
			if (!IS_VALID_HDBC (Handle))
			{
				ODBC_UNLOCK ();
				return SQL_INVALID_HANDLE;
			}
			break;
		case SQL_HANDLE_STMT:
			if (!IS_VALID_HSTMT (Handle))
			{
				ODBC_UNLOCK ();
				return SQL_INVALID_HANDLE;
			}
			break;
		case SQL_HANDLE_DESC:
			if (!IS_VALID_HDESC (Handle))
			{
				ODBC_UNLOCK ();
				return SQL_INVALID_HANDLE;
			}
			break;
		default:
			ODBC_UNLOCK ();
			return SQL_INVALID_HANDLE;
	}
	rc = NeoGetDiagRec(HandleType,Handle,RecNumber,Sqlstate,NativeError,MessageText,BufferLength,TextLength,isWideCall);
	ODBC_UNLOCK ();
	return rc;
}
SQLRETURN SQL_API SQLGetDiagRec(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber,
				SQLCHAR *Sqlstate,
				SQLINTEGER *NativeError, 
				SQLCHAR *MessageText,
				SQLSMALLINT BufferLength, 
				SQLSMALLINT *TextLength)
{
	return SQLGetDiagRec_common(HandleType, 
				Handle,
				RecNumber,
				Sqlstate,
				NativeError, 
				MessageText,
				BufferLength, 
				TextLength,
				false);
}

SQLRETURN SQL_API SQLGetDiagRecA(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber,
				SQLCHAR *Sqlstate,
				SQLINTEGER *NativeError, 
				SQLCHAR *MessageText,
				SQLSMALLINT BufferLength, 
				SQLSMALLINT *TextLength)
{
	return SQLGetDiagRec_common(HandleType, 
				Handle,
				RecNumber,
				Sqlstate,
				NativeError, 
				MessageText,
				BufferLength, 
				TextLength,
				false);
}

SQLRETURN SQL_API SQLGetDiagRecW(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber,
				SQLWCHAR *Sqlstate,
				SQLINTEGER *NativeError, 
				SQLWCHAR *MessageText,
				SQLSMALLINT BufferLength, 
				SQLSMALLINT *TextLength)
{
	return SQLGetDiagRec_common(HandleType, 
				Handle,
				RecNumber,
										 (SQLCHAR*)Sqlstate,
				NativeError, 
										 (SQLCHAR*)MessageText,
				BufferLength, 
				TextLength,
				true);
}

SQLRETURN SQL_API SQLGetDiagField_common(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber, 
				SQLSMALLINT DiagIdentifier,
				SQLPOINTER DiagInfo, 
				SQLSMALLINT BufferLength,
				SQLSMALLINT *StringLength,
				bool isWideCall)
{	
	GENV (genv, Handle);
	CONN (con, Handle);
	STMT (stmt, Handle);
	DESC (desc, Handle);
	SQLRETURN	rc;

	ODBC_LOCK ();
	switch (HandleType)
    {
		case SQL_HANDLE_ENV:
			if (!IS_VALID_HENV (Handle))
			{
				ODBC_UNLOCK ();
				return SQL_INVALID_HANDLE;
			}
			con = NULL;
			stmt = NULL;
			desc = NULL;
			break;
		case SQL_HANDLE_DBC:
			if (!IS_VALID_HDBC (Handle))
			{
				ODBC_UNLOCK ();
				return SQL_INVALID_HANDLE;
			}
			genv = con->getEnvHandle();
			stmt = NULL;
			desc = NULL;
			break;
		case SQL_HANDLE_STMT:
			if (!IS_VALID_HSTMT (Handle))
			{
				ODBC_UNLOCK ();
				return SQL_INVALID_HANDLE;
			}
			con = stmt->getConnectHandle();
			genv = con->getEnvHandle();
			desc = NULL;
			break;
		case SQL_HANDLE_DESC:
			if (!IS_VALID_HDESC (Handle))
			{
				ODBC_UNLOCK ();
				return SQL_INVALID_HANDLE;
			}
			stmt = desc->getDescStmt();
			con = desc->getDescConnect();
			genv = con->getEnvHandle();
			break;
		default:
			ODBC_UNLOCK ();
			return SQL_INVALID_HANDLE;
    }
/* Header record */
	switch (DiagIdentifier)
	{
		case SQL_DIAG_NUMBER:
			if (RecNumber != 0)
			{
				ODBC_UNLOCK ();
				return SQL_ERROR;
			}
			break;
		case SQL_DIAG_RETURNCODE:
			if (RecNumber != 0)
			{
				ODBC_UNLOCK ();
				return SQL_ERROR;
			}
			if (desc != NULL)
				rc = desc->returncode;
			else if (stmt != NULL)
				rc = stmt->returncode;
			else if (con != NULL)
				rc = con->returncode;
			else if (genv != NULL)
				rc = genv->returncode;
			else
			{
				ODBC_UNLOCK ();
				return SQL_INVALID_HANDLE;
			}
			*(SQLRETURN*)DiagInfo = rc;
			return SQL_SUCCESS;
			break;
		case SQL_DIAG_ROW_COUNT:
		{
			if (RecNumber != 0)
			{
				ODBC_UNLOCK ();
				return SQL_ERROR;
			}
			if (HandleType != SQL_HANDLE_STMT || !stmt)
			{
				ODBC_UNLOCK ();
				return SQL_ERROR;
			}
			if (stmt->state != en_stmt_executed && stmt->state != en_stmt_cursoropen)
			{
				ODBC_UNLOCK ();
				return SQL_ERROR;
			}
			if (!con)
			{
				ODBC_UNLOCK ();
				return SQL_INVALID_HANDLE;
			}
			break;
		}
		case SQL_DIAG_CURSOR_ROW_COUNT:
		case SQL_DIAG_DYNAMIC_FUNCTION:
		case SQL_DIAG_DYNAMIC_FUNCTION_CODE:
		{
			if (RecNumber != 0)
			{
				ODBC_UNLOCK ();
				return SQL_ERROR;
			}
			if (HandleType != SQL_HANDLE_STMT || !stmt)
			{
				ODBC_UNLOCK ();
				return SQL_ERROR;
			}
			if (stmt->state != en_stmt_executed && stmt->state != en_stmt_cursoropen)
			{
				ODBC_UNLOCK ();
				return SQL_ERROR;
			}
			if (!con)
			{
				ODBC_UNLOCK ();
				return SQL_INVALID_HANDLE;
			}
			break;
		}
	}
	rc = NeoGetDiagField(HandleType,Handle,RecNumber,DiagIdentifier,DiagInfo,BufferLength,StringLength,isWideCall);
	ODBC_UNLOCK ();
	return rc;
}

SQLRETURN SQL_API SQLGetDiagField(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber, 
				SQLSMALLINT DiagIdentifier,
				SQLPOINTER DiagInfo, 
				SQLSMALLINT BufferLength,
				SQLSMALLINT *StringLength)
{	
	return SQLGetDiagField_common(HandleType, 
				Handle,
				RecNumber, 
				DiagIdentifier,
				DiagInfo, 
				BufferLength,
				StringLength,
				false);
}			
 
SQLRETURN SQL_API SQLGetDiagFieldA(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber, 
				SQLSMALLINT DiagIdentifier,
				SQLPOINTER DiagInfo, 
				SQLSMALLINT BufferLength,
				SQLSMALLINT *StringLength)
{	
	return SQLGetDiagField_common(HandleType, 
				Handle,
				RecNumber, 
				DiagIdentifier,
				DiagInfo, 
				BufferLength,
				StringLength,
				false);
}	

SQLRETURN SQL_API SQLGetDiagFieldW(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber, 
				SQLSMALLINT DiagIdentifier,
				SQLPOINTER DiagInfo, 
				SQLSMALLINT BufferLength,
				SQLSMALLINT *StringLength)
{	
	return SQLGetDiagField_common(HandleType, 
				Handle,
				RecNumber, 
				DiagIdentifier,
				DiagInfo, 
				BufferLength,
				StringLength,
				true);
}	

SQLRETURN SQL_API SQLEndTran(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT CompletionType)
{
	SQLRETURN	rc;

	switch (HandleType)
    {
		case SQL_HANDLE_DBC:
		case SQL_HANDLE_ENV:
			break;
		default:
			return SQL_INVALID_HANDLE;
    }
	rc = SQLTransact (
		HandleType == SQL_HANDLE_ENV ? Handle : SQL_NULL_HENV,
		HandleType == SQL_HANDLE_DBC ? Handle : SQL_NULL_HDBC,
		CompletionType);
	RETURNCODE (Handle, rc);
	return rc;
//	rc = SQLEndTranA(HandleType,Handle,CompletionType);
//	return rc;
}

SQLRETURN SQL_API SQLConnect_common(SQLHDBC ConnectionHandle,
           SQLCHAR *ServerName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *UserName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *Authentication, 
		   SQLSMALLINT NameLength3,
		   bool isWideCall)
{
	SQLRETURN	rc;
	CONN (pdbc, ConnectionHandle);
	ODBC_LOCK();
	if (!IS_VALID_HDBC (pdbc))
    {
		ODBC_UNLOCK();
		return SQL_INVALID_HANDLE;
    }
	CLEAR_ERRORS(pdbc);

	/* check arguments */
	if ((NameLength1 < 0 && NameLength1 != SQL_NTS)
		|| (NameLength2 < 0 && NameLength2 != SQL_NTS)
		|| (NameLength3 < 0 && NameLength3 != SQL_NTS)
		|| (NameLength1 > SQL_MAX_DSN_LENGTH))
    {
		PUSHSQLERR (pdbc, IDS_S1_010);
		RETURNCODE (pdbc, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	if (ServerName == NULL || NameLength1 == 0)
    {
		PUSHSQLERR (pdbc, IDS_IM_002);
		RETURNCODE (pdbc, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
	}
	/* check state */
	if (pdbc->state != en_dbc_allocated)
    {
		PUSHSQLERR (pdbc, IDS_08_002);
		RETURNCODE (pdbc, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }

	rc = NeoConnect(ConnectionHandle,ServerName,NameLength1,UserName,NameLength2,Authentication,NameLength3,isWideCall);

	RETURNCODE (ConnectionHandle, rc);
	if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    {
		ODBC_UNLOCK ();
		return rc;
    }
	/* state transition */
	pdbc->state = en_dbc_connected;
	ODBC_UNLOCK ();
	return rc;
}

SQLRETURN SQL_API SQLConnect(SQLHDBC ConnectionHandle,
           SQLCHAR *ServerName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *UserName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *Authentication, 
		   SQLSMALLINT NameLength3)
{
   return SQLConnect_common(ConnectionHandle,
                            ServerName, 
                            NameLength1,
                            UserName, 
                            NameLength2,
                            Authentication, 
                            NameLength3,
                            false);
}
SQLRETURN SQL_API SQLConnectA(SQLHDBC ConnectionHandle,
           SQLCHAR *ServerName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *UserName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *Authentication, 
		   SQLSMALLINT NameLength3)
{
   return SQLConnect_common(ConnectionHandle,
                            ServerName, 
                            NameLength1,
                            UserName, 
                            NameLength2,
                            Authentication, 
                            NameLength3,
                            false);
}

SQLRETURN SQL_API SQLConnectW(SQLHDBC ConnectionHandle,
           SQLWCHAR *ServerName, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *UserName, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *Authentication, 
		   SQLSMALLINT NameLength3)
{
   return SQLConnect_common(ConnectionHandle,
                            (SQLCHAR*)ServerName, 
                            NameLength1,
                            (SQLCHAR*)UserName, 
                            NameLength2,
                            (SQLCHAR*)Authentication, 
                            NameLength3,
                            true);
}

SQLRETURN SQL_API SQLDisconnect(SQLHDBC ConnectionHandle)
{
	SQLRETURN	rc;
	CONN (pdbc, ConnectionHandle);
	int sqlstat = 0;

	ODBC_LOCK ();
	if (!IS_VALID_HDBC (pdbc))
    {
		ODBC_UNLOCK ();
		return SQL_INVALID_HANDLE;
    }
	CLEAR_ERRORS(pdbc);
	/* check hdbc state */
	if (pdbc->state == en_dbc_allocated)
    {
		sqlstat = IDS_08_003;
    }
	/* check stmt(s) state */
#ifdef VERSION3 
	std::list<CHandlePtr>::iterator	i;
	CStmt* pStmt;
	for( i = pdbc->m_StmtCollect.begin(); i != pdbc->m_StmtCollect.end(); ++i)
		if ((pStmt = (CStmt*)(*i)) != NULL)
#else
	RWTPtrSlistIterator<CHandle> i(pdbc->m_StmtCollect);
	CStmt* pStmt;
	while ((pStmt = (CStmt*)i()) != NULL)
#endif
	{
		if (pStmt->state >= en_stmt_needdata || pStmt->asyn_on != en_NullProc)
		{
			sqlstat = IDS_S1_010;
			break;
		}
	}
	if (sqlstat != 0)
    {
		PUSHSQLERR (pdbc, sqlstat);
		RETURNCODE (pdbc, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	rc = NeoDisconnect(ConnectionHandle);

	if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
/* state transition */
		pdbc->state = en_dbc_allocated;
    }
	RETURNCODE (pdbc, rc);
	ODBC_UNLOCK ();
	return rc;
}
 
SQLRETURN  SQL_API SQLSetConnectAttr_common(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength,
		   bool isWideCall)
{
	CONN (con, ConnectionHandle);
	SQLRETURN	rc = SQL_SUCCESS;

	ENTER_HDBC (con);
	switch(Attribute)
	{
		case SQL_ATTR_ASYNC_ENABLE:
			if ((unsigned long)Value != SQL_ASYNC_ENABLE_OFF && (unsigned long)Value != SQL_ASYNC_ENABLE_ON )
			{
				PUSHSQLERR (con, IDS_S1_009);
				RETURNCODE (con, SQL_ERROR);
				LEAVE_HDBC (con, SQL_ERROR);
			}
			break;
	}
	if (Attribute == SQL_ATTR_TRACE)
    {
#if defined(_WIN64) || defined(__LP64__)
        switch ((INT64)Value)
#else
		switch ((SQLINTEGER)Value)
#endif
		{
		case SQL_OPT_TRACE_ON:
			EnterCriticalSection2(&g_csWrite);
			dwGlobalTraceVariable=1;
			pdwGlobalTraceVariable = TraceProcessEntry();
			LeaveCriticalSection2(&g_csWrite);
			break;
		case SQL_OPT_TRACE_OFF:
			EnterCriticalSection2(&g_csWrite);
			dwGlobalTraceVariable=0;
			TraceCloseLogFile();
			LeaveCriticalSection2(&g_csWrite);
			break;
		default:
			PUSHSQLERR (con, IDS_S1_009);
			rc = SQL_ERROR;
		}
		RETURNCODE (con, rc);
		LEAVE_HDBC (con, rc);
    }
	if (Attribute == SQL_ATTR_TRACEFILE)
    /* Tracing file can be set before and after connect 
     * and only meaningful for driver manager. 
     */
	{
		DWORD dwTraceVariable;

		EnterCriticalSection2(&g_csWrite);

		dwTraceVariable = dwGlobalTraceVariable; // save it because TraceCloseLogFile sets trace to OFF
		TraceCloseLogFile();
		sprintf(szGlobalTraceFileName,"%s",(char*)Value);
		dwGlobalTraceVariable = dwTraceVariable;

		LeaveCriticalSection2(&g_csWrite);

		RETURNCODE (con, SQL_SUCCESS);
		LEAVE_HDBC (con, SQL_SUCCESS);
	}
	if (con->state == en_dbc_needdata)
    {
		PUSHSQLERR (con, IDS_HY_010);
		RETURNCODE (con, SQL_ERROR);
		LEAVE_HDBC (con, SQL_ERROR);
    }
	rc = NeoSetConnectAttr(ConnectionHandle,Attribute,Value,StringLength,isWideCall);
	RETURNCODE (con, rc);
	LEAVE_HDBC (con, rc);
}

SQLRETURN  SQL_API SQLSetConnectAttr(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	return SQLSetConnectAttr_common(ConnectionHandle,
           Attribute, 
		   Value,
           StringLength,
		   false);

}

SQLRETURN  SQL_API SQLSetConnectAttrA(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	return SQLSetConnectAttr_common(ConnectionHandle,
           Attribute, 
		   Value,
           StringLength,
		   false);

}
SQLRETURN  SQL_API SQLSetConnectAttrW(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	return SQLSetConnectAttr_common(ConnectionHandle,
           Attribute, 
		   Value,
           StringLength,
		   true);

}

SQLRETURN  SQL_API SQLGetConnectAttr_common(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength,
		   bool isWideCall)
{
	CONN (con, ConnectionHandle);
	SQLRETURN	rc=SQL_SUCCESS;

	ENTER_HDBC (con);
  /* Tracing and tracing file options are only 
   * meaningful for driver manager
   */
	if (Attribute == SQL_ATTR_TRACE)
    {
		*((UDWORD_P *) Value) = (UDWORD_P) dwGlobalTraceVariable;
		RETURNCODE (con, rc);
		LEAVE_HDBC (con, rc);
    }
	if (Attribute == SQL_ATTR_TRACEFILE)
    {
		strcpy ((char*)Value, szGlobalTraceFileName);
		RETURNCODE (con, rc);
		LEAVE_HDBC (con, rc);
    }
	if (con->state == en_dbc_needdata)
    {
		PUSHSQLERR (con, IDS_HY_010);
		RETURNCODE (con, SQL_ERROR);
		LEAVE_HDBC (con, SQL_ERROR);
    }
	rc = NeoGetConnectAttr(ConnectionHandle,Attribute,Value,BufferLength,StringLength,isWideCall);
	RETURNCODE (con, rc);
	LEAVE_HDBC (con, rc);
}
SQLRETURN  SQL_API SQLGetConnectAttr(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength)
{

	return SQLGetConnectAttr_common(ConnectionHandle,
                                           Attribute, 
		                                   Value,
                                           BufferLength,
		                                   StringLength,
		                                   false);

}
SQLRETURN  SQL_API SQLGetConnectAttrA(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength)
{

	return SQLGetConnectAttr_common(ConnectionHandle,
                                           Attribute, 
		                                   Value,
                                           BufferLength,
		                                   StringLength,
		                                   false);

}
SQLRETURN  SQL_API SQLGetConnectAttrW(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength)
{

	return SQLGetConnectAttr_common(ConnectionHandle,
                                           Attribute, 
		                                   Value,
                                           BufferLength,
		                                   StringLength,
		                                   true);

}

SQLRETURN  SQL_API SQLSetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	GENV (genv, EnvironmentHandle);
	SQLRETURN	rc;

	ODBC_LOCK ();
	if (!IS_VALID_HENV (genv))
    {
		ODBC_UNLOCK ();
		return (SQL_INVALID_HANDLE);
    }
	CLEAR_ERRORS (genv);
#ifdef VERSION3 
	if (genv->m_ConnectCollect.size())
#else
	if (genv->m_ConnectCollect.entries())
#endif
    {
		PUSHSQLERR (genv, IDS_HY_010);
		RETURNCODE (genv, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	switch (Attribute)
    {
		case SQL_ATTR_CONNECTION_POOLING:
#if defined(_WIN64) || defined(__LP64__)
            switch ((INT64)Value)
#else
  		    switch ((SQLINTEGER)Value)
#endif
			{
				case SQL_CP_OFF:
				case SQL_CP_ONE_PER_DRIVER:
				case SQL_CP_ONE_PER_HENV:
					PUSHSQLERR (genv, IDS_HY_C00);
					RETURNCODE (genv, SQL_ERROR);
					ODBC_UNLOCK ();
					return SQL_ERROR;	/* not implemented yet */
				default:
					PUSHSQLERR (genv, IDS_HY_024);
					RETURNCODE (genv, SQL_ERROR);
					ODBC_UNLOCK ();
					return SQL_ERROR;
			}
		case SQL_ATTR_CP_MATCH:
#if defined(_WIN64) || defined(__LP64__)
            switch ((INT64)Value)
#else
   		    switch ((SQLINTEGER)Value)
#endif
			{
				case SQL_CP_STRICT_MATCH:
				case SQL_CP_RELAXED_MATCH:
					PUSHSQLERR (genv, IDS_HY_C00);
					RETURNCODE (genv, SQL_ERROR);
					ODBC_UNLOCK ();
					return SQL_ERROR;	/* not implemented yet */
				default:
					PUSHSQLERR (genv, IDS_HY_024);
					RETURNCODE (genv, SQL_ERROR);
					ODBC_UNLOCK ();
					return SQL_ERROR;
			}
		case SQL_ATTR_ODBC_VERSION:
			isODBCVersionSet = true; /* Fix CR 4930: Mark the SQL_ATTR_ODBC_VERSION has been set*/
#if defined(_WIN64) || defined(__LP64__)
            switch ((INT64)Value)
#else
   		    switch ((SQLINTEGER)Value)
#endif
			{
				case SQL_OV_ODBC2:
				case SQL_OV_ODBC3:
					break;
				default:
					PUSHSQLERR (genv, IDS_HY_024);
					RETURNCODE (genv, SQL_ERROR);
					ODBC_UNLOCK ();
					return SQL_ERROR;
			}
			break;
		case SQL_ATTR_OUTPUT_NTS:
#if defined(_WIN64) || defined(__LP64__)
            switch ((INT64)Value)
#else
   		    switch ((SQLINTEGER)Value)
#endif
			{
				case SQL_TRUE:
					RETURNCODE (genv, SQL_SUCCESS);
					ODBC_UNLOCK ();
					return SQL_SUCCESS;
				case SQL_FALSE:
					PUSHSQLERR (genv, IDS_HY_C00);
					RETURNCODE (genv, SQL_ERROR);
					ODBC_UNLOCK ();
					return SQL_ERROR;
				default:
					PUSHSQLERR (genv, IDS_HY_024);
					RETURNCODE (genv, SQL_ERROR);
					ODBC_UNLOCK ();
					return SQL_ERROR;
			}
#if defined(ASYNCIO) && defined(DBSELECT)
		case SQL_ATTR_DBSELECT:
		case SQL_ATTR_DBSELECT_QUEUE_LEN:
			break;
#endif

		default:
			PUSHSQLERR (genv, IDS_HY_092);
			RETURNCODE (genv, SQL_ERROR);
			ODBC_UNLOCK ();
			return SQL_ERROR;
    }
	rc = NeoSetEnvAttr(EnvironmentHandle,Attribute,Value,StringLength);
	RETURNCODE (genv, rc);
	ODBC_UNLOCK ();
	return rc;
}

SQLRETURN  SQL_API SQLGetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength)
{
	GENV (genv, EnvironmentHandle);
	SQLRETURN	rc;
	HDBC con;

	ODBC_LOCK ();
	if (!IS_VALID_HENV (genv))
    {
		ODBC_UNLOCK ();
		return (SQL_INVALID_HANDLE);
    }
	CLEAR_ERRORS (genv);

	if (Attribute != SQL_ATTR_CONNECTION_POOLING &&
		Attribute != SQL_ATTR_CP_MATCH &&
		Attribute != SQL_ATTR_ODBC_VERSION && 
		Attribute != SQL_ATTR_OUTPUT_NTS)
    {
		PUSHSQLERR (genv, IDS_HY_092);
		RETURNCODE (genv, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	/* ODBC DM env attributes */
	if (Attribute == SQL_ATTR_ODBC_VERSION)
    {
    }
	if (Attribute == SQL_ATTR_CONNECTION_POOLING)
    {
		if (Value)
			*((SQLUINTEGER *) Value) = SQL_CP_OFF;
		RETURNCODE (genv, SQL_SUCCESS);
		ODBC_UNLOCK ();
		return SQL_SUCCESS;
    }
	if (Attribute == SQL_ATTR_CP_MATCH)
    {
		if (Value)
			*((SQLUINTEGER *) Value) = SQL_CP_STRICT_MATCH;
		RETURNCODE (genv, SQL_SUCCESS);
		ODBC_UNLOCK ();
		return SQL_SUCCESS;
    }
	if (Attribute == SQL_ATTR_OUTPUT_NTS)
    {
		if (Value)
			*((SQLINTEGER *) Value) = SQL_TRUE;
		RETURNCODE (genv, SQL_SUCCESS);
		ODBC_UNLOCK ();
		return SQL_SUCCESS;
    }
	rc = NeoGetEnvAttr(EnvironmentHandle,Attribute,Value,BufferLength,StringLength);
	RETURNCODE (genv, rc);
	ODBC_UNLOCK ();
	return rc;
}

SQLRETURN  SQL_API SQLSetStmtAttr_common(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength,
		   bool isWideCall)
{
	STMT (stmt, StatementHandle);
	SQLRETURN	rc;

	ENTER_STMT (stmt);
	if (stmt->state == en_stmt_needdata)
    {
		PUSHSQLERR (stmt, IDS_HY_010);
		RETURNCODE (stmt, SQL_ERROR);
		LEAVE_STMT (stmt, SQL_ERROR);
    }
	switch (Attribute)
    {
		case SQL_ATTR_ASYNC_ENABLE:
			if ((unsigned long)Value != SQL_ASYNC_ENABLE_OFF && (unsigned long)Value != SQL_ASYNC_ENABLE_ON )
			{
				PUSHSQLERR (stmt, IDS_S1_009);
				RETURNCODE (stmt, SQL_ERROR);
				LEAVE_STMT (stmt, SQL_ERROR);
			}
			break;
		case SQL_ATTR_APP_PARAM_DESC:
		case SQL_ATTR_APP_ROW_DESC:
			if (Value != SQL_NULL_HDESC)
			{
				if (!IS_VALID_HDESC (Value))
				{
					PUSHSQLERR (stmt, IDS_HY_024);
					RETURNCODE (stmt, SQL_ERROR);
					LEAVE_STMT (stmt, SQL_ERROR);
				}
				else
				{
					DESC (pdesc, Value);
					if (pdesc->getDescConnect() != stmt->getConnectHandle() || IS_VALID_HSTMT (pdesc->getDescStmt()))
					{
						PUSHSQLERR (stmt, IDS_HY_017);
						RETURNCODE (stmt, SQL_ERROR);
						LEAVE_STMT (stmt, SQL_ERROR);
					}
				}
			}
			break;
		case SQL_ATTR_IMP_PARAM_DESC:
		case SQL_ATTR_IMP_ROW_DESC:
			PUSHSQLERR (stmt, IDS_HY_017);
			RETURNCODE (stmt, SQL_ERROR);
			LEAVE_STMT (stmt, SQL_ERROR);
			break;
		default:
			break;
	}
	rc = NeoSetStmtAttr(StatementHandle,Attribute,Value,StringLength,isWideCall);
	RETURNCODE (stmt, rc);
	LEAVE_STMT (stmt, rc);
}

SQLRETURN  SQL_API SQLSetStmtAttr(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	return SQLSetStmtAttr_common(StatementHandle,
           Attribute, 
		   Value,
           StringLength,
		   false);

}

SQLRETURN  SQL_API SQLSetStmtAttrA(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	return SQLSetStmtAttr_common(StatementHandle,
           Attribute, 
		   Value,
           StringLength,
		   false);

}

SQLRETURN  SQL_API SQLSetStmtAttrW(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	return SQLSetStmtAttr_common(StatementHandle,
           Attribute, 
		   Value,
           StringLength,
		   true);

}

SQLRETURN  SQL_API SQLGetStmtAttr_common(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength,
		   bool isWideCall)
{
	STMT (stmt, StatementHandle);
	SQLRETURN	rc;

	ENTER_STMT (stmt);
	rc = NeoGetStmtAttr(StatementHandle,Attribute,Value,BufferLength,StringLength,isWideCall);
	RETURNCODE (stmt, rc);
	LEAVE_STMT (stmt, rc);
}

SQLRETURN  SQL_API SQLGetStmtAttr(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength)
{
	return SQLGetStmtAttr_common(StatementHandle,
           Attribute, 
		   Value,
           BufferLength,
		   StringLength,
		   false);
}

SQLRETURN  SQL_API SQLGetStmtAttrA(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength)
{
	return SQLGetStmtAttr_common(StatementHandle,
           Attribute, 
		   Value,
           BufferLength,
		   StringLength,
		   false);
}

SQLRETURN  SQL_API SQLGetStmtAttrW(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength)
{
	return SQLGetStmtAttr_common(StatementHandle,
           Attribute, 
		   Value,
           BufferLength,
		   StringLength,
		   true);
}



SQLRETURN  SQL_API SQLGetInfo_common(SQLHDBC ConnectionHandle,
           SQLUSMALLINT InfoType, 
		   SQLPOINTER InfoValuePtr,
           SQLSMALLINT BufferLength,
		   SQLSMALLINT *StringLengthPtr,
		   bool isWideCall)
{
	CONN (pdbc, ConnectionHandle);
	SQLRETURN	rc=SQL_SUCCESS;
    SQLULEN dword;
	int size = 0, len = 0;
	char buf[16] =  {'\0'};
#ifdef VERSION3 
			std::list<CHandlePtr>::iterator	i;
#else
			RWTPtrSlistIterator<CHandle> i(pdbc->m_StmtCollect);
#endif
			CStmt *tpstmt,*pstmt=NULL;

	ENTER_HDBC(pdbc);
	if (BufferLength < 0)
    {
		PUSHSQLERR (pdbc, IDS_S1_090);
		RETURNCODE (pdbc, SQL_ERROR);
		LEAVE_HDBC (pdbc, SQL_ERROR);
    }
	if (InfoType == SQL_DM_VER)
	{
		sprintf (buf, "%02d.%02d.%04d.%04d", 3, 52, 0, 0);
		if (InfoValuePtr != NULL && BufferLength > 0)
		{
			len = strlen(buf);
			if (len > BufferLength - 1)
			{
				len = BufferLength - 1;
				PUSHSQLERR (pdbc, IDS_01_004);
				rc = SQL_SUCCESS_WITH_INFO;
			}
			strncpy((char *)InfoValuePtr, buf, len);
			((char *) InfoValuePtr)[len] = '\0';
		}
		if (StringLengthPtr != NULL)
		{
			*StringLengthPtr = (SWORD) len;
		}
		RETURNCODE (pdbc, rc);
		LEAVE_HDBC (pdbc, rc);
	}
	if (InfoType == SQL_ODBC_VER) // || InfoType == SQL_DRIVER_ODBC_VER) - DRIVER_ODBC_VER should not be implemented by DM
    {
		sprintf (buf, "%02d.%02d.%04d", 3, 52, 0);
		//sprintf (buf, "%02d.%02d", 3, 51);
		if (InfoValuePtr != NULL && BufferLength > 0)
		{
			len = strlen(buf);
			if (len > BufferLength - 1)
			{
				len = BufferLength - 1;
				PUSHSQLERR (pdbc, IDS_01_004);
				rc = SQL_SUCCESS_WITH_INFO;
			}
			strncpy((char *)InfoValuePtr, buf, len);
			((char *) InfoValuePtr)[len] = '\0';
		}
		if (StringLengthPtr != NULL)
		{
			*StringLengthPtr = (SWORD) len;
		}
		RETURNCODE (pdbc, rc);
		LEAVE_HDBC (pdbc, rc);
    }
	if (pdbc->state == en_dbc_allocated || pdbc->state == en_dbc_needdata)
    {
		PUSHSQLERR (pdbc, IDS_08_003);
		RETURNCODE (pdbc, SQL_ERROR);
		LEAVE_HDBC (pdbc, SQL_ERROR);
    }
	switch (InfoType)
    {
		case SQL_DRIVER_HDBC:
			dword = (SQLULEN)(pdbc);
			size = sizeof (dword);
			break;
		case SQL_DRIVER_HENV:
			dword = (SQLULEN)(pdbc->getEnvHandle());
			size = sizeof (dword);
			break;
		case SQL_DRIVER_HLIB:
			PUSHSQLERR (pdbc, IDS_HY_C00);
			LEAVE_HDBC (pdbc, SQL_ERROR);
			break;
		case SQL_DRIVER_HSTMT:
			if (InfoValuePtr != NULL)
			{
				pstmt = *((CStmt **) InfoValuePtr);
			}
			for( i = pdbc->m_StmtCollect.begin(); i != pdbc->m_StmtCollect.end(); ++i)
			{
				if ((tpstmt = (CStmt*)(*i)) != NULL)
					if (tpstmt== pstmt) break;
			}
			if (i == pdbc->m_StmtCollect.end())
			{
				PUSHSQLERR (pdbc, IDS_S1_009);
				RETURNCODE (pdbc, SQL_ERROR);
				LEAVE_HDBC (pdbc, SQL_ERROR);
			}
			dword = (SQLULEN)(pstmt);
			size = sizeof (dword);
			break;
		default:
			break;
    }
	if (size)
    {
		if (InfoValuePtr != NULL)
		{
			*((SQLULEN *) InfoValuePtr) = dword;
		}
		if (StringLengthPtr != NULL)
		{
			*(StringLengthPtr) = (SWORD) size;
		}
		RETURNCODE (pdbc, SQL_SUCCESS);
		LEAVE_HDBC (pdbc, SQL_SUCCESS);
	}
	rc = NeoGetInfo(ConnectionHandle,InfoType,InfoValuePtr,BufferLength,StringLengthPtr,isWideCall);
	RETURNCODE (pdbc, rc);
	LEAVE_HDBC (pdbc, rc);
}

SQLRETURN  SQL_API SQLGetInfo(SQLHDBC ConnectionHandle,
           SQLUSMALLINT InfoType, 
		   SQLPOINTER InfoValuePtr,
           SQLSMALLINT BufferLength,
		   SQLSMALLINT *StringLengthPtr)
{
	return SQLGetInfo_common(ConnectionHandle,
           InfoType, 
		   InfoValuePtr,
           BufferLength,
		   StringLengthPtr,
		   false);
}

SQLRETURN  SQL_API SQLGetInfoA(SQLHDBC ConnectionHandle,
           SQLUSMALLINT InfoType, 
		   SQLPOINTER InfoValuePtr,
           SQLSMALLINT BufferLength,
		   SQLSMALLINT *StringLengthPtr)
{
	return SQLGetInfo_common(ConnectionHandle,
           InfoType, 
		   InfoValuePtr,
           BufferLength,
		   StringLengthPtr,
		   false);
}

SQLRETURN  SQL_API SQLGetInfoW(SQLHDBC ConnectionHandle,
           SQLUSMALLINT InfoType, 
		   SQLPOINTER InfoValuePtr,
           SQLSMALLINT BufferLength,
		   SQLSMALLINT *StringLengthPtr)
{
	return SQLGetInfo_common(ConnectionHandle,
           InfoType, 
		   InfoValuePtr,
           BufferLength,
		   StringLengthPtr,
		   true);
}

SQLRETURN  SQL_API SQLSetDescField_common(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength,
		   bool isWideCall)
{
	DESC (desc, DescriptorHandle);
	SQLRETURN	rc;

	ENTER_HDESC (desc);
	rc = NeoSetDescField(DescriptorHandle,RecNumber,FieldIdentifier,ValuePtr,BufferLength,isWideCall);
	RETURNCODE (desc, rc);
	LEAVE_HDESC (desc, rc);
}

SQLRETURN  SQL_API SQLSetDescField(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength)
{
	return SQLSetDescField_common(DescriptorHandle,
           RecNumber, 
		   FieldIdentifier,
           ValuePtr, 
		   BufferLength,
		   false);
}

SQLRETURN  SQL_API SQLSetDescFieldA(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength)
{
	return SQLSetDescField_common(DescriptorHandle,
           RecNumber, 
		   FieldIdentifier,
           ValuePtr, 
		   BufferLength,
		   false);
}
	SQLRETURN  SQL_API SQLSetDescFieldW(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength)
{
	return SQLSetDescField_common(DescriptorHandle,
           RecNumber, 
		   FieldIdentifier,
           ValuePtr, 
		   BufferLength,
		   true);
}

SQLRETURN  SQL_API SQLSetDescRec(SQLHDESC DescriptorHandle,
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
	DESC (desc, DescriptorHandle);
	SQLRETURN	rc;

	ENTER_HDESC (desc);
	rc = NeoSetDescRec(DescriptorHandle,RecNumber,Type,SubType,Length,Precision,Scale,Data,StringLengthPtr,IndicatorPtr);
	RETURNCODE (desc, rc);
	LEAVE_HDESC (desc, rc);
}

SQLRETURN  SQL_API SQLGetDescField_common(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength,
           SQLINTEGER *StringLengthPtr,
		   bool isWideCall)
{
	DESC (desc, DescriptorHandle);
	SQLRETURN	rc;

	ENTER_HDESC (desc);
	rc = NeoGetDescField(DescriptorHandle,RecNumber,FieldIdentifier,ValuePtr,BufferLength,StringLengthPtr,isWideCall);
	RETURNCODE (desc, rc);
	LEAVE_HDESC (desc, rc);
}

SQLRETURN  SQL_API SQLGetDescField(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength,
           SQLINTEGER *StringLengthPtr)
{
	return SQLGetDescField_common(DescriptorHandle,
           RecNumber, 
		   FieldIdentifier,
           ValuePtr, 
		   BufferLength,
           StringLengthPtr,
		   false);
}

SQLRETURN  SQL_API SQLGetDescFieldA(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength,
           SQLINTEGER *StringLengthPtr)
{
	return SQLGetDescField_common(DescriptorHandle,
           RecNumber, 
		   FieldIdentifier,
           ValuePtr, 
		   BufferLength,
           StringLengthPtr,
		   false);
}

SQLRETURN  SQL_API SQLGetDescFieldW(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength,
           SQLINTEGER *StringLengthPtr)
{
	return SQLGetDescField_common(DescriptorHandle,
           RecNumber, 
		   FieldIdentifier,
           ValuePtr, 
		   BufferLength,
           StringLengthPtr,
		   true);
}

SQLRETURN  SQL_API SQLGetDescRec_common(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLCHAR *Name,
           SQLSMALLINT BufferLength, 
		   SQLSMALLINT *StringLengthPtr,
           SQLSMALLINT *TypePtr, 
		   SQLSMALLINT *SubTypePtr, 
           SQLLEN     *LengthPtr, 
		   SQLSMALLINT *PrecisionPtr, 
           SQLSMALLINT *ScalePtr, 
		   SQLSMALLINT *NullablePtr,
		   bool isWideCall)
{
	DESC (desc, DescriptorHandle);
	SQLRETURN	rc;

	ENTER_HDESC (desc);
	rc = NeoGetDescRec(DescriptorHandle,RecNumber,Name,BufferLength,StringLengthPtr,TypePtr,SubTypePtr,LengthPtr,PrecisionPtr,ScalePtr,NullablePtr,isWideCall);
	RETURNCODE (desc, rc);
	LEAVE_HDESC (desc, rc);
}

SQLRETURN  SQL_API SQLGetDescRec(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLCHAR *Name,
           SQLSMALLINT BufferLength, 
		   SQLSMALLINT *StringLengthPtr,
           SQLSMALLINT *TypePtr, 
		   SQLSMALLINT *SubTypePtr, 
           SQLLEN     *LengthPtr, 
		   SQLSMALLINT *PrecisionPtr, 
           SQLSMALLINT *ScalePtr, 
		   SQLSMALLINT *NullablePtr)
{
    return SQLGetDescRec_common(DescriptorHandle,
           RecNumber, 
		   Name,
           BufferLength, 
		   StringLengthPtr,
           TypePtr, 
		   SubTypePtr, 
           LengthPtr, 
		   PrecisionPtr, 
           ScalePtr, 
		   NullablePtr,
		   false);
}
SQLRETURN  SQL_API SQLGetDescRecA(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLCHAR *Name,
           SQLSMALLINT BufferLength, 
		   SQLSMALLINT *StringLengthPtr,
           SQLSMALLINT *TypePtr, 
		   SQLSMALLINT *SubTypePtr, 
           SQLLEN      *LengthPtr, 
		   SQLSMALLINT *PrecisionPtr, 
           SQLSMALLINT *ScalePtr, 
		   SQLSMALLINT *NullablePtr)
{
    return SQLGetDescRec_common(DescriptorHandle,
           RecNumber, 
		   Name,
           BufferLength, 
		   StringLengthPtr,
           TypePtr, 
		   SubTypePtr, 
           LengthPtr, 
		   PrecisionPtr, 
           ScalePtr, 
		   NullablePtr,
		   false);
}
SQLRETURN  SQL_API SQLGetDescRecW(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLWCHAR *Name,
           SQLSMALLINT BufferLength, 
		   SQLSMALLINT *StringLengthPtr,
           SQLSMALLINT *TypePtr, 
		   SQLSMALLINT *SubTypePtr, 
           SQLLEN     *LengthPtr, 
		   SQLSMALLINT *PrecisionPtr, 
           SQLSMALLINT *ScalePtr, 
		   SQLSMALLINT *NullablePtr)
{
    return SQLGetDescRec_common(DescriptorHandle,
           RecNumber, 
										  (SQLCHAR*)Name,
           BufferLength, 
		   StringLengthPtr,
           TypePtr, 
		   SubTypePtr, 
           LengthPtr, 
		   PrecisionPtr, 
           ScalePtr, 
		   NullablePtr,
		   true);
}


SQLRETURN  SQL_API SQLBindCol(SQLHSTMT StatementHandle, 
		   SQLUSMALLINT ColumnNumber, 
		   SQLSMALLINT TargetType, 
		   SQLPOINTER TargetValue, 
		   SQLLEN BufferLength, 
	   	   SQLLEN *StrLen_or_IndPtr)
{
	CStmt *pstmt = (CStmt *) StatementHandle;
	SQLRETURN	rc;

	ENTER_STMT (pstmt);
	/* check argument */
	if (ColumnNumber == 0 && (TargetType != SQL_C_BOOKMARK || TargetType != SQL_C_VARBOOKMARK))
	{
		PUSHSQLERR (pstmt, IDS_07_006);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
	}
	switch (TargetType)
    {
		case SQL_C_DEFAULT:
		case SQL_C_BIT:
		case SQL_C_BINARY:
		case SQL_C_CHAR:
		case SQL_C_WCHAR:
		case SQL_C_DATE:
		case SQL_C_DOUBLE:
		case SQL_C_FLOAT:
		case SQL_C_LONG:
		case SQL_C_SHORT:
		case SQL_C_SLONG:
		case SQL_C_SSHORT:
		case SQL_C_STINYINT:
		case SQL_C_TIME:
		case SQL_C_TIMESTAMP:
		case SQL_C_TINYINT:
		case SQL_C_ULONG:
		case SQL_C_USHORT:
		case SQL_C_UTINYINT:

		case SQL_C_GUID:
		case SQL_C_INTERVAL_DAY:
		case SQL_C_INTERVAL_DAY_TO_HOUR:
		case SQL_C_INTERVAL_DAY_TO_MINUTE:
		case SQL_C_INTERVAL_DAY_TO_SECOND:
		case SQL_C_INTERVAL_HOUR:
		case SQL_C_INTERVAL_HOUR_TO_MINUTE:
		case SQL_C_INTERVAL_HOUR_TO_SECOND:
		case SQL_C_INTERVAL_MINUTE:
		case SQL_C_INTERVAL_MINUTE_TO_SECOND:
		case SQL_C_INTERVAL_MONTH:
		case SQL_C_INTERVAL_SECOND:
		case SQL_C_INTERVAL_YEAR:
		case SQL_C_INTERVAL_YEAR_TO_MONTH:
		case SQL_C_NUMERIC:
		case SQL_C_SBIGINT:
		case SQL_C_TYPE_DATE:
		case SQL_C_TYPE_TIME:
		case SQL_C_TYPE_TIMESTAMP:
		case SQL_C_UBIGINT:
			break;
		default:
			PUSHSQLERR (pstmt, IDS_S1_003);
			RETURNCODE (pstmt, SQL_ERROR);
			LEAVE_STMT (pstmt, SQL_ERROR);
    }
	if (BufferLength < 0)
    {
		PUSHSQLERR (pstmt, IDS_S1_090);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	/* check state */
	if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
    {
		PUSHSQLERR (pstmt, IDS_S1_010);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoBindCol(StatementHandle,ColumnNumber,TargetType,TargetValue,BufferLength,StrLen_or_IndPtr);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLBindParameter(SQLHSTMT StatementHandle,
			SQLUSMALLINT ParameterNumber, 
			SQLSMALLINT InputOutputType,
			SQLSMALLINT ValueType,
			SQLSMALLINT ParameterType, 
			SQLULEN  	ColumnSize,
			SQLSMALLINT DecimalDigits,
			SQLPOINTER  ParameterValuePtr,
			SQLLEN    	BufferLength,
			SQLLEN     *StrLen_or_IndPtr)		   
{
	SQLRETURN	rc;
	STMT (pstmt, StatementHandle);
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	if (ParameterNumber < 1)
    {
      sqlstat = IDS_07_009;
    }
	else if ((ParameterValuePtr == NULL && StrLen_or_IndPtr == NULL) 
		&& InputOutputType != SQL_PARAM_OUTPUT)
    {
		sqlstat = IDS_S1_009;
		/* This means, I allow output to nowhere
		* (i.e. * junk output result). But I can't  
		* allow input from nowhere. 
		*/
    }
/**********
	else if( BufferLength < 0L && BufferLength != SQL_SETPARAM_VALUE_MAX )
	{
		sqlstat = IDS_S1_090;
	}
**********/
	else if (InputOutputType != SQL_PARAM_INPUT
		&& InputOutputType != SQL_PARAM_OUTPUT
			&& InputOutputType != SQL_PARAM_INPUT_OUTPUT)
	{
		sqlstat = IDS_S1_105;
    }
	else
    {
		switch (ValueType)
		{
			case SQL_C_DEFAULT:
			case SQL_C_BINARY:
			case SQL_C_BIT:
			case SQL_C_CHAR:
			case SQL_C_WCHAR:
			case SQL_C_DATE:
			case SQL_C_DOUBLE:
			case SQL_C_FLOAT:
			case SQL_C_LONG:
			case SQL_C_SHORT:
			case SQL_C_SLONG:
			case SQL_C_SSHORT:
			case SQL_C_STINYINT:
			case SQL_C_TIME:
			case SQL_C_TIMESTAMP:
			case SQL_C_TINYINT:
			case SQL_C_ULONG:
			case SQL_C_USHORT:
			case SQL_C_UTINYINT:
			case SQL_C_GUID:
			case SQL_C_INTERVAL_DAY:
			case SQL_C_INTERVAL_DAY_TO_HOUR:
			case SQL_C_INTERVAL_DAY_TO_MINUTE:
			case SQL_C_INTERVAL_DAY_TO_SECOND:
			case SQL_C_INTERVAL_HOUR:
			case SQL_C_INTERVAL_HOUR_TO_MINUTE:
			case SQL_C_INTERVAL_HOUR_TO_SECOND:
			case SQL_C_INTERVAL_MINUTE:
			case SQL_C_INTERVAL_MINUTE_TO_SECOND:
			case SQL_C_INTERVAL_MONTH:
			case SQL_C_INTERVAL_SECOND:
			case SQL_C_INTERVAL_YEAR:
			case SQL_C_INTERVAL_YEAR_TO_MONTH:
			case SQL_C_NUMERIC:
			case SQL_C_SBIGINT:
			case SQL_C_TYPE_DATE:
			case SQL_C_TYPE_TIME:
			case SQL_C_TYPE_TIMESTAMP:
			case SQL_C_UBIGINT:
				break;
			default:
				sqlstat = IDS_S1_003;
				break;
		}
	}
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
	}
	/* check state */
	if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
	{
		PUSHSQLERR (pstmt, IDS_S1_010);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
	}
 	rc = NeoBindParameter(StatementHandle,ParameterNumber,InputOutputType,ValueType,ParameterType,ColumnSize,DecimalDigits,ParameterValuePtr,BufferLength,StrLen_or_IndPtr);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLBrowseConnect_common(
    SQLHDBC            ConnectionHandle,
    SQLCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLCHAR 		  *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT       *StringLength2Ptr,
    bool               isWideCall)
{
	SQLRETURN	rc;
	CONN (pdbc, ConnectionHandle);
	ODBC_LOCK ();
	if (!IS_VALID_HDBC (pdbc))
    {
		ODBC_UNLOCK ();
		return SQL_INVALID_HANDLE;
    }
	CLEAR_ERRORS(pdbc);
	/* check arguments */
	if ((StringLength1 < 0 && StringLength1 != SQL_NTS) || BufferLength < 0)
    {
		PUSHSQLERR (pdbc, IDS_S1_090);
		RETURNCODE (pdbc, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	if (InConnectionString == NULL || StringLength1 == 0)
    {
		PUSHSQLERR (pdbc, IDS_IM_002);
		RETURNCODE (pdbc, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	if (pdbc->state == en_dbc_allocated)
    {
	}
	else if (pdbc->state != en_dbc_needdata)
    {
		PUSHSQLERR (pdbc, IDS_08_002);
		RETURNCODE (pdbc, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	rc = NeoBrowseConnect(ConnectionHandle,InConnectionString,StringLength1,OutConnectionString,BufferLength,StringLength2Ptr,isWideCall);
	
	switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
		pdbc->state = en_dbc_connected;
		break;
    case SQL_NEED_DATA:
		pdbc->state = en_dbc_needdata;
		break;
    case SQL_ERROR:
		pdbc->state = en_dbc_allocated;
		break;
    default:
		break;
    }
	RETURNCODE (pdbc, rc);
	ODBC_UNLOCK ();
	return rc;
} // SQLBrowseConnect_common()

SQLRETURN SQL_API SQLBrowseConnect(
    SQLHDBC            ConnectionHandle,
    SQLCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLCHAR 		  *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT       *StringLength2Ptr)
{
    return SQLBrowseConnect_common(
       ConnectionHandle,
       InConnectionString,
       StringLength1,
       OutConnectionString,
       BufferLength,
       StringLength2Ptr,
       false);
} // SQLBrowseConnect()

SQLRETURN SQL_API SQLBrowseConnectA(
    SQLHDBC            ConnectionHandle,
    SQLCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLCHAR 		  *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT       *StringLength2Ptr)
{
    return SQLBrowseConnect_common(
       ConnectionHandle,
       InConnectionString,
       StringLength1,
       OutConnectionString,
       BufferLength,
       StringLength2Ptr,
       false);
} // SQLBrowseConnect()

SQLRETURN SQL_API SQLBrowseConnectW(
    SQLHDBC            ConnectionHandle,
    SQLWCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLWCHAR 		  *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT       *StringLength2Ptr)
{
    return SQLBrowseConnect_common(
       ConnectionHandle,
       (SQLCHAR*)InConnectionString,
       StringLength1,
       (SQLCHAR*)OutConnectionString,
       BufferLength,
       StringLength2Ptr,
       true);
} // SQLBrowseConnectW()

SQLRETURN SQL_API SQLDriverConnect_common(SQLHDBC  ConnectionHandle,
    SQLHWND            WindowHandle,
    SQLCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLCHAR           *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT 	  *StringLength2Ptr,
    SQLUSMALLINT       DriverCompletion,
	bool               isWideCall)
{
	SQLRETURN	rc;
	CONN (pdbc, ConnectionHandle);
	ODBC_LOCK ();
	if (!IS_VALID_HDBC (pdbc))
    {
		ODBC_UNLOCK ();
		return SQL_INVALID_HANDLE;
    }
	CLEAR_ERRORS(pdbc);
	/* check arguments */
	if ((StringLength1 < 0 && StringLength1 != SQL_NTS) || BufferLength < 0)
	{
		PUSHSQLERR (pdbc, IDS_S1_090);
		RETURNCODE (pdbc, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	/* check state */
	if (pdbc->state != en_dbc_allocated)
    {
		PUSHSQLERR (pdbc, IDS_08_002);
		RETURNCODE (pdbc, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	rc = NeoDriverConnect(ConnectionHandle,WindowHandle,InConnectionString,StringLength1,OutConnectionString,BufferLength,StringLength2Ptr,DriverCompletion,isWideCall);
	RETURNCODE (pdbc, rc);
	if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    {
		ODBC_UNLOCK ();
		return rc;
    }
	/* state transition */
	pdbc->state = en_dbc_connected;
	ODBC_UNLOCK ();
	return rc;
}

SQLRETURN SQL_API SQLDriverConnect(SQLHDBC  ConnectionHandle,
    SQLHWND            WindowHandle,
    SQLCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLCHAR           *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT 	  *StringLength2Ptr,
    SQLUSMALLINT       DriverCompletion)
{
    return SQLDriverConnect_common(ConnectionHandle,
                            WindowHandle,
                            InConnectionString,
                            StringLength1,
                            OutConnectionString,
                            BufferLength,
                            StringLength2Ptr,
                            DriverCompletion,
                            false);
}

SQLRETURN SQL_API SQLDriverConnectA(SQLHDBC  ConnectionHandle,
    SQLHWND            WindowHandle,
    SQLCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLCHAR           *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT 	  *StringLength2Ptr,
    SQLUSMALLINT       DriverCompletion)
{
    return SQLDriverConnect_common(ConnectionHandle,
                            WindowHandle,
                            InConnectionString,
                            StringLength1,
                            OutConnectionString,
                            BufferLength,
                            StringLength2Ptr,
                            DriverCompletion,
                            false);
}

SQLRETURN SQL_API SQLDriverConnectW(SQLHDBC  ConnectionHandle,
    SQLHWND            WindowHandle,
    SQLWCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLWCHAR           *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT 	  *StringLength2Ptr,
    SQLUSMALLINT       DriverCompletion)
{
    return SQLDriverConnect_common(ConnectionHandle,
                            WindowHandle,
											  (SQLCHAR*)InConnectionString,
                            StringLength1,
											  (SQLCHAR*)OutConnectionString,
                            BufferLength,
                            StringLength2Ptr,
                            DriverCompletion,
                            true);
}


/*
 *
 *   SQLPrepare and its variants
 *
 */

SQLRETURN SQL_API SQLPrepare_common(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength,
		   bool isWideCall)
{
	SQLRETURN	rc;
	STMT (pstmt, StatementHandle);
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	/* check state */
	if (pstmt->asyn_on == en_NullProc)
    {
		/* not on asyn state */
		switch (pstmt->state)
		{
			case en_stmt_fetched:
			case en_stmt_xfetched:
				sqlstat = IDS_24_000;
				break;
			case en_stmt_needdata:
			case en_stmt_mustput:
			case en_stmt_canput:
				sqlstat = IDS_S1_010;
				break;
			default:
				break;
		}
    }
	else if (pstmt->asyn_on != en_Prepare)
    {
		/* asyn on other */
		sqlstat = IDS_S1_010;
    }
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	if (StatementText == NULL)
    {
		PUSHSQLERR (pstmt, IDS_S1_009);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	if (TextLength < 0 && TextLength != SQL_NTS)
    {
		PUSHSQLERR (pstmt, IDS_S1_090);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoPrepare(StatementHandle,StatementText,TextLength,isWideCall);

	/* stmt state transition */
	if (pstmt->asyn_on == en_Prepare)
	{
		switch (rc)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			case SQL_ERROR:
				pstmt->asyn_on = en_NullProc;
				break;
			case SQL_STILL_EXECUTING:
			default:
				LEAVE_STMT (pstmt, rc);
		}
    }
	switch (rc)
	{
		case SQL_STILL_EXECUTING:
			pstmt->asyn_on = en_Prepare;
			break;
		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO:
			pstmt->state = en_stmt_prepared;
			pstmt->prep_state = 1;
			break;
		case SQL_ERROR:
			switch (pstmt->state)
			{
				case en_stmt_prepared:
				case en_stmt_executed:
					pstmt->state = en_stmt_allocated;
					pstmt->prep_state = 0;
					break;
				default:
					break;
			}
		default:
			break;
	}
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);

} // SQLPrepare_common()

SQLRETURN SQL_API SQLPrepare(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength)
{
   return SQLPrepare_common(StatementHandle,
                            StatementText,
                            TextLength,
                            false);
} // SQLPrepare()

SQLRETURN SQL_API SQLPrepareA(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength)
{
   return SQLPrepare_common(StatementHandle,
                            StatementText,
                            TextLength,
                            false);
} // SQLPrepareA()

SQLRETURN SQL_API SQLPrepareW(SQLHSTMT StatementHandle,
           SQLWCHAR *StatementText,
		   SQLINTEGER TextLength)
{
   return SQLPrepare_common(StatementHandle,
                            (SQLCHAR*)StatementText,
                            TextLength,
                            true);
} // SQLPrepareW()


SQLRETURN SQL_API SQLExecDirect_common(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength,
		   bool isWideCall)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN	rc;
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	/* check arguments */
	if (StatementText == NULL)
    {
		sqlstat = IDS_S1_009;
    }
	else if (TextLength < 0 && TextLength != SQL_NTS)
    {
		sqlstat = IDS_S1_090;
    }
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	/* check state */
	if (pstmt->asyn_on == en_NullProc)
    {
		switch (pstmt->state)
		{
			case en_stmt_fetched:
			case en_stmt_xfetched:
				sqlstat = IDS_24_000;
				break;
			case en_stmt_needdata:
			case en_stmt_mustput:
			case en_stmt_canput:
				sqlstat = IDS_S1_010;
				break;
			default:
				break;
		}
    }
	else if (pstmt->asyn_on != en_ExecDirect)
    {
		sqlstat = IDS_S1_010;
    }
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoExecDirect(StatementHandle,StatementText,TextLength,isWideCall);
	/* stmt state transition */
	if (pstmt->asyn_on == en_ExecDirect)
    {
		switch (rc)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			case SQL_NEED_DATA:
			case SQL_ERROR:
				pstmt->asyn_on = en_NullProc;
				break;
			case SQL_STILL_EXECUTING:
			default:
				RETURNCODE (pstmt, rc);
				LEAVE_STMT (pstmt, rc);
		}
    }
	if (pstmt->state <= en_stmt_executed)
    {
		switch (rc)
		{
			case SQL_SUCCESS_WITH_INFO:
			case SQL_SUCCESS:
				fun_do_cursoropen_exec_direct (pstmt);
				break;
			case SQL_NEED_DATA:
				pstmt->state = en_stmt_needdata;
				pstmt->need_on = en_ExecDirect;
				break;
			case SQL_STILL_EXECUTING:
				pstmt->asyn_on = en_ExecDirect;
				break;
			case SQL_ERROR:
				pstmt->state = en_stmt_allocated;
				pstmt->cursor_state = en_stmt_cursor_no;
				pstmt->prep_state = 0;
				break;
			default:
				break;
		}
    }
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLExecDirect(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength)

{
   return SQLExecDirect_common(StatementHandle,
                               StatementText,
                               TextLength,
                               false);
}

SQLRETURN SQL_API SQLExecDirectA(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength)

{
   return SQLExecDirect_common(StatementHandle,
                               StatementText,
                               TextLength,
                               false);
}
SQLRETURN SQL_API SQLExecDirectW(SQLHSTMT StatementHandle,
           SQLWCHAR *StatementText,
		   SQLINTEGER TextLength)

{
   return SQLExecDirect_common(StatementHandle,
                               (SQLCHAR*)StatementText,
                               TextLength,
                               true);
}

SQLRETURN  SQL_API SQLDescribeCol_common(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLCHAR *ColumnName,
           SQLSMALLINT BufferLength, 
		   SQLSMALLINT *NameLengthPtr,
           SQLSMALLINT *DataTypePtr, 
		   SQLULEN *ColumnSizePtr,
           SQLSMALLINT *DecimalDigitsPtr,
		   SQLSMALLINT *NullablePtr,
		   bool         isWideCall)
{
	CStmt *pstmt = (CStmt *) StatementHandle;
	SQLRETURN	rc;
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	/* check arguments */
	if (ColumnNumber == 0)
    {
		sqlstat = IDS_S1_002;
    }
	else if (BufferLength < 0)
    {
		sqlstat = IDS_S1_090;
    }
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoDescribeCol(StatementHandle,ColumnNumber,ColumnName,BufferLength,NameLengthPtr,DataTypePtr,ColumnSizePtr,DecimalDigitsPtr,NullablePtr,isWideCall);
	/* state transition */
	if (pstmt->asyn_on == en_DescribeCol)
    {
		switch (rc)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			case SQL_ERROR:
				pstmt->asyn_on = en_NullProc;
				break;
			default:
				RETURNCODE (pstmt, rc);
				LEAVE_STMT (pstmt, rc);
		}
    }
	switch (pstmt->state)
    {
		case en_stmt_prepared:
		case en_stmt_cursoropen:
		case en_stmt_fetched:
		case en_stmt_xfetched:
			if (rc == SQL_STILL_EXECUTING)
			{
				pstmt->asyn_on = en_DescribeCol;
			}
			break;
		default:
			break;
    }
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN  SQL_API SQLDescribeCol(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLCHAR *ColumnName,
           SQLSMALLINT BufferLength, 
		   SQLSMALLINT *NameLengthPtr,
           SQLSMALLINT *DataTypePtr, 
		   SQLULEN *ColumnSizePtr,
           SQLSMALLINT *DecimalDigitsPtr,
		   SQLSMALLINT *NullablePtr)
{
    return SQLDescribeCol_common(StatementHandle,
                                 ColumnNumber, 
                                 ColumnName,
                                 BufferLength, 
                                 NameLengthPtr,
                                 DataTypePtr, 
                                 ColumnSizePtr,
                                 DecimalDigitsPtr,
                                 NullablePtr,
                                 false);
}

SQLRETURN  SQL_API SQLDescribeColA(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLCHAR *ColumnName,
           SQLSMALLINT BufferLength, 
		   SQLSMALLINT *NameLengthPtr,
           SQLSMALLINT *DataTypePtr, 
		   SQLULEN     *ColumnSizePtr,
           SQLSMALLINT *DecimalDigitsPtr,
		   SQLSMALLINT *NullablePtr)
{
    return SQLDescribeCol_common(StatementHandle,
                                 ColumnNumber, 
                                 ColumnName,
                                 BufferLength, 
                                 NameLengthPtr,
                                 DataTypePtr, 
                                 ColumnSizePtr,
                                 DecimalDigitsPtr,
                                 NullablePtr,
                                 false);
}

SQLRETURN  SQL_API SQLDescribeColW(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLWCHAR *ColumnName,
           SQLSMALLINT BufferLength, 
		   SQLSMALLINT *NameLengthPtr,
           SQLSMALLINT *DataTypePtr, 
		   SQLULEN *ColumnSizePtr,
           SQLSMALLINT *DecimalDigitsPtr,
		   SQLSMALLINT *NullablePtr)
{
    return SQLDescribeCol_common(StatementHandle,
                                 ColumnNumber, 
                                 (SQLCHAR*)ColumnName,
                                 BufferLength, 
                                 NameLengthPtr,
                                 DataTypePtr, 
                                 ColumnSizePtr,
                                 DecimalDigitsPtr,
                                 NullablePtr,
                                 true);
}

SQLRETURN  SQL_API SQLNumResultCols(SQLHSTMT StatementHandle,
           SQLSMALLINT *ColumnCountPtr)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN	rc;

	ENTER_STMT (pstmt);
	rc = fun_NumResultCols (StatementHandle, ColumnCountPtr);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN  SQL_API SQLNumParams(SQLHSTMT StatementHandle,
           SQLSMALLINT *ParameterCountPtr)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN	rc;

	ENTER_STMT (pstmt);
	/* check argument */
	if (!ParameterCountPtr)
    {
		RETURNCODE (pstmt, SQL_SUCCESS);
		LEAVE_STMT (pstmt, SQL_SUCCESS);
    }
	/* check state */
	if (pstmt->asyn_on == en_NullProc)
    {
		switch (pstmt->state)
		{
			case en_stmt_allocated:
			case en_stmt_needdata:
			case en_stmt_mustput:
			case en_stmt_canput:
				PUSHSQLERR (pstmt, IDS_S1_010);
				RETURNCODE (pstmt, SQL_ERROR);
				LEAVE_STMT (pstmt, SQL_ERROR);
			default:
				break;
		}
    }
	else if (pstmt->asyn_on != en_NumParams)
    {
		PUSHSQLERR (pstmt, IDS_S1_010);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoNumParams(StatementHandle,ParameterCountPtr);
	/* state transition */
	if (pstmt->asyn_on == en_NumParams)
    {
		switch (rc)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			case SQL_ERROR:
				break;
			default:
				RETURNCODE (pstmt, rc);
				LEAVE_STMT (pstmt, rc);
		}
    }
	if (rc == SQL_STILL_EXECUTING)
    {
		pstmt->asyn_on = en_NumParams;
    }
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLDescribeParam(
	SQLHSTMT           StatementHandle,
	SQLUSMALLINT       ParameterNumber,
    SQLSMALLINT 	  *DataTypePtr,
    SQLULEN      	  *ParameterSizePtr,
    SQLSMALLINT 	  *DecimalDigitsPtr,
    SQLSMALLINT 	  *NullablePtr)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN	rc;

	ENTER_STMT (pstmt);
	/* check argument */
	if (ParameterNumber == 0)
    {
		PUSHSQLERR (pstmt, IDS_S1_093);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	/* check state */
	if (pstmt->asyn_on == en_NullProc)
    {
		switch (pstmt->state)
		{
			case en_stmt_allocated:
			case en_stmt_needdata:
			case en_stmt_mustput:
			case en_stmt_canput:
				PUSHSQLERR (pstmt, IDS_S1_010);
				RETURNCODE (pstmt, SQL_ERROR);
				LEAVE_STMT (pstmt, SQL_ERROR);
			default:
				break;
		}
    }
	else if (pstmt->asyn_on != en_DescribeParam)
    {
		PUSHSQLERR (pstmt, IDS_S1_010);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoDescribeParam(StatementHandle,ParameterNumber,DataTypePtr,ParameterSizePtr,DecimalDigitsPtr,NullablePtr);
	/* state transition */
	if (pstmt->asyn_on == en_DescribeParam)
    {
		switch (rc)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			case SQL_ERROR:
				break;
			default:
				RETURNCODE (pstmt, rc);
				LEAVE_STMT (pstmt, rc);
		}
    }
	if (rc == SQL_STILL_EXECUTING)
    {
		pstmt->asyn_on = en_DescribeParam;
    }
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}


SQLRETURN SQL_API SQLCloseCursor(SQLHSTMT StatementHandle)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN	rc;

	ENTER_STMT (pstmt);
	/* check state */
	if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
    {
		PUSHSQLERR (pstmt, IDS_S1_010);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	else if (pstmt->state < en_stmt_cursoropen)
    {
		PUSHSQLERR (pstmt, IDS_24_000);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoCloseCursor(StatementHandle);
	if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    {
		RETURNCODE (pstmt, rc);
		LEAVE_STMT (pstmt, rc);
    }
	/*
	*  State transition
	*/
	pstmt->cursor_state = en_stmt_cursor_no;
	switch (pstmt->state)
    {
		case en_stmt_allocated:
		case en_stmt_prepared:
			break;
		case en_stmt_executed:
		case en_stmt_cursoropen:
		case en_stmt_fetched:
		case en_stmt_xfetched:
			if (pstmt->prep_state)
				pstmt->state = en_stmt_prepared;
			else
				pstmt->state = en_stmt_allocated;
			break;
		default:
			break;
    }
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN  SQL_API SQLTables_common(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
           SQLCHAR *TableType, 
		   SQLSMALLINT NameLength4,
		   bool isWideCall)
{
	SQLRETURN	rc;
	STMT (pstmt, StatementHandle);
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	for (;;)
    {
		if (CatalogName != NULL && CatalogName[0] != 0 && NameLength1 < 0 && NameLength1 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (SchemaName != NULL && SchemaName[0] != 0 && NameLength2 < 0 && NameLength2 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (TableName != NULL && TableName[0] != 0 && NameLength3 < 0 && NameLength3 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (TableType != NULL && TableType[0] != 0 && NameLength4 < 0 && NameLength4 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		rc = fun_cata_state_ok (pstmt, en_Tables);
		if (rc != SQL_SUCCESS)
		{
			LEAVE_STMT (pstmt, SQL_ERROR);
		}
		sqlstat = 0;
		break;
    }
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
	}
	rc = NeoTables(StatementHandle,CatalogName,NameLength1,SchemaName,NameLength2,TableName,NameLength3,TableType,NameLength4,isWideCall);
	rc = fun_cata_state_tr (pstmt, en_Tables, rc);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN  SQL_API SQLTables(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
           SQLCHAR *TableType, 
		   SQLSMALLINT NameLength4)
{
	return SQLTables_common(StatementHandle,
           CatalogName, 
		   NameLength1,
           SchemaName, 
		   NameLength2,
           TableName, 
		   NameLength3,
           TableType, 
		   NameLength4,
		   false);
}

SQLRETURN  SQL_API SQLTablesA(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
           SQLCHAR *TableType, 
		   SQLSMALLINT NameLength4)
{
	return SQLTables_common(StatementHandle,
           CatalogName, 
		   NameLength1,
           SchemaName, 
		   NameLength2,
           TableName, 
		   NameLength3,
           TableType, 
		   NameLength4,
		   false);
}

SQLRETURN  SQL_API SQLTablesW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableName, 
		   SQLSMALLINT NameLength3,
           SQLWCHAR *TableType, 
		   SQLSMALLINT NameLength4)
{
	return SQLTables_common(StatementHandle,
									(SQLCHAR*)CatalogName, 
		   NameLength1,
									(SQLCHAR*)SchemaName, 
		   NameLength2,
									(SQLCHAR*)TableName, 
		   NameLength3,
									(SQLCHAR*)TableType, 
		   NameLength4,
		   true);
}

SQLRETURN  SQL_API SQLColumns_common(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
           SQLCHAR *ColumnName, 
		   SQLSMALLINT NameLength4,
		   bool isWideCall)
{
	SQLRETURN	rc;
	STMT (pstmt, StatementHandle);
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	for (;;)
    {
		if (CatalogName != NULL && CatalogName[0] != 0 && NameLength1 < 0 && NameLength1 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (SchemaName != NULL && SchemaName[0] != 0 && NameLength2 < 0 && NameLength2 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (TableName != NULL && TableName[0] != 0 && NameLength3 < 0 && NameLength3 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (ColumnName != NULL && ColumnName[0] != 0 && NameLength4 < 0 && NameLength4 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		rc = fun_cata_state_ok (pstmt, en_Columns);
		if (rc != SQL_SUCCESS)
		{
			LEAVE_STMT (pstmt, SQL_ERROR);
		}
		sqlstat = 0;
		break;
	}
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoColumns(StatementHandle,CatalogName,NameLength1,SchemaName,NameLength2,TableName,NameLength3,ColumnName,NameLength4,isWideCall);
	rc = fun_cata_state_tr (pstmt, en_Columns, rc);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN  SQL_API SQLColumns(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
           SQLCHAR *ColumnName, 
		   SQLSMALLINT NameLength4)
{
	return SQLColumns_common(StatementHandle,
           CatalogName, 
		   NameLength1,
           SchemaName, 
		   NameLength2,
           TableName, 
		   NameLength3,
           ColumnName, 
		   NameLength4,
		   false);
}

SQLRETURN  SQL_API SQLColumnsA(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
           SQLCHAR *ColumnName, 
		   SQLSMALLINT NameLength4)
{
	return SQLColumns_common(StatementHandle,
           CatalogName, 
		   NameLength1,
           SchemaName, 
		   NameLength2,
           TableName, 
		   NameLength3,
           ColumnName, 
		   NameLength4,
		   false);
}

SQLRETURN  SQL_API SQLColumnsW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableName, 
		   SQLSMALLINT NameLength3,
           SQLWCHAR *ColumnName, 
		   SQLSMALLINT NameLength4)
{
	return SQLColumns_common(StatementHandle,
									 (SQLCHAR*)CatalogName, 
		   NameLength1,
									 (SQLCHAR*)SchemaName, 
		   NameLength2,
									 (SQLCHAR*)TableName, 
		   NameLength3,
									 (SQLCHAR*)ColumnName, 
		   NameLength4,
		   true);
}



SQLRETURN  SQL_API SQLSpecialColumns_common(SQLHSTMT StatementHandle,
		   SQLUSMALLINT IdentifierType,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   SQLUSMALLINT Scope,
		   SQLUSMALLINT Nullable,
		   bool isWideCall)
{
 	SQLRETURN	rc;
	STMT (pstmt, StatementHandle);
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	for (;;)
    {
		if (CatalogName != NULL && CatalogName[0] != 0 && NameLength1 < 0 && NameLength1 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (SchemaName != NULL && SchemaName[0] != 0 && NameLength2 < 0 && NameLength2 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (TableName != NULL && TableName[0] != 0 && NameLength3 < 0 && NameLength3 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (IdentifierType != SQL_BEST_ROWID && IdentifierType != SQL_ROWVER)
		{
			sqlstat = IDS_S1_097;
			break;
		}
		if (Scope != SQL_SCOPE_CURROW
			&& Scope != SQL_SCOPE_TRANSACTION
			&& Scope != SQL_SCOPE_SESSION)
		{
			sqlstat = IDS_S1_098;
			break;
		}
		if (Nullable != SQL_NO_NULLS && Nullable != SQL_NULLABLE)
		{
			sqlstat = IDS_S1_099;
			break;
		}
		rc = fun_cata_state_ok (pstmt, en_SpecialColumns);
		if (rc != SQL_SUCCESS)
		{
			LEAVE_STMT (pstmt, SQL_ERROR);
		}

		sqlstat = 0;
		break;
	}
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
	}
	rc = NeoSpecialColumns(StatementHandle,IdentifierType,CatalogName,NameLength1,SchemaName,NameLength2,TableName,NameLength3,Scope,Nullable,isWideCall);
	rc = fun_cata_state_tr (pstmt, en_SpecialColumns, rc);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN  SQL_API SQLSpecialColumns(SQLHSTMT StatementHandle,
		   SQLUSMALLINT IdentifierType,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   SQLUSMALLINT Scope,
		   SQLUSMALLINT Nullable)
{
	return SQLSpecialColumns_common(StatementHandle,
		   IdentifierType,
           CatalogName, 
		   NameLength1,
           SchemaName, 
		   NameLength2,
           TableName, 
		   NameLength3,
		   Scope,
		   Nullable,
		   false);
}

SQLRETURN  SQL_API SQLSpecialColumnsA(SQLHSTMT StatementHandle,
		   SQLUSMALLINT IdentifierType,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   SQLUSMALLINT Scope,
		   SQLUSMALLINT Nullable)
{
	return SQLSpecialColumns_common(StatementHandle,
		   IdentifierType,
           CatalogName, 
		   NameLength1,
           SchemaName, 
		   NameLength2,
           TableName, 
		   NameLength3,
		   Scope,
		   Nullable,
		   false);
}
SQLRETURN  SQL_API SQLSpecialColumnsW(SQLHSTMT StatementHandle,
		   SQLUSMALLINT IdentifierType,
           SQLWCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   SQLUSMALLINT Scope,
		   SQLUSMALLINT Nullable)
{
	return SQLSpecialColumns_common(StatementHandle,
		   IdentifierType,
											  (SQLCHAR*)CatalogName, 
		   NameLength1,
											  (SQLCHAR*)SchemaName, 
		   NameLength2,
											  (SQLCHAR*)TableName, 
		   NameLength3,
		   Scope,
		   Nullable,
		   true);
}

SQLRETURN  SQL_API SQLGetTypeInfo_common(SQLHSTMT StatementHandle,
           SQLSMALLINT DataType,
		   bool isWideCall)
{
	SQLRETURN	rc;
	STMT (pstmt, StatementHandle);
	int sqlstat = 0;

	ENTER_STMT (pstmt);

	rc = fun_cata_state_ok (pstmt, en_GetTypeInfo);
	if (rc != SQL_SUCCESS)
	{
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
	}
    SQLINTEGER ODBCAppVersion = pstmt->getODBCAppVersion();
    SQLSMALLINT convDataType = DataType;
    if (ODBCAppVersion >= SQL_OV_ODBC3)
    {
        switch(DataType)
        {
            case SQL_DATE:
                convDataType = SQL_TYPE_DATE;
                break;
            case SQL_TIME:
                convDataType = SQL_TYPE_TIME;
                break;
            case SQL_TIMESTAMP:
                convDataType = SQL_TYPE_TIMESTAMP;
                break;
        }
    }
    else
    {
        switch(DataType)
        {
            case SQL_TYPE_DATE:
                convDataType = SQL_DATE;
                break;
            case SQL_TYPE_TIME:
                convDataType = SQL_TIME;
                break;
            case SQL_TYPE_TIMESTAMP:
                convDataType = SQL_TIMESTAMP;
                break;
        }
    }
    rc = NeoGetTypeInfo(StatementHandle,convDataType,isWideCall);
    rc = fun_cata_state_tr (pstmt, en_GetTypeInfo, rc);
    RETURNCODE (pstmt, rc);
    LEAVE_STMT (pstmt, rc);
}

SQLRETURN  SQL_API SQLGetTypeInfo(SQLHSTMT StatementHandle,
           SQLSMALLINT DataType)
{
	return SQLGetTypeInfo_common(StatementHandle,
           DataType,
		   false);
}

SQLRETURN  SQL_API SQLGetTypeInfoA(SQLHSTMT StatementHandle,
           SQLSMALLINT DataType)
{
	return SQLGetTypeInfo_common(StatementHandle,
           DataType,
		   false);
}

SQLRETURN  SQL_API SQLGetTypeInfoW(SQLHSTMT StatementHandle,
           SQLSMALLINT DataType)
{
	return SQLGetTypeInfo_common(StatementHandle,
           DataType,
		   true);
}


SQLRETURN  SQL_API SQLStatistics_common(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   SQLUSMALLINT Unique,
		   SQLUSMALLINT Reserved,
		   bool isWideCall)
{
 	SQLRETURN	rc;
	STMT (pstmt, StatementHandle);
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	for (;;)
    {
		if (CatalogName != NULL && CatalogName[0] != 0 && NameLength1 < 0 && NameLength1 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (SchemaName != NULL && SchemaName[0] != 0 && NameLength2 < 0 && NameLength2 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (TableName != NULL && TableName[0] != 0 && NameLength3 < 0 && NameLength3 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (Unique != SQL_INDEX_UNIQUE && Unique != SQL_INDEX_ALL)
		{
			sqlstat = IDS_S1_100;
			break;
		}
		if (Reserved != SQL_ENSURE && Reserved != SQL_QUICK)
		{
			sqlstat = IDS_S1_101;
			break;
		}
		rc = fun_cata_state_ok (pstmt, en_Statistics);
		if (rc != SQL_SUCCESS)
		{
			LEAVE_STMT (pstmt, SQL_ERROR);
		}
		sqlstat = 0;
		break;
	}
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoStatistics(StatementHandle,CatalogName,NameLength1,SchemaName,NameLength2,TableName,NameLength3,Unique,Reserved,isWideCall);
	rc = fun_cata_state_tr (pstmt, en_Statistics, rc);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN  SQL_API SQLStatistics(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   SQLUSMALLINT Unique,
		   SQLUSMALLINT Reserved)
{
     return SQLStatistics_common(StatementHandle,
           CatalogName, 
		   NameLength1,
           SchemaName, 
		   NameLength2,
           TableName, 
		   NameLength3,
		   Unique,
		   Reserved,
		   false);
}
	
SQLRETURN  SQL_API SQLStatisticsA(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   SQLUSMALLINT Unique,
		   SQLUSMALLINT Reserved)
{
     return SQLStatistics_common(StatementHandle,
           CatalogName, 
		   NameLength1,
           SchemaName, 
		   NameLength2,
           TableName, 
		   NameLength3,
		   Unique,
		   Reserved,
		   false);
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
     return SQLStatistics_common(StatementHandle,
											(SQLCHAR*)CatalogName, 
		   NameLength1,
           (SQLCHAR*)SchemaName, 
		   NameLength2,
           (SQLCHAR*)TableName, 
		   NameLength3,
		   Unique,
		   Reserved,
		   true);
}


SQLRETURN  SQL_API SQLGetCursorName_common(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLengthPtr,
		   bool isWideCall)
{
	CStmt *pstmt = (CStmt *) StatementHandle;
	SQLRETURN	rc;

	ENTER_STMT (pstmt);
	/* check argument */
	if (BufferLength < (SWORD) 0)
    {
		PUSHSQLERR (pstmt, IDS_S1_090);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	/* check state */
	if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
    {
		PUSHSQLERR (pstmt, IDS_S1_010);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
       rc = NeoGetCursorName(StatementHandle,CursorName,BufferLength,NameLengthPtr,isWideCall);
        RETURNCODE (pstmt, rc);
        LEAVE_STMT (pstmt, rc);

	if (pstmt->state < en_stmt_cursoropen && pstmt->cursor_state == en_stmt_cursor_no)
    {
		PUSHSQLERR (pstmt, IDS_HY_015);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
}

SQLRETURN  SQL_API SQLGetCursorName(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLengthPtr)
{
	return SQLGetCursorName_common(StatementHandle,
           CursorName, 
		   BufferLength,
           NameLengthPtr,
		   false);

}
SQLRETURN  SQL_API SQLGetCursorNameA(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLengthPtr)
{
	return SQLGetCursorName_common(StatementHandle,
           CursorName, 
		   BufferLength,
           NameLengthPtr,
		   false);

}
SQLRETURN  SQL_API SQLGetCursorNameW(SQLHSTMT StatementHandle,
           SQLWCHAR *CursorName, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLengthPtr)
{
	return SQLGetCursorName_common(StatementHandle,
           (SQLCHAR*)CursorName, 
		   BufferLength,
           NameLengthPtr,
		   true);

}

SQLRETURN  SQL_API SQLSetCursorName_common(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, 
		   SQLSMALLINT NameLength,
		   bool isWideCall)
{
	SQLRETURN	rc;
	STMT (pstmt, StatementHandle);
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	if (CursorName == NULL)
    {
		PUSHSQLERR (pstmt, IDS_S1_009);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	if (NameLength < 0 && NameLength != SQL_NTS)
    {
		PUSHSQLERR (pstmt, IDS_S1_090);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
	}
	/* check state */
	if (pstmt->asyn_on != en_NullProc)
    {
		sqlstat = IDS_S1_010;
    }
	else
    {
		switch (pstmt->state)
		{
			case en_stmt_executed:
			case en_stmt_cursoropen:
			case en_stmt_fetched:
			case en_stmt_xfetched:
			  sqlstat = IDS_24_000;
			  break;

			case en_stmt_needdata:
			case en_stmt_mustput:
			case en_stmt_canput:
			  sqlstat = IDS_S1_010;
			  break;

			default:
			  break;
		}
	}
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoSetCursorName(StatementHandle,CursorName,NameLength,isWideCall);
	if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
		pstmt->cursor_state = en_stmt_cursor_named;
    }
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN  SQL_API SQLSetCursorName(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, 
		   SQLSMALLINT NameLength)
{
	return SQLSetCursorName_common(StatementHandle,
           CursorName, 
		   NameLength,
		   false);
}
		   
SQLRETURN  SQL_API SQLSetCursorNameA(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, 
		   SQLSMALLINT NameLength)
{
	return SQLSetCursorName_common(StatementHandle,
           CursorName, 
		   NameLength,
		   false);
}
	SQLRETURN  SQL_API SQLSetCursorNameW(SQLHSTMT StatementHandle,
           SQLWCHAR *CursorName, 
		   SQLSMALLINT NameLength)
{
	return SQLSetCursorName_common(StatementHandle,
           (SQLCHAR*)CursorName, 
		   NameLength,
		   true);
}

SQLRETURN  SQL_API SQLRowCount(SQLHSTMT StatementHandle, 
	   SQLLEN	*RowCountPtr)
{
	CStmt *pstmt = (CStmt FAR *) StatementHandle;
	SQLRETURN	rc;

	ENTER_STMT (pstmt);
	/* check state */
	if (pstmt->state >= en_stmt_needdata
		|| pstmt->state <= en_stmt_prepared
		|| pstmt->asyn_on != en_NullProc)
    {
		PUSHSQLERR (pstmt, IDS_S1_010);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoRowCount(StatementHandle,RowCountPtr);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}


SQLRETURN SQL_API SQLMoreResults(
    SQLHSTMT           StatementHandle)
{
	SQLRETURN	rc;
	STMT (pstmt, StatementHandle);

	ENTER_STMT (pstmt);
	/* check state */
	if (pstmt->asyn_on == en_NullProc)
	{
		switch (pstmt->state)
		{
#if 0
			case en_stmt_allocated:
			case en_stmt_prepared:
				RETURNCODE (pstmt, SQL_NO_DATA_FOUND);
				LEAVE_STMT (pstmt, SQL_NO_DATA_FOUND);
#endif
			case en_stmt_needdata:
			case en_stmt_mustput:
			case en_stmt_canput:
				PUSHSQLERR (pstmt, IDS_S1_010);
				RETURNCODE (pstmt, SQL_ERROR);
				LEAVE_STMT (pstmt, SQL_ERROR);
			default:
				break;
		}
	}
	else if (pstmt->asyn_on != en_MoreResults)
    {
		PUSHSQLERR (pstmt, IDS_S1_010);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoMoreResults(StatementHandle);

	/* state transition */
	if (pstmt->asyn_on == en_MoreResults)
	{
		switch (rc)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			case SQL_NO_DATA_FOUND:
			case SQL_ERROR:
				pstmt->asyn_on = en_NullProc;
				break;
			case SQL_STILL_EXECUTING:
			default:
				RETURNCODE (pstmt, rc);
				LEAVE_STMT (pstmt, rc);
		}
	}
	switch (pstmt->state)
	{
		case en_stmt_allocated:
		case en_stmt_prepared:
			/* driver should LEAVE_STMT (pstmt, SQL_NO_DATA_FOUND); */
			if (pstmt->prep_state)
			{
				pstmt->state = en_stmt_cursoropen;
			}
			else
			{
				pstmt->state = en_stmt_prepared;
			}
			break;
		case en_stmt_executed:
			if (rc == SQL_NO_DATA_FOUND)
			{
				if (pstmt->prep_state)
				{
					pstmt->state = en_stmt_prepared;
				}
				else
				{
					pstmt->state = en_stmt_cursoropen;
				}
			}
			else if (rc == SQL_STILL_EXECUTING)
			{
				pstmt->asyn_on = en_MoreResults;
			}
			break;
		case en_stmt_cursoropen:
		case en_stmt_fetched:
		case en_stmt_xfetched:
			if (rc == SQL_SUCCESS)
			{
				break;
			}
			else if (rc == SQL_NO_DATA_FOUND)
			{
				if (pstmt->prep_state)
				{
					pstmt->state = en_stmt_prepared;
				}
				else
				{
					pstmt->state = en_stmt_allocated;
				}
			}
			else if (rc == SQL_STILL_EXECUTING)
			{
				pstmt->asyn_on = en_MoreResults;
			}
			break;
		default:
			break;
	}
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLNativeSql_common(
    SQLHDBC            ConnectionHandle,
    SQLCHAR 		  *InStatementText,
    SQLINTEGER         TextLength1,
    SQLCHAR 		  *OutStatementText,
    SQLINTEGER         BufferLength,
    SQLINTEGER 		  *TextLength2Ptr,
	bool              isWideCall)
{
	SQLRETURN	rc;
	int sqlstat = 0;
	CONN (pdbc, ConnectionHandle);

	ENTER_HDBC (pdbc);

	/* check argument */
	if (InStatementText == NULL)
	{
		sqlstat = IDS_S1_009;
	}
	else if (TextLength1 < 0 && TextLength1 != SQL_NTS)
    {
		sqlstat = IDS_S1_090;
    }

	if (sqlstat != 0)
    {
		PUSHSQLERR (pdbc, sqlstat);
		RETURNCODE (pdbc, SQL_ERROR);
		LEAVE_HDBC (pdbc, SQL_ERROR);
    }
	/* check state */
	if (pdbc->state <= en_dbc_needdata)
	{
		PUSHSQLERR (pdbc, IDS_08_003);
		RETURNCODE (pdbc, SQL_ERROR);
		LEAVE_HDBC (pdbc, SQL_ERROR);
	}
	rc = NeoNativeSql(ConnectionHandle,InStatementText,TextLength1,OutStatementText,BufferLength,TextLength2Ptr,isWideCall);
	RETURNCODE (pdbc, rc);
	LEAVE_HDBC (pdbc, rc);
}

SQLRETURN SQL_API SQLNativeSql(
    SQLHDBC            ConnectionHandle,
    SQLCHAR 		  *InStatementText,
    SQLINTEGER         TextLength1,
    SQLCHAR 		  *OutStatementText,
    SQLINTEGER         BufferLength,
    SQLINTEGER 		  *TextLength2Ptr)
{
	return SQLNativeSql_common(
        ConnectionHandle,
        InStatementText,
        TextLength1,
        OutStatementText,
        BufferLength,
        TextLength2Ptr,
		false);
}

SQLRETURN SQL_API SQLNativeSqlA(
    SQLHDBC            ConnectionHandle,
    SQLCHAR 		  *InStatementText,
    SQLINTEGER         TextLength1,
    SQLCHAR 		  *OutStatementText,
    SQLINTEGER         BufferLength,
    SQLINTEGER 		  *TextLength2Ptr)
{
	return SQLNativeSql_common(
        ConnectionHandle,
        InStatementText,
        TextLength1,
        OutStatementText,
        BufferLength,
        TextLength2Ptr,
		false);
}
SQLRETURN SQL_API SQLNativeSqlW(
    SQLHDBC            ConnectionHandle,
    SQLWCHAR 		  *InStatementText,
    SQLINTEGER         TextLength1,
    SQLWCHAR 		  *OutStatementText,
    SQLINTEGER         BufferLength,
    SQLINTEGER 		  *TextLength2Ptr)
{
	return SQLNativeSql_common(
        ConnectionHandle,
        (SQLCHAR*)InStatementText,
        TextLength1,
        (SQLCHAR*)OutStatementText,
        BufferLength,
        TextLength2Ptr,
		true);
}

SQLRETURN SQL_API SQLCancel(SQLHSTMT StatementHandle)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN	rc;

	//ENTER_STMT (pstmt);
	if (!IS_VALID_HSTMT (pstmt))
	{
		return SQL_INVALID_HANDLE;
	}
	/* check argument */
	/* check state */
	rc = NeoCancel(StatementHandle);
	/* state transition */
	if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    {
		RETURNCODE (pstmt, rc);
		//LEAVE_STMT (pstmt, rc);
		return rc;
    }
	switch (pstmt->state)
	{
		case en_stmt_allocated:
		case en_stmt_prepared:
			break;
		case en_stmt_executed:
			if (pstmt->prep_state)
			{
				pstmt->state = en_stmt_prepared;
			}
			else
			{
				pstmt->state = en_stmt_allocated;
			}
			break;
		case en_stmt_cursoropen:
		case en_stmt_fetched:
		case en_stmt_xfetched:
			if (pstmt->prep_state)
			{
				pstmt->state = en_stmt_prepared;
			}
			else
			{
				pstmt->state = en_stmt_allocated;
			}
			break;
		case en_stmt_needdata:
		case en_stmt_mustput:
		case en_stmt_canput:
			switch (pstmt->need_on)
			{
				case en_ExecDirect:
					pstmt->state = en_stmt_allocated;
					break;
				case en_Execute:
					pstmt->state = en_stmt_prepared;
					break;
				case en_SetPos:
					pstmt->state = en_stmt_xfetched;
					break;
				default:
					break;
			}
			pstmt->need_on = en_NullProc;
			break;
		default:
			break;
	}
	RETURNCODE (pstmt, rc);
	//LEAVE_STMT (pstmt, rc);
	return rc;
}

SQLRETURN SQL_API SQLColAttribute_common(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttributePtr, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLengthPtr,
#if defined(_WIN64) || defined(__LP64__)            
		   SQLLEN *NumericAttributePtr,
#else
		   SQLPOINTER NumericAttributePtr,
#endif
           bool isWideCall)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN	rc;
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	/* check state */
	if (pstmt->asyn_on == en_NullProc)
    {
		if (pstmt->state == en_stmt_allocated
			|| pstmt->state >= en_stmt_needdata)
		{
			sqlstat = IDS_S1_010;
		}
    }
	else if (pstmt->asyn_on != en_ColAttribute)
    {
		sqlstat = IDS_S1_010;
    }
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoColAttribute(StatementHandle,ColumnNumber,FieldIdentifier,CharacterAttributePtr,BufferLength,StringLengthPtr,NumericAttributePtr,isWideCall);
	if (rc == SQL_SUCCESS && FieldIdentifier == SQL_DESC_CONCISE_TYPE)
	{
        SQLINTEGER ODBCAppVersion = pstmt->getODBCAppVersion();
        if (ODBCAppVersion >= SQL_OV_ODBC3)
        {
            switch((SQLINTEGER)*(SQLINTEGER*)NumericAttributePtr)
            {
                case SQL_DATE:
                    *(SQLINTEGER*)NumericAttributePtr = SQL_TYPE_DATE;
                    break;
                case SQL_TIME:
                    *(SQLINTEGER*)NumericAttributePtr = SQL_TYPE_TIME;
                    break;
                case SQL_TIMESTAMP:
                    *(SQLINTEGER*)NumericAttributePtr = SQL_TYPE_TIMESTAMP;
                    break;
            }
        }
        else
        {
            switch((SQLINTEGER)*(SQLINTEGER*)NumericAttributePtr)
            {
                case SQL_TYPE_DATE:
                    *(SQLINTEGER*)NumericAttributePtr = SQL_DATE;
                    break;
                case SQL_TYPE_TIME:
                    *(SQLINTEGER*)NumericAttributePtr = SQL_TIME;
                    break;
                case SQL_TYPE_TIMESTAMP:
                    *(SQLINTEGER*)NumericAttributePtr = SQL_TIMESTAMP;
                    break;
            }
        }
    }
	/* state transition */
	if (pstmt->asyn_on == en_ColAttribute)
    {
		switch (rc)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			case SQL_ERROR:
				pstmt->asyn_on = en_NullProc;
				break;
			default:
				RETURNCODE (pstmt, rc);
				LEAVE_STMT (pstmt, rc);
		}
    }
	switch (pstmt->state)
    {
		case en_stmt_prepared:
		case en_stmt_cursoropen:
		case en_stmt_fetched:
		case en_stmt_xfetched:
			if (rc == SQL_STILL_EXECUTING)
			{
				pstmt->asyn_on = en_ColAttribute;
			}
			break;
		default:
			break;
    }
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);

} // SQLColAttribute_common()

SQLRETURN SQL_API SQLColAttribute(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttributePtr, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLengthPtr,
#if defined(_WIN64) || defined(__LP64__)            
		   SQLLEN *NumericAttributePtr)
#else
		   SQLPOINTER NumericAttributePtr)
#endif
{
   return SQLColAttribute_common(StatementHandle,
                                 ColumnNumber, 
                                 FieldIdentifier,
                                 CharacterAttributePtr, 
                                 BufferLength,
                                 StringLengthPtr,
                                 NumericAttributePtr,
                                 false);

} // SQLColAttribute()

SQLRETURN SQL_API SQLColAttributeA(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttributePtr, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLengthPtr,
#if defined(_WIN64) || defined(__LP64__) || defined(__64BIT__) || defined(_LP64)           
		   SQLLEN *NumericAttributePtr)
#else
		   SQLPOINTER NumericAttributePtr)
#endif
{
   return SQLColAttribute_common(StatementHandle,
                                 ColumnNumber, 
                                 FieldIdentifier,
                                 CharacterAttributePtr, 
                                 BufferLength,
                                 StringLengthPtr,
                                 NumericAttributePtr,
                                 false);

} // SQLColAttributeA()

SQLRETURN SQL_API SQLColAttributeW(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttributePtr, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLengthPtr,
#if defined(_WIN64) || defined(__LP64__)            
		   SQLLEN *NumericAttributePtr)
#else
		   SQLPOINTER NumericAttributePtr)
#endif
{
   return SQLColAttribute_common(StatementHandle,
                                 ColumnNumber, 
                                 FieldIdentifier,
                                 CharacterAttributePtr, 
                                 BufferLength,
                                 StringLengthPtr,
                                 NumericAttributePtr,
                                 true);

} // SQLColAttributeW()

SQLRETURN SQL_API SQLExecute(SQLHSTMT StatementHandle)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN	rc;
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	/* check state */
	if (pstmt->asyn_on == en_NullProc)
    {
		switch (pstmt->state)
		{
			case en_stmt_allocated:
				if (pstmt->getODBCAppVersion() == SQL_OV_ODBC3)
					sqlstat = IDS_HY_010;
				else
					sqlstat = IDS_S1_010;
				break;
			case en_stmt_executed:
				if (!pstmt->prep_state)
				{
					sqlstat = IDS_S1_010;
				}
				break;
			case en_stmt_cursoropen:
				if (!pstmt->prep_state)
				{
					sqlstat = IDS_S1_010;
				}
				break;
			case en_stmt_fetched:
			case en_stmt_xfetched:
				if (!pstmt->prep_state)
				{
					sqlstat = IDS_S1_010;
				}
				else
				{
					sqlstat = IDS_24_000;
				}
				break;
			case en_stmt_needdata:
			case en_stmt_mustput:
			case en_stmt_canput:
				sqlstat = IDS_S1_010;
				break;
			default:
				break;
			}
	}
	else if (pstmt->asyn_on != en_Execute)
    {
		sqlstat = IDS_S1_010;
    }
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoExecute(StatementHandle);
	/* stmt state transition */
	if (pstmt->asyn_on == en_Execute)
    {
		switch (rc)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			case SQL_NEED_DATA:
			case SQL_ERROR:
				pstmt->asyn_on = en_NullProc;
				break;
			case SQL_STILL_EXECUTING:
			default:
				RETURNCODE (pstmt, rc);
				LEAVE_STMT (pstmt, rc);
		}
    }
	switch (pstmt->state)
    {
		case en_stmt_prepared:
			switch (rc)
			{
				case SQL_SUCCESS_WITH_INFO:
				case SQL_SUCCESS:
					fun_do_cursoropen (pstmt);
					break;
				case SQL_NEED_DATA:
					pstmt->state = en_stmt_needdata;
					pstmt->need_on = en_Execute;
					break;
				case SQL_STILL_EXECUTING:
					pstmt->asyn_on = en_Execute;
					break;
				default:
					break;
			}
			break;
		case en_stmt_executed:
			switch (rc)
			{
				case SQL_ERROR:
					pstmt->state = en_stmt_prepared;
//					pstmt->state = en_stmt_allocated;
//					pstmt->cursor_state = en_stmt_cursor_no;
//					pstmt->prep_state = 0;
					break;
				case SQL_NEED_DATA:
					pstmt->state = en_stmt_needdata;
					pstmt->need_on = en_Execute;
					break;
				case SQL_STILL_EXECUTING:
					pstmt->asyn_on = en_Execute;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN  SQL_API SQLParamData(SQLHSTMT StatementHandle,
           SQLPOINTER *ValuePtrPtr)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN	rc;

	ENTER_STMT (pstmt);
	/* check argument */
	/* check state */
	if (pstmt->asyn_on == en_NullProc)
    {
		if (pstmt->state <= en_stmt_xfetched)
		{
			PUSHSQLERR (pstmt, IDS_S1_010);
			RETURNCODE (pstmt, SQL_ERROR);
			LEAVE_STMT (pstmt, SQL_ERROR);
		}
    }
	else if (pstmt->asyn_on != en_ParamData)
    {
		PUSHSQLERR (pstmt, IDS_S1_010);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoParamData(StatementHandle,ValuePtrPtr);
	/* state transition */
	if (pstmt->asyn_on == en_ParamData)
    {
		switch (rc)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			case SQL_ERROR:
				pstmt->asyn_on = en_NullProc;
				break;
			case SQL_STILL_EXECUTING:
			default:
				RETURNCODE (pstmt, rc);
				LEAVE_STMT (pstmt, rc);
		}
    }
	if (pstmt->state < en_stmt_needdata)
    {
		RETURNCODE (pstmt, rc);
		LEAVE_STMT (pstmt, rc);
    }
	switch (rc)
    {
		case SQL_ERROR:
			switch (pstmt->need_on)
			{
				case en_ExecDirect:
					pstmt->state = en_stmt_allocated;
					break;
				case en_Execute:
					pstmt->state = en_stmt_prepared;
					break;
				case en_SetPos:
					pstmt->state = en_stmt_xfetched;
					pstmt->cursor_state = en_stmt_cursor_xfetched;
					break;
				default:
					break;
			}
			pstmt->need_on = en_NullProc;
			break;
		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO:
			switch (pstmt->state)
			{
				case en_stmt_needdata:
					pstmt->state = en_stmt_mustput;
					break;
				case en_stmt_canput:
					switch (pstmt->need_on)
					{
						case en_SetPos:
							pstmt->state = en_stmt_xfetched;
							pstmt->cursor_state = en_stmt_cursor_xfetched;
							break;
						case en_ExecDirect:
						case en_Execute:
							fun_do_cursoropen (pstmt);
							break;
						default:
							break;
					}
					break;
				default:
					break;
			}
			pstmt->need_on = en_NullProc;
			break;
		case SQL_NEED_DATA:
			pstmt->state = en_stmt_mustput;
			break;
		default:
			break;
	}
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN  SQL_API SQLPutData(SQLHSTMT StatementHandle,
           SQLPOINTER DataPtr, 
		   SQLLEN StrLen_or_Ind)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN	rc;

	ENTER_STMT (pstmt);
	/* check argument value */
	if (DataPtr == NULL &&
		(StrLen_or_Ind != SQL_DEFAULT_PARAM && StrLen_or_Ind != SQL_NULL_DATA))
	{
		PUSHSQLERR (pstmt, IDS_S1_009);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	/* check state */
	if (pstmt->asyn_on == en_NullProc)
    {
		if (pstmt->state <= en_stmt_xfetched)
		{
			PUSHSQLERR (pstmt, IDS_S1_010);
			RETURNCODE (pstmt, SQL_ERROR);
			LEAVE_STMT (pstmt, SQL_ERROR);
		}
    }
	else if (pstmt->asyn_on != en_PutData)
    {
		PUSHSQLERR (pstmt, IDS_S1_010);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoPutData(StatementHandle,DataPtr,StrLen_or_Ind);
	/* state transition */
	if (pstmt->asyn_on == en_PutData)
    {
		switch (rc)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			case SQL_ERROR:
				pstmt->asyn_on = en_NullProc;
				break;
			case SQL_STILL_EXECUTING:
			default:
				RETURNCODE (pstmt, rc);
				LEAVE_STMT (pstmt, rc);
		}
    }
	/* must in mustput or canput states */
	switch (rc)
    {
		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO:
			pstmt->state = en_stmt_canput;
			break;
		case SQL_ERROR:
			switch (pstmt->need_on)
			{
				case en_ExecDirect:
					pstmt->state = en_stmt_allocated;
					pstmt->need_on = en_NullProc;
					break;
				case en_Execute:
					if (pstmt->prep_state)
					{
						pstmt->state = en_stmt_prepared;
						pstmt->need_on = en_NullProc;
					}
					break;
				case en_SetPos:
					/* Is this possible ???? */
					pstmt->state = en_stmt_xfetched;
					break;
				default:
					break;
			}
			break;
		case SQL_STILL_EXECUTING:
			pstmt->asyn_on = en_PutData;
			break;
		default:
			break;
	}
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLFetch(SQLHSTMT StatementHandle)
{

	SQLRETURN	rc;
	STMT (pstmt, StatementHandle);
	ENTER_STMT (pstmt);
	/* check state */
	if (pstmt->asyn_on == en_NullProc)
    {
		switch (pstmt->state)
		{
			case en_stmt_allocated:
			case en_stmt_prepared:
			case en_stmt_xfetched:
			case en_stmt_needdata:
			case en_stmt_mustput:
			case en_stmt_canput:
				PUSHSQLERR (pstmt, IDS_S1_010);
				RETURNCODE (pstmt, SQL_ERROR);
				LEAVE_STMT (pstmt, SQL_ERROR);
			default:
				break;
		}
    }
	else if (pstmt->asyn_on != en_Fetch)
	{
		PUSHSQLERR (pstmt, IDS_S1_010);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
	}
	rc = NeoFetch(StatementHandle);
	/* state transition */
	if (pstmt->asyn_on == en_Fetch)
    {
		switch (rc)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			case SQL_NO_DATA_FOUND:
			case SQL_ERROR:
				pstmt->asyn_on = en_NullProc;
				break;
			case SQL_STILL_EXECUTING:
			default:
				RETURNCODE (pstmt, rc);
				LEAVE_STMT (pstmt, rc);
		}
    }

	switch (pstmt->state)
    {
		case en_stmt_cursoropen:
		case en_stmt_fetched:
			switch (rc)
			{
				case SQL_SUCCESS:
				case SQL_SUCCESS_WITH_INFO:
					pstmt->state = en_stmt_fetched;
					pstmt->cursor_state = en_stmt_cursor_fetched;
					break;

				case SQL_NO_DATA_FOUND:
					if (pstmt->prep_state)
					{
						pstmt->state = en_stmt_prepared;
					}
					else
					{
						pstmt->state = en_stmt_allocated;
					}
					pstmt->cursor_state = en_stmt_cursor_no;
					break;
				case SQL_STILL_EXECUTING:
					pstmt->asyn_on = en_Fetch;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLGetData(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLSMALLINT TargetType,
           SQLPOINTER TargetValuePtr, 
		   SQLLEN     BufferLength,
           SQLLEN    *StrLen_or_IndPtr)
{
	SQLRETURN	rc;
	STMT (pstmt, StatementHandle);
	int sqlstat = 0;
	ENTER_STMT (pstmt);

	/* check argument */
	if (TargetValuePtr == NULL)
    {
		sqlstat = IDS_S1_009;
    }
	else if (BufferLength < 0)
    {
		sqlstat = IDS_S1_090;
    }
	else
    {
		switch (TargetType)
		{
		case SQL_C_DEFAULT:
		case SQL_C_BINARY:
		case SQL_C_BIT:
		case SQL_C_CHAR:
		case SQL_C_WCHAR:
		case SQL_C_DATE:
		case SQL_C_DOUBLE:
		case SQL_C_FLOAT:
		case SQL_C_LONG:
		case SQL_C_SHORT:
		case SQL_C_SLONG:
		case SQL_C_SSHORT:
		case SQL_C_STINYINT:
		case SQL_C_TIME:
		case SQL_C_TIMESTAMP:
		case SQL_C_TINYINT:
		case SQL_C_ULONG:
		case SQL_C_USHORT:
		case SQL_C_UTINYINT:
		case SQL_C_GUID:
		case SQL_C_INTERVAL_DAY:
		case SQL_C_INTERVAL_DAY_TO_HOUR:
		case SQL_C_INTERVAL_DAY_TO_MINUTE:
		case SQL_C_INTERVAL_DAY_TO_SECOND:
		case SQL_C_INTERVAL_HOUR:
		case SQL_C_INTERVAL_HOUR_TO_MINUTE:
		case SQL_C_INTERVAL_HOUR_TO_SECOND:
		case SQL_C_INTERVAL_MINUTE:
		case SQL_C_INTERVAL_MINUTE_TO_SECOND:
		case SQL_C_INTERVAL_MONTH:
		case SQL_C_INTERVAL_SECOND:
		case SQL_C_INTERVAL_YEAR:
		case SQL_C_INTERVAL_YEAR_TO_MONTH:
		case SQL_C_NUMERIC:
		case SQL_C_SBIGINT:
		case SQL_C_TYPE_DATE:
		case SQL_C_TYPE_TIME:
		case SQL_C_TYPE_TIMESTAMP:
		case SQL_C_UBIGINT:
			break;

		default:
			sqlstat = IDS_S1_003;
			break;
		}
	}
	if (sqlstat != 0)
	{
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
	}

	/* check state */
	if (pstmt->asyn_on == en_NullProc)
    {
		switch (pstmt->state)
		{
			case en_stmt_allocated:
			case en_stmt_prepared:
			case en_stmt_needdata:
			case en_stmt_mustput:
			case en_stmt_canput:
				sqlstat = IDS_S1_010;
				break;
			case en_stmt_executed:
			case en_stmt_cursoropen:
				sqlstat = IDS_24_000;
				break;
			default:
				break;
		}
	}
	else if (pstmt->asyn_on != en_GetData)
    {
		sqlstat = IDS_S1_010;
    }
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoGetData(StatementHandle,ColumnNumber,TargetType,TargetValuePtr,BufferLength,StrLen_or_IndPtr);
	/* state transition */
	if (pstmt->asyn_on == en_GetData)
	{
		switch (rc)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			case SQL_NO_DATA_FOUND:
			case SQL_ERROR:
				pstmt->asyn_on = en_NullProc;
				break;
			case SQL_STILL_EXECUTING:
			default:
				RETURNCODE (pstmt, rc);
				LEAVE_STMT (pstmt, rc);
		}
    }
	switch (pstmt->state)
	{
		case en_stmt_fetched:
		case en_stmt_xfetched:
			if (rc == SQL_STILL_EXECUTING)
			{
				pstmt->asyn_on = en_GetData;
				break;
			}
			break;
		default:
			break;
	}
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLSetPos(
    SQLHSTMT        StatementHandle,
    SQLSETPOSIROW	RowNumber,
    SQLUSMALLINT    Operation,
    SQLUSMALLINT    LockType)
{
	SQLRETURN	rc;
	STMT (pstmt, StatementHandle);

	ENTER_STMT (pstmt);
	rc = fun_SetPos (StatementHandle, RowNumber, Operation, LockType);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLCopyDesc(SQLHDESC SourceDescHandle,
		SQLHDESC TargetDescHandle)
{
	DESC (desc, SourceDescHandle);
	DESC (desc1, TargetDescHandle);
	SQLRETURN	rc;

	ENTER_HDESC (desc);
	ODBC_LOCK ();
	if (!IS_VALID_HDESC (desc1))
    {
		ODBC_UNLOCK ();
		RETURNCODE (desc, SQL_INVALID_HANDLE);
		LEAVE_HDESC (desc, SQL_INVALID_HANDLE);
    }
	CLEAR_ERRORS (desc1);
	ODBC_UNLOCK ();
	rc = NeoCopyDesc(SourceDescHandle,TargetDescHandle);
	RETURNCODE (desc, rc);
	LEAVE_HDESC (desc, rc);
}

SQLRETURN SQL_API SQLColumnPrivileges_common(SQLHSTMT StatementHandle,
     SQLCHAR* CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR* SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR* TableName,
     SQLSMALLINT NameLength3,
     SQLCHAR* ColumnName,
     SQLSMALLINT NameLength4,
	 bool isWideCall)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN rc;
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	for (;;)
    {
		if (CatalogName != NULL && CatalogName[0] != 0 && NameLength1 < 0 && NameLength1 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (SchemaName != NULL && SchemaName[0] != 0 && NameLength2 < 0 && NameLength2 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (TableName != NULL && TableName[0] != 0 && NameLength3 < 0 && NameLength3 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (ColumnName != NULL && ColumnName[0] != 0 && NameLength4 < 0 && NameLength4 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		rc = fun_cata_state_ok (pstmt, en_ColumnPrivileges);
		if (rc != SQL_SUCCESS)
		{
			RETURNCODE (pstmt, SQL_ERROR);
			LEAVE_STMT (pstmt, SQL_ERROR);
		}
		sqlstat = 0;
		break;
    }
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoColumnPrivileges(StatementHandle,CatalogName,NameLength1,SchemaName,NameLength2,TableName,NameLength3,ColumnName,NameLength4,isWideCall);
	rc = fun_cata_state_tr (pstmt, en_ColumnPrivileges, rc);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLColumnPrivileges(SQLHSTMT StatementHandle,
     SQLCHAR* CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR* SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR* TableName,
     SQLSMALLINT NameLength3,
     SQLCHAR* ColumnName,
     SQLSMALLINT NameLength4)
{
     return SQLColumnPrivileges_common(StatementHandle,
               CatalogName,
               NameLength1,
               SchemaName,
               NameLength2,
               TableName,
               NameLength3,
               ColumnName,
               NameLength4,
		       false);
}

SQLRETURN SQL_API SQLColumnPrivilegesA(SQLHSTMT StatementHandle,
     SQLCHAR* CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR* SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR* TableName,
     SQLSMALLINT NameLength3,
     SQLCHAR* ColumnName,
     SQLSMALLINT NameLength4)
{
     return SQLColumnPrivileges_common(StatementHandle,
               CatalogName,
               NameLength1,
               SchemaName,
               NameLength2,
               TableName,
               NameLength3,
               ColumnName,
               NameLength4,
		       false);
}
SQLRETURN SQL_API SQLColumnPrivilegesW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
     SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
     SQLSMALLINT NameLength2,
     SQLWCHAR* TableName,
     SQLSMALLINT NameLength3,
           SQLWCHAR *ColumnName, 
     SQLSMALLINT NameLength4)
{
     return SQLColumnPrivileges_common(StatementHandle,
               (SQLCHAR*)CatalogName,
               NameLength1,
               (SQLCHAR*)SchemaName,
               NameLength2,
               (SQLCHAR*)TableName,
               NameLength3,
               (SQLCHAR*)ColumnName,
               NameLength4,
		       true);
}

SQLRETURN SQL_API SQLForeignKeys_common(SQLHSTMT StatementHandle,
     SQLCHAR *PKCatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *PKSchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *PKTableName,
     SQLSMALLINT NameLength3,
     SQLCHAR *FKCatalogName,
     SQLSMALLINT NameLength4,
     SQLCHAR *FKSchemaName,
     SQLSMALLINT NameLength5,
     SQLCHAR *FKTableName,
     SQLSMALLINT NameLength6,
	 bool isWideCall)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN rc;
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	for (;;)
    {
		if (PKCatalogName != NULL && PKCatalogName[0] != 0 && NameLength1 < 0 && NameLength1 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (PKSchemaName != NULL && PKSchemaName[0] != 0 && NameLength2 < 0 && NameLength2 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (PKTableName != NULL && PKTableName[0] != 0 && NameLength3 < 0 && NameLength3 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (FKCatalogName != NULL && FKCatalogName[0] != 0 && NameLength4 < 0 && NameLength4 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (FKSchemaName != NULL && FKSchemaName[0] != 0 && NameLength5 < 0 && NameLength5 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (FKTableName != NULL && FKTableName[0] != 0 && NameLength6 < 0 && NameLength6 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		rc = fun_cata_state_ok (pstmt, en_ForeignKeys);
		if (rc != SQL_SUCCESS)
		{
			RETURNCODE (pstmt, SQL_ERROR);
			LEAVE_STMT (pstmt, SQL_ERROR);
		}
		sqlstat = 0;
		break;
	}
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
	}
	rc = NeoForeignKeys(StatementHandle,PKCatalogName,NameLength1,PKSchemaName,NameLength2,PKTableName,NameLength3,FKCatalogName,NameLength4,FKSchemaName,NameLength5,FKTableName,NameLength6,isWideCall);
	rc = fun_cata_state_tr (pstmt, en_ForeignKeys, rc);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLForeignKeys(SQLHSTMT StatementHandle,
     SQLCHAR *PKCatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *PKSchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *PKTableName,
     SQLSMALLINT NameLength3,
     SQLCHAR *FKCatalogName,
     SQLSMALLINT NameLength4,
     SQLCHAR *FKSchemaName,
     SQLSMALLINT NameLength5,
     SQLCHAR *FKTableName,
     SQLSMALLINT NameLength6)
{
	return SQLForeignKeys_common(StatementHandle,
     PKCatalogName,
     NameLength1,
     PKSchemaName,
     NameLength2,
     PKTableName,
     NameLength3,
     FKCatalogName,
     NameLength4,
     FKSchemaName,
     NameLength5,
     FKTableName,
     NameLength6,
	 false);
}

SQLRETURN SQL_API SQLForeignKeysA(SQLHSTMT StatementHandle,
     SQLCHAR *PKCatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *PKSchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *PKTableName,
     SQLSMALLINT NameLength3,
     SQLCHAR *FKCatalogName,
     SQLSMALLINT NameLength4,
     SQLCHAR *FKSchemaName,
     SQLSMALLINT NameLength5,
     SQLCHAR *FKTableName,
     SQLSMALLINT NameLength6)
{
	return SQLForeignKeys_common(StatementHandle,
     PKCatalogName,
     NameLength1,
     PKSchemaName,
     NameLength2,
     PKTableName,
     NameLength3,
     FKCatalogName,
     NameLength4,
     FKSchemaName,
     NameLength5,
     FKTableName,
     NameLength6,
	 false);
}

SQLRETURN SQL_API SQLForeignKeysW(SQLHSTMT StatementHandle,
     SQLWCHAR *PKCatalogName,
     SQLSMALLINT NameLength1,
     SQLWCHAR *PKSchemaName,
     SQLSMALLINT NameLength2,
     SQLWCHAR *PKTableName,
     SQLSMALLINT NameLength3,
     SQLWCHAR *FKCatalogName,
     SQLSMALLINT NameLength4,
     SQLWCHAR *FKSchemaName,
     SQLSMALLINT NameLength5,
     SQLWCHAR *FKTableName,
     SQLSMALLINT NameLength6)
{
	return SQLForeignKeys_common(StatementHandle,
     (SQLCHAR*)PKCatalogName,
     NameLength1,
     (SQLCHAR*)PKSchemaName,
     NameLength2,
     (SQLCHAR*)PKTableName,
     NameLength3,
     (SQLCHAR*)FKCatalogName,
     NameLength4,
     (SQLCHAR*)FKSchemaName,
     NameLength5,
     (SQLCHAR*)FKTableName,
     NameLength6,
	 true);
}


SQLRETURN SQL_API SQLPrimaryKeys_common(SQLHSTMT StatementHandle,
     SQLCHAR *CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *TableName,
     SQLSMALLINT NameLength3,
	 bool isWideCall)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN rc;
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	for (;;)
    {
		if (CatalogName != NULL && CatalogName[0] != 0 && NameLength1 < 0 && NameLength1 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (SchemaName != NULL && SchemaName[0] != 0 && NameLength2 < 0 && NameLength2 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (TableName != NULL && TableName[0] != 0 && NameLength3 < 0 && NameLength3 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		rc = fun_cata_state_ok (pstmt, en_PrimaryKeys);
		if (rc != SQL_SUCCESS)
		{
			RETURNCODE (pstmt, SQL_ERROR);
			LEAVE_STMT (pstmt, SQL_ERROR);
		}
		sqlstat = 0;
		break;
	}
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoPrimaryKeys(StatementHandle,CatalogName,NameLength1,SchemaName,NameLength2,TableName,NameLength3,isWideCall);
	rc = fun_cata_state_tr (pstmt, en_PrimaryKeys, rc);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLPrimaryKeys(SQLHSTMT StatementHandle,
     SQLCHAR *CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *TableName,
     SQLSMALLINT NameLength3)
{
     return SQLPrimaryKeys_common(StatementHandle,
             CatalogName,
             NameLength1,
             SchemaName,
             NameLength2,
             TableName,
             NameLength3,
             false);
}

SQLRETURN SQL_API SQLPrimaryKeysA(SQLHSTMT StatementHandle,
     SQLCHAR *CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *TableName,
     SQLSMALLINT NameLength3)
{
     return SQLPrimaryKeys_common(StatementHandle,
             CatalogName,
             NameLength1,
             SchemaName,
             NameLength2,
             TableName,
             NameLength3,
             false);
}
 SQLRETURN SQL_API SQLPrimaryKeysW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
     SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
     SQLSMALLINT NameLength2,
           SQLWCHAR *TableName, 
     SQLSMALLINT NameLength3)
{
     return SQLPrimaryKeys_common(StatementHandle,
											 (SQLCHAR*)CatalogName,
             NameLength1,
											 (SQLCHAR*)SchemaName,
             NameLength2,
											 (SQLCHAR*)TableName,
             NameLength3,
             true);
}

SQLRETURN SQL_API SQLProcedureColumns_common( SQLHSTMT StatementHandle,
     SQLCHAR *CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *ProcName,
     SQLSMALLINT NameLength3,
     SQLCHAR *ColumnName,
     SQLSMALLINT NameLength4,
	 bool isWideCall)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN rc;
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	for (;;)
    {
		if (CatalogName != NULL && CatalogName[0] != 0 && NameLength1 < 0 && NameLength1 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (SchemaName != NULL && SchemaName[0] != 0 && NameLength2 < 0 && NameLength2 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (ProcName != NULL && ProcName[0] != 0 && NameLength3 < 0 && NameLength3 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (ColumnName != NULL && ColumnName[0] != 0 && NameLength4 < 0 && NameLength4 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		rc = fun_cata_state_ok (pstmt, en_ProcedureColumns);
		if (rc != SQL_SUCCESS)
		{
			RETURNCODE (pstmt, SQL_ERROR);
			LEAVE_STMT (pstmt, SQL_ERROR);
		}
		sqlstat = 0;
		break;
	}
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
	}
	rc = NeoProcedureColumns(StatementHandle,CatalogName,NameLength1,SchemaName,NameLength2,ProcName,NameLength3,ColumnName,NameLength4,isWideCall);
	rc = fun_cata_state_tr (pstmt, en_ProcedureColumns, rc);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLProcedureColumns(SQLHSTMT StatementHandle,
     SQLCHAR *CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *ProcName,
     SQLSMALLINT NameLength3,
     SQLCHAR *ColumnName,
     SQLSMALLINT NameLength4)
{
	return SQLProcedureColumns_common(StatementHandle,
     CatalogName,
     NameLength1,
     SchemaName,
     NameLength2,
     ProcName,
     NameLength3,
     ColumnName,
     NameLength4,
	 false);
}

SQLRETURN SQL_API SQLProcedureColumnsA(SQLHSTMT StatementHandle,
     SQLCHAR *CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *ProcName,
     SQLSMALLINT NameLength3,
     SQLCHAR *ColumnName,
     SQLSMALLINT NameLength4)
{
	return SQLProcedureColumns_common(StatementHandle,
     CatalogName,
     NameLength1,
     SchemaName,
     NameLength2,
     ProcName,
     NameLength3,
     ColumnName,
     NameLength4,
	 false);
}

SQLRETURN SQL_API SQLProcedureColumnsW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
     SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
     SQLSMALLINT NameLength2,
           SQLWCHAR *ProcName, 
     SQLSMALLINT NameLength3,
           SQLWCHAR *ColumnName, 
     SQLSMALLINT NameLength4)
{
	return SQLProcedureColumns_common(StatementHandle,
     (SQLCHAR*)CatalogName,
     NameLength1,
     (SQLCHAR*)SchemaName,
     NameLength2,
     (SQLCHAR*)ProcName,
     NameLength3,
     (SQLCHAR*)ColumnName,
     NameLength4,
	 true);
}


SQLRETURN SQL_API SQLProcedures_common(SQLHSTMT  StatementHandle,
     SQLCHAR *CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *ProcName,
     SQLSMALLINT NameLength3,
	 bool isWideCall)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN rc;
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	for (;;)
    {
		if (CatalogName != NULL && CatalogName[0] != 0 && NameLength1 < 0 && NameLength1 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (SchemaName != NULL && SchemaName[0] != 0 && NameLength2 < 0 && NameLength2 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (ProcName != NULL && ProcName[0] != 0 && NameLength3 < 0 && NameLength3 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		rc = fun_cata_state_ok (pstmt, en_Procedures);
		if (rc != SQL_SUCCESS)
		{
			RETURNCODE (pstmt, SQL_ERROR);
			LEAVE_STMT (pstmt, SQL_ERROR);
		}
		sqlstat = 0;
		break;
	}
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
	}
	rc = NeoProcedures(StatementHandle,CatalogName,NameLength1,SchemaName,NameLength2,ProcName,NameLength3,isWideCall);
	rc = fun_cata_state_tr (pstmt, en_Procedures, rc);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLProcedures(SQLHSTMT  StatementHandle,
     SQLCHAR *CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *ProcName,
     SQLSMALLINT NameLength3)
{
	return SQLProcedures_common(StatementHandle,
     CatalogName,
     NameLength1,
     SchemaName,
     NameLength2,
     ProcName,
     NameLength3,
	 false);
}

SQLRETURN SQL_API SQLProceduresA(SQLHSTMT  StatementHandle,
     SQLCHAR *CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *ProcName,
     SQLSMALLINT NameLength3)
{
	return SQLProcedures_common(StatementHandle,
     CatalogName,
     NameLength1,
     SchemaName,
     NameLength2,
     ProcName,
     NameLength3,
	 false);
}

SQLRETURN SQL_API SQLProceduresW(SQLHSTMT  StatementHandle,
           SQLWCHAR *CatalogName, 
     SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
     SQLSMALLINT NameLength2,
           SQLWCHAR *ProcName, 
     SQLSMALLINT NameLength3)
{
	return SQLProcedures_common(StatementHandle,
     (SQLCHAR*)CatalogName,
     NameLength1,
     (SQLCHAR*)SchemaName,
     NameLength2,
     (SQLCHAR*)ProcName,
     NameLength3,
	 true);
}

SQLRETURN SQL_API SQLTablePrivileges_common(SQLHSTMT StatementHandle,
     SQLCHAR *CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *TableName,
     SQLSMALLINT NameLength3,
	 bool isWideCall)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN rc;
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	for (;;)
    {
		if (CatalogName != NULL && CatalogName[0] != 0 && NameLength1 < 0 && NameLength1 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (SchemaName != NULL && SchemaName[0] != 0 && NameLength2 < 0 && NameLength2 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		if (TableName != NULL && TableName[0] != 0 && NameLength3 < 0 && NameLength3 != SQL_NTS)
		{
			sqlstat = IDS_S1_090;
			break;
		}
		rc = fun_cata_state_ok (pstmt, en_TablePrivileges);
		if (rc != SQL_SUCCESS)
		{
			RETURNCODE (pstmt, SQL_ERROR);
			LEAVE_STMT (pstmt, SQL_ERROR);
		}
		rc = 0;
		break;
	}
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoTablePrivileges(StatementHandle,CatalogName,NameLength1,SchemaName,NameLength2,TableName,NameLength3,isWideCall);
	rc = fun_cata_state_tr (pstmt, en_TablePrivileges, rc);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLTablePrivileges(SQLHSTMT StatementHandle,
     SQLCHAR *CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *TableName,
     SQLSMALLINT NameLength3)
{
	return SQLTablePrivileges_common(StatementHandle,
     CatalogName,
     NameLength1,
     SchemaName,
     NameLength2,
     TableName,
     NameLength3,
	 false);
}

SQLRETURN SQL_API SQLTablePrivilegesA(SQLHSTMT StatementHandle,
     SQLCHAR *CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *TableName,
     SQLSMALLINT NameLength3)
{
	return SQLTablePrivileges_common(StatementHandle,
     CatalogName,
     NameLength1,
     SchemaName,
     NameLength2,
     TableName,
     NameLength3,
	 false);
}
SQLRETURN SQL_API SQLTablePrivilegesW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
     SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
     SQLSMALLINT NameLength2,
     SQLWCHAR* TableName,
     SQLSMALLINT NameLength3)
{
	return SQLTablePrivileges_common(StatementHandle,
     (SQLCHAR*)CatalogName,
     NameLength1,
     (SQLCHAR*)SchemaName,
     NameLength2,
     (SQLCHAR*)TableName,
     NameLength3,
	 true);
}
RETCODE SQL_API SQLBulkOperations (
	SQLHSTMT statementHandle,
    SQLSMALLINT Operation)
{
	STMT (pstmt, statementHandle);
	RETCODE rc;

	ENTER_STMT (pstmt);
	switch (Operation)
    {
		case SQL_ADD:
		case SQL_UPDATE_BY_BOOKMARK:
		case SQL_DELETE_BY_BOOKMARK:
		case SQL_FETCH_BY_BOOKMARK:
			break;
		default:
			PUSHSQLERR (pstmt, IDS_HY_092);
			RETURNCODE (pstmt, SQL_ERROR);
			LEAVE_STMT (pstmt, SQL_ERROR);
    }
	rc = NeoBulkOperations(statementHandle, Operation);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

RETCODE SQL_API SQLFetchScroll (
	SQLHSTMT statementHandle,
    SQLSMALLINT fetchOrientation,
    SQLLEN fetchOffset)
{
	STMT (pstmt, statementHandle);
	RETCODE rc;

	ENTER_STMT (pstmt);
	switch (fetchOrientation)
    {
		case SQL_FETCH_NEXT:
		case SQL_FETCH_PRIOR:
		case SQL_FETCH_FIRST:
		case SQL_FETCH_LAST:
		case SQL_FETCH_ABSOLUTE:
		case SQL_FETCH_RELATIVE:
		case SQL_FETCH_BOOKMARK:
			break;
		default:
			PUSHSQLERR (pstmt, IDS_HY_092);
			RETURNCODE (pstmt, SQL_ERROR);
			LEAVE_STMT (pstmt, SQL_ERROR);
	}
	rc = NeoFetchScroll(statementHandle, fetchOrientation, fetchOffset);
	if (pstmt->asyn_on == en_Fetch)
    {
		switch (rc)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			case SQL_NO_DATA_FOUND:
			case SQL_ERROR:
				pstmt->asyn_on = en_NullProc;
				break;
			case SQL_STILL_EXECUTING:
			default:
				RETURNCODE (pstmt, rc);
				LEAVE_STMT (pstmt, rc);
		}
    }

	switch (pstmt->state)
    {
		case en_stmt_cursoropen:
		case en_stmt_fetched:
			switch (rc)
			{
				case SQL_SUCCESS:
				case SQL_SUCCESS_WITH_INFO:
					pstmt->state = en_stmt_fetched;
					pstmt->cursor_state = en_stmt_cursor_fetched;
					break;

				case SQL_NO_DATA_FOUND:
					if (pstmt->prep_state)
					{
						pstmt->state = en_stmt_prepared;
					}
					else
					{
						pstmt->state = en_stmt_allocated;
					}
					pstmt->cursor_state = en_stmt_cursor_no;
					break;
				case SQL_STILL_EXECUTING:
					pstmt->asyn_on = en_Fetch;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}

	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLGetFunctions(
     SQLHDBC ConnectionHandle,
     SQLUSMALLINT FunctionId,
     SQLUSMALLINT * SupportedPtr)
{
	CONN (pdbc, ConnectionHandle);
	SQLRETURN rc;
	int i;
	UWORD functions2[100];
	UWORD functions3[SQL_API_ODBC3_ALL_FUNCTIONS_SIZE];

	ENTER_HDBC (pdbc);
	if (pdbc->state == en_dbc_allocated || pdbc->state == en_dbc_needdata)
    {
		PUSHSQLERR (pdbc, IDS_S1_010);
		RETURNCODE (pdbc, SQL_ERROR);
		LEAVE_HDBC (pdbc, SQL_ERROR);
    }
	if (SupportedPtr == NULL)
    {
		RETURNCODE (pdbc, SQL_SUCCESS);
		LEAVE_HDBC (pdbc, SQL_SUCCESS);
    }
	/*
	*  These functions are supported by the iODBC driver manager
	*/
	if (FunctionId == SQL_API_SQLDATASOURCES || FunctionId == SQL_API_SQLDRIVERS
		|| FunctionId == SQL_API_SQLGETENVATTR || FunctionId == SQL_API_SQLSETENVATTR)
    {
		*SupportedPtr = (UWORD) 1;
		RETURNCODE (pdbc, SQL_SUCCESS);
		LEAVE_HDBC (pdbc, SQL_SUCCESS);
    }
	/*
	*  Map deprecated functions
	*/
	if (FunctionId == SQL_API_SQLSETPARAM)
    {
		FunctionId = SQL_API_SQLBINDPARAMETER;
    }
	/*
	*  Initialize intermediate result arrays
	*/
	memset (functions2, '\0', sizeof (functions2));
	memset (functions3, '\0', sizeof (functions3));
	/*
	*  Build result array by scanning for all API calls
	*/
	for (i = 1;; i++)
    {
		int j = FunctionNumbers[i];
		if (j == __LAST_API_FUNCTION__) break;
		if (j != 0)
		{
			if (j < 100) functions2[j] = 1;
			functions3[j >> 4] |= (1 << (j & 0x000F));
		}
    }
	/*
	*  Finally return the information
	*/
	if (FunctionId == SQL_API_ALL_FUNCTIONS)
    {
		memcpy (SupportedPtr, &functions2, sizeof (functions2));
    }
	else if (FunctionId == SQL_API_ODBC3_ALL_FUNCTIONS)
    {
		memcpy (SupportedPtr, &functions3, sizeof (functions3));
    }
	else
    {
		*SupportedPtr = SQL_FUNC_EXISTS (functions3, FunctionId);
    }
	RETURNCODE (pdbc, SQL_SUCCESS);
	LEAVE_HDBC (pdbc, SQL_SUCCESS);
}

SQLRETURN SQL_API SQLExtendedFetch(SQLHSTMT StatementHandle,
     SQLUSMALLINT FetchOrientation,
     SQLLEN FetchOffset,
     SQLULEN *RowCountPtr,
     SQLUSMALLINT *RowStatusArray)
{
	STMT (pstmt, StatementHandle);
	SQLRETURN rc;

	ENTER_STMT (pstmt);
	rc = fun_ExtendedFetch (StatementHandle,FetchOrientation,FetchOffset,RowCountPtr,RowStatusArray);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

#if defined(ASYNCIO) && defined(DBSELECT)
extern "C"
SQLRETURN  SQL_API dbSelect( int ArraySize, 
							 SQLHANDLE *Connhandles,
							 SQLHANDLE *Stmthandles,
							 const struct timespec *timeout)
{
	return gDrvrGlobal.gdbSelect->dbSelect(ArraySize,Connhandles,Stmthandles,timeout);
} // dbSelect()
#endif /* ASYNCIO && DBSELECT */
