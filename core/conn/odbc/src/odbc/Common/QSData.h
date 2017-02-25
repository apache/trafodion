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

#ifndef QSDATA_H
#define QSDATA_H

#include <string>
using namespace std;

#include "QSGlobal.h"

//====================================================================
 /*
 * Exception for
 * operation 'Query Service'
 */
struct qrysrvc_exc_ {
	IDL_long exception_nr;
	IDL_long exception_detail;
	IDL_long exception_error;
	IDL_char exception_txt[100];
};

//=============== aggregation ===========================
#define AGGR_QT_INIT       0x00000000 
#define AGGR_QT_INSERT     0x00000001 
#define AGGR_QT_UPDATE     0x00000002 
#define AGGR_QT_DELETE     0x00000004
#define AGGR_QT_SELECT     0x00000008 
#define AGGR_QT_NO_ACTION  0x00000010 


//=============== trace =================================
#define TRACE_INIT		0x00000000
#define TRACE_QS		0x00000001
#define TRACE_STATS		0x00000002
#define TRACE_COM		0x00000004
#define TRACE_SYNC		0x00000008
#define TRACE_SYNC_AS	0x00000010
#define TRACE_ELAPSED	0x00000040
#define TRACE_RULE		0x00000080
#define TRACE_OFFND		0x00000100
#define TRACE_ALL	(TRACE_QS | TRACE_STATS | TRACE_COM | TRACE_SYNC | TRACE_SYNC_AS | TRACE_ELAPSED | TRACE_RULE | TRACE_OFFND)

#define DEFAULT_TRACE_FILE	"QSTRACE"
#define DEFAULT_TRACE_PATH	""
#define DEFAULT_LOG_FILE	"QSLOG"

#define DEFAULT_SYNC_LOG_FILE	"QSLSYN"
#define DEFAULT_SCHEMA 			"NWMS_SCHEMA"

#ifndef _DEBUG
#define	CANCEL_COM_TIMEOUT		((long long)(5*60*1000000LL))	// 5 min - when com does not respond - qs serverice will kill the process
#define	CANCEL_STATS_TIMEOUT	((long long)(5*60*1000000LL))	// 5 min - when stats does not respond - qs serverice will kill the process
#define	CANCEL_RULE_TIMEOUT		((long long)(5*60*1000000LL))	// 5 min - when stats does not respond - qs serverice will kill the process
#define CLIENT_DISCONNECT_TIMEOUT ((long long)(1*60*1000000LL)) // 1 min - cancel query when client disapeared
#else
//LCOV_EXCL_START
// in debug mode we can have longer timeouts
#define	CANCEL_COM_TIMEOUT		((long long)(10*60*1000000LL))	// 10 min - when com does not respond - qs serverice will kill the process
#define	CANCEL_STATS_TIMEOUT	((long long)(10*60*1000000LL))	// 10 min - when stats does not respond - qs serverice will kill the process
#define	CANCEL_RULE_TIMEOUT		((long long)(10*60*1000000LL))	// 10 min - when stats does not respond - qs serverice will kill the process
#define CLIENT_DISCONNECT_TIMEOUT ((long long)(1*60*1000000LL)) // 1 min - cancel query when client disapeared
//LCOV_EXCL_STOP
#endif

//=============== Query Service description =======================

#define MAX_EXEC_QUERIES	10
#define DEFAULT_DELTA_EXEC_QUERIES	2


#define DEFAULT_WMS_NUMBER		4
#define DEFAULT_MAX_WMS_NUMBER	16
#define DEFAULT_WMS_LAYOUT		(unsigned char*)"01020304"

#define DEFAULT_MAX_QUERIES		2000
#define DEFAULT_INTERVAL		5	// 5 seconds
#define DEFAULT_RULE_INTERVAL	60	// 60 seconds
#define DEFAULT_OFFND_INTERVAL	5	// 5 seconds
#define DEFAULT_QUERY_COMPLETED	500
#define DEFAULT_COMPLETED_TIMEOUT	((long long)(30*1000000LL)) // 30 seconds
#define DEFAULT_RULE_TIME			((long long)(300*1000000LL)) // 5 minutes
#define DEFAULT_SSD_TIMEOUT			((long long)(30*1000000LL)) // 30 seconds

#define DEFAULT_QUEUENAME		"QUEUE"

#define DEFAULT_CPU_BUSY		100

// Currently for SQ we default to 100%. Needs to be decided.
#define DEFAULT_MEM_USAGE		100

#define DEFAULT_DP2_USAGE		80

#define DEFAULT_GLOBAL_STATS_RATE	30		// 1 minute
#define DEFAULT_GLOBAL_STATS_SIZE	10
#define DEFAULT_GLOBAL_STATS_INTERVAL	((long long)(5*1000000LL)) // 5 seconds

#define DEFAULT_MAX_RULES		1000

#define MAX_PLAN_LEN			8192
#define MAX_TEXT_LEN			4096
#define MAX_COL_LEN				3800
#define MAX_PROCESS_LEN			50

#define MAX_CONN_SIZE			1000	// max number of selected connection items
#define MAX_CONN_STRING			500		// max string length in connection expression
#define MAX_RULE_SIZE			100		// max number of rules in one service
#define MAX_RULE_SERVICES		500		// one rule can be used in so many services
#define MAX_EXPR_SIZE			15		// max number of expressions in one rule item
#define MAX_SQL_ERROR_TEXT		200
#define MAX_LIST_SIZE			_max(MAX_CONN_SIZE, MAX_RULE_SIZE)

#define MSG_CACHE				50
#define OFFENDER_CACHE			100
#define MAX_CPU_SEGMENT			16

//-------------------------- Transaction ------------------
#define MAX_TXN_QUERIES 10
#define MAX_NUM_OFFENDER_TRANSACTIONS 10

//-------------------------- Support ------------------
#define SUPP_LOAD		0x0001
#define SUPP_REPREAPRE	0x0002
#define SUPP_EXECUTE	0x0004
#define SUPP_RELEASE_ESPS 0x0008

//-------------------------- Global ---------------------------
typedef enum _GLOBAL_STATE
{
	GLOBAL_INIT = 0,
	GLOBAL_ACTIVE,
	GLOBAL_HOLD,
	GLOBAL_SHUTDOWN,
	GLOBAL_STOPPED,
} GLOBAL_STATE;

//-------------------------- Queue -----------------------------
typedef enum _QUEUE_PRTY
{
	QPRTY_INIT = 0,
	QPRTY_URGENT,
	QPRTY_HIGH,
	QPRTY_MEDIUM_HIGH,
	QPRTY_MEDIUM,
	QPRTY_LOW_MEDIUM,
	QPRTY_LOW,
} QUEUE_PRTY;

//-------------------------- Process --------------------------
typedef enum _PROCESS_PRTY
{
	PPRTY_INIT = 0,
	PPRTY_URGENT = 181,
	PPRTY_HIGH = 140,
	PPRTY_MEDIUM_HIGH = 125,
	PPRTY_MEDIUM = 110,
	PPRTY_LOW_MEDIUM = 95,
	PPRTY_LOW = 80,
} PROCESS_PRTY;

typedef struct _QUEUE_PROCESS_PRIORITY
{
	QUEUE_PRTY	 queue_priority;
	PROCESS_PRTY process_priority;
} QUEUE_PROCESS_PRIORITY;

typedef enum _QUEUE_WGHT
{
	QWGHT_INIT = 0,
	QWGHT_URGENT = 6,
	QWGHT_HIGH = 5,
	QWGHT_MEDIUM_HIGH = 4,
	QWGHT_MEDIUM = 3,
	QWGHT_LOW_MEDIUM = 2,
	QWGHT_LOW = 1,
} QUEUE_WGHT;

typedef enum _QUEUE_STATE
{
	QUEUE_INIT = 0,
	QUEUE_ACTIVE,
	QUEUE_HOLD,
	QUEUE_QUIESCE,
	QUEUE_STOPPING,
	QUEUE_STOPPED,
	QUEUE_TRANSIENT,//COM uses to denote that segment states don't agree
	QUEUE_LAST
} QUEUE_STATE;

//-------------------------- Query ---------------------------
typedef enum _QUERY_STATE
{
	QUERY_INIT							= 0,
//
	QUERY_WAITING						= 0x0100,
	QUERY_WAITING_MAX_CPU_BUSY			= 0x0101,
	QUERY_WAITING_MAX_MEM_USAGE			= 0x0102,
	QUERY_WAITING_RELEASED_BY_ADMIN		= 0x0103,
	QUERY_WAITING_MAX_INSTANCE_EXEC_QUERIES = 0x0104,
	QUERY_WAITING_TXN_BACKOUT			= 0x0105,
	QUERY_WAITING_MAX_SSD_USAGE		    = 0x0106, //ssd overflow
	QUERY_WAITING_MAX_ESPS				= 0x0107,
	QUERY_WAITING_EST_MAX_CPU_BUSY		= 0x0108,
	QUERY_WAITING_MAX_SERVICE_EXEC_QUERIES  = 0x0109,
	QUERY_WAITING_CANARY_EXEC              = 0x010A,
//
	QUERY_EXECUTING						= 0x0200,
	QUERY_EXECUTING_RELEASED_BY_ADMIN	= 0x0201,
	QUERY_EXECUTING_RELEASED_BY_RULE	= 0x0202,
	QUERY_EXECUTING_CANCEL_IN_PROGRESS	= 0x0203,
	QUERY_EXECUTING_CANCEL_FAILED		= 0x0204,
	QUERY_EXECUTING_CANCEL_FAILED_8026	= 0x0205,
	QUERY_EXECUTING_CANCEL_FAILED_8027	= 0x0206,
	QUERY_EXECUTING_CANCEL_FAILED_8028	= 0x0207,
	QUERY_EXECUTING_CANCEL_FAILED_8029	= 0x0208,
	QUERY_EXECUTING_CANCEL_FAILED_8031	= 0x0209,
//
	QUERY_HOLDING						= 0x0400,
	QUERY_HOLDING_LOAD					= 0x0401,		
	QUERY_HOLDING_REPREPARING			= 0x0402,
	QUERY_HOLDING_EXECUTING_SQL_CMD		= 0x0403,
	QUERY_HOLDING_BY_RULE				= 0x0404,
	QUERY_HOLDING_BY_ADMIN				= 0x0405,
//
	QUERY_COMPLETED						= 0x0800,
	QUERY_COMPLETED_HOLD_TIMEOUT		= 0x0801,
	QUERY_COMPLETED_EXEC_TIMEOUT		= 0x0802,
	QUERY_COMPLETED_BY_ADMIN			= 0x0803,
	QUERY_COMPLETED_QUERY_NOT_FOUND		= 0x0804,
	QUERY_COMPLETED_CONNECTION_FAILED	= 0x0805,
	QUERY_COMPLETED_NDCS_PROCESS_FAILED	= 0x0806,
	QUERY_COMPLETED_CPU_FAILED			= 0x0807,
	QUERY_COMPLETED_SEGMENT_FAILED		= 0x0808,
	QUERY_COMPLETED_BY_RULE				= 0x0809,
	QUERY_COMPLETED_SERVICE_NOT_ACTIVE	= 0x080A,
	QUERY_COMPLETED_HARDWARE_FAILURE	= 0x080B,
	QUERY_COMPLETED_UNEXPECTED_STATE	= 0x080C,
	QUERY_COMPLETED_CLIENT_DISAPPEARED	= 0x080D,
	QUERY_COMPLETED_BY_CLIENT			= 0x080E,
//
	QUERY_COMPLETED_NDCS_DLG_INIT		= 0x080F,
	QUERY_COMPLETED_NDCS_CONN_IDLE		= 0x0810,
	QUERY_COMPLETED_NDCS_DLG_TERM		= 0x0811,
	QUERY_COMPLETED_NDCS_DLG_BREAK		= 0x0812,
	QUERY_COMPLETED_NDCS_STOP_SRVR		= 0x0813,
	QUERY_COMPLETED_NDCS_RMS_ERROR		= 0x0814,
	QUERY_COMPLETED_NDCS_REPOS_IDLE		= 0x0815,
	QUERY_COMPLETED_NDCS_REPOS_INTERVAL	= 0x0816,
	QUERY_COMPLETED_NDCS_REPOS_PARTIAL	= 0x0817,
	QUERY_COMPLETED_NDCS_EXEC_INTERVAL	= 0x0818,
	QUERY_COMPLETED_NDCS_CONN_RULE_CHANGED = 0x0819,
	QUERY_COMPLETED_NDCS_CLOSE			= 0x081A,
	QUERY_COMPLETED_NDCS_PREPARE		= 0x081B,
	QUERY_COMPLETED_NDCS_WMS_ERROR		= 0x081C,
	QUERY_COMPLETED_BY_ADMIN_SERVER     = 0x081D,
//
	QUERY_REJECTED						= 0x1000,
	QUERY_REJECTED_BY_ADMIN				= 0x1001,
	QUERY_REJECTED_CONNECTION_FAILED	= 0x1002,
	QUERY_REJECTED_NDCS_PROCESS_FAILED	= 0x1003,
	QUERY_REJECTED_CPU_FAILED			= 0x1004,
	QUERY_REJECTED_SEGMENT_FAILED		= 0x1005,
	QUERY_REJECTED_QMSGCANCELLED		= 0x1006,
	QUERY_REJECTED_VERSION_MISMATCH		= 0x1007,
	QUERY_REJECTED_WMSONHOLD			= 0x1008,
	QUERY_REJECTED_MAX_QUERIES_REACHED	= 0x1009,
	QUERY_REJECTED_SERVICE_NOT_FOUND	= 0x100A,
	QUERY_REJECTED_SERVICE_ON_HOLD		= 0x100B,
	QUERY_REJECTED_BY_RULE				= 0x100C,
	QUERY_REJECTED_UNKNOWNUSER			= 0x100D,
	QUERY_REJECTED_UNEXPECTED_STATE		= 0x100E,
	QUERY_REJECTED_HOLD_TIMEOUT			= 0x100F,
	QUERY_REJECTED_WAIT_TIMEOUT			= 0x1010,
	QUERY_REJECTED_CLIENT_DISAPPEARED	= 0x1011,
	QUERY_REJECTED_LONG_TRANS_ABORTING	= 0x1012,
//
	QUERY_SUSPENDED						= 0x2000,
	QUERY_SUSPENDED_BY_ADMIN			= 0x2001,
	QUERY_SUSPENDED_BY_RULE				= 0x2002,
	QUERY_SUSPENDED_CANCELED			= 0x2003,
	QUERY_SUSPENDED_CANCELED_BY_ADMIN	= 0x2004,
	QUERY_SUSPENDED_CANCELED_BY_RULE	= 0x2005,
	QUERY_SUSPENDED_CANCELED_BY_TIMEOUT	= 0x2007,
//
} QUERY_STATE;

//----------------------- slot ---------------------------------

typedef enum _SLOT_STATE
{
	SLOT_EMPTY = 0,
	SLOT_ALLOC,
} SLOT_STATE;

//----------------------- Threshold ---------------------------

typedef enum _THRE_TYPE
{
	THRE_TYPE_INIT = 0,
	THRE_TYPE_ELAPSED_TIME,
	THRE_TYPE_ESTIMATED_COST,
	THRE_TYPE_EXECUTION_TIME,
	THRE_TYPE_ROWS_ACCESSED,
	THRE_TYPE_ROWS_USED
		
} THRESHOLD_TYPE;

typedef enum _THRE_OPERAND
{
	THRE_OPERAND_INIT = 0,
	THRE_OPERAND_GREATERTHAN,
	THRE_OPERAND_LESSTHAN,
	THRE_OPERAND_EQUALTO
		
} THRE_OPERAND;


typedef enum _THRE_ACTION
{
	THRE_ACTION_INIT = 0,
	THRE_ACTION_COMMIT,
	THRE_ACTION_HOLD,
	THRE_ACTION_CONTINUE,
	THRE_ACTION_PRIORITY,
	THRE_ACTION_ROLLBACK,
	THRE_ACTION_STOP
		
} THRE_ACTION;

typedef enum _THRE_ACTION_ARGUMENT
{
	THRE_ACTION_ARGUMENT_INIT = 0,
	THRE_ACTION_ARGUMENT_MINUS,
	THRE_ACTION_ARGUMENT_PLUS,
	THRE_ACTION_ARGUMENT_VALUE
		
} THRE_ACTION_ARG;

//----------------------- COM and STATS Command set ---------------------------
typedef enum _COM_EXEC
{
	COM_EXEC_INIT = 0,
	COM_EXEC_ONCE,				// Execute command only once (no matter on which segment)
	COM_EXEC_ALL,				// Execute command on all segments
	COM_EXEC_SEGMENT,			// Execute command only on segment from QID
	COM_EXEC_MODIFY_METADATA,	// Execute command on all segments. If error on first segment - stop
	COM_EXEC_BROADCAST,			// Like MODIFY_METADATA but different name
	COM_EXEC_FROM_COMPLETED,
	COM_EXEC_FROM_FILE

} COM_EXEC;

typedef enum _COM_OPERATION
{
	COM_OPER_INIT = 0,
	COM_OPER_STATUS_QUERIES_ALL,							//1
	COM_OPER_STATUS_QUERIES_EXECUTING,						//2
	COM_OPER_STATUS_QUERIES_WAITING,						//3
	COM_OPER_STATUS_QUERIES_HOLDING,						//4
	COM_OPER_STATUS_QUERIES_SERVICE_ALL,					//5
	COM_OPER_STATUS_QUERIES_SERVICE_EXECUTING,				//6
	COM_OPER_STATUS_QUERIES_SERVICE_WAITING,				//7
	COM_OPER_STATUS_QUERIES_SERVICE_HOLDING,				//8
	COM_OPER_STATUS_QUERY,									//9
	COM_OPER_STATUS_QUERY_STATS,							//10
	COM_OPER_STATUS_QUERY_ALL_STATS,						//11
	COM_OPER_STATUS_QUERY_PLAN,								//12
	COM_OPER_STATUS_QUERY_TEXT,								//13
	COM_OPER_STATUS_QUERIES_HOLD,							//14
	COM_OPER_STATUS_QUERIES_ROLE_ALL,						//15
	COM_OPER_STATUS_QUERIES_ROLE_EXECUTING,					//16
	COM_OPER_STATUS_QUERIES_ROLE_WAITING,					//17
	COM_OPER_STATUS_QUERIES_ROLE_HOLDING,					//18
	COM_OPER_STATUS_QUERIES_CARDINALITY,					//19
	COM_OPER_STATUS_QUERIES_IO_TIME,						//20
	COM_OPER_STATUS_QUERIES_TOTAL_TIME,						//21
	COM_OPER_STATUS_QUERIES_ELAPSED_TIME,					//22
	COM_OPER_STATUS_QUERIES_NODE,							//23
	COM_OPER_STATUS_QUERIES_SEGMENT,						//24 
	COM_OPER_STATUS_QUERIES_EXECUTING_STATS,				//25
	COM_OPER_STATUS_QUERY_MERGED,							//27
	COM_OPER_STATUS_QUERY_ALL_MERGED,						//28
	COM_OPER_STATUS_QUERY_STATS_PERTABLE,					//29
	COM_OPER_STATUS_OFFENDER_CPU,							//30
	COM_OPER_STATUS_OFFENDER_MEMORY,						//31
	COM_OPER_STATUS_OFFENDER_RMS_CPU,						//32
	COM_OPER_STATUS_OFFENDER_RMS_MEMORY,					//33
	COM_OPER_STATUS_OFFENDER_RMS_DISK,						//34
	COM_OPER_STATUS_OFFENDER_CHILDREN,						//35
	COM_OPER_STATUS_OFFENDER_PROC_DETAIL,					//36
	COM_OPER_STATUS_OFFENDER_PSTATE,						//37
	COM_OPER_STATUS_QUERIES_LOGIN_ALL,						//38
	COM_OPER_STATUS_QUERIES_LOGIN_EXECUTING,				//39
	COM_OPER_STATUS_QUERIES_LOGIN_WAITING,					//40
	COM_OPER_STATUS_QUERIES_LOGIN_HOLDING,					//41
	COM_OPER_STATUS_QUERY_CHILDREN,                                         //42
	COM_OPER_STATUS_OFFENDER_TRANSACTION_TRANSID,     			
        COM_OPER_STATUS_OFFENDER_TRANSACTIONS,  
        COM_OPER_STATUS_OFFENDER_TRANSACTIONS_ABORTING, 

	//----------------------- COM->QS messaging Begin ---------------------------

	COM_OPER_CANCEL_QUERIES_ALL = 50,						//50
	COM_OPER_CANCEL_QUERIES_EXECUTING,						//51
	COM_OPER_CANCEL_QUERIES_WAITING,						//52
	COM_OPER_CANCEL_QUERIES_HOLDING,						//53
	COM_OPER_CANCEL_QUERIES_SERVICE_ALL,					//54
	COM_OPER_CANCEL_QUERIES_SERVICE_EXECUTING,				//55
	COM_OPER_CANCEL_QUERIES_SERVICE_WAITING,				//56
	COM_OPER_CANCEL_QUERIES_SERVICE_HOLDING,				//57
	COM_OPER_CANCEL_QUERY,									//58
	COM_OPER_HOLD_QUERIES_ALL,								//59
	COM_OPER_HOLD_QUERIES_SERVICE,							//60
	COM_OPER_HOLD_QUERY,									//61
	COM_OPER_RELEASE_QUERIES_ALL,							//62
	COM_OPER_RELEASE_QUERIES_SERVICE,						//63
	COM_OPER_RELEASE_QUERY,									//64
	COM_OPER_ALTPRI_QUERIES_ALL,							//65
	COM_OPER_ALTPRI_QUERIES_SERVICE,						//66
	COM_OPER_ALTPRI_QUERY,									//67
	COM_OPER_HOLD_QS,										//68
	COM_OPER_RELEASE_QS,									//69
	COM_OPER_HOLD_SERVICE_ALL,								//70
	COM_OPER_HOLD_SERVICE_NAME,								//71
	COM_OPER_RELEASE_SERVICE_ALL,							//72
	COM_OPER_RELEASE_SERVICE_NAME,							//73
	COM_OPER_DELETE_SERVICE,								//74
	COM_OPER_DELETE_SERVICE_IMMEDIATE,						//75
	COM_OPER_ADD_SERVICE,									//76
	COM_OPER_DELETE_QUERY,									//77
	//----------------------- COM->QS messaging End ---------------------------

	//----------------------- STATS->QS messaging Start -----------------------

	COM_OPER_QUERY_COMPLETED = 100,							//100
	COM_OPER_QUERY_COMPLETED_DELETE,						//101
	COM_OPER_QUERY_NOT_FOUND,								//102

	//----------------------- STATS->QS messaging End -------------------------

	COM_OPER_STATUS_WMS = 150,								//150
	COM_OPER_STATUS_WMS_VERSION,                            //151
	COM_OPER_STATUS_WMS_DETAIL,								//152
	COM_OPER_STATUS_SERVICE,								//153
	COM_OPER_STATUS_SERVICE_ALL,							//154
	COM_OPER_STATUS_SERVICE_STATS,							//155
	COM_OPER_STATUS_SERVICE_DETAIL,							//156
	COM_OPER_ALTER_WMS,										//157
	COM_OPER_ALTER_SERVICE,									//158
	COM_OPER_RESTART_WMS,									//162
	COM_OPER_STATUS_WMS_STATS,								//164
	COM_OPER_STATUS_SERVICE_STATS_DETAIL,					//165
	COM_OPER_STATUS_WMS_CHECK,								//166
	COM_OPER_STATUS_WMS_CHECK_DETAIL,						//167
	COM_OPER_STATUS_WMS_CHECK_METADATA,						//168
	COM_OPER_STATUS_WMS_SSD,								//169 - ssd overflow
	COM_OPER_STATUS_SERVICE_ALL_STATS,						//170 - internal command only for hpdm
//----------------------- Release 2.4 Rules -------------------------

	COM_OPER_STATUS_QUERIES_WARN = 200,						//200
	COM_OPER_STATUS_RULE,									//201
	COM_OPER_STATUS_RULE_ALL,								//202
	COM_OPER_STATUS_RULE_NAME,								//203
	COM_OPER_STATUS_RULE_CONN,								//204
	COM_OPER_STATUS_RULE_COMP,								//205
	COM_OPER_STATUS_RULE_EXEC,								//206
	COM_OPER_ADD_RULE_NAME,									//207
	COM_OPER_ALTER_RULE_NAME,								//208
	COM_OPER_DELETE_RULE_NAME,								//214
	COM_OPER_DELETE_RULE_NAME_FORCE,						//215
	COM_OPER_LOAD_QUERY_ALL,								//216
	COM_OPER_LOAD_QUERY_TEXT,								//217
	COM_OPER_LOAD_QUERY_PLAN,								//218
	COM_OPER_REPREPARE_QUERY,								//219
	COM_OPER_REPREPARE_QUERY_SQL_CMD,						//220
	COM_OPER_RELEASE_QUERY_SQLCMD,							//221
	COM_OPER_STATUS_SERVICE_COMP,							//226
	COM_OPER_STATUS_SERVICE_EXEC,							//227
	COM_OPER_STATUS_WMS_CONN,								//229
	COM_OPER_ALTER_WMS_CONN_RESET,							//230
	COM_OPER_ALTER_SERVICE_COMP_RESET,						//231
	COM_OPER_ALTER_SERVICE_EXEC_RESET,						//232
	COM_OPER_ALTER_WMS_RESET_STATS,							//233
	COM_OPER_ALTER_SERVICE_RESET_STATS,						//234
	COM_OPER_STATUS_QUERIES_WARN_LOW,						//235
	COM_OPER_STATUS_QUERIES_WARN_MEDIUM,					//236
	COM_OPER_STATUS_QUERIES_WARN_HIGH,						//237
	COM_OPER_STATUS_QUERY_WARN,			                    //238
	COM_OPER_STATUS_SERVICE_RULE,							//239
	COM_OPER_EXECUTE_QUERY_SQL_CMD,							//241
	COM_OPER_STOP_WMS,										//242
	COM_OPER_STOP_WMS_IMMEDIATE,							//243
	COM_OPER_START_WMS,										//244
	COM_OPER_STOP_SERVICE,									//245
	COM_OPER_STOP_SERVICE_IMMEDIATE,						//246
	COM_OPER_START_SERVICE,									//247

	//----------------------- RULE -QS messaging BEGIN -------------------------

	COM_OPER_QSRULE_CANCEL_QUERY = 500,						//500
	COM_OPER_QSRULE_WARN_QUERY, 							//501
	COM_OPER_QSRULE_DELETE_WARN_QUERY,						//502
	COM_OPER_QUERY_CLIENT_DISAPPEARED,						//503
	COM_OPER_QSRULE_STATS_PERTABLE_QUERY,					//504
	COM_OPER_QSRULE_STATS_PERTABLE_CANCEL_QUERY,			//505

} COM_OPERATION;

typedef enum _COM_OPERATION_ARG
{
	COM_OPER_ARG_INIT = 0,
	COM_OPER_ARG_PLUSPLUS,
	COM_OPER_ARG_MINUSMINUS 
		
} COM_OPERATION_ARG;
//
//======================================================================
//============= DEFINITIONS AND STRUCTURES IN RULE BLOCK ===============
//======================================================================
//
//==================== WARN LEVEL ======================================
//
#define WLVL_LOW		0x0001
#define WLVL_MEDIUM		0x0002
#define WLVL_HIGH		0x0004
#define WLVL_NO_WARN	0x0008
//
//===================== RULE TYPE ======================================
//
typedef enum _RULE_TYPE
{
	RLTYPE_INIT		= 0x0000,
	RLTYPE_CONN		= 0x0001,
	RLTYPE_COMP		= 0x0002,
	RLTYPE_EXEC		= 0x0004,

} RULE_TYPE;
//
//===================== GLOBAL OPERATOR ================================
//

