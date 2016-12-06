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
#include "TestBase.h"


CTestBase::CTestBase(const char * chDsn, const char * chUID, const char * chPwd)
{
	m_chDsn = new char[100];
	m_chUID = new char[100];
	m_chPwd = new char[100];

	if (m_chDsn)
	{
		strcpy(m_chDsn, chDsn);
	}

	if (m_chUID)
	{
		strcpy(m_chUID, chUID);
	}
	
	if (m_chPwd)
	{
		strcpy(m_chPwd, chPwd);
	}
	
	henv = SQL_NULL_HANDLE;
	hstmt = SQL_NULL_HANDLE;
	hdbc = SQL_NULL_HANDLE;
	hWnd = SQL_NULL_HANDLE;
}


CTestBase::~CTestBase()
{
	if (m_chDsn)
	{
		delete[] m_chDsn;
	}

	if (m_chUID)
	{
		delete[] m_chUID;
	}

	if (m_chPwd)
	{
		delete[] m_chPwd;
	}
}

bool CTestBase::Run()
{
	if (!Connect())
		return false;

	if (!Prepare())
	{
		CleanUp();
		Disconnect();
		return false;
	}
	if (!TestGo())
	{
		CleanUp();
		Disconnect();
		return false;
	}
	CleanUp();
	Disconnect();

	return true;
}

bool CTestBase::Connect()
{
	unsigned char InConnStr[MAX_CONNECT_STRING];
	unsigned char OutConnStr[MAX_CONNECT_STRING];
	SQLRETURN	returnCode;
	SQLSMALLINT	ConnStrLength;

	sprintf((char *)InConnStr, "DSN=%s;UID=%s;PWD=%s", m_chDsn, m_chUID, m_chPwd);
	// Allocate Environment Handle
	returnCode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv)", returnCode);
		return false;
	}
		
	// Set ODBC version to 3.0
	returnCode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0)", returnCode);
		return false;
	}
		
	// Allocate Connection handle
	returnCode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc)", returnCode);
		return false;
	}

	//Connect to the database
	returnCode = SQLDriverConnect(hdbc, hWnd, InConnStr, SQL_NTS, OutConnStr, sizeof(OutConnStr), &ConnStrLength, SQL_DRIVER_NOPROMPT);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLDriverConnect", returnCode);
		return false;
	}
		
	//Allocate Statement handle
	returnCode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	if (returnCode != SQL_SUCCESS)
	{
		LogDiagnostics("SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt)", returnCode);
		return false;
	}

	return true;
}

char *SqlRetText(int rc);
void CTestBase::LogDiagnostics(const char *sqlFunction, SQLRETURN rc)
{
	SQLRETURN diagRC = SQL_SUCCESS;
	SQLSMALLINT recordNumber;
	SQLINTEGER nativeError;
	SQLCHAR messageText[SQL_MAX_MESSAGE_LENGTH];
	SQLCHAR sqlState[6];
	int diagsPrinted = 0;
	bool printedErrorLogHeader = false;
	SQLSMALLINT totalNumber = 0;

	std::cout << "Function " << sqlFunction << "returned : " << SqlRetText(rc) << endl;

	/* Log any henv Diagnostics */
	recordNumber = 1;
	do{
		diagRC = SQLGetDiagRec(SQL_HANDLE_ENV, henv, recordNumber, sqlState, &nativeError, messageText, SQL_NTS, NULL);
		if (diagRC == SQL_SUCCESS)
		{
			if (!printedErrorLogHeader){
				std::cout << "Diagnostics associated with environment handle:" << endl;
				printedErrorLogHeader = true;
			}
			std::cout << "SQL Diag " << recordNumber << endl
				<< "Native Error : " << nativeError << endl
				<< "SQL State : " << sqlState << endl
				<< "Message: " << messageText << endl;
		}
		recordNumber++;
	} while (diagRC == SQL_SUCCESS);

	/* Log any hdbc Diagnostics */
	recordNumber = 1;
	printedErrorLogHeader = false;
	do{
		diagRC = SQLGetDiagRec(SQL_HANDLE_DBC, hdbc, recordNumber, sqlState, &nativeError, messageText, sizeof(messageText), &totalNumber);
		if (diagRC == SQL_SUCCESS)
		{
			if (!printedErrorLogHeader){
				std::cout << "Diagnostics associated with connection handle:" << endl;
				printedErrorLogHeader = true;
			}
			std::cout << "SQL Diag " << recordNumber << endl
				<< dec << "Native Error : " << nativeError << endl
				<< "SQL State : " << sqlState << endl
				<< "Message: " << messageText << endl
				<< "TextLength: " << totalNumber << endl;
		}
		else if (diagRC == SQL_SUCCESS_WITH_INFO)
		{
			std::cout << "Error Msg Truncated, total number: " << dec << totalNumber << endl;
		}
		recordNumber++;
	} while (diagRC == SQL_SUCCESS);

	/* Log any hstmt Diagnostics */
	recordNumber = 1;
	printedErrorLogHeader = false;
	do{
		diagRC = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, recordNumber, sqlState, &nativeError, messageText, sizeof(messageText), &totalNumber);
		if (diagRC == SQL_SUCCESS)
		{
			if (!printedErrorLogHeader){
				std::cout << "Diagnostics associated with statmement handle:" << endl;
				printedErrorLogHeader = true;
			}
			std::cout << "SQL Diag " << recordNumber << endl
				<< dec << "Native Error : " << nativeError << endl
				<< "SQL State : " << sqlState << endl
				<< "Message: " << messageText << endl
				<< "TextLength: " << totalNumber << endl;
		}
		else if (diagRC == SQL_SUCCESS_WITH_INFO)
		{
			std::cout << "Error Msg Truncated, total number: " << dec << totalNumber << endl;
		}

		recordNumber++;
	} while (diagRC == SQL_SUCCESS);
}

void CTestBase::Disconnect()
{
	if (hstmt != SQL_NULL_HANDLE)
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	if (hdbc != SQL_NULL_HANDLE)
	{
		SQLDisconnect(hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	}
	if (henv != SQL_NULL_HANDLE)
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
}

char *SqlRetText(int rc)
{
	static char buffer[80];
	switch (rc)
	{
	case SQL_SUCCESS:
		return("SQL_SUCCESS");
	case SQL_SUCCESS_WITH_INFO:
		return("SQL_SUCCESS_WITH_INFO");
	case SQL_NO_DATA:
		return("SQL_NO_DATA");
	case SQL_ERROR:
		return("SQL_ERROR");
	case SQL_INVALID_HANDLE:
		return("SQL_INVALID_HANDLE");
	case SQL_STILL_EXECUTING:
		return("SQL_STILL_EXECUTING");
	case SQL_NEED_DATA:
		return("SQL_NEED_DATA");
	}
	sprintf(buffer, "SQL Error %d", rc);
	return(buffer);
}
