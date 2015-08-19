/*************************************************************************
*
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

#include <stdio.h>
#include <windows.h>
#include "cextdecs.h"


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

   if (base == 10) {
     if (n < 0) {
        *(f++) = '-';
        n = -n;
     }

   while ( n > 0) {
      d = n % base;
      *(c++) = d + '0';
      n = n / base;
   }
   
   }
   
   else {
	  if (base == 2) bit = 1;
      else if (base == 8) bit = 3;
      else if (base == 16) bit = 4;
      else return NULL; // Base value unknown!

	  while (n != 0) {
		 d = (n  & (base-1));
		 *(c++) = d < 10 ? d + '0' : d - 10 + 'A';
		 n = (unsigned int) n >> bit;
	  }

   }

   c--;

   while (c >= t) *(f++) = *(c--);
     
   *f = '\0';
   return buff;

}


char * _ltoa(long n, char *buff, int base) 
{

   char t[100], *c=t, *f=buff;
   long d;
   char sign;
   int bit;

   if (base == 10) {
     if (n < 0) {
        *(f++) = '-';
        n = -n;
	 }

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
     else return NULL; // Base value unknown!

     while (n != 0) {
        d = (n  & (base-1));
        *(c++) = d < 10 ? d + '0' : d - 10 + 'A';
        n = (unsigned int) n >> bit;
	 }

   }

   c--;

   while (c >= t) *(f++) = *(c--);
     
   *f = '\0';
   return buff;
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
	short pHandle[10];

	if ((error = PROCESSHANDLE_NULLIT_ (pHandle)) != 0)
		return -1;

	if ((error = PROCESSHANDLE_GETMINE_(pHandle)) != 0)
		return -1;

	if ((error = PROCESSHANDLE_DECOMPOSE_ (pHandle
						, &cpuNumber
						, &processId)) != 0)
		return -1;


	return processId;
}



