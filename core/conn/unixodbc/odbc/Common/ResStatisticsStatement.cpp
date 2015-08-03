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
/**************************************************************************
**************************************************************************/

/* MODULE: ResStatisticsStatement.cpp
   PURPOSE: Implements the member functions of ResStatisticsStatement class
*/

 
#include <windows.h>
#include "ResStatisticsStatement.h"
#define SEP " "
#define PREPARE 1
#define EXECUTE 2
#define FETCH 3
#define CLOSE 4
#define EXECDIRECT 5

using namespace SRVR;

int printSequenceNumber = 0;

ResStatisticsStatement::ResStatisticsStatement()
{
 flag = 0;
 memset(tmpString,'\0',4000);
 tmpFlag = FALSE;
 resStatementOn = FALSE;
 sqlOn = FALSE;
 prepareOn = FALSE;
 executeOn = FALSE;
 execdirectOn = FALSE;
 fetchOn = FALSE;
 catFlagOn = FALSE;
 SQLValue = NULL;
 memset(rowsAccessed,'\0',MAX_LENGTH);
 memset(rowsUsed,'\0',MAX_LENGTH);
 memset(discReads,'\0',MAX_LENGTH);
 memset(msgsToDisc,'\0',MAX_LENGTH);
 memset(msgByteToDisc,'\0',MAX_LENGTH);
 memset(lockWaits,'\0',MAX_LENGTH);
 memset(lockEscalation,'\0',MAX_LENGTH); 
 longrowsAccessed = 0;
 longrowsUsed = 0;
 longdiscReads = 0;
 longmsgsToDisc = 0;
 longmsgByteToDisc = 0;
 longlockWaits = 0;
 longlockEscalation = 0;
}


ResStatisticsStatement::~ResStatisticsStatement()
{
  
}

void ResStatisticsStatement::start(char *inState)
{
	short error = 0;
	init(); 
	statementStartTime = JULIANTIMESTAMP();
	char tmpString[32];
  
      
 // get cpu time ( Process_getInfo)
 	statementStartCpuTime = getCpuTime();
 
	strncpy(state,inState,sizeof(state));
	state[sizeof(state)-1]=0;

  // if statement is on
	if (resStatementOn && strcmp(state,"Prepare" ) == 0)
	{
		openCollector(collectorName);
		
	}

    if (resStatementOn && strcmp(state,"Execute" ) == 0)
	{
		
		openCollector(collectorName);
		
	}

    if (resStatementOn && strcmp(state,"ExecDirect" ) == 0)
	{
		openCollector(collectorName);
		
	}

    if (resStatementOn && strcmp(state,"Fetch" ) == 0)
	{
		openCollector(collectorName);
			
	}
	if (resStatementOn && strcmp(state,"Close" ) == 0)
	{
		openCollector(collectorName);
		
	}

 }

