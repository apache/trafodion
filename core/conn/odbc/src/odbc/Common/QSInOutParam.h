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

#ifndef QSINOUTPARAM_DEFINED
#define QSINOUTPARAM_DEFINED

#include "sqlcli.h"
#include "QSData.h"

#define	SPEC_QUERY_INIT 			0x00000000
#define SPEC_QUERY_SPECIAL			0x00000001
#define SPEC_QUERY_HPDM				0x00000002

#define SPEC_QUERY_IMMEDIATE		(SPEC_QUERY_SPECIAL | SPEC_QUERY_HPDM)

typedef struct tagIN_AGGR
{
	  tagIN_AGGR() :
			  EstimatedRowsAccessed(0.0)
			, EstimatedRowsUsed(0.0)
			, StatsBytes(0)
			, NumRowsIUD(0)
			, RowsReturned(0)
			, SQLProcessBusyTime(0)
			, AQRnumRetries(0)
			, AQRdelayBeforeRetry(0)
			, NumberOfRows(0)
			, OpenBusyTime(0)
			, NumOpens(0)
			, ProcessesCreated(0)
			, ProcessCreateBusyTime(0)
			, RowsAccessed(0)
			, RowsRetrieved(0)
			, DiscProcessBusyTime(0)
			, DiscReads(0)
			, SpaceTotal(0)
			, SpaceUsed(0)
			, HeapTotal(0)
			, HeapUsed(0)
			, TotalMemory(0)
			, Dp2SpaceTotal(0)
			, Dp2SpaceUsed(0)
			, Dp2HeapTotal(0)
			, Dp2HeapUsed(0)
			, MsgsToDisc(0)
			, MsgsBytesToDisc(0)
			, NumRqstMsgs(0)
			, NumRqstMsgBytes(0)
			, NumRplyMsgs(0)
			, NumRplyMsgBytes(0)
			, LockWaits(0)
			, LockEscalation(0)
			, TotalExecutes(0) 
			, TotalAggregates(0)
			, ScratchFileCount(0)
			, ScratchBufferBlockSize(0)
			, ScratchBufferBlocksRead(0)
			, ScratchBufferBlocksWritten(0)
			, ScratchOverflowMode(0)
			, ScratchBufferReadCount(0)
			, ScratchBufferWriteCount(0)
			  {}

	void reset()
	{
		EstimatedRowsAccessed	= 0.0;
		EstimatedRowsUsed		= 0.0;
		StatsBytes				= 0;
		NumRowsIUD				= 0;
		RowsReturned			= 0;
		SQLProcessBusyTime		= 0;
		AQRnumRetries			= 0;
		AQRdelayBeforeRetry		= 0;
		NumberOfRows			= 0;
		OpenBusyTime			= 0;
		NumOpens				= 0;
		ProcessesCreated		= 0;
		ProcessCreateBusyTime	= 0;
		RowsAccessed			= 0;
		RowsRetrieved			= 0;
		DiscProcessBusyTime		= 0;
		DiscReads				= 0;
		SpaceTotal				= 0;
		SpaceUsed				= 0;
		HeapTotal				= 0;
		HeapUsed				= 0;
		TotalMemory				= 0;
		Dp2SpaceTotal			= 0;
		Dp2SpaceUsed			= 0;
		Dp2HeapTotal			= 0;
		Dp2HeapUsed				= 0;
		MsgsToDisc				= 0;
		MsgsBytesToDisc			= 0;
		NumRqstMsgs				= 0;
		NumRqstMsgBytes			= 0;
		NumRplyMsgs				= 0;
		NumRplyMsgBytes			= 0;
		LockWaits				= 0;
		LockEscalation			= 0;
		TotalExecutes			= 0; 
		TotalAggregates			= 0;

		ScratchFileCount		= 0;
		ScratchBufferBlockSize	= 0;
		ScratchBufferBlocksRead	= 0;
		ScratchBufferBlocksWritten = 0;
		ScratchOverflowMode		= 0;
		ScratchBufferReadCount = 0;
		ScratchBufferWriteCount = 0;
	}

	tagIN_AGGR& operator=(const tagIN_AGGR& rhs) 
	{
		EstimatedRowsAccessed	= rhs.EstimatedRowsAccessed;
		EstimatedRowsUsed		= rhs.EstimatedRowsUsed;
		StatsBytes				= rhs.StatsBytes;
		NumRowsIUD				= rhs.NumRowsIUD;
		RowsReturned			= rhs.RowsReturned;
		SQLProcessBusyTime		= rhs.SQLProcessBusyTime;
		AQRnumRetries			= rhs.AQRnumRetries;
		AQRdelayBeforeRetry		= rhs.AQRdelayBeforeRetry;
		NumberOfRows			= rhs.NumberOfRows;
		OpenBusyTime			= rhs.OpenBusyTime;
		NumOpens				= rhs.NumOpens;
		ProcessesCreated		= rhs.ProcessesCreated;
		ProcessCreateBusyTime	= rhs.ProcessCreateBusyTime;
		RowsAccessed			= rhs.RowsAccessed;
		RowsRetrieved			= rhs.RowsRetrieved;
		DiscProcessBusyTime		= rhs.DiscProcessBusyTime;
		DiscReads				= rhs.DiscReads;
		SpaceTotal				= rhs.SpaceTotal;
		SpaceUsed				= rhs.SpaceUsed;
		HeapTotal				= rhs.HeapTotal;
		HeapUsed				= rhs.HeapUsed;
		TotalMemory				= rhs.TotalMemory;
		Dp2SpaceTotal			= rhs.Dp2SpaceTotal;
		Dp2SpaceUsed			= rhs.Dp2SpaceUsed;
		Dp2HeapTotal			= rhs.Dp2HeapTotal;
		Dp2HeapUsed				= rhs.Dp2HeapUsed;
		MsgsToDisc				= rhs.MsgsToDisc;
		MsgsBytesToDisc			= rhs.MsgsBytesToDisc;
		NumRqstMsgs				= rhs.NumRqstMsgs;
		NumRqstMsgBytes			= rhs.NumRqstMsgBytes;
		NumRplyMsgs				= rhs.NumRplyMsgs;
		NumRplyMsgBytes			= rhs.NumRplyMsgBytes;
		LockWaits				= rhs.LockWaits;
		LockEscalation			= rhs.LockEscalation;
		TotalExecutes			= rhs.TotalExecutes; 
		TotalAggregates			= rhs.TotalAggregates;

		ScratchFileCount		= rhs.ScratchFileCount;						//AGGREG
		ScratchBufferBlockSize	= rhs.ScratchBufferBlockSize;				//FIRST
		ScratchBufferBlocksRead	= rhs.ScratchBufferBlocksRead;				//AGGREG
		ScratchBufferBlocksWritten = rhs.ScratchBufferBlocksWritten;		//AGGREG
		ScratchBufferReadCount	= rhs.ScratchBufferReadCount;				//AGGREG
		ScratchBufferWriteCount = rhs.ScratchBufferWriteCount;				//AGGREG
		ScratchOverflowMode		= rhs.ScratchOverflowMode;					//FIRST

		return *this;
	}


	int64 LastUpdate;

	double EstimatedRowsAccessed;	//AGGREG	estRowsAccessed 
	double EstimatedRowsUsed;		//AGGREG	estRowsUsed
	int64 StatsBytes;				//AGGREG	statsBytes
	int64 NumRowsIUD;				//AGGREG
	int64 RowsReturned;				//AGGREG	rowsReturned
	int64 SQLProcessBusyTime;		//AGGREG	ProcessBusyTime
	int64 AQRnumRetries;			//AGGREG	AQRnumRetries
	int64 AQRdelayBeforeRetry;		//AGGREG	AQRdelayBeforeRetry
	int64 NumberOfRows;				//AGGREG	numberOfRows
	int64 OpenBusyTime;				//AGGREG	OpenTime
	int64 NumOpens;					//AGGREG	Opens
	int64 ProcessesCreated;			//AGGREG	NewProcess
	int64 ProcessCreateBusyTime;	//AGGREG	NewProcessTime
	int64 RowsAccessed;				//AGGREG	AccessedRows
	int64 RowsRetrieved;			//AGGREG	UsedRows
	int64 DiscProcessBusyTime;		//AGGREG	DiskProcessBusyTime
	int64 DiscReads;				//AGGREG	DiskIOs
	int64 SpaceTotal;				//AGGREG	SpaceTotal
	int64 SpaceUsed;				//AGGREG	SpaceUsed
	int64 HeapTotal;				//AGGREG	HeapTotal
	int64 HeapUsed;					//AGGREG	HeapUsed
	int64 TotalMemory;				//AGGREG	TotalMemAlloc
	int64 Dp2SpaceTotal;			//AGGREG	Dp2SpaceTotal 
	int64 Dp2SpaceUsed;				//AGGREG	Dp2SpaceUsed
	int64 Dp2HeapTotal;				//AGGREG	Dp2HeapTotal
	int64 Dp2HeapUsed;				//AGGREG	Dp2HeapUsed
	int64 MsgsToDisc;				//AGGREG	NumMessages
	int64 MsgsBytesToDisc;			//AGGREG	MessagesBytes
	int64 NumRqstMsgs;				//AGGREG	reqMsgCnt
	int64 NumRqstMsgBytes;			//AGGREG	reqMsgBytes 
	int64 NumRplyMsgs;				//AGGREG	replyMsgCnt
	int64 NumRplyMsgBytes;			//AGGREG	replyMsgBytes
	int64 LockWaits;				//AGGREG	LockWaits
	int64 LockEscalation;			//AGGREG	Escalations
	int64 TotalExecutes;			//AGGREG	totalStatementExecutes
	int64 TotalAggregates;

	int64 ScratchFileCount;						//AGGREG
	int64 ScratchBufferBlockSize;				//FIRST
	int64 ScratchBufferBlocksRead;				//AGGREG
	int64 ScratchBufferBlocksWritten;			//AGGREG
	short ScratchOverflowMode;					//FIRST
	int64 ScratchBufferReadCount;				//AGGREG
	int64 ScratchBufferWriteCount;				//AGGREG

} IN_AGGR;

