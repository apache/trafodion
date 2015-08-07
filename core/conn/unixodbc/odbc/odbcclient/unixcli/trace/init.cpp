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
/*
** init.c - This module contains the initialization functions
**
*/

#include "stdio.h"
#include "feerrors.h"
#include "stubtrace.h"
#include "odbcinst.h"
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <time.h>

#ifdef MXOSS
#include <spthread.h>
#else
#include <pthread.h>
#endif

#include <drvrglobal.h>

extern "C"
void T7971G10_01NOV2004_ABC_CLI_1220 (void){}

extern int hpodbc_dmanager;

#if defined (MXHPUXIA)
extern int unaligned_access_count;
extern "C" void allow_unaligned_data_access();
#endif

//int userStreams = 0;

typedef unsigned long*  (SQL_API *FPGetODBCSharedData) ();
extern FPGetODBCSharedData pGetODBCSharedData;

DWORD	dwGlobalTraceVariable=0;

#define DEFAULT_TRACE_FILE_SIZE "1024" // default trace file size in megabytes
#define MIN_TRACE_SIZE 1	// min allowed size for trace file in megabtyes
#define MAX_TRACE_SIZE 2000 // max allowed size for trace file in megabytes

long	maxTraceFileSize = 0;
char	szGlobalTraceFileName[256];
char	szCurrentTraceFileName[256];
DWORD	gTraceFlags = 0;

CRITICAL_SECTION2	g_csWrite;

CEnvironment g_Environment;

CEnvironment::CEnvironment()
{

#if defined (MXHPUXIA)
        allow_unaligned_data_access();
#endif

	cpu = 0;
	pin = 0;
	trace_pin = 0;
	nodenumber	=	0;
	nodename[0] =	'\0';
	volume[0]	=	'\0';
	subvolume[0]=	'\0';
	progname[0] =	'\0';
}

