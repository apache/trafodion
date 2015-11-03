/**************************************************************************
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
// MODULE: CSrvrStmt.h
//
// PURPOSE: Defines CSrvrStmt class
//
#ifndef _CSRVRSTMT_DEFINED
#define _CSRVRSTMT_DEFINED
#include <platform_ndcs.h>
#ifdef NSK_PLATFORM
#include <sqlWin.h>
#else
#include <sql.h>
#endif
#include <sqlext.h>
#include "sqlcli.h"
#include "CoreCommon.h"
#include "CDesc.h"
#include "jni.h"

//#include "spthread.h" commented by venu for TSLX
#define     UNKNOWN_METHOD          -1
#define     Prepare_From_Module     20  // PrepareFromModule Method Operation Index
// addition to SrvrOthers.h indexes, which stop at 19
#define     MAX_RESULT_SETS         255 // Max number of RS per stmt

#define     UNKNOWN_METHOD              -1

#define     ROWSET_NOT_DEFINED          0

#define     DEFAULT_ROWSET_SIZE         1

typedef struct ROWSET_ERROR_NODE {
    ROWSET_ERROR_NODE(){
        rowNumber = 0;
        SQLError.errorList._length = 0;
        SQLError.errorList._buffer = NULL;
        nextNode = NULL;
    }
    Int32 rowNumber;
    odbc_SQLSvc_SQLError SQLError;
    ROWSET_ERROR_NODE* nextNode;
} ROWSET_ERROR_NODE;

typedef struct ROWSET_ERROR_LIST {
    ROWSET_ERROR_LIST(){
        nodeCount = 0;
        firstNode = NULL;
    }
    UInt32 nodeCount;
    ROWSET_ERROR_NODE* firstNode;
} ROWSET_ERROR_LIST;


class SRVR_STMT_HDL {
public:
    enum DESC_TYPE {
        Input,
        Output
    };
    short                   stmtType;                         // Used to indecate internal or external statment
    char                    cursorName[MAX_CURSOR_NAME_LEN+1];
    short                   cursorNameLen;
    char                    previousCursorName[MAX_CURSOR_NAME_LEN+1];
    char                    stmtName[MAX_STMT_NAME_LEN+1];
    int                     paramCount; /*Venu changed the following two variables from long to int for 64 bit porting */
    int                     columnCount;
    SQLSTMT_ID              stmt;
    SQLDESC_ID              inputDesc ;
    SQLDESC_ID              outputDesc;
    BYTE                    *inputDescVarBuffer;        // Data Buffer for input values
    BYTE                    *outputDescVarBuffer;       // Data Buffer for output values
    Int32                   inputDescVarBufferLen;       // Might need it as long in the future?
    Int32                   outputDescVarBufferLen;      // Might need it as long in the future?
    BOOL                    isReadFromModule;
    jobject                 resultSetObject;
    BOOL                    endOfData;
    BOOL                    isSPJRS;                        // RS Query Stmt Type (e.g. SQL_CALL_NO_RESULT_SETS, SQL_CALL_WITH_RESULT_SETS)
    long                    RSIndex;                        // Current index into RSArray[]
    long                    RSMax;

    // Control Values
    short                   currentMethod;
    HANDLE                  asyncThread;
    HANDLE                  queryTimeoutThread;
    DWORD                   threadStatus;
    DWORD                   threadId;
    SQLRETURN               threadReturnCode;

    // Input Values
    short                   sqlAsyncEnable;
    long                    queryTimeout;
    SQLValue_def            sqlString;
    char                    *sqlStringText;
    Int32                   maxRowCnt;
    Int32                   maxRowLen;
    long                    inputRowCnt;
    short                   sqlStmtType;
    unsigned short          freeResourceOpt;
    SQLValueList_def        inputValueList;

    Int32                   inputDescBufferLength;
    BYTE                    *inputDescBuffer;
    Int32                   outputDescBufferLength;
    BYTE                    *outputDescBuffer;
    Int32                   sqlWarningOrErrorLength;
    BYTE                    *sqlWarningOrError;
    Int32                   delayedSqlWarningOrErrorLength;
    BYTE                    *delayedSqlWarningOrError;

    // child query visibility
    Int32                   sqlSubQueryType;

    Int32                   maxRowsetSize;
    short                   RowsetFetchRetcode;
    short                   PerfFetchRetcode;

    bool                    sqlBulkFetchPossible;
    bool                    bFirstSqlBulkFetch;

    // output Values
    long                    estimatedCost;
    int                     rowsAffected;
    Int32                   rowsAffectedHigherBytes; // Combine both rowsAffected and rowsAffectedHigherBytes as __int64 when interface between drvr/srvr changes
    Int32                   delayedRowsAffected;
    
    SQLItemDescList_def     inputDescList;
    SQLItemDescList_def     outputDescList;
    RES_HIT_DESC_def        rgPolicyHit;
    ERROR_DESC_LIST_def     sqlWarning;
    odbc_SQLSvc_SQLError    sqlError;
    SQLValueList_def        outputValueList;
    BYTE                    *outputValueVarBuffer;
    BYTE                    *inputValueVarBuffer;
    ROWS_COUNT_LIST_def     rowCount;
    long                    totalRowCount;
    // language (character set) specific variables
    unsigned long           clientLCID;
    SQLMODULE_ID            moduleId;       // Should we keep it separately
    char                    moduleName[MAX_MODULE_NAME_LEN+1];
    char                    inputDescName[MAX_DESC_NAME_LEN+1];
    char                    outputDescName[MAX_DESC_NAME_LEN+1];
    char                    isClosed;
    SRVR_DESC_HDL           *IPD;
    SRVR_DESC_HDL           *IRD;
    BOOL                    useDefaultDesc;
    long                    dialogueId;
    long                    nowaitRetcode;
    short                   holdability;
    long                    fetchQuadEntries;
    long                    fetchRowsetSize;
    struct SQLCLI_QUAD_FIELDS *fetchQuadField;
    long                    batchQuadEntries;
    long                    batchRowsetSize;
    long                    batchMaxRowsetSize;
    long                    inputDescParamOffset;
    struct SQLCLI_QUAD_FIELDS *batchQuadField;

    /*
    // Rowsets and SPJ control variables
    SQLSTMT_ID      *callStmtId;     // If this is an SPJ result set, then its CALL statement
    Int32           resultSetIndex;
    */
    ROWSET_ERROR_LIST       rowsetErrorList;
    Int32                   estRowLength;

    bool                    NA_supported;
    bool                    bSQLMessageSet;
    bool                    bSQLValueListSet;

    bool                    bFetchStarted;

    bool                           preparedWithRowsets;  
    struct SQLCLI_QUAD_FIELDS      *inputQuadList;
    struct SQLCLI_QUAD_FIELDS      *inputQuadList_recover;
    BYTE                           *transportBuffer;
    Int32                          transportBufferLen;

    struct SQLCLI_QUAD_FIELDS      *outputQuadList;
    BYTE                           *outputBuffer;
    Int32                          outputBufferLength;
    Int32                   returnCodeForDelayedError; // for the hash driver we need to save the returnCode

    // Added for MODE_SPECIAL_1 behavior
    Int32 numErrRows;
    BYTE *errRowsArray;

    // The following elements apply to SPJ result sets
    Int32                            numResultSets;  // If this is CALL statement, then the number of RS for this invocation
    SRVR_STMT_HDL                   *previousSpjRs;  // If this is SPJ RS, then the previous SPJ RS or CALL statement.
    SRVR_STMT_HDL                   *nextSpjRs;      // If this is CALL statement or SPJ RS, then the next SPJ RS
    SQLSTMT_ID                      *callStmtId;     // If this is an SPJ result set, then its CALL statement
    SRVR_STMT_HDL                   *callStmtHandle; // If this is an SPJ result set, then its CALL statement handle
    Int32                            resultSetIndex;

    char *SpjProxySyntaxString;    // SPJ Proxy Syntax String
    Int32  SpjProxySyntaxStringLen; // SPJ Proxy Syntax String Len
    //--------------------------------------------------------------------------------

    int64   m_curRowsFetched;

    //---------------------------------------------------------------------------------
    DESC_HDL_LISTSTMT   *SqlDescInfo;
    Int32   current_holdableCursor;
    Int32   holdableCursor;

    bool    reprepareWarn;

    SQL_DataValue_def outputDataValue;
    SQL_DataValue_def inputDataValue;
    SQL_DataValue_def delayedOutputDataValue;

    IDL_long    myKey;

    // +++ T2_REPO
    char                    sqlUniqueQueryID[MAX_QUERY_NAME_LEN + 1];
    SQL_QUERY_COST_INFO     cost_info;
    SQL_QUERY_COMPILER_STATS_INFO comp_stats_info;
    unsigned short  m_state;        //state + substate
    int64           m_wait_time;    //in seconds
    int64           m_hold_time;    //in seconds
    int64           m_suspended_time;//in seconds
    int64           m_exec_time;    //in seconds
    int64           m_WMSstart_ts;  //timestamp
    unsigned short  m_warnLevel;
    int64           queryStartTime;
    int64           queryEndTime;
    int64           queryStartCpuTime;
    int64           queryEndCpuTime;
    Int32           sqlNewQueryType;
    // Connection, Compilation, Execution rules
    //
    //
    // rules - size is MAX_RULE_NAME_LEN + ':' + '99' or 'ALL' + '\0'
    //
    char m_con_rule_name[MAX_RULE_NAME_LEN + 1 + 3 + 1];
    char m_cmp_rule_name[MAX_RULE_NAME_LEN + 1 + 3 + 1];
    char m_exe_rule_name[MAX_RULE_NAME_LEN + 1 + 3 + 1];
    NDCS_SUBSTATE   m_mxsrvr_substate;
    bool            bLowCost;
    char*           pSqlString;
    UInt32          sqlStringLen;
    bool            m_bNewQueryId;  // +++ Has to be set right after a prepare
    char            m_shortQueryText[RMS_STORE_SQL_SOURCE_LEN + 1];
    long            m_rmsSqlSourceLen;
    short           stmtNameLen;
    bool            m_need_21036_end_msg;
    int64           m_lastQueryEndTime;
    int64           m_lastQueryEndCpuTime;
    bool            m_bqueryFinish;
    Int32           inState;
    Int32           sqlQueryType;
    UInt32          m_maxMemUsed;
    Int32           sqlUniqueQueryIDLen;

    enum EX_PLAN_STATUS { UNAVAILABLE, COLLECT, COLLECTED, STORED };
    EX_PLAN_STATUS exPlan;
    char *sqlPlan;
    UInt32 sqlPlanLen;

    // T2_REPO

    bool isISUD;
    BOOL                    stmtInitForNowait;
    inline void setSqlQueryStatementType (int Type) {
        SqlQueryStatementType = Type;
    };

    inline int getSqlQueryStatementType (void) {
        return (SqlQueryStatementType);
    };

