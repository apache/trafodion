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
// MODULE: CSrvrStmt.cpp
//
// PURPOSE: Implements the member functions of CSrvrStmt class
//

/*Change Log
 * Methods Changed: Removed setOfCQD & added listOfCQD
 */
#include "CSrvrStmt.h"
#include "SqlInterface.h"
#include "SrvrKds.h"
#include "SrvrCommon.h"
#include "CommonDiags.h"
//#include "pThreadsSync.h"
#include "Debug.h"
//#include <thread_safe_extended.h>

static const int QUAD_THRESHOLD = 2;

SRVR_STMT_HDL::SRVR_STMT_HDL(long inDialogueId)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::SRVR_STMT_HDL",("inDialogueId=%ld)",inDialogueId));

    cursorName[0] = '\0';
    previousCursorName[0] = '\0';
    stmtName[0] = '\0';
    paramCount = 0;
    columnCount = 0;
    SqlQueryStatementType = INVALID_SQL_QUERY_STMT_TYPE;
    stmtType = EXTERNAL_STMT;
    inputDescVarBuffer = NULL;
    outputDescVarBuffer = NULL;
    inputDescVarBufferLen = 0;
    outputDescVarBufferLen = 0;
    sqlQueryType = SQL_UNKNOWN;
    inputDescBuffer = NULL;
    outputDescBuffer = NULL;
    inputDescBufferLength = 0;
    outputDescBufferLength  = 0;
    outputDescBufferLength = 0;
    outputDescBuffer = NULL;
    sqlWarningOrErrorLength = 0;
    sqlWarningOrError = NULL;
    delayedSqlWarningOrErrorLength = 0;
    delayedSqlWarningOrError = NULL;
    SpjProxySyntaxString = NULL;
    SpjProxySyntaxStringLen = 0;
    SqlDescInfo = NULL;
    endOfData = FALSE;

    // The following were added for SPJRS support
    isSPJRS = FALSE;
    RSIndex = 0;
    RSMax = 0;

    currentMethod = UNKNOWN_METHOD;
    asyncThread = NULL;
    queryTimeoutThread = NULL;
    threadStatus = SQL_SUCCESS;
    threadId = 0;
    threadReturnCode = SQL_SUCCESS;

    sqlAsyncEnable = SQL_ASYNC_ENABLE_OFF;
    queryTimeout = 0;
    sqlString.dataValue._buffer = NULL;
    sqlString.dataValue._length = 0;
    sqlStringLen = 0;
    sqlStringText = NULL;
    inputRowCnt = 0;
    maxRowsetSize = ROWSET_NOT_DEFINED;
    maxRowCnt = 0;
    sqlStmtType = TYPE_UNKNOWN;
    freeResourceOpt = SQL_CLOSE;
    inputValueList._length = 0;
    inputValueList._buffer = NULL;

    numResultSets  = 0;
    previousSpjRs  = NULL;
    nextSpjRs      = NULL;
    callStmtId     = NULL;
    resultSetIndex = 0;
    callStmtHandle = NULL;
    errRowsArray   = NULL;

    NA_supported = false;
    bSQLMessageSet = false;
    bSQLValueListSet = false;
    bFetchStarted = false;
    preparedWithRowsets = false;

    inputQuadList         =    NULL;
    inputQuadList_recover =    NULL;
    transportBuffer       =    NULL;
    transportBufferLen    =    0;

    outputQuadList        =    NULL;
    outputBuffer          =    NULL;
    outputBufferLength    =    0;
    returnCodeForDelayedError = SQL_SUCCESS;
    
    estimatedCost = 0;
    rowsAffected = 0;
    inputDescList._length = 0;
    inputDescList._buffer = NULL;
    outputDescList._length = 0;
    outputDescList._buffer = NULL;
    CLEAR_WARNING(sqlWarning);
    CLEAR_ERROR(sqlError);
    outputValueList._length = 0;
    outputValueList._buffer = NULL;
    outputValueVarBuffer = NULL;
    inputValueVarBuffer = NULL;
    clientLCID = srvrGlobal->clientLCID;
    rowCount._length = 0;
    rowCount._buffer = NULL;
    isReadFromModule = FALSE;
    moduleName[0] = '\0';
    inputDescName[0] = '\0';
    outputDescName[0] = '\0';
    isClosed = TRUE;
    IPD = NULL;
    IRD = NULL;
    useDefaultDesc = FALSE;
    dialogueId = inDialogueId;
    nowaitRetcode = SQL_SUCCESS;
    holdability = CLOSE_CURSORS_AT_COMMIT;
    fetchQuadEntries = 0;
    fetchRowsetSize = 0;
    fetchQuadField = NULL;
    batchQuadEntries = 0;
    batchRowsetSize = 0;
    batchQuadField = NULL;
    inputDescParamOffset = 0;
    batchMaxRowsetSize = 0;
    stmtInitForNowait = FALSE;
    // +++ T2_REPO
    bLowCost = false;   // May not need this
    m_need_21036_end_msg = false;
    bzero(m_shortQueryText, sizeof(m_shortQueryText));
    m_rmsSqlSourceLen = 0;
    stmtNameLen = 0;
    m_lastQueryEndTime = 0;
    m_lastQueryEndCpuTime = 0;
    m_bqueryFinish = false;
    inState = STMTSTAT_NONE;
    sqlQueryType = SQL_UNKNOWN;
    sqlUniqueQueryIDLen = 0;
    sqlPlan = NULL;
    //
    // Rowsets
    callStmtId     = NULL;
    resultSetIndex = 0;
    
    // Rowsets - phase 2
    outputDataValue._length = 0;
    outputDataValue._buffer = NULL;
    outputDataValue.pad_to_offset_8_ = {'\0', '\0', '\0', '\0'};
    inputDataValue._length = 0;
    inputDataValue._buffer = NULL;
    delayedOutputDataValue._length = 0;
    delayedOutputDataValue._buffer = NULL;
    current_holdableCursor = SQL_NONHOLDABLE;
    holdableCursor = SQL_NONHOLDABLE;
    myKey = 0; // Will be evaluated by createSrvrStmt()
               // in SRVR_CONNECT_HDL with value count

    sqlPlanLen = 0;

    m_wbuffer = NULL;
    m_rbuffer = NULL;
    m_wbuffer_length = 0;
    m_rbuffer_length = 0;

    FUNCTION_RETURN_VOID((NULL));
}

SRVR_STMT_HDL::SRVR_STMT_HDL()
{
    FUNCTION_ENTRY("SRVR_STMT_HDL()::SRVR_STMT_HDL",(NULL));
    SRVR_STMT_HDL(0);
    FUNCTION_RETURN_VOID((NULL));
}

