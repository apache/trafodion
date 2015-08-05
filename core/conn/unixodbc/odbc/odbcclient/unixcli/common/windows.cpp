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

#include <feerrors.h>
#include <stdio.h>
#include <windows.h>
#include <odbccommon.h>
#include <odbcsrvrcommon.h>
#ifndef unixcli
#include <cextdecs.h>
#endif
#include "errno.h"
#ifdef MXOSS
#include <spthread.h>
#else
#include <pthread.h>
#endif

#if defined MXHPUX && !defined MXOSS
#include <sys/param.h>
#include <sys/pstat.h>
#endif
#include <unistd.h>
#include <sys/types.h>

#ifdef MXAIX
#include <sys/procfs.h>
#include <procinfo.h>
#endif

#ifdef MXSUNSPARC
#include <procfs.h>
#include <fcntl.h>
#endif

/*
char * _strupr(char *);
char * _itoa(int, char *, int);
char * _ltoa(long, char *, int);
*/

char * _strupr(char * str) 
{

	char *s;
    for (s=str ; *s != '\0'; s++)  if (*s <= 'z' && *s >= 'a')  *s &= 0xdf;
  
	return str;
}


char * _itoa(int n, char *buff, int base) {

   char t[100], *c=t, *f=buff;
   int d;
   char sign;
   int bit;
   unsigned int un;

   if (base == 10) {
		if (n < 0) {
			*(f++) = '-';
			un = -n;
		} else
			un = n;

	   while ( un > 0) {
			d = un % base;
			*(c++) = d + '0';
			un = un / base;
	   }
   } else {
	  if (base == 2) bit = 1;
      else if (base == 8) bit = 3;
      else if (base == 16) bit = 4;
      else 
		  return "";

	  while (n != 0) {
		 d = (n  & (base-1));
		 *(c++) = d < 10 ? d + '0' : d - 10 + 'A';
		 n = (unsigned int) n >> bit;
	  }

   }

   c--;

   while (c >= t) *(f++) = *(c--);
     
   if (buff == f)
	   *(f++) = '0';
   *f = '\0';
   return buff;
}


char * _ltoa(long n, char *buff, int base) 
{

   char t[100], *c=t, *f=buff;
   long d;
   char sign;
   int bit;
   unsigned long un;

   if (base == 10) {
     if (n < 0) {
        *(f++) = '-';
        un = -n;
	 }
	 else
		 un = n;

     while (un > 0) {
        d = un % base;
        *(c++) = d + '0';
        un = un / base;
	 }
   }
   
   else {

     if (base == 2) bit = 1;
     else if (base == 8) bit = 3;
     else if (base == 16) bit = 4;
     else 
	 { base = 16; bit = 4; }  // printf("Base value unknown!\n");

     while (n != 0) {
        d = (n  & (base-1));
        *(c++) = d < 10 ? d + '0' : d - 10 + 'A';
        n = (unsigned int) n >> bit;
	 }

   }

   c--;

   while (c >= t) *(f++) = *(c--);
     
   if (buff == f)
	   *(f++) = '0';
   *f = '\0';
   return buff;
}

char *_strdup( const char *strSource )
{
	char* tp = NULL;
	if ((tp = (char*)malloc(strlen(strSource)+1))!=NULL)
		strcpy(tp,strSource);
	return tp;
}


char* _ultoa(unsigned long n, char *buff, int base)
{

   char t[100], *c=t, *f=buff;
   unsigned long d;
   int bit;

   if (base == 10) {

     while (n > 0) {
        d = n % base;
        *(c++) = d + '0';
        n = n / base;
	 }
   }
   
   else {

     if (base == 2) bit = 1;
     else if (base == 8) bit = 3;
     else if (base == 16) bit = 4;
     else 
	 { base = 16; bit =4;} // printf("Base value unknown!\n");

     while (n != 0) {
        d = (n  & (base-1));
        *(c++) = d < 10 ? d + '0' : d - 10 + 'A';
        n = (unsigned long) n >> bit;
	 }

   }

   c--;

   while (c >= t) *(f++) = *(c--);
     
   if (buff == f)
	   *(f++) = '0';
   *f = '\0';
   return buff;
}