typedef enum _RULE_SPEC_OPER
{
	RSOPR_NOP	= 0x0000,
	RSOPR_OR	= 0x0001,
	RSOPR_AND	= 0x0002,

} RULE_SPEC_OPER;
//
//===================== RULE OPERATOR =======================================
//
// = | >= | > | < | <= | <> | numeric-value% 
// 
typedef enum _RULE_EXPR_OPER 
{
   OPER_NOP		= 0x0000,		// no operator
   OPER_EQ		= 0x0001,		// equal
   OPER_GE		= 0x0002,		// greater or equal
   OPER_GT		= 0x0004,		// greater than
   OPER_LT		= 0x0008,		// less than
   OPER_LE		= 0x0010,		// less or equal
   OPER_NE		= 0x0020,		// not equal
   OPER_IC		= 0x0040,		// ignore case for CONN 
   OPER_PR		= 0x0800,		// operator %
   OPER_SC		= 0x1000,		// shortcut special operator for connection type

} RULE_EXPR_OPER;
//
//===================== RULE ACTION  =======================================
//
typedef enum _RULE_SPEC_ACT
{
	ACT_INIT		= 0x0000,
	ACT_WARN		= 0x0001,
	CONN_EXEC		= 0x0002,
	COMP_EXEC		= 0x0004,
	COMP_REJECT		= 0x0008,
	COMP_HOLD		= 0x0010,
	EXEC_CANCEL		= 0x0020,
	ACT_NO_WARN		= 0x0040,
	ACT_NO_ACTION	= 0x0080,
	CONN_AGGREGATE	= 0x0100,
	EXEC_STATS_PERTABLE = 0x0200,			// exec rule action: get Pertable statistics for the query
	EXEC_STATS_PERTABLE_CANCEL = 0x0400,	// exec rule action: get Pertable statistics and Cancel the query
	COMP_EXECUTE		= 0x0800,
	COMP_EXECUTE_NO		= 0x1000,
} RULE_SPEC_ACT;
//
//==================== RULE LEFT OPERAND ====================================
//
typedef enum _RULE_EXPR_LOPND
{
	LOPND_INIT					= 0x00000000,
			 /* CONNECTION */
	LOPND_APPL					= 0x01000001,
	LOPND_SESSION				= 0x01000002,
 	LOPND_LOGIN					= 0x01000004,
	LOPND_LOGIN_ROLE			= 0x01010004,
	LOPND_LOGIN_USER			= 0x01020004,
	LOPND_DSN					= 0x01000008,
             /* COMP */      
	LOPND_EST_TOTAL_MEMORY		= 0x02000001,
	LOPND_EST_TOTAL_TIME		= 0x02000002,
	LOPND_EST_CARDINALITY		= 0x02000004, 
	LOPND_EST_ACCESSED_ROWS		= 0x02000008, 
	LOPND_EST_USED_ROWS			= 0x02000010,
	LOPND_NUM_JOINS				= 0x02000020,
	LOPND_UPDATE_STATS_WARNING	= 0x02000040,
	LOPND_CROSS_PRODUCT			= 0x02000080,
	LOPND_SCAN_SIZE				= 0x02000100,          
             /** EXECUTE **/ 
	LOPND_USED_ROWS				= 0x04000001,
	LOPND_ACCESSED_ROWS			= 0x04000002,
	LOPND_TOTAL_MEM_ALLOC		= 0x04000004,
	LOPND_ELAPSED_TIME			= 0x04000008,
	LOPND_CPU_TIME				= 0x04000010,
	LOPND_PROCESS_BUSY_TIME		= 0x04000020,
	LOPND_STATS_IDLE_TIME	= 0x04000040,

} RULE_EXPR_LOPND;
//
//======================= RULE RIGHT OPERAND =======================================
//
typedef enum _RULE_EXPR_ROPND
{
	ROPND_NUMBER                    = 0x00000000,

	ROPND_EST_USED_ROWS             = 0x00000010,
	ROPND_EST_ACCESSED_ROWS         = 0x00000020,
	ROPND_EST_TOTAL_MEMORY          = 0x00000040,
	ROPND_EST_CPU_TIME              = 0x00000080,
	ROPND_EST_IO_TIME               = 0x00000100,

} RULE_EXPR_ROPND;
//
//======================= RULE EXPRESSIONS ========================================================
//
//====================== CONNECTION EXPRESSION ===================================================
//
 typedef struct _RULE_EXPR_OFFSET
 {
 	union
 	{
 		unsigned long item;
 		unsigned short offset[2];
 	} u;
 } RULE_EXPR_OFFSET;

typedef struct _CONN_EXPR
{
	RULE_EXPR_LOPND		m_lopnd;			// Left operand type
	RULE_EXPR_OPER		m_oper;
	RULE_EXPR_OFFSET	m_role;
	RULE_EXPR_OFFSET	m_user;
	char				m_value[MAX_ROPND_STR_LEN + 1];	// string is right operand (we can use a pointer)?

}  CONN_EXPR, *pCONN_EXPR;
//
//====================== COMP and EXEC EXPRESSION ==============================================
//   
typedef struct _RULE_EXPR
{
	RULE_EXPR_LOPND	m_lopnd;			// Left operand type
	RULE_EXPR_OPER	m_oper;
	RULE_EXPR_OPER	m_operNum;
	RULE_EXPR_ROPND	m_ropnd;			// Right operand type
	int64			m_value;			// number if right operand is ROPND_NUMBER

}  RULE_EXPR, *pRULE_EXPR;
//
//======================= RULE ITEM ========================================================
//
typedef struct _RULE_ITEM
{
    int				m_ruleId;
	unsigned short	m_warnLevel;					// level of warning for this rule
	RULE_TYPE		m_type;
	char			m_ruleName[MAX_RULE_NAME_LEN + 1];	// name of the rule
	RULE_SPEC_OPER	m_operator;						// global operator for all expressions
	
	short			m_exprCount;				// current count of expressions in m_exp array

	CONN_EXPR		m_connExpr[MAX_EXPR_SIZE];		// max number of expressions in connection item = 15  
	RULE_EXPR		m_ruleExpr[MAX_EXPR_SIZE];		// max number of expressions in one rule = 15  
	
	RULE_SPEC_ACT	m_action;							// action defined for this rule item
//
// execute action with sql_text, If m_length = 0 and m_sqlCmd = 0 -> no sql
//
	unsigned short	m_sqlLength;
	unsigned char*	m_sqlCmd;
//
// if comment
//
	unsigned short	m_commentLength;
	unsigned char*	m_comment;
	bool			m_wms_off;				// perf
	bool			m_end_stats_off;		// perf

// 2.5 Aggregation
	short			m_aggr_query_types;		// query types to aggregate for this rule item
	short			m_aggr_repos_interval;// repository update interval in minutes
	short			m_aggr_wms_interval;// wms update interval in minutes
	short			m_aggr_exec_interval;
	short			m_aggr_stats_once;		// perf

	unsigned short	m_usedCount;
	unsigned short	m_usedList[MAX_RULE_SERVICES];	// list of services where the rule is used (we can use bit map)	

} RULE_ITEM, * pRULE_ITEM;

//
//======================== LIST OF RULES =================================
//
typedef struct _RULE_LIST
{
	short	m_maxRuleCounter;			// max number of Rules
	short	m_curRuleCounter;			// current counter of rules
	int64	m_lastRuleUpdate;			// timestamp last time the list has been modified

	pRULE_ITEM	m_ruleMap[DEFAULT_MAX_RULES];	// array of pointers to RULE_ITEM structures;

} RULE_LIST, *pRULE_LIST;
//
//========================= LIST OF SELECTED CONNECTIONS =================
//
typedef struct _CONN_SELECTED_LIST
{
	unsigned short m_connRuleItem;
	unsigned short m_serviceNumber;
	
}CONN_SELECTED_LIST, *pCONN_SELECTED_LIST;

typedef struct _RULE_CONNSRVR_ITEM
{
    int				m_ruleId;
	unsigned short	m_warnLevel;					// level of warning for this rule
	RULE_TYPE		m_type;
	char			m_ruleName[MAX_RULE_NAME_LEN + 1];	// name of the rule
	RULE_SPEC_OPER	m_operator;						// global operator for all expressions
	
	short			m_exprCount;				// current count of expressions in m_exp array
	CONN_EXPR		m_connExpr[MAX_EXPR_SIZE];		// max number of expressions in connection item = 15  
	RULE_SPEC_ACT	m_action;							// action defined for this rule item
//
// execute action with sql_text, If m_length = 0 and m_sqlCmd = 0 -> no sql
//
	unsigned short	m_sqlLength;
	unsigned char*	m_sqlCmd;
	bool			m_wms_off;				// perf
	bool			m_end_stats_off;		// perf
// 2.5 Aggregation
	short			m_aggr_query_types;		// query types to aggregate for this rule item
	short			m_aggr_repos_interval;// repository update interval in minutes
	short			m_aggr_wms_interval;// wms update interval in minutes
	short			m_aggr_exec_interval;
	short			m_aggr_stats_once;		// perf

} RULE_CONNSRVR_ITEM, * pRULE_CONNSRVR_ITEM;

typedef struct _CONN_SRVR_LIST
{
	int64				m_timestamp;
	bool				m_hpsOnly;
	unsigned short		m_serviceCount;
	char*				m_serviceName;
	unsigned short		m_selectedCount;
	CONN_SELECTED_LIST	m_selected[MAX_CONN_SIZE];
	RULE_CONNSRVR_ITEM m_rules[1];

} CONN_SRVR_LIST, *pCONN_SRVR_LIST;
//
//========================================================================
//======================== Messages ======================================
//========================================================================
//
typedef enum _MSG_STATE {
	MSS_EMPTY = 0,
	MSS_NEW,
	MSS_PROCESSED,
} MSG_STATE;

typedef enum _MSG_TYPE {
	MST_REQUEST = 1,
	MST_INFO,
} MSG_TYPE;
//
//
//=========================================================================================
//========================== Definitions and structures for WARN block ====================
//=========================================================================================
//
//==================== REASON ==========================================================
//
			/** CONN **/
#define WRSNM_INIT					0x0000 
#define WRSNM_RULE					0x1000 
#define WRSNM_NO_RULES				0x1001 
#define WRSNM_NO_MATCHING_RULES		0x1002 

            /** COMP **/ 
#define WRSNC_INIT					0x0000 
#define WRSNC_RULE					0x1000     
#define WRSNC_EST_TOTAL_MEMORY		0x1001 	// rule
#define WRSNC_EST_TOTAL_TIME		0x1002 	// rule
#define WRSNC_EST_CARDINALITY		0x1004 	// rule 
#define WRSNC_EST_ACCESSED_ROWS		0x1008 	// rule 
#define WRSNC_EST_USED_ROWS			0x1010 	// rule
#define WRSNC_MAX_NUM_JOINS			0x1020 	// rule
#define WRSNC_UPDATE_STATS_WARNING	0x1040 	// rule
#define WRSNC_CROSS_PRODUCT			0x1080 	// rule
#define WRSNC_SCAN_SIZE				0x1100 	// rule 
//#define WRSNC_MAX_CPU_BUSY			0x0200 	// service threshold
//#define WRSNC_MAX_MEM_USAGE			0x0400 	// service threshold
//#define WRSNC_WAIT_TIMEOUT			0x0800 	// service threshold 
//#define WRSNC_MAX_ROWS_FETCHED		0x1000 	// service threshold 

           /** RUNTIME **/ 
#define WRSNE_INIT					0x0000 
#define WRSNE_RULE					0x1000
#define WRSNE_USED_ROWS				0x1001 	// rule
#define WRSNE_ACCESSED_ROWS			0x1002 	// rule
#define WRSNE_TOTAL_MEM_ALLOC		0x1004 	// rule
#define WRSNE_ELAPSED_TIME			0x1008 	// rule
#define WRSNE_CPU_TIME				0x1010 	// rule
#define WRSNE_PROCESS_BUSY_TIME		0x1020 	// rule
#define WRSNE_STATS_IDLE_TIME	0x1040 	// rule 

#define WRSNE_SERVICE_THRESHOLD		0x2000
#define WRSNE_EXEC_TIMEOUT			0x2040 	// service threshold

#define WRSNE_SYSTEM_THRESHOLD		0x4000  
#define WRSNE_WAIT					0x4080 	// system threshold - Query waits forever - all ESPs are in waiting state
#define WRSNE_CPU					0x4100 	// system threshold - Query uses too much CPU
#define WRSNE_MEM					0x4200 	// system threshold - Query uses too much memory on specific cpu

//==================== WARN ITEMS ==========================================================
//
//=====================CONN ITEM ============================
// 1. CONN WARN is sent from MXOSRVR in WouldLikeToExecute
// 2. DEFAULT for rule is NO_WARN
// 3. There are only two reasons:
//		SERVICE selected by RULE
//      DEFAULT SERVICE selected because no RULES matched.
// 4. Level of WARNs is always LOW
// 5. There is only one warning per query.

typedef struct _CONN_ITEM
{
	unsigned char		m_Version;		// this version is copied from m_CurrConnVersion
										// when WARN is created or updated
	unsigned short		m_level;
	unsigned short		m_reason;
	unsigned short		m_ruleId;
	unsigned short		m_exprNo;

} CONN_ITEM;

//=====================COMP ITEM ============================
//
// COMP ITEM has only expresion number:
// rule number is index in rule item array plus 1.
// For operator OR m_expr keeps first expresion number which returns TRUE.
// For operator AND m_expr keeps 1 which means all expresions return TRUE.
//
// We do not have to keep info about thresholds because all of them either have action REJECT or
// WAIT (for system thresholds) which we are not going to trace.
//
// WARN_LEVEL is 'ored' WARN_LEVEL for every rule in the item.

typedef struct _COMP_ITEM
{
	unsigned char		m_Version;		// this version is copied from m_CurrCompVersion
										// when WARN is created or updated
	unsigned short		m_level;
	unsigned char		m_expr[MAX_RULE_SIZE];

} COMP_ITEM;

//
// EXEC ITEM is the same as COMP ITEM.
//

typedef struct _EXEC_ITEM
{
	unsigned char		m_Version;	// this version is copied from m_CurrExecVersion
									// when WARN is created or updated
	unsigned short		m_level;
	unsigned char		m_expr[MAX_RULE_SIZE];
//
// we need only one service/system threshold because for all of them action is CANCEL
	unsigned short		m_exec_service_threshold;
	unsigned short		m_exec_system_threshold;
//
// for system threshold we need info about process
	short				m_segment;
	short				m_cpu;
	short				m_pin;

} EXEC_ITEM;


typedef struct _WARN_ITEM
{
	CONN_ITEM connItem;
//
	COMP_ITEM compItem; 
//
	EXEC_ITEM execItem;

} WARN_ITEM, * pWARN_ITEM;

//==================== query ==========================================================

typedef struct _WARN_QUERY
{
	SLOT_STATE		m_slot_state;
//
	unsigned long			m_internal_Id;		// used to identify the query
	WARN_ITEM		m_item;
;
	short m_left;				// index to previous query
	short m_right;				// index to next query
	short m_cur;

} WARN_QUERY, *pWARN_QUERY;

       
//==================== query list =====================================================

typedef struct _WARN_QUERY_LIST
{
	unsigned char m_CurrConnVersion;	//QSCOM bumps this version by one when CONN rules change
	unsigned char m_CurrCompVersion;	//QSCOM bumps this version by one when COMP rules change
	unsigned char m_CurrExecVersion;	//QSCOM bumps this version by one when EXEC rules change

	short		m_firstWarnNumber;		// number of first warn query in the link list
	short		m_lastWarnNumber;
	int64		m_lastWarnQueryUpdate;	// timestamp last time the list has been modified
	
	WARN_QUERY	m_warnQueryDesc[1];		// place holder for array of warn query definitions
	
} WARN_QUERY_LIST, *pWARN_QUERY_LIST;
//
//===================================================================================
//================= INPUT  QUERY SERVICE DESCRIPTION ================================
//===================================================================================
//
// Information in INPUT_QS_BLOCK - Com Service creates this block from metadata
//
typedef struct _QUEUE_DESC
{
	SLOT_STATE	m_slot_state;	// SLOT_EMPTY - slot is empty, SLOT_ALLOC - slot assigned;

	short		m_queue_index;		 // sequential number of the queue
	long		m_queue_handle;		 // pointer to queue object in Query Service
//
// thresholds
//
	int64		m_maxRowsFetched;		// number of fetched rows
	short		m_maxCpuBusy;			// cpu busy in %(range 0-100%) for this service 
	short		m_maxMemUsage;          // memory utilization %(range 0-100%) for this service
	short		m_maxOverflowUsage;     // ssd overflow
	short		m_maxCurExecQueries;	// threshold - max number of current executing queries for the instance
	long		m_maxEsps;				// threshold for max number of ESPs
	long		m_wait_timeout;			// Timeout in minutes for waiting queries
	long		m_hold_timeout;			// Timeout in minutes for holding queries
	long		m_exec_timeout;			// Timeout in minutes for executing queries
	short		m_maxDiskIo;            // disk io in mb/sec (range 0-25mb/sec)
	short		m_maxDiskCache;         // disk cache in mb/sec (range 0-?mb/sec)
//
	short		m_queue_weight;			// QUEUE_WGHT  
//
	short		m_cur_wait;	 		// current waiting queries in the Service for this node
	short		m_cur_exec; 		// current executing queries in the Service for this node
	short		m_cur_hold;		 	// current holding queries in the Service for this node
	short		m_cur_suspend; 		// current suspended queries in the Service for this node
;
	QUEUE_PRTY	m_queue_prty;		// from 1 to 6
	QUEUE_STATE	m_queue_state;		// QUEUE_ACTIVE, QUEUE_HOLD, QUEUE_STOPPING, QUEUE_STOPPED
	short		m_service_id;		// id of the service in database
//
	char		m_sql_cmds[MAX_SQL_CMD_LEN + 1];	//	SQL commands
	char		m_cancelIfClientDisappear[6];
	char		m_checkQueryEstResourceUse[6];
	short		m_master_prty;		// NDCS server master priority
;
	char		m_queue_name[MAX_SERVICE_NAME_LEN + 1];	//	Queue Name
	short		m_active_begin;		// Number of minutes from 12AM 
	short		m_active_end;		// Number of minutes from 12AM
	bool		m_sqlPlan;			// Generate SQL Plan flag
	bool		m_sqlText;          // Generate SQL Text flag
;
	unsigned short	m_compRuleCount;					// Number of comp rules - 100
	unsigned short	m_compRuleSelected[MAX_RULE_SIZE];	// Array of numbers of comp rules
;
	unsigned short	m_execRuleCount;					// Number of execute rules - 100
	unsigned short	m_execRuleSelected[MAX_RULE_SIZE];	// Array of numbers of execute rules
;
	unsigned short	m_commentLength;
	unsigned char*	m_comment;

;
	short		m_left;				// index to previous service
	short		m_right;			// index to next service
	short		m_cur;
} QUEUE_DESC, *pQUEUE_DESC;

typedef struct _QUERYSERVICE_DESC
{
	GLOBAL_STATE m_GlobalState;		// GLOBAL_ACTIVE, GLOBAL_HOLD, GLOBAL_SHUTDOWN, GLOBAL_STOPPED
	short		m_Interval;			// seconds - NSKStat collects info
	short		m_ruleInterval;		// seconds - QSRULE runs rules
	short		m_ruleIntervalExecTime;
//
	bool		m_SqlBufferFull;	// if true - WMS will send all queries to WAITING queue
	short		m_SqlMaxBuffer;		// max usage of Dp2 buffer - default is 80%
	short		m_SqlBuffer;		// usage of Dp2 buffer in precentage
//
	bool		m_TxnBackout;		// if true - WMS sends all queries to WAITING queue
	unsigned long m_AbortTxnMask;	// 32 bits mark 32 segments when transaction is Aborting on that segment
	int64		m_TimestampDelay;	// correction to timestamp on primary segment
//
	long		m_isoMapping;		// charset support
//
// number of queries in the segment for all services
//
// --------------- ALL memory can have WAITING + EXECUTING + COMPLETED queries
// --------------- EXEC memory can have EXECUTING + COMPLETED queries
//
	short		m_MaxQueries;			// ALL and EXEC shared memories allocates for this number of queries
	short		m_MaxExecQueries;		// Max number of queries modified by EMA
	short		m_NodeExecutingQueries;	// Current number of exec queries
//
// ------------- default thresholds
//
	int64		m_maxRowsFetched;		// number of fetched rows
	short		m_maxCpuBusy;			// cpu busy in %(range 0-100%) for this service 
	short		m_maxMemUsage;          // memory utilization %(range 0-100%) for this service
	short		m_maxOverflowUsage;     // ssd overflow
	short		m_maxCurExecQueries;
	long		m_maxEsps;				// threshold for max number of ESPs
	OVERFLOW_MODE   m_overflowMode;		// UNKNOWN, DISK, SSD, HYBRID
	long		m_wait_timeout;			// Timeout in minutes for waiting queries
	long		m_hold_timeout;			// Timeout in minutes for holding queries
	long		m_exec_timeout;			// Timeout in minutes for executing queries
	short		m_maxDiskIo;            // disk io in mb/sec (range 0-25mb/sec)
	short		m_maxDiskCache;         // disk cache in mb/sec (range 0-?mb/sec)
	short		m_canaryIntervalMin;
	short		m_canaryExecSec;
	char		m_canaryQuerySrc[MAX_CANARY_LEN + 1];
	short           m_maxTxnRollMin;
	char		m_cancelIfClientDisappear[6];
	char		m_checkQueryEstResourceUse[6];
	char		m_Name[MAX_NAME_LEN];	// configuration name
	short		m_MajorVersion;         // version number
	short		m_MinorVersion;
	char		m_MemVproc[MAX_VPROC_NAME_LEN]; // VPROC of QSMGR when shared memory was created
;
	short		m_MaxMemoryQueueCounter; // Max number of slots
	short		m_QueueCounter;			// how many queue is defined
	short		m_FirstQueueNumber;		// number of first allocated queue in the link list
	short		m_EmptySlotIndex;		// index to the next empty slot
;
	VERSION_def m_Version[6];			// versions for QSSYNC, QSMGR, QSCOM, QSSTATS, QSRULE, QSOFFNDR
	char		m_Vproc[6][MAX_VPROC_NAME_LEN]; // VPROCs for QSSYNC, QSMGR, QSCOM, QSSTATS, QSRULE, QSOFFNDR 
;
	short		m_MetaDataMajorVersion; //Metadata major version
	short		m_MetaDataMinorVersion; //Metadata minor version
	short		m_MetaDataBuildId;		//Metadata build id
;
	char		m_rsCollectorPool[MAX_RS_POOL_COUNT][EXT_FILENAME_LEN];	//pool of rs collectors 
//
//============== timestamps for last rule update =========================================
// These timestamps are updated when rules are changed and also when  
// selection of the rules for services (COMP and EXEC) and for CONN is changed
//
	int64		m_lastConnRuleUpdate;	// timestamp last time CONN rules have been modified
	int64		m_lastCompRuleUpdate;	// timestamp last time COMP rules have been modified
	int64		m_lastExecRuleUpdate;	// timestamp last time EXEC rules have been modified
;
//============= semaphores used to access rules ====================================
// When QSMGR or QSCOM or QSRULE want to access any info about rule which can be changed
// must use the following semaphores. They are allocated by QSMGR and QSCOM and QSRULE have
// to use them
//
	long		m_ConnRule_semid;
	long		m_CompRule_semid;
	long		m_ExecRule_semid;
		
	unsigned short		m_connRuleCount;				// number of selected CONN LISTS
	unsigned short      m_HPSRuleCount;						// number of selected HPS* rules
	CONN_SELECTED_LIST	m_connRuleSelected[MAX_CONN_SIZE];	// array of selected CONN LISTS
	bool	m_ConnRuleListHpsOnly;
//
//======================================================
//============= OFFENDER Thresholds ====================
//======================================================
//
	short	m_offSampleInterval;		//from 10 to 60 seconds. Default 10
	short	m_offSampleCpus;			//from 2 to 16 CPUs. Default 2
	short	m_offSampleCache;			//from 10 to 100. Default 10
//
	short	m_offTxnleInterval;			//from 1 to 10 minutes. Default 1
	short	m_offTxnDuration;			//from 1 minute. Default 1 minute
	short	m_offTxnCache;				//from 10 to 100. Default 10
//
	short	m_wms_health_stats_interval; //from 1 to 30 minutes. Default 10
//--------------------------------------------------------------------------------
// min and max port is used to limit searching for Query TCPIP State
//
	unsigned short m_min_port;			//min TCPIP port accross all queries
	unsigned short m_max_port;			//max TCPIP port accross all queries
//
//================================================================================
//
  QUEUE_DESC	m_QueueDesc[1];		    // place holder for array of queue definitions
} QUERYSERVICE_DESC, *pQUERYSERVICE_DESC;
//
//===================================================================================
//====================== INPUT  THRESHOLD DESCRIPTION ===============================
//===================================================================================
//
// Information in INPUT_THRESHOLD_BLOCK - Com Service creates this block from metadata
//
//
typedef struct _THRESHOLD
{
	THRESHOLD_TYPE  tre_type;
	THRE_OPERAND    tre_operand;
	int64			tre_value;
	THRE_ACTION		tre_action;
	THRE_ACTION_ARG tre_action_argument;	// only for priority - PLUS or MINUS orVALUE
	short           tre_action_arg_value;	// only for priority - Holds the actual value
	short			tre_queueId;			// to which queue this threshold is assigned
	
} THRESHOLD, *pTHRESHOLD;

typedef struct _THRESHOLD_DESC
{
	short		m_ThresholdCounter;		// total number of thresholds
	THRESHOLD	m_Threshold[1];		// placeholder for threshold array
	
} THRESHOLD_DESC, *pTHRESHOLD_DESC;
//
//===================================================================================
//================== QUERIES (WAITING + EXECUTING) DESCRIPTION ======================
//=================================================================================== 
//
// Information in OUT_QUERY_BLOCK - this block is created by Query Service
//
typedef struct _OUTPUT_QUERY_DESC
{
// in Shared Data Memory we allocate QUERYSERVICE_DESC.m_TotalQueryCounter slots
// for OUTPUT_QUERY_DESC. Slots are assigned for queries by the Query Service when
// the query id is sent by the MXOSRVR. The slot is released when the query finishes
// These slots describe WAITING and EXECUTING queries. When the query executes we
// assigne additional slot EXEC_QUERY_DESC to keep stats
//
	SLOT_STATE			m_slot_state;	// SLOT_EMPTY - slot is empty, SLOT_ALLOC - slot assigned;
	;
	QUERY_STATE			m_query_state;		// QUERY_WAITING, QUERY_EXECUTING
//
	bool				m_asegment_allocated;
	short				m_adaptive_segment;
	long				m_stmtHandle;
//
	unsigned long		m_internal_Id;		// used to identify the query
//
	bool				m_sentBySqlCli;		// query sent by SQL CLI
//
	unsigned short		m_warnLevel;		// a warn_level ored from warn messages
	//
	// values returned after query prepare
	//
//
// from SQL_QUERY_COST_INFO
//
	double	m_EST_cpuTime;
	double	m_EST_ioTime;
	double	m_EST_msgTime;
	double	m_EST_idleTime;
	double	m_EST_totalTime;
	double	m_EST_cardinality; 
	double	m_EST_estimatedTotalMem;
	short	m_EST_resourceUsage;
	double	m_EST_numSeqIOs;
	double	m_EST_numRandIOs;
	short	m_EST_maxCpuBusy;

//
// from SQL_QUERY_COMPILER_STATS_INFO
//
	unsigned long	m_CMP_affinityNumber;
	long	m_CMP_dop; /* degree of parallel execution */
	short	m_CMP_xnNeeded;
	short	m_CMP_mandatoryCrossProduct;
	short	m_CMP_missingStats;
	short	m_CMP_numOfJoins;
	short	m_CMP_fullScanOnTable;
	short	m_CMP_highDp2MxBufferUsage;
	double	m_CMP_rowsAccessedForFullScan;
	double	m_CMP_dp2RowsAccessed;
	double	m_CMP_dp2RowsUsed;
	bool	m_CMP_noStats;


	// Currently not in R2.5. To be enabled post R2.5/SQ
//	unsigned short	m_CMP_numOfUDRs;
	COMP_OVERFLOW m_compOverflow; //ssd overflow
//
	//short	m_processHandle[10];	// Handle of MXOSRVR
	TPT_DECL(m_processHandle);
	char	m_process_name[32];		// Process Name of MXOSRVR
	char	m_segmentname[128];
	long	m_segmentnumber;
	short	m_cpu;
	short	m_pin;
	long	m_process_id;
	short	m_priority;				// will be used for releasing query from WAITING queue if value is 100
	unsigned short m_user_id;
	//
	char	m_query_id[MAX_QUERY_ID_LEN];		// Query ID returned by PREPARE
	char	m_sessionId[SESSION_ID_LEN];
	char	m_ds_name[MAX_DSOURCE_NAME + 1];	// Datasource Name
	char	m_query_name[MAX_QUERYNAME_LEN + 1];// Query Name (Alias) assigned by the user
	char	m_user_name[MAX_USERNAME_LEN + 1];		// User Name
	char	m_user[MAX_USERNAME_LEN + 1];		// User alias name
	char	m_dbuser_name[MAX_USERNAME_LEN + 1];		// db User Name
	short	m_query_type;						// Query type
	bool	m_mute;								// Dashboard testing - no 21036 message when mxosrvr is stopped externally
	bool	m_ext_21036;		// extended 21036 message - for SRPQ
	int64	m_query_start_time;	// timestamp when 21036 start message was generated

	long	m_queue_handle;		// Pointer to queue object (internals Query Service)				
	long	m_query_handle;		// Pointer to query object (internals Query Service)
	//
	//
	short	m_queue_index;		// index of the queue
	short	m_stats_index;		// index of the query in the stats queue
	//
	int64	m_start_timestamp;	// Timestamp when the query has been registred to Query Service
	int64	m_entry_timestamp;	// Timestamp when the query has been inserted into current queue
//
	int64	m_start_in_state_timestamp; // Timestamp when the query was in the current state
	long	m_wait_time;		// wait time
	long	m_hold_time;		// hold time
	long	m_exec_time;		// exec time
	long	m_suspended_time;	// suspend time

	long	m_plan_text_len;
	long	m_plan_text_loc;	// Pointer to a location in Query data segment where the plan/text is stored
//
	char	m_SqlSrc[RMS_STORE_SQL_SOURCE_LEN + 1];	//	pre SQL Text (only first 254 bytes)
	long	m_SqlSrcLen;
	char	m_StatementId[MAX_NAME_LEN];
//
// AGGREGATION
//
	bool				m_baggregate_query;
	short				m_aggr_query_types;
	short				m_aggr_repos_interval;
	short				m_aggr_wms_interval;
	short				m_aggr_exec_interval;
	short				m_aggr_stats_once;		// perf
//
	int64				m_last_aggr_update;		//2.5 aggregation - timestamp of last update
	int64				m_total_aggr_time;		// time of the package
	int64				m_total_aggr_queries;	// 2.5 aggregation - total number of queries in the package
//
// CONN WARN
//
	unsigned short		m_reason;
//
// Connection rule
//
	unsigned short		m_con_ruleId;
	unsigned short		m_con_exprNo;
	char				m_con_rule_name[MAX_RULE_NAME_LEN + 1];
//
// Compilation rule
//
	unsigned short		m_cmp_ruleId;
	unsigned short		m_cmp_exprNo;
	char				m_cmp_rule_name[MAX_RULE_NAME_LEN+1];
//
// Execution rule
//
	unsigned short		m_exe_ruleId;
	unsigned short		m_exe_exprNo;
	char				m_exe_rule_name[MAX_RULE_NAME_LEN+1];
//
	unsigned short		m_ruleId;
	unsigned short		m_exprNo;
	char				m_rule_name[MAX_RULE_NAME_LEN+1];
//
// TCPIP info about connection with client
//
	char	m_tcpProcessName[MAX_TCP_PROCESS_NAME];
	unsigned short m_port;
	int		m_tcpStatus;
	int64	m_client_disappeared_timestamp;
//
	short	m_rsCollectorNumber; //number of collector in collector pool
//
	bool	m_pertable_stats;	// true - WMS wrote perTableStats for this query
//
	short	m_left;				// index to previous query
	short	m_right;			// index to next query
	short	m_cur;
	
} OUTPUT_QUERY_DESC, *pOUTPUT_QUERY_DESC;

