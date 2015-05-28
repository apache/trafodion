// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2002-2014 Hewlett-Packard Development Company, L.P.
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

// Drvr35Adm.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "Drvr35Adm.h"
#include "pagegeneral.h"
#include "pagenetwork.h"
#include "pageoptions.h"
#include "pagelocaltr.h"
#include "pagetracing.h"
#include "tabpagegeneral.h"
#include "tabpagenetwork.h"
#include "tabpageoptions.h"
#include "tabpagelocaltr.h"
#include "tabpagetracing.h"
#include "tabpagetesting.h"
#include "propsheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL bNewDSN;
char szDSN[MAXDSNAME];	// Original data source name

LPCSTR lpszDrvr;
HWND hwndParent;
LPAttr aAttr;			// Attribute array pointer
struct LOOKUP_ENTRY
{
	char	szKey[ MAXKEYLEN];
	int		iKey;
};


// Attribute string look-up table (maps keys to associated indexes)
struct LOOKUP_ENTRY s_aLookup[] =
{
	"DSN",							KEY_DSN,
	"Description",					KEY_DESC,
	"Catalog",						KEY_CATALOG,
	"Schema",						KEY_SCHEMA,
	"Location",						KEY_LOCATION,
	"SQL_LOGIN_TIMEOUT",			KEY_LOGIN,
	"SQL_ATTR_CONNECTION_TIMEOUT",	KEY_CONNECTION,
	"SQL_QUERY_TIMEOUT",			KEY_QUERY,
	"IPAddress",					KEY_IPADDRESS,
	"PortNumber",					KEY_PORTNUM,
	"ErrorMsgLang",					KEY_ERRORLANG,
	"DataLang",						KEY_DATALANG,
	"TraceFlags",					KEY_TRACE_FLAGS,
	"TraceFile",					KEY_TRACE_FILE,
	"TranslationDLL",				KEY_TRANSLATION_DLL,
	"TranslationOption",			KEY_TRANSLATION_OPTION,
	"FetchBufferSize",				KEY_FETCH_BUFFER_SIZE,
	"ServiceName",					KEY_SERVICE_NAME,
	"ReplacementCharacter",			KEY_REPLACEMENT_CHAR,
	"Compression",					KEY_COMPRESSION,
	"",								0
};

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CDrvr35AdmApp

BEGIN_MESSAGE_MAP(CDrvr35AdmApp, CWinApp)
	//{{AFX_MSG_MAP(CDrvr35AdmApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDrvr35AdmApp construction

CDrvr35AdmApp::CDrvr35AdmApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDrvr35AdmApp object

CDrvr35AdmApp theApp;

void ParseAttributes (LPCSTR  lpszAttributes);

void LoadDriverDll();

