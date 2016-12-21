/*************************************************************************
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

#include "LargeVarcharCol.h"

CLargeVarcharCol::CLargeVarcharCol(const char * chDsn, const char * chUID, const char * chPwd) 
	:CTestBase(chDsn,chUID,chPwd)
{
	std::cout << "******************************" << endl;
	std::cout << "*Varchar 32k Unit Test Begin *" << endl;
	std::cout << "******************************" << endl;
}

CLargeVarcharCol::~CLargeVarcharCol()
{
}

bool CLargeVarcharCol::Prepare()
{
	cout << "Preparing environment for upcoming test...this process may take a while..." <<endl;
	SQLRETURN	returnCode;
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"create schema iso88591", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, create schema iso88591)", returnCode);
		return false;
	}
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"create schema utf8", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, create schema utf8)", returnCode);
		return false;
	}
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"create table iso88591.tvarchar(c1 varchar(200000))", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, create table iso88591.tvarchar(c1 varchar(200000)))", returnCode);
		return false;
	}
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"create table utf8.tvarchar(c1 varchar(50000) character set utf8)", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, create table utf8.tvarchar(c1 varchar(50000) character set utf8))", returnCode);
		return false;
	}
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"create table iso88591.tlongvarchar(c1 long varchar(2000))", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, create table iso88591.tlongvarchar(c1 long varchar(2000)))", returnCode);
		return false;
	}
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"create table utf8.tlongvarchar(c1 long varchar(2000) character set utf8)", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, create table utf8.tlongvarchar(c1 long varchar(2000) character set utf8))", returnCode);
		return false;
	}
	return true;
}

bool CLargeVarcharCol::TestGo()
{
	if (!InsertCharToVarcharCol(TRAF_ALL))
		return false;
	if (!InsertWCharToVarcharCol(TRAF_ALL))
		return false;
	if (!InsertCharToLongVarcharCol(TRAF_ALL))
		return false;
	if (!InsertWCharToLongVarcharCol(TRAF_ALL))
		return false;
	if (!VarcharToULong(TRAF_ALL))
		return false;
	if (!VarcharToDate(TRAF_ALL))
		return false;
	if (!VarcharToDouble(TRAF_ALL))
		return false;
	if (!VarcharToTime(TRAF_ALL))
		return false;
	if (!VarcharToTimestamp(TRAF_ALL))
		return false;
	if (!VarcharToInterval(TRAF_ALL))
		return false;

	std::cout << "Test Result: Pass" << endl;
	return true;
}

const char * strDictionary = (char *)"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*0123456789012345678901234567890123456789";//100 Bytes
const wchar_t  wstrDictionary[200] = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*0123456789012345678901234567890123456789";//100 Characters
unsigned char chInArray[200001];
unsigned char chOutArray[200001];
wchar_t wchInArray[200001];
wchar_t wchOutArray[200001];

bool CLargeVarcharCol::InsertCharToVarcharCol(int schema)
{
	SQLRETURN returnCode;
	SQLLEN cbC1 = SQL_NTS;

	/************************************************************************/
	/* Purge table data                                                     */
	/************************************************************************/
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
		return false;
	}
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
		return false;
	}
		
	memset(chInArray, 0, sizeof(chInArray));
	memset(chOutArray, 0, sizeof(chOutArray));
	if ((schema & TRAF_ISO88591) == TRAF_ISO88591)
	{
		/************************************************************************/
		/* insert char                                                          */
		/************************************************************************/
		std::cout << "char ==> iso88591-varchar:" << endl;
#ifdef LARGECOL
		for (int i = 0; i < 2000; i++)
		{
			strcat((char *)chInArray, strDictionary);
		}
		std::cout << "length: " << strlen((char *)chInArray) << " bytes..." << endl;

		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0, chInArray, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into iso88591.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}

		/************************************************************************/
		/* retrieve data to verify result                                       */
		/************************************************************************/
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from iso88591.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_CHAR, &chOutArray, sizeof(chOutArray), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (strcmp((char *)chInArray, (char *)chOutArray) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch iso88591.tvarchar", returnCode);
			return false;
		}
		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
			return false;
		}
