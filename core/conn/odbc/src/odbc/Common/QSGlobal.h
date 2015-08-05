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

#ifndef QSGLOBAL_H
#define QSGLOBAL_H

#include "MemLeak.h"
#include "wmsversbin.h"

#define WAIT_10_SECONDS 10

#define SERVICES_PORT_NUM	5110
#define RCNODEOK	           0
#define RCNODENOTWMS           4


const int64 ONE_KB = 1024;
const int64 ONE_MB = ONE_KB * ONE_KB;
const int64 ONE_GB = ONE_KB * ONE_MB;

#define USERNAME_LENGTH 128

#define MAX_QUERY_ID_LEN			160
#define RMS_STORE_SQL_SOURCE_LEN	254
#define MAX_TCP_PROCESS_NAME		50

#define MAX_CLUSTER_SIZE 32
#define MAX_CHECK_BUFFER 400
#define MAX_CHECK_COMMENT_LEN 50

#define DEFAULT_MAX_USERS_SERVICES		255
#define NUMBER_OF_PREDEFINED_SERVICES	4
#define TOTAL_SERVICES (DEFAULT_MAX_USERS_SERVICES + NUMBER_OF_PREDEFINED_SERVICES)

//=================== tmp cli.h =====================

#define CLI_INSUFFICIENT_STATS_DESC	8928

#define MAX_IDENTIFIER_INT_LEN		128
#define MAX_IDENTIFIER_EXT_LEN		2 * MAX_IDENTIFIER_INT_LEN + 2
#define MAX_DELIMIDENT_LEN			MAX_IDENTIFIER_EXT_LEN

#define MAX_ANSI_NAME_EXT_LEN		3 * MAX_IDENTIFIER_EXT_LEN + 2
#define MAX_ANSI_NAME_INT_LEN		3 * MAX_IDENTIFIER_INT_LEN + 2

//====================== tcp state ===================================================

#define TCP_STATE_CLOSED		0
#define TCP_STATE_LISTEN		1
#define TCP_STATE_SYNC_SENT		2
#define TCP_STATE_SYNC_RECV		3
#define TCP_STATE_ESTAB			4
#define TCP_STATE_CLOSE_WAIT	5
#define TCP_STATE_FIN_WAIT_1	6
#define TCP_STATE_CLOSING		7
#define TCP_STATE_LAST_ACK		8
#define TCP_STATE_FIN_WAIT_2	9
#define TCP_STATE_TIME_WAIT		10
#define TCP_STATE_FAIL_WAIT		11
#define TCP_STATE_UNKNOWN		99

//===================================================================================
//============================ offender ============================================
//===================================================================================
//
typedef enum _PROCESS_TYPE
{
	PROCESS_INIT = 0,
	PROCESS_SRVR,
	PROCESS_CMP,
	PROCESS_ESP,
	PROCESS_UDR,
	PROCESS_CI,
	PROCESS_OTHER,
	PROCESS_ALL,
	PROCESS_SQL,
	PROCESS_WMS,

} PROCESS_TYPE;


//---- string for PROCESS_GETINFOLIST_ -------------------
typedef struct
{
	short len;
	char string[1];
}* PSTRING;

#define MAX_SQ_PROCESSES		256
#define MAX_SQ_PSTATCK_LINES	300
#define MAX_SQ_PSTACK_LINE_LENGTH 80

#define MAX_CLUSTER_NODES	256
#define MAX_FRONT_END_NODES	MAX_CLUSTER_NODES
#define MAX_SSD_NODES	MAX_CLUSTER_NODES

#define NODE_INIT		0
#define NODE_ACTIVE		0x0001
#define NODE_WITH_WMS	0x0002
#define NODE_PRIMARY	0x0004
#define NODE_VIRTUAL	0x0008
#define NODE_WITH_SQL	0x0010
#define NODE_WITH_SSD	0x0020

#define PROC_DEATH_QSMGR	0x0001
#define PROC_DEATH_QSSEND	0x0002