/* ConfigDSN ---------------------------------------------------------------
  Description:  ODBC Setup entry point
                This entry point is called by the ODBC Installer
                (see file header for more details)
  Input      :  hwnd ----------- Parent window handle
                fRequest ------- Request type (i.e., add, config, or remove)
                lpszIDriver ---- Driver name
                lpszAttributes - data source attribute string
  Output     :  TRUE success, FALSE otherwise
--------------------------------------------------------------------------*/
extern "C" BOOL WINAPI EXPORT ConfigDSN (HWND    hwnd,
                           WORD    fRequest,
                           LPCSTR  lpszDriver,
                           LPCSTR  lpszAttributes)
{
#ifdef _AFXDLL
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
#endif

	BOOL  fSuccess;		// Success/fail flag

	lpszDrvr = lpszDriver;
	hwndParent = hwnd;

	aAttr = (struct ATTR *)malloc (sizeof(Attr) * NUMOFKEYS);

	// Not sure about the return code
	if (aAttr == NULL)
		return FALSE;

	for (int i = 0; i< NUMOFKEYS; i++)
	{
		aAttr[i].fSupplied = 0;
		aAttr[i].szAttr[0] = '\0';
	}

	// Parse attribute string
	if( lpszAttributes)
	{
		ParseAttributes( lpszAttributes);
	}

	// Save original data source name
	if( aAttr[ KEY_DSN].fSupplied)
	{
		lstrcpy( szDSN, aAttr[ KEY_DSN].szAttr);
	}
	else
	{
		szDSN[ 0] = '\0';
	}


	if( fRequest == ODBC_REMOVE_DSN)
	{	// Process REMOVE data source
		if( !aAttr[ KEY_DSN].fSupplied)
		{	// Fail if no data source name was supplied
			fSuccess = FALSE;
		}
		else
		{	// Otherwise remove data source from ODBC.INI
			fSuccess = SQLRemoveDSNFromIni( aAttr[ KEY_DSN].szAttr);

			// delete File DSN
			DeleteFileDSN( aAttr[ KEY_DSN].szAttr);
		}
	}
	else
	{	// Process ADD and CONFIGURE data source

		if (hwnd)
		{
			LPCSTR  lpszDSN;
			int posTCP;
			int posSlash;
			LPSTR ptr;
	
			bNewDSN = fRequest == ODBC_ADD_DSN;
// Data source name
			lpszDSN = aAttr[ KEY_DSN].szAttr;
// Data source description
			if( !aAttr[ KEY_DESC].fSupplied)
				SQLGetPrivateProfileString( lpszDSN, INI_KDESC, "", aAttr[ KEY_DESC].szAttr,
						sizeof( aAttr[ KEY_DESC].szAttr), ODBC_INI);
// Service name
			if( !aAttr[ KEY_SERVICE_NAME].fSupplied)
				SQLGetPrivateProfileString( lpszDSN, INI_KSN, "", aAttr[ KEY_SERVICE_NAME].szAttr,
						sizeof( aAttr[ KEY_SERVICE_NAME].szAttr), ODBC_INI);
			
// Catalog string
			if( !aAttr[ KEY_CATALOG].fSupplied)
				SQLGetPrivateProfileString( lpszDSN, INI_CATALOG, "", aAttr[ KEY_CATALOG].szAttr,
						sizeof( aAttr[ KEY_CATALOG].szAttr), ODBC_INI);
		
// Schema string
			if( !aAttr[ KEY_SCHEMA].fSupplied)
				SQLGetPrivateProfileString( lpszDSN, INI_SCHEMA, "", aAttr[ KEY_SCHEMA].szAttr,
						sizeof( aAttr[ KEY_SCHEMA].szAttr), ODBC_INI);
		
// Association Service (IP Address and Port Number)
			if( (!aAttr[ KEY_IPADDRESS].fSupplied) && (!aAttr[ KEY_PORTNUM].fSupplied))
			{					
				char szBuf[MAXIPLEN+30];
				memset(szBuf, '\0', sizeof(szBuf));
				SQLGetPrivateProfileString( lpszDSN, INI_NETWORK, "", szBuf,
					sizeof( szBuf), ODBC_INI);
				if (strlen(szBuf))
				{
					ptr = szBuf;
// posTCP represents the position of the colon.				
					posTCP = strcspn( szBuf, ":");	
// posSlash represents the position of the slash.				
					posSlash = strcspn( szBuf, "/");	
					if ((posTCP >0) && (posSlash >0))				
					{					
// ptr points to the buffer after :, not including :				
						ptr = ptr + posTCP + 1;					
						strncpy(aAttr[ KEY_IPADDRESS].szAttr, ptr, posSlash - posTCP-1);				

						aAttr[ KEY_IPADDRESS].szAttr[posSlash - posTCP-1] = '\0';
							
						ptr = szBuf + posSlash + 1;				
						strncpy(aAttr[ KEY_PORTNUM].szAttr, ptr, strlen(szBuf) - posSlash-1);								
						aAttr[ KEY_PORTNUM].szAttr[strlen(szBuf) - posSlash-1] = '\0';
				
					}
				}				
			}

			if( !aAttr[ KEY_LOGIN].fSupplied)
				SQLGetPrivateProfileString( lpszDSN, INI_LOGIN, "", aAttr[ KEY_LOGIN].szAttr,
					sizeof( aAttr[ KEY_LOGIN].szAttr), ODBC_INI);

			if( !aAttr[ KEY_CONNECTION].fSupplied)
				SQLGetPrivateProfileString( lpszDSN, INI_CONNECTION, "", aAttr[ KEY_CONNECTION].szAttr,
					sizeof( aAttr[ KEY_CONNECTION].szAttr), ODBC_INI);

			if( !aAttr[ KEY_QUERY].fSupplied)
				SQLGetPrivateProfileString( lpszDSN, INI_QUERY, "", aAttr[ KEY_QUERY].szAttr,
					sizeof( aAttr[ KEY_QUERY].szAttr), ODBC_INI);

			if( !aAttr[ KEY_ERRORLANG].fSupplied)
				SQLGetPrivateProfileString( lpszDSN, INI_ERRORLANG, "", aAttr[ KEY_ERRORLANG].szAttr,
					sizeof( aAttr[ KEY_ERRORLANG].szAttr), ODBC_INI);

			if( !aAttr[ KEY_DATALANG].fSupplied)
				SQLGetPrivateProfileString( lpszDSN, INI_DATALANG, "", aAttr[ KEY_DATALANG].szAttr,
					sizeof( aAttr[ KEY_DATALANG].szAttr), ODBC_INI);

			if( !aAttr[ KEY_TRANSLATION_DLL].fSupplied)
				SQLGetPrivateProfileString( lpszDSN, INI_TRANSLATION_DLL, "", aAttr[ KEY_TRANSLATION_DLL].szAttr,
					sizeof( aAttr[ KEY_TRANSLATION_DLL].szAttr), ODBC_INI);

			if( !aAttr[ KEY_TRANSLATION_OPTION].fSupplied)
				SQLGetPrivateProfileString( lpszDSN, INI_TRANSLATION_OPTION, "", aAttr[ KEY_TRANSLATION_OPTION].szAttr,
					sizeof( aAttr[ KEY_TRANSLATION_OPTION].szAttr), ODBC_INI);

			if(!aAttr[ KEY_TRACE_FLAGS].fSupplied)
				SQLGetPrivateProfileString(szODBC, 
										INI_TRACE_FLAGS, 
										szDefaultTraceFlags,
										aAttr[ KEY_TRACE_FLAGS].szAttr, 
										sizeof( aAttr[ KEY_TRACE_FLAGS].szAttr), 
										szODBCIni);

			if(!aAttr[ KEY_TRACE_FILE].fSupplied)
				SQLGetPrivateProfileString(szODBC, 
										INI_TRACE_FILE, 
										szDefaultTraceFile,
										aAttr[ KEY_TRACE_FILE].szAttr, 
										sizeof( aAttr[ KEY_TRACE_FILE].szAttr), 
										szODBCIni);

			if( !aAttr[ KEY_FETCH_BUFFER_SIZE].fSupplied)
				SQLGetPrivateProfileString( lpszDSN, INI_FETCH_BUFFER_SIZE, "", aAttr[ KEY_FETCH_BUFFER_SIZE].szAttr,
					sizeof( aAttr[ KEY_FETCH_BUFFER_SIZE].szAttr), ODBC_INI);

			if( !aAttr[ KEY_REPLACEMENT_CHAR].fSupplied)
				SQLGetPrivateProfileString( lpszDSN, INI_REPLACEMENT_CHAR, "", aAttr[ KEY_REPLACEMENT_CHAR].szAttr,
					sizeof( aAttr[ KEY_REPLACEMENT_CHAR].szAttr), ODBC_INI);

			if( !aAttr[ KEY_COMPRESSION].fSupplied)
				SQLGetPrivateProfileString( lpszDSN, INI_COMPRESSION, "", aAttr[ KEY_COMPRESSION].szAttr,
					sizeof( aAttr[ KEY_COMPRESSION].szAttr), ODBC_INI);
			CPropSheet dlgPropSheet("TRAF ODBC Setup");
			dlgPropSheet.m_psh.hIcon = AfxGetApp()->LoadIcon(IDI_BOBICON);
			dlgPropSheet.m_psh.dwFlags |= (PSH_USEHICON | PSH_HASHELP) ;

			if(bNewDSN)
			{
				PageGeneral pageGeneral;
				PageNetwork pageNetwork;
				PageOptions pageOptions;
				PageLocalTr pageLocalTr;
				//PageTracing pageTracing;

				pageGeneral.m_ppropsheet = &dlgPropSheet;
				pageNetwork.m_ppropsheet = &dlgPropSheet;
				pageOptions.m_ppropsheet = &dlgPropSheet;
				//pageTracing.m_ppropsheet = &dlgPropSheet;
				pageLocalTr.m_ppropsheet = &dlgPropSheet;

				pageGeneral.m_psp.dwFlags |= PSP_HASHELP ;
				pageNetwork.m_psp.dwFlags |= PSP_HASHELP ;
				pageOptions.m_psp.dwFlags |= PSP_HASHELP ;
				pageLocalTr.m_psp.dwFlags |= PSP_HASHELP ;
				//pageTracing.m_psp.dwFlags |= PSP_HASHELP ;

				dlgPropSheet.AddPage(&pageGeneral);
				dlgPropSheet.AddPage(&pageNetwork);
				dlgPropSheet.AddPage(&pageOptions);
				dlgPropSheet.AddPage(&pageLocalTr);
				//dlgPropSheet.AddPage(&pageTracing);

				dlgPropSheet.SetWizardMode();

				if(dlgPropSheet.DoModal()==ID_WIZFINISH)
					fSuccess = TRUE;
				else
					fSuccess = FALSE;
			}
			else
			{

				TabPageGeneral pageGeneral;
				TabPageNetwork pageNetwork;
				TabPageOptions pageOptions;
				TabPageLocalTr pageLocalTr;
				//TabPageTracing pageTracing;
				TabPageTesting pageTesting;

				pageGeneral.m_psp.dwFlags |= PSP_HASHELP ;
				pageNetwork.m_psp.dwFlags |= PSP_HASHELP ;
				pageOptions.m_psp.dwFlags |= PSP_HASHELP ;
				pageLocalTr.m_psp.dwFlags |= PSP_HASHELP ;
				//pageTracing.m_psp.dwFlags |= PSP_HASHELP ;
				pageTesting.m_psp.dwFlags |= PSP_HASHELP ;

				pageGeneral.m_ppropsheet = &dlgPropSheet;
				pageNetwork.m_ppropsheet = &dlgPropSheet;
				pageOptions.m_ppropsheet = &dlgPropSheet;
				//pageTracing.m_ppropsheet = &dlgPropSheet;
				pageLocalTr.m_ppropsheet = &dlgPropSheet;
				pageTesting.m_ppropsheet = &dlgPropSheet;

				dlgPropSheet.AddPage(&pageGeneral);
				dlgPropSheet.AddPage(&pageNetwork);
				dlgPropSheet.AddPage(&pageOptions);
				dlgPropSheet.AddPage(&pageLocalTr);
				//dlgPropSheet.AddPage(&pageTracing);
				dlgPropSheet.AddPage(&pageTesting);

				if(dlgPropSheet.DoModal()==IDOK)
					fSuccess = TRUE;
				else
					fSuccess = FALSE;
			}
		}

		else if( aAttr[ KEY_DSN].fSupplied)
			fSuccess = SetDSNAttributes( hwnd, lpszDriver, fRequest==ODBC_ADD_DSN);
		else
			fSuccess = FALSE;
	}
	free (aAttr);
	return fSuccess;
}