char *_i64toa( __int64 n, char *buff, int base )
{
   char t[100], *c=t, *f=buff;
   long d;
    int bit;

   if (base == 10) {
     if (n < 0) {
        *(f++) = '-';
        n = -n;
	 }

     while (n != 0) {
        d = n % base;
		if (d < 0) d = -d;
        *(c++) = d + '0';
        n = n / base;
	 }
   }
   
   else {
	 short bitlen = 64;

     if (base == 2) bit = 1;
     else if (base == 8) bit = 3;
     else if (base == 16) bit = 4;
	 { base = 16; bit =4;} // printf("Base value unknown!\n");

     while (bitlen != 0) {
        d = (n  & (base-1));
        *(c++) = d < 10 ? d + '0' : d - 10 + 'A';
        n =  n >> bit;
		bitlen -= bit;
	 }

   }

   c--;

   while (c >= t) *(f++) = *(c--);
     
   if (buff == f)
	   *(f++) = '0';
   *f = '\0';
   return buff;
}

// Add definition of this function for converting unsigned long long to char*
char *_ui64toa(unsigned __int64 n, char *buff, int base )
{
   char t[100], *c=t, *f=buff;
   long d;
    int bit;

   if (base == 10) {
     while (n != 0) {
        d = n % base;
		if (d < 0) d = -d;
        *(c++) = d + '0';
        n = n / base;
	 }
   }
   
   else {
	 short bitlen = 64;

     if (base == 2) bit = 1;
     else if (base == 8) bit = 3;
     else if (base == 16) bit = 4;
	 { base = 16; bit =4;} // printf("Base value unknown!\n");

     while (bitlen != 0) {
        d = (n  & (base-1));
        *(c++) = d < 10 ? d + '0' : d - 10 + 'A';
        n =  n >> bit;
		bitlen -= bit;
	 }

   }

   c--;

   while (c >= t) *(f++) = *(c--);
     
   if (buff == f)
	   *(f++) = '0';
   *f = '\0';
   return buff;
}

__int64 _atoi64( const char *s )
{
	__int64 n = 0;
	char* t = (char*)s;
	char c;

	while(*t != 0)
	{
		c = *t++;
		if (c < '0' || c > '9') continue;
		n = n * 10 +c - '0';
	}
	if (*s == '-') n = -n;
	return n;
}


char* trim(char *string)
{
	char sep[] = " ";
	char *token;
	char *assembledStr;
	char *lasts;

	assembledStr = (char*)malloc( strlen(string) + 1);
	if (assembledStr == NULL ) return string;
	assembledStr[0]=0;
#ifndef unixcli
	token = strtok( string, sep);   
	while( token != NULL )   {
	  strcat(assembledStr, token); 
	  token = strtok( NULL, sep);
	  if(token != NULL)
		strcat(assembledStr, sep);
	  }
	strcpy( string, assembledStr);
	free( assembledStr);
	return string;
#else
	token = strtok_r( string, sep, &lasts );   
	while( token != NULL )   {
	  strcat(assembledStr, token); 
	  token = strtok_r( NULL, sep, &lasts );
	  if(token != NULL)
		strcat(assembledStr, sep);
	  }
	strcpy( string, assembledStr);
	free( assembledStr);
	return string;
#endif
}

char * _strnset(char * str, int c, unsigned int count) 
{

	char *s;
	int i = 0;
    for (s=str ; i < count; s++,i++) *s = c;
  
	return str;
}

BOOL GetComputerName (LPSTR lpBuffer, LPDWORD nSize)
{
	short actualSize;
#ifndef unixcli
	NODENUMBER_TO_NODENAME_(	-1L,// Node number (if not present or -1 is the current node)
		lpBuffer,	// buffer that contains the name
		*nSize,		// buffer size
		&actualSize);// actual size returned
#endif
	lpBuffer[actualSize] = '\0';
	*nSize = actualSize;

	return TRUE;
}

