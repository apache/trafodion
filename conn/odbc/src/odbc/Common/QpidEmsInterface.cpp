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

#include "QpidEmsInterface.h"

// trace variables
static bool	gv_trace_ems_dll = false;
static bool	gv_trace_legacy_dll = false;

typedef void (*FunctionSendTokenizedEvent) (qpid_struct_type, void*);
typedef void (*FunctionSendToEventLog ) (short, short, char *, char *, short, va_list);
typedef void (*FunctionSetCriticalDialout ) ();
typedef void (*FunctionSetIsWms ) ();
typedef void (*FunctionSetTraceVariables ) (bool, bool);

#define QpidEmsDll "libzqsevents.so"
#define SEND_TOKENIZED_EVENT 	"sendTokenizedEvent"
#define SEND_TO_EVENT_LOG		"sendToEventLog"
#define SET_CRITICAL_DIALOUT	"setCriticalDialoutDll"
#define SET_IS_WMS				"setIsWmsDll"
#define SET_TRACE_VARIABLES		"setTraceVariables"

dlHandle hQpidEmsDll;
int dl_result;
char dl_error[100];

FunctionSendTokenizedEvent	CallSendTokenizedEvent;
FunctionSendToEventLog		CallSendToEventLog;
FunctionSetCriticalDialout	CallSetCriticalDialout;
FunctionSetIsWms			CallSetIsWms;
FunctionSetTraceVariables	CallSetTraceVariables;

static bool open_dll(char* dl_name ){
	bool bret = true;
	dl_result = 0;
	memset(dl_error,0,sizeof(dl_error));

	hQpidEmsDll = dlopen(dl_name,RTLD_NOW);
	if(hQpidEmsDll == 0){
		bret = false;
	}
	else {
		CallSendTokenizedEvent = (FunctionSendTokenizedEvent) dlsym(hQpidEmsDll, SEND_TOKENIZED_EVENT);
		if (CallSendTokenizedEvent == 0){
			bret = false;
		}
		if (bret == true) {
			CallSendToEventLog = (FunctionSendToEventLog) dlsym(hQpidEmsDll, SEND_TO_EVENT_LOG);
			if (CallSendToEventLog == 0){
				bret = false;
			}
		}
		if (bret == true) {
			CallSetCriticalDialout = (FunctionSetCriticalDialout) dlsym(hQpidEmsDll, SET_CRITICAL_DIALOUT);
			if (CallSendToEventLog == 0){
				bret = false;
			}
		}
		if (bret == true) {
			CallSetIsWms = (FunctionSetIsWms) dlsym(hQpidEmsDll, SET_IS_WMS);
			if (CallSetIsWms == 0){
				bret = false;
			}
		}
		if (bret == true) {
			CallSetTraceVariables = (FunctionSetTraceVariables) dlsym(hQpidEmsDll, SET_TRACE_VARIABLES);
			if (CallSetTraceVariables == 0){
				bret = false;
			}
		}
	}
	if (bret == false){
		dl_result = dlresultcode();
		strncpy(dl_error, dlerror(), sizeof(dl_error) - 1);
		fprintf(stderr, "WMS EXCEPTION: open_dll(%s) error: %s",dl_name, dl_error);

		if (hQpidEmsDll != NULL){
			dlclose(hQpidEmsDll);
			hQpidEmsDll = NULL;
		}
		CallSendTokenizedEvent = NULL;
		CallSendToEventLog = NULL;
		CallSetCriticalDialout = NULL;
		CallSetIsWms = NULL;
		CallSetTraceVariables = NULL;
	}
	return bret;
}

