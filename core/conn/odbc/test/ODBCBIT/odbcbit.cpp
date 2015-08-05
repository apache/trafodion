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
#include <windows.h>
#include <tchar.h>
#include <sql.h>
#include "odbcbit.h"

TestInfo	*pTestInfo;
TestInfo	testInfo;
HANDLE hInst;
SQLHWND hWnd;

//#####################################################################################################
// This functions cover the following APIs:
// SQLAllocEnv
// SQLAllocConnect
// SQLDriverConnect
// SQLAllocStmt
//#####################################################################################################
BOOL FullConnect()
{
	RETCODE returncode;
	_TCHAR	inConnectString[MAX_CONNECT_STRING];
	_TCHAR	outConnectString[MAX_CONNECT_STRING];
	short	ConnStrLength;
	_TCHAR	*pString;
	_TCHAR	*pTempString;
	_TCHAR	TempString[MAX_CONNECT_STRING];
	HENV	henv;
	HDBC	hdbc;
	HSTMT	hstmt;

	//returncode = SQLAllocEnv(&henv);
	returncode = SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&henv);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to Allocate Envirnoment"));
		return(FALSE);
	}

	returncode = SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, SQL_NTS);
	if (returncode != SQL_SUCCESS)
	{
		SQLFreeHandle (SQL_HANDLE_ENV, henv);
		henv = NULL;
		_tprintf(_T("Error: Connect Func: SQLSetEnvAttr"));
		//fprintf (logfile, "Error: Connect Func: SQLSetEnvAttr\r\n");
		return(FALSE);
	}

	//returncode = SQLAllocConnect(henv,&hdbc);
	returncode = SQLAllocHandle(SQL_HANDLE_DBC,henv,&hdbc);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to Allocate Connect"));
		//SQLFreeEnv(henv);
		SQLFreeHandle(SQL_HANDLE_ENV,henv);
		return(FALSE);
	}

	if (pTestInfo->DataSource[0] != 0)
		_stprintf(inConnectString,_T("DSN=%s;UID=%s;PWD=%s;"),pTestInfo->DataSource,pTestInfo->UserName,pTestInfo->Password);
	else
		_stprintf(inConnectString,_T("Driver={HP ODBC 3.0};SERVER=TCP:%s/%s;UID=%s;PWD=%s;"),pTestInfo->Server,pTestInfo->Port,pTestInfo->UserName,pTestInfo->Password);

	returncode = SQLDriverConnect(hdbc,hWnd,(SQLTCHAR*)inConnectString,_tcslen(inConnectString),
								  (SQLTCHAR*)outConnectString, sizeof(outConnectString)/sizeof(_TCHAR),&ConnStrLength,
		                          SQL_DRIVER_NOPROMPT);
	if ((returncode != SQL_SUCCESS) && (returncode != SQL_SUCCESS_WITH_INFO))
	{
		_tprintf(_T("Unable to Connect"));
		//SQLFreeConnect(hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC,hdbc);
		//SQLFreeEnv(henv);
		SQLFreeHandle(SQL_HANDLE_ENV,henv);
		return(FALSE);
	}
	
	//returncode = SQLAllocStmt(hdbc,&hstmt);
	returncode=SQLAllocHandle(SQL_HANDLE_STMT,hdbc,&hstmt);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to Allocate Statement"));
		SQLDisconnect(hdbc);
		//SQLFreeConnect(hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC,hdbc);
		//SQLFreeEnv(henv);
		SQLFreeHandle(SQL_HANDLE_ENV,henv);
		return(FALSE);
	}
      
   // Strip out DSN from connection string. 
	pString = _tcsstr(outConnectString,_T("DSN="));
	if (pString != NULL) 
	{
		pString += 4;           // skip past 'DSN=' 
		pTempString = TempString;
		while(*pString != ';')
		{
			*pTempString = *pString;
			pTempString++;
			pString++;
		}         
		*pTempString = (_TCHAR)NULL;
		_tcscpy(pTestInfo->DataSource,TempString);
	}
	// Strip out UID from connection string. 
	pString = _tcsstr(outConnectString,_T("UID="));
	if (pString != NULL)
	{
		pString += 4;           // skip past 'UID=' 
		pTempString = TempString;
		while(*pString != ';')
		{
			*pTempString = *pString;
			pTempString++;
			pString++;
		}         
		*pTempString = (_TCHAR)NULL;
		_tcscpy(pTestInfo->UserName,TempString);
	}
	else
		_tcscpy(pTestInfo->UserName,_T(""));

   // Strip out PWD from connection string. 
	pString = _tcsstr(outConnectString,_T("PWD="));
	if (pString != NULL)
	{
		pString += 4;           // skip past 'PWD=' 
		pTempString = TempString;
		while(*pString != ';')
		{
			*pTempString = *pString;
			pTempString++;
			pString++;
		}         
		*pTempString = (_TCHAR)NULL;
		_tcscpy((_TCHAR *)pTestInfo->Password,TempString);
	}
	else
		_tcscpy((_TCHAR *)pTestInfo->Password,_T(""));
   
	// Strip out DBQ from connection string. 
	pString = _tcsstr(outConnectString,_T("DBQ="));
	if (pString != NULL)
	{
		pString += 4;           // skip past 'DBQ=' 
		pTempString = TempString;
		while(*pString != ';')
		{
			*pTempString = *pString;
			pTempString++;
			pString++;
		}         
		*pTempString = (_TCHAR)NULL;
		_tcscpy((_TCHAR *)pTestInfo->Database,TempString);
	}
	else
		_tcscpy((_TCHAR *)pTestInfo->Database,_T("MASTER"));

	// Strip out CATALOG from connection string. 
	pString = _tcsstr(outConnectString,_T("CATALOG="));
	if (pString != NULL)
	{
		pString += 8;           // skip past 'CATALOG=' 
		pTempString = TempString;
		while(*pString != ';')
		{
			*pTempString = *pString;
			pTempString++;
			pString++;
		}         
		*pTempString = (_TCHAR)NULL;
		if (_tcscmp(pTempString, _T("")) != 0)
			_tcscpy((_TCHAR *)pTestInfo->Catalog,TempString);
	}

	// Strip out SCHEMA from connection string. 
	pString = _tcsstr(outConnectString,_T("SCHEMA="));
	if (pString != NULL)
	{
		pString += 7;           // skip past 'SCHEMA=' 
		pTempString = TempString;
		while(*pString != ';')
		{
			*pTempString = *pString;
			pTempString++;
			pString++;
		}         
		*pTempString = (_TCHAR)NULL;
		if (_tcscmp(pTempString, _T("")) != 0)
			_tcscpy((_TCHAR *)pTestInfo->Schema,TempString);
	}

	_tcscpy((_TCHAR *)pTestInfo->Table,_T("ODBCTAB"));

	// send the current handles to the caller 
   pTestInfo->henv = henv;
   pTestInfo->hdbc = hdbc;
   pTestInfo->hstmt = hstmt;
   
   return(TRUE);
}

void EnvErrorHandler (SQLHENV henv)
{
	_TCHAR buf[500];
	_TCHAR errbuf[600];
	_TCHAR State[6];
	SDWORD	NativeError;
	RETCODE returncode;
	int i = 1;

	returncode = SQLGetDiagRec (SQL_HANDLE_ENV, henv, i, (SQLTCHAR*)State, &NativeError, (SQLTCHAR*)buf, 500, NULL);
	while((returncode == SQL_SUCCESS) || (returncode == SQL_SUCCESS_WITH_INFO))
	{
		i++;
		State[5]=(_TCHAR)'\0';
		_stprintf(errbuf, _T("   State: %s\n   Native Error: %ld\n   Error: %s\n"),State,NativeError,buf);
		_tprintf(errbuf,State);
		returncode = SQLGetDiagRec (SQL_HANDLE_ENV, henv, i, (SQLTCHAR*)State, &NativeError, (SQLTCHAR*)buf, 500, NULL);
	}
}

void DBCErrorHandler (SQLHDBC hdbc)
{
	_TCHAR buf[500];
	_TCHAR errbuf[600];
	_TCHAR State[6];
	SDWORD	NativeError;
	RETCODE returncode;
	int i = 1;

	returncode = SQLGetDiagRec (SQL_HANDLE_DBC, hdbc, i, (SQLTCHAR*)State, &NativeError, (SQLTCHAR*)buf, 500, NULL);
	while((returncode == SQL_SUCCESS) || (returncode == SQL_SUCCESS_WITH_INFO))
	{
		i++;
		State[5]=(_TCHAR)'\0';
		_stprintf(errbuf, _T("   State: %s\n   Native Error: %ld\n   Error: %s\n"),State,NativeError,buf);
		_tprintf(errbuf,State);
		returncode = SQLGetDiagRec (SQL_HANDLE_DBC, hdbc, i, (SQLTCHAR*)State, &NativeError, (SQLTCHAR*)buf, 500, NULL);
	}
}

void StmtErrorHandler (SQLHSTMT hstmt)
{
	_TCHAR buf[500];
	_TCHAR errbuf[600];
	_TCHAR State[6];
	SDWORD	NativeError;
	RETCODE returncode;
	int i = 1;

	returncode = SQLGetDiagRec (SQL_HANDLE_STMT, hstmt, i, (SQLTCHAR*)State, &NativeError, (SQLTCHAR*)buf, 500, NULL);
	while((returncode == SQL_SUCCESS) || (returncode == SQL_SUCCESS_WITH_INFO))
	{
		i++;
		State[5]=(_TCHAR)'\0';
		_stprintf(errbuf, _T("   State: %s\r\n   Native Error: %ld\r\n   Error: %s\r\n"),State,NativeError,buf);
		_tprintf(errbuf,State);
		returncode = SQLGetDiagRec (SQL_HANDLE_STMT, hstmt, i, (SQLTCHAR*)State, &NativeError, (SQLTCHAR*)buf, 500, NULL);
	}
}

void LogAllErrors (TestInfo *pTestInfo/*SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt*/)
{
	EnvErrorHandler (pTestInfo->henv);
	DBCErrorHandler (pTestInfo->hdbc);
	StmtErrorHandler (pTestInfo->hstmt);
}

_TCHAR *SetSchema()
{
	_stprintf(SQLStmt,_T("set schema %s.%s"),pTestInfo->Catalog,pTestInfo->Schema);
	return (SQLStmt);
}

_TCHAR *CreateCatalog()
{
	_stprintf(SQLStmt,_T("create catalog %s"),pTestInfo->Catalog);
	return (SQLStmt);
}

_TCHAR *CreateSchema()
{
	_stprintf(SQLStmt,_T("create schema %s"),pTestInfo->Schema);
	return (SQLStmt);
}

_TCHAR *DropTable()
{
	_stprintf(SQLStmt,_T("%s %s"),DROP_TABLE, pTestInfo->Table);
	return(SQLStmt);
}