typedef struct _OUTPUT_QUERY_LIST
{
	short	m_CurQueryCounter;			// current number of queries
	short	m_MaxQueryCounter;			// max number of queries
	short	m_FirstQueryNumber;			// number of first allocated query in the link list
	short	m_EmptySlotIndex;			// index to the next empty slot
	int64	m_LastQueryUpdate;			// timestamp when last time the query array has been changed
	
	OUTPUT_QUERY_DESC m_OutputQueryDesc[1];	// place holder for array of query definitions
	
} OUTPUT_QUERY_LIST, *pOUTPUT_QUERY_LIST;
//
//===================================================================================
// ======================= EXEC QUERY ARRAY =========================================
//===================================================================================
//
// Information in OUT_STATS_BLOCK - This block is created by Query Service and updated peridicaly by Stats Service
// Because total number of executing queries can not be changed without restarting
// Query Service, this block has max number of slots and QS distributes them for
// executing queries. So that, location of the query is always fixed for execution time and
// can be easy updated.
//

typedef struct _EXEC_QUERY_DESC
{
	SLOT_STATE	m_slot_state;					// 0 - slot is empty, 1 - query assigned;
	char		m_query_id[MAX_QUERY_ID_LEN];
	long		m_query_handle;					// pointer to query object in Query Service memory
	long		m_queue_handle;					// pointer to queue object in Query Service memory
	//
	short		m_queue_index;					// index of the queue
	short		m_query_index;					// index of the query queue
	//
	short		m_Interval;						// seconds - Interval when Stats Service collects stats from Executor
	bool		m_completed;					// ON when query state is COMPLETED
	int64		m_comp_timestamp;				// completed timestamp
	unsigned long m_internal_Id;					// used to identify the query
	int64		m_exec_timestamp;				// exec timout timestamp
//
// offender values
//
	short		m_CpuUsage;						// in %
	short		m_MemoryUsage;					// in kbytes
 	unsigned long	m_KBMainTotal;					// in kbytes
	//
	//----- Stats Service updates this part
	//
	char		m_StatementId[MAX_NAME_LEN];
	int64		m_CompileStartTime;
	int64		m_CompileEndTime;
	int64		m_CompileElapsedTime;
	int64		m_ExecutionStartTime;
	int64		m_ExecutionEndTime;
	int64		m_ExecutionElapsedTime;
	int64		m_AccessedRows;            
	int64		m_UsedRows;                
	int64		m_MessageCount;            
	int64		m_MessageBytes;            
	int64		m_StatsBytes;                
	int64		m_DiskIOs;                 
	int64		m_LockWaits;         
	int64		m_LockEscalations;         
	int64		m_ProcessBusyTime;                
	int64		m_Opens;                    
	int64		m_OpenTime;                
	int64		m_ProcessesCreated;        
	int64		m_ProcessCreateTime;
	short		m_State;
	short		m_RetryCnt;
	short		m_SqlErrorCode;
	int64		m_NumRowsIUD;
	short		m_StatsErrorCode;
	int64		m_SqlSpaceAlloc;//2.5 aggregation changed from long to int64
	int64		m_SqlSpaceUsed;//2.5 aggregation changed from long to int64
	int64		m_SqlHeapAlloc;//2.5 aggregation changed from long to int64
	int64		m_SqlHeapUsed;//2.5 aggregation changed from long to int64
	int64		m_SqlCpuTime;
	int64		m_EIDSpaceAlloc;//2.5 aggregation changed from long to int64
	int64		m_EIDSpaceUsed;//2.5 aggregation changed from long to int64
	int64		m_EIDHeapAlloc;//2.5 aggregation changed from long to int64
	int64		m_EIDHeapUsed;//2.5 aggregation changed from long to int64
	int64		m_TotalMemory;			// Is the addition of SQL Space used and SQL Heap used
	unsigned long	m_maxMemUsed;		// Added for manageability requirements
	double		m_EstAccessedRows;//2.5 aggregation changed from float to double
	double		m_EstUsedRows;//2.5 aggregation changed from float to double
	char		m_SubQryType[SUB_QRY_TYPE_LEN+1];
	char		m_ParSysName[PAR_SYS_NAME_LEN+1];
	int64           m_stats_update_timestamp; // recording query stats updating time
	// New_Col
	int32		m_total_child_count;
	int32		m_active_child_count;
	char		m_super_parent_query_id[MAX_QUERY_ID_LEN];// first grand parent when query has many child
	char		m_parent_query_id[MAX_QUERY_ID_LEN];
	long		m_NumSqlProcs;
	long		m_ExePriority;
	char		m_SqlSrc[RMS_STORE_SQL_SOURCE_LEN + 1];	//	pre SQL Text (only first 254 bytes)
	long		m_SqlSrcLen;
	char		m_TransID[MAX_TXN_STR_LEN];
	int64		m_RowsReturned;
	int64		m_FirstRowReturnTime;
	int64		m_ReqMsgCnt;//2.5 aggregation changed from long to int64
	int64		m_ReqMsgBytes;
	int64		m_ReplyMsgCnt;//2.5 aggregation changed from long to int64
	int64		m_ReplyMsgBytes;
	short		m_LastErrorBeforeAQR;
	int64		m_AQRNumRetries;//2.5 aggregation changed from short to int64
	int64		m_DelayBeforeAQR;//2.5 aggregation changed from short to int64

	// Currently not in R2.5. To be enabled post R2.5/SQ
	int64           m_UdrCpuTime;
	int64           m_numCpus;
	EXEC_OVERFLOW m_execOverflow; //ssd overflow
//
	bool		m_suspended;		// ON when query is SUSPENDED

	int64		m_suspended_ts;		//3289
	int64 		m_released_ts;
	int64 		m_cancelled_ts;

	//
	short		m_left;				// index to previous query
	short		m_right;			// index to next query
	short		m_cur;
	
} EXEC_QUERY_DESC, *pEXEC_QUERY_DESC;

typedef struct _EXEC_QUERY_LIST
{
	short	m_CurExecQueryCounter;			// current number of exec queries
	short	m_MaxExecQueryCounter;			// max number of exec queries (EXEC_QUERY_DESC size)
	short	m_FirstExecNumber;				// number of first exec query in the link list
	short	m_ExecEmptySlotIndex;			// next empty slot
	int64	m_LastExecQueryUpdate;			// timestamp last time the list has been modified
	
	EXEC_QUERY_DESC m_ExecQueryDesc[1];		// place holder for array of exec query definitions
	
} EXEC_QUERY_LIST, *pEXEC_QUERY_LIST;

//
//==============================================================================================
//================ Messages for communication betwee QS and STATS, COM ==============
//==============================================================================================
//
//STATS---------->QUERY SERVICE
//	
typedef struct _STATS_MSG
{
	MSG_STATE	m_MsgState;					// message state
	MSG_TYPE	m_MsgType;					// message type
	short		m_OpId;						// operation id
	short		m_Error;
	short		m_ErrorDetail;
	COM_OPERATION_ARG m_AltPriAction;		// plus or minus
	short		m_AltPriValue;              // nn 
	char		m_QueryId[MAX_QUERY_ID_LEN]; // query id
	unsigned long m_internal_Id;
	long		m_QueueHandle;
	
} STATS_MSG, *pSTATS_MSG;

typedef struct _STATS_MSG_LIST
{
	short		m_MaxMsgCounter;
	int64		m_LastMsgUpdate;
	short		m_MsgPrty[MSG_CACHE];
	STATS_MSG	m_StatsMsg[MSG_CACHE];
	
} STATS_MSG_LIST, *pSTATS_MSG_LIST;
//
// COM------------>QUERY SERVICE
//
typedef struct _COM_MSG
{
	MSG_STATE	m_MsgState;					// message state
	long long	m_MsgNumber;				// message sequential number
	MSG_TYPE	m_MsgType;					// message type
	short		m_OpId;   					// operation id
	short		m_Error;
	short		m_ErrorDetail;
	char		m_ErrorMsg[256];
	COM_OPERATION_ARG m_AltPriAction;		// plus or minus
	short		m_AltPriValue;				// nn 
	char		m_QueryId[MAX_QUERY_ID_LEN];	// query id
	short		m_QueueId;					// queue Id
	short		m_QueueIndex;				// queue index
	char		m_QueueName[MAX_SERVICE_NAME_LEN + 1];	// queue name
	char		m_Pid[17];					// process id
	REQUESTER	m_RequesterType;
	char		m_RequesterApplication[MAX_APPLICATION_NAME_LEN +1];
	char		m_RequesterComputer[MAX_COMPUTER_NAME_LEN + 1];
    char		m_RequesterName[USERNAME_LENGTH + 1];
    char		m_RequesterRole[USERNAME_LENGTH + 1];
    char		m_RequesterDBUserName[USERNAME_LENGTH + 1];
    char		m_CancelQueryComment[MAX_COMMENT_LEN/4];
//
} COM_MSG, *pCOM_MSG;

typedef struct _COM_MSG_LIST
{
	short	m_MaxMsgCounter;
	int64	m_LastMsgUpdate;
	short	m_MsgPrty[MSG_CACHE];
	COM_MSG m_ComMsg[MSG_CACHE];
	
} COM_MSG_LIST, *pCOM_MSG_LIST;

typedef struct _COM_MSG_WAIT
{
	COM_MSG		   m_msg;
	unsigned short m_sqlLength;
	unsigned char  m_sqlCmd[MAX_SQL_CMD_LEN + 1];	// sql cmd
	REQUESTER	m_RequesterType;
	char		m_RequesterApplication[MAX_APPLICATION_NAME_LEN +1];
	char		m_RequesterComputer[MAX_COMPUTER_NAME_LEN + 1];
    char		m_RequesterName[USERNAME_LENGTH + 1];
    char		m_RequesterRole[USERNAME_LENGTH + 1];
    char		m_RequesterDBUserName[USERNAME_LENGTH + 1];

} COM_MSG_WAIT, *pCOM_MSG_WAIT;

//
// RULE-------------->QUERY SERVICE
//
typedef struct _RULE_MSG
{
	MSG_STATE	m_MsgState;					// message state
	MSG_TYPE	m_MsgType;					// message type
	short		m_OpId;						// operation id
	short		m_Error;					//R
	short		m_ErrorDetail;				//R
	unsigned long m_internal_Id;				//R
	short		m_query_index;				//R
	RULE_TYPE	m_rule_type;
//
// description of warn
//
	unsigned short		m_level;
	unsigned short		m_reason;
	unsigned short		m_rule;
	unsigned short		m_expression;
	unsigned char		m_exec_service_threshold;
	unsigned char		m_exec_system_threshold;
// for system threshold we need info about process
	short				m_segment;
	short				m_cpu;
	short				m_pin;

} RULE_MSG, *pRULE_MSG;

typedef struct _RULE_MSG_LIST
{
	short		m_MaxMsgCounter;
	int64		m_LastMsgUpdate;
	short		m_MsgPrty[MSG_CACHE];
	RULE_MSG	m_RuleMsg[MSG_CACHE];
	
} RULE_MSG_LIST, *pRULE_MSG_LIST;
//
//===================================================================================
//===================== Client Disappeared Messages ========================================
//===================================================================================

typedef struct _CL_MSG
{
	MSG_STATE	m_MsgState;					// message state
	MSG_TYPE	m_MsgType;					// message type
	short		m_OpId;						// operation id
	short		m_Error;
	short		m_ErrorDetail;
	unsigned long m_internal_Id;
	char		m_QueryId[MAX_QUERY_ID_LEN]; // query id
	long		m_QueueHandle;

} CL_MSG, *pCL_MSG;

typedef struct _CL_MSG_LIST
{
	short		m_MaxMsgCounter;
	int64		m_LastMsgUpdate;
	CL_MSG		m_ClMsg[MSG_CACHE];

} CL_MSG_LIST, *pCL_MSG_LIST;

//
//===================================================================================
//===================== Global Stats Information ====================================
//===================================================================================
//
typedef struct _GLOBAL_STATS
{
	int64	m_LastGlobalStatsUpdate;
	
	float	m_DiskIo;
	float	m_DiskCache;
	float	m_CpuBusy;
	float	m_MemoryUsage;
	unsigned long	m_KBMainTotal;
    float   m_OverflowUsage; //ssd overflow
    long	m_AvgEspNumber;
	long	m_MaxEspNumber;
//
	int		m_ClusterWaitingQueries;	 		// current waiting queries in the cluster
	int		m_ClusterExecutingQueries; 			// current executing queries in the cluster
	int		m_ClusterHoldQueries;		 		// current holding queries in the cluster
	int		m_ClusterSuspendedQueries; 			// current suspended queries in the cluster

	int		m_ServiceWaitingQueries[TOTAL_SERVICES];	 		// current waiting queries for the service
	int		m_ServiceExecutingQueries[TOTAL_SERVICES]; 			// current executing queries for the service
	int		m_ServiceHoldQueries[TOTAL_SERVICES];		 		// current holding queries for the service
	int		m_ServiceSuspendedQueries[TOTAL_SERVICES]; 			// current suspended queries for the service
//
// this part is used by Linux QSSYNC to keep info about nodes
//
	bool	m_bWarning;
	int64	m_LastMulticastUpdate;
	short	m_num_physical_nodes;
	CLUSTER_NODE m_ClusterNode[MAX_CLUSTER_NODES];
//
// for SSD support
//
	short	m_num_ssd_sql_nodes;
	short	m_num_ssd_nodes;
	SSD_NODE	m_SsdNode[MAX_SSD_NODES];
//
	bool	m_sq_virtual_cluster;
	bool    m_sq_cluster_size; //20130717  true for not 1,false for 1
//
	float	m_ExecPercentage; // Rao - Prototype for dynamically adjusting query executing percentage

	short   NumCompletedQueries;
	float	m_eaves;
	float	m_eavel;

	int64   m_ClusterCanaryExecTime;

} GLOBAL_STATS, *pGLOBAL_STATS;
//
//===================================================================================
//======================= Trace Information =========================================
//===================================================================================
//
typedef struct _TRACE
{
	long	m_trace;
	char	m_FileName[MAX_TRACE_FILENAME_LEN];		// File name
	char	m_FilePath[MAX_TRACE_FILEPATH_LEN];		// File path
	unsigned char coms[256];	// list of active segments
	unsigned short exp_percentage[6];	//percentage of queries for every prty
	unsigned short cur_percentage[6];	//current percentage for every prty

} TRACE, *pTRACE;
//
//===================================================================================
//====================== QUERY PLAN/TEXT DATA ======================================= 
//===================================================================================
//
typedef struct _QUERY_PLAN_TEXT
{
	long catcher;
	char			m_query_id[MAX_QUERY_ID_LEN];// Query ID returned by PREPARE
	unsigned long	m_plan_len;
	char			*m_plan;					// Query plan returned by PREPARE
	unsigned long	m_text_len;
	char			*m_text;					// Query text
} QUERY_PLAN_TEXT, *pQUERY_PLAN_TEXT;
//
//===================================================================================
//============================ WMS STATS DATA ====================================
//===================================================================================
//
#define	SM_INIT			0x0000
#define	SM_REJECTED		0x0001
#define	SM_CANCELED		0x0002
#define	SM_COMPLETED	0x0004
#define	SM_WAIT			0x0008
#define	SM_HOLD			0x0010
#define	SM_EXEC			0x0020
#define	SM_SUSPEND		0x0040

typedef struct _SERVICE_STATS_DATA
{
	unsigned long	m_total_queries;
	unsigned long	m_total_rejected;
	unsigned long	m_total_canceled;
	unsigned long	m_total_completed;
	unsigned long	m_total_wait;
	unsigned long	m_total_hold;
	unsigned long	m_total_exec;
	unsigned long	m_total_suspended;

	unsigned long	m_total_wait_time;			//in seconds total wait time
	unsigned long	m_total_hold_time;			//in seconds total hold time
	unsigned long	m_total_exec_time;			//in seconds total exec time
	unsigned long	m_total_suspended_time;		//in seconds total suspended time

	unsigned long	m_wait_time;			//in seconds average wait time (per query)
	unsigned long	m_hold_time;			//in seconds average wait time (per query)
	unsigned long	m_exec_time;			//in seconds average exec time (per query)
	unsigned long	m_suspended_time;		//in seconds average suspended time (per query)

} SERVICE_STATS_DATA, *pSERVICE_STATS_DATA;

typedef struct _WMS_STATS_DATA
{
	bool			m_reset;
	int64			m_startWmsStats;
;
	unsigned long	m_total_queries;
	unsigned long	m_total_rejected;
	unsigned long	m_total_canceled;
	unsigned long	m_total_completed;
	unsigned long	m_total_wait;
	unsigned long	m_total_hold;
	unsigned long	m_total_exec;
	unsigned long	m_total_suspended;

	unsigned long	m_total_wait_time;			//in seconds total wait time
	unsigned long	m_total_hold_time;			//in seconds total hold time
	unsigned long	m_total_exec_time;			//in seconds total exec time
	unsigned long	m_total_suspended_time;		//in seconds total suspended time

	unsigned long	m_wait_time;			//in seconds average wait time (per query)
	unsigned long	m_hold_time;			//in seconds average wait time (per query)
	unsigned long	m_exec_time;			//in seconds average exec time (per query)
	unsigned long	m_suspended_time;		//in seconds average suspended time (per query)
//
// this part is for wms health
//
	unsigned long	m_delta_total_queries;
	unsigned long	m_delta_total_rejected;
	unsigned long	m_delta_total_canceled;
	unsigned long	m_delta_total_completed;
	unsigned long	m_delta_total_wait;
	unsigned long	m_delta_total_hold;
	unsigned long	m_delta_total_exec;
	unsigned long	m_delta_total_suspended;

	unsigned long	m_delta_total_wait_time;			//in seconds
	unsigned long	m_delta_total_hold_time;			//in seconds
	unsigned long	m_delta_total_exec_time;			//in seconds
	unsigned long	m_delta_total_suspended_time;		//in seconds

	unsigned long	m_delta_conn_rule_triggered;	//conn rules
	unsigned long	m_delta_comp_rule_triggered;	//comp rules
	unsigned long	m_delta_exec_rule_triggered;	//exec rules

	short				m_service_count;
	SERVICE_STATS_DATA	m_service[1];

} WMS_STATS_DATA, *pWMS_STATS_DATA;
//
//===================================================================================
//============================ CHECK_SEGMENTS_BY_SERVICE ====================================
//===================================================================================
//
typedef struct CHECK_SEGMENTS_
{
	short operation;
	short function;
	short state;
	short found_state;
	char  segment[MAX_NODE_NAME+1];
	char  ServiceName[MAX_SERVICE_NAME_LEN + 1];
	WMS_STATS_DATA stats_data;
	long  cur_exec;
	long  cur_wait;
	long  cur_hold;
	long  cur_suspend;
	long  service_cur_exec;
	long  service_cur_wait;
	long  service_cur_hold;
	long  service_cur_suspend;

} CHECK_SEGMENTS,*pCHECK_SEGMENTS;
//
//===================================================================================
//============================ offender ============================================
//===================================================================================
//

typedef struct _OFFENDER_MEM_ITEM
{
	SLOT_STATE			m_slot_state;					// 0 - slot is empty, 1 - query assigned;
//
//--------- for SQ offender ----------------------------------
/*
following variables are used in SQ offender:
	m_sq_node_name	is mapped to node_name
	m_cpu			is mapped to nid
	m_pin			is mapped to os_pid
	m_pcpu
	m_pmem			is mapped to pmem
	m_mem			is mapped to rss
	m_sq_vsz		is mapped to vsz
	m_prty
	m_type
	m_procname
	m_progname
*/
	unsigned char		m_pcpu;							// cpu in %
	unsigned char		m_pmem;							// mem in %
	int					m_mem;							// memory usage in M/Kbytes
	int					m_sq_vsz;						// virtual memory
	char				m_sq_node_name[256];
//
	long				m_segment;
	short				m_cpu;							// cpu number
	short				m_pin;							// process pin
	short				m_prty;
	PROCESS_TYPE		m_type;							// type of process: MXOSRVR, ESP,CMP,UDR,OTHER
	char				m_procname[32];					// process name
	char				m_progname[256];				// program name
//
//========== PARENT ===========================
//
/*
following variables are used in SQ offender:
	m_parent_sq_node_name	is mapped to node_name
	m_parent_cpu			is mapped to nid
	m_parent_pin			is mapped to os_pid
	m_parent_prty
	m_parent_ptype
	m_parent_procname
	m_parent_progname
*/
	char		m_parent_query_id[MAX_QUERY_ID_LEN];
	char				m_parent_sq_node_name[256];
	long				m_parent_seg_number;
	int					m_parent_cpu;
	int					m_parent_pin;
	short				m_parent_prty;
	PROCESS_TYPE		m_parent_ptype;
	char				m_parent_procname[32];
	char				m_parent_progname[256];
//
//========== from WMS ==========================
//
	char				m_query_id[MAX_QUERY_ID_LEN];		// if any
	char				m_tcpStatus[10];
	unsigned short		m_port;
	long				m_ExecutionElapsedTime;				// elapsed time in seconds
	long				m_wait_time;						// wait time in seconds
	long				m_hold_time;						// hold time in seconds
	char				m_ds_name[MAX_DSOURCE_NAME + 1];	// Datasource Name
	char				m_query_name[MAX_QUERYNAME_LEN + 1];// Query Name
	char				m_user_name[MAX_NAME_LEN + 1];		// Role Name
	char				m_user[MAX_NAME_LEN + 1];			// User alias name
	char				m_sql_text[RMS_STORE_SQL_SOURCE_LEN + 1];		// Only 254 bytes

} OFFENDER_MEM_ITEM, *pOFFENDER_MEM_ITEM;

typedef struct _OFFENDER_CPU_ITEM
{
	SLOT_STATE			m_slot_state;					// 0 - slot is empty, 1 - query assigned;
//
	unsigned char		m_pcpu;							// cpu in %
	unsigned char		m_pmem;							// mem in %
	int					m_mem;							// memory usage in MBytes
	int					m_sq_vsz;						// virtual memory
	char				m_sq_node_name[256];
//
//
	long				m_segment;
	int					m_cpu;							// cpu number
	int					m_pin;							// process pin
	short				m_prty;
	PROCESS_TYPE		m_type;							// type of process: MXOSRVR, ESP,CMP,UDR,OTHER
	char				m_procname[32];					// process name
	char				m_progname[256];				// program name
//
//========== PARENT ===========================
//
	char		m_parent_query_id[MAX_QUERY_ID_LEN];
	char				m_parent_sq_node_name[256];
	long				m_parent_seg_number;
	int					m_parent_cpu;
	int					m_parent_pin;
	short				m_parent_prty;
	PROCESS_TYPE		m_parent_ptype;
	char				m_parent_procname[32];
	char				m_parent_progname[256];
//
//========== from WMS ==========================
//
	char				m_query_id[MAX_QUERY_ID_LEN];		// if any
	char				m_tcpStatus[10];
	unsigned short		m_port;
	long				m_ExecutionElapsedTime;				// elapsed time in seconds
	long				m_wait_time;						// wait time in seconds
	long				m_hold_time;						// hold time in seconds
	char				m_ds_name[MAX_DSOURCE_NAME + 1];	// Datasource Name
	char				m_query_name[MAX_QUERYNAME_LEN + 1];// Query Name
	char				m_user_name[MAX_NAME_LEN + 1];		// Role Name
	char				m_user[MAX_NAME_LEN];				// User alias name
	char				m_sql_text[RMS_STORE_SQL_SOURCE_LEN + 1];		// Only 254 bytes

} OFFENDER_CPU_ITEM, *pOFFENDER_CPU_ITEM;

typedef struct _OFFENDER_RMS_CPU_ITEM
{
	SLOT_STATE			m_slot_state;					// 0 - slot is empty, 1 - query assigned;
//
	unsigned char		m_pcpu;							// delta of CpuTime in % between samples
	unsigned char		m_pmem;							// mem in %
	int64				m_delta;
//
	long				m_segment;
	short				m_cpu;
	short				m_pin;
	short				m_prty;
	PROCESS_TYPE		m_type;							// type of process: MXOSRVR, ESP,CMP,UDR,OTHER
	char				m_process_id[17];
	char				m_procname[32];					// process name
	char				m_progname[10];					// program name
//
//========== PARENT ===========================
//
	long				m_parent_seg_number;
	short				m_parent_cpu;
	short				m_parent_pin;
	short				m_parent_prty;
	PROCESS_TYPE		m_parent_type;
	char				m_parent_procname[32];
	char				m_parent_progname[10];
//
	char				m_query_id[MAX_QUERY_ID_LEN];	// Query ID
	int64				m_CpuTime;						//CpuTime: 3702 
	int64				m_SpaceUsed;					//SpaceUsed: 1011 
	int64				m_SpaceTotal;					//SpaceTotal: 1088 
	int64				m_HeapUsed;						//HeapUsed: 30 
	int64				m_HeapTotal;					//HeapTotal: 39

} OFFENDER_RMS_CPU_ITEM, *pOFFENDER_RMS_CPU_ITEM;
//
// RMS MEM Offender is not implemented yet
//
/*
typedef struct _OFFENDER_RMS_MEM_ITEM
{
	SLOT_STATE			m_slot_state;					// 0 - slot is empty, 1 - query assigned;
	long				m_segment;
	short				m_cpu;
	short				m_pin;
	char				m_process_id[17];
	char				m_query_id[MAX_QUERY_ID_LEN];	// Query ID
	unsigned char		m_pcpu;							// cpu in %
	unsigned short		m_mem;							// memory usage in M/Kbytes

	int64				m_CpuTime;						//CpuTime: 3702 
	int64				m_SpaceUsed;					//SpaceUsed: 1011 
	int64				m_SpaceTotal;					//SpaceTotal: 1088 
	int64				m_HeapUsed;						//HeapUsed: 30 
	int64				m_HeapTotal;					//HeapTotal: 39

} OFFENDER_RMS_MEM_ITEM, *pOFFENDER_RMS_MEM_ITEM;
*/
typedef struct _OFFENDER_RMS_DISK_ITEM
{
	SLOT_STATE			m_slot_state;					// 0 - slot is empty, 1 - query assigned;
//
	unsigned char		m_pcpu;							// delta of ProcessBusyTime in % between samples
//
	long				m_segment;
	short				m_cpu;
	short				m_pin;
	short				m_prty;
	PROCESS_TYPE		m_type;							// type of process: MXOSRVR, ESP,CMP,UDR,OTHER
	char				m_process_id[17];
	char				m_procname[17];					// process name
	char				m_progname[10];					// program name
//
	char				m_query_id[MAX_QUERY_ID_LEN];	// Query ID
	char				m_AnsiName[4 * 256];			//AnsiName: MANAGEABILITY.METRICS.REPOSITORY_MEASURE_CPU_STATS 
	int64				m_EstRowsAccessed;				//EstRowsAccessed: 0 
	int64				m_EstRowsUsed;					//EstRowsUsed: 0 
	int64				m_NumMessages;					//NumMessages: 1 
	int64				m_MessagesBytes;				//MessagesBytes: 872 
	int64				m_StatsBytes;					//StatsBytes: 232 
	int64				m_AccessedRows;					//AccessedRows: 0 
	int64				m_UsedRows;						//UsedRows: 0 
	int64				m_DiskIOs;						//DiskIOs: 0 
	int64				m_Escalations;					//Escalations: 0 
	int64				m_LockWaits;					//LockWaits: 0 
	int64				m_ProcessBusyTime;				//ProcessBusyTime: 39
	int64				m_Opens;						//Opens: 0 
	int64				m_OpenTime;						//OpenTime: 0

} OFFENDER_RMS_DISK_ITEM, *pOFFENDER_RMS_DISK_ITEM;

