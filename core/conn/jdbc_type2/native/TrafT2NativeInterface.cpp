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
//
// MODULE: TrafT2InterfaceOdbc.cpp
//
// Purpose: Implements JNI layer functions for transition of prepare/execute/fetch
// buffers between JAVA layer and native layer.
//
#include "org_trafodion_jdbc_t2_SQLMXPreparedStatement.h"
#include "org_trafodion_jdbc_t2_TrafT2NativeInterface.h"
#include "SQLMXCommonFunctions.h"
#include "TrafT2SrvrConnect.h"
#include "JdbcDriverGlobal.h"
#include "CSrvrConnect.h"
#include "CoreCommon.h"
#include "CSrvrStmt.h"
#include "Debug.h"

void DISPATCH_SQLRequest(short operation_id, Long &connHandle, Long &stmtId, const char *inBuffer);
void SQLPREPARE_IOMessage(short operation_id, Long &connHandle, Long &stmtId, const char *inBuffer);
void SQLEXECUTE_IOMessage(short operation_id, Long &connHandle, Long &stmtId, const char *inBuffer);
void SQLFETCH_IOMessage(short operation_id, Long &connHandle, Long &stmtId, const char *inBuffer);

JNIEXPORT jbyteArray JNICALL Java_org_trafodion_jdbc_t2_TrafT2NativeInterface_processRequest
(JNIEnv *jenv, jclass jobj, jshort executeAPI, jlong dialogueId, jlong stmtId, jbyteArray jInValueArray)
{
    short      operation_id      = executeAPI;
    jbyteArray jOutValueArray    = NULL;
    jboolean   isCopy            = JNI_FALSE;
    char       *outBuffer        = NULL;
    const char *inBuffer         = NULL;
    int        outBufferLen      = 0;
    int        retCode           = SQL_SUCCESS;
    Long       stmtHandle        = stmtId;
    Long       connHandle        = dialogueId;
    
    SRVR_CONNECT_HDL  *pConnect  = NULL;
    SRVR_STMT_HDL     *pSrvrStmt = NULL;

    if(operation_id != SRVR_API_SQLPREPARE
            && operation_id != SRVR_API_SQLCONNECT)
    {
        if(stmtHandle == 0)
        {
            throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000");
            FUNCTION_RETURN_PTR(NULL, "Invalid Statement Handle.");
        }
        pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;

        if(pSrvrStmt->dialogueId == 0 || pSrvrStmt->dialogueId != connHandle)
        {
            throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000");
            FUNCTION_RETURN_PTR(NULL, "Invalid Connection Handle.");
        }

        pConnect = (SRVR_CONNECT_HDL *)pSrvrStmt->dialogueId;
    }

    if(jInValueArray != NULL)
    {
        if((inBuffer = (const char*)JNI_GetByteArrayElements(jenv, jInValueArray, &isCopy)) == NULL)
        {
            throwSQLException(jenv, INVALID_DATA_BUFFER_ERROR, NULL, "HY000");
            FUNCTION_RETURN_PTR(NULL, "Empty Buffer of Byte Brray");
        }
    }

    DISPATCH_SQLRequest(operation_id, connHandle, stmtHandle, inBuffer);

    if(operation_id == SRVR_API_SQLPREPARE)
    {
        if(stmtHandle == 0)
        {
            throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000");
            FUNCTION_RETURN_PTR(NULL, "Invalid Statement Handle.");
        }
        pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;
    }

    outBuffer    = pSrvrStmt->w_buffer();
    outBufferLen = pSrvrStmt->w_buffer_length();

    if((jOutValueArray = JNI_NewByteArray(jenv, outBufferLen)) == NULL)
    {
        FUNCTION_RETURN_PTR(NULL, "JNI_NewByteArray() for output to Java failed.");
    }

    JNI_SetByteArrayRegion(jenv, jOutValueArray, 0, outBufferLen, (jbyte *)outBuffer);

    FUNCTION_RETURN_PTR(jOutValueArray, NULL);
}

