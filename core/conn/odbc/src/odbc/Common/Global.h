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
#include <platform_ndcs.h>
#include <stdio.h>
#include <time.h>
#include "tip.h"
#include "cee.h"
#include "odbcCommon.h"
#include "odbcsrvrcommon.h"
#include "odbcMxSecurity.h"
#include "odbcmxProductName.h"
#include "ceecfg.h"
#include "PubQueryStats.h"

#define LOCALHOST "localhost"
#define DEFAULT_ZOOKEEPER_CLIENT_PORT 2181


#define _MAX_PATH	256

#ifdef __TIME_LOGGER
#include "TimeLogger.h"
#endif

#include <string>
#include <map>		// 64bit changes

#if defined(NSK_PLATFORM)
#include <hash_map>
#endif
#include <ext/hash_map>
namespace __gnu_cxx
{
template <>
struct hash<std::string> {
        size_t operator() (const std::string& x) const {
                return hash<const char*>()(x.c_str());
        // hash<const char*> already exists
        }
};
}

class CDataSourceList ;
class ODBCMXTraceMsg;


#define	MAX_IP_ADDRESS_LEN		128
#define MAX_HOST_NAME_LEN		128
#define MAX_INSTANCE_NAME_LEN	128
#define	MAX_PROCESS_NAME_LEN	128
#define MAX_NODE_NAME			128
#define MAX_PORT_NUMBER			65535
#define MAX_DSOURCE_NAME		512
#define	MAX_TCP_PROCESS_NAME_LEN	20
#define MAX_ROLENAME_LEN		128
#define MAX_USERNAME_LEN		128
#define MAX_SQL_IDENTIFIER_LEN  512
#define MAX_APPLICATIONID_LENGTH 128

#define EXT_FILENAME_LEN ZSYS_VAL_LEN_FILENAME

#define MAX_NAME_LEN				50
#include "QSGlobal.h"
#include "PubInterface.h"
#define DEFAULT_QS_PROCESS_NAME		"$ZWMGR"
#define DEFAULT_SYNC_PROCESS_NAME	"$ZWSYN"

#define DEFAULT_QS_POLLING			2
#define DEFAULT_STATS_POLLING		2
#define DEFAULT_COM_POLLING			10
#define DEFAULT_RULE_POLLING		2
#define DEFAULT_OFFND_POLLING		2
#define DEFAULT_SYNC_POLLING		10

// AS defaults
#define DEFAULT_TCP_PROCESS			"$ZTC0"
#define DEFAULT_NUMBER_OF_SERVERS	2
#define DEFAULT_PORT_NUMBER			18650
#define DEFAULT_CFGSRVR_TIMEOUT		120
#define DEFAULT_MAJOR_VERSION		1
#define DEFAULT_PORT_RANGE			250
#define DEFAULT_AS_POLLING			10

#define DEFAULT_EMS					"evlogd"

#define DEFAULT_EMS_TIMEOUT			100 // 1 sec
#define DEFAULT_INIT_SRVR			5
#define DEFAULT_INIT_TIME			5
#define DEFAULT_DSG				false // delegate super group: false - only super.super, true - any user
#define DEFAULT_MAX_CFG_START		5 // AS will try to spawn CFG before stops
#define DEFAULT_CONNECTING_POLLING	20 //seconds
#define DEFAULT_CONNECTING_TOTAL_TIME 3600 //seconds
#define DEFAULT_MINIMUM_SEGMENTS	8
#define DEFAULT_INIT_SEG_TIMEOUT	5 * 60 * 100 // 5 minutes
#define DEFAULT_CPUWEIGHT_POLLING	60 // seconds - for DBT rule based engine
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
#define DEFAULT_KEEPALIVE               0     //OPEN KEEPALIVE
#define DEFAULT_KEEPALIVE_TIMESEC       3600
#define DEFAULT_KEEPALIVE_COUNT         3
#define DEFAULT_KEEPALIVE_INTVL         20
#define INFINITE_SRVR_IDLE_TIMEOUT		-1
#define INFINITE_CONN_IDLE_TIMEOUT		-1
#define STATE_TRANSITION_TIMEOUT_SECS	300

#define CLIENT_KEEPALIVE_ATTR_TIMEOUT   3001

#define JDBC_ATTR_CONN_IDLE_TIMEOUT		3000
#define JDBC_DATASOURCE_CONN_IDLE_TIMEOUT -1L
#define JDBC_INFINITE_CONN_IDLE_TIMEOUT	0

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
#define INFOSTATS_SYNTAX_ERROR		-35
#define INFOSTATS_STMT_NOT_FOUND	-36
#define SRVR_NOT_CONNECTED			-37
#define QSYNC_NOT_FOUND				-38


#define ADDED						1
#define UPDATED						2

#define LOG							0
#define LOG_WITH_INFO				1

// For environment and variable types
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
#define SQL_ATTR_CLIPVARCHAR        2001

// session statistics values range from 0x00000001 thru 0x00000128
#define SESSTAT_LOGINFO				1
#define SESSTAT_SUMMARY				2

