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

#ifndef SRVRCONNECT_DEFINED
#define SRVRCONNECT_DEFINED

typedef struct AS_CALL_CONTEXT {
	CEE_handle_def timerHandle;
	CEE_handle_def ASSvc_proxy;
	SRVR_STATE srvrState;
} AS_CALL_CONTEXT;

typedef struct {
	CEE_handle_def	call_id;
	CEE_tag_def		objTag;
	DIALOGUE_ID_def dialogueId;
	CEE_status		sts;
} MonitorCallContext;

enum E_GetSQLInfoType { 
	EXPLAIN_PLAN = 0,
	MODE_SPECIAL_1,
	NESTED_JOINS,
	USER_ROLE,
	SCHEMA_DEFAULT,
	DEFAULT_SCHEMA_ACCESS_ONLY
};

// For Workload Service
#define MAX_QUERIES_LIMIT	500

#define CHECK_SERVICE		1
#define CHECK_SERVICEMAX	2
#define CHECK_SERVICEPRTY	3
#define CHECK_MAXQUERIES_OTHERS	4 // this includes EXECUTING and WAITING
#define CHECK_QUERIES_WAITING	5
#define CHECK_QUERIES_EXECUTING	6

#define	CMD_ADD					1
#define	CMD_ALTER				2
#define	CMD_DELETE				4
#define	CMD_INFO				8
#define	CMD_SERVICE				16
#define	CMD_THRESHOLD			32
#define	CMD_PREPARE				64

#define	ARG_SERVICE_NAME		1
#define	ARG_PRIORITYURGENT		2
#define	ARG_PRIORITYHIGH		4
#define	ARG_PRIORITYMEDIUMHIGH	8
#define	ARG_PRIORITYMEDIUM		16
#define	ARG_PRIORITYLOWMEDIUM	32
#define	ARG_PRIORITYLOW			64
#define	ARG_MAX_EXECUTING		128
#define	ARG_MAX_WAITING			256
#define	ARG_MAX_CPU_BUSY		512
#define	ARG_STATS_INTERVAL		1024
#define	ARG_ACTIVE				2048
#define	ARG_MAX_MEM_USAGE		4096
#define	ARG_COMMENT				8192
#define	ARG_VALUE				16384
#define	ARG_STATEMENT_NAME		32768

#define EXC_SERVICEINVALIDARGUMENTS		1
#define EXC_SERVICEALREADYEXISTS		2
#define EXC_SERVICENOTEXISTS			3
#define EXC_SERVICECANNOTDELETE			4
#define EXC_STATEMENTINVALIDLENGTH		5
#define EXC_SQLFAILED					6
#define EXC_SERVICEINVALIDARGUMENTTYPE	7
#define EXC_SERVICELIMITSERVICEQUERIES	8
#define EXC_SERVICELIMITSYSTEMQUERIES	9
#define CmdDll "libzcmd.so"

#define HPDCI_APPLICATION	"HPDCI"


bool ChkDsExists(char* dsname,char* PrimaryCatalog, int& retcode);
bool CheckUserPerm(char* PrimaryCatalog,long& uid);
bool ChkWSvcCommands(char* dsname, int& retcode, long type);

namespace SRVR {

void __cdecl ASTimerExpired(CEE_tag_def timer_tag);
BOOL checkIfASSvcLives( void );
void __cdecl SrvrSessionCleanup(void);
bool __cdecl CompilerCacheReset(char *errorMsg);
void __cdecl BreakDialogue(CEE_tag_def monitor_tag);

extern void __cdecl srvrIdleTimerExpired(CEE_tag_def timer_tag);
extern void exitServerProcess();

extern void ENABLE_SERVER_TRACE(IDL_long TraceType);
extern void DISABLE_SERVER_TRACE(IDL_long TraceType);
extern void ENABLE_STATISTICS(IDL_long StatisticsType);
extern void DISABLE_STATISTICS();
extern void UPDATE_SERVER_CONTEXT(const SRVR_CONTEXT_def *srvrContext);
extern void UPDATE_SERVER_WAITED(IDL_long TraceType, IDL_long StatisticsType, IDL_long ContextType,const SRVR_CONTEXT_def *srvrContext);
extern BOOL updateSrvrState(_SRVR_STATE srvrState);
extern void RegisterSrvr(char* IpAddress, char* HostName);
extern long getConnIdleTimeout();
extern long getSrvrIdleTimeout();

extern "C" void
odbc_SQLSvc_Prepare2_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_long sqlAsyncEnable
  , /* In    */ IDL_long queryTimeout
  , /* In    */ IDL_long inputRowCnt
  , /* In    */ IDL_long sqlStmtType
  , /* In    */ IDL_long stmtLength
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_long stmtLabelCharset
  , /* In    */ IDL_long cursorLength
  , /* In    */ IDL_string cursorName
  , /* In    */ IDL_long cursorCharset
  , /* In    */ IDL_long moduleNameLength
  , /* In    */ const IDL_char *moduleName
  , /* In    */ IDL_long moduleCharset
  , /* In    */ IDL_long_long moduleTimestamp
  , /* In    */ IDL_long sqlStringLength
  , /* In    */ IDL_string sqlString
  , /* In    */ IDL_long sqlStringCharset
  , /* In    */ IDL_long setStmtOptionsLength
  , /* In    */ IDL_string setStmtOptions
  );

