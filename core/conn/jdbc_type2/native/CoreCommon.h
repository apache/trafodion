/*************************************************************************
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
**************************************************************************/
//
// MODULE: corecommon.h
#ifndef _CORECOMMON_DEFINED
#define _CORECOMMON_DEFINED

// T2_REPO
//typedef long int64;
//typedef char BOOL;

extern "C" {
   void TIME( short * a );
}
char *_i64toa( long long value, char *string, int radix );
// T2_REPO

#include <set>
#include <string>
#include <cstring>
#include "tip.h"
#include "sqlcli.h"
#include "idltype.h"

// +++ T2_REPO
#include <PubQueryStats.h>
#include <platform_ndcs.h>
//

#define MAX_STMT_LABEL_LEN      512
#define MAX_CURSOR_NAME_LEN     512
#define MAX_STMT_NAME_LEN       512
#define MAX_ANSI_NAME_LEN       512
#define MAX_TEXT_SID_LEN        186
#define EXT_FILENAME_LEN        34
#define MAX_DBNAME_LEN          25 /* (1+7) + (1+7) + (1+8) */
/* \SYSTEM.$VOLUME.SUBVOLUME */
#define MAX_DESC_NAME_LEN       512
#define MAX_MODULE_NAME_LEN     368 //(128+128+128+2)

/* **Rowsets **/
#define MAX_SQLDESC_NAME_LEN    512
#define MAX_ERROR_MSG_LEN       1024
#define MAX_INTERNAL_STMT_LEN   512
#define MAX_PARAMETER_NAME_LEN      512
#define MAX_ANSI_NAME_LEN           512
#define MAX_FETCH_BUFFER_SIZE   0x80000

// +++ T2_REPO
#define MAX_USERNAME_LEN            128
#define MAX_SQL_IDENTIFIER_LEN      512
#define MAX_APPLICATIONID_LENGTH    128
#define MAX_DSOURCE_NAME            512
#define MAX_IP_ADDRESS_LEN          128
#define MAX_ROLE_LEN                128
#define MAX_COMPUTER_NAME_LEN       16*4 //limited in ODBC driver
#define MAX_APPLICATION_NAME_LEN    128*4 //limited in ODBC driver

#define MAX_QUERY_NAME_LEN          160
#define MAX_QUERY_ID_LEN            160
#define RMS_STORE_SQL_SOURCE_LEN    254
#define MAX_TXN_STR_LEN             64
#define SUB_QRY_TYPE_LEN            36
#define PAR_SYS_NAME_LEN            128
#define MAX_RULE_NAME_LEN           24*4

#ifndef MAX_SESSION_ID_LEN          // This should be available in sqlcli.h
  #define MAX_SESSION_ID_LEN        104 // 103 + null terminator.
#endif

#define SESSION_ID_LEN              MAX_SESSION_ID_LEN
#define STATISTICS_INTERVAL         60
#define STATISTICS_LIMIT            60

// statement statistics values ranges from 256 thru 32768
#define STMTSTAT_NONE               0   // during construct and destruct time
#define STMTSTAT_SQL                256
#define STMTSTAT_PREPARE            512
#define STMTSTAT_EXECUTE            1024
#define STMTSTAT_EXECDIRECT         2048
#define STMTSTAT_FETCH              4096
#define STMTSTAT_CLOSE              8192

// +++ T2_REPO - ToDo change NDCS to DCS
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

