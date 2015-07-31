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

#ifndef WINDOWS_H_
#define WINDOWS_H_
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <time.h>
#include "cextdecs.h"
#include "zsysc.h"

#ifndef NULL
#define NULL 0
#endif

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif

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

typedef int INT;

typedef char BOOL;

#ifndef _LONG_DEFINED
#define _LONG_DEFINED
typedef long LONG;

#endif // !_LONG_DEFINED
#ifndef _DWORD_DEFINED
#define _DWORD_DEFINED
typedef unsigned long DWORD;

#endif // !_DWORD_DEFINED
#ifndef _LRESULT_DEFINED
#define _LRESULT_DEFINED
typedef LONG LRESULT;

#endif // !_LRESULT_DEFINED

typedef char CHAR;


typedef unsigned char UCHAR;

typedef short SHORT;

typedef unsigned short USHORT;

#ifndef _ULONG_DEFINED
#define _ULONG_DEFINED
typedef DWORD ULONG;
#endif //!_ULONG_DEFINED

typedef double DOUBLE;

typedef long long __int64;

typedef float FLOAT;
typedef BOOL BOOLEAN;
#ifndef _PVOID_DEFINED
#define _PVOID_DEFINED
typedef void* PVOID;
#endif //!_PVOID_DEFINED
typedef PVOID HLOCAL;

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef LONG HRESULT;

#endif // !_HRESULT_DEFINED


// from WCTYPE.H
/* Define __cdecl for non-Microsoft compilers */

#if	( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif

#ifndef SYSTEMTIME
typedef struct _SYSTEMTIME {
	WORD wYear; 
	WORD wMonth; 
	WORD wDayOfWeek; 
	WORD wDay; 
	WORD wHour; 
	WORD wMinute; 
	WORD wSecond; 
	WORD wMilliseconds; 
} SYSTEMTIME;

#endif //SYSTEMTIME

#define CONST const

//from WINNT.H
//
// The types of events that can be logged.
//
#define EVENTLOG_SUCCESS                0X0000
#define EVENTLOG_ERROR_TYPE             0x0001
#define EVENTLOG_WARNING_TYPE           0x0002
#define EVENTLOG_INFORMATION_TYPE       0x0004
#define EVENTLOG_AUDIT_SUCCESS          0x0008
#define EVENTLOG_AUDIT_FAILURE          0x0010

// Registry Specific Access Rights.
//
#define SYNCHRONIZE                      (0x00100000L)
#define READ_CONTROL                     (0x00020000L)
#define STANDARD_RIGHTS_READ             (READ_CONTROL)

#define KEY_QUERY_VALUE         (0x0001)
#define KEY_SET_VALUE           (0x0002)
#define KEY_CREATE_SUB_KEY      (0x0004)
#define KEY_ENUMERATE_SUB_KEYS  (0x0008)
#define KEY_NOTIFY              (0x0010)
#define KEY_CREATE_LINK         (0x0020)

#define KEY_READ                ((STANDARD_RIGHTS_READ       |\
                                  KEY_QUERY_VALUE            |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY)                 \
                                  &                           \
                                 (~SYNCHRONIZE))

#define LANG_ENGLISH                     0x09
#define LANG_JAPANESE                    0x11
#define LANGIDFROMLCID(lcid)   ((WORD  )(lcid))
#define PRIMARYLANGID(lgid)    ((WORD  )(lgid) & 0x3ff)
#define SORT_DEFAULT                     0x0 
#define SUBLANG_NEUTRAL                  0x00

#define INFINITE				0xFFFFFFFF
#define STATUS_WAIT_0			((DWORD   )0x00000000L)  
#define WAIT_OBJECT_0			((STATUS_WAIT_0 ) + 0 )
/****************************************************/
// My RSSH added stuff, I'll check it later
// from MAPIWIN.H
// Critical section code
#ifndef CRITICAL_SECTION
#define CRITICAL_SECTION ULONG
#endif

#ifndef LPCRITICAL_SECTION
#define LPCRITICAL_SECTION ULONG *
#endif

#ifndef HINSTANCE
#define HINSTANCE int
#endif

#ifndef HANDLE
#define HANDLE int
#endif

#ifndef HMODULE
#define HMODULE int
#endif

#ifndef BYTE
#define BYTE char
#endif

#ifndef HWND
#define HWND int
#endif

#ifndef UINT
#define UINT	unsigned int
#endif

#ifndef WORD
#define WORD	int
#endif

#ifndef DWORD
#define DWORD	long
#endif

#ifndef LPCVOID
#define LPCVOID	void *
#endif

#ifndef PVOID
#define PVOID char *
#endif

#ifndef FARPROC
#define FARPROC long *
#endif

#ifndef TCHAR
#define TCHAR char
#endif

