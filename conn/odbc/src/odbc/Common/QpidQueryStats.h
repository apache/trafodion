// ===============================================================================================
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
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
#ifndef QPID_QUERY_STATS_DEFINED
#define QPID_QUERY_STATS_DEFINED

#include <string>
using namespace std;

enum qpid_struct_type
{
	QPID_TYPE_INIT = 0,
	QPID_TYPE_SESSION_START,
	QPID_TYPE_SESSION_END,
	QPID_TYPE_STATEMENT_SQL_TEXT,
//	QPID_TYPE_STATEMENT_START_QUERYEXECUTION,
	QPID_TYPE_STATEMENT_END_QUERYEXECUTION,
	QPID_TYPE_WMS_STATS,
	QPID_TYPE_WMS_RESOURCES,
	QPID_TYPE_WMS_PERTABLESTATS
};

// typedef for sql text event, replace sending _QPID_STATEMENT_START_QUERYEXECUTION
typedef struct _QPID_STATEMENT_SQL_TEXT
{
	string m_functionCalled;
	string m_queryId;
	int64 m_queryStartTime;	
	string m_msgBuffer;
} QPID_STATEMENT_SQL_TEXT, *pQPID_STATEMENT_SQL_TEXT;
/*
//typedefs for query_stats event 21036
typedef struct _QPID_STATEMENT_START_QUERYEXECUTION
{
	string m_functionCalled;
	string m_queryStatsType;
	string m_sessionId;
	string m_statementId;
	string m_queryId;
	string m_statementType;
	string m_clientId;
	string m_userName;
	int m_userId;
	string m_QSRoleName;
	string m_applicationId;
	string m_nodeName;
	string m_cpuPin;
	string m_DSName;
	string m_QSServiceName;
	int64 m_queryStartTime;
	int64 m_entryTime;
	int m_currentPriority;
	int64 m_prepareStartTime;
	int64 m_prepareEndTime;
	int64 m_prepareTime;
	double m_est_cost;
	double m_cardinality;
	double m_est_totalTime;
	double m_ioTime;
	double m_msgTime;
	double m_idleTime;
	double m_cpuTime;
	double m_estTotalMem;
	double m_resourceUsage;
	double m_estNumSeqIOs; // Sprint-3 New NCM counters
	double m_estNumRandIOs; // Sprint-3 New NCM counters
	unsigned long m_affinityNumber;
	int m_dop;
	int m_xnNeeded;
	int m_mandatoryCrossProduct;
	int m_missingStats;
	int m_numOfJoins;
	int m_fullScanOnTable;
	int m_highDp2MxBufferUsage;
	double m_rowsAccessedForFullScan;
	double m_dp2RowsAccessed;
	double m_dp2RowsUsed;
	// int m_numOfUdrs			// For post R2.5/SQ
	string m_compilerId;
	unsigned long m_cmpCpuTotal;
	unsigned long m_cmpCpuBinder;
	unsigned long m_cmpCpuNormalizer;
	unsigned long m_cmpCpuAnalyzer;
	unsigned long m_cmpCpuOptimizer;
	unsigned long m_cmpCpuGenerator;
	unsigned long m_metadataCacheHits;
	unsigned long m_metadataCacheLookups;
	int m_queryCacheState;
	unsigned long m_histogramCacheHits;
	unsigned long m_histogramCacheLookups;
	unsigned long m_stmtHeapSize;
	unsigned long m_cxtHeapSize;
	unsigned long m_optTasks;
	unsigned long m_optContexts;
	int m_isRecompile;
	int64 m_WMSstartTS;
	string m_queryState;
	string m_querySubState;
	int64 m_waitTime;
	int64 m_holdTime;
	string m_con_rule_name;
	string m_cmp_rule_name;
	string m_exe_rule_name;
	string m_warnLevel;
	string m_aggregation;
	string m_msgBuffer;
	unsigned short m_cmp_number_of_bmos; //ssd overflow
	unsigned short m_cmp_overflow_mode;
	int64 m_cmp_overflow_size;
} QPID_STATEMENT_START_QUERYEXECUTION, *pQPID_STATEMENT_START_QUERYEXECUTION;
*/
typedef struct _QPID_STATEMENT_END_QUERYEXECUTION
{
	string m_functionCalled;
	string m_queryStatsType;
	string m_sessionId;
	string m_stmtName;
	string m_sqlUniqueQueryID;
	string m_parentQID;
	string m_transID;
	string m_statementType;
	string m_clientId;
	string m_userName;
	int m_userId;
	string m_QSRoleName;
	string m_applicationId;
	string m_nodeName;
	string m_cpuPin;
	string m_DSName;
	string m_QSServiceName;
	double m_estRowsAccessed;
	double m_estRowsUsed;
	int64 m_queryStartTime;
//new fields start
	int64 m_entryTime;
	int m_currentPriority;
	int64 m_prepareStartTime;
	int64 m_prepareEndTime;
	int64 m_prepareTime;
	double m_est_cost;
	double m_cardinality;
	double m_est_totalTime;
	double m_ioTime;
	double m_msgTime;
	double m_idleTime;
	double m_cpuTime;
	double m_estTotalMem;
	double m_resourceUsage;
	double m_estNumSeqIOs; // Sprint-3 New NCM counters
	double m_estNumRandIOs; // Sprint-3 New NCM counters
	unsigned long m_affinityNumber;
	int m_dop;
	int m_xnNeeded;
	int m_mandatoryCrossProduct;
	int m_missingStats;
	int m_numOfJoins;
	int m_fullScanOnTable;
	int m_highDp2MxBufferUsage;
	double m_rowsAccessedForFullScan;
	double m_dp2RowsAccessed;
	double m_dp2RowsUsed;
	// int m_numOfUdrs			// For post R2.5/SQ
	string m_compilerId;
	unsigned long m_cmpCpuTotal;
	unsigned long m_cmpCpuBinder;
	unsigned long m_cmpCpuNormalizer;
	unsigned long m_cmpCpuAnalyzer;
	unsigned long m_cmpCpuOptimizer;
	unsigned long m_cmpCpuGenerator;
	unsigned long m_metadataCacheHits;
	unsigned long m_metadataCacheLookups;
	int m_queryCacheState;
	unsigned long m_histogramCacheHits;
	unsigned long m_histogramCacheLookups;
	unsigned long m_stmtHeapSize;
	unsigned long m_cxtHeapSize;
	unsigned long m_optTasks;
	unsigned long m_optContexts;
	int m_isRecompile;
	string m_con_rule_name;
	string m_cmp_rule_name;
	string m_aggregation;
	string m_msgBuffer;
	unsigned short m_cmp_number_of_bmos; //ssd overflow
	unsigned short m_cmp_overflow_mode;
	int64 m_cmp_overflow_size;
//new fields end
	int64 m_inexeEndTime;
	int64 m_statementEndTime;
	int64 m_inqueryElapseTime;
	int64 m_inqueryExecutionTime;
	int64 m_firstRowReturnTime;
	int64 m_rowsReturned;
	int64 m_ProcessBusyTime; //SQLProcessBusyTime
	int m_numSqlProcs;
	int64 m_numberOfRows;
	int m_errorCode;
	int m_sqlErrorCode;
	int m_statsErrorCode;
	int m_AQRlastError;
	int m_AQRnumRetries;
	int m_AQRdelayBeforeRetry;
	int64 m_WMSstartTS;
	string m_queryState;
	string m_querySubstate;
	string m_SQLState;
	int64 m_execTime;
	int64 m_waitTime;
	int64 m_holdTime;
	int64 m_suspendTime;
	int64 m_OpenTime; //OpenBusyTime
	int64 m_Opens;
	int64 m_NewProcess;
	int64 m_NewProcessTime;
	string m_WarnLevel;
	int64 m_AccessedRows;
	int64 m_UsedRows;
	int64 m_DiskProcessBusyTime;
	int64 m_DiskIOs;
	int64 m_SpaceTotal;
	int64 m_SpaceUsed;
	int64 m_HeapTotal;
	int64 m_HeapUsed;
	int64 m_TotalMemAlloc;
	int64 m_MaxMemUsed;
	int64 m_Dp2SpaceTotal;
	int64 m_Dp2SpaceUsed;
	int64 m_Dp2HeapTotal;
	int64 m_Dp2HeapUsed;
	int64 m_NumMessages;
	int64 m_MessagesBytes;
	int64 m_reqMsgCnt;
	int64 m_reqMsgBytes;
	int64 m_replyMsgCnt;
	int64 m_replyMsgBytes;
	int64 m_LockWaits;
	int64 m_Escalations;
	string m_exe_rule_name;
	int64 m_TotalAggregates;
	string m_errorText;
	int64 m_ovf_file_count; //ssd overflow
	int64 m_ovf_space_allocated;
	int64 m_ovf_space_used;
	int64 m_ovf_block_size;
	int64 m_ovf_ios;
	int64 m_ovf_message_buffers_to;
	int64 m_ovf_message_to;
	int64 m_ovf_message_bytes_to;
	int64 m_ovf_message_buffers_out;
	int64 m_ovf_message_out;
	int64 m_ovf_message_bytes_out;
	int64 m_suspended_ts;	//3289
	int64 m_released_ts;
	int64 m_cancelled_ts;
	int64 m_numCpus;
	int64 m_UDRProcessBusyTime;
	string m_QuerySubType;
	string m_ParentSystemName;
	bool m_pertable_stats;
} QPID_STATEMENT_END_QUERYEXECUTION, *pQPID_STATEMENT_END_QUERYEXECUTION;