typedef enum _QUERY_STATE
{
    QUERY_INIT                          = 0,
//
    QUERY_WAITING                       = 0x0100,
    QUERY_WAITING_MAX_CPU_BUSY          = 0x0101,
    QUERY_WAITING_MAX_MEM_USAGE         = 0x0102,
    QUERY_WAITING_RELEASED_BY_ADMIN     = 0x0103,
    QUERY_WAITING_MAX_INSTANCE_EXEC_QUERIES = 0x0104,
    QUERY_WAITING_TXN_BACKOUT           = 0x0105,
    QUERY_WAITING_MAX_SSD_USAGE         = 0x0106, //ssd overflow
    QUERY_WAITING_MAX_ESPS              = 0x0107,
    QUERY_WAITING_EST_MAX_CPU_BUSY      = 0x0108,
    QUERY_WAITING_MAX_SERVICE_EXEC_QUERIES  = 0x0109,
    QUERY_WAITING_CANARY_EXEC              = 0x010A,
//
    QUERY_EXECUTING                     = 0x0200,
    QUERY_EXECUTING_RELEASED_BY_ADMIN   = 0x0201,
    QUERY_EXECUTING_RELEASED_BY_RULE    = 0x0202,
    QUERY_EXECUTING_CANCEL_IN_PROGRESS  = 0x0203,
    QUERY_EXECUTING_CANCEL_FAILED       = 0x0204,
    QUERY_EXECUTING_CANCEL_FAILED_8026  = 0x0205,
    QUERY_EXECUTING_CANCEL_FAILED_8027  = 0x0206,
    QUERY_EXECUTING_CANCEL_FAILED_8028  = 0x0207,
    QUERY_EXECUTING_CANCEL_FAILED_8029  = 0x0208,
    QUERY_EXECUTING_CANCEL_FAILED_8031  = 0x0209,
//
    QUERY_HOLDING                       = 0x0400,
    QUERY_HOLDING_LOAD                  = 0x0401,
    QUERY_HOLDING_REPREPARING           = 0x0402,
    QUERY_HOLDING_EXECUTING_SQL_CMD     = 0x0403,
    QUERY_HOLDING_BY_RULE               = 0x0404,
    QUERY_HOLDING_BY_ADMIN              = 0x0405,
//
    QUERY_COMPLETED                     = 0x0800,
    QUERY_COMPLETED_HOLD_TIMEOUT        = 0x0801,
    QUERY_COMPLETED_EXEC_TIMEOUT        = 0x0802,
    QUERY_COMPLETED_BY_ADMIN            = 0x0803,
    QUERY_COMPLETED_QUERY_NOT_FOUND     = 0x0804,
    QUERY_COMPLETED_CONNECTION_FAILED   = 0x0805,
    QUERY_COMPLETED_NDCS_PROCESS_FAILED = 0x0806,
    QUERY_COMPLETED_CPU_FAILED          = 0x0807,
    QUERY_COMPLETED_SEGMENT_FAILED      = 0x0808,
    QUERY_COMPLETED_BY_RULE             = 0x0809,
    QUERY_COMPLETED_SERVICE_NOT_ACTIVE  = 0x080A,
    QUERY_COMPLETED_HARDWARE_FAILURE    = 0x080B,
    QUERY_COMPLETED_UNEXPECTED_STATE    = 0x080C,
    QUERY_COMPLETED_CLIENT_DISAPPEARED  = 0x080D,
    QUERY_COMPLETED_BY_CLIENT           = 0x080E,
//
    QUERY_COMPLETED_NDCS_DLG_INIT       = 0x080F,
    QUERY_COMPLETED_NDCS_CONN_IDLE      = 0x0810,
    QUERY_COMPLETED_NDCS_DLG_TERM       = 0x0811,
    QUERY_COMPLETED_NDCS_DLG_BREAK      = 0x0812,
    QUERY_COMPLETED_NDCS_STOP_SRVR      = 0x0813,
    QUERY_COMPLETED_NDCS_RMS_ERROR      = 0x0814,
    QUERY_COMPLETED_NDCS_REPOS_IDLE     = 0x0815,
    QUERY_COMPLETED_NDCS_REPOS_INTERVAL = 0x0816,
    QUERY_COMPLETED_NDCS_REPOS_PARTIAL  = 0x0817,
    QUERY_COMPLETED_NDCS_EXEC_INTERVAL  = 0x0818,
    QUERY_COMPLETED_NDCS_CONN_RULE_CHANGED = 0x0819,
    QUERY_COMPLETED_NDCS_CLOSE          = 0x081A,
    QUERY_COMPLETED_NDCS_PREPARE        = 0x081B,
    QUERY_COMPLETED_NDCS_WMS_ERROR      = 0x081C,
//
    QUERY_REJECTED                      = 0x1000,
    QUERY_REJECTED_BY_ADMIN             = 0x1001,
    QUERY_REJECTED_CONNECTION_FAILED    = 0x1002,
    QUERY_REJECTED_NDCS_PROCESS_FAILED  = 0x1003,
    QUERY_REJECTED_CPU_FAILED           = 0x1004,
    QUERY_REJECTED_SEGMENT_FAILED       = 0x1005,
    QUERY_REJECTED_QMSGCANCELLED        = 0x1006,
    QUERY_REJECTED_VERSION_MISMATCH     = 0x1007,
    QUERY_REJECTED_WMSONHOLD            = 0x1008,
    QUERY_REJECTED_MAX_QUERIES_REACHED  = 0x1009,
    QUERY_REJECTED_SERVICE_NOT_FOUND    = 0x100A,
    QUERY_REJECTED_SERVICE_ON_HOLD      = 0x100B,
    QUERY_REJECTED_BY_RULE              = 0x100C,
    QUERY_REJECTED_UNKNOWNUSER          = 0x100D,
    QUERY_REJECTED_UNEXPECTED_STATE     = 0x100E,
    QUERY_REJECTED_HOLD_TIMEOUT         = 0x100F,
    QUERY_REJECTED_WAIT_TIMEOUT         = 0x1010,
    QUERY_REJECTED_CLIENT_DISAPPEARED   = 0x1011,
    QUERY_REJECTED_LONG_TRANS_ABORTING  = 0x1012,
//
    QUERY_SUSPENDED                     = 0x2000,
    QUERY_SUSPENDED_BY_ADMIN            = 0x2001,
    QUERY_SUSPENDED_BY_RULE             = 0x2002,
    QUERY_SUSPENDED_CANCELED            = 0x2003,
    QUERY_SUSPENDED_CANCELED_BY_ADMIN   = 0x2004,
    QUERY_SUSPENDED_CANCELED_BY_RULE    = 0x2005,
    QUERY_SUSPENDED_CANCELED_BY_TIMEOUT = 0x2007,
//
} QUERY_STATE;