typedef struct _TRANS_QUERIES_LIST
{
	int64                           m_query_start_time;
        char                            m_query_id[MAX_QUERY_ID_LEN+1];
	QUERY_STATE        		m_query_state;
        char    			m_sqlSrc[RMS_STORE_SQL_SOURCE_LEN + 1]; //      pre SQL Text (only first 254 bytes)
}TRANS_QUERIES_LIST,*pTRANS_QUERIES_LIST;


typedef struct _OFFENDER_TXN_ITEM
{
	SLOT_STATE                      m_slot_state;
        int64                           m_txnId;                                                //transaction Id
        char                            m_txnText[MAX_TXN_STR_LEN+1];
        int64                           m_txnStartTime;
        char                            m_txnStartTimeText[50];
        long                            m_txnDuration;
        long                            m_rollbackDuration;
        long                            m_txnNode;
        long                            m_txnSeq;
        char                            m_txnProcName[MS_MON_MAX_PROCESS_NAME+1];
        char                            m_txnProgName[MS_MON_MAX_PROCESS_PATH+1];
        short                           m_txnState;
	TRANS_QUERIES_LIST		m_queries[MAX_TXN_QUERIES];
	

} OFFENDER_TXN_ITEM, *pOFFENDER_TXN_ITEM;


typedef struct _OFFENDER_LIST
{
	int64				m_LastUpdate;
//
//=========== for communication with Offender
//
	char				m_QSOffndProcessName[MAX_PROCESS_NAME_LEN];
//
	OFFENDER_CPU_ITEM	m_OffenderCpuListSQL[OFFENDER_CACHE];
	OFFENDER_CPU_ITEM	m_OffenderCpuListALL[OFFENDER_CACHE];
	OFFENDER_MEM_ITEM	m_OffenderMemListSQL[OFFENDER_CACHE];
	OFFENDER_MEM_ITEM	m_OffenderMemListALL[OFFENDER_CACHE];

	OFFENDER_RMS_CPU_ITEM	m_OffenderRmsCpuList[OFFENDER_CACHE];
//	OFFENDER_RMS_MEM_ITEM	m_OffenderRmsMemList[OFFENDER_CACHE];
	OFFENDER_RMS_DISK_ITEM	m_OffenderRmsDiskList[OFFENDER_CACHE];

	OFFENDER_CPU_ITEM	m_OffenderCPUSelList[OFFENDER_CACHE];
	OFFENDER_MEM_ITEM	m_OffenderMEMSelList[OFFENDER_CACHE];
//
	OFFENDER_TXN_ITEM	m_OffenderTXNList[OFFENDER_CACHE];
	
} OFFENDER_LIST, *pOFFENDER_LIST;
//
//===================================================================================
//============================ Parse result info ====================================
//===================================================================================
//
typedef struct _qrysrvc_rule_list_map {
	IDL_char	ruleName[MAX_RULE_NAME_LEN + 1];
	IDL_char	srvcName[MAX_SERVICE_NAME_LEN + 1];
	IDL_short	priority;
} qrysrvc_rule_list_map;

typedef struct _qrysrvc_rule_list {
	IDL_short				type;
	IDL_short				ruleCnt;
	qrysrvc_rule_list_map	map[MAX_LIST_SIZE];
} qrysrvc_rule_list;

typedef struct _qrysrvc_rule_expr {
	IDL_short		priority;
	IDL_long		lopnd;
	IDL_short		oper;
	IDL_short		operNum;
	IDL_long		ropnd;
	IDL_long_long	ropndNum;
	IDL_char		ropndStr[MAX_ROPND_STR_LEN + 1];
} qrysrvc_rule_expr;

typedef struct _qrysrvc_aggregate_spec
{
	IDL_long			queryTypes;
	IDL_boolean			queryTypesFlag;
	IDL_short			reposInterval;
	IDL_boolean			reposIntervalFlag;
	IDL_short			wmsInterval;
	IDL_boolean			wmsIntervalFlag;
	IDL_short			execInterval;
	IDL_boolean			execIntervalFlag;
	IDL_short			statsOnce;			// perf
	IDL_boolean			statsOnceFlag;
} qrysrvc_aggregate_spec;

typedef struct _qrysrvc_rule_spec {
	IDL_short			type;
	IDL_char			name[MAX_RULE_NAME_LEN + 1];
	IDL_short			oper;
	IDL_boolean			operFlag;
	IDL_short			exprCnt;
	qrysrvc_rule_expr	exprList[MAX_EXPR_SIZE];
	IDL_boolean			exprListFlag;
	IDL_short			action;
	IDL_boolean			actionFlag;
	IDL_char			sqlCmds[MAX_SQL_CMD_LEN + 1];
	IDL_short			numSqlCmdsBytes;
	IDL_boolean			sqlCmdsFlag;
	IDL_short			warnLevel;
	IDL_boolean			warnLevelFlag;
	IDL_short			numCommentChars;
	IDL_char			comment[MAX_COMMENT_LEN + 1];
	IDL_boolean			commentFlag;
	qrysrvc_aggregate_spec aggregateSpec;
} qrysrvc_rule_spec;


struct qrysrvc_parseInfo_def;

struct qrysrvc_short_parseInfo_def {

	  qrysrvc_short_parseInfo_def() :
			  reqType(0)
			, maxCpuBusyFlag(false)
			, maxCpuBusy(0)
			, maxMemUsageFlag(false)
			, maxMemUsage(0)
	  	  	, maxCurExecQueriesFlag(false)
	   	    , maxCurExecQueries(0)
			, statsIntervalFlag(false)
			, statsInterval(0)
			, priorityFlag(false)
			, priority(0)
			, activeFlag(false)
			, activeBegMins(0)
			, activeEndMins(0)
			, commentFlag(false)
			, numCommentChars(0)
			, queryIdFlag(false)
			, serviceNameFlag(false)
			, numServiceNameChars(0)
			, roleFlag(false)
			, traceFlag(false)
			, traceQS(false)
			, traceStats(false)
			, traceCom(false)
			, traceSyncAS(false)
			, traceSync(false)
			, traceRule(false)
	  	  	, traceOffndr(false)
			, traceElapsed(false)
			, traceFileNameFlag(false)
			, traceFilePathFlag(false)
			, segmentNumFlag(false)
			, segmentNum(0)
			, sqlPlanFlag(false)
			, sqlPlan(false)
			, sqlTextFlag(false) 
			, sqlText(false)
			, simulateFlag(false)
			, simulateCount(0)
			, execTimeoutFlag(false)
			, execTimeout(0)
			, waitTimeoutFlag(false)
			, waitTimeout(0)
			, holdTimeoutFlag(false)
			, holdTimeout(0)
			, sqlDefaultsFlag(false)
			, ruleInterval(0)
			, ruleIntervalFlag(false)
			, ruleIntervalExecTime(0)
			, ruleIntervalExecTimeFlag(false)
			, maxRowsFetched(0)
			, maxRowsFetchedFlag(false)
			, priorityOverride(false)
			, ruleListFlag(false)
			, offenderFlag(false)
			, offenderProcFlag(false)
			, offenderProc(0)
			, offenderSampleIntervalFlag(false)
			, offenderSampleInterval(0)
			, offenderSampleCpusFlag(false)
			, offenderSampleCpus(0)
			, offenderSampleCacheFlag(false)
			, offenderSampleCache(0)
			, offenderCpuNumFlag(false)
			, offenderCpuNum(0)
			, offenderChildrenFlag(false) 
			, offenderProcDetailFlag(false) 
			, pstateFlag(false)
			, maxEspsFlag(false)
			, maxEsps(0)
			, canaryIntervalMinFlag(false)
			, canaryIntervalMin(0)
			, canaryExecSecFlag(false)
			, canaryExecSec(0)
			, canaryQuerySrcFlag(false)
			, numCanaryQueryChars(false)
			, cancelIfClientDisappearFlag(false)
			, numCancelIfClientDisappear(0)
			, checkQueryEstResourceUseFlag(false)
			, numCheckQueryEstResourceUse(0)
			, transIdFlag(false)
                        , transId(0)
                        , maxTxnRollMinFlag(false)
                        , maxTxnRollMin(0)
//
	  {
		bzero(active, sizeof(active));
		bzero(comment, sizeof(comment));
		bzero(queryId, sizeof(queryId));
		bzero(serviceName, sizeof(serviceName));
		bzero(role, sizeof(role));
		bzero(traceFileName, sizeof(traceFileName));
		bzero(traceFilePath, sizeof(traceFilePath));
		bzero(sqlDefaults, sizeof(sqlDefaults));
		bzero(clipsFileName, sizeof(clipsFileName));
		bzero(offenderChildren, sizeof(offenderChildren));
		bzero(offenderProcDetail, sizeof(offenderProcDetail));
		bzero(pstate, sizeof(pstate));
		bzero(&ruleSpec, sizeof(ruleSpec));

		requesterType = REQUESTER_INIT;
		bzero(requesterApplication, sizeof(requesterApplication));
		bzero(requesterComputer, sizeof(requesterComputer));
		bzero(requesterName, sizeof(requesterName));
		bzero(requesterRole, sizeof(requesterRole));
		bzero(requesterDBUserName, sizeof(requesterDBUserName));
		//ssd overflow
		maxOverflowUsageFlag = false;
		maxOverflowUsage = 0;

		bzero(canaryQuerySrc, sizeof(canaryQuerySrc));
		bzero(cancelIfClientDisappear, sizeof(cancelIfClientDisappear));
		bzero(checkQueryEstResourceUse, sizeof(checkQueryEstResourceUse));
		//operator privileges
		bzero(privMask, sizeof(bitmask_type));
	  };

	qrysrvc_short_parseInfo_def& operator=(const qrysrvc_parseInfo_def& rhs); 

	IDL_short			reqType;
	IDL_boolean			maxCpuBusyFlag;
	IDL_short			maxCpuBusy;
	IDL_boolean			maxMemUsageFlag;
	IDL_short			maxMemUsage;
	IDL_boolean         maxCurExecQueriesFlag;
	IDL_short           maxCurExecQueries;
	IDL_boolean			statsIntervalFlag;
	IDL_short			statsInterval;
	IDL_boolean			priorityFlag;
	IDL_short			priority;
	IDL_boolean			activeFlag;
	IDL_char			active[MAX_ACTIVE_TIME_LEN + 1];
	IDL_short			activeBegMins;
	IDL_short			activeEndMins;
	IDL_boolean			commentFlag;
	IDL_char			comment[MAX_COMMENT_LEN + 1];
	IDL_short			numCommentChars;
	IDL_boolean			queryIdFlag;
	IDL_char			queryId[MAX_QUERY_ID_LEN + 1];
	IDL_boolean			serviceNameFlag;
	IDL_char			serviceName[MAX_SERVICE_NAME_LEN + 1];
	IDL_short			numServiceNameChars;
	IDL_char			roleFlag;
	IDL_char			role[MAX_ALIAS_LEN];
	IDL_boolean			traceFlag;
	IDL_boolean			traceQS;
	IDL_boolean			traceStats;
	IDL_boolean			traceCom;
	IDL_boolean			traceSyncAS;
	IDL_boolean			traceSync;
	IDL_boolean			traceRule;
	IDL_boolean			traceOffndr;
	IDL_boolean			traceElapsed;
	IDL_boolean			traceFileNameFlag;
	IDL_char			traceFileName[20 + 1];
	IDL_boolean			traceFilePathFlag;
	IDL_char			traceFilePath[MAX_NAME_LEN + 1];
	IDL_boolean			segmentNumFlag;
	IDL_short			segmentNum;//used by restart & status memory commands
	IDL_boolean			sqlPlanFlag;
	IDL_boolean			sqlPlan;
	IDL_boolean			sqlTextFlag;
	IDL_boolean			sqlText;
	IDL_boolean			simulateFlag;
	IDL_short			simulateCount;
	IDL_boolean			execTimeoutFlag;
	IDL_short			execTimeout;
	IDL_boolean			waitTimeoutFlag;
	IDL_short			waitTimeout;
	IDL_boolean			holdTimeoutFlag;
	IDL_short			holdTimeout;
	IDL_char			sqlDefaults[MAX_SQL_CMD_LEN + 1];
	IDL_boolean			sqlDefaultsFlag;
	IDL_short			ruleInterval;
	IDL_boolean			ruleIntervalFlag;
	IDL_short			ruleIntervalExecTime;
	IDL_boolean			ruleIntervalExecTimeFlag;
	IDL_char			clipsFileName[20 + 1];
	IDL_long_long		maxRowsFetched;
	IDL_boolean			maxRowsFetchedFlag;
	qrysrvc_rule_spec	ruleSpec;
	IDL_short			ruleListFlag;
//
	IDL_boolean			priorityOverride; // { PRIORITY | PRIORITY_OVERRIDE }
	IDL_boolean			offenderFlag;
	IDL_boolean			offenderProcFlag;
	IDL_short			offenderProc;
	IDL_boolean			offenderSampleIntervalFlag;
	IDL_short			offenderSampleInterval;
	IDL_boolean			offenderSampleCpusFlag;
	IDL_short			offenderSampleCpus;
	IDL_boolean			offenderSampleCacheFlag;
	IDL_short			offenderSampleCache;
	IDL_boolean			offenderCpuNumFlag; // status cpu [cpuNum] [segment <segmentNum>]
	IDL_short			offenderCpuNum;
	IDL_boolean			offenderChildrenFlag; 
	IDL_char			offenderChildren[MAX_PROCESS_LEN + 1];
	IDL_boolean			offenderProcDetailFlag; 
	IDL_char			offenderProcDetail[MAX_PROCESS_LEN + 1];
	IDL_boolean			pstateFlag; 
	IDL_char			pstate[MAX_PROCESS_LEN + 1];
//
// info about requester
//
	REQUESTER			requesterType;
	IDL_char			requesterApplication[MAX_APPLICATION_NAME_LEN +1];
	IDL_char			requesterComputer[MAX_COMPUTER_NAME_LEN + 1];
	IDL_char			requesterName[USERNAME_LENGTH + 1];
	IDL_char			requesterRole[MAX_ROLE_LEN + 1];
	IDL_char			requesterDBUserName[MAX_ROLE_LEN + 1];
//ssd overflow
	IDL_boolean         maxOverflowUsageFlag;
	IDL_short           maxOverflowUsage;

	IDL_boolean         canaryIntervalMinFlag;
	IDL_short           canaryIntervalMin;
	IDL_boolean         canaryExecSecFlag;
	IDL_short           canaryExecSec;
	IDL_boolean         canaryQuerySrcFlag;
	IDL_char            canaryQuerySrc[MAX_CANARY_LEN + 1];
	IDL_short           numCanaryQueryChars;
	IDL_boolean         cancelIfClientDisappearFlag;
	IDL_char            cancelIfClientDisappear[6];
	IDL_short           numCancelIfClientDisappear;
	IDL_boolean         checkQueryEstResourceUseFlag;
	IDL_char            checkQueryEstResourceUse[6];
	IDL_short           numCheckQueryEstResourceUse;
	IDL_boolean         transIdFlag; 
        long            transId;
        IDL_boolean         maxTxnRollMinFlag;
        IDL_short           maxTxnRollMin;
// Component privileges
	bitmask_type 		privMask;
	IDL_boolean			maxEspsFlag;
	IDL_short			maxEsps;

};


struct qrysrvc_parseInfo_def {

	  qrysrvc_parseInfo_def() :
			  reqType(0)
			, maxCpuBusyFlag(false)
			, maxCpuBusy(0)
			, maxMemUsageFlag(false)
			, maxMemUsage(0)
	  	  	, maxCurExecQueriesFlag(false)
	  	    , maxCurExecQueries(0)
			, statsIntervalFlag(false)
			, statsInterval(0)
			, priorityFlag(false)
			, priority(0)
			, activeFlag(false)
			, activeBegMins(0)
			, activeEndMins(0)
			, commentFlag(false)
			, numCommentChars(0)
			, queryIdFlag(false)
			, serviceNameFlag(false)
			, numServiceNameChars(0)
			, roleFlag(false)
			, traceFlag(false)
			, traceQS(false)
			, traceStats(false)
			, traceCom(false)
			, traceSyncAS(false)
			, traceSync(false)
			, traceRule(false)
	        , traceOffndr(false)
			, traceElapsed(false)
			, traceFileNameFlag(false)
			, traceFilePathFlag(false)
			, segmentNumFlag(false)
			, segmentNum(0)
			, sqlPlanFlag(false)
			, sqlPlan(false)
			, sqlTextFlag(false) 
			, sqlText(false)
			, simulateFlag(false)
			, simulateCount(0)
			, execTimeoutFlag(false)
			, execTimeout(0)
			, waitTimeoutFlag(false)
			, waitTimeout(0)
			, holdTimeoutFlag(false)
			, holdTimeout(0)
			, sqlDefaultsFlag(false)
			, ruleInterval(0)
			, ruleIntervalFlag(false)
			, ruleIntervalExecTime(0)
			, ruleIntervalExecTimeFlag(false)
			, maxRowsFetched(0)
			, maxRowsFetchedFlag(false)
			, priorityOverride(false)
			, ruleListFlag(false)
			, offenderFlag(false)
			, offenderProcFlag(false)
			, offenderProc(0)
			, offenderSampleIntervalFlag(false)
			, offenderSampleInterval(0)
			, offenderSampleCpusFlag(false)
			, offenderSampleCpus(0)
			, offenderSampleCacheFlag(false)
			, offenderSampleCache(0)
			, offenderCpuNumFlag(false)
			, offenderCpuNum(0)
			, offenderChildrenFlag(false) 
			, offenderProcDetailFlag(false) 
			, pstateFlag(false)
			, maxEspsFlag(false)
			, maxEsps(0)
			, canaryIntervalMinFlag(false)
			, canaryIntervalMin(0)
			, canaryExecSecFlag(false)
			, canaryExecSec(0)
			, canaryQuerySrcFlag(false)
			, numCanaryQueryChars(false)
			, cancelIfClientDisappearFlag(false)
			, numCancelIfClientDisappear(0)
			, checkQueryEstResourceUseFlag(false)//Gary 20130426
			, numCheckQueryEstResourceUse(0)
			, transIdFlag(false)
                        , transId(0)
                        , maxTxnRollMinFlag(false)
                        , maxTxnRollMin(0)
//
		{
				bzero(active, sizeof(active));
				bzero(comment, sizeof(comment));
				bzero(queryId, sizeof(queryId));
				bzero(serviceName, sizeof(serviceName));
				bzero(role, sizeof(role));
				bzero(traceFileName, sizeof(traceFileName));
				bzero(traceFilePath, sizeof(traceFilePath));
				bzero(sqlDefaults, sizeof(sqlDefaults));
				bzero(clipsFileName, sizeof(clipsFileName));
				bzero(offenderChildren, sizeof(offenderChildren));
				bzero(offenderProcDetail, sizeof(offenderProcDetail));
				bzero(pstate, sizeof(pstate));
				bzero(&ruleSpec, sizeof(ruleSpec));

				bzero(&ruleList, sizeof(ruleList));

				requesterType = REQUESTER_INIT;
				bzero(requesterApplication, sizeof(requesterApplication));
				bzero(requesterComputer, sizeof(requesterComputer));
 				bzero(requesterName, sizeof(requesterName));
 				bzero(requesterRole, sizeof(requesterRole));
 				bzero(requesterDBUserName, sizeof(requesterDBUserName));
				//ssd overflow	
				maxOverflowUsageFlag = false;
				maxOverflowUsage = 0;

				bzero(canaryQuerySrc, sizeof(canaryQuerySrc));
				bzero(cancelIfClientDisappear, sizeof(cancelIfClientDisappear));
				bzero(checkQueryEstResourceUse, sizeof(checkQueryEstResourceUse));
				bzero(privMask, sizeof(bitmask_type));

		};

	qrysrvc_parseInfo_def& operator=(const qrysrvc_short_parseInfo_def& rhs) 
	{
		reqType					= rhs.reqType;
		maxCpuBusyFlag			= rhs.maxCpuBusyFlag;
		maxCpuBusy				= rhs.maxCpuBusy;
		maxMemUsageFlag			= rhs.maxMemUsageFlag;
		maxMemUsage				= rhs.maxMemUsage;
		maxCurExecQueriesFlag	= rhs.maxCurExecQueriesFlag;
		maxCurExecQueries		= rhs.maxCurExecQueries;
		statsIntervalFlag		= rhs.statsIntervalFlag;
		statsInterval			= rhs.statsInterval;
		priorityFlag			= rhs.priorityFlag;
		priority				= rhs.priority;
		activeFlag				= rhs.activeFlag;
		memcpy(active,	rhs.active, sizeof(active));
		activeBegMins			= rhs.activeBegMins;
		activeEndMins			= rhs.activeEndMins;
		commentFlag				= rhs.commentFlag;
		memcpy(comment, rhs.comment, sizeof(comment));
		numCommentChars			= rhs.numCommentChars;
		queryIdFlag				= rhs.queryIdFlag;
		memcpy(queryId, rhs.queryId, sizeof(queryId));
		serviceNameFlag			= rhs.serviceNameFlag;
		memcpy(serviceName, rhs.serviceName, sizeof(serviceName));
		numServiceNameChars		= rhs.numServiceNameChars;
		roleFlag				= rhs.roleFlag;
		memcpy(role, rhs.role, sizeof(role));
		traceFlag				= rhs.traceFlag;
		traceQS					= rhs.traceQS;
		traceStats				= rhs.traceStats;
		traceCom				= rhs.traceCom;
		traceSyncAS				= rhs.traceSyncAS;
		traceSync				= rhs.traceSync;
		traceRule				= rhs.traceRule;
		traceOffndr				= rhs.traceOffndr;
		traceElapsed			= rhs.traceElapsed;
		traceFileNameFlag		= rhs.traceFileNameFlag;
		memcpy(traceFileName, rhs.traceFileName, sizeof(traceFileName));
		traceFilePathFlag		= rhs.traceFilePathFlag;
		memcpy(traceFilePath, rhs.traceFilePath, sizeof(traceFilePath));
		segmentNumFlag			= rhs.segmentNumFlag;
		segmentNum				= rhs.segmentNum;
		sqlPlanFlag				= rhs.sqlPlanFlag;
		sqlPlan					= rhs.sqlPlan;
		sqlTextFlag				= rhs.sqlTextFlag;
		sqlText					= rhs.sqlText;
		simulateFlag			= rhs.simulateFlag;
		simulateCount			= rhs.simulateCount;
		execTimeoutFlag			= rhs.execTimeoutFlag;
		execTimeout				= rhs.execTimeout;
		waitTimeoutFlag			= rhs.waitTimeoutFlag;
		waitTimeout				= rhs.waitTimeout;
		holdTimeoutFlag			= rhs.holdTimeoutFlag;
		holdTimeout				= rhs.holdTimeout;
		memcpy(sqlDefaults, rhs.sqlDefaults, sizeof(sqlDefaults));
		sqlDefaultsFlag			= rhs.sqlDefaultsFlag;
		ruleInterval			= rhs.ruleInterval;
		ruleIntervalFlag		= rhs.ruleIntervalFlag;
		ruleIntervalExecTime	= rhs.ruleIntervalExecTime;
		ruleIntervalExecTimeFlag = rhs.ruleIntervalExecTimeFlag;
		memcpy(clipsFileName, rhs.clipsFileName, sizeof(clipsFileName));
		maxRowsFetched			= rhs.maxRowsFetched;
		maxRowsFetchedFlag		= rhs.maxRowsFetchedFlag;
		ruleSpec				= rhs.ruleSpec;
		ruleListFlag			= rhs.ruleListFlag;
//
		bzero(&ruleList, sizeof(ruleList));
//
		priorityOverride		= rhs.priorityOverride;
		offenderFlag			= rhs.offenderFlag;
		offenderProcFlag		= rhs.offenderProcFlag;
		offenderProc			= rhs.offenderProc;
		offenderSampleIntervalFlag = rhs.offenderSampleIntervalFlag;
		offenderSampleInterval	= rhs.offenderSampleInterval;
		offenderSampleCpusFlag	= rhs.offenderSampleCpusFlag;
		offenderSampleCpus		= rhs.offenderSampleCpus;
		offenderSampleCacheFlag	= rhs.offenderSampleCacheFlag;
		offenderSampleCache		= rhs.offenderSampleCache;
		offenderCpuNumFlag		= rhs.offenderCpuNumFlag;
		offenderCpuNum			= rhs.offenderCpuNum;
		offenderChildrenFlag	= rhs.offenderChildrenFlag; 
		memcpy(offenderChildren, rhs.offenderChildren, sizeof(offenderChildren));
		offenderProcDetailFlag	= rhs.offenderProcDetailFlag; 
		memcpy(offenderProcDetail, rhs.offenderProcDetail, sizeof(offenderProcDetail));
		pstateFlag				= rhs.pstateFlag; 
		memcpy(pstate, rhs.pstate, sizeof(pstate));
//
// info about requester
//
		requesterType			= rhs.requesterType;
		memcpy(requesterApplication, rhs.requesterApplication, sizeof(requesterApplication));
		memcpy(requesterComputer, rhs.requesterComputer, sizeof(requesterComputer));
		memcpy(requesterName, rhs.requesterName, sizeof(requesterName));
		memcpy(requesterRole, rhs.requesterRole, sizeof(requesterRole));
		memcpy(requesterDBUserName, rhs.requesterDBUserName, sizeof(requesterDBUserName));
//ssd overflow
		maxOverflowUsageFlag	= rhs.maxOverflowUsageFlag;
		maxOverflowUsage		= rhs.maxOverflowUsage;

		canaryIntervalMinFlag   = rhs.canaryIntervalMinFlag;
		canaryIntervalMin       = rhs.canaryIntervalMin;
		canaryExecSecFlag       = rhs.canaryExecSecFlag;
		canaryExecSec           = rhs.canaryExecSec;
		canaryQuerySrcFlag      = rhs.canaryQuerySrcFlag;
		memcpy(canaryQuerySrc, rhs.canaryQuerySrc, sizeof(canaryQuerySrc));
		numCanaryQueryChars     = rhs.numCanaryQueryChars;
		cancelIfClientDisappearFlag      = rhs.cancelIfClientDisappearFlag;
		memcpy(cancelIfClientDisappear, rhs.cancelIfClientDisappear , sizeof(cancelIfClientDisappear));
		numCancelIfClientDisappear     = rhs.numCancelIfClientDisappear;
		checkQueryEstResourceUseFlag      = rhs.checkQueryEstResourceUseFlag;
		memcpy(checkQueryEstResourceUse, rhs.checkQueryEstResourceUse , sizeof(checkQueryEstResourceUse));
		numCheckQueryEstResourceUse     = rhs.numCheckQueryEstResourceUse;
		transIdFlag     = rhs.transIdFlag;
                transId     = rhs.transId;
                maxTxnRollMinFlag  = rhs.maxTxnRollMinFlag;
                maxTxnRollMin     = rhs.maxTxnRollMin; 
//operator privileges
		//operator privileges
		memcpy(privMask, rhs.privMask, sizeof(bitmask_type));
		maxEspsFlag			= rhs.maxEspsFlag;
		maxEsps				= rhs.maxEsps;

		return *this;
	};

