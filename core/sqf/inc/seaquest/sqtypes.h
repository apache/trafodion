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
#ifndef _SQTYPES_H
#define _SQTYPES_H

#include <stdlib.h>

#include "fs/feerrors.h" 
#define DLLEXPORT
#define SECLIBAPI
#define __cdecl
#define WINAPI 
#ifndef FAR
#define FAR
#endif 

#define _MAX_PATH       260


#define FALSE         0

#define NO_ERROR      0L

#ifdef __GNUC__
#if __GNUC__ * 100 + __GNUC_MINOR__ >= 404
// int64_t/uint64_t is an extension in gcc 4.4
#if __WORDSIZE == 64
#else
typedef long long int int64_t;
typedef unsigned long long int uint64_t;
#endif
#endif
#endif

typedef unsigned long  ULONG;
typedef ULONG          u_long;
typedef long           LONG;
typedef LONG           *PLONG;

typedef unsigned int   UINT;
typedef int            INT;
typedef INT            *PINT;

typedef unsigned short WORD;
typedef WORD           *PWORD;

typedef unsigned short USHORT;
typedef short          SHORT;
typedef SHORT          *PSHORT;

typedef short         int_16;

// dg64 - vvv - Add xint_32
typedef int           _xint_32;
typedef _xint_32      xint_32;
// dg64 - ^^^
#ifdef NA_64BIT
// dg64 - a little screwy - but 32 bit container seems to hold a lot of
//        pointers so just make it 64-bits
typedef int          _int_32;   /* DG had it as "long" */
#else
typedef int           _int_32;
#endif
typedef _int_32       int_32;

#ifdef NA_64BIT
typedef unsigned int _unsigned_32; /* DG had it as "long" */
#else
typedef unsigned long _unsigned_32;
#endif
typedef _unsigned_32  unsigned_32;

// dg64 - vvv - Add xunsigned_32
typedef unsigned int  _xunsigned_32;
typedef _xunsigned_32  xunsigned_32;
// dg64 - ^^^

#ifdef NA_64BIT
// dg64 - the right type
typedef  long     _int_64; /* DG had it as "long" */
#else
typedef long long     _int_64;
#endif

#ifndef _int64
#ifdef NA_64BIT
  #define  _int64 long
  #define __int64 long
#else
  #define  _int64 long long int
  #define __int64 long long int
#endif
#endif

typedef  int              BOOL  ;
typedef  unsigned int     DWORD ;
typedef  DWORD        DWORD_PTR ;

typedef  void           VOID   ;
typedef  void         * PVOID  ;
typedef  void         * LPVOID ;
typedef  void         * HANDLE ;
typedef  char           TCHAR  ;
typedef  wchar_t        WCHAR  ;

#ifndef TRUE
#define TRUE 1
#endif

#define ERROR_INVALID_HANDLE -2

#define __declspec(x)
#define  _declspec(x)
#define _stricmp(a,b) strcasecmp(a,b)

typedef struct _listEntry {
  struct _listEntry * fwdLink  ;
  struct _listEntry * bkwdLink ;
} LIST_ENTRY ;
typedef LIST_ENTRY *PLIST_ENTRY ;

typedef struct _CRITICAL_SECTION_DEBUG {
    WORD               Type                  ;
    WORD               CreatorBackTraceIndex ;
    struct _CRITICAL_SECTION *CriticalSection;
    LIST_ENTRY         ProcessLocksList      ;
    DWORD              EntryCnt              ;
    DWORD              ContentionCnt         ;
    DWORD              Filler[2]             ;
} CRITICAL_SECTION_DEBUG ;
typedef  CRITICAL_SECTION_DEBUG *pCRITICAL_SECTION_DEBUG ;

typedef struct _CRITICAL_SECTION {
    pCRITICAL_SECTION_DEBUG DbgInfo ;
    LONG    LockCnt       ;
    LONG    RecursionCnt  ;
    HANDLE  OwningThread  ; // From thread's ClientId->UniqueThread
    HANDLE  LockSemaphore ;
    ULONG * SpinCnt       ; // Ensure size on 64-bit systems when packed
} CRITICAL_SECTION ;

typedef struct _FILETIME {
    DWORD  dwLowDateTime  ;
    DWORD  dwHighDateTime ;
} FILETIME ;

typedef struct _SYSTEMTM {
    WORD  wYear         ;
    WORD  wMonth        ;
    WORD  wDayOfWeek    ;
    WORD  wDay          ;
    WORD  wHour         ;
    WORD  wMinute       ;
    WORD  wSecond       ;
    WORD  wMilliseconds ;
} SYSTEMTIME ;