int GetWindowText(HWND hWnd, LPTSTR lpString, int nMaxCount )
{
	NSK_PROCESS_HANDLE pHandle;
	short retlen = 0;
	lpString[0]=0;
	char *spaceChar = NULL;

    int mypid;

#if defined MXHPUX && !defined MXOSS
    struct pst_status myproc;
#endif

#if defined (MXLINUX) || defined(MXIA64LINUX)
    char           buf[256];
    FILE         * file;
#endif

#if defined MXAIX || defined MXSUNSPARC
	char			buf[256];
	struct psinfo pb;
	int file;
	int count;
#endif

    mypid = getpid();
	
#if defined MXHPUX && !defined MXOSS
// on HPUX we can get the appname from the procces structure pst_status
    int status;
	
	status = pstat_getproc(&myproc, sizeof(myproc), (size_t)0, mypid);
	if (status != -1)
		strncpy(lpString, myproc.pst_ucomm, nMaxCount);
	else
		sprintf(lpString,"UNKNOWN");
#endif

#if defined (MXLINUX) || defined(MXIA64LINUX)
// on Linux systems read the appname from the /proc/'pid'/cmdline file
    snprintf( buf, 256, "/proc/%d/cmdline", mypid );

    if ((file = fopen( buf, "r" )) == NULL)
		sprintf(lpString, "UNKNOWN");
	else
	{
    	fgets( lpString, nMaxCount, file );
    	fclose( file );
	}
#endif

#if defined MXAIX || defined MXSUNSPARC
	snprintf( buf, 256, "/proc/%d/psinfo", mypid );

	if ((file = open( buf, O_RDONLY )) == NULL)
		sprintf(lpString, "UNKNOWN");
	else
	{
    	count = read(file, &pb, sizeof(pb));
		if (count != 0)
			strncpy(lpString, pb.pr_fname, nMaxCount);
		else
			sprintf(lpString,"UNKNOWN");

		close(file);
	}
#endif

#ifdef MXOSS
		sprintf(lpString,"OSSApp");	
#endif
	lpString[nMaxCount-1] = '\0';
	if(NULL !=  (spaceChar = strchr(lpString, ' ')))
		*spaceChar = '\0';
	return (strlen(lpString));
}
void ODBCNLS_GetCodePage(unsigned int *dwACP)
{
	*dwACP = 0;
}


// thread functions
struct myThreadDef {
	int ThreadId;
	int ThreadType;
    LPTHREAD_START_ROUTINE lpStartAddress;
    LPVOID lpParameter;
    DWORD dwCreationFlags;
    DWORD ExitCode;
} myThreadDef;

BOOL CloseHandle(HANDLE hObject)
{
	struct myThreadDef * myThread;
	myThread = 	(struct myThreadDef *)hObject;
	if (myThread != NULL)
	{
		delete myThread;
		return TRUE;
	}
	return FALSE;
}
BOOL
GetExitCodeThread(
    HANDLE hThread,
    LPDWORD lpExitCode
    )
{
	struct myThreadDef * myThread;
	myThread = (struct myThreadDef *) hThread;
	if (myThread != NULL)
	{
		*lpExitCode = myThread->ExitCode;
		return TRUE;
	}
	return FALSE;
}

HANDLE
CreateThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    DWORD dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    )
{
	struct myThreadDef *myThread;

	myThread = (struct myThreadDef *) new (struct myThreadDef);
	myThread->lpStartAddress = lpStartAddress;
	myThread->lpParameter = lpParameter;
	myThread->dwCreationFlags = dwCreationFlags;

	if (dwCreationFlags != CREATE_SUSPENDED)
	{
		// call the thread function right away
		myThread->ExitCode = (*lpStartAddress)(lpParameter);
	}
	return (HANDLE) myThread;
}


DWORD
SuspendThread(
    HANDLE hThread
    )
{
	// we can't suspend it, return error
	return 0xFFFFFFFF;
}

DWORD
ResumeThread(
    HANDLE hThread
    )
{
	struct myThreadDef *myThread;

	myThread = (struct myThreadDef *) hThread;
	if (myThread != NULL)
	{
		// call the stored function
		myThread->ExitCode = (*myThread->lpStartAddress)(myThread->lpParameter);
	}
	return 1; // (the thread was suspended and successfully restarted)
}

DWORD
WaitForSingleObject(
    HANDLE hHandle,
    DWORD dwMilliseconds
    )
{
	// I don't think there is a need to wait ??
	return WAIT_OBJECT_0;
}


