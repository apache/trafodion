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
#include "diagfunctions.h"
#include "odbcinst.h"

LPCSTR	szTraceFlagsKey=			"TraceFlags";
LPCSTR	szDefaultTraceFlags=  		"0";
LPCSTR	szTraceDllKey=				"TraceDll";
LPCSTR	szDefaultTraceDll=  		"Tdm_OdbcTrace";
LPCSTR	szODBC=						"ODBC";
LPCSTR	szODBCIni=					"ODBC.INI";
	
FPTraceProcessEntry			fpTraceProcessEntry = NULL;
FPTraceDebugOut				fpTraceDebugOut = NULL;
FPTracePrintMarker			fpTracePrintMarker = NULL;
FPTraceFirstEntry			fpTraceFirstEntry = NULL;
FPTraceReturn				fpTraceReturn = NULL;
FPTraceSQLAllocHandle		fpTraceSQLAllocHandle = NULL;		
FPTraceSQLBindCol			fpTraceSQLBindCol = NULL;
FPTraceSQLBindParameter		fpTraceSQLBindParameter = NULL;
FPTraceSQLCancel			fpTraceSQLCancel = NULL;
FPTraceSQLCloseCursor		fpTraceSQLCloseCursor = NULL;
FPTraceSQLCopyDesc			fpTraceSQLCopyDesc = NULL;
FPTraceSQLDescribeParam		fpTraceSQLDescribeParam = NULL;
FPTraceSQLDisconnect		fpTraceSQLDisconnect = NULL;
FPTraceSQLEndTran			fpTraceSQLEndTran = NULL;
FPTraceSQLExecute			fpTraceSQLExecute = NULL;
FPTraceSQLExtendedFetch		fpTraceSQLExtendedFetch = NULL;
FPTraceSQLFetch				fpTraceSQLFetch = NULL;
FPTraceSQLFetchScroll		fpTraceSQLFetchScroll = NULL;
FPTraceSQLFreeHandle		fpTraceSQLFreeHandle = NULL;
FPTraceSQLFreeStmt			fpTraceSQLFreeStmt = NULL;
FPTraceSQLGetData			fpTraceSQLGetData = NULL;
FPTraceSQLGetEnvAttr		fpTraceSQLGetEnvAttr = NULL;
FPTraceSQLGetTypeInfo		fpTraceSQLGetTypeInfo = NULL;
FPTraceSQLMoreResults		fpTraceSQLMoreResults = NULL;
FPTraceSQLNumParams			fpTraceSQLNumParams = NULL;
FPTraceSQLNumResultCols		fpTraceSQLNumResultCols = NULL;
FPTraceSQLParamData			fpTraceSQLParamData = NULL;
FPTraceSQLPutData			fpTraceSQLPutData = NULL;
FPTraceSQLRowCount			fpTraceSQLRowCount = NULL;
FPTraceSQLSetEnvAttr		fpTraceSQLSetEnvAttr = NULL;
FPTraceSQLSetPos			fpTraceSQLSetPos = NULL;
VERSION_def**				fpTraceASVersion = NULL;
VERSION_def**				fpTraceSrvrVersion = NULL;
VERSION_def**				fpTraceSqlVersion = NULL;
FPTraceTransportIn			fpTraceTransportIn = NULL;
FPTraceTransportOut			fpTraceTransportOut = NULL;
FPTraceSQLSetDescRec		fpTraceSQLSetDescRec = NULL;