	void copy_to_short(qrysrvc_short_parseInfo_def* rhs)
	{
		rhs->reqType				= reqType;
		rhs->maxCpuBusyFlag			= maxCpuBusyFlag;
		rhs->maxCpuBusy				= maxCpuBusy;
		rhs->maxMemUsageFlag		= maxMemUsageFlag;
		rhs->maxMemUsage			= maxMemUsage;
		rhs->maxCurExecQueriesFlag	= maxCurExecQueriesFlag;
		rhs->maxCurExecQueries		= maxCurExecQueries;

		rhs->statsIntervalFlag		= statsIntervalFlag;
		rhs->statsInterval			= statsInterval;
		rhs->priorityFlag			= priorityFlag;
		rhs->priority				= priority;
		rhs->activeFlag				= activeFlag;
		memcpy(rhs->active,	active, sizeof(active));
		rhs->activeBegMins			= activeBegMins;
		rhs->activeEndMins			= activeEndMins;
		rhs->commentFlag			= commentFlag;
		memcpy(rhs->comment, comment, sizeof(comment));
		rhs->numCommentChars		= numCommentChars;
		rhs->queryIdFlag			= queryIdFlag;
		memcpy(rhs->queryId, queryId, sizeof(queryId));
		rhs->serviceNameFlag		= serviceNameFlag;
		memcpy(rhs->serviceName, serviceName, sizeof(serviceName));
		rhs->numServiceNameChars	= numServiceNameChars;
		rhs->roleFlag				= roleFlag;
		memcpy(rhs->role, role, sizeof(role));
		rhs->traceFlag				= traceFlag;
		rhs->traceQS				= traceQS;
		rhs->traceStats				= traceStats;
		rhs->traceCom				= traceCom;
		rhs->traceSyncAS			= traceSyncAS;
		rhs->traceSync				= traceSync;
		rhs->traceRule				= traceRule;
		rhs->traceOffndr			= traceOffndr;
		rhs->traceElapsed			= traceElapsed;
		rhs->traceFileNameFlag		= traceFileNameFlag;
		memcpy(rhs->traceFileName, traceFileName, sizeof(traceFileName));
		rhs->traceFilePathFlag		= traceFilePathFlag;
		memcpy(rhs->traceFilePath, traceFilePath, sizeof(traceFilePath));
		rhs->segmentNumFlag			= segmentNumFlag;
		rhs->segmentNum				= segmentNum;
		rhs->sqlPlanFlag			= sqlPlanFlag;
		rhs->sqlPlan				= sqlPlan;
		rhs->sqlTextFlag			= sqlTextFlag;
		rhs->sqlText				= sqlText;
		rhs->simulateFlag			= simulateFlag;
		rhs->simulateCount			= simulateCount;
		rhs->execTimeoutFlag		= execTimeoutFlag;
		rhs->execTimeout			= execTimeout;
		rhs->waitTimeoutFlag		= waitTimeoutFlag;
		rhs->waitTimeout			= waitTimeout;
		rhs->holdTimeoutFlag		= holdTimeoutFlag;
		rhs->holdTimeout			= holdTimeout;
		memcpy(rhs->sqlDefaults, sqlDefaults, sizeof(sqlDefaults));
		rhs->sqlDefaultsFlag		= sqlDefaultsFlag;
		rhs->ruleInterval			= ruleInterval;
		rhs->ruleIntervalFlag		= ruleIntervalFlag;
		rhs->ruleIntervalExecTime	= ruleIntervalExecTime;
		rhs->ruleIntervalExecTimeFlag = ruleIntervalExecTimeFlag;
		memcpy(rhs->clipsFileName, clipsFileName, sizeof(clipsFileName));
		rhs->maxRowsFetched			= maxRowsFetched;
		rhs->maxRowsFetchedFlag		= maxRowsFetchedFlag;
		rhs->ruleSpec				= ruleSpec;
		rhs->ruleListFlag			= ruleListFlag;
	//
		rhs->priorityOverride		= priorityOverride;
		rhs->offenderFlag			= offenderFlag;
		rhs->offenderProcFlag		= offenderProcFlag;
		rhs->offenderProc			= offenderProc;
		rhs->offenderSampleIntervalFlag = offenderSampleIntervalFlag;
		rhs->offenderSampleInterval	= offenderSampleInterval;
		rhs->offenderSampleCpusFlag	= offenderSampleCpusFlag;
		rhs->offenderSampleCpus		= offenderSampleCpus;
		rhs->offenderSampleCacheFlag = offenderSampleCacheFlag;
		rhs->offenderSampleCache	= offenderSampleCache;
		rhs->offenderCpuNumFlag		= offenderCpuNumFlag;
		rhs->offenderCpuNum			= offenderCpuNum;
		rhs->offenderChildrenFlag	= offenderChildrenFlag; 
		memcpy(rhs->offenderChildren, offenderChildren, sizeof(offenderChildren));
		rhs->offenderProcDetailFlag	= offenderProcDetailFlag; 
		memcpy(rhs->offenderProcDetail, offenderProcDetail, sizeof(offenderProcDetail));
		rhs->pstateFlag				= pstateFlag; 
		memcpy(rhs->pstate, pstate, sizeof(pstate));
	//
	// info about requester
	//
		rhs->requesterType			= requesterType;
		memcpy(rhs->requesterApplication, requesterApplication, sizeof(requesterApplication));
		memcpy(rhs->requesterComputer, requesterComputer, sizeof(requesterComputer));
		memcpy(rhs->requesterName, requesterName, sizeof(requesterName));
		memcpy(rhs->requesterRole, requesterRole, sizeof(requesterRole));
		memcpy(rhs->requesterDBUserName, requesterDBUserName, sizeof(requesterDBUserName));
	//ssd overflow
		rhs->maxOverflowUsageFlag	= maxOverflowUsageFlag;
		rhs->maxOverflowUsage		= maxOverflowUsage;

		rhs->canaryIntervalMinFlag      = canaryIntervalMinFlag;
		rhs->canaryIntervalMin          = canaryIntervalMin;
		rhs->canaryExecSecFlag          = canaryExecSecFlag;
		rhs->canaryExecSec              = canaryExecSec;
		rhs->canaryQuerySrcFlag         = canaryQuerySrcFlag;
		memcpy(rhs->canaryQuerySrc, canaryQuerySrc, sizeof(canaryQuerySrc));
		rhs->numCanaryQueryChars        = numCanaryQueryChars;
		rhs->cancelIfClientDisappearFlag         = cancelIfClientDisappearFlag; 
		memcpy(rhs->cancelIfClientDisappear ,cancelIfClientDisappear , sizeof(cancelIfClientDisappear ));
		rhs->numCancelIfClientDisappear        = numCancelIfClientDisappear ;
		rhs->checkQueryEstResourceUseFlag         = checkQueryEstResourceUseFlag; 
		memcpy(rhs->checkQueryEstResourceUse ,checkQueryEstResourceUse , sizeof(checkQueryEstResourceUse ));
		rhs->numCheckQueryEstResourceUse        = numCheckQueryEstResourceUse ;
		rhs->transIdFlag    = transIdFlag;
                rhs->transId        = transId;
                rhs->maxTxnRollMinFlag    = maxTxnRollMinFlag;
                rhs->maxTxnRollMin    = maxTxnRollMin;
	//operator privileges
		memcpy(rhs->privMask, privMask, sizeof(bitmask_type));
		rhs->maxEspsFlag			= maxEspsFlag;
		rhs->maxEsps				= maxEsps;
	};

	IDL_short			reqType;
	IDL_boolean			maxCpuBusyFlag;
	IDL_short			maxCpuBusy;
	IDL_boolean			maxMemUsageFlag;
	IDL_short			maxMemUsage;
	IDL_boolean			maxCurExecQueriesFlag;
	IDL_short			maxCurExecQueries;
	IDL_boolean			statsIntervalFlag;
	IDL_short			statsInterval;
	IDL_boolean			priorityFlag;
	IDL_short			priority;
	IDL_boolean			activeFlag;
	IDL_char			active[MAX_ACTIVE_TIME_LEN + 1];
	IDL_short			activeBegMins;
	IDL_short			activeEndMins;
	IDL_boolean			commentFlag;
	IDL_char			comment[MAX_COMMENT_LEN + 1];
	IDL_short			numCommentChars;
	IDL_boolean			queryIdFlag;
	IDL_char			queryId[MAX_QUERY_ID_LEN + 1];
	IDL_boolean			serviceNameFlag;
	IDL_char			serviceName[MAX_SERVICE_NAME_LEN + 1];
	IDL_short			numServiceNameChars;
	IDL_char			roleFlag;
	IDL_char			role[MAX_ALIAS_LEN];
	IDL_boolean			traceFlag;
	IDL_boolean			traceQS;
	IDL_boolean			traceStats;
	IDL_boolean			traceCom;
	IDL_boolean			traceSyncAS;
	IDL_boolean			traceSync;
	IDL_boolean			traceRule;
	IDL_boolean			traceOffndr;
	IDL_boolean			traceElapsed;
	IDL_boolean			traceFileNameFlag;
	IDL_char			traceFileName[20 + 1];
	IDL_boolean			traceFilePathFlag;
	IDL_char			traceFilePath[MAX_NAME_LEN + 1];
	IDL_boolean			segmentNumFlag;
	IDL_short			segmentNum;//used by restart & status memory commands
	IDL_boolean			sqlPlanFlag;
	IDL_boolean			sqlPlan;
	IDL_boolean			sqlTextFlag;
	IDL_boolean			sqlText;
	IDL_boolean			simulateFlag;
	IDL_short			simulateCount;
	IDL_boolean			execTimeoutFlag;
	IDL_short			execTimeout;
	IDL_boolean			waitTimeoutFlag;
	IDL_short			waitTimeout;
	IDL_boolean			holdTimeoutFlag;
	IDL_short			holdTimeout;
	IDL_char			sqlDefaults[MAX_SQL_CMD_LEN + 1];
	IDL_boolean			sqlDefaultsFlag;
	IDL_short			ruleInterval;
	IDL_boolean			ruleIntervalFlag;
	IDL_short			ruleIntervalExecTime;
	IDL_boolean			ruleIntervalExecTimeFlag;
	IDL_char			clipsFileName[20 + 1];
	IDL_long_long		maxRowsFetched;
	IDL_boolean			maxRowsFetchedFlag;
	qrysrvc_rule_spec	ruleSpec;
	IDL_short			ruleListFlag;
//
	qrysrvc_rule_list	ruleList;
//
	IDL_boolean			priorityOverride; // { PRIORITY | PRIORITY_OVERRIDE }
//offender
	IDL_boolean			offenderFlag;
	IDL_boolean			offenderProcFlag;
	IDL_short			offenderProc;
	IDL_boolean			offenderSampleIntervalFlag;
	IDL_short			offenderSampleInterval;
	IDL_boolean			offenderSampleCpusFlag;
	IDL_short			offenderSampleCpus;
	IDL_boolean			offenderSampleCacheFlag;
	IDL_short			offenderSampleCache;
	IDL_boolean			offenderCpuNumFlag; // status cpu [cpuNum] [segment <segmentNum>]
	IDL_short			offenderCpuNum;
	IDL_boolean			offenderChildrenFlag; 
	IDL_char			offenderChildren[MAX_PROCESS_LEN + 1];
	IDL_boolean			offenderProcDetailFlag; 
	IDL_char			offenderProcDetail[MAX_PROCESS_LEN + 1];
	IDL_boolean			pstateFlag; 
	IDL_char			pstate[MAX_PROCESS_LEN + 1];
//
// info about requester
//
	REQUESTER			requesterType;
	IDL_char			requesterApplication[MAX_APPLICATION_NAME_LEN +1];
	IDL_char			requesterComputer[MAX_COMPUTER_NAME_LEN + 1];
    IDL_char			requesterName[USERNAME_LENGTH + 1];
    IDL_char			requesterRole[MAX_ROLE_LEN + 1];
    IDL_char			requesterDBUserName[MAX_ROLE_LEN + 1];
//ssd overflow
    IDL_boolean         maxOverflowUsageFlag;
    IDL_short           maxOverflowUsage;

    IDL_boolean         canaryIntervalMinFlag;
    IDL_short           canaryIntervalMin;
    IDL_boolean         canaryExecSecFlag;
    IDL_short           canaryExecSec;
    IDL_boolean         canaryQuerySrcFlag;
    IDL_char            canaryQuerySrc[MAX_CANARY_LEN + 1];
    IDL_short           numCanaryQueryChars;
    IDL_boolean         cancelIfClientDisappearFlag;
    IDL_char            cancelIfClientDisappear[6];
    IDL_short           numCancelIfClientDisappear;
    IDL_boolean         checkQueryEstResourceUseFlag;//Gary 20130426
    IDL_char            checkQueryEstResourceUse[6];
    IDL_short           numCheckQueryEstResourceUse;
    IDL_boolean         transIdFlag;
    long            transId;
    IDL_boolean         maxTxnRollMinFlag;
    IDL_short           maxTxnRollMin;
// Component privileges
	bitmask_type 		privMask;
	IDL_boolean			maxEspsFlag;
	IDL_short			maxEsps;
};

//================================================================
//================ queue state ===================================
const string FMT_ACTIVE						= "ACTIVE";
const string FMT_HOLD						= "HOLD";
const string FMT_QUIESCE					= "QUIESCE";
const string FMT_STOPPING					= "STOPPING";
const string FMT_STOPPED					= "STOPPED";
const string FMT_SHUTDOWN					= "SHUTDOWN";
const string FMT_TRANSIENT					= "TRANSIENT";
//================================================================
//================ queue prty ====================================
const string FMT_URGENT						= "URGENT";
const string FMT_HIGH						= "HIGH";
const string FMT_MEDIUM_HIGH				= "MEDIUM-HIGH";
const string FMT_MEDIUM						= "MEDIUM";
const string FMT_LOW_MEDIUM					= "LOW-MEDIUM";
const string FMT_LOW						= "LOW";
//================================================================
//=============== query state ====================================
const string FMT_INIT						= "INIT";
const string FMT_REJECTED					= "REJECTED";
const string FMT_WAITING					= "WAITING";
const string FMT_EXECUTING					= "EXECUTING";
const string FMT_EXECUTING_TXN_ROLLBACK				= "WMS_TXN_ROLLBACK_IN_PROGRESS";
const string FMT_HOLDING					= "HOLDING";
const string FMT_RELEASED					= "RELEASED";
const string FMT_COMPLETED					= "COMPLETED";
const string FMT_SUSPENDED					= "SUSPENDED";
const string FMT_UNKNOWN					= "UNKNOWN";
//================================================================
//=============== query substate =================================
const string FMT_NA							= "N/A";
const string FMT_NONE						= "NONE";
const string FMT_MAX_CPU_BUSY				= "WMS_MAX_CPU_BUSY";
const string FMT_EST_MAX_CPU_BUSY			= "WMS_EST_MAX_CPU_BUSY";
const string FMT_MAX_MEM_USAGE				= "WMS_MAX_MEM_USAGE";
const string FMT_MAX_SSD_USAGE         		= "WMS_MAX_SSD_USAGE";	//ssd overflow
const string FMT_MAX_AVG_ESPS         		= "WMS_MAX_AVG_ESPS";
const string FMT_WAITING_CANARY             = "WMS_LOW_QUERY_THROUGHPUT";
const string FMT_RELEASED_BY_ADMIN			= "WMS_QUERY_RELEASED_BY_ADMIN";
const string FMT_RELEASED_BY_EXEC_RULE		= "WMS_QUERY_RELEASED_BY_EXEC_RULE";
//const string FMT_MAX_CUR_EXEC_QUERIES		= "WMS_MAX_EXEC_QUERIES";
const string FMT_MAX_SERVICE_EXEC_QUERIES   = "WMS_MAX_EXEC_QUERIES_SERVICE";
const string FMT_MAX_INSTANCE_EXEC_QUERIES  = "WMS_MAX_EXEC_QUERIES_INSTANCE";
const string FMT_LOADING					= "WMS_QUERY_LOADING";
const string FMT_REPREPARING				= "WMS_QUERY_REPREPARING";
const string FMT_EXECUTING_SQL_CMD			= "WMS_QUERY_EXECUTING";
const string FMT_BY_COMP_RULE				= "WMS_COMP_RULE"; 
const string FMT_BY_EXEC_RULE				= "WMS_EXEC_RULE"; 
const string FMT_BY_ADMIN					= "WMS_ADMIN"; 
const string FMT_HOLD_TIMEOUT				= "WMS_HOLD_TIMEOUT";
const string FMT_EXEC_TIMEOUT				= "WMS_EXEC_TIMEOUT"; 
const string FMT_CANCELLED_BY_ADMIN			= "WMS_QUERY_CANCELLED"; 
const string FMT_CANCELLED_BY_ADMIN_SERVER		= "QUERY_CANCELLED";
const string FMT_CANCELLED_BY_CLIENT			= "CLIENT_QUERY_CANCELLED";
const string FMT_CANCEL_IN_PROGRESS			= "CANCEL_IN_PROGRESS";
const string FMT_CANCEL_FAILED				= "CANCEL_FAILED";
const string FMT_CANCEL_FAILED_8026			= "CANCEL_FAILED-8026";
const string FMT_CANCEL_FAILED_8027			= "CANCEL_FAILED-8027";
const string FMT_CANCEL_FAILED_8028			= "CANCEL_FAILED-8028";
const string FMT_CANCEL_FAILED_8029			= "CANCEL_FAILED-8029";
const string FMT_CANCEL_FAILED_8031			= "CANCEL_FAILED-8031";
const string FMT_QUERY_NOT_FOUND			= "WMS_QUERY_NOT_FOUND"; 
const string FMT_CONNECTION_FAILED			= "WMS_CONNECTION_FAILED";
const string FMT_NDCS_PROCESS_FAILED		= "HPDCS_PROCESS_FAILED";
const string FMT_CPU_FAILED					= "WMS_CPU_FAILED"; 
const string FMT_SEGMENT_FAILED				= "WMS_NODE_FAILED"; 
const string FMT_SERVICE_NOT_ACTIVE			= "WMS_SERVICE_NOT_ACTIVE"; 
const string FMT_UNEXPECTED_STATE			= "WMS_UNEXPECTED_STATE"; 
const string FMT_CLIENT_DISAPPEARED			= "WMS_CLIENT_DISAPPEARED";
const string FMT_QUEUE_MSG_CANCELLED		= "WMS_QUEUE_MSG_CANCELLED";
const string FMT_VERSION_MISMATCH			= "WMS_VERSION_MISMATCH";
const string FMT_WMS_ON_HOLD				= "WMS_ON_HOLD";
const string FMT_MAX_QUERIES_REACHED		= "WMS_MAXIMUM_QUERIES_REACHED";
const string FMT_SERVICE_NOT_FOUND			= "WMS_SERVICE_NOT_FOUND";
const string FMT_SERVICE_ON_HOLD			= "WMS_SERVICE_ON_HOLD";
const string FMT_UNKNOWN_USER				= "WMS_UNKNOWN_USER";
const string FMT_WAIT_TIMEOUT				= "WMS_WAIT_TIMEOUT";
const string FMT_QUERY_CANCELED				= "WMS_QUERY_CANCELED";
const string FMT_QUERY_CANCELED_BY_RULE		= "WMS_QUERY_CANCELED_BY_RULE";
const string FMT_QUERY_CANCELED_BY_ADMIN	= "WMS_QUERY_CANCELED_BY_ADMIN";
const string FMT_QUERY_CANCELED_BY_TIMEOUT	= "WMS_QUERY_CANCELED_BY_TIMEOUT";
const string FMT_WAITING_TXN_BACKOUT		= "WMS_TRANSACTION_BACKOUT";

//================================================================
//=============== WarnLevel ======================================
const string FMT_WLVL_LOW					= "LOW";
const string FMT_WLVL_MEDIUM				= "MEDIUM";
const string FMT_WLVL_HIGH					= "HIGH";
const string FMT_WLVL_NO_WARN				= "NO_WARN";
//================================================================
//=============== Action =========================================
const string FMT_ACT_INIT					= "INIT";
const string FMT_ACT_WARN					= "WARN";
const string FMT_CONN_EXEC					= "CONN_EXEC";
const string FMT_COMP_EXEC					= "COMP_EXEC";
const string FMT_COMP_EXECUTE					= "COMP_EXECUTE";
const string FMT_COMP_REJECT				= "COMP_REJECT";
const string FMT_COMP_HOLD					= "COMP_HOLD";
const string FMT_EXEC_CANCEL				= "EXEC_CANCEL";
const string FMT_EXEC_STATS_PERTABLE		= "EXEC_STATS_PERTABLE";
const string FMT_EXEC_STATS_PERTABLE_CANCEL	= "EXEC_STATS_PERTABLE_CANCEL";
const string FMT_ACT_NO_WARN				= "NO_WARN";
//=============== Aggregation ===============================
const string FMT_AGGREGATE					= "AGGREGATE";
const string FMT_INSERT						= "INSERT";
const string FMT_UPDATE						= "UPDATE";
const string FMT_DELETE						= "DELETE";
const string FMT_SELECT						= "SELECT";
//================================================================
//=============== WarnReason =====================================
const string FMT_WRSNE_INIT					= "INIT";
const string FMT_WRSNE_RULE					= "RULE";
const string FMT_WRSNE_USED_ROWS			= "USED_ROWS";
const string FMT_WRSNE_ACCESSED_ROWS		= "ACCESSED_ROWS";
const string FMT_WRSNE_TOTAL_MEM_ALLOC		= "TOTAL_MEM_ALLOC";
const string FMT_WRSNE_ELAPSED_TIME			= "ELAPSED_TIME";
const string FMT_WRSNE_CPU_TIME				= "CPU_TIME";
const string FMT_WRSNE_PROCESS_BUSY_TIME	= "PROCESS_BUSY_TIME";
const string FMT_WRSNE_STATS_IDLE_TIME	= "STATS_IDLE_MINUTES";
//================================================================
//=============== ProcessType =====================================
const string FMT_PROCESS_TYPE_INIT			= "INIT";
const string FMT_PROCESS_TYPE_SRVR			= "MXOSRVR";
const string FMT_PROCESS_TYPE_CMP			= "CMP";
const string FMT_PROCESS_TYPE_ESP			= "ESP";
const string FMT_PROCESS_TYPE_UDR			= "UDR";
const string FMT_PROCESS_TYPE_CI			= "CI";
const string FMT_PROCESS_TYPE_OTHER			= "OTHER";
const string FMT_PROCESS_TYPE_ALL			= "ALL";
const string FMT_PROCESS_TYPE_SQL			= "SQL";
const string FMT_PROCESS_TYPE_WMS			= "WMS"; //added for SQ offender
//================================================================
//================================================================
//=============== ProcessType =====================================
const string FMT_WORKLOAD_TYPE_INIT			= "INIT";
const string FMT_WORKLOAD_TYPE_SHORT		= "SHORT";
const string FMT_WORKLOAD_TYPE_USER			= "USER";
//================================================================
//================ SQL statement state ===========================
const string FMT_INITIAL					= "INITIAL";
const string FMT_OPEN						= "OPEN";
const string FMT_EOF						= "EOF";
const string FMT_CLOSE						= "CLOSE";
const string FMT_DEALLOCATED				= "DEALLOCATED";
const string FMT_FETCH						= "FETCH";
const string FMT_CLOSE_TABLES				= "CLOSETABLES";
const string FMT_PREPARE					= "PREPARE";
const string FMT_PROCESS_ENDED				= "PROCESSEND";
//================================================================
//================================================================
/*=============== TcpStatus ======================================
     CLOSED       Connection is closed.
     LISTEN       Listening for a connection.
     SYN_SENT     Active; have sent SYN. Represents waiting for a matching
                  connection request after having sent a connection request.
     SYN_RCVD     Have sent and received SYN. Represents waiting for a
                  confirming connection request acknowledgment after having
                  both received and sent connection requests.
     ESTABLISHED  Connection established.
     CLOSE_WAIT   Have received FIN; waiting to receive CLOSE.
     LAST_ACK     Have received FIN and CLOSE; awaiting FIN ACK.
     FIN_WAIT_1   Have closed; sent FIN.
     CLOSING      Closed; exchanged FIN; awaiting FIN.
     FIN_WAIT_2   Have closed; FIN is acknowledged; awaiting FIN.
     TIME_WAIT    In 2MSL (twice the maximum segment length) quiet wait after close.
*/
const string FMT_TCP_STATE_CLOSED			= "CLOSED";
const string FMT_TCP_STATE_SYNC_SENT		= "SYNC_SENT";
const string FMT_TCP_STATE_SYNC_RECV		= "SYNC_RECV";
const string FMT_TCP_STATE_ESTAB			= "ESTAB";
const string FMT_TCP_STATE_CLOSE_WAIT		= "CLOSE_WAIT";
const string FMT_TCP_STATE_FIN_WAIT_1		= "FIN_WAIT_1";
const string FMT_TCP_STATE_CLOSING			= "CLOSING";
const string FMT_TCP_STATE_LAST_ACK			= "LAST_ACK";
const string FMT_TCP_STATE_FIN_WAIT_2		= "FIN_WAIT_2";
const string FMT_TCP_STATE_TIME_WAIT		= "TIME_WAIT";
const string FMT_TCP_STATE_FAIL_WAIT		= "FAIL_WAIT";
const string FMT_TCP_STATE_LISTEN			= "LISTEN";
//
//
const string FMT_NDCS_DLG_INIT				= "HPDCS_DIALOGUE_INIT";
const string FMT_NDCS_CONN_IDLE				= "HPDCS_CONNECTION_IDLE";
const string FMT_NDCS_DLG_TERM				= "HPDCS_DIALOGUE_TERMINATED";
const string FMT_NDCS_DLG_BREAK				= "HPDCS_DIALOGUE_BREAK";
const string FMT_NDCS_STOP_SRVR				= "HPDCS_SRVER_STOPPED";
const string FMT_NDCS_RMS_ERROR				= "HPDCS_RMS_ERROR";
const string FMT_NDCS_REPOS_IDLE			= "HPDCS_REPOS_INTERVAL_SINGLE";
const string FMT_NDCS_REPOS_INTERVAL		= "HPDCS_REPOS_INTERVAL_MULTI";
const string FMT_NDCS_REPOS_PARTIAL			= "HPDCS_REPOS_INTERVAL_PARTIAL";
const string FMT_NDCS_EXEC_INTERVAL			= "HPDCS_EXEC_INTERVAL_SINGLE";
const string FMT_NDCS_CONN_RULE_CHANGED		= "HPDCS_CONN_RULE_CHANGED";
const string FMT_NDCS_CLOSE					= "HPDCS_CONNECTION_CLOSED";
const string FMT_NDCS_PREPARE				= "HPDCS_QUERY_PREPARE";
const string FMT_NDCS_WMS_ERROR				= "HPDCS_WMS_ERROR";
const string FMT_NDCS_QUERY_CANCELED		= "HPDCS_QUERY_CANCELED";
const string FMT_NDCS_QUERY_REJECTED		= "HPDCS_QUERY_REJECTED";
const string FMT_LONG_TRANS_ABORTING        	= "WMS_TXN_ROLLBACK_IN_PROGRESS";

const string FMT_EST_CPU_TIME				= "EST_CPU_TIME";
const string FMT_EST_IO_TIME				= "EST_IO_TIME";
const string FMT_EST_TOTAL_TIME				= "EST_TOTAL_TIME";
const string FMT_EST_CARDINALITY			= "EST_CARDINALITY";
const string FMT_ELAPSED_TIME				= "ELAPSED_TIME";
const string FMT_ACCESSED_ROWS				= "ACCESSED_ROWS";
const string FMT_USED_ROWS					= "USED_ROWS";
const string FMT_STATS_IDLE_TIME                        = "STATS_IDLE_MINUTES";
const string FMT_OPEN_PAREN					= "(";
const string FMT_CLOSE_PAREN				= ")";
const string FMT_GT							= ">";
const string FMT_LT							= "<";
const string FMT_EQ							= "=";
const string FMT_GE							= ">=";
const string FMT_LE							= "<=";
const string FMT_NE							= "<>";
const string FMT_PCT						= "%";
const string FMT_SEMICOLON					= ":";
const string FMT_AND						= "AND";
const string FMT_CPU_TIME					= "CPU_TIME";
const string FMT_EST_ACCESSED_ROWS			= "EST_ACCESSED_ROWS";
const string FMT_EST_USED_ROWS				= "EST_USED_ROWS";
const string FMT_OFF						= "OFF";
const string FMT_ON							= "ON";
const string FMT_QSSYNC						= "QSSYNC";
const string FMT_QSMGR						= "QSMGR";
const string FMT_QSSTATS					= "QSSTATS";
const string FMT_QSCOM						= "QSCOM";
const string FMT_QSSYNC_AS					= "QSSYNCAS";
const string FMT_QSRULE						= "QSRULE";
const string FMT_QSOFFNDR					= "QSOFFNDR";
const string FMT_HEAP						= "HEAP";
const string FMT_ELAPSED					= "ELAPSED";
const string FMT_ALL						= "ALL";
const string FMT_TRACE_INIT					= "TRACE_INIT";
const string FMT_TRACE_QS					= "TRACE_QS";
const string FMT_TRACE_STATS				= "TRACE_STATS";
const string FMT_TRACE_COM					= "TRACE_COM";
const string FMT_TRACE_SYNC_AS				= "TRACE_SYNC_AS";
const string FMT_TRACE_SYNC					= "TRACE_SYNC";
const string FMT_TRACE_HEAP					= "TRACE_HEAP";
const string FMT_TRACE_ELAPSED				= "TRACE_ELAPSED";
const string FMT_TRACE_RULE					= "TRACE_RULE";
const string FMT_TRACE_OFFNDR				= "TRACE_OFFNDR";
const string FMT_CONN						= "CONN";
const string FMT_COMP						= "COMP";
const string FMT_EXEC						= "EXEC";
const string FMT_NOP						= "NOP";
const string FMT_OR							= "OR";
const string FMT_WARN						= "WARN";
const string FMT_NO_WARN					= "NO-WARN";
const string FMT_REJECT						= "REJECT";
const string FMT_CANCEL						= "CANCEL";
const string FMT_STATS_PERTABLE				= "STATS_PERTABLE";
const string FMT_STATS_PERTABLE_CANCEL		= "STATS_PERTABLE_CANCEL";
const string FMT_APPL						= "APPL";
const string FMT_SESSION					= "SESSION";
const string FMT_LOGIN						= "LOGIN";
const string FMT_DSN						= "DSN";
const string FMT_EST_TOTAL_MEMORY			= "EST_TOTAL_MEMORY";
const string FMT_NUM_JOINS					= "NUM_JOINS";
const string FMT_UPDATE_STATS_WARNING		= "UPDATE_STATS_WARNING";
const string FMT_CROSS_PRODUCT				= "CROSS_PRODUCT";
const string FMT_SCAN_SIZE					= "SCAN_SIZE";
const string FMT_TOTAL_MEM_ALLOC			= "TOTAL_MEM_ALLOC";
const string FMT_PROCESS_BUSY_TIME			= "PROCESS_BUSY_TIME";
const string FMT_SC							= "";
const string FMT_UNKN						= "UNKN";
const string FMT_ICASE						= "ICASE";
const string FMT_ALL_EXPRESSIONS_EQUAL_TRUE		= "ALL EXPRESSIONS = TRUE";
const string FMT_NO_CONNECTION_RULES_IN_NDCS	= "NO CONNECTION RULES IN NDCS";
const string FMT_NO_MATCHING_CONNECTION_RULES	= "NO MATCHING CONNECTION RULES";

