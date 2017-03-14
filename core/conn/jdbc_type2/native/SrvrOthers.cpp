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
// MODULE: SrvrOther.cpp
//
// PURPOSE: Implements the following methods
//      odbc_SQLSvc_Prepare_sme_
//      odbc_SQLSvc_ExecuteN_sme_
//      odbc_SQLSvc_Close_sme_
//      odbc_SQLSvc_FetchN_sme_
//      odbc_SQLSvc_EndTransaction_sme_
//

#include <platform_ndcs.h>
#include <stdio.h>
#include <sql.h>
#include <sqlext.h>
#include "SrvrCommon.h"
#include "CoreCommon.h"
#include "SrvrFunctions.h"
#include "SrvrKds.h"
#include "SqlInterface.h"
#include "CommonDiags.h"
#include "Debug.h"
#include "SQLWrapper.h"

using namespace SRVR;

// +++ T2_REPO - ToDo needs to be initialized
#include "ResStatisticsSession.h"
ResStatisticsSession    *resStatSession = NULL;
//

#ifdef NSK_PLATFORM     // Linux port - Todo
SMD_SELECT_TABLE SQLCommitStmt[] = {
    { STRING_TYPE, "COMMIT WORK"},
    { END_OF_TABLE}
};

SMD_SELECT_TABLE SQLRollbackStmt[] = {
    { STRING_TYPE, "ROLLBACK WORK"},
    { END_OF_TABLE}
};

SMD_QUERY_TABLE tranQueryTable[] = {
    {"SQL_COMMIT", SQLCommitStmt, TYPE_UNKNOWN, FALSE, FALSE},
    {"SQL_ROLLBACK", SQLRollbackStmt, TYPE_UNKNOWN, FALSE, FALSE},
    {NULL}
};
#endif

//Start Soln no:10-091103-5969
extern jboolean getSqlStmtType(unsigned char* sql);
//End Soln no:10-091103-5969

/*
* Synchronous method function  for
* operation 'odbc_SQLSvc_Prepare'
*/
extern "C" void
odbc_SQLSvc_Prepare_sme_(   void *               objtag_,           /* In   */
                         const CEE_handle_def     *call_id_,            /* In   */
                         ExceptionStruct             *exception_,       /* Out  */
                         long                      dialogueId,      /* In   */
                         const char           *stmtLabel,       /* In   */
                         const char           *stmtExplainLabel,    /* In   */
                         short                 stmtType,            /* In   */
                         const SQLValue_def       *sqlString,       /* In the SQL statement */
                         short                 holdability,     /* In cursor holdability true/false */
                         short                 sqlStmtType,     /* In SQL statment is a SELECT, INVOKE, or */
                         long                  batchSize,       /* In   */
                         long                  fetchSize,       /* In   */
                         long                  queryTimeout,        /* In   */
                         long                 *estimatedCost,   /* Out Not used */
                         SQLItemDescList_def      *inputDesc,       /* Out  */
                         SQLItemDescList_def      *outputDesc,      /* Out  */
                         ERROR_DESC_LIST_def      *sqlWarning,      /* Out  */
                         long                     *stmtId,          /* Out  */
                         long                 *inputParamOffset,     /* Out   */
                         char                 *moduleName,
                         bool isISUD)

{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_STMT, "odbc_SQLSvc_Prepare_sme_",(""));

    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  objtag_=0x%08x, call_id_=0x%08x, exception_=0x%08x",
        objtag_,
        call_id_,
        exception_));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  dialogueId=0x%08x, stmtLabel=%s, stmtExplainLabel=%s",
        dialogueId,
        DebugString(stmtLabel),
        DebugString(stmtExplainLabel)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  stmtType=%s, sqlString='%s', holdability=%d, isISUD=%d",
        CliDebugStatementType(stmtType),
        CLI_SQL_VALUE_STR(sqlString),
        holdability,isISUD));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("   sqlStmtType=%s, batchSize=%ld, fetchSize=%ld, queryTimeout=%ld",
        CliDebugSqlStatementType(sqlStmtType),
        batchSize,
        fetchSize,
        queryTimeout));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inputDesc=0x%08x, outputDesc=0x%08x, sqlWarning=0x%08x",
        inputDesc,
        outputDesc,
        sqlWarning));
    SRVR_STMT_HDL *pSrvrStmt;
    SQLRETURN      rc;
    long           sqlcode;

#ifdef _ODBCMX_TRACE
    if(diagnostic_flags){
        TraceOut(TR_SRVR_KRYPTON_API,"odbc_SQLSvc_Prepare_sme_(%#x, %#x, %#x, %ld, %s, %s, %d, %s, %d, %ld, %#x, %#x, %#x, %#x, %#x)",
            objtag_,
            call_id_,
            exception_,
            dialogueId,
            stmtLabel,
            stmtExplainLabel,
            stmtType,
            sqlString,
            holdability,
            queryTimeout,
            estimatedCost,
            inputDesc,
            outputDesc,
            sqlWarning,
            stmtId);
    }
#endif

    if (sqlString ==  NULL){
        exception_->exception_nr = odbc_SQLSvc_Prepare_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_NULL_SQL_STMT;
        FUNCTION_RETURN_VOID(("sqlString is  NULL"));
    }
    // Need to validate the stmtLabel
    // Given a label find out the SRVR_STMT_HDL
    pSrvrStmt = createSrvrStmt(dialogueId,
        stmtLabel,
        &sqlcode,
        NULL,
        0,
        0,
        sqlStmtType,
        false);
    if (pSrvrStmt == NULL ){
        exception_->exception_nr = odbc_SQLSvc_Prepare_SQLInvalidHandle_exn_;
        exception_->u.SQLInvalidHandle.sqlcode = sqlcode;
        FUNCTION_RETURN_VOID(("createSrvrStmt() Failed"));
    }

    // Setup the output descriptors using the fetch size
    pSrvrStmt->resetFetchSize(fetchSize);

    rc = pSrvrStmt->setMaxBatchSize(batchSize);
    if (rc==SQL_SUCCESS){
    //Start Soln no:10-091103-5969
        jboolean stmtType_ = getSqlStmtType(sqlString->dataValue._buffer);
        if(stmtType_ == JNI_TRUE && batchSize > 0 || srvrGlobal->moduleCaching == 0)
        {
            rc = pSrvrStmt->Prepare(sqlString, stmtType, holdability, queryTimeout,isISUD);
        }
        else
        {
            //rc = pSrvrStmt->PrepareforMFC(sqlString, stmtType, holdability, queryTimeout,isISUD);
        }
    //End Soln no:10-091103-5969
    }

    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        exception_->exception_nr = 0;
        // Copy all the output parameters
        *estimatedCost      = pSrvrStmt->estimatedCost;
        inputDesc->_length  = pSrvrStmt->inputDescList._length;
        inputDesc->_buffer  = pSrvrStmt->inputDescList._buffer;
        outputDesc->_length = pSrvrStmt->outputDescList._length;
        outputDesc->_buffer = pSrvrStmt->outputDescList._buffer;
        sqlWarning->_length = pSrvrStmt->sqlWarning._length;
        sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
        *stmtId             = (long)pSrvrStmt;
        *inputParamOffset   = pSrvrStmt->inputDescParamOffset;
        break;
    case SQL_STILL_EXECUTING:
        exception_->exception_nr = odbc_SQLSvc_Prepare_SQLStillExecuting_exn_;
        break;

    case ODBC_RG_ERROR:
    case SQL_ERROR:
        ERROR_DESC_def *error_desc_def;
        error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
        if (pSrvrStmt->sqlError.errorList._length != 0 &&
            (error_desc_def->sqlcode == -8007 ||
            error_desc_def->sqlcode == -8007)){
                exception_->exception_nr = odbc_SQLSvc_Prepare_SQLQueryCancelled_exn_;
                exception_->u.SQLQueryCancelled.sqlcode = error_desc_def->sqlcode;
        }
        else{
            exception_->exception_nr = odbc_SQLSvc_Prepare_SQLError_exn_;
            exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
            exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
        }
        break;
    case PROGRAM_ERROR:
        exception_->exception_nr = odbc_SQLSvc_Prepare_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_PREPARE_FAILED;
    default:
        break;
    }
    FUNCTION_RETURN_VOID((NULL));
}


/*
* Synchronous method function for
* operation 'odbc_SQLSvc_ExecuteN'
*/
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
                          , /* Out   */ ERROR_DESC_LIST_def *sqlWarning)
{
    FUNCTION_ENTRY("odbc_SQLSvc_ExecuteN_sme_",(""));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  objtag_=0x%08x, call_id_=0x%08x, exception_=0x%08x",
        objtag_,
        call_id_,
        exception_));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  dialogueId=0x%08x, stmtId=0x%08x",
        dialogueId,
        stmtId));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  cursorName=%s, sqlStmtType=%s, totalRowCount=%ld",
        DebugString(cursorName),
        CliDebugSqlStatementType(sqlStmtType),
        totalRowCount));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inputValueList=0x%08x, sqlAsyncEnable=%d, queryTimeout=%ld, outputValueList=0x%08x, sqlWarning=0x%08x",
        inputValueList,
        sqlAsyncEnable,
        queryTimeout,
        outputValueList,
        sqlWarning));

    SRVR_STMT_HDL *pSrvrStmt;
    SQLRETURN rc;
    long    sqlcode;

    if ((pSrvrStmt = getSrvrStmt(dialogueId, stmtId, &sqlcode)) == NULL)
    {
        exception_->exception_nr = odbc_SQLSvc_ExecuteN_SQLInvalidHandle_exn_;
        exception_->u.SQLInvalidHandle.sqlcode = sqlcode;
        FUNCTION_RETURN_VOID(("getSrvrStmt() Failed"));
    }

    // Clear out any previous rowcount results
    //  MEMORY_DELETE(pSrvrStmt->rowCount._buffer);
    pSrvrStmt->rowCount._length = 0;

    if (totalRowCount <= 0)
    {
        exception_->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_ROW_COUNT;
        FUNCTION_RETURN_VOID(("totalRowCount <= 0"));
    }

    if ((sqlStmtType & TYPE_SELECT) && totalRowCount > 1)
    {
        exception_->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_ROW_COUNT_AND_SELECT;
        FUNCTION_RETURN_VOID(("sqlStmtType is TYPE_SELECT && totalRowCount > 1"));
    }


    /// For Modius.
    rc = pSrvrStmt->switchContext();
    if(rc == SQL_SUCCESS)
    {
        CLI_ClearDiagnostics(&pSrvrStmt->stmt);
    }
    rc = pSrvrStmt->Execute(cursorName, totalRowCount, sqlStmtType, inputValueList, sqlAsyncEnable,
        queryTimeout, outputValueList);

    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        exception_->exception_nr = 0;
        sqlWarning->_length = pSrvrStmt->sqlWarning._length;
        sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
        break;
    case SQL_STILL_EXECUTING:
        exception_->exception_nr = odbc_SQLSvc_ExecuteN_SQLStillExecuting_exn_;
        break;
    case SQL_NEED_DATA:
        exception_->exception_nr = odbc_SQLSvc_ExecuteN_SQLNeedData_exn_;
        break;
    case SQL_NO_DATA_FOUND:
        exception_->exception_nr = odbc_SQLSvc_ExecuteN_SQLNoDataFound_exn_;
        break;
    case ODBC_SERVER_ERROR:
    case SQL_ERROR:
        ERROR_DESC_def *error_desc_def;
        error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
        if (pSrvrStmt->sqlError.errorList._length != 0 &&
            (error_desc_def->sqlcode == -8007 || error_desc_def->sqlcode == -8007))
        {
            exception_->exception_nr = odbc_SQLSvc_ExecuteN_SQLQueryCancelled_exn_;
            exception_->u.SQLQueryCancelled.sqlcode = error_desc_def->sqlcode;
        }
        else
        {
            exception_->exception_nr = odbc_SQLSvc_ExecuteN_SQLError_exn_;
            exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
            exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
        }
        break;
    case -8814:
    case 8814:
        // SQL Error/Warning 8814: The transaction mode at run time (value) differs
        // from that specified at compile time (value). 8814 is translated to an
        // odbc_SQLSvc_ExecuteCall_SQLRetryCompile_exn_ exception.
        exception_->exception_nr = odbc_SQLSvc_ExecuteN_SQLRetryCompile_exn_;
        break;
    case PROGRAM_ERROR:
        exception_->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECUTE_FAILED;
    default:
        break;
    }
    FUNCTION_RETURN_VOID((NULL));
}


