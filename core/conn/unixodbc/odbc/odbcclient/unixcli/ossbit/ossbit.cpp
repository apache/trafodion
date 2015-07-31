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
#include "sql.h"
#include "ossbit.h"
#include <assert.h>

HANDLE hInst;
TestInfo *pTestInfo;
SQLHWND hWnd;


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
	char*	dsnName = pTestInfo->DataSource;
	char*	password = pTestInfo->Password;
	char*	user = pTestInfo->UserID;

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

char *CreateTable2(TestInfo *pTestInfo)
{
	int i;
	char *pk;		// primary key string
	BOOL YN = FALSE;

	pk = (char *) malloc(300);
	strcpy(pk,"");

	strcpy(SQLStmt,"");
	strcat(SQLStmt,CREATE_TABLE);
	strcat (SQLStmt, pTestInfo->Catalog);
	strcat(SQLStmt, ".");
	strcat(SQLStmt, pTestInfo->Schema);
	strcat(SQLStmt, ".");
	strcat(SQLStmt,pTestInfo->Table);
	strcat(SQLStmt," (");
	i = 0;
	while (strcmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (strcmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0))
			strcat(SQLStmt,",");
		strcat(SQLStmt,ColumnInfo2[i].Name);
		strcat(SQLStmt," ");
		strcat(SQLStmt,ColumnInfo2[i].Description);
		if (strcmp(ColumnInfo2[i].Precision,"") != 0)
		{
			strcat(SQLStmt,"(");
			strcat(SQLStmt,ColumnInfo2[i].Precision);
			if (strcmp(ColumnInfo2[i].Scale,"") != 0)
			{
				strcat(SQLStmt,",");
				strcat(SQLStmt,ColumnInfo2[i].Scale);
			}
			strcat(SQLStmt,")");
		}
		if (ColumnInfo2[i].PriKey)
		{
			strcat(SQLStmt," NOT NULL");
			if (YN)
				strcat(pk,",");
			strcat(pk,ColumnInfo2[i].Name);
  		YN = TRUE;
		}
		else
		{
			strcat(SQLStmt," default ");
			strcat(SQLStmt,ColumnInfo2[i].Default);
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

char *CreateTable3(TestInfo *pTestInfo)
{
	int i;
	char *pk;		// primary key string
	BOOL YN = FALSE;

	pk = (char *) malloc(300);
	strcpy(pk,"");

	strcpy(SQLStmt,"");
	strcat(SQLStmt,CREATE_TABLE);
	strcat (SQLStmt, pTestInfo->Catalog);
	strcat(SQLStmt, ".");
	strcat(SQLStmt, pTestInfo->Schema);
	strcat(SQLStmt, ".");
	strcat(SQLStmt,pTestInfo->Table);
	strcat(SQLStmt," (");
	i = 0;
	while (strcmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (strcmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0))
			strcat(SQLStmt,",");
		strcat(SQLStmt,ColumnInfo3[i].Name);
		strcat(SQLStmt," ");
		strcat(SQLStmt,ColumnInfo3[i].Description);
		if (strcmp(ColumnInfo3[i].Precision,"") != 0)
		{
			strcat(SQLStmt,"(");
			strcat(SQLStmt,ColumnInfo3[i].Precision);
			if (strcmp(ColumnInfo3[i].Scale,"") != 0)
			{
				strcat(SQLStmt,",");
				strcat(SQLStmt,ColumnInfo3[i].Scale);
			}
			strcat(SQLStmt,")");
		}
		strcat(SQLStmt," default ");
		strcat(SQLStmt,ColumnInfo3[i].Default);
		if (ColumnInfo3[i].PriKey)
		{
			strcat(SQLStmt," NOT NULL");
			if (YN)
				strcat(pk,",");
			strcat(pk,ColumnInfo3[i].Name);
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


char *InsertTable2(TestInfo *pTestInfo)
{
	int i;

	strcpy(SQLStmt,"");
	strcat(SQLStmt,INSERT_TABLE);
	strcat(SQLStmt,pTestInfo->Table);
	strcat(SQLStmt," (");
	i = 0;
	while (strcmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (strcmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0))
			strcat(SQLStmt,",");
		strcat(SQLStmt,ColumnInfo2[i].Name);
		i++;
	}
	strcat(SQLStmt,") values (");
	
	i = 0;
	while (strcmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (strcmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0))
			strcat(SQLStmt,",");
		strcat(SQLStmt,"?");
		i++;
	}
	strcat(SQLStmt,")");

	return (SQLStmt);
}

char *InsertTableWithValues2(TestInfo *pTestInfo)
{
	int i;

	strcpy(SQLStmt,"");
	strcat(SQLStmt,INSERT_TABLE);
	strcat(SQLStmt,pTestInfo->Table);
	strcat(SQLStmt," (");
	
	i = 0;
	while (strcmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (strcmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0))
			strcat(SQLStmt,",");
		strcat(SQLStmt,ColumnInfo2[i].Name);
		i++;
	}

	strcat(SQLStmt,") values (");
	
	strcat(SQLStmt,"'");
	strcat(SQLStmt,InputOutputValues2[rowdata].LongVarCharValue);
	strcat(SQLStmt,"'");
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalYearValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalMonthValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalYearToMonthValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalDayValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalHourValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalMinuteValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalSecondValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalDayToHourValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalDayToMinuteValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalDayToSecondValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalHourToMinuteValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalHourToSecondValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalMinuteToSecondValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].DateValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].TimeValue); 
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].TimestampValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,"'");
	strcat(SQLStmt,InputOutputValues2[rowdata].CharValue);
	strcat(SQLStmt,"'");
	strcat(SQLStmt,")");

	return (SQLStmt);
}

char *InsertTableWithDefaults2(TestInfo *pTestInfo)
{
	int i;

	strcpy(SQLStmt,"");
	strcat(SQLStmt,INSERT_TABLE);
	strcat(SQLStmt,pTestInfo->Table);
	strcat(SQLStmt," (");
	
	i = 0;
//	while (strcmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0)
//	{
//		if ((i != 0) && (strcmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0))
//			strcat(SQLStmt,",");
		//strcat(SQLStmt,ColumnInfo2[i].Name);
		strcat(SQLStmt,"COLUMN_CHAR");
//		i++;
//	}

	strcat(SQLStmt,") values (");
	
	//strcat(SQLStmt,"'");
	//strcat(SQLStmt,InputOutputValues2[rowdata].LongVarCharValue);
	//strcat(SQLStmt,"'");
	//strcat(SQLStmt,",");
	strcat(SQLStmt,"'");
	strcat(SQLStmt,InputOutputValues2[rowdata].CharValue);
	strcat(SQLStmt,"'");
/*	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalYearValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalMonthValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalYearToMonthValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalDayValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalHourValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalMinuteValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalSecondValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalDayToHourValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalDayToMinuteValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalDayToSecondValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalHourToMinuteValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalHourToSecondValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].IntervalMinuteToSecondValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].DateValue);
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].TimeValue); 
	strcat(SQLStmt,",");
	strcat(SQLStmt,InputOutputValues2[rowdata].TimestampValue);*/
	strcat(SQLStmt,")");

	return (SQLStmt);
}

