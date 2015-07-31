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
#include "cenv.h"
#include "cconnect.h"
#include "cstmt.h"
#include "cdesc.h"
#include "dmfunctions.h"
#include "drvrmanager.h"
#include "dmmapping.h"
#ifdef unixcli
#include "unix_extra.h"
#endif

using namespace ODBC;

#ifndef unixcli
SQLRETURN SQL_API SQLAllocConnect (
    SQLHENV henv,
    SQLHDBC FAR * phdbc)
#else
SQLRETURN SQL_API SQLAllocConnect (
    SQLHENV henv,
    SQLHDBC * phdbc)
#endif

{
	SQLRETURN rc = SQL_SUCCESS;
	GENV(genv, henv);
	if (!IS_VALID_HENV (genv))
    {
		return (SQL_INVALID_HANDLE);
    }
	if (genv->getODBCAppVersion() == 0)
		  rc = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC2,0);
	if ( rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
		rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, phdbc);
	return rc;
}

#ifndef unixcli
SQLRETURN SQL_API SQLAllocEnv (SQLHENV FAR * phenv)
#else
SQLRETURN SQL_API SQLAllocEnv (SQLHENV * phenv)
#endif
{
	SQLRETURN rc = SQL_SUCCESS;
	rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, phenv);
        if ( rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
        	rc = SQLSetEnvAttr(*phenv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC2,0);
	return rc;
	
}

SQLRETURN SQL_API SQLAllocStmt ( 
	SQLHDBC hdbc,
    SQLHSTMT* phstmt)
{
	CONN (pdbc, hdbc);
	SQLRETURN rc = SQL_SUCCESS;

	ENTER_HDBC (pdbc);
	if (phstmt == NULL)
    {
		PUSHSQLERR (pdbc, IDS_S1_009);
		RETURNCODE (pdbc, SQL_ERROR);
		LEAVE_HDBC (pdbc, SQL_ERROR);
    }
	if (pdbc->getConnectionStatus() == SQL_CD_TRUE)
	{
		PUSHSQLERR (pdbc, IDS_08_003);
		RETURNCODE (pdbc, SQL_ERROR);
		LEAVE_HDBC (pdbc, SQL_ERROR);
	}
	/* check state */
	switch (pdbc->state)
	{
		case en_dbc_connected:
		case en_dbc_hstmt:
			break;
		case en_dbc_allocated:
		case en_dbc_needdata:
			PUSHSQLERR (pdbc, IDS_08_003);
			*phstmt = SQL_NULL_HSTMT;
			RETURNCODE (pdbc, SQL_ERROR);
			LEAVE_HDBC (pdbc, SQL_ERROR);
		default:
			RETURNCODE (pdbc, SQL_INVALID_HANDLE);
			LEAVE_HDBC (pdbc, SQL_INVALID_HANDLE);
     }

	rc = NeoAllocHandle(SQL_HANDLE_STMT, hdbc, phstmt);
	if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    {
		*phstmt = SQL_NULL_HSTMT;
		RETURNCODE (pdbc, rc);
		LEAVE_HDBC (pdbc, rc);
    }
	/* state transition */
	pdbc->state = en_dbc_hstmt;
	RETURNCODE (pdbc, SQL_SUCCESS);
	LEAVE_HDBC (pdbc, SQL_SUCCESS);
}

SQLRETURN SQL_API SQLBindParam (
    SQLHSTMT hstmt,
    SQLUSMALLINT ipar,
    SQLSMALLINT fCType,
    SQLSMALLINT fSqlType,
    SQLUINTEGER cbParamDef,
    SQLSMALLINT ibScale,
    SQLPOINTER rgbValue,
    SQLLEN *pcbValue)
{
	return SQLBindParameter (hstmt, ipar, SQL_PARAM_INPUT, fCType, fSqlType, cbParamDef, ibScale, rgbValue, SQL_MAX_OPTION_STRING_LENGTH, pcbValue);
}