//Unicode functions
FPTraceSQLGetDiagRecW		fpTraceSQLGetDiagRecW = NULL;
FPTraceSQLGetDiagFieldW		fpTraceSQLGetDiagFieldW = NULL;
FPTraceSQLConnectW			fpTraceSQLConnectW = NULL;
FPTraceSQLSetConnectAttrW	fpTraceSQLSetConnectAttrW = NULL;
FPTraceSQLGetConnectAttrW	fpTraceSQLGetConnectAttrW = NULL;
FPTraceSQLSetStmtAttrW		fpTraceSQLSetStmtAttrW = NULL;
FPTraceSQLGetStmtAttrW		fpTraceSQLGetStmtAttrW = NULL;
FPTraceSQLGetInfoW			fpTraceSQLGetInfoW = NULL;
FPTraceSQLSetDescFieldW		fpTraceSQLSetDescFieldW = NULL;
FPTraceSQLGetDescFieldW		fpTraceSQLGetDescFieldW = NULL;
FPTraceSQLGetDescRecW		fpTraceSQLGetDescRecW = NULL;
FPTraceSQLBrowseConnectW	fpTraceSQLBrowseConnectW = NULL;
FPTraceSQLDriverConnectW	fpTraceSQLDriverConnectW = NULL;
FPTraceSQLPrepareW			fpTraceSQLPrepareW = NULL;
FPTraceSQLExecDirectW		fpTraceSQLExecDirectW = NULL;
FPTraceSQLDescribeColW		fpTraceSQLDescribeColW = NULL;
FPTraceSQLTablesW			fpTraceSQLTablesW = NULL;
FPTraceSQLColumnsW			fpTraceSQLColumnsW = NULL;
FPTraceSQLSpecialColumnsW	fpTraceSQLSpecialColumnsW = NULL;
FPTraceSQLPrimaryKeysW		fpTraceSQLPrimaryKeysW = NULL;
FPTraceSQLStatisticsW		fpTraceSQLStatisticsW = NULL;
FPTraceSQLGetCursorNameW	fpTraceSQLGetCursorNameW = NULL;
FPTraceSQLSetCursorNameW	fpTraceSQLSetCursorNameW = NULL;
FPTraceSQLNativeSqlW		fpTraceSQLNativeSqlW = NULL;
FPTraceSQLColAttributeW		fpTraceSQLColAttributeW = NULL;
FPTraceSQLProceduresW        fpTraceSQLProceduresW = NULL;
FPTraceSQLProcedureColumnsW  fpTraceSQLProcedureColumnsW = NULL;
FPTraceSQLColumnPrivilegesW	fpTraceSQLColumnPrivilegesW = NULL;
FPTraceSQLTablePrivilegesW	fpTraceSQLTablePrivilegesW = NULL;
FPTraceSQLForeignKeysW		fpTraceSQLForeignKeysW = NULL;


char *TraceOptionString[] =
{	
	"",					/* 0x00000001   TRACE ON/OFF*/
	"ODBC_API     ",	/* 0x00000002   */
	"TRANSPORT		 ",	/* 0x00000004   */
};

#define DEBUG_BUFFER_SIZE	2560 	    /* Maximum string length */


typedef enum FORMAT_OPTIONS
{
	FORMAT_TEXT,
	FORMAT_DUMP
} FORMAT_OPTIONS;

char *TraceOptionToString(long TraceOption)
{
	long index;

	if (TraceOption == 0) 
            index = sizeof(TraceOptionString) - 1;	    
	else 
            for (index = 0; 
                 (TraceOption & 1) == 0 && index < sizeof(TraceOptionString);  
                 TraceOption >>= 1, index++);
	return TraceOptionString[index];
}

BOOL IsTraceLibrary(){
	if (GetModuleHandle(TRACE_DLL_NAME) == NULL) return FALSE;
	return true;
}

void TraceOut(long TraceOption, char *text, ...)
{
	char buffer[DEBUG_BUFFER_SIZE*2]; // double the size to 4096
	char *p;
	int  rc = 0;

	if (IsTraceLibrary())
	{
		if (pdwGlobalTraceVariable == NULL) 
			InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TraceOption))
		{
			if (fpTraceDebugOut)
			{
				p = (char *)&text + (long) sizeof (char *);   // Get VA_ARG pointer    
				rc = _vsnprintf(buffer, DEBUG_BUFFER_SIZE*2, text, p);
				if (rc == -1) /* if truncation happened, errno will be set to 34 */
					errno = 0;/* need to reset it back to 0, otherwise ctosqlconv may fail when */
				              /* converting numeric due to errno already set */
			
				fpTraceDebugOut(strlen(buffer), buffer, TraceOptionToString(TraceOption), FORMAT_TEXT);
			}
		}
	}
	else
		RESET_TRACE()
	return;
}

void HexOut(long TraceOption, SQLLEN* len, void* buffer, char *text )
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

	//Read Trace Flag and TraceDLL
	char	szTraceFlags[100];
	char	szTraceDll[_MAX_PATH+1];

	UWORD   wConfigMode;

	if (g_hTraceDLL != NULL && GetModuleHandle(TRACE_DLL_NAME) == g_hTraceDLL){
		if(fpTraceProcessEntry)
			pdwGlobalTraceVariable = fpTraceProcessEntry();
		return;
	}
	
	SQLGetConfigMode( &wConfigMode);
	SQLSetConfigMode( ODBC_USER_DSN);
	SQLGetPrivateProfileString(szODBC, 
								szTraceDllKey, 
								szDefaultTraceDll,
								szTraceDll, 
								sizeof(szTraceDll), 
								szODBCIni);