SRVR_STMT_HDL::~SRVR_STMT_HDL()
{
    FUNCTION_ENTRY("SRVR_STMT_HDL()::~SRVR_STMT_HDL",(NULL));
    int retcode;
    cleanupAll();
    inState = STMTSTAT_NONE;
#ifndef DISABLE_NOWAIT
    if (stmtInitForNowait) mutexCondDestroy(&cond, &mutex);
#endif
    FUNCTION_RETURN_VOID((NULL));
}

SQLRETURN SRVR_STMT_HDL::Prepare(const SQLValue_def *inSqlString, short inStmtType, short inHoldability,
                                 long inQueryTimeout,bool isISUD)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::Prepare",(""));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inSqlString='%s'",
        CLI_SQL_VALUE_STR(inSqlString)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inStmtType=%s, inHoldability=%d, inQueryTimeout=%ld, isISUD=%d",
        CliDebugStatementType(inStmtType),
        inHoldability,
        inQueryTimeout,isISUD));

    SQLRETURN rc;
    size_t  len;
    this->isISUD = isISUD;
    if (isReadFromModule)   // Already SMD label is found
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
    // cleanup all memory allocated in the previous operations
    cleanupAll();
    sqlString.dataCharset = inSqlString->dataCharset;
    sqlString.dataType = inSqlString->dataType;
    MEMORY_ALLOC_ARRAY(sqlString.dataValue._buffer,unsigned char,inSqlString->dataValue._length+1);
    sqlString.dataValue._length = inSqlString->dataValue._length+1;

    strncpy((char *)sqlString.dataValue._buffer, (const char *)inSqlString->dataValue._buffer, inSqlString->dataValue._length);
    sqlString.dataValue._buffer[inSqlString->dataValue._length] = '\0';

    sqlStringLen = inSqlString->dataValue._length;
    MEMORY_ALLOC_ARRAY(sqlStringText, char, sqlStringLen+1);
    strncpy((char *)sqlStringText, (const char *)inSqlString->dataValue._buffer, sqlStringLen);
    sqlStringText[inSqlString->dataValue._length] = '\0';

    stmtType = inStmtType;
    holdability = inHoldability;

    CLI_DEBUG_RETURN_SQL(PREPARE(this));
}

SQLRETURN SRVR_STMT_HDL::Execute(const char *inCursorName, IDL_long inInputRowCnt, IDL_short inSqlStmtType, 
        const SQLValueList_def *inValueList, 
        short inSqlAsyncEnable, Int32 inQueryTimeout)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::Execute",("Rowsets entry"));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inCursorName=%s, inInputRowCnt=%ld, inSqlStmtType=%s",
                DebugString(inCursorName),
                inInputRowCnt,
                CliDebugSqlStatementType(inSqlStmtType)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inValueList=0x%08x",
                inValueList));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inSqlAsyncEnable=%d, inQueryTimeout=%ld",
                inSqlAsyncEnable,
                inQueryTimeout));
    SQLRETURN rc;

    if(bSQLMessageSet)
        cleanupSQLMessage();
    if(bSQLValueListSet)
        cleanupSQLValueList();
    inputRowCnt = inInputRowCnt;
    sqlStmtType = inSqlStmtType;

    if (inCursorName != NULL && inCursorName[0] != '\0')
    {
        cursorNameLen = strlen(inCursorName);
        cursorNameLen = cursorNameLen < sizeof(cursorName)? cursorNameLen : sizeof(cursorName);
        strcpyUTF8(cursorName, inCursorName, sizeof(cursorName));
    }
    else
        cursorName[0] = '\0';


    inputValueList._buffer = inValueList->_buffer;
    inputValueList._length = inValueList->_length;

    currentMethod = odbc_SQLSvc_ExecuteN_ldx_;

    rc = EXECUTE_R(this);

    switch (rc)
    {
        case SQL_SUCCESS:
            break;
        case ODBC_RG_WARNING:
            rc = SQL_SUCCESS_WITH_INFO;
        case SQL_SUCCESS_WITH_INFO:
            GETSQLERROR2(bSQLMessageSet, &sqlError);
            break;
        case SQL_ERROR:
            GETSQLERROR2(bSQLMessageSet, &sqlError);
            break;
        case ODBC_SERVER_ERROR:
            // Allocate Error Desc
            kdsCreateSQLErrorException(&sqlError, 1, bSQLMessageSet);
            // Add SQL Error
            kdsCopySQLErrorException(&sqlError, NULL_VALUE_ERROR, NULL_VALUE_ERROR_SQLCODE,
                    NULL_VALUE_ERROR_SQLSTATE);
            break;
        case -8814:
        case 8814:
            rc = SQL_RETRY_COMPILE_AGAIN;
            break;
        default:
            break;
    }

    CLI_DEBUG_RETURN_SQL(rc);
}

