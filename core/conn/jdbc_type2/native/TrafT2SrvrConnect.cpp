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

// MODULE: SrvrConnect.cpp
//
// PURPOSE: Implements the following methods
//
//      odbc_SQLSrvr_Prepare_ame_
//      odbc_SQLSrvr_Fetch_ame_
//      odbc_SQLSrvr_Close_ame_
//      odbc_SQLSrvr_ExecDirect_ame_
//      odbc_SQLSrvr_Execute2_ame_
//
// HISTORY:
// FEB/10/2015  Implement new Prepare, Execute method
//              for Rowsets.
//

#include <platform_ndcs.h>
#include <stdio.h>
#include <sql.h>
#include <sqlext.h>
#include <dlfcn.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "TrafT2SrvrConnect.h"
#include "SrvrCommon.h"
#include "CoreCommon.h"
#include "SrvrFunctions.h"
#include "SrvrKds.h"
#include "SqlInterface.h"
#include "CommonDiags.h"
#include "Debug.h"
#include "SqlInterface.h"

#include <queue>
#include <fstream>
using namespace std;


extern long maxHeapPctExit;
extern long initSessMemSize ;
int fd = -1;
bool heapSizeExit = false;

short DO_WouldLikeToExecute(
        Long dialogueId
        , Long stmtHandle
        , IDL_long* returnCode
        , IDL_long* sqlWarningOrErrorLength
        , BYTE*& sqlWarningOrError
        );

short qrysrvc_ExecuteFinished(Long dialogueId
        , const Long stmtHandle
        , const bool bCheckSqlQueryType
        , const short error_code
        , const bool bFetch
        , const bool bException = false
        , const bool bErase = true
        );

SRVR_STMT_HDL * pQueryStmt = NULL;

bool InsertControls(char* sqlString, odbc_SQLSvc_ExecDirect_exc_ *exception_);
bool LoadControls(char* sqlString, bool genOrexc, char* genRequestError, odbc_SQLSvc_PrepareRowset_exc_ *exception_, SRVR_STMT_HDL **stmtHandle); //3155
bool ResetControls(char* genRequestError);
bool GetHashInfo(char* sqlString, char* genRequestError, char *HashTableInfo);
bool getSQLInfo(E_GetSQLInfoType option, long stmtHandle=NULL, char *stmtLabel=NULL );

pthread_mutex_t Thread_mutex;
/*
 *  New method for Prepare implements rowsets
 * */
extern "C" void
odbc_SQLSrvr_Prepare_ame_(
          /* In    */ Long dialogueId
        , /* Out   */ Long &stmtHandle
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
)
{
    IDL_long	returnCode = SQL_SUCCESS;
    IDL_long	sqlWarningOrErrorLength = 0;
    BYTE		*sqlWarningOrError = NULL;
    IDL_long	sqlQueryType = 0;
    IDL_long	estimatedCost = 0;
    IDL_long	inputParamsLength = 0;
    IDL_long	inputDescLength = 0;
    BYTE		*inputDesc = NULL;
    IDL_long	outputColumnsLength = 0;
    IDL_long	outputDescLength = 0;
    BYTE		*outputDesc = NULL;
    SRVR_STMT_HDL *pSrvrStmt = NULL;
    CEE_status		rc = 0;
    char errorBuffer[512];            // a buffer for formatting error messages

    IDL_boolean bPrepareWithRowsets = IDL_FALSE;

    if (maxRowsetSize > 1)
    {
        bPrepareWithRowsets = IDL_TRUE;
    }

    MEMORY_ALLOC_ARRAY(sqlWarningOrError, BYTE, 1024);
    if(sqlWarningOrError == NULL)
        exit(1);

    memset(sqlWarningOrError, 0, sizeof(sqlWarningOrError));

    if(bPrepareWithRowsets)
    {
        odbc_SQLSvc_Prepare2withRowsets_sme_(dialogueId, sqlAsyncEnable, queryTimeout,
                maxRowsetSize, sqlStmtType, stmtLength, stmtLabel, stmtLabelCharset, cursorLength, cursorName,
                cursorCharset, moduleNameLength, moduleName, moduleCharset, moduleTimestamp,
                sqlStringLength, sqlString, sqlStringCharset, setStmtOptionsLength, setStmtOptions, holdableCursor,
                &returnCode, &sqlWarningOrErrorLength, sqlWarningOrError, &sqlQueryType,
                &stmtHandle, &estimatedCost, &inputDescLength, inputDesc,
                &outputDescLength, outputDesc);
    }
    else
    {
        odbc_SQLSvc_Prepare2_sme_(dialogueId,
                maxRowsetSize,
                sqlStmtType,
                stmtLabel,
                sqlString,
                holdableCursor,
                &returnCode,
                &sqlWarningOrErrorLength,
                sqlWarningOrError,
                &sqlQueryType,
                &stmtHandle,
                &estimatedCost,
                &inputDescLength,
                inputDesc,
                &outputDescLength,
                outputDesc);
    }

    pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;

    odbc_SQLSrvr_Prepare_param_res_(
            pSrvrStmt
            , returnCode
            , sqlWarningOrErrorLength
            , sqlWarningOrError
            , sqlQueryType
            , pSrvrStmt->myKey
            , estimatedCost
            , inputDescLength
            , inputDesc
            , outputDescLength
            , outputDesc
            );

    MEMORY_DELETE_ARRAY(sqlWarningOrError);
    sqlWarningOrErrorLength = 0;

    return;
} /* odbc_SQLSrvr_Prepare_ame_() */

