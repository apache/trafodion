/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
********************************************************************/
/**************************************************************************
**************************************************************************/

#include <platform_ndcs.h>
#include <platform_utils.h>
#include <stdio.h>
#include "EventMsgs.h"
#include "Global.h"

#define EVENTMSG_ERROR_MSGBOX_TITLE "ODBC/MX Event Message Handling Error"

// These defines are extracted from regvalues.h in order to eliminate
// unnecessary dependencies.
#define MAX_REGKEY_PATH_LEN     1000
#define MAX_SQL_ID_LEN          128
#define MAX_NODE_CNT            16
#define DEFAULTNODECOUNT        -1


// strings used to build RegValues lookup keys
#define _CLUSTER_NAME_VAL_NM    "Cluster-Name"
#define _BOOT_CONFIG_VAL_NM     "Boot-Config"
#define _CLUSTER_KEY            "CLUSTER"
#define _NODE_KEY               "NODE"
#define _NETBIOS_VAL_NM         "NetBiosName"
bool consoleOn = false;


#ifdef NSK_ODBC_SRVR
// mxosrvr related trace variables
const char *gp_mxosrvr_trace_filename         = "mxosrvr_trace_";

const char *gp_mxosrvr_env_trace_enable       = "MXOSRVR_TRACE_ENABLE";  // enable trace
bool        gv_mxosrvr_trace_enable = false;

const char *gp_mxosrvr_env_trace_ems          = "MXOSRVR_TRACE_EMS";     // trace what used to be sent to the legacy ems collector
bool        gv_mxosrvr_trace_ems = false;

const char *gp_mxosrvr_env_trace_legacy       = "MXOSRVR_TRACE_LEGACY";
bool        gv_mxosrvr_trace_legacy = false;

int mxosrvr_init_seabed_trace()
{
   msg_getenv_bool(gp_mxosrvr_env_trace_enable, &gv_mxosrvr_trace_enable);

   if(gv_mxosrvr_trace_enable)
   {
      msg_getenv_bool(gp_mxosrvr_env_trace_ems, &gv_mxosrvr_trace_ems);
      msg_getenv_bool(gp_mxosrvr_env_trace_legacy, &gv_mxosrvr_trace_legacy);

      trace_init((char*)gp_mxosrvr_trace_filename,
                  true,
                  NULL,
                  false);
   }

   return 0;
}

int mxosrvr_seabed_trace_close()
{
   trace_close();
}

#endif

#ifdef NSK_AS
// mxoas related trace variables
const char *gp_mxoas_trace_filename         = "mxoas_trace_";

const char *gp_mxoas_env_trace_enable       = "MXOAS_TRACE_ENABLE";  // enable trace
bool        gv_mxoas_trace_enable = false;

const char *gp_mxoas_env_trace_ems          = "MXOAS_TRACE_EMS";     // trace what used to be sent to the legacy ems collector
bool        gv_mxoas_trace_ems = false;

int mxoas_init_seabed_trace()
{
   msg_getenv_bool(gp_mxoas_env_trace_enable, &gv_mxoas_trace_enable);

   if(gv_mxoas_trace_enable)
   {
      msg_getenv_bool(gp_mxoas_env_trace_ems, &gv_mxoas_trace_ems);

      trace_init((char*)gp_mxoas_trace_filename,
                  true,
                  NULL,
                  false);
   }

   return 0;
}

int mxoas_seabed_trace_close()
{
   trace_close();
}

#endif

#ifdef NSK_CFGSRVR
// mxocfg related trace variables
const char *gp_mxocfg_trace_filename         = "mxocfg_trace_";

const char *gp_mxocfg_env_trace_enable       = "MXOCFG_TRACE_ENABLE";    // enable trace
bool        gv_mxocfg_trace_enable = false;

const char *gp_mxocfg_env_trace_ems          = "MXOCFG_TRACE_EMS";       // trace what used to be sent to the legacy ems collector
bool        gv_mxocfg_trace_ems = false;

int mxocfg_init_seabed_trace()
{
   msg_getenv_bool(gp_mxocfg_env_trace_enable, &gv_mxocfg_trace_enable);

   if(gv_mxocfg_trace_enable)
   {
      msg_getenv_bool(gp_mxocfg_env_trace_ems, &gv_mxocfg_trace_ems);

      trace_init((char*)gp_mxocfg_trace_filename,
                  true,
                  NULL,
                  false);
   }

   return 0;
}