SQLRETURN SRVR_STMT_HDL::Execute(const char *inCursorName, long totalRowCount, short inSqlStmtType,
                                 const SQLValueList_def *inValueList,
                                 short inSqlAsyncEnable, long inQueryTimeout,
                                 SQLValueList_def *outValueList)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::Execute",(""));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inCursorName=%s, totalRowCount=%ld, inSqlStmtType=%s",
        DebugString(inCursorName),
        totalRowCount,
        CliDebugSqlStatementType(inSqlStmtType)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inValueList=0x%08x",
        inValueList));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inSqlAsyncEnable=%d, inQueryTimeout=%ld",
        inSqlAsyncEnable,
        inQueryTimeout));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  outValueList=0x%08x",
        outValueList));
    SQLRETURN rc;
    char *saveptr=NULL;
    SRVR_CONNECT_HDL *pConnect = NULL;

    if (dialogueId == 0) CLI_DEBUG_RETURN_SQL(SQL_ERROR);
    pConnect = (SRVR_CONNECT_HDL *)dialogueId;

    cleanupSQLMessage();
    if (rowCount._buffer == NULL || batchRowsetSize > inputRowCnt)
    {
       inputRowCnt = batchRowsetSize;
           if (inputRowCnt == 0)
              inputRowCnt = 1;
           if (rowCount._buffer != NULL)
              MEMORY_DELETE(rowCount._buffer);
       MEMORY_ALLOC_ARRAY(rowCount._buffer,int, inputRowCnt);
       rowCount._length = 0;
        }
    memset(rowCount._buffer,0,inputRowCnt*sizeof(int));

    sqlStmtType = inSqlStmtType;
    if (inCursorName != NULL)
    {
        if (strlen(inCursorName) < MAX_CURSOR_NAME_LEN)
            strcpy(cursorName, inCursorName);
        else
        {
            strncpy(cursorName, inCursorName, MAX_CURSOR_NAME_LEN);
            cursorName[MAX_CURSOR_NAME_LEN] = '\0';
        }
    }
    else
        cursorName[0] = '\0';

    inputValueList._buffer = inValueList->_buffer;
    inputValueList._length = inValueList->_length;

    // Create the output value list
    if (outputValueList._buffer == NULL)
    {
        if ((rc = AllocAssignValueBuffer(&outputDescList, &outputValueList, outputDescVarBufferLen,
            1, outputValueVarBuffer)) != SQL_SUCCESS)
            CLI_DEBUG_RETURN_SQL(rc);
    }
    else
    {
        outputValueList._length = 0;
    }

    DEBUG_OUT(DEBUG_LEVEL_CLI,("Execute(outputValueList=0x%08x, _buffer=0x%08x, _length=0x%08x)",
        &outputValueList,
        outputValueList._buffer,
        outputValueList._length));

    rc = EXECUTE(this);

    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        outValueList->_buffer = outputValueList._buffer;
        outValueList->_length = outputValueList._length;
        //MFC update srvrGlobal if any CQD/catalog/schema is set
        if (this->sqlString.dataValue._buffer != NULL)
        {
            if (this->SqlQueryStatementType == 9) // CQD is being set here
            {
                // The CQDs which are considered for creating HASH
                // name of the Module File. Right now this conisders
                // only the CQDs set by the JDBC/MX T2 Driver.
                //Modified for sol 10-100618-1193
                //srvrGlobal->setOfCQD.insert(this->sqlString.dataValue._buffer);
                ((SRVR_CONNECT_HDL *)dialogueId)->listOfCQDs.push_back((const char *)this->sqlString.dataValue._buffer);
            }
            if (this->SqlQueryStatementType == 11) // set catalog
            {
                char currentSqlString[100];
                strcpy(currentSqlString,(const char *)this->sqlString.dataValue._buffer);
                strToUpper(currentSqlString);
                char *stringtoken = strtok_r(currentSqlString," ",&saveptr);
                stringtoken = strtok_r(NULL," ",&saveptr);
                stringtoken = strtok_r(NULL," ;'",&saveptr);
                strcpy(pConnect->CurrentCatalog,(stringtoken));
            }
            if(this->SqlQueryStatementType == 12) // set schema
            {
                char currentSqlString1[100],currentSqlString2[100];
                strcpy(currentSqlString1,(const char *)this->sqlString.dataValue._buffer);
                strToUpper(currentSqlString1);

                saveptr=NULL;
                char *stringtoken = strtok_r(currentSqlString1," ",&saveptr);
                stringtoken = strtok_r(NULL," ",&saveptr);
                stringtoken = strtok_r(NULL," ;\n\t",&saveptr);
                strcpy(currentSqlString2,stringtoken);

                int pos = strcspn(stringtoken,".");
                if (pos == strlen(stringtoken))
                    strcpy(pConnect->CurrentSchema,(stringtoken));
                else
                {
                    saveptr=NULL;
                    stringtoken = strtok_r(currentSqlString2,".",&saveptr);
                    strcpy(pConnect->CurrentCatalog,(stringtoken));
                    stringtoken = strtok_r(NULL,"; \t\n",&saveptr);
                    strcpy(pConnect->CurrentSchema,(stringtoken));
                }
            }
        }
        //MFC update srvrGlobal end
        break;
    case ODBC_SERVER_ERROR:
        // Allocate Error Desc
        kdsCreateSQLErrorException(&sqlError, 1);
        // Add SQL Error
        kdsCopySQLErrorException(&sqlError, NULL_VALUE_ERROR, NULL_VALUE_ERROR_SQLCODE,
            NULL_VALUE_ERROR_SQLSTATE);
        break;
    case -8814:
    case 8814:
        // SQL Error/Warning 8814: The transaction mode at run time (value) differs from that
        // specified at compile time (value). 8814 is translated to a SQL_RETRY_COMPILE_AGAIN
        // (-104) error.
        rc = SQL_RETRY_COMPILE_AGAIN;
        break;
    default:
        break;
    }
    CLI_DEBUG_RETURN_SQL(rc);
}

SQLRETURN SRVR_STMT_HDL::Close(unsigned short inFreeResourceOpt)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_STMT,"SRVR_STMT_HDL::Close",("inFreeResourceOpt=%d",
        inFreeResourceOpt));
    SQLRETURN rc;

    if (stmtType == INTERNAL_STMT) CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
    cleanupSQLMessage();
    freeResourceOpt = inFreeResourceOpt;
    rc = FREESTATEMENT(this);
    if (inFreeResourceOpt == SQL_DROP)
        removeSrvrStmt(dialogueId, (long)this);

    CLI_DEBUG_RETURN_SQL(rc);
}

SQLRETURN SRVR_STMT_HDL::InternalStmtClose(unsigned short inFreeResourceOpt)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::InternalStmtClose",("inFreeResourceOpt=%d",
        inFreeResourceOpt));

    SQLRETURN rc;
    cleanupSQLMessage();
    freeResourceOpt = inFreeResourceOpt;
    CLI_DEBUG_RETURN_SQL(FREESTATEMENT(this));
}

SQLRETURN SRVR_STMT_HDL::Fetch(long inMaxRowCnt, short inSqlAsyncEnable, long inQueryTimeout)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::Fetch",("inMaxRowCnt=%ld, inSqlAsyncEnable=%d, inQueryTimeout=%ld",
        inMaxRowCnt,
        inSqlAsyncEnable,
        inQueryTimeout));

    SQLRETURN rc;
    cleanupSQLMessage();
    if (outputValueList._buffer == NULL  || maxRowCnt < inMaxRowCnt)
    {
        cleanupSQLValueList();
        rc = AllocAssignValueBuffer(&outputDescList, &outputValueList, outputDescVarBufferLen,
            inMaxRowCnt, outputValueVarBuffer);
        if (rc != SQL_SUCCESS)
            CLI_DEBUG_RETURN_SQL(rc);
    }
    else
        // Reset the length to 0, but the _buffer points to array of required SQLValue_defs
        outputValueList._length = 0;

    maxRowCnt = inMaxRowCnt;
    CLI_DEBUG_RETURN_SQL(FETCH(this));
}

