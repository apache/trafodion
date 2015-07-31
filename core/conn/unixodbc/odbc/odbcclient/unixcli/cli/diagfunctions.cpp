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
**************************************************************************/
//
//

#include "drvrglobal.h"
#include "diagfunctions.h"
#include "odbcinst.h"
#include "stubtrace.h"

#ifdef MXOSS
#include <assert.h>
#endif

extern  CRITICAL_SECTION2 g_csWrite;

LPCSTR	szTraceFlagsKey=			"TraceFlags";
#ifdef unixcli
LPCSTR	szDefaultTraceFlags=  		"1";
#endif
LPCSTR	szTraceDllKey=				"TraceDll";
LPCSTR	szDefaultTraceDll=  		"Tdm_OdbcTrace";
LPCSTR	szODBC=						"ODBC";
	LPCSTR	szODBCIni=					"TRAFDSN";
FPTraceProcessEntry			fpTraceProcessEntry = NULL;
FPTraceDebugOut				fpTraceDebugOut = NULL;
FPTracePrintMarker			fpTracePrintMarker = NULL;
FPTraceFirstEntry			fpTraceFirstEntry = NULL;
FPTraceReturn				fpTraceReturn = NULL;
FPTraceError				fpTraceError = NULL;
FPTraceSQLAllocHandle		fpTraceSQLAllocHandle = NULL;		
FPTraceSQLBindCol			fpTraceSQLBindCol = NULL;
FPTraceSQLBindParameter		fpTraceSQLBindParameter = NULL;
FPTraceSQLBrowseConnect		fpTraceSQLBrowseConnect = NULL;
FPTraceSQLCancel			fpTraceSQLCancel = NULL;
FPTraceSQLCloseCursor		fpTraceSQLCloseCursor = NULL;
FPTraceSQLColAttribute		fpTraceSQLColAttribute = NULL;
FPTraceSQLColumnPrivileges	fpTraceSQLColumnPrivileges = NULL;
FPTraceSQLColumns			fpTraceSQLColumns = NULL;
FPTraceSQLConnect			fpTraceSQLConnect = NULL;
FPTraceSQLCopyDesc			fpTraceSQLCopyDesc = NULL;
FPTraceSQLDescribeCol		fpTraceSQLDescribeCol = NULL;
FPTraceSQLDescribeParam		fpTraceSQLDescribeParam = NULL;
FPTraceSQLDisconnect		fpTraceSQLDisconnect = NULL;
FPTraceSQLDriverConnect		fpTraceSQLDriverConnect = NULL;
FPTraceSQLEndTran			fpTraceSQLEndTran = NULL;
FPTraceSQLExecDirect		fpTraceSQLExecDirect = NULL;
FPTraceSQLExecute			fpTraceSQLExecute = NULL;
FPTraceSQLExtendedFetch		fpTraceSQLExtendedFetch = NULL;
FPTraceSQLFetch				fpTraceSQLFetch = NULL;
FPTraceSQLFetchScroll		fpTraceSQLFetchScroll = NULL;
FPTraceSQLForeignKeys		fpTraceSQLForeignKeys = NULL;
FPTraceSQLFreeHandle		fpTraceSQLFreeHandle = NULL;
FPTraceSQLFreeStmt			fpTraceSQLFreeStmt = NULL;
FPTraceSQLGetConnectAttr	fpTraceSQLGetConnectAttr = NULL;
FPTraceSQLGetCursorName		fpTraceSQLGetCursorName = NULL;
FPTraceSQLGetData			fpTraceSQLGetData = NULL;
FPTraceSQLGetDescField		fpTraceSQLGetDescField = NULL;
FPTraceSQLGetDescRec		fpTraceSQLGetDescRec = NULL;
FPTraceSQLGetDiagField		fpTraceSQLGetDiagField = NULL;
FPTraceSQLGetDiagRec		fpTraceSQLGetDiagRec = NULL;
FPTraceSQLGetEnvAttr		fpTraceSQLGetEnvAttr = NULL;
FPTraceSQLGetInfo			fpTraceSQLGetInfo = NULL;
FPTraceSQLGetStmtAttr		fpTraceSQLGetStmtAttr = NULL;
FPTraceSQLGetTypeInfo		fpTraceSQLGetTypeInfo = NULL;
FPTraceSQLMoreResults		fpTraceSQLMoreResults = NULL;
FPTraceSQLNativeSql			fpTraceSQLNativeSql = NULL;
FPTraceSQLNumParams			fpTraceSQLNumParams = NULL;
FPTraceSQLNumResultCols		fpTraceSQLNumResultCols = NULL;
FPTraceSQLParamData			fpTraceSQLParamData = NULL;
FPTraceSQLPrepare			fpTraceSQLPrepare = NULL;
FPTraceSQLPrimaryKeys		fpTraceSQLPrimaryKeys = NULL;
FPTraceSQLPutData			fpTraceSQLPutData = NULL;
FPTraceSQLRowCount			fpTraceSQLRowCount = NULL;
FPTraceSQLSetConnectAttr	fpTraceSQLSetConnectAttr = NULL;
FPTraceSQLSetCursorName		fpTraceSQLSetCursorName = NULL;
FPTraceSQLSetDescField		fpTraceSQLSetDescField = NULL;
FPTraceSQLSetDescRec		fpTraceSQLSetDescRec = NULL;
FPTraceSQLSetEnvAttr		fpTraceSQLSetEnvAttr = NULL;
FPTraceSQLSetPos			fpTraceSQLSetPos = NULL;
FPTraceSQLSetStmtAttr		fpTraceSQLSetStmtAttr = NULL;
FPTraceSQLSpecialColumns	fpTraceSQLSpecialColumns = NULL;
FPTraceSQLStatistics		fpTraceSQLStatistics = NULL;
FPTraceSQLTablePrivileges	fpTraceSQLTablePrivileges = NULL;
FPTraceSQLTables			fpTraceSQLTables = NULL;
FPTraceSQLProcedures        fpTraceSQLProcedures = NULL;
FPTraceSQLProcedureColumns  fpTraceSQLProcedureColumns = NULL;
VERSION_def**				fpTraceASVersion = NULL;
VERSION_def**				fpTraceSrvrVersion = NULL;
VERSION_def**				fpTraceSqlVersion = NULL;
FPTraceTransportIn			fpTraceTransportIn = NULL;
FPTraceTransportOut			fpTraceTransportOut = NULL;


