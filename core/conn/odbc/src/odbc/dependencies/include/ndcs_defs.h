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
// Platform independent definitions for ndcs.

#ifndef NDCS_DEFS_H_
#define NDCS_DEFS_H_

#include <sqltypes.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif // !FALSE

#ifndef _BYTE_DEFINED
#define _BYTE_DEFINED
typedef unsigned char BYTE;
#endif // !_BYTE_DEFINED

#ifndef _WORD_DEFINED
#define _WORD_DEFINED
typedef unsigned short WORD;
#endif // !_WORD_DEFINED

#ifndef _UINT_DEFINED
#define _UINT_DEFINED
typedef unsigned int UINT;
#endif // !_UINT_DEFINED



typedef char CHAR;
typedef unsigned char UCHAR;


#ifndef _PVOID_DEFINED
#define _PVOID_DEFINED
typedef void* PVOID;
#endif //!_PVOID_DEFINED

#if	( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif

//
// The types of events that can be logged.
//
#define EVENTLOG_SUCCESS                0X0000
#define EVENTLOG_ERROR_TYPE             0x0001
#define EVENTLOG_WARNING_TYPE           0x0002
#define EVENTLOG_INFORMATION_TYPE       0x0004
#define EVENTLOG_AUDIT_SUCCESS          0x0008
#define EVENTLOG_AUDIT_FAILURE          0x0010

#define LANG_ENGLISH                     0x09
#define LANG_JAPANESE                    0x11
#define LANGIDFROMLCID(lcid)   ((WORD  )(lcid))
#define PRIMARYLANGID(lgid)    ((WORD  )(lgid) & 0x3ff)


#define CRITICAL_SECTION ULONG
#define LPCRITICAL_SECTION ULONG *

#define HANDLE int
#define HMODULE int

#define VOID	void

#define far				_far
#define FAR				_far
typedef void far *LPVOID;

#ifndef VERSION3

#ifndef _max
#define _max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef _min
#define _min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#endif

typedef DWORD *LPDWORD;

#define STATUS_DATATYPE_MISALIGNMENT     ((DWORD   )0x80000002L)    
#define STATUS_BREAKPOINT                ((DWORD   )0x80000003L)    
#define STATUS_SINGLE_STEP               ((DWORD   )0x80000004L)    
#define STATUS_ACCESS_VIOLATION          ((DWORD   )0xC0000005L)    

#define STATUS_NONCONTINUABLE_EXCEPTION  ((DWORD   )0xC0000025L)    

#define STATUS_ARRAY_BOUNDS_EXCEEDED     ((DWORD   )0xC000008CL)    

#define STATUS_FLOAT_DENORMAL_OPERAND    ((DWORD   )0xC000008DL)    
#define STATUS_FLOAT_DIVIDE_BY_ZERO      ((DWORD   )0xC000008EL)    
#define STATUS_FLOAT_INEXACT_RESULT      ((DWORD   )0xC000008FL)    
#define STATUS_FLOAT_INVALID_OPERATION   ((DWORD   )0xC0000090L)    
#define STATUS_FLOAT_OVERFLOW            ((DWORD   )0xC0000091L)    
#define STATUS_FLOAT_STACK_CHECK         ((DWORD   )0xC0000092L)    
#define STATUS_FLOAT_UNDERFLOW           ((DWORD   )0xC0000093L)    
#define STATUS_INTEGER_DIVIDE_BY_ZERO    ((DWORD   )0xC0000094L)    
#define STATUS_INTEGER_OVERFLOW          ((DWORD   )0xC0000095L)    
#define STATUS_PRIVILEGED_INSTRUCTION    ((DWORD   )0xC0000096L)    

#define EXCEPTION_ACCESS_VIOLATION          STATUS_ACCESS_VIOLATION
#define EXCEPTION_DATATYPE_MISALIGNMENT     STATUS_DATATYPE_MISALIGNMENT
#define EXCEPTION_BREAKPOINT                STATUS_BREAKPOINT
#define EXCEPTION_SINGLE_STEP               STATUS_SINGLE_STEP
#define EXCEPTION_ARRAY_BOUNDS_EXCEEDED     STATUS_ARRAY_BOUNDS_EXCEEDED
#define EXCEPTION_FLT_DENORMAL_OPERAND      STATUS_FLOAT_DENORMAL_OPERAND
#define EXCEPTION_FLT_DIVIDE_BY_ZERO        STATUS_FLOAT_DIVIDE_BY_ZERO
#define EXCEPTION_FLT_INEXACT_RESULT        STATUS_FLOAT_INEXACT_RESULT
#define EXCEPTION_FLT_INVALID_OPERATION     STATUS_FLOAT_INVALID_OPERATION
#define EXCEPTION_FLT_OVERFLOW              STATUS_FLOAT_OVERFLOW
#define EXCEPTION_FLT_STACK_CHECK           STATUS_FLOAT_STACK_CHECK
#define EXCEPTION_FLT_UNDERFLOW             STATUS_FLOAT_UNDERFLOW
#define EXCEPTION_INT_DIVIDE_BY_ZERO        STATUS_INTEGER_DIVIDE_BY_ZERO
#define EXCEPTION_INT_OVERFLOW              STATUS_INTEGER_OVERFLOW
#define EXCEPTION_PRIV_INSTRUCTION          STATUS_PRIVILEGED_INSTRUCTION
#define EXCEPTION_NONCONTINUABLE_EXCEPTION  STATUS_NONCONTINUABLE_EXCEPTION

#define MAX_COMPUTERNAME_LENGTH 15

#define ERROR_SUCCESS                    0L  

typedef unsigned short                  WCHAR;
typedef WCHAR FAR *                     LPWSTR;

#ifndef _LPSTR_DEFINED
#define _LPSTR_DEFINED
typedef CHAR * LPSTR;
#endif //!_LPSTR_DEFINED

#define LANG_NEUTRAL                     0x00
#define SUBLANG_DEFAULT                  0x00
#define MAKELANGID(p, s)       ((((WORD  )(s)) << 10) | (WORD  )(p))

#define LPTSTR				char *

#define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100
#define FORMAT_MESSAGE_FROM_SYSTEM     0x00001000

#ifdef NSK_ODBC_SRVR
#define EnterCriticalSection2(y) *(SB_Thread::Mutex*)(y)->lock();
#define LeaveCriticalSection2(y) *(SB_Thread::Mutex*)(y)->unlock();
#endif

#define InitializeCriticalSection(LPCRITICAL_SECTION) ((void)0)
#define DeleteCriticalSection(LPCRITICAL_SECTION) ((void)0)
#define EnterCriticalSection(LPCRITICAL_SECTION) ((void)0)
#define LeaveCriticalSection(LPCRITICAL_SECTION) ((void)0)


#define Sleep(M) sleep(M/1000)

#define GetACP()	1033

#define _timeb			timeb
#define _ftime			ftime

#define _stricmp		strcasecmp
#define stricmp			strcasecmp
#define strnicmp		strncasecmp

#define WINAPI

#define _I64_MAX	LLONG_MAX

#define FILE_EMS			1800
#define PRODUCT_NO_SIZE 8
#endif