DWORD	GetCurrentProcessId()
{
	short error;
	short processId;
	short cpuNumber;
	short errorDetail;
	NSK_PROCESS_HANDLE pHandle;
#ifndef unixcli
	if ((error = PROCESSHANDLE_NULLIT_ (pHandle)) != 0)
		return -1;

	if ((error = PROCESSHANDLE_GETMINE_(pHandle)) != 0)
		return -1;

	if ((error = PROCESSHANDLE_DECOMPOSE_ (pHandle
						, &cpuNumber
						, &processId)) != 0)
		return -1;

	/*if ((error = PROCESS_GETINFO_(pHandle,
			OMITREF, OMITSHORT,OMITREF,		// proc string,max buf len,act len
			OMITREF,						// priority
			OMITREF,						// Mom's proc handle 
			OMITREF, OMITSHORT,OMITREF,		// home term,max buf len,act len  
			OMITREF,						// Process execution time 
			OMITREF,						// Creator Access Id 
			OMITREF,						// Process Access Id 
			OMITREF,						// Grand Mom's proc handle 
			OMITREF,						// Job Id 
			OMITREF, OMITSHORT,OMITREF,		// Program file,max buf len,act len  
			OMITREF, OMITSHORT,OMITREF,		// Swap file,max buf len,act len 
			&errorDetail,
			OMITREF,						// Process type 
			&processId) ) != 0)

	{
		return -1;
	}
	*/

#endif
	return processId;
}

void	GetCurrentProcessAndThreadId(char* buff)
{
	pid_t pid = getpid();
	pthread_t thread_id = pthread_self();
	sprintf(buff,"%d.%u", pid,thread_id);
}


DWORD	GetCurrentThreadId()
{
	return 0;
}

void OutputDebugString( char *lpOutputString)
{
	printf("%s\n",lpOutputString);
}

//========================= NSK Driver Conversion ====================

int _vsnprintf( char *buffer, size_t count, const char *format, va_list argptr )
{ 
	return vsprintf(buffer, format, argptr);
}

HANDLE CreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset,BOOL bInitialState,LPTSTR lpName ) 
{
	return NULL;
}
 
BOOL SetEvent(HANDLE hEvent )
{ 
	return TRUE;
}

DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE *lpHandles, BOOL fWaitAll,DWORD dwMilliseconds )
{
	return WAIT_OBJECT_0;
}

DWORD FormatMessage (DWORD dwFlags, LPCVOID lpSource,DWORD dwMessageId,DWORD dwLanguageId,LPTSTR lpBuffer,DWORD nSize, va_list *Arguments)
{
	if ((lpBuffer = (char*)malloc(33))==NULL)
		return 0;
	lpBuffer[0]=0;
	ltoa(dwMessageId,lpBuffer,10);
	return strlen(lpBuffer);
}

int GetDateFormat(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpDate, LPCTSTR lpFormat, LPTSTR lpDateStr,int cchDate )
{
	return 0;
}

int GetTimeFormat(LCID Locale, DWORD dwFlags, const SYSTEMTIME *lpTime, LPCTSTR lpFormat, LPTSTR lpTimeStr, int cchTime )
{
	return 0;
}

// this is already defined as an inline in windows.h
#ifndef unixcli
int GetNumberFormat(LCID Locale, DWORD dwFlags, LPCTSTR lpValue, const NUMBERFMT *lpFormat, LPTSTR lpNumberStr, int cchNumber )
{
	return 0;
}
#endif

BOOL FreeLibrary( HMODULE hLibModule)
{
	return TRUE;
}

HMODULE GetModuleHandle(LPCTSTR lpModuleName )
{ 
	return NULL;
}

int GetLastError()
{
	return 999;
}

HLOCAL LocalFree(HLOCAL hMem )
{
	if (hMem != NULL) free (hMem);
	return NULL;
}

FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	return NULL;
}

void ODBCNLS_ValidateLanguage (unsigned long *dwLanguageId)
{
}