typedef struct _FRONT_END_NODE
{
	_FRONT_END_NODE() : wms_nid(0), status(NODE_INIT), nid(0), services_port(0), proc_death(0) {bzero(services_ip_address,sizeof(services_ip_address));}
	int wms_nid;		//nid with WMS which processes requests comming to this node
	unsigned short status;
	int nid;
	char services_ip_address[MAX_IP_ADDRESS_LEN];
	int services_port;
	unsigned short proc_death;
}FRONT_END_NODE,* PFRONT_END_NODE;

typedef struct _CLUSTER_NODE
{
	_CLUSTER_NODE() : m_status(NODE_INIT), m_NodeCpuBusy(0.0), m_NodeMemoryUsage(0.0),m_NodeKBMainTotal(0),
			m_NodeDiskCache(0.0), m_NodeDiskIo(0.0), m_SSDUsage(0.0), m_NodeEspCount(0),
			m_NodeExecutingQueries(0), m_NodeWaitingQueries(0), m_NodeHoldQueries(0), m_NodeSuspendedQueries(0),
			m_nid(0), m_WmsNid(0), m_bWarning(false), m_LastMulticastUpdate(0) {
		bzero(m_ServiceWaitingQueries, TOTAL_SERVICES );
		bzero(m_ServiceExecutingQueries, TOTAL_SERVICES);
		bzero(m_ServiceHoldQueries, TOTAL_SERVICES);
		bzero(m_ServiceSuspendedQueries, TOTAL_SERVICES);
		bzero(m_nodeName, MAX_NODE_NAME+1);
	}
	unsigned short m_status;
	bool	m_bWarning;
	int64	m_LastMulticastUpdate;
	char	m_nodeName[MAX_NODE_NAME+1];

	float	m_NodeCpuBusy;
	float	m_NodeMemoryUsage;
	unsigned long m_NodeKBMainTotal;
	float	m_NodeDiskCache;

	float	m_NodeDiskIo;
	float	m_SSDUsage;
	long	m_NodeEspCount;
//
	int		m_NodeExecutingQueries; //it is total number of executing queries in this node
	int		m_NodeWaitingQueries;
	int		m_NodeHoldQueries;
	int		m_NodeSuspendedQueries;
//
	int		m_ServiceWaitingQueries[TOTAL_SERVICES];	 		// current waiting queries for the service
	int		m_ServiceExecutingQueries[TOTAL_SERVICES]; 			// current executing queries for the service
	int		m_ServiceHoldQueries[TOTAL_SERVICES];		 		// current holding queries for the service
	int		m_ServiceSuspendedQueries[TOTAL_SERVICES]; 			// current suspended queries for the service
//
	int 	m_nid;
	int		m_WmsNid;

}CLUSTER_NODE,* PCLUSTER_NODE;

typedef struct _SSD_NODE
{
	_SSD_NODE() : m_status(NODE_INIT), m_SSDTotalMemory(0.0), m_SSDUsageMemory(0.0), m_SSDUsageMemoryPercentage(0.0), m_nid(0) {bzero(m_SSDMountPoint,sizeof(m_SSDMountPoint));}
	unsigned short m_status;
	int 	m_nid;
	float	m_SSDTotalMemory;
	float	m_SSDUsageMemory;
	float	m_SSDUsageMemoryPercentage;
	char	m_SSDMountPoint[50];

}SSD_NODE,* PSSD_NODE;

//ssd overflow
typedef enum _OVERFLOW_MODE
{
	OVERFLOW_MODE_UNKNOWN = -1,
	OVERFLOW_MODE_DISK,
	OVERFLOW_MODE_SSD,
	OVERFLOW_MODE_HYBRID
} OVERFLOW_MODE;

typedef struct _COMP_OVERFLOW
{
	unsigned short m_CMP_NumOfBMOs;
	int64          m_CMP_EST_OverflowSize;
	OVERFLOW_MODE  m_CMP_OverflowMode;
} COMP_OVERFLOW, *PCOMP_OVERFLOW;

