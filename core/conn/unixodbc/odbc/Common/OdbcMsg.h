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
#ifndef ODBCMSG
#define ODBCMSG

#ifdef _AFXDLL
#include <stdafx.h>
#else
#include <windows.h>
#include <string>
using namespace std;
#endif

#define DRVRMSG_DLL	1
#define SRVRMSG_DLL	2

// 9/9/98 Ramses
// When we implement functions to move Unicode data back we have 
// to define Unicode functions and Unicode message structure to 
// avoid any confusion.  Right now we only use Multibyte (or ANSI)
// based character strings.  This functions should be enough to deal with 
// client language settings.


// Message structure
class ODBCMXMSG_Def 
{
public:
	char	lpsSQLState[6];
#ifdef _AFXDLL
	CString lpsMsgText;
#else
	string	lpsMsgText;
#endif
};


class OdbcMsg {
public:
	OdbcMsg();
	OdbcMsg(int iDllType);
	OdbcMsg(int iDllType, DWORD LanguageId);
	~OdbcMsg();
	BOOL GetOdbcMessage (DWORD dwLanguageId, DWORD ErrCode, ODBCMXMSG_Def *MsgStruc, ...);
	DWORD GetMsgId(DWORD ErrCode);
	DWORD GetLanguageId ();
	void  SetLanguageId (DWORD LanguageId);

private:
	void FillMsgStructure (UINT ErrCode, LPSTR lpMsgBuf, ODBCMXMSG_Def *MsgStruc, int actual_len);
	void CleanUpMsgStructure(ODBCMXMSG_Def *MsgStruc);
	void ReportError(ODBCMXMSG_Def *MsgStruc, UINT MessageID, UINT error_code);
	void LoadMsgDll (int iDllType);

private:
	// global variables for the class
	BOOL		bMsgDLLLoaded;
	HMODULE		hMsgDll;
	DWORD		dwLanguageId;
	DWORD		dwACP;
	DWORD		dwLCID;
};


#endif
