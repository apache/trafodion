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
//
//  CommonDiags.h - Shared Diagnostic Stuff
//

#ifndef COMMONDIAGS_DEFINED
#define COMMONDIAGS_DEFINED

//=====================================================
// Defines used in automatic testing tool
//=====================================================

#define _TEST_POINTS

//=====================================================
// Driver defines ( 51 - 100 )
//=====================================================

//=====================================================
// Server defines ( 1 - 50 ) 
//=====================================================

// For WAIT_OPERATION, DEBUG_OPERATION, KILL_OPERATION

#define SET_AUTOCOMMIT_OFF		1
#define END_TRANSACTION			2
#define TIP_SUSPEND				3
#define TIP_RESUME				4
#define GET_TIP_URL				5
#define TIP_OPEN				6
#define TIP_PULL				7

// For CUST_OPERATION

#define GET_WRONG_TIP_URL		8
#define TIP_SUSPEND_RETCODE		9
#define TIP_RESUME_RETCODE		10
#define TIP_OPEN_RETCODE		11
#define TIP_PULL_RETCODE	12

//=======================================================
// AS defines ( 101 - 150 )
//=======================================================

#define GET_OBJECT_REF			101

//=======================================================
// General defines
//=======================================================

#define MIN_POINT_ID	1
#define MAX_POINT_ID	150
#define FIRST_SERVER_ID	1		// 1-50
#define FIRST_DRIVER_ID	51		// 51-100
#define FIRST_AS_ID		101		// 101-150

#define DRIVER_POINTS	FIRST_AS_ID		 - FIRST_DRIVER_ID
#define SERVER_POINTS	FIRST_DRIVER_ID  - FIRST_SERVER_ID
#define AS_POINTS		MAX_POINT_ID + 1 - FIRST_AS_ID

#define WAIT_OPERATION		0x80000000
#define DEBUG_OPERATION		0x40000000
#define KILL_OPERATION		0x20000000
#define CUST_OPERATION		0x10000000
#define WAIT_INSTANT		0x08000000
#define DEBUG_INSTANT		0x04000000
#define KILL_INSTANT		0x02000000

#define VALID_OPERATIONS	WAIT_OPERATION | DEBUG_OPERATION | KILL_OPERATION | CUST_OPERATION\
	| WAIT_INSTANT | DEBUG_INSTANT | KILL_INSTANT

#define ALL_CLEAR			0x8000
#define DRIVER_CLEAR		0x4000
#define SERVER_CLEAR		0x2000

#define POINTS_CLEAR		0x00FF

#define VALID_CLEARS		ALL_CLEAR | DRIVER_CLEAR | SERVER_CLEAR

#define RETURN_SUCCESS		0
#define NULL_HDBC			-1
#define KRYPTON_ERROR		-2
#define PARAM_ERROR			-3
#define INVALID_CONNECTION	-4
#define DEAD_CONNECTION		-5
#define RANGE_ERROR			-6
#define INVALID_OPERATION	-7
#define NOT_IMPLEMENTED		-8
#define EXCEPTION_OCCURRED	-9

#define RET_CODE			522


//=============================================================
// macros:
//   for the driver: TEST_POINT( pConnection, TEST_POINT )
//   for the server: TEST_POINT( TEST_POINT, PARAM )
//   for the AS:     TEST_POINT( TEST_POINT, PARAM )
//
// Note:
//   When _TEST_POINTS is undefined, empty macros are generated
//==============================================================

#ifdef _AFXDLL

typedef void (WINAPI* ExternProcessTrace)( char* );
typedef void (WINAPI* DebugOut)(unsigned int,char*, char*, int);
typedef void (WINAPI* EnterCSection)(void);
typedef void (WINAPI* ExitCSection)(void);

extern long* pTrace_flags;
extern ExternProcessTrace pExternProcessTrace;
extern DebugOut pDebugOut;
extern EnterCSection pEnterCSection;
extern ExitCSection pExitCSection;

// This macro is only for the driver

#ifdef _TEST_POINTS

#define TEST_POINT( pCONNECTION, TEST_POINT )\
	if( pCONNECTION->m_TestPointArray != NULL)\
	{\
		long Operation = pCONNECTION->m_TestPointArray[ TEST_POINT - FIRST_DRIVER_ID ];\
		if( Operation != 0 )\
		{\
			if( Operation & WAIT_OPERATION )\
			{\
				DWORD mSeconds = Operation & 0x0000FFFF;\
				Sleep (mSeconds);\
			}else if ( Operation & DEBUG_OPERATION )\
			{\
				DebugBreak();\
			}else if ( Operation & KILL_OPERATION )\
			{\
				TerminateProcess(GetCurrentProcess(), RET_CODE);\
			}else if ( Operation & CUST_OPERATION )\
			{\
			}\
		}\
	}