typedef struct tagIN_PARAM
{
	  tagIN_PARAM() : IN_PARAM_length(sizeof(struct tagIN_PARAM))
		  , pQuery(0)
		  , stmtHandle(0)
		  , adaptive_segment(0)
		  , query_type(0)
		  , query_trace(0)
		  , mute(0)
		  , ext_21036(0)
		  , query_start_time(0)
		  , planLen(0)
		  , planOffset(0)
		  , textLen(0)
		  , textOffset(0)
		  , supp_request(0)
		  , internal_Id(0)
		  , allocatedResources(0)
	      , reason(0)
		  , ruleId(0)
		  , exprNo(0)
		  , aggr_query_types(0)
		  , aggr_repos_interval(0)
		  , aggr_wms_interval(0)
		  , aggr_exec_interval(0)
		  , aggr_stats_once(0)
		  , port(0)
		  , baggregate_query(false)
		  , CompStartTime(0)
		  , CompEndTime(0)
		  , CompTime(0)
		  , StatementType(0)
		  , priority(0)
		  , cpu(0)
		  , process_id(0)
		  , querySpecial(QUERY_INIT)


				{			bzero(&cost_info,sizeof(cost_info));
							bzero(&comp_stats_info,sizeof(comp_stats_info));
							bzero(ds_name,sizeof(ds_name));
							bzero(user_name,sizeof(user_name));
							bzero(service_name,sizeof(service_name));
							bzero(query_name,sizeof(query_name));
							bzero(queryID,sizeof(queryID));
							bzero(sessionId,sizeof(sessionId));
							bzero(statementId,sizeof(statementId));
							bzero(sqlWarningOrError,sizeof(sqlWarningOrError));
							bzero(user,sizeof(user));
							bzero(tcpProcessName,sizeof(tcpProcessName));
							bzero(rsCollector,sizeof(rsCollector));
							bzero(stmtName,sizeof(stmtName));
							bzero(ParentQueryId,sizeof(ParentQueryId));
							bzero(ruleName, sizeof(ruleName));
							bzero(segmentName, sizeof(segmentName));
							bzero(dbuser_name,sizeof(dbuser_name));

				}
  	  unsigned long supp_request;
	  long  internal_Id;
	  char sqlWarningOrError[MAX_SQL_ERROR_TEXT + 1];
	  char rsCollector[EXT_FILENAME_LEN];

	  Long  pQuery;
	  Long	stmtHandle;
	  SQL_QUERY_COST_INFO cost_info;
	  SQL_QUERY_COMPILER_STATS_INFO comp_stats_info;
	  short adaptive_segment;
	  char ds_name[MAX_DSOURCE_NAME + 1];
	  char user_name[MAX_USERNAME_LEN + 1];
	  char dbuser_name[MAX_USERNAME_LEN + 1];
	  char service_name[MAX_SERVICE_NAME_LEN + 1];
	  char query_name[MAX_QUERYNAME_LEN + 1];
	  char user[MAX_USERNAME_LEN + 1];
	  IDL_char queryID[MAX_QUERY_ID_LEN];
	  IDL_long query_type;
	  bool query_trace;
	  bool mute;
	  bool ext_21036;
	  int64 query_start_time;

	  char sessionId[SESSION_ID_LEN];
	  char statementId[MAX_NAME_LEN];
	  IDL_long planLen;
	  Long planOffset;
	  IDL_long textLen;
	  Long textOffset;
	  int64 allocatedResources;
//
// Warn and Aggregation info from CONN rules
//
		short	aggr_query_types; //AGGREGATION
		short	aggr_repos_interval;
		short	aggr_wms_interval;
		short	aggr_exec_interval;
		short	aggr_stats_once;		// perf
//
		unsigned short reason;
		unsigned short ruleId;
		unsigned short exprNo;
		char ruleName[MAX_RULE_NAME_LEN + 1];
//
// TCPIP info about connection with client
//
	  char tcpProcessName[20];
	  unsigned short port;
//
// fir 254 bytes of the query
//
	  char shortQueryText[RMS_STORE_SQL_SOURCE_LEN + 1];
	  long	rmsSqlSourceLen;
//
// AGGREGATION
//
	  bool baggregate_query;

	  int64 CompStartTime;
	  int64 CompEndTime;
	  int64 CompTime;
	  char stmtName[MAX_NAME_LEN];
	  long StatementType;
	  char ParentQueryId[MAX_QUERY_ID_LEN];
//
	  char	segmentName[128];
	  TCPU_DECL(cpu);
	  long	process_id;
	  short	priority;

	  unsigned long querySpecial;
//
// Mgr2 data
//
  	  short IN_PARAM_length;
} IN_PARAM;