void DISPATCH_SQLRequest(
        /* In */    short operation_id
        , /* In */    Long   &connHandle
        , /* In */    Long   &stmtId
        , /* In */    const char *inBuffer
        )
{
    switch(operation_id)
    {
        case SRVR_API_SQLCONNECT:
            //SQLCONNECT_IOMessage(objtag_, call_id_);
            break;
        case SRVR_API_SQLDISCONNECT:
            //SQLDISCONNECT_IOMessage(objtag_, call_id_);
            break;
        case SRVR_API_SQLSETCONNECTATTR:
            //SQLSETCONNECTATTR_IOMessage(objtag_, call_id_);
            break;
        case SRVR_API_SQLENDTRAN:
            //SQLENDTRAN_IOMessage(objtag_, call_id_);
            break;
        case SRVR_API_SQLPREPARE:
            SQLPREPARE_IOMessage(operation_id, connHandle, stmtId, inBuffer);
            break;
        case SRVR_API_SQLEXECDIRECT_ROWSET:
        case SRVR_API_SQLEXECDIRECT:
        case SRVR_API_SQLEXECUTE2:
        case SRVR_API_SQLEXECUTECALL:
            SQLEXECUTE_IOMessage(operation_id, connHandle, stmtId, inBuffer);
            break;
        case SRVR_API_SQLFREESTMT:
            //SQLFREESTMT_IOMessage(objtag_, call_id_);
            break;
        case SRVR_API_GETCATALOGS:
            //SQLGETCATALOGS_IOMessage(objtag_, call_id_);
            break;
        case SRVR_API_SQLFETCH:
        case SRVR_API_SQLFETCH_ROWSET:
            SQLFETCH_IOMessage(operation_id, connHandle, stmtId, inBuffer);
            break;
        case SRVR_API_STOPSRVR:
            //STOPSRVR_IOMessage(objtag_, call_id_);
            break;
        case SRVR_API_ENABLETRACE:
            //ENABLETRACE_IOMessage(objtag_, call_id_);
            break;
        case SRVR_API_DISABLETRACE:
            //DISABLETRACE_IOMessage(objtag_, call_id_);
            break;
        case SRVR_API_ENABLE_SERVER_STATISTICS:
            //ENABLESTATISTICS_IOMessage(objtag_, call_id_);
            break;
        case SRVR_API_DISABLE_SERVER_STATISTICS:
            //DISABLESTATISTICS_IOMessage(objtag_, call_id_);
            break;
        case SRVR_API_UPDATE_SERVER_CONTEXT:
            //UPDATECONTEXT_IOMessage(objtag_, call_id_);
            break;
        default:
            break;
    }
} /* end of DISPATCH_SQLRequest */