// statement statistics values ranges from 0x00000256 thru 0x00032768
#define STMTSTAT_NONE				0	// during construct and destruct time
#define STMTSTAT_SQL				256
#define STMTSTAT_PREPARE			512
#define STMTSTAT_EXECUTE			1024
#define STMTSTAT_EXECDIRECT			2048
#define STMTSTAT_FETCH				4096
#define STMTSTAT_CLOSE				8192

// Set values ranges from 0x00000001 thru 0x000.......
// Add this defines in driver code in drvrglobal.h
#define MXO_ODBC_35					1
#define MXO_MSACCESS_1997			2
#define MXO_MSACCESS_2000			4
#define MXO_BIGINT_NUMERIC			8
#define MXO_ROWSET_ERROR_RECOVERY	16
#define MXO_METADATA_ID				32
#define MXO_FRACTION_IN_MICROSECS	64
#define MXO_FRACTION_IN_NANOSECS	128
#define MXO_PASSWORD_EXPIRY			256  // no longer used, always return connection warning
#define MXO_SPECIAL_1_MODE			512
#define MXO_SQLTABLES_MV_TABLE		1024
#define MXO_SQLTABLES_MV_VIEW		2048


// end of add define in drvrglobal.h

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

#define ATTR_TYPE7			"SQL_ATTR_WARNING"  // no longer used, always return connection warning
#define ATTR_TYPE7_VALUE1	"0"	// To suppress the warning information during connection
#define ATTR_TYPE7_VALUE2	"1"	// To populate the warning information during connection

#define ATTR_TYPE8			"SQL_ATTR_FRACTION_FIELD"
#define ATTR_TYPE8_VALUE1	"6"
#define ATTR_TYPE8_VALUE2	"9"

// generic SET attributes
#define ATTR_TYPE9			"SQL_ATTR_GENERIC"
#define ATTR_TYPE9_VALUE1	"CLEANUP_CONNECTION"
#define ATTR_TYPE9_VALUE2	"CLEANUP_TIME"
#define ATTR_TYPE9_VALUE3	"EST_CARDINALITY"
#define ATTR_TYPE9_VALUE4	"EST_COST"

#define ATTR_TYPE10			"WMS_AS"
#define ATTR_TYPE10_VALUE1	"ON"
#define ATTR_TYPE10_VALUE2	"OFF"

#define ATTR_TYPE11			"EVENT_21036"
#define ATTR_TYPE11_VALUE1	"ON"
#define ATTR_TYPE11_VALUE2	"OFF"



#define ATTR_TYPE14			"SQLTABLES_MV_TYPE"
#define ATTR_TYPE14_VALUE1	"TABLE"
#define ATTR_TYPE14_VALUE2	"VIEW"

#define ATTR_TYPE15			"SQL_ATTR_IGNORE_CANCEL"
#define ATTR_TYPE15_VALUE1	"0"	// SQLCancel() is processed as a stopServer request (default)
#define ATTR_TYPE15_VALUE2	"1"	// SQLCancel() is ignored and a SQL_SUCCESS is returned

#define ATTR_TYPE16			"SQL_ATTR_FETCH_AHEAD"
#define ATTR_TYPE16_VALUE1	"ON"	// enables fetch ahead
#define ATTR_TYPE16_VALUE2	"OFF"	// fetch ahead off by default

#define ATTR_TYPE17			"EVENT_EXTENDED_21036"
#define ATTR_TYPE17_VALUE1	"ON"
#define ATTR_TYPE17_VALUE2	"OFF"

#define ATTR_TYPE18			"SQL_ATTR_ENABLE_LONGVARCHAR"
#define ATTR_TYPE18_VALUE1	"ON"	// enables long varchar
#define ATTR_TYPE18_VALUE2	"OFF"	// long varchar is disabled by default

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
#define SQL_SHAPE_WARNING			-108

#define ANONYMOUS_USER "ANONYMOUS"
#define TOO_LONG_HOST_NAME "TOO_LONG_HOST_NAME"

// will eventuall replace PRIMARY_PORT	"$ODBCPR", BACKUP_PORT
// "$ODBCBK" and SHUTDOWN_PORT "$ODBCSD"
#define ODBCMX_PORT_AS			"$OXA0000"
#define ODBCMX_PORT_AS_START	"$OXASTRT"
#define ODBCMX_PORT_AS_STOP		"$OXASTOP"

#define ODBCMX_ERR_MASK				0x2FFFFFFF

#define MAX_NSKCATALOGNAME_LEN	25

#ifndef MAX_SESSION_ID_LEN			// This should be available in sqlcli.h
  #define MAX_SESSION_ID_LEN	104 // 103 + null terminator.
#endif

#define SESSION_ID_LEN			MAX_SESSION_ID_LEN
#define STATISTICS_INTERVAL	60
#define STATISTICS_LIMIT	60

#ifndef MAX_SESSION_NAME_LEN		// This should be available in sqlcli.h
  #define MAX_SESSION_NAME_LEN	24
