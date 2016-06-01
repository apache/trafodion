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
// MODULE: SrvrDrvr.h
//
// PURPOSE:

#ifndef _DRVRSRVR_DEFINED
#define _DRVRSRVR_DEFINED

#include "sqltypes.h"

#define MAX_STMT_LABEL_LEN		512
#define MAX_SQLDESC_NAME_LEN	512
#define MAX_ERROR_MSG_LEN		1024

#define MAX_CURSOR_NAME_LEN			512
#define MAX_STMT_NAME_LEN			512

#define MAX_PARAMETER_NAME_LEN		512
#define MAX_ANSI_NAME_LEN			512

#define MAX_DESC_NAME_LEN		512
#define MAX_MODULE_NAME_LEN		368 //(128+128+128+2)
#define MAX_QUERY_NAME_LEN		160

#define TYPE_UNKNOWN				0x000000000L
#define TYPE_SELECT					0x000000001L
#define TYPE_UPDATE					0x000000002L
#define TYPE_DELETE					0x000000004L
#define TYPE_INSERT					0x000000008L
#define TYPE_EXPLAIN				0x000000010L
#define TYPE_CREATE					0x000000020L
#define TYPE_GRANT					0x000000040L
#define TYPE_DROP					0x000000080L
#define TYPE_INSERT_PARAM			0x000000100L
#define TYPE_SELECT_CATALOG			0x000000200L
#define TYPE_SMD					0x000000400L // added for user module support
#define TYPE_CALL					0x000000800L // added for stored proc call support
#define TYPE_STATS					0x000001000L // added for INFOSTATS support
//#define TYPE_CONFIG					0x000002000L // removed as part of cleanup effort
#define TYPE_QS								0x000004000L // added for query services support
#define TYPE_QS_OPEN				0x000004001L // query service
#define TYPE_QS_CLOSE				0x000004002L // query service
#define TYPE_CMD						0x000003000L
#define TYPE_CMD_OPEN				0x000003001L
#define TYPE_CMD_CLOSE				0x000003002L


// Statement runtime cursor state
#define CRSR_UNKNOWN		0x00000000L
#define CRSR_NAMED			0x00000001L
#define CRSR_OPEN			0x00000008L

#define MAX_SQL_COMMAND_LEN			16000
#define SQL_MODE_NULL				1005

#define SET_CATALOG					1000
#define SET_SCHEMA					1001
#define SET_LOCATION				1002
#define SET_INFER_NCHAR				1003
#define RESET_DEFAULTS				1004
#define SET_ODBC_PROCESS			1005
#define SET_CATALOGNAMETYPE			1006
#define SET_SETANDCONTROLSTMTS		1007
#define SET_JDBC_PROCESS			1008


#define CONTROL_TABLE_PRIORITY		1012
#define SET_EXPLAIN_PLAN			1013
#define CUT_CONTROLQUERYSHAPE		1014
#define RESET_RESET_DEFAULTS		1015
#define BEGIN_SESSION				1016
#define END_SESSION					1017
#define SET_SESSION_USERNAME		1018
#define SET_DBTR_PROCESS			1019
#define SET_STATISTICS				1020
#define SET_SERVICE_NAME			1030
#define SET_SPJ_ENABLE_PROXY		1040 // JDBC sets this connection attribute to enable SPJ Rowset Proxy syntax
#define SQL_ATTR_JOIN_UDR_TRANSACTION		1041	// Bug fix 3196
#define SQL_ATTR_SUSPEND_UDR_TRANSACTION	1042	// Bug fix 3196
#define SET_INPUT_CHARSET			1050
#define SET_TERMINAL_CHARSET		1051
#define SET_AUTOBEGIN				1053
#define SET_NVCI_PROCESS			1060
#define CONN_IDLE_TIMER_RESET		1070
#define SQL_TXN_VERSIONING			10
#define EXTERNAL_STMT			0
#define INTERNAL_STMT			1
#define WMS_QUERY_MONITORING		1080

#define  HP_DEFAULT_SERVICE			"HP_DEFAULT_SERVICE"

typedef struct tagDATE_TYPE
{
        unsigned short		year;
        unsigned char		month;
        unsigned char		day;
} DATE_TYPES;

typedef struct tagTIME_TYPE
{
        unsigned char		hour;
        unsigned char		minute;
        unsigned char		second;
		unsigned char		fraction[4];
} TIME_TYPES;

typedef struct tagTIMESTAMP_TYPE
{
        SWORD   year;
        UCHAR   month;
        UCHAR   day;
        UCHAR   hour;
        UCHAR   minute;
        UCHAR   second;
		UCHAR   fraction[4]; // This has been treated as UDWORD by SQL. However this is not
							 // word aligned. We should ensure that we don't treat this as UDWORD
							 // without copying to temp variable. Same is the case when we do copy.
							//  Using sizeof (UDWORD) for copying to and fro.
} TIMESTAMP_TYPES;

// moved here from Drvr.h as needed by both client and server
#define MAXLOGINNAME          MAXLOGNAME     // max length of field in loginrec
                                             // essentially two definitions of same
                                             // value due to order dependencies in
                                             // build objects
#define SQLCLI_ODBC_VERSION 2
#define SQLCLI_ODBC_MODULE_VERSION 1

class SRVR_DESC_HDL {
public:
	long	dataType;
	BYTE	*varPtr;
	BYTE	*indPtr;
	long	charSet;
	long	length;
	long	paramMode;
};

