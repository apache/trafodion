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

/* MODULE: ResStatisticsSession.cpp
   PURPOSE: Implements the member functions of ResAccountingSession class
*/

 
#include "ResStatisticsSession.h" 

using namespace SRVR;

void ResStatisticsSession::start(struct collect_info *setinit)
{
   if (setinit != NULL)
   {
	strcpy (startTime, getCurrentTime());
	strcpy(resCollectinfo.clientId,setinit->clientId);                       
    strcpy(resCollectinfo.userName,setinit->userName);
 	strcpy(resCollectinfo.applicationId,setinit->applicationId);
	strcpy(resCollectinfo.nodeName,setinit->nodeName);
	strcpy(resCollectinfo.cpuPin,setinit->cpuPin);
	resCollectinfo.startPriority = setinit->startPriority;
	strcpy(resCollectinfo.DSName,setinit->DSName);
	resCollectinfo.userId = setinit->userId;
	openCollector(collectorName);
	  
   //if logon information for session is on write the initial information to the collector
    if (logonOn && logonFlag == FALSE)
	{
		char componentName[20];
		strcpy(componentName,"ODBCMX_Server");
		strcpy(msgAttribute,"SESSION:ConnectionInformation");
		sequenceNumber = 0;
		sprintf(sequenceNumberStr,"%d",sequenceNumber);
		sprintf(msgInfo," Component:%s UserName:%s UserId:%d ClientId:%s ApplicationId:%s DataSource:%s NodeName:%s CpuPin:%s"
		,componentName,resCollectinfo.userName,resCollectinfo.userId,resCollectinfo.clientId,resCollectinfo.applicationId,resCollectinfo.DSName,resCollectinfo.nodeName,resCollectinfo.cpuPin);
		if (resStatCollectorError == TRUE)
		{
			openCollector(collectorName);
		}
		sendCollector();
		logonFlag = TRUE;
	}
	closeCollector();
   }
   
}

 

void ResStatisticsSession::end()
{
	strcpy (endTime, getCurrentTime());
	openCollector(collectorName);
	
   // if summary for session is on
	if (summaryOn)
	{
		strcpy(msgAttribute,"SESSION:SessionSummary");
		memset(msgInfo,'\0',BUFFERSIZE);
		sprintf(msgInfo," StartTime:%s EndTime:%s StartPriority:%d TotalOdbcExecutionTime:%d TotalODBCElapsedTime:%d TotalInsertStmtsExecuted:%d TotalDeleteStmtsExecuted:%d TotalUpdateStmtsExecuted:%d TotalSelectStmtsExecuted:%d TotalCatalogStmts:%d TotalPrepares:%d TotalExecutes:%d TotalFetches:%d TotalCloses:%d TotalExecDirects:%d TotalErrors:%d TotalWarnings:%d "
		,startTime,endTime,resCollectinfo.startPriority,totalOdbcExecutionTime,totalOdbcElapseTime,totalInsertStatements,totalDeleteStatements,totalUpdateStatements,totalSelectStatements,totalCatalogStatements,totalPrepares,totalExecutes,totalFetches,totalCloses,totalExecDirects,totalErrors,totalWarnings);
		if (resStatCollectorError == TRUE)
		{
			openCollector(collectorName);
		}
		sendCollector();
	 
	}
	closeCollector();
}




void ResStatisticsSession::accumulateStatistics(passSession *ps)
{
  // Compute totals
  //Increment the number of prepares, executes, closes, fetch according to state
  //Increment the type of statement (Insert, delete etc) according to state
    
	// Check for state and increment accordingly
	if (strcmp(ps->state,"Prepare") == 0)
		{
		totalPrepares ++;
		}
	else if(strcmp(ps->state,"Execute") == 0)
		{
		totalExecutes ++;
		}
	else if(strcmp(ps->state,"Fetch") == 0)
		{
		totalFetches ++;
		}
	else if(strcmp(ps->state,"Close") == 0)
		{
		totalCloses ++;
		}
	else if(strcmp(ps->state,"ExecDirect") == 0)
		{
		totalExecDirects ++;
		}

	// Check for Statement Type and increment accordingly
	if (ps->stmtType == TYPE_SELECT)
		{
		totalSelectStatements ++;
		}
	else if (ps->stmtType == TYPE_INSERT)
		{
		totalInsertStatements++;
		}
	else if (ps->stmtType == TYPE_DELETE)
		{
		totalDeleteStatements++;
		}
	else if (ps->stmtType == TYPE_UPDATE)
		{
		totalUpdateStatements++;
		}

	totalOdbcElapseTime = totalOdbcElapseTime + ps->odbcElapseTime;
    totalOdbcExecutionTime = totalOdbcExecutionTime + ps->odbcExecutionTime;
  //totalSqlExecutionTime = totalSqlExecutionTime + ps->SqlExecutionTime;
  //totalSqlElapseTime = totalSqlElapsedTime + ps->SqlElapsedTime;
    totalErrors = totalErrors + ps->errorStatement ; 
	totalWarnings = totalWarnings + ps->warningStatement;

}

void ResStatisticsSession::init()
{

// Initialize the variables to be accumulated
  
  totalSqlExecutionTime = 0;
  totalOdbcElapseTime = 0;
  totalSqlElapseTime = 0;
  totalOdbcExecutionTime = 0;
  totalInsertStatements = 0;
  totalDeleteStatements = 0;
  totalUpdateStatements = 0;
  totalSelectStatements = 0;
  totalPrepares = 0;
  totalExecutes = 0;
  totalFetches = 0;
  totalCloses = 0;
  totalExecDirects = 0;
  totalErrors = 0;
  totalWarnings = 0;
  totalCatalogStatements = 0 ;
  totalCatalogErrors = 0;
  totalCatalogWarnings = 0 ;
  logonFlag = FALSE;
 
}

ResStatisticsSession::ResStatisticsSession()
{
 resSessionOn = FALSE;
 logonOn = FALSE;
 summaryOn = FALSE;
}

ResStatisticsSession::~ResStatisticsSession()
{
 
}