//
// ODBC DM 3.52 works only with HKEY_CURRENT_USER
//
/*
	if ((g_hTraceDLL = GetModuleHandle(szTraceDll)) == NULL)
	{
		SQLSetConfigMode( ODBC_SYSTEM_DSN);
		SQLGetPrivateProfileString(szODBC, 
									szTraceDllKey, 
									szDefaultTraceDll,
									szTraceDll, 
									sizeof(szTraceDll), 
									szODBCIni);
		if ((g_hTraceDLL = GetModuleHandle(szTraceDll)) == NULL)
		{
			SQLSetConfigMode( wConfigMode);
			return;
		}
	}
*/
	SQLGetPrivateProfileString(szODBC, 
								szTraceFlagsKey, 
								szDefaultTraceFlags,
								szTraceFlags, 
								sizeof(szTraceFlags), 
								szODBCIni);
		
	SQLSetConfigMode( wConfigMode);

	gTraceFlags = atol(szTraceFlags);

	if ((g_hTraceDLL = GetModuleHandle(TRACE_DLL_NAME)) != NULL)
	{
		fpTraceProcessEntry = (FPTraceProcessEntry) GetProcAddress(g_hTraceDLL, "TraceProcessEntry");
		fpTraceDebugOut = (FPTraceDebugOut) GetProcAddress(g_hTraceDLL, "TraceDebugOut");
		fpTracePrintMarker = (FPTracePrintMarker) GetProcAddress(g_hTraceDLL, "TracePrintMarker");
		fpTraceFirstEntry = (FPTraceFirstEntry) GetProcAddress(g_hTraceDLL, "TraceFirstEntry");
		fpTraceReturn = (FPTraceReturn) GetProcAddress(g_hTraceDLL, "TraceReturn");
		fpTraceSQLAllocHandle = (FPTraceSQLAllocHandle) GetProcAddress(g_hTraceDLL, "TraceSQLAllocHandle");
		fpTraceSQLBindCol = (FPTraceSQLBindCol) GetProcAddress(g_hTraceDLL, "TraceSQLBindCol");
		fpTraceSQLBindParameter = (FPTraceSQLBindParameter) GetProcAddress(g_hTraceDLL, "TraceSQLBindParameter");
		fpTraceSQLCancel = (FPTraceSQLCancel) GetProcAddress(g_hTraceDLL, "TraceSQLCancel");
		fpTraceSQLCloseCursor = (FPTraceSQLCloseCursor) GetProcAddress(g_hTraceDLL, "TraceSQLCloseCursor");
		fpTraceSQLCopyDesc = (FPTraceSQLCopyDesc) GetProcAddress(g_hTraceDLL, "TraceSQLCopyDesc");
		fpTraceSQLDescribeParam = (FPTraceSQLDescribeParam) GetProcAddress(g_hTraceDLL, "TraceSQLDescribeParam");
		fpTraceSQLDisconnect = (FPTraceSQLDisconnect) GetProcAddress(g_hTraceDLL, "TraceSQLDisconnect");
		fpTraceSQLEndTran = (FPTraceSQLEndTran) GetProcAddress(g_hTraceDLL, "TraceSQLEndTran");
		fpTraceSQLExecute = (FPTraceSQLExecute) GetProcAddress(g_hTraceDLL, "TraceSQLExecute");
		fpTraceSQLExtendedFetch = (FPTraceSQLExtendedFetch) GetProcAddress(g_hTraceDLL, "TraceSQLExtendedFetch");
		fpTraceSQLFetch = (FPTraceSQLFetch) GetProcAddress(g_hTraceDLL, "TraceSQLFetch");
		fpTraceSQLFetchScroll = (FPTraceSQLFetchScroll) GetProcAddress(g_hTraceDLL, "TraceSQLFetchScroll");
		fpTraceSQLFreeHandle = (FPTraceSQLFreeHandle) GetProcAddress(g_hTraceDLL, "TraceSQLFreeHandle");
		fpTraceSQLFreeStmt = (FPTraceSQLFreeStmt) GetProcAddress(g_hTraceDLL, "TraceSQLFreeStmt");
		fpTraceSQLGetData = (FPTraceSQLGetData) GetProcAddress(g_hTraceDLL, "TraceSQLGetData");
		fpTraceSQLGetEnvAttr = (FPTraceSQLGetEnvAttr) GetProcAddress(g_hTraceDLL, "TraceSQLGetEnvAttr");
		fpTraceSQLGetTypeInfo = (FPTraceSQLGetTypeInfo) GetProcAddress(g_hTraceDLL, "TraceSQLGetTypeInfo");
		fpTraceSQLMoreResults = (FPTraceSQLMoreResults) GetProcAddress(g_hTraceDLL, "TraceSQLMoreResults");
		fpTraceSQLNumParams = (FPTraceSQLNumParams) GetProcAddress(g_hTraceDLL, "TraceSQLNumParams");
		fpTraceSQLNumResultCols = (FPTraceSQLNumResultCols) GetProcAddress(g_hTraceDLL, "TraceSQLNumResultCols");
		fpTraceSQLParamData = (FPTraceSQLParamData) GetProcAddress(g_hTraceDLL, "TraceSQLParamData");
		fpTraceSQLPutData = (FPTraceSQLPutData) GetProcAddress(g_hTraceDLL, "TraceSQLPutData");
		fpTraceSQLRowCount = (FPTraceSQLRowCount) GetProcAddress(g_hTraceDLL, "TraceSQLRowCount");
		fpTraceSQLSetEnvAttr = (FPTraceSQLSetEnvAttr) GetProcAddress(g_hTraceDLL, "TraceSQLSetEnvAttr");
		fpTraceSQLSetPos = (FPTraceSQLSetPos) GetProcAddress(g_hTraceDLL, "TraceSQLSetPos");
		fpTraceASVersion = (VERSION_def**) GetProcAddress(g_hTraceDLL, "ASVersion");
		fpTraceSrvrVersion = (VERSION_def**) GetProcAddress(g_hTraceDLL, "SrvrVersion");
		fpTraceSqlVersion = (VERSION_def**) GetProcAddress(g_hTraceDLL, "SqlVersion");
		fpTraceTransportIn = (FPTraceTransportIn) GetProcAddress(g_hTraceDLL, "TraceTransportIn");
		fpTraceTransportOut = (FPTraceTransportOut) GetProcAddress(g_hTraceDLL, "TraceTransportOut");
		fpTraceSQLSetDescRec = (FPTraceSQLSetDescRec) GetProcAddress(g_hTraceDLL, "TraceSQLSetDescRec");
		//Unicode functions
		fpTraceSQLGetDiagRecW = (FPTraceSQLGetDiagRecW) GetProcAddress(g_hTraceDLL, "TraceSQLGetDiagRecW");
		fpTraceSQLGetDiagFieldW = (FPTraceSQLGetDiagFieldW) GetProcAddress(g_hTraceDLL, "TraceSQLGetDiagFieldW");
		fpTraceSQLConnectW = (FPTraceSQLConnectW) GetProcAddress(g_hTraceDLL, "TraceSQLConnectW");
		fpTraceSQLSetConnectAttrW = (FPTraceSQLSetConnectAttrW) GetProcAddress(g_hTraceDLL, "TraceSQLSetConnectAttrW");
		fpTraceSQLGetConnectAttrW = (FPTraceSQLGetConnectAttrW) GetProcAddress(g_hTraceDLL, "TraceSQLGetConnectAttrW");
		fpTraceSQLSetStmtAttrW = (FPTraceSQLSetStmtAttrW) GetProcAddress(g_hTraceDLL, "TraceSQLSetStmtAttrW");
		fpTraceSQLGetStmtAttrW = (FPTraceSQLGetStmtAttrW) GetProcAddress(g_hTraceDLL, "TraceSQLGetStmtAttrW");
		fpTraceSQLGetInfoW = (FPTraceSQLGetInfoW) GetProcAddress(g_hTraceDLL, "TraceSQLGetInfoW");
		fpTraceSQLSetDescFieldW = (FPTraceSQLSetDescFieldW) GetProcAddress(g_hTraceDLL, "TraceSQLSetDescFieldW");
		fpTraceSQLGetDescFieldW = (FPTraceSQLGetDescFieldW) GetProcAddress(g_hTraceDLL, "TraceSQLGetDescFieldW");
		fpTraceSQLGetDescRecW = (FPTraceSQLGetDescRecW) GetProcAddress(g_hTraceDLL, "TraceSQLGetDescRecW");
		fpTraceSQLBrowseConnectW = (FPTraceSQLBrowseConnectW) GetProcAddress(g_hTraceDLL, "TraceSQLBrowseConnectW");
		fpTraceSQLDriverConnectW = (FPTraceSQLDriverConnectW) GetProcAddress(g_hTraceDLL, "TraceSQLDriverConnectW");
		fpTraceSQLPrepareW = (FPTraceSQLPrepareW) GetProcAddress(g_hTraceDLL, "TraceSQLPrepareW");
		fpTraceSQLExecDirectW = (FPTraceSQLExecDirectW) GetProcAddress(g_hTraceDLL, "TraceSQLExecDirectW");
		fpTraceSQLDescribeColW = (FPTraceSQLDescribeColW) GetProcAddress(g_hTraceDLL, "TraceSQLDescribeColW");
		fpTraceSQLTablesW = (FPTraceSQLTablesW) GetProcAddress(g_hTraceDLL, "TraceSQLTablesW");
		fpTraceSQLColumnsW = (FPTraceSQLColumnsW) GetProcAddress(g_hTraceDLL, "TraceSQLColumnsW");
		fpTraceSQLSpecialColumnsW = (FPTraceSQLSpecialColumnsW) GetProcAddress(g_hTraceDLL, "TraceSQLSpecialColumnsW");
		fpTraceSQLPrimaryKeysW = (FPTraceSQLPrimaryKeysW) GetProcAddress(g_hTraceDLL, "TraceSQLPrimaryKeysW");
		fpTraceSQLStatisticsW = (FPTraceSQLStatisticsW) GetProcAddress(g_hTraceDLL, "TraceSQLStatisticsW");
		fpTraceSQLGetCursorNameW = (FPTraceSQLGetCursorNameW) GetProcAddress(g_hTraceDLL, "TraceSQLGetCursorNameW");
		fpTraceSQLSetCursorNameW = (FPTraceSQLSetCursorNameW) GetProcAddress(g_hTraceDLL, "TraceSQLSetCursorNameW");
		fpTraceSQLNativeSqlW = (FPTraceSQLNativeSqlW) GetProcAddress(g_hTraceDLL, "TraceSQLNativeSqlW");
		fpTraceSQLColAttributeW = (FPTraceSQLColAttributeW) GetProcAddress(g_hTraceDLL, "TraceSQLColAttributeW");
		fpTraceSQLProceduresW = (FPTraceSQLProceduresW) GetProcAddress(g_hTraceDLL, "TraceSQLProceduresW");
		fpTraceSQLProcedureColumnsW = (FPTraceSQLProcedureColumnsW) GetProcAddress(g_hTraceDLL, "TraceSQLProcedureColumnsW");
		fpTraceSQLColumnPrivilegesW = (FPTraceSQLColumnPrivilegesW) GetProcAddress(g_hTraceDLL, "TraceSQLColumnPrivilegesW");
		fpTraceSQLTablePrivilegesW = (FPTraceSQLTablePrivilegesW) GetProcAddress(g_hTraceDLL, "TraceSQLTablePrivilegesW");
		fpTraceSQLForeignKeysW = (FPTraceSQLForeignKeysW) GetProcAddress(g_hTraceDLL, "TraceSQLForeignKeysW");
		




	}
	if(fpTraceProcessEntry)
		pdwGlobalTraceVariable = fpTraceProcessEntry();
}