SQLRETURN SQL_API SQLColAttributes_common (
    SQLHSTMT hstmt,
    SQLUSMALLINT icol,
    SQLUSMALLINT fDescType,
    SQLPOINTER rgbDesc,
    SQLSMALLINT cbDescMax,
    SQLSMALLINT FAR * pcbDesc,
    SQLLEN FAR * pfDesc,
    bool isWideCall)
{
	CStmt *pstmt = (CStmt *) hstmt;
	SQLRETURN rc;
	int sqlstat = 0;

	ENTER_STMT (pstmt);
	/* check arguments */
	if (icol == 0 && fDescType != SQL_COLUMN_COUNT)
    {
		sqlstat = IDS_S1_002;
    }
	else if (cbDescMax < 0)
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
		if (pstmt->state == en_stmt_allocated
			|| pstmt->state >= en_stmt_needdata)
		{
			sqlstat = IDS_S1_010;
		}
    }
	else if (pstmt->asyn_on != en_ColAttributes)
    {
		sqlstat = IDS_S1_010;
    }
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
    }
	SQLUSMALLINT new_attr = fDescType;
	switch (new_attr)
	{
		case SQL_COLUMN_NAME:
			new_attr = SQL_DESC_NAME;
			break;
		case SQL_COLUMN_NULLABLE:
			new_attr = SQL_DESC_NULLABLE;
			break;
		case SQL_COLUMN_COUNT:
			new_attr = SQL_DESC_COUNT;
			break;
	}
	rc = NeoColAttribute(hstmt,icol,new_attr,rgbDesc,cbDescMax,pcbDesc,pfDesc,isWideCall);
	/* state transition */
	if (pstmt->asyn_on == en_ColAttributes)
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
				pstmt->asyn_on = en_ColAttributes;
			}
			break;
		default:
			break;
    }
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLColAttributes(
    SQLHSTMT hstmt,
    SQLUSMALLINT icol,
    SQLUSMALLINT fDescType,
    SQLPOINTER rgbDesc,
    SQLSMALLINT cbDescMax,
    SQLSMALLINT FAR * pcbDesc,
    SQLLEN FAR * pfDesc)
{
   return SQLColAttributes_common (
        hstmt,
        icol,
        fDescType,
        rgbDesc,
        cbDescMax,
        pcbDesc,
        pfDesc,
        false);
}
SQLRETURN SQL_API SQLColAttributesA(
    SQLHSTMT hstmt,
    SQLUSMALLINT icol,
    SQLUSMALLINT fDescType,
    SQLPOINTER rgbDesc,
    SQLSMALLINT cbDescMax,
    SQLSMALLINT FAR * pcbDesc,
    SQLLEN FAR * pfDesc)
{
   return SQLColAttributes_common (
        hstmt,
        icol,
        fDescType,
        rgbDesc,
        cbDescMax,
        pcbDesc,
        pfDesc,
        false);
}
SQLRETURN SQL_API SQLColAttributesW(
    SQLHSTMT hstmt,
    SQLUSMALLINT icol,
    SQLUSMALLINT fDescType,
    SQLPOINTER rgbDesc,
    SQLSMALLINT cbDescMax,
    SQLSMALLINT FAR * pcbDesc,
    SQLLEN FAR * pfDesc)
{
   return SQLColAttributes_common (
        hstmt,
        icol,
        fDescType,
        rgbDesc,
        cbDescMax,
        pcbDesc,
        pfDesc,
        true);
}

SQLRETURN SQL_API SQLError (
    SQLHENV henv,
    SQLHDBC hdbc,
    SQLHSTMT hstmt,
    SQLCHAR FAR * szSqlstate,
    SQLINTEGER FAR * pfNativeError,
    SQLCHAR FAR * szErrorMsg,
    SQLSMALLINT cbErrorMsgMax,
    SQLSMALLINT FAR * pcbErrorMsg)
{
	SQLRETURN retcode;

	ODBC_LOCK ();
	retcode = fun_sqlerror (henv, hdbc, hstmt, szSqlstate, pfNativeError,
		szErrorMsg, cbErrorMsgMax, pcbErrorMsg, 1,false);
	ODBC_UNLOCK ();
	return retcode;
}