extern "C" void
odbc_SQLSrvr_Fetch_ame_(
        /* In    */       Long  dialogueId
        , /* In    */       IDL_long         sqlAsyncEnable
        , /* In    */       IDL_long         queryTimeout
        , /* In    */       Long             stmtHandle
        , /* In    */ const IDL_string       stmtLabel
        , /* In    */       IDL_unsigned_long_long maxRowCnt
        , /* In    */       IDL_unsigned_long_long maxRowLen
        )
{
    IDL_long  returnCode              = SQL_SUCCESS;
    IDL_long  rowsAffected            = 0;
    IDL_long  outValuesFormat         = UNKNOWN_DATA_FORMAT;
    IDL_long  outValuesLength         = 0;
    BYTE	   *outValues               = NULL;
    IDL_long  sqlWarningOrErrorLength = 0;
    BYTE	   *sqlWarningOrError       = NULL;
    RETCODE   rc                      = 0;
    SQL_DataValue_def outputDataValue = {0,0};

    char errorBuffer[512];            // a buffer for formatting error messages

    bool firstFetch = false;

    MEMORY_ALLOC_ARRAY(sqlWarningOrError, BYTE, 1024);
    if(sqlWarningOrError == NULL)
    {
        // Exception should throw up to Java layer here.
        exit(1);
    }

    SRVR_STMT_HDL *pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;
    if(pSrvrStmt == NULL)
    {
        returnCode = SQL_ERROR;
        GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Statement Handle.", &sqlWarningOrErrorLength, sqlWarningOrError);
        // Exception should throw up to Java layer here.
        goto FETCH_EXIT;
    }

    if (srvrGlobal->fetchAhead && pSrvrStmt->sqlStmtType != TYPE_SELECT_CATALOG) 
    {   // T2 does not fetch ahead for now, for future implementation
        // set firstFetch here
        if (pSrvrStmt->rowsAffected == 0 && pSrvrStmt->m_curRowsFetched == 0
                && !pSrvrStmt->sqlWarningOrError)
            firstFetch = true;

        if (!firstFetch)
        {
            // need to send response to the client
            returnCode = pSrvrStmt->returnCodeForDelayedError;
            sqlWarningOrErrorLength = pSrvrStmt->delayedSqlWarningOrErrorLength;
            sqlWarningOrError = pSrvrStmt->delayedSqlWarningOrError;
            rowsAffected = pSrvrStmt->delayedRowsAffected;
            if (srvrGlobal->drvrVersion.buildId & ROWWISE_ROWSET)
                outValuesFormat = ROWWISE_ROWSETS;
            else
                outValuesFormat = COLUMNWISE_ROWSETS;
            outputDataValue._buffer = pSrvrStmt->delayedOutputDataValue._buffer;
            outputDataValue._length = pSrvrStmt->delayedOutputDataValue._length;

            odbc_SQLSrvr_Fetch_param_res_(pSrvrStmt 
                    , returnCode
                    , sqlWarningOrErrorLength
                    , sqlWarningOrError
                    , rowsAffected
                    , outValuesFormat
                    , outputDataValue._length
                    , outputDataValue._buffer);

            if (returnCode == SQL_NO_DATA_FOUND || returnCode == SQL_ERROR || returnCode == SQL_INVALID_HANDLE || returnCode == SQL_STILL_EXECUTING)
            {
                pSrvrStmt->returnCodeForDelayedError = SQL_SUCCESS; // reset returnCodeForDelayedError
                goto FETCH_EXIT; // fetch ahead is stopped
            }
        }

        odbc_SQLSrvr_FetchPerf_sme_(
                dialogueId,
                &returnCode,
                stmtHandle,
                maxRowCnt,
                maxRowLen,
                sqlAsyncEnable,
                queryTimeout,
                &rowsAffected,
                &outValuesFormat,
                &outputDataValue,
                &sqlWarningOrErrorLength,
                sqlWarningOrError);

        qrysrvc_ExecuteFinished(dialogueId, stmtHandle, false, returnCode, true);

        if (firstFetch)
        {
            odbc_SQLSrvr_Fetch_param_res_(pSrvrStmt
                    , returnCode
                    , sqlWarningOrErrorLength
                    , sqlWarningOrError
                    , rowsAffected
                    , outValuesFormat
                    , outputDataValue._length
                    , outputDataValue._buffer);

            if (returnCode == SQL_SUCCESS || returnCode == SQL_SUCCESS_WITH_INFO)
            {
                odbc_SQLSrvr_FetchPerf_sme_(
                        dialogueId,
                        &returnCode,
                        stmtHandle,
                        maxRowCnt,
                        maxRowLen,
                        sqlAsyncEnable,
                        queryTimeout,
                        &rowsAffected,
                        &outValuesFormat,
                        &outputDataValue,
                        &sqlWarningOrErrorLength,
                        sqlWarningOrError);
            }

        }

        if (pSrvrStmt != NULL)
        {
            pSrvrStmt->returnCodeForDelayedError = returnCode;
            pSrvrStmt->delayedRowsAffected = rowsAffected;
            // If ahead fetch got no data found return code, do not use old data buffer for sending useless data.
            if(returnCode==SQL_NO_DATA_FOUND){
                pSrvrStmt->delayedOutputDataValue._buffer=NULL;
                pSrvrStmt->delayedOutputDataValue._length=0;
            }
            else{
                pSrvrStmt->delayedOutputDataValue._buffer = outputDataValue._buffer;
                pSrvrStmt->delayedOutputDataValue._length = outputDataValue._length;
            }
            pSrvrStmt->delayedSqlWarningOrErrorLength = sqlWarningOrErrorLength;
            pSrvrStmt->delayedSqlWarningOrError = sqlWarningOrError;
        }
    }
    else
    {
        odbc_SQLSrvr_FetchPerf_sme_(dialogueId,
                &returnCode,
                stmtHandle,
                maxRowCnt,
                maxRowLen,
                sqlAsyncEnable,
                queryTimeout,
                &rowsAffected,
                &outValuesFormat,
                &outputDataValue,
                &sqlWarningOrErrorLength,
                sqlWarningOrError);

        if (pSrvrStmt->sqlNewQueryType == SQL_SP_RESULT_SET)
        {
            if (pSrvrStmt->callStmtHandle->inState == STMTSTAT_CLOSE)
            {
                pSrvrStmt = pSrvrStmt->callStmtHandle;
            }
        }

        qrysrvc_ExecuteFinished(dialogueId, stmtHandle, false, returnCode, true);

        odbc_SQLSrvr_Fetch_param_res_(pSrvrStmt 
                , returnCode
                , sqlWarningOrErrorLength
                , sqlWarningOrError
                , rowsAffected
                , outValuesFormat
                , outputDataValue._length
                , outputDataValue._buffer);
    }

FETCH_EXIT:

    MEMORY_DELETE_ARRAY(sqlWarningOrError);
    sqlWarningOrErrorLength = 0;

    return;

}  // end odbc_SQLSrvr_Fetch_ame_()


