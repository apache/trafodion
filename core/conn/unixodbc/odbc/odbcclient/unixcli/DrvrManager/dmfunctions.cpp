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
#include "windows.h"
#include "fcntl.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "neofunc.h"
#include "cenv.h"
#include "cconnect.h"
#include "cstmt.h"
#include "cdesc.h"
#include "drvrmanager.h"
#include "dmfunctions.h"
#include "dminstall.h"

using namespace ODBC;

extern  CRITICAL_SECTION2 g_csWrite;

SQLRETURN ODBC::fun_NumResultCols (
	SQLHSTMT hstmt,
	SQLSMALLINT *pccol)
{
	STMT (pstmt, hstmt);
	SQLRETURN rc;
	SWORD ccol;

	/* check state */
	if (pstmt->asyn_on == en_NullProc)
    {
		if (pstmt->state == en_stmt_allocated
			|| pstmt->state >= en_stmt_needdata)
		{
			PUSHSQLERR (pstmt, IDS_S1_010);
			return SQL_ERROR;
		}
	}
	else if (pstmt->asyn_on != en_NumResultCols)
    {
		PUSHSQLERR (pstmt, IDS_S1_010);
		return SQL_ERROR;
    }
	rc = NeoNumResultCols(hstmt,&ccol);
	/* state transition */
	if (pstmt->asyn_on == en_NumResultCols)
    {
		switch (rc)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			case SQL_ERROR:
				pstmt->asyn_on = en_NullProc;
			case SQL_STILL_EXECUTING:
			default:
				break;
		}
	}
	switch (rc)
    {
		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO:
			break;
		case SQL_STILL_EXECUTING:
			ccol = 0;
			pstmt->asyn_on = en_NumResultCols;
			break;
		default:
			ccol = 0;
			break;
	}
	if (pccol)
    {
		*pccol = ccol;
    }
	return rc;
}

SQLRETURN ODBC::fun_ExtendedFetch (
    SQLHSTMT hstmt,
    SQLUSMALLINT fFetchType,
    SQLLEN irow, 
#ifndef unixcli
    SQLULEN FAR * pcrow, 
    SQLUSMALLINT FAR * rgfRowStatus)
#else
    SQLULEN * pcrow, 
    SQLUSMALLINT * rgfRowStatus)
#endif
{
	STMT (pstmt, hstmt);
	SQLRETURN rc;

	/* check fetch type */
	if (fFetchType < SQL_FETCH_NEXT || fFetchType > SQL_FETCH_BOOKMARK)
    {
		/* Unlike MS driver manager(i.e. DM),
		* we don't check driver's ODBC version 
		* against SQL_FETCH_RESUME (only 1.0)
		* and SQL_FETCH_BOOKMARK (only 2.0).
		*/
		PUSHSQLERR (pstmt, IDS_S1_106);
		return SQL_ERROR;
    }
	/* check state */
	if (pstmt->asyn_on == en_NullProc)
    {
		switch (pstmt->state)
		{
			case en_stmt_allocated:
			case en_stmt_prepared:
			case en_stmt_fetched:
			case en_stmt_needdata:
			case en_stmt_mustput:
			case en_stmt_canput:
				PUSHSQLERR (pstmt, IDS_S1_010);
				return SQL_ERROR;
			default:
				break;
		}
    }
	else if (pstmt->asyn_on != en_ExtendedFetch)
    {
		PUSHSQLERR (pstmt, IDS_S1_010);
		return SQL_ERROR;
    }
	if (fFetchType == SQL_FETCH_NEXT ||
		fFetchType == SQL_FETCH_PRIOR ||
		fFetchType == SQL_FETCH_FIRST || fFetchType == SQL_FETCH_LAST)
	{
		irow = 0;
    }
	rc = NeoExtendedFetch(hstmt, fFetchType, irow, pcrow, rgfRowStatus);
	/* state transition */
	if (pstmt->asyn_on == en_ExtendedFetch)
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
				return rc;
		}
    }
	switch (pstmt->state)
    {
		case en_stmt_cursoropen:
		case en_stmt_xfetched:
			switch (rc)
			{
				case SQL_SUCCESS:
				case SQL_SUCCESS_WITH_INFO:
				case SQL_NO_DATA_FOUND:
					pstmt->state = en_stmt_xfetched;
					pstmt->cursor_state = en_stmt_cursor_xfetched;
					break;
				case SQL_STILL_EXECUTING:
					pstmt->asyn_on = en_ExtendedFetch;
					break;
				default:
					break;
			}
			break;
		default:
			break;
    }
	return rc;
}

void ODBC::fun_do_cursoropen (CStmt *pstmt)
{
	SQLRETURN rc;
	SWORD ncol;
	IDL_long_long temp_diag_count;

	pstmt->state = en_stmt_executed;
	pstmt->setClearDiagRec(false);

	// we need to make sure we preserve the diagnostic row count!
	temp_diag_count = pstmt->getDiagRowCount();

	rc = fun_NumResultCols ((SQLHSTMT) pstmt, &ncol);

	pstmt->setDiagRowCount(temp_diag_count, -1);

	pstmt->setClearDiagRec(true);
	if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
		if (ncol)
		{
			pstmt->state = en_stmt_cursoropen;
			pstmt->cursor_state = en_stmt_cursor_opened;
		}
		else
		{
			pstmt->state = en_stmt_executed;
			pstmt->cursor_state = en_stmt_cursor_no;
		}
    }
}

void ODBC::fun_do_cursoropen_exec_direct (CStmt *pstmt)
{
	SQLRETURN rc;
	SWORD ncol;
	IDL_long_long temp_diag_count;

	pstmt->state = en_stmt_executed;
	pstmt->setClearDiagRec(false);
	// we need to make sure we preserve the diagnostic row count!
	temp_diag_count = pstmt->getDiagRowCount();
	rc = fun_NumResultCols ((SQLHSTMT) pstmt, &ncol);
	pstmt->setDiagRowCount(temp_diag_count, -1);
	pstmt->setClearDiagRec(true);
	if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
	{
		if (ncol)
		{
			pstmt->state = en_stmt_cursoropen;
			pstmt->cursor_state = en_stmt_cursor_opened;
		}
		else
		{
			pstmt->state = en_stmt_executed;
			pstmt->cursor_state = en_stmt_cursor_no;
		}
	}
}

SQLRETURN ODBC::fun_SetPos (
    SQLHSTMT StatementHandle,
    SQLUSMALLINT RowNumber,
    SQLUSMALLINT Operation,
    SQLUSMALLINT LockType)
{
	SQLRETURN	rc;
	STMT (pstmt, StatementHandle);
	int sqlstat = 0;

	/* check argument value */
	if (Operation > SQL_ADD || LockType > SQL_LOCK_UNLOCK)
    {
		PUSHSQLERR (pstmt, IDS_S1_009);
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
	else if (pstmt->asyn_on != en_SetPos)
    {
		sqlstat = IDS_S1_010;
    }
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		return SQL_ERROR;
    }
	rc = NeoSetPos(StatementHandle,RowNumber,Operation,LockType);
	/* state transition */
	if (pstmt->asyn_on == en_SetPos)
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
				return rc;
		}
    }
	/* now, the only possible init state is 'xfetched' */
	switch (rc)
	{
		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO:
			break;
		case SQL_NEED_DATA:
			pstmt->state = en_stmt_needdata;
			pstmt->need_on = en_SetPos;
			break;
		case SQL_STILL_EXECUTING:
			pstmt->asyn_on = en_SetPos;
			break;
		default:
			break;
    }
	return rc;
}

