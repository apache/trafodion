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
// UpdateDSN.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <shellapi.h>
#include <string>

bool DeleteThisFile(LPCTSTR lpszDir, bool noRecycleBin = true)
{
  int len = _tcslen(lpszDir);
  TCHAR *pszFrom = new TCHAR[len+2];
  _tcscpy(pszFrom, lpszDir);
  pszFrom[len] = 0;
  pszFrom[len+1] = 0;
  
  SHFILEOPSTRUCT fileop;
  fileop.hwnd   = NULL;    // no status display
  fileop.wFunc  = FO_DELETE;  // delete operation
  fileop.pFrom  = pszFrom;  // source file name as double null terminated string
  fileop.pTo    = NULL;    // no destination needed
  fileop.fFlags = FOF_NOCONFIRMATION|FOF_SILENT;  // do not prompt the user
  
  if(!noRecycleBin)
    fileop.fFlags |= FOF_ALLOWUNDO;

  fileop.fAnyOperationsAborted = FALSE;
  fileop.lpszProgressTitle     = NULL;
  fileop.hNameMappings         = NULL;

  int ret = SHFileOperation(&fileop);
  delete [] pszFrom;  
  return (ret == 0);
}


void CleanUp()
{
   DWORD ulOptions = 0;
   PHKEY phkResult;
   LONG  retVal;
   HKEY hKey;

#ifdef WIN64
   REGSAM samDesired = KEY_WOW64_32KEY | KEY_SET_VALUE;
#else
   REGSAM samDesired = KEY_WOW64_64KEY | KEY_SET_VALUE;
#endif

	TCHAR buffer[2048];
	std::string sysroot = getenv("SYSTEMROOT");
	std::string hpodbcdll   = sysroot + "\\system32\\trfodbc1.dll";
//	std::string OdbcTrace   = sysroot + "\\system32\\traf_OdbcTrace0100.dll";
	std::string tcpipv6     = sysroot + "\\system32\\traf_tcpipv60100.dll";
	std::string odbcDrvMsg  = sysroot + "\\system32\\traf_odbcDrvMsg_intl0100.dll";
	std::string translation = sysroot + "\\system32\\traf_translation01.dll";
	std::string hpoadm02    = sysroot + "\\system32\\trfoadm1.dll";
	std::string ores        = sysroot + "\\system32\\traf_ores0100.dll";
    mbstowcs(buffer,hpodbcdll.c_str(), sizeof(buffer));
	DeleteThisFile((LPCTSTR)buffer, false);
    mbstowcs(buffer,tcpipv6.c_str(), sizeof(buffer));
	DeleteThisFile((LPCTSTR)buffer, false);

    mbstowcs(buffer,odbcDrvMsg.c_str(), sizeof(buffer));
	DeleteThisFile((LPCTSTR)buffer, false);

    mbstowcs(buffer,translation.c_str(), sizeof(buffer));
	DeleteThisFile((LPCTSTR)buffer, false);

    mbstowcs(buffer,hpoadm02.c_str(), sizeof(buffer));
	DeleteThisFile((LPCTSTR)buffer, false);

    mbstowcs(buffer,ores.c_str(), sizeof(buffer));
	DeleteThisFile((LPCTSTR)buffer, false);

	retVal = RegOpenKeyEx(
				HKEY_LOCAL_MACHINE,
				(LPCWSTR)L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDlls",
				ulOptions,
				KEY_SET_VALUE,
				&hKey);
	retVal = RegDeleteValue(
				hKey,
				(LPCWSTR)L"C:\\WINDOWS\\system32\\trfodbc1.dll");

//	retVal = RegDeleteValue(
//				hKey,
//				(LPCWSTR)L"C:\\WINDOWS\\system32\\traf_OdbcTrace0100.dll");

	retVal = RegDeleteValue(
				hKey,
				(LPCWSTR)L"C:\\WINDOWS\\system32\\traf_tcpipv60100.dll");

	retVal = RegDeleteValue(
				hKey,
				(LPCWSTR)L"C:\\WINDOWS\\system32\\traf_odbcDrvMsg_intl0100.dll");

	retVal = RegDeleteValue(
				hKey,
				(LPCWSTR)L"C:\\WINDOWS\\system32\\traf_translation01.dll");

	retVal = RegDeleteValue(
				hKey,
				(LPCWSTR)L"C:\\WINDOWS\\system32\\trfoadm1.dll");

	retVal = RegDeleteValue(
				hKey,
				(LPCWSTR)L"C:\\WINDOWS\\system32\\traf_ores0100.dll");
}

int _tmain(int argc, _TCHAR* argv[])
{

	UWORD Mode,oldMode;
	SQLHANDLE m_henv;
	SQLUSMALLINT direction;
	SQLRETURN retVal = SQL_SUCCESS;
	SQLWCHAR DSNName[SQL_MAX_DSN_LENGTH + 1];
	SQLWCHAR Description[100];
	SQLSMALLINT nameLen, descLen;
	TCHAR NewDriver[1024];

	if(argc > 1 && strcmp((const char*)argv[1],"-cleanup"))
	{
		CleanUp();
		return 0;
	}

	// The rest of the code was to deal with the file name change from hpodbc_0200.dll to hpodbc02.dll that was
	// required for the move to MSI installer. (Installshield did not like file names > 8 letters)
	// For seaquest, since we're starting off with an 8 character driver name, we won't need this, atleast for now

//	SQLGetConfigMode(&oldMode);

//	GetSystemDirectory(NewDriver,sizeof(NewDriver));

//	if(argc > 1 && strcmp((const char*)argv[1],"-revert"))
//		wcscat_s(NewDriver,L"\\hp_odbc0200.dll");
//	else
//		wcscat_s(NewDriver,L"\\hpodbc02.dll");

//	Mode = ODBC_BOTH_DSN;
//	direction = SQL_FETCH_NEXT;

//	SQLSetConfigMode(Mode);

//	retVal = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv);
//	retVal = SQLSetEnvAttr(m_henv, SQL_ATTR_ODBC_VERSION,(SQLPOINTER)SQL_OV_ODBC3, 0);
	

	//
	// Loop through all of the HP ODBC 2.0 DataSources, modifying the Driver= entry
	//

//	retVal = SQLDataSources(m_henv, direction, DSNName,	sizeof(DSNName),
//							&nameLen, Description, sizeof(Description),
//							&descLen);

//	while (retVal == SQL_SUCCESS || retVal == SQL_SUCCESS_WITH_INFO)
//	{
//		if (wcscmp(L"HP ODBC 2.0", Description) == 0)
//			SQLWritePrivateProfileString((LPCWSTR)DSNName,L"Driver",NewDriver,L"ODBC.INI");

//		retVal = SQLDataSources(m_henv, SQL_FETCH_NEXT, DSNName,
//								sizeof(DSNName), &nameLen,
//								Description, sizeof(Description), &descLen);
//	}


//	SQLSetConfigMode(oldMode);

	return 0;

}