//===================================================================================
// WMS Operator Privileges
const string ADMIN_ADD 				= "01";
const string ADMIN_ALTER 			= "02";
const string ADMIN_DELETE 			= "03";
const string ADMIN_STATUS 			= "04";
const string ADMIN_STOP 			= "05";
const string ADMIN_START 			= "06";
const string ADMIN_CANCEL 			= "07";
const string ADMIN_HOLD 			= "08";
const string ADMIN_RELEASE 			= "09";
const string ADMIN_REPREPARE 		= "0A";
const string ADMIN_LOAD_QUERY 		= "0B";
const string ADMIN_EXECUTE_QUERY 	= "0C";
const string ADMIN_RESTART			= "0D";
//Add new privileges here

//===================================================================================
// -----------------------------------------------------------------------
// List of all errors generated in the SQL executor code
// -----------------------------------------------------------------------

enum ExeErrorCode
{
	EXE_FIRST_ERROR					= 8000,
	EXE_INTERNAL_ERROR				= 8001,
	EXE_NOWAIT_OP_INCOMPLETE		= 8002,
	EXE_OUTPUT_DESCRIPTOR_LOCKED   	= 8003,
	EXE_CURSOR_ALREADY_OPEN			= 8004,
	EXE_CURSOR_NOT_OPEN				= 8005,
	EXE_STREAM_TIMEOUT              = 8006,
	EXE_CANCELED					= 8007,
	EXE_INVALID_CAT_NAME			= 8008,
	EXE_INVALID_SCH_NAME			= 8009,
	EXE_INFO_DEFAULT_CAT_SCH		= 8010,
	EXE_BLOCK_CARDINALITY_VIOLATION	= 8011,
	EXE_INFO_CQD_NAME_VALUE_PAIRS	= 8012,
	EXE_CURSOR_NOT_FETCHED			= 8013,
	EXE_CS_EOD						= 8014,
	EXE_CS_EOD_ROLLBACK_ERROR		= 8015,
	EXE_VERSION_ERROR				= 8016,
	EXE_NO_EXPLAIN_INFO				= 8017,
	EXE_PARTN_SKIPPED				= 8018,
	EXE_EXPLAIN_BAD_DATA			= 8019,

	EXE_INITIALIZE_MAINTAIN			= 8020,
	EXE_PURGEDATA_CAT				= 8021,
	EXE_PARALLEL_PURGEDATA_FAILED	= 8022,

	EXE_QUERY_LIMITS_CPU			= 8023,
	EXE_QUERY_LIMITS_CPU_DEBUG		= 8024,
	EXE_QUERY_LIMITS_CPU_DP2		= 8025,

	EXE_CANCEL_QID_NOT_FOUND		= 8026,
	EXE_CANCEL_TIMEOUT				= 8027,
	EXE_CANCEL_PROCESS_NOT_FOUND	= 8028,
	EXE_CANCEL_NOT_AUTHORIZED		= 8029,
        EXE_EXPLAIN_PLAN_TOO_LARGE            = 8033,

// ---------------------------------------------------------------------
// Data integrity errors
// ---------------------------------------------------------------------
	EXE_INVALID_DEFINE_OR_ENVVAR		= 8100,
	EXE_TABLE_CHECK_CONSTRAINT			= 8101, // SQLSTATE 23000
	EXE_DUPLICATE_RECORD				= 8102, // SQLSTATE 23000
	EXE_RI_CONSTRAINT_VIOLATION			= 8103, // SQLSTATE 23000
	EXE_CHECK_OPTION_VIOLATION_CASCADED	= 8104, // SQLSTATE 44000
	EXE_CHECK_OPTION_VIOLATION			= 8105, // SQLSTATE 44000
	EXE_CURSOR_UPDATE_CONFLICT			= 8106,
	EXE_HALLOWEEN_INSERT_AUTOCOMMIT		= 8107,
	EXE_DUPLICATE_IDENTITY_VALUE		= 8108, // SQLSTATE 23000
	EXE_INVALID_SESSION_DEFAULT			= 8109,
	EXE_DUPLICATE_ENTIRE_RECORD			= 8110,
	EXE_LAST_INTEGRITY_ERROR			= 8139,

// ---------------------------------------------------------------------
// Some internal testing "errors"
// ---------------------------------------------------------------------
	EXE_CANCEL_INJECTED					= 8140,
	EXE_ERROR_INJECTED					= 8141,

// ---------------------------------------------------------------------
// Set session default "warning".
// ---------------------------------------------------------------------
	EXE_CLEANUP_ESP						= 8143,

//----------------------------------------------------------------------
// Late-name resolution and late-binding/similarity check errors.
//----------------------------------------------------------------------
	EXE_NAME_MAPPING_ERROR				= 8300,
	EXE_NAME_MAPPING_FS_ERROR			= 8301,
	EXE_NAME_MAPPING_NO_PART_AVAILABLE	= 8302,
	EXE_NAME_MAPPING_BAD_ANCHOR			= 8303,

//----------------------------------------------------------------------
// Available for future use.
//----------------------------------------------------------------------
	CLI_ASSIGN_INCOMPATIBLE_CHARSET		= 8350,

//----------------------------------------------------------------------
// Expressions errors
//----------------------------------------------------------------------
	EXE_INVALID_DEFINE_CLASS_ERROR		= 8400,
	EXE_CARDINALITY_VIOLATION			= 8401,
	EXE_STRING_OVERFLOW					= 8402,
	EXE_SUBSTRING_ERROR					= 8403,
	EXE_TRIM_ERROR						= 8404,
	EXE_CONVERTTIMESTAMP_ERROR			= 8405,
	EXE_JULIANTIMESTAMP_ERROR			= 8407,
	EXE_INVALID_ESCAPE_CHARACTER		= 8409,
	EXE_INVALID_ESCAPE_SEQUENCE			= 8410,
	EXE_NUMERIC_OVERFLOW				= 8411,
	EXE_MISSING_NULL_TERMINATOR			= 8412,
	EXE_CONVERT_STRING_ERROR			= 8413,
	EXE_CONVERT_NOT_SUPPORTED			= 8414,
	EXE_CONVERT_DATETIME_ERROR			= 8415,
	EXE_DATETIME_FIELD_OVERFLOW			= 8416,
	EXE_USER_FUNCTION_ERROR				= 8417,
	EXE_USER_FUNCTION_NOT_SUPP			= 8418,
	EXE_DIVISION_BY_ZERO				= 8419,
	EXE_MISSING_INDICATOR_VARIABLE		= 8420,
	EXE_ASSIGNING_NULL_TO_NOT_NULL		= 8421,
	EXE_CONVERT_INTERVAL_ERROR			= 8422,
	EXE_FIELD_NUM_OVERFLOW				= 8423,
	EXE_MATH_FUNC_NOT_SUPPORTED			= 8424,
	EXE_DEFAULT_VALUE_ERROR				= 8425,
	EXE_SORT_ERROR						= 8427,
	EXE_BAD_ARG_TO_MATH_FUNC			= 8428,
	EXE_MAPPED_FUNCTION_ERROR			= 8429,
	EXE_GETBIT_ERROR					= 8430,
	EXE_IS_BITWISE_AND_ERROR			= 8431,
	EXE_UNSIGNED_OVERFLOW				= 8432,
	EXE_INVALID_CHARACTER				= 8433,
	EXE_HISTORY_BUFFER_TOO_SMALL		= 8440,
	EXE_OLAP_OVERFLOW_NOT_SUPPORTED		= 8441,
	EXE_LAST_EXPRESSIONS_ERROR			= 8499,

// ---------------------------------------------------------------------
// File System and DP2 errors.
// ---------------------------------------------------------------------
	EXE_ERROR_FROM_DP2					= 8550,
	EXE_ERROR_FROM_FS2					= 8551,
	EXE_FS2_FETCH_VERSION_ERROR			= 8552,
	EXE_ERROR_STREAM_OVERFLOW			= 8553,
	EXE_EID_INTERNAL_ERROR				= 8555,
	EXE_LAST_ERROR_FROM_FS_DP2			= 8569,

// ---------------------------------------------------------------------
// Build-time and other catastophic errors
// ---------------------------------------------------------------------
	EXE_NO_MEM_TO_BUILD					= 8570,
	EXE_NO_MEM_TO_EXEC					= 8571,
	EXE_CANNOT_CONTINUE					= 8572,
	EXE_ACCESS_VIOLATION 				= 8573,

// ------------------------------------------------------------
// Error 8574, lost open. Could result in reopening the table.
// Error 8575, could result in recompilation.
// Warning 8576, statement was recompiled.
// ------------------------------------------------------------
	EXE_LOST_OPEN						= 8574,
	EXE_TIMESTAMP_MISMATCH				= 8575,
	EXE_RECOMPILE						= 8576,
	EXE_TABLE_NOT_FOUND					= 8577,
	EXE_SIM_CHECK_PASSED				= 8578,
	EXE_SIM_CHECK_FAILED				= 8579,
	EXE_PARTITION_UNAVAILABLE			= 8580,
	EXE_NO_MEM_FOR_IN_MEM_JOIN			= 8581,
	EXE_USER_PREPARE_NEEDED				= 8582,
	EXE_RELEASE_WORK_TIMEOUT			= 8584,
	EXE_SCHEMA_SECURITY_CHANGED			= 8585,
	EXE_ASSIGN_ESPS_ERROR				= 8586,
	EXE_IAR_MFMAP_BAD					= 8590,
	EXE_IAR_ERROR_EXTRACTING_COLUMNS	= 8591,
	EXE_IAR_NO_MFMAP					= 8592,
	EXE_IAR_MISSING_COLS_COMPRESSED_AUDIT = 8593,
	EXE_AUDIT_IMAGE_EXPR_EVAL_ERROR		= 8594,
	EXE_MERGE_STMT_ERROR				= 8595,
	EXE_ESP_CHANGE_PRIORITY_FAILED		= 8596,
	EXE_RECOMPILE_AUTO_QUERY_RETRY		= 8597,
	EXE_VIEW_NOT_FOUND					= 8598,
	EXE_MV_UNAVILABLE					= 12073,

//-------------------------------------------------------------
// Errors codes for concurrency control.
//-------------------------------------------------------------
	EXE_FIRST_CONCURRENCY_CONTROL_ERROR	= 8600,
	EXE_LOCK_UNLOCK_ERROR				= 8601,
	EXE_FILESYSTEM_ERROR				= 8602,
	EXE_BEGIN_TRANSACTION_ERROR			= 8603,
	EXE_BEGIN_ERROR_FROM_TRANS_SUBSYS	= 8604,
	EXE_COMMIT_TRANSACTION_ERROR		= 8605,
	EXE_COMMIT_ERROR_FROM_TRANS_SUBSYS	= 8606,
	EXE_ROLLBACK_TRANSACTION_ERROR		= 8607,
	EXE_ROLLBACK_ERROR_FROM_TRANS_SUBSYS 	= 8608,
	EXE_ROLLBACK_TRANSACTION_WAITED_ERROR 	= 8609,
	EXE_ROLLBACK_WAITED_ERROR_TRANS_SUBSYS 	= 8610,
	EXE_SET_TRANS_ERROR_FROM_TRANS_SUBSYS 	= 8612,
	EXE_CANT_COMMIT_OR_ROLLBACK			= 8613,
	EXE_CANT_BEGIN_WITH_MULTIPLE_CONTEXTS	= 8614,
	EXE_CANT_BEGIN_USER_TRANS_WITH_LRU	= 8615,
	EXE_LAST_CONCURRENCY_CONTROL_ERROR	= 8629,

//-------------------------------------------------------------
// Error codes for bulk replicate
//-------------------------------------------------------------
	EXE_REPL_TO_UNSUPPORTED_TGT_SYS       = 8645,
	EXE_BDR_ALREADY_INITIALIZED           = 8646,
	EXE_BDR_SERVICE_PROCESS_COMM_ERROR    = 8647,
	EXE_REPL_TARGET_REPL_PROCESS_COMM_ERROR = 8648,
	EXE_REPL_SRC_TGT_PARTN_MISMATCH       = 8650,
	EXE_REPL_SRC_TGT_DDL_MISMATCH         = 8651,
	EXE_REPL_SRC_TGT_VERSION_MISMATCH     = 8652,
	EXE_BDR_REPL_PROCESS_COMM_ERROR       = 8653,
	EXE_REPL_QUERY_ID_NOT_FOUND           = 8654,
	EXE_REPL_COULD_NOT_ABORT_QUERY        = 8655,
	EXE_REPL_QUERY_WAS_ABORTED            = 8656,
	EXE_REPL_COULD_NOT_RECOVER            = 8657,
	EXE_REPL_INVALID_IPADDR_OR_PORTNUM    = 8658,

//-------------------------------------------------------------
// Errors codes for suspend/resume.
//-------------------------------------------------------------
	EXE_SUSPEND_AUDIT                     = 8670,
	EXE_SUSPEND_LOCKS                     = 8671,
	EXE_SUSPEND_QID_NOT_ACTIVE            = 8672,
	EXE_SUSPEND_GUARDIAN_ERROR_1          = 8673,
	EXE_SUSPEND_GUARDIAN_ERROR_2          = 8674,
	EXE_SUSPEND_SQL_ERROR                 = 8675,
	EXE_SUSPEND_ALREADY_SUSPENDED         = 8676,
	EXE_SUSPEND_NOT_SUSPENDED             = 8677,

//-------------------------------------------------------------
// Errors codes translate function.
//-------------------------------------------------------------
	EXE_INVALID_CHAR_IN_TRANSLATE_FUNC    = 8690,

// ---------------------------------------------------------------------
// Parallel execution
// ---------------------------------------------------------------------
	EXE_PARALLEL_EXECUTION_ERROR		  = 8700,
	EXE_PARALLEL_EXTRACT_OPEN_ERROR       = 8701,
	EXE_PARALLEL_EXTRACT_CONNECT_ERROR    = 8702,

// ---------------------------------------------------------------------
// Warning from updating Measure SQL counters.
// ---------------------------------------------------------------------
	EXE_MEASURE                           = 8710,

//----------------------------------------------------------------------
// Errors generated in the CLI code
//----------------------------------------------------------------------
	CLI_FIRST_ERROR			= 8730,
	CLI_PROBLEM_READING_USERS             = 8731,
	CLI_USER_NOT_REGISTERED               = 8732,
	CLI_USER_NOT_VALID                    = 8733,
	CLI_INVALID_QUERY_PRIVS               = 8734,

	CLI_UNUSED                            = 8740,
	CLI_CANNOT_EXECUTE_IN_MEM_DEFN        = 8741,
	CLI_DUPLICATE_DESC					  = 8801,
	CLI_DUPLICATE_STMT					  = 8802,
	CLI_DESC_NOT_EXSISTS				  = 8803,
	CLI_STMT_NOT_EXSISTS				  = 8804,
	CLI_NOT_DYNAMIC_DESC				  = 8805,
	CLI_NOT_DYNAMIC_STMT				  = 8806,
	CLI_DATA_OUTOFRANGE					  = 8807,
	CLI_MODULEFILE_CORRUPTED			  = 8808,
	CLI_MODULEFILE_OPEN_ERROR			  = 8809,

	CLI_NO_ERROR_IN_DIAGS                 = 8810,

	CLI_STMT_NOT_OPEN					= 8811,
	CLI_STMT_NOT_CLOSE					= 8812,
	CLI_STMT_CLOSE						= 8813,
	CLI_TRANS_MODE_MISMATCH				= 8814,
	CLI_TCB_EXECUTE_ERROR				= 8816,
	CLI_TCB_FETCH_ERROR					= 8817,
	CLI_TDB_DESCRIBE_ERROR				= 8818,
	CLI_BEGIN_TRANSACTION_ERROR			= 8819,
	CLI_COMMIT_TRANSACTION_ERROR		= 8820,

	CLI_ROLLBACK_TRANSACTION_ERROR		= 8821,
	CLI_STMT_NOT_PREPARED				= 8822,
	CLI_IO_REQUESTS_PENDING				= 8823,
	CLI_NO_MODULE_NAME					= 8824,
	CLI_MODULE_ALREADY_ADDED			= 8825,
	CLI_ADD_MODULE_ERROR				= 8826,
	CLI_SEND_REQUEST_ERROR				= 8827,
	CLI_OUT_OF_MEMORY					= 8828,
	CLI_INVALID_DESC_ENTRY				= 8829,
	CLI_NO_CURRENT_CONTEXT				= 8830,

	CLI_MODULE_NOT_ADDED				= 8831,
	CLI_TRANSACTION_NOT_STARTED			= 8832,
	CLI_INVALID_SQLTRANS_COMMAND		= 8833,
	CLI_NO_INSTALL_DIR					= 8834,
	CLI_INVALID_DESC_INFO_REQUEST    	= 8835,
	CLI_INVALID_UPDATE_COLUMN       	= 8836,
	CLI_INVALID_USERID   				= 8837,
	CLI_RECEIVE_ERROR   				= 8838,
	CLI_VALIDATE_TRANSACTION_ERROR   	= 8839,
	CLI_SELECT_INTO_ERROR           	= 8401,
	CLI_INVALID_OBJECTNAME				= 8840,

	CLI_USER_ENDED_EXE_XN              	= 8841,
	CLI_NON_UPDATABLE_SELECT_CURSOR		= 8842,
	CLI_ITEM_NUM_OUT_OF_RANGE			= 8843,
	CLI_USER_ENDED_XN_CLEANUP			= 8844,
	CLI_INTERR_NULL_TCB					= 8845,
	CLI_EMPTY_SQL_STMT					= 8846,
	CLI_SQLMP_RTD_ERROR					= 8847,
	CLI_CANCEL_REJECTED					= 8848,
	CLI_NON_CURSOR_UPDEL_TABLE			= 8850,
	CLI_USER_MEMORY_IN_EXECUTOR_SEGMENT	= 8851,
	CLI_CURSOR_CANNOT_BE_HOLDABLE		= 8852,
	CLI_INVALID_ATTR_NAME				= 8853,
	CLI_INVALID_ATTR_VALUE				= 8854,
	CLI_CURSOR_ATTR_CANNOT_BE_SET		= 8855,
	CLI_ARRAY_MAXSIZE_INVALID_ENTRY		= 8856,
	CLI_LOCAL_AUTHENTICATION			= 8857,
	CLI_INVALID_SQL_ID					= 8858,
	CLI_UPDATE_PENDING					= 8859,
// ---------------------------------------------------------------------
// Module versioning errors
// ---------------------------------------------------------------------
	CLI_MODULE_HDR_VERSION_ERROR          = 8860,
	CLI_MOD_DLT_HDR_VERSION_ERROR         = 8861,
	CLI_MOD_DLT_ENT_VERSION_ERROR         = 8862,
	CLI_MOD_DESC_HDR_VERSION_ERROR        = 8863,
	CLI_MOD_DESC_ENT_VERSION_ERROR        = 8864,
	CLI_MOD_PLT_HDR_VERSION_ERROR         = 8865,
	CLI_MOD_PLT_ENT_VERSION_ERROR         = 8866,
	CLI_READ_ERROR                        = 8867,
	CLI_CREATE_CONTEXT_EXE_TRANSACTION    = 8868,
	CLI_INVALID_QFO_NUMBER                = 8869,
	CLI_STATEMENT_WITH_NO_QFO             = 8870,
	CLI_NOWAIT_TAG_NOT_SPECIFIED          = 8871,
	CLI_OPERATION_WITH_PENDING_OPS        = 8872,
	CLI_STATEMENT_ASSOCIATED_WITH_QFO     = 8873,

	CLI_SAVEPOINT_ROLLBACK_FAILED         = 8874,
	CLI_SAVEPOINT_ROLLBACK_DONE           = 8875,
	CLI_PARTIAL_UPDATED_DATA              = 8876,
	CLI_AUTO_BEGIN_TRANSACTION_ERROR      = 8877,
	CLI_BUFFER_TOO_SMALL                  = 8879,
	CLI_REMOVE_CURRENT_CONTEXT            = 8880,
	CLI_CONTEXT_NOT_FOUND                 = 8881,
	CLI_NO_SQL_ACCESS_MODE_VIOLATION      = 8882,
	CLI_NOT_CHECK_VIOLATION               = 8883,
	CLI_NO_TRANS_STMT_VIOLATION           = 8884,

	CLI_SEND_ARKCMP_CONTROL				  = 8885,

	CLI_INTERR_ON_CONTEXT_SWITCH          = 8886,

	CLI_GENCODE_BUFFER_TOO_SMALL          = 8887,

	CLI_IUD_IN_PROGRESS					  = 8888,

	CLI_RS_PROXY_BUFFER_SMALL_OR_NULL     = 8889,

	CLI_ARKCMP_INIT_FAILED				  = 8890,
	CLI_NOT_ASCII_CHAR_TYPE				  = 8891,
	CLI_RTD_BUFFER_TOO_SMALL			  = 8892,
	CLI_STMT_DESC_COUNT_MISMATCH          = 8893,
	CLI_RESERVED_ARGUMENT                 = 8894,
	CLI_INVALID_CHARSET_FOR_DESCRIPTOR    = 8895,
	CLI_CHARSET_MISMATCH                  = 8896,

	CLI_SHADOW_RPC_EXCEPTION			  = 8897,
	CLI_INTERNAL_ERROR					  = 8898,
	CLI_LAST_ERROR						  = 8899,

// ---------------------------------------------------------------------
// Diagnostic message errors
// ---------------------------------------------------------------------
	CLI_MSG_CHAR_SET_NOT_SUPPORTED        = 8900,

// ---------------------------------------------------------------------
// Execution errors for user-defined functions and procedures
// ---------------------------------------------------------------------
	EXE_UDR_SERVER_WENT_AWAY              = 8901,
	EXE_UDR_INVALID_HANDLE                = 8902,
	EXE_UDR_ATTEMPT_TO_KILL               = 8903,
	EXE_UDR_REPLY_ERROR                   = 8904,
	EXE_UDR_ACCESS_VIOLATION              = 8905,
	EXE_UDR_INVALID_OR_CORRUPT_REPLY      = 8906,
	EXE_UDR_RESULTSETS_NOT_SUPPORTED      = 8907,
	EXE_UDR_RS_ALLOC_RS_NOT_SUPPORTED     = 8908,
	EXE_UDR_RS_ALLOC_STMT_NOT_CALL        = 8909,
//                                      8910 is used by some other feature
//                                      8911 is used by some other feature
	EXE_UDR_RS_ALLOC_INTERNAL_ERROR       = 8912,
	EXE_UDR_RS_PREPARE_NOT_ALLOWED        = 8913,
	EXE_UDR_RS_REOPEN_NOT_ALLOWED         = 8914,
	EXE_UDR_RS_NOT_AVAILABLE              = 8915,
	EXE_UDR_RS_ALLOC_INVALID_INDEX        = 8916,
	EXE_UDR_RS_ALLOC_ALREADY_EXISTS       = 8917,
	EXE_RTS_NOT_STARTED                   = 8918,
	EXE_RTS_INVALID_QID                   = 8919,
	EXE_RTS_INVALID_CPU_PID               = 8920,
	EXE_RTS_TIMED_OUT                     = 8921,
	EXE_RTS_REQ_PARTIALY_SATISFIED        = 8922,
	EXE_RTS_QID_NOT_FOUND                 = 8923,
	CLI_MERGED_STATS_NOT_AVAILABLE        = 8924,
	CLI_QID_NOT_MATCHING                  = 8925,
	EXE_STAT_NOT_FOUND                    = 8926,
	EXE_ERROR_IN_STAT_ITEM                = 8927,
	CLI_INSUFFICIENT_STATS_DESC_SQL		  = 8928,
	CLI_INSUFFICIENT_SIKEY_BUFF           = 8929,

	CLI_CONSUMER_QUERY_BUF_TOO_SMALL      = 8930, // For parallel extract

	EXE_SG_MAXVALUE_EXCEEDED              = 8934, // For sequence generator
	EXE_SG_UPDATE_FAILURE                 = 8935,

	EXE_UDR_INVALID_DATA                  = 8940,

	CLI_SESSION_ATTR_BUFFER_TOO_SMALL     = 8941,
	CLI_USERNAME_BUFFER_TOO_SMALL         = 8942,

	EXE_ROWLENGTH_EXCEEDS_BUFFER          = 8943,

//-------------------------------------------------------------
// Error codes for bulk replicate - Part 2
//-------------------------------------------------------------
	EXE_INTERNALLY_GENERATED_COMMAND      = 8950,


    EXE_AES_INVALID_IV                    = 8954,
    EXE_ERR_PARAMCOUNT_FOR_FUNC           = 8955,
    EXE_OPTION_IGNORED                    = 8956,
    EXE_OPENSSL_ERROR                     = 8957,
//fast transport

	EXE_EXTRACT_ERROR_CREATING_FILE       = 8960,
	EXE_EXTRACT_ERROR_WRITING_TO_FILE     = 8961,
	EXE_EXTRACT_CANNOT_ALLOCATE_BUFFER    = 8962,

// ---------------------------------------------------------------------
// Execution errors related to JSon parser
// ---------------------------------------------------------------------
    EXE_JSON_INVALID_TOKEN                  = 8971,
    EXE_JSON_INVALID_VALUE                  = 8972,
    EXE_JSON_INVALID_STRING                 = 8973,
    EXE_JSON_INVALID_ARRAY_START            = 8974,
    EXE_JSON_INVALID_ARRAY_NEXT             = 8975,
    EXE_JSON_INVALID_OBJECT_START           = 8976,
    EXE_JSON_INVALID_OBJECT_LABEL           = 8977,
    EXE_JSON_INVALID_OBJECT_NEXT            = 8978,
    EXE_JSON_INVALID_OBJECT_COMMA           = 8979,
    EXE_JSON_INVALID_END                    = 8980,
    EXE_JSON_END_PREMATURELY                = 8981,
    EXE_JSON_UNEXPECTED_ERROR               = 8982,

// ---------------------------------------------------------------------
// Scratch file I/O errors (10100 - 10199)
// ---------------------------------------------------------------------
	EXE_SCR_IO_CREATE                     = 10101,
	EXE_SCR_IO_OPEN                       = 10102,
	EXE_SCR_IO_CLOSE                      = 10103,
	EXE_SCR_IO_WRITE                      = 10104,
	EXE_SCR_IO_READ                       = 10105,

	EXE_SCR_IO_SETMODE                    = 10110,
	EXE_SCR_IO_AWAITIOX                   = 10111,
	EXE_SCR_IO_POSITION                   = 10112,
	EXE_SCR_IO_GETINFO                    = 10113,
	EXE_SCR_IO_GETINFOLIST                = 10114,
	EXE_SCR_IO_GETINFOLISTBYNAME          = 10115,
	EXE_SCR_IO_GET_PHANDLE                = 10116,
	EXE_SCR_IO_DECOMPOSE_PHANDLE          = 10117,
	EXE_SCR_IO_FILENAME_FINDSTART         = 10118,
	EXE_SCR_IO_FILENAME_FINDNEXT          = 10119,

	EXE_SCR_IO_CREATEDIR                  = 10130,
	EXE_SCR_IO_CREATEFILE                 = 10131,
	EXE_SCR_IO_GETTMPFNAME                = 10132,
	EXE_SCR_IO_CLOSEHANDLE                = 10133,
	EXE_SCR_IO_WRITEFILE                  = 10134,
	EXE_SCR_IO_SETFILEPOINTER             = 10135,
	EXE_SCR_IO_CREATEEVENT                = 10136,
	EXE_SCR_IO_WAITMULTOBJ                = 10137,
	EXE_SCR_IO_WAITSINGLEOBJ              = 10138,
	EXE_SCR_IO_GETOVERLAPPEDRESULT        = 10139,
	EXE_SCR_IO_RESETEVENT                 = 10140,
	EXE_SCR_IO_GETDISKFREESPACE           = 10141,

	EXE_SCR_IO_NO_DISKS                   = 10150,
	EXE_SCR_IO_THRESHOLD                  = 10151,
	EXE_SCR_IO_INVALID_BLOCKNUM           = 10152,
	EXE_SCR_IO_UNMAPPED_BLOCKNUM          = 10153,

// ---------------------------------------------------------------------
// Execution errors related to Materialized Views
// ---------------------------------------------------------------------
	CLI_MV_EXECUTE_UNINITIALIZED          = 12301,

// ---------------------------------------------------------------------
// Execution errors related to Rowsets
// ---------------------------------------------------------------------
	EXE_ROWSET_INDEX_OUTOF_RANGE 			= 30008,
	EXE_ROWSET_OVERFLOW          			= 30009,
	EXE_ROWSET_CORRUPTED         			= 30010,