static char *TraceOptionString[] =
    {	"",					/* 0x00000001   TRACE ON/OFF*/
		"ERROR           ",	/* 0x00000002   */
		"WARNING         ",	/* 0x00000004	*/
		"CONFIG 		 ",	/* 0x00000008   */
		"INFO            ",	/* 0x00000010   */
		"DEBUG           ",	/* 0x00000020   */
		"TRANSPORT       ",	/* 0x00000040   */
		"UNKNOWN08",    	/* 0x00000080   */
		"UNKNOWN09",    	/* 0x00000100   */
		"UNKNOWN10",    	/* 0x00000200   */
		"UNKNOWN11",		/* 0x00000400   */
		"UNKNOWN12",		/* 0x00000800   */
		"UNKNOWN13",		/* 0x00001000   */
		"UNKNOWN14",		/* 0x00002000   */
		"UNKNOWN15",		/* 0x00004000   */
		"UNKNOWN16"			/* 0x00008000   */
    };

#define DEBUG_BUFFER_SIZE	4096 	    /* Maximum string length */


typedef enum FORMAT_OPTIONS
{
	FORMAT_TEXT,
	FORMAT_DUMP
} FORMAT_OPTIONS;

char *TraceOptionToString(long TraceOption)
{
	long index;

	if (TraceOption == 0) 
		index = 15;	    
	else 
		for (index = 0; (TraceOption & 1) == 0; TraceOption >>= 1, index++);
	return TraceOptionString[index];
}

BOOL IsTraceLibrary(){
//	if (GetModuleHandle(TRACE_DLL_NAME) == NULL) return FALSE;
	return true;
}

void TraceOut(long TraceOption, char *text, ...)
{
	char buffer[DEBUG_BUFFER_SIZE];
	char *p;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TraceOption))
		{
			if (fpTraceDebugOut)
			{
                                va_list p;
                                va_start(p,text);
                                vsnprintf(buffer, DEBUG_BUFFER_SIZE, text, p);
                                va_end(p);
				fpTraceDebugOut(strlen(buffer), buffer, TraceOptionToString(TraceOption), FORMAT_TEXT);
			}
		}
	}
	else
		RESET_TRACE()
	return;
}