#endif

#define SESSION_NAME_LEN		MAX_SESSION_NAME_LEN

#define ODBCMX_DEFAULT_CATALOG	"TRAFODION"
#define ODBCMX_PREV_DEFAULT_CATALOG	"NEO"
#define ODBCMX_DEFAULT_SCHEMA	"USR"

#define ODBCMX_DEFAULT_VOL		"$SYSTEM"
#define ODBCMX_DEFAULT_SUBVOL	"ZMXODBC"

// defaults to kill esp orphans

#define SQL_RWRS_SPECIAL_INSERT  16 //DBT

#define PGMFILENAME 4
#define PROCPIN 38
#define MOMPHANDLE 40
#define PROCPHANDLE 48
#define RETURN_VALUES_MAXLEN 2000
#define EACH_RETURN_BUF 20
#define SRCH_ATTR_COUNT 2
#define SRCH_OPTION 2

#define MAX_KILL_ATTEMPTS 2
#define DELAY_KILL_ATTEMPT 50		//0.5 sec
#define CHECK_ORPHANS	10	//10 seconds

#ifdef USE_NEW_PHANDLE //for new phandle changes
#define TCPU_DECL(name)      int name
#define TPIN_DECL(name)      int name
#define TVAR_DECL(name)		 int name
#define TPT_DECL(name)       SB_Phandle_Type name
#define TPT_PTR(name)        SB_Phandle_Type *name
#define TPT_REF(name)        (&name)
#define TPT_TYPE             SB_Phandle_Type
#define TPT_RECEIVE			 FS_Receiveinfo_Type*
#else
#define TCPU_DECL(name)      short name
#define TPIN_DECL(name)      short name
#define TVAR_DECL(name)      short name
#define TPT_DECL(name)       short name[10]
#define TPT_PTR(name)        short *name
#define TPT_REF(name)        (name)
#define TPT_RECEIVE	 short*
#endif
//---------------------------------

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

// WMS Server type

typedef enum _WMS_SRVR_TYPE
{
	WMS_UNKNOWN_SRVR = 0,
	WMS_MGR_SRVR,
	WMS_STATS_SRVR,
	WMS_RULE_SRVR,
	WMS_COM_SRVR,
	WMS_OFFND_SRVR,
	WMS_CFG_SRVR,
	WMS_SRVR_SRVR

} WMS_SRVR_TYPE;

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

typedef enum _TCP_ADDRESS
{
	ADDRESS_INIT = 0,
	IP_ADDRESS,
	HOST_NAME,
	SEGMENT_NAME
} TCP_ADDRESS;

typedef enum _DCS_SERVER_STATE
{
	INITIAL = 0,
	AVAILABLE = 1,
	CONNECTING,
	CONNECTED,
	CONNECT_FAILED,
	CONNECT_REJECTED
} DCS_SERVER_STATE;
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
#define SQLSVC_EXCEPTION_DLLOPEN_ERR	"Open DLL(cfgdll)failed. Can't execute MXCS commands."
#define SQLSVC_EXCEPTION_DLSYM_ERR		"dlsym call failed. Can't complete the requetsed command."
#define ASSVC_EXCEPTION_FAIL_ASLIST     "Failed to get List of Services or Service information."
#define SQLSVC_EXCEPTION_INVALID_SYNTAX "Failed to parse the command. Invalid Syntax."
#define SQLSVC_EXCEPTION_NOT_AUTHORISED "The user has insufficient privileges and the operation cannot complete."
#define SQLSVC_EXCEPTION_QUOTE_ERR		"Quotes not allowed around the name."
#define ODBCSVC_EXCEPTION_OPEN_SESSION_FAILED     "CFGOpenSession failed."
#define SQLSVC_EXCEPTION_DELETE_ERR     "DS may be in use(not in stopped state). Cann't delete."
#define ASSVC_EXCEPTION_DEFAULT_DS      "Cannot delete default DataSource"
#define SQLSVC_EXCEPTION_DS_LIMITS		"Please check the server limits. The available and initial value cannot exceed the maximum value"
#define SQLSVC_EXCEPTION_DS_ALREADY_EXISTS "DS already exists with this name."
#define ASSVC_EXCEPTION_FAIL_ASSTART	"Failed to start Service. Service may not exist."
#define ASSVE_EXCEPTION_FAIL_DS_START   "Failed to start DataSource. Invalid DataSource or Serivice Name."
#define ASSVE_EXCEPTION_FAIL_AS_STOP    "Failed to stop service. Invalid service name."
#define ASSVE_EXCEPTION_FAIL_DS_STOP    "Failed to stop DataSource. Invalid DataSource or Serivice Name."
#define ASSVC_EXCEPTION_FAIL_DS_AUTOSTART "Failed to alter DataSource.DataSource may not exist."
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
#define ASSVC_EXCEPTION_UNSUPPORTED_DRIVER_VERSION "Unsupported driver version"
#define ASSVC_EXCEPTION_INCOMPATIBLE_VERSION "Version Mismatch, Please Check Your Client Version"
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
#define SQLSVC_EXCEPTION_INVALID_INFOSTATS_STMT_LABEL "Invalid statement/cursor name specified"
#define SQLSVC_EXCEPTION_INVALID_PLAN_STMT_LABEL "Invalid statement specified."
#define SQLSVC_EXCEPTION_PLAN_STMT_LABEL_ALREADY_EXISTS "Statement Name already exists."
#define SQLSVC_EXCEPTION_PLAN_STMT_CONTROL_RESET_FAILED "Unable to reset CQD or Cut CQS."
#define SQLSVC_EXCEPTION_INVALID_SCHEMA_CATALOG_OPTION "Invalid Schema or catalog specified in the Datasource"
#define SQLSVC_EXCEPTION_SERVICE_ALREADY_EXISTS "Service already exists."
#define SQLSVC_EXCEPTION_SERVICE_NOT_EXISTS "Service does not exists."
#define SQLSVC_EXCEPTION_SERVICE_CANNOT_DELETE "Cannot delete Reserved Service name."
#define SQLSVC_EXCEPTION_SERVICE_INVALID_ARGUMENTS "Number of arguments provided for this command are incorrect."
#define SQLSVC_EXCEPTION_SERVICE_INCORRECT_ARGUMENT_TYPE "Incorrect argument type provided for this command."
#define SQLSVC_EXCEPTION_SERVICE_QUERY_FAILED "Query failed"
#define SQLSVC_EXCEPTION_SERVICE_LIMIT_SERVICE_QUERIES "MAX_EXECUTING or MAX_WAITING value is not with in the limits."
#define SQLSVC_EXCEPTION_SERVICE_LIMIT_SYSTEM_QUERIES "MAX_EXECUTING or MAX_WAITING value exceeded the max limit across all SERVICES."