char *InsertTable3(TestInfo *pTestInfo)
{
	int i;

	strcpy(SQLStmt,"");
	strcat(SQLStmt,INSERT_TABLE);
	strcat(SQLStmt,pTestInfo->Table);
	strcat(SQLStmt," (");
	i = 0;
	while (strcmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (strcmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0))
			strcat(SQLStmt,",");
		strcat(SQLStmt,ColumnInfo3[i].Name);
		i++;
	}
	strcat(SQLStmt,") values (");
	
	i = 0;
	while (strcmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (strcmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0))
			strcat(SQLStmt,",");
		strcat(SQLStmt,"?");
		i++;
	}
	strcat(SQLStmt,")");

	return (SQLStmt);
}



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
	strcat(SQLStmt," order by 1 ");
	return (SQLStmt);

}


char *SelectTable2(TestInfo *pTestInfo)
{
	int i;

	strcpy(SQLStmt,"");
	strcat(SQLStmt,SELECT_TABLE);
	i = 0;
	while (strcmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (strcmp(ColumnInfo2[i].DataTypestr,END_LOOP) != 0))
			strcat(SQLStmt,",");
		strcat(SQLStmt,ColumnInfo2[i].Name);
		i++;
	}
	strcat(SQLStmt," from ");
	strcat(SQLStmt,pTestInfo->Table);
	strcat(SQLStmt," order by 1 ");
	return (SQLStmt);

}

char *SelectTable3(TestInfo *pTestInfo)
{
	int i;

	strcpy(SQLStmt,"");
	strcat(SQLStmt,SELECT_TABLE);
	i = 0;
	while (strcmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0)
	{
		if ((i != 0) && (strcmp(ColumnInfo3[i].DataTypestr,END_LOOP) != 0))
			strcat(SQLStmt,",");
		strcat(SQLStmt,ColumnInfo3[i].Name);
		i++;
	}
	strcat(SQLStmt," from ");
	strcat(SQLStmt,pTestInfo->Table);
	//strcat(SQLStmt," order by 1 ");
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
	int			pvParam;
	
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
	
	if (pvParam != SQL_MODE_READ_WRITE)
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

	if (pvParam != SQL_TXN_READ_COMMITTED)
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


BOOL InitialSetup2(TestInfo *pTestInfo)
{
  RETCODE returncode;                        
	int			pvParam;
 	
	returncode = SQLSetConnectOption(pTestInfo->hdbc,SQL_ACCESS_MODE,SQL_MODE_READ_WRITE);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to set access mode to read/write");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLGetConnectOption(pTestInfo->hdbc,SQL_ACCESS_MODE,&pvParam);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to get access mode to read/write");
	}
	
	if (pvParam != SQL_MODE_READ_WRITE)
	{
		printf("Invalid access mode value for read/write returned");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLSetConnectOption(pTestInfo->hdbc,SQL_TXN_ISOLATION,SQL_TXN_READ_COMMITTED);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to set transaction isolation mode to read committed");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLGetConnectOption(pTestInfo->hdbc,SQL_TXN_ISOLATION,&pvParam);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to get transaction isolation mode to read committed");
	}

	if (pvParam != SQL_TXN_READ_COMMITTED)
	{
		printf("Invalid access mode value for read/write returned");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	SQLExecDirect(pTestInfo->hstmt,(UCHAR *)DropTable(pTestInfo),SQL_NTS); // Cleanup
	
	returncode = SQLExecDirect(pTestInfo->hstmt,(UCHAR *)CreateTable2(pTestInfo),SQL_NTS);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to Create Table - 2");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	

	return(TRUE);
}
	