_TCHAR *CreateTable()
{
	int i;
	_TCHAR *pk;		// primary key string
	BOOL YN = FALSE;

	pk = (_TCHAR *) malloc(sizeof(_TCHAR)*300);
	_tcscpy(pk,_T(""));

	_tcscpy(SQLStmt,_T(""));
	_tcscat(SQLStmt,CREATE_TABLE);
	_tcscat (SQLStmt, pTestInfo->Catalog);
	_tcscat(SQLStmt, _T("."));
	_tcscat(SQLStmt, pTestInfo->Schema);
	_tcscat(SQLStmt, _T("."));
	_tcscat(SQLStmt,pTestInfo->Table);
	_tcscat(SQLStmt,_T(" ("));
	i = 0;
	while (_tcscmp(ColumnInfo[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (_tcscmp(ColumnInfo[i].DataTypestr,END_LOOP) != 0))
			_tcscat(SQLStmt,_T(","));
		_tcscat(SQLStmt,ColumnInfo[i].Name);
		_tcscat(SQLStmt,_T(" "));
		_tcscat(SQLStmt,ColumnInfo[i].Description);
		if (_tcscmp(ColumnInfo[i].Precision,_T("")) != 0)
		{
			_tcscat(SQLStmt,_T("("));
			_tcscat(SQLStmt,ColumnInfo[i].Precision);
			if (_tcscmp(ColumnInfo[i].Scale,_T("")) != 0)
			{
				_tcscat(SQLStmt,_T(","));
				_tcscat(SQLStmt,ColumnInfo[i].Scale);
			}
			_tcscat(SQLStmt,_T(")"));
		}
		if (ColumnInfo[i].PriKey)
		{
			_tcscat(SQLStmt,_T(" NOT NULL"));
			if (YN)
				_tcscat(pk,_T(","));
			_tcscat(pk,ColumnInfo[i].Name);
  		YN = TRUE;
		}
		i++;
	}

	
	if (_tcscmp(pk,_T("")) != 0)
	{
		_tcscat(SQLStmt,_T(", PRIMARY KEY ("));
		_tcscat(SQLStmt,pk);
		_tcscat(SQLStmt,_T(")"));
	}
	_tcscat(SQLStmt,_T(")"));
	Actual_Num_Columns = i;
	free(pk); 

	return (SQLStmt);

}

_TCHAR *CreateTable2()
{
	int i;
	_TCHAR *pk;		// primary key string
	BOOL YN = FALSE;

	pk = (_TCHAR *) malloc(sizeof(_TCHAR)*300);
	_tcscpy(pk,_T(""));

	_tcscpy(SQLStmt,_T(""));
	_tcscat(SQLStmt,CREATE_TABLE);
	_tcscat (SQLStmt, pTestInfo->Catalog);
	_tcscat(SQLStmt, _T("."));
	_tcscat(SQLStmt, pTestInfo->Schema);
	_tcscat(SQLStmt, _T("."));
	_tcscat(SQLStmt,pTestInfo->Table);
	_tcscat(SQLStmt,_T(" ("));
	i = 0;
	while (_tcscmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (_tcscmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0))
			_tcscat(SQLStmt,_T(","));
		_tcscat(SQLStmt,ColumnInfo2[i].Name);
		_tcscat(SQLStmt,_T(" "));
		_tcscat(SQLStmt,ColumnInfo2[i].Description);
		if (_tcscmp(ColumnInfo2[i].Precision,_T("")) != 0)
		{
			_tcscat(SQLStmt,_T("("));
			_tcscat(SQLStmt,ColumnInfo2[i].Precision);
			if (_tcscmp(ColumnInfo2[i].Scale,_T("")) != 0)
			{
				_tcscat(SQLStmt,_T(","));
				_tcscat(SQLStmt,ColumnInfo2[i].Scale);
			}
			_tcscat(SQLStmt,_T(")"));
		}
		if (ColumnInfo2[i].PriKey)
		{
			_tcscat(SQLStmt,_T(" NOT NULL"));
			if (YN)
				_tcscat(pk,_T(","));
			_tcscat(pk,ColumnInfo2[i].Name);
  		YN = TRUE;
		}
		else
		{
			_tcscat(SQLStmt,_T(" default "));
			_tcscat(SQLStmt,ColumnInfo2[i].Default);
		}
		i++;
	}

	
	if (_tcscmp(pk,_T("")) != 0)
	{
		_tcscat(SQLStmt,_T(", PRIMARY KEY ("));
		_tcscat(SQLStmt,pk);
		_tcscat(SQLStmt,_T(")"));
	}
	_tcscat(SQLStmt,_T(")"));
	Actual_Num_Columns = i;
	free(pk); 

	return (SQLStmt);

}

_TCHAR *CreateTable3()
{
	int i;
	_TCHAR *pk;		// primary key string
	BOOL YN = FALSE;

	pk = (_TCHAR *) malloc(sizeof(_TCHAR)*300);
	_tcscpy(pk,_T(""));

	_tcscpy(SQLStmt,_T(""));
	_tcscat(SQLStmt,CREATE_TABLE);
	_tcscat (SQLStmt, pTestInfo->Catalog);
	_tcscat(SQLStmt, _T("."));
	_tcscat(SQLStmt, pTestInfo->Schema);
	_tcscat(SQLStmt, _T("."));
	_tcscat(SQLStmt,pTestInfo->Table);
	_tcscat(SQLStmt,_T(" ("));
	i = 0;
	while (_tcscmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (_tcscmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0))
			_tcscat(SQLStmt,_T(","));
		_tcscat(SQLStmt,ColumnInfo3[i].Name);
		_tcscat(SQLStmt,_T(" "));
		_tcscat(SQLStmt,ColumnInfo3[i].Description);
		if (_tcscmp(ColumnInfo3[i].Precision,_T("")) != 0)
		{
			_tcscat(SQLStmt,_T("("));
			_tcscat(SQLStmt,ColumnInfo3[i].Precision);
			if (_tcscmp(ColumnInfo3[i].Charset,_T("character set UTF8")) == 0)
			{
				_tcscat(SQLStmt,_T(" bytes"));
			}
			if (_tcscmp(ColumnInfo3[i].Scale,_T("")) != 0)
			{
				_tcscat(SQLStmt,_T(","));
				_tcscat(SQLStmt,ColumnInfo3[i].Scale);
			}
			_tcscat(SQLStmt,_T(")"));
			if (_tcscmp(ColumnInfo3[i].Charset,_T("")) != 0)
			{
				_tcscat(SQLStmt,ColumnInfo3[i].Charset);
			}
		}
		if (ColumnInfo3[i].PriKey)
		{
			_tcscat(SQLStmt,_T(" NOT NULL"));
			if (YN)
				_tcscat(pk,_T(","));
			_tcscat(pk,ColumnInfo3[i].Name);
  		YN = TRUE;
		}
		i++;
	}

	
	if (_tcscmp(pk,_T("")) != 0)
	{
		_tcscat(SQLStmt,_T(", PRIMARY KEY ("));
		_tcscat(SQLStmt,pk);
		_tcscat(SQLStmt,_T(")"));
	}
	_tcscat(SQLStmt,_T(")"));
	Actual_Num_Columns = i;
	free(pk); 

	return (SQLStmt);

}

_TCHAR *InsertTable()
{
	int i;

	_tcscpy(SQLStmt,_T(""));
	_tcscat(SQLStmt,INSERT_TABLE);
	_tcscat(SQLStmt,pTestInfo->Table);
	_tcscat(SQLStmt,_T(" ("));
	i = 0;
	while (_tcscmp(ColumnInfo[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (_tcscmp(ColumnInfo[i].DataTypestr,END_LOOP) != 0))
			_tcscat(SQLStmt,_T(","));
		_tcscat(SQLStmt,ColumnInfo[i].Name);
		i++;
	}
	_tcscat(SQLStmt,_T(") values ("));
	
	i = 0;
	while (_tcscmp(ColumnInfo[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (_tcscmp(ColumnInfo[i].DataTypestr,END_LOOP) != 0))
			_tcscat(SQLStmt,_T(","));
		_tcscat(SQLStmt,_T("?"));
		i++;
	}
	_tcscat(SQLStmt,_T(")"));

	return (SQLStmt);
}

_TCHAR *InsertTable2()
{
	int i;

	_tcscpy(SQLStmt,_T(""));
	_tcscat(SQLStmt,INSERT_TABLE);
	_tcscat(SQLStmt,pTestInfo->Table);
	_tcscat(SQLStmt,_T(" ("));
	i = 0;
	while (_tcscmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (_tcscmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0))
			_tcscat(SQLStmt,_T(","));
		_tcscat(SQLStmt,ColumnInfo2[i].Name);
		i++;
	}
	_tcscat(SQLStmt,_T(") values ("));
	
	i = 0;
	while (_tcscmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (_tcscmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0))
			_tcscat(SQLStmt,_T(","));
		_tcscat(SQLStmt,_T("?"));
		i++;
	}
	_tcscat(SQLStmt,_T(")"));

	return (SQLStmt);
}

_TCHAR *InsertTableWithValues2()
{
	int i;

	_tcscpy(SQLStmt,_T(""));
	_tcscat(SQLStmt,INSERT_TABLE);
	_tcscat(SQLStmt,pTestInfo->Table);
	_tcscat(SQLStmt,_T(" ("));
	
	i = 0;
	while (_tcscmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (_tcscmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0))
			_tcscat(SQLStmt,_T(","));
		_tcscat(SQLStmt,ColumnInfo2[i].Name);
		i++;
	}

	_tcscat(SQLStmt,_T(") values ("));
	
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].LongVarcharValue);
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalYearValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalMonthValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalYearToMonthValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalDayValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalHourValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalMinuteValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalSecondValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalDayToHourValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalDayToMinuteValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalDayToSecondValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalHourToMinuteValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalHourToSecondValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalMinuteToSecondValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].DateValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].TimeValue); 
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].TimestampValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].CharValue);
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,_T(")"));

	return (SQLStmt);
}

_TCHAR *InsertTableWithDefaults2()
{
	int i;

	_tcscpy(SQLStmt,_T(""));
	_tcscat(SQLStmt,INSERT_TABLE);
	_tcscat(SQLStmt,pTestInfo->Table);
	_tcscat(SQLStmt,_T(" ("));
	
	i = 0;
//	while (_tcscmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0)
//	{
//		if ((i != 0) && (_tcscmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0))
//			_tcscat(SQLStmt,_T(","));
//		_tcscat(SQLStmt,ColumnInfo2[i].Name);
		_tcscat(SQLStmt,_T("COLUMN_CHAR"));
//		i++;
//	}

	_tcscat(SQLStmt,_T(") values ("));
	
	//_tcscat(SQLStmt,_T("'"));
	//_tcscat(SQLStmt,InputOutputValues2[rowdata].LongVarcharValue);
	//_tcscat(SQLStmt,_T("'"));
	//_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].CharValue);
	_tcscat(SQLStmt,_T("'"));
/*	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalYearValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalMonthValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalYearToMonthValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalDayValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalHourValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalMinuteValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalSecondValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalDayToHourValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalDayToMinuteValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalDayToSecondValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalHourToMinuteValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalHourToSecondValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].IntervalMinuteToSecondValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].DateValue);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].TimeValue); 
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,InputOutputValues2[rowdata].TimestampValue);*/
	_tcscat(SQLStmt,_T(")"));

	return (SQLStmt);
}

_TCHAR *InsertTable3()
{
	int i;

	_tcscpy(SQLStmt,_T(""));
	_tcscat(SQLStmt,INSERT_TABLE);
	_tcscat(SQLStmt,pTestInfo->Table);
	_tcscat(SQLStmt,_T(" ("));
	i = 0;
	while (_tcscmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (_tcscmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0))
			_tcscat(SQLStmt,_T(","));
		_tcscat(SQLStmt,ColumnInfo3[i].Name);
		i++;
	}
	_tcscat(SQLStmt,_T(") values ("));
	
	i = 0;
	while (_tcscmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (_tcscmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0))
			_tcscat(SQLStmt,_T(","));
		_tcscat(SQLStmt,_T("?"));
		i++;
	}
	_tcscat(SQLStmt,_T(")"));

	return (SQLStmt);
}

_TCHAR *InsertTableWithValues3()
{
	int i;
	TCHAR temp[10];

	_tcscpy(SQLStmt,_T(""));
	_tcscat(SQLStmt,INSERT_TABLE);
	_tcscat(SQLStmt,pTestInfo->Table);
	_tcscat(SQLStmt,_T(" ("));
	
	i = 0;
	while (_tcscmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (_tcscmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0))
			_tcscat(SQLStmt,_T(","));
		_tcscat(SQLStmt,ColumnInfo3[i].Name);
		i++;
	}

	_tcscat(SQLStmt,_T(") values ("));
	
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,InputOutputValues3[rowdata].UTF8CharValue);
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,InputOutputValues3[rowdata].UTF8VarcharValue);
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,InputOutputValues3[rowdata].WCharValue1);
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,InputOutputValues3[rowdata].WCharValue2);
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,InputOutputValues3[rowdata].WVarcharValue);
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,_T("'"));
	_stprintf(temp, _T("%d"), InputOutputValues3[rowdata].BitValue);
	_tcscat(SQLStmt,(const TCHAR*)temp);
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,_T(","));
	_stprintf(temp, _T("%d"), InputOutputValues3[rowdata].TinyintValue);
	_tcscat(SQLStmt,(const TCHAR*)temp);
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,InputOutputValues3[rowdata].BinaryValue);
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,_T(","));
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,InputOutputValues3[rowdata].VarBinaryValue);
	_tcscat(SQLStmt,_T("'"));
	_tcscat(SQLStmt,_T(")"));

	return (SQLStmt);
}

_TCHAR *SelectTable()
{
	int i;

	_tcscpy(SQLStmt,_T(""));
	_tcscat(SQLStmt,SELECT_TABLE);
	i = 0;
	while (_tcscmp(ColumnInfo[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (_tcscmp(ColumnInfo[i].DataTypestr,END_LOOP) != 0))
			_tcscat(SQLStmt,_T(","));
		_tcscat(SQLStmt,ColumnInfo[i].Name);
		i++;
	}
	_tcscat(SQLStmt,_T(" from "));
	_tcscat(SQLStmt,pTestInfo->Table);
	_tcscat(SQLStmt,_T(" order by 1 "));
	return (SQLStmt);

}

_TCHAR *SelectTable2()
{
	int i;

	_tcscpy(SQLStmt,_T(""));
	_tcscat(SQLStmt,SELECT_TABLE);
	i = 0;
	while (_tcscmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (_tcscmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0))
			_tcscat(SQLStmt,_T(","));
		_tcscat(SQLStmt,ColumnInfo2[i].Name);
		i++;
	}
	_tcscat(SQLStmt,_T(" from "));
	_tcscat(SQLStmt,pTestInfo->Table);
	_tcscat(SQLStmt,_T(" order by 1 "));
	return (SQLStmt);

}

