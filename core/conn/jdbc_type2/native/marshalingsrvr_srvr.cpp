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

//#include "ceercv.h"

#include <platform_ndcs.h>
#include "marshaling.h"
#include "CSrvrStmt.h"
#include "Debug.h"

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
        )
{
    IDL_char  *buffer;
    IDL_unsigned_long message_length;

    IDL_char* curptr;
    IDL_long wlength = 0;

    Long      stmtHandle = (Long)pnode;
    //
    // calculate length of the buffer for each parameter
    //
    // length of IDL_long returnCode
    //
    wlength += sizeof(returnCode);
    //
    // length of IDL_long sqlWarningOrErrorLength
    // length of BYTE *sqlWarningOrError
    //
    if ((sqlWarningOrError != NULL) && (sqlWarningOrErrorLength != 0))
    {
        wlength += sizeof(sqlWarningOrErrorLength);
        wlength += sqlWarningOrErrorLength;
    }
    //
    // length of IDL_long sqlQueryType
    //
    wlength += sizeof(sqlQueryType);
    //
    // length of IDL_long stmtHandle
    //
    wlength += sizeof(stmtHandle);
    //
    // length of IDL_long estimatedCost
    //
    wlength += sizeof(estimatedCost);
    //
    // length of IDL_long inputDescLength
    // length of BYTE *inputDesc
    //
    if (inputDesc != NULL)
    {
        wlength += sizeof(inputDescLength);
        wlength += inputDescLength;
    }
    else
        wlength += sizeof(inputDescLength);
    //
    // length of IDL_long outputDescLength
    // length of BYTE *outputDesc
    //
    if (outputDesc != NULL)
    {
        wlength += sizeof(outputDescLength);
        wlength += outputDescLength;
    }
    else
        wlength += sizeof(outputDescLength);

    //
    // message_length = header + param + maplength + data length
    //
    message_length = wlength;
    buffer = pnode->w_allocate(message_length);
    if (buffer == NULL)
    {
        return CEE_ALLOCFAIL;
    }

    curptr = (IDL_char*)buffer;  // NOT network protocol, no HEADER
    //
    // copy of IDL_long returnCode
    //
    IDL_long_copy(&returnCode, curptr);
    //
    // copy IDL_long sqlWarningOrErrorLength
    // copy BYTE *sqlWarningOrError
    //
    if ((sqlWarningOrError != NULL) && (sqlWarningOrErrorLength != 0))
    {
        IDL_long_copy(&sqlWarningOrErrorLength, curptr);
        IDL_byteArray_copy(sqlWarningOrError, sqlWarningOrErrorLength, curptr);
    }
    //
    // copy of IDL_long sqlQueryType
    //
    IDL_long_copy(&sqlQueryType, curptr);
    //
    // copy of IDL_long stmtHandle
    //
    Long_copy(&stmtHandle, curptr);
    //
    // copy of IDL_long estimatedCost
    //
    IDL_long_copy(&estimatedCost, curptr);
    //
    // copy IDL_long inputDescLength
    // copy BYTE *inputDesc
    //
    if (inputDesc != NULL)
    {
        IDL_long_copy(&inputDescLength, curptr);
        IDL_byteArray_copy(inputDesc, inputDescLength, curptr);
    }
    else
    {
        IDL_long_copy(&inputDescLength, curptr);
    }
    //
    // copy IDL_long outputDescLength
    // copy BYTE *outputDesc
    //
    if (outputDesc != NULL)
    {
        IDL_long_copy(&outputDescLength, curptr);
        IDL_byteArray_copy(outputDesc, outputDescLength, curptr);
    }
    else
    {
        IDL_long_copy(&outputDescLength, curptr);
    }

    if (curptr > buffer + message_length)
    {
        //LCOV_EXCL_START
        exit(1000);
        //LCOV_EXCL_STOP
    }

    SRVRTRACE_EXIT(FILE_OMR+15);
    return CEE_SUCCESS;
} /* odbc_SQLSrvr_Prepare_param_res_() */

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
        )
{
    IDL_char  *curptr;
    IDL_long   wlength;
    IDL_char  *buffer;
    IDL_unsigned_long message_length;

    SRVR_STMT_HDL *pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;
    SRVR_STMT_HDL *rsSrvrStmt;  // To iterate thru the result set
    Long      rsStmtHandle; // result set statement handle
    IDL_long      rsStmtLabelLength;
    IDL_char      *rsStmtName;
    IDL_long      rsOutputDescBufferLength;
    BYTE			*rsOutputDescBuffer;
    IDL_long      charSet = 1;  // KAS - SQLCHARSETCODE_ISO88591 - change this when supporting character sets
    IDL_long      numResultSets  = 0;
    IDL_long		proxySyntaxStringLen = 0;

    if(pSrvrStmt != NULL)
        numResultSets = pSrvrStmt->numResultSets; // SPJ result sets

    wlength = 0;//sizeof(HEADER);

    //
    // calculate length of the buffer for each parameter
    //

    // length of IDL_long returnCode
    //
    wlength += sizeof(returnCode);

    //
    // length of IDL_long sqlWarningOrErrorLength
    // length of BYTE *sqlWarningOrError
    //
    wlength += sizeof (sqlWarningOrErrorLength);
    if (sqlWarningOrError != NULL)
    {
        wlength += sqlWarningOrErrorLength;
    }


    //
    // length of IDL_long outputDescLength
    // length of BYTE* outputDesc
    //
    wlength += sizeof (outputDescLength);
    if(outputDescLength > 0)
        wlength += outputDescLength;

    //
    // length of IDL_long rowsAffected
    //
    wlength += sizeof(rowsAffected);

    //
    // length of IDL_long sqlQueryType
    //
    wlength += sizeof(sqlQueryType);

    //
    // length of IDL_long estimatedCost
    //
    wlength += sizeof(estimatedCost);

    //
    // length of IDL_long outValuesLength
    // length of BYTE *outValues
    //
    wlength += sizeof (outValuesLength);
    if (outValues != NULL)
    {
        wlength += outValuesLength;
    }

    //
    // length of SPJ numResultSets
    //
    wlength += sizeof (numResultSets);

    if(numResultSets > 0)
    {
        //
        // length of result set information
        //
        rsSrvrStmt = pSrvrStmt->nextSpjRs;

        for (int i = 0; i < numResultSets; i++)
        {
            rsStmtHandle      = (Long)rsSrvrStmt;
            rsStmtLabelLength = (IDL_long)rsSrvrStmt->stmtNameLen + 1;  // add 1 for null
            rsStmtName        = (IDL_char *)rsSrvrStmt->stmtName;
            rsOutputDescBufferLength = rsSrvrStmt->outputDescBufferLength;

            wlength += sizeof(stmtHandleKey);
            wlength += sizeof(rsStmtLabelLength);
            wlength += rsStmtLabelLength;
            wlength += sizeof(charSet);
            wlength += sizeof(rsOutputDescBufferLength);
            wlength += rsOutputDescBufferLength;
            wlength += sizeof(proxySyntaxStringLen);

            if(rsSrvrStmt->SpjProxySyntaxString != NULL)
                proxySyntaxStringLen = strlen(rsSrvrStmt->SpjProxySyntaxString);
            else
                proxySyntaxStringLen = 0;

            if(proxySyntaxStringLen > 0)
                wlength += proxySyntaxStringLen + 1; // null terminated string

            rsSrvrStmt = rsSrvrStmt->nextSpjRs;

        }  // end for

    } // if numResultSets > 0

    wlength += sizeof (proxySyntaxStringLen);

    if((pSrvrStmt != NULL) && (pSrvrStmt->SpjProxySyntaxString != NULL))
        proxySyntaxStringLen = strlen(pSrvrStmt->SpjProxySyntaxString);
    else
        proxySyntaxStringLen = 0;

    if(proxySyntaxStringLen > 0)
        wlength += proxySyntaxStringLen + 1; // null terminated string


    //
    // message_length = header + data length
    //
    message_length = wlength;

    buffer = pnode->w_allocate(message_length);
    if (buffer == NULL)
        return CEE_ALLOCFAIL;


    curptr = (IDL_char*)buffer; //+ sizeof(HEADER));

    //
    // copy of IDL_long returnCode
    //
    IDL_long_copy(&returnCode, curptr);

    //
    // copy IDL_long sqlWarningOrErrorLength
    // copy BYTE *sqlWarningOrError
    //
    IDL_long_copy(&sqlWarningOrErrorLength, curptr);
    if (sqlWarningOrError != NULL)
    {
        IDL_byteArray_copy(sqlWarningOrError, sqlWarningOrErrorLength, curptr);
    }


    //
    // copy of IDL_long outDescLength
    // copy of BYTE* outDesc
    //
    IDL_long_copy(&outputDescLength, curptr);

    if(outputDescLength > 0 && outputDesc != NULL)
        IDL_byteArray_copy(outputDesc, outputDescLength, curptr);


    //
    // copy of IDL_long rowsAffected
    //
    IDL_long_copy(&rowsAffected, curptr);

    //
    // copy of IDL_long sqlQueryType
    //
    IDL_long_copy(&sqlQueryType, curptr);

    //
    // copy of IDL_long queryType
    //
    IDL_long_copy(&estimatedCost, curptr);


    //
    // copy IDL_long outValuesLength
    // copy BYTE *outValues
    //
    IDL_long_copy(&outValuesLength, curptr);

    if (outValues != NULL)
        IDL_byteArray_copy(outValues, outValuesLength, curptr);


    //
    // copy of IDL_long numResultSets
    //
    IDL_long_copy(&numResultSets, curptr);

    if(numResultSets > 0)
    {
        //
        // copy result set information
        //
        rsSrvrStmt = pSrvrStmt->nextSpjRs;

        for (int i = 0; i < numResultSets; i++)
        {
            rsStmtHandle  = (Long)rsSrvrStmt;
            rsStmtLabelLength = (IDL_long)rsSrvrStmt->stmtNameLen + 1;  // add 1 for null
            rsStmtName        = (IDL_char *)rsSrvrStmt->stmtName;
            rsOutputDescBufferLength = rsSrvrStmt->outputDescBufferLength;
            rsOutputDescBuffer = rsSrvrStmt->outputDescBuffer;

            IDL_long_copy(&stmtHandleKey, curptr);
            IDL_long_copy(&rsStmtLabelLength, curptr);
            memcpy(curptr, rsStmtName, rsStmtLabelLength - 1);  // subtract 1 for the null
            curptr = curptr + (rsStmtLabelLength - 1);
            *curptr = '\0';
            curptr = curptr + 1;
            IDL_long_copy(&charSet, curptr);
            IDL_long_copy(&rsOutputDescBufferLength, curptr);
            memcpy(curptr, rsOutputDescBuffer, rsOutputDescBufferLength);
            curptr = curptr + rsOutputDescBufferLength;

            if(rsSrvrStmt->SpjProxySyntaxString != NULL)
                proxySyntaxStringLen = strlen(rsSrvrStmt->SpjProxySyntaxString);
            else
                proxySyntaxStringLen = 0;

            if(proxySyntaxStringLen > 0)
            {
                proxySyntaxStringLen = proxySyntaxStringLen + 1; // null terminated
                IDL_long_copy(&proxySyntaxStringLen, curptr);
                IDL_charArray_copy((const IDL_char *)rsSrvrStmt->SpjProxySyntaxString, curptr);
            }
            else
                IDL_long_copy(&proxySyntaxStringLen, curptr);


            rsSrvrStmt = rsSrvrStmt->nextSpjRs;
        }  // end for

    } // if numResultSets > 0

    if((pSrvrStmt != NULL) && (pSrvrStmt->SpjProxySyntaxString != NULL))
        proxySyntaxStringLen = strlen(pSrvrStmt->SpjProxySyntaxString);
    else
        proxySyntaxStringLen = 0;

    if(proxySyntaxStringLen > 0)
    {

        proxySyntaxStringLen = proxySyntaxStringLen + 1; // null terminated
        IDL_long_copy(&proxySyntaxStringLen, curptr);
        IDL_charArray_copy((const IDL_char *)pSrvrStmt->SpjProxySyntaxString, curptr);
    }
    else
        IDL_long_copy(&proxySyntaxStringLen, curptr);

    if (curptr > buffer + message_length)
    {
        /* // Programing error handling:
        */
        exit(1000);
    }

    return CEE_SUCCESS;

}  // end odbc_SQLSrvr_Execute_param_res_()

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
        )
{
    SRVRTRACE_ENTER(FILE_OMR+15);

    IDL_char  *curptr;
    IDL_long   wlength;
    IDL_char  *buffer;
    IDL_unsigned_long message_length;

    wlength = 0;//sizeof(HEADER);

    //
    // calculate length of the buffer for each parameter
    //

    // length of IDL_long returnCode
    //
    wlength += sizeof(returnCode);

    //
    // length of IDL_long sqlWarningOrErrorLength
    // length of BYTE *sqlWarningOrError
    //
    if (sqlWarningOrError != NULL)
    {
        wlength += sizeof (sqlWarningOrErrorLength);
        wlength += sqlWarningOrErrorLength;
    }

    //
    // length of IDL_long rowsAffected
    //
    wlength += sizeof(outValuesFormat);

    //
    // length of IDL_long rowsAffected
    //
    wlength += sizeof(rowsAffected);

    //
    // length of IDL_long outValuesLength
    // length of BYTE *outValues
    //
    if (outValues != NULL)
    {
        wlength += sizeof (outValuesLength);
        wlength += outValuesLength;
    }
    else
        wlength += sizeof (outValuesLength);

    //
    // message_length = header + param + maplength + data length
    //
    message_length = wlength;

    buffer = pnode->w_allocate(message_length);
    if (buffer == NULL)
        return CEE_ALLOCFAIL;


    curptr = (IDL_char*)buffer;// + sizeof(HEADER));

    //
    // copy of IDL_long returnCode
    //
    IDL_long_copy(&returnCode, curptr);

    //
    // copy IDL_long sqlWarningOrErrorLength
    // copy BYTE *sqlWarningOrError
    //
    if ((sqlWarningOrError != NULL) && (sqlWarningOrErrorLength != 0))
    {
        IDL_long_copy(&sqlWarningOrErrorLength, curptr);
        IDL_byteArray_copy(sqlWarningOrError, sqlWarningOrErrorLength, curptr);
    }

    //
    // copy of IDL_long rowsAffected
    //
    IDL_long_copy(&rowsAffected, curptr);

    //
    // copy of IDL_long outValuesFormat
    //
    IDL_long_copy(&outValuesFormat, curptr);

    //
    // copy IDL_long outValuesLength
    // copy BYTE *outValues
    //
    if (outValues != NULL)
    {
        IDL_long_copy(&outValuesLength, curptr);
        if (outValues != NULL)
        {
            IDL_byteArray_copy(outValues, outValuesLength, curptr);
        }

    }
    else
    {
        IDL_long_copy(&outValuesLength, curptr);
    }

    if (curptr > buffer + message_length)
    {
        //LCOV_EXCL_START
        exit(1000);
        //LCOV_EXCL_START
    }

    SRVRTRACE_EXIT(FILE_OMR+15);
    return CEE_SUCCESS;

}  // end odbc_SQLSrvr_Fetch_param_res_()