void HexOut(long TraceOption, long len, void* buffer, char *text )
{
	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TraceOption))
		{
			if (fpTraceDebugOut)
			{
                                if( buffer != NULL && text != NULL && len > 0)
                                {
                                   fpTraceDebugOut(len, (char*)buffer, text, FORMAT_DUMP);
				}
			}
		}
	}
	else
		RESET_TRACE()
	return;
}

extern void SRVR_HexOut(long TraceOption, long* len, void* buffer, char *text )
{
	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TraceOption))
		{
			if (fpTraceDebugOut)
			{
				if( buffer != NULL && len != NULL && text != NULL)
				{
					if( *len > 0 )
					{
						fpTraceDebugOut(*len % 500, (char*)buffer, text, FORMAT_DUMP);
					}
				}
			}
		}
	}
	else
		RESET_TRACE()
	return;
}

void InitializeTrace()
{
	short pin;
	short pHandle[10];

	if (g_Environment.trace_pin != 0)
		return;

	EnterCriticalSection2(&g_csWrite);

	// check the trace_pin again to see if the initialization has already been 
	// completed while we were waiting to acquire the lock

	if (g_Environment.trace_pin != 0)
	{
		LeaveCriticalSection2(&g_csWrite);
		return;
	}

	g_Environment.SetEnvironment();

	fpTraceProcessEntry = (FPTraceProcessEntry) TraceProcessEntry;
	fpTraceDebugOut = (FPTraceDebugOut) TraceDebugOut;
	fpTracePrintMarker = (FPTracePrintMarker) TracePrintMarker;
	fpTraceFirstEntry = (FPTraceFirstEntry) TraceFirstEntry;
	fpTraceReturn = (FPTraceReturn) TraceReturn_P;
	fpTraceError = (FPTraceError) TraceError;
	fpTraceSQLAllocHandle = (FPTraceSQLAllocHandle) TraceSQLAllocHandle;
	fpTraceSQLBindCol = (FPTraceSQLBindCol) TraceSQLBindCol;
	fpTraceSQLBindParameter = (FPTraceSQLBindParameter) TraceSQLBindParameter;
	fpTraceSQLBrowseConnect = (FPTraceSQLBrowseConnect) TraceTdmSQLBrowseConnect;
	fpTraceSQLCancel = (FPTraceSQLCancel) TraceSQLCancel;
	fpTraceSQLCloseCursor = (FPTraceSQLCloseCursor) TraceSQLCloseCursor;
	fpTraceSQLColAttribute = (FPTraceSQLColAttribute) TraceSQLColAttribute;
	fpTraceSQLColumnPrivileges = (FPTraceSQLColumnPrivileges) TraceSQLColumnPrivileges;
	fpTraceSQLColumns = (FPTraceSQLColumns) TraceSQLColumns;
	fpTraceSQLConnect = (FPTraceSQLConnect) TraceSQLConnect;
	fpTraceSQLCopyDesc = (FPTraceSQLCopyDesc) TraceSQLCopyDesc;
	fpTraceSQLDescribeCol = (FPTraceSQLDescribeCol) TraceSQLDescribeCol;
	fpTraceSQLDescribeParam = (FPTraceSQLDescribeParam) TraceSQLDescribeParam;
	fpTraceSQLDisconnect = (FPTraceSQLDisconnect) TraceSQLDisconnect;
	fpTraceSQLDriverConnect = (FPTraceSQLDriverConnect) TraceTdmSQLDriverConnect;
	fpTraceSQLEndTran = (FPTraceSQLEndTran) TraceSQLEndTran;
	fpTraceSQLExecDirect = (FPTraceSQLExecDirect) TraceSQLExecDirect;
	fpTraceSQLExecute = (FPTraceSQLExecute) TraceSQLExecute;
	fpTraceSQLExtendedFetch = (FPTraceSQLExtendedFetch) TraceSQLExtendedFetch;
	fpTraceSQLFetch = (FPTraceSQLFetch) TraceSQLFetch;
	fpTraceSQLFetchScroll = (FPTraceSQLFetchScroll) TraceSQLFetchScroll;
	fpTraceSQLForeignKeys = (FPTraceSQLForeignKeys) TraceSQLForeignKeys;
	fpTraceSQLFreeHandle = (FPTraceSQLFreeHandle) TraceSQLFreeHandle;
	fpTraceSQLFreeStmt = (FPTraceSQLFreeStmt) TraceSQLFreeStmt;
	fpTraceSQLGetConnectAttr = (FPTraceSQLGetConnectAttr) TraceSQLGetConnectAttr;
	fpTraceSQLGetCursorName = (FPTraceSQLGetCursorName) TraceSQLGetCursorName;
	fpTraceSQLGetData = (FPTraceSQLGetData) TraceSQLGetData;
	fpTraceSQLGetDescField = (FPTraceSQLGetDescField) TraceSQLGetDescField;
	fpTraceSQLGetDescRec = (FPTraceSQLGetDescRec) TraceSQLGetDescRec;
	fpTraceSQLGetDiagField = (FPTraceSQLGetDiagField) TraceSQLGetDiagField;
	fpTraceSQLGetDiagRec = (FPTraceSQLGetDiagRec) TraceSQLGetDiagRec;
	fpTraceSQLGetEnvAttr = (FPTraceSQLGetEnvAttr) TraceSQLGetEnvAttr;
	fpTraceSQLGetInfo = (FPTraceSQLGetInfo) TraceSQLGetInfo;
	fpTraceSQLGetStmtAttr = (FPTraceSQLGetStmtAttr) TraceSQLGetStmtAttr;
	fpTraceSQLGetTypeInfo = (FPTraceSQLGetTypeInfo) TraceSQLGetTypeInfo;
	fpTraceSQLMoreResults = (FPTraceSQLMoreResults) TraceSQLMoreResults;
	fpTraceSQLNativeSql = (FPTraceSQLNativeSql) TraceSQLNativeSql;
	fpTraceSQLNumParams = (FPTraceSQLNumParams) TraceSQLNumParams;
	fpTraceSQLNumResultCols = (FPTraceSQLNumResultCols) TraceSQLNumResultCols;
	fpTraceSQLParamData = (FPTraceSQLParamData) TraceSQLParamData;
	fpTraceSQLPrepare = (FPTraceSQLPrepare) TraceSQLPrepare;
	fpTraceSQLPrimaryKeys = (FPTraceSQLPrimaryKeys) TraceSQLPrimaryKeys;
	fpTraceSQLPutData = (FPTraceSQLPutData) TraceSQLPutData;
	fpTraceSQLRowCount = (FPTraceSQLRowCount) TraceSQLRowCount;
	fpTraceSQLSetConnectAttr = (FPTraceSQLSetConnectAttr) TraceSQLSetConnectAttr;
	fpTraceSQLSetCursorName = (FPTraceSQLSetCursorName) TraceSQLSetCursorName;
	fpTraceSQLSetDescField = (FPTraceSQLSetDescField) TraceSQLSetDescField;
	fpTraceSQLSetDescRec = (FPTraceSQLSetDescRec) TraceSQLSetDescRec;
	fpTraceSQLSetEnvAttr = (FPTraceSQLSetEnvAttr) TraceSQLSetEnvAttr;
	fpTraceSQLSetPos = (FPTraceSQLSetPos) TraceSQLSetPos;
	fpTraceSQLSetStmtAttr = (FPTraceSQLSetStmtAttr) TraceSQLSetStmtAttr;
	fpTraceSQLSpecialColumns = (FPTraceSQLSpecialColumns) TraceSQLSpecialColumns;
	fpTraceSQLStatistics = (FPTraceSQLStatistics) TraceSQLStatistics;
	fpTraceSQLTablePrivileges = (FPTraceSQLTablePrivileges) TraceSQLTablePrivileges;
	fpTraceSQLTables = (FPTraceSQLTables) TraceSQLTables;
	fpTraceASVersion = (VERSION_def**) &ASVersion;
	fpTraceSrvrVersion = (VERSION_def**) &SrvrVersion;
	fpTraceSqlVersion = (VERSION_def**) &SqlVersion;
	fpTraceTransportIn = (FPTraceTransportIn) TraceTransportIn;
	fpTraceTransportOut = (FPTraceTransportOut) TraceTransportOut;

	if(fpTraceProcessEntry)
	{
		pdwGlobalTraceVariable = fpTraceProcessEntry();
	}

	LeaveCriticalSection2(&g_csWrite);
}

