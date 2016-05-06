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


CEE_status 
FormatSQLDescSeq(
        SQLItemDescList_def *ODBCDesc
        , IDL_char *SQLDesc
        , IDL_long SQLDescLength
        , IDL_long sqlQueryType
        )
{
    IDL_long msg_total_len = 0;
    SQLItemDesc_def *SQLItemDesc;
    IDL_long data_total_len = 0;

    IDL_long VarAlign;
    IDL_long IndAlign;
    IDL_long Version;
    IDL_long DataType;
    IDL_long DateTimeCode;
    IDL_long Length;
    IDL_long Precision;
    IDL_long Scale;
    IDL_long Nullable;
    IDL_long SignType;
    IDL_long ODBCDataType;
    IDL_long ODBCPrecision;
    IDL_long SQLCharset;
    IDL_long ODBCCharset;
    IDL_long ColHeadingNmlen;
    IDL_long TableNamelen;
    IDL_long CatalogNamelen;
    IDL_long SchemaNamlen;
    IDL_long Headinglen;
    IDL_long IntLeadPrec;
    IDL_long paramMode;

    IDL_long dataBufferLength = 0;
    IDL_long numEntries = 0;

    IDL_char *curptr;
    IDL_char *padptr;
    int i;

    curptr = SQLDesc;

    dataBufferLength = *(IDL_long*)(curptr+msg_total_len);
    msg_total_len += sizeof(dataBufferLength);

    numEntries = *(IDL_long*)(curptr+msg_total_len);
    msg_total_len += sizeof(numEntries);

    if (numEntries == 0)
    {
        ODBCDesc->_length = 0;
        ODBCDesc->_buffer = 0;
    }
    else
    {
        if(sqlQueryType == SQL_INSERT_RWRS)
            ODBCDesc->_buffer = new SQLItemDesc_def[numEntries -3];
        else
            ODBCDesc->_buffer = new SQLItemDesc_def[numEntries];
        if (ODBCDesc->_buffer == NULL)
            return CEE_ALLOCFAIL;
        ODBCDesc->_length = 0;

        // Use the ODBCDesc->pad_to_offset_8_[4] to store the max row length
        // temporarily, maybe we will have other better way future.
        padptr = ODBCDesc->pad_to_offset_8_;
        IDL_long_copy(&dataBufferLength, padptr);
    }

    for (i = 0; i < numEntries; i++)
    {
        SQLItemDesc = (SQLItemDesc_def *)ODBCDesc->_buffer + ODBCDesc->_length;

        // copy VarAlign location
        VarAlign = *(IDL_long*)(curptr+msg_total_len);
        msg_total_len += sizeof(VarAlign);
        SQLItemDesc->varAlign = VarAlign;

        // copy IndAlign location
        IndAlign = *(IDL_long*)(curptr+msg_total_len);
        msg_total_len += sizeof(IndAlign);
        SQLItemDesc->indAlign = VarAlign;

        Version = *(IDL_long*)(curptr+msg_total_len);
        SQLItemDesc->version = Version;
        msg_total_len += sizeof(Version);

        DataType = *(IDL_long*)(curptr+msg_total_len);
        SQLItemDesc->dataType = DataType;
        msg_total_len += sizeof(DataType);

        DateTimeCode = *(IDL_long*)(curptr+msg_total_len);
        SQLItemDesc->datetimeCode = DateTimeCode;
        msg_total_len += sizeof(DateTimeCode);

        Length = *(IDL_long*)(curptr+msg_total_len);
        SQLItemDesc->maxLen = Length;
        msg_total_len += sizeof(Length);

        Precision = *(IDL_long*)(curptr+msg_total_len);
        SQLItemDesc->precision = (short)Precision;
        msg_total_len += sizeof(Precision);

        Scale = *(IDL_long*)(curptr+msg_total_len);
        SQLItemDesc->scale = (short)Scale;
        msg_total_len += sizeof(Scale);

        Nullable = *(IDL_long*)(curptr+msg_total_len);
        if (Nullable)
            SQLItemDesc->nullInfo = true;
        else
            SQLItemDesc->nullInfo = false;
        msg_total_len += sizeof(Nullable);

        SignType = *(IDL_long*)(curptr+msg_total_len);
        if (SignType)
            SQLItemDesc->signType = true;
        else
            SQLItemDesc->signType = false;
        msg_total_len += sizeof(SignType);

        ODBCDataType = *(IDL_long*)(curptr+msg_total_len);
        SQLItemDesc->ODBCDataType = ODBCDataType;
        msg_total_len += sizeof(ODBCDataType);

        ODBCPrecision = *(IDL_long*)(curptr+msg_total_len);
        SQLItemDesc->ODBCPrecision = (IDL_short)ODBCPrecision;
        msg_total_len += sizeof(ODBCPrecision);

        SQLCharset = *(IDL_long*)(curptr+msg_total_len);
        SQLItemDesc->SQLCharset = SQLCharset;
        msg_total_len += sizeof(SQLCharset);

        ODBCCharset = *(IDL_long*)(curptr+msg_total_len);
        SQLItemDesc->ODBCCharset = ODBCCharset;
        msg_total_len += sizeof(ODBCCharset);

        ColHeadingNmlen	= *(IDL_long*)(curptr+msg_total_len);
        msg_total_len += sizeof(ColHeadingNmlen);
        if (ColHeadingNmlen > 0)
        {
            memcpy(SQLItemDesc->colHeadingNm, curptr+msg_total_len, ColHeadingNmlen);
            msg_total_len += ColHeadingNmlen;
        }
        else
            SQLItemDesc->colHeadingNm[0] = '\0';

        TableNamelen = *(IDL_long*)(curptr+msg_total_len);
        msg_total_len += sizeof(TableNamelen);
        if (TableNamelen > 0)
        {
            memcpy(SQLItemDesc->TableName, curptr+msg_total_len, TableNamelen);
            msg_total_len += TableNamelen;
        }
        else
            SQLItemDesc->TableName[0] = '\0';

        CatalogNamelen	= *(IDL_long*)(curptr+msg_total_len);
        msg_total_len += sizeof(CatalogNamelen);
        if (CatalogNamelen > 0)
        {
            memcpy(SQLItemDesc->CatalogName, curptr+msg_total_len, CatalogNamelen);
            msg_total_len += CatalogNamelen;
        }
        else
            SQLItemDesc->CatalogName[0] = '\0';

        SchemaNamlen	= *(IDL_long*)(curptr+msg_total_len);
        msg_total_len += sizeof(SchemaNamlen);
        if (SchemaNamlen > 0)
        {
            memcpy(SQLItemDesc->SchemaName, curptr+msg_total_len, SchemaNamlen);
            msg_total_len += SchemaNamlen;
        }
        else
            SQLItemDesc->SchemaName[0] = '\0';

        Headinglen	= *(IDL_long*)(curptr+msg_total_len);
        msg_total_len += sizeof(Headinglen);
        if (Headinglen > 0)
        {
            memcpy(SQLItemDesc->Heading, curptr+msg_total_len, Headinglen);
            msg_total_len += Headinglen;
        }
        else
            SQLItemDesc->Heading[0] = '\0';

        IntLeadPrec = *(IDL_long*)(curptr+msg_total_len);
        SQLItemDesc->intLeadPrec = IntLeadPrec;
        msg_total_len += sizeof(IntLeadPrec);

        paramMode = *(IDL_long*)(curptr+msg_total_len);
        SQLItemDesc->paramMode = paramMode;
        msg_total_len += sizeof(paramMode);

        if(sqlQueryType == SQL_INSERT_RWRS)
        {
            if(i >= 3)
                ODBCDesc->_length++;
        }
        else
            ODBCDesc->_length++;
    }

    if (msg_total_len > SQLDescLength)
    {
        return CEE_INTERNALFAIL;
    }

    return CEE_SUCCESS;
}