/*
* Synchronous method function for
* operation 'odbc_SQLSvc_Close'
*/
extern "C" void
odbc_SQLSvc_Close_sme_(
                       /* In    */ void * objtag_
                       , /* In  */ const CEE_handle_def *call_id_
                       , /* Out   */ ExceptionStruct *exception_
                       , /* In  */ long dialogueId
                       , /* In  */ long stmtId
                       , /* In  */ unsigned short freeResourceOpt
                       , /* Out   */ long *rowsAffected
                       , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
                       )
{
    FUNCTION_ENTRY("odbc_SQLSvc_Close_sme_",("objtag_=0x%08x, call_id_=0x%08x, exception_=0x%08x, dialogueId=0x%08x, stmtId=0x%08x, freeResourceOpt=%d, rowsAffected=0x%08x, sqlWarning=0x%08x",
        objtag_,
        call_id_,
        exception_,
        dialogueId,
        stmtId,
        freeResourceOpt,
        rowsAffected,
        sqlWarning));

    SRVR_STMT_HDL *pSrvrStmt;
    SQLRETURN rc;
    long    sqlcode;

#ifdef _ODBCMX_TRACE
    if(diagnostic_flags){
        TraceOut(TR_SRVR_KRYPTON_API,"odbc_SQLSvc_Close_sme_(%#x, %#x, %#x, %ld, %#x, %d, %#x, %#x)",
            objtag_,
            call_id_,
            exception_,
            dialogueId,
            stmtId,
            freeResourceOpt,
            rowsAffected,
            sqlWarning);
    }
#endif
    if (freeResourceOpt != SQL_CLOSE && freeResourceOpt != SQL_DROP &&
        freeResourceOpt != SQL_UNBIND && freeResourceOpt != SQL_RESET_PARAMS)
    {
        exception_->exception_nr = odbc_SQLSvc_Close_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_RESOURCE_OPT_CLOSE;
        FUNCTION_RETURN_VOID(("freeResourceOpt is not SQL_CLOSE,SQL_DROP,SQL_UNBIND or SQL_RESET_PARAMS"));
    }

    if ((pSrvrStmt = getSrvrStmt(dialogueId, stmtId, &sqlcode)) == NULL)
    {
        *rowsAffected = -1;
        exception_->exception_nr = 0;
        sqlWarning->_buffer = NULL;
        sqlWarning->_length = 0;
        FUNCTION_RETURN_VOID(("getSrvrStmt() Statement not found"));
    }

    rc = pSrvrStmt->Close(freeResourceOpt);
    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        exception_->exception_nr = 0;
        if (freeResourceOpt != SQL_DROP)
        {
            *rowsAffected = pSrvrStmt->rowsAffected;
            sqlWarning->_length = pSrvrStmt->sqlWarning._length;
            sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
        }
        else
        {
            *rowsAffected = 0;
            sqlWarning->_length = 0;
            sqlWarning->_buffer = NULL;
        }
        break;
    case SQL_ERROR:
        exception_->exception_nr = odbc_SQLSvc_Close_SQLError_exn_;
        exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
        exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
        break;
    case PROGRAM_ERROR:
        exception_->exception_nr = odbc_SQLSvc_Close_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_CLOSE_FAILED;
    default:
        break;
    }
    FUNCTION_RETURN_VOID(("*rowsAffected=%ld",*rowsAffected));
}


/*
* Synchronous method function for
* operation 'odbc_SQLSvc_FetchN'
*/
extern "C" void
odbc_SQLSvc_FetchN_sme_(
                        /* In   */ void * objtag_
                        , /* In */ const CEE_handle_def *call_id_
                        , /* Out   */ ExceptionStruct *exception_
                        , /* In */ long dialogueId
                        , /* In */ long stmtId
                        , /* In */ long maxRowCnt
                        , /* In */ short sqlAsyncEnable
                        , /* In */ long queryTimeout
                        , /* Out   */ long *rowsAffected
                        , /* Out   */ SQLValueList_def *outputValueList
                        , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
                        )
{
    FUNCTION_ENTRY("odbc_SQLSvc_FetchN_sme_",("..."));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  objtag_=0x%08x, call_id_=0x%08x, exception_=0x%08x",
        objtag_,
        call_id_,
        exception_));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  dialogueId=0x%08x, stmtId=0x%08x, maxRowCnt=%ld",
        dialogueId,
        stmtId,
        maxRowCnt));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  sqlAsyncEnable=%d, queryTimeout=%ld, outputValueList=0x%08x, sqlWarning=0x%08x",
        sqlAsyncEnable,
        queryTimeout,
        outputValueList,
        sqlWarning));

    SRVR_STMT_HDL *pSrvrStmt;
    SQLRETURN       rc;
    long            sqlcode;

#ifdef _ODBCMX_TRACE
    if(diagnostic_flags){
        TraceOut(TR_SRVR_KRYPTON_API,"odbc_SQLSvc_FetchN_sme_(%#x, %#x, %#x, %ld, %s, %ld, %d, %ld, %#x, %#x, %#x)",
            objtag_,
            call_id_,
            exception_,
            dialogueId,
            stmtLabel,
            maxRowCnt,
            sqlAsyncEnable,
            queryTimeout,
            rowsAffected,
            outputValueList,
            sqlWarning);
    }
#endif
    if (maxRowCnt < 0)
    {
        exception_->exception_nr = odbc_SQLSvc_FetchN_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_ROW_COUNT;
        FUNCTION_RETURN_VOID(("maxRowCnt < 0"));
    }
    if ((pSrvrStmt = getSrvrStmt(dialogueId, stmtId, &sqlcode)) == NULL)
    {
        exception_->exception_nr = odbc_SQLSvc_FetchN_SQLInvalidHandle_exn_;
        exception_->u.SQLInvalidHandle.sqlcode = sqlcode;
        FUNCTION_RETURN_VOID(("getSrvrStmt() Failed"));
    }

    /// For Modius.
    rc = pSrvrStmt->switchContext();
    if(rc == SQL_SUCCESS)
    {
        rc = pSrvrStmt->Fetch(maxRowCnt, sqlAsyncEnable, queryTimeout);
    }

    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        exception_->exception_nr = 0;
        *rowsAffected = pSrvrStmt->rowsAffected;
        outputValueList->_length = pSrvrStmt->outputValueList._length;
        outputValueList->_buffer = pSrvrStmt->outputValueList._buffer;
        sqlWarning->_length = pSrvrStmt->sqlWarning._length;
        sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
        break;
    case SQL_STILL_EXECUTING:
        exception_->exception_nr = odbc_SQLSvc_FetchN_SQLStillExecuting_exn_;
        break;
    case SQL_INVALID_HANDLE:
        exception_->exception_nr = odbc_SQLSvc_FetchN_SQLInvalidHandle_exn_;
        break;
    case SQL_NO_DATA_FOUND:
        exception_->exception_nr = odbc_SQLSvc_FetchN_SQLNoDataFound_exn_;
        break;
    case SQL_ERROR:
        ERROR_DESC_def *error_desc_def;
        error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
        if (pSrvrStmt->sqlError.errorList._length != 0 &&
            (error_desc_def->sqlcode == -8007 || error_desc_def->sqlcode == -8007))
        {
            exception_->exception_nr = odbc_SQLSvc_FetchN_SQLQueryCancelled_exn_;
            exception_->u.SQLQueryCancelled.sqlcode = error_desc_def->sqlcode;
        }
        else
        {
            exception_->exception_nr = odbc_SQLSvc_FetchN_SQLError_exn_;
            exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
            exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
        }
        break;
    case PROGRAM_ERROR:
        exception_->exception_nr = odbc_SQLSvc_FetchN_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_FETCH_FAILED;
    default:
        break;
    }
    FUNCTION_RETURN_VOID((NULL));
}

/*
* Synchronous method function prototype for
* operation 'odbc_SQLSvc_ExecDirect'
*/
extern "C" void
odbc_SQLSvc_ExecDirect_sme_(
                            /* In   */ void * objtag_
                            , /* In */ const CEE_handle_def *call_id_
                            , /* Out   */ ExceptionStruct *exception_
                            , /* In */ long dialogueId
                            , /* In */ const char *stmtLabel
                            , /* In */ char *cursorName
                            , /* In */ const char *stmtExplainLabel
                            , /* In */ short stmtType
                            , /* In */ short sqlStmtType
                            , /* In */ const SQLValue_def *sqlString
                            , /* In */ short holdability
                            , /* In */ long queryTimeout
                            , /* In */ long resultSet
                            , /* Out   */ long *estimatedCost
                            , /* Out   */ SQLItemDescList_def *outputDesc
                            , /* Out   */ long *rowsAffected
                            , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
                            , /* Out   */ long *stmtId
                            , /* In*/long currentStmtId
                            )
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_STMT,"odbc_SQLSvc_ExecDirect_sme_",(""));

    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  objtag_=0x%08x, call_id_=0x%08x, exception_=0x%08x",
        objtag_,
        call_id_,
        exception_));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY|DEBUG_LEVEL_STMT,("  dialogueId=0x%08x, stmtLabel=%s, stmtId=0x%08x",
        dialogueId,
        DebugString(stmtLabel),
        stmtId));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY|DEBUG_LEVEL_STMT,("  cursorName=%s, stmtType=%s, sqlStmtType=%s",
        DebugString(cursorName),
        CliDebugStatementType(stmtType),
        CliDebugSqlStatementType(sqlStmtType)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY|DEBUG_LEVEL_STMT,("  sqlString='%s'",
        CLI_SQL_VALUE_STR(sqlString)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  holdability=%d, queryTimeout=%ld, resultSet=0x%08x, sqlWarning=0x%08x",
        holdability,
        queryTimeout,
        resultSet,
        sqlWarning));

    SRVR_STMT_HDL *pSrvrStmt;
    SQLRETURN rc;
    long        sqlcode;

#ifdef _ODBCMX_TRACE
    if(diagnostic_flags){
        TraceOut(TR_SRVR_KRYPTON_API,"odbc_SQLSvc_ExecDirect_sme_(%#x, %#x, %#x, %ld, %s, %s, %s, %d, %d, %s, %d, %ld, %#x, %#x. %#x, %#x, %#x)",
            objtag_,
            call_id_,
            exception_,
            dialogueId,
            stmtLabel,
            cursorName,
            stmtExplainLabel,
            stmtType,
            sqlStmtType,
            sqlString,
            holdability,
            queryTimeout,
            estimatedCost,
            outputDesc,
            rowsAffected,
            sqlWarning,
            stmtId);
    }
