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
/*************************************************************************
**************************************************************************/
//
#include "neofunc.h"
#include "cenv.h"
#include "cconnect.h"
#include "cstmt.h"
#include "cdesc.h"
#include "dmfunctions.h"
#include "drvrmanager.h"
#include "dminstall.h"

using namespace ODBC;

UWORD configMode = ODBC_BOTH_DSN;
UWORD wSystemDSN = USERDSN_ONLY;

DWORD ierror[ERROR_NUM] = {0};
LPSTR errormsg[ERROR_NUM] = {0};
SWORD numerrors = -1;
LPSTR errortable[] = {
	"",
	"General installer error",
	"Invalid buffer length",
	"Invalid window handle",
	"Invalid string parameter",
	"Invalid type of request",
	"Component not found",
	"Invalid name parameter",
	"Invalid keyword-value pairs",
	"Invalid DSN",
	"Invalid .INF file",
	"Request failed",
	"Invalid install path.",
	"Could not load the driver or translator setup library",
	"Invalid parameter sequence",
	"Invalid log file name.",
	"Operation canceled on user request",
	"Could not increment or decrement the component usage count",
	"Creation of the DSN failed",
	"Error during writing system information",
	"Deletion of the DSN failed",
	"Out of memory",
	"Output string truncated due to a buffer not large enough",
};

BOOL INSTAPI SQLConfigDataSource(
     HWND hwndParent,
     WORD fRequest,
     LPCSTR lpszDriver,
     LPCSTR lpszAttributes)
{
	BOOL retcode = FALSE;
	/* Check input parameters */
	CLEAR_ERROR ();

	/* Map the request User/System */
	switch (fRequest)
    {
		case ODBC_ADD_DSN:
		case ODBC_CONFIG_DSN:
		case ODBC_REMOVE_DSN:
			configMode = ODBC_USER_DSN;
			break;
		case ODBC_ADD_SYS_DSN:
		case ODBC_CONFIG_SYS_DSN:
		case ODBC_REMOVE_SYS_DSN:
			configMode = ODBC_SYSTEM_DSN;
			fRequest = fRequest - ODBC_ADD_SYS_DSN + ODBC_ADD_DSN;
			break;
		case ODBC_REMOVE_DEFAULT_DSN:
			retcode = RemoveDefaultDataSource ();
			goto resetdsnmode;
			break;
		default:
			PUSH_ERROR (ODBC_ERROR_INVALID_REQUEST_TYPE);
			goto resetdsnmode;
			break;
    };
	if (!lpszDriver || !strlen (lpszDriver))
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_NAME);
    }
	/* Error : ConfigDSN could no be found */
	PUSH_ERROR (ODBC_ERROR_LOAD_LIB_FAILED);
resetdsnmode:
	wSystemDSN = USERDSN_ONLY;
	configMode = ODBC_BOTH_DSN;
	return retcode;
}

BOOL INSTAPI SQLConfigDriver(
     HWND hwndParent,
     WORD fRequest,
     LPCSTR lpszDriver,
     LPCSTR lpszArgs,
     LPSTR lpszMsg,
     WORD cbMsgMax,
     WORD *pcbMsgOut)
{
	/* Check input parameters */
	CLEAR_ERROR ();
	if (!lpszDriver || !strlen (lpszDriver))
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_NAME);
    }
	/* Map the request User/System */
	if (fRequest < ODBC_INSTALL_DRIVER || fRequest > ODBC_CONFIG_DRIVER_MAX)
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_REQUEST_TYPE);
    }
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	return FALSE;
}

BOOL INSTAPI SQLCreateDataSource(
     HWND hwnd,
     LPSTR lpszDSN)
{
	/* Check input parameters */
	CLEAR_ERROR ();
	if (!hwnd)
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_HWND);
		return FALSE;
    }
	else if (!lpszDSN)
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_DSN);
		return FALSE;
    }
	else if ( !ValidDSN (lpszDSN) || !strlen (lpszDSN))
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_DSN);
		return FALSE;
    }
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	return FALSE;
}

