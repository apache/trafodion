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

#ifndef MARSHALING_SRVR_H
#define MARSHALING_SRVR_H


CEE_status
odbc_SQLSrvr_Close_param_res_(
        SRVR_STMT_HDL* pnode
        , char*& buffer
        , UInt32& message_length
        , /* In    */ IDL_long returnCode
        , /* In    */ IDL_long sqlWarningOrErrorLength
        , /* In    */ BYTE *sqlWarningOrError
        , /* In    */ IDL_long rowsAffected
        );

CEE_status
odbc_SQLSrvr_Prepare_param_res_(
        SRVR_STMT_HDL* pnode
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
odbc_SQLSrvr_Fetch_param_res_(
        /* In    */ SRVR_STMT_HDL* pnode
        , /* In    */ IDL_long returnCode
        , /* In    */ IDL_long sqlWarningOrErrorLength
        , /* In    */ BYTE *sqlWarningOrError
        , /* In    */ IDL_long rowsAffected
        , /* In    */ IDL_long outValuesFormat
        , /* In    */ IDL_long outValuesLength
        , /* In    */ BYTE *outValues
        );

CEE_status
odbc_SQLSrvr_Execute_param_res_(
        SRVR_STMT_HDL* pnode
        , /* In    */ IDL_long returnCode
        , /* In    */ IDL_long sqlWarningOrErrorLength
        , /* In    */ BYTE *sqlWarningOrError
        , /* In    */ IDL_long rowsAffected
        , /* In    */ IDL_long sqlQueryType     // Used by ExecDirect for unique selects
        , /* In    */ IDL_long estimatedCost
        , /* In    */ IDL_long outValuesLength
        , /* In    */ BYTE *outValues
        , /* In    */ IDL_long outputDescLength // Used to return the output descriptors for ExecDirect
        , /* In    */ BYTE *outputDesc          // Used to return the output descriptors for ExecDirect
        , /* In    */ Long stmtHandle       // Statement handle - needed to copy out SPJ result sets
        , /* In    */ IDL_long stmtHandleKey        
        );


CEE_status
odbc_SQLSrvr_GetSQLCatalogs_param_res_(
        SRVR_STMT_HDL* pnode
        , IDL_char*& buffer
        , IDL_unsigned_long& message_length
        , /* In    */ const struct odbc_SQLSvc_GetSQLCatalogs_exc_ *exception_
        , /* In    */ const IDL_char *catStmtLabel
        , /* In    */ SQLItemDescList_def *outputDesc
        , /* In    */ ERROR_DESC_LIST_def *sqlWarning
        , /* In    */ SRVR_STMT_HDL *pSrvrStmt
        );

CEE_status
odbc_SQLSrvr_EndTransaction_param_res_(
        SRVR_STMT_HDL* pnode
        , IDL_char*& buffer
        , IDL_unsigned_long& message_length
        , /* In    */ const struct odbc_SQLSvc_EndTransaction_exc_ *exception_
        , /* In    */ ERROR_DESC_LIST_def *sqlWarning
        );

#endif
