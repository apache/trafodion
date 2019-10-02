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
// MODULE: sqlInterface.h
//
// PURPOSE: function prototype for sqlInterface.cpp
//
//
 /*Change Log
 * Methods Changed: Removed setOfCQD
 */

#ifndef _SQLINTERFACE_DEFINED
#define _SQLINTERFACE_DEFINED
#include "CDesc.h"
#include "CSrvrConnect.h"


extern SQLRETURN AllocAssignValueBuffer(SQLItemDescList_def *SQLDesc,  SQLValueList_def *SQLValueList,
										long totalMemLen, 	long maxRowCount, BYTE *&VarBuffer);
extern SQLRETURN CopyValueList(SQLValueList_def *outValueList, const SQLValueList_def *inValueList);

extern SQLRETURN EXECUTE(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN FREESTATEMENT(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN PREPARE(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN FETCH(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN GETSQLERROR(SRVR_STMT_HDL *pSrvrStmt,
							 odbc_SQLSvc_SQLError *SQLError);

extern SQLRETURN EXECDIRECT(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN EXECUTESPJRS(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN GETSQLWARNING(SRVR_STMT_HDL *pSrvrStmt,
							   ERROR_DESC_LIST_def *sqlWarning);

extern SQLRETURN CANCEL(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN CLEARDIAGNOSTICS(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN ALLOCSQLMXHDLS(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN ALLOCSQLMXHDLS_SPJRS(SRVR_STMT_HDL *pSrvrStmt, SQLSTMT_ID *callpStmt, const char *RSstmtName);

extern SQLRETURN EXECUTECALL(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN CONNECT(SRVR_CONNECT_HDL *pSrvrConnect);

extern SQLRETURN DISCONNECT(SRVR_CONNECT_HDL *pSrvrConnect);

extern SQLRETURN SWITCHCONTEXT(SRVR_CONNECT_HDL *pSrvrConnect, long *sqlcode);

extern SQLRETURN GETSQLCODE(SRVR_STMT_HDL *pSrvrStmt);

SQLRETURN SET_DATA_PTR(SRVR_STMT_HDL *pSrvrStmt, SRVR_STMT_HDL::DESC_TYPE descType);

SQLRETURN _SQL_WaitCloseOnErr (SRVR_STMT_HDL *pSrvrStmt, long int *retcode );

SQLRETURN _SQL_Wait (SRVR_STMT_HDL *pSrvrStmt, long int *retcode );

#endif
