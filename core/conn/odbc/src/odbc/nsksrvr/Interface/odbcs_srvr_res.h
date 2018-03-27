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

#ifndef ODBCS_SRVR_RES_H
#define ODBCS_SRVR_RES_H

#include "CSrvrStmt.h"


CEE_status
odbc_SQLSvc_InitializeDialogue_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_InitializeDialogue_exc_ *exception_
  , /* In    */ const OUT_CONNECTION_CONTEXT_def *outContext
  );

CEE_status
odbc_SQLSvc_TerminateDialogue_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_TerminateDialogue_exc_ *exception_
  );

CEE_status
odbc_SQLSvc_SetConnectionOption_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_SetConnectionOption_exc_ *exception_
  , /* In    */ const ERROR_DESC_LIST_def *sqlWarning
  );

CEE_status
odbc_SQLSvc_Prepare_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_Prepare_exc_ *exception_
  , /* In    */ IDL_long estimatedCost
  , /* In    */ const SQLItemDescList_def *inputDesc
  , /* In    */ const SQLItemDescList_def *outputDesc
  , /* In    */ const ERROR_DESC_LIST_def *sqlWarning
  );

CEE_status
odbc_SQLSvc_ExecDirect_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_ExecDirect_exc_ *exception_
  , /* In    */ IDL_long estimatedCost
  , /* In    */ const SQLItemDescList_def *outputDesc
  , /* In    */ IDL_long rowsAffected
  , /* In    */ const ERROR_DESC_LIST_def *sqlWarning
  );

CEE_status
odbc_SQLSvc_PrepareRowset_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_PrepareRowset_exc_ *exception_
  , /* In    */ IDL_long estimatedCost
  , /* In    */ const SQLItemDescList_def *inputDesc
  , /* In    */ const SQLItemDescList_def *outputDesc
  , /* In    */ const ERROR_DESC_LIST_def *sqlWarning
  );

CEE_status
odbc_SQLSvc_ExecuteRowset_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_ExecuteRowset_exc_ *exception_
  , /* In    */ IDL_long rowsAffected
  , /* In    */ const ERROR_DESC_LIST_def *sqlWarning
);

CEE_status
odbc_SQLSvc_ExecDirectRowset_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_ExecDirectRowset_exc_ *exception_
  , /* In    */ IDL_long estimatedCost
  , /* In    */ const SQLItemDescList_def *outputDesc
  , /* In    */ IDL_long rowsAffected
  , /* In    */ const ERROR_DESC_LIST_def *sqlWarning
);

CEE_status
odbc_SQLSvc_FetchRowset_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_FetchRowset_exc_ *exception_
  , /* In    */ IDL_long rowsAffected
  , /* In    */ const SQL_DataValue_def *outputDataValue
  , /* In    */ const ERROR_DESC_LIST_def *sqlWarning
);

CEE_status
odbc_SQLSvc_FetchPerf_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_FetchPerf_exc_ *exception_
  , /* In    */ IDL_long rowsAffected
  , /* In    */ const SQL_DataValue_def *outputDataValue
  , /* In    */ const ERROR_DESC_LIST_def *sqlWarning
);

CEE_status
odbc_SQLSvc_ExecuteN_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_ExecuteN_exc_ *exception_
  , /* In    */ IDL_long rowsAffected
  , /* In    */ const ERROR_DESC_LIST_def *sqlWarning
);

CEE_status
odbc_SQLSvc_Prepare2_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In   */ IDL_long returnCode
  , /* In   */ IDL_long sqlWarningOrErrorLength
  , /* In   */ BYTE *sqlWarningOrError
  , /* In   */ IDL_long sqlQueryType
  , /* In   */ Long stmtHandle
  , /* In   */ IDL_long estimatedCost
  , /* In   */ IDL_long inputDescLength
  , /* In   */ BYTE *inputDesc
  , /* In   */ IDL_long outputDescLength
  , /* In   */ BYTE *outputDesc
);

CEE_status
odbc_SQLSvc_Execute2_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ IDL_long returnCode
  , /* In    */ IDL_long sqlWarningOrErrorLength
  , /* In    */ BYTE *sqlWarningOrError
  , /* In    */ IDL_long rowsAffected
  , /* In    */ IDL_long outValuesLength
  , /* In    */ BYTE *outValues
);

CEE_status
odbc_SQLSvc_Close_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_Close_exc_ *exception_
  , /* In    */ IDL_long rowsAffected
  , /* In    */ const ERROR_DESC_LIST_def *sqlWarning
);

CEE_status
odbc_SQLSvc_EndTransaction_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_EndTransaction_exc_ *exception_
  , /* In    */ const ERROR_DESC_LIST_def *sqlWarning
);

CEE_status
odbc_SQLSvc_GetSQLCatalogs_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_GetSQLCatalogs_exc_ *exception_
  , /* In    */ const IDL_char *catStmtLabel
  , /* In    */ const SQLItemDescList_def *outputDesc
  , /* In    */ const ERROR_DESC_LIST_def *sqlWarning
);

CEE_status
odbc_SQLSvc_DisableServerStatistics_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_DisableServerStatistics_exc_ *exception_
);