/* ParseAttributes ---------------------------------------------------------
  Description:  Parse attribute string moving values into the aAttr array
  Input      :  lpszAttributes - Pointer to attribute string
  Output     :  None (global aAttr normally updated)
--------------------------------------------------------------------------*/
void ParseAttributes(LPCSTR lpszAttributes)
{
  
  LPCSTR  lpsz;
  LPCSTR  lpszStart;
  char    aszKey[MAXKEYLEN];
  long   iElement;
  long   cbKey;

  for (lpsz=lpszAttributes; *lpsz; lpsz++) {
    //  Extract key name (e.g., DSN), it must be terminated by an equals
    lpszStart = lpsz;
    for (;; lpsz++)
      if (!*lpsz)            return;      // No key was found
      else if (*lpsz == '=') break;       // Valid key found

    // Determine the key's index in the key table (-1 if not found)
    iElement = -1;
    cbKey    = lpsz - lpszStart;
    if (cbKey < sizeof(aszKey)) {
      register short  j;

      memcpy(aszKey, lpszStart, cbKey);
      aszKey[cbKey] = '\0';
      for (j = 0; *s_aLookup[j].szKey; j++)
        if (!lstrcmpi(s_aLookup[j].szKey, aszKey)) {
          iElement = s_aLookup[j].iKey;
          break;
        }
    }

    // Locate end of key value
    lpszStart = ++lpsz;
    for (; *lpsz; lpsz++);

    // Save value if key is known
    // NOTE: This code assumes the szAttr buffers in aAttr have been
    //       zero initialized
    if (iElement >= 0) {
      aAttr[iElement].fSupplied = TRUE;
      memcpy(aAttr[iElement].szAttr,
               lpszStart,
               min(lpsz-lpszStart+1, sizeof(aAttr[0].szAttr)-1));
    }
  }
  return;
  
}

