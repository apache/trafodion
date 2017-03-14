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
// MODULE: SrvrOthers.h
//
#ifndef _SRVROTHERS_DEFINED
#define _SRVROTHERS_DEFINED

#include <map>
#include <string>
#include <sql.h>
#include "CoreCommon.h"

class SRVR_STMT_HDL;
/*
* Exception number constants for
* operation 'odbc_SQLSvc_FetchN'
*/
#define odbc_SQLSvc_FetchN_ParamError_exn_ 1
#define odbc_SQLSvc_FetchN_InvalidConnection_exn_ 2
#define odbc_SQLSvc_FetchN_SQLError_exn_ 3
#define odbc_SQLSvc_FetchN_SQLInvalidHandle_exn_ 4
#define odbc_SQLSvc_FetchN_SQLNoDataFound_exn_ 5
#define odbc_SQLSvc_FetchN_SQLStillExecuting_exn_ 6
#define odbc_SQLSvc_FetchN_SQLQueryCancelled_exn_ 7
#define odbc_SQLSvc_FetchN_TransactionError_exn_ 8
#define SQL_RECOMPILE_WARNING			113		// MFC - definition for recompilation warnings

extern "C" void
odbc_SQLSvc_FetchN_sme_(
						/* In	*/ void * objtag_
						, /* In	*/ const CEE_handle_def *call_id_
						, /* Out   */ ExceptionStruct *exception_
						, /* In	*/ long dialogueId
						, /* In	*/ long stmtId
						, /* In	*/ long maxRowCnt
						, /* In	*/ short sqlAsyncEnable
						, /* In	*/ long queryTimeout
						, /* Out   */ long *rowsAffected
						, /* Out   */ SQLValueList_def *outputValueList
						, /* Out   */ ERROR_DESC_LIST_def *sqlWarning
						);

/*
 * Exception number constants for
 * operation 'odbc_SQLSvc_FetchPerf'
 */
#define odbc_SQLSvc_FetchPerf_ParamError_exn_ 1
#define odbc_SQLSvc_FetchPerf_InvalidConnection_exn_ 2
#define odbc_SQLSvc_FetchPerf_SQLError_exn_ 3
#define odbc_SQLSvc_FetchPerf_SQLInvalidHandle_exn_ 4
#define odbc_SQLSvc_FetchPerf_SQLNoDataFound_exn_ 5
#define odbc_SQLSvc_FetchPerf_SQLStillExecuting_exn_ 6
#define odbc_SQLSvc_FetchPerf_SQLQueryCancelled_exn_ 7
#define odbc_SQLSvc_FetchPerf_TransactionError_exn_ 8

/*
 * Local index for operation 'odbc_SQLSvc_FetchPerf'
 */
#define odbc_SQLSvc_FetchPerf_ldx_ ((IDL_unsigned_long) 18)

extern "C" void
odbc_SQLSrvr_FetchPerf_sme_(
    /* In    */ Long dialogueId
  , /* Out   */ IDL_long *returnCode
  , /* In    */ Long     stmtHandle
  , /* In    */ IDL_long maxRowCnt
  , /* In    */ IDL_long maxRowLen
  , /* In    */ IDL_short sqlAsyncEnable
  , /* In    */ IDL_long queryTimeout
  , /* Out   */ IDL_long *rowsAffected
  , /* Out   */ IDL_long *outValuesFormat
  , /* Out   */ SQL_DataValue_def *outputDataValue
  , /* Out   */ IDL_long *sqlWarningOrErrorLength
  , /* Out   */ BYTE     *&sqlWarningOrError);


/*
* Exception number constants for
* operation 'odbc_SQLSvc_Close'
*/
#define odbc_SQLSvc_Close_ParamError_exn_ 1
#define odbc_SQLSvc_Close_InvalidConnection_exn_ 2
#define odbc_SQLSvc_Close_SQLError_exn_ 3
#define odbc_SQLSvc_Close_TransactionError_exn_ 4