// T2_REPO

#define EXTERNAL_STMT           0
#define INTERNAL_STMT           1

#define ODBC_SRVR_COMPONENT     4

#define NT_VERSION_MAJOR_1  1
#define NT_VERSION_MINOR_0  0
#define NT_BUILD_1          1

#define  SQLERRWARN 1
#define  ESTIMATEDCOSTRGERRWARN 2

#define MAX_PROCESS_NAME_LEN    9

#define INFINITE_SRVR_IDLE_TIMEOUT      -1
#define INFINITE_CONN_IDLE_TIMEOUT      -1

typedef enum _STOP_TYPE
{
    STOP_UNKNOWN = -1,
    STOP_ABRUPT,
    STOP_WHEN_DISCONNECTED,
    STOP_SRVR
} STOP_TYPE;

#define ODBCMX_SERVER "ODBC/MX Server"

#define PROGRAM_ERROR               -11
#define NULL_VALUE_ERROR            "Null Value in a non nullable column"
#define NULL_VALUE_ERROR_SQLCODE    -1
#define NULL_VALUE_ERROR_SQLSTATE   "23000"
#define SQLSVC_EXCEPTION_SMD_STMT_LABEL_NOT_FOUND   "SMD STMT LABEL NOT FOUND"
#define SQLSVC_EXCEPTION_UNSUPPORTED_SMD_DATA_TYPE  "UNSUPPORTED INPUT SMD DATA TYPE"
#define SQLSVC_EXCEPTION_WILDCARD_NOT_SUPPORTED "Wildcard characters in Catalog/Qualifier or Schema/Owner or Table name are not supported"
#define SQLSVC_EXCEPTION_NULL_SQL_STMT  "Null SQL Statement"
#define SQLSVC_EXCEPTION_INVALID_ROW_COUNT  "Invalid Row Count"
#define SQLSVC_EXCEPTION_INVALID_ROW_COUNT_AND_SELECT   "Invalid Row Count and Select Stmt"
#define SQLSVC_EXCEPTION_INVALID_RESOURCE_OPT_CLOSE "Invalid Resource Option for Close"
#define SQLSVC_EXCEPTION_INVALID_TRANSACT_OPT   "Invalid TransactOpt"
#define SQLSVC_EXCEPTION_INVALID_OPTION_VALUE_NUM   "Invalid OptionValueNum"
#define SQLSVC_EXCEPTION_INVALID_OPTION_VALUE_STR   "Invalid OptionValueStr (too big)"
#define SQLSVC_EXCEPTION_INVALID_CONNECTION_OPTION  "Invalid ConnectionOpt"
#define SQLSVC_EXCEPTION_PREPARE_FAILED "Prepare Failed"
#define SQLSVC_EXCEPTION_EXECUTE_FAILED "Execute Failed"
#define SQLSVC_EXCEPTION_EXECDIRECT_FAILED  "ExecDirect Failed"
#define SQLSVC_EXCEPTION_EXECSPJRS_FAILED   "ExecSPJRS Failed"
#define SQLSVC_EXCEPTION_CLOSE_FAILED   "Close Failed"
#define SQLSVC_EXCEPTION_FETCH_FAILED   "Fetch Failed"
#define SQLSVC_EXCEPTION_CANCEL_FAILED  "Cancel Failed"
#define SQLSVC_EXCEPTION_ENDTRANSACTION_FAILED  "EndTransaction Failed"
#define SQLSVC_EXCEPTION_SETCONNECTOPTION_FAILED    "SetConnectionOption Failed"
#define SQLSVC_EXCEPTION_DATA_ERROR  "Input Parameter/Output Data Error"
#define SQLSVC_EXCEPTION_USER_CATALOG_NAME_ERROR "NULL Value in guardian name in mapping table"
#define SQLSVC_EXCEPTION_CATSMD_MODULE_ERROR "The Catalog SMD file is either corrupted or not found or cursor not found"
#define SQLSVC_EXCEPTION_BUFFER_ALLOC_FAILED "Buffer Allocation Failed"
#define SQLSVC_EXCEPTION_INVALID_HANDLE "Error while allocating Handles in SQL/MX"
#define SQLSVC_EXCEPTION_PREPARE_FAILED "Error while preparing the query"
#define SQLSVC_EXCEPTION_INVALID_SCHEMA_VERSION "Invalid Schema version"  // Used for Metadata schemaVersion setup