#endif
    if (sqlString ==  NULL)
    {
        exception_->exception_nr = odbc_SQLSvc_ExecDirect_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_NULL_SQL_STMT;
        FUNCTION_RETURN_VOID(("sqlString ==  NULL"));
    }

    // Need to validate the stmtLabel
    // Given a label find out the SRVR_STMT_HDL
    pSrvrStmt = createSrvrStmt(dialogueId,
        stmtLabel,
        &sqlcode,
        NULL,
        0,
        0,
        sqlStmtType,
        false,
        false,
        currentStmtId);
    if (pSrvrStmt == NULL)
    {
        exception_->exception_nr = odbc_SQLSvc_ExecDirect_SQLInvalidHandle_exn_;
        exception_->u.SQLInvalidHandle.sqlcode = sqlcode;
        FUNCTION_RETURN_VOID(("createSrvrStmt() Failed"));
    }
    pSrvrStmt->resultSetObject = (jobject) resultSet;
    rc = pSrvrStmt->ExecDirect(cursorName, sqlString, stmtType, sqlStmtType, holdability, queryTimeout);
    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        exception_->exception_nr = 0;
        *estimatedCost = pSrvrStmt->estimatedCost;
        *rowsAffected = pSrvrStmt->rowsAffected;
        if (pSrvrStmt->outputDescList._length == 0 && pSrvrStmt->sqlWarning._length == 0)
        {
            DEBUG_OUT(DEBUG_LEVEL_STMT,("Close Stmt --> pSrvrStmt=0x%08x",
                pSrvrStmt));
            pSrvrStmt->Close(SQL_DROP);
            outputDesc->_length = 0;
            outputDesc->_buffer = NULL;
            sqlWarning->_length = 0;
            sqlWarning->_buffer = NULL;
            *stmtId = NULL;
        }
        else
        {
            outputDesc->_length = pSrvrStmt->outputDescList._length;
            outputDesc->_buffer = pSrvrStmt->outputDescList._buffer;
            sqlWarning->_length = pSrvrStmt->sqlWarning._length;
            sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
            *stmtId = (long)pSrvrStmt;
        }
        break;
    case SQL_STILL_EXECUTING:
        exception_->exception_nr = odbc_SQLSvc_ExecDirect_SQLStillExecuting_exn_;
        break;
    case ODBC_RG_ERROR:
    case SQL_ERROR:
        ERROR_DESC_def *out_error_desc_def;
        ERROR_DESC_def *stmt_error_desc_def;
        *stmtId = (long)pSrvrStmt;
        stmt_error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
        if (pSrvrStmt->sqlError.errorList._length != 0 &&
            (stmt_error_desc_def->sqlcode == -8007 || stmt_error_desc_def->sqlcode == -8007))
        {
            exception_->exception_nr = odbc_SQLSvc_ExecDirect_SQLQueryCancelled_exn_;
            exception_->u.SQLQueryCancelled.sqlcode = stmt_error_desc_def->sqlcode;
        }
        else
        {
            int listLen = pSrvrStmt->sqlError.errorList._length;
            exception_->exception_nr = odbc_SQLSvc_ExecDirect_SQLError_exn_;
            exception_->u.SQLError.errorList._length = listLen;
            MEMORY_ALLOC_ARRAY(exception_->u.SQLError.errorList._buffer, ERROR_DESC_def, listLen);
            out_error_desc_def = exception_->u.SQLError.errorList._buffer;
            for(int i=0; i<listLen; i++, out_error_desc_def++, stmt_error_desc_def++) {
                int errTxtLen = strlen(stmt_error_desc_def->errorText) + 1;
                MEMORY_ALLOC_ARRAY(out_error_desc_def->errorText, char, errTxtLen);
                out_error_desc_def->sqlcode = stmt_error_desc_def->sqlcode;
                out_error_desc_def->rowId = stmt_error_desc_def->rowId;
                out_error_desc_def->errorDiagnosticId= stmt_error_desc_def->errorDiagnosticId;
                out_error_desc_def->operationAbortId = stmt_error_desc_def->operationAbortId;
                out_error_desc_def->errorCodeType = stmt_error_desc_def->errorCodeType;
                strcpy(out_error_desc_def->errorText, stmt_error_desc_def->errorText);
                strcpy(out_error_desc_def->sqlstate, stmt_error_desc_def->sqlstate);
            }
        }
        out_error_desc_def = NULL;
        stmt_error_desc_def = NULL;
        //SRVR_STMT_HDL Should be drop when error
        pSrvrStmt->Close(SQL_DROP);
        break;
    case PROGRAM_ERROR:
        exception_->exception_nr = odbc_SQLSvc_ExecDirect_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECDIRECT_FAILED;
        //SRVR_STMT_HDL Should be drop when error
        pSrvrStmt->Close(SQL_DROP);
    default:
        break;
    }
    FUNCTION_RETURN_VOID((NULL));
}


/*
* Synchronous method function prototype for
* operation 'jdbc_SQLSvc_ExecSPJRS'
*/
extern "C" void
jdbc_SQLSvc_ExecSPJRS_sme_(
                           /* In    */ void * objtag_
                           , /* In  */ const CEE_handle_def *call_id_
                           , /* Out   */ ExceptionStruct *exception_
                           , /* In  */ long dialogueId
                           , /* In  */ const char *stmtLabel
                           , /* In  */ const char *RSstmtLabel
                           , /* In  */ short stmtType
                           , /* In  */ short sqlStmtType
                           , /* In  */ long resultSet
                           , /* In   */ long ResultSetIndex
                           , /* Out   */ SQLItemDescList_def *outputDesc
                           , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
                           , /* Out   */ long *RSstmtId
                           , long stmtId
                           )
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_STMT, "jdbc_SQLSvc_ExecSPJRS_sme_",(""));

    DEBUG_OUT(DEBUG_LEVEL_ENTRY|DEBUG_LEVEL_STMT,("  objtag_=0x%08x, call_id_=0x%08x, exception_=0x%08x",
        objtag_,
        call_id_,
        exception_));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY|DEBUG_LEVEL_STMT,("  dialogueId=0x%08x, stmtLabel=%s, RSstmtLabel=%s, RSstmtId=0x%08x, stmtType=%s",
        dialogueId,
        DebugString(stmtLabel),
        DebugString(RSstmtLabel),
        RSstmtId,
        CliDebugStatementType(stmtType)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY|DEBUG_LEVEL_STMT,("  resultSet=0x%08x, ResultSetIndex=%ld, sqlWarning=0x%08x",
        resultSet,
        ResultSetIndex,
        sqlWarning));

    SRVR_STMT_HDL   *pSrvrStmt;
    SRVR_STMT_HDL   *dpSrvrStmt;
    SQLRETURN       rc;
    long            sqlcode;

    // Obtain the pSrvrStmt handle of the call stmt from the stmtLabel
    pSrvrStmt = createSrvrStmt(dialogueId,
        stmtLabel,
        &sqlcode,
        NULL,
        0,
        0,
        sqlStmtType,
        false,
        false,
        stmtId);
    if (pSrvrStmt == NULL)
    {
        exception_->exception_nr = jdbc_SQLSvc_ExecSPJRS_SQLInvalidHandle_exn_;
        exception_->u.SQLInvalidHandle.sqlcode = sqlcode;
        FUNCTION_RETURN_VOID(("createSrvrStmt() Failed"));
    }

    CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

    // Create a new "dummy" pSrvrStmt for SPJRS
    dpSrvrStmt = createSpjrsSrvrStmt(pSrvrStmt,
        dialogueId,
        RSstmtLabel,
        &sqlcode,
        NULL,
        0,
        0,
        sqlStmtType,
        ResultSetIndex,
        RSstmtLabel,
        false);
    if (dpSrvrStmt == NULL)
    {
        exception_->exception_nr = jdbc_SQLSvc_ExecSPJRS_SQLInvalidHandle_exn_;
        exception_->u.SQLInvalidHandle.sqlcode = sqlcode;
        FUNCTION_RETURN_VOID(("createSrvrStmt() -- dummy stmt-- Failed"));
    }

    CLI_DEBUG_SHOW_SERVER_STATEMENT(dpSrvrStmt);

    rc = dpSrvrStmt->ExecSPJRS();

    DEBUG_OUT(DEBUG_LEVEL_STMT,("  jdbc_SQLSvc_ExecSPJRS_sme_ -- rc=%ld, ResultSetIndex=%ld",
        rc, ResultSetIndex));

    pSrvrStmt->resultSetObject = (jobject) resultSet;

    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        exception_->exception_nr = 0;
        if (dpSrvrStmt->outputDescList._length == 0 && dpSrvrStmt->sqlWarning._length == 0)
        {
            dpSrvrStmt->Close(SQL_DROP);
            outputDesc->_length = 0;
            outputDesc->_buffer = NULL;
            sqlWarning->_length = 0;
            sqlWarning->_buffer = NULL;
        }
        else
        {
            outputDesc->_length = dpSrvrStmt->outputDescList._length;
            outputDesc->_buffer = dpSrvrStmt->outputDescList._buffer;
            sqlWarning->_length = dpSrvrStmt->sqlWarning._length;
            sqlWarning->_buffer = dpSrvrStmt->sqlWarning._buffer;
        }
        *RSstmtId = (long)dpSrvrStmt;
        break;
    case SQL_RS_DOES_NOT_EXIST:
        exception_->exception_nr = 0;
        dpSrvrStmt->Close(SQL_DROP);
        outputDesc->_length = 0;
        outputDesc->_buffer = NULL;
        sqlWarning->_length = 0;
        sqlWarning->_buffer = NULL;
        *RSstmtId = (long)dpSrvrStmt;
        break;
    case SQL_STILL_EXECUTING:
        exception_->exception_nr = jdbc_SQLSvc_ExecSPJRS_SQLStillExecuting_exn_;
        break;
    case SQL_ERROR:
        ERROR_DESC_def *error_desc_def;
        error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
        if (pSrvrStmt->sqlError.errorList._length != 0 && (error_desc_def->sqlcode == -8007))
        {
            exception_->exception_nr = jdbc_SQLSvc_ExecSPJRS_SQLQueryCancelled_exn_;
            exception_->u.SQLQueryCancelled.sqlcode = error_desc_def->sqlcode;
        }
        else
        {
            exception_->exception_nr = jdbc_SQLSvc_ExecSPJRS_SQLError_exn_;
            exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
            exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
        }
        break;
    case PROGRAM_ERROR:
        exception_->exception_nr = jdbc_SQLSvc_ExecSPJRS_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECSPJRS_FAILED;
    default:
        exception_->exception_nr = jdbc_SQLSvc_ExecSPJRS_SQLError_exn_ ;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECSPJRS_FAILED;
        break;
    }
    FUNCTION_RETURN_VOID((NULL));
}

/*
* Synchronous method function implementation for
* operation 'odbc_SQLSvc_CancelStatement'
*/
extern "C" void
odbc_SQLSvc_CancelStatement_sme_(
                                 /* In  */ void * objtag_
                                 , /* In    */ const CEE_handle_def *call_id_
                                 , /* Out   */ ExceptionStruct *exception_
                                 , /* In    */ long dialogueId
                                 , /* In    */ long stmtId
                                 , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
                                 )
{
    FUNCTION_ENTRY("odbc_SQLSvc_CancelStatement_sme_",("..."));

    SRVR_STMT_HDL *pSrvrStmt;
    SQLRETURN   rc;
    long        sqlcode;

#ifdef _ODBCMX_TRACE
    if(diagnostic_flags){
        TraceOut(TR_SRVR_KRYPTON_API,"odbc_SQLSvc_CancelStatement_sme_(%#x, %#x, %#x, %ld, %#x, #x)",
            objtag_,
            call_id_,
            exception_,
            dialogueId,
            stmtId,
            sqlWarning);
    }
#endif
    if ((pSrvrStmt = getSrvrStmt(dialogueId, stmtId, &sqlcode)) == NULL)
    {
        exception_->exception_nr = odbc_SQLSvc_CancelStatement_SQLInvalidHandle_exn_;
        exception_->u.SQLInvalidHandle.sqlcode = sqlcode;
        FUNCTION_RETURN_VOID(("getSrvrStmt() Failed"));
    }
    rc = pSrvrStmt->Cancel();
    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        exception_->exception_nr = 0;
        // SqlWarning in pSrvrStmt may pertain the actual method that is being executed
        // Hope that cancel doesn't return any warning
        sqlWarning->_length = 0;
        sqlWarning->_buffer = NULL;
        break;
    case SQL_ERROR:
        exception_->exception_nr = odbc_SQLSvc_CancelStatement_SQLError_exn_;
        exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
        exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
        break;
    case PROGRAM_ERROR:
    default:
        exception_->exception_nr = odbc_SQLSvc_CancelStatement_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_CLOSE_FAILED;
        break;
    }
    FUNCTION_RETURN_VOID((NULL));
}