/* SetDSNAttributes --------------------------------------------------------
  Description:  Write data source attributes to ODBC.INI
  Input      :  hwnd - Parent window handle (plus globals)
  Output     :  TRUE if successful, FALSE otherwise
--------------------------------------------------------------------------*/
BOOL SetDSNAttributes( HWND hwndParent, LPCSTR  lpszDrvr, BOOL    bNewDSN)
{
	LPCSTR	lpszDSN;                        // Pointer to data source name

	lpszDSN = aAttr[ KEY_DSN].szAttr;

	// validate data source name argument
	if( bNewDSN && !*aAttr[ KEY_DSN].szAttr)
		{
		return FALSE;
		}

	// write data source name
	if( !SQLWriteDSNToIni( lpszDSN, lpszDrvr))
	{
		if( hwndParent)
		{
			
			char  szMsg[ MAXPATHLEN];
			CString szBuf;
			
			szBuf.LoadString(IDS_BADDSN);
			wsprintf( szMsg, szBuf, lpszDSN);			
			AfxMessageBox( szMsg);
		}
		return FALSE;
	}

	// update ODBC.INI
	// save the value if the data source is new, edited, or explicitly supplied
	if( hwndParent || aAttr[ KEY_DESC].fSupplied)
		{
		char  szBuf[ MAXPATHLEN];

		WritePrivateProfileString( lpszDSN,	INI_KDESC, aAttr[ KEY_DESC].szAttr,	ODBC_INI);
// update File DSN
		lstrcpy(szBuf, lpszDSN );
		lstrcat(szBuf, " (not sharable)");
		SQLWriteFileDSN( szBuf, "ODBC", "DSN", lpszDSN );

		SQLWritePrivateProfileString( lpszDSN,	INI_KDESC, aAttr[ KEY_DESC].szAttr,	ODBC_INI);
		}

	if( hwndParent || aAttr[ KEY_SERVICE_NAME].fSupplied)
		{
		WritePrivateProfileString( lpszDSN,	INI_KSN, aAttr[ KEY_SERVICE_NAME].szAttr, ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,	INI_KSN, aAttr[ KEY_SERVICE_NAME].szAttr, ODBC_INI);
		}

	if( hwndParent || aAttr[ KEY_CATALOG].fSupplied)
		{
		WritePrivateProfileString( lpszDSN,	INI_CATALOG, aAttr[ KEY_CATALOG].szAttr, ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,	INI_CATALOG, aAttr[ KEY_CATALOG].szAttr, ODBC_INI);
		}

	if( hwndParent || aAttr[ KEY_SCHEMA].fSupplied)
		{
		WritePrivateProfileString( lpszDSN,	INI_SCHEMA, aAttr[ KEY_SCHEMA].szAttr, ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,	INI_SCHEMA, aAttr[ KEY_SCHEMA].szAttr, ODBC_INI);
		}

	if( hwndParent || aAttr[ KEY_LOCATION].fSupplied)
		{
		WritePrivateProfileString( lpszDSN,	INI_LOCATION, aAttr[ KEY_LOCATION].szAttr, ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,	INI_LOCATION, aAttr[ KEY_LOCATION].szAttr, ODBC_INI);
		}

// Association Service (IP Address and Port Number)
// fSupplied is not working for some unknown reason at this time.
//	 if( hwndParent)
		if( hwndParent || aAttr[ KEY_IPADDRESS].fSupplied )
		{	
//		if (strlen(aAttr[ KEY_PORTNUM].szAttr)|| strlen(aAttr[ KEY_IPADDRESS].szAttr))
//		{
			char szBuf[MAXIPLEN+30];
			wsprintf(szBuf, "%s%s/%s", TCP_STR, aAttr[ KEY_IPADDRESS].szAttr, aAttr[ KEY_PORTNUM].szAttr);

		WritePrivateProfileString( lpszDSN,	INI_NETWORK, szBuf, ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,	INI_NETWORK, szBuf, ODBC_INI);
		//}
		
		}
		
	if( hwndParent || aAttr[ KEY_LOGIN].fSupplied)
		{
		WritePrivateProfileString( lpszDSN,	INI_LOGIN, aAttr[ KEY_LOGIN].szAttr, ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,	INI_LOGIN, aAttr[ KEY_LOGIN].szAttr, ODBC_INI);
		}
	if( hwndParent || aAttr[ KEY_CONNECTION].fSupplied)
		{
		WritePrivateProfileString( lpszDSN,	INI_CONNECTION, aAttr[ KEY_CONNECTION].szAttr, ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,	INI_CONNECTION, aAttr[ KEY_CONNECTION].szAttr, ODBC_INI);
		}
	if( hwndParent || aAttr[ KEY_QUERY].fSupplied)
		{
		WritePrivateProfileString( lpszDSN,	INI_QUERY, aAttr[ KEY_QUERY].szAttr, ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,	INI_QUERY, aAttr[ KEY_QUERY].szAttr, ODBC_INI);
		}

	// the following are the localization (internationalization) attributes
	if( hwndParent || aAttr[ KEY_ERRORLANG].fSupplied)
		{
		WritePrivateProfileString( lpszDSN,	INI_ERRORLANG, aAttr[ KEY_ERRORLANG].szAttr, ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,	INI_ERRORLANG, aAttr[ KEY_ERRORLANG].szAttr, ODBC_INI);
		}

	if( hwndParent || aAttr[ KEY_DATALANG].fSupplied)
		{
		WritePrivateProfileString( lpszDSN,	INI_DATALANG, aAttr[ KEY_DATALANG].szAttr, ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,	INI_DATALANG, aAttr[ KEY_DATALANG].szAttr, ODBC_INI);
		}

	if( hwndParent || aAttr[ KEY_TRANSLATION_DLL].fSupplied)
		{
		WritePrivateProfileString( lpszDSN,	INI_TRANSLATION_DLL, aAttr[ KEY_TRANSLATION_DLL].szAttr, ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,	INI_TRANSLATION_DLL, aAttr[ KEY_TRANSLATION_DLL].szAttr, ODBC_INI);
		}

	if( hwndParent || aAttr[ KEY_TRANSLATION_OPTION].fSupplied)
		{
		WritePrivateProfileString( lpszDSN,	INI_TRANSLATION_OPTION, aAttr[ KEY_TRANSLATION_OPTION].szAttr, ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,	INI_TRANSLATION_OPTION, aAttr[ KEY_TRANSLATION_OPTION].szAttr, ODBC_INI);
		}

	if( hwndParent || aAttr[ KEY_FETCH_BUFFER_SIZE].fSupplied)
		{
		WritePrivateProfileString( lpszDSN,	INI_FETCH_BUFFER_SIZE, aAttr[ KEY_FETCH_BUFFER_SIZE].szAttr, ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,	INI_FETCH_BUFFER_SIZE, aAttr[ KEY_FETCH_BUFFER_SIZE].szAttr, ODBC_INI);
		}

	if( hwndParent || aAttr[ KEY_TRACE_FILE].fSupplied)
		{
		WriteTraceRegistry( INI_TRACE_FILE,aAttr[ KEY_TRACE_FILE].szAttr);
		}
	if( hwndParent || aAttr[ KEY_TRACE_FLAGS].fSupplied)
		{
		WriteTraceRegistry( INI_TRACE_FLAGS,aAttr[ KEY_TRACE_FLAGS].szAttr);
		}

	if( hwndParent || aAttr[ KEY_REPLACEMENT_CHAR].fSupplied)
		{
		WritePrivateProfileString( lpszDSN,	INI_REPLACEMENT_CHAR, aAttr[ KEY_REPLACEMENT_CHAR].szAttr, ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,	INI_REPLACEMENT_CHAR, aAttr[ KEY_REPLACEMENT_CHAR].szAttr, ODBC_INI);
		}

	if( hwndParent || aAttr[ KEY_COMPRESSION].fSupplied)
		{
		WritePrivateProfileString( lpszDSN,	INI_COMPRESSION, aAttr[ KEY_COMPRESSION].szAttr, ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,	INI_COMPRESSION, aAttr[ KEY_COMPRESSION].szAttr, ODBC_INI);
		}


// if the data source name has changed, remove the old name
	if( aAttr[ KEY_DSN].fSupplied && lstrcmpi( szDSN, aAttr[ KEY_DSN].szAttr))
		{
		SQLRemoveDSNFromIni( szDSN);
// remove File Data Source Name
		DeleteFileDSN( szDSN );
		}

	return TRUE;
}

