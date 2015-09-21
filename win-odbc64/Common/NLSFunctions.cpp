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
#ifdef _AFXDLL
	#include <stdafx.h>
#else
	#include <windows.h>
#endif
#include "NLSFunctions.h"

// This module contains all specific locale information functions.
// The current implementation is windows based, something different 
// has to be developed for NSK.

void ODBCNLS_GetCodePage(unsigned long *dwACP)
{
#ifdef _AFXDLL
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
#endif
	// This function will retrieve the Active Code Page (ACP)
	// default values
	*dwACP = GetACP();
}


void ODBCNLS_GetErrorLanguage(unsigned long *dwLanguageId)
{
#ifdef _AFXDLL
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
#endif

	ODBCNLS_GetLanguage ("TDM_ODBC_ERROR_LANG", dwLanguageId);
}

void ODBCNLS_GetSQLLanguage(unsigned long *dwLanguageId)
{
#ifdef _AFXDLL
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
#endif

	ODBCNLS_GetLanguage("TDM_ODBC_DATA_LANG", dwLanguageId);
}

void ODBCNLS_GetLanguage(char * envVar, unsigned long *dwLanguageId)
{
#ifdef _AFXDLL
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
#endif

	char			*EnvVariable;

	*dwLanguageId = LOCALE_SYSTEM_DEFAULT;
	
	// try to get the value from an environment variable
	EnvVariable = getenv(envVar);
	if (EnvVariable != NULL)
	{
		if ((*dwLanguageId = atol (EnvVariable)) != 0)
		{
			ODBCNLS_ValidateLanguage (dwLanguageId);
			return;
		}
		else
			return;
	}
	ODBCNLS_GetLanguage (dwLanguageId);
}

void ODBCNLS_GetLanguage(unsigned long *dwLanguageId)
{
#ifdef _AFXDLL
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
#endif

	OSVERSIONINFO	VersionInformation;

	// we'll get current locale settings, we have to be specific to the platform
	VersionInformation.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	
	if (GetVersionEx( &VersionInformation )!=0)	
	{
		// get values depending on platform specific API calls...
		if (VersionInformation.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		{
			// Windows95 when dwMinorVersion = 0
			// Windows98 when dwMinorVersion = 10
			// get system locale, is 0 OK?
			*dwLanguageId = GetSystemDefaultLCID();
		}
		else if (VersionInformation.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			// get thread locale
			*dwLanguageId = GetThreadLocale();
		}
	}
	ODBCNLS_ValidateLanguage (dwLanguageId);
}

void ODBCNLS_ValidateLanguage (unsigned long *dwLanguageId)
{
#ifdef _AFXDLL
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
#endif

	if ((*dwLanguageId) != 0)
	{
		switch (PRIMARYLANGID (*dwLanguageId))
		{
			// we'll only support english and japanese for now
			case LANG_NEUTRAL:
			case LANG_ENGLISH:
			case LANG_JAPANESE:
				return;
			default:
				// other languages will default to english
				*dwLanguageId = MAKELCID (MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT);
				return;
		}
	}
}
