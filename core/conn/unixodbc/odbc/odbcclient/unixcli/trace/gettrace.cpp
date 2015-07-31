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
#include "stubtrace.h"


RETCODE SQL_API TraceOpenLogFile(
	LPWSTR	szFileName,
	LPWSTR	lpwszOutputMsg,
	DWORD	cbOutputMsg)
{
	return SQL_SUCCESS;
}

RETCODE	SQL_API TraceCloseLogFile()
{
	return SQL_SUCCESS;
}
extern "C" VOID	SQL_API TraceReturn_P(RETCODE RetHandle, RETCODE RetCode)
{
}
extern "C" VOID	SQL_API TraceError(RETCODE	RetHandle, RETCODE RetCode)
{
}

#ifndef DISABLE_TRACE

static LPCSTR mask = "********";

void MaskUseridPassword(LPTRACESTR lpCallStr, SWORD index1 , SWORD index2, char *maskedArg)
{
   int uidProcessed, pwdProcessed = FALSE ;
   if ((char *)lpCallStr->lpvArg[index1] != NULL)
   {
      strcpy(maskedArg, (char *)lpCallStr->lpvArg[index1]); // Make a copy anyway
      if (strstr(maskedArg,";UID=") != NULL) // we have uid info
      {
         char tmpArg[2049];
         tmpArg[0]=';'; tmpArg[1]='\0';
         char* beginUser = (strstr(maskedArg,";UID=") + 5);
         if (beginUser!=NULL)
         {
            char* endUser = strchr(beginUser,';');
            if (endUser != NULL)
               strcpy(tmpArg,endUser); 
            strcpy(beginUser,mask);
            strcat(beginUser,tmpArg);
            uidProcessed = TRUE;
         }
       }
       if (strstr(maskedArg,";PWD=") != NULL) // we have the pswd info
       {
          char tmpArg[2049];
          tmpArg[0]=';'; tmpArg[1]='\0';
          char* beginPwd = (strstr(maskedArg,";PWD=") + 5);
          if (beginPwd!=NULL)
          {
             char* endPwd = strchr(beginPwd,';');
             if (endPwd != NULL)
                strcpy(tmpArg,endPwd); 
             strcpy(beginPwd,mask);
             strcat(beginPwd,tmpArg);
             pwdProcessed=TRUE;
          }
       }
       if (uidProcessed || pwdProcessed)
       { 
	  lpCallStr->lpvArg[index1] = (LPVOID) maskedArg;
	  lpCallStr->lpvArg[index2] = (LPVOID) strlen(maskedArg);
       }
  }
}
#endif
///// Trace function for SQLAllocConnect /////

extern "C" extern "C" RETCODE SQL_API TraceSQLAllocConnect  (HENV arg0,
			 HDBC * arg1)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLALLOCCONNECT;
	lpCallStr->szFuncName="SQLAllocConnect";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HENV";
	lpCallStr->atArg[0]=TYP_HENV;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="HDBC *";
	lpCallStr->atArg[1]=TYP_HDBCPTR;
	lpCallStr->nArgs = 2;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLAllocEnv /////

extern "C" extern "C" RETCODE SQL_API TraceSQLAllocEnv  (HENV * arg0)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLALLOCENV;
	lpCallStr->szFuncName="SQLAllocEnv";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HENV *";
	lpCallStr->atArg[0]=TYP_HENVPTR;
	lpCallStr->nArgs = 1;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLAllocStmt /////

extern "C" extern "C" RETCODE SQL_API TraceSQLAllocStmt  (HDBC arg0,
		 HSTMT * arg1)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLALLOCSTMT;
	lpCallStr->szFuncName="SQLAllocStmt";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="HSTMT *";
	lpCallStr->atArg[1]=TYP_HSTMTPTR;
	lpCallStr->nArgs = 2;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLBindCol /////

extern "C" extern "C" RETCODE SQL_API TraceSQLBindCol  (HSTMT arg0,
			UWORD arg1,
			SWORD arg2,
			PTR arg3,
			SDWORD_P arg4,
			UNALIGNED SDWORD_P * arg5)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLBINDCOL;
	lpCallStr->szFuncName="SQLBindCol";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="PTR";
	lpCallStr->atArg[3]=TYP_PTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SDWORD";
	lpCallStr->atArg[4]=TYP_SDWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SDWORD *";
	lpCallStr->atArg[5]=TYP_SDWORDPTR;
	lpCallStr->nArgs = 6;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLCancel /////

extern "C" extern "C" RETCODE SQL_API TraceSQLCancel  (HSTMT arg0)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLCANCEL;
	lpCallStr->szFuncName="SQLCancel";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->nArgs = 1;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLColAttributes /////

extern "C" extern "C" RETCODE SQL_API TraceSQLColAttributes  (HSTMT arg0,
		 UWORD arg1,
		 UWORD arg2,
		 PTR arg3,
		 SWORD arg4,
		 UNALIGNED SWORD * arg5,
		 UNALIGNED SDWORD_P * arg6)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLCOLATTRIBUTES;
	lpCallStr->szFuncName="SQLColAttributes";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="UWORD";
	lpCallStr->atArg[2]=TYP_UWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="PTR";
	lpCallStr->atArg[3]=TYP_PTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SWORD *";
	lpCallStr->atArg[5]=TYP_SWORDPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SDWORD *";
	lpCallStr->atArg[6]=TYP_SDWORDPTR;
	lpCallStr->nArgs = 7;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)	
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLConnect /////

extern "C" extern "C" RETCODE SQL_API TraceSQLConnect  (HDBC arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	char maskedArg[2049];
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLCONNECT;
	lpCallStr->szFuncName="SQLConnectW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	strcpy(maskedArg,mask);
	lpCallStr->lpvArg[3] = (LPVOID) maskedArg;
	lpCallStr->szArg[3]="UCHAR *";
	lpCallStr->atArg[3]=TYP_UCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) strlen(maskedArg);
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) maskedArg;
	lpCallStr->szArg[5]="UCHAR *";
	lpCallStr->atArg[5]=TYP_UCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) strlen(maskedArg);
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->nArgs = 7;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLDescribeCol /////

extern "C" extern "C" RETCODE SQL_API TraceSQLDescribeCol  (HSTMT arg0,
		 UWORD arg1,
		 UCHAR * arg2,
		 SWORD arg3,
		 SWORD * arg4,
		 UNALIGNED SWORD * arg5,
		 UNALIGNED UDWORD_P * arg6,
		 UNALIGNED SWORD * arg7,
		 UNALIGNED SWORD * arg8)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLDESCRIBECOL;
	lpCallStr->szFuncName="SQLDescribeColW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="UCHAR *";
	lpCallStr->atArg[2]=TYP_UCHARPTR;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD";
	lpCallStr->atArg[3]=TYP_SWORD;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD *";
	lpCallStr->atArg[4]=TYP_SWORDPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SWORD *";
	lpCallStr->atArg[5]=TYP_SWORDPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="UDWORD *";
	lpCallStr->atArg[6]=TYP_UDWORDPTR;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SWORD *";
	lpCallStr->atArg[7]=TYP_SWORDPTR;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="SWORD *";
	lpCallStr->atArg[8]=TYP_SWORDPTR;
	lpCallStr->nArgs = 9;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLDisconnect /////

extern "C" extern "C" RETCODE SQL_API TraceSQLDisconnect  (HDBC arg0)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLDISCONNECT;
	lpCallStr->szFuncName="SQLDisconnect";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->nArgs = 1;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLError /////

extern "C" extern "C" RETCODE SQL_API TraceSQLError  (HENV arg0,
		 HDBC arg1,
		 HSTMT arg2,
		 UCHAR * arg3,
		 UNALIGNED SDWORD_P * arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 UNALIGNED SWORD * arg7)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLERROR;
	lpCallStr->szFuncName="SQLError";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HENV";
	lpCallStr->atArg[0]=TYP_HENV;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="HDBC";
	lpCallStr->atArg[1]=TYP_HDBC;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="HSTMT";
	lpCallStr->atArg[2]=TYP_HSTMT;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="UCHAR *";
	lpCallStr->atArg[3]=TYP_UCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SDWORD *";
	lpCallStr->atArg[4]=TYP_SDWORDPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="UCHAR *";
	lpCallStr->atArg[5]=TYP_UCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SWORD *";
	lpCallStr->atArg[7]=TYP_SWORDPTR;
	lpCallStr->nArgs = 8;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLExecDirect /////

extern "C" extern "C" RETCODE SQL_API TraceSQLExecDirect  (HSTMT arg0,
		 UCHAR * arg1,
		 SDWORD_P arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLEXECDIRECT;
	lpCallStr->szFuncName="SQLExecDirectW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SDWORD";
	lpCallStr->atArg[2]=TYP_SDWORD;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLExecute /////

extern "C" extern "C" RETCODE SQL_API TraceSQLExecute  (HSTMT arg0)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLEXECUTE;
	lpCallStr->szFuncName="SQLExecute";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->nArgs = 1;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLFetch /////

extern "C" extern "C" RETCODE SQL_API TraceSQLFetch  (HSTMT arg0)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLFETCH;
	lpCallStr->szFuncName="SQLFetch";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->nArgs = 1;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLFreeConnect /////

extern "C" extern "C" RETCODE SQL_API TraceSQLFreeConnect  (HDBC arg0)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLFREECONNECT;
	lpCallStr->szFuncName="SQLFreeConnect";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->nArgs = 1;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLFreeEnv /////

extern "C" extern "C" RETCODE SQL_API TraceSQLFreeEnv  (HENV arg0)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLFREEENV;
	lpCallStr->szFuncName="SQLFreeEnv";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HENV";
	lpCallStr->atArg[0]=TYP_HENV;
	lpCallStr->nArgs = 1;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLFreeStmt /////

extern "C" extern "C" RETCODE SQL_API TraceSQLFreeStmt  (HSTMT arg0,
		 UWORD arg1)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLFREESTMT;
	lpCallStr->szFuncName="SQLFreeStmt";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->nArgs = 2;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetCursorName /////

extern "C" extern "C" RETCODE SQL_API TraceSQLGetCursorName  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UNALIGNED SWORD * arg3)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETCURSORNAME;
	lpCallStr->szFuncName="SQLGetCursorNameW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD *";
	lpCallStr->atArg[3]=TYP_SWORDPTR;
	lpCallStr->nArgs = 4;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLNumResultCols /////

extern "C" extern "C" RETCODE SQL_API TraceSQLNumResultCols  (HSTMT arg0,
		 UNALIGNED SWORD * arg1)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLNUMRESULTCOLS;
	lpCallStr->szFuncName="SQLNumResultCols";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SWORD *";
	lpCallStr->atArg[1]=TYP_SWORDPTR;
	lpCallStr->nArgs = 2;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLPrepare /////

