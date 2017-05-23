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

/* MODULE: ResStatisticsStatement.cpp
   PURPOSE: Implements the member functions of ResStatisticsStatement class
*/
//#define STATS_CLI
//#undef STATS_CLI

#include <platform_ndcs.h>
#include "ResStatisticsStatement.h"
#include "CoreCommon.h"
#include "SrvrCommon.h"
#include <errno.h>
#include <sys/resource.h>
// #include "QSGlobal.h"    +++ T2_REPO
// #include "QSData.h"      +++ T2_REPO
#include <sstream>
#include <algorithm>

// #include "commonFunctions.h"     +++ T2_REPO
using namespace std;
#define SEP " "
#define PREPARE 1
#define EXECUTE 2
#define FETCH 3
#define CLOSE 4
#define EXECDIRECT 5

#define STATS_ROWTYPE       "statsRowType"
#define STATS_ROWTYPE_LEN   12
//

void sendQueryStats(pub_struct_type pub_type, std::tr1::shared_ptr<STATEMENT_QUERYEXECUTION> pQuery_info);

void UpdateStringText(string& textStr)
{
    int pos = 0;
    while (pos != -1)
    {
        pos = textStr.find("'", pos);
        if (pos != -1)
        {
            if(textStr.substr(pos, 2).compare("''") != 0)
                textStr.insert(pos, "'");
            pos = pos + 2;
        }
    }
}

extern ResStatisticsSession   *resStatSession;

// using namespace SRVR;    +++ T2_REPO

ResStatisticsStatement::ResStatisticsStatement(bool useCLI)
{
    useCLI_ = useCLI;

    flag = 0;
    memset(tmpString,'\0',MAX_ROWBUF_SIZE);
    tmpFlag = FALSE;
    catFlagOn = FALSE;
    SQLValue = NULL;

    // row 1 of RMS stats
    memset(queryId, '\0', MAX_QUERY_NAME_LEN + 1);
    compStartTime = 0;
    compEndTime = 0;
    compTime = 0;
    exeStartTime = 0;
    exeEndTime = 0;
    exeTime = 0; // same as exeElapsedTime ?
//  rowsAffected = 0;
    sqlErrorCode = 0;
    statsErrorCode = 0;
    state = 0;
    statsType = 0;
    queryType = 0;
    estRowsAccessed = 0.0f;
    estRowsUsed = 0.0f;
    exeElapsedTime = 0;
    memset(parentQID, '\0', MAX_QUERY_NAME_LEN + 1); // start of new col
    strcpy(parentQID,"NONE");
    memset(childQID, '0', sizeof(childQID));
    numSqlProcs = 0;
    numCpus = 0;
    memset(sqlSrc, '\0', RMS_STORE_SQL_SOURCE_LEN + 1);
    sqlSrcLen = 0;
    exePriority = 0;
    memset(transID, '\0', MAX_TXN_STR_LEN + 1);
    strcpy(transID,"<N/A>");
    rowsReturned = 0;
    NumRowsIUD = 0;
    firstRowReturnTime = 0;
    memset(subQryType, '\0', SUB_QRY_TYPE_LEN  + 1);
    memset(parentSysName, '\0', PAR_SYS_NAME_LEN + 1);

        // row2 of RMS stats
    NumMessages = 0;
    MessagesBytes = 0;
    AccessedRows = 0;
    UsedRows = 0;
    DiskIOs = 0;
    Escalations = 0;
    LockWaits = 0;
    Opens = 0;
    OpenTime = 0;
    StatsBytes = 0;
    ProcessBusyTime = 0;
    DiskProcessBusyTime = 0;
    NewProcess = 0;
    NewProcessTime = 0;
    SpaceTotal = 0;
    SpaceUsed = 0;
    HeapTotal = 0;
    HeapUsed = 0;
    HeapWM = 0;
    CpuTime = 0;
    Dp2SpaceTotal = 0;
    Dp2SpaceUsed = 0;
    Dp2HeapTotal = 0;
    Dp2HeapUsed = 0;

    UdrCpuTime = 0;

    reqMsgCnt = 0;
    reqMsgBytes = 0;
    replyMsgCnt = 0;
    replyMsgBytes = 0;

    TotalMemAlloc = 0;

    ScratchFileCount = 0;
    ScratchBufferBlockSize = 0;
    ScratchBufferBlocksRead = 0;
    ScratchBufferBlocksWritten = 0;
    ScratchOverflowMode = 0;
    ScratchBufferReadCount = 0;
    ScratchBufferWriteCount = 0;
    bmoSpaceBufferSize = 0;
    bmoSpaceBufferCount = 0;
    bmoInterimRowCount = 0;
    ScratchIOSize = 0;
    ScratchIOMaxTime = 0;
    topN = 0;
    estimatedCost = 0;

    rtsExeCols = NULL;
    rtsDp2Cols = NULL;
//
// AGGREGATION----------------CONSTRUCTOR
//
//
// rules
//
    bzero(con_rule_name,sizeof(con_rule_name));
    bzero(cmp_rule_name,sizeof(cmp_rule_name));
    bzero(exe_rule_name,sizeof(exe_rule_name));

    pubStarted = false;
    queryFinished = false;
    wouldLikeToStart_ts = 0;
}


ResStatisticsStatement::~ResStatisticsStatement()
{
    RTS_Col *tmpRtsCol;

    while ( rtsExeCols ) {
        tmpRtsCol = rtsExeCols->next;
        delete rtsExeCols;
        rtsExeCols = NULL;
        rtsExeCols = tmpRtsCol;
    }

    while ( rtsDp2Cols ) {
        tmpRtsCol = rtsDp2Cols->next;
        delete rtsDp2Cols;
        rtsDp2Cols = NULL;
        rtsDp2Cols = tmpRtsCol;
    }

}

void ResStatisticsStatement::init()
{

    stmtType = 0;
    //sqlExecutionTime = 0;
    //sqlElapseTime = 0;
    odbcElapseTime = 0;
    odbcExecutionTime = 0;
    queryElapseTime = 0;
    queryExecutionTime = 0;
    estimatedCost = 0;
    maxRowCnt = 1000;
    maxRowLen = 1000;
    retcode = 0;

    //sqlExecutionTime = 0;
    //sqlElapseTime = 0;

    // row 1
    memset(queryId, '\0', MAX_QUERY_NAME_LEN + 1);
    compStartTime = 0;
    compEndTime = 0;
    compTime = 0;
    exeStartTime = 0;
    exeEndTime = 0;
    exeTime = 0;
//  rowsAffected = 0;
    sqlErrorCode = 0;
    statsErrorCode = 0;
    state = 0;
    statsType = 0;
    queryType = 0;
    estRowsAccessed = 0.0f;
    estRowsUsed = 0.0f;
    exeElapsedTime = 0;
    // New_Col
    memset(parentQID, '\0', sizeof(parentQID));
    strcpy(parentQID,"NONE");
    memset(childQID, '0', sizeof(childQID));
    numSqlProcs = 0;
    numCpus = 0;
    memset(sqlSrc, '\0', sizeof(sqlSrc));
    sqlSrcLen = 0;
    exePriority = 0;
    memset(transID , '\0', sizeof(transID ));
    strcpy(transID,"<N/A>");
    rowsReturned = 0;
    firstRowReturnTime = 0;
    memset(subQryType, '\0', sizeof(subQryType));
    memset(parentSysName, '\0', sizeof(parentSysName));

    // row 2
    NumMessages = 0;
    MessagesBytes = 0;
    AccessedRows = 0;
    UsedRows = 0;
    DiskIOs = 0;
    Escalations = 0;
    LockWaits = 0;
    Opens = 0;
    OpenTime = 0;
    StatsBytes = 0;
    ProcessBusyTime = 0;
    DiskProcessBusyTime = 0;
    NewProcess = 0;
    NewProcessTime = 0;
    SpaceTotal = 0;
    SpaceUsed = 0;
    HeapTotal = 0;
    HeapUsed = 0;
    CpuTime = 0;
    Dp2SpaceTotal = 0;
    Dp2SpaceUsed = 0;
    Dp2HeapTotal = 0;
    Dp2HeapUsed = 0;

    UdrCpuTime = 0;

    // New_Col
    reqMsgCnt = 0;
    reqMsgBytes = 0;
    replyMsgCnt = 0;
    replyMsgBytes = 0;

    TotalMemAlloc = 0;
    estTotalMem = 0;
//
    ScratchFileCount = 0;
    ScratchBufferBlockSize = 0;
    ScratchBufferBlocksRead = 0;
    ScratchBufferBlocksWritten = 0;
    ScratchOverflowMode = 0;
    ScratchBufferReadCount = 0;
    ScratchBufferWriteCount = 0;
}
//
// +++ T2_REPO TODO
string getSrvrSubstate(unsigned short mx_substate)
{
    return "";
}
/*
string getSrvrSubstate(NDCS_SUBSTATE mx_substate)
{
    switch(mx_substate)
    {
    case NDCS_DLG_INIT:             return FMT_NDCS_DLG_INIT;
    case NDCS_CONN_IDLE:            return FMT_NDCS_CONN_IDLE;
    case NDCS_DLG_TERM:             return FMT_NDCS_DLG_TERM;
    case NDCS_DLG_BREAK:            return FMT_NDCS_DLG_BREAK;
    case NDCS_STOP_SRVR:            return FMT_NDCS_STOP_SRVR;
    case NDCS_RMS_ERROR:            return FMT_NDCS_RMS_ERROR;
    case NDCS_REPOS_IDLE:           return FMT_NDCS_REPOS_IDLE;
    case NDCS_REPOS_INTERVAL:       return FMT_NDCS_REPOS_INTERVAL;
    case NDCS_REPOS_PARTIAL:        return FMT_NDCS_REPOS_PARTIAL;
    case NDCS_EXEC_INTERVAL:        return FMT_NDCS_EXEC_INTERVAL;
    case NDCS_CONN_RULE_CHANGED:    return FMT_NDCS_CONN_RULE_CHANGED;
    case NDCS_CLOSE:                return FMT_NDCS_CLOSE;
    case NDCS_PREPARE:              return FMT_NDCS_PREPARE;
    case NDCS_WMS_ERROR:            return FMT_NDCS_WMS_ERROR;
    case NDCS_QUERY_CANCELED:       return FMT_NDCS_QUERY_CANCELED;
    case NDCS_QUERY_REJECTED:       return FMT_NDCS_QUERY_REJECTED;

    case NDCS_INIT:
    default:
        return FMT_UNKNOWN;
    }
    return "";
}
*/

// +++ T2_REPO - Not used???
//
//===============================================
/*
string getSQLStateStringRes(UInt32 state)
{
    switch(state)
    {
    case SQLSTMT_STATE_INITIAL:         return FMT_INITIAL;
    case SQLSTMT_STATE_OPEN:            return FMT_OPEN;
    case SQLSTMT_STATE_EOF:             return FMT_EOF;
    case SQLSTMT_STATE_CLOSE:           return FMT_CLOSE;
    case SQLSTMT_STATE_DEALLOCATED:     return FMT_DEALLOCATED;
    case SQLSTMT_STATE_FETCH:           return FMT_FETCH;
    case SQLSTMT_STATE_CLOSE_TABLES:    return FMT_CLOSE_TABLES;
    case SQLSTMT_STATE_PREPARE:         return FMT_PREPARE;
    case SQLSTMT_STATE_PROCESS_ENDED:   return FMT_PROCESS_ENDED;

    default:
        return FMT_UNKNOWN;
    }
}
*/