typedef struct _TIME_ZONE_INFO {
    LONG       Bias             ;
    WCHAR      StandardName[32] ;
    SYSTEMTIME StandardDate     ;
    LONG       StandardBias     ;
    WCHAR      DaylightName[32] ;
    SYSTEMTIME DaylightDate     ;
    LONG       DaylightBias     ;
} TIME_ZONE_INFO ;

typedef  DWORD ( * PTHRD_START_ROUTINE )( void * pThrdParameter ) ;
typedef  PTHRD_START_ROUTINE  LPTHREAD_START_ROUTINE ;

void Sleep( DWORD milliSecs ) ;
DWORD SleepEx( DWORD milliSecs, BOOL alertableFlag ) ;

void InitializeCriticalSection( CRITICAL_SECTION * pCriticalSection ) ;
void EnterCriticalSection(      CRITICAL_SECTION * pCriticalSection ) ;
BOOL TryEnterCriticalSection(   CRITICAL_SECTION * pCriticalSection ) ;
void LeaveCriticalSection(      CRITICAL_SECTION * pCriticalSection ) ;
void DeleteCriticalSection(     CRITICAL_SECTION * pCriticalSection ) ;

BOOL GetSystemTimeAdjustment( DWORD * pTimeAdjustment, DWORD * pTimeIncrement,
                              BOOL  * pTimeAdjustmentDisabled ) ;

void GetSystemTimeAsFileTime( FILETIME * pSystemTimeAsFileTime ) ;

DWORD GetTimeZoneInformation( TIME_ZONE_INFO * pTimeZoneInfo ) ;

#define  MB_OK              0x0L
#define  MB_ICONERROR       0x010L
#define  MB_ICONWARNING     0x030L
#define  MB_ICONINFORMATION 0x040L
#define  MB_SETFOREGROUND   0x010000L
#define  MB_TOPMOST         0x040000L

#define CTRL_C_EVENT        0
#define CTRL_BREAK_EVENT    1
#define CTRL_CLOSE_EVENT    2
// Values of 3 and 4 are reserved.
#define CTRL_LOGOFF_EVENT   5
#define CTRL_SHUTDOWN_EVENT 6


unsigned int GetCurrThreadId( void ) ;

HANDLE CreateNewThread( LPTHREAD_START_ROUTINE pStartAddress,
                        void * pParameter ) ;

struct LargeInt {
    long long QuadPart ;
} ;

BOOL QueryPerfCounter( LargeInt * pPerfCount ) ;

#ifdef NA_64BIT
typedef long     int_64; /* DG had it as "long" */
#else
typedef long long     int_64;
#endif

typedef int_64        fixed_0 ;

typedef double        DblInt;
typedef unsigned int  DWORD;
typedef DWORD         *PDWORD;
typedef DWORD         *LPDWORD;

typedef unsigned char unsigned_char;
typedef unsigned char UCHAR;
typedef UCHAR         BYTE;
typedef BYTE          *PBYTE;

typedef char          CHAR;
typedef CHAR          *PCHAR;
typedef CHAR          *LPSTR;

typedef const char    *LPCSTR,*PCSTR;

typedef wchar_t       WCHAR;
typedef WCHAR         *PWCHAR;
typedef WCHAR         *LPWSTR;
typedef const WCHAR   *LPCWSTR;

class ex_send_bottom_tcb;
class ExDp2OperTcb;
class ExDp2InsertPrivateState;
class ExDp2VSBBInsertTcb;
class ExDp2UniqueOperPrivateState;
class ex_mj_private_state;
class ex_partn_access_private_state;
class ex_union_private_state;
class CatRWObject;
class CatPartitionList;
class CatPrivSettings;
class CatCkColUsageList;
class CatPartitioningKeyList;

/********* Some Windows structures **/

#if 0
#define DllImport
#define _declspec(a) 
#define __declspec(a) 
#define INVALID_HANDLE_VALUE (HANDLE) -1

#define EqualMemory(Destination,Source,Length) (!memcmp((Destination),(Source),(Length)))
#define MoveMemory(Destination,Source,Length) memmove((Destination),(Source),(Length))
#define CopyMemory(Destination,Source,Length) memcpy((Destination),(Source),(Length))
#define FillMemory(Destination,Length,Fill) memset((Destination),(Fill),(Length))
#define ZeroMemory(Destination,Length) memset((Destination),0,(Length))
#define TRUE          -1
#define MAX_PATH       _MAX_PATH

