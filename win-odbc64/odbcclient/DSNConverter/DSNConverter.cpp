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
// DSNConverter.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "DSNConverter.h"
#include "OdbcDS.h"

const int g_verNameLen = 25;
const char g_versions[][g_verNameLen] = {"NonStop ODBC/MX",
									 "NonStop ODBC/MX 1.8",
									 "NonStop ODBC/MX 2.0"};
const int g_versionCnt = sizeof(g_versions) / g_verNameLen;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

// IsDriverInstalled
//
// Description:
//   Checks if a particular driver is installed.
//
// Params:
//   DrvName: Name of driver to find.
//   Message: Pointer to a previously allocated buffer. No boundary checks are
//            performed on this buffer.
//
//  Returns:
//   TRUE is at least one data source was found. FALSE is returned in an error
//   occurs with Message containing error information.
BOOL IsDriverInstalled(const char* DrvName, char* Message)
{
	if (DrvName == NULL || Message == NULL)
		return FALSE;

	COdbcDS OdbcDS;
	return OdbcDS.IsDrvrInstalled(DrvName, Message);
}

// FindADSNbyVersion
//
// Description:
//   Checks if any data sources are associated with a particular driver.
//
// Params:
//   DrvName: Name of the driver associated with any existing data sources.
//   Message: Pointer to a previously allocated buffer. No boundary checks are
//            performed on this buffer.
//
// Returns:
//   TRUE is at least one data source was found. FALSE is returned in an error
//   occurs with Message containing error information.
BOOL FindADSNbyVersion(const char* DrvName, char* Message)
{
	if (DrvName == NULL || Message == NULL)
		return FALSE;

	COdbcDS OdbcDS;

	return OdbcDS.FindDSNByVersion(DrvName, Message);
}

// FindAllDSNbyVersion
//
// Description:
//   Checks if any data sources are associated with a previous version of an
//   ODBC/MX driver.
//
// Params:
//   DrvName: Name of driver of which previous versions will be found.
//   Message: Pointer to a previously allocated buffer. No boundary checks are
//            performed on this buffer.
//
// Returns:
//   TRUE is at least one data source was found. FALSE is returned if the
//   driver name specified in DrvName is not found in the known list of
//   driver versions. If an error occurs, Message will contain error
//   information.
BOOL FindAllDSNbyVersion(const char* DrvName, char* Message)
{
	if (DrvName == NULL || Message == NULL)
		return FALSE;

	COdbcDS OdbcDS;
	BOOL bFound = FALSE;
	int verIndex = -1;

	// Find the driver name in the list
	for (int i = 0; i < g_versionCnt && verIndex == -1; i++)
	{
		if (strcmp(g_versions[i], DrvName) == 0)
			verIndex = i;
	}

	// Check to see if the driver version was not found.
	if (verIndex == -1)
		return FALSE;

	// Loop through all known previous versions
	for (i = 0; i < verIndex && !bFound; i++)
	{
		bFound = OdbcDS.FindDSNByVersion(g_versions[i], Message);
	}

	return bFound;
}

// MigrateDS
//
// Description:
//   Migrates all data sources associated with one version of an ODBC/MX driver
//   to another version.
//
// Params:
//   oldDrvrName: Name of driver to update.
//   newDrvrName: Name of driver to which data sources will be migrated.
//   Message: Pointer to a previously allocated buffer. No boundary checks are
//            performed on this buffer.
//
// Returns:
//   TRUE the migration was successful. If an error occured, FALSE is returned
//   and Message will contain error information.
BOOL MigrateDS(const char* oldDrvName, const char* newDrvName, char* Message)
{
	if (oldDrvName == NULL || newDrvName == NULL || Message == NULL)
		return FALSE;

	COdbcDS OdbcDS;
	char* msg = NULL;
	msg = OdbcDS.Convert(oldDrvName, newDrvName);

	if (msg == NULL)
		return true;
	else
	{
		strcpy(Message, msg);
		return false;
	}
}

// MigrateAllDS
//
// Description:
//   Migrates all data sources associated with all previous versions of an
//   ODBC/MX driver to another version.
//
// Params:
//   DrvrName: Name of driver to which all data sources will be migrated.
//   Message: Pointer to a previously allocated buffer. No boundary checks are
//            performed on this buffer.
//
// Returns:
//   TRUE the migration was successful. If an error occured, FALSE is returned
//   and Message will contain error information.
BOOL MigrateAllDS(const char* DrvName, char* Message)
{
	if (DrvName == NULL || Message == NULL)
		return FALSE;

	int verIndex = -1;

	// Initialize Message since we'll be checking it later.
	*Message = '\0';

	// Find the driver name in the list
	for (int i = 0; i < g_versionCnt && verIndex == -1; i++)
	{
		if (strcmp(g_versions[i], DrvName) == 0)
			verIndex = i;
	}

	// Check to see if the driver version was not found.
	if (verIndex == -1)
	{
		strcpy(Message, "The driver was not found.");
		return FALSE;
	}

	// Loop through all known previous versions
	for (i = 0; i < verIndex; i++)
	{
		if (FindADSNbyVersion(g_versions[i], Message))
			MigrateDS(g_versions[i], DrvName, Message);

		// Check for potential error conditions.
		if (strlen(Message) > 0)
			return FALSE;
	}

	return TRUE;
}