void
SQLPREPARE_IOMessage(
        /* In */    short operation_id
        , /* In */    Long   &connHandle
        , /* In */    Long   &stmtId
        , /* In */    const char *inBuffer
        )
{
    IDL_char   *curptr = NULL;

    Long          dialogueId = 0;
    IDL_long      sqlAsyncEnable = 0;
    IDL_long      queryTimeout = 0;
    IDL_short     stmtType = 0;
    IDL_long      sqlStmtType = 0;
    IDL_long      stmtLength = 0;
    IDL_char      *stmtLabel = NULL;
    IDL_long      stmtLabelCharset = 0;
    IDL_long      cursorLength = 0;
    IDL_string    cursorName = NULL;
    IDL_long      cursorCharset = 0;
    IDL_long      moduleNameLength = 0;
    IDL_char      *moduleName = NULL;
    IDL_long      moduleCharset = 0;
    IDL_long_long moduleTimestamp = 0;
    IDL_long      sqlStringLength = 0;
    IDL_string    sqlString = NULL;
    IDL_long      sqlStringCharset = 0;
    IDL_long      setStmtOptionsLength = 0;
    IDL_string    setStmtOptions = NULL;
    IDL_long      stmtExplainLabelLength = 0;
    IDL_string    stmtExplainLabel = NULL;
    IDL_long      maxRowsetSize = 0;
    IDL_long	  transactionIDLength = 0; // JDBC is the only one that will use this to join a transaction
    IDL_long_long transactionID = 0;     // JDBC is the only one that will use this to join a transaction
    IDL_long      holdableCursor = SQL_NONHOLDABLE; // default
    IDL_short	  *extTransId = NULL;
    IDL_short	ix;
    IDL_char	*temp = NULL;
    bool		all_zero=true;

    IDL_long inputPosition = 0;

    curptr = (char*)inBuffer;

    dialogueId = connHandle; // Do not use the value passed within the protocol byte buffer, it's byte order swapped.
    inputPosition += sizeof(dialogueId);

    holdableCursor	= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(sqlAsyncEnable);

    queryTimeout	= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(queryTimeout);

    stmtType		= *(IDL_short*)(curptr+inputPosition);
    inputPosition += sizeof(stmtType);

    sqlStmtType		= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(sqlStmtType);

    stmtLength		= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(stmtLength);
    if (stmtLength > 0)
    {
        stmtLabel = curptr+inputPosition;
        inputPosition += stmtLength;
        stmtLabelCharset = *(IDL_long*)(curptr+inputPosition);
        inputPosition += sizeof(stmtLabelCharset);
    }

    cursorLength	= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(cursorLength);
    if (cursorLength > 0)
    {
        cursorName = curptr+inputPosition;
        inputPosition += cursorLength;
        cursorCharset = *(IDL_long*)(curptr+inputPosition);
        inputPosition += sizeof(cursorCharset);
    }

    moduleNameLength	= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(moduleNameLength);
    if (moduleNameLength > 0)
    {
        moduleName = curptr+inputPosition;
        inputPosition += moduleNameLength;
        moduleCharset = *(IDL_long*)(curptr+inputPosition);
        inputPosition += sizeof(moduleCharset);
        moduleTimestamp = *(IDL_long_long*)(curptr+inputPosition);
        inputPosition += sizeof(moduleTimestamp);
    }

    sqlStringLength	= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(sqlStringLength);
    if (sqlStringLength > 0)
    {
        sqlString = curptr+inputPosition;
        inputPosition += sqlStringLength;
        sqlStringCharset	= *(IDL_long*)(curptr+inputPosition);
        inputPosition += sizeof(sqlStringCharset);
    }

    setStmtOptionsLength	= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(setStmtOptionsLength);
    if (setStmtOptionsLength > 0)
    {
        setStmtOptions = curptr+inputPosition;
        inputPosition += setStmtOptionsLength;
    }

    stmtExplainLabelLength	= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(stmtExplainLabelLength);
    if (stmtExplainLabelLength > 0)
    {
        stmtExplainLabel = curptr+inputPosition;
        inputPosition += stmtExplainLabelLength;
    }

    maxRowsetSize  = *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(maxRowsetSize);

    transactionIDLength = *(IDL_unsigned_long*)(curptr+inputPosition);
    inputPosition += sizeof(transactionIDLength);

    if(transactionIDLength > 0)
    {
        //LCOV_EXCL_START
        if (transactionIDLength == 17){
            extTransId = (IDL_short*)(curptr+inputPosition);
            temp = (IDL_char*)(curptr+inputPosition);
            inputPosition += transactionIDLength;
            // need to check the extTransId, if it contains all 0 (17 bytes), change the address to NULL
            // this will prevent TMF_JOIN_EXT_ to be called at the odbc_SQLSrvr_..._ame_
            for (ix=0; ix<transactionIDLength; ix++)
            {
                if (*temp != 0) {
                    all_zero=false;
                    break;
                }
                temp++;
            }
            if (all_zero)
                extTransId = NULL;
        }
        else{
            if(transactionIDLength == 5)
                transactionID = *(IDL_long*)(curptr+inputPosition);
            if(transactionIDLength == 9)
                transactionID = *(IDL_long_long*)(curptr+inputPosition);
            inputPosition += transactionIDLength;
        }
        //LCOV_EXCL_STOP
    }


    odbc_SQLSrvr_Prepare_ame_(
            dialogueId
            , stmtId
            , sqlAsyncEnable
            , queryTimeout
            , stmtType
            , sqlStmtType
            , stmtLength
            , stmtLabel
            , stmtLabelCharset
            , cursorLength
            , cursorName
            , cursorCharset
            , moduleNameLength
            , moduleName
            , moduleCharset
            , moduleTimestamp
            , sqlStringLength
            , sqlString
            , sqlStringCharset
            , setStmtOptionsLength
            , setStmtOptions
            , stmtExplainLabelLength
            , stmtExplainLabel
            , maxRowsetSize
            , transactionID
            , extTransId
            , holdableCursor
            );
} /* SQLPREPARE_IOMessage() */

