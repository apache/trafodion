// ===============================================================================================
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
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
// ===============================================================================================
#ifndef QPID_EMS_INTERFACE_DEFINED
#define QPID_EMS_INTERFACE_DEFINED

#include <platform_ndcs.h>
#include <platform_utils.h>
#include <dlfcn.h>

#include "QpidQueryStats.h"

extern void sendTokenizedEvent(const QPID_SESSION_START& ss);
extern void sendTokenizedEvent(const QPID_SESSION_END& se);
extern void sendTokenizedEvent(const QPID_STATEMENT_SQL_TEXT& sst);
extern void sendTokenizedEvent(const QPID_STATEMENT_END_QUERYEXECUTION& seqe);
extern void sendTokenizedEvent(const QPID_WMS_STATS& ws);
extern void sendTokenizedEvent(const QPID_WMS_RESOURCES& wr);
extern void sendTokenizedEvent(const QPID_WMS_PERTABLESTATS& wps);

#include <sqlcli.h>

struct collect_info
{
    char					        clientId[MAX_COMPUTERNAME_LENGTH*4 + 1];
    char				            userName[MAX_USERNAME_LEN + 1];
    char				            clientUserName[MAX_SQL_IDENTIFIER_LEN + 1];
    char					        applicationId[MAX_APPLICATIONID_LENGTH*4 + 1];
    char						    nodeName[10];
    char							cpuPin[20];
	char							DSName[MAX_DSOURCE_NAME + 1];
    long							userId;
    short							startPriority;
    short							currentPriority;
    unsigned long					totalLoginTime;
    unsigned long					ldapLoginTime;
    unsigned long					sqlUserTime;
    unsigned long					searchConnectionTime;
    unsigned long					searchTime;
    unsigned long					authenticationConnectionTime;
    unsigned long					authenticationTime;
};

extern void SetSessionStatsInfoStart(
		  const char *functionCalled
		, const char *sessionId
		, const char *session_status
		, int64 user_id
		, const char *user_name
		, const char *role_name
		, const char *client_name
		, const char *client_user_name
		, const char *application_name
		, const char *datasource_name
		, int64 entryTime
		, int64 total_login_elapsed_time_mcsec
		, int64 ldap_login_elapsed_time_mcsec
		, int64 sql_user_elapsed_time_mcsec
		, int64 search_connection_elapsed_time_mcsec
		, int64 search_elapsed_time_mcsec
		, int64 authentication_connection_elapsed_time_mcsec
		, int64 authentication_elapsed_time_mcsec
		, int64 session_start_time
);
extern void SetSessionStatsInfoEnd(
		  const char *functionCalled
		, const char *sessionId
		, const char *session_status
		, int64 total_odbc_execution_time
		, int64 total_odbc_elapsed_time
		, int64 total_insert_stmts_executed
		, int64 total_delete_stmts_executed
		, int64 total_update_stmts_executed
		, int64 total_select_stmts_executed
		, int64 total_catalog_stmts
		, int64 total_prepares
		, int64 total_executes
		, int64 total_fetches
		, int64 total_closes
		, int64 total_execdirects
		, int64 total_errors
		, int64 total_warnings
		, int64 session_end_time
);
extern void SetQueryStatsInfoSqlText(
	const char *functionCalled
	, const char *queryId
	, const int64 queryStartTime
	, const char *msgBuffer
	);

extern void SetQueryStatsInfoEndQueryExecution(
		  const char *functionCalled
	    , const char *sessionId
		, const char *stmtName
		, const char *sqlUniqueQueryID
		, const char *parentQID
		, const char *transID
		, const char *statementType
		, const char *clientId
		, const char *userName
		, int userId
		, const char *QSRoleName
		, const char *applicationId
		, const char *nodeName
		, const char *cpuPin
		, const char *DSName
		, const char *QSServiceName
		, double estRowsAccessed
		, double estRowsUsed
		, int64 entryTime
		, int64 queryStartTime
		, int currentPriority
		, int64 prepareStartTime
		, int64 prepareEndTime
		, int64 prepareTime
		, double estTotalMem
		, const char *aggregation
		, const char *msgBuffer
		, struct collect_info& resCollectinfo
		, SQL_QUERY_COST_INFO& cost_info
		, SQL_QUERY_COMPILER_STATS_INFO& comp_stats_info
		, SQL_COMPILATION_STATS_DATA *comp_stats_data

		, int64 inexeEndTime
		, int64 statementEndTime
		, int64 inqueryElapseTime
		, int64 inqueryExecutionTime
		, int64 firstRowReturnTime
		, int64 rowsReturned
		, int64 ProcessBusyTime  //SQLProcessBusyTime
		, int numSqlProcs
		, int64 numberOfRows
		, int errorCode
		, int sqlErrorCode
		, int statsErrorCode
		, int AQRlastError
		, int AQRnumRetries
		, int AQRdelayBeforeRetry
		, int64 WMSstartTS
		, const char *queryState
		, const char *querySubstate
		, const char *SQLState
		, int64 execTime
		, int64 waitTime
		, int64 holdTime
		, int64 suspendTime
		, int64 OpenTime  //OpenBusyTime
		, int64 Opens
		, int64 NewProcess
		, int64 NewProcessTime
		, const char *WarnLevel
		, int64 AccessedRows
		, int64 UsedRows
		, int64 DiskProcessBusyTime
		, int64 DiskIOs
		, int64 SpaceTotal
		, int64 SpaceUsed
		, int64 HeapTotal
		, int64 HeapUsed
		, int64 TotalMemAlloc
		, int64 MaxMemUsed
		, int64 Dp2SpaceTotal
		, int64 Dp2SpaceUsed
		, int64 Dp2HeapTotal
		, int64 Dp2HeapUsed
		, int64 NumMessages
		, int64 MessagesBytes
		, int64 reqMsgCnt
		, int64 reqMsgBytes
		, int64 replyMsgCnt
		, int64 replyMsgBytes
		, int64 LockWaits
		, int64 Escalations
		, const char *con_rule_name
		, const char *cmp_rule_name
		, const char *exe_rule_name
		, int64 totalStatementExecutes
		, int64 TotalAggregates
		, EXEC_OVERFLOW& execOverflow
		, int64 suspended_ts	//3289
		, int64 released_ts
		, int64 cancelled_ts
		, int64 numCpus
		, int64 UDRProcessBusyTime
		, const char *QuerySubType
		, const char *ParentSystemName
		, const char *errorText = ""
		, bool pertable_stats = false
		);

extern void SendEventMsg(
		  DWORD EventId
		, short EventLogType
		, DWORD Pid
		, char *ComponentName
		, char *ObjectRef
		, short nToken
		, ...);

extern void setCriticalDialout();

extern void setIsWms();

#ifdef NSK_ODBC_SRVR
extern int mxosrvr_init_seabed_trace_dll();
extern int mxosrvr_seabed_trace_close_dll();
#endif

#ifdef NSK_AS
extern int mxoas_init_seabed_trace_dll();
extern int mxoas_seabed_trace_close_dll();
#endif

#ifdef NSK_CFGSRVR
extern int mxocfg_init_seabed_trace_dll();
extern int mxocfg_seabed_trace_close_dll();
#endif

#endif
