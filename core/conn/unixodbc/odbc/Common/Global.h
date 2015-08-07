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
#ifndef _GLOBAL_DEFINED
#define _GLOBAL_DEFINED

#include <stdio.h>
#include <time.h>
#include "tip.h"
#include "cee.h"
#include "odbcCommon.h"
#include "odbcsrvrcommon.h"
#include "odbcMxSecurity.h"
#include "odbcmxProductName.h"
#include "EventMsgs.h"
#include "ceeCfg.h"
#ifdef NSK_PLATFORM
#include <limits.h>
#include "tfds\tfdrtl.h"
#define _MAX_PATH	256
#endif

class CDataSourceList ;
class ODBCMXTraceMsg;
//class ResStatisticsStatement;
//class ResStatisticsSession;


#define	MAX_IP_ADDRESS_LEN		128
#define	MAX_PROCESS_NAME_LEN	128
#define MAX_NODE_NAME			128
#define MAX_PORT_NUMBER			65535
#define MAX_DSOURCE_NAME		128
#define	MAX_TCP_PROCESS_NAME_LEN	20

#define EXT_FILENAME_LEN ZSYS_VAL_LEN_FILENAME

// AS defaults
#define DEFAULT_TCP_PROCESS			"$ZTC0"
#define DEFAULT_NUMBER_OF_SERVERS	2
#define DEFAULT_PORT_NUMBER			18650
#define DEFAULT_CFGSRVR_TIMEOUT		120
#define DEFAULT_MAJOR_VERSION		1
#define DEFAULT_PORT_RANGE			250
#define DEFAULT_AS_POLLING			10
#define DEFAULT_EMS					"$0"
#define DEFAULT_EMS_TIMEOUT			100 // 1 sec
#define DEFAULT_INIT_SRVR			5
#define DEFAULT_INIT_TIME			5
#define DEFAULT_DSG					false // delegate super group: false - only super.super, true - any user
#define DEFAULT_MAX_CFG_START		5 // AS will try to spawn CFG before stops
#define DEFAULT_CONNECTING_POLLING	20 //seconds
#define DEFAULT_CONNECTING_TOTAL_TIME 600 //seconds

#define CONSOLE_WINDOW			1
#define SRVR_DEBUG_BREAK		2
#define AS_SRVR_DEBUG_BREAK		4
#define CFG_SRVR_DEBUG_BREAK	8
#define CA_SRVR_DEBUG_BREAK		16
#define AS_TRANSPORT_TRACE		0x100 // 256
#define CFG_TRANSPORT_TRACE		0x200 // 512
#define SRVR_TRANSPORT_TRACE	0x400 // 1024

#define EVENT_INFO_VERBOSE		2

#define PUBLIC_DSID				0
#define NO_DEFAULT_DSID			-1

#define PUBLIC_DSNAME			"PUBLIC"

#define NO_MAX_SRVR_LIMIT		0
#define DEFAULT_SRVR_IDLE_TIMEOUT_MINS	10
#define DEFAULT_CONN_IDLE_TIMEOUT_MINS	10
#define DEFAULT_REFRESH_RATE_SECS		60
#define DEFAULT_SRVR_IDLE_TIMEOUT		0
#define DEFAULT_CONN_IDLE_TIMEOUT		0
#define INFINITE_SRVR_IDLE_TIMEOUT		-1
#define INFINITE_CONN_IDLE_TIMEOUT		-1
#define STATE_TRANSITION_TIMEOUT_SECS	300

#define ODBCMX_RETRY_DELAY_BASE_MS		600
#define ODBCMX_MAX_RETRY_STARTS			100
#define ODBCMX_MAX_RETRY_STOPS			10