//=====================================================

#define MAX_INI_VAL 261

void DeleteFileDSN( LPCSTR lpszDSN )
{
	const char Key1[]  = "Software\\ODBC\\ODBC.INI\\ODBC File DSN";
	const char Name1[] = "DefaultDSNDir";

	const char Key2[]  = "Software\\Microsoft\\Windows\\CurrentVersion";
	const char Name2[] = "CommonFilesDir";

	char	szBuf[ MAXPATHLEN];
	CHAR    szIniVal[MAX_INI_VAL];
	LRESULT lResult;
	DWORD   dwType = 0;
	DWORD   cbData;
	HKEY	hkGlobal = NULL;
	int		rc = -1;

// registry
	lResult= RegOpenKeyEx(HKEY_LOCAL_MACHINE, Key1, 0, KEY_READ, &hkGlobal );
  
	if( lResult == ERROR_SUCCESS ){
		   
		cbData = MAX_INI_VAL;
		szIniVal[0] = '\0';

		lResult= RegQueryValueEx(hkGlobal, Name1, 0, &dwType, (LPBYTE)szIniVal, &cbData );
		
		RegCloseKey(hkGlobal);

		if( lResult == ERROR_SUCCESS ){  

			lstrcpy(szBuf, szIniVal );
			lstrcat(szBuf, "\\" );
			lstrcat(szBuf, lpszDSN );
			lstrcat(szBuf, " (not sharable).dsn");
			rc = _unlink(szBuf);

		}

	}
	if( rc == -1)
	{
		lResult= RegOpenKeyEx(HKEY_LOCAL_MACHINE, Key2, 0, KEY_READ, &hkGlobal );
  
		if( lResult == ERROR_SUCCESS ){
		   
			cbData = MAX_INI_VAL;
			szIniVal[0] = '\0';

			lResult= RegQueryValueEx(hkGlobal, Name2, 0, &dwType, (LPBYTE)szIniVal, &cbData );
		
			RegCloseKey(hkGlobal);

			if( lResult == ERROR_SUCCESS ){  

				lstrcpy(szBuf, szIniVal );
				lstrcat(szBuf, "\\ODBC\\Data Sources\\" );
				lstrcat(szBuf, lpszDSN );
				lstrcat(szBuf, " (not sharable).dsn");
				_unlink(szBuf);

			}

		}
	}

// registry end
}

