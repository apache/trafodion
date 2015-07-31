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

#ifndef MARSHALING_DRVR_H
#define MARSHALING_DRVR_H

#include "marshaling.h"

CEE_status 
odbcas_ASSvc_GetObjRefHdl_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ const CONNECTION_CONTEXT_def *inContext
		, /* In    */ const USER_DESC_def *userDesc
		, /* In    */ IDL_long srvrType
		, /* In    */ IDL_short retryCount
);

CEE_status
odbcas_ASSvc_UpdateSrvrState_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ IDL_long srvrType
		, /* In    */ const IDL_char *srvrObjRef
		, /* In    */ IDL_long srvrState
);

CEE_status
odbcas_ASSvc_StopSrvr_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ IDL_long srvrType
		, /* In    */ const IDL_char *srvrObjRef
		, /* In    */ IDL_long StopType
);

CEE_status
odbcas_ASSvc_StatusDSAll_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
);

CEE_status
odbc_SQLSvc_InitializeDialogue_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ const USER_DESC_def *userDesc
		, /* In    */ const CONNECTION_CONTEXT_def *inContext
		, /* In    */ DIALOGUE_ID_def dialogueId
);

CEE_status
odbc_SQLSvc_TerminateDialogue_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
);

CEE_status
odbc_SQLSvc_SetConnectionOption_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ IDL_short connectionOption
		, /* In    */ IDL_long optionValueNum
		, /* In    */ IDL_string optionValueStr
);

CEE_status
odbc_SQLSvc_Prepare_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ const IDL_char *stmtExplainLabel
		, /* In    */ IDL_short stmtType
		, /* In    */ IDL_string sqlString
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
);

CEE_status
odbc_SQLSvc_ExecDirect_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_string cursorName
		, /* In    */ const IDL_char *stmtExplainLabel
		, /* In    */ IDL_short stmtType
		, /* In    */ IDL_short sqlStmtType
		, /* In    */ IDL_string sqlString
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
);

CEE_status
odbc_SQLSvc_PrepareRowset_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ const IDL_char *stmtExplainLabel
		, /* In    */ IDL_short stmtType
		, /* In    */ IDL_short sqlStmtType
		, /* In    */ IDL_string sqlString
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
		, /* In    */ IDL_long maxRowsetSize
);

CEE_status
odbc_SQLSvc_ExecuteN_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_string cursorName
		, /* In    */ IDL_short sqlStmtType
		, /* In    */ IDL_long inputRowCnt
		, /* In    */ const SQLValueList_def *inputValueList
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
);

CEE_status
odbc_SQLSvc_ExecuteRowset_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_string cursorName
		, /* In    */ IDL_short sqlStmtType
		, /* In    */ IDL_long inputRowCnt
		, /* In    */ const SQL_DataValue_def *inputDataValue
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
);

CEE_status
odbc_SQLSvc_ExecDirectRowset_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_string cursorName
		, /* In    */ const IDL_char *stmtExplainLabel
		, /* In    */ IDL_short stmtType
		, /* In    */ IDL_short sqlStmtType
		, /* In    */ IDL_string sqlString
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
		, /* In    */ IDL_long maxRowsetSize
);

CEE_status
odbc_SQLSvc_FetchRowset_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_long maxRowCnt
		, /* In    */ IDL_long maxRowLen
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
);

CEE_status
odbc_SQLSvc_FetchPerf_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_long maxRowCnt
		, /* In    */ IDL_long maxRowLen
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
);

CEE_status
odbc_SQLSvc_Close_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_unsigned_short freeResourceOpt
);

CEE_status
odbc_SQLSvc_EndTransaction_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
	    , /* In    */ DIALOGUE_ID_def dialogueId
	    , /* In    */ IDL_unsigned_short transactionOpt
);

CEE_status
odbc_SQLSvc_GetSQLCatalogs_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
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

CEE_status
odbc_SQLSvc_ExecuteCall_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_string cursorName
		, /* In    */ IDL_short sqlStmtType
		, /* In    */ IDL_long inputRowCnt
		, /* In    */ const SQLValueList_def *inputValueList
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
);

CEE_status
odbc_SQLSvc_MonitorCall_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
);

#endif