void sendTokenizedEvent(const QPID_SESSION_START& ss){
	//remove when logging is figured out
	return;

	bool bret = true;

	if (hQpidEmsDll == NULL)
		bret = open_dll(QpidEmsDll);
	if (bret == true)
		CallSendTokenizedEvent(QPID_TYPE_SESSION_START, (void*)&ss);
}
void sendTokenizedEvent(const QPID_SESSION_END& se){
	//remove when logging is figured out
	return;

	bool bret = true;

	if (hQpidEmsDll == NULL)
		bret = open_dll(QpidEmsDll);
	if (bret == true)
		CallSendTokenizedEvent(QPID_TYPE_SESSION_END, (void*)&se);
}
void sendTokenizedEvent(const QPID_STATEMENT_SQL_TEXT& sst){
	//remove when logging is figured out
	return;

	bool bret = true;

	if (hQpidEmsDll == NULL)
		bret = open_dll(QpidEmsDll);
	if (bret == true)
		CallSendTokenizedEvent(QPID_TYPE_STATEMENT_SQL_TEXT, (void*)&sst);
}
void sendTokenizedEvent(const QPID_STATEMENT_END_QUERYEXECUTION& seqe){
	//remove when logging is figured out
	return;

	bool bret = true;

	if (hQpidEmsDll == NULL)
		bret = open_dll(QpidEmsDll);
	if (bret == true)
		CallSendTokenizedEvent(QPID_TYPE_STATEMENT_END_QUERYEXECUTION, (void*)&seqe);
}
void sendTokenizedEvent(const QPID_WMS_STATS& ws){
	//remove when logging is figured out
	return;

	bool bret = true;

	if (hQpidEmsDll == NULL)
		bret = open_dll(QpidEmsDll);
	if (bret == true)
		CallSendTokenizedEvent(QPID_TYPE_WMS_STATS, (void*)&ws);
}
void sendTokenizedEvent(const QPID_WMS_RESOURCES& wr){
	//remove when logging is figured out
	return;

	bool bret = true;

	if (hQpidEmsDll == NULL)
		bret = open_dll(QpidEmsDll);
	if (bret == true)
		CallSendTokenizedEvent(QPID_TYPE_WMS_RESOURCES, (void*)&wr);
}
void sendTokenizedEvent(const QPID_WMS_PERTABLESTATS& wps){
	//remove when logging is figured out
	return;

	bool bret = true;

	if (hQpidEmsDll == NULL)
		bret = open_dll(QpidEmsDll);
	if (bret == true)
		CallSendTokenizedEvent(QPID_TYPE_WMS_PERTABLESTATS, (void*)&wps);
}
//
//
void SetSessionStatsInfoStart(
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
)
{
	QPID_SESSION_START ss;
	ss.m_functionCalled = functionCalled;
	ss.m_sessionStatsType = "SESSION:START";
	ss.m_sessionId = sessionId;
	ss.m_session_status = session_status;
	ss.m_user_id = user_id;
	ss.m_user_name = user_name;
	ss.m_role_name = role_name;
	ss.m_client_name = client_name;
	ss.m_client_user_name = client_user_name;
	ss.m_application_name = application_name;
	ss.m_datasource_name = datasource_name;
	ss.m_entryTime = entryTime;
	ss.m_total_login_elapsed_time_mcsec = total_login_elapsed_time_mcsec;
	ss.m_ldap_login_elapsed_time_mcsec = ldap_login_elapsed_time_mcsec;
	ss.m_sql_user_elapsed_time_mcsec = sql_user_elapsed_time_mcsec;
	ss.m_search_connection_elapsed_time_mcsec = search_connection_elapsed_time_mcsec;
	ss.m_search_elapsed_time_mcsec = search_elapsed_time_mcsec;
	ss.m_authentication_connection_elapsed_time_mcsec = authentication_connection_elapsed_time_mcsec;
	ss.m_authentication_elapsed_time_mcsec = authentication_elapsed_time_mcsec;
	ss.m_session_start_time = session_start_time;
	sendTokenizedEvent(ss);
}

void SetSessionStatsInfoEnd(
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
)
{
	QPID_SESSION_END se;
	se.m_functionCalled = functionCalled;
	se.m_sessionStatsType = "SESSION:END";
	se.m_sessionId = sessionId;
	se.m_session_status = session_status;
	se.m_total_odbc_execution_time = total_odbc_execution_time;
	se.m_total_odbc_elapsed_time = total_odbc_elapsed_time;
	se.m_total_insert_stmts_executed = total_insert_stmts_executed;
	se.m_total_delete_stmts_executed = total_delete_stmts_executed;
	se.m_total_update_stmts_executed = total_update_stmts_executed;
	se.m_total_select_stmts_executed = total_select_stmts_executed;
	se.m_total_catalog_stmts = total_catalog_stmts;
	se.m_total_prepares = total_prepares;
	se.m_total_executes = total_executes;
	se.m_total_fetches = total_fetches;
	se.m_total_closes = total_closes;
	se.m_total_execdirects = total_execdirects;
	se.m_total_errors = total_errors;
	se.m_total_warnings = total_warnings;
	se.m_session_end_time = session_end_time;
	sendTokenizedEvent(se);
}