//typedefs for session_stats event 21035
typedef struct _QPID_SESSION_START
{
	string m_functionCalled;
	string m_sessionStatsType;
	string m_sessionId;
	string m_session_status;
	int64 m_user_id;
	string m_user_name;
	string m_role_name;
	string m_client_name;
	string m_client_user_name;
	string m_application_name;
	string m_datasource_name;
	int64 m_entryTime;
	int64 m_total_login_elapsed_time_mcsec;
	int64 m_ldap_login_elapsed_time_mcsec;
	int64 m_sql_user_elapsed_time_mcsec;
	int64 m_search_connection_elapsed_time_mcsec;
	int64 m_search_elapsed_time_mcsec;
	int64 m_authentication_connection_elapsed_time_mcsec;
	int64 m_authentication_elapsed_time_mcsec;
	int64 m_session_start_time;
} QPID_SESSION_START, *pQPID_SESSION_START;

typedef struct _QPID_SESSION_END
{
	string m_functionCalled;
	string m_sessionStatsType;
	string m_sessionId;
	string m_session_status;
	int64 m_total_odbc_execution_time;
	int64 m_total_odbc_elapsed_time;
	int64 m_total_insert_stmts_executed;
	int64 m_total_delete_stmts_executed;
	int64 m_total_update_stmts_executed;
	int64 m_total_select_stmts_executed;
	int64 m_total_catalog_stmts;
	int64 m_total_prepares;
	int64 m_total_executes;
	int64 m_total_fetches;
	int64 m_total_closes;
	int64 m_total_execdirects;
	int64 m_total_errors;
	int64 m_total_warnings;
	int64 m_session_end_time;
} QPID_SESSION_END, *pQPID_SESSION_END;