/*
*
* Synchronous method function for
* operation 'odbc_SQLSvc_SetConnectionOption'
*/
extern "C" void
odbc_SQLSvc_SetConnectionOption_sme_(
                                     /* In  */ void * objtag_
                                     , /* In    */ const CEE_handle_def *call_id_
                                     , /* Out   */ ExceptionStruct *exception_
                                     , /* In    */ long dialogueId
                                     , /* In    */ short connectionOption
                                     , /* In    */ long  optionValueNum
                                     , /* In    */ char *optionValueStr
                                     , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
                                     )
{
    FUNCTION_ENTRY("odbc_SQLSvc_SetConnectionOption_sme_",("... connectionOption=%ld, optionValueNum=%ld, optionValueStr=%s",
        connectionOption,
        optionValueNum,
        optionValueStr));

    char stmtLabel[MAX_STMT_LABEL_LEN+1];
    SQLValue_def sqlStringValue;
    long retcode = 0;
    long    sqlcode;

    SRVR_CONNECT_HDL *pConnect = NULL;
    if(dialogueId == 0) {
        exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_InvalidConnection_exn_;
        FUNCTION_RETURN_VOID(("Connection handle is NULL"));
    }
    pConnect = (SRVR_CONNECT_HDL *)dialogueId;

    SRVR_STMT_HDL *pSrvrStmt;
    char valueStr[129];
    char schemaValueStr[128+128+5+1]; // 5 for quotes + dot
    char sqlString[512];

    sqlStringValue.dataValue._buffer = (unsigned char *)sqlString;
    sqlStringValue.dataCharset = 0;
    sqlStringValue.dataType = SQLTYPECODE_VARCHAR;
    sqlStringValue.dataInd = 0;

    if(diagnostic_flags){
        TraceOut(TR_SRVR_KRYPTON_API,"odbc_SQLSvc_SetConnectionOption_sme_(%#x, %#x, %#x, %ld, %d, %ld, %s, %#x)",
            objtag_,
            call_id_,
            exception_,
            dialogueId,
            connectionOption,
            optionValueNum,
            optionValueStr,
            sqlWarning);
    }

    exception_->exception_nr = CEE_SUCCESS;
    sqlWarning->_length = 0;
    sqlWarning->_buffer = NULL;
    strcpy(stmtLabel, "STMT_INTERNAL_1");
    sqlString[0] = 0;

    switch (connectionOption) {
    case SQL_ACCESS_MODE:
        strcpy(sqlString, "SET TRANSACTION ");
        switch (optionValueNum) {
    case SQL_MODE_READ_WRITE:
        strcat(sqlString, "READ WRITE");
        break;
    case SQL_MODE_READ_ONLY:
        strcat(sqlString, "READ ONLY");
        break;
    default:
        exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_OPTION_VALUE_NUM;
        FUNCTION_RETURN_VOID(("SQL_ACCESS_MODE - optionValueNum Unknown"));
        }
        break;
    case SQL_TXN_ISOLATION:
        strcpy(sqlString, "SET TRANSACTION ISOLATION LEVEL ");
        switch (optionValueNum) {
    case SQL_TXN_READ_UNCOMMITTED:
        strcat(sqlString, "READ UNCOMMITTED");
        break;
    case SQL_TXN_READ_COMMITTED:
        strcat(sqlString, "READ COMMITTED");
        break;
    case SQL_TXN_REPEATABLE_READ:
        strcat(sqlString, "REPEATABLE READ");
        break;
    case SQL_TXN_SERIALIZABLE:
        strcat(sqlString, "SERIALIZABLE");
        break;
    default:
        exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_OPTION_VALUE_NUM;
        FUNCTION_RETURN_VOID(("SQL_TXN_ISOLATION - optionValueNum Unknown"));
        }
        break;
    case SQL_ATTR_ENLIST_IN_DTC:
        FUNCTION_RETURN_VOID(("SQL_ATTR_ENLIST_IN_DTC"));
    case SQL_AUTOCOMMIT:
        strcpy(sqlString, "SET TRANSACTION AUTOCOMMIT ");

        if (optionValueNum)
            strcat(sqlString, "ON");
        else
            strcat(sqlString, "OFF");
        break;
    case SET_CATALOG:
    case SQL_ATTR_CURRENT_CATALOG:
        if (optionValueStr == NULL || (optionValueStr != NULL && optionValueStr[0] == '\0'))
            strcpy(valueStr, "TRAFODION");
        else
        {
            if (strlen(optionValueStr) >= sizeof(valueStr))
            {
                exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
                exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_OPTION_VALUE_STR;
                FUNCTION_RETURN_VOID(("SQL_ATTR_CURRENT_CATALOG - optionValueNum Length Error"));
            }
            strcpy(valueStr, optionValueStr);
        }

        strcpy(pConnect->DefaultCatalog, valueStr);
        strcpy(sqlString, "SET CATALOG '");
        if ((strlen(sqlString)+strlen(valueStr)+1) >= sizeof(sqlString))
        {
            exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
            exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_OPTION_VALUE_STR;
            FUNCTION_RETURN_VOID(("SQL_ATTR_CURRENT_CATALOG - valueStr Length Error"));
        }
        strcat(sqlString, valueStr);
        strcat(sqlString, "'");
        break;
    case SET_SCHEMA:
        if (optionValueStr == NULL || (optionValueStr != NULL && optionValueStr[0] == '\0'))
            strcpy(schemaValueStr, "TRAFODION.SEABASE");
        else
        {
            if (strlen(optionValueStr) >= sizeof(schemaValueStr))
            {
                exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
                exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_OPTION_VALUE_STR;
                FUNCTION_RETURN_VOID(("SET_SCHEMA - optionValueStr Length Error"));
            }
            strcpy(schemaValueStr, optionValueStr);
        }
        strcpy(pConnect->DefaultSchema, schemaValueStr);
        strcpy(sqlString, "SET SCHEMA ");
        if ((strlen(sqlString)+strlen(schemaValueStr)) >= sizeof(sqlString))
        {
            exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
            exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_OPTION_VALUE_STR;
            FUNCTION_RETURN_VOID(("SET_SCHEMA - schemaValueStr Length Error"));
        }
        strcat(sqlString, schemaValueStr);
        break;
    case BEGIN_SESSION:
        if(optionValueStr != NULL && strlen(optionValueStr) > 0)
            sprintf(sqlString,"SET SESSION DEFAULT SQL_SESSION 'BEGIN:%0.200s';",optionValueStr);
        else
            strcpy(sqlString, "SET SESSION DEFAULT SQL_SESSION 'BEGIN';");
        break;
    case END_SESSION:
        strcpy(sqlString, "SET SESSION DEFAULT SQL_SESSION 'END'");
        break;
    case RESET_DEFAULTS:
        strcpy(sqlString, "CONTROL QUERY DEFAULT * RESET");
        break;
    case SET_SETANDCONTROLSTMTS:
        strcpy(sqlString, optionValueStr);
        break;
    case SET_ODBC_PROCESS:
        strcpy(sqlString, "CONTROL QUERY DEFAULT ODBC_PROCESS 'TRUE'");
        srvrGlobal->jdbcProcess = FALSE;
        break;
    case CQD_DOOM_USER_TXN:
        strcpy(sqlString, "CONTROL QUERY DEFAULT DOOM_USERTRANSACTION 'ON'");
        break;
    case SET_JDBC_PROCESS:
        // Note that in handling the odbc_process CQD SQL/MX sets the
        // FLOATTYPE to IEEE.
        srvrGlobal->jdbcProcess = TRUE;
        strcpy(sqlString, "CONTROL QUERY DEFAULT JDBC_PROCESS 'TRUE'");
        break;
    // new code begin
    case CQD_PCODE_OFF:
        strcpy(sqlString, "CONTROL QUERY DEFAULT PCODE_OPT_FLAGS '28'");
        break;
    // new code end
    case SET_MPLOC:
        strcpy(sqlString, "SET MPLOC '");
        strcat(sqlString, optionValueStr);
        strcat(sqlString, "'");
        break;
        //Added for solution 10-120315-2068
    case SET_NAMETYPE:
        strcpy(sqlString, "SET NAMETYPE '");
        strcat(sqlString, optionValueStr);
        strcat(sqlString, "'");
        break;
    case SET_INFER_NCHAR:
#ifdef INFER_CHAR_SET_SUPPORTED
        strcpy(sqlString, "CONTROL QUERY DEFAULT INFER_CHAR_SET ");

        if (optionValueNum)
            strcat(sqlString, "ON");
        else
            strcat(sqlString, "OFF");
        break;
#else
        // the present code will be disabled (until the control statement is supported)
        exception_->exception_nr = CEE_SUCCESS;
        FUNCTION_RETURN_VOID(("SET_INFER_NCHAR"));
#endif
    case SET_OLT_QUERY_OPT:
        strcpy(sqlString, "CONTROL QUERY DEFAULT OLT_QUERY_OPT 'OFF'");
        break;
    case CLEAR_CATALOG:
        strcpy(sqlString, "CONTROL QUERY DEFAULT CATALOG RESET");
        // Set default catalog to null
        pConnect->DefaultCatalog[0] = '\0';
        break;
    case CLEAR_SCHEMA:
        strcpy(sqlString, "CONTROL QUERY DEFAULT SCHEMA RESET");
        // Set default schema to null
        pConnect->DefaultSchema[0] =  '\0';
        break;
        // MFC option to set recompilation warnings on
    case SQL_RECOMPILE_WARNING:
        strcpy(sqlString, "CONTROL QUERY DEFAULT RECOMPILATION_WARNINGS 'ON'");
        break;
        // MFC support for BigNum
    case SET_SESSION_INTERNAL_IO:
        strcpy(sqlString, "SET SESSION DEFAULT internal_format_io 'on'");
        break;
    default:
        exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_CONNECTION_OPTION;
        FUNCTION_RETURN_VOID(("connectionOption Unknown"));
    }

    // Given a label find out the SRVR_STMT_HDL
    pSrvrStmt = createSrvrStmt(dialogueId,
        (const char *)stmtLabel,
        &sqlcode,
        NULL,
        0,
        0,
        TYPE_UNKNOWN,
        false,true);
    if (pSrvrStmt == NULL){
        exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_SQLInvalidHandle_exn_;
        exception_->u.SQLInvalidHandle.sqlcode = sqlcode;
        FUNCTION_RETURN_VOID(("createSrvrStmt() Failed"));
    }

    sqlStringValue.dataValue._length = strlen(sqlString);
    retcode = pSrvrStmt->ExecDirect(NULL, &sqlStringValue, INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
    switch (retcode)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        exception_->exception_nr = 0;
        // Ignore estimatedCost and rowsAffected
        sqlWarning->_length = pSrvrStmt->sqlWarning._length;
        sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
        break;
    case SQL_ERROR:
        exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_SQLError_exn_;
        exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
        exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
        break;
    case PROGRAM_ERROR:
        exception_->exception_nr = odbc_SQLSvc_SetConnectionOption_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_SETCONNECTOPTION_FAILED;
    default:
        break;
    }
    FUNCTION_RETURN_VOID((NULL));
}