void SetQueryStatsInfoSqlText(
	const char *functionCalled
	, const char *queryId
	, const int64 queryStartTime
	, const char *msgBuffer
	)
{
	QPID_STATEMENT_SQL_TEXT sst;
	sst.m_functionCalled = functionCalled;
	sst.m_queryId = queryId;
	sst.m_queryStartTime = queryStartTime;
	sst.m_msgBuffer = msgBuffer;
	sendTokenizedEvent(sst);
}

void SetQueryStatsInfoEndQueryExecution(
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
		, const char *errorText
		, bool pertable_stats
		)
{
	QPID_STATEMENT_END_QUERYEXECUTION seqe;
	seqe.m_functionCalled = functionCalled;
	seqe.m_queryStatsType = "STATEMENT:END:QueryExecution";
	seqe.m_sessionId = sessionId;
	seqe.m_stmtName = stmtName;
	seqe.m_sqlUniqueQueryID = sqlUniqueQueryID;
	seqe.m_parentQID = parentQID;
	seqe.m_transID = transID;
	seqe.m_statementType = statementType;
	seqe.m_clientId = clientId;
	seqe.m_userName = userName;
	seqe.m_userId = userId;
	seqe.m_QSRoleName = QSRoleName;
	seqe.m_applicationId = applicationId;
	seqe.m_nodeName = nodeName;
	seqe.m_cpuPin = cpuPin;
	seqe.m_DSName = DSName;
	seqe.m_QSServiceName = QSServiceName;
	seqe.m_estRowsAccessed = estRowsAccessed;
	seqe.m_estRowsUsed = estRowsUsed;
//new fields start
	seqe.m_entryTime = entryTime;
	seqe.m_currentPriority = currentPriority;
	seqe.m_prepareStartTime = prepareStartTime;
	seqe.m_prepareEndTime = prepareEndTime;
	seqe.m_prepareTime = prepareTime;
	seqe.m_est_cost = cost_info.totalTime;
	seqe.m_cardinality = cost_info.cardinality;
	seqe.m_est_totalTime = cost_info.totalTime;
	seqe.m_ioTime = cost_info.ioTime;
	seqe.m_msgTime = cost_info.msgTime;
	seqe.m_idleTime = cost_info.idleTime;
	seqe.m_cpuTime = cost_info.cpuTime;
	seqe.m_estTotalMem = estTotalMem;
	seqe.m_resourceUsage = cost_info.resourceUsage;
	seqe.m_estNumSeqIOs = cost_info.numSeqIOs;
	seqe.m_estNumRandIOs = cost_info.numRandIOs;
	seqe.m_affinityNumber = comp_stats_info.affinityNumber;
	seqe.m_dop = comp_stats_info.dop;
	seqe.m_xnNeeded = comp_stats_info.xnNeeded;
	seqe.m_mandatoryCrossProduct = comp_stats_info.mandatoryCrossProduct;
	seqe.m_missingStats = comp_stats_info.missingStats;
	seqe.m_numOfJoins = comp_stats_info.numOfJoins;
	seqe.m_fullScanOnTable = comp_stats_info.fullScanOnTable;
	seqe.m_highDp2MxBufferUsage = comp_stats_info.highDp2MxBufferUsage;
	seqe.m_rowsAccessedForFullScan = comp_stats_info.rowsAccessedForFullScan;
	seqe.m_dp2RowsAccessed = comp_stats_info.dp2RowsAccessed;
	seqe.m_dp2RowsUsed = comp_stats_info.dp2RowsUsed;
	seqe.m_cmp_number_of_bmos = comp_stats_info.numOfBmos;
	seqe.m_cmp_overflow_mode = comp_stats_info.overflowMode;
	seqe.m_cmp_overflow_size = (int64)comp_stats_info.overflowSize / 1024;
	// seqe.m_numOfUdrs	= numOfUdrs;		// For post R2.5/SQ
	if (comp_stats_data != NULL)
	{
		seqe.m_compilerId = comp_stats_data->compilerId;
		seqe.m_cmpCpuTotal = comp_stats_data->cmpCpuTotal;
		seqe.m_cmpCpuBinder = comp_stats_data->cmpCpuBinder;
		seqe.m_cmpCpuNormalizer = comp_stats_data->cmpCpuNormalizer;
		seqe.m_cmpCpuAnalyzer = comp_stats_data->cmpCpuAnalyzer;
		seqe.m_cmpCpuOptimizer = comp_stats_data->cmpCpuOptimizer;
		seqe.m_cmpCpuGenerator = comp_stats_data->cmpCpuGenerator;
		seqe.m_metadataCacheHits = comp_stats_data->metadataCacheHits;
		seqe.m_metadataCacheLookups = comp_stats_data->metadataCacheLookups;
		seqe.m_queryCacheState = comp_stats_data->queryCacheState;
		seqe.m_histogramCacheHits = comp_stats_data->histogramCacheHits;
		seqe.m_histogramCacheLookups = comp_stats_data->histogramCacheLookups;
		seqe.m_stmtHeapSize = comp_stats_data->stmtHeapSize;
		seqe.m_cxtHeapSize = comp_stats_data->cxtHeapSize;
		seqe.m_optTasks = comp_stats_data->optTasks;
		seqe.m_optContexts = comp_stats_data->optContexts;
		seqe.m_isRecompile = comp_stats_data->isRecompile;
	}
	else
	{
		seqe.m_compilerId = "";
		seqe.m_cmpCpuTotal = seqe.m_cmpCpuBinder = seqe.m_cmpCpuNormalizer = seqe.m_cmpCpuAnalyzer = seqe.m_cmpCpuOptimizer = -1;
		seqe.m_cmpCpuGenerator = seqe.m_metadataCacheHits = seqe.m_metadataCacheLookups = seqe.m_queryCacheState = -1;
		seqe.m_histogramCacheHits = seqe.m_histogramCacheLookups = seqe.m_stmtHeapSize = seqe.m_cxtHeapSize = -1;
		seqe.m_optTasks = seqe.m_optContexts = seqe.m_isRecompile = -1;
	}
	seqe.m_con_rule_name = con_rule_name;
	seqe.m_cmp_rule_name = cmp_rule_name;
	seqe.m_aggregation = aggregation;
	seqe.m_msgBuffer = msgBuffer;
//new fields end
	seqe.m_queryStartTime = queryStartTime;
	seqe.m_inexeEndTime = inexeEndTime;
	seqe.m_statementEndTime = statementEndTime;
	seqe.m_inqueryElapseTime = inqueryElapseTime;
	seqe.m_inqueryExecutionTime = inqueryExecutionTime;
	seqe.m_firstRowReturnTime = firstRowReturnTime;
	seqe.m_rowsReturned = rowsReturned;
	seqe.m_ProcessBusyTime = ProcessBusyTime;  //SQLProcessBusyTime
	seqe.m_numSqlProcs = numSqlProcs;
	seqe.m_errorCode = errorCode;
	seqe.m_numberOfRows = numberOfRows;
	seqe.m_sqlErrorCode = sqlErrorCode;
	seqe.m_statsErrorCode = statsErrorCode;
	seqe.m_AQRlastError = AQRlastError;
	seqe.m_AQRnumRetries = AQRnumRetries;
	seqe.m_AQRdelayBeforeRetry = AQRdelayBeforeRetry;
	seqe.m_WMSstartTS = WMSstartTS;
	seqe.m_queryState = queryState;
	seqe.m_querySubstate = querySubstate;
	seqe.m_SQLState = SQLState;
	seqe.m_execTime = execTime;
	seqe.m_waitTime = waitTime;
	seqe.m_holdTime = holdTime;
	seqe.m_suspendTime = suspendTime;
	seqe.m_OpenTime = OpenTime;  //OpenBusyTime
	seqe.m_Opens = Opens;
	seqe.m_NewProcess = NewProcess;
	seqe.m_NewProcessTime = NewProcessTime;
	seqe.m_WarnLevel = WarnLevel;
	seqe.m_AccessedRows = AccessedRows;
	seqe.m_UsedRows = UsedRows;
	seqe.m_DiskProcessBusyTime = DiskProcessBusyTime;
	seqe.m_DiskIOs = DiskIOs;
	seqe.m_SpaceTotal = SpaceTotal;
	seqe.m_SpaceUsed = SpaceUsed;
	seqe.m_HeapTotal = HeapTotal;
	seqe.m_HeapUsed = HeapUsed;
	seqe.m_TotalMemAlloc = TotalMemAlloc;
	seqe.m_MaxMemUsed = MaxMemUsed;
	seqe.m_Dp2SpaceTotal = Dp2SpaceTotal;
	seqe.m_Dp2SpaceUsed = Dp2SpaceUsed;
	seqe.m_Dp2HeapTotal = Dp2HeapTotal;
	seqe.m_Dp2HeapUsed = Dp2HeapUsed;
	seqe.m_NumMessages = NumMessages;
	seqe.m_MessagesBytes = MessagesBytes;
	seqe.m_reqMsgCnt = reqMsgCnt;
	seqe.m_reqMsgBytes = reqMsgBytes;
	seqe.m_replyMsgCnt = replyMsgCnt;
	seqe.m_replyMsgBytes = replyMsgBytes;
	seqe.m_LockWaits = LockWaits;
	seqe.m_Escalations = Escalations;
	//seqe.m_con_rule_name = con_rule_name;
	//seqe.m_cmp_rule_name = cmp_rule_name;
	seqe.m_exe_rule_name = exe_rule_name;
	//seqe.m_totalStatementExecutes = totalStatementExecutes;
	seqe.m_TotalAggregates = TotalAggregates;
	seqe.m_errorText = errorText;
	seqe.m_ovf_file_count = execOverflow.m_OvfFileCount;
	seqe.m_ovf_space_allocated = execOverflow.m_OvfSpaceAllocated;
	seqe.m_ovf_space_used = execOverflow.m_OvfSpaceUsed;
	seqe.m_ovf_block_size = execOverflow.m_OvfBlockSize;
	seqe.m_ovf_ios = execOverflow.m_OvfIOs;
	seqe.m_ovf_message_buffers_to = execOverflow.m_OvfMessageBuffersTo;
	seqe.m_ovf_message_to = execOverflow.m_OvfMessageTo;
	seqe.m_ovf_message_bytes_to = execOverflow.m_OvfMessageBytesTo;
	seqe.m_ovf_message_buffers_out = execOverflow.m_OvfMessageBuffersFrom;
	seqe.m_ovf_message_out = execOverflow.m_OvfMessageFrom;
	seqe.m_ovf_message_bytes_out = execOverflow.m_OvfMessageBytesFrom;
	seqe.m_suspended_ts = suspended_ts;	//3289
	seqe.m_released_ts = released_ts;
	seqe.m_cancelled_ts = cancelled_ts;
	seqe.m_numCpus = numCpus;
	seqe.m_UDRProcessBusyTime = UDRProcessBusyTime;
	seqe.m_QuerySubType = QuerySubType;
	seqe.m_ParentSystemName = ParentSystemName;
	seqe.m_pertable_stats = pertable_stats;

	sendTokenizedEvent(seqe);
}

