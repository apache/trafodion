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
// OdbcDS.cpp: implementation of the COdbcDS class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <malloc.h>
#include "OdbcDS.h"
#include "ODBCMXAttribs.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
const int COdbcDS::DS_TYPE_SYS = 0;
const int COdbcDS::DS_TYPE_USER = 1;

COdbcDS::COdbcDS():
m_henv(NULL)
{
	Connect();
}

COdbcDS::~COdbcDS()
{
	Disconnect();
}

char* COdbcDS::Convert(const char* oldDriverName, const char* newDriverName)
{
	CDSList UserList, SysList;

	try
	{
		GetMxDSList(oldDriverName, DS_TYPE_SYS, SysList);
		GetMxDSList(oldDriverName, DS_TYPE_USER, UserList);

		RetrieveMxDSInfo(DS_TYPE_SYS, SysList);
		RetrieveMxDSInfo(DS_TYPE_USER, UserList);

		UpdateDS(newDriverName, DS_TYPE_SYS, SysList);
		UpdateDS(newDriverName, DS_TYPE_USER, UserList);
	}
	catch(int exception)
	{
		if (exception > 0)
		{
			return GetLastError();
		}
		else if (exception < 0)
		{
			return GetLastInstallerError();
		}
		return NULL;
	}

	return NULL;
}

void COdbcDS::Connect()
{
	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv))
	{
		throw 0;
	}

	if (SQL_SUCCESS != SQLSetEnvAttr(m_henv, SQL_ATTR_ODBC_VERSION,
						(SQLPOINTER)SQL_OV_ODBC3, 0))
	{
		throw 1;
	}
}

bool COdbcDS::FindDSNByVersion(const char* pDriverName, char* Message)
{
	bool bFound = false;

	try
	{
		CDSList UserList, SysList;

		// First look at user then system data sources.
		bFound = GetMxDSList(pDriverName, DS_TYPE_USER, UserList);
		if (!bFound)
			bFound = GetMxDSList(pDriverName, DS_TYPE_SYS, SysList);
	}
	catch(int exception)
	{
		char* msg = NULL;

		if (exception > 0)
		{
			msg = GetLastError();
			strcpy(Message, msg);
		}
		else if (exception < 0)
		{
			msg = GetLastInstallerError();
			strcpy(Message, msg);
		}
		return false;
	}

	return bFound;
}

bool COdbcDS::GetMxDSList(const char* pDriverName, int iType, CDSList& DSList)
{
	SQLCHAR DSNName[SQL_MAX_DSN_LENGTH + 1];
	SQLCHAR Description[100];
	SQLSMALLINT nameLen, descLen;
	SQLUSMALLINT direction;
	SQLRETURN retVal = SQL_SUCCESS;
	bool bFound = FALSE;

	// Set the config mode to the desired type.
	UWORD oldMode, newMode;

	if (iType == COdbcDS::DS_TYPE_SYS)
	{
		newMode = ODBC_SYSTEM_DSN;
		direction = SQL_FETCH_FIRST_SYSTEM;
	}
	else
	{
		newMode = ODBC_USER_DSN;
		direction = SQL_FETCH_FIRST_USER;
	}

	SQLGetConfigMode(&oldMode);
	SQLSetConfigMode(newMode);

	// Get the type of data source we're interested in.
	retVal = SQLDataSources(m_henv, direction, DSNName,	sizeof(DSNName),
							&nameLen, Description, sizeof(Description),
							&descLen);
	if (retVal == SQL_ERROR)
	{
		SQLSetConfigMode(oldMode);
		throw 4;
	}

	// Loop through all of the data sources until the one we're looking for
	// is found.
	while (retVal == SQL_SUCCESS || retVal == SQL_SUCCESS_WITH_INFO)
	{
		if (strcmp(pDriverName, (const char*)Description) == 0)
		{
			DSList.append((const char*)DSNName, pDriverName);
			bFound = true;
		}
		
		retVal = SQLDataSources(m_henv, SQL_FETCH_NEXT, DSNName,
								sizeof(DSNName), &nameLen,
								Description, sizeof(Description), &descLen);

		if (retVal == SQL_ERROR)
		{
			SQLSetConfigMode(oldMode);
			throw 4;
		}

	}

	// Restore old config mode.
	SQLSetConfigMode(oldMode);
	return bFound;
}

bool COdbcDS::IsDrvrInstalled(const char* pDriverName, char* Message)
{
	bool bIsInstalled = false;

	try
	{
		bIsInstalled = FindInstalledDrvr(pDriverName);
	}
	catch(int exception)
	{
		char* msg = NULL;

		if (exception > 0)
		{
			msg = GetLastError();
			strcpy(Message, msg);
		}
		else if (exception < 0)
		{
			msg = GetLastInstallerError();
			strcpy(Message, msg);
		}
		return false;
	}

	return bIsInstalled;
}

bool COdbcDS::FindInstalledDrvr(const char* pDrvrName)
{
	SQLCHAR Description[100];
	SQLSMALLINT descLen;
	SQLRETURN retVal = SQL_SUCCESS;

	// Loop through all of the data sources until the one we're looking for
	// is found.
	int count = 0;
	while (retVal == SQL_SUCCESS || retVal == SQL_SUCCESS_WITH_INFO)
	{
		if (count == 0)
			retVal = SQLDrivers(m_henv, SQL_FETCH_FIRST, Description,
									sizeof(Description), &descLen,
									NULL, 0, NULL);
		else
			retVal = SQLDrivers(m_henv, SQL_FETCH_NEXT, Description,
									sizeof(Description), &descLen,
									NULL, 0, NULL);
		count++;
		if (retVal == SQL_ERROR)
		{
			throw 3;
		}

		if (strcmp(pDrvrName, (const char*)Description) == 0)
		{
			return true;
		}
	}
	return false;
}

