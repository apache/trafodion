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

#include <platform_ndcs.h>
#include <cee.h>
#include <idltype.h>
#include <marshaling.h>

#define SRVRTRACE_ENTER(name)
#define SRVRTRACE_EXIT(name)

//
//----------- input parameters ------------------------------
//
CEE_status decodeParameters(short count, long* param[], char* buffer, long length)
{
    SRVRTRACE_ENTER(FILE_IMR+1);
    char* pbuffer = buffer;
    long* parptr, *mapptr;
    int i;
    long start_data = 0;
    long end_data = length;
    long offset;

    memset (param, 0, count*sizeof(long));

    for (start_data=count * sizeof(long) ;start_data < length; start_data += sizeof(long))
    {
        if ((long)buffer[start_data] == 0)
        {
            start_data += sizeof(long);
            break;
        }
    }
    if (start_data >= length)
    {
        SRVRTRACE_EXIT(FILE_IMR+1);
        return -1;
    }

    for (parptr = (long*)pbuffer, i=0; i < count; i++, parptr++)
    {
        if (*parptr != 0)
        {
            offset = *(parptr);
            if (offset < start_data || offset >= end_data)
            {
                SRVRTRACE_EXIT(FILE_IMR+1);
                return i+1;
            }
            *(parptr) += (long)pbuffer;
            param[i] = (long*)*(parptr);
        }
        else
            param[i] = NULL;
    }

    for (mapptr = (long*)buffer + count; *mapptr != 0; mapptr++, i++)
    {
        offset =(long)*mapptr;
        if (offset < start_data || offset >= end_data)
        {
            SRVRTRACE_EXIT(FILE_IMR+1);
            return i+1;
        }
        parptr = (long*)(*mapptr + pbuffer);
        offset = *(parptr);
        if (offset < start_data || offset >= end_data)
        {
            SRVRTRACE_EXIT(FILE_IMR+1);
            return i+1;
        }
        *parptr += (long)pbuffer;
    }

    SRVRTRACE_EXIT(FILE_IMR+1);
    return CEE_SUCCESS;
}
//
//---------------- output parameters ---------------------------
//
//---------------- calculate the length ------------------------
//
void LIST_length( LIST_def* pname, long length, long& wlength, long& maplength)
{
    if (pname->_buffer != NULL && pname->_length > 0)
    {
        wlength += ( pname->_length * length);
        maplength += sizeof(long);
    }
}

void OCTET_length( OCTET_def* pname, long& wlength, long& maplength)
{
    if (pname->_buffer != NULL && pname->_length > 0)
    {
        wlength += pname->_length;
        maplength += sizeof(long);
    }
}

void STRING_length( IDL_string pname,  long& wlength, long& maplength)
{
    if (pname != NULL)
    {
        wlength += strlen(pname) + 1;
        maplength += sizeof(long);
    }
}

void ERROR_DESC_LIST_length( ERROR_DESC_LIST_def* pname,  long& wlength, long& maplength )
{
    SRVRTRACE_ENTER(FILE_IMR+2);
    wlength += sizeof(ERROR_DESC_LIST_def);

    LIST_length((LIST_def*) pname, sizeof(ERROR_DESC_def), wlength, maplength);

    if (pname->_buffer != NULL && pname->_length > 0)
    {
        ERROR_DESC_def* pED;
        for (unsigned int i = 0; i < pname->_length; i++)
        {
            pED = pname->_buffer + i;

            STRING_length( pED->errorText, wlength, maplength);
            STRING_length( pED->Param1, wlength, maplength);
            STRING_length( pED->Param2, wlength, maplength);
            STRING_length( pED->Param3, wlength, maplength);
            STRING_length( pED->Param4, wlength, maplength);
            STRING_length( pED->Param5, wlength, maplength);
            STRING_length( pED->Param6, wlength, maplength);
            STRING_length( pED->Param7, wlength, maplength);
        }
    }
    SRVRTRACE_EXIT(FILE_IMR+2);
}

