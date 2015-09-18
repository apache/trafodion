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
// PropSheet.cpp : implementation file
//

#include "stdafx.h"
#include "drvr35adm.h"
#include "PropSheet.h"
#include "HtmlHelp.h"
#include <atlconv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropSheet

IMPLEMENT_DYNAMIC(CPropSheet, CPropertySheet)

const int CPropSheet::m_NewDSNIndexOffset = 7;

CPropSheet::CPropSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	bUpdateDSN=false;
}

CPropSheet::CPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	bUpdateDSN=false;
}

CPropSheet::~CPropSheet()
{
}


BEGIN_MESSAGE_MAP(CPropSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CPropSheet)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropSheet message handlers

#define ODBCMXDS					"odbcmxds.chm"

#define General_Create_DS			100
#define Network_Create_DS			110
#define Data_Access_Create_DS		120
#define Localize_Create_DS			125
#define Trace_Create_DS				130
#define Test_Create_DS				140

#define Test_dlg_box_Create_DS		150
#define General_Reconfigure_DS		200
#define Network_Reconfigure_DS		210
#define Data_Access_Reconfigure_DS	220
#define Trace_Reconfigure_DS		230
#define Test_Reconfigure_DS			240
#define Localize_Reconfigure_DS		225

enum {
	pageGeneral = 1,
	pageNetwork,
	pageOptions,
	pageLocalTr,
	pageTracing,
	pageTesting,
	tabGeneral,
	tabNetwork,
	tabOptions,
	tabLocalTr,
	tabTracing,
	tabTesting
};

int map[] = {
	General_Create_DS,
	Network_Create_DS,
	Data_Access_Create_DS,
	Localize_Create_DS,
	Trace_Create_DS,
	Test_Create_DS,

	Test_dlg_box_Create_DS,
	General_Reconfigure_DS,
	Network_Reconfigure_DS,
	Data_Access_Reconfigure_DS,
	Localize_Reconfigure_DS,
	Trace_Reconfigure_DS,
	Test_Reconfigure_DS,
};


BOOL CPropSheet::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	DisplayHtmlHelp();

	return CPropertySheet::OnHelpInfo(pHelpInfo);
}

BOOL CPropSheet::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	if(wParam==IDHELP)
	{
		DisplayHtmlHelp();
	}
	
	return CPropertySheet::OnCommand(wParam, lParam);
}

BOOL CPropSheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();

//	CMenu* pCMenu = GetSystemMenu(FALSE);
//	while (pCMenu != NULL && pCMenu->GetMenuItemCount() > 0)
//		pCMenu->DeleteMenu( 0, MF_BYPOSITION);

	if (bNewDSN == TRUE)
		SetWindowText(CREATE_NEW_DSN);
	else
	{
		CString cTitle;

		cTitle = cTitle + CONFIGURE_DSN1 + aAttr[ KEY_DSN].szAttr + CONFIGURE_DSN2;
		SetWindowText(cTitle);
	}

	return bResult;
}