extern "C" void
odbc_SQLSrvr_ExecDirect_ame_(
        /* In    */ Long dialogueId
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
        )
{
    /* Not used for now, might be needed once
     * the logic of a normal statement changes
     * for performance enhancement purpose.
     * keep the portal here.
     */
} // odbc_SQLSrvr_ExecDirect_ame_()

extern "C" void
odbc_SQLSrvr_Execute2_ame_(
        /* In    */ Long dialogueId
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
        , /* In    */ IDL_long rowLength	      // For DBT to obtain the Rowlength
        , /* In    */ IDL_long_long txnID       // T4 driver sends a transaction ID which we need to join
        , /* In    */ IDL_long holdableCursor
        )
{
    IDL_long returnCode              = SQL_SUCCESS;
    IDL_long sqlWarningOrErrorLength = 0;
    BYTE     *sqlWarningOrError      = NULL;
    IDL_long rowsAffected            = 0;
    IDL_long estimatedCost           = 0;
    IDL_long outValuesLength         = 0;
    BYTE     *outValues              = NULL;
    SQLItemDescList_def outputItemDescList = {0,0};
    SQLValueList_def outputValueList = {0,0};
    ERROR_DESC_LIST_def sqlWarning = {0,0};
    char errorText[512];

    IDL_long cursorLength = (cursorName != NULL) ? strlen(cursorName) : 0;

    odbc_SQLSvc_ExecDirect_exc_ ExecDirect_exception_={0,0,0};
    odbc_SQLSvc_ExecuteCall_exc_ ExecCall_exception_={0,0,0};


    IDL_long	paramCount = 0;
    RETCODE		rc = 0;
    bool		isStatusRowsetDelayed = false;
    bool		noRepository = true;

    SRVR_STMT_HDL *pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;

    MEMORY_ALLOC_ARRAY(sqlWarningOrError, BYTE, 1024);
    if(sqlWarningOrError == NULL)
    {
        // Exception should throw up to Java layer here.
        exit(1);
    }

    if(pSrvrStmt == NULL)
    {
        returnCode = SQL_ERROR;
        GETMXCSWARNINGORERROR(-1, "HY000", "Invalid Statement Handle.", &sqlWarningOrErrorLength, sqlWarningOrError);
        // Exception should throw up to Java layer here.
    }

    if (pSrvrStmt != NULL) {
        paramCount = pSrvrStmt->paramCount;
        if ( pSrvrStmt->sqlQueryType == SQL_RWRS_SPECIAL_INSERT)
            pSrvrStmt->maxRowLen = rowLength;
    }

    //LCOV_EXCL_START
    // To improve the throughput as the server gets the first rowset, it returns back a success
    // code back to the application before it processes the rowset. The application can then send the
    // second rowset to the driver. By doing this, both the server and driver are always busy by
    // piggybacking the messages back and forth. Because the server and driver are always busy, the
    // application will always get the status error delayed by one rowset. For example, the application
    // sends 4 rowset of 10 rows. The first rowset will get all success back, then the second rowset
    // will get the status array for the first rowset. The second status array may have success,
    // warning, and errors for first rowset. Then the third rowset will have status array for second
    // rowset and so on. The last rowset will be a dummy to get the last status error for the previous
    // rowset which is the rowset with the valid data.
    if (srvrGlobal->drvrVersion.buildId & STREAMING_MODE || srvrGlobal->drvrVersion.buildId & STREAMING_DELAYEDERROR_MODE)
    {
        if(pSrvrStmt == NULL)
        {
            returnCode = SQL_ERROR;
            GETMXCSWARNINGORERROR(-1, "HY000", "Statement Label not found.", &sqlWarningOrErrorLength, sqlWarningOrError);
        }
        else
        {
            if(srvrGlobal->drvrVersion.buildId & STREAMING_DELAYEDERROR_MODE)
            {
                sqlWarningOrErrorLength = pSrvrStmt->sqlWarningOrErrorLength;
                sqlWarningOrError = pSrvrStmt->sqlWarningOrError;
                rowsAffected = pSrvrStmt->rowsAffected;
                estimatedCost = pSrvrStmt->rowsAffectedHigherBytes; // combine both rowsAffected and rowsAffectedHigherBytes as __int64 when interface between drvr/srvr changes
                outValuesLength = pSrvrStmt->outputDescVarBufferLen;
                outValues = pSrvrStmt->outputDescVarBuffer;
                returnCode = pSrvrStmt->returnCodeForDelayedError;
            }
            paramCount = pSrvrStmt->paramCount;
        }

        if(srvrGlobal->drvrVersion.buildId & STREAMING_DELAYEDERROR_MODE)
        {
            odbc_SQLSrvr_Execute_param_res_(
                    pSrvrStmt,
                    returnCode,
                    sqlWarningOrErrorLength,
                    sqlWarningOrError,
                    rowsAffected,
                    sqlQueryType,
                    estimatedCost,
                    outValuesLength,         // for exec2
                    outValues,               // for exec2
                    outputDescLength,        // for execdirect calls
                    outputDesc,              // for execdirect calls
                    stmtHandle,              // for SPJ result sets
                    (pSrvrStmt != NULL) ? pSrvrStmt->myKey : 0
                    );

            isStatusRowsetDelayed = true;
            returnCode = SQL_SUCCESS;
        }
    }
    //LCOV_EXCL_STOP

    if(returnCode != SQL_SUCCESS)
    {
        // do nothing
    }
    else
    {
        bool bExecute2withRowsets = true;
        bool bExecute2 = true;

        bExecute2withRowsets = 	((inputRowCnt > 1) ||
                ((inputRowCnt==1) &&
                 (pSrvrStmt->preparedWithRowsets))) &&
            (pSrvrStmt->sqlQueryType != SQL_RWRS_SPECIAL_INSERT);

        bExecute2 = (inputRowCnt > 0) ||
            sqlQueryType != SQL_UNKNOWN  ||
            ( inputRowCnt == 0 && pSrvrStmt != NULL &&
              (pSrvrStmt->sqlQueryType != SQL_INSERT_UNIQUE &&
               pSrvrStmt->sqlQueryType != SQL_INSERT_NON_UNIQUE));
        DO_WouldLikeToExecute(dialogueId, stmtHandle, &returnCode, &sqlWarningOrErrorLength, sqlWarningOrError);
        if (returnCode == SQL_SUCCESS && pSrvrStmt != NULL)
        {
            if(bExecute2withRowsets)
            {
                odbc_SQLSvc_Execute2withRowsets_sme_(dialogueId, sqlAsyncEnable, queryTimeout,
                        inputRowCnt, sqlStmtType, stmtHandle, cursorLength, cursorName, cursorCharset, holdableCursor,
                        inValuesLength, inValues, &returnCode, &sqlWarningOrErrorLength, sqlWarningOrError,
                        &rowsAffected, &outValuesLength, outValues);
            }
            else
            {
                if(bExecute2)
                    odbc_SQLSvc_Execute2_sme_(dialogueId, sqlAsyncEnable, queryTimeout,
                            inputRowCnt, sqlStmtType, stmtHandle, cursorLength, cursorName, cursorCharset, holdableCursor,
                            inValuesLength, inValues, &returnCode, &sqlWarningOrErrorLength, sqlWarningOrError,
                            &rowsAffected, &outValuesLength, outValues);
            }
        }
    }

    qrysrvc_ExecuteFinished(dialogueId, stmtHandle, true, returnCode, false);

    //LCOV_EXCL_START
    // To improve the throughput as the server gets the first rowset, it returns back a success
    // code back to the application before it processes the rowset. The application can then send the
    // second rowset to the driver. By doing this, both the server and driver are always busy by
    // piggybacking the messages back and forth. Because the server and driver are always busy, the
    // application will always get the status error delayed by one rowset. For example, the application
    // sends 4 rowset of 10 rows. The first rowset will get all success back, then the second rowset
    // will get the status array for the first rowset. The second status array may have success,
    // warning, and errors for first rowset. Then the third rowset will have status array for second
    // rowset and so on. The last rowset will be a dummy to get the last status error for the previous
    // rowset which is the rowset with the valid data.

    if (!isStatusRowsetDelayed)
    {
        if (pSrvrStmt != NULL)
            estimatedCost = pSrvrStmt->rowsAffectedHigherBytes; // combine both rowsAffected and rowsAffectedHigherBytes as __int64 when interface between drvr/srvr changes
        odbc_SQLSrvr_Execute_param_res_(
                pSrvrStmt,
                returnCode,
                sqlWarningOrErrorLength,
                sqlWarningOrError,
                rowsAffected,
                sqlQueryType,
                estimatedCost,
                outValuesLength,         // for exec2
                outValues,               // for exec2
                outputDescLength,        // for execdirect calls
                outputDesc,              // for execdirect calls
                stmtHandle,              // for SPJ result sets
                (pSrvrStmt != NULL) ? pSrvrStmt->myKey : 0
                );
    }

    MEMORY_DELETE_ARRAY(sqlWarningOrError);
    sqlWarningOrErrorLength = 0;

    return;
} // odbc_SQLSrvr_Execute2_ame_()


