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
//=========================================================
// This function is used by both the driver and server sides of
// ODBC to provide a common place for trace diagnostics.
//=========================================================
//
// NOTE: There is a problem compiling this file under the
//       driver project (which is an MFC DLL) and any server
//       project (which in NOT an MFC DLL).  The following
//       #ifdef takes care of those problems.
// 
#ifdef _AFXDLL
#include "stdafx.h"
#else
#include <platform_ndcs.h>
#endif
//=========================================================

#include <string.h>
#include "CommonDiags.h"
#include <platform_utils.h>

//=========================================================
// Global variables used in tracing
//=========================================================
long diagnostic_flags = 0; // used for dynamic tracing

char g_ElapsedTime[24];

//=========================================================
// Procedures for trace "new" "delete"
//=========================================================
#ifndef DRIVER

long trace_memory = 0;

//LCOV_EXCL_START
void LogNew( char* line, void** ppointer, void* pointer, long length )
{
	line=line;
	ppointer=ppointer;
	pointer=pointer;
	length=length;
}

void LogDelete( char* line, void** ppointer, void* pointer )
{
	line=line;
	ppointer=ppointer;
	pointer=pointer;
}

#endif

//=========================================================
// These trace option strings need to match the #defines
// in CommonDiags.h
//=========================================================
static char *TraceOptionString[] =
    {	"",					/* 0x00000001   TRACE ON/OFF*/
		"ODBC_MX_API     ",	/* 0x00000002   */
		"ODBC_MX_API_EXIT",	/* 0x00000004	*/
		"KRYPTON		 ",	/* 0x00000008   */
		"SRVR_KRYPTON    ",	/* 0x00000010   */
		"SQL_API         ",	/* 0x00000020   */
		"POST_ERROR      ",	/* 0x00000040   */
		"CONFIG          ",	/* 0x00000080   */
		"ASSOC_SRVR      ",	/* 0x00000100   */
		"SQL_API_EXIT    ",	/* 0x00000200   */
		"UNKNOWN11",		/* 0x00000400   */
		"UNKNOWN12",		/* 0x00000800   */
		"UNKNOWN13",		/* 0x00001000   */
		"UNKNOWN14",		/* 0x00002000   */
		"UNKNOWN15",		/* 0x00004000   */
		"UNKNOWN16"			/* 0x00008000   */
    };

#define DEBUG_BUFFER_SIZE	2048 	    /* Maximum string length */

//=========================================================
// This function returns a pointer to a character string
// representation of the elapsed time since <StartTime>
// in the form ss.mmm.  The character string is
// contained in a global array.
//=========================================================
char *ElapsedTimeString(struct _timeb StartTime)
{
	char b[30];
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

	sprintf(b,"%ld", ElapsedSeconds);
	snprintf(g_ElapsedTime,24,"%.15s.%03d sec",b,ElapsedMilliSec);

	return g_ElapsedTime;
}

void LogFile( char* txt1, char* txt2, char* txt3 )
{
	char fileName[80];
	FILE* pFile = NULL;
	char buffer[501];

	sprintf( fileName, "P%d",GetCurrentProcessId());
	if((pFile = fopen( fileName, "a" )) == NULL ) return;

	if (*txt2 != 0)
		sprintf( buffer, "%.158s---------->%.158s---------->%.158s\n",txt1,txt2,txt3);
	else
		sprintf(buffer,"%.244s---------->%.244s\n",txt1,txt3);
	fputs( buffer, pFile );

	fclose( pFile );
}

static char *TraceOptionToString(long TraceOption)
{
	long index;

	if (TraceOption == 0) index = 15;	    
	else {
		for (index = 0; (TraceOption & 1) == 0; TraceOption >>= 1, index++);
		}

	return TraceOptionString[index];
}

enum {FORMAT_TEXT,FORMAT_DUMP};

void TraceOut(long TraceOption, char *text, ...)
{
#ifdef DRIVER

	pEnterCSection();
	   
	if (pTrace_flags && (*pTrace_flags & TraceOption))
	{
		char *p;
		char buffer[DEBUG_BUFFER_SIZE];
		p = (char *)&text + (long) sizeof (char *);   // Get VA_ARG pointer    
		_vsnprintf(buffer, DEBUG_BUFFER_SIZE, text, p);		   

		pDebugOut(strlen(buffer), buffer, TraceOptionToString(TraceOption), FORMAT_TEXT);
	}

	pExitCSection();

#endif
}


void HexDump(  unsigned Len,char *Data, LPSTR TextString )
{
#ifdef DRIVER

	pEnterCSection();

	pDebugOut(Len, Data, TextString, FORMAT_DUMP);

	pExitCSection();

#endif
}  //  end of HexDump
// ************************************************************


char* GetExceptionMessage( unsigned long code )
{
	switch( code )
	{
	case EXCEPTION_ACCESS_VIOLATION:
		return "EXCEPTION_ACCESS_VIOLATION";
	case EXCEPTION_BREAKPOINT:
		return "EXCEPTION_BREAKPOINT";
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		return "EXCEPTION_DATATYPE_MISALIGNMENT";
	case EXCEPTION_SINGLE_STEP:
		return "EXCEPTION_SINGLE_STEP";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		return "EXCEPTION_FLT_DENORMAL_OPERAND";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
	case EXCEPTION_FLT_INEXACT_RESULT:
		return "EXCEPTION_FLT_INEXACT_RESULT";
	case EXCEPTION_FLT_INVALID_OPERATION:
		return "EXCEPTION_FLT_INVALID_OPERATION";
	case EXCEPTION_FLT_OVERFLOW:
		return "EXCEPTION_FLT_OVERFLOW";
	case EXCEPTION_FLT_STACK_CHECK:
		return "EXCEPTION_FLT_STACK_CHECK";
	case EXCEPTION_FLT_UNDERFLOW:
		return "EXCEPTION_FLT_UNDERFLOW";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		return "EXCEPTION_INT_DIVIDE_BY_ZERO";
	case EXCEPTION_INT_OVERFLOW:
		return "EXCEPTION_INT_OVERFLOW";
	case EXCEPTION_PRIV_INSTRUCTION:
		return "EXCEPTION_PRIV_INSTRUCTION";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
	default:
		return "EXCEPTION_UNKNOWN";

	}
}
//LCOV_EXCL_STOP