#define DS_NOT_FOUND				-1
#define DS_ALREADY_STOPPED			-2
#define DS_ALREADY_STARTED			-3
#define DS_STATE_CHANGE_ERROR		-4
#define PROCESS_STOP_ERROR			-5
#define PORT_NOT_AVAILABLE			-6
#define SRVR_TYPE_UNKNOWN			-7
#define SRVR_STATE_CHANGE_ERROR		-8
#define ADD_SRVR_ENTRY_ERROR		-9
#define SRVR_CREATION_ERROR			-10
#define PROGRAM_ERROR				-11
#define NO_SRVR_AVAILABLE			-12
#define AS_NOT_AVAILABLE			-13
#define DS_NOT_AVAILABLE_ERROR		-14
#define GET_OBJREF_TRY_AGAIN		-15
#define	SRVR_NOT_FOUND				-16
#define SRVR_IN_USE					-17
#define DS_STOPPING_ERROR			-18
#define DS_STOPPED_ERROR			-19
#define AS_STOPPING					-20
#define AS_STOPPED					-21
#define SAVE_DSSTATUS_ERROR			-22
#define SAVE_ASSTATUS_ERROR			-23
#define	TRACE_ENABLE_ERROR			-24
#define TRACE_DISABLE_ERROR			-25
#define TRACE_ALREADY_ENABLED		-26
#define TRACE_ALREADY_DISABLED		-27
#define	STATISTICS_ENABLE_ERROR		-28
#define STATISTICS_DISABLE_ERROR	-29
#define STATISTICS_ALREADY_ENABLED	-30
#define STATISTICS_ALREADY_DISABLED	-31
#define CLIENT_STILL_CONNECTED		-32
#define UPDATE_CONTEXT_ERROR		-33
#define CPU_NOT_AVAILABLE			-34


#define ADDED						1
#define UPDATED						2

#define LOG							0
#define LOG_WITH_INFO				1

//for environment and variable types
#define ENV_SET						0
#define ENV_CONTROL					1
#define ENV_DEFINE					2
#define ENV_STATISTICS				3 // statement resource statistics
#define ENV_CPULIST					4 // List of CPUs for process spawn
#define ENV_PROCESSPRIORITY			5 // Priority for the Process to start.
#define ENV_PRIVILEGES				6 // User privileges on Metadata tables.

#define ENV_RESOURCE				-1
#define RES_ESTIMATEDCOST			0

#define	SQL_ATTR_ROWSET_RECOVERY	2000

//session statistics values range from 0x00000001 thru 0x00000128
#define SESSTAT_LOGINFO				1
#define SESSTAT_SUMMARY				2

//statement statistics values ranges from 0x00000256 thru 0x00032768
#define STMTSTAT_SQL				256
#define STMTSTAT_PREPARE			512
#define STMTSTAT_EXECUTE			1024
#define STMTSTAT_EXECDIRECT			2048
#define STMTSTAT_FETCH				4096
#define STMTSTAT_CLOSE				8192

// Set values ranges from 0x00000001 thru 0x000.......
//Add this defines in driver code in drvrglobal.h
#define MXO_ODBC_35					1
#define MXO_MSACCESS_1997			2
#define MXO_MSACCESS_2000			4
#define MXO_BIGINT_NUMERIC			8
#define MXO_ROWSET_ERROR_RECOVERY	16
#define MXO_METADATA_ID				32
#define MXO_FRACTION_IN_MICROSECS	64
#define MXO_FRACTION_IN_NANOSECS	128
#define MXO_PASSWORD_EXPIRY			256

//end of add define in drvrglobal.h

#define ATTR_TYPE1			"SQL_ATTR_ACCESS_MODE"
#define ATTR_TYPE1_VALUE1	"SQL_MODE_READ_WRITE"
#define ATTR_TYPE1_VALUE2	"SQL_MODE_READ_ONLY"

#define ATTR_TYPE2			"SQL_ATTR_TXN_ISOLATION"
#define ATTR_TYPE2_VALUE1	"SQL_TXN_READ_UNCOMMITTED"
#define ATTR_TYPE2_VALUE2	"SQL_TXN_READ_COMMITTED"
#define ATTR_TYPE2_VALUE3	"SQL_TXN_REPEATABLE_READ"
#define ATTR_TYPE2_VALUE4	"SQL_TXN_SERIALIZABLE"

#define ATTR_TYPE3			"SQL_ATTR_MSACCESS_VERSION"
#define ATTR_TYPE3_VALUE1	"1997"
#define ATTR_TYPE3_VALUE2	"2000"