// +++ T2_REPO - TODO
string getQueryStateStringRes(unsigned short state)
{
/*
    switch(state)
    {
    case QUERY_INIT:
        return FMT_INIT;
        break;
    case QUERY_WAITING:
    case QUERY_WAITING_MAX_CPU_BUSY:
    case QUERY_WAITING_MAX_MEM_USAGE:
    case QUERY_WAITING_RELEASED_BY_ADMIN:
    case QUERY_WAITING_MAX_SERVICE_EXEC_QUERIES:
    case QUERY_WAITING_MAX_INSTANCE_EXEC_QUERIES:
    case QUERY_WAITING_TXN_BACKOUT:
    case QUERY_WAITING_MAX_ESPS:
    case QUERY_WAITING_CANARY_EXEC:
    case QUERY_WAITING_EST_MAX_CPU_BUSY:
        return FMT_WAITING;
        break;
    case QUERY_EXECUTING:
    case QUERY_EXECUTING_RELEASED_BY_ADMIN:
    case QUERY_EXECUTING_RELEASED_BY_RULE:
    case QUERY_EXECUTING_CANCEL_IN_PROGRESS:
    case QUERY_EXECUTING_CANCEL_FAILED:
    case QUERY_EXECUTING_CANCEL_FAILED_8026:
    case QUERY_EXECUTING_CANCEL_FAILED_8027:
    case QUERY_EXECUTING_CANCEL_FAILED_8028:
    case QUERY_EXECUTING_CANCEL_FAILED_8029:
    case QUERY_EXECUTING_CANCEL_FAILED_8031:
        return FMT_EXECUTING;
        break;
    case QUERY_HOLDING:
    case QUERY_HOLDING_LOAD:
    case QUERY_HOLDING_REPREPARING:
    case QUERY_HOLDING_EXECUTING_SQL_CMD:
    case QUERY_HOLDING_BY_RULE:
    case QUERY_HOLDING_BY_ADMIN:
        return FMT_HOLDING;
        break;
    case QUERY_COMPLETED:
    case QUERY_COMPLETED_HOLD_TIMEOUT:
    case QUERY_COMPLETED_EXEC_TIMEOUT:
    case QUERY_COMPLETED_BY_ADMIN:
    case QUERY_COMPLETED_QUERY_NOT_FOUND:
    case QUERY_COMPLETED_CONNECTION_FAILED:
    case QUERY_COMPLETED_NDCS_PROCESS_FAILED:
    case QUERY_COMPLETED_CPU_FAILED:
    case QUERY_COMPLETED_SEGMENT_FAILED:
    case QUERY_COMPLETED_BY_RULE:
    case QUERY_COMPLETED_SERVICE_NOT_ACTIVE:
    case QUERY_COMPLETED_HARDWARE_FAILURE:
    case QUERY_COMPLETED_UNEXPECTED_STATE:
    case QUERY_COMPLETED_CLIENT_DISAPPEARED:
    case QUERY_COMPLETED_BY_CLIENT:
//
    case QUERY_COMPLETED_NDCS_DLG_INIT:
    case QUERY_COMPLETED_NDCS_CONN_IDLE:
    case QUERY_COMPLETED_NDCS_DLG_TERM:
    case QUERY_COMPLETED_NDCS_DLG_BREAK:
    case QUERY_COMPLETED_NDCS_STOP_SRVR:
    case QUERY_COMPLETED_NDCS_RMS_ERROR:
    case QUERY_COMPLETED_NDCS_REPOS_IDLE:
    case QUERY_COMPLETED_NDCS_REPOS_INTERVAL:
    case QUERY_COMPLETED_NDCS_REPOS_PARTIAL:
    case QUERY_COMPLETED_NDCS_EXEC_INTERVAL:
    case QUERY_COMPLETED_NDCS_CONN_RULE_CHANGED:
    case QUERY_COMPLETED_NDCS_CLOSE:
    case QUERY_COMPLETED_NDCS_PREPARE:
    case QUERY_COMPLETED_NDCS_WMS_ERROR:
        return FMT_COMPLETED;
        break;
    case QUERY_REJECTED:
    case QUERY_REJECTED_BY_ADMIN:
    case QUERY_REJECTED_CONNECTION_FAILED:
    case QUERY_REJECTED_NDCS_PROCESS_FAILED:
    case QUERY_REJECTED_CPU_FAILED:
    case QUERY_REJECTED_SEGMENT_FAILED:
    case QUERY_REJECTED_QMSGCANCELLED:
    case QUERY_REJECTED_VERSION_MISMATCH:
    case QUERY_REJECTED_WMSONHOLD:
    case QUERY_REJECTED_MAX_QUERIES_REACHED:
    case QUERY_REJECTED_SERVICE_NOT_FOUND:
    case QUERY_REJECTED_SERVICE_ON_HOLD:
    case QUERY_REJECTED_BY_RULE:
    case QUERY_REJECTED_UNKNOWNUSER:
    case QUERY_REJECTED_UNEXPECTED_STATE:
    case QUERY_REJECTED_HOLD_TIMEOUT:
    case QUERY_REJECTED_WAIT_TIMEOUT:
    case QUERY_REJECTED_CLIENT_DISAPPEARED:
    case QUERY_REJECTED_LONG_TRANS_ABORTING:
        return FMT_REJECTED;
        break;
    case QUERY_SUSPENDED:
    case QUERY_SUSPENDED_BY_ADMIN:
    case QUERY_SUSPENDED_BY_RULE:
    case QUERY_SUSPENDED_CANCELED:
    case QUERY_SUSPENDED_CANCELED_BY_ADMIN:
    case QUERY_SUSPENDED_CANCELED_BY_RULE:
    case QUERY_SUSPENDED_CANCELED_BY_TIMEOUT:
        return FMT_SUSPENDED;
        break;
    default:
        return FMT_UNKNOWN;
        break;
    }
*/
    return "";
}

// +++ T2_REPO - TODO
string getQuerySubStateStringRes(unsigned short state)
{
/*
    switch(state)
    {
    case QUERY_WAITING_MAX_CPU_BUSY:            return FMT_MAX_CPU_BUSY;
    case QUERY_WAITING_EST_MAX_CPU_BUSY:        return FMT_EST_MAX_CPU_BUSY;
//
    case QUERY_WAITING_MAX_MEM_USAGE:           return FMT_MAX_MEM_USAGE;
//
    case QUERY_WAITING_RELEASED_BY_ADMIN:
    case QUERY_EXECUTING_RELEASED_BY_ADMIN:     return FMT_RELEASED_BY_ADMIN;
//
    case QUERY_EXECUTING_RELEASED_BY_RULE:      return FMT_RELEASED_BY_EXEC_RULE;
//
    case QUERY_EXECUTING_CANCEL_IN_PROGRESS:    return FMT_CANCEL_IN_PROGRESS;
//
    case QUERY_WAITING_MAX_SERVICE_EXEC_QUERIES:    return FMT_MAX_SERVICE_EXEC_QUERIES;
    case QUERY_WAITING_MAX_INSTANCE_EXEC_QUERIES:   return FMT_MAX_INSTANCE_EXEC_QUERIES;
//
    case QUERY_WAITING_TXN_BACKOUT:             return FMT_WAITING_TXN_BACKOUT;
//
    case QUERY_WAITING_MAX_ESPS:                return FMT_MAX_AVG_ESPS;
    case QUERY_WAITING_CANARY_EXEC:             return FMT_WAITING_CANARY;
//
    case QUERY_HOLDING_LOAD:                    return FMT_LOADING;
//
    case QUERY_HOLDING_REPREPARING:             return FMT_REPREPARING;
//
    case QUERY_HOLDING_EXECUTING_SQL_CMD:       return FMT_EXECUTING_SQL_CMD;
//
    case QUERY_REJECTED_BY_RULE:
    case QUERY_HOLDING_BY_RULE:                 return FMT_BY_COMP_RULE;
//
    case QUERY_SUSPENDED_BY_RULE:               return FMT_BY_EXEC_RULE;
//
    case QUERY_SUSPENDED_BY_ADMIN:
    case QUERY_REJECTED_BY_ADMIN:
    case QUERY_HOLDING_BY_ADMIN:                return FMT_BY_ADMIN;
//
    case QUERY_REJECTED_HOLD_TIMEOUT:
    case QUERY_COMPLETED_HOLD_TIMEOUT:          return FMT_HOLD_TIMEOUT;
//
    case QUERY_COMPLETED_EXEC_TIMEOUT:          return FMT_EXEC_TIMEOUT;
//
    case QUERY_COMPLETED_BY_ADMIN:              return FMT_CANCELLED_BY_ADMIN;
    case QUERY_COMPLETED_BY_CLIENT:         return FMT_CANCELLED_BY_CLIENT;
//
    case QUERY_COMPLETED_QUERY_NOT_FOUND:       return FMT_QUERY_NOT_FOUND;
//
    case QUERY_REJECTED_CONNECTION_FAILED:
    case QUERY_COMPLETED_CONNECTION_FAILED:     return FMT_CONNECTION_FAILED;
//
    case QUERY_REJECTED_NDCS_PROCESS_FAILED:
    case QUERY_COMPLETED_NDCS_PROCESS_FAILED:   return FMT_NDCS_PROCESS_FAILED;
//
    case QUERY_REJECTED_CPU_FAILED:
    case QUERY_COMPLETED_CPU_FAILED:            return FMT_CPU_FAILED;
//
    case QUERY_REJECTED_SEGMENT_FAILED:
    case QUERY_COMPLETED_SEGMENT_FAILED:        return FMT_SEGMENT_FAILED;
//
    case QUERY_COMPLETED_BY_RULE:               return FMT_BY_EXEC_RULE;
//
    case QUERY_COMPLETED_SERVICE_NOT_ACTIVE:    return FMT_SERVICE_NOT_ACTIVE;
//
    case QUERY_REJECTED_UNEXPECTED_STATE:
    case QUERY_COMPLETED_UNEXPECTED_STATE:      return FMT_UNEXPECTED_STATE;
//
    case QUERY_REJECTED_CLIENT_DISAPPEARED:
    case QUERY_COMPLETED_CLIENT_DISAPPEARED:    return FMT_CLIENT_DISAPPEARED;
//
    case QUERY_REJECTED_QMSGCANCELLED:          return FMT_QUEUE_MSG_CANCELLED;
//
    case QUERY_REJECTED_VERSION_MISMATCH:       return FMT_VERSION_MISMATCH;
//
    case QUERY_REJECTED_WMSONHOLD:              return FMT_WMS_ON_HOLD;
//
    case QUERY_REJECTED_MAX_QUERIES_REACHED:    return FMT_MAX_QUERIES_REACHED;
//
    case QUERY_REJECTED_SERVICE_NOT_FOUND:      return FMT_SERVICE_NOT_FOUND;
//
    case QUERY_REJECTED_SERVICE_ON_HOLD:        return FMT_SERVICE_ON_HOLD;
//
    case QUERY_REJECTED_UNKNOWNUSER:            return FMT_UNKNOWN_USER;
//
    case QUERY_REJECTED_WAIT_TIMEOUT:           return FMT_WAIT_TIMEOUT;
//
    case QUERY_SUSPENDED_CANCELED:              return FMT_QUERY_CANCELED;
    case QUERY_SUSPENDED_CANCELED_BY_RULE:      return FMT_QUERY_CANCELED_BY_RULE;
    case QUERY_SUSPENDED_CANCELED_BY_ADMIN:     return FMT_QUERY_CANCELED_BY_ADMIN;
    case QUERY_SUSPENDED_CANCELED_BY_TIMEOUT:   return FMT_QUERY_CANCELED_BY_TIMEOUT;
//
    case QUERY_COMPLETED_NDCS_DLG_INIT:         return FMT_NDCS_DLG_INIT;
    case QUERY_COMPLETED_NDCS_CONN_IDLE:        return FMT_NDCS_CONN_IDLE;
    case QUERY_COMPLETED_NDCS_DLG_TERM:         return FMT_NDCS_DLG_TERM;
    case QUERY_COMPLETED_NDCS_DLG_BREAK:        return FMT_NDCS_DLG_BREAK;
    case QUERY_COMPLETED_NDCS_STOP_SRVR:        return FMT_NDCS_STOP_SRVR;
    case QUERY_COMPLETED_NDCS_RMS_ERROR:        return FMT_NDCS_RMS_ERROR;
    case QUERY_COMPLETED_NDCS_REPOS_IDLE:       return FMT_NDCS_REPOS_IDLE;
    case QUERY_COMPLETED_NDCS_REPOS_INTERVAL:   return FMT_NDCS_REPOS_INTERVAL;
    case QUERY_COMPLETED_NDCS_REPOS_PARTIAL:    return FMT_NDCS_REPOS_PARTIAL;
    case QUERY_COMPLETED_NDCS_EXEC_INTERVAL:    return FMT_NDCS_EXEC_INTERVAL;
    case QUERY_COMPLETED_NDCS_CONN_RULE_CHANGED: return FMT_NDCS_CONN_RULE_CHANGED;
    case QUERY_COMPLETED_NDCS_CLOSE:            return FMT_NDCS_CLOSE;
    case QUERY_COMPLETED_NDCS_PREPARE:          return FMT_NDCS_PREPARE;
    case QUERY_COMPLETED_NDCS_WMS_ERROR:        return FMT_NDCS_WMS_ERROR;
    case QUERY_REJECTED_LONG_TRANS_ABORTING:    return FMT_LONG_TRANS_ABORTING;
    default:
        return FMT_NA;
    }
*/
    return "";
}

// +++ T2_REPO - Not used???
/*
string get_WarnLevelStringRes(unsigned short value)
{
    if(WLVL_HIGH & value)
        return FMT_WLVL_HIGH;
    else if(WLVL_MEDIUM & value)
        return FMT_WLVL_MEDIUM;
    else if(WLVL_LOW & value)
        return FMT_WLVL_LOW;
    else if(WLVL_NO_WARN & value)
        return FMT_WLVL_NO_WARN;
    else
        return FMT_NONE;
}
*/