void WriteTraceRegistry( const char szName[],char* szData)
{
	UWORD wConfigMode;

	SQLGetConfigMode(&wConfigMode);
	SQLSetConfigMode(ODBC_USER_DSN);
	SQLWritePrivateProfileString(szODBC,szName,szData, szODBCIni);
	SQLSetConfigMode(ODBC_SYSTEM_DSN);
	SQLWritePrivateProfileString(szODBC,szName,szData, szODBCIni);
	SQLSetConfigMode(wConfigMode);
/*
	HKEY	hkGlobal = NULL;
	DWORD	dwDisposition;

    if(ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, TRACE_PATH
		,0,(char*)szName,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hkGlobal,&dwDisposition ))
		return;
	
	RegSetValueEx( hkGlobal,szName,0,REG_SZ,(UCHAR*)szData,strlen(szData)+1);
	RegCloseKey(hkGlobal);
*/

}

void ReadTraceRegistry( const char szName[],char* szData, long size)
{
/*
	DWORD   dwType = 0;
	DWORD   cbData;
	HKEY	hkGlobal = NULL;

	if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER,TRACE_PATH,0,KEY_READ,&hkGlobal))
		return;
 
	cbData = size;
	*szData = 0;

	RegQueryValueEx(hkGlobal,szName,0,&dwType,(LPBYTE)szData,&cbData );
	RegCloseKey(hkGlobal);
*/
}