typedef struct _EXEC_OVERFLOW
{
	unsigned int 	m_OvfFileCount;			//scratch file count
	int64           m_OvfSpaceAllocated;    //no. of scratch files used * 2GB
	int64           m_OvfSpaceUsed;         //scratch buffer blocks written * scratch buffer block size
	int64           m_OvfBlockSize;         //scratch buffer block size
	int64           m_OvfIOs;               //scratch buffer blocks read + scratch buffer blocks written
	int64           m_OvfMessageBuffersTo;  //no. of buffer blocks written
	int64           m_OvfMessageTo;         //no. of times buffer blocks written
	int64           m_OvfMessageBytesTo;    //scratch buffer blocks written * scratch buffer block size
	int64           m_OvfMessageBuffersFrom;//no. of  buffer blocks read
	int64           m_OvfMessageFrom;       //no. of times buffer blocks read
	int64           m_OvfMessageBytesFrom;  //scratch buffer blocks read * scratch buffer block size
} EXEC_OVERFLOW, *PEXEC_OVERFLOW;

#define OVERFLOW_UNKNOWN		"UNKNOWN"
#define OVERFLOW_DISK			"DISK"
#define OVERFLOW_SSD			"SSD"
#define OVERFLOW_HYBRID			"HYBRID"

#define OVERFLOW_MODE_NAME		"OVERFLOW_MODE"
#define OVERFLOW_MODE_NAME_LEN  13

#define MAX_OVERFLOW_STR_LEN	10      //ssd overflow
#define MAX_SEGMENT_NAME_LEN	10      // e.g. \WMS0101
#define MAX_SSDVOLUME_NAME_LEN	20      // e.g. \WMS0101.$SC0010
#define MAX_YESNO_LEN			3       // e.g. YES | NO

#define ONE_MINUTE_TIMESTAMP ((long long) (60 * 1000 * 1000LL))
#define ONE_SECOND_TIMESTAMP ((long long) (1000 * 1000LL))

//From Common/wmsversbin.h
#define QUERY_SERVICE_MAJOR		VERS_CV_MAJ
#define QUERY_SERVICE_MINOR		VERS_CV_MIN
#define QUERY_SERVICE_BUILD		VERS_CV_UPD


#define QS_SRVR_DEBUG_BREAK		2
#define STATS_SRVR_DEBUG_BREAK	4
#define COM_SRVR_DEBUG_BREAK	8
#define SYNC_SRVR_DEBUG_BREAK	16
#define RULE_SRVR_DEBUG_BREAK	32
#define OFFND_SRVR_DEBUG_BREAK	64

#define QS_TRANSPORT_TRACE		0x0001
#define STATS_TRANSPORT_TRACE	0x0002
#define COM_TRANSPORT_TRACE		0x0004
#define SYNC_TRANSPORT_TRACE	0x0008
#define RULE_TRANSPORT_TRACE	0x0016
#define OFFND_TRANSPORT_TRACE	0x0032


#define INITIALIZATION_ERROR_0	990
#define INITIALIZATION_ERROR_1	991
#define INITIALIZATION_ERROR_2	992
#define INITIALIZATION_ERROR_3	993
#define INITIALIZATION_ERROR_4	994
#define INITIALIZATION_ERROR_5	995
#define INITIALIZATION_ERROR_6	996
#define INITIALIZATION_ERROR_7	997
#define INITIALIZATION_ERROR_8	998
#define SERVER_THROWS_ERROR_MESSAGE	999

#define MAX_NAME_LEN				50
#define MAX_ROLE_LEN				128
#define MAX_ALIAS_LEN				50
#define MAX_COMMENT_LEN				256*4
#define MAX_ACTIVE_TIME_LEN			20
#define MAX_TRACE_FILENAME_LEN		20
#define MAX_TRACE_FILEPATH_LEN		256
#define MAX_PARSER_BUF_LEN			1024*4 //See union in NskComDllParser.y
#define MAX_SQL_CMD_LEN				1024 
#define MAX_QUERYNAME_LEN			50*4 //Concatenation of Computer || Application names
#define MAX_COMPUTER_NAME_LEN		16*4 //limited in ODBC driver
#define MAX_APPLICATION_NAME_LEN	128*4 //limited in ODBC driver
#define MAX_TXN_STR_LEN				64
#define SUB_QRY_TYPE_LEN 36
#define PAR_SYS_NAME_LEN 128
#define MAX_CANARY_LEN 1024

#define HP_DEFAULT_SERVICE	"HP_DEFAULT_SERVICE"

#define ROLE_PREFIX					"ROLE"
#define ROLE_PREFIX_LEN				4