extern "C" void
odbc_SQLSvc_Close_sme_(
					   /* In	*/ void * objtag_
					   , /* In	*/ const CEE_handle_def *call_id_
					   , /* Out   */ ExceptionStruct *exception_
					   , /* In	*/ long dialogueId
					   , /* In	*/ long stmtId
					   , /* In	*/ unsigned short freeResourceOpt
					   , /* Out   */ long *rowsAffected
					   , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
					   );

/*
* Exception number constants for
* operation 'odbc_SQLSvc_GetSQLCatalogs'
*/
#define odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_ 1
#define odbc_SQLSvc_GetSQLCatalogs_InvalidConnection_exn_ 2
#define odbc_SQLSvc_GetSQLCatalogs_SQLError_exn_ 3
#define odbc_SQLSvc_GetSQLCatalogs_SQLInvalidHandle_exn_ 4
#define odbc_SQLSvc_GetSQLCatalogs_SQLNoDataFound_exn_ 5

/*
* Exception number constants for
* operation 'odbc_SQLSvc_Prepare'
*/
#define odbc_SQLSvc_Prepare_ParamError_exn_ 1
#define odbc_SQLSvc_Prepare_InvalidConnection_exn_ 2
#define odbc_SQLSvc_Prepare_SQLError_exn_ 3
#define odbc_SQLSvc_Prepare_SQLStillExecuting_exn_ 4
#define odbc_SQLSvc_Prepare_SQLQueryCancelled_exn_ 5
#define odbc_SQLSvc_Prepare_TransactionError_exn_ 6
#define odbc_SQLSvc_Prepare_SQLInvalidHandle_exn_ 7

extern "C" void
odbc_SQLSvc_Prepare_sme_( 	void *               objtag_,			/* In	*/
        const CEE_handle_def     *call_id_,			/* In	*/
        ExceptionStruct			 *exception_,		/* Out  */
        long                      dialogueId,		/* In	*/
        const char           *stmtLabel,		/* In	*/
        const char           *stmtExplainLabel,	/* In	*/
        short                 stmtType,			/* In	*/
        const SQLValue_def       *sqlString,		/* In the SQL statement */
        short                 holdability,		/* In cursor holdability true/false */
        short                 sqlStmtType,		/* In SQL statment is a SELECT, INVOKE, or */
        long                  batchSize,		/* In	*/
        long                  fetchSize,		/* In	*/
        long                  queryTimeout,		/* In	*/
        long                 *estimatedCost,	/* Out Not used */
        SQLItemDescList_def      *inputDesc,		/* Out  */
        SQLItemDescList_def      *outputDesc,		/* Out  */
        ERROR_DESC_LIST_def      *sqlWarning,		/* Out  */
        long                     *stmtId,			/* Out  */
        long                 *inputParamOffset,	/* Out   */
        char                *moduleName,
        bool isISUD
);

/*
 * Exception number constants for
 * operation 'odbc_SQLSvc_PrepareRowset'
 */
#define odbc_SQLSvc_PrepareRowset_ParamError_exn_ 1
#define odbc_SQLSvc_PrepareRowset_InvalidConnection_exn_ 2
#define odbc_SQLSvc_PrepareRowset_SQLError_exn_ 3
#define odbc_SQLSvc_PrepareRowset_SQLStillExecuting_exn_ 4
#define odbc_SQLSvc_PrepareRowset_SQLQueryCancelled_exn_ 5
#define odbc_SQLSvc_PrepareRowset_TransactionError_exn_ 6
/*
 * Exception union for
 * operation 'odbc_SQLSvc_PrepareRowset'
 */
struct odbc_SQLSvc_PrepareRowset_exc_ {
    IDL_long exception_nr;
    IDL_long exception_detail;
    union {
        odbc_SQLSvc_ParamError ParamError;
        odbc_SQLSvc_SQLError SQLError;
        odbc_SQLSvc_SQLQueryCancelled SQLQueryCancelled;
    } u;
};
/*
 * Local index for operation 'odbc_SQLSvc_PrepareRowset'
 */