#else

#define TEST_POINT( pCONNECTION, TEST_POINT )

#endif
#else
#define TEST_POINT( TEST_POINT, PARAM )

#endif
//======================================================
//
//======================================================

#include <stdio.h>
#include <sys/timeb.h>
#include <time.h>
#include <eh.h>

#ifdef __cplusplus
extern "C" {
#endif
//LCOV_EXCL_START
char *ElapsedTimeString(struct _timeb StartTime);
void LogFile( char* txt1, char* txt2, char* txt3 );
void TraceOut(long TraceOption, char *text, ...);
void HexDump(  unsigned Len,char *Data, LPSTR TextString );
BOOL OpenTraceFile(void);
void CloseTraceFile(void);

void LogDelete( char* line, void** ppointer, void* pointer );
void LogNew( char* line, void** ppointer, void* pointer, long length );
//LCOV_EXCL_STOP
#ifdef __cplusplus
}
#endif

#define MAX_PC_NAME			100

#define TRACE_SETDIAGINFO	-32767
#define SEND_CDINFO			-32766
#define SET_TESTPOINT		-32765
#define CLEAR_TESTPOINT     -32764

struct TraceInfo {
	long diag_flag;
	char ClientPCName[MAX_PC_NAME];
	};
typedef struct TraceInfo TraceInfo;

//==========================================================
// Eternal declarations for global variables
//==========================================================
extern long diagnostic_flags;
extern char szTraceFile[];

extern char g_ElapsedTime[];

extern long trace_memory;

#define TRACE_STRING_MAX	100

//==========================================================
// Trace options
//		These must match with the TraceOptionString array
//		in CommonDiags.cpp
//==========================================================
#define TR_ON					0x00000001
#define TR_ODBC_API				0x00000002
#define TR_ODBC_API_EXIT		0x00000004
#define TR_DRVR_KRYPTON_API		0x00000008
#define TR_SRVR_KRYPTON_API		0x00000010
#define TR_SQL_API				0x00000020
#define TR_POST_ERROR			0x00000040
#define TR_CFG					0x00000080
#define TR_ASSOC_SRVR			0x00000100
#define TR_SQL_API_EXIT			0x00000200
#define TR_SRVR					0x00000400
	// ...and so on up to 32 trace options
#define TR_SPARE32				0x80000000



// Some useful macros to cut down on typing
// ========================================

// This macro allows an ODBC handle to be displayed
// as SQL_NULL_HANDLE or its actual hex value
#define HANDLE_TO_CHAR(dest,hptr) \
		if(!hptr) strcpy(dest,"<null-handle>"); \
		else sprintf(dest,"0x%08lX",hptr)


// This macro allows a pointer to be displayed as
// <null-pointer> or its actual hex value
#define PTR_TO_CHAR(dest,ptr) \
		if(!ptr) strcpy(dest,"<null-pointer>"); \
		else sprintf(dest,"0x%08lX",ptr)

// This macro will allow a pointer to a short int to be
// displayed as <null-pointer> or the actual numeric string
#define SHORT_PTR_TO_CHAR(dest,ptr) \
		if(!ptr) strcpy(dest,"<null-pointer>"); \
		else sprintf(dest,"%d",*ptr)

// This macro will allow a pointer to a long int to be
// displayed as <null-pointer> or the actual numeric string
#define LONG_PTR_TO_CHAR(dest,ptr) \
		if(!ptr) strcpy(dest,"<null-pointer>"); \
		else sprintf(dest,"%ld",*ptr)

// This macro will allow a pointer to a string to be
// displayed as <null-pointer>, <empty-string>, or the
// actual ascii string
#define STRING_PTR_TO_CHAR(dest,ptr) \
		if(!ptr) strcpy(dest,"<null-pointer>"); \
		else if(*ptr=='\0') strcpy(dest,"<empty-string>"); \
		else sprintf(dest,"'%s'",ptr)
		
// This macro will allow a pointer to a string to be
// displayed as <null-pointer>, <empty-string>, the
// actual ascii string, or if the 'len' parameter is
// invalid the pointer's address.
#define STRING_PTR_TO_CHAR_N(dest,ptr,len) \
		if(!ptr) strcpy(dest,"<null-pointer>"); \
		else if((*ptr=='\0')||(len==0)) strcpy(dest,"<empty-string>"); \
		else { \
			if(len>0) { \
				strncpy(dest,(char *)ptr,len); \
				dest[len]='\0'; \
				} \
			else if(len==SQL_NTS) sprintf(dest,"'%s'",ptr); \
			else sprintf(dest,"0x%08lX",ptr); \
			}

char* GetExceptionMessage( unsigned long );

#endif  // COMMONDIAGS_DEFINED

// end of CommonDiags.h