// New format
void ERROR_DESC_LIST_length( ERROR_DESC_LIST_def* pname,  IDL_long& wlength)
{
    IDL_unsigned_long i = 0;

    wlength += sizeof(pname->_length);

    if(pname->_length > 0 && pname->_buffer != NULL)
    {
        ERROR_DESC_def *ptr = pname->_buffer;

        for(i = 0; i < pname->_length; i++)
        {
            wlength += sizeof(ptr->rowId);
            wlength += sizeof(ptr->errorDiagnosticId);
            wlength += sizeof(ptr->sqlcode);
            wlength += sizeof(ptr->sqlstate);

            wlength += sizeof(IDL_long);
            if (ptr->errorText != NULL)
                wlength += strlen(ptr->errorText) + 1;

            wlength += sizeof(ptr->operationAbortId);
            wlength += sizeof(ptr->errorCodeType);

            wlength += sizeof(IDL_long);
            if (ptr->Param1 != NULL)
                wlength += strlen(ptr->Param1) + 1;

            wlength += sizeof(IDL_long);
            if (ptr->Param2 != NULL)
                wlength += strlen(ptr->Param2) + 1;

            wlength += sizeof(IDL_long);
            if (ptr->Param3 != NULL)
                wlength += strlen(ptr->Param3) + 1;

            wlength += sizeof(IDL_long);
            if (ptr->Param4 != NULL)
                wlength += strlen(ptr->Param4) + 1;

            wlength += sizeof(IDL_long);
            if (ptr->Param5 != NULL)
                wlength += strlen(ptr->Param5) + 1;

            wlength += sizeof(IDL_long);
            if (ptr->Param6 != NULL)
                wlength += strlen(ptr->Param6) + 1;

            wlength += sizeof(IDL_long);
            if (ptr->Param7 != NULL)
                wlength += strlen(ptr->Param7) + 1;

            ptr++;

        }

    }

} // ERROR_DESC_LIST_length

void SRVR_CONTEXT_length( const SRVR_CONTEXT_def* pname, long& wlength, long& maplength )
{
    SRVRTRACE_ENTER(FILE_IMR+3);
    wlength += sizeof(SRVR_CONTEXT_def);
    ENV_DESC_LIST_length( &pname->envDescList, wlength, maplength );
    RES_DESC_LIST_length( &pname->resDescList, wlength, maplength );
    SRVRTRACE_EXIT(FILE_IMR+3);
}

void DATASOURCE_CFG_LIST_length( const DATASOURCE_CFG_LIST_def* pname,  long& wlength, long& maplength )
{
    SRVRTRACE_ENTER(FILE_IMR+4);
    wlength += sizeof(DATASOURCE_CFG_LIST_def);

    LIST_length((LIST_def*)pname, sizeof(DATASOURCE_CFG_def), wlength, maplength);

    if (pname->_buffer != NULL && pname->_length > 0)
    {
        DATASOURCE_CFG_def* pDCFG;

        for (unsigned int i=0; i < pname->_length; i++)
        {
            pDCFG = pname->_buffer + i;
            ENV_DESC_LIST_length( &pDCFG->EnvDescList, wlength, maplength );
            RES_DESC_LIST_length( &pDCFG->ResDescList, wlength, maplength );
            OCTET_length( (OCTET_def*)&pDCFG->DefineDescList, wlength, maplength);
        }
    }
    SRVRTRACE_EXIT(FILE_IMR+4);
}

void SQLVALUE_LIST_length( const SQLValueList_def* pname,  long& wlength, long& maplength )
{
    SRVRTRACE_ENTER(FILE_IMR+5);
    wlength += sizeof(SQLValueList_def);

    LIST_length((LIST_def*)pname, sizeof(SQLValue_def), wlength, maplength);

    if (pname->_buffer != NULL && pname->_length > 0)
    {
        SQLValue_def* pSQLValue;

        for (unsigned int i=0; i < pname->_length; i++)
        {
            wlength += sizeof(SQLValue_def);

            pSQLValue = pname->_buffer + i;

            OCTET_length( (OCTET_def*)&pSQLValue->dataValue, wlength, maplength);

        }
    }
    SRVRTRACE_EXIT(FILE_IMR+5);
}

// new format
void SQLVALUE_LIST_length(SQLValueList_def* pname,  IDL_long& wlength)
{
    IDL_unsigned_long i;

    wlength += sizeof(pname->_length);

    if(pname->_length > 0)
    {
        SQLValue_def *pSQLValue = pname->_buffer;

        for(i = 0; i < pname->_length; i++)
        {
            wlength += sizeof(pSQLValue->dataType);
            wlength += sizeof(pSQLValue->dataInd);
            wlength += sizeof(pSQLValue->dataValue._length);
            if(pSQLValue->dataValue._length > 0)
                wlength += pSQLValue->dataValue._length;
            wlength += sizeof(pSQLValue->dataCharset);
            pSQLValue++;
        }

    }

} // SQLVALUE_LIST_length()