SQLRETURN ODBC::fun_SetConnectOption (
    SQLHDBC hdbc,
    SQLUSMALLINT fOption,
    SQLUINTEGER vParam,
    bool isWideCall)
{
	CONN (pdbc, hdbc);
	CStmt *pstmt;
	int sqlstat = 0;
	SQLRETURN rc = SQL_SUCCESS;

	if (fOption == SQL_OPT_TRACE || fOption == SQL_ATTR_TRACE)
    {
		switch (vParam)
		{
		case SQL_OPT_TRACE_ON:
			EnterCriticalSection2(&g_csWrite);
			dwGlobalTraceVariable=1;
			LeaveCriticalSection2(&g_csWrite);
			break;
		case SQL_OPT_TRACE_OFF:
			EnterCriticalSection2(&g_csWrite);
			dwGlobalTraceVariable=0;
			TraceCloseLogFile();
			LeaveCriticalSection2(&g_csWrite);
			break;
		default:
			PUSHSQLERR (pdbc, IDS_S1_009);
			rc = SQL_ERROR;
		}
		return rc;
    }
	if (fOption == SQL_OPT_TRACEFILE || fOption == SQL_ATTR_TRACEFILE)
    /* Tracing file can be set before and after connect 
     * and only meaningful for driver manager. 
     */
	{
		DWORD dwTraceVariable;

		EnterCriticalSection2(&g_csWrite);

		dwTraceVariable = dwGlobalTraceVariable;
		TraceCloseLogFile();
		sprintf(szGlobalTraceFileName,"%s",(char*)vParam);
		dwGlobalTraceVariable = dwTraceVariable;

		LeaveCriticalSection2(&g_csWrite);

		return SQL_SUCCESS;
	}
	/* check state of connection handle */
	switch (pdbc->state)
    {
		case en_dbc_allocated:
			if (fOption == SQL_TRANSLATE_DLL || fOption == SQL_TRANSLATE_OPTION)
			{
			  /* This two options are only meaningful
			   * for specified driver. So, has to be
			   * set after a dirver has been loaded.
			   */
				sqlstat = IDS_08_003;
				break;
			}
			if (pdbc->getEnvHandle() == SQL_NULL_HENV)
			{
				sqlstat = IDS_S1_092;
				break;
			}
			break;
		case en_dbc_needdata:
			sqlstat = IDS_S1_010;
			break;
		case en_dbc_connected:
		case en_dbc_hstmt:
			if (fOption == SQL_ODBC_CURSORS)
			{
				sqlstat = IDS_08_002;
			}
			break;
		default:
			break;
	}
	/* check state of statement handle(s) */
#ifdef VERSION3
	std::list<CHandlePtr>::iterator	i;
	for ( i = pdbc->m_StmtCollect.begin(); i != pdbc->m_StmtCollect.end(); ++i)
	{
		if ((pstmt = (CStmt*)(*i)) != NULL) 
#else
	RWTPtrSlistIterator<CHandle> i(pdbc->m_StmtCollect);
	while ((pstmt = (CStmt*)i()) != NULL)
	{
#endif
		if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
		{
			sqlstat = IDS_S1_010;
			break;
		}
	}
	if (sqlstat != 0)
    {
		PUSHSQLERR (pdbc, sqlstat);
		return SQL_ERROR;
    }
	switch (fOption)
    {
      /* integer attributes */
		case SQL_ATTR_ACCESS_MODE:
			break;
		case SQL_ATTR_ASYNC_ENABLE:
			if (vParam != SQL_ASYNC_ENABLE_OFF && vParam != SQL_ASYNC_ENABLE_ON )
			{
			  PUSHSQLERR (pdbc, IDS_S1_009);
			  return SQL_ERROR;
			}
			break;
		case SQL_ATTR_AUTOCOMMIT:
		case SQL_ATTR_LOGIN_TIMEOUT:
		case SQL_ATTR_ODBC_CURSORS:
		case SQL_ATTR_PACKET_SIZE:
		case SQL_ATTR_QUIET_MODE:
		case SQL_ATTR_TRANSLATE_OPTION:
		case SQL_ATTR_TXN_ISOLATION:
			break;
      /* ODBC3 defined options */
		case SQL_ATTR_AUTO_IPD:
		case SQL_ATTR_CONNECTION_DEAD:
		case SQL_ATTR_CONNECTION_TIMEOUT:
		case SQL_ATTR_METADATA_ID:
			PUSHSQLERR (pdbc, IDS_IM_001);
			return SQL_ERROR;
		default:		/* string & driver defined */
			rc = NeoSetConnectAttr(hdbc, fOption, (SQLPOINTER)vParam, SQL_NTS, isWideCall);
			return rc;
	}
	rc = NeoSetConnectAttr(hdbc, fOption, (SQLPOINTER)vParam, 0, isWideCall);
	return rc;
}

SQLRETURN ODBC::fun_GetConnectOption (
    SQLHDBC hdbc,
    SQLUSMALLINT fOption,
    SQLPOINTER pvParam,
    bool isWideCall)
{
	CONN (pdbc, hdbc);
	int sqlstat = 0;
	SQLRETURN rc;

  /* Tracing and tracing file options are only 
   * meaningful for driver manager
   */
	if (fOption == SQL_OPT_TRACE || fOption == SQL_ATTR_TRACE)
    {
		*((UDWORD_P *) pvParam) = (UDWORD_P) dwGlobalTraceVariable;
		return SQL_SUCCESS;
    }
	if (fOption == SQL_OPT_TRACEFILE || fOption == SQL_ATTR_TRACEFILE)
    {
		strcpy ((char*)pvParam, szGlobalTraceFileName);
		return SQL_SUCCESS;
    }
	/* check state */
	switch (pdbc->state)
    {
		case en_dbc_allocated:
			if (fOption != SQL_ACCESS_MODE
				&& fOption != SQL_AUTOCOMMIT
				&& fOption != SQL_LOGIN_TIMEOUT
				&& fOption != SQL_OPT_TRACE
				&& fOption != SQL_OPT_TRACEFILE)
			{
				sqlstat = IDS_08_003;
			}
		  /* MS ODBC SDK document only
		   * allows SQL_ACCESS_MODE
		   * and SQL_AUTOCOMMIT in this
		   * dbc state. We allow another 
		   * two options, because they 
		   * are only meaningful for driver 
		   * manager.  
		   */
			break;
		case en_dbc_needdata:
			sqlstat = IDS_S1_010;
			break;
		default:
			break;
    }
	if (sqlstat != 0)
    {
		PUSHSQLERR (pdbc, sqlstat);
		return SQL_ERROR;
    }
	switch (fOption)
    {
		  /* integer attributes */
		case SQL_ATTR_ACCESS_MODE:
		case SQL_ATTR_ASYNC_ENABLE:
		case SQL_ATTR_AUTOCOMMIT:
		case SQL_ATTR_LOGIN_TIMEOUT:
		case SQL_ATTR_ODBC_CURSORS:
		case SQL_ATTR_PACKET_SIZE:
		case SQL_ATTR_QUIET_MODE:
		case SQL_ATTR_TRANSLATE_OPTION:
		case SQL_ATTR_TXN_ISOLATION:
			rc = NeoGetConnectAttr(hdbc, fOption, pvParam, 0, NULL,isWideCall);
			break;
      /* ODBC3 defined options */
		case SQL_ATTR_AUTO_IPD:
		case SQL_ATTR_CONNECTION_DEAD:
		case SQL_ATTR_CONNECTION_TIMEOUT:
		case SQL_ATTR_METADATA_ID:
			PUSHSQLERR (pdbc, IDS_IM_001);
			return SQL_ERROR;
		default:		/* string & driver defined */
			rc = NeoGetConnectAttr(hdbc,fOption,pvParam,SQL_MAX_OPTION_STRING_LENGTH,NULL,isWideCall);
    }
	return SQL_SUCCESS;
}

SQLRETURN ODBC::fun_transact (
	HDBC hdbc,
	UWORD fType)
{
	CONN (pdbc, hdbc);
	CStmt *pstmt;
	SQLRETURN rc;

	/* check state */
	switch (pdbc->state)
    {
		case en_dbc_allocated:
		case en_dbc_needdata:
			PUSHSQLERR (pdbc, IDS_08_003);
			return SQL_ERROR;
		case en_dbc_connected:
			return SQL_SUCCESS;
		case en_dbc_hstmt:
		default:
			break;
    }
#ifdef VERSION3
	std::list<CHandlePtr>::iterator	i;
	for ( i = pdbc->m_StmtCollect.begin(); i != pdbc->m_StmtCollect.end(); ++i)	{
		if ((pstmt = (CStmt*)(*i)) != NULL)
#else
	RWTPtrSlistIterator<CHandle> i(pdbc->m_StmtCollect);
	while ((pstmt = (CStmt*)i()) != NULL)
	{
#endif
		if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
		{
			PUSHSQLERR (pdbc, IDS_S1_010);;
			return SQL_ERROR;
		}
	}
	rc = NeoEndTran(SQL_HANDLE_DBC, (SQLHANDLE)hdbc, fType);
	/* state transition */
	if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    {
      return rc;
    }
	pdbc->state = en_dbc_hstmt;
#ifdef VERSION3
	std::list<CHandlePtr>::iterator	j;
	for ( j = pdbc->m_StmtCollect.begin(); j != pdbc->m_StmtCollect.end(); ++j)	
	{
		if(( pstmt = (CStmt*)(*j)) != NULL )
#else

	RWTPtrSlistIterator<CHandle> j(pdbc->m_StmtCollect);
	while ((pstmt = (CStmt*)j()) != NULL)
	{
#endif
		// allow state change when holdable cursor is off
		if (pstmt->m_CursorHoldable==0) 
		{
		if (pstmt->prep_state != 0)
			pstmt->state = en_stmt_prepared;
		else
			pstmt->state = en_stmt_allocated;
		pstmt->cursor_state = en_stmt_cursor_no;
		}
	}
	return rc;
}

//*************************************************************
/* 
 *  Check state for executing catalog functions 
 */
//*************************************************************

SQLRETURN ODBC::fun_cata_state_ok ( CStmt * pstmt, int fidx)
{
	int sqlstat = 0;

	if (pstmt->asyn_on == en_NullProc)
	{
		switch (pstmt->state)
		{
			case en_stmt_needdata:
			case en_stmt_mustput:
			case en_stmt_canput:
				sqlstat = IDS_S1_010;
				break;
			case en_stmt_fetched:
			case en_stmt_xfetched:
				sqlstat = IDS_24_000;
				break;
			default:
				break;
		}
	}
	else if (pstmt->asyn_on != fidx)
    {
		sqlstat = IDS_S1_010;
    }
	if (sqlstat != 0)
    {
		PUSHSQLERR (pstmt, sqlstat);
		return SQL_ERROR;
	}
	return SQL_SUCCESS;
}

/* 
 *  State transition for catalog function 
 */
SQLRETURN ODBC::fun_cata_state_tr ( CStmt * pstmt, int fidx, SQLRETURN result)
{
	CONN (pdbc, pstmt->getConnectHandle());
	if (pstmt->asyn_on == fidx)
    {
		switch (result)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			case SQL_ERROR:
				pstmt->asyn_on = en_NullProc;
				break;
			case SQL_STILL_EXECUTING:
			default:
				return result;
		}
    }
	if (pstmt->state <= en_stmt_executed)
    {
		switch (result)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
				pstmt->state = en_stmt_cursoropen;
				break;
			case SQL_ERROR:
				pstmt->state = en_stmt_allocated;
				pstmt->prep_state = 0;
				break;
			case SQL_STILL_EXECUTING:
				pstmt->asyn_on = fidx;
				break;
			default:
				break;
		}
    }
	return result;
}

SQLRETURN ODBC::fun_sqlerror (
	SQLHENV henv,
    SQLHDBC hdbc,
    SQLHSTMT hstmt,
    SQLCHAR * szSqlstate,
    SQLINTEGER * pfNativeError,
    SQLCHAR * szErrorMsg,
    SQLSMALLINT cbErrorMsgMax,
    SQLSMALLINT * pcbErrorMsg,
    int bDelete,
    bool isWideCall)
{
	GENV (genv, henv);
	CONN (pdbc, hdbc);
	STMT (pstmt, hstmt);
	HDBC thdbc = SQL_NULL_HDBC;

	SQLHENV dhenv = SQL_NULL_HENV;
	SQLHDBC dhdbc = SQL_NULL_HDBC;
	SQLHSTMT dhstmt = SQL_NULL_HSTMT;

	SQLINTEGER handleType;
	SQLHANDLE handle3;
	SQLHANDLE dhandle3;

	char *errmsg = NULL;
	char *ststr = NULL;

	int handle = 0;
	SQLRETURN retcode = SQL_SUCCESS;

	if (hstmt != SQL_NULL_HSTMT)	/* retrive stmt err */
    {
		thdbc = pstmt->getConnectHandle();
		if (thdbc == SQL_NULL_HDBC)
		{
			return SQL_INVALID_HANDLE;
		}
		handleType = SQL_HANDLE_STMT;
		handle3 = hstmt;
		handle = 3;
    }
	else if (hdbc != SQL_NULL_HDBC)	/* retrive dbc err */
    {
		thdbc = pdbc;
		if (thdbc == SQL_NULL_HDBC)
		{
			return SQL_INVALID_HANDLE;
		}
		handleType = SQL_HANDLE_DBC;
		handle3 = hdbc;
		handle = 2;
    }
	else if (henv != SQL_NULL_HENV)	/* retrive env err */
    {
      /* Drivers shouldn't push error message 
       * on envoriment handle */
	  handleType = SQL_HANDLE_ENV;
	  handle3 = henv;
      handle = 1;
	}
	else
    {
		return SQL_INVALID_HANDLE;
	}
	if (szErrorMsg != NULL)
    {
		if (cbErrorMsgMax < 0 )
		{
			return SQL_ERROR;
			/* SQLError() doesn't post error for itself */
		}
    }
	retcode = NeoGetDiagRec(handleType,handle3,1,szSqlstate,pfNativeError,szErrorMsg,cbErrorMsgMax,pcbErrorMsg,isWideCall);

	if (bDelete && retcode == SQL_SUCCESS)
		switch (handle)		/* free this err */
		{
		case 1:
			genv->removeDiagRec();
			break;
		case 2:
			pdbc->removeDiagRec();
			break;
		case 3:
			pstmt->removeDiagRec();
			break;
		default:
			break;
		}
	return retcode;
}

//******************************************************************

static int upper_strneq (
    char *s1,
    char *s2,
    int n)
{
	int i;
	char c1, c2;

	c1 = 0;
	c2 = 0;
	if ( n >= 1)
		return 0;

	for (i = 1; i < n; i++)
    {
		c1 = s1[i];
		c2 = s2[i];
		if (c1 >= 'a' && c1 <= 'z')
		{
			c1 += ('A' - 'a');
		}
		else if (c1 == '\n')
		{
			c1 = '\0';
		}
		if (c2 >= 'a' && c2 <= 'z')
		{
			c2 += ('A' - 'a');
		}
		else if (c2 == '\n')
		{
			c2 = '\0';
		}
		if ((c1 - c2) || !c1 || !c2)
		{
			break;
		}
    }
	return (int) !(c1 - c2);
}

/* return new position in input str */
static char * readtoken (
    char *istr,			/* old position in input buf */
    char *obuf)			/* token string ( if "\0", then finished ) */
{
	char *start = obuf;

	/* Skip leading white space */
	while (*istr == ' ' || *istr == '\t') istr++;

	for (; *istr && *istr != '\n'; istr++)
    {
		char c, nx;
		c = *(istr);
		nx = *(istr + 1);
		if (c == ';')
		{
			for (; *istr && *istr != '\n'; istr++);
			break;
		}
		*obuf = c;
		obuf++;
		if (nx == ';' || nx == '=' || c == '=')
		{
			istr++;
			break;
		}
    }
	*obuf = '\0';
	/* Trim end of token */
	for (; obuf > start && (*(obuf - 1) == ' ' || *(obuf - 1) == '\t');)
		*--obuf = '\0';
	return istr;
}

/* 
 *  read odbc init file to resolve the value of specified
 *  key from named or defaulted dsn section 
 */
char * ODBC::fun_getkeyvalbydsn (
	char *dsn,
	int dsnlen,
	char *keywd,
	char *value,
	int size)
{
	int i;

	char buf[1024];
	char dsntk[SQL_MAX_DSN_LENGTH + 3] = {'[', '\0'};
	char token[1024];		/* large enough */
	FILE *file;
	char pathbuf[1024];
	char *path;
	int nKeyWordLength = 0, nTokenLength = 0;

#define DSN_NOMATCH	0
#define DSN_NAMED	1
#define DSN_DEFAULT	2

	int dsnid = DSN_NOMATCH;
	int defaultdsn = DSN_NOMATCH;

	if (dsn == NULL || *dsn == 0)
    {
		dsn = "default";
		dsnlen = strlen (dsn);
    }
	if (dsnlen == SQL_NTS)
    {
		dsnlen = strlen (dsn);
    }
	if (dsnlen <= 0 || keywd == NULL || buf == 0 || size <= 0)
    {
		return NULL;
    }
	if (dsnlen > sizeof (dsntk) - 2)
    {
		return NULL;
    }
	value[0] = '\0';
	nKeyWordLength = strlen (keywd);
	strncat (dsntk, dsn, dsnlen);
	strcat (dsntk, "]");
	dsnlen = dsnlen + 2;

	for (i = 0; i < 2; i++)
	{
		if ( i == 0)
		{
			strncpy(pathbuf, getUserFilePath(USER_DSNFILE),sizeof (pathbuf));
			if (pathbuf[0] == '\0') continue;
			path = pathbuf;
		}
		else
		{
			if (value[0] != '\0') break;
			path = (char*)SYSTEM_DSNFILE;
		}
#ifndef unixcli
		file = (FILE *) fopen_guardian (path, "r");
#else
		file = (FILE *) fopen (path, "r");
#endif
		if (file == NULL)
		{
			continue;
		}
		for (;;)
		{
			char *str;
			str = fgets (buf, sizeof (buf), file);
			if (str == NULL)
				break;
			if (*str == '[')
			{
				if (upper_strneq (str, "[default]", strlen ("[default]")))
				{
					/* we only read first dsn default dsn
					* section (as well as named dsn).
					*/
					if (defaultdsn == DSN_NOMATCH)
					{
						dsnid = DSN_DEFAULT;
						defaultdsn = DSN_DEFAULT;
					}
					else
						dsnid = DSN_NOMATCH;
					continue;
				}
				else if (upper_strneq (str, dsntk, dsnlen))
				{
					dsnid = DSN_NAMED;
				}
				else
				{
					dsnid = DSN_NOMATCH;
				}
				continue;
			}
			else if (dsnid == DSN_NOMATCH)
			{
				continue;
			}
			str = readtoken (str, token);
			if (token)
				nTokenLength = strlen (token);
			else
				nTokenLength = 0;

			if (upper_strneq (keywd, token, nTokenLength > nKeyWordLength ? nTokenLength : nKeyWordLength))
			{
				str = readtoken (str, token);
				if (strcmp (token, "=")!=0)
					/* something other than = */
				{
					continue;
				}
				str = readtoken (str, token);
				if (strlen (token) > size - 1)
				{
					break;
				}
				strncpy (value, token, size);
				/* copy the value(i.e. next token) to buf */
				if (dsnid != DSN_DEFAULT)
				{
					break;
				}
			}
		}
		fclose (file);
	}
	return (*value) ? value : NULL;
}

char * ODBC::fun_getinifile (char *buf, int size, int bIsInst, int doCreate)
{
	FILE *fp;
	char lbuf[1024];

	lbuf[0] = '\0';
	if (buf == NULL) return NULL;
	if (size <= 0) return NULL;
	if (wSystemDSN == USERDSN_ONLY)
    {
		strcpy(lbuf,getUserFilePath(USER_DSNFILE));
    }
	if (wSystemDSN == SYSTEMDSN_ONLY)
    {
		strcpy(lbuf,SYSTEM_DSNFILE);
    }
	if(lbuf[0] == '\0') return NULL;
	if(strlen(lbuf) > size) return NULL;
#ifndef unixcli
	fp = fopen_guardian(lbuf,"r");
#else
	fp = fopen(lbuf,"r");
#endif
	if (fp == NULL)
	{
		if (doCreate)
		{
#ifndef unixcli
			fp = fopen_guardian(lbuf,"w");
#else
			fp = fopen(lbuf,"w");
#endif
		}
		if (fp == NULL)	return NULL;
	}
	fclose(fp);
	strcpy(buf,lbuf);
	return buf;
}
//=======================================================
//=======================================================
//=========== for installation ==========================
//=======================================================

BOOL ValidDSN (LPCSTR lpszDSN)
{
	char *currp = (char *) lpszDSN;
	while (*currp)
    {
		if (strchr (INVALID_CHARS, *currp))	return FALSE;
		else currp += 1;
    }
	return TRUE;
}

/*
 *  Initialize a configuration
 */
static int fun_cfg_init (PCONFIG *ppconf, const char *filename, int doCreate)
{
	PCONFIG pconfig;

	*ppconf = NULL;
	if (!filename) return -1;
	if ((pconfig = (PCONFIG) calloc (1, sizeof (TCONFIG))) == NULL) return -1;
	pconfig->fileName = strdup (filename);
	if (pconfig->fileName == NULL)
    {
		fun_cfg_done (pconfig);
		return -1;
    }
	/* If the file does not exist, try to create it */
	if (doCreate && access (pconfig->fileName, 0) == -1)
    {
		FILE *fd;
#ifndef unixcli
		fd = fopen_guardian (filename, "ab");
#else
		fd = fopen (filename, "ab");
#endif
		if (fd)	fclose (fd);
    }
	if (fun_cfg_refresh (pconfig) == -1)
    {
		fun_cfg_done (pconfig);
		return -1;
    }
	*ppconf = pconfig;
	return 0;
}


/*
 *  Free all data associated with a configuration
 */
static int fun_cfg_done (PCONFIG pconfig)
{
	if (pconfig)
    {
		fun_cfg_freeimage (pconfig);
		if (pconfig->fileName) free (pconfig->fileName);
		free (pconfig);
    }
	return 0;
}


/*
 *  Free the content specific data of a configuration
 */
static int fun_cfg_freeimage (PCONFIG pconfig)
{
	char *saveName;
	PCFGENTRY e;
	u_int i;

	if (pconfig->image) free (pconfig->image);
	if (pconfig->entries)
    {
		e = pconfig->entries;
		for (i = 0; i < pconfig->numEntries; i++, e++)
		{
			if (e->flags & CFE_MUST_FREE_SECTION) free (e->section);
			if (e->flags & CFE_MUST_FREE_ID) free (e->id);
			if (e->flags & CFE_MUST_FREE_VALUE) free (e->value);
			if (e->flags & CFE_MUST_FREE_COMMENT) free (e->comment);
		}
		free (pconfig->entries);
    }
	saveName = pconfig->fileName;
	memset (pconfig, 0, sizeof (TCONFIG));
	pconfig->fileName = saveName;
	return 0;
}


/*
 *  This procedure reads an copy of the file into memory
 *  caching the content based on stat
 */
static int fun_cfg_refresh (PCONFIG pconfig)
{
	FILE *fp;
	long size,length;
	long pos = 0;
	int status;
	char *mem;

	if (pconfig == NULL ) return -1;
#ifndef unixcli
	if ((fp = fopen_guardian (pconfig->fileName, "rb")) == NULL) return -1;
#else
	if ((fp = fopen (pconfig->fileName, "rb")) == NULL) return -1;
#endif
	size=0;
	mem = (char *) malloc (80);
	if (mem == NULL )
    {
		fclose (fp);
		return -1;
    }
	while(!feof(fp))
	{
		size += fread (mem, 1, 80, fp);
	}
	free (mem);
	rewind(fp);
	mem = (char *) malloc (size + 1);
	if (mem == NULL )
    {
		fclose (fp);
		return -1;
    }
	length = fread (mem, 1, size, fp);
	size = length;
	mem[size] = 0;
	fclose (fp);
	/*
	*  Store the new copy
	*/
	fun_cfg_freeimage (pconfig);
	pconfig->image = mem;
	pconfig->size = size;
	pconfig->mtime = 0;
	if (fun_cfg_parse (pconfig) == -1)
    {
		fun_cfg_freeimage (pconfig);
		return -1;
    }
	return 1;
}


#define iseolchar(C) (strchr ("\n\r\x1a", C) != NULL)
#define iswhite(C) (strchr ("\f\t ", C) != NULL)


static char * fun_cfg_skipwhite (char *s)
{
	while (*s && iswhite (*s)) s++;
	return s;
}

static int fun_cfg_getline (char **pCp, char **pLinePtr)
{
	char *start;
	char *cp = *pCp;

	while (*cp && iseolchar (*cp)) cp++;
	start = cp;
	if (pLinePtr) *pLinePtr = cp;

	while (*cp && !iseolchar (*cp)) cp++;
	if (*cp)
    {
		*cp++ = 0;
		*pCp = cp;
		while (--cp >= start && iswhite (*cp));
		cp[1] = 0;
    }
	else
		*pCp = cp;
	return *start ? 1 : 0;
}


static char * rtrim (char *str)
{
	char *endPtr;
	if (str == NULL || *str == '\0') return NULL;
	for (endPtr = &str[strlen (str) - 1]; endPtr >= str && isspace (*endPtr); endPtr--);
	endPtr[1] = 0;
	return endPtr >= str ? endPtr : NULL;
}


/*
 *  Parse the in-memory copy of the configuration data
 */
static int fun_cfg_parse (PCONFIG pconfig)
{
	int isContinue, inString;
	char *imgPtr;
	char *endPtr;
	char *lp;
	char *section;
	char *id;
	char *value;
	char *comment;

	if (pconfig == NULL) return 0;
	if (fun_cfg_valid (pconfig)) return 0;
	endPtr = pconfig->image + pconfig->size;
	for (imgPtr = pconfig->image; imgPtr < endPtr;)
    {
		if (!fun_cfg_getline (&imgPtr, &lp)) continue;
		section = id = value = comment = NULL;
		/*
         *  Skip leading spaces
		*/
		if (iswhite (*lp))
		{
			lp = fun_cfg_skipwhite (lp);
			isContinue = 1;
		}
		else isContinue = 0;
		/*
		*  Parse Section
		*/
		if (*lp == '[')
		{
			section = fun_cfg_skipwhite (lp + 1);
			if ((lp = strchr (section, ']')) == NULL) continue;
			*lp++ = 0;
			if (rtrim (section) == NULL)
			{
				section = NULL;
				continue;
			}
			lp = fun_cfg_skipwhite (lp);
		}
		else if (*lp != ';')
		{
			/* Try to parse
			*   1. Key = Value
			*   2. Value (iff isContinue)
			*/
			if (!isContinue)
			{
				/* Parse `<Key> = ..' */
				id = lp;
				if ((lp = strchr (id, '=')) == NULL) continue;
				*lp++ = 0;
				rtrim (id);
				lp = fun_cfg_skipwhite (lp);
			}
			/* Parse value */
			inString = 0;
			value = lp;
			while (*lp)
			{
				if (inString)
				{
					if (*lp == inString) inString = 0;
				}
				else if (*lp == '"' || *lp == '\'') inString = *lp;
				else if (*lp == ';' && iswhite (lp[-1]))
				{
					*lp = 0;
					comment = lp + 1;
					rtrim (value);
					break;
				}
				lp++;
			}
		}
		/*
		*  Parse Comment
		*/
		if (*lp == ';')	comment = lp + 1;
		if (fun_cfg_storeentry (pconfig, section, id, value, comment, 0) == -1)
		{
			pconfig->dirty = 1;
			return -1;
		}
    }
	pconfig->flags |= CFG_VALID;
	return 0;
}


static int fun_cfg_storeentry (
    PCONFIG pconfig,
    char *section,
    char *id,
    char *value,
    char *comment,
    int dynamic)
{
	PCFGENTRY data;

	if ((data = fun_cfg_poolalloc (pconfig, 1)) == NULL) return -1;
	data->flags = 0;
	if (dynamic)
    {
		if (section) section = strdup (section);
		if (id)	id = strdup (id);
		if (value) value = strdup (value);
		if (comment) comment = strdup (value);
		if (section) data->flags |= CFE_MUST_FREE_SECTION;
		if (id) data->flags |= CFE_MUST_FREE_ID;
		if (value) data->flags |= CFE_MUST_FREE_VALUE;
		if (comment)data->flags |= CFE_MUST_FREE_COMMENT;
    }
	data->section = section;
	data->id = id;
	data->value = value;
	data->comment = comment;
	return 0;
}


static PCFGENTRY fun_cfg_poolalloc (PCONFIG p, u_int count)
{
	PCFGENTRY newBase;
	u_int newMax;

	if (p->numEntries + count > p->maxEntries)
    {
		newMax = p->maxEntries ? count + p->maxEntries + p->maxEntries / 2 : count +  4096 / sizeof (TCFGENTRY);
		newBase = (PCFGENTRY) malloc (newMax * sizeof (TCFGENTRY));
		if (newBase == NULL) return NULL;
		if (p->entries)
		{
			memcpy (newBase, p->entries, p->numEntries * sizeof (TCFGENTRY));
			free (p->entries);
		}
		p->entries = newBase;
		p->maxEntries = newMax;
    }
	newBase = &p->entries[p->numEntries];
	p->numEntries += count;
	return newBase;
}


/*** COMPATIBILITY LAYER ***/


static int fun_cfg_rewind (PCONFIG pconfig)
{
	if (!fun_cfg_valid (pconfig)) return -1;
	pconfig->flags = CFG_VALID;
	pconfig->cursor = 0;
	return 0;
}


/*
 *  returns:
 *	 0 success
 *	-1 no next entry
 *
 *		section		id		value		flags			meaning
 *		!0			0		!0			SECTION			[value]
 *		!0			!0		!0			DEFINE			id = value|id="value"|id='value'
 *		!0			0		!0			0				value
 *		0			0		0			EOF				end of file encountered
 */
static int fun_cfg_nextentry (PCONFIG pconfig)
{
	PCFGENTRY e;

	if (!fun_cfg_valid (pconfig) || fun_cfg_eof (pconfig)) return -1;
	pconfig->flags &= ~(CFG_TYPEMASK);
	pconfig->id = pconfig->value = NULL;
	while (1)
    {
		if (pconfig->cursor >= pconfig->numEntries)
		{
			pconfig->flags |= CFG_EOF;
			return -1;
		}
		e = &pconfig->entries[pconfig->cursor++];
		if (e->section)
		{
			pconfig->section = e->section;
			pconfig->flags |= CFG_SECTION;
			return 0;
		}
		if (e->value)
		{
			pconfig->value = e->value;
			if (e->id)
			{
				pconfig->id = e->id;
				pconfig->flags |= CFG_DEFINE;
			}
			else
				pconfig->flags |= CFG_CONTINUE;
			return 0;
		}
    }
}


static int fun_cfg_find (PCONFIG pconfig, char *section, char *id)
{
	int atsection;

	if (!fun_cfg_valid (pconfig) || fun_cfg_rewind (pconfig)) return -1;
	atsection = 0;
	while (fun_cfg_nextentry (pconfig) == 0)
    {
		if (atsection)
		{
			if (fun_cfg_section (pconfig)) return -1;
			else if (fun_cfg_define (pconfig))
			{
				char *szId = fun_remove_quotes (pconfig->id);
				int bSame;
				if (szId)
				{
					bSame = !strcasecmp (szId, id);
					free (szId);
					if (bSame) return 0;
				}
			}
		}
		else if (fun_cfg_section (pconfig) && !strcasecmp (pconfig->section, section))
		{
			if (id == NULL) return 0;
			atsection = 1;
		}
    }
	return -1;
}


/*** WRITE MODULE ****/


/*
 *  Change the configuration
 *
 *  section id    value		action
 *  --------------------------------------------------------------------------
 *   value  value value		update '<entry>=<string>' in section <section>
 *   value  value NULL		delete '<entry>' from section <section>
 *   value  NULL  NULL		delete section <section>
 */
static int fun_cfg_write (
    PCONFIG pconfig,
    char *section,
    char *id,
    char *value)
{
	PCFGENTRY e, e2, eSect;
	int idx;
	int i;

	if (!fun_cfg_valid (pconfig) || section == NULL) return -1;
	/* find the section */
	e = pconfig->entries;
	i = pconfig->numEntries;
	eSect = 0;
	while (i--)
    {
		if (e->section && !strcasecmp (e->section, section))
		{
			eSect = e;
			break;
		}
		e++;
    }
	/* did we find the section? */
	if (!eSect)
    {
		/* check for delete operation on a nonexisting section */
		if (!id || !value) return 0;
		/* add section first */
		if (fun_cfg_storeentry (pconfig, (char *)section, (char *)NULL, (char *)NULL, (char *)NULL, 1) == -1
				|| fun_cfg_storeentry (pconfig, (char *)NULL, (char *)id, (char *)value, (char *)NULL, 1) == -1)
			return -1;
		pconfig->dirty = 1;
		return 0;
    }
	/* ok - we have found the section - let's see what we need to do */
	if (id)
    {
		if (value)
		{
		/* add / update a key */
			while (i--)
			{
				e++;
				/* break on next section */
				if (e->section)
				{
					/* insert new entry before e */
					idx = e - pconfig->entries;
					if (fun_cfg_poolalloc (pconfig, 1) == NULL) return -1;
					memmove (e + 1, e, (pconfig->numEntries - idx) * sizeof (TCFGENTRY));
					e->section = NULL;
					e->id = strdup (id);
					e->value = strdup (value);
					e->comment = NULL;
					if (e->id == NULL || e->value == NULL) return -1;
					e->flags |= CFE_MUST_FREE_ID | CFE_MUST_FREE_VALUE;
					pconfig->dirty = 1;
					return 0;
				}
				if (e->id && !strcasecmp (e->id, id))
				{
					/* found key - do update */
					if (e->value && (e->flags & CFE_MUST_FREE_VALUE))
					{
						e->flags &= ~CFE_MUST_FREE_VALUE;
						free (e->value);
					}
					pconfig->dirty = 1;
					if ((e->value = strdup (value)) == NULL) return -1;
					e->flags |= CFE_MUST_FREE_VALUE;
					return 0;
				}
			}
			/* last section in file - add new entry */
			if (fun_cfg_storeentry (pconfig, (char *)NULL, (char *)id, (char *)value, (char *)NULL, 1) == -1)return -1;
			pconfig->dirty = 1;
			return 0;
		}
		else
		{
			/* delete a key */
			while (i--)
			{
				e++;
				/* break on next section */
				if (e->section) return 0;	/* not found */
				if (e->id && !strcasecmp (e->id, id))
				{
					/* found key - do delete */
					eSect = e;
					e++;
					goto doDelete;
				}
			}
			/* key not found - that' ok */
			return 0;
		}
	}
	else
    {
		/* delete entire section */
		/* find e : next section */
		while (i--)
		{
			e++;
			/* break on next section */
			if (e->section) break;
		}
		if (i < 0) e++;
		/* move up e while comment */
		e2 = e - 1;
		while (e2->comment && !e2->section && !e2->id && !e2->value && (iswhite (e2->comment[0]) || e2->comment[0] == ';'))
			e2--;
		e = e2 + 1;
doDelete:
		/* move up eSect while comment */
		e2 = eSect - 1;
		while (e2->comment && !e2->section && !e2->id && !e2->value && (iswhite (e2->comment[0]) || e2->comment[0] == ';'))
			e2--;
		eSect = e2 + 1;
		/* delete everything between eSect .. e */
		for (e2 = eSect; e2 < e; e2++)
		{
			if (e2->flags & CFE_MUST_FREE_SECTION) free (e2->section);
			if (e2->flags & CFE_MUST_FREE_ID) free (e2->id);
			if (e2->flags & CFE_MUST_FREE_VALUE) free (e2->value);
			if (e2->flags & CFE_MUST_FREE_COMMENT) free (e2->comment);
		}
		idx = e - pconfig->entries;
		memmove (eSect, e, (pconfig->numEntries - idx) * sizeof (TCFGENTRY));
		pconfig->numEntries -= e - eSect;
		pconfig->dirty = 1;
	}
	return 0;
}


/*
 *  Write a formatted copy of the configuration to a file
 *
 *  This assumes that the inifile has already been parsed
 */
static void fun_cfg_outputformatted (PCONFIG pconfig, FILE *fd)
{
	PCFGENTRY e = pconfig->entries;
	int i = pconfig->numEntries;
	int m = 0;
	int j, l;
	int skip = 0;

	while (i--)
	{
		if (e->section)
		{
			/* Add extra line before section, unless comment block found */
			if (skip) fprintf (fd, "\n");
			fprintf (fd, "[%s]", e->section);
			if (e->comment) fprintf (fd, "\t;%s", e->comment);
			/* Calculate m, which is the length of the longest key */
			m = 0;
			for (j = 1; j <= i; j++)
			{
				if (e[j].section) break;
				if (e[j].id && (l = strlen (e[j].id)) > m) m = l;
			}
			/* Add an extra lf next time around */
			skip = 1;
		}
		/*
		*  Key = value
		*/
		else if (e->id && e->value)
		{
		if (m) fprintf (fd, "%-*.*s = %s", m, m, e->id, e->value);
		else  fprintf (fd, "%s = %s", e->id, e->value);
		if (e->comment) fprintf (fd, "\t;%s", e->comment);
	}
	/*
	*  Value only (continuation)
	*/
	else if (e->value)
	{
		fprintf (fd, "  %s", e->value);
		if (e->comment) fprintf (fd, "\t;%s", e->comment);
	}
      /*
       *  Comment only - check if we need an extra lf
       *
       *  1. Comment before section gets an extra blank line before
       *     the comment starts.
       *
       *          previousEntry = value
       *          <<< INSERT BLANK LINE HERE >>>
       *          ; Comment Block
       *          ; Sticks to section below
       *          [new section]
       *
       *  2. Exception on 1. for commented out definitions:
       *     (Immediate nonwhitespace after ;)
       *          [some section]
       *          v1 = 1
       *          ;v2 = 2   << NO EXTRA LINE >>
       *          v3 = 3
       *
       *  3. Exception on 2. for ;; which certainly is a section comment
       *          [some section]
       *          definitions
       *          <<< INSERT BLANK LINE HERE >>>
       *          ;; block comment
       *          [new section]
       */
		else if (e->comment)
		{
			if (skip && (iswhite (e->comment[0]) || e->comment[0] == ';'))
			{
				for (j = 1; j <= i; j++)
				{
					if (e[j].section)
					{
						fprintf (fd, "\n");
						skip = 0;
						break;
					}
					if (e[j].id || e[j].value)  break;
				}
			}
			fprintf (fd, ";%s", e->comment);
		}
		fprintf (fd, "\n");
		e++;
	}
}


/*
 *  Write the changed file back
 */
static int fun_cfg_commit (PCONFIG pconfig)
{
	FILE *fp;

	if (!fun_cfg_valid (pconfig)) return -1;
	if (pconfig->dirty)
    {
#ifndef unixcli
		if ((fp = fopen_guardian (pconfig->fileName, "w")) == NULL) return -1;
#else
		if ((fp = fopen (pconfig->fileName, "w")) == NULL) return -1;
#endif
		fun_cfg_outputformatted (pconfig, fp);
		fclose (fp);
		pconfig->dirty = 0;
    }
	return 0;
}


static int fun_cfg_next_section(PCONFIG pconfig)
{
	PCFGENTRY e;

	do
		if (0 != fun_cfg_nextentry (pconfig)) return -1;
	while (!fun_cfg_section (pconfig));
	return 0;
}


static int fun_cfg_search_init(PCONFIG *ppconf, const char *filename, int doCreate)
{
	char pathbuf[1024];

	/*if (access(filename, R_OK) == 0) return fun_cfg_init (ppconf, filename, doCreate); */
	if (strstr (filename, DSNFILE) || strstr (filename, DSNFILE))
		return fun_cfg_init (ppconf, fun_getinifile (pathbuf,sizeof (pathbuf), FALSE, doCreate), doCreate);
	else
		return -1;
}


static int fun_list_sections (PCONFIG pCfg, LPSTR lpszRetBuffer, int cbRetBuffer)
{
	int curr = 0, sect_len = 0;
	lpszRetBuffer[0] = 0;

	if (0 == fun_cfg_rewind (pCfg))
    {
		while (curr < cbRetBuffer && 0 == fun_cfg_next_section (pCfg) && pCfg->section)
		{
			sect_len = strlen (pCfg->section) + 1;
			sect_len = sect_len > cbRetBuffer - curr ? cbRetBuffer - curr : sect_len;
			memmove (lpszRetBuffer + curr, pCfg->section, sect_len);
			curr += sect_len;
		}
		if (curr < cbRetBuffer)	lpszRetBuffer[curr] = 0;
		return curr;
	}
	return 0;
}


static int fun_list_entries (PCONFIG pCfg, LPCSTR lpszSection, LPSTR lpszRetBuffer, int cbRetBuffer)
{
	int curr = 0, sect_len = 0;
	lpszRetBuffer[0] = 0;

	if (0 == fun_cfg_rewind (pCfg))
    {
		while (curr < cbRetBuffer && 0 == fun_cfg_nextentry (pCfg))
		{
			if (fun_cfg_define (pCfg) && !strcmp (pCfg->section, lpszSection) && pCfg->id)
			{
				sect_len = strlen (pCfg->id) + 1;
				sect_len = sect_len > cbRetBuffer - curr ? cbRetBuffer - curr : sect_len;
				memmove (lpszRetBuffer + curr, pCfg->id, sect_len);
				curr += sect_len;
			}
		}
		if (curr < cbRetBuffer) lpszRetBuffer[curr] = 0;
		return curr;
    }
	return 0;
}


static BOOL do_create_dsns (PCONFIG pCfg, PCONFIG pInfCfg, LPSTR szDriver, LPSTR szDSNS, LPSTR szDiz)
{
	char *szValue = strdup (szDSNS), *szCurr = szValue, *szComma;
	int hasMore = FALSE;
	BOOL retcode = FALSE;

	do
    {
		szComma = strchr (szCurr, ',');
		if (szComma)
		{
			*szComma = 0;
			hasMore = TRUE;
		}
		else
			hasMore = FALSE;
		if (fun_cfg_write (pCfg, (char *)"ODBC Data Sources", szCurr, szDiz)) goto error;
		if (!ValidDSN (szCurr) || fun_cfg_write (pCfg, szCurr, NULL, NULL)) goto error;
		if (fun_cfg_find (pInfCfg, szCurr, NULL) && !fun_cfg_write (pCfg, szCurr, NULL, NULL))
		{
			if (fun_cfg_write (pCfg, szCurr, "Driver", szDriver)) goto error;
			while (!fun_cfg_nextentry (pInfCfg) && fun_cfg_define (pInfCfg))
			{
				if (fun_cfg_write (pCfg, szCurr, pInfCfg->id,pInfCfg->value)) goto error;
			}
		}
		if (hasMore)
			szCurr = szComma + 1;
    }
	while (hasMore);
	retcode = TRUE;
error:
	free (szValue);
	return retcode;
}


static BOOL install_from_ini (PCONFIG pCfg, PCONFIG pOdbcCfg, LPSTR szInfFile, LPSTR szDriver, BOOL drivers)
{
	PCONFIG pInfCfg;
	char *szKeysSection = NULL, *szDriverFile = NULL, *szSetupFile = NULL,*szValue = NULL, *szId = NULL, *szComma, *szComma1;
	BOOL ret = FALSE;

	if (fun_cfg_write (pCfg, szDriver, NULL, NULL)) return ret;
	if (fun_cfg_init (&pInfCfg, szInfFile, FALSE)) return ret;
	if (fun_cfg_find (pInfCfg, drivers ? (char *)"ODBC Drivers" : (char *)"ODBC Translators", szDriver)) goto error;
	if (fun_cfg_write (pCfg, drivers ? (char *)"ODBC Drivers" : (char *)"ODBC Translators", szDriver, (char *)"Installed")) goto error;
	if (fun_cfg_find (pInfCfg, szDriver, drivers ? (char *)"Driver" : (char *)"Translator")) goto error;
	szComma = strchr (pInfCfg->value, ',');
	if (szComma != NULL)
	{
		szComma1 = strchr (szComma + 1, ',');
		if (szComma1 == NULL)
			goto error;
	}
	else
		goto error;
	if (!szComma || !szComma1 || szComma + 1 == szComma1) goto error;
	*szComma1 = 0;
	szDriverFile = strdup (szComma + 1);
	if (fun_cfg_write (pCfg, szDriver, drivers ? (char *)"Driver" : (char *)"Translator", szDriverFile)) goto error;
	if (!fun_cfg_find (pInfCfg, szDriver, "Setup"))
    {
		szComma = strchr (pInfCfg->value, ',');
		if (szComma != NULL)
		{
			szComma1 = strchr (szComma + 1, ',');
			if (szComma1 == NULL)
				goto error;
		}
		else
			goto error;
		if (!szComma || !szComma1 || szComma + 1 == szComma1) goto error;
		*szComma1 = 0;
		szSetupFile = strdup (szComma + 1);
		if (fun_cfg_write (pCfg, szDriver, "Setup", szSetupFile))goto error;
    }
	if (!fun_cfg_find (pInfCfg, szDriver, NULL))
    {
		while (!fun_cfg_nextentry (pInfCfg) && fun_cfg_define (pInfCfg))
			if (strcmp (pInfCfg->id, drivers ? "\"Driver\"" : "\"Translator\"") && strcmp (pInfCfg->id, "\"Setup\""))
			{
				szComma = strchr (pInfCfg->value, ',');
				szComma1 = strchr (szComma + 1, ',');
				if (!szComma || !szComma1 || szComma + 1 == szComma1) szValue = strdup ("");
				else
				{
					*szComma1 = 0;
					szValue = strdup (szComma + 1);
				}
				szComma = strchr (pInfCfg->id, '"');
				szComma1 = strchr (szComma + 1, '"');
				if (!szComma || !szComma1 || szComma + 1 == szComma1) goto loop_cont;
				else
				{
					*szComma1 = 0;
					szId = strdup (szComma + 1);
				}
				if (fun_cfg_write (pCfg, szDriver, szId, szValue)) goto error;
loop_cont:
				if (szValue)
				{
					free (szValue);
					szValue = NULL;
				}
				if (szId)
				{
					free (szId);
					szId = NULL;
				}
			}
		}
	if (!drivers) goto quit;
	szKeysSection = (char *) calloc (strlen (szDriver) + 6, sizeof (char));
	if (szKeysSection != NULL)
	{
		strcpy (szKeysSection, szDriver);
		strcat (szKeysSection, "-Keys");
		if (!fun_cfg_find (pInfCfg, szKeysSection, NULL))
		{
			while (!fun_cfg_nextentry (pInfCfg) && fun_cfg_define (pInfCfg))
			{
				if (strcmp (pInfCfg->id, "CreateDSN"))
				{
					if (fun_cfg_write (pCfg, szDriver, pInfCfg->id, pInfCfg->value)) goto error;
				}
				else if (!do_create_dsns (pOdbcCfg, pCfg, szDriverFile, pInfCfg->value, szDriver)) goto error;
			}
		}
	}
quit:
	ret = TRUE;
error:
	if (szKeysSection) free (szKeysSection);
	if (szDriverFile) free (szDriverFile);
	if (szSetupFile) free (szSetupFile);
	if (szValue) free (szValue);
	if (szId) free (szId);
	fun_cfg_done (pInfCfg);
	return ret;
}


static int install_from_string (PCONFIG pCfg, PCONFIG pOdbcCfg, LPSTR lpszDriver, BOOL drivers)
{
	char *szCurr = (char *) lpszDriver, *szDiz = lpszDriver;
	char *szAsignment, *szEqual, *szValue, *szDriver = NULL;

	if (fun_cfg_write (pCfg, lpszDriver, NULL, NULL)) return FALSE;
	if (fun_cfg_write (pCfg, drivers ? (char *)"ODBC Drivers" : (char *)"ODBC Translators", lpszDriver, "Installed")) return FALSE;
	for (szCurr = lpszDriver + strlen (lpszDriver) + 1; *szCurr; szCurr += strlen (szCurr) + 1)
    {
		szAsignment = strdup (szCurr);
		szEqual = strchr (szAsignment, '=');
		if (szEqual) *szEqual = 0;
		else goto loop_error;
		szValue = szEqual + 1;
		if ((drivers && !strcmp (szAsignment, "Driver")) || (!drivers && !strcmp (szAsignment, "Translator")))
		{
			if (szDriver) free (szDriver);
			szDriver = strdup (szValue);
		}
		if (drivers)
		{
			if (strcmp (szAsignment, "CreateDSN"))
			{
				if (fun_cfg_write (pCfg, lpszDriver, szAsignment, szValue)) goto loop_error;
			}
			else if (!do_create_dsns (pOdbcCfg, pCfg, szDriver, szValue, szDiz)) goto loop_error;
		}
		else if (fun_cfg_write (pCfg, lpszDriver, szAsignment, szValue)) goto loop_error;
		free (szAsignment);
		continue;
loop_error:
		if (szDriver) free (szDriver);
		free (szAsignment);
		return FALSE;
    }
	if (szDriver) free (szDriver);
	else return FALSE;
	return TRUE;
}

static char *fun_remove_quotes(const char *szString)
{
	char *szWork, *szPtr;

	while (*szString == '\'' || *szString == '\"') szString += 1;
	if (!*szString) return NULL;
	szWork = strdup (szString);
	szPtr = strchr (szWork, '\'');
	if (szPtr) *szPtr = 0;
	szPtr = strchr (szWork, '\"');
	if (szPtr) *szPtr = 0;
	return szWork;
}

int GetPrivateProfileString (
	LPCSTR lpszSection, 
	LPCSTR lpszEntry,
    LPCSTR lpszDefault, 
	LPSTR lpszRetBuffer, 
	int cbRetBuffer,
    LPCSTR lpszFilename)
{
	char *defval = (char *) lpszDefault, *value = NULL;
	int len = 0;
	PCONFIG pCfg;

//	lpszRetBuffer[cbRetBuffer - 1] = 0;
	memset(lpszRetBuffer,0,cbRetBuffer - 1);
	/* If error during reading the file */
	if (fun_cfg_search_init (&pCfg, lpszFilename, FALSE))
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_PATH);
		goto fail;
    }
	/* List all sections from the ini file */
	if (lpszSection == NULL || *lpszSection == '\0')
    {
		len = fun_list_sections (pCfg, lpszRetBuffer, cbRetBuffer);
		goto done;
    }
	/* List all the entries of the specified section */
	if (lpszEntry == NULL || *lpszEntry == '\0')
    {
		len = fun_list_entries (pCfg, lpszSection, lpszRetBuffer, cbRetBuffer);
		goto done;
    }
	/*
	*  Sorry for this one -- Windows cannot handle a default value of
	*  "" in GetPrivateProfileString, so it is passed as " " instead.
	*/
	if (defval == NULL || *defval == '\0') defval = " ";
	/*
	*  Check whether someone else has modified the odbc.ini file
	*/
	fun_cfg_refresh (pCfg);
	if (!fun_cfg_find (pCfg, (LPSTR) lpszSection, (LPSTR) lpszEntry))
		value = pCfg->value;
	if (value == NULL)
    {
		value = defval;
		if (value[0] == ' ' && value[1] == '\0') value = "";
    }
	strncpy (lpszRetBuffer, value, cbRetBuffer - 1);