#define MAX_RS_POOL_COUNT			5	// Pool of RS Collectors maintained by QSMGR for MXOSRVRs


#define MAX_SERVICE_NAME_LEN	24*4
#define MAX_LIMIT_VALUE_LEN		1024
#define MAX_ACTION_VALUE_LEN	1024

#define MAX_MEM_USAGE_LOW		0
#define MAX_MEM_USAGE_HIGH		100
#define MAX_MEM_USAGE_DEFAULT	85

#define MAX_SSD_USAGE_LOW		0
#define MAX_SSD_USAGE_HIGH		100
#define MAX_SSD_USAGE_DEFAULT	90

#define MAX_NODE_NAME			128

#define MAX_CUR_EXEC_QUERIES_LOW		0
#define MAX_CUR_EXEC_QUERIES_HIGH		32000
#define MAX_CUR_EXEC_QUERIES_DEFAULT	10

#define MAX_CPU_BUSY_LOW		0
#define MAX_CPU_BUSY_HIGH		100
#define MAX_CPU_BUSY_DEFAULT	100
#define EST_MAX_CPU_BUSY		101
#define STATS_INTERVAL_LOW		5
#define STATS_INTERVAL_HIGH		300
#define STATS_INTERVAL_DEFAULT	5
#define EYE_CATCHER				0x0F0F0F0F
#define EXEC_TIMEOUT_LOW		0
#define EXEC_TIMEOUT_HIGH		1440
#define EXEC_TIMEOUT_DEFAULT	0
#define WAIT_TIMEOUT_LOW		0
#define WAIT_TIMEOUT_HIGH		1440
#define WAIT_TIMEOUT_DEFAULT	0
#define HOLD_TIMEOUT_LOW		0 //minutes
#define HOLD_TIMEOUT_HIGH		1440  
#define HOLD_TIMEOUT_DEFAULT	0
#define MAX_ROWS_FETCHED_LOW    0L
#define MAX_ROWS_FETCHED_HIGH   9223372036854775807L
#define MAX_ROWS_FETCHED_DEFAULT 0L
#define MAX_NUM_JOINS_LOW       0
#define MAX_NUM_JOINS_HIGH      10000000000000
#define MAX_NUM_JOINS_DEFAULT   0
#define MAX_RULE_NAME_LEN		24*4
#define RULE_INTERVAL_LOW		60
#define RULE_INTERVAL_HIGH		3600
#define RULE_INTERVAL_EXEC_TIME_LOW			1
#define RULE_INTERVAL_EXEC_TIME_HIGH		30
#define RULE_INTERVAL_EXEC_TIME_DEFAULT	5 //seconds

#define RULE_INTERVAL_DEFAULT	60 //seconds
#define RULE_PERCENTAGE_LOW		100
#define RULE_PERCENTAGE_HIGH	1000
#define RULE_NUMERIC_VAL_LOW	0L
#define RULE_NUMERIC_VAL_HIGH   9223372036854775807L
#define STATS_IDLE_MINUTES_VAL_LOW 1L
#define STATS_IDLE_MINUTES_VAL_HIGH 10L
#define MAX_ROPND_STR_LEN		128
#define MAX_SESSION_STR_LEN		24
//#define MAX_LOGIN_STR_LEN		17
#define MAX_LOGIN_STR_LEN		128
#define MAX_APPL_STR_LEN		30
#define MAX_DSN_STR_LEN			32
#define OFFENDER_SAMPLE_INTERVAL_LOW		10 //seconds
#define OFFENDER_SAMPLE_INTERVAL_HIGH		60 
#define OFFENDER_SAMPLE_INTERVAL_DEFAULT	10
#define OFFENDER_SAMPLE_CPUS_LOW			2
#define OFFENDER_SAMPLE_CPUS_HIGH			16
#define OFFENDER_SAMPLE_CPUS_DEFAULT		2
#define OFFENDER_SAMPLE_CACHE_LOW			10
#define OFFENDER_SAMPLE_CACHE_HIGH			100
#define OFFENDER_SAMPLE_CACHE_DEFAULT		10