#define ATTR_TYPE4			"SQL_ATTR_MAP_DATATYPE"
#define ATTR_TYPE4_VALUE1	"BIGINT_TO_NUMERIC"

#define ATTR_TYPE5			"SQL_ATTR_ROWSET_ERROR_RECOVERY"
#define ATTR_TYPE5_VALUE1	"TRUE"

#define ATTR_TYPE6			"SQL_ATTR_METADATA_ID"
#define ATTR_TYPE6_VALUE1	"SQL_TRUE"

#define ATTR_TYPE7			"SQL_ATTR_WARNING"
#define ATTR_TYPE7_VALUE1	"0"	// To supress the warning information during connection
#define ATTR_TYPE7_VALUE2	"1"	// To populate the warning information during connection		

#define ATTR_TYPE8			"SQL_ATTR_FRACTION_FIELD"
#define ATTR_TYPE8_VALUE1	"6"
#define ATTR_TYPE8_VALUE2	"9"

// -- for changing Data Source
#define DSNOP_CHANGED				0
#define DSCFG_CHANGED				1
#define DSRES_CHANGED				2 // for resource governing.
#define DSENV_CHANGED				3 // for set, control & defines
#define DSDEL_CHANGED				4
#define DSADD_CHANGED				5
#define DSSTA_CHANGED				6 // for resource statistics
#define DSDEF_CHANGED				7 // for defines

// General Param
// PARAM_TOKEN
#define UNKNOWN_TOKEN		0
#define CONFIG_PARAM		1
#define RESOURCE_PARAM		2
#define DEFINE_PARAM		3
#define CONTROL_PARAM		4
#define SET_PARAM			5

// PARAM_OPERATION
#define UNKNOWN_OPERATION	0
#define PARAM_ADDED			1
#define PARAM_CHANGED		2
#define PARAM_DELETED		3


// ODBC_SERVER_ERROR should be different than the possible values of SQLRETURN as defined in SQLTypes.h
// Else there may be overlap between SQLRETRUN values and this value
// Same is the case with ODBC_RG_ERROR and ODBC_RG_WARNING
#define ODBC_SERVER_ERROR			-101
#define ODBC_RG_ERROR				-102
#define ODBC_RG_WARNING				-103
#define SQL_RETRY_COMPILE_AGAIN		-104
#define	SQL_QUERY_CANCELLED			-105
#define CANCEL_NOT_POSSIBLE			-106
#define ROWSET_SQL_ERROR			-107

#define ANONYMOUS_USER "ANONYMOUS"

// will eventuall replace PRIMARY_PORT	"$ODBCPR", BACKUP_PORT
// "$ODBCBK" and SHUTDOWN_PORT "$ODBCSD"
#define ODBCMX_PORT_AS			"$OXA0000"
#define ODBCMX_PORT_AS_START	"$OXASTRT"
#define ODBCMX_PORT_AS_STOP		"$OXASTOP"

#define ODBCMX_ERR_MASK				0x2FFFFFFF

#define MAX_NSKCATALOGNAME_LEN	25

#define SESSION_ID_LEN		75

#define ODBCMX_DEFAULT_CATALOG	"NONSTOP_SYSTEM_NSK"
#define ODBCMX_DEFAULT_SCHEMA	"PUBLIC_ACCESS_SCHEMA"

typedef enum _EVENT_INFO_LEVEL
{
	EVENT_INFO_LEVEL1 = 1,
	EVENT_INFO_LEVEL2
} EVENT_INFO_LEVEL;


typedef enum _ERROR_COMPONENT
{
	 DRIVER_ERROR =	0,
	 SERVER_ERROR,
	 NETWORK_ERROR,
	 SQLMX_ERROR,
	 ASSOC_SERVER_ERROR,
	 CONFIG_SERVER_ERROR
} ERROR_COMPONENT;

typedef enum _START_TYPE
{
	NON_INTERACTIVE = 0, // StartAS is issued automatically, when CFG Server registers
	INTERACTIVE,		 // Starting from Launch Program /MMC - StartAS is issued from CfgCl  
	FAILOVER			 // FailOver Starting by CA - StartAS issued automatically, but DS and AS
						 // started as per the saved status settings.
} START_TYPE;