void ResStatisticsStatement::end(char *inState,short inStmtType,long inEstimatedCost,char *inSqlStatement,long inErrorStatement,long inWarningStatement,long inRowCount,long inErrorCode)
{
	short error = 0;
   	statementEndTime = JULIANTIMESTAMP();
  
 // get cpu time (Process_getInfo)
 	statementEndCpuTime = getCpuTime();

//Calculate Elapsed and Execution time
	odbcElapseTime = statementEndTime - statementStartTime;
	odbcExecutionTime = statementEndCpuTime - statementStartCpuTime;
 	ps.odbcElapseTime = odbcElapseTime;
	ps.odbcExecutionTime = odbcExecutionTime;

	strncpy(state,inState,sizeof(state));
	state[sizeof(state)-1]=0;
	strncpy(ps.state,inState,sizeof(ps.state));
	ps.state[sizeof(ps.state)-1]=0;
    stmtType = inStmtType;
	ps.stmtType = inStmtType;
	estimatedCost = inEstimatedCost;

	ps.errorStatement = inErrorStatement;
	ps.warningStatement = inWarningStatement;

    numberOfRows = inRowCount;	

	totalStatementOdbcElapseTime = totalStatementOdbcElapseTime + odbcElapseTime;
	totalStatementOdbcExecutionTime = totalStatementOdbcExecutionTime + odbcExecutionTime; 

	errorCode = inErrorCode;
	
	if(strcmp(state,"Execute") == 0 || strcmp(state,"ExecDirect") == 0)
	{
		totalStatementExecutes ++;
	}	

	
 
// if statement is on 
   if (resStatementOn)
   {
	    if(strcmp(state,"Prepare") == 0 && sqlOn)
		{
			markNewOperator,sqlStatement = new char[strlen(inSqlStatement) + 1];
			if (sqlStatement != NULL)
			{
				strcpy(sqlStatement,inSqlStatement);
				int length    = 0;
				char *pBuffer = msgBuffer;
				strcpy(msgAttribute,"STATEMENT:SQLStatement");
				sequenceNumber = 0;
				sprintf(sequenceNumberStr,"%d",sequenceNumber);
				
				pBuffer  = printLongString(pBuffer, sqlStatement,sequenceNumber);

				sprintf(msgInfo," StatementId:%.31s SqlText:%.490s",statementId,msgBuffer);
				if (resStatCollectorError == TRUE)
				{
					openCollector(collectorName);
				}
				sendCollector();
				delete sqlStatement;
				sqlStatement = NULL;
			}
		}

		if(strcmp(state,"Prepare") == 0 && prepareOn)
		{
			markNewOperator,sqlStatement = new char[strlen(inSqlStatement) + 1];
			if (sqlStatement != NULL)
			{
				strcpy(sqlStatement,inSqlStatement);
				strcpy(msgAttribute,"STATEMENT:SQLPrepare");
				sequenceNumber = 0;
				sprintf(sequenceNumberStr,"%d",sequenceNumber);
				char *str;
				if  ((str=strtok(sqlStatement,SEP)) != NULL)
				{
					sprintf(typeOfStatement,"%.10s",str);
				}
				
				sprintf(msgInfo," StatementId:%s EstimatedCost:%d StatementType:%s SQLCompileTime:%d ErrorCode:%d ",statementId,estimatedCost,typeOfStatement,ps.odbcElapseTime,errorCode);
				if (resStatCollectorError == TRUE)
				{
					openCollector(collectorName);
				}
				sendCollector();

				delete sqlStatement;
				sqlStatement = NULL;
			}					
			
		}
		if (strcmp(state,"Execute") == 0 && executeOn)
		{
		    sendExecute();	
		} 
		if (strcmp(state,"Close") == 0 && statStatisticsFlag == TRUE && fetchOn)
		{
			strcpy(msgAttribute,"STATEMENT:SQLFetch/SQLClose");
			sequenceNumber = 0;
			sprintf(sequenceNumberStr,"%d",sequenceNumber);
		    splitString(); 
			if (catFlagOn == FALSE)
			{
			sprintf(msgInfo," StatementId:%s RowsAccessed:%d RowsRetrieved:%d DiscReads:%d MsgsToDisc:%d MsgsBytesToDisc:%d LockWaits:%d LockEscalation:%d TotalOdbcElapsedTime:%d TotalOdbcExecutionTime:%d TotalExecutes:%d "
			,statementId,longrowsAccessed,longrowsUsed,longdiscReads,longmsgsToDisc,longmsgByteToDisc,longlockWaits,longlockEscalation,totalStatementOdbcElapseTime,totalStatementOdbcExecutionTime,totalStatementExecutes);
			
			if (resStatCollectorError == TRUE)
			{
				openCollector(collectorName);
			}
			sendCollector();
			statStatisticsFlag = FALSE;
			}
			else
			{
				catFlagOn = FALSE;
			}
			
		}

		if(strcmp(state,"ExecDirect") == 0 && sqlOn)
		{
			markNewOperator,sqlStatement = new char[strlen(inSqlStatement) + 1];
			if (sqlStatement != NULL)
			{
				strcpy(sqlStatement,inSqlStatement);
				int length    = 0;
				char *pBuffer = msgBuffer;
				strcpy(msgAttribute,"STATEMENT:SQLStatement");
				sequenceNumber = 0;
				sprintf(sequenceNumberStr,"%d",sequenceNumber);
				
				pBuffer  = printLongString(pBuffer, sqlStatement,sequenceNumber);

				sprintf(msgInfo," StatementId:%s SqlText:%s",statementId,msgBuffer);
				if (resStatCollectorError == TRUE)
				{
					openCollector(collectorName);
				}
				sendCollector();
				delete sqlStatement;
				sqlStatement = NULL;
			}
		}

		if (strcmp(state,"ExecDirect") == 0 && execdirectOn)
		{
			markNewOperator,sqlStatement = new char[strlen(inSqlStatement) + 1];
			if (sqlStatement != NULL)
			{
				strcpy(sqlStatement,inSqlStatement);
				strcpy(msgAttribute,"STATEMENT:SQLExecDirect");
				sequenceNumber = 0;
				sprintf(sequenceNumberStr,"%d",sequenceNumber);
				char *str;
				char typeOfStatement[10];
				if  ((str=strtok(sqlStatement,SEP)) != NULL)
				{
				sprintf(typeOfStatement,"%s",str);
				}
				
				if (stmtType == TYPE_INSERT || stmtType == TYPE_DELETE || stmtType == TYPE_UPDATE)
				{
					splitString();
					sprintf(msgInfo," StatementId:%s EstimatedCost:%d StatementType:%s ODBCElapsedTime:%d ODBCExecutionTime:%d NumberOfRows(ins/upd/del):%d ErrorCode:%d RowsAccessed:%d RowsRetrieved:%d DiscReads:%d MsgsToDisc:%d MsgsBytesToDisc:%d LockWaits:%d LockEscalation:%d",statementId,estimatedCost,typeOfStatement,ps.odbcElapseTime,ps.odbcExecutionTime,numberOfRows,errorCode,longrowsAccessed,longrowsUsed,longdiscReads,longmsgsToDisc,longmsgByteToDisc,longlockWaits,longlockEscalation);
				}
				else		   
				{
				sprintf(msgInfo," StatementId:%s EstimatedCost:%d StatementType:%s ODBCElapsedTime:%d ODBCExecutionTime:%d ErrorCode:%d ",statementId,estimatedCost,typeOfStatement,ps.odbcElapseTime,ps.odbcExecutionTime,errorCode);
				}
				if (resStatCollectorError == TRUE)
				{
				 openCollector(collectorName);
				}
				sendCollector();
				delete sqlStatement;
				sqlStatement = NULL;
			}			
		}
			
    } 
	if (resStatementOn)
	{
		closeCollector();
	}

	if (srvrGlobal->resStatSession != NULL)
	{
		srvrGlobal->resStatSession->accumulateStatistics(&ps);
	}

}


