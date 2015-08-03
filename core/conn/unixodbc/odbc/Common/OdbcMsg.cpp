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
#include "odbcMsg.h"
#include "nlsfunctions.h"

#define DRVRMSGDLL_NAME	"tdm_odbcDrvMsg_intl0200.DLL"
#define SRVRMSGDLL_NAME "tdm_odbcSrvrMsg_intl0200.DLL"
#define MSG_NOT_FOUND "Message for ID[%d] not found"
#define MSG_INCOMPLETE "Message for ID[%d] is incomplete"
#define GENERAL_SQLSTATE	"HY000"

OdbcMsg::OdbcMsg()
//************************************************************************
//*
//*   Function: OdbcMsg
//*
//*   Input Params: None
//*
//*   Output Params: None
//*
//*   Description: It will load the Server Messages DLL by default
//*					and it will use a neutral language (based on system settings)
//*
//************************************************************************
{
	OdbcMsg(SRVRMSG_DLL);
}

OdbcMsg::OdbcMsg(int iDllType)
//************************************************************************
//*
//*   Function: OdbcMsg
//*
//*   Input Params: type of DLL to load; 
//*
//*   Output Params: None
//*
//*   Description:	This function will load the given DLL with a default
//*					locale setting for the computer
//*
//************************************************************************
{

	// get locale information for language and code pages
	ODBCNLS_GetErrorLanguage (&dwLanguageId);
	
	// the following is required only when we perform Unicode conversions
	ODBCNLS_GetCodePage(&dwACP);

	// load the message DLL
	LoadMsgDll(iDllType);
}

OdbcMsg::OdbcMsg(int iDllType, DWORD dwLangId)
//************************************************************************
//*
//*   Function:OdbcMsg
//*
//*   Input Params: iDllType - Type of Message Dll to load
//*					LanguageId - Language ID definition (from the client)
//*
//*   Output Params: None
//*
//*   Description:	It will try to load the Message DLL store the recommended
//*					locale language to be used
//*
//************************************************************************
{
	// valid values for the language are generated through the MAKELANGID macro
	// and the provided LANG_*, SUBLANG_* literals
	dwLanguageId = dwLangId;
	
	// to get parameters for Unicode conversions
	ODBCNLS_GetCodePage(&dwACP);

	LoadMsgDll (iDllType);
}

OdbcMsg::~OdbcMsg()
//************************************************************************
//*
//*   Function: ~OdbcMsg
//*
//*   Input Params: None
//*
//*   Output Params: None
//*
//*   Description: It will try to free the Message Dll if loaded
//*
//************************************************************************
{
	// if library was loaded, attempt to free the library
	if (hMsgDll)
		FreeLibrary(hMsgDll);
}


BOOL OdbcMsg::GetOdbcMessage (DWORD dwLangId, DWORD ErrCode, ODBCMXMSG_Def *MsgStruc, ...)
//************************************************************************
//*
//*   Function:  GetOdbcMessage
//*
//*   Input Params: ErrCode - the Message ID to look for in the message file
//*
//*   Output Params: MsgStruc - The Message text components
//*
//*   Description:  It will look for the message in the messages file (if loaded).
//*					Valid values for the language are generated through the MAKELANGID 
//*					macro and the provided LANG_*, SUBLANG_* literals defined in WinNT.h; 
//*					to use the pre-defined value the user can retrieve it using the 
//*					GetLanguageId function in this class.
//************************************************************************
{
	UINT	actual_len;
	DWORD	error_code;
	LPTSTR	lpMsgBuf;
	va_list	Arguments;

	va_start( Arguments, MsgStruc);
	lpMsgBuf = NULL;

	// clean up message structure
	CleanUpMsgStructure(MsgStruc);
	if (bMsgDLLLoaded)
	{
		// Load string from the DLL
		// if the function cannot find a message for the LANGID specified,
		// it returns ERROR_RESOURCE_LANG_NOT_FOUND; if we pass 0, it will look for
		// a message in te following order:
		//	1) Language Neutral
		//  2) Thread LANGID
		//  3) user default LANGID, based on user locale (be aware that it could be the server)
		//  4) system default LANGID
		//  5) US English

		actual_len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_HMODULE,
			hMsgDll,
			ErrCode,
			LANGIDFROMLCID(dwLangId),
			(LPTSTR) &lpMsgBuf,
			0,
			&Arguments	// va_list
			);
		error_code = GetLastError();
		if ((actual_len == 0) && (error_code == ERROR_RESOURCE_LANG_NOT_FOUND))
		{
			// deallocate previously allocated buffer
			if (lpMsgBuf)
				LocalFree( lpMsgBuf );

			// if the specified language is not there, try english as default
			actual_len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_HMODULE,
				hMsgDll,
				ErrCode,
				MAKELANGID (LANG_ENGLISH, SUBLANG_NEUTRAL),
				(LPTSTR) &lpMsgBuf,
				0,
				&Arguments	// va_list
				);
			error_code = GetLastError();
			if ((actual_len == 0) && 
				(error_code == ERROR_RESOURCE_LANG_NOT_FOUND) &&
				(dwLangId != 0)) // lang id = 0 already tested before
			{
				// deallocate previously allocated buffer
				if (lpMsgBuf)
					LocalFree( lpMsgBuf );

				// try the default option for FormatMessage
				actual_len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_HMODULE,
					hMsgDll,
					ErrCode,
					0,	// to go through all default four options again
					(LPTSTR) &lpMsgBuf,
					0,
					&Arguments	// va_list
					);
				if (actual_len == 0) // cannot get the message, maybe is not there
				{
					ReportError (MsgStruc, ErrCode, GetLastError());
				}
				else
				{
					// we got the message finally
					FillMsgStructure(ErrCode, (LPSTR)lpMsgBuf, MsgStruc, actual_len);
				}
			}
			else if (actual_len > 0)
			{
				// we got something, let's return it
				FillMsgStructure(ErrCode, (LPSTR)lpMsgBuf, MsgStruc, actual_len);
			}
			else
			{
				// this is an error
				ReportError (MsgStruc, ErrCode, GetLastError());
			}
		}
		else if (actual_len > 0)
		{
			// we got a message, return it
			FillMsgStructure(ErrCode, (LPSTR)lpMsgBuf, MsgStruc, actual_len);
		}
		else
		{
			// it is trully and error
			ReportError (MsgStruc, ErrCode, GetLastError());
		}
		// deallocate previously allocated buffer
		if (lpMsgBuf)
			LocalFree( lpMsgBuf );
	}
	else // Message DLL is not loaded
	{
		ReportError (MsgStruc, ErrCode, 0) ;
		return FALSE;
	}
	// if successfull, return TRUE
	return TRUE;
}