char g_ElapsedTime[50];
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
void LogFile( char* txt1, char* txt2, char* txt3 )
{
	char fileName[80];
	FILE* pFile = NULL;
	char buffer[501];

	sprintf( fileName, "P%d.txt",GetCurrentProcessId());
	if((pFile = fopen( fileName, "a" )) == NULL ) return;

	if (*txt2 != 0)
	{
		strcat(buffer,txt1);
		strcat(buffer,"---------->");
		strncat(buffer,txt2,100);
		strcat(buffer,"---------->");
		strcat(buffer,txt3);
		strcat(buffer,"\n");
	}
	else
		sprintf(buffer,"%s---------->%s\n",txt1,txt3);
	fputs( buffer, pFile );

	fclose( pFile );
}

void LogInfo(VERSION_def* ASVersion, VERSION_def* SrvrVersion,VERSION_def* SqlVersion)
{
	if(fpTraceASVersion != NULL) *fpTraceASVersion = ASVersion;
	if(fpTraceSrvrVersion != NULL) *fpTraceSrvrVersion = SrvrVersion;
	if(fpTraceSqlVersion != NULL) *fpTraceSqlVersion = SqlVersion;

	if(fpTraceFirstEntry)
		fpTraceFirstEntry(DRIVER_DLL_NAME);
}

void TRACE_TRANSPORT_IN(int operation, char* reference, void* prheader, char* rbuffer, long tcount, long timeout)
{
	if (IsTraceLibrary())
	{
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_DRVR_TRANSPORT_API))
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
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_DRVR_TRANSPORT_API))
		{
			if (fpTraceTransportOut)
				fpTraceTransportOut(operation, reference, pwheader, wbuffer, tcount, timeout);
		}
	}
	else
		RESET_TRACE()
}