void ResStatisticsStatement::start(Int32 inState,
                   Int32 inSqlQueryType,
                   const char *inStmtName,
                   SRVR_STMT_HDL *pSrvrStmt,
                   double inEstimatedCost,
                   bool *flag_21036,
                   /*
                   Int32 *returnCode,
                   Int32 *sqlWarningOrErrorLength,
                   BYTE * &sqlWarningOrError,
                   */
                   char *inSqlStatement)
{
    char *inQueryId = NULL;
    SQL_QUERY_COST_INFO cost_info;
    SQL_QUERY_COMPILER_STATS_INFO comp_stats_info;
    SQL_COMPILATION_STATS_DATA comp_stats_data;
    unsigned short queryState;
    unsigned short warnLevel;
    int64 holdTime = 0,
          waitTime = 0,
          WMSstartTS = 0;

    Int32 inSqlNewQueryType = inSqlQueryType;


    if(inState == STMTSTAT_PREPARE || inState == STMTSTAT_EXECDIRECT)
    {
        statementId[0] = '\0';
        totalStatementOdbcElapseTime = 0;
        totalStatementOdbcExecutionTime = 0;
        totalStatementExecutes = 0;
        queryElapseTime = 0;
        queryExecutionTime = 0;
    }
    if (inStmtName == NULL)
        strcpy(statementId,"<NULL>");
    else if (inStmtName[0]==0)
        strcpy(statementId,"<EMPTY>");
    else
        strcpy(statementId,inStmtName);

    // perf
    if (pSrvrStmt)
            init();

    if (catFlagOn == true) return;

    statementStartTime = JULIANTIMESTAMP();
    if(inState == STMTSTAT_PREPARE)
        prepareStartTime = statementStartTime;

    // get cpu time ( Process_getInfo)
    statementStartCpuTime = getCpuTime();
    if(inState==STMTSTAT_EXECUTE||inState == STMTSTAT_PREPARE)
    {
        queryStartTime = statementStartTime;
        queryStartCpuTime = statementStartCpuTime;
    }
    if (pSrvrStmt != NULL)
    {
        inQueryId  = pSrvrStmt->sqlUniqueQueryID;
        cost_info  = pSrvrStmt->cost_info;
        comp_stats_info = pSrvrStmt->comp_stats_info;
        comp_stats_data = comp_stats_info.compilationStats;
        queryState = pSrvrStmt->m_state;
        warnLevel  = pSrvrStmt->m_warnLevel;
        holdTime   = pSrvrStmt->m_hold_time;
        waitTime   = pSrvrStmt->m_wait_time;
        WMSstartTS = pSrvrStmt->m_WMSstart_ts;

        bzero(con_rule_name, sizeof(con_rule_name));
        bzero(cmp_rule_name, sizeof(cmp_rule_name));
        bzero(exe_rule_name, sizeof(exe_rule_name));

        memcpy(con_rule_name, pSrvrStmt->m_con_rule_name, sizeof(con_rule_name));
        memcpy(cmp_rule_name, pSrvrStmt->m_cmp_rule_name,sizeof(cmp_rule_name));
        memcpy(exe_rule_name, pSrvrStmt->m_exe_rule_name,sizeof(exe_rule_name));

        if (comp_stats_info.dop > 1)
            estTotalMem = cost_info.estimatedTotalMem * comp_stats_info.dop;
        else
            estTotalMem = cost_info.estimatedTotalMem;

        pSrvrStmt->queryStartTime = queryStartTime;
        pSrvrStmt->queryStartCpuTime = queryStartCpuTime;

        inSqlNewQueryType = pSrvrStmt->sqlNewQueryType;

        if (comp_stats_data.compilerId[0] == 0)
            strcpy(comp_stats_data.compilerId,"<N/A>");
        //if (comp_stats_data.compileInfoLen <= 0 || comp_stats_data.compileInfo[0] == 0)
        //  strcpy(comp_stats_data.compileInfo,"<N/A>");

    }

    sequenceNumber = 0;
    totalmsgNumber = 0;
    if ((srvrGlobal->resourceStatistics & STMTSTAT_SQL) && (inState == STMTSTAT_PREPARE || inState == STMTSTAT_EXECDIRECT))
    {
        if (inSqlStatement != NULL)
        {
            sqlStatement = new char[strlen(inSqlStatement) + 1];
            strcpy(sqlStatement,inSqlStatement);
        }
        else
        {
            sqlStatement = new char[5];
            strcpy(sqlStatement,"NULL");
        }
        if (sqlStatement != NULL)
        {
#ifdef RES_STATS_EVENT
            int len = min((int)strlen(sqlStatement), 254);
            string sqlString(sqlStatement, len);
            stringstream ssEvent;
            ssEvent  << "START:SQLStatement: StatementId: " << statementId
                     << ", SqlText: " << sqlString;
            SendEventMsg(MSG_RES_STAT_INFO,
                                EVENTLOG_INFORMATION_TYPE,
                                srvrGlobal->nskProcessInfo.processId,
                                ODBCMX_SERVER,
                                srvrGlobal->srvrObjRef,
                                4,
                                srvrGlobal->sessionId,
                                "STATISTICS INFORMATION",
                                "0",
                                ssEvent.str().c_str());
#endif
            delete sqlStatement;
            sqlStatement = NULL;
        }
        else
        {
            strcpy(tmpString, "Error in allocating memory for sqlStatement");
// +++ T2_REPO TODO
/*
            SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                1, tmpString);
*/
            return;
        }

    }
    sequenceNumber = 0;
    totalmsgNumber = 0;
    sprintf(sequenceNumberStr,"%d/%d",sequenceNumber,totalmsgNumber);
    if ((srvrGlobal->resourceStatistics & STMTSTAT_PREPARE) && (inState == STMTSTAT_PREPARE))
    {
        if (inSqlStatement != NULL)
        {
            sqlStatement = new char[strlen(inSqlStatement) + 1];
            strcpy(sqlStatement,inSqlStatement);
        }
        else
        {
            sqlStatement = new char[5];
            strcpy(sqlStatement,"NULL");
        }
        if (sqlStatement != NULL)
        {
#ifdef RES_STATS_EVENT
            stringstream ssEvent;
            ssEvent  << "START:SQLPrepare: StatementId: " << statementId
                     << ", StatementType: " << getStatementType(inSqlNewQueryType);
            SendEventMsg(MSG_RES_STAT_INFO,
                                EVENTLOG_INFORMATION_TYPE,
                                srvrGlobal->nskProcessInfo.processId,
                                ODBCMX_SERVER,
                                srvrGlobal->srvrObjRef,
                                4,
                                srvrGlobal->sessionId,
                                "STATISTICS INFORMATION",
                                "0",
                                ssEvent.str().c_str());
#endif
            delete sqlStatement;
            sqlStatement = NULL;
        }
        else
        {
            strcpy(tmpString, "Error in allocating memory for sqlStatement");
// +++ T2_REPO TODO
/*
            SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                1, tmpString);
*/
            return;
        }
    }

    if (inQueryId == NULL)
        strcpy(queryId, "<NULL>");
    else if (inQueryId[0] == 0)
        strcpy(queryId, "<EMPTY>");
    else
        strcpy(queryId, inQueryId);

//  Always enable 21036
//  if ((srvrGlobal->resourceStatistics & STMTSTAT_EXECUTE) && (inState == STMTSTAT_EXECUTE))
    if (inState == STMTSTAT_EXECUTE)
    {
        estimatedCost = inEstimatedCost;

        if (inSqlStatement != NULL)
        {
            sqlStatement = new char[strlen(inSqlStatement) + 1];
            strcpy(sqlStatement,inSqlStatement);
        }
        else
        {
            sqlStatement = new char[5];
            strcpy(sqlStatement,"NULL");
        }
        if (sqlStatement != NULL)
        {
            if (pSrvrStmt->bLowCost == false)
            {
// +++ T2_REPO - TODO
//              if (queryState == QUERY_EXECUTING)
//                  queryState = QUERY_INIT;
//

                *flag_21036 = true;
                stringstream ss2;
                ss2 << "File: " << __FILE__ << ", Fuction: " << __FUNCTION__ << ", Line: " << __LINE__ << ", QID: " << queryId;
/*              SetQueryStatsInfoSqlText(
                          (const char *)ss2.str().c_str() //"ResStatisticsStatement::start():"
                        , (const char *)queryId
                        , queryStartTime
                        , (const char *)inSqlStatement
                        );
*/
            }
            delete sqlStatement;
            sqlStatement = NULL;
        }
        else
        {
            strcpy(tmpString, "Error in allocating memory for sqlStatement");
// +++ T2_REPO TODO
/*
            SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                1, tmpString);
*/
            return;
        }

        if (srvrGlobal->resourceStatistics & STMTSTAT_EXECUTE)
        {
#ifdef RES_STATS_EVENT
            stringstream ssEvent;
            ssEvent  << "START:SQLExecute: StatementId: " << statementId
                     << ", QueryId: " << queryId;
            SendEventMsg(MSG_RES_STAT_INFO,
                                EVENTLOG_INFORMATION_TYPE,
                                srvrGlobal->nskProcessInfo.processId,
                                ODBCMX_SERVER,
                                srvrGlobal->srvrObjRef,
                                4,
                                srvrGlobal->sessionId,
                                "STATISTICS INFORMATION",
                                "0",
                                ssEvent.str().c_str());
#endif
        }
    }
    if ((srvrGlobal->resourceStatistics & STMTSTAT_FETCH) && (inState == STMTSTAT_FETCH))
    {
#ifdef RES_STATS_EVENT
        stringstream ssEvent;
        ssEvent  << "START:SQLFetch: StatementId: " << statementId
                 << ", QueryId: " << queryId;
        SendEventMsg(MSG_RES_STAT_INFO,
                            EVENTLOG_INFORMATION_TYPE,
                            srvrGlobal->nskProcessInfo.processId,
                            ODBCMX_SERVER,
                            srvrGlobal->srvrObjRef,
                            4,
                            srvrGlobal->sessionId,
                            "STATISTICS INFORMATION",
                            "0",
                            ssEvent.str().c_str());
#endif
    }

}