private:
    //message buffers between JAVA layer and native layer
    char* m_wbuffer;
    int   m_wbuffer_length;
    char* m_rbuffer;
    int   m_rbuffer_length;

public:
    SRVR_STMT_HDL();
    SRVR_STMT_HDL(long dialogueId);
    ~SRVR_STMT_HDL();
    //SRVR_STMT_HDL(const char *inStmtLabel, const char *moduleName);
    SQLRETURN Prepare(const SQLValue_def *inSqlString, short inStmtType, short holdability, long inQueryTimeout,bool isISUD = FALSE);
    // MFC
    //SQLRETURN PrepareforMFC(const SQLValue_def *inSqlString, short inStmtType, short holdability, long inQueryTimeout, bool isISUD = FALSE);
    SQLRETURN Execute(const char *inCursorName, long inputRowcnt, short sqlStmtType,
        const SQLValueList_def *inputValueList,short inSqlAsyncEnable, long inQueryTimeout,
        SQLValueList_def *outputValueList);
    // Rowsets
    SQLRETURN Execute(const char *inCursorName, IDL_long inInputRowCnt, IDL_short inSqlStmtType,
            const SQLValueList_def *inValueList, short inSqlAsyncEnable, Int32 inQueryTimeout);

    SQLRETURN Close(unsigned short inFreeResourceOpt);
    SQLRETURN Fetch(long inMaxRowCnt, short inSqlAsyncEnable, long inQueryTimeout);
    SQLRETURN ExecDirect(const char *inCursorName, const SQLValue_def *inSqlString, short inStmtType, short inSqlStmtType,
        short holdability, long inQueryTimeout);
    SQLRETURN ExecSPJRS();
    SQLRETURN Cancel();
    void cleanupSQLMessage();
    void cleanupSQLValueList();
    void cleanupSQLDescList();
    void cleanupAll();
    SQLRETURN PrepareFromModule(short stmtType);
    SQLRETURN InternalStmtClose(unsigned short inFreeResourceOpt);
    SQLRETURN freeBuffers(short descType);
    void processThreadReturnCode(void);
    SQLRETURN allocSqlmxHdls(const char *stmtName
            , const char *moduleName
            , long long moduleTimestamp
            , long moduleVersion
            , short sqlStmtType
            , BOOL useDefaultDesc
            , const char *inInputDescName = NULL
            , const char *inOutputDescName = NULL);
    SQLRETURN allocSqlmxHdls_spjrs(SQLSTMT_ID *callpstmt, const char *stmtName, const char *moduleName, long long moduleTimestamp,
        long moduleVersion, short sqlStmtType, BOOL useDefaultDesc, long RSindex, const char * RSstmtName);
    SQLRETURN ExecuteCall(const SQLValueList_def *inValueList,short inSqlAsyncEnable,
        long inQueryTimeout);
    SQLRETURN switchContext();
    SQLCTX_HANDLE getContext();
    void resetFetchSize(long fetchSize);
    static void resetFetchSize(long dialogueId, long stmtId, long fetchSize);
    void prepareSetup(void);
    void batchSetup(long statementCount);
    SQLRETURN setMaxBatchSize(long maxRowsetSize);
    SRVR_DESC_HDL *allocImplDesc(DESC_TYPE descType);
    SRVR_DESC_HDL *getImplDesc(DESC_TYPE descType);
    long *getDescBufferLenPtr(DESC_TYPE descType);
    long getDescEntryCount(DESC_TYPE descType);
    long getQuadEntryCount(DESC_TYPE descType);
    SQLDESC_ID *getDesc(DESC_TYPE descType);
    SQLItemDescList_def *getDescList(DESC_TYPE descType);
    BYTE **getDescVarBufferPtr(DESC_TYPE descType);
    long getRowsetSize(DESC_TYPE descType);
    struct SQLCLI_QUAD_FIELDS *getQuadField(DESC_TYPE descType);

    //message buffers between JAVA layer and native layer
    char*    w_allocate(size_t);
    char*    r_allocate(size_t);
    char*    w_assign(char*, int);
    char*    r_assign(char*, int);
    void     w_release();
    void     r_release();
    char*    w_buffer();
    char*    r_buffer();
    int      w_buffer_length();
    int      r_buffer_length();

private:
    int             SqlQueryStatementType;             // This is used to indecate the type of SQL query statment (SELECT, INSERT, UPDATE, ...)
};

#endif