void SQLITEMDESC_LIST_length(SQLItemDescList_def* pname,  IDL_long& wlength)
{
    IDL_unsigned_long i;
    IDL_unsigned_long tmpLength = 0;

    wlength += sizeof(pname->_length);

    if(pname->_length > 0)
    {

        tmpLength += sizeof(pname->_buffer->version);
        tmpLength += sizeof(pname->_buffer->dataType);
        tmpLength += sizeof(pname->_buffer->datetimeCode);
        tmpLength += sizeof(pname->_buffer->maxLen);
        tmpLength += sizeof(pname->_buffer->precision);
        tmpLength += sizeof(pname->_buffer->scale);
        tmpLength += sizeof(pname->_buffer->nullInfo);
        tmpLength += sizeof(IDL_long); // length followed by data (colHeadingNm)
        tmpLength += sizeof(pname->_buffer->signType);	  
        tmpLength += sizeof(pname->_buffer->ODBCDataType);
        tmpLength += sizeof(pname->_buffer->ODBCPrecision);	  
        tmpLength += sizeof(pname->_buffer->SQLCharset);
        tmpLength += sizeof(pname->_buffer->ODBCCharset);
        tmpLength += sizeof(IDL_long); // length followed by data (TableName)
        tmpLength += sizeof(IDL_long); // length followed by data (CatalogName)
        tmpLength += sizeof(IDL_long); // length followed by data (SchemaName)
        tmpLength += sizeof(IDL_long); // length followed by data (Heading)
        tmpLength += sizeof(pname->_buffer->intLeadPrec);
        tmpLength += sizeof(pname->_buffer->paramMode);


        wlength += (pname->_length * tmpLength);

        SQLItemDesc_def *pSQLItemDesc = pname->_buffer;

        for(i = 0; i < pname->_length; i++)
        {
            if(pSQLItemDesc->colHeadingNm[0] != '\0')
                wlength += (strlen(pSQLItemDesc->colHeadingNm) +1);
            if(pSQLItemDesc->colHeadingNm[0] != '\0')
                wlength += (strlen(pSQLItemDesc->TableName) +1);
            if(pSQLItemDesc->colHeadingNm[0] != '\0')
                wlength += (strlen(pSQLItemDesc->CatalogName) +1);
            if(pSQLItemDesc->colHeadingNm[0] != '\0')
                wlength += (strlen(pSQLItemDesc->SchemaName) +1);
            if(pSQLItemDesc->colHeadingNm[0] != '\0')
                wlength += (strlen(pSQLItemDesc->Heading) +1);
            pSQLItemDesc++;
        }
    }

} // SQLITEMDESC_LIST_length()


void SRVR_STATUS_LIST_length( const SRVR_STATUS_LIST_def* pname, long& wlength, long& maplength )
{
    SRVRTRACE_ENTER(FILE_IMR+6);
    wlength += sizeof(SRVR_STATUS_LIST_def);

    LIST_length((LIST_def*)pname, sizeof(SRVR_STATUS_def), wlength, maplength);

    if (pname->_buffer != NULL && pname->_length > 0)
    {
        SRVR_STATUS_def* pSRVR;

        for (unsigned int i=0; i < pname->_length; i++)
        {
            wlength += sizeof(SRVR_STATUS_def);

            pSRVR = pname->_buffer + i;

            STRING_length( pSRVR->userName, wlength, maplength);
            STRING_length( pSRVR->windowText, wlength, maplength);

        }
    }
    SRVRTRACE_EXIT(FILE_IMR+6);
}

void ENV_DESC_LIST_length( const ENV_DESC_LIST_def* pname, long& wlength, long& maplength )
{
    SRVRTRACE_ENTER(FILE_IMR+7);
    wlength += sizeof(ENV_DESC_LIST_def);

    LIST_length((LIST_def*)pname, sizeof(ENV_DESC_def), wlength, maplength);
    if (pname->_buffer != NULL && pname->_length > 0)
    {
        ENV_DESC_def* pED;
        for (unsigned int i = 0; i < pname->_length; i++)
        {
            pED = pname->_buffer + i;

            STRING_length( pED->VarVal, wlength, maplength);
        }
    }
    SRVRTRACE_EXIT(FILE_IMR+7);
}

void RES_DESC_LIST_length( const RES_DESC_LIST_def* pname, long& wlength, long& maplength )
{
    SRVRTRACE_ENTER(FILE_IMR+8);
    wlength += sizeof(RES_DESC_LIST_def);

    LIST_length((LIST_def*)pname, sizeof(RES_DESC_def), wlength, maplength);
    if (pname->_buffer != NULL && pname->_length > 0)
    {
        RES_DESC_def* pRD;
        for (unsigned int i = 0; i < pname->_length; i++)
        {
            pRD = pname->_buffer + i;

            STRING_length( pRD->Action, wlength, maplength);
        }
    }
    SRVRTRACE_EXIT(FILE_IMR+8);
}