void ResStatisticsStatement::end(Int32 inState,
                 Int32 inSqlQueryType,
                 short inStmtType,
                 char *inQueryId,
                 double inEstimatedCost,
                 char *inSqlStatement,
                 Int32 inErrorStatement,
                 Int32 inWarningStatement,
                 int64 inRowCount,
                 Int32 inErrorCode,
                 ResStatisticsSession *resStatSession,
                 Int32 inSqlErrorLength,
                 char *inSqlError,
                 SRVR_STMT_HDL *pSrvrStmt,
                 bool *flag_21036,
                 Int32 inSqlNewQueryType,
                 char isClosed)
{
    if (catFlagOn == true) return;
    statementEndTime = JULIANTIMESTAMP();

 // get cpu time (Process_getInfo)
    statementEndCpuTime = getCpuTime();

//Calculate Elapsed and Execution time
    odbcElapseTime = statementEndTime - statementStartTime;
    odbcExecutionTime = statementEndCpuTime - statementStartCpuTime;
    ps.odbcElapseTime = odbcElapseTime;
    ps.odbcExecutionTime = odbcExecutionTime;

    ps.state = inState;
    stmtType = inStmtType;
    ps.stmtType = inStmtType;
    ps.sqlNewQueryType = inSqlNewQueryType;
    estimatedCost = inEstimatedCost;

    if (inQueryId == NULL)
        strcpy(queryId, "<NULL>");
    else if (inQueryId[0] == 0)
        strcpy(queryId, "<EMPTY>");
    else
        strcpy(queryId, inQueryId);

    ps.errorStatement = inErrorStatement;
    ps.warningStatement = inWarningStatement;
    numberOfRows = inRowCount;

    totalStatementOdbcElapseTime = totalStatementOdbcElapseTime + odbcElapseTime;
    totalStatementOdbcExecutionTime = totalStatementOdbcExecutionTime + odbcExecutionTime;

    errorCode = inErrorCode;

    if(inState == STMTSTAT_EXECUTE || inState == STMTSTAT_EXECDIRECT)
        totalStatementExecutes ++;
    else if(inState == STMTSTAT_PREPARE && pSrvrStmt != NULL)
    {
        // now get compile stats from RMS
        setStatistics(pSrvrStmt);
        if (statsErrorCode == 0 and sqlErrorCode == 0)
        {
            prepareStartTime = compStartTime;
            prepareEndTime = compEndTime;
            prepareTime = compTime;
        }
        else
        {
            prepareEndTime = statementEndTime;
            prepareTime = prepareEndTime - prepareStartTime;
        }
    }


    sequenceNumber = 0;
    totalmsgNumber = 0;
    sprintf(sequenceNumberStr,"%d/%d",sequenceNumber,totalmsgNumber);
    if ((srvrGlobal->resourceStatistics & STMTSTAT_PREPARE) && (inState == STMTSTAT_PREPARE))
    {
        if (inSqlStatement != NULL)
        {
            sqlStatement = new char[strlen(inSqlStatement) + 1];
            strcpy(sqlStatement,inSqlStatement);
        }
        else
        {
            sqlStatement = new char[5];
            strcpy(sqlStatement,"NULL");
        }
        if (sqlStatement != NULL)
        {
#ifdef RES_STATS_EVENT
            stringstream ssEvent;
            ssEvent  << "END:SQLPrepare: StatementId: " << statementId
                     << ", QueryId: " << queryId
                     << ", EstimatedCost: " << estimatedCost
                     << ", StatementType: " << getStatementType(inSqlNewQueryType)
                     << ", SQLCompileTime: " << ps.odbcElapseTime
                     << ", ErrorCode: " << errorCode;
            SendEventMsg(MSG_RES_STAT_INFO,
                                EVENTLOG_INFORMATION_TYPE,
                                srvrGlobal->nskProcessInfo.processId,
                                ODBCMX_SERVER,
                                srvrGlobal->srvrObjRef,
                                4,
                                srvrGlobal->sessionId,
                                "STATISTICS INFORMATION",
                                "0",
                                ssEvent.str().c_str());
#endif
            delete sqlStatement;
            sqlStatement = NULL;
        }
        else
        {
            strcpy(tmpString, "Error in allocating memory for sqlStatement");
// +++ T2_REPO TODO
/*
            SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                1, tmpString);
*/
            return;
        }
    }

    // if statement is on
    sequenceNumber = 0;
    totalmsgNumber = 0;
    sprintf(sequenceNumberStr,"%d/%d",sequenceNumber,totalmsgNumber);
    if ((srvrGlobal->resourceStatistics & STMTSTAT_SQL) && (inState == STMTSTAT_PREPARE || inState == STMTSTAT_EXECDIRECT))
    {
        if (inSqlStatement != NULL)
        {
            sqlStatement = new char[strlen(inSqlStatement) + 1];
            strcpy(sqlStatement,inSqlStatement);
        }
        else
        {
            sqlStatement = new char[5];
            strcpy(sqlStatement,"NULL");
        }
        if (sqlStatement != NULL)
        {
#ifdef RES_STATS_EVENT
            int len = min((int)strlen(sqlStatement), 254);
            string sqlString(sqlStatement, len);
            stringstream ssEvent;
            ssEvent  << "END:SQLStatement: StatementId: " << statementId
                     << ", QueryId: " << queryId
                     << ", SqlText: " << sqlString;
            SendEventMsg(MSG_RES_STAT_INFO,
                                EVENTLOG_INFORMATION_TYPE,
                                srvrGlobal->nskProcessInfo.processId,
                                ODBCMX_SERVER,
                                srvrGlobal->srvrObjRef,
                                4,
                                srvrGlobal->sessionId,
                                "STATISTICS INFORMATION",
                                "0",
                                ssEvent.str().c_str());
#endif
            delete sqlStatement;
            sqlStatement = NULL;
        }
        else
        {
            strcpy(tmpString, "Error in allocating memory for sqlStatement");
// +++ T2_REPO TODO
/*
            SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                1, tmpString);
*/
            return;
        }
    }
//  Always enable 21036
//  if ((srvrGlobal->resourceStatistics & STMTSTAT_EXECUTE) && (inState == STMTSTAT_EXECUTE))
    if (inState == STMTSTAT_EXECUTE)
    {

        pSrvrStmt->queryEndTime = statementEndTime;
        pSrvrStmt->queryEndCpuTime = statementEndCpuTime;
        queryElapseTime = statementEndTime - pSrvrStmt->queryStartTime;
        queryExecutionTime = statementEndCpuTime - pSrvrStmt->queryStartCpuTime;

        if (stmtType == TYPE_INSERT || stmtType == TYPE_INSERT_PARAM || stmtType == TYPE_DELETE || stmtType == TYPE_UPDATE || inSqlQueryType == SQL_SELECT_UNIQUE)
        {
#ifdef RES_STATS_EVENT
            stringstream ssEvent;
            ssEvent  << "END:SQLExecute: StatementId: " << statementId
            << ", QueryId: " << queryId
            << ", StatementType: " << getStatementType(inSqlNewQueryType)
            << ", ClientId: " << resCollectinfo.clientId
            << ", UserName: " << resCollectinfo.userName
            << ", UserId: " << resCollectinfo.userId
            << ", ApplicationId: " << resCollectinfo.applicationId
            << ", NodeName: " << resCollectinfo.nodeName
            << ", CpuPin: " << resCollectinfo.cpuPin
            << ", DSName: " << resCollectinfo.DSName
            << ", Time: " << statementEndTime
            << ", ODBCElapsedTime: " << ps.odbcElapseTime
            << ", ODBCExecutionTime: " << ps.odbcExecutionTime
            << ", NumberOfRows: " << numberOfRows
            << ", ErrorCode: " << errorCode
            << ", RowsAccessed: " << AccessedRows
            << ", RowsRetrieved: " << UsedRows
            << ", DiscReads: " << DiskIOs
            << ", MsgsToDisc: " << NumMessages
            << ", MsgsBytesToDisc: " << MessagesBytes
            << ", LockWaits: " << LockWaits
            << ", LockEscalation: " << Escalations
            << ", TotalExecutes: <N/A>"
            ;
            SendEventMsg(MSG_RES_STAT_INFO,
                                EVENTLOG_INFORMATION_TYPE,
                                srvrGlobal->nskProcessInfo.processId,
                                ODBCMX_SERVER,
                                srvrGlobal->srvrObjRef,
                                4,
                                srvrGlobal->sessionId,
                                "STATISTICS INFORMATION",
                                "0",
                                ssEvent.str().c_str());
#endif
        }
        else
        {
            if (srvrGlobal->resourceStatistics & STMTSTAT_EXECUTE)
            {
#ifdef RES_STATS_EVENT
            stringstream ssEvent;
            ssEvent << "END:SQLExecute: StatementId: " << statementId
                    << ", QueryId: " << queryId
                    << ", StatementType: " << getStatementType(inSqlNewQueryType)
                    << ", ClientId: " << resCollectinfo.clientId
                    << ", UserName: " << resCollectinfo.userName
                    << ", UserId: " << resCollectinfo.userId
                    << ", ApplicationId: " << resCollectinfo.applicationId
                    << ", NodeName: " << resCollectinfo.nodeName
                    << ", CpuPin: " << resCollectinfo.cpuPin
                    << ", DSName: " << resCollectinfo.DSName
                    << ", Time: " << statementEndTime
                    << ", ODBCElapsedTime: " << ps.odbcElapseTime
                    << ", ODBCExecutionTime: " << ps.odbcExecutionTime
                    << ", NumberOfRows:<N/A>"
                    << ", ErrorCode: " << errorCode
                    << ", RowsAccessed:<N/A>"
                    << ", RowsRetrieved:<N/A>"
                    << ", DiscReads:<N/A>"
                    << ", MsgsToDisc:<N/A>"
                    << ", MsgsBytesToDisc:<N/A>"
                    << ", LockWaits:<N/A>"
                    << ", LockEscalation:<N/A>"
                    << ", TotalExecutes:<N/A>"
                    ;
                    SendEventMsg(MSG_RES_STAT_INFO,
                                EVENTLOG_INFORMATION_TYPE,
                                srvrGlobal->nskProcessInfo.processId,
                                ODBCMX_SERVER,
                                srvrGlobal->srvrObjRef,
                                4,
                                srvrGlobal->sessionId,
                                "STATISTICS INFORMATION",
                                "0",
                                ssEvent.str().c_str());
#endif
            }
        }
        if (srvrGlobal->resourceStatistics & STMTSTAT_EXECUTE)
        {
        }

//      delay generating 21036 message until qrysrvc_ExecuteFinished is called
    }
//  Always enable 21036
//  if ((srvrGlobal->resourceStatistics & STMTSTAT_FETCH) && (inState == STMTSTAT_FETCH || inState == STMTSTAT_CLOSE))
    if (inState == STMTSTAT_FETCH || inState == STMTSTAT_CLOSE)
    {
        pSrvrStmt->queryEndTime = statementEndTime;
        pSrvrStmt->queryEndCpuTime = statementEndCpuTime;
        queryElapseTime = statementEndTime - pSrvrStmt->queryStartTime;
        queryExecutionTime = statementEndCpuTime - pSrvrStmt->queryStartCpuTime;
        statStatisticsFlag = FALSE;

        if (srvrGlobal->resourceStatistics & STMTSTAT_FETCH)
        {
#ifdef RES_STATS_EVENT
            stringstream ssEvent;
            ssEvent << "END:SQLFetch: StatementId: " << statementId
                    << ", StatementType: " << getStatementType(inSqlNewQueryType)
                    << ", ClientId: " << resCollectinfo.clientId
                    << ", UserName: " << resCollectinfo.userName
                    << ", UserId: " << resCollectinfo.userId
                    << ", ApplicationId: " << resCollectinfo.applicationId
                    << ", NodeName: " << resCollectinfo.nodeName
                    << ", CpuPin: " << resCollectinfo.cpuPin
                    << ", DSName: " << resCollectinfo.DSName
                    << ", Time: " << statementEndTime
                    << ", ODBCElapsedTime: " << totalStatementOdbcElapseTime
                    << ", ODBCExecutionTime: " << totalStatementOdbcElapseTime
                    << ", NumberOfRows: <N/A>"
                    << ", ErrorCode: " << errorCode
                    << ", RowsAccessed: " << AccessedRows
                    << ", RowsRetrieved: " << UsedRows
                    << ", DiscReads: " << DiskIOs
                    << ", MsgsToDisc: " << NumMessages
                    << ", MsgsBytesToDisc: " << MessagesBytes
                    << ", LockWaits: " << LockWaits
                    << ", LockEscalation: " << Escalations
                    << ", TotalExecutes: " << totalStatementExecutes
                    ;
            SendEventMsg(MSG_RES_STAT_INFO,
                                EVENTLOG_INFORMATION_TYPE,
                                srvrGlobal->nskProcessInfo.processId,
                                ODBCMX_SERVER,
                                srvrGlobal->srvrObjRef,
                                4,
                                srvrGlobal->sessionId,
                                "STATISTICS INFORMATION",
                                "0",
                                ssEvent.str().c_str());
#endif
        }
//      delay writing of 21036 msg  to dashboard

    }

//  if ((srvrGlobal->resourceStatistics & STMTSTAT_SQL) || (srvrGlobal->resourceStatistics & STMTSTAT_PREPARE) || (srvrGlobal->resourceStatistics & STMTSTAT_EXECUTE) || (srvrGlobal->resourceStatistics & STMTSTAT_EXECDIRECT) || (srvrGlobal->resourceStatistics & STMTSTAT_FETCH)) // Check for lower bits to enable session
//  to always enable 21036
    if (resStatSession != NULL)
        resStatSession->accumulateStatistics(&ps);
}

static void initSqlStatsItems(SQLSTATS_ITEM *sqlStatsItem, short noOfStatsItem, SQLSTATS_DESC* sqlStatsDesc_)
{
    for (int i = 0; i < noOfStatsItem; i++)
    {
        sqlStatsItem[i].stats_type = sqlStatsDesc_->stats_type;
        sqlStatsItem[i].tdb_id = sqlStatsDesc_->tdb_id;
        sqlStatsItem[i].str_value = NULL;
    }
}

void ResStatisticsStatement::init_rms_counters(bool resetAll)
{
        bzero(parentQID, sizeof(parentQID));
        bzero(childQID, sizeof(childQID));
        bzero(sqlSrc, sizeof(sqlSrc));
        bzero(subQryType, sizeof(subQryType));
        bzero(parentSysName, sizeof(parentSysName));
        sqlSrcLen = 0;

        // row 2 col value resets
        NumMessages = 0;
        MessagesBytes = 0;
        AccessedRows = 0;
        UsedRows = 0;
        DiskIOs = 0;
        Escalations = 0;
        LockWaits = 0;
        Opens = 0;
        OpenTime = 0;
        StatsBytes = 0;
        ProcessBusyTime = 0;
        DiskProcessBusyTime = 0;
        NewProcess = 0;
        NewProcessTime = 0;
        SpaceTotal = 0;
        SpaceUsed = 0;
        HeapTotal = 0;
        HeapUsed = 0;
        CpuTime = 0;
        Dp2SpaceTotal = 0;
        Dp2SpaceUsed = 0;
        Dp2HeapTotal = 0;
        Dp2HeapUsed = 0;

        reqMsgCnt = 0;
        reqMsgBytes = 0;
        replyMsgCnt = 0;
        replyMsgBytes = 0;
        TotalMemAlloc = 0;
        MaxMemUsed = 0;

        // row 1 col value reset
        compStartTime = 0;
        compEndTime = 0;
        compTime = 0;
        exeStartTime = 0;
        exeEndTime = 0;
        exeTime = 0;
        state = 0;
        if( resetAll )
        {
            statsErrorCode = 0;
            rowsAffected = 0;
        }
        sqlErrorCode = 0;
        estRowsAccessed = 0.0f;
        estRowsUsed = 0.0f;
        exeElapsedTime = 0;
        numSqlProcs = 0;
        numCpus = 0;
        sqlSrcLen = 0;
        exePriority = 0;
        rowsReturned = 0;
        NumRowsIUD = 0;
        firstRowReturnTime = 0;

        Timeouts = 0;
        NumSorts = 0;
        SortElapsedTime = 0;
        StatsRowType = 0;
        AQRlastError = 0;
        AQRnumRetries = 0;
        AQRdelayBeforeRetry = 0;
        UdrCpuTime = 0;

        ScratchFileCount = 0;
        ScratchBufferBlockSize = 0;
        ScratchBufferBlocksRead = 0;
        ScratchBufferBlocksWritten = 0;
        ScratchOverflowMode = 0;
        ScratchBufferReadCount = 0;
        ScratchBufferWriteCount = 0;
}