SQLRETURN SRVR_STMT_HDL::ExecDirect(const char *inCursorName, const SQLValue_def *inSqlString,
                                    short inStmtType, short inSqlStmtType,
                                    short inHoldability, long inQueryTimeout)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_STMT,"SRVR_STMT_HDL::ExecDirect",(""));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inCursorName=%s",
        DebugString(inCursorName)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY|DEBUG_LEVEL_STMT,("  inSqlString=%s",
        CLI_SQL_VALUE_STR(inSqlString)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY|DEBUG_LEVEL_STMT,("  inStmtType=%s, inSqlStmtType=%s",
        CliDebugStatementType(inStmtType),
        CliDebugSqlStatementType(inSqlStmtType)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inHoldability=%d, inQueryTimeout=%ld",
        inHoldability,
        inQueryTimeout));

    SQLRETURN rc;
    size_t  len;

    SQLValueList_def inValueList;
    SQLValueList_def outValueList;

    rc = Prepare(inSqlString, inStmtType, inHoldability, inQueryTimeout);

    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        CLI_DEBUG_RETURN_SQL(rc);
    inValueList._buffer = NULL;
    inValueList._length = 0;

    rc = Execute(inCursorName, 1, inSqlStmtType, &inValueList, FALSE, inQueryTimeout, &outValueList);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        CLI_DEBUG_RETURN_SQL(rc);
    if (rowCount._buffer != 0)
        rowsAffected = *rowCount._buffer;
    else
        rowsAffected = 0;
    CLI_DEBUG_RETURN_SQL(rc);
}

SQLRETURN SRVR_STMT_HDL::ExecSPJRS(void)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::ExecSPJRS",(""));

    SQLRETURN rc;

    rc = EXECUTESPJRS(this);

    if(rc == SQL_RS_DOES_NOT_EXIST)
    {
        CLEARDIAGNOSTICS(this);
    }
    CLI_DEBUG_RETURN_SQL(rc);
}

SQLRETURN SRVR_STMT_HDL::Cancel(void)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::Cancel",(NULL));

    CLI_DEBUG_RETURN_SQL(CANCEL(this));
}

void  SRVR_STMT_HDL::cleanupSQLMessage(void)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::cleanupSQLMessage",(NULL));

    unsigned long i;
    ERROR_DESC_def *errorDesc;
    // Cleanup SQLWarning
    for (i = 0 ; i < sqlWarning._length && sqlWarning._buffer != NULL ; i++)
    {
        errorDesc = (ERROR_DESC_def *)sqlWarning._buffer + i;
        MEMORY_DELETE(errorDesc->errorText);
    }
    MEMORY_DELETE(sqlWarning._buffer);
    sqlWarning._length = 0;

    // Cleanup sqlErrror
    for (i = 0 ; i < sqlError.errorList._length && sqlError.errorList._buffer != NULL ; i++)
    {
        errorDesc = (ERROR_DESC_def *)sqlError.errorList._buffer + i;
        MEMORY_DELETE(errorDesc->errorText);
        MEMORY_DELETE(errorDesc->Param1);
        MEMORY_DELETE(errorDesc->Param2);
        MEMORY_DELETE(errorDesc->Param3);
        MEMORY_DELETE(errorDesc->Param4);
        MEMORY_DELETE(errorDesc->Param5);
        MEMORY_DELETE(errorDesc->Param6);
        MEMORY_DELETE(errorDesc->Param7);
    }
    MEMORY_DELETE(sqlError.errorList._buffer);
    sqlError.errorList._length = 0;
    FUNCTION_RETURN_VOID((NULL));
}

void  SRVR_STMT_HDL::cleanupSQLValueList(void)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::cleanupSQLValueList",(NULL));

    bSQLValueListSet = false;
    MEMORY_DELETE_ARRAY(outputValueList._buffer);
    MEMORY_DELETE_ARRAY(outputValueVarBuffer);
    outputValueList._length = 0;
    maxRowCnt = 0;
    if (rowCount._buffer != NULL)
    {
        MEMORY_DELETE(rowCount._buffer);
    }
    rowCount._buffer = NULL;
    rowCount._length = 0;
    
    FUNCTION_RETURN_VOID((NULL));
}

void  SRVR_STMT_HDL::cleanupSQLDescList(void)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::cleanupSQLDescList",(NULL));

    MEMORY_DELETE_ARRAY(inputDescList._buffer);
    MEMORY_DELETE_ARRAY(inputDescVarBuffer);
    inputDescList._length = 0;
    inputDescVarBufferLen = 0;

    MEMORY_DELETE_ARRAY(outputDescList._buffer);
    MEMORY_DELETE_ARRAY(outputDescVarBuffer);
    outputDescList._length = 0;
    outputDescVarBufferLen = 0;

    MEMORY_DELETE_ARRAY(inputDescBuffer);
    inputDescBufferLength = 0;

    MEMORY_DELETE_ARRAY(outputDescBuffer);
    outputDescBufferLength = 0;

    MEMORY_DELETE_ARRAY(SqlDescInfo);

    FUNCTION_RETURN_VOID((NULL));
}

void  SRVR_STMT_HDL::cleanupAll(void)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::cleanupAll",(NULL));

    MEMORY_DELETE_ARRAY(sqlString.dataValue._buffer);
    sqlString.dataValue._length = 0;
    
    MEMORY_DELETE_ARRAY(sqlStringText);

    cleanupSQLMessage();
    cleanupSQLDescList();
    cleanupSQLValueList();
    inputValueList._buffer = NULL;
    inputValueList._length = 0;
    inputValueVarBuffer = NULL;
    MEMORY_DELETE_ARRAY(rowCount._buffer);
    rowCount._length = 0;
    w_release();
    r_release();
    MEMORY_DELETE_ARRAY(IPD);
    MEMORY_DELETE_ARRAY(IRD);
    MEMORY_DELETE_ARRAY(fetchQuadField);
    MEMORY_DELETE_ARRAY(batchQuadField);
    
    MEMORY_DELETE_ARRAY(sqlPlan);
    sqlPlanLen = 0;

    MEMORY_DELETE_ARRAY(inputQuadList);
    MEMORY_DELETE_ARRAY(inputQuadList_recover);
    MEMORY_DELETE_ARRAY(outputQuadList);
   
    MEMORY_DELETE_ARRAY(errRowsArray);
    
    MEMORY_DELETE_ARRAY(sqlWarningOrError);
    sqlWarningOrErrorLength = 0;

    exPlan = UNAVAILABLE;

    current_holdableCursor = SQL_NONHOLDABLE;
    holdableCursor = SQL_NONHOLDABLE;
    sqlQueryType = SQL_UNKNOWN;

    FUNCTION_RETURN_VOID((NULL));
}

SQLRETURN SRVR_STMT_HDL::PrepareFromModule(short inStmtType)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::PrepareFromModule",("inStmtType=%s",
        CliDebugStatementType(inStmtType)));

    SQLRETURN rc;
    size_t  len;
    if (srvrGlobal->moduleCaching)
    {
        if (!this->isClosed)
        {
            long retcode = SQL_SUCCESS;
            SQLSTMT_ID  *pStmt = &(this->stmt);
            retcode = CLI_CloseStmt(pStmt);

            if (retcode!=0) retcode = CLI_ClearDiagnostics(pStmt);
            this->isClosed = TRUE;
        }
    }

    if (isReadFromModule) CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
    // cleanup all memory allocated in the previous operations
    cleanupAll();
    stmtType = inStmtType;
    estimatedCost = -1;
    rc = PREPARE_FROM_MODULE(this);
    if (rc != SQL_ERROR)
        isReadFromModule = TRUE;
    CLI_DEBUG_RETURN_SQL(rc);
}