int mxocfg_seabed_trace_close()
{
   trace_close();
}

#endif




//long FindClusterName(char *);
//int FindMyNodeNum ( void );


//=========== no wait ==============

static bool segment_error;
static long segment_size;
static short segment_id;
static long ext_address;
static short* pool;
static long pool_size;

static unsigned long seqcount = 0;

ODBCMXEventMsg::ODBCMXEventMsg()
    : optionalTokens(false)
      ,isWms(false)
      ,collectorType(COLL_EMS)
{
    SRVRTRACE_ENTER(FILE_EMS+1);
    PVOID lpMsgBuf;
    hModule = NULL;
    sendEventMsgPtr = NULL;
    ClusterName[0] = '\0';
    NodeId = -1;
    process_name[0] = '\0';
        nid = -1;
    pid = -1;
    SRVRTRACE_EXIT(FILE_EMS+1);
}

ODBCMXEventMsg::~ODBCMXEventMsg()
{
}

//////////////////////////////////////////////////////////////////////////
//
// Name:    SendEventMsg
//
// Purpose: Add a specified type of parameter to an event.
//          This logs a message with (any) token parameters previously
//          specified by the Add* methods.
//
// Note:    Parameters in event messages are specified in the message
//          template by using the form "%n", where "n" indicates the
//          relative call to AddToken which supplies the value for the
//          parameter.  Thus, the first AddToken call will provide the
//          value for token %1, the second for token %2, etc.
//          The maximum number of tokens is defined in teventlog.h.
//          Token is a null-terminated string that becomes the parameter
//          value.
//
// Input:   EventId - numeric value for the event.  This corresponds
//                    to the value defined by the message compiler for
//                    the event.
//          EventLogType - The type of the event being logged
//                         (e.g., EVENTLOG_INFORMATION_TYPE,
//                         EVENTLOG_WARNING_TYPE, and
//                         EVENTLOG_ERROR_TYPE)
//          Pid - Process ID
//          ComponentName - ODBC/MX Component (e.g., Association Server)
//          ObjectRef - Object Reference
//          nToken - Number of additional tokens
//
// Output:  None
//
//////////////////////////////////////////////////////////////////////////

short ODBCMXEventMsg::SendEventMsg(DWORD EventId, short EventLogType, DWORD Pid, char *ComponentName,
            char *ObjectRef, short nToken, ...)
{
    SRVRTRACE_ENTER(FILE_EMS+2);
    va_list marker;

    va_start( marker, nToken);
    short error = 0;


   send_to_eventlog(EventId & 0x0FFFF, EventLogType, ComponentName, ObjectRef, nToken, marker);

    SRVRTRACE_EXIT(FILE_EMS+2);

    return error;
}

//////////////////////////////////////////////////////////////////////////
//
// Name:    FindClusterName
//
// Purpose: Get the cluster name from the registry.
//
//////////////////////////////////////////////////////////////////////////
#ifdef __OBSOLETE
long FindClusterName(char *ClusterName)
{
    SRVRTRACE_ENTER(FILE_EMS+3);
    short errorCode;
    char  sysNm[8];
    short sysNmLen;
   TPT_DECL(processHandle);

   PROCESSHANDLE_NULLIT_(TPT_REF(processHandle));
    ClusterName[0] = '\0';

    PROCESSHANDLE_GETMINE_(TPT_REF(processHandle));
    errorCode = PROCESSHANDLE_DECOMPOSE_(TPT_REF(processHandle)
                                       ,OMITREF /* cpu */
                                       ,OMITREF /* pin */
                                       ,OMITREF /* nodenumber */
                                       ,sysNm
                                       ,(short)sizeof(sysNm)
                                       ,&sysNmLen);
    if (errorCode == FEOK)
    {
        memcpy(ClusterName, sysNm, sysNmLen);
        ClusterName[sysNmLen] = '\0';
    }
    else
    {
        strcpy(ClusterName, "UNKNOWN");
      return errorCode;
    }

    return ERROR_SUCCESS;

    SRVRTRACE_EXIT(FILE_EMS+3);
}
#endif /* #ifdef __OBSOLETE */

