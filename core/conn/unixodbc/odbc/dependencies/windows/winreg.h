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

#ifndef _WINREG_H_
#define _WINREG_H_

//
// Reserved Key Handles.
//

#define HKEY_CLASSES_ROOT           (( HKEY ) 0x80000000 )
#define HKEY_CURRENT_USER           (( HKEY ) 0x80000001 )
#define HKEY_LOCAL_MACHINE          (( HKEY ) 0x80000002 )
#define HKEY_USERS                  (( HKEY ) 0x80000003 )
#define HKEY_PERFORMANCE_DATA       (( HKEY ) 0x80000004 )
//#if(WINVER >= 0x0400)
#define HKEY_CURRENT_CONFIG         (( HKEY ) 0x80000005 )
#define HKEY_DYN_DATA               (( HKEY ) 0x80000006 )
//#endif

//
// Requested Key access mask type.
//

typedef ACCESS_MASK REGSAM;
#define HKEY	HANDLE
typedef HKEY *PHKEY;

LONG RegOpenKey (HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
LONG RegOpenKeyEx (HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired,
    PHKEY phkResult);
LONG RegCloseKey (HKEY hKey);
LONG RegQueryValueEx (HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType,
    LPBYTE lpData, LPDWORD lpcbData);

#endif