void ResStatisticsStatement::setStatistics()
{
 
	unsigned long curColumnNo = 0;
    short	sTmpLength;
	SQLValueList_def *inValueList;
	markNewOperator,inValueList = new SQLValueList_def();
	if (inValueList == NULL)
	{
		strcpy(tmpString, "Error in allocating memory for inValueList");
		srvrEventLogger->SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, srvrGlobal->srvrObjRef, 
			1, tmpString);
		executeOn = FALSE;
		execdirectOn = FALSE;
		fetchOn = FALSE; 
		prepareOn = FALSE;
		sqlOn = FALSE;
		return;
	}
	inValueList->_buffer = NULL;
	inValueList->_length = 0;
	outputValueList._length = 0;
	outputValueList._buffer = NULL;
  
 // If statement is on , Execute the statistics procedure query and fetch the results
 //  and Set the attributes
	 if (resStatementOn && prepareFlag == FALSE)
	 {
		 if ((resSrvrStmt = getSrvrStmt(stmtLabel, FALSE)) == NULL)
		 {
			strcpy(tmpString, "Invalid Statement Handle.");
			srvrEventLogger->SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, srvrGlobal->srvrObjRef, 
			1, tmpString);
			executeOn = FALSE;
			execdirectOn = FALSE;
			fetchOn = FALSE; 
			prepareOn = FALSE;
			sqlOn = FALSE;	 
		 }
		 else
		 {	
			retcode = resSrvrStmt->Execute(NULL,5,TYPE_SELECT,inValueList,SQL_ASYNC_ENABLE_OFF,0);
			switch (retcode)
			{
				case SQL_SUCCESS:
				case SQL_SUCCESS_WITH_INFO:
				retcode = resSrvrStmt->Fetch(maxRowCnt, maxRowLen, SQL_ASYNC_ENABLE_OFF,0);
				switch (retcode)
				{
         			case SQL_SUCCESS:
					case SQL_SUCCESS_WITH_INFO:
						outputValueList._length = resSrvrStmt->outputValueList._length;
	   					outputValueList._buffer = new SQLValue_def[outputValueList._length]; 
						memcpy((void *)(outputValueList._buffer),resSrvrStmt->outputValueList._buffer, 
						(sizeof(SQLValue_def)*outputValueList._length));       
					// Writing fetched data into the output structure
						for (curColumnNo = 0 ; curColumnNo < outputValueList._length ; curColumnNo++)
						{
							SQLValue = (SQLValue_def *)outputValueList._buffer + curColumnNo;
							if (SQLValue->dataInd == -1)
							{
							continue;	
							}
							else
							{
								switch (curColumnNo) {
									case 0:  //Variable_info
									// VARCHAR TO CHAR conversion
									{
										sTmpLength = *(USHORT *)SQLValue->dataValue._buffer;
										memcpy((void *)tmpString, 
										(const void *)(SQLValue->dataValue._buffer+sizeof(USHORT)), 
										sTmpLength);
										tmpString[sTmpLength] = '\0';
										tmpFlag = TRUE;
									}
									break; 
								}
							}	
						}
					delete outputValueList._buffer;
					//outputValueList._buffer = NULL;
					break;
    				case SQL_ERROR:
						 strcpy(tmpString, "Error in fetching data from Statistics Procedure. Statement Statistics Disabled.");
						 srvrEventLogger->SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
						 0, ODBCMX_SERVER, srvrGlobal->srvrObjRef, 
						 1, tmpString);
						 executeOn = FALSE;
						 execdirectOn = FALSE;
						 fetchOn = FALSE; 
						prepareOn = FALSE;
						 sqlOn = FALSE;
					
					break;
					default:
					break;
				}
				break;
				case SQL_ERROR:
		 			strcpy(tmpString, "Error in Executing query for Statistics Procedure. Statement Statistics Disabled.");
					srvrEventLogger->SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
					0, ODBCMX_SERVER, srvrGlobal->srvrObjRef, 
					1, tmpString);
					executeOn = FALSE;
					execdirectOn = FALSE;
					fetchOn = FALSE; 
					prepareOn = FALSE;
					sqlOn = FALSE;
		 
			break;
			}
			resSrvrStmt->InternalStmtClose(SQL_CLOSE);
		 }

	}
			delete inValueList;
			//inValueList->_buffer = NULL;
			//inValueList->_length = 0;

	 
} 