#define odbc_SQLSvc_PrepareRowset_ldx_ ((IDL_unsigned_long) 20)
/*
 * Operation synopsis for operation 'odbc_SQLSvc_PrepareRowset'
 */
#define odbc_SQLSvc_PrepareRowset_osy_ ((IDL_long) 2039310938)

extern "C" void
odbc_SQLSvc_Prepare2withRowsets_sme_(
          /* In    */ Long dialogueId
        , /* In    */ Int32 sqlAsyncEnable
        , /* In    */ Int32 queryTimeout
        , /* In    */ Int32 inputRowCnt
        , /* In    */ Int32 sqlStmtType
        , /* In    */ Int32 stmtLength
        , /* In    */ const IDL_char *stmtLabel
        , /* In    */ Int32 stmtLabelCharset
        , /* In    */ Int32 cursorLength
        , /* In    */ IDL_string cursorName
        , /* In    */ Int32 cursorCharset
        , /* In    */ Int32 moduleNameLength
        , /* In    */ const IDL_char *moduleName
        , /* In    */ Int32 moduleCharset
        , /* In    */ Int64 moduleTimestamp
        , /* In    */ Int32 sqlStringLength
        , /* In    */ IDL_string sqlString
        , /* In    */ Int32 sqlStringCharset
        , /* In    */ Int32 setStmtOptionsLength
        , /* In    */ IDL_string setStmtOptions
        , /* In    */ Int32 holdableCursor
        , /* Out   */ Int32 *returnCode
        , /* Out   */ Int32 *sqlWarningOrErrorLength
        , /* Out   */ BYTE *&sqlWarningOrError
        , /* Out   */ Int32 *sqlQueryType
        , /* Out   */ Long *stmtHandle
        , /* Out   */ Int32 *estimatedCost
        , /* Out   */ Int32 *inputDescLength
        , /* Out   */ BYTE *&inputDesc
        , /* Out   */ Int32 *outputDescLength
        , /* Out   */ BYTE *&outputDesc
        );

extern "C" void
odbc_SQLSvc_Prepare2_sme_(
        /* In    */ Long dialogueId
        , /* In    */ Int32 inputRowCnt
        , /* In    */ Int32 sqlStmtType
        , /* In    */ const IDL_char *stmtLabel
        , /* In    */ IDL_string sqlString
        , /* In    */ Int32 holdableCursor
        , /* Out   */ Int32 *returnCode
        , /* Out   */ Int32 *sqlWarningOrErrorLength
        , /* Out   */ BYTE *&sqlWarningOrError
        , /* Out   */ Int32 *sqlQueryType
        , /* Out   */ Long *stmtHandle
        , /* Out   */ Int32 *estimatedCost
        , /* Out   */ Int32 *inputDescLength
        , /* Out   */ BYTE *&inputDesc
        , /* Out   */ Int32 *outputDescLength
        , /* Out   */ BYTE *&outputDesc
        , /* In    */ bool isFromExecDirect = false
        );

extern "C" void 
rePrepare2( SRVR_STMT_HDL *pSrvrStmt
        , Int32           sqlStmtType
        , Int32           inputRowCnt
        , Int32			holdableCursor
        , SQLRETURN     	*rc 
        , Int32          	*returnCode
        , Int32      		*sqlWarningOrErrorLength
        , BYTE          	*&sqlWarningOrError );