#define SQLSVC_EXCEPTION_STATEMENT_INVALID_LENGTH "Statement Name length is greater than 32 bytes."
#define SQLSVC_EXCEPTION_PASSWORD_ENCRYPTION_REQUIRED "Password encryption is required. Please check client version."

// Literals used in the outContext between Driver and Server
#define	NT_ODBCAS_COMPONENT		1
#define NSK_ODBCAS_COMPONENT		2
#define	SQL_COMPONENT			3
#define ODBC_SRVR_COMPONENT		4
#define ODBC_CFGSRVR_COMPONENT		5
#define CFGCL_COMPONENT			6
#define	DRVR_COMPONENT			7
#define APP_COMPONENT			8
#define CLPS_SRVR_COMPONENT		9
#define	JDBC_DRVR_COMPONENT		20
#define	LINUX_DRVR_COMPONENT		21
#define	HPUX_DRVR_COMPONENT		22
#define	AIX_DRVR_COMPONENT		23
#define OLEDB_DRVR_COMPONENT		24
#define DOT_NET_DRVR_COMPONENT		25
#define WIN_UNICODE_DRVR_COMPONENT		26
#define LINUX_UNICODE_DRVR_COMPONENT	27
#define HPUX_UNICODE_DRVR_COMPONENT		28
#define SUNSPARC32_DRVR_COMPONENT 30
#define SUNSPARC64_DRVR_COMPONENT 31

#define QUERY_SERVICE_QS_COMPONENT		90
#define QUERY_SERVICE_STATS_COMPONENT	91
#define QUERY_SERVICE_COM_COMPONENT		92
#define QUERY_SERVICE_SYNC_COMPONENT	93
#define QUERY_SERVICE_DLL_COMPONENT		94
#define QUERY_SERVICE_RULE_COMPONENT	95
#define QUERY_SERVICE_OFFND_COMPONENT	96

#define NSK_ENDIAN				256
#define NT_ENDIAN				0

#define	NT_VERSION_MAJOR_1	1
#define	NT_VERSION_MINOR_0	0
#define NT_BUILD_1			1

#define	 SQLERRWARN 1
#define	 ESTIMATEDCOSTRGERRWARN	2
#define  INFOSTATSERR 3
#define  CONFIGERR 4
#define	 SECURITYERR 5

#define	NSK_VERSION_MAJOR_1	NT_VERSION_MAJOR_1+2
#define	NSK_VERSION_MINOR_0	0
#define NSK_BUILD_1			1

// Added for version checking
#define MINDRVR_MAJVER	1
#define MINDRVR_MINVER 101

#define NDCS_MAJ_VER 3
#define NDCS_MIN_VER 0

//Added for displaying the messges according to error code returned
#define PATH_NOT_FOUND			4002
#define PROCESS_SPAWN_TIMEOUT	4126
#define SRVR_PATH_NOT_FOUND     21002
#define PROCESS_SERVICE_ERROR	20007

// Added for unicode driver checking
#define IS_UNICODE_DRVR_COMPONENT(x) \
	((x) == WIN_UNICODE_DRVR_COMPONENT || \
	 (x) == LINUX_UNICODE_DRVR_COMPONENT || \
	 (x) == HPUX_UNICODE_DRVR_COMPONENT)