void DATASOURCE_CFG_length( const DATASOURCE_CFG_def* pname, long& wlength, long& maplength )
{
    SRVRTRACE_ENTER(FILE_IMR+9);
    wlength += sizeof(DATASOURCE_CFG_def);
    ENV_DESC_LIST_length( &pname->EnvDescList, wlength, maplength );
    RES_DESC_LIST_length( &pname->ResDescList, wlength, maplength );
    OCTET_length( (OCTET_def*)&pname->DefineDescList, wlength, maplength);
    SRVRTRACE_EXIT(FILE_IMR+9);
}

//
//--------------------- copy parameters ------------------------
//
void IDL_charArray_copy(const IDL_char* pname, char*& curptr)
{
    long length;

    if (pname != NULL)
    {
        length = strlen(pname) + 1;
        memcpy(curptr, pname,length);
        curptr += length;
    }
}

void IDL_charArray_Pad_copy(const IDL_char* pname, long length, char*& curptr)
{
    if (pname != NULL)
    {
        memset(curptr,0,length);
        memcpy(curptr, pname,length);
        curptr += length;
    }
}

void IDL_byteArray_copy(BYTE* pname, long length, char*& curptr)
{
    if (pname != NULL)
    {
        memcpy(curptr, pname,length);
        curptr += length;
    }
}

/*
 *	for 64bit
 */

void Long_copy(Long* pname, char*& curptr)
{
    memcpy(curptr, pname, sizeof(Long));
    curptr += sizeof(Long);
}

void IDL_long_copy(IDL_long* pname, char*& curptr)
{
    memcpy(curptr, pname, sizeof(IDL_long));
    curptr += sizeof(IDL_long);
}

void IDL_unsigned_long_copy(IDL_unsigned_long* pname, char*& curptr)
{
    memcpy(curptr, pname, sizeof(IDL_unsigned_long));
    curptr += sizeof(IDL_unsigned_long);
}

void IDL_short_copy(IDL_short* pname, char*& curptr)
{
    memcpy(curptr, pname, sizeof(IDL_short));
    curptr += sizeof(IDL_short);
}

void IDL_unsigned_short_copy(IDL_unsigned_short* pname, char*& curptr)
{
    memcpy(curptr, pname, sizeof(IDL_unsigned_short));
    curptr += sizeof(IDL_unsigned_short);
}

void IDL_long_long_copy(IDL_long_long* pname, char*& curptr)
{
    memcpy(curptr, pname, sizeof(IDL_long_long));
    curptr += sizeof(IDL_long_long);
}

void IDL_unsigned_long_long_copy(IDL_unsigned_long_long* pname, char*& curptr)
{
    memcpy(curptr, pname, sizeof(IDL_unsigned_long_long));
    curptr += sizeof(IDL_unsigned_long_long);
}

void IDL_double_copy(double* pname, char*& curptr)
{
    memcpy(curptr, pname, sizeof(double));
    curptr += sizeof(double);
}

void LIST_copy( char* buffer, LIST_def* pname, LIST_def* parptr, long length, char*& curptr, long*& mapptr)
{
    long* tmpptr;
    if (pname->_buffer != NULL && pname->_length > 0)
    {
        tmpptr = (long*)&(parptr->_buffer);
        *(mapptr++) = (char *)tmpptr - buffer;
        *tmpptr = curptr - buffer;
        memcpy(curptr, pname->_buffer, pname->_length * length);
        curptr += pname->_length * length;
    }
}

void OCTET_copy( char* buffer, OCTET_def* pname, OCTET_def* parptr, char*& curptr, long*& mapptr)
{
    long* tmpptr;
    if (pname->_buffer != NULL && pname->_length > 0)
    {
        tmpptr = (long*)&(parptr->_buffer);
        *(mapptr++) = (char *)tmpptr - buffer;
        *tmpptr =  curptr - buffer;
        memcpy(curptr, pname->_buffer, pname->_length);
        curptr += pname->_length;
    }
}

void STRING_copy( char* buffer, IDL_string pname, IDL_string* parptr, char*& curptr, long*& mapptr)
{
    long length;
    long* tmpptr;

    if (pname != NULL)
    {
        tmpptr = (long*)parptr;
        *(mapptr++) = (char *)tmpptr - buffer;
        *tmpptr =  curptr - buffer;
        length = strlen(pname) + 1;
        memcpy(curptr,pname, length);
        curptr += length;
    }
}