typedef struct _QPID_WMS_STATS
{
	string m_functionCalled;
	int m_node_id;
	string m_node_name;
	string m_node_list;
	int64 m_total_queries;
	int64 m_total_exec;
	int64 m_total_wait;
	int64 m_total_hold;
	int64 m_total_suspend;
	int64 m_total_reject;
	int64 m_total_cancel;
	int64 m_total_complete;
	int m_avg_exec_secs;
	int m_avg_wait_secs;
	int m_avg_hold_secs;
	int m_avg_suspend_secs;
	int m_conn_rule;
	int m_comp_rule;
	int m_exec_rule;
	int m_cur_esps;
	int64 m_begin_ts_lct;
	int64 m_end_ts_lct;
} QPID_WMS_STATS, *pQPID_WMS_STATS;

typedef struct _QPID_WMS_RESOURCES
{
	string m_functionCalled;
	int m_cpu_busy;
	int m_memory_usage;
	int m_ssd_usage;
	int m_max_node_esps;
	int m_avg_node_esps;
	int64 m_current_ts_lct;
} QPID_WMS_RESOURCES, *pQPID_WMS_RESOURCES;

typedef struct _QPID_WMS_PERTABLESTATS
{
	string m_functionCalled;
	string m_queryId;
	string m_tblName;
	int64 m_numMessages;
	int64 m_messagesBytes;
	int64 m_diskIOs;
	int64 m_lockWaits;
	int64 m_escalations;
	int64 m_processBusyTime;
	int64 m_opens;
	int64 m_openTime;
	double m_accessedRows;
	double m_usedRows;
	int64 m_current_ts_lct;

} QPID_WMS_PERTABLESTATS, *pQPID_WMS_PERTABLESTATS;

#endif