typedef struct tagDESC_HDL_LISTSTMT
{
		long DataType;
		long Length;
		long Nullable;
		long VarBuf;
		long IndBuf;
} DESC_HDL_LISTSTMT;

#define SQLCHARSETSTRING_ISO88591_DEF  "ISO88591"

// Function Prototypes
namespace SRVR {
	extern int getAllocLength(int DataType, int Length);
}

//--------------- DATETIME MP datatypes ----------------------------

#define SQLDTCODE_YEAR					4
#define SQLDTCODE_YEAR_TO_MONTH			5
//defineSQLDTCODE_YEAR_TO_DAY			1 //SQL DATE
#define SQLDTCODE_YEAR_TO_HOUR			7 //ODBC TIMESTAMP(0)
#define SQLDTCODE_YEAR_TO_MINUTE		8
//defineSQLDTCODE_YEAR_TO_SECOND		3 //SQL TIMESTAMP(0)
//defineSQLDTCODE_YEAR_TO_FRACTION		3 //SQL TIMESTAMP(1 - 5)
#define SQLDTCODE_MONTH					10
#define SQLDTCODE_MONTH_TO_DAY			11
#define SQLDTCODE_MONTH_TO_HOUR			12
#define SQLDTCODE_MONTH_TO_MINUTE		13
#define SQLDTCODE_MONTH_TO_SECOND		14
#define SQLDTCODE_MONTH_TO_FRACTION		14
#define SQLDTCODE_DAY					15
#define SQLDTCODE_DAY_TO_HOUR			16
#define SQLDTCODE_DAY_TO_MINUTE			17
#define SQLDTCODE_DAY_TO_SECOND			18
#define SQLDTCODE_DAY_TO_FRACTION		18
#define SQLDTCODE_HOUR					19
#define SQLDTCODE_HOUR_TO_MINUTE		20
//define SQLDTCODE_HOUR_TO_SECOND		2  //SQL TIME(0)
//define SQLDTCODE_HOUR_TO_FRACTION		2  //SQL TIME(1 - 6)
#define SQLDTCODE_MINUTE				22
#define SQLDTCODE_MINUTE_TO_SECOND		23
#define SQLDTCODE_MINUTE_TO_FRACTION	23
#define SQLDTCODE_SECOND				24
#define SQLDTCODE_SECOND_TO_FRACTION	24
#define SQLDTCODE_FRACTION_TO_FRACTION	29

#define DIALOGUE_ID_NULL_ERROR			-29401

// for Transactions
#define  TFILE_CONCURRENT_COUNT 100

#define TYPE_BLOB				2004
#define TYPE_CLOB				2005
#define CLOB_HEADING			"JDBC_CLOB_COLUMN -"
#define BLOB_HEADING			"JDBC_BLOB_COLUMN -"

#define SQL_INVALID_USER_CODE -8837
#define SQL_PASSWORD_EXPIRING 8857 // +ve code since a warning is returned by Executor
#define SQL_PASSWORD_EXPIRED -8857 // -ve code since a error is returned by Executor for password expired
#define SQL_PASSWORD_GRACEPERIOD 8837 // +ve code since a warning is returned by Executor
#define DEFAULT_EXPIRY_DAYS 2 // Assuming the safecom returns a warning when the password is about to expire in 2 days.

// Added this for backward compatibility
// Increment major version only incase we force customer to perform coldloads.
// This can be done when a change in interfaces or update in CLI in SQL or new catalog release in SQL etc.
// Increment minor version incase we release a new driver which should work with old or new server and vice verse.
#define	MXOSRVR_VERSION_MAJOR	3
#define	MXOSRVR_VERSION_MINOR	5 // result set support
#define MXOSRVR_VERSION_BUILD	1

// For Hash support.
#define STREAMING_MODE 1073741824 //(2^30)
#define STREAMING_DELAYEDERROR_MODE   536870912 //(2^29)
#define CHARSET		   268435456 //(2^28) // For charset changes compatibility
#define ROWWISE_ROWSET 134217728 //(2^27)
#define PASSWORD_SECURITY 67108864 //(2^26)
#define MXO_SPECIAL_1_MODE 512 // Compatibility mode

// InContext Connection1 Options Bits

#define INCONTEXT_OPT1_SESSIONNAME 2147483648 // (2^31)
#define INCONTEXT_OPT1_FETCHAHEAD 1073741824 // (2^30)
#define INCONTEXT_OPT1_CERTIFICATE_TIMESTAMP 536870912 //(2^29)
#define INCONTEXT_OPT1_CLIENT_USERNAME 268435456 //(2^28)

// OutContext Connection1 Options Bits
#define OUTCONTEXT_OPT1_ENFORCE_ISO88591 1 // (2^0)
#define OUTCONTEXT_OPT1_DOWNLOAD_CERTIFICATE 536870912 //(2^29)
#define OUTCONTEXT_OPT1_IGNORE_SQLCANCEL 1073741824 // (2^30)
#define OUTCONTEXT_OPT1_ROLENAME 2147483648 // (2^31)

//wms_mapping
#define MAPPING_NONE				0x00000000
#define MAPPING_APPL				0x00000001	//Application Group
#define MAPPING_SESSION				0x00000002	//Session Group
#define MAPPING_LOGIN				0x00000004	//Login Group
#define MAPPING_DSN					0x00000008	//Datasource Group

#endif