typedef struct tag_PLAN_TEXT
{
	  char *plan;
	  char *text;
} IN_PLAN_TEXT;

typedef struct tagOUT_PARAM
{
	  tagOUT_PARAM() : OUT_PARAM_length(sizeof(struct tagOUT_PARAM))
		  , supp_request(0)
		  , internal_Id(0)
	  	  , aggPriority(0)
		  , maxRowsFetched(0)
		  , rule_sql_cmds_len(0)
		  , m_state(0)
		  , m_wait_time(0)
		  , m_hold_time(0)
		  , m_suspended_time(0)
		  , m_exec_time(0)
		  , m_warnLevel(0)
		  ,	m_WMSstart_ts(0)
		  , m_maxMemUsed(0)
		  , sql_cmds_len(0)
							{	bzero(sql_cmds,sizeof(sql_cmds));
								bzero(rule_sql_cmds,sizeof(rule_sql_cmds));
								bzero(m_con_rule_name,sizeof(m_con_rule_name));
								bzero(m_cmp_rule_name,sizeof(m_cmp_rule_name));
								bzero(m_exe_rule_name,sizeof(m_exe_rule_name));
							}
		  

	unsigned long supp_request;
	long  internal_Id;
	short aggPriority;
	int64 maxRowsFetched;
	char sql_cmds[MAX_SQL_CMD_LEN];
	long sql_cmds_len;
	bool sqlPlan;
	bool sqlText;
	char rule_sql_cmds[MAX_SQL_CMD_LEN+1];
	long rule_sql_cmds_len;
//
// Added for "Single Row Per Query" project
//
	unsigned short	m_state;		//state + susbstate
	int64			m_wait_time;	//in seconds
	int64			m_hold_time;	//in seconds
	int64			m_suspended_time;//in seconds
	int64			m_exec_time;	//in seconds
	int64			m_WMSstart_ts;	//timestamp
	unsigned short	m_warnLevel;
	unsigned long	m_maxMemUsed;
//
// rules - size is MAX_RULE_NAME_LEN + ':' + '99' or 'ALL' + '\0'
//
	char			m_con_rule_name[MAX_RULE_NAME_LEN + 1 + 3 + 1];
	char			m_cmp_rule_name[MAX_RULE_NAME_LEN + 1 + 3 + 1];
	char			m_exe_rule_name[MAX_RULE_NAME_LEN + 1 + 3 + 1];

	//int64 m_suspended_ts;	//3289
	//int64 m_released_ts;
	//int64 m_cancelled_ts;

	short OUT_PARAM_length;

} OUT_PARAM;

