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
#include "ResStatisticsStatement.h"

// T2_REPO
#include "SrvrCommon.h"
//

void sendAggrStats(pub_struct_type pub_type, std::tr1::shared_ptr<SESSION_AGGREGATION> pAggr_info);
void sendSessionEnd(std::tr1::shared_ptr<SESSION_END> pSession_info);

//using namespace SRVR; +++ T2_REPO
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

/* +++ T2_REPO - Don't need this
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
*/

// +++ T2_REPO
//      stringstream ss;
//      ss << "File: " << __FILE__ << ", Fuction: " << __FUNCTION__ << ", Line: " << __LINE__ << ", SESSIONID: " << srvrGlobal->sessionId;
//
        string session_status = "START";
        int64 entryTime = JULIANTIMESTAMP();
        int64 startTime = JULIANTIMESTAMP();

        startTime_ts = startTime;
        entryTime_ts = entryTime;
        if( srvrGlobal->m_bStatisticsEnabled && srvrGlobal->m_statisticsPubType == STATISTICS_AGGREGATED )
        {
            std::tr1::shared_ptr<SESSION_AGGREGATION> pAggr_info = getAggrStats();
            sendAggrStats(PUB_TYPE_SESSION_START_AGGREGATION, pAggr_info);
        }
    }
}

void ResStatisticsSession::end()
{
    char ts[25];
    getCurrentTime(ts);
    strcpy(endTime, ts);

// +++ T2_REPO
/*
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
*/

// +++ T2_REPO
//  stringstream ss;
//  ss << "File: " << __FILE__ << ", Fuction: " << __FUNCTION__ << ", Line: " << __LINE__ << ", SESSIONID: " << srvrGlobal->sessionId;
//
    string session_status = "END";
    int64 endTime = JULIANTIMESTAMP();

    endTime_ts = endTime;

    std::tr1::shared_ptr<SESSION_END> pSession_info = std::tr1::shared_ptr<SESSION_END>(new SESSION_END);
    *pSession_info = {0};

    pSession_info->m_process_id = srvrGlobal->process_id;
    pSession_info->m_thread_id = srvrGlobal->receiveThrId;
    pSession_info->m_node_id = srvrGlobal->m_NodeId;
    pSession_info->m_ip_address_id = srvrGlobal->IpAddress;
    pSession_info->m_process_name = srvrGlobal->m_ProcName;
    pSession_info->m_sessionId = srvrGlobal->sessionId;
    pSession_info->m_session_status = "END";
    pSession_info->m_session_start_utc_ts = startTime_ts;
    pSession_info->m_session_end_utc_ts = endTime_ts;
    pSession_info->m_user_id = srvrGlobal->userID;
    pSession_info->m_user_name = srvrGlobal->userSID;
    pSession_info->m_role_name = srvrGlobal->QSRoleName;
    pSession_info->m_client_name = srvrGlobal->ClientComputerName;
    pSession_info->m_client_user_name = resCollectinfo.clientUserName;
    pSession_info->m_application_name = srvrGlobal->ApplicationName;
    pSession_info->m_total_odbc_exection_time = totalOdbcExecutionTime;
    pSession_info->m_total_odbc_elapsed_time = totalOdbcElapseTime;
    pSession_info->m_total_insert_stmts_executed = totalInsertStatements;
    pSession_info->m_total_delete_stmts_executed = totalDeleteStatements;
    pSession_info->m_total_update_stmts_executed = totalUpdateStatements;
    pSession_info->m_total_select_stmts_executed = totalSelectStatements;
    pSession_info->m_total_catalog_stmts = totalCatalogStatements;
    pSession_info->m_total_prepares = totalPrepares;
    pSession_info->m_total_executes = totalExecutes;
    pSession_info->m_total_fetches = totalFetches;
    pSession_info->m_total_closes = totalCloses;
    pSession_info->m_total_execdirects = totalExecDirects;
    pSession_info->m_total_errors = totalErrors;
    pSession_info->m_total_warnings = totalWarnings;
    pSession_info->m_total_login_elapsed_time_mcsec = resCollectinfo.totalLoginTime;
    pSession_info->m_ldap_login_elapsed_time_mcsec = resCollectinfo.ldapLoginTime;
    pSession_info->m_sql_user_elapsed_time_mcsec = resCollectinfo.sqlUserTime;
    pSession_info->m_search_connection_elapsed_time_mcsec = resCollectinfo.searchConnectionTime;
    pSession_info->m_search_elapsed_time_mcsec = resCollectinfo.searchTime;
    pSession_info->m_authentication_connection_elapsed_time_mcsec = resCollectinfo.authenticationConnectionTime;
    pSession_info->m_authentication_elapsed_time_mcsec =  resCollectinfo.authenticationTime;

    if ( srvrGlobal->m_bStatisticsEnabled )
    {
        if( srvrGlobal->m_statisticsPubType == STATISTICS_AGGREGATED )
        {
            std::tr1::shared_ptr<SESSION_AGGREGATION> pAggr_info = getAggrStats();
            sendAggrStats(PUB_TYPE_SESSION_END_AGGREGATION, pAggr_info);
        }
        sendSessionEnd(pSession_info);
    }
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
        if (ps->errorStatement)
            totalSelectErrors++;
        break;
    case TYPE_INSERT:
        totalInsertStatements++;
        if (ps->errorStatement)
            totalInsertErrors++;
        break;
    case TYPE_DELETE:
        totalDeleteStatements++;
        if (ps->errorStatement)
            totalDeleteErrors++;
        break;
    case TYPE_UPDATE:
        totalUpdateStatements++;
        if (ps->errorStatement)
            totalUpdateErrors++;
        break;
    default:
        switch (ps->sqlNewQueryType)
        {
        case SQL_OTHER:
        case SQL_CALL_NO_RESULT_SETS:
        case SQL_CALL_WITH_RESULT_SETS:
        case SQL_SP_RESULT_SET:
            totalOtherStatements++;
            if (ps->errorStatement)
                totalOtherErrors++;
            break;
        case SQL_CAT_UTIL:
        case SQL_EXE_UTIL:
            totalUtilStatements++;
            if (ps->errorStatement)
                totalUtilErrors++;
            break;
        default:
            break;
        }
        break;
    }
    totalOdbcElapseTime = totalOdbcElapseTime + ps->odbcElapseTime;
    totalOdbcExecutionTime = totalOdbcExecutionTime + ps->odbcExecutionTime;
    //totalSqlExecutionTime = totalSqlExecutionTime + ps->SqlExecutionTime;
    //totalSqlElapseTime = totalSqlElapsedTime + ps->SqlElapsedTime;
    totalErrors = totalErrors + ps->errorStatement;
    totalWarnings = totalWarnings + ps->warningStatement;
}