bool CPropSheet::UpdateDSN( void )
{
	char* lpszDSN;
	char  szBuf[ _MAX_PATH];

	lpszDSN = aAttr[ KEY_DSN].szAttr;

// if the data source name has changed, remove the old name
	if( !bNewDSN && strcmp( szDSN, lpszDSN) != 0)
	{
		SQLRemoveDSNFromIni( szDSN);
// remove File Data Source Name
		DeleteFileDSN( szDSN );
	}

	if(strcmp( szDSN, lpszDSN )!=0)
	{
		WritePrivateProfileString( lpszDSN,	INI_KDESC, aAttr[ KEY_DESC].szAttr,	ODBC_INI);
		WritePrivateProfileString( lpszDSN,	INI_KSN, aAttr[ KEY_SERVICE_NAME].szAttr,	ODBC_INI);
// update File DSN
		lstrcpy(szBuf, lpszDSN );
		lstrcat(szBuf, " (not sharable)");
		SQLWriteFileDSN( szBuf, "ODBC", "DSN", lpszDSN );
		SQLWritePrivateProfileString( lpszDSN,	INI_KDESC, aAttr[ KEY_DESC].szAttr,	ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,	INI_KSN, aAttr[ KEY_SERVICE_NAME].szAttr,	ODBC_INI);
	}

// write data source name
	if( !SQLWriteDSNToIni( lpszDSN, lpszDrvr))
	{
		char  szMsg[ MAXPATHLEN];
		CString szBuf;
			
		szBuf.LoadString(IDS_BADDSN);
		wsprintf( szMsg, szBuf, lpszDSN);			
		AfxMessageBox( szMsg);
		return FALSE;
	}

// update ODBC.INI
// save the value if the data source is new, edited, or explicitly supplied
	WritePrivateProfileString( lpszDSN,	INI_KDESC, aAttr[ KEY_DESC].szAttr,	ODBC_INI);
	WritePrivateProfileString( lpszDSN,	INI_KSN, aAttr[ KEY_SERVICE_NAME].szAttr,	ODBC_INI);
// update File DSN
	lstrcpy(szBuf, lpszDSN );
	lstrcat(szBuf, " (not sharable)");
	SQLWriteFileDSN( szBuf, "ODBC", "DSN", lpszDSN );
	SQLWritePrivateProfileString( lpszDSN,	INI_KDESC, aAttr[ KEY_DESC].szAttr,	ODBC_INI);
	SQLWritePrivateProfileString( lpszDSN,	INI_KSN, aAttr[ KEY_SERVICE_NAME].szAttr,	ODBC_INI);

// the following are the localization (internationalization) attributes
	WritePrivateProfileString( lpszDSN,	INI_ERRORLANG, aAttr[ KEY_ERRORLANG].szAttr, ODBC_INI);
	SQLWritePrivateProfileString( lpszDSN,	INI_ERRORLANG, aAttr[ KEY_ERRORLANG].szAttr, ODBC_INI);

	WritePrivateProfileString( lpszDSN,	INI_DATALANG, aAttr[ KEY_DATALANG].szAttr, ODBC_INI);
	SQLWritePrivateProfileString( lpszDSN,	INI_DATALANG, aAttr[ KEY_DATALANG].szAttr, ODBC_INI);

// Association Service (IP Address and Port Number)
	wsprintf(szBuf, "%s%s/%s", TCP_STR, aAttr[ KEY_IPADDRESS].szAttr, aAttr[ KEY_PORTNUM].szAttr);

	WritePrivateProfileString( lpszDSN,	INI_NETWORK, szBuf, ODBC_INI);
	SQLWritePrivateProfileString( lpszDSN,	INI_NETWORK, szBuf, ODBC_INI);

	WritePrivateProfileString( lpszDSN,	INI_LOGIN, aAttr[ KEY_LOGIN].szAttr, ODBC_INI);
	SQLWritePrivateProfileString( lpszDSN,	INI_LOGIN, aAttr[ KEY_LOGIN].szAttr, ODBC_INI);

	WritePrivateProfileString( lpszDSN,	INI_CONNECTION, aAttr[ KEY_CONNECTION].szAttr, ODBC_INI);
	SQLWritePrivateProfileString( lpszDSN,	INI_CONNECTION, aAttr[ KEY_CONNECTION].szAttr, ODBC_INI);

	WritePrivateProfileString( lpszDSN,	INI_QUERY, aAttr[ KEY_QUERY].szAttr, ODBC_INI);
	SQLWritePrivateProfileString( lpszDSN,	INI_QUERY, aAttr[ KEY_QUERY].szAttr, ODBC_INI);

	WritePrivateProfileString( lpszDSN,	INI_CATALOG, aAttr[ KEY_CATALOG].szAttr, ODBC_INI);
	SQLWritePrivateProfileString( lpszDSN,	INI_CATALOG, aAttr[ KEY_CATALOG].szAttr, ODBC_INI);
	
	WritePrivateProfileString( lpszDSN,	INI_SCHEMA, aAttr[ KEY_SCHEMA].szAttr, ODBC_INI);
	SQLWritePrivateProfileString( lpszDSN,	INI_SCHEMA, aAttr[ KEY_SCHEMA].szAttr, ODBC_INI);

	WritePrivateProfileString( lpszDSN,	INI_TRANSLATION_DLL, aAttr[ KEY_TRANSLATION_DLL].szAttr, ODBC_INI);
	SQLWritePrivateProfileString( lpszDSN,	INI_TRANSLATION_DLL, aAttr[ KEY_TRANSLATION_DLL].szAttr, ODBC_INI);

	WritePrivateProfileString( lpszDSN,	INI_TRANSLATION_OPTION, aAttr[ KEY_TRANSLATION_OPTION].szAttr, ODBC_INI);
	SQLWritePrivateProfileString( lpszDSN,	INI_TRANSLATION_OPTION, aAttr[ KEY_TRANSLATION_OPTION].szAttr, ODBC_INI);

	WritePrivateProfileString( lpszDSN,	INI_FETCH_BUFFER_SIZE, aAttr[ KEY_FETCH_BUFFER_SIZE].szAttr, ODBC_INI);
	SQLWritePrivateProfileString( lpszDSN,	INI_FETCH_BUFFER_SIZE, aAttr[ KEY_FETCH_BUFFER_SIZE].szAttr, ODBC_INI);

	WritePrivateProfileString( lpszDSN,	INI_REPLACEMENT_CHAR, aAttr[ KEY_REPLACEMENT_CHAR].szAttr, ODBC_INI);
	SQLWritePrivateProfileString( lpszDSN,	INI_REPLACEMENT_CHAR, aAttr[ KEY_REPLACEMENT_CHAR].szAttr, ODBC_INI);

	WritePrivateProfileString( lpszDSN,	INI_COMPRESSION, aAttr[ KEY_COMPRESSION].szAttr, ODBC_INI);
	SQLWritePrivateProfileString( lpszDSN,	INI_COMPRESSION, aAttr[ KEY_COMPRESSION].szAttr, ODBC_INI);

	WriteTraceRegistry( INI_TRACE_FILE,aAttr[ KEY_TRACE_FILE].szAttr);
	
	WriteTraceRegistry( INI_TRACE_FLAGS,aAttr[ KEY_TRACE_FLAGS].szAttr);

	return TRUE;
}

