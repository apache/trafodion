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
// TranslationDll.cpp : Defines the entry point for the DLL application.
//

#include "TranslationDll.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	default:
		break;
	}
	return TRUE;
}


// SQLDriverToDataSource supports translations for ODBC drivers. This function is not called by 
// ODBC-enabled applications; applications request translation through SQLSetConnectAttr. The driver 
// associated with the ConnectionHandle specified in SQLSetConnectAttr calls the specified DLL to perform 
// translations of all data flowing from the driver to the data source. A default translation DLL can be 
// specified in the ODBC initialization file.
//
// Arguments:
// fOption [Input] -		Option value.
// fSqlType [Input]	-		The ODBC SQL data type. This argument tells the driver how to convert rgbValueIn 
//							into a form acceptable by the data source. For a list of valid SQL data types, 
//							see the "SQL Data Types" section in Appendix D, "Data Types."
// rgbValueIn [Input] -		Value to translate.
// cbValueIn [Input] -		Length of rgbValueIn.
// rgbValueOut [Output] -	Result of the translation.  The translation DLL does not null-terminate this value.
// cbValueOutMax [Input] -	Length of rgbValueOut.
// pcbValueOut [Output] -	The total number of bytes (excluding the null-termination byte) available to return 
//							in rgbValueOut.  For character or binary data, if this is greater than or equal to 
//							cbValueOutMax, the data in rgbValueOut is truncated to cbValueOutMax bytes.
//							For all other data types, the value of cbValueOutMax is ignored and the translation DLL 
//							assumes that the size of rgbValueOut is the size of the default C data type of the 
//							SQL data type specified with fSqlType.  The pcbValueOut argument can be a null pointer.
// szErrorMsg [Output] -	Pointer to storage for an error message. This is an empty string unless the translation 
//							failed.
// cbErrorMsgMax [Input] -	Length of szErrorMsg.
// pcbErrorMsg [Output] -	Pointer to the total number of bytes (excluding the null-termination byte) available 
//							to return in szErrorMsg. If this is greater than or equal to cbErrorMsg, the data in 
//							szErrorMsg is truncated to cbErrorMsgMax minus the null-termination character. 
//							The pcbErrorMsg argument can be a null pointer.
//
// Returns:
// TRUE if the translation was successful.
// FALSE if the translation failed.

//(fpSQLDriverToDataSource) (translateOption,
//							 ODBCDataType,
//							 DataPtr,
//							 DataLen,
//							 outDataPtr,
//							 OutLen,
//							 &translateLength,
//							 errorMsg,
//							 errorMsgMax,
//							 NULL)