#endif
		/************************************************************************/
		/* insert char                                                          */
		/************************************************************************/
		memset(chOutArray, 0, sizeof(chOutArray));
		memset(chInArray, 0, sizeof(chInArray));
		cbC1 = SQL_NTS;
		strcat((char *)chInArray, (char *)strDictionary);
		std::cout << "length: " << strlen((char *)chInArray) << " bytes..." << endl;
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0, chInArray, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into iso88591.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
			
		/************************************************************************/
		/* retrieve data to verify result                                       */
		/************************************************************************/
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from iso88591.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_CHAR, &chOutArray, sizeof(chOutArray), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (strcmp((char *)chInArray, (char *)chOutArray) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch iso88591.tvarchar", returnCode);
			return false;
		}
		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
			return false;
		}
	}

	memset(chInArray, 0, sizeof(chInArray));
	memset(chOutArray, 0, sizeof(chOutArray));
	if ((schema & TRAF_UTF8) == TRAF_UTF8)
	{
		/************************************************************************/
		/* insert char                                                          */
		/************************************************************************/
		std::cout << "char ==> utf8-varchar:" << endl;
#ifdef LARGECOL
		for (int i = 0; i < 2000; i++)
		{
			strcat((char *)chInArray, strDictionary);
		}
		cbC1 = SQL_NTS;
		std::cout << "length: " << strlen((char *)chInArray) << " bytes..." << endl;

		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0, chInArray, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into utf8.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
			
		/************************************************************************/
		/* retrieve data to verify result                                       */
		/************************************************************************/
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from utf8.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_CHAR, &chOutArray, sizeof(chOutArray), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (strcmp((char *)chInArray, (char *)chOutArray) == 0){
				std::cout << "Succeeded." << endl;
			}
			else{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch utf8.tvarchar", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
			return false;
		}
#endif
		/************************************************************************/
		/* insert char                                                          */
		/************************************************************************/
		memset(chOutArray, 0, sizeof(chOutArray));
		memset(chInArray, 0, sizeof(chInArray));
		cbC1 = SQL_NTS;
		strcat((char *)chInArray, (char *)strDictionary);
		std::cout << "length: " << strlen((char *)chInArray) << " bytes..." << endl;

		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0, chInArray, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into utf8.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}

		/************************************************************************/
		/* retrieve data to verify result                                       */
		/************************************************************************/
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from utf8.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_CHAR, &chOutArray, sizeof(chOutArray), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (strcmp((char *)chInArray, (char *)chOutArray) == 0){
				std::cout << "Succeeded." << endl;
			}
			else{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch utf8.tvarchar", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
			return false;
		}
	}
	return true;
}

bool CLargeVarcharCol::InsertWCharToVarcharCol(int schema)
{
	SQLRETURN returnCode;
	SQLLEN cbC1 = SQL_NTS;

	/************************************************************************/
	/* Purge table data                                                     */
	/************************************************************************/
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
		return false;
	}
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
		return false;
	}
		
	memset(wchInArray, 0, sizeof(wchInArray));
	memset(wchOutArray, 0, sizeof(wchOutArray));
	if ((schema & TRAF_ISO88591) == TRAF_ISO88591)
	{
		/************************************************************************/
		/* insert wchar                                                         */
		/************************************************************************/
		std::cout << "wchar ==> iso88591-varchar:" << endl;

#ifdef LARGECOL
		for (int i = 0; i < 2000; i++)
		{
			wcscat(wchInArray, wstrDictionary);
		}

		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_VARCHAR, 0, 0, wchInArray, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into iso88591.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
			
		/************************************************************************/
		/* retrieve data to verify result                                       */
		/************************************************************************/
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from iso88591.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
			
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_WCHAR, wchOutArray, sizeof(wchOutArray), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
				
			if (memcmp(wchInArray, wchOutArray, sizeof(wchInArray)) == 0){
				std::cout << "Succeeded." << endl;
			}
			else{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch iso88591.tvarchar", returnCode);
			return false;
		}
		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
			return false;
		}