void COdbcDS::RetrieveMxDSInfo(int iType, CDSList& DSList)
{
	TCHAR Buff[255];
	char* DSName = NULL;
	int count = DSList.getCount();
	int length = sizeof(PRIV_PROFILE) / MAXKEYLEN;

	// Set the config mode to the desired type.
	UWORD oldMode, newMode;

	if (iType == COdbcDS::DS_TYPE_SYS)
	{
		newMode = ODBC_SYSTEM_DSN;
	}
	else
	{
		newMode = ODBC_USER_DSN;
	}

	SQLGetConfigMode(&oldMode);
	SQLSetConfigMode(newMode);

	for (int i = 0; i < count; i++)
	{
		DSName = DSList.getAt(i);

		for(int index = 0; index < length; index++)
		{

			// Retrieve the private profile strings.
			SQLGetPrivateProfileString(DSName, PRIV_PROFILE[index],
				"", Buff, sizeof(Buff), ODBC_INI);
			DSList.addAttrib(i, PRIV_PROFILE[index], Buff);
		}

		delete [] DSName;
	}

	// Restore old config mode.
	SQLSetConfigMode(oldMode);
}

void COdbcDS::UpdateDS(const char* pNewDriverName, int iType, CDSList& DSList)
{
	int count = DSList.getCount();
	int length = sizeof(PRIV_PROFILE) / MAXKEYLEN;
	char* DSName = NULL;
	char* DSAttVal = NULL;

	// Set the config mode to the desired type.
	UWORD oldMode, newMode;

	if (iType == COdbcDS::DS_TYPE_SYS)
	{
		newMode = ODBC_SYSTEM_DSN;
	}
	else
	{
		newMode = ODBC_USER_DSN;
	}

	SQLGetConfigMode(&oldMode);
	SQLSetConfigMode(newMode);

	for (int i = 0; i < count; i++)
	{
		DSName = DSList.getAt(i);

		// Remove the old data sources
		SQLRemoveDSNFromIni(DSName);

		// Add the new data source
		if (TRUE != SQLWriteDSNToIni(DSName, pNewDriverName))
		{
			// Restore old config mode.
			SQLSetConfigMode(oldMode);
			throw -1;
		}

		// Configure the new data source using previous information.
		for (int index = 0; index < length; index++)
		{
			DSAttVal = DSList.getAttribValue(i, PRIV_PROFILE[index]);

			if (TRUE != SQLWritePrivateProfileString(DSName,
							PRIV_PROFILE[index], DSAttVal, ODBC_INI))
			{
				// Restore old config mode.
				SQLSetConfigMode(oldMode);
				throw -2;
			}

			delete [] DSAttVal;
		}

		delete [] DSName;
	}

	// Restore old config mode.
	SQLSetConfigMode(oldMode);
}

void COdbcDS::Disconnect()
{
	if (SQL_SUCCESS != SQLFreeHandle(SQL_HANDLE_ENV, m_henv))
		throw 5;
}

char* COdbcDS::GetLastError()
{
	SQLCHAR MsgTxt[300] = "";
	SQLSMALLINT MsgLen;
	SQLRETURN retVal = SQL_SUCCESS;
	int icount = 1;
	char* msg = NULL;
	int length = 0;

	retVal = SQLGetDiagRec(SQL_HANDLE_ENV, m_henv, icount++, NULL, NULL, MsgTxt,
						sizeof(MsgTxt), &MsgLen);

	while(retVal == SQL_SUCCESS)
	{
		length = strlen((const char*)MsgTxt);

		// New message
		if (msg == NULL)
		{
			msg = (char*) malloc(sizeof(char) * (length + 1));
			strcpy(msg, (const char*)MsgTxt);
		}
		// Append to message
		else
		{
			int length2 = strlen((const char*)msg);
			// include room for null and new line.
			char* tmp = (char*) malloc(sizeof(char) * (length + length2 + 2));
			strcpy(tmp, msg);
			strcat(tmp, "\n");
			strcat(tmp, (const char*)MsgTxt);
			free(msg);
			msg = tmp;
		}

		retVal = SQLGetDiagRec(SQL_HANDLE_ENV, m_henv, icount++, NULL, NULL,
							MsgTxt, sizeof(MsgTxt), &MsgLen);

	}

	return msg;
}

char* COdbcDS::GetLastInstallerError()
{
	RETCODE retVal = SQL_SUCCESS;
	DWORD fErrorCode;
	TCHAR szErrorMsg[300] = "";
	WORD errorSize;
	int recNum = 1;
	int length = 0;
	char* msg = NULL;

	retVal = SQLInstallerError(recNum, &fErrorCode, szErrorMsg,
							sizeof(szErrorMsg), &errorSize);

	while ((retVal == SQL_SUCCESS || retVal == SQL_SUCCESS_WITH_INFO) && recNum < 9)
	{
		recNum++;
		length = strlen((const char*)szErrorMsg);

		// New message
		if (msg == NULL)
		{
			msg = (char*) malloc(sizeof(char) * (length + 1));
			strcpy(msg, (const char*)szErrorMsg);
		}
		// Append to message
		else
		{
			int length2 = strlen((const char*)msg);
			// include room for null and new line.
			char* tmp = (char*) malloc(sizeof(char) * (length + length2 + 2));
			strcpy(tmp, msg);
			strcat(tmp, "\n");
			strcat(tmp, (const char*)szErrorMsg);
			free(msg);
			msg = tmp;
		}
		
		retVal = SQLInstallerError(recNum, &fErrorCode, szErrorMsg,
							sizeof(szErrorMsg), &errorSize);

	}

	return msg;
}