void ResStatisticsStatement::init()
{
 stmtType = 0;
 memset(state,'\0',STATE_LEN);
 //sqlExecutionTime = 0;
 odbcElapseTime = 0;
 //sqlElapseTime = 0;
 odbcExecutionTime = 0;
 estimatedCost = 0;
 maxRowCnt = 1000;
 maxRowLen = 1000;
 retcode = 0;
 prepareFlag = FALSE;
 
 }

void ResStatisticsStatement::prepareQuery( )
{
	//char stmtLabel[MAX_STMT_LABEL_LEN+1];
	strcpy(stmtLabel,"STMT_INTERNAL_STATISTICS");
    resSrvrStmt = getSrvrStmt(stmtLabel,TRUE);
	if (resSrvrStmt == NULL)
	{
		sprintf(tmpString, "%s", "Unable to allocate statement to Statistics."); 
		srvrEventLogger->SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, srvrGlobal->srvrObjRef, 
			2, "STMT_INTERNAL_STATISTICS", tmpString);

		 executeOn = FALSE;
		 execdirectOn = FALSE;
		 fetchOn = FALSE; 
		 prepareOn = FALSE;
		 sqlOn = FALSE;
	}
	else
	{
		strcpy(sqlString, "CONTROL QUERY DEFAULT detailed_statistics 'ACCUMULATED'");
		retcode = resSrvrStmt->ExecDirect(NULL, sqlString, INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0); 
		switch (retcode)
		{
			case SQL_ERROR:
					
					 strcpy(tmpString, "Error in Executing Control Query for Statistics. Statement Statistics Disabled. ");
					 srvrEventLogger->SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
					 0, ODBCMX_SERVER, srvrGlobal->srvrObjRef, 
						 1, tmpString);
					 executeOn = FALSE;
					 execdirectOn = FALSE;
					 fetchOn = FALSE; 
					 prepareOn = FALSE;
					 sqlOn = FALSE;
					 
			break;
			default:
			break;
		}
		strcpy(sqlString, "select variable_info from table(statistics(null,null))");
		retcode = resSrvrStmt->Prepare(sqlString,INTERNAL_STMT,SQL_ASYNC_ENABLE_OFF, 0);
		switch (retcode)
		{
			case SQL_ERROR:
			 
			 strcpy(tmpString, "Error in Preparing Query for Statistics Procedure. Statement Statistics Disabled");
					srvrEventLogger->SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
					0, ODBCMX_SERVER, srvrGlobal->srvrObjRef, 
					1, tmpString);
			 prepareFlag = TRUE;
			 executeOn = FALSE;
			 execdirectOn = FALSE;
			 fetchOn = FALSE; 
			 prepareOn = FALSE;
			 sqlOn = FALSE;
			 
			break;
			default:
			break;
		}
	}
}