SQLRETURN SRVR_STMT_HDL::freeBuffers(short descType)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::freeBuffers",("descType=%d",
        descType));

    switch (descType)
    {
    case SQLWHAT_INPUT_DESC:
        if(inputDescVarBuffer != NULL)
            MEMORY_DELETE_ARRAY(inputDescVarBuffer);
        inputDescVarBuffer = NULL;
        inputDescVarBufferLen = 0;
        paramCount = 0;
        break;
    case SQLWHAT_OUTPUT_DESC:
        if(outputDescVarBuffer != NULL)
            MEMORY_DELETE_ARRAY(outputDescVarBuffer);
        outputDescVarBuffer = NULL;
        outputDescVarBufferLen = 0;
        columnCount = 0;
        break;
    default:
        CLI_DEBUG_RETURN_SQL(SQL_ERROR);
    }
    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

void SRVR_STMT_HDL::processThreadReturnCode(void)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::processThreadReturnCode",(NULL));

    switch (threadReturnCode)
    {
    case SQL_SUCCESS:
    case ODBC_RG_ERROR:
        break;
    case SQL_SUCCESS_WITH_INFO:
        GETSQLWARNING(this, &sqlWarning);
        break;
    case SQL_ERROR:
        GETSQLERROR(this, &sqlError);
        break;
    case ODBC_RG_WARNING:
        // if there is RG_WARNING, we don't pass SQL Warning to the application
        // Hence, we need to clear any warnings
        // TODO: Pass SQL warnings also to the application
        // call SQL_EXEC_ClearDiagnostics
        CLEARDIAGNOSTICS(this);
    case ODBC_SERVER_ERROR:
        // Allocate Error Desc
        kdsCreateSQLErrorException(&sqlError, 1);
        // Add SQL Error
        kdsCopySQLErrorException(&sqlError, NULL_VALUE_ERROR, NULL_VALUE_ERROR_SQLCODE,
            NULL_VALUE_ERROR_SQLSTATE);
        threadReturnCode = SQL_ERROR;
        break;
    case -8814:
    case 8814:
        // SQL Error/Warning 8814: The transaction mode at run time (value) differs from that
        // specified at compile time (value). 8814 is translated to a SQL_RETRY_COMPILE_AGAIN
        // (-104) error.
        threadReturnCode = SQL_RETRY_COMPILE_AGAIN;
        break;
    case NOWAIT_ERROR:
        // Allocate Error Desc
        kdsCreateSQLErrorException(&sqlError, 1);
        kdsCopySQLErrorException(&sqlError, SQLSVC_EXCEPTION_NOWAIT_ERROR, nowaitRetcode,
            "HY000");
        threadReturnCode = SQL_ERROR;
        break;
    }
    FUNCTION_RETURN_VOID((NULL));
}

SQLRETURN SRVR_STMT_HDL::allocSqlmxHdls(const char *inStmtName, const char *inModuleName,
        long long inModuleTimestamp, long inModuleVersion, short inSqlStmtType,
        BOOL inUseDefaultDesc, const char *inInputDescName,
        const char *inOutputDescName)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::allocSqlmxHdls",(""));

    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inStmtName=%s",
        DebugString(inStmtName)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inModuleName=%s",
        DebugString(inModuleName)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inModuleTimestamp=%s, inModuleVersion=%ld",
        DebugTimestampStr(inModuleTimestamp),
        inModuleVersion));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inSqlStmtType=%s, inUseDefaultDesc=%d",
        CliDebugSqlStatementType(inSqlStmtType),
        inUseDefaultDesc));

    SQLRETURN rc = SQL_SUCCESS;

    stmtNameLen = strlen(inStmtName);
    stmtNameLen = stmtNameLen < sizeof(stmtName)? stmtNameLen : sizeof(stmtName);
    strcpyUTF8(stmtName, inStmtName, sizeof(stmtName));

    if (inModuleName != NULL)
    {
        moduleId.version = inModuleVersion;
        strcpy(moduleName, inModuleName);
        moduleId.module_name = moduleName;
        moduleId.module_name_len = strlen(moduleName);
        moduleId.charset = "ISO88591";
        moduleId.creation_timestamp = inModuleTimestamp;
    }
    else
    {
        moduleId.version = SQLCLI_ODBC_MODULE_VERSION;
        moduleId.module_name = NULL;
        moduleId.module_name_len = 0;
        moduleId.charset = "ISO88591";
        moduleId.creation_timestamp = 0;
    }

    if (inInputDescName != NULL)
        strcpyUTF8(inputDescName, inInputDescName, sizeof(inputDescName));
    if (inOutputDescName != NULL)
        strcpyUTF8(outputDescName, inOutputDescName, sizeof(outputDescName));

    sqlStmtType = inSqlStmtType;
    useDefaultDesc = inUseDefaultDesc;
    currentMethod = 0;
    rc = ALLOCSQLMXHDLS(this);

#ifndef DISABLE_NOWAIT
    if (rc >= 0)
        rc = initStmtForNowait(&cond, &mutex);
    if (rc == 0)
        stmtInitForNowait = TRUE;
#endif

    CLI_DEBUG_RETURN_SQL(rc);
}

SQLRETURN SRVR_STMT_HDL::allocSqlmxHdls_spjrs(SQLSTMT_ID *callpStmt, const char *inRSStmtName, const char *inModuleName,
                                              long long inModuleTimestamp, long inModuleVersion, short inSqlStmtType,
                                              BOOL inUseDefaultDesc, long inRSindex, const char *RSstmtName)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::allocSqlmxHdls_spjrs",(""));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inRSStmtName=%s",
        DebugString(inRSStmtName)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inModuleName=%s",
        DebugString(inModuleName)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inModuleTimestamp=%s, inModuleVersion=%ld",
        DebugTimestampStr(inModuleTimestamp),
        inModuleVersion));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inSqlStmtType=%s, inUseDefaultDesc=%d",
        CliDebugSqlStatementType(inSqlStmtType),
        inUseDefaultDesc));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inRSindex=%ld, RSstmtName=%s",
        inRSindex,
        DebugString(RSstmtName)));

    SQLRETURN rc = SQL_SUCCESS;

    strcpy(stmtName, inRSStmtName);
    if (inModuleName != NULL)
    {
        moduleId.version = inModuleVersion;
        strcpy(moduleName, inModuleName);
        moduleId.module_name = moduleName;
        moduleId.module_name_len = strlen(moduleName);
        moduleId.charset = "ISO88591";
        moduleId.creation_timestamp = inModuleTimestamp;
    }
    else
    {
        moduleId.version = SQLCLI_ODBC_MODULE_VERSION;
        moduleId.module_name = NULL;
        moduleId.module_name_len = 0;
        moduleId.charset = "ISO88591";
        moduleId.creation_timestamp = 0;
    }
    sqlStmtType = inSqlStmtType;
    useDefaultDesc = inUseDefaultDesc;
    RSIndex = inRSindex;
    isSPJRS = true;

    rc = ALLOCSQLMXHDLS_SPJRS(this, callpStmt, RSstmtName);