#ifndef VOID
#define VOID	void
#endif

// types from Windef.h
typedef DWORD *PDWORD;
typedef BYTE *PBYTE;
typedef BYTE *LPBYTE;
#define far				_far
#define FAR				_far
typedef void far            *LPVOID;

#ifndef VERSION3
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#endif

// types from WinTypes.h
typedef PVOID PSID;
typedef DWORD *LPDWORD;

typedef struct  _SECURITY_ATTRIBUTES
    {
    DWORD nLength;
    /* [size_is] */ LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
    }	SECURITY_ATTRIBUTES;

//typedef struct _SECURITY_ATTRIBUTES __RPC_FAR *PSECURITY_ATTRIBUTES;

typedef struct _SECURITY_ATTRIBUTES *LPSECURITY_ATTRIBUTES;

typedef DWORD ( *PTHREAD_START_ROUTINE)(
    LPVOID lpThreadParameter
    );
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;


// from WinNt.h
typedef DWORD ACCESS_MASK;
#ifndef WIN32_NO_STATUS 
/*lint -save -e767 */  
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
/*lint -restore */  
#endif //WIN32_NO_STATUS

// from WINBASE.H

#define WAIT_FAILED (DWORD)0xFFFFFFFF
#define WAIT_OBJECT_0       ((STATUS_WAIT_0 ) + 0 )

#define WAIT_ABANDONED         ((STATUS_ABANDONED_WAIT_0 ) + 0 )
#define WAIT_ABANDONED_0       ((STATUS_ABANDONED_WAIT_0 ) + 0 )

#define WAIT_TIMEOUT                        STATUS_TIMEOUT
#define WAIT_IO_COMPLETION                  STATUS_USER_APC
#define STILL_ACTIVE                        STATUS_PENDING
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
#define EXCEPTION_IN_PAGE_ERROR             STATUS_IN_PAGE_ERROR
#define EXCEPTION_ILLEGAL_INSTRUCTION       STATUS_ILLEGAL_INSTRUCTION
#define EXCEPTION_NONCONTINUABLE_EXCEPTION  STATUS_NONCONTINUABLE_EXCEPTION
#define EXCEPTION_STACK_OVERFLOW            STATUS_STACK_OVERFLOW
#define EXCEPTION_INVALID_DISPOSITION       STATUS_INVALID_DISPOSITION
#define EXCEPTION_GUARD_PAGE                STATUS_GUARD_PAGE_VIOLATION
#define EXCEPTION_INVALID_HANDLE            STATUS_INVALID_HANDLE

#define MAX_COMPUTERNAME_LENGTH 15

//
// dwCreationFlag values
//

#define DEBUG_PROCESS               0x00000001
#define DEBUG_ONLY_THIS_PROCESS     0x00000002

#define CREATE_SUSPENDED            0x00000004

#define DETACHED_PROCESS            0x00000008

#define CREATE_NEW_CONSOLE          0x00000010

#define NORMAL_PRIORITY_CLASS       0x00000020
#define IDLE_PRIORITY_CLASS         0x00000040
#define HIGH_PRIORITY_CLASS         0x00000080
#define REALTIME_PRIORITY_CLASS     0x00000100

#define CREATE_NEW_PROCESS_GROUP    0x00000200
#define CREATE_UNICODE_ENVIRONMENT  0x00000400

#define CREATE_SEPARATE_WOW_VDM     0x00000800
#define CREATE_SHARED_WOW_VDM       0x00001000
#define CREATE_FORCEDOS             0x00002000

#define CREATE_DEFAULT_ERROR_MODE   0x04000000
#define CREATE_NO_WINDOW            0x08000000

#define PROFILE_USER                0x10000000
#define PROFILE_KERNEL              0x20000000
#define PROFILE_SERVER              0x40000000

#define THREAD_PRIORITY_LOWEST          THREAD_BASE_PRIORITY_MIN
#define THREAD_PRIORITY_BELOW_NORMAL    (THREAD_PRIORITY_LOWEST+1)
#define THREAD_PRIORITY_NORMAL          0
#define THREAD_PRIORITY_HIGHEST         THREAD_BASE_PRIORITY_MAX
#define THREAD_PRIORITY_ABOVE_NORMAL    (THREAD_PRIORITY_HIGHEST-1)
#define THREAD_PRIORITY_ERROR_RETURN    (MAXLONG)

#define THREAD_PRIORITY_TIME_CRITICAL   THREAD_BASE_PRIORITY_LOWRT
#define THREAD_PRIORITY_IDLE            THREAD_BASE_PRIORITY_IDLE

void 
OutputDebugString(
    char *lpOutputString
    );

BOOL
GetExitCodeThread(
    HANDLE hThread,
    LPDWORD lpExitCode
    );

HANDLE
CreateThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    DWORD dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    );