extern "C" extern "C" RETCODE SQL_API TraceSQLPrepare  (HSTMT arg0,
		 UCHAR * arg1,
		 SDWORD_P arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLPREPARE;
	lpCallStr->szFuncName="SQLPrepareW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SDWORD";
	lpCallStr->atArg[2]=TYP_SDWORD;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLRowCount /////

extern "C" extern "C" RETCODE SQL_API TraceSQLRowCount  (HSTMT arg0,
		 UNALIGNED SDWORD_P * arg1)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLROWCOUNT;
	lpCallStr->szFuncName="SQLRowCount";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SDWORD *";
	lpCallStr->atArg[1]=TYP_SDWORDPTR;
	lpCallStr->nArgs = 2;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSetCursorName /////

extern "C" extern "C" RETCODE SQL_API TraceSQLSetCursorName  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSETCURSORNAME;
	lpCallStr->szFuncName="SQLSetCursorNameW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSetParam /////

extern "C" extern "C" RETCODE SQL_API TraceSQLSetParam  (HSTMT arg0,
		 UWORD arg1,
		 SWORD arg2,
		 SWORD arg3,
		 UDWORD_P arg4,
		 SWORD arg5,
		 PTR arg6,
		 UNALIGNED SDWORD_P * arg7)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSETPARAM;
	lpCallStr->szFuncName="SQLSetParam";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD";
	lpCallStr->atArg[3]=TYP_SWORD;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="UDWORD";
	lpCallStr->atArg[4]=TYP_UDWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SWORD";
	lpCallStr->atArg[5]=TYP_SWORD;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="PTR";
	lpCallStr->atArg[6]=TYP_PTR;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SDWORD *";
	lpCallStr->atArg[7]=TYP_SDWORDPTR;
	lpCallStr->nArgs = 8;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLTransact /////

extern "C" extern "C" RETCODE SQL_API TraceSQLTransact  (HENV arg0,
		 HDBC arg1,
		 UWORD arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLTRANSACT;
	lpCallStr->szFuncName="SQLTransact";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HENV";
	lpCallStr->atArg[0]=TYP_HENV;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="HDBC";
	lpCallStr->atArg[1]=TYP_HDBC;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="UWORD";
	lpCallStr->atArg[2]=TYP_UWORD;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLColumns /////

extern "C" extern "C" RETCODE SQL_API TraceSQLColumns  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 UCHAR * arg7,
		 SWORD arg8)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLCOLUMNS;
	lpCallStr->szFuncName="SQLColumnsW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="UCHAR *";
	lpCallStr->atArg[3]=TYP_UCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="UCHAR *";
	lpCallStr->atArg[5]=TYP_UCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="UCHAR *";
	lpCallStr->atArg[7]=TYP_UCHARPTR;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="SWORD";
	lpCallStr->atArg[8]=TYP_SWORD;
	lpCallStr->nArgs = 9;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLDriverConnect /////

extern "C" extern "C" RETCODE SQL_API TraceSQLDriverConnect  (HDBC arg0,
		 HWND arg1,
		 UCHAR * arg2,
		 SWORD arg3,
		 UCHAR * arg4,
		 SWORD arg5,
		 UNALIGNED SWORD * arg6,
		 UWORD arg7)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLDRIVERCONNECT;
	lpCallStr->szFuncName="SQLDriverConnectW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="HWND";
	lpCallStr->atArg[1]=TYP_HWND;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="UCHAR *";
	lpCallStr->atArg[2]=TYP_UCHARPTR;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD";
	lpCallStr->atArg[3]=TYP_SWORD;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="UCHAR *";
	lpCallStr->atArg[4]=TYP_UCHARPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SWORD";
	lpCallStr->atArg[5]=TYP_SWORD;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD *";
	lpCallStr->atArg[6]=TYP_SWORDPTR;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="UWORD";
	lpCallStr->atArg[7]=TYP_UWORD;
	lpCallStr->nArgs = 8;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for TdmSQLDriverConnect /////

extern "C" extern "C" RETCODE SQL_API TraceTdmSQLDriverConnect  (HDBC arg0,
		 HWND arg1,
		 UCHAR * arg2,
		 SWORD arg3,
		 UCHAR * arg4,
		 SWORD arg5,
		 UNALIGNED SWORD * arg6,
		 UWORD arg7)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	char maskedArg[2049];
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLDRIVERCONNECT;
	lpCallStr->szFuncName="SQLDriverConnectW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="HWND";
	lpCallStr->atArg[1]=TYP_HWND;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="UCHAR *";
	lpCallStr->atArg[2]=TYP_UCHARPTR;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD";
	lpCallStr->atArg[3]=TYP_SWORD;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="UCHAR *";
	lpCallStr->atArg[4]=TYP_UCHARPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SWORD";
	lpCallStr->atArg[5]=TYP_SWORD;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD *";
	lpCallStr->atArg[6]=TYP_SWORDPTR;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="UWORD";
	lpCallStr->atArg[7]=TYP_UWORD;
	lpCallStr->nArgs = 8;
	lpCallStr->dwTdmFlags = TDM_FLAG_PASSWORD; 
	MaskUseridPassword(lpCallStr,2,3,maskedArg);
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetConnectOption /////

extern "C" extern "C" RETCODE SQL_API TraceSQLGetConnectOption  (HDBC arg0,
		 UWORD arg1,
		 PTR arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETCONNECTOPTION;
	lpCallStr->szFuncName="SQLGetConnectOption";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="PTR";
	lpCallStr->atArg[2]=TYP_PTR;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetData /////

extern "C" extern "C" RETCODE SQL_API TraceSQLGetData  (HSTMT arg0,
		 UWORD arg1,
		 SWORD arg2,
		 PTR arg3,
		 SDWORD_P arg4,
		 UNALIGNED SDWORD_P * arg5)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETDATA;
	lpCallStr->szFuncName="SQLGetData";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="PTR";
	lpCallStr->atArg[3]=TYP_PTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SDWORD";
	lpCallStr->atArg[4]=TYP_SDWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SDWORD *";
	lpCallStr->atArg[5]=TYP_SDWORDPTR;
	lpCallStr->nArgs = 6;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetFunctions /////

extern "C" extern "C" RETCODE SQL_API TraceSQLGetFunctions  (HDBC arg0,
		 UWORD arg1,
		 UWORD  * arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETFUNCTIONS;
	lpCallStr->szFuncName="SQLGetFunctions";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_SQLUSMALLINT;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="UWORD *";
	lpCallStr->atArg[2]=TYP_UWORDPTR;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetInfo /////

extern "C" RETCODE SQL_API TraceSQLGetInfo  (HDBC arg0,
		 UWORD arg1,
		 PTR arg2,
		 SWORD arg3,
		 UNALIGNED SWORD * arg4)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETINFO;
	lpCallStr->szFuncName="SQLGetInfoW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	switch (arg1)
	{
	case SQL_ACCESSIBLE_PROCEDURES:
	case SQL_ACCESSIBLE_TABLES:
	case SQL_CATALOG_NAME:
	case SQL_CATALOG_NAME_SEPARATOR:
	case SQL_CATALOG_TERM:
	case SQL_COLLATION_SEQ:
	case SQL_COLUMN_ALIAS:
	case SQL_DATA_SOURCE_NAME:
	case SQL_DATA_SOURCE_READ_ONLY:
	case SQL_DATABASE_NAME:
	case SQL_DBMS_NAME:
	case SQL_DESCRIBE_PARAMETER:
	case SQL_DM_VER:
	case SQL_DRIVER_NAME:
	case SQL_DRIVER_ODBC_VER:
	case SQL_DRIVER_VER:
	case SQL_EXPRESSIONS_IN_ORDERBY:
	case SQL_IDENTIFIER_QUOTE_CHAR:
	case SQL_KEYWORDS:
	case SQL_LIKE_ESCAPE_CLAUSE:
	case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
	case SQL_MULT_RESULT_SETS:
	case SQL_MULTIPLE_ACTIVE_TXN:
	case SQL_NEED_LONG_DATA_LEN:
	case SQL_ODBC_VER:
	case SQL_ORDER_BY_COLUMNS_IN_SELECT:
	case SQL_PROCEDURE_TERM:
	case SQL_PROCEDURES:
	case SQL_ROW_UPDATES:
	case SQL_SCHEMA_TERM:
	case SQL_SEARCH_PATTERN_ESCAPE:
	case SQL_SERVER_NAME:
	case SQL_SPECIAL_CHARACTERS:
	case SQL_TABLE_TERM:
	case SQL_USER_NAME:
	case SQL_XOPEN_CLI_YEAR:
	case SQL_DBMS_VER:
		lpCallStr->szArg[2]="SQLCHAR *";
		lpCallStr->atArg[2]=TYP_SQLCHARPTR;
		break;
	case SQL_ACTIVE_ENVIRONMENTS:
	case SQL_CATALOG_LOCATION:
	case SQL_CONCAT_NULL_BEHAVIOR:
	case SQL_CORRELATION_NAME:
	case SQL_CURSOR_COMMIT_BEHAVIOR:
	case SQL_CURSOR_ROLLBACK_BEHAVIOR:
	case SQL_FILE_USAGE:
	case SQL_GROUP_BY:
	case SQL_IDENTIFIER_CASE:
	case SQL_INTEGRITY:
	case SQL_MAX_CATALOG_NAME_LEN:
	case SQL_MAX_COLUMN_NAME_LEN:
	case SQL_MAX_COLUMNS_IN_GROUP_BY:
	case SQL_MAX_COLUMNS_IN_INDEX:
	case SQL_MAX_COLUMNS_IN_ORDER_BY:
	case SQL_MAX_COLUMNS_IN_SELECT:
	case SQL_MAX_COLUMNS_IN_TABLE:
	case SQL_MAX_CONCURRENT_ACTIVITIES:
	case SQL_MAX_CURSOR_NAME_LEN:
	case SQL_MAX_DRIVER_CONNECTIONS:
	case SQL_MAX_IDENTIFIER_LEN:
	case SQL_MAX_PROCEDURE_NAME_LEN:
	case SQL_MAX_SCHEMA_NAME_LEN:
	case SQL_MAX_TABLE_NAME_LEN:
	case SQL_MAX_TABLES_IN_SELECT:
	case SQL_MAX_USER_NAME_LEN:
	case SQL_NON_NULLABLE_COLUMNS:
	case SQL_NULL_COLLATION:
	case SQL_QUOTED_IDENTIFIER_CASE:
	case SQL_TXN_CAPABLE:
		lpCallStr->szArg[2]="SQLUSMALLINT";
		lpCallStr->atArg[2]=TYP_UWORD;
		break;
	case SQL_AGGREGATE_FUNCTIONS:
	case SQL_ALTER_DOMAIN:
	case SQL_ALTER_TABLE:
	case SQL_ASYNC_MODE:
	case SQL_BATCH_ROW_COUNT:
	case SQL_BATCH_SUPPORT:
	case SQL_BOOKMARK_PERSISTENCE:
	case SQL_CATALOG_USAGE:
	case SQL_CONVERT_BIGINT:
	case SQL_CONVERT_BINARY:
	case SQL_CONVERT_BIT:
	case SQL_CONVERT_CHAR:
	case SQL_CONVERT_DATE:
	case SQL_CONVERT_DECIMAL:
	case SQL_CONVERT_DOUBLE:
	case SQL_CONVERT_FLOAT:
	case SQL_CONVERT_INTEGER:
	case SQL_CONVERT_INTERVAL_YEAR_MONTH:
	case SQL_CONVERT_INTERVAL_DAY_TIME:
	case SQL_CONVERT_LONGVARBINARY:
	case SQL_CONVERT_NUMERIC:
	case SQL_CONVERT_REAL:
	case SQL_CONVERT_SMALLINT:
	case SQL_CONVERT_TIME:
	case SQL_CONVERT_TIMESTAMP:
	case SQL_CONVERT_TINYINT:
	case SQL_CONVERT_VARBINARY:
	case SQL_CONVERT_VARCHAR:
	case SQL_CONVERT_FUNCTIONS:
	case SQL_CREATE_ASSERTION:
	case SQL_CREATE_CHARACTER_SET:
	case SQL_CREATE_COLLATION:
	case SQL_CREATE_DOMAIN:
	case SQL_CREATE_SCHEMA:
	case SQL_CREATE_TABLE:
	case SQL_CREATE_TRANSLATION:
	case SQL_CREATE_VIEW:
	case SQL_CURSOR_SENSITIVITY:
	case SQL_DATETIME_LITERALS:
	case SQL_DDL_INDEX:
	case SQL_DEFAULT_TXN_ISOLATION:
	case SQL_DRIVER_HDBC:
	case SQL_DRIVER_HENV:
	case SQL_DRIVER_HDESC:
	case SQL_DRIVER_HLIB:
	case SQL_DRIVER_HSTMT:
	case SQL_DROP_ASSERTION:
	case SQL_DROP_CHARACTER_SET:
	case SQL_DROP_COLLATION:
	case SQL_DROP_DOMAIN:
	case SQL_DROP_SCHEMA:
	case SQL_DROP_TABLE:
	case SQL_DROP_TRANSLATION:
	case SQL_DROP_VIEW:
	case SQL_DYNAMIC_CURSOR_ATTRIBUTES1:
	case SQL_DYNAMIC_CURSOR_ATTRIBUTES2:
	case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1:
	case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2:
	case SQL_GETDATA_EXTENSIONS:
	case SQL_INDEX_KEYWORDS:
	case SQL_INFO_SCHEMA_VIEWS:
	case SQL_INSERT_STATEMENT:
	case SQL_KEYSET_CURSOR_ATTRIBUTES1:
	case SQL_KEYSET_CURSOR_ATTRIBUTES2:
	case SQL_MAX_ASYNC_CONCURRENT_STATEMENTS:
	case SQL_MAX_BINARY_LITERAL_LEN:
	case SQL_MAX_CHAR_LITERAL_LEN:
	case SQL_MAX_INDEX_SIZE:
	case SQL_MAX_ROW_SIZE:
	case SQL_MAX_STATEMENT_LEN:
	case SQL_NUMERIC_FUNCTIONS:
	case SQL_ODBC_INTERFACE_CONFORMANCE:
	case SQL_OJ_CAPABILITIES:
	case SQL_PARAM_ARRAY_ROW_COUNTS:
	case SQL_PARAM_ARRAY_SELECTS:
	case SQL_POS_OPERATIONS:
	case SQL_SCHEMA_USAGE:
	case SQL_SCROLL_OPTIONS:
	case SQL_SQL_CONFORMANCE:
	case SQL_SQL92_DATETIME_FUNCTIONS:
	case SQL_SQL92_FOREIGN_KEY_DELETE_RULE:
	case SQL_SQL92_FOREIGN_KEY_UPDATE_RULE:
	case SQL_SQL92_GRANT:
	case SQL_SQL92_NUMERIC_VALUE_FUNCTIONS:
	case SQL_SQL92_PREDICATES:
	case SQL_SQL92_RELATIONAL_JOIN_OPERATORS:
	case SQL_SQL92_REVOKE:
	case SQL_SQL92_ROW_VALUE_CONSTRUCTOR:
	case SQL_SQL92_STRING_FUNCTIONS:
	case SQL_STANDARD_CLI_CONFORMANCE:
	case SQL_STATIC_CURSOR_ATTRIBUTES1:
	case SQL_STATIC_CURSOR_ATTRIBUTES2:
	case SQL_STRING_FUNCTIONS:
	case SQL_SUBQUERIES:
	case SQL_SYSTEM_FUNCTIONS:
	case SQL_TIMEDATE_ADD_INTERVALS:
	case SQL_TIMEDATE_DIFF_INTERVALS:
	case SQL_TIMEDATE_FUNCTIONS:
	case SQL_TXN_ISOLATION_OPTION:
	case SQL_UNION:
		lpCallStr->szArg[2]="SQLUINTEGER";
		lpCallStr->atArg[2]=TYP_UDWORD;
		break;
	default:
		lpCallStr->szArg[2]="SQLCHAR *";
		lpCallStr->atArg[2]=TYP_SQLCHARPTR;
		break;
	}
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD";
	lpCallStr->atArg[3]=TYP_SWORD;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD *";
	lpCallStr->atArg[4]=TYP_SWORDPTR;
	lpCallStr->nArgs = 5;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetStmtOption /////

extern "C" RETCODE SQL_API TraceSQLGetStmtOption  (HSTMT arg0,
		 UWORD arg1,
		 PTR arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETSTMTOPTION;
	lpCallStr->szFuncName="SQLGetStmtOption";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="PTR";
	lpCallStr->atArg[2]=TYP_PTR;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetTypeInfo /////

extern "C" RETCODE SQL_API TraceSQLGetTypeInfo  (HSTMT arg0,
		 SWORD arg1)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETTYPEINFO;
	lpCallStr->szFuncName="SQLGetTypeInfoW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SWORD";
	lpCallStr->atArg[1]=TYP_SWORD;
	lpCallStr->nArgs = 2;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLParamData /////

extern "C" RETCODE SQL_API TraceSQLParamData  (HSTMT arg0,
		 PTR * arg1)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLPARAMDATA;
	lpCallStr->szFuncName="SQLParamData";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="PTR *";
	lpCallStr->atArg[1]=TYP_PTRPTR;
	lpCallStr->nArgs = 2;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLPutData /////

extern "C" RETCODE SQL_API TraceSQLPutData  (HSTMT arg0,
		 PTR arg1,
		 SDWORD_P arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLPUTDATA;
	lpCallStr->szFuncName="SQLPutData";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="PTR";
	lpCallStr->atArg[1]=TYP_PTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SDWORD";
	lpCallStr->atArg[2]=TYP_SDWORD;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSetConnectOption /////

extern "C" RETCODE SQL_API TraceSQLSetConnectOption  (HDBC arg0,
		 UWORD arg1,
		 UDWORD_P arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSETCONNECTOPTION;
	lpCallStr->szFuncName="SQLSetConnectOption";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="UDWORD";
	lpCallStr->atArg[2]=TYP_UDWORD;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_CONFIG)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSetStmtOption /////

extern "C" RETCODE SQL_API TraceSQLSetStmtOption  (HSTMT arg0,
		 UWORD arg1,
		 UDWORD_P arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSETSTMTOPTION;
	lpCallStr->szFuncName="SQLSetStmtOption";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="UDWORD";
	lpCallStr->atArg[2]=TYP_UDWORD;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_CONFIG)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSpecialColumns /////

extern "C" RETCODE SQL_API TraceSQLSpecialColumns  (HSTMT arg0,
		 UWORD arg1,
		 UCHAR * arg2,
		 SWORD arg3,
		 UCHAR * arg4,
		 SWORD arg5,
		 UCHAR * arg6,
		 SWORD arg7,
		 UWORD arg8,
		 UWORD arg9)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSPECIALCOLUMNS;
	lpCallStr->szFuncName="SQLSpecialColumnsW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="UCHAR *";
	lpCallStr->atArg[2]=TYP_UCHARPTR;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD";
	lpCallStr->atArg[3]=TYP_SWORD;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="UCHAR *";
	lpCallStr->atArg[4]=TYP_UCHARPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SWORD";
	lpCallStr->atArg[5]=TYP_SWORD;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="UCHAR *";
	lpCallStr->atArg[6]=TYP_UCHARPTR;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SWORD";
	lpCallStr->atArg[7]=TYP_SWORD;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="UWORD";
	lpCallStr->atArg[8]=TYP_UWORD;
	lpCallStr->lpvArg[9] = (LPVOID) arg9;
	lpCallStr->szArg[9]="UWORD";
	lpCallStr->atArg[9]=TYP_UWORD;
	lpCallStr->nArgs = 10;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLStatistics /////

extern "C" RETCODE SQL_API TraceSQLStatistics  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 UWORD arg7,
		 UWORD arg8)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSTATISTICS;
	lpCallStr->szFuncName="SQLStatisticsW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="UCHAR *";
	lpCallStr->atArg[3]=TYP_UCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="UCHAR *";
	lpCallStr->atArg[5]=TYP_UCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="UWORD";
	lpCallStr->atArg[7]=TYP_UWORD;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="UWORD";
	lpCallStr->atArg[8]=TYP_UWORD;
	lpCallStr->nArgs = 9;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLTables /////

extern "C" RETCODE SQL_API TraceSQLTables  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 UCHAR * arg7,
		 SWORD arg8)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLTABLES;
	lpCallStr->szFuncName="SQLTablesW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="UCHAR *";
	lpCallStr->atArg[3]=TYP_UCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="UCHAR *";
	lpCallStr->atArg[5]=TYP_UCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="UCHAR *";
	lpCallStr->atArg[7]=TYP_UCHARPTR;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="SWORD";
	lpCallStr->atArg[8]=TYP_SWORD;
	lpCallStr->nArgs = 9;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLBrowseConnect /////

extern "C" RETCODE SQL_API TraceSQLBrowseConnect  (HDBC arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UNALIGNED SWORD * arg5)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLBROWSECONNECT;
	lpCallStr->szFuncName="SQLBrowseConnectW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="UCHAR *";
	lpCallStr->atArg[3]=TYP_UCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SWORD *";
	lpCallStr->atArg[5]=TYP_SWORDPTR;
	lpCallStr->nArgs = 6;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLBrowseConnect /////

extern "C" RETCODE SQL_API TraceTdmSQLBrowseConnect  (HDBC arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UNALIGNED SWORD * arg5)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	char maskedArg[2049];
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLBROWSECONNECT;
	lpCallStr->szFuncName="SQLBrowseConnect";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="UCHAR *";
	lpCallStr->atArg[3]=TYP_UCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SWORD *";
	lpCallStr->atArg[5]=TYP_SWORDPTR;
	lpCallStr->nArgs = 6;
	lpCallStr->wPrintMarker = TRUE;
	lpCallStr->dwTdmFlags = TDM_FLAG_PASSWORD; 
	MaskUseridPassword(lpCallStr,1,2,maskedArg);
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLColumnPrivileges /////

extern "C" RETCODE SQL_API TraceSQLColumnPrivileges  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 UCHAR * arg7,
		 SWORD arg8)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLCOLUMNPRIVILEGES;
	lpCallStr->szFuncName="SQLColumnPrivilegesW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="UCHAR *";
	lpCallStr->atArg[3]=TYP_UCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="UCHAR *";
	lpCallStr->atArg[5]=TYP_UCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="UCHAR *";
	lpCallStr->atArg[7]=TYP_UCHARPTR;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="SWORD";
	lpCallStr->atArg[8]=TYP_SWORD;
	lpCallStr->nArgs = 9;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLDataSources /////

extern "C" RETCODE SQL_API TraceSQLDataSources  (HENV arg0,
		 UWORD arg1,
		 UCHAR * arg2,
		 SWORD arg3,
		 SWORD * arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 SWORD * arg7)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLDATASOURCES;
	lpCallStr->szFuncName="SQLDataSources";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HENV";
	lpCallStr->atArg[0]=TYP_HENV;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="UCHAR *";
	lpCallStr->atArg[2]=TYP_UCHARPTR;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD";
	lpCallStr->atArg[3]=TYP_SWORD;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD *";
	lpCallStr->atArg[4]=TYP_SWORDPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="UCHAR *";
	lpCallStr->atArg[5]=TYP_UCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SWORD *";
	lpCallStr->atArg[7]=TYP_SWORDPTR;
	lpCallStr->nArgs = 8;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLDescribeParam /////

extern "C" RETCODE SQL_API TraceSQLDescribeParam  (HSTMT arg0,
		 UWORD arg1,
		 UNALIGNED SWORD * arg2,
		 UNALIGNED UDWORD_P * arg3,
		 UNALIGNED SWORD * arg4,
		 UNALIGNED SWORD * arg5)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLDESCRIBEPARAM;
	lpCallStr->szFuncName="SQLDescribeParam";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD *";
	lpCallStr->atArg[2]=TYP_SWORDPTR;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="UDWORD *";
	lpCallStr->atArg[3]=TYP_UDWORDPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD *";
	lpCallStr->atArg[4]=TYP_SWORDPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SWORD *";
	lpCallStr->atArg[5]=TYP_SWORDPTR;
	lpCallStr->nArgs = 6;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLExtendedFetch /////

extern "C" RETCODE SQL_API TraceSQLExtendedFetch  (HSTMT arg0,
		 UWORD arg1,
		 SDWORD_P arg2,
		 UNALIGNED UDWORD_P * arg3,
		 UNALIGNED UWORD * arg4)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLEXTENDEDFETCH;
	lpCallStr->szFuncName="SQLExtendedFetch";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SDWORD";
	lpCallStr->atArg[2]=TYP_SDWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="UDWORD *";
	lpCallStr->atArg[3]=TYP_UDWORDPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="UWORD *";
	lpCallStr->atArg[4]=TYP_UWORDPTR;
	lpCallStr->nArgs = 5;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLForeignKeys /////

extern "C" RETCODE SQL_API TraceSQLForeignKeys  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 UCHAR * arg7,
		 SWORD arg8,
		 UCHAR * arg9,
		 SWORD arg10,
		 UCHAR * arg11,
		 SWORD arg12)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLFOREIGNKEYS;
	lpCallStr->szFuncName="SQLForeignKeysW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="UCHAR *";
	lpCallStr->atArg[3]=TYP_UCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="UCHAR *";
	lpCallStr->atArg[5]=TYP_UCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="UCHAR *";
	lpCallStr->atArg[7]=TYP_UCHARPTR;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="SWORD";
	lpCallStr->atArg[8]=TYP_SWORD;
	lpCallStr->lpvArg[9] = (LPVOID) arg9;
	lpCallStr->szArg[9]="UCHAR *";
	lpCallStr->atArg[9]=TYP_UCHARPTR;
	lpCallStr->lpvArg[10] = (LPVOID) arg10;
	lpCallStr->szArg[10]="SWORD";
	lpCallStr->atArg[10]=TYP_SWORD;
	lpCallStr->lpvArg[11] = (LPVOID) arg11;
	lpCallStr->szArg[11]="UCHAR *";
	lpCallStr->atArg[11]=TYP_UCHARPTR;
	lpCallStr->lpvArg[12] = (LPVOID) arg12;
	lpCallStr->szArg[12]="SWORD";
	lpCallStr->atArg[12]=TYP_SWORD;
	lpCallStr->nArgs = 13;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLMoreResults /////

extern "C" RETCODE SQL_API TraceSQLMoreResults  (HSTMT arg0)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLMORERESULTS;
	lpCallStr->szFuncName="SQLMoreResults";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->nArgs = 1;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLNativeSql /////

extern "C" RETCODE SQL_API TraceSQLNativeSql  (HDBC arg0,
		 UCHAR * arg1,
		 SDWORD_P arg2,
		 UCHAR * arg3,
		 SDWORD_P arg4,
		 UNALIGNED SDWORD_P * arg5)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLNATIVESQL;
	lpCallStr->szFuncName="SQLNativeSqlW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SDWORD";
	lpCallStr->atArg[2]=TYP_SDWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="UCHAR *";
	lpCallStr->atArg[3]=TYP_UCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SDWORD";
	lpCallStr->atArg[4]=TYP_SDWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SDWORD *";
	lpCallStr->atArg[5]=TYP_SDWORDPTR;
	lpCallStr->nArgs = 6;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLNumParams /////

extern "C" RETCODE SQL_API TraceSQLNumParams  (HSTMT arg0,
		 UNALIGNED SWORD * arg1)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLNUMPARAMS;
	lpCallStr->szFuncName="SQLNumParams";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SWORD *";
	lpCallStr->atArg[1]=TYP_SWORDPTR;
	lpCallStr->nArgs = 2;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLParamOptions /////

extern "C" RETCODE SQL_API TraceSQLParamOptions  (HSTMT arg0,
		 UDWORD_P arg1,
		 UNALIGNED UDWORD_P * arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLPARAMOPTIONS;
	lpCallStr->szFuncName="SQLParamOptions";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UDWORD";
	lpCallStr->atArg[1]=TYP_UDWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="UDWORD *";
	lpCallStr->atArg[2]=TYP_UDWORDPTR;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLPrimaryKeys /////

extern "C" RETCODE SQL_API TraceSQLPrimaryKeys  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLPRIMARYKEYS;
	lpCallStr->szFuncName="SQLPrimaryKeysW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="UCHAR *";
	lpCallStr->atArg[3]=TYP_UCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="UCHAR *";
	lpCallStr->atArg[5]=TYP_UCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->nArgs = 7;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLProcedureColumns /////

extern "C" RETCODE SQL_API TraceSQLProcedureColumns  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 UCHAR * arg7,
		 SWORD arg8)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLPROCEDURECOLUMNS;
	lpCallStr->szFuncName="SQLProcedureColumnsW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="UCHAR *";
	lpCallStr->atArg[3]=TYP_UCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="UCHAR *";
	lpCallStr->atArg[5]=TYP_UCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="UCHAR *";
	lpCallStr->atArg[7]=TYP_UCHARPTR;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="SWORD";
	lpCallStr->atArg[8]=TYP_SWORD;
	lpCallStr->nArgs = 9;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLProcedures /////

extern "C" RETCODE SQL_API TraceSQLProcedures  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLPROCEDURES;
	lpCallStr->szFuncName="SQLProceduresW"; //Change for Unicode drivers
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="UCHAR *";
	lpCallStr->atArg[3]=TYP_UCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="UCHAR *";
	lpCallStr->atArg[5]=TYP_UCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->nArgs = 7;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSetPos /////

extern "C" RETCODE SQL_API TraceSQLSetPos  (HSTMT arg0,
		 UWORD arg1,
		 UWORD arg2,
		 BOOL arg3)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSETPOS;
	lpCallStr->szFuncName="SQLSetPos";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="UWORD";
	lpCallStr->atArg[2]=TYP_UWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="BOOL";
	lpCallStr->atArg[3]=TYP_BOOL;
	lpCallStr->nArgs = 4;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSetScrollOptions /////

extern "C" RETCODE SQL_API TraceSQLSetScrollOptions  (HSTMT arg0,
		 UWORD arg1,
		 SDWORD_P arg2,
		 UWORD arg3)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSETSCROLLOPTIONS;
	lpCallStr->szFuncName="SQLSetScrollOptions";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SDWORD";
	lpCallStr->atArg[2]=TYP_SDWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="UWORD";
	lpCallStr->atArg[3]=TYP_UWORD;
	lpCallStr->nArgs = 4;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLTablePrivileges /////

extern "C" RETCODE SQL_API TraceSQLTablePrivileges  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLTABLEPRIVILEGES;
	lpCallStr->szFuncName="SQLTablePrivilegesW"; //change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UCHAR *";
	lpCallStr->atArg[1]=TYP_UCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="UCHAR *";
	lpCallStr->atArg[3]=TYP_UCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="UCHAR *";
	lpCallStr->atArg[5]=TYP_UCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->nArgs = 7;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLDrivers /////

extern "C" RETCODE SQL_API TraceSQLDrivers  (HENV arg0,
		 UWORD arg1,
		 UCHAR * arg2,
		 SWORD arg3,
		 SWORD * arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 SWORD * arg7)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLDRIVERS;
	lpCallStr->szFuncName="SQLDrivers";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HENV";
	lpCallStr->atArg[0]=TYP_HENV;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="UCHAR *";
	lpCallStr->atArg[2]=TYP_UCHARPTR;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD";
	lpCallStr->atArg[3]=TYP_SWORD;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD *";
	lpCallStr->atArg[4]=TYP_SWORDPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="UCHAR *";
	lpCallStr->atArg[5]=TYP_UCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SWORD *";
	lpCallStr->atArg[7]=TYP_SWORDPTR;
	lpCallStr->nArgs = 8;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}


