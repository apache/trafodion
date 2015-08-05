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
//  SrvrMsg.cpp
#include <windows.h>

BOOL WINAPI DllMain (
		HINSTANCE	hInstDll,
		DWORD		fdwReason,
		LPVOID		fImpLoad)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		// the DLL is being mapped into the process' address space
		break;
	case DLL_THREAD_ATTACH:
		// A Thread is being created
		break;
	case DLL_THREAD_DETACH:
		// a thread is exiting cleanly
		break;
	case DLL_PROCESS_DETACH:
		// the DLL is being unmapped from the process's address space
		break;
	}
	return (TRUE);	// used only for DLL_PROCESS_ATTACH
}