void SendEventMsg(DWORD EventId, short EventLogType, DWORD Pid, char *ComponentName,
			char *ObjectRef, short nToken, ...){

	//remove when logging is figured out
	return;

	va_list marker;
	va_start( marker, nToken);

	bool bret = true;
	if (hQpidEmsDll == NULL)
		bret = open_dll(QpidEmsDll);
	if (bret == true)
		CallSendToEventLog(EventId & 0x0FFFF, EventLogType, ComponentName, ObjectRef, nToken, marker);

	va_end(marker);
	return;

}

extern void setCriticalDialout() {

	//remove when logging is figured out
	return;

	bool bret = true;

	if (hQpidEmsDll == NULL)
		bret = open_dll(QpidEmsDll);
	if (bret == true)
		CallSetCriticalDialout();
}

extern void setIsWms() {

	//remove when logging is figured out
	return;

	bool bret = true;

	if (hQpidEmsDll == NULL)
		bret = open_dll(QpidEmsDll);
	if (bret == true)
		CallSetIsWms();
}

#ifdef NSK_ODBC_SRVR
int mxosrvr_init_seabed_trace_dll()
{
	bool bret = true;

	const char *gp_mxosrvr_trace_filename         = "mxosrvr_trace_";
	const char *gp_mxosrvr_env_trace_enable       = "MXOSRVR_TRACE_ENABLE";	 // enable trace
	const char *gp_mxosrvr_env_trace_ems          = "MXOSRVR_TRACE_EMS";     // trace what used to be sent to the legacy ems collector
	const char *gp_mxosrvr_env_trace_legacy       = "MXOSRVR_TRACE_LEGACY"; 
	bool        gv_mxosrvr_trace_enable = false;

	msg_getenv_bool(gp_mxosrvr_env_trace_enable, &gv_mxosrvr_trace_enable);

	if(gv_mxosrvr_trace_enable)
	{
		msg_getenv_bool(gp_mxosrvr_env_trace_ems, &gv_trace_ems_dll);
		msg_getenv_bool(gp_mxosrvr_env_trace_legacy, &gv_trace_legacy_dll);

		trace_init((char*)gp_mxosrvr_trace_filename,
                  true,
                  NULL,
                  false);

		if (hQpidEmsDll == NULL)
			bret = open_dll(QpidEmsDll);
		if (bret == true)
			CallSetTraceVariables(gv_trace_ems_dll,gv_trace_legacy_dll);
	}
	return 0;
}
int mxosrvr_seabed_trace_close_dll()
{
   trace_close();
}
#endif