extern "C" void
odbc_SQLSvc_Execute2withRowsets_sme_(
        /* In    */ Long dialogueId
        , /* In    */ Int32 sqlAsyncEnable
        , /* In    */ Int32 queryTimeout
        , /* In    */ Int32 inputRowCnt
        , /* In    */ Int32 sqlStmtType
        , /* In    */ Long stmtHandle
        , /* In    */ Int32 cursorLength
        , /* In    */ IDL_string cursorName
        , /* In    */ Int32 cursorCharset
        , /* In    */ Int32 holdableCursor
        , /* In    */ Int32 inValuesLength
        , /* In    */ BYTE *inValues
        , /* Out   */ Int32 *returnCode
        , /* Out   */ Int32 *sqlWarningOrErrorLength
        , /* Out   */ BYTE *&sqlWarningOrError
        , /* Out   */ Int32 *rowsAffected
        , /* Out   */ Int32 *outValuesLength
        , /* Out   */ BYTE *&outValues);

extern "C" void
odbc_SQLSvc_Execute2_sme_(
        /* In    */ Long dialogueId
        , /* In    */ Int32 sqlAsyncEnable
        , /* In    */ Int32 queryTimeout
        , /* In    */ Int32 inputRowCnt
        , /* In    */ Int32 sqlStmtType
        , /* In    */ Long stmtHandle
        , /* In    */ Int32 cursorLength
        , /* In    */ IDL_string cursorName
        , /* In    */ Int32 cursorCharset
        , /* In    */ Int32 holdableCursor
        , /* In    */ Int32 inValuesLength
        , /* In    */ BYTE *inValues
        , /* Out   */ Int32 *returnCode
        , /* Out   */ Int32 *sqlWarningOrErrorLength
        , /* Out   */ BYTE *&sqlWarningOrError
        , /* Out   */ Int32 *rowsAffected
        , /* Out   */ Int32 *outValuesLength
        , /* Out   */ BYTE *&outValues);

/*
* Exception number constants for
* operation 'odbc_SQLSvc_ExecuteN'
*/
#define odbc_SQLSvc_ExecuteN_ParamError_exn_ 1
#define odbc_SQLSvc_ExecuteN_InvalidConnection_exn_ 2
#define odbc_SQLSvc_ExecuteN_SQLError_exn_ 3
#define odbc_SQLSvc_ExecuteN_SQLInvalidHandle_exn_ 4
#define odbc_SQLSvc_ExecuteN_SQLNeedData_exn_ 5
#define odbc_SQLSvc_ExecuteN_SQLNoDataFound_exn_ 6
#define odbc_SQLSvc_ExecuteN_SQLRetryCompile_exn_ 7
#define odbc_SQLSvc_ExecuteN_SQLStillExecuting_exn_ 8
#define odbc_SQLSvc_ExecuteN_SQLQueryCancelled_exn_ 9
#define odbc_SQLSvc_ExecuteN_TransactionError_exn_ 10

extern "C" void
odbc_SQLSvc_ExecuteN_sme_(
						  /* In    */ void * objtag_
						  , /* In    */ const CEE_handle_def *call_id_
						  , /* Out   */ ExceptionStruct *exception_
						  , /* In    */ long dialogueId
						  , /* In    */ long stmtId
						  , /* In    */ char *cursorName
						  , /* In    */ short sqlStmtType
						  , /* In    */ long totalRowCount
						  , /* In    */ const SQLValueList_def *inputValueList
						  , /* In    */ short sqlAsyncEnable
						  , /* In    */ long queryTimeout
						  , /* Out   */ SQLValueList_def *outputValueList
						  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
						  );

/*
 *  * Local index for operation 'odbc_SQLSvc_ExecuteN'
 *   */
#define odbc_SQLSvc_ExecuteN_ldx_ ((IDL_unsigned_long) 8)


/*
* Exception number constants for
* operation 'odbc_SQLSvc_ExecDirect'
*/
#define odbc_SQLSvc_ExecDirect_ParamError_exn_ 1
#define odbc_SQLSvc_ExecDirect_InvalidConnection_exn_ 2
#define odbc_SQLSvc_ExecDirect_SQLError_exn_ 3
#define odbc_SQLSvc_ExecDirect_SQLStillExecuting_exn_ 4
#define odbc_SQLSvc_ExecDirect_SQLQueryCancelled_exn_ 5
#define odbc_SQLSvc_ExecDirect_TransactionError_exn_ 6
#define odbc_SQLSvc_ExecDirect_SQLInvalidHandle_exn_ 7