/*
* Synchronous method function prototype for
* operation 'odbc_SQLSvc_PrepareFromModule'
*/
extern "C" void
odbc_SQLSvc_PrepareFromModule_sme_(
                                   /* In    */ void * objtag_
                                   , /* In  */ const CEE_handle_def *call_id_
                                   , /* Out   */ ExceptionStruct *exception_
                                   , /* In  */ long dialogueId
                                   , /* In  */ char *moduleName
                                   , /* In  */ long moduleVersion
                                   , /* In  */ long long moduleTimestamp
                                   , /* In  */ char *stmtName
                                   , /* In  */ short sqlStmtType
                                   , /* In  */ long fetchSize
                                   ,/* In   */ long batchSize
                                   , /* In   */ long holdability
                                   , /* Out   */ long *estimatedCost
                                   , /* Out   */ SQLItemDescList_def *inputDesc
                                   , /* Out   */ SQLItemDescList_def *outputDesc
                                   , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
                                   , /* Out   */ long *stmtId
                                   , /* Out   */ long *inputParamOffset
                                   )
{
    FUNCTION_ENTRY("odbc_SQLSvc_PrepareFromModule_sme_",("... fetchSize=%ld, inputParamOffset=%ld",
        fetchSize,
        inputParamOffset));

    SRVR_STMT_HDL *pSrvrStmt;
    SQLRETURN rc;
    ERROR_DESC_def error_desc;
    long    sqlcode;

    odbc_SQLSvc_SQLError ModuleError;
    CLEAR_ERROR(ModuleError);

    // Need to validate the stmtLabel
    // Given a label find out the SRVR_STMT_HDL
    if ((pSrvrStmt = createSrvrStmtForMFC(dialogueId, stmtName, &sqlcode, moduleName,
        moduleVersion, moduleTimestamp, sqlStmtType, TRUE)) == NULL)
    {
        exception_->exception_nr = odbc_SQLSvc_PrepareFromModule_SQLError_exn_;
        kdsCreateSQLErrorException(&ModuleError, 1);
        kdsCopySQLErrorException(&ModuleError, SQLSVC_EXCEPTION_READING_FROM_MODULE_FAILED, sqlcode,
            "HY000");
        exception_->u.SQLError.errorList._length = ModuleError.errorList._length;
        exception_->u.SQLError.errorList._buffer = ModuleError.errorList._buffer;
        FUNCTION_RETURN_VOID(("createSrvrStmt() Failed"));
    }

    // Setup the output descriptors using the fetch size
    pSrvrStmt->holdability = holdability;
    pSrvrStmt->resetFetchSize(fetchSize);

    rc = pSrvrStmt->setMaxBatchSize(batchSize);

    // Prepare the statement
    if(rc == SQL_SUCCESS)
    {
        rc = pSrvrStmt->PrepareFromModule(EXTERNAL_STMT);
    }

    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        exception_->exception_nr = 0;
        // Copy all the output parameters
        *estimatedCost = pSrvrStmt->estimatedCost;
        inputDesc->_length = pSrvrStmt->inputDescList._length;
        inputDesc->_buffer = pSrvrStmt->inputDescList._buffer;
        outputDesc->_length = pSrvrStmt->outputDescList._length;
        outputDesc->_buffer = pSrvrStmt->outputDescList._buffer;
        sqlWarning->_length = pSrvrStmt->sqlWarning._length;
        sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
        *stmtId = (long)pSrvrStmt;
        *inputParamOffset = pSrvrStmt->inputDescParamOffset;
        break;
    case SQL_STILL_EXECUTING:
        exception_->exception_nr = odbc_SQLSvc_PrepareFromModule_SQLStillExecuting_exn_;
        break;
    case ODBC_RG_ERROR:
    case SQL_ERROR:
        ERROR_DESC_def *error_desc_def;
        error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
        if (pSrvrStmt->sqlError.errorList._length != 0 &&
            (error_desc_def->sqlcode == -8007 || error_desc_def->sqlcode == -8007))
        {
            exception_->exception_nr = odbc_SQLSvc_PrepareFromModule_SQLQueryCancelled_exn_;
            exception_->u.SQLQueryCancelled.sqlcode = error_desc_def->sqlcode;
        }
        else
        {
            exception_->exception_nr = odbc_SQLSvc_PrepareFromModule_SQLError_exn_;
            exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
            exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
        }
        break;
    case PROGRAM_ERROR:
        exception_->exception_nr = odbc_SQLSvc_PrepareFromModule_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_PREPARE_FAILED;
    default:
        break;
    }
    FUNCTION_RETURN_VOID((NULL));
}

/*
* Synchronous method function prototype for
* operation 'odbc_SQLSvc_ExecuteCall'
*/
extern "C" void
odbc_SQLSvc_ExecuteCall_sme_(
                             /* In  */ void * objtag_
                             , /* In    */ const CEE_handle_def *call_id_
                             , /* Out   */ ExceptionStruct *exception_
                             , /* In    */ long dialogueId
                             , /* In    */ long stmtId
                             , /* In    */ const SQLValueList_def *inputValueList
                             , /* In    */ short sqlAsyncEnable
                             , /* In    */ long queryTimeout
                             , /* Out   */ SQLValueList_def *outputValueList
                             , /* Out   */ short *returnResult
                             , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
                             )
{
    FUNCTION_ENTRY("odbc_SQLSvc_ExecuteCall_sme_",("..."));

    SRVR_STMT_HDL   *pSrvrStmt;
    SQLRETURN       rc;
    long            sqlcode;

    if ((pSrvrStmt = getSrvrStmt(dialogueId, stmtId, &sqlcode)) == NULL)
    {
        exception_->exception_nr = odbc_SQLSvc_ExecuteCall_SQLInvalidHandle_exn_;
        exception_->u.SQLInvalidHandle.sqlcode = sqlcode;
        FUNCTION_RETURN_VOID(("getSrvrStmt() Failed"));
    }
    rc = pSrvrStmt->ExecuteCall(inputValueList, sqlAsyncEnable, queryTimeout);

    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        exception_->exception_nr = 0;
        // Copy the output values
        *returnResult = 0;
        outputValueList->_length = pSrvrStmt->outputValueList._length;
        outputValueList->_buffer = pSrvrStmt->outputValueList._buffer;
        sqlWarning->_length = pSrvrStmt->sqlWarning._length;
        sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
        break;
    case SQL_STILL_EXECUTING:
        exception_->exception_nr = odbc_SQLSvc_ExecuteCall_SQLStillExecuting_exn_;
        break;
    case SQL_INVALID_HANDLE:
        exception_->exception_nr = odbc_SQLSvc_ExecuteCall_SQLInvalidHandle_exn_;
        break;
    case SQL_NEED_DATA:
        exception_->exception_nr = odbc_SQLSvc_ExecuteCall_SQLNeedData_exn_;
        break;
    case ODBC_SERVER_ERROR:
    case SQL_ERROR:
        ERROR_DESC_def *error_desc_def;
        error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
        if (pSrvrStmt->sqlError.errorList._length != 0 &&
            (error_desc_def->sqlcode == -8007 || error_desc_def->sqlcode == -8007))
        {
            exception_->exception_nr = odbc_SQLSvc_ExecuteCall_SQLQueryCancelled_exn_;
            exception_->u.SQLQueryCancelled.sqlcode = error_desc_def->sqlcode;
        }
        else
        {
            exception_->exception_nr = odbc_SQLSvc_ExecuteCall_SQLError_exn_;
            exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
            exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
        }
        break;
    case -8814:
    case 8814:
        // SQL Error/Warning 8814: The transaction mode at run time (value) differs
        // from that specified at compile time (value). 8814 is translated to an
        // odbc_SQLSvc_ExecuteCall_SQLRetryCompile_exn_ exception.
        exception_->exception_nr = odbc_SQLSvc_ExecuteCall_SQLRetryCompile_exn_;
        break;
    case PROGRAM_ERROR:
        exception_->exception_nr = odbc_SQLSvc_ExecuteCall_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_EXECUTE_FAILED;
    default:
        break;
    }
    FUNCTION_RETURN_VOID((NULL));
}

/*
* Synchronous method function for
* operation 'odbc_SQLSvc_CloseUsingLabel'
*/
extern "C" void
odbc_SQLSvc_CloseUsingLabel_sme_(
                                 /* In  */ void * objtag_
                                 , /* In    */ const CEE_handle_def *call_id_
                                 , /* Out   */ ExceptionStruct *exception_
                                 , /* In    */ long dialogueId
                                 , /* In    */ char *stmtName
                                 , /* In    */ unsigned short freeResourceOpt
                                 , /* Out   */ long *rowsAffected
                                 , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
                                 )
{
    FUNCTION_ENTRY("odbc_SQLSvc_CloseUsingLabel_sme_",("..."));

    SRVR_STMT_HDL *pSrvrStmt;
    SQLRETURN rc;
    long    sqlcode;

#ifdef _ODBCMX_TRACE
    if(diagnostic_flags){
        TraceOut(TR_SRVR_KRYPTON_API,"odbc_SQLSvc_Close_sme_(%#x, %#x, %#x, %ld, %#x, %d, %#x, %#x)",
            objtag_,
            call_id_,
            exception_,
            dialogueId,
            stmtId,
            freeResourceOpt,
            rowsAffected,
            sqlWarning);
    }
#endif
    if (freeResourceOpt != SQL_CLOSE && freeResourceOpt != SQL_DROP &&
        freeResourceOpt != SQL_UNBIND && freeResourceOpt != SQL_RESET_PARAMS)
    {
        exception_->exception_nr = odbc_SQLSvc_Close_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_INVALID_RESOURCE_OPT_CLOSE;
        FUNCTION_RETURN_VOID(("freeResourceOpt is not SQL_CLOSE,SQL_DROP,SQL_UNBIND or SQL_RESET_PARAMS"));
    }
    if ((pSrvrStmt = getSrvrStmt(dialogueId, 0, &sqlcode)) == NULL)
    {
        *rowsAffected = -1;
        exception_->exception_nr = 0;
        sqlWarning->_buffer = NULL;
        sqlWarning->_length = 0;
        FUNCTION_RETURN_VOID(("getSrvrStmt() Failed"));
    }
    rc = pSrvrStmt->Close(freeResourceOpt);
    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        exception_->exception_nr = 0;
        if (freeResourceOpt != SQL_DROP)
        {
            *rowsAffected = pSrvrStmt->rowsAffected;
            sqlWarning->_length = pSrvrStmt->sqlWarning._length;
            sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
        }
        else
        {
            *rowsAffected = 0;
            sqlWarning->_length = 0;
            sqlWarning->_buffer = NULL;
        }
        break;
    case SQL_ERROR:
        exception_->exception_nr = odbc_SQLSvc_Close_SQLError_exn_;
        exception_->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
        exception_->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
        break;
    case PROGRAM_ERROR:
        exception_->exception_nr = odbc_SQLSvc_Close_ParamError_exn_;
        exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_CLOSE_FAILED;
    default:
        break;
    }
    FUNCTION_RETURN_VOID((NULL));
}

/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_Prepare2withRowsets'
 */
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
        )
{
    SRVR_STMT_HDL *pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;
    /* This is not used for now 
     * needed by ODBC rowsets logic
     * if DCS port to use this lib*/
    return;	
}

