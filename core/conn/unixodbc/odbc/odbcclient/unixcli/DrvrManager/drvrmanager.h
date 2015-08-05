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
/**************************************************************************
**************************************************************************/
//
#ifndef DRVRMANAGER_H
#define DRVRMANAGER_H

#define ODBC_LOCK()
#define ODBC_UNLOCK()

#define RETURNCODE(x, rc) \
	if ((x)) ((CHandle *)(x))->returncode = rc

#define	PUSHSQLERR(x, code)	\
	x->setDiagRec( DM_ERROR, code)

#define CLEAR_ERRORS(x) \
	x->clearError()

//===================== Environment ================================

#define IS_VALID_HENV(x) \
	((x) != SQL_NULL_HENV && ((CEnv *)(x))->type == SQL_HANDLE_ENV)

//===================== Connection ==================================

#define IS_VALID_HDBC(x) \
	((x) != SQL_NULL_HDBC && ((CConnect *)(x))->type == SQL_HANDLE_DBC)

#define ENTER_HDBC(pdbc) \
	ODBC_LOCK();\
	if (!IS_VALID_HDBC (pdbc)) \
	{ \
		ODBC_UNLOCK (); \
		return SQL_INVALID_HANDLE; \
	} \
	else if (pdbc->dbc_cip) \
	{ \
		PUSHSQLERR (pdbc, IDS_S1_010); \
		ODBC_UNLOCK(); \
		return SQL_ERROR; \
	} \
	pdbc->dbc_cip = 1; \
	CLEAR_ERRORS (pdbc); \
	ODBC_UNLOCK();


#define LEAVE_HDBC(pdbc, err) \
	pdbc->dbc_cip = 0; \
	return (err);

//================================ Statement =======================

#define IS_VALID_HSTMT(x) \
	((x) != SQL_NULL_HSTMT && \
	 ((CStmt *)(x))->type == SQL_HANDLE_STMT && \
	 ((CStmt *)(x))->getConnectHandle() != SQL_NULL_HDBC)

#define ENTER_STMT(pstmt) \
	ODBC_LOCK(); \
	if (!IS_VALID_HSTMT (pstmt)) \
	{ \
		ODBC_UNLOCK(); \
		return SQL_INVALID_HANDLE; \
	} \
	else if (pstmt->stmt_cip) \
	{ \
		PUSHSQLERR (pstmt, IDS_S1_010); \
		ODBC_UNLOCK(); \
		return SQL_ERROR; \
	} \
	pstmt->stmt_cip = 1; \
	CLEAR_ERRORS (pstmt); \
	ODBC_UNLOCK(); \


#define LEAVE_STMT(pstmt, err) \
	pstmt->stmt_cip = 0; \
	return (err);

//==================== Descriptor =======================

#define IS_VALID_HDESC(x) \
	((x) != SQL_NULL_HDESC && \
		((CDesc *)(x))->type == SQL_HANDLE_DESC && \
		((CDesc *)(x))->getDescConnect() != SQL_NULL_HDBC)

#define ENTER_HDESC(pdesc) \
	ODBC_LOCK();\
	if (!IS_VALID_HDESC (pdesc)) \
	{ \
		ODBC_UNLOCK (); \
		return SQL_INVALID_HANDLE; \
	} \
	else if (pdesc->desc_cip) \
	{ \
		PUSHSQLERR (pdesc, IDS_S1_010); \
		ODBC_UNLOCK(); \
		return SQL_ERROR; \
	} \
	pdesc->desc_cip = 1; \
	CLEAR_ERRORS (pdesc); \
	ODBC_UNLOCK();


#define LEAVE_HDESC(pdesc, err) \
	pdesc->desc_cip = 0; \
	return (err);

 //=====================================================

#define STMT(stmt, var) \
	CStmt *stmt = (CStmt *)var

#define CONN(con, var) \
	CConnect *con = (CConnect *)var

#define GENV(genv, var) \
	CEnv *genv = (CEnv *)var

#define DESC(desc, var) \
	CDesc *desc = (CDesc *)var

//=================================================================
enum odbcapi_t
{
	en_NullProc = 0,

	en_AllocEnv,
	en_AllocConnect,
	en_Connect,
	en_DriverConnect,
	en_BrowseConnect,

	en_DataSources,
	en_Drivers,
	en_GetInfo,
	en_GetFunctions,
	en_GetTypeInfo,

	en_SetConnectOption,
	en_GetConnectOption,
	en_SetStmtOption,
	en_GetStmtOption,

