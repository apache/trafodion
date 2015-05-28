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
#ifndef _COMMONFUNCTION_DEFINED
#define _COMMONFUNCTION_DEFINED

#include "cee.h"
#include "ceecfg.h"
#include "Global.h"
#include <seabed/ms.h>


extern CEECFG_Transport getTransport(const IDL_Object srvrObjRef);
extern long getPortNumber(const IDL_Object srvrObjRef);
extern BOOL getInitParam(int argc, char *argv[], SRVR_INIT_PARAM_Def &initParam, char* strName, char* strValue);
extern long ping( char* IPAddress );
extern BOOL checkProcess( char* strProcessName );
extern bool checkProcessHandle(NSK_PROCESS_HANDLE processHandle);

extern void insertIpAddressAndHostNameInObjRef(char* srvrObjRef, char* IpAddress, char* HostName);
extern void insertInObjRef(char* srvrObjRef, char* IpAddress, char* HostName, char* SegmentName, TCP_ADDRESS tcp_address, char* process_name );
extern void removeIpAddressAndHostNameFromObjRef(char* srvrObjRef, char* IpAddress, char* HostName);

extern short Get_Sysinfo_( void );
extern BOOL isNeoHW();
extern BOOL isNeoBlades();

extern void terminateThreads(int exitcode);
extern bool RegisterProcessDeathMessage(char *processName);

extern bool isUTF8(const char *str);
extern char* strcpyUTF8(char *dest, const char *src, size_t destSize, size_t copySize=0);

void escSingleQuotes(string &str);
#endif