/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_Prepare2'
 */
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
        , /* In    */ bool isFromExecDirect)
{
    SRVR_STMT_HDL *pSrvrStmt = NULL;
    SQL_QUERY_COST_INFO cost_info;
    SQLRETURN rc = SQL_SUCCESS;
    long sqlcode;
    char b[317];

    bool bSkipWouldLikeToExecute = false; // some queries have to skip Would Like To Execute
    bool flag_21036 = false;

    if (sqlString == NULL)
    {
        *returnCode = SQL_ERROR;
        GETMXCSWARNINGORERROR(-1, "HY090", "Invalid SQL String.", sqlWarningOrErrorLength, sqlWarningOrError);
    }

    // Need to validate the stmtLabel
    // Given a label find out the SRVR_STMT_HDL
    pSrvrStmt = createSrvrStmt(dialogueId, stmtLabel, &sqlcode,	NULL, 0, 0, sqlStmtType, false);
    if(pSrvrStmt == NULL)
    {
        *returnCode = SQL_ERROR;
        GETMXCSWARNINGORERROR(sqlcode, "HY000", "Statement Label could not be allocated.", sqlWarningOrErrorLength, sqlWarningOrError);
    }

    if(*returnCode == 0)
    {
        *stmtHandle = (long)pSrvrStmt;
        // cleanup all memory allocated in the previous operations
        pSrvrStmt->cleanupAll();
        pSrvrStmt->sqlStringLen = strlen(sqlString) + 1;
        MEMORY_ALLOC_ARRAY(pSrvrStmt->sqlStringText, char, pSrvrStmt->sqlStringLen);

        strncpy(pSrvrStmt->sqlStringText, sqlString, pSrvrStmt->sqlStringLen);
        pSrvrStmt->sqlStmtType = (short)sqlStmtType;
        pSrvrStmt->batchMaxRowsetSize = inputRowCnt;
        if (pSrvrStmt->batchMaxRowsetSize == ROWSET_NOT_DEFINED) pSrvrStmt->batchMaxRowsetSize = DEFAULT_ROWSET_SIZE;

        pSrvrStmt->currentMethod = odbc_SQLSvc_PrepareRowset_ldx_;
        pSrvrStmt->holdableCursor = holdableCursor;
        rc = PREPARE_R(pSrvrStmt, isFromExecDirect);
        switch (rc)
        {
            case ODBC_RG_WARNING:
            case SQL_SHAPE_WARNING:
            case SQL_SUCCESS_WITH_INFO:
                *returnCode = SQL_SUCCESS_WITH_INFO;
                *inputDescLength = pSrvrStmt->inputDescBufferLength;
                inputDesc = pSrvrStmt->inputDescBuffer;
                *outputDescLength = pSrvrStmt->outputDescBufferLength;
                outputDesc = pSrvrStmt->outputDescBuffer;
                if (rc == SQL_SUCCESS_WITH_INFO)
                {
                    GETSQLWARNINGORERROR2(pSrvrStmt);
                    *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                    memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
                }
                else if (rc == SQL_SHAPE_WARNING)
                {
                    *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                    memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
                }
                else
                {
                    char *RGWarningOrError = NULL;
                    MEMORY_ALLOC_ARRAY(RGWarningOrError, char, 256);
                    sprintf(b,"lf",pSrvrStmt->cost_info.totalTime);
                    sprintf(RGWarningOrError, "The query's estimated cost: %.50s exceeded resource management attribute limit set.", b);
                    GETMXCSWARNINGORERROR(1, "01000", RGWarningOrError, sqlWarningOrErrorLength, sqlWarningOrError);
                    MEMORY_DELETE_ARRAY(RGWarningOrError);
                }
                break;
            case SQL_SUCCESS:
                *returnCode = SQL_SUCCESS;
                *estimatedCost = (Int32)pSrvrStmt->cost_info.totalTime; // SQL returns cost in a strcuture - cost.totalTime == estimatedCost
                *sqlQueryType = pSrvrStmt->sqlQueryType;
                *inputDescLength = pSrvrStmt->inputDescBufferLength;
                inputDesc = pSrvrStmt->inputDescBuffer;
                *outputDescLength = pSrvrStmt->outputDescBufferLength;
                outputDesc = pSrvrStmt->outputDescBuffer;
                break;
            case SQL_ERROR:
            case ODBC_RG_ERROR:
                *returnCode = SQL_ERROR;
                if (rc == SQL_ERROR)
                {
                    GETSQLWARNINGORERROR2(pSrvrStmt);
                    *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                    memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
                }
                else
                {
                    char *RGWarningOrError = NULL;
                    MEMORY_ALLOC_ARRAY(RGWarningOrError, char, 256);
                    sprintf(b,"lf",pSrvrStmt->cost_info.totalTime);
                    sprintf(RGWarningOrError, "The query's estimated cost: %.50s exceeded resource management attribute limit set.", b);
                    GETMXCSWARNINGORERROR(-1, "HY000", RGWarningOrError, sqlWarningOrErrorLength, sqlWarningOrError);
                    MEMORY_DELETE_ARRAY(RGWarningOrError);
                }
                break;
            case PROGRAM_ERROR:
                *returnCode = SQL_ERROR;
                GETMXCSWARNINGORERROR(-1, "HY000", SQLSVC_EXCEPTION_PREPARE_FAILED, sqlWarningOrErrorLength, sqlWarningOrError);
                break;
            default:
                //			case INFOSTATS_SYNTAX_ERROR:
                //			case INFOSTATS_STMT_NOT_FOUND:
                *returnCode = SQL_ERROR;
                *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
                break;
        }
    }

    if(pSrvrStmt != NULL)
        pSrvrStmt->m_need_21036_end_msg = flag_21036;

    return;
}

//--------------------------------------------------------------------------------
/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_Execute2withRowsets'
 */
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
        , /* Out   */ BYTE *&outValues)
{
    bool bRePrepare2 = false;

    /*
     * The performance team wanted to be able to stub out the actual inserts
     * to measure the contributions of individual components to the overall
     * load times. If the env variable mxosrvr-stubout-EXECUTE2withRowsets
     * is set in ms.env, we will skip over the call to EXECUTE2withRowsets
     * and return sql_success, and rowsAffected = input row count
     */
    /*static*/ bool bCheckStubExecute2WithRowsets = true;
    /*static*/ bool bStubExecute2WithRowsets = false;

    if(bCheckStubExecute2WithRowsets)
    {
        char *env = getenv("mxosrvr-stubout-EXECUTE2withRowsets");
        if (env != NULL && strcmp(env,"true") == 0)
            bStubExecute2WithRowsets = true;
        bCheckStubExecute2WithRowsets = false;
    }



    SRVR_STMT_HDL *pSrvrStmt = NULL;
    SQLRETURN rc = SQL_SUCCESS;
    long sqlcode;

    if ((pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle) == NULL)
    {
        *returnCode = SQL_ERROR;
        GETMXCSWARNINGORERROR(-1, "HY000", "Statement Label not found.", sqlWarningOrErrorLength, sqlWarningOrError);
    }
    else
    {
        *returnCode = SQL_SUCCESS;
        if (inputRowCnt < 0)
        {
        }
        else if (sqlStmtType == TYPE_SELECT && inputRowCnt > 1)
        {
        }
        else if ((pSrvrStmt->maxRowsetSize < inputRowCnt)  || (pSrvrStmt->current_holdableCursor != holdableCursor))
        {
            rePrepare2( pSrvrStmt
                    , sqlStmtType
                    , inputRowCnt
                    , holdableCursor
                    , &rc
                    , returnCode
                    , sqlWarningOrErrorLength
                    , sqlWarningOrError
                    );
            bRePrepare2 = true;
        }

        if (*returnCode == 0 || *returnCode == 1)
        {
            if (inputRowCnt < 0)
            {
                *returnCode = SQL_ERROR;
                GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Row Count.", sqlWarningOrErrorLength, sqlWarningOrError);
            }
            else
            {
                if (sqlStmtType == TYPE_SELECT && inputRowCnt > 1)
                {
                    *returnCode = SQL_ERROR;
                    GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Row Count.", sqlWarningOrErrorLength, sqlWarningOrError);
                }
            }

            if (*returnCode == 0 || *returnCode == 1)
            {
                // Added additional checks to make sure the warnings, if any, are not lost from the
                // rePrepare2() call in SrvrConnect.cpp (returnCode could be 0).
                if ((*returnCode == 0) && (pSrvrStmt->sqlWarningOrErrorLength > 0) && pSrvrStmt->reprepareWarn == FALSE) // To preserve warning returned at prepare time
                {
                    MEMORY_DELETE_ARRAY(pSrvrStmt->sqlWarningOrError);
                    pSrvrStmt->sqlWarningOrErrorLength = 0;
                    pSrvrStmt->sqlWarningOrError = NULL;
                }

                if (pSrvrStmt->bSQLMessageSet)
                    pSrvrStmt->cleanupSQLMessage();
                if(pSrvrStmt->bSQLValueListSet)
                    pSrvrStmt->cleanupSQLValueList();


                if ( (*returnCode == 0 && rc == 0) || (*returnCode == 1 && rc == 1) )
                {
                    pSrvrStmt->inputRowCnt = inputRowCnt;
                    pSrvrStmt->sqlStmtType = (short)sqlStmtType;

                    if (cursorLength > 0)
                    {
                        pSrvrStmt->cursorNameLen = cursorLength;
                        memcpy(pSrvrStmt->cursorName, cursorName, cursorLength);
                        pSrvrStmt->cursorName[cursorLength] = '\0';
                    }
                    else
                        pSrvrStmt->cursorName[0] = '\0';


                    if (pSrvrStmt->sqlQueryType == SQL_RWRS_SPECIAL_INSERT)
                    {
                        *((Int32 *)pSrvrStmt->inputDescVarBuffer) = pSrvrStmt->inputRowCnt;
                        *((Int32 *)(pSrvrStmt->inputDescVarBuffer+4)) = pSrvrStmt->maxRowLen;
                        *((BYTE **)(pSrvrStmt->inputDescVarBuffer+8)) = inValues  ;

                    }

                    if (pSrvrStmt->preparedWithRowsets == TRUE)
                    {
                        pSrvrStmt->transportBuffer = inValues;
                        pSrvrStmt->transportBufferLen = inValuesLength;
                    }
                    else if (pSrvrStmt->inputDescVarBufferLen == inValuesLength)
                        memcpy(pSrvrStmt->inputDescVarBuffer, inValues, inValuesLength);
                    else
                    {
                        *returnCode = SQL_ERROR;
                        GETMXCSWARNINGORERROR( -1
                                , "HY090"
                                , "Invalid param Values."
                                , sqlWarningOrErrorLength
                                , sqlWarningOrError
                                );
                        goto out;
                    }

                    if (bRePrepare2)
                    {
                        // Note: The below method ends in a dummy call in CommonNSKFunctions.cpp. CR 5763 takes care of this.
                        //rc = rePrepare2WouldLikeToExecute((Long)pSrvrStmt, (Int32*)returnCode, (Int32*)sqlWarningOrErrorLength, (char*&)sqlWarningOrError);
                        rc = true;
                        if (rc == false)
                        {
                            *rowsAffected = -1;
                            goto out;
                        }
                    }

                    pSrvrStmt->currentMethod = odbc_SQLSvc_ExecuteN_ldx_;
                    if(!bStubExecute2WithRowsets) {
                        rc = EXECUTE2withRowsets(pSrvrStmt);
                    }
                    else {
                        rc = SQL_SUCCESS;
                        pSrvrStmt->rowsAffected = inputRowCnt;
                    }

                    switch (rc)
                    {
                        case ROWSET_SQL_ERROR:
                            // Copy the output values
                            *rowsAffected = -1;

                            if (pSrvrStmt->sqlWarningOrErrorLength > 0) // Overwriting warning returned at prepare time
                            {
                                MEMORY_DELETE_ARRAY(pSrvrStmt->sqlWarningOrError);
                                pSrvrStmt->sqlWarningOrErrorLength = 0;
                                pSrvrStmt->sqlWarningOrError = NULL;
                            }

                            GETSQLWARNINGORERROR2forRowsets(pSrvrStmt);
                            *returnCode = SQL_ERROR;
                            *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                            memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
                            break;
                        case SQL_SUCCESS_WITH_INFO:
                            *returnCode = SQL_SUCCESS_WITH_INFO;
                            *rowsAffected = pSrvrStmt->rowsAffected;
                            if (   pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE
                                    || pSrvrStmt->sqlStmtType  == TYPE_CALL)
                            {
                                *outValuesLength = pSrvrStmt->outputDescVarBufferLen;
                                outValues = pSrvrStmt->outputDescVarBuffer;
                            }
                            else
                            {
                                *outValuesLength = 0;
                                outValues = 0;
                            }
                            if (pSrvrStmt->sqlWarningOrErrorLength > 0) // Overwriting warning returned at prepare time
                            {
                                MEMORY_DELETE_ARRAY(pSrvrStmt->sqlWarningOrError);
                                pSrvrStmt->sqlWarningOrErrorLength = 0;
                                pSrvrStmt->sqlWarningOrError = NULL;
                            }

                            if (pSrvrStmt->sqlWarning._length > 0)
                                GETSQLWARNINGORERROR2forRowsets(pSrvrStmt);
                            else
                                GETSQLWARNINGORERROR2(pSrvrStmt);
                            *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                            memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
                            break;
                        case SQL_SUCCESS:
                            *returnCode = SQL_SUCCESS;
                            *rowsAffected = pSrvrStmt->rowsAffected;
                            if (pSrvrStmt->sqlWarning._length > 0)
                            {
                                if (pSrvrStmt->sqlWarningOrErrorLength > 0) // Overwriting warning returned at prepare time
                                {
                                    MEMORY_DELETE_ARRAY(pSrvrStmt->sqlWarningOrError);
                                    pSrvrStmt->sqlWarningOrErrorLength = 0;
                                    pSrvrStmt->sqlWarningOrError = NULL;
                                }

                                GETSQLWARNINGORERROR2forRowsets(pSrvrStmt);
                                *returnCode = SQL_SUCCESS_WITH_INFO;  // We have warnings so return success witn info.
                                *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                                memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
                            }

                            if (pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE
                                    || pSrvrStmt->sqlStmtType  == TYPE_CALL)
                            {
                                *outValuesLength = pSrvrStmt->outputDescVarBufferLen;
                                outValues = pSrvrStmt->outputDescVarBuffer;
                            }
                            else
                            {
                                *outValuesLength = 0;
                                outValues = 0;
                            }
                            break;
                        case SQL_NO_DATA_FOUND:
                            *returnCode = SQL_NO_DATA_FOUND;
                            break;
                        case SQL_INVALID_HANDLE:
                            *returnCode = SQL_ERROR;
                            GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Statement Handle.", sqlWarningOrErrorLength, sqlWarningOrError);
                            break;
                        case SQL_ERROR:
                            if (pSrvrStmt->sqlWarningOrErrorLength > 0) // Overwriting warning returned at prepare time
                            {
                                MEMORY_DELETE_ARRAY(pSrvrStmt->sqlWarningOrError);
                                pSrvrStmt->sqlWarningOrErrorLength = 0;
                                pSrvrStmt->sqlWarningOrError = NULL;
                            }

                            GETSQLWARNINGORERROR2(pSrvrStmt);
                            *returnCode = SQL_ERROR;
                            *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                            memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
                            break;
                        default:
                            break;
                    }
                }
out:
                return;
            }
        }  // end if (*returnCode == 0 && rc == 0)
    }  // end else

outout:
    pSrvrStmt->returnCodeForDelayedError = *returnCode;
    return;
}  // end odbc_SQLSvc_Execute2withRowsets_sme_

