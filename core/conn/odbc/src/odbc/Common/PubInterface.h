// ===============================================================================================
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
// ===============================================================================================
#ifndef PUB_INTERFACE_DEFINED
#define PUB_INTERFACE_DEFINED

#include <platform_ndcs.h>
#include <platform_utils.h>
#include "PubQueryStats.h"

#include <sqlcli.h>

#define MIN_INTERVAL 10

struct collect_info
{
    char					        clientId[MAX_COMPUTERNAME_LENGTH*4 + 1];
    char				            userName[MAX_USERNAME_LEN + 1];
    char				            clientUserName[MAX_SQL_IDENTIFIER_LEN + 1];
    char					        applicationId[MAX_APPLICATIONID_LENGTH*4 + 1];
    char						    nodeName[10];
    char							cpuPin[20];
	char							DSName[MAX_DSOURCE_NAME + 1];
    long							userId;
    short							startPriority;
    short							currentPriority;
    unsigned long					totalLoginTime;
    unsigned long					ldapLoginTime;
    unsigned long					sqlUserTime;
    unsigned long					searchConnectionTime;
    unsigned long					searchTime;
    unsigned long					authenticationConnectionTime;
    unsigned long					authenticationTime;
};
extern void UpdateStringText(string& textStr);

extern void SendEventMsg(
		  DWORD EventId
		, short EventLogType
		, DWORD Pid
		, char *ComponentName
		, char *ObjectRef
		, short nToken
		, ...);

extern void setCriticalDialout();

extern void setIsWms();

#ifdef NSK_ODBC_SRVR
extern int mxosrvr_init_seabed_trace_dll();
extern int mxosrvr_seabed_trace_close_dll();
#endif

#ifdef NSK_AS
extern int mxoas_init_seabed_trace_dll();
extern int mxoas_seabed_trace_close_dll();
#endif

#ifdef NSK_CFGSRVR
extern int mxocfg_init_seabed_trace_dll();
extern int mxocfg_seabed_trace_close_dll();
#endif
#endif