typedef enum _STOP_TYPE
{
	STOP_UNKNOWN = -1,
	STOP_ABRUPT,
	STOP_WHEN_DISCONNECTED,
	STOP_SRVR
} STOP_TYPE;

// This enum is also used in VIEWS and CFG Client/Server
// If you add an element in it, Pl. make necessary
// Changes in VIEWS & CFG Client/Server. 
typedef enum _SRVR_STATE
{
	SRVR_UNINITIALIZED = 0,
	SRVR_STARTING,
	SRVR_STARTED,
	SRVR_AVAILABLE,
	SRVR_CONNECTING,
	SRVR_CONNECTED,
	SRVR_DISCONNECTING,
	SRVR_DISCONNECTED,
	SRVR_STOPPING,
	SRVR_STOPPED,
	SRVR_ABENDED,
	SRVR_CONNECT_REJECTED,
	SRVR_CONNECT_FAILED,
	SRVR_CLIENT_DISAPPEARED,
	SRVR_ABENDED_WHEN_CONNECTED, // This combination state is used to update stat of DS
	SRVR_STATE_FAULT,
	SRVR_STOP_WHEN_DISCONNECTED
} SRVR_STATE;

typedef enum _SRVR_TYPE
{
	SRVR_UNKNOWN = 0,
	AS_SRVR,
	CORE_SRVR,
	CFG_SRVR
} SRVR_TYPE;

// This enum is also used in VIEWS and CFG Client/Server
// If you add an element in it, Pl. make necessary
// Changes in VIEWS & CFG Client/Server. 
typedef enum _DS_STATE
{
	DS_NOT_AVAILABLE = 0,
	DS_STARTING,
	DS_STARTED,
	DS_STOPPING,
	DS_STOPPED,
	DS_UNINITIALIZED   // This is equivalent to DS_NOT_FOUND
} DS_STATE;

typedef enum _DS_MODE 
{
	DS_UNKNOWN = 0,
	DS_INIT,
	DS_AVAILABLE
} DS_MODE;

typedef enum _DS_TRACE_STATE 
{
	DS_TRACE_DISABLED = 0,
	DS_TRACE_ENABLED
} DS_TRACE_STATE;

typedef enum _SRVR_TRACE_STATE 
{
	SRVR_TRACE_DISABLED = 0,
	SRVR_TRACE_DISABLING,
	SRVR_TRACE_ENABLING,
	SRVR_TRACE_ENABLED
} SRVR_TRACE_STATE;

typedef enum _SRVR_STATISTICS_STATE 
{
	SRVR_STATISTICS_DISABLED = 1,
	SRVR_STATISTICS_DISABLING,
	SRVR_STATISTICS_ENABLING,
	SRVR_STATISTICS_ENABLED
} SRVR_STATISTICS_STATE;

typedef enum _LIFE_PERMIT
{
	DIE = 0,
	LIVE,
	DIE_ZOMBIE
} LIFE_PERMIT;

typedef struct _SRVR_INIT_PARAM_Def
{
	long				debugFlag;
	long				eventFlag;
	IDL_OBJECT_def		asSrvrObjRef;
	SRVR_TYPE			srvrType;
	START_TYPE			startType;
	long				noOfServers;
	long				cfgSrvrTimeout;
	IDL_long			DSId;
	CEECFG_Transport	transport;
	long				portNumber;
	long				IpPortRange;
#ifdef NSK_PLATFORM
	char				TcpProcessName[MAX_TCP_PROCESS_NAME_LEN];
	char				ASProcessName[MAX_PROCESS_NAME_LEN];
	char				EmsName[EXT_FILENAME_LEN];
	int					EmsTimeout;
	char				TraceCollector[EXT_FILENAME_LEN];
	char				RSCollector[EXT_FILENAME_LEN];
	long				initIncSrvr;
	long				initIncTime;
	bool				DSG;
	bool				srvrTrace;
#else
	char				IpAddress[MAX_IP_ADDRESS_LEN];
#endif
	short				majorVersion;
	ODBCMXEventMsg*		eventLogger;
	char				DSName[MAX_DSOURCE_NAME + 1];
} SRVR_INIT_PARAM_Def;

