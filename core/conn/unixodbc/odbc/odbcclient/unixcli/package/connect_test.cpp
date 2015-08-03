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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sql.h>
#include <sqlext.h>

SQLHENV henv;
SQLHDBC hdbc;
SQLHSTMT hstmt;
SQLHWND hWnd;

#define MAX_SQLSTRING_LEN	1000
#define STATE_SIZE		6
#define MAX_CONNECT_STRING      256
#define TRUE			1
#define FALSE			0
#define	ARGS			"d:u:p:"

const char *SqlRetText(int rc)
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
	sprintf(buffer,"SQL Error %d",rc);
	return(buffer);
}

void CleanUp()
{
	printf("\nConnect Test FAILED!!!\n");
	if(hstmt != SQL_NULL_HANDLE)
		SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
	if(hdbc != SQL_NULL_HANDLE)
	{
		SQLDisconnect(hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC,hdbc);
	}
	if(henv != SQL_NULL_HANDLE)
		SQLFreeHandle(SQL_HANDLE_ENV,henv);
	exit(EXIT_FAILURE);

}

void LogDiagnostics(const char *sqlFunction, SQLRETURN rc, bool exitOnError=true)
{             
	SQLRETURN diagRC = SQL_SUCCESS;
	SQLSMALLINT recordNumber;
	SQLINTEGER nativeError, diagColumn, diagRow;
	SQLCHAR messageText[SQL_MAX_MESSAGE_LENGTH];
	SQLCHAR sqlState[6];
	int diagsPrinted = 0;
	bool printedErrorLogHeader = false;
	
	printf("Function %s returned %s\n", sqlFunction, SqlRetText(rc));

	/* Log any henv Diagnostics */
	recordNumber = 1;
	do{
		diagRC = SQLGetDiagRec(SQL_HANDLE_ENV, henv, recordNumber,sqlState,&nativeError,messageText,sizeof(messageText),NULL);
		if(diagRC==SQL_SUCCESS)
		{
			if(!printedErrorLogHeader){
				printf("Diagnostics associated with environment handle:\n");
				printedErrorLogHeader = true;
			}
			printf("\n\tSQL Diag %d\n\tNative Error: %ld\n\tSQL State:    %s\n\tMessage:      %s\n",
				recordNumber,nativeError,sqlState,messageText);
		}
		recordNumber++;
	} while (diagRC==SQL_SUCCESS);
	
   /* Log any hdbc Diagnostics */
	recordNumber = 1;
	printedErrorLogHeader = false;
	do{
		diagRC = SQLGetDiagRec(SQL_HANDLE_DBC, hdbc, recordNumber,sqlState,&nativeError,messageText,sizeof(messageText),NULL);
		if(diagRC==SQL_SUCCESS)
		{
			if(!printedErrorLogHeader){
				printf("Diagnostics associated with connection handle:\n");
				printedErrorLogHeader = true;
			}
			printf("\n\tSQL Diag %d\n\tNative Error: %ld\n\tSQL State:    %s\n\tMessage:      %s\n",
				recordNumber,nativeError,sqlState,messageText);
		}
		recordNumber++;
	} while (diagRC==SQL_SUCCESS);

   /* Log any hstmt Diagnostics */
	recordNumber = 1;
	printedErrorLogHeader = false;
	do{
		diagRC = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, recordNumber,sqlState,&nativeError,messageText,sizeof(messageText),NULL);
		if(diagRC==SQL_SUCCESS)
		{
			if(!printedErrorLogHeader){
				printf("Diagnostics associated with statmement handle:\n");
				printedErrorLogHeader = true;
			}
			printf("\n\tSQL Diag %d\n\tNative Error: %ld\n\tSQL State:    %s\n\tMessage:      %s\n",
				recordNumber,nativeError,sqlState,messageText);
		}
		recordNumber++;
	} while (diagRC==SQL_SUCCESS);

	if(exitOnError && rc!=SQL_SUCCESS_WITH_INFO)
		CleanUp();
}                     

// Main Program
int main (int argc, char **argv)
{
	SQLCHAR		*dsnName;
	SQLCHAR		*user;
	SQLCHAR		*password;
	SQLRETURN	returnCode;
	bool		testPassed = true;
	SQLCHAR		InConnStr[MAX_CONNECT_STRING];
	SQLCHAR		OutConnStr[MAX_CONNECT_STRING];
	SQLSMALLINT	ConnStrLength;
	int c, errflag = 0;
	
	optarg = NULL;
	if (argc != 7)
		errflag++;

	while (!errflag && (c = getopt(argc, argv, ARGS)) != -1)
		switch (c) {
			case 'd':
				dsnName = (SQLCHAR*)optarg;	
				break;
			case 'u':
				user = (SQLCHAR*)optarg;
				break;
			case 'p':
				password = (SQLCHAR*)optarg;
				break;
			default :
				errflag++;
		}
	if (errflag) {
		printf("Command line error.\n");
		printf("Usage: %s [-d <datasource>] [-u <userid>] [-p <password>]\n", argv[0] );
		return FALSE;
	}

	// Initialize handles to NULL
	henv = SQL_NULL_HANDLE;
	hstmt = SQL_NULL_HANDLE;
	hdbc = SQL_NULL_HANDLE;

	// Allocate Environment Handle
	returnCode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
	if(returnCode != SQL_SUCCESS)
		LogDiagnostics("SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv)",returnCode);

	// Set ODBC version to 3.0
	returnCode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); 
	if(returnCode != SQL_SUCCESS)
		LogDiagnostics("SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0)",returnCode,false);

	// Allocate Connection handle
	returnCode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
	if(returnCode != SQL_SUCCESS)
		LogDiagnostics("SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc)", returnCode);

	//Connect to the database
	sprintf((char*)InConnStr,"DSN=%s;UID=%s;PWD=%s;%c",(char*)dsnName, (char*)user, (char*)password,'\0');
	printf("Using Connect String: %s\n", InConnStr);
	returnCode = SQLDriverConnect(hdbc,hWnd,InConnStr,SQL_NTS,OutConnStr,sizeof(OutConnStr),&ConnStrLength,SQL_DRIVER_NOPROMPT);
	if(returnCode != SQL_SUCCESS)
		LogDiagnostics("SQLDriverConnect",returnCode);

	//Allocate Statement handle
	returnCode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	if(returnCode != SQL_SUCCESS)
		LogDiagnostics("SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt)", returnCode);

	//Free Statement handle
	returnCode = SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	if(returnCode != SQL_SUCCESS)
		LogDiagnostics("SQLFreeHandle(SQL_HANDLE_STMT, hstmt)", returnCode);
	hstmt = SQL_NULL_HANDLE;

	//Disconnect
	returnCode = SQLDisconnect(hdbc);
	if(returnCode != SQL_SUCCESS)
		LogDiagnostics("SQLDisconnect(hdbc)", returnCode);

	//Free Connection handle
	returnCode = SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	if(returnCode != SQL_SUCCESS)
		LogDiagnostics("SQLFreeHandle(SQL_HANDLE_DBC, hdbc)", returnCode);
	hdbc = SQL_NULL_HANDLE;

	//Free Environment handle
	returnCode = SQLFreeHandle(SQL_HANDLE_ENV, henv);
	if(returnCode != SQL_SUCCESS)
		LogDiagnostics("SQLFreeHandle(SQL_HANDLE_ENV, henv)", returnCode);
	henv = SQL_NULL_HANDLE;

	printf("\nConnect Test Passed...\n");
	exit(EXIT_SUCCESS);
}