#define OFFENDER_TXN_INTERVAL_LOW			1	// minutes
#define OFFENDER_TXN_INTERVAL_HIGH			10 
#define OFFENDER_TXN_INTERVAL_DEFAULT		1
#define OFFENDER_TXN_DURATION_LOW			1	// minutes
#define OFFENDER_TXN_DURATION_HIGH			10
#define OFFENDER_TXN_DURATION_DEFAULT		1
#define OFFENDER_TXN_CACHE_LOW				10
#define OFFENDER_TXN_CACHE_HIGH				100
#define OFFENDER_TXN_CACHE_DEFAULT			10

#define OFFENDER_WMS_HEALTH_INTERVAL_LOW	 1	//minutes
#define OFFENDER_WMS_HEALTH_INTERVAL_DEFAULT 5
#define OFFENDER_WMS_HEALTH_INTERVAL_HIGH	 30

#define CANARY_INTERVAL_LOW                      5
#define CANARY_INTERVAL_HIGH                     60
#define CANARY_INTERVAL_DEFAULT                  5      //minutes
#define CANARY_EXEC_TIME_LOW                     0
#define CANARY_EXEC_TIME_HIGH                    60
#define CANARY_EXEC_TIME_DEFAULT                 0      //seconds
#define CANARY_QUERY_TEXT_DEFAULT       "SELECT ROW COUNT FROM MANAGEABILITY.NWMS_SCHEMA.WMS_CANARY"
#define MAX_TXN_ROLL_MIN_DEFAULT                 0 
#define MAX_TXN_ROLL_MIN_LOW                     3 
#define MAX_TXN_ROLL_MIN_HIGH                    300 
#define TXN_ROLLBACK_REJECT				 1 
#define TXN_ROLLBACK_WARN				 3

//Aggregation
#define REPOS_INTERVAL_LOW		1 //minutes
#define REPOS_INTERVAL_HIGH		5
#define REPOS_INTERVAL_DEFAULT	1
#define WMS_INTERVAL_LOW		1 //minutes
#define WMS_INTERVAL_HIGH		5
#define WMS_INTERVAL_DEFAULT	1
#define EXEC_INTERVAL_LOW		1 //minutes
#define EXEC_INTERVAL_HIGH		5
#define EXEC_INTERVAL_DEFAULT	1
#define STATS_ONCE_OFF			0 // perf
#define STATS_ONCE_ON			1
#define STATS_ONCE_DEFAULT		0 // 0=OFF, 1=ON
#define MAX_AVG_ESPS_LOW		1
#define MAX_AVG_ESPS_HIGH		4000
#define MAX_AVG_ESPS_DEFAULT	0

// Note: Any changes made to the service priority types below SHOULD
// have a corresponding define below for a master executor priority value.
typedef enum _SERVICE_PRIORITY
{
	PRIORITY_URGENT = 1,
	PRIORITY_HIGH,
	PRIORITY_MEDIUM_HIGH,
	PRIORITY_MEDIUM,
	PRIORITY_LOW_MEDIUM,
	PRIORITY_LOW,
} SERVICE_PRIORITY;

#define URGENT_MASTER_DEFAULT		181
#define HIGH_MASTER_DEFAULT			140
#define MEDIUM_HIGH_MASTER_DEFAULT	125
#define MEDIUM_MASTER_DEFAULT		110
#define LOW_MEDIUM_MASTER_DEFAULT	95
#define LOW_MASTER_DEFAULT			80

#define SYNCREADUPDATE		100
#define SYNCREGISTER		200
#define SYNCADAPTIVESEGMENT	300
#define SYNCTRACE			400
#define SYNCSERVICECHECK	500
#define SYNCWMSCHECK	 	600
#define SYNCREGPROCINFO		700
#define SYNCGETWMSINFO		800

#define SYNC_EXIST			1
#define SYNC_IS_STATE		2
#define SYNC_IS_NOT_STATE	3
#define SYNC_SERVICE_DATA	4
#define SYNC_WMS_DATA		5
#define SYNC_SERVICE_STATE_AND_DATA 6

#define ALLOCATE_SEGEMENT	1
#define ALLOCATE_SEGMENTN	2
#define DEALLOCATE_SEGMENT	3