void ResStatisticsStatement::setStatistics(SRVR_STMT_HDL *pSrvrStmt, SQLSTATS_TYPE statsType, char *qID, short qIdLen ,int activeQueryNum)//20111208
{

#define MAX_ACCUMULATED_STATS_DESC  2
#define MAX_PERTABLE_STATS_DESC     30

#define MAX_MASTERSTATS_ENTRY       31
#define MAX_MEASSTATS_ENTRY         30
#define MAX_PERTABLE_ENTRY          10

    int i;
    Int32 cliRC;

    SQLSTATS_DESC* sqlStatsDesc_ = NULL;
    Int32 maxStatsDescEntries_;
    short statsCollectType_;
    Int32 retStatsDescEntries_;

    short currStatsDescEntry_ = 0;
    short currPerTblEntry_ = 0;
    short reqType;
    short mergeType;

    SQLSTATS_ITEM* masterStatsItems_ = NULL;
    SQLSTATS_ITEM* measStatsItems_ = NULL;
    SQLSTATS_ITEM* pertableStatsItems_ = NULL;
    char* qrid_ = NULL;
    char* parentid_ = NULL;
    char* childid_ = NULL;
    char* rmsSqlSource_ = NULL;
    char* tblName_ = NULL;
    char* subQueryType_ = NULL;
    char* parentSystem_ = NULL;
    // To pass either query ID or statement name to CLI
    // Default is CURRENT if pSrvrStmt is NULL.
    //
    char *reqStr = NULL;
    short reqStrLen = 0;
    char *reqStrCurrent = "CURRENT";

    if (!srvrGlobal->m_bStatisticsEnabled)
        return;

    if (pSrvrStmt != NULL)
        pSrvrStmt->m_mxsrvr_substate = NDCS_INIT;

    // Fix for bugzilla #2388
    // For queries having STATS_COLLECTION_TYPE = NO_STATS we should not be calling
    // SQL_EXEC_GetStatistics2() since no stats are collected for these queries. Moreover, for
    // some (NO_STATS) queries like 'get statistics for <qid>' etc. calling SQL_EXEC_GetStatistics2() with
    // reqType=SQLCLI_STATS_REQ_QID_CURRENT will return stats collected for a previously executed query.
    if (statsType == SQLCLI_ACCUMULATED_STATS && pSrvrStmt != NULL)
    {
        // Populate the short query text
        if (pSrvrStmt->comp_stats_info.statsCollectionType == SQLCLI_NO_STATS && pSrvrStmt->comp_stats_info.compilationStats.compilerId[0] != 0)
        {
            if (pSrvrStmt->pSqlString != NULL && pSrvrStmt->m_bNewQueryId == true )
            {
                int len = pSrvrStmt->sqlStringLen > RMS_STORE_SQL_SOURCE_LEN ? RMS_STORE_SQL_SOURCE_LEN : strlen(pSrvrStmt->pSqlString);
                bzero (pSrvrStmt->m_shortQueryText, sizeof(pSrvrStmt->m_shortQueryText));
                pSrvrStmt->m_rmsSqlSourceLen = pSrvrStmt->sqlStringLen;
                translateToUTF8(srvrGlobal->isoMapping, pSrvrStmt->pSqlString, len, pSrvrStmt->m_shortQueryText, RMS_STORE_SQL_SOURCE_LEN);
                pSrvrStmt->m_bNewQueryId = false;
            }
            init_rms_counters();
            return;
        }
    }

    if (qIdLen != 0)
    {
        reqStr = qID;
        reqStrLen = qIdLen;
    }

    switch( statsType )
    {
    case SQLCLI_ACCUMULATED_STATS :
        maxStatsDescEntries_ = MAX_ACCUMULATED_STATS_DESC;
        mergeType = SQLCLI_ACCUMULATED_STATS;
        if( qIdLen !=0 )
            reqType = SQLCLI_STATS_REQ_QID ;
        else
        {
            if (pSrvrStmt != NULL &&
                pSrvrStmt->stmtName != NULL &&
                pSrvrStmt->stmtNameLen > 0)
            {
                reqType = SQLCLI_STATS_REQ_STMT;
                reqStr = pSrvrStmt->stmtName;
                reqStrLen = pSrvrStmt->stmtNameLen;
            }
            else
            {
                reqType = SQLCLI_STATS_REQ_QID_CURRENT;
                reqStr = reqStrCurrent;
                reqStrLen = strlen(reqStrCurrent);
            }
        }
        break;

    case SQLCLI_PERTABLE_STATS :
        maxStatsDescEntries_ = MAX_PERTABLE_STATS_DESC;
        reqType = SQLCLI_STATS_REQ_QID;
        mergeType = SQLCLI_PERTABLE_STATS;
        if( !qID )
            ;   // error
        break;
    }
    try
    {
        bzero(perTableStats, sizeof(perTableStats));
        perTableRowSize = 0;

        sqlStatsDesc_ = new (nothrow) SQLSTATS_DESC[maxStatsDescEntries_];
        if (sqlStatsDesc_ == NULL) { cliRC = 990; throw("error");}

            cliRC = SQL_EXEC_GetStatistics2(
                    reqType,
                    reqStr,
                    reqStrLen,
                    activeQueryNum,
                    mergeType,
                    &statsCollectType_,
                    sqlStatsDesc_,
                    maxStatsDescEntries_,
                    &retStatsDescEntries_);

        if (cliRC != 0) throw("error");
        if (retStatsDescEntries_ <= 0) { cliRC = 991; throw("error");}

        while (currStatsDescEntry_ < retStatsDescEntries_)
        {
            switch (sqlStatsDesc_[currStatsDescEntry_].stats_type)
            {
            case SQLSTATS_DESC_MASTER_STATS:
                masterStatsItems_ = new (nothrow) SQLSTATS_ITEM[MAX_MASTERSTATS_ENTRY];
                if (masterStatsItems_ == NULL) { cliRC = 992; throw ("error");}

                bzero(masterStatsItems_, sizeof(SQLSTATS_ITEM)*MAX_MASTERSTATS_ENTRY);
                initSqlStatsItems(masterStatsItems_, MAX_MASTERSTATS_ENTRY, &sqlStatsDesc_[currStatsDescEntry_]);
                masterStatsItems_[0].statsItem_id = SQLSTATS_QUERY_ID;
                masterStatsItems_[1].statsItem_id = SQLSTATS_COMP_START_TIME;
                masterStatsItems_[2].statsItem_id = SQLSTATS_COMP_END_TIME;
                masterStatsItems_[3].statsItem_id = SQLSTATS_COMP_TIME;
                masterStatsItems_[4].statsItem_id = SQLSTATS_EXECUTE_START_TIME;
                masterStatsItems_[5].statsItem_id = SQLSTATS_FIRST_ROW_RET_TIME;
                masterStatsItems_[6].statsItem_id = SQLSTATS_EXECUTE_END_TIME;
                masterStatsItems_[7].statsItem_id = SQLSTATS_EXECUTE_TIME;
                masterStatsItems_[8].statsItem_id = SQLSTATS_STMT_STATE;
                masterStatsItems_[9].statsItem_id = SQLSTATS_STATS_ERROR_CODE;
                masterStatsItems_[10].statsItem_id = SQLSTATS_SQL_ERROR_CODE;
                masterStatsItems_[11].statsItem_id = SQLSTATS_QUERY_TYPE;
                masterStatsItems_[12].statsItem_id = SQLSTATS_ROWS_RETURNED;
                masterStatsItems_[13].statsItem_id = SQLSTATS_EST_ROWS_ACCESSED;
                masterStatsItems_[14].statsItem_id = SQLSTATS_EST_ROWS_USED;
                masterStatsItems_[15].statsItem_id = SQLSTATS_FIXUP_TIME;
                masterStatsItems_[16].statsItem_id = SQLSTATS_PARENT_QUERY_ID; // new col
                masterStatsItems_[17].statsItem_id = SQLSTATS_NUM_SQLPROCS;
                masterStatsItems_[18].statsItem_id = SQLSTATS_NUM_CPUS;
                masterStatsItems_[19].statsItem_id = SQLSTATS_SOURCE_STR;
                masterStatsItems_[20].statsItem_id = SQLSTATS_SOURCE_STR_LEN;
                masterStatsItems_[21].statsItem_id = SQLSTATS_MASTER_PRIORITY;
                masterStatsItems_[22].statsItem_id = SQLSTATS_TRANSID;
                masterStatsItems_[23].statsItem_id = SQLSTATS_AQR_LAST_ERROR;
                masterStatsItems_[24].statsItem_id = SQLSTATS_AQR_NUM_RETRIES;
                masterStatsItems_[25].statsItem_id = SQLSTATS_AQR_DELAY_BEFORE_RETRY;
                masterStatsItems_[26].statsItem_id = SQLSTATS_ROWS_AFFECTED;
                masterStatsItems_[27].statsItem_id = SQLSTATS_CHILD_QUERY_ID;    // new col
                masterStatsItems_[28].statsItem_id = SQLSTATS_RECLAIM_SPACE_COUNT;
                masterStatsItems_[29].statsItem_id = SQLSTATS_SUBQUERY_TYPE;
                masterStatsItems_[30].statsItem_id = SQLSTATS_PARENT_QUERY_SYSTEM;

                // MAX_MASTERSTATS_ENTRY is set to 31
                qrid_ = masterStatsItems_[0].str_value = new (nothrow) char[MAX_QUERY_ID_LEN+1];
                if (qrid_ == NULL) { cliRC = 993; throw ("error");}
                bzero(qrid_, MAX_QUERY_ID_LEN+1);
                masterStatsItems_[0].str_max_len = MAX_QUERY_ID_LEN;

                parentid_ = masterStatsItems_[16].str_value = new (nothrow) char[MAX_QUERY_ID_LEN+1];
                if (parentid_ == NULL) { cliRC = 994; throw ("error");}
                bzero(parentid_, MAX_QUERY_ID_LEN+1 );
                masterStatsItems_[16].str_max_len = MAX_QUERY_ID_LEN;

                childid_ = masterStatsItems_[27].str_value = new (nothrow) char[MAX_QUERY_ID_LEN+1];
                if (childid_ == NULL) { cliRC = 995; throw ("error");}
                bzero(childid_, MAX_QUERY_ID_LEN+1);
                masterStatsItems_[27].str_max_len = MAX_QUERY_ID_LEN;

                rmsSqlSource_ = masterStatsItems_[19].str_value = new (nothrow) char[RMS_STORE_SQL_SOURCE_LEN+2];
                if (rmsSqlSource_ == NULL) { cliRC = 996; throw ("error");}
                bzero(rmsSqlSource_, RMS_STORE_SQL_SOURCE_LEN+2);
                masterStatsItems_[19].str_max_len = RMS_STORE_SQL_SOURCE_LEN;

                subQueryType_ = masterStatsItems_[29].str_value = new (nothrow) char[SUB_QRY_TYPE_LEN+1];
                if (subQueryType_ == NULL) { cliRC = 1000; throw ("error");}
                bzero(subQueryType_, SUB_QRY_TYPE_LEN+1);
                masterStatsItems_[29].str_max_len = SUB_QRY_TYPE_LEN;

                parentSystem_ = masterStatsItems_[30].str_value = new (nothrow) char[PAR_SYS_NAME_LEN+1];
                if (parentSystem_ == NULL) { cliRC = 1001; throw ("error");}
                bzero(parentSystem_, PAR_SYS_NAME_LEN+1);
                masterStatsItems_[30].str_max_len = PAR_SYS_NAME_LEN;

                cliRC = SQL_EXEC_GetStatisticsItems(
                    reqType,
                    reqStr,
                    reqStrLen,
                    MAX_MASTERSTATS_ENTRY,
                    masterStatsItems_);
                if (cliRC != 0) throw("error");

                break;

            case SQLSTATS_DESC_MEAS_STATS:
                measStatsItems_ = new (nothrow) SQLSTATS_ITEM[MAX_MEASSTATS_ENTRY];
                if (measStatsItems_ == NULL) { cliRC = 997; throw ("error");}

                bzero(measStatsItems_, sizeof(SQLSTATS_ITEM)*MAX_MEASSTATS_ENTRY);

                initSqlStatsItems(measStatsItems_, MAX_MEASSTATS_ENTRY, &sqlStatsDesc_[currStatsDescEntry_]);
                measStatsItems_[0].statsItem_id = SQLSTATS_ACT_ROWS_ACCESSED;
                measStatsItems_[1].statsItem_id = SQLSTATS_ACT_ROWS_USED;
                measStatsItems_[2].statsItem_id = SQLSTATS_SE_IOS;
                measStatsItems_[3].statsItem_id = SQLSTATS_SE_IO_BYTES;
                measStatsItems_[4].statsItem_id = SQLSTATS_SE_IO_MAX_TIME;
                measStatsItems_[5].statsItem_id = SQLSTATS_SQL_CPU_BUSY_TIME;
                measStatsItems_[6].statsItem_id = SQLSTATS_SQL_SPACE_ALLOC;
                measStatsItems_[7].statsItem_id = SQLSTATS_SQL_SPACE_USED;
                measStatsItems_[8].statsItem_id = SQLSTATS_SQL_HEAP_ALLOC;
                measStatsItems_[9].statsItem_id = SQLSTATS_SQL_HEAP_USED;
                measStatsItems_[10].statsItem_id = SQLSTATS_SQL_HEAP_WM;
                measStatsItems_[11].statsItem_id = SQLSTATS_OPENS;
                measStatsItems_[12].statsItem_id = SQLSTATS_OPEN_TIME;
                measStatsItems_[13].statsItem_id = SQLSTATS_PROCESS_CREATED;
                measStatsItems_[14].statsItem_id = SQLSTATS_PROCESS_CREATE_TIME;
                measStatsItems_[15].statsItem_id = SQLSTATS_REQ_MSG_CNT;
                measStatsItems_[16].statsItem_id = SQLSTATS_REQ_MSG_BYTES;
                measStatsItems_[17].statsItem_id = SQLSTATS_REPLY_MSG_CNT;
                measStatsItems_[18].statsItem_id = SQLSTATS_REPLY_MSG_BYTES;
                measStatsItems_[19].statsItem_id = SQLSTATS_SCRATCH_OVERFLOW_MODE;
                measStatsItems_[20].statsItem_id = SQLSTATS_SCRATCH_FILE_COUNT;
                measStatsItems_[21].statsItem_id = SQLSTATS_BMO_SPACE_BUFFER_SIZE;
                measStatsItems_[22].statsItem_id = SQLSTATS_BMO_SPACE_BUFFER_COUNT;
                measStatsItems_[23].statsItem_id = SQLSTATS_SCRATCH_IO_SIZE;
                measStatsItems_[24].statsItem_id = SQLSTATS_SCRATCH_READ_COUNT;
                measStatsItems_[25].statsItem_id = SQLSTATS_SCRATCH_WRITE_COUNT;
                measStatsItems_[26].statsItem_id = SQLSTATS_SCRATCH_IO_MAX_TIME;
                measStatsItems_[27].statsItem_id = SQLSTATS_INTERIM_ROW_COUNT;
                measStatsItems_[28].statsItem_id = SQLSTATS_TOPN;
                measStatsItems_[29].statsItem_id = SQLSTATS_UDR_CPU_BUSY_TIME;
                // MAX_MEASSTATS_ENTRY is set to 30

                cliRC = SQL_EXEC_GetStatisticsItems(
                    reqType,
                    reqStr,
                    reqStrLen,
                    MAX_MEASSTATS_ENTRY,
                    measStatsItems_);
                if (cliRC != 0) throw("error");

                break;
            case SQLSTATS_DESC_ROOT_OPER_STATS:
                break;
            case SQLSTATS_DESC_PERTABLE_STATS:

                if( !pertableStatsItems_ ) {
                    pertableStatsItems_ = new (nothrow) SQLSTATS_ITEM[MAX_PERTABLE_ENTRY];
                    if (pertableStatsItems_ == NULL) { cliRC = 998; throw ("error");}
                }

                initSqlStatsItems(pertableStatsItems_, MAX_PERTABLE_ENTRY, &sqlStatsDesc_[currStatsDescEntry_]);
                pertableStatsItems_[0].statsItem_id = SQLSTATS_TABLE_ANSI_NAME;
                pertableStatsItems_[1].statsItem_id = SQLSTATS_EST_ROWS_ACCESSED;
                pertableStatsItems_[2].statsItem_id = SQLSTATS_EST_ROWS_USED;
                pertableStatsItems_[3].statsItem_id = SQLSTATS_ACT_ROWS_ACCESSED;
                pertableStatsItems_[4].statsItem_id = SQLSTATS_ACT_ROWS_USED;
                pertableStatsItems_[5].statsItem_id = SQLSTATS_SE_IOS;
                pertableStatsItems_[6].statsItem_id = SQLSTATS_SE_IO_BYTES;
                pertableStatsItems_[7].statsItem_id = SQLSTATS_SE_IO_MAX_TIME;
                pertableStatsItems_[8].statsItem_id = SQLSTATS_OPENS;
                pertableStatsItems_[9].statsItem_id = SQLSTATS_OPEN_TIME;

                if( pertableStatsItems_[0].str_value == NULL ) {
                    tblName_ = pertableStatsItems_[0].str_value = new char[MAX_SQL_IDENTIFIER_LEN+1];
                    if (tblName_ == NULL) { cliRC = 999; throw ("error");}
                    pertableStatsItems_[0].str_max_len = MAX_SQL_IDENTIFIER_LEN+1;
                }
                bzero(pertableStatsItems_[0].str_value, MAX_SQL_IDENTIFIER_LEN+1);

                // MAX_PERTABLE_ENTRY is set to 10

                cliRC = SQL_EXEC_GetStatisticsItems(
                    reqType,
                    reqStr,
                    reqStrLen,
                    MAX_PERTABLE_ENTRY,
                    pertableStatsItems_);
                if (cliRC != 0) throw("error");

                //
                for (i = 0; i < MAX_PERTABLE_ENTRY; i++)
                {
                    if (pertableStatsItems_[i].error_code != 0) continue;
                    switch (pertableStatsItems_[i].statsItem_id)
                    {
                    case SQLSTATS_TABLE_ANSI_NAME:
                        memcpy( perTableStats[currPerTblEntry_].tblName,
                                    pertableStatsItems_[i].str_value,
                                    pertableStatsItems_[i].str_ret_len);
                        perTableStats[currPerTblEntry_].tblName[pertableStatsItems_[i].str_ret_len] = '\x0';
                        break;
                    case SQLSTATS_EST_ROWS_ACCESSED:
                        perTableStats[currPerTblEntry_].estAccessedRows = pertableStatsItems_[i].double_value;
                        break;
                    case SQLSTATS_EST_ROWS_USED:
                        perTableStats[currPerTblEntry_].estUsedRows = pertableStatsItems_[i].double_value;
                        break;
                    case SQLSTATS_ACT_ROWS_ACCESSED:
                        perTableStats[currPerTblEntry_].accessedRows = pertableStatsItems_[i].int64_value;
                        break;
                    case SQLSTATS_ACT_ROWS_USED:
                        perTableStats[currPerTblEntry_].usedRows = pertableStatsItems_[i].int64_value;
                        break;
                    case SQLSTATS_SE_IOS:
                        perTableStats[currPerTblEntry_].diskIOs = pertableStatsItems_[i].int64_value;
                        break;
                    case SQLSTATS_SE_IO_BYTES:
                        perTableStats[currPerTblEntry_].messagesBytes = pertableStatsItems_[i].int64_value;
                        break;
                    case SQLSTATS_SE_IO_MAX_TIME:
                        perTableStats[currPerTblEntry_].dp2BusyTime = pertableStatsItems_[i].int64_value;
                        break;
                    case SQLSTATS_OPENS:
                        perTableStats[currPerTblEntry_].opens = pertableStatsItems_[i].int64_value;
                        break;
                    case SQLSTATS_OPEN_TIME:
                        perTableStats[currPerTblEntry_].openTime = pertableStatsItems_[i].int64_value;
                        break;
                    default:
                        break;
                    }
                }
                currPerTblEntry_++;
                break;
            default:
                break;
            }

            currStatsDescEntry_++;
        }

        perTableRowSize = currPerTblEntry_;
        Int32 len;

        if (masterStatsItems_ != NULL)
            for (i = 0; i < MAX_MASTERSTATS_ENTRY; i++)
            {
                if (masterStatsItems_[i].error_code != 0) continue;
                switch (masterStatsItems_[i].statsItem_id)
                {
                case SQLSTATS_QUERY_ID:             //char*
                    masterStatsItems_[i].str_value[masterStatsItems_[i].str_ret_len] = '\0';
                    break;
                case SQLSTATS_COMP_START_TIME:      //int64
                    compStartTime = masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_COMP_END_TIME:        //int64
                    compEndTime = masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_COMP_TIME:            //int64
                    compTime = masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_EXECUTE_START_TIME:   //int64
                    exeStartTime = masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_FIRST_ROW_RET_TIME:   //int64
                    firstRowReturnTime = masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_EXECUTE_END_TIME:     //int64
                    exeEndTime = masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_EXECUTE_TIME:         //int64
                    exeElapsedTime = masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_STMT_STATE:           //int64
                    state = masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_ROWS_RETURNED:        //int64
                    rowsReturned = masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SQL_ERROR_CODE:       //int64
                    sqlErrorCode = masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_STATS_ERROR_CODE:     //int64
                    statsErrorCode = masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_QUERY_TYPE:           //int64
                    break;
                case SQLSTATS_EST_ROWS_ACCESSED:    //double
                    estRowsAccessed = masterStatsItems_[i].double_value;
                    break;
                case SQLSTATS_EST_ROWS_USED:        //double
                    estRowsUsed = masterStatsItems_[i].double_value;
                    break;
                case SQLSTATS_FIXUP_TIME:
                    break;
                case SQLSTATS_PARENT_QUERY_ID:
                    strncpy(parentQID, masterStatsItems_[i].str_value, masterStatsItems_[i].str_ret_len);
                    parentQID[masterStatsItems_[i].str_ret_len]='\0';
                    break;
                case SQLSTATS_NUM_SQLPROCS:
                    numSqlProcs = masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_NUM_CPUS:
                    numCpus = masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SOURCE_STR:
                    strncpy(sqlSrc, masterStatsItems_[i].str_value, masterStatsItems_[i].str_ret_len);
                    sqlSrc[masterStatsItems_[i].str_ret_len]='\0';
                    break;
                case SQLSTATS_SOURCE_STR_LEN:
                    sqlSrcLen = masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_MASTER_PRIORITY:
                    exePriority = masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_TRANSID:
                    transIDnum = masterStatsItems_[i].int64_value;
                    if (transIDnum > 0)
                        _i64toa(transIDnum, transID, 10);
                    else {
                        memset(transID, '\0', MAX_TXN_STR_LEN + 1);
                            strcpy(transID,"<N/A>");
                    }

//                  len = TransIdToText(masterStatsItems_[i].int64_value, transID, (short)(sizeof(transID)));
//                  if (len > 0)
//                  transID[len] = '\0';

                    break;
                case SQLSTATS_AQR_LAST_ERROR:
                    AQRlastError = (short)masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_AQR_NUM_RETRIES:
                    AQRnumRetries = (short)masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_AQR_DELAY_BEFORE_RETRY:
                    AQRdelayBeforeRetry = (short)masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_ROWS_AFFECTED:
                    NumRowsIUD = masterStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_CHILD_QUERY_ID:
                    strncpy(childQID, masterStatsItems_[i].str_value, masterStatsItems_[i].str_ret_len);
                    childQID[masterStatsItems_[i].str_ret_len]='\0';
                    break;
                case SQLSTATS_RECLAIM_SPACE_COUNT:
                    break;
                case SQLSTATS_SUBQUERY_TYPE:
                    char tmpSubQry[2];
                    memset(tmpSubQry,'\0',2);
                    tmpSubQry[0]=masterStatsItems_[i].str_value[0];
                    if( strncmp(tmpSubQry,"2",1) ==0)
                        strcpy(subQryType,"SQL_STMT_REPLICATE");
                    else if( strncmp(tmpSubQry,"1",1)==0 )
                        strcpy(subQryType,"SQL_STMT_CTAS");
                    else
                        strcpy(subQryType,"SQL_STMT_NA");
                    break;
                case SQLSTATS_PARENT_QUERY_SYSTEM:
                    strncpy(parentSysName, masterStatsItems_[i].str_value, masterStatsItems_[i].str_ret_len);
                    parentSysName[masterStatsItems_[i].str_ret_len]='\0';
                    break;
                default:
                    break;
                }
            }

        if (measStatsItems_ != NULL)
        {
            for (i = 0; i < MAX_MEASSTATS_ENTRY; i++)
            {
                if (measStatsItems_[i].error_code != 0) continue;
                switch (measStatsItems_[i].statsItem_id)
                {
                case SQLSTATS_ACT_ROWS_ACCESSED:        //int64
                    AccessedRows = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_ACT_ROWS_USED:            //int64
                    UsedRows = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SE_IOS:                //int64
                    NumMessages = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SE_IO_BYTES:                //int64
                    MessagesBytes = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SE_IO_MAX_TIME:        //int64
                    DiskProcessBusyTime = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SQL_CPU_BUSY_TIME:        //int64
                    ProcessBusyTime = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SQL_SPACE_ALLOC:          //int64
                    SpaceTotal = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SQL_SPACE_USED:           //int64
                    SpaceUsed = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SQL_HEAP_ALLOC:           //int64
                    HeapTotal = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SQL_HEAP_USED:            //int64
                    HeapUsed = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SQL_HEAP_WM:            //int64
                    HeapWM = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_OPENS:                    //int64
                    Opens = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_OPEN_TIME:                //int64
                    OpenTime = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_PROCESS_CREATED:          //int64
                    NewProcess = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_PROCESS_CREATE_TIME:      //int64
                    NewProcessTime = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_REQ_MSG_CNT:          //int64
                    reqMsgCnt = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_REQ_MSG_BYTES:            //int64
                    reqMsgBytes = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_REPLY_MSG_CNT:            //int64
                    replyMsgCnt = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_REPLY_MSG_BYTES:          //int64
                    replyMsgBytes = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SCRATCH_FILE_COUNT:
                    ScratchFileCount = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_BMO_SPACE_BUFFER_SIZE:
                    bmoSpaceBufferSize = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_BMO_SPACE_BUFFER_COUNT:
                    bmoSpaceBufferCount = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SCRATCH_IO_SIZE:
                    ScratchIOSize = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SCRATCH_OVERFLOW_MODE:
                    ScratchOverflowMode = (short)measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SCRATCH_READ_COUNT:
                    ScratchBufferReadCount = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SCRATCH_WRITE_COUNT:
                    ScratchBufferWriteCount = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_SCRATCH_IO_MAX_TIME:
                    ScratchIOMaxTime = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_INTERIM_ROW_COUNT:
                    bmoInterimRowCount = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_TOPN:
                    topN = measStatsItems_[i].int64_value;
                    break;
                case SQLSTATS_UDR_CPU_BUSY_TIME:
                    UdrCpuTime = measStatsItems_[i].int64_value;
                    break;
                default:
                    break;
                }
            }
// +++ T2_REPO ToDo - Scartch file currently not supported ??
/*
            if (pSrvrStmt != NULL)
            {
                pSrvrStmt->m_execOverflow.m_OvfFileCount = ScratchFileCount;
                pSrvrStmt->m_execOverflow.m_OvfSpaceAllocated = (ScratchFileCount * 2 * ONE_GB) / ONE_KB;
                pSrvrStmt->m_execOverflow.m_OvfSpaceUsed = ScratchBufferBlocksWritten * ScratchBufferBlockSize;
                pSrvrStmt->m_execOverflow.m_OvfBlockSize = ScratchBufferBlockSize;
                pSrvrStmt->m_execOverflow.m_OvfIOs = ScratchBufferReadCount + ScratchBufferWriteCount;
                pSrvrStmt->m_execOverflow.m_OvfMessageBuffersTo = ScratchBufferBlocksWritten;
                pSrvrStmt->m_execOverflow.m_OvfMessageTo = ScratchBufferWriteCount;
                pSrvrStmt->m_execOverflow.m_OvfMessageBytesTo = ScratchBufferBlocksWritten * ScratchBufferBlockSize;
                pSrvrStmt->m_execOverflow.m_OvfMessageBuffersFrom = ScratchBufferBlocksRead;
                pSrvrStmt->m_execOverflow.m_OvfMessageFrom = ScratchBufferReadCount;
                pSrvrStmt->m_execOverflow.m_OvfMessageBytesFrom = ScratchBufferBlocksRead * ScratchBufferBlockSize;
            }
*/
            TotalMemAlloc = SpaceUsed + HeapUsed;
            MaxMemUsed = SpaceTotal + HeapTotal +  Dp2SpaceTotal + Dp2HeapTotal;

        }
    }
//LCOV_EXCL_START
    catch(...)
    {
        statsErrorCode = cliRC;
        if (pSrvrStmt != NULL)
            pSrvrStmt->m_mxsrvr_substate = NDCS_RMS_ERROR;

        SQL_EXEC_ClearDiagnostics(NULL);

        // Fix for query disappearing when the below CQD is issued
        // 'CQD detailed_statistics 'ALL' CR??
        init_rms_counters();
    }
//LCOV_EXCL_STOP
    if (pSrvrStmt != NULL)
        if (rmsSqlSource_ != NULL && pSrvrStmt->m_bNewQueryId == true )
        {
            bzero (pSrvrStmt->m_shortQueryText, sizeof(pSrvrStmt->m_shortQueryText));
            pSrvrStmt->m_rmsSqlSourceLen = sqlSrcLen + 1;
            translateToUTF8(srvrGlobal->isoMapping, rmsSqlSource_, sqlSrcLen + 1, pSrvrStmt->m_shortQueryText, RMS_STORE_SQL_SOURCE_LEN);
            pSrvrStmt->m_bNewQueryId = false;
        }
    if (sqlStatsDesc_ != NULL) delete[] sqlStatsDesc_;
    if (masterStatsItems_ != NULL) delete[] masterStatsItems_;
    if (measStatsItems_ != NULL) delete[] measStatsItems_;
    if (qrid_ != NULL) delete[] qrid_;
    if (parentid_ != NULL) delete[] parentid_;
    if (childid_ != NULL) delete[] childid_;
    if (rmsSqlSource_ != NULL) delete[] rmsSqlSource_;
    if (pertableStatsItems_ != NULL) delete[] pertableStatsItems_;
    if (tblName_ != NULL) delete[] tblName_;
    if (subQueryType_ != NULL) delete[] subQueryType_;
    if (parentSystem_ != NULL) delete[] parentSystem_;

}