done:
	fun_cfg_done (pCfg);
fail:
	if (!len)
		len = strlen (lpszRetBuffer);
	if (len == cbRetBuffer - 1)
		PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
	return len;
}

BOOL WritePrivateProfileString (
		LPCSTR lpszSection, 
		LPCSTR lpszEntry,
		LPCSTR lpszString, 
		LPCSTR lpszFilename)
{
	BOOL retcode = FALSE;
	PCONFIG pCfg = NULL;

	/* Check Input parameters */
	if (lpszSection == NULL || *lpszSection == '\0')
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_REQUEST_TYPE);
		goto fail;
    }
	/* If error during reading the file */
	if (fun_cfg_search_init (&pCfg, lpszFilename, TRUE))
    {
		PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
		goto fail;
    }
	/* Check if the section must be deleted */
	if (!lpszEntry)
    {
		fun_cfg_write (pCfg, (LPSTR) lpszSection, NULL, NULL);
		goto done;
    }
	/* Check if the entry must be deleted */
	if (!lpszString)
    {
		fun_cfg_write (pCfg, (LPSTR) lpszSection, (LPSTR) lpszEntry, NULL);
		goto done;
    }
	/* Else add the entry */
	fun_cfg_write (pCfg, (LPSTR) lpszSection, (LPSTR) lpszEntry, (LPSTR) lpszString);