void OdbcMsg::FillMsgStructure (UINT ErrCode, 
								LPSTR lpMsgBuf, 
								ODBCMXMSG_Def *MsgStruc, 
								int actual_len)
//************************************************************************
//*
//*   Function:	FillMsgStructure
//*
//*   Input Params: lpMsgBuf - The original message buffer
//*
//*   Output Params: MsgStruc - will contain all message components
//*
//*   Description:  It will extract the message components based on the
//*					columns SQLState, and Message Text.
//*					it is assumed that the message structure is already clean
//*
//************************************************************************
{


	char	tempStr[512];
	char	*szTmp;

	// verify that the message is long enough, otherwise no clean message to return
	if (actual_len >= 5)
	{
			
		// from character 1 to character 5 is the SQLCode
		strncpy(MsgStruc->lpsSQLState, lpMsgBuf, min(5, sizeof(MsgStruc->lpsSQLState)-1));

		if (actual_len > 13) 
		{
			// then we may have the help ID plus something for message text
			// from character 6 to character 10 is the Help ID
			// from character 12 on is the message text

			szTmp = &lpMsgBuf[11];
		}
		else
		{
			// we don't have enough text to return
			sprintf(tempStr, MSG_INCOMPLETE, GetMsgId(ErrCode));
			szTmp = tempStr;
		}
	}
	else
	{
		// return general SQLCode and error message
		strcpy(MsgStruc->lpsSQLState, GENERAL_SQLSTATE);
		sprintf(tempStr, MSG_INCOMPLETE, GetMsgId(ErrCode));
		szTmp = tempStr;
	}

	MsgStruc->lpsMsgText = szTmp;
	
}


//************************************************************************
//*
//*   Function:	CleanUpMsgStructure
//*
//*   Input Params: MsgStruc - The original message structure
//*
//*   Output Params: MsgStruc - will contain empty message buffers
//*
//*   Description:  Empty message buffers
//*
//************************************************************************
void OdbcMsg::CleanUpMsgStructure (ODBCMXMSG_Def * MsgStruc)
{
	memset (MsgStruc->lpsSQLState, '\0', sizeof (MsgStruc->lpsSQLState));
}

//************************************************************************
//*
//*   Function:	ReportError
//*
//*   Input Params: MsgStruc - The original message structure
//*
//*   Output Params: MsgStruc - will contain a default error message with data
//*
//*   Description:  Empty message buffers
//*
//************************************************************************
void OdbcMsg::ReportError (ODBCMXMSG_Def * MsgStruc, UINT MessageId, UINT error_code)
{
	char	tempStr[512];
	LPVOID	lpMsgBuf;
#ifdef _DEBUG
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error_code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL);
	// printf ("Error:[%s]\n", lpMsgBuf);
	LocalFree( lpMsgBuf );
#endif
	// use default error message (with message file message ID)
	strcpy(MsgStruc->lpsSQLState, GENERAL_SQLSTATE);
	sprintf(tempStr, MSG_NOT_FOUND, GetMsgId(MessageId));

	MsgStruc->lpsMsgText = tempStr;
	return ;
}

//************************************************************************
//*
//*   Function:	LoadMsgDll
//*
//*   Input Params: identifier of the MsgDLL to be loaded
//*
//*   Output Params: none
//*
//*   Description:  It loads the specified message DLL and sets the
//*					appropriate state variables
//*
//************************************************************************
void OdbcMsg::LoadMsgDll (int iDllType)
{
	bMsgDLLLoaded = FALSE;

	// attempt to load the library
	switch (iDllType)
	{
		case DRVRMSG_DLL:
			hMsgDll = LoadLibrary(DRVRMSGDLL_NAME);
			break;

		case SRVRMSG_DLL:
			hMsgDll = LoadLibrary(SRVRMSGDLL_NAME);
			break;

		default:
			// loads the server DLL by default
			hMsgDll = LoadLibrary(SRVRMSGDLL_NAME);

	}
	if (hMsgDll != NULL)
	{
		bMsgDLLLoaded = TRUE;
	}
}

//************************************************************************
//*
//*   Function:	GetMsgId
//*
//*   Input Params: Error code as stored in the message header file
//*
//*   Output Params: none
//*
//*   Description:  It returns the Error code as assigned in the message file
//*					(removes application name, flags, etc)
//*
//************************************************************************
DWORD OdbcMsg::GetMsgId (DWORD ErrCode)
{
	return ErrCode  & 0x0FFFF;
}


void OdbcMsg::SetLanguageId (DWORD LanguageId)
{
	dwLanguageId = LanguageId;
	// set the internal language to something we handle
	ODBCNLS_ValidateLanguage (&dwLanguageId);
}

DWORD OdbcMsg::GetLanguageId ()
{
	return dwLanguageId;
}