typedef struct _SRVR_MONITOR_CONTEXT {
	CEE_handle_def	call_id;
	CEE_tag_def		objtag_;
	SRVR_TYPE		srvrType;
	IDL_OBJECT_def	srvrObjRef;
	IDL_long		DSId;
	CEE_status		sts;
} SRVR_MONITOR_CONTEXT;

typedef struct SRVR_GETOBJREF_CONTEXT {
	CEE_handle_def	call_id;
	CEE_tag_def		objtag_;
} SRVR_GETOBJREF_CONTEXT;

typedef struct _SRVR_GLOBAL_Def 
{
	_SRVR_GLOBAL_Def(){ 
		bRowsetSupported = FALSE;
		bAutoCommitOn = FALSE;
		bAutoCommitSet = FALSE;
	}

	long				debugFlag;
	long				eventFlag;
	IDL_OBJECT_def		asSrvrObjRef;
	SRVR_TYPE			srvrType;
	IDL_long			DSId;
	PROCESS_ID_def		nskProcessInfo;
	SRVR_STATE			srvrState;
	DIALOGUE_ID_def		dialogueId;
	IDL_OBJECT_def		srvrObjRef;
	CEE_handle_def		ASSvc_proxy;
	CEE_handle_def		ASSvc_MonitorProxy;
	CEE_handle_def		ASSvc_ifch;
	SRVR_CONTEXT_def	srvrContext;
	CEE_handle_def		connIdleTimerHandle;
	CEE_handle_def		srvrIdleTimerHandle;
//	BOOL				validTimerHandle;
#ifdef NSK_PLATFORM
	char				ASProcessName[MAX_PROCESS_NAME_LEN];
	PROCESS_ID_def		nskASProcessInfo;
	CEE_handle_def		ASTimerHandle;
	unsigned short		CreatorAccessId;
	unsigned short		ProcessAccessId;
	bool				DSG;
	TFDS_RegInfo		regInfo;
	ODBCMXTraceMsg			*traceLogger;
//	ResStatisticsSession    *resStatSession;
//	ResStatisticsStatement  *resStatStatement;
	char				sessionId[SESSION_ID_LEN];
	char				TraceCollector[EXT_FILENAME_LEN];
	char				RSCollector[EXT_FILENAME_LEN];
#endif
	BOOL				bAutoCommitOn;
	BOOL				bAutoCommitSet;

	BOOL				bRowsetSupported;

	BOOL				resGovernOn;
	BOOL				envVariableOn;
	char				userSID[MAX_TEXT_SID_LEN+1];// This points to the SID in which the server is running
													// If this is same as the incoming SID do not flush the
													//  cache
	tip_handle_t		tip_gateway;
	char				*pxid_url;
	IDL_long_long		local_xid;
	UINT				xid_length;
		
	STOP_TYPE			stopTypeFlag;
	LPCRITICAL_SECTION	CSObject;
	BOOL				failOverEnabled;
	long				clientACP;
	long				clientErrorLCID;
	long				clientLCID;
	BOOL				useCtrlInferNCHAR;
	VERSION_def			srvrVersion;
	VERSION_def			drvrVersion;
	VERSION_def			appVersion;
	char				NskSystemCatalogName[MAX_NSKCATALOGNAME_LEN+1]; //MP system catalog name
	char				DefaultCatalog[129];
	char				DefaultSchema[129];
	short				EnvironmentType; // Since from 2.0 SQL/MX we don't need SHORTANSI,
										// ApplicationType is merged into this variable.
	char				NskSystemCatalogsTableName[EXT_FILENAME_LEN + 1];
	char				DSName[MAX_DSOURCE_NAME + 1];
	char				SystemCatalog[129]; // Real MX system catalog name
	short				tfileNum; // TFILE number in multi context env.
	long				schemaVersion;
	long				resourceStatistics;
} SRVR_GLOBAL_Def ;