void CEnvironment::SetEnvironment()
{
	FILE *file;
	short error = 0;
	short len;
	short processhandle[10];
	char  msgfile_name[MAX_DSN_NAME_LEN];//EXT_FILENAME_LEN is not enough for "/etc/hpodbc/MXODSN" 
	char *ptr_msgfile_name;
	char  object[EXT_FILENAME_LEN+1];
	char tmpTraceFileName[256];
	char tmpTraceFileSize[6];
	long tmpSize;
	char    tmpTraceTime[256];

	short nodename_length;

	szGlobalTraceFileName[0] = 0;
	szCurrentTraceFileName[0] = 0;
	tmpTraceFileName[0] = 0;
	tmpTraceFileSize[0] = 0;

	CHAR*	szTraceStartKey=			"TraceStart";
	CHAR*	szDefaultTraceStart=  		"0";

	CHAR*	szTraceFileNameKey=			"TraceFile";
	CHAR	szDefaultTraceFile[17];

	CHAR*	szTraceFlagsKey=			"TraceFlags";
	CHAR*	szDefaultTraceFlags=  		"0";

	CHAR*	szODBC=						"ODBC";
	CHAR*	szODBCIni=					"TRAFDSN";
	char	szTraceWork[100];
	CHAR*	tmpEnvValue;

	UWORD   wSaveConfigMode;
	UWORD   wConfigMode;

	if (hpodbc_dmanager)
	{
		memset(msgfile_name, '\0', MAX_DSN_NAME_LEN);
		if (( tmpEnvValue = getenv("TRAFDSN")) != NULL && tmpEnvValue[0] != 0)
		{
		    sprintf(msgfile_name,tmpEnvValue);
		}
		else
		    goto defdsn;
		file = fopen (msgfile_name, "r");
		// not a valid path
		if (file == NULL)
		{
		    goto defdsn;
		}
		else
		{
		    fclose(file);
		    wConfigMode = ODBC_USER_DSN;  // not sure about this.
		    goto dsnend;
		}
	defdsn:
		sprintf(msgfile_name,"./TRAFDSN");
		file = fopen (msgfile_name, "r");
		// unable to open CWD MXODSN file
		if (file == NULL)
		{
			wConfigMode = ODBC_SYSTEM_DSN;
			strcpy(msgfile_name,SYSTEM_DSNFILE);
		}
		else
		{
			fclose(file);
			wConfigMode = ODBC_USER_DSN;
			strcpy(msgfile_name,DSNFILE);
		}
	dsnend:
		ptr_msgfile_name = msgfile_name;

		if ((tmpEnvValue = getenv("HPODBC_TRACE_LEVEL")) != NULL)
		{
			dwGlobalTraceVariable = 1;
			strncpy(szTraceWork, tmpEnvValue, sizeof(szTraceWork));
		}
		else
		{
		// what we need here is to open a file based on misc information	
		// for now we'll just open a file in the CWD with a default name
		(gDrvrGlobal.fpSQLGetPrivateProfileString)(szODBC,
			szTraceStartKey,
			szDefaultTraceStart,
			szTraceWork,
			sizeof(szTraceWork),
			ptr_msgfile_name);

		dwGlobalTraceVariable = atol(szTraceWork);

   		(gDrvrGlobal.fpSQLGetPrivateProfileString)(szODBC,
			szTraceFlagsKey,
			szDefaultTraceFlags,
			szTraceWork,
			sizeof(szTraceWork),
			ptr_msgfile_name);
	}
	}
	// we can check the environment variables for tracing
	else
	{
		if ((tmpEnvValue = getenv("HPODBC_TRACE_LEVEL")) != NULL)
		{
			dwGlobalTraceVariable = 1;
			strncpy(szTraceWork, tmpEnvValue, sizeof(szTraceWork));
		}
		else
			strncpy(szTraceWork, szDefaultTraceFlags, sizeof(szTraceWork));
	}

	if (strcasecmp(szTraceWork,"ERROR") == 0)
		gTraceFlags = TR_ODBC_ERROR;
	else if (strcasecmp(szTraceWork,"WARNING") == 0)
        gTraceFlags = TR_ODBC_WARN | TR_ODBC_ERROR;
    else if (strcasecmp(szTraceWork,"CONFIG") == 0)
        gTraceFlags = TR_ODBC_WARN | TR_ODBC_ERROR | TR_ODBC_CONFIG;
    else if (strcasecmp(szTraceWork,"INFO") == 0)
        gTraceFlags = TR_ODBC_WARN | TR_ODBC_ERROR | TR_ODBC_CONFIG | TR_ODBC_INFO;
    else if (strcasecmp(szTraceWork,"TRANSPORT") == 0)
        gTraceFlags = TR_ODBC_WARN | TR_ODBC_ERROR | TR_ODBC_CONFIG | TR_ODBC_INFO | TR_ODBC_TRANSPORT;
    else if (strcasecmp(szTraceWork,"DEBUG") == 0)
        gTraceFlags = TR_ODBC_WARN | TR_ODBC_ERROR | TR_ODBC_CONFIG | TR_ODBC_INFO | TR_ODBC_DEBUG;
	else
		gTraceFlags = 0;

#ifdef DEBUG
	if( (tmpEnvValue = getenv("HPODBC_TRACE_BITFLAGS")) != NULL)
	{
		long tmpEnvLongVal = atol(tmpEnvValue);
		if( TR_ODBC_RANGE_MIN < tmpEnvLongVal && tmpEnvLongVal < TR_ODBC_RANGE_MAX )
		{
			dwGlobalTraceVariable = 1;
			gTraceFlags = tmpEnvLongVal;
		}
	}
#endif
    sprintf(tmpTraceFileName, "./tracefile.log");

	// if using hpodbc driver manager read trace value from MXODSN
	if (hpodbc_dmanager)
	{
		if ((tmpEnvValue = getenv("HPODBC_TRACEFILE_NAME")) != NULL)
			strncpy(szGlobalTraceFileName, tmpEnvValue, sizeof(szGlobalTraceFileName));
		else
		{

// fix this to deal with tracefile looping as well as the default name!
			len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(szODBC, 
			"TraceFile", 
			tmpTraceFileName,
			szGlobalTraceFileName, 
			sizeof(szGlobalTraceFileName), 
			ptr_msgfile_name);
			if (len == 0)
			   strcpy(szGlobalTraceFileName,tmpTraceFileName);
		}
	}
	// read trace value from environment variable
	else
	{
		if ((tmpEnvValue = getenv("HPODBC_TRACEFILE_NAME")) != NULL)
			strncpy(szGlobalTraceFileName, tmpEnvValue, sizeof(szGlobalTraceFileName));
		else
			strncpy(szGlobalTraceFileName, tmpTraceFileName, sizeof(szGlobalTraceFileName));
	}

	// if the global trace variable is set then create the tracelog file
	if (dwGlobalTraceVariable)
	{
		sprintf(tmpTraceTime,".%d_%ld", getpid(), time(NULL));
		strcpy(szCurrentTraceFileName, szGlobalTraceFileName);
		strncat(szCurrentTraceFileName, tmpTraceTime, 256);
		file = fopen (szCurrentTraceFileName, "w+");	

		// if we are unable to create the log file then set the global trace
		// variable to 0.
		if (file == NULL)
			dwGlobalTraceVariable = 0;
	}

	if (hpodbc_dmanager)
	{
		if ((tmpEnvValue = getenv("HPODBC_TRACEFILE_SIZE")) != NULL)
			strncpy(tmpTraceFileSize, tmpEnvValue, sizeof(tmpTraceFileSize));
		else
		{
		// setup initial value for max tracefile size
		len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(szODBC, 
			"MaxTraceFileSize", 
			DEFAULT_TRACE_FILE_SIZE,
			tmpTraceFileSize, 
			sizeof(tmpTraceFileSize), 
			ptr_msgfile_name);
	
			if(len ==0)
			   strcpy(tmpTraceFileSize,DEFAULT_TRACE_FILE_SIZE);
		}
	}
	else
	{
		if ((tmpEnvValue = getenv("HPODBC_TRACEFILE_SIZE")) != NULL)
			strncpy(tmpTraceFileSize, tmpEnvValue, sizeof(tmpTraceFileSize));
		else
			strncpy(tmpTraceFileSize, DEFAULT_TRACE_FILE_SIZE, sizeof(tmpTraceFileSize));
	}

	tmpSize = (atol((const char*)tmpTraceFileSize));

	if ((tmpSize >= MIN_TRACE_SIZE) && (tmpSize <= MAX_TRACE_SIZE))
		maxTraceFileSize = tmpSize;
	else
		maxTraceFileSize = (atol(DEFAULT_TRACE_FILE_SIZE));
	

	trace_pin = (short)getpid();
}
CEnvironment::~CEnvironment()
{
}