// +++ T2_REPO TODO
void ResStatisticsSession::accumulateStatistics(const ResStatistics * const pResStats)
{
    ResStatisticsStatement *pResStatsStmt = (ResStatisticsStatement *)pResStats;

// Aggregated counters
//
//  pSrvrStmt->m_aggODBCElapsedTime         += pResStatsStmt->inqueryElapseTime;                //EVERY
//  pSrvrStmt->m_aggODBCExecutionTime       += pResStatsStmt->inqueryExecutionTime;         //EVERY

    sessWlStats.aggrStats.EstimatedRowsAccessed += pResStatsStmt->estRowsAccessed;              //AGGREG
    sessWlStats.aggrStats.EstimatedRowsUsed     += pResStatsStmt->estRowsUsed;                  //AGGREG
    sessWlStats.aggrStats.StatsBytes            += pResStatsStmt->StatsBytes;                   //AGGREG
    sessWlStats.aggrStats.RowsReturned          += pResStatsStmt->rowsReturned;             //AGGREG
    sessWlStats.aggrStats.SQLProcessBusyTime    += pResStatsStmt->ProcessBusyTime;              //AGGREG
    sessWlStats.aggrStats.AQRnumRetries         += pResStatsStmt->AQRnumRetries;                //AGGREG
    sessWlStats.aggrStats.AQRdelayBeforeRetry   += pResStatsStmt->AQRdelayBeforeRetry;          //AGGREG
    sessWlStats.aggrStats.NumberOfRows          += pResStatsStmt->numberOfRows;             //AGGREG
    sessWlStats.aggrStats.OpenBusyTime          += pResStatsStmt->OpenTime;                 //AGGREG
    sessWlStats.aggrStats.NumOpens              += pResStatsStmt->Opens;                        //AGGREG
    sessWlStats.aggrStats.ProcessesCreated      += pResStatsStmt->NewProcess;                   //AGGREG
    sessWlStats.aggrStats.ProcessCreateBusyTime += pResStatsStmt->NewProcessTime;               //AGGREG
    sessWlStats.aggrStats.RowsAccessed          += pResStatsStmt->AccessedRows;                 //AGGREG
    sessWlStats.aggrStats.RowsRetrieved         += pResStatsStmt->UsedRows;                     //AGGREG
    sessWlStats.aggrStats.DiscProcessBusyTime   += pResStatsStmt->DiskProcessBusyTime;          //AGGREG
    sessWlStats.aggrStats.DiscReads             += pResStatsStmt->DiskIOs;                      //AGGREG
    sessWlStats.aggrStats.SpaceTotal            += pResStatsStmt->SpaceTotal;                   //AGGREG
    sessWlStats.aggrStats.SpaceUsed             += pResStatsStmt->SpaceUsed;                    //AGGREG
    sessWlStats.aggrStats.HeapTotal             += pResStatsStmt->HeapTotal;                    //AGGREG
    sessWlStats.aggrStats.HeapUsed              += pResStatsStmt->HeapUsed;                 //AGGREG
    sessWlStats.aggrStats.TotalMemory           += pResStatsStmt->TotalMemAlloc;                //AGGREG
    sessWlStats.aggrStats.Dp2SpaceTotal         += pResStatsStmt->Dp2SpaceTotal;                //AGGREG
    sessWlStats.aggrStats.Dp2SpaceUsed          += pResStatsStmt->Dp2SpaceUsed;             //AGGREG
    sessWlStats.aggrStats.Dp2HeapTotal          += pResStatsStmt->Dp2HeapTotal;             //AGGREG
    sessWlStats.aggrStats.Dp2HeapUsed           += pResStatsStmt->Dp2HeapUsed;                  //AGGREG
    sessWlStats.aggrStats.MsgsToDisc            += pResStatsStmt->NumMessages;                  //AGGREG
    sessWlStats.aggrStats.MsgsBytesToDisc       += pResStatsStmt->MessagesBytes;                //AGGREG
    sessWlStats.aggrStats.NumRqstMsgs           += pResStatsStmt->reqMsgCnt;                    //AGGREG
    sessWlStats.aggrStats.NumRqstMsgBytes       += pResStatsStmt->reqMsgBytes;                  //AGGREG
    sessWlStats.aggrStats.NumRplyMsgs           += pResStatsStmt->replyMsgCnt;                  //AGGREG
    sessWlStats.aggrStats.NumRplyMsgBytes       += pResStatsStmt->replyMsgBytes;                //AGGREG
    sessWlStats.aggrStats.LockWaits             += pResStatsStmt->LockWaits;                    //AGGREG
    sessWlStats.aggrStats.LockEscalation        += pResStatsStmt->Escalations;                  //AGGREG
    sessWlStats.aggrStats.TotalExecutes++;
    sessWlStats.aggrStats.totalSelects          = totalSelectStatements;
    sessWlStats.aggrStats.totalInserts          = totalInsertStatements;
    sessWlStats.aggrStats.totalUpdates          = totalUpdateStatements;
    sessWlStats.aggrStats.totalDeletes          = totalDeleteStatements;
    sessWlStats.aggrStats.totalDDLs             = totalDDLStatements;
    sessWlStats.aggrStats.totalUtils            = totalUtilStatements;
    sessWlStats.aggrStats.totalCatalogs         = totalCatalogStatements;
    sessWlStats.aggrStats.totalOthers           = totalOtherStatements;
    sessWlStats.aggrStats.totalSelectErrors     = totalSelectErrors;
    sessWlStats.aggrStats.totalInsertErrors     = totalInsertErrors;
    sessWlStats.aggrStats.totalUpdateErrors     = totalUpdateErrors;
    sessWlStats.aggrStats.totalDeleteErrors     = totalDeleteErrors;
    sessWlStats.aggrStats.totalDDLErrors        = totalDDLErrors;
    sessWlStats.aggrStats.totalUtilErrors       = totalUtilErrors;
    sessWlStats.aggrStats.totalCatalogErrors    = totalCatalogErrors;
    sessWlStats.aggrStats.totalOtherErrors      = totalOtherErrors;
    sessWlStats.aggrStats.NumRowsIUD            = (totalInsertStatements+totalUpdateStatements+totalDeleteStatements);

}