CEE_status
odbc_SQLSvc_DisableServerTrace_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_DisableServerTrace_exc_ *exception_
);

CEE_status
odbc_SQLSvc_EnableServerStatistics_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_EnableServerStatistics_exc_ *exception_
);

CEE_status
odbc_SQLSvc_EnableServerTrace_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_EnableServerTrace_exc_ *exception_
);

CEE_status
odbc_SQLSvc_ExecuteCall_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_ExecuteCall_exc_ *exception_
  , /* In    */ const SQLValueList_def *outputValueList
  , /* In    */ IDL_long rowsAffected
  , /* In    */ const ERROR_DESC_LIST_def *sqlWarning
);

CEE_status
odbc_SQLSvc_StopServer_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_StopServer_exc_ *exception_
);

CEE_status
odbc_SQLSvc_UpdateServerContext_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_UpdateServerContext_exc_ *exception_
);

CEE_status
odbc_SQLSvc_MonitorCall_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_MonitorCall_exc_ *exception_
);

CEE_status
odbc_SQLSvc_Fetch2_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ IDL_long returnCode
  , /* In    */ IDL_long sqlWarningOrErrorLength
  , /* In    */ BYTE *sqlWarningOrError
  , /* In    */ IDL_long rowsAffected
  , /* In    */ IDL_long outValuesFormat
  , /* In    */ IDL_long outValuesLength
  , /* In    */ BYTE *outValues
);

CEE_status
odbc_SQLSrvr_Close_ts_res_(
	/* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ IDL_long returnCode
  , /* In    */ IDL_long rowsAffected
  , /* In    */ IDL_long sqlWarningOrErrorLength
  , /* In    */ BYTE *sqlWarningOrError
);

CEE_status
odbc_SQLSrvr_Prepare_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In   */ IDL_long returnCode
  , /* In   */ IDL_long sqlWarningOrErrorLength
  , /* In   */ BYTE *sqlWarningOrError
  , /* In   */ IDL_long sqlQueryType
  , /* In   */ IDL_long stmtHandleKey
  , /* In   */ IDL_long estimatedCost
  , /* In   */ IDL_long inputDescLength
  , /* In   */ BYTE *inputDesc
  , /* In   */ IDL_long outputDescLength
  , /* In   */ BYTE *outputDesc
);

CEE_status
odbc_SQLSrvr_Fetch_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ IDL_long returnCode
  , /* In    */ IDL_long sqlWarningOrErrorLength
  , /* In    */ BYTE *sqlWarningOrError
  , /* In    */ IDL_long rowsAffected
  , /* In    */ IDL_long outValuesFormat
  , /* In    */ IDL_long outValuesLength
  , /* In    */ BYTE *outValues
);

CEE_status
odbc_SQLSrvr_Execute_ts_res_(
    /* In    */       CEE_tag_def     objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */       IDL_long        returnCode
  , /* In    */       IDL_long        sqlWarningOrErrorLength
  , /* In    */       BYTE           *sqlWarningOrError
  , /* In    */       IDL_long        rowsReturned
  , /* In    */       IDL_long        sqlQueryType
  , /* In    */       IDL_long        estimatedCost
  , /* In    */       IDL_long        outValuesLength
  , /* In    */       BYTE           *outValues
  , /* In    */		  IDL_long outputDescLength // Used with execdirect. Execdirect will call prepare/execute. This is one of the output params from prepare
  , /* In    */		  BYTE *outputDesc          // Used with execdirect. Execdirect will call prepare/execute. This is one of the output params from prepare
  , /* In    */       Long		 stmtHandle // Used for SPJ result sets
  , /* In    */       IDL_long   stmtHandleKey  
);

CEE_status
odbc_SQLSrvr_SetConnectionOption_ts_res_(
    /* In    */       CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_SetConnectionOption_exc_ *exception_
  , /* In    */       ERROR_DESC_LIST_def *sqlWarning
  );

CEE_status
odbc_SQLSrvr_GetSQLCatalogs_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_GetSQLCatalogs_exc_ *exception_
  , /* In    */ const IDL_char *catStmtLabel
  , /* In    */ SQLItemDescList_def *outputDesc
  , /* In    */ ERROR_DESC_LIST_def *sqlWarning
  , /* In    */ SRVR_STMT_HDL *pSrvrStmt
  );

CEE_status
odbc_SQLSrvr_EndTransaction_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_EndTransaction_exc_ *exception_
  , /* In    */ ERROR_DESC_LIST_def *sqlWarning
  );

CEE_status
odbc_SQLSrvr_ExtractLob_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLsrvr_ExtractLob_exc_ *exception_
  , /* In   */ IDL_short  extractLobAPI
  , /* In   */ IDL_long_long  lobLength
  , /* In   */ IDL_long_long  extractLen
  , /* In   */ BYTE   *  extractData
);

void
odbc_SQLSrvr_UpdateLob_ts_res_(
  /* In   */ CEE_tag_def objtag_
  , /* In   */ const CEE_handle_def * call_id_
  , /* In   */ const struct odbc_SQLSvc_UpdateLob_exc_ * exception_
  );
#endif