#define EXCEPTION				1
#define	NO_MEMORY				2
#define WRONG_SERVER_STATE		3
#define WRONG_OBJECT			4
#define WRONG_DSID				5
#define WRONG_DSNAME			6
#define PROGRAM_ERROR			7

//-------------------------- NDCS SUBSTATE ---------------------------
typedef enum _NDCS_SUBSTATE
{
	NDCS_INIT = 0,
	NDCS_DLG_INIT,
	NDCS_CONN_IDLE,
	NDCS_DLG_TERM,
	NDCS_DLG_BREAK,
	NDCS_STOP_SRVR,
	NDCS_RMS_ERROR,
	NDCS_REPOS_IDLE,
	NDCS_REPOS_INTERVAL,
	NDCS_REPOS_PARTIAL,
	NDCS_EXEC_INTERVAL,
	NDCS_CONN_RULE_CHANGED,
	NDCS_CLOSE,
	NDCS_PREPARE,
	NDCS_WMS_ERROR,
	NDCS_QUERY_CANCELED,
	NDCS_QUERY_REJECTED,
//
} NDCS_SUBSTATE;
//==================================================================

#include "QSGlobal.h"
#include "QSData.h"
//-------------- Adaptive Segmentation Data Structure -------------

#define AS_TIMEOUT	60 * 100		// 60 seconds
#define CATCHER 0x0F0F0F
enum AS_OPER
{
	ASOPER_INIT,
	ASOPER_ALLOCATE,
	ASOPER_DEALLOCATE,
	ASOPER_DEALLOCATE_ALL
};

typedef struct ADAPTIVE_SEGMENT_DATA_
{
	long catcher;
	AS_OPER operation;
	short segment;

} ADAPTIVE_SEGMENT_DATA, *pADAPTIVE_SEGMENT_DATA;

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
	char				TcpProcessName[MAX_TCP_PROCESS_NAME_LEN];
	char				ASProcessName[MAX_PROCESS_NAME_LEN];
	char				QSProcessName[MAX_PROCESS_NAME_LEN];
	char				EmsName[EXT_FILENAME_LEN];
	int				EmsTimeout;
	char				TraceCollector[EXT_FILENAME_LEN];
	char				RSCollector[EXT_FILENAME_LEN];
	long				initIncSrvr;
	long				initIncTime;
	bool				DSG;
	bool				srvrTrace;
	short				majorVersion;
	char				DSName[MAX_DSOURCE_NAME + 1];
	TCP_ADDRESS			tcp_address;
	char				neoODBC[500];
	short				minSeg;
	long				initSegTimeout;
//-----------------------------------------------
	char*				sql;
	bool				mute;//Dashboard testing - no 21036 message
	bool				ext_21036; // true - for single row per query, false - original 21036 msg, default true
	bool                            isBlades;
        bool                            floatIP;
	bool				timeLogger;
	char				QSsyncProcessName[MAX_PROCESS_NAME_LEN];
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

typedef struct _NEO_SEGMENT
{
	char name[MAX_NODE_NAME];
	long number;
	char volume[10];
	char subvol[10];
	short tos_version;
	unsigned short HSW;
	unsigned short LSW;
} NEO_SEGMENT;

// --- this is only for OssCfgCl
typedef struct _NEO_NODE
{
	char nodename[MAX_NODE_NAME];
	long nodenumber;
	char volume[10];
	char subvol[10];
	short tos_version;
	unsigned short HSW;
	unsigned short LSW;
} NEO_NODE;

//wms_mapping
typedef struct _MAPPING_OPTIONS
{
	unsigned long options;
	char applicationName[MAX_APPLICATION_NAME_LEN + 1];
	char sessionName[SESSION_NAME_LEN + 1];
	char userRole[MAX_TEXT_SID_LEN + 1];
	char userName[MAX_TEXT_SID_LEN + 1];
	char userDBName[MAX_TEXT_SID_LEN + 1];
	char dataSourceName[MAX_DSOURCE_NAME + 1];
	unsigned short sqlCmdsLen;
	char sqlCmds[MAX_SQL_CMD_LEN + 1];
//
// CONN WARN
//
	unsigned short reason;
	unsigned short ruleId;
	unsigned short exprNo;

} MAPPING_OPTIONS;