extern "C" void
odbc_SQLSvc_Execute2_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_long sqlAsyncEnable
  , /* In    */ IDL_long queryTimeout
  , /* In    */ IDL_long inputRowCnt
  , /* In    */ IDL_long sqlStmtType
  , /* In    */ Long stmtHandle
  , /* In    */ IDL_long cursorLength
  , /* In    */ IDL_string cursorName
  , /* In    */ IDL_long cursorCharset
  , /* In    */ IDL_long inValuesLength
  , /* In    */ BYTE *inValues
  );

extern "C" void
odbc_SQLSvc_Fetch2_ame_(
    /* In    */       CEE_tag_def      objtag_
  , /* In    */ const CEE_handle_def  *call_id_
  , /* In    */       DIALOGUE_ID_def  dialogueId
  , /* In    */       IDL_long         sqlAsyncEnable
  , /* In    */       IDL_long         queryTimeout
  , /* In    */       Long         stmtHandle
  , /* In    */       IDL_long         maxRowCnt 
  , /* In    */       IDL_long         cursorLength
  , /* In    */       IDL_string       cursorName
  , /* In    */       IDL_long         cursorCharset
  );

extern "C" void
odbc_SQLSrvr_Close_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_unsigned_short freeResourceOpt
  , /* Out   */ IDL_long *rowsAffected
  , /* Out   */ IDL_long *returncode
  , /* Out   */	IDL_long *sqlWarningOrErrorLength
  , /* Out   */	BYTE *&sqlWarningOrError
  );

extern "C" void
odbc_SQLSrvr_Close_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_unsigned_short freeResourceOpt
  );

}

extern "C" void
odbc_SQLSrvr_Prepare_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_long sqlAsyncEnable
  , /* In    */ IDL_long queryTimeout
  , /* In    */ IDL_short stmtType
  , /* In    */ IDL_long sqlStmtType
  , /* In    */ IDL_long stmtLength
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_long stmtLabelCharset
  , /* In    */ IDL_long cursorLength
  , /* In    */ IDL_string cursorName
  , /* In    */ IDL_long cursorCharset
  , /* In    */ IDL_long moduleNameLength
  , /* In    */ const IDL_char *moduleName
  , /* In    */ IDL_long moduleCharset
  , /* In    */ IDL_long_long moduleTimestamp
  , /* In    */ IDL_long sqlStringLength
  , /* In    */ IDL_string sqlString
  , /* In    */ IDL_long sqlStringCharset
  , /* In    */ IDL_long setStmtOptionsLength
  , /* In    */ IDL_string setStmtOptions
  , /* In    */ IDL_long stmtExplainLabelLength
  , /* In    */ IDL_string stmtExplainLabel
  , /* In    */ IDL_long maxRowsetSize
  , /* In    */ IDL_long_long txnID       // T4 driver sends a transaction ID which we need to join
  , /* In    */ IDL_short *extTransId     // T4 driver sends a transaction ID which we need to join
  , /* In    */ IDL_long holdableCursor
  );

extern "C" void
odbc_SQLSrvr_Fetch_ame_(
    /* In    */       CEE_tag_def      objtag_
  , /* In    */ const CEE_handle_def  *call_id_
  , /* In    */       DIALOGUE_ID_def  dialogueId
  , /* In    */       IDL_short        operation_id
  , /* In    */       IDL_long         sqlAsyncEnable
  , /* In    */       IDL_long         queryTimeout
  , /* In    */       Long         stmtHandle
  , /* In    */ const IDL_string       stmtLabel
  , /* In    */       IDL_unsigned_long_long maxRowCnt 
  , /* In    */       IDL_unsigned_long_long maxRowLen
  );