DWORD
SuspendThread(
    HANDLE hThread
    );

DWORD
ResumeThread(
    HANDLE hThread
    );

DWORD
WaitForSingleObject(
    HANDLE hHandle,
    DWORD dwMilliseconds
    );

/*
typedef struct _SID_IDENTIFIER_AUTHORITY {
    BYTE  Value[6];
} SID_IDENTIFIER_AUTHORITY, *PSID_IDENTIFIER_AUTHORITY;

typedef struct _SID {
   BYTE  Revision;
   BYTE  SubAuthorityCount;
   SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
#ifdef MIDL_PASS
   [size_is(SubAuthorityCount)] DWORD SubAuthority[*];
#else // MIDL_PASS
   DWORD SubAuthority[ANYSIZE_ARRAY];
#endif // MIDL_PASS
} SID, *PISID;

*/

/***************************************************/
// from WINERROR.H
//
//  The operation completed successfully.
//
#define ERROR_SUCCESS                    0L  
#define ERROR_NO_TOKEN                   1008L
#define ERROR_LOGON_FAILURE              1326L

// from MAPINLS.H
//
typedef unsigned short                  WCHAR;
typedef WCHAR FAR *                     LPWSTR;
//typedef const WCHAR FAR *               LPCWSTR;
//typedef CHAR FAR *                      LPSTR;
#ifndef _LPSTR_DEFINED
#define _LPSTR_DEFINED
typedef CHAR * LPSTR;
#endif //!_LPSTR_DEFINED
//typedef const CHAR FAR *                LPCSTR;
//typedef TCHAR FAR *                     LPTSTR;
//typedef const TCHAR FAR *               LPCTSTR;
typedef const CHAR * LPCTSTR;
typedef const CHAR * LPCWSTR;
typedef DWORD                           LCID;
//typedef const void FAR *                LPCVOID;


// from y:\inc\cextdecs\cextdecs.h
//
//static short const          OMITSHORT         = -291;
//static int const            OMITINT           = -19070975;
#ifndef OMITREF
#define OMITREF     
#endif

#ifndef OMITSHORT
#define OMITSHORT     
#endif

#ifndef OMITINT
#define OMITINT     
#endif

// international character sets from WINRESRC.H
#define LANG_NEUTRAL                     0x00
#define SUBLANG_DEFAULT                  0x00
#define LPTSTR				char *
#define LPSTR				char *
#define LPCSTR				const char *
#define wsprintf	sprintf
#define TEXT

// Macros to process language values (from WINNT.H)
#define MAKELANGID(p, s)       ((((WORD  )(s)) << 10) | (WORD  )(p))
#define PRIMARYLANGID(lgid)    ((WORD  )(lgid) & 0x3ff)
#define SUBLANGID(lgid)        ((WORD  )(lgid) >> 10)
#define MAKELCID(lgid, srtid)  ((DWORD)((((DWORD)((WORD  )(srtid))) << 16) | ((DWORD)((WORD  )(lgid))))) 
typedef HANDLE * PHANDLE;

#ifndef NUMBERFMT
#define NUMBERFMT
typedef struct _numberfmtA {
    UINT    NumDigits;                 // number of decimal digits
    UINT    LeadingZero;               // if leading zero in decimal fields
    UINT    Grouping;                  // group size left of decimal
    LPSTR   lpDecimalSep;              // ptr to decimal separator string
    LPSTR   lpThousandSep;             // ptr to thousand separator string
    UINT    NegativeOrder;             // negative number ordering
} NUMBERFMT;
#endif

/*
#define TOKEN_QUERY             (0x0008)

typedef struct _TOKEN_USER {
    SID_AND_ATTRIBUTES User;
} TOKEN_USER, *PTOKEN_USER;

typedef struct _TOKEN_OWNER {
    PSID Owner;
} TOKEN_OWNER, *PTOKEN_OWNER;

typedef enum _SID_NAME_USE {
    SidTypeUser = 1,
    SidTypeGroup,
    SidTypeDomain,
    SidTypeAlias,
    SidTypeWellKnownGroup,
    SidTypeDeletedAccount,
    SidTypeInvalid,
    SidTypeUnknown
} SID_NAME_USE, *PSID_NAME_USE;

typedef enum _TOKEN_INFORMATION_CLASS {
    TokenUser = 1,
    TokenGroups,
    TokenPrivileges,
    TokenOwner,
    TokenPrimaryGroup,
    TokenDefaultDacl,
    TokenSource,
    TokenType,
    TokenImpersonationLevel,
    TokenStatistics
} TOKEN_INFORMATION_CLASS, *PTOKEN_INFORMATION_CLASS;

typedef enum _TOKEN_TYPE {
    TokenPrimary = 1,
    TokenImpersonation
    } TOKEN_TYPE;
typedef TOKEN_TYPE *PTOKEN_TYPE;
*/

