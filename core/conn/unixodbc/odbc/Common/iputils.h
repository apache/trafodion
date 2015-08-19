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
//
// MODULE: IPUtils.h
#ifndef _IPUTILS_H_DEFINED
#define _IPUTILS_H_DEFINED

#ifdef _AFXDLL
#include <stdafx.h>
#else
#include <windows.h>
#endif
#include "regvalues.h"

typedef unsigned short USHORT;
//typedef unsigned long  ULONG;
//typedef signed long    DWORD;

extern "C"
BOOL PingIpAddress(char * PingIp, DWORD Timeout);

extern "C" 
BOOL PingIpAddressBin(ULONG PingIp, DWORD Timeout);

extern "C" 
DWORD NameToIPs(char * PingIpName, ULONG * IPs);

extern "C" 
BOOL PingIPs(ULONG * IPs, DWORD Timeout);

extern "C" 
USHORT Checksum(USHORT *buffer, int size);

extern "C" 
int CheckIPAddress(char *pzInAddr);

extern "C"
DWORD MapNetBiosNm2IpAddress ( char*	pNetBiosName, char *pIpAddress); 
#endif