// Single Row Per Query Initiative -
//
//    generate the delayed 21036 message to repository
//

void ResStatisticsStatement::endRepository(SRVR_STMT_HDL *pSrvrStmt,
                    Int32 sqlWarningOrErrorLength,
                    BYTE *sqlWarningOrError,
                    bool bClose_Fetch)
{

    SQL_COMPILATION_STATS_DATA comp_stats_data;
    /*
     if (*flag_21036 == false) // do not generate message
        return;
    */
    if (pSrvrStmt == NULL ||
        pSrvrStmt->sqlUniqueQueryID == NULL ||
        pSrvrStmt->sqlUniqueQueryID[0] == '\0' )
        return;

    if (pSrvrStmt->m_need_21036_end_msg == false) // do not generate message
        return;

    if (pSrvrStmt->bLowCost == true) // Low Cost - do not generate message. Should not reach here if above is false.
        return;
//
// AGGREGATION----------------endRepository EXECUTE
//
    pSrvrStmt->m_lastQueryEndTime = statementEndTime;
    pSrvrStmt->m_lastQueryEndCpuTime = statementEndCpuTime;

    comp_stats_data = pSrvrStmt->comp_stats_info.compilationStats;
    if (comp_stats_data.compilerId[0] == 0)
        strcpy(comp_stats_data.compilerId,"<N/A>");

    if (srvrGlobal->m_statisticsPubType == STATISTICS_AGGREGATED)
    {
        if (resStatSession != NULL)
            resStatSession->accumulateStatistics(this);
    }

    if (pSrvrStmt->m_bqueryFinish == false) // do not generate message
        return;

    Int32  inState        = pSrvrStmt->inState,
          inSqlQueryType = pSrvrStmt->sqlQueryType;
    char *sqlString      = pSrvrStmt->pSqlString;
    bool  isClosed       = pSrvrStmt->isClosed;
    bool newStmt = false;
    SQL_QUERY_COST_INFO cost_info = pSrvrStmt->cost_info;

    unsigned short queryState = pSrvrStmt->m_state;
    UInt32 maxMemUsed = pSrvrStmt->m_maxMemUsed;
    unsigned short warnLevel = pSrvrStmt->m_warnLevel;

    int64 execTime           = pSrvrStmt->m_exec_time,
          holdTime           = pSrvrStmt->m_hold_time + pSrvrStmt->m_suspended_time,
          suspendTime        = pSrvrStmt->m_suspended_time,
          waitTime           = pSrvrStmt->m_wait_time,
          WMSstartTS         = pSrvrStmt->m_WMSstart_ts;

    memcpy(con_rule_name, pSrvrStmt->m_con_rule_name, sizeof(con_rule_name));
    memcpy(cmp_rule_name, pSrvrStmt->m_cmp_rule_name, sizeof(cmp_rule_name));
    memcpy(exe_rule_name, pSrvrStmt->m_exe_rule_name, sizeof(exe_rule_name));

    int64 inexeEndTime = exeEndTime; // SQLSTATS_EXECUTE_END_TIME
    int64 inqueryElapseTime = 0;
    int64 inqueryExecutionTime = 0;

    char *pBuffer = msgBuffer;

    if (memcmp(queryId, pSrvrStmt->sqlUniqueQueryID, pSrvrStmt->sqlUniqueQueryIDLen) != 0)
        newStmt = true;

    if (pSrvrStmt->queryStartTime == 0)
        inqueryElapseTime = pSrvrStmt->queryEndTime - statementStartTime;
    else
        inqueryElapseTime = pSrvrStmt->queryEndTime - pSrvrStmt->queryStartTime;

    if (pSrvrStmt->queryStartCpuTime == 0)
        inqueryExecutionTime = pSrvrStmt->queryEndCpuTime - statementStartCpuTime;
    else
        inqueryExecutionTime = pSrvrStmt->queryEndCpuTime - pSrvrStmt->queryStartCpuTime;

    if (inqueryElapseTime <= 0)
    {
        statementEndTime = JULIANTIMESTAMP();
        statementEndCpuTime = getCpuTime();
        inqueryElapseTime = statementEndTime - pSrvrStmt->queryStartTime;
        inqueryExecutionTime = statementEndCpuTime - pSrvrStmt->queryStartCpuTime;
    }

    if (inexeEndTime <= 0)
        inexeEndTime = statementEndTime;

    if (pSrvrStmt->m_maxMemUsed > MaxMemUsed)
        MaxMemUsed = pSrvrStmt->m_maxMemUsed;

    if (inState == STMTSTAT_EXECUTE) {
        // log the end message
        if ((stmtType != TYPE_SELECT && isClosed == TRUE)
            || (inSqlQueryType == SQL_SELECT_UNIQUE)
            || (errorCode != SQL_SUCCESS && errorCode != SQL_SUCCESS_WITH_INFO)) {
            if (stmtType == TYPE_INSERT || stmtType == TYPE_INSERT_PARAM || stmtType == TYPE_DELETE || stmtType == TYPE_UPDATE || inSqlQueryType == SQL_SELECT_UNIQUE) {

                string querySubstate;

                if (queryState == QUERY_INIT || queryState == QUERY_COMPLETED)
                {
                    queryState = QUERY_COMPLETED;

                        querySubstate = pSrvrStmt->m_mxsrvr_substate == NDCS_INIT? getQuerySubStateStringRes(pSrvrStmt->m_state) : getSrvrSubstate(pSrvrStmt->m_mxsrvr_substate);
                }
                else
                    querySubstate = getQuerySubStateStringRes(queryState);

                stringstream ss;
                ss << "File: " << __FILE__ << ", Fuction: " << __FUNCTION__ << ", Line: " << __LINE__ << ", QID: " << pSrvrStmt->sqlUniqueQueryID;
// +++ T2_REPO ToDo - Scartch file currently not supported ??
//                EXEC_OVERFLOW execOverflow = { 0 };
//                memcpy(&execOverflow, &pSrvrStmt->m_execOverflow, sizeof(EXEC_OVERFLOW));
//
                // Send query end message
                if (srvrGlobal->m_bStatisticsEnabled && ((srvrGlobal->m_statisticsPubType == STATISTICS_QUERY) || pubStarted))
                    SendQueryStats(false, pSrvrStmt, (char *)sqlWarningOrError, sqlWarningOrErrorLength);
            }
            else { // non unique select error condition

                string querySubstate;

                if (queryState == QUERY_INIT || queryState == QUERY_COMPLETED)
                {
                    queryState = QUERY_COMPLETED;
                        querySubstate = pSrvrStmt->m_mxsrvr_substate == NDCS_INIT? getQuerySubStateStringRes(pSrvrStmt->m_state) : getSrvrSubstate(pSrvrStmt->m_mxsrvr_substate);
                }
                else
                    querySubstate = getQuerySubStateStringRes(queryState);

                stringstream ss;
                ss << "File: " << __FILE__ << ", Fuction: " << __FUNCTION__ << ", Line: " << __LINE__ << ", QID: " << pSrvrStmt->sqlUniqueQueryID;
// +++ T2_REPO ToDo - Scartch file currently not supported ??
//                EXEC_OVERFLOW execOverflow = { 0 };
//                memcpy(&execOverflow, &pSrvrStmt->m_execOverflow, sizeof(EXEC_OVERFLOW));
//
                // Send query end message
                if (srvrGlobal->m_bStatisticsEnabled && ((srvrGlobal->m_statisticsPubType == STATISTICS_QUERY) || pubStarted))
                    SendQueryStats(false, pSrvrStmt, (char *)sqlWarningOrError, sqlWarningOrErrorLength);
            }
            /* *flag_21036 = false; */
            pSrvrStmt->m_need_21036_end_msg = false;
    }
}


    if ((inState == STMTSTAT_FETCH) || inState == STMTSTAT_CLOSE)
    {
        string querySubstate;

        if (queryState == QUERY_INIT || queryState == QUERY_COMPLETED)
        {
            queryState = QUERY_COMPLETED;
                querySubstate = pSrvrStmt->m_mxsrvr_substate == NDCS_INIT? getQuerySubStateStringRes(pSrvrStmt->m_state) : getSrvrSubstate(pSrvrStmt->m_mxsrvr_substate);
        }
        else
        {
            querySubstate = getQuerySubStateStringRes(queryState);
        }

        stringstream ss;
        ss << "File: " << __FILE__ << ", Fuction: " << __FUNCTION__ << ", Line: " << __LINE__ << ", QID: " << pSrvrStmt->sqlUniqueQueryID;
// +++ T2_REPO ToDo - Scartch file currently not supported ??
//        EXEC_OVERFLOW execOverflow = { 0 };
//        memcpy(&execOverflow, &pSrvrStmt->m_execOverflow, sizeof(EXEC_OVERFLOW));
//
        // Send query end message
        if (srvrGlobal->m_bStatisticsEnabled && ((srvrGlobal->m_statisticsPubType == STATISTICS_QUERY) || pubStarted))
            SendQueryStats(false, pSrvrStmt, (char *)sqlWarningOrError, sqlWarningOrErrorLength);

        /* *flag_21036 = false; */
        pSrvrStmt->m_need_21036_end_msg = false;
    }
}