#define  SQLERRWARN 1
#define  ESTIMATEDCOSTRGERRWARN 2

#define CATALOG_NT                  0
#define CATALOG_ANSI                1
#define CATALOG_SHORTANSI           2


// ODBC_SERVER_ERROR should be different than the possible values of SQLRETURN as defined in SQLTypes.h
// Else there may be overlap between SQLRETRUN values and this value
// Same is the case with ODBC_RG_ERROR and ODBC_RG_WARNING
#define ODBC_SERVER_ERROR           -101
#define ODBC_RG_ERROR               -102
#define ODBC_RG_WARNING             -103
#define SQL_RETRY_COMPILE_AGAIN     -104
#define SQL_QUERY_CANCELLED         -105
#define CANCEL_NOT_POSSIBLE         -106
#define SQL_RS_DOES_NOT_EXIST       -108

#define TYPE_UNKNOWN                0x0000
#define TYPE_SELECT                 0x0001
#define TYPE_UPDATE                 0x0002
#define TYPE_DELETE                 0x0004
#define TYPE_INSERT                 0x0008
#define TYPE_EXPLAIN                0x0010
#define TYPE_CREATE                 0x0020
#define TYPE_GRANT                  0x0040
#define TYPE_DROP                   0x0080
#define TYPE_CALL                   0x0100
#define TYPE_INSERT_PARAM           0x0120 //Added for CQDs filter

typedef enum _SRVR_TYPE
{
    SRVR_UNKNOWN = 0,
    AS_SRVR,
    CORE_SRVR,
    CFG_SRVR
} SRVR_TYPE;

typedef enum SRVR_STATE
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

struct VERSION_t {
    IDL_short componentId;
    IDL_short majorVersion;
    IDL_short minorVersion;
    char pad_to_offset_8_[2];
    IDL_unsigned_long buildId;
};
typedef VERSION_t VERSION_def;