#define srvrStarted		0x00000001
#define srvrRegistered	0x00000002
#define srvrConnected	0x00000004
#define srvrConnecting	0x00000008

typedef struct _SRVR_LIST_Def
{
	long				portNumber;
	long				DSId;
	IDL_OBJECT_def		srvrObjRef;
	SRVR_TYPE			srvrType;
	SRVR_STATE			srvrState;
	PROCESS_ID_def		nskProcessInfo;
	char				processName[MAX_PROCESS_NAME_LEN];
	char				nodeName[MAX_NODE_NAME];
	char				computerName[MAX_COMPUTERNAME_LENGTH+1];
	char				userSID[MAX_TEXT_SID_LEN+1];
	DWORD				processId;
	char				*userName;
	char				*windowText;
	DIALOGUE_ID_def		dialogueId;
	time_t				lastUpdatedTime;
	_SRVR_MONITOR_CONTEXT *srvrMonitorContext;
	CEE_handle_def		srvrProxy;
	VERSION_def			srvrVersion;
	SRVR_GETOBJREF_CONTEXT *srvrGetObjRefContext;
	unsigned long		srvrCountMask;
	SRVR_TRACE_STATE	traceState;

	int					statisticsWaited;
	int					traceWaited;
	bool				bSrvrContextWaited;
	int					connectingTotalTime;
} SRVR_LIST_Def;
	
typedef struct _AS_SRVR_GLOBAL_Def 
{
	long			debugFlag;
	long			eventFlag;
	long			noOfServers; 
	long			CurNodeNumForSrvr;
	long			cfgSrvrTimeout;
	START_TYPE		startType;
	SRVR_TYPE		srvrType;
	IDL_OBJECT_def	asSrvrObjRef;
	long			transport;	
	long			portNumber;
	long			IpPortRange;
#ifdef NSK_PLATFORM
	char			TcpProcessName[MAX_TCP_PROCESS_NAME_LEN];
	char			EmsName[EXT_FILENAME_LEN];
	char			TraceCollector[EXT_FILENAME_LEN];
	char			RSCollector[EXT_FILENAME_LEN];
	unsigned short	CreatorAccessId;
	unsigned short	ProcessAccessId;
	long			initIncSrvr;
	long			initIncTime;
	bool			DSG;
	short			countCfgStart;
	TFDS_RegInfo	regInfo;
#else
	char			IpAddress[MAX_IP_ADDRESS_LEN];
#endif
	DWORD			processId;
	PROCESS_ID_def	nskProcessInfo;
	SRVR_STATE		srvrState;
	BOOL			cfgSrvrNeeded;
	CEE_handle_def	cfgSvcIntf;
	IDL_OBJECT_def  cfgSrvrObjRef;
	CEE_handle_def	cfgTimerHandle;
	BOOL			cfgTimerExpired;
	SRVR_STATE		cfgSrvrState;
	IDL_long		defaultDSId;
	CEE_handle_def	coreServerIntf;
	char*			asStopReason;
	SRVR_LIST_Def	*srvrList;
	CDataSourceList *pDSList;
	short			caTimerExpired;
	CEE_handle_def	caTimerHandle;
	long			endPortRange;
	time_t			srvrStateChangeTime;
	SRVR_STATE		savedStatus;
	time_t			savedStatusChangeTime;
	VERSION_def		odbcVersion;
	char			ASProgramName[_MAX_PATH+1];	// Contains either the fully qualified Guardian pathname or NT name
	char			ASProcessName[MAX_PROCESS_NAME_LEN];
#ifdef NSK_PLATFORM
	char			cfgProcessName[MAX_PROCESS_NAME_LEN];
	char			cfgNodeName[MAX_NODE_NAME];
	// OSS absolute path names can be up to 1023 characters, however, the servers will be in the same location as 
	// the AS (which is in a Guardian space) with a maximum of 20 characters i.e. /G/DATA001/x1234567, 
	// so we'll use the NT default (in case it applies to NT also)
	char			AS_OSSPathname[_MAX_PATH+1];
#endif
	CEE_handle_def	ConnectingTimerHandle;
} AS_SRVR_GLOBAL_Def;