BOOL INSTAPI SQLGetAvailableDrivers (
	LPCSTR lpszInfFile, 
	LPSTR lpszBuf, 
	WORD cbBufMax,
    WORD *pcbBufOut)
{
	CLEAR_ERROR ();
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	return FALSE;
}

BOOL INSTAPI SQLGetConfigMode (UWORD *pwConfigMode)
{
	BOOL retcode = FALSE;
	/* Clear errors */
	CLEAR_ERROR ();

	/* Check input parameter */
	if (!pwConfigMode)
    {
		PUSH_ERROR (ODBC_ERROR_OUT_OF_MEM);
    }
	else
    {
		*pwConfigMode = configMode;
		retcode = TRUE;
    }
	return retcode;
}

BOOL INSTAPI SQLGetInstalledDrivers (
		LPSTR lpszBuf, 
		WORD cbBufMax, 
		WORD *pcbBufOut)
{
	CLEAR_ERROR ();
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	return FALSE;
}

int INSTAPI SQLGetPrivateProfileString (
		LPCSTR lpszSection, 
		LPCSTR lpszEntry,
		LPCSTR lpszDefault, 
		LPSTR lpszRetBuffer, 
		int cbRetBuffer,
		LPCSTR lpszFilename)
{
	char pathbuf[1024];
	int len = 0;

	/* Check input parameters */
	CLEAR_ERROR ();
	if (!lpszRetBuffer || !cbRetBuffer)
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
		goto quit;
    }
	if (!lpszDefault)
    {
		PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
		goto quit;
    }
	/* Else go through user/system odbc.ini */
	switch (configMode)
    {
		case ODBC_USER_DSN:
			wSystemDSN = USERDSN_ONLY;
			if (lpszFilename)
			{
				len = GetPrivateProfileString (lpszSection, lpszEntry, lpszDefault,
						lpszRetBuffer, cbRetBuffer, lpszFilename);
				goto quit;
			}
			if (fun_getinifile (pathbuf, sizeof (pathbuf), FALSE, FALSE))
				len = GetPrivateProfileString (lpszSection, lpszEntry, lpszDefault,
						lpszRetBuffer, cbRetBuffer, pathbuf);
			goto quit;
		case ODBC_SYSTEM_DSN:
			wSystemDSN = SYSTEMDSN_ONLY;
			if (lpszFilename)
			{
				len = GetPrivateProfileString (lpszSection, lpszEntry, lpszDefault,
						lpszRetBuffer, cbRetBuffer, lpszFilename);
				goto quit;
			}
			if (fun_getinifile (pathbuf, sizeof (pathbuf), FALSE, FALSE))
				len = GetPrivateProfileString (lpszSection, lpszEntry, lpszDefault,
						lpszRetBuffer, cbRetBuffer, pathbuf);
			goto quit;
		case ODBC_BOTH_DSN:
			wSystemDSN = USERDSN_ONLY;
			if (lpszFilename)
			{
				len = GetPrivateProfileString (lpszSection, lpszEntry, lpszDefault,
						lpszRetBuffer, cbRetBuffer, lpszFilename);
				if (!len)
				{
					CLEAR_ERROR ();
					wSystemDSN = SYSTEMDSN_ONLY;
					len = GetPrivateProfileString (lpszSection, lpszEntry,
							lpszDefault, lpszRetBuffer, cbRetBuffer, lpszFilename);
				}
				goto quit;
			}
			if (fun_getinifile (pathbuf, sizeof (pathbuf), FALSE, FALSE))
				len = GetPrivateProfileString (lpszSection, lpszEntry, lpszDefault,
						lpszRetBuffer, cbRetBuffer, pathbuf);
			else
			{
				CLEAR_ERROR ();
				wSystemDSN = SYSTEMDSN_ONLY;
				if (fun_getinifile (pathbuf, sizeof (pathbuf), FALSE, FALSE))
					len = GetPrivateProfileString (lpszSection, lpszEntry, lpszDefault,
							lpszRetBuffer, cbRetBuffer, pathbuf);
			}
			goto quit;
	};
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	goto quit;
quit:
	wSystemDSN = USERDSN_ONLY;
	configMode = ODBC_BOTH_DSN;
	return len;
}