BOOL SQL_API SQLDriverToDataSource(
	UDWORD	fOption,
	SWORD	fSqlType,
	PTR		rgbValueIn,
	SDWORD	cbValueIn,
	PTR		rgbValueOut,
	SDWORD	cbValueOutMax,
	SDWORD*	pcbValueOut,
	UCHAR*	szErrorMsg,
	SWORD	cbErrorMsgMax,
	SWORD*	pcbErrorMsg)
{
	DWORD	inCharSet = (fOption & 0xffff0000L) >> 16;
	DWORD	outCharSet = (fOption & 0x0000ffffL);

	int		errorCode = 0;
	char	errorText[256];
	int		errorTextLen = 0;
	int		tempLen = 0;
	int		tempMaxLen = 0;
	char*	temp = NULL;
	char*	firstUntranslatedChar = NULL;
	unsigned int translatedCharCnt = 0;
	errorText[0] = 0;

	if (rgbValueIn == NULL)
	{
		if (szErrorMsg != NULL && cbErrorMsgMax > 0)
		{
			sprintf(errorText, "Input buffer is NULL.");
			errorTextLen = min(strlen (errorText),cbErrorMsgMax-1);
			strncpy((char*)szErrorMsg, errorText, errorTextLen);
			szErrorMsg[errorTextLen] = '\0';			
		}
		return false;
	}
	if (rgbValueOut == NULL)
	{
		if (szErrorMsg != NULL && cbErrorMsgMax > 0)
		{
			sprintf(errorText, "Onput buffer is NULL.");
			errorTextLen = min(strlen (errorText),cbErrorMsgMax-1);
			strncpy((char*)szErrorMsg, errorText, errorTextLen);
			szErrorMsg[errorTextLen] = '\0';
		}
		return false;
	}

	if (fSqlType == SQL_CHAR || fSqlType == SQL_VARCHAR || fSqlType == SQL_LONGVARCHAR)
	{
		switch (outCharSet)
		{
		case cnv_UTF8:
			errorCode = LocaleToUTF8 (cnv_version1,
									 (const char *)rgbValueIn,
									 cbValueIn,
									 (const char *)rgbValueOut,
									 cbValueOutMax,
									 (enum cnv_charset)inCharSet,
									 firstUntranslatedChar,
									 (unsigned int*)pcbValueOut,
									 TRUE, // addNullAtEnd_flag
									 &translatedCharCnt);
			if (errorCode != 0)	
			{
				HandleCnvError(errorCode, szErrorMsg, cbErrorMsgMax);
				return FALSE;
			}
			break;
		case cnv_UTF16:
			errorCode = LocaleToUTF16 (cnv_version1,
									  (const char*)rgbValueIn,
									  cbValueIn,
									  (const char*)rgbValueOut,
									  cbValueOutMax,
									  (enum cnv_charset)inCharSet,
									  firstUntranslatedChar,
									  (unsigned int*)pcbValueOut,
									  0, // cnv_flags
									  TRUE, // addNullAtEnd_flag
									  &translatedCharCnt);
			if (errorCode != 0)	
			{
				HandleCnvError(errorCode, szErrorMsg, cbErrorMsgMax);
				return FALSE;
			}
			(*pcbValueOut)--;
			break;
		case cnv_ISO88591:
		case cnv_SJIS:
		case cnv_EUCJP:
		case cnv_KSC:
		case cnv_BIG5:
		case cnv_GB2312:
		case cnv_GB18030:
			tempMaxLen = cbValueIn * 2;
			temp = new char [tempMaxLen];
			errorCode = LocaleToUTF16 (cnv_version1,
									  (const char*)rgbValueIn,
									  cbValueIn,
									  (const char*)temp,
									  tempMaxLen,
									  (enum cnv_charset)inCharSet,
									  firstUntranslatedChar,
									  (unsigned int*)&tempLen,
									  0, // cnv_flags
									  FALSE, // addNullAtEnd_flag
									  &translatedCharCnt);
			if (errorCode != 0)	
			{
				HandleCnvError(errorCode, szErrorMsg, cbErrorMsgMax);
				delete [] temp;
				return FALSE;
			}
			errorCode = UTF16ToLocale (cnv_version1,
									  (const char *)temp,
									  tempLen,
									  (const char *)rgbValueOut,
									  cbValueOutMax,
									  (enum cnv_charset)outCharSet,
									  firstUntranslatedChar,
									  (unsigned int*)pcbValueOut,
									  0, // cnv_flags
									  TRUE, // addNullAtEnd_flag
									  TRUE, // allow_invalids_flag
									  &translatedCharCnt);
			if (errorCode != 0)	
			{
				HandleCnvError(errorCode, szErrorMsg, cbErrorMsgMax);
				delete [] temp;
				return FALSE;
			}
			if (outCharSet == cnv_UTF16) // pcbValueOut should not count null-terminator
				(*pcbValueOut)--;
			delete [] temp;
			break;
		default:
			if (szErrorMsg != NULL && cbErrorMsgMax > 0)
			{
				sprintf(errorText, "Invalid output character set specified.");
				errorTextLen = min(strlen (errorText),cbErrorMsgMax-1);
				strncpy((char*)szErrorMsg, errorText, errorTextLen);
				szErrorMsg[errorTextLen] = '\0';
			}
			return FALSE;
		}
	}
	else
	{
		errorCode = LocaleToUTF8 (cnv_version1,
								 (const char *)rgbValueIn,
								 cbValueIn,
								 (const char *)rgbValueOut,
								 cbValueOutMax,
								 (enum cnv_charset)inCharSet,
								 firstUntranslatedChar,
								 (unsigned int*)pcbValueOut,
								 TRUE, // addNullAtEnd_flag
								 &translatedCharCnt);
		if (errorCode != 0)	
		{
			HandleCnvError(errorCode, szErrorMsg, cbErrorMsgMax);
			return FALSE;
		}
	}
	(*pcbValueOut)--;
	return TRUE;
}