void SQLEXECUTE_IOMessage(
        /* In */    short operation_id
        , /* In */    Long   &connHandle
        , /* In */    Long   &stmtId
        , /* In */    const char *inBuffer
        )
{
    CEE_status	sts = CEE_SUCCESS;
    CEE_status	retcode;
    IDL_char	*curptr;
    IDL_unsigned_long i;

    Long       dialogueId = 0;
    IDL_long   sqlAsyncEnable = 0;
    IDL_long   queryTimeout = 0;
    IDL_long   inputRowCnt = 0;
    IDL_long   maxRowsetSize = 0;
    IDL_long   sqlStmtType = 0;
    Long       stmtHandle = 0;
    IDL_long   stmtHandleKey = 0;
    IDL_long   stmtType = 0;
    IDL_long   sqlStringLength = 0;
    IDL_string sqlString = NULL;
    IDL_long   sqlStringCharset = 0;
    IDL_long   cursorLength = 0;
    IDL_string cursorName = NULL;
    IDL_long   cursorCharset = 0;
    IDL_long   stmtLength = 0;
    IDL_char  *stmtLabel = NULL;
    IDL_long   stmtLabelCharset = 0;
    IDL_long   stmtExplainLabelLength = 0;
    IDL_string stmtExplainLabel = NULL;
    IDL_long   inValuesLength = 0;
    BYTE      *inValues = NULL;
    IDL_long	transactionIDLength = 0; // JDBC is the only one that will use this to join a transaction
    IDL_long_long transactionID = 0;     // JDBC is the only one that will use this to join a transaction


    IDL_long  holdableCursor = SQL_NONHOLDABLE; //default
    IDL_long inputPosition = 0;
    IDL_short ix;

    curptr = (char*)inBuffer;

    dialogueId = connHandle; // Do not use the value passed within the protocol byte buffer, it's byte order swapped.
    inputPosition += sizeof(dialogueId);

    holdableCursor	= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(sqlAsyncEnable);

    queryTimeout	= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(queryTimeout);

    inputRowCnt		= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(inputRowCnt);

    maxRowsetSize	= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(maxRowsetSize);

    sqlStmtType		= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(sqlStmtType);

    stmtHandle = stmtId;
    stmtHandleKey	 = *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(IDL_long);

    stmtType = *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(stmtType);

    sqlStringLength	= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(sqlStringLength);
    if (sqlStringLength > 0)
    {
        sqlString = curptr+inputPosition;
        inputPosition += sqlStringLength;
        sqlStringCharset	= *(IDL_long*)(curptr+inputPosition);
        inputPosition += sizeof(sqlStringCharset);
    }

    cursorLength	= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(cursorLength);
    if (cursorLength > 0)
    {
        cursorName = curptr+inputPosition;
        inputPosition += cursorLength;
        cursorCharset = *(IDL_long*)(curptr+inputPosition);
        inputPosition += sizeof(cursorCharset);
    }

    stmtLength		= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(stmtLength);
    if (stmtLength > 0)
    {
        stmtLabel = curptr+inputPosition;
        inputPosition += stmtLength;
        stmtLabelCharset = *(IDL_long*)(curptr+inputPosition);
        inputPosition += sizeof(stmtLabelCharset);
    }

    stmtExplainLabelLength	= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(stmtExplainLabelLength);
    if (stmtExplainLabelLength > 0)
    {
        stmtExplainLabel = curptr+inputPosition;
        inputPosition += stmtExplainLabelLength;
    }

    inValuesLength	= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(inValuesLength);
    if (inValuesLength > 0)
    {
        inValues = (BYTE *) curptr+inputPosition;
        inputPosition += inValuesLength;
    }

    transactionIDLength = *(IDL_unsigned_long*)(curptr+inputPosition);
    inputPosition += sizeof(transactionIDLength);

    if(transactionIDLength > 0)
    {
        //LCOV_EXCL_START
        //LCOV_EXCL_STOP
        if(transactionIDLength == 5)
            transactionID = *(IDL_long*)(curptr+inputPosition);
        if(transactionIDLength == 9)
            transactionID = *(IDL_long_long*)(curptr+inputPosition);

        inputPosition += transactionIDLength;
    }

    if( operation_id == SRVR_API_SQLEXECUTE2)
    {
        odbc_SQLSrvr_Execute2_ame_(
                dialogueId,
                sqlAsyncEnable,
                queryTimeout,
                inputRowCnt,
                sqlStmtType,
                stmtHandle,
                cursorName,
                cursorCharset,
                inValuesLength,
                inValues,
                0,        // Sql Query Type (used for execdirect calls)
                0,        // output Descriptor Length (used for execdirect calls)
                NULL,     // output Descriptor (used for execdirect calls)
                maxRowsetSize, //For DBT to obtain the Rowlength from Driver
                transactionID, // JDBC sends this to join an existing transaction for SPJ calls
                holdableCursor
                );
    } /* if operation_id == SRVR_API_SQLEXECUTE2 */
    else if( operation_id == SRVR_API_SQLEXECDIRECT)
    {

        odbc_SQLSrvr_ExecDirect_ame_(
                dialogueId,
                stmtLabel,
                cursorName,
                stmtExplainLabel,
                stmtType,
                sqlStmtType,
                sqlString,
                sqlAsyncEnable,
                queryTimeout,
                inputRowCnt,
                transactionID, // JDBC sends this to join an existing transaction for SPJ calls
                holdableCursor
                );
    } /* if operation_id == SRVR_API_SQLEXECDIRECT */

} /* end of SQLEXECUTE_Message */

