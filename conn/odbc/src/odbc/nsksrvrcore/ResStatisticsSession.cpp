/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
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
		char ts[25];
		getCurrentTime(ts);
		strcpy(startTime, ts);
		strcpy(resCollectinfo.clientId, setinit->clientId);
		strcpy(resCollectinfo.userName, setinit->userName);
		strcpy(resCollectinfo.clientUserName, setinit->clientUserName);
		strcpy(resCollectinfo.applicationId, setinit->applicationId);
		strcpy(resCollectinfo.nodeName, setinit->nodeName);
		strcpy(resCollectinfo.cpuPin, setinit->cpuPin);
		resCollectinfo.startPriority = setinit->startPriority;
		resCollectinfo.currentPriority = setinit->startPriority;
		strcpy(resCollectinfo.DSName, setinit->DSName);
		resCollectinfo.userId = setinit->userId;
		resCollectinfo.totalLoginTime = setinit->totalLoginTime;
		resCollectinfo.ldapLoginTime = setinit->ldapLoginTime;
		resCollectinfo.sqlUserTime = setinit->sqlUserTime;
		resCollectinfo.searchConnectionTime = setinit->searchConnectionTime;
		resCollectinfo.searchTime = setinit->searchTime;
		resCollectinfo.authenticationConnectionTime = setinit->authenticationConnectionTime;
		resCollectinfo.authenticationTime = setinit->authenticationTime;

		//if logon information for session is on write the initial information to the collector
		if (srvrGlobal->resourceStatistics & SESSTAT_LOGINFO)
		{
#ifdef RES_STATS_EVENT
			stringstream ss;
			ss  << "Connection Information: UserName: " << resCollectinfo.userName
				<< ", UserId: " << resCollectinfo.userId
				<< ", ClientId: " << resCollectinfo.clientId
				<< ", ApplicationId: " << resCollectinfo.applicationId
				<< ", ClientUserName: " << resCollectinfo.clientUserName
				<< ", DataSource: " << resCollectinfo.DSName
				<< ", NodeName: " << resCollectinfo.nodeName
				<< ", CpuPin: " << resCollectinfo.cpuPin
				<< ", TotalLoginTime: " << resCollectinfo.totalLoginTime
				<< ", LDAPLoginTime: " << resCollectinfo.ldapLoginTime;
				<< ", SQLUserTime: " << resCollectinfo.sqlUserTime;
				<< ", SearchConnectionTime: " << resCollectinfo.searchConnectionTime;
				<< ", SearchTime: " << resCollectinfo.searchTime;
				<< ", AuthenticationConnectionTime: " << resCollectinfo.authenticationConnectionTime;
				<< ", AuthenticationTime: " << resCollectinfo.authenticationTime;
						
			SendEventMsg(MSG_RES_STAT_INFO,
								EVENTLOG_INFORMATION_TYPE,
								srvrGlobal->nskProcessInfo.processId,
								ODBCMX_SERVER,
								srvrGlobal->srvrObjRef,
								4,
								srvrGlobal->sessionId,
								"STATISTICS INFORMATION",
								"0",
								ss.str().c_str());
#endif
			logonFlag = TRUE;
		}
		stringstream ss;
     	ss << "File: " << __FILE__ << ", Fuction: " << __FUNCTION__ << ", Line: " << __LINE__ << ", SESSIONID: " << srvrGlobal->sessionId;
		string session_status = "START";
		int64 entryTime = JULIANTIMESTAMP();
		int64 startTime = JULIANTIMESTAMP();

		SetSessionStatsInfoStart(
				  (const char *)ss.str().c_str()
				, (const char *)srvrGlobal->sessionId
				, (const char *)session_status.c_str()
				, resCollectinfo.userId
				, (const char *)resCollectinfo.userName
				, (const char *)srvrGlobal->QSRoleName
				, (const char *)resCollectinfo.clientId
				, (const char *)resCollectinfo.clientUserName
				, (const char *)resCollectinfo.applicationId
				, (const char *)resCollectinfo.DSName
				, entryTime
				, resCollectinfo.totalLoginTime
				, resCollectinfo.ldapLoginTime
                                , resCollectinfo.sqlUserTime
                                , resCollectinfo.searchConnectionTime
                                , resCollectinfo.searchTime
                                , resCollectinfo.authenticationConnectionTime
                                , resCollectinfo.authenticationTime
				, startTime
		);
	}
}

