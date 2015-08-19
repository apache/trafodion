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
#ifndef NDCS_PLATFORM_UTILS_H_
#define NDCS_PLATFORM_UTILS_H_

#include "ndcs_defs.h"
#include <Global.h>
#include <odbcsrvrcommon.h> 

// Function prototype for initialization routine in platform_utils.cpp
void platformSpecificInit(int &argc, char *argv[]);

BOOL GetComputerName (LPSTR lpBuffer, LPDWORD nSize);

DWORD	GetCurrentProcessId(void);

short Get_Sysinfo_( void );

BOOL isNeoHW();

int ndcsStopProcess(TPT_PTR(phandle), int attempts);
bool processExists(char *processName, TPT_PTR(pHandleAddr) );
short getProcessHandle(char *processName, TPT_PTR(pHandleAddr) );

BOOL checkProcess( char* strProcessName );
bool getProcessName(char *processName, TPT_PTR(pHandleAddr), int maxLen);
bool checkProcessHandle(TPT_DECL(processHandle));

#endif