SQLRETURN SQL_API SQLErrorA (
    SQLHENV henv,
    SQLHDBC hdbc,
    SQLHSTMT hstmt,
    SQLCHAR FAR * szSqlstate,
    SQLINTEGER FAR * pfNativeError,
    SQLCHAR FAR * szErrorMsg,
    SQLSMALLINT cbErrorMsgMax,
    SQLSMALLINT FAR * pcbErrorMsg)
{
	SQLRETURN retcode;

	ODBC_LOCK ();
	retcode = fun_sqlerror (henv, hdbc, hstmt, szSqlstate, pfNativeError,
		szErrorMsg, cbErrorMsgMax, pcbErrorMsg, 1,false);
	ODBC_UNLOCK ();
	return retcode;
}

SQLRETURN SQL_API SQLErrorW (
    SQLHENV henv,
    SQLHDBC hdbc,
    SQLHSTMT hstmt,
    SQLWCHAR* szSqlstate,
    SQLINTEGER* pfNativeError,
    SQLWCHAR* szErrorMsg,
    SQLSMALLINT cbErrorMsgMax,
    SQLSMALLINT* pcbErrorMsg)
{
	SQLRETURN retcode;

	ODBC_LOCK ();
	retcode = fun_sqlerror (henv, hdbc, hstmt, (SQLCHAR*)szSqlstate, pfNativeError,
									(SQLCHAR*)szErrorMsg, cbErrorMsgMax, pcbErrorMsg, 1,true);
	ODBC_UNLOCK ();
	return retcode;
}

SQLRETURN SQL_API SQLFreeConnect (
	SQLHDBC hdbc)
{
	return SQLFreeHandle(SQL_HANDLE_DBC,hdbc);
}

SQLRETURN SQL_API SQLFreeEnv (SQLHENV henv)
{
	return SQLFreeHandle(SQL_HANDLE_ENV,henv);
}

