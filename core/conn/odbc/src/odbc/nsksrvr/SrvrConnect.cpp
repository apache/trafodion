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
//
// MODULE: SrvrConnect.cpp
//
// PURPOSE: Implements the following methods
//	odbc_SQLSvc_InitializeDialogue_sme_
//  odbc_SQLSvc_SetConnectionOption_sme_
//  odbc_SQLSvc_TerminateDialogue_sme_
//  ImplInit
//		odbc_SQLSvc_Prepare_ame_
//		odbc_SQLSvc_ExecuteN_ame_
//		odbc_SQLSvc_Close_ame_
//		odbc_SQLSvc_FetchN_ame_
//		odbc_SQLSvc_EndTransaction_ame_
//		odbc_SQLSvc_SetDiagInfo_ame_
//		odbc_SQLSvc_ExecuteCall_ame_
//
//      odbc_SQLSrvr_Prepare_ame_
//      odbc_SQLSrvr_Fetch_ame_
//      odbc_SQLSrvr_Close_ame_
//
// HISTORY:
//  98/04/15 made changes to initializeDialogue for generation of
//           and use of the user identity at logon time.
//           changes primarily to allow for anonymous logon
//
//  MODIFICATION: Add trace messages -- 4/29/98
//
// 00/10/23 - change this to fix MS Access problem.
// Since MS Access does insert, update
// and delete while SQL_ACCESS_MODE is SQL_MODE_READ_ONLY.


#include <platform_ndcs.h>
#include <platform_utils.h>
#include <stdio.h>
#include <sql.h>
#include <sqlext.h>
#include <dlfcn.h>
#include <tal.h>

#include <string.h>
#include "DrvrSrvr.h"
#include "Global.h"
#include "QSGlobal.h"
#include "QSData.h"
#include "TransportBase.h"
#include "odbcCommon.h"
#include "odbc_sv.h"
#include "odbcas_cl.h"
#include "srvrcommon.h"
#include "sqlinterface.h"
#include "SQLWrapper.h"
#include "odbcMxSecurity.h"
#include "RegValues.h"
#include "CommonDiags.h"
#include "tdm_odbcSrvrMsg.h"
#include "RegValues.h"
#include "tmf_tipapi/ntiperror.h"

#include "ODBCMXTraceMsgs.h"
#include "SrvrConnect.h"
#include "commonFunctions.h"
#include <dlfcn.h>
#include "ResStatisticsSession.h"
#include "ResStatisticsStatement.h"
#include "ComDllload.h"
#include <dlfcn.h>
#include "secsrvrmxo.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef PERF_TEST	// Added for performance testing
#include "PerformanceMeasure.h"
PerformanceMeasure *perf = 0;
#endif

#include <arpa/inet.h>
#include <netinet/in.h>
#include "zookeeper/zookeeper.h"

#include <tr1/memory>
#include <pthread.h>

#include <queue>

extern zhandle_t *zh;
extern stringstream availSrvrNode;
extern string regSrvrData;
extern string dcsRegisteredNode;
extern string availSrvrData;
extern int shutdownThisThing;
extern char instanceId[8];
extern char zkRootNode[256];
extern int sdconn;
extern int clientConnTimeOut;
extern short stopOnDisconnect;
extern int aggrInterval;
extern int statisticsCacheSize;
extern int queryPubThreshold;
extern statistics_type statisticsPubType;
extern bool bStatisticsEnabled;
extern int myNid;
extern int myPid;
extern string myProcName;
extern bool bPlanEnabled;


extern long maxHeapPctExit;
extern long initSessMemSize ;
int fd = -1;
bool heapSizeExit = false;
int interval_count=0;
int interval_max=1;
int limit_count=0;
int limit_max=-1;
long long lastUpdatedTime = 0;

bool updateZKState(DCS_SERVER_STATE currState, DCS_SERVER_STATE newState);

static void free_String_vector(struct String_vector *v)
{
    if (v->data)
    {
        for (int32_t i=0; i < v->count; i++)
        {
            free(v->data[i]);
        }
        free(v->data);
        v->data = NULL;
        v->count = 0;
    }
}
void sync_string_completion(int rc, const char *name, const void *data)
{
	if( rc != ZOK )
	{
		char tmpString[1024];
		sprintf(tmpString, "sync_string_completion...Error %d calling zoo_async() for %s. Server exiting.", rc, (char *)data);
		SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			1, tmpString);
		SRVR::exitServerProcess();
	}
//	delete [] (char *)data;
}

short DO_WouldLikeToExecute(
	      IDL_char *stmtLabel
		, Long stmtHandle
		, IDL_long* returnCode
		, IDL_long* sqlWarningOrErrorLength
		, BYTE*& sqlWarningOrError
		);
short qrysrvc_ExecuteFinished(
	      const IDL_char *stmtLabel
	    , const Long stmtHandle
		, const bool bCheckSqlQueryType
		, const short error_code
		, const bool bFetch
		, const bool bException = false
		, const bool bErase = true
		);

extern char zkHost[256];
extern void sendAggrStats(pub_struct_type pub_type, std::tr1::shared_ptr<SESSION_AGGREGATION> pAggr_info);
extern void sendSessionStats(std::tr1::shared_ptr<SESSION_INFO> pSession_info);
extern void sendQueryStats(pub_struct_type pub_type, std::tr1::shared_ptr<STATEMENT_QUERYEXECUTION> pQuery_info);
CEE_handle_def StatisticsTimerHandle;
SRVR_STMT_HDL * pQueryStmt = NULL;

typedef struct _REPOS_STATS
{
	std::tr1::shared_ptr<SESSION_INFO> m_pSessionStats;
	std::tr1::shared_ptr<STATEMENT_QUERYEXECUTION> m_pQuery_stats;
	std::tr1::shared_ptr<SESSION_AGGREGATION> m_pAggr_stats;
	pub_struct_type m_pub_type;
}REPOS_STATS, *pREPOS_STATS;

#include "dbUserAuth.h" // to get the dbUser ID after authorization
#include "ndcsversion.h"


// Needed for bypassing checks in compiler once component privileges have been tested
// Internal calls - Defined in libcli.so

void SQL_EXEC_SetParserFlagsForExSqlComp_Internal( /*IN*/ unsigned int flagbits);
void SQL_EXEC_ResetParserFlagsForExSqlComp_Internal( /*IN*/ unsigned int flagbits);

Int32 SQL_EXEC_GetAuthState(
   /*OUT*/  bool &authenticationEnabled,
   /*OUT*/  bool &authorizationEnabled,
   /*OUT*/  bool &authorizationReady,
   /*OUT*/  bool &auditingEnabled);

#define SKIP_COMPRIV_CHECK 0x100000

#define MAX_EVAR_VALUE_LENGTH 3900 + 1

#define CHECK_QUERYTYPE(y) \
		(( y == SQL_SELECT_NON_UNIQUE || y == SQL_INSERT_NON_UNIQUE || \
		y == SQL_UPDATE_NON_UNIQUE || y == SQL_DELETE_NON_UNIQUE || y == 10000) ? TRUE : FALSE)

using namespace SRVR;

#include "ceercv.h"
// #ifndef NSK_CLPS_LIB
	#include "Transport.h"
	#include "FileSystemSrvr.h"
	#include "TCPIPSystemSrvr.h"
	#include "odbcs_srvr_res.h"
	#include "QSData.h"
	#include "commonFunctions.h"
// #endif

#include "NskUtil.h"
//LCOV_EXCL_START
extern void logError( short Code, short Severity, short Operation );

extern char errStrBuf1[], errStrBuf2[], errStrBuf3[], errStrBuf4[], errStrBuf5[];
//LCOV_EXCL_STOP

extern ResStatisticsSession    *resStatSession;
extern ResStatisticsStatement  *resStatStatement;
extern struct					collect_info setinit;


extern IDL_short tempSqlStmtType;
extern bool informas;
extern bool sqlflag;

extern bool securitySetup;

// Component privileges
bitmask_type wmsPrivMask;
bitmask_type hpdcsPrivMask;

extern void ClearAdaptiveSegment(short adapiveSeg = -1);


extern "C" void releaseGlobalBuffer();

///////////////////////////////////////////////////////////////

MonitorCallContext	*monitorCallContext = NULL;
CEE_handle_def	callIdStopServer;

char srvrSessionId[SESSION_ID_LEN];
char savedDefaultSchema[MAX_SQL_IDENTIFIER_LEN+3]; // this is to allow double quotes around the schema name

bool getSQLInfo(E_GetSQLInfoType option, long stmtHandle=NULL, char *stmtLabel=NULL );

bool loadPrivileges( char *component, bitmask_type mask);
void setPrivMask( short priv, bitmask_type bitMask );

// QSSYNC registered processes info
REG_PROC_INFO regProcInfo[256];

pthread_mutex_t Thread_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

template<class T>
class Repos_Queue
{
private:
    std::queue<T> my_queue;
    //boost::condition_variable_any cond;
    pthread_cond_t cond;
    pthread_mutex_t my_mutex;

public:
    Repos_Queue(){}
    ~Repos_Queue()
    {
    	pthread_cond_destroy(&cond);
    }

private:
    Repos_Queue(const Repos_Queue&);
    const Repos_Queue& operator=(const Repos_Queue&);

public:
    void push_task(const T & repos_stats)
    {
    //mutex lock
        //boost::unique_lock<pthread_mutex_t> lock(my_mutex);
        pthread_mutex_lock(&my_mutex);

        my_queue.push(repos_stats);
        //Notify other threads
        //cond.notify_one();
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&my_mutex);
    }
    T get_task()
    {
        //mutex lock
        //boost::unique_lock<pthread_mutex_t> lock(my_mutex);
        pthread_mutex_lock(&my_mutex);

        if(my_queue.size()==0)
        {
            //if no task in the queue, waite for mutex
            //cond.wait(lock);
            pthread_cond_wait(&cond,&my_mutex);
        }
        //point to head of the queue
        T repos_stats(my_queue.front());
        //dequeue
        my_queue.pop();
        pthread_mutex_unlock(&my_mutex);
        return repos_stats;
    }
    int get_size()
    {
        return my_queue.size();
    }

    bool isEmpty(){
        return my_queue.empty();
    }
};

static Repos_Queue<REPOS_STATS> repos_queue;
static bool record_session_done = true;

//0:None 1:update 2:insert/upsert cache limit 3:achieve timeline

#define REPOS_EXECUTE_NONE       0
#define REPOS_EXECUTE_UPDATE     1
#define REPOS_EXECUTE_CACHELIMIT 2
#define REPOS_EXECUTE_TIMELINE   3

static void* SessionWatchDog(void* arg)
{
	record_session_done = false;

	SRVR_STMT_HDL *pSrvrStmt = NULL;
	SQLCTX_HANDLE thread_context_handle = 0;
	char tmpString[128];

	int rc = pthread_mutex_lock(&Thread_mutex);
	if (rc != 0)
	{
		sprintf(tmpString, "Failed to acquire mutex lock for repository session: error code %d", rc);
		SendEventMsg(MSG_ODBC_NSK_ERROR,
		             EVENTLOG_ERROR_TYPE,
		             0,
		             ODBCMX_SERVER,
		             srvrGlobal->srvrObjRef,
		             1,
		             tmpString);
		record_session_done = true;
		return 0;
	}


	try
	{
		if (WSQL_EXEC_CreateContext(&thread_context_handle, NULL, 0) < 0)
		{
			SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
						0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
						1, "Failed to create new SQL context");
			record_session_done = true;
			pthread_mutex_unlock(&Thread_mutex);
			return 0;
		}

		SendEventMsg(MSG_SERVER_TRACE_INFO,
						  EVENTLOG_INFORMATION_TYPE,
						  srvrGlobal->nskASProcessInfo.processId,
						  ODBCMX_SERVICE,
						  srvrGlobal->srvrObjRef,
						  3,
						  srvrGlobal->sessionId,
						  "Created new SQL context",
						  "0");

		stringstream errMsg;
		string errStr;
		ERROR_DESC_def *p_buffer = NULL;
		int retcode;
		bool okToGo = true;
		stringstream ss;
		string execStr;

		retcode = WSQL_EXEC_SwitchContext(thread_context_handle, NULL);
		if (retcode < 0)
		{
			SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
									0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
									1, "Failed to switch to new SQL context");
			okToGo = false;
		}
		if (okToGo)
		{
			SQL_EXEC_SetParserFlagsForExSqlComp_Internal(0x20000);

			pSrvrStmt = getSrvrStmt("STMT_PUBLICATION", FALSE);
			if (pSrvrStmt != NULL)
			{
				pSrvrStmt->cleanupAll();
				pSrvrStmt->Close(SQL_DROP);
				pSrvrStmt = NULL;
			}
			if ((pSrvrStmt = getSrvrStmt("STMT_PUBLICATION", TRUE)) == NULL)
			{

				SendEventMsg(MSG_ODBC_NSK_ERROR,
				             EVENTLOG_ERROR_TYPE,
				             0,
				             ODBCMX_SERVER,
				             srvrGlobal->srvrObjRef,
				             1,
				             "Failed to allocate statement for repository publications"
				             );
				okToGo = false;
			}
		}

        if (okToGo)
		{
			retcode = pSrvrStmt->ExecDirect(NULL, "CONTROL QUERY DEFAULT traf_no_dtm_xn 'ON'", INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
			if (retcode < 0)
			{
				errMsg.str("");
				if(pSrvrStmt->sqlError.errorList._length > 0)
					p_buffer = pSrvrStmt->sqlError.errorList._buffer;
				else if(pSrvrStmt->sqlWarning._length > 0)
					p_buffer = pSrvrStmt->sqlWarning._buffer;
				if(p_buffer != NULL && p_buffer->errorText)
					errMsg << "Failed to skip transaction - " << p_buffer->errorText;
				else
					errMsg << "Failed to skip transaction - " << " no additional information";

				errStr = errMsg.str();
				SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
										0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
										1, errStr.c_str());
				okToGo = false;
			}
		}

		if (okToGo)
		{
			retcode = pSrvrStmt->ExecDirect(NULL, "CONTROL QUERY DEFAULT attempt_esp_parallelism 'OFF'", INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
			if (retcode < 0)
			{
				errMsg.str("");
				if(pSrvrStmt->sqlError.errorList._length > 0)
					p_buffer = pSrvrStmt->sqlError.errorList._buffer;
				else if(pSrvrStmt->sqlWarning._length > 0)
					p_buffer = pSrvrStmt->sqlWarning._buffer;
				if(p_buffer != NULL && p_buffer->errorText)
					errMsg << "Failed to disable ESP startup - " << p_buffer->errorText;
				else
					errMsg << "Failed to disable ESP startup - " << " no additional information";

				errStr = errMsg.str();
				SendEventMsg(MSG_ODBC_NSK_ERROR,
				            EVENTLOG_ERROR_TYPE,
				            0,
				            ODBCMX_SERVER,
				            srvrGlobal->srvrObjRef,
				            1,
				            errStr.c_str()
				            );

				okToGo = false;
			}
		}
        if (okToGo)
        {
            retcode = pSrvrStmt->ExecDirect(NULL, "CQD DETAILED_STATISTICS 'OFF'", INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
            if (retcode < 0)
            {
                errMsg.str("");
                if(pSrvrStmt->sqlError.errorList._length > 0)
                    p_buffer = pSrvrStmt->sqlError.errorList._buffer;
                else if(pSrvrStmt->sqlWarning._length > 0)
                    p_buffer = pSrvrStmt->sqlWarning._buffer;
                if(p_buffer != NULL && p_buffer->errorText)
                    errMsg << "Failed to turn off statistics - " << p_buffer->errorText;
                else
                    errMsg << "Failed to turn off statistics - " << " no additional information";

                errStr = errMsg.str();
                SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                                        0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                                        1, errStr.c_str());
                okToGo = false;
            }
        }
        if (okToGo)
        {
            retcode = pSrvrStmt->ExecDirect(NULL, "CQD HIST_MISSING_STATS_WARNING_LEVEL '0'", INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
            if (retcode < 0)
            {
                errMsg.str("");
                if(pSrvrStmt->sqlError.errorList._length > 0)
                    p_buffer = pSrvrStmt->sqlError.errorList._buffer;
                else if(pSrvrStmt->sqlWarning._length > 0)
                    p_buffer = pSrvrStmt->sqlWarning._buffer;
                if(p_buffer != NULL && p_buffer->errorText)
                    errMsg << "Failed to turn off missing statistics warning - " << p_buffer->errorText;
                else
                    errMsg << "Failed to turn off missing statistics warning - " << " no additional information";

                errStr = errMsg.str();
                SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                                        0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                                        1, errStr.c_str());
                okToGo = false;
            }
        }
        vector< vector<string> > query_list;
        vector<string> session_start;
        vector<string> statement_new_query;
        vector<string> session_stat_aggregation;

        session_start.push_back("upsert into Trafodion.\"_REPOS_\".metric_session_table values");
        statement_new_query.push_back("insert into Trafodion.\"_REPOS_\".metric_query_table values");
        session_stat_aggregation.push_back("insert into Trafodion.\"_REPOS_\".metric_query_aggr_table values");

        query_list.push_back(session_start);
        query_list.push_back(statement_new_query);
        query_list.push_back(session_stat_aggregation);

        int query_limit = statisticsCacheSize;
        int time_limit = aggrInterval;
        //0:None 1:update 2:insert/upsert cache limit 3:achieve timeline
        int execute_flag = REPOS_EXECUTE_NONE;
        int sleep_count = 0;

        REPOS_STATS repos_stats;
        while(!record_session_done && okToGo)
        {
            sleep_count = 0;
            while(repos_queue.isEmpty() && (sleep_count < time_limit)){
                sleep(1);
                sleep_count++;
            }

            if(!repos_queue.isEmpty()){
                repos_stats = repos_queue.get_task();
            }else{
                // go to executeDirect
                execute_flag = REPOS_EXECUTE_TIMELINE;
                goto execute;
            }

			ss.str("");
			ss.clear();

			if (repos_stats.m_pub_type == PUB_TYPE_SESSION_START || repos_stats.m_pub_type == PUB_TYPE_SESSION_END)
			{
				std::tr1::shared_ptr<SESSION_INFO> pSessionInfo = repos_stats.m_pSessionStats;
				if(NULL == pSessionInfo)
				{
					SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
							0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
							1, "Invalid data pointer founded in SessionWatchDog()");
					break;
				}

				//upsert into Trafodion.\"_REPOS_\".metric_session_table values
				ss << "(";
				ss << pSessionInfo->m_instance_id << ",";
				ss << pSessionInfo->m_tenant_id << ",";
				ss << pSessionInfo->m_component_id << ",";
				ss << pSessionInfo->m_process_id << ",";
				ss << pSessionInfo->m_thread_id << ",";
				ss << pSessionInfo->m_node_id << ",";
				ss << pSessionInfo->m_pnid_id << ",";
				ss << pSessionInfo->m_host_id << ",'";
				ss << pSessionInfo->m_ip_address_id.c_str() << "',";
				ss << pSessionInfo->m_sequence_number << ",'";
				ss << pSessionInfo->m_process_name.c_str() << "','";
				ss << pSessionInfo->m_sessionId.c_str() << "','";
				ss << pSessionInfo->m_session_status.c_str() << "',CONVERTTIMESTAMP(";
				ss << pSessionInfo->m_session_start_utc_ts << "),";
                                if (pSessionInfo->m_session_end_utc_ts > 0)
                                        ss << "CONVERTTIMESTAMP(" << pSessionInfo->m_session_end_utc_ts << "),";
                                else
                                        ss << "NULL,";
				ss << pSessionInfo->m_user_id << ",'";
				ss << pSessionInfo->m_user_name.c_str() << "','";
				ss << pSessionInfo->m_role_name.c_str() << "','";
				ss << pSessionInfo->m_client_name.c_str() << "','";
				ss << pSessionInfo->m_client_user_name.c_str() << "','";
				ss << pSessionInfo->m_application_name.c_str() << "','";
 				ss << pSessionInfo->m_profile_name.c_str() << "','";
                                ss << pSessionInfo->m_sla_name.c_str() << "',";
				ss << pSessionInfo->m_total_odbc_exection_time << ",";
				ss << pSessionInfo->m_total_odbc_elapsed_time << ",";
				ss << pSessionInfo->m_total_insert_stmts_executed << ",";
				ss << pSessionInfo->m_total_delete_stmts_executed << ",";
				ss << pSessionInfo->m_total_update_stmts_executed << ",";
				ss << pSessionInfo->m_total_select_stmts_executed << ",";
				ss << pSessionInfo->m_total_catalog_stmts << ",";
				ss << pSessionInfo->m_total_prepares << ",";
				ss << pSessionInfo->m_total_executes << ",";
				ss << pSessionInfo->m_total_fetches << ",";
				ss << pSessionInfo->m_total_closes << ",";
				ss << pSessionInfo->m_total_execdirects << ",";
				ss << pSessionInfo->m_total_errors << ",";
				ss << pSessionInfo->m_total_warnings << ",";
				ss << pSessionInfo->m_total_login_elapsed_time_mcsec << ",";
				ss << pSessionInfo->m_ldap_login_elapsed_time_mcsec << ",";
				ss << pSessionInfo->m_sql_user_elapsed_time_mcsec << ",";
				ss << pSessionInfo->m_search_connection_elapsed_time_mcsec << ",";
				ss << pSessionInfo->m_search_elapsed_time_mcsec << ",";
				ss << pSessionInfo->m_authentication_connection_elapsed_time_mcsec << ",";
				ss << pSessionInfo->m_authentication_elapsed_time_mcsec << ")";

                execute_flag = REPOS_EXECUTE_NONE;
                vector<string> *tmp = &query_list.at(0);
                (*tmp).push_back(ss.str());
                if ((*tmp).size() > query_limit)
                {
                    //go executeDirect
                    execute_flag = REPOS_EXECUTE_CACHELIMIT;
                    execStr ="";
                    vector<string>::iterator it = (*tmp).begin();
                    execStr += *it +  *(it + 1);
                    it += 2;
                    for(; it != (*tmp).end(); it++)
                    {
                        execStr+= "," + *it;
                    }
                    (*tmp).erase((*tmp).begin() + 1, (*tmp).end());
                }

			}
			else if (repos_stats.m_pub_type == PUB_TYPE_STATEMENT_NEW_QUERYEXECUTION)
			{
				std::tr1::shared_ptr<STATEMENT_QUERYEXECUTION> pQueryAdd = repos_stats.m_pQuery_stats;
				if(NULL == pQueryAdd)
				{
					SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                                                        0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                                                        1, "Invalid data pointer founded in SessionWatchDog()");
					break;
				}
                                lastUpdatedTime = JULIANTIMESTAMP();

				//ss << "insert into Trafodion.\"_REPOS_\".metric_query_table values(";
                                ss << "(";
				ss << pQueryAdd->m_instance_id << ",";
				ss << pQueryAdd->m_tenant_id << ",";
				ss << pQueryAdd->m_component_id << ",";
				ss << pQueryAdd->m_process_id << ",";
				ss << pQueryAdd->m_thread_id << ",";
				ss << pQueryAdd->m_node_id << ",";
				ss << pQueryAdd->m_pnid_id << ",";
				ss << pQueryAdd->m_host_id << ",'";
				ss << pQueryAdd->m_ip_address_id.c_str() << "',";
				ss << pQueryAdd->m_sequence_number << ",'";
				ss << pQueryAdd->m_process_name.c_str() << "',CONVERTTIMESTAMP(";
				ss << pQueryAdd->m_exec_start_utc_ts << "),'";
				ss << pQueryAdd->m_query_id.c_str() << "','";
				ss << pQueryAdd->m_query_signature_id.c_str() << "','";
				ss << pQueryAdd->m_user_name.c_str() << "','";
				ss << pQueryAdd->m_role_name.c_str() << "',";
				ss << pQueryAdd->m_start_priority << ",'";
				ss << pQueryAdd->m_master_process_id.c_str() << "','";
				ss << pQueryAdd->m_session_id.c_str() << "','";
				ss << pQueryAdd->m_client_name.c_str() << "','";
				ss << pQueryAdd->m_application_name.c_str() << "','";
				ss << pQueryAdd->m_statement_id.c_str() << "','";
				ss << pQueryAdd->m_statement_type.c_str() << "','";
				ss << pQueryAdd->m_statement_subtype.c_str() << "',";
				if (pQueryAdd->m_submit_utc_ts > 0)
					ss << "CONVERTTIMESTAMP(" << pQueryAdd->m_submit_utc_ts << "),";
				else
					ss << "NULL,";
				if (pQueryAdd->m_compile_start_utc_ts > 0)
					ss << "CONVERTTIMESTAMP(" << pQueryAdd->m_compile_start_utc_ts << "),";
				else
					ss << "NULL,";
				if (pQueryAdd->m_compile_end_utc_ts > 0)
					ss << "CONVERTTIMESTAMP(" << pQueryAdd->m_compile_end_utc_ts << "),";
				else
					ss << "NULL,";
				ss << pQueryAdd->m_compile_elapsed_time << ",";
				ss << pQueryAdd->m_cmp_affinity_num << ",";
				ss << pQueryAdd->m_cmp_dop << ",";
				ss << pQueryAdd->m_cmp_txn_needed << ",";
				ss << pQueryAdd->m_cmp_mandatory_x_prod << ",";
				ss << pQueryAdd->m_cmp_missing_stats << ",";
				ss << pQueryAdd->m_cmp_num_joins << ",";
				ss << pQueryAdd->m_cmp_full_scan_on_table << ",";
				ss << pQueryAdd->m_cmp_rows_accessed_full_scan << ",";
				ss << pQueryAdd->m_est_accessed_rows << ",";
				ss << pQueryAdd->m_est_used_rows << ",'";
				ss << pQueryAdd->m_cmp_compiler_id.c_str() << "',";
				ss << pQueryAdd->m_cmp_cpu_path_length << ",";
				ss << pQueryAdd->m_cmp_cpu_binder << ",";
				ss << pQueryAdd->m_cmp_cpu_normalizer << ",";
				ss << pQueryAdd->m_cmp_cpu_analyzer << ",";
				ss << pQueryAdd->m_cmp_cpu_optimizer << ",";
				ss << pQueryAdd->m_cmp_cpu_generator << ",";
				ss << pQueryAdd->m_cmp_metadata_cache_hits << ",";
				ss << pQueryAdd->m_cmp_metadata_cache_lookups << ",";
				ss << pQueryAdd->m_cmp_query_cache_status << ",";
				ss << pQueryAdd->m_cmp_histogram_cache_hits << ",";
				ss << pQueryAdd->m_cmp_histogram_cache_lookups << ",";
				ss << pQueryAdd->m_cmp_stmt_heap_size << ",";
				ss << pQueryAdd->m_cmp_context_heap_size << ",";
				ss << pQueryAdd->m_cmp_optimization_tasks << ",";
				ss << pQueryAdd->m_cmp_optimization_contexts << ",";
				ss << pQueryAdd->m_cmp_is_recompile << ",";
				ss << pQueryAdd->m_est_num_seq_ios << ",";
				ss << pQueryAdd->m_est_num_rand_ios << ",";
				ss << pQueryAdd->m_est_cost << ",";
				ss << pQueryAdd->m_est_cardinality << ",";
				ss << pQueryAdd->m_est_io_time << ",";
				ss << pQueryAdd->m_est_msg_time << ",";
				ss << pQueryAdd->m_est_idle_time << ",";
				ss << pQueryAdd->m_est_cpu_time << ",";
				ss << pQueryAdd->m_est_total_time << ",";
				ss << pQueryAdd->m_est_total_mem << ",";
				ss << pQueryAdd->m_est_resource_usage << ",'";
				ss << pQueryAdd->m_aggregation_option.c_str() << "',";
				ss << pQueryAdd->m_cmp_number_of_bmos << ",'";
				ss << pQueryAdd->m_cmp_overflow_mode.c_str() << "',";
				ss << pQueryAdd->m_cmp_overflow_size << ",";
				ss << pQueryAdd->m_aggregate_total << ",";
				ss << pQueryAdd->m_stats_error_code << ",";
				ss << pQueryAdd->m_query_elapsed_time << ",";
				ss << pQueryAdd->m_sql_process_busy_time << ",";
				ss << pQueryAdd->m_disk_process_busy_time << ",";
				ss << pQueryAdd->m_disk_ios << ",";
				ss << pQueryAdd->m_num_sql_processes << ",";
				ss << pQueryAdd->m_sql_space_allocated << ",";
				ss << pQueryAdd->m_sql_space_used << ",";
				ss << pQueryAdd->m_sql_heap_allocated << ",";
				ss << pQueryAdd->m_sql_heap_used << ",";
				ss << pQueryAdd->m_total_mem_alloc << ",";
				ss << pQueryAdd->m_max_mem_used << ",'";
				ss << pQueryAdd->m_transaction_id.c_str() << "',";
				ss << pQueryAdd->m_num_request_msgs << ",";
				ss << pQueryAdd->m_num_request_msg_bytes << ",";
				ss << pQueryAdd->m_num_reply_msgs << ",";
				ss << pQueryAdd->m_num_reply_msg_bytes << ",";
				if (pQueryAdd->m_first_result_return_utc_ts > 0)
					ss << "CONVERTTIMESTAMP(" << pQueryAdd->m_first_result_return_utc_ts << "),";
				else
					ss << "NULL,";
				ss << pQueryAdd->m_rows_returned_to_master << ",'";
				ss << pQueryAdd->m_parent_query_id.c_str() << "','";
				ss << pQueryAdd->m_parent_system_name.c_str() << "',";
				if (pQueryAdd->m_exec_end_utc_ts > 0)
					ss << "CONVERTTIMESTAMP(" << pQueryAdd->m_exec_end_utc_ts << "),";
				else
					ss << "NULL,";
				ss << pQueryAdd->m_master_execution_time << ",";
				ss << pQueryAdd->m_master_elapse_time << ",'";
				ss << pQueryAdd->m_query_status << "','";
				ss << pQueryAdd->m_query_sub_status << "',";
				ss << pQueryAdd->m_error_code << ",";
				ss << pQueryAdd->m_sql_error_code << ",'";
				ss << pQueryAdd->m_error_text.c_str() << "','";
				ss << pQueryAdd->m_query_text.c_str() << "','";
				ss << "',";	// Explain plan. Updated later below using a CLI call
				ss << pQueryAdd->m_last_error_before_aqr << ",";
				ss << pQueryAdd->m_delay_time_before_aqr_sec << ",";
				ss << pQueryAdd->m_total_num_aqr_retries << ",";
				ss << pQueryAdd->m_msg_bytes_to_disk << ",";
				ss << pQueryAdd->m_msgs_to_disk << ",";
				ss << pQueryAdd->m_rows_accessed << ",";
				ss << pQueryAdd->m_rows_retrieved << ",";
				ss << pQueryAdd->m_num_rows_iud << ",";
				ss << pQueryAdd->m_processes_created << ",";
				ss << pQueryAdd->m_process_create_busy_time << ",";
				ss << pQueryAdd->m_ovf_file_count << ",";
				ss << pQueryAdd->m_ovf_space_allocated << ",";
				ss << pQueryAdd->m_ovf_space_used << ",";
				ss << pQueryAdd->m_ovf_block_size << ",";
				ss << pQueryAdd->m_ovf_write_read_count << ",";
				ss << pQueryAdd->m_ovf_write_count << ",";
				ss << pQueryAdd->m_ovf_buffer_blocks_written << ",";
				ss << pQueryAdd->m_ovf_buffer_bytes_written << ",";
				ss << pQueryAdd->m_ovf_read_count << ",";
				ss << pQueryAdd->m_ovf_buffer_blocks_read << ",";
				ss << pQueryAdd->m_ovf_buffer_bytes_read << ",";
				ss << pQueryAdd->m_num_nodes << ",";
				ss << pQueryAdd->m_udr_process_busy_time << ",";
				ss << pQueryAdd->m_pertable_stats << ",";
				if ( lastUpdatedTime > 0)
                                        ss << "CONVERTTIMESTAMP(" << lastUpdatedTime << ")";
                                else
                                        ss << "NULL";

                                ss <<")";

                                execute_flag = REPOS_EXECUTE_NONE;
                                vector<string> *tmp = &query_list.at(1);
                                (*tmp).push_back(ss.str());
                                if ((*tmp).size() > query_limit)
                                {
                                    //go executeDirect
                                    execute_flag = REPOS_EXECUTE_CACHELIMIT;
                                    execStr ="";
                                    vector<string>::iterator it = (*tmp).begin();
                                    execStr += *it;
                                    execStr += *(it + 1);
                                    it += 2;
                                    for(; it != (*tmp).end(); it++)
                                    {
                                        execStr+= "," + *it;
                                    }
                                    (*tmp).erase((*tmp).begin() + 1, (*tmp).end());
                                }

			}
			else if (repos_stats.m_pub_type == PUB_TYPE_STATEMENT_UPDATE_QUERYEXECUTION)
			{
				std::tr1::shared_ptr<STATEMENT_QUERYEXECUTION> pQueryUpdate = repos_stats.m_pQuery_stats;
				if(NULL == pQueryUpdate)
				{
					SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                                                        0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                                                        1, "Invalid data pointer founded in SessionWatchDog()");
					break;
				}

                                lastUpdatedTime = JULIANTIMESTAMP();
				ss << "update Trafodion.\"_REPOS_\".metric_query_table ";
				ss << "set STATEMENT_TYPE= '" << pQueryUpdate->m_statement_type.c_str() << "',";
				ss << "STATEMENT_SUBTYPE= '" << pQueryUpdate->m_statement_subtype.c_str() << "',";
				ss << "AGGREGATE_TOTAL= " << pQueryUpdate->m_aggregate_total << ",";
				ss << "STATS_ERROR_CODE= " << pQueryUpdate->m_stats_error_code << ",";
				ss << "QUERY_ELAPSED_TIME= " << pQueryUpdate->m_query_elapsed_time << ",";
				ss << "SQL_PROCESS_BUSY_TIME= " << pQueryUpdate->m_sql_process_busy_time << ",";
				ss << "DISK_PROCESS_BUSY_TIME= " << pQueryUpdate->m_disk_process_busy_time << ",";
				ss << "DISK_IOS= " << pQueryUpdate->m_disk_ios << ",";
				ss << "NUM_SQL_PROCESSES= " << pQueryUpdate->m_num_sql_processes << ",";
				ss << "SQL_SPACE_ALLOCATED= " << pQueryUpdate->m_sql_space_allocated << ",";
				ss << "SQL_SPACE_USED= " << pQueryUpdate->m_sql_space_used << ",";
				ss << "SQL_HEAP_ALLOCATED= " << pQueryUpdate->m_sql_heap_allocated << ",";
				ss << "SQL_HEAP_USED= " << pQueryUpdate->m_sql_heap_used << ",";
				ss << "TOTAL_MEM_ALLOC= " << pQueryUpdate->m_total_mem_alloc << ",";
				ss << "MAX_MEM_USED= " << pQueryUpdate->m_max_mem_used << ",";
				ss << "NUM_REQUEST_MSGS= " << pQueryUpdate->m_num_request_msgs << ",";
				ss << "NUM_REQUEST_MSG_BYTES= " << pQueryUpdate->m_num_request_msg_bytes << ",";
				ss << "NUM_REPLY_MSGS= " << pQueryUpdate->m_num_reply_msgs << ",";
				ss << "NUM_REPLY_MSG_BYTES= " << pQueryUpdate->m_num_reply_msg_bytes << ",";
				if (pQueryUpdate->m_first_result_return_utc_ts > 0)
					ss << "FIRST_RESULT_RETURN_UTC_TS= CONVERTTIMESTAMP(" << pQueryUpdate->m_first_result_return_utc_ts << "),";
				ss << "ROWS_RETURNED_TO_MASTER= " << pQueryUpdate->m_rows_returned_to_master << ",";
				if (pQueryUpdate->m_exec_end_utc_ts > 0)
					ss << "EXEC_END_UTC_TS= CONVERTTIMESTAMP(" << pQueryUpdate->m_exec_end_utc_ts << "),";
				ss << "MASTER_EXECUTION_TIME= " << pQueryUpdate->m_master_execution_time << ",";
				ss << "MASTER_ELAPSED_TIME= " << pQueryUpdate->m_master_elapse_time << ",";
				ss << "QUERY_STATUS= '" << pQueryUpdate->m_query_status << "',";
				ss << "QUERY_SUB_STATUS= '" << pQueryUpdate->m_query_sub_status << "',";
				ss << "ERROR_CODE= " << pQueryUpdate->m_error_code << ",";
				ss << "SQL_ERROR_CODE= " << pQueryUpdate->m_sql_error_code << ",";
				ss << "ERROR_TEXT= '" << pQueryUpdate->m_error_text.c_str()<< "',";
				ss << "LAST_ERROR_BEFORE_AQR= " << pQueryUpdate->m_last_error_before_aqr << ",";
				ss << "DELAY_TIME_BEFORE_AQR_SEC= " << pQueryUpdate->m_delay_time_before_aqr_sec << ",";
				ss << "TOTAL_NUM_AQR_RETRIES= " << pQueryUpdate->m_total_num_aqr_retries << ",";
				ss << "MSG_BYTES_TO_DISK= " << pQueryUpdate->m_msg_bytes_to_disk << ",";
				ss << "MSGS_TO_DISK= " << pQueryUpdate->m_msgs_to_disk << ",";
				ss << "ROWS_ACCESSED= " << pQueryUpdate->m_rows_accessed << ",";
				ss << "ROWS_RETRIEVED= " << pQueryUpdate->m_rows_retrieved << ",";
				ss << "NUM_ROWS_IUD= " << pQueryUpdate->m_num_rows_iud << ",";
				ss << "PROCESSES_CREATED= " << pQueryUpdate->m_processes_created << ",";
				ss << "PROCESS_CREATE_BUSY_TIME= " << pQueryUpdate->m_process_create_busy_time << ",";
				ss << "OVF_FILE_COUNT= " << pQueryUpdate->m_ovf_file_count << ",";
				ss << "OVF_SPACE_ALLOCATED= " << pQueryUpdate->m_ovf_space_allocated << ",";
				ss << "OVF_SPACE_USED= " << pQueryUpdate->m_ovf_space_used << ",";
				ss << "OVF_BLOCK_SIZE= " << pQueryUpdate->m_ovf_block_size << ",";
				ss << "OVF_WRITE_READ_COUNT= " << pQueryUpdate->m_ovf_write_read_count << ",";
				ss << "OVF_WRITE_COUNT= " << pQueryUpdate->m_ovf_write_count << ",";
				ss << "OVF_BUFFER_BLOCKS_WRITTEN= " << pQueryUpdate->m_ovf_buffer_blocks_written << ",";
				ss << "OVF_BUFFER_BYTES_WRITTEN= " << pQueryUpdate->m_ovf_buffer_bytes_written << ",";
				ss << "OVF_READ_COUNT= " << pQueryUpdate->m_ovf_read_count << ",";
				ss << "OVF_BUFFER_BLOCKS_READ= " << pQueryUpdate->m_ovf_buffer_blocks_read << ",";
				ss << "OVF_BUFFER_BYTES_READ= " << pQueryUpdate->m_ovf_buffer_bytes_read << ",";
				ss << "NUM_NODES= " << pQueryUpdate->m_num_nodes << ",";
				ss << "UDR_PROCESS_BUSY_TIME= " << pQueryUpdate->m_udr_process_busy_time << ",";
				ss << "PERTABLE_STATS= " << pQueryUpdate->m_pertable_stats << ",";
                                ss << "LAST_UPDATED_TIME= CONVERTTIMESTAMP(" << lastUpdatedTime << ")";
				ss << " where QUERY_ID = '" << pQueryUpdate->m_query_id.c_str() << "'";
				ss << " and EXEC_START_UTC_TS = CONVERTTIMESTAMP(" << pQueryUpdate->m_exec_start_utc_ts << ")";


                                //go executeDirect
                                execute_flag = REPOS_EXECUTE_UPDATE;
                                execStr = ss.str();
			}
			else if (repos_stats.m_pub_type == PUB_TYPE_SESSION_START_AGGREGATION)
			{
				std::tr1::shared_ptr<SESSION_AGGREGATION> pAggrStat = repos_stats.m_pAggr_stats;
				if(NULL == pAggrStat)
				{
					SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                                                        0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                                                        1, "Invalid data pointer founded in SessionWatchDog()");
					break;
				}

				ss << "(";
				ss << pAggrStat->m_instance_id << ",";
				ss << pAggrStat->m_tenant_id << ",";
				ss << pAggrStat->m_component_id << ",";
				ss << pAggrStat->m_process_id << ",";
				ss << pAggrStat->m_thread_id << ",";
				ss << pAggrStat->m_node_id << ",";
				ss << pAggrStat->m_pnid_id << ",";
				ss << pAggrStat->m_host_id << ",'";
				ss << pAggrStat->m_ip_address_id.c_str() << "',";
				ss << pAggrStat->m_sequence_number << ",'";
				ss << pAggrStat->m_process_name.c_str() << "','";
				ss << pAggrStat->m_sessionId.c_str() << "',CONVERTTIMESTAMP(";
				ss << pAggrStat->m_session_start_utc_ts << "),CONVERTTIMESTAMP(";
				ss << pAggrStat->m_aggregation_last_update_utc_ts << "),";
				ss << pAggrStat->m_aggregation_last_elapsed_time << ",";
				ss << pAggrStat->m_user_id << ",'";
				ss << pAggrStat->m_user_name.c_str() << "','";
				ss << pAggrStat->m_role_name.c_str() << "','";
				ss << pAggrStat->m_client_name.c_str() << "','";
				ss << pAggrStat->m_client_user_name.c_str() << "','";
				ss << pAggrStat->m_application_name.c_str() << "',";
				ss << pAggrStat->m_total_est_rows_accessed << ",";
				ss << pAggrStat->m_total_est_rows_used << ",";
				ss << pAggrStat->m_total_rows_retrieved << ",";
				ss << pAggrStat->m_total_num_rows_iud << ",";
				ss << pAggrStat->m_total_selects << ",";
				ss << pAggrStat->m_total_inserts << ",";
				ss << pAggrStat->m_total_updates << ",";
				ss << pAggrStat->m_total_deletes << ",";
				ss << pAggrStat->m_total_ddl_stmts << ",";
				ss << pAggrStat->m_total_util_stmts << ",";
				ss << pAggrStat->m_total_catalog_stmts << ",";
				ss << pAggrStat->m_total_other_stmts << ",";
				ss << pAggrStat->m_total_insert_errors << ",";
				ss << pAggrStat->m_total_delete_errors << ",";
				ss << pAggrStat->m_total_update_errors << ",";
				ss << pAggrStat->m_total_select_errors << ",";
				ss << pAggrStat->m_total_ddl_errors << ",";
				ss << pAggrStat->m_total_util_errors << ",";
				ss << pAggrStat->m_total_catalog_errors << ",";
				ss << pAggrStat->m_total_other_errors << ",";
				ss << pAggrStat->m_delta_estimated_rows_accessed << ",";
				ss << pAggrStat->m_delta_estimated_rows_used << ",";
				ss << pAggrStat->m_delta_rows_accessed << ",";
				ss << pAggrStat->m_delta_rows_retrieved << ",";
				ss << pAggrStat->m_delta_num_rows_iud << ",";
				ss << pAggrStat->m_delta_total_selects << ",";
				ss << pAggrStat->m_delta_total_inserts << ",";
				ss << pAggrStat->m_delta_total_updates << ",";
				ss << pAggrStat->m_delta_total_deletes << ",";
				ss << pAggrStat->m_delta_total_ddl_stmts << ",";
				ss << pAggrStat->m_delta_total_util_stmts << ",";
				ss << pAggrStat->m_delta_total_catalog_stmts << ",";
				ss << pAggrStat->m_delta_total_other_stmts << ",";
				ss << pAggrStat->m_delta_insert_errors << ",";
				ss << pAggrStat->m_delta_delete_errors << ",";
				ss << pAggrStat->m_delta_update_errors << ",";
				ss << pAggrStat->m_delta_select_errors << ",";
				ss << pAggrStat->m_delta_ddl_errors << ",";
				ss << pAggrStat->m_delta_util_errors << ",";
				ss << pAggrStat->m_delta_catalog_errors << ",";
				ss << pAggrStat->m_delta_other_errors << ",";
				ss << pAggrStat->m_average_response_time << ",";
                ss << pAggrStat->m_throughput_per_sec << ")";

                execute_flag = REPOS_EXECUTE_NONE;
                vector<string> *tmp = &query_list.at(2);
                (*tmp).push_back(ss.str());
                if ((*tmp).size() > query_limit)
                {
                    //go executeDirect
                    execute_flag = REPOS_EXECUTE_CACHELIMIT;
                    execStr ="";
                    vector<string>::iterator it = (*tmp).begin();
                    execStr += *it + *(it + 1);
                    it += 2;
                    for(; it != (*tmp).end(); it++)
                    {
                        execStr+= "," + *it;
                    }
                    (*tmp).erase((*tmp).begin() + 1, (*tmp).end());
                }

            }
			else if (repos_stats.m_pub_type == PUB_TYPE_SESSION_UPDATE_AGGREGATION || repos_stats.m_pub_type == PUB_TYPE_SESSION_END_AGGREGATION)
			{
				std::tr1::shared_ptr<SESSION_AGGREGATION> pAggrStat = repos_stats.m_pAggr_stats;
				if(NULL == pAggrStat)
				{
					SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                                                        0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                                                        1, "Invalid data pointer founded in SessionWatchDog()");
					break;
				}

				ss << "update Trafodion.\"_REPOS_\".metric_query_aggr_table ";

				ss << "set AGGREGATION_LAST_UPDATE_UTC_TS = CONVERTTIMESTAMP(" << pAggrStat->m_aggregation_last_update_utc_ts << "),";
				ss << "AGGREGATION_LAST_ELAPSED_TIME = " << pAggrStat->m_aggregation_last_elapsed_time << ",";
				ss << "TOTAL_EST_ROWS_ACCESSED = " << pAggrStat->m_total_est_rows_accessed << ",";
				ss << "TOTAL_EST_ROWS_USED = " << pAggrStat->m_total_est_rows_used << ",";
				ss << "TOTAL_ROWS_RETRIEVED = " << pAggrStat->m_total_rows_retrieved << ",";
				ss << "TOTAL_NUM_ROWS_IUD = " << pAggrStat->m_total_num_rows_iud << ",";
				ss << "TOTAL_SELECTS = " << pAggrStat->m_total_selects << ",";
				ss << "TOTAL_INSERTS = " << pAggrStat->m_total_inserts << ",";
				ss << "TOTAL_UPDATES = " << pAggrStat->m_total_updates << ",";
				ss << "TOTAL_DELETES = " << pAggrStat->m_total_deletes << ",";
				ss << "TOTAL_DDL_STMTS = " << pAggrStat->m_total_ddl_stmts << ",";
				ss << "TOTAL_UTIL_STMTS = " << pAggrStat->m_total_util_stmts << ",";
				ss << "TOTAL_CATALOG_STMTS = " << pAggrStat->m_total_catalog_stmts << ",";
				ss << "TOTAL_OTHER_STMTS = " << pAggrStat->m_total_other_stmts << ",";
				ss << "TOTAL_INSERT_ERRORS = " << pAggrStat->m_total_insert_errors << ",";
				ss << "TOTAL_DELETE_ERRORS = " << pAggrStat->m_total_delete_errors << ",";
				ss << "TOTAL_UPDATE_ERRORS = " << pAggrStat->m_total_update_errors << ",";
				ss << "TOTAL_SELECT_ERRORS = " << pAggrStat->m_total_select_errors << ",";
				ss << "TOTAL_DDL_ERRORS = " << pAggrStat->m_total_ddl_errors << ",";
				ss << "TOTAL_UTIL_ERRORS = " << pAggrStat->m_total_util_errors << ",";
				ss << "TOTAL_CATALOG_ERRORS = " << pAggrStat->m_total_catalog_errors << ",";
				ss << "TOTAL_OTHER_ERRORS = " << pAggrStat->m_total_other_errors << ",";
				ss << "DELTA_ESTIMATED_ROWS_ACCESSED = " << pAggrStat->m_delta_estimated_rows_accessed << ",";
				ss << "DELTA_ESTIMATED_ROWS_USED = " << pAggrStat->m_delta_estimated_rows_used << ",";
				ss << "DELTA_ROWS_ACCESSED = " << pAggrStat->m_delta_rows_accessed << ",";
				ss << "DELTA_ROWS_RETRIEVED = " << pAggrStat->m_delta_rows_retrieved << ",";
				ss << "DELTA_NUM_ROWS_IUD = " << pAggrStat->m_delta_num_rows_iud << ",";
				ss << "DELTA_SELECTS = " << pAggrStat->m_delta_total_selects << ",";
				ss << "DELTA_INSERTS = " << pAggrStat->m_delta_total_inserts << ",";
				ss << "DELTA_UPDATES = " << pAggrStat->m_delta_total_updates << ",";
				ss << "DELTA_DELETES = " << pAggrStat->m_delta_total_deletes << ",";
				ss << "DELTA_DDL_STMTS = " << pAggrStat->m_delta_total_ddl_stmts << ",";
				ss << "DELTA_UTIL_STMTS = " << pAggrStat->m_delta_total_util_stmts << ",";
				ss << "DELTA_CATALOG_STMTS = " << pAggrStat->m_delta_total_catalog_stmts << ",";
				ss << "DELTA_OTHER_STMTS = " << pAggrStat->m_delta_total_other_stmts << ",";
				ss << "DELTA_INSERT_ERRORS = " << pAggrStat->m_delta_insert_errors << ",";
				ss << "DELTA_DELETE_ERRORS = " << pAggrStat->m_delta_delete_errors << ",";
				ss << "DELTA_UPDATE_ERRORS = " << pAggrStat->m_delta_update_errors << ",";
				ss << "DELTA_SELECT_ERRORS = " << pAggrStat->m_delta_select_errors << ",";
				ss << "DELTA_DDL_ERRORS = " << pAggrStat->m_delta_ddl_errors << ",";
				ss << "DELTA_UTIL_ERRORS = " << pAggrStat->m_delta_util_errors << ",";
				ss << "DELTA_CATALOG_ERRORS = " << pAggrStat->m_delta_catalog_errors << ",";
				ss << "DELTA_OTHER_ERRORS = " << pAggrStat->m_delta_other_errors << ",";
				ss << "AVERAGE_RESPONSE_TIME = " << pAggrStat->m_average_response_time << ",";
				ss << "THROUGHPUT_PER_SECOND = " << pAggrStat->m_throughput_per_sec;
				ss << " where SESSION_START_UTC_TS = CONVERTTIMESTAMP(" << pAggrStat->m_session_start_utc_ts << ")";
				ss << " and SESSION_ID = '" << pAggrStat->m_sessionId.c_str() << "'";

                               //go executeDirect
                               execute_flag = REPOS_EXECUTE_UPDATE;
                               execStr = ss.str();
			}
			else
			{
				break;
			}
execute:
            if(REPOS_EXECUTE_UPDATE == execute_flag || REPOS_EXECUTE_CACHELIMIT == execute_flag){
                retcode = pSrvrStmt->ExecDirect(NULL, execStr.c_str(), INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
                if (retcode < 0)
                {
                    errMsg.str("");
                    if(pSrvrStmt->sqlError.errorList._length > 0)
                        p_buffer = pSrvrStmt->sqlError.errorList._buffer;
                    else if(pSrvrStmt->sqlWarning._length > 0)
                        p_buffer = pSrvrStmt->sqlWarning._buffer;
                    if(p_buffer != NULL && p_buffer->errorText)
                        errMsg << "Failed to write statistics: " << execStr.c_str() << "----Error detail - " << p_buffer->errorText;
                    else
                        errMsg << "Failed to write statistics: " << execStr.c_str() << "----Error detail - " << " no additional information";
                    errStr = errMsg.str();
                    SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                            0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                            1, errStr.c_str());
                }
                else {
                    // Update QUERY_TABLE with explain plan if needed
                    if (repos_stats.m_pub_type == PUB_TYPE_STATEMENT_NEW_QUERYEXECUTION && TRUE == srvrGlobal->sqlPlan)
                    {
                        std::tr1::shared_ptr<STATEMENT_QUERYEXECUTION> pQueryAdd = repos_stats.m_pQuery_stats;
                        if(NULL == pQueryAdd)
                        {
                            SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                                    0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                                    1, "Invalid data pointer found in SessionWatchDog(). Cannot write explain plan.");
                            break;
                        }
                        if (pQueryAdd->m_explain_plan && (pQueryAdd->m_explain_plan_len > 0))
                        {
                            retcode = SQL_EXEC_StoreExplainData( &(pQueryAdd->m_exec_start_utc_ts),
                                    (char *)(pQueryAdd->m_query_id.c_str()),
                                    pQueryAdd->m_explain_plan,
                                    pQueryAdd->m_explain_plan_len );

                            if (retcode == -EXE_EXPLAIN_PLAN_TOO_LARGE)
                            {
                                // explain info is too big to be stored in repository.
                                // ignore this error and continue with query execution.
                                retcode = 0;
                            }
                            else if (retcode < 0)
                            {
                                char errStr[256];
                                sprintf( errStr, "Error updating explain data. SQL_EXEC_StoreExplainData() returned: %d", retcode );
                                SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                                        0, ODBCMX_SERVER,
                                        srvrGlobal->srvrObjRef, 1, errStr);
                            }
                            // Clear diagnostics
                            SRVR::WSQL_EXEC_ClearDiagnostics(NULL);
                        }
                    }
                }
                if (pSrvrStmt != NULL){
                    pSrvrStmt->cleanupAll();
                    REALLOCSQLMXHDLS(pSrvrStmt);
                }
            
            }else if (REPOS_EXECUTE_TIMELINE == execute_flag)
            {
                for(int i = 0;i < 3;i++)
                {
                    vector<string> *tmp = &query_list.at(i);

                    if( (*tmp).size() < 2)
                    {
                        continue;
                    }
                    execStr ="";
                    vector<string>::iterator it = (*tmp).begin();
                    execStr += *it + *(it + 1);
                    it += 2;
                    for(; it != (*tmp).end(); it++)
                    {
                        execStr+= "," + *it;
                    }
                    (*tmp).erase((*tmp).begin() + 1, (*tmp).end());

                    retcode = pSrvrStmt->ExecDirect(NULL, execStr.c_str(), INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
                    if (retcode < 0)
                    {
                        errMsg.str("");
                        if(pSrvrStmt->sqlError.errorList._length > 0)
                            p_buffer = pSrvrStmt->sqlError.errorList._buffer;
                        else if(pSrvrStmt->sqlWarning._length > 0)
                            p_buffer = pSrvrStmt->sqlWarning._buffer;
                        if(p_buffer != NULL && p_buffer->errorText)
                            errMsg << "Failed to write statistics: " << execStr.c_str() << "----Error detail - " << p_buffer->errorText;
                        else
                            errMsg << "Failed to write statistics: " << execStr.c_str() << "----Error detail - " << " no additional information";
                        errStr = errMsg.str();
                        SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                                0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                                1, errStr.c_str());
                    }
                    else {
                        // Update QUERY_TABLE with explain plan if needed
                        if (repos_stats.m_pub_type == PUB_TYPE_STATEMENT_NEW_QUERYEXECUTION && TRUE == srvrGlobal->sqlPlan)
                        {
                            std::tr1::shared_ptr<STATEMENT_QUERYEXECUTION> pQueryAdd = repos_stats.m_pQuery_stats;
                            if(NULL == pQueryAdd)
                            {
                                SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                                        0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                                        1, "Invalid data pointer found in SessionWatchDog(). Cannot write explain plan.");
                                break;
                            }
                            if (pQueryAdd->m_explain_plan && (pQueryAdd->m_explain_plan_len > 0))
                            {
                                retcode = SQL_EXEC_StoreExplainData( &(pQueryAdd->m_exec_start_utc_ts),
                                        (char *)(pQueryAdd->m_query_id.c_str()),
                                        pQueryAdd->m_explain_plan,
                                        pQueryAdd->m_explain_plan_len );

                                if (retcode == -EXE_EXPLAIN_PLAN_TOO_LARGE)
                                {
                                    // explain info is too big to be stored in repository.
                                    // ignore this error and continue with query execution.
                                    retcode = 0;
                                }
                                else if (retcode < 0)
                                {
                                    char errStr[256];
                                    sprintf( errStr, "Error updating explain data. SQL_EXEC_StoreExplainData() returned: %d", retcode );
                                    SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                                            0, ODBCMX_SERVER,
                                            srvrGlobal->srvrObjRef, 1, errStr);
                                }
                                // Clear diagnostics
                                SRVR::WSQL_EXEC_ClearDiagnostics(NULL);
                            }
                        }
                    }
	                
                    if (pSrvrStmt != NULL){
			            pSrvrStmt->cleanupAll();
			            REALLOCSQLMXHDLS(pSrvrStmt);
                    }
                }//End for
            }//end else
		}//End while

	}
	catch(...)
	{
		//Write to Log4cxx the error message..
	}

	if (pSrvrStmt != NULL)
		pSrvrStmt->cleanupAll();

	// Statements allocated earlier will get cleaned up
	// during stop processing

	WSQL_EXEC_DeleteContext(thread_context_handle);

	SendEventMsg(MSG_SERVER_TRACE_INFO,
					  EVENTLOG_INFORMATION_TYPE,
					  srvrGlobal->nskASProcessInfo.processId,
					  ODBCMX_SERVICE,
					  srvrGlobal->srvrObjRef,
					  3,
					  srvrGlobal->sessionId,
					  "Deleted new SQL context",
					  "0");

	record_session_done = true;
	rc = pthread_mutex_unlock(&Thread_mutex);
	if (rc != 0)
	{
		sprintf(tmpString, "Failed to release mutex lock for repository session: error code %d", rc);
		SendEventMsg(MSG_ODBC_NSK_ERROR,
		             EVENTLOG_ERROR_TYPE,
		             0,
		             ODBCMX_SERVER,
		             srvrGlobal->srvrObjRef,
		             1,
		             tmpString);
	}
}


#ifdef __TIME_LOGGER
void createTimeLoggerFile()
{
	MS_Mon_Process_Info_Type  proc_info;
	char  myProcname[128];
	short procname_len;
	int error =0;
	char tmpString[1024];

	if ((error = PROCESSHANDLE_DECOMPOSE_ (
                                    TPT_REF(srvrGlobal->nskProcessInfo.pHandle)
                                    ,OMITREF                                //[ short *cpu ]
                                    ,OMITREF                //[ short *pin ]
                                    ,OMITREF                //[ long *nodenumber ]
                                    ,OMITREF                        //[ char *nodename ]
                                    ,OMITSHORT      //[ short maxlen ]
                                    ,OMITREF                //[ short *nodename-length ]
                                    ,myProcname                     //[ char *procname ]
                                    ,sizeof(myProcname)     //[ short maxlen ]
                                    ,&procname_len          //[ short *procname-length ]
                                    ,OMITREF                        //[ long long *sequence-number ]
                                    )) != 0)
	{
   //LCOV_EXCL_START
		    tmpString[0]='\0';
                    sprintf(tmpString, "%d", error);
                    SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                            0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                            1, tmpString);
   //LCOV_EXCL_STOP
	}

            myProcname[procname_len] = 0;

       error = msg_mon_get_process_info_detail(myProcname, &proc_info);
       if (error != XZFIL_ERR_OK )
       {
    //LCOV_EXCL_START
		    tmpString[0]='\0';
                    sprintf(tmpString, "%d", error);
                    SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
                            0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                            1, tmpString);
    //LCOV_EXCL_STOP
       }

            srvrGlobal->process_id = proc_info.pid;
            srvrGlobal->cpu = proc_info.nid;

	srvrGlobal->timeLogger.createLogFile(proc_info.nid, proc_info.pid,myProcname);
}
#endif

// "ImplInit" is the 'object'initialization function for
//  the odbc_SQLSvc object .
//
extern "C" void
ImplInit (
   /* In  */ const CEE_handle_def  *objectHandle,
   /* In  */ const char            *initParam,
   /* In  */ long                  initParamLen,
   /* Out */ CEE_status            *returnSts,
   /* Out */ CEE_tag_def           *objTag,
   /* Out */ CEE_handle_def        *implementationHandle)
{
	CEE_handle_def intf;
	char tmpString[100];

	// Added for exit on SQL un-recoverable errors
	// Initialize sqlErrorExit and errorIndex.
	errorIndex = 0;
	for( int i = 0; i < 8; i++ )
		sqlErrorExit[i] = 0;

	SRVR_INIT_PARAM_Def			*srvrInitParam;

	srvrInitParam = (SRVR_INIT_PARAM_Def *)initParam;

	*returnSts = CEE_SUCCESS;

	if (srvrGlobal == NULL)
	{
		srvrGlobal = new SRVR_GLOBAL_Def;
		if (srvrGlobal == NULL)
		{
			IDL_OBJECT_def objRef;
			memset(&objRef, 0, sizeof(IDL_OBJECT_def));
//LCOV_EXCL_START
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					GetCurrentProcessId(), ODBCMX_SERVER,
					objRef, 1, "srvrGlobal");
			exitServerProcess();
//LCOV_EXCL_STOP
		}
	}

   srvrGlobal->dialogueId = -1;
   srvrGlobal->receiveThrId = getpid();
   timer_register();

   CEE_HANDLE_SET_NIL(&StatisticsTimerHandle);

	srvrGlobal->srvrVersion.componentId = 0; // Unknown
	if (srvrGlobal->srvrVersion.componentId == 0)
	{

		srvrGlobal->srvrVersion.componentId = NSK_ENDIAN + ODBC_SRVR_COMPONENT;
		// done to support backward compatibility
		if (WSQL_EXEC_CLI_VERSION() >= 91) // Roadrunner and above
		{
			srvrGlobal->srvrVersion.majorVersion = MXOSRVR_VERSION_MAJOR;
			srvrGlobal->srvrVersion.minorVersion = MXOSRVR_VERSION_MINOR;
			srvrGlobal->srvrVersion.buildId = MXOSRVR_VERSION_BUILD;
		}
		else
		{
//LCOV_EXCL_START
			srvrGlobal->srvrVersion.majorVersion = NSK_VERSION_MAJOR_1;
			srvrGlobal->srvrVersion.minorVersion = NSK_VERSION_MAJOR_1; // Confusing! don't. Legacy code carried forward ....
			srvrGlobal->srvrVersion.buildId = NSK_BUILD_1;
//LCOV_EXCL_STOP
		}
	}

	short error;
	short errorDetail;

	srvrGlobal->sqlVersion.majorVersion = 0;
	srvrGlobal->sqlVersion.minorVersion = 0;
	srvrGlobal->sqlVersion.buildId = 0;

	srvrGlobal->sqlVersion.majorVersion = VERS_PV_MAJ;
	srvrGlobal->sqlVersion.minorVersion = VERS_PV_MIN;
	srvrGlobal->sqlVersion.buildId = VERS_PV_UPD;

	strcpy(srvrInitParam->asSrvrObjRef, "DUMMY");	// To let it work in RegisterSrvr()

	if (initParam != NULL && initParamLen != 0)
	{
		short error;
		srvrGlobal->debugFlag = srvrInitParam->debugFlag;
		strcpy(srvrGlobal->asSrvrObjRef, srvrInitParam->asSrvrObjRef);
		srvrGlobal->srvrType = srvrInitParam->srvrType;
		srvrGlobal->DSId = srvrInitParam->DSId;
		strcpy(srvrGlobal->DSName, srvrInitParam->DSName);
		srvrGlobal->eventFlag = srvrInitParam->eventFlag;
		srvrGlobal->stopTypeFlag=STOP_UNKNOWN;
		srvrGlobal->timeLoggerFlag=srvrInitParam->timeLogger;

		BUILD_OBJECTREF(srvrInitParam->asSrvrObjRef, srvrGlobal->srvrObjRef, "NonStopODBC", srvrInitParam->portNumber);

		strcpy(srvrGlobal->ASProcessName, srvrInitParam->ASProcessName);

                if ((error = getProcessHandle(srvrGlobal->ASProcessName,
                         TPT_REF(srvrGlobal->nskASProcessInfo.pHandle))) != 0)
		{
//LCOV_EXCL_START
			*returnSts = 9999;
			SET_ERROR((long)0, NSK, UNKNOWN_TRANSPORT, SRVR_API_INIT, E_SERVER, "ImplInit", O_INIT_PROCESS, F_FILENAME_TO_PROCESSHANDLE_, *returnSts, error);
			return;
//LCOV_EXCL_STOP
		}


		MS_Mon_Process_Info_Type  proc_info;

		//Get my node id then get registered processes info from QSSYNC
		//If QSSYNC not found then append my node id to init param -QS name
		//otherwise use QSMGR name for my node id returned from QSSYNC
		msg_mon_get_process_info_detail(NULL, &proc_info);
		int myNid = proc_info.nid;

		strcpy(srvrGlobal->QSProcessName, srvrInitParam->QSProcessName);
/*
		error = msg_mon_get_process_info_detail(srvrInitParam->QSsyncProcessName, &proc_info);
		if (error == XZFIL_ERR_OK )
		{	//QSSYNC process is running
			char segmentArray[256];
			if(true == loadSyncValues(segmentArray,srvrInitParam->QSsyncProcessName))
			{
				if(regProcInfo[myNid].qsname)
					sprintf(srvrGlobal->QSProcessName,"%s",regProcInfo[myNid].qsname);
				else
				{   //QSSYNC returned info but no QSMGR running on my node...use default QSMGR
//LCOV_EXCL_START
					sprintf(tmpString, "WMS manager process not registered on node %d", myNid);
					SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_WARNING_TYPE,
						0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
						1, tmpString);
					sprintf(srvrGlobal->QSProcessName,"%s%02d",srvrInitParam->QSProcessName,myNid);
//LCOV_EXCL_STOP
				}
			}
			else
				//Error getting data from QSSYNC...use default QSMGR
				sprintf(srvrGlobal->QSProcessName,"%s%02d",srvrInitParam->QSProcessName,myNid);
		}
		else
		{	//QSSYNC process not running...use default QSMGR
			sprintf(tmpString, "WMS process %s does not exist", srvrInitParam->QSsyncProcessName);
			SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_WARNING_TYPE,
				0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
				1, tmpString);
			sprintf(srvrGlobal->QSProcessName,"%s%02d",srvrInitParam->QSProcessName,myNid);
		}
*/
		// Added for performance improvement to avoid msg_mon calls in qsmgr. The info collected
		// here will be passed to WMS.
		MS_Mon_Node_Info_Type node_info;
		error = msg_mon_get_node_info_detail(proc_info.nid,&node_info);
		if (error == XZFIL_ERR_OK )
			strcpy(srvrGlobal->segmentname, node_info.node[0].node_name);
//LCOV_EXCL_START
		else
			bzero(srvrGlobal->segmentname, sizeof(srvrGlobal->segmentname));
//LCOV_EXCL_STOP

		srvrGlobal->QSProcessLen = strlen(srvrGlobal->QSProcessName);
		srvrGlobal->mute = srvrInitParam->mute;//Dashboard testing - no 21036 message
		srvrGlobal->ext_21036 = srvrInitParam->ext_21036;//extended 21036 message - for SRPQ

		strcpy(srvrGlobal->TraceCollector, srvrInitParam->TraceCollector);
		if (stricmp(srvrInitParam->RSCollector,srvrInitParam->EmsName)==0)
		{
			strcpy(srvrGlobal->RSCollector, srvrInitParam->RSCollector);
		}
		else
		{
			GetSystemNm(tmpString);
			sprintf(srvrGlobal->RSCollector, "%s.%s", tmpString, srvrInitParam->RSCollector);
		}

		srvrGlobal->portNumber = srvrInitParam->portNumber;
	}

//LCOV_EXCL_START
	if ((*returnSts = InstantiateRGObject()) != ERROR_SUCCESS)
	{
		sprintf(tmpString, "%ld", *returnSts);
		SendEventMsg(MSG_ODBC_NT_ERROR, EVENTLOG_ERROR_TYPE,
			_getpid(), ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			1, tmpString);
		SendEventMsg(MSG_READ_REGISTRY_ERROR, EVENTLOG_ERROR_TYPE,
			_getpid(), ODBCMX_SERVER, srvrGlobal->srvrObjRef, 0);
		SET_ERROR((long)0, NSK, UNKNOWN_TRANSPORT, SRVR_API_INIT, E_SERVER, "ImplInit", O_INIT_PROCESS, F_INSTANTIATE_RG_OBJECT, *returnSts, 0);
		return;
	}
//LCOV_EXCL_STOP

	srvrGlobal->bAutoCommitOn = FALSE;

	srvrGlobal->resGovernOn = FALSE;
	srvrGlobal->envVariableOn = FALSE;
	srvrGlobal->EnvironmentType = MXO_ODBC_35;
	srvrGlobal->clientLCID = LANG_NEUTRAL;
	srvrGlobal->clientErrorLCID = LANG_NEUTRAL;
	srvrGlobal->clientACP = GetACP();
	strcpy(srvrGlobal->userSID, "Invalid User");

	srvrGlobal->WMSSrvrType = WMS_SRVR_SRVR;

	// Added for replacing USER_GETINFO_() with PROCESS_GETINFO().
	srvrGlobal->userID = 0;

	srvrGlobal->resourceStatistics = 0;
	TCPU_DECL(processId);

	int iprocessId;		// for OSS and NT process Ids

	if ((error = PROCESSHANDLE_GETMINE_(TPT_REF(srvrGlobal->nskProcessInfo.pHandle))) != 0)
	{
//LCOV_EXCL_START
		sprintf(tmpString, "%d", error);
		SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			1, tmpString);
		*returnSts = 9998;
		SET_ERROR((long)0, NSK, UNKNOWN_TRANSPORT, SRVR_API_INIT, E_SERVER, "ImplInit", O_INIT_PROCESS, F_PROCESSHANDLE_GETMINE_, *returnSts, error);
		return;
//LCOV_EXCL_STOP
	}
	if ((error = PROCESSHANDLE_DECOMPOSE_(TPT_REF(srvrGlobal->nskProcessInfo.pHandle),
				 &srvrGlobal->nskProcessInfo.nodeId,
			     &processId,
				 OMITREF, OMITREF,
			     OMITSHORT, OMITREF, OMITREF, OMITSHORT,
			     OMITREF, OMITREF)) != 0)
//LCOV_EXCL_START
	{
		sprintf(tmpString, "%d", error);
		SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			1, tmpString);
		*returnSts = 9997;
		SET_ERROR((long)0, NSK, UNKNOWN_TRANSPORT, SRVR_API_INIT, E_SERVER, "ImplInit", O_INIT_PROCESS, F_PROCESSHANDLE_DECOMPOSE_, *returnSts, error);
		return;
	}
//LCOV_EXCL_STOP

	srvrGlobal->nskProcessInfo.processId = processId;

	//  Create Session ID
	char tmpsrvrSessionId[SESSION_ID_LEN];
	getSessionId(tmpsrvrSessionId);
	strcpy(srvrSessionId, tmpsrvrSessionId);
	strcpy(srvrGlobal->sessionId,srvrSessionId);

	srvrGlobal->numConnection = 0;
	srvrGlobal->lastCleanupTime = time(NULL);
	srvrGlobal->cleanupByConnection = 0;
	srvrGlobal->cleanupByTime = 0;
//LCOV_EXCL_START
	//	srvrInitParam->srvrTrace = true;
	//  Server Trace Class initialization
	if (srvrInitParam != NULL && srvrInitParam->srvrTrace == true)
	{
		sprintf(tmpString, "Server Trace Enabled.");
		SendEventMsg(MSG_SERVER_TRACE_INFO,
					      EVENTLOG_INFORMATION_TYPE,
					      srvrGlobal->nskASProcessInfo.processId,
					      ODBCMX_SERVICE,
					      srvrGlobal->srvrObjRef,
					      4,
					      srvrGlobal->sessionId,
					      "EnableServerTrace",
					      "0", tmpString);

		srvrGlobal->traceLogger = new ODBCMXTraceMsg(srvrGlobal->nskProcessInfo.processId, srvrGlobal->srvrObjRef);
		if (srvrGlobal->traceLogger == NULL)
		{
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
					srvrGlobal->srvrObjRef, 1, "srvrGlobal->traceLogger");
			exitServerProcess();
		}
		srvrGlobal->traceLogger->OpenTraceCollector(srvrGlobal->TraceCollector);
	}
	else
//LCOV_EXCL_STOP
		srvrGlobal->traceLogger = NULL;

	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceImplInitEnter(srvrInitParam, initParamLen);
	}

	if (srvrGlobal->asSrvrObjRef[0] == '\0')
	{
//LCOV_EXCL_START
		srvrGlobal->srvrContext.srvrIdleTimeout = INFINITE_SRVR_IDLE_TIMEOUT;
		srvrGlobal->srvrContext.connIdleTimeout = INFINITE_CONN_IDLE_TIMEOUT;
		srvrGlobal->srvrContext.resDescList._buffer = NULL;
		srvrGlobal->srvrContext.resDescList._length = 0;
		srvrGlobal->srvrContext.envDescList._buffer = NULL;
		srvrGlobal->srvrContext.envDescList._length = 0;
//LCOV_EXCL_STOP
	}

	srvrGlobal->tip_gateway = (tip_handle_t)NULL;
	srvrGlobal->pxid_url = (char *)NULL;
	srvrGlobal->local_xid = NULL;
	srvrGlobal->xid_length =  0;
	srvrGlobal->DefaultCatalog[0] = '\0';
	srvrGlobal->DefaultSchema[0] = '\0';
//	srvrGlobal->ext_21036 = false;


	srvrGlobal->CSObject = new CRITICAL_SECTION;

	InitializeCriticalSection(srvrGlobal->CSObject);

	//Call SQLInitialization function here
	initSqlCore();

//	checkIfRowsetSupported();
	srvrGlobal->bRowsetSupported = TRUE;

	srvrGlobal->SystemCatalog[0] = '\0';

	if (envGetMXSystemCatalogName (&srvrGlobal->SystemCatalog[0]) != TRUE)
	{
		*returnSts = 9997;
		SET_ERROR((long)0, NSK, UNKNOWN_TRANSPORT, SRVR_API_INIT, E_SERVER, "ImplInit", O_INIT_PROCESS, F_ENV_GET_MX_SYSTEM_CATALOG_NAME, *returnSts, 0);
		return;
	}
	srvrGlobal->bSkipASTimer = false;

	srvrGlobal->m_NodeId = myNid;
	strncpy(srvrGlobal->m_ProcName, myProcName.c_str(), MS_MON_MAX_PROCESS_NAME);
	srvrGlobal->m_statisticsPubType = statisticsPubType;
	srvrGlobal->m_bStatisticsEnabled = bStatisticsEnabled;
	
	srvrGlobal->m_iAggrInterval = aggrInterval;
	
	interval_max=aggrInterval/MIN_INTERVAL;
	if(aggrInterval%MIN_INTERVAL) 
		interval_max+=1;
	
	srvrGlobal->m_iQueryPubThreshold = queryPubThreshold;

	if(queryPubThreshold>=0) 
	{
		limit_max=queryPubThreshold/MIN_INTERVAL;
		if(queryPubThreshold%MIN_INTERVAL) 
			limit_max+=1;
	}
	
	if (!srvrGlobal->m_bStatisticsEnabled)
		bPlanEnabled = false;
	srvrGlobal->sqlPlan = bPlanEnabled;
		

	CEE_TIMER_CREATE2(DEFAULT_AS_POLLING,0,ASTimerExpired,(CEE_tag_def)NULL, &srvrGlobal->ASTimerHandle,srvrGlobal->receiveThrId);

	resStatSession = NULL;
	resStatStatement = NULL;
	if (srvrGlobal->m_bStatisticsEnabled)
	{
		resStatSession = new ResStatisticsSession();
//LCOV_EXCL_START
		if (resStatSession == NULL)
		{
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
					srvrGlobal->srvrObjRef, 1, "resStatSession");
			exitServerProcess();
		}
//LCOV_EXCL_STOP

		resStatStatement = new ResStatisticsStatement();
		if (resStatStatement == NULL)
		{
//LCOV_EXCL_START
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
					srvrGlobal->srvrObjRef, 1, "resStatStatement");
			exitServerProcess();
//LCOV_EXCL_STOP
		}


	}
	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceImplInitExit(*returnSts);
	}

#ifdef PERF_TEST	// Added for performance testing

	char fName[256], tmpStr[SESSION_ID_LEN+1];
	strcpy( tmpStr, srvrSessionId );
	short slen = strlen(tmpStr);
	for( int i =0; i < slen; i++ )
	{
		if( tmpStr[i] == '$' )
			tmpStr[i] = '_';
		else
		if( tmpStr[i] == ':' )
			tmpStr[i] = '_';
		else
		if( tmpStr[i] == '?' )
			tmpStr[i] = 'X';

	}
	sprintf( fName, "/home/sqperf1/wms_perf/Srvr%s.xml", tmpStr );
	perf = new PerformanceMeasure( fName ); // , pToken );
#endif

#ifdef __TIME_LOGGER
		if(srvrGlobal->timeLoggerFlag)
		{
			createTimeLoggerFile();
		}
#endif
}


extern void SRVR::RegisterSrvr(char* IpAddress, char* HostName)
{
	SRVRTRACE_ENTER(FILE_AME+1);
	CEE_status	retcode = CEE_SUCCESS;
	IDL_OBJECT_def	srvrObjRef;

	if (srvrGlobal->asSrvrObjRef[0] != '\0')
	{
		AS_CALL_CONTEXT* asCallContext;

		MS_Mon_Process_Info_Type process_info;
		msg_mon_get_process_info_detail(NULL,&process_info);
		srvrGlobal->nskProcessInfo.nodeId = process_info.nid;
		srvrGlobal->nskQSProcessInfo.nodeId = process_info.nid;

		asCallContext = new AS_CALL_CONTEXT;
		if (asCallContext == NULL)
		{
//LCOV_EXCL_START
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
					srvrGlobal->srvrObjRef, 1, "asCallContext");
			exitServerProcess();
//LCOV_EXCL_STOP
		}

		strcpy(srvrObjRef, srvrGlobal->srvrObjRef);

		insertIpAddressAndHostNameInObjRef(srvrObjRef, IpAddress, HostName);
		if(retcode = odbcas_ASSvc_RegProcess_pst_(
				&(asCallContext->ASSvc_proxy),
				asCallContext,
				odbcas_ASSvc_RegProcess_ccf_,
				&srvrGlobal->srvrVersion,
				CORE_SRVR,
				srvrObjRef,
				&srvrGlobal->nskProcessInfo) != CEE_SUCCESS)
		{
//LCOV_EXCL_START

			delete asCallContext;

			SendEventMsg(MSG_SRVR_REGISTER_ERROR, EVENTLOG_ERROR_TYPE,
				srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
				0);
			exitServerProcess();
//LCOV_EXCL_STOP
		}
	}
	SRVRTRACE_EXIT(FILE_AME+1);
}

void __cdecl SRVR::ASTimerExpired(CEE_tag_def timer_tag)
{
	SRVRTRACE_ENTER(FILE_AME+2);

    if(srvrGlobal->mutex->locked())
	   return;

    srvrGlobal->mutex->lock();


	if(srvrGlobal->bSkipASTimer == false && (srvrGlobal->srvrState == SRVR_AVAILABLE || srvrGlobal->srvrState == SRVR_UNINITIALIZED))
	{
		// Shutdown if DCS stops.
		// Look for nodes under master and server and determine if DCS has stopped
		int rc = ZOK;
		Stat stat;
		struct String_vector children;
		children.count = 0;
		children.data = NULL;
		stringstream ss;
		string pathStr;

		ss.str("");
		ss << zkRootNode << "/dcs/master";
		pathStr = ss.str();
		rc = zoo_get_children(zh, pathStr.c_str(), 0, &children);
		if( rc != ZOK || !(children.count > 0) )
		{
			free_String_vector(&children);
			ss.str("");
			ss << zkRootNode << "/dcs/servers/running";
			pathStr = ss.str();
			rc = zoo_get_children(zh, pathStr.c_str(), 0, &children);
			if( rc != ZOK || !(children.count > 0) )
				shutdownThisThing = 1;
		}
		free_String_vector(&children);

		if( !shutdownThisThing )
		{
            timeval tv;
            int retcode = gettimeofday(&tv, NULL);
            long long current_time = tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;

			static long prevDialogueId = 0;
			static short clientConnErrorTimeOut = 10;	// secs
			long currDialogueId = 0;
			char zkData[256];
			int zkDataLen = sizeof(zkData);

			rc = zoo_get(zh, dcsRegisteredNode.c_str(), false, zkData, &zkDataLen, &stat);
			if( rc == ZOK )
			{
				// The first token should be state
				char *tkn = NULL;
				char state[32];
				bool zkStatus = true;
				tkn = strtok(zkData, ":");

				if( tkn == NULL )
					goto HandleNoTokens;

                enum {REG_CONNECTING, REG_CONNECT_FAILED, REG_CONNECT_REJECTED, REG_CONNECT_OTHER} reg_connection_state;

                // use numeric to represent the state since the state will used again,
                // and we don't want compare the string twice
                if (stricmp(tkn, "CONNECTING") == 0) {
                    reg_connection_state = REG_CONNECTING;
                }
                else if (stricmp(tkn, "CONNECT_FAILED") == 0) {
                    reg_connection_state = REG_CONNECT_FAILED;
                }
                else if (stricmp(tkn, "CONNECT_REJECTED") == 0) {
                    reg_connection_state = REG_CONNECT_REJECTED;
                }
                else {
                    reg_connection_state = REG_CONNECT_OTHER;
                    prevDialogueId = 0;
                }

                if (reg_connection_state != REG_CONNECT_OTHER) {
                    strcpy(state, tkn);

                    // Skip second token - Timestamp
                    tkn = strtok(NULL, ":");
                    if (tkn == NULL)
                        goto HandleNoTokens;

                    // Third token is dialogue ID
                    tkn = strtok(NULL, ":");
                    if (tkn == NULL)
                        goto HandleNoTokens;

                    currDialogueId = atoi(tkn);
                    prevDialogueId = currDialogueId;

                    if (reg_connection_state == REG_CONNECTING && (current_time - stat.mtime) > (clientConnTimeOut * 1000))
                        zkStatus = updateZKState(CONNECTING, AVAILABLE);
                    else if ((current_time - stat.mtime) > (clientConnErrorTimeOut * 1000)) {
                        if (reg_connection_state == REG_CONNECT_FAILED)
                            zkStatus = updateZKState(CONNECT_FAILED, AVAILABLE);
                        else if (reg_connection_state == REG_CONNECT_REJECTED)
                            zkStatus = updateZKState(CONNECT_REJECTED, AVAILABLE);
                    }

					if( !zkStatus )
					{
						srvrGlobal->mutex->unlock();
						zookeeper_close(zh);
						exitServerProcess();
					}
				}
HandleNoTokens:
				;	// Cannot retrieve tokens from ZK entry at this time.
			}
			else
				shutdownThisThing = 1;
		}

		if( shutdownThisThing )
		{
	        srvrGlobal->mutex->unlock();
	        zookeeper_close(zh);
			exitServerProcess();
		}
	}

    srvrGlobal->mutex->unlock();

	SRVRTRACE_EXIT(FILE_AME+2);
}

BOOL SRVR::checkIfASSvcLives( void )
{
	return processExists(NULL, TPT_REF(srvrGlobal->nskASProcessInfo.pHandle) );
}

/*
 * Call Completion function for
 * operation 'odbcas_ASSvc_RegProcess'
 */
extern "C" void
odbcas_ASSvc_RegProcess_ccf_(
    /* In    */ CEE_tag_def cmptag_
  , /* In    */ const odbcas_ASSvc_RegProcess_exc_ *exception_
  , /* In    */ const SRVR_CONTEXT_def *srvrContext
  )
{
	SRVRTRACE_ENTER(FILE_AME+4);
	int i;
	short wms_nid;

	RES_DESC_def *pResValuesIn;
	RES_DESC_def *pResValues;
	ENV_DESC_def *pEnvValuesIn;
	ENV_DESC_def *pEnvValues;
	char *saveptr;

	CEE_status sts;
	char tmpString[25];

	AS_CALL_CONTEXT	*asCallContext = (AS_CALL_CONTEXT *)cmptag_;

	srvrGlobal->srvrContext.resDescList._buffer = NULL;
	srvrGlobal->srvrContext.resDescList._length = 0;
	srvrGlobal->resGovernOn = FALSE;
	srvrGlobal->srvrContext.envDescList._buffer = NULL;
	srvrGlobal->srvrContext.envDescList._length = 0;
	srvrGlobal->envVariableOn = FALSE;
	srvrGlobal->srvrContext.connIdleTimeout = DEFAULT_CONN_IDLE_TIMEOUT_MINS;
	srvrGlobal->srvrContext.srvrIdleTimeout = INFINITE_SRVR_IDLE_TIMEOUT;
	srvrGlobal->srvrState = SRVR_AVAILABLE;

	delete asCallContext;

	SRVRTRACE_EXIT(FILE_AME+4);
}


long getMemSize(char *sessionPhase)
{
	//int fd = -1;
	char tmpString[128];
	int  local_n;
	char nameBuf[1000];
       	char dataBuf[1000];
	char sessionPhaseStr[40];
	long memSize =0 ;

	memset(sessionPhaseStr,'\0', 40);
	if(!memcmp(sessionPhase,"Initial",7))
		memcpy(sessionPhaseStr,"odbc_SQLSvc_InitializeDialogue_ame_",35);
	else if(!memcmp(sessionPhase,"Terminate",9))
		memcpy(sessionPhaseStr,"odbc_SQLSvc_TerminateDialogue_ame_",34);
	else if(!memcmp(sessionPhase,"Break",5))
		memcpy(sessionPhaseStr,"BreakDialogue",13);

       	memset(nameBuf,'\0',1000);
       	memset(dataBuf,'\0',1000);
       	sprintf(nameBuf, "/proc/%d/statm", srvrGlobal->nskProcessInfo.processId);

	if(fd == -1){
		if ((fd = open(nameBuf, O_RDONLY)) == -1) {
			memset(tmpString,'\0',128);
    			sprintf(tmpString, "open %s error in %s", nameBuf,sessionPhaseStr);
               		SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
                       		srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                     		1, tmpString);
        	} else {
			if ((local_n = read(fd, dataBuf, sizeof dataBuf - 1)) < 0) {
				memset(tmpString,'\0',128);
    				sprintf(tmpString, "read %s error in %s", nameBuf,sessionPhaseStr);
               			SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
                       			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                      			1, tmpString);
			} else
				sscanf(dataBuf, "%*ld %ld ", &memSize);
			}
		} else {
			lseek(fd, 0L, SEEK_SET);
			if ((local_n = read(fd, dataBuf, sizeof dataBuf - 1)) < 0) {
				memset(tmpString,'\0',128);
    				sprintf(tmpString, "read %s error %s", nameBuf,sessionPhaseStr);
                		SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
                       			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
                       			1, tmpString);
			} else
				sscanf(dataBuf, "%*ld %ld ", &memSize);


	}

	return memSize;

}
/*
 * Asynchronous method function prototype for
 * operation 'odbc_SQLSvc_InitializeDialogue'
 */
extern "C" void
odbc_SQLSvc_InitializeDialogue_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const USER_DESC_def *userDesc
  , /* In    */ const CONNECTION_CONTEXT_def *inContext
  , /* In    */ DIALOGUE_ID_def dialogueId
  )
{

	SRVRTRACE_ENTER(FILE_AME+5);
	OUT_CONNECTION_CONTEXT_def outContext;
	odbc_SQLSvc_MonitorCall_exc_ monitorException_={0,0};

	odbc_SQLSvc_InitializeDialogue_exc_ exception_={0,0,0};
	odbc_SQLSvc_SetConnectionOption_exc_ setConnectException={0,0,0};
	char tmpString[100];
	ERROR_DESC_LIST_def sqlWarning = {0,0};

	unsigned long	curRowNo;
	ENV_DESC_def	*pEnvDesc;
	char			VariableValue[3900];
	char			seps[]   = " \t\n.";
	char			*EnvTypes[] = {"SET"};
	char			*SetTypes[] = {"CATALOG", "SCHEMA"};
	char			*token;
	char			*saveptr;

	unsigned long	computerNameLen = MAX_COMPUTERNAME_LENGTH*4 + 1;
	VERSION_def		*versionPtr;
	VERSION_def		*clientVersionPtr;
	CEE_status		sts;
	short			retCode = 0;
	char			TmpstrRole[MAX_ROLENAME_LEN + 1];
	__int64			prevRedefTime = 0;

	//Initialize the isShapeLoaded
        srvrGlobal->isShapeLoaded = false;

//    cleanupJniDLL();

	strcpy(TmpstrRole, srvrGlobal->QSRoleName);
	prevRedefTime = srvrGlobal->redefTime;

	srvrGlobal->QSServiceId = 0;
	bzero(srvrGlobal->QSServiceName, sizeof(srvrGlobal->QSServiceName));
	bzero(srvrGlobal->QSRoleName, sizeof(srvrGlobal->QSRoleName));
	bzero(srvrGlobal->QSRuleName, sizeof(srvrGlobal->QSRuleName));
	bzero(srvrGlobal->QSUserName, sizeof(srvrGlobal->QSUserName));
	bzero(srvrGlobal->QSDBUserName, sizeof(srvrGlobal->QSDBUserName));
	bzero(srvrGlobal->ApplicationName, sizeof(srvrGlobal->ApplicationName));
        bzero(srvrGlobal->mappedProfileName, sizeof(srvrGlobal->mappedProfileName));
        bzero(srvrGlobal->mappedSLAName, sizeof(srvrGlobal->mappedSLAName));
	bzero(&outContext, sizeof(outContext));
	srvrGlobal->bSpjEnableProxy = FALSE;
	srvrGlobal->bspjTxnJoined = FALSE;
	srvrGlobal->spjTxnId = 0;
	srvrGlobal->enableLongVarchar = false;
	if (inContext->inContextOptions1 & INCONTEXT_OPT1_FETCHAHEAD)
		srvrGlobal->fetchAhead = TRUE;
	else
		srvrGlobal->fetchAhead = FALSE;
	srvrGlobal->defaultSchemaAccessOnly = false;

	// Added for 64bit work
	srvrGlobal->stmtHandleMap.clear();
	SRVR_STMT_HDL::globalKey = 0;

	int		   catLen;
	char	   TmpstrCat[MAX_SQL_IDENTIFIER_LEN+3];
	char	   TmpstrSch[MAX_SQL_IDENTIFIER_LEN+3];
	char	   *tmpPtr, *tmpPtr2;

	bool delimit = true;
	char *UTF8ErrorText;
	long UTF8ErrorTextLen;

	DBUserAuth *userSession = DBUserAuth::GetInstance();

	TmpstrCat[0]='\0';
	TmpstrSch[0]='\0';

	diagnostic_flags=inContext->diagnosticFlag;

	strncpy(srvrGlobal->ClientComputerName, inContext->computerName, sizeof(srvrGlobal->ClientComputerName) - 1);
	srvrGlobal->ClientComputerName[sizeof(srvrGlobal->ClientComputerName) -1] = '\0';

	strncpy(srvrGlobal->ApplicationName, inContext->windowText,sizeof(srvrGlobal->ApplicationName) - 1);
	srvrGlobal->ApplicationName[sizeof(srvrGlobal->ApplicationName) -1] = '\0';
	strcpy(srvrGlobal->QSRoleName, inContext->userRole);
	strcpy(srvrGlobal->QSUserName, userDesc->userName != NULL ? userDesc->userName: "");

	char tmpsrvrSessionId[SESSION_ID_LEN];
	getSessionId(tmpsrvrSessionId);
	strcpy(srvrGlobal->sessionId,tmpsrvrSessionId);
	srvrGlobal->m_bNewConnection = true;

	char schemaValueStr[MAX_SQL_IDENTIFIER_LEN+MAX_SQL_IDENTIFIER_LEN+5+1];	// 5 for quotes + dot
	char catTempStr[MAX_SQL_IDENTIFIER_LEN+1];
	char schTempStr[MAX_SQL_IDENTIFIER_LEN+1];
	catTempStr[0] = '\0';
	schTempStr[0] = '\0';

	BOOL InternalUse = TRUE;

	srvrGlobal->bWMS_AdaptiveSegment = false;

	srvrGlobal->EnvironmentType = MXO_ODBC_35;

	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceConnectEnter(userDesc, inContext, dialogueId);
	}

	// This check is not possible, since we can update the srvr state from client also
	// Client updates the state as SRVR_CLIENT_DISAPPEARED in case of login timeout and
	// the srvr state may not be in sync with the corresponding srvr state in AS
	// Hence commenting
	// ---- we removed update the state from the client

//	if (srvrGlobal->srvrState != SRVR_AVAILABLE)
//		exception_.exception_nr = odbc_SQLSvc_InitializeDialogue_InvalidConnection_exn_;
//	else

	sts = CEE_TMP_ALLOCATE(call_id_, sizeof(VERSION_def)*2, (void **)&versionPtr);
	if( sts != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "SrvrConnect.cpp");
		strcpy( errStrBuf3, "odbc_SQLSvc_InitializeDialogue_sme_");
		strcpy( errStrBuf4, "CEE_TMP_ALLOCATE");
		sprintf( errStrBuf5, "Failed to get <%d> bytes", sizeof(VERSION_def)*2);
		logError( NO_MEMORY, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
//LCOV_EXCL_STOP
	}

	int len_length = inContext->clientVersionList._length;
	VERSION_def *p_buffer = inContext->clientVersionList._buffer;

	for(int i=0; i < len_length; i++)
	{
		clientVersionPtr = p_buffer + i;
		switch( clientVersionPtr->componentId )
		{
		case DRVR_COMPONENT:
		case WIN_UNICODE_DRVR_COMPONENT:
		case LINUX_UNICODE_DRVR_COMPONENT:
		case HPUX_UNICODE_DRVR_COMPONENT:
		case OLEDB_DRVR_COMPONENT:
		case DOT_NET_DRVR_COMPONENT:
		case JDBC_DRVR_COMPONENT:
		case LINUX_DRVR_COMPONENT:
		case HPUX_DRVR_COMPONENT:
		case AIX_DRVR_COMPONENT:
		case SUNSPARC32_DRVR_COMPONENT:
		case SUNSPARC64_DRVR_COMPONENT:

			srvrGlobal->drvrVersion.componentId = clientVersionPtr->componentId;
			srvrGlobal->drvrVersion.majorVersion = clientVersionPtr->majorVersion;
			srvrGlobal->drvrVersion.minorVersion = clientVersionPtr->minorVersion;
			srvrGlobal->drvrVersion.buildId = clientVersionPtr->buildId;
			break;
		case APP_COMPONENT:
			srvrGlobal->appVersion.componentId = clientVersionPtr->componentId;
			srvrGlobal->appVersion.majorVersion = clientVersionPtr->majorVersion;
			srvrGlobal->appVersion.minorVersion = clientVersionPtr->minorVersion;
			srvrGlobal->appVersion.buildId = clientVersionPtr->buildId;
			break;
		}
	}

	outContext.versionList._length = 2;
	outContext.versionList._buffer = versionPtr;
	versionPtr = outContext.versionList._buffer + 0; // First element
	versionPtr->componentId = srvrGlobal->srvrVersion.componentId;
	versionPtr->majorVersion = srvrGlobal->srvrVersion.majorVersion;
	versionPtr->minorVersion = srvrGlobal->srvrVersion.minorVersion;
	versionPtr->buildId	= srvrGlobal->srvrVersion.buildId;
	versionPtr = outContext.versionList._buffer + 1; // Second element
	versionPtr->componentId = SQL_COMPONENT;
	versionPtr->majorVersion = srvrGlobal->sqlVersion.majorVersion;
	versionPtr->minorVersion = srvrGlobal->sqlVersion.minorVersion;
	versionPtr->buildId	= srvrGlobal->sqlVersion.buildId;

	outContext.nodeId = srvrGlobal->nskProcessInfo.nodeId;
	outContext.processId = srvrGlobal->nskProcessInfo.processId;
	if (!GetComputerName(outContext.computerName, &computerNameLen))
		outContext.computerName[0] = '\0';
	outContext.outContextOptions1 = 0;
	outContext.outContextOptions2 = 0;

	char zkData[256];
	char state[32];
	int zkDataLen = sizeof(zkData);
	Stat stat;
	bool zk_error = false;
	char zkErrStr[128];
	char *data = NULL;

	int rc = zoo_exists(zh, dcsRegisteredNode.c_str(), 0, &stat);
	if( rc == ZOK )
	{
		bool stateOk = false;
		short cnt = 6;
		char *tkn = NULL;
		while(!stateOk && cnt)
		{
			// call sync to get up to date data
			data = strdup(dcsRegisteredNode.c_str());
			rc = zoo_async(zh, dcsRegisteredNode.c_str(), sync_string_completion, data);
			if ( data != NULL )
				free(data);
			if( rc != ZOK )
			{
				sprintf(tmpString, "odbc_SQLSvc_InitializeDialogue_ame_...Error %d calling zoo_async() for %s. Server exiting.", rc, dcsRegisteredNode.c_str());
				SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
					1, tmpString);
				exitServerProcess();
			}

			cnt--;
			// Get the dialogue ID from the data part of connecting znode
			rc = zoo_get(zh, dcsRegisteredNode.c_str(), false, zkData, &zkDataLen, &stat);
			if( rc == ZOK )
			{
				// The first token should be CONNECTING state
				tkn = strtok(zkData, ":");
				if( tkn == NULL || stricmp(tkn, "CONNECTING") )
				{
					// If last try then return error
					if(!cnt)
						break;
					else
					{
						// Wait a short while for the state update
						sleep(10);
						continue;
					}
				}
				else
					stateOk = true;
			}
			else
				break;
		}

		if( rc == ZOK )
		{
			if( tkn == NULL || stricmp(tkn, "CONNECTING") )
			{
				char errMsg[512];
				if( tkn == NULL )
					sprintf( errMsg, "Trafodion Internal error: Zookeeper entry not in connecting state for %ld. Current state is NULL", srvrGlobal->portNumber);
				else
					sprintf( errMsg, "Trafodion Internal error: Zookeeper entry not in connecting state for %ld. Current state is %s", srvrGlobal->portNumber, tkn );

				exception_.exception_detail = -1;
				//exception_.exception_nr = odbc_SQLSvc_InitializeDialogue_InvalidConnection_exn_;
				exception_.exception_nr = odbc_SQLSvc_InitializeDialogue_SQLError_exn_;
				SETSRVRERROR(SQLERRWARN, -1, "HY000", errMsg, &exception_.u.SQLError.errorList);
				//SETSRVRERROR(SQLERRWARN, -1, "HY000", "Trafodion Internal error: Zookeeper entry not in connecting state", &exception_.u.SQLError.errorList);
				odbc_SQLSvc_InitializeDialogue_ts_res_(objtag_, call_id_, &exception_, &outContext);
				return;
			}

			// Skip second token - Timestamp
			tkn = strtok(NULL, ":");

			// Third token in data is dialogue ID
			srvrGlobal->dialogueId = -1;
			tkn = strtok(NULL, ":");
			if( tkn != NULL )
				srvrGlobal->dialogueId = atoi(tkn);

			if( tkn == NULL || srvrGlobal->dialogueId == -1 )
			{
				SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
					1, "No dialogue ID in registered node. Server exiting.");
				exitServerProcess();
			}

			// Return error if dialogue ID does not match.
			if (srvrGlobal->dialogueId != dialogueId)
			{
				exception_.exception_detail = -1;
				//exception_.exception_nr = odbc_SQLSvc_InitializeDialogue_InvalidConnection_exn_;
				//SETSRVRERROR(SECURITYERR, -1, "HY000", "Dialogue ID does not match", &exception_.u.SQLError.errorList);
				exception_.exception_nr = odbc_SQLSvc_InitializeDialogue_SQLError_exn_;
				SETSRVRERROR(SQLERRWARN, -1, "HY000", "Trafodion: Dialogue ID does not match", &exception_.u.SQLError.errorList);
				odbc_SQLSvc_InitializeDialogue_ts_res_(objtag_, call_id_, &exception_, &outContext);
				return;
			}
		}
	}

	if( rc != ZOK )
	{
		sprintf(tmpString, "Error %d getting registered node data from Zookeeper. Server exiting.", rc);
		SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			1, tmpString);
		exitServerProcess();
	}

	sdconn = ((CTCPIPSystemSrvr* )objtag_)->m_nSocketFnum;
	// If the server state is connecting then Initialize_Dialogue
	// is called second time with or without changing the password
	// Update the SrvrState as Connecting

	if (srvrGlobal->srvrState == SRVR_AVAILABLE )
		srvrGlobal->srvrState = SRVR_CONNECTING;

	if( TestPointArray != NULL )
	{
//LCOV_EXCL_START
		delete[] TestPointArray;
		TestPointArray = NULL;
//LCOV_EXCL_STOP
	}

	setConnectException.exception_nr = 0;
	WSQL_EXEC_ClearDiagnostics(NULL);

//	volatile int done = 0;
//	while (!done) {
//	  sleep(10);
//	}

	// Security Initialization
	if (!securitySetup)
	{
		retCode = SECURITY_SETUP_();
		if (retCode == SECMXO_NO_ERROR)
			securitySetup = true;
		else
		{
//LCOV_EXCL_START
			exception_.exception_detail = retCode;
			exception_.exception_nr = odbc_SQLSvc_InitializeDialogue_SQLError_exn_;
			SETSECURITYERROR(retCode, &exception_.u.SQLError.errorList);
			odbc_SQLSvc_InitializeDialogue_ts_res_(objtag_, call_id_, &exception_, &outContext);
			updateSrvrState(SRVR_CONNECT_REJECTED);
			if (retCode == SECMXO_INTERNAL_ERROR_FATAL)
			{
				SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
					1, "Security layer returned fatal error. Server exiting.");
				exitServerProcess();
			}
			return;
//LCOV_EXCL_STOP
		}
	}

//	R2.93 - Check if password security is required and reject old driver
//	R2.5  - Allow old driver connection without password encryption if security policy allows it
	if ( !(srvrGlobal->drvrVersion.buildId & PASSWORD_SECURITY))
	{
//LCOV_EXCL_START
		exception_.exception_nr = odbc_SQLSvc_InitializeDialogue_SQLError_exn_;
		SETSRVRERROR(SECURITYERR, -8837, "HY000", SQLSVC_EXCEPTION_PASSWORD_ENCRYPTION_REQUIRED, &exception_.u.SQLError.errorList);
		odbc_SQLSvc_InitializeDialogue_ts_res_(objtag_, call_id_, &exception_, &outContext);
		updateSrvrState(SRVR_CONNECT_REJECTED);
		return;
//LCOV_EXCL_STOP
	}

	if (srvrGlobal->drvrVersion.buildId & PASSWORD_SECURITY)
	{
		if (inContext->inContextOptions1 & INCONTEXT_OPT1_CERTIFICATE_TIMESTAMP)
		{
			short certificateLen = 0;
			char* certificatePtr = NULL;
			char* certificateTS = inContext->connectOptions;
			if (userDesc->password._buffer == NULL || IS_CERTIFICATE_NEEDED_((char *)(userDesc->password._buffer)))
				retCode = VALIDATE_CERTIFICATE_TS(certificateTS);
			switch(retCode)
			{
			case SECMXO_CERTIFICATE_UPDATED:
				// certificates don't match, but policy permits downloading
				// start an autodownload of the certificate
				retCode = GET_CERTIFICATE(NULL, &certificateLen);
				if (retCode == SECMXO_NO_ERROR)
				{
//LCOV_EXCL_START
					sts = CEE_TMP_ALLOCATE(call_id_, certificateLen+1, (void **)&certificatePtr);
					if( sts != CEE_SUCCESS)
					{
						strcpy( errStrBuf2, "SrvrConnect.cpp");
						strcpy( errStrBuf3, "odbc_SQLSvc_InitializeDialogue_ame_");
						strcpy( errStrBuf4, "CEE_TMP_ALLOCATE");
						sprintf( errStrBuf5, "Failed to get <%d> bytes", certificateLen);
						logError( NO_MEMORY, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
//LCOV_EXCL_STOP
					}
					retCode = GET_CERTIFICATE(certificatePtr, &certificateLen);
				}
				if (retCode == SECMXO_NO_ERROR)
				{
					outContext.outContextOptions1 = outContext.outContextOptions1 | OUTCONTEXT_OPT1_DOWNLOAD_CERTIFICATE;
					outContext.outContextOptionString = certificatePtr;
					outContext.outContextOptionStringLen = certificateLen;
					exception_.exception_nr = odbc_SQLSvc_InitializeDialogue_InvalidUser_exn_;
					odbc_SQLSvc_InitializeDialogue_ts_res_(objtag_, call_id_, &exception_, &outContext);
					return;
				}
				else
				{
//LCOV_EXCL_START
					exception_.exception_detail = retCode;
					exception_.exception_nr = odbc_SQLSvc_InitializeDialogue_SQLError_exn_;
					SETSECURITYERROR(retCode, &exception_.u.SQLError.errorList);
					odbc_SQLSvc_InitializeDialogue_ts_res_(objtag_, call_id_, &exception_, &outContext);
					updateSrvrState(SRVR_CONNECT_REJECTED);
                    // reset the srvrState
                    srvrGlobal->srvrState = SRVR_AVAILABLE;
					if (retCode == SECMXO_INTERNAL_ERROR_FATAL)
					{
						SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
							srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
							1, "Security layer returned fatal error. Server exiting.");
						exitServerProcess();
					}
					return;
//LCOV_EXCL_STOP
				}
				break;
			case SECMXO_NO_CERTIFICATE:
				// certificates don't match, and policy prohibits downloading
				// report error to user, no autodownload
				exception_.exception_detail = retCode;
				exception_.exception_nr = odbc_SQLSvc_InitializeDialogue_SQLError_exn_;
				SETSECURITYERROR(retCode, &exception_.u.SQLError.errorList);
				odbc_SQLSvc_InitializeDialogue_ts_res_(objtag_, call_id_, &exception_, &outContext);
				return;
				break;
//LCOV_EXCL_START
			case SECMXO_CERTIFICATE_EXPIRED:
				// certificates match, but they are expired, and policy enforces certificate expiration
				// report error to user, no autodownload
			case SECMXO_INTERNAL_ERROR:
			case SECMXO_INTERNAL_ERROR_FATAL:
				// unexpected error, an EMS log entry was made
				// report error to user
				exception_.exception_detail = retCode;
				exception_.exception_nr = odbc_SQLSvc_InitializeDialogue_SQLError_exn_;
				SETSECURITYERROR(retCode, &exception_.u.SQLError.errorList);
				odbc_SQLSvc_InitializeDialogue_ts_res_(objtag_, call_id_, &exception_, &outContext);
				updateSrvrState(SRVR_CONNECT_REJECTED);
				if (retCode == SECMXO_INTERNAL_ERROR_FATAL)
				{
					SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
						1, "Security layer returned fatal error. Server exiting.");
					exitServerProcess();
//LCOV_EXCL_STOP
				}
				return;
				break;
			case SECMXO_NO_ERROR:
				// certificates match
				// continue, certificate is good
			default:
				break;
			}
		}
	}
	if (userDesc->userName == NULL || userDesc->password._buffer == NULL)
	{
//LCOV_EXCL_START
		exception_.exception_nr = odbc_SQLSvc_InitializeDialogue_InvalidUser_exn_;
		SETSRVRERROR(SQLERRWARN, -8837, "28000", "Invalid authorization specification", &exception_.u.SQLError.errorList);
		odbc_SQLSvc_InitializeDialogue_ts_res_(objtag_, call_id_, &exception_, &outContext);
		updateSrvrState(SRVR_CONNECT_REJECTED);
		return;
//LCOV_EXCL_STOP
	}

	if (strlen(inContext->catalog) > MAX_SQL_IDENTIFIER_LEN || strlen(inContext->schema) > MAX_SQL_IDENTIFIER_LEN)
	{
		exception_.exception_nr = odbc_SQLSvc_InitializeDialogue_ParamError_exn_;
		exception_.u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_OPTION_VALUE_STR;
		updateSrvrState(SRVR_CONNECT_REJECTED);
	}

	/*
	 * Read the set values as it has SQL_ATTR_WARNING has to be
	 * set before the connection is established
	 * SET SQL_ATTR_WARNING 0 - Supress the connection time warnings
	 * SET SQL_ATTR_WARNING 1 - Not to Supress the connection time warnings
	 */
	if (srvrGlobal->envVariableOn)
	{
		int len_length = srvrGlobal->srvrContext.envDescList._length;
		ENV_DESC_def *p_buffer = (ENV_DESC_def *)srvrGlobal->srvrContext.envDescList._buffer;
		char *saveptr;

		for (curRowNo = 0; curRowNo < len_length; curRowNo ++)
		{
			pEnvDesc = p_buffer + curRowNo;
			if ( pEnvDesc->VarType == ENV_SET )
			{
				strcpy(VariableValue, pEnvDesc->VarVal);
				token = strtok_r(VariableValue, seps, &saveptr);
				if (_stricmp(token, EnvTypes[0]) == 0)
				{
					token = strtok_r(NULL, seps, &saveptr );
					if (_stricmp(token,ATTR_TYPE7) == 0)
					{
						token = strtok_r(NULL,seps, &saveptr);
						if(_stricmp(token,ATTR_TYPE7_VALUE2) == 0) // Not to Supress the connection time warnings
							srvrGlobal->EnvironmentType = srvrGlobal->EnvironmentType | MXO_PASSWORD_EXPIRY;
					}
					else if(_stricmp(token,ATTR_TYPE15) == 0) // "SQL_ATTR_IGNORE_CANCEL"
					{
						token = strtok_r(NULL,seps, &saveptr);
						if(_stricmp(token,ATTR_TYPE15_VALUE2) == 0) // Force SQLCancel to be ignored
							outContext.outContextOptions1 = outContext.outContextOptions1 | OUTCONTEXT_OPT1_IGNORE_SQLCANCEL;
					}
				}
			}
		}
	}

	/*
	 We should get rid of this, but we cant right now
	 - if we remove it, because of a bug in SQL that does not reset transactions, an MXOSRVR could get into an unusable state
	 for ex: an application sets txn isolation level to read uncommited. All subseq connection will also get this.
	 - ideally we need to have a reset txn cqd (which will only be available in 2.5)

	- now setting this isolation to read committed is also a problem. If the system defaults has read uncommitted, then any
	scenario which will cause a new compiler to be created (ex: a diff userid logging on) will cause it to get a read committed isolation level
	so potential error 73s can happen. The workaround for this ofcourse is to set it at the datasource level
	*/

	odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
										call_id_,
										&setConnectException,
										dialogueId,
										SQL_TXN_ISOLATION,
										SQL_TXN_READ_COMMITTED,
										NULL
										, &sqlWarning
										);

	if (setConnectException.exception_nr != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		sprintf(tmpString, "%ld", inContext->txnIsolationLevel);
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "SQL_TXN_ISOLATION", tmpString);
		goto MapException;
//LCOV_EXCL_STOP
	}

        	odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
										call_id_,
										&setConnectException,
										dialogueId,
										RESET_DEFAULTS,
										0,
										NULL,
										&sqlWarning
										);
	if (setConnectException.exception_nr != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "RESET_DEFAULTS", "");
		goto MapException;
//LCOV_EXCL_STOP
	}

	odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
										call_id_,
										&setConnectException,
										dialogueId,
										CUT_CONTROLQUERYSHAPE,
										0,
										NULL,
										&sqlWarning
										);
	if (setConnectException.exception_nr != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "CUT_CONTROLQUERYSHAPE", "");
		goto MapException;
//LCOV_EXCL_STOP
	}

	// collect information for auditing and repository

	memset(setinit.clientId,'\0',MAX_COMPUTERNAME_LENGTH*4 + 1);
	memset(setinit.applicationId,'\0',APPLICATIONID_LENGTH*4 + 1);
	memset(setinit.clientUserName,'\0',MAX_SQL_IDENTIFIER_LEN + 1);

	strcpyUTF8(setinit.clientId,inContext->computerName, sizeof(setinit.clientId));
	strcpyUTF8(setinit.applicationId,inContext->windowText,sizeof(setinit.applicationId));
	if (inContext->clientUserName != NULL)
		strcpyUTF8(setinit.clientUserName,inContext->clientUserName, sizeof(setinit.clientUserName));
	else
		strcpy(setinit.clientUserName, "<N/A>");

	odbc_SQLSvc_InitializeDialogue_sme_(objtag_, call_id_, &exception_, userDesc, inContext,
						dialogueId, &outContext);

	// If there is an exception, do not proceed to set the server initial context
	if (exception_.exception_nr != 0)
	{
		odbc_SQLSvc_InitializeDialogue_ts_res_(objtag_, call_id_, &exception_, &outContext);
		updateSrvrState(SRVR_CONNECT_REJECTED);
		if (outContext.outContextOptionStringLen > 0)
			delete [] outContext.outContextOptionString;

			if (srvrGlobal->traceLogger != NULL)
			{
				srvrGlobal->traceLogger->TraceConnectExit(exception_, outContext);
			}

		return;
	}
	else
	{

// Get Default Catalog Schema
		getCurrentCatalogSchema();
		if ( srvrGlobal->DefaultSchema[0] == '\0' || srvrGlobal->DefaultCatalog[0] == '\0' )
		{
			strcpy(srvrGlobal->DefaultCatalog, ODBCMX_DEFAULT_CATALOG);
			strcpy(srvrGlobal->DefaultSchema, ODBCMX_DEFAULT_SCHEMA);
		}


		if (inContext->catalog[0] != NULL)
		{
// Temporary - till drivers get fixed
//
                if (stricmp(inContext->catalog, ODBCMX_PREV_DEFAULT_CATALOG) != 0)
                {
			strcpy(srvrGlobal->DefaultCatalog, """");
			strcat(srvrGlobal->DefaultCatalog, inContext->catalog);
			strcat(srvrGlobal->DefaultCatalog, """");
                }
                else
// Convert the default catalog set by old drivers to the current
			strcpy(srvrGlobal->DefaultCatalog, ODBCMX_DEFAULT_CATALOG);
		} // inContext->catalog[0] != NULL


		static bool defaultSchemaSaved = false;
		if (stricmp(TmpstrRole, srvrGlobal->QSRoleName) != 0) // it means user role has been updated - default schema also needs to be updated
			defaultSchemaSaved = false;
		if (!defaultSchemaSaved)
		{
			if(!getSQLInfo( SCHEMA_DEFAULT )) // populate savedDefaultSchema
			{
				//this should not happen - but let's put defensive code to set it to "USR"
				strcpy(savedDefaultSchema,ODBCMX_DEFAULT_SCHEMA);
			}
			defaultSchemaSaved = true;
		}

		if (inContext->schema[0] == NULL)
		{
			strcpy(srvrGlobal->DefaultSchema, savedDefaultSchema);
			strcpy(schemaValueStr, """");
			strcat(schemaValueStr, srvrGlobal->DefaultCatalog);
			strcat(schemaValueStr, """");
			strcat(schemaValueStr, ".");
			strcat(schemaValueStr, savedDefaultSchema);

			odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
												call_id_,
												&setConnectException,
												dialogueId,
												SET_SCHEMA,
												0,
												(IDL_string)schemaValueStr
												,&sqlWarning
												);
			if (setConnectException.exception_nr != CEE_SUCCESS)
			{
//LCOV_EXCL_START
				SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
					2, "RESET_SCHEMA", schemaValueStr);
				goto MapException;
//LCOV_EXCL_STOP
			}
		}
		if (inContext->schema[0] != NULL)
		{
			// Fix: If the catalog or schema name itself has a dot within it like in below
			// "a&new*.cat"."a&new*.sch" then we have to handle that.
			tmpPtr = tmpPtr2 = NULL;
			if(   tmpPtr = (char *) strrchr(inContext->schema,'.')   )
			{
				// Search backwards for a double quotes if it exists then check if there is
				// any dot before that and pick that position.
				char	   TmpstrCatSch[257];
				strcpy( TmpstrCatSch, inContext->schema );
				TmpstrCatSch[tmpPtr - inContext->schema] = '\x0';

				tmpPtr2 = strrchr(TmpstrCatSch,'"');
				if( tmpPtr2 != NULL )
				{
					TmpstrCatSch[tmpPtr2 - TmpstrCatSch] = '\x0';
					if( tmpPtr2 = strrchr(TmpstrCatSch,'.') )
						tmpPtr = (char *)(inContext->schema + (tmpPtr2 - TmpstrCatSch));
					else
						tmpPtr = NULL;
				}
			}

			if( tmpPtr != NULL )
			{
				catLen = strlen(inContext->schema) - strlen(tmpPtr);
				//copying the Catalog
				strncpy(TmpstrCat,inContext->schema,catLen);
				TmpstrCat[catLen] = '\0';
				*tmpPtr++;
				//copying the Schema
				strcpy(TmpstrSch, tmpPtr);

				strcpy(srvrGlobal->DefaultCatalog, """");
				strcat(srvrGlobal->DefaultCatalog, TmpstrCat);
				strcat(srvrGlobal->DefaultCatalog, """");

				strcpy(srvrGlobal->DefaultSchema, """");
				strcat(srvrGlobal->DefaultSchema, TmpstrSch);
				strcat(srvrGlobal->DefaultSchema, """");

					if ( srvrGlobal->DefaultSchema[0] == '\0' || srvrGlobal->DefaultCatalog[0] == '\0' )
					{
						exception_.exception_nr = odbc_SQLSvc_InitializeDialogue_ParamError_exn_;
						exception_.u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_SCHEMA_CATALOG_OPTION;
						updateSrvrState(SRVR_CONNECT_REJECTED);
					}

				}

			else
			{
				strcpy(srvrGlobal->DefaultSchema, """");
				strcat(srvrGlobal->DefaultSchema, inContext->schema);
				strcat(srvrGlobal->DefaultSchema, """");
			}

		}

		strcpy(outContext.catalog, srvrGlobal->DefaultCatalog);
		// The size of srvrGlobal->DefaultSchema is increased to 131
		// to allow double-quotes around the schema name
		// we need to be careful not to overrun outContext.schema
		strncpy(outContext.schema, srvrGlobal->DefaultSchema, sizeof(outContext.schema));
		outContext.schema[sizeof(outContext.schema)-1] = '\0';
	}


	// Added to detect MODE_SPECIAL_1 CQD
	static bool firstTime = true;
	if ( firstTime )
	{
		srvrGlobal->modeSpecial_1 = false;
		if( getSQLInfo( MODE_SPECIAL_1 ))
			srvrGlobal->modeSpecial_1 = true;

		firstTime = false;
	}
	if( srvrGlobal->modeSpecial_1 )
		outContext.versionList._buffer->buildId = outContext.versionList._buffer->buildId | MXO_SPECIAL_1_MODE;

	// assign client locale information to srvrGlobal
	srvrGlobal->clientLCID = inContext->ctxDataLang;
	srvrGlobal->clientErrorLCID = inContext->ctxErrorLang;
	srvrGlobal->clientACP = inContext->ctxACP;
	srvrGlobal->useCtrlInferNCHAR = inContext->ctxCtrlInferNCHAR;

	if (srvrGlobal->tip_gateway != NULL)
	{
		#ifdef TIP_DEFINED
			tip_close(srvrGlobal->tip_gateway);
		#endif
		srvrGlobal->tip_gateway = NULL;
	}


	//getSQLInfo(USER_ROLE); // srvrGlobal->RoleName and srvrGlobal->QSRoleName is set here

	if (srvrGlobal->QSRoleName[0] != '\0')
	{
		outContext.outContextOptions1 = outContext.outContextOptions1 | OUTCONTEXT_OPT1_ROLENAME;
		outContext.outContextOptionStringLen = strlen(srvrGlobal->QSRoleName)+5;
		outContext.outContextOptionString = new char[outContext.outContextOptionStringLen];
		sprintf(outContext.outContextOptionString, "RN=%s;", srvrGlobal->QSRoleName);
	}
	else
		outContext.outContextOptionStringLen = 0;


	//  +++ Fix for update stats problem on volatile table. This code was earlier
	//  just before SET_ODBC_PROCESS connection attr above.
	//	Have moved the BEGIN_SESSION here to fix an issue with AQR.

	//
	//	Session ID:
	//	===========
	//	MXID<version><segment><cpu><pin><processStartTS><sessNum><unLen><userName><snLen><sessionName>
	//	<version>:         version number of ID                   : 2 digits
	//	<segment>:         segment number                         : 3 digits
	//	<cpu>:             cpu number                             : 2 digits
	//	<pin>:             pin                                    : 4 digits
	//	<processStartTS>:  time when master exe process started   : 18 digits
	//	<sessNum>:         sequentially increasing session number : 10 digits
	//	<unLen>:           length of user ALIAS name              : 2 digits
	//	<userName>:        actual user name                       : unLen bytes(max 32)
	//	<snLen>:           length of user specified session name  : 2 digits
	//	<sessionName>:     actual session name                    : snLen bytes(max 24)

	//	Query ID:
	//	=========
	//	<Session ID>_<queryNum>_<userStmtName>
	//	<queryNum>:       unique query number                    : max 18 digits
	//	<userStmtName>:   odbc generated stmt name               : max 32 bytes

	//	Max Query ID Len:  160 bytes

        odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
                                                                                call_id_,
                                                                                &setConnectException,
                                                                                dialogueId,
                                                                                BEGIN_SESSION,
                                                                                0,
                                                                                (IDL_string)inContext->sessionName,
                                                                                &sqlWarning
                                                                                );
	if (setConnectException.exception_nr != CEE_SUCCESS)
	{
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "BEGIN_SESSION", "");
		goto MapException;
	}
	else
	{
		enum SESSIONATTR_TYPE {SESSION_ATTR_ID = 1};
		char tmpsrvrSessionId[SESSION_ID_LEN];
		Int32 tmpsrvrSessionIdLen=0;

		if (WSQL_EXEC_GetSessionAttr(1, 0, tmpsrvrSessionId, SESSION_ID_LEN, &tmpsrvrSessionIdLen) == 0)
		{
			tmpsrvrSessionId[tmpsrvrSessionIdLen] = '\0';
			strcpy(srvrGlobal->sessionId,tmpsrvrSessionId);
		}
		else
		{
			getSessionId(tmpsrvrSessionId);
			strcpy(srvrGlobal->sessionId,tmpsrvrSessionId);
		}
	}

	if (srvrGlobal->srvrState == SRVR_CONNECTING)
	{
	   updateSrvrState(SRVR_CONNECTED);
	}

	// For performance reasons, SQL statements to setup the initial context
	// are executed after responding back to client
	//


	odbc_SQLSvc_InitializeDialogue_ts_res_(objtag_, call_id_, &exception_, &outContext);

	if (outContext.outContextOptionStringLen > 0)
		delete [] outContext.outContextOptionString;

	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceConnectExit(exception_, outContext);
	}

	odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
										call_id_,
										&setConnectException,
										dialogueId,
										SET_ODBC_PROCESS,
										0,
										NULL,
										&sqlWarning
										);
	if (setConnectException.exception_nr != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "SET_ODBC_PROCESS", "");
		goto MapException;
//LCOV_EXCL_STOP
	}

	odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
										call_id_,
										&setConnectException,
										dialogueId,
										WMS_QUERY_MONITORING,
										0,
										NULL,
										&sqlWarning
										);
	if (setConnectException.exception_nr != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "WMS_QUERY_MONITORING", "");
		goto MapException;
//LCOV_EXCL_STOP
	}


// Need to enable this for JDBC driver
	odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
										call_id_,
										&setConnectException,
										dialogueId,
										SET_JDBC_PROCESS,
										0,
										NULL,
										&sqlWarning
										);
	if (setConnectException.exception_nr != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "SET_JDBC_PROCESS", "");
		goto MapException;
//LCOV_EXCL_STOP
	}

// Need to enable this for NCI
	if (strcmp(srvrGlobal->ApplicationName, HPDCI_APPLICATION) == 0)
	{
		odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
											call_id_,
											&setConnectException,
											dialogueId,
											SET_NVCI_PROCESS,
											0,
											NULL,
											&sqlWarning
											);
		if (setConnectException.exception_nr != CEE_SUCCESS)
		{
//LCOV_EXCL_START
			SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
				srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
				2, "SET_NVCI_PROCESS", "");
			goto MapException;
//LCOV_EXCL_STOP
		}
	}

// Need to enable this for to generate explain plans by default.
	odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
										call_id_,
										&setConnectException,
										dialogueId,
										SET_EXPLAIN_PLAN,
										0,
										NULL,
										&sqlWarning
										);
	if (setConnectException.exception_nr != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "SET_EXPLAIN_PLAN", "");
		goto MapException;
//LCOV_EXCL_STOP
	}

	// This is added for dynamic reconfiguration. To reset the nametype back to ANSI.
	// Then is set according to Data Source configured.
	odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
										call_id_,
										&setConnectException,
										dialogueId,
										SET_CATALOGNAMETYPE,
										0,
										NULL,
										&sqlWarning
										);
	if (setConnectException.exception_nr != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "SET_CATALOGNAMETYPE", "");
		goto MapException;
//LCOV_EXCL_STOP
	}


	odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
										call_id_,
										&setConnectException,
										dialogueId,
										SET_AUTOBEGIN,
										0,
										NULL,
										&sqlWarning
										);
	if (setConnectException.exception_nr != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "SET_AUTOBEGIN", "");
		goto MapException;
//LCOV_EXCL_STOP
	}

	odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
										call_id_,
										&setConnectException,
										dialogueId,
										SQL_AUTOCOMMIT,
										inContext->autoCommit,
										NULL
										, &sqlWarning
										);

	if (setConnectException.exception_nr != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		sprintf(tmpString, "%ld", inContext->autoCommit);
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "SQL_AUTOCOMMIT", tmpString);
		goto MapException;
//LCOV_EXCL_STOP
	}

	srvrGlobal->estCardinality = srvrGlobal->estCost = -1;
	if (srvrGlobal->envVariableOn)
	{
		int len_length = srvrGlobal->srvrContext.envDescList._length;
		ENV_DESC_def *p_buffer = (ENV_DESC_def *)srvrGlobal->srvrContext.envDescList._buffer;
		char* saveptr;

		for (curRowNo = 0; curRowNo < len_length; curRowNo ++)  // scan through each RG policy
		{
			pEnvDesc = p_buffer + curRowNo;
			//  VarType
			if ((pEnvDesc->VarType == ENV_SET) || (pEnvDesc->VarType == ENV_CONTROL))// Set & Control statements
			{
				strncpy(VariableValue, pEnvDesc->VarVal, sizeof(VariableValue));
				VariableValue[sizeof(VariableValue)-1] = 0;
				token = strtok_r(VariableValue, seps, &saveptr );
				if (_stricmp(token, EnvTypes[0]) == 0)
				{
					token = strtok_r(NULL, seps, &saveptr);
					if (_stricmp(token, ATTR_TYPE1) == 0)
					{
						token = strtok_r(NULL, seps, &saveptr);
						if (_stricmp(token, ATTR_TYPE1_VALUE1) == 0)
						{
							odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
														call_id_,
														&setConnectException,
														dialogueId,
														SQL_ACCESS_MODE,
														SQL_MODE_READ_WRITE,
														NULL
														, &sqlWarning
														);
						}
						else if (_stricmp(token, ATTR_TYPE1_VALUE2) == 0)
						{
							odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
														call_id_,
														&setConnectException,
														dialogueId,
														SQL_ACCESS_MODE,
														SQL_MODE_READ_ONLY,
														NULL
														, &sqlWarning
														);
						}
						else
							InternalUse = FALSE;
						token = strtok_r(NULL, seps, &saveptr); // Check for forth token should be NULL else error.
						if (token != NULL)
							InternalUse = FALSE;
					}
					else if (_stricmp(token, ATTR_TYPE2) == 0)
					{
						token = strtok_r(NULL, seps, &saveptr);
						if (_stricmp(token, ATTR_TYPE2_VALUE1) == 0)
						{
							odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
														call_id_,
														&setConnectException,
														dialogueId,
														SQL_TXN_ISOLATION,
														SQL_TXN_READ_UNCOMMITTED,
														NULL
														, &sqlWarning
														);
						}
						else if (_stricmp(token, ATTR_TYPE2_VALUE2) == 0)
						{
							odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
														call_id_,
														&setConnectException,
														dialogueId,
														SQL_TXN_ISOLATION,
														SQL_TXN_READ_COMMITTED,
														NULL
														, &sqlWarning
														);
						}
						else if (_stricmp(token, ATTR_TYPE2_VALUE3) == 0)
						{
							odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
														call_id_,
														&setConnectException,
														dialogueId,
														SQL_TXN_ISOLATION,
														SQL_TXN_REPEATABLE_READ,
														NULL
														, &sqlWarning
														);
						}
						else if (_stricmp(token, ATTR_TYPE2_VALUE4) == 0)
						{
							odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
														call_id_,
														&setConnectException,
														dialogueId,
														SQL_TXN_ISOLATION,
														SQL_TXN_SERIALIZABLE,
														NULL
														, &sqlWarning
														);
						}
						else
							InternalUse = FALSE;
						token = strtok_r(NULL, seps, &saveptr); // Check for forth token should be NULL else error.
						if (token != NULL)
							InternalUse = FALSE;
					}
					else if (_stricmp(token, ATTR_TYPE3) == 0)
					{
						token = strtok_r(NULL, seps, &saveptr);
						if (_stricmp(token, ATTR_TYPE3_VALUE1) == 0)
						{
							srvrGlobal->EnvironmentType = srvrGlobal->EnvironmentType | MXO_MSACCESS_1997;
							odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
														call_id_,
														&setConnectException,
														dialogueId,
														SQL_ACCESS_MODE,
														SQL_MODE_READ_WRITE,
														NULL
														, &sqlWarning
														);
						}
						else if (_stricmp(token, ATTR_TYPE3_VALUE2) == 0)
						{
							srvrGlobal->EnvironmentType = srvrGlobal->EnvironmentType | MXO_MSACCESS_2000;
							odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
														call_id_,
														&setConnectException,
														dialogueId,
														SQL_ACCESS_MODE,
														SQL_MODE_READ_WRITE,
														NULL
														, &sqlWarning
														);
						}
						else
							InternalUse = FALSE;
						token = strtok_r(NULL, seps, &saveptr); // Check for forth token should be NULL else error.
						if (token != NULL)
							InternalUse = FALSE;
					}
					else if (_stricmp(token, ATTR_TYPE4) == 0)
					{
						token = strtok_r(NULL, seps, &saveptr);
						if (_stricmp(token, ATTR_TYPE4_VALUE1) == 0)
						{
							srvrGlobal->EnvironmentType = srvrGlobal->EnvironmentType | MXO_BIGINT_NUMERIC;
						}
						else
							InternalUse = FALSE;
						token = strtok_r(NULL, seps, &saveptr); // Check for forth token should be NULL else error.
						if (token != NULL)
							InternalUse = FALSE;
					}
					else if (_stricmp(token, ATTR_TYPE5) == 0)//for error recovery
					{
						token = strtok_r(NULL, seps, &saveptr);
						if (_stricmp(token, ATTR_TYPE5_VALUE1) == 0)
						{
							srvrGlobal->EnvironmentType = srvrGlobal->EnvironmentType | MXO_ROWSET_ERROR_RECOVERY;
						}
						else
							InternalUse = FALSE;
						token = strtok_r(NULL, seps, &saveptr); // Check for forth token should be NULL else error.
						if (token != NULL)
							InternalUse = FALSE;
					}
					else if (_stricmp(token, ATTR_TYPE6) == 0) //for metadata id
					{
						token = strtok_r(NULL, seps, &saveptr);
						if (_stricmp(token, ATTR_TYPE6_VALUE1) == 0)
						{
							srvrGlobal->EnvironmentType = srvrGlobal->EnvironmentType | MXO_METADATA_ID;
						}
						else
							InternalUse = FALSE;
						token = strtok_r(NULL, seps, &saveptr); // Check for forth token should be NULL else error.
						if (token != NULL)
							InternalUse = FALSE;
					}
					else if (_stricmp(token, ATTR_TYPE7) == 0)// Rajani - Check for password expiry
					{
						token = strtok_r(NULL, seps, &saveptr);
						if ((_stricmp(token, ATTR_TYPE7_VALUE1) == 0) || (_stricmp(token, ATTR_TYPE7_VALUE2) == 0))
						{
							// DO Nothing since we already tookcare of it before calling odbc_SQLSvc_InitializeDialogue_sme_
						}
						else
							InternalUse = FALSE;
						token = strtok_r(NULL, seps, &saveptr); // Check for forth token should be NULL else error.
						if (token != NULL)
							InternalUse = FALSE;
					}
					else if (_stricmp(token, ATTR_TYPE8) == 0)//for microsec option
					{
						token = strtok_r(NULL, seps, &saveptr);
						if (_stricmp(token, ATTR_TYPE8_VALUE1) == 0) //for the microsecs option
						{
							srvrGlobal->EnvironmentType = srvrGlobal->EnvironmentType | MXO_FRACTION_IN_MICROSECS;
						}
						else if(_stricmp(token, ATTR_TYPE8_VALUE2) == 0)
						{
							srvrGlobal->EnvironmentType = srvrGlobal->EnvironmentType | MXO_FRACTION_IN_NANOSECS;
						}
						else
							InternalUse = FALSE;
						token = strtok_r(NULL, seps, &saveptr); // Check for forth token should be NULL else error.
						if (token != NULL)
							InternalUse = FALSE;
					}
					// generic SET attributes
					else if (_stricmp(token, ATTR_TYPE9) == 0)
					{
						long temp_val = 0;
						double est_val = 0.0;
						char seps2[] = " \t\n,:";
						token = strtok_r(NULL, seps2, &saveptr); // CLEANUP_CONNECTION | CLEANUP_TIME | FATAL_ERROR
						if (_stricmp(token, ATTR_TYPE9_VALUE1) == 0) // CLEANUP_CONNECTION
						{
							token = strtok_r(NULL,seps2, &saveptr);
							if (token != NULL)
							{
								temp_val = atol(token);
								if (temp_val < 1)
									InternalUse = FALSE;
								else
									srvrGlobal->cleanupByConnection = temp_val;
							}
							else
								InternalUse = FALSE;
						}
						else if (_stricmp(token, ATTR_TYPE9_VALUE2) == 0) // CLEANUP_TIME
						{
							token = strtok_r(NULL,seps2, &saveptr);
							if (token != NULL)
							{
								temp_val = atol(token);
								if (temp_val < 1)
									InternalUse = FALSE;
								else
									srvrGlobal->cleanupByTime = temp_val;
							}
							else
								InternalUse = FALSE;
						}
						// Added for workaround for cases where a large number of short
						// running queries can make the QSMGR very busy and impact the
						// overall system performance. Compiler estimates can now be
						// entered in the DSN and MXOSRVR will bypass WMS for queries with
						// estimates lower than those entered.
						// This is currently an undocumented feature and will be exposed
						// only as need basis.
						else if (_stricmp(token, ATTR_TYPE9_VALUE3) == 0) // EST_CARDINALITY
						{
							token = strtok_r(NULL,seps2, &saveptr);
							if (token != NULL)
							{
								est_val = atof(token);
								if (est_val < 0)
									InternalUse = FALSE;
								else
									srvrGlobal->estCardinality = est_val;
							}
							else
								InternalUse = FALSE;
						}
						else if (_stricmp(token, ATTR_TYPE9_VALUE4) == 0) // EST_COST
						{
							token = strtok_r(NULL,seps2, &saveptr);
							if (token != NULL)
							{
								est_val = atof(token);
								if (est_val < 0)
									InternalUse = FALSE;
								else
									srvrGlobal->estCost = est_val;
							}
							else
								InternalUse = FALSE;
						}
						else
							InternalUse = FALSE;

						token = strtok_r(NULL, seps, &saveptr);
						if (token != NULL)
							InternalUse = FALSE;
					}
					else if (_stricmp(token, ATTR_TYPE10) == 0)//for WMS Adaptive Segmentation
					{
						if (srvrGlobal->fnumAS != -1)
								FILE_CLOSE_(srvrGlobal->fnumAS);
						srvrGlobal->fnumAS == -1;

						token = strtok_r(NULL, seps, &saveptr);
						if (_stricmp(token, ATTR_TYPE10_VALUE1) == 0) //for ON
						{
							srvrGlobal->bWMS_AdaptiveSegment = true;

							_cc_status cc;
							long timeout = AS_TIMEOUT;

							// bits <1> ON - nowait
							short option = 0x4000;

							short error = FILE_OPEN_(srvrGlobal->QSProcessName
											, strlen(srvrGlobal->QSProcessName)
											, &srvrGlobal->fnumAS
											, 0			//access
											, 0			//exclusion
											, 1			//nowait_depth
											, 0			//sync-or-receive-depth
											, option	//options
											);
							if (error == 0)
							{
								if (_status_lt(cc))
									FILE_GETINFO_ (srvrGlobal->fnumAS, &error);
								else
									error = 0;
							}
							if (error == 0)
							{
                       if (! processExists(srvrGlobal->QSProcessName,
                                                    TPT_REF(srvrGlobal->pASHandle)))

										error = 1;
							}
							if (error)
							{
								if (srvrGlobal->fnumAS != -1)	//timeout
									FILE_CLOSE_(srvrGlobal->fnumAS);
								srvrGlobal->fnumAS = -1;
							}

						}
						else if(_stricmp(token, ATTR_TYPE10_VALUE2) == 0) //for OFF
						{
							srvrGlobal->bWMS_AdaptiveSegment = false;
						}
						else
							InternalUse = FALSE;
						token = strtok_r(NULL, seps, &saveptr); // Check for forth token should be NULL else error.
						if (token != NULL)
							InternalUse = FALSE;
					}
					else if (_stricmp(token, ATTR_TYPE11) == 0) // To turn the 21036 EMS messages ON/OFF
					{
						token = strtok_r(NULL, seps, &saveptr);
						if (_stricmp(token, ATTR_TYPE11_VALUE1) == 0) //for ON
						{
							srvrGlobal->mute = false;
						}
						else if(_stricmp(token, ATTR_TYPE11_VALUE2) == 0) //for OFF
						{
							srvrGlobal->mute = true;
						}
						else
							InternalUse = FALSE;
						token = strtok_r(NULL, seps, &saveptr); // Check for forth token should be NULL else error.
						if (token != NULL)
							InternalUse = FALSE;
					}
					else if (_stricmp(token, ATTR_TYPE14) == 0)//for SQLTABLES MV option
					{
						token = strtok_r(NULL, seps, &saveptr);
						if (_stricmp(token, ATTR_TYPE14_VALUE1) == 0) //for SQLTABLES TABLE TYPE as TABLE
						{
							srvrGlobal->EnvironmentType = srvrGlobal->EnvironmentType | MXO_SQLTABLES_MV_TABLE;
						}
						else if(_stricmp(token, ATTR_TYPE14_VALUE2) == 0) //for SQLTABLES TABLE TYPE as VIEW
						{
							srvrGlobal->EnvironmentType = srvrGlobal->EnvironmentType | MXO_SQLTABLES_MV_VIEW;
						}
						else
							InternalUse = FALSE;
						token = strtok_r(NULL, seps, &saveptr); // Check for forth token should be NULL else error.
						if (token != NULL)
							InternalUse = FALSE;
					}
					else if (_stricmp(token, ATTR_TYPE15) == 0) // SQL_ATTR_IGNORE_CANCEL
					{
						token = strtok_r(NULL, seps, &saveptr);
						if ((_stricmp(token, ATTR_TYPE15_VALUE1) == 0) || (_stricmp(token, ATTR_TYPE15_VALUE2) == 0))
						{
							// DO Nothing since we already tookcare of it
						}
						else
							InternalUse = FALSE;
						token = strtok_r(NULL, seps, &saveptr); // Check for forth token should be NULL else error.
						if (token != NULL)
							InternalUse = FALSE;
					}
					else if (_stricmp(token, ATTR_TYPE16) == 0) // SQL_ATTR_FETCH_AHEAD
					{
						token = strtok_r(NULL, seps, &saveptr);
						if (_stricmp(token, ATTR_TYPE16_VALUE1) == 0) // SQL_ATTR_FETCH_AHEAD 'ON'
						{
							srvrGlobal->fetchAhead = TRUE;
						}
						else if (_stricmp(token, ATTR_TYPE16_VALUE2) == 0) // SQL_ATTR_FETCH_AHEAD 'OFF'
						{
							// SQL_ATTR_FETCH_AHEAD 'OFF' by default
							srvrGlobal->fetchAhead = FALSE;
						}
						else
							InternalUse = FALSE;
						token = strtok_r(NULL, seps, &saveptr); // Check for forth token should be NULL else error.
						if (token != NULL)
							InternalUse = FALSE;
					}
					else if (_stricmp(token, ATTR_TYPE17) == 0) // To turn the extended 21036 EMS messages ON/OFF
					{
						token = strtok_r(NULL, seps, &saveptr);
						if (_stricmp(token, ATTR_TYPE17_VALUE1) == 0) //for ON
						{
							srvrGlobal->ext_21036 = true;
						}
						else if(_stricmp(token, ATTR_TYPE17_VALUE2) == 0) //for OFF
						{
							srvrGlobal->ext_21036 = false;
						}
						else
							InternalUse = FALSE;
						token = strtok_r(NULL, seps, &saveptr); // Check for forth token should be NULL else error.
						if (token != NULL)
							InternalUse = FALSE;
					}
					else if (_stricmp(token, ATTR_TYPE18) == 0) // SQL_ATTR_ENABLE_LONGVARCHAR
					{
						token = strtok_r(NULL, seps, &saveptr);
						if (_stricmp(token, ATTR_TYPE18_VALUE1) == 0) // SQL_ATTR_ENABLE_LONGVARCHAR  'ON'
						{
							srvrGlobal->enableLongVarchar = true;
						}
						else //LONGVARCHAR is disabled (OFF) by default.
						{
							srvrGlobal->enableLongVarchar = false;
						}
						token = strtok_r(NULL, seps, &saveptr); // Check for forth token should be NULL else error.
						if (token != NULL)
							InternalUse = FALSE;
					}
					else
						InternalUse = FALSE;
				}
				else
					InternalUse = FALSE;

				if (!InternalUse)
				{
					odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
												call_id_,
												&setConnectException,
												dialogueId,
												SET_SETANDCONTROLSTMTS,
												0,
												(IDL_string)pEnvDesc->VarVal,
												&sqlWarning
												);
					InternalUse = TRUE;
				}

				if (setConnectException.exception_nr != CEE_SUCCESS)
				{
					SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
						2, "SET_SETANDCONTROLSTMTS", pEnvDesc->VarVal);
					goto MapException;
				}
			}

		}  // for loop

	}

	srvrGlobal->EnvironmentType |= MXO_ROWSET_ERROR_RECOVERY;

	if(inContext->schema[0] != '\0')
	{

		// With character-set changes we need to take care of four scenarios
		// 1. schema names in ascci with case sensitive behavior (means with and without double quotes)
		// 2. schema names in UTF-8 characters with and without double quotes
		// To make logic simple and to take care of unsymmetric double quotes in
		// schema names (for eg ""abc", ""def, edfg" ..etc) the code
		// removes all quotes if present and re-introduce them as needed.
		bool delimit = true;
		if((srvrGlobal->DefaultSchema[0] == '"') ||
				(srvrGlobal->DefaultSchema[strlen(srvrGlobal->DefaultSchema)-1] == '"'))
		{
			char tmpDefaultcatalog[MAX_SQL_IDENTIFIER_LEN+3];
			//remove multiple double quotes, if any
			char* startPtr = srvrGlobal->DefaultSchema;
			while (*startPtr == '"') ++startPtr;
			char* endPtr = NULL;
			if(startPtr < (srvrGlobal->DefaultSchema + strlen(srvrGlobal->DefaultSchema)))
			{
				endPtr = strchr(startPtr,'"');
				if(endPtr == NULL) // We have a string with no ending quotes!
				{
					strcpy(tmpDefaultcatalog, startPtr);
				}
				else
				{
					int length = endPtr-startPtr;
					strncpy(tmpDefaultcatalog,startPtr, length);
					tmpDefaultcatalog[length]='\0';
				}
				strcpy(srvrGlobal->DefaultSchema, tmpDefaultcatalog);
			}
		}
		else //we have a schema name with no " around, check whether it is ascii, then
			// we don't need to delimit the schema name
		{
				delimit = false;
			for (int i=0; i < strlen(srvrGlobal->DefaultSchema); i++)
				if (!isalnum(srvrGlobal->DefaultSchema[i]))
				{
					delimit = true;
					break;
				}
		}

		strcpy(schemaValueStr, """");
		strcat(schemaValueStr, srvrGlobal->DefaultCatalog);
		strcat(schemaValueStr, """");
		strcat(schemaValueStr, ".");
		if (delimit)
			strcat(schemaValueStr, "\"");
		strcat(schemaValueStr, srvrGlobal->DefaultSchema);
		if (delimit)
			strcat(schemaValueStr, "\"");

		odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
											call_id_,
											&setConnectException,
											dialogueId,
											SET_SCHEMA,
											0,
											(IDL_string)schemaValueStr
											,&sqlWarning
											);
		if (setConnectException.exception_nr != CEE_SUCCESS)
		{
//LCOV_EXCL_START
			SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
				srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
				2, "SET_SCHEMA", schemaValueStr);
			goto MapException;
//LCOV_EXCL_STOP
		}
	}

	odbc_SQLSvc_SetConnectionOption_sme_(objtag_,
										call_id_,
										&setConnectException,
										dialogueId,
										RESET_RESET_DEFAULTS,
										0,
										NULL,
										&sqlWarning
										);
	if (setConnectException.exception_nr != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "RESET_RESET_DEFAULTS", "");
		goto MapException;
//LCOV_EXCL_STOP
	}


	SRVR_STMT_HDL *RbwSrvrStmt;
	SRVR_STMT_HDL *CmwSrvrStmt;

	if ((RbwSrvrStmt = getSrvrStmt("STMT_ROLLBACK_1", FALSE)) != NULL)
		RbwSrvrStmt->Close(SQL_DROP);
	if ((RbwSrvrStmt = getSrvrStmt("STMT_ROLLBACK_1", TRUE)) == NULL)
	{
//LCOV_EXCL_START
		setConnectException.exception_nr = 99;
		sprintf(tmpString, "%s", "Unable to allocate statement to Rollback.");
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "STMT_ROLLBACK_1", tmpString);
		goto MapException;
//LCOV_EXCL_STOP
	}
	retCode = RbwSrvrStmt->Prepare("ROLLBACK WORK",INTERNAL_STMT,SQL_ASYNC_ENABLE_OFF, 0);
	if (retCode == SQL_ERROR)
	{
//LCOV_EXCL_START
		setConnectException.exception_nr = 99;
		sprintf(tmpString, "%s", "Error in Preparing Query for Rollback.");
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "STMT_ROLLBACK_1", tmpString);
		goto MapException;
//LCOV_EXCL_STOP
	}
	if ((CmwSrvrStmt = getSrvrStmt("STMT_COMMIT_1", FALSE)) != NULL)
		CmwSrvrStmt->Close(SQL_DROP);
	if ((CmwSrvrStmt = getSrvrStmt("STMT_COMMIT_1", TRUE)) == NULL)
	{
//LCOV_EXCL_START
		setConnectException.exception_nr = 99;
		sprintf(tmpString, "%s", "Unable to allocate statement for Commit.");
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "STMT_ROLLBACK_1", tmpString);
		goto MapException;
//LCOV_EXCL_STOP
	}
	retCode = CmwSrvrStmt->Prepare("COMMIT WORK",INTERNAL_STMT,SQL_ASYNC_ENABLE_OFF, 0);
	if (retCode == SQL_ERROR)
	{
//LCOV_EXCL_START
		setConnectException.exception_nr = 99;
		sprintf(tmpString, "%s", "Error in Preparing Query for Commit.");
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "STMT_ROLLBACK_1", tmpString);
		goto MapException;
//LCOV_EXCL_STOP
	}

	// batch job support for T4
	SRVR_STMT_HDL *TranOnSrvrStmt;
	SRVR_STMT_HDL *TranOffSrvrStmt;

	if ((TranOnSrvrStmt = getSrvrStmt("STMT_TRANS_ON_1", FALSE)) != NULL)
		TranOnSrvrStmt->Close(SQL_DROP);
	if ((TranOnSrvrStmt = getSrvrStmt("STMT_TRANS_ON_1", TRUE)) == NULL)
	{
//LCOV_EXCL_START
		setConnectException.exception_nr = 99;
		sprintf(tmpString, "%s", "Unable to allocate statement to set transaction on.");
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "STMT_TRANS_ON_1", tmpString);
		goto MapException;
//LCOV_EXCL_STOP
	}
	retCode = TranOnSrvrStmt->Prepare("SET TRANSACTION AUTOCOMMIT ON",INTERNAL_STMT,SQL_ASYNC_ENABLE_OFF, 0);
	if (retCode == SQL_ERROR)
	{
//LCOV_EXCL_START
		setConnectException.exception_nr = 99;
		sprintf(tmpString, "%s", "Error in Preparing Query for set transaction on.");
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "STMT_TRANS_ON_1", tmpString);
		goto MapException;
//LCOV_EXCL_STOP
	}
	if ((TranOffSrvrStmt = getSrvrStmt("STMT_TRANS_OFF_1", FALSE)) != NULL)
		TranOffSrvrStmt->Close(SQL_DROP);
	if ((TranOffSrvrStmt = getSrvrStmt("STMT_TRANS_OFF_1", TRUE)) == NULL)
	{
//LCOV_EXCL_START
		setConnectException.exception_nr = 99;
		sprintf(tmpString, "%s", "Unable to allocate statement to set transaction off.");
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "STMT_TRANS_OFF_1", tmpString);
		goto MapException;
//LCOV_EXCL_STOP
	}
	retCode = TranOffSrvrStmt->Prepare("SET TRANSACTION AUTOCOMMIT OFF",INTERNAL_STMT,SQL_ASYNC_ENABLE_OFF, 0);
	if (retCode == SQL_ERROR)
	{
//LCOV_EXCL_START
		setConnectException.exception_nr = 99;
		sprintf(tmpString, "%s", "Error in Preparing Query for set transaction off.");
		SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, "STMT_TRANS_OFF_1", tmpString);
		goto MapException;
//LCOV_EXCL_STOP
	}

	srvrGlobal->javaConnIdleTimeout = JDBC_DATASOURCE_CONN_IDLE_TIMEOUT;
	if ((srvrGlobal->drvrVersion.componentId == JDBC_DRVR_COMPONENT) && ((long) (inContext->idleTimeoutSec) > JDBC_DATASOURCE_CONN_IDLE_TIMEOUT))
		srvrGlobal->javaConnIdleTimeout = inContext->idleTimeoutSec;

    srvrGlobal->clipVarchar = 0;
	// collect information for resource statistics
	char nodename[100];
	short error;
	char cpuPin[20];
	char systemNm[10];
	short priority;
	char procName[MAX_PROCESS_NAME_LEN];
	char userName[UNLEN + 1 + UNLEN + 1];
	short username_len;

	memset(setinit.cpuPin,'\0',20);
	memset(setinit.nodeName,'\0',10);
	memset(setinit.DSName,'\0',MAX_DSOURCE_NAME + 1);
	memset(setinit.userName,'\0',USERNAME_LENGTH + 1);
	setinit.userId = 0;
	setinit.startPriority = 0;

//	The following two are directly setup in srvrothers.cpp
//	setinit.totalLoginTime = 0;
//	setinit.ldapLoginTime = 0;

	sprintf(cpuPin,"%d,%d",srvrGlobal->nskProcessInfo.nodeId,srvrGlobal->nskProcessInfo.processId);
	strcpy(setinit.cpuPin,cpuPin);
	strcpyUTF8(setinit.userName,userDesc->userName, sizeof(setinit.userName));
	strcpyUTF8(setinit.DSName,srvrGlobal->DSName, sizeof(setinit.DSName));
	GetSystemNm(systemNm);
	strcpy(setinit.nodeName,systemNm);

	// Modified below code for replacing the expensive USER_GETINFO_() call with
	// PROCESS_GETINFO() call for better performance.
	int crID;

	crID = userSession->getUserID();
	userSession->getDBUserName(srvrGlobal->QSDBUserName, sizeof(srvrGlobal->QSDBUserName));

	// Get the current external name of the user.

	userSession->getExternalUsername(srvrGlobal->QSUserName, sizeof(srvrGlobal->QSUserName));



	strcpyUTF8(setinit.userName,srvrGlobal->QSUserName, sizeof(setinit.userName));

	// For component privileges
	bzero(hpdcsPrivMask, sizeof(hpdcsPrivMask));

#ifdef NSK_PLATFORM
	if ((error = PROCESS_GETINFO_(TPT_REF(srvrGlobal->nskProcessInfo.pHandle),
		OMITREF, OMITSHORT,OMITREF,		// proc string,max buf len,act len
		&priority,						// priority
		OMITREF,						// Mom's proc handle
		OMITREF, OMITSHORT,OMITREF,		// home term,max buf len,act len
		OMITREF,						// Process execution time
		&crID,							// Creator Access Id
		OMITREF,						// Process Access Id
		OMITREF,						// Grand Mom's proc handle
		OMITREF,						// Job Id
		OMITREF, OMITSHORT,OMITREF,		// Program file,max buf len,act len
		OMITREF, OMITSHORT,OMITREF,		// Swap file,max buf len,act len
		OMITREF,
		OMITREF,						// Process type
		OMITREF) ) != 0)			    // OSS or NT process Id
	{
		sprintf(tmpString, "%d", error);
		SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			1, tmpString);
	}
	setinit.startPriority = priority;

#else
   MS_Mon_Process_Info_Type  proc_info;
	char  myProcname[128];
   short procname_len;

	if ((error = PROCESSHANDLE_DECOMPOSE_ (
				TPT_REF(srvrGlobal->nskProcessInfo.pHandle)
				,OMITREF				//[ short *cpu ]
				,OMITREF		//[ short *pin ]
				,OMITREF		//[ long *nodenumber ]
				,OMITREF			//[ char *nodename ]
				,OMITSHORT	//[ short maxlen ]
				,OMITREF		//[ short *nodename-length ]
				,myProcname			//[ char *procname ]
				,sizeof(myProcname)	//[ short maxlen ]
				,&procname_len		//[ short *procname-length ]
				,OMITREF			//[ long long *sequence-number ]
				)) != 0)
	{
//LCOV_EXCL_START
		sprintf(tmpString, "%d", error);
		SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			1, tmpString);
//LCOV_EXCL_STOP
	}

	myProcname[procname_len] = 0;

   error = msg_mon_get_process_info_detail(myProcname, &proc_info);
   if (error != XZFIL_ERR_OK )
   {
//LCOV_EXCL_START
		sprintf(tmpString, "%d", error);
		SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			1, tmpString);
//LCOV_EXCL_STOP
   }
	setinit.startPriority = priority = (short)proc_info.priority;

	srvrGlobal->process_id = proc_info.pid;
	srvrGlobal->cpu = proc_info.nid;
#endif

	srvrGlobal->ProcessAccessId = setinit.userId = crID;

	// Set the userSID for WMS.
	// strcpy(srvrGlobal->userSID, userDesc->userName);

	strcpy(srvrGlobal->userSID, srvrGlobal->QSUserName);

	//	srvrGlobal->srvrPriority = priority;

	// Fix for WMS SQL_DEFAULTS not resetting back to original
	// process priority. When the priority is changed by one service
	// and a new set service does NOT have a process priority
	// set then it should default to the priority that the process
	// started off with.
	// ++++ Note: When service-level default process priority feature is enabled
	// ++++ then this code will not be relevant and should be removed at that time.
	static bool prtySet = false;
	if ( !prtySet )
	{
		srvrGlobal->srvrPriority = priority;
		prtySet = true;
	}
	srvrGlobal->prtyChanged = false;
	// If the client sets the fetchbuffer size to zero then we don't allocate memory
	// the output buffer in the case of unique selects and can corrupt memory in
	// SRVR::BuildSQLDesc2().
	// Defaulting to 512K if the fetch buffer size is set as zero.
	if (inContext->rowSetSize <= 0)
		srvrGlobal->m_FetchBufferSize = 524288;
	else
		srvrGlobal->m_FetchBufferSize = inContext->rowSetSize*1024;

	// Moved watch dog thread creation from ImplInit() to below to avoid some initialization issues
	static bool sessionWatchDogStarted = false;
	if (srvrGlobal->m_bStatisticsEnabled && !sessionWatchDogStarted)
	{
		//boost::thread thrd(&SessionWatchDog);
		pthread_t thrd;
		pthread_create(&thrd, NULL, SessionWatchDog, NULL);
		sessionWatchDogStarted = true;
	}

	if (resStatSession != NULL)
	{
		resStatSession->init();
		resStatSession->start(&setinit);		
		if ((srvrGlobal->m_bStatisticsEnabled)&&(srvrGlobal->m_statisticsPubType==STATISTICS_AGGREGATED))
		{
			if (CEE_HANDLE_IS_NIL(&StatisticsTimerHandle) == IDL_FALSE)
			{
				CEE_TIMER_DESTROY(&StatisticsTimerHandle);
				CEE_HANDLE_SET_NIL(&StatisticsTimerHandle);
			}		
			interval_count=0;
			CEE_TIMER_CREATE2(MIN_INTERVAL, 0, StatisticsTimerExpired, (CEE_tag_def)NULL, &StatisticsTimerHandle, srvrGlobal->receiveThrId);
		}
	}

	if (resStatStatement != NULL)
	{
		// if statement is on
		resStatStatement->prepareQuery(&setinit);
	}

	// Trying to preserve the cached SQL objects in case of invalid user and hence doing this
	// after WSQL_EXEC_Set_AuthID call. However I am not sure of the effects in releasing the SQL objects
	// when effective user ID is changed.

	if( crID != srvrGlobal->userID )
	{
		releaseCachedObject(TRUE, NDCS_DLG_INIT);
		srvrGlobal->userID = crID;
	}

	else
	{

		// If the ID is the same, then check if the compiler cache related to roles needs
		// to be cleared.
		//
		// Do note in the case when the crID is not the same as userID
		// new compilers get started - hence there is no need to clear compiler cache

		if ( prevRedefTime != 0 &&
		     prevRedefTime != srvrGlobal->redefTime )
		{
			char errorMsg[100] = {};

			if (! SRVR::CompilerCacheReset(errorMsg))
			{
//LCOV_EXCL_START
				SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
					2, "COMPILER_CACHE_RESET", errorMsg);

				SendEventMsg(MSG_SRVR_POST_CONNECT_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
					2, "COMPILER_CACHE_RESET", "Fatal Error - Server exiting");

				exitServerProcess();
//LCOV_EXCL_STOP
			}
		}
	}
//

	if( (maxHeapPctExit != 0) &&  (initSessMemSize == 0))
		initSessMemSize = getMemSize("Initial");

	return;

MapException:
	// Write to event log and update to SRVR CONNECT FAILED
	//
	IDL_unsigned_long curErrorNo;
	ERROR_DESC_def *error_desc_def;
	odbc_SQLSvc_SQLError  *SQLError;

	switch (setConnectException.exception_nr)
	{
	case CEE_SUCCESS:
		break;
	case odbc_SQLSvc_SetConnectionOption_SQLError_exn_ :
		{
//LCOV_EXCL_START
			SQLError = &setConnectException.u.SQLError;
			int len_length = SQLError->errorList._length;
			ERROR_DESC_def *p_buffer = SQLError->errorList._buffer;
			char *UTF8ErrorText = NULL;
			long UTF8ErrorTextLen = 0;
			for (curErrorNo = 0;curErrorNo < len_length ; curErrorNo++)
			{
				error_desc_def = p_buffer + curErrorNo;
				if( error_desc_def->sqlcode == 0 && error_desc_def->errorText == NULL )
					continue;

//				Check for error -8841. This error happens if transaction is aborted externally.
//				User process is expected to clear the transaction state by calling ROLLBACK or COMMIT WORK.
//
//				Since during connection time a user initiated ROLLBACK is not possible,
//				we report this as fatal error and exit.

				if( error_desc_def->sqlcode == -8841 )
					sprintf(tmpString, "%ld returned during connection (Fatal error). Server exiting", error_desc_def->sqlcode);
				else
					sprintf(tmpString, "%ld", error_desc_def->sqlcode);

				UTF8ErrorTextLen = strlen(error_desc_def->errorText)*4;
				markNewOperator,UTF8ErrorText = new char[UTF8ErrorTextLen];
				translateToUTF8(srvrGlobal->isoMapping, error_desc_def->errorText, strlen(error_desc_def->errorText), UTF8ErrorText, UTF8ErrorTextLen);
				SendEventMsg(MSG_SQL_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
					3, ODBCMX_SERVER, tmpString, UTF8ErrorText);
				delete [] UTF8ErrorText;

				if( error_desc_def->sqlcode == -8841 )
					exitServerProcess();
			}
		}
		break;
	case odbc_SQLSvc_SetConnectionOption_ParamError_exn_:
		SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			1, setConnectException.u.ParamError.ParamDesc);
		break;
	case odbc_SQLSvc_SetConnectionOption_InvalidConnection_exn_:
	case odbc_SQLSvc_SetConnectionOption_SQLInvalidHandle_exn_:
		break;
	default:
		sprintf(tmpString, "%ld", setConnectException.exception_nr);
		SendEventMsg(MSG_KRYPTON_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, tmpString, FORMAT_LAST_ERROR());
		break;
//LCOV_EXCL_STOP
	}
	if (! updateSrvrState(SRVR_CONNECT_FAILED))
		return;
	SRVRTRACE_EXIT(FILE_AME+5);
	return;
}


/*
 * Asynchronous method function prototype for
 * operation 'odbc_SQLSvc_TerminateDialogue'
 */
extern "C" void
odbc_SQLSvc_TerminateDialogue_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  )
{
	SRVRTRACE_ENTER(FILE_AME+6);
	long status = 0;

	odbc_SQLSvc_TerminateDialogue_exc_ exception_={0,0,0};
	odbc_SQLSvc_MonitorCall_exc_	monitorException_={0,0};

	exception_.exception_nr = CEE_SUCCESS;

    long exitSesMemSize = 0;

    char tmpStringEnv[1024];
    sprintf(tmpStringEnv,
		   "Client %s Disconnecting: Data Source: %s, Application: %s, Server Reference: %s",
		    srvrGlobal->ClientComputerName,
		    srvrGlobal->DSName,
		    srvrGlobal->ApplicationName,
		    srvrGlobal->srvrObjRef);

    if (srvrGlobal->traceLogger != NULL)
    {
//LCOV_EXCL_START
		SendEventMsg(MSG_SERVER_TRACE_INFO
						, EVENTLOG_INFORMATION_TYPE
						, srvrGlobal->nskProcessInfo.processId
						, ODBCMX_SERVER
						, srvrGlobal->srvrObjRef
						, 4
						, srvrGlobal->sessionId
						, "TerminateDialog"
						, "0"
						, tmpStringEnv);

		srvrGlobal->traceLogger->TraceDisconnectEnter(dialogueId);
//LCOV_EXCL_STOP
	}

    // Suspend any joined txn if they are still active
    if( srvrGlobal->bspjTxnJoined && srvrGlobal->spjTxnId != 0)
    {
		status = SUSPENDTRANSACTION( (short*)&(srvrGlobal->spjTxnId) );	// ??? Int32 enough
		if(status != 0)
		{
			exception_.exception_nr = odbc_SQLSvc_TerminateDialogue_SQLError_exn_;
			exception_.exception_detail = 25000;
			odbc_SQLSvc_TerminateDialogue_ts_res_(objtag_, call_id_, &exception_);
			exitServerProcess();
		}
		srvrGlobal->bspjTxnJoined = FALSE;
		srvrGlobal->spjTxnId = 0;
    }
    else
    {
		status = WSQL_EXEC_Xact(SQLTRANS_STATUS,NULL);
		if (srvrGlobal->bAutoCommitOn == FALSE)
		{
			if (status == 0)
			{
	// transaction is running for autocommit set to off
				exception_.exception_nr = odbc_SQLSvc_TerminateDialogue_SQLError_exn_;
				exception_.exception_detail = 25000;
				odbc_SQLSvc_TerminateDialogue_ts_res_(objtag_, call_id_, &exception_);
				goto bailout;
			}
		}
	}

	if (srvrGlobal->srvrState == SRVR_CONNECTED)
	{
		if (dialogueId != srvrGlobal->dialogueId)
			exception_.exception_nr = odbc_SQLSvc_TerminateDialogue_InvalidConnection_exn_;
	}
	else
		exception_.exception_nr = odbc_SQLSvc_TerminateDialogue_InvalidConnection_exn_;

	if (exception_.exception_nr == CEE_SUCCESS)
	{
		if (srvrGlobal->tip_gateway != NULL)
		{
			#ifdef TIP_DEFINED
				tip_close(srvrGlobal->tip_gateway);
			#endif
			srvrGlobal->tip_gateway = NULL;
		}
		odbc_SQLSvc_TerminateDialogue_sme_(objtag_, call_id_, &exception_, dialogueId);

		releaseCachedObject(FALSE, NDCS_DLG_TERM);
		// Ignore any error, since we need to cleanup anyway
		diagnostic_flags = 0;
		srvrGlobal->bAutoCommitOn = FALSE;
                SRVR::SrvrSessionCleanup();
                srvrGlobal->dialogueId = -1;
	}

	exitSesMemSize = 0;
	if( maxHeapPctExit != 0 && initSessMemSize != 0 )
		exitSesMemSize = getMemSize("Terminate");

	if((exitSesMemSize - initSessMemSize) > initSessMemSize*maxHeapPctExit/100   )
		heapSizeExit = true;
	else
		heapSizeExit = false;


	if( heapSizeExit == false ){
		if( !updateZKState(CONNECTED, AVAILABLE) )
		{
			exception_.exception_nr = odbc_SQLSvc_TerminateDialogue_SQLError_exn_;
			exception_.exception_detail = 25000;
			odbc_SQLSvc_TerminateDialogue_ts_res_(objtag_, call_id_, &exception_);
			exitServerProcess();
		}
	}

	odbc_SQLSvc_TerminateDialogue_ts_res_(objtag_, call_id_, &exception_);

        if( heapSizeExit == true )
	{
		odbc_SQLSvc_StopServer_exc_ StopException;
		StopException.exception_nr=0;
		if (srvrGlobal->traceLogger != NULL)
		{
			srvrGlobal->traceLogger->TraceStopServerExit(StopException);
		}
		exitServerProcess();
	}

bailout:
	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceDisconnectExit(exception_);
	}
	SRVRTRACE_EXIT(FILE_AME+6);
	return;
}

void __cdecl SRVR::SrvrSessionCleanup(void)
{
	double  t = 0;

	// Rollback the transaction, Don't bother to check if autocommit is on or off, since SQL
	// doesn't check for it
	// When there is no transaction outstanding, SQL would give an error and ignore this error.

    // Suspend any joined txn if they are still active
	short status;
    if( srvrGlobal->bspjTxnJoined && srvrGlobal->spjTxnId != 0)
    {
		status = SUSPENDTRANSACTION( (short*)&(srvrGlobal->spjTxnId) );
		if(status != 0)
			exitServerProcess();

		srvrGlobal->bspjTxnJoined = FALSE;
		srvrGlobal->spjTxnId = 0;
    }
    else
    {
		if (WSQL_EXEC_Xact(SQLTRANS_STATUS,NULL) == 0)
			if (EXECDIRECT("ROLLBACK WORK") == ODBC_SERVER_ERROR)
				exitServerProcess();
    }
	releaseCachedObject(FALSE, NDCS_DLG_BREAK);

	// resource statistics
	if (resStatSession != NULL)
	{
		resStatSession->end();
		if (CEE_HANDLE_IS_NIL(&StatisticsTimerHandle) == IDL_FALSE)
		{
			CEE_TIMER_DESTROY(&StatisticsTimerHandle);
			CEE_HANDLE_SET_NIL(&StatisticsTimerHandle);
		}
	}

	//end rs
	strcpy(srvrGlobal->sessionId, srvrSessionId);


	if (srvrGlobal->cleanupByTime > 0)
	   t = difftime(time(NULL), srvrGlobal->lastCleanupTime); // seconds
	else
	   t = 0;

	// Fix for - The below code has been moved to before the
	// end of session call since this was causing issues with RMS.
	// If the service context does not have a priority set then have to
	// default to the priority what the process was started with.
	// We set only the master priority. SQL will adjust the compiler and
	// ESP priorities accordingly.
	// ++++ Note: When service-level default process priority feature is enabled
	// ++++ then this code will not be relevant and should be removed at that time.
	if( srvrGlobal->prtyChanged )
	{
	   char sqlcmd[64];
	   sprintf(sqlcmd, "SET SESSION DEFAULT MASTER_PRIORITY '%d'",
	           srvrGlobal->srvrPriority );
	   EXECDIRECT(sqlcmd);
	   srvrGlobal->prtyChanged = false;
	}

	// Fixed a problem when AutoCommit is OFF and SQL starts a transaction
	// during session end (for e.g. dropping of any volatile tables). In this
	// case the transaction does not get commited and new connections to the
	// server fails and the server process becomes useless. The fix
	// is to set AutoCommit to ON so that SQL can commit the transaction.
	// The fix is marked as "AutoCommit OFF fix" below.

	// AutoCommit OFF fix
	EXECDIRECT("SET TRANSACTION AUTOCOMMIT ON");

	if ((srvrGlobal->numConnection+1 == srvrGlobal->cleanupByConnection) || (t > (double)srvrGlobal->cleanupByTime * 60))
	{
	   srvrGlobal->numConnection = 0;
	   srvrGlobal->lastCleanupTime = time(NULL);
	   EXECDIRECT("SET SESSION DEFAULT SQL_SESSION 'END:CLEANUP_ESPS'");
	   // all ESPs are stopped
	   srvrGlobal->allocatedResources = 0;
	}
	else
	{
	   EXECDIRECT("SET SESSION DEFAULT SQL_SESSION 'END'");
	   if (srvrGlobal->numConnection == 2147483647)
	      srvrGlobal->numConnection = 0; // reset to prevent overflow
	   else
	      srvrGlobal->numConnection++;
	}


	ClearAdaptiveSegment();
	srvrGlobal->bConfigurationChanged = false;

	releaseGlobalBuffer();
} /* SRVR::SrvrSessionCleanUp() */

bool __cdecl SRVR::CompilerCacheReset(char *errorMsg)
{

	// Clear compiler cache by executing the following DELETE statements
	//
	// DELETE ALL FROM TABLE(QUERYCACHE('ALL','LOCAL'))
	// DELETE ALL FROM TABLE(NATABLECACHE())


	SRVR_STMT_HDL	*CmpStmt = NULL;
	SQLRETURN	retcode = SQL_SUCCESS;
	char		CmpQuery[100] = {0};

	if ((CmpStmt = getSrvrStmt("STMT_CMP_CACHE_RESET_1", TRUE)) == NULL)
	{
//LCOV_EXCL_START

		strcpy (errorMsg, "Allocate Statement");
		return false;

//LCOV_EXCL_STOP
	}

	strcpy(CmpQuery,"DELETE ALL FROM TABLE(QUERYCACHE('ALL','LOCAL'))");
	retcode = CmpStmt->ExecDirect(NULL, CmpQuery, INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
	if (retcode != SQL_ERROR)
	{
		strcpy(CmpQuery,"DELETE ALL FROM TABLE(NATABLECACHE())");
		retcode = CmpStmt->ExecDirect(NULL, CmpQuery, INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
	}


	CmpStmt->InternalStmtClose(SQL_DROP);

//LCOV_EXCL_START

	if (retcode == SQL_ERROR)
	{
		strcpy (errorMsg, "Error while clearing internal compiler cache");
		return false;
	}

//LCOV_EXCL_STOP

	return true;

} /* SRVR::CompilerCacheReset() */

void __cdecl SRVR::BreakDialogue(CEE_tag_def monitor_tag)
{
	long exitSesMemSize = 0;
	SRVRTRACE_ENTER(FILE_AME+7);
	if (srvrGlobal->srvrState == SRVR_AVAILABLE)
	{
		updateSrvrState(SRVR_CLIENT_DISAPPEARED);
		return;
	}

        if(srvrGlobal->dialogueId != -1)
	{
           SRVR::SrvrSessionCleanup();
           srvrGlobal->dialogueId = -1;
	}

	if(srvrGlobal->stopTypeFlag == STOP_WHEN_DISCONNECTED)
	{
		odbc_SQLSvc_StopServer_exc_ StopException;
		StopException.exception_nr=0;
		if (srvrGlobal->traceLogger != NULL)
		{
			srvrGlobal->traceLogger->TraceStopServerExit(StopException);
		}
		exitServerProcess();
	}


	exitSesMemSize = 0;
	if( maxHeapPctExit != 0  && initSessMemSize != 0 )
		exitSesMemSize = getMemSize("Break");

	if((exitSesMemSize - initSessMemSize) > initSessMemSize*maxHeapPctExit/100   )
	{
		odbc_SQLSvc_StopServer_exc_ StopException;
		StopException.exception_nr=0;
		if (srvrGlobal->traceLogger != NULL)
		{
			srvrGlobal->traceLogger->TraceStopServerExit(StopException);
		}

		exitServerProcess();
	}

	if (srvrGlobal->srvrState == SRVR_CONNECTED)
	{
		updateSrvrState(SRVR_DISCONNECTED);
	}
	else
		updateSrvrState(SRVR_CLIENT_DISAPPEARED);


	SRVRTRACE_EXIT(FILE_AME+7);
}

// Timer Expiration routine, when srvrIdleTimeout expires
void __cdecl SRVR::srvrIdleTimerExpired(CEE_tag_def timer_tag)
{
	SRVRTRACE_ENTER(FILE_AME+9);
	CEE_status sts;
	char tmpString[128];

    if(srvrGlobal->mutex->locked())
	   return;


	// Post a message to Assoicatio to check if the srvr should live only if the srvr state is
	// SRVR_AVAILABLE. Normally you should reach this function only when the server state is SRVR_AVAILABLE
	// The timer is destoryed when the server is connected and made active only when the srvr is disconnected
	// We should rather post a message, than make a synchornous call to association server, since the server
	// can be ready to take a connection message from the client


	if ( checkIfASSvcLives() == TRUE && srvrGlobal->srvrState == SRVR_AVAILABLE )
	{

		AS_CALL_CONTEXT* asCallContext;
		asCallContext = new AS_CALL_CONTEXT;
		if (asCallContext == NULL)
		{
//LCOV_EXCL_START
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
					srvrGlobal->srvrObjRef, 1, "asCallContext");
			exitServerProcess();
//LCOV_EXCL_STOP
		}

		if((sts = odbcas_ASSvc_WouldLikeToLive_pst_(
			&(asCallContext->ASSvc_proxy),
			asCallContext,
			odbcas_ASSvc_WouldLikeToLive_ccf_,
			srvrGlobal->srvrType,
			srvrGlobal->srvrObjRef)) != CEE_SUCCESS)
		{
//LCOV_EXCL_START
			delete asCallContext;
			sprintf(tmpString, "%ld", sts);
			SendEventMsg(MSG_KRYPTON_ERROR, EVENTLOG_ERROR_TYPE,
				srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
				2, tmpString, FORMAT_LAST_ERROR());
			exitServerProcess();
//LCOV_EXCL_STOP
		}
	}
	SRVRTRACE_EXIT(FILE_AME+9);
}

/*
 * Call Completion function prototype for
 * operation 'odbcas_ASSvc_WouldLikeToLive'
 */
extern "C" void
odbcas_ASSvc_WouldLikeToLive_ccf_(
    /* In    */ CEE_tag_def cmptag_
  , /* In    */ const odbcas_ASSvc_WouldLikeToLive_exc_ *exception_
  , /* In    */ IDL_long lifePermit
  )
{
	SRVRTRACE_ENTER(FILE_AME+10);

	BOOL createTimer = FALSE;
	CEE_status		sts;
	char			tmpString[25];

	AS_CALL_CONTEXT	*asCallContext = (AS_CALL_CONTEXT *)cmptag_;

	if (EXECDIRECT("SET SESSION DEFAULT SQL_SESSION 'BEGIN';") == ODBC_SERVER_ERROR)
		exitServerProcess();
	if (EXECDIRECT("SET SESSION DEFAULT SQL_SESSION 'END:CLEANUP_ESPS'") == ODBC_SERVER_ERROR)
		exitServerProcess();
	// all ESPs are stopped
	srvrGlobal->allocatedResources = 0;

	if (exception_->exception_nr == CEE_SUCCESS)
	{
		// Check if server is SRVR_AVAILABLE state
		if (srvrGlobal->srvrState == SRVR_AVAILABLE)
		{
		// add the following lines of code
			if (lifePermit == DIE)
				exitServerProcess();
			else
				createTimer = TRUE;
		}
		else
			createTimer = FALSE;
	}
	else
	{
//LCOV_EXCL_START
		sprintf(tmpString, "%ld", exception_->exception_nr);
		SendEventMsg(MSG_KRYPTON_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			2, tmpString, FORMAT_LAST_ERROR());
		SendEventMsg(MSG_SRVR_IDLE_TIMEOUT_ERROR, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			0);
		// Better, this server process die, since it is idle
		exitServerProcess();
//LCOV_EXCL_STOP
	}
	delete asCallContext;
	SRVRTRACE_EXIT(FILE_AME+10);
}

long SRVR::getConnIdleTimeout()
{
	long connIdleTimeout = INFINITE_CONN_IDLE_TIMEOUT;

	if (srvrGlobal != NULL && srvrGlobal->srvrState == SRVR_CONNECTED)
	{
		if ((srvrGlobal->drvrVersion.componentId == JDBC_DRVR_COMPONENT) && (srvrGlobal->javaConnIdleTimeout > JDBC_DATASOURCE_CONN_IDLE_TIMEOUT))
		{
			if (srvrGlobal->javaConnIdleTimeout != JDBC_INFINITE_CONN_IDLE_TIMEOUT)
				connIdleTimeout = (long)srvrGlobal->javaConnIdleTimeout;
		}
		else if (srvrGlobal->srvrContext.connIdleTimeout != INFINITE_CONN_IDLE_TIMEOUT)
		{
			connIdleTimeout = (long)srvrGlobal->srvrContext.connIdleTimeout * 60;
       		}
	}
        return connIdleTimeout;
}

long SRVR::getSrvrIdleTimeout()
{
   long srvrIdleTimeout = INFINITE_SRVR_IDLE_TIMEOUT;
   if (srvrGlobal->srvrContext.srvrIdleTimeout != INFINITE_SRVR_IDLE_TIMEOUT)
      srvrIdleTimeout = (long)srvrGlobal->srvrContext.srvrIdleTimeout * 60;
   return srvrIdleTimeout;
}

BOOL SRVR::updateSrvrState(SRVR_STATE srvrState)
{
	SRVRTRACE_ENTER(FILE_AME+13);
	CEE_status sts;
	char	tmpString[32];
	double  t = 0;
	bool result = TRUE;

	// Fix for - clients hang on a connect because the server does not cleanup after an exception
	if(srvrState == SRVR_DISCONNECTED ||
	   srvrState == SRVR_CLIENT_DISAPPEARED ||
	   srvrState == SRVR_CONNECT_REJECTED ||
	   srvrState == SRVR_CONNECT_FAILED)
	{
       GTransport.m_TCPIPSystemSrvr_list->cleanup();
       GTransport.m_FSystemSrvr_list->cleanup();
	}

	switch (srvrState)
	{
	case SRVR_DISCONNECTED:
	case SRVR_CLIENT_DISAPPEARED:
	case SRVR_CONNECT_REJECTED:
	case SRVR_CONNECT_FAILED:
		//This fix is for db Transporter. Since dbt makes TMF calls for some reason SQL_EXEC_Xact
		// is not returning there is a active transaction in this case. So work around is before putting
		// server in availabe state check from active transaction and rollback. This is safe any way since
		// server can be continue to run.
		ABORTTRANSACTION();


		srvrGlobal->srvrState = SRVR_AVAILABLE;
		bool result;

		if( srvrState == SRVR_CONNECT_REJECTED || srvrState == SRVR_CONNECT_FAILED )
		{
		// Commenting the following code. MXOSRVR remains in this state
		// for timeout duration (clientConnErrorTimeOut).
		// Changing the state to AVAILABLE immediately.
		// Leaving the related code in place (ASTimerExpired)
		// if this gets revisited.
		//
		//      result = updateZKState(CONNECTING, srvrState == SRVR_CONNECT_REJECTED? CONNECT_REJECTED : CONNECT_FAILED);
			result = updateZKState(CONNECTING, AVAILABLE);
		}
		else
			result = updateZKState(CONNECTED, AVAILABLE);

		if( !result )
			exitServerProcess();

		// The server need to die, when disconnected, hence don't start any timer
		if (srvrGlobal->stopTypeFlag == STOP_WHEN_DISCONNECTED)
			break;
		// If server is available, restart timer
		if (CEE_HANDLE_IS_NIL(&srvrGlobal->ASTimerHandle) == IDL_FALSE)
		{
//LCOV_EXCL_START
			CEE_TIMER_DESTROY(&srvrGlobal->ASTimerHandle);
			CEE_HANDLE_SET_NIL(&srvrGlobal->ASTimerHandle);
//LCOV_EXCL_STOP
		}

		srvrGlobal->bSkipASTimer = false;

		CEE_TIMER_CREATE2(DEFAULT_AS_POLLING,0,ASTimerExpired,(CEE_tag_def)NULL, &srvrGlobal->ASTimerHandle,srvrGlobal->receiveThrId);
		break;
	case SRVR_CONNECTED:
		srvrGlobal->srvrState = srvrState;
		// If server is connected, stop checking AS
		if (CEE_HANDLE_IS_NIL(&srvrGlobal->ASTimerHandle) == IDL_FALSE)
		{
			srvrGlobal->bSkipASTimer = true;
		}
		if( !updateZKState(CONNECTING, CONNECTED) )
		      exitServerProcess();
		break;
	case SRVR_STOP_WHEN_DISCONNECTED:
		if (srvrGlobal->cleanupByTime > 0)
			t = difftime(time(NULL), srvrGlobal->lastCleanupTime); // seconds
		else
			t = 0;

		// AutoCommit OFF fix
		EXECDIRECT("SET TRANSACTION AUTOCOMMIT ON");

		if ((srvrGlobal->numConnection+1 == srvrGlobal->cleanupByConnection) || (t > (double)srvrGlobal->cleanupByTime * 60))
		{
			srvrGlobal->numConnection = 0;
			srvrGlobal->lastCleanupTime = time(NULL);
			EXECDIRECT("SET SESSION DEFAULT SQL_SESSION 'END:CLEANUP_ESPS'");
			// all ESPs are stopped
			srvrGlobal->allocatedResources = 0;
		}
		else
		{
			EXECDIRECT("SET SESSION DEFAULT SQL_SESSION 'END'");
			if (srvrGlobal->numConnection == 2147483647)
				srvrGlobal->numConnection = 0; // reset to prevent overflow
			else
				srvrGlobal->numConnection++;
		}
		break;
	default:
		break;
	}

	SRVRTRACE_EXIT(FILE_AME+13);
	return TRUE;
}

void SRVR::UPDATE_SERVER_WAITED(IDL_long TraceType, IDL_long StatisticsType, IDL_long ContextType,const SRVR_CONTEXT_def *srvrContext)
{
	SRVRTRACE_ENTER(FILE_AME+14);
	if (TraceType & 0x80000000)
		DISABLE_SERVER_TRACE(TraceType & ~0x80000000);
	else if (TraceType != 0)
		ENABLE_SERVER_TRACE(TraceType);

	if (StatisticsType & 0x80000000)
		DISABLE_STATISTICS();
	else if (StatisticsType != 0)
		ENABLE_STATISTICS(StatisticsType);

	if (ContextType)
		UPDATE_SERVER_CONTEXT(srvrContext);

	SRVRTRACE_EXIT(FILE_AME+14);
}
/*
 * Call Completion function pointer type for
 * operation 'odbcas_ASSvc_UpdateSrvrState'
 */
extern "C" void odbcas_ASSvc_UpdateSrvrState_ccf_(
    /* In    */ CEE_tag_def cmptag_
  , /* In    */ const odbcas_ASSvc_UpdateSrvrState_exc_ *exception_
  , /* In    */ IDL_long TraceType
  , /* In    */ IDL_long StatisticsType
  , /* In    */ IDL_long ContextType
  , /* In    */ const SRVR_CONTEXT_def *srvrContext
  )
{
	SRVRTRACE_ENTER(FILE_AME+15);
	char errorMessage[100];

	AS_CALL_CONTEXT	*asCallContext = (AS_CALL_CONTEXT *)cmptag_;

	SRVR_STATE srvrState = asCallContext->srvrState;

	delete asCallContext;

	switch (exception_->exception_nr)
	{
	case CEE_SUCCESS:
		break;
	case odbcas_ASSvc_UpdateSrvrState_ASTimeout_exn_ :
		// We use this exception to signal server to stop
		exitServerProcess();
		break;
	case odbcas_ASSvc_UpdateSrvrState_ASParamError_exn_ :
		break; // This exception is not raised by AS Now
//LCOV_EXCL_START
	case odbcas_ASSvc_UpdateSrvrState_ASStateChangeError_exn_:
		SendEventMsg(MSG_UPDATE_SRVR_STATE_FAILED, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			0);
		break;
	default:
		SendEventMsg(MSG_UPDATE_SRVR_STATE_FAILED, EVENTLOG_ERROR_TYPE,
			srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			0);
//LCOV_EXCL_STOP
	}
	if (exception_->exception_nr != 0)
	{
		if (srvrGlobal->srvrState == srvrState) // check if the current state is what we tried to update
		{
			if (srvrGlobal->srvrState != SRVR_CONNECTED)
			{
//LCOV_EXCL_START
				sprintf(errorMessage, "Exception=%ld CurrentSrvrState=%d SrvrState=%d", exception_->exception_nr, srvrGlobal->srvrState, srvrState);
				SendEventMsg(MSG_KRYPTON_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
					2, errorMessage, FORMAT_LAST_ERROR());
				exitServerProcess();
//LCOV_EXCL_STOP
			}
			else // trigger stopping when disconnected
			{
				CEE_HANDLE_SET_NIL(&(callIdStopServer));
				srvrGlobal->stopTypeFlag = STOP_WHEN_DISCONNECTED;
			}
		}
		else
		{
//LCOV_EXCL_START
			// late response from AS and hence it is better for the server to die
			sprintf(errorMessage, "Exception=%ld CurrentSrvrState=%d SrvrState=%d", exception_->exception_nr, srvrGlobal->srvrState, srvrState);
			SendEventMsg(MSG_KRYPTON_ERROR, EVENTLOG_ERROR_TYPE,
				srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
				2, errorMessage, FORMAT_LAST_ERROR());
			exitServerProcess();
//LCOV_EXCL_STOP
		}
	}
	else
	{
		switch (srvrState)
		{
		case SRVR_DISCONNECTED:
		case SRVR_CLIENT_DISAPPEARED:
		case SRVR_CONNECT_REJECTED:
		case SRVR_CONNECT_FAILED:
			UPDATE_SERVER_WAITED(TraceType, StatisticsType, ContextType, srvrContext);
			break;
		}
	}

	SRVRTRACE_EXIT(FILE_AME+15);
	return ;
}

void SRVR::exitServerProcess()
{
	SRVRTRACE_ENTER(FILE_AME+16);
	short	nskError;
	char	tmpString[32];

	SendEventMsg(MSG_SERVER_TRACE_INFO,
				      EVENTLOG_INFORMATION_TYPE,
				      srvrGlobal->nskASProcessInfo.processId,
				      ODBCMX_SERVICE,
				      srvrGlobal->srvrObjRef,
				      3,
				      srvrGlobal->sessionId,
				      "exitServerProcess()",
				      "0");

	terminateThreads(1);
	exit(1);
	SRVRTRACE_EXIT(FILE_AME+16);
}

/*
 * Asynchronous method function prototype for
 * operation 'odbc_SQLSvc_StopServer'
 */
extern "C" void
odbc_SQLSvc_StopServer_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_long StopType
  , /* In    */ IDL_string ReasonText
  )
{
	SRVRTRACE_ENTER(FILE_AME+17);
	odbc_SQLSvc_StopServer_exc_ StopException={0,0};

	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceStopServerEnter(dialogueId, StopType, ReasonText);
	}

	StopException.exception_nr = 0;
	if (srvrGlobal->srvrState == SRVR_STOPPING)
	{
		StopException.exception_nr = odbc_SQLSvc_StopServer_ProcessStopError_exn_;
		StopException.u.ProcessStopError.ErrorText = "Already Stopped";
		odbc_SQLSvc_StopServer_ts_res_(objtag_, call_id_, &StopException);
		if (srvrGlobal->traceLogger != NULL)
		{
			srvrGlobal->traceLogger->TraceStopServerExit(StopException);
		}
	}
	if (srvrGlobal->srvrState == SRVR_CONNECTED)
	{
		if (dialogueId == srvrGlobal->dialogueId)
		{
			//Susan , changed STOP to STOP_SRVR in Global.H
			if(StopType == STOP_SRVR)
			{
				if (WSQL_EXEC_Xact(SQLTRANS_STATUS,NULL) == 0)
					EXECDIRECT("ROLLBACK WORK");

				releaseCachedObject(FALSE, NDCS_STOP_SRVR);

				odbc_SQLSvc_StopServer_ts_res_(objtag_, call_id_, &StopException);
				if (srvrGlobal->traceLogger != NULL)
				{
					srvrGlobal->traceLogger->TraceStopServerExit(StopException);
				}

				exitServerProcess();
			}
			else
			{
				srvrGlobal->stopTypeFlag = STOP_WHEN_DISCONNECTED;
				memcpy(&(callIdStopServer), call_id_, sizeof(CEE_handle_def));
				// ASSOC server knows when server exists from filesystem.
				odbc_SQLSvc_StopServer_ts_res_(objtag_, call_id_, &StopException);
			}
		}
	}
	else
	{
		if (WSQL_EXEC_Xact(SQLTRANS_STATUS,NULL) == 0)
			EXECDIRECT("ROLLBACK WORK");
		releaseCachedObject(FALSE, NDCS_STOP_SRVR);

		odbc_SQLSvc_StopServer_ts_res_(objtag_, call_id_, &StopException);
		if (srvrGlobal->traceLogger != NULL)
		{
			srvrGlobal->traceLogger->TraceStopServerExit(StopException);
		}

		exitServerProcess();
	}
	SRVRTRACE_EXIT(FILE_AME+17);
}


/*
 * Asynchronous method implementation for
 * operation 'odbc_SQLSrvr_Close'
 */
extern "C" void
odbc_SQLSrvr_Close_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_unsigned_short freeResourceOpt
  )
{

	SRVRTRACE_ENTER(FILE_AME+18);
	odbc_SQLSvc_Close_exc_ exception_={0,0,0};
	IDL_long rowsAffected = -1;
	IDL_long	returnCode = SQL_SUCCESS;
	IDL_long	sqlWarningOrErrorLength = 0;
	BYTE		*sqlWarningOrError = NULL;
	RETCODE rc = SQL_SUCCESS;

	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceCloseEnter(dialogueId, stmtLabel, freeResourceOpt);
	}

	if (srvrGlobal != NULL && srvrGlobal->srvrType == CORE_SRVR)
	{
		if (srvrGlobal->srvrState == SRVR_CONNECTED)
		{
			if (dialogueId != srvrGlobal->dialogueId)
			{
//LCOV_EXCL_START
				returnCode = SQL_ERROR;
				GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Connection.", &sqlWarningOrErrorLength, sqlWarningOrError);
//LCOV_EXCL_STOP
			}
			else
			{
				odbc_SQLSrvr_Close_sme_( objtag_,
					                     call_id_,
										 dialogueId,
										 stmtLabel,
						                 freeResourceOpt,
										 &rowsAffected,
										 &returnCode,
										 &sqlWarningOrErrorLength,
						                 sqlWarningOrError);
			}
		}
		else
		{
		   returnCode = SQL_ERROR;
		   GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Connection.", &sqlWarningOrErrorLength, sqlWarningOrError);
		}
	}
	else
	{
		odbc_SQLSrvr_Close_sme_( objtag_,
			                     call_id_,
								 dialogueId,
								 stmtLabel,
						         freeResourceOpt,
								 &rowsAffected,
								 &returnCode,
								 &sqlWarningOrErrorLength,
						         sqlWarningOrError);
	}


				qrysrvc_ExecuteFinished(stmtLabel, NULL, false, returnCode, false);
				SRVR_STMT_HDL *pSrvrStmt = SRVR::getSrvrStmt(stmtLabel, FALSE);

				if (pSrvrStmt != NULL)
				{
					if ((resStatStatement != NULL) && (pSrvrStmt->stmtType == EXTERNAL_STMT))
							resStatStatement->endRepository(pSrvrStmt,
								sqlWarningOrErrorLength,
								sqlWarningOrError,
								true);
				}



	odbc_SQLSrvr_Close_ts_res_(objtag_,
		                       call_id_,
							   returnCode,
							   rowsAffected,
							   sqlWarningOrErrorLength,
						       sqlWarningOrError);

//LCOV_EXCL_START
	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceClose2Exit(returnCode,
			                                     rowsAffected,
												 sqlWarningOrErrorLength,
												 sqlWarningOrError);
	}
//LCOV_EXCL_STOP

	SRVRTRACE_EXIT(FILE_AME+18);

	return;

} /* odbc_SQLSrvr_Close_ame_() */


void SRVR::ENABLE_SERVER_TRACE(IDL_long TraceType)
{
	char tmpString[50];
	if(srvrGlobal->traceLogger == NULL) // check if trace is enabled or not
	{
		// trace class here
		srvrGlobal->traceLogger = new ODBCMXTraceMsg(srvrGlobal->nskProcessInfo.processId, srvrGlobal->srvrObjRef);
		if (srvrGlobal->traceLogger == NULL)
		{
//LCOV_EXCL_START
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
					srvrGlobal->srvrObjRef, 1, "srvrGlobal->traceLogger");
			exitServerProcess();
//LCOV_EXCL_STOP
		}
		srvrGlobal->traceLogger->OpenTraceCollector(srvrGlobal->TraceCollector);

		sprintf(tmpString, "Server Trace Enabled.");
		SendEventMsg(MSG_SERVER_TRACE_INFO,
						  EVENTLOG_INFORMATION_TYPE,
						  srvrGlobal->nskASProcessInfo.processId,
						  ODBCMX_SERVICE,
						  srvrGlobal->srvrObjRef,
						  4,
						  srvrGlobal->sessionId,
						  "EnableServerTrace",
						  "0", tmpString);
	}
}

/*
 * Asynchronous method function for
 * operation 'odbc_SQLSvc_EnableServerTrace'
 */
extern "C" void
odbc_SQLSvc_EnableServerTrace_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_long TraceType
  )
{
	SRVRTRACE_ENTER(FILE_AME+29);
	char tmpString[50];
	odbc_SQLSvc_EnableServerTrace_exc_ exception={0,0,0};

	if (srvrGlobal->srvrState == SRVR_STOPPING)
	{
		exception.exception_nr = odbc_SQLSvc_EnableServerTrace_TraceError_exn_;
	}
	else if ((srvrGlobal->srvrState == SRVR_CONNECTED) || (srvrGlobal->srvrState == SRVR_AVAILABLE))
	{
		if ((dialogueId == srvrGlobal->dialogueId) || (dialogueId == 0))
		{
			ENABLE_SERVER_TRACE(TraceType);
		}
		else
			exception.exception_nr = odbc_SQLSvc_EnableServerTrace_TraceError_exn_;
	}
	else
		exception.exception_nr = odbc_SQLSvc_EnableServerTrace_TraceError_exn_;

	if (exception.exception_nr != 0)
	{
//LCOV_EXCL_START
		sprintf(tmpString, "Server Trace Enable failed.");
		SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			1, tmpString);
//LCOV_EXCL_STOP
	}
	odbc_SQLSvc_EnableServerTrace_ts_res_(objtag_, call_id_, &exception);
	SRVRTRACE_EXIT(FILE_AME+29);
}

void SRVR::DISABLE_SERVER_TRACE(IDL_long TraceType)
{
	char tmpString[50];
	if(srvrGlobal->traceLogger != NULL) // check if trace is enabled or not
	{
		// trace class here
		delete srvrGlobal->traceLogger;
		srvrGlobal->traceLogger = NULL;

		sprintf(tmpString, "Server Trace Disabled.");
		SendEventMsg(MSG_SERVER_TRACE_INFO,
						  EVENTLOG_INFORMATION_TYPE,
						  srvrGlobal->nskASProcessInfo.processId,
						  ODBCMX_SERVICE,
						  srvrGlobal->srvrObjRef,
						  4,
						  srvrGlobal->sessionId,
						  "DisableServerTrace",
						  "0", tmpString);
	}
}

/*
 * Asynchronous method function for
 * operation 'odbc_SQLSvc_DisableServerTrace'
 */
extern "C" void
odbc_SQLSvc_DisableServerTrace_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_long TraceType
  )
{
	SRVRTRACE_ENTER(FILE_AME+30);
	char tmpString[50];
	odbc_SQLSvc_DisableServerTrace_exc_ exception={0,0,0};

	if (srvrGlobal->srvrState == SRVR_STOPPING)
	{
		exception.exception_nr = odbc_SQLSvc_DisableServerTrace_TraceError_exn_;
	}
	else if (srvrGlobal->srvrState == SRVR_AVAILABLE)
	{
		if ((dialogueId == srvrGlobal->dialogueId) || (dialogueId == 0))
			DISABLE_SERVER_TRACE(TraceType);
		else
			exception.exception_nr = odbc_SQLSvc_DisableServerTrace_TraceError_exn_;
	}
	else
		exception.exception_nr = odbc_SQLSvc_DisableServerTrace_TraceError_exn_;

	if (exception.exception_nr != 0)
	{
		sprintf(tmpString, "Server Trace Disable failed.");
		SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			1, tmpString);
	}
	odbc_SQLSvc_DisableServerTrace_ts_res_(objtag_, call_id_, &exception);
	SRVRTRACE_EXIT(FILE_AME+30);
}

void SRVR::ENABLE_STATISTICS(IDL_long StatisticsType)
{
	char tmpString[50];
	// Statistics class here
	if (srvrGlobal->resourceStatistics == 0 && StatisticsType > 0)
	{
#ifdef RES_STATS_EVENT
		stringstream ss;
		ss << "Server Statistics Enabled." << " (DSName=" << srvrGlobal->DSName << " resourceStatistics=0x" << hex << StatisticsType << ")";
		//sprintf(tmpString, "Server Statistics Enabled.");
		SendEventMsg(	MSG_RES_STAT_INFO,
									EVENTLOG_INFORMATION_TYPE,
									srvrGlobal->nskProcessInfo.processId,
									ODBCMX_SERVER,
									srvrGlobal->srvrObjRef,
									4,
									srvrGlobal->sessionId,
									"STATISTICS INFORMATION",
									"0",
									//tmpString);
									ss.str().c_str());
#endif
	}
	srvrGlobal->resourceStatistics = StatisticsType;
}
/*
 * Asynchronous method function for
 * operation 'odbc_SQLSvc_EnableStatistics'
 */
extern "C" void
odbc_SQLSvc_EnableServerStatistics_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_long StatisticsType
  )
{
	SRVRTRACE_ENTER(FILE_AME+31);
	char tmpString[50];
	odbc_SQLSvc_EnableServerStatistics_exc_ exception={0,0,0};

	if (srvrGlobal->srvrState == SRVR_STOPPING)
	{
		exception.exception_nr = odbc_SQLSvc_EnableServerStatistics_StatisticsError_exn_;
	}
	else if (srvrGlobal->srvrState == SRVR_AVAILABLE)
	{
		if ((dialogueId == srvrGlobal->dialogueId) || (dialogueId == 0))
			ENABLE_STATISTICS(StatisticsType);
		else
			exception.exception_nr = odbc_SQLSvc_EnableServerStatistics_StatisticsError_exn_;
	}
	else
		exception.exception_nr = odbc_SQLSvc_EnableServerStatistics_StatisticsError_exn_;

	if (exception.exception_nr != 0)
	{
		sprintf(tmpString, "Server Statistics Enable failed.");
		SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			1, tmpString);
	}
	odbc_SQLSvc_EnableServerStatistics_ts_res_(objtag_, call_id_, &exception);
	SRVRTRACE_EXIT(FILE_AME+31);
}

void SRVR::DISABLE_STATISTICS()
{
	char tmpString[50];
	if(srvrGlobal->resourceStatistics > 0) // check if Statistics is disabled or not
	{
#ifdef RES_STATS_EVENT
		stringstream ss;
		ss << "Server Statistics Disabled." << " (DSName=" << srvrGlobal->DSName << " resourceStatistics=0x" << hex << srvrGlobal->resourceStatistics << ")";
		// Statistics class here
		//sprintf(tmpString, "Server Statistics Disabled.");
		SendEventMsg(	MSG_RES_STAT_INFO,
										EVENTLOG_INFORMATION_TYPE,
										srvrGlobal->nskProcessInfo.processId,
										ODBCMX_SERVER,
										srvrGlobal->srvrObjRef,
										4,
										srvrGlobal->sessionId,
										"STATISTICS INFORMATION",
										"0",
										//tmpString);
										ss.str().c_str());
#endif
		srvrGlobal->resourceStatistics = 0;
	}
}

/*
 * Asynchronous method function for
 * operation 'odbc_SQLSvc_DisableServerStatistics'
 */
extern "C" void
odbc_SQLSvc_DisableServerStatistics_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  )
{
	SRVRTRACE_ENTER(FILE_AME+32);
	char tmpString[50];
	odbc_SQLSvc_DisableServerStatistics_exc_ exception={0,0,0};

	if (srvrGlobal->srvrState == SRVR_STOPPING)
	{
		exception.exception_nr = odbc_SQLSvc_DisableServerStatistics_StatisticsError_exn_;
	}
	else if (srvrGlobal->srvrState == SRVR_AVAILABLE)
	{
		if ((dialogueId == srvrGlobal->dialogueId) || (dialogueId == 0))
			DISABLE_STATISTICS();
		else
			exception.exception_nr = odbc_SQLSvc_DisableServerStatistics_StatisticsError_exn_;
	}
	else
		exception.exception_nr = odbc_SQLSvc_DisableServerStatistics_StatisticsError_exn_;

	if (exception.exception_nr != 0)
	{
		sprintf(tmpString, "Server Statistics Disable failed.");
		SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			1, tmpString);
	}
	odbc_SQLSvc_DisableServerStatistics_ts_res_(objtag_, call_id_, &exception);
	SRVRTRACE_EXIT(FILE_AME+32);
}

void SRVR::UPDATE_SERVER_CONTEXT(const SRVR_CONTEXT_def *srvrContext)
{
	SRVRTRACE_ENTER(FILE_AME+33);
	int i;
	CEE_status sts;

	RES_DESC_def *pResValuesIn;
	RES_DESC_def *pResValues;
	ENV_DESC_def *pEnvValuesIn;
	ENV_DESC_def *pEnvValues;

	srvrGlobal->srvrContext.srvrIdleTimeout = srvrContext->srvrIdleTimeout;
	srvrGlobal->srvrContext.connIdleTimeout = srvrContext->connIdleTimeout;
	if (srvrGlobal->srvrContext.srvrIdleTimeout == DEFAULT_SRVR_IDLE_TIMEOUT)
		srvrGlobal->srvrContext.srvrIdleTimeout = DEFAULT_SRVR_IDLE_TIMEOUT_MINS;
	if (srvrGlobal->srvrContext.connIdleTimeout == DEFAULT_CONN_IDLE_TIMEOUT)
		srvrGlobal->srvrContext.connIdleTimeout = DEFAULT_CONN_IDLE_TIMEOUT_MINS;

	if (srvrGlobal->srvrContext.resDescList._length > 0)
	{
		int len_length = srvrGlobal->srvrContext.resDescList._length;
		RES_DESC_def *p_buffer = (RES_DESC_def *)srvrGlobal->srvrContext.resDescList._buffer;

		for( i=0; i < len_length; i++)
		{
			pResValues = p_buffer + i;
			if (pResValues->Action != NULL)
				delete pResValues->Action;
		}
		delete srvrGlobal->srvrContext.resDescList._buffer;
	}
	srvrGlobal->resGovernOn = FALSE;
	srvrGlobal->srvrContext.resDescList._buffer = NULL;
	srvrGlobal->srvrContext.resDescList._length = 0;

	if (srvrGlobal->srvrContext.envDescList._length > 0)
	{
		int len_length = srvrGlobal->srvrContext.envDescList._length;
		ENV_DESC_def *p_buffer = (ENV_DESC_def *)srvrGlobal->srvrContext.envDescList._buffer;

		for( i=0; i < len_length; i++)
		{
			pEnvValues = p_buffer + i;
			if (pEnvValues->VarVal != NULL)
				delete pEnvValues->VarVal;
		}
		delete srvrGlobal->srvrContext.envDescList._buffer;
	}
	srvrGlobal->envVariableOn = FALSE;
	srvrGlobal->srvrContext.envDescList._buffer = NULL;
	srvrGlobal->srvrContext.envDescList._length = 0;

	// Copy the srvr Context to Global Structure
	if(srvrContext->resDescList._length > 0)
	{
		srvrGlobal->resGovernOn = TRUE;

		srvrGlobal->srvrContext.resDescList._length=srvrContext->resDescList._length;
		srvrGlobal->srvrContext.resDescList._buffer = new RES_DESC_def[srvrContext->resDescList._length];
		if (srvrGlobal->srvrContext.resDescList._buffer == NULL)
		{
//LCOV_EXCL_START
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
					srvrGlobal->srvrObjRef, 1, "resDescList._buffer");
			exitServerProcess();
//LCOV_EXCL_STOP
		}
		memcpy((void *)(srvrGlobal->srvrContext.resDescList._buffer),
			srvrContext->resDescList._buffer,
			(sizeof(RES_DESC_def)*srvrGlobal->srvrContext.resDescList._length));

		int len_length = srvrContext->resDescList._length;
		RES_DESC_def *p_Inbuffer = (RES_DESC_def *)srvrContext->resDescList._buffer;
		RES_DESC_def *p_buffer = (RES_DESC_def *)srvrGlobal->srvrContext.resDescList._buffer;

		for( i=0; i < len_length; i++)
		{
			pResValuesIn = p_Inbuffer + i;
			pResValues = p_buffer + i;
			pResValues->Action = new char[strlen(pResValuesIn->Action) + 1];
			if (pResValues->Action == NULL)
			{
//LCOV_EXCL_START
				SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
						srvrGlobal->srvrObjRef, 1, "Action");
				exitServerProcess();
//LCOV_EXCL_STOP
			}
			strcpy(pResValues->Action,pResValuesIn->Action);
		}
	}

	if(srvrContext->envDescList._length > 0)
	{

		srvrGlobal->envVariableOn = TRUE;

		srvrGlobal->srvrContext.envDescList._length=srvrContext->envDescList._length;
		srvrGlobal->srvrContext.envDescList._buffer = new ENV_DESC_def[srvrContext->envDescList._length];
		if (srvrGlobal->srvrContext.envDescList._buffer == NULL)
		{
//LCOV_EXCL_START
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
					srvrGlobal->srvrObjRef, 1, "envDescList._buffer");
			exitServerProcess();
//LCOV_EXCL_STOP
		}
		memcpy((void *)(srvrGlobal->srvrContext.envDescList._buffer),
			srvrContext->envDescList._buffer,
			(sizeof(ENV_DESC_def)*srvrGlobal->srvrContext.envDescList._length));

		int len_length = srvrContext->envDescList._length;
		ENV_DESC_def *p_Inbuffer = (ENV_DESC_def *)srvrContext->envDescList._buffer;
		ENV_DESC_def *p_buffer = srvrGlobal->srvrContext.envDescList._buffer;

		for( i=0; i < len_length; i++)
		{
			pEnvValuesIn = p_Inbuffer + i;
			pEnvValues = p_buffer + i;
			pEnvValues->VarVal = new char[strlen(pEnvValuesIn->VarVal) + 1];
			if (pEnvValues->VarVal == NULL)
			{
//LCOV_EXCL_START
				SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
						srvrGlobal->srvrObjRef, 1, "VarVal");
				exitServerProcess();
//LCOV_EXCL_STOP
			}
			strcpy(pEnvValues->VarVal,pEnvValuesIn->VarVal);
		}
	}
	SRVRTRACE_EXIT(FILE_AME+33);
}

/*
 * Asynchronous method function for
 * operation 'odbc_SQLSvc_UpdateServerContext'
 */
extern "C" void
odbc_SQLSvc_UpdateServerContext_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const SRVR_CONTEXT_def *srvrContext
  )
{
	SRVRTRACE_ENTER(FILE_AME+34);
	CEE_status sts;
	char tmpString[100];
	odbc_SQLSvc_UpdateServerContext_exc_ exception={0,0,0};

	if (srvrGlobal->srvrState == SRVR_STOPPING || srvrGlobal->stopTypeFlag == STOP_WHEN_DISCONNECTED)
	{
		exception.exception_nr = odbc_SQLSvc_UpdateServerContext_SQLError_exn_;
	}
	else if (srvrGlobal->srvrState == SRVR_AVAILABLE)
	{
		UPDATE_SERVER_CONTEXT(srvrContext);
	}

	else
		exception.exception_nr = odbc_SQLSvc_UpdateServerContext_SQLError_exn_;

	if (exception.exception_nr != 0)
	{
//LCOV_EXCL_START
		sprintf(tmpString, "Update Server Context failed due to server state not Available.");
		SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, srvrGlobal->srvrObjRef, 1, tmpString);
//LCOV_EXCL_STOP
	}
	odbc_SQLSvc_UpdateServerContext_ts_res_(objtag_, call_id_, &exception);
	SRVRTRACE_EXIT(FILE_AME+34);
}

//LCOV_EXCL_START

void SQL_EXECDIRECT(SRVR_INIT_PARAM_Def* initParam)
{
	char			RequestError[200];
	SQLRETURN		rc = SQL_SUCCESS;
	SRVR_STMT_HDL	*CleanupStmt = NULL;

	RequestError[0] = '\0';

	srvrGlobal = new SRVR_GLOBAL_Def;
	if (srvrGlobal == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
				0, ODBCMX_SERVER, "N/A", 1, "srvrGlobal");
		return;
	}

	initSqlCore();

	if ((CleanupStmt = getSrvrStmt("STMT_CLEANUP_ON_1", TRUE)) == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
				0, ODBCMX_SERVER, "N/A", 1, SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT);
	}
	else
	{
		rc = CleanupStmt->ExecDirect(NULL, "SET TRANSACTION AUTOCOMMIT ON", INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
		if (rc == SQL_ERROR)
		{
			ERROR_DESC_def *p_buffer = CleanupStmt->sqlError.errorList._buffer;
			strncpy(RequestError, p_buffer->errorText,200);
			RequestError[199] = '\0';
			SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
					0, ODBCMX_SERVER, "N/A", 1, RequestError);
		}
		else
		{
			CleanupStmt->ExecDirect(NULL, initParam->sql, INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
			if (rc == SQL_ERROR)
			{
				ERROR_DESC_def *p_buffer = CleanupStmt->sqlError.errorList._buffer;
				strncpy(RequestError, p_buffer->errorText,200);
				RequestError[199] = '\0';
				SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
						0, ODBCMX_SERVER, "N/A", 1, RequestError);
			}
			else
			{
				sprintf(RequestError, "Executed %s command successfully", initParam->sql);
				SendEventMsg(MSG_SRVR_ENV, EVENTLOG_INFORMATION_TYPE,
								0, ODBCMX_SERVER, "N/A", 1,	RequestError);
			}
		}
		CleanupStmt->cleanupAll();
		CleanupStmt->currentMethod = odbc_SQLSvc_Close_ldx_;
		CleanupStmt->freeResourceOpt = SQL_DROP;
		FREESTATEMENT(CleanupStmt);
	}
	return;
}

short SQL_EXECDIRECT_FETCH(SRVR_INIT_PARAM_Def* initParam)
{
	SRVR_STMT_HDL	*pSrvrStmt;
	SQLRETURN		rc = SQL_SUCCESS;
	short			SQLDataInd=0;
	short			SQLDataValueLen;
	unsigned long	Index=0;
	long			cnt = -1;
	char			RequestError[200];

	RequestError[0] = '\0';

	srvrGlobal = new SRVR_GLOBAL_Def;
	if (srvrGlobal == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
				0, ODBCMX_SERVER, "N/A", 1, "srvrGlobal");
		return -1;
	}

	srvrGlobal->srvrType = initParam->srvrType;
	initSqlCore();

	if ((pSrvrStmt = getSrvrStmt("STMT_SYSCATCNT_ON_1", TRUE)) == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, "N/A", 1, SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT);
	}
	else
	{
		rc = pSrvrStmt->ExecDirect(NULL, initParam->sql, EXTERNAL_STMT, TYPE_SELECT, SQL_ASYNC_ENABLE_OFF, 0);
		if (rc == SQL_ERROR)
		{
			ERROR_DESC_def *p_buffer = pSrvrStmt->sqlError.errorList._buffer;
			strncpy(RequestError, p_buffer->errorText,200);
			RequestError[199] = '\0';
			SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
					0, ODBCMX_SERVER, "N/A", 1, RequestError);
		}
		else
		{
			rc = pSrvrStmt->FetchPerf(100, 0, SQL_ASYNC_ENABLE_OFF, 0);
			if (rc == SQL_ERROR)
			{
				ERROR_DESC_def *p_buffer = pSrvrStmt->sqlError.errorList._buffer;
				strncpy(RequestError, p_buffer->errorText,200);
				RequestError[199] = '\0';
				SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
						0, ODBCMX_SERVER, "N/A", 1, RequestError);
			}
			else
			{
				if (rc != SQL_NO_DATA_FOUND)
				{
					Index = 0;
					while (Index < pSrvrStmt->outputDataValue._length - 1)
					{
						SQLDataInd = (short)*(unsigned char*)(pSrvrStmt->outputDataValue._buffer + Index);
						Index = Index + 1;
						if (SQLDataInd == 0)
						{
							cnt = *(long*)(pSrvrStmt->outputDataValue._buffer + Index);
							Index = Index + sizeof(SQLDataValueLen);
							Index = Index + 1;
						}
					}
				}
				else
				{
					sprintf(RequestError, "Get registered system catalog count failed, no data found.");
					SendEventMsg(MSG_SRVR_ENV, EVENTLOG_INFORMATION_TYPE,
						0, ODBCMX_SERVER, "N/A", 1,	RequestError);
				}
			}
		}
		pSrvrStmt->cleanupAll();
		pSrvrStmt->currentMethod = odbc_SQLSvc_Close_ldx_;
		pSrvrStmt->freeResourceOpt = SQL_DROP;
		FREESTATEMENT(pSrvrStmt);
	}
	return (short)cnt;
}
//LCOV_EXCL_STOP


//LCOV_EXCL_START

/*
 * Asynchronous method function for
 * operation 'odbcas_ASSvc_SrvrMonitorCall'
*/
#define odbc_SQLSvc_MonitorCall_InvalidConnection_exn_ 2

extern "C" void
odbc_SQLSvc_MonitorCall_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  )
{
	SRVRTRACE_ENTER(FILE_AME+36);
	odbc_SQLSvc_MonitorCall_exc_ exception_;
	if (srvrGlobal != NULL && srvrGlobal->srvrType == CORE_SRVR)
	{
		if (srvrGlobal->srvrState == SRVR_CONNECTED)
		{
			if (dialogueId != srvrGlobal->dialogueId)
				exception_.exception_nr = odbc_SQLSvc_MonitorCall_InvalidConnection_exn_;
			else
				odbc_SQLSvc_MonitorCall_sme_(objtag_, call_id_, &exception_, dialogueId);
		}
		else
			exception_.exception_nr = odbc_SQLSvc_MonitorCall_InvalidConnection_exn_;
	}
	else
		exception_.exception_nr = odbc_SQLSvc_MonitorCall_InvalidConnection_exn_;

	odbc_SQLSvc_MonitorCall_ts_res_(objtag_, call_id_, &exception_);
	SRVRTRACE_EXIT(FILE_AME+36);
}

extern "C" void
odbc_SQLSvc_MonitorCall_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLSvc_MonitorCall_exc_ *exception_
  , /* In    */ DIALOGUE_ID_def dialogueId
  )
{
}

// Function to get specific information from SQL
// The supported options right now are:
// 1 - Explain plan for a query. Also writes it to a QS config table.
// 2 - Gets the mode value set in the system defaults table.
//     Returns TRUE if mode is set FALSE otherwise
bool getSQLInfo(E_GetSQLInfoType option, long stmtHandle, char *stmtLabel )
{
#ifdef PERF_TEST
	perf->clockIt("getSQLInfo_START", true);
#endif

	SRVR_STMT_HDL	*pSrvrStmt = NULL;
	char			sqlQuery[300];
	SQLRETURN		iqqcode = SQL_SUCCESS;
	SRVR_STMT_HDL	*QrySrvrStmt = NULL;
	long			i = 0;
	long			j = 0;
	long			Index = 0;
	long			sqlStrLen = 0;
	short			SQLDataInd=0;
	short			SQLDataValueLen;
	unsigned long	TotalSQLDataValueLen=0;
	bool			returnVal = false, freeMem = true;
	char			*QueryOutput = NULL;
	int 			explainDataLen = 50000; // start with 50K bytes
	int 			retExplainLen = 0;
	char 			*explainData = NULL;

	if ((QrySrvrStmt = getSrvrStmt("STMT_QRYSTS_ON_1", TRUE)) == NULL)
		return false;

	sqlQuery[0] = '\0';

	bool testNewExplain = false;
	if(option == EXPLAIN_PLAN && testNewExplain)
	{
		if (stmtHandle != NULL)
			pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;
		else if (stmtLabel != NULL)
			pSrvrStmt = getSrvrStmt(stmtLabel, FALSE);

		if (pSrvrStmt == NULL ||
			pSrvrStmt->sqlUniqueQueryID == NULL ||
			pSrvrStmt->sqlUniqueQueryID[0] == '\0' )
			return false;

		// If the current WMS service context does not need plan then
		// don't collect it.
		if (FALSE == srvrGlobal->sqlPlan)
			return true;

		if (pSrvrStmt->exPlan == SRVR_STMT_HDL::COLLECTED)
			return true;
		if (QrySrvrStmt->sqlString != NULL)
		{
			delete QrySrvrStmt->sqlString;
			QrySrvrStmt->sqlString  = new char[256];
		}
		if (QrySrvrStmt->sqlString == NULL)
			return false;

		sprintf(QrySrvrStmt->sqlString, "EXPLAIN %s", pSrvrStmt->stmtName);
		QrySrvrStmt->sqlStringLen = strlen(QrySrvrStmt->sqlString);

		QrySrvrStmt->sqlStmtType = (short)TYPE_SELECT;
		QrySrvrStmt->maxRowsetSize = 1;
		QrySrvrStmt->inputRowCnt = 1;

		QrySrvrStmt->currentMethod = odbc_SQLSvc_PrepareRowset_ldx_;
		iqqcode = PREPARE2(QrySrvrStmt);
		if (iqqcode != SQL_ERROR)
		{
			QrySrvrStmt->cursorNameLen = 0;
			QrySrvrStmt->cursorName[0] = '\0';

			pSrvrStmt->currentMethod = odbc_SQLSvc_ExecuteN_ldx_;
			iqqcode = EXECUTE2(QrySrvrStmt);
			if (iqqcode != SQL_ERROR)
			{
				QrySrvrStmt->maxRowCnt = srvrGlobal->m_FetchBufferSize/QrySrvrStmt->outputDescVarBufferLen;
				QrySrvrStmt->maxRowLen = QrySrvrStmt->outputDescVarBufferLen;
				if (QrySrvrStmt->outputDataValue._length > 0)
				{
					if (QrySrvrStmt->outputDataValue._buffer != NULL)
						delete QrySrvrStmt->outputDataValue._buffer;
					QrySrvrStmt->outputDataValue._buffer = NULL;
					QrySrvrStmt->outputDataValue._length = 0;
				}
				QrySrvrStmt->currentMethod = odbc_SQLSvc_FetchPerf_ldx_;
				iqqcode = FETCH2bulk(QrySrvrStmt);
				if (iqqcode != SQL_ERROR)
				{
					returnVal = true;
					freeMem = false;
					pSrvrStmt->exPlan = SRVR_STMT_HDL::COLLECTED;
				}
			}
		}
		if (QrySrvrStmt->sqlWarningOrErrorLength > 0)
		{
			if (QrySrvrStmt->sqlWarningOrError != NULL)
				delete QrySrvrStmt->sqlWarningOrError;
			QrySrvrStmt->sqlWarningOrErrorLength = 0;
			QrySrvrStmt->sqlWarningOrError = NULL;
		}
	}
	else
	{
		switch( option )
		{
			case EXPLAIN_PLAN: // Explain
				if (FALSE == srvrGlobal->sqlPlan)
					return true;

				if (stmtHandle != NULL)
					pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;
				else if (stmtLabel != NULL)
					pSrvrStmt = getSrvrStmt(stmtLabel, FALSE);

				if (pSrvrStmt == NULL ||
					pSrvrStmt->sqlUniqueQueryID == NULL ||
					pSrvrStmt->sqlUniqueQueryID[0] == '\0' )
					return false;

				if (pSrvrStmt->exPlan == SRVR_STMT_HDL::COLLECTED)
					return true;

				// Ignore plan collection of unique queries and ones with no stats for performance reasons
				if (pSrvrStmt->sqlNewQueryType == SQL_SELECT_UNIQUE ||
					pSrvrStmt->sqlNewQueryType == SQL_INSERT_UNIQUE ||
					pSrvrStmt->sqlNewQueryType == SQL_UPDATE_UNIQUE ||
					pSrvrStmt->sqlNewQueryType == SQL_DELETE_UNIQUE ||
					(pSrvrStmt->comp_stats_info.statsCollectionType == SQLCLI_NO_STATS && pSrvrStmt->comp_stats_info.compilationStats.compilerId[0] != 0))
					return true;
					
				// allocate explainDataLen bytes of explainData space
				explainData = new char[explainDataLen + 1];
				if (explainData == NULL)
				{
					char errStr[128];
					sprintf( errStr, "Packed explain for %d bytes", explainDataLen );
					SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
							srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
							srvrGlobal->srvrObjRef, 1, errStr);
					return false;
				}
				iqqcode = SQL_EXEC_GetExplainData(&(pSrvrStmt->stmt),
													explainData,
													explainDataLen + 1,
													&retExplainLen);
				if (iqqcode == -CLI_GENCODE_BUFFER_TOO_SMALL)
				{
					explainDataLen = retExplainLen;

					// Clear diagnostics
					SRVR::WSQL_EXEC_ClearDiagnostics(NULL);

					// allocate explainDataLen bytes of explainData space
					if (explainData)
						delete explainData;
					explainData = new char[explainDataLen + 1];
					if (explainData == NULL)
					{
						char errStr[128];
						sprintf( errStr, "Packed explain for %d bytes", explainDataLen );
						SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
								srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
								srvrGlobal->srvrObjRef, 1, errStr);
						return false;
					}

					iqqcode = SQL_EXEC_GetExplainData(&(pSrvrStmt->stmt),
												explainData,
												explainDataLen + 1,
												&retExplainLen);
				}
				else if (iqqcode == -EXE_NO_EXPLAIN_INFO)
				{
					retExplainLen = 0;
					if (explainData)
						delete explainData;
					explainData = 0;
					iqqcode = 0;

					// Clear diagnostics
					SRVR::WSQL_EXEC_ClearDiagnostics(NULL);
				}

				if (iqqcode < 0)
				{
					char errStr[256];
					sprintf( errStr, "Error retrieving packed explain. SQL_EXEC_GetExplainData() returned: %d", iqqcode );
					SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
							srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
							srvrGlobal->srvrObjRef, 1, errStr);
					delete explainData;
					return false;
				}
				if (pSrvrStmt->sqlPlan != NULL)
				{
					delete pSrvrStmt->sqlPlan;
					pSrvrStmt->sqlPlan = NULL;
				}

				pSrvrStmt->sqlPlan = explainData;
				if (retExplainLen > 0)
					pSrvrStmt->sqlPlanLen = retExplainLen + 1; // include null terminator
				pSrvrStmt->exPlan = SRVR_STMT_HDL::COLLECTED;
				return true;
			break;

			case MODE_SPECIAL_1:
				sprintf(sqlQuery,"control query default showcontrol_show_all 'ON'");
				iqqcode = QrySrvrStmt->ExecDirect(NULL, sqlQuery, INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
				if (iqqcode != SQL_SUCCESS)
					return false;
				sprintf(sqlQuery,"showcontrol default mode_special_1, match full, no header");
				sqlStrLen = 0;
			break;

			case NESTED_JOINS:
				sprintf(sqlQuery,"control query default showcontrol_show_all 'ON'");
				iqqcode = QrySrvrStmt->ExecDirect(NULL, sqlQuery, INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
				if (iqqcode != SQL_SUCCESS)
					return false;
				sprintf(sqlQuery,"showcontrol default NESTED_JOINS, match full, no header");
				sqlStrLen = 0;
			break;

			case USER_ROLE:
				sprintf(sqlQuery,"values(current_role)");
				sqlStrLen = 0;
				break;

			case SCHEMA_DEFAULT:
				sprintf(sqlQuery,"SHOWCONTROL DEFAULT SCHEMA, match full, no header");
				sqlStrLen = 0;
				break;

			case DEFAULT_SCHEMA_ACCESS_ONLY:
				sprintf(sqlQuery,"control query default showcontrol_show_all 'ON'");
				iqqcode = QrySrvrStmt->ExecDirect(NULL, sqlQuery, INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
				if (iqqcode != SQL_SUCCESS)
					return false;
				sprintf(sqlQuery,"showcontrol default DEFAULT_SCHEMA_ACCESS_ONLY, match full, no header");
				sqlStrLen = 0;
				break;
			default:
				return false;
		}

		iqqcode = QrySrvrStmt->ExecDirect(NULL, sqlQuery, EXTERNAL_STMT, TYPE_SELECT, SQL_ASYNC_ENABLE_OFF, 0);
		if (iqqcode != SQL_ERROR)
		{
			iqqcode = QrySrvrStmt->FetchPerf(100, 0, SQL_ASYNC_ENABLE_OFF, 0);
			if (iqqcode != SQL_NO_DATA_FOUND && iqqcode != SQL_ERROR)
			{
				unsigned long	QueryLen;
				QueryLen = QrySrvrStmt->outputDataValue._length + 512;
				QueryOutput = new char[QueryLen];

				if( !QueryOutput ) {
					returnVal = false;
					goto Handle_Return;
				}

				QueryOutput[0] = '\0';
				Index = 0;

				while (Index < QrySrvrStmt->outputDataValue._length - 1)
				{
					SQLDataInd = (short)*(unsigned char*)(QrySrvrStmt->outputDataValue._buffer + Index);
					Index = Index + 1;
					if (SQLDataInd == 0)
					{
						SQLDataValueLen = 0;
						SQLDataValueLen = *(short*)(QrySrvrStmt->outputDataValue._buffer + Index);
						Index = Index + sizeof(SQLDataValueLen);
						if( TotalSQLDataValueLen+SQLDataValueLen+1 > QueryLen )
							break;
					strncat(QueryOutput+TotalSQLDataValueLen, (const char *)QrySrvrStmt->outputDataValue._buffer + Index, SQLDataValueLen);
						Index = Index + SQLDataValueLen + 1;
						TotalSQLDataValueLen = TotalSQLDataValueLen + SQLDataValueLen;
						strncat(QueryOutput+TotalSQLDataValueLen, "\n", 1);
						TotalSQLDataValueLen = TotalSQLDataValueLen + 1;
					}
					else
						break;
				}

				if( SQLDataInd == 0 )
				{
					QueryOutput[TotalSQLDataValueLen] = '\0';
					if( option == MODE_SPECIAL_1
						&& strnicmp( QueryOutput, "ON", 2 ) == 0 )
					{
						returnVal = true;
					}
					else if( option == NESTED_JOINS
						&& strnicmp( QueryOutput, "ON", 2 ) == 0 )
					{
						returnVal = true;
					}
					else if ( option == USER_ROLE ) {
//LCOV_EXCL_START

						char *rolename;

						bzero(srvrGlobal->QSRoleName, sizeof(srvrGlobal->QSRoleName));
						bzero(srvrGlobal->RoleName, sizeof(srvrGlobal->RoleName));
						QueryOutput[SQLDataValueLen] = '\0';
						strcpy(srvrGlobal->QSRoleName, QueryOutput);

						// Output is always in the form <max 8 chars>.<max 8 chars>
						rolename = (char *)memchr(QueryOutput, '.', 8 + 1);

						// Extract the role name if output starts with "ROLE"
						if (rolename != NULL && (memcmp(QueryOutput, ROLE_PREFIX, ROLE_PREFIX_LEN) == 0))
						{
							strcpy(srvrGlobal->RoleName, rolename + 1);
						}
						else
						{
							strcpy(srvrGlobal->RoleName, QueryOutput);
						}
// SQ TBD: Roles are
                        strcpy(srvrGlobal->QSRoleName,"SUPER.SERVICES");
                        strcpy(srvrGlobal->RoleName,"SUPER.SERVICES");
						returnVal = true;
//LCOV_EXCL_STOP
					}
					else if ( option == SCHEMA_DEFAULT ) {
						if (SQLDataValueLen > 0)
						{
							QueryOutput[SQLDataValueLen] = '\0';
							strncpy(savedDefaultSchema,QueryOutput,sizeof(savedDefaultSchema));
							savedDefaultSchema[sizeof(savedDefaultSchema) -1] = '\0';
							returnVal = true;
						}
						else // Default Schema is empty
						{
							strcpy(savedDefaultSchema,ODBCMX_DEFAULT_SCHEMA);
							SendEventMsg(MSG_SQL_ERROR, EVENTLOG_ERROR_TYPE,
								srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
								3, ODBCMX_SERVER, "HY000", "Default Schema is empty.");
						}
					}
					else if( option == DEFAULT_SCHEMA_ACCESS_ONLY ){
						if( strnicmp( QueryOutput, "ON", 2 ) == 0 )
							srvrGlobal->defaultSchemaAccessOnly = true;
						else
							srvrGlobal->defaultSchemaAccessOnly = false;
						returnVal = true;
					}
				}
			}
			else // FetchPerf returned error
			{
//LCOV_EXCL_START
				if(iqqcode == SQL_ERROR)
				{
					GETSQLERROR(QrySrvrStmt->bSQLMessageSet, &QrySrvrStmt->sqlError);
					ERROR_DESC_def *sqlError = QrySrvrStmt->sqlError.errorList._buffer;
					SendEventMsg(MSG_SQL_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
						3, ODBCMX_SERVER, sqlError->sqlstate, sqlError->errorText);
				}
//LCOV_EXCL_STOP
			}
		}
		else // ExecDirect returned error
		{
//LCOV_EXCL_START
                        char errorBuf[512];
                        int numSQLErrors = QrySrvrStmt->sqlError.errorList._length;
			ERROR_DESC_def *sqlError = QrySrvrStmt->sqlError.errorList._buffer;

                        if(numSQLErrors > 0)
                        {
                           if(sqlError->errorText)
                              snprintf(errorBuf,sizeof(errorBuf),"Error executing %s : sqlcode=%d,sqlerror=%s",QrySrvrStmt->sqlString,sqlError->sqlcode,sqlError->errorText);
                           else
                              snprintf(errorBuf,sizeof(errorBuf),"Error executing %s : sqlcode=%d",QrySrvrStmt->sqlString,sqlError->sqlcode);
                        }
                        else
                              snprintf(errorBuf,sizeof(errorBuf),"Error executing %s, no SQL diagnostics available ",QrySrvrStmt->sqlString);

			SendEventMsg(MSG_SQL_ERROR, EVENTLOG_ERROR_TYPE,
				srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
				3, ODBCMX_SERVER, sqlError->sqlstate,errorBuf);
//LCOV_EXCL_STOP
		}
	}
Handle_Return:

	if (option == MODE_SPECIAL_1 ||
	    option == NESTED_JOINS ||
	    option == DEFAULT_SCHEMA_ACCESS_ONLY)
	{
		sprintf(sqlQuery,"control query default showcontrol_show_all 'OFF'");
		QrySrvrStmt->ExecDirect(NULL, sqlQuery, INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
	}

	if (QueryOutput != NULL && freeMem) {
		delete [] QueryOutput;
		QueryOutput = NULL;
	}

#ifdef PERF_TEST
	perf->clockIt("getSQLInfo_END", true);
#endif

	return returnVal;
}


bool ChkWSvcCommands(char* wsname, int& retcode, long type)
{
	SRVR_STMT_HDL	*QryControlSrvrStmt;
	char			ControlQuery[200];
	SQLRETURN		iqqcode = SQL_SUCCESS;
	retcode = -1;

	short			length;
	char			buffer[10];
	char* service_id = wsname;
	unsigned long	Index=0;
	short			SQLDataInd=0;
	short			SQLDataValueLen;
	unsigned long	ConfigQueryLen;
	unsigned long	TotalSQLDataValueLen=0;


	if ((QryControlSrvrStmt = getSrvrStmt("STMT_QRYSTS_ON_1", FALSE)) != NULL)
		QryControlSrvrStmt->Close(SQL_DROP);

	if ((QryControlSrvrStmt = getSrvrStmt("STMT_QRYSTS_ON_1", TRUE)) == NULL)
		return false;

	ControlQuery[0] = '\0';
	switch (type)
	{
	case CHECK_SERVICE:
		sprintf(ControlQuery,"select service_id from NEO.NWMS_SCHEMA.SERVICES where service_name = \'%s\' for read uncommitted access", wsname);
		break;
	case CHECK_SERVICEMAX:
		sprintf(ControlQuery,"select MAX(service_id) from NEO.NWMS_SCHEMA.SERVICES for read uncommitted access");
		break;
	case CHECK_SERVICEPRTY:
		sprintf(ControlQuery,"select service_priority from NEO.NWMS_SCHEMA.SERVICES  where service_name = \'%s\' for read uncommitted access", wsname);
		break;
//	case CHECK_MAXQUERIES_TOTAL:
//		sprintf(ControlQuery,"select cast(sum(cast(limit_value as integer)) as integer) from NEO.NWMS_SCHEMA.THRESHOLDS where threshold_type in (0,1) for read uncommitted access");
//		break;
	case CHECK_MAXQUERIES_OTHERS:
		sprintf(ControlQuery,"select cast(sum(cast(limit_value as integer)) as integer) from NEO.NWMS_SCHEMA.THRESHOLDS where threshold_type in (0,1) and service_id <> %s for read uncommitted access", service_id);
		break;
	case CHECK_QUERIES_WAITING:
		sprintf(ControlQuery,"select limit_value from NEO.NWMS_SCHEMA.THRESHOLDS where threshold_type = 1 and service_id = %s for read uncommitted access", service_id);
		break;
	case CHECK_QUERIES_EXECUTING:
		sprintf(ControlQuery,"select limit_value from NEO.NWMS_SCHEMA.THRESHOLDS where threshold_type = 0 and service_id = %s for read uncommitted access", service_id);
		break;
	default:
		return false;
	}

	iqqcode = QryControlSrvrStmt->ExecDirect(NULL, ControlQuery, EXTERNAL_STMT, TYPE_SELECT, SQL_ASYNC_ENABLE_OFF, 0);
	if (iqqcode != SQL_SUCCESS)
	{
		return false;
	}
	else
	{
		iqqcode = QryControlSrvrStmt->FetchPerf(100, 0, SQL_ASYNC_ENABLE_OFF, 0);

		if (iqqcode == SQL_ERROR)
		{
			return false;
		} else if (iqqcode == SQL_NO_DATA_FOUND)
		{
			retcode = -1;
			return true;
		} else
		{
			switch (type)
			{
			case CHECK_SERVICE:
				Index = 0;
				if (Index < QryControlSrvrStmt->outputDataValue._length - 1)
				{
					ControlQuery[0] = '\0';
					SQLDataInd = (short)*(unsigned char*)(QryControlSrvrStmt->outputDataValue._buffer + Index);
					Index = Index + 1;
					if (SQLDataInd == 0)
					{
						retcode = *(long*)(QryControlSrvrStmt->outputDataValue._buffer + Index);
						Index = Index + sizeof(SQLDataValueLen);
						Index = Index + 1;
					}
				}
				return true;
			case CHECK_SERVICEMAX:
				Index = 0;
				if (Index < QryControlSrvrStmt->outputDataValue._length - 1)
				{
					ControlQuery[0] = '\0';
					SQLDataInd = (short)*(unsigned char*)(QryControlSrvrStmt->outputDataValue._buffer + Index);
					Index = Index + 1;
					if (SQLDataInd == 0)
					{
						retcode = *(long*)(QryControlSrvrStmt->outputDataValue._buffer + Index);
						Index = Index + sizeof(SQLDataValueLen);
						Index = Index + 1;
					}
				}
				return true;
			case CHECK_SERVICEPRTY:
				Index = 0;
				if (Index < QryControlSrvrStmt->outputDataValue._length - 1)
				{
					ControlQuery[0] = '\0';
					SQLDataInd = (short)*(unsigned char*)(QryControlSrvrStmt->outputDataValue._buffer + Index);
					Index = Index + 1;
					if (SQLDataInd == 0)
					{
						retcode = (long)*(short*)(QryControlSrvrStmt->outputDataValue._buffer + Index);
						Index = Index + sizeof(SQLDataValueLen);
						Index = Index + 1;
					}
				}
				return true;

			case CHECK_MAXQUERIES_OTHERS:
				Index = 0;
				if (Index < QryControlSrvrStmt->outputDataValue._length - 1)
				{
					ControlQuery[0] = '\0';
					SQLDataInd = (short)*(unsigned char*)(QryControlSrvrStmt->outputDataValue._buffer + Index);
					Index = Index + 1;
					if (SQLDataInd == 0)
					{
						retcode = *(long*)(QryControlSrvrStmt->outputDataValue._buffer + Index);
						Index = Index + sizeof(SQLDataValueLen);
						Index = Index + 1;
					}
				}
				return true;
			case CHECK_QUERIES_WAITING:
			case CHECK_QUERIES_EXECUTING:
				Index = 0;
				if (Index < QryControlSrvrStmt->outputDataValue._length - 1)
				{
					ControlQuery[0] = '\0';
					SQLDataInd = (short)*(unsigned char*)(QryControlSrvrStmt->outputDataValue._buffer + Index);
					Index = Index + 1;
					if (SQLDataInd == 0)
					{
						memcpy(&length, (unsigned char*)(QryControlSrvrStmt->outputDataValue._buffer + Index), 2);
						Index += 2;
						memcpy(buffer, (unsigned char*)(QryControlSrvrStmt->outputDataValue._buffer + Index), length);
						buffer[Index] = 0;
						retcode = atol(buffer);
					}
				}
				return true;
			default:
				return false;
			}
		}
	}
}




/*
 *  New wire protocol method for Prepare
 */
extern "C" void
odbc_SQLSrvr_Prepare_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_long sqlAsyncEnable
  , /* In    */ IDL_long queryTimeout
  , /* In    */ IDL_short stmtType
  , /* In    */ IDL_long sqlStmtType
  , /* In    */ IDL_long stmtLength
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_long stmtLabelCharset
  , /* In    */ IDL_long cursorLength
  , /* In    */ IDL_string cursorName
  , /* In    */ IDL_long cursorCharset
  , /* In    */ IDL_long moduleNameLength
  , /* In    */ const IDL_char *moduleName
  , /* In    */ IDL_long moduleCharset
  , /* In    */ IDL_long_long moduleTimestamp
  , /* In    */ IDL_long sqlStringLength
  , /* In    */ IDL_string sqlString
  , /* In    */ IDL_long sqlStringCharset
  , /* In    */ IDL_long setStmtOptionsLength
  , /* In    */ IDL_string setStmtOptions
  , /* In    */ IDL_long stmtExplainLabelLength
  , /* In    */ IDL_string stmtExplainLabel
  , /* In    */ IDL_long maxRowsetSize
  , /* In    */ IDL_long_long txnID       // T4 driver sends a transaction ID which we need to join
  , /* In    */ IDL_short *extTransId     // T4 driver sends a transaction ID which we need to join
  , /* In    */ IDL_long holdableCursor
  )
{
#ifdef PERF_TEST
	perf->init();
	perf->clockIt("SQLSrvr_Prepare_ame_START", true);
#endif

	SRVRTRACE_ENTER(FILE_AME+19);
	IDL_long	returnCode = SQL_SUCCESS;
	IDL_long	sqlWarningOrErrorLength = 0;
	BYTE		*sqlWarningOrError = NULL;
	IDL_long	sqlQueryType = 0;
	Long stmtHandle = 0;
	IDL_long	estimatedCost = 0;
	IDL_long	inputParamsLength = 0;
	IDL_long	inputDescLength = 0;
	BYTE		*inputDesc = NULL;
	IDL_long	outputColumnsLength = 0;
	IDL_long	outputDescLength = 0;
	BYTE		*outputDesc = NULL;
	RETCODE		rc = 0;
	char errorBuffer[512];            // a buffer for formatting error messages

	IDL_boolean bPrepareWithRowsets = IDL_FALSE;

	if (maxRowsetSize > 1)
	{
		bPrepareWithRowsets = IDL_TRUE;
	}

	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TracePrepare2Enter(dialogueId, sqlAsyncEnable, queryTimeout, maxRowsetSize,
			sqlStmtType, stmtLength, stmtLabel, stmtLabelCharset, cursorLength, cursorName,
			cursorCharset, moduleNameLength, moduleName, moduleCharset, moduleTimestamp, sqlStringLength,
			sqlString, sqlStringCharset, setStmtOptionsLength, setStmtOptions, txnID, holdableCursor);
	}

	if (srvrGlobal != NULL && srvrGlobal->srvrType == CORE_SRVR)
	{
		if (srvrGlobal->srvrState == SRVR_CONNECTED)
		{
			if (dialogueId != srvrGlobal->dialogueId)
			{
//LCOV_EXCL_START
				returnCode = SQL_ERROR;
				GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Connection.", &sqlWarningOrErrorLength, sqlWarningOrError);
//LCOV_EXCL_STOP
			}
			else
			{
				odbc_SQLSvc_PrepareRowset_exc_ exception_={0,0,0};
				if(bPrepareWithRowsets)
				{
					odbc_SQLSvc_Prepare2withRowsets_sme_(objtag_, call_id_, dialogueId, sqlAsyncEnable, queryTimeout,
							maxRowsetSize, sqlStmtType, stmtLength, stmtLabel, stmtLabelCharset, cursorLength, cursorName,
							cursorCharset, moduleNameLength, moduleName, moduleCharset, moduleTimestamp,
							sqlStringLength, sqlString, sqlStringCharset, setStmtOptionsLength, setStmtOptions, holdableCursor,
							&returnCode, &sqlWarningOrErrorLength, sqlWarningOrError, &sqlQueryType,
							&stmtHandle, &estimatedCost, &inputDescLength, inputDesc,
							&outputDescLength, outputDesc);
				}
				else
				{
					odbc_SQLSvc_Prepare2_sme_(maxRowsetSize,
									sqlStmtType,
									stmtLabel,
									sqlString,
									holdableCursor,
									queryTimeout,			
									&returnCode,
									&sqlWarningOrErrorLength,
									sqlWarningOrError,
									&sqlQueryType,
									&stmtHandle,
									&estimatedCost,
									&inputDescLength,
									inputDesc,
									&outputDescLength,
									outputDesc);

				}
			}
		}
		else
		{
//LCOV_EXCL_START
			returnCode = SQL_ERROR;
			GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Connection.", &sqlWarningOrErrorLength, sqlWarningOrError);
//LCOV_EXCL_STOP
		}
	}
	else
	{
		if(bPrepareWithRowsets)
		{
			odbc_SQLSvc_Prepare2withRowsets_sme_(objtag_, call_id_, dialogueId, sqlAsyncEnable, queryTimeout,
				maxRowsetSize, sqlStmtType, stmtLength, stmtLabel, stmtLabelCharset, cursorLength, cursorName,
				cursorCharset, moduleNameLength, moduleName, moduleCharset, moduleTimestamp,
				sqlStringLength, sqlString, sqlStringCharset, setStmtOptionsLength, setStmtOptions, holdableCursor,
				&returnCode, &sqlWarningOrErrorLength, sqlWarningOrError, &sqlQueryType,
				&stmtHandle, &estimatedCost, &inputDescLength, inputDesc,
				&outputDescLength, outputDesc);

		}
		else
		{
			odbc_SQLSvc_Prepare2_sme_(maxRowsetSize,
									sqlStmtType,
									stmtLabel,
									sqlString,
									holdableCursor,
									queryTimeout,
									&returnCode,
									&sqlWarningOrErrorLength,
									sqlWarningOrError,
									&sqlQueryType,
									&stmtHandle,
									&estimatedCost,
									&inputDescLength,
									inputDesc,
									&outputDescLength,
									outputDesc);
		}
	}

	// For 64bit since the wire protocol has the stmtHandle defined as a Int32 we can no longer pass
	// the srvrStmt address to the client. Instead we'll now pass a key value, which will be used to
	// retrieve the stmtHandle from std::map definition defined in SRVR_GLOBAL_Def. The SRVR_STMT_HDL
	// constructor and destructor will be responsible to add and delete entries from the above map.
	SRVR_STMT_HDL *pSrvrStmt=NULL;
	pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;
	//publishing compile error
	if((returnCode != SQL_SUCCESS) && (returnCode != SQL_SUCCESS_WITH_INFO) && pSrvrStmt)
	{
		pSrvrStmt->m_need_21036_end_msg = true;
		pSrvrStmt->inState = STMTSTAT_CLOSE;
		pSrvrStmt->m_bqueryFinish = true;
		if(pSrvrStmt->queryStartTime <= 0)
			pSrvrStmt->queryStartTime = JULIANTIMESTAMP();

		if ((resStatStatement != NULL) && (pSrvrStmt->stmtType == EXTERNAL_STMT)) // if statement is on
			resStatStatement->endRepository(pSrvrStmt,
													  sqlWarningOrErrorLength,
													  sqlWarningOrError,
													  false);
	}

	odbc_SQLSrvr_Prepare_ts_res_(objtag_, call_id_, returnCode, sqlWarningOrErrorLength,
		sqlWarningOrError,	sqlQueryType, (pSrvrStmt !=NULL) ? pSrvrStmt->myKey : 0, estimatedCost, inputDescLength, inputDesc,
		outputDescLength, outputDesc);

	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TracePrepare2Exit(returnCode, sqlWarningOrErrorLength,
		sqlWarningOrError, sqlQueryType, stmtHandle, estimatedCost, inputDescLength, inputDesc,
		outputDescLength, outputDesc);
	}

	SRVRTRACE_EXIT(FILE_AME+19);

#ifdef PERF_TEST
	perf->clockIt("SQLSrvr_Prepare_ame_END", true);
#endif

	return;
} /* odbc_SQLSrvr_Prepare_ame_() */

extern "C" void
odbc_SQLSrvr_Fetch_ame_(
    /* In    */       CEE_tag_def      objtag_
  , /* In    */ const CEE_handle_def  *call_id_
  , /* In    */       DIALOGUE_ID_def  dialogueId
  , /* In    */       IDL_short        operation_id
  , /* In    */       IDL_long         sqlAsyncEnable
  , /* In    */       IDL_long         queryTimeout
  , /* In    */       Long         stmtHandle
  , /* In    */ const IDL_string       stmtLabel
  , /* In    */       IDL_unsigned_long_long maxRowCnt
  , /* In    */       IDL_unsigned_long_long maxRowLen

                       )
{
	SRVRTRACE_ENTER(FILE_AME+37);

	IDL_long  returnCode              = SQL_SUCCESS;
	IDL_long  rowsAffected            = 0;
	IDL_long  outValuesFormat         = UNKNOWN_DATA_FORMAT;
	IDL_long  outValuesLength         = 0;
	BYTE	   *outValues               = NULL;
	IDL_long  sqlWarningOrErrorLength = 0;
	BYTE	   *sqlWarningOrError       = NULL;
	RETCODE   rc                      = 0;
	SQL_DataValue_def outputDataValue = {0,0};

	char errorBuffer[512];            // a buffer for formatting error messages

	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceSrvrFetchEnter( dialogueId
                                                  , sqlAsyncEnable
                                                  , queryTimeout
                                                  , stmtHandle
										          , maxRowCnt
                                                  , maxRowLen
												  , (long)srvrGlobal->fetchAhead);
	}

	bool firstFetch = false;
	SRVR_STMT_HDL *pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;
	if(pSrvrStmt == NULL)
	{
		pSrvrStmt = SRVR::getSrvrStmt(stmtLabel, FALSE);

		if(pSrvrStmt == NULL)
		{
			returnCode = SQL_ERROR;
			GETMXCSWARNINGORERROR(-1, "HY000", "Statement Label not found.", &sqlWarningOrErrorLength, sqlWarningOrError);
			odbc_SQLSrvr_Fetch_ts_res_( objtag_
												 , call_id_
												 , returnCode
												 , sqlWarningOrErrorLength
												 , sqlWarningOrError
												 , rowsAffected
												 , outValuesFormat
												 , outputDataValue._length
												 , outputDataValue._buffer);
			goto FETCH_EXIT;
		}
	}

	if (srvrGlobal->fetchAhead && pSrvrStmt->sqlStmtType != TYPE_SELECT_CATALOG)
	{
		// set firstFetch here
		if (pSrvrStmt->rowsAffected == 0 && pSrvrStmt->m_curRowsFetched == 0
			&& !pSrvrStmt->sqlWarningOrError)
			firstFetch = true;

		if (!firstFetch)
		{
			// need to send response to the client
			returnCode = pSrvrStmt->returnCodeForDelayedError;
			sqlWarningOrErrorLength = pSrvrStmt->delayedSqlWarningOrErrorLength;
			sqlWarningOrError = pSrvrStmt->delayedSqlWarningOrError;
			rowsAffected = pSrvrStmt->delayedRowsAffected;
			if (srvrGlobal->drvrVersion.buildId & ROWWISE_ROWSET)
				outValuesFormat = ROWWISE_ROWSETS;
			else
				outValuesFormat = COLUMNWISE_ROWSETS;
			outputDataValue._buffer = pSrvrStmt->delayedOutputDataValue._buffer;
			outputDataValue._length = pSrvrStmt->delayedOutputDataValue._length;

			odbc_SQLSrvr_Fetch_ts_res_( objtag_
							, call_id_
							, returnCode
							, sqlWarningOrErrorLength
							, sqlWarningOrError
							, rowsAffected
							, outValuesFormat
							, outputDataValue._length
							, outputDataValue._buffer);

			if (returnCode == SQL_NO_DATA_FOUND || returnCode == SQL_ERROR || returnCode == SQL_INVALID_HANDLE || returnCode == SQL_STILL_EXECUTING)
			{
				pSrvrStmt->returnCodeForDelayedError = SQL_SUCCESS; // reset returnCodeForDelayedError
				goto FETCH_EXIT; // fetch ahead is stopped
			}
		}

	if (srvrGlobal != NULL && srvrGlobal->srvrType == CORE_SRVR)
	{
		if (srvrGlobal->srvrState == SRVR_CONNECTED)
		{
			if (dialogueId != srvrGlobal->dialogueId)
			{
//LCOV_EXCL_START
				returnCode = SQL_ERROR;
				GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Connection.", &sqlWarningOrErrorLength, sqlWarningOrError);
//LCOV_EXCL_STOP
			}
			else
			{
					odbc_SQLSrvr_FetchPerf_sme_(objtag_,
									  call_id_,
									  &returnCode,
									  dialogueId,
									  stmtLabel,
									  maxRowCnt,
									  maxRowLen,
									  sqlAsyncEnable,
									  queryTimeout,
									  &rowsAffected,
									  &outValuesFormat,
									  &outputDataValue,
									  &sqlWarningOrErrorLength,
									  sqlWarningOrError);

				} /* dialogueId == srvrGlobal->dialogueId */
			}
			else
			{
				returnCode = SQL_ERROR;
				GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Connection.", &sqlWarningOrErrorLength, sqlWarningOrError);
			} /* srvrGlobal->srvrState != SRVR_CONNECTED */
		}
		else
		{
			odbc_SQLSrvr_FetchPerf_sme_(objtag_,
								   call_id_,
								   &returnCode,
								   dialogueId,
								   stmtLabel,
								   maxRowCnt,
								   maxRowLen,
								   sqlAsyncEnable,
								   queryTimeout,
								   &rowsAffected,
								   &outValuesFormat,
								   &outputDataValue,
								   &sqlWarningOrErrorLength,
								   sqlWarningOrError);

		} /* srvrGlobal->srvrType != CORE_SRVR */

		qrysrvc_ExecuteFinished(stmtLabel, NULL, false, returnCode, true);
		if ((resStatStatement != NULL) && (pSrvrStmt->stmtType == EXTERNAL_STMT)) // if statement is on
			resStatStatement->endRepository(pSrvrStmt,
						sqlWarningOrErrorLength,
						sqlWarningOrError,
						true);
//LCOV_EXCL_START
		if (firstFetch)
		{
			odbc_SQLSrvr_Fetch_ts_res_( objtag_
									, call_id_
									, returnCode
									, sqlWarningOrErrorLength
									, sqlWarningOrError
									, rowsAffected
									, outValuesFormat
									, outputDataValue._length
									, outputDataValue._buffer);
			if (returnCode == SQL_SUCCESS || returnCode == SQL_SUCCESS_WITH_INFO)
			{
				odbc_SQLSrvr_FetchPerf_sme_(objtag_,
						  call_id_,
						  &returnCode,
						  dialogueId,
						  stmtLabel,
						  maxRowCnt,
						  maxRowLen,
						  sqlAsyncEnable,
						  queryTimeout,
						  &rowsAffected,
						  &outValuesFormat,
						  &outputDataValue,
						  &sqlWarningOrErrorLength,
						  sqlWarningOrError);
			}

		}
//LCOV_EXCL_STOP
		if (pSrvrStmt != NULL)
		{
			pSrvrStmt->returnCodeForDelayedError = returnCode;
			pSrvrStmt->delayedRowsAffected = rowsAffected;
			// Daniel - if ahead fetch got no data found return code, do not use old data buffer for sending useless data.
			if(returnCode==SQL_NO_DATA_FOUND){
				pSrvrStmt->delayedOutputDataValue._buffer=NULL;
				pSrvrStmt->delayedOutputDataValue._length=0;
			}
			else{
				pSrvrStmt->delayedOutputDataValue._buffer = outputDataValue._buffer;
				pSrvrStmt->delayedOutputDataValue._length = outputDataValue._length;
			}
			pSrvrStmt->delayedSqlWarningOrErrorLength = sqlWarningOrErrorLength;
			pSrvrStmt->delayedSqlWarningOrError = sqlWarningOrError;
		}
	}
	else
	{ // if (!srvrGlobal->fetchAhead) - keep original code to prevent regression
		if (srvrGlobal != NULL && srvrGlobal->srvrType == CORE_SRVR)
		{
			if (srvrGlobal->srvrState == SRVR_CONNECTED)
			{
				if (dialogueId != srvrGlobal->dialogueId)
				{
//LCOV_EXCL_START
					returnCode = SQL_ERROR;
					GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Connection.", &sqlWarningOrErrorLength, sqlWarningOrError);
//LCOV_EXCL_STOP
				}
				else
				{
					odbc_SQLSrvr_FetchPerf_sme_(objtag_,
									  call_id_,
									  &returnCode,
									  dialogueId,
									  stmtLabel,
									  maxRowCnt,
									  maxRowLen,
									  sqlAsyncEnable,
									  queryTimeout,
									  &rowsAffected,
									  &outValuesFormat,
									  &outputDataValue,
									  &sqlWarningOrErrorLength,
									  sqlWarningOrError);
			} /* dialogueId == srvrGlobal->dialogueId */
		}
		else
		{
//LCOV_EXCL_START
			returnCode = SQL_ERROR;
			GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Connection.", &sqlWarningOrErrorLength, sqlWarningOrError);
//LCOV_EXCL_STOP
		} /* srvrGlobal->srvrState != SRVR_CONNECTED */
	}
	else
	{
		odbc_SQLSrvr_FetchPerf_sme_(objtag_,
					    call_id_,
	  				    &returnCode,
					    dialogueId,
					    stmtLabel,
					    maxRowCnt,
					    maxRowLen,
					    sqlAsyncEnable,
					    queryTimeout,
					    &rowsAffected,
					    &outValuesFormat,
					    &outputDataValue,
					    &sqlWarningOrErrorLength,
					    sqlWarningOrError);

	} /* srvrGlobal->srvrType != CORE_SRVR */


	if (pSrvrStmt->sqlNewQueryType == SQL_SP_RESULT_SET)
	{
		if (pSrvrStmt->callStmtHandle->inState == STMTSTAT_CLOSE)
		{
			if (returnCode == SQL_ERROR && sqlWarningOrError != NULL && resStatStatement != NULL)
				resStatStatement->setSqlErrorCode(*(Int32 *)(sqlWarningOrError+8));
			else
			{
				resStatStatement->setSqlErrorCode(returnCode);
			}
			pSrvrStmt = pSrvrStmt->callStmtHandle;
		}
	}

	qrysrvc_ExecuteFinished(NULL, (long)pSrvrStmt, false, returnCode, true);
	if (pSrvrStmt != NULL) {
		if ((resStatStatement != NULL) && (pSrvrStmt->stmtType == EXTERNAL_STMT)) // if statement is on
			resStatStatement->endRepository(pSrvrStmt,
						sqlWarningOrErrorLength,
						sqlWarningOrError,
						true);
	}


	odbc_SQLSrvr_Fetch_ts_res_( objtag_
                            , call_id_
                            , returnCode
                            , sqlWarningOrErrorLength
                            , sqlWarningOrError
                            , rowsAffected
			                , outValuesFormat
                            , outputDataValue._length
                            , outputDataValue._buffer);
	}

FETCH_EXIT:
	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceSrvrFetchExit(returnCode,
							    sqlWarningOrErrorLength,
							    sqlWarningOrError,
							    rowsAffected,
							    outValuesLength,
							    outValues);
	}

	SRVRTRACE_EXIT(FILE_AME+37);
	return;

}  /* end odbc_SQLSrvr_Fetch_ame_() */


extern "C" void
odbc_SQLSrvr_ExecDirect_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_string cursorName
  , /* In    */ const IDL_char *stmtExplainLabel
  , /* In    */ IDL_short stmtType
  , /* In    */ IDL_short sqlStmtType
  , /* In    */ IDL_string sqlString
  , /* In    */ IDL_short sqlAsyncEnable
  , /* In    */ IDL_long queryTimeout
  , /* In    */ IDL_long inputRowCnt
  , /* In    */ IDL_long_long txnID       // T4 driver sends a transaction ID which we need to join
  , /* In    */ IDL_long holdableCursor
  )
{

#ifdef PERF_TEST
	perf->init();
	perf->clockIt("SQLSrvr_ExecDirect_ame_START", true);
#endif

	SRVRTRACE_ENTER(FILE_AME+23);

	IDL_long returnCode              = SQL_SUCCESS;
	IDL_long sqlWarningOrErrorLength = 0;
	BYTE     *sqlWarningOrError      = NULL;
	IDL_long rowsAffected            = 0;
    IDL_long sqlQueryType            = SQL_UNKNOWN;
    IDL_long estimatedCost           = 0;
	IDL_long outValuesLength         = 0;
	BYTE     *outValues              = NULL;
    SQLItemDescList_def outputItemDescList = {0,0};
    SQLValueList_def outputValueList = {0,0};

	char errorBuffer[512];            // a buffer for formatting error messages

	IDL_long	inputDescLength = 0;  // Output from Prepare
	BYTE		*inputDesc = NULL;    // Output from Prepare
	IDL_long	outputDescLength = 0; // Output from Prepare
	BYTE		*outputDesc = NULL;   // Output from Prepare
	Long    stmtHandle = 0;       // Output from Prepare
	IDL_long	cursorCharset = 1;    // Input for Execute
	IDL_long    inValuesLength = 0;   // Input for Execute
	BYTE        *inValues = NULL;	  // Input for Execute

	IDL_long    rowLength = 0;
	IDL_long    cursorLength = 0;
	char *outparams[20];
	bool CmdOpenOrClose = false;

	ERROR_DESC_LIST_def sqlWarning = {0,0};
    odbc_SQLSvc_ExecDirect_exc_ ExecDirect_exception_={0,0,0};
    odbc_SQLSvc_ExecuteCall_exc_ ExecCall_exception_={0,0,0};

	RETCODE rc = 0;
	bool executed = false;
	bool noRepository = true;
	IDL_long tmpPrepareRC = SQL_SUCCESS;


	SRVR_STMT_HDL *pSrvrStmt = NULL;
	if(cursorName != NULL)
		cursorLength = strlen(cursorName);

	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceExecDirectEnter(dialogueId, stmtLabel, cursorName,
							      stmtExplainLabel, stmtType, sqlStmtType,
							      sqlString, sqlAsyncEnable, queryTimeout);
	}

	if (srvrGlobal != NULL && srvrGlobal->srvrType == CORE_SRVR)
	{
		if (srvrGlobal->srvrState == SRVR_CONNECTED)
		{
			if (dialogueId != srvrGlobal->dialogueId)
			{
//LCOV_EXCL_START
				returnCode = SQL_ERROR;
				GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Connection.", &sqlWarningOrErrorLength, sqlWarningOrError);
//LCOV_EXCL_STOP
			}
			//Aruna
			else
			{
						bool rcPlan = false;
						short error;
						struct qrysrvc_exc_ wms_exception_ = {0};
						char serviceName[MAX_SERVICE_NAME_LEN + 1];

						//
						// check if command format is INFO SYSTEM
						// -- Added for manageability requirement.
						//
						if (isInfoSystem(sqlString, stmtLabel, error))
						{
							if (error != 0)
							{
//LCOV_EXCL_START
								returnCode = SQL_ERROR;
								sprintf(errorBuffer, "Operation Failed");
								GETMXCSWARNINGORERROR(-1, "S1008", errorBuffer, &sqlWarningOrErrorLength, sqlWarningOrError);
								goto cfgerrexit;
//LCOV_EXCL_STOP
							}
						}
						//
						// check if command format is INFO OBJECT
						// -- Added for manageability requirement.
						//
						else if (isInfoObject(sqlString, stmtLabel, error))
						{
							if (error != 0)
							{
//LCOV_EXCL_START
								returnCode = SQL_ERROR;
								sprintf(errorBuffer, "Operation Failed");
								GETMXCSWARNINGORERROR(-1, "S1008", errorBuffer, &sqlWarningOrErrorLength, sqlWarningOrError);
								goto cfgerrexit;
//LCOV_EXCL_STOP
							}
						}
						//
						// check if command format is INFO DISKS
						// -- Added for HPDM.
						//
						else if ((strcmp(srvrGlobal->ApplicationName, "HPDM") == 0) &&
									isInfoDisk(sqlString, stmtLabel, error, errorBuffer))
						{
							if (error != 0)
							{
//LCOV_EXCL_START
								returnCode = SQL_ERROR;
								GETMXCSWARNINGORERROR(-1, "S1008", errorBuffer, &sqlWarningOrErrorLength, sqlWarningOrError);
								// Clear diagnostics
								WSQL_EXEC_ClearDiagnostics(NULL);
								goto cfgerrexit;
//LCOV_EXCL_STOP
							}
						}
						//
						// check if command format is HPDM_GETPRIVILEGES
						// Internal command added for HPDM
						//
						else if ((strcmp(srvrGlobal->ApplicationName, "HPDM") == 0) &&
									isGetPrivileges(sqlString, stmtLabel, error))
						{
							if (error != 0)
							{
//LCOV_EXCL_START
								returnCode = SQL_ERROR;
								sprintf(errorBuffer, "Operation Failed");
								GETMXCSWARNINGORERROR(-1, "S1008", errorBuffer, &sqlWarningOrErrorLength, sqlWarningOrError);
								goto cfgerrexit;
//LCOV_EXCL_STOP
							}
						}

						if(!executed)
						{
							if (rcPlan == false)
							{
								odbc_SQLSvc_Prepare2_sme_(inputRowCnt,
											sqlStmtType,
											stmtLabel,
											sqlString,
											holdableCursor,
											queryTimeout,
											&returnCode,
											&sqlWarningOrErrorLength,
											sqlWarningOrError,
											&sqlQueryType,
											&stmtHandle,
											&estimatedCost,
											&inputDescLength,
											inputDesc,
											&outputDescLength,
										   outputDesc,
											true); // prepare is called from ExecDirect

								if(returnCode == SQL_SUCCESS ||
								   returnCode == SQL_SUCCESS_WITH_INFO)
								{
									tmpPrepareRC = returnCode;
									pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;
									if (srvrGlobal->isShapeLoaded == false)
									{

										DO_WouldLikeToExecute(NULL, stmtHandle, &returnCode, &sqlWarningOrErrorLength, sqlWarningOrError);
										if (returnCode == SQL_SUCCESS || returnCode == SQL_SUCCESS_WITH_INFO)
										{
											if(((inputRowCnt > 1) || ((inputRowCnt==1) && (pSrvrStmt->preparedWithRowsets))) && (pSrvrStmt->sqlQueryType != SQL_RWRS_SPECIAL_INSERT))//&& (paramCount > 0))
											{
											  odbc_SQLSvc_Execute2withRowsets_sme_(objtag_, call_id_, dialogueId, sqlAsyncEnable, queryTimeout,
												inputRowCnt, sqlStmtType, stmtHandle, cursorLength, cursorName, cursorCharset, holdableCursor,
												inValuesLength, inValues, &returnCode, &sqlWarningOrErrorLength, sqlWarningOrError,
												&rowsAffected, &outValuesLength, outValues);
											}
											else
											{
												if((inputRowCnt > 0) || sqlQueryType != SQL_UNKNOWN  || (inputRowCnt == 0 && pSrvrStmt != NULL &&
													(pSrvrStmt->sqlQueryType != SQL_INSERT_UNIQUE && pSrvrStmt->sqlQueryType != SQL_INSERT_NON_UNIQUE)))
												odbc_SQLSvc_Execute2_sme_(objtag_, call_id_, dialogueId, sqlAsyncEnable, queryTimeout,
													inputRowCnt, sqlStmtType, stmtHandle, cursorLength, cursorName, cursorCharset, holdableCursor,
													inValuesLength, inValues, &returnCode, &sqlWarningOrErrorLength, sqlWarningOrError,
													&rowsAffected, &outValuesLength, outValues);
											}
											estimatedCost = pSrvrStmt->rowsAffectedHigherBytes; // combine both rowsAffected and rowsAffectedHigherBytes as __int64 when interface between drvr/srvr changes
											if((tmpPrepareRC == SQL_SUCCESS_WITH_INFO) && (returnCode == SQL_SUCCESS))
												returnCode = SQL_SUCCESS_WITH_INFO;
											if (pSrvrStmt->m_need_21036_end_msg == true)
												noRepository = false;
										}
									} // srvrGlobal->isShapeLoaded == false



								   goto cfgerrexit;;
								}
								else
								{
									//publishing compile error.
									pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;
									pSrvrStmt->inState = STMTSTAT_CLOSE;
									pSrvrStmt->m_need_21036_end_msg = true;
									pSrvrStmt->m_bqueryFinish = true;
									if(pSrvrStmt->queryStartTime <= 0)
										pSrvrStmt->queryStartTime = JULIANTIMESTAMP();
									noRepository = false;

								}

							} /* if(rcPlan == false) */

						} /* if(!executed) */

			} /* else if (dialogueId == srvrGlobal->dialogueId) */

		} /* if (srvrGlobal->srvrState == SRVR_CONNECTED) */
		else
		{
			returnCode = SQL_ERROR;
			GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Connection.", &sqlWarningOrErrorLength, sqlWarningOrError);
		}
	}
	else
	{
		odbc_SQLSvc_Prepare2_sme_(inputRowCnt,
					sqlStmtType,
					stmtLabel,
					sqlString,
					holdableCursor,
					queryTimeout,
					&returnCode,
					&sqlWarningOrErrorLength,
					sqlWarningOrError,
					&sqlQueryType,
					&stmtHandle,
					&estimatedCost,
					&inputDescLength,
					inputDesc,
					&outputDescLength,
					outputDesc);

		if(returnCode == SQL_SUCCESS ||
		   returnCode == SQL_SUCCESS_WITH_INFO)
		{
			pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;
			if (srvrGlobal->isShapeLoaded == false)
			{
				DO_WouldLikeToExecute(NULL, stmtHandle, &returnCode, &sqlWarningOrErrorLength, sqlWarningOrError);

				if (returnCode == SQL_SUCCESS)
				{
					if(((inputRowCnt > 1) || ((inputRowCnt==1) && (pSrvrStmt->preparedWithRowsets))) && (pSrvrStmt->sqlQueryType != SQL_RWRS_SPECIAL_INSERT))//&& (paramCount > 0))
					{
						  odbc_SQLSvc_Execute2withRowsets_sme_(objtag_, call_id_, dialogueId, sqlAsyncEnable, queryTimeout,
								inputRowCnt, sqlStmtType, stmtHandle, cursorLength, cursorName, cursorCharset, holdableCursor,
								inValuesLength, inValues, &returnCode, &sqlWarningOrErrorLength, sqlWarningOrError,
								&rowsAffected, &outValuesLength, outValues);
					}
					else
					{
						if((inputRowCnt > 0) || sqlQueryType != SQL_UNKNOWN  || (inputRowCnt == 0 && pSrvrStmt != NULL &&
							(pSrvrStmt->sqlQueryType != SQL_INSERT_UNIQUE && pSrvrStmt->sqlQueryType != SQL_INSERT_NON_UNIQUE)))
								odbc_SQLSvc_Execute2_sme_(objtag_, call_id_, dialogueId, sqlAsyncEnable, queryTimeout,
									inputRowCnt, sqlStmtType, stmtHandle, cursorLength, cursorName, cursorCharset, holdableCursor,
									inValuesLength, inValues, &returnCode, &sqlWarningOrErrorLength, sqlWarningOrError,
									&rowsAffected, &outValuesLength, outValues);
					}
					estimatedCost = pSrvrStmt->rowsAffectedHigherBytes; // combine both rowsAffected and rowsAffectedHigherBytes as __int64 when interface between drvr/srvr changes
					if (pSrvrStmt->m_need_21036_end_msg == true)
						noRepository = false;
				}
			} // srvrGlobal->isShapeLoaded == false
		} // returnCode == SQL_SUCCESS ||  returnCode == SQL_SUCCESS_WITH_INFO
		else
		{
			//publishing compile error
			pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;
			pSrvrStmt->inState = STMTSTAT_CLOSE;
			pSrvrStmt->m_need_21036_end_msg = true;
			pSrvrStmt->m_bqueryFinish = true;
			if(pSrvrStmt->queryStartTime <= 0)
				pSrvrStmt->queryStartTime = JULIANTIMESTAMP();

			noRepository = false;
		}
	}

cfgerrexit:

	qrysrvc_ExecuteFinished(NULL,stmtHandle, true, returnCode, false);
	if (noRepository == false ) {
		if ((resStatStatement != NULL) && (pSrvrStmt->stmtType == EXTERNAL_STMT)) // if statement is on
		/*
			resStatStatement->endRepository(pSrvrStmt->inState,
					pSrvrStmt->sqlQueryType,
					pSrvrStmt->sqlString,
					pSrvrStmt->isClosed,
					pSrvrStmt->cost_info,
					pSrvrStmt->comp_stats_info,
					&pSrvrStmt->m_need_21036_end_msg,
					sqlWarningOrErrorLength,
					sqlWarningOrError);
					*/
			resStatStatement->endRepository(pSrvrStmt,
					sqlWarningOrErrorLength,
					sqlWarningOrError,
					false);
	}


	odbc_SQLSrvr_Execute_ts_res_(objtag_,
		                         call_id_,
                                 returnCode,
                                 sqlWarningOrErrorLength,
                                 sqlWarningOrError,
                                 rowsAffected,
								 sqlQueryType,
								 estimatedCost,
								 outValuesLength,         // for exec2
                                 outValues,               // for exec2
								 outputDescLength,        // for execdirect calls
								 outputDesc,              // for execdirect calls
								 stmtHandle,              // for SPJ result sets
								 (pSrvrStmt != NULL) ? pSrvrStmt->myKey : 0
								 );

	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceExecDirectExit(ExecDirect_exception_, estimatedCost, outputItemDescList,
							     rowsAffected, sqlWarning);
	}

	SRVRTRACE_EXIT(FILE_AME+23);

#ifdef PERF_TEST
	perf->clockIt("SQLSrvr_ExecDirect_ame_END", true);
#endif


} /* odbc_SQLSrvr_ExecDirect_ame_() */


extern "C" void
odbc_SQLSrvr_Execute2_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_long sqlAsyncEnable
  , /* In    */ IDL_long queryTimeout
  , /* In    */ IDL_long inputRowCnt
  , /* In    */ IDL_long sqlStmtType
  , /* In    */ Long stmtHandle
  , /* In    */ IDL_string cursorName
  , /* In    */ IDL_long cursorCharset
  , /* In    */ IDL_long inValuesLength
  , /* In    */ BYTE *inValues
  , /* In    */ IDL_long sqlQueryType     // Used with execdirect. Execdirect will call prepare/execute. This is one of the output params from prepare
  , /* In    */ IDL_long outputDescLength // Used with execdirect. Execdirect will call prepare/execute. This is one of the output params from prepare
  , /* In    */ BYTE *outputDesc          // Used with execdirect. Execdirect will call prepare/execute. This is one of the output params from prepare
  , /* In    */ IDL_long rowLength	      // For DBT to obtain the Rowlength
  , /* In    */ IDL_long_long txnID       // T4 driver sends a transaction ID which we need to join
  , /* In    */ IDL_long holdableCursor
  )
{
#ifdef PERF_TEST
	perf->clockIt("SQLSrvr_Execute2_ame_START", true);
#endif

	SRVRTRACE_ENTER(FILE_AME+19);

	IDL_long returnCode              = SQL_SUCCESS;
	IDL_long sqlWarningOrErrorLength = 0;
	BYTE     *sqlWarningOrError      = NULL;
	IDL_long rowsAffected            = 0;
    IDL_long estimatedCost           = 0;
	IDL_long outValuesLength         = 0;
	BYTE     *outValues              = NULL;
    SQLItemDescList_def outputItemDescList = {0,0};
    SQLValueList_def outputValueList = {0,0};
    ERROR_DESC_LIST_def sqlWarning = {0,0};
	char errorText[512];

	IDL_long cursorLength = (cursorName != NULL) ? strlen(cursorName) : 0;

    odbc_SQLSvc_ExecDirect_exc_ ExecDirect_exception_={0,0,0};
    odbc_SQLSvc_ExecuteCall_exc_ ExecCall_exception_={0,0,0};


	IDL_long	paramCount = 0;
	RETCODE		rc = 0;
	bool		isStatusRowsetDelayed = false;
	bool		noRepository = true;

	SRVR_STMT_HDL *pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;

	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceExecute2Enter(dialogueId, sqlAsyncEnable, queryTimeout, inputRowCnt,
			sqlStmtType, stmtHandle, cursorLength, cursorName, cursorCharset, inValuesLength, inValues);
	}

	if(pSrvrStmt == NULL)
	{
	   returnCode = SQL_ERROR;
	   GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Statement Handle.", &sqlWarningOrErrorLength, sqlWarningOrError);
	}

	if (pSrvrStmt != NULL) {
	   paramCount = pSrvrStmt->paramCount;
	   if ( pSrvrStmt->sqlQueryType == SQL_RWRS_SPECIAL_INSERT)
	      pSrvrStmt->maxRowLen = rowLength;

	}

//LCOV_EXCL_START
	// To improve the throughput as the server gets the first rowset, it returns back a success
	// code back to the application before it processes the rowset. The application can then send the
	// second rowset to the driver. By doing this, both the server and driver are always busy by
	// piggybacking the messages back and forth. Because the server and driver are always busy, the
	// application will always get the status error delayed by one rowset. For example, the application
	// sends 4 rowset of 10 rows. The first rowset will get all success back, then the second rowset
	// will get the status array for the first rowset. The second status array may have success,
	// warning, and errors for first rowset. Then the third rowset will have status array for second
	// rowset and so on. The last rowset will be a dummy to get the last status error for the previous
	// rowset which is the rowset with the valid data.
	if (srvrGlobal->drvrVersion.buildId & STREAMING_MODE || srvrGlobal->drvrVersion.buildId & STREAMING_DELAYEDERROR_MODE)
	{
		if(pSrvrStmt == NULL)
		{
			returnCode = SQL_ERROR;
			GETMXCSWARNINGORERROR(-1, "HY000", "Statement Label not found.", &sqlWarningOrErrorLength, sqlWarningOrError);
		}
		else
		{
			if(srvrGlobal->drvrVersion.buildId & STREAMING_DELAYEDERROR_MODE)
			{
				sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
				sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
				rowsAffected = pSrvrStmt->rowsAffected;
				estimatedCost = pSrvrStmt->rowsAffectedHigherBytes; // combine both rowsAffected and rowsAffectedHigherBytes as __int64 when interface between drvr/srvr changes
				outValuesLength = pSrvrStmt->outputDescVarBufferLen;
				outValues = pSrvrStmt->outputDescVarBuffer;
				returnCode = pSrvrStmt->returnCodeForDelayedError;
			}
			paramCount = pSrvrStmt->paramCount;
		}

		if(srvrGlobal->drvrVersion.buildId & STREAMING_DELAYEDERROR_MODE)
		{
			odbc_SQLSrvr_Execute_ts_res_(objtag_,
						     call_id_,
						     returnCode,
						     sqlWarningOrErrorLength,
						     sqlWarningOrError,
						     rowsAffected,
						     sqlQueryType,
						     estimatedCost,
						     outValuesLength,         // for exec2
						     outValues,               // for exec2
						     outputDescLength,        // for execdirect calls
						     outputDesc,              // for execdirect calls
						     stmtHandle,              // for SPJ result sets
						     (pSrvrStmt != NULL) ? pSrvrStmt->myKey : 0
						 );

			isStatusRowsetDelayed = true;
			returnCode = SQL_SUCCESS;
		}
	}
//LCOV_EXCL_STOP

	if(returnCode != SQL_SUCCESS)
	{
		// do nothing
	}
	else if (srvrGlobal != NULL && srvrGlobal->srvrType == CORE_SRVR)
	{
		if (srvrGlobal->srvrState == SRVR_CONNECTED)
		{
			if (dialogueId != srvrGlobal->dialogueId)
			{
//LCOV_EXCL_START
				returnCode = SQL_ERROR;
				GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Connection.", &sqlWarningOrErrorLength, sqlWarningOrError);
//LCOV_EXCL_STOP
			}
			else
			{
				if (srvrGlobal->isShapeLoaded == false)
				{
					bool bExecute2withRowsets = true;
					bool bExecute2 = true;

					bExecute2withRowsets = 	((inputRowCnt > 1) ||
												((inputRowCnt==1) &&
														(pSrvrStmt->preparedWithRowsets))) &&
															(pSrvrStmt->sqlQueryType != SQL_RWRS_SPECIAL_INSERT);

					bExecute2 = (inputRowCnt > 0) ||
									sqlQueryType != SQL_UNKNOWN  ||
										( inputRowCnt == 0 && pSrvrStmt != NULL &&
												(pSrvrStmt->sqlQueryType != SQL_INSERT_UNIQUE &&
														pSrvrStmt->sqlQueryType != SQL_INSERT_NON_UNIQUE));
					DO_WouldLikeToExecute(NULL, stmtHandle, &returnCode, &sqlWarningOrErrorLength, sqlWarningOrError);
					if (returnCode == SQL_SUCCESS && pSrvrStmt != NULL)
					{
						if(bExecute2withRowsets)
						{
						  odbc_SQLSvc_Execute2withRowsets_sme_(objtag_, call_id_, dialogueId, sqlAsyncEnable, queryTimeout,
							inputRowCnt, sqlStmtType, stmtHandle, cursorLength, cursorName, cursorCharset, holdableCursor,
							inValuesLength, inValues, &returnCode, &sqlWarningOrErrorLength, sqlWarningOrError,
							&rowsAffected, &outValuesLength, outValues);
						}
						else
						{
							if(bExecute2)
							odbc_SQLSvc_Execute2_sme_(objtag_, call_id_, dialogueId, sqlAsyncEnable, queryTimeout,
								inputRowCnt, sqlStmtType, stmtHandle, cursorLength, cursorName, cursorCharset, holdableCursor,
								inValuesLength, inValues, &returnCode, &sqlWarningOrErrorLength, sqlWarningOrError,
								&rowsAffected, &outValuesLength, outValues);
						}
						if (pSrvrStmt->m_need_21036_end_msg == true)
							noRepository = false;
					}
				} // srvrGlobal->isShapeLoaded == false
			}
		}
		else
		{
			returnCode = SQL_ERROR;
			GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Connection.", &sqlWarningOrErrorLength, sqlWarningOrError);
		}
	}
	else {
		odbc_SQLSvc_Execute2_sme_(objtag_, call_id_, dialogueId, sqlAsyncEnable, queryTimeout,
			inputRowCnt, sqlStmtType, stmtHandle, cursorLength, cursorName, cursorCharset, holdableCursor,
			inValuesLength, inValues, &returnCode, &sqlWarningOrErrorLength, sqlWarningOrError,
			&rowsAffected, &outValuesLength, outValues);
		if (pSrvrStmt->m_need_21036_end_msg == true)
			noRepository = false;
	}


	qrysrvc_ExecuteFinished(NULL,stmtHandle, true, returnCode, false);
	if (noRepository == false) {
		if ((resStatStatement != NULL) && (pSrvrStmt->stmtType == EXTERNAL_STMT)) // if statement is on
		/*	resStatStatement->endRepository(pSrvrStmt->inState,
					pSrvrStmt->sqlQueryType,
					pSrvrStmt->sqlString,
					pSrvrStmt->isClosed,
					pSrvrStmt->cost_info,
					pSrvrStmt->comp_stats_info,
					&pSrvrStmt->m_need_21036_end_msg,
					sqlWarningOrErrorLength,
					sqlWarningOrError);
					*/
			resStatStatement->endRepository(pSrvrStmt,
					sqlWarningOrErrorLength,
					sqlWarningOrError,
					false);
	}
//LCOV_EXCL_START
	// To improve the throughput as the server gets the first rowset, it returns back a success
	// code back to the application before it processes the rowset. The application can then send the
	// second rowset to the driver. By doing this, both the server and driver are always busy by
	// piggybacking the messages back and forth. Because the server and driver are always busy, the
	// application will always get the status error delayed by one rowset. For example, the application
	// sends 4 rowset of 10 rows. The first rowset will get all success back, then the second rowset
	// will get the status array for the first rowset. The second status array may have success,
	// warning, and errors for first rowset. Then the third rowset will have status array for second
	// rowset and so on. The last rowset will be a dummy to get the last status error for the previous
	// rowset which is the rowset with the valid data.
	if (!isStatusRowsetDelayed)
	{
		if (pSrvrStmt != NULL)
			estimatedCost = pSrvrStmt->rowsAffectedHigherBytes; // combine both rowsAffected and rowsAffectedHigherBytes as __int64 when interface between drvr/srvr changes
			odbc_SQLSrvr_Execute_ts_res_(objtag_,
		         call_id_,
                 returnCode,
                 sqlWarningOrErrorLength,
                 sqlWarningOrError,
                 rowsAffected,
				 sqlQueryType,
				 estimatedCost,
				 outValuesLength,         // for exec2
                 outValues,               // for exec2
				 outputDescLength,        // for execdirect calls
				 outputDesc,              // for execdirect calls
				 stmtHandle,              // for SPJ result sets
				 (pSrvrStmt != NULL) ? pSrvrStmt->myKey : 0
				 );

	}
//LCOV_EXCL_STOP


	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceExecute2Exit(returnCode, sqlWarningOrErrorLength, sqlWarningOrError,
		rowsAffected, outValuesLength, outValues);
	}

	SRVRTRACE_EXIT(FILE_AME+19);

#ifdef PERF_TEST
	perf->clockIt("SQLSrvr_Execute2_ame_END", true);
#endif

	return;

} /* odbc_SQLSrvr_Execute2_ame_() */

/*
 * Asynchronous method function prototype for
 * operation 'odbc_SQLSvc_SetConnectionOption'
 */
extern "C" void
odbc_SQLSrvr_SetConnectionOption_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_short connectionOption
  , /* In    */ IDL_long  optionValueNum
  , /* In    */ IDL_string optionValueStr)
{
	SRVRTRACE_ENTER(FILE_AME+21);
	odbc_SQLSvc_SetConnectionOption_exc_ exception_={0,0,0};
	ERROR_DESC_LIST_def   sqlWarning = {0,0};

	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceConnectOptionEnter(dialogueId,  connectionOption,
								 optionValueNum, optionValueStr);
	}

	if (srvrGlobal != NULL && srvrGlobal->srvrType == CORE_SRVR)
	{
		if (srvrGlobal->srvrState == SRVR_CONNECTED)
		{
			if (dialogueId != srvrGlobal->dialogueId)
				exception_.exception_nr = odbc_SQLSvc_SetConnectionOption_InvalidConnection_exn_;
			else
				odbc_SQLSvc_SetConnectionOption_sme_(objtag_, call_id_, &exception_, dialogueId,
						connectionOption, optionValueNum, optionValueStr, &sqlWarning);
		}
		else
			exception_.exception_nr = odbc_SQLSvc_SetConnectionOption_InvalidConnection_exn_;
	}
	else
		odbc_SQLSvc_SetConnectionOption_sme_(objtag_, call_id_, &exception_, dialogueId,
				connectionOption, optionValueNum, optionValueStr, &sqlWarning);


	odbc_SQLSrvr_SetConnectionOption_ts_res_(objtag_, call_id_, &exception_, &sqlWarning);

	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceConnectOptionExit(exception_, sqlWarning);
	}

	SRVRTRACE_EXIT(FILE_AME+21);

} // odbc_SQLSrvr_SetConnectionOption_ame_()

extern "C" void
odbc_SQLSrvr_EndTransaction_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_unsigned_short transactionOpt
  )
{
	SRVRTRACE_ENTER(FILE_AME+20);
	odbc_SQLSvc_EndTransaction_exc_ exception_={0,0,0};
	ERROR_DESC_LIST_def   sqlWarning = {0,0};
	RETCODE rc = 0;

	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceEndTransactEnter(dialogueId, transactionOpt);
	}

	if (srvrGlobal != NULL && srvrGlobal->srvrType == CORE_SRVR)
	{
		if (srvrGlobal->srvrState == SRVR_CONNECTED)
		{
			if (dialogueId != srvrGlobal->dialogueId)
				exception_.exception_nr = odbc_SQLSvc_EndTransaction_InvalidConnection_exn_;
			else
				odbc_SQLSvc_EndTransaction_sme_(objtag_, call_id_, &exception_, dialogueId,
					transactionOpt, &sqlWarning);
		}
		else
			exception_.exception_nr = odbc_SQLSvc_EndTransaction_InvalidConnection_exn_;
	}
	else
		odbc_SQLSvc_EndTransaction_sme_(objtag_, call_id_, &exception_, dialogueId,
				transactionOpt, &sqlWarning);

	odbc_SQLSrvr_EndTransaction_ts_res_(objtag_, call_id_, &exception_, &sqlWarning);

	if (srvrGlobal->traceLogger != NULL)
	{
		srvrGlobal->traceLogger->TraceEndTransactExit(exception_, sqlWarning);
	}

	SRVRTRACE_EXIT(FILE_AME+20);
	return;

} // odbc_SQLSrvr_EndTransaction_ame_()


//LCOV_EXCL_START
//#endif /* NSK_CLPS_LIB */

//LCOV_EXCL_STOP

void
odbc_SQLSrvr_ExtractLob_ame_(
    /* In   */ CEE_tag_def objtag_
  , /* In   */ const CEE_handle_def *call_id_
  , /* In   */ IDL_short   extractLobAPI
  , /* In   */ IDL_string  lobHandle
  , /* In   */ IDL_long_long    extractLen)
{
    ERROR_DESC_LIST_def sqlWarning = {0, 0};
    IDL_long_long lobLength = 0;
    BYTE * extractData = NULL;

    odbc_SQLsrvr_ExtractLob_exc_ exception_ = {0, 0};

    odbc_SQLSrvr_ExtractLob_sme_(objtag_,
                                 call_id_,
                                 &exception_,
                                 extractLobAPI,
                                 lobHandle,
                                 lobLength,
                                 extractLen,
                                 extractData);

    odbc_SQLSrvr_ExtractLob_ts_res_(objtag_,
                                    call_id_,
                                    &exception_,
                                    extractLobAPI,
                                    lobLength,
                                    extractLen,
                                    extractData);
}

void
odbc_SQLSrvr_UpdateLob_ame_(
     /* In   */ CEE_tag_def objtag_
  ,  /* In   */ const CEE_handle_def *call_id_
  ,  /* In   */ IDL_short lobUpdateType
  ,  /* In   */ IDL_string lobHandle
  ,  /* In   */ IDL_long_long totalLength
  ,  /* In   */ IDL_long_long offset
  ,  /* In   */ IDL_long_long length
  ,  /* In   */ BYTE * data)
{
    ERROR_DESC_LIST_def  sqlWarning = {0, 0};
    odbc_SQLSvc_UpdateLob_exc_ exception_ = {0, 0};

    odbc_SQLSrvr_UpdateLob_sme_(objtag_,
                                call_id_,
                                &exception_,
                                lobUpdateType,
                                lobHandle,
                                totalLength,
                                offset,
                                length,
                                data);

    odbc_SQLSrvr_UpdateLob_ts_res_(objtag_,
                                   call_id_,
                                   &exception_
                                   );

}

void getCurrentCatalogSchema()
{
	short			Index;
	char			sqlQuery[] = "showcontrol default schema";
	SQLRETURN		iqqcode = SQL_SUCCESS;
	SRVR_STMT_HDL	*QrySrvrStmt = NULL;
	//"Current DEFAULTS"
	//"  CATALOG                       	NEO"
	//"  SCHEMA                        	USR"
	//char			Defaults[]  = "Current DEFAULTS ";
	char			Defaults[]  = "Current DEFAULTS";
	char			CatPatern[] = "CATALOG";
	char			SchPatern[] = "SCHEMA";
	short			lenDef = sizeof(Defaults) - 1;
	short			lenCat = sizeof(CatPatern) - 1;
	short			lenSch = sizeof(SchPatern) - 1;
	char* ptr;
	unsigned long len;
	char			seps[]   = " \t\n";
	char			*token;
	char			*saveptr;
	char			temp[300];
	unsigned long	templen = 0;
;

// showcontrol default schema;

	if ((QrySrvrStmt = getSrvrStmt("STMT_QRYSTS_ON_1", TRUE)) == NULL)
		return;

	Index = 0;

	iqqcode = QrySrvrStmt->ExecDirect(NULL, sqlQuery, EXTERNAL_STMT, TYPE_SELECT, SQL_ASYNC_ENABLE_OFF, 0);
	if (iqqcode != SQL_ERROR)
	{
		while((iqqcode = QrySrvrStmt->FetchPerf(1, 0, SQL_ASYNC_ENABLE_OFF, 0)) != SQL_NO_DATA_FOUND && iqqcode != SQL_ERROR)
		{
			ptr = (char*)QrySrvrStmt->outputDataValue._buffer;
			len = *(unsigned long*)ptr;
			len &= 0x00FFFF00;
			len >>= 8;
			ptr += 3;

			if (len == 0) continue;

			switch(Index)
			{
			case 0:
				if (memcmp(ptr, Defaults, lenDef) == 0)
					Index = 1;
				break;
			case 1:
				memcpy(temp,ptr,len);
				temp[len] = '\0';
				token = strtok_r(temp, seps, &saveptr);
				if (token != NULL)
				{
					if (memcmp(token, CatPatern, lenCat) == 0)
					{
						token = strtok_r(NULL, seps, &saveptr);
						if (token != NULL)
						{
							bzero(srvrGlobal->DefaultCatalog, sizeof(srvrGlobal->DefaultCatalog));
							templen = token - temp;
							len = _min((temp+len)-token, sizeof(srvrGlobal->DefaultCatalog)-1);
							memcpy(srvrGlobal->DefaultCatalog, ptr+templen, len);
						}
					}
					else
					if (memcmp(token, SchPatern, lenSch) == 0)
					{
						token = strtok_r(NULL, seps, &saveptr);
						if (token != NULL)
						{
							bzero(srvrGlobal->DefaultSchema, sizeof(srvrGlobal->DefaultSchema));
							templen = token - temp;
							len = _min((temp+len)-token, sizeof(srvrGlobal->DefaultSchema)-1);
							memcpy(srvrGlobal->DefaultSchema, ptr+templen, len);
						}
					}
				}
				break;
			}
		}
	}
	QrySrvrStmt->Close(SQL_DROP);
}

void flushCollectors()
{
}








static bool strincmp(char* in, char* out, short ilen)
{
	short i = 0;
	char* iin = in;
	char* oout = out;
	char ich;
	char och;

	while (*iin != '\0' && i++ < ilen)
	{
		ich = *iin++;
		och = *oout++;
		if ((ich | 0x20) != (och | 0x20))
			return false;
	}
	return true;
}

bool checkSyntaxInfoSystem(char* sqlString)
{
	char* in = sqlString;

	while (*in != '\0' && isspace(*in)) in++;   // Skip the leading blanks

	if (strincmp(in,"INFO",4) == false)
		return false;
	in += 4;
	if (*in == '\0' || false == isspace(*in))
		return false;

	while (*in != '\0' && isspace(*in)) in++;   // Skip the leading blanks

	if (strincmp(in,"SYSTEM",6) == false)
		return false;
	in += 6;
	if (*in != '\0' && *in != ';' && false == isspace(*in))
		return false;

	return true;
}

bool isInfoSystem(char*& sqlString, const IDL_char *stmtLabel, short& error)
{
   if (false == checkSyntaxInfoSystem(sqlString))
      return false;

   error = 0;
   static char buffer[4000];
   char* in = sqlString;
   SRVR_STMT_HDL *pSrvrStmt = NULL;
   char *databaseVersion;
  
   databaseVersion = getenv("TRAFODION_VER");

   // get Timezone and GMT offset
   time_t tim     = time(NULL);
   struct tm *now = localtime(&tim);

   string databaseEdition = getenv("TRAFODION_VER_PROD"); 
 
   bool authenticationEnabled = false;
   bool authorizationEnabled = false;
   bool authorizationReady = false;
   bool auditingEnabled = false;
 
   Int32 rc = SQL_EXEC_GetAuthState(authenticationEnabled,
                            authorizationEnabled,
                            authorizationReady,
                            auditingEnabled);

   char pattern[] = "SELECT [first 1]"
                    "current_timestamp as \"CURRENT_TIME\","
                    "'%s' as \"NDCS_VERSION\","
                    "'%s' as \"TM_ZONE\","
                    "'%d' as \"TM_GMTOFF_SEC\","
                    "'%s' as \"DATABASE_VERSION\","
                    "'%s' as \"DATABASE_EDITION\","
                    "'%s' as \"AUTHENTICATION_ENABLED\","
                    "'%s' as \"AUTHORIZATION_ENABLED\""
	            "FROM (values(1)) X(A);";

   sprintf (buffer, pattern,
	    ndcs_vers_str(),
            now->tm_zone,
            now->tm_gmtoff,
            databaseVersion,
            databaseEdition.c_str(),
            authenticationEnabled ? "true" : "false",
            authorizationEnabled ? "true" : "false");

// other comments:
// the repository view does not exist - maybe a M6 item
// platform version: SCM have anything to return just the platform version.
// its tagged on at the end of the version string for each component (the bits after 'Release'
// ex: mxosrvr Version 1.0.1 Release 5.0.0 (Build release [5939], date 03Apr11)



	if (stmtLabel != NULL && stmtLabel[0] != 0)
		pSrvrStmt = SRVR::getSrvrStmt(stmtLabel, TRUE);

	if (pSrvrStmt == NULL)
	{
		error = 1;
		return true;
	}

	pSrvrStmt->m_bSkipWouldLikeToExecute = true;

	sqlString = buffer;

	return true;
}

bool checkSyntaxInfoObject(char* sqlString, short &idx)
{
	char* in = sqlString;

	while (*in != '\0' && isspace(*in)) in++;   // Skip the leading blanks

	if (strincmp(in,"INFO",4) == false)
		return false;
	in += 4;
	if (*in == '\0' || false == isspace(*in))
		return false;

	while (*in != '\0' && isspace(*in)) in++;   // Skip the leading blanks

	if (strincmp(in,"OBJECT",6) == false)
		return false;
	in += 6;

	while (*in != '\0' && isspace(*in)) in++;   // Skip the leading blanks

	if (*in == '\0' || *in == ';')
		return false;

	idx = in - sqlString;
	if(idx <= 0 || idx >= strlen(sqlString))
		return false;

	return true;
}

// This method checks if sqlString starts with "INFO OBJECT <object_name>" and does a SQL
// invoke on the SQL object passed with the command.
bool isInfoObject(char*& sqlString, const IDL_char *stmtLabel, short& error )
{
	short idx = 0;
	// idx will contain the index in the sqlString where the table name will start
	if (false == checkSyntaxInfoObject(sqlString, idx))
		return false;

	static char buffer[512];
	char pattern[] = "INVOKE %s;";

	char* in = sqlString;
	SRVR_STMT_HDL *pSrvrStmt = NULL;

	char *pStr = sqlString+idx;
	sprintf (buffer, pattern, pStr);

	if (stmtLabel != NULL && stmtLabel[0] != 0)
		pSrvrStmt = SRVR::getSrvrStmt(stmtLabel, TRUE);

	if (pSrvrStmt == NULL)
	{
		error = 1;
		return true;
	}

	pSrvrStmt->m_bSkipWouldLikeToExecute = true;

	sqlString = buffer;

	return true;
}

bool checkSyntaxGetPrivileges(char* sqlString, short& idx)
{
	char* in = sqlString;

	while (*in != '\0' && isspace(*in)) in++;   // Skip the leading blanks

	if (strincmp(in,"HPDM_GETPRIVILEGES",18) == false)
		return false;
	in += 18;
	if (*in == '\0' || false == isspace(*in))
		return false;

	while (*in != '\0' && isspace(*in)) in++;   // Skip the leading blanks

	idx = (short)(in - sqlString);
	if(idx >= strlen(sqlString))
		return false;

	return true;
}

bool isGetPrivileges(char*& sqlString, const IDL_char *stmtLabel, short& error )
{
	static char buffer[1000];
	char* in = sqlString;
	SRVR_STMT_HDL *pSrvrStmt = NULL;
	char temp[4];
	short idx;

	if (false == checkSyntaxGetPrivileges(sqlString, idx))
		return false;

	strcpy(buffer, &sqlString[idx]);

	if (stmtLabel != NULL && stmtLabel[0] != 0)
		pSrvrStmt = SRVR::getSrvrStmt(stmtLabel, TRUE);

	if (pSrvrStmt == NULL)
	{
		error = 1;
		return true;
	}

	//pSrvrStmt->m_bSkipWouldLikeToExecute = true;
	pSrvrStmt->querySpl = SPEC_QUERY_IMMEDIATE;

	sqlString = buffer;
	return true;
}

// Valid Syntax:
// INFO DISK [ALL | <disk_name>]
bool checkSyntaxInfoDisk(char* sqlString, char *diskName)
{
	char* in = sqlString;

	while (*in != '\0' && isspace(*in)) in++;   // Skip the leading blanks

	if (strincmp(in,"INFO",4) == false)
		return false;
	in += 4;
	if (*in == '\0' || false == isspace(*in))
		return false;

	while (*in != '\0' && isspace(*in)) in++;   // Skip the leading blanks

	if (strincmp(in,"DISK",4) == false)
		return false;
	in += 4;

	if (*in == '\0')
		return true;

	if (false == isspace(*in))
		return false;

	while (*in != '\0' && isspace(*in)) in++;   // Skip the leading blanks

	if (*in == '\0')
		return true;

	if (strincmp(in,"ALL",3) == true)
	{
		strcpy( diskName, "ALL" );
		in += 3;
	}
	else	// If a disk name is provided then use that
	{
		short i=0;
		while (*in != '\0' && !isspace(*in))
		{
			diskName[i] = *in;
			in++;
			i++;
		}
	}

	if (*in != '\0' && *in != ';' && false == isspace(*in))
		return false;

	return true;
}

bool isInfoDisk(char*& sqlString, const IDL_char *stmtLabel, short& error, char *errBuf )
{
  return true;

#if 0
  // Obsolete function, should not be used

	static char buffer[1000];
	static char * str = NULL;
	static int strSize = 0;
	int newSize = 0;
	char diskName[MS_MON_MAX_PROCESS_NAME +1];

	memset(diskName, '\x0', sizeof(diskName));
	if (false == checkSyntaxInfoDisk(sqlString, diskName))
	  return false;

	static char pattern[] = "SELECT "
									"DISK_NAME,"
									"CAPACITY,"
									"FREESPACE "
							"FROM(VALUES("
									"CAST('%s' as VARCHAR(50) CHARACTER SET UTF8),"
									"CAST(%lld AS LARGEINT),"
									"CAST(%lld AS LARGEINT)"
									")) "
							"QTABLE ("
									"\"DISK_NAME\","
									"\"CAPACITY\","
									"\"FREESPACE\""
									")";


	Int64 capacity, freeSpace;
	int retCode;
	SRVR_STMT_HDL *pSrvrStmt = NULL;
	char *diskBuf = NULL;
	int numTSE = 0, maxTSELen = 0, diskBufLen = 0;
	short retryCnt;
	char volume[MS_MON_MAX_PROCESS_NAME +1];
	stringstream ss;
	stringstream ss1;
	stringstream ss2;
	stringstream ss3;

	// If no disk name specified then default to $SYSTEM
	if(strlen(diskName) == 0)
		strcpy(diskName, "$SYSTEM");

	if (strincmp(diskName,"ALL",3) == true)
	{
		diskBufLen = 16384;
		// If SQL_EXEC_GetListOfDisks() returns with an error for insufficient diskBuf space
		// then we'll need to retry with the corrected size returned back in diskBufLen.
		retryCnt = 3;
		while( retryCnt-- )
		{
			diskBuf = new (nothrow) char[diskBufLen];
			if (diskBuf == NULL)
			{
				error = -1;
				sprintf(errBuf, "Operation Failed. isInfoDisk:new diskBuf failed");
				goto out;
			}

			retCode = SQL_EXEC_GetListOfDisks( diskBuf,
												&numTSE,
												&maxTSELen,
												&diskBufLen);
			// If error then numTSE and maxTSELen will be populated to calculate
			// correct diskBuf size
			if(retCode == -8879)	// Error CLI_BUFFER_TOO_SMALL - retry
			{
				if(diskBuf != NULL )
				{
					delete [] diskBuf;
					diskBuf = NULL;
				}
				if( retryCnt > 0 )
					// Clear diagnostics in case we had got a -8879 error
					SRVR::WSQL_EXEC_ClearDiagnostics(NULL);
			}
			else
				break;
		}	// end while

		if( retCode != 0 )
		{
			error = retCode;
			sprintf(errBuf, "Operation Failed. SQL_EXEC_GetListOfDisks failed with error %d", retCode);
			goto out;
		}

		bool isFirst=true;

		ss1 << "DISK_NAME,";
		ss1 << "CAPACITY,";
		ss1 << "FREESPACE ";

		for (int i= 0; i<numTSE; i++)
		{
		    strcpy(volume, &diskBuf[i*maxTSELen]);	// each dik name will be NULL terminated
			retCode = SQL_EXEC_GetDiskMaxSize(volume, &capacity, &freeSpace);
			if( retCode != 0 )
			{
				error = retCode;
				sprintf(errBuf, "Operation Failed. SQL_EXEC_GetDiskMaxSize failed with error %d", retCode);
				goto out;
			}

			if(isFirst == false)
				ss2 << ",";

			ss2 << "(";
			ss2 << "CAST('";
			ss2 << volume;
			ss2 << "' as VARCHAR(50) CHARACTER SET UTF8),";
			ss2 << "CAST(";
			ss2 << capacity;
			ss2 << " AS LARGEINT),";
			ss2 << "CAST(";
			ss2 << freeSpace;
			ss2 << " AS LARGEINT)";
			ss2 << ")";

			isFirst = false;
		}
		ss3 << "\"DISK_NAME\",";
		ss3 << "\"CAPACITY\",";
		ss3 << "\"FREESPACE\"";

		ss << "SELECT ";
		ss << ss1.str().c_str();
		ss << " FROM(VALUES";
		ss << ss2.str().c_str();
		ss << ") ";
		ss << "QTABLE(";
		ss << ss3.str().c_str();
		ss << ")";

		// str will not be deleted but instead reused for subsequent calls
		newSize = ss.str().length() + 1;
		if( strSize < newSize )
		{
			if( str != NULL ) {
				delete [] str;
				str = NULL;
			}
			strSize = newSize;
			str = new char[strSize];
		}

		if (str == NULL)
		{
			error = 1;
			sprintf(errBuf, "Operation Failed. isInfoDisk:new str failed");
			goto out;
		}

		strcpy(str, ss.str().c_str());
		sqlString = str;
	}
	else
	{
		retCode = SQL_EXEC_GetDiskMaxSize(diskName, &capacity, &freeSpace);
		if( retCode == 0 )
		{
			sprintf (buffer, pattern,
								diskName,
								capacity,
								freeSpace
								);
			sqlString = buffer;
		}
		else
		{
			error = retCode;
			sprintf(errBuf, "Operation Failed. SQL_EXEC_GetDiskMaxSize failed with error %d", retCode);
			goto out;
		}
	}

	if (stmtLabel != NULL && stmtLabel[0] != 0)
		pSrvrStmt = SRVR::getSrvrStmt(stmtLabel, TRUE);

	if (pSrvrStmt == NULL)
	{
		error = -1;
		sprintf(errBuf, "Operation Failed. SRVR::getSrvrStmt failed");
		goto out;
	}
	pSrvrStmt->m_bSkipWouldLikeToExecute = true;

out:
	if(diskBuf != NULL )
	{
		delete [] diskBuf;
		diskBuf = NULL;
	}

	return true;
#endif        
}


bool updateZKState(DCS_SERVER_STATE currState, DCS_SERVER_STATE newState)
{
	int rc = ZOK;
	stringstream ss;
	Stat stat;
	bool zk_error = false;
	string nodeToCheck;
	char realpath[1024];
	char zkErrStr[2048];
	char zkData[256];
	char state[32];
	int zkDataLen = sizeof(zkData);

	if( currState == CONNECTING && newState == CONNECTED )
	{
		struct sockaddr_in clientaddr;
		socklen_t addrlen = sizeof(clientaddr);
		char str[INET6_ADDRSTRLEN];
		char s_port[10];

		rc = zoo_exists(zh, dcsRegisteredNode.c_str(), 0, &stat);
		if( rc == ZOK )
		{
/*
			// Get the dialogue ID from the data part of connecting znode
			rc = zoo_get(zh, dcsRegisteredNode.c_str(), false, zkData, &zkDataLen, &stat);
			if( rc != ZOK )
			{
				zk_error = true;
				sprintf(zkErrStr, "***** zoo_get() for %s failed with error %d",dcsRegisteredNode.c_str(), rc);
				goto bailout;
			}
			// The first token should be CONNECTING state
			char *tkn = NULL;
			tkn = strtok(zkData, ":");
			if( tkn == NULL || stricmp(tkn, "CONNECTING") )
			{
				zk_error = true;
				sprintf(zkErrStr, "***** State not in CONNECTING. State: %s", tkn);
				goto bailout;
			}

			// Skip second token - Timestamp
			tkn = strtok(NULL, ":");

			// Third token in data is dialogue ID
			srvrGlobal->dialogueId = -1;
			tkn = strtok(NULL, ":");
			if( tkn != NULL )
				srvrGlobal->dialogueId = atoi(tkn);

			if( tkn == NULL || srvrGlobal->dialogueId == -1 )
			{
				zk_error = true;
				sprintf(zkErrStr, "***** Connecting state dialogue ID not found");
				goto bailout;
			}
*/
			string connectedSrvrData;
			getpeername (sdconn, (struct sockaddr *) &clientaddr, &addrlen);

			ss.str("");

			if (inet_ntop(AF_INET, &clientaddr.sin_addr, str, sizeof(str))){
				sprintf(s_port, "%d", ntohs(clientaddr.sin_port));
				ss << "CONNECTED"
				   << ":"
				   << JULIANTIMESTAMP()
				   << ":"
				   << srvrGlobal->dialogueId
				   << ":"
				   << regSrvrData
				   << ":"
				   << srvrGlobal->ClientComputerName
				   << ":"
				   << str
				   << ":"
				   << s_port
				   << ":"
				   << srvrGlobal->ApplicationName
			   	   << ":";

			}
			else {
				ss << "CONNECTED"
				   << ":"
				   << JULIANTIMESTAMP()
				   << ":"
				   << srvrGlobal->dialogueId
				   << ":"
				   << regSrvrData
				   << ":"
				   << srvrGlobal->ClientComputerName
				   << ":"
				   << strerror(errno)
				   << ":"
				   << errno
				   << ":"
				   << srvrGlobal->ApplicationName
			   	   << ":";

			}
			string data(ss.str());

			rc = zoo_set(zh, dcsRegisteredNode.c_str(), data.c_str(), data.length(), -1);

			if( rc != ZOK )
			{
				zk_error = true;
				sprintf(zkErrStr, "***** zoo_set() failed for %s with error %d", dcsRegisteredNode.c_str(), rc);
				goto bailout;
			}
			else
				srvrGlobal->dcsCurrState = CONNECTED;
		}
		else
		{
			zk_error = true;
			sprintf(zkErrStr, "***** zoo_exists() for %s failed with error %d",dcsRegisteredNode.c_str(), rc);
			goto bailout;
		}
	}
	else
	if( currState == CONNECTING && newState == AVAILABLE )	// A Connection failure
	{
		rc = zoo_exists(zh, dcsRegisteredNode.c_str(), 0, &stat);
		if( rc == ZOK )
		{
			ss.str("");
			ss << "AVAILABLE"
			   << ":"
			   << JULIANTIMESTAMP()
			   << ":"					// Dialogue ID
			   << ":"
			   << regSrvrData
			   << ":"					// Client computer name
			   << ":"					// Client address
			   << ":"					// Client port
			   << ":"					// Client Appl name
			   << ":";

			string data(ss.str());

			rc = zoo_set(zh, dcsRegisteredNode.c_str(), data.c_str(), data.length(), -1);
			if( rc != ZOK )
			{
				zk_error = true;
				sprintf(zkErrStr, "***** zoo_set() failed for %s with error %d", dcsRegisteredNode.c_str(), rc);
				goto bailout;
			}
			else
				srvrGlobal->dcsCurrState = AVAILABLE;
		}
		else
		{
			zk_error = true;
			sprintf(zkErrStr, "***** zoo_exists() for %s failed with error %d",dcsRegisteredNode.c_str(), rc);
			goto bailout;
		}
	}
	else
	if( currState == CONNECTED && newState == AVAILABLE)	// Move from connected to available
	{
		// Fix for bug #1315537 - ZK dialogue ID mismatch.
		// Added check to not set to AVAILABLE if already in that state in case a break dialogue is called after a terminate dialogue.
		if( srvrGlobal->dcsCurrState == AVAILABLE )
			return true;
		rc = zoo_exists(zh, dcsRegisteredNode.c_str(), 0, &stat);
		if( rc == ZOK )
		{
			ss.str("");
			ss << "AVAILABLE"
			   << ":"
			   << JULIANTIMESTAMP()
			   << ":"					// Dialogue ID
			   << ":"
			   << regSrvrData
			   << ":"					// Client computer name
			   << ":"					// Client address
			   << ":"					// Client port
			   << ":"					// Client Appl name
			   << ":";

			string data(ss.str());

			rc = zoo_set(zh, dcsRegisteredNode.c_str(), data.c_str(), data.length(), -1);
			if( rc != ZOK )
			{
				zk_error = true;
				sprintf(zkErrStr, "***** zoo_set() failed for %s with error %d", dcsRegisteredNode.c_str(), rc);
				goto bailout;
			}
			else
				srvrGlobal->dcsCurrState = AVAILABLE;
		}
		else
		{
			zk_error = true;
			sprintf(zkErrStr, "***** zoo_exists() for %s failed with error %d",dcsRegisteredNode.c_str(), rc);
			goto bailout;
		}
	}
	else
	if( currState == CONNECTING && (newState == CONNECT_FAILED || newState == CONNECT_REJECTED) )	// A Connection failure
	{
		rc = zoo_exists(zh, dcsRegisteredNode.c_str(), 0, &stat);
		if( rc == ZOK )
		{
			ss.str("");
			if (newState == CONNECT_FAILED)
				ss << "CONNECT_FAILED"
				   << ":"
				   << JULIANTIMESTAMP()
				   << ":"					// Dialogue ID
				   << srvrGlobal->dialogueId
				   << ":"
				   << regSrvrData
				   << ":"					// Client computer name
				   << ":"					// Client address
				   << ":"					// Client port
				   << ":"					// Client Appl name
				   << ":";
			else
				ss << "CONNECT_REJECTED"
				   << ":"
				   << JULIANTIMESTAMP()
				   << ":"					// Dialogue ID
				   << srvrGlobal->dialogueId
				   << ":"
				   << regSrvrData
				   << ":"					// Client computer name
				   << ":"					// Client address
				   << ":"					// Client port
				   << ":"					// Client Appl name
				   << ":";

			string data(ss.str());

			rc = zoo_set(zh, dcsRegisteredNode.c_str(), data.c_str(), data.length(), -1);
			if( rc != ZOK )
			{
				zk_error = true;
				sprintf(zkErrStr, "***** zoo_set() failed for %s with error %d", dcsRegisteredNode.c_str(), rc);
				goto bailout;
			}
			else
			{
				if (newState == CONNECT_FAILED)
					srvrGlobal->dcsCurrState = CONNECT_FAILED;
				else
				if (newState == CONNECT_REJECTED)
					srvrGlobal->dcsCurrState = CONNECT_REJECTED;
			}
		}
		else
		{
			zk_error = true;
			sprintf(zkErrStr, "***** zoo_exists() for %s failed with error %d",dcsRegisteredNode.c_str(), rc);
			goto bailout;
		}
	}
	else
	if( (currState == CONNECT_FAILED || currState == CONNECT_REJECTED) && newState == AVAILABLE)	// Move from connected to available
	{
		rc = zoo_exists(zh, dcsRegisteredNode.c_str(), 0, &stat);
		if( rc == ZOK )
		{
			ss.str("");
			ss << "AVAILABLE"
			   << ":"
			   << JULIANTIMESTAMP()
			   << ":"					// Dialogue ID
			   << ":"
			   << regSrvrData
			   << ":"					// Client computer name
			   << ":"					// Client address
			   << ":"					// Client port
			   << ":"					// Client Appl name
			   << ":";

			string data(ss.str());

			rc = zoo_set(zh, dcsRegisteredNode.c_str(), data.c_str(), data.length(), -1);
			if( rc != ZOK )
			{
				zk_error = true;
				sprintf(zkErrStr, "***** zoo_set() failed for %s with error %d", dcsRegisteredNode.c_str(), rc);
				goto bailout;
			}
			else
				srvrGlobal->dcsCurrState = AVAILABLE;
		}
		else
		{
			zk_error = true;
			sprintf(zkErrStr, "***** zoo_exists() for %s failed with error %d",dcsRegisteredNode.c_str(), rc);
			goto bailout;
		}
	}


bailout:
	if(zk_error)
		SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
				0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
				1, zkErrStr);

	return (zk_error == ZOK) ? true : false;
}

short DO_WouldLikeToExecute(
                  IDL_char *stmtLabel
                , Long stmtHandle
                , IDL_long* returnCode
                , IDL_long* sqlWarningOrErrorLength
                , BYTE*& sqlWarningOrError
                )
{
        SRVR_STMT_HDL *pSrvrStmt = NULL;
        if (stmtLabel != NULL && stmtLabel[0] != 0)
            pSrvrStmt = SRVR::getSrvrStmt(stmtLabel, FALSE);
        else
            pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;

        if (pSrvrStmt == NULL)
            return 0;

        if (srvrGlobal->sqlPlan)
        {
            if (! getSQLInfo( EXPLAIN_PLAN, stmtHandle, stmtLabel ))
            {
                // Clear diagnostics if there were errors while retrieving the plan
                SRVR::WSQL_EXEC_ClearDiagnostics(NULL);

                if (pSrvrStmt->sqlPlan != NULL)
                {
                    delete pSrvrStmt->sqlPlan;
                    pSrvrStmt->sqlPlan = NULL;
                    pSrvrStmt->sqlPlanLen = 0;
                }
            }
        }

        if (resStatStatement != NULL)
        {
            resStatStatement->wouldLikeToStart_ts = JULIANTIMESTAMP();
            resStatStatement->pubStarted = false;
            resStatStatement->queryFinished = false;
        }

// Update the query status
        pSrvrStmt->m_state = QUERY_EXECUTING;

        pQueryStmt = pSrvrStmt;
        pSrvrStmt->m_bDoneWouldLikeToExecute = true;
        if ((srvrGlobal->m_bStatisticsEnabled)&&(srvrGlobal->m_statisticsPubType==STATISTICS_AGGREGATED)&&(srvrGlobal->m_iQueryPubThreshold>=0))
        {
            limit_count=0;
        }

        return 0;
}

short qrysrvc_ExecuteFinished(
		const IDL_char *stmtLabel
		, const Long stmtHandle
		, const bool bCheckSqlQueryType
		, const short error_code
		, const bool bFetch
		, const bool bException
		, const bool bErase
		)
{
	SRVR_STMT_HDL *pSrvrStmt = NULL;

	#define RC_SUCCESS(retcode) \
		((!bException && (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)) || \
		 ( bException && (retcode == CEE_SUCCESS)) ? TRUE : FALSE)

	if (stmtLabel != NULL && stmtLabel[0] != 0)
		pSrvrStmt = SRVR::getSrvrStmt(stmtLabel, FALSE);
	else
		pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;

	if (pSrvrStmt == NULL)
		return 0;

	if (pSrvrStmt->m_bDoneWouldLikeToExecute == false)
		return 0;

	pSrvrStmt->m_bqueryFinish = true;

	if (bCheckSqlQueryType)
	{
		if (RC_SUCCESS(error_code) &&
			(pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE ||
			pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE ||
			pSrvrStmt->sqlQueryType == SQL_CALL_WITH_RESULT_SETS ||
			pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET))
		{
			pSrvrStmt->m_bqueryFinish = false;
			return 0;
		}
	}
	else if (bFetch)
	{
		if (RC_SUCCESS(error_code))
		{
			pSrvrStmt->m_bqueryFinish = false;
			return 0;
		}
	}

	// Update the query status
	if (pSrvrStmt->m_bqueryFinish)
	{
		pSrvrStmt->m_state = QUERY_COMPLETED;
		if (pSrvrStmt->sqlWarningOrError)
		{
			Int32 sqlError = *(Int32 *)(pSrvrStmt->sqlWarningOrError+8);
			if (sqlError == -8007)
			{
				pSrvrStmt->m_state = QUERY_COMPLETED_BY_ADMIN_SERVER;
			}
		}
		else if (STMTSTAT_CLOSE == pSrvrStmt->inState)
		{
			pSrvrStmt->m_state = QUERY_COMPLETED_BY_CLIENT;
		}
	}

	if (resStatStatement != NULL)
	{
		resStatStatement->queryFinished = true;
		resStatStatement->wouldLikeToStart_ts = 0;
	}
	pQueryStmt = NULL;

	return 0;
}

void sendSessionStats(std::tr1::shared_ptr<SESSION_INFO> pSession_info)
{
	REPOS_STATS session_stats;
	session_stats.m_pSessionStats = pSession_info;
	if (record_session_done)
	{
	        session_stats.m_pub_type = PUB_TYPE_SESSION_END;
		pthread_t thrd;
		pthread_create(&thrd, NULL, SessionWatchDog, NULL);
	} else
                session_stats.m_pub_type = PUB_TYPE_SESSION_START;
	repos_queue.push_task(session_stats);
}

void sendAggrStats(pub_struct_type pub_type, std::tr1::shared_ptr<SESSION_AGGREGATION> pAggr_info)
{
	REPOS_STATS aggr_stats;
	aggr_stats.m_pAggr_stats = pAggr_info;
	aggr_stats.m_pub_type = pub_type;
	if (record_session_done)
	{
		pthread_t thrd;
		pthread_create(&thrd, NULL, SessionWatchDog, NULL);
	}
	repos_queue.push_task(aggr_stats);
}

void sendQueryStats(pub_struct_type pub_type, std::tr1::shared_ptr<STATEMENT_QUERYEXECUTION> pQuery_info)
{
	REPOS_STATS query_stats;
	query_stats.m_pQuery_stats = pQuery_info;
	query_stats.m_pub_type = pub_type;
	if (record_session_done)
	{
		pthread_t thrd;
		pthread_create(&thrd, NULL, SessionWatchDog, NULL);
	}
	repos_queue.push_task(query_stats);
}

void __cdecl StatisticsTimerExpired(CEE_tag_def timer_tag)
{
    if (!(srvrGlobal->m_bStatisticsEnabled && srvrGlobal->m_statisticsPubType==STATISTICS_AGGREGATED))
		return;
	//update aggregation stats per interval
	if(++interval_count >=interval_max)
	{
	    if(resStatSession != NULL)
			resStatSession->update();		
		interval_count=0;	
	}
	
    //update query stats once longer than limit	    
	if(limit_max>=0
		&& resStatStatement != NULL
		&& !resStatStatement->queryFinished
		&& !resStatStatement->pubStarted
		&& resStatStatement->wouldLikeToStart_ts > 0
		&& pQueryStmt!= NULL)			
	{
		if(limit_count++ >=limit_max)
		{
			resStatStatement->SendQueryStats(true, pQueryStmt);
			limit_count=0;
		}		
	}	
	
}

void SyncPublicationThread()
{
	if (!record_session_done)
	{
		REPOS_STATS exit_stats;
		exit_stats.m_pub_type = PUB_TYPE_INIT;

		// Fix for bug 1404108 where mxosrvr processes do not stop when sqstop is called
		// Will loop until the SessionWatchDog thread exits, which is holding on to Thread_mutex
		bool mDone = false;
		int mReturn = 0;
		while(!mDone)
		{
			repos_queue.push_task(exit_stats);
			mReturn = pthread_mutex_trylock(&Thread_mutex);

			char tmpstr[256];
			sprintf( tmpstr, "pthread_mutex_trylock()...returned...%d", mReturn );
			SendEventMsg(MSG_SERVER_TRACE_INFO,
							  EVENTLOG_INFORMATION_TYPE,
							  srvrGlobal->nskASProcessInfo.processId,
							  ODBCMX_SERVICE,
							  srvrGlobal->srvrObjRef,
							  3,
							  srvrGlobal->sessionId,
							  tmpstr,
							  "0");

			if( mReturn == 0 )
				mDone = true;
			else
				sleep(1);
		}
		pthread_mutex_unlock(&Thread_mutex);
	}
}