// from error.h
#define ERROR_INSUFFICIENT_BUFFER   122
//#ifndef NO_ERROR
//#define NO_ERROR            0
//#endif
#define ERROR_FILE_NOT_FOUND        2


// Generic Format Message function from WINBASE.H
#define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100
#define FORMAT_MESSAGE_IGNORE_INSERTS  0x00000200
#define FORMAT_MESSAGE_FROM_STRING     0x00000400
#define FORMAT_MESSAGE_FROM_HMODULE    0x00000800
#define FORMAT_MESSAGE_FROM_SYSTEM     0x00001000
#define FORMAT_MESSAGE_ARGUMENT_ARRAY  0x00002000
#define FORMAT_MESSAGE_MAX_WIDTH_MASK  0x000000FF
#define LOGON32_LOGON_INTERACTIVE   2
#define LOGON32_PROVIDER_DEFAULT    0

FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
HMODULE LoadLibrary(LPCSTR lpLibFileName);
BOOL FreeLibrary( HMODULE hLibModule); 

//BOOL IsValidSid (PSID pSid);
//PSID_IDENTIFIER_AUTHORITY GetSidIdentifierAuthority (PSID pSid);
//PDWORD GetSidSubAuthority (PSID pSid,DWORD nSubAuthority);
//PUCHAR GetSidSubAuthorityCount (PSID pSid);
//BOOL LookupAccountName (LPCSTR lpSystemName, LPCSTR lpAccountName, PSID Sid, LPDWORD cbSid,
//    LPSTR ReferencedDomainName, LPDWORD cbReferencedDomainName, PSID_NAME_USE peUse);
//BOOL LookupAccountSid (LPCSTR lpSystemName, PSID Sid, LPSTR Name, LPDWORD cbName,
//    LPSTR ReferencedDomainName, LPDWORD cbReferencedDomainName, PSID_NAME_USE peUse);
//BOOL GetTokenInformation (HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass,
//    LPVOID TokenInformation, DWORD TokenInformationLength, PDWORD ReturnLength);

DWORD FormatMessage(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId,
 LPSTR lpBuffer, DWORD nSize, va_list *Arguments);

HANDLE GetCurrentThread(VOID);
DWORD GetCurrentThreadId(VOID);
HANDLE GetCurrentProcess(VOID);
DWORD GetCurrentProcessId(VOID);
BOOL GetComputerName (LPSTR lpBuffer, LPDWORD nSize);
BOOL CloseHandle(HANDLE hObject);
int GetWindowText(HWND hWnd, LPTSTR lpString, int nMaxCount ); 

//void InitializeCriticalSection (LPCRITICAL_SECTION);
//void DeleteCriticalSection (LPCRITICAL_SECTION);
//VOID EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
//VOID LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
#define InitializeCriticalSection(LPCRITICAL_SECTION) ((void)0)
#define DeleteCriticalSection(LPCRITICAL_SECTION) ((void)0)
#define EnterCriticalSection(LPCRITICAL_SECTION) ((void)0)
#define LeaveCriticalSection(LPCRITICAL_SECTION) ((void)0)

BOOL GetComputerName (LPSTR lpBuffer, LPDWORD nSize);
//VOID Sleep(DWORD dwMilliseconds);
#ifdef NSK_DRIVER
#define Sleep(M) DELAY(M/500)
#else
#define Sleep(M) sleep(M/1000)
#endif

BOOL LogonUser (LPSTR lpszUsername, LPSTR lpszDomain, LPSTR lpszPassword,
    DWORD dwLogonType, DWORD dwLogonProvider, PHANDLE phToken);


// missing function prototypes???
#define DebugBreak() DEBUG()
#define strupr(myparam)\
	myparam
int GetLastError();
SetLastError(int);
long GetProcAddress();
//UINT GetACP(void);
#define GetACP()	1033

// Stuff for MessageBox from WINUSER.H
int MessageBox(HWND hWnd , LPCSTR lpText, LPCSTR lpCaption, UINT uType);
/*
 * MessageBox() Flags
 */
#define MB_OK                       0x00000000L
#define MB_OKCANCEL                 0x00000001L
#define MB_ABORTRETRYIGNORE         0x00000002L
#define MB_YESNOCANCEL              0x00000003L
#define MB_YESNO                    0x00000004L
#define MB_RETRYCANCEL              0x00000005L
#define MB_ICONHAND                 0x00000010L
#define MB_ICONQUESTION             0x00000020L
#define MB_ICONEXCLAMATION          0x00000030L
#define MB_ICONASTERISK             0x00000040L


LocalFree(LPTSTR);


//Redefine timeb.h
#define _timeb			timeb
#define _ftime			ftime