char * ResStatisticsStatement::printLongString(char *buffer,char *longStr ,int sequenceNumber )
{
	int length  = 0;
	int size    = 0;
	int strSize = 0;
	char *p;
		
	printSequenceNumber = sequenceNumber;
	
	size    = BUFFERSIZE-(buffer-msgBuffer);
	strSize = strlen(longStr);
	p       = longStr;
	

	if(strSize > size)
	{	
		while (strSize > size)
		{
			strncpy(buffer, p, size);
			
			sprintf(msgInfo," StatementId:%s SqlText:%s",statementId,buffer);
			
			buffer      += size;
			p           += size;
			strSize     -= size;

			sprintf(sequenceNumberStr,"%d",printSequenceNumber);
			if (resStatCollectorError == TRUE)
			{
			  openCollector(collectorName);
			}
			sendCollector();
            
						
			buffer = msgBuffer;
			printSequenceNumber ++;
			memset(sequenceNumberStr,'\0',20);
			sprintf(sequenceNumberStr,"%d",printSequenceNumber);
			size   = BUFFERSIZE;
		}
	}
	length = sprintf(buffer,"%s ", p);
	buffer += length; 
	return buffer;
}

long long ResStatisticsStatement::getCpuTime()
{
	short error;
    long long cpuTime = 0;
	char errorString[32];
	if ((error = PROCESS_GETINFO_(srvrGlobal->nskProcessInfo.pHandle,
			OMITREF, OMITSHORT,OMITREF,		// proc string,max buf len,act len
			OMITREF,						// priority
			OMITREF,						// Mom's proc handle 
			OMITREF, OMITSHORT,OMITREF,		// home term,max buf len,act len  
			&cpuTime,						// Process execution time 
			OMITREF,						// Creator Access Id 
			OMITREF,						// Process Access Id 
			OMITREF,						// Grand Mom's proc handle 
			OMITREF,						// Job Id 
			OMITREF, OMITSHORT,OMITREF,		// Program file,max buf len,act len  
			OMITREF, OMITSHORT,OMITREF,		// Swap file,max buf len,act len 
			OMITREF,
			OMITREF,						// Process type 
			OMITREF) ) != 0)			// OSS or NT process Id
  {
		sprintf(errorString, "%d", error);
		srvrEventLogger->SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, srvrGlobal->srvrObjRef, 
			1, errorString);	
		return;
  }
	return cpuTime;
}

void ResStatisticsStatement::setStatisticsFlag(bool setStatisticsFlag)
{
	statStatisticsFlag = setStatisticsFlag;
}

void ResStatisticsStatement::setStatementId()
{
	memset(statementId,'\0',30);
	totalStatementOdbcElapseTime = 0;
	totalStatementOdbcExecutionTime = 0;
	totalStatementExecutes = 0;
	strcpy(statementId,getJulianTime());
	
}


