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
//
// MODULE: SrvrDrvr.h
//
// PURPOSE: 

#ifndef _DRVRSRVR_DEFINED
#define _DRVRSRVR_DEFINED

#define MAX_STMT_LABEL_LEN		30
#define MAX_SQLDESC_NAME_LEN	60
#define MAX_ERROR_MSG_LEN		1024

#define MAX_CURSOR_NAME_LEN			30
#define MAX_STMT_NAME_LEN			30

#define MAX_PARAMETER_NAME_LEN		128
#define MAX_ANSI_NAME_LEN			128

#define MAX_DESC_NAME_LEN		128
#define MAX_MODULE_NAME_LEN		368 //(128+128+128+2)
#define MAX_QUERY_NAME_LEN		128

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
#define SET_SESSION_MASTER_PRIORITY		1009
#define SET_SESSION_MXCMP_PRIORITY		1010
#define SET_SESSION_ESP_PRIORITY		1011
#define CONTROL_TABLE_DP2_PRIORITY		1012
#define SET_EXPLAIN_PLAN			1013

#define SQL_TXN_VERSIONING			10
#define EXTERNAL_STMT			0
#define INTERNAL_STMT			1

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
		UCHAR   fraction[4]; // This has been treated as UDWORD by SQL/MX. However this is not
							 // word aligned. We should ensure that we don't treat this as UDWORD
							 // without copying to temp variable. Same is the case when we do copy.
							//  I am using sizeof (UDWORD) for copying to and fro.
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

#define SQLCHARSETSTRING_ISO88591_DEF  "ISO88591" 

// Function Prototypes
namespace SRVR {
	extern long getAllocLength(long DataType, long Length);
}

//--------------- DATETIME MP datatypes ----------------------------

#define SQLDTCODE_YEAR					4
#define SQLDTCODE_YEAR_TO_MONTH			5
//defineSQLDTCODE_YEAR_TO_DAY			1 //SQL/MX DATE
#define SQLDTCODE_YEAR_TO_HOUR			7 //ODBC TIMESTAMP(0)
#define SQLDTCODE_YEAR_TO_MINUTE		8
//defineSQLDTCODE_YEAR_TO_SECOND		3 //SQL/MX TIMESTAMP(0)
//defineSQLDTCODE_YEAR_TO_FRACTION		3 //SQL/MX TIMESTAMP(1 - 5)
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
//define SQLDTCODE_HOUR_TO_SECOND		2  //SQL/MX TIME(0)
//define SQLDTCODE_HOUR_TO_FRACTION		2  //SQL/MX TIME(1 - 6)
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
#define	MXOSRVR_VERSION_MINOR	4
#define MXOSRVR_VERSION_BUILD	1
// For Hash support.
#define STREAMING_MODE 1073741824 //(2^30) 
#define STREAMING_DELAYEDERROR_MODE   536870912 //(2^29)

#endif
