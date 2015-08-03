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
#if _MSC_VER >= 1100
#pragma once
#endif
#ifndef _NSK_commonhi_
#define _NSK_commonhi_
#ifndef DllImport
#define DllImport  __declspec(dllimport)
#endif
#ifdef DllExport
#undef DllExport
#define DllExport  __declspec(dllexport)
#endif

#ifndef TDM_NSKLIBAPI_GENERATOR
#define TDM_NSKLIBAPI_PREFIX  DllImport
#else
#define TDM_NSKLIBAPI_PREFIX  DllExport
#endif

// since SETSTOP, TOSVERSION ... etc  used to be defined in this file.
// so for those clients who used to include NSKcommonhi.h to get these
// definitions here we implicitly source in the definition for them.

#include "seaquest/sqtypes.h"

#include "rosetta/rosgen.h"
#define  ppctlz_h_including_section
#define  ppctlz_h_setstop
#include "guardian/ppctlz.h"
#include "guardian/pverz.h"





    TDM_NSKLIBAPI_PREFIX
    PVOID NSKMapCommon ( BOOL createmap, DWORD * size, HANDLE *common_memory_handle );


    TDM_NSKLIBAPI_PREFIX
    VOID NSKStopmodeToggle( DWORD stopmask );

    TDM_NSKLIBAPI_PREFIX
    VOID NSKHalt( DWORD exitcode );

    TDM_NSKLIBAPI_PREFIX
    VOID NSKTrace(DWORD tracelevel,DWORD dwEventId, char * szTypes, ...);
	
    TDM_NSKLIBAPI_PREFIX
    VOID NSKTrace(DWORD tracelevel,DWORD dwEventId);


	// This bug check is for SQL code..
	TDM_NSKLIBAPI_PREFIX
	VOID NSKBugCheck( PCHAR pmessage, DWORD value1 );

    TDM_NSKLIBAPI_PREFIX
    VOID NSKBugCheck(DWORD dwEventId, char * szTypes, ...);
	
    TDM_NSKLIBAPI_PREFIX
    VOID NSKBugCheck(DWORD dwEventId);
	
    TDM_NSKLIBAPI_PREFIX
    VOID NSKWarning(DWORD tracelevel, DWORD dwEventId, char * szTypes, ...);
	
    TDM_NSKLIBAPI_PREFIX
    VOID NSKWarning(DWORD tracelevel, DWORD dwEventId);
	

    TDM_NSKLIBAPI_PREFIX
    PCHAR NSKGetRegKeyServerWarePath ();

    TDM_NSKLIBAPI_PREFIX
    PCHAR NSKGetRegKeyConfigPath ();

    TDM_NSKLIBAPI_PREFIX
    PCSTR NSKNTNodeNumberToNodeName (DWORD i);

    TDM_NSKLIBAPI_PREFIX
    DWORD NSKNTNodeNameToNodeNumber (PCSTR nodename);

#define NSK_INVALID_NT_NODE_NAME  ( (DWORD)(-1) )

    TDM_NSKLIBAPI_PREFIX
    BOOL NSKPhoenixExist (DWORD pid);

    TDM_NSKLIBAPI_PREFIX
    VOID NSKPhoenixSetupShadow (DWORD UPpid, DWORD SPpid);

#endif
