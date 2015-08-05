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

#include <fs\feerrors.h>
#include <stdio.h>
#include <windows.h>
#include <odbcCommon.h>
#include <odbcsrvrcommon.h>
#include <cextdecs.h>
#include "errno.h"

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
	char *saveptr;

	assembledStr = (char*)malloc( strlen(string) + 1);
	if (assembledStr == NULL ) return string;
	assembledStr[0]=0;

	token = strtok_r( string, sep, &saveptr );
	while( token != NULL )   {
	  strcat(assembledStr, token); 
	  token = strtok_r( NULL, sep, &saveptr );
	  if(token != NULL)
		strcat(assembledStr, sep);
	  }
	strcpy( string, assembledStr);
	free( assembledStr);
	return string;
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

	NODENUMBER_TO_NODENAME_(	-1L,// Node number (if not present or -1 is the current node)
		lpBuffer,	// buffer that contains the name
		*nSize,		// buffer size
		&actualSize);// actual size returned
	lpBuffer[actualSize] = '\0';
	*nSize = actualSize;

	return TRUE;
}

int GetWindowText(HWND hWnd, LPTSTR lpString, int nMaxCount )
{
	NSK_PROCESS_HANDLE pHandle;
	short retlen = 0;
	lpString[0]=0;

	if (PROCESSHANDLE_GETMINE_(pHandle) == 0)
	{
		PROCESSHANDLE_DECOMPOSE_ (
					pHandle
					,OMITREF			//[ short *cpu ]
					,OMITREF			//[ short *pin ]
					,OMITREF			//[ long *nodenumber ]
					,OMITREF			//[ char *nodename ]
					,OMITSHORT			//[ short maxlen ]
					,OMITREF			//[ short *nodename-length ]
					,lpString			//[ char *procname ]
					,nMaxCount			//[ short maxlen ]
					,&retlen			//[ short *procname-length ]
					,OMITREF			//[ long long *sequence-number ] 
					);
		lpString[retlen]=0;
	}
	return retlen;
}
void ODBCNLS_GetCodePage(unsigned long *dwACP)
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


	return processId;
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

int GetNumberFormat(LCID Locale, DWORD dwFlags, LPCTSTR lpValue, const NUMBERFMT *lpFormat, LPTSTR lpNumberStr, int cchNumber )
{
	return 0;
}

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

FARPROC GetProcAddress(HMODULE hModule, LPCWSTR lpProcName)
{
	return NULL;
}

void ODBCNLS_ValidateLanguage (unsigned long *dwLanguageId)
{
}

//=======================Data Sources ============================================

char* getUserFilePath(char* idsnFileName)
{
	static char	dsnFilePath[EXT_FILENAME_LEN+1];

	short	processhandle[10];
	char	object[EXT_FILENAME_LEN+1];
	char	subvol[FULL_SUBVOL_LEN+1];
	short	error;
	short	len;

	memset(dsnFilePath, '\0', EXT_FILENAME_LEN+1);

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

	error = FILE_GETINFOLISTBYNAME_ ( idsnFileName
		,length
		,&item_list
		,number_of_items
		,(short*)&result
		,result_max);

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

		fp = fopen_guardian( idsnFileName,"r");
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
	return fCaches[currentCache-1].error;
}