#ifndef unixcli
BOOL INSTAPI SQLGetTranslator (
    HWND hwnd,
    LPSTR lpszName,
    WORD cbNameMax,
    WORD FAR * pcbNameOut,
    LPSTR lpszPath,
    WORD cbPathMax,
    WORD FAR * pcbPathOut,
    DWORD FAR * pvOption)
#else
BOOL INSTAPI SQLGetTranslator (
    HWND hwnd,
    LPSTR lpszName,
    WORD cbNameMax,
    WORD * pcbNameOut,
    LPSTR lpszPath,
    WORD cbPathMax,
    WORD * pcbPathOut,
    DWORD * pvOption)
#endif
{
	/* Check input parameters */
	CLEAR_ERROR ();
	if (!hwnd)
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_HWND);
    }
	if (!lpszName || !lpszPath || cbNameMax < 1 || cbPathMax < 1)
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
    }
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	return FALSE;
}

BOOL INSTAPI SQLInstallDriver (
	LPCSTR lpszInfFile, 
	LPCSTR lpszDriver, 
	LPSTR lpszPath,
    WORD cbPathMax, 
	WORD *pcbPathOut)
{
	/* Check input parameters */
	CLEAR_ERROR ();
	if (!lpszDriver || !strlen (lpszDriver))
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_PARAM_SEQUENCE);
    }
	if (!lpszPath || !cbPathMax)
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
	}
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	return FALSE;
}

BOOL INSTAPI SQLInstallDriverEx (
		LPCSTR lpszDriver, 
		LPCSTR lpszPathIn, 
		LPSTR lpszPathOut,
		WORD cbPathOutMax, 
		WORD *pcbPathOut, 
		WORD fRequest, 
		LPDWORD lpdwUsageCount)
{
	CLEAR_ERROR ();

	switch (fRequest)
    {
		case ODBC_INSTALL_INQUIRY:
		case ODBC_INSTALL_COMPLETE:
			break;
		default:
			PUSH_ERROR (ODBC_ERROR_INVALID_REQUEST_TYPE);
    };
	/* Check input parameters */
	if (!lpszDriver || !strlen (lpszDriver))
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_PARAM_SEQUENCE);
    }
	if (!lpszPathOut || !cbPathOutMax)
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
    }
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	return FALSE;
}

BOOL INSTAPI SQLInstallDriverManager (
		LPSTR lpszPath, 
		WORD cbPathMax, 
		WORD *pcbPathOut)
{
	/* Check input parameters */
	CLEAR_ERROR ();
	if (!lpszPath || !cbPathMax)
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
    }
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	return FALSE;
}

RETCODE INSTAPI SQLInstallerError (
		WORD iError, 
		DWORD *pfErrorCode, 
		LPSTR lpszErrorMsg,
		WORD cbErrorMsgMax, 
		WORD * pcbErrorMsg)
{
	LPSTR message;
	RETCODE retcode = SQL_ERROR;

	/* Check if the index is valid to retrieve an error */
	if ((iError - 1) > numerrors)
    {
		retcode = SQL_NO_DATA;
		goto quit;
    }
	if (!lpszErrorMsg || !cbErrorMsgMax)
		goto quit;
	lpszErrorMsg[cbErrorMsgMax - 1] = 0;
	/* Copy the message error */
	message = (errormsg[iError - 1]) ? errormsg[iError - 1] : errortable[ierror[iError - 1]];
	if (strlen (message) >= cbErrorMsgMax - 1)
    {
		strncpy (lpszErrorMsg, message, cbErrorMsgMax - 1);
		retcode = SQL_SUCCESS_WITH_INFO;
		goto quit;
    }
	else
		strcpy (lpszErrorMsg, message);
	if (pfErrorCode)
		*pfErrorCode = ierror[iError - 1];
	if (pcbErrorMsg)
		*pcbErrorMsg = strlen (lpszErrorMsg);
	retcode = SQL_SUCCESS;
quit:
	return retcode;
}

BOOL INSTAPI SQLInstallODBC (
	HWND hwndParent, 
	LPCSTR lpszInfFile, 
	LPCSTR lpszSrcPath,
    LPCSTR lpszDrivers)
{
	/* Check input parameters */
	CLEAR_ERROR ();
	if (!lpszDrivers || !strlen (lpszDrivers))
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_NAME);
    }
	if (!lpszInfFile || !strlen (lpszInfFile))
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_INF);
    }
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	return FALSE;
}

