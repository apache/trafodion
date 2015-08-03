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
// MODULE: SrvrKds.h
//
// PURPOSE:
//     
//
//

#ifndef _SRVRKDS_DEFINED
#define _SRVRKDS_DEFINED

extern bool g_bSqlMessageSet;

extern void kdsCreateSQLDescSeq(SQLItemDescList_def *SQLDesc, short numEntries);
extern void kdsCreateEmptySQLDescSeq(SQLItemDescList_def *SQLDesc);

extern void kdsCreateSQLErrorException(	odbc_SQLSvc_SQLError *SQLError, 
								long numConditions, bool &bSQLMessageSet = g_bSqlMessageSet);

extern void kdsCopySQLErrorException(odbc_SQLSvc_SQLError *SQLError, 
								char *msg_buf,
								long sqlcode,
								char *sqlState);

extern void kdsCopySQLErrorExceptionAndRowCount(
							odbc_SQLSvc_SQLError *SQLError, 
							char *msg_buf,
							long sqlcode,
							char *sqlState,
							long currentRowCount);

extern void kdsCopyToSQLValueSeq(SQLValueList_def *SQLValueList,
							 long dataType, 
							 short indValue, 
							 void *varPtr, 
							 long allocLength,
							 long Charset);

long kdsCopyToSMDSQLValueSeq(SQLValueList_def *SQLValueList,
							 long dataType, 
							 short indValue, 
							 char *dataValue, 
							 long allocLength,
							 long Charset);

void kdsCopyRGErrorException(odbc_SQLSvc_SQLError *SQLError, 
							RES_HIT_DESC_def *rgPolicyHit);

void kdsCreateSQLWarningException(ERROR_DESC_LIST_def *SQLWarning, 
								long numConditions);
void kdsCopyRGWarningException(ERROR_DESC_LIST_def *SQLWarning, 
							RES_HIT_DESC_def *rgPolicyHit);

#endif