//=======================Data Sources ============================================
char* getEnvFilePath(char* pEnvName)
{
	static char	dsnFilePath[EXT_FILENAME_LEN+1];
	char *p=NULL;
	dsnFilePath[0]='\0';
	if(!pEnvName)
	  return pEnvName;
#ifdef unixcli
	FILE *file=NULL;
	if((p=getenv(pEnvName))!=NULL)
	  {
	    file = fopen (p, "r");
	    if(file){
	      // valid file
	      fclose(file);
	      strcpy(dsnFilePath,p);
	    }
	  }
#endif
	return dsnFilePath;
}
char* getUserFilePath(char* idsnFileName)
{
	static char	dsnFilePath[EXT_FILENAME_LEN+1];

	short	processhandle[10];
	char	object[EXT_FILENAME_LEN+1];
	char	subvol[FULL_SUBVOL_LEN+1];
	short	error;
	short	len;

	memset(dsnFilePath, '\0', EXT_FILENAME_LEN+1);
#ifndef unixcli
	PROCESSHANDLE_GETMINE_(processhandle);
	error = PROCESS_GETINFO_(processhandle
                           , /* proc_fname */
                           , /* maxlen     */
                           , /* len        */
                           , /* priority   */
                           , /* mon-processhandle */
                           , /* hometerm   */
                           , /* max-len    */
                           , /* len        */
                           , /* processtime */
                           , /* creator_access_id */
                           , /* process-access-id */
                           , /* gmon-process-handle */
                           , /* job-id */
                           , object
                           , EXT_FILENAME_LEN
                           , &len);
	if (error == FEOK)
	{
		object[len]='\0';
		error = FILENAME_DECOMPOSE_(object,
								   (short)strlen(object),
								   subvol,
								   FULL_SUBVOL_LEN,
								   &len,
								   1,
								   0x02);
		if (error == FEOK)
		{
			subvol[len]='\0';
			strncpy(dsnFilePath, subvol, len);
			strcat(dsnFilePath, ".");
			len = sizeof(dsnFilePath) - strlen(dsnFilePath);
			if (len > 0 )
			{
				strncat(dsnFilePath, idsnFileName, len);
				dsnFilePath[sizeof(dsnFilePath)-1]=0;
			}
		}
	}
#endif
	sprintf(dsnFilePath,"./%s",idsnFileName);
	return dsnFilePath;
}

long long getFileLastModificationTime(char* idsnFileName)
{
	short	error;
	short	length = strlen(idsnFileName);
	short item_list = 144;					// Last file modification time
	short number_of_items = 1;
	long long result = 0;
	short result_max = 8;
#ifndef unixcli
	error = FILE_GETINFOLISTBYNAME_ ( idsnFileName
		,length
		,&item_list
		,number_of_items
		,(short*)&result
		,result_max);
#endif
	if (error != FEOK)
		return 0;
	return result;
}

//---------------------------------------------------

OdbcDsn odbcDsn = OdbcDsn();

OdbcDsnLine::OdbcDsnLine()
{
}

OdbcDsnLine::OdbcDsnLine(char* buffer)
{
	next = NULL;
	strncpy(this->buffer,buffer,sizeof(this->buffer));
}

OdbcDsnLine::~OdbcDsnLine()
{
}

OdbcDsn::OdbcDsn()
{
	memset(&fCaches, 0, sizeof(fCaches));
	currentLineInDsnCache = NULL;
	currentCache = 0;
}

OdbcDsn::~OdbcDsn()
{
	deleteDsnCache(1);
	deleteDsnCache(2);
}

void OdbcDsn::deleteDsnCache(int cache)
{
	OdbcDsnLine* pdsnLine;
	pdsnLine = fCaches[cache-1].dsnCache;

	OdbcDsnLine* ptmpdsnLine;
	while (pdsnLine != NULL)
	{
		ptmpdsnLine = pdsnLine->next;
		delete pdsnLine;
		pdsnLine = ptmpdsnLine;
	}
	fCaches[cache-1].error = 0;
	fCaches[cache-1].dsnFileName[0] = 0;
	fCaches[cache-1].lastFileModificationTime = 0;
	fCaches[cache-1].dsnCache = NULL;
}

void OdbcDsn::insertIntoDsnCache(char* readbuffer)
{
	OdbcDsnLine* ptmpdsnLine;

	OdbcDsnLine* pdsnLine = new OdbcDsnLine(readbuffer);
	if (pdsnLine == NULL)
		return;
	if (fCaches[currentCache-1].dsnCache == NULL)
	{
		fCaches[currentCache-1].dsnCache = pdsnLine;
		return;
	}
	ptmpdsnLine = fCaches[currentCache-1].dsnCache;
	while (ptmpdsnLine->next != NULL)
	{
		ptmpdsnLine = ptmpdsnLine->next;
	}
	ptmpdsnLine->next = pdsnLine;
}