void ResStatisticsSession::update()
{
    if(srvrGlobal->m_bStatisticsEnabled && srvrGlobal->m_statisticsPubType == STATISTICS_AGGREGATED && endTime_ts == 0)
    {
        sessWlStats.computeDeltaStats();
        std::tr1::shared_ptr<SESSION_AGGREGATION> pAggr_info = getAggrStats();
        sendAggrStats(PUB_TYPE_SESSION_UPDATE_AGGREGATION, pAggr_info);
    }
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
    totalDDLStatements = 0;
    totalUtilStatements = 0;
    totalOtherStatements = 0;
    totalInsertErrors = 0;
    totalUpdateErrors = 0;
    totalDeleteErrors = 0;
    totalSelectErrors = 0;
    totalDDLErrors = 0;
    totalUtilErrors = 0;
    totalOtherErrors = 0;
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
    sessWlStats.aggrStats.reset();
    sessWlStats.deltaStats.reset();

    startTime_ts = 0;
    entryTime_ts = 0;
    endTime_ts = 0;
}

std::tr1::shared_ptr<SESSION_AGGREGATION> ResStatisticsSession::getAggrStats()
{
    std::tr1::shared_ptr<SESSION_AGGREGATION> pAggr_info = std::tr1::shared_ptr<SESSION_AGGREGATION>(new SESSION_AGGREGATION);
    *pAggr_info = {0};

    pAggr_info->m_process_id = srvrGlobal->process_id;
    pAggr_info->m_thread_id = srvrGlobal->receiveThrId;
    pAggr_info->m_node_id = srvrGlobal->m_NodeId;
    pAggr_info->m_ip_address_id = srvrGlobal->IpAddress;
    pAggr_info->m_process_name = srvrGlobal->m_ProcName;
    pAggr_info->m_sessionId = srvrGlobal->sessionId;
    pAggr_info->m_session_start_utc_ts = startTime_ts;
    pAggr_info->m_aggregation_last_update_utc_ts = JULIANTIMESTAMP();
    pAggr_info->m_aggregation_last_elapsed_time = srvrGlobal->m_iAggrInterval * 1000;   // milliseconds
    pAggr_info->m_user_id = srvrGlobal->userID;
    pAggr_info->m_user_name = srvrGlobal->userSID;
    pAggr_info->m_role_name = srvrGlobal->QSRoleName;
    pAggr_info->m_client_name = srvrGlobal->ClientComputerName;
    pAggr_info->m_application_name = srvrGlobal->ApplicationName;
    pAggr_info->m_client_user_name = resCollectinfo.clientUserName;
    pAggr_info->m_total_est_rows_accessed = sessWlStats.aggrStats.EstimatedRowsAccessed;
    pAggr_info->m_total_est_rows_used = sessWlStats.aggrStats.EstimatedRowsUsed;
    pAggr_info->m_total_rows_retrieved = sessWlStats.aggrStats.RowsRetrieved;
    pAggr_info->m_total_num_rows_iud = sessWlStats.aggrStats.NumRowsIUD;
    pAggr_info->m_total_selects = sessWlStats.aggrStats.totalSelects;
    pAggr_info->m_total_inserts = sessWlStats.aggrStats.totalInserts;
    pAggr_info->m_total_updates = sessWlStats.aggrStats.totalUpdates;
    pAggr_info->m_total_deletes = sessWlStats.aggrStats.totalDeletes;
    pAggr_info->m_total_ddl_stmts = sessWlStats.aggrStats.totalDDLs;
    pAggr_info->m_total_util_stmts = sessWlStats.aggrStats.totalUtils;
    pAggr_info->m_total_catalog_stmts = sessWlStats.aggrStats.totalCatalogs;
    pAggr_info->m_total_other_stmts = sessWlStats.aggrStats.totalOthers;
    pAggr_info->m_total_select_errors = sessWlStats.aggrStats.totalSelectErrors;
    pAggr_info->m_total_insert_errors = sessWlStats.aggrStats.totalInsertErrors;
    pAggr_info->m_total_update_errors = sessWlStats.aggrStats.totalUpdateErrors;
    pAggr_info->m_total_delete_errors = sessWlStats.aggrStats.totalDeleteErrors;
    pAggr_info->m_total_ddl_errors = sessWlStats.aggrStats.totalDDLErrors;
    pAggr_info->m_total_util_errors = sessWlStats.aggrStats.totalUtilErrors;
    pAggr_info->m_total_catalog_errors = sessWlStats.aggrStats.totalCatalogErrors;
    pAggr_info->m_total_other_errors = sessWlStats.aggrStats.totalOtherErrors;
    pAggr_info->m_delta_estimated_rows_accessed =max(double(0), sessWlStats.deltaStats.EstimatedRowsAccessed);
    pAggr_info->m_delta_estimated_rows_used = max(double(0),sessWlStats.deltaStats.EstimatedRowsUsed);
    pAggr_info->m_delta_rows_accessed = max(int64(0),sessWlStats.deltaStats.RowsAccessed);
    pAggr_info->m_delta_rows_retrieved = max(int64(0),sessWlStats.deltaStats.RowsRetrieved);
    pAggr_info->m_delta_num_rows_iud = max(int64(0),sessWlStats.deltaStats.NumRowsIUD);
    pAggr_info->m_delta_total_selects = max(int64(0),sessWlStats.deltaStats.totalSelects);
    pAggr_info->m_delta_total_inserts = max(int64(0),sessWlStats.deltaStats.totalInserts);
    pAggr_info->m_delta_total_updates = max(int64(0),sessWlStats.deltaStats.totalUpdates);
    pAggr_info->m_delta_total_deletes = max(int64(0),sessWlStats.deltaStats.totalDeletes);
    pAggr_info->m_delta_total_ddl_stmts = max(int64(0),sessWlStats.deltaStats.totalDDLs);
    pAggr_info->m_delta_total_util_stmts = max(int64(0),sessWlStats.deltaStats.totalUtils);
    pAggr_info->m_delta_total_catalog_stmts = max(int64(0),sessWlStats.deltaStats.totalCatalogs);
    pAggr_info->m_delta_total_other_stmts = max(int64(0),sessWlStats.deltaStats.totalOthers);
    pAggr_info->m_delta_select_errors = max(int64(0),sessWlStats.deltaStats.totalSelectErrors);
    pAggr_info->m_delta_insert_errors = max(int64(0),sessWlStats.deltaStats.totalInsertErrors);
    pAggr_info->m_delta_update_errors = max(int64(0),sessWlStats.deltaStats.totalUpdateErrors);
    pAggr_info->m_delta_delete_errors = max(int64(0),sessWlStats.deltaStats.totalDeleteErrors);
    pAggr_info->m_delta_ddl_errors = max(int64(0),sessWlStats.deltaStats.totalDDLErrors);
    pAggr_info->m_delta_util_errors = max(int64(0),sessWlStats.deltaStats.totalUtilErrors);
    pAggr_info->m_delta_catalog_errors = max(int64(0),sessWlStats.deltaStats.totalCatalogErrors);
    pAggr_info->m_delta_other_errors = max(int64(0),sessWlStats.deltaStats.totalOtherErrors);

    return pAggr_info;
}

ResStatisticsSession::ResStatisticsSession()
{
}

ResStatisticsSession::~ResStatisticsSession()
{
}