#ifndef DISABLE_NOWAIT
    if (rc >= 0)
        rc = initStmtForNowait(&cond, &mutex);
    if (rc == 0)
        stmtInitForNowait = TRUE;
#endif

    CLI_DEBUG_RETURN_SQL(rc);
}

SQLRETURN SRVR_STMT_HDL::ExecuteCall(const SQLValueList_def *inValueList,short inSqlAsyncEnable,
                                     long inQueryTimeout)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::ExecuteCall",("inValueList0x%08x, inSqlAsyncEnable=%d, inQueryTimeout=%ld",
        inValueList,
        inSqlAsyncEnable,
        inQueryTimeout));

    SQLRETURN rc;
    cleanupSQLMessage();
    inputValueList._buffer = inValueList->_buffer;
    inputValueList._length = inValueList->_length;
#ifndef _FASTPATH
    if (outputValueList._buffer == NULL  || maxRowCnt < inMaxRowCnt)
    {
        if ((rc = AllocAssignValueBuffer(&outputDescList, &outputValueList, outputDescVarBufferLen,
            1, outputValueVarBuffer)) != SQL_SUCCESS)
            CLI_DEBUG_RETURN_SQL(rc);
    }
    else
        outputValueList._length = 0;
#else
    outputValueList._buffer = NULL;
    outputValueList._length = 0;
#endif
    CLI_DEBUG_RETURN_SQL(EXECUTECALL(this));
}

SQLRETURN SRVR_STMT_HDL::switchContext(void)
{
    FUNCTION_ENTRY("SQLRETURN SRVR_STMT_HDL::switchContext",(NULL));
    long    sqlcode;
    SRVR_CONNECT_HDL *pConnect;
    SQLRETURN rc = SQL_SUCCESS;;

    if (dialogueId == 0) CLI_DEBUG_RETURN_SQL(SQL_ERROR);
    pConnect = (SRVR_CONNECT_HDL *)dialogueId;
    rc = pConnect->switchContext(&sqlcode);
    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
            pConnect->setCurrentStmt(this);
        break;
    default:
        break;
    }
    CLI_DEBUG_RETURN_SQL(rc);
}

SQLCTX_HANDLE SRVR_STMT_HDL::getContext(void)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::getContext",(NULL));

    if (dialogueId == 0) FUNCTION_RETURN_NUMERIC(0,("Dialog ID is NULL"));
    SRVR_CONNECT_HDL *pConnect = (SRVR_CONNECT_HDL *)dialogueId;
    FUNCTION_RETURN_NUMERIC(pConnect->contextHandle,(NULL));
}

void SRVR_STMT_HDL::resetFetchSize(long fetchSize)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ROWSET,"SRVR_STMT_HDL::resetFetchSize",("fetchSize=%ld",
        fetchSize));

    // See if we want to create a SQL Rowset.  If already a rowset we have to setup
    //  since we cannot go back to non-rowset statement.
    if (fetchRowsetSize ||
        (columnCount && (sqlStmtType & TYPE_SELECT) && (fetchSize>QUAD_THRESHOLD)))
    {
        // If the size is the same as the current fetch size, we are done
        if (fetchQuadEntries && (fetchRowsetSize==fetchSize)) FUNCTION_RETURN_VOID(("Fetch size already set"));
        // We want to create a rowset.  Create the quad structure.
        if (fetchSize>0) fetchRowsetSize = fetchSize;
        else fetchRowsetSize = 1;
        // If columnCount is zero, the descriptor should not be used, so just deallocate.
        // Need to leave fetchRowsetSize set so we know that the statement was a rowset before.
        fetchQuadEntries = columnCount;
        MEMORY_DELETE_ARRAY(fetchQuadField);
        if (fetchQuadEntries)
        {
            MEMORY_ALLOC_ARRAY(fetchQuadField, struct SQLCLI_QUAD_FIELDS,fetchQuadEntries);
            memset(fetchQuadField,0,sizeof(struct SQLCLI_QUAD_FIELDS) * fetchQuadEntries);
        }
        DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_ROWSET,("Rowset allocated. fetchRowsetSize=%ld fetchQuadEntries=%ld",
            fetchRowsetSize,
            fetchQuadEntries));
    }

    // If we can reset the pointers now, do it.  If not, the statement is in the process of
    //  being created and it will call SET_DATA_PTR later.
    if (outputDescVarBufferLen)
    {
        DEBUG_OUT(DEBUG_LEVEL_ROWSET,("Rebuilding data with SET_DATA_PTR"));
        SET_DATA_PTR(this, Output);
    }

    FUNCTION_RETURN_VOID((NULL));
}

void SRVR_STMT_HDL::prepareSetup(void)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ROWSET,"SRVR_STMT_HDL::prepareSetup",(NULL));
    if (batchMaxRowsetSize && (inputDescParamOffset==0))
    {
        // We have not set up input desc with rowsets yet
        if (paramCount==0)
        {
            // If there are not parameters, we will not have any input desc entries
            inputDescParamOffset = 0;
        } else {
            // Set the input desc offset to 1 to skip the rowset size entry
            inputDescParamOffset = 1;
            // Adjust the parameter count.  When set, it was the number of entries.
            paramCount -= inputDescParamOffset;
            DEBUG_OUT(DEBUG_LEVEL_CLI|DEBUG_LEVEL_ROWSET,("paramCount adjusted to %ld",
                paramCount));
        }
    }
    FUNCTION_RETURN_VOID(("inputDescParamOffset set to %ld",inputDescParamOffset));
}