void ERROR_DESC_LIST_copy( char* buffer, ERROR_DESC_LIST_def* pname, ERROR_DESC_LIST_def* parptr, char*& curptr, long*& mapptr)
{
    SRVRTRACE_ENTER(FILE_IMR+10);
    memcpy(curptr, pname, sizeof(ERROR_DESC_LIST_def));
    curptr += sizeof(ERROR_DESC_LIST_def);

    LIST_copy( buffer, (LIST_def*)pname, (LIST_def*) parptr, sizeof(ERROR_DESC_def), curptr, mapptr);

    if (pname->_buffer != NULL && pname->_length > 0)
    {
        ERROR_DESC_def* pED;
        ERROR_DESC_def* pEDpar;
        for (unsigned int i = 0; i < pname->_length; i++)
        {
            pED = pname->_buffer + i;
            pEDpar = (ERROR_DESC_def*)((ULONG)parptr->_buffer + (ULONG)buffer + i*sizeof(ERROR_DESC_def));

            STRING_copy( buffer, pED->errorText, &pEDpar->errorText, curptr, mapptr);
            STRING_copy( buffer, pED->Param1, &pEDpar->Param1, curptr, mapptr);
            STRING_copy( buffer, pED->Param2, &pEDpar->Param2, curptr, mapptr);
            STRING_copy( buffer, pED->Param3, &pEDpar->Param3, curptr, mapptr);
            STRING_copy( buffer, pED->Param4, &pEDpar->Param4, curptr, mapptr);
            STRING_copy( buffer, pED->Param5, &pEDpar->Param5, curptr, mapptr);
            STRING_copy( buffer, pED->Param6, &pEDpar->Param6, curptr, mapptr);
            STRING_copy( buffer, pED->Param7, &pEDpar->Param7, curptr, mapptr);
        }
    }
    SRVRTRACE_EXIT(FILE_IMR+10);
}

// marshalling - new format
void ERROR_DESC_LIST_copy(ERROR_DESC_LIST_def* pname, IDL_char*& curptr)
{

    IDL_unsigned_long i = 0;
    IDL_long tmpLength;

    IDL_unsigned_long_copy(&pname->_length, curptr);

    if(pname->_length > 0 && pname->_buffer != NULL)
    {
        ERROR_DESC_def *ptr = pname->_buffer;

        for(i = 0; i < pname->_length; i++)
        {

            IDL_long_copy(&ptr->rowId, curptr);
            IDL_long_copy(&ptr->errorDiagnosticId, curptr);
            IDL_long_copy(&ptr->sqlcode, curptr);

            strncpy(curptr,ptr->sqlstate,sizeof(ptr->sqlstate));
            curptr += sizeof(ptr->sqlstate);

            tmpLength = (ptr->errorText == NULL) ? 0 : strlen(ptr->errorText);
            if(tmpLength > 0)
            {
                tmpLength++; // null terminator
                IDL_long_copy(&tmpLength,curptr);
                IDL_charArray_copy(ptr->errorText,curptr);

            }
            else
                IDL_long_copy(&tmpLength,curptr);

            IDL_long_copy(&ptr->operationAbortId,curptr);
            IDL_long_copy(&ptr->errorCodeType,curptr);

            tmpLength = (ptr->Param1 == NULL) ? 0 : strlen(ptr->Param1);
            if(tmpLength > 0)
            {
                tmpLength++; // null terminator
                IDL_long_copy(&tmpLength,curptr);
                IDL_charArray_copy(ptr->Param1,curptr);

            }
            else
                IDL_long_copy(&tmpLength,curptr);

            tmpLength = (ptr->Param2 == NULL) ? 0 : strlen(ptr->Param2);
            if(tmpLength > 0)
            {
                tmpLength++; // null terminator
                IDL_long_copy(&tmpLength,curptr);
                IDL_charArray_copy(ptr->Param2,curptr);

            }
            else
                IDL_long_copy(&tmpLength,curptr);

            tmpLength = (ptr->Param3 == NULL) ? 0 : strlen(ptr->Param3);
            if(tmpLength > 0)
            {
                tmpLength++; // null terminator
                IDL_long_copy(&tmpLength,curptr);
                IDL_charArray_copy(ptr->Param3,curptr);

            }
            else
                IDL_long_copy(&tmpLength,curptr);

            tmpLength = (ptr->Param4 == NULL) ? 0 : strlen(ptr->Param4);
            if(tmpLength > 0)
            {
                tmpLength++; // null terminator
                IDL_long_copy(&tmpLength,curptr);
                IDL_charArray_copy(ptr->Param4,curptr);

            }
            else
                IDL_long_copy(&tmpLength,curptr);

            tmpLength = (ptr->Param5 == NULL) ? 0 : strlen(ptr->Param5);
            if(tmpLength > 0)
            {
                tmpLength++; // null terminator
                IDL_long_copy(&tmpLength,curptr);
                IDL_charArray_copy(ptr->Param5,curptr);

            }
            else
                IDL_long_copy(&tmpLength,curptr);			

            tmpLength = (ptr->Param6 == NULL) ? 0 : strlen(ptr->Param6);
            if(tmpLength > 0)
            {
                tmpLength++; // null terminator
                IDL_long_copy(&tmpLength,curptr);
                IDL_charArray_copy(ptr->Param6,curptr);

            }
            else
                IDL_long_copy(&tmpLength,curptr);

            tmpLength = (ptr->Param7 == NULL) ? 0 : strlen(ptr->Param7);
            if(tmpLength > 0)
            {
                tmpLength++; // null terminator
                IDL_long_copy(&tmpLength,curptr);
                IDL_charArray_copy(ptr->Param7,curptr);

            }
            else
                IDL_long_copy(&tmpLength,curptr);

            ptr++;
        }
    }

} /* ERROR_DESC_LIST_copy() */

