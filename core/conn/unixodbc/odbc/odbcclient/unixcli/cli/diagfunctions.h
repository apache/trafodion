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
#include <sqlext.h>
#include "sqltrace.h"
#include "stubtrace.h"
#include <sys/timeb.h>

#define RESET_TRACE()\
{\
	pdwGlobalTraceVariable = NULL;\
}

extern void TraceOut(long TraceOption, char *text, ...);
extern void HexOut(long TraceOption, long len, void* buffer, char *text );
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
extern FPTraceError					fpTraceError;
extern FPTraceSQLAllocHandle		fpTraceSQLAllocHandle;		
extern FPTraceSQLBindCol			fpTraceSQLBindCol;
extern FPTraceSQLBindParameter		fpTraceSQLBindParameter;
extern FPTraceSQLBrowseConnect		fpTraceSQLBrowseConnect;
extern FPTraceSQLCancel				fpTraceSQLCancel;
extern FPTraceSQLCloseCursor		fpTraceSQLCloseCursor;
extern FPTraceSQLColAttribute		fpTraceSQLColAttribute;
extern FPTraceSQLColumnPrivileges	fpTraceSQLColumnPrivileges;
extern FPTraceSQLColumns			fpTraceSQLColumns;
extern FPTraceSQLConnect			fpTraceSQLConnect;
extern FPTraceSQLCopyDesc			fpTraceSQLCopyDesc;
extern FPTraceSQLDescribeCol		fpTraceSQLDescribeCol;
extern FPTraceSQLDescribeParam		fpTraceSQLDescribeParam;
extern FPTraceSQLDisconnect			fpTraceSQLDisconnect;
extern FPTraceSQLDriverConnect		fpTraceSQLDriverConnect;
extern FPTraceSQLEndTran			fpTraceSQLEndTran;
extern FPTraceSQLExecDirect			fpTraceSQLExecDirect;
extern FPTraceSQLExecute			fpTraceSQLExecute;
extern FPTraceSQLExtendedFetch		fpTraceSQLExtendedFetch;
extern FPTraceSQLFetch				fpTraceSQLFetch;
extern FPTraceSQLFetchScroll		fpTraceSQLFetchScroll;
extern FPTraceSQLForeignKeys		fpTraceSQLForeignKeys;
extern FPTraceSQLFreeHandle			fpTraceSQLFreeHandle;
extern FPTraceSQLFreeStmt			fpTraceSQLFreeStmt;
extern FPTraceSQLGetConnectAttr		fpTraceSQLGetConnectAttr;
extern FPTraceSQLGetCursorName		fpTraceSQLGetCursorName;
extern FPTraceSQLGetData			fpTraceSQLGetData;
extern FPTraceSQLGetDescField		fpTraceSQLGetDescField;
extern FPTraceSQLGetDescRec			fpTraceSQLGetDescRec;
extern FPTraceSQLGetDiagField		fpTraceSQLGetDiagField;
extern FPTraceSQLGetDiagRec			fpTraceSQLGetDiagRec;
extern FPTraceSQLGetEnvAttr			fpTraceSQLGetEnvAttr;
extern FPTraceSQLGetInfo			fpTraceSQLGetInfo;
extern FPTraceSQLGetStmtAttr		fpTraceSQLGetStmtAttr;
extern FPTraceSQLGetTypeInfo		fpTraceSQLGetTypeInfo;
extern FPTraceSQLMoreResults		fpTraceSQLMoreResults;
extern FPTraceSQLNativeSql			fpTraceSQLNativeSql;
extern FPTraceSQLNumParams			fpTraceSQLNumParams;
extern FPTraceSQLNumResultCols		fpTraceSQLNumResultCols;
extern FPTraceSQLParamData			fpTraceSQLParamData;
extern FPTraceSQLPrepare			fpTraceSQLPrepare;
extern FPTraceSQLPrimaryKeys		fpTraceSQLPrimaryKeys;
extern FPTraceSQLPutData			fpTraceSQLPutData;
extern FPTraceSQLRowCount			fpTraceSQLRowCount;
extern FPTraceSQLSetConnectAttr		fpTraceSQLSetConnectAttr;
extern FPTraceSQLSetCursorName		fpTraceSQLSetCursorName;
extern FPTraceSQLSetDescField		fpTraceSQLSetDescField;
extern FPTraceSQLSetDescRec			fpTraceSQLSetDescRec;
extern FPTraceSQLSetEnvAttr			fpTraceSQLSetEnvAttr;
extern FPTraceSQLSetPos				fpTraceSQLSetPos;
extern FPTraceSQLSetStmtAttr		fpTraceSQLSetStmtAttr;
extern FPTraceSQLSpecialColumns		fpTraceSQLSpecialColumns;
extern FPTraceSQLStatistics			fpTraceSQLStatistics;
extern FPTraceSQLTablePrivileges	fpTraceSQLTablePrivileges;
extern FPTraceSQLTables				fpTraceSQLTables;
extern FPTraceSQLProcedures	        fpTraceSQLProcedures;
extern FPTraceSQLProcedureColumns	fpTraceSQLProcedureColumns;
extern FPTraceTransportIn			fpTraceTransportIn;
extern FPTraceTransportOut			fpTraceTransportOut;

#endif