typedef struct _SRVR_GLOBAL_Def
{
	_SRVR_GLOBAL_Def(){
		bRowsetSupported = FALSE;
		bAutoCommitOn = FALSE;
		bAutoCommitSet = FALSE;
		javaConnIdleTimeout = JDBC_DATASOURCE_CONN_IDLE_TIMEOUT;
        clipVarchar  = 0;
		bSpjEnableProxy = FALSE;
		lastCQDAdaptiveSegment = -1;
		bWMS_AdaptiveSegment = false;
		fnumAS = -1;
		QSFileSystem = NULL;
		QStimestamp_current = 0;
		QStimestamp_loaded = 0;
		QStimestamp = 0;
		pQSConnRules = NULL;
		QSServiceId = 0;
		sqlPlan = false;
		sqlText = false;
		maxRowsFetched = 0;
		sqlCmdsLen = 0;
		sqlCmds[0] = '\x0';
		execSqlCmds = false;
		masterPrty = 0;
		prtyChanged = false;
		allocatedResources = 0;
		MXCS_SCHEMA_Version =0;
		WMSSrvrType = WMS_UNKNOWN_SRVR;
		m_bstart_aggr_timer = false;
		m_aggr_query_types = 0;
		m_aggr_repos_interval = 0;
		m_aggr_wms_interval = 0;
		m_aggr_exec_interval = 0;
		m_aggr_stats_once = 0;		// perf
		receiveThrId = 0;
	    mutex = new SB_Thread::Errorcheck_Mutex(true);

		wmsActive = false;
		process_id = 0;
		cpu = 0;
		qsOpen = false;
		bzero(segmentname,sizeof(segmentname));
		m_statisticsPubType = STATISTICS_AGGREGATED;
		m_bStatisticsEnabled = true;
		m_iAggrInterval = STATISTICS_INTERVAL;
		m_iQueryPubThreshold = STATISTICS_LIMIT;
		m_NodeId = 0;
		bzero(m_ProcName,sizeof(m_ProcName));
		m_bNewConnection = false;
		m_bNewService = false;
               clientKeepaliveStatus = false;
               clientKeepaliveIdletime = 0;
               clientKeepaliveIntervaltime = 0;
               clientKeepaliveRetrycount = 0;
		m_rule_wms_off = false;		// perf
		m_rule_endstats_off = false;// perf

		bzero(m_service_name,sizeof(m_service_name));
		bzero(m_query_name,sizeof(m_query_name));

		traceLogger = NULL;

		WmsNid = -1;
		bConfigurationChanged = false;

		bspjTxnJoined = FALSE;
		spjTxnId = 0;
		dcsCurrState = INITIAL;

		bzero(srvrObjRef,sizeof(srvrObjRef));

	}

	long				debugFlag;
	long				eventFlag;
	IDL_OBJECT_def		asSrvrObjRef;
	SRVR_TYPE			srvrType;
	WMS_SRVR_TYPE		WMSSrvrType;
	IDL_long			DSId;
	PROCESS_ID_def		nskProcessInfo;
	SRVR_STATE			srvrState;
	DIALOGUE_ID_def		dialogueId;
	IDL_OBJECT_def		srvrObjRef;
	CEE_handle_def		ASSvc_proxy;
	CEE_handle_def		ASSvc_MonitorProxy;
	CEE_handle_def		ASSvc_ifch;
	SRVR_CONTEXT_def	srvrContext;
	IDL_long		javaConnIdleTimeout;		//timeout in seconds, -1 from datasource, 0 no timeout

	long			odbcConnIdleTimeout;		//For ODBC Client timeout value

    IDL_long            clipVarchar ;
	//	BOOL				validTimerHandle;
	char				ASProcessName[MAX_PROCESS_NAME_LEN];
	PROCESS_ID_def		nskASProcessInfo;
	CEE_handle_def		ASTimerHandle;
	bool				bSkipASTimer;
//
// WMS ----------------------------------------------------------------
//
	int64				allocatedResources;
	int64				maxRowsFetched;			//WMS in Service can have threshold on RowsFetched
	char				QSProcessName[MAX_PROCESS_NAME_LEN];
	short				QSProcessLen;
	PROCESS_ID_def		nskQSProcessInfo;
	short				WmsNid;					// Wms assigned to this MXOSRVR
	bool				bConfigurationChanged;	// When configuration changed (node down)

	void*				QSFileSystem;
	int64				QStimestamp_current;
	int64				QStimestamp_loaded;

	int64				QStimestamp;
	void*				pQSConnRules;
	short				QSServiceId;
	char				QSServiceName[MAX_PROCESS_NAME_LEN]; // Service name set by the user (QSMGR can select different one)
	char				QSRoleName[MAX_ROLE_LEN + 1];	// ROLE user name - ROLE.USER, ROLE.MGR, SUPER.SERVICES etc
	char				QSUserName[MAX_TEXT_SID_LEN + 1];
	char				QSDBUserName[MAX_TEXT_SID_LEN + 1];
	char				QSRuleName[MAX_RULE_NAME_LEN + 1];

//
// sqlPlan and sqlText are dynamic booleans which are taken from the Service choosen by QSMGR.
// It does not have to be the same as selected by the user
//
	bool				sqlPlan;			// QSMGR returns it
	bool				sqlText;			// QSMGR returns it
	long				sqlCmdsLen;
	char				sqlCmds[MAX_SQL_CMD_LEN + 1];	// QSMGR returns it
	bool				execSqlCmds;
	short				masterPrty;
	bool				prtyChanged;
	short				srvrPriority;
//
// ---------------------------------------------------------------------
//
	char				ClientComputerName[MAX_COMPUTER_NAME_LEN + 1];
	char				ApplicationName[MAX_APPLICATION_NAME_LEN + 1];
	char				mappedProfileName[MAX_APPLICATION_NAME_LEN + 1];
	char				mappedSLAName[MAX_APPLICATION_NAME_LEN + 1];
	int		                CreatorAccessId;
	int		                ProcessAccessId;
	char				RoleName[MAX_ROLE_LEN + 1];		// ROLE Name
										//	- MGR  for ROLE.MGR,
										//	- USER for ROLE.USER
										//	- DBA  for ROLE.DBA
										//	- SUPER.SERVICES for SUPER.SERVICES etc.
	bool				DSG;
	ODBCMXTraceMsg		*traceLogger;
	char				sessionId[SESSION_ID_LEN];
	char				TraceCollector[EXT_FILENAME_LEN];
	char				RSCollector[EXT_FILENAME_LEN];
	char				IpAddress[MAX_IP_ADDRESS_LEN];
	char				HostName[MAX_HOST_NAME_LEN];
	char				sutVer[32];
	BOOL				bAutoCommitOn;
	BOOL				bAutoCommitSet;

	BOOL				bRowsetSupported;

	BOOL				resGovernOn;
	BOOL				envVariableOn;
	char				userSID[MAX_TEXT_SID_LEN+1];// This points to the SID in which the server is running
								    // If this is same as the incoming SID do not flush the
								    //  cache
	long				userID;		// Added for replacing the expensive
									// USER_GETINFO_() call with PROCESS_GETINFO()
	__int64				redefTime;	// Store the last time this users information
									// was changed

	tip_handle_t		tip_gateway;

       BOOL                    clientKeepaliveStatus;
       int                     clientKeepaliveIdletime;
       int                     clientKeepaliveIntervaltime;
       int                     clientKeepaliveRetrycount;
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
	VERSION_def			sqlVersion;
	VERSION_def			drvrVersion;
	VERSION_def			appVersion;
	char				DefaultCatalog[MAX_SQL_IDENTIFIER_LEN+3];
	char				DefaultSchema[MAX_SQL_IDENTIFIER_LEN+3]; // this is to allow double quotes around the schema name
	short				EnvironmentType; // Since from 2.0 SQL we don't need SHORTANSI,
								// ApplicationType is merged into this variable.
	char				NskSystemCatalogsTableName[EXT_FILENAME_LEN+1];
	char				DSName[MAX_DSOURCE_NAME+1];
	char				SystemCatalog[MAX_SQL_IDENTIFIER_LEN+3]; // System catalog name
	short				tfileNum; // TFILE number in multi context env.
	long				schemaVersion;	//this is the schema version of the user schema, used in the catalog api's
	long				MXCS_SCHEMA_Version;	//this is the schema version of systems schema.
	long				resourceStatistics;
	long				m_FetchBufferSize;		// additional end
//===================== This part is for NEO =======================================
	short				neoSegmentMax;
	NEO_SEGMENT*		neoSegment;
	short				neoActiveSegments;
	char*				neoODBC;
//
// timer to check esp orphans
//
	CEE_handle_def		CheckTimerHandle;
	bool				isShapeLoaded;

	bool				modeSpecial_1;

// session cleanup
	long				numConnection;
	long				cleanupByConnection;
	long				cleanupByTime;
	time_t				lastCleanupTime;
	double				estCardinality;
	double				estCost;
// Dashboard testing - no 21036 message
	bool				mute;
	bool				ext_21036;
	bool				bSpjEnableProxy; // JDBC: SPJ Enable Proxy Syntax
	bool				bspjTxnJoined;
	IDL_long_long		spjTxnId;

//
// lastCQDAdaptiveSegment - adaptive segment set by last CQD
//
	short				lastCQDAdaptiveSegment;
	bool				bWMS_AdaptiveSegment;		//true or false - default false
	short				fnumAS;

//phandle change for SQ
	TPT_DECL(pASHandle);

	int				isoMapping;	// charset support
	bool				fetchAhead; // fetch ahead support
	MAPPING_OPTIONS		mapOptions;	//wms_mapping
	bool				defaultSchemaAccessOnly;
	bool			enableLongVarchar; //enable/disable old longvarchar feature

	bool m_rule_wms_off;		// perf
	bool m_rule_endstats_off;	// perf

//
// AGGREGATION----------------data definition
//
	bool	m_bstart_aggr_timer;
	short	m_aggr_query_types;
	short	m_aggr_repos_interval;
	short	m_aggr_wms_interval;
	short	m_aggr_exec_interval;
	short	m_aggr_stats_once;		// perf
	statistics_type m_statisticsPubType;
	bool	m_bStatisticsEnabled;
	int	m_iAggrInterval;
	int	m_iQueryPubThreshold;
	int	m_NodeId;
	char	m_ProcName[MS_MON_MAX_PROCESS_NAME];	
    int     receiveThrId;
    SB_Thread::Errorcheck_Mutex *mutex; // mutex

	// Added for performance improvements
	short	wmsActive;
	char	segmentname[128];
	TCPU_DECL(cpu);
	long	process_id;
	bool	qsOpen;

	bool	m_bNewConnection; //used before calling WMS
	bool	m_bNewService;
	char	m_service_name[MAX_SERVICE_NAME_LEN + 1];
	char	m_query_name[MAX_QUERYNAME_LEN + 1];

	// Added for 64bit work
	//typedef std::map<IDL_long, Long> STMT_HANDLE_MAP;
	typedef __gnu_cxx::hash_map<IDL_long, Long> STMT_HANDLE_MAP;
	STMT_HANDLE_MAP	stmtHandleMap;
	//std::map<int, long>::iterator stmtHandleIter;
	//std::pair<map<int, long>::iterator,bool> stmtHandleRet;

	long				portNumber;

	DCS_SERVER_STATE dcsCurrState;		// Contains current DCS state
#ifdef __TIME_LOGGER
	TimeLogger timeLogger;
	bool timeLoggerFlag;
#endif

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
	IDL_short		clusterNodeId;		// For node number across clusters
	char				processName[MAX_PROCESS_NAME_LEN];
	char				fullProcessName[MAX_PROCESS_NAME_LEN];
	char				nodeName[MAX_NODE_NAME];
	char				computerName[MAX_COMPUTER_NAME_LEN+1];
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

	long				srvrConnectionCount;
	int					lastUserId;
//	int					curUserId;

	char				IpAddress[MAX_IP_ADDRESS_LEN];
	char				HostName[MAX_HOST_NAME_LEN];

	bool				orphansKilled;

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
	char			HostName[MAX_HOST_NAME_LEN]; // Cluster or Hostname
	char			InstanceName[MAX_INSTANCE_NAME_LEN]; // Instance Name or Seaquest ID Name
	long			transport;
	long			portNumber;
	long			IpPortRange;
	char			TcpProcessName[MAX_TCP_PROCESS_NAME_LEN];
	char			EmsName[EXT_FILENAME_LEN];
	char			TraceCollector[EXT_FILENAME_LEN];
	char			RSCollector[EXT_FILENAME_LEN];
	int                     CreatorAccessId;
	int	                ProcessAccessId;
	long			initIncSrvr;
	long			initIncTime;
	bool			DSG;
	short			countCfgStart;
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
	char			cfgProcessName[MAX_PROCESS_NAME_LEN];
	char			cfgNodeName[MAX_NODE_NAME];
	// OSS absolute path names can be up to 1023 characters, however, the servers will be in the same location as
	// the AS (which is in a Guardian space) with a maximum of 20 characters i.e. /G/DATA001/x1234567,
	// so we'll use the NT default (in case it applies to NT also)
	char			AS_OSSPathname[_MAX_PATH+1];
	char			QSProcessName[MAX_PROCESS_NAME_LEN];
	CEE_handle_def		ConnectingTimerHandle;
	TCP_ADDRESS		tcp_address;
//===================== This part is for NEO =======================================
// NEO can have max 16 nodes - it is not true anymore
// every node can have max 16 CPUs
//
	bool			isBlades;
	short			neoSegmentMax;
	NEO_SEGMENT*		neoSegment;
	short			neoActiveSegments;
	char*			neoODBC;

//Dashboard testing - no 21036 message
	bool			mute;

	bool			ext_21036;
	int			isoMapping;	// charset support
// DBT - rule based engine
	bool			rule_based_engine;	// false - no RBE
	char			cpuDistFile[36];	// cpu weight file name
	CEE_handle_def		CpuDistTimerHandle;	// timer handler

//
// These pointers can be used in debugging process to easy check WMS shared memory.
//
	pQUERYSERVICE_DESC	m_pQS;
	pOUTPUT_QUERY_LIST	m_pQueryList;
	pEXEC_QUERY_LIST	m_pExecList;
	pSTATS_MSG_LIST		m_pStatsMsgList;
	pCOM_MSG_LIST		m_pComMsgList;
	pRULE_MSG_LIST		m_pRuleMsgList;
	pGLOBAL_STATS		m_pGlobalStats;
	pTRACE				m_pTrace;
	pRULE_LIST			m_pRuleList;
	pWARN_QUERY_LIST	m_pWarnList;
	pWMS_STATS_DATA		m_pWmsStats;
	pOFFENDER_LIST		m_pOffenderList;
	pCL_MSG_LIST		m_pCLMsgList;
//
	long	ext_address;
	short	shared_memory_segment_id;

	// Cache the userName -> userID so that we can avoid a expensive (NEO_)USER_GETINFO call


	__gnu_cxx::hash_map<std::string, int> userIDcache;
	char			QSsyncProcessName[MAX_PROCESS_NAME_LEN];

#ifdef __TIME_LOGGER
		TimeLogger timeLogger;
		bool timeLoggerFlag;
#endif
} AS_SRVR_GLOBAL_Def;

#endif