_TCHAR *SelectTable3()
{
	int i;

	_tcscpy(SQLStmt,_T(""));
	_tcscat(SQLStmt,SELECT_TABLE);
	i = 0;
	while (_tcscmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (_tcscmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0))
			_tcscat(SQLStmt,_T(","));
		_tcscat(SQLStmt,ColumnInfo3[i].Name);
		i++;
	}
	_tcscat(SQLStmt,_T(" from "));
	_tcscat(SQLStmt,pTestInfo->Table);
	//_tcscat(SQLStmt,_T(" order by 1 ");*/
	return (SQLStmt);

}

void ReleaseAll(TestInfo *pTestInfo)
{
//		SQLFreeStmt(pTestInfo->hstmt,SQL_DROP);
		SQLFreeHandle (SQL_HANDLE_STMT, pTestInfo->hstmt);
		SQLDisconnect(pTestInfo->hdbc);
//		SQLFreeConnect(pTestInfo->hdbc);
		SQLFreeHandle (SQL_HANDLE_DBC, pTestInfo->hdbc);
//		SQLFreeEnv(pTestInfo->henv);
		SQLFreeHandle (SQL_HANDLE_ENV, pTestInfo->henv);
}

//#####################################################################################################
// This functions cover the following APIs:
// SQLExecDirect
// SQLSetConnectOption
// SQLGetConnectOption
//#####################################################################################################
BOOL InitialSetup()
{
	RETCODE		returncode;                        
	PTR			pvParam;
 	
	returncode = SQLSetConnectOption(pTestInfo->hdbc,SQL_ACCESS_MODE,SQL_MODE_READ_WRITE);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to set access mode to read/write"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLGetConnectOption(pTestInfo->hdbc,SQL_ACCESS_MODE,&pvParam);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to get access mode to read/write"));
	}
	
	if ((UDWORD)pvParam != SQL_MODE_READ_WRITE)
	{
		_tprintf(_T("Invalid access mode value for read/write returned"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLSetConnectOption(pTestInfo->hdbc,SQL_TXN_ISOLATION,SQL_TXN_READ_COMMITTED);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to set transaction isolation mode to read committed"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLGetConnectOption(pTestInfo->hdbc,SQL_TXN_ISOLATION,&pvParam);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to get transaction isolation mode to read committed"));
	}

	if ((UDWORD)pvParam != SQL_TXN_READ_COMMITTED)
	{
		_tprintf(_T("Invalid access mode value for read/write returned"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	SQLExecDirect(pTestInfo->hstmt, (SQLTCHAR *)SetSchema(),SQL_NTS);

	SQLExecDirect(pTestInfo->hstmt,(SQLTCHAR *)DropTable(),SQL_NTS); // Cleanup
	
	returncode = SQLExecDirect(pTestInfo->hstmt,(SQLTCHAR *)CreateTable(),SQL_NTS);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to Create Table"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	
	return(TRUE);
}	

BOOL InitialSetup2()
{
	RETCODE		returncode;                        
	PTR			pvParam;
 	
	returncode = SQLSetConnectOption(pTestInfo->hdbc,SQL_ACCESS_MODE,SQL_MODE_READ_WRITE);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to set access mode to read/write"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLGetConnectOption(pTestInfo->hdbc,SQL_ACCESS_MODE,&pvParam);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to get access mode to read/write"));
	}
	
	if ((UDWORD)pvParam != SQL_MODE_READ_WRITE)
	{
		_tprintf(_T("Invalid access mode value for read/write returned"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLSetConnectOption(pTestInfo->hdbc,SQL_TXN_ISOLATION,SQL_TXN_READ_COMMITTED);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to set transaction isolation mode to read committed"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLGetConnectOption(pTestInfo->hdbc,SQL_TXN_ISOLATION,&pvParam);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to get transaction isolation mode to read committed"));
	}

	if ((UDWORD)pvParam != SQL_TXN_READ_COMMITTED)
	{
		_tprintf(_T("Invalid access mode value for read/write returned"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	SQLExecDirect(pTestInfo->hstmt,(SQLTCHAR *)DropTable(),SQL_NTS); // Cleanup
	
	returncode = SQLExecDirect(pTestInfo->hstmt,(SQLTCHAR *)CreateTable2(),SQL_NTS);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to Create Table - 2"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	
	return(TRUE);
}
	
BOOL InitialSetup3()
{
	RETCODE		returncode;                        
	PTR			pvParam;
 	
	returncode = SQLSetConnectOption(pTestInfo->hdbc,SQL_ACCESS_MODE,SQL_MODE_READ_WRITE);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to set access mode to read/write"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLGetConnectOption(pTestInfo->hdbc,SQL_ACCESS_MODE,&pvParam);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to get access mode to read/write"));
	}
	
	if ((UDWORD)pvParam != SQL_MODE_READ_WRITE)
	{
		_tprintf(_T("Invalid access mode value for read/write returned"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLSetConnectOption(pTestInfo->hdbc,SQL_TXN_ISOLATION,SQL_TXN_READ_COMMITTED);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to set transaction isolation mode to read committed"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLGetConnectOption(pTestInfo->hdbc,SQL_TXN_ISOLATION,&pvParam);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to get transaction isolation mode to read committed"));
	}

	if ((UDWORD)pvParam != SQL_TXN_READ_COMMITTED)
	{
		_tprintf(_T("Invalid access mode value for read/write returned"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	SQLExecDirect(pTestInfo->hstmt,(SQLTCHAR *)DropTable(),SQL_NTS); // Cleanup
	
	returncode = SQLExecDirect(pTestInfo->hstmt,(SQLTCHAR *)CreateTable3(),SQL_NTS);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to Create Table - 3"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	
	return(TRUE);
}

//#####################################################################################################
// This functions cover the following APIs:
// SQLExecDirect
//#####################################################################################################
BOOL Cleanup()
{
	RETCODE		returncode;                        
	_TCHAR		*temp;
	ULONG		pvParam;

	temp = (_TCHAR *)malloc(sizeof(_TCHAR)*50);
	_tcscpy(temp,_T(""));
	_tcscpy(temp,DropTable());

	returncode = SQLSetStmtOption(pTestInfo->hstmt,SQL_ASYNC_ENABLE,SQL_ASYNC_ENABLE_ON);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to set ASYNC mode to ENABLE"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode = SQLGetStmtOption(pTestInfo->hstmt,SQL_ASYNC_ENABLE,&pvParam);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to get ASYNC mode to ENABLE"));
	}

	if (pvParam != SQL_ASYNC_ENABLE_ON)
	{
		_tprintf(_T("Invalid ASYNC mode value for ENABLE returned"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQL_STILL_EXECUTING;
	while (returncode == SQL_STILL_EXECUTING)
	{
		returncode = SQLExecDirect(pTestInfo->hstmt,(SQLTCHAR *)temp,SQL_NTS);
	}
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to Drop Table"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLSetStmtOption(pTestInfo->hstmt,SQL_ASYNC_ENABLE,SQL_ASYNC_ENABLE_OFF);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to set ASYNC mode to OFF"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	free(temp);

	return(TRUE);
}	


//#####################################################################################################
// This functions cover the following APIs:
// SQLDisconnect
// SQLFreeConnect
// SQLFreeEnv
//#####################################################################################################

BOOL FullDisconnect()
{
	RETCODE returncode;                        
   
	returncode = SQLDisconnect(pTestInfo->hdbc);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to Disconnect"));
		SQLDisconnect(pTestInfo->hdbc);
//		SQLFreeConnect(pTestInfo->hdbc);
//		SQLFreeEnv(pTestInfo->henv);
		SQLFreeHandle (SQL_HANDLE_DBC, pTestInfo->hdbc);
		SQLFreeHandle (SQL_HANDLE_ENV, pTestInfo->henv);
		return(FALSE);
	}
   
	returncode = SQLFreeConnect(pTestInfo->hdbc);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to FreeConnect"));
		SQLDisconnect(pTestInfo->hdbc);
//		SQLFreeConnect(pTestInfo->hdbc);
//		SQLFreeEnv(pTestInfo->henv);
		SQLFreeHandle (SQL_HANDLE_DBC, pTestInfo->hdbc);
		SQLFreeHandle (SQL_HANDLE_ENV, pTestInfo->henv);
		return(FALSE);
	}

   
	returncode = SQLFreeEnv(pTestInfo->henv);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to FreeEnvirnoment"));
		SQLDisconnect(pTestInfo->hdbc);
//		SQLFreeConnect(pTestInfo->hdbc);
//		SQLFreeEnv(pTestInfo->henv);
		SQLFreeHandle (SQL_HANDLE_DBC, pTestInfo->hdbc);
		SQLFreeHandle (SQL_HANDLE_ENV, pTestInfo->henv);
		return(FALSE);
	}
  
  return(TRUE);
}

//#####################################################################################################
// This functions cover the following APIs:
// SQLDatasources
// SQLDrivers
// SQLGetInfo
//#####################################################################################################
BOOL GetAllInfo()
{
	RETCODE returncode;                        
	_TCHAR		szDSN[SQL_MAX_SERVER_LEN], szDESC[SQL_MAX_SERVER_LEN];
	SWORD		cbDSN, pcbDESC;
	_TCHAR		szDRVDESC[SQL_MAX_DRIVER_LENGTH], szDRVATTR[SQL_MAX_DRIVER_LENGTH];
	SWORD		cbDRVDESC, pcbDRVATTR;
	PTR			fFuncs;

	returncode = SQLDataSources(pTestInfo->henv, SQL_FETCH_FIRST, (SQLTCHAR*)szDSN, SQL_MAX_SERVER_LEN, &cbDSN, (SQLTCHAR*)szDESC, SQL_MAX_SERVER_LEN, &pcbDESC); 
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to Fetch first datasource"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

/*	returncode = SQLDataSources(pTestInfo->henv, SQL_FETCH_NEXT, szDSN, SQL_MAX_DSN_LENGTH, &cbDSN, szDESC, SQL_MAX_DSN_LENGTH, &pcbDESC); 
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to Fetch next datasource"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	} */

	returncode = SQLDrivers(pTestInfo->henv, SQL_FETCH_FIRST, (SQLTCHAR*)szDRVDESC, SQL_MAX_DRIVER_LENGTH, &cbDRVDESC, (SQLTCHAR*)szDRVATTR, SQL_MAX_DRIVER_LENGTH, &pcbDRVATTR); 
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to Fetch first driver"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

/*	returncode = SQLDrivers(pTestInfo->henv, SQL_FETCH_NEXT, szDRVDESC, SQL_MAX_DRIVER_LENGTH, &cbDRVDESC, szDRVATTR, SQL_MAX_DRIVER_LENGTH), &pcbDRVATTR); 
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to Fetch next driver"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	} */

	fFuncs = (_TCHAR *)malloc(sizeof(_TCHAR)*SQL_MAX_SERVER_LEN);
	returncode = SQLGetInfo(pTestInfo->hdbc, SQL_DATA_SOURCE_NAME, fFuncs, SQL_MAX_SERVER_LEN*sizeof(_TCHAR), NULL);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to get active connections"));
	}
	if (_tcscmp((_TCHAR *)fFuncs,pTestInfo->DataSource) != 0)
	{
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	free(fFuncs);

	return(TRUE);

}