void SRVR_CONTEXT_copy( char* buffer, const SRVR_CONTEXT_def* pname, SRVR_CONTEXT_def* parptr, char*& curptr, long*& mapptr)
{
    SRVRTRACE_ENTER(FILE_IMR+11);
    memcpy(curptr, pname, sizeof(SRVR_CONTEXT_def));
    curptr += sizeof(SRVR_CONTEXT_def);

    ENV_DESC_LIST_copy(buffer, &pname->envDescList, &parptr->envDescList, curptr, mapptr);
    RES_DESC_LIST_copy(buffer, &pname->resDescList, &parptr->resDescList, curptr, mapptr);
    SRVRTRACE_EXIT(FILE_IMR+11);
}

void DATASOURCE_CFG_LIST_copy( char* buffer, const DATASOURCE_CFG_LIST_def* pname, DATASOURCE_CFG_LIST_def* parptr, char*& curptr, long*& mapptr)
{
    SRVRTRACE_ENTER(FILE_IMR+12);
    memcpy(curptr, pname, sizeof(DATASOURCE_CFG_LIST_def));
    curptr += sizeof(DATASOURCE_CFG_LIST_def);

    LIST_copy( buffer, (LIST_def*)pname, (LIST_def*)parptr, sizeof(DATASOURCE_CFG_def), curptr, mapptr);

    if (pname->_buffer != NULL && pname->_length > 0)
    {
        DATASOURCE_CFG_def* pDCFG;
        DATASOURCE_CFG_def* pDCFGpar;

        for (unsigned int i=0; i < pname->_length; i++)
        {
            pDCFG = pname->_buffer + i;
            pDCFGpar = (DATASOURCE_CFG_def*)((ULONG)parptr->_buffer + (ULONG)buffer + i*sizeof(DATASOURCE_CFG_def));

            ENV_DESC_LIST_copy(buffer,  &pDCFG->EnvDescList, &pDCFGpar->EnvDescList, curptr, mapptr);
            RES_DESC_LIST_copy(buffer,  &pDCFG->ResDescList, &pDCFGpar->ResDescList, curptr, mapptr);
            OCTET_copy( buffer, (OCTET_def*)&pDCFG->DefineDescList, (OCTET_def*)&pDCFGpar->DefineDescList, curptr, mapptr);
        }
    }
    SRVRTRACE_EXIT(FILE_IMR+12);
}

void SQLVALUE_LIST_copy( char* buffer, const SQLValueList_def* pname, SQLValueList_def* parptr, char*& curptr, long*& mapptr)
{
    SRVRTRACE_ENTER(FILE_IMR+13);
    memcpy(curptr, pname, sizeof(SQLValueList_def));
    curptr += sizeof(SQLValueList_def);

    LIST_copy( buffer, (LIST_def*)pname, (LIST_def*)parptr, sizeof(SQLValue_def), curptr, mapptr);

    if (pname->_buffer != NULL && pname->_length > 0)
    {
        SQLValue_def* pSQLValue;
        SQLValue_def* pSQLValuepar;

        for (unsigned int i=0; i < pname->_length; i++)
        {
            pSQLValue = pname->_buffer + i;
            pSQLValuepar = (SQLValue_def*)((ULONG)parptr->_buffer + (ULONG)buffer + i*sizeof(SQLValue_def));

            OCTET_copy( buffer, (OCTET_def*)&pSQLValue->dataValue, (OCTET_def*)&pSQLValuepar->dataValue, curptr, mapptr);
        }
    }
    SRVRTRACE_EXIT(FILE_IMR+13);
}