//
// Single Row Per Query initiative -
//
//    WMS returned error on DO_WouldLikeToExecute (after Prepare), we still need to generate
//    21036 start and end message for Repository.
//
//    this funtion takes care of a) start message b) get the stats from RMS
//
void ResStatisticsStatement::toRepository(SRVR_STMT_HDL *pSrvrStmt,
                    Int32 sqlWarningOrErrorLength,
                    BYTE *sqlWarningOrError)
{
    Int32  inState                 = pSrvrStmt->inState,
          inSqlNewQueryType          = pSrvrStmt->sqlNewQueryType;
    char *inSqlStatement          = pSrvrStmt->pSqlString;
    SQL_QUERY_COST_INFO cost_info = pSrvrStmt->cost_info;
    SQL_QUERY_COMPILER_STATS_INFO comp_stats_info = pSrvrStmt->comp_stats_info;
    SQL_COMPILATION_STATS_DATA comp_stats_data = comp_stats_info.compilationStats;

    unsigned short queryState     = pSrvrStmt->m_state;
    unsigned short warnLevel      = pSrvrStmt->m_warnLevel;

    int64 holdTime                = pSrvrStmt->m_hold_time,
          waitTime                = pSrvrStmt->m_wait_time,
          WMSstartTS              = pSrvrStmt->m_WMSstart_ts;

    memcpy(con_rule_name, pSrvrStmt->m_con_rule_name, sizeof(con_rule_name));
    memcpy(cmp_rule_name, pSrvrStmt->m_cmp_rule_name, sizeof(cmp_rule_name));
    memcpy(exe_rule_name, pSrvrStmt->m_exe_rule_name, sizeof(exe_rule_name));

    char *pBuffer = msgBuffer;
//
// AGGREGATION----------------toRepository
//

    if (comp_stats_info.dop > 1)
        estTotalMem = cost_info.estimatedTotalMem * comp_stats_info.dop;
    else
        estTotalMem = cost_info.estimatedTotalMem;

    if (pSrvrStmt->queryStartTime > 0)
    {
        queryStartTime    = pSrvrStmt->queryStartTime;
        queryStartCpuTime = pSrvrStmt->queryStartCpuTime;
    }

    strcpy(queryId, pSrvrStmt->sqlUniqueQueryID);
    if (comp_stats_data.compilerId[0] == 0)
        strcpy(comp_stats_data.compilerId,"<N/A>");
    //if (comp_stats_data.compileInfoLen <= 0 || comp_stats_data.compileInfo[0] == 0)
    //  strcpy(comp_stats_data.compileInfo,"<N/A>");

    if (inSqlStatement != NULL && pSrvrStmt->bLowCost == false) {
        // log the start message 21036
        stringstream ss;
        ss << "File: " << __FILE__ << ", Fuction: " << __FUNCTION__ << ", Line: " << __LINE__ << ", QID: " << queryId;
/*      SetQueryStatsInfoSqlText(
              (const char *)ss.str().c_str() //"ResStatisticsStatement::toRepository()"
            , (const char *)queryId
            , queryStartTime
            , (const char *)inSqlStatement
            );
*/
        pSrvrStmt->m_need_21036_end_msg = true;

        // now get stats from RMS
//      setStatistics();

    }

}

inline string ResStatisticsStatement::getErrorText(char *inSqlError, size_t inSqlErrorLength, size_t inMaxSqlErrorLength)
{
    string s1;

    if (inSqlErrorLength > 0 && inSqlError != NULL)
    {
        err_desc_def *error = (err_desc_def*)inSqlError;
        s1.assign(inSqlError + 16, _min(inMaxSqlErrorLength, error->length-1));
    }
    else
        s1.assign("<N/A>");

    return s1;
}

void ResStatisticsStatement::appendErrorText(char *inSqlError, Int32 inSqlErrorLength)
{
    int length  = 0;
    int strSize = 0;

        err_desc_def *error;

    error = (err_desc_def*)inSqlError;

    strcat(msgInfo, " ErrorText:");

    if ((inSqlErrorLength > 0) && (inSqlError != NULL))
    {

        // Have at least 24 characters of error text - Initial string (till end of error number) takes 16 chars
        // For example:

        // 12345678901234567890......
        // *** ERROR[nnnn] error text .. [date and time]

        length  = strlen(msgInfo);
        strSize = BUFFERSIZE - (length + 40);

    }
    if (strSize > 0)
            //strncat(msgInfo, inSqlError, min(strSize,inSqlErrorLength));
            strncat(msgInfo, inSqlError+16, _min(strSize,error->length));
    else
            strcat(msgInfo, "<N/A>");
}

int64 ResStatisticsStatement::getCpuTime()
{
    short error;
        int64 cpuTime = 0;
    char errorString[32];

  struct rusage  my_usage;

  if (error = getrusage(RUSAGE_SELF, &my_usage) != 0 )
  {
        sprintf(errorString, "%d", errno);
// +++ T2_REPO TODO
/*
        SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
            0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
            1, errorString);
*/
        return cpuTime;
  }

  cpuTime = my_usage.ru_utime.tv_sec * 1000000 +
         my_usage.ru_stime.tv_sec * 1000000 +
         my_usage.ru_stime.tv_usec +
         my_usage.ru_utime.tv_usec;

  return cpuTime;
}