BOOL INSTAPI SQLInstallTranslator (
		LPCSTR lpszInfFile, 
		LPCSTR lpszTranslator,
		LPCSTR lpszPathIn, 
		LPSTR lpszPathOut, 
		WORD cbPathOutMax,
		WORD *pcbPathOut, 
		WORD fRequest, 
		LPDWORD lpdwUsageCount)
{
	/* Check input parameters */
	CLEAR_ERROR ();
	switch (fRequest)
    {
		case ODBC_INSTALL_INQUIRY:
		case ODBC_INSTALL_COMPLETE:
			break;
		default:
			PUSH_ERROR (ODBC_ERROR_INVALID_REQUEST_TYPE);
    };
	/* Check input parameters */
	if (!lpszTranslator || !strlen (lpszTranslator))
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_PARAM_SEQUENCE);
    }
	if (!lpszPathOut || !cbPathOutMax)
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
    }
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	return FALSE;
}

BOOL INSTAPI SQLInstallTranslatorEx (
		LPCSTR lpszTranslator, 
		LPCSTR lpszPathIn,
		LPSTR lpszPathOut, 
		WORD cbPathOutMax, 
		WORD *pcbPathOut, 
		WORD fRequest,
		LPDWORD lpdwUsageCount)
{
	CLEAR_ERROR ();
	switch (fRequest)
    {
		case ODBC_INSTALL_INQUIRY:
		case ODBC_INSTALL_COMPLETE:
			break;
		default:
			PUSH_ERROR (ODBC_ERROR_INVALID_REQUEST_TYPE);
    };
	/* Check input parameters */
	if (!lpszTranslator || !strlen (lpszTranslator))
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_PARAM_SEQUENCE);
    }
	if (!lpszPathOut || !cbPathOutMax)
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
    }
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	return FALSE;
}

BOOL INSTAPI SQLManageDataSources (HWND hwndParent)
{
	/* Check input parameters */
	CLEAR_ERROR ();
	if (!hwndParent)
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_HWND);
    }
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	return FALSE;
}

RETCODE INSTAPI SQLPostInstallerError (
		DWORD fErrorCode, 
		LPSTR szErrorMsg)
{
	RETCODE retcode = SQL_ERROR;

	/* Check if the index is valid to retrieve an error */
	if (fErrorCode < ODBC_ERROR_GENERAL_ERR || fErrorCode > ODBC_ERROR_DRIVER_SPECIFIC)
		goto quit;
	if (numerrors < ERROR_NUM)
    {
		ierror[++numerrors] = fErrorCode;
		errormsg[numerrors] = szErrorMsg;;
    }
	retcode = SQL_SUCCESS;
quit:
	return retcode;
}

BOOL INSTAPI SQLReadFileDSN (
		LPCSTR lpszFileName, 
		LPCSTR lpszAppName, 
		LPCSTR lpszKeyName,
		LPSTR lpszString, 
		WORD cbString, 
		WORD *pcbString)
{
	BOOL retcode = FALSE;
	WORD len = 0, i;

	/* Check input parameters */
	CLEAR_ERROR ();
	if (!lpszString || !cbString)
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
		goto quit;
    }
	if (!lpszAppName && lpszKeyName)
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_REQUEST_TYPE);
		goto quit;
    }
	/* Is a file is specified */
	if (lpszFileName)
    {
		len = GetPrivateProfileString (lpszAppName, lpszKeyName, "", lpszString, cbString, lpszFileName);
		if (numerrors == -1) retcode = TRUE;
		goto quit;
    }
	PUSH_ERROR (ODBC_ERROR_INVALID_PATH);
	goto quit;
quit:
	for (i = 0; i < len; i++)
		if (!lpszString[i]) lpszString[i] = ';';
	if (pcbString) *pcbString = len;
	if (len == cbString - 1)
    {
		PUSH_ERROR (ODBC_ERROR_OUTPUT_STRING_TRUNCATED);
		retcode = FALSE;
    }
	return retcode;
}