char* OdbcDsn::getNextLineFromDsnCache()
{
	if (currentLineInDsnCache == NULL)
	{
		currentLineInDsnCache = fCaches[currentCache-1].dsnCache;
		strcpy(readbuffer, currentLineInDsnCache->buffer);
		return readbuffer;
	}
	if (currentLineInDsnCache->next == NULL)
		return NULL;
	currentLineInDsnCache = currentLineInDsnCache->next;
	strcpy(readbuffer, currentLineInDsnCache->buffer);
	return readbuffer;
}


void OdbcDsn::openFile(char* idsnFileName)
{
	FILE* fp;
	long long tmpLastFileModificationTime;
	bool rebuildDsnCache = false;

	currentLineInDsnCache = 0;
	currentCache = 0;

	if (strcmp(fCaches[0].dsnFileName, idsnFileName) == 0)
		currentCache = 1;
	else if (strcmp(fCaches[1].dsnFileName, idsnFileName) == 0)
		currentCache = 2;
	else if (fCaches[0].dsnFileName[0] == 0)
		currentCache = 1;
	else if (fCaches[1].dsnFileName[0] == 0)
		currentCache = 2;
	else
		currentCache = 1;

	tmpLastFileModificationTime = getFileLastModificationTime(idsnFileName);

	if (fCaches[currentCache-1].dsnFileName[0] == 0 || strcmp(fCaches[currentCache-1].dsnFileName,idsnFileName) != 0)
	{
		rebuildDsnCache = true;
		deleteDsnCache(currentCache);
	}

	if (rebuildDsnCache || fCaches[currentCache-1].lastFileModificationTime != tmpLastFileModificationTime)
	{
		fCaches[currentCache-1].error = 0;
		strncpy(fCaches[currentCache-1].dsnFileName,idsnFileName,sizeof(fCaches[currentCache-1].dsnFileName));
		fCaches[currentCache-1].dsnFileName[sizeof(fCaches[currentCache-1].dsnFileName)-1]=0;
#ifndef unixcli
		fp = fopen_guardian( idsnFileName,"r");
#else
		fp = fopen( idsnFileName,"r");
#endif
		if (fp==NULL)
		{
			if (rebuildDsnCache == true)	//if we can not open the file and we have a cache we ignore the error
				fCaches[currentCache-1].error = errno;
			return;
		}
		fCaches[currentCache-1].lastFileModificationTime = tmpLastFileModificationTime;
		while(fgets( readbuffer, sizeof(readbuffer), fp)!=NULL)
		{
			readbuffer[strlen(readbuffer)-1]=0;
			trim(readbuffer);
			insertIntoDsnCache(readbuffer);
		}
		fclose(fp);
	}
}

int OdbcDsn::getODBCDsnError()
{
#ifdef ASYNCIO
	dsnMutex.Lock();
#endif
	int error = fCaches[currentCache-1].error;
#ifdef ASYNCIO
	dsnMutex.UnLock();
#endif
	return error;
}

char* OdbcDsn::getODBCDsnFileName()
{
#ifdef ASYNCIO
	dsnMutex.Lock();
#endif
	char *dsnFileName = fCaches[currentCache-1].dsnFileName;
#ifdef ASYNCIO
	dsnMutex.UnLock();
#endif
	return dsnFileName;
}

short OdbcDsn::readFile()
{

	if (fCaches[currentCache-1].dsnCache != NULL)
	{
		if(getNextLineFromDsnCache()!=NULL)
			return 1;
	}
	return 0;
}

BOOL OdbcDsn::isFileOpen()
{
	return fCaches[currentCache-1].dsnCache!=NULL;
}

BOOL OdbcDsn::WritePrivateProfileString(
	  LPCTSTR lpAppName,  // section name
	  LPCTSTR lpKeyName,  // key name
	  LPCTSTR lpString,   // string to add
	  LPCTSTR lpFileName  // initialization file
)
{
	return TRUE;
}