//strcasecmp is defined in tdmXdev\d45\include\strings.h
#define _stricmp		strcasecmp
#define stricmp			strcasecmp
#define strnicmp		strncasecmp
#define _strnicmp		strncasecmp
#define _fcvt			fcvt
#define _ui64toa		_i64toa

#define WINAPI

#define Sleep(x)		sleep((x))

#define itoa			_itoa
#define ltoa			_ltoa
#define strupr			_strupr

char * _itoa(int, char *, int);
char *	_ltoa(long, char *, int);
char * _strupr(char *);
char* trim(char *string);
char *_strdup( const char *strSource );
#define strdup _strdup

char * _strnset(char * str, int c, unsigned int count);
#define strnset _strnset


// change the NT limit to NSK limit
#define _I64_MAX	LLONG_MAX

//============== NSK Driver Conversion ============================

#define FULL_SYSTEM_LEN 25
#define FULL_VOL_LEN 25
#define FULL_SUBVOL_LEN 25
#define EXT_FILENAME_LEN ZSYS_VAL_LEN_FILENAME
#define	MAX_IMMU_MSG_LEN 120
#define PRODUCT_NO_SIZE 8
#define MAX_ERROR_TEXT_LEN 512

#define ODBC_DRIVER		"NonStop ODBC/MX"
#define DSNFILE			"TRAFDSN"
#define SYSTEM_PATH		"$SYSTEM.SYSTEM"

#define SYSTEM_DSNFILE	"$SYSTEM.SYSTEM.MXODSN"
#define USER_DSNFILE	DSNFILE

enum FILE_STATUS { FILE_CLOSED, FILE_OPENED, FILE_ERROR };

char* getUserFilePath(char* idsnFileName);
long long getFileLastModificationTime(char* idsnFileName);

class OdbcDsnLine {
public:
	OdbcDsnLine();
	OdbcDsnLine(char* buffer);
	~OdbcDsnLine();
	char buffer[81];
	OdbcDsnLine* next;
};

typedef struct FCache {
	int		error;
	char	dsnFileName[128];
	long long lastFileModificationTime;
	OdbcDsnLine* dsnCache;
} FCache;

class OdbcDsn {
public:
	OdbcDsn();
	~OdbcDsn();
	BOOL WritePrivateProfileString(
		  LPCTSTR lpAppName,  // section name
		  LPCTSTR lpKeyName,  // key name
		  LPCTSTR lpString,   // string to add
		  LPCTSTR lpFileName  // initialization file
	);
	DWORD GetPrivateProfileString(
		  LPCTSTR lpAppName,        // section name
		  LPCTSTR lpKeyName,        // key name
		  LPCTSTR lpDefault,        // default string
		  LPTSTR lpReturnedString,  // destination buffer
		  DWORD nSize,              // size of destination buffer
		  LPCTSTR lpFileName        // initialization file name
	);
	int		getODBCDsnError();
	char*	getODBCDsnFileName();
private:
	void	openFile(char* fileName);
	short	readFile();
	BOOL	isFileOpen();
	void	deleteDsnCache(int cache);
	void	insertIntoDsnCache(char* readbuffer);
	char*	getNextLineFromDsnCache();
private:
	char	readbuffer[256];
	OdbcDsnLine* currentLineInDsnCache;

	int		currentCache;
	FCache	fCaches[2];
};

__int64 _atoi64( const char *string );
char *_i64toa( __int64 value, char *string, int radix );
char * _ui64toa( __int64 value, char *string, int radix );
char* _ultoa(unsigned long n, char *buff, int base);

extern "C" char  *fcvt(double, int, int *, int *);
extern "C" char  *gcvt(double, int, char *);
int _vsnprintf( char *buffer, size_t count, const char *format, va_list argptr );

HANDLE CreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset,BOOL bInitialState,LPTSTR lpName ); 
BOOL SetEvent(HANDLE hEvent );
DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE *lpHandles, BOOL fWaitAll,DWORD dwMilliseconds );
DWORD FormatMessage (DWORD dwFlags, LPCVOID lpSource,DWORD dwMessageId,DWORD dwLanguageId,LPTSTR lpBuffer,DWORD nSize, va_list *Arguments);
int GetDateFormat(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpDate, LPCTSTR lpFormat, LPTSTR lpDateStr,int cchDate ); 
int GetTimeFormat(LCID Locale, DWORD dwFlags, const SYSTEMTIME *lpTime, LPCTSTR lpFormat, LPTSTR lpTimeStr, int cchTime ); 
int GetNumberFormat(LCID Locale, DWORD dwFlags, LPCTSTR lpValue, const NUMBERFMT *lpFormat, LPTSTR lpNumberStr, int cchNumber );
FARPROC GetProcAddress(HMODULE hModule,LPCWSTR lpProcName); 
HMODULE GetModuleHandle(LPCTSTR lpModuleName );
HLOCAL LocalFree(HLOCAL hMem );
void ODBCNLS_GetCodePage(unsigned long *dwACP);
void ODBCNLS_ValidateLanguage (unsigned long *dwLanguageId);