short ResStatisticsStatement::currentPriority()
{
    short error;
    short curPriority = 0;
    char errorString[32];

    return curPriority;
}


void ResStatisticsStatement::setStatisticsFlag(bool setStatisticsFlag)
{
    statStatisticsFlag = setStatisticsFlag;
}


char *typeOfStatementList[] = {
    "SQL_OTHER",
    "SQL_UNKNOWN",
    "SQL_SELECT_UNIQUE",
    "SQL_SELECT_NON_UNIQUE",
    "SQL_INSERT_UNIQUE",
    "SQL_INSERT_NON_UNIQUE",
    "SQL_UPDATE_UNIQUE",
    "SQL_UPDATE_NON_UNIQUE",
    "SQL_DELETE_UNIQUE",
    "SQL_DELETE_NON_UNIQUE",
    "SQL_CONTROL",
    "SQL_SET_TRANSACTION",
    "SQL_SET_CATALOG",
    "SQL_SET_SCHEMA",
    "SQL_CALL_NO_RESULT_SETS",
    "SQL_CALL_WITH_RESULT_SETS",
    "SQL_SP_RESULT_SET",
    "SQL_INSERT_RWRS",
    "SQL_CAT_UTIL",
    "SQL_EXE_UTIL",
    "SQL_SELECT_UNLOAD",
    "SQL_NOT_SUPPORTED"
};

char* ResStatisticsStatement::getStatementType(Int32     inSqlQType)
{
    if(inSqlQType>=SQL_OTHER && inSqlQType<=SQL_SELECT_UNLOAD)
        return typeOfStatementList[inSqlQType+1];
    else
        return typeOfStatementList[21];
}

//
// AGGREGATION----------------procedures
//

const char* ResStatisticsStatement::mapEmptyToNA(char* input)
{
    if (input[0] != 0 && input[0] != ' ') return input;
    return "<N/A>";
}

void ResStatisticsStatement::SendQueryStats(bool bStart, SRVR_STMT_HDL *pSrvrStmt, char *inSqlError, Int32 inSqlErrorLength)
{
    SQL_COMPILATION_STATS_DATA comp_stats_data;
    comp_stats_data = pSrvrStmt->comp_stats_info.compilationStats;

    std::tr1::shared_ptr<STATEMENT_QUERYEXECUTION> pQuery_info = std::tr1::shared_ptr<STATEMENT_QUERYEXECUTION>(new STATEMENT_QUERYEXECUTION);
    *pQuery_info = {0};

    long long endtime = 0;
    if (!bStart)
        endtime = JULIANTIMESTAMP();

    pQuery_info->m_process_id = srvrGlobal->process_id;
    pQuery_info->m_thread_id = srvrGlobal->receiveThrId;
    pQuery_info->m_node_id = srvrGlobal->m_NodeId;
    pQuery_info->m_ip_address_id = srvrGlobal->IpAddress;
    pQuery_info->m_process_name = srvrGlobal->m_ProcName;
    pQuery_info->m_exec_start_utc_ts = queryStartTime;
    pQuery_info->m_query_id = queryId;
    pQuery_info->m_user_name = srvrGlobal->userSID;
    pQuery_info->m_role_name = srvrGlobal->QSRoleName;
    pQuery_info->m_start_priority = resCollectinfo.startPriority;
    pQuery_info->m_session_id = srvrGlobal->sessionId;
    pQuery_info->m_client_name = srvrGlobal->ClientComputerName;
    pQuery_info->m_application_name = srvrGlobal->ApplicationName;
    pQuery_info->m_statement_id = statementId;
    pQuery_info->m_statement_type = getStatementType(pSrvrStmt->sqlQueryType);
    //SUBMIT_UTC_TS=EXEC_START_UTC_TS for now
    //Will fix it once wms is brought in
    pQuery_info->m_submit_utc_ts = pQuery_info->m_exec_start_utc_ts;
    if (!pubStarted)
    {
    pQuery_info->m_compile_start_utc_ts = comp_stats_data.compileStartTime;
    pQuery_info->m_compile_end_utc_ts = comp_stats_data.compileEndTime;
    pQuery_info->m_compile_elapsed_time = comp_stats_data.compileEndTime - comp_stats_data.compileStartTime;
    pQuery_info->m_cmp_affinity_num = pSrvrStmt->comp_stats_info.affinityNumber;
    pQuery_info->m_cmp_dop = pSrvrStmt->comp_stats_info.dop;
    pQuery_info->m_cmp_txn_needed = pSrvrStmt->comp_stats_info.xnNeeded;
    pQuery_info->m_cmp_mandatory_x_prod = pSrvrStmt->comp_stats_info.mandatoryCrossProduct;
    pQuery_info->m_cmp_missing_stats = pSrvrStmt->comp_stats_info.missingStats;
    pQuery_info->m_cmp_num_joins = pSrvrStmt->comp_stats_info.numOfJoins;
    pQuery_info->m_cmp_full_scan_on_table = pSrvrStmt->comp_stats_info.fullScanOnTable;
    pQuery_info->m_cmp_rows_accessed_full_scan = max(double(0),pSrvrStmt->comp_stats_info.rowsAccessedForFullScan);
    pQuery_info->m_est_accessed_rows = estRowsAccessed;
    pQuery_info->m_est_used_rows = estRowsUsed;
    pQuery_info->m_cmp_compiler_id = comp_stats_data.compilerId;
    pQuery_info->m_cmp_cpu_path_length = comp_stats_data.cmpCpuTotal;
    pQuery_info->m_cmp_cpu_binder = comp_stats_data.cmpCpuBinder;
    pQuery_info->m_cmp_cpu_normalizer = comp_stats_data.cmpCpuNormalizer;
    pQuery_info->m_cmp_cpu_analyzer = comp_stats_data.cmpCpuAnalyzer;
    pQuery_info->m_cmp_cpu_optimizer = comp_stats_data.cmpCpuOptimizer;
    pQuery_info->m_cmp_cpu_generator = comp_stats_data.cmpCpuGenerator;
    pQuery_info->m_cmp_metadata_cache_hits = comp_stats_data.metadataCacheHits;
    pQuery_info->m_cmp_metadata_cache_lookups = comp_stats_data.metadataCacheLookups;
    pQuery_info->m_cmp_query_cache_status = comp_stats_data.queryCacheState;
    pQuery_info->m_cmp_histogram_cache_hits = comp_stats_data.histogramCacheHits;
    pQuery_info->m_cmp_histogram_cache_lookups = comp_stats_data.histogramCacheLookups;
    pQuery_info->m_cmp_stmt_heap_size = comp_stats_data.stmtHeapSize;
    pQuery_info->m_cmp_context_heap_size = comp_stats_data.cxtHeapSize;
    pQuery_info->m_cmp_optimization_tasks = comp_stats_data.optTasks;
    pQuery_info->m_cmp_optimization_contexts = comp_stats_data.optContexts;
    pQuery_info->m_cmp_is_recompile = comp_stats_data.isRecompile;
    pQuery_info->m_est_num_seq_ios = pSrvrStmt->cost_info.numSeqIOs;
    pQuery_info->m_est_num_rand_ios = pSrvrStmt->cost_info.numRandIOs;
    pQuery_info->m_est_cost = estimatedCost;
    pQuery_info->m_est_cardinality = pSrvrStmt->cost_info.cardinality;
    pQuery_info->m_est_io_time = pSrvrStmt->cost_info.ioTime;
    pQuery_info->m_est_msg_time = pSrvrStmt->cost_info.msgTime;
    pQuery_info->m_est_idle_time = pSrvrStmt->cost_info.idleTime;
    pQuery_info->m_est_cpu_time = pSrvrStmt->cost_info.cpuTime;
    pQuery_info->m_est_total_time = pSrvrStmt->cost_info.totalTime;
    pQuery_info->m_est_total_mem = pSrvrStmt->cost_info.estimatedTotalMem;
    pQuery_info->m_est_resource_usage = pSrvrStmt->cost_info.resourceUsage;
    //pQuery_info->m_aggregation_option =
    pQuery_info->m_cmp_number_of_bmos = pSrvrStmt->comp_stats_info.numOfBmos;
    char overflowmode_str[64];
    memset(overflowmode_str,0,sizeof(overflowmode_str));
    sprintf(overflowmode_str,"%u",pSrvrStmt->comp_stats_info.overflowMode);
    pQuery_info->m_cmp_overflow_mode = string(overflowmode_str);
    pQuery_info->m_cmp_overflow_size = pSrvrStmt->comp_stats_info.overflowSize;
    }
    //pQuery_info->m_aggregate_total =
    pQuery_info->m_stats_error_code = statsErrorCode;
    pQuery_info->m_query_elapsed_time = JULIANTIMESTAMP() - queryStartTime;
    pQuery_info->m_sql_process_busy_time = ProcessBusyTime;
    pQuery_info->m_disk_process_busy_time = DiskProcessBusyTime;
    pQuery_info->m_disk_ios = DiskIOs;
    pQuery_info->m_num_sql_processes = numSqlProcs;
    pQuery_info->m_sql_space_allocated = SpaceTotal;
    pQuery_info->m_sql_space_used = SpaceUsed;
    pQuery_info->m_sql_heap_allocated = HeapTotal;
    pQuery_info->m_sql_heap_used = HeapUsed;
    pQuery_info->m_total_mem_alloc = TotalMemAlloc;
    pQuery_info->m_max_mem_used = MaxMemUsed;
    pQuery_info->m_transaction_id = transID;
    pQuery_info->m_num_request_msgs = reqMsgCnt;
    pQuery_info->m_num_request_msg_bytes = reqMsgBytes;
    pQuery_info->m_num_reply_msgs = replyMsgCnt;
    pQuery_info->m_num_reply_msg_bytes = replyMsgBytes;
    pQuery_info->m_first_result_return_utc_ts = firstRowReturnTime;
    pQuery_info->m_rows_returned_to_master = rowsReturned;
    pQuery_info->m_parent_query_id = parentQID;
    pQuery_info->m_parent_system_name = parentSysName;
    pQuery_info->m_exec_end_utc_ts = endtime;
    pQuery_info->m_master_execution_time = queryExecutionTime;
    pQuery_info->m_master_elapse_time = queryElapseTime;
    pQuery_info->m_query_status = getQueryStateStringRes(pSrvrStmt->m_state);
    pQuery_info->m_error_code = errorCode;
    pQuery_info->m_sql_error_code = sqlErrorCode;
    pQuery_info->m_error_text = getErrorText(inSqlError, inSqlErrorLength, MAX_ERROR_TEXT_LENGTH);
    UpdateStringText(pQuery_info->m_error_text);
    if(pSrvrStmt->pSqlString!=NULL)
    {
        pQuery_info->m_query_text =pSrvrStmt->pSqlString;
        UpdateStringText(pQuery_info->m_query_text);
    }
    if (pSrvrStmt->exPlan != SRVR_STMT_HDL::STORED && pSrvrStmt->sqlPlan != NULL && pSrvrStmt->sqlPlanLen > 0)
    {
        pQuery_info->m_explain_plan = new char[pSrvrStmt->sqlPlanLen];
        if (pQuery_info->m_explain_plan != NULL)
        {
            memcpy( pQuery_info->m_explain_plan, pSrvrStmt->sqlPlan, pSrvrStmt->sqlPlanLen );
            pQuery_info->m_explain_plan_len = pSrvrStmt->sqlPlanLen;
            pSrvrStmt->exPlan = SRVR_STMT_HDL::STORED;  // Ignores for updates since plan does not change
        }
        else
            ;
// +++ T2_REPO TODO
/*
            SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
                            0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                            1, "SQL explain plan");
*/
    }
    pQuery_info->m_last_error_before_aqr = AQRlastError;
    pQuery_info->m_delay_time_before_aqr_sec = AQRdelayBeforeRetry;
    pQuery_info->m_total_num_aqr_retries = AQRnumRetries;
    pQuery_info->m_msg_bytes_to_disk = replyMsgBytes;
    pQuery_info->m_msgs_to_disk = replyMsgCnt;
    pQuery_info->m_rows_accessed = AccessedRows;
    pQuery_info->m_rows_retrieved = rowsReturned;
    pQuery_info->m_num_rows_iud = NumRowsIUD;
    pQuery_info->m_processes_created = NewProcess;
    pQuery_info->m_process_create_busy_time = NewProcessTime;
    pQuery_info->m_ovf_file_count = ScratchFileCount;
    pQuery_info->m_ovf_space_allocated = SpaceTotal;
    pQuery_info->m_ovf_space_used = SpaceUsed;
    pQuery_info->m_ovf_block_size = ScratchBufferBlockSize;
    pQuery_info->m_ovf_write_read_count = ScratchBufferReadCount + ScratchBufferWriteCount;
    pQuery_info->m_ovf_write_count = ScratchBufferWriteCount;
    pQuery_info->m_ovf_buffer_blocks_written = ScratchBufferBlocksWritten;
    //pQuery_info->m_ovf_buffer_bytes_written = ;
    pQuery_info->m_ovf_read_count = ScratchBufferReadCount;
    pQuery_info->m_ovf_buffer_blocks_read = ScratchBufferBlocksRead;
    //pQuery_info->m_ovf_buffer_bytes_read = ;
    //pQuery_info->m_num_nodes = ;
    pQuery_info->m_udr_process_busy_time = ProcessBusyTime;
    pQuery_info->m_pertable_stats = perTableRowSize;

    if (!pubStarted)
        sendQueryStats(PUB_TYPE_STATEMENT_NEW_QUERYEXECUTION, pQuery_info);
    else
        sendQueryStats(PUB_TYPE_STATEMENT_UPDATE_QUERYEXECUTION, pQuery_info);

    if (bStart)
        pubStarted = true;
}