CString CPropSheet::GetHelpFilePath()
{
    CString helpPath, regKey, regValue;
    HKEY ODBCKey;
    DWORD MaxValue = MAX_PATH + 1;
	bool bError = true;

	regKey.LoadString(IDS_REGKEY_HELP);
	regValue.LoadString(IDS_REGVALUE_HELP);

    //  Get the path name out of the registry.
    //  If this fails, return the system directory.
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    regKey,
					0,
					KEY_READ,
                    &ODBCKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(ODBCKey, regValue, NULL, NULL,
                         (LPBYTE) helpPath.GetBuffer(MaxValue),
                         &MaxValue) == ERROR_SUCCESS)
		{
			bError = false;
		}
		helpPath.ReleaseBuffer();
		RegCloseKey(ODBCKey);
    }
	
	if (bError)
	{
		GetSystemDirectory(helpPath.GetBuffer(MaxValue), MaxValue);
		helpPath.ReleaseBuffer();
	}

	// Append a "\" if directory path doesn't already have one.
	if (helpPath.Right(1) != _T("\\"))
		helpPath += _T("\\");

    return helpPath;
}

/******************************************************************************
DisplayHtmlHelp()

  Description:
    This function performs some error handling of the HtmlHelp() API.

  Returns:
    TRUE  - An HtmlHelp window was successfully created.
	FALSE - An HtmlHelp window could not be created.
*****************************************************************************/
BOOL CPropSheet::DisplayHtmlHelp()
{
	USES_CONVERSION; // For Unicode to ANSI string conversion

	HWND hwndHelp = NULL;
	CString strError, strCaption, strFile;
	HH_LAST_ERROR hherror;
	int index = GetActiveIndex();
	int context = -1;

	// Setting up the file name
	strFile.LoadString(IDS_HELPFILENAME);
	strFile = GetHelpFilePath() + strFile;

	// Setting up the index
	if (bNewDSN != TRUE) index += m_NewDSNIndexOffset;
	context = map[index];
#ifndef _WIN64
	// Win64 link gives unresolved reference
	hwndHelp = ::HtmlHelp(m_hWnd, strFile, HH_HELP_CONTEXT, context);
#endif

	if (hwndHelp == NULL)
	{
		// Try to get error message.
#ifndef _WIN64
	// Win64 link gives unresolved reference
		hwndHelp = ::HtmlHelp(m_hWnd,
							NULL, HH_GET_LAST_ERROR,
							reinterpret_cast<DWORD>(&hherror));
#endif

		// Generate an error message if we have one.
		if (hwndHelp != 0)
		{
			if (hherror.hr < 0)
			{
				// Is there a text message to display?
				if (hherror.description)
				{
					// Convert the String to ANSI
					TCHAR* pDesc = OLE2T(hherror.description) ;
					::SysFreeString(hherror.description) ;

					// Generate our message
					strError = pDesc;
				}
			}
		}

		// Couldn't determine error from HtmlHelp. Generate a generic error message.
		else
		{
			strError.Format(IDS_HELP_GENERIC_ERROR, strFile);
		}

		strCaption.LoadString(IDS_HELP_ERROR_CAPTION);
		MessageBox(strError, strCaption, MB_OK | MB_ICONERROR);
		return FALSE;
	}
	else
		return TRUE;
}