#define GET_DS_ALL "GET_DS_ALL"
typedef void* (* FPGET_DS_ALL)(char* objRef);

void* pDataSourceInfo = NULL;

void getDSInfo(char* Ip, char* port)
{
	char objref[MAXIPLEN+30];
	char Key[50]  = "Software\\ODBC\\ODBCINST.INI\\";
	const char Name[] = "Driver";
	HMODULE Handle = NULL;

	char	szBuf[ MAXPATHLEN];
	LRESULT lResult;
	DWORD   dwType = 0;
	DWORD   cbData;
	HKEY	hkGlobal = NULL;
	int		rc = -1;

	FPGET_DS_ALL fpGET_DS_ALL;

	wsprintf(objref, "%s%s/%s", TCP_STR, Ip, port);
// registry
	strcat(Key,lpszDrvr);

	lResult= RegOpenKeyEx(HKEY_LOCAL_MACHINE, Key, 0, KEY_READ, &hkGlobal );
  
	if( lResult == ERROR_SUCCESS ){
		   
		cbData = MAX_INI_VAL;
		szBuf[0] = '\0';

		lResult= RegQueryValueEx(hkGlobal, Name, 0, &dwType, (LPBYTE)szBuf, &cbData );
		
		RegCloseKey(hkGlobal);
/*
		if( lResult == ERROR_SUCCESS ){  

			if ((Handle = GetModuleHandle(szBuf)) != NULL || (Handle = LoadLibrary(szBuf)) != NULL )
			{
				fpGET_DS_ALL = (FPGET_DS_ALL)GetProcAddress( Handle, GET_DS_ALL);
				if (fpGET_DS_ALL != NULL)
					pDataSourceInfo = (fpGET_DS_ALL)(objref);
				else
					pDataSourceInfo = NULL;
			}
		}
*/
	}

}