/*
 * Exception union for
 * operation 'odbc_SQLSvc_ExecDirect'
 */
struct odbc_SQLSvc_ExecDirect_exc_ {
    IDL_long exception_nr;
    IDL_long exception_detail;
    union {
        odbc_SQLSvc_ParamError ParamError;
        odbc_SQLSvc_SQLError SQLError;
        odbc_SQLSvc_SQLQueryCancelled SQLQueryCancelled;
    } u;
};
/*
 * Local index for operation 'odbc_SQLSvc_ExecDirect'
 */
#define odbc_SQLSvc_ExecDirect_ldx_ ((IDL_unsigned_long) 7)
/*
 * Operation synopsis for operation 'odbc_SQLSvc_ExecDirect'
 */
#define odbc_SQLSvc_ExecDirect_osy_ ((IDL_long) -1127826003)

extern "C" void
odbc_SQLSvc_ExecDirect_sme_(
							/* In	*/ void * objtag_
							, /* In	*/ const CEE_handle_def *call_id_
							, /* Out   */ ExceptionStruct *exception_
							, /* In	*/ long dialogueId
							, /* In	*/ const char *stmtLabel
							, /* In	*/ char *cursorName
							, /* In	*/ const char *stmtExplainLabel
							, /* In	*/ short stmtType
							, /* In	*/ short sqlStmtType
							, /* In	*/ const SQLValue_def *sqlString
							, /* In	*/ short holdability
							, /* In	*/ long queryTimeout
							, /* In	*/ long resultSet
							, /* Out   */ long *estimatedCost
							, /* Out   */ SQLItemDescList_def *outputDesc
							, /* Out   */ long *rowsAffected
							, /* Out   */ ERROR_DESC_LIST_def *sqlWarning
							, /* Out   */ long *stmtId
							,/* In*/long currentStmtId
							);

/*
* Exception number constants for
* operation 'jdbc_SQLSvc_ExecSPJRS'
*/
#define jdbc_SQLSvc_ExecSPJRS_ParamError_exn_ 1
#define jdbc_SQLSvc_ExecSPJRS_InvalidConnection_exn_ 2
#define jdbc_SQLSvc_ExecSPJRS_SQLError_exn_ 3
#define jdbc_SQLSvc_ExecSPJRS_SQLStillExecuting_exn_ 4
#define jdbc_SQLSvc_ExecSPJRS_SQLQueryCancelled_exn_ 5
#define jdbc_SQLSvc_ExecSPJRS_TransactionError_exn_ 6
#define jdbc_SQLSvc_ExecSPJRS_SQLInvalidHandle_exn_ 7
#define jdbc_SQLSvc_ExecSPJRS_NonexistentRS_exn_ 8

extern "C" void
jdbc_SQLSvc_ExecSPJRS_sme_(
						   /* In	*/ void *objtag_
						   , /* In	*/ const CEE_handle_def *call_id_
						   , /* Out   */ ExceptionStruct *exception_
						   , /* In	*/ long dialogueId
						   , /* In	*/ const char *stmtLabel
						   , /* In	*/ const char *RSstmtLabel
						   , /* In	*/ short stmtType
						   , /* In	*/ short sqlStmtType
						   , /* In	*/ long resultSet
						   , /* In   */ long ResultSetIndex
						   , /* Out   */ SQLItemDescList_def *outputDesc
						   , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
						   , /* Out   */ long *RSstmtId
						   , long stmtId
						   );

/*
* Exception number constants for
* operation 'odbc_SQLSvc_CancelStatement'
*/
#define odbc_SQLSvc_CancelStatement_ParamError_exn_ 1
#define odbc_SQLSvc_CancelStatement_InvalidConnection_exn_ 2
#define odbc_SQLSvc_CancelStatement_SQLError_exn_ 3
#define odbc_SQLSvc_CancelStatement_SQLInvalidHandle_exn_ 4