//////////////////////////////////////////////////////////////////////////
//
// Name:    ReadNodeNumInRegistry
//
// Purpose: Use the local computer name and search in the registry to
//          find its node number.
//
// Note:    The default node ID is -1 which will be shown as
//              Node ID: Unknown
//          in the event message.
//
//////////////////////////////////////////////////////////////////////////

int ReadNodeNumInRegistry(char *LocalComputerName)
{
    SRVRTRACE_ENTER(FILE_EMS+4);

    int     NodeCount = DEFAULTNODECOUNT;

   int nid, pid, error;
   char myName[50];
   MS_Mon_Node_Info_Type nodeInfo;

   error = msg_mon_get_process_info(NULL, &nid, &pid);

   if (error != XZFIL_ERR_OK )
       NodeCount = -1;
   else
       NodeCount = nid;;

    SRVRTRACE_EXIT(FILE_EMS+4);
    return NodeCount;
}

#ifdef __OBSOLETE
//////////////////////////////////////////////////////////////////////////
//
// Name:    FindMyNodeNum
//
// Purpose: Get the local computer name and search in the registry to
//          find its node number.
//
//////////////////////////////////////////////////////////////////////////

int FindMyNodeNum ( void )
{
    SRVRTRACE_ENTER(FILE_EMS+5);
    char    LocalComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    unsigned long   dwMyNodeNmLen = MAX_COMPUTERNAME_LENGTH ;


    LocalComputerName[0] = '\0';
    GetComputerName(LocalComputerName, &dwMyNodeNmLen);

    SRVRTRACE_EXIT(FILE_EMS+5);
    return ReadNodeNumInRegistry(LocalComputerName);
}
#endif /* #ifndef __OBSOLETE */


/*----------------------------------------------------------------------*/
/* open_ems_name                                                  */
/*----------------------------------------------------------------------*/
short ODBCMXEventMsg::open_ems_name( char* collector)
{
    short error = 0;
    SRVRTRACE_ENTER(FILE_EMS+6);
    if (collector != NULL)
    {
        strncpy(ems_name, collector, sizeof(ems_name));
        ems_name[sizeof(ems_name)-1] = 0;
    }
    else
        bzero(ems_name, sizeof(ems_name));
    SRVRTRACE_EXIT(FILE_EMS+6);
    return error;
}

char* ODBCMXEventMsg::get_ems_name( void )
{
    return ems_name;
}

// ODBCMXEventMsg::send_to_eventlog is defined in linux/EventMsgs_ps.cpp

bool isUTF8(const char *str)
{
    char c;
    unsigned short byte = 1;
    size_t len = strlen(str);

    for (size_t i=0; i<len; i++)
    {
        c = str[i];

        if (c >= 0x00 && c < 0x80 && byte == 1) // ascii
            continue;
        else if (c >= 0x80 && c < 0xc0 && byte > 1) // second, third, or fourth byte of a multi-byte sequence
            byte--;
        else if (c == 0xc0 || c == 0xc1) // overlong encoding
            return false;
        else if (c >= 0xc2 && c < 0xe0 && byte == 1) // start of 2-byte sequence
            byte = 2;
        else if (c >= 0xe0 && c < 0xf0 && byte == 1) // start of 3-byte sequence
            byte = 3;
        else if (c >= 0xf0 && c < 0xf5 && byte == 1) // start of 4-byte sequence
            byte = 4;
        else
            return false;
    }
    return true;
}

// copies src to dest
// if dest is not big enough, src is truncated up to size of dest
// when truncating UTF8 string, it will not truncate in the middle of multi-byte sequence
// always null-terminated

char* strcpyUTF8(char *dest, const char *src, size_t destSize, size_t copySize)
{
    char c;
    size_t len;

    if (copySize == 0)
        len = strlen(src);
    else
        len = copySize;

    if (len >= destSize)
        len = destSize-1; // truncation

    while (len > 0)
    {
        c = src[len-1];
        if (c < 0x80 || c > 0xbf)
            break;
        len--; // in second, third, or fourth byte of a multi-byte sequence
    }
    strncpy((char*)dest, (const char*)src, len);
    dest[len] = 0;

    return dest;
}