	EXE_ROWSET_NEGATIVE_SIZE     			= 30013,
	EXE_ROWSET_WRONG_SIZETYPE    			= 30014,
	EXE_ROWSET_VARDATA_OR_INDDATA_ERROR     = 30018,
	EXE_ROWSET_SCALAR_ARRAY_MISMATCH 		= 30019,
	EXE_NONFATAL_ERROR_SEEN = 30022,
	EXE_ROWSET_ROW_COUNT_ARRAY_WRONG_SIZE 	= 30023,
	EXE_ROWSET_ROW_COUNT_ARRAY_NOT_AVAILABLE = 30024,
	EXE_NOTATOMIC_ENABLED_AFTER_TRIGGER 	= 30029,
	EXE_NONATOMIC_FAILURE_LIMIT_EXCEEDED 	= 30031,
	CLI_STMT_NEEDS_PREPARE 					= 30032,
	EXE_NONFATAL_ERROR_ON_ALL_ROWS 			= 30035,

	CLI_ROWWISE_ROWSETS_NOT_SUPPORTED 		= 30036,

	CLI_RWRS_DECOMPRESS_ERROR        		= 30045,
	CLI_RWRS_DECOMPRESS_LENGTH_ERROR 		= 30046,
	CLI_NAR_ERROR_DETAILS            		= 30047,

// ---------------------------------------------------------------------
// the trailer (use temporarily for new errors that aren't added yet)
// ---------------------------------------------------------------------
	EXE_NEW_ERROR							= 8999
};


//===================================================================================
const string FMT_EXE_FIRST_ERROR					= "[8000],EXE_FIRST_ERROR";
const string FMT_EXE_INTERNAL_ERROR					= "[8001],EXE_INTERNAL_ERROR";
const string FMT_EXE_NOWAIT_OP_INCOMPLETE			= "[8002],EXE_NOWAIT_OP_INCOMPLETE";
const string FMT_EXE_OUTPUT_DESCRIPTOR_LOCKED   	= "[8003],EXE_OUTPUT_DESCRIPTOR_LOCKED";
const string FMT_EXE_CURSOR_ALREADY_OPEN			= "[8004],EXE_CURSOR_ALREADY_OPEN";
const string FMT_EXE_CURSOR_NOT_OPEN				= "[8005],EXE_CURSOR_NOT_OPEN";
const string FMT_EXE_STREAM_TIMEOUT            		= "[8006],EXE_STREAM_TIMEOUT";
const string FMT_EXE_CANCELED		                = "[8007],EXE_CANCELED";
const string FMT_EXE_INVALID_CAT_NAME				= "[8008],EXE_INVALID_CAT_NAME";
const string FMT_EXE_INVALID_SCH_NAME				= "[8009],EXE_INVALID_SCH_NAME";
const string FMT_EXE_INFO_DEFAULT_CAT_SCH			= "[8010],EXE_INFO_DEFAULT_CAT_SCH";
const string FMT_EXE_BLOCK_CARDINALITY_VIOLATION	= "[8011],EXE_BLOCK_CARDINALITY_VIOLATION";
const string FMT_EXE_INFO_CQD_NAME_VALUE_PAIRS		= "[8012],EXE_INFO_CQD_NAME_VALUE_PAIRS";
const string FMT_EXE_CURSOR_NOT_FETCHED				= "[8013],EXE_CURSOR_NOT_FETCHED";
const string FMT_EXE_CS_EOD               			= "[8014],EXE_CS_EOD";
const string FMT_EXE_CS_EOD_ROLLBACK_ERROR			= "[8015],EXE_CS_EOD_ROLLBACK_ERROR";
const string FMT_EXE_VERSION_ERROR					= "[8016],EXE_VERSION_ERROR";
const string FMT_EXE_NO_EXPLAIN_INFO				= "[8017],EXE_NO_EXPLAIN_INFO";
const string FMT_EXE_PARTN_SKIPPED					= "[8018],EXE_PARTN_SKIPPED";
const string FMT_EXE_EXPLAIN_BAD_DATA				= "[8019],EXE_EXPLAIN_BAD_DATA";

const string FMT_EXE_INITIALIZE_MAINTAIN               = "[8020],EXE_INITIALIZE_MAINTAIN";
const string FMT_EXE_PURGEDATA_CAT                     = "[8021],EXE_PURGEDATA_CAT";
const string FMT_EXE_PARALLEL_PURGEDATA_FAILED         = "[8022],EXE_PARALLEL_PURGEDATA_FAILED";

const string FMT_EXE_QUERY_LIMITS_CPU                  = "[8023],EXE_QUERY_LIMITS_CPU";
const string FMT_EXE_QUERY_LIMITS_CPU_DEBUG            = "[8024],EXE_QUERY_LIMITS_CPU_DEBUG";
const string FMT_EXE_QUERY_LIMITS_CPU_DP2              = "[8025],EXE_QUERY_LIMITS_CPU_DP2";

const string FMT_EXE_CANCEL_QID_NOT_FOUND              = "[8026],EXE_CANCEL_QID_NOT_FOUND";
const string FMT_EXE_CANCEL_TIMEOUT                    = "[8027],EXE_CANCEL_TIMEOUT";
const string FMT_EXE_CANCEL_PROCESS_NOT_FOUND          = "[8028],EXE_CANCEL_PROCESS_NOT_FOUND";
const string FMT_EXE_CANCEL_NOT_AUTHORIZED             = "[8029],EXE_CANCEL_NOT_AUTHORIZED";

// ---------------------------------------------------------------------
// Data integrity errors
// ---------------------------------------------------------------------
const string FMT_EXE_INVALID_DEFINE_OR_ENVVAR          = "[8100],EXE_INVALID_DEFINE_OR_ENVVAR";
const string FMT_EXE_TABLE_CHECK_CONSTRAINT            = "[8101],EXE_TABLE_CHECK_CONSTRAINT"; // SQLSTATE 23000
const string FMT_EXE_DUPLICATE_RECORD                  = "[8102],EXE_DUPLICATE_RECORD"; // SQLSTATE 23000
const string FMT_EXE_RI_CONSTRAINT_VIOLATION           = "[8103],EXE_RI_CONSTRAINT_VIOLATION"; // SQLSTATE 23000
const string FMT_EXE_CHECK_OPTION_VIOLATION_CASCADED   = "[8104],EXE_CHECK_OPTION_VIOLATION_CASCADED"; // SQLSTATE 44000
const string FMT_EXE_CHECK_OPTION_VIOLATION            = "[8105],EXE_CHECK_OPTION_VIOLATION"; // SQLSTATE 44000
const string FMT_EXE_CURSOR_UPDATE_CONFLICT            = "[8106],EXE_CURSOR_UPDATE_CONFLICT";
const string FMT_EXE_HALLOWEEN_INSERT_AUTOCOMMIT       = "[8107],EXE_HALLOWEEN_INSERT_AUTOCOMMIT";
const string FMT_EXE_DUPLICATE_IDENTITY_VALUE          = "[8108],EXE_DUPLICATE_IDENTITY_VALUE"; // SQLSTATE 23000
const string FMT_EXE_INVALID_SESSION_DEFAULT           = "[8109],EXE_INVALID_SESSION_DEFAULT";
const string FMT_EXE_DUPLICATE_ENTIRE_RECORD           = "[8110],EXE_DUPLICATE_ENTIRE_RECORD";
const string FMT_EXE_LAST_INTEGRITY_ERROR              = "[8139],EXE_LAST_INTEGRITY_ERROR";

// ---------------------------------------------------------------------
// Some internal testing "errors"
// ---------------------------------------------------------------------
const string FMT_EXE_CANCEL_INJECTED                   = "[8140],EXE_CANCEL_INJECTED";
const string FMT_EXE_ERROR_INJECTED                    = "[8141],EXE_ERROR_INJECTED";

// ---------------------------------------------------------------------
// Set session default "warning".
// ---------------------------------------------------------------------
const string FMT_EXE_CLEANUP_ESP                       = "[8143],EXE_CLEANUP_ESP";

//----------------------------------------------------------------------
// Late-name resolution and late-binding/similarity check errors.
//----------------------------------------------------------------------
const string FMT_EXE_NAME_MAPPING_ERROR                = "[8300],EXE_NAME_MAPPING_ERROR";
const string FMT_EXE_NAME_MAPPING_FS_ERROR             = "[8301],EXE_NAME_MAPPING_FS_ERROR";
const string FMT_EXE_NAME_MAPPING_NO_PART_AVAILABLE    = "[8302],EXE_NAME_MAPPING_NO_PART_AVAILABLE";
const string FMT_EXE_NAME_MAPPING_BAD_ANCHOR   	   	   = "[8303],EXE_NAME_MAPPING_BAD_ANCHOR";

//----------------------------------------------------------------------
// Available for future use.
//----------------------------------------------------------------------
const string FMT_ASSIGN_INCOMPATIBLE_CHARSET       	  = "[8350,ASSIGN_INCOMPATIBLE_CHARSET";

//----------------------------------------------------------------------
// Expressions errors
//----------------------------------------------------------------------
const string FMT_EXE_INVALID_DEFINE_CLASS_ERROR        	= "[8400],EXE_INVALID_DEFINE_CLASS_ERROR";
const string FMT_EXE_CARDINALITY_VIOLATION				= "[8401],EXE_CARDINALITY_VIOLATION";
const string FMT_EXE_STRING_OVERFLOW					= "[8402],EXE_STRING_OVERFLOW";
const string FMT_EXE_SUBSTRING_ERROR					= "[8403],EXE_SUBSTRING_ERROR";
const string FMT_EXE_TRIM_ERROR							= "[8404],EXE_TRIM_ERROR";
const string FMT_EXE_CONVERTTIMESTAMP_ERROR				= "[8405],EXE_CONVERTTIMESTAMP_ERROR";
const string FMT_EXE_JULIANTIMESTAMP_ERROR				= "[8407],EXE_JULIANTIMESTAMP_ERROR";
const string FMT_EXE_INVALID_ESCAPE_CHARACTER			= "[8409],EXE_INVALID_ESCAPE_CHARACTER";
const string FMT_EXE_INVALID_ESCAPE_SEQUENCE			= "[8410],EXE_INVALID_ESCAPE_SEQUENCE";
const string FMT_EXE_NUMERIC_OVERFLOW					= "[8411],EXE_NUMERIC_OVERFLOW";
const string FMT_EXE_MISSING_NULL_TERMINATOR			= "[8412],EXE_MISSING_NULL_TERMINATOR";
const string FMT_EXE_CONVERT_STRING_ERROR				= "[8413],EXE_CONVERT_STRING_ERROR";
const string FMT_EXE_CONVERT_NOT_SUPPORTED				= "[8414],EXE_CONVERT_NOT_SUPPORTED";
const string FMT_EXE_CONVERT_DATETIME_ERROR				= "[8415],EXE_CONVERT_DATETIME_ERROR";
const string FMT_EXE_DATETIME_FIELD_OVERFLOW           	= "[8416],EXE_DATETIME_FIELD_OVERFLOW";
const string FMT_EXE_USER_FUNCTION_ERROR				= "[8417],EXE_USER_FUNCTION_ERROR";
const string FMT_EXE_USER_FUNCTION_NOT_SUPP				= "[8418],EXE_USER_FUNCTION_NOT_SUPP";
const string FMT_EXE_DIVISION_BY_ZERO					= "[8419],EXE_DIVISION_BY_ZERO";
const string FMT_EXE_MISSING_INDICATOR_VARIABLE        	= "[8420],EXE_MISSING_INDICATOR_VARIABLE";
const string FMT_EXE_ASSIGNING_NULL_TO_NOT_NULL        	= "[8421],EXE_ASSIGNING_NULL_TO_NOT_NULL";
const string FMT_EXE_CONVERT_INTERVAL_ERROR            	= "[8422],EXE_CONVERT_INTERVAL_ERROR";
const string FMT_EXE_FIELD_NUM_OVERFLOW                	= "[8423],EXE_FIELD_NUM_OVERFLOW";
const string FMT_EXE_MATH_FUNC_NOT_SUPPORTED           	= "[8424],EXE_MATH_FUNC_NOT_SUPPORTED";
const string FMT_EXE_DEFAULT_VALUE_ERROR               	= "[8425],EXE_DEFAULT_VALUE_ERROR";
const string FMT_EXE_SORT_ERROR                        	= "[8427],EXE_SORT_ERROR";
const string FMT_EXE_BAD_ARG_TO_MATH_FUNC              	= "[8428],EXE_BAD_ARG_TO_MATH_FUNC";
const string FMT_EXE_MAPPED_FUNCTION_ERROR             	= "[8429],EXE_MAPPED_FUNCTION_ERROR";
const string FMT_EXE_GETBIT_ERROR						= "[8430],EXE_GETBIT_ERROR";
const string FMT_EXE_IS_BITWISE_AND_ERROR				= "[8431],EXE_IS_BITWISE_AND_ERROR";
const string FMT_EXE_UNSIGNED_OVERFLOW                 	= "[8432],EXE_UNSIGNED_OVERFLOW";
const string FMT_EXE_INVALID_CHARACTER                 	= "[8433],EXE_INVALID_CHARACTER";
const string FMT_EXE_HISTORY_BUFFER_TOO_SMALL			= "[8440],EXE_HISTORY_BUFFER_TOO_SMALL";
const string FMT_EXE_OLAP_OVERFLOW_NOT_SUPPORTED       	= "[8441],EXE_OLAP_OVERFLOW_NOT_SUPPORTED";
const string FMT_EXE_LAST_EXPRESSIONS_ERROR				= "[8499],EXE_LAST_EXPRESSIONS_ERROR";

// ---------------------------------------------------------------------
// File System and DP2 errors.
// ---------------------------------------------------------------------
const string FMT_EXE_ERROR_FROM_DP2						= "[8550],EXE_ERROR_FROM_DP2";
const string FMT_EXE_ERROR_FROM_FS2						= "[8551],EXE_ERROR_FROM_FS2";
const string FMT_EXE_FS2_FETCH_VERSION_ERROR			= "[8552],EXE_FS2_FETCH_VERSION_ERROR";
const string FMT_EXE_ERROR_STREAM_OVERFLOW             	= "[8553],EXE_ERROR_STREAM_OVERFLOW";
const string FMT_EXE_EID_INTERNAL_ERROR                	= "[8555],EXE_EID_INTERNAL_ERROR";
const string FMT_EXE_LAST_ERROR_FROM_FS_DP2				= "[8569],EXE_LAST_ERROR_FROM_FS_DP2";

// ---------------------------------------------------------------------
// Build-time and other catastophic errors
// ---------------------------------------------------------------------
const string FMT_EXE_NO_MEM_TO_BUILD					= "[8570],EXE_NO_MEM_TO_BUILD";
const string FMT_EXE_NO_MEM_TO_EXEC 					= "[8571],EXE_NO_MEM_TO_EXEC";
const string FMT_EXE_CANNOT_CONTINUE                   	= "[8572],EXE_CANNOT_CONTINUE";
const string FMT_EXE_ACCESS_VIOLATION                  	= "[8573],EXE_ACCESS_VIOLATION";

// ------------------------------------------------------------
// Error 8574, lost open. Could result in reopening the table.
// Error 8575, could result in recompilation.
// Warning 8576, statement was recompiled.
// ------------------------------------------------------------
const string FMT_EXE_LOST_OPEN                         = "[8574],EXE_LOST_OPEN";
const string FMT_EXE_TIMESTAMP_MISMATCH                = "[8575],EXE_TIMESTAMP_MISMATCH";
const string FMT_EXE_RECOMPILE                         = "[8576],EXE_RECOMPILE";
const string FMT_EXE_TABLE_NOT_FOUND                   = "[8577],EXE_TABLE_NOT_FOUND";
const string FMT_EXE_SIM_CHECK_PASSED                  = "[8578],EXE_SIM_CHECK_PASSED";
const string FMT_EXE_SIM_CHECK_FAILED                  = "[8579],EXE_SIM_CHECK_FAILED";
const string FMT_EXE_PARTITION_UNAVAILABLE             = "[8580],EXE_PARTITION_UNAVAILABLE";
const string FMT_EXE_NO_MEM_FOR_IN_MEM_JOIN			   = "[8581],EXE_NO_MEM_FOR_IN_MEM_JOIN";
const string FMT_EXE_USER_PREPARE_NEEDED               = "[8582],EXE_USER_PREPARE_NEEDED";
const string FMT_EXE_RELEASE_WORK_TIMEOUT              = "[8584],EXE_RELEASE_WORK_TIMEOUT";
const string FMT_EXE_SCHEMA_SECURITY_CHANGED           = "[8585],EXE_SCHEMA_SECURITY_CHANGED";
const string FMT_EXE_ASSIGN_ESPS_ERROR                 = "[8586],EXE_ASSIGN_ESPS_ERROR";
const string FMT_EXE_IAR_MFMAP_BAD                     = "[8590],EXE_IAR_MFMAP_BAD";
const string FMT_EXE_IAR_ERROR_EXTRACTING_COLUMNS      = "[8591],EXE_IAR_ERROR_EXTRACTING_COLUMNS";
const string FMT_EXE_IAR_NO_MFMAP                      = "[8592],EXE_IAR_NO_MFMAP";
const string FMT_EXE_IAR_MISSING_COLS_COMPRESSED_AUDIT = "[8593],EXE_IAR_MISSING_COLS_COMPRESSED_AUDIT";
const string FMT_EXE_AUDIT_IMAGE_EXPR_EVAL_ERROR       = "[8594],EXE_AUDIT_IMAGE_EXPR_EVAL_ERROR";
const string FMT_EXE_MERGE_STMT_ERROR                  = "[8595],EXE_MERGE_STMT_ERROR";
const string FMT_EXE_ESP_CHANGE_PRIORITY_FAILED        = "[8596],EXE_ESP_CHANGE_PRIORITY_FAILED";
const string FMT_EXE_RECOMPILE_AUTO_QUERY_RETRY        = "[8597],EXE_RECOMPILE_AUTO_QUERY_RETRY";
const string FMT_EXE_VIEW_NOT_FOUND                    = "[8598],EXE_VIEW_NOT_FOUND";
const string FMT_EXE_MV_UNAVILABLE                     = "12073],EXE_MV_UNAVILABLE";

//-------------------------------------------------------------
// Errors codes for concurrency control.
//-------------------------------------------------------------
const string FMT_EXE_FIRST_CONCURRENCY_CONTROL_ERROR	= "[8600],EXE_FIRST_CONCURRENCY_CONTROL_ERROR";
const string FMT_EXE_LOCK_UNLOCK_ERROR					= "[8601],EXE_LOCK_UNLOCK_ERROR";
const string FMT_EXE_FILESYSTEM_ERROR					= "[8602],EXE_FILESYSTEM_ERROR";
const string FMT_EXE_BEGIN_TRANSACTION_ERROR			= "[8603],EXE_BEGIN_TRANSACTION_ERROR";
const string FMT_EXE_BEGIN_ERROR_FROM_TRANS_SUBSYS		= "[8604],EXE_BEGIN_ERROR_FROM_TRANS_SUBSYS";
const string FMT_EXE_COMMIT_TRANSACTION_ERROR			= "[8605],EXE_COMMIT_TRANSACTION_ERROR";
const string FMT_EXE_COMMIT_ERROR_FROM_TRANS_SUBSYS		= "[8606],EXE_COMMIT_ERROR_FROM_TRANS_SUBSYS";
const string FMT_EXE_ROLLBACK_TRANSACTION_ERROR			= "[8607],EXE_ROLLBACK_TRANSACTION_ERROR";
const string FMT_EXE_ROLLBACK_ERROR_FROM_TRANS_SUBSYS	= "[8608],EXE_ROLLBACK_ERROR_FROM_TRANS_SUBSYS";
const string FMT_EXE_ROLLBACK_TRANSACTION_WAITED_ERROR	= "[8609],EXE_ROLLBACK_TRANSACTION_WAITED_ERROR";
const string FMT_EXE_ROLLBACK_WAITED_ERROR_TRANS_SUBSYS	= "[8610],EXE_ROLLBACK_WAITED_ERROR_TRANS_SUBSYS";
const string FMT_EXE_SET_TRANS_ERROR_FROM_TRANS_SUBSYS	= "[8612],EXE_SET_TRANS_ERROR_FROM_TRANS_SUBSYS";
const string FMT_EXE_CANT_COMMIT_OR_ROLLBACK           	= "[8613],EXE_CANT_COMMIT_OR_ROLLBACK";
const string FMT_EXE_CANT_BEGIN_WITH_MULTIPLE_CONTEXTS 	= "[8614],EXE_CANT_BEGIN_WITH_MULTIPLE_CONTEXTS";
const string FMT_EXE_CANT_BEGIN_USER_TRANS_WITH_LRU    	= "[8615],EXE_CANT_BEGIN_USER_TRANS_WITH_LRU";
const string FMT_EXE_LAST_CONCURRENCY_CONTROL_ERROR		= "[8629],EXE_LAST_CONCURRENCY_CONTROL_ERROR";

//-------------------------------------------------------------
// Error codes for bulk replicate
//-------------------------------------------------------------
const string FMT_EXE_REPL_TO_UNSUPPORTED_TGT_SYS       	= "[8645],EXE_REPL_TO_UNSUPPORTED_TGT_SYS";
const string FMT_EXE_BDR_ALREADY_INITIALIZED           	= "[8646],EXE_BDR_ALREADY_INITIALIZED";
const string FMT_EXE_BDR_SERVICE_PROCESS_COMM_ERROR    	= "[8647],EXE_BDR_SERVICE_PROCESS_COMM_ERROR";
const string FMT_EXE_REPL_TARGET_REPL_PROCESS_COMM_ERROR = "[8648],EXE_REPL_TARGET_REPL_PROCESS_COMM_ERROR";
const string FMT_EXE_REPL_SRC_TGT_PARTN_MISMATCH       	= "[8650],EXE_REPL_SRC_TGT_PARTN_MISMATCH";
const string FMT_EXE_REPL_SRC_TGT_DDL_MISMATCH         	= "[8651],EXE_REPL_SRC_TGT_DDL_MISMATCH";
const string FMT_EXE_REPL_SRC_TGT_VERSION_MISMATCH     	= "[8652],EXE_REPL_SRC_TGT_VERSION_MISMATCH";
const string FMT_EXE_BDR_REPL_PROCESS_COMM_ERROR       	= "[8653],EXE_BDR_REPL_PROCESS_COMM_ERROR";
const string FMT_EXE_REPL_QUERY_ID_NOT_FOUND           	= "[8654],EXE_REPL_QUERY_ID_NOT_FOUND";
const string FMT_EXE_REPL_COULD_NOT_ABORT_QUERY        	= "[8655],EXE_REPL_COULD_NOT_ABORT_QUERY";
const string FMT_EXE_REPL_QUERY_WAS_ABORTED            	= "[8656],EXE_REPL_QUERY_WAS_ABORTED";
const string FMT_EXE_REPL_COULD_NOT_RECOVER            	= "[8657],EXE_REPL_COULD_NOT_RECOVER";
const string FMT_EXE_REPL_INVALID_IPADDR_OR_PORTNUM    	= "[8658],EXE_REPL_INVALID_IPADDR_OR_PORTNUM";

//-------------------------------------------------------------
// Errors codes for suspend/resume.
//-------------------------------------------------------------
const string FMT_EXE_SUSPEND_AUDIT                     	= "[8670],EXE_SUSPEND_AUDIT";
const string FMT_EXE_SUSPEND_LOCKS                     	= "[8671],EXE_SUSPEND_LOCKS";
const string FMT_EXE_SUSPEND_QID_NOT_ACTIVE            	= "[8672],EXE_SUSPEND_QID_NOT_ACTIVE";
const string FMT_EXE_SUSPEND_GUARDIAN_ERROR_1          	= "[8673],EXE_SUSPEND_GUARDIAN_ERROR_1";
const string FMT_EXE_SUSPEND_GUARDIAN_ERROR_2          	= "[8674],EXE_SUSPEND_GUARDIAN_ERROR_2";
const string FMT_EXE_SUSPEND_SQL_ERROR                 	= "[8675],EXE_SUSPEND_SQL_ERROR";
const string FMT_EXE_SUSPEND_ALREADY_SUSPENDED         	= "[8676],EXE_SUSPEND_ALREADY_SUSPENDED";
const string FMT_EXE_SUSPEND_NOT_SUSPENDED             	= "[8677],EXE_SUSPEND_NOT_SUSPENDED";

//-------------------------------------------------------------
// Errors codes translate function.
//-------------------------------------------------------------
const string FMT_EXE_INVALID_CHAR_IN_TRANSLATE_FUNC    = "[8690],EXE_INVALID_CHAR_IN_TRANSLATE_FUNC";

// ---------------------------------------------------------------------
// Parallel execution
// ---------------------------------------------------------------------
const string FMT_EXE_PARALLEL_EXECUTION_ERROR			= "[8700],EXE_PARALLEL_EXECUTION_ERROR";
const string FMT_EXE_PARALLEL_EXTRACT_OPEN_ERROR       	= "[8701],EXE_PARALLEL_EXTRACT_OPEN_ERROR";
const string FMT_EXE_PARALLEL_EXTRACT_CONNECT_ERROR    	= "[8702],EXE_PARALLEL_EXTRACT_CONNECT_ERROR";

// ---------------------------------------------------------------------
// Warning from updating Measure SQL counters.
// ---------------------------------------------------------------------
const string FMT_EXE_MEASURE                   			= "[8710],EXE_MEASURE";

//----------------------------------------------------------------------
// Errors generated in the CLI code
//----------------------------------------------------------------------
const string FMT_CLI_FIRST_ERROR						= "[8730],CLI_FIRST_ERROR";
const string FMT_CLI_PROBLEM_READING_USERS             	= "[8731],CLI_PROBLEM_READING_USERS";
const string FMT_CLI_USER_NOT_REGISTERED               	= "[8732],CLI_USER_NOT_REGISTERED";
const string FMT_CLI_USER_NOT_VALID                    	= "[8733],CLI_USER_NOT_VALID";
const string FMT_CLI_INVALID_QUERY_PRIVS               	= "[8734],CLI_INVALID_QUERY_PRIVS";

const string FMT_CLI_UNUSED                            	= "[8740],CLI_UNUSED";
const string FMT_CLI_CANNOT_EXECUTE_IN_MEM_DEFN        	= "[8741],CLI_CANNOT_EXECUTE_IN_MEM_DEFN";
const string FMT_CLI_DUPLICATE_DESC						= "[8801],CLI_DUPLICATE_DESC";
const string FMT_CLI_DUPLICATE_STMT						= "[8802],CLI_DUPLICATE_STMT";
const string FMT_CLI_DESC_NOT_EXSISTS					= "[8803],CLI_DESC_NOT_EXSISTS";
const string FMT_CLI_STMT_NOT_EXSISTS					= "[8804],CLI_STMT_NOT_EXSISTS";
const string FMT_CLI_NOT_DYNAMIC_DESC					= "[8805],CLI_NOT_DYNAMIC_DESC";
const string FMT_CLI_NOT_DYNAMIC_STMT					= "[8806],CLI_NOT_DYNAMIC_STMT";
const string FMT_CLI_DATA_OUTOFRANGE					= "[8807],CLI_DATA_OUTOFRANGE";
const string FMT_CLI_MODULEFILE_CORRUPTED				= "[8808],CLI_MODULEFILE_CORRUPTED";
const string FMT_CLI_MODULEFILE_OPEN_ERROR				= "[8809],CLI_MODULEFILE_OPEN_ERROR";

const string FMT_CLI_NO_ERROR_IN_DIAGS                 	= "[8810],CLI_NO_ERROR_IN_DIAGS";

const string FMT_CLI_STMT_NOT_OPEN						= "[8811],CLI_STMT_NOT_OPEN";
const string FMT_CLI_STMT_NOT_CLOSE						= "[8812],CLI_STMT_NOT_CLOSE";
const string FMT_CLI_STMT_CLOSE							= "[8813],CLI_STMT_CLOSE";
const string FMT_CLI_TRANS_MODE_MISMATCH				= "[8814],CLI_TRANS_MODE_MISMATCH";
const string FMT_CLI_TCB_EXECUTE_ERROR					= "[8816],CLI_TCB_EXECUTE_ERROR";
const string FMT_CLI_TCB_FETCH_ERROR					= "[8817],CLI_TCB_FETCH_ERROR";
const string FMT_CLI_TDB_DESCRIBE_ERROR					= "[8818],CLI_TDB_DESCRIBE_ERROR";
const string FMT_CLI_BEGIN_TRANSACTION_ERROR			= "[8819],CLI_BEGIN_TRANSACTION_ERROR";
const string FMT_CLI_COMMIT_TRANSACTION_ERROR			= "[8820],CLI_COMMIT_TRANSACTION_ERROR";