BOOL INSTAPI SQLRemoveDefaultDataSource (void)
{
  return SQLConfigDataSource (NULL, ODBC_REMOVE_DEFAULT_DSN, NULL, NULL);
}

BOOL INSTAPI SQLRemoveDriver (
		LPCSTR lpszDriver, 
		BOOL fRemoveDSN, 
		LPDWORD lpdwUsageCount)
{
	/* Check input parameters */
	CLEAR_ERROR ();
	if (!lpszDriver || !strlen (lpszDriver))
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_NAME);
    }
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	return FALSE;
}

BOOL INSTAPI SQLRemoveDriverManager (LPDWORD lpdwUsageCount)
{
	CLEAR_ERROR ();
	return TRUE;
}

BOOL INSTAPI SQLRemoveDSNFromIni (LPCSTR lpszDSN)
{
	BOOL retcode = FALSE;

	CLEAR_ERROR ();
	switch (configMode)
    {
		case ODBC_USER_DSN:
			wSystemDSN = USERDSN_ONLY;
			retcode = RemoveDSNFromIni (lpszDSN);
			goto quit;
		case ODBC_SYSTEM_DSN:
			wSystemDSN = SYSTEMDSN_ONLY;
			retcode = RemoveDSNFromIni (lpszDSN);
			goto quit;
		case ODBC_BOTH_DSN:
			wSystemDSN = USERDSN_ONLY;
			retcode = RemoveDSNFromIni (lpszDSN);
			if (!retcode)
			{
				CLEAR_ERROR ();
				wSystemDSN = SYSTEMDSN_ONLY;
				retcode = RemoveDSNFromIni (lpszDSN);
			}
			goto quit;
    };
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	goto quit;
quit:
	wSystemDSN = USERDSN_ONLY;
	configMode = ODBC_BOTH_DSN;
	return retcode;
}

BOOL INSTAPI SQLRemoveTranslator (LPCSTR lpszTranslator, LPDWORD lpdwUsageCount)
{
	/* Check input parameter */
	CLEAR_ERROR ();
	if (!lpszTranslator || !strlen(lpszTranslator))
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_NAME);
    }
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	return FALSE;
}

BOOL INSTAPI SQLSetConfigMode (UWORD wConfigMode)
{
	BOOL retcode = FALSE;
	UWORD uwConfigMode;

	/* Check input parameters */
	CLEAR_ERROR ();

	uwConfigMode = (UWORD)wConfigMode;
	switch (uwConfigMode)
    {
		case ODBC_BOTH_DSN:
		case ODBC_USER_DSN:
			wSystemDSN = USERDSN_ONLY;
			goto configmode;
		case ODBC_SYSTEM_DSN:
			wSystemDSN = SYSTEMDSN_ONLY;
configmode:
			configMode = uwConfigMode;
			wSystemDSN = USERDSN_ONLY;
			retcode = TRUE;
			break;
		default:
			PUSH_ERROR (ODBC_ERROR_INVALID_PARAM_SEQUENCE);
	};
	return retcode;
}

BOOL INSTAPI SQLValidDSN (LPCSTR lpszDSN)
{
	BOOL retcode = FALSE;

	/* Check dsn */
	CLEAR_ERROR ();
	if (!lpszDSN || !strlen (lpszDSN) || strlen (lpszDSN) >= SQL_MAX_DSN_LENGTH)
    {
		PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
		goto quit;
	}
	retcode = ValidDSN (lpszDSN);
quit:
	return retcode;
}