#endif
		/************************************************************************/
		/* insert wchar                                                         */
		/************************************************************************/
		memset(wchOutArray, 0, sizeof(wchOutArray));
		memset(wchInArray, 0, sizeof(wchInArray));
		wcscat(wchInArray, wstrDictionary);

		cbC1 = SQL_NTS;
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_VARCHAR, 0, 0, &wchInArray, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into iso88591.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}

		/************************************************************************/
		/* retrieve data to verify result                                       */
		/************************************************************************/
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from iso88591.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_WCHAR, wchOutArray, sizeof(wchOutArray), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (wcscmp(wchInArray, wchOutArray) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch iso88591.tvarchar", returnCode);
			return false;
		}
		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
			return false;
		}
	}

	memset(wchInArray, 0, sizeof(wchInArray));
	memset(wchOutArray, 0, sizeof(wchOutArray));

	if ((schema & TRAF_UTF8) == TRAF_UTF8)
	{
		/************************************************************************/
		/* insert wchar                                                          */
		/************************************************************************/
		std::cout << "wchar ==> utf8-varchar:" << endl;
		cbC1 = SQL_NTS;
#ifdef LARGECOL
		for (int i = 0; i < 2000; i++)
		{
			wcscat(wchInArray, wstrDictionary);
		}

		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_VARCHAR, 0, 0, wchInArray, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into utf8.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
			
		/************************************************************************/
		/* retrieve data to verify result                                       */
		/************************************************************************/
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from utf8.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
			
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_WCHAR, &wchOutArray, sizeof(wchOutArray), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (wcscmp(wchInArray, wchOutArray) == 0){
				std::cout << "Succeeded." << endl;
			}
			else{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch utf8.tvarchar", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
			return false;
		}
#endif
		/************************************************************************/
		/* insert wchar                                                          */
		/************************************************************************/
		memset(wchOutArray, 0, sizeof(wchOutArray));
		memset(wchInArray, 0, sizeof(wchInArray));
		wcscat(wchInArray, wstrDictionary);
		cbC1 = SQL_NTS;
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_VARCHAR, 0, 0, wchInArray, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into utf8.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
			
		/************************************************************************/
		/* retrieve data to verify result                                       */
		/************************************************************************/
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from utf8.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_WCHAR, wchOutArray, sizeof(wchOutArray), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (wcscmp(wchInArray, wchOutArray) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch utf8.tvarchar", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
			return false;
		}
	}
	return true;
}

bool CLargeVarcharCol::InsertCharToLongVarcharCol(int schema)
{
	SQLRETURN returnCode;
	SQLLEN cbC1 = SQL_NTS;

	/************************************************************************/
	/* Purge table data                                                     */
	/************************************************************************/
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tlongvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tlongvarchar)", returnCode);
		return false;
	}
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tlongvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tlongvarchar)", returnCode);
		return false;
	}
	memset(chInArray, 0, sizeof(chInArray));
	memset(chOutArray, 0, sizeof(chOutArray));

	if ((schema & TRAF_ISO88591) == TRAF_ISO88591)
	{
		/************************************************************************/
		/* insert char                                                          */
		/************************************************************************/
		std::cout << "char ==> iso88591-longvarchar:" << endl;
		for (int i = 0; i < 20; i++)
		{
			strcat((char *)chInArray, strDictionary);
		}
		std::cout << "length: " << strlen((char *)chInArray) << " bytes..." << endl;

		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR, 0, 0, chInArray, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}	
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into iso88591.tlongvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}	
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* retrieve data to verify result                                       */
		/************************************************************************/
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from iso88591.tlongvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_CHAR, &chOutArray, sizeof(chOutArray), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (strcmp((char *)chInArray, (char *)chOutArray) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else
		{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch iso88591.tlongvarchar", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tlongvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tlongvarchar)", returnCode);
			return false;
		}
		/************************************************************************/
		/* insert char                                                          */
		/************************************************************************/
		memset(chOutArray, 0, sizeof(chOutArray));
		memset(chInArray, 0, sizeof(chInArray));
		cbC1 = SQL_NTS;
		strcat((char *)chInArray, (char *)strDictionary);
		std::cout << "length: " << strlen((char *)chInArray) << " bytes..." << endl;
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR, 0, 0, chInArray, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into iso88591.tlongvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}	
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* retrieve data to verify result                                       */
		/************************************************************************/
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from iso88591.tlongvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_CHAR, &chOutArray, sizeof(chOutArray), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (strcmp((char *)chInArray, (char *)chOutArray) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch iso88591.tlongvarchar", returnCode);
			return false;
		}
		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}	
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tlongvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tlongvarchar)", returnCode);
			return false;
		}	
	}

	memset(chInArray, 0, sizeof(chInArray));
	memset(chOutArray, 0, sizeof(chOutArray));

	if ((schema & TRAF_UTF8) == TRAF_UTF8)
	{
		/************************************************************************/
		/* insert char                                                          */
		/************************************************************************/
		std::cout << "char ==> utf8-longvarchar:" << endl;
		for (int i = 0; i < 20; i++)
		{
			strcat((char *)chInArray, strDictionary);
		}
		cbC1 = SQL_NTS;
		std::cout << "length: " << strlen((char *)chInArray) << " bytes..." << endl;

		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR, 0, 0, chInArray, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}	
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into utf8.tlongvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}	
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* retrieve data to verify result                                       */
		/************************************************************************/
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from utf8.tlongvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}	
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_CHAR, &chOutArray, sizeof(chOutArray), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (strcmp((char *)chInArray, (char *)chOutArray) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch utf8.tlongvarchar", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tlongvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tlongvarchar)", returnCode);
			return false;
		}
		/************************************************************************/
		/* insert char                                                          */
		/************************************************************************/
		memset(chOutArray, 0, sizeof(chOutArray));
		memset(chInArray, 0, sizeof(chInArray));
		cbC1 = SQL_NTS;
		strcat((char *)chInArray, (char *)strDictionary);
		std::cout << "length: " << strlen((char *)chInArray) << " bytes..." << endl;

		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR, 0, 0, chInArray, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}	
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into utf8.tlongvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}	
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* retrieve data to verify result                                       */
		/************************************************************************/
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from utf8.tlongvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_CHAR, &chOutArray, sizeof(chOutArray), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (strcmp((char *)chInArray, (char *)chOutArray) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch utf8.tlongvarchar", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tlongvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tlongvarchar)", returnCode);
			return false;
		}
	}
	return true;
}