const string FMT_CLI_ROLLBACK_TRANSACTION_ERROR			= "[8821],CLI_ROLLBACK_TRANSACTION_ERROR";
const string FMT_CLI_STMT_NOT_PREPARED					= "[8822],CLI_STMT_NOT_PREPARED";
const string FMT_CLI_IO_REQUESTS_PENDING				= "[8823],CLI_IO_REQUESTS_PENDING";
const string FMT_CLI_NO_MODULE_NAME						= "[8824],CLI_NO_MODULE_NAME";
const string FMT_CLI_MODULE_ALREADY_ADDED				= "[8825],CLI_MODULE_ALREADY_ADDED";
const string FMT_CLI_ADD_MODULE_ERROR					= "[8826],CLI_ADD_MODULE_ERROR";
const string FMT_CLI_SEND_REQUEST_ERROR					= "[8827],CLI_SEND_REQUEST_ERROR";
const string FMT_CLI_OUT_OF_MEMORY						= "[8828],CLI_OUT_OF_MEMORY";
const string FMT_CLI_INVALID_DESC_ENTRY					= "[8829],CLI_INVALID_DESC_ENTRY";
const string FMT_CLI_NO_CURRENT_CONTEXT					= "[8830],CLI_NO_CURRENT_CONTEXT";

const string FMT_CLI_MODULE_NOT_ADDED					= "[8831],CLI_MODULE_NOT_ADDED";
const string FMT_CLI_TRANSACTION_NOT_STARTED			= "[8832],CLI_TRANSACTION_NOT_STARTED";
const string FMT_CLI_INVALID_SQLTRANS_COMMAND			= "[8833],CLI_INVALID_SQLTRANS_COMMAND";
const string FMT_CLI_NO_INSTALL_DIR						= "[8834],CLI_NO_INSTALL_DIR";
const string FMT_CLI_INVALID_DESC_INFO_REQUEST         	= "[8835],CLI_INVALID_DESC_INFO_REQUEST";
const string FMT_CLI_INVALID_UPDATE_COLUMN             	= "[8836],CLI_INVALID_UPDATE_COLUMN";
const string FMT_CLI_INVALID_USERID   					= "[8837],CLI_INVALID_USERID";
const string FMT_CLI_RECEIVE_ERROR   					= "[8838],CLI_RECEIVE_ERROR";
const string FMT_CLI_VALIDATE_TRANSACTION_ERROR        	= "[8839],CLI_VALIDATE_TRANSACTION_ERROR";
const string FMT_CLI_SELECT_INTO_ERROR                 	= "[8401],CLI_SELECT_INTO_ERROR";
const string FMT_CLI_INVALID_OBJECTNAME					= "[8840],CLI_INVALID_OBJECTNAME";

const string FMT_CLI_USER_ENDED_EXE_XN                 	= "[8841],CLI_USER_ENDED_EXE_XN";
const string FMT_CLI_NON_UPDATABLE_SELECT_CURSOR       	= "[8842],CLI_NON_UPDATABLE_SELECT_CURSOR";
const string FMT_CLI_ITEM_NUM_OUT_OF_RANGE             	= "[8843],CLI_ITEM_NUM_OUT_OF_RANGE";
const string FMT_CLI_USER_ENDED_XN_CLEANUP             	= "[8844],CLI_USER_ENDED_XN_CLEANUP";
const string FMT_CLI_INTERR_NULL_TCB                   	= "[8845],CLI_INTERR_NULL_TCB";
const string FMT_CLI_EMPTY_SQL_STMT                    	= "[8846],CLI_EMPTY_SQL_STMT";
const string FMT_CLI_SQLMP_RTD_ERROR					= "[8847],CLI_SQLMP_RTD_ERROR";
const string FMT_CLI_CANCEL_REJECTED                   	= "[8848],CLI_CANCEL_REJECTED";
const string FMT_CLI_NON_CURSOR_UPDEL_TABLE            	= "[8850],CLI_NON_CURSOR_UPDEL_TABLE";
const string FMT_CLI_USER_MEMORY_IN_EXECUTOR_SEGMENT   	= "[8851],CLI_USER_MEMORY_IN_EXECUTOR_SEGMENT";
const string FMT_CLI_CURSOR_CANNOT_BE_HOLDABLE         	= "[8852],CLI_CURSOR_CANNOT_BE_HOLDABLE";
const string FMT_CLI_INVALID_ATTR_NAME                 	= "[8853],CLI_INVALID_ATTR_NAME";
const string FMT_CLI_INVALID_ATTR_VALUE                	= "[8854],CLI_INVALID_ATTR_VALUE";
const string FMT_CLI_CURSOR_ATTR_CANNOT_BE_SET         	= "[8855],CLI_CURSOR_ATTR_CANNOT_BE_SET";
const string FMT_CLI_ARRAY_MAXSIZE_INVALID_ENTRY       	= "[8856],CLI_ARRAY_MAXSIZE_INVALID_ENTRY";
const string FMT_CLI_LOCAL_AUTHENTICATION              	= "[8857],CLI_LOCAL_AUTHENTICATION";
const string FMT_CLI_INVALID_SQL_ID						= "[8858],CLI_INVALID_SQL_ID";
const string FMT_CLI_UPDATE_PENDING                    	= "[8859],CLI_UPDATE_PENDING";
// ---------------------------------------------------------------------
// Module versioning errors
// ---------------------------------------------------------------------
const string FMT_CLI_MODULE_HDR_VERSION_ERROR          	= "[8860],CLI_MODULE_HDR_VERSION_ERROR";
const string FMT_CLI_MOD_DLT_HDR_VERSION_ERROR         	= "[8861],CLI_MOD_DLT_HDR_VERSION_ERROR";
const string FMT_CLI_MOD_DLT_ENT_VERSION_ERROR        	= "[8862],CLI_MOD_DLT_ENT_VERSION_ERROR";
const string FMT_CLI_MOD_DESC_HDR_VERSION_ERROR        	= "[8863],CLI_MOD_DESC_HDR_VERSION_ERROR";
const string FMT_CLI_MOD_DESC_ENT_VERSION_ERROR        	= "[8864],CLI_MOD_DESC_ENT_VERSION_ERROR";
const string FMT_CLI_MOD_PLT_HDR_VERSION_ERROR         	= "[8865],CLI_MOD_PLT_HDR_VERSION_ERROR";
const string FMT_CLI_MOD_PLT_ENT_VERSION_ERROR         	= "[8866],CLI_MOD_PLT_ENT_VERSION_ERROR";
const string FMT_CLI_READ_ERROR                        	= "[8867],CLI_READ_ERROR";
const string FMT_CLI_CREATE_CONTEXT_EXE_TRANSACTION    	= "[8868],CLI_CREATE_CONTEXT_EXE_TRANSACTION";
const string FMT_CLI_INVALID_QFO_NUMBER                	= "[8869],CLI_INVALID_QFO_NUMBER";
const string FMT_CLI_STATEMENT_WITH_NO_QFO             	= "[8870],CLI_STATEMENT_WITH_NO_QFO";
const string FMT_CLI_NOWAIT_TAG_NOT_SPECIFIED          	= "[8871],CLI_NOWAIT_TAG_NOT_SPECIFIED";
const string FMT_CLI_OPERATION_WITH_PENDING_OPS        	= "[8872],CLI_OPERATION_WITH_PENDING_OPS";
const string FMT_CLI_STATEMENT_ASSOCIATED_WITH_QFO     	= "[8873],CLI_STATEMENT_ASSOCIATED_WITH_QFO";

const string FMT_CLI_SAVEPOINT_ROLLBACK_FAILED         	= "[8874],CLI_SAVEPOINT_ROLLBACK_FAILED";
const string FMT_CLI_SAVEPOINT_ROLLBACK_DONE           	= "[8875],CLI_SAVEPOINT_ROLLBACK_DONE";
const string FMT_CLI_PARTIAL_UPDATED_DATA              	= "[8876],CLI_PARTIAL_UPDATED_DATA";
const string FMT_CLI_AUTO_BEGIN_TRANSACTION_ERROR      	= "[8877],CLI_AUTO_BEGIN_TRANSACTION_ERROR";
const string FMT_CLI_BUFFER_TOO_SMALL                  	= "[8879],CLI_BUFFER_TOO_SMALL";
const string FMT_CLI_REMOVE_CURRENT_CONTEXT            	= "[8880],CLI_REMOVE_CURRENT_CONTEXT";
const string FMT_CLI_CONTEXT_NOT_FOUND                 	= "[8881],CLI_CONTEXT_NOT_FOUND";
const string FMT_CLI_NO_SQL_ACCESS_MODE_VIOLATION      	= "[8882],CLI_NO_SQL_ACCESS_MODE_VIOLATION";
const string FMT_CLI_NOT_CHECK_VIOLATION               	= "[8883],CLI_NOT_CHECK_VIOLATION";
const string FMT_CLI_NO_TRANS_STMT_VIOLATION           	= "[8884],CLI_NO_TRANS_STMT_VIOLATION";

const string FMT_CLI_SEND_ARKCMP_CONTROL				= "[8885],CLI_SEND_ARKCMP_CONTROL";

const string FMT_CLI_INTERR_ON_CONTEXT_SWITCH          	= "[8886],CLI_INTERR_ON_CONTEXT_SWITCH";

const string FMT_CLI_GENCODE_BUFFER_TOO_SMALL          	= "[8887],CLI_GENCODE_BUFFER_TOO_SMALL";

const string FMT_CLI_IUD_IN_PROGRESS					= "[8888],CLI_IUD_IN_PROGRESS";

const string FMT_CLI_RS_PROXY_BUFFER_SMALL_OR_NULL     	= "[8889],CLI_RS_PROXY_BUFFER_SMALL_OR_NULL";

const string FMT_CLI_ARKCMP_INIT_FAILED					= "[8890],CLI_ARKCMP_INIT_FAILED";
const string FMT_CLI_NOT_ASCII_CHAR_TYPE				= "[8891],CLI_NOT_ASCII_CHAR_TYPE";
const string FMT_CLI_RTD_BUFFER_TOO_SMALL				= "[8892],CLI_RTD_BUFFER_TOO_SMALL";
const string FMT_CLI_STMT_DESC_COUNT_MISMATCH          	= "[8893],CLI_STMT_DESC_COUNT_MISMATCH";
const string FMT_CLI_RESERVED_ARGUMENT                 	= "[8894],CLI_RESERVED_ARGUMENT";
const string FMT_CLI_INVALID_CHARSET_FOR_DESCRIPTOR    	= "[8895],CLI_INVALID_CHARSET_FOR_DESCRIPTOR";
const string FMT_CLI_CHARSET_MISMATCH                  	= "[8896],CLI_CHARSET_MISMATCH";

const string FMT_CLI_SHADOW_RPC_EXCEPTION				= "[8897],CLI_SHADOW_RPC_EXCEPTION";
const string FMT_CLI_INTERNAL_ERROR						= "[8898],CLI_INTERNAL_ERROR";
const string FMT_CLI_LAST_ERROR							= "[8899],CLI_LAST_ERROR";

// ---------------------------------------------------------------------
// Diagnostic message errors
// ---------------------------------------------------------------------
const string FMT_CLI_MSG_CHAR_SET_NOT_SUPPORTED        	= "[8900],CLI_MSG_CHAR_SET_NOT_SUPPORTED";

// ---------------------------------------------------------------------
// Execution errors for user-defined functions and procedures
// ---------------------------------------------------------------------
const string FMT_EXE_UDR_SERVER_WENT_AWAY              	= "[8901],EXE_UDR_SERVER_WENT_AWAY";
const string FMT_EXE_UDR_INVALID_HANDLE                	= "[8902],EXE_UDR_INVALID_HANDLE";
const string FMT_EXE_UDR_ATTEMPT_TO_KILL               	= "[8903],EXE_UDR_ATTEMPT_TO_KILL";
const string FMT_EXE_UDR_REPLY_ERROR                   	= "[8904],EXE_UDR_REPLY_ERROR";
const string FMT_EXE_UDR_ACCESS_VIOLATION              	= "[8905],EXE_UDR_ACCESS_VIOLATION";
const string FMT_EXE_UDR_INVALID_OR_CORRUPT_REPLY      	= "[8906],EXE_UDR_INVALID_OR_CORRUPT_REPLY";
const string FMT_EXE_UDR_RESULTSETS_NOT_SUPPORTED      	= "[8907],EXE_UDR_RESULTSETS_NOT_SUPPORTED";
const string FMT_EXE_UDR_RS_ALLOC_RS_NOT_SUPPORTED     	= "[8908],EXE_UDR_RS_ALLOC_RS_NOT_SUPPORTED";
const string FMT_EXE_UDR_RS_ALLOC_STMT_NOT_CALL        	= "[8909],EXE_UDR_RS_ALLOC_STMT_NOT_CALL";
//                                      8910 is used by some other feature
//                                      8911 is used by some other feature
const string FMT_EXE_UDR_RS_ALLOC_INTERNAL_ERROR       	= "[8912],EXE_UDR_RS_ALLOC_INTERNAL_ERROR";
const string FMT_EXE_UDR_RS_PREPARE_NOT_ALLOWED        	= "[8913],EXE_UDR_RS_PREPARE_NOT_ALLOWED";
const string FMT_EXE_UDR_RS_REOPEN_NOT_ALLOWED         	= "[8914],EXE_UDR_RS_REOPEN_NOT_ALLOWED";
const string FMT_EXE_UDR_RS_NOT_AVAILABLE              	= "[8915],EXE_UDR_RS_NOT_AVAILABLE";
const string FMT_EXE_UDR_RS_ALLOC_INVALID_INDEX        	= "[8916],EXE_UDR_RS_ALLOC_INVALID_INDEX";
const string FMT_EXE_UDR_RS_ALLOC_ALREADY_EXISTS       	= "[8917],EXE_UDR_RS_ALLOC_ALREADY_EXISTS";
const string FMT_EXE_RTS_NOT_STARTED                   	= "[8918],EXE_RTS_NOT_STARTED";
const string FMT_EXE_RTS_INVALID_QID                   	= "[8919],EXE_RTS_INVALID_QID";
const string FMT_EXE_RTS_INVALID_CPU_PID               	= "[8920],EXE_RTS_INVALID_CPU_PID";
const string FMT_EXE_RTS_TIMED_OUT                     	= "[8921],EXE_RTS_TIMED_OUT";
const string FMT_EXE_RTS_REQ_PARTIALY_SATISFIED        	= "[8922],EXE_RTS_REQ_PARTIALY_SATISFIED";
const string FMT_EXE_RTS_QID_NOT_FOUND                 	= "[8923],EXE_RTS_QID_NOT_FOUND";
const string FMT_CLI_MERGED_STATS_NOT_AVAILABLE        	= "[8924],CLI_MERGED_STATS_NOT_AVAILABLE";
const string FMT_CLI_QID_NOT_MATCHING                  	= "[8925],CLI_QID_NOT_MATCHING";
const string FMT_EXE_STAT_NOT_FOUND                    	= "[8926],EXE_STAT_NOT_FOUND";
const string FMT_EXE_ERROR_IN_STAT_ITEM                	= "[8927],EXE_ERROR_IN_STAT_ITEM";
const string FMT_CLI_INSUFFICIENT_STATS_DESC_SQL		= "[8928],CLI_INSUFFICIENT_STATS_DESC";
const string FMT_CLI_INSUFFICIENT_SIKEY_BUFF			= "[8929],CLI_INSUFFICIENT_SIKEY_BUFF";

const string FMT_CLI_CONSUMER_QUERY_BUF_TOO_SMALL		= "[8930],CLI_CONSUMER_QUERY_BUF_TOO_SMALL"; // For parallel extract

const string FMT_EXE_SG_MAXVALUE_EXCEEDED				= "[8934],EXE_SG_MAXVALUE_EXCEEDED"; // For sequence generator
const string FMT_EXE_SG_UPDATE_FAILURE					= "[8935],EXE_SG_UPDATE_FAILURE";

const string FMT_EXE_UDR_INVALID_DATA					= "[8940],EXE_UDR_INVALID_DATA";

const string FMT_CLI_SESSION_ATTR_BUFFER_TOO_SMALL		= "[8941],CLI_SESSION_ATTR_BUFFER_TOO_SMALL";
const string FMT_CLI_USERNAME_BUFFER_TOO_SMALL			= "[8942],CLI_USERNAME_BUFFER_TOO_SMALL";

const string FMT_EXE_ROWLENGTH_EXCEEDS_BUFFER			= "[8943],EXE_ROWLENGTH_EXCEEDS_BUFFER";

//-------------------------------------------------------------
// Error codes for bulk replicate - Part 2
//-------------------------------------------------------------
const string FMT_EXE_INTERNALLY_GENERATED_COMMAND		= "[8950],EXE_INTERNALLY_GENERATED_COMMAND";

//-------------------------------------------------------------
// Error codes for AES encrypt/decrypt functions
// ------------------------------------------------------------
const string FMT_EXE_AES_INVALID_IV                     = "[8954],EXE_AES_INVALID_IV";
const string FMT_EXE_ERR_PARAMCOUNT_FOR_FUNC            = "[8955],EXE_ERR_PARAMCOUNT_FOR_FUNC";
const string FMT_EXE_OPTION_IGNORED                     = "[8956],EXE_OPTION_IGNORED";
const string FMT_EXE_OPENSSL_ERROR                      = "[8957],EXE_OPENSSL_ERROR";


//fast transport

const string FMT_EXE_EXTRACT_ERROR_CREATING_FILE		= "[8960],EXE_EXTRACT_ERROR_CREATING_FILE";
const string FMT_EXE_EXTRACT_ERROR_WRITING_TO_FILE		= "[8961],EXE_EXTRACT_ERROR_WRITING_TO_FILE";
const string FMT_EXE_EXTRACT_CANNOT_ALLOCATE_BUFFER		= "[8962],EXE_EXTRACT_CANNOT_ALLOCATE_BUFFER";

// ---------------------------------------------------------------------
// Execution errors related to JSon parser
// ---------------------------------------------------------------------
const string FMT_EXE_JSON_INVALID_TOKEN                 = "[8971],EXE_JSON_INVALID_TOKEN";
const string FMT_EXE_JSON_INVALID_VALUE                 = "[8972],EXE_JSON_INVALID_VALUE";
const string FMT_EXE_JSON_INVALID_STRING                = "[8973],EXE_JSON_INVALID_STRING";
const string FMT_EXE_JSON_INVALID_ARRAY_START           = "[8974],EXE_JSON_INVALID_ARRAY_START";
const string FMT_EXE_JSON_INVALID_ARRAY_NEXT            = "[8975],EXE_JSON_INVALID_ARRAY_NEXT";
const string FMT_EXE_JSON_INVALID_OBJECT_START          = "[8976],EXE_JSON_INVALID_OBJECT_START";
const string FMT_EXE_JSON_INVALID_OBJECT_LABEL          = "[8977],EXE_JSON_INVALID_OBJECT_LABEL";
const string FMT_EXE_JSON_INVALID_OBJECT_NEXT           = "[8978],EXE_JSON_INVALID_OBJECT_NEXT";
const string FMT_EXE_JSON_INVALID_OBJECT_COMMA          = "[8979],EXE_JSON_INVALID_OBJECT_COMMA";
const string FMT_EXE_JSON_INVALID_END                   = "[8980],EXE_JSON_INVALID_END";
const string FMT_EXE_JSON_END_PREMATURELY               = "[8981],EXE_JSON_END_PREMATURELY";
const string FMT_EXE_JSON_UNEXPECTED_ERROR              = "[8982],EXE_JSON_UNEXPECTED_ERROR";

// ---------------------------------------------------------------------
// Scratch file I/O errors (10100 - 10199)
// ---------------------------------------------------------------------
const string FMT_EXE_SCR_IO_CREATE						= "[10101],EXE_SCR_IO_CREATE";
const string FMT_EXE_SCR_IO_OPEN						= "[10102],EXE_SCR_IO_OPEN";
const string FMT_EXE_SCR_IO_CLOSE						= "[10103],EXE_SCR_IO_CLOSE";
const string FMT_EXE_SCR_IO_WRITE						= "[10104],EXE_SCR_IO_WRITE";
const string FMT_EXE_SCR_IO_READ						= "[10105],EXE_SCR_IO_READ";

const string FMT_EXE_SCR_IO_SETMODE						= "[10110],EXE_SCR_IO_SETMODE";
const string FMT_EXE_SCR_IO_AWAITIOX					= "[10111],EXE_SCR_IO_AWAITIOX";
const string FMT_EXE_SCR_IO_POSITION					= "[10112],EXE_SCR_IO_POSITION";
const string FMT_EXE_SCR_IO_GETINFO						= "[10113],EXE_SCR_IO_GETINFO";
const string FMT_EXE_SCR_IO_GETINFOLIST					= "[10114],EXE_SCR_IO_GETINFOLIST";
const string FMT_EXE_SCR_IO_GETINFOLISTBYNAME			= "[10115],EXE_SCR_IO_GETINFOLISTBYNAME";
const string FMT_EXE_SCR_IO_GET_PHANDLE					= "[10116],EXE_SCR_IO_GET_PHANDLE";
const string FMT_EXE_SCR_IO_DECOMPOSE_PHANDLE			= "[10117],EXE_SCR_IO_DECOMPOSE_PHANDLE";
const string FMT_EXE_SCR_IO_FILENAME_FINDSTART			= "[10118],EXE_SCR_IO_FILENAME_FINDSTART";
const string FMT_EXE_SCR_IO_FILENAME_FINDNEXT			= "[10119],EXE_SCR_IO_FILENAME_FINDNEXT";

const string FMT_EXE_SCR_IO_CREATEDIR					= "[10130],EXE_SCR_IO_CREATEDIR";
const string FMT_EXE_SCR_IO_CREATEFILE					= "[10131],EXE_SCR_IO_CREATEFILE";
const string FMT_EXE_SCR_IO_GETTMPFNAME					= "[10132],EXE_SCR_IO_GETTMPFNAME";
const string FMT_EXE_SCR_IO_CLOSEHANDLE					= "[10133],EXE_SCR_IO_CLOSEHANDLE";
const string FMT_EXE_SCR_IO_WRITEFILE					= "[10134],EXE_SCR_IO_WRITEFILE";
const string FMT_EXE_SCR_IO_SETFILEPOINTER				= "[10135],EXE_SCR_IO_SETFILEPOINTER";
const string FMT_EXE_SCR_IO_CREATEEVENT					= "[10136],EXE_SCR_IO_CREATEEVENT";
const string FMT_EXE_SCR_IO_WAITMULTOBJ					= "[10137],EXE_SCR_IO_WAITMULTOBJ";
const string FMT_EXE_SCR_IO_WAITSINGLEOBJ				= "[10138],EXE_SCR_IO_WAITSINGLEOBJ";
const string FMT_EXE_SCR_IO_GETOVERLAPPEDRESULT			= "[10139],EXE_SCR_IO_GETOVERLAPPEDRESULT";
const string FMT_EXE_SCR_IO_RESETEVENT					= "[10140],EXE_SCR_IO_RESETEVENT";
const string FMT_EXE_SCR_IO_GETDISKFREESPACE			= "[10141],EXE_SCR_IO_GETDISKFREESPACE";

const string FMT_EXE_SCR_IO_NO_DISKS					= "[10150],EXE_SCR_IO_NO_DISKS";
const string FMT_EXE_SCR_IO_THRESHOLD					= "[10151],EXE_SCR_IO_THRESHOLD";
const string FMT_EXE_SCR_IO_INVALID_BLOCKNUM			= "[10152],EXE_SCR_IO_INVALID_BLOCKNUM";
const string FMT_EXE_SCR_IO_UNMAPPED_BLOCKNUM			= "[10153],EXE_SCR_IO_UNMAPPED_BLOCKNUM";

// ---------------------------------------------------------------------
// Execution errors related to Materialized Views
// ---------------------------------------------------------------------
const string FMT_CLI_MV_EXECUTE_UNINITIALIZED			= "[12301],CLI_MV_EXECUTE_UNINITIALIZED";

// ---------------------------------------------------------------------
// Execution errors related to Rowsets
// ---------------------------------------------------------------------
const string FMT_EXE_ROWSET_INDEX_OUTOF_RANGE			= "[30008],EXE_ROWSET_INDEX_OUTOF_RANGE";
const string FMT_EXE_ROWSET_OVERFLOW					= "[30009],EXE_ROWSET_OVERFLOW";
const string FMT_EXE_ROWSET_CORRUPTED					= "[30010],EXE_ROWSET_CORRUPTED";

const string FMT_EXE_ROWSET_NEGATIVE_SIZE				= "[30013],EXE_ROWSET_NEGATIVE_SIZE";
const string FMT_EXE_ROWSET_WRONG_SIZETYPE				= "[30014],EXE_ROWSET_WRONG_SIZETYPE";
const string FMT_EXE_ROWSET_VARDATA_OR_INDDATA_ERROR	= "[30018],EXE_ROWSET_VARDATA_OR_INDDATA_ERROR";
const string FMT_EXE_ROWSET_SCALAR_ARRAY_MISMATCH		= "[30019],EXE_ROWSET_SCALAR_ARRAY_MISMATCH";
const string FMT_EXE_NONFATAL_ERROR_SEEN				= "[30022],EXE_NONFATAL_ERROR_SEEN";
const string FMT_EXE_ROWSET_ROW_COUNT_ARRAY_WRONG_SIZE	= "[30023],EXE_ROWSET_ROW_COUNT_ARRAY_WRONG_SIZE";
const string FMT_EXE_ROWSET_ROW_COUNT_ARRAY_NOT_AVAILABLE	= "[30024],EXE_ROWSET_ROW_COUNT_ARRAY_NOT_AVAILABLE";
const string FMT_EXE_NOTATOMIC_ENABLED_AFTER_TRIGGER	= "[30029],EXE_NOTATOMIC_ENABLED_AFTER_TRIGGER";
const string FMT_EXE_NONATOMIC_FAILURE_LIMIT_EXCEEDED	= "[30031],EXE_NONATOMIC_FAILURE_LIMIT_EXCEEDED";
const string FMT_CLI_STMT_NEEDS_PREPARE					= "[30032],CLI_STMT_NEEDS_PREPARE";
const string FMT_EXE_NONFATAL_ERROR_ON_ALL_ROWS			= "[30035],EXE_NONFATAL_ERROR_ON_ALL_ROWS";

const string FMT_CLI_ROWWISE_ROWSETS_NOT_SUPPORTED		= "[30036],CLI_ROWWISE_ROWSETS_NOT_SUPPORTED";

const string FMT_CLI_RWRS_DECOMPRESS_ERROR				= "[30045],CLI_RWRS_DECOMPRESS_ERROR";
const string FMT_CLI_RWRS_DECOMPRESS_LENGTH_ERROR		= "[30046],CLI_RWRS_DECOMPRESS_LENGTH_ERROR";
const string FMT_CLI_NAR_ERROR_DETAILS					= "[30047],CLI_NAR_ERROR_DETAILS";

//===================================================================================
//===================================================================================
QUERY_STATE convertMXSubStateToState(NDCS_SUBSTATE mxsrvr_substate);

int GetSetSegment(int& my_segment, bool bSetGet=false);
char* GetNameOfPrimarySegment();

extern int64* pTimestampDelay;
int64 QSTIMESTAMP();
int64 LQSTIMESTAMP();
void SET_QSTIMESTAMP();
string getQueryTypeString(long queryType);
string getSQLStateString(unsigned long state);
string getQueryStateString(const QUERY_STATE state);
string getQuerySubStateString(const QUERY_STATE state);
string getWarnLevelString(unsigned short value);
string getActionString(RULE_SPEC_ACT action);
string getWarnReasonString(short reason);
string getQueueStateString(const QUEUE_STATE state);
string getQueuePrtyString(const QUEUE_PRTY prty);
string getGlobalStateString(const GLOBAL_STATE state);
string getProcessTypeString(PROCESS_TYPE type);
string getRuleTypeString(RULE_TYPE value);
string getRuleOperString(RULE_SPEC_OPER value);
string getRuleExprString(RULE_TYPE type,
	                     RULE_EXPR_LOPND lopnd,
					     RULE_EXPR_OPER oper,
						 short operNum,
					     RULE_EXPR_ROPND ropnd,
						 int64 ropndNum,
						 char * ropndStr);
string getRuleExprLopndString(RULE_EXPR_LOPND value);
string getRuleExprOperString(RULE_TYPE type,
	                         RULE_EXPR_LOPND lopnd,
	                         RULE_EXPR_OPER oper,
							 short operNum);
string getRuleExprRopndString(RULE_TYPE type,
							  RULE_EXPR_LOPND lopnd,
							  RULE_EXPR_OPER oper,
						      RULE_EXPR_ROPND ropnd,
	                          int64 ropndNum,
							  char * ropndStr);
string getRuleWarnLevelString(int value);
string getRuleActionString(RULE_SPEC_ACT value,
	                       char * sqlCmds);
string getRuleAggrQueryTypesString(short value);
string getRuleAggrStatsOnceString(short value);

string getExeErrorCodeString(int value);


#endif

int64  utc2lct_useconds_jts(const int64 utcTimestamp);
int64  lct2utc_useconds_jts(const int64 lctTimestamp);
bool checkPrivMask( char *privStr, bitmask_type bitMask );