BOOL INSTAPI SQLWriteDSNToIni (LPCSTR lpszDSN, LPCSTR lpszDriver)
{
	BOOL retcode = FALSE;

	/* Check input parameters */
	CLEAR_ERROR ();
	if (!lpszDSN || !ValidDSN (lpszDSN) || !strlen (lpszDSN))
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_DSN);
		goto quit;
    }
	if (!lpszDriver || !strlen (lpszDriver))
    {
		PUSH_ERROR (ODBC_ERROR_INVALID_NAME);
		goto quit;
    }
	switch (configMode)
    {
		case ODBC_USER_DSN:
			wSystemDSN = USERDSN_ONLY;
			retcode = WriteDSNToIni (lpszDSN, lpszDriver);
			goto quit;
		case ODBC_SYSTEM_DSN:
			wSystemDSN = SYSTEMDSN_ONLY;
			retcode = WriteDSNToIni (lpszDSN, lpszDriver);
			goto quit;
		case ODBC_BOTH_DSN:
			wSystemDSN = USERDSN_ONLY;
			retcode = WriteDSNToIni (lpszDSN, lpszDriver);
			if (!retcode)
			{
				CLEAR_ERROR ();
				wSystemDSN = SYSTEMDSN_ONLY;
				retcode = WriteDSNToIni (lpszDSN, lpszDriver);
			}
			goto quit;
    }
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	goto quit;
quit:
	wSystemDSN = USERDSN_ONLY;
	configMode = ODBC_BOTH_DSN;
	return retcode;
}

BOOL INSTAPI SQLWriteFileDSN (
	LPCSTR lpszFileName, 
	LPCSTR lpszAppName, 
	LPCSTR lpszKeyName,
    LPSTR lpszString)
{
	BOOL retcode = FALSE;

	/* Check input parameters */
	CLEAR_ERROR ();
	/* Is a file is specified */
	if (lpszFileName)
    {
		retcode = WritePrivateProfileString (lpszAppName, lpszKeyName, lpszString, lpszFileName);
		goto quit;
    }
	PUSH_ERROR (ODBC_ERROR_INVALID_PATH);
	goto quit;
quit:
	return retcode;
}

BOOL INSTAPI SQLWritePrivateProfileString (
	LPCSTR lpszSection, 
	LPCSTR lpszEntry,
    LPCSTR lpszString, 
	LPCSTR lpszFilename)
{
	char pathbuf[1024];
	BOOL retcode = FALSE;

	/* Check input parameters */
	CLEAR_ERROR ();
	/* Else go through user/system odbc.ini */
	switch (configMode)
    {
		case ODBC_USER_DSN:
			wSystemDSN = USERDSN_ONLY;
			if (lpszFilename)
			{
				retcode = WritePrivateProfileString (lpszSection,lpszEntry,lpszString,lpszFilename);
				goto quit;
			}
			if (fun_getinifile (pathbuf, sizeof (pathbuf), FALSE, TRUE))
				retcode = WritePrivateProfileString (lpszSection, lpszEntry, lpszString, pathbuf);
			goto quit;
		case ODBC_SYSTEM_DSN:
			wSystemDSN = SYSTEMDSN_ONLY;
			if (lpszFilename)
			{
				retcode = WritePrivateProfileString (lpszSection, lpszEntry, lpszString, lpszFilename);
				goto quit;
			}
			if (fun_getinifile (pathbuf, sizeof (pathbuf), FALSE, TRUE))
				retcode = WritePrivateProfileString (lpszSection, lpszEntry, lpszString, pathbuf);
			goto quit;
		case ODBC_BOTH_DSN:
			wSystemDSN = USERDSN_ONLY;
			if (lpszFilename)
			{
				retcode = WritePrivateProfileString (lpszSection, lpszEntry, lpszString, lpszFilename);
				if (!retcode)
				{
					CLEAR_ERROR ();
					wSystemDSN = SYSTEMDSN_ONLY;
					retcode = WritePrivateProfileString (lpszSection, lpszEntry, lpszString, lpszFilename);
				}
				goto quit;
			}
			if (fun_getinifile (pathbuf, sizeof (pathbuf), FALSE, TRUE))
				retcode = WritePrivateProfileString (lpszSection, lpszEntry, lpszString, pathbuf);
			else
			{
				CLEAR_ERROR ();
				wSystemDSN = SYSTEMDSN_ONLY;
				if (fun_getinifile (pathbuf, sizeof (pathbuf), FALSE, TRUE))
					retcode = WritePrivateProfileString (lpszSection, lpszEntry, lpszString, pathbuf);
			}
			goto quit;
	};
	PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
	goto quit;
quit:
	wSystemDSN = USERDSN_ONLY;
	configMode = ODBC_BOTH_DSN;
	return retcode;
}