typedef struct _SRVR_GLOBAL_Def
{
    // +++ T2_REPO TODO - Temporarily assignment...needs to come from properties
    _SRVR_GLOBAL_Def()
    {
        m_bStatisticsEnabled = true;
        m_statisticsPubType = STATISTICS_AGGREGATED;
        m_iAggrInterval = STATISTICS_INTERVAL;
        m_iQueryPubThreshold = STATISTICS_LIMIT;
        resourceStatistics = 0;
    }

    long                dialogueId;
    char                userSID[MAX_TEXT_SID_LEN+1];// This points to the SID in which the server is running
    // If this is same as the incoming SID do not flush the
    //  cache
    tip_handle_t        tip_gateway;
    char                *pxid_url;
    long long           local_xid;
    bool                failOverEnabled;
    long                clientACP;
    long                clientErrorLCID;
    long                clientLCID;
    bool                useCtrlInferNCHAR;
    char                DefaultCatalog[129];
    char                DefaultSchema[129];
    char                CurrentCatalog[129]; // Added for MFC
    char                CurrentSchema[129];  // Added for MFC
    bool                jdbcProcess;        // This flag is used to determine the query for SQLTables
    char                SystemCatalog[129]; // MX system catalog name
    short               boolFlgforInitialization; // Flag intorduced for Connect/disconnect imp.

// +++ T2_REPO
    long            resourceStatistics;
    bool            m_bStatisticsEnabled;
    statistics_type m_statisticsPubType;
    int             m_iAggrInterval;
    int             m_iQueryPubThreshold;

    // +++ T2_REPO  TODO - These need to be initialized
    int     isoMapping;
    long    process_id;
    char    m_ProcName[MS_MON_MAX_PROCESS_NAME];
    int     receiveThrId;
    int     m_NodeId;
    char    IpAddress[MAX_IP_ADDRESS_LEN];
    char    sessionId[SESSION_ID_LEN];
    long    userID;
    char    QSRoleName[MAX_ROLE_LEN + 1];
    char    ClientComputerName[MAX_COMPUTER_NAME_LEN + 1];
    char    ApplicationName[MAX_APPLICATION_NAME_LEN + 1];
    
    // Used by rowsets logic - need to be initialized
    long                m_FetchBufferSize;
    bool                fetchAhead;
    VERSION_def         srvrVersion;
    VERSION_def         sqlVersion;
    VERSION_def         drvrVersion;
    VERSION_def         appVersion;
    Int64               maxRowsFetched; // Not sure will this be useful in the future, just keep it.
    bool                enableLongVarchar;
//
} SRVR_GLOBAL_Def ;

// These defines should be same as what is in DrvrSrvr.h of ODBC
#define SET_CATALOG                 1000
#define SET_SCHEMA                  1001
#define SET_LOCATION                1002
#define SET_INFER_NCHAR             1003
#define RESET_DEFAULTS              1004
#define SET_ODBC_PROCESS            1005
#define SET_CATALOGNAMETYPE         1006
#define SET_SETANDCONTROLSTMTS      1007
#define SET_NAMETYPE                1008
#define SET_JDBC_PROCESS            1009
#define SET_MPLOC                   1010
#define SET_OLT_QUERY_OPT           1011
#define CLEAR_CATALOG               1012
#define CLEAR_SCHEMA                1013
#define CQD_DOOM_USER_TXN           1014
#define CQD_PCODE_OFF               1015
#define BEGIN_SESSION               1016
#define END_SESSION                 1017

typedef struct tagDATE_TYPE
{
    unsigned short      year;
    unsigned char       month;
    unsigned char       day;
} DATE_TYPES;

typedef struct tagTIME_TYPE
{
    unsigned char       hour;
    unsigned char       minute;
    unsigned char       second;
} TIME_TYPES;

#define SQLCLI_ODBC_VERSION 2
#define SQLCLI_ODBC_MODULE_VERSION 1


// Errors returned from SQL_EXEC_AllocStmtForRS()
#define STMT_ALREADY_EXISTS     -8802
#define STMT_DOES_NOT_EXIST     -8804
#define STMT_IS_NOT_CALL        -8909
#define RS_INDEX_OUT_OF_RANGE   -8910
#define RS_ALREADY_EXISTS       -8911
#define RS_ALLOC_ERROR          -8912
#define RS_PREPARE_NOT_ALLOWED  -8913
#define RS_REOPEN_NOT_ALLOWED   -8914
// Errors returned from SQL_EXEC_DescribeStmt or SQL_EXEC_Exec
#define RS_DOES_NOT_EXIST       -8915

#define STMT_ID_NULL_ERROR      -29400
#define DIALOGUE_ID_NULL_ERROR  -29401
#define STMT_ID_MISMATCH_ERROR  -29402
#define HOLD_CURSORS_OVER_COMMIT    1
#define CLOSE_CURSORS_AT_COMMIT     2