//------------------------------------------------------------------------------
extern "C" void
rePrepare2( SRVR_STMT_HDL *pSrvrStmt
        , Int32			sqlStmtType
        , Int32			inputRowCnt
        , Int32		holdableCursor
        , SQLRETURN     *rc
        , Int32          *returnCode
        , Int32      *sqlWarningOrErrorLength
        , BYTE          *&sqlWarningOrError
        )
{
    UInt32	tmpSqlStringLen  = pSrvrStmt->sqlStringLen;
    char	*tmpSqlString;
    short	tmpStmtType      = pSrvrStmt->stmtType;
    short	tmpSqlStmtType   = sqlStmtType; // need to do this since PREPARE does not pass this from driver
    Int32	tmpMaxRowsetSize = pSrvrStmt->maxRowsetSize;
    Int32	sqlQueryType;
    Int32	estimatedCost;
    char b[317];

    if (pSrvrStmt->sqlWarningOrErrorLength > 0) // To preserve warning returned at prepare time
    {
        MEMORY_DELETE_ARRAY(pSrvrStmt->sqlWarningOrError);
        pSrvrStmt->sqlWarningOrErrorLength = 0;
        pSrvrStmt->sqlWarningOrError = NULL;
    }

    if (pSrvrStmt->bSQLMessageSet)
        pSrvrStmt->cleanupSQLMessage();
    if(pSrvrStmt->bSQLValueListSet)
        pSrvrStmt->cleanupSQLValueList();

    MEMORY_ALLOC_ARRAY(tmpSqlString, char, tmpSqlStringLen+1);
    strcpy(tmpSqlString, pSrvrStmt->sqlStringText);

    // cleanup all memory allocated in the previous operations
    pSrvrStmt->cleanupAll();
    pSrvrStmt->sqlStringLen = tmpSqlStringLen;

    MEMORY_ALLOC_ARRAY(pSrvrStmt->sqlStringText, char, pSrvrStmt->sqlStringLen+1);
    strcpy(pSrvrStmt->sqlStringText, tmpSqlString);
    MEMORY_DELETE_ARRAY(tmpSqlString);

    pSrvrStmt->stmtType      = tmpStmtType;
    pSrvrStmt->sqlStmtType   = tmpSqlStmtType;
    pSrvrStmt->maxRowsetSize = inputRowCnt;
    pSrvrStmt->holdableCursor= holdableCursor;

    if (pSrvrStmt->maxRowsetSize == ROWSET_NOT_DEFINED)
        pSrvrStmt->maxRowsetSize = DEFAULT_ROWSET_SIZE;

    *rc = REALLOCSQLMXHDLS(pSrvrStmt); // This is a workaround for executor when we switch between OLTP vs NON-OLTP
    if (*rc < 0)
    {
        GETSQLWARNINGORERROR2(pSrvrStmt);
        *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
        memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
        goto out;
    }

    if (pSrvrStmt->sqlStmtType == TYPE_INSERT_PARAM)
    {
        *rc = WSQL_EXEC_SetStmtAttr(&pSrvrStmt->stmt, SQL_ATTR_ROWSET_ATOMICITY, SQL_ATOMIC, 0);
        if (*rc < 0)
        {
            GETSQLWARNINGORERROR2(pSrvrStmt);
            goto out;
        }
        WSQL_EXEC_ClearDiagnostics(&pSrvrStmt->stmt);
    }

    pSrvrStmt->currentMethod = odbc_SQLSvc_PrepareRowset_ldx_;
    if(pSrvrStmt->maxRowsetSize > 1)
        *rc = PREPARE2withRowsets(pSrvrStmt);
    else
        *rc = PREPARE_R(pSrvrStmt);


    switch (*rc)
    {
        case ODBC_RG_WARNING:
        case SQL_SUCCESS_WITH_INFO:
            *returnCode = SQL_SUCCESS_WITH_INFO;
            estimatedCost = (Int32)pSrvrStmt->cost_info.totalTime; // change to double in future
            sqlQueryType = pSrvrStmt->sqlQueryType;

            if (*rc == SQL_SUCCESS_WITH_INFO)
            {
                GETSQLWARNINGORERROR2(pSrvrStmt);
                *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
            }
            else
            {
                char RGWarningOrError[256];

                sprintf(b,"lf",pSrvrStmt->cost_info.totalTime);
                sprintf( RGWarningOrError
                        , "The query's estimated cost: %.50s exceeded resource management attribute limit set."
                        , b
                       );
                GETMXCSWARNINGORERROR(1, "01000", RGWarningOrError, sqlWarningOrErrorLength, sqlWarningOrError);
            }
            break;
        case SQL_SUCCESS:
            WSQL_EXEC_ClearDiagnostics(&pSrvrStmt->stmt);
            estimatedCost = (Int32)pSrvrStmt->cost_info.totalTime; // change to double in future
            sqlQueryType = pSrvrStmt->sqlQueryType;
            break;
        case SQL_ERROR:
        case ODBC_RG_ERROR:
            *returnCode = SQL_ERROR;
            if (*rc == SQL_ERROR)
            {
                GETSQLWARNINGORERROR2(pSrvrStmt);
                *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
            }
            else
            {
                char *RGWarningOrError = NULL;

                MEMORY_ALLOC_ARRAY(RGWarningOrError, char, 256);
                sprintf(b,"lf",pSrvrStmt->cost_info.totalTime);
                sprintf( RGWarningOrError
                        , "The query's estimated cost: %.50s exceeded resource management attribute limit set."
                        , b
                       );
                GETMXCSWARNINGORERROR( -1
                        , "HY000"
                        , RGWarningOrError
                        , sqlWarningOrErrorLength
                        , sqlWarningOrError
                        );
                MEMORY_DELETE_ARRAY(RGWarningOrError);
            }
            break;
        case PROGRAM_ERROR:
            GETMXCSWARNINGORERROR( -1
                    , "HY000"
                    , SQLSVC_EXCEPTION_PREPARE_FAILED
                    , sqlWarningOrErrorLength
                    , sqlWarningOrError
                    );
            break;
        default:
            break;
    }  // end switch
out:
    return;
}  // end rePrepare2

/*
 * Synchronous method function for
 * operation 'odbc_SQLSvc_Execute2'
 */
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
        , /* Out   */ BYTE *&outValues)
{
    SRVRTRACE_ENTER(FILE_SME+19);

    bool bRePrepare2 = false;
    SRVR_STMT_HDL *pSrvrStmt = NULL;
    SQLRETURN rc = SQL_SUCCESS;

    if ((pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle) == NULL)
    {
        *returnCode = SQL_ERROR;
        GETMXCSWARNINGORERROR(-1, "HY000", "Statement Label not found.", sqlWarningOrErrorLength, sqlWarningOrError);
    }
    else
    {
        if (pSrvrStmt->current_holdableCursor != holdableCursor)
        {
            rePrepare2( pSrvrStmt
                    , sqlStmtType
                    , inputRowCnt
                    , holdableCursor
                    , &rc
                    , returnCode
                    , sqlWarningOrErrorLength
                    , sqlWarningOrError
                    );
            bRePrepare2 = true;
        }

        if (*returnCode == 0 || *returnCode == 1)
        {
            if (inputRowCnt < 0)
            {
                *returnCode = SQL_ERROR;
                GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Row Count.", sqlWarningOrErrorLength, sqlWarningOrError);
            }
            else
            {
                if (sqlStmtType == TYPE_SELECT && inputRowCnt > 1)
                {
                    *returnCode = SQL_ERROR;
                    GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Row Count.", sqlWarningOrErrorLength, sqlWarningOrError);
                }
            }

            if (*returnCode == 0 || *returnCode == 1)
            {
                if ((*returnCode == 0) && (pSrvrStmt->sqlWarningOrErrorLength > 0)) // To preserve warning returned at prepare time
                {
                    MEMORY_DELETE_ARRAY(pSrvrStmt->sqlWarningOrError);
                    pSrvrStmt->sqlWarningOrErrorLength = 0;
                    pSrvrStmt->sqlWarningOrError = NULL;
                }
                pSrvrStmt->inputRowCnt = inputRowCnt;
                pSrvrStmt->sqlStmtType = (short)sqlStmtType;

                if (cursorLength > 0)
                {
                    pSrvrStmt->cursorNameLen = cursorLength;
                    memcpy(pSrvrStmt->cursorName, cursorName, cursorLength);
                    pSrvrStmt->cursorName[cursorLength] = '\0';
                }
                else
                    pSrvrStmt->cursorName[0] = '\0';

                if (pSrvrStmt->sqlQueryType == SQL_RWRS_SPECIAL_INSERT)
                {
                    *((Int32 *)pSrvrStmt->inputDescVarBuffer) = pSrvrStmt->inputRowCnt;
                    *((Int32 *)(pSrvrStmt->inputDescVarBuffer+4)) = pSrvrStmt->maxRowLen;
                    *((BYTE **)(pSrvrStmt->inputDescVarBuffer+8)) = inValues  ;

                }
                else
                {

                    if (pSrvrStmt->inputDescVarBufferLen == inValuesLength)
                        memcpy(pSrvrStmt->inputDescVarBuffer, inValues, inValuesLength);
                    else
                    {
                        *returnCode = SQL_ERROR;
                        GETMXCSWARNINGORERROR(-1, "HY090", "Invalid param Values.", sqlWarningOrErrorLength, sqlWarningOrError);
                    }
                }

                if (bRePrepare2)
                {
                    rc = true;
                    if (rc == false)
                    {
                        *rowsAffected = -1;
                        return;
                    }
                }

                pSrvrStmt->currentMethod = odbc_SQLSvc_ExecuteN_ldx_;
                rc = EXECUTE2(pSrvrStmt);

                switch (rc)
                {
                    case SQL_SUCCESS_WITH_INFO:
                        *returnCode = SQL_SUCCESS_WITH_INFO;
                        *rowsAffected = pSrvrStmt->rowsAffected;
                        if (   pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE
                                || pSrvrStmt->sqlStmtType  == TYPE_CALL)
                        {
                            *outValuesLength = pSrvrStmt->outputDescVarBufferLen;
                            outValues = pSrvrStmt->outputDescVarBuffer;
                        }
                        else
                        {
                            if (pSrvrStmt->sqlQueryType == SQL_RWRS_SPECIAL_INSERT)
                            {
                                if (inValues != NULL)
                                    *pSrvrStmt->inputDescVarBuffer = NULL;
                            }
                            *outValuesLength = 0;
                            outValues = 0;
                        }

                        if (pSrvrStmt->sqlWarningOrErrorLength > 0) // overwriting warning returned at prepare time
                        {
                            MEMORY_DELETE_ARRAY(pSrvrStmt->sqlWarningOrError);
                            pSrvrStmt->sqlWarningOrErrorLength = 0;
                            pSrvrStmt->sqlWarningOrError = NULL;
                        }

                        GETSQLWARNINGORERROR2(pSrvrStmt);
                        *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                        memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
                        break;
                    case SQL_SUCCESS:
                        *returnCode = SQL_SUCCESS;
                        *rowsAffected = pSrvrStmt->rowsAffected;
                        if (   pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE
                                || pSrvrStmt->sqlStmtType  == TYPE_CALL)
                        {
                            *outValuesLength = pSrvrStmt->outputDescVarBufferLen;
                            outValues = pSrvrStmt->outputDescVarBuffer;
                        }
                        else
                        {
                            if (pSrvrStmt->sqlQueryType == SQL_RWRS_SPECIAL_INSERT)
                            {
                                if (inValues != NULL)
                                    *pSrvrStmt->inputDescVarBuffer = NULL;
                            }
                            *outValuesLength = 0;
                            outValues = 0;
                        }
                        break;
                    case SQL_NO_DATA_FOUND:
                        *returnCode = SQL_NO_DATA_FOUND;
                        break;
                    case SQL_INVALID_HANDLE:
                        *returnCode = SQL_ERROR;
                        GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Statement Handle.", sqlWarningOrErrorLength, sqlWarningOrError);
                        break;
                    case SQL_ERROR:
                        if (pSrvrStmt->sqlWarningOrErrorLength > 0) // Overwriting warning returned at prepare time
                        {
                            MEMORY_DELETE_ARRAY(pSrvrStmt->sqlWarningOrError);
                            pSrvrStmt->sqlWarningOrErrorLength = 0;
                            pSrvrStmt->sqlWarningOrError = NULL;
                        }

                        GETSQLWARNINGORERROR2(pSrvrStmt);
                        *returnCode = SQL_ERROR;
                        *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                        memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
                        break;
                    default:
                        break;
                }
            }
        }
    }
    return;
}