extern "C" BOOL WriteMyPrivateProfileString(
  LPCTSTR lpAppName,  // section name
  LPCTSTR lpKeyName,  // key name
  LPCTSTR lpString,   // string to add
  LPCTSTR lpFileName  // initialization file
);

extern "C" DWORD GetMyPrivateProfileString(
  LPCTSTR lpAppName,        // section name
  LPCTSTR lpKeyName,        // key name
  LPCTSTR lpDefault,        // default string
  LPTSTR lpReturnedString,  // destination buffer
  DWORD nSize,              // size of destination buffer
  LPCTSTR lpFileName        // initialization file name
);

extern "C" int getODBCDsnError();

extern "C" char* getODBCDsnFileName();

void swap_long_long(char* inp);


//#define TRACE_MEMORY_LEAK

#ifdef TRACE_MEMORY_LEAK
	void operator delete( void *p);
	void operator delete[]( void *p);
	void* operator new[](size_t s);
	void* operator new(size_t s);
#endif

#define markNewOperator\
	markNOperator(__FILE__,__FUNCTION__, __LINE__)

	void listAllocatedMemory(char* description);
	void markNOperator(char* file,char* function, long line);

#endif

//========================================================

#ifndef SRVRTRC_H
#define SRVRTRC_H

#define SRVRTRC_PROCEDURES_0_ //  Add/Remove _0_ to stop/start the server trace log

#ifdef SRVRTRC_PROCEDURES

extern "C" void PrintSrvrTraceFile();

#define SRVRTRC_MAX_LEVEL	30

#define SRVRTRC_START		1*60*1000000 // Start server trace after 10 minutes from connected time.

#define SRVRTRC_ENTER		0
#define SRVRTRC_EXIT		1

#define FILE_LSN			0
#define FILE_LSNS			100
#define FILE_TNSPT			200
#define FILE_TNSPTB			300
#define FILE_TSS			400
#define FILE_CMPRS			500
#define FILE_IIOM			600
#define FILE_IMR			700
#define FILE_AME			800
#define FILE_SME			900
#define FILE_CSTMT			1000
#define FILE_INTF			1100
#define FILE_COMMON			1200
#define FILE_KDS			1300
#define FILE_OIOM			1400
#define FILE_OMR			1500
#define FILE_RSWAP			1600
#define FILE_SWAP			1700
#define FILE_EMS			1800
#define FILE_CLI			1900		// 200 must be reserved
#define FILE_TCP			2100