bool CLargeVarcharCol::InsertWCharToLongVarcharCol(int schema)
{
	SQLRETURN returnCode;
	SQLLEN cbC1 = SQL_NTS;

	/************************************************************************/
	/* Purge table data                                                     */
	/************************************************************************/
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tlongvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tlongvarchar)", returnCode);
		return false;
	}
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tlongvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tlongvarchar)", returnCode);
		return false;
	}
	memset(wchInArray, 0, sizeof(wchInArray));
	memset(wchOutArray, 0, sizeof(wchOutArray));

	if ((schema & TRAF_ISO88591) == TRAF_ISO88591)
	{
		/************************************************************************/
		/* insert wchar                                                          */
		/************************************************************************/
		std::cout << "wchar ==> iso88591-longvarchar:" << endl;
		for (int i = 0; i < 20; i++)
		{
			wcscat(wchInArray, wstrDictionary);
		}

		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_LONGVARCHAR, 0, 0, wchInArray, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}	
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into iso88591.tlongvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}	
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* retrieve data to verify result                                       */
		/************************************************************************/
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from iso88591.tlongvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_WCHAR, &wchOutArray, sizeof(wchOutArray), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (wcscmp(wchInArray, wchOutArray) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else
		{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch iso88591.tlongvarchar", returnCode);
			return false;
		}
		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}	
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tlongvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tlongvarchar)", returnCode);
			return false;
		}
		/************************************************************************/
		/* insert wchar                                                          */
		/************************************************************************/
		memset(wchOutArray, 0, sizeof(wchOutArray));
		memset(wchInArray, 0, sizeof(wchInArray));
		wcscat(wchInArray, wstrDictionary);
		cbC1 = SQL_NTS;
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_LONGVARCHAR, 0, 0, wchInArray, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}	
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into iso88591.tlongvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}	
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* retrieve data to verify result                                       */
		/************************************************************************/
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from iso88591.tlongvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}	
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_WCHAR, &wchOutArray, sizeof(wchOutArray), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (wcscmp(wchInArray, wchOutArray) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch iso88591.tlongvarchar", returnCode);
			return false;
		}
		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}	
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tlongvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tlongvarchar)", returnCode);
			return false;
		}	
	}

	memset(wchInArray, 0, sizeof(wchInArray));
	memset(wchOutArray, 0, sizeof(wchOutArray));

	if ((schema & TRAF_UTF8) == TRAF_UTF8)
	{
		/************************************************************************/
		/* insert wchar                                                          */
		/************************************************************************/
		std::cout << "wchar ==> utf8-longvarchar:" << endl;
		cbC1 = SQL_NTS;
		for (int i = 0; i < 20; i++)
		{
			wcscat(wchInArray, wstrDictionary);
		}

		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_LONGVARCHAR, 0, 0, wchInArray, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into utf8.tlongvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* retrieve data to verify result                                       */
		/************************************************************************/
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from utf8.tlongvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_WCHAR, &wchOutArray, sizeof(wchOutArray), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (wcscmp(wchInArray, wchOutArray) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else
		{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch utf8.tlongvarchar", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tlongvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tlongvarchar)", returnCode);
			return false;
		}

		/************************************************************************/
		/* insert wchar                                                          */
		/************************************************************************/
		memset(wchOutArray, 0, sizeof(wchOutArray));
		memset(wchInArray, 0, sizeof(wchInArray));
		wcscat(wchInArray, wstrDictionary);
		cbC1 = SQL_NTS;

		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_LONGVARCHAR, 0, 0, wchInArray, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into utf8.tlongvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* retrieve data to verify result                                       */
		/************************************************************************/
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from utf8.tlongvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_WCHAR, wchOutArray, sizeof(wchOutArray), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (wcscmp(wchInArray, wchOutArray) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch utf8.tlongvarchar", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tlongvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tlongvarchar)", returnCode);
			return false;
		}	
	}
	return true;
}

