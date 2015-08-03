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
#include <platform_ndcs.h>
#include <sys/utsname.h>
#include <seabed/ms.h>
#include <platform_utils.h>

char gErrText[1024];

void platformSpecificInit(int &argc, char *argv[])
{
   // Initialize seabed
   file_init(&argc, &argv);
   file_mon_process_startup(true);
   msg_mon_enable_mon_messages(true); 
}

BOOL GetComputerName (LPSTR lpBuffer, LPDWORD nSize)
{
   // Get name of the current system
   struct utsname name;
   int result;

   result = uname(&name);
   if (result >= 0)
   {
      // Copy node name to caller's buffer but don't overrun caller's buffer
      size_t nodenameLen = strlen(name.nodename);
      if (*nSize < nodenameLen) nodenameLen = *nSize;
      strncpy(lpBuffer, name.nodename, nodenameLen);
      lpBuffer[nodenameLen] = 0;

      // Set caller's variable to length of string
      *nSize = nodenameLen;
      return TRUE;
   }
   else
   {   // uname was unsuccessful
      *nSize = 0;
      return FALSE;
   }
}

DWORD	GetCurrentProcessId()
{
   return getpid();
}

short Get_Sysinfo_( void )
{
   return 0;
}

BOOL isNeoHW()
{
   return TRUE;
}


int ndcsStopProcess(TPT_PTR(phandle), int attempts)
{


   int kill_attempt = 1;
   int pid;
   //short pid;
   short processNameLen;
   int nodeNum;
   char processName[256];
   int error;


	//changes related to new phandle
    error = PROCESSHANDLE_DECOMPOSE_( phandle,
      &nodeNum,
      &pid,
      OMITREF,
      OMITREF,
      OMITSHORT,
      OMITREF,
      processName,
      sizeof( processName)-1,
      &processNameLen,
      NULL);
   processName[processNameLen] = 0;

   if (!error)
   {
      while (kill_attempt <= attempts)
      {
         /*********************************************************************
             msg_mon_stop_process() stops the process with name, nid, and pid.
                XZFIL_ERR_OK - Success
                XZFIL_ERR_INVALIDSTATE - Not initialized
         *********************************************************************/
         error = msg_mon_stop_process(processName, nodeNum, pid);
         if (error == XZFIL_ERR_INVALIDSTATE ) 
         {
            kill_attempt++;
            DELAY(DELAY_KILL_ATTEMPT);
            continue;
         }
         else
            break;
      }
   }

   return error;
}


// Check whether or not the given process exists.
BOOL checkProcess( char* processName )
{
   BOOL b1=FALSE;

   // Temporary measure: indicate specific process exists.
   if (strcmp("$ZTC0", processName) == 0) return TRUE;

   b1 = (processExists(processName, NULL)) ? (BOOL) TRUE : (BOOL) FALSE;
   return  b1;
}

// Check whether or not the given process exists.
// Linux implementation uses processName not processHandle.
bool processExists(char *processName, TPT_PTR(pHandleAddr))
{
	int nid;
	int pid;
	int result;
	char procname[MS_MON_MAX_PROCESS_NAME];

   //PROCESSHANDLE_NULLIT_(pHandleAddr);

   if (processName != NULL)
   {

      // Get information about the requested process
      result = msg_mon_get_process_info(processName, &nid, &pid);
      
	  if( result != XZFIL_ERR_OK )
	  {
		memset( gErrText, '\x0', sizeof(gErrText));
		sprintf(gErrText, "msg_mon_get_process_info() for process %s failed with error %d", processName, result);
      }      

      // Return true if process exists, false otherwise
      return result == XZFIL_ERR_OK && nid != -1 && pid != -1;
   }
   else
      if (pHandleAddr != NULL)
      {

		result = PROCESSHANDLE_DECOMPOSE_( pHandleAddr,
			 &nid,			//[ *cpu ]
			 &pid,			//[ *pin ]
             OMITREF,
             OMITREF,
             OMITSHORT,
             OMITREF,
             OMITREF, 
             OMITSHORT,
             OMITREF,
             OMITREF);

		if (result != FEOK)
			return FALSE;

		result =msg_mon_get_process_name(nid, pid, procname);
 		return result == XZFIL_ERR_OK && nid != -1 && pid != -1;
     }
      else
         return FALSE;
      
}


short getProcessHandle(char *processName, TPT_PTR(pHandleAddr))
{
   FILENAME_TO_PROCESSHANDLE_(processName, strlen(processName), pHandleAddr);
   return 0;
}

bool getProcessName(char *processName, SB_Phandle_Type* pHandleAddr, int maxLen)
{
   short actualLen=0;
   int result;

   if (pHandleAddr != NULL)
   {
      result = PROCESSHANDLE_DECOMPOSE_( pHandleAddr,
          OMITREF,
          OMITREF,
          OMITREF,
          OMITREF,
          OMITSHORT,
          OMITREF,
          processName,        //procname
          maxLen,          //procname_maxlen
          &actualLen,         //procname_length
          OMITREF);

      processName[actualLen] = '\0';

      return (result == FEOK);
   }
   else
      return FALSE;

}

bool checkProcessHandle(SB_Phandle_Type processHandle)
{

#ifdef ___wms_port_inprogress
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
#endif
	
	return TRUE;
}
