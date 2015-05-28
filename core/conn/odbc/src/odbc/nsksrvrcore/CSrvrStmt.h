/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
********************************************************************/
//
// MODULE: CSrvrStmt.h
//
// PURPOSE: Defines CSrvrStmt class
//

#ifndef _CSRVRSTMT_DEFINED
#define _CSRVRSTMT_DEFINED

#include <platform_ndcs.h>
//#include <winbase.h>
#include <sql.h>
#include <sqlext.h>
#include "sqlcli.h"
#include "Global.h"
#include "cee.h"
#include "odbcCommon.h"
#include "odbcsrvrcommon.h"
#include "odbc_sv.h"
#include "DrvrSrvr.h"
#include "QSInOutParam.h"
typedef void (*SrvrflushQueryHandle) (Long);

#define		UNKNOWN_METHOD				-1

#define		ROWSET_NOT_DEFINED			0

#define		DEFAULT_ROWSET_SIZE			1

#define		Allocate_Statement_Handle	0

#define		Fetch_Catalog_Rowset		99

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
	short					stmtType;
	short					cursorNameLen;
	char					cursorName[MAX_CURSOR_NAME_LEN+1];
	char					previousCursorName[MAX_CURSOR_NAME_LEN+1];
	short					stmtNameLen;
	char					stmtName[MAX_STMT_NAME_LEN+1];
	Int32					paramCount;
	Int32					columnCount;
	SQLSTMT_ID				stmt;
	SQLDESC_ID				inputDesc ;
	SQLDESC_ID				outputDesc;
	BYTE					*inputDescVarBuffer;		// Data Buffer for input values
	Int32					inputDescVarBufferLen;
	Int32					inputDescIndBufferLen;
	BYTE					*outputDescVarBuffer;		// Data Buffer for output values
	Int32					outputDescVarBufferLen;
	Int32					outputDescIndBufferLen;
	BOOL					isReadFromModule;

	// Control Values
	short					currentMethod;
	HANDLE 					asyncThread;
	HANDLE					queryTimeoutThread;
	DWORD					threadStatus;
	DWORD					threadId;
	SQLRETURN				threadReturnCode;

	// Input Values
	short					sqlAsyncEnable;
	Int32					queryTimeout;
	UInt32			sqlStringLen;
	char					*sqlString;
	Int32					inputRowCnt;
	Int32					maxRowCnt;
	Int32					maxRowLen;
	short					sqlStmtType;
	Int32					sqlQueryType;
	Int32					inputDescBufferLength;
	BYTE					*inputDescBuffer;
	Int32					outputDescBufferLength;
	BYTE					*outputDescBuffer;
	Int32					sqlWarningOrErrorLength;
	BYTE					*sqlWarningOrError;
	Int32					delayedSqlWarningOrErrorLength;
	BYTE					*delayedSqlWarningOrError;
	// child query visibility
	Int32					sqlSubQueryType;

	// The below has been added to avoid too many code changes to support the new
	// SQL_EXE_UTIL query type(for MAINTAIN, REORG etc.), which needs to be treated 
	// similar to SQL_SELECT_NON_UNIQUE. Only used in 
	// QSInterfaceDrvr.cpp->qrysrvc_WouldLikeToExecute() for WMS.
	Int32					sqlNewQueryType;
	bool					bLowCost; // Based on DSN setting for cost - Skip generating 21036 message if cost/time is less

	__int64					cliStartTime;
	__int64					cliEndTime;
	__int64					cliElapseTime;
	bool					sqlBulkFetchPossible;
	bool					bFirstSqlBulkFetch;

	Int32					sqlUniqueQueryIDLen;
	char					sqlUniqueQueryID[MAX_QUERY_NAME_LEN + 1]; // Since measure can accept up to 776 bytes.
	unsigned short				freeResourceOpt;
	SQLValueList_def			inputValueList;
	Int32					maxRowsetSize;
	short					RowsetFetchRetcode;
	short					PerfFetchRetcode;

	// output Values
	SQL_QUERY_COST_INFO		cost_info;
	SQL_QUERY_COMPILER_STATS_INFO comp_stats_info;