// SQLDataSourceToDriver supports translations for ODBC drivers. 
// This function is not called by ODBC-enabled applications; applications request translation through 
// SQLSetConnectAttr. The driver associated with the ConnectionHandle specified in SQLSetConnectAttr 
// calls the specified DLL to perform translations of all data flowing from the data source to the driver. 
// A default translation DLL can be specified in the ODBC initialization file.
// 
// Arguments:
// fOption [Input] -		Option value.
// fSqlType [Input] -		The SQL data type. This argument tells the driver how to convert rgbValueIn into 
//							a form acceptable by the application. For a list of valid SQL data types, 
//							see the "SQL Data Types" section in Appendix D, "Data Types."
// rgbValueIn [Input] -		Value to translate.
// cbValueIn [Input] -		Length of rgbValueIn.
// rgbValueOut [Output] -	Result of the translation.  The translation DLL does not null-terminate this value.
// cbValueOutMax [Input] -	Length of rgbValueOut.
// pcbValueOut [Output] -	The total number of bytes (excluding the null-termination byte) available to return 
//							in rgbValueOut.  For character or binary data, if this is greater than or equal to 
//							cbValueOutMax, the data in rgbValueOut is truncated to cbValueOutMax bytes.
//							For all other data types, the value of cbValueOutMax is ignored and the translation DLL 
//							assumes that the size of rgbValueOut is the size of the default C data type of the SQL 
//							data type specified with fSqlType.  The pcbValueOut argument can be a null pointer.
// szErrorMsg [Output] -	Pointer to storage for an error message. This is an empty string unless the translation failed.
// cbErrorMsgMax [Input] -	Length of szErrorMsg.
// pcbErrorMsg [Output] -	Pointer to the total number of bytes (excluding the null-termination byte) available 
//							to return in szErrorMsg. If this is greater than or equal to cbErrorMsg, the data in 
//							szErrorMsg is truncated to cbErrorMsgMax minus the null-termination character. 
//							The pcbErrorMsg argument can be a null pointer.
//
// Returns:
// TRUE if the translation was successful.
// FALSE if the translation failed.

//(fpSQLDataSourceToDriver) (translateOption, 
//							 ODBCDataType,
//							 DataPtr, 
//							 DataLen,
//							 targetDataPtr,
//							 translateLengthMax, 
//							 &translateLength,
//							 errorMsg,
//							 errorMsgMax,
//							 NULL)