// Parameter errors for error messages (from the server)
#define SRVR_ERROR_MSGBOX_TITLE	"ODBC/MX Server Wrapper Error"
#define REG_REJECT			"Registration Rejected By Association Server"
#define NO_SRVR_AVAILABLE_MSG "No Server Available to connect"
#define STATE_CHANGE_ERROR	"State Transition Error"
#define ODBC_STOP_MSG		"NonStop ODBC/MX Services Stopped"
#define INIT_ERROR_MSGBOX_TITLE "ODBC/MX Initialization Error"
#define INIT_SUCCESSFUL_MSG "ODBC/MX Initialization Successful"
#define INIT_ERROR_DATABASE_FOUND "ODBC/MX Database Found"
#define INIT_ERROR_INVALID_HANDLE "SQL Invalid Handle Error"
#define INIT_ERROR_DATABASE_NOTFOUND "ODBC/MX Database Not Found"
// #define PROGRAM_ERROR_MSG		"Programming Error" // replaced by detailed exceptions below
#define WOULD_LIKE_TO_LIVE_ERROR_MSG "Exception in WouldLikeToLive Call"
#define NULL_VALUE_ERROR		"Null Value in a non nullable column"
#define NULL_VALUE_ERROR_SQLCODE	-1
#define NULL_VALUE_ERROR_SQLSTATE   "23000"
#define INIT_WARNING_TITLE "ODBC/MX Initialization Warning Message"

// the following literals will be used to identify a "parameter" value for
// specific cases of ParamError exceptions.  A corresponding event message
// has to be logged if the exception is critical.