extern "C" void
odbc_SQLSvc_CancelStatement_sme_(
								 /* In	*/ void * objtag_
								 , /* In	*/ const CEE_handle_def *call_id_
								 , /* Out   */ ExceptionStruct *exception_
								 , /* In	*/ long dialogueId
								 , /* In	*/ long stmtId
								 , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
								 );

/*
* Exception number constants for
* operation 'odbc_SQLSvc_SetConnectionOption'
*/
#define odbc_SQLSvc_SetConnectionOption_ParamError_exn_ 1
#define odbc_SQLSvc_SetConnectionOption_InvalidConnection_exn_ 2
#define odbc_SQLSvc_SetConnectionOption_SQLError_exn_ 3
#define odbc_SQLSvc_SetConnectionOption_SQLInvalidHandle_exn_ 4

extern "C" void
odbc_SQLSvc_SetConnectionOption_sme_(
									 /* In	*/ void * objtag_
									 , /* In	*/ const CEE_handle_def *call_id_
									 , /* Out   */ ExceptionStruct *exception_
									 , /* In	*/ long dialogueId
									 , /* In	*/ short connectionOption
									 , /* In	*/ long  optionValueNum
									 , /* In	*/ char *optionValueStr
									 , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
									 );

/*
* Exception number constants for
* operation 'odbc_SQLSvc_PrepareFromModule'
*/
#define odbc_SQLSvc_PrepareFromModule_ParamError_exn_ 1
#define odbc_SQLSvc_PrepareFromModule_InvalidConnection_exn_ 2
#define odbc_SQLSvc_PrepareFromModule_SQLError_exn_ 3
#define odbc_SQLSvc_PrepareFromModule_SQLStillExecuting_exn_ 4
#define odbc_SQLSvc_PrepareFromModule_SQLQueryCancelled_exn_ 5
#define odbc_SQLSvc_PrepareFromModule_TransactionError_exn_ 6

extern "C" void
odbc_SQLSvc_PrepareFromModule_sme_(
								   /* In	*/ void * objtag_
								   , /* In	*/ const CEE_handle_def *call_id_
								   , /* Out   */ ExceptionStruct *exception_
								   , /* In	*/ long dialogueId
								   , /* In	*/ char *moduleName
								   , /* In	*/ long moduleVersion
								   , /* In	*/ long long moduleTimestamp
								   , /* In	*/ char *stmtName
								   , /* In	*/ short sqlStmtType
								   , /* In	*/ long fetchSize
								   , /* In   */ long batchSize
								   , /* In   */ long holdability
								   , /* Out   */ long *estimatedCost
								   , /* Out   */ SQLItemDescList_def *inputDesc
								   , /* Out   */ SQLItemDescList_def *outputDesc
								   , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
								   , /* Out   */ long *stmtId
								   , /* Out   */ long *inputParamOffset
								   );

/*
* Exception number constants for
* operation 'odbc_SQLSvc_ExecuteCall'
*/
#define odbc_SQLSvc_ExecuteCall_ParamError_exn_ 1
#define odbc_SQLSvc_ExecuteCall_InvalidConnection_exn_ 2
#define odbc_SQLSvc_ExecuteCall_SQLError_exn_ 3
#define odbc_SQLSvc_ExecuteCall_SQLInvalidHandle_exn_ 4
#define odbc_SQLSvc_ExecuteCall_SQLNeedData_exn_ 5
#define odbc_SQLSvc_ExecuteCall_SQLRetryCompile_exn_ 6
#define odbc_SQLSvc_ExecuteCall_SQLStillExecuting_exn_ 7
#define odbc_SQLSvc_ExecuteCall_SQLQueryCancelled_exn_ 8
#define odbc_SQLSvc_ExecuteCall_TransactionError_exn_ 9

/*
 *  * Exception union for
 *   * operation 'odbc_SQLSvc_ExecuteCall'
 *    */