BOOL SQL_API SQLDataSourceToDriver(
	UDWORD	fOption,
	SWORD	fSqlType,
	PTR		rgbValueIn,
	SDWORD	cbValueIn,
	PTR		rgbValueOut,
	SDWORD	cbValueOutMax,
	SDWORD*	pcbValueOut,
	UCHAR*	szErrorMsg,
	SWORD	cbErrorMsgMax,
	SWORD*	pcbErrorMsg,
	PTR		replacementChar)
{
	DWORD	inCharSet = (fOption & 0xffff0000L) >> 16;
	DWORD	outCharSet = (fOption & 0x0000ffffL);

	int		errorCode = 0;
	char	errorText[256];
	int		errorTextLen = 0;
	int		tempLen = 0;
	int		tempMaxLen = 0;
	char*	temp = NULL;
	char*	firstUntranslatedChar = NULL;
	bool	useReplacementChar = FALSE;
	unsigned int translatedCharCnt = 0;
	errorText[0] = 0;

	if (rgbValueIn == NULL)
	{
		if (szErrorMsg != NULL && cbErrorMsgMax > 0)
		{
			sprintf(errorText, "Input buffer is NULL.");
			errorTextLen = min(strlen (errorText),cbErrorMsgMax-1);
			strncpy((char*)szErrorMsg, errorText, errorTextLen);
			szErrorMsg[errorTextLen] = '\0';
		}
		return false;
	}
	if (rgbValueOut == NULL)
	{
		if (szErrorMsg != NULL && cbErrorMsgMax > 0)
		{
			sprintf(errorText, "Onput buffer is NULL.");
			errorTextLen = min(strlen (errorText),cbErrorMsgMax-1);
			strncpy((char*)szErrorMsg, errorText, errorTextLen);
			szErrorMsg[errorTextLen] = '\0';
		}
		return false;
	}

	if (replacementChar != NULL)
		useReplacementChar = TRUE;

	if (fSqlType == SQL_CHAR || fSqlType == SQL_VARCHAR || fSqlType == SQL_LONGVARCHAR)
	{
		switch (inCharSet)
		{
		case cnv_UTF8:
			errorCode = UTF8ToLocale (cnv_version1,
									 (const char *)rgbValueIn,
									 cbValueIn,
									 (const char *)rgbValueOut,
									 cbValueOutMax,
									 (enum cnv_charset)outCharSet,
									 firstUntranslatedChar,
									 (unsigned int*)pcbValueOut,
									 TRUE, // addNullAtEnd_flag
									 useReplacementChar, // allow_invalids_flag
									 &translatedCharCnt,
									 (const char *)replacementChar);
			if (errorCode != 0)	
			{
				HandleCnvError(errorCode, szErrorMsg, cbErrorMsgMax);
				return FALSE;
			}
			break;
		case cnv_UTF16:
			errorCode = UTF16ToLocale (cnv_version1,
									  (const char *)rgbValueIn,
									  cbValueIn,
									  (const char *)rgbValueOut,
									  cbValueOutMax,
									  (enum cnv_charset)outCharSet,
									  firstUntranslatedChar,
									  (unsigned int*)pcbValueOut,
									  0, // cnv_flags
									  TRUE, // addNullAtEnd_flag
									  useReplacementChar, // allow_invalids_flag
									  &translatedCharCnt,
									  (const char *)replacementChar);
			if (errorCode != 0)	
			{
				HandleCnvError(errorCode, szErrorMsg, cbErrorMsgMax);
				return FALSE;
			}
			break;
		case cnv_ISO88591:
		case cnv_SJIS:
		case cnv_EUCJP:
		case cnv_KSC:
		case cnv_BIG5:
		case cnv_GB2312:
		case cnv_GB18030:
			tempMaxLen = cbValueIn * 2;
			temp = new char [tempMaxLen];
			errorCode = LocaleToUTF16 (cnv_version1,
									  (const char*)rgbValueIn,
									  cbValueIn,
									  (const char*)temp,
									  tempMaxLen,
									  (enum cnv_charset)inCharSet,
									  firstUntranslatedChar,
									  (unsigned int*)&tempLen,
									  0, // cnv_flags
									  FALSE, // addNullAtEnd_flag
									  &translatedCharCnt);
			if (errorCode != 0)	
			{
				HandleCnvError(errorCode, szErrorMsg, cbErrorMsgMax);
				delete [] temp;
				return FALSE;
			}
			errorCode = UTF16ToLocale (cnv_version1,
									  (const char *)temp,
									  tempLen,
									  (const char *)rgbValueOut,
									  cbValueOutMax,
									  (enum cnv_charset)outCharSet,
									  firstUntranslatedChar,
									  (unsigned int*)pcbValueOut,
									  0, // cnv_flags
									  TRUE, // addNullAtEnd_flag
									  useReplacementChar, // allow_invalids_flag
									  &translatedCharCnt,
									  (const char *)replacementChar);
			if (errorCode != 0)	
			{
				HandleCnvError(errorCode, szErrorMsg, cbErrorMsgMax);
				delete [] temp;
				return FALSE;
			}
			delete [] temp;
			break;
		default:
			if (szErrorMsg != NULL && cbErrorMsgMax > 0)
			{
				sprintf(errorText, "Invalid input character set specified.");
				errorTextLen = min(strlen (errorText),cbErrorMsgMax-1);
				strncpy((char*)szErrorMsg, errorText, errorTextLen);
				szErrorMsg[errorTextLen] = '\0';
			}
			return FALSE;
		}
	}
	else
	{
		errorCode = UTF8ToLocale (cnv_version1,
								 (const char *)rgbValueIn,
								 cbValueIn,
								 (const char *)rgbValueOut,
								 cbValueOutMax,
								 (enum cnv_charset)outCharSet,
								 firstUntranslatedChar,
								 (unsigned int*)pcbValueOut,
								 TRUE, // addNullAtEnd_flag
								 useReplacementChar, // allow_invalids_flag
								 &translatedCharCnt,
								 (const char *)replacementChar);
		if (errorCode != 0)	
		{
			HandleCnvError(errorCode, szErrorMsg, cbErrorMsgMax);
			return FALSE;
		}
	}
	if (outCharSet == cnv_UTF16) // pcbValueOut should not count null-terminator
		(*pcbValueOut)--;
	(*pcbValueOut)--;
	return TRUE;
}

void HandleCnvError (int cnv_error, unsigned char* errorMsg, long errorMsgMax)
{
	char	errorText[256];
	int		errorTextLen = 0;
	
	if (errorMsg != NULL && errorMsgMax > 0)
	{
		switch (cnv_error)
		{
		case CNV_ERR_INVALID_CHAR:
			sprintf(errorText, "Character(s) in input cannot be translated.");
			break;
		case CNV_ERR_BUFFER_OVERRUN:
			sprintf(errorText, "Output buffer overflow.");
			break;
		case CNV_ERR_NOINPUT:
			sprintf(errorText, "No input buffer or input count is negative.");
			break;
		case CNV_ERR_INVALID_CS:
			sprintf(errorText, "Invalid character set is specified.");
			break;
		case CNV_ERR_INVALID_VERS:
			sprintf(errorText, "Invalid version is specified.");
			break;
		default:
			errorText[0] = '\0';
			break;
		}

		errorTextLen = min(strlen (errorText),errorMsgMax-1);
		strncpy((char*)errorMsg, errorText, errorTextLen);
		errorMsg[errorTextLen] = '\0';	
	}
	return;
}
