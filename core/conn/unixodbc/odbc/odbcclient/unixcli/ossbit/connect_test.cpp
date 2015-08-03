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
#include "mxosql.h"
#include "ossbit.h"
#include <assert.h>

HANDLE hInst;
TestInfo *pTestInfo;
SQLHWND hWnd;

#ifdef ONYX
	int Count = 1;
	int loop, logging=0, loglevel=0;
	char *Userid = NULL;
	char *Password = NULL;
	char *Table = NULL;
#endif

void LogAllErrors(TestInfo *pTestInfo)
{             
	char buf[MAX_SQLSTRING_LEN];
	char State[STATE_SIZE];
	RETCODE returncode;

	/* Log any henv error messages */
	strcpy(buf,"");
	strcpy(State,"");
	returncode = SQLError(pTestInfo->henv, NULL, NULL, (UCHAR *)State, NULL, (UCHAR *)buf, MAX_SQLSTRING_LEN, NULL);
	while((returncode == SQL_SUCCESS) || (returncode == SQL_SUCCESS_WITH_INFO))
	{
		printf("%s %s\n",buf,State);
		returncode = SQLError(pTestInfo->henv, NULL, NULL, (UCHAR *)State, NULL, (UCHAR *)buf, MAX_SQLSTRING_LEN, NULL);
	}

   /* Log any hdbc error messages */
	returncode = SQLError(NULL, pTestInfo->hdbc, NULL, (UCHAR *)State, NULL, (UCHAR *)buf, MAX_SQLSTRING_LEN, NULL);
	while((returncode == SQL_SUCCESS) || (returncode == SQL_SUCCESS_WITH_INFO))
	{
		printf("%s %s\n",buf,State);
		returncode = SQLError(NULL, pTestInfo->hdbc, NULL, (UCHAR *)State, NULL, (UCHAR *)buf, MAX_SQLSTRING_LEN, NULL);
	}

   /* Log any hstmt error messages */
	returncode = SQLError(NULL, NULL, pTestInfo->hstmt, (UCHAR *)State, NULL, (UCHAR *)buf, MAX_SQLSTRING_LEN, NULL);
	while((returncode == SQL_SUCCESS) || (returncode == SQL_SUCCESS_WITH_INFO))
	{
		printf("%s %s\n",buf,State);
		returncode = SQLError(NULL, NULL, pTestInfo->hstmt, (UCHAR *)State, NULL, (UCHAR *)buf, MAX_SQLSTRING_LEN, NULL);
	}

}                     

//#####################################################################################################
// Those functions cover the following APIs:
// SQLAllocEnv
// SQLAllocConnect
// SQLDriverConnect
// SQLAllocStmt
//#####################################################################################################
BOOL FullConnectPromptUser(TestInfo *pTestInfo)
{
	RETCODE returncode;
	char	ConnectString[MAX_CONNECT_STRING];
	short	ConnStrLength;
	char	*pString;
	char	*pTempString;
	char	TempString[MAX_CONNECT_STRING];
	SQLHENV	henv;
	SQLHDBC	hdbc;
	SQLHSTMT hstmt;
	char*	dsnName = DSN;
	char*	password = PWD;
	char*	user = UID;

	returncode = SQLAllocEnv(&henv);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to Allocate Envirnoment (AllocEnv)\n");
		return(FALSE);
	}
	pTestInfo->henv = henv;

	returncode = SQLAllocConnect(henv,&hdbc);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to Allocate Connect (AllocConnect)\n");
		LogAllErrors(pTestInfo);		
		SQLFreeEnv(henv);
		return(FALSE);
	}
	pTestInfo->hdbc = hdbc;
#ifdef ONYX
	if (Userid)
		user = Userid;
	if (Password)
		password = Password;
#endif
	sprintf(TempString,"DSN=%s;UID=%s;PWD=%s;",dsnName, user, password);
	returncode = SQLDriverConnect(hdbc,hWnd,
						(UCHAR*)TempString,strlen(TempString),
                        (UCHAR*)ConnectString,sizeof(ConnectString),&ConnStrLength,
                        SQL_DRIVER_NOPROMPT);
	printf("%s\n", TempString);
	if ((returncode != SQL_SUCCESS) && (returncode != SQL_SUCCESS_WITH_INFO))
	{
		printf("Unable to Connect (Connect)\n");
		LogAllErrors(pTestInfo);		
		SQLFreeConnect(hdbc);
		SQLFreeEnv(henv);
		return(FALSE);
	}
	
	returncode = SQLAllocStmt(hdbc,&hstmt);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to Allocate Statement", "Statement");
		LogAllErrors(pTestInfo);		
		SQLDisconnect(hdbc);
		SQLFreeConnect(hdbc);
		SQLFreeEnv(henv);
		return(FALSE);
	}
	pTestInfo->hstmt = hstmt;
      
   // Strip out DSN from connection string. 
	pString = strstr(ConnectString,"DSN=");
	pString += 4;           // skip past 'DSN=' 
  pTempString = TempString;
	while(*pString != ';')
	{
		*pTempString = *pString;
		pTempString++;
		pString++;
	}         
	*pTempString = (char)NULL;
	strcpy(pTestInfo->DataSource,TempString);

	// Strip out UID from connection string. 
	pString = strstr(ConnectString,"UID=");
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
	  *pTempString = (char)NULL;
		strcpy(pTestInfo->UserID,TempString);
	}
  else
		strcpy(pTestInfo->UserID,"");

   // Strip out PWD from connection string. 
	pString = strstr(ConnectString,"PWD=");
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
		*pTempString = (char)NULL;
		strcpy((char *)pTestInfo->Password,TempString);
	}
	else
		strcpy((char *)pTestInfo->Password,"");
   
	// Strip out DBQ from connection string. 
	pString = strstr(ConnectString,"DBQ=");
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
		*pTempString = (char)NULL;
		strcpy((char *)pTestInfo->Database,TempString);
	}
	else
		strcpy((char *)pTestInfo->Database,"MASTER");

	// Strip out CATALOG from connection string. 
	pString = strstr(ConnectString,"CATALOG=");
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
		*pTempString = (char)NULL;
		strcpy((char *)pTestInfo->Catalog,TempString);
	}
	else
		strcpy((char *)pTestInfo->Catalog,"TANDEM_SYSTEM_NSK");

	// Strip out SCHEMA from connection string. 
	pString = strstr(ConnectString,"SCHEMA=");
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
		*pTempString = (char)NULL;
		strcpy((char *)pTestInfo->Schema,TempString);
	}
	else
		strcpy((char *)pTestInfo->Schema,"ODBC_SCHEMA");