typedef long long         __int64;
typedef long long         _int64;
typedef unsigned long  DWORDLONG;
typedef __int64       LONGLONG;

typedef int_64        fixed_0;
#define fixed_0       int_64   

typedef unsigned long  DWORDLONG;

typedef void          *LPVOID;
typedef void          VOID;
typedef VOID          *PVOID;
typedef PVOID         PSID;

typedef int           BOOL;
typedef BOOL          *PBOOL;
typedef BOOL          *LPBOOL;


typedef PVOID         HANDLE;
typedef HANDLE        HKEY;
typedef HANDLE        *PHANDLE;
typedef HANDLE        *LPHANDLE;

typedef struct _LIST_ENTRY {
  struct _LIST_ENTRY * Flink;
  struct _LIST_ENTRY * Blink;
} LIST_ENTRY, *PLIST_ENTRY;

typedef union _LARGE_INTEGER {
    struct {
        DWORD LowPart;
        LONG HighPart;
    };
    struct {
        DWORD LowPart;
        LONG HighPart;
    } u;
    LONGLONG QuadPart;
} LARGE_INTEGER;

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

typedef struct _OVERLAPPED {
    DWORD   Internal;
    DWORD   InternalHigh;
    DWORD   Offset;
    DWORD   OffsetHigh;
    HANDLE  hEvent;
} OVERLAPPED, *LPOVERLAPPED;

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

typedef struct _TIME_ZONE_INFORMATION {
    LONG Bias;
    WCHAR StandardName[ 32 ];
    SYSTEMTIME StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[ 32 ];
    SYSTEMTIME DaylightDate;
    LONG DaylightBias;
} TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;

typedef struct _WSABUF {
    u_long      len;     /* the length of the buffer */
    char FAR *  buf;     /* the pointer to the buffer */
} WSABUF, FAR * LPWSABUF;

#endif

//
//  File System time stamps are represented with the following structure:
//


#define min(a,b)            (((a) < (b)) ? (a) : (b))


#define TIME_ZONE_ID_UNKNOWN  0
#define TIME_ZONE_ID_STANDARD 1
#define TIME_ZONE_ID_DAYLIGHT 2
#define TIME_ZONE_ID_INVALID  ((DWORD)0xFFFFFFFF)
#define INFINITE              0xFFFFFFFF  // Infinite timeout

/********* Win32 Function definitions */

void DebugBreak();
#define MessageBox MessageBoxA
int MessageBoxA( void * ,
                 const char * text ,
                 const char * process ,
                 int         type )   ;

HANDLE GetCurrentProcess(void);


/** Some Windows Status codes ****/

#define STATUS_WAIT_0                    ((DWORD   )0x00000000L)    
#define STATUS_ABANDONED_WAIT_0          ((DWORD   )0x00000080L)    
#define STATUS_USER_APC                  ((DWORD   )0x000000C0L)    
#define STATUS_TIMEOUT                   ((DWORD   )0x00000102L)    
#define STATUS_PENDING                   ((DWORD   )0x00000103L)    
#define STATUS_SEGMENT_NOTIFICATION      ((DWORD   )0x40000005L)    
#define STATUS_GUARD_PAGE_VIOLATION      ((DWORD   )0x80000001L)    
#define STATUS_DATATYPE_MISALIGNMENT     ((DWORD   )0x80000002L)    
#define STATUS_BREAKPOINT                ((DWORD   )0x80000003L)    
#define STATUS_SINGLE_STEP               ((DWORD   )0x80000004L)    
#define STATUS_ACCESS_VIOLATION          ((DWORD   )0xC0000005L)    
#define STATUS_IN_PAGE_ERROR             ((DWORD   )0xC0000006L)    
#define STATUS_INVALID_HANDLE            ((DWORD   )0xC0000008L)    
#define STATUS_NO_MEMORY                 ((DWORD   )0xC0000017L)    
#define STATUS_ILLEGAL_INSTRUCTION       ((DWORD   )0xC000001DL)    
#define STATUS_NONCONTINUABLE_EXCEPTION  ((DWORD   )0xC0000025L)    
#define STATUS_INVALID_DISPOSITION       ((DWORD   )0xC0000026L)    
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
#define STATUS_STACK_OVERFLOW            ((DWORD   )0xC00000FDL)    
#define STATUS_CONTROL_C_EXIT            ((DWORD   )0xC000013AL)    

#define stricmp strcasecmp
#endif
