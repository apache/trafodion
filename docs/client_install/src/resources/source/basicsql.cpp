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

#ifdef __linux
    #include <unistd.h>
#else
    #include <windows.h>
    #include <tchar.h>
#endif

//#include <stdarg.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <sql.h>
#include <sqlext.h>

SQLHENV henv;
SQLHDBC hdbc;
SQLHSTMT hstmt;
SQLHWND hWnd;

#define MAX_SQLSTRING_LEN       1000
#define STATE_SIZE              6
#define MAX_CONNECT_STRING      256
#define TRUE                    1
#define FALSE                   0
#define ARGS                    "d:u:p:"

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
  SQLINTEGER nativeError;
  SQLCHAR messageText[SQL_MAX_MESSAGE_LENGTH];
  SQLCHAR sqlState[6];
  int diagsPrinted = 0;
  bool printedErrorLogHeader = false;
        
  printf("Function %s returned %s\n", sqlFunction, SqlRetText(rc));

  /* Log any henv Diagnostics */
  recordNumber = 1;
  do
  {
    diagRC = SQLGetDiagRec( SQL_HANDLE_ENV
                          , henv
                          , recordNumber
                          , sqlState
                          , &nativeError
                          , messageText
                          , sizeof(messageText)
                          , NULL
                          );
    if(diagRC==SQL_SUCCESS)
    {
      if(!printedErrorLogHeader)
      {
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
  do
  {
    diagRC = SQLGetDiagRec( SQL_HANDLE_DBC
                          , hdbc
                          , recordNumber
                          , sqlState
                          , &nativeError
                          , messageText
                          , sizeof(messageText)
                          , NULL
                          );
    if(diagRC==SQL_SUCCESS)
    {
      if(!printedErrorLogHeader)
      {
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
  do
  {
    diagRC = SQLGetDiagRec( SQL_HANDLE_STMT
                          , hstmt
                          , recordNumber
                          , sqlState
                          , &nativeError
                          , messageText
                          , sizeof(messageText)
                          , NULL
                          );
    if(diagRC==SQL_SUCCESS)
    {
      if(!printedErrorLogHeader)
      {
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
int main (int argc, char *argv[])
{
  unsigned char dsnName[20];
  unsigned char user[20];
  unsigned char password[20];
  SQLRETURN       returnCode;
  bool            testPassed = true;
  SQLCHAR         InConnStr[MAX_CONNECT_STRING];
  SQLCHAR         OutConnStr[MAX_CONNECT_STRING];
  SQLSMALLINT     ConnStrLength;
  int errflag = 0;
        
  //optarg = NULL;
  if (argc != 4)
     errflag++;

  if (!errflag )
  {
    strcpy ((char *)dsnName, argv[1]);
    strcpy ((char *)dsnName, argv[1]);
    strcpy ((char *)user, argv[2]);
    strcpy ((char *)password, argv[3]);
  }
  
  if (errflag) 
  {
    printf("Command line error.\n");
    printf("Usage: %s <datasource> <userid> <password>\n", argv[0] );
    return FALSE;
  }

  // Initialize handles to NULL
  henv = SQL_NULL_HANDLE;
  hstmt = SQL_NULL_HANDLE;
  hdbc = SQL_NULL_HANDLE;

  // Allocate Environment Handle
  returnCode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
  if (!SQL_SUCCEEDED(returnCode))
     LogDiagnostics("SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv)",returnCode);

  // Set ODBC version to 3.0
  returnCode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); 
  if (!SQL_SUCCEEDED(returnCode))
     LogDiagnostics( "SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0)"
                   , returnCode
                   , false
                   );

  // Allocate Connection handle
  returnCode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
  if (!SQL_SUCCEEDED(returnCode))
     LogDiagnostics("SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc)", returnCode);

  //Connect to the database
  sprintf((char*)InConnStr,"DSN=%s;UID=%s;PWD=%s;%c",(char*)dsnName, (char*)user, (char*)password,'\0');
  printf("Using Connect String: %s\n", InConnStr);
  returnCode = SQLDriverConnect( hdbc
                               , hWnd
                               , InConnStr
                               , SQL_NTS
                               , OutConnStr
                               , sizeof(OutConnStr)
                               , &ConnStrLength
                               , SQL_DRIVER_NOPROMPT
                               );
  if (!SQL_SUCCEEDED(returnCode))
     LogDiagnostics("SQLDriverConnect",returnCode);
  printf("Successfully connected using SQLDriverConnect.\n");

  //Allocate Statement handle
  returnCode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
  if (!SQL_SUCCEEDED(returnCode))
     LogDiagnostics("SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt)", returnCode);

  printf("Drop sample table if it exists...\n");
  //Drop the test table if it exists
  //DROP IF EXISTS TASKS;
  returnCode = SQLExecDirect(hstmt, (SQLCHAR*)"DROP TABLE IF EXISTS TASKS", SQL_NTS);
  if (!SQL_SUCCEEDED(returnCode))
     LogDiagnostics("SQLExecDirect of DROP", returnCode);

  printf("Creating sample table TASKS...\n");
  //Create a test table in default schema
  //CREATE TABLE TASKS (ID INT NOT NULL, TASK VARCHAR(10), LAST_UPDATE TIMESTAMP, PRIMARY KEY (C1));
  returnCode =
    SQLExecDirect
    ( hstmt
    , (SQLCHAR*)"CREATE TABLE TASKS (ID INT NOT NULL, TASK CHAR(20), COMPLETED DATE, PRIMARY KEY (ID))"
    , SQL_NTS
    );
  if (!SQL_SUCCEEDED(returnCode))
     LogDiagnostics("SQLExecDirect of CREATE", returnCode);
  printf("Table TASKS created using SQLExecDirect.\n");

  printf("Inserting data using SQLBindParameter, SQLPrepare, SQLExecute\n");
  //Insert few rows into test table using bound parameters
  //INSERT INTO TASKS VALUES (?, ?, ?);
  SQLINTEGER intID;
  SQLLEN cbID = 0, cbTask = SQL_NTS, cbCompleted = 0;
  SQLCHAR strTask[200];
  SQL_DATE_STRUCT dsCompleted;

  returnCode = SQLBindParameter( hstmt
			       , 1
			       , SQL_PARAM_INPUT
			       , SQL_C_SHORT
			       , SQL_INTEGER
			       , 0
			       , 0
			       , &intID
			       , 0
			       , &cbID
			       );
  if (!SQL_SUCCEEDED(returnCode))
     LogDiagnostics("SQLBindParameter 1", returnCode);

  returnCode = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, &strTask, 0, &cbTask);
  if (!SQL_SUCCEEDED(returnCode))
     LogDiagnostics("SQLBindParameter 2", returnCode);

  returnCode = SQLBindParameter( hstmt
			       , 3
			       , SQL_PARAM_INPUT
			       , SQL_C_TYPE_DATE
			       , SQL_DATE
			       , sizeof(dsCompleted)
			       , 0
			       , &dsCompleted
			       , 0
			       , &cbCompleted
			       );
  if (!SQL_SUCCEEDED(returnCode))
     LogDiagnostics("SQLBindParameter 3", returnCode);

  returnCode = SQLPrepare(hstmt, (SQLCHAR*)"INSERT INTO TASKS VALUES (?, ?, ?)", SQL_NTS);
  if (!SQL_SUCCEEDED(returnCode))
     LogDiagnostics("SQLPrepare of INSERT", returnCode);

  intID = 1000;
  strcpy ((char*)strTask, "CREATE REPORTS");
  dsCompleted.year = 2014;
  dsCompleted.month = 3;
  dsCompleted.day = 22;

  returnCode = SQLExecute(hstmt);
  if (!SQL_SUCCEEDED(returnCode))
     LogDiagnostics("SQLExecute", returnCode);
  printf("Data inserted.\n");

  //Select rows from test table and fetch the data
  //SELECT * from TASKS WHERE TASK LIKE '%REPORT%'
  printf("Fetching data using SQLExecDirect, SQLFetch, SQLGetData\n");
  returnCode = SQLExecDirect(hstmt, (SQLCHAR*)"SELECT ID, TASK, COMPLETED FROM TASKS", SQL_NTS);
  if (!SQL_SUCCEEDED(returnCode))
  LogDiagnostics("SQLExecDirect of SELECT", returnCode);
        
  //loop thru resultset
  while (TRUE) 
  {
    returnCode = SQLFetch(hstmt);
    if (returnCode == SQL_ERROR || returnCode == SQL_SUCCESS_WITH_INFO) 
    {
      LogDiagnostics("SQLFetch", returnCode);
    }
    if (returnCode == SQL_SUCCESS || returnCode == SQL_SUCCESS_WITH_INFO)
    {
      SQLGetData(hstmt, 1, SQL_C_SHORT, &intID, 0, &cbID);
      SQLGetData(hstmt, 2, SQL_C_CHAR, strTask, 20, &cbTask);
      SQLGetData(hstmt, 3, SQL_C_TYPE_DATE, &dsCompleted, sizeof(dsCompleted), &cbCompleted);
      printf( "Data selected: %d %s %d-%d-%d\n"
	    , intID
	    , strTask
	    , dsCompleted.year
	    , dsCompleted.month
	    , dsCompleted.day
	    );
    } 
    else 
      break;
  }
  
  //Free Statement handle
  returnCode = SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
  if (!SQL_SUCCEEDED(returnCode))
     LogDiagnostics("SQLFreeHandle(SQL_HANDLE_STMT, hstmt)", returnCode);
  hstmt = SQL_NULL_HANDLE;

  //Disconnect
  returnCode = SQLDisconnect(hdbc);
  if (!SQL_SUCCEEDED(returnCode))
     LogDiagnostics("SQLDisconnect(hdbc)", returnCode);

  //Free Connection handle
  returnCode = SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
  if (!SQL_SUCCEEDED(returnCode))
     LogDiagnostics("SQLFreeHandle(SQL_HANDLE_DBC, hdbc)", returnCode);
  hdbc = SQL_NULL_HANDLE;

  //Free Environment handle
  returnCode = SQLFreeHandle(SQL_HANDLE_ENV, henv);
  if (!SQL_SUCCEEDED(returnCode))
     LogDiagnostics("SQLFreeHandle(SQL_HANDLE_ENV, henv)", returnCode);
  henv = SQL_NULL_HANDLE;

  printf("Basic SQL ODBC Test Passed!\n");
  exit(EXIT_SUCCESS);
}