extern "C" void
odbc_SQLSrvr_FetchPerf_sme_(
        /* In    */ Long dialogueId
        , /* Out   */ IDL_long *returnCode
        , /* In    */ Long  stmtHandle
        , /* In    */ Int32 maxRowCnt
        , /* In    */ Int32 maxRowLen
        , /* In    */ IDL_short sqlAsyncEnable
        , /* In    */ Int32 queryTimeout
        , /* Out   */ Int32 *rowsAffected
        , /* Out   */ Int32 *outValuesFormat
        , /* Out   */ SQL_DataValue_def *outputDataValue
        , /* Out   */ Int32 *sqlWarningOrErrorLength
        , /* Out   */ BYTE     *&sqlWarningOrError
        )
{
    SRVRTRACE_ENTER(FILE_SME+8);

    SRVR_STMT_HDL *pSrvrStmt = NULL;
    SQLRETURN rc = SQL_SUCCESS;
    int outputDataOffset = 0;

    *returnCode = SQL_SUCCESS;

    if (maxRowCnt < 0)
    {
        *returnCode = SQL_ERROR;
        GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Row Count", sqlWarningOrErrorLength, sqlWarningOrError);
    }
    else
    {
        pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;

        if (pSrvrStmt == NULL)
        {
            *returnCode = SQL_ERROR;
            GETMXCSWARNINGORERROR(-1, "HY000", "Statement Label not found", sqlWarningOrErrorLength, sqlWarningOrError);
        }
        else
        {
            if (pSrvrStmt->sqlWarningOrErrorLength > 0 &&
                    pSrvrStmt->sqlWarningOrError != NULL)
            {
                MEMORY_DELETE_ARRAY(pSrvrStmt->sqlWarningOrError);
            }
            pSrvrStmt->sqlWarningOrErrorLength = 0;
            pSrvrStmt->sqlWarningOrError = NULL;
        }

    }

    if (*returnCode == SQL_SUCCESS)
    {
        pSrvrStmt->maxRowCnt = maxRowCnt;
        pSrvrStmt->maxRowLen = maxRowLen;

        if (pSrvrStmt->sqlStmtType != TYPE_SELECT_CATALOG)
        {
            if (pSrvrStmt->bSQLMessageSet)
                pSrvrStmt->cleanupSQLMessage();

            if(pSrvrStmt->outputDataValue._length > 0 &&
                    pSrvrStmt->outputDataValue._buffer != NULL)
                MEMORY_DELETE_ARRAY(pSrvrStmt->outputDataValue._buffer);

            pSrvrStmt->outputDataValue._length = 0;
            pSrvrStmt->outputDataValue._buffer = NULL;

            if (pSrvrStmt->isClosed)
            {
                pSrvrStmt->m_curRowsFetched = 0;
                pSrvrStmt->bFirstSqlBulkFetch = false;
                *returnCode = SQL_NO_DATA_FOUND;
                goto ret;
            }

            pSrvrStmt->currentMethod = odbc_SQLSvc_FetchPerf_ldx_;
            // This will be supported in the near future
            if (srvrGlobal->drvrVersion.buildId & ROWWISE_ROWSET)
            {
                *outValuesFormat = ROWWISE_ROWSETS;

                rc = FETCH2bulk(pSrvrStmt);
                if (pSrvrStmt->rowsAffected > 0)
                {
                    if(pSrvrStmt->outputDataValue._length == 0 && pSrvrStmt->outputDataValue._buffer == NULL)
                    {
                        outputDataValue->_buffer = pSrvrStmt->outputDescVarBuffer;
                        outputDataValue->_length = pSrvrStmt->outputDescVarBufferLen*pSrvrStmt->rowsAffected;
                    }
                    else
                    {
                        outputDataValue->_buffer = pSrvrStmt->outputDataValue._buffer;
                        outputDataValue->_length = pSrvrStmt->outputDataValue._length;
                    }
                }
                else
                {
                    outputDataValue->_buffer = NULL;
                    outputDataValue->_length = 0;
                }
            }
            else
            {
                *outValuesFormat = COLUMNWISE_ROWSETS;

                rc = FETCHPERF(pSrvrStmt, outputDataValue);
            }

            switch (rc)
            {
                case ODBC_RG_WARNING:
                case SQL_SUCCESS_WITH_INFO:
                    *returnCode = SQL_SUCCESS_WITH_INFO;
                    GETSQLWARNINGORERROR2(pSrvrStmt);
                    *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                    memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
                    *rowsAffected = pSrvrStmt->rowsAffected;
                    if (*rowsAffected > 0)
                        pSrvrStmt->m_curRowsFetched += *rowsAffected;
                    break;

                case SQL_SUCCESS:
                    *returnCode = SQL_SUCCESS;
                    *rowsAffected = pSrvrStmt->rowsAffected;

                    if (*rowsAffected > 0)
                        pSrvrStmt->m_curRowsFetched += *rowsAffected;
                    break;

                case SQL_STILL_EXECUTING:
                    *returnCode = SQL_STILL_EXECUTING;
                    break;

                case SQL_INVALID_HANDLE:
                    *returnCode = SQL_INVALID_HANDLE;
                    break;

                case SQL_NO_DATA_FOUND:
                    pSrvrStmt->bFirstSqlBulkFetch = false;
                    *returnCode = SQL_NO_DATA_FOUND;
                    break;

                case SQL_ERROR:
                    pSrvrStmt->bFirstSqlBulkFetch = false;
                    *returnCode = SQL_ERROR;
                    GETSQLWARNINGORERROR2(pSrvrStmt);
                    *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                    memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
                    break;

                case PROGRAM_ERROR:
                    pSrvrStmt->bFirstSqlBulkFetch = false;
                    *returnCode = SQL_ERROR;
                    GETMXCSWARNINGORERROR(-1, "HY000", "Fetch Failed", sqlWarningOrErrorLength, sqlWarningOrError);
                    break;

                default:
                    break;
            }
        }
        else
        { // Catalog APIs
            outputDataOffset  = *(int*)pSrvrStmt->outputDataValue.pad_to_offset_8_;

            *outValuesFormat = COLUMNWISE_ROWSETS;
            rc = FETCHPERF(pSrvrStmt, &pSrvrStmt->outputDataValue);
            if (pSrvrStmt->sqlError.errorList._buffer != NULL)
            {
                *returnCode = SQL_ERROR;
                GETSQLWARNINGORERROR2(pSrvrStmt);
                *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
                MEMORY_DELETE_ARRAY(pSrvrStmt->outputDataValue._buffer);
                pSrvrStmt->outputDataValue._buffer = NULL;
                pSrvrStmt->outputDataValue._length = 0;

            }
            else if (pSrvrStmt->rowsAffected == 0 || pSrvrStmt->rowsAffected == -1)
            {
                if (pSrvrStmt->bSQLMessageSet)
                    pSrvrStmt->cleanupSQLMessage();
                pSrvrStmt->outputDataValue._buffer = NULL;
                pSrvrStmt->outputDataValue._length = 0;
                *(int*)pSrvrStmt->outputDataValue.pad_to_offset_8_=0;
                outputDataOffset = 0;
                pSrvrStmt->InternalStmtClose(SQL_CLOSE);
                *returnCode = SQL_NO_DATA_FOUND;
            }
            else
            {
                *rowsAffected = pSrvrStmt->rowsAffected;

                if (pSrvrStmt->sqlWarning._length != 0)
                {
                    *returnCode = SQL_SUCCESS_WITH_INFO;
                    GETSQLWARNINGORERROR2(pSrvrStmt);
                    *sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                    memcpy(sqlWarningOrError, pSrvrStmt->sqlWarningOrError, *sqlWarningOrErrorLength);
                }
                else
                {
                    char *tmpByte = (char*)&pSrvrStmt->outputDataValue._length;
                    for(int i=0; i<sizeof(pSrvrStmt->outputDataValue.pad_to_offset_8_); i++) {
                        pSrvrStmt->outputDataValue.pad_to_offset_8_[i] = *tmpByte;
                        tmpByte++;
                    }

                    *returnCode = SQL_SUCCESS;
                }

                pSrvrStmt->rowsAffected = 0;
            }

            outputDataValue->_length = pSrvrStmt->outputDataValue._length - outputDataOffset;
            outputDataValue->_buffer = pSrvrStmt->outputDataValue._buffer + outputDataOffset;
        }

ret:

        if (*returnCode != SQL_SUCCESS &&
                *returnCode != SQL_SUCCESS_WITH_INFO)
        {
            MEMORY_DELETE_ARRAY(pSrvrStmt->outputDataValue._buffer);
            pSrvrStmt->outputDataValue._length = 0;
            pSrvrStmt->outputDataValue._buffer = NULL;
        }

        if (pSrvrStmt->sqlNewQueryType == SQL_SP_RESULT_SET)
        {
            if (pSrvrStmt->callStmtHandle->isClosed == true && *returnCode == SQL_NO_DATA_FOUND || *returnCode == SQL_ERROR)
            {
                pSrvrStmt->callStmtHandle->inState = STMTSTAT_CLOSE;
                // Do nothing for now
            }
        }
    }

    SRVRTRACE_EXIT(FILE_SME+8);

    return;

} // odbc_SQLSrvr_FetchPerf_sme_()