DWORD OdbcDsn::GetPrivateProfileString(
	  LPCTSTR lpAppName,        // section name
	  LPCTSTR lpKeyName,        // key name
	  LPCTSTR lpDefault,        // default string
	  LPTSTR lpReturnedString,  // destination buffer
	  DWORD nSize,              // size of destination buffer
	  LPCTSTR lpFileName        // initialization file name
)
{
	BOOL bFoundApp = FALSE;
	BOOL bFoundKey = FALSE;

	char tmpAppName[80];
	char tmpKeyName[80];
	char tmpBuffer[256];
	short lenKey;
	short lenApp;
	char* istr;
	int i;

	short lenReturnedString=0;
	if (lpAppName == NULL || lpKeyName == NULL || lpReturnedString == NULL || lpFileName == NULL)
		return 0;
#ifdef ASYNCIO
	dsnMutex.Lock();
#endif

	memset(tmpAppName,0,80);
	strcpy(tmpAppName,"[");
	strncat(tmpAppName,lpAppName,77);
	strcat(tmpAppName,"]");
	lenApp = strlen(tmpAppName);

	memset(tmpKeyName,0,80);
	strncpy(tmpKeyName,lpKeyName,78);
	strcat(tmpKeyName,"=");
	lenKey = strlen(tmpKeyName);
	openFile((char*)lpFileName);
	if (isFileOpen())
	{
		while(readFile())
		{
			istr = readbuffer;
			if (bFoundApp == FALSE)
			{
				while (*istr == ' ' || *istr == '\t') istr++;
				if (*istr != '[') continue;
				i=0;
				do
				{
					tmpBuffer[i++] = *istr++;
				}
				while(*istr != ']');
				tmpBuffer[i++]=*istr;
				tmpBuffer[i]=0;
				if (strncmp(tmpBuffer,tmpAppName,lenApp)==0)
					bFoundApp = TRUE;
			}
			else
			{
				while (*istr == ' ' || *istr == '\t') istr++;
				if (*istr=='[')	break;
				i=0;
				do
				{
					if (*istr == ' ' || *istr == '\t') {istr++;continue;}
					tmpBuffer[i++] = *istr++;
				}
				while (*istr != '=');
				tmpBuffer[i++]=*istr++;
				tmpBuffer[i]=0;
				if (strncmp(tmpBuffer,tmpKeyName,lenKey)==0)
				{
					while (*istr == ' ' || *istr == '\t') istr++;
					strncpy(lpReturnedString,istr,nSize);
					lpReturnedString[nSize-1]=0;
					trim(lpReturnedString);
					lenReturnedString = strlen(lpReturnedString);
					bFoundKey = TRUE;
					break;
				}
			}

		}
		if (lpDefault != NULL && bFoundApp && bFoundKey == FALSE)
		{
			strncpy(lpReturnedString,lpDefault,nSize);
			lpReturnedString[nSize-1]=0;
			trim(lpReturnedString);
			lenReturnedString = strlen(lpReturnedString);
		}
	}

	if (isFileOpen()== FALSE)
		lenReturnedString = -2;
	else if (bFoundApp == FALSE)
		lenReturnedString = -1;
#ifdef ASYNCIO
	dsnMutex.UnLock();
#endif

	return lenReturnedString; // number of characters copied to the buffer excluding NULL
}

extern "C" int getODBCDsnError()
{
	return odbcDsn.getODBCDsnError();
}

extern "C" char* getODBCDsnFileName()
{
	return odbcDsn.getODBCDsnFileName();
}

extern "C" BOOL WriteMyPrivateProfileString(
	  LPCTSTR lpAppName,  // section name
	  LPCTSTR lpKeyName,  // key name
	  LPCTSTR lpString,   // string to add
	  LPCTSTR lpFileName  // initialization file
)
{
	return odbcDsn.WritePrivateProfileString( 
		lpAppName,
		lpKeyName,
		lpString,
		lpFileName);
}

extern "C" DWORD GetMyPrivateProfileString(
	  LPCTSTR lpAppName,        // section name
	  LPCTSTR lpKeyName,        // key name
	  LPCTSTR lpDefault,        // default string
	  LPTSTR lpReturnedString,  // destination buffer
	  DWORD nSize,              // size of destination buffer
	  LPCTSTR lpFileName        // initialization file name
)
{
	return odbcDsn.GetPrivateProfileString(
		lpAppName,
		lpKeyName,
		lpDefault,
		lpReturnedString,
		nSize,
		lpFileName);
}

void swap_long_long(char* inp)
{
	char buffer[8];
	for (int i=0; i<8;i++)
	{
		buffer[i] = inp[7-i];
	}
	memcpy(inp,buffer,8);
}