void freeDSInfo()
{
	if (pDataSourceInfo != NULL)
	{
		delete pDataSourceInfo;
		pDataSourceInfo = NULL;
	}
}
static unsigned long index = 0;

static char* getDSName(unsigned long index)
{
	DATASOURCE_STATUS_LIST_def *ppDataSourceInfo = (DATASOURCE_STATUS_LIST_def*)pDataSourceInfo;
	if (index >= ppDataSourceInfo->_length)
		return "";
	DATASOURCE_STATUS_def *buffer = ppDataSourceInfo->_buffer + index;
	return buffer->DSName;
}

char* getFirstDSName()
{
	if (pDataSourceInfo != NULL)
	{
		index = 0;
		return getDSName(index++);
	}
	else
		return "";
}

char* getNextDSName()
{
	if (pDataSourceInfo != NULL)
		return getDSName(index++);
	else
		return "";
}

DATASOURCE_STATUS_def *getDSNameDetails(char* sDSName)
{
	DATASOURCE_STATUS_LIST_def* ppDataSourceInfo = (DATASOURCE_STATUS_LIST_def*)pDataSourceInfo;
	DATASOURCE_STATUS_def *buffer;
	
	if (pDataSourceInfo != NULL && ppDataSourceInfo->_buffer != NULL )
	{
		for (unsigned long index = 0; index < ppDataSourceInfo->_length; index++)
		{
			buffer = ppDataSourceInfo->_buffer + index;
			if (strcmp(buffer->DSName, sDSName) == 0)
			{
				return buffer;
			}
		}
	}
	return NULL;
}