	en_AllocStmt,
	en_Prepare,
	en_BindParameter,
	en_ParamOptions,
	en_GetCursorName,
	en_SetCursorName,
	en_SetScrollOptions,
	en_SetParam,

	en_Execute,
	en_ExecDirect,
	en_NativeSql,
	en_DescribeParam,
	en_NumParams,
	en_ParamData,
	en_PutData,

	en_RowCount,
	en_NumResultCols,
	en_DescribeCol,
	en_ColAttributes,
	en_BindCol,
	en_Fetch,
	en_ExtendedFetch,
	en_GetData,
	en_SetPos,
	en_MoreResults,
	en_Error,

	en_ColumnPrivileges,
	en_Columns,
	en_ForeignKeys,
	en_PrimaryKeys,
	en_ProcedureColumns,
	en_Procedures,
	en_SpecialColumns,
	en_Statistics,
	en_TablePrivileges,
	en_Tables,

	en_FreeStmt,
	en_Cancel,
	en_Transact,

	en_Disconnect,
	en_FreeConnect,
	en_FreeEnv,

	en_AllocHandle,
	en_BindParam,
	en_BulkOperations,
	en_CloseCursor,
	en_ColAttribute,
	en_CopyDesc,
	en_EndTran,
	en_FetchScroll,
	en_FreeHandle,
	en_GetConnectAttr,
	en_GetDescField,
	en_GetDescRec,
	en_GetDiagField,
	en_GetDiagRec,
	en_GetEnvAttr,
	en_GetStmtAttr,
	en_SetConnectAttr,
	en_SetDescField,
	en_SetDescRec,
	en_SetEnvAttr,
	en_SetStmtAttr,
  } ;

//=============== states ======================

enum
  {
    en_dbc_allocated = 0,
    en_dbc_needdata,
    en_dbc_connected,
    en_dbc_hstmt
  };

enum
  {
    en_stmt_allocated = 0,
    en_stmt_prepared,
    en_stmt_executed,
    en_stmt_cursoropen,
    en_stmt_fetched,
    en_stmt_xfetched,
    en_stmt_needdata,		/* not call SQLParamData() yet */
    en_stmt_mustput,		/* not call SQLPutData() yet */
    en_stmt_canput		/* SQLPutData() called */
  };				/* for statement handle state */

enum
  {
    en_stmt_cursor_no = 0,
    en_stmt_cursor_named,
    en_stmt_cursor_opened,
    en_stmt_cursor_fetched,
    en_stmt_cursor_xfetched
  };				/* for statement cursor state */

//=========================== info ======================================
#define SQL_ODBC3_SET_FUNC_ON(pfExists, uwAPI) \
	*( ((UWORD*) (pfExists)) + ((uwAPI) >> 4) ) |= (1 << ((uwAPI) & 0x000F))

#define SQL_ODBC3_SET_FUNC_OFF(pfExists, uwAPI) \
	*( ((UWORD*) (pfExists)) + ((uwAPI) >> 4) ) &= !(1 << ((uwAPI) & 0x000F))

#define __LAST_API_FUNCTION__ 9999