#ifdef NSK_AS
int mxoas_init_seabed_trace_dll()
{
	bool bret = true;

	const char *gp_mxoas_trace_filename         = "mxoas_trace_";
	const char *gp_mxoas_env_trace_enable       = "MXOAS_TRACE_ENABLE";	 // enable trace
	const char *gp_mxoas_env_trace_ems          = "MXOAS_TRACE_EMS";     // trace what used to be sent to the legacy ems collector
	bool        gv_mxoas_trace_enable = false;

	msg_getenv_bool(gp_mxoas_env_trace_enable, &gv_mxoas_trace_enable);

	if(gv_mxoas_trace_enable)
	{
      msg_getenv_bool(gp_mxoas_env_trace_ems, &gv_trace_ems_dll);

      trace_init((char*)gp_mxoas_trace_filename,
                  true,
                  NULL,
                  false);

		if (hQpidEmsDll == NULL)
			bret = open_dll(QpidEmsDll);
		if (bret == true)
			CallSetTraceVariables(gv_trace_ems_dll, gv_trace_legacy_dll);
	}
	return 0;
}
int mxoas_seabed_trace_close_dll()
{
   trace_close();
}
#endif

#ifdef NSK_CFGSRVR
int mxocfg_init_seabed_trace_dll()
{
	bool bret = true;

	const char *gp_mxocfg_trace_filename         = "mxocfg_trace_";
	const char *gp_mxocfg_env_trace_enable       = "MXOCFG_TRACE_ENABLE";	 // enable trace
	const char *gp_mxocfg_env_trace_ems          = "MXOCFG_TRACE_EMS";       // trace what used to be sent to the legacy ems collector
	bool        gv_mxocfg_trace_enable = false;

	msg_getenv_bool(gp_mxocfg_env_trace_enable, &gv_mxocfg_trace_enable);

	if(gv_mxocfg_trace_enable)
	{
		msg_getenv_bool(gp_mxocfg_env_trace_ems, &gv_trace_ems_dll);

		trace_init((char*)gp_mxocfg_trace_filename,
                  true,
                  NULL,
                  false);

		if (hQpidEmsDll == NULL)
			bret = open_dll(QpidEmsDll);
		if (bret == true)
			CallSetTraceVariables(gv_trace_ems_dll,gv_trace_legacy_dll);
   }
   return 0;
}
int mxocfg_seabed_trace_close_dll()
{
   trace_close();
}
#endif