//
	Int32					rowsAffected;
	Int32					rowsAffectedHigherBytes; // Combine both rowsAffected and rowsAffectedHigherBytes as __int64 when interface between drvr/srvr changes
	Int32					delayedRowsAffected;
	SQLItemDescList_def		inputDescList;
	SQLItemDescList_def		outputDescList;
	RES_HIT_DESC_def		rgPolicyHit;
	ERROR_DESC_LIST_def		sqlWarning;
	odbc_SQLSvc_SQLError	sqlError;
	SQLValueList_def		outputValueList;
	BYTE					*outputValueVarBuffer;
	BYTE					*inputValueVarBuffer;
// language (character set) specific variables
	UInt32			clientLCID;
	SQLMODULE_ID			moduleId;		// Should we keep it separately
	char					moduleName[MAX_MODULE_NAME_LEN+1];
	char					inputDescName[MAX_DESC_NAME_LEN+1];
	char					outputDescName[MAX_DESC_NAME_LEN+1];
	char					isClosed;
	SRVR_DESC_HDL			*IPD;
	SRVR_DESC_HDL			*IRD;
// for Genus
	SQLHANDLE				GN_stmtDrvrHandle;
	Int32					GN_maxRowsetSize;
	Int32					GN_currentRowsetSize;
	struct SQLCLI_QUAD_FIELDS *GN_SelectParamQuadFields;
// for NONATOMIC rowset recovery
// false if after prepare we get following error codes:
// 30026, 30028, 30033, 30034 and 30029 ? // Based on SQL group input disabling the 30027 check and make it RFE in Executor. Enable it back once Executor supports it.
	bool					NA_supported;
//
	ROWSET_ERROR_LIST		rowsetErrorList;
	Int32					estRowLength;

	bool					bSQLMessageSet;
	bool				bSQLValueListSet;

	bool                            preparedWithRowsets;  
	struct SQLCLI_QUAD_FIELDS      *inputQuadList;
	struct SQLCLI_QUAD_FIELDS      *inputQuadList_recover;
	BYTE                           *transportBuffer;
	Int32                            transportBufferLen;
        
	struct SQLCLI_QUAD_FIELDS      *outputQuadList;
	BYTE                           *outputBuffer;
	Int32                            outputBufferLength;
    Int32                    returnCodeForDelayedError; // for the hash driver we need to save the returnCode
	bool					bFetchStarted;

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

	enum EX_PLAN_STATUS { UNAVAILABLE, COLLECT, COLLECTED };
	EX_PLAN_STATUS exPlan;
	char *sqlPlan;
	UInt32 sqlPlanLen;

	char *SpjProxySyntaxString;    // SPJ Proxy Syntax String
	Int32  SpjProxySyntaxStringLen; // SPJ Proxy Syntax String Len
//--------------------------------------------------------------------------------
//
//  adaptive segment has been allocated by SQLPrepare
//
	bool	m_isAdaptiveSegmentAllocated;
	bool	m_need_21036_end_msg;
	short	m_adaptive_segment;

	unsigned long	m_internal_Id;		// random number to identify the query in WMS
	short	m_aggPriority;				// used for release aggregated queries
	bool	m_bSkipWouldLikeToExecute; // for this statement do not call WouldLikeToExecute
	bool	m_bDoneWouldLikeToExecute; // true when WouldLikeToexecute done on this statement
//
//  QSCom conversion of virtual SELECTs statements to not use SQL.
	void*	m_result_set;
//
// WMS can limit number of fetched rows.
//
	int64	m_curRowsFetched;
	Int32	inState;

	bool	m_bNewQueryId;
	char	m_shortQueryText[RMS_STORE_SQL_SOURCE_LEN + 1];
	long	m_rmsSqlSourceLen;

//
//---------------------------------------------------------------------------------
	DESC_HDL_LISTSTMT	*SqlDescInfo;
	Int32	current_holdableCursor;
	Int32 	holdableCursor;
//
// Added for "Single Row Per Query" project
//
	unsigned short	m_state;		//state + substate
	int64			m_wait_time;	//in seconds
	int64			m_hold_time;	//in seconds
	int64			m_suspended_time;//in seconds
	int64			m_exec_time;	//in seconds
	int64			m_WMSstart_ts;	//timestamp
	unsigned short	m_warnLevel;
	UInt32	m_maxMemUsed;
	int64		queryStartTime;
	int64		queryEndTime;
	int64		queryStartCpuTime;
	int64		queryEndCpuTime;

	bool			m_bqueryFinish;