#ifdef ONYX
	if (Table)
		strcpy((char *)pTestInfo->Table,Table);
	else
#endif ONYX
	strcpy((char *)pTestInfo->Table,ODBCTAB);

   return(TRUE);
}



char *CreateCatalog(TestInfo *pTestInfo)
{

	sprintf(SQLStmt,"create catalog %s",pTestInfo->Catalog);
	return (SQLStmt);

}

char *CreateSchema(TestInfo *pTestInfo)
{

	sprintf(SQLStmt,"create schema %s",pTestInfo->Schema);
	return (SQLStmt);

}

char *DropTable(TestInfo *pTestInfo)
{

	sprintf(SQLStmt,"%s %s",DROP_TABLE, pTestInfo->Table);
	return(SQLStmt);

}

char *CreateTable(TestInfo *pTestInfo)
{
	int i;
	char *pk;		// primary key string
	BOOL YN = FALSE;

	pk = (char *) malloc(300);
	strcpy(pk,"");

	strcpy(SQLStmt,"");
	strcat(SQLStmt,CREATE_TABLE);
	if (strlen(pTestInfo->Catalog) > 0)
	{
		strcat (SQLStmt, pTestInfo->Catalog);
		strcat(SQLStmt, ".");
	}
	if (strlen(pTestInfo->Schema) > 0)
	{
		strcat(SQLStmt, pTestInfo->Schema);
		strcat(SQLStmt, ".");
	}
	strcat(SQLStmt,pTestInfo->Table);
	strcat(SQLStmt," (");
	i = 0;
	while (strcmp(ColumnInfo[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (strcmp(ColumnInfo[i].DataTypestr,END_LOOP) != 0))
			strcat(SQLStmt,",");
		strcat(SQLStmt,ColumnInfo[i].Name);
		strcat(SQLStmt," ");
		strcat(SQLStmt,ColumnInfo[i].Description);
		if (strcmp(ColumnInfo[i].Precision,"") != 0)
		{
			strcat(SQLStmt,"(");
			strcat(SQLStmt,ColumnInfo[i].Precision);
			if (strcmp(ColumnInfo[i].Scale,"") != 0)
			{
				strcat(SQLStmt,",");
				strcat(SQLStmt,ColumnInfo[i].Scale);
			}
			strcat(SQLStmt,")");
		}
		if (ColumnInfo[i].PriKey)
		{
			strcat(SQLStmt," NOT NULL");
			if (YN)
				strcat(pk,",");
			strcat(pk,ColumnInfo[i].Name);
  		YN = TRUE;
		}
		i++;
	}

	
	if (strcmp(pk,"") != 0)
	{
		strcat(SQLStmt,", PRIMARY KEY (");
		strcat(SQLStmt,pk);
		strcat(SQLStmt,")");
	}
	strcat(SQLStmt,")");
	Actual_Num_Columns = i;
	free(pk); 

	return (SQLStmt);

}

char *InsertTable(TestInfo *pTestInfo)
{
	int i;

	strcpy(SQLStmt,"");
	strcat(SQLStmt,INSERT_TABLE);
	strcat(SQLStmt,pTestInfo->Table);
	strcat(SQLStmt," (");
	i = 0;
	while (strcmp(ColumnInfo[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (strcmp(ColumnInfo[i].DataTypestr,END_LOOP) != 0))
			strcat(SQLStmt,",");
		strcat(SQLStmt,ColumnInfo[i].Name);
		i++;
	}
	strcat(SQLStmt,") values (");
	
	i = 0;
	while (strcmp(ColumnInfo[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (strcmp(ColumnInfo[i].DataTypestr,END_LOOP) != 0))
			strcat(SQLStmt,",");
		strcat(SQLStmt,"?");
		i++;
	}
	strcat(SQLStmt,")");

	return (SQLStmt);
}

#ifdef ONYX
char *DeleteTable(TestInfo *pTestInfo)
{
	int i;

	strcpy(SQLStmt,"");
	strcat(SQLStmt,"delete from ");
	strcat(SQLStmt,pTestInfo->Table);

	return (SQLStmt);
}
#endif

char *SelectTable(TestInfo *pTestInfo)
{
	int i;

	strcpy(SQLStmt,"");
	strcat(SQLStmt,SELECT_TABLE);
	i = 0;
	while (strcmp(ColumnInfo[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (strcmp(ColumnInfo[i].DataTypestr,END_LOOP) != 0))
			strcat(SQLStmt,",");
		strcat(SQLStmt,ColumnInfo[i].Name);
		i++;
	}
	strcat(SQLStmt," from ");
	strcat(SQLStmt,pTestInfo->Table);
	return (SQLStmt);

}

void ReleaseAll(TestInfo *pTestInfo)
{
		SQLFreeStmt(pTestInfo->hstmt,SQL_DROP);
		SQLDisconnect(pTestInfo->hdbc);
		SQLFreeConnect(pTestInfo->hdbc);
		SQLFreeEnv(pTestInfo->henv);
}

//#####################################################################################################
// This functions cover the following APIs:
// SQLExecDirect
// SQLSetConnectOption
// SQLGetConnectOption
//#####################################################################################################
BOOL InitialSetup(TestInfo *pTestInfo)
{
  RETCODE returncode;                        
	PTR			pvParam;
	
	returncode = SQLSetConnectOption(pTestInfo->hdbc,SQL_ACCESS_MODE,SQL_MODE_READ_WRITE);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to set access mode to read/write (SetConnectOption)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLGetConnectOption(pTestInfo->hdbc,SQL_ACCESS_MODE,&pvParam);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to get access mode to read/write (GetConnectOption)\n");
	}
	
	if ((UDWORD)pvParam != SQL_MODE_READ_WRITE)
	{
		printf("Invalid access mode value for read/write returned (GetConnectOption)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLSetConnectOption(pTestInfo->hdbc,SQL_TXN_ISOLATION,SQL_TXN_READ_COMMITTED);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to set transaction isolation mode to read committed (SetConnectOption)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLGetConnectOption(pTestInfo->hdbc,SQL_TXN_ISOLATION,&pvParam);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to get transaction isolation mode to read committed (GetConnectOption)\n");
	}

	if ((UDWORD)pvParam != SQL_TXN_READ_COMMITTED)
	{
		printf("Invalid access mode value for read/write returned (GetConnectOption)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	SQLExecDirect(pTestInfo->hstmt,(UCHAR *)(pTestInfo),SQL_NTS); // Cleanup
	SQLExecDirect(pTestInfo->hstmt,(UCHAR *)DropTable(pTestInfo),SQL_NTS);
	returncode = SQLExecDirect(pTestInfo->hstmt,(UCHAR *)CreateTable(pTestInfo),SQL_NTS);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to Create Table (ExecDirect)\n");
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
BOOL Cleanup(TestInfo *pTestInfo)
{
  RETCODE returncode;                        
	char		*temp;
	ULONG		pvParam;

	temp = (char *)malloc(50);
	strcpy(temp,"");
	strcpy(temp,DropTable(pTestInfo));

	returncode = SQLSetStmtOption(pTestInfo->hstmt,SQL_ASYNC_ENABLE,SQL_ASYNC_ENABLE_OFF);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to set ASYNC mode to ENABLE_OFF (SetStmtOption)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode = SQLGetStmtOption(pTestInfo->hstmt,SQL_ASYNC_ENABLE,&pvParam);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to get ASYNC mode to ENABLE (GetStmtOption)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	if (pvParam != SQL_ASYNC_ENABLE_OFF)
	{
		printf("Invalid ASYNC mode value for ENABLE_OFF returned (GetStmtOption)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLSetStmtOption(pTestInfo->hstmt,SQL_ASYNC_ENABLE,SQL_ASYNC_ENABLE_ON);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to set ASYNC mode to ENABLE (SetStmtOption)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode = SQLGetStmtOption(pTestInfo->hstmt,SQL_ASYNC_ENABLE,&pvParam);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to get ASYNC mode to ENABLE (GetStmtOption)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	if (pvParam != SQL_ASYNC_ENABLE_ON)
	{
		printf("Invalid ASYNC mode value for ENABLE returned (GetStmtOption)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode = SQL_STILL_EXECUTING;
	while (returncode == SQL_STILL_EXECUTING)
	{
		returncode = SQLExecDirect(pTestInfo->hstmt,(UCHAR *)temp,SQL_NTS);
	}
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to Drop Table (ExecDirect)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		//return(FALSE);
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

BOOL FullDisconnect(TestInfo *pTestInfo)
{
  RETCODE returncode;                        
   
  returncode = SQLDisconnect(pTestInfo->hdbc);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to Disconnect (Disconnect)\n");
		SQLDisconnect(pTestInfo->hdbc);
		SQLFreeConnect(pTestInfo->hdbc);
		SQLFreeEnv(pTestInfo->henv);
		return(FALSE);
	}
   
  returncode = SQLFreeConnect(pTestInfo->hdbc);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to FreeConnect (FreeConnect)\n");
		SQLDisconnect(pTestInfo->hdbc);
		SQLFreeConnect(pTestInfo->hdbc);
		SQLFreeEnv(pTestInfo->henv);
		return(FALSE);
	}

   
  returncode = SQLFreeEnv(pTestInfo->henv);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to FreeEnvirnoment (FreeEnv)\n");
		SQLDisconnect(pTestInfo->hdbc);
		SQLFreeConnect(pTestInfo->hdbc);
		SQLFreeEnv(pTestInfo->henv);
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
BOOL GetAllInfo(TestInfo *pTestInfo)
{
  RETCODE returncode;                        
	UCHAR		szDSN[SQL_MAX_DSN_LENGTH], szDESC[SQL_MAX_DSN_LENGTH];
	SWORD		cbDSN, pcbDESC;
	UCHAR		szDRVDESC[SQL_MAX_DRIVER_LENGTH], szDRVATTR[SQL_MAX_DRIVER_LENGTH];
	SWORD		cbDRVDESC, pcbDRVATTR;
	PTR			fFuncs;

	returncode = SQLDataSources(pTestInfo->henv, SQL_FETCH_FIRST, szDSN, SQL_MAX_DSN_LENGTH, &cbDSN, szDESC, SQL_MAX_DSN_LENGTH, &pcbDESC); 
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to Fetch first datasource (DataSources)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

/*	returncode = SQLDataSources(pTestInfo->henv, SQL_FETCH_NEXT, szDSN, SQL_MAX_DSN_LENGTH, &cbDSN, szDESC, SQL_MAX_DSN_LENGTH, &pcbDESC); 
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to Fetch next datasource", "DataSources");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	} */

	returncode = SQLDrivers(pTestInfo->henv, SQL_FETCH_FIRST, szDRVDESC, SQL_MAX_DRIVER_LENGTH, &cbDRVDESC, szDRVATTR, SQL_MAX_DRIVER_LENGTH, &pcbDRVATTR); 
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to Fetch first driver (Drivers)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

/*	returncode = SQLDrivers(pTestInfo->henv, SQL_FETCH_NEXT, szDRVDESC, SQL_MAX_DRIVER_LENGTH, &cbDRVDESC, szDRVATTR, SQL_MAX_DRIVER_LENGTH, &pcbDRVATTR); 
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to Fetch next driver (Drivers)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	} */

	fFuncs = (char *)malloc(SQL_MAX_DSN_LENGTH);
	returncode = SQLGetInfo(pTestInfo->hdbc, SQL_DATA_SOURCE_NAME, fFuncs, SQL_MAX_DSN_LENGTH, NULL);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to get active connections (GetInfo)\n");
	}
	if (strcmp((char *)fFuncs,pTestInfo->DataSource) != 0)
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
BOOL StatementFunctions(TestInfo *pTestInfo)
{
  RETCODE	returncode;                        
	SWORD		numparams;
	char		temp[50];
	int			iparam, num_rows_insert;
	SWORD		paramSQLDataType;
	UDWORD	paramColDef;
	SWORD		paramColScale,paramColNull;

	num_rows_insert = 0;
	while (strcmp(InputOutputValues[num_rows_insert].CharValue,END_LOOP) != 0)
	{
		returncode = SQLPrepare(pTestInfo->hstmt,(UCHAR *)InsertTable(pTestInfo),SQL_NTS);
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to prepare insert Table (Prepare)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

		returncode = SQLNumParams(pTestInfo->hstmt,&numparams);
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to return number of parameters for insert statement (NumParams)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		if (numparams != Actual_Num_Columns)
		{
			sprintf(temp,"Number of parameters doesn't match expected: %d and actual: %d (NumParams)\n",numparams,Actual_Num_Columns); 
			printf(temp);
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

	// Begin of bind parameter
		returncode = SQLBindParameter(pTestInfo->hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, MAX_SQLSTRING_LEN, 0, InputOutputValues[num_rows_insert].CharValue, 0, &(InputOutputValues[num_rows_insert].InValue));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_CHAR to SQL_CHAR during insert statement (BindParam)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, MAX_SQLSTRING_LEN, 0, InputOutputValues[num_rows_insert].VarCharValue, 0, &(InputOutputValues[num_rows_insert].InValue));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_CHAR to SQL_VARCHAR during insert statement (BindParam)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_DECIMAL, 0, 0, InputOutputValues[num_rows_insert].DecimalValue, 0, &(InputOutputValues[num_rows_insert].InValue));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_CHAR to SQL_DECIMAL during insert statement (BindParam)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_NUMERIC, 0, 0, InputOutputValues[num_rows_insert].NumericValue, 0, &(InputOutputValues[num_rows_insert].InValue));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_CHAR to SQL_NUMERIC during insert statement (BindParam)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 5, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, 0, 0, &(InputOutputValues[num_rows_insert].ShortValue), 0, &(InputOutputValues[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_SHORT to SQL_SMALLINT during insert statement(BindParam)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 6, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &(InputOutputValues[num_rows_insert].LongValue), 0, &(InputOutputValues[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_LONG to SQL_INTEGER during insert statement (BindParam)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 7, SQL_PARAM_INPUT, SQL_C_FLOAT, SQL_REAL, 0, 0, &(InputOutputValues[num_rows_insert].RealValue), 0, &(InputOutputValues[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_FLOAT to SQL_REAL during insert statement (BindParam)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

		returncode = SQLBindParameter(pTestInfo->hstmt, 8, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_FLOAT, 0, 0, &(InputOutputValues[num_rows_insert].FloatValue), 0, &(InputOutputValues[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_DOUBLE to SQL_FLOAT during insert statement (BindParam)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

		returncode = SQLBindParameter(pTestInfo->hstmt, 9, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE, 0, 0, &(InputOutputValues[num_rows_insert].DoubleValue), 0, &(InputOutputValues[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_DOUBLE to SQL_DOUBLE during insert statement (BindParam)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 10, SQL_PARAM_INPUT, SQL_C_DATE, SQL_DATE, 0, 0, &(InputOutputValues[num_rows_insert].DateValue), 0, &(InputOutputValues[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_DATE to SQL_DATE during insert statement (BindParam)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 11, SQL_PARAM_INPUT, SQL_C_TIME, SQL_TIME, 0, 0, &(InputOutputValues[num_rows_insert].TimeValue), 0, &(InputOutputValues[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_TIME to SQL_TIME during insert statement (BindParam)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 12, SQL_PARAM_INPUT, SQL_C_TIMESTAMP, SQL_TIMESTAMP, 0, 0, &(InputOutputValues[num_rows_insert].TimestampValue), 0, &(InputOutputValues[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_TIMESTAMP to SQL_TIMESTAMP during insert statement (BindParam)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 13, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_BIGINT, MAX_SQLSTRING_LEN, 0, InputOutputValues[num_rows_insert].BigintValue, 0, &(InputOutputValues[num_rows_insert].InValue));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_CHAR to SQL_BIGINT during insert statement (BindParam)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

	// End of bind parameter

		returncode = SQLExecute(pTestInfo->hstmt);
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to execute the insert statement after bind parameter (Execute)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

		for(iparam = 1; iparam <= numparams; iparam++)
		{
			returncode = SQLDescribeParam(pTestInfo->hstmt,iparam,&paramSQLDataType,&paramColDef,&paramColScale,&paramColNull);
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to execute describe parameter after insert (DescribeParam)\n");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			if (iparam != 8)	// bug since SQL return DOUBLE for FLOAT also.
			{
				if (paramSQLDataType != ColumnInfo[iparam-1].DataType)
				{
					sprintf(temp,"Parameter %d doesn't match expected: %d and actual: %d (DescribeParam)\n",iparam,paramSQLDataType,ColumnInfo[iparam-1].DataType); 
					printf(temp);
					ReleaseAll(pTestInfo);
					return(FALSE);
				}
			}
		} // end of for loop 
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
BOOL ResultFunctions(TestInfo *pTestInfo)
{
  RETCODE						returncode;                        
	SWORD							numcols;
	int								icol;
	UCHAR							columnName[SQL_MAX_COLUMN_NAME_LEN];
	SWORD							columnLength, columnSQLDataType;
	UDWORD						columnColDef;
	SWORD							columnColScale,columnNull;
	char							temp[50];
	PTR								columnAttribute;
	SWORD							pcDesc;
	SDWORD						pfDesc;
	char							*CharOutput;
	char							*VarCharOutput;
	char							*DecimalOutput;
	char							*NumericOutput;
	SWORD							ShortOutput;
	SDWORD						LongOutput;
	SFLOAT						RealOutput;
	SDOUBLE						FloatOutput;
	SDOUBLE						DoubleOutput;
	DATE_STRUCT				DateOutput;
	TIME_STRUCT				TimeOutput;
	TIMESTAMP_STRUCT	TimestampOutput;
	char							*BigintOutput;
	SDWORD						CharOutputLen;
	SDWORD						VarCharOutputLen;
	SDWORD						DecimalOutputLen;
	SDWORD						NumericOutputLen;
	SDWORD						ShortOutputLen;
	SDWORD						LongOutputLen;
	SDWORD						RealOutputLen;
	SDWORD						FloatOutputLen;
	SDWORD						DoubleOutputLen;
	SDWORD						DateOutputLen;
	SDWORD						TimeOutputLen;
	SDWORD						TimestampOutputLen;
	SDWORD						BigintOutputLen;

	CharOutput = (char *)malloc(MAX_COLUMN_OUTPUT);
	VarCharOutput = (char *)malloc(MAX_COLUMN_OUTPUT);
	DecimalOutput = (char *)malloc(MAX_COLUMN_OUTPUT);
	NumericOutput = (char *)malloc(MAX_COLUMN_OUTPUT);
	BigintOutput = (char *)malloc(MAX_COLUMN_OUTPUT);

	returncode = SQLExecDirect(pTestInfo->hstmt,(UCHAR *)SelectTable(pTestInfo),SQL_NTS);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to Drop Table (ExecDirect)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLNumResultCols(pTestInfo->hstmt,&numcols);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to return number of parameters for insert statement (NumParams)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (numcols != Actual_Num_Columns)
	{
		sprintf(temp,"Number of columns doesn't match expected: %d and actual: %d (NumCols)\n",numcols,Actual_Num_Columns); 
		printf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	for (icol = 0; icol < numcols; icol++)
	{
		returncode = SQLDescribeCol(pTestInfo->hstmt,icol+1,columnName,SQL_MAX_COLUMN_NAME_LEN,&columnLength,&columnSQLDataType,&columnColDef,&columnColScale,&columnNull);
		if (returncode != SQL_SUCCESS)
		{
			sprintf(temp,"Unable to describe column %d after select statement (DescribeCol)\n",icol+1); 
			printf(temp);
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		if ((icol+1) != 8 && (icol+1) != 13) // bug since SQL return DOUBLE for FLOAT and NUMERIC for LARGE_INT also. May be we treat this as feature.
		{
			if ((_strnicmp((char *)columnName,ColumnInfo[icol].Name,columnLength) != 0) && (columnSQLDataType != ColumnInfo[icol].DataType))
			{
				sprintf(temp,"Column %d doesn't match column name expected: %s and actual: %s and datatype expected: %d and actual: %d (DescribeCol)\n",icol+1,ColumnInfo[icol].Name,columnName,ColumnInfo[icol].DataType,columnSQLDataType); 
				printf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
		}
		columnAttribute = (char *)malloc(SQL_MAX_COLUMN_NAME_LEN);
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_NAME,columnAttribute,SQL_MAX_COLUMN_NAME_LEN,&pcDesc,&pfDesc);
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to get column attribute name after select statement (ColumnAttribute)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		if (_strnicmp(ColumnInfo[icol].Name,(char *)columnAttribute,pcDesc) != 0)
		{
			sprintf(temp,"Column %d doesn't match column name expected: %s and actual: %s (DescribeCol)\n",icol+1,ColumnInfo[icol].Name,columnAttribute); 
			printf(temp);
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_TYPE,columnAttribute,0,&pcDesc,&pfDesc);
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to get column attribute type after select statement (ColumnAttribute)\n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		if ((icol+1) != 8 && (icol+1) != 13) // bug since SQL return DOUBLE for FLOAT also NUMERIC for LARGE_INT. May be we treat this as feature.
		{
			if (ColumnInfo[icol].DataType != pfDesc)
			{
				sprintf(temp,"Column %d doesn't match column type expected: %d and actual: %d (DescribeCol)\n",icol+1,ColumnInfo[icol].DataType,pfDesc); 
				printf(temp);
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
		}
		free(columnAttribute); 
	
		switch (ColumnInfo[icol].DataType)
		{
			case SQL_CHAR:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,CharOutput,MAX_COLUMN_OUTPUT,&CharOutputLen);
				break;
			case SQL_VARCHAR:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,VarCharOutput,MAX_COLUMN_OUTPUT,&VarCharOutputLen);
				break;
			case SQL_DECIMAL:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,DecimalOutput,MAX_COLUMN_OUTPUT,&DecimalOutputLen);
				break;
			case SQL_NUMERIC:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,NumericOutput,MAX_COLUMN_OUTPUT,&NumericOutputLen);
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
			case SQL_DATE:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,&DateOutput,0,&DateOutputLen);
				break;
			case SQL_TIME:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,&TimeOutput,0,&TimeOutputLen);
				break;
			case SQL_TIMESTAMP:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,&TimestampOutput,0,&TimestampOutputLen);
				break;
			case SQL_BIGINT:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,BigintOutput,MAX_COLUMN_OUTPUT,&BigintOutputLen);
				break;
		}
		if (returncode != SQL_SUCCESS)
		{
			sprintf(temp,"Unable to bind column %d after select statement (BindCol)\n",icol); 
			printf(temp);
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
	
	}
	returncode = SQLFetch(pTestInfo->hstmt);
	if (returncode != SQL_SUCCESS)
	{
		assert(0);
		printf("Unable to fetch after bind column (Fetch)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	icol = 0;
	// for future release you can loop here
	if (_strnicmp(InputOutputValues[icol].CharValue,CharOutput,strlen(InputOutputValues[icol].CharValue)) != 0)
	{
		sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s (DescribeCol)\n",icol+1,InputOutputValues[icol].CharValue,CharOutput); 
		printf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (_strnicmp(InputOutputValues[icol].VarCharValue,VarCharOutput,strlen(InputOutputValues[icol].VarCharValue)) != 0)
	{
		sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s (DescribeCol)\n",icol+1,InputOutputValues[icol].VarCharValue,VarCharOutput); 
		printf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (_strnicmp(InputOutputValues[icol].DecimalValue,DecimalOutput,strlen(InputOutputValues[icol].DecimalValue)) != 0)
	{
		sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s (DescribeCol)\n",icol+1,InputOutputValues[icol].DecimalValue,DecimalOutput); 
		printf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (_strnicmp(InputOutputValues[icol].NumericValue,NumericOutput,strlen(InputOutputValues[icol].NumericValue)) != 0)
	{
		sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s ,(DescribeCol)\n",icol+1,InputOutputValues[icol].NumericValue,NumericOutput); 
		printf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (InputOutputValues[icol].ShortValue != ShortOutput)
	{
		sprintf(temp,"Column %d output doesn't match expected: %d and actual: %d ,(DescribeCol)\n",icol+1,InputOutputValues[icol].ShortValue,ShortOutput); 
		printf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (InputOutputValues[icol].LongValue != LongOutput)
	{
		sprintf(temp,"Column %d output doesn't match expected: %d and actual: %d (DescribeCol)\n",icol+1,InputOutputValues[icol].LongValue,LongOutput); 
		printf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (InputOutputValues[icol].RealValue != RealOutput)
	{
		sprintf(temp,"Column %d output doesn't match expected: %f and actual: %f (DescribeCol)\n",icol+1,InputOutputValues[icol].RealValue,RealOutput); 
		printf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (InputOutputValues[icol].FloatValue != FloatOutput)
	{
		sprintf(temp,"Column %d output doesn't match expected: %e and actual: %e (DescribeCol)\n",icol+1,InputOutputValues[icol].FloatValue,FloatOutput); 
		printf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	if (InputOutputValues[icol].DoubleValue != DoubleOutput)
	{
		sprintf(temp,"Column %d output doesn't match expected: %e and actual: %e (DescribeCol)\n",icol+1,InputOutputValues[icol].DoubleValue,DoubleOutput); 
		printf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if ((InputOutputValues[icol].DateValue.month != DateOutput.month) && (InputOutputValues[icol].DateValue.day != DateOutput.day) && (InputOutputValues[icol].DateValue.year != DateOutput.year))
	{
		sprintf(temp,"Column %d output doesn't match expected: %d and actual: %d (DescribeCol)\n",icol+1,InputOutputValues[icol].DateValue,DateOutput); 
		printf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if ((InputOutputValues[icol].TimeValue.hour != TimeOutput.hour) && (InputOutputValues[icol].TimeValue.minute != TimeOutput.minute) && (InputOutputValues[icol].TimeValue.second != TimeOutput.second))
	{
		sprintf(temp,"Column %d output doesn't match expected: %d and actual: %d (DescribeCol)\n",icol+1,InputOutputValues[icol].TimeValue,TimeOutput); 
		printf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if ((InputOutputValues[icol].TimestampValue.month != TimestampOutput.month) && (InputOutputValues[icol].TimestampValue.day != TimestampOutput.day) && (InputOutputValues[icol].TimestampValue.year != TimestampOutput.year) && (InputOutputValues[icol].TimestampValue.hour != TimestampOutput.hour) && (InputOutputValues[icol].TimestampValue.minute != TimestampOutput.minute) && (InputOutputValues[icol].TimestampValue.second != TimestampOutput.second) && (InputOutputValues[icol].TimestampValue.fraction != TimestampOutput.fraction))
	{
		sprintf(temp,"Column %d output doesn't match expected: %d and actual: %d ,(DescribeCol)\n",icol+1,InputOutputValues[icol].TimestampValue,TimestampOutput); 
		printf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (_strnicmp(InputOutputValues[icol].BigintValue,BigintOutput,strlen(InputOutputValues[icol].BigintValue)) != 0)
	{
		sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s (DescribeCol)\n",icol+1,InputOutputValues[icol].BigintValue,BigintOutput); 
		printf(temp);
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLFreeStmt(pTestInfo->hstmt,SQL_CLOSE);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to freestmt with CLOSE option (FreeStmt)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	free(CharOutput);
	free(VarCharOutput);
	free(DecimalOutput);
	free(NumericOutput);
	free(BigintOutput);

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
BOOL CatalogFunctions(TestInfo *pTestInfo)
{
  RETCODE		returncode;
//	char			temp[300];
	char			*TableType = "TABLE";
	char			*Remark = "";
//  char			*CatalogNameOutput,*SchemaNameOutput,*TableNameOuput,*TableTypeOutput,*RemarkOutput;
//	SDWORD		CatalogNameOutputLen,SchemaNameOutputLen,TableNameOuputLen,TableTypeOutputLen,RemarkOutputLen;


	returncode = SQLTables(pTestInfo->hstmt,(UCHAR *)pTestInfo->Catalog,(SWORD)strlen(pTestInfo->Catalog),(UCHAR *)pTestInfo->Schema,(SWORD)strlen(pTestInfo->Schema),(UCHAR *)pTestInfo->Table,(SWORD)strlen(pTestInfo->Table),(UCHAR *)TableType,(SWORD)strlen(TableType));
	if (returncode != SQL_SUCCESS)
	{
		printf("Catalog API Tables failed (Tables)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode = SQLFreeStmt(pTestInfo->hstmt,SQL_CLOSE);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to freestmt with CLOSE option after Tables API (FreeStmt/Tables)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLColumns(pTestInfo->hstmt,(UCHAR *)pTestInfo->Catalog,(SWORD)strlen(pTestInfo->Catalog),(UCHAR *)pTestInfo->Schema,(SWORD)strlen(pTestInfo->Schema),(UCHAR *)pTestInfo->Table,(SWORD)strlen(pTestInfo->Table),(UCHAR *)ColumnInfo[0].Name,(SWORD)strlen(ColumnInfo[0].Name));
	if (returncode != SQL_SUCCESS)
	{
		printf("Catalog API Columns failed (Tables)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode = SQLFreeStmt(pTestInfo->hstmt,SQL_CLOSE);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to freestmt with CLOSE option after Columns API (FreeStmt/Columns)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLStatistics(pTestInfo->hstmt,(UCHAR *)pTestInfo->Catalog,(SWORD)strlen(pTestInfo->Catalog),(UCHAR *)pTestInfo->Schema,(SWORD)strlen(pTestInfo->Schema),(UCHAR *)pTestInfo->Table,(SWORD)strlen(pTestInfo->Table),SQL_INDEX_UNIQUE,SQL_QUICK);
	if (returncode != SQL_SUCCESS)
	{
		printf("Catalog API Columns failed (Tables)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode = SQLFreeStmt(pTestInfo->hstmt,SQL_CLOSE);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to freestmt with CLOSE option after Statistics API (FreeStmt/Statistics)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLPrimaryKeys(pTestInfo->hstmt,(UCHAR *)pTestInfo->Catalog,(SWORD)strlen(pTestInfo->Catalog),(UCHAR *)pTestInfo->Schema,(SWORD)strlen(pTestInfo->Schema),(UCHAR *)pTestInfo->Table,(SWORD)strlen(pTestInfo->Table));
	if (returncode != SQL_SUCCESS)
	{
		printf("Catalog API Primary Keys failed (Tables)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode = SQLFreeStmt(pTestInfo->hstmt,SQL_CLOSE);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to freestmt with CLOSE option after Primary Keys API (FreeStmt/Primary Keys)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
				
	returncode = SQLSpecialColumns(pTestInfo->hstmt,SQL_ROWVER,(UCHAR *)pTestInfo->Catalog,(SWORD)strlen(pTestInfo->Catalog),(UCHAR *)pTestInfo->Schema,(SWORD)strlen(pTestInfo->Schema),(UCHAR *)pTestInfo->Table,(SWORD)strlen(pTestInfo->Table),SQL_SCOPE_TRANSACTION,SQL_NULLABLE);
	if (returncode != SQL_SUCCESS)
	{
		printf("Catalog API Special Columns Keys failed (Tables)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLFreeStmt(pTestInfo->hstmt,SQL_CLOSE);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to freestmt with CLOSE option after Primary Keys API (FreeStmt/Special Columns)\n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
/*
	CatalogNameOutput = (char *)malloc(SQL_MAX_DSN_LENGTH);
	SchemaNameOutput = (char *)malloc(SQL_MAX_SCHEMA_NAME_LEN);
	TableNameOuput = (char *)malloc(SQL_MAX_TABLE_NAME_LEN);
	TableTypeOutput = (char *)malloc(SQL_MAX_TABLE_TYPE_LEN);
	RemarkOutput = (char *)malloc(SQL_MAX_REMARK_LEN);

	strcpy(CatalogNameOutput,"");
	strcpy(SchemaNameOutput,"");
	strcpy(TableNameOuput,"");
	strcpy(TableTypeOutput,"");
	strcpy(RemarkOutput,"");

	returncode = SQLBindCol(hstmt,1,SQL_C_CHAR,CatalogNameOutput,SQL_MAX_DSN_LENGTH,&CatalogNameOutputLen);
	if (returncode != SQL_SUCCESS)
	{
		printf("Catalog API Tables failed while binding Catalog name", "Tables/BindCol");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode=SQLBindCol(hstmt,2,SQL_C_CHAR,SchemaNameOutput,SQL_MAX_SCHEMA_NAME_LEN,&SchemaNameOutputLen);
	if (returncode != SQL_SUCCESS)
	{
		printf("Catalog API Tables failed while binding Schema name", "Tables/BindCol");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode=SQLBindCol(hstmt,3,SQL_C_CHAR,TableNameOuput,NAME_LEN,&TableNameOuputLen);
	if (returncode != SQL_SUCCESS)
	{
		printf("Catalog API Tables failed while binding Table name", "Tables/BindCol");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode=SQLBindCol(hstmt,4,SQL_C_CHAR,TableTypeOuput,NAME_LEN,&TableTypeOuputLen);
	if (returncode != SQL_SUCCESS)
	{
		printf("Catalog API Tables failed while binding Table Type", "Tables/BindCol");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode=SQLBindCol(hstmt,5,SQL_C_CHAR,RemarkOutput,NAME_LEN,&RemarkOutputLen);
	if (returncode != SQL_SUCCESS)
	{
		printf("Catalog API Tables failed while binding Remark", "Tables/BindCol");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	returncode = SQLFetch(hstmt);
	if (returncode != SQL_SUCCESS)
	{
		printf("Catalog API Tables failed while Fetch", "Tables/Fetch");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (_stricmp(pTestInfo->CatalogName,CatalogNameOutput) != 0)
	{
		sprintf(temp,Catalog Name is not matching expected: %s and actual %s,pTestInfo->CatalogName,CatalogNameOutput);
		printf(temp, "Tables/Fetch");
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (_stricmp(pTestInfo->SchemaName,SchemaNameOutput) != 0) 
	{
		sprintf(temp,"Schema Name is not matching expected: %s and actual %s,pTestInfo->SchemaName,SchemaNameOutput);
		printf(temp, "Tables/Fetch");
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (_stricmp(pTestInfo->TableName,TableNameOutput) != 0)
	{
		sprintf(temp,"Table Name is not matching expected: %s and actual %s,pTestInfo->TableName,TableNameOutput);
		printf(temp, "Tables/Fetch");
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (_stricmp(Table[i].TableType,oTableType) != 0)
	{
		sprintf(temp,"Table Type is not matching expected: %s and actual %s,TableType,TableTypeOutput);
		printf(temp, "Tables/Fetch");
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (_stricmp(Remark,oRemark) != 0)
	{
		sprintf(temp,"Remark is not matching expected: %s and actual %s,Remark,RemarkOutput);
		printf(temp, "Tables/Fetch");
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
*/ 	
	return(TRUE);
}

void OdbcBitProc()
{
	BOOL BitPass;
	int i;

	pTestInfo->henv = (SQLHENV) NULL;
	pTestInfo->hstmt = (SQLHSTMT) NULL;
	pTestInfo->hdbc = (SQLHDBC) NULL;

	for (i=1; i < 20; i++)
	{
		BitPass = FullConnectPromptUser(pTestInfo);
		SQLDisconnect(pTestInfo->hdbc);
//		if(BitPass)
//			BitPass = FullDisconnect(pTestInfo);
	}
	if (BitPass)
#ifdef ONYX
		printf("");
#else
		printf("ODBC BIT has PASSED.\n");
#endif
	else
		printf("ODBC BIT has FAILED.\n");
} 

void OdbcBitInit()
{
	pTestInfo	= new TestInfo;
} 


// Main Program
#ifdef ONYX
#include <time.h>
#include <sys/time.h>
int main (int argc, char **argv)
#else
int main ()
#endif
{
#ifdef ONYX
#define	ARGS	"n:u:p:t:l:"
	int c, errflag = 0;
	clock_t	startclock, endclock;
	struct timeval starttod, endtod;

	optarg = NULL;
	while (!errflag && (c = getopt(argc, argv, ARGS)) != -1)
		switch (c) {
			case 'l':
				logging = 1;
				loglevel = atoi(optarg);
				break;
			case 'n':
				Count = atoi(optarg);
				break;
			case 'u':
				Userid = optarg;
				break;
			case 'p':
				Password = optarg;
				break;
			case 't':
				Table = optarg;
				break;
			default :
				errflag++;
		}
	if (errflag) {
		printf("Command line error.\n");
		printf("Usage: %s [-n <count>] [-u <userid>] [-p <password>] [-t <table>]\n");
		exit(1);
	}
#endif
    OdbcBitInit();
#ifdef ONYX
	gettimeofday(&starttod, NULL);
	startclock = clock();
	loop = Count;
#endif
	OdbcBitProc();
#ifdef ONYX
	endclock = clock();
	gettimeofday(&endtod, NULL);
	printf("Values %d %d %d %d\n",endtod.tv_sec,starttod.tv_sec,endtod.tv_usec,starttod.tv_usec);
	printf("Average Elapsed time used for each iteration = %10.3f secs\n", 
		(double) (((endtod.tv_sec - starttod.tv_sec) * 1000000) +
			(endtod.tv_usec - starttod.tv_usec) )/1000000/Count);
	printf("Average Processor time used for each iteration = %10.2f msecs\n", 
		(double)(endclock - startclock)/1000/Count);
#endif
	return(0);
}