void SRVR_STMT_HDL::batchSetup(long statementCount)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ROWSET,"SRVR_STMT_HDL::batchSetup",("statementCount=%ld",
        statementCount));

    if (batchMaxRowsetSize==0) FUNCTION_RETURN_VOID(("Max size never set.  Cannot use Rowsets."));

    long totalRows;
    // Limit the size to the maximum
    if (statementCount>batchMaxRowsetSize) totalRows = batchMaxRowsetSize;
    else totalRows = statementCount;

    if ((batchRowsetSize!=totalRows) && inputDescVarBufferLen)
    {
        DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_ROWSET,("Batch area being reallocated"));
        MEMORY_DELETE_ARRAY(batchQuadField);
        batchQuadEntries = 0;
        batchRowsetSize = 0;

        // See if we want to create a SQL Rowset
        if (paramCount && totalRows)
        {
            // We want to create a rowset
            batchRowsetSize = totalRows;
            // For output SQL rowsets, first entry used for rowset size.
            batchQuadEntries = paramCount + inputDescParamOffset;
            MEMORY_ALLOC_ARRAY(batchQuadField, struct SQLCLI_QUAD_FIELDS, batchQuadEntries);
            memset(batchQuadField,0,sizeof(struct SQLCLI_QUAD_FIELDS) * batchQuadEntries);
        }
        SET_DATA_PTR(this, Input);
    } else DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_ROWSET,("Batch setup skipped at this time"));

    FUNCTION_RETURN_VOID(("batchRowsetSize=%ld",batchRowsetSize));
}

SQLRETURN SRVR_STMT_HDL::setMaxBatchSize(long maxRowsetSize)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ROWSET,"SRVR_STMT_HDL::setMaxBatchSize",("maxRowsetSize=%ld",
        maxRowsetSize));

    if (maxRowsetSize==batchMaxRowsetSize)
    {
        DEBUG_OUT(DEBUG_LEVEL_ROWSET,("batchMaxRowsetSize(%ld) is already equal to maxRowsetSize",
            batchMaxRowsetSize));
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
    }

    if ((batchMaxRowsetSize!=0) && (maxRowsetSize==0))
    {
        DEBUG_OUT(DEBUG_LEVEL_ROWSET,("batchMaxRowsetSize(%ld) cannot be set back to zero",
            batchMaxRowsetSize));
        CLI_DEBUG_RETURN_SQL(SQL_ERROR);
    }

    if (sqlStmtType&TYPE_CALL)
    {
        DEBUG_ASSERT(batchMaxRowsetSize==0,("batchMaxRowsetSize is set for a Callable statement"));
        DEBUG_OUT(DEBUG_LEVEL_ROWSET,("Rowsets skipped for Callable Statement"));
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
    }

    DEBUG_OUT(DEBUG_LEVEL_ROWSET,("Setting max input array size to %ld",
        maxRowsetSize));
    SQLRETURN retcode = CLI_SetStmtAttr(&stmt,
        SQL_ATTR_INPUT_ARRAY_MAXSIZE,
        maxRowsetSize,
        NULL);
    if (retcode==SQL_SUCCESS)
    {
        batchMaxRowsetSize = maxRowsetSize;
    }
    CLI_DEBUG_RETURN_SQL(retcode);
}

void SRVR_STMT_HDL::resetFetchSize(long dialogueId, long stmtId, long fetchSize)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ROWSET,"SRVR_STMT_HDL::resetFetchSize",("dialogueId=0x%08x, stmtId=0x%08x, fetchSize=%ld",
        dialogueId,
        stmtId,
        fetchSize));

    SRVR_STMT_HDL *pSrvrStmt;
    long        sqlcode;

    if ((pSrvrStmt = getSrvrStmt(dialogueId, stmtId, &sqlcode)) == NULL)
        FUNCTION_RETURN_VOID(("No Statement found"));

    pSrvrStmt->resetFetchSize(fetchSize);

    FUNCTION_RETURN_VOID((NULL));
}

SRVR_DESC_HDL *SRVR_STMT_HDL::allocImplDesc(DESC_TYPE descType)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ROWSET,"SRVR_STMT_HDL::allocImplDesc",("descType=%s",
        CliDebugDescTypeStr(descType)));
    switch (descType)
    {
    case Input:
        MEMORY_DELETE_ARRAY(IPD);
        if (paramCount > 0) MEMORY_ALLOC_ARRAY(IPD,SRVR_DESC_HDL,paramCount+inputDescParamOffset);
        FUNCTION_RETURN_PTR(IPD,("Input IPD"));
    case Output:
        MEMORY_DELETE_ARRAY(IRD);
        if (columnCount > 0) MEMORY_ALLOC_ARRAY(IRD,SRVR_DESC_HDL,columnCount);
        FUNCTION_RETURN_PTR(IRD,("Output IRD"));
    }
    FUNCTION_RETURN_PTR(NULL,("Unknown"));
}

SRVR_DESC_HDL *SRVR_STMT_HDL::getImplDesc(DESC_TYPE descType)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ROWSET,"SRVR_STMT_HDL::getImplDesc",("descType=%s",
        CliDebugDescTypeStr(descType)));
    switch (descType)
    {
    case Input:
        FUNCTION_RETURN_PTR(IPD,("Input IPD"));
    case Output:
        FUNCTION_RETURN_PTR(IRD,("Output IRD"));
    }
    FUNCTION_RETURN_PTR(NULL,("Unknown"));
}

long *SRVR_STMT_HDL::getDescBufferLenPtr(DESC_TYPE descType)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ROWSET,"SRVR_STMT_HDL::getDescBufferLenPtr",("descType=%s",
        CliDebugDescTypeStr(descType)));
    switch (descType)
    {
        case Input:
            FUNCTION_RETURN_PTR((long *)&inputDescVarBufferLen,
                    ("inputDescVarBufferLen=%ld", inputDescVarBufferLen));
        case Output:
            FUNCTION_RETURN_PTR((long *)&outputDescVarBufferLen,
                    ("outputDescVarBufferLen=%ld", outputDescVarBufferLen));
    }
    FUNCTION_RETURN_PTR(NULL,("Unknown"));
}

long SRVR_STMT_HDL::getDescEntryCount(DESC_TYPE descType)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ROWSET,"SRVR_STMT_HDL::getDescEntryCount",("descType=%s",
        CliDebugDescTypeStr(descType)));
    switch (descType)
    {
    case Input:
        FUNCTION_RETURN_NUMERIC(paramCount+inputDescParamOffset,
            ("paramCount+%ld",inputDescParamOffset));
    case Output:
        FUNCTION_RETURN_NUMERIC(columnCount,("columnCount"));
    }
    FUNCTION_RETURN_NUMERIC(-1,("Unknown"));
}

long SRVR_STMT_HDL::getQuadEntryCount(DESC_TYPE descType)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ROWSET,"SRVR_STMT_HDL::getQuadEntryCount",("descType=%s",
        CliDebugDescTypeStr(descType)));
    switch (descType)
    {
    case Input:
        FUNCTION_RETURN_NUMERIC(batchQuadEntries,("batchQuadEntries"));
    case Output:
        FUNCTION_RETURN_NUMERIC(fetchQuadEntries,("fetchQuadEntries"));
    }
    FUNCTION_RETURN_NUMERIC(-1,("Unknown"));
}