//#####################################################################################################
// This functions cover the following APIs:
// SQLPrepare
// SQLDescribeParam
// SQLExecute
// SQLBindParam
// SQLNumParams
// SQLSet/GetCursorName
//#####################################################################################################
BOOL StatementFunctions()
{
	RETCODE		returncode;                        
	SWORD		numparams;
	_TCHAR		temp[250];
	int			iparam, num_rows_insert;
	SWORD		paramSQLDataType;
	SQLULEN		paramColDef;
	SWORD		paramColScale,paramColNull;

	num_rows_insert = 0;
	while (_tcscmp(InputOutputValues[num_rows_insert].CharValue,END_LOOP) != 0)
	{
		returncode = SQLPrepare(pTestInfo->hstmt,(SQLTCHAR *)InsertTable(),SQL_NTS);
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to prepare insert Table"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

		returncode = SQLNumParams(pTestInfo->hstmt,&numparams);
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to return number of parameters for insert statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		if (numparams != Actual_Num_Columns)
		{
			_stprintf(temp,_T("Number of parameters doesn't match expected: %d and actual: %d"),numparams,Actual_Num_Columns); 
			_tprintf(temp);
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

	// Begin of bind parameter
		returncode = SQLBindParameter(pTestInfo->hstmt, 1, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_CHAR, MAX_SQLSTRING_LEN, 0, InputOutputValues[num_rows_insert].CharValue, 0, &(InputOutputValues[num_rows_insert].InValue));
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_CHAR during insert statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 2, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_VARCHAR, MAX_SQLSTRING_LEN, 0, InputOutputValues[num_rows_insert].VarcharValue, 0, &(InputOutputValues[num_rows_insert].InValue));
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_VARCHAR during insert statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 3, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_DECIMAL, 0, 0, InputOutputValues[num_rows_insert].DecimalValue, 0, &(InputOutputValues[num_rows_insert].InValue));
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_DECIMAL during insert statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 4, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_NUMERIC, 0, 0, InputOutputValues[num_rows_insert].NumericValue, 0, &(InputOutputValues[num_rows_insert].InValue));
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_NUMERIC during insert statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 5, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, 0, 0, &(InputOutputValues[num_rows_insert].ShortValue), 0, &(InputOutputValues[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to convert from SQL_C_SHORT to SQL_SMALLINT during insert statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 6, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &(InputOutputValues[num_rows_insert].LongValue), 0, &(InputOutputValues[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to convert from SQL_C_LONG to SQL_INTEGER during insert statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 7, SQL_PARAM_INPUT, SQL_C_FLOAT, SQL_REAL, 0, 0, &(InputOutputValues[num_rows_insert].RealValue), 0, &(InputOutputValues[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to convert from SQL_C_FLOAT to SQL_REAL during insert statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 8, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_FLOAT, 0, 0, &(InputOutputValues[num_rows_insert].FloatValue), 0, &(InputOutputValues[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to convert from SQL_C_DOUBLE to SQL_FLOAT during insert statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 9, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE, 0, 0, &(InputOutputValues[num_rows_insert].DoubleValue), 0, &(InputOutputValues[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to convert from SQL_C_DOUBLE to SQL_DOUBLE during insert statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 10, SQL_PARAM_INPUT, SQL_C_DATE, SQL_TYPE_DATE, 0, 0, &(InputOutputValues[num_rows_insert].DateValue), 0, &(InputOutputValues[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to convert from SQL_C_DATE to SQL_TYPE_DATE during insert statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 11, SQL_PARAM_INPUT, SQL_C_TIME, SQL_TYPE_TIME, 0, 0, &(InputOutputValues[num_rows_insert].TimeValue), 0, &(InputOutputValues[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to convert from SQL_C_TIME to SQL_TYPE_TIME during insert statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 12, SQL_PARAM_INPUT, SQL_C_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0, &(InputOutputValues[num_rows_insert].TimestampValue), 0, &(InputOutputValues[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to convert from SQL_C_TIMESTAMP to SQL_TYPE_TIMESTAMP during insert statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 13, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_BIGINT, MAX_SQLSTRING_LEN, 0, InputOutputValues[num_rows_insert].BigintValue, 0, &(InputOutputValues[num_rows_insert].InValue));
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_BIGINT during insert statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
	// End of bind parameter

		returncode = SQLExecute(pTestInfo->hstmt);
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to execute the insert statement after bind parameter"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

		for(iparam = 1; iparam <= numparams; iparam++)
		{
			returncode = SQLDescribeParam(pTestInfo->hstmt,iparam,&paramSQLDataType,&paramColDef,&paramColScale,&paramColNull);
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to execute describe parameter after insert"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			if (iparam != 8)	// bug since SQL return DOUBLE for FLOAT also.
			{
				if (paramSQLDataType != ColumnInfo[iparam-1].DataType)
				{
					_stprintf(temp,_T("Parameter %d doesn't match expected: %d and actual: %d"),iparam,paramSQLDataType,ColumnInfo[iparam-1].DataType); 
					_tprintf(temp);
					ReleaseAll(pTestInfo);
					return(FALSE);
				}
			}
		} // end of for loop 
		num_rows_insert++;
	} // end of while loop

	return(TRUE);
}

BOOL StatementFunctions2()
{
	RETCODE		returncode;                        
	SWORD		numparams;
	_TCHAR		temp[250];
	int			iparam, num_rows_insert;
	SWORD		paramSQLDataType;
	SQLULEN		paramColDef;
	SWORD		paramColScale,paramColNull;

	num_rows_insert = 0;
	while (_tcscmp(InputOutputValues2[num_rows_insert].LongVarcharValue,END_LOOP) != 0)
	{
		if (InputOutputValues2[num_rows_insert].UseExecDirect)
		{
			rowdata = num_rows_insert;
			returncode = SQLExecDirect(pTestInfo->hstmt,(SQLTCHAR *)InsertTableWithValues2(),SQL_NTS);
			if (returncode != SQL_SUCCESS)
			{
				if (returncode == SQL_SUCCESS_WITH_INFO)
				{
					_tprintf(_T("This is not an error"));
					LogAllErrors(pTestInfo);
				}
				else
				{
					_tprintf(_T("Unable to ExecDirect insert Table"));
					LogAllErrors(pTestInfo);		
					ReleaseAll(pTestInfo);
					return(FALSE);
				}
			}
		}
		else
		if (InputOutputValues2[num_rows_insert].UseDefaults)
		{
			rowdata = num_rows_insert;
			returncode = SQLExecDirect(pTestInfo->hstmt,(SQLTCHAR *)InsertTableWithDefaults2(),SQL_NTS);
			if (returncode != SQL_SUCCESS)
			{
				if (returncode == SQL_SUCCESS_WITH_INFO)
				{
					_tprintf(_T("This is not an error"));
					LogAllErrors(pTestInfo);
				}
				else
				{
					_tprintf(_T("Unable to ExecDirect insert Table"));
					LogAllErrors(pTestInfo);		
					ReleaseAll(pTestInfo);
					return(FALSE);
				}
			}
		}
		else
		{
		//Indent here
			returncode = SQLPrepare(pTestInfo->hstmt,(SQLTCHAR *)InsertTable2(),SQL_NTS);
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to prepare insert Table"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}

			returncode = SQLNumParams(pTestInfo->hstmt,&numparams);
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to return number of parameters for insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			if (numparams != Actual_Num_Columns)
			{
				_stprintf(temp,_T("Number of parameters doesn't match expected: %d and actual: %d"),numparams,Actual_Num_Columns); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}

			// Begin of bind parameter
			returncode = SQLBindParameter(pTestInfo->hstmt, 1, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_LONGVARCHAR, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].LongVarcharValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_LONGVARCHAR during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 2, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_INTERVAL_YEAR, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalYearValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_INTERVAL_YEAR during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 3, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_INTERVAL_MONTH, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalMonthValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_INTERVAL_MONTH during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 4, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_INTERVAL_YEAR_TO_MONTH, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalYearToMonthValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_INTERVAL_YEAR_TO_MONTH during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 5, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_INTERVAL_DAY, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalDayValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_INTERVAL_DAY during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 6, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_INTERVAL_HOUR, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalHourValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_INTERVAL_HOUR during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 7, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_INTERVAL_MINUTE, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalMinuteValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_INTERVAL_MINUTE during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 8, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_INTERVAL_SECOND, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalSecondValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_INTERVAL_SECOND during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 9, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_INTERVAL_DAY_TO_HOUR, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalDayToHourValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_INTERVAL_DAY_TO_HOUR during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 10, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_INTERVAL_DAY_TO_MINUTE, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalDayToMinuteValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_INTERVAL_DAY_TO_MINUTE during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 11, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_INTERVAL_DAY_TO_SECOND, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalDayToSecondValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_INTERVAL_DAY_TO_SECOND during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 12, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_INTERVAL_HOUR_TO_MINUTE, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalHourToMinuteValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_INTERVAL_HOUR_TO_MINUTE during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 13, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_INTERVAL_HOUR_TO_SECOND, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalHourToSecondValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_INTERVAL_HOUR_TO_SECOND during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 14, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_INTERVAL_MINUTE_TO_SECOND, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalMinuteToSecondValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_INTERVAL_MINUTE_TO_SECOND during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 15, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_TYPE_DATE, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].DateValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_TYPE_DATE during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 16, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_TYPE_TIME, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].TimeValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_TYPE_TIME during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 17, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_TYPE_TIMESTAMP, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].TimestampValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_TYPE_TIMESTAMP during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 18, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_CHAR, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].CharValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_CHAR during insert statement - 2"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			// End of bind parameter

			returncode = SQLExecute(pTestInfo->hstmt);
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to execute the insert statement after bind parameter"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}

			for(iparam = 1; iparam <= numparams; iparam++)
			{
				returncode = SQLDescribeParam(pTestInfo->hstmt,iparam,&paramSQLDataType,&paramColDef,&paramColScale,&paramColNull);
				if (returncode != SQL_SUCCESS)
				{
					_tprintf(_T("Unable to execute describe parameter after insert"));
					LogAllErrors(pTestInfo);		
					ReleaseAll(pTestInfo);
					return(FALSE);
				}
			}  
		// indent upto here
		}	
		num_rows_insert++;
	} // end of while loop

	return(TRUE);
}

BOOL StatementFunctions3()
{
	RETCODE		returncode;                        
	SWORD		numparams;
	_TCHAR		temp[250];
	int			iparam, num_rows_insert;
	SWORD		paramSQLDataType;
	SQLULEN		paramColDef;
	SWORD		paramColScale,paramColNull;

	num_rows_insert = 0;
	while (_tcscmp(InputOutputValues3[num_rows_insert].UTF8CharValue,END_LOOP) != 0)
	{
		if (InputOutputValues3[num_rows_insert].UseExecDirect)
		{
			rowdata = num_rows_insert;
			returncode = SQLExecDirect(pTestInfo->hstmt,(SQLTCHAR *)InsertTableWithValues3(),SQL_NTS);
			if (returncode != SQL_SUCCESS)
			{
				if (returncode == SQL_SUCCESS_WITH_INFO)
				{
					_tprintf(_T("This is not an error"));
					LogAllErrors(pTestInfo);
				}
				else
				{
					_tprintf(_T("Unable to ExecDirect insert Table"));
					LogAllErrors(pTestInfo);		
					ReleaseAll(pTestInfo);
					return(FALSE);
				}
			}
		}
		else
		{
			returncode = SQLPrepare(pTestInfo->hstmt,(SQLTCHAR *)InsertTable3(),SQL_NTS);
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to prepare insert Table"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}

			returncode = SQLNumParams(pTestInfo->hstmt,&numparams);
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to return number of parameters for insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			if (numparams != Actual_Num_Columns)
			{
				_stprintf(temp,_T("Number of parameters doesn't match expected: %d and actual: %d"),numparams,Actual_Num_Columns); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}

			// Begin of bind parameter
			returncode = SQLBindParameter(pTestInfo->hstmt, 1, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_CHAR, MAX_SQLSTRING_LEN, 0, InputOutputValues3[num_rows_insert].UTF8CharValue, 0, &(InputOutputValues3[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_CHAR(UTF8) during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 2, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_VARCHAR, MAX_SQLSTRING_LEN, 0, InputOutputValues3[num_rows_insert].UTF8VarcharValue, 0, &(InputOutputValues3[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_VARCHAR(UTF8) during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 3, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_WCHAR, MAX_SQLSTRING_LEN, 0, InputOutputValues3[num_rows_insert].WCharValue1, 0, &(InputOutputValues3[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_WCHAR(1) during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 4, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_WCHAR, MAX_SQLSTRING_LEN, 0, InputOutputValues3[num_rows_insert].WCharValue2, 0, &(InputOutputValues3[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_WCHAR(2) during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 5, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_WVARCHAR, MAX_SQLSTRING_LEN, 0, InputOutputValues3[num_rows_insert].WVarcharValue, 0, &(InputOutputValues3[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_WVARCHAR during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 6, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_BIT, 0, 0, &(InputOutputValues3[num_rows_insert].BitValue), 0, &(InputOutputValues3[num_rows_insert].InValue1));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_SHORT to SQL_BIT during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 7, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_TINYINT, 0, 0, &(InputOutputValues3[num_rows_insert].TinyintValue), 0, &(InputOutputValues3[num_rows_insert].InValue1));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_SHORT to SQL_TINYINT during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 8, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_BINARY, MAX_SQLSTRING_LEN, 0, InputOutputValues3[num_rows_insert].BinaryValue, 0, &(InputOutputValues3[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_BINARY during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 9, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_VARBINARY, MAX_SQLSTRING_LEN, 0, InputOutputValues3[num_rows_insert].VarBinaryValue, 0, &(InputOutputValues3[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to convert from SQL_C_TCHAR to SQL_VARBINARY during insert statement"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			// End of bind parameter

			returncode = SQLExecute(pTestInfo->hstmt);
			if (returncode != SQL_SUCCESS)
			{
				_tprintf(_T("Unable to execute the insert statement after bind parameter"));
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}

			for(iparam = 1; iparam <= numparams; iparam++)
			{
				returncode = SQLDescribeParam(pTestInfo->hstmt,iparam,&paramSQLDataType,&paramColDef,&paramColScale,&paramColNull);
				if (returncode != SQL_SUCCESS)
				{
					_tprintf(_T("Unable to execute describe parameter after insert"));
					LogAllErrors(pTestInfo);		
					ReleaseAll(pTestInfo);
					return(FALSE);
				}
			}  
		}
		num_rows_insert++;
	} // end of while loop

	return(TRUE);
}

//#####################################################################################################
// This functions cover the following APIs:
// SQLNumResultCols
// SQLDescribeParam
// SQLExecute
// SQLBindParam
// SQLNumParams
// SQLSet/GetCursorName
//#####################################################################################################
BOOL ResultFunctions()
{
	RETCODE						returncode;                        
	SWORD						numcols;
	int							icol;
	int							irow;
	_TCHAR						columnName[SQL_MAX_BUFFER_LEN];
	SWORD						columnLength, columnSQLDataType;
	SQLULEN						columnColDef;
	SWORD						columnColScale,columnNull;
	_TCHAR						temp[250];
	PTR							columnAttribute;
	SWORD						pcDesc;
	SQLLEN						pfDesc;
	_TCHAR						*CharOutput;
	_TCHAR						*VarcharOutput;
	_TCHAR						*DecimalOutput;
	_TCHAR						*NumericOutput;
	SWORD						ShortOutput;
	SDWORD						LongOutput;
	SFLOAT						RealOutput;
	SDOUBLE						FloatOutput;
	SDOUBLE						DoubleOutput;
	DATE_STRUCT					DateOutput;
	TIME_STRUCT					TimeOutput;
	TIMESTAMP_STRUCT			TimestampOutput;
	_TCHAR						*BigintOutput;
	SQLLEN						CharOutputLen;
	SQLLEN						VarcharOutputLen;
	SQLLEN						DecimalOutputLen;
	SQLLEN						NumericOutputLen;
	SQLLEN						ShortOutputLen;
	SQLLEN						LongOutputLen;
	SQLLEN						RealOutputLen;
	SQLLEN						FloatOutputLen;
	SQLLEN						DoubleOutputLen;
	SQLLEN						DateOutputLen;
	SQLLEN						TimeOutputLen;
	SQLLEN						TimestampOutputLen;
	SQLLEN						BigintOutputLen;

	CharOutput = (_TCHAR *)malloc(sizeof(_TCHAR)*MAX_COLUMN_OUTPUT);
	VarcharOutput = (_TCHAR *)malloc(sizeof(_TCHAR)*MAX_COLUMN_OUTPUT);
	DecimalOutput = (_TCHAR *)malloc(sizeof(_TCHAR)*MAX_COLUMN_OUTPUT);
	NumericOutput = (_TCHAR *)malloc(sizeof(_TCHAR)*MAX_COLUMN_OUTPUT);
	BigintOutput = (_TCHAR *)malloc(sizeof(_TCHAR)*MAX_COLUMN_OUTPUT);

	returncode = SQLExecDirect(pTestInfo->hstmt,(SQLTCHAR *)SelectTable(),SQL_NTS);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to Drop Table"), "ExecDirect");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLNumResultCols(pTestInfo->hstmt,&numcols);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to return number of parameters for insert statement"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (numcols != Actual_Num_Columns)
	{
		_stprintf(temp,_T("Number of columns doesn't match expected: %d and actual: %d"),numcols,Actual_Num_Columns); 
		_tprintf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	for (icol = 0; icol < numcols; icol++)
	{
		returncode = SQLDescribeCol(pTestInfo->hstmt,icol+1,(SQLTCHAR*)columnName,SQL_MAX_BUFFER_LEN,&columnLength,&columnSQLDataType,&columnColDef,&columnColScale,&columnNull);
		if (returncode != SQL_SUCCESS)
		{
			_stprintf(temp,_T("Unable to describe column %d after select statement"),icol+1); 
			_tprintf(temp);
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		if ((icol+1) != 8 && (icol+1) != 13) // bug since SQL return DOUBLE for FLOAT and NUMERIC for LARGE_INT also. May be we treat this as feature.
		{
			if ((_tcsnicmp((_TCHAR *)columnName,ColumnInfo[icol].Name,columnLength) != 0) && (columnSQLDataType != ColumnInfo[icol].DataType))
			{
				_stprintf(temp,_T("Column %d doesn't match column name expected: %s and actual: %s and datatype expected: %d and actual: %d"),icol+1,ColumnInfo[icol].Name,columnName,ColumnInfo[icol].DataType,columnSQLDataType); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
		}
		columnAttribute = (_TCHAR *)malloc(sizeof(_TCHAR)*SQL_MAX_BUFFER_LEN);
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_NAME,columnAttribute,SQL_MAX_BUFFER_LEN*sizeof(_TCHAR),&pcDesc,&pfDesc);
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to get column attribute name after select statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		if (_tcsnicmp(ColumnInfo[icol].Name,(_TCHAR *)columnAttribute,pcDesc/sizeof(_TCHAR)) != 0)
		{
			_stprintf(temp,_T("Column %d doesn't match column name expected: %s and actual: %s"),icol+1,ColumnInfo[icol].Name,columnAttribute); 
			_tprintf(temp);
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_TYPE,columnAttribute,0,&pcDesc,&pfDesc);
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to get column attribute type after select statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		if ((icol+1) != 8 && (icol+1) != 13) // bug since SQL return DOUBLE for FLOAT also NUMERIC for LARGE_INT. May be we treat this as feature.
		{
			if (ColumnInfo[icol].DataType != pfDesc)
			{
				_stprintf(temp,_T("Column %d doesn't match column type expected: %d and actual: %d"),icol+1,ColumnInfo[icol].DataType,pfDesc); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
		}
		free(columnAttribute); 
	
		switch (ColumnInfo[icol].DataType)
		{
			case SQL_CHAR:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,CharOutput,sizeof(_TCHAR)*MAX_COLUMN_OUTPUT,&CharOutputLen);
				break;
			case SQL_VARCHAR:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,VarcharOutput,sizeof(_TCHAR)*MAX_COLUMN_OUTPUT,&VarcharOutputLen);
				break;
			case SQL_DECIMAL:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,DecimalOutput,sizeof(_TCHAR)*MAX_COLUMN_OUTPUT,&DecimalOutputLen);
				break;
			case SQL_NUMERIC:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,NumericOutput,sizeof(_TCHAR)*MAX_COLUMN_OUTPUT,&NumericOutputLen);
				break;
			case SQL_SMALLINT:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,&ShortOutput,0,&ShortOutputLen);
				break;
			case SQL_INTEGER:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,&LongOutput,0,&LongOutputLen);
				break;
			case SQL_REAL:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,&RealOutput,0,&RealOutputLen);
				break;
			case SQL_FLOAT:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,&FloatOutput,0,&FloatOutputLen);
				break;
			case SQL_DOUBLE:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,&DoubleOutput,0,&DoubleOutputLen);
				break;
			case SQL_TYPE_DATE:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,&DateOutput,0,&DateOutputLen);
				break;
			case SQL_TYPE_TIME:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,&TimeOutput,0,&TimeOutputLen);
				break;
			case SQL_TYPE_TIMESTAMP:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,&TimestampOutput,0,&TimestampOutputLen);
				break;
			case SQL_BIGINT:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,BigintOutput,sizeof(_TCHAR)*MAX_COLUMN_OUTPUT,&BigintOutputLen);
				break;
		}
		if (returncode != SQL_SUCCESS)
		{
			_stprintf(temp,_T("Unable to bind column %d after select statement"),icol); 
			_tprintf(temp);
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
	
	}

	irow = 0;
	while (returncode == SQL_SUCCESS)
	{
		returncode = SQLFetch(pTestInfo->hstmt);
		if((returncode != SQL_NO_DATA_FOUND)&&(returncode != SQL_SUCCESS))
		{
			_tprintf(_T("Unable to fetch after bind column"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		else if (returncode == SQL_SUCCESS)
		{
			icol = 1;
			if (_tcsnicmp(InputOutputValues[irow].CharValue,CharOutput,_tcslen(InputOutputValues[irow].CharValue)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,InputOutputValues[irow].CharValue,CharOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_tcsnicmp(InputOutputValues[irow].VarcharValue,VarcharOutput,_tcslen(InputOutputValues[irow].VarcharValue)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,InputOutputValues[irow].VarcharValue,VarcharOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_tcsnicmp(InputOutputValues[irow].DecimalValue,DecimalOutput,_tcslen(InputOutputValues[irow].DecimalValue)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,InputOutputValues[irow].DecimalValue,DecimalOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_tcsnicmp(InputOutputValues[irow].NumericValue,NumericOutput,_tcslen(InputOutputValues[irow].NumericValue)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,InputOutputValues[irow].NumericValue,NumericOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (InputOutputValues[irow].ShortValue != ShortOutput)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %d and actual: %d"),icol,InputOutputValues[irow].ShortValue,ShortOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (InputOutputValues[irow].LongValue != LongOutput)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %d and actual: %d"),icol,InputOutputValues[irow].LongValue,LongOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (InputOutputValues[irow].RealValue != RealOutput)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %f and actual: %f"),icol,InputOutputValues[irow].RealValue,RealOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (InputOutputValues[irow].FloatValue != FloatOutput)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %e and actual: %e"),icol,InputOutputValues[irow].FloatValue,FloatOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (InputOutputValues[irow].DoubleValue != DoubleOutput)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %e and actual: %e"),icol,InputOutputValues[irow].DoubleValue,DoubleOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if ((InputOutputValues[irow].DateValue.month != DateOutput.month) && (InputOutputValues[irow].DateValue.day != DateOutput.day) && (InputOutputValues[irow].DateValue.year != DateOutput.year))
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %d and actual: %d"),icol,InputOutputValues[irow].DateValue,DateOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if ((InputOutputValues[irow].TimeValue.hour != TimeOutput.hour) && (InputOutputValues[irow].TimeValue.minute != TimeOutput.minute) && (InputOutputValues[irow].TimeValue.second != TimeOutput.second))
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %d and actual: %d"),icol,InputOutputValues[irow].TimeValue,TimeOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if ((InputOutputValues[irow].TimestampValue.month != TimestampOutput.month) && (InputOutputValues[irow].TimestampValue.day != TimestampOutput.day) && (InputOutputValues[irow].TimestampValue.year != TimestampOutput.year) && (InputOutputValues[irow].TimestampValue.hour != TimestampOutput.hour) && (InputOutputValues[irow].TimestampValue.minute != TimestampOutput.minute) && (InputOutputValues[irow].TimestampValue.second != TimestampOutput.second) && (InputOutputValues[irow].TimestampValue.fraction != TimestampOutput.fraction))
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %d and actual: %d"),icol,InputOutputValues[irow].TimestampValue,TimestampOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_tcsnicmp(InputOutputValues[irow].BigintValue,BigintOutput,_tcslen(InputOutputValues[irow].BigintValue)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %d and actual: %d"),icol,InputOutputValues[irow].BigintValue,BigintOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			irow++;
		}
	}

	returncode = SQLFreeStmt(pTestInfo->hstmt,SQL_CLOSE);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to freestmt with CLOSE option"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	free(CharOutput);
	free(VarcharOutput);
	free(DecimalOutput);
	free(NumericOutput);
	free(BigintOutput);

	return(TRUE);
}

void ConvertIntervalStructToString(SQL_INTERVAL_STRUCT InStruct, SWORD SQLType, _TCHAR *OutString)
{
	if (InStruct.interval_sign)
		{	
			switch (SQLType)
			{
			case SQL_INTERVAL_YEAR:
				_stprintf(OutString, _T("-%d"), InStruct.intval.year_month.year);
				break;
			case SQL_INTERVAL_MONTH:
				_stprintf(OutString, _T("-%d"), InStruct.intval.year_month.month);
				break;
			case SQL_INTERVAL_YEAR_TO_MONTH:
				_stprintf(OutString, _T("-%d-%d"), InStruct.intval.year_month.year, InStruct.intval.year_month.month);
				break;
			case SQL_INTERVAL_DAY:
				_stprintf(OutString, _T("-%d"), InStruct.intval.day_second.day);
				break;
			case SQL_INTERVAL_HOUR:
				_stprintf(OutString, _T("-%d"), InStruct.intval.day_second.hour);
				break;
			case SQL_INTERVAL_MINUTE:
				_stprintf(OutString, _T("-%d"), InStruct.intval.day_second.minute);
				break;
			case SQL_INTERVAL_SECOND:
				if (InStruct.intval.day_second.fraction == 0)
					_stprintf(OutString, _T("-%d"), InStruct.intval.day_second.second);
				else
					_stprintf(OutString, _T("-%d.%d"), InStruct.intval.day_second.second, InStruct.intval.day_second.fraction);
				break;
			case SQL_INTERVAL_DAY_TO_HOUR:
				_stprintf(OutString, _T("-%d %d"), InStruct.intval.day_second.day, InStruct.intval.day_second.hour);
				break;
			case SQL_INTERVAL_DAY_TO_MINUTE:
				_stprintf(OutString, _T("-%d %d:%d"), InStruct.intval.day_second.day, InStruct.intval.day_second.hour, InStruct.intval.day_second.minute);
				break;
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (InStruct.intval.day_second.fraction == 0)
					_stprintf(OutString, _T("-%d %d:%d:%d"), InStruct.intval.day_second.day, InStruct.intval.day_second.hour, InStruct.intval.day_second.minute, InStruct.intval.day_second.second);
				else
					_stprintf(OutString, _T("-%d %d:%d:%d.%d"), InStruct.intval.day_second.day, InStruct.intval.day_second.hour, InStruct.intval.day_second.minute, InStruct.intval.day_second.second, InStruct.intval.day_second.fraction);
				break;
			case SQL_INTERVAL_HOUR_TO_MINUTE:
				_stprintf(OutString, _T("-%d:%d"), InStruct.intval.day_second.hour, InStruct.intval.day_second.minute);
				break;
			case SQL_INTERVAL_HOUR_TO_SECOND:
				if (InStruct.intval.day_second.fraction == 0)
					_stprintf(OutString, _T("-%d:%d:%d"), InStruct.intval.day_second.hour, InStruct.intval.day_second.minute, InStruct.intval.day_second.second);
				else
					_stprintf(OutString, _T("-%d:%d:%d.%d"), InStruct.intval.day_second.hour, InStruct.intval.day_second.minute, InStruct.intval.day_second.second, InStruct.intval.day_second.fraction);
				break;
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				if (InStruct.intval.day_second.fraction == 0)
					_stprintf(OutString, _T("-%d:%d"), InStruct.intval.day_second.minute, InStruct.intval.day_second.second);
				else
					_stprintf(OutString, _T("-%d:%d.%d"), InStruct.intval.day_second.minute, InStruct.intval.day_second.second, InStruct.intval.day_second.fraction);
				break;
			}
		}
	else
		{	
			switch (SQLType)
			{
			case SQL_INTERVAL_YEAR:
				_stprintf(OutString, _T("%d"), InStruct.intval.year_month.year);
				break;
			case SQL_INTERVAL_MONTH:
				_stprintf(OutString, _T("%d"), InStruct.intval.year_month.month);
				break;
			case SQL_INTERVAL_YEAR_TO_MONTH:
				_stprintf(OutString, _T("%d-%d"), InStruct.intval.year_month.year, InStruct.intval.year_month.month);
				break;
			case SQL_INTERVAL_DAY:
				_stprintf(OutString, _T("%d"), InStruct.intval.day_second.day);
				break;
			case SQL_INTERVAL_HOUR:
				_stprintf(OutString, _T("%d"), InStruct.intval.day_second.hour);
				break;
			case SQL_INTERVAL_MINUTE:
				_stprintf(OutString, _T("%d"), InStruct.intval.day_second.minute);
				break;
			case SQL_INTERVAL_SECOND:
				if (InStruct.intval.day_second.fraction == 0)
					_stprintf(OutString, _T("%d"), InStruct.intval.day_second.second);
				else
					_stprintf(OutString, _T("%d.%d"), InStruct.intval.day_second.second, InStruct.intval.day_second.fraction);
				break;
			case SQL_INTERVAL_DAY_TO_HOUR:
				_stprintf(OutString, _T("%d %d"), InStruct.intval.day_second.day, InStruct.intval.day_second.hour);
				break;
			case SQL_INTERVAL_DAY_TO_MINUTE:
				_stprintf(OutString, _T("%d %d:%d"), InStruct.intval.day_second.day, InStruct.intval.day_second.hour, InStruct.intval.day_second.minute);
				break;
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (InStruct.intval.day_second.fraction == 0)
					_stprintf(OutString, _T("%d %d:%d:%d"), InStruct.intval.day_second.day, InStruct.intval.day_second.hour, InStruct.intval.day_second.minute, InStruct.intval.day_second.second);
				else
					_stprintf(OutString, _T("%d %d:%d:%d.%d"), InStruct.intval.day_second.day, InStruct.intval.day_second.hour, InStruct.intval.day_second.minute, InStruct.intval.day_second.second, InStruct.intval.day_second.fraction);
				break;
			case SQL_INTERVAL_HOUR_TO_MINUTE:
				_stprintf(OutString, _T("%d:%d"), InStruct.intval.day_second.hour, InStruct.intval.day_second.minute);
				break;
			case SQL_INTERVAL_HOUR_TO_SECOND:
				if (InStruct.intval.day_second.fraction == 0)
					_stprintf(OutString, _T("%d:%d:%d"), InStruct.intval.day_second.hour, InStruct.intval.day_second.minute, InStruct.intval.day_second.second);
				else
					_stprintf(OutString, _T("%d:%d:%d.%d"), InStruct.intval.day_second.hour, InStruct.intval.day_second.minute, InStruct.intval.day_second.second, InStruct.intval.day_second.fraction);
				break;
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				if (InStruct.intval.day_second.fraction == 0)
					_stprintf(OutString, _T("%d:%d"), InStruct.intval.day_second.minute, InStruct.intval.day_second.second);
				else
					_stprintf(OutString, _T("%d:%d.%d"), InStruct.intval.day_second.minute, InStruct.intval.day_second.second, InStruct.intval.day_second.fraction);
				break;
			}
		}
	return;
}

BOOL ResultFunctions2()
{
	RETCODE						returncode;                        
	SWORD						numcols;
	int							icol;
	int							irow;
	_TCHAR						columnName[SQL_MAX_BUFFER_LEN];
	SWORD						columnLength, columnSQLDataType;
	SQLULEN						columnColDef;
	SWORD						columnColScale,columnNull;
	_TCHAR						temp[250];
	PTR							columnAttribute;
	SWORD						pcDesc;
	SQLLEN						pfDesc;
	_TCHAR						*LongVarcharOutput;
	_TCHAR						*CharOutput;
	SQL_INTERVAL_STRUCT			IntervalOutputYear;
	SQL_INTERVAL_STRUCT			IntervalOutputMonth;
	SQL_INTERVAL_STRUCT			IntervalOutputYearToMonth;
	SQL_INTERVAL_STRUCT			IntervalOutputDay;
	SQL_INTERVAL_STRUCT			IntervalOutputHour;
	SQL_INTERVAL_STRUCT			IntervalOutputMinute;
	SQL_INTERVAL_STRUCT			IntervalOutputSecond;
	SQL_INTERVAL_STRUCT			IntervalOutputDayToHour;
	SQL_INTERVAL_STRUCT			IntervalOutputDayToMinute;
	SQL_INTERVAL_STRUCT			IntervalOutputDayToSecond;
	SQL_INTERVAL_STRUCT			IntervalOutputHourToMinute;
	SQL_INTERVAL_STRUCT			IntervalOutputHourToSecond;
	SQL_INTERVAL_STRUCT			IntervalOutputMinuteToSecond;
	DATE_STRUCT					DateOutput;
	TIME_STRUCT					TimeOutput;
	TIMESTAMP_STRUCT			TimestampOutput;

	_TCHAR						*IntervalOutputString;

	SQLLEN						LongVarcharOutputLen;
	SQLLEN						IntervalOutputLen;
	SQLLEN						DateOutputLen;
	SQLLEN						TimeOutputLen;
	SQLLEN						TimestampOutputLen;
	SQLLEN						CharOutputLen;

	CharOutput = (_TCHAR *)malloc(sizeof(_TCHAR)*MAX_COLUMN_OUTPUT);
	LongVarcharOutput =		(_TCHAR *)malloc(sizeof(_TCHAR)*MAX_COLUMN_OUTPUT);
	IntervalOutputString =	(_TCHAR *)malloc(sizeof(_TCHAR)*100);

	returncode = SQLExecDirect(pTestInfo->hstmt,(SQLTCHAR*)SelectTable2(),SQL_NTS);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to Drop Table"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLNumResultCols(pTestInfo->hstmt,&numcols);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to return number of parameters for insert statement"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (numcols != Actual_Num_Columns)
	{
		_stprintf(temp,_T("Number of columns doesn't match expected: %d and actual: %d"),numcols,Actual_Num_Columns); 
		_tprintf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	for (icol = 0; icol < numcols; icol++)
	{
		returncode = SQLDescribeCol(pTestInfo->hstmt,icol+1,(SQLTCHAR*)columnName,SQL_MAX_BUFFER_LEN,&columnLength,&columnSQLDataType,&columnColDef,&columnColScale,&columnNull);
		if (returncode != SQL_SUCCESS)
		{
			_stprintf(temp,_T("Unable to describe column %d after select statement"),icol+1); 
			_tprintf(temp);
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

		columnAttribute = (_TCHAR *)malloc(sizeof(_TCHAR)*SQL_MAX_BUFFER_LEN);
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_NAME,columnAttribute,SQL_MAX_BUFFER_LEN*sizeof(_TCHAR),&pcDesc,&pfDesc);
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to get column attribute name after select statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		if (_tcsnicmp(ColumnInfo2[icol].Name,(_TCHAR *)columnAttribute,pcDesc/sizeof(_TCHAR)) != 0)
		{
			_stprintf(temp,_T("Column %d doesn't match column name expected: %s and actual: %s"),icol+1,ColumnInfo2[icol].Name,columnAttribute); 
			_tprintf(temp);
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_TYPE,columnAttribute,0,&pcDesc,&pfDesc);
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to get column attribute type after select statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

		free(columnAttribute); 
	
		switch (ColumnInfo2[icol].DataType)
		{
			case SQL_LONGVARCHAR:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,LongVarcharOutput,sizeof(_TCHAR)*MAX_COLUMN_OUTPUT,&LongVarcharOutputLen);
				break;
			case SQL_INTERVAL_YEAR:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,&IntervalOutputYear,MAX_COLUMN_OUTPUT,&IntervalOutputLen);
				break;
			case SQL_INTERVAL_MONTH:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,&IntervalOutputMonth,MAX_COLUMN_OUTPUT,&IntervalOutputLen);
				break;
			case SQL_INTERVAL_YEAR_TO_MONTH:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,&IntervalOutputYearToMonth,MAX_COLUMN_OUTPUT,&IntervalOutputLen);
				break;
			case SQL_INTERVAL_DAY:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,&IntervalOutputDay,MAX_COLUMN_OUTPUT,&IntervalOutputLen);
				break;
			case SQL_INTERVAL_HOUR:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,&IntervalOutputHour,MAX_COLUMN_OUTPUT,&IntervalOutputLen);
				break;
			case SQL_INTERVAL_MINUTE:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,&IntervalOutputMinute,MAX_COLUMN_OUTPUT,&IntervalOutputLen);
				break;
			case SQL_INTERVAL_SECOND:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,&IntervalOutputSecond,MAX_COLUMN_OUTPUT,&IntervalOutputLen);
				break;
			case SQL_INTERVAL_DAY_TO_HOUR:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,&IntervalOutputDayToHour,MAX_COLUMN_OUTPUT,&IntervalOutputLen);
				break;
			case SQL_INTERVAL_DAY_TO_MINUTE:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,&IntervalOutputDayToMinute,MAX_COLUMN_OUTPUT,&IntervalOutputLen);
				break;
			case SQL_INTERVAL_DAY_TO_SECOND:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,&IntervalOutputDayToSecond,MAX_COLUMN_OUTPUT,&IntervalOutputLen);
				break;
			case SQL_INTERVAL_HOUR_TO_MINUTE:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,&IntervalOutputHourToMinute,MAX_COLUMN_OUTPUT,&IntervalOutputLen);
				break;
			case SQL_INTERVAL_HOUR_TO_SECOND:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,&IntervalOutputHourToSecond,MAX_COLUMN_OUTPUT,&IntervalOutputLen);
				break;
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,&IntervalOutputMinuteToSecond,MAX_COLUMN_OUTPUT,&IntervalOutputLen);
				break;
			case SQL_TYPE_DATE:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,&DateOutput,MAX_COLUMN_OUTPUT,&DateOutputLen);
				break;
			case SQL_TYPE_TIME:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,&TimeOutput,MAX_COLUMN_OUTPUT,&TimeOutputLen);
				break;
			case SQL_TYPE_TIMESTAMP:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,&TimestampOutput,MAX_COLUMN_OUTPUT,&TimestampOutputLen);
				break;
			case SQL_CHAR:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,CharOutput,sizeof(_TCHAR)*MAX_COLUMN_OUTPUT,&CharOutputLen);
				break;
		}
		if (returncode != SQL_SUCCESS)
		{
			_stprintf(temp,_T("Unable to bind column %d after select statement"),icol); 
			_tprintf(temp);
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
	}

	irow = 0;
	while (returncode == SQL_SUCCESS)
	{
		returncode = SQLFetch(pTestInfo->hstmt);
		if((returncode != SQL_NO_DATA_FOUND)&&(returncode != SQL_SUCCESS))
		{
			_tprintf(_T("Unable to fetch after bind column"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		else if (returncode == SQL_SUCCESS)
		{
			icol = 1;
			if (_tcsnicmp(InputOutputValues2[irow].LongVarcharValue,LongVarcharOutput,_tcslen(InputOutputValues2[irow].LongVarcharValue)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,InputOutputValues2[irow].LongVarcharValue,LongVarcharOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputYear, SQL_INTERVAL_YEAR, IntervalOutputString);
			if (_tcsnicmp(IntervalValues[irow].Year,IntervalOutputString,_tcslen(IntervalValues[irow].Year)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,IntervalValues[irow].Year,IntervalOutputString); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputMonth, SQL_INTERVAL_MONTH, IntervalOutputString);
			if (_tcsnicmp(IntervalValues[irow].Month,IntervalOutputString,_tcslen(IntervalValues[irow].Month)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,IntervalValues[irow].Month,IntervalOutputString); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputYearToMonth, SQL_INTERVAL_YEAR_TO_MONTH, IntervalOutputString);
			if (_tcsnicmp(IntervalValues[irow].YearToMonth,IntervalOutputString,_tcslen(IntervalValues[irow].YearToMonth)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,IntervalValues[irow].YearToMonth,IntervalOutputString); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputDay, SQL_INTERVAL_DAY, IntervalOutputString);
			if (_tcsnicmp(IntervalValues[irow].Day,IntervalOutputString,_tcslen(IntervalValues[irow].Day)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,IntervalValues[irow].Day,IntervalOutputString); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputHour, SQL_INTERVAL_HOUR, IntervalOutputString);
			if (_tcsnicmp(IntervalValues[irow].Hour,IntervalOutputString,_tcslen(IntervalValues[irow].Hour)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,IntervalValues[irow].Hour,IntervalOutputString); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputMinute, SQL_INTERVAL_MINUTE, IntervalOutputString);
			if (_tcsnicmp(IntervalValues[irow].Minute,IntervalOutputString,_tcslen(IntervalValues[irow].Minute)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,IntervalValues[irow].Minute,IntervalOutputString); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputSecond, SQL_INTERVAL_SECOND, IntervalOutputString);
			if (_tcsnicmp(IntervalValues[irow].Second,IntervalOutputString,_tcslen(IntervalValues[irow].Second)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,IntervalValues[irow].Second,IntervalOutputString); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputDayToHour, SQL_INTERVAL_DAY_TO_HOUR, IntervalOutputString);
			if (_tcsnicmp(IntervalValues[irow].DayToHour,IntervalOutputString,_tcslen(IntervalValues[irow].DayToHour)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,IntervalValues[irow].DayToHour,IntervalOutputString); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputDayToMinute, SQL_INTERVAL_DAY_TO_MINUTE, IntervalOutputString);
			if (_tcsnicmp(IntervalValues[irow].DayToMinute,IntervalOutputString,_tcslen(IntervalValues[irow].DayToMinute)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,IntervalValues[irow].DayToMinute,IntervalOutputString); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputDayToSecond, SQL_INTERVAL_DAY_TO_SECOND, IntervalOutputString);
			if (_tcsnicmp(IntervalValues[irow].DayToSecond,IntervalOutputString,_tcslen(IntervalValues[irow].DayToSecond)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,IntervalValues[irow].DayToSecond,IntervalOutputString); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputHourToMinute, SQL_INTERVAL_HOUR_TO_MINUTE, IntervalOutputString);
			if (_tcsnicmp(IntervalValues[irow].HourToMinute,IntervalOutputString,_tcslen(IntervalValues[irow].HourToMinute)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,IntervalValues[irow].HourToMinute,IntervalOutputString); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputHourToSecond, SQL_INTERVAL_HOUR_TO_SECOND, IntervalOutputString);
			if (_tcsnicmp(IntervalValues[irow].HourToSecond,IntervalOutputString,_tcslen(IntervalValues[irow].HourToSecond)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,IntervalValues[irow].HourToSecond,IntervalOutputString); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputMinuteToSecond, SQL_INTERVAL_MINUTE_TO_SECOND, IntervalOutputString);
			if (_tcsnicmp(IntervalValues[irow].MinuteToSecond,IntervalOutputString,_tcslen(IntervalValues[irow].MinuteToSecond)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,IntervalValues[irow].MinuteToSecond,IntervalOutputString); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if ((IntervalValues[irow].DateValue.month != DateOutput.month) && (IntervalValues[irow].DateValue.day != DateOutput.day) && (IntervalValues[irow].DateValue.year != DateOutput.year))
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %d and actual: %d"),icol,IntervalValues[irow].DateValue,DateOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if ((IntervalValues[irow].TimeValue.hour != TimeOutput.hour) && (IntervalValues[irow].TimeValue.minute != TimeOutput.minute) && (IntervalValues[irow].TimeValue.second != TimeOutput.second))
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %d:%d:%d and actual: %d:%d:%d"),icol,IntervalValues[irow].TimeValue.hour,IntervalValues[irow].TimeValue.minute,IntervalValues[irow].TimeValue.second,TimeOutput.hour,TimeOutput.minute,TimeOutput.second); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if ((IntervalValues[irow].TimestampValue.month != TimestampOutput.month) && (IntervalValues[irow].TimestampValue.day != TimestampOutput.day) && (IntervalValues[irow].TimestampValue.year != TimestampOutput.year) && (IntervalValues[irow].TimestampValue.hour != TimestampOutput.hour) && (IntervalValues[irow].TimestampValue.minute != TimestampOutput.minute) && (IntervalValues[irow].TimestampValue.second != TimestampOutput.second) && (IntervalValues[irow].TimestampValue.fraction != TimestampOutput.fraction))
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %d and actual: %d"),icol,IntervalValues[irow].TimestampValue,TimestampOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_tcsnicmp(InputOutputValues2[irow].CharValue,CharOutput,_tcslen(InputOutputValues2[irow].CharValue)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,InputOutputValues2[irow].CharValue,CharOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			irow++;
		}
	}

	returncode = SQLFreeStmt(pTestInfo->hstmt,SQL_CLOSE);
//	returncode = SQLFreeHandle (SQL_HANDLE_STMT, pTestInfo->hstmt);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to freestmt with CLOSE option"));
//		_tprintf(_T("Unable to freehandle with CLOSE option"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	free(LongVarcharOutput);
	free(IntervalOutputString);

	return(TRUE);
}

//#####################################################################################################
// This functions cover the following APIs:
// SQLNumResultCols
// SQLDescribeParam
// SQLExecute
// SQLBindParam
// SQLNumParams
// SQLSet/GetCursorName
//#####################################################################################################
BOOL ResultFunctions3()
{
	RETCODE						returncode;                        
	SWORD						numcols;
	int							icol;
	int							irow;
	_TCHAR						columnName[SQL_MAX_BUFFER_LEN];
	SWORD						columnLength, columnSQLDataType;
	SQLULEN						columnColDef;
	SWORD						columnColScale,columnNull;
	_TCHAR						temp[250];
	PTR							columnAttribute;
	SWORD						pcDesc;
	SQLLEN						pfDesc;
	_TCHAR						*UTF8CharOutput;
	_TCHAR						*UTF8VarcharOutput;
	_TCHAR						*WCharOutput1;
	_TCHAR						*WCharOutput2;
	_TCHAR						*WVarcharOutput;
	SWORD						BitOutput;
	SWORD						TinyintOutput;
	_TCHAR						*BinaryOutput;
	_TCHAR						*VarBinaryOutput;
	SQLLEN						UTF8CharOutputLen;
	SQLLEN						UTF8VarcharOutputLen;
	SQLLEN						WCharOutputLen1;
	SQLLEN						WCharOutputLen2;
	SQLLEN						WVarcharOutputLen;
	SQLLEN						BitOutputLen;
	SQLLEN						TinyintOutputLen;
	SQLLEN						BinaryOutputLen;
	SQLLEN						VarBinaryOutputLen;

	UTF8CharOutput = (_TCHAR *)malloc(sizeof(_TCHAR)*MAX_COLUMN_OUTPUT);
	UTF8VarcharOutput = (_TCHAR *)malloc(sizeof(_TCHAR)*MAX_COLUMN_OUTPUT);
	WCharOutput1 = (_TCHAR *)malloc(sizeof(_TCHAR)*MAX_COLUMN_OUTPUT);
	WCharOutput2 = (_TCHAR *)malloc(sizeof(_TCHAR)*MAX_COLUMN_OUTPUT);
	WVarcharOutput = (_TCHAR *)malloc(sizeof(_TCHAR)*MAX_COLUMN_OUTPUT);
	BinaryOutput = (_TCHAR *)malloc(sizeof(_TCHAR)*MAX_COLUMN_OUTPUT);
	VarBinaryOutput = (_TCHAR *)malloc(sizeof(_TCHAR)*MAX_COLUMN_OUTPUT);

	returncode = SQLExecDirect(pTestInfo->hstmt,(SQLTCHAR *)SelectTable3(),SQL_NTS);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to Drop Table"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLNumResultCols(pTestInfo->hstmt,&numcols);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to return number of parameters for insert statement"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (numcols != Actual_Num_Columns)
	{
		_stprintf(temp,_T("Number of columns doesn't match expected: %d and actual: %d"),numcols,Actual_Num_Columns); 
		_tprintf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	for (icol = 0; icol < numcols; icol++)
	{
		returncode = SQLDescribeCol(pTestInfo->hstmt,icol+1,(SQLTCHAR*)columnName,SQL_MAX_BUFFER_LEN,&columnLength,&columnSQLDataType,&columnColDef,&columnColScale,&columnNull);
		if (returncode != SQL_SUCCESS)
		{
			_stprintf(temp,_T("Unable to describe column %d after select statement"),icol+1); 
			_tprintf(temp);
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

		columnAttribute = (_TCHAR *)malloc(sizeof(_TCHAR)*SQL_MAX_BUFFER_LEN);
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_NAME,columnAttribute,SQL_MAX_BUFFER_LEN*sizeof(_TCHAR),&pcDesc,&pfDesc);
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to get column attribute name after select statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		if (_tcsnicmp(ColumnInfo3[icol].Name,(_TCHAR *)columnAttribute,pcDesc/sizeof(_TCHAR)) != 0)
		{
			_stprintf(temp,_T("Column %d doesn't match column name expected: %s and actual: %s"),icol+1,ColumnInfo3[icol].Name,columnAttribute); 
			_tprintf(temp);
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_TYPE,columnAttribute,0,&pcDesc,&pfDesc);
		if (returncode != SQL_SUCCESS)
		{
			_tprintf(_T("Unable to get column attribute type after select statement"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		free(columnAttribute); 
	
		switch (ColumnInfo3[icol].DataType)
		{
			case SQL_CHAR:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo3[icol].CDataType,UTF8CharOutput,sizeof(_TCHAR)*MAX_COLUMN_OUTPUT,&UTF8CharOutputLen);
				break;
			case SQL_VARCHAR:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo3[icol].CDataType,UTF8VarcharOutput,sizeof(_TCHAR)*MAX_COLUMN_OUTPUT,&UTF8VarcharOutputLen);
				break;
			case SQL_WCHAR:
				if (icol == 2)
					returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo3[icol].CDataType,WCharOutput1,sizeof(_TCHAR)*MAX_COLUMN_OUTPUT,&WCharOutputLen1);
				else
					returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo3[icol].CDataType,WCharOutput2,sizeof(_TCHAR)*MAX_COLUMN_OUTPUT,&WCharOutputLen2);
				break;
			case SQL_WVARCHAR:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo3[icol].CDataType,WVarcharOutput,sizeof(_TCHAR)*MAX_COLUMN_OUTPUT,&WVarcharOutputLen);
				break;
			case SQL_BIT:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo3[icol].CDataType,&BitOutput,sizeof(SWORD),&BitOutputLen);
				break;
			case SQL_TINYINT:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo3[icol].CDataType,&TinyintOutput,sizeof(SWORD),&TinyintOutputLen);
				break;
			case SQL_BINARY:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo3[icol].CDataType,BinaryOutput,sizeof(_TCHAR)*MAX_COLUMN_OUTPUT,&BinaryOutputLen);
				break;
			case SQL_VARBINARY:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo3[icol].CDataType,VarBinaryOutput,sizeof(_TCHAR)*MAX_COLUMN_OUTPUT,&VarBinaryOutputLen);
				break;
		}
		if (returncode != SQL_SUCCESS)
		{
			_stprintf(temp,_T("Unable to bind column %d after select statement"),icol); 
			_tprintf(temp);
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
	
	}

	irow = 0;
	while (returncode == SQL_SUCCESS)
	{
		returncode = SQLFetch(pTestInfo->hstmt);
		if(returncode != SQL_NO_DATA_FOUND && returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
		{
			_tprintf(_T("Unable to fetch after bind column"));
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		else if (returncode == SQL_SUCCESS || returncode == SQL_SUCCESS_WITH_INFO)
		{
			icol = 1;
			if (_tcsnicmp(InputOutputValues3[irow].UTF8CharValue,UTF8CharOutput,_tcslen(InputOutputValues3[irow].UTF8CharValue)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,InputOutputValues3[irow].UTF8CharValue,UTF8CharOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_tcsnicmp(InputOutputValues3[irow].UTF8VarcharValue,UTF8VarcharOutput,_tcslen(InputOutputValues3[irow].UTF8VarcharValue)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,InputOutputValues3[irow].UTF8VarcharValue,UTF8VarcharOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_tcsnicmp(InputOutputValues3[irow].WCharValue1,WCharOutput1,_tcslen(InputOutputValues3[irow].WCharValue1)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,InputOutputValues3[irow].WCharValue1,WCharOutput1); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_tcsnicmp(InputOutputValues3[irow].WCharValue2,WCharOutput2,_tcslen(InputOutputValues3[irow].WCharValue2)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,InputOutputValues3[irow].WCharValue2,WCharOutput2); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_tcsnicmp(InputOutputValues3[irow].WVarcharValue,WVarcharOutput,_tcslen(InputOutputValues3[irow].WVarcharValue)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,InputOutputValues3[irow].WVarcharValue,WVarcharOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (InputOutputValues3[irow].BitValue != BitOutput)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %d and actual: %d"),icol,InputOutputValues3[irow].BitValue,BitOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (InputOutputValues3[irow].TinyintValue != TinyintOutput)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %d and actual: %d"),icol,InputOutputValues3[irow].TinyintValue,TinyintOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_tcsnicmp(InputOutputValues3[irow].BinaryValue,BinaryOutput,_tcslen(InputOutputValues3[irow].BinaryValue)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,InputOutputValues3[irow].BinaryValue,BinaryOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_tcsnicmp(InputOutputValues3[irow].VarBinaryValue,VarBinaryOutput,_tcslen(InputOutputValues3[irow].VarBinaryValue)) != 0)
			{
				_stprintf(temp,_T("Column %d output doesn't match expected: %s and actual: %s"),icol,InputOutputValues3[irow].VarBinaryValue,VarBinaryOutput); 
				_tprintf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			irow++;
		}
	}

	returncode = SQLFreeStmt(pTestInfo->hstmt,SQL_CLOSE);
//	returncode = SQLFreeHandle (SQL_HANDLE_STMT, hstmt);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to freestmt with CLOSE option"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	free(UTF8CharOutput);
	free(UTF8VarcharOutput);
	free(WCharOutput1);
	free(WCharOutput2);
	free(WVarcharOutput);
	free(BinaryOutput);
	free(VarBinaryOutput);

	return(TRUE);
}

//#####################################################################################################
// This functions cover the following APIs:
// SQLTables
// SQLColumns
// SQLPrimaryKeys
// SQLStatistics
// SQLSpecialColumns
//#####################################################################################################
BOOL CatalogFunctions()
{
	RETCODE		returncode;
	_TCHAR		temp[300];
	_TCHAR		*TableType = _T("TABLE");
	_TCHAR		*Remark = _T("");
//  _TCHAR		*CatalogNameOutput,*SchemaNameOutput,*TableNameOuput,*TableTypeOutput,*RemarkOutput;
//	SDWORD		CatalogNameOutputLen,SchemaNameOutputLen,TableNameOuputLen,TableTypeOutputLen,RemarkOutputLen;


	returncode = SQLTables(pTestInfo->hstmt,(SQLTCHAR *)pTestInfo->Catalog,(SWORD)_tcslen(pTestInfo->Catalog),(SQLTCHAR *)pTestInfo->Schema,(SWORD)_tcslen(pTestInfo->Schema),(SQLTCHAR *)pTestInfo->Table,(SWORD)_tcslen(pTestInfo->Table),(SQLTCHAR *)TableType,(SWORD)_tcslen(TableType));
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Catalog API Tables failed"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode = SQLFreeStmt(pTestInfo->hstmt,SQL_CLOSE);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to freestmt with CLOSE option after Tables API"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLColumns(pTestInfo->hstmt,(SQLTCHAR *)pTestInfo->Catalog,(SWORD)_tcslen(pTestInfo->Catalog),(SQLTCHAR *)pTestInfo->Schema,(SWORD)_tcslen(pTestInfo->Schema),(SQLTCHAR *)pTestInfo->Table,(SWORD)_tcslen(pTestInfo->Table),(SQLTCHAR *)ColumnInfo[0].Name,(SWORD)_tcslen(ColumnInfo[0].Name));
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Catalog API Columns failed"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode = SQLFreeStmt(pTestInfo->hstmt,SQL_CLOSE);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to freestmt with CLOSE option after Columns API"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLStatistics(pTestInfo->hstmt,(SQLTCHAR *)pTestInfo->Catalog,(SWORD)_tcslen(pTestInfo->Catalog),(SQLTCHAR *)pTestInfo->Schema,(SWORD)_tcslen(pTestInfo->Schema),(SQLTCHAR *)pTestInfo->Table,(SWORD)_tcslen(pTestInfo->Table),SQL_INDEX_UNIQUE,SQL_QUICK);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Catalog API Statistics failed"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode = SQLFreeStmt(pTestInfo->hstmt,SQL_CLOSE);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to freestmt with CLOSE option after Statistics API"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLPrimaryKeys(pTestInfo->hstmt,(SQLTCHAR *)pTestInfo->Catalog,(SWORD)_tcslen(pTestInfo->Catalog),(SQLTCHAR *)pTestInfo->Schema,(SWORD)_tcslen(pTestInfo->Schema),(SQLTCHAR *)pTestInfo->Table,(SWORD)_tcslen(pTestInfo->Table));
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Catalog API Primary Keys failed"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode = SQLFreeStmt(pTestInfo->hstmt,SQL_CLOSE);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to freestmt with CLOSE option after Primary Keys API"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
				
	returncode = SQLSpecialColumns(pTestInfo->hstmt,SQL_ROWVER,(SQLTCHAR *)pTestInfo->Catalog,(SWORD)_tcslen(pTestInfo->Catalog),(SQLTCHAR *)pTestInfo->Schema,(SWORD)_tcslen(pTestInfo->Schema),(SQLTCHAR *)pTestInfo->Table,(SWORD)_tcslen(pTestInfo->Table),SQL_SCOPE_TRANSACTION,SQL_NULLABLE);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Catalog API Special Columns Keys failed"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode = SQLFreeStmt(pTestInfo->hstmt,SQL_CLOSE);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Unable to freestmt with CLOSE option after Primary Keys API"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

/*	CatalogNameOutput = (_TCHAR *)malloc(sizeof(_TCHAR)*SQL_MAX_DSN_LENGTH);
	SchemaNameOutput = (_TCHAR *)malloc(sizeof(_TCHAR)*SQL_MAX_SCHEMA_NAME_LEN);
	TableNameOuput = (_TCHAR *)malloc(sizeof(_TCHAR)*SQL_MAX_TABLE_NAME_LEN);
	TableTypeOutput = (_TCHAR *)malloc(sizeof(_TCHAR)*SQL_MAX_TABLE_TYPE_LEN);
	RemarkOutput = (_TCHAR *)malloc(sizeof(_TCHAR)*SQL_MAX_REMARK_LEN);

	_tcscpy(CatalogNameOutput,_T(""));
	_tcscpy(SchemaNameOutput,_T(""));
	_tcscpy(TableNameOuput,_T(""));
	_tcscpy(TableTypeOutput,_T(""));
	_tcscpy(RemarkOutput,_T(""));

	returncode = SQLBindCol(hstmt,1,SQL_C_TCHAR,CatalogNameOutput,SQL_MAX_DSN_LENGTH,&CatalogNameOutputLen);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Catalog API Tables failed while binding Catalog name"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode=SQLBindCol(hstmt,2,SQL_C_TCHAR,SchemaNameOutput,SQL_MAX_SCHEMA_NAME_LEN,&SchemaNameOutputLen);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Catalog API Tables failed while binding Schema name"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode=SQLBindCol(hstmt,3,SQL_C_TCHAR,TableNameOuput,NAME_LEN,&TableNameOuputLen);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Catalog API Tables failed while binding Table name"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode=SQLBindCol(hstmt,4,SQL_C_TCHAR,TableTypeOuput,NAME_LEN,&TableTypeOuputLen);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Catalog API Tables failed while binding Table Type"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode=SQLBindCol(hstmt,5,SQL_C_TCHAR,RemarkOutput,NAME_LEN,&RemarkOutputLen);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Catalog API Tables failed while binding Remark"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode = SQLFetch(hstmt);
	if (returncode != SQL_SUCCESS)
	{
		_tprintf(_T("Catalog API Tables failed while Fetch"));
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (_stricmp(pTestInfo->CatalogName,CatalogNameOutput) != 0)
	{
		_stprintf(temp,Catalog Name is not matching expected: %s and actual %s,pTestInfo->CatalogName,CatalogNameOutput);
		_tprintf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (_stricmp(pTestInfo->SchemaName,SchemaNameOutput) != 0) 
	{
		_stprintf(temp,_T("Schema Name is not matching expected: %s and actual %s,pTestInfo->SchemaName,SchemaNameOutput);
		_tprintf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (_stricmp(pTestInfo->TableName,TableNameOutput) != 0)
	{
		_stprintf(temp,_T("Table Name is not matching expected: %s and actual %s,pTestInfo->TableName,TableNameOutput);
		_tprintf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (_stricmp(Table[i].TableType,oTableType) != 0)
	{
		_stprintf(temp,_T("Table Type is not matching expected: %s and actual %s,TableType,TableTypeOutput);
		_tprintf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (_stricmp(Remark,oRemark) != 0)
	{
		_stprintf(temp,_T("Remark is not matching expected: %s and actual %s,Remark,RemarkOutput);
		_tprintf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
*/
	return(TRUE);
}



// Main Program
int _tmain(int argc, _TCHAR* argv[])
{
	//process command line params

	if (argc == 4) {
		_tcscpy(testInfo.DataSource, argv[1]);
		_tcscpy(testInfo.UserName, argv[2]);
		_tcscpy(testInfo.Password, argv[3]);
		testInfo.Server[0] = 0;
		testInfo.Port[0] = 0;
	}
	else if (argc == 5) {
		_tcscpy(testInfo.Server, argv[1]);
		_tcscpy(testInfo.Port, argv[2]);
		_tcscpy(testInfo.UserName, argv[3]);
		_tcscpy(testInfo.Password, argv[4]);
		testInfo.DataSource[0] = 0;
	}
	else {
		_tprintf(_T("Usage: %s <datasource> <username> <password>\n")
				 _T("Or:    %s <server> <port> <username> <password>\n")
				 , argv[0], argv[0] );
		exit(1);
	}

	_tcscpy(testInfo.Catalog, _T("CAT"));
	_tcscpy(testInfo.Schema, _T("SCH"));

	pTestInfo = &testInfo;
	pTestInfo->henv = (HENV) NULL;
	pTestInfo->hstmt = (HSTMT) NULL;
	pTestInfo->hdbc = (HDBC) NULL;

	bool BitPass = true;

	BitPass = FullConnect();
	if(BitPass)
		BitPass = InitialSetup();
	if(BitPass)
		BitPass = GetAllInfo();
	if(BitPass)
		BitPass = StatementFunctions();	
	if(BitPass)
		BitPass = ResultFunctions();	
	if(BitPass)
		BitPass = CatalogFunctions();	
	if(BitPass)
		BitPass = Cleanup();

// Adding a bunch of new procedures to test new datatypes support.
// LongvarChar, Interval data types, Variable input formats for data, time & ts.

	_tcscpy((_TCHAR *)pTestInfo->Table,_T("ODBCTAB2"));

	if(BitPass)
		BitPass = InitialSetup2();
	if(BitPass)
		BitPass = StatementFunctions2();	
	if(BitPass)
		BitPass = ResultFunctions2();	
	if(BitPass)
		BitPass = Cleanup();

// These new procedures will test new datatypes support for SQ.
// CHAR(UTF8), VARCHAR(UTF8), WCHAR, VARWCHAR, BIT, TINYINT, BINARY, VARBINARY
		
	_tcscpy((_TCHAR *)pTestInfo->Table,_T("ODBCTAB3"));

	if(BitPass)
		BitPass = InitialSetup3();
	if(BitPass)
		BitPass = StatementFunctions3();	
	if(BitPass)
		BitPass = ResultFunctions3();	
	if(BitPass)
		BitPass = Cleanup();

	if(BitPass)
		BitPass = FullDisconnect();

	if (BitPass) {
		_tprintf(_T("\nODBCBIT has PASSED.\n\n"));
	}
	else {
		_tprintf(_T("\nODBCBIT has FAILED.\n\n"));
		exit(1);
	}

	exit(0);
}