extern "C" void
odbc_SQLSrvr_Execute2_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_long sqlAsyncEnable
  , /* In    */ IDL_long queryTimeout
  , /* In    */ IDL_long inputRowCnt
  , /* In    */ IDL_long sqlStmtType
  , /* In    */ Long stmtHandle
  , /* In    */ IDL_string cursorName
  , /* In    */ IDL_long cursorCharset
  , /* In    */ IDL_long inValuesLength
  , /* In    */ BYTE *inValues
  , /* In    */ IDL_long sqlQueryType     // Used with execdirect. Execdirect will call prepare/execute. This is one of the output params from prepare
  , /* In    */ IDL_long outputDescLength // Used with execdirect. Execdirect will call prepare/execute. This is one of the output params from prepare
  , /* In    */ BYTE *outputDesc          // Used with execdirect. Execdirect will call prepare/execute. This is one of the output params from prepare
  , /* In    */ IDL_long rowLength		  // For DBT to obtain the Rowlength 
  , /* In    */ IDL_long_long txnID       // T4 driver sends a transaction ID which we need to join
  , /* In    */ IDL_long holdableCursor 
  );


extern "C" void
odbc_SQLSrvr_ExecDirect_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_string cursorName
  , /* In    */ const IDL_char *stmtExplainLabel
  , /* In    */ IDL_short stmtType
  , /* In    */ IDL_short sqlStmtType
  , /* In    */ IDL_string sqlString
  , /* In    */ IDL_short sqlAsyncEnable
  , /* In    */ IDL_long queryTimeout
  , /* In    */ IDL_long inputRowCnt
  , /* In    */ IDL_long_long txnID       // T4 driver sends a transaction ID which we need to join
  , /* In    */ IDL_long holdableCursor
  );

extern "C" void
odbc_SQLSrvr_SetConnectionOption_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_short connectionOption
  , /* In    */ IDL_long  optionValueNum
  , /* In    */ IDL_string optionValueStr);

extern "C" void
odbc_SQLSrvr_GetSQLCatalogs_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_short APIType
  , /* In    */ const IDL_char *catalogNm
  , /* In    */ const IDL_char *schemaNm
  , /* In    */ const IDL_char *tableNm
  , /* In    */ const IDL_char *tableTypeList
  , /* In    */ const IDL_char *columnNm
  , /* In    */ IDL_long columnType
  , /* In    */ IDL_long rowIdScope
  , /* In    */ IDL_long nullable
  , /* In    */ IDL_long uniqueness
  , /* In    */ IDL_long accuracy
  , /* In    */ IDL_short sqlType
  , /* In    */ IDL_unsigned_long metadataId
  , /* In    */ const IDL_char *fkcatalogNm
  , /* In    */ const IDL_char *fkschemaNm
  , /* In    */ const IDL_char *fktableNm
  );

extern "C" void
odbc_SQLSrvr_EndTransaction_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_unsigned_short transactionOpt
  );

extern "C" void
odbc_SQLSrvr_ExtractLob_ame_(
    /* In   */ CEE_tag_def objtag_
  , /* In   */ const CEE_handle_def *call_id_
  , /* In   */ IDL_short    extractLobAPI
  , /* In   */ IDL_string  lobHandle
  , /* In   */ IDL_long_long   extractLen
  );

void
odbc_SQLSrvr_UpdateLob_ame_(
     /* In   */ CEE_tag_def objtag_
  ,  /* In   */ const CEE_handle_def *call_id_
  ,  /* In   */ IDL_short lobUpdateType
  ,  /* In   */ IDL_string lobHandle
  ,  /* In   */ IDL_long_long totalLength
  ,  /* In   */ IDL_long_long offset
  ,  /* In   */ IDL_long_long length
  ,  /* In   */ BYTE * data
  );

bool SetHigherPriorityForMetaDataAccess();


void getCurrentCatalogSchema();
void  CmdDllClose();
bool loadSyncValues(char segmentArray[],char* filename);

bool isInfoSystem(char*& sqlString, const IDL_char *stmtLabel, short& error );
bool isInfoObject(char*& sqlString, const IDL_char *stmtLabel, short& error );
bool isGetPrivileges(char*& sqlString, const IDL_char *stmtLabel, short& error );
bool checkSyntaxGetPrivileges(char* sqlString, short& idx);
bool isInfoDisk(char*& sqlString, const IDL_char *stmtLabel, short& error, char *errBuf );
bool checkSyntaxInfoDisk(char* sqlString, char *diskName);

void __cdecl StatisticsTimerExpired(CEE_tag_def timer_tag);
void SyncPublicationThread();
#endif