#define WSQL_AddModule	FILE_CLI + 1
#define WSQL_ADDMODULE	FILE_CLI + 2
#define WSQL_AllocDesc	FILE_CLI + 3
#define WSQL_ALLOCDESC	FILE_CLI + 4
#define WSQL_AssocFileNumber	FILE_CLI + 5
#define WSQL_ASSOCFILENUMBER	FILE_CLI + 6
#define WSQL_AllocStmt	FILE_CLI + 7
#define WSQL_ALLOCSTMT	FILE_CLI + 8
#define WSQL_ClearDiagnostics	FILE_CLI + 9
#define WSQL_CLEARDIAGNOSTICS	FILE_CLI + 10
#define WSQL_CLI_VERSION	FILE_CLI + 11
#define WSQL_CloseStmt	FILE_CLI + 12
#define WSQL_CLOSESTMT	FILE_CLI + 13
#define WSQL_CreateContext	FILE_CLI + 14
#define WSQL_CREATECONTEXT	FILE_CLI + 15
#define WSQL_CurrentContext	FILE_CLI + 16
#define WSQL_CURRENTCONTEXT	FILE_CLI + 17
#define WSQL_DeleteContext	FILE_CLI + 18
#define WSQL_DELETECONTEXT	FILE_CLI + 19
#define WSQL_ResetContext	FILE_CLI + 20
#define WSQL_RESETCONTEXT	FILE_CLI + 21
#define WSQL_DeallocDesc	FILE_CLI + 22
#define WSQL_DEALLOCDESC	FILE_CLI + 23
#define WSQL_DeallocStmt	FILE_CLI + 24
#define WSQL_DEALLOCSTMT	FILE_CLI + 25
#define WSQL_DefineDesc	FILE_CLI + 26
#define WSQL_DEFINEDESC	FILE_CLI + 27
#define WSQL_DescribeStmt	FILE_CLI + 28
#define WSQL_DESCRIBESTMT	FILE_CLI + 29
#define WSQL_DisassocFileNumber	FILE_CLI + 30
#define WSQL_DISASSOCFILENUMBER	FILE_CLI + 31
#define WSQL_DropContext	FILE_CLI + 32
#define WSQL_DROPCONTEXT	FILE_CLI + 33
#define WSQL_Exec	FILE_CLI + 34
#define WSQL_EXEC	FILE_CLI + 35
#define WSQL_ExecClose	FILE_CLI + 36
#define WSQL_EXECCLOSE	FILE_CLI + 37
#define WSQL_ExecDirect	FILE_CLI + 38
#define WSQL_EXECDIRECT	FILE_CLI + 39
#define WSQL_ExecDirectDealloc	FILE_CLI + 40
#define WSQL_EXECDIRECTDEALLOC	FILE_CLI + 41
#define WSQL_ExecFetch	FILE_CLI + 42
#define WSQL_EXECFETCH	FILE_CLI + 43
#define WSQL_ClearExecFetchClose	FILE_CLI + 44
#define WSQL_CLEAREXECFETCHCLOSE	FILE_CLI + 45
#define WSQL_Fetch	FILE_CLI + 46
#define WSQL_FETCH	FILE_CLI + 47
#define WSQL_FetchClose	FILE_CLI + 48
#define WSQL_FETCHCLOSE	FILE_CLI + 49
#define WSQL_FetchMultiple	FILE_CLI + 50
#define WSQL_FETCHMULTIPLE	FILE_CLI + 51
#define WSQL_Cancel	FILE_CLI + 52
#define WSQL_CANCEL	FILE_CLI + 53
#define WSQL_GetDescEntryCount	FILE_CLI + 54
#define WSQL_GETDESCENTRYCOUNT	FILE_CLI + 55
#define WSQL_GetDescItem	FILE_CLI + 56
#define WSQL_GETDESCITEM	FILE_CLI + 57
#define WSQL_GetDescItems	FILE_CLI + 58
#define WSQL_GETDESCITEMS	FILE_CLI + 59
#define WSQL_GetDescItems2	FILE_CLI + 60
#define WSQL_GETDESCITEMS2	FILE_CLI + 61
#define WSQL_GetDiagnosticsStmtInfo	FILE_CLI + 62
#define WSQL_GETDIAGNOSTICSSTMTINFO	FILE_CLI + 63
#define WSQL_GetDiagnosticsStmtInfo2	FILE_CLI + 64
#define WSQL_GETDIAGNOSTICSSTMTINFO2	FILE_CLI + 65
#define WSQL_GetDiagnosticsCondInfo	FILE_CLI + 66
#define WSQL_GetDiagnosticsCondInfo2	FILE_CLI + 67
#define WSQL_GETDIAGNOSTICSCONDINFO	FILE_CLI + 68
#define WSQL_GETDIAGNOSTICSCONDINFO2 FILE_CLI + 69
#define WSQL_GetMainSQLSTATE	FILE_CLI + 70
#define WSQL_GETMAINSQLSTATE	FILE_CLI + 71
#define WSQL_GetCSQLSTATE	FILE_CLI + 72
#define WSQL_GETCSQLSTATE	FILE_CLI + 73
#define WSQL_GetCobolSQLSTATE	FILE_CLI + 74
#define WSQL_GETCOBOLSQLSTATE	FILE_CLI + 75
#define WSQL_GetSQLSTATE	FILE_CLI + 76
#define WSQL_GETSQLSTATE	FILE_CLI + 77
#define WSQL_GetStatistics	FILE_CLI + 78
#define WSQL_GETSTATISTICS	FILE_CLI + 79
#define WSQL_GetStmtAttr	FILE_CLI + 80
#define WSQL_GETSTMTATTR	FILE_CLI + 81
#define WSQL_GETMPCATALOG	FILE_CLI + 82
#define WSQL_GoAway	FILE_CLI + 83
#define WSQL_GOAWAY	FILE_CLI + 84
#define WSQL_Prepare	FILE_CLI + 85
#define WSQL_PREPARE	FILE_CLI + 86
#define WSQL_ResDescName	FILE_CLI + 87
#define WSQL_RESDESCNAME	FILE_CLI + 88
#define WSQL_ResStmtName	FILE_CLI + 89
#define WSQL_RESSTMTNAME	FILE_CLI + 90
#define WSQL_SetCursorName	FILE_CLI + 91
#define WSQL_SETCURSORNAME	FILE_CLI + 92
#define WSQL_SetDescEntryCount	FILE_CLI + 93
#define WSQL_SETDESCENTRYCOUNT	FILE_CLI + 94
#define WSQL_SetDescItem	FILE_CLI + 95
#define WSQL_SETDESCITEM	FILE_CLI + 96
#define WSQL_SetDescItems	FILE_CLI + 97
#define WSQL_SETDESCITEMS	FILE_CLI + 98
#define WSQL_SetDescItems2	FILE_CLI + 99
#define WSQL_SETDESCITEMS2	FILE_CLI + 100
#define WSQL_SetDescPointers	FILE_CLI + 101
#define WSQL_SETDESCPOINTERS	FILE_CLI + 102
#define WSQL_SetRowsetDescPointers	FILE_CLI + 103
#define WSQL_SETROWSETDESCPOINTERS	FILE_CLI + 104
#define WSQL_SetStmtAttr	FILE_CLI + 105
#define WSQL_SETSTMTATTR	FILE_CLI + 106
#define WSQL_SwitchContext	FILE_CLI + 107
#define WSQL_SWITCHCONTEXT	FILE_CLI + 108
#define WSQL_Xact	FILE_CLI + 109
#define WSQL_XACT	FILE_CLI + 110
#define WSQL_SetAuthID	FILE_CLI + 111
#define WSQL_SETAUTHID	FILE_CLI + 112
#define WSQL_AllocDesc_max	FILE_CLI + 113
#define WSQL_GetDescEntryCount_num	FILE_CLI + 114
#define WSQL_SetDescEntryCount_num	FILE_CLI + 115
#define WSQL_GetUniqueQueryIdAttrs	FILE_CLI + 116