void
SQLFETCH_IOMessage(
        /* In */    short operation_id
        , /* In */    Long   &connHandle
        , /* In */    Long   &stmtId
        , /* In */    const char *inBuffer
        )
{
    IDL_char		*curptr;

    Long            dialogueId           = 0;
    IDL_long        sqlAsyncEnable       = 0;
    IDL_long        queryTimeout         = 0;
    Long 			stmtHandle           = 0;
    IDL_long 		stmtHandleKey           = 0;
    IDL_long        stmtLength           = 0;
    IDL_string      stmtLabel            = NULL;
    IDL_long        stmtLabelCharset     = 0;
    IDL_unsigned_long_long   maxRowCnt            = 0;
    IDL_unsigned_long_long   maxRowLen            = 0;
    IDL_long        cursorLength         = 0;
    IDL_string      cursorName           = NULL;
    IDL_long        cursorCharset        = 0;
    IDL_long        setStmtOptionsLength = 0;
    IDL_string      setStmtOptions       = NULL;


    IDL_long inputPosition = 0;

    curptr = (IDL_char *)inBuffer;

    dialogueId     = connHandle; // Do not use the value passed within the protocol byte buffer, it's byte order swapped.
    inputPosition += sizeof(dialogueId);

    sqlAsyncEnable = *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(sqlAsyncEnable);

    queryTimeout	 = *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(queryTimeout);

    stmtHandleKey	 = *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(IDL_long);

    stmtHandle = stmtId;

    stmtLength	 = *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(stmtLength);
    if (stmtLength > 0)
    {
        stmtLabel     = curptr+inputPosition;
        inputPosition += stmtLength;
        stmtLabelCharset  = *(IDL_long*)(curptr+inputPosition);
        inputPosition += sizeof(stmtLabelCharset);
    }

    maxRowCnt	 = *(IDL_unsigned_long_long*)(curptr+inputPosition);
    inputPosition += sizeof(maxRowCnt);

    maxRowLen	 = *(IDL_unsigned_long_long*)(curptr+inputPosition);
    inputPosition += sizeof(maxRowLen);

    /* Unused for now */
    cursorLength	 = *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(cursorLength);
    if (cursorLength > 0)
    {
        cursorName     = curptr+inputPosition;
        inputPosition += cursorLength;
        cursorCharset  = *(IDL_long*)(curptr+inputPosition);
        inputPosition += sizeof(cursorCharset);
    }

    /* Unused for now */
    setStmtOptionsLength	= *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(setStmtOptionsLength);
    if (setStmtOptionsLength > 0)
    {
        setStmtOptions = curptr+inputPosition;
        inputPosition += setStmtOptionsLength;
    }

    odbc_SQLSrvr_Fetch_ame_(
            dialogueId
            , sqlAsyncEnable
            , queryTimeout
            , stmtHandle
            , stmtLabel
            , maxRowCnt
            , maxRowLen);

} /* SQLFETCH_IOMessage() */