SQLDESC_ID *SRVR_STMT_HDL::getDesc(DESC_TYPE descType)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ROWSET,"SRVR_STMT_HDL::getDesc",("descType=%s",
        CliDebugDescTypeStr(descType)));
    switch (descType)
    {
    case Input:
        FUNCTION_RETURN_PTR(&inputDesc,("inputDesc"));
    case Output:
        FUNCTION_RETURN_PTR(&outputDesc,("outputDesc"));
    }
    FUNCTION_RETURN_PTR(NULL,("Unknown"));
}

SQLItemDescList_def *SRVR_STMT_HDL::getDescList(DESC_TYPE descType)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ROWSET,"SRVR_STMT_HDL::getDescList",("descType=%s",
        CliDebugDescTypeStr(descType)));
    switch (descType)
    {
    case Input:
        FUNCTION_RETURN_PTR(&inputDescList,("inputDescList"));
    case Output:
        FUNCTION_RETURN_PTR(&outputDescList,("outputDescList"));
    }
    FUNCTION_RETURN_PTR(NULL,("Unknown"));
}

BYTE **SRVR_STMT_HDL::getDescVarBufferPtr(DESC_TYPE descType)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ROWSET,"SRVR_STMT_HDL::getDescVarBufferPtr",("descType=%s",
        CliDebugDescTypeStr(descType)));
    switch (descType)
    {
    case Input:
        FUNCTION_RETURN_PTR(&inputDescVarBuffer,("inputDescVarBuffer"));
    case Output:
        FUNCTION_RETURN_PTR(&outputDescVarBuffer,("outputDescVarBuffer"));
    }
    FUNCTION_RETURN_PTR(NULL,("Unknown"));
}

long SRVR_STMT_HDL::getRowsetSize(DESC_TYPE descType)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ROWSET,"SRVR_STMT_HDL::getRowsetSize",("descType=%s",
        CliDebugDescTypeStr(descType)));
    switch (descType)
    {
    case Input:
        FUNCTION_RETURN_NUMERIC(batchRowsetSize,("batchRowsetSize"));
    case Output:
        FUNCTION_RETURN_NUMERIC(fetchRowsetSize,("fetchRowsetSize"));
    }
    FUNCTION_RETURN_NUMERIC(-1,("Unknown"));
}

struct SQLCLI_QUAD_FIELDS *SRVR_STMT_HDL::getQuadField(DESC_TYPE descType)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ROWSET,"SRVR_STMT_HDL::getQuadField",("descType=%s",
        CliDebugDescTypeStr(descType)));
    switch (descType)
    {
    case Input:
        FUNCTION_RETURN_PTR(batchQuadField,("batchQuadField"));
    case Output:
        FUNCTION_RETURN_PTR(fetchQuadField,("fetchQuadField"));
    }
    FUNCTION_RETURN_PTR(NULL,("Unknown"));
}
//MFC
// MFC
/*
SQLRETURN SRVR_STMT_HDL::PrepareforMFC(const SQLValue_def *inSqlString, short inStmtType, short inHoldability,
                                       long inQueryTimeout,bool isISUD)
{
    FUNCTION_ENTRY("SRVR_STMT_HDL::PrepareforMFC",(""));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inSqlString='%s'",
        CLI_SQL_VALUE_STR(inSqlString)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inStmtType=%s, inHoldability=%d, inQueryTimeout=%ld, isISUD=%d",
        CliDebugStatementType(inStmtType),
        inHoldability,
        inQueryTimeout,isISUD));

    SQLRETURN rc;
    size_t  len;
    this->isISUD = isISUD;
    if (srvrGlobal->moduleCaching)
    {
        if (!this->isClosed)
        {
            long retcode = SQL_SUCCESS;
            SQLSTMT_ID  *pStmt = &(this->stmt);
            retcode = CLI_CloseStmt(pStmt);

            if (retcode!=0)
            {
                retcode = CLI_ClearDiagnostics(pStmt);
            }
            this->isClosed = TRUE;
        }
    }
    if (isReadFromModule)   // Already SMD label is found
    {
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
    }
    // cleanup all memory allocated in the previous operations
    cleanupAll();
    sqlString.dataCharset = inSqlString->dataCharset;
    sqlString.dataType = inSqlString->dataType;
    MEMORY_ALLOC_ARRAY(sqlString.dataValue._buffer,unsigned char,inSqlString->dataValue._length+1);
    sqlString.dataValue._length = inSqlString->dataValue._length+1;

    strncpy((char *)sqlString.dataValue._buffer, (const char *)inSqlString->dataValue._buffer, inSqlString->dataValue._length);
    sqlString.dataValue._buffer[inSqlString->dataValue._length] = '\0';
    stmtType = inStmtType;
    holdability = inHoldability;

    CLI_DEBUG_RETURN_SQL(PREPAREFORMFC(this));
}
*/

char* SRVR_STMT_HDL::w_allocate(size_t size)
{
    if (m_wbuffer != NULL && size <= m_wbuffer_length)
    {
        return m_wbuffer;
    }

    MEMORY_DELETE_ARRAY(m_wbuffer);
    
    MEMORY_ALLOC_ARRAY(m_wbuffer, char, size);
    if (m_wbuffer != NULL)
        m_wbuffer_length = size;
    else
        m_wbuffer_length = 0;

    return m_wbuffer;
}

char* SRVR_STMT_HDL::r_allocate(size_t size)
{
    if (m_rbuffer != NULL && size <= m_rbuffer_length)
    {
        return m_rbuffer;
    }

    MEMORY_DELETE_ARRAY(m_rbuffer);
    
    MEMORY_ALLOC_ARRAY(m_rbuffer, char, size);
    if (m_rbuffer != NULL)
        m_rbuffer_length = size;
    else
        m_rbuffer_length = 0;

    return m_rbuffer;
}

char* SRVR_STMT_HDL::w_assign(char* buffer, int length)
{
    MEMORY_DELETE_ARRAY(m_wbuffer);
    
    m_wbuffer = buffer;
    m_wbuffer_length = length;
}

char* SRVR_STMT_HDL::r_assign(char* buffer, int length)
{
    MEMORY_DELETE_ARRAY(m_rbuffer);
    
    m_rbuffer = buffer;
    m_rbuffer_length = length;
}

void SRVR_STMT_HDL::w_release()
{
    MEMORY_DELETE_ARRAY(m_wbuffer);
    
    m_wbuffer_length = 0;
}

void SRVR_STMT_HDL::r_release()
{
    MEMORY_DELETE_ARRAY(m_rbuffer);
    
    m_rbuffer_length = 0;
}

char* SRVR_STMT_HDL::w_buffer()
{
    return m_wbuffer;
}

char* SRVR_STMT_HDL::r_buffer()
{
    return m_rbuffer;
}

int SRVR_STMT_HDL::w_buffer_length()
{
    return m_wbuffer_length;
}

int SRVR_STMT_HDL::r_buffer_length()
{
    return m_rbuffer_length;
}