bool CLargeVarcharCol::VarcharToULong(int schema)
{
	SQLRETURN returnCode;
	SQLUINTEGER ulInC1 = 3141592653;
	SQLUINTEGER ulOutC1 = 0;

	/************************************************************************/
	/* Purge table data                                                     */
	/************************************************************************/
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
		return false;
	}
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
		return false;
	}
	if ((schema & TRAF_ISO88591) == TRAF_ISO88591)
	{
		std::cout << "insert ulong ==> iso88591-varchar..." << endl;
		SQLLEN cbC1 = SQL_NTS;
		/************************************************************************/
		/* insert ulong to varchar                                              */
		/************************************************************************/
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_VARCHAR, 0, 0, &ulInC1, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}	
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into iso88591.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}	
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* select ul from varchar                                               */
		/************************************************************************/
		std::cout << "select ulong from iso88591-varchar..." << endl;
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from iso88591.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_ULONG, &ulOutC1, sizeof(ulOutC1), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (ulInC1 == ulOutC1)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				std::cout << "Out Data: " << ulOutC1 << endl;
				return false;
			}
		}
		else
		{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch(SQL_HANDLE_STMT)", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
			return false;
		}	
	}

	ulOutC1 = 0;
	if ((schema & TRAF_UTF8) == TRAF_UTF8)
	{
		SQLLEN cbC1 = SQL_NTS;
		/************************************************************************/
		/* insert ulong to varchar                                              */
		/************************************************************************/
		std::cout << "insert ulong ==> utf8-varchar..." << endl;
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_VARCHAR, 0, 0, &ulInC1, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}	
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into utf8.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* select ul from varchar                                               */
		/************************************************************************/
		std::cout << "select ulong from utf8-varchar..." << endl;
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from utf8.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_ULONG, &ulOutC1, sizeof(ulOutC1), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (ulInC1 == ulOutC1)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				std::cout << "Out Data: " << ulOutC1 << endl;
				return false;
			}
		}
		else
		{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch(SQL_HANDLE_STMT)", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
			return false;
		}	
	}
	return true;
}

DATE_STRUCT dtInC1;
DATE_STRUCT dtOutC1;
bool CLargeVarcharCol::VarcharToDate(int schema)
{
	SQLRETURN returnCode;

	dtInC1.year = 2015;
	dtInC1.month = 8;
	dtInC1.day = 31;
	dtOutC1.year = 0;
	dtOutC1.month = 0;
	dtOutC1.day = 0;

	/************************************************************************/
	/* Purge table data                                                     */
	/************************************************************************/
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
		return false;
	}
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
		return false;
	}
	if ((schema & TRAF_ISO88591) == TRAF_ISO88591)
	{
		std::cout << "insert date ==> iso88591-varchar..." << endl;
		SQLLEN cbC1 = SQL_NTS;
		/************************************************************************/
		/* insert date to varchar                                               */
		/************************************************************************/
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_DATE, SQL_VARCHAR, 0, 0, &dtInC1, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}	
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into iso88591.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}	
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* select date from varchar                                             */
		/************************************************************************/
		std::cout << "select date from iso88591-varchar..." << endl;
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from iso88591.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_TYPE_DATE, &dtOutC1, sizeof(DATE_STRUCT), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (memcmp(&dtInC1, &dtOutC1, sizeof(DATE_STRUCT)) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				std::cout << "Out Data: " << dtOutC1.year << ":" << dtOutC1.month << ":" << dtOutC1.day << endl;
				return false;
			}
		}
		else
		{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch(SQL_HANDLE_STMT)", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}	
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
			return false;
		}	
	}

	dtOutC1.year = 0;
	dtOutC1.month = 0;
	dtOutC1.day = 0;
	if ((schema & TRAF_UTF8) == TRAF_UTF8)
	{
		SQLLEN cbC1 = SQL_NTS;
		/************************************************************************/
		/* insert date to varchar                                               */
		/************************************************************************/
		std::cout << "insert date ==> utf8-varchar..." << endl;
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_DATE, SQL_VARCHAR, 0, 0, &dtInC1, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}	
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into utf8.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* select date from varchar                                             */
		/************************************************************************/
		std::cout << "select date from utf8-varchar..." << endl;
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from utf8.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_TYPE_DATE, &dtOutC1, sizeof(DATE_STRUCT), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
				
			if (memcmp(&dtInC1, &dtOutC1, sizeof(DATE_STRUCT)) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				std::cout << "Out Data: " << dtOutC1.year << ":" << dtOutC1.month << ":" << dtOutC1.day << endl;
				return false;
			}
		}
		else
		{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch(SQL_HANDLE_STMT)", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
			return false;
		}
	}
	return true;
}