SQLRETURN SQL_API SQLFreeStmt(SQLHSTMT StatementHandle,
        SQLUSMALLINT Option)
{
	SQLRETURN	rc;

	if(Option == SQL_DROP)
	   return SQLFreeHandle(SQL_HANDLE_STMT,StatementHandle);

	STMT (pstmt, StatementHandle);

	ODBC_LOCK ();
	if (!IS_VALID_HSTMT (pstmt))
    {
		ODBC_UNLOCK ();
		return SQL_INVALID_HANDLE;
    }
	CLEAR_ERRORS (pstmt);

	/* check option */
	switch (Option)
	{
		case SQL_DROP:
		case SQL_CLOSE:
		case SQL_UNBIND:
		case SQL_RESET_PARAMS:
			break;
		default:
			PUSHSQLERR (pstmt, IDS_S1_092);
			RETURNCODE (pstmt, SQL_ERROR);
			ODBC_UNLOCK ();
			return SQL_ERROR;
    }
	/* check state */
	if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
    {
		PUSHSQLERR (pstmt, IDS_S1_010);
		RETURNCODE (pstmt, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	if (Option != SQL_DROP)
		rc = NeoFreeStmt(StatementHandle,Option);
	if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    {
		RETURNCODE (pstmt, rc);
		ODBC_UNLOCK ();
		return rc;
    }
	/* state transition */
	switch (Option)
    {
		case SQL_DROP:
			break;
		case SQL_CLOSE:
			pstmt->cursor_state = en_stmt_cursor_no;
			/* This means cursor name set by
			* SQLSetCursorName() call will also 
			* be erased.
			*/
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
					{
						pstmt->state = en_stmt_prepared;
					}
					else
					{
						pstmt->state = en_stmt_allocated;
					}
					break;
				default:
					break;
			}
			break;
		case SQL_UNBIND:
		case SQL_RESET_PARAMS:
		default:
			break;
	}
	RETURNCODE (pstmt, rc);
	ODBC_UNLOCK ();
	return rc;
}

SQLRETURN SQL_API SQLGetConnectOption (
    SQLHDBC hdbc,
    SQLUSMALLINT fOption,
    SQLPOINTER pvParam)
{
	CONN (pdbc, hdbc);
	SQLRETURN rc;

	ENTER_HDBC (pdbc);
	rc = fun_GetConnectOption (hdbc, fOption, pvParam,false);
	RETURNCODE (pdbc, rc);
	LEAVE_HDBC (pdbc, rc);
}

SQLRETURN SQL_API SQLGetConnectOptionA (
    SQLHDBC hdbc,
    SQLUSMALLINT fOption,
    SQLPOINTER pvParam)
{
	CONN (pdbc, hdbc);
	SQLRETURN rc;

	ENTER_HDBC (pdbc);
	rc = fun_GetConnectOption (hdbc, fOption, pvParam,false);
	RETURNCODE (pdbc, rc);
	LEAVE_HDBC (pdbc, rc);
}

SQLRETURN SQL_API SQLGetConnectOptionW (
    SQLHDBC hdbc,
    SQLUSMALLINT fOption,
    SQLPOINTER pvParam)
{
	CONN (pdbc, hdbc);
	SQLRETURN rc;

	ENTER_HDBC (pdbc);
	rc = fun_GetConnectOption (hdbc, fOption, pvParam,true);
	RETURNCODE (pdbc, rc);
	LEAVE_HDBC (pdbc, rc);
}

SQLRETURN SQL_API SQLGetStmtOption (
    SQLHSTMT hstmt,
    SQLUSMALLINT fOption,
    SQLPOINTER pvParam )
{
	STMT (pstmt, hstmt);
	int sqlstat = 0;
	SQLRETURN rc;

	ENTER_STMT (pstmt);
	/* check state */
	if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
	{
		sqlstat = IDS_S1_010;
    }
	else
    {
		switch (pstmt->state)
		{
			case en_stmt_allocated:
			case en_stmt_prepared:
			case en_stmt_executed:
			case en_stmt_cursoropen:
				if (fOption == SQL_ROW_NUMBER || fOption == SQL_GET_BOOKMARK)
				{
					sqlstat = IDS_24_000;
				}
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

	switch (fOption)
	{
		/* ODBC integer attributes */
		case SQL_ATTR_ASYNC_ENABLE:
		case SQL_ATTR_CONCURRENCY:
		case SQL_ATTR_CURSOR_TYPE:
		case SQL_ATTR_KEYSET_SIZE:
		case SQL_ATTR_MAX_LENGTH:
		case SQL_ATTR_MAX_ROWS:
		case SQL_ATTR_NOSCAN:
		case SQL_ATTR_QUERY_TIMEOUT:
		case SQL_ATTR_RETRIEVE_DATA:
		case SQL_ATTR_ROW_BIND_TYPE:
		case SQL_ATTR_ROW_NUMBER:
		case SQL_ATTR_SIMULATE_CURSOR:
		case SQL_ATTR_USE_BOOKMARKS:
			rc = NeoGetStmtAttr(hstmt, fOption, pvParam, 0, NULL,false);
			break;
		/* ODBC3 attributes */
		case SQL_ATTR_APP_PARAM_DESC:
		case SQL_ATTR_APP_ROW_DESC:
		case SQL_ATTR_CURSOR_SCROLLABLE:
		case SQL_ATTR_CURSOR_SENSITIVITY:
		case SQL_ATTR_ENABLE_AUTO_IPD:
		case SQL_ATTR_FETCH_BOOKMARK_PTR:
		case SQL_ATTR_IMP_PARAM_DESC:
		case SQL_ATTR_IMP_ROW_DESC:
		case SQL_ATTR_METADATA_ID:
		case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
		case SQL_ATTR_PARAM_BIND_TYPE:
		case SQL_ATTR_PARAM_STATUS_PTR:
		case SQL_ATTR_PARAMS_PROCESSED_PTR:
		case SQL_ATTR_PARAMSET_SIZE:
		case SQL_ATTR_ROW_ARRAY_SIZE:
		case SQL_ATTR_ROW_BIND_OFFSET_PTR:
		case SQL_ATTR_ROW_OPERATION_PTR:
		case SQL_ATTR_ROW_STATUS_PTR:
		case SQL_ATTR_ROWS_FETCHED_PTR:
			PUSHSQLERR (pstmt, IDS_IM_001);
			RETURNCODE (pstmt, SQL_ERROR);
			LEAVE_STMT (pstmt, SQL_ERROR);
		default:
			rc = NeoGetStmtAttr(hstmt, fOption, pvParam, SQL_MAX_OPTION_STRING_LENGTH,NULL,false);
			break;
	}
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLParamOptions (
	SQLHSTMT hstmt,
    SQLULEN  crow,
    SQLULEN* pirow)
{
	STMT (pstmt, hstmt);
	SQLRETURN rc;

	ENTER_STMT (pstmt);
	if (crow == (UDWORD_P) 0UL)
	{
		PUSHSQLERR (pstmt, IDS_S1_107);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
	}
	if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
	{
		PUSHSQLERR (pstmt, IDS_S1_010);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
	}

	rc = NeoSetStmtAttr(hstmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)crow, 0,false);
	if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
		rc = NeoSetStmtAttr(hstmt, SQL_ATTR_PARAMS_PROCESSED_PTR, pirow, 0,false);

	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLSetConnectOption (
    SQLHDBC hdbc,
    SQLUSMALLINT fOption,
    SQLULEN vParam)
{
	CONN (pdbc, hdbc);
	SQLRETURN rc;

	ENTER_HDBC (pdbc);
	rc = fun_SetConnectOption (hdbc, fOption, vParam,false);
	RETURNCODE (pdbc, rc);
	LEAVE_HDBC (pdbc, rc);
}
SQLRETURN SQL_API SQLSetConnectOptionA (
    SQLHDBC hdbc,
    SQLUSMALLINT fOption,
    SQLULEN vParam)
{
	CONN (pdbc, hdbc);
	SQLRETURN rc;

	ENTER_HDBC (pdbc);
	rc = fun_SetConnectOption (hdbc, fOption, vParam,false);
	RETURNCODE (pdbc, rc);
	LEAVE_HDBC (pdbc, rc);
}
SQLRETURN SQL_API SQLSetConnectOptionW (
    SQLHDBC hdbc,
    SQLUSMALLINT fOption,
    SQLULEN vParam)
{
	CONN (pdbc, hdbc);
	SQLRETURN rc;

	ENTER_HDBC (pdbc);
	rc = fun_SetConnectOption (hdbc, fOption, vParam,true);
	RETURNCODE (pdbc, rc);
	LEAVE_HDBC (pdbc, rc);
}

SQLRETURN SQL_API SQLSetParam (
    SQLHSTMT hstmt,
    SQLUSMALLINT ipar,
    SQLSMALLINT fCType,
    SQLSMALLINT fSqlType,
    SQLUINTEGER cbColDef,
    SQLSMALLINT ibScale,
    SQLPOINTER rgbValue,
    SQLLEN *pcbValue)
{
	return SQLBindParameter (hstmt,
		ipar,
		(SWORD) SQL_PARAM_INPUT_OUTPUT,
		fCType,
		fSqlType,
		cbColDef,
		ibScale,
		rgbValue,
		SQL_SETPARAM_VALUE_MAX,
		pcbValue);
}

SQLRETURN SQL_API SQLSetStmtOption (
	SQLHSTMT hstmt,
    SQLUSMALLINT fOption,
    SQLULEN vParam)
{
	STMT (pstmt, hstmt);
	int sqlstat = 0;
	SQLRETURN rc;

	ENTER_STMT (pstmt);

	if (fOption == SQL_CONCURRENCY
		|| fOption == SQL_CURSOR_TYPE
		|| fOption == SQL_SIMULATE_CURSOR
		|| fOption == SQL_USE_BOOKMARKS)
    {
		if (pstmt->asyn_on != en_NullProc)
		{
			if (pstmt->prep_state)
			{
				sqlstat = IDS_S1_010; //IDS_S1_011
			}
		}
		else
		{
			switch (pstmt->state)
			{
				case en_stmt_prepared:
					sqlstat = IDS_S1_010; //IDS_S1_011
					break;
				case en_stmt_executed:
				case en_stmt_cursoropen:
				case en_stmt_fetched:
				case en_stmt_xfetched:
					sqlstat = IDS_24_000;
					break;

				case en_stmt_needdata:
				case en_stmt_mustput:
				case en_stmt_canput:
					if (pstmt->prep_state)
					{
						sqlstat = IDS_S1_010; //IDS_S1_011
					}
					break;
				default:
					break;
			}
		}
	}
	else
    {
		if (pstmt->asyn_on != en_NullProc)
		{
			if (!pstmt->prep_state)
			{
				sqlstat = IDS_S1_010;
			}
		}
		else
		{
			if (pstmt->state >= en_stmt_needdata)
			{
				sqlstat = IDS_S1_010;
			}
		}
	}
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		RETURNCODE (pstmt, SQL_ERROR);
		LEAVE_STMT (pstmt, SQL_ERROR);
	}
	switch (fOption)
	{
		/* ODBC integer attributes */   
		case SQL_ATTR_ASYNC_ENABLE:
			if (vParam != SQL_ASYNC_ENABLE_OFF && vParam != SQL_ASYNC_ENABLE_ON )
			{
				PUSHSQLERR (pstmt, IDS_S1_009);
				RETURNCODE (pstmt, SQL_ERROR);
				LEAVE_STMT (pstmt, SQL_ERROR);
			}
			break;
		case SQL_ATTR_CONCURRENCY:
		case SQL_ATTR_CURSOR_TYPE:
		case SQL_ATTR_KEYSET_SIZE:
		case SQL_ATTR_MAX_LENGTH:
		case SQL_ATTR_MAX_ROWS:
		case SQL_ATTR_NOSCAN:
		case SQL_ATTR_QUERY_TIMEOUT:
		case SQL_ATTR_RETRIEVE_DATA:
		case SQL_ATTR_ROW_BIND_TYPE:
		case SQL_ATTR_ROW_NUMBER:
		case SQL_ATTR_SIMULATE_CURSOR:
		case SQL_ATTR_USE_BOOKMARKS:
			break;	  
		/* ODBC3 attributes */   
		case SQL_ATTR_APP_PARAM_DESC:
		case SQL_ATTR_APP_ROW_DESC:
		case SQL_ATTR_CURSOR_SCROLLABLE:
		case SQL_ATTR_CURSOR_SENSITIVITY:
		case SQL_ATTR_ENABLE_AUTO_IPD:
		case SQL_ATTR_FETCH_BOOKMARK_PTR:
		case SQL_ATTR_IMP_PARAM_DESC:
		case SQL_ATTR_IMP_ROW_DESC:
		case SQL_ATTR_METADATA_ID:
		case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
		case SQL_ATTR_PARAM_BIND_TYPE:
		case SQL_ATTR_PARAM_STATUS_PTR:
		case SQL_ATTR_PARAMS_PROCESSED_PTR:
		case SQL_ATTR_PARAMSET_SIZE:
		case SQL_ATTR_ROW_ARRAY_SIZE:
		case SQL_ATTR_ROW_BIND_OFFSET_PTR:
		case SQL_ATTR_ROW_OPERATION_PTR:
		case SQL_ATTR_ROW_STATUS_PTR:
		case SQL_ATTR_ROWS_FETCHED_PTR:
			PUSHSQLERR (pstmt, IDS_IM_001);
			RETURNCODE (pstmt, SQL_ERROR);
			LEAVE_STMT (pstmt, SQL_ERROR);

		default:
			rc = NeoSetStmtAttr(hstmt, fOption, (SQLPOINTER)vParam, SQL_NTS,false);
			RETURNCODE (pstmt, rc);
			LEAVE_STMT (pstmt, rc);
	}
	rc = NeoSetStmtAttr(hstmt, fOption, (SQLPOINTER)vParam, 0,false);
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLSetScrollOptions (
	SQLHSTMT hstmt,
    SQLUSMALLINT fConcurrency,
    SQLINTEGER crowKeyset,
    SQLUSMALLINT crowRowset)
{
	STMT (pstmt, hstmt);
	int sqlstat = 0;
	SQLRETURN rc = SQL_ERROR;

	ENTER_STMT (pstmt);
	for (;;)
	{
		if (crowRowset == (UWORD) 0)
		{
			sqlstat = IDS_S1_107;
			break;
		}
		if (crowKeyset > (SDWORD_P) 0L && crowKeyset < (SDWORD_P) crowRowset)
		{
			sqlstat = IDS_S1_107;
			break;
		}
		if (crowKeyset < 1)
		{
			if (crowKeyset != SQL_SCROLL_FORWARD_ONLY
				&& crowKeyset != SQL_SCROLL_STATIC
				&& crowKeyset != SQL_SCROLL_KEYSET_DRIVEN
				&& crowKeyset != SQL_SCROLL_DYNAMIC)
			{
				sqlstat = IDS_S1_107;
				break;
			}
		}
		if (fConcurrency != SQL_CONCUR_READ_ONLY
			&& fConcurrency != SQL_CONCUR_LOCK
			&& fConcurrency != SQL_CONCUR_ROWVER
			&& fConcurrency != SQL_CONCUR_VALUES)
		{
			sqlstat = IDS_S1_108;
			break;
		}
		SQLINTEGER InfoValue, InfoType, Value;
		switch (crowKeyset)
		{
			case SQL_SCROLL_FORWARD_ONLY:
			  InfoType = SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2;
			  Value = SQL_CURSOR_FORWARD_ONLY;
			  break;
			case SQL_SCROLL_STATIC:
			  InfoType = SQL_STATIC_CURSOR_ATTRIBUTES2;
			  Value = SQL_CURSOR_STATIC;
			  break;
			case SQL_SCROLL_DYNAMIC:
			  InfoType = SQL_DYNAMIC_CURSOR_ATTRIBUTES2;
			  Value = SQL_CURSOR_DYNAMIC;
			  break;
			case SQL_SCROLL_KEYSET_DRIVEN:
			default:
			  InfoType = SQL_KEYSET_CURSOR_ATTRIBUTES2;
			  Value = SQL_CURSOR_KEYSET_DRIVEN;
			  break;
		}
		rc = SQLGetInfo ((SQLHDBC)pstmt->getConnectHandle(), InfoType, &InfoValue, 0, NULL);
		if (rc != SQL_SUCCESS)
		{
			RETURNCODE (pstmt, rc);
			LEAVE_STMT (pstmt, rc);
		}
		switch (fConcurrency)
		{
			case SQL_CONCUR_READ_ONLY:
				if (!(InfoValue & SQL_CA2_READ_ONLY_CONCURRENCY))
				{
					PUSHSQLERR (pstmt, IDS_S1_C00);
					RETURNCODE (pstmt, SQL_ERROR);
					LEAVE_STMT (pstmt, SQL_ERROR);
				}
				break;
			case SQL_CONCUR_LOCK:
				if (!(InfoValue & SQL_CA2_LOCK_CONCURRENCY))
				{
					PUSHSQLERR (pstmt, IDS_S1_C00);
					RETURNCODE (pstmt, SQL_ERROR);
					LEAVE_STMT (pstmt, SQL_ERROR);
				}
				break;
			case SQL_CONCUR_ROWVER:
				if (!(InfoValue & SQL_CA2_OPT_ROWVER_CONCURRENCY))
				{
					PUSHSQLERR (pstmt, IDS_S1_C00);
					RETURNCODE (pstmt, SQL_ERROR);
					LEAVE_STMT (pstmt, SQL_ERROR);
				}
				break;
			case SQL_CONCUR_VALUES:
				if (!(InfoValue & SQL_CA2_OPT_VALUES_CONCURRENCY))
				{
					PUSHSQLERR (pstmt, IDS_S1_C00);
					RETURNCODE (pstmt, SQL_ERROR);
					LEAVE_STMT (pstmt, SQL_ERROR);
				}
				break;
		}
		rc = NeoSetStmtAttr(hstmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)Value, 0,false);
		if (rc != SQL_SUCCESS)
		{
			RETURNCODE (pstmt, rc);
			LEAVE_STMT (pstmt, rc);
		}
		rc = NeoSetStmtAttr(hstmt, SQL_ATTR_CONCURRENCY, (SQLPOINTER)fConcurrency, 0,false);
		if (rc != SQL_SUCCESS)
		{
			RETURNCODE (pstmt, rc);
			LEAVE_STMT (pstmt, rc);
		}
		if (crowKeyset > 0)
		{
			rc = NeoSetStmtAttr(hstmt, SQL_ATTR_KEYSET_SIZE, (SQLPOINTER)crowKeyset, 0,false);
			if (rc != SQL_SUCCESS)
			{
				RETURNCODE (pstmt, rc);
				LEAVE_STMT (pstmt, rc);
			}
		}
		rc = NeoSetStmtAttr(hstmt, SQL_ROWSET_SIZE, (SQLPOINTER)crowRowset, 0,false);
		if (rc != SQL_SUCCESS)
		{
			RETURNCODE (pstmt, rc);
			LEAVE_STMT (pstmt, rc);
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
	RETURNCODE (pstmt, rc);
	LEAVE_STMT (pstmt, rc);
}

SQLRETURN SQL_API SQLTransact (
    SQLHENV henv,
    SQLHDBC hdbc,
    SQLUSMALLINT fType)
{
	CHandle* phandle;

	GENV (genv, henv);
	CONN (pdbc, hdbc);
	SQLRETURN rc = SQL_SUCCESS;

	ODBC_LOCK ();
	if (IS_VALID_HDBC (pdbc))
    {
		CLEAR_ERRORS (pdbc);
		phandle = pdbc;
    }
	else if (IS_VALID_HENV (genv))
    {
		CLEAR_ERRORS (genv);
		phandle = genv;
    }
	else
    {
		ODBC_UNLOCK ();
		return SQL_INVALID_HANDLE;
    }
	/* check argument */
	if (fType != SQL_COMMIT && fType != SQL_ROLLBACK)
    {
		SQLHENV handle = IS_VALID_HDBC (pdbc) ? ((SQLHENV) pdbc) : (SQLHENV) genv;
		PUSHSQLERR (phandle, IDS_S1_012);
		RETURNCODE (phandle, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	if (hdbc != SQL_NULL_HDBC)
    {
		rc = fun_transact (pdbc, fType);
    }
	else
    {
#ifdef VERSION3
		std::list<CHandlePtr>::iterator	i;
		CConnect* pConnect;

		for (i = genv->m_ConnectCollect.begin(); i != genv->m_ConnectCollect.end() ; ++i)
		{
			if( (pConnect = (CConnect*)(*i)) != NULL)
				rc |= fun_transact (pConnect, fType);
		}
#else
		RWTPtrSlistIterator<CHandle> i(genv->m_ConnectCollect);
		CConnect* pConnect;

		while ((pConnect = (CConnect*)i()) != NULL)
		{
			rc |= fun_transact (pConnect, fType);
		}
#endif
	}
	if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    {
		ODBC_UNLOCK ();
		RETURNCODE (phandle, SQL_ERROR);
		/* fail on one of the connection */
		return SQL_ERROR;
    }
	RETURNCODE (phandle, rc);
	ODBC_UNLOCK ();
	return rc;
}