done:
	if (!fun_cfg_commit (pCfg))
		retcode = TRUE;
	else
    {
		PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
		goto fail;
    }
fail:
	if (pCfg) fun_cfg_done (pCfg);
	return retcode;
}

BOOL RemoveDSNFromIni (LPCSTR lpszDSN)
{
	char szBuffer[4096];
	BOOL retcode = FALSE;
	PCONFIG pCfg;

	/* Check dsn */
	if (!lpszDSN || !ValidDSN (lpszDSN) || !strlen (lpszDSN))
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_DSN);
		goto quit;
    }
	if (fun_cfg_search_init (&pCfg, DSNFILE, TRUE))
    {
		PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
		goto quit;
    }
	if (strcmp (lpszDSN, "Default"))
    {
		/* deletes a DSN from [ODBC data sources] section */
		fun_cfg_write (pCfg, "ODBC Data Sources", (LPSTR) lpszDSN, NULL);
    }
	/* deletes the DSN section in odbc.ini */
	fun_cfg_write (pCfg, (LPSTR) lpszDSN, NULL, NULL);
	if (fun_cfg_commit (pCfg))
    {
		PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
		goto done;
    }
	retcode = TRUE;
done:
	fun_cfg_done (pCfg);
quit:
	return retcode;
}

BOOL RemoveDefaultDataSource (void)
{
	BOOL retcode = TRUE;
	PCONFIG pCfg = NULL;

	/* removes the default dsn */
	if (!RemoveDSNFromIni ("Default"))
    {
		PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
		retcode = FALSE;
    }
	return retcode;
}