#define KRYPTON_EXCEPTION_PROXY_FAILURE	"Krypton: Proxy Failure"
#define KRYPTON_EXCEPTION_NO_MEM		"Krypton: No memory"
#define ASSVC_EXCEPTION_NO_DS			"Data Source Not Found"
#define ASSVC_EXCEPTION_NO_SRVR_REF		"No Object Reference"
#define ASSVC_EXCEPTION_NO_DSN_LIST		"No DSN list or no Global variable"
#define ASSVC_EXCEPTION_DSN_START		"DSN Error or Servers not started"
#define ASSVC_EXCEPTION_INVALID_LST_PROTOCOL	"Invalid list or protocol"
#define ASSVC_EXCEPTION_INVALID_SRVR_TYPE	"Invalid Server Type"
#define ASSVC_SRVR_IDLE_TIMEOUT_EXPIRED		"ODBC Server idle timeout expired"
#define ASSVC_EXCEPTION_ENABLE_TRACE	"Unable to Start Data Source or Server Trace"
#define ASSVC_EXCEPTION_DISABLE_TRACE	"Unable to Stop Data Source or Server Trace"
#define ASSVC_EXCEPTION_DSN_START_NO_AS "Unable to start Data Source since AS is not in available state"
#define ASSVC_EXCEPTION_NO_CPU_AVAILABLE "No CPUs or Invalid CPU list are set for MXCS server to start"
#define SQLSVC_EXCEPTION_SMD_STMT_LABEL_NOT_FOUND	"SMD STMT LABEL NOT FOUND"
#define SQLSVC_EXCEPTION_UNSUPPORTED_SMD_DATA_TYPE	"UNSUPPORTED INPUT SMD DATA TYPE"
#define SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED	"Wildcard characters in Catalog/Qualifier or Schema/Owner or Table name are not supported"
#define SQLSVC_EXCEPTION_NULL_SQL_STMT	"Null SQL Statement"
#define SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT	"Unable to allocate SQL Statement"
#define SQLSVC_EXCEPTION_INVALID_ROW_COUNT	"Invalid Row Count"
#define SQLSVC_EXCEPTION_INVALID_ROW_COUNT_AND_SELECT	"Invalid Row Count and Select Stmt"
#define SQLSVC_EXCEPTION_INVALID_RESOURCE_OPT_CLOSE	"Invalid Resource Option for Close"
#define SQLSVC_EXCEPTION_INVALID_TRANSACT_OPT	"Invalid TransactOpt"
#define SQLSVC_EXCEPTION_INVALID_OPTION_VALUE_NUM	"Invalid OptionValueNum"
#define SQLSVC_EXCEPTION_INVALID_OPTION_VALUE_STR	"Invalid OptionValueStr (too big)"
#define SQLSVC_EXCEPTION_INVALID_CONNECTION_OPTION	"Invalid ConnectionOpt"
#define SQLSVC_EXCEPTION_PREPARE_FAILED	"Prepare Failed"
#define SQLSVC_EXCEPTION_EXECUTE_FAILED "Execute Failed"
#define SQLSVC_EXCEPTION_EXECDIRECT_FAILED	"ExecDirect Failed"
#define SQLSVC_EXCEPTION_CLOSE_FAILED	"Close Failed"
#define SQLSVC_EXCEPTION_FETCH_FAILED	"Fetch Failed"
#define SQLSVC_EXCEPTION_CANCEL_FAILED	"Cancel Failed"
#define SQLSVC_EXCEPTION_ENDTRANSACTION_FAILED	"EndTransaction Failed"
#define SQLSVC_EXCEPTION_OPEN_TIP_GATEWAY_FAILED	"Open Tip Gateway Failed"
#define SQLSVC_EXCEPTION_PULL_TIP_FAILED	"Pull Tip Failed"
#define SQLSVC_EXCEPTION_PUTENVCLI_FAILED	"putenvCLI Failed"
#define SQLSVC_EXCEPTION_SETCONNECTOPTION_FAILED	"SetConnectionOption Failed"
#define SQLSVC_EXCEPTION_CATSMD_MODULE_ERROR "The Catalog SMD file is either corrupted or not found or else cursor not found"
#define SQLSVC_EXCEPTION_BUFFER_ALLOC_FAILED "Buffer Allocation Failed"
#define SQLSVC_EXCEPTION_DATA_ERROR	 "Input Parameter/Output Data Error"
#define SQLSVC_EXCEPTION_SHORTANSI_NOT_SUPPORTED "ShortAnsi Not Supported"
#define SQLSVC_EXCEPTION_INVALID_SCHEMA_VERSION "Invalid Schema version"
#define SQLSVC_EXCEPTION_INVALID_MODULE_VERSION "Incompatibility between MXCS server and NONSTOP_SQLMX_NSK.MXCS_SCHEMA.CATANSIMX module file."
#define SQLSVC_EXCEPTION_UNSUPPORTED_SMD_API_TYPE "UNSUPPORTED INPUT SMD API TYPE"
#define SQLSVC_EXCEPTION_PASSWORD_EXPIRED "Password Expired. Access denied"

// Literals used in the outContext between Driver and Server
#define	NT_ODBCAS_COMPONENT		1
#define NSK_ODBCAS_COMPONENT	2
#define	SQL_COMPONENT			3
#define ODBC_SRVR_COMPONENT		4
#define ODBC_CFGSRVR_COMPONENT	5
#define CFGCL_COMPONENT			6
#define	DRVR_COMPONENT			7
#define APP_COMPONENT			8
#define CLPS_SRVR_COMPONENT		9

#define NSK_ENDIAN				256
#define NT_ENDIAN				0

#define	NT_VERSION_MAJOR_1	1
#define	NT_VERSION_MINOR_0	0
#define NT_BUILD_1			1


#define	 SQLERRWARN 1
#define	 ESTIMATEDCOSTRGERRWARN	2

#define	NSK_VERSION_MAJOR_1	NT_VERSION_MAJOR_1+2
#define	NSK_VERSION_MINOR_0	0
#define NSK_BUILD_1			1

//-------------- TFDS codes ---------------------------------------

#define TFDS_EXCEPTION				1
#define	TFDS_NO_MEMORY				2
#define TFDS_WRONG_SERVER_STATE		3
#define TFDS_WRONG_OBJECT			4
#define TFDS_WRONG_DSID				5
#define TFDS_WRONG_DSNAME			6
#define TFDS_PROGRAM_ERROR			7

#endif