static int FunctionNumbers[] =
{
    0,
	SQL_API_SQLALLOCENV,								// "AllocEnv"
	SQL_API_SQLALLOCCONNECT,							// "AllocConnect"
	SQL_API_SQLCONNECT,									// "Connect"
	SQL_API_SQLDRIVERCONNECT,							// "DriverConnect"
	SQL_API_SQLBROWSECONNECT,							// "BrowseConnect"

	SQL_API_SQLDATASOURCES,								// "DataSources"
	SQL_API_SQLDRIVERS,									// "Driver"
	SQL_API_SQLGETINFO,									// "GetInfo"
	SQL_API_SQLGETFUNCTIONS,							// "GetFunctions"
	SQL_API_SQLGETTYPEINFO,								// "GetTypeInfo"

	SQL_API_SQLSETCONNECTOPTION,						// "SetConnectOption"
	SQL_API_SQLGETCONNECTOPTION,						// "GetConnectOption"
	SQL_API_SQLSETSTMTOPTION,							// "SetStmtOption"
	SQL_API_SQLGETSTMTOPTION,							// "GetStmtOption"

	SQL_API_SQLALLOCSTMT,								// "AllocStmt"
	SQL_API_SQLPREPARE,									// "Prepare"
	SQL_API_SQLBINDPARAMETER,							// "BindParameter"
	SQL_API_SQLPARAMOPTIONS,							// "ParamOptions"
	SQL_API_SQLGETCURSORNAME,							// "GetCursorName"
	SQL_API_SQLSETCURSORNAME,							// "SetCursorName"
	SQL_API_SQLSETSCROLLOPTIONS,						// "SetScrollOptions"
	SQL_API_SQLSETPARAM,								// "SetParam"

	SQL_API_SQLEXECUTE,									// "Execute"
	SQL_API_SQLEXECDIRECT,								// "ExecDirect"
	SQL_API_SQLNATIVESQL,								// "NativeSql"
	SQL_API_SQLDESCRIBEPARAM,							// "DescribeParam"
	SQL_API_SQLNUMPARAMS,								// "NumParams"
	SQL_API_SQLPARAMDATA,								// "ParamData"
	SQL_API_SQLPUTDATA,									// "PutData"

	SQL_API_SQLROWCOUNT,								// "RowCount"
	SQL_API_SQLNUMRESULTCOLS,							// "NumResultCols"
	SQL_API_SQLDESCRIBECOL,								// "DescribeCol"
	SQL_API_SQLCOLATTRIBUTES,							// "ColAttributes"
	SQL_API_SQLBINDCOL,									// "BindCol"
	SQL_API_SQLFETCH,									// "Fetch"
	SQL_API_SQLEXTENDEDFETCH,							// "ExtendedFetch"
	SQL_API_SQLGETDATA,									// "GetData"
	SQL_API_SQLSETPOS,									// "SetPos"
	SQL_API_SQLMORERESULTS,								// "MoreResults"
	SQL_API_SQLERROR,									// "Error"

	SQL_API_SQLCOLUMNPRIVILEGES,						// "ColumnPrivileges"
	SQL_API_SQLCOLUMNS,									// "Columns"
	SQL_API_SQLFOREIGNKEYS,								// "ForeignKeys"
	SQL_API_SQLPRIMARYKEYS,								// "PrimaryKeys"
	SQL_API_SQLPROCEDURECOLUMNS,						// "ProcedureColumns"
	SQL_API_SQLPROCEDURES,								// "Procedures"
	SQL_API_SQLSPECIALCOLUMNS,							// "SpecialColumns"
	SQL_API_SQLSTATISTICS,								// "Statistics"
	SQL_API_SQLTABLEPRIVILEGES,							// "TablePrivileges"
	SQL_API_SQLTABLES,									// "Tables"

	SQL_API_SQLFREESTMT,								// "FreeStmt"
	SQL_API_SQLCANCEL,									// "Cancel"
	SQL_API_SQLTRANSACT,								// "Transact"

	SQL_API_SQLDISCONNECT,								// "Disconnect"
	SQL_API_SQLFREECONNECT,								// "FreeConnect"
	SQL_API_SQLFREEENV,									// "FreeEnv"

// ODBCVER >= 0x0300 
	SQL_API_SQLALLOCHANDLE,								// "AllocHandle"
	SQL_API_SQLBINDPARAM,								// "BindParam"
	0, //SQL_API_SQLBULKOPERATIONS,						// "BulkOperations"
	SQL_API_SQLCLOSECURSOR,								// "CloseCursor"
	SQL_API_SQLCOLATTRIBUTE,							// "ColAttribute"
	SQL_API_SQLCOPYDESC,								// "CopyDesc"
	SQL_API_SQLENDTRAN,									// "EndTran"
	SQL_API_SQLFETCHSCROLL,								// "FetchScroll"
	SQL_API_SQLFREEHANDLE,								// "FreeHandle"
	SQL_API_SQLGETCONNECTATTR,							// "GetConnectAttr"
	SQL_API_SQLGETDESCFIELD,							// "GetDescField"
	SQL_API_SQLGETDESCREC,								// "GetDescRec"
	SQL_API_SQLGETDIAGFIELD,							// "GetDiagField"
	SQL_API_SQLGETDIAGREC,								// "GetDiagRec"
	SQL_API_SQLGETENVATTR,								// "GetEnvAttr"
	SQL_API_SQLGETSTMTATTR,								// "GetStmtAttr"
	SQL_API_SQLSETCONNECTATTR,							// "SetConnectAttr"
	SQL_API_SQLSETDESCFIELD,							// "SetDescField"
	SQL_API_SQLSETDESCREC,								// "SetDescRec"
	SQL_API_SQLSETENVATTR,								// "SetEnvAttr"
	SQL_API_SQLSETSTMTATTR,								// "SetStmtAttr"

	__LAST_API_FUNCTION__
};

#endif
