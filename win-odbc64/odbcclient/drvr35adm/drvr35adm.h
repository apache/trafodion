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
// Drvr35Adm.h : main header file for the DRVR35ADM DLL
//

#if !defined(AFX_DRVR35ADM_H__E21E4AF0_D9A1_11D3_ABEF_080009DC95E5__INCLUDED_)
#define AFX_DRVR35ADM_H__E21E4AF0_D9A1_11D3_ABEF_080009DC95E5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"				// main symbols
#define MAXPATHLEN      (260+1)		// Max path length
#define MAX_INI_VAL		261
#define MAXIPLEN        (90+1)		// Max length for IP Address
#define MAXPORTLEN      (5+1)		// Max length for port number
#define MAXDSNAME       (32+1)		// Max data source name length
#define MAXKEYLEN       (32+1)		// Max keyword length
#define MAXDESC         (255+1)		// Max description length
#define MAXCATLEN       (128+1)		// Max length for catalog (or schema)
#define MAXSN			(20+1)

// Attribute key indexes (into an array of Attr structs, see below)
#define KEY_DSN					0
#define KEY_DESC				1
#define KEY_CATALOG				2
#define KEY_SCHEMA				3
#define KEY_LOCATION			4
#define KEY_LOGIN				5
#define KEY_CONNECTION			6
#define KEY_QUERY				7
#define KEY_IPADDRESS			8
#define KEY_PORTNUM				9
#define KEY_ERRORLANG			10
#define KEY_DATALANG			11
#define KEY_TRACE_FLAGS			12
#define KEY_TRACE_FILE			13
#define KEY_TRANSLATION_DLL		14
#define KEY_TRANSLATION_OPTION	15
#define KEY_FETCH_BUFFER_SIZE	16
#define KEY_SERVICE_NAME		17
#define KEY_REPLACEMENT_CHAR	18
#define KEY_COMPRESSION			19
#define NUMOFKEYS				20	// Number of keys supported


// ODBC.INI keywords
const char ODBC_INI[]				= "ODBC.INI";		// ODBC initialization file
const char INI_KDESC[]				= "Description";	// Data source description
const char INI_KSN[]				= "ServiceName";	// Service Name
const char INI_CATALOG[]			= "Catalog";		// Second option
const char INI_SCHEMA[]				= "Schema";		    // Schema string
const char INI_LOCATION[]			= "Location";		// Location string
const char INI_NETWORK[]			= "Server";
const char INI_LOGIN[]				= "SQL_LOGIN_TIMEOUT";
const char INI_CONNECTION[]			= "SQL_ATTR_CONNECTION_TIMEOUT";
const char INI_QUERY[]				= "SQL_QUERY_TIMEOUT";
const char INI_SDEFAULT[]			= "Default";		// Default data source name
const char TCP_STR[]				= "TCP:";           // For Association Server
const char INI_ERRORLANG[]			= "ErrorMsgLang";	// Language ID (LCID) for error messages
const char INI_DATALANG[]			= "DataLang";		// Language ID (LCID) for SQL data
const char INI_TRACE_FLAGS[]		= "TraceFlags";
const char INI_TRACE_FILE[]			= "TraceFile";
const char INI_TRANSLATION_DLL[]	= "TranslationDLL";
const char INI_TRANSLATION_OPTION[] = "TranslationOption";
const char INI_FETCH_BUFFER_SIZE[]  = "FetchBufferSize";
const char INI_REPLACEMENT_CHAR[]	= "ReplacementCharacter";
const char INI_COMPRESSION[]		= "Compression";

const char TRACE_PATH[] = "Software\\ODBC\\ODBC.INI\\ODBC";
const char DRIVER_NAME[] = "TRAF ODBC 2.3";
const char CREATE_NEW_DSN[] = "Create a New Trafodion ODBC Data Source to Trafodion Database";
const char CONFIGURE_DSN1[] = "Trafodion ODBC Data Source '";
const char DRVR_MSG_DLL[] = "traf_odbcDrvMsg_intl0100.dll";
const char CONFIGURE_DSN2[] = "' Configuration";

const char	szDefaultTraceFlags[]=  		"0";
const char	szDefaultTraceFile[]=			"C:\\ODBC.LOG";
const char	szODBC[]=						"ODBC";
const char	szODBCIni[]=					"ODBC.INI";


struct ATTR
{
	BOOL	fSupplied;
	char	szAttr[ MAXPATHLEN];
};
	
typedef struct ATTR			Attr;
typedef Attr FAR *			LPAttr;

extern char szDSN[];
extern HWND hwndParent;
extern LPCSTR lpszDrvr;
extern BOOL bNewDSN;
extern LPAttr  aAttr;
/////////////////////////////////////////////////////////////////////////////
// CDrvr35AdmApp
// See Drvr35Adm.cpp for the implementation of this class
//

class CDrvr35AdmApp : public CWinApp
{
public:
	CDrvr35AdmApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDrvr35AdmApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CDrvr35AdmApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// Function Prototypes
void WriteTraceRegistry( const char szName[],char* szData);
void ReadTraceRegistry( const char szName[],char* szData,long size);
BOOL SetDSNAttributes(HWND    hwnd, LPCSTR  lpszDrvr, BOOL  bNewDSN);
void DeleteFileDSN( LPCSTR lpDSName );

#define IDL_PTR_SIZE 4
#define IDL_PTR_PAD(name, count)  char name##pad_[8 - IDL_PTR_SIZE] [count];

typedef char IDL_char;
typedef short IDL_short;
typedef long IDL_long;
typedef unsigned long IDL_unsigned_long;
typedef IDL_long IDL_enum;
typedef IDL_char SQL_IDENTIFIER_def[129];
typedef IDL_enum DS_AUTOMATION_t;
typedef DS_AUTOMATION_t DS_AUTOMATION_def;
typedef IDL_long TIME_def;

struct DATASOURCE_STATUS_t {
  SQL_IDENTIFIER_def DSName;
  char pad_to_offset_132_[3];
  DS_AUTOMATION_def DSAutomation;
  IDL_short defaultFlag;
  char pad_to_offset_140_[2];
  IDL_long DSState;
  IDL_long MaxSrvrCnt;
  IDL_long InitSrvrCnt;
  IDL_long AvailSrvrCnt;
  IDL_long StartAheadCnt;
  IDL_long CurrentSrvrRegistered;
  IDL_long CurrentSrvrConnected;
  TIME_def StateChangeTime;
};
typedef struct DATASOURCE_STATUS_t DATASOURCE_STATUS_def;
typedef struct DATASOURCE_STATUS_LIST_def_seq_ {
  IDL_unsigned_long _length;
  char pad_to_offset_8_[4];
  DATASOURCE_STATUS_def *_buffer;
  IDL_PTR_PAD(_buffer, 1)
} DATASOURCE_STATUS_LIST_def;

void getDSInfo(char* Ip, char* port);
void freeDSInfo();
char* getFirstDSName();
char* getNextDSName();
DATASOURCE_STATUS_def *getDSNameDetails(char* sDSName);

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DRVR35ADM_H__E21E4AF0_D9A1_11D3_ABEF_080009DC95E5__INCLUDED_)