static bool strincmp(char* in, char* out, short ilen)
{
    short i = 0;
    char* iin = in;
    char* oout = out;
    char ich;
    char och;

    while (*iin != '\0' && i++ < ilen)
    {
        ich = *iin++;
        och = *oout++;
        if ((ich | 0x20) != (och | 0x20))
            return false;
    }
    return true;
}

short DO_WouldLikeToExecute(
        Long dialogueId
        , Long stmtHandle
        , IDL_long* returnCode
        , IDL_long* sqlWarningOrErrorLength
        , BYTE*& sqlWarningOrError
        )
{
    SRVR_STMT_HDL *pSrvrStmt = NULL;
    if (stmtHandle != NULL)
        pSrvrStmt = getSrvrStmt(dialogueId, stmtHandle, FALSE);
    else
        pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;

    if (pSrvrStmt == NULL)
        return 0;

    pQueryStmt = pSrvrStmt;
    return 0;
}

short qrysrvc_ExecuteFinished(Long dialogueId
        , const Long stmtHandle
        , const bool bCheckSqlQueryType
        , const short error_code
        , const bool bFetch
        , const bool bException
        , const bool bErase
        )
{
    SRVR_STMT_HDL *pSrvrStmt = NULL;

#define RC_SUCCESS(retcode) \
    ((!bException && (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)) || \
     ( bException && (retcode == CEE_SUCCESS)) ? TRUE : FALSE)

    if (stmtHandle != NULL)
        pSrvrStmt = getSrvrStmt(dialogueId, stmtHandle, FALSE);
    else
        pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;

    if (pSrvrStmt == NULL)
        return 0;

    pSrvrStmt->m_bqueryFinish = true;

    if (bCheckSqlQueryType)
    {
        if (RC_SUCCESS(error_code) &&
                (pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE ||
                 pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE ||
                 pSrvrStmt->sqlQueryType == SQL_CALL_WITH_RESULT_SETS ||
                 pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET))
        {
            pSrvrStmt->m_bqueryFinish = false;
            return 0;
        }
    }
    else if (bFetch)
    {
        if (RC_SUCCESS(error_code))
        {
            pSrvrStmt->m_bqueryFinish = false;
            return 0;
        }
    }

    pQueryStmt = NULL;

    return 0;
}