void ResStatisticsStatement::sendExecute()
{
		strcpy(msgAttribute,"STATEMENT:SQLExecute");
		sequenceNumber = 0;
		sprintf(sequenceNumberStr,"%d",sequenceNumber);
		memset(msgInfo,'\0',BUFFERSIZE);
		if (stmtType == TYPE_INSERT || stmtType == TYPE_DELETE || stmtType == TYPE_UPDATE)
		{
			splitString();
			sprintf(msgInfo," StatementId:%s ODBCElapsedTime:%d ODBCExecutionTime:%d NumberOfRows(ins/upd/del):%d ErrorCode:%d RowsAccessed:%d RowsRetrieved:%d DiscReads:%d MsgsToDisc:%d MsgsBytesToDisc:%d LockWaits:%d LockEscalation:%d",statementId,ps.odbcElapseTime,ps.odbcExecutionTime,numberOfRows,errorCode,longrowsAccessed,longrowsUsed,longdiscReads,longmsgsToDisc,longmsgByteToDisc,longlockWaits,longlockEscalation);
        }
		else
		{
		sprintf(msgInfo," StatementId:%s ODBCElapsedTime:%d ODBCExecutionTime:%d ErrorCode:%d ",statementId,ps.odbcElapseTime,ps.odbcExecutionTime,errorCode);
		}
		if (resStatCollectorError == TRUE)
		{
			openCollector(collectorName);
		}
		sendCollector();
}

void ResStatisticsStatement::setMaxRowCnt(long inMaxRowCnt)
{
    maxRowCnt = inMaxRowCnt;
}

void ResStatisticsStatement::setMaxRowLen(long inMaxRowLen)
{
	maxRowLen = inMaxRowLen;
}

void ResStatisticsStatement::splitString()
{
		char *p;
		if  ((p=strtok(tmpString,SEP)) != NULL)
		{
			sprintf(strmsgsToDisc,"%.30s",p);
					 			          
			if  ((p=strtok(NULL,SEP)) != NULL)
			{
				sprintf(msgsToDisc,"%.1000s",p);
				
			}
			if ((p=strtok(NULL,SEP)) != NULL)
			{
				sprintf(strmsgByteToDisc,"%.30s",p);
				
			}
			if ((p=strtok(NULL,SEP)) != NULL)
			{
				sprintf(msgByteToDisc,"%.1000s",p);
				
			}
			if ((p=strtok(NULL,SEP)) != NULL)
			{
			}
			if ((p=strtok(NULL,SEP)) != NULL)
			{}
			if ((p=strtok(NULL,SEP)) != NULL)
			{
				sprintf(strrowsAccessed,"%.30s",p);
			}
			if ((p=strtok(NULL,SEP)) != NULL)
			{
				sprintf(rowsAccessed,"%.1000s",p);
			}
			if ((p=strtok(NULL,SEP)) != NULL)
			{
				sprintf(strrowsUsed,"%.30s",p);
			}
			if ((p=strtok(NULL,SEP)) != NULL)
			{
				sprintf(rowsUsed,"%.1000s",p);
			}
			if ((p=strtok(NULL,SEP)) != NULL)
			{
				sprintf(strdiscReads,"%.30s",p);
			}
			if ((p=strtok(NULL,SEP)) != NULL)
			{
				sprintf(discReads,"%.1000s",p);
			}
			if ((p=strtok(NULL,SEP)) != NULL)
			{
				sprintf(strlockEscalation,"%.30s",p);
			}
			if ((p=strtok(NULL,SEP)) != NULL)
			{
				sprintf(lockEscalation,"%.1000s",p);
			}
			if ((p=strtok(NULL,SEP)) != NULL)
			{
				sprintf(strlockWaits,"%.30s",p);
			}
			if ((p=strtok(NULL,SEP)) != NULL)
			{
				sprintf(lockWaits,"%.1000s",p);
			}
	        
			longrowsAccessed = atoi(rowsAccessed);
			longrowsUsed = atoi(rowsUsed);
			longdiscReads = atoi(discReads);
			longmsgsToDisc = atoi(msgsToDisc);
			longmsgByteToDisc = atoi(msgByteToDisc);
			longlockWaits = atoi(lockWaits);
			longlockEscalation = atoi(lockEscalation);
			
		
        }

		  
}