void setWarningOrErrors(ExceptionStruct *exception, const BYTE *WarningOrError, IDL_long returnCode) 
{
    IDL_long msg_total_len = 0;

    IDL_long numConditions = 0;
    char sqlState[6];
    IDL_long sqlCode;
    IDL_long errorTextLen = 0;
    IDL_char *errorText;
    IDL_long rowId = 0;

    ERROR_DESC_def *error_desc_def = NULL;

    sqlState[0] = '\0';

    unsigned char *curptr;
    int i;

    curptr = (unsigned char *)WarningOrError;

    numConditions = *(IDL_long*)(curptr+msg_total_len);
    msg_total_len +=4;

    if (numConditions > 0)
    {
        MEMORY_ALLOC_ARRAY(exception->u.SQLError.errorList._buffer, ERROR_DESC_def, numConditions);
        error_desc_def = exception->u.SQLError.errorList._buffer;
        for (i = 0; i < numConditions; i++)
        {
            error_desc_def->rowId = *(IDL_long*)(curptr+msg_total_len);
            msg_total_len +=4;

            error_desc_def->sqlcode = *(IDL_long*)(curptr+msg_total_len);
            msg_total_len +=4;

            errorTextLen= *(IDL_long*)(curptr+msg_total_len);
            msg_total_len +=4;

            if (errorTextLen > 0)
            {
                error_desc_def->errorText = new char[errorTextLen];
                memcpy(error_desc_def->errorText, curptr+msg_total_len, errorTextLen);
                msg_total_len +=errorTextLen;
            }
            else
            {
                error_desc_def->errorText = new char[1]; 
                error_desc_def->errorText[0] = '\0';
            }

            memcpy(error_desc_def->sqlstate, curptr+msg_total_len, sizeof(error_desc_def->sqlstate));
            error_desc_def->sqlstate[5] = '\0';
            msg_total_len +=sizeof(error_desc_def->sqlstate);
        }
    }
}