TIME_STRUCT tmInC1;
TIME_STRUCT tmOutC1;
bool CLargeVarcharCol::VarcharToTime(int schema)
{
	SQLRETURN returnCode;

	tmInC1.hour = 14;
	tmInC1.minute = 9;
	tmInC1.second = 13;
	memset(&tmOutC1, 0, sizeof(TIME_STRUCT));
	/************************************************************************/
	/* Purge table data                                                     */
	/************************************************************************/
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
		return false;
	}
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
		return false;
	}
	if ((schema & TRAF_ISO88591) == TRAF_ISO88591)
	{
		std::cout << "insert time ==> iso88591-varchar..." << endl;
		SQLLEN cbC1 = SQL_NTS;
		/************************************************************************/
		/* insert time to varchar                                              */
		/************************************************************************/
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIME, SQL_VARCHAR, 0, 0, &tmInC1, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into iso88591.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* select time from varchar                                               */
		/************************************************************************/
		std::cout << "select time from iso88591-varchar..." << endl;
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from iso88591.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_TYPE_TIME, &tmOutC1, sizeof(TIME_STRUCT), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (memcmp(&tmInC1, &tmOutC1, sizeof(TIME_STRUCT)) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else
		{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch(SQL_HANDLE_STMT)", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}	
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
			return false;
		}
	}

	memset(&tmOutC1, 0, sizeof(TIME_STRUCT));

	if ((schema & TRAF_UTF8) == TRAF_UTF8)
	{
		std::cout << "insert time ==> utf8-varchar..." << endl;
		SQLLEN cbC1 = SQL_NTS;
		/************************************************************************/
		/* insert time to varchar                                              */
		/************************************************************************/
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIME, SQL_VARCHAR, 0, 0, &tmInC1, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into utf8.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* select time from varchar                                               */
		/************************************************************************/
		std::cout << "select time from utf8-varchar..." << endl;
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from utf8.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_TYPE_TIME, &tmOutC1, sizeof(TIME_STRUCT), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (memcmp(&tmInC1, &tmOutC1, sizeof(TIME_STRUCT)) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else
		{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch(SQL_HANDLE_STMT)", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
			return false;
		}	
	}
	return true;
}

bool CLargeVarcharCol::VarcharToDouble(int schema)
{
	SQLRETURN returnCode;
	SQLDOUBLE dbInC1 = 3.141592653;
	SQLDOUBLE dbOutC1 = 0;

	/************************************************************************/
	/* Purge table data                                                     */
	/************************************************************************/
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
		return false;
	}
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
		return false;
	}
	if ((schema & TRAF_ISO88591) == TRAF_ISO88591)
	{
		std::cout << "insert double ==> iso88591-varchar..." << endl;
		SQLLEN cbC1 = SQL_NTS;
		/************************************************************************/
		/* insert double to varchar                                              */
		/************************************************************************/
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_VARCHAR, 0, 0, &dbInC1, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}	
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into iso88591.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}	
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* select double from varchar                                               */
		/************************************************************************/
		std::cout << "select double from iso88591-varchar..." << endl;
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from iso88591.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_DOUBLE, &dbOutC1, sizeof(dbOutC1), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (fabs(dbInC1 - dbOutC1) < 0.0000000001)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				std::cout << "Out Data: " << dbOutC1 << endl;
				return false;
			}
		}
		else
		{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch(SQL_HANDLE_STMT)", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}	
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
			return false;
		}	
	}

	dbOutC1 = 0;
	if ((schema & TRAF_UTF8) == TRAF_UTF8)
	{
		SQLLEN cbC1 = SQL_NTS;
		/************************************************************************/
		/* insert double to varchar                                              */
		/************************************************************************/
		std::cout << "insert double ==> utf8-varchar..." << endl;
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_VARCHAR, 0, 0, &dbInC1, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}	
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into utf8.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}	
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}	
		/************************************************************************/
		/* select double from varchar                                               */
		/************************************************************************/
		std::cout << "select double from utf8-varchar..." << endl;
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from utf8.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_DOUBLE, &dbOutC1, sizeof(dbOutC1), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (fabs(dbInC1 - dbOutC1) < 0.0000000001)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				std::cout << "Out Data: " << dbOutC1 << endl;
				return false;
			}
		}
		else
		{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch(SQL_HANDLE_STMT)", returnCode);
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
			return false;
		}
	}
	return true;
}