#define MGR_OPEN_SESSION_TIMEOUT	60 * 5
#define MGR_TIMEOUT					60 * 5

#define COM_TIMEOUT			60 * 5
#define STATS_TIMEOUT		60 * 5
#define RULE_TIMEOUT		60 * 5
#define OFFND_TIMEOUT		60 * 5

#define SYNC_PROCESSUP_TIMEOUT		60 * 5
#define SYNC_OPEN_SESSION_TIMEOUT	60 * 5
#define SYNC_TIMEOUT				60 * 5

#define SPI_TIMEOUT			60 * 5

#define MAX_NUMBER_OF_ERRORS	2
#define MAX_VPROC_NAME_LEN		100

// Component privileges
#define MAX_BITMASK_SIZE 256
typedef __int64 mask_type;
#define BITMASK_ARR_SIZE MAX_BITMASK_SIZE/(sizeof(mask_type)*8)
typedef mask_type bitmask_type[BITMASK_ARR_SIZE];

typedef struct AS_request
{
	short request_code;
	short sub_code;
	TPT_DECL(handle);
	short segment;
} AS_request_def,*pAS_request_def;

typedef struct REGISTER_MSG_
{
	short operation;	
	short max_services;
	short max_queries;
	short rts;
	short sysStats;
	short offender;
	char  Schema[128];
	bool  bIsComPrimary;
	char  services_ip_address[MAX_IP_ADDRESS_LEN];
	int	  services_port;
//
// QSMGR on primary segment in nClusterSize returns to QSSYNC number of segments in the cluster.
// QSMGR on other segments return 0. 
// QSSYNC uses nClusterSize only in production environment (when program pathy is /G/system/zwlmgr)
// to monitor on which segments QSMGR is not running. If QSMGR is not running longer than 15 minutes
// QSSYNC tries to spawn new QSMGR using SPI calls to SCP
//
	short nClusterSize;
	short mySegmentnumber;

	short shared_memory_segment_id;

	VERSION_def Version;
	char  Vproc[MAX_VPROC_NAME_LEN];

} REGISTER_MSG,*pREGISTER_MSG;

typedef struct TRACE_MSG_
{
	short operation;	
	long  trace;
	char  FileName[20];
	char  FilePath[50];

} TRACE_MSG,*pTRACE_MSG;
//

//--------------SQLCancel Requester -----------------------
typedef enum _REQUESTER
{
	REQUESTER_INIT,
	REQUESTER_APPLICATION,
	REQUESTER_DBADMIN,
	REQUESTER_WMSADMIN,
	REQUESTER_HOLDTIMEOUT,
	REQUESTER_EXECTIMEOUT,
	REQUESTER_EXECRULE

} REQUESTER;

//
// This message is sent by QSCOM to QSSYNC. QSSYNC returns two types of information:
// 1. backout transaction mask
// 2. List of active segments
//
typedef struct GLOBAL_VALUE_MSG_
{
	short operation;
	short segment;					// from which segment the message was sent
//
	short restartWMS;
//
	unsigned long abortTxnMask;	// backout transaction mask
//
	unsigned char coms[256];		//list of active segments
//
} GLOBAL_VALUE_MSG,*pGLOBAL_VALUE_MSG;

typedef struct REG_PROC_INFO_
{
	char	qsname[MS_MON_MAX_PROCESS_NAME];
	char	comname[MS_MON_MAX_PROCESS_NAME];
	int		priority;
	bool	recover;
} REG_PROC_INFO;

typedef struct SYNC_REG_PROC_INFO_MSG_
{
	short operation;
	short segment;
	int64 proc_info_timestamp;
;
	short shared_memory_segment_id; //shared memory segment
	int	wmsNumber;
	unsigned char coms[MAX_CLUSTER_NODES];
	REG_PROC_INFO regProcInfo[MAX_CLUSTER_NODES];
	char configArray[MAX_CLUSTER_NODES];

} SYNC_REG_PROC_INFO_MSG,*pSYNC_REG_PROC_INFO_MSG;