static char g_ElapsedTime[50];
char *ElapsedTimeString(struct _timeb StartTime)
{
   struct _timeb CurrentTime;
	long ElapsedSeconds;
	short ElapsedMilliSec;

   _ftime(&CurrentTime);

	ElapsedSeconds=(long)((long)CurrentTime.time-(long)StartTime.time);
	ElapsedMilliSec=(signed short)(CurrentTime.millitm-StartTime.millitm);
	if(ElapsedMilliSec<0){
		ElapsedSeconds--;
		ElapsedMilliSec+=1000;
		}

	if (ElapsedSeconds > 0)
		sprintf(g_ElapsedTime,"%ld.%03d sec<----------",ElapsedSeconds,ElapsedMilliSec);
	else
		sprintf(g_ElapsedTime,"%ld.%03d sec",ElapsedSeconds,ElapsedMilliSec);

	return g_ElapsedTime;
}
void LogInfo(VERSION_def* ASVersion, VERSION_def* SrvrVersion,VERSION_def* SqlVersion)
{
	if(fpTraceASVersion != NULL) *fpTraceASVersion = ASVersion;
	if(fpTraceSrvrVersion != NULL) *fpTraceSrvrVersion = SrvrVersion;
	if(fpTraceSqlVersion != NULL) *fpTraceSqlVersion = SqlVersion;

	if(fpTraceFirstEntry)
#ifdef NSK_DRIVER
		fpTraceFirstEntry("GUARDIAN ODBC DRIVER");
#elif unixcli
#ifdef MXHPUXIA
	#ifdef __LP64__
		fpTraceFirstEntry("HPUX IA64 ODBC DRIVER");
	#else
		fpTraceFirstEntry("HPUX IA32 ODBC DRIVER");
	#endif
#elif MXHPUXPA
	#ifdef __LP64__
		fpTraceFirstEntry("HPUX PA64 ODBC DRIVER");
	#else
		fpTraceFirstEntry("HPUX PA32 ODBC DRIVER");
	#endif
#elif MXOSS
		fpTraceFirstEntry("OSS ODBC DRIVER");
#elif MXLINUX
	#ifdef __LP64__
		fpTraceFirstEntry("LINUX 64 ODBC DRIVER");
	#else
		fpTraceFirstEntry("LINUX 32 ODBC DRIVER");
	#endif
#elif MXIA64LINUX
		fpTraceFirstEntry("LINUX IA64 ODBC DRIVER");
#elif MXAIX
	#ifdef __LP64__
		fpTraceFirstEntry("AIX 64 ODBC DRIVER");
	#else
		fpTraceFirstEntry("AIX 32 ODBC DRIVER");
	#endif
#elif MXSUNSPARC
	#ifdef __LP64__
		fpTraceFirstEntry("SUN SPARC 64 ODBC DRIVER");
	#else
		fpTraceFirstEntry("SUN SPARC 32 ODBC DRIVER");
	#endif
#else
		fpTraceFirstEntry("HPODBC DRIVER");
#endif
#else
		fpTraceFirstEntry("OSS ODBC DRIVER");
#endif
}

void TRACE_TRANSPORT_IN(int operation, char* reference, void* prheader, char* rbuffer, long tcount, long timeout)
{
	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_TRANSPORT))
		{
			if (fpTraceTransportIn)
				fpTraceTransportIn(operation, reference, prheader, rbuffer, tcount, timeout);
		}
	}
	else
		RESET_TRACE()
}

void TRACE_TRANSPORT_OUT(int operation, char* reference, void* pwheader, char* wbuffer, long tcount, long timeout)
{
	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_TRANSPORT))
		{
			if (fpTraceTransportOut)
				fpTraceTransportOut(operation, reference, pwheader, wbuffer, tcount, timeout);
		}
	}
	else
		RESET_TRACE()
}