// new format
void SQLVALUE_LIST_copy(SQLValueList_def* pname, IDL_char*& curptr)
{

    IDL_unsigned_long i;

    IDL_unsigned_long_copy(&pname->_length, curptr);

    if(pname->_length > 0)
    {
        SQLValue_def *pSQLValue = pname->_buffer;

        for(i = 0; i < pname->_length; i++)
        {
            IDL_long_copy(&pSQLValue->dataType,curptr);
            IDL_short_copy(&pSQLValue->dataInd,curptr);
            IDL_unsigned_long_copy(&pSQLValue->dataValue._length, curptr);
            if(pSQLValue->dataValue._length > 0)
                IDL_byteArray_copy(pSQLValue->dataValue._buffer,pSQLValue->dataValue._length,curptr);
            IDL_long_copy(&pSQLValue->dataCharset,curptr);
            pSQLValue++;
        }

    }	
} // SQLVALUE_LIST_copy()

// new format
void SQLITEMDESC_LIST_copy(SQLItemDescList_def* pname, IDL_char*& curptr)
{

    IDL_unsigned_long i;

    IDL_long colHeadingNmLength = 0;
    IDL_long TableNameLength = 0;
    IDL_long CatalogNameLength = 0;
    IDL_long SchemaNameLength = 0;
    IDL_long HeadingLength = 0;


    IDL_unsigned_long_copy(&pname->_length, curptr);

    if(pname->_length > 0 && pname->_buffer != NULL)
    {
        SQLItemDesc_def *pSQLItemDesc = pname->_buffer;

        for(i = 0; i < pname->_length; i++)
        {
            IDL_long_copy(&pSQLItemDesc->version,curptr);
            IDL_long_copy(&pSQLItemDesc->dataType,curptr);
            IDL_long_copy(&pSQLItemDesc->datetimeCode,curptr);
            IDL_long_copy(&pSQLItemDesc->maxLen,curptr);
            IDL_short_copy(&pSQLItemDesc->precision,curptr);
            IDL_short_copy(&pSQLItemDesc->scale,curptr);
            IDL_byteArray_copy(&pSQLItemDesc->nullInfo,sizeof(pSQLItemDesc->nullInfo),curptr);

            if(pSQLItemDesc->colHeadingNm[0] != '\0')
                colHeadingNmLength = (strlen(pSQLItemDesc->colHeadingNm) +1);
            else
                colHeadingNmLength = 0;
            IDL_long_copy(&colHeadingNmLength,curptr);
            if(colHeadingNmLength > 0)
                IDL_charArray_copy(pSQLItemDesc->colHeadingNm,curptr);


            IDL_byteArray_copy(&pSQLItemDesc->signType,sizeof(pSQLItemDesc->signType),curptr);
            IDL_long_copy(&pSQLItemDesc->ODBCDataType,curptr);
            IDL_short_copy(&pSQLItemDesc->ODBCPrecision,curptr);
            IDL_long_copy(&pSQLItemDesc->SQLCharset,curptr);
            IDL_long_copy(&pSQLItemDesc->ODBCCharset,curptr);

            if(pSQLItemDesc->TableName[0] != '\0')
                TableNameLength = (strlen(pSQLItemDesc->TableName) +1);
            else
                TableNameLength = 0;
            IDL_long_copy(&TableNameLength,curptr);
            if(TableNameLength > 0)
                IDL_charArray_copy(pSQLItemDesc->TableName,curptr);

            if(pSQLItemDesc->CatalogName[0] != '\0')
                CatalogNameLength = (strlen(pSQLItemDesc->CatalogName) +1);
            else
                CatalogNameLength = 0;
            IDL_long_copy(&CatalogNameLength,curptr);
            if(CatalogNameLength > 0)
                IDL_charArray_copy(pSQLItemDesc->CatalogName,curptr);

            if(pSQLItemDesc->SchemaName[0] != '\0')
                SchemaNameLength = (strlen(pSQLItemDesc->SchemaName) +1);
            else
                SchemaNameLength = 0;
            IDL_long_copy(&SchemaNameLength,curptr);
            if(SchemaNameLength > 0)
                IDL_charArray_copy(pSQLItemDesc->SchemaName,curptr);

            if(pSQLItemDesc->Heading[0] != '\0')
                HeadingLength = (strlen(pSQLItemDesc->Heading) +1);
            else
                HeadingLength = 0;
            IDL_long_copy(&HeadingLength,curptr);
            if(HeadingLength > 0)
                IDL_charArray_copy(pSQLItemDesc->Heading,curptr);

            IDL_long_copy(&pSQLItemDesc->intLeadPrec,curptr);
            IDL_long_copy(&pSQLItemDesc->paramMode,curptr);

            pSQLItemDesc++;
        }

    }	
} // SQLVALUE_LIST_copy()