typedef struct SYNC_WMS_INFO_
{
//input values
	short operation;	//must be SYNCGETWMSINFO
	short nid;			//QSSYNC will map this nid to wms nid
;
//returned values
	short retcode;		// if 4 (RCNODENOTWMS), wms has not registered yet
	char  services_port[10];
	char  services_ip_address[MAX_IP_ADDRESS_LEN];

}SYNC_WMS_INFO, *pSYNC_WMS_INFO;

//---------------- Message between COM or STATS or RULE and MGR ------------------

#define MSG_CATCHER 0x81818181

enum MSG_SRC
{
	MSG_INIT,
	MSG_COM,
	MSG_STATS,
	MSG_RULE,
	MSG_OFFND,
	MSG_CFG,
	MSG_QSMGR
};

enum QSMGR_OPER
{
	QSMGR_INIT,
	QSMGR_MSG,				// QS reads messages from shared memory
	QSMGR_NEXT_EXEC,		// QS tries to execute next query
	QSMGR_WAIT,				// caller waits for response from MXOSRVR
	QSMGR_CL_DISAPPEARED,	// QS processes DISAPPEARED lost messages
	QSMGR_FIND_QID,			// Request from QSOFFND to find QID for process (Offender)
	QSMGR_FOR_CPU,			// Request from QSCOM to QSOFFND to select offending processes from CPU
	QSMGR_CANCEL_QUERY,		// Request to cancel the query (from CFG)
	QSMGR_CANCEL_CHILDREN,	// Request to cancel children for this parent
	QSMGR_GET_CHILDREN,
	QSMGR_GET_PROCESS,
	QSMGR_GET_PSTACK,
	QSMGR_CANARY_EXEC,
	QSMGR_CHILD_UPDATE,
};

typedef struct MESSAGE_DATA_
{
	long		catcher;
	MSG_SRC		source;
	QSMGR_OPER	operation;
	short		error;
	short		error_detail;
//
// to cancel the query from configuration server
//
	short		segment;
	short		cpu;
	short		pin;
	long		srvrApi;
	REQUESTER	requesterType;
	char		requesterApplication[129];
	char		requesterComputer[129];
	char		requesterName[129];
    char		cancelQueryComment[MAX_COMMENT_LEN/4];

} MESSAGE_DATA, *pMESSAGE_DATA;

typedef struct MESSAGE_DATA_WAIT_
{
	MESSAGE_DATA md;

	short		tag;
	short		opId;
	char		queryId[MAX_QUERY_ID_LEN];	// query id
	short		queueId;					// queue Id
	short		queueIndex;				// queue index
	char		queueName[MAX_SERVICE_NAME_LEN + 1];	// queue name
	unsigned short sqlLength;
	union {
		unsigned char  sqlCmd[MAX_SQL_CMD_LEN + 1];	// sql cmd
		unsigned char  sqlError[MAX_SQL_CMD_LEN + 1];	// sql error
	} sql;

} MESSAGE_DATA_WAIT, *pMESSAGE_DATA_WAIT;

typedef struct MESSAGE_CANCELL_CHILDREN_
{
	MESSAGE_DATA md;
	long	last_nid;
	long 	completed_state;
	char	parent_qid[MAX_QUERY_ID_LEN];	// parent query id

}MESSAGE_CANCELL_CHILDREN, *pMESSAGE_CANCELL_CHILDREN;

typedef struct MESSAGE_QUERY_INFO_
{
	MESSAGE_DATA md;

	long seg;
	short cpu;
	short pin;
	short query_relation; //any = 0; child = 1; parent = 2
//
//====================================================================
//
	char				super_parent_query_id[MAX_QUERY_ID_LEN];// first grand parent when query has many child
	char				parent_query_id[MAX_QUERY_ID_LEN];	//
	char				query_id[MAX_QUERY_ID_LEN];			// if any
	char				tcpStatus[10];
	unsigned short		port;
	char				tcpProcessName[MAX_TCP_PROCESS_NAME];
	long				ExecutionElapsedTime;				// elapsed time in seconds
	long				wait_time;							// wait time in seconds
	long				hold_time;							// hold time in seconds
	char				ds_name[MAX_DSOURCE_NAME + 1];		// Datasource Name
	char				service_name[MAX_SERVICE_NAME_LEN + 1];	//	Queue Name
	char				query_name[MAX_QUERYNAME_LEN + 1];	// Query Name
	char				user_name[MAX_NAME_LEN + 1];		// Role Name
	char				user[MAX_NAME_LEN];					// User alias name
	char				dbuser_name[MAX_USERNAME_LEN + 1];		// db User Name
	char				sql_text[RMS_STORE_SQL_SOURCE_LEN + 1];		// Only 254 bytes


} MESSAGE_QUERY_INFO, *pMESSAGE_QUERY_INFO;

