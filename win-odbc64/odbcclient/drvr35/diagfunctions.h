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
#ifndef DIAGFUNCTIONS_H
#define DIAGFUNCTIONS_H

#include <windows.h>
#include <sql.h>
#include <sqlExt.h>
#include "sqlTrace.h"
#include <sys/timeb.h>

//==========================================================
// Trace options
//==========================================================
#define TR_ON					0x00000001
#define TR_ODBC_API				0x00000002
#define TR_DRVR_TRANSPORT_API	0x00000004

#define RESET_TRACE()\
{\
	pdwGlobalTraceVariable = NULL;\
}

extern void TraceOut(long TraceOption, char *text, ...);
extern void HexOut(long TraceOption, SQLLEN* len, void* buffer, char *text );
extern void InitializeTrace();
extern BOOL IsTraceLibrary();
extern char *ElapsedTimeString(struct _timeb StartTime);
extern void LogFile( char* txt1, char* txt2, char* txt3 );
extern void LogInfo(VERSION_def* ASVersion, VERSION_def* SrvrVersion,VERSION_def* SqlVersion);
extern void TRACE_TRANSPORT_IN(int operation, char* reference, void* prheader, char* rbuffer, long tcount, long timeout);
extern void TRACE_TRANSPORT_OUT(int operation, char* reference, void* pheader, char* wbuffer, long tcount, long timeout);

extern FPTraceProcessEntry			fpTraceProcessEntry;
extern FPTraceDebugOut				fpTraceDebugOout;
extern FPTracePrintMarker			fpTracePrintMarker;
extern FPTraceReturn				fpTraceReturn;
extern FPTraceSQLAllocHandle		fpTraceSQLAllocHandle;		
extern FPTraceSQLBindCol			fpTraceSQLBindCol;
extern FPTraceSQLBindParameter		fpTraceSQLBindParameter;
extern FPTraceSQLCancel				fpTraceSQLCancel;
extern FPTraceSQLCloseCursor		fpTraceSQLCloseCursor;
extern FPTraceSQLCopyDesc			fpTraceSQLCopyDesc;
extern FPTraceSQLDescribeParam		fpTraceSQLDescribeParam;
extern FPTraceSQLDisconnect			fpTraceSQLDisconnect;
extern FPTraceSQLEndTran			fpTraceSQLEndTran;
extern FPTraceSQLExecute			fpTraceSQLExecute;
extern FPTraceSQLExtendedFetch		fpTraceSQLExtendedFetch;
extern FPTraceSQLFetch				fpTraceSQLFetch;
extern FPTraceSQLFetchScroll		fpTraceSQLFetchScroll;
extern FPTraceSQLFreeHandle			fpTraceSQLFreeHandle;
extern FPTraceSQLFreeStmt			fpTraceSQLFreeStmt;
extern FPTraceSQLGetData			fpTraceSQLGetData;
extern FPTraceSQLGetEnvAttr			fpTraceSQLGetEnvAttr;
extern FPTraceSQLGetTypeInfo		fpTraceSQLGetTypeInfo;
extern FPTraceSQLMoreResults		fpTraceSQLMoreResults;
extern FPTraceSQLNumParams			fpTraceSQLNumParams;
extern FPTraceSQLNumResultCols		fpTraceSQLNumResultCols;
extern FPTraceSQLParamData			fpTraceSQLParamData;
extern FPTraceSQLPutData			fpTraceSQLPutData;
extern FPTraceSQLRowCount			fpTraceSQLRowCount;
extern FPTraceSQLSetEnvAttr			fpTraceSQLSetEnvAttr;
extern FPTraceSQLSetPos				fpTraceSQLSetPos;
extern FPTraceSQLSetDescRec			fpTraceSQLSetDescRec;
//Unicode driver
extern FPTraceSQLGetDiagRecW		fpTraceSQLGetDiagRecW;
extern FPTraceSQLGetDiagFieldW		fpTraceSQLGetDiagFieldW;
extern FPTraceSQLConnectW			fpTraceSQLConnectW;
extern FPTraceSQLSetConnectAttrW	fpTraceSQLSetConnectAttrW;
extern FPTraceSQLGetConnectAttrW	fpTraceSQLGetConnectAttrW;
extern FPTraceSQLSetStmtAttrW		fpTraceSQLSetStmtAttrW;
extern FPTraceSQLGetStmtAttrW		fpTraceSQLGetStmtAttrW;
extern FPTraceSQLGetInfoW			fpTraceSQLGetInfoW;
extern FPTraceSQLSetDescFieldW		fpTraceSQLSetDescFieldW;
extern FPTraceSQLGetDescFieldW		fpTraceSQLGetDescFieldW;
extern FPTraceSQLGetDescRecW		fpTraceSQLGetDescRecW;
extern FPTraceSQLBrowseConnectW	fpTraceSQLBrowseConnectW;
extern FPTraceSQLDriverConnectW	fpTraceSQLDriverConnectW;
extern FPTraceSQLPrepareW			fpTraceSQLPrepareW;
extern FPTraceSQLExecDirectW		fpTraceSQLExecDirectW;
extern FPTraceSQLDescribeColW		fpTraceSQLDescribeColW;
extern FPTraceSQLTablesW			fpTraceSQLTablesW;
extern FPTraceSQLColumnsW			fpTraceSQLColumnsW;
extern FPTraceSQLSpecialColumnsW	fpTraceSQLSpecialColumnsW;
extern FPTraceSQLPrimaryKeysW		fpTraceSQLPrimaryKeysW;
extern FPTraceSQLStatisticsW		fpTraceSQLStatisticsW;
extern FPTraceSQLGetCursorNameW	fpTraceSQLGetCursorNameW;
extern FPTraceSQLSetCursorNameW	fpTraceSQLSetCursorNameW;
extern FPTraceSQLNativeSqlW		fpTraceSQLNativeSqlW;
extern FPTraceSQLColAttributeW		fpTraceSQLColAttributeW;
extern FPTraceSQLProceduresW        fpTraceSQLProceduresW;
extern FPTraceSQLProcedureColumnsW  fpTraceSQLProcedureColumnsW;
extern FPTraceSQLColumnPrivilegesW	fpTraceSQLColumnPrivilegesW;
extern FPTraceSQLTablePrivilegesW	fpTraceSQLTablePrivilegesW;
extern FPTraceSQLForeignKeysW		fpTraceSQLForeignKeysW;




#endif