BOOL InitialSetup3(TestInfo *pTestInfo)
{
  RETCODE returncode;                        
	int			pvParam;
 	
	returncode = SQLSetConnectOption(pTestInfo->hdbc,SQL_ACCESS_MODE,SQL_MODE_READ_WRITE);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to set access mode to read/write");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLGetConnectOption(pTestInfo->hdbc,SQL_ACCESS_MODE,&pvParam);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to get access mode to read/write");
	}
	
	if (pvParam != SQL_MODE_READ_WRITE)
	{
		printf("Invalid access mode value for read/write returned");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLSetConnectOption(pTestInfo->hdbc,SQL_TXN_ISOLATION,SQL_TXN_READ_COMMITTED);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to set transaction isolation mode to read committed");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLGetConnectOption(pTestInfo->hdbc,SQL_TXN_ISOLATION,&pvParam);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to get transaction isolation mode to read committed");
	}

	if (pvParam != SQL_TXN_READ_COMMITTED)
	{
		printf("Invalid access mode value for read/write returned");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	SQLExecDirect(pTestInfo->hstmt,(UCHAR *)DropTable(pTestInfo),SQL_NTS); // Cleanup
	
	returncode = SQLExecDirect(pTestInfo->hstmt,(UCHAR *)CreateTable3(pTestInfo),SQL_NTS);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to Create Table - 3");
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

	returncode = SQLSetStmtOption(pTestInfo->hstmt,SQL_ASYNC_ENABLE,SQL_ASYNC_ENABLE_OFF);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to set ASYNC mode to ENABLE_OFF (SetStmtOption)\n");
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
	if (returncode != SQL_SUCCESS && returncode != SQL_NO_DATA_FOUND)
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
	SQLULEN 	paramColDef;
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

BOOL StatementFunctions2(TestInfo *pTestInfo)
{
	RETCODE		returncode;                        
	SWORD		numparams;
	char		temp[50];
	int			iparam, num_rows_insert;
	SWORD		paramSQLDataType;
	SQLULEN		paramColDef;
	SWORD		paramColScale,paramColNull;

	num_rows_insert = 0;
	while (strcmp(InputOutputValues2[num_rows_insert].LongVarCharValue,END_LOOP) != 0)
	{
		if (InputOutputValues2[num_rows_insert].UseExecDirect)
		{
			rowdata = num_rows_insert;
			returncode = SQLExecDirect(pTestInfo->hstmt,(UCHAR *)InsertTableWithValues2(pTestInfo),SQL_NTS);
			if (returncode != SQL_SUCCESS)
			{
				if (returncode == SQL_SUCCESS_WITH_INFO)
				{
					printf("This is not an error");
					LogAllErrors(pTestInfo);
				}
				else
				{
					printf("Unable to ExecDirect insert Table");
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
			returncode = SQLExecDirect(pTestInfo->hstmt,(UCHAR *)InsertTableWithDefaults2(pTestInfo),SQL_NTS);
			if (returncode != SQL_SUCCESS)
			{
				if (returncode == SQL_SUCCESS_WITH_INFO)
				{
					printf("This is not an error");
					LogAllErrors(pTestInfo);
				}
				else
				{
					printf("Unable to ExecDirect insert Table");
					LogAllErrors(pTestInfo);		
					ReleaseAll(pTestInfo);
					return(FALSE);
				}
			}
		}
		else
		{
		//Indent here
			returncode = SQLPrepare(pTestInfo->hstmt,(UCHAR *)InsertTable2(pTestInfo),SQL_NTS);
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to prepare insert Table");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}

			returncode = SQLNumParams(pTestInfo->hstmt,&numparams);
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to return number of parameters for insert statement");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			if (numparams != Actual_Num_Columns)
			{
				sprintf(temp,"Number of parameters doesn't match expected: %d and actual: %d",numparams,Actual_Num_Columns); 
				printf("NumParams");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}

			// Begin of bind parameter
			returncode = SQLBindParameter(pTestInfo->hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].LongVarCharValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_LONGVARCHAR during insert statement (BindParam)\n");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_INTERVAL_YEAR, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalYearValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_INTERVAL_YEAR during insert statement (BindParam) \n");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_INTERVAL_MONTH, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalMonthValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_INTERVAL_MONTH during insert statement (BindParam) \n");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_INTERVAL_YEAR_TO_MONTH, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalYearToMonthValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_INTERVAL_YEAR_TO_MONTH during insert statement (BindParam) \n");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 5, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_INTERVAL_DAY, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalDayValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_INTERVAL_DAY during insert statement (BindParam) \n" );
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 6, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_INTERVAL_HOUR, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalHourValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_INTERVAL_HOUR during insert statement (BindParam) \n");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 7, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_INTERVAL_MINUTE, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalMinuteValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_INTERVAL_MINUTE during insert statement (BindParam) \n");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 8, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_INTERVAL_SECOND, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalSecondValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_INTERVAL_SECOND during insert statement (BindParam) \n");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 9, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_INTERVAL_DAY_TO_HOUR, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalDayToHourValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_INTERVAL_DAY_TO_HOUR during insert statement (BindParam) \n");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 10, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_INTERVAL_DAY_TO_MINUTE, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalDayToMinuteValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_INTERVAL_DAY_TO_MINUTE during insert statement (BindParam) \n");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 11, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_INTERVAL_DAY_TO_SECOND, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalDayToSecondValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_INTERVAL_DAY_TO_SECOND during insert statement (BindParam) \n ");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 12, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_INTERVAL_HOUR_TO_MINUTE, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalHourToMinuteValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_INTERVAL_HOUR_TO_MINUTE during insert statement (BindParam) \n");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 13, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_INTERVAL_HOUR_TO_SECOND, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalHourToSecondValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_INTERVAL_HOUR_TO_SECOND during insert statement (BindParam) \n" );
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
 			returncode = SQLBindParameter(pTestInfo->hstmt, 14, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_INTERVAL_MINUTE_TO_SECOND, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].IntervalMinuteToSecondValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_INTERVAL_MINUTE_TO_SECOND during insert statement (BindParam) \n ");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 15, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_TYPE_DATE, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].DateValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_TYPE_DATE during insert statement (BindParam) \n" );
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 16, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_TYPE_TIME, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].TimeValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_TYPE_TIME during insert statement (BindParam) \n");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 17, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_TYPE_TIMESTAMP, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].TimestampValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_TYPE_TIMESTAMP during insert statement (BindParam) \n");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			returncode = SQLBindParameter(pTestInfo->hstmt, 18, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, MAX_SQLSTRING_LEN, 0, InputOutputValues2[num_rows_insert].CharValue, 0, &(InputOutputValues2[num_rows_insert].InValue));
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to convert from SQL_C_CHAR to SQL_CHAR during insert statement - 2 (BindParam) \n");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			// End of bind parameter

			returncode = SQLExecute(pTestInfo->hstmt);
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to execute the insert statement after bind parameter (Execute) \n");
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}

			for(iparam = 1; iparam <= numparams; iparam++)
			{
				returncode = SQLDescribeParam(pTestInfo->hstmt,iparam,&paramSQLDataType,&paramColDef,&paramColScale,&paramColNull);
				if (returncode != SQL_SUCCESS)
				{
					printf("Unable to execute describe parameter after insert (DescribeParam) \n" );
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

BOOL StatementFunctions3(TestInfo *pTestInfo)
{
	RETCODE		returncode;                        
	SWORD		numparams;
	char		temp[50];
	int			iparam, num_rows_insert;
	SWORD		paramSQLDataType;
	SQLULEN		paramColDef;
	SWORD		paramColScale,paramColNull;

	num_rows_insert = 0;
	while (strcmp(InputOutputValues3[num_rows_insert].WCharValue,END_LOOP) != 0)
	{
/*		if (InputOutputValues3[num_rows_insert].UseExecDirect)
		{
			rowdata = num_rows_insert;
			returncode = SQLExecDirect(pTestInfo->hstmt,(UCHAR *)InsertTableWithValues3(pTestInfo),SQL_NTS);
			if (returncode != SQL_SUCCESS)
			{
				MessageBox(NULL,"Unable to ExecDirect insert Table", "ExecuteDirect",MB_OK);
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
		}
		else  
		if (InputOutputValues3[num_rows_insert].UseDefaults)
		{
			rowdata = num_rows_insert;
			returncode = SQLExecDirect(pTestInfo->hstmt,(UCHAR *)InsertTableWithDefaults3(pTestInfo),SQL_NTS);
			if (returncode != SQL_SUCCESS)
			{
				MessageBox(NULL,"Unable to ExecDirect insert Table", "ExecuteDirect",MB_OK);
				LogAllErrors(pTestInfo);		
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
		}
		else */
		{
		//Indent here
		returncode = SQLPrepare(pTestInfo->hstmt,(UCHAR *)InsertTable3(pTestInfo),SQL_NTS);
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to prepare insert Table (Prepare) \n" );
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

		returncode = SQLNumParams(pTestInfo->hstmt,&numparams);
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to return number of parameters for insert statement", "(NumParams)");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		if (numparams != Actual_Num_Columns)
		{
			sprintf(temp,"Number of parameters doesn't match expected: %d and actual: %d",numparams,Actual_Num_Columns); 
			printf(temp);
			printf("NumParams");
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

		// Begin of bind parameter
		returncode = SQLBindParameter(pTestInfo->hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_WCHAR, MAX_SQLSTRING_LEN, 0, InputOutputValues3[num_rows_insert].WCharValue, 0, &(InputOutputValues3[num_rows_insert].InValue));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_CHAR to SQL_WCHAR during insert statement (BindParam) \n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_WVARCHAR, MAX_SQLSTRING_LEN, 0, InputOutputValues3[num_rows_insert].WVarCharValue, 0, &(InputOutputValues3[num_rows_insert].InValue));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_CHAR to SQL_WVARCHAR during insert statement (BindParam) \n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_WLONGVARCHAR, MAX_SQLSTRING_LEN, 0, InputOutputValues3[num_rows_insert].WLongVarCharValue, 0, &(InputOutputValues3[num_rows_insert].InValue));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_CHAR to SQL_WLONGVARCHAR during insert statement (BindParam) \n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 4, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_BIT, 0, 0, &(InputOutputValues3[num_rows_insert].BitValue), 0, &(InputOutputValues3[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_SHORT to SQL_BIT during insert statement (BindParam) \n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 5, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_TINYINT, 0, 0, &(InputOutputValues3[num_rows_insert].TinyintValue), 0, &(InputOutputValues3[num_rows_insert].InValue1));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_SHORT to SQL_TINYINT during insert statement (BindParam) \n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 6, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_BINARY, MAX_SQLSTRING_LEN, 0, InputOutputValues3[num_rows_insert].BinaryValue, 0, &(InputOutputValues3[num_rows_insert].InValue));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_CHAR to SQL_BINARY during insert statement (BindParam) \n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 7, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARBINARY, MAX_SQLSTRING_LEN, 0, InputOutputValues3[num_rows_insert].VarBinaryValue, 0, &(InputOutputValues3[num_rows_insert].InValue));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_CHAR to SQL_VARBINARY during insert statement (BindParam) \n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		returncode = SQLBindParameter(pTestInfo->hstmt, 8, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARBINARY, MAX_SQLSTRING_LEN, 0, InputOutputValues3[num_rows_insert].LongVarBinaryValue, 0, &(InputOutputValues3[num_rows_insert].InValue));
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to convert from SQL_C_CHAR to SQL_LONGVARBINARY during insert statement (BindParam) \n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
	// End of bind parameter

		returncode = SQLExecute(pTestInfo->hstmt);
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to execute the insert statement after bind parameter (Execute) \n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

		for(iparam = 1; iparam <= numparams; iparam++)
		{
			returncode = SQLDescribeParam(pTestInfo->hstmt,iparam,&paramSQLDataType,&paramColDef,&paramColScale,&paramColNull);
			if (returncode != SQL_SUCCESS)
			{
				printf("Unable to execute describe parameter after insert (DescribeParam) \n");
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
	SQLULEN						columnColDef;
	SWORD							columnColScale,columnNull;
	char							temp[50];
	PTR								columnAttribute;
	SWORD							pcDesc;
#if defined(_WIN64) || defined(__LP64__) || defined(__64BIT__)
	SQLINTEGER					pfDesc_4bytes;
#endif
	SQLLEN						pfDesc;


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
	SQLLEN						CharOutputLen;
	SQLLEN						VarCharOutputLen;
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
#if defined(_WIN64) || defined(__LP64__) || defined(__64BIT__)
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_TYPE,columnAttribute,0,&pcDesc,(SQLLEN*)&pfDesc_4bytes);
		pfDesc = pfDesc_4bytes;
#else
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_TYPE,columnAttribute,0,&pcDesc,&pfDesc);
#endif
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




void ConvertIntervalStructToString(SQL_INTERVAL_STRUCT InStruct, SWORD SQLType, char *OutString)
{
	if (InStruct.interval_sign)
		{	
			switch (SQLType)
			{
			case SQL_INTERVAL_YEAR:
				sprintf(OutString, "-%d", InStruct.intval.year_month.year);
				break;
			case SQL_INTERVAL_MONTH:
				sprintf(OutString, "-%d", InStruct.intval.year_month.month);
				break;
			case SQL_INTERVAL_YEAR_TO_MONTH:
				sprintf(OutString, "-%d-%d", InStruct.intval.year_month.year, InStruct.intval.year_month.month);
				break;
			case SQL_INTERVAL_DAY:
				sprintf(OutString, "-%d", InStruct.intval.day_second.day);
				break;
			case SQL_INTERVAL_HOUR:
				sprintf(OutString, "-%d", InStruct.intval.day_second.hour);
				break;
			case SQL_INTERVAL_MINUTE:
				sprintf(OutString, "-%d", InStruct.intval.day_second.minute);
				break;
			case SQL_INTERVAL_SECOND:
				if (InStruct.intval.day_second.fraction == 0)
					sprintf(OutString, "-%d", InStruct.intval.day_second.second);
				else
					sprintf(OutString, "-%d.%d", InStruct.intval.day_second.second, InStruct.intval.day_second.fraction);
				break;
			case SQL_INTERVAL_DAY_TO_HOUR:
				sprintf(OutString, "-%d %d", InStruct.intval.day_second.day, InStruct.intval.day_second.hour);
				break;
			case SQL_INTERVAL_DAY_TO_MINUTE:
				sprintf(OutString, "-%d %d:%d", InStruct.intval.day_second.day, InStruct.intval.day_second.hour, InStruct.intval.day_second.minute);
				break;
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (InStruct.intval.day_second.fraction == 0)
					sprintf(OutString, "-%d %d:%d:%d", InStruct.intval.day_second.day, InStruct.intval.day_second.hour, InStruct.intval.day_second.minute, InStruct.intval.day_second.second);
				else
					sprintf(OutString, "-%d %d:%d:%d.%d", InStruct.intval.day_second.day, InStruct.intval.day_second.hour, InStruct.intval.day_second.minute, InStruct.intval.day_second.second, InStruct.intval.day_second.fraction);
				break;
			case SQL_INTERVAL_HOUR_TO_MINUTE:
				sprintf(OutString, "-%d:%d", InStruct.intval.day_second.hour, InStruct.intval.day_second.minute);
				break;
			case SQL_INTERVAL_HOUR_TO_SECOND:
				if (InStruct.intval.day_second.fraction == 0)
					sprintf(OutString, "-%d:%d:%d", InStruct.intval.day_second.hour, InStruct.intval.day_second.minute, InStruct.intval.day_second.second);
				else
					sprintf(OutString, "-%d:%d:%d.%d", InStruct.intval.day_second.hour, InStruct.intval.day_second.minute, InStruct.intval.day_second.second, InStruct.intval.day_second.fraction);
				break;
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				if (InStruct.intval.day_second.fraction == 0)
					sprintf(OutString, "-%d:%d", InStruct.intval.day_second.minute, InStruct.intval.day_second.second);
				else
					sprintf(OutString, "-%d:%d.%d", InStruct.intval.day_second.minute, InStruct.intval.day_second.second, InStruct.intval.day_second.fraction);
				break;
			}
		}
	else
		{	
			switch (SQLType)
			{
			case SQL_INTERVAL_YEAR:
				sprintf(OutString, "%d", InStruct.intval.year_month.year);
				break;
			case SQL_INTERVAL_MONTH:
				sprintf(OutString, "%d", InStruct.intval.year_month.month);
				break;
			case SQL_INTERVAL_YEAR_TO_MONTH:
				sprintf(OutString, "%d-%d", InStruct.intval.year_month.year, InStruct.intval.year_month.month);
				break;
			case SQL_INTERVAL_DAY:
				sprintf(OutString, "%d", InStruct.intval.day_second.day);
				break;
			case SQL_INTERVAL_HOUR:
				sprintf(OutString, "%d", InStruct.intval.day_second.hour);
				break;
			case SQL_INTERVAL_MINUTE:
				sprintf(OutString, "%d", InStruct.intval.day_second.minute);
				break;
			case SQL_INTERVAL_SECOND:
				if (InStruct.intval.day_second.fraction == 0)
					sprintf(OutString, "%d", InStruct.intval.day_second.second);
				else
					sprintf(OutString, "%d.%d", InStruct.intval.day_second.second, InStruct.intval.day_second.fraction);
				break;
			case SQL_INTERVAL_DAY_TO_HOUR:
				sprintf(OutString, "%d %d", InStruct.intval.day_second.day, InStruct.intval.day_second.hour);
				break;
			case SQL_INTERVAL_DAY_TO_MINUTE:
				sprintf(OutString, "%d %d:%d", InStruct.intval.day_second.day, InStruct.intval.day_second.hour, InStruct.intval.day_second.minute);
				break;
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (InStruct.intval.day_second.fraction == 0)
					sprintf(OutString, "%d %d:%d:%d", InStruct.intval.day_second.day, InStruct.intval.day_second.hour, InStruct.intval.day_second.minute, InStruct.intval.day_second.second);
				else
					sprintf(OutString, "%d %d:%d:%d.%d", InStruct.intval.day_second.day, InStruct.intval.day_second.hour, InStruct.intval.day_second.minute, InStruct.intval.day_second.second, InStruct.intval.day_second.fraction);
				break;
			case SQL_INTERVAL_HOUR_TO_MINUTE:
				sprintf(OutString, "%d:%d", InStruct.intval.day_second.hour, InStruct.intval.day_second.minute);
				break;
			case SQL_INTERVAL_HOUR_TO_SECOND:
				if (InStruct.intval.day_second.fraction == 0)
					sprintf(OutString, "%d:%d:%d", InStruct.intval.day_second.hour, InStruct.intval.day_second.minute, InStruct.intval.day_second.second);
				else
					sprintf(OutString, "%d:%d:%d.%d", InStruct.intval.day_second.hour, InStruct.intval.day_second.minute, InStruct.intval.day_second.second, InStruct.intval.day_second.fraction);
				break;
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				if (InStruct.intval.day_second.fraction == 0)
					sprintf(OutString, "%d:%d", InStruct.intval.day_second.minute, InStruct.intval.day_second.second);
				else
					sprintf(OutString, "%d:%d.%d", InStruct.intval.day_second.minute, InStruct.intval.day_second.second, InStruct.intval.day_second.fraction);
				break;
			}
		}
	return;
}

BOOL ResultFunctions2(TestInfo *pTestInfo)
{
	RETCODE						returncode;                        
	SWORD						numcols;
	int							icol;
	int							irow;
	UCHAR						columnName[SQL_MAX_COLUMN_NAME_LEN];
	SWORD						columnLength, columnSQLDataType;
	SQLULEN						columnColDef;
	SWORD						columnColScale,columnNull;
	char						temp[50];
	PTR							columnAttribute;
	SWORD						pcDesc;
#if defined(_WIN64) || defined(__LP64__) || defined(__64BIT__)
	SQLINTEGER					pfDesc_4bytes;
#endif
	SDWORD						pfDesc;
	char						*LongVarCharOutput;
	char						*CharOutput;
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

	char						*IntervalOutputString;

	SQLLEN						LongVarCharOutputLen;
	SQLLEN						IntervalOutputLen;
	SQLLEN						DateOutputLen;
	SQLLEN						TimeOutputLen;
	SQLLEN						TimestampOutputLen;
	SQLLEN						CharOutputLen;

	CharOutput = (char *)malloc(MAX_COLUMN_OUTPUT);
	LongVarCharOutput =		(char *)malloc(MAX_COLUMN_OUTPUT);
	IntervalOutputString =	(char *)malloc(100);

	returncode = SQLExecDirect(pTestInfo->hstmt,(UCHAR *)SelectTable2(pTestInfo),SQL_NTS);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to Drop Table (ExecDirect) \n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLNumResultCols(pTestInfo->hstmt,&numcols);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to return number of parameters for insert statement (NumParams) \n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (numcols != Actual_Num_Columns)
	{
		sprintf(temp,"Number of columns doesn't match expected: %d and actual: %d",numcols,Actual_Num_Columns); 
		printf(temp);
		printf(" NumCols \n");
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	for (icol = 0; icol < numcols; icol++)
	{
		returncode = SQLDescribeCol(pTestInfo->hstmt,icol+1,columnName,SQL_MAX_COLUMN_NAME_LEN,&columnLength,&columnSQLDataType,&columnColDef,&columnColScale,&columnNull);
		if (returncode != SQL_SUCCESS)
		{
			sprintf(temp,"Unable to describe column %d after select statement",icol+1); 
			printf(temp);
			printf(" DescribeCol \n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

		columnAttribute = (char *)malloc(SQL_MAX_COLUMN_NAME_LEN);
#if defined(_WIN64) || defined(__LP64__) || defined(__64BIT__)
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_NAME,columnAttribute,SQL_MAX_COLUMN_NAME_LEN,&pcDesc,(SQLLEN*)&pfDesc);
#else
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_NAME,columnAttribute,SQL_MAX_COLUMN_NAME_LEN,&pcDesc,&pfDesc);
#endif
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to get column attribute name after select statement (ColumnAttribute) \n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		if (_strnicmp(ColumnInfo2[icol].Name,(char *)columnAttribute,pcDesc) != 0)
		{
			sprintf(temp,"Column %d doesn't match column name expected: %s and actual: %s",icol+1,ColumnInfo2[icol].Name,columnAttribute); 
			printf(temp);
			printf(" DescribeCol \n");
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

#if defined(_WIN64) || defined(__LP64__) || defined(__64BIT__)
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_TYPE,columnAttribute,0,&pcDesc,(SQLLEN*)&pfDesc_4bytes);
		pfDesc = pfDesc_4bytes;
#else
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_TYPE,columnAttribute,0,&pcDesc,(SQLLEN*)&pfDesc);
#endif
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to get column attribute type after select statement (ColumnAttribute) \n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

		free(columnAttribute); 
	
		switch (ColumnInfo2[icol].DataType)
		{
			case SQL_LONGVARCHAR:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,LongVarCharOutput,MAX_COLUMN_OUTPUT,&LongVarCharOutputLen);
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
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo2[icol].CDataType,CharOutput,MAX_COLUMN_OUTPUT,&CharOutputLen);
				break;
		}
		if (returncode != SQL_SUCCESS)
		{
			sprintf(temp,"Unable to bind column %d after select statement",icol); 
			printf(temp);
			printf(" BindCol \n");
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
			printf("Unable to fetch after bind column (Fetch) \n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		else if (returncode == SQL_SUCCESS)
		{
			icol = 1;
			if (_strnicmp(InputOutputValues2[irow].LongVarCharValue,LongVarCharOutput,strlen(InputOutputValues2[irow].LongVarCharValue)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,InputOutputValues2[irow].LongVarCharValue,LongVarCharOutput); 
				printf(temp);
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputYear, SQL_INTERVAL_YEAR, IntervalOutputString);
			if (_strnicmp(IntervalValues[irow].Year,IntervalOutputString,strlen(IntervalValues[irow].Year)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,IntervalValues[irow].Year,IntervalOutputString); 
				printf(temp);
				printf(" Output Value Compare  \n ");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputMonth, SQL_INTERVAL_MONTH, IntervalOutputString);
			if (_strnicmp(IntervalValues[irow].Month,IntervalOutputString,strlen(IntervalValues[irow].Month)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,IntervalValues[irow].Month,IntervalOutputString); 
				printf(temp);
				printf("  Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputYearToMonth, SQL_INTERVAL_YEAR_TO_MONTH, IntervalOutputString);
			if (_strnicmp(IntervalValues[irow].YearToMonth,IntervalOutputString,strlen(IntervalValues[irow].YearToMonth)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,IntervalValues[irow].YearToMonth,IntervalOutputString); 
				printf(temp);
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputDay, SQL_INTERVAL_DAY, IntervalOutputString);
			if (_strnicmp(IntervalValues[irow].Day,IntervalOutputString,strlen(IntervalValues[irow].Day)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,IntervalValues[irow].Day,IntervalOutputString); 
				printf(temp );
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputHour, SQL_INTERVAL_HOUR, IntervalOutputString);
			if (_strnicmp(IntervalValues[irow].Hour,IntervalOutputString,strlen(IntervalValues[irow].Hour)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,IntervalValues[irow].Hour,IntervalOutputString); 
				printf(temp);
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputMinute, SQL_INTERVAL_MINUTE, IntervalOutputString);
			if (_strnicmp(IntervalValues[irow].Minute,IntervalOutputString,strlen(IntervalValues[irow].Minute)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,IntervalValues[irow].Minute,IntervalOutputString); 
				printf(temp );
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputSecond, SQL_INTERVAL_SECOND, IntervalOutputString);
			if (_strnicmp(IntervalValues[irow].Second,IntervalOutputString,strlen(IntervalValues[irow].Second)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,IntervalValues[irow].Second,IntervalOutputString); 
				printf(temp);
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputDayToHour, SQL_INTERVAL_DAY_TO_HOUR, IntervalOutputString);
			if (_strnicmp(IntervalValues[irow].DayToHour,IntervalOutputString,strlen(IntervalValues[irow].DayToHour)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,IntervalValues[irow].DayToHour,IntervalOutputString); 
				printf(temp );
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputDayToMinute, SQL_INTERVAL_DAY_TO_MINUTE, IntervalOutputString);
			if (_strnicmp(IntervalValues[irow].DayToMinute,IntervalOutputString,strlen(IntervalValues[irow].DayToMinute)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,IntervalValues[irow].DayToMinute,IntervalOutputString); 
				printf(temp);
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputDayToSecond, SQL_INTERVAL_DAY_TO_SECOND, IntervalOutputString);
			if (_strnicmp(IntervalValues[irow].DayToSecond,IntervalOutputString,strlen(IntervalValues[irow].DayToSecond)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,IntervalValues[irow].DayToSecond,IntervalOutputString); 
				printf(temp);
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputHourToMinute, SQL_INTERVAL_HOUR_TO_MINUTE, IntervalOutputString);
			if (_strnicmp(IntervalValues[irow].HourToMinute,IntervalOutputString,strlen(IntervalValues[irow].HourToMinute)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,IntervalValues[irow].HourToMinute,IntervalOutputString); 
				printf(temp );
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputHourToSecond, SQL_INTERVAL_HOUR_TO_SECOND, IntervalOutputString);
			if (_strnicmp(IntervalValues[irow].HourToSecond,IntervalOutputString,strlen(IntervalValues[irow].HourToSecond)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,IntervalValues[irow].HourToSecond,IntervalOutputString); 
				printf(temp);
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			ConvertIntervalStructToString (IntervalOutputMinuteToSecond, SQL_INTERVAL_MINUTE_TO_SECOND, IntervalOutputString);
			if (_strnicmp(IntervalValues[irow].MinuteToSecond,IntervalOutputString,strlen(IntervalValues[irow].MinuteToSecond)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,IntervalValues[irow].MinuteToSecond,IntervalOutputString); 
				printf(temp );
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if ((IntervalValues[irow].DateValue.month != DateOutput.month) && (IntervalValues[irow].DateValue.day != DateOutput.day) && (IntervalValues[irow].DateValue.year != DateOutput.year))
			{
				sprintf(temp,"Column %d output doesn't match expected: %d and actual: %d",icol,IntervalValues[irow].DateValue,DateOutput); 
				printf(temp );
				printf(" Output Value Compare \n" ); 
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if ((IntervalValues[irow].TimeValue.hour != TimeOutput.hour) && (IntervalValues[irow].TimeValue.minute != TimeOutput.minute) && (IntervalValues[irow].TimeValue.second != TimeOutput.second))
			{
				sprintf(temp,"Column %d output doesn't match expected: %d:%d:%d and actual: %d:%d:%d",icol,IntervalValues[irow].TimeValue.hour,IntervalValues[irow].TimeValue.minute,IntervalValues[irow].TimeValue.second,TimeOutput.hour,TimeOutput.minute,TimeOutput.second); 
				printf(temp);
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if ((IntervalValues[irow].TimestampValue.month != TimestampOutput.month) && (IntervalValues[irow].TimestampValue.day != TimestampOutput.day) && (IntervalValues[irow].TimestampValue.year != TimestampOutput.year) && (IntervalValues[irow].TimestampValue.hour != TimestampOutput.hour) && (IntervalValues[irow].TimestampValue.minute != TimestampOutput.minute) && (IntervalValues[irow].TimestampValue.second != TimestampOutput.second) && (IntervalValues[irow].TimestampValue.fraction != TimestampOutput.fraction))
			{
				sprintf(temp,"Column %d output doesn't match expected: %d and actual: %d",icol,IntervalValues[irow].TimestampValue,TimestampOutput); 
				printf(temp );
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_strnicmp(InputOutputValues2[irow].CharValue,CharOutput,strlen(InputOutputValues2[irow].CharValue)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,InputOutputValues2[irow].CharValue,CharOutput); 
				printf(temp);
				printf(" Output Value Compare \n");
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
		printf("Unable to freestmt with CLOSE option (FreeStmt) \n");
//		MessageBox(NULL,"Unable to freehandle with CLOSE option", "FreeHandle",MB_OK);
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	free(LongVarCharOutput);
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
BOOL ResultFunctions3(TestInfo *pTestInfo)
{
  RETCODE						returncode;                        
	SWORD						numcols;
	int							icol;
	int							irow;
	UCHAR						columnName[SQL_MAX_COLUMN_NAME_LEN];
	SWORD						columnLength, columnSQLDataType;
	SQLULEN						columnColDef;
	SWORD						columnColScale,columnNull;
	char						temp[50];
	PTR							columnAttribute;
	SWORD						pcDesc;
#if defined(_WIN64) || defined(__LP64__) || defined(__64BIT__)
	SQLINTEGER					pfDesc_4bytes;
#endif
	SDWORD						pfDesc;
	char						*WCharOutput;
	char						*WVarCharOutput;
	char						*WLongVarCharOutput;
	SWORD						BitOutput;
	SWORD						TinyintOutput;
	char						*BinaryOutput;
	char						*VarBinaryOutput;
	char						*LongVarBinaryOutput;
	SQLLEN						WCharOutputLen;
	SQLLEN						WVarCharOutputLen;
	SQLLEN						WLongVarCharOutputLen;
	SQLLEN						BitOutputLen;
	SQLLEN						TinyintOutputLen;
	SQLLEN						BinaryOutputLen;
	SQLLEN						VarBinaryOutputLen;
	SQLLEN						LongVarBinaryOutputLen;

	WCharOutput = (char *)malloc(MAX_COLUMN_OUTPUT);
	WVarCharOutput = (char *)malloc(MAX_COLUMN_OUTPUT);
	WLongVarCharOutput = (char *)malloc(MAX_COLUMN_OUTPUT);
	BinaryOutput = (char *)malloc(MAX_COLUMN_OUTPUT);
	VarBinaryOutput = (char *)malloc(MAX_COLUMN_OUTPUT);
	LongVarBinaryOutput = (char *)malloc(MAX_COLUMN_OUTPUT);

	returncode = SQLExecDirect(pTestInfo->hstmt,(UCHAR *)SelectTable3(pTestInfo),SQL_NTS);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to Drop Table (ExecDirect) \n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	returncode = SQLNumResultCols(pTestInfo->hstmt,&numcols);
	if (returncode != SQL_SUCCESS)
	{
		printf("Unable to return number of parameters for insert statement (NumParams) \n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	if (numcols != Actual_Num_Columns)
	{
		sprintf(temp,"Number of columns doesn't match expected: %d and actual: %d",numcols,Actual_Num_Columns); 
		printf(temp);
		printf(" NumCols");
		ReleaseAll(pTestInfo);
		return(FALSE);
	}
	for (icol = 0; icol < numcols; icol++)
	{
		returncode = SQLDescribeCol(pTestInfo->hstmt,icol+1,columnName,SQL_MAX_COLUMN_NAME_LEN,&columnLength,&columnSQLDataType,&columnColDef,&columnColScale,&columnNull);
		if (returncode != SQL_SUCCESS)
		{
			sprintf(temp,"Unable to describe column %d after select statement",icol+1); 
			printf(temp );
			printf("  DescribeCol");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}

		columnAttribute = (char *)malloc(SQL_MAX_COLUMN_NAME_LEN);
#if defined(_WIN64) || defined(__LP64__) || defined(__64BIT__)
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_NAME,columnAttribute,SQL_MAX_COLUMN_NAME_LEN,&pcDesc,(SQLLEN*)&pfDesc);
#else
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_NAME,columnAttribute,SQL_MAX_COLUMN_NAME_LEN,&pcDesc,&pfDesc);
#endif
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to get column attribute name after select statement (ColumnAttribute) \n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		if (_strnicmp(ColumnInfo[icol].Name,(char *)columnAttribute,pcDesc) != 0)
		{
			sprintf(temp,"Column %d doesn't match column name expected: %s and actual: %s",icol+1,ColumnInfo[icol].Name,columnAttribute); 
			printf(temp );
			printf(" DescribeCol \n");
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
#if defined(_WIN64) || defined(__LP64__) || defined(__64BIT__)
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_TYPE,columnAttribute,0,&pcDesc,(SQLLEN*)&pfDesc_4bytes);
		pfDesc = pfDesc_4bytes;
#else
		returncode = SQLColAttributes(pTestInfo->hstmt,icol+1,SQL_COLUMN_TYPE,columnAttribute,0,&pcDesc,&pfDesc);
#endif
		if (returncode != SQL_SUCCESS)
		{
			printf("Unable to get column attribute type after select statement (ColumnAttribute) \n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		free(columnAttribute); 
	
		switch (ColumnInfo[icol].DataType)
		{
			case SQL_WCHAR:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,WCharOutput,MAX_COLUMN_OUTPUT,&WCharOutputLen);
				break;
			case SQL_WVARCHAR:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,WVarCharOutput,MAX_COLUMN_OUTPUT,&WVarCharOutputLen);
				break;
			case SQL_WLONGVARCHAR:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,WLongVarCharOutput,MAX_COLUMN_OUTPUT,&WLongVarCharOutputLen);
				break;
			case SQL_BIT:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,&BitOutput,MAX_COLUMN_OUTPUT,&BitOutputLen);
				break;
			case SQL_TINYINT:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,&TinyintOutput,0,&TinyintOutputLen);
				break;
			case SQL_BINARY:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,BinaryOutput,0,&BinaryOutputLen);
				break;
			case SQL_VARBINARY:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,VarBinaryOutput,0,&VarBinaryOutputLen);
				break;
			case SQL_LONGVARBINARY:
				returncode = SQLBindCol(pTestInfo->hstmt,icol+1,ColumnInfo[icol].CDataType,LongVarBinaryOutput,0,&LongVarBinaryOutputLen);
				break;
		}
		if (returncode != SQL_SUCCESS)
		{
			sprintf(temp,"Unable to bind column %d after select statement",icol); 
			printf(temp );
			printf("  BindCol\n");
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
			printf("Unable to fetch after bind column (Fetch) \n");
			LogAllErrors(pTestInfo);		
			ReleaseAll(pTestInfo);
			return(FALSE);
		}
		else if (returncode == SQL_SUCCESS)
		{
			icol = 1;
			if (_strnicmp(InputOutputValues3[irow].WCharValue,WCharOutput,strlen(InputOutputValues3[irow].WCharValue)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,InputOutputValues3[irow].WCharValue,WCharOutput); 
				printf(temp);
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_strnicmp(InputOutputValues3[irow].WVarCharValue,WVarCharOutput,strlen(InputOutputValues3[irow].WVarCharValue)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,InputOutputValues3[irow].WVarCharValue,WVarCharOutput); 
				printf(temp);
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_strnicmp(InputOutputValues3[irow].WLongVarCharValue,WLongVarCharOutput,strlen(InputOutputValues3[irow].WLongVarCharValue)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,InputOutputValues3[irow].WLongVarCharValue,WLongVarCharOutput); 
				printf(temp);
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (InputOutputValues3[irow].BitValue != BitOutput)
			{
				sprintf(temp,"Column %d output doesn't match expected: %d and actual: %d",icol,InputOutputValues3[irow].BitValue,BitOutput); 
				printf(temp);
				printf("  Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (InputOutputValues3[irow].TinyintValue != TinyintOutput)
			{
				sprintf(temp,"Column %d output doesn't match expected: %d and actual: %d",icol,InputOutputValues3[irow].TinyintValue,TinyintOutput); 
				printf(temp);
				printf(" Output Value Compare\n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_strnicmp(InputOutputValues3[irow].BinaryValue,BinaryOutput,strlen(InputOutputValues3[irow].BinaryValue)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,InputOutputValues3[irow].BinaryValue,BinaryOutput); 
				printf(temp);
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_strnicmp(InputOutputValues3[irow].VarBinaryValue,VarBinaryOutput,strlen(InputOutputValues3[irow].VarBinaryValue)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,InputOutputValues3[irow].VarBinaryValue,VarBinaryOutput); 
				printf(temp );
				printf(" Output Value Compare \n");
				ReleaseAll(pTestInfo);
				return(FALSE);
			}
			icol++;
			if (_strnicmp(InputOutputValues3[irow].LongVarBinaryValue,LongVarBinaryOutput,strlen(InputOutputValues3[irow].LongVarBinaryValue)) != 0)
			{
				sprintf(temp,"Column %d output doesn't match expected: %s and actual: %s",icol,InputOutputValues3[irow].LongVarBinaryValue,LongVarBinaryOutput); 
				printf(temp);
				printf("  Output Value Compare \n");
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
		printf("Unable to freestmt with CLOSE option (Freestmt) \n");
		LogAllErrors(pTestInfo);		
		ReleaseAll(pTestInfo);
		return(FALSE);
	}

	free(WCharOutput);
	free(WVarCharOutput);
	free(WLongVarCharOutput);
	free(BinaryOutput);
	free(VarBinaryOutput);
	free(LongVarBinaryOutput);

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

int OdbcBitProc()
{
	BOOL BitPass;

	pTestInfo->henv = (SQLHENV) NULL;
	pTestInfo->hstmt = (SQLHSTMT) NULL;
	pTestInfo->hdbc = (SQLHDBC) NULL;


	BitPass = FullConnectPromptUser(pTestInfo);
	if(BitPass)
		BitPass = InitialSetup(pTestInfo);
	if(BitPass)
		BitPass = GetAllInfo(pTestInfo);
	if(BitPass)
		BitPass = StatementFunctions(pTestInfo);	
	if(BitPass)
		BitPass = ResultFunctions(pTestInfo);
	if(BitPass)
		BitPass = CatalogFunctions(pTestInfo);
	if(BitPass)
		BitPass = Cleanup(pTestInfo);


//New code from 3.0 version - Arvind
// Adding a bunch of new procedures to test new datatypes support.
// LongvarChar, Interval data types, Variable input formats for data, time & ts.

			strcpy((char *)pTestInfo->Table,"ODBCTAB2");

			if(BitPass)
				BitPass = InitialSetup2(pTestInfo);
			if(BitPass)
				BitPass = StatementFunctions2(pTestInfo);	
			if(BitPass)
				BitPass = ResultFunctions2(pTestInfo);	
			if(BitPass)
				BitPass = Cleanup(pTestInfo);

// These new procedures will test new datatypes support for R2.
// WCHAR, VARWCHAR, LONGWVARCHAR, BIT, TINYINT, BINARY, VARBINARY(n), LONG VARBINARY
/*		
			strcpy((char *)pTestInfo->Table,"ODBCTAB3");

			if(BitPass)
				BitPass = InitialSetup3(pTestInfo);
			if(BitPass)
				BitPass = StatementFunctions3(pTestInfo);	
			if(BitPass)
				BitPass = ResultFunctions3(pTestInfo);	
			if(BitPass)
				BitPass = Cleanup(pTestInfo);
*/
//New code from 3.0 version - Arvind

	if(BitPass)
		BitPass = FullDisconnect(pTestInfo);
	if (BitPass)
	{
		printf("ODBC BIT has PASSED.\n");
		return (0);
	}
	else
	{
		printf("ODBC BIT has FAILED.\n");
		return (1);
		
	}
} 

void OdbcBitInit(char DSN[],char UID[],char PWD[])
{
	pTestInfo	= new TestInfo;
	strcpy(pTestInfo->DataSource,DSN);
	strcpy(pTestInfo->UserID,UID);
	strcpy(pTestInfo->Password,PWD);
} 


// Main Program
#include<stdio.h>
#include<string.h>
void OdbcBitInit(char DSN[],char UID[],char PWD[]) ;
int main ( int argc, char *argv[] )
{
int retval;
if( argc == 4 )
	{
		strcpy(DSN, argv[1]);
		strcpy(UID, argv[2]);
		strcpy(PWD, argv[3]);
		OdbcBitInit(DSN,UID,PWD);
		retval = OdbcBitProc();
		//OdbcBitInit() ;
	}
else if( argc > 4 )
			printf("Too many arguments supplied.Arguments expected in this form: ossbit  DSN UserName Password \n");
else
{	
			printf("One or more arguments expected in this form: ossbit  DSN UserName Password .\n");
			retval=2;
}
		
		exit(retval);
}


