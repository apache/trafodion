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
#include <platform_ndcs.h>
#define _GUARDIAN_SOCKETS
#include <netdb.h>
#include <platform_utils.h>
#include <odbcCommon.h>
#include <odbcsrvrcommon.h>
#include <cextdecs.h>
#include "errno.h"
#include "tdm_odbcSrvrMsg.h"
#include "commonFunctions.h"
//#include "Interface/Transport.h"

//extern char TfdsStrBuf1[];  //unresolved external, replacing w/tmpBuf

//extern CTransport GTransport;
//extern SRVR_GLOBAL_Def *srvrGlobal;

/***************************
 typedef struct _ip_address
 {
 	union
 	{
 		unsigned long ipaddr;
 		unsigned char ip[4];
 	} u;
 } ip_address;
***************************/



void platformSpecificInit(int &argc, char *argv[])
{
//*** need body ***
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

typedef _tal _callable _extensible int64 ( *getsysinfo ) ( void );

short Get_Sysinfo_( void )
{
	getsysinfo function;
	
	/*------------------------------------------------------------------*/
	/* Get the address to the function. Determine whether the OS        */
	/* supports this call.                                              */
	/*------------------------------------------------------------------*/
	function =
		(getsysinfo ) SYSTEMENTRYPOINT_RISC_
											( "SYSTEMTYPEINFO_"
											, (int16) strlen( "SYSTEMTYPEINFO_" )
											) ;

	if (0 == function) return ZNEO_VAL_PUSE_UNDEFINED;

	zneo_ddl_systype_info_def sysinfo;
	sysinfo.u_zsysinfo.zsysinfo = function();

	return sysinfo.u_zsysinfo.zinfo.zplatformusage;

} /* of Get_Sysinfo _ */


BOOL isNeoHW()
{
	static BOOL init = TRUE;
	static BOOL neoHW = FALSE;

	if( init ) {
		if( Get_Sysinfo_() == ZNEO_VAL_PUSE_NEO )
			neoHW = TRUE;
		init = FALSE;
	}

	return neoHW;
}



// Given a process handle, stop a process.  If PROCESS_STOP_ fails
// keep trying up to "attempts" times.
//int ndcsStopProcess(NSK_PROCESS_HANDLE& phandle, int attempts)
int ndcsStopProcess(NSK_PROCESS_HANDLE phandle, int attempts)
{

    int kill_attempt = 1;
    int nskError;


    while (kill_attempt <= attempts)
    {
        //nskError = PROCESS_STOP_((short *) &phandle,
        nskError = PROCESS_STOP_(phandle,
								 OMITSHORT,	  // specifier 
								 OMITSHORT,   // options
								 OMITSHORT,	  // Completion code 
								 OMITSHORT,   // Termination info 
								 OMITREF,     // Subsystem Id 
								 OMITREF,OMITSHORT); // Text and length 

        if ((nskError == 638) || (nskError == 639))
        {
            kill_attempt++;
            DELAY(DELAY_KILL_ATTEMPT);
            continue;
        }
        else
            break;
    }

    return nskError;
}


BOOL checkProcess( char* strProcessName )
{
	NSK_PROCESS_HANDLE processHandle;
	short retcode;

	if( strProcessName == NULL ) return FALSE;

	PROCESSHANDLE_NULLIT_( processHandle ); 

	retcode = FILENAME_TO_PROCESSHANDLE_( strProcessName, 
								strlen( strProcessName),
								 processHandle );

	if( retcode != 0 ) return FALSE;
	
	return checkProcessHandle(processHandle);
}


// Check whether or not the given process exists.
// NSK implementation uses processHandle not processName.
bool processExists(char *processName, short *processHandle)
{
	int error;
	short errorDetail;
	short proctype;

	error = PROCESS_GETINFO_(processHandle,
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
			&proctype,						// Process type 
			OMITREF);						// OSS or NT process Id

    return (error == 0);
}

// Convert a process name to a process handle
short getProcessHandle(char *processName, short *pHandleAddr)
{
    short error;

    error = FILENAME_TO_PROCESSHANDLE_(processName,
                                       strlen(processName),
                                       pHandleAddr);
    return error;
      
}


bool checkProcessHandle(NSK_PROCESS_HANDLE processHandle)
{
	short errorDetail;
	short proctype;
	short retcode;

	retcode = PROCESS_GETINFO_(processHandle,
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
			&proctype,						// Process type 
			OMITREF);						// OSS or NT process Id

	if( retcode != 0 ) return FALSE;
	
	return TRUE;
}