BOOL WriteDSNToIni (LPCSTR lpszDSN, LPCSTR lpszDriver)
{
	char szBuffer[4096];
	BOOL retcode = FALSE;
	PCONFIG pCfg = NULL;

	strncpy(szBuffer,ODBC_DRIVER,sizeof(szBuffer));
	if (fun_cfg_search_init (&pCfg, DSNFILE, TRUE))
    {
		PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
		goto done;
    }
	if (strcmp (lpszDSN, "Default"))
    {
		/* adds a DSN=Driver to the [ODBC data sources] section */
		if (fun_cfg_write (pCfg, "ODBC Data Sources", (LPSTR) lpszDSN, (LPSTR) szBuffer))
		{
			PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
			goto done;
		}
    }
	/* deletes the DSN section in odbc.ini */
	if (fun_cfg_write (pCfg, (LPSTR) lpszDSN, NULL, NULL))
    {
		PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
		goto done;
    }
	/* adds a [DSN] section with Driver key */
	if (fun_cfg_write (pCfg, (LPSTR) lpszDSN, "Driver", szBuffer) || fun_cfg_commit (pCfg))
    {
		PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
		goto done;
    }
	retcode = TRUE;
done:
	wSystemDSN = USERDSN_ONLY;
	configMode = ODBC_BOTH_DSN;
	if (pCfg) fun_cfg_done (pCfg);
	return retcode;
}