///// Trace function for SQLBindParameter /////

extern "C" RETCODE SQL_API TraceSQLBindParameter  (HSTMT arg0,
		 UWORD arg1,
		 SWORD arg2,
		 SWORD arg3,
		 SWORD arg4,
		 UDWORD_P arg5,
		 SWORD arg6,
		 PTR arg7,
		 SDWORD_P arg8,
			UNALIGNED SDWORD_P * arg9)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLBINDPARAMETER;
	lpCallStr->szFuncName="SQLBindParameter";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD";
	lpCallStr->atArg[3]=TYP_SWORD;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="UDWORD";
	lpCallStr->atArg[5]=TYP_UDWORD;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="PTR";
	lpCallStr->atArg[7]=TYP_PTR;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="SDWORD";
	lpCallStr->atArg[8]=TYP_SDWORD;
	lpCallStr->lpvArg[9] = (LPVOID) arg9;
	lpCallStr->szArg[9]="SDWORD *";
	lpCallStr->atArg[9]=TYP_SDWORDPTR;
	lpCallStr->nArgs = 10;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLAllocHandle /////

extern "C" RETCODE SQL_API TraceSQLAllocHandle  (SQLSMALLINT arg0,
		 SQLHANDLE arg1,
		 SQLHANDLE * arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLALLOCHANDLE;
	lpCallStr->szFuncName="SQLAllocHandle";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLSMALLINT";
	lpCallStr->atArg[0]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLHANDLE";
	lpCallStr->atArg[1]=TYP_SQLHANDLE;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLHANDLE *";
	lpCallStr->atArg[2]=TYP_SQLHANDLEPTR;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLBindParam /////

extern "C" RETCODE SQL_API TraceSQLBindParam  (SQLHSTMT arg0,
		 SQLSMALLINT arg1,
		 SQLSMALLINT arg2,
		 SQLSMALLINT arg3,
		 SQLINTEGER arg4,
		 SQLSMALLINT arg5,
		 SQLPOINTER arg6,
		 UNALIGNED SQLINTEGER * arg7)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLBINDPARAM;
	lpCallStr->szFuncName="SQLBindParam";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHSTMT";
	lpCallStr->atArg[0]=TYP_SQLHSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLSMALLINT";
	lpCallStr->atArg[1]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLSMALLINT";
	lpCallStr->atArg[2]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLSMALLINT";
	lpCallStr->atArg[3]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLINTEGER";
	lpCallStr->atArg[4]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SQLSMALLINT";
	lpCallStr->atArg[5]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SQLPOINTER";
	lpCallStr->atArg[6]=TYP_SQLPOINTER;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SQLINTEGER *";
	lpCallStr->atArg[7]=TYP_SQLINTEGERPTR;
	lpCallStr->nArgs = 8;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLCloseCursor /////

extern "C" RETCODE SQL_API TraceSQLCloseCursor  (SQLHSTMT arg0)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLCLOSECURSOR;
	lpCallStr->szFuncName="SQLCloseCursor";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHSTMT";
	lpCallStr->atArg[0]=TYP_SQLHSTMT;
	lpCallStr->nArgs = 1;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLColAttribute /////

extern "C" RETCODE SQL_API TraceSQLColAttribute  (SQLHSTMT arg0,
		 SQLSMALLINT arg1,
		 SQLSMALLINT arg2,
		 SQLPOINTER arg3,
		 SQLSMALLINT arg4,
		 UNALIGNED SQLSMALLINT * arg5,
		 SQLPOINTER arg6)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLCOLATTRIBUTE;
	lpCallStr->szFuncName="SQLColAttributeW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHSTMT";
	lpCallStr->atArg[0]=TYP_SQLHSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLSMALLINT";
	lpCallStr->atArg[1]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLSMALLINT";
	lpCallStr->atArg[2]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLPOINTER";
	lpCallStr->atArg[3]=TYP_SQLPOINTER;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLSMALLINT";
	lpCallStr->atArg[4]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SQLSMALLINT *";
	lpCallStr->atArg[5]=TYP_SQLSMALLINTPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SQLPOINTER";
	lpCallStr->atArg[6]=TYP_SQLPOINTER;
	lpCallStr->nArgs = 7;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLCopyDesc /////

extern "C" RETCODE SQL_API TraceSQLCopyDesc  (SQLHDESC arg0,
		 SQLHDESC arg1)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLCOPYDESC;
	lpCallStr->szFuncName="SQLCopyDesc";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHDESC";
	lpCallStr->atArg[0]=TYP_SQLHDESC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLHDESC";
	lpCallStr->atArg[1]=TYP_SQLHDESC;
	lpCallStr->nArgs = 2;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLEndTran /////

extern "C" RETCODE SQL_API TraceSQLEndTran  (SQLSMALLINT arg0,
		 SQLHANDLE arg1,
		 SQLSMALLINT arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLENDTRAN;
	lpCallStr->szFuncName="SQLEndTran";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLSMALLINT";
	lpCallStr->atArg[0]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLHANDLE";
	lpCallStr->atArg[1]=TYP_SQLHANDLE;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLSMALLINT";
	lpCallStr->atArg[2]=TYP_SQLSMALLINT;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLFetchScroll /////

extern "C" RETCODE SQL_API TraceSQLFetchScroll  (SQLHSTMT arg0,
		 SQLSMALLINT arg1,
		 SQLINTEGER arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLFETCHSCROLL;
	lpCallStr->szFuncName="SQLFetchScroll";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHSTMT";
	lpCallStr->atArg[0]=TYP_SQLHSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLSMALLINT";
	lpCallStr->atArg[1]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLINTEGER";
	lpCallStr->atArg[2]=TYP_SQLINTEGER;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLFreeHandle /////

extern "C" RETCODE SQL_API TraceSQLFreeHandle  (SQLSMALLINT arg0,
		 SQLHANDLE arg1)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLFREEHANDLE;
	lpCallStr->szFuncName="SQLFreeHandle";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLSMALLINT";
	lpCallStr->atArg[0]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLHANDLE";
	lpCallStr->atArg[1]=TYP_SQLHANDLE;
	lpCallStr->nArgs = 2;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetConnectAttr /////

extern "C" RETCODE SQL_API TraceSQLGetConnectAttr  (SQLHDBC arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3,
		 UNALIGNED SQLINTEGER * arg4)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETCONNECTATTR;
	lpCallStr->szFuncName="SQLGetConnectAttrW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHDBC";
	lpCallStr->atArg[0]=TYP_SQLHDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLINTEGER";
	lpCallStr->atArg[1]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLPOINTER";
	lpCallStr->atArg[2]=TYP_SQLPOINTER;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLINTEGER";
	lpCallStr->atArg[3]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLINTEGER *";
	lpCallStr->atArg[4]=TYP_SQLINTEGERPTR;
	lpCallStr->nArgs = 5;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetDescField /////

extern "C" RETCODE SQL_API TraceSQLGetDescField  (SQLHDESC arg0,
		 SQLSMALLINT arg1,
		 SQLSMALLINT arg2,
		 SQLPOINTER arg3,
		 SQLINTEGER arg4,
		 UNALIGNED SQLINTEGER * arg5)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETDESCFIELD;
	lpCallStr->szFuncName="SQLGetDescFieldW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHDESC";
	lpCallStr->atArg[0]=TYP_SQLHDESC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLSMALLINT";
	lpCallStr->atArg[1]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLSMALLINT";
	lpCallStr->atArg[2]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	switch (arg2)
	{
	default:
		lpCallStr->szArg[3]="SQLPOINTER";
		lpCallStr->atArg[3]=TYP_SQLPOINTER;
		break;
	case SQL_DESC_ARRAY_STATUS_PTR:
	case SQL_DESC_BIND_OFFSET_PTR:
	case SQL_DESC_ROWS_PROCESSED_PTR:
	case SQL_DESC_DATA_PTR:
	case SQL_DESC_INDICATOR_PTR:
	case SQL_DESC_OCTET_LENGTH_PTR:
		lpCallStr->szArg[3]="SQLPOINTER*";
		lpCallStr->atArg[3]=TYP_SQLHANDLEPTR;
		break;
	case SQL_DESC_BASE_COLUMN_NAME:
	case SQL_DESC_BASE_TABLE_NAME:
	case SQL_DESC_CATALOG_NAME:
	case SQL_DESC_LABEL:
	case SQL_DESC_LITERAL_PREFIX:
	case SQL_DESC_LITERAL_SUFFIX:
	case SQL_DESC_LOCAL_TYPE_NAME:
	case SQL_DESC_NAME:
	case SQL_DESC_SCHEMA_NAME:
	case SQL_DESC_TABLE_NAME:
	case SQL_DESC_TYPE_NAME:
		lpCallStr->szArg[3]="CHAR *";
		lpCallStr->atArg[3]=TYP_SQLCHARPTR;
		break;
	case SQL_DESC_ALLOC_TYPE:
	case SQL_DESC_COUNT:
	case SQL_DESC_CONCISE_TYPE:
	case SQL_DESC_DATETIME_INTERVAL_CODE:
	case SQL_DESC_FIXED_PREC_SCALE:
	case SQL_DESC_NULLABLE:
	case SQL_DESC_PARAMETER_TYPE:
	case SQL_DESC_PRECISION:
	case SQL_DESC_SCALE:
	case SQL_DESC_SEARCHABLE:
	case SQL_DESC_TYPE:
	case SQL_DESC_UNNAMED:
	case SQL_DESC_UNSIGNED:
	case SQL_DESC_UPDATABLE:
		lpCallStr->szArg[3]="SMALLINTEGER";
		lpCallStr->atArg[3]=TYP_SQLSMALLINT;
		break;
	case SQL_DESC_AUTO_UNIQUE_VALUE:
	case SQL_DESC_CASE_SENSITIVE:
	case SQL_DESC_BIND_TYPE:
	case SQL_DESC_DATETIME_INTERVAL_PRECISION:
	case SQL_DESC_DISPLAY_SIZE:
	case SQL_DESC_NUM_PREC_RADIX:
	case SQL_DESC_OCTET_LENGTH:
		lpCallStr->szArg[3]="INTEGER";
		lpCallStr->atArg[3]=TYP_SQLINTEGER;
		break;
	case SQL_DESC_ARRAY_SIZE:
	case SQL_DESC_LENGTH:
		lpCallStr->szArg[3]="UINTEGER";
		lpCallStr->atArg[3]=TYP_UDWORD;
		break;
	}
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLINTEGER";
	lpCallStr->atArg[4]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SQLINTEGER *";
	lpCallStr->atArg[5]=TYP_SQLINTEGERPTR;
	lpCallStr->nArgs = 6;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetDescRec /////

extern "C" RETCODE SQL_API TraceSQLGetDescRec  (SQLHDESC arg0,
		 SQLSMALLINT arg1,
		 SQLCHAR * arg2,
		 SQLSMALLINT arg3,
		 UNALIGNED SQLSMALLINT * arg4,
		 UNALIGNED SQLSMALLINT * arg5,
		 UNALIGNED SQLSMALLINT * arg6,
		 UNALIGNED SQLINTEGER  * arg7,
		 UNALIGNED SQLSMALLINT * arg8,
		 UNALIGNED SQLSMALLINT * arg9,
		 UNALIGNED SQLSMALLINT * arg10)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETDESCREC;
	lpCallStr->szFuncName="SQLGetDescRecW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHDESC";
	lpCallStr->atArg[0]=TYP_SQLHDESC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLSMALLINT";
	lpCallStr->atArg[1]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLCHAR *";
	lpCallStr->atArg[2]=TYP_SQLCHARPTR;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLSMALLINT";
	lpCallStr->atArg[3]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLSMALLINT *";
	lpCallStr->atArg[4]=TYP_SQLSMALLINTPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SQLSMALLINT *";
	lpCallStr->atArg[5]=TYP_SQLSMALLINTPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SQLSMALLINT *";
	lpCallStr->atArg[6]=TYP_SQLSMALLINTPTR;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SQLINTEGER *";
	lpCallStr->atArg[7]=TYP_SQLINTEGERPTR;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="SQLSMALLINT *";
	lpCallStr->atArg[8]=TYP_SQLSMALLINTPTR;
	lpCallStr->lpvArg[9] = (LPVOID) arg9;
	lpCallStr->szArg[9]="SQLSMALLINT *";
	lpCallStr->atArg[9]=TYP_SQLSMALLINTPTR;
	lpCallStr->lpvArg[10] = (LPVOID) arg10;
	lpCallStr->szArg[10]="SQLSMALLINT *";
	lpCallStr->atArg[10]=TYP_SQLSMALLINTPTR;
	lpCallStr->nArgs = 11;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetDiagField /////

extern "C" RETCODE SQL_API TraceSQLGetDiagField  (SQLSMALLINT arg0,
		 SQLHANDLE arg1,
		 SQLSMALLINT arg2,
		 SQLSMALLINT arg3,
		 SQLPOINTER arg4,
		 SQLSMALLINT arg5,
		 UNALIGNED SQLSMALLINT * arg6)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETDIAGFIELD;
	lpCallStr->szFuncName="SQLGetDiagFieldW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLSMALLINT";
	lpCallStr->atArg[0]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLHANDLE";
	lpCallStr->atArg[1]=TYP_SQLHANDLE;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLSMALLINT";
	lpCallStr->atArg[2]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLSMALLINT";
	lpCallStr->atArg[3]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLPOINTER";
	lpCallStr->atArg[4]=TYP_SQLPOINTER;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SQLSMALLINT";
	lpCallStr->atArg[5]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SQLSMALLINT *";
	lpCallStr->atArg[6]=TYP_SQLSMALLINTPTR;
	lpCallStr->nArgs = 7;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetDiagRec /////

extern "C" RETCODE SQL_API TraceSQLGetDiagRec  (SQLSMALLINT arg0,
		 SQLHANDLE arg1,
		 SQLSMALLINT arg2,
		 SQLCHAR * arg3,
		 UNALIGNED SQLINTEGER * arg4,
		 SQLCHAR * arg5,
		 SQLSMALLINT arg6,
		 UNALIGNED SQLSMALLINT * arg7)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETDIAGREC;
	lpCallStr->szFuncName="SQLGetDiagRecW"; //change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLSMALLINT";
	lpCallStr->atArg[0]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLHANDLE";
	lpCallStr->atArg[1]=TYP_SQLHANDLE;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLSMALLINT";
	lpCallStr->atArg[2]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLCHAR *";
	lpCallStr->atArg[3]=TYP_SQLCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLINTEGER *";
	lpCallStr->atArg[4]=TYP_SQLINTEGERPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SQLCHAR *";
	lpCallStr->atArg[5]=TYP_SQLCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SQLSMALLINT";
	lpCallStr->atArg[6]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SQLSMALLINT *";
	lpCallStr->atArg[7]=TYP_SQLSMALLINTPTR;
	lpCallStr->nArgs = 8;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetEnvAttr /////

extern "C" RETCODE SQL_API TraceSQLGetEnvAttr  (SQLHENV arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3,
		 UNALIGNED SQLINTEGER * arg4)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETENVATTR;
	lpCallStr->szFuncName="SQLGetEnvAttr";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHENV";
	lpCallStr->atArg[0]=TYP_SQLHENV;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLINTEGER";
	lpCallStr->atArg[1]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLPOINTER";
	lpCallStr->atArg[2]=TYP_SQLPOINTER;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLINTEGER";
	lpCallStr->atArg[3]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLINTEGER *";
	lpCallStr->atArg[4]=TYP_SQLINTEGERPTR;
	lpCallStr->nArgs = 5;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetStmtAttr /////

extern "C" RETCODE SQL_API TraceSQLGetStmtAttr  (SQLHSTMT arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3,
		 UNALIGNED SQLINTEGER * arg4)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETSTMTATTR;
	lpCallStr->szFuncName="SQLGetStmtAttrW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHSTMT";
	lpCallStr->atArg[0]=TYP_SQLHSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLINTEGER";
	lpCallStr->atArg[1]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	switch(arg1)
	{
	case SQL_ATTR_APP_PARAM_DESC:
	case SQL_ATTR_APP_ROW_DESC:
	case SQL_ATTR_IMP_PARAM_DESC:
	case SQL_ATTR_IMP_ROW_DESC:
		lpCallStr->szArg[2]="HANDLE *";
		lpCallStr->atArg[2]=TYP_SQLHANDLEPTR;
		break;
	case SQL_ATTR_PARAM_OPERATION_PTR:
	case SQL_ATTR_PARAM_STATUS_PTR:
	case SQL_ATTR_ROW_OPERATION_PTR:
	case SQL_ATTR_ROW_STATUS_PTR:
		lpCallStr->szArg[2]="USMALLIT *";
		lpCallStr->atArg[2]=TYP_UWORDPTR;
		break;
	case SQL_ATTR_FETCH_BOOKMARK_PTR:
	case SQL_ATTR_PARAMS_PROCESSED_PTR:
	case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
	case SQL_ATTR_ROW_BIND_OFFSET_PTR:
	case SQL_ATTR_ROWS_FETCHED_PTR:
		lpCallStr->szArg[2]="UINTEGER *";
		lpCallStr->atArg[2]=TYP_UDWORDPTR;
		break;
	default:
		lpCallStr->szArg[2]="UINTEGER";
		lpCallStr->atArg[2]=TYP_UDWORD;
		break;
	}
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLINTEGER";
	lpCallStr->atArg[3]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLINTEGER *";
	lpCallStr->atArg[4]=TYP_SQLINTEGERPTR;
	lpCallStr->nArgs = 5;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSetConnectAttr /////

extern "C" RETCODE SQL_API TraceSQLSetConnectAttr  (SQLHDBC arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSETCONNECTATTR;
	lpCallStr->szFuncName="SQLSetConnectAttrW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHDBC";
	lpCallStr->atArg[0]=TYP_SQLHDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLINTEGER";
	lpCallStr->atArg[1]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	switch (arg1)
	{
	case SQL_ATTR_ACCESS_MODE:
	case SQL_ATTR_ASYNC_ENABLE:
	case SQL_ATTR_AUTO_IPD:
	case SQL_ATTR_AUTOCOMMIT:
	case SQL_ATTR_CONNECTION_TIMEOUT:
	case SQL_ATTR_LOGIN_TIMEOUT:
	case SQL_ATTR_METADATA_ID:
	case SQL_ATTR_ODBC_CURSORS:
	case SQL_ATTR_PACKET_SIZE:
	case SQL_ATTR_TRACE:
	case SQL_ATTR_TRANSLATE_OPTION:
	case SQL_ATTR_TXN_ISOLATION:
		lpCallStr->szArg[2]="SQLUINTEGER";
		lpCallStr->atArg[2]=TYP_UDWORD;
		break;
	default:
		lpCallStr->szArg[2]="SQLPOINTER";
		lpCallStr->atArg[2]=TYP_SQLPOINTER;
		break;
	}
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLINTEGER";
	lpCallStr->atArg[3]=TYP_SQLINTEGER;
	lpCallStr->nArgs = 4;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_CONFIG)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSetDescField /////

extern "C" RETCODE SQL_API TraceSQLSetDescField  (SQLHDESC arg0,
		 SQLSMALLINT arg1,
		 SQLSMALLINT arg2,
		 SQLPOINTER arg3,
		 SQLINTEGER arg4)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSETDESCFIELD;
	lpCallStr->szFuncName="SQLSetDescFieldW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHDESC";
	lpCallStr->atArg[0]=TYP_SQLHDESC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLSMALLINT";
	lpCallStr->atArg[1]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLSMALLINT";
	lpCallStr->atArg[2]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	switch (arg2)
	{
	default:
		lpCallStr->szArg[3]="SQLPOINTER";
		lpCallStr->atArg[3]=TYP_SQLPOINTER;
		break;
	case SQL_DESC_ARRAY_STATUS_PTR:
	case SQL_DESC_BIND_OFFSET_PTR:
	case SQL_DESC_ROWS_PROCESSED_PTR:
	case SQL_DESC_DATA_PTR:
	case SQL_DESC_INDICATOR_PTR:
	case SQL_DESC_OCTET_LENGTH_PTR:
		lpCallStr->szArg[3]="SQLPOINTER*";
		lpCallStr->atArg[3]=TYP_SQLHANDLEPTR;
		break;
	case SQL_DESC_BASE_COLUMN_NAME:
	case SQL_DESC_BASE_TABLE_NAME:
	case SQL_DESC_CATALOG_NAME:
	case SQL_DESC_LABEL:
	case SQL_DESC_LITERAL_PREFIX:
	case SQL_DESC_LITERAL_SUFFIX:
	case SQL_DESC_LOCAL_TYPE_NAME:
	case SQL_DESC_NAME:
	case SQL_DESC_SCHEMA_NAME:
	case SQL_DESC_TABLE_NAME:
	case SQL_DESC_TYPE_NAME:
		lpCallStr->szArg[3]="CHAR *";
		lpCallStr->atArg[3]=TYP_SQLCHARPTR;
		break;
	case SQL_DESC_ALLOC_TYPE:
	case SQL_DESC_COUNT:
	case SQL_DESC_CONCISE_TYPE:
	case SQL_DESC_DATETIME_INTERVAL_CODE:
	case SQL_DESC_FIXED_PREC_SCALE:
	case SQL_DESC_NULLABLE:
	case SQL_DESC_PARAMETER_TYPE:
	case SQL_DESC_PRECISION:
	case SQL_DESC_SCALE:
	case SQL_DESC_SEARCHABLE:
	case SQL_DESC_TYPE:
	case SQL_DESC_UNNAMED:
	case SQL_DESC_UNSIGNED:
	case SQL_DESC_UPDATABLE:
		lpCallStr->szArg[3]="SMALLINTEGER";
		lpCallStr->atArg[3]=TYP_SQLSMALLINT;
		break;
	case SQL_DESC_AUTO_UNIQUE_VALUE:
	case SQL_DESC_CASE_SENSITIVE:
	case SQL_DESC_BIND_TYPE:
	case SQL_DESC_DATETIME_INTERVAL_PRECISION:
	case SQL_DESC_DISPLAY_SIZE:
	case SQL_DESC_NUM_PREC_RADIX:
	case SQL_DESC_OCTET_LENGTH:
		lpCallStr->szArg[3]="INTEGER";
		lpCallStr->atArg[3]=TYP_SQLINTEGER;
		break;
	case SQL_DESC_ARRAY_SIZE:
	case SQL_DESC_LENGTH:
		lpCallStr->szArg[3]="UINTEGER";
		lpCallStr->atArg[3]=TYP_UDWORD;
		break;
	}
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLINTEGER";
	lpCallStr->atArg[4]=TYP_SQLINTEGER;
	lpCallStr->nArgs = 5;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSetDescRec /////

extern "C" RETCODE SQL_API TraceSQLSetDescRec  (SQLHDESC arg0,
		 SQLSMALLINT arg1,
		 SQLSMALLINT arg2,
		 SQLSMALLINT arg3,
		 SQLINTEGER arg4,
		 SQLSMALLINT arg5,
		 SQLSMALLINT arg6,
		 SQLPOINTER arg7,
		 UNALIGNED SQLINTEGER * arg8,
		 UNALIGNED SQLINTEGER * arg9)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSETDESCREC;
	lpCallStr->szFuncName="SQLSetDescRec";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHDESC";
	lpCallStr->atArg[0]=TYP_SQLHDESC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLSMALLINT";
	lpCallStr->atArg[1]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLSMALLINT";
	lpCallStr->atArg[2]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLSMALLINT";
	lpCallStr->atArg[3]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLINTEGER";
	lpCallStr->atArg[4]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SQLSMALLINT";
	lpCallStr->atArg[5]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SQLSMALLINT";
	lpCallStr->atArg[6]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SQLPOINTER";
	lpCallStr->atArg[7]=TYP_SQLPOINTER;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="SQLINTEGER *";
	lpCallStr->atArg[8]=TYP_SQLINTEGERPTR;
	lpCallStr->lpvArg[9] = (LPVOID) arg9;
	lpCallStr->szArg[9]="SQLINTEGER *";
	lpCallStr->atArg[9]=TYP_SQLINTEGERPTR;
	lpCallStr->nArgs = 10;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSetEnvAttr /////

extern "C" RETCODE SQL_API TraceSQLSetEnvAttr  (SQLHENV arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSETENVATTR;
	lpCallStr->szFuncName="SQLSetEnvAttr";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHENV";
	lpCallStr->atArg[0]=TYP_SQLHENV;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLINTEGER";
	lpCallStr->atArg[1]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	switch (arg1)
	{
	case SQL_ATTR_CONNECTION_POOLING:
	case SQL_ATTR_CP_MATCH:
		lpCallStr->szArg[2]="SQLUINTEGER";
		lpCallStr->atArg[2]=TYP_UDWORD;
		break;
	case SQL_ATTR_ODBC_VERSION:
	case SQL_ATTR_OUTPUT_NTS:
		lpCallStr->szArg[2]="SQLINTEGER";
		lpCallStr->atArg[2]=TYP_SQLINTEGER;
		break;
	default:
		lpCallStr->szArg[2]="SQLPOINTER";
		lpCallStr->atArg[2]=TYP_SQLPOINTER;
		break;
	}
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLINTEGER";
	lpCallStr->atArg[3]=TYP_SQLINTEGER;
	lpCallStr->nArgs = 4;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_CONFIG)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSetStmtAttr /////

extern "C" RETCODE SQL_API TraceSQLSetStmtAttr  (SQLHSTMT arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSETSTMTATTR;
	lpCallStr->szFuncName="SQLSetStmtAttrW"; //Change for Unicode driver
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHSTMT";
	lpCallStr->atArg[0]=TYP_SQLHSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLINTEGER";
	lpCallStr->atArg[1]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	switch(arg1)
	{
	case SQL_ATTR_APP_PARAM_DESC:
	case SQL_ATTR_APP_ROW_DESC:
	case SQL_ATTR_IMP_PARAM_DESC:
	case SQL_ATTR_IMP_ROW_DESC:
		lpCallStr->szArg[2]="HANDLE *";
		lpCallStr->atArg[2]=TYP_SQLHANDLEPTR;
		break;
	case SQL_ATTR_PARAM_OPERATION_PTR:
	case SQL_ATTR_PARAM_STATUS_PTR:
	case SQL_ATTR_ROW_OPERATION_PTR:
	case SQL_ATTR_ROW_STATUS_PTR:
		lpCallStr->szArg[2]="USMALLIT *";
		lpCallStr->atArg[2]=TYP_UWORDPTR;
		break;
	case SQL_ATTR_FETCH_BOOKMARK_PTR:
	case SQL_ATTR_PARAMS_PROCESSED_PTR:
	case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
	case SQL_ATTR_ROW_BIND_OFFSET_PTR:
	case SQL_ATTR_ROWS_FETCHED_PTR:
		lpCallStr->szArg[2]="UINTEGER *";
		lpCallStr->atArg[2]=TYP_UDWORDPTR;
		break;
	default:
		lpCallStr->szArg[2]="UINTEGER";
		lpCallStr->atArg[2]=TYP_UDWORD;
		break;
	}
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLINTEGER";
	lpCallStr->atArg[3]=TYP_SQLINTEGER;
	lpCallStr->nArgs = 4;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_CONFIG)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLAllocHandleStd /////

extern "C" RETCODE SQL_API TraceSQLAllocHandleStd  (SQLSMALLINT arg0,
		 SQLHANDLE arg1,
		 SQLHANDLE * arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLALLOCHANDLESTD;
	lpCallStr->szFuncName="SQLAllocHandleStd";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLSMALLINT";
	lpCallStr->atArg[0]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLHANDLE";
	lpCallStr->atArg[1]=TYP_SQLHANDLE;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLHANDLE *";
	lpCallStr->atArg[2]=TYP_SQLHANDLEPTR;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLBulkOperations /////

extern "C" RETCODE SQL_API TraceSQLBulkOperations  (SQLHSTMT arg0,
			SQLSMALLINT arg1)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLBULKOPERATIONS;
	lpCallStr->szFuncName="SQLBulkOperations";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHSTMT";
	lpCallStr->atArg[0]=TYP_SQLHSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLSMALLINT";
	lpCallStr->atArg[1]=TYP_SQLSMALLINT;
	lpCallStr->nArgs = 2;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}
// Generated by makefile (dofuncs.pl), do not modify! // 

///// Trace function for SQLColAttributes /////

extern "C" RETCODE SQL_API TraceSQLColAttributesW (HSTMT arg0,
		 UWORD arg1,
		 UWORD arg2,
		 PTR arg3,
		 SWORD arg4,
		 UNALIGNED SWORD * arg5,
		 UNALIGNED SDWORD_P * arg6)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLCOLATTRIBUTES;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLColAttributesW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="UWORD";
	lpCallStr->atArg[2]=TYP_UWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="PTR";
	lpCallStr->atArg[3]=TYP_PTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SWORD *";
	lpCallStr->atArg[5]=TYP_SWORDPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SDWORD *";
	lpCallStr->atArg[6]=TYP_SDWORDPTR;
	lpCallStr->nArgs = 7;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLConnect /////

extern "C" RETCODE SQL_API TraceSQLConnectW (HDBC arg0,
		 WCHAR * arg1,
		 SWORD arg2,
		 WCHAR * arg3,
		 SWORD arg4,
		 WCHAR * arg5,
		 SWORD arg6)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLCONNECT;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLConnectW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="WCHAR *";
	lpCallStr->atArg[1]=TYP_WCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="WCHAR *";
	lpCallStr->atArg[3]=TYP_WCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="WCHAR *";
	lpCallStr->atArg[5]=TYP_WCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->nArgs = 7;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLDescribeCol /////

extern "C" RETCODE SQL_API TraceSQLDescribeColW (HSTMT arg0,
		 UWORD arg1,
		 WCHAR * arg2,
		 SWORD arg3,
		 SWORD * arg4,
		 UNALIGNED SWORD * arg5,
		 UNALIGNED UDWORD_P * arg6,
		 UNALIGNED SWORD * arg7,
		 UNALIGNED SWORD * arg8)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLDESCRIBECOL;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLDescribeColW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="WCHAR *";
	lpCallStr->atArg[2]=TYP_WCHARPTR;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD";
	lpCallStr->atArg[3]=TYP_SWORD;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD *";
	lpCallStr->atArg[4]=TYP_SWORDPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SWORD *";
	lpCallStr->atArg[5]=TYP_SWORDPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="UDWORD *";
	lpCallStr->atArg[6]=TYP_UDWORDPTR;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SWORD *";
	lpCallStr->atArg[7]=TYP_SWORDPTR;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="SWORD *";
	lpCallStr->atArg[8]=TYP_SWORDPTR;
	lpCallStr->nArgs = 9;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLError /////

extern "C" RETCODE SQL_API TraceSQLErrorW (HENV arg0,
		 HDBC arg1,
		 HSTMT arg2,
		 WCHAR * arg3,
		 UNALIGNED SDWORD_P * arg4,
		 WCHAR * arg5,
		 SWORD arg6,
		 UNALIGNED SWORD * arg7)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLERROR;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLErrorW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HENV";
	lpCallStr->atArg[0]=TYP_HENV;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="HDBC";
	lpCallStr->atArg[1]=TYP_HDBC;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="HSTMT";
	lpCallStr->atArg[2]=TYP_HSTMT;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="WCHAR *";
	lpCallStr->atArg[3]=TYP_WCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SDWORD *";
	lpCallStr->atArg[4]=TYP_SDWORDPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="WCHAR *";
	lpCallStr->atArg[5]=TYP_WCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SWORD *";
	lpCallStr->atArg[7]=TYP_SWORDPTR;
	lpCallStr->nArgs = 8;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLExecDirect /////

extern "C" RETCODE SQL_API TraceSQLExecDirectW (HSTMT arg0,
		 WCHAR * arg1,
		 SDWORD_P arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLEXECDIRECT;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLExecDirectW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="WCHAR *";
	lpCallStr->atArg[1]=TYP_WCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SDWORD";
	lpCallStr->atArg[2]=TYP_SDWORD;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetCursorName /////

extern "C" RETCODE SQL_API TraceSQLGetCursorNameW (HSTMT arg0,
		 WCHAR * arg1,
		 SWORD arg2,
		 UNALIGNED SWORD * arg3)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETCURSORNAME;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLGetCursorNameW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="WCHAR *";
	lpCallStr->atArg[1]=TYP_WCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD *";
	lpCallStr->atArg[3]=TYP_SWORDPTR;
	lpCallStr->nArgs = 4;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLPrepare /////

extern "C" RETCODE SQL_API TraceSQLPrepareW (HSTMT arg0,
		 WCHAR * arg1,
		 SDWORD_P arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLPREPARE;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLPrepareW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="WCHAR *";
	lpCallStr->atArg[1]=TYP_WCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SDWORD";
	lpCallStr->atArg[2]=TYP_SDWORD;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSetCursorName /////

extern "C" RETCODE SQL_API TraceSQLSetCursorNameW (HSTMT arg0,
		 WCHAR * arg1,
		 SWORD arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSETCURSORNAME;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLSetCursorNameW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="WCHAR *";
	lpCallStr->atArg[1]=TYP_WCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLColumns /////

extern "C" RETCODE SQL_API TraceSQLColumnsW (HSTMT arg0,
		 WCHAR * arg1,
		 SWORD arg2,
		 WCHAR * arg3,
		 SWORD arg4,
		 WCHAR * arg5,
		 SWORD arg6,
		 WCHAR * arg7,
		 SWORD arg8)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLCOLUMNS;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLColumnsW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="WCHAR *";
	lpCallStr->atArg[1]=TYP_WCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="WCHAR *";
	lpCallStr->atArg[3]=TYP_WCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="WCHAR *";
	lpCallStr->atArg[5]=TYP_WCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="WCHAR *";
	lpCallStr->atArg[7]=TYP_WCHARPTR;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="SWORD";
	lpCallStr->atArg[8]=TYP_SWORD;
	lpCallStr->nArgs = 9;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLDriverConnect /////

extern "C" RETCODE SQL_API TraceSQLDriverConnectW (HDBC arg0,
		 HWND arg1,
		 WCHAR * arg2,
		 SWORD arg3,
		 WCHAR * arg4,
		 SWORD arg5,
		 UNALIGNED SWORD * arg6,
		 UWORD arg7)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLDRIVERCONNECT;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLDriverConnectW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="HWND";
	lpCallStr->atArg[1]=TYP_HWND;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="WCHAR *";
	lpCallStr->atArg[2]=TYP_WCHARPTR;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD";
	lpCallStr->atArg[3]=TYP_SWORD;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="WCHAR *";
	lpCallStr->atArg[4]=TYP_WCHARPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SWORD";
	lpCallStr->atArg[5]=TYP_SWORD;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD *";
	lpCallStr->atArg[6]=TYP_SWORDPTR;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="UWORD";
	lpCallStr->atArg[7]=TYP_UWORD;
	lpCallStr->nArgs = 8;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetConnectOption /////

extern "C" RETCODE SQL_API TraceSQLGetConnectOptionW (HDBC arg0,
		 UWORD arg1,
		 PTR arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETCONNECTOPTION;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLGetConnectOptionW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="PTR";
	lpCallStr->atArg[2]=TYP_PTR;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetInfo /////

extern "C" RETCODE SQL_API TraceSQLGetInfoW (HDBC arg0,
		 UWORD arg1,
		 PTR arg2,
		 SWORD arg3,
		 UNALIGNED SWORD * arg4)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETINFO;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLGetInfoW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	switch (arg1)
	{
	case SQL_ACCESSIBLE_PROCEDURES:
	case SQL_ACCESSIBLE_TABLES:
	case SQL_CATALOG_NAME:
	case SQL_CATALOG_NAME_SEPARATOR:
	case SQL_CATALOG_TERM:
	case SQL_COLLATION_SEQ:
	case SQL_COLUMN_ALIAS:
	case SQL_DATA_SOURCE_NAME:
	case SQL_DATA_SOURCE_READ_ONLY:
	case SQL_DATABASE_NAME:
	case SQL_DBMS_NAME:
	case SQL_DESCRIBE_PARAMETER:
	case SQL_DM_VER:
	case SQL_DRIVER_NAME:
	case SQL_DRIVER_ODBC_VER:
	case SQL_DRIVER_VER:
	case SQL_EXPRESSIONS_IN_ORDERBY:
	case SQL_IDENTIFIER_QUOTE_CHAR:
	case SQL_KEYWORDS:
	case SQL_LIKE_ESCAPE_CLAUSE:
	case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
	case SQL_MULT_RESULT_SETS:
	case SQL_MULTIPLE_ACTIVE_TXN:
	case SQL_NEED_LONG_DATA_LEN:
	case SQL_ODBC_VER:
	case SQL_ORDER_BY_COLUMNS_IN_SELECT:
	case SQL_PROCEDURE_TERM:
	case SQL_PROCEDURES:
	case SQL_ROW_UPDATES:
	case SQL_SCHEMA_TERM:
	case SQL_SEARCH_PATTERN_ESCAPE:
	case SQL_SERVER_NAME:
	case SQL_SPECIAL_CHARACTERS:
	case SQL_TABLE_TERM:
	case SQL_USER_NAME:
	case SQL_XOPEN_CLI_YEAR:
		lpCallStr->szArg[2]="SQLCHAR *";
		lpCallStr->atArg[2]=TYP_SQLCHARPTR;
		break;
	case SQL_ACTIVE_ENVIRONMENTS:
	case SQL_CATALOG_LOCATION:
	case SQL_CONCAT_NULL_BEHAVIOR:
	case SQL_CORRELATION_NAME:
	case SQL_CURSOR_COMMIT_BEHAVIOR:
	case SQL_CURSOR_ROLLBACK_BEHAVIOR:
	case SQL_FILE_USAGE:
	case SQL_GROUP_BY:
	case SQL_IDENTIFIER_CASE:
	case SQL_INTEGRITY:
	case SQL_MAX_CATALOG_NAME_LEN:
	case SQL_MAX_COLUMN_NAME_LEN:
	case SQL_MAX_COLUMNS_IN_GROUP_BY:
	case SQL_MAX_COLUMNS_IN_INDEX:
	case SQL_MAX_COLUMNS_IN_ORDER_BY:
	case SQL_MAX_COLUMNS_IN_SELECT:
	case SQL_MAX_COLUMNS_IN_TABLE:
	case SQL_MAX_CONCURRENT_ACTIVITIES:
	case SQL_MAX_CURSOR_NAME_LEN:
	case SQL_MAX_DRIVER_CONNECTIONS:
	case SQL_MAX_IDENTIFIER_LEN:
	case SQL_MAX_PROCEDURE_NAME_LEN:
	case SQL_MAX_SCHEMA_NAME_LEN:
	case SQL_MAX_TABLE_NAME_LEN:
	case SQL_MAX_TABLES_IN_SELECT:
	case SQL_MAX_USER_NAME_LEN:
	case SQL_NON_NULLABLE_COLUMNS:
	case SQL_NULL_COLLATION:
	case SQL_QUOTED_IDENTIFIER_CASE:
	case SQL_TXN_CAPABLE:
		lpCallStr->szArg[2]="SQLUSMALLINT *";
		lpCallStr->atArg[2]=TYP_UWORDPTR;
		break;
	case SQL_AGGREGATE_FUNCTIONS:
	case SQL_ALTER_DOMAIN:
	case SQL_ALTER_TABLE:
	case SQL_ASYNC_MODE:
	case SQL_BATCH_ROW_COUNT:
	case SQL_BATCH_SUPPORT:
	case SQL_BOOKMARK_PERSISTENCE:
	case SQL_CATALOG_USAGE:
	case SQL_CONVERT_BIGINT:
	case SQL_CONVERT_BINARY:
	case SQL_CONVERT_BIT:
	case SQL_CONVERT_CHAR:
	case SQL_CONVERT_DATE:
	case SQL_CONVERT_DECIMAL:
	case SQL_CONVERT_DOUBLE:
	case SQL_CONVERT_FLOAT:
	case SQL_CONVERT_INTEGER:
	case SQL_CONVERT_INTERVAL_YEAR_MONTH:
	case SQL_CONVERT_INTERVAL_DAY_TIME:
	case SQL_CONVERT_LONGVARBINARY:
	case SQL_CONVERT_NUMERIC:
	case SQL_CONVERT_REAL:
	case SQL_CONVERT_SMALLINT:
	case SQL_CONVERT_TIME:
	case SQL_CONVERT_TIMESTAMP:
	case SQL_CONVERT_TINYINT:
	case SQL_CONVERT_VARBINARY:
	case SQL_CONVERT_VARCHAR:
	case SQL_CONVERT_FUNCTIONS:
	case SQL_CREATE_ASSERTION:
	case SQL_CREATE_CHARACTER_SET:
	case SQL_CREATE_COLLATION:
	case SQL_CREATE_DOMAIN:
	case SQL_CREATE_SCHEMA:
	case SQL_CREATE_TABLE:
	case SQL_CREATE_TRANSLATION:
	case SQL_CREATE_VIEW:
	case SQL_CURSOR_SENSITIVITY:
	case SQL_DATETIME_LITERALS:
	case SQL_DDL_INDEX:
	case SQL_DEFAULT_TXN_ISOLATION:
	case SQL_DRIVER_HDBC:
	case SQL_DRIVER_HENV:
	case SQL_DRIVER_HDESC:
	case SQL_DRIVER_HLIB:
	case SQL_DRIVER_HSTMT:
	case SQL_DROP_ASSERTION:
	case SQL_DROP_CHARACTER_SET:
	case SQL_DROP_COLLATION:
	case SQL_DROP_DOMAIN:
	case SQL_DROP_SCHEMA:
	case SQL_DROP_TABLE:
	case SQL_DROP_TRANSLATION:
	case SQL_DROP_VIEW:
	case SQL_DYNAMIC_CURSOR_ATTRIBUTES1:
	case SQL_DYNAMIC_CURSOR_ATTRIBUTES2:
	case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1:
	case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2:
	case SQL_GETDATA_EXTENSIONS:
	case SQL_INDEX_KEYWORDS:
	case SQL_INFO_SCHEMA_VIEWS:
	case SQL_INSERT_STATEMENT:
	case SQL_KEYSET_CURSOR_ATTRIBUTES1:
	case SQL_KEYSET_CURSOR_ATTRIBUTES2:
	case SQL_MAX_ASYNC_CONCURRENT_STATEMENTS:
	case SQL_MAX_BINARY_LITERAL_LEN:
	case SQL_MAX_CHAR_LITERAL_LEN:
	case SQL_MAX_INDEX_SIZE:
	case SQL_MAX_ROW_SIZE:
	case SQL_MAX_STATEMENT_LEN:
	case SQL_NUMERIC_FUNCTIONS:
	case SQL_ODBC_INTERFACE_CONFORMANCE:
	case SQL_OJ_CAPABILITIES:
	case SQL_PARAM_ARRAY_ROW_COUNTS:
	case SQL_PARAM_ARRAY_SELECTS:
	case SQL_POS_OPERATIONS:
	case SQL_SCHEMA_USAGE:
	case SQL_SCROLL_OPTIONS:
	case SQL_SQL_CONFORMANCE:
	case SQL_SQL92_DATETIME_FUNCTIONS:
	case SQL_SQL92_FOREIGN_KEY_DELETE_RULE:
	case SQL_SQL92_FOREIGN_KEY_UPDATE_RULE:
	case SQL_SQL92_GRANT:
	case SQL_SQL92_NUMERIC_VALUE_FUNCTIONS:
	case SQL_SQL92_PREDICATES:
	case SQL_SQL92_RELATIONAL_JOIN_OPERATORS:
	case SQL_SQL92_REVOKE:
	case SQL_SQL92_ROW_VALUE_CONSTRUCTOR:
	case SQL_SQL92_STRING_FUNCTIONS:
	case SQL_STANDARD_CLI_CONFORMANCE:
	case SQL_STATIC_CURSOR_ATTRIBUTES1:
	case SQL_STATIC_CURSOR_ATTRIBUTES2:
	case SQL_STRING_FUNCTIONS:
	case SQL_SUBQUERIES:
	case SQL_SYSTEM_FUNCTIONS:
	case SQL_TIMEDATE_ADD_INTERVALS:
	case SQL_TIMEDATE_DIFF_INTERVALS:
	case SQL_TIMEDATE_FUNCTIONS:
	case SQL_TXN_ISOLATION_OPTION:
	case SQL_UNION:
		lpCallStr->szArg[2]="SQLUINTEGER *";
		lpCallStr->atArg[2]=TYP_UDWORDPTR;
		break;
	default:
		lpCallStr->szArg[2]="SQLCHAR *";
		lpCallStr->atArg[2]=TYP_SQLCHARPTR;
		break;
	}
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD";
	lpCallStr->atArg[3]=TYP_SWORD;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD *";
	lpCallStr->atArg[4]=TYP_SWORDPTR;
	lpCallStr->nArgs = 5;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetTypeInfo /////

extern "C" RETCODE SQL_API TraceSQLGetTypeInfoW (HSTMT arg0,
		 SWORD arg1)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETTYPEINFO;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLGetTypeInfoW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SWORD";
	lpCallStr->atArg[1]=TYP_SWORD;
	lpCallStr->nArgs = 2;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSetConnectOption /////

extern "C" RETCODE SQL_API TraceSQLSetConnectOptionW (HDBC arg0,
		 UWORD arg1,
		 UDWORD_P arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSETCONNECTOPTION;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLSetConnectOptionW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="UDWORD";
	lpCallStr->atArg[2]=TYP_UDWORD;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_CONFIG)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSpecialColumns /////

extern "C" RETCODE SQL_API TraceSQLSpecialColumnsW (HSTMT arg0,
		 UWORD arg1,
		 WCHAR * arg2,
		 SWORD arg3,
		 WCHAR * arg4,
		 SWORD arg5,
		 WCHAR * arg6,
		 SWORD arg7,
		 UWORD arg8,
		 UWORD arg9)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSPECIALCOLUMNS;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLSpecialColumnsW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="WCHAR *";
	lpCallStr->atArg[2]=TYP_WCHARPTR;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD";
	lpCallStr->atArg[3]=TYP_SWORD;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="WCHAR *";
	lpCallStr->atArg[4]=TYP_WCHARPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SWORD";
	lpCallStr->atArg[5]=TYP_SWORD;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="WCHAR *";
	lpCallStr->atArg[6]=TYP_WCHARPTR;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SWORD";
	lpCallStr->atArg[7]=TYP_SWORD;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="UWORD";
	lpCallStr->atArg[8]=TYP_UWORD;
	lpCallStr->lpvArg[9] = (LPVOID) arg9;
	lpCallStr->szArg[9]="UWORD";
	lpCallStr->atArg[9]=TYP_UWORD;
	lpCallStr->nArgs = 10;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLStatistics /////

extern "C" RETCODE SQL_API TraceSQLStatisticsW (HSTMT arg0,
		 WCHAR * arg1,
		 SWORD arg2,
		 WCHAR * arg3,
		 SWORD arg4,
		 WCHAR * arg5,
		 SWORD arg6,
		 UWORD arg7,
		 UWORD arg8)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSTATISTICS;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLStatisticsW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="WCHAR *";
	lpCallStr->atArg[1]=TYP_WCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="WCHAR *";
	lpCallStr->atArg[3]=TYP_WCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="WCHAR *";
	lpCallStr->atArg[5]=TYP_WCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="UWORD";
	lpCallStr->atArg[7]=TYP_UWORD;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="UWORD";
	lpCallStr->atArg[8]=TYP_UWORD;
	lpCallStr->nArgs = 9;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLTables /////

extern "C" RETCODE SQL_API TraceSQLTablesW (HSTMT arg0,
		 WCHAR * arg1,
		 SWORD arg2,
		 WCHAR * arg3,
		 SWORD arg4,
		 WCHAR * arg5,
		 SWORD arg6,
		 WCHAR * arg7,
		 SWORD arg8)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLTABLES;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLTablesW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="WCHAR *";
	lpCallStr->atArg[1]=TYP_WCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="WCHAR *";
	lpCallStr->atArg[3]=TYP_WCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="WCHAR *";
	lpCallStr->atArg[5]=TYP_WCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="WCHAR *";
	lpCallStr->atArg[7]=TYP_WCHARPTR;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="SWORD";
	lpCallStr->atArg[8]=TYP_SWORD;
	lpCallStr->nArgs = 9;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLBrowseConnect /////

extern "C" RETCODE SQL_API TraceSQLBrowseConnectW (HDBC arg0,
		 WCHAR * arg1,
		 SWORD arg2,
		 WCHAR * arg3,
		 SWORD arg4,
		 UNALIGNED SWORD * arg5)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLBROWSECONNECT;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLBrowseConnectW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="WCHAR *";
	lpCallStr->atArg[1]=TYP_WCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="WCHAR *";
	lpCallStr->atArg[3]=TYP_WCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SWORD *";
	lpCallStr->atArg[5]=TYP_SWORDPTR;
	lpCallStr->nArgs = 6;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLColumnPrivileges /////

extern "C" RETCODE SQL_API TraceSQLColumnPrivilegesW (HSTMT arg0,
		 WCHAR * arg1,
		 SWORD arg2,
		 WCHAR * arg3,
		 SWORD arg4,
		 WCHAR * arg5,
		 SWORD arg6,
		 WCHAR * arg7,
		 SWORD arg8)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLCOLUMNPRIVILEGES;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLColumnPrivilegesW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="WCHAR *";
	lpCallStr->atArg[1]=TYP_WCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="WCHAR *";
	lpCallStr->atArg[3]=TYP_WCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="WCHAR *";
	lpCallStr->atArg[5]=TYP_WCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="WCHAR *";
	lpCallStr->atArg[7]=TYP_WCHARPTR;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="SWORD";
	lpCallStr->atArg[8]=TYP_SWORD;
	lpCallStr->nArgs = 9;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLDataSources /////

extern "C" RETCODE SQL_API TraceSQLDataSourcesW (HENV arg0,
		 UWORD arg1,
		 WCHAR * arg2,
		 SWORD arg3,
		 SWORD * arg4,
		 WCHAR * arg5,
		 SWORD arg6,
		 SWORD * arg7)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLDATASOURCES;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLDataSourcesW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HENV";
	lpCallStr->atArg[0]=TYP_HENV;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="WCHAR *";
	lpCallStr->atArg[2]=TYP_WCHARPTR;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD";
	lpCallStr->atArg[3]=TYP_SWORD;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD *";
	lpCallStr->atArg[4]=TYP_SWORDPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="WCHAR *";
	lpCallStr->atArg[5]=TYP_WCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SWORD *";
	lpCallStr->atArg[7]=TYP_SWORDPTR;
	lpCallStr->nArgs = 8;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLForeignKeys /////

extern "C" RETCODE SQL_API TraceSQLForeignKeysW (HSTMT arg0,
		 WCHAR * arg1,
		 SWORD arg2,
		 WCHAR * arg3,
		 SWORD arg4,
		 WCHAR * arg5,
		 SWORD arg6,
		 WCHAR * arg7,
		 SWORD arg8,
		 WCHAR * arg9,
		 SWORD arg10,
		 WCHAR * arg11,
		 SWORD arg12)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLFOREIGNKEYS;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLForeignKeysW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="WCHAR *";
	lpCallStr->atArg[1]=TYP_WCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="WCHAR *";
	lpCallStr->atArg[3]=TYP_WCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="WCHAR *";
	lpCallStr->atArg[5]=TYP_WCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="WCHAR *";
	lpCallStr->atArg[7]=TYP_WCHARPTR;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="SWORD";
	lpCallStr->atArg[8]=TYP_SWORD;
	lpCallStr->lpvArg[9] = (LPVOID) arg9;
	lpCallStr->szArg[9]="WCHAR *";
	lpCallStr->atArg[9]=TYP_WCHARPTR;
	lpCallStr->lpvArg[10] = (LPVOID) arg10;
	lpCallStr->szArg[10]="SWORD";
	lpCallStr->atArg[10]=TYP_SWORD;
	lpCallStr->lpvArg[11] = (LPVOID) arg11;
	lpCallStr->szArg[11]="WCHAR *";
	lpCallStr->atArg[11]=TYP_WCHARPTR;
	lpCallStr->lpvArg[12] = (LPVOID) arg12;
	lpCallStr->szArg[12]="SWORD";
	lpCallStr->atArg[12]=TYP_SWORD;
	lpCallStr->nArgs = 13;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLNativeSql /////

extern "C" RETCODE SQL_API TraceSQLNativeSqlW (HDBC arg0,
		 WCHAR * arg1,
		 SDWORD_P arg2,
		 WCHAR * arg3,
		 SDWORD_P arg4,
		 UNALIGNED SDWORD_P * arg5)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLNATIVESQL;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLNativeSqlW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HDBC";
	lpCallStr->atArg[0]=TYP_HDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="WCHAR *";
	lpCallStr->atArg[1]=TYP_WCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SDWORD";
	lpCallStr->atArg[2]=TYP_SDWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="WCHAR *";
	lpCallStr->atArg[3]=TYP_WCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SDWORD";
	lpCallStr->atArg[4]=TYP_SDWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SDWORD *";
	lpCallStr->atArg[5]=TYP_SDWORDPTR;
	lpCallStr->nArgs = 6;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLPrimaryKeys /////

extern "C" RETCODE SQL_API TraceSQLPrimaryKeysW (HSTMT arg0,
		 WCHAR * arg1,
		 SWORD arg2,
		 WCHAR * arg3,
		 SWORD arg4,
		 WCHAR * arg5,
		 SWORD arg6)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLPRIMARYKEYS;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLPrimaryKeysW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="WCHAR *";
	lpCallStr->atArg[1]=TYP_WCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="WCHAR *";
	lpCallStr->atArg[3]=TYP_WCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="WCHAR *";
	lpCallStr->atArg[5]=TYP_WCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->nArgs = 7;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLProcedureColumns /////

extern "C" RETCODE SQL_API TraceSQLProcedureColumnsW (HSTMT arg0,
		 WCHAR * arg1,
		 SWORD arg2,
		 WCHAR * arg3,
		 SWORD arg4,
		 WCHAR * arg5,
		 SWORD arg6,
		 WCHAR * arg7,
		 SWORD arg8)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLPROCEDURECOLUMNS;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLProcedureColumnsW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="WCHAR *";
	lpCallStr->atArg[1]=TYP_WCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="WCHAR *";
	lpCallStr->atArg[3]=TYP_WCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="WCHAR *";
	lpCallStr->atArg[5]=TYP_WCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="WCHAR *";
	lpCallStr->atArg[7]=TYP_WCHARPTR;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="SWORD";
	lpCallStr->atArg[8]=TYP_SWORD;
	lpCallStr->nArgs = 9;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLProcedures /////

extern "C" RETCODE SQL_API TraceSQLProceduresW (HSTMT arg0,
		 WCHAR * arg1,
		 SWORD arg2,
		 WCHAR * arg3,
		 SWORD arg4,
		 WCHAR * arg5,
		 SWORD arg6)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLPROCEDURES;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLProceduresW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="WCHAR *";
	lpCallStr->atArg[1]=TYP_WCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="WCHAR *";
	lpCallStr->atArg[3]=TYP_WCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="WCHAR *";
	lpCallStr->atArg[5]=TYP_WCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->nArgs = 7;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLTablePrivileges /////

extern "C" RETCODE SQL_API TraceSQLTablePrivilegesW (HSTMT arg0,
		 WCHAR * arg1,
		 SWORD arg2,
		 WCHAR * arg3,
		 SWORD arg4,
		 WCHAR * arg5,
		 SWORD arg6)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLTABLEPRIVILEGES;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLTablePrivilegesW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HSTMT";
	lpCallStr->atArg[0]=TYP_HSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="WCHAR *";
	lpCallStr->atArg[1]=TYP_WCHARPTR;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SWORD";
	lpCallStr->atArg[2]=TYP_SWORD;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="WCHAR *";
	lpCallStr->atArg[3]=TYP_WCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD";
	lpCallStr->atArg[4]=TYP_SWORD;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="WCHAR *";
	lpCallStr->atArg[5]=TYP_WCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->nArgs = 7;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLDrivers /////

extern "C" RETCODE SQL_API TraceSQLDriversW (HENV arg0,
		 UWORD arg1,
		 WCHAR * arg2,
		 SWORD arg3,
		 SWORD * arg4,
		 WCHAR * arg5,
		 SWORD arg6,
		 SWORD * arg7)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLDRIVERS;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLDriversW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="HENV";
	lpCallStr->atArg[0]=TYP_HENV;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="UWORD";
	lpCallStr->atArg[1]=TYP_UWORD;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="WCHAR *";
	lpCallStr->atArg[2]=TYP_WCHARPTR;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SWORD";
	lpCallStr->atArg[3]=TYP_SWORD;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SWORD *";
	lpCallStr->atArg[4]=TYP_SWORDPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="WCHAR *";
	lpCallStr->atArg[5]=TYP_WCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SWORD";
	lpCallStr->atArg[6]=TYP_SWORD;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SWORD *";
	lpCallStr->atArg[7]=TYP_SWORDPTR;
	lpCallStr->nArgs = 8;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLColAttribute /////

extern "C" RETCODE SQL_API TraceSQLColAttributeW (SQLHSTMT arg0,
		 SQLSMALLINT arg1,
		 SQLSMALLINT arg2,
		 SQLPOINTER arg3,
		 SQLSMALLINT arg4,
		 UNALIGNED SQLSMALLINT * arg5,
		 SQLPOINTER arg6)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLCOLATTRIBUTE;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLColAttributeW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHSTMT";
	lpCallStr->atArg[0]=TYP_SQLHSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLSMALLINT";
	lpCallStr->atArg[1]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLSMALLINT";
	lpCallStr->atArg[2]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLPOINTER";
	lpCallStr->atArg[3]=TYP_SQLPOINTER;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLSMALLINT";
	lpCallStr->atArg[4]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SQLSMALLINT *";
	lpCallStr->atArg[5]=TYP_SQLSMALLINTPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SQLPOINTER";
	lpCallStr->atArg[6]=TYP_SQLPOINTER;
	lpCallStr->nArgs = 7;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetConnectAttr /////

extern "C" RETCODE SQL_API TraceSQLGetConnectAttrW (SQLHDBC arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3,
		 UNALIGNED SQLINTEGER * arg4)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETCONNECTATTR;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLGetConnectAttrW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHDBC";
	lpCallStr->atArg[0]=TYP_SQLHDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLINTEGER";
	lpCallStr->atArg[1]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLPOINTER";
	lpCallStr->atArg[2]=TYP_SQLPOINTER;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLINTEGER";
	lpCallStr->atArg[3]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLINTEGER *";
	lpCallStr->atArg[4]=TYP_SQLINTEGERPTR;
	lpCallStr->nArgs = 5;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetDescField /////

extern "C" RETCODE SQL_API TraceSQLGetDescFieldW (SQLHDESC arg0,
		 SQLSMALLINT arg1,
		 SQLSMALLINT arg2,
		 SQLPOINTER arg3,
		 SQLINTEGER arg4,
		 UNALIGNED SQLINTEGER * arg5)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETDESCFIELD;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLGetDescFieldW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHDESC";
	lpCallStr->atArg[0]=TYP_SQLHDESC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLSMALLINT";
	lpCallStr->atArg[1]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLSMALLINT";
	lpCallStr->atArg[2]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	switch (arg2)
	{
	default:
		lpCallStr->szArg[3]="SQLPOINTER";
		lpCallStr->atArg[3]=TYP_SQLPOINTER;
		break;
	case SQL_DESC_ARRAY_STATUS_PTR:
	case SQL_DESC_BIND_OFFSET_PTR:
	case SQL_DESC_ROWS_PROCESSED_PTR:
	case SQL_DESC_DATA_PTR:
	case SQL_DESC_INDICATOR_PTR:
	case SQL_DESC_OCTET_LENGTH_PTR:
		lpCallStr->szArg[3]="SQLPOINTER*";
		lpCallStr->atArg[3]=TYP_SQLHANDLEPTR;
		break;
	case SQL_DESC_BASE_COLUMN_NAME:
	case SQL_DESC_BASE_TABLE_NAME:
	case SQL_DESC_CATALOG_NAME:
	case SQL_DESC_LABEL:
	case SQL_DESC_LITERAL_PREFIX:
	case SQL_DESC_LITERAL_SUFFIX:
	case SQL_DESC_LOCAL_TYPE_NAME:
	case SQL_DESC_NAME:
	case SQL_DESC_SCHEMA_NAME:
	case SQL_DESC_TABLE_NAME:
	case SQL_DESC_TYPE_NAME:
		lpCallStr->szArg[3]="CHAR *";
		lpCallStr->atArg[3]=TYP_SQLCHARPTR;
		break;
	case SQL_DESC_ALLOC_TYPE:
	case SQL_DESC_COUNT:
	case SQL_DESC_CONCISE_TYPE:
	case SQL_DESC_DATETIME_INTERVAL_CODE:
	case SQL_DESC_FIXED_PREC_SCALE:
	case SQL_DESC_NULLABLE:
	case SQL_DESC_PARAMETER_TYPE:
	case SQL_DESC_PRECISION:
	case SQL_DESC_SCALE:
	case SQL_DESC_SEARCHABLE:
	case SQL_DESC_TYPE:
	case SQL_DESC_UNNAMED:
	case SQL_DESC_UNSIGNED:
	case SQL_DESC_UPDATABLE:
		lpCallStr->szArg[3]="SMALLINTEGER *";
		lpCallStr->atArg[3]=TYP_SQLSMALLINTPTR;
		break;
	case SQL_DESC_AUTO_UNIQUE_VALUE:
	case SQL_DESC_CASE_SENSITIVE:
	case SQL_DESC_BIND_TYPE:
	case SQL_DESC_DATETIME_INTERVAL_PRECISION:
	case SQL_DESC_DISPLAY_SIZE:
	case SQL_DESC_NUM_PREC_RADIX:
	case SQL_DESC_OCTET_LENGTH:
		lpCallStr->szArg[3]="INTEGER *";
		lpCallStr->atArg[3]=TYP_SQLINTEGERPTR;
		break;
	case SQL_DESC_ARRAY_SIZE:
	case SQL_DESC_LENGTH:
		lpCallStr->szArg[3]="UINTEGER *";
		lpCallStr->atArg[3]=TYP_UDWORDPTR;
		break;
	}
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLINTEGER";
	lpCallStr->atArg[4]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SQLINTEGER *";
	lpCallStr->atArg[5]=TYP_SQLINTEGERPTR;
	lpCallStr->nArgs = 6;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetDescRec /////

extern "C" RETCODE SQL_API TraceSQLGetDescRecW (SQLHDESC arg0,
		 SQLSMALLINT arg1,
		 SQLWCHAR * arg2,
		 SQLSMALLINT arg3,
		 UNALIGNED SQLSMALLINT * arg4,
		 UNALIGNED SQLSMALLINT * arg5,
		 UNALIGNED SQLSMALLINT * arg6,
		 UNALIGNED SQLINTEGER  * arg7,
		 UNALIGNED SQLSMALLINT * arg8,
		 UNALIGNED SQLSMALLINT * arg9,
		 UNALIGNED SQLSMALLINT * arg10)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETDESCREC;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLGetDescRecW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHDESC";
	lpCallStr->atArg[0]=TYP_SQLHDESC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLSMALLINT";
	lpCallStr->atArg[1]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLWCHAR *";
	lpCallStr->atArg[2]=TYP_SQLWCHARPTR;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLSMALLINT";
	lpCallStr->atArg[3]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLSMALLINT *";
	lpCallStr->atArg[4]=TYP_SQLSMALLINTPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SQLSMALLINT *";
	lpCallStr->atArg[5]=TYP_SQLSMALLINTPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SQLSMALLINT *";
	lpCallStr->atArg[6]=TYP_SQLSMALLINTPTR;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SQLINTEGER *";
	lpCallStr->atArg[7]=TYP_SQLINTEGERPTR;
	lpCallStr->lpvArg[8] = (LPVOID) arg8;
	lpCallStr->szArg[8]="SQLSMALLINT *";
	lpCallStr->atArg[8]=TYP_SQLSMALLINTPTR;
	lpCallStr->lpvArg[9] = (LPVOID) arg9;
	lpCallStr->szArg[9]="SQLSMALLINT *";
	lpCallStr->atArg[9]=TYP_SQLSMALLINTPTR;
	lpCallStr->lpvArg[10] = (LPVOID) arg10;
	lpCallStr->szArg[10]="SQLSMALLINT *";
	lpCallStr->atArg[10]=TYP_SQLSMALLINTPTR;
	lpCallStr->nArgs = 11;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetDiagField /////

extern "C" RETCODE SQL_API TraceSQLGetDiagFieldW (SQLSMALLINT arg0,
		 SQLHANDLE arg1,
		 SQLSMALLINT arg2,
		 SQLSMALLINT arg3,
		 SQLPOINTER arg4,
		 SQLSMALLINT arg5,
		 UNALIGNED SQLSMALLINT * arg6)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETDIAGFIELD;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLGetDiagFieldW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLSMALLINT";
	lpCallStr->atArg[0]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLHANDLE";
	lpCallStr->atArg[1]=TYP_SQLHANDLE;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLSMALLINT";
	lpCallStr->atArg[2]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLSMALLINT";
	lpCallStr->atArg[3]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLPOINTER";
	lpCallStr->atArg[4]=TYP_SQLPOINTER;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SQLSMALLINT";
	lpCallStr->atArg[5]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SQLSMALLINT *";
	lpCallStr->atArg[6]=TYP_SQLSMALLINTPTR;
	lpCallStr->nArgs = 7;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetDiagRec /////

extern "C" RETCODE SQL_API TraceSQLGetDiagRecW (SQLSMALLINT arg0,
		 SQLHANDLE arg1,
		 SQLSMALLINT arg2,
		 SQLWCHAR * arg3,
		 UNALIGNED SQLINTEGER * arg4,
		 SQLWCHAR * arg5,
		 SQLSMALLINT arg6,
		 UNALIGNED SQLSMALLINT * arg7)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETDIAGREC;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLGetDiagRecW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLSMALLINT";
	lpCallStr->atArg[0]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLHANDLE";
	lpCallStr->atArg[1]=TYP_SQLHANDLE;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLSMALLINT";
	lpCallStr->atArg[2]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLWCHAR *";
	lpCallStr->atArg[3]=TYP_SQLWCHARPTR;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLINTEGER *";
	lpCallStr->atArg[4]=TYP_SQLINTEGERPTR;
	lpCallStr->lpvArg[5] = (LPVOID) arg5;
	lpCallStr->szArg[5]="SQLWCHAR *";
	lpCallStr->atArg[5]=TYP_SQLWCHARPTR;
	lpCallStr->lpvArg[6] = (LPVOID) arg6;
	lpCallStr->szArg[6]="SQLSMALLINT";
	lpCallStr->atArg[6]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[7] = (LPVOID) arg7;
	lpCallStr->szArg[7]="SQLSMALLINT *";
	lpCallStr->atArg[7]=TYP_SQLSMALLINTPTR;
	lpCallStr->nArgs = 8;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLGetStmtAttr /////

extern "C" RETCODE SQL_API TraceSQLGetStmtAttrW (SQLHSTMT arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3,
		 UNALIGNED SQLINTEGER * arg4)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLGETSTMTATTR;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLGetStmtAttrW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHSTMT";
	lpCallStr->atArg[0]=TYP_SQLHSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLINTEGER";
	lpCallStr->atArg[1]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	switch(arg1)
	{
	case SQL_ATTR_APP_PARAM_DESC:
	case SQL_ATTR_APP_ROW_DESC:
	case SQL_ATTR_IMP_PARAM_DESC:
	case SQL_ATTR_IMP_ROW_DESC:
		lpCallStr->szArg[2]="HANDLE *";
		lpCallStr->atArg[2]=TYP_SQLHANDLEPTR;
		break;
	case SQL_ATTR_PARAM_OPERATION_PTR:
	case SQL_ATTR_PARAM_STATUS_PTR:
	case SQL_ATTR_ROW_OPERATION_PTR:
	case SQL_ATTR_ROW_STATUS_PTR:
		lpCallStr->szArg[2]="USMALLIT *";
		lpCallStr->atArg[2]=TYP_UWORDPTR;
		break;
	default:
		lpCallStr->szArg[2]="UINTEGER *";
		lpCallStr->atArg[2]=TYP_UDWORDPTR;
		break;
	}
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLINTEGER";
	lpCallStr->atArg[3]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLINTEGER *";
	lpCallStr->atArg[4]=TYP_SQLINTEGERPTR;
	lpCallStr->nArgs = 5;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSetConnectAttr /////

extern "C" RETCODE SQL_API TraceSQLSetConnectAttrW (SQLHDBC arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSETCONNECTATTR;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLSetConnectAttrW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHDBC";
	lpCallStr->atArg[0]=TYP_SQLHDBC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLINTEGER";
	lpCallStr->atArg[1]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLPOINTER";
	lpCallStr->atArg[2]=TYP_SQLPOINTER;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLINTEGER";
	lpCallStr->atArg[3]=TYP_SQLINTEGER;
	lpCallStr->nArgs = 4;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_CONFIG)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSetDescField /////

extern "C" RETCODE SQL_API TraceSQLSetDescFieldW (SQLHDESC arg0,
		 SQLSMALLINT arg1,
		 SQLSMALLINT arg2,
		 SQLPOINTER arg3,
		 SQLINTEGER arg4)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSETDESCFIELD;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLSetDescFieldW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHDESC";
	lpCallStr->atArg[0]=TYP_SQLHDESC;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLSMALLINT";
	lpCallStr->atArg[1]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLSMALLINT";
	lpCallStr->atArg[2]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	switch (arg2)
	{
	default:
		lpCallStr->szArg[3]="SQLPOINTER";
		lpCallStr->atArg[3]=TYP_SQLPOINTER;
		break;
	case SQL_DESC_ARRAY_STATUS_PTR:
	case SQL_DESC_BIND_OFFSET_PTR:
	case SQL_DESC_ROWS_PROCESSED_PTR:
	case SQL_DESC_DATA_PTR:
	case SQL_DESC_INDICATOR_PTR:
	case SQL_DESC_OCTET_LENGTH_PTR:
		lpCallStr->szArg[3]="SQLPOINTER*";
		lpCallStr->atArg[3]=TYP_SQLHANDLEPTR;
		break;
	case SQL_DESC_BASE_COLUMN_NAME:
	case SQL_DESC_BASE_TABLE_NAME:
	case SQL_DESC_CATALOG_NAME:
	case SQL_DESC_LABEL:
	case SQL_DESC_LITERAL_PREFIX:
	case SQL_DESC_LITERAL_SUFFIX:
	case SQL_DESC_LOCAL_TYPE_NAME:
	case SQL_DESC_NAME:
	case SQL_DESC_SCHEMA_NAME:
	case SQL_DESC_TABLE_NAME:
	case SQL_DESC_TYPE_NAME:
		lpCallStr->szArg[3]="CHAR *";
		lpCallStr->atArg[3]=TYP_SQLCHARPTR;
		break;
	case SQL_DESC_ALLOC_TYPE:
	case SQL_DESC_COUNT:
	case SQL_DESC_CONCISE_TYPE:
	case SQL_DESC_DATETIME_INTERVAL_CODE:
	case SQL_DESC_FIXED_PREC_SCALE:
	case SQL_DESC_NULLABLE:
	case SQL_DESC_PARAMETER_TYPE:
	case SQL_DESC_PRECISION:
	case SQL_DESC_SCALE:
	case SQL_DESC_SEARCHABLE:
	case SQL_DESC_TYPE:
	case SQL_DESC_UNNAMED:
	case SQL_DESC_UNSIGNED:
	case SQL_DESC_UPDATABLE:
		lpCallStr->szArg[3]="SMALLINTEGER *";
		lpCallStr->atArg[3]=TYP_SQLSMALLINTPTR;
		break;
	case SQL_DESC_AUTO_UNIQUE_VALUE:
	case SQL_DESC_CASE_SENSITIVE:
	case SQL_DESC_BIND_TYPE:
	case SQL_DESC_DATETIME_INTERVAL_PRECISION:
	case SQL_DESC_DISPLAY_SIZE:
	case SQL_DESC_NUM_PREC_RADIX:
	case SQL_DESC_OCTET_LENGTH:
		lpCallStr->szArg[3]="INTEGER *";
		lpCallStr->atArg[3]=TYP_SQLINTEGERPTR;
		break;
	case SQL_DESC_ARRAY_SIZE:
	case SQL_DESC_LENGTH:
		lpCallStr->szArg[3]="UINTEGER *";
		lpCallStr->atArg[3]=TYP_UDWORDPTR;
		break;
	}
	lpCallStr->lpvArg[4] = (LPVOID) arg4;
	lpCallStr->szArg[4]="SQLINTEGER";
	lpCallStr->atArg[4]=TYP_SQLINTEGER;
	lpCallStr->nArgs = 5;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLSetStmtAttr /////

extern "C" RETCODE SQL_API TraceSQLSetStmtAttrW (SQLHSTMT arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLSETSTMTATTR;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLSetStmtAttrW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLHSTMT";
	lpCallStr->atArg[0]=TYP_SQLHSTMT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLINTEGER";
	lpCallStr->atArg[1]=TYP_SQLINTEGER;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	switch(arg1)
	{
	case SQL_ATTR_APP_PARAM_DESC:
	case SQL_ATTR_APP_ROW_DESC:
	case SQL_ATTR_IMP_PARAM_DESC:
	case SQL_ATTR_IMP_ROW_DESC:
		lpCallStr->szArg[2]="HANDLE *";
		lpCallStr->atArg[2]=TYP_SQLHANDLEPTR;
		break;
	case SQL_ATTR_PARAM_OPERATION_PTR:
	case SQL_ATTR_PARAM_STATUS_PTR:
	case SQL_ATTR_ROW_OPERATION_PTR:
	case SQL_ATTR_ROW_STATUS_PTR:
		lpCallStr->szArg[2]="USMALLIT *";
		lpCallStr->atArg[2]=TYP_UWORDPTR;
		break;
	default:
		lpCallStr->szArg[2]="UINTEGER *";
		lpCallStr->atArg[2]=TYP_UDWORDPTR;
		break;
	}
	lpCallStr->lpvArg[3] = (LPVOID) arg3;
	lpCallStr->szArg[3]="SQLINTEGER";
	lpCallStr->atArg[3]=TYP_SQLINTEGER;
	lpCallStr->nArgs = 4;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_CONFIG)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}

///// Trace function for SQLAllocHandleStd /////

extern "C" RETCODE SQL_API TraceSQLAllocHandleStdW (SQLSMALLINT arg0,
		 SQLHANDLE arg1,
		 SQLHANDLE * arg2)
{
#ifndef DISABLE_TRACE
	LPTRACESTR	lpCallStr = (LPTRACESTR) malloc(sizeof(TRACESTR));
	if (lpCallStr == NULL)
		return 0;
	memset(lpCallStr,0,sizeof(TRACESTR));
	lpCallStr->dwCallFunc = SQL_API_SQLALLOCHANDLESTD;
	lpCallStr->dwFlags |= TRACESTR_FLAG_UNICODE;
	lpCallStr->szFuncName="SQLAllocHandleStdW";
	lpCallStr->lpvArg[0] = (LPVOID) arg0;
	lpCallStr->szArg[0]="SQLSMALLINT";
	lpCallStr->atArg[0]=TYP_SQLSMALLINT;
	lpCallStr->lpvArg[1] = (LPVOID) arg1;
	lpCallStr->szArg[1]="SQLHANDLE";
	lpCallStr->atArg[1]=TYP_SQLHANDLE;
	lpCallStr->lpvArg[2] = (LPVOID) arg2;
	lpCallStr->szArg[2]="SQLHANDLE *";
	lpCallStr->atArg[2]=TYP_SQLHANDLEPTR;
	lpCallStr->nArgs = 3;
	lpCallStr->wPrintMarker = TRUE;
	if  (gTraceFlags & TR_ODBC_INFO)
		DoTrace(lpCallStr);
	return((RETCODE)SetNextHandle(lpCallStr));
#else
	return 0;
#endif
}