#ifdef OSS_DRIVER

// This patch is for Krypton PC4110 
/*
FILE * fopen ( const char * filename, const char * mode )
{
	FILE* pFile;

	pFile = (FILE* ) fopen_guardian( filename, mode );
	return pFile;
}
*/
/* End of fopen */
 
#endif

#ifdef TRACE_MEMORY_LEAK

//---------------------- LinkList -------------------------------
char gFile[51];
char gFunction[51];
long  gLine;

typedef struct node 
{
	void* p;
	size_t size;
	long id;
	char file[51];
	char function[51];
	long line;
	node* next;
} node;

typedef struct nlist 
{
	node* list;
	nlist()
	{
		list=NULL;
		gFile[0]=0;
		gFunction[0]=0;
		gLine=0;
	}
	~nlist()
	{
		node* cnode = list;
		node* nnode;
		while( cnode != NULL )
		{
			nnode = cnode->next;
			free(cnode);
			cnode = nnode;
		}
		list=NULL;
	}

	list_node(char* description)
	{
		char fileName[80];

		sprintf( fileName, "ML%d",GetCurrentProcessId());
		FILE *fp = fopen_guardian( fileName,"w");
		if (fp != NULL)
		{
			fprintf(fp,"---- %s ----\n",description);
			long total = 0,lp=0;
			node* cnode = list;
			node* nnode;
			while( cnode != NULL ){
				nnode = cnode->next;
				total += cnode->size;
				fprintf(fp,"%d id = %d, size = %d, file=%s, function=%s, line=%d\n",
					++lp,cnode->id,cnode->size,cnode->file,cnode->function,cnode->line);
				cnode = nnode;
			}
			fprintf(fp,"Total memory %d\n",total);
			fclose(fp);
		}
	}

	bool ins_node( void* p, long id, size_t size)
	{
		node* cnode = list;
		node* pnode = list;
		node* nnode;

		while(cnode!=NULL )
		{
			pnode=cnode;
			cnode=cnode->next;
		}
		if((nnode = (node*)malloc(sizeof(node)))!=NULL)
		{
			nnode->id = id;
			nnode->p = p;
			nnode->size = size;
			if (gFile[0] != 0)
			{
				strncpy(nnode->file,gFile,50);
				nnode->file[50]=0;
				gFile[0]=0;
			}
			if (gFunction[0] != 0)
			{
				strncpy(nnode->function,gFunction,50);
				nnode->function[50]=0;
				gFunction[0] = 0;
			}
			nnode->line=gLine;
			gLine = 0;
			nnode->next = cnode;
			if(pnode!=NULL) 
				pnode->next = nnode;
			else
				list = nnode;
		}
		return (nnode == NULL)?false:true;
	}
	bool del_node(void* p )
	{
		node* cnode = list;
		node* pnode = list;
		while( cnode!= NULL )
		{
			if ( p == cnode->p )
				break;
			pnode = cnode;
			cnode = cnode->next;
		}
		if( cnode==NULL)
			return false;
		if (pnode == list && cnode == list)
			list = cnode->next;
		else
			pnode->next = cnode->next;
		free(cnode);
		return true;
	}
	bool find_node(void* p )
	{
		node* cnode = list;
		while( cnode != NULL )
		{
			if ( p == cnode->p )
				break;
			cnode = cnode->next;
		}
		if( cnode==NULL)
			return false;
		return true;
	}
} nlist;

nlist memList;
static long id = 1;

void* operator_new(size_t nSize)
{
	void* p=malloc(nSize);
	memList.ins_node(p,id++,nSize);
	return p;
}

void* operator new[](size_t s)
{ 
	return operator_new(s); 
}

void* operator new(size_t s)
{ 
	return operator_new(s); 
}

void operator delete( void *p)
{
	memList.del_node(p);
	free(p);
}

void operator delete[]( void *p)
{
	memList.del_node(p);
	free(p);
}
#endif

void listAllocatedMemory(char* description)
{
#ifdef TRACE_MEMORY_LEAK
	memList.list_node(description);
#endif
}

void markNOperator(char* file,char* function,long line)
{
#ifdef TRACE_MEMORY_LEAK
	strncpy(gFile,file,50);
	strncpy(gFunction,function,50);
	gLine=line;
#endif
}
