char* OdbcDsn::getODBCDsnFileName()
{
	return fCaches[currentCache-1].dsnFileName;
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

static char* frmt_elapsed_time(struct timeb StartTime)
{
	static char frmt_buffer[50];
	struct timeb CurrentTime;
	long ElapsedSeconds;
	short ElapsedMilliSec;
	char *timeline;

   ftime(&CurrentTime);

	ElapsedSeconds=(long)((long)CurrentTime.time-(long)StartTime.time);
	ElapsedMilliSec=(signed short)(CurrentTime.millitm-StartTime.millitm);
	if(ElapsedMilliSec<0){
		ElapsedSeconds--;
		ElapsedMilliSec+=1000;
		}

	timeline = ctime( & ( CurrentTime.time ) );
	*(timeline + 20 + 4 ) = 0;
	*(timeline + 19 ) = 0;
	sprintf(frmt_buffer,"%.19s.%hu (%ld.%03d sec.)", &timeline[11], CurrentTime.millitm, ElapsedSeconds, ElapsedMilliSec );

	return frmt_buffer;
}

static char* frmt_current_time(struct timeb CurrentTime)
{
	static char frmt_buffer[50];
	char *timeline;

	timeline = ctime(&(CurrentTime.time));
	*(timeline + 20 + 4 ) = 0;
	*(timeline + 19 ) = 0;
	sprintf( frmt_buffer, "%.19s.%hu",&timeline[11],CurrentTime.millitm);
	return frmt_buffer;
}

static char* frmt_current_date(struct timeb CurrentTime)
{
	static char frmt_buffer[50];
	char *timeline;

	timeline = ctime(&(CurrentTime.time));
	sprintf( frmt_buffer, "%s",timeline);
	frmt_buffer[strlen(frmt_buffer) -1 ] = 0;
	return frmt_buffer;
}


#ifdef SRVRTRC_PROCEDURES

//	if (bStartTrc == 0 && ((JULIANTIMESTAMP() - startJTime) > SRVRTRC_START))

FILE*		fp;
char		srvrTraceFileName[80];

SRVR_EVENT	srvrEventArray[MAX_EVENT_COUNT];
int			nextEvent = 0;
int			level = 0;
long long	startJTime;
bool		bStartLog = false;
int			skippedFunctions = 0;


//===================================================

char* functionNumberToName(int functionNumber)
{

	char	functionName[80];
	int		fileName;

	fileName = functionNumber/100;

	switch (functionNumber)
	{
		// Listener.cpp file
		case FILE_LSN+1:return "constr_CTimer";break;
		case FILE_LSN+2:return "timer_restart";break;
		case FILE_LSN+3:return "constr_CTimer_list";break;
		case FILE_LSN+4:return "destr_CTimer_list";break;
		case FILE_LSN+5:return "timer_create";break;
		case FILE_LSN+6:return "timer_destroy";break;
		case FILE_LSN+7:return "find_timerByhandle";break;
		case FILE_LSN+8:return "del_timer_node";break;
		case FILE_LSN+9:return "constr_CNSKListener";break;
		case FILE_LSN+10:return "CheckReceiveMessage";break;
		case FILE_LSN+11:return "runProgram";break;

		// Listener_srvr.cpp file
		case FILE_LSNS+1:return "constr_CNSKListenerSrvr";break;
		case FILE_LSNS+2:return "destr_CNSKListenerSrvr";break;
		case FILE_LSNS+3:return "OpenTCPIPSession";break;
		case FILE_LSNS+4:return "CheckTCPIPRequest";break;
		case FILE_LSNS+5:return "ListenToPort";break;
		case FILE_LSNS+6:return "runProgram";break;

		// Transport.cpp file
		case FILE_TNSPT+1:return "constr_CTransport";break;
		case FILE_TNSPT+2:return "destr_CTransport";break;
		case FILE_TNSPT+3:return "log_error";break;
		case FILE_TNSPT+4:return "log_info";break;

		// TransportBase.cpp file
		case FILE_TNSPTB+1:return "ADD_ONE_TO_HANDLE";break;
		case FILE_TNSPTB+2:return "CEE_HANDLE_IS_NIL";break;
		case FILE_TNSPTB+3:return "CEE_HANDLE_SET_NIL";break;
		case FILE_TNSPTB+4:return "constr_CTempMemory";break;
		case FILE_TNSPTB+5:return "destr_CTempMemory";break;
		case FILE_TNSPTB+6:return "constr_CTempMemory_list";break;
		case FILE_TNSPTB+7:return "destr_CTempMemory_list";break;
		case FILE_TNSPTB+8:return "add_mem";break;
		case FILE_TNSPTB+9:return "del_mem";break;
		case FILE_TNSPTB+10:return "del_mem_node";break;
		case FILE_TNSPTB+11:return "del_mem_callid";break;
		case FILE_TNSPTB+12:return "del_tmp_allocate";break;
		case FILE_TNSPTB+13:return "constr_CError";break;
		case FILE_TNSPTB+14:return "destr_CError";break;

		// TCPIPSystemSrvr.cpp file
		case FILE_TSS+1:return "process_swap_tss_";break;
		case FILE_TSS+2:return "cleanup_tss_";break;
		case FILE_TSS+3:return "ins_node_tss_";break;
		case FILE_TSS+4:return "del_node_ipnode_tss_";break;
		case FILE_TSS+5:return "del_node_call_id_tss_";break;
		case FILE_TSS+6:return "find_node_call_id_tss_";break;
		case FILE_TSS+7:return "find_node_nSocketFnum_tss_";break;
		case FILE_TSS+8:return "cleanup_node_tss_";break;
		case FILE_TSS+9:return "enum_nodes_tss_";break;
		case FILE_TSS+10:return "send_error_tss_";break;
		case FILE_TSS+11:return "send_response_tss_";break;
		case FILE_TSS+12:return "getMsgHeader_tss_";break;
		case FILE_TSS+13:return "do_read_tss_";break;
		case FILE_TSS+14:return "do_write_tss_";break;
		case FILE_TSS+15:return "READ_TCPIP_REQUEST_tss_";break;
		case FILE_TSS+16:return "WRITE_TCPIP_RESPONSE_tss_";break;
		case FILE_TSS+17:return "PROCESS_TCPIP_REQUEST_tss_";break;
		case FILE_TSS+18:return "BUILD_TCPIP_REQUEST_tss_";break;
		case FILE_TSS+19:return "DoCompression_tss_";break;
		case FILE_TSS+20:return "DoExpand_tss_";break;
		case FILE_TSS+21:return "cnstrct_CTCPIPSystemSrvr";break;
		case FILE_TSS+22:return "dstrct_CTCPIPSystemSrvr";break;
		case FILE_TSS+23:return "w_allocate";break;
		case FILE_TSS+24:return "r_allocate";break;
		case FILE_TSS+25:return "r_assign";break;
		case FILE_TSS+26:return "w_assign";break;
		case FILE_TSS+27:return "w_release";break;
		case FILE_TSS+28:return "r_release";break;
		case FILE_TSS+29:return "transport";break;

		// Compression.cpp file
		case FILE_CMPRS+1:return "cnstrct_CCompression";break;
		case FILE_CMPRS+2:return "dstrct_CCompression";break;
		case FILE_CMPRS+3:return "compress";break;
		case FILE_CMPRS+4:return "find_match";break;
		case FILE_CMPRS+5:return "expand";break;
		case FILE_CMPRS+6:return "decode_string";break;
		case FILE_CMPRS+7:return "input_code";break;
		case FILE_CMPRS+8:return "output_code";break;

		// TCPIPSystemSrvr.cpp file but TCP/IP and filesystem calls
		case FILE_TCP+1:return "send_nw2";break;
		case FILE_TCP+2:return "AWAITIOX";break;
		case FILE_TCP+3:return "socket_get_len";break;

		// odbcs_srvr.cpp file
		case FILE_IIOM+1:return "DISPATCH_TCPIPRequest_iom_";break;
		case FILE_IIOM+2:return "DISPATCH_IOMessage_iom_";break;
		case FILE_IIOM+3:return "DISPATCH_PROCDEATH_SMessage_iom_";break;
		case FILE_IIOM+4:return "DISPATCH_CPUDOWN_SMessage_iom_";break;
		case FILE_IIOM+5:return "DISPATCH_OPEN_SMessage_iom_";break;
		case FILE_IIOM+6:return "DISPATCH_CLOSE_SMessage_iom_";break;
		case FILE_IIOM+7:return "DISPATCH_DEVINFO_SMessage_iom_";break;
		case FILE_IIOM+8:return "DISPATCH_CONFIGINFO_SMessage_iom_";break;

		// marshaling.cpp file
		case FILE_IMR+1:return "decodeParameters_mrs_";break;
		case FILE_IMR+2:return "ERROR_DESC_LIST_length_mrs_";break;
		case FILE_IMR+3:return "SRVR_CONTEXT_length_mrs_";break;
		case FILE_IMR+4:return "DATASOURCE_CFG_LIST_length_mrs_";break;
		case FILE_IMR+5:return "SQLVALUE_LIST_length_mrs_";break;
		case FILE_IMR+6:return "SRVR_STATUS_LIST_length_mrs_";break;
		case FILE_IMR+7:return "ENV_DESC_LIST_length_mrs_";break;
		case FILE_IMR+8:return "RES_DESC_LIST_length_mrs_";break;
		case FILE_IMR+9:return "DATASOURCE_CFG_length_mrs_";break;
		case FILE_IMR+10:return "ERROR_DESC_LIST_copy_mrs_";break;
		case FILE_IMR+11:return "SRVR_CONTEXT_copy_mrs_";break;
		case FILE_IMR+12:return "DATASOURCE_CFG_LIST_copy_mrs_";break;
		case FILE_IMR+13:return "SQLVALUE_LIST_copy_mrs_";break;
		case FILE_IMR+14:return "SRVR_STATUS_LIST_copy_mrs_";break;
		case FILE_IMR+15:return "ENV_DESC_LIST_copy_mrs_";break;
		case FILE_IMR+16:return "RES_DESC_LIST_copy_mrs_";break;
		case FILE_IMR+17:return "DATASOURCE_CFG_copy_mrs_";break;

		// srvrconnect.cpp file
		case FILE_AME+1:return "RegisterSrvr";break;
		case FILE_AME+2:return "ASTimerExpired";break;
		case FILE_AME+3:return "checkIfASSvcLives";break;
		case FILE_AME+4:return "RegProcess_ccf_";break;
		case FILE_AME+5:return "InitializeDialogue_ame_";break;
		case FILE_AME+6:return "TerminateDialogue_ame_";break;
		case FILE_AME+7:return "BreakDialogue";break;
		case FILE_AME+8:return "connIdleTimerExpired";break;
		case FILE_AME+9:return "srvrIdleTimerExpired";break;
		case FILE_AME+10:return "WouldLikeToLive_ccf_";break;
		case FILE_AME+11:return "destroyConnIdleTimer";break;
		case FILE_AME+12:return "startConnIdleTimer";break;
		case FILE_AME+13:return "updateSrvrState";break;
		case FILE_AME+14:return "UPDATE_SERVER_WAITED";break;
		case FILE_AME+15:return "UpdateSrvrState_ccf_";break;
		case FILE_AME+16:return "exitServerProcess";break;
		case FILE_AME+17:return "StopServer_ame_";break;
		case FILE_AME+18:return "Close_ame_";break;
		case FILE_AME+19:return "ExecuteN_ame_";break;
		case FILE_AME+20:return "EndTransaction_ame_";break;
		case FILE_AME+21:return "SetConnectionOption_ame_";break;
		case FILE_AME+22:return "Prepare_ame_";break;
		case FILE_AME+23:return "ExecDirect_ame_";break;
		case FILE_AME+24:return "FetchPerf_ame_";break;
		case FILE_AME+25:return "FetchRowset_ame_";break;
		case FILE_AME+26:return "PrepareRowset_ame_";break;
		case FILE_AME+27:return "ExecDirectRowset_ame_";break;
		case FILE_AME+28:return "ExecuteRowset_ame_";break;
		case FILE_AME+29:return "EnableServerTrace_ame_";break;
		case FILE_AME+30:return "DisableServerTrace_ame_";break;
		case FILE_AME+31:return "EnableServerStatistics_ame_";break;
		case FILE_AME+32:return "DisableServerStatistics_ame_";break;
		case FILE_AME+33:return "UPDATE_SERVER_CONTEXT";break;
		case FILE_AME+34:return "UpdateServerContext_ame_";break;
		case FILE_AME+35:return "ExecuteCall_ame_";break;
		case FILE_AME+36:return "MonitorCall_ame_";break;

		// srvrothers.cpp file
		case FILE_SME+1:return "Prepare_sme_";break;
		case FILE_SME+2:return "ExecuteN_sme_";break;
		case FILE_SME+3:return "Close_sme_";break;
		case FILE_SME+4:return "FetchN_sme_";break;
		case FILE_SME+5:return "EndTransaction_sme_";break;
		case FILE_SME+6:return "ExecDirect_sme_";break;
		case FILE_SME+7:return "CancelStatement_sme_";break;
		case FILE_SME+8:return "FetchPerf_sme_";break;
		case FILE_SME+9:return "FetchRowset_sme_";break;
		case FILE_SME+10:return "PrepareRowset_sme_";break;
		case FILE_SME+11:return "ExecDirectRowset_sme_";break;
		case FILE_SME+12:return "ExecuteRowset_sme_";break;
		case FILE_SME+13:return "ExecuteCall_sme_";break;
		case FILE_SME+14:return "GetSQLCatalogs_sme_";break;
		case FILE_SME+15:return "InitializeDialog_sme_";break;
		case FILE_SME+16:return "TerminateDialog_sme_";break;
		case FILE_SME+17:return "SetConnectionOption_sme_";break;
		case FILE_SME+18:return "Prepare2_sme_";break;
		case FILE_SME+19:return "Execute2_sme_";break;

		// CSrvrStmt.cpp file
		case FILE_CSTMT+1:return "SRVR_STMT_HDL_con_cstmt";break;
		case FILE_CSTMT+2:return "SRVR_STMT_HDL_dis_cstmt";break;
		case FILE_CSTMT+3:return "Prepare_cstmt_";break;
		case FILE_CSTMT+4:return "Execute_cstmt_";break;
		case FILE_CSTMT+5:return "Close_cstmt_";break;
		case FILE_CSTMT+6:return "InternalStmtClose_cstmt_";break;
		case FILE_CSTMT+7:return "Fetch_cstmt_";break;
		case FILE_CSTMT+8:return "ExecDirect_cstmt_";break;
		case FILE_CSTMT+9:return "Cancel_cstmt_";break;
		case FILE_CSTMT+10:return "cleanupSQLMessage_cstmt_";break;
		case FILE_CSTMT+11:return "cleanupSQLValueList_cstmt_";break;
		case FILE_CSTMT+12:return "cleanupSQLDescList_cstmt_";break;
		case FILE_CSTMT+13:return "cleanupAll_cstmt_";break;
		case FILE_CSTMT+14:return "GetCharset_cstmt_";break;
		case FILE_CSTMT+15:return "ControlProc_cstmt_";break;
		case FILE_CSTMT+16:return "FetchPerf_cstmt_";break;
		case FILE_CSTMT+17:return "FetchRowset_cstmt_";break;
		case FILE_CSTMT+18:return "PrepareRowset_cstmt_";break;
		case FILE_CSTMT+19:return "ExecDirectRowset_cstmt_";break;
		case FILE_CSTMT+20:return "ExecuteRowset_cstmt_";break;
		case FILE_CSTMT+21:return "PrepareFromModule_cstmt_";break;
		case FILE_CSTMT+22:return "freeBuffers_cstmt_";break;
		case FILE_CSTMT+23:return "allocSqlmxHdls_cstmt_";break;
		case FILE_CSTMT+24:return "ExecuteCall_cstmt_";break;
		case FILE_CSTMT+25:return "FetchCatalogRowset_cstmt_";break;

		// sqlinterface.cpp file
		case FILE_INTF+1:return "GetODBCValues_intf_";break;
		case FILE_INTF+2:return "GetRowsetODBCValues_intf_";break;
		case FILE_INTF+3:return "SetDataPtr_intf_";break;
		case FILE_INTF+4:return "SetRowsetDataPtr_intf_";break;
		case FILE_INTF+5:return "AllocAssignValueBuffer_intf_";break;
		case FILE_INTF+6:return "CopyValueList_intf_";break;
		case FILE_INTF+7:return "BuildSQLDesc_intf_";break;
		case FILE_INTF+8:return "BuildRowsetSQLDesc_intf_";break;
		case FILE_INTF+9:return "SqlMaxDataLength_intf_";break;
		case FILE_INTF+10:return "dataLength_intf_";break;
		case FILE_INTF+11:return "recompileInsert_intf_";break;
		case FILE_INTF+12:return "adjustVarcharType_intf_";break;
		case FILE_INTF+13:return "EXECUTE_intf_";break;
		case FILE_INTF+14:return "EXECUTEROWSET_intf_";break;
		case FILE_INTF+15:return "RECOVERY_FROM_ROWSET_ERROR_intf_";break;
		case FILE_INTF+16:return "INSERT_NODE_TO_LIST_intf_";break;
		case FILE_INTF+17:return "COPYSQLERROR_LIST_TO_SRVRSTMT_intf_";break;
		case FILE_INTF+18:return "ADDSQLERROR_TO_LIST_intf_";break;
		case FILE_INTF+19:return "COMMIT_ROWSET_intf_";break;
		case FILE_INTF+20:return "GETSQLERROR_AND_ROWCOUNT_intf_";break;
		case FILE_INTF+21:return "FREESTATEMENT_intf_";break;
		case FILE_INTF+22:return "RESOURCEGOV_intf_";break;
		case FILE_INTF+23:return "PREPARE_intf_";break;
		case FILE_INTF+24:return "PREPARE_FROM_MODULE_intf_";break;
		case FILE_INTF+25:return "PREPAREROWSET_intf_";break;
		case FILE_INTF+26:return "FETCH_intf_";break;
		case FILE_INTF+27:return "allocGlobalBuffer_intf_";break;
		case FILE_INTF+28:return "releaseGlobalBuffer_intf_";break;
		case FILE_INTF+29:return "FETCHPERF_intf_";	break;
		case FILE_INTF+30:return "FETCHROWSET_intf_";break;
		case FILE_INTF+31:return "GETSQLERROR_intf_";break;
		case FILE_INTF+32:return "EXECDIRECT_intf_";break;
		case FILE_INTF+33:return "EXECDIRECT_INTERNAL_intf_";break;
		case FILE_INTF+34:return "EXECDIRECTROWSET_intf_";break;
		case FILE_INTF+35:return "GETSQLWARNING_intf_";break;
		case FILE_INTF+36:return "CANCEL_intf_";break;
		case FILE_INTF+37:return "ALLOCSQLMXHDLS_intf_";break;
		case FILE_INTF+38:return "EXECUTECALL_intf_";break;
		case FILE_INTF+39:return "FETCHCATALOGPERF_intf_";break;

		// svrvcommon.cpp file
		case FILE_COMMON+1:return "allocSrvrSessionHdl_cmn_";break;
		case FILE_COMMON+2:return "initSqlCore_cmn_";break;
		case FILE_COMMON+3:return "allocSrvrStmtHdlList_cmn_";break;
		case FILE_COMMON+4:return "addSrvrStmt_cmn_";break;
		case FILE_COMMON+5:return "removeSrvrStmt_cmn_";break;
		case FILE_COMMON+6:return "allocSrvrStmtHdl_cmn_";break;
		case FILE_COMMON+7:return "getSrvrStmt_cmn_";break;
		case FILE_COMMON+8:return "releaseCachedObject_cmn_";break;
		case FILE_COMMON+9:	return "do_ExecSql_cmn_";break;
		case FILE_COMMON+10:return "do_ExecSMD_cmn_";break;
		case FILE_COMMON+11:return "convertWildcard_cmn_";break;
		case FILE_COMMON+12:return "convertWildcardNoEsc_cmn_";break;
		case FILE_COMMON+13:return "checkIfWildCard_cmn_";break;
		case FILE_COMMON+14:return "executeSQLQuery_cmn_";break;
		case FILE_COMMON+15:return "executeAndFetchSQLQuery_cmn_";break;
		case FILE_COMMON+16:return "executeAndFetchSMDQuery_cmn_";break;
		case FILE_COMMON+17:return "completeTransaction_cmn_";break;
		case FILE_COMMON+18:return "SetAutoCommitOff_cmn_";break;
		case FILE_COMMON+19:return "execDirectSQLQuery_cmn_";break;
		case FILE_COMMON+20:return "writeServerException_cmn_";break;
		case FILE_COMMON+21:return "getSessionId_cmn_";	break;
		case FILE_COMMON+22:return "getJulianTime_cmn_";break;
		case FILE_COMMON+23:return "getCurrentTime_cmn_";break;
		case FILE_COMMON+24:return "do_ExecFetchSMD_cmn_";break;

		// svrvkds.cpp file
		case FILE_KDS+1:return "kdsCreateSQLDescSeq_kds_";break;
		case FILE_KDS+2:return "kdsCreateEmptySQLDescSeq_kds_";	break;
		case FILE_KDS+3:return "kdsCopyToSQLDescSeq_kds_";break;
		case FILE_KDS+4:return "kdsCreateSQLErrorException_kds_";break;
		case FILE_KDS+5:return "kdsCopySQLErrorException_kds_";break;
		case FILE_KDS+6:return "kdsCopySQLErrorExceptionAndRowCount_kds_";break;
		case FILE_KDS+7:return "kdsCopyToSQLValueSeq_kds_";break;
		case FILE_KDS+8:return "kdsCopyToSQLDataSeq_kds_";break;
		case FILE_KDS+9:return "kdsCopyToSMDSQLValueSeq_kds_";break;
		case FILE_KDS+10:return "kdsCreateSQLWarningException_kds_";break;
		case FILE_KDS+11:return "CopyRGMsg_kds_";break;
		case FILE_KDS+12:return "kdsCopyRGWarningException_kds_";break;
		case FILE_KDS+13:return "kdsCopyRGErrorException_kds_";break;

		// odbcs_srvr_res.cpp file
		case FILE_OIOM+1:return "InitializeDialogue_ts_res_";break;
		case FILE_OIOM+2:return "TerminateDialogue_ts_res_";break;
		case FILE_OIOM+3:return "SetConnectionOption_ts_res_";break;
		case FILE_OIOM+4:return "Prepare_ts_res_";break;
		case FILE_OIOM+5:return "ExecDirect_ts_res_";break;
		case FILE_OIOM+6:return "PrepareRowset_ts_res_";break;
		case FILE_OIOM+7:return "ExecuteRowset_ts_res_";break;
		case FILE_OIOM+8:return "ExecDirectRowset_ts_res_";break;
		case FILE_OIOM+9:return "FetchPerf_ts_res_";break;
		case FILE_OIOM+10:return "FetchRowset_ts_res_";break;
		case FILE_OIOM+11:return "ExecuteN_ts_res_";break;
		case FILE_OIOM+12:return "Close_ts_res_";break;
		case FILE_OIOM+13:return "EndTransaction_ts_res_";break;
		case FILE_OIOM+14:return "GetSQLCatalogs_ts_res_";break;
		case FILE_OIOM+15:return "ExecuteCall_ts_res_";break;
		case FILE_OIOM+16:return "StopServer_ts_res_";break;
		case FILE_OIOM+17:return "EnableServerTrace_ts_res_";break;
		case FILE_OIOM+18:return "DisableServerTrace_ts_res_";break;
		case FILE_OIOM+19:return "EnableServerStatistics_ts_res_";break;
		case FILE_OIOM+20:return "DisableServerStatistics_ts_res_";	break;
		case FILE_OIOM+21:return "UpdateServerContext_ts_res_";	break;
		case FILE_OIOM+22:return "MonitorCall_ts_res_";	break;

		// marshalingsrvr_srvr.cpp file
		case FILE_OMR+1:return "InitializeDialogue_param_res_";	break;
		case FILE_OMR+2:return "TerminateDialogue_param_res_";break;
		case FILE_OMR+3:return "SetConnectionOption_param_res_";break;
		case FILE_OMR+4:return "Prepare_param_res_";break;
		case FILE_OMR+5:return "ExecDirect_param_res_";	break;
		case FILE_OMR+6:return "PrepareRowset_param_res_";break;
		case FILE_OMR+7:return "ExecuteRowset_param_res_";break;
		case FILE_OMR+8:return "ExecDirectRowset_param_res_";break;
		case FILE_OMR+9:return "FetchPerf_param_res_";break;
		case FILE_OMR+10:return "FetchRowset_param_res_";break;
		case FILE_OMR+11:return "ExecuteN_param_res_";break;
		case FILE_OMR+12:return "Close_param_res_";	break;
		case FILE_OMR+13:return "EndTransaction_param_res_";break;
		case FILE_OMR+14:return "GetSQLCatalogs_param_res_";break;
		case FILE_OMR+15:return "ExecuteCall_param_res_";break;
		case FILE_OMR+16:return "StopServer_param_res_";break;
		case FILE_OMR+17:return "EnableServerTrace_param_res_";	break;
		case FILE_OMR+18:return "DisableServerTrace_param_res_";break;
		case FILE_OMR+19:return "EnableServerStatistics_param_res_";break;
		case FILE_OMR+20:return "DisableServerStatistics_param_res_";break;
		case FILE_OMR+21:return "UpdateServerContext_param_res_";break;
		case FILE_OMR+22:return "MonitorCall_param_res_";break;

		// swaps_srvr.cpp file
		case FILE_RSWAP+1:return "PROCESS_res_swap";break;

		// swap.cpp file
 		case FILE_SWAP+1:return "swapPointers";	break;
		case FILE_SWAP+2:return "SHORT_swap";break;
		case FILE_SWAP+3:return "USHORT_swap";break;
		case FILE_SWAP+4:return "LONG_swap";break;
		case FILE_SWAP+5:return "ULONG_swap";break;
		case FILE_SWAP+6:return "POINTER_swap";	break;
		case FILE_SWAP+7:return "LONGLONG_swap";break;
		case FILE_SWAP+8:return "VERSION_LIST_swap";break;
		case FILE_SWAP+9:return "HEADER_swap";	break;
		case FILE_SWAP+10:return "ERROR_DESC_LIST_swap";break;
		case FILE_SWAP+11:return "OUT_CONNECTION_CONTEXT_swap";	break;
		case FILE_SWAP+12:return "SQL_VALUE_LIST_swap";	break;
		case FILE_SWAP+13:return "SQL_ITEM_DESC_LIST_swap";	break;

		// EventMsgs.cpp file
		case FILE_EMS+1:return "ODBCMXEventMsg_ems_";break;
		case FILE_EMS+2:return "SendEventMsg_ems_";	break;
		case FILE_EMS+3:return "FindClusterName_ems_";	break;
		case FILE_EMS+4:return "ReadNodeNumInRegistry_ems_";break;
		case FILE_EMS+5:return "FindMyNodeNum_ems_";break;
		case FILE_EMS+6:return "open_ems_name_ems_";break;
		case FILE_EMS+7:return "open_ems_ems_";	break;
		case FILE_EMS+8:return "close_ems_ems_";break;
		case FILE_EMS+9:return "format_spi_token_ems_";	break;
		case FILE_EMS+10:return "display_spi_error_ems_";break;
		case FILE_EMS+11:return "send_to_ems_ems_";	break;


		case WSQL_AddModule: return "WSQL_AddModule";break;
		case WSQL_ADDMODULE	: return "WSQL_ADDMODULE";break;
		case WSQL_AllocDesc	: return "WSQL_AllocDesc";break;
		case WSQL_ALLOCDESC	: return "WSQL_ALLOCDESC";break;
		case WSQL_AssocFileNumber	: return "WSQL_AssocFileNumber";break;
		case WSQL_ASSOCFILENUMBER	: return "WSQL_ASSOCFILENUMBER";break;
		case WSQL_AllocStmt	: return "WSQL_AllocStmt";break;
		case WSQL_ALLOCSTMT	: return "WSQL_ALLOCSTMT";break;
		case WSQL_ClearDiagnostics	: return "WSQL_ClearDiagnostics";break;
		case WSQL_CLEARDIAGNOSTICS	: return "WSQL_CLEARDIAGNOSTICS";break;
		case WSQL_CLI_VERSION	: return "WSQL_CLI_VERSION";break;
		case WSQL_CloseStmt	: return "WSQL_CloseStmt";break;
		case WSQL_CLOSESTMT	: return "WSQL_CLOSESTMT";break;
		case WSQL_CreateContext	: return "WSQL_CreateContext";break;
		case WSQL_CREATECONTEXT	: return "WSQL_CREATECONTEXT";break;
		case WSQL_CurrentContext	: return "WSQL_CurrentContext";break;
		case WSQL_CURRENTCONTEXT	: return "WSQL_CURRENTCONTEXT";break;
		case WSQL_DeleteContext	: return "WSQL_DeleteContext";break;
		case WSQL_DELETECONTEXT	: return "WSQL_DELETECONTEXT";break;
		case WSQL_ResetContext	: return "WSQL_ResetContext";break;
		case WSQL_RESETCONTEXT	: return "WSQL_RESETCONTEXT";break;
		case WSQL_DeallocDesc	: return "WSQL_DeallocDesc";break;
		case WSQL_DEALLOCDESC	: return "WSQL_DEALLOCDESC";break;
		case WSQL_DeallocStmt	: return "WSQL_DeallocStmt";break;
		case WSQL_DEALLOCSTMT	: return "WSQL_DEALLOCSTMT";break;
		case WSQL_DefineDesc	: return "WSQL_DefineDesc";break;
		case WSQL_DEFINEDESC	: return "WSQL_DEFINEDESC";break;
		case WSQL_DescribeStmt	: return "WSQL_DescribeStmt";break;
		case WSQL_DESCRIBESTMT	: return "WSQL_DESCRIBESTMT";break;
		case WSQL_DisassocFileNumber	: return "WSQL_DisassocFileNumber";break;
		case WSQL_DISASSOCFILENUMBER	: return "WSQL_DISASSOCFILENUMBER";break;
		case WSQL_DropContext	: return "WSQL_DropContext";break;
		case WSQL_DROPCONTEXT	: return "WSQL_DROPCONTEXT";break;
		case WSQL_Exec	: return "WSQL_Exec";break;
		case WSQL_EXEC	: return "WSQL_EXEC";break;
		case WSQL_ExecClose	: return "WSQL_ExecClose";break;
		case WSQL_EXECCLOSE	: return "WSQL_EXECCLOSE";break;
		case WSQL_ExecDirect	: return "WSQL_ExecDirect";break;
		case WSQL_EXECDIRECT	: return "WSQL_EXECDIRECT";break;
		case WSQL_ExecDirectDealloc	: return "WSQL_ExecDirectDealloc";break;
		case WSQL_EXECDIRECTDEALLOC	: return "WSQL_EXECDIRECTDEALLOC";break;
		case WSQL_ExecFetch	: return "WSQL_ExecFetch";break;
		case WSQL_EXECFETCH	: return "WSQL_EXECFETCH";break;
		case WSQL_ClearExecFetchClose	: return "WSQL_ClearExecFetchClose";break;
		case WSQL_CLEAREXECFETCHCLOSE	: return "WSQL_CLEAREXECFETCHCLOSE";break;
		case WSQL_Fetch	: return "WSQL_Fetch";break;
		case WSQL_FETCH	: return "WSQL_FETCH";break;
		case WSQL_FetchClose	: return "WSQL_FetchClose";break;
		case WSQL_FETCHCLOSE	: return "WSQL_FETCHCLOSE";break;
		case WSQL_FetchMultiple	: return "WSQL_FetchMultiple";break;
		case WSQL_FETCHMULTIPLE	: return "WSQL_FETCHMULTIPLE";break;
		case WSQL_Cancel	: return "WSQL_Cancel";break;
		case WSQL_CANCEL	: return "WSQL_CANCEL";break;
		case WSQL_GetDescEntryCount	: return "WSQL_GetDescEntryCount";break;
		case WSQL_GETDESCENTRYCOUNT	: return "WSQL_GETDESCENTRYCOUNT";break;
		case WSQL_GetDescItem	: return "WSQL_GetDescItem";break;
		case WSQL_GETDESCITEM	: return "WSQL_GETDESCITEM";break;
		case WSQL_GetDescItems	: return "WSQL_GetDescItems";break;
		case WSQL_GETDESCITEMS	: return "WSQL_GETDESCITEMS";break;
		case WSQL_GetDescItems2	: return "WSQL_GetDescItems2";break;
		case WSQL_GETDESCITEMS2	: return "WSQL_GETDESCITEMS2";break;
		case WSQL_GetDiagnosticsStmtInfo	: return "WSQL_GetDiagnosticsStmtInfo";break;
		case WSQL_GETDIAGNOSTICSSTMTINFO	: return "WSQL_GETDIAGNOSTICSSTMTINFO";break;
		case WSQL_GetDiagnosticsStmtInfo2	: return "WSQL_GetDiagnosticsStmtInfo2";break;
		case WSQL_GETDIAGNOSTICSSTMTINFO2	: return "WSQL_GETDIAGNOSTICSSTMTINFO2";break;
		case WSQL_GetDiagnosticsCondInfo	: return "WSQL_GetDiagnosticsCondInfo";break;
		case WSQL_GetDiagnosticsCondInfo2	: return "WSQL_GetDiagnosticsCondInfo2";break;
		case WSQL_GETDIAGNOSTICSCONDINFO	: return "WSQL_GETDIAGNOSTICSCONDINFO";break;
		case WSQL_GETDIAGNOSTICSCONDINFO2: return "WSQL_GETDIAGNOSTICSCONDINFO2";break;
		case WSQL_GetMainSQLSTATE	: return "WSQL_GetMainSQLSTATE";break;
		case WSQL_GETMAINSQLSTATE	: return "WSQL_GETMAINSQLSTATE";break;
		case WSQL_GetCSQLSTATE	: return "WSQL_GetCSQLSTATE";break;
		case WSQL_GETCSQLSTATE	: return "WSQL_GETCSQLSTATE";break;
		case WSQL_GetCobolSQLSTATE	: return "WSQL_GetCobolSQLSTATE";break;
		case WSQL_GETCOBOLSQLSTATE	: return "WSQL_GETCOBOLSQLSTATE";break;
		case WSQL_GetSQLSTATE	: return "WSQL_GetSQLSTATE";break;
		case WSQL_GETSQLSTATE	: return "WSQL_GETSQLSTATE";break;
		case WSQL_GetStatistics	: return "WSQL_GetStatistics";break;
		case WSQL_GETSTATISTICS	: return "WSQL_GETSTATISTICS";break;
		case WSQL_GetStmtAttr	: return "WSQL_GetStmtAttr";break;
		case WSQL_GETSTMTATTR	: return "WSQL_GETSTMTATTR";break;
		case WSQL_GETMPCATALOG	: return "WSQL_GETMPCATALOG";break;
		case WSQL_GoAway	: return "WSQL_GoAway";break;
		case WSQL_GOAWAY	: return "WSQL_GOAWAY";break;
		case WSQL_Prepare	: return "WSQL_Prepare";break;
		case WSQL_PREPARE	: return "WSQL_PREPARE";break;
		case WSQL_ResDescName	: return "WSQL_ResDescName";break;
		case WSQL_RESDESCNAME	: return "WSQL_RESDESCNAME";break;
		case WSQL_ResStmtName	: return "WSQL_ResStmtName";break;
		case WSQL_RESSTMTNAME	: return "WSQL_RESSTMTNAME";break;
		case WSQL_SetCursorName	: return "WSQL_SetCursorName";break;
		case WSQL_SETCURSORNAME	: return "WSQL_SETCURSORNAME";break;
		case WSQL_SetDescEntryCount	: return "WSQL_SetDescEntryCount";break;
		case WSQL_SETDESCENTRYCOUNT	: return "WSQL_SETDESCENTRYCOUNT";break;
		case WSQL_SetDescItem	: return "WSQL_SetDescItem";break;
		case WSQL_SETDESCITEM	: return "WSQL_SETDESCITEM";break;
		case WSQL_SetDescItems	: return "WSQL_SetDescItems";break;
		case WSQL_SETDESCITEMS	: return "WSQL_SETDESCITEMS";break;
		case WSQL_SetDescItems2	: return "WSQL_SetDescItems2";break;
		case WSQL_SETDESCITEMS2	: return "WSQL_SETDESCITEMS2";break;
		case WSQL_SetDescPointers	: return "WSQL_SetDescPointers";break;
		case WSQL_SETDESCPOINTERS	: return "WSQL_SETDESCPOINTERS";break;
		case WSQL_SetRowsetDescPointers	: return "WSQL_SetRowsetDescPointers";break;
		case WSQL_SETROWSETDESCPOINTERS	: return "WSQL_SETROWSETDESCPOINTERS";break;
		case WSQL_SetStmtAttr	: return "WSQL_SetStmtAttr";break;
		case WSQL_SETSTMTATTR	: return "WSQL_SETSTMTATTR";break;
		case WSQL_SwitchContext	: return "WSQL_SwitchContext";break;
		case WSQL_SWITCHCONTEXT	: return "WSQL_SWITCHCONTEXT";break;
		case WSQL_Xact	: return "WSQL_Xact";break;
		case WSQL_XACT	: return "WSQL_XACT";break;
		case WSQL_SetAuthID	: return "WSQL_SetAuthID";break;
		case WSQL_SETAUTHID	: return "WSQL_SETAUTHID";break;
		case WSQL_AllocDesc_max	: return "WSQL_AllocDesc_max";break;
		case WSQL_GetDescEntryCount_num	: return "WSQL_GetDescEntryCount_num";break;
		case WSQL_SetDescEntryCount_num	: return "WSQL_SetDescEntryCount_num";break;

		default:
			_itoa(functionNumber,functionName,10);
			break;
	}

	return functionName;
}

extern "C" void PrintSrvrTraceFile()
{
	char tab[] = "                                              ";
	int level;

	struct timeb CurrentTime;

	short error;
	short processId;
	short cpuNumber;
	short errorDetail;
	short procname_len = 0;
	char  procname[81];
	short maxlen = sizeof(procname)-1;
	short processHandle[10];

	procname[0]=0;
	PROCESSHANDLE_GETMINE_(processHandle);
	if ((error = PROCESSHANDLE_DECOMPOSE_ (
					processHandle
					,&cpuNumber			//[ short *cpu ]
					,&processId			//[ short *pin ]
					,OMITREF			//[ long *nodenumber ]
					,OMITREF			//[ char *nodename ]
					,OMITSHORT			//[ short maxlen ]
					,OMITREF			//[ short *nodename-length ]
					,procname			//[ char *procname ]
					,maxlen				//[ short maxlen ]
					,&procname_len		//[ short *procname-length ]
					,OMITREF			//[ long long *sequence-number ] 
					)) != 0)
	{
		return;
	}
	procname[procname_len] = 0;

	ftime(&CurrentTime);

	sprintf(srvrTraceFileName, "TT%s",procname+1);
	fp = fopen_guardian(srvrTraceFileName,"wa");
	if (fp != NULL)
	{
		fprintf(fp,"<==========SERVER TRACE (%s)==========>\n",frmt_current_date(CurrentTime));
		fprintf(fp,"<==========PROCESS %s (%02d,%03d) ==========>\n\n",procname, cpuNumber, processId);
		fflush(fp);

		long long dExecTime = 0;
		for (int i = 0; i < nextEvent; i++)
		{
			int level = srvrEventArray[i].level;
			tab[level] = 0;

			if (srvrEventArray[i].EnterExit == SRVRTRC_ENTER)
			{
				if (srvrEventArray[i+1].EnterExit == SRVRTRC_ENTER)
				{
					fprintf(fp,"%02d %sEnter: %s; bExecTime: %Ld dEE: %Ld\n",
						level,
						tab,
						functionNumberToName(srvrEventArray[i].functionNumber), 
						srvrEventArray[i].functionStartTime, srvrEventArray[i+1].functionStartTime-srvrEventArray[i].functionStartTime);
				}
				else
				{
					fprintf(fp,"%02d %sEnter: %s; bExecTime: %Ld\n",
						level,
						tab,
						functionNumberToName(srvrEventArray[i].functionNumber), 
						srvrEventArray[i].functionStartTime);
				}
			}
			else
			{
				if (srvrEventArray[i+1].EnterExit == SRVRTRC_EXIT)
				{
					fprintf(fp,"%02d %sExit: %s; eExecTime: %Ld dXX: %Ld; dExecTime: %Ld\n", 
						level,
						tab,
						functionNumberToName(srvrEventArray[i].functionNumber), 
						srvrEventArray[i].functionEndTime,srvrEventArray[i+1].functionEndTime-srvrEventArray[i].functionEndTime, 
						srvrEventArray[i].functionEndTime - srvrEventArray[i].functionStartTime);
				}
				else
				{
					fprintf(fp,"%02d %sExit: %s; eExecTime: %Ld; dExecTime: %Ld\n", 
						level,
						tab,
						functionNumberToName(srvrEventArray[i].functionNumber), 
						srvrEventArray[i].functionEndTime, 
						srvrEventArray[i].functionEndTime - srvrEventArray[i].functionStartTime);
				}
			}
			tab[level] = ' ';
		}
		fflush(fp);
		fclose(fp);

	}
}

#endif