void SRVR_STATUS_LIST_copy(char* buffer, const SRVR_STATUS_LIST_def* pname, SRVR_STATUS_LIST_def* parptr, char*& curptr, long*& mapptr)
{
    SRVRTRACE_ENTER(FILE_IMR+14);
    memcpy(curptr, pname, sizeof(SRVR_STATUS_LIST_def));
    curptr += sizeof(SRVR_STATUS_LIST_def);

    LIST_copy( buffer, (LIST_def*)pname, (LIST_def*)parptr, sizeof(SRVR_STATUS_def), curptr, mapptr);

    if (pname->_buffer != NULL && pname->_length > 0)
    {
        SRVR_STATUS_def* pSRVR;
        SRVR_STATUS_def* pSRVRpar;

        for (unsigned int i=0; i < pname->_length; i++)
        {
            pSRVR = pname->_buffer + i;
            pSRVRpar = (SRVR_STATUS_def*)((ULONG)parptr->_buffer + (ULONG)buffer + i*sizeof(SRVR_STATUS_def));

            STRING_copy( buffer, pSRVR->userName, &pSRVRpar->userName, curptr, mapptr);
            STRING_copy( buffer, pSRVR->windowText, &pSRVRpar->windowText, curptr, mapptr);
        }
    }
    SRVRTRACE_EXIT(FILE_IMR+14);
}

void ENV_DESC_LIST_copy(char* buffer,  const ENV_DESC_LIST_def* pname, ENV_DESC_LIST_def* parptr, char*& curptr, long*& mapptr)
{
    SRVRTRACE_ENTER(FILE_IMR+15);
    memcpy(curptr, pname, sizeof(ENV_DESC_LIST_def));
    curptr += sizeof(ENV_DESC_LIST_def);

    LIST_copy( buffer, (LIST_def*)pname, (LIST_def*)parptr, sizeof(ENV_DESC_def), curptr, mapptr);

    if (pname->_buffer != NULL && pname->_length > 0)
    {
        ENV_DESC_def* pED;
        ENV_DESC_def* pEDpar;
        for (unsigned int i = 0; i < pname->_length; i++)
        {
            pED = pname->_buffer + i;
            pEDpar = (ENV_DESC_def*)((ULONG)parptr->_buffer + (ULONG)buffer + i*sizeof(ENV_DESC_def));

            STRING_copy( buffer, pED->VarVal, &pEDpar->VarVal, curptr, mapptr);
        }
    }
    SRVRTRACE_EXIT(FILE_IMR+15);
}

void RES_DESC_LIST_copy(char* buffer,  const RES_DESC_LIST_def* pname, RES_DESC_LIST_def* parptr, char*& curptr, long*& mapptr)
{
    SRVRTRACE_ENTER(FILE_IMR+16);
    memcpy(curptr, pname, sizeof(RES_DESC_LIST_def));
    curptr += sizeof(RES_DESC_LIST_def);

    LIST_copy( buffer, (LIST_def*)pname, (LIST_def*)parptr, sizeof(RES_DESC_def), curptr, mapptr);

    if (pname->_buffer != NULL && pname->_length > 0)
    {
        RES_DESC_def* pRD;
        RES_DESC_def* pRDpar;
        for (unsigned int i = 0; i < pname->_length; i++)
        {
            pRD = pname->_buffer + i;
            pRDpar = (RES_DESC_def*)((ULONG)parptr->_buffer + (ULONG)buffer + i*sizeof(RES_DESC_def));

            STRING_copy( buffer, pRD->Action, &pRDpar->Action, curptr, mapptr);
        }
    }
    SRVRTRACE_EXIT(FILE_IMR+16);
}

void DATASOURCE_CFG_copy( char* buffer, const DATASOURCE_CFG_def* pname, DATASOURCE_CFG_def* parptr, char*& curptr, long*& mapptr)
{
    SRVRTRACE_ENTER(FILE_IMR+17);
    memcpy(curptr, pname, sizeof(DATASOURCE_CFG_def));
    curptr += sizeof(DATASOURCE_CFG_def);

    ENV_DESC_LIST_copy(buffer,  &pname->EnvDescList, &parptr->EnvDescList, curptr, mapptr);
    RES_DESC_LIST_copy(buffer,  &pname->ResDescList, &parptr->ResDescList, curptr, mapptr);
    OCTET_copy( buffer, (OCTET_def*)&pname->DefineDescList, (OCTET_def*)&parptr->DefineDescList, curptr, mapptr);
    SRVRTRACE_EXIT(FILE_IMR+17);
}