//============================================================
#define MAX_EVENT_COUNT 1000
#define SKIPPED_FUNCTION_COUNT	0

typedef struct tag_SRVR_EVENT {
	int EnterExit;
	int level;
	int functionNumber;
	long long functionStartTime;
	long long functionEndTime;
} SRVR_EVENT;

extern SRVR_EVENT srvrEventArray[];
extern int nextEvent;
extern int level;
extern bool	bStartLog;
extern int skippedFunctions;
/*
		if (skippedFunctions++ >= SKIPPED_FUNCTION_COUNT) \
*/
#define PRINTSRVRTRC \
	if (bStartLog && nextEvent < MAX_EVENT_COUNT) \
		PrintSrvrTraceFile();

#define INITSRVRTRC \
	skippedFunctions = 0; \
	nextEvent = 0;		\
	level = 0;			\
	bStartLog = false;

#define SRVRTRACE_ENTER(fileAndfunctionNumber)	\
	long long functionStartTime;				\
	if (bStartLog && nextEvent < MAX_EVENT_COUNT) \
	{											\
		PROCESS_GETINFO_(,						\
				OMITREF, OMITSHORT,OMITREF,		\
				OMITREF,						\
				OMITREF,						\
				OMITREF, OMITSHORT,OMITREF,		\
				&functionStartTime,				\
				OMITREF,						\
				OMITREF,						\
				OMITREF,						\
				OMITREF,						\
				OMITREF, OMITSHORT,OMITREF,		\
				OMITREF, OMITSHORT,OMITREF,		\
				OMITREF,						\
				OMITREF,						\
				OMITREF);						\
		srvrEventArray[nextEvent].EnterExit = SRVRTRC_ENTER; \
		srvrEventArray[nextEvent].level = level++; \
		srvrEventArray[nextEvent].functionNumber = fileAndfunctionNumber; \
		srvrEventArray[nextEvent].functionStartTime = functionStartTime; \
		nextEvent++;							\
	}

#define SRVRTRACE_EXIT(fileAndfunctionNumber)	\
	if (bStartLog && nextEvent < MAX_EVENT_COUNT) \
	{											\
		long long functionEndTime ;				\
		PROCESS_GETINFO_(,						\
				OMITREF, OMITSHORT,OMITREF,		\
				OMITREF,						\
				OMITREF,						\
				OMITREF, OMITSHORT,OMITREF,		\
				&functionEndTime,				\
				OMITREF,						\
				OMITREF,						\
				OMITREF,						\
				OMITREF,						\
				OMITREF, OMITSHORT,OMITREF,		\
				OMITREF, OMITSHORT,OMITREF,		\
				OMITREF,						\
				OMITREF,						\
				OMITREF);						\
		level--;										\
		level = (level < 0)? 0: level;					\
		srvrEventArray[nextEvent].EnterExit = SRVRTRC_EXIT; \
		srvrEventArray[nextEvent].level = level; \
		srvrEventArray[nextEvent].functionNumber = fileAndfunctionNumber; \
		srvrEventArray[nextEvent].functionStartTime = functionStartTime; \
		srvrEventArray[nextEvent].functionEndTime = functionEndTime; \
		nextEvent++;							\
	}

#else

#define PRINTSRVRTRC 
#define INITSRVRTRC

#define SRVRTRACE_ENTER(fileAndfunctionNumber)
#define SRVRTRACE_EXIT(fileAndfunctionNumber)

#endif
#endif




