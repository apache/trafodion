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

#ifndef ODBCS_DRVR_H
#define ODBCS_DRVR_H

#include "cconnect.h"

CEE_status MAP_SRVR_ERRORS(CConnect *pConnection);

/* mxHash --------------------------------------------------------------
 * Compute the hash value for the given set of key values and the
 * given number of partitions.  This is the external interface to the SQL/MX
 * hash function used by the Hash Partitioning feature.  Values representing
 * partitioning key data are passed in along with the number of partitions in
 * the target table and the result is the number of the partition where this
 * key data will reside.
 */
#ifdef __cplusplus
extern "C"
{
#endif

enum eHashScheme {
  eHashSchemeUnknown = 0,
  eHashSchemeHP = 1,
  eHashSchemeH2 = 2
};

unsigned int
mxHash(void *vals[],
       unsigned int valLens[],
       unsigned int numVals,
       unsigned int numParts,
       eHashScheme hashScheme);

typedef struct HASH_STREAM
{
	CTCPIPUnixDrvr* SrvrSteam;

} HASH_STREAM;


#ifdef __cplusplus
}
#endif

extern "C"  CEE_status
odbc_SQLDrvr_Prepare_pst_(
    /* In    */ CEE_tag_def tag_
  , /* In    */ IDL_string setStmtOptions
  , /* Out   */ IDL_long *returnCode
  , /* Out   */ BYTE *&sqlWarningOrError
  , /* Out   */ IDL_long *sqlQueryType
  , /* Out   */ IDL_long *stmtHandle
  , /* Out   */ IDL_long *estimatedCost
  , /* Out   */ BYTE *&inputParams
  , /* Out   */ SQLItemDescList_def *inputDesc
  , /* Out   */ BYTE *&outputColumns
  , /* Out   */ SQLItemDescList_def *outputDesc
);


extern "C"  CEE_status
odbc_SQLDrvr_Fetch_pst_(
    /* In    */ CEE_tag_def tag_
  , /* In    */ IDL_string setStmtOptions
  , /* Out   */ IDL_long *returnCode
  , /* Out   */ BYTE *&sqlWarningOrError
  , /* Out   */ IDL_long *rowsReturned
  , /* Out   */ IDL_long *outValuesFormat
  , /* Out   */ SQL_DataValue_def *outputDataValue
  );

extern "C"  CEE_status
odbc_SQLSvc_InitializeDialogue_(
    /* In    */ const CEE_handle_def *ph_
  , /* In    */ CEE_tag_def tag_
  , /* In    */ const USER_DESC_def *userDesc
  , /* In    */ const CONNECTION_CONTEXT_def *inContext
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* Out   */ odbc_SQLSvc_InitializeDialogue_exc_ *exception_
  , /* Out   */ OUT_CONNECTION_CONTEXT_def *outContext
  );

extern "C"  CEE_status
odbc_SQLSvc_TerminateDialogue_(
    /* In    */ const CEE_handle_def *ph_
  , /* In    */ CEE_tag_def tag_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* Out   */ odbc_SQLSvc_TerminateDialogue_exc_ *exception_
  );

extern "C"  CEE_status
odbc_SQLDrvr_Close_pst_(
    /* In    */ CEE_tag_def tag_
  , /* Out   */ IDL_long *returnCode
  , /* Out   */ IDL_long *rowsAffected
  , /* Out   */ BYTE *&sqlWarningOrError
  );

extern "C"  CEE_status
odbc_SQLDrvr_Execute_pst_(
    /* In    */ CEE_tag_def tag_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_long sqlAsyncEnable
  , /* In    */ IDL_long holdableCursor
  , /* In    */ IDL_long queryTimeout
  , /* In    */ IDL_long inputRowCnt
  , /* In    */ IDL_long maxRowsetSize
  , /* In    */ IDL_long sqlStmtType
  , /* In    */ IDL_long stmtHandle
  , /* In    */ IDL_long stmtType
  , /* In    */ IDL_string sqlString
  , /* In    */ IDL_string cursorName
  , /* In    */ IDL_string stmtLabel
  , /* In    */ IDL_string stmtExplainLabel
  , /* In    */ const SQLValueList_def *inputValueList
  , /* Out   */ IDL_long *returnCode
  , /* Out   */ BYTE *&sqlWarningOrError
  , /* Out   */ IDL_long_long *rowsAffected
  , /* Out   */ IDL_long *queryType
  , /* Out   */ IDL_long *estimatedCost
  , /* Out   */ SQL_DataValue_def *outputDataValue
  , /* Out   */ SQLItemDescList_def *outputItemDescList
  , /* Out   */ BYTE *&outputColumns
  );


extern "C"  CEE_status
odbc_SQLDrvr_SetConnectionOption_pst_(
    /* In    */ CEE_tag_def tag_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_short connectionOption
  , /* In    */ IDL_long optionValueNum
  , /* In    */ IDL_string optionValueStr
  , /* Out    */ struct odbc_SQLSvc_SetConnectionOption_exc_ *exception_
  , /* Out    */ ERROR_DESC_LIST_def *sqlWarning
  );

extern "C"  CEE_status
odbc_SQLDrvr_GetSQLCatalogs_pst_(
    /* In    */ CEE_tag_def tag_
  , /* Out   */ IDL_string *catStmtLabel
  , /* Out   */ SQLItemDescList_def *outputItemDescList
  , /* Out   */ struct odbc_SQLSvc_GetSQLCatalogs_exc_ *exception_
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
  );

extern "C"  CEE_status
odbc_SQLDrvr_EndTransaction_pst_(
    /* In    */ CEE_tag_def tag_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_unsigned_short transactionOpt
  , /* Out   */ struct odbc_SQLSvc_EndTransaction_exc_ *exception_
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
  );

#endif /* ODBCS_DRVR_H */