struct odbc_SQLSvc_ExecuteCall_exc_ {
    IDL_long exception_nr;
    IDL_long exception_detail;
    union {
        odbc_SQLSvc_ParamError ParamError;
        odbc_SQLSvc_SQLError SQLError;
        odbc_SQLSvc_SQLRetryCompile SQLRetryCompile;
        odbc_SQLSvc_SQLQueryCancelled SQLQueryCancelled;
    } u;
};
/*
 *  * Local index for operation 'odbc_SQLSvc_ExecuteCall'
 *   */
#define odbc_SQLSvc_ExecuteCall_ldx_ ((IDL_unsigned_long) 28)
/*
 *  * Operation synopsis for operation 'odbc_SQLSvc_ExecuteCall'
 *   */
#define odbc_SQLSvc_ExecuteCall_osy_ ((IDL_long) -1549026474)

extern "C" void
odbc_SQLSvc_ExecuteCall_sme_(
        /* In	*/ void * objtag_
        , /* In	*/ const CEE_handle_def *call_id_
        , /* Out   */ ExceptionStruct *exception_
        , /* In	*/ long dialogueId
        , /* In	*/ long stmtId
        , /* In	*/ const SQLValueList_def *inputValueList
        , /* In	*/ short sqlAsyncEnable
        , /* In	*/ long queryTimeout
        , /* Out   */ SQLValueList_def *outputValueList
        , /* Out   */ short *returnResult
        , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
        );

/*
 * Exception number constants for
* operation 'odbc_SQLSvc_CloseUsingLabel'
*/
#define odbc_SQLSvc_CloseUsingLabel_ParamError_exn_ 1
#define odbc_SQLSvc_CloseUsingLabel_InvalidConnection_exn_ 2
#define odbc_SQLSvc_CloseUsingLabel_SQLError_exn_ 3
#define odbc_SQLSvc_CloseUsingLabel_TransactionError_exn_ 4

extern "C" void
odbc_SQLSvc_CloseUsingLabel_sme_(
								 /* In	*/ void * objtag_
								 , /* In	*/ const CEE_handle_def *call_id_
								 , /* Out   */ ExceptionStruct *exception_
								 , /* In	*/ long dialogueId
								 , /* In	*/ char *stmtName
								 , /* In	*/ unsigned short freeResourceOpt
								 , /* Out   */ long *rowsAffected
								 , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
								 );

//
// Actually defined in SrvrSmd.cpp
//
extern "C" void
odbc_SQLSvc_GetSQLCatalogs_sme_(
								/* In	*/ void * objtag_
								, /* In	*/ const CEE_handle_def *call_id_
								, /* Out   */ ExceptionStruct *exception_
								, /* In	*/ long dialogueId
								, /* In	*/ short APIType
								, /* In	*/ const char *catalogNm
								, /* In	*/ const char *schemaNm
								, /* In	*/ const char *tableNm
								, /* In	*/ const char *tableTypeList
								, /* In	*/ const char *columnNm
								, /* In	*/ long columnType
								, /* In	*/ long rowIdScope
								, /* In	*/ long nullable
								, /* In	*/ long uniqueness
								, /* In	*/ long accuracy
								, /* In	*/ short sqlType
								, /* In	*/ unsigned long metadataId
								, /* Out   */ char *catStmtLabel
								, /* Out   */ SQLItemDescList_def *outputDesc
								, /* Out   */ ERROR_DESC_LIST_def *sqlWarning
								, /* Out   */ long *rowsAffected
								, /* Out   */ SQLValueList_def *outputValueList
								, /* Out   */ long *stmtId
								, /* In    */ const char *fkcatalogNm
								, /* In    */ const char *fkschemaNm
								, /* In    */ const char *fktableNm
								);

//extern std::map<std::string, std::string> mapOfSQLToModuleFile;

#endif // _SRVROTHERS_DEFINED
# define SET_SESSION_INTERNAL_IO 197  //MFC support for BigNum