typedef struct MESSAGE_FROM_CPU_
{
	MESSAGE_DATA md;

	short cpu;
	bool bAll;
	bool bCpuBusy;

} MESSAGE_FROM_CPU, *pMESSAGE_FROM_CPU;

typedef struct MESSAGE_CANARY_EXEC_
{
    MESSAGE_DATA md;
    int64    canary_exec_time;

}MESSAGE_CANARY_EXEC, *pMESSAGE_CANARY_EXEC;

typedef struct MESSAGE_CHILD_UPDATE_
{
	MESSAGE_DATA md;
	bool bChildStart;
	char parent_query_id[MAX_QUERY_ID_LEN];
	unsigned long   parent_internal_Id;
	short		parent_nid;
	char query_id[MAX_QUERY_ID_LEN];
	unsigned long	internal_Id;
	short		nid;
	
}MESSAGE_CHILD_UPDATE, *pMESSAGE_CHILD_UPDATE;

inline void SQ_EXIT(short retcode)
{
	file_mon_process_shutdown_now();
	exit(retcode);
}

extern char TfdsStrBuf1[], TfdsStrBuf2[], TfdsStrBuf3[], TfdsStrBuf4[], TfdsStrBuf5[];

inline void ERR_EXIT(short retcode)
{
	fprintf(stderr, "retcode=%d, buf1=%s, buf2=%s , buf3=%s , buf4=%s , buf5=%s", retcode, TfdsStrBuf1, TfdsStrBuf2, TfdsStrBuf3, TfdsStrBuf4, TfdsStrBuf5);
	SQ_EXIT(retcode);
}


typedef struct SQ_PROCESS_DATA_
{
	float pcpu;
	float pmem;
	int rss;
	int vsz;

	int nid;
	int pid;
	PROCESS_TYPE ptype;

} SQ_PROCESS_DATA, *pSQ_PROCESS_DATA;

typedef struct MESSAGE_GET_SQ_PROCESSES_
{
//
// input
//
	MESSAGE_DATA md;

	int nid;
	int pid;	
//
// output
//
// process details
	
	bool bno_node_in_cmd;
	char buffer[1000];
	PROCESS_TYPE ptype;

	char node_name[MS_MON_MAX_PROCESSOR_NAME];
	MS_Mon_Process_Info_Type procInfo;

// children

	int count;
	union
	{
		SQ_PROCESS_DATA proc[MAX_SQ_PROCESSES];
		char lines[MAX_SQ_PSTATCK_LINES][MAX_SQ_PSTACK_LINE_LENGTH];
	} u;

} MESSAGE_GET_SQ_PROCESSES, *pMESSAGE_GET_SQ_PROCESSES;
 
typedef struct UDF_MESSAGE_TAG_
{
	int session_id;
	long long message_number;
//
	short nid;
	short numcores;
	short used_numcores;
	bool  isWms;
	bool  isSql;
	bool  isShutdown;
//
	float avrg_cpu_busy;
	float memory_usage;
	unsigned long kb_main_total;
	long  esp_count;

	int	serviceWaitingQueries[TOTAL_SERVICES];	 	// current waiting queries for the service
	int	serviceExecutingQueries[TOTAL_SERVICES]; 			// current executing queries for the service
	int	serviceHoldQueries[TOTAL_SERVICES];		 			// current holding queries for the service
	int	serviceSuspendedQueries[TOTAL_SERVICES]; 			// current suspended queries for the service

} UDF_MESSAGE, *pUDF_MESSAGE;

#define MAXBUFSIZE 65536 // Max UDP Packet size is 64 Kbyte

#define MLTCAST_PORT 4096
#define MLTCAST_INET_ADDR "224.0.0.1"

#define DEFAULT_IS_ZOOKEEPER true

#endif