#define TYPE_BLOB               2004
#define TYPE_CLOB               2005
#define CLOB_HEADING            "JDBC_CLOB_COLUMN -"
#define BLOB_HEADING            "JDBC_BLOB_COLUMN -"
#define INVALID_SQL_QUERY_STMT_TYPE 255

typedef struct {
    //unsigned long _length; 64 change
    unsigned int _length;
    unsigned char *_buffer;
} SQL_DataValue_def;

typedef struct {
    long dataType;
    short dataInd;
    SQL_DataValue_def dataValue;
    long dataCharset;
} SQLValue_def;

typedef struct {
    //unsigned long _length; 64 bit change
    unsigned int _length;
    SQLValue_def *_buffer;
} SQLValueList_def;

typedef struct tag {
    long version;
    long dataType;
    long datetimeCode;
    long maxLen;
    short precision;
    short scale;
    long vc_ind_length;
    unsigned char nullInfo;
    char colNm[129];
    char colLabel[129];
    unsigned char signType;
    long ODBCDataType;
    short ODBCPrecision;
    long SQLCharset;
    long ODBCCharset;
    char catalogNm[129];
    char schemaNm[129];
    char tableNm[129];
    long fsDataType;
    long intLeadPrec;
    long paramMode;
} SQLItemDesc_def;

typedef struct SQLItemDescList_def_seq_ {
    unsigned long _length;
    SQLItemDesc_def *_buffer;
} SQLItemDescList_def;

typedef struct {
    long rowId;
    long errorDiagnosticId;
    long sqlcode;
    char sqlstate[6];
    char *errorText;
    long operationAbortId;
    long errorCodeType;
    char *Param1;
    char *Param2;
    char *Param3;
    char *Param4;
    char *Param5;
    char *Param6;
    char *Param7;
} ERROR_DESC_def;

typedef struct {
    unsigned long _length;
    ERROR_DESC_def *_buffer;
} ERROR_DESC_LIST_def;

typedef struct {
    char AttrNm[129];
    long long Limit;
    char Action[129];
    long long ActualValue;
} RES_HIT_DESC_def;

struct odbc_SQLSvc_SQLError { /* Exception */
    ERROR_DESC_LIST_def errorList;
};

typedef struct {
    unsigned int _length;
    int *_buffer;
} ROWS_COUNT_LIST_def;

typedef struct {
    long contents[4];
} CEE_handle_def;

struct odbc_SQLSvc_ParamError { /* Exception */
    char *ParamDesc;
};

struct odbc_SQLSvc_SQLQueryCancelled { /* Exception */
    long sqlcode;
};

struct odbc_SQLSvc_SQLInvalidHandle { /* Exception */
    long sqlcode;
};

struct odbc_SQLSvc_SQLRetryCompile { /* Exception */
    ERROR_DESC_LIST_def errorList;
};

struct ExceptionStruct {
    long exception_nr;
    long exception_detail;
    union {
        odbc_SQLSvc_ParamError ParamError;
        odbc_SQLSvc_SQLError SQLError;
        odbc_SQLSvc_SQLInvalidHandle SQLInvalidHandle;
        odbc_SQLSvc_SQLRetryCompile SQLRetryCompile;
        odbc_SQLSvc_SQLQueryCancelled SQLQueryCancelled;
    } u;
};

// Added for MFC
inline char* strToUpper(char* str)
{
    char* pTemp = str;
    long i;
    long len = strlen(str);
    long single_quotes = 0;
    long double_quotes = 0;

    for (i=0; i<len; i++)
    {
        switch(pTemp[i])
        {
        case '\'':
            single_quotes++;
            break;
        case '\"':
            double_quotes++;
            break;
        default:
            if (single_quotes % 2 == 0 && double_quotes %2 == 0)
                pTemp[i] = toupper(pTemp[i]);
            break;
        }
    }
    return(pTemp);
}


#define CEE_SUCCESS ((long) 0)

/*
// Linux port - ToDo
// Added dummy functions for transaction support (originally in pThreadsSync.cpp - we ar enot including that)
// Changing the method name (capitalize first letter) since SQL already has functions with same name results in a multiple definitions error
short BeginTransaction(long *transTag)
{
    return 0;
}

short resumeTransaction(long transTag)
{
    return 0;
}

short endTransaction(void)
{
    return 0;
}

short abortTransaction(void)
{
    return 0;
}
*/

#include "SrvrOthers.h"

#endif