TIMESTAMP_STRUCT tmstpInC1;
TIMESTAMP_STRUCT tmstpOutC1;
bool CLargeVarcharCol::VarcharToTimestamp(int schema)
{
	SQLRETURN returnCode;

	tmstpInC1.year = 2015;
	tmstpInC1.month = 8;
	tmstpInC1.day = 31;
	tmstpInC1.hour = 5;
	tmstpInC1.minute = 4;
	tmstpInC1.second = 3;
	tmstpInC1.fraction = 2000;

	memset(&tmstpOutC1, 0, sizeof(TIMESTAMP_STRUCT));

	/************************************************************************/
	/* Purge table data                                                     */
	/************************************************************************/
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
		return false;
	}
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
		return false;
	}
	if ((schema & TRAF_ISO88591) == TRAF_ISO88591)
	{
		std::cout << "insert timestamp ==> iso88591-varchar..." << endl;
		SQLLEN cbC1 = SQL_NTS;
		/************************************************************************/
		/* insert timestamp to varchar                                          */
		/************************************************************************/
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_VARCHAR, 0, 0, &tmstpInC1, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into iso88591.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* select timestamp from varchar                                        */
		/************************************************************************/
		std::cout << "select timestamp from iso88591-varchar..." << endl;
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from iso88591.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_TYPE_TIMESTAMP, &tmstpOutC1, sizeof(TIMESTAMP_STRUCT), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (memcmp(&tmstpInC1, &tmstpOutC1, sizeof(TIMESTAMP_STRUCT)) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else
		{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch(SQL_HANDLE_STMT)", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode); 
			return false;
		}
	}

	memset(&tmstpOutC1, 0, sizeof(TIMESTAMP_STRUCT));

	if ((schema & TRAF_UTF8) == TRAF_UTF8)
	{
		std::cout << "insert timestamp ==> utf8-varchar..." << endl;
		SQLLEN cbC1 = SQL_NTS;
		/************************************************************************/
		/* insert timestamp to varchar                                          */
		/************************************************************************/
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_VARCHAR, 0, 0, &tmstpInC1, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into utf8.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* select timestamp from varchar                                        */
		/************************************************************************/
		std::cout << "select timestamp from utf8-varchar..." << endl;
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from utf8.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_TYPE_TIMESTAMP, &tmstpOutC1, sizeof(TIMESTAMP_STRUCT), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (memcmp(&tmstpInC1, &tmstpOutC1, sizeof(TIMESTAMP_STRUCT)) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else
		{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch(SQL_HANDLE_STMT)", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
			return false;
		}
	}
	return true;
}

SQL_INTERVAL_STRUCT intvalInC1;
SQL_INTERVAL_STRUCT intvalOutC1;
bool CLargeVarcharCol::VarcharToInterval(int schema)
{
	SQLRETURN returnCode;

	memset(&intvalInC1, 0, sizeof(SQL_INTERVAL_STRUCT));
	memset(&intvalOutC1, 0, sizeof(SQL_INTERVAL_STRUCT));

	intvalInC1.interval_type = SQL_IS_YEAR_TO_MONTH;
	intvalInC1.interval_sign = 1;
	intvalInC1.intval.year_month.year = 1;
	intvalInC1.intval.year_month.month = 2;

	/************************************************************************/
	/* Purge table data                                                     */
	/************************************************************************/
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
		return false;
	}
	returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
		return false;
	}
	if ((schema & TRAF_ISO88591) == TRAF_ISO88591)
	{
		std::cout << "insert interval ==> iso88591-varchar..." << endl;
		SQLLEN cbC1 = SQL_NTS;
		/************************************************************************/
		/* insert interval to varchar                                          */
		/************************************************************************/
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_INTERVAL_YEAR_TO_MONTH, SQL_VARCHAR, 0, 0, &intvalInC1, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}	
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into iso88591.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* select interval from varchar                                        */
		/************************************************************************/
		std::cout << "select interval from iso88591-varchar..." << endl;
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from iso88591.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_INTERVAL_YEAR_TO_MONTH, &intvalOutC1, sizeof(SQL_INTERVAL_STRUCT), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (memcmp(&intvalInC1, &intvalOutC1, sizeof(SQL_INTERVAL_STRUCT)) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else
		{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch(SQL_HANDLE_STMT)", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from iso88591.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from iso88591.tvarchar)", returnCode);
			return false;
		}
			
	}

	memset(&intvalOutC1, 0, sizeof(SQL_INTERVAL_STRUCT));

	if ((schema & TRAF_UTF8) == TRAF_UTF8)
	{
		std::cout << "insert interval ==> utf8-varchar..." << endl;
		SQLLEN cbC1 = SQL_NTS;
		/************************************************************************/
		/* insert interval to varchar                                          */
		/************************************************************************/
		returnCode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_INTERVAL_YEAR_TO_MONTH, SQL_VARCHAR, 0, 0, &intvalInC1, 0, &cbC1);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLBindParameter(SQL_HANDLE_STMT, SINGLEVARCHAR)", returnCode);
			return false;
		}
		returnCode = SQLPrepare(hstmt,
			(SQLCHAR *)"insert into utf8.tvarchar values (?)",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLPrepare(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}	
		returnCode = SQLExecute(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecute(SQL_HANDLE_STMT, INSERT)", returnCode);
			return false;
		}
		/************************************************************************/
		/* select interval from varchar                                        */
		/************************************************************************/
		std::cout << "select interval from utf8-varchar..." << endl;
		returnCode = SQLExecDirect(hstmt,
			(SQLCHAR *)"select c1 from utf8.tvarchar",
			SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(SQL_HANDLE_STMT, SELECT)", returnCode);
			return false;
		}
		returnCode = SQLFetch(hstmt);
		if (returnCode == SQL_SUCCESS)
		{
			returnCode = SQLGetData(hstmt, 1, SQL_C_INTERVAL_YEAR_TO_MONTH, &intvalOutC1, sizeof(SQL_INTERVAL_STRUCT), &cbC1);
			if (returnCode != SQL_SUCCESS)
			{
				LogDiagnostics("SQLGetData(SQL_HANDLE_STMT, SELECT)", returnCode);
				return false;
			}
			if (memcmp(&intvalInC1, &intvalOutC1, sizeof(SQL_INTERVAL_STRUCT)) == 0)
			{
				std::cout << "Succeeded." << endl;
			}
			else
			{
				std::cout << "Failed: Data Comparation Failed." << endl;
				return false;
			}
		}
		else
		{
			std::cout << "Failed." << endl;
			LogDiagnostics("SQLFetch(SQL_HANDLE_STMT)", returnCode);
			return false;
		}

		returnCode = SQLCloseCursor(hstmt);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLCloseCursor(hstmt)", returnCode);
			return false;
		}
		returnCode = SQLExecDirect(hstmt, (SQLCHAR *)"delete from utf8.tvarchar", SQL_NTS);
		if (returnCode != SQL_SUCCESS)
		{
			LogDiagnostics("SQLExecDirect(hstmt, delete from utf8.tvarchar)", returnCode);
			return false;
		}
	}
	return true;
}
void CLargeVarcharCol::CleanUp()
{
	std::cout << "Cleaning up environment of previous test...this process may take a while..." << endl;
	SQLCloseCursor(hstmt);
	SQLExecDirect(hstmt, (SQLCHAR *)"drop table iso88591.tvarchar", SQL_NTS);
	SQLExecDirect(hstmt, (SQLCHAR *)"drop table utf8.tvarchar", SQL_NTS);
	SQLExecDirect(hstmt, (SQLCHAR *)"drop table iso88591.tlongvarchar", SQL_NTS);
	SQLExecDirect(hstmt, (SQLCHAR *)"drop table utf8.tlongvarchar", SQL_NTS);
	SQLExecDirect(hstmt, (SQLCHAR *)"drop schema iso88591", SQL_NTS);
	SQLExecDirect(hstmt, (SQLCHAR *)"drop schema utf8", SQL_NTS);
}