void ResStatisticsSession::end()
{
	char ts[25];
	getCurrentTime(ts);
	strcpy(endTime, ts);

	// if summary for session is on
	if (srvrGlobal->resourceStatistics & SESSTAT_SUMMARY)
	{
#ifdef RES_STATS_EVENT
		stringstream ss;
		ss  << "Session Summary: StartTime: " << startTime
			<< ", EndTime: " << endTime
			<< ", StartPriority: " << resCollectinfo.startPriority
			<< ", TotalOdbcExecutionTime: " << totalOdbcExecutionTime
			<< ", TotalODBCElapsedTime: " << totalOdbcElapseTime
			<< ", TotalInsertStmtsExecuted: " << totalInsertStatements
			<< ", TotalDeleteStmtsExecuted: " << totalDeleteStatements
			<< ", TotalUpdateStmtsExecuted: " << totalUpdateStatements
			<< ", TotalSelectStmtsExecuted: " << totalSelectStatements
			<< ", TotalCatalogStmts: " << totalCatalogStatements
			<< ", TotalPrepares: " << totalPrepares
			<< ", TotalExecutes: " << totalExecutes
			<< ", TotalFetches: " << totalFetches
			<< ", TotalCloses: " << totalCloses
			<< ", TotalExecDirects: " << totalExecDirects
			<< ", TotalErrors: " << totalErrors
			<< ", TotalWarnings: " << totalWarnings;
					
		SendEventMsg(MSG_RES_STAT_INFO,
							EVENTLOG_INFORMATION_TYPE,
							srvrGlobal->nskProcessInfo.processId,
							ODBCMX_SERVER,
							srvrGlobal->srvrObjRef,
							4,
							srvrGlobal->sessionId,
							"STATISTICS INFORMATION",
							"0",
							ss.str().c_str());
#endif
	}
	stringstream ss;
 	ss << "File: " << __FILE__ << ", Fuction: " << __FUNCTION__ << ", Line: " << __LINE__ << ", SESSIONID: " << srvrGlobal->sessionId;
	string session_status = "END";
	int64 endTime = JULIANTIMESTAMP();

	SetSessionStatsInfoEnd(
			  (const char *)ss.str().c_str()
			, (const char *)srvrGlobal->sessionId
			, (const char *)session_status.c_str()
			, totalOdbcExecutionTime
			, totalOdbcElapseTime
			, totalInsertStatements
			, totalDeleteStatements
			, totalUpdateStatements
			, totalSelectStatements
			, totalCatalogStatements
			, totalPrepares
			, totalExecutes
			, totalFetches
			, totalCloses
			, totalExecDirects
			, totalErrors
			, totalWarnings
			, endTime
	);
}

void ResStatisticsSession::accumulateStatistics(passSession *ps)
{
	// Compute totals
	//Increment the number of prepares, executes, closes, fetch according to state
	//Increment the type of statement (Insert, delete etc) according to state

	// Check for state and increment accordingly
	switch (ps->state)
	{
	case STMTSTAT_PREPARE:
		totalPrepares++;
		break;
	case STMTSTAT_EXECUTE:
		totalExecutes++;
		break;
	case STMTSTAT_FETCH:
		totalFetches++;
		break;
	case STMTSTAT_CLOSE:
		totalCloses++;
		break;
	case STMTSTAT_EXECDIRECT:
		totalExecDirects++;
		break;
	default:
		break;
	}

	// Check for Statement Type and increment accordingly
	switch (ps->stmtType)
	{
	case TYPE_SELECT:
		totalSelectStatements++;
		break;
	case TYPE_INSERT:
		totalInsertStatements++;
		break;
	case TYPE_DELETE:
		totalDeleteStatements++;
		break;
	case TYPE_UPDATE:
		totalUpdateStatements++;
		break;
	default:
		break;
	}
	totalOdbcElapseTime = totalOdbcElapseTime + ps->odbcElapseTime;
	totalOdbcExecutionTime = totalOdbcExecutionTime + ps->odbcExecutionTime;
	//totalSqlExecutionTime = totalSqlExecutionTime + ps->SqlExecutionTime;
	//totalSqlElapseTime = totalSqlElapsedTime + ps->SqlElapsedTime;
	totalErrors = totalErrors + ps->errorStatement;
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
	totalCatalogStatements = 0;
	totalCatalogErrors = 0;
	totalCatalogWarnings = 0;
	logonFlag = FALSE;
}

ResStatisticsSession::ResStatisticsSession()
{
}

ResStatisticsSession::~ResStatisticsSession()
{
}