//---------------------------- for query finished message

enum SUB_CMD { FINISH_QUERY = 1, UPDATE_AGGR, FINISH_AGGR };

typedef struct tagIN_FPARAM
{
	long	internal_Id;
	short 	aggPriority;
	Long stmtHandle;
;
	SUB_CMD		m_sub_command;
//
// AGGREGATION
//
	int64 m_aggExecuteStartTime;
	int64 m_aggExecuteEndTime;
	int64 m_aggFirstResultReturnTime;
	int64 m_aggTransactionId;
	short m_aggState;
	short m_aggSqlErrorCode;
	int64 m_sqlCpuTime;
	long  m_aggNumSqlProcs;
	long  m_aggExePriority;
	short m_aggStatsErrorCode;
	short m_aggLastErrorBeforeAQR;
	int64 m_aggMaxMemUsed;
	IN_AGGR m_aggr;

	NDCS_SUBSTATE	m_mxsrvr_substate;
//

} IN_FPARAM;

typedef struct tagOUT_FPARAM
{
	Long  pQuery;
	int64 timestamp_conn;
//
// Added for "Single Row Per Query" project
//
	unsigned short	m_state;		//state + susbstate
	int64			m_wait_time;	//in seconds
	int64			m_hold_time;	//in seconds
	int64			m_suspended_time;//in seconds
	int64			m_exec_time;	//in seconds
	int64			m_WMSstart_ts;	//timestamp
	unsigned short	m_warnLevel;
	unsigned long	m_maxMemUsed;
//
	unsigned short	m_queryCanceled;
//
	bool			m_pertable_stats;	// true - WMS wrote perTableStats for this query
//
// rules - size is MAX_RULE_NAME_LEN + ':' + '99' or 'ALL' + '\0'
//
	char			m_con_rule_name[MAX_RULE_NAME_LEN + 1 + 3 + 1];
	char			m_cmp_rule_name[MAX_RULE_NAME_LEN + 1 + 3 + 1];
	char			m_exe_rule_name[MAX_RULE_NAME_LEN + 1 + 3 + 1];

	int64			m_suspended_ts;
	int64			m_released_ts;
	int64			m_cancelled_ts;

} OUT_FPARAM;

//-------------------------------------------------------------------

typedef struct tagIN_GETSERVICE
{
	char service_name[MAX_SERVICE_NAME_LEN + 1];
} IN_GETSERVICE;

typedef struct tagOUT_GETSERVICE
{
	char service_name[MAX_SERVICE_NAME_LEN + 1];
	bool sqlPlan;
	bool sqlText;
	char sql_cmds[MAX_SQL_CMD_LEN];
	long sql_cmds_len;
	short master_prty;
	int64 timestamp_conn;

} OUT_GETSERVICE;

//-----------------------------------------------------------------

typedef struct tagIN_LOAD_CONN_RULES
{
  int64 timestamp;

} IN_LOAD_CONN_RULES;

typedef struct tagOUT_LOAD_CONN_RULES
{
  int64 timestamp;
  long  connRulesLength;
  void* pConnRules;

} OUT_LOAD_CONN_RULES;

#endif