//
// Connection, Compilation, Execution rules
//
//
// rules - size is MAX_RULE_NAME_LEN + ':' + '99' or 'ALL' + '\0'
//
	char m_con_rule_name[MAX_RULE_NAME_LEN + 1 + 3 + 1];
	char m_cmp_rule_name[MAX_RULE_NAME_LEN + 1 + 3 + 1];
	char m_exe_rule_name[MAX_RULE_NAME_LEN + 1 + 3 + 1];

	EXEC_OVERFLOW m_execOverflow; //ssd overflow

	int64 m_suspended_ts;
	int64 m_released_ts;
	int64 m_cancelled_ts;

//
// AGGREGATION
//
//
	bool	m_query_rejected;
//
//
	int64	m_lastQueryStartTime;
	int64	m_lastQueryEndTime;
	int64	m_lastQueryStartCpuTime;
	int64	m_lastQueryEndCpuTime;
//
	NDCS_SUBSTATE m_mxsrvr_substate;
//
// to flush query stmt
//
	SrvrflushQueryHandle m_flushQuery;
//
// For setting special query for WMS
	unsigned long querySpl;
//
	bool	m_pertable_stats;	// true - WMS wrote perTableStats for this query.
//
// 	Added for 64-bit work
	static IDL_long globalKey;
	IDL_long myKey;

//	
// An earlier fix had caused a regression where the warnings after a re-prepare was getting deleted during execute 
// in turn causing the NAR tests to fail. Workaround now is to add a flag to indicate if that a warning occurred during rePrepare
// and should not be deleted.
	bool reprepareWarn;
//
public:
	SRVR_STMT_HDL();
	~SRVR_STMT_HDL();
	SQLRETURN Prepare(const char *inSqlString, short inStmtType, short inSqlAsyncEnable, Int32 inQueryTimeout);
	SQLRETURN Execute(const char *inCursorName, IDL_long inputRowcnt, IDL_short sqlStmtType, 
		const SQLValueList_def *inputValueList,short inSqlAsyncEnable, Int32 inQueryTimeout);
	SQLRETURN Close(unsigned short inFreeResourceOpt);
	SQLRETURN Fetch(Int32 inMaxRowCnt, Int32 inMaxRowLen, short inSqlAsyncEnable, Int32 inQueryTimeout);
	SQLRETURN ExecDirect(const char *inCursorName, const char *inSqlString, short inStmtType, IDL_short inSqlStmtType, 
									short inSqlAsyncEnable, Int32 inQueryTimeout);

	void cleanupSQLMessage();
	void cleanupSQLValueList();
	void cleanupSQLDescList();
	void cleanupAll();
// language (character set) specific functions
	Int32 GetCharset();

//  set the clientLCIDvalue
	void setclientLCID(UInt32 value);

// Performance
	SQLRETURN FetchPerf(Int32 inMaxRowCnt, Int32 inMaxRowLen, short inSqlAsyncEnable, Int32 inQueryTimeout);

// Rowset
	SQLRETURN ExecuteRowset(const char *inCursorName, IDL_long inInputRowCnt, IDL_short inSqlStmtType, 
								const SQL_DataValue_def *inDataValue, 
								short inSqlAsyncEnable, Int32 inQueryTimeout);
// Modules	
	SQLRETURN PrepareFromModule(short stmtType);
	SQLRETURN InternalStmtClose(unsigned short inFreeResourceOpt);
	SQLRETURN freeBuffers(short descType);
	SQLRETURN allocSqlmxHdls(const char *stmtName, const char *moduleName, int64 moduleTimestamp,
			Int32 moduleVersion, const char *inputDescName, const char *outputDescName,
			short sqlStmtType);

	SQL_DataValue_def outputDataValue;
	SQL_DataValue_def inputDataValue;
	SQL_DataValue_def delayedOutputDataValue;

	SQLRETURN ExecuteCall(const SQLValueList_def *inValueList,short inSqlAsyncEnable, 
		Int32 inQueryTimeout);

	SQLRETURN FetchCatalogRowset(Int32 inMaxRowCnt, Int32 inMaxRowLen, short inSqlAsyncEnable, Int32 inQueryTimeout, bool resetValues);
//
// AGGREGATION
//
//
};
//extern DWORD WINAPI ThreadControlProc(LPVOID pParam);
//extern DWORD WINAPI doQueryTimeout(LPVOID pParam);
namespace SRVR {
	DWORD WINAPI ControlProc(LPVOID pParam);
}

#endif
