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

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the DSNCONVERTER_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// DSNCONVERTER_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef DSNCONVERTER_EXPORTS
#define DSNCONVERTER_API __declspec(dllexport)
#else
#define DSNCONVERTER_API __declspec(dllimport)
#endif

DSNCONVERTER_API BOOL IsDriverInstalled(const char* DrvName, char* Message);
DSNCONVERTER_API BOOL FindADSNbyVersion(const char* DrvName, char* Message);
DSNCONVERTER_API BOOL FindAllDSNbyVersion(const char* DrvName, char* Message);
DSNCONVERTER_API BOOL MigrateDS(const char* oldDrvName, const char* newDrvName,
								char* Message);
DSNCONVERTER_API BOOL MigrateAllDS(const char* DrvName, char* Message);
