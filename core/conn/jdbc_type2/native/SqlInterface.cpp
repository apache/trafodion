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
// MODULE: sqlInterface.cpp
//
// PURPOSE: Implements the Srvr interface to SQL
//
/*Change Log
 * Methods Changed: ClearDiags
 */
/*Change Log
 * Methods Changed: Removed setOfCQD
 */

#include <platform_ndcs.h>
#include <sql.h>
#include <sqlext.h>
#include "SrvrCommon.h"
#include "SrvrKds.h"
#include "SqlInterface.h"
#include "SQLWrapper.h"
#include "CommonDiags.h"
#include "Debug.h"
#include "GlobalInformation.h"
#include <map>	//MFC
#include <sys/stat.h> // MFC
#include <fcntl.h>
//Added for CQDs filter

using namespace SRVR;

#if defined(TAG64)
std::map<long,SRVR_STMT_HDL*> tempStmtIdMap;
#endif

#ifndef TODO	// Linux port ToDo
extern long SQLMXStatement_int_FAILED(void);
extern long SQLMXStatement_SUCCESS_NO_INFO(void);
#endif

extern __thread Int32               sqlErrorExit[];
extern __thread short               errorIndex;

extern int SPJRS;

long AdjustCharLength(SRVR_STMT_HDL::DESC_TYPE descType, long SQLCharset, long Length)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,"AdjustCharLength",("descType=%s, SQLCharset=%s, Length=%ld",
                CliDebugDescTypeStr(descType),
                getCharsetEncoding(SQLCharset),
                Length));

    long ret_length = Length;

    switch (SQLCharset)
    {
        // SBB: The doubling for Kanji and KSC5601 needs to be removed
        // upon incorporation of MXCMP Case 10-040224-9870 into next sut (UCS2 also ?)
        case SQLCHARSETCODE_KANJI:
        case SQLCHARSETCODE_KSC5601:
        case SQLCHARSETCODE_UCS2:
            ret_length *= 2;
            break;
    }

    FUNCTION_RETURN_NUMERIC(ret_length,(NULL));
}

long MapSQLCharsetToJDBC(long SQLCharset)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE, "MapSQLCharsetToODBC",("SQLCharset=%s",
                getCharsetEncoding(SQLCharset)));

    long rc = SQLCHARSETCODE_ISO88591;

    //if locale is Japanses switch to SJIS (MBCS)
    if (PRIMARYLANGID(LANGIDFROMLCID(srvrGlobal->clientLCID)) == LANG_JAPANESE)
        rc = SQLCHARSETCODE_SJIS;

    // Unicode support
    switch (SQLCharset)
    {
        case SQLCHARSETCODE_ISO88591:
            rc = SQLCHARSETCODE_ISO88591;
            break;
        case SQLCHARSETCODE_KANJI:
            rc = SQLCHARSETCODE_KANJI;
            break;
        case SQLCHARSETCODE_KSC5601:
            rc = SQLCHARSETCODE_KSC5601;
            break;
        case SQLCHARSETCODE_SJIS:
            rc = SQLCHARSETCODE_SJIS;
            break;
        case SQLCHARSETCODE_UCS2:
            rc = SQLCHARSETCODE_UCS2;
            break;
    }

    FUNCTION_RETURN_NUMERIC(rc,("%s",getCharsetEncoding(rc)));
}

// * ***********************************************************************************
// *
// * This routine is only called by BuildSQLDesc, which is in this same file
// *
// * ***********************************************************************************

SQLRETURN GetJDBCValues( SQLItemDesc_def *SQLItemDesc,
        long &totalMemLen,
        char *ColHeading)
{
    FUNCTION_ENTRY("GetJDBCValues",
            ("DataType=%s, DateTimeCode=%ld, Length=%ld, Precision=%ld, ODBCDataType=%ld, ODBCPrecision=%ld, SignType=%ld, Nullable=%ld, totalMemLen=%ld, SQLCharset=%ld, ODBCCharset=%ld, IntLeadPrec=%ld",
             CliDebugSqlTypeCode(SQLItemDesc->dataType),
             SQLItemDesc->datetimeCode,
             SQLItemDesc->maxLen,
             SQLItemDesc->precision,
             SQLItemDesc->ODBCDataType,
             SQLItemDesc->ODBCPrecision,
             SQLItemDesc->signType,
             SQLItemDesc->nullInfo,
             totalMemLen,
             SQLItemDesc->SQLCharset,
             SQLItemDesc->ODBCCharset,
             SQLItemDesc->intLeadPrec));

    SQLItemDesc->ODBCCharset = SQLCHARSETCODE_ISO88591;

    long memAlignOffset;
    //long allocSize; 64 bit change
    int allocSize;
    getMemoryAllocInfo(	SQLItemDesc->dataType,
            SQLItemDesc->SQLCharset,
            SQLItemDesc->maxLen,
            SQLItemDesc->vc_ind_length,
            totalMemLen,
            &memAlignOffset,
            &allocSize,
            NULL);

    switch (SQLItemDesc->dataType)
    {
        case SQLTYPECODE_CHAR:
            SQLItemDesc->ODBCPrecision = SQLItemDesc->maxLen;
            SQLItemDesc->ODBCDataType  = SQL_CHAR;
            SQLItemDesc->signType      = FALSE;
            SQLItemDesc->ODBCCharset   = MapSQLCharsetToJDBC(SQLItemDesc->SQLCharset);
            break;
        case SQLTYPECODE_VARCHAR:
        case SQLTYPECODE_VARCHAR_WITH_LENGTH:
            SQLItemDesc->ODBCPrecision = SQLItemDesc->maxLen;
            SQLItemDesc->ODBCDataType  = SQL_VARCHAR;
            SQLItemDesc->signType      = FALSE;
            SQLItemDesc->ODBCCharset   = MapSQLCharsetToJDBC(SQLItemDesc->SQLCharset);
            break;
        case SQLTYPECODE_VARCHAR_LONG:
            SQLItemDesc->ODBCPrecision = SQLItemDesc->maxLen;
            SQLItemDesc->ODBCDataType  = SQL_LONGVARCHAR;
            SQLItemDesc->signType      = FALSE;
            SQLItemDesc->ODBCCharset   = MapSQLCharsetToJDBC(SQLItemDesc->SQLCharset);
            break;
        case SQLTYPECODE_SMALLINT:
            SQLItemDesc->ODBCPrecision = 5;
            SQLItemDesc->ODBCDataType  = SQL_SMALLINT;
            SQLItemDesc->signType      = TRUE;
            break;
        case SQLTYPECODE_SMALLINT_UNSIGNED:
            SQLItemDesc->ODBCPrecision = 5;
            SQLItemDesc->ODBCDataType  = SQL_SMALLINT;
            SQLItemDesc->signType      = FALSE;
            break;
        case SQLTYPECODE_INTEGER:
            SQLItemDesc->ODBCPrecision = 10;
            SQLItemDesc->ODBCDataType  = SQL_INTEGER;
            SQLItemDesc->signType      = TRUE;
            break;
        case SQLTYPECODE_INTEGER_UNSIGNED:
            SQLItemDesc->ODBCPrecision = 10;
            SQLItemDesc->ODBCDataType  = SQL_INTEGER;
            SQLItemDesc->signType      = FALSE;
            break;
        case SQLTYPECODE_LARGEINT:
            SQLItemDesc->ODBCDataType  = SQL_BIGINT;  // The LARGEINT is mapped to Types.BIGINT unless CLOB or BLOB
            SQLItemDesc->ODBCPrecision = 19;          // Make sure that the precision is set correctly
            if (ColHeading[0] != '\0')
            {
                if (strstr(ColHeading, CLOB_HEADING) != NULL)
                    SQLItemDesc->ODBCDataType = TYPE_CLOB;
                else if (strstr(ColHeading, BLOB_HEADING) != NULL)
                    SQLItemDesc->ODBCDataType = TYPE_BLOB;
            }
            SQLItemDesc->signType = TRUE;
            break;
        case SQLTYPECODE_IEEE_REAL:
        case SQLTYPECODE_TDM_REAL:
            SQLItemDesc->ODBCDataType  = SQL_REAL;
            SQLItemDesc->ODBCPrecision = 7;
            SQLItemDesc->signType      = TRUE;
            break;
        case SQLTYPECODE_IEEE_DOUBLE:
        case SQLTYPECODE_TDM_DOUBLE:
            SQLItemDesc->ODBCDataType  = SQL_DOUBLE;
            SQLItemDesc->ODBCPrecision = 15;
            SQLItemDesc->signType      = TRUE;
            break;
        case SQLTYPECODE_DATETIME:
            switch (SQLItemDesc->datetimeCode)
            {
                case SQLDTCODE_DATE:
                    SQLItemDesc->ODBCDataType = SQL_TYPE_DATE;
                    break;
                case SQLDTCODE_TIME:
                    SQLItemDesc->ODBCDataType = SQL_TYPE_TIME;
                    break;
                case SQLDTCODE_TIMESTAMP:
                    SQLItemDesc->ODBCDataType = SQL_TYPE_TIMESTAMP;
                    break;
                default:
                    SQLItemDesc->ODBCDataType = SQL_TYPE_TIMESTAMP;
                    break;
            }
            SQLItemDesc->signType      = FALSE;
            SQLItemDesc->ODBCPrecision = SQLItemDesc->precision;
            getMemoryAllocInfo(	SQLItemDesc->dataType,
                    SQLItemDesc->SQLCharset,
                    SQLItemDesc->maxLen,
                    SQLItemDesc->vc_ind_length,
                    totalMemLen,
                    &memAlignOffset,
                    &allocSize,
                    NULL);
            break;
        case SQLTYPECODE_NUMERIC:
            SQLItemDesc->ODBCDataType  = SQL_NUMERIC;
            SQLItemDesc->ODBCPrecision = SQLItemDesc->precision;
            SQLItemDesc->signType      = TRUE;
            break;
        case SQLTYPECODE_NUMERIC_UNSIGNED:
            SQLItemDesc->ODBCDataType  = SQL_NUMERIC;
            SQLItemDesc->ODBCPrecision = SQLItemDesc->precision;
            SQLItemDesc->signType      = FALSE;
            break;
        case SQLTYPECODE_DECIMAL_UNSIGNED:
            SQLItemDesc->ODBCPrecision = SQLItemDesc->maxLen;
            SQLItemDesc->ODBCDataType  = SQL_DECIMAL;
            SQLItemDesc->signType      = FALSE;
            break;
        case SQLTYPECODE_DECIMAL:
            SQLItemDesc->ODBCPrecision = SQLItemDesc->maxLen;
            SQLItemDesc->ODBCDataType  = SQL_DECIMAL;
            SQLItemDesc->signType      = TRUE;
            break;
        case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
            SQLItemDesc->ODBCDataType  = SQL_DOUBLE; // Since there is no corresponding ODBC DataType, Map it as a double
            SQLItemDesc->ODBCPrecision = 15;
            SQLItemDesc->signType      = FALSE;
            break;
        case SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
            SQLItemDesc->ODBCDataType  = SQL_DOUBLE; // Since there is no corresponding ODBC DataType, Map it as a double
            SQLItemDesc->ODBCPrecision = 15;
            SQLItemDesc->signType      = TRUE;
            break;
        case SQLTYPECODE_INTERVAL:		// Interval will be sent in ANSIVARCHAR format
            SQLItemDesc->ODBCDataType  = SQL_INTERVAL;
            SQLItemDesc->signType      = FALSE;
            SQLItemDesc->ODBCPrecision = SQLItemDesc->precision;
            // Calculate the length based on Precision and IntLeadPrec
            // The max. length is for Day to Fraction(6)
            // Day = IntLeadPrec + 1 ( 1 for Blank space)
            // Hour = 3 ( 2+1)
            // Minute = 3 (2+1)
            // Seconds = 3 (2+1)
            // Fraction = Precision
            /*SQLItemDesc->maxLen =  SQLItemDesc->intLeadPrec + 1 + 3 + 3 + 3 + SQLItemDesc->precision; //Anitha -- 10-060510-6400 */
            /* Soln No: 10-060510-6400
             * Desc:String overflow no longer occurs during evaluation
             */
            SQLItemDesc->maxLen =  SQLItemDesc->intLeadPrec + 1 + 1 + 3 + 3 + 3 + SQLItemDesc->precision;

            getMemoryAllocInfo(	SQLItemDesc->dataType,
                    SQLItemDesc->SQLCharset,
                    SQLItemDesc->maxLen,
                    SQLItemDesc->vc_ind_length,
                    totalMemLen,
                    &memAlignOffset,
                    &allocSize,
                    NULL);
            break;
        case SQLTYPECODE_BIT:
        case SQLTYPECODE_BITVAR:
        case SQLTYPECODE_IEEE_FLOAT:
        case SQLTYPECODE_FLOAT:
            // SQLTYPECODE_FLOAT is also considered an error, Since Trafodion will never
            // return SQLTYPECODE_FLOAT. It returns either SQLTYPECODE_IEEE_REAL or SQLTYPECODE_IEEE_DOUBLE
            // depending upon the precision for FLOAT fields
        case SQLTYPECODE_BPINT_UNSIGNED:
        default:
            SQLItemDesc->ODBCDataType  = SQL_TYPE_NULL;
            SQLItemDesc->ODBCPrecision = 0;
            SQLItemDesc->signType      = FALSE;
            break;
    }
    totalMemLen += memAlignOffset + allocSize;

    if (SQLItemDesc->nullInfo)
    {
        // 2-byte boundary
        totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1;
        totalMemLen += sizeof(short) ;
    }
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("Result - ODBCPrecision=%ld ODBCDataType=%ld SignType=%ld Length=%ld totalMemLen=%ld ODBCCharset=%ld",
                SQLItemDesc->ODBCPrecision,
                SQLItemDesc->ODBCDataType,
                SQLItemDesc->signType,
                SQLItemDesc->maxLen,
                totalMemLen,
                SQLItemDesc->ODBCCharset));
    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN SET_DATA_PTR(SRVR_STMT_HDL *pSrvrStmt, SRVR_STMT_HDL::DESC_TYPE descType)
{
    FUNCTION_ENTRY("SET_DATA_PTR", ("pSrvrStmt=0x%08x, descType=%s",
                pSrvrStmt,
                CliDebugDescTypeStr(descType)));

    SQLDESC_ID *pDesc = pSrvrStmt->getDesc(descType);
    SQLItemDescList_def *SQLDesc = pSrvrStmt->getDescList(descType);
    long *totalMemLen = pSrvrStmt->getDescBufferLenPtr(descType);
    BYTE **varBuffer = pSrvrStmt->getDescVarBufferPtr(descType);
    int numEntries = pSrvrStmt->getDescEntryCount(descType);
    int rowsetSize = pSrvrStmt->getRowsetSize(descType);
    struct SQLCLI_QUAD_FIELDS *quadField = pSrvrStmt->getQuadField(descType);	// Linux port - ToDo SQLCLI_QUAD_FIELDS struct is different on SQ
    SRVR_DESC_HDL *implDesc = pSrvrStmt->getImplDesc(descType);


    SQLItemDesc_def *SQLItemDesc;
    long memOffset = 0, memAlignOffset,  varLayout;
    int allocSize; //64 change
    BYTE *VarPtr;
    BYTE *IndPtr;
    BYTE *memPtr;
    unsigned long i;
    int retcode;
    BOOL sqlWarning = FALSE;
    long buffer_multiplier;
    long quadOffset = pSrvrStmt->getQuadEntryCount(descType) - numEntries;

    // Adjust totalMemLen to word boundary
    *totalMemLen = ((*totalMemLen + 8 - 1) >> 3) << 3;

    if (rowsetSize) buffer_multiplier = rowsetSize;
    else buffer_multiplier = 1;

    MEMORY_DELETE_ARRAY(*varBuffer);

    DEBUG_ASSERT(*totalMemLen!=0,("totalMemLen==0"));
    MEMORY_ALLOC_ARRAY(*varBuffer,BYTE,(*totalMemLen) * buffer_multiplier);
#ifdef _DEBUG
    memset(*varBuffer,0,(*totalMemLen) * buffer_multiplier);
#endif

    memPtr = *varBuffer ;
    memOffset = 0;
    for (i = 0 ; i < SQLDesc->_length ; i++)
    {
        SQLItemDesc = (SQLItemDesc_def *) SQLDesc->_buffer + i;

        getMemoryAllocInfo(SQLItemDesc->dataType, SQLItemDesc->SQLCharset, SQLItemDesc->maxLen, SQLItemDesc->vc_ind_length, memOffset,
                &memAlignOffset, &allocSize, &varLayout);

        if ((SQLItemDesc->dataType==SQLTYPECODE_INTERVAL) ||
                (SQLItemDesc->dataType==SQLTYPECODE_DATETIME))
        {
            retcode = CLI_SetDescItem(pDesc,
                    i+1,
                    SQLDESC_TYPE,
                    (long)SQLTYPECODE_VARCHAR_WITH_LENGTH,
                    NULL);
            HANDLE_ERROR(retcode, sqlWarning);
            retcode = CLI_SetDescItem(pDesc,
                    i+1,
                    SQLDESC_LENGTH,
                    SQLItemDesc->maxLen,
                    NULL);
            HANDLE_ERROR(retcode, sqlWarning);
            retcode = CLI_SetDescItem(pDesc,
                    i+1,
                    SQLDESC_VC_IND_LENGTH,
                    (long)sizeof(short),
                    NULL);
            HANDLE_ERROR(retcode, sqlWarning);
        }

        VarPtr = memPtr + memOffset + memAlignOffset;

        DEBUG_OUT(DEBUG_LEVEL_DATA,("Desc %d dataType=%s, VarPtr=0x%08x, memAlignOffset=%ld",
                    i,
                    CliDebugSqlTypeCode(SQLItemDesc->dataType),
                    VarPtr,
                    memAlignOffset));

        memOffset += memAlignOffset + allocSize * buffer_multiplier;

        if (SQLItemDesc->nullInfo)
        {
            memAlignOffset = (((memOffset + 2 - 1) >> 1) << 1) - memOffset;
            IndPtr = memPtr + memOffset + memAlignOffset;
            DEBUG_OUT(DEBUG_LEVEL_DATA,("IndPtr=0x%08x, memAlignOffset=%ld",
                        IndPtr,
                        memAlignOffset));
            memOffset += memAlignOffset + sizeof(short) * buffer_multiplier;
        } else IndPtr = NULL;

        if (rowsetSize==0)
        {
            retcode = CLI_SetDescItem(pDesc,
                    i+1,
                    SQLDESC_VAR_PTR,
                    (long)VarPtr,
                    NULL);
            HANDLE_ERROR(retcode, sqlWarning);
            retcode = CLI_SetDescItem(pDesc,
                    i+1,
                    SQLDESC_IND_PTR,
                    (long)IndPtr,
                    NULL);
            HANDLE_ERROR(retcode, sqlWarning);
        } else {
            quadField[i+quadOffset].var_layout = varLayout;
            quadField[i+quadOffset].var_ptr = (BYTE *) VarPtr;
            quadField[i+quadOffset].ind_ptr = (BYTE *) IndPtr;
            if (IndPtr) quadField[i+quadOffset].ind_layout = sizeof(short);
            else quadField[i+quadOffset].ind_layout = 0;
        }

        implDesc[i].varPtr = VarPtr;
        implDesc[i].indPtr = IndPtr;

        if (memOffset > (*totalMemLen * buffer_multiplier))
        {
            DEBUG_OUT(DEBUG_LEVEL_ENTRY,("memOffset (%ld) > totalMemLen (%ld) * buffer_multiplier(%ld)",
                        memOffset,
                        *totalMemLen,
                        buffer_multiplier));
            CLI_DEBUG_RETURN_SQL(PROGRAM_ERROR);
        }
    }

    if (rowsetSize)
    {
        // Must set rowset entry var_layout to zero for set
        if (descType==SRVR_STMT_HDL::Input) quadField[0].var_layout = 0;
        // Setup descriptors for rowset processing
        int rowset_status;
        retcode = CLI_SETROWSETDESCPOINTERS(
                pDesc,
                rowsetSize,
                &rowset_status,
                1L,
                numEntries,
                quadField);
        HANDLE_ERROR(retcode, sqlWarning);
    }

    if (sqlWarning) CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN AllocAssignValueBuffer(SQLItemDescList_def *SQLDesc,  SQLValueList_def *SQLValueList,
        long totalMemLen, 	long maxRowCount, BYTE *&VarBuffer)
{
    FUNCTION_ENTRY("AllocAssignValueBuffer",
            ("SQLDesc=0x%08x, SQLValueList=0x%08x, totalMemLen=%ld, maxRowCount=%ld, VarBuffer=0x%08x",
             SQLDesc,
             SQLValueList,
             totalMemLen,
             maxRowCount,
             VarBuffer));


    SQLItemDesc_def *SQLItemDesc;
    SQLValue_def *SQLValue;
    long memOffset = 0;
    BYTE *VarPtr;
    BYTE *IndPtr;
    BYTE *memPtr;
    unsigned long curValueCount, curDescCount;
    long curRowCount;
    long numValues;
    //long AllocLength; 64 change
    int AllocLength;
    long totalRowMemLen;

    // Allocate SQLValue Array
    MEMORY_DELETE_ARRAY(SQLValueList->_buffer);
    MEMORY_DELETE_ARRAY(VarBuffer);
    numValues = SQLDesc->_length * maxRowCount;
    if (numValues == 0)
    {
        SQLValueList->_buffer = NULL;
        SQLValueList->_length = 0;
        VarBuffer = NULL;
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
    }
    else
    {
        MEMORY_ALLOC_ARRAY(SQLValueList->_buffer, SQLValue_def, numValues);
        SQLValueList->_length = 0;
    }


    // Allocate the Value Buffer
    totalRowMemLen = totalMemLen * maxRowCount;
    MEMORY_ALLOC_ARRAY(VarBuffer, BYTE, totalRowMemLen);
    memPtr = VarBuffer ;
    memOffset = 0;

    for (curRowCount = 0, curValueCount = 0 ; curRowCount < maxRowCount ; curRowCount++)
    {
        for (curDescCount = 0 ; curDescCount < SQLDesc->_length ; curDescCount++, curValueCount++)
        {
            long memAlignOffset;
            SQLItemDesc = (SQLItemDesc_def *)SQLDesc->_buffer + curDescCount;
            SQLValue  = (SQLValue_def *)SQLValueList->_buffer + curValueCount;

            getMemoryAllocInfo(SQLItemDesc->dataType, SQLItemDesc->SQLCharset, SQLItemDesc->maxLen, SQLItemDesc->vc_ind_length, memOffset,
                    &memAlignOffset, &AllocLength, NULL);

            VarPtr = memPtr + memOffset + memAlignOffset;
            memOffset += memAlignOffset + AllocLength;

            if (SQLItemDesc->nullInfo)
            {
                memOffset = ((memOffset + 2 - 1) >> 1) << 1;
                IndPtr = memPtr + memOffset;
                memOffset += sizeof(short) ;
            }
            else
                IndPtr = NULL;

            SQLValue->dataValue._buffer = (unsigned char *) VarPtr;
            // Ignore the indPtr, since it is declared as short already in SQLValue_def
            SQLValue->dataType = SQLItemDesc->dataType;
            SQLValue->dataValue._length = AllocLength;
            SQLValue->dataCharset = SQLItemDesc->ODBCCharset;
            DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLValue: dataValue._buffer=0x%08x, dataType=%s, dataValue._length=%ld, dataCharset=%s",
                        SQLValue->dataValue._buffer,
                        CliDebugSqlTypeCode(SQLValue->dataType),
                        SQLValue->dataValue._length,
                        getCharsetEncoding(SQLValue->dataCharset)));

            if (memOffset > totalRowMemLen)
            {
                //TFDS
                DEBUG_OUT(DEBUG_LEVEL_ENTRY,("memOffset > totalRowMemLen"));
                CLI_DEBUG_RETURN_SQL(PROGRAM_ERROR);
            }
        }
        // Align it to next word boundary for the next row
        memOffset = ((memOffset + 8 - 1) >> 3) << 3;
    }
    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN CopyValueList(SQLValueList_def *outValueList, const SQLValueList_def *inValueList)
{
    FUNCTION_ENTRY("CopyValueList",
            ("outValueList=0x%08x, inValueList=0x%08x",
             outValueList,
             inValueList));

    SQLValue_def *inValue;
    SQLValue_def *outValue;
    unsigned long i;

    for (i = 0; i < inValueList->_length ; i++)
    {
        inValue = (SQLValue_def *)inValueList->_buffer + i;
        outValue = (SQLValue_def *)outValueList->_buffer + i;

        if (inValue->dataType != outValue->dataType)
        {
            DEBUG_OUT(DEBUG_LEVEL_ENTRY,("inValue->dataType != outValue->dataType"));
            CLI_DEBUG_RETURN_SQL(PROGRAM_ERROR);
        }
        outValue->dataInd = inValue->dataInd;
        if (inValue->dataInd == 0)
        {
            if (inValue->dataValue._length != outValue->dataValue._length)
            {
                // TFDS
                DEBUG_OUT(DEBUG_LEVEL_ENTRY,("inValue->dataValue._length != outValue->dataValue._length"));
                CLI_DEBUG_RETURN_SQL(PROGRAM_ERROR);
            }
            memcpy(outValue->dataValue._buffer, inValue->dataValue._buffer, outValue->dataValue._length);
        }
        else
        {
            outValue->dataValue._length = 0;
        }
    }
    outValueList->_length = inValueList->_length;
    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN BuildSQLDesc(SRVR_STMT_HDL*pSrvrStmt, SRVR_STMT_HDL::DESC_TYPE descType)
{
    FUNCTION_ENTRY("BuildSQLDesc", ("pSrvrStmt=0x%08x, pSrvrStmt->stmtName = %s, SQL Statement = %s, descType=%s",
                pSrvrStmt,
                pSrvrStmt->stmtName,
                pSrvrStmt->sqlString.dataValue._buffer,
                CliDebugDescTypeStr(descType)));

    long retcode = SQL_SUCCESS;
    short i;
    short j;
    short k;

    BOOL sqlWarning = FALSE;

    long *totalMemLen = pSrvrStmt->getDescBufferLenPtr(descType);
    long numEntries = pSrvrStmt->getDescEntryCount(descType);
    SQLDESC_ID *pDesc = pSrvrStmt->getDesc(descType);
    SQLItemDescList_def *SQLDesc = pSrvrStmt->getDescList(descType);
    SQLItemDesc_def *SQLItemDesc = (SQLItemDesc_def *)SQLDesc->_buffer + SQLDesc->_length;

    SRVR_DESC_HDL *implDesc = pSrvrStmt->allocImplDesc(descType);

    // The following routine is hard coded for at least 15 items, so make sure it does not change
    DEBUG_ASSERT(NO_OF_DESC_ITEMS>=15,("NO_OF_DESC_ITEMS(%d) is less than 15",NO_OF_DESC_ITEMS));
    *totalMemLen = 0;
    for (i = 0; i < numEntries; i++) {
        SQLItemDesc = (SQLItemDesc_def *)SQLDesc->_buffer + SQLDesc->_length;
        // Initialize the desc entry in SQLDESC_ITEM struct
        for (j = 0; j < NO_OF_DESC_ITEMS ; j++) {
            gDescItems[j].entry = i+1;
        }
        gDescItems[10].num_val_or_len = MAX_ANSI_NAME_LEN+1;
        gDescItems[11].num_val_or_len = MAX_ANSI_NAME_LEN+1;
        gDescItems[12].num_val_or_len = MAX_ANSI_NAME_LEN+1;
        gDescItems[13].num_val_or_len = MAX_ANSI_NAME_LEN+1;
        gDescItems[14].num_val_or_len = MAX_ANSI_NAME_LEN+1;

        retcode = CLI_GetDescItems2(pDesc,
                NO_OF_DESC_ITEMS,
                (SQLDESC_ITEM *)&gDescItems);
        HANDLE_ERROR(retcode, sqlWarning);

        SQLItemDesc->dataType     = gDescItems[0].num_val_or_len;
        SQLItemDesc->maxLen       = gDescItems[1].num_val_or_len;
        SQLItemDesc->precision    = (short)gDescItems[2].num_val_or_len;
        SQLItemDesc->scale        = (short)gDescItems[3].num_val_or_len;
        SQLItemDesc->nullInfo     = (BOOL)gDescItems[4].num_val_or_len;
        SQLItemDesc->paramMode    = gDescItems[5].num_val_or_len;
        SQLItemDesc->intLeadPrec  = gDescItems[6].num_val_or_len;
        SQLItemDesc->datetimeCode = gDescItems[7].num_val_or_len;
        SQLItemDesc->SQLCharset   = gDescItems[8].num_val_or_len;
        SQLItemDesc->fsDataType   = gDescItems[9].num_val_or_len;
        for (k = 10; k < 15; k++) {
            gDescItems[k].string_val[gDescItems[k].num_val_or_len] = '\0';
        }
        SQLItemDesc->vc_ind_length = gDescItems[15].num_val_or_len;

        SQLItemDesc->maxLen = AdjustCharLength(descType, SQLItemDesc->SQLCharset, SQLItemDesc->maxLen);

        GetJDBCValues(	SQLItemDesc, 			// Input
                *totalMemLen,
                gDescItems[14].string_val);

        implDesc[i].charSet         = SQLItemDesc->SQLCharset;
        implDesc[i].dataType        = SQLItemDesc->dataType;
        implDesc[i].length          = SQLItemDesc->maxLen;
        implDesc[i].precision       = SQLItemDesc->ODBCPrecision;
        implDesc[i].scale           = SQLItemDesc->scale;
        implDesc[i].sqlDatetimeCode = SQLItemDesc->datetimeCode;
        implDesc[i].FSDataType      = SQLItemDesc->fsDataType;
        implDesc[i].paramMode       = SQLItemDesc->paramMode;
        implDesc[i].vc_ind_length   = SQLItemDesc->vc_ind_length;

        SQLItemDesc->version = 0;

        strcpy(SQLItemDesc->CatalogName, gDescItems[10].string_val);
        strcpy(SQLItemDesc->SchemaName, gDescItems[11].string_val);
        strcpy(SQLItemDesc->TableName, gDescItems[12].string_val);
        strcpy(SQLItemDesc->ColumnName, gDescItems[13].string_val);
        strcpy(SQLItemDesc->ColumnLabel, gDescItems[14].string_val);

        SQLDesc->_length++;

    }

    retcode = SET_DATA_PTR(pSrvrStmt, descType);
    HANDLE_ERROR(retcode, sqlWarning);

    if (sqlWarning) CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

/* ***********************************************************************************************
 * FUNCTION: RSgetRSmax
 *
 * DESCRIPTION:
 * This routine will call SQL_EXEC_GetStmtAttr() to get the maximum number
 * of RS's the stmt can rtn. {Note: This value corrsponds to the value of
 * "DYNAMIC RESULT SETS" specified during the CREATE PROCEDURE registration.
 *
 * ARGUMENTS:
 *     INPUT/OUTPUT:
 *			pSrvrStmt			- A C++ object that contains information about the
 *								statement being used.
 *     OUTPUT:
 *			pSrvrStmt->RSMax	- Max number of SPJRS allowed for this stmt.
 * Returns:
 *	SQL_SUCCESS or
 *    SQL failures (see CLI_GetStmtAttr() retcode failures).
 *********************************************************************************************** */
SQLRETURN RSgetRSmax(SRVR_STMT_HDL *pSrvrStmt)
{
    FUNCTION_ENTRY("RSgetRSmax",
            ("pSrvrStmt=0x%08x",pSrvrStmt));

    CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

    BOOL			sqlWarning = FALSE;
    SQLSTMT_ID		*pStmt = &pSrvrStmt->stmt;
    int			retcode = SQL_SUCCESS;
    int			max_num_rs = 0;

    pSrvrStmt->RSMax = 0;

    // Get RSMax (max SPJ Result Sets that can be returned for this stmt)
    retcode = CLI_GetStmtAttr((SQLSTMT_ID *)pStmt,			// (IN) SQL statement ID
            SQL_ATTR_MAX_RESULT_SETS,	// (IN) Request query statement attribute (max RS per stmt)
            &max_num_rs,				// (OUT) Place to store Max resultsets
            NULL,						// (OUT) Optional string
            0,							// (IN) Max size of optional string buffer
            NULL );						// (IN) Length of item

    // Handle CLI errors
    if (retcode != SQL_SUCCESS)
    {
        // CLI_ClearDiagnostics(NULL);
        DEBUG_OUT(DEBUG_LEVEL_CLI|DEBUG_LEVEL_STMT,
                ("CLI_GetStmtAttr(SQL_ATTR_MAX_RESULT_SETS) FAILED : retcode=%s",
                 CliDebugSqlError(retcode)));
        CLI_DEBUG_RETURN_SQL(retcode);
    }

    pSrvrStmt->RSMax = max_num_rs;

    DEBUG_ASSERT((pSrvrStmt->RSMax>0)&&(pSrvrStmt->RSMax<256),
            ("Max ResultSets(%ld) is out of range", max_num_rs));
    // Point to first RS if greater than 0
    if(pSrvrStmt->RSMax > 0)
        pSrvrStmt->RSIndex = 1;

    // Also should fail if value is less than one or greater than 255
    DEBUG_OUT(DEBUG_LEVEL_STMT,("RSMax: %d  RSIndex: %d  isSPJRS: %d",
                pSrvrStmt->RSMax, pSrvrStmt->RSIndex, pSrvrStmt->isSPJRS));

    CLI_DEBUG_RETURN_SQL(retcode);
}

/* ***********************************************************************************************
 * FUNCTION: ReadRow
 *
 * DESCRIPTION: Called during int or a fetch.
 * This routine will retrieve information from a descriptor entry.
 * Length of retrieved items is returned in totalRows.
 * Copy items to the SQL output value list
 * via the kdsCopyToSQLValueSeq() routine.
 *
 * ARGUMENTS:
 *     INPUT/OUTPUT:
 *			pSrvrStmt - A C++ object that contains information about the statement being
 *                               used.
 *     OUTPUT:
 *			totalRows - running total of rows read.
 *			rowsRead  - rows read by last fetch
 * Returns:
 *	SQL_SUCCESS or
 *    SQL failures (see CLI_GetDescItem() for retcode failures).
 *********************************************************************************************** */
static SQLRETURN ReadRow(SRVR_STMT_HDL *pSrvrStmt,
        int *totalRows, long *rowsRead)
{
    FUNCTION_ENTRY("ReadRow",
            ("pSrvrStmt=0x%08x, totalRows=%ld, rowsRead=0x%08x",
             pSrvrStmt,
             *totalRows,
             rowsRead));

    CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

    long	retcode = SQL_SUCCESS;			// assume success
    //long	allocLength;					// allocation length 64 change
    int		allocLength;					// allocation length
    long	charSet;
    long	columnCount = 0;				// total number of columns
    long	curColumnNo = 0;				// current column number
    long	dataLength;
    long	dataType;
    short	*indPtr;						// Linux port - ToDo: this is declared as Int64 in SQ ???
    short	indValue;
    BYTE	*pBytes;

    SQLDESC_ID		*pOutputDesc;
    SQLValueList_def *pOutputValueList;
    BOOL sqlWarning = FALSE;
    SRVR_DESC_HDL	*IRD;

    pOutputValueList = &pSrvrStmt->outputValueList;
    pOutputDesc = &pSrvrStmt->outputDesc;
    columnCount = pSrvrStmt->columnCount;
    IRD = pSrvrStmt->IRD;

    DEBUG_OUT(DEBUG_LEVEL_DATA,("pOutputValueList=0x%08x",
                pOutputValueList));

    // Assert that outputValueList buffer is not null, if null assert
    DEBUG_ASSERT(pOutputValueList->_buffer!=NULL,("pOutputValueList->_buffer==NULL"));
    // This check may be needed; otherwise, a SIG11 occurs from calling kdsCopyToSqlValueSeq()
    // if the buffer is null.  This has been found to occur when JVM memory is exhausted.
    // if (pOutputValueList->_buffer == NULL) CLI_DEBUG_RETURN_SQL(SQL_NULL_DATA);

    if (pSrvrStmt->fetchRowsetSize > 0)
    {
        // This is a Rowset fetch
        int item_count = 0;
        long row;
        retcode = CLI_GetDescItem(pOutputDesc,
                1L,
                SQLDESC_ROWSET_NUM_PROCESSED,
                &item_count,
                NULL,0,NULL,0);

        if (retcode != SQL_SUCCESS) CLI_DEBUG_RETURN_SQL(retcode);

        // Assert that colCnt equals fetch QuadEntries, if not equal assert
        DEBUG_ASSERT(columnCount==pSrvrStmt->fetchQuadEntries,
                ("columnCount(%ld)!=pSrvrStmt->fetchQuadEntries(%ld)",
                 columnCount,
                 pSrvrStmt->fetchQuadEntries));
        DEBUG_OUT(DEBUG_LEVEL_DATA,("Rowset rowsRead=%ld",item_count));

        if (rowsRead) *rowsRead = item_count;
        if (item_count==0) CLI_DEBUG_RETURN_SQL(retcode);

        for (row=0; row<item_count; row++)
        {
            for (curColumnNo = 0; curColumnNo < columnCount; curColumnNo++)
            {
                dataType = IRD[curColumnNo].dataType;
                indPtr = (short *) pSrvrStmt->fetchQuadField[curColumnNo].ind_ptr;
                if ((indPtr == NULL) || (indPtr[row] != -1)) indValue = 0;
                else indValue = -1;
                charSet = IRD[curColumnNo].charSet;
                dataLength = IRD[curColumnNo].length;
                getMemoryAllocInfo(dataType,
                        charSet,
                        dataLength,
                        IRD[curColumnNo].vc_ind_length,
                        0,
                        NULL,
                        &allocLength,
                        NULL);
                pBytes = (BYTE *)(pSrvrStmt->fetchQuadField[curColumnNo].var_ptr);

                if (charSet != SQLCHARSETCODE_ISO88591 && dataType == SQLTYPECODE_VARCHAR_WITH_LENGTH)
                    pBytes += row * (allocLength - 1);
                else
                    pBytes += row * allocLength;

                DEBUG_OUT(DEBUG_LEVEL_DATA,("RowSet Row %ld Column %ld pBytes=0x%08x item_count=%ld",
                            row,
                            curColumnNo,
                            pBytes,
                            item_count));
                MEMORY_DUMP(DEBUG_LEVEL_DATA,pBytes,allocLength);
                kdsCopyToSQLValueSeq(pOutputValueList,
                        dataType,
                        indValue,
                        pBytes,
                        allocLength,
                        charSet);
            }
        }
        *totalRows += item_count;
    }
    else
    {
        // This is not a Rowset fetch
        *totalRows += 1;
        if (rowsRead) *rowsRead = 1;

        for (curColumnNo = 0; curColumnNo < columnCount; curColumnNo++)
        {
            dataType = IRD[curColumnNo].dataType;
            indPtr = (short *) IRD[curColumnNo].indPtr;
            if ((indPtr == NULL) || (indPtr[0] != -1)) indValue = 0;
            else indValue = -1;
            charSet = IRD[curColumnNo].charSet;
            pBytes = (BYTE *)(IRD[curColumnNo].varPtr);
            dataLength = IRD[curColumnNo].length;

            // Compute memory allocation requirements for descriptor
            getMemoryAllocInfo(dataType,
                    charSet,
                    dataLength,
                    IRD[curColumnNo].vc_ind_length,
                    0,
                    NULL,
                    &allocLength,
                    NULL);
            DEBUG_OUT(DEBUG_LEVEL_DATA,("totalRows %ld curColumnNo %ld",*totalRows,curColumnNo));
            MEMORY_DUMP(DEBUG_LEVEL_DATA,pBytes,allocLength);
            kdsCopyToSQLValueSeq(pOutputValueList, dataType, indValue, pBytes, allocLength, charSet);
        }
    }

    DEBUG_OUT(DEBUG_LEVEL_DATA,("totalRows=%ld curColumnNo=%ld pOutputValueList=0x%08x",
                *totalRows, curColumnNo, pOutputValueList));
    CLI_DEBUG_RETURN_SQL((SQLRETURN)retcode);
}

/* ***********************************************************************************************
 * FUNCTION: EXECUTE
 *
 * DESCRIPTION: Evaluate input parameters and execute an SQL statement, make diagnostic
 *              information available and, if results are present, position the implicit cursor
 *              before the first row of the result set.
 *
 *			For V2.1+ SQL their are four classes of Trafodion query stmt types:
 *			1) Unique_selects (1 row) call ClearExecFetchClose() SQL/CLI (w/ a valid outputDesc).
 *			2) Non_unique selects (multiple rows) statement types call Trafodion CLI Exec (not chg'd from V2.0 SQL).
 *			3) UID (unique and non_unique) stmt types call SQL/CLI ClearExecFetchClose() (w/ a NULL outputDesc).
 *			4) All other statements (SQL_CONTROL, SQL_SET_TRANSACTION, SQL_SET_CATALOG, SQL_SET_SCHEMA)
 *			call SQL/CLI ClearExecFetchClose() (w/ a NULL outputDesc).
 *
 *
 * ARGUMENTS:
 *     INPUT: pSrvrStmt->inputRowCnt - When rowsets is used, this is the maximum number of rows to be fetched
 *                                     when using the ExecFetch CLI call.
 *            pSrvrStmt->sqlStmtType - Used with V2.0 SQL to indicate a select and non-select statement type.
 *
 *     OUTPUT: pSrvrStmt->rowCount - The number of rows affected by the Exec or ClearExec/Fetch/Close.
 *
 *********************************************************************************************** */
SQLRETURN EXECUTE(SRVR_STMT_HDL* pSrvrStmt)
{
    FUNCTION_ENTRY("EXECUTE",
            ("pSrvrStmt=0x%08x",
             pSrvrStmt));

    CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

    SRVR_CONNECT_HDL *pConnect = NULL;
    if(pSrvrStmt->dialogueId == 0) CLI_DEBUG_RETURN_SQL(SQL_ERROR);
    pConnect = (SRVR_CONNECT_HDL*)pSrvrStmt->dialogueId;

    BOOL isSPJRS = false;
    long retcode = SQL_SUCCESS;
    SQLDESC_ID *pInputDesc;
    SQLDESC_ID *pOutputDesc;
    SQLSTMT_ID *pStmt;
    SQLSTMT_ID cursorId;
    long		paramCount;
    char		*cursorName;
    long		len;
    long		curParamNo;
    long		curLength;
    BYTE		*varPtr;
    BYTE		*indPtr;
    void			*pBytes;
    SQLValue_def	*SQLValue;
    BOOL			sqlWarning = FALSE;
    int				rtn = 0;
    pSrvrStmt->isSPJRS = FALSE;
    pStmt = &pSrvrStmt->stmt;
    pSrvrStmt->endOfData = FALSE;		// true=return code 100 from CLI call (no data found)
    if ((pSrvrStmt->stmtType == EXTERNAL_STMT) &&
            (pSrvrStmt->moduleId.module_name == NULL))
    {
        // Conditions to check cursor
        // SQL 2.0 Chk cursor if version==SQL 2.0 AND SQL Stmt Type==TYPE_SELECT.
        // SQL 2.1+ Chk cursor if SQL Stmt query type is a non_unique select.
        if ((pSrvrStmt->getSqlQueryStatementType() == SQL_SELECT_NON_UNIQUE) ||
                ((GlobalInformation::getSQLMX_Version() == CLI_VERSION_R2) && (pSrvrStmt->sqlStmtType & TYPE_SELECT)))
        {
            cursorName = pSrvrStmt->cursorName;
            // If cursor name is not specified, use the stmt name as cursor name
            // due to bug in Trafodion, though it should default automatically
            if (*cursorName == '\0')
                cursorName = pSrvrStmt->stmtName;

            DEBUG_OUT(DEBUG_LEVEL_CLI,("cursorName='%s', previousCursorName='%s'",
                        cursorName,
                        pSrvrStmt->previousCursorName));

            // If cursorName has chg'd from last EXEC or EXECDIRECT cmd
            // or has not yet been set for the first time call SetCursorName
            if ((strcmp(pSrvrStmt->previousCursorName, cursorName) != 0) &&
                    (*cursorName != '\0'))
            {
                cursorId.version        = SQLCLI_ODBC_VERSION;
                cursorId.module         = pStmt->module;
                cursorId.handle         = 0;
                cursorId.charset        = SQLCHARSETSTRING_ISO88591;
                cursorId.name_mode      = cursor_name;
                cursorId.identifier_len = strlen(cursorName);
                cursorId.identifier     = cursorName;
                strcpy(pSrvrStmt->previousCursorName, cursorName);	// keep track of last cursorName
                DEBUG_OUT(DEBUG_LEVEL_CLI,("EXECUTE cursorName %s previousCursorName %s",
                            cursorName,
                            pSrvrStmt->previousCursorName));

                retcode = CLI_SetCursorName(pStmt, &cursorId);
                HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
            }
        }
    }

    pInputDesc  = &pSrvrStmt->inputDesc;		// Input descriptor pointer
    pOutputDesc = &pSrvrStmt->outputDesc;		// Output descriptor pointer. Used in ClearExecFetchClose()
    paramCount  = pSrvrStmt->paramCount;

    if(GlobalInformation::getSQLMX_Version() == CLI_VERSION_R2)
    {
        pSrvrStmt->isClosed = FALSE;		// Assume we are open after EXEC_Exec

        if (pSrvrStmt->sqlStmtType & TYPE_SELECT) retcode = CLI_Exec(pStmt, pInputDesc, 0);
        else retcode = CLI_ExecFetch(pStmt, pInputDesc, 0);
    }
    else				// VER 2.1
    {
        // Should be Closed. If Opened assert
        DEBUG_ASSERT(pSrvrStmt->isClosed, ("Server Statement is Open before execute."));
        pSrvrStmt->isClosed = FALSE;		// Assume we are open after EXEC_Exec
        switch (pSrvrStmt->getSqlQueryStatementType())
        {
            case SQL_SELECT_UNIQUE:
                DEBUG_OUT(DEBUG_LEVEL_CLI,("Unique SELECT statement type."));
                if(pConnect->isSPJRS)
                    isSPJRS = true;
                if(!isSPJRS) {
                    // ClearExecFetchClose performs exec, fetch, and close
                    retcode = CLI_ClearExecFetchClose(pStmt, pInputDesc, pOutputDesc, 0, 0, 0);
                    // ClearExecFetchClose() does an implicit close
                    pSrvrStmt->isClosed = TRUE;
                }
                else {
                    //For SPJRS SELECT UNIQUE
                    //Using Exec,fetch separately for SPJRS Configuration to avoid statement that is in the closed state.
                    retcode = CLI_Exec(pStmt, pInputDesc, 0);
                    if (retcode != 0) CLI_ClearDiagnostics(pStmt);
                    else {
                        retcode = CLI_Fetch(pStmt, pOutputDesc, 0);
                        if (retcode != 0) {
                            CLI_ClearDiagnostics(pStmt);
                            pSrvrStmt->endOfData = TRUE; //SPJRS
                        }
                    }
                }
                break;

            case SQL_SELECT_NON_UNIQUE:
                DEBUG_OUT(DEBUG_LEVEL_CLI,("Non-Unique SELECT statement type."));
                retcode = CLI_Exec(pStmt, pInputDesc, 0);
                break;

                // IUD (Insert, Update, Delete) unique and non-unique Statement Types
            case SQL_CONTROL:
            case SQL_SET_TRANSACTION:
            case SQL_SET_CATALOG:
            case SQL_SET_SCHEMA:
                DEBUG_OUT(DEBUG_LEVEL_CLI,("Control or Set Connection Attribute statement type."));
            case SQL_INSERT_UNIQUE:
            case SQL_UPDATE_UNIQUE:
            case SQL_DELETE_UNIQUE:
                // SQL_OTHER (DDLs - create/alter/drop/etc) should behave like IUD non-unique Statement Types
            case SQL_OTHER:
            case SQL_INSERT_NON_UNIQUE:
            case SQL_UPDATE_NON_UNIQUE:
            case SQL_DELETE_NON_UNIQUE:
            default:
                DEBUG_OUT(DEBUG_LEVEL_CLI,("IUD (Non-Unique and Unique) and control statement types."));
                // ClearExecFetchClose performs exec, fetch, and close (implicit close)
                // Output Desc = NULL
                retcode = CLI_ClearExecFetchClose(pStmt, pInputDesc, NULL, 0, 0, 0);

                // ClearExecFetchClose() does an implicit close
                pSrvrStmt->isClosed = TRUE;
                break;
        } // End of stmt type switch
    } // End of VER20 check

#ifndef DISABLE_NOWAIT		
    if (retcode == NOWAIT_PENDING)
    {
        rtn = WaitForCompletion(pSrvrStmt, &pSrvrStmt->cond, &pSrvrStmt->mutex);
        DEBUG_OUT(DEBUG_LEVEL_CLI,("WaitForCompletion() returned %d",rtn));

        if (rtn == 0)
        {
            SQLRETURN rc = pSrvrStmt->switchContext();
            DEBUG_OUT(DEBUG_LEVEL_CLI,("pSrvrStmt->switchContext() returned %ld", rc));
            if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) THREAD_RETURN(pSrvrStmt,rc);

            switch (pSrvrStmt->nowaitRetcode)
            {
                case 0:		// Wait Success
                    // If not closed, try closing and clear out diag's
                    if(!pSrvrStmt->isClosed)
                    {
                        retcode = CLI_CloseStmt(pStmt);
                        pSrvrStmt->isClosed = TRUE;
                        if (retcode != 0) CLI_ClearDiagnostics(pStmt);
                    }
                    retcode = 0;
                    break;
                case 9999:	// Wait error
                    pSrvrStmt->isClosed = TRUE;
                    THREAD_RETURN(pSrvrStmt,NOWAIT_ERROR);
                default:	// All other errors
                    pSrvrStmt->isClosed = TRUE;
                    retcode = GETSQLCODE(pSrvrStmt);
                    break;
            }
            DEBUG_OUT(DEBUG_LEVEL_CLI,("pSrvrStmt->nowaitRetcode=%ld, retcode=%s",
                        pSrvrStmt->nowaitRetcode,
                        CliDebugSqlError(retcode)));
        }
        else
        {
            // If waitForCompletion() was not successful (rtn != 0)
            pSrvrStmt->isClosed = TRUE;
            pSrvrStmt->nowaitRetcode = rtn;
            THREAD_RETURN(pSrvrStmt,NOWAIT_ERROR);
        }
    }
    else
#endif	
    {
        if (retcode!=SQL_SUCCESS) pSrvrStmt->isClosed = TRUE;
    }
    if (srvrGlobal->moduleCaching)
    {
        if ( (retcode == 8579) || (retcode == 8578) || (retcode == -1004) || (retcode == -4082) )
        {
            //If a module file exists and there is a DDL modification in the
            //table, we handle it here and return the error to client.
            std::string strModuleName = pSrvrStmt->moduleName;
            if(strModuleName.find("T2MFC") != -1)
            {
                pConnect->removeFromLoadedModuleSet(strModuleName);
                remove(strModuleName.c_str()); // removing the Module file
            }
        }
    }

    // Process the SQL CLI return code
    if (retcode != 0){						// SQL success
        if (retcode == 100) {				// No Data Found
            CLI_ClearDiagnostics(pStmt);
            retcode = 0;
            pSrvrStmt->endOfData = TRUE;	// indicates no data found using ClearExecFetchClose()
        } else if (retcode < 0) {			// SQL Error
            THREAD_RETURN(pSrvrStmt,SQL_ERROR);
        } else {							// > 0, SQL Warning.
            sqlWarning = TRUE;
        }
    }

    HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

    if(GlobalInformation::getSQLMX_Version() == CLI_VERSION_R2)
    {
        // Not a SELECT stmt type
        if ((pSrvrStmt->sqlStmtType & TYPE_SELECT) == 0)
        {
            Int64 tmpRowCount;
            // Get row count
            retcode =  CLI_GetDiagnosticsStmtInfo2 (pStmt, SQLDIAG_ROW_COUNT,
                    &tmpRowCount,
                    NULL, 0, NULL);
            if (retcode == SQL_SUCCESS)
                pSrvrStmt->rowCount._buffer[0] = (int)tmpRowCount;
            else
                pSrvrStmt->rowCount._buffer[0] = 0;
            pSrvrStmt->totalRowCount += pSrvrStmt->rowCount._buffer[0];
            // Batch Binding Size
            if (pSrvrStmt->batchRowsetSize)
            {
                // Currently we only get one value back from SQL for rowsets.
                // We will fill in all values with SUCCESS_NO_INFO and ignore the
                //   one value until SQL updates to return the actual number.
                long row_idx;
#ifndef TODO	// Linux port Todo - Currently hardcoding actual value
                long info_value = SQLMXStatement_SUCCESS_NO_INFO();
#else
                long info_value = -2L;
#endif				
                for (row_idx=0; row_idx<pSrvrStmt->inputRowCnt; row_idx++)
                {
                    pSrvrStmt->rowCount._buffer[pSrvrStmt->rowCount._length+row_idx] = info_value;
                }
            }
            if (retcode < 0) sqlWarning = TRUE;
            else pSrvrStmt->rowCount._length += pSrvrStmt->inputRowCnt;
        }
    }
    else {				// VER 2.1 or later

        // Linux port- moving variable declarations here from case blocks because of compile errors
        long rows_read = 0;
        long row_idx;
        long info_value;

        switch (pSrvrStmt->getSqlQueryStatementType())
        {
            // CLI will not return unique stmt types for rowsets
            // Rowset processing will not be a unique command
            case SQL_UPDATE_UNIQUE:
            case SQL_INSERT_UNIQUE:
            case SQL_DELETE_UNIQUE:
                if(pSrvrStmt->endOfData) {		// UID unique cmds chk for end of data
                    pSrvrStmt->rowCount._buffer[0] = 0;	// If NO DATA FOUND (100) set to 0 row count
                }
                else {
                    pSrvrStmt->rowCount._buffer[0] = 1;	// If DATA FOUND set row count to 1
                }
                pSrvrStmt->rowCount._length = 1;
                pSrvrStmt->totalRowCount += pSrvrStmt->rowCount._buffer[0];
                break;
            case SQL_SELECT_UNIQUE:
                // Read row count and load outputValueList for unique select
                pSrvrStmt->rowsAffected = 0;
                rows_read = 0;
                if(!pSrvrStmt->endOfData)
                {
                    retcode = ReadRow(pSrvrStmt, &pSrvrStmt->rowsAffected, &rows_read);
                    if (retcode < 0) THREAD_RETURN(pSrvrStmt,retcode);
                    if (retcode > 0) sqlWarning = TRUE;
                    pSrvrStmt->totalRowCount += rows_read;
                }
                DEBUG_OUT(DEBUG_LEVEL_DATA,("pSrvrStmt->rowsAffected=%ld ; rows_read=%ld", pSrvrStmt->rowsAffected, rows_read));
                break;
            case SQL_CONTROL:
            case SQL_SET_TRANSACTION:
            case SQL_SET_CATALOG:
            case SQL_SET_SCHEMA:
                DEBUG_OUT(DEBUG_LEVEL_CLI,("Control --- Set Connection Attribute statement type."));
            case SQL_SELECT_NON_UNIQUE:			// Row count to be filled in on subsequent fetch operations
                break;
                // SQL_OTHER (DDLs - create/alter/drop/etc) should behave like IUD non-unique Statement Types
            case SQL_OTHER:
            case SQL_INSERT_NON_UNIQUE:
            case SQL_UPDATE_NON_UNIQUE:
            case SQL_DELETE_NON_UNIQUE:
            default:
                Int64 tmpRowCount;
                // For all other NON-UNIQUE statements get the row count from GetDiag2
                retcode =  CLI_GetDiagnosticsStmtInfo2(	pStmt,
                        SQLDIAG_ROW_COUNT,
                        &tmpRowCount, 
                        NULL,
                        0,
                        NULL);
                if (retcode == 0)
                    pSrvrStmt->rowCount._buffer[0] = (int)tmpRowCount;
                else
                    pSrvrStmt->rowCount._buffer[0] = 0;
                pSrvrStmt->totalRowCount += pSrvrStmt->rowCount._buffer[0];
                if (pSrvrStmt->batchRowsetSize > 0){
                    // Currently we only get one value back from SQL for rowsets.
                    // We will fill in all values with SUCCESS_NO_INFO and ignore the
                    // one value until SQL updates to return the actual number.
#ifndef TODO	// Linux port Todo - Currently hardcoding actual value				
                    info_value = SQLMXStatement_SUCCESS_NO_INFO();
#else				
                    info_value = -2L;
#endif
                    for (row_idx=0; row_idx<pSrvrStmt->inputRowCnt; row_idx++) {
                        pSrvrStmt->rowCount._buffer[pSrvrStmt->rowCount._length+row_idx] = info_value;
                    }
                }
                if (retcode < 0) sqlWarning = TRUE;
                else {
                    pSrvrStmt->rowCount._length += pSrvrStmt->inputRowCnt;
                    DEBUG_OUT(DEBUG_LEVEL_CLI,("pSrvrStmt->rowCount._length=%ld", pSrvrStmt->rowCount._length));
                }
                break;
        } // end of default type
    } // End of Version 2.0 Check

    DEBUG_OUT(DEBUG_LEVEL_ENTRY,( "pSrvrStmt->endOfData=%ld, pSrvrStmt->outputValueList._buffer=0x%08x, pSrvrStmt->outputValueList._length=0x%08x, pSrvrStmt->totalRowCount=%ld",
                pSrvrStmt->endOfData, pSrvrStmt->outputValueList._buffer, pSrvrStmt->outputValueList._length,pSrvrStmt->totalRowCount));

    if (sqlWarning) THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);
    THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}

SQLRETURN FREESTATEMENT(SRVR_STMT_HDL* pSrvrStmt)
{
    FUNCTION_ENTRY("FREESTATEMENT",
            ("pSrvrStmt=0x%08x",
             pSrvrStmt));

	IDL_enum freeResourceOpt = pSrvrStmt->freeResourceOpt;
	Int32 *rowsAffected = &pSrvrStmt->rowsAffected;

	Int32 retcode;
	BOOL sqlWarning = FALSE;
	SRVR_STMT_HDL* pTmpSrvrStmt;
	SRVR_STMT_HDL* pTmp2SrvrStmt;
	bool done = false;
	
	SQLSTMT_ID *pStmt;
	SQLDESC_ID *pDesc;

	if(pSrvrStmt == NULL)
        THREAD_RETURN(pSrvrStmt,SQL_INVALID_HANDLE);

	pStmt = &pSrvrStmt->stmt;
	
	*rowsAffected = -1;
	switch( freeResourceOpt )
	{
	case SQL_DROP:
		// If we are de-allocating the SPJ CALL stmt then we
		// should also de-alloc any RS stmts associated with it.
		pTmpSrvrStmt = pSrvrStmt;
		pTmp2SrvrStmt = NULL;
		done = false;

		if (pSrvrStmt->sqlQueryType == SQL_CALL_WITH_RESULT_SETS && 
				pSrvrStmt->nextSpjRs != NULL)
			pSrvrStmt = pSrvrStmt->nextSpjRs;

		while( !done )
		{
			pStmt = &pSrvrStmt->stmt;
			pDesc = &pSrvrStmt->inputDesc;
			pSrvrStmt->freeBuffers(SQLWHAT_INPUT_DESC);
			if (pSrvrStmt->inputDescName[0] == '\0')
			{
				retcode = WSQL_EXEC_DeallocDesc(pDesc);
			}
			pDesc = &pSrvrStmt->outputDesc;
			pSrvrStmt->freeBuffers(SQLWHAT_OUTPUT_DESC);
			if (pSrvrStmt->outputDescName[0] == '\0')
			{
				retcode = WSQL_EXEC_DeallocDesc(pDesc);
			}
			if (! pSrvrStmt->isClosed)
			{
				retcode = WSQL_EXEC_CloseStmt(pStmt);
				pSrvrStmt->isClosed = TRUE;
			}
			if (pSrvrStmt->moduleName[0] == '\0')
			{
				retcode = WSQL_EXEC_DeallocStmt(pStmt);
			}	

			// If the RS stmt is passed to the method then only that stmt should be deleted
			if (pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET && pSrvrStmt != pTmpSrvrStmt)
			{	
				pTmp2SrvrStmt = pSrvrStmt;
				if (pSrvrStmt->nextSpjRs == NULL)
				{
					pSrvrStmt = pTmpSrvrStmt;
					pSrvrStmt->nextSpjRs = NULL;
				}
				else
					pSrvrStmt = pSrvrStmt->nextSpjRs;

				removeSrvrStmt(pTmp2SrvrStmt->dialogueId, (long)pTmp2SrvrStmt);
			}
			else
				done = true;
		}
		removeSrvrStmt(pSrvrStmt->dialogueId, (long)pSrvrStmt);
		break;
	case SQL_CLOSE:
		if (! pSrvrStmt->isClosed)
		{
			retcode = WSQL_EXEC_CloseStmt(pStmt);
			if (pSrvrStmt->stmtType != INTERNAL_STMT)
			{
				if (retcode == -8811)
				{
					retcode = SQL_SUCCESS;
				}
				HANDLE_ERROR(retcode, sqlWarning);
			}
			pSrvrStmt->isClosed = TRUE;
		}
		break;
	case SQL_UNBIND:
		pDesc = &pSrvrStmt->outputDesc;
		pSrvrStmt->freeBuffers(SQLWHAT_OUTPUT_DESC);
		if (pSrvrStmt->outputDescName[0] == '\0')
		{
			retcode = WSQL_EXEC_DeallocDesc(pDesc);
		}
		break;
	case SQL_RESET_PARAMS:
		pDesc = &pSrvrStmt->inputDesc;
		pSrvrStmt->freeBuffers(SQLWHAT_INPUT_DESC);
		if (pSrvrStmt->inputDescName[0] == '\0')
		{
			retcode = WSQL_EXEC_DeallocDesc(pDesc);
		}
		break;
	default:
		break;
	}  
	SRVRTRACE_EXIT(FILE_INTF+21);

    if (sqlWarning) THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);
    THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}

// This shall be removed once we ported all native interfaces
// to use the protocol buffer interface, and function PREPARE_ROWSETS
// will be renamed as PREPARE
SQLRETURN PREPARE(SRVR_STMT_HDL* pSrvrStmt) 
{
    FUNCTION_ENTRY("PREPARE",
            ("pSrvrStmt=0x%08x",
             pSrvrStmt));

    CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

    long retcode;
    SQLRETURN rc;

    SQLSTMT_ID	*pStmt;
    SQLDESC_ID	*pInputDesc;
    SQLDESC_ID	*pOutputDesc;

    long		numEntries;
    char		*pStmtName;
    BOOL		sqlWarning = FALSE;
    BOOL		rgWarning = FALSE;
    int    SqlQueryStatementType;

    pStmt = &pSrvrStmt->stmt;
    pOutputDesc = &pSrvrStmt->outputDesc;
    pInputDesc = &pSrvrStmt->inputDesc;

    if (!pSrvrStmt->isClosed)
    {
        retcode = CLI_CloseStmt(pStmt);
        if (retcode!=0) retcode = CLI_ClearDiagnostics(pStmt);
        pSrvrStmt->isClosed = TRUE;
    }

    if (pSrvrStmt->holdability == HOLD_CURSORS_OVER_COMMIT){
        retcode = CLI_SetStmtAttr(pStmt, SQL_ATTR_CURSOR_HOLDABLE, SQL_HOLDABLE, NULL);
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
    }

    SQLDESC_ID	sqlString_desc;

    sqlString_desc.version        = SQLCLI_ODBC_VERSION;
    sqlString_desc.module         = &pSrvrStmt->moduleId;
    sqlString_desc.name_mode      = string_data;
    sqlString_desc.identifier     = (const char *)pSrvrStmt->sqlString.dataValue._buffer;
    sqlString_desc.handle         = 0;
    sqlString_desc.identifier_len = pSrvrStmt->sqlString.dataValue._length;
    sqlString_desc.charset        = SQLCHARSETSTRING_ISO88591;

    retcode = CLI_Prepare(pStmt, &sqlString_desc);

    int rtn;

#ifndef DISABLE_NOWAIT		
    if (retcode == NOWAIT_PENDING){
        rtn = WaitForCompletion(pSrvrStmt, &pSrvrStmt->cond, &pSrvrStmt->mutex);
        DEBUG_OUT(DEBUG_LEVEL_CLI,("WaitForCompletion() returned %d",rtn));

        if (rtn == 0){
            rc = pSrvrStmt->switchContext();
            DEBUG_OUT(DEBUG_LEVEL_CLI,("pSrvrStmt->switchContext() returned %ld", rc));
            if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) THREAD_RETURN(pSrvrStmt,rc);

            switch (pSrvrStmt->nowaitRetcode)
            {
                case 0:
                    retcode = 0;
                    break;
                case 9999:
                    THREAD_RETURN(pSrvrStmt,NOWAIT_ERROR);
                default:
                    retcode = GETSQLCODE(pSrvrStmt);
                    break;
            }
            DEBUG_OUT(DEBUG_LEVEL_CLI,("pSrvrStmt->nowaitRetcode=%ld, retcode=%s",
                        pSrvrStmt->nowaitRetcode,
                        CliDebugSqlError(retcode)));
        }
        else
        {
            pSrvrStmt->nowaitRetcode = rtn;
            THREAD_RETURN(pSrvrStmt,NOWAIT_ERROR);
        }
    }
#endif

    HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

    pSrvrStmt->estimatedCost = -1;

    retcode = CLI_DescribeStmt(pStmt, pInputDesc, pOutputDesc);
    HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

    retcode = CLI_GetDescEntryCount(pInputDesc, (int *)&pSrvrStmt->paramCount);
    HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

    retcode = CLI_GetDescEntryCount(pOutputDesc, (int *)&pSrvrStmt->columnCount);
    HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

    pSrvrStmt->prepareSetup();

    if (pSrvrStmt->paramCount > 0){
        kdsCreateSQLDescSeq(&pSrvrStmt->inputDescList, pSrvrStmt->paramCount+pSrvrStmt->inputDescParamOffset);
        retcode = BuildSQLDesc(pSrvrStmt, SRVR_STMT_HDL::Input);
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
    } else {
        kdsCreateEmptySQLDescSeq(&pSrvrStmt->inputDescList);
    }

    if (pSrvrStmt->columnCount > 0){
        kdsCreateSQLDescSeq(&pSrvrStmt->outputDescList, pSrvrStmt->columnCount);
        retcode = BuildSQLDesc(pSrvrStmt, SRVR_STMT_HDL::Output);
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
    } else {
        kdsCreateEmptySQLDescSeq(&pSrvrStmt->outputDescList);
    }

    /* *****************************************************************************
     * The call to CLI_GetStmtAttr to query the statement type was added as a
     * performance enhancement. Previous versions of the Trafodion database will not return
     * a statement type, but will return a 0 which is SQL_OTHER. In the case were
     * SQL_OTHER is returned and JDBC/MX knows what the statement type is, then the
     * JDBC/MX statement type will be used. This will allow the JDBC/MX driver to
     * run with an older version of the Trafodion.
     * ***************************************************************************** */

    DEBUG_OUT(DEBUG_LEVEL_CLI,( "getSQLMX_Version: returned %i", GlobalInformation::getSQLMX_Version()));

    if (GlobalInformation::getSQLMX_Version() == CLI_VERSION_R2 ) {    //If this version of Trafodion is version R2
        if (pSrvrStmt->sqlStmtType != TYPE_UNKNOWN)                    //If this is a SELECT, INVOKE, or SHOWSHAPE
            SqlQueryStatementType = SQL_SELECT_NON_UNIQUE;              //then force an execute with no fetch
        else SqlQueryStatementType = SQL_OTHER;                         //else allow an executeFetch
    }
    else
    {
        retcode = CLI_GetStmtAttr( &pSrvrStmt->stmt,		// (IN) SQL statement ID
                SQL_ATTR_QUERY_TYPE,		// (IN) Request query statement attribute
                (int*)&SqlQueryStatementType,	// (OUT) Place to store query statement type
                NULL,					// (OUT) Optional string
                0,						// (IN) Max size of optional string buffer
                NULL );					// (IN) Length of item


        //If there is an error this statement will return
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
    }
    DEBUG_OUT(DEBUG_LEVEL_CLI,("SQL Query Statement Type=%s",
                CliDebugSqlQueryStatementType(SqlQueryStatementType)));

    if (SqlQueryStatementType == SQL_EXE_UTIL  &&
            pSrvrStmt->columnCount > 0)
        SqlQueryStatementType = SQL_SELECT_NON_UNIQUE;

    pSrvrStmt->setSqlQueryStatementType(SqlQueryStatementType);

    switch (pSrvrStmt->getSqlQueryStatementType())
    {
        case SQL_CALL_NO_RESULT_SETS:
            DEBUG_OUT(DEBUG_LEVEL_CLI|DEBUG_LEVEL_STMT,("Prepare SQL_CALL_NO_RESULT_SETS query type"));
            pSrvrStmt->isSPJRS = false;		// Indicate this is an RS.
            pSrvrStmt->RSIndex = 0;			// Index into RS array
            pSrvrStmt->RSMax = 0;			// No Result Sets to return
            DEBUG_OUT(DEBUG_LEVEL_CLI|DEBUG_LEVEL_STMT,("RSMax: %d  RSIndex: %d  isSPJRS: %d ", pSrvrStmt->RSMax, pSrvrStmt->RSIndex,  pSrvrStmt->isSPJRS));
            break;

        case SQL_CALL_WITH_RESULT_SETS:
            DEBUG_OUT(DEBUG_LEVEL_CLI|DEBUG_LEVEL_STMT,("Prepare SQL_CALL_WITH_RESULT_SETS query type"));
            pSrvrStmt->isSPJRS = true;
            retcode = RSgetRSmax(pSrvrStmt);
            DEBUG_OUT(DEBUG_LEVEL_CLI|DEBUG_LEVEL_STMT,("RSMax: %d  RSIndex: %d  isSPJRS: %d  ", pSrvrStmt->RSMax, pSrvrStmt->RSIndex,  pSrvrStmt->isSPJRS));
            //If there is an error this statement will return
            HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
            break;
    }
    if (sqlWarning) THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);
    THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}

static SQLRETURN NoDataFound(SRVR_STMT_HDL *pSrvrStmt, long curRowNo, int curRowCount, int *rowsAffected)
{
    FUNCTION_ENTRY("NoDataFound",("pSrvrStmt=0x%08x, curRowNo=%ld, curRowCount=%ld, rowsAffected=0x%08x",
                pSrvrStmt,
                curRowNo,
                curRowCount,
                rowsAffected));

    if (curRowNo == 1)
    {
        CLI_ClearDiagnostics(&pSrvrStmt->stmt);
        CLI_DEBUG_RETURN_SQL(SQL_NO_DATA_FOUND);
    }
    *rowsAffected = curRowCount;
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("rowsAffected=%ld",*rowsAffected));
    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
}

SQLRETURN FETCH(SRVR_STMT_HDL *pSrvrStmt)
{
    FUNCTION_ENTRY("FETCH",
            ("pSrvrStmt=0x%08x",
             pSrvrStmt));

    long retcode=SQL_SUCCESS;
    int curRowCount = 0;
    long curRowNo;
    long curColumnNo;
    BYTE *varPtr;
    short *indPtr;
    BYTE *pBytes;
    long dataType;
    long dataLength;
    long allocLength;
    short indValue;
    long columnCount;
    long charSet;
    long fetchSize;

    SQLDESC_ID *pDesc;
    BOOL sqlWarning = FALSE;

    pDesc = &pSrvrStmt->outputDesc;
    columnCount = pSrvrStmt->columnCount;

    if (pSrvrStmt->fetchRowsetSize == 0) {
        fetchSize = 1;
    }
    else {
        fetchSize = pSrvrStmt->fetchRowsetSize;
    }

    curRowNo = 1;
    while (curRowNo <= pSrvrStmt->maxRowCnt)
    {
        DEBUG_OUT(DEBUG_LEVEL_STMT,("***Anitha ---- >pSrvrStmt->isClosed=%ld", pSrvrStmt->isClosed));

        /// For Modius
        CLI_ClearDiagnostics(&pSrvrStmt->stmt);
        retcode = CLI_Fetch(&pSrvrStmt->stmt, pDesc, 0);

        int rtn;

#ifndef DISABLE_NOWAIT		
        if (retcode == NOWAIT_PENDING){
            rtn = WaitForCompletion(pSrvrStmt, &pSrvrStmt->cond, &pSrvrStmt->mutex);
            DEBUG_OUT(DEBUG_LEVEL_ENTRY,("WaitForCompletion() returned %d",rtn));

            if (rtn == 0){
                SQLRETURN rc = pSrvrStmt->switchContext();
                DEBUG_OUT(DEBUG_LEVEL_CLI,("pSrvrStmt->switchContext() returned %ld", rc));
                if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) THREAD_RETURN(pSrvrStmt,rc);

                switch (pSrvrStmt->nowaitRetcode)
                {
                    case 0:			// nowaitRetcode is successful
                        retcode = 0;
                        break;
                    case 9999:
                        THREAD_RETURN(pSrvrStmt,NOWAIT_ERROR);
                    default:
                        /* Soln No: 10-070223-2784
Desc: JDBC/MX should call stmtinfo2 instead of Diagoninfo2 CLI call for rowsets
*/
                        /*     long row = 0;
                               retcode = CLI_GetDiagnosticsStmtInfo2(&pSrvrStmt->stmt,SQLDIAG_ROW_COUNT,&row,NULL,0,NULL);
                               if(row == 0)
                               retcode = GETSQLCODE(pSrvrStmt);
                               */
                        // Refixed 10-070223-2784 for sol.10-090613-2299
                        retcode = GETSQLCODE(pSrvrStmt);

                        long rows_read_fin = 0;
                        long retcodenew = 0;
                        if (pSrvrStmt->fetchRowsetSize > 0)
                        {
                            retcodenew = ReadRow(pSrvrStmt, &curRowCount, &rows_read_fin);
                            if (retcodenew < 0) THREAD_RETURN(pSrvrStmt,retcodenew);
                            if (retcodenew > 0) sqlWarning = TRUE;

                            curRowNo += rows_read_fin;
                        }
                        break;
                }
                DEBUG_OUT(DEBUG_LEVEL_CLI,("pSrvrStmt->nowaitRetcode=%ld, retcode=%s",
                            pSrvrStmt->nowaitRetcode,
                            CliDebugSqlError(retcode)));
            }
            else {
                pSrvrStmt->nowaitRetcode = rtn;
                THREAD_RETURN(pSrvrStmt,NOWAIT_ERROR);
            }
        }
#endif

        if (retcode != 0)
        {                  //Check for a bad return code
            if (retcode == 100)
            {
                THREAD_RETURN(pSrvrStmt,NoDataFound(pSrvrStmt,curRowNo,curRowCount,&pSrvrStmt->rowsAffected));
            }
            else if (retcode < 0)
            {
                THREAD_RETURN(pSrvrStmt,SQL_ERROR);
            }
            else
            {
                sqlWarning = TRUE;
            }
        }

        // Read row count and load the output value list
        long rows_read = 0;
        retcode = ReadRow(pSrvrStmt, &curRowCount, &rows_read);
        if (retcode < 0) THREAD_RETURN(pSrvrStmt,retcode);
        if (retcode > 0) sqlWarning = TRUE;

        curRowNo += rows_read;
    }
    pSrvrStmt->rowsAffected = curRowCount > pSrvrStmt->maxRowCnt ? pSrvrStmt->maxRowCnt : curRowCount;
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("pSrvrStmt->rowsAffected=%ld curRowCount=%ld pSrvrStmt->maxRowCnt=%ld",
                pSrvrStmt->rowsAffected, curRowCount, pSrvrStmt->maxRowCnt));

    if (sqlWarning) THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);
    THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}


SQLRETURN GETSQLERROR(SRVR_STMT_HDL *pSrvrStmt,
        odbc_SQLSvc_SQLError *SQLError)
{
    FUNCTION_ENTRY("GETSQLERROR",
            ("pSrvrStmt=0x%08x, SQLError=0x%08x",
             pSrvrStmt,
             SQLError));

    long retcode;
    int total_conds = 0;
    int buf_len;
    int sqlcode  = 0;
    char sqlState[6];
    int curr_cond = 1;

    retcode =  CLI_GetDiagnosticsStmtInfo2(NULL,
            SQLDIAG_NUMBER,
            &total_conds, NULL, 0, NULL);

    if (total_conds == 0)
    {
        kdsCreateSQLErrorException(SQLError, 1);
        kdsCopySQLErrorException(SQLError, "No error message in Trafodion diagnostics area, but sqlcode is non-zero", retcode, "");
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
    }

    kdsCreateSQLErrorException(SQLError, total_conds);
    while (curr_cond <= total_conds)
    {
        char *msg_buf=NULL;
        int msg_buf_len;
        retcode = CLI_GetDiagnosticsCondInfo2(SQLDIAG_SQLCODE, curr_cond,
                &sqlcode, NULL, 0, NULL);
        if (retcode >= SQL_SUCCESS)
        {
            if (sqlcode == 100)
            {
                // We are not copying the Warning message if the error code is 100
                // It is ok, though we have allocated more SQLError, but length is incremented
                // only when SQLError is copied
                curr_cond++;
                continue;
            }

            retcode = CLI_GetDiagnosticsCondInfo2(SQLDIAG_MSG_LEN, curr_cond,
                    &msg_buf_len, NULL, 0, NULL);
        }
        if (retcode >= SQL_SUCCESS)
        {
            MEMORY_ALLOC_ARRAY(msg_buf, char, msg_buf_len+1);
            msg_buf[msg_buf_len] = 0;
            buf_len = 0;
            // By passing the msg_buf_len to the following SQL call,
            // the returned buf_len will be equal to msg_buf_len w/o a
            // null terminator. If a value greater than msg_buf_len is passed,
            // msg_buf will be null terminated by SQL.
            retcode = CLI_GetDiagnosticsCondInfo2(SQLDIAG_MSG_TEXT, curr_cond,
                    NULL, msg_buf, msg_buf_len, &buf_len);
            DEBUG_ASSERT(msg_buf[msg_buf_len]==0,("Memory corruption detected during error message handling"));
        }
        if (retcode >= SQL_SUCCESS)
        {
            // Found that the returned CLI message length can be incorrect.  If the
            //   size returned is too small, we will retry once with a bigger buffer and hope
            //   it works.
            if (buf_len>msg_buf_len)
            {
                DEBUG_OUT(DEBUG_LEVEL_CLI,("CLI Message length changed.  Retrying."));
                MEMORY_DELETE_ARRAY(msg_buf);
                msg_buf_len = buf_len;
                MEMORY_ALLOC_ARRAY(msg_buf, char, msg_buf_len+1);
                msg_buf[msg_buf_len] = 0;
                buf_len = 0;
                retcode = CLI_GetDiagnosticsCondInfo2(SQLDIAG_MSG_TEXT, curr_cond,
                        NULL, msg_buf, msg_buf_len, &buf_len);
                DEBUG_ASSERT(msg_buf[msg_buf_len]==0,("Memory corruption detected during error message retry handling"));
                // Happened again.  Just use the short buffer.
                if (buf_len>msg_buf_len) buf_len = msg_buf_len;
            }
            // Null terminate msg_buf since it is not yet null terminated.
            msg_buf[buf_len] = '\0';
            DEBUG_OUT(DEBUG_LEVEL_CLI,("msg_buf='%s'",msg_buf));
            buf_len = 0;
            retcode = CLI_GetDiagnosticsCondInfo2(SQLDIAG_RET_SQLSTATE, curr_cond,
                    NULL, sqlState, sizeof(sqlState), &buf_len);
        }
        if (retcode < SQL_SUCCESS)
        {
            kdsCopySQLErrorException(SQLError, "Internal Error : From CLI_GetDiagnosticsCondInfo2",
                    retcode, "");
            MEMORY_DELETE_ARRAY(msg_buf);
            break;
        }
        sqlState[5] = '\0';
        kdsCopySQLErrorException(SQLError, msg_buf, sqlcode, sqlState);
        MEMORY_DELETE_ARRAY(msg_buf);
        curr_cond++;
    }

    CLI_ClearDiagnostics(NULL);
    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN EXECDIRECT(SRVR_STMT_HDL* pSrvrStmt)
{
    FUNCTION_ENTRY("EXECDIRECT",
            ("pSrvrStmt=0x%08x",
             pSrvrStmt));

    CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

    long retcode = SQL_SUCCESS;

    SQLSTMT_ID	*pStmt;
    SQLDESC_ID	*pInputDesc;
    SQLDESC_ID	*pOutputDesc;
    SQLSTMT_ID  cursorId;

    long		numEntries;
    char		*pStmtName;
    char		*cursorName;
    size_t		len;
    BOOL		sqlWarning = FALSE;

    pStmt = &pSrvrStmt->stmt;
    pInputDesc = &pSrvrStmt->inputDesc;
    pOutputDesc = &pSrvrStmt->outputDesc;

    SQLDESC_ID	sqlString_desc;

    sqlString_desc.version        = SQLCLI_ODBC_VERSION;
    sqlString_desc.module         = &pSrvrStmt->moduleId;
    sqlString_desc.name_mode      = string_data;
    sqlString_desc.identifier     = (const char *) pSrvrStmt->sqlString.dataValue._buffer;
    sqlString_desc.handle         = 0;
    sqlString_desc.identifier_len = pSrvrStmt->sqlString.dataValue._length;
    sqlString_desc.charset        = SQLCHARSETSTRING_ISO88591;

    DEBUG_ASSERT(pSrvrStmt->isClosed, ("Server Statement is Open before execute."));
    pSrvrStmt->isClosed = TRUE;

    if (pSrvrStmt->sqlStmtType & TYPE_SELECT)
    {
        retcode = CLI_Prepare(pStmt, &sqlString_desc);
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
        pSrvrStmt->estimatedCost = -1;
    }
    else
    {
        retcode = CLI_ExecDirect(pStmt, &sqlString_desc, 0, 0);
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
        pSrvrStmt->isClosed = FALSE;
        pSrvrStmt->estimatedCost = -1;
    }

    if (pSrvrStmt->sqlStmtType & TYPE_SELECT)
    {
        // Retrieving/loading OutputDesc values
        retcode = CLI_DescribeStmt(pStmt, (SQLDESC_ID *)NULL, pOutputDesc);
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

        retcode = CLI_GetDescEntryCount(pOutputDesc, (int*)&pSrvrStmt->columnCount);
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

        if (pSrvrStmt->columnCount > 0)
        {
            kdsCreateSQLDescSeq(&pSrvrStmt->outputDescList, pSrvrStmt->columnCount);
            retcode = BuildSQLDesc(pSrvrStmt, SRVR_STMT_HDL::Output);
            HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
        }
        else
        {
            kdsCreateEmptySQLDescSeq(&pSrvrStmt->outputDescList);
        }
    }
    else
    {
        pSrvrStmt->columnCount = 0;
        kdsCreateEmptySQLDescSeq(&pSrvrStmt->outputDescList);
    }
    if (pSrvrStmt->stmtType == EXTERNAL_STMT)
    {
        if (pSrvrStmt->sqlStmtType & TYPE_SELECT)
        {
            cursorName = pSrvrStmt->cursorName;
            // If cursor name is not specified, use the stmt name as cursor name
            if (*cursorName == '\0') {
                cursorName = pSrvrStmt->stmtName;
            }

            DEBUG_OUT(DEBUG_LEVEL_CLI,("cursorName %s previousCursorName %s",
                        cursorName,
                        pSrvrStmt->previousCursorName));

            // If cursorName has chg'd from last EXEC or EXECDIRECT cmd
            // or has not yet been set for the first time call SetCursorName
            if ((strcmp(pSrvrStmt->previousCursorName, cursorName) != 0) && *cursorName != '\0')
            {
                cursorId.version        = SQLCLI_ODBC_VERSION;
                cursorId.module         = pStmt->module;
                cursorId.handle         = 0;
                cursorId.charset        = SQLCHARSETSTRING_ISO88591;
                cursorId.name_mode      = cursor_name;
                cursorId.identifier_len = strlen(cursorName);
                cursorId.identifier     = cursorName;
                strcpy(pSrvrStmt->previousCursorName, cursorName);		// keep track of last cursor name used
                DEBUG_OUT(DEBUG_LEVEL_CLI,("cursorName %s previousCursorName %s",
                            cursorName,
                            pSrvrStmt->previousCursorName));

                retcode = CLI_SetCursorName(pStmt, &cursorId);
                HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
            }
            retcode = CLI_Exec(pStmt, NULL, 0);
            HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
            pSrvrStmt->isClosed = FALSE;
        }
    }

    if (retcode >= SQL_SUCCESS)
    {
        // If NOT a select type
        if ((pSrvrStmt->sqlStmtType & TYPE_SELECT)==0)
        {
            Int64 tmpRowCount;
            retcode =  CLI_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &tmpRowCount,
                    NULL, 0, NULL);
            if (retcode < 0)
            {
                sqlWarning = TRUE;
                pSrvrStmt->rowsAffected = -1;
            }
            else
                pSrvrStmt->rowsAffected = (int)tmpRowCount;
        }
        else
            pSrvrStmt->rowsAffected = -1;
    }

    if (sqlWarning) THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);
    THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}

SQLRETURN EXECUTESPJRS(SRVR_STMT_HDL *pSrvrStmt)
{
    FUNCTION_ENTRY("EXECUTESPJRS",
            ("pSrvrStmt=0x%08x",
             pSrvrStmt));

    CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

    long retcode = SQL_SUCCESS;

    SQLDESC_ID		*pOutputDesc;
    SQLSTMT_ID		*pStmt;
    SQLSTMT_ID		cursorId;
    long			numEntries;
    char			*pStmtName;
    char			*cursorName;
    size_t			len;
    BOOL			sqlWarning = FALSE;
    int				rtn = 0;
    SQLRETURN		rc;

    // For debug purposes only
    //long int		SqlQueryStatementType;

    pStmt = &pSrvrStmt->stmt;
    pOutputDesc = &pSrvrStmt->outputDesc;

    DEBUG_OUT(DEBUG_LEVEL_STMT,("pStmt=0x%08x, pOutputDesc=0x%08x, RSIndex=%ld, isClosed=0x%08x",
                pStmt,
                pOutputDesc,
                pSrvrStmt->RSIndex,
                pSrvrStmt->isClosed));

    // Retrieving/loading OutputDesc values
    retcode = CLI_DescribeStmt(pStmt, (SQLDESC_ID *)NULL, pOutputDesc);

    DEBUG_OUT(DEBUG_LEVEL_CLI, ("EXECUTESPJRS : retcode = %s",
                CliDebugSqlError(retcode)));

    // Trying to open a non-existent SPJRS
    if(retcode == RS_DOES_NOT_EXIST)
    {
        pSrvrStmt->isClosed = TRUE;
        kdsCreateEmptySQLDescSeq(&pSrvrStmt->outputDescList);
        THREAD_RETURN(pSrvrStmt,SQL_RS_DOES_NOT_EXIST);
    }
    HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

    retcode = CLI_GetDescEntryCount(pOutputDesc,(int*) &pSrvrStmt->columnCount);
    HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

    if (pSrvrStmt->columnCount > 0)
    {
        kdsCreateSQLDescSeq(&pSrvrStmt->outputDescList, pSrvrStmt->columnCount);
        retcode = BuildSQLDesc(pSrvrStmt, SRVR_STMT_HDL::Output);
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
    }
    else
    {
        kdsCreateEmptySQLDescSeq(&pSrvrStmt->outputDescList);
    }

    cursorName = pSrvrStmt->cursorName;
    // If cursor name is not specified, use the stmt name as cursor name
    if (*cursorName == '\0') {
        cursorName = pSrvrStmt->stmtName;
    }

    DEBUG_OUT(DEBUG_LEVEL_CLI|DEBUG_LEVEL_STMT,("cursorName %s : previousCursorName %s",
                cursorName, pSrvrStmt->previousCursorName));

    // If cursorName has chg'd from last EXEC or EXECDIRECT cmd
    // or has not yet been set for the first time call SetCursorName
    if ((strcmp(pSrvrStmt->previousCursorName, cursorName) != 0) && *cursorName != '\0')
    {
        cursorId.version        = SQLCLI_ODBC_VERSION;
        cursorId.module         = pStmt->module;
        cursorId.handle         = 0;
        cursorId.charset        = SQLCHARSETSTRING_ISO88591;
        cursorId.name_mode      = cursor_name;
        cursorId.identifier_len = strlen(cursorName);
        cursorId.identifier     = cursorName;
        strcpy(pSrvrStmt->previousCursorName, cursorName);
        DEBUG_OUT(DEBUG_LEVEL_CLI|DEBUG_LEVEL_STMT,
                ("Calling CLI_SetCursorName - cursorName %s : previousCursorName %s",
                 cursorName,
                 pSrvrStmt->previousCursorName));

        retcode = CLI_SetCursorName(pStmt, &cursorId);
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
    }

    DEBUG_ASSERT(pSrvrStmt->isClosed, ("Server RS Statement is Open before execute."));
    retcode = CLI_Exec(pStmt, NULL, 0);
    DEBUG_OUT(DEBUG_LEVEL_STMT,("EXECUTESPJRS  CLI_EXEC  retcode: %ld.", retcode));
    HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
    pSrvrStmt->isClosed = FALSE;

#ifndef DISABLE_NOWAIT		
    if (retcode == NOWAIT_PENDING){
        rtn = WaitForCompletion(pSrvrStmt, &pSrvrStmt->cond, &pSrvrStmt->mutex);
        DEBUG_OUT(DEBUG_LEVEL_CLI,("EXECUTESPJRS : WaitForCompletion() returned %d",rtn));

        if (rtn == 0)
        {
            rc = pSrvrStmt->switchContext();
            DEBUG_OUT(DEBUG_LEVEL_CLI,("EXECUTESPJRS  pSrvrStmt->switchContext() return with: %ld.", rc));
            if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) THREAD_RETURN(pSrvrStmt,rc);

            switch (pSrvrStmt->nowaitRetcode)
            {
                case 0:
                    retcode = 0;
                    break;
                case 9999:
                    pSrvrStmt->isClosed = TRUE;
                    THREAD_RETURN(pSrvrStmt,NOWAIT_ERROR);
                default:
                    pSrvrStmt->isClosed = TRUE;
                    retcode = GETSQLCODE(pSrvrStmt);
                    break;
            }
            DEBUG_OUT(DEBUG_LEVEL_CLI,
                    ("EXECUTESPJRS : pSrvrStmt->nowaitRetcode=%ld, retcode=%s",
                     pSrvrStmt->nowaitRetcode,
                     CliDebugSqlError(retcode)));
        }
        else
        {
            pSrvrStmt->isClosed = TRUE;
            pSrvrStmt->nowaitRetcode = rtn;
            THREAD_RETURN(pSrvrStmt,NOWAIT_ERROR);
        }
    }
#endif

    // Note this could do a return
    HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

    if (sqlWarning) THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);
    THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}

SQLRETURN GETSQLWARNING(SRVR_STMT_HDL *pSrvrStmt,
        ERROR_DESC_LIST_def *sqlWarning)
{
    // This function is needed since SQLError uses odbc_SQLSvc_SQLError * instead of ERROR_DESC_LIST_def *
    // Hence this function wraps around GETSQLERROR
    FUNCTION_ENTRY("GETSQLWARNING",
            ("pSrvrStmt=0x%08x, sqlWarning=0x%08x",
             pSrvrStmt,
             sqlWarning));

    odbc_SQLSvc_SQLError SQLError;
    CLEAR_ERROR(SQLError);

    long retcode = GETSQLERROR(pSrvrStmt, &SQLError);
    sqlWarning->_length = SQLError.errorList._length;
    sqlWarning->_buffer = SQLError.errorList._buffer;
    CLI_DEBUG_RETURN_SQL((SQLRETURN)retcode);
}


SQLRETURN CANCEL(SRVR_STMT_HDL *pSrvrStmt)
{
    FUNCTION_ENTRY("CANCEL",
            ("pSrvrStmt=0x%08x",
             pSrvrStmt));

    THREAD_RETURN(pSrvrStmt,CLI_Cancel(&pSrvrStmt->stmt));
}


SQLRETURN CLEARDIAGNOSTICS(SRVR_STMT_HDL *pSrvrStmt)
{
    FUNCTION_ENTRY("CLEARDIAGNOSTICS",
            ("pSrvrStmt=0x%08x",
             pSrvrStmt));

    long retcode = CLI_ClearDiagnostics(&pSrvrStmt->stmt);
    CLI_DEBUG_RETURN_SQL((SQLRETURN)retcode);
}

SQLRETURN PREPARE_FROM_MODULE(SRVR_STMT_HDL* pSrvrStmt)
{
    FUNCTION_ENTRY("PREPARE_FROM_MODULE",
            ("pSrvrStmt=0x%08x",
             pSrvrStmt));

    long retcode = SQL_SUCCESS;

    SQLSTMT_ID	*pStmt;
    SQLDESC_ID	*pInputDesc;
    SQLDESC_ID	*pOutputDesc;
    int    SqlQueryStatementType;

    long		numEntries;
    char		*pStmtName;
    BOOL		sqlWarning = FALSE;

    pStmt       = &pSrvrStmt->stmt;
    pInputDesc  = &pSrvrStmt->inputDesc;
    pOutputDesc = &pSrvrStmt->outputDesc;

    if (!pSrvrStmt->isClosed)
    {
        retcode = CLI_CloseStmt(pStmt);

        if (retcode!=0)
        {
            retcode = CLI_ClearDiagnostics(pStmt);
        }
        pSrvrStmt->isClosed = TRUE;
    }
    if (pSrvrStmt->holdability == HOLD_CURSORS_OVER_COMMIT)
    {
        retcode = CLI_SetStmtAttr(&pSrvrStmt->stmt, SQL_ATTR_CURSOR_HOLDABLE, SQL_HOLDABLE, NULL);
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
    }

    // MFC  if mfc is on allocate descriptors even if descriptor name is NULL
    if ((pSrvrStmt->useDefaultDesc) && (!srvrGlobal->moduleCaching))
    {
        if (pSrvrStmt->inputDescName[0] != '\0')
        {
            retcode = CLI_GetDescEntryCount(pInputDesc, (int *)&pSrvrStmt->paramCount);
            HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
        }
        else
        {
            pSrvrStmt->paramCount = 0;
        }

        if (pSrvrStmt->outputDescName[0] != '\0')
        {
            retcode = CLI_GetDescEntryCount(pOutputDesc, (int *)&pSrvrStmt->columnCount);
            HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
        }
        else
            pSrvrStmt->columnCount = 0;

    }
    else
    {
        retcode = CLI_DescribeStmt(pStmt, pInputDesc, pOutputDesc);
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

        retcode = CLI_GetDescEntryCount(pInputDesc, (int *)&pSrvrStmt->paramCount);
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

        retcode = CLI_GetDescEntryCount(pOutputDesc, (int *)&pSrvrStmt->columnCount);
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
    }

    if (pSrvrStmt->paramCount > 0)
    {
        kdsCreateSQLDescSeq(&pSrvrStmt->inputDescList, pSrvrStmt->paramCount);
        retcode = BuildSQLDesc(pSrvrStmt, SRVR_STMT_HDL::Input);
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
    }
    else
    {
        kdsCreateEmptySQLDescSeq(&pSrvrStmt->inputDescList);
    }

    if (pSrvrStmt->columnCount > 0)
    {
        kdsCreateSQLDescSeq(&pSrvrStmt->outputDescList, pSrvrStmt->columnCount);
        retcode = BuildSQLDesc(pSrvrStmt, SRVR_STMT_HDL::Output);
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
    }
    else
    {
        kdsCreateEmptySQLDescSeq(&pSrvrStmt->outputDescList);
    }

    /* *****************************************************************************
     * The call to SQL_EXEC_GetStmtAttr to query the statement type was added as a
     * performance enhancement. Previous version of the Trafodion database will not return
     * a statement type, but will return a 0 which is SQL_OTHER. In the case were
     * SQL_OTHER is returned and JDBC/MX knows what the statement type is, then the
     * JDBC/MX statement type will be used. This will allow the JDBC/MX driver to
     * run with an older version of the Trafodion.
     * ***************************************************************************** */


    DEBUG_OUT(DEBUG_LEVEL_CLI,( "getSQLMX_Version: returned %i", GlobalInformation::getSQLMX_Version()));

    if (GlobalInformation::getSQLMX_Version() == CLI_VERSION_R2 ) {    //If this version of Trafodion is version R2
        if (pSrvrStmt->sqlStmtType != TYPE_UNKNOWN)                     //If this is a SELECT, INVOKE, or SHOWSHAPE
            SqlQueryStatementType = SQL_SELECT_NON_UNIQUE;              //then force an execute with no fetch
        else SqlQueryStatementType = SQL_OTHER;                         //else allow an executeFetch
    }
    else
    {
        retcode = CLI_GetStmtAttr( &pSrvrStmt->stmt,		// (IN) SQL statement ID
                SQL_ATTR_QUERY_TYPE,		// (IN) Request query statement attribute
                &SqlQueryStatementType,	// (OUT) Place to store query statement type
                NULL,					// (OUT) Optional string
                0,						// (IN) Max size of optional string buffer
                NULL );					// (IN) Length of item
        //If there is an error this statement will return
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
    }
    DEBUG_OUT(DEBUG_LEVEL_CLI,("SQL Query Statement Type=%s",
                CliDebugSqlQueryStatementType(SqlQueryStatementType)));
    pSrvrStmt->setSqlQueryStatementType(SqlQueryStatementType);

    CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

    if (sqlWarning) THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);
    THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}

//---------------------------------------------------------------------------
// PREPARE2 Rowset prepare
SQLRETURN PREPARE2withRowsets(SRVR_STMT_HDL* pSrvrStmt)
{
    SQLItemDescList_def *inputSQLDesc = &pSrvrStmt->inputDescList;

    char              *pSqlStr     = pSrvrStmt->sqlStringText;
    RES_HIT_DESC_def  *rgPolicyHit = &pSrvrStmt->rgPolicyHit;

    Int32               retcode;

    SQLSTMT_ID	*pStmt;
    SQLDESC_ID	*pInputDesc;
    SQLDESC_ID	*pOutputDesc;

    char		*pStmtName;
    BOOL		sqlWarning  = FALSE;
    BOOL		rgWarning   = FALSE;
    BOOL		shapeWarning = FALSE;
    Int32		tempmaxRowsetSize = 0;
    UInt32 tmpBuildID = 0;
    Int32		holdableCursor = pSrvrStmt->holdableCursor;

    pSrvrStmt->PerfFetchRetcode   = SQL_SUCCESS;
    pSrvrStmt->RowsetFetchRetcode = SQL_SUCCESS;

    pStmt = &pSrvrStmt->stmt;
    pOutputDesc = &pSrvrStmt->outputDesc;
    pInputDesc = &pSrvrStmt->inputDesc;

    SQLDESC_ID         sqlString_desc;
    sqlString_desc.version        = SQLCLI_ODBC_VERSION;
    sqlString_desc.module         = &pSrvrStmt->moduleId;
    sqlString_desc.name_mode      = string_data;
    sqlString_desc.identifier     = (const char *) pSqlStr;
    sqlString_desc.handle         = 0;
    sqlString_desc.identifier_len = pSrvrStmt->sqlStringLen;
    sqlString_desc.charset = SQLCHARSETSTRING_UTF8;

    retcode = WSQL_EXEC_SetDescItem(pOutputDesc, 0, SQLDESC_ROWSET_TYPE, 0, 0);
    HANDLE_ERROR(retcode, sqlWarning);

    if (pSrvrStmt->maxRowsetSize > 1
            && (   pSrvrStmt->sqlStmtType == TYPE_INSERT_PARAM
                || pSrvrStmt->sqlStmtType == TYPE_UPDATE
                || pSrvrStmt->sqlStmtType == TYPE_DELETE
               )
       )
        retcode = WSQL_EXEC_SetStmtAttr(pStmt,SQL_ATTR_INPUT_ARRAY_MAXSIZE,pSrvrStmt->maxRowsetSize, NULL);
    else
        // We have to do the following CLI to set rowsize to ZERO,
        // since we switch between rowset to non-rowsets
        retcode = WSQL_EXEC_SetStmtAttr(pStmt,SQL_ATTR_INPUT_ARRAY_MAXSIZE,tempmaxRowsetSize, NULL);

    HANDLE_ERROR(retcode, sqlWarning);

    // need to set HOLDABLE cursor only when it differs from current holdable cursor
    if (holdableCursor != pSrvrStmt->current_holdableCursor) {
        retcode =  WSQL_EXEC_SetStmtAttr(pStmt,SQL_ATTR_CURSOR_HOLDABLE,holdableCursor, NULL);
        pSrvrStmt->current_holdableCursor = holdableCursor;
        HANDLE_ERROR(retcode, sqlWarning);
    }	// need to set HOLDABLE cursor

    pSrvrStmt->NA_supported = true;		// NOT ATOMIC ROWSET RECOVERY initialization
    pSrvrStmt->sqlUniqueQueryIDLen = MAX_QUERY_NAME_LEN;
    pSrvrStmt->sqlUniqueQueryID[0] = '\0';
    retcode = WSQL_EXEC_Prepare2(pStmt, &sqlString_desc, NULL, NULL, NULL, &pSrvrStmt->cost_info, &pSrvrStmt->comp_stats_info,
            pSrvrStmt->sqlUniqueQueryID, &pSrvrStmt->sqlUniqueQueryIDLen);
    pSrvrStmt->sqlUniqueQueryID[pSrvrStmt->sqlUniqueQueryIDLen] = '\0';
    pSrvrStmt->m_bNewQueryId = true;

    if (retcode != 30032) // ignore warning about recompile
        HANDLE_ERROR(retcode, sqlWarning);

    if (retcode == 30026 || retcode == 30028 || retcode == 30033 || retcode == 30034 || retcode == 30029 || pSrvrStmt->sqlStmtType == TYPE_UPDATE || pSrvrStmt->sqlStmtType == TYPE_DELETE)	// Based on SQL's input disabling the 30027 check and make it RFE in Executor. Enable it back once Executor supports it.
        pSrvrStmt->NA_supported = false; // NOT ATOMIC ROWSET RECOVERY is not supported for this SQL

    retcode = WSQL_EXEC_DescribeStmt(pStmt, pInputDesc, pOutputDesc);
    if (retcode != 30032) // ignore warning about recompile
        HANDLE_ERROR(retcode, sqlWarning);

    retcode = WSQL_EXEC_GetDescEntryCount(pInputDesc, &pSrvrStmt->paramCount);
    if (retcode != 30032) // ignore warning about recompile
        HANDLE_ERROR(retcode, sqlWarning);

    retcode = WSQL_EXEC_GetDescEntryCount(pOutputDesc, &pSrvrStmt->columnCount);
    if (retcode != 30032) // ignore warning about recompile
        HANDLE_ERROR(retcode, sqlWarning);

    // Child query visibility
    retcode = WSQL_EXEC_GetStmtAttr(pStmt, SQL_ATTR_SUBQUERY_TYPE, &pSrvrStmt->sqlSubQueryType, NULL, 0, NULL);

    retcode = WSQL_EXEC_GetStmtAttr(pStmt, SQL_ATTR_QUERY_TYPE, &pSrvrStmt->sqlQueryType, NULL, 0, NULL);

    // Added the below to treat the new SQL_EXE_UTIL type as SQL_SELECT_NON_UNIQUE
    // in MXOSRVR to minimize code changes and to send the new query type to WMS.
    pSrvrStmt->sqlNewQueryType = pSrvrStmt->sqlQueryType;
    if( pSrvrStmt->sqlQueryType == SQL_EXE_UTIL )
    {
        if(pSrvrStmt->columnCount > 0)
            pSrvrStmt->sqlQueryType = SQL_SELECT_NON_UNIQUE;
        else
            pSrvrStmt->sqlQueryType = SQL_OTHER;
    }


    HANDLE_ERROR(retcode, sqlWarning);
    if (pSrvrStmt->paramCount > 0)
    {
        tmpBuildID = srvrGlobal->drvrVersion.buildId; // should go way once we support rowwise rowsets
        srvrGlobal->drvrVersion.buildId = 0;

        kdsCreateSQLDescSeq(inputSQLDesc, pSrvrStmt->paramCount);
        retcode = BuildSQLDesc2withRowsets( pInputDesc
                , -9999
                , 0
                , pSrvrStmt->sqlBulkFetchPossible
                , pSrvrStmt->paramCount
                , pSrvrStmt->inputDescBuffer
                , pSrvrStmt->inputDescBufferLength
                , pSrvrStmt->inputDescVarBuffer
                , pSrvrStmt->inputDescVarBufferLen
                , pSrvrStmt->IPD
                , pSrvrStmt->inputQuadList
                , pSrvrStmt->inputQuadList_recover);

        srvrGlobal->drvrVersion.buildId = tmpBuildID; // should go way once we support rowwise rowsets);

        HANDLE_ERROR(retcode, sqlWarning);
    }

    Int32 estRowLength = 0;
    if (pSrvrStmt->columnCount > 0)
    {
        retcode = BuildSQLDesc2(pOutputDesc, pSrvrStmt->sqlQueryType,  pSrvrStmt->maxRowsetSize, pSrvrStmt->sqlBulkFetchPossible,
                pSrvrStmt->columnCount, pSrvrStmt->outputDescBuffer, pSrvrStmt->outputDescBufferLength,
                pSrvrStmt->outputDescVarBuffer, pSrvrStmt->outputDescVarBufferLen, pSrvrStmt->IRD,pSrvrStmt->SqlDescInfo);
        HANDLE_ERROR(retcode, sqlWarning);

        if (pSrvrStmt->sqlBulkFetchPossible && pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE)
        {
            estRowLength = pSrvrStmt->outputDescVarBufferLen;
            pSrvrStmt->bFirstSqlBulkFetch = true;
        }
        else
        {
            int columnCount = pSrvrStmt->columnCount;
            Int32 estLength;
            SRVR_DESC_HDL *IRD = pSrvrStmt->IRD;

            for (int curColumnNo = 0; curColumnNo < columnCount ; curColumnNo++)
            {
                IRD = pSrvrStmt->IRD;
                estLength = getAllocLength(IRD[curColumnNo].dataType, IRD[curColumnNo].length);
                estLength += 1;
                estRowLength += estLength;
            }
        }
    }
    pSrvrStmt->estRowLength = estRowLength;
    pSrvrStmt->preparedWithRowsets = TRUE;

    if (rgWarning)
        return ODBC_RG_WARNING;
    if (sqlWarning)
        return SQL_SUCCESS_WITH_INFO;
    else
        return SQL_SUCCESS;

}	// end PREPARE2withRowsets

SQLRETURN ALLOCSQLMXHDLS(SRVR_STMT_HDL* pSrvrStmt)
{
    FUNCTION_ENTRY("ALLOCSQLMXHDLS", ("pSrvrStmt=0x%08x",
                pSrvrStmt));

    Int32 retcode = SQL_SUCCESS;
    SQLSTMT_ID  *pStmt = &pSrvrStmt->stmt;
    SQLDESC_ID  *pInputDesc;
    SQLDESC_ID  *pOutputDesc;
    SQLMODULE_ID *pModule = &pSrvrStmt->moduleId;
    BOOL        sqlWarning;

    pStmt->version = SQLCLI_ODBC_VERSION;
    pStmt->module = pModule;
    pStmt->handle = 0;
    pStmt->charset = SQLCHARSETSTRING_UTF8;
    if (pSrvrStmt->stmtName[0] != '\0')
    {
        pStmt->name_mode = stmt_name;
        pStmt->identifier_len = pSrvrStmt->stmtNameLen;
        pStmt->identifier = pSrvrStmt->stmtName;
    }
    else
    {
        pStmt->name_mode = stmt_handle;
        pStmt->identifier_len = 0;
        pStmt->identifier = NULL;
    }

    if (pModule->module_name == NULL)
    {
        if (pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
        {
            retcode = WSQL_EXEC_AllocStmtForRS(pSrvrStmt->callStmtId, pSrvrStmt->resultSetIndex, pStmt);
            HANDLE_ERROR2(retcode, sqlWarning);
        }
        else
        {
            retcode = WSQL_EXEC_AllocStmt(pStmt,(SQLSTMT_ID *)NULL);
            HANDLE_ERROR(retcode, sqlWarning);
        }
    }

    pInputDesc = &pSrvrStmt->inputDesc;
    pInputDesc->version = SQLCLI_ODBC_VERSION;
    pInputDesc->handle = 0;
    pInputDesc->charset = SQLCHARSETSTRING_UTF8;
    if (pSrvrStmt->inputDescName[0] != '\0')
    {
        pInputDesc->name_mode = desc_name;
        pInputDesc->identifier_len = strlen(pSrvrStmt->inputDescName);
        pInputDesc->identifier = pSrvrStmt->inputDescName;
        pInputDesc->module = pModule;
    }
    else
    {
        pInputDesc->name_mode = desc_handle;
        pInputDesc->identifier_len = 0;
        pInputDesc->identifier = NULL;
        pInputDesc->module = &nullModule;
        retcode = WSQL_EXEC_AllocDesc(pInputDesc, (SQLDESC_ID *)NULL);
        HANDLE_ERROR(retcode, sqlWarning);
    }

    pOutputDesc = &pSrvrStmt->outputDesc;
    pOutputDesc->version = SQLCLI_ODBC_VERSION;
    pOutputDesc->handle = 0;
    pOutputDesc->charset = SQLCHARSETSTRING_UTF8;
    if (pSrvrStmt->outputDescName[0] != '\0')
    {
        pOutputDesc->name_mode = desc_name;
        pOutputDesc->identifier_len = strlen(pSrvrStmt->outputDescName);
        pOutputDesc->identifier = pSrvrStmt->outputDescName;
        pOutputDesc->module = pModule;
    }
    else
    {
        pOutputDesc->name_mode = desc_handle;
        pOutputDesc->identifier_len = 0;
        pOutputDesc->identifier = NULL;
        pOutputDesc->module = &nullModule;
        retcode = WSQL_EXEC_AllocDesc(pOutputDesc, (SQLDESC_ID *)NULL);
        HANDLE_ERROR(retcode, sqlWarning);
    }

    retcode = WSQL_EXEC_SetDescItem(pInputDesc, 0, SQLDESC_DESCRIPTOR_TYPE, DESCRIPTOR_TYPE_WIDE, NULL);
    HANDLE_ERROR(retcode, sqlWarning);
    retcode = WSQL_EXEC_SetDescItem(pOutputDesc, 0, SQLDESC_DESCRIPTOR_TYPE, DESCRIPTOR_TYPE_WIDE, NULL);
    HANDLE_ERROR(retcode, sqlWarning);

    CLI_DEBUG_RETURN_SQL(retcode);
}

SQLRETURN ALLOCSQLMXHDLS_SPJRS(SRVR_STMT_HDL *pSrvrStmt, SQLSTMT_ID *callpStmt, const char *RSstmtLabel)
{
    FUNCTION_ENTRY("ALLOCSQLMXHDLS_SPJRS", ("pSrvrStmt=0x%08x, callpStmt=0x%08x, RSstmtLabel=%s",
                pSrvrStmt,
                callpStmt,
                RSstmtLabel));
#if defined(TAG64)
    int _ptr32* tempStmtId;
#endif
    long retcode = SQL_SUCCESS;
    SQLSTMT_ID	*pStmt = &pSrvrStmt->stmt;
    SQLDESC_ID	*pOutputDesc;
    SQLMODULE_ID *pModule = &pSrvrStmt->moduleId;
    BOOL		sqlWarning;

    pStmt->version = SQLCLI_ODBC_VERSION;
    pStmt->module = pModule;
    pStmt->handle = 0;
    pStmt->charset = SQLCHARSETSTRING_ISO88591;
    if (pSrvrStmt->stmtName[0] != '\0')
    {
        pStmt->name_mode = stmt_name;
        pStmt->identifier_len = strlen(pSrvrStmt->stmtName);
        pStmt->identifier = pSrvrStmt->stmtName;
    }
    else
    {
        pStmt->name_mode = stmt_handle;
        pStmt->identifier_len = 0;
        pStmt->identifier = NULL;
    }
    DEBUG_OUT(DEBUG_LEVEL_STMT,("***pStmt->name_mode=%ld", pStmt->name_mode));
    DEBUG_OUT(DEBUG_LEVEL_STMT,("***pStmt->identifier_len=%ld", pStmt->identifier_len));
    DEBUG_OUT(DEBUG_LEVEL_STMT,("***pStmt->identifier=%s", pStmt->identifier));

    if (srvrGlobal->nowaitOn)
    {
#if defined(TAG64)
        tempStmtId=(int _ptr32*)malloc32(sizeof(int));
        pStmt->tag=(int)tempStmtId;
        tempStmtIdMap[(int)tempStmtId]=pSrvrStmt;

#else
        pStmt->tag = (long)pSrvrStmt;
#endif
    }
    else
        pStmt->tag = 0;
    if (pModule->module_name == NULL)
    {
        DEBUG_OUT(DEBUG_LEVEL_STMT,("***pModule->module_name == NULL  Call AllocStmtForRs()"));
        /* Commenting out for now - will be looked at when SPJ is supported
#ifdef NSK_PLATFORM
CLI_AllocStmtForRS(callpStmt,
pSrvrStmt->RSIndex,
pStmt);
#endif */
        if (retcode < 0)
        {
            CLI_ClearDiagnostics(NULL);
            CLI_DEBUG_RETURN_SQL(retcode);
        }
    }

    if (pSrvrStmt->useDefaultDesc)
    {
        pOutputDesc = &pSrvrStmt->outputDesc;
        pOutputDesc->version = SQLCLI_ODBC_VERSION;
        pOutputDesc->handle = 0;
        pOutputDesc->charset = SQLCHARSETSTRING_ISO88591;
        pOutputDesc->name_mode = desc_name;
        pOutputDesc->identifier_len = MAX_DESC_NAME_LEN;
        pOutputDesc->identifier = pSrvrStmt->outputDescName;
        pOutputDesc->module = pModule;
        retcode = CLI_ResDescName(pOutputDesc, pStmt, SQLWHAT_OUTPUT_DESC);
        if (retcode == -8803)
        {
            pOutputDesc->identifier_len = 0;
            retcode = CLI_ClearDiagnostics(NULL);
            retcode = 0;
        }
        if (retcode < 0)
        {
            CLI_ClearDiagnostics(NULL);
            CLI_DEBUG_RETURN_SQL(retcode);
        }
        pSrvrStmt->outputDescName[pOutputDesc->identifier_len] = '\0';
    }
    else
    {
        pOutputDesc = &pSrvrStmt->outputDesc;
        pOutputDesc->version = SQLCLI_ODBC_VERSION;
        pOutputDesc->handle = 0;
        pOutputDesc->charset = SQLCHARSETSTRING_ISO88591;
        pOutputDesc->name_mode = desc_handle;
        pOutputDesc->identifier_len = 0;
        pOutputDesc->identifier = NULL;
        pOutputDesc->module = pModule;
        retcode = CLI_AllocDesc(pOutputDesc, (SQLDESC_ID *)NULL);
        if (retcode < 0)
        {
            CLI_ClearDiagnostics(NULL);
            CLI_DEBUG_RETURN_SQL(retcode);
        }
    }

    if (srvrGlobal->nowaitOn)
    {
        retcode = CLI_AssocFileNumber(pStmt, srvrGlobal->nowaitFilenum);
        if (retcode < 0)
        {
            CLI_ClearDiagnostics(NULL);
            CLI_DEBUG_RETURN_SQL(retcode);
        }
    }

    // Set the output Desc to be Wide Descriptors
    if (pSrvrStmt->useDefaultDesc)
    {
        if (pSrvrStmt->outputDescName[0] != '\0')
        {
            DEBUG_OUT(DEBUG_LEVEL_CLI,("Default descriptor. Output Descriptor Name=%s",
                        pSrvrStmt->outputDescName));
            //R321: passing 1 instead of 0 for CLI_SetDescItem
            retcode = CLI_SetDescItem(pOutputDesc, 1, SQLDESC_DESCRIPTOR_TYPE, DESCRIPTOR_TYPE_WIDE, NULL);
            if (retcode < 0)
            {
                CLI_ClearDiagnostics(NULL);
                CLI_DEBUG_RETURN_SQL(retcode);
            }
        }
    }
    else
    {
        DEBUG_OUT(DEBUG_LEVEL_CLI,("Non-Default descriptor."));
        //R321: passing 1 instead of 0 for CLI_SetDescItem
        retcode = CLI_SetDescItem(pOutputDesc, 1, SQLDESC_DESCRIPTOR_TYPE, DESCRIPTOR_TYPE_WIDE, NULL);
        if (retcode < 0)
        {
            CLI_ClearDiagnostics(NULL);
            CLI_DEBUG_RETURN_SQL(retcode);
        }
    }
    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN EXECUTECALL(SRVR_STMT_HDL *pSrvrStmt)
{
    FUNCTION_ENTRY("EXECUTECALL",
            ("pSrvrStmt=0x%08x",
             pSrvrStmt));

    CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

    int 			rtn;
    long			columnCount;
    long			paramCount;
    long			retcode = SQL_SUCCESS;

    BOOL			sqlWarning = FALSE;
    SQLDESC_ID 		*pDesc;
    SQLDESC_ID		*pDescParam;
    SQLDESC_ID 		*pDescValue;
    SQLSTMT_ID 		*pStmt;
    SQLRETURN 		rc;

    pStmt      = &pSrvrStmt->stmt;

    DEBUG_OUT(DEBUG_LEVEL_STMT,("EXECUTECALL : isClosed = %ld", pSrvrStmt->isClosed));
    if (!pSrvrStmt->isClosed)
    {
        retcode = CLI_CloseStmt(pStmt);
        if (retcode!=0) retcode = CLI_ClearDiagnostics(pStmt);
        pSrvrStmt->isClosed = TRUE;
    }

    pDesc      = &pSrvrStmt->inputDesc;
    paramCount = pSrvrStmt->paramCount;

    if (paramCount > 0){
        pDescValue = pDesc;
    }
    else {
        pDescValue = NULL;
    }

    DEBUG_ASSERT(pSrvrStmt->isClosed, ("Server Statement is Open before int."));
    pSrvrStmt->isClosed = FALSE;

    retcode = CLI_Exec(pStmt, pDescValue, 0);
    DEBUG_OUT(DEBUG_LEVEL_STMT,("intCALL  CLI_EXEC  retcode: %ld.", retcode));
    HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
    pSrvrStmt->isClosed = FALSE;

#ifndef DISABLE_NOWAIT
    if (retcode == NOWAIT_PENDING){
        rtn = WaitForCompletion(pSrvrStmt, &pSrvrStmt->cond, &pSrvrStmt->mutex);
        DEBUG_OUT(DEBUG_LEVEL_CLI,("WaitForCompletion() returned %d",rtn));

        if (rtn == 0)
        {
            rc = pSrvrStmt->switchContext();
            DEBUG_OUT(DEBUG_LEVEL_CLI,("pSrvrStmt->switchContext() return with: %ld.", rc));
            if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) THREAD_RETURN(pSrvrStmt,rc);

            switch (pSrvrStmt->nowaitRetcode)
            {
                case 0:
                    retcode = 0;
                    break;
                case 9999:
                    pSrvrStmt->isClosed = TRUE;
                    THREAD_RETURN(pSrvrStmt,NOWAIT_ERROR);
                default:
                    pSrvrStmt->isClosed = TRUE;
                    retcode = GETSQLCODE(pSrvrStmt);
                    break;
            }
            DEBUG_OUT(DEBUG_LEVEL_CLI,("pSrvrStmt->nowaitRetcode=%ld, retcode=%s",
                        pSrvrStmt->nowaitRetcode,
                        CliDebugSqlError(retcode)));
        }
        else
        {
            pSrvrStmt->isClosed = TRUE;
            pSrvrStmt->nowaitRetcode = rtn;
            THREAD_RETURN(pSrvrStmt,NOWAIT_ERROR);
        }
    }
    else
#endif
    {
        if (retcode!=SQL_SUCCESS) pSrvrStmt->isClosed = TRUE;
    }

    // Note this could do a return
    HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

    pDesc       = &pSrvrStmt->outputDesc;
    columnCount = pSrvrStmt->columnCount;

    if (columnCount > 0) {
        pDescParam = pDesc;
    }
    else {
        pDescParam = NULL;
    }

    DEBUG_OUT(DEBUG_LEVEL_STMT,("***Anitha ---- >pSrvrStmt->isClosed=%ld", pSrvrStmt->isClosed));

    retcode = CLI_Fetch(pStmt, pDescParam, 0);

#ifndef DISABLE_NOWAIT
    if (retcode == NOWAIT_PENDING) {
        rtn = WaitForCompletion(pSrvrStmt, &pSrvrStmt->cond, &pSrvrStmt->mutex);
        DEBUG_OUT(DEBUG_LEVEL_CLI,("WaitForCompletion() returned %d",rtn));

        if (rtn == 0)
        {
            rc = pSrvrStmt->switchContext();
            DEBUG_OUT(DEBUG_LEVEL_CLI,("pSrvrStmt->switchContext() return with: %ld.", rc));
            if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) THREAD_RETURN(pSrvrStmt,rc);

            switch (pSrvrStmt->nowaitRetcode)
            {
                case 0:
                    retcode = 0;
                    break;
                case 9999:
                    pSrvrStmt->isClosed = TRUE;
                    THREAD_RETURN(pSrvrStmt,NOWAIT_ERROR);
                default:
                    pSrvrStmt->isClosed = TRUE;
                    retcode = GETSQLCODE(pSrvrStmt);
                    break;
            }
            DEBUG_OUT(DEBUG_LEVEL_CLI,("pSrvrStmt->nowaitRetcode=%ld, retcode=%s",
                        pSrvrStmt->nowaitRetcode,
                        CliDebugSqlError(retcode)));
        }
        else
        {
            pSrvrStmt->isClosed = TRUE;
            pSrvrStmt->nowaitRetcode = rtn;
            THREAD_RETURN(pSrvrStmt,NOWAIT_ERROR);
        }
    }
    else
#endif	
    {
        if (retcode!=SQL_SUCCESS) pSrvrStmt->isClosed = TRUE;
    }

    // Return if the fetch failed
    HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);


    // SPJRS - Only close when not SPJRS and not already closed
    if ((!pSrvrStmt->isSPJRS) && (!pSrvrStmt->isClosed))
    {
        retcode = CLI_CloseStmt(pStmt);
        // Set the close flag so that another close is not tried
        pSrvrStmt->isClosed = TRUE;
        HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
    }

    if (sqlWarning) THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);
    THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}


SQLRETURN CONNECT(SRVR_CONNECT_HDL *pSrvrConnect)
{
    FUNCTION_ENTRY("CONNECT",
            ("pSrvrConnect=0x%08x",
             pSrvrConnect));

    long retcode;
    BOOL sqlWarning = FALSE;
    retcode = CLI_CreateContext(&pSrvrConnect->contextHandle, NULL, 0);
    HANDLE_ERROR(retcode, sqlWarning);
    retcode = CLI_SwitchContext(pSrvrConnect->contextHandle, NULL);
    HANDLE_ERROR(retcode, sqlWarning);
    if (sqlWarning) CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN DISCONNECT(SRVR_CONNECT_HDL *pSrvrConnect)
{
    FUNCTION_ENTRY("DISCONNECT",
            ("pSrvrConnect=0x%08x",
             pSrvrConnect));

    long retcode;

    // Hack.
    // Executor starts an internal transaction to clean up volatile schemas
    // when a context is deleted. But it does not clean up properly when this
    // is called from the T2 driver. This is because transactions from the T2
    // interface are no-waited. TMF expects AWAITIOX to be called after the
    // ENDTRANSACTION to clean up the TFILE entry. However, Executor does not
    // make any AWAITIOX calls. So this affects the "active" transaction for
    // the process - pthreads thinks it is working on a user transaction when
    // it is actually working on the committed Executor transaction. This causes
    // the calling thread to hang.
    //
    // Designing a proper fix for this will take some time. For now, we hack
    // around this by resuming the user transaction after the Executor returns
    // from the DeleteContext call.
    //Start Soln. No.: 10-110830-9447
    short txHandle[10];

    long txBeginTag;


    retcode = CLI_DeleteContext(pSrvrConnect->contextHandle);

    // Resume the transaction before handling any errors from the disconnect.
    resumeTransaction(txBeginTag);
    //End Soln. No.: 10-110830-9447

    if (retcode==0) CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
}

SQLRETURN SWITCHCONTEXT(SRVR_CONNECT_HDL *pSrvrConnect, long *sqlcode)
{
    FUNCTION_ENTRY("SWITCHCONTEXT",
            ("pSrvrConnect=0x%08x, sqlcode=(out)",
             pSrvrConnect));
    long retcode;
    BOOL sqlWarning = FALSE;

    retcode = CLI_SwitchContext(pSrvrConnect->contextHandle, NULL);
    if (sqlcode != NULL)
        *sqlcode = retcode;
    HANDLE_ERROR(retcode, sqlWarning);
    if (sqlWarning) CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN GETSQLCODE(SRVR_STMT_HDL *pSrvrStmt)
{
    FUNCTION_ENTRY("GETSQLCODE",("pSrvrStmt=0x%08x",pSrvrStmt));
    long retcode;
    int sqlcode  = 0;

    retcode = CLI_GetDiagnosticsCondInfo2(SQLDIAG_SQLCODE,
            1, &sqlcode, NULL, 0, NULL);
    if (retcode >= SQL_SUCCESS)
        CLI_DEBUG_RETURN_SQL(sqlcode);
    CLI_DEBUG_RETURN_SQL(retcode);
}

// MFC
// Input Descriptor Info here for creating MDF file

InputDescInfo::InputDescInfo()
{
    CountPosition = 0;
    DataType = 0;
    memset(DataTypeString, '\0', 50);
    Length = 0;
    DateTimeCode = 0;
    Precision = 0;
    SQLCharset = 0;
    ODBCPrecision = 0;
    ODBCDataType = 0;
    Scale = 0;
    Nullable = 0;
    IntLeadPrec = 0;
}

InputDescInfo::~InputDescInfo()
{
    CountPosition = 0;
    DataType = 0;
    memset(DataTypeString, '\0', 50);
    Length = 0;
    DateTimeCode = 0;
    Precision = 0;
    SQLCharset = 0;
    ODBCPrecision = 0;
    ODBCDataType = 0;
    Nullable = 0;
    Scale = 0;
    IntLeadPrec =0;
}

// MFC - method to map JDBC data types to Trafodion data types

void InputDescInfo::setData(int countPosition, long dataType, long length, long scale,long nullable,
        long dateTimeCode, long precision,long intLeadPrec, long sQLCharset, SRVR_GLOBAL_Def *srvrGlobal)
{
    CountPosition = countPosition;
    DataType = dataType;
    Length = length;
    DateTimeCode = dateTimeCode;
    Precision = precision;
    SQLCharset = sQLCharset;
    Scale = scale;
    Nullable = nullable;
    IntLeadPrec = intLeadPrec;
    switch (DataType)
    {
        case SQLTYPECODE_CHAR:
            ODBCPrecision = Length;
            ODBCDataType = SQL_CHAR;
            strcpy(DataTypeString, "char");
            break;
        case SQLTYPECODE_VARCHAR:
        case SQLTYPECODE_VARCHAR_WITH_LENGTH:
            ODBCPrecision = Length;
            ODBCDataType = SQL_VARCHAR;
            strcpy(DataTypeString, "VARCHAR");
            if (Length >= 255)
            {
                ODBCDataType = SQL_LONGVARCHAR;
                strcpy(DataTypeString, "VARCHAR");
            }
            break;
        case SQLTYPECODE_VARCHAR_LONG:
            ODBCPrecision = Length;
            ODBCDataType = SQL_LONGVARCHAR;
            strcpy(DataTypeString, "VARCHAR");
            break;
        case SQLTYPECODE_SMALLINT:
            if (Precision == 0)
            {
                ODBCPrecision = 5;
                ODBCDataType = SQL_SMALLINT;
                strcpy(DataTypeString, "short");
            }
            else
            {
                ODBCPrecision = Precision;
                ODBCDataType = SQL_NUMERIC;
                strcpy(DataTypeString, "NUMERIC");
            }
            break;
        case SQLTYPECODE_SMALLINT_UNSIGNED:
            if (Precision == 0)
            {
                ODBCPrecision = 5;
                ODBCDataType = SQL_SMALLINT;
                strcpy(DataTypeString, "unsigned short");
            }
            else
            {
                ODBCPrecision = Precision;
                ODBCDataType = SQL_NUMERIC;
                strcpy(DataTypeString, "unsigned NUMERIC");
            }
            break;
        case SQLTYPECODE_INTEGER:
            if (Precision == 0)
            {
                ODBCPrecision = 10;
                ODBCDataType = SQL_INTEGER;
                strcpy(DataTypeString, "int");
            }
            else
            {
                ODBCPrecision = Precision;
                ODBCDataType = SQL_NUMERIC;
                strcpy(DataTypeString, "NUMERIC");
            }
            break;
        case SQLTYPECODE_INTEGER_UNSIGNED:
            if (Precision == 0)
            {
                ODBCPrecision = 10;
                ODBCDataType = SQL_INTEGER;
                strcpy(DataTypeString, "unsigned int");

            }
            else
            {
                ODBCPrecision = Precision;
                ODBCDataType = SQL_NUMERIC;
                strcpy(DataTypeString, "unsigned NUMERIC");
            }
            break;
        case SQLTYPECODE_LARGEINT:
            if (Precision == 0)
            {
                ODBCPrecision = 19;
                ODBCDataType = SQL_BIGINT;
                strcpy(DataTypeString, "long long");

            }
            else
            {
                ODBCPrecision = Precision;
                ODBCDataType = SQL_NUMERIC;
                strcpy(DataTypeString, "NUMERIC");
            }
            break;
        case SQLTYPECODE_IEEE_REAL:
        case SQLTYPECODE_TDM_REAL:
            ODBCDataType = SQL_REAL;
            ODBCPrecision = 7;
            strcpy(DataTypeString, "float");
            break;
        case SQLTYPECODE_IEEE_DOUBLE:
        case SQLTYPECODE_TDM_DOUBLE:
            ODBCDataType = SQL_DOUBLE;
            ODBCPrecision = 15;
            strcpy(DataTypeString, "double");
            break;
        case SQLTYPECODE_DATETIME:
            switch (DateTimeCode)
            {
                case SQLDTCODE_DATE:					//1
                    ODBCDataType = SQL_DATE;
                    ODBCPrecision = 10;
                    strcpy(DataTypeString, "DATE");
                    break;
                case SQLDTCODE_TIME:					//2
                    ODBCDataType = SQL_TIME;
                    strcpy(DataTypeString, "TIME");
                    if (Precision == 0)
                    {
                        ODBCPrecision = 8;
                    }
                    else
                    {
                        ODBCDataType = SQL_TIMESTAMP;
                        ODBCPrecision = 20+Precision;
                        strcpy(DataTypeString, "TIMESTAMP");
                    }
                    break;
                case SQLDTCODE_TIMESTAMP:				//3
                    ODBCDataType = SQL_TIMESTAMP;
                    strcpy(DataTypeString, "TIMESTAMP");
                    if (Precision == 0)
                    {
                        ODBCPrecision = 19;
                    }
                    else
                    {
                        ODBCPrecision = 20+Precision;
                    }
                    break;
                    //
                    // Mapping Non-standard SQL/MP DATETIME types to DATE/TIME/TIMESTAMP
                    //

                default:
                    ODBCDataType = SQL_TYPE_NULL;
                    ODBCPrecision = 0;
                    strcpy(DataTypeString, "SQL_TYPE_NULL");
                    break;
            } // switch datetime ends
            break;
        case SQLTYPECODE_DECIMAL_UNSIGNED:
            ODBCPrecision = Length;
            ODBCDataType = SQL_DECIMAL;
            strcpy(DataTypeString, "unsigned DECIMAL");
            break;
        case SQLTYPECODE_DECIMAL:
            ODBCPrecision = Length;
            ODBCDataType = SQL_DECIMAL;
            strcpy(DataTypeString, "DECIMAL");
            break;
        case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
            ODBCDataType = SQL_DOUBLE; // Since there is no corresponding JDBC DataType, Map it as a double
            ODBCPrecision = 15;
            strcpy(DataTypeString, "double");
            break;
        case SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
            ODBCDataType = SQL_DOUBLE; // Since there is no corresponding JDBC DataType, Map it as a double
            ODBCPrecision = 15;
            strcpy(DataTypeString, "double");
            break;
        case SQLTYPECODE_INTERVAL:		// Interval will be sent in ANSIVARCHAR format
            switch (DateTimeCode)
            {
                case SQLINTCODE_YEAR:
                    ODBCDataType = SQL_INTERVAL_YEAR;
                    ODBCPrecision = 0;
                    strcpy(DataTypeString, "INTERVAL YEAR");
                    break;
                case SQLINTCODE_MONTH:
                    ODBCDataType = SQL_INTERVAL_MONTH;
                    ODBCPrecision = 0;
                    strcpy(DataTypeString, "INTERVAL MONTH");
                    break;
                case SQLINTCODE_DAY:
                    ODBCDataType = SQL_INTERVAL_DAY;
                    ODBCPrecision = 0;
                    strcpy(DataTypeString, "INTERVAL DAY");
                    break;
                case SQLINTCODE_HOUR:
                    ODBCDataType = SQL_INTERVAL_HOUR;
                    ODBCPrecision = 0;
                    strcpy(DataTypeString, "INTERVAL HOUR");
                    break;
                case SQLINTCODE_MINUTE:
                    ODBCDataType = SQL_INTERVAL_MINUTE;
                    ODBCPrecision = 0;
                    strcpy(DataTypeString, "INTERVAL MINUTE");
                    break;
                case SQLINTCODE_SECOND:
                    ODBCDataType = SQL_INTERVAL_SECOND;
                    ODBCPrecision = Precision;
                    strcpy(DataTypeString, "INTERVAL SECOND");
                    break;
                case SQLINTCODE_YEAR_MONTH:
                    ODBCDataType = SQL_INTERVAL_YEAR_TO_MONTH;
                    ODBCPrecision = 0;
                    strcpy(DataTypeString, "INTERVAL YEAR TO MONTH");
                    break;
                case SQLINTCODE_DAY_HOUR:
                    ODBCDataType = SQL_INTERVAL_DAY_TO_HOUR;
                    ODBCPrecision = 0;
                    strcpy(DataTypeString, "INTERVAL DAY TO HOUR");
                    break;
                case SQLINTCODE_DAY_MINUTE:
                    ODBCDataType = SQL_INTERVAL_DAY_TO_MINUTE;
                    ODBCPrecision = 0;
                    strcpy(DataTypeString, "INTERVAL DAY TO MINUTE");
                    break;
                case SQLINTCODE_DAY_SECOND:
                    ODBCDataType = SQL_INTERVAL_DAY_TO_SECOND;
                    ODBCPrecision = Precision;
                    strcpy(DataTypeString, "INTERVAL DAY TO SECOND");
                    break;
                case SQLINTCODE_HOUR_MINUTE:
                    ODBCDataType = SQL_INTERVAL_HOUR_TO_MINUTE;
                    ODBCPrecision = 0;
                    strcpy(DataTypeString, "INTERVAL HOUR TO MINUTE");
                    break;
                case SQLINTCODE_HOUR_SECOND:
                    ODBCDataType = SQL_INTERVAL_HOUR_TO_SECOND;
                    ODBCPrecision = Precision;
                    strcpy(DataTypeString, "INTERVAL HOUR TO SECOND");
                    break;
                case SQLINTCODE_MINUTE_SECOND:
                    ODBCDataType = SQL_INTERVAL_MINUTE_TO_SECOND;
                    ODBCPrecision = Precision;
                    strcpy(DataTypeString, "INTERVAL MINUTE TO SECOND");
                    break;
                default:
                    ODBCDataType = SQL_TYPE_NULL;
                    ODBCPrecision = 0;
                    strcpy(DataTypeString, "SQL_TYPE_NULL");
                    break;
            }
            break;
        default:
            ODBCDataType = SQL_TYPE_NULL;
            ODBCPrecision = 0;
            strcpy(DataTypeString, "SQL_TYPE_NULL");
            break;
    }
}

// MFC BuildDesc and return InputDescInfo

SQLRETURN BuildSQLDesc(SRVR_STMT_HDL*pSrvrStmt, SRVR_STMT_HDL::DESC_TYPE descType,InputDescInfo *pInputDescInfo)
{
    FUNCTION_ENTRY("BuildSQLDesc", ("pSrvrStmt=0x%08x, pSrvrStmt->stmtName = %s, SQL Statement = %s, descType=%s",
                pSrvrStmt,
                pSrvrStmt->stmtName,
                pSrvrStmt->sqlString.dataValue._buffer,
                CliDebugDescTypeStr(descType)));

    long retcode = SQL_SUCCESS;
    short i;
    short j;
    short k;

    BOOL sqlWarning = FALSE;

    long *totalMemLen = pSrvrStmt->getDescBufferLenPtr(descType);
    long numEntries = pSrvrStmt->getDescEntryCount(descType);
    SQLDESC_ID *pDesc = pSrvrStmt->getDesc(descType);
    SQLItemDescList_def *SQLDesc = pSrvrStmt->getDescList(descType);
    SQLItemDesc_def *SQLItemDesc = (SQLItemDesc_def *)SQLDesc->_buffer + SQLDesc->_length;

    SRVR_DESC_HDL *implDesc = pSrvrStmt->allocImplDesc(descType);

    // The following routine is hard coded for at least 15 items, so make sure it does not change
    DEBUG_ASSERT(NO_OF_DESC_ITEMS>= 16,("NO_OF_DESC_ITEMS(%d) is less than 16",NO_OF_DESC_ITEMS));
    *totalMemLen = 0;
    for (i = 0; i < numEntries; i++) {
        SQLItemDesc = (SQLItemDesc_def *)SQLDesc->_buffer + SQLDesc->_length;
        // Initialize the desc entry in SQLDESC_ITEM struct
        for (j = 0; j < NO_OF_DESC_ITEMS ; j++) {
            gDescItems[j].entry = i+1;
        }
        gDescItems[10].num_val_or_len = MAX_ANSI_NAME_LEN+1;
        gDescItems[11].num_val_or_len = MAX_ANSI_NAME_LEN+1;
        gDescItems[12].num_val_or_len = MAX_ANSI_NAME_LEN+1;
        gDescItems[13].num_val_or_len = MAX_ANSI_NAME_LEN+1;
        gDescItems[14].num_val_or_len = MAX_ANSI_NAME_LEN+1;

        retcode = CLI_GetDescItems2(pDesc,
                NO_OF_DESC_ITEMS,
                (SQLDESC_ITEM *)&gDescItems);
        HANDLE_ERROR(retcode, sqlWarning);

        SQLItemDesc->dataType     = gDescItems[0].num_val_or_len;
        SQLItemDesc->maxLen       = gDescItems[1].num_val_or_len;
        SQLItemDesc->precision    = (short)gDescItems[2].num_val_or_len;
        SQLItemDesc->scale        = (short)gDescItems[3].num_val_or_len;
        SQLItemDesc->nullInfo     = (BOOL)gDescItems[4].num_val_or_len;
        SQLItemDesc->paramMode    = gDescItems[5].num_val_or_len;
        SQLItemDesc->intLeadPrec  = gDescItems[6].num_val_or_len;
        SQLItemDesc->datetimeCode = gDescItems[7].num_val_or_len;
        SQLItemDesc->SQLCharset   = gDescItems[8].num_val_or_len;
        SQLItemDesc->fsDataType   = gDescItems[9].num_val_or_len;
        for (k = 10; k < 15; k++) {
            gDescItems[k].string_val[gDescItems[k].num_val_or_len] = '\0';
        }
        SQLItemDesc->vc_ind_length = gDescItems[15].num_val_or_len;

        SQLItemDesc->maxLen = AdjustCharLength(descType, SQLItemDesc->SQLCharset, SQLItemDesc->maxLen);

        GetJDBCValues(	SQLItemDesc, 			// Input
                *totalMemLen,
                gDescItems[14].string_val);

        implDesc[i].charSet         = SQLItemDesc->SQLCharset;
        implDesc[i].dataType        = SQLItemDesc->dataType;
        implDesc[i].length          = SQLItemDesc->maxLen;
        implDesc[i].precision       = SQLItemDesc->ODBCPrecision;
        implDesc[i].scale           = SQLItemDesc->scale;
        implDesc[i].sqlDatetimeCode = SQLItemDesc->datetimeCode;
        implDesc[i].FSDataType      = SQLItemDesc->fsDataType;
        implDesc[i].paramMode       = SQLItemDesc->paramMode;
        implDesc[i].vc_ind_length   = SQLItemDesc->vc_ind_length;

        SQLItemDesc->version = 0;

        strcpy(SQLItemDesc->CatalogName, gDescItems[10].string_val);
        strcpy(SQLItemDesc->SchemaName, gDescItems[11].string_val);
        strcpy(SQLItemDesc->TableName, gDescItems[12].string_val);
        strcpy(SQLItemDesc->ColumnName, gDescItems[13].string_val);
        strcpy(SQLItemDesc->ColumnLabel, gDescItems[14].string_val);

        SQLDesc->_length++;

    }

    /*
       if ((srvrGlobal->moduleCaching) &&(descType == SRVR_STMT_HDL::Input)&& (pSrvrStmt->stmtType == EXTERNAL_STMT))
       {
       retcode = BuildSQLDesc2ForModFile(pSrvrStmt->inputDesc, pSrvrStmt->paramCount, pInputDescInfo);
       HANDLE_ERROR(retcode, sqlWarning);
       }
       */
    retcode = SET_DATA_PTR(pSrvrStmt, descType);
    HANDLE_ERROR(retcode, sqlWarning);

    if (sqlWarning) CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN COMMIT_ROWSET(long dialogueId, bool& bSQLMessageSet, odbc_SQLSvc_SQLError* SQLError, Int32 currentRowCount)
{
    SQLRETURN retcode;
    long      sqlcode;
    SQLValueList_def inValueList;
    inValueList._buffer = NULL;
    inValueList._length = 0;

    SRVR_STMT_HDL *CmwSrvrStmt = getInternalSrvrStmt(dialogueId, "STMT_COMMIT_1", &sqlcode);
    /* Should process the error here if CmwSrvrStmt is NULL */
    if(!CmwSrvrStmt || sqlcode == SQL_INVALID_HANDLE)
    {
        kdsCreateSQLErrorException(SQLError, 1, bSQLMessageSet);
        kdsCopySQLErrorExceptionAndRowCount(SQLError,
                "Internal Error: From Commit Rowsets, getInternalSrvrStmt() failed to get the prepared \"STMT_COMMIT_1\" statement",
                sqlcode,
                "HY000",
                currentRowCount+1);
    }

    // This should be changed to use the rowsets Execute() function once the code is checked in
    retcode = CmwSrvrStmt->Execute(NULL,1,TYPE_UNKNOWN,&inValueList,SQL_ASYNC_ENABLE_OFF,0, /* */NULL);
    if (retcode == SQL_ERROR)
    {
        ERROR_DESC_def *error_desc_def = CmwSrvrStmt->sqlError.errorList._buffer;
        if (CmwSrvrStmt->sqlError.errorList._length != 0 )
        {
            if(error_desc_def->sqlcode != -8605 )
            {
                kdsCreateSQLErrorException(SQLError, 1, bSQLMessageSet);
                kdsCopySQLErrorExceptionAndRowCount(SQLError, error_desc_def->errorText, error_desc_def->sqlcode, error_desc_def->sqlstate, currentRowCount+1);
            }
            else
                retcode = SQL_SUCCESS;
        }
        else
        {
            kdsCreateSQLErrorException(SQLError, 1, bSQLMessageSet);
            kdsCopySQLErrorExceptionAndRowCount(SQLError, "Internal Error: From Commit Rowsets ", retcode, "", currentRowCount+1);
        }
    }
    else if (retcode != SQL_SUCCESS)
    {
        kdsCreateSQLErrorException(SQLError, 1, bSQLMessageSet);
        kdsCopySQLErrorExceptionAndRowCount(SQLError, "Internal Error: From Commit Rowsets ", retcode, "", currentRowCount+1);
    }

    THREAD_RETURN(CmwSrvrStmt,retcode);
}

SQLRETURN GETSQLWARNINGORERROR2(SRVR_STMT_HDL* pSrvrStmt)
{
    FUNCTION_ENTRY("GETSQLWARNINGORERROR2",
            ("pSrvrStmt=0x%08x, pSrvrStmt->stmtName=%s, pSrvrStmt->sqlStringText=%s",
             pSrvrStmt, pSrvrStmt->stmtName, pSrvrStmt->sqlStringText, isFromExecDirect));

    Int32 retcode;
    Int32 total_conds = 0;
    char *msg_buf=NULL;
    Int32 buf_len;
    Int32 sqlcode  = 0;
    char sqlState[6];
    Int32 curr_cond = 1;
    Int32 skipped_conds = 0;
    Int32 msg_buf_len = 0;
    Int32 msg_total_len = 0,Tot_Alloc_Buffer_len = 0;
    Int32 rowId = 0; // use this for rowset recovery.
    char  strNow[TIMEBUFSIZE + 1];

    pSrvrStmt->sqlWarningOrErrorLength = 0;
    retcode =  WSQL_EXEC_GetDiagnosticsStmtInfo2(NULL, SQLDIAG_NUMBER, (Int64*)&total_conds, NULL, 0, NULL);

    // Calculate Total Buffer to allocate
    while(curr_cond <= total_conds)
    {
        retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_SQLCODE, curr_cond,	&sqlcode, NULL, 0, NULL);

        // We are not copying the Warning message if the error code is 100
        // An Error Code -8916 or -8915 is how the UDR server signals that there are no more result sets
        // There is no need to pass this error to the client
        if(sqlcode != -8916 && sqlcode != -8915 && sqlcode != 100)
        {
            Tot_Alloc_Buffer_len += sizeof(total_conds) + sizeof(rowId) + sizeof(sqlcode);
            Tot_Alloc_Buffer_len += sizeof(sqlState) + sizeof(msg_buf_len);
            // The above call returns the number of "characters" in the error message text.
            // Incase of multibyte characters, one character can have upto 4 bytes each.
            retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_MSG_LEN, curr_cond,
                    &msg_buf_len, NULL, 0, NULL);

            Tot_Alloc_Buffer_len += (msg_buf_len + 1)*4 + TIMEBUFSIZE; //*4 To take care of multi-byte characters
            //TIMEBUFSIZE for timestamp
        }
        else
        {
            skipped_conds++;
        }

        curr_cond++;
    }

    MEMORY_ALLOC_ARRAY(pSrvrStmt->sqlWarningOrError, BYTE, Tot_Alloc_Buffer_len);

    *(Int32 *)(pSrvrStmt->sqlWarningOrError+msg_total_len) = total_conds - skipped_conds;
    msg_total_len += sizeof(total_conds);

    curr_cond = 1;
    while (curr_cond <= total_conds)
    {
        // Need to get the sqlcode ahead, so that we can skip diagnostics that we are not interested in
        WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_SQLCODE, curr_cond,	&sqlcode, NULL, 0, NULL);
        if(sqlcode == -8915 || sqlcode == -8916 || sqlcode == 100)
        {
            curr_cond++;
            continue;
        }

        retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_ROW_NUMBER, curr_cond, &rowId, NULL, 0, NULL);
        if (retcode < SQL_SUCCESS)
            rowId = -1;
        else if(rowId != -1)
            rowId++;  //indexing starts at 0
        *(Int32 *)(pSrvrStmt->sqlWarningOrError+msg_total_len) = rowId;
        msg_total_len += sizeof(rowId);

        *(Int32 *)(pSrvrStmt->sqlWarningOrError+msg_total_len) = sqlcode;
        msg_total_len += sizeof(sqlcode);

        if (retcode >= SQL_SUCCESS)
        {
            msg_buf_len = 0;
            retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_MSG_LEN, curr_cond,
                    &msg_buf_len, NULL, 0, NULL);
            msg_buf_len += 1; // Null terminator
        }
        if (retcode >= SQL_SUCCESS )
        {
            // The above call returns the number of "characters" in the error message text.
            // Incase of multibyte characters, one character can have upto 4 bytes each.
            // Only for ISO88591 configuration msg_buf_len and buf_len (see below) will be the same.
            // Incase the error message has multi-byte characters, we need to be sure
            // of providing enough buffer to hold the error message
            msg_buf_len = msg_buf_len*4;	//max, for multibyte characters
            MEMORY_ALLOC_ARRAY(msg_buf, char, msg_buf_len);
            buf_len = 0;
            retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_MSG_TEXT, curr_cond,
                    NULL, msg_buf, msg_buf_len, &buf_len);
            //Note that the buf_len always has the correct number of returned bytes.
            msg_buf[buf_len] = '\0';
            msg_buf_len = buf_len +1;
            //Get the timetsamp
            time_t  now = time(NULL);
            bzero(strNow, sizeof(strNow) );
            strftime(strNow, sizeof(strNow), " [%Y-%m-%d %H:%M:%S]", localtime(&now));
            strcat(msg_buf, strNow);
            msg_buf_len += TIMEBUFSIZE;
            *(Int32*)(pSrvrStmt->sqlWarningOrError+msg_total_len) = msg_buf_len;
            msg_total_len += sizeof(msg_buf_len);
            memcpy(pSrvrStmt->sqlWarningOrError+msg_total_len, msg_buf, msg_buf_len);
            msg_total_len += msg_buf_len;
        }
        else
            THREAD_RETURN(pSrvrStmt,retcode);

        if (retcode >= SQL_SUCCESS)
        {
            buf_len = 0;
            sqlState[0] = '\0';
            retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_RET_SQLSTATE, curr_cond,
                    NULL, sqlState, sizeof(sqlState), &buf_len);
            sqlState[5] = '\0';
            memcpy(pSrvrStmt->sqlWarningOrError+msg_total_len, sqlState, sizeof(sqlState));
            msg_total_len += sizeof(sqlState);
        }

        // We'll add 2034 error to sqlErrorExit array only if on a file system error 31
        if(sqlcode == -2034)
        {
            if( strstr((const char *)msg_buf, (const char *)"error 31" ) != NULL && errorIndex < 8 )
            {
                sqlErrorExit[errorIndex++] = sqlcode;
            }
        }

        if (msg_buf != NULL) MEMORY_DELETE_ARRAY(msg_buf);
        curr_cond++;
    }

    WSQL_EXEC_ClearDiagnostics(NULL);
    pSrvrStmt->sqlWarningOrErrorLength = msg_total_len;

    retcode = SQL_SUCCESS;
ret:
    THREAD_RETURN(pSrvrStmt,retcode);
}  // end GETSQLWARNINGORERROR2

//---------------------------------------------------------------
SQLRETURN GETSQLWARNINGORERROR2forRowsets(SRVR_STMT_HDL* pSrvrStmt)
{
    FUNCTION_ENTRY("GETSQLWARNINGORERROR2forRowsets",
            ("pSrvrStmt=0x%08x, pSrvrStmt->stmtName=%s, pSrvrStmt->sqlStringText=%s",
             pSrvrStmt, pSrvrStmt->stmtName, pSrvrStmt->sqlStringText, isFromExecDirect));

    Int32  retcode;
    Int32  total_conds          = 0;
    char *msg_buf              = NULL;
    Int32  buf_len;
    Int32  sqlcode              = 0;
    char  sqlState[6];
    Int32  curr_cond            = 1;
    Int32  msg_buf_len          = 0;
    Int32  msg_total_len        = 0;
    Int32  Tot_Alloc_Buffer_len = 0;
    Int32  rowId                = 0;

    pSrvrStmt->sqlWarningOrErrorLength = 0;
    total_conds = pSrvrStmt->sqlWarning._length;

    retcode = 0; //WSQL_EXEC_GetDiagnosticsStmtInfo2(NULL, SQLDIAG_NUMBER, &total_conds, NULL, 0, NULL);

    // Calculate Total Buffer to allocate
    while(curr_cond <= total_conds)
    {

        if (sqlcode != 100)
        {
            Tot_Alloc_Buffer_len += sizeof(total_conds) + sizeof(rowId) + sizeof(sqlcode);
            sqlcode               = pSrvrStmt->sqlWarning._buffer[curr_cond - 1].sqlcode;
            Tot_Alloc_Buffer_len += sizeof(sqlState) + sizeof(msg_buf_len);
            msg_buf_len           = strlen(pSrvrStmt->sqlWarning._buffer[curr_cond - 1].errorText);
            Tot_Alloc_Buffer_len += msg_buf_len + 1;
        }
        curr_cond++;
    }

    curr_cond = 1;

   MEMORY_ALLOC_ARRAY(pSrvrStmt->sqlWarningOrError, BYTE, Tot_Alloc_Buffer_len);

    if (pSrvrStmt->sqlWarningOrError == NULL)
    {
        exit(1);
    }

    *(Int32 *)(pSrvrStmt->sqlWarningOrError+msg_total_len) = total_conds;
    msg_total_len += sizeof(total_conds);

    while (curr_cond <= total_conds)
    {
        if (retcode >= SQL_SUCCESS)
        {
            if (sqlcode == 100)
            {
                // We are not copying the Warning message if the error code is 100
                // It is ok, though we have allocated more SQLError, but length is incremented
                // only when SQLError is copied
                curr_cond++;
                continue;
            }
            rowId = pSrvrStmt->sqlWarning._buffer[curr_cond - 1].rowId;
            *(Int32 *)(pSrvrStmt->sqlWarningOrError+msg_total_len) = rowId;
            msg_total_len += sizeof(rowId);
            sqlcode = pSrvrStmt->sqlWarning._buffer[curr_cond - 1].sqlcode;
            *(Int32 *)(pSrvrStmt->sqlWarningOrError+msg_total_len) = sqlcode;
            msg_total_len += sizeof(sqlcode);
            msg_buf_len  = 0;
            msg_buf_len  = strlen(pSrvrStmt->sqlWarning._buffer[curr_cond - 1].errorText);
            msg_buf_len += 1; // Null terminator
        }
        if (retcode >= SQL_SUCCESS )
        {
            MEMORY_ALLOC_ARRAY(msg_buf, char, msg_buf_len);
            buf_len = 0;
            memcpy(msg_buf, pSrvrStmt->sqlWarning._buffer[curr_cond - 1].errorText, msg_buf_len);
            msg_buf[msg_buf_len-1] = '\0';
            *(Int32*)(pSrvrStmt->sqlWarningOrError+msg_total_len) = msg_buf_len;
            msg_total_len += sizeof(msg_buf_len);
            memcpy(pSrvrStmt->sqlWarningOrError+msg_total_len, msg_buf, msg_buf_len);
            msg_total_len += msg_buf_len;
        }
        else
            THREAD_RETURN(pSrvrStmt,retcode);

        if (retcode >= SQL_SUCCESS)
        {
            buf_len = 0;
            sqlState[0] = '\0';
            memcpy(sqlState, pSrvrStmt->sqlWarning._buffer[curr_cond - 1].sqlstate, 5);
            sqlState[5] = '\0';
            memcpy(pSrvrStmt->sqlWarningOrError+msg_total_len, sqlState, sizeof(sqlState));
            msg_total_len += sizeof(sqlState);
        }

        // We'll add 2034 error to sqlErrorExit array only if on a file system error 31
        if(sqlcode == -2034)
        {
            if( strstr((const char *)msg_buf, (const char *)"error 31" ) != NULL && errorIndex < 8 )
            {
                sqlErrorExit[errorIndex++] = sqlcode;
            }
        }

        if (msg_buf != NULL) MEMORY_DELETE_ARRAY(msg_buf);
        curr_cond++;
    }  // end while

    pSrvrStmt->sqlWarningOrErrorLength = msg_total_len;

    retcode = SQL_SUCCESS;
ret:
    THREAD_RETURN(pSrvrStmt,retcode);
}  // end GETSQLWARNINGORERROR2forRowsets

SQLRETURN PREPARE_ROWSETS(SRVR_STMT_HDL* pSrvrStmt, bool isFromExecDirect)
{
    FUNCTION_ENTRY("PREPARE",
            ("pSrvrStmt=0x%08x, isFromExecDirect=%d",
             pSrvrStmt, isFromExecDirect));

    CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

    Int32 retcode;
    UInt32 flags = 0;
    if(isFromExecDirect)
        flags = flags | PREPARE_STANDALONE_QUERY;

    char *pSqlStr = pSrvrStmt->sqlStringText;

    SQLSTMT_ID	*pStmt;
    SQLDESC_ID	*pInputDesc;
    SQLDESC_ID	*pOutputDesc;

    char        *pStmtName;
    BOOL		sqlWarning = FALSE;
    BOOL		rgWarning = FALSE;
    BOOL		shapeWarning = FALSE;
    Int32		tempmaxRowsetSize = 0;
    Int32 		holdableCursor = pSrvrStmt->holdableCursor;

    pSrvrStmt->PerfFetchRetcode = SQL_SUCCESS;
    pSrvrStmt->RowsetFetchRetcode = SQL_SUCCESS;

    pStmt = &pSrvrStmt->stmt;
    pOutputDesc = &pSrvrStmt->outputDesc;
    pInputDesc = &pSrvrStmt->inputDesc;

    SQLDESC_ID	sqlString_desc;
    sqlString_desc.version = SQLCLI_ODBC_VERSION;
    sqlString_desc.module = &pSrvrStmt->moduleId;
    sqlString_desc.name_mode = string_data;
    sqlString_desc.identifier     = (const char *) pSqlStr;
    sqlString_desc.handle = 0;
    sqlString_desc.identifier_len = pSrvrStmt->sqlStringLen;
    sqlString_desc.charset = SQLCHARSETSTRING_UTF8;
    UInt32 tmpBuildID = 0;


    if (srvrGlobal->drvrVersion.buildId & ROWWISE_ROWSET)
        retcode = WSQL_EXEC_SetDescItem(pOutputDesc, 0, SQLDESC_ROWSET_TYPE, 3, 0);
    else
        retcode = WSQL_EXEC_SetDescItem(pOutputDesc, 0, SQLDESC_ROWSET_TYPE, 0, 0);
    HANDLE_ERROR(retcode, sqlWarning);

    // We have to do the following CLI to set rowsize to ZERO, Since we switch between rowset to non-rowsets
    retcode = WSQL_EXEC_SetStmtAttr(pStmt,SQL_ATTR_INPUT_ARRAY_MAXSIZE,tempmaxRowsetSize, NULL);
    HANDLE_ERROR(retcode, sqlWarning);

    // need to set HOLDABLE cursor only when it differs from current holdable cursor
    if (holdableCursor != pSrvrStmt->current_holdableCursor) {
        retcode =  WSQL_EXEC_SetStmtAttr(pStmt,SQL_ATTR_CURSOR_HOLDABLE,holdableCursor, NULL);
        pSrvrStmt->current_holdableCursor = holdableCursor;
        HANDLE_ERROR(retcode, sqlWarning);
    }

    if (pSrvrStmt->sqlQueryType != SQL_SP_RESULT_SET)
    {
        // Result sets are already prepared.
        pSrvrStmt->sqlUniqueQueryIDLen = MAX_QUERY_NAME_LEN;
        pSrvrStmt->sqlUniqueQueryID[0] = '\0';
        retcode = WSQL_EXEC_Prepare2(pStmt, &sqlString_desc
                , NULL, NULL, NULL, &pSrvrStmt->cost_info
                , &pSrvrStmt->comp_stats_info, pSrvrStmt->sqlUniqueQueryID
                , &pSrvrStmt->sqlUniqueQueryIDLen
                , flags);
        pSrvrStmt->sqlUniqueQueryID[pSrvrStmt->sqlUniqueQueryIDLen] = '\0';
        pSrvrStmt->m_bNewQueryId = true;

        HANDLE_ERROR(retcode, sqlWarning);
    }

    retcode = WSQL_EXEC_DescribeStmt(pStmt, pInputDesc, pOutputDesc);

    if (pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
    {
        // An SPJ can be declared to return say for example 3 Result Sets, but actually
        // return only 2 Result Sets. In this case, SQL_EXEC_AllocStmtForRS would not
        // return -8916 when we call it for the 3rd result set. However when we try
        // to describe it we would get a -8915. This should also be treated as a signal
        // that there are no more result sets. HANDLE_ERROR2 would return this value
        // to the previous stack frame where we will handle this condition
        HANDLE_ERROR2(retcode, sqlWarning);
    }
    else
    {
        HANDLE_ERROR(retcode, sqlWarning);
    }

    retcode = WSQL_EXEC_GetDescEntryCount(pInputDesc, &pSrvrStmt->paramCount);
    HANDLE_ERROR(retcode, sqlWarning);

    retcode = WSQL_EXEC_GetDescEntryCount(pOutputDesc, &pSrvrStmt->columnCount);
    HANDLE_ERROR(retcode, sqlWarning);

    // Child query visibility
    retcode = WSQL_EXEC_GetStmtAttr(pStmt, SQL_ATTR_SUBQUERY_TYPE, &pSrvrStmt->sqlSubQueryType, NULL, 0, NULL);

    retcode = WSQL_EXEC_GetStmtAttr(pStmt, SQL_ATTR_QUERY_TYPE, &pSrvrStmt->sqlQueryType, NULL, 0, NULL);
    HANDLE_ERROR(retcode, sqlWarning);

    if(pSrvrStmt->sqlStmtType == TYPE_CALL)
    {
        pSrvrStmt->prepareSetup();

        if (pSrvrStmt->paramCount > 0){
            kdsCreateSQLDescSeq(&pSrvrStmt->inputDescList, pSrvrStmt->paramCount+pSrvrStmt->inputDescParamOffset);
            retcode = BuildSQLDesc(pSrvrStmt, SRVR_STMT_HDL::Input);
            HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
        } else {
            kdsCreateEmptySQLDescSeq(&pSrvrStmt->inputDescList);
        }

        if (pSrvrStmt->columnCount > 0){
            kdsCreateSQLDescSeq(&pSrvrStmt->outputDescList, pSrvrStmt->columnCount);
            retcode = BuildSQLDesc(pSrvrStmt, SRVR_STMT_HDL::Output);
            HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
        } else {
            kdsCreateEmptySQLDescSeq(&pSrvrStmt->outputDescList);
        }
    }
    else
    {
        if (pSrvrStmt->paramCount > 0)
        {
            tmpBuildID = srvrGlobal->drvrVersion.buildId; // should go way once we support rowwise rowsets
            srvrGlobal->drvrVersion.buildId = 0;
            retcode = BuildSQLDesc2(pInputDesc, -9999, 0, pSrvrStmt->sqlBulkFetchPossible, pSrvrStmt->paramCount,
                    pSrvrStmt->inputDescBuffer, pSrvrStmt->inputDescBufferLength,
                    pSrvrStmt->inputDescVarBuffer, pSrvrStmt->inputDescVarBufferLen, pSrvrStmt->IPD,pSrvrStmt->SqlDescInfo);
            srvrGlobal->drvrVersion.buildId = tmpBuildID; // should go way once we support rowwise rowsets
            HANDLE_ERROR(retcode, sqlWarning);
        }

        Int32 estRowLength=0;
        if (pSrvrStmt->columnCount > 0)
        {
            retcode = BuildSQLDesc2(pOutputDesc, pSrvrStmt->sqlQueryType,  pSrvrStmt->maxRowsetSize, pSrvrStmt->sqlBulkFetchPossible,
                    pSrvrStmt->columnCount, pSrvrStmt->outputDescBuffer, pSrvrStmt->outputDescBufferLength,
                    pSrvrStmt->outputDescVarBuffer, pSrvrStmt->outputDescVarBufferLen, pSrvrStmt->IRD,pSrvrStmt->SqlDescInfo);
            HANDLE_ERROR(retcode, sqlWarning);

            if (srvrGlobal->drvrVersion.buildId & ROWWISE_ROWSET)
            {
                estRowLength = pSrvrStmt->outputDescVarBufferLen;
                pSrvrStmt->bFirstSqlBulkFetch = true;
            }
            else
            {
                int columnCount = pSrvrStmt->columnCount;
                Int32 estLength;
                SRVR_DESC_HDL *IRD = pSrvrStmt->IRD;

                for (int curColumnNo = 0; curColumnNo < columnCount ; curColumnNo++)
                {
                    IRD = pSrvrStmt->IRD;
                    estLength = getAllocLength(IRD[curColumnNo].dataType, IRD[curColumnNo].length);
                    estLength += 1;
                    estRowLength += estLength;
                }
            }
        }
        pSrvrStmt->estRowLength = estRowLength;
    }

    if (rgWarning)
        THREAD_RETURN(pSrvrStmt,ODBC_RG_WARNING);
    if (shapeWarning)
        THREAD_RETURN(pSrvrStmt,SQL_SHAPE_WARNING);
    if (sqlWarning)
        THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);

    THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}

SQLRETURN PREPARE_R(SRVR_STMT_HDL* pSrvrStmt, bool isFromExecDirect)
{
    FUNCTION_ENTRY("PREPARE",
            ("pSrvrStmt=0x%08x, isFromExecDirect=%d",
             pSrvrStmt, isFromExecDirect));

    CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

    Int32 retcode;
    UInt32 flags = 0;
    if(isFromExecDirect)
        flags = flags | PREPARE_STANDALONE_QUERY;

    char *pSqlStr = pSrvrStmt->sqlStringText;

    SQLSTMT_ID	*pStmt;
    SQLDESC_ID	*pInputDesc;
    SQLDESC_ID	*pOutputDesc;

    char        *pStmtName;
    BOOL		sqlWarning = FALSE;
    BOOL		rgWarning = FALSE;
    BOOL		shapeWarning = FALSE;
    Int32		tempmaxRowsetSize = 0;
    Int32 		holdableCursor = pSrvrStmt->holdableCursor;

    pSrvrStmt->PerfFetchRetcode = SQL_SUCCESS;
    pSrvrStmt->RowsetFetchRetcode = SQL_SUCCESS;

    pStmt = &pSrvrStmt->stmt;
    pOutputDesc = &pSrvrStmt->outputDesc;
    pInputDesc = &pSrvrStmt->inputDesc;

    SQLDESC_ID	sqlString_desc;
    sqlString_desc.version = SQLCLI_ODBC_VERSION;
    sqlString_desc.module = &pSrvrStmt->moduleId;
    sqlString_desc.name_mode = string_data;
    sqlString_desc.identifier     = (const char *) pSqlStr;
    sqlString_desc.handle = 0;
    sqlString_desc.identifier_len = pSrvrStmt->sqlStringLen;
    sqlString_desc.charset = SQLCHARSETSTRING_UTF8;
    UInt32 tmpBuildID = 0;


    if (srvrGlobal->drvrVersion.buildId & ROWWISE_ROWSET)
        retcode = WSQL_EXEC_SetDescItem(pOutputDesc, 0, SQLDESC_ROWSET_TYPE, 3, 0);
    else
        retcode = WSQL_EXEC_SetDescItem(pOutputDesc, 0, SQLDESC_ROWSET_TYPE, 0, 0);
    HANDLE_ERROR(retcode, sqlWarning);

    // We have to do the following CLI to set rowsize to ZERO, Since we switch between rowset to non-rowsets
    retcode = WSQL_EXEC_SetStmtAttr(pStmt,SQL_ATTR_INPUT_ARRAY_MAXSIZE,tempmaxRowsetSize, NULL);
    HANDLE_ERROR(retcode, sqlWarning);

    // need to set HOLDABLE cursor only when it differs from current holdable cursor
    if (holdableCursor != pSrvrStmt->current_holdableCursor) {
        retcode =  WSQL_EXEC_SetStmtAttr(pStmt,SQL_ATTR_CURSOR_HOLDABLE,holdableCursor, NULL);
        pSrvrStmt->current_holdableCursor = holdableCursor;
        HANDLE_ERROR(retcode, sqlWarning);
    }

    if (pSrvrStmt->sqlQueryType != SQL_SP_RESULT_SET)
    {
        // Result sets are already prepared.
        pSrvrStmt->sqlUniqueQueryIDLen = MAX_QUERY_NAME_LEN;
        pSrvrStmt->sqlUniqueQueryID[0] = '\0';
        retcode = WSQL_EXEC_Prepare2(pStmt, &sqlString_desc
                , NULL, NULL, NULL, &pSrvrStmt->cost_info
                , &pSrvrStmt->comp_stats_info, pSrvrStmt->sqlUniqueQueryID
                , &pSrvrStmt->sqlUniqueQueryIDLen
                , flags);
        pSrvrStmt->sqlUniqueQueryID[pSrvrStmt->sqlUniqueQueryIDLen] = '\0';
        pSrvrStmt->m_bNewQueryId = true;

        HANDLE_ERROR(retcode, sqlWarning);
    }

    retcode = WSQL_EXEC_DescribeStmt(pStmt, pInputDesc, pOutputDesc);

    if (pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
    {
        // An SPJ can be declared to return say for example 3 Result Sets, but actually
        // return only 2 Result Sets. In this case, SQL_EXEC_AllocStmtForRS would not
        // return -8916 when we call it for the 3rd result set. However when we try
        // to describe it we would get a -8915. This should also be treated as a signal
        // that there are no more result sets. HANDLE_ERROR2 would return this value
        // to the previous stack frame where we will handle this condition
        HANDLE_ERROR2(retcode, sqlWarning);
    }
    else
    {
        HANDLE_ERROR(retcode, sqlWarning);
    }

    retcode = WSQL_EXEC_GetDescEntryCount(pInputDesc, &pSrvrStmt->paramCount);
    HANDLE_ERROR(retcode, sqlWarning);

    retcode = WSQL_EXEC_GetDescEntryCount(pOutputDesc, &pSrvrStmt->columnCount);
    HANDLE_ERROR(retcode, sqlWarning);

    // Child query visibility
    retcode = WSQL_EXEC_GetStmtAttr(pStmt, SQL_ATTR_SUBQUERY_TYPE, &pSrvrStmt->sqlSubQueryType, NULL, 0, NULL);

    retcode = WSQL_EXEC_GetStmtAttr(pStmt, SQL_ATTR_QUERY_TYPE, &pSrvrStmt->sqlQueryType, NULL, 0, NULL);
    HANDLE_ERROR(retcode, sqlWarning);

    if(pSrvrStmt->sqlStmtType == TYPE_CALL)
    {
        pSrvrStmt->prepareSetup();

        if (pSrvrStmt->paramCount > 0){
            kdsCreateSQLDescSeq(&pSrvrStmt->inputDescList, pSrvrStmt->paramCount+pSrvrStmt->inputDescParamOffset);
            retcode = BuildSQLDesc(pSrvrStmt, SRVR_STMT_HDL::Input);
            HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
        } else {
            kdsCreateEmptySQLDescSeq(&pSrvrStmt->inputDescList);
        }

        if (pSrvrStmt->columnCount > 0){
            kdsCreateSQLDescSeq(&pSrvrStmt->outputDescList, pSrvrStmt->columnCount);
            retcode = BuildSQLDesc(pSrvrStmt, SRVR_STMT_HDL::Output);
            HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
        } else {
            kdsCreateEmptySQLDescSeq(&pSrvrStmt->outputDescList);
        }
    }
    else
    {
        if (pSrvrStmt->paramCount > 0)
        {
            tmpBuildID = srvrGlobal->drvrVersion.buildId; // should go way once we support rowwise rowsets
            srvrGlobal->drvrVersion.buildId = 0;
            retcode = BuildSQLDesc2(pInputDesc, -9999, 0, pSrvrStmt->sqlBulkFetchPossible, pSrvrStmt->paramCount,
                    pSrvrStmt->inputDescBuffer, pSrvrStmt->inputDescBufferLength,
                    pSrvrStmt->inputDescVarBuffer, pSrvrStmt->inputDescVarBufferLen, pSrvrStmt->IPD,pSrvrStmt->SqlDescInfo);
            srvrGlobal->drvrVersion.buildId = tmpBuildID; // should go way once we support rowwise rowsets
            HANDLE_ERROR(retcode, sqlWarning);
        }

        Int32 estRowLength=0;
        if (pSrvrStmt->columnCount > 0)
        {
            retcode = BuildSQLDesc2(pOutputDesc, pSrvrStmt->sqlQueryType,  pSrvrStmt->maxRowsetSize, pSrvrStmt->sqlBulkFetchPossible,
                    pSrvrStmt->columnCount, pSrvrStmt->outputDescBuffer, pSrvrStmt->outputDescBufferLength,
                    pSrvrStmt->outputDescVarBuffer, pSrvrStmt->outputDescVarBufferLen, pSrvrStmt->IRD,pSrvrStmt->SqlDescInfo);
            HANDLE_ERROR(retcode, sqlWarning);

            if (srvrGlobal->drvrVersion.buildId & ROWWISE_ROWSET)
            {
                estRowLength = pSrvrStmt->outputDescVarBufferLen;
                pSrvrStmt->bFirstSqlBulkFetch = true;
            }
            else
            {
                int columnCount = pSrvrStmt->columnCount;
                Int32 estLength;
                SRVR_DESC_HDL *IRD = pSrvrStmt->IRD;

                for (int curColumnNo = 0; curColumnNo < columnCount ; curColumnNo++)
                {
                    IRD = pSrvrStmt->IRD;
                    estLength = getAllocLength(IRD[curColumnNo].dataType, IRD[curColumnNo].length);
                    estLength += 1;
                    estRowLength += estLength;
                }
            }
        }
        pSrvrStmt->estRowLength = estRowLength;
    }

    if (rgWarning)
        THREAD_RETURN(pSrvrStmt,ODBC_RG_WARNING);
    if (shapeWarning)
        THREAD_RETURN(pSrvrStmt,SQL_SHAPE_WARNING);
    if (sqlWarning)
        THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);

    THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}

SQLRETURN BuildSQLDesc2(SQLDESC_ID *pDesc,
        Int32 sqlQueryType,
        Int32 maxRowsetSize,
        bool &sqlBulkFetchPossible,
        Int32 numEntries,
        BYTE *&SQLDesc,
        Int32 &SQLDescLen,
        BYTE *&varBuffer,
        Int32 &totalMemLen,
        SRVR_DESC_HDL *&implDesc,
        DESC_HDL_LISTSTMT *&SqlDescInfo)
{
    FUNCTION_ENTRY("BuildSQLDesc2", ("pDesc= 0x%08x, SQLDesc=0x%08x, SQLDescLen=%d, varBuffer= 0x%08x, \
                totalMemLen=%ld, implDesc=0x%08x, SqlDescInfo=0x%08x, numEntries=%d",
                pDesc, SQLDesc, SQLDescLen, varBuffer,
                totalMemLen, implDesc, SqlDescInfo, numEntries));

    Int32 FSDataType; // added this to get file system datatype
    // to find numeric signed or unsigned.
    Int32 VarAlign;
    Int32 IndAlign;
    Int32 Version = 0;
    Int32 DataType;
    Int32 DateTimeCode;
    Int32 Length;
    Int32 Precision;
    Int32 Scale;
    Int32 Nullable;
    BOOL SignType;
    Int32 ODBCDataType;
    Int32 ODBCPrecision;
    Int32 SQLCharset;
    Int32 ODBCCharset;
    Int32 ColHeadingNmlen;
    Int32 TableNamelen;
    Int32 CatalogNamelen;
    Int32 SchemaNamlen;
    Int32 Headinglen;
    Int32 IntLeadPrec;
    Int32 paramMode;

    Int32 totalDescLength = 0;
    Int32 i, j;
    Int32 tempLen = 0;
    Int32 retcode = SQL_SUCCESS;
    BOOL sqlWarning = FALSE;
    char Heading[MAX_ANSI_NAME_LEN];


    bool bRWRS = false;
    if (srvrGlobal->drvrVersion.buildId & ROWWISE_ROWSET)
        bRWRS = true;

    struct ODBCDescriptors
    {
        Int32 varAlign;
        Int32 indAlign;
        Int32 version;
        Int32 dataType;
        Int32 datetimeCode;
        Int32 maxLen;
        Int32 precision;
        Int32 scale;
        Int32 nullInfo;
        Int32 signType;
        Int32 ODBCDataType;
        Int32 ODBCPrecision;
        Int32 SQLCharset;
        Int32 ODBCCharset;
        Int32 FSDataType;
        Int32 colHeadingNmlen;
        char colHeadingNm[MAX_ANSI_NAME_LEN+1];
        Int32 TableNamelen;
        char TableName[MAX_ANSI_NAME_LEN+1];
        Int32 CatalogNamelen;
        char CatalogName[MAX_ANSI_NAME_LEN+1];
        Int32 SchemaNamlen;
        char SchemaName[MAX_ANSI_NAME_LEN+1];
        Int32 Headinglen;
        char Heading[MAX_ANSI_NAME_LEN+1];
        Int32 intLeadPrec;
        Int32 paramMode;
    };

    Int32			tempDescLen = 0;

    MEMORY_DELETE_ARRAY(implDesc);
    MEMORY_DELETE_ARRAY(SqlDescInfo);
    MEMORY_DELETE_ARRAY(SQLDesc);

    if (numEntries > 0)
    {

        MEMORY_ALLOC_ARRAY(implDesc, SRVR_DESC_HDL, numEntries);
        if (implDesc == NULL)
        {
            exit(1);
        }

        MEMORY_ALLOC_ARRAY(SqlDescInfo, DESC_HDL_LISTSTMT, numEntries);
        if (SqlDescInfo == NULL)
        {
            exit(1);
        }

        tempDescLen	= sizeof(totalMemLen) + sizeof(numEntries) + (sizeof(ODBCDescriptors) * numEntries);
        MEMORY_ALLOC_ARRAY(SQLDesc, BYTE, tempDescLen);
        if (SQLDesc == NULL)
        {
            exit(1);
        }

        *(Int32 *)(SQLDesc+totalDescLength) = 0; // Initialize totalMemLen, Since its calculated later
        totalDescLength += sizeof(totalMemLen);
        *(Int32 *)(SQLDesc+totalDescLength) = numEntries;
        totalDescLength += sizeof(numEntries);

    }

    for (i = 1, totalMemLen = 0; i <= numEntries; i++)
    {
        // Initialize the desc entry in SQLDESC_ITEM struct
        for (j = 0; j < NO_OF_DESC_ITEMS ; j++)
        {
            gDescItems[j].entry = i;
        }
        gDescItems[10].num_val_or_len = MAX_ANSI_NAME_LEN;
        gDescItems[11].num_val_or_len = MAX_ANSI_NAME_LEN;
        gDescItems[12].num_val_or_len = MAX_ANSI_NAME_LEN;
        gDescItems[13].num_val_or_len = MAX_ANSI_NAME_LEN;
        gDescItems[14].num_val_or_len = MAX_ANSI_NAME_LEN;

        retcode = WSQL_EXEC_GetDescItems2(pDesc,
                NO_OF_DESC_ITEMS,
                (SQLDESC_ITEM *)&gDescItems);
        HANDLE_ERROR(retcode, sqlWarning);

        Heading[0] = '\0';

        DataType = gDescItems[0].num_val_or_len;
        Length = gDescItems[1].num_val_or_len;
        Precision = gDescItems[2].num_val_or_len;
        Scale = gDescItems[3].num_val_or_len;
        Nullable = gDescItems[4].num_val_or_len;
        paramMode = gDescItems[5].num_val_or_len;
        IntLeadPrec = gDescItems[6].num_val_or_len;
        DateTimeCode = gDescItems[7].num_val_or_len;
        SQLCharset = gDescItems[8].num_val_or_len;
        FSDataType = gDescItems[9].num_val_or_len;
        CatalogNm[gDescItems[10].num_val_or_len] = '\0';
        SchemaNm[gDescItems[11].num_val_or_len] = '\0';
        TableNm[gDescItems[12].num_val_or_len] = '\0';
        ColumnName[gDescItems[13].num_val_or_len] = '\0';
        ColumnHeading[gDescItems[14].num_val_or_len] = '\0';

        // Added this check since SQL started returning SQLTYPECODE_NUMERIC.
        // Instead of changing on driver side Datatype is changed here to work correctly.
        if (DataType == SQLTYPECODE_NUMERIC || DataType == SQLTYPECODE_NUMERIC_UNSIGNED)
        {
            switch (FSDataType)
            {
                case 130:
                    DataType = SQLTYPECODE_SMALLINT;
                    break;
                case 131:
                    DataType = SQLTYPECODE_SMALLINT_UNSIGNED;
                    break;
                case 132:
                    DataType = SQLTYPECODE_INTEGER;
                    break;
                case 133:
                    DataType = SQLTYPECODE_INTEGER_UNSIGNED;
                    break;
                case 134:
                    DataType = SQLTYPECODE_LARGEINT;
                    break;
                default:
                    break;
            }
        }

        retcode = GetODBCValues(DataType, DateTimeCode, Length, Precision, ODBCDataType,
                ODBCPrecision, SignType, Nullable, totalMemLen,	SQLCharset, ODBCCharset,
                IntLeadPrec, ColumnHeading);
        HANDLE_ERROR(retcode, sqlWarning);
        if (DataType == SQLTYPECODE_LARGEINT && Precision == 0 && Scale > 0)
            ODBCDataType = SQL_NUMERIC;

        implDesc[i-1].charSet = SQLCharset;
        implDesc[i-1].dataType = DataType;
        implDesc[i-1].length = Length;
        implDesc[i-1].paramMode = paramMode;
        implDesc[i-1].precision = Precision;
        implDesc[i-1].scale = Scale;
        implDesc[i-1].sqlDatetimeCode = DateTimeCode;
        implDesc[i-1].FSDataType = FSDataType;

        SqlDescInfo[i-1].DataType = DataType;
        SqlDescInfo[i-1].Length = Length;
        SqlDescInfo[i-1].Nullable = Nullable;
        SqlDescInfo[i-1].VarBuf = totalDescLength;
        totalDescLength += sizeof(VarAlign);
        SqlDescInfo[i-1].IndBuf = totalDescLength;
        totalDescLength += sizeof(IndAlign);

        *(Int32 *)(SQLDesc+totalDescLength) = Version;
        totalDescLength += sizeof(Version);

        *(Int32 *)(SQLDesc+totalDescLength) = DataType;
        totalDescLength += sizeof(DataType);

        *(Int32 *)(SQLDesc+totalDescLength) = DateTimeCode;
        totalDescLength += sizeof(DateTimeCode);

        *(Int32 *)(SQLDesc+totalDescLength) = Length;
        totalDescLength += sizeof(Length);

        *(Int32 *)(SQLDesc+totalDescLength) = Precision;
        totalDescLength += sizeof(Precision);

        *(Int32 *)(SQLDesc+totalDescLength) = Scale;
        totalDescLength += sizeof(Scale);

        *(Int32 *)(SQLDesc+totalDescLength) = Nullable;
        totalDescLength += sizeof(Nullable);

        if (SignType)  // This may change if SQL returns sign and since desc will be Int32 return as Int32
            *(Int32 *)(SQLDesc+totalDescLength) = 1;
        else
            *(Int32 *)(SQLDesc+totalDescLength) = 0;
        totalDescLength += sizeof(Int32);

        *(Int32 *)(SQLDesc+totalDescLength) = ODBCDataType;
        totalDescLength += sizeof(ODBCDataType);

        *(Int32 *)(SQLDesc+totalDescLength) = ODBCPrecision;
        totalDescLength += sizeof(ODBCPrecision);

        *(Int32 *)(SQLDesc+totalDescLength) = SQLCharset;
        totalDescLength += sizeof(SQLCharset);

        *(Int32 *)(SQLDesc+totalDescLength) = ODBCCharset;
        totalDescLength += sizeof(ODBCCharset);

        *(Int32 *)(SQLDesc+totalDescLength) = FSDataType;
        totalDescLength += sizeof(FSDataType);

        ColHeadingNmlen = gDescItems[13].num_val_or_len+1; // Null terminator
        *(Int32 *)(SQLDesc+totalDescLength) = ColHeadingNmlen;
        totalDescLength += sizeof(ColHeadingNmlen);
        if (ColHeadingNmlen > 0)
        {
            memcpy(SQLDesc+totalDescLength,ColumnName,ColHeadingNmlen);
            totalDescLength += ColHeadingNmlen;
        }

        TableNamelen = gDescItems[12].num_val_or_len+1;
        *(Int32 *)(SQLDesc+totalDescLength) = TableNamelen;
        totalDescLength += sizeof(TableNamelen);
        if (TableNamelen > 0)
        {
            memcpy(SQLDesc+totalDescLength,TableNm,TableNamelen);
            totalDescLength += TableNamelen;
        }

        CatalogNamelen = gDescItems[10].num_val_or_len+1;
        *(Int32 *)(SQLDesc+totalDescLength) = CatalogNamelen;
        totalDescLength += sizeof(CatalogNamelen);
        if (CatalogNamelen > 0)
        {
            memcpy(SQLDesc+totalDescLength,CatalogNm,CatalogNamelen);
            totalDescLength += CatalogNamelen;
        }

        SchemaNamlen = gDescItems[11].num_val_or_len+1;
        *(Int32 *)(SQLDesc+totalDescLength) = SchemaNamlen;
        totalDescLength += sizeof(SchemaNamlen);
        if (SchemaNamlen > 0)
        {
            memcpy(SQLDesc+totalDescLength,SchemaNm,SchemaNamlen);
            totalDescLength += SchemaNamlen;
        }

        Headinglen = 1;
        *(Int32 *)(SQLDesc+totalDescLength) = Headinglen;
        totalDescLength += sizeof(Headinglen);
        if (Headinglen > 0)
        {
            memcpy(SQLDesc+totalDescLength,Heading,Headinglen);
            totalDescLength += Headinglen;
        }

        *(Int32 *)(SQLDesc+totalDescLength) = IntLeadPrec;
        totalDescLength += sizeof(IntLeadPrec);

        *(Int32 *)(SQLDesc+totalDescLength) = paramMode;
        totalDescLength += sizeof(paramMode);
    }

    if (!bRWRS)
    {
        // Adjust totalMemLen to word boundary
        totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3;
    }

    MEMORY_DELETE_ARRAY(varBuffer);
    
    //	if (sqlBulkFetchPossible && (sqlQueryType == SQL_SELECT_NON_UNIQUE || sqlQueryType == SQL_SP_RESULT_SET))
    if (bRWRS)
    {
        MEMORY_ALLOC_ARRAY(varBuffer, BYTE, srvrGlobal->m_FetchBufferSize);
    }
    else
    {
        MEMORY_ALLOC_ARRAY(varBuffer, BYTE, totalMemLen);
    }

    //setting the Indicator and Variable pointer
    retcode =  SetIndandVarPtr(pDesc,bRWRS,numEntries,SQLDesc,varBuffer,totalMemLen,
            implDesc,SqlDescInfo);

    SQLDescLen = totalDescLength;

    if (bRWRS)
    {
        Int32 TmpMaxRows = (srvrGlobal->m_FetchBufferSize)/totalMemLen;

        retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWSET_TYPE, 3, 0);

        HANDLE_ERROR(retcode, sqlWarning);

        retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWWISE_ROWSET_PTR, (long)varBuffer, 0);
        HANDLE_ERROR(retcode, sqlWarning);

        retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWWISE_ROWSET_SIZE, TmpMaxRows, 0);
        HANDLE_ERROR(retcode, sqlWarning);

        retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWWISE_ROWSET_ROW_LEN, totalMemLen, 0);
        HANDLE_ERROR(retcode, sqlWarning);
    }

    if (sqlWarning)
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
    else
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
} // BuildSQLDesc2

SQLRETURN SetIndandVarPtr(SQLDESC_ID *pDesc,
        bool &bRWRS,
        Int32 numEntries,
        BYTE *&SQLDesc,
        BYTE *&varBuffer,
        Int32 &totalMemLen,
        SRVR_DESC_HDL *&implDesc,
        DESC_HDL_LISTSTMT *&SqlDescInfo)
{
    FUNCTION_ENTRY("BuildSQLDesc2", ("pDesc= 0x%08x, SQLDesc=0x%08x, varBuffer=0x%08x, \
                totalMemLen=%d, implDesc=0x%08x, SqlDescInfo=0x%08x, numEntries=%d",
                pDesc, SQLDesc, varBuffer,
                totalMemLen, implDesc, SqlDescInfo, numEntries));

    BYTE *IndPtr;
    BYTE *memPtr;
    BYTE *VarPtr;

    Int32 retcode = SQL_SUCCESS;
    BOOL sqlWarning = FALSE;


    Int32 memOffSet = 0;
    int i;

    memPtr = varBuffer ;
    memOffSet = 0;

    *(Int32 *)SQLDesc = totalMemLen;

    for (i = 0 ; i < numEntries ; i++)
    {
        if (bRWRS)
        {
            if (SqlDescInfo[i].Nullable)
            {
                memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
                IndPtr = memPtr + memOffSet;
                *(Int32 *)(SQLDesc+SqlDescInfo[i].IndBuf) = (Int32)(IndPtr-memPtr);
                memOffSet += 2 ;
            }
            else
            {
                IndPtr = NULL;
                *(Int32 *)(SQLDesc+SqlDescInfo[i].IndBuf) = (Int32)(-1);
            }
        }
        switch (SqlDescInfo[i].DataType)
        {
            case SQLTYPECODE_CHAR:
            case SQLTYPECODE_VARCHAR:
                VarPtr = memPtr + memOffSet;
                memOffSet += SqlDescInfo[i].Length;
                if (!bRWRS)
                    memOffSet += 1;
                break;
            case SQLTYPECODE_VARCHAR_WITH_LENGTH:
                if(SqlDescInfo[i].Length > 0x7FFF)
                {
                    memOffSet = ((memOffSet + 4 - 1) >> 2) << 2;
                    VarPtr = memPtr + memOffSet;
                    memOffSet += SqlDescInfo[i].Length + 4;
                }
                else
                {
                    memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
                    VarPtr = memPtr + memOffSet;
                    memOffSet += SqlDescInfo[i].Length + 2;
                }
                if (!bRWRS)
                    memOffSet += 1;
                break;
            case SQLTYPECODE_VARCHAR_LONG:
                memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
                VarPtr = memPtr + memOffSet;
                memOffSet += SqlDescInfo[i].Length + 2;
                if (!bRWRS)
                    memOffSet += 1;
                break;
            case SQLTYPECODE_SMALLINT:
            case SQLTYPECODE_SMALLINT_UNSIGNED:
                memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
                VarPtr = memPtr + memOffSet;
                memOffSet += SqlDescInfo[i].Length;
                break;
            case SQLTYPECODE_INTEGER:
            case SQLTYPECODE_INTEGER_UNSIGNED:
                //case SQLTYPECODE_IEEE_REAL:
                memOffSet = ((memOffSet + 4 - 1) >> 2) << 2;
                VarPtr = memPtr + memOffSet;
                memOffSet += SqlDescInfo[i].Length;
                break;
            case SQLTYPECODE_LARGEINT:
            case SQLTYPECODE_IEEE_REAL:
            case SQLTYPECODE_IEEE_FLOAT:
            case SQLTYPECODE_IEEE_DOUBLE:
                memOffSet = ((memOffSet + 8 - 1) >> 3) << 3;
                VarPtr = memPtr + memOffSet;
                memOffSet += SqlDescInfo[i].Length;
                break;
            case SQLTYPECODE_DECIMAL_UNSIGNED:
            case SQLTYPECODE_DECIMAL:
            case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
            case SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
            case SQLTYPECODE_INTERVAL:		// Treating as CHAR
            case SQLTYPECODE_DATETIME:
                VarPtr = memPtr + memOffSet;
                memOffSet += SqlDescInfo[i].Length;
                break;
            default:
                memOffSet = ((memOffSet + 8 - 1) >> 3) << 3;
                VarPtr = memPtr + memOffSet;
                memOffSet += SqlDescInfo[i].Length;
                break;
        }
        *(Int32 *)(SQLDesc+SqlDescInfo[i].VarBuf) = (Int32)(VarPtr-memPtr);

        if (!bRWRS)
        {
            if (SqlDescInfo[i].Nullable)
            {
                memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
                IndPtr = memPtr + memOffSet;
                *(Int32 *)(SQLDesc+SqlDescInfo[i].IndBuf) = (Int32)(IndPtr-memPtr);
                memOffSet += 2 ;
            }
            else
            {
                IndPtr = NULL;
                *(Int32 *)(SQLDesc+SqlDescInfo[i].IndBuf) = (Int32)(-1);
            }
        }

        retcode = WSQL_EXEC_SetDescItem(pDesc,
                i+1,
                SQLDESC_VAR_PTR,
                (long)VarPtr,
                0);
        HANDLE_ERROR(retcode, sqlWarning);

        retcode = WSQL_EXEC_SetDescItem(pDesc,
                i+1,
                SQLDESC_IND_PTR,
                (long)IndPtr,
                0);
        HANDLE_ERROR(retcode, sqlWarning);

        implDesc[i].varPtr = VarPtr;
        implDesc[i].indPtr = IndPtr;

        if (memOffSet > totalMemLen)
        {
            CLI_DEBUG_RETURN_SQL(PROGRAM_ERROR);
        }
    }

    CLI_DEBUG_RETURN_SQL(retcode);
} // End of SetIndandVarPtr

SQLRETURN GetODBCValues(Int32 DataType, Int32 DateTimeCode, Int32 &Length, Int32 Precision,
        Int32 &ODBCDataType, Int32 &ODBCPrecision, BOOL &SignType,
        Int32 Nullable, Int32 &totalMemLen, Int32 SQLCharset, Int32 &ODBCCharset,
        Int32 IntLeadPrec, char *ColHeading)
{
    FUNCTION_ENTRY("GetODBCValues", ("DataType=%d, DateTimeCode=%d, Precision=%d, \
                Nullable=%d, SQLCharset=%d, IntLeadPrec=%d",
                DataType, DateTimeCode, Precision,
                Nullable, SQLCharset, IntLeadPrec));

    bool bRWRS = false;
    if (srvrGlobal->drvrVersion.buildId & ROWWISE_ROWSET)
        bRWRS = true;

    ODBCCharset = SQLCHARSETCODE_ISO88591;

    if (bRWRS)
    {
        if (Nullable)
        {
            totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1;
            totalMemLen += 2 ;
        }
    }

    switch (DataType)
    {
        case SQLTYPECODE_CHAR:
            ODBCPrecision = Length;
            ODBCDataType = SQL_CHAR;
            SignType = FALSE;
            totalMemLen += Length;
            if (!bRWRS)
                totalMemLen += 1;
            ODBCCharset = SQLCharset;
            break;
        case SQLTYPECODE_VARCHAR:
            ODBCPrecision = Length;
            ODBCDataType = SQL_VARCHAR;
            if(srvrGlobal->enableLongVarchar)
            {
                if (Length >= 255)	// we have to fix the data type to comply with ODBC def since SQL does not return this type
                    ODBCDataType = SQL_LONGVARCHAR;
            }
            SignType = FALSE;
            totalMemLen += Length;
            if (!bRWRS)
                totalMemLen += 1;
            ODBCCharset = SQLCharset;
            break;
        case SQLTYPECODE_VARCHAR_WITH_LENGTH:
            ODBCPrecision = Length;
            ODBCDataType = SQL_VARCHAR;
            if(srvrGlobal->enableLongVarchar)
            {
                if (Length >= 255)	// we have to fix the data type to comply with ODBC def since SQL does not return this type
                    ODBCDataType = SQL_LONGVARCHAR;
            }
            SignType = FALSE;
            if (Length > 0x7FFF)
            {
                totalMemLen = ((totalMemLen + 4 - 1) >> 2) << 2;
                totalMemLen += Length + 4;
            }
            else
            {
                totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1;
                totalMemLen += Length + 2;
            }

            if (!bRWRS)
                totalMemLen += 1;
            ODBCCharset = SQLCharset;
            break;
        case SQLTYPECODE_VARCHAR_LONG:
            ODBCPrecision = Length;
            ODBCDataType = SQL_LONGVARCHAR;
            SignType = FALSE;
            totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1;
            totalMemLen += Length + 2;
            if (!bRWRS)
                totalMemLen += 1;
            ODBCCharset = SQLCharset;
            break;
        case SQLTYPECODE_SMALLINT:
            if (Precision == 0)
            {
                ODBCPrecision = 5;
                ODBCDataType = SQL_SMALLINT;
            }
            else
            {
                ODBCPrecision = Precision;
                ODBCDataType = SQL_NUMERIC;
            }
            SignType = TRUE;
            totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1;
            totalMemLen += Length ;
            break;
        case SQLTYPECODE_SMALLINT_UNSIGNED:
            if (Precision == 0)
            {
                ODBCPrecision = 5;
                ODBCDataType = SQL_SMALLINT;
                SignType = FALSE;
            }
            else
            {
                ODBCPrecision = Precision;
                ODBCDataType = SQL_NUMERIC;
                SignType = FALSE;
            }
            totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1;
            totalMemLen += Length ;
            break;
        case SQLTYPECODE_INTEGER:
            if (Precision == 0)
            {
                ODBCPrecision = 10;
                ODBCDataType = SQL_INTEGER;
            }
            else
            {
                ODBCPrecision = Precision;
                ODBCDataType = SQL_NUMERIC;
            }
            SignType = TRUE;
            totalMemLen = ((totalMemLen + 4 - 1) >> 2) << 2;
            totalMemLen += Length ;
            break;
        case SQLTYPECODE_INTEGER_UNSIGNED:
            if (Precision == 0)
            {
                ODBCPrecision = 10;
                ODBCDataType = SQL_INTEGER;
                SignType = FALSE;
            }
            else
            {
                ODBCPrecision = Precision;
                ODBCDataType = SQL_NUMERIC;
                SignType = FALSE;
            }
            totalMemLen = ((totalMemLen + 4 - 1) >> 2) << 2;
            totalMemLen += Length ;
            break;
        case SQLTYPECODE_LARGEINT:
            if (Precision == 0)
            {
                ODBCPrecision = 19;
                ODBCDataType = SQL_BIGINT;
            }
            else
            {
                ODBCPrecision = Precision;
                ODBCDataType = SQL_NUMERIC;
            }
            if (ColHeading[0] != '\0')
            {
                if (strstr(ColHeading, CLOB_HEADING) != NULL)
                    ODBCDataType = TYPE_CLOB;
                else if (strstr(ColHeading, BLOB_HEADING) != NULL)
                    ODBCDataType = TYPE_BLOB;
            }
            SignType = TRUE;
            totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3;
            totalMemLen += Length ;
            break;
        case SQLTYPECODE_IEEE_REAL:
            ODBCDataType = SQL_REAL;
            ODBCPrecision = 7;
            SignType = TRUE;
            if (bRWRS)
                totalMemLen = ((totalMemLen + 4 - 1) >> 2) << 2;
            else
                totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3; // what condition will this work?
            totalMemLen += Length ;
            break;
        case SQLTYPECODE_IEEE_FLOAT:
            ODBCDataType = SQL_FLOAT;
            ODBCPrecision = 15;
            SignType = TRUE;
            totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3;
            totalMemLen += Length ;
            break;
        case SQLTYPECODE_IEEE_DOUBLE:
            ODBCDataType = SQL_DOUBLE;
            ODBCPrecision = 15;
            SignType = TRUE;
            totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3;
            totalMemLen += Length ;
            break;
        case SQLTYPECODE_DATETIME:
            switch (DateTimeCode)
            {
                case SQLDTCODE_DATE:					//1
                    ODBCDataType = SQL_DATE;
                    ODBCPrecision = 10;
                    break;
                case SQLDTCODE_TIME:					//2
                    if (!bRWRS) //for RWRS
                    {
                        ODBCDataType = SQL_TIME;
                        if (Precision == 0)
                            ODBCPrecision = 8;
                        else
                        {
                            ODBCDataType = SQL_TIMESTAMP;
                            ODBCPrecision = 20+Precision;

                        }
                    }
                    else
                    {
                        ODBCDataType = SQL_TIME;
                        ODBCPrecision = Precision;
                    }
                    break;
                case SQLDTCODE_TIMESTAMP:				//3
                    ODBCDataType = SQL_TIMESTAMP;
                    if (Precision == 0)
                        ODBCPrecision = 19;
                    else
                        ODBCPrecision = 20+Precision;
                    break;
                    //
                    // Mapping Non-standard SQL DATETIME types to DATE/TIME/TIMESTAMP
                    //
                case SQLDTCODE_YEAR:					//4
                    ODBCDataType = SQL_DATE;
                    ODBCPrecision = 10;
                    break;
                case SQLDTCODE_YEAR_TO_MONTH:			//5
                    ODBCDataType = SQL_DATE;
                    ODBCPrecision = 10;
                    break;
                case SQLDTCODE_YEAR_TO_HOUR:			//7
                    ODBCDataType = SQL_TIMESTAMP;
                    ODBCPrecision = 19;
                    break;
                case SQLDTCODE_YEAR_TO_MINUTE:			//8
                    ODBCDataType = SQL_TIMESTAMP;
                    ODBCPrecision = 19;
                    break;
                case SQLDTCODE_MONTH:					//10
                    ODBCDataType = SQL_DATE;
                    ODBCPrecision = 10;
                    break;
                case SQLDTCODE_MONTH_TO_DAY:			//11
                    ODBCDataType = SQL_DATE;
                    ODBCPrecision = 10;
                    break;
                case SQLDTCODE_MONTH_TO_HOUR:			//12
                    ODBCDataType = SQL_TIMESTAMP;
                    ODBCPrecision = 19;
                    break;
                case SQLDTCODE_MONTH_TO_MINUTE:			//13
                    ODBCDataType = SQL_TIMESTAMP;
                    ODBCPrecision = 19;
                    break;
                case SQLDTCODE_MONTH_TO_FRACTION:		//14
                    ODBCDataType = SQL_TIMESTAMP;
                    if (Precision == 0)
                        ODBCPrecision = 19;
                    else
                        ODBCPrecision = 20+Precision;
                    break;
                case SQLDTCODE_DAY:						//15
                    ODBCDataType = SQL_DATE;
                    ODBCPrecision = 10;
                    break;
                case SQLDTCODE_DAY_TO_HOUR:				//16
                    ODBCDataType = SQL_TIMESTAMP;
                    ODBCPrecision = 19;
                    break;
                case SQLDTCODE_DAY_TO_MINUTE:			//17
                    ODBCDataType = SQL_TIMESTAMP;
                    ODBCPrecision = 19;
                    break;
                case SQLDTCODE_DAY_TO_FRACTION:			//18
                    ODBCDataType = SQL_TIMESTAMP;
                    if (Precision == 0)
                        ODBCPrecision = 19;
                    else
                        ODBCPrecision = 20+Precision;
                    break;
                case SQLDTCODE_HOUR:					//19
                    ODBCDataType = SQL_TIME;
                    ODBCPrecision = 8;
                    break;
                case SQLDTCODE_HOUR_TO_MINUTE:			//20
                    ODBCDataType = SQL_TIME;
                    ODBCPrecision = 8;
                    break;
                case SQLDTCODE_MINUTE:					//22
                    ODBCDataType = SQL_TIME;
                    ODBCPrecision = 8;
                    break;
                case SQLDTCODE_MINUTE_TO_FRACTION:		//23
                    ODBCDataType = SQL_TIME;
                    if (Precision == 0)
                        ODBCPrecision = 8;
                    else
                    {
                        ODBCDataType = SQL_TIMESTAMP;
                        ODBCPrecision = 20+Precision;
                    }
                    break;
                case SQLDTCODE_SECOND_TO_FRACTION:		//24
                    ODBCDataType = SQL_TIME;
                    if (Precision == 0)
                        ODBCPrecision = 8;
                    else
                    {
                        ODBCDataType = SQL_TIMESTAMP;
                        ODBCPrecision = 20+Precision;
                    }
                    break;
                default:
                    ODBCDataType = SQL_TYPE_NULL;
                    ODBCPrecision = 0;
                    break;
            }
            SignType = FALSE;
            if (!bRWRS)
                totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3;
            totalMemLen += Length ;
            break;
        case SQLTYPECODE_DECIMAL_UNSIGNED:
            ODBCPrecision = Length;
            ODBCDataType = SQL_DECIMAL;
            SignType = FALSE;
            totalMemLen += Length;
            break;
        case SQLTYPECODE_DECIMAL:
            ODBCPrecision = Length;
            ODBCDataType = SQL_DECIMAL;
            SignType = TRUE;
            totalMemLen += Length;
            break;
        case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // extension
            ODBCDataType = SQL_DOUBLE; // Since there is no corresponding ODBC DataType, Map it as a double
            ODBCPrecision = 15;
            SignType = FALSE;
            totalMemLen += Length;
            break;
        case SQLTYPECODE_DECIMAL_LARGE: // extension
            ODBCDataType = SQL_DOUBLE; // Since there is no corresponding ODBC DataType, Map it as a double
            ODBCPrecision = 15;
            SignType = TRUE;
            totalMemLen += Length;
            break;
        case SQLTYPECODE_INTERVAL:		// Interval will be sent in ANSIVARCHAR format
            switch (DateTimeCode)
            {
                case SQLINTCODE_YEAR:
                    ODBCDataType = SQL_INTERVAL_YEAR;
                    ODBCPrecision = 0;
                    break;
                case SQLINTCODE_MONTH:
                    ODBCDataType = SQL_INTERVAL_MONTH;
                    ODBCPrecision = 0;
                    break;
                case SQLINTCODE_DAY:
                    ODBCDataType = SQL_INTERVAL_DAY;
                    ODBCPrecision = 0;
                    break;
                case SQLINTCODE_HOUR:
                    ODBCDataType = SQL_INTERVAL_HOUR;
                    ODBCPrecision = 0;
                    break;
                case SQLINTCODE_MINUTE:
                    ODBCDataType = SQL_INTERVAL_MINUTE;
                    ODBCPrecision = 0;
                    break;
                case SQLINTCODE_SECOND:
                    ODBCDataType = SQL_INTERVAL_SECOND;
                    ODBCPrecision = Precision;
                    break;
                case SQLINTCODE_YEAR_MONTH:
                    ODBCDataType = SQL_INTERVAL_YEAR_TO_MONTH;
                    ODBCPrecision = 0;
                    break;
                case SQLINTCODE_DAY_HOUR:
                    ODBCDataType = SQL_INTERVAL_DAY_TO_HOUR;
                    ODBCPrecision = 0;
                    break;
                case SQLINTCODE_DAY_MINUTE:
                    ODBCDataType = SQL_INTERVAL_DAY_TO_MINUTE;
                    ODBCPrecision = 0;
                    break;
                case SQLINTCODE_DAY_SECOND:
                    ODBCDataType = SQL_INTERVAL_DAY_TO_SECOND;
                    ODBCPrecision = Precision;
                    break;
                case SQLINTCODE_HOUR_MINUTE:
                    ODBCDataType = SQL_INTERVAL_HOUR_TO_MINUTE;
                    ODBCPrecision = 0;
                    break;
                case SQLINTCODE_HOUR_SECOND:
                    ODBCDataType = SQL_INTERVAL_HOUR_TO_SECOND;
                    ODBCPrecision = Precision;
                    break;
                case SQLINTCODE_MINUTE_SECOND:
                    ODBCDataType = SQL_INTERVAL_MINUTE_TO_SECOND;
                    ODBCPrecision = Precision;
                    break;
                default:
                    ODBCDataType = SQL_TYPE_NULL;
                    ODBCPrecision = 0;
                    break;
            }
            SignType = FALSE;
            // Calculate the length based on Precision and IntLeadPrec
            // The max. length is for Day to Fraction(6)
            // Sign = 1
            // Day = IntLeadPrec + 1 ( 1 for Blank space)
            // Hour = 3 ( 2+1)
            // Minute = 3 (2+1)
            // Seconds = 3 (2+1)
            // Fraction = Precision
            totalMemLen += Length;
            break;
        case SQLTYPECODE_NUMERIC:              //2
            ODBCPrecision = Precision;
            ODBCDataType = SQL_NUMERIC;
            SignType = TRUE;
            totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3;
            totalMemLen += Length ;
            break;
        case SQLTYPECODE_NUMERIC_UNSIGNED:    //-201
            ODBCPrecision = Precision;
            ODBCDataType = SQL_NUMERIC;
            SignType = FALSE;
            totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3;
            totalMemLen += Length ;
            break;
        default:
            ODBCDataType = SQL_TYPE_NULL;
            ODBCPrecision = 0;
            SignType = FALSE;
            totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3;
            totalMemLen += Length ;
            break;
    }

    if (!bRWRS)
    {
        if (Nullable)
        {
            totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1;
            totalMemLen += 2 ;
        }
    }

    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN REALLOCSQLMXHDLS(SRVR_STMT_HDL* pSrvrStmt)
{
    Int32 retcode = SQL_SUCCESS;
    SQLSTMT_ID	*pStmt = &pSrvrStmt->stmt;
    SQLMODULE_ID *pModule = &pSrvrStmt->moduleId;
    BOOL		sqlWarning;

    if (! pSrvrStmt->isClosed)
    {
        retcode = WSQL_EXEC_CloseStmt(pStmt);
        pSrvrStmt->isClosed = TRUE;
    }
    if (pSrvrStmt->moduleName[0] == '\0')
    {
        retcode = WSQL_EXEC_DeallocStmt(pStmt);
    }

    pStmt->version = SQLCLI_ODBC_VERSION;
    pStmt->module = pModule;
    pStmt->handle = 0;
    pStmt->charset = SQLCHARSETSTRING_UTF8;
    if (pSrvrStmt->stmtName[0] != '\0')
    {
        pStmt->name_mode = stmt_name;
        pStmt->identifier_len = pSrvrStmt->stmtNameLen;
        pStmt->identifier = pSrvrStmt->stmtName;
    }
    else
    {
        pStmt->name_mode = stmt_handle;
        pStmt->identifier_len = 0;
        pStmt->identifier = NULL;
    }

    if (pModule->module_name == NULL)
    {
        if (pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
        {
            retcode = WSQL_EXEC_AllocStmtForRS(pSrvrStmt->callStmtId, pSrvrStmt->resultSetIndex, pStmt);
            HANDLE_ERROR2(retcode, sqlWarning);
        }
        else
        {
            retcode = WSQL_EXEC_AllocStmt(pStmt,(SQLSTMT_ID *)NULL);
            HANDLE_ERROR(retcode, sqlWarning);
        }
    }

    CLI_DEBUG_RETURN_SQL(retcode);
}

// Rowset execute
SQLRETURN EXECUTE2withRowsets(SRVR_STMT_HDL* pSrvrStmt)
{
    Int32                 inputRowCnt    =  pSrvrStmt->inputRowCnt;
    IDL_short                sqlStmtType    =  pSrvrStmt->sqlStmtType;
    const SQL_DataValue_def *inputDataValue = &pSrvrStmt->inputDataValue;
    Int64  rowsAffected_cli;
    Int32                *rowsAffected  = &pSrvrStmt->rowsAffected;
    Int32				*rowsAffectedHigherBytes = &pSrvrStmt->rowsAffectedHigherBytes;

    Int32		maxRowsetSize;
    Int32		retcode        = SQL_SUCCESS;
    Int32		execretcode	= SQL_SUCCESS;

    Int32		raretcode	= SQL_SUCCESS;

    BYTE		          *indBuffPtr  = NULL;
    BYTE		          *dataBuffPtr = NULL;
    short	 	          *typeBuffPtr;
    struct SQLCLI_QUAD_FIELDS *quadBuffPtr;

    short		          *TypePtr;
    struct SQLCLI_QUAD_FIELDS *QuadPtr;

    short		          *curTypePtr;
    struct SQLCLI_QUAD_FIELDS *curQuadPtr;

    SQLDESC_ID     *pDesc;
    SQLSTMT_ID     *pStmt;
    Int32		paramCount;
    char	       *cursorName;
    Int32		curRowInRowset;
    Int32		curParamNo;
    Int32		curLength;
    Int32		varPtr;
    Int32		indPtr;
    Int32		DataType;
    Int32		DataLen;
    Int32		offset_var_layout,offset_ind_layout;
    Int32		len,datalen,indlen;
    Int32		sqlMaxLength;

    short		SQLDataInd     = 0;
    short		SQLDataLength  = 0;
    void*		SQLDataValue;

    SQLSTMT_ID	cursorId;
    BOOL		sqlWarning     = FALSE;
    Int32		lendata;
    Int32            lenind;

    pSrvrStmt->PerfFetchRetcode = SQL_SUCCESS;
    pSrvrStmt->RowsetFetchRetcode = SQL_SUCCESS;

    if (&pSrvrStmt->stmt == NULL)
        CLI_DEBUG_RETURN_SQL(SQL_INVALID_HANDLE);

    maxRowsetSize = pSrvrStmt->maxRowsetSize;

    // Initialize this to TRUE later in this function depending on sqlStmtType if SQL_SELECT
    // then and retcode is SUCCESS or SUCCESS_WITH_INFO set it to FALSE. Since SQL opens CURSOR
    // after execute in case of SUCCESS or SUCCESS_WITH_INFO and closes only when it reaches
    // NO_DATA_FOUND or CLOSE STATEMENT or COMMIT WORK or ROLLBACK WORK.
    pSrvrStmt->isClosed = TRUE;

    pStmt = &pSrvrStmt->stmt;
    if (pSrvrStmt->stmtType == EXTERNAL_STMT && sqlStmtType == TYPE_SELECT &&
            pSrvrStmt->moduleId.module_name == NULL)
    {
        cursorName = pSrvrStmt->cursorName;
        // If cursor name is not specified, use the stmt name as cursor name
        if (*cursorName == '\0')
            cursorName = pSrvrStmt->stmtName;
        // If cursorName has chg'd from last EXEC or EXECDIRECT cmd
        // or has not yet been set for the first time call SetCursorName
        if ((strcmp(pSrvrStmt->previousCursorName, cursorName) != 0) && *cursorName != '\0')
        {
            cursorId.version        = SQLCLI_ODBC_VERSION;
            cursorId.module         = pStmt->module;
            cursorId.handle         = 0;
            cursorId.charset        = SQLCHARSETSTRING_UTF8;
            cursorId.name_mode      = cursor_name;
            if (*cursorName == '\0')
                cursorId.identifier_len = pSrvrStmt->stmtNameLen;
            else
                cursorId.identifier_len = pSrvrStmt->cursorNameLen;
            cursorId.identifier     = cursorName;
            strcpy(pSrvrStmt->previousCursorName, cursorName);	// keep track of last cursorName
            retcode = WSQL_EXEC_SetCursorName(pStmt, &cursorId);
            HANDLE_ERROR(retcode, sqlWarning);
        }

    }
    pDesc = &pSrvrStmt->inputDesc;
    paramCount = pSrvrStmt->paramCount;

    if (paramCount > 0)
    {
        BYTE *base = pSrvrStmt->IPD[0].varPtr;

        pSrvrStmt->inputQuadList[0].var_layout = 0;
        pSrvrStmt->inputQuadList[0].var_ptr    = &inputRowCnt; // KAS this is a bug. Need to use type returned in prepare.
        pSrvrStmt->inputQuadList[0].ind_layout = 0;
        pSrvrStmt->inputQuadList[0].ind_ptr    = NULL;

        for (int i = 1; i < paramCount; i++)
        {
            pSrvrStmt->inputQuadList[i].var_ptr = pSrvrStmt->transportBuffer + ((pSrvrStmt->IPD[i-1].varPtr - base) *  inputRowCnt);
            if (pSrvrStmt->IPD[i-1].indPtr != 0)
                pSrvrStmt->inputQuadList[i].ind_ptr = pSrvrStmt->transportBuffer + ((pSrvrStmt->IPD[i-1].indPtr - base) *  inputRowCnt);
            else
            {
                pSrvrStmt->inputQuadList[i].ind_layout = 0;
                pSrvrStmt->inputQuadList[i].ind_ptr    = NULL;
            }
        }  // end for

        SQLCLI_QUAD_FIELDS *quadList = pSrvrStmt->inputQuadList;

        retcode = WSQL_EXEC_SETROWSETDESCPOINTERS( pDesc,
                maxRowsetSize,
                NULL,
                1,
                paramCount,
                quadList);
        if (retcode < SQL_SUCCESS)
            goto ret;
    }  // end if (paramCount > 0)

    retcode = execretcode = WSQL_EXEC_ExecFetch(pStmt, pDesc, 0);

    pSrvrStmt->numErrRows = 0;

    rowsAffected_cli=0;
    *rowsAffected = 0;
    *rowsAffectedHigherBytes = 0;
    if (retcode == 30022 || retcode == 30035) // some rows are inserted
    {
        raretcode = WSQL_EXEC_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &rowsAffected_cli, NULL, 0, NULL);
        if (raretcode < SQL_SUCCESS)
        {
            *rowsAffected = -1;
            *rowsAffectedHigherBytes = -1;
        }
        else if (raretcode == 8411)
        {
            if (GetRowsAffected(pSrvrStmt) < SQL_SUCCESS)
            {
                *rowsAffected = -1;
                *rowsAffectedHigherBytes = -1;
            }
        }
        else
        {
            *rowsAffected=(Int32)(rowsAffected_cli << 32 >> 32);
            *rowsAffectedHigherBytes=(Int32)(rowsAffected_cli>>32);
        }

        // get information about inserted rows
        GETNOTATOMICROWSET2(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlWarning, pSrvrStmt);
    }
    else if (retcode < SQL_SUCCESS)
    {
        if ((   sqlStmtType == TYPE_INSERT_PARAM
                    || sqlStmtType == TYPE_UPDATE
                    || sqlStmtType == TYPE_DELETE
            ) &&  (pSrvrStmt->NA_supported == false))
        {
            retcode = RECOVERY_FROM_ROWSET_ERROR2(
                    pSrvrStmt,
                    pDesc,
                    pStmt,
                    inputRowCnt,
                    &rowsAffected_cli);
            COPYSQLERROR_LIST_TO_SRVRSTMT(pSrvrStmt);
            if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
                retcode = ROWSET_SQL_ERROR;
        }
        else
            retcode = SQL_ERROR;

        *rowsAffected=(Int32)(rowsAffected_cli << 32 >> 32);
        *rowsAffectedHigherBytes=(Int32)(rowsAffected_cli>>32);
        goto ret;
    }
    else
    {
        HANDLE_ERROR(retcode, sqlWarning);
        raretcode = WSQL_EXEC_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &rowsAffected_cli, NULL, 0, NULL);
        if (raretcode < SQL_SUCCESS)
        {
            *rowsAffected = -1;
            *rowsAffectedHigherBytes = -1;
        }
        else if (raretcode == 8411)
        {
            if (GetRowsAffected(pSrvrStmt) < SQL_SUCCESS)
            {
                *rowsAffected = -1;
                *rowsAffectedHigherBytes = -1;
            }
        }
        else
        {
            *rowsAffected=(Int32)(rowsAffected_cli << 32 >> 32);
            *rowsAffectedHigherBytes=(Int32)(rowsAffected_cli>>32);
        }

    }

    // Added for MODE_SPECIAL_1 behavior
    /*
       if (srvrGlobal->modeSpecial_1 && pSrvrStmt->sqlStmtType == TYPE_INSERT_PARAM)
       {
    // In case where we have all the rows in the rowset failing with error -8102 (dup key) then
    // SQL is returning us code 100 without any diagnostics. So, we will treat this case as
    // a successesful insert.
    if( retcode == 100 ) {
    sqlWarning = false;
    retcode = SQL_SUCCESS;
    }

    if( *rowsAffected >= 0 && *rowsAffected < inputRowCnt && retcode != -1 )
    {
    if( pSrvrStmt->numErrRows > 0 )	// There are errors in diags
     *rowsAffected = inputRowCnt - pSrvrStmt->numErrRows;
     else
     *rowsAffected = inputRowCnt;
     }
     }
     */

ret:

    if (retcode < SQL_SUCCESS)
        CLI_DEBUG_RETURN_SQL(retcode);

    // Initialize this to TRUE later in this function depending on sqlStmtType if SQL_SELECT
    // then and retcode is SUCCESS or SUCCESS_WITH_INFO set it to FALSE. Since SQL opens CURSOR
    // after execute in case of SUCCESS or SUCCESS_WITH_INFO and closes only when it reaches
    // NO_DATA_FOUND or CLOSE STATEMENT or COMMIT WORK or ROLLBACK WORK.
    if (sqlStmtType == TYPE_SELECT)
        pSrvrStmt->isClosed = FALSE;

    // Added for fix to SQL returning sqlcode=SQL_NO_DATA_FOUND for non-select
    // stmts when no rows get affected - 10/03/06
    if ((sqlWarning || retcode == SQL_SUCCESS_WITH_INFO)
            && !(IGNORE_NODATAFOUND(sqlStmtType, execretcode)))
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
    else
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}  // end EXECUTE2withRowsets

// We need to change rowsAffected in pSrvrStmt to __int64
SQLRETURN GetRowsAffected(SRVR_STMT_HDL *pSrvrStmt)
{
    SQLDESC_ID *rowCountDesc = NULL;
    SQLMODULE_ID * module = NULL;

    Int32 retcode = SQL_SUCCESS;
    BOOL sqlWarning = FALSE;

    __int64 rowsAffected_ = 0;
    SQLDIAG_STMT_INFO_ITEM_ID sqlItem = SQLDIAG_ROW_COUNT;
    MEMORY_ALLOC_OBJ(rowCountDesc, SQLDESC_ID);
    rowCountDesc->version = SQLCLI_ODBC_VERSION;
    rowCountDesc->name_mode = desc_handle;
    MEMORY_ALLOC_OBJ(module, SQLMODULE_ID);
    rowCountDesc->module = module;
    module->module_name = 0;
    module->charset = SQLCHARSETSTRING_UTF8;
    module->module_name_len = 0;
    module->version = SQLCLI_ODBC_VERSION;
    rowCountDesc->identifier = 0;
    rowCountDesc->handle = 0;

    // get number of rows affected

    retcode = WSQL_EXEC_AllocDesc (rowCountDesc, 1);
    HANDLE_ERROR(retcode, sqlWarning);

    retcode = WSQL_EXEC_SetDescItem(rowCountDesc, 1,
            SQLDESC_TYPE_FS,
            134, 0);
    HANDLE_ERROR(retcode, sqlWarning);

    retcode = WSQL_EXEC_SetDescItem(rowCountDesc, 1,
            SQLDESC_VAR_PTR,
            (long) &rowsAffected_, 0);
    HANDLE_ERROR(retcode, sqlWarning);

    Int32 *temp;
    temp = (Int32 *) &sqlItem;

    retcode = WSQL_EXEC_GetDiagnosticsStmtInfo(temp, rowCountDesc);
    HANDLE_ERROR(retcode, sqlWarning);

    pSrvrStmt->rowsAffected = (Int32)(rowsAffected_ << 32 >> 32);
    pSrvrStmt->rowsAffectedHigherBytes = (Int32)(rowsAffected_ >> 32);

    // free up resources
    WSQL_EXEC_DeallocDesc(rowCountDesc);
    MEMORY_DELETE_ARRAY(rowCountDesc->module);
    MEMORY_DELETE_ARRAY(rowCountDesc);

    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);

} // SRVR::GetRowsAffected()


//========================NOT ATOMIC ROWSET==================================

    extern "C"
SQLRETURN GETNOTATOMICROWSET2(bool& bSQLMessageSet, ERROR_DESC_LIST_def *sqlWarning, SRVR_STMT_HDL* pSrvrStmt)
{
#define MAX_SQLSTATE_LEN 6
#define MAX_MSG_TEXT_LEN 350

    SQLDIAG_COND_INFO_ITEM_VALUE* pCondInfoItems = NULL;
    Int32* pSqlcode = NULL;
    Int32* pRowNumber = NULL;
    Int32* pSqlstateLen = NULL;
    Int32* pMessageTextLen = NULL;
    char* pSqlstate = NULL;
    char* pMessageText = NULL;
    UInt32 gbuf_len;
    char* gbuf_ptr = NULL;

    odbc_SQLSvc_SQLError SSQLError;
    odbc_SQLSvc_SQLError* SQLError = &SSQLError;

    Int32 retcode = SQL_SUCCESS;
    Int32 total_conds = 0;

    Int32 sqlcode  = 0;
    char sqlState[MAX_SQLSTATE_LEN+1];
    Int32 row_number;
    char MessageText[MAX_MSG_TEXT_LEN+1];

    Int32 no_of_cond_items;
    int i;

    SQLDESC_ID	*pDesc;
    SQLSTMT_ID	*pStmt;
    if (pSrvrStmt != NULL )
    {
        pDesc = &pSrvrStmt->inputDesc;
        pStmt = &pSrvrStmt->stmt;
    }

    retcode =  WSQL_EXEC_GetDiagnosticsStmtInfo2(NULL,
            SQLDIAG_NUMBER,
            (Int64*)&total_conds, NULL, 0, NULL);
    if (total_conds == 0)
    {
        kdsCreateSQLErrorException(SQLError, 1, bSQLMessageSet);
        kdsCopySQLErrorException(SQLError, "No error message in SQL diagnostics area, but sqlcode is non-zero", retcode, "");
    }
    else
    {
        // we need following ids: SQLCODE, SQLSTATE, ROW_NUMBER, MESSAGE TEXT

        kdsCreateSQLErrorException(SQLError, total_conds, bSQLMessageSet);

        no_of_cond_items = 4 * total_conds;

        gbuf_len = no_of_cond_items * sizeof(SQLDIAG_COND_INFO_ITEM_VALUE) +
            total_conds * sizeof(Int32) +
            total_conds * sizeof(Int32) +
            total_conds * sizeof(Int32) +
            total_conds * sizeof(Int32) +
            total_conds * (MAX_SQLSTATE_LEN + 1) +
            total_conds * (MAX_MSG_TEXT_LEN + 1);

        MEMORY_ALLOC_ARRAY(gbuf_ptr, char, gbuf_len);
        if (gbuf_ptr == NULL)
        {
            kdsCopySQLErrorException(SQLError, "No Memory Error : From GETNOTATOMICROWSET2",	999, "");
            retcode = 999;
            goto bailout;
        }

        pCondInfoItems = (SQLDIAG_COND_INFO_ITEM_VALUE *)gbuf_ptr;

        pSqlcode = (Int32*)(pCondInfoItems + no_of_cond_items);
        pRowNumber = pSqlcode + total_conds;
        pSqlstateLen = pRowNumber + total_conds;
        pMessageTextLen = pSqlstateLen + total_conds;
        pSqlstate = (char*)(pMessageTextLen + total_conds);
        pMessageText = pSqlstate + total_conds * (MAX_SQLSTATE_LEN + 1);

        for (i=0; i < total_conds; i++)
        {
            pCondInfoItems[4*i+0].item_id_and_cond_number.item_id = SQLDIAG_SQLCODE;
            pCondInfoItems[4*i+1].item_id_and_cond_number.item_id = SQLDIAG_RET_SQLSTATE;
            pCondInfoItems[4*i+2].item_id_and_cond_number.item_id = SQLDIAG_ROW_NUMBER;
            pCondInfoItems[4*i+3].item_id_and_cond_number.item_id = SQLDIAG_MSG_TEXT;
            //
            pCondInfoItems[4*i+0].item_id_and_cond_number.cond_number_desc_entry = i+1;
            pCondInfoItems[4*i+1].item_id_and_cond_number.cond_number_desc_entry = i+1;
            pCondInfoItems[4*i+2].item_id_and_cond_number.cond_number_desc_entry = i+1;
            pCondInfoItems[4*i+3].item_id_and_cond_number.cond_number_desc_entry = i+1;
            //SQLCODE
            pCondInfoItems[4*i+0].string_val = 0;
            pCondInfoItems[4*i+0].num_val_or_len = pSqlcode + i;
            //SQLSTATE
            pCondInfoItems[4*i+1].string_val = pSqlstate + i * (MAX_SQLSTATE_LEN+1);
            pSqlstateLen[i] = MAX_SQLSTATE_LEN;
            pCondInfoItems[4*i+1].num_val_or_len = &pSqlstateLen[i];
            //ROW_NUMBER
            pCondInfoItems[4*i+2].string_val = 0;
            pCondInfoItems[4*i+2].num_val_or_len = pRowNumber + i;
            //MESSAGE TEXT
            pCondInfoItems[4*i+3].string_val = pMessageText + i * (MAX_MSG_TEXT_LEN+1);
            pMessageTextLen[i] = MAX_MSG_TEXT_LEN;
            pCondInfoItems[4*i+3].num_val_or_len = &pMessageTextLen[i];
        }
        retcode = SQL_EXEC_GetDiagnosticsCondInfo3(no_of_cond_items, pCondInfoItems);

        if (retcode < SQL_SUCCESS)
        {
            kdsCopySQLErrorException(SQLError, "Internal Error : From GetDiagnosticsCondInfo3",	retcode, "");
            goto bailout;
        }

        sqlState[MAX_SQLSTATE_LEN]=0;
        MessageText[MAX_MSG_TEXT_LEN]=0;

        for (i=0; i < total_conds; i++)
        {
            sqlcode = (Int32)*(pCondInfoItems[4*i+0].num_val_or_len);
            strcpy(sqlState,pCondInfoItems[4*i+1].string_val);
            row_number = (Int32)*(pCondInfoItems[4*i+2].num_val_or_len);
            strcpy(MessageText,pCondInfoItems[4*i+3].string_val);

            //if (srvrGlobal->modeSpecial_1)
            //	doErr = true;

            //for NAR surrogate key feature
            if (sqlcode == -8108 && pSrvrStmt != NULL)
            {
                retcode = RECOVERY_FOR_SURROGATE_ERROR2(pSrvrStmt,pStmt,pDesc,row_number);
                if (retcode != SQL_SUCCESS)
                    kdsCopySQLErrorExceptionAndRowCount(
                            SQLError,
                            MessageText,
                            sqlcode,
                            sqlState,
                            row_number + 1);
                //else
                // do nothing for now
                //if (srvrGlobal->modeSpecial_1 && retcode == SQL_SUCCESS)
                //	doErr = false;
            }
            else
                //for NAR surrogate key feature

                kdsCopySQLErrorExceptionAndRowCount(
                        SQLError,
                        MessageText,
                        sqlcode,
                        sqlState,
                        row_number + 1);

            // Added for MODE_SPECIAL_1 behavior
            // Increment pSrvrStmt->numErrRows only for the first occurrence of the
            // current row_number. The row_number in the SQL diags area is 0-based.
            //if( srvrGlobal->modeSpecial_1 && doErr && row_number >= 0 )
            //{
            //	if( pSrvrStmt->errRowsArray[ row_number ] == 0x0 ) {
            //		pSrvrStmt->errRowsArray[ row_number ] = 0x1;
            //		pSrvrStmt->numErrRows++;
            //	}
            //}

        }
    }
bailout:
    WSQL_EXEC_ClearDiagnostics(NULL);
    sqlWarning->_length = SQLError->errorList._length;
    sqlWarning->_buffer = SQLError->errorList._buffer;
    MEMORY_DELETE_ARRAY(gbuf_ptr);
    
    CLI_DEBUG_RETURN_SQL(retcode);
}


// Should be only called from EXECUTE2withRowsets thru GETNOTATOMICROWSET2 functions
SQLRETURN RECOVERY_FOR_SURROGATE_ERROR2(SRVR_STMT_HDL* pSrvrStmt,
        SQLSTMT_ID	*pStmt,
        SQLDESC_ID	*pDesc,
        Int32 currRowcnt)
{

    short *TypePtr;
    struct SQLCLI_QUAD_FIELDS *QuadPtr_original;
    struct SQLCLI_QUAD_FIELDS *QuadPtr;
    short *curTypePtr;
    struct SQLCLI_QUAD_FIELDS *curQuadPtr;

    SQLRETURN retcode;
    odbc_SQLSvc_SQLError SQLError;
    Int32 paramCount,curParamNo;
    Int32 sqlRowCount = 0;

    Int32 currentRowsetSize = 0;
    Int32 DataType;
    Int32 DataLen;
    Int32 sqlMaxLength;
    Int32 currentRowCount = 0;
    Int32 RowsetSize;

    paramCount = pSrvrStmt->paramCount;
    currentRowCount = currRowcnt;
    currentRowsetSize = RowsetSize = 1;

    //	TypePtr = (short*)pSrvrStmt->inputDescVarBuffer;
    //	Int32 quad_length = sizeof(SQLCLI_QUAD_FIELDS) * (paramCount + 1);
    //	QuadPtr_original = (struct SQLCLI_QUAD_FIELDS *)(pSrvrStmt->inputDescVarBuffer + sizeof(short) * paramCount);
    //	QuadPtr = QuadPtr_original + paramCount + 1;

    Int32 quad_length       = sizeof(SQLCLI_QUAD_FIELDS) * paramCount;
    QuadPtr_original  = pSrvrStmt->inputQuadList;
    QuadPtr           = pSrvrStmt->inputQuadList_recover;

    memcpy(QuadPtr, QuadPtr_original, quad_length);


    retcode = GETSQLERROR_AND_ROWCOUNT(pSrvrStmt->bSQLMessageSet,&SQLError,1,currentRowCount,&sqlRowCount);

    QuadPtr->var_ptr = (void*)&currentRowsetSize;

    for (curParamNo = 1 ; curParamNo < paramCount ; curParamNo++)
    {
        curQuadPtr = QuadPtr + curParamNo;
        DataType   = pSrvrStmt->IPD[curParamNo - 1].dataType;
        DataLen    = curQuadPtr->var_layout;

        switch (DataType)
        {
            case SQLTYPECODE_VARCHAR_WITH_LENGTH:
            case SQLTYPECODE_VARCHAR_LONG:
            case SQLTYPECODE_BITVAR:
                sqlMaxLength = DataLen + 2;
                sqlMaxLength = ((sqlMaxLength + 2 - 1)>>1)<<1;
                break;
            default:
                sqlMaxLength = DataLen;
                break;
        }  // end switch

        curQuadPtr->var_ptr = (BYTE*)curQuadPtr->var_ptr + (sqlMaxLength * (currentRowCount));

        if (curQuadPtr->ind_ptr != NULL)
            curQuadPtr->ind_ptr = (BYTE*)curQuadPtr->ind_ptr + (currentRowCount * sizeof(short));
    }  // end for

    retcode = WSQL_EXEC_SETROWSETDESCPOINTERS( pDesc
            , 1
            , NULL
            , 1
            , paramCount
            , QuadPtr
            );
    if (retcode < SQL_SUCCESS)
    {
        GETSQLERROR_AND_ROWCOUNT(pSrvrStmt->bSQLMessageSet, &SQLError, 1 , currentRowCount, &sqlRowCount);
        ADDSQLERROR_TO_LIST( pSrvrStmt, &SQLError, currentRowCount + 1);
        CLI_DEBUG_RETURN_SQL(SQL_ERROR);
    }
    retcode = WSQL_EXEC_ExecFetch(pStmt, pDesc, 0);

    if (retcode == SQL_SUCCESS) pSrvrStmt->rowsAffected++ ;

    // Added for fix to SQL returning sqlcode=SQL_NO_DATA_FOUND for non-select
    // stmts when no rows get affected - 10/03/06
    if (retcode == SQL_NO_DATA_FOUND)
        retcode = SQL_SUCCESS;

    CLI_DEBUG_RETURN_SQL(retcode);

}

SQLRETURN RECOVERY_FROM_ROWSET_ERROR2(
        SRVR_STMT_HDL *pSrvrStmt
        , SQLDESC_ID	   *pDesc
        , SQLSTMT_ID	   *pStmt
        , Int32       inputRowCnt
        , Int64      *totalrowsAffected
        )
{
    short                     *TypePtr;
    struct SQLCLI_QUAD_FIELDS *QuadPtr_original;
    struct SQLCLI_QUAD_FIELDS *QuadPtr;
    short                     *curTypePtr;
    struct SQLCLI_QUAD_FIELDS *curQuadPtr;

    SQLRETURN            retcode;
    odbc_SQLSvc_SQLError SQLError;
    Int32                 paramCount        = pSrvrStmt->paramCount;
    Int32                 curParamNo;
    Int32                 sqlRowCount;
    Int32                 currentRowCount   = 0;
    Int32                 prevRowCount      = 0;
    Int32                 currentRowsetSize = 0;
    Int64                 rowsAffected      = 0;
    Int32             RowsetSize;
    Int32                 DataType;
    Int32                 DataLen;
    Int32                 sqlMaxLength;
    Int32                 quad_length;
    SQLRETURN execRetCode;	// Added for MODE_SPECIAL_1 behavior

    RowsetSize        = inputRowCnt;
    currentRowsetSize = RowsetSize;
    quad_length       = sizeof(SQLCLI_QUAD_FIELDS) * paramCount;
    QuadPtr_original  = pSrvrStmt->inputQuadList;
    QuadPtr           = pSrvrStmt->inputQuadList_recover;


    Int32 prevErrRow = -1;

    memcpy(QuadPtr, QuadPtr_original, quad_length);

    *totalrowsAffected = -1;

    retcode = GETSQLERROR_AND_ROWCOUNT(pSrvrStmt->bSQLMessageSet, &SQLError,RowsetSize, currentRowCount, &sqlRowCount);
    if (sqlRowCount == -1)
    {
        ADDSQLERROR_TO_LIST( pSrvrStmt, &SQLError, currentRowCount + 1);
        CLI_DEBUG_RETURN_SQL(SQL_ERROR);
    }
    else
        ADDSQLERROR_TO_LIST( pSrvrStmt, &SQLError, currentRowCount + sqlRowCount+1);

    if (sqlRowCount == 0)
    {
        //for Surrogate key AR
        if (SQLError.errorList._buffer->sqlcode == -8108 )    //if the first row
        {
            prevRowCount = currentRowCount;
            prevErrRow   = currentRowCount;
        }
        //for Surrogate key AR
        else
        {
            prevRowCount = currentRowCount;
            currentRowCount++;
            currentRowsetSize--;
        }

    }
    else
    {
        prevRowCount      = currentRowCount;
        currentRowsetSize = sqlRowCount;
    }

    while( currentRowCount < RowsetSize)
    {
        QuadPtr->var_ptr = &currentRowsetSize;

        for (curParamNo = 1 ; curParamNo < paramCount ; curParamNo++)
        {
            curQuadPtr = QuadPtr + curParamNo;
            DataType   = pSrvrStmt->IPD[curParamNo - 1].dataType;
            DataLen    = curQuadPtr->var_layout;

            switch (DataType)
            {
                case SQLTYPECODE_VARCHAR_WITH_LENGTH:
                case SQLTYPECODE_VARCHAR_LONG:
                case SQLTYPECODE_BITVAR:
                    sqlMaxLength = DataLen + 2;
                    sqlMaxLength = ((sqlMaxLength + 2 - 1)>>1)<<1;
                    break;
                default:
                    sqlMaxLength = DataLen;
                    break;
            }  // end switch

            curQuadPtr->var_ptr = (BYTE*)curQuadPtr->var_ptr + (sqlMaxLength * (currentRowCount - prevRowCount));

            if (curQuadPtr->ind_ptr != NULL)
                curQuadPtr->ind_ptr = (BYTE*)curQuadPtr->ind_ptr + ((currentRowCount - prevRowCount) * sizeof(short));
        }  // end for

        retcode = WSQL_EXEC_SETROWSETDESCPOINTERS( pDesc
                , pSrvrStmt->maxRowsetSize
                , NULL
                , 1
                , paramCount
                , QuadPtr
                );
        if (retcode < SQL_SUCCESS)
        {
            GETSQLERROR_AND_ROWCOUNT(pSrvrStmt->bSQLMessageSet, &SQLError, RowsetSize, currentRowCount, &sqlRowCount);
            ADDSQLERROR_TO_LIST( pSrvrStmt, &SQLError, currentRowCount + 1);
            CLI_DEBUG_RETURN_SQL(SQL_ERROR);
        }
        retcode = execRetCode = WSQL_EXEC_ExecFetch(pStmt, pDesc, 0);
        if (retcode < SQL_SUCCESS)
        {
            GETSQLERROR_AND_ROWCOUNT(pSrvrStmt->bSQLMessageSet, &SQLError, RowsetSize, currentRowCount, &sqlRowCount);
            if (sqlRowCount == -1)
            {
                ADDSQLERROR_TO_LIST( pSrvrStmt, &SQLError, currentRowCount + 1);
                CLI_DEBUG_RETURN_SQL(SQL_ERROR);
            }
            else
            {
                if  (SQLError.errorList._buffer->sqlcode != -8108)
                    ADDSQLERROR_TO_LIST( pSrvrStmt, &SQLError, currentRowCount + sqlRowCount+1);
            }
            if (sqlRowCount == 0)
            {
                //for Surrogate key AR
                if ((SQLError.errorList._buffer->sqlcode == -8108 ))
                {
                    if ( (prevErrRow == currentRowCount) )
                    {
                        ADDSQLERROR_TO_LIST( pSrvrStmt, &SQLError, currentRowCount+1);
                        prevRowCount = currentRowCount;
                        prevErrRow   = currentRowCount;
                        currentRowCount++ ;
                        currentRowsetSize  = RowsetSize - currentRowCount;
                    }
                    else
                    {
                        prevRowCount = currentRowCount;
                        prevErrRow   = currentRowCount;
                        currentRowsetSize  = RowsetSize - currentRowCount;
                    }
                }
                else
                {
                    prevRowCount = currentRowCount;
                    currentRowCount++;
                    currentRowsetSize  = RowsetSize - currentRowCount;
                }
            }

            else
            {
                prevRowCount = currentRowCount;
                currentRowsetSize = sqlRowCount;
            }
        }  // end if retcode < SQL_SUCCESS
        else
        {
            if (retcode > 0)
                WSQL_EXEC_ClearDiagnostics(NULL);
            if (WSQL_EXEC_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &rowsAffected, NULL, 0, NULL) < SQL_SUCCESS)
                CLI_DEBUG_RETURN_SQL(SQL_ERROR);
            retcode = COMMIT_ROWSET(pSrvrStmt->dialogueId, pSrvrStmt->bSQLMessageSet, &SQLError, currentRowCount);
            if (retcode != SQL_SUCCESS)
            {
                ADDSQLERROR_TO_LIST( pSrvrStmt, &SQLError, currentRowCount + 1);
                CLI_DEBUG_RETURN_SQL(SQL_ERROR);
            }

            // Added for MODE_SPECIAL_1 behavior
            /*
               if (srvrGlobal->modeSpecial_1 && pSrvrStmt->sqlStmtType == TYPE_INSERT_PARAM) {
               if ( *totalrowsAffected == -1)
             *totalrowsAffected = currentRowsetSize;
             else
             *totalrowsAffected += currentRowsetSize;
             }
             else {
             */
            if ( *totalrowsAffected == -1)
                *totalrowsAffected = rowsAffected;
            else
                *totalrowsAffected += rowsAffected;
            //}

            //for Surrogate key AR
            if ( (SQLError.errorList._buffer->sqlcode == -8108 ))
            {
                prevRowCount = currentRowCount;
                //if( srvrGlobal->modeSpecial_1 && (execRetCode == SQL_SUCCESS || execRetCode == SQL_SUCCESS_WITH_INFO ||
                //			execRetCode == SQL_NO_DATA_FOUND) )
                //	rowsAffected = currentRowsetSize;
                if (pSrvrStmt->sqlStmtType == TYPE_UPDATE || pSrvrStmt->sqlStmtType == TYPE_DELETE) // should not touch for update/delete
                    rowsAffected = currentRowsetSize;
                currentRowCount += rowsAffected ;
                prevErrRow  = currentRowCount;
                currentRowsetSize  = RowsetSize - currentRowCount;
            }
            else
                //for Surrogate key AR
            {
                prevRowCount = currentRowCount;
                // Added for MODE_SPECIAL_1 behavior
                // In the following conditions the rowsAffected should be the same as
                // the currentRowsetSize. If all rows are duplicate in the rowset then
                // SQL returns SQL_NO_DATA_FOUND (100)
                /*if( srvrGlobal->modeSpecial_1 && (execRetCode == SQL_SUCCESS || execRetCode == SQL_SUCCESS_WITH_INFO ||
                  execRetCode == SQL_NO_DATA_FOUND) )
                  rowsAffected = currentRowsetSize; */
                if (pSrvrStmt->sqlStmtType == TYPE_UPDATE || pSrvrStmt->sqlStmtType == TYPE_DELETE) // should not touch for update/delete
                    rowsAffected = currentRowsetSize;
                currentRowCount += rowsAffected + 1;
                currentRowsetSize  = RowsetSize - currentRowCount;
            }
        } // end else
    }  // end while

    //return SQL_SUCCESS_WITH_INFO;
    if ( *totalrowsAffected == -1)  *totalrowsAffected =0;

    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);

}  // end RECOVERY_FROM_ROWSET_ERROR2


void COPYSQLERROR_LIST_TO_SRVRSTMT(SRVR_STMT_HDL* pSrvrStmt)
{
    Int32 i;
    Int32 totallength = pSrvrStmt->sqlWarning._length;
    ERROR_DESC_def* pErrorDesc;
    ROWSET_ERROR_NODE* currNode, *nextNode;
    if (pSrvrStmt->rowsetErrorList.nodeCount == 0)
        return;
    pSrvrStmt->bSQLMessageSet = true;
    currNode = pSrvrStmt->rowsetErrorList.firstNode;
    for (i = 0; i < pSrvrStmt->rowsetErrorList.nodeCount; i++)
    {
        totallength += currNode->SQLError.errorList._length;
        currNode = currNode->nextNode;
    }
    if (totallength == 0)
        return;
    ERROR_DESC_def* _buffer = NULL;
    MEMORY_ALLOC_ARRAY(_buffer, ERROR_DESC_def, totallength);
    
    pErrorDesc = _buffer;
    if (pSrvrStmt->sqlWarning._length > 0)
    {
        memcpy(pErrorDesc, pSrvrStmt->sqlWarning._buffer, pSrvrStmt->sqlWarning._length * sizeof(ERROR_DESC_def));
        pErrorDesc += pSrvrStmt->sqlWarning._length;
        MEMORY_DELETE_ARRAY(pSrvrStmt->sqlWarning._buffer);
    }

    currNode = pSrvrStmt->rowsetErrorList.firstNode;

    for (i = 0; i < pSrvrStmt->rowsetErrorList.nodeCount; i++)
    {
        memcpy(pErrorDesc, currNode->SQLError.errorList._buffer, currNode->SQLError.errorList._length * sizeof(ERROR_DESC_def));
        pErrorDesc += currNode->SQLError.errorList._length;
        currNode = currNode->nextNode;
    }
    pSrvrStmt->sqlWarning._length = totallength;
    pSrvrStmt->sqlWarning._buffer = _buffer;

    currNode = pSrvrStmt->rowsetErrorList.firstNode;
    while (currNode != NULL)
    {
        nextNode = currNode->nextNode;
        MEMORY_DELETE_OBJ(currNode);
        currNode = nextNode;
    }
    pSrvrStmt->rowsetErrorList.nodeCount = 0;
    pSrvrStmt->rowsetErrorList.firstNode = NULL;

    return;
}

SQLRETURN GETSQLERROR_AND_ROWCOUNT(
        bool& bSQLMessageSet,
        odbc_SQLSvc_SQLError *SQLError,
        Int32 RowsetSize,
        Int32 currentRowCount,
        Int32* errorRowCount)
{
    Int32 retcode;
    Int32 total_conds = 0;
    char *msg_buf;
    Int32 buf_len;
    Int32 sqlcode  = 0;
    char sqlState[6];
    Int32 curr_cond = 1;
    Int32 msg_buf_len;
    ERROR_DESC_def *error_desc_def;
    Int32 sqlRowCount = -1;
    *errorRowCount = -1;
    char  strNow[TIMEBUFSIZE + 1];

    retcode =  WSQL_EXEC_GetDiagnosticsStmtInfo2(NULL,
            SQLDIAG_NUMBER,
            (Int64*)&total_conds, NULL, 0, NULL);
    if (total_conds == 0)
    {
        kdsCreateSQLErrorException(SQLError, 1, bSQLMessageSet);
        kdsCopySQLErrorExceptionAndRowCount(SQLError, "Internal Error: From GetStmtDiagnostics ", retcode, "", currentRowCount);
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
    }

    kdsCreateSQLErrorException(SQLError, total_conds, bSQLMessageSet);
    while (curr_cond <= total_conds)
    {
        retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_SQLCODE, curr_cond,
                &sqlcode, NULL, 0, NULL);
        if (retcode >= SQL_SUCCESS)
        {
            if (sqlcode == 100)
            {
                // We are not copying the Warning message if the error code is 100
                // It is ok, though we have allocated more SQLError, but length is incremented
                // only when SQLError is copied
                curr_cond++;
                continue;
            }
            retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_MSG_LEN, curr_cond,
                    &msg_buf_len, NULL, 0, NULL);
        }
        if (retcode >= SQL_SUCCESS)
        {
            msg_buf_len = (msg_buf_len+1)*4;
            msg_buf_len += TIMEBUFSIZE;
            MEMORY_ALLOC_ARRAY(msg_buf, char, msg_buf_len);
            buf_len = 0;
            retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_MSG_TEXT, curr_cond,
                    NULL, msg_buf, msg_buf_len, &buf_len);
        }
        if (retcode >= SQL_SUCCESS)
        {
            msg_buf[buf_len] = '\0';
            //Get the timetsamp
            time_t  now = time(NULL);
            bzero(strNow, sizeof(strNow) );
            strftime(strNow, sizeof(strNow), " [%Y-%m-%d %H:%M:%S]", localtime(&now));
            strcat(msg_buf, strNow);
            buf_len = 0;
            retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_RET_SQLSTATE, curr_cond,
                    NULL, sqlState, sizeof(sqlState), &buf_len);
        }
        if (retcode >= SQL_SUCCESS)
        {
            retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_ROW_NUMBER, curr_cond,
                    &sqlRowCount, NULL, 0, NULL);
        }
        if (retcode >= SQL_SUCCESS)
            sqlState[5] = '\0';
        else
        {
            kdsCopySQLErrorExceptionAndRowCount(SQLError, "Internal Error : From GetCondDiagnostics ", retcode, "", currentRowCount);
            MEMORY_DELETE_ARRAY(msg_buf);
            break;
        }
        kdsCopySQLErrorException(SQLError, msg_buf, sqlcode, sqlState);
        
        MEMORY_DELETE_ARRAY(msg_buf);
        if (fatalSQLError(sqlcode))
        {
            *errorRowCount = -1;
            WSQL_EXEC_ClearDiagnostics(NULL);
            CLI_DEBUG_RETURN_SQL(SQL_ERROR);
        }
        if (sqlRowCount < 0 || sqlRowCount > RowsetSize)
            sqlRowCount = -1;
        if (sqlRowCount != -1) *errorRowCount = sqlRowCount;
        curr_cond++;
    }
    if (*errorRowCount != -1)
        currentRowCount += *errorRowCount;

    if (total_conds)
        bSQLMessageSet = true;

    curr_cond = 0;
    while(curr_cond < total_conds)
    {
        error_desc_def = SQLError->errorList._buffer + curr_cond;
        error_desc_def->rowId = currentRowCount + 1;
        curr_cond++;
    }

    WSQL_EXEC_ClearDiagnostics(NULL);

    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

void ADDSQLERROR_TO_LIST(
        SRVR_STMT_HDL* pSrvrStmt,
        odbc_SQLSvc_SQLError *SQLError,
        Int32 rowCount)
{
    ROWSET_ERROR_NODE* pNode = NULL;
    UInt32 i;

    MEMORY_ALLOC_OBJ(pNode, ROWSET_ERROR_NODE);

    pNode->rowNumber = rowCount;
    pNode->SQLError.errorList._length = SQLError->errorList._length;
    pNode->SQLError.errorList._buffer = SQLError->errorList._buffer;

    INSERT_NODE_TO_LIST(pSrvrStmt, pNode, rowCount);

    return;
}

void INSERT_NODE_TO_LIST(SRVR_STMT_HDL* pSrvrStmt, ROWSET_ERROR_NODE* pNode, Int32 rowCount)
{
    ROWSET_ERROR_NODE* currNode=NULL, *nextNode=NULL, *prevNode=NULL;

    currNode = pSrvrStmt->rowsetErrorList.firstNode;

    while (currNode != NULL)
    {
        if (currNode->rowNumber < rowCount) // insert after it
        {
            nextNode = currNode->nextNode;
            currNode->nextNode = pNode;
            pNode->nextNode = nextNode;
            break;
        }
        prevNode = currNode;
        currNode = currNode->nextNode;
    }

    if (currNode == NULL)
    {
        if (prevNode == NULL) // list is empty
        {
            pSrvrStmt->rowsetErrorList.firstNode = pNode;
        }
        else				// insert after last element
        {
            prevNode->nextNode = pNode;
        }
        pNode->nextNode = NULL;
    }
    pSrvrStmt->rowsetErrorList.nodeCount++;

    return;
}

SQLRETURN GETSQLERROR2(bool& bSQLMessageSet,
        odbc_SQLSvc_SQLError *SQLError)
{
    Int32 retcode;
    Int32 total_conds = 0;
    char *msg_buf;
    Int32 buf_len;
    Int32 sqlcode  = 0;
    char sqlState[6];
    Int32 curr_cond = 1;
    Int32 msg_buf_len;
    Int32 sqlRowCount;
    char  strNow[TIMEBUFSIZE + 1];

    retcode =  WSQL_EXEC_GetDiagnosticsStmtInfo2(NULL,
            SQLDIAG_NUMBER,
            (Int64*)&total_conds, NULL, 0, NULL);
    if (total_conds == 0)
    {
        kdsCreateSQLErrorException(SQLError, 1, bSQLMessageSet);
        kdsCopySQLErrorException(SQLError, "No error message in SQL diagnostics area, but sqlcode is non-zero", retcode, "");
    }
    else
    {
        kdsCreateSQLErrorException(SQLError, total_conds, bSQLMessageSet);
        while (curr_cond <= total_conds)
        {
            retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_SQLCODE, curr_cond,
                    &sqlcode, NULL, 0, NULL);
            if (retcode >= SQL_SUCCESS)
            {
                if (sqlcode == 100)
                {
                    // We are not copying the Warning message if the error code is 100
                    // It is ok, though we have allocated more SQLError, but length is incremented
                    // only when SQLError is copied
                    curr_cond++;
                    continue;
                }
                retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_MSG_LEN, curr_cond,
                        &msg_buf_len, NULL, 0, NULL);
            }
            if (retcode >= SQL_SUCCESS)
            {
                msg_buf_len = (msg_buf_len+1)*4 + TIMEBUFSIZE;
                MEMORY_ALLOC_ARRAY(msg_buf, char, msg_buf_len);
                buf_len = 0;
                retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_MSG_TEXT, curr_cond,
                        NULL, msg_buf, msg_buf_len, &buf_len);
            }
            if (retcode >= SQL_SUCCESS)
            {
                msg_buf[buf_len] = '\0';
                //Get the timetsamp
                time_t  now = time(NULL);
                bzero(strNow, sizeof(strNow) );
                strftime(strNow, sizeof(strNow), " [%Y-%m-%d %H:%M:%S]", localtime(&now));
                strcat(msg_buf, strNow);
                buf_len = 0;
                retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_RET_SQLSTATE, curr_cond,
                        NULL, sqlState, sizeof(sqlState), &buf_len);
            }
            if (retcode >= SQL_SUCCESS)
                sqlState[5] = '\0';
            else
            {
                kdsCopySQLErrorException(SQLError, "Internal Error : From GetCondDiagnostics",
                        retcode, "");
                MEMORY_DELETE_ARRAY(msg_buf);
                break;
            }
            sqlRowCount = -1;
            retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_ROW_NUMBER, curr_cond,
                    &sqlRowCount, NULL, 0, NULL);

            if (retcode < SQL_SUCCESS)
                sqlRowCount = -1;
            else if (sqlRowCount != -1)
                sqlRowCount++;

            kdsCopySQLErrorExceptionAndRowCount(SQLError, msg_buf, sqlcode, sqlState, sqlRowCount);
            MEMORY_DELETE_ARRAY(msg_buf);
            curr_cond++;
        }

        WSQL_EXEC_ClearDiagnostics(NULL);
    }

    CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN EXECUTE2(SRVR_STMT_HDL* pSrvrStmt)
{
    Int32 inputRowCnt = pSrvrStmt->inputRowCnt;
    IDL_short sqlStmtType = pSrvrStmt->sqlStmtType;
    Int64 rowsAffected_cli;
    Int32 *rowsAffected = &pSrvrStmt->rowsAffected;
    Int32 *rowsAffectedHigherBytes = &pSrvrStmt->rowsAffectedHigherBytes;

    Int32 retcode = SQL_SUCCESS;

    SQLDESC_ID *pInputDesc;
    SQLDESC_ID *pOutputDesc;
    SQLSTMT_ID *pStmt;
    SQLSTMT_ID cursorId;
    Int32		paramCount;
    char		*cursorName;
    Int32		len;
    Int32		curRowCnt;
    Int32		curParamNo;
    Int32		curLength;
    BYTE		*varPtr;
    BYTE		*indPtr;
    void			*pBytes;
    SQLValue_def	*SQLValue;
    BOOL			sqlWarning = FALSE;
    SRVR_DESC_HDL	*IPD;

    Int32		raretcode	= SQL_SUCCESS;

    pSrvrStmt->PerfFetchRetcode = SQL_SUCCESS;
    pSrvrStmt->RowsetFetchRetcode = SQL_SUCCESS;

    pStmt = &pSrvrStmt->stmt;

    // Initialize this to TRUE later in this function depending on sqlStmtType if SQL_SELECT
    // then and retcode is SUCCESS or SUCCESS_WITH_INFO set it to FALSE. Since SQL opens CURSOR
    // after execute in case of SUCCESS or SUCCESS_WITH_INFO and closes only when it reaches
    // NO_DATA_FOUND or CLOSE STATEMENT or COMMIT WORK or ROLLBACK WORK.
    pSrvrStmt->isClosed = TRUE;

    if (pStmt == NULL)
        CLI_DEBUG_RETURN_SQL(SQL_INVALID_HANDLE);

    //
    // Handle statements.
    //
    if (    pSrvrStmt->stmtType == EXTERNAL_STMT
            &&  (pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE || pSrvrStmt->sqlStmtType == TYPE_CALL)
            &&  pSrvrStmt->moduleId.module_name == NULL)
    {
        cursorName = pSrvrStmt->cursorName;
        // If cursor name is not specified, use the stmt name as cursor name
        if (*cursorName == '\0')
            cursorName = pSrvrStmt->stmtName;
        // If cursorName has chg'd from last EXEC or EXECDIRECT cmd
        // or has not yet been set for the first time call SetCursorName
        if ((strcmp(pSrvrStmt->previousCursorName, cursorName) != 0) && *cursorName != '\0')
        {
            cursorId.version        = SQLCLI_ODBC_VERSION;
            cursorId.module         = pStmt->module;
            cursorId.handle         = 0;
            cursorId.charset        = SQLCHARSETSTRING_UTF8;
            cursorId.name_mode      = cursor_name;
            if (*cursorName == '\0')
                cursorId.identifier_len = pSrvrStmt->stmtNameLen;
            else
                cursorId.identifier_len = pSrvrStmt->cursorNameLen;
            cursorId.identifier     = cursorName;
            strcpy(pSrvrStmt->previousCursorName, cursorName);	// keep track of last cursorName
            retcode = WSQL_EXEC_SetCursorName(pStmt, &cursorId);
            HANDLE_ERROR(retcode, sqlWarning);
        }
    }
    pInputDesc = &pSrvrStmt->inputDesc;
    paramCount = pSrvrStmt->paramCount;

    if (  pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE
            || pSrvrStmt->sqlStmtType  == TYPE_CALL)
        pOutputDesc = &pSrvrStmt->outputDesc;
    else
        pOutputDesc = NULL;

    switch (pSrvrStmt->sqlQueryType)
    {
        case SQL_SELECT_UNIQUE:
        case SQL_CALL_NO_RESULT_SETS:
            retcode = WSQL_EXEC_SetDescItem(&pSrvrStmt->outputDesc, 0, SQLDESC_ROWWISE_ROWSET_SIZE, 1, 0);
            if(retcode >= 0)
                retcode = WSQL_EXEC_ClearExecFetchClose(pStmt, pInputDesc, pOutputDesc, 0, NULL, NULL);
            if(retcode == SQL_NO_DATA_FOUND) //SQL does NOT clear diagnostic records, so MXOSRVR clears it
                WSQL_EXEC_ClearDiagnostics(&pSrvrStmt->stmt);
            break;
        case SQL_SELECT_NON_UNIQUE:
            retcode = WSQL_EXEC_Exec(pStmt, pInputDesc, 0);
            break;
        case SQL_OTHER:
        case SQL_UNKNOWN:
        case SQL_CAT_UTIL:		// Added new types. Previously showing up as SQL_OTHER.
            // client side statement typing should not influence decisions on the server
            // removed to resolve issues with LOCK and PURGEDATA statements not being fetched/closed
            //if (sqlStmtType == TYPE_SELECT)
            //retcode = WSQL_EXEC_Exec(pStmt, pInputDesc, 0);
            //else
            retcode = WSQL_EXEC_ClearExecFetchClose(pStmt, pInputDesc, pOutputDesc, 0, NULL, NULL);
            if(retcode == SQL_NO_DATA_FOUND) //SQL does NOT clear diagnostic records, so MXOSRVR clears it
                WSQL_EXEC_ClearDiagnostics(&pSrvrStmt->stmt);
            break;
        case SQL_CALL_WITH_RESULT_SETS:
            WSQL_EXEC_CloseStmt(pStmt); // Added this code since for resultsets, The fetches till end of data will only close resultsets not the call statement.
            retcode = WSQL_EXEC_Exec(pStmt, pInputDesc, 0);
            if (retcode >= 0)
                retcode = WSQL_EXEC_SetDescItem(&pSrvrStmt->outputDesc, 0, SQLDESC_ROWWISE_ROWSET_SIZE, 1, 0);
            if (retcode >= 0)
                retcode = WSQL_EXEC_Fetch(&pSrvrStmt->stmt, &pSrvrStmt->outputDesc, 0);
            break;
        case SQL_RWRS_SPECIAL_INSERT:
            retcode = WSQL_EXEC_ExecFetch(pStmt, pInputDesc, 0);
            break;
        default:
            retcode = WSQL_EXEC_ClearExecFetchClose(pStmt, pInputDesc, pOutputDesc, 0, NULL, NULL);
            if (retcode == -8812)
            {
                WSQL_EXEC_CloseStmt(pStmt);
                retcode = WSQL_EXEC_ClearExecFetchClose(pStmt, pInputDesc, pOutputDesc, 0, NULL, NULL);
            }
            if(retcode == SQL_NO_DATA_FOUND) //SQL does NOT clear diagnostic records, so MXOSRVR clears it
                WSQL_EXEC_ClearDiagnostics(&pSrvrStmt->stmt);
            break;
    } // end switch

    if (retcode != SQL_NO_DATA_FOUND)
        HANDLE_ERROR(retcode, sqlWarning);

    rowsAffected_cli=0;
    *rowsAffected = 0;
    *rowsAffectedHigherBytes = 0;

    //
    // this is to handle the case where AQR warnings are returned for these query types and its a SQL_NO_DATA condition
    // In this case, a AQR warning would be returned instead of SQL_NO_DATA. So would have to check the diagnostics
    // to see if a sql_no_data is retuned in the diagnostics to set the rowsAffected value
    // Using SQL_EXEC_GetDiagnosticsStmtInfo2 to get the rows affected does not work for these query types
    //

    if( pSrvrStmt->sqlQueryType == SQL_INSERT_UNIQUE ||
            pSrvrStmt->sqlQueryType == SQL_UPDATE_UNIQUE ||
            pSrvrStmt->sqlQueryType == SQL_DELETE_UNIQUE ||
            pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE ||
            pSrvrStmt->sqlQueryType == SQL_CALL_NO_RESULT_SETS ||
            pSrvrStmt->sqlQueryType == SQL_CALL_WITH_RESULT_SETS)
    {
        *rowsAffected = 1;

        if(retcode == SQL_NO_DATA_FOUND)
            *rowsAffected = 0;
        else if(retcode > 0 ) // AQR warning (SQL_NO_DATA_FOUND already handle in the if-case above
        {
            Int32 total_conds = 0;
            Int32 curr_cond = 1;
            Int32 sqlcode  = 0;


            WSQL_EXEC_GetDiagnosticsStmtInfo2(NULL, SQLDIAG_NUMBER, (Int64*)&total_conds, NULL, 0, NULL);

            while(curr_cond <= total_conds)
            {
                WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_SQLCODE, curr_cond,	&sqlcode, NULL, 0, NULL);
                if(sqlcode == 100)
                {
                    *rowsAffected = 0;
                    break;
                }
                curr_cond++;
            }
        }
    }


    switch (pSrvrStmt->sqlQueryType)
    {
        case SQL_INSERT_UNIQUE:
        case SQL_UPDATE_UNIQUE:
        case SQL_DELETE_UNIQUE:
            if(retcode == SQL_NO_DATA_FOUND)
                retcode = SQL_SUCCESS;
            // rowsAffected is already taken care of
            break;
        case SQL_SELECT_UNIQUE:
        case SQL_CALL_NO_RESULT_SETS:
        case SQL_CALL_WITH_RESULT_SETS:
            // rowsAffected is already taken care of
            break;
        case SQL_OTHER:
        case SQL_UNKNOWN:
        case SQL_CAT_UTIL:		// Added new types. Previously showing up as SQL_OTHER.
            raretcode = WSQL_EXEC_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &rowsAffected_cli, NULL, 0, NULL);
            if (raretcode < SQL_SUCCESS)
            {
                *rowsAffected = -1;
                *rowsAffectedHigherBytes = -1;
            }
            else if (raretcode == 8411)
            {
                if (GetRowsAffected(pSrvrStmt) < SQL_SUCCESS) // Propagate rowsAffected as int64 once interface is changed
                {
                    *rowsAffected = -1;
                    *rowsAffectedHigherBytes = -1;
                }
            }
            else
            {
                *rowsAffected=(Int32)(rowsAffected_cli << 32 >> 32);
                *rowsAffectedHigherBytes=(Int32)(rowsAffected_cli>>32);
            }
            break;
        case SQL_SELECT_NON_UNIQUE:
            break;
        case SQL_INSERT_NON_UNIQUE:
        case SQL_DELETE_NON_UNIQUE:
        case SQL_UPDATE_NON_UNIQUE:
        default:
            if (retcode != SQL_NO_DATA_FOUND)
            {
                raretcode = WSQL_EXEC_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &rowsAffected_cli, NULL, 0, NULL);
                if (raretcode < SQL_SUCCESS)
                {
                    *rowsAffected = -1;
                    *rowsAffectedHigherBytes = -1;
                }
                else if (raretcode == 8411)
                {
                    if (GetRowsAffected(pSrvrStmt) < SQL_SUCCESS) // Propagate rowsAffected as int64 once interface is changed
                    {
                        *rowsAffected = -1;
                        *rowsAffectedHigherBytes = -1;
                    }
                }
                else
                {
                    *rowsAffected=(Int32)(rowsAffected_cli << 32 >> 32);
                    *rowsAffectedHigherBytes=(Int32)(rowsAffected_cli>>32);
                }
            }
            else
                retcode = SQL_SUCCESS;
            break;
    }

    if (retcode == SQL_NO_DATA_FOUND)
        CLI_DEBUG_RETURN_SQL(SQL_NO_DATA_FOUND);

    // Initialize this to TRUE later in this function depending on sqlStmtType if SQL_SELECT
    // then and retcode is SUCCESS or SUCCESS_WITH_INFO set it to FALSE. Since SQL opens CURSOR
    // after execute in case of SUCCESS or SUCCESS_WITH_INFO and closes only when it reaches
    // NO_DATA_FOUND or CLOSE STATEMENT or COMMIT WORK or ROLLBACK WORK.

    if ( (pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE) ||
            (pSrvrStmt->sqlQueryType == SQL_CALL_WITH_RESULT_SETS))
    {
        pSrvrStmt->isClosed = FALSE;
        pSrvrStmt->bFetchStarted = FALSE;
    }

    //
    // Process any SPJ RS
    //
    if (pSrvrStmt->sqlQueryType == SQL_CALL_WITH_RESULT_SETS)
    {
        Int32            rsError            = 0;
        int             rsNum              = 1;
        char            rsName[MAX_STMT_NAME_LEN+1];
        SRVR_STMT_HDL  *rsSrvrStmt         = NULL;
        SRVR_STMT_HDL  *previousRsSrvrStmt = pSrvrStmt;  // point it to the CALL statement to make things work
        BOOL            done               = FALSE;
        int             prefixLen;
        SQLRETURN       rc;
        char           *suffix1    = "RS";
        int             suffixLen1 = strlen(suffix1);
        char           *suffix2    = "000";
        int             suffixLen2 = strlen(suffix2);

        // Make prefix for Result Set names.
        // For ease of debugging, try to prefix the cursor name with the call statement name.
        // If the call statement name is too Int32, then generate a cursor name.
        // The final form of the names should be something like:
        //
        //  SQL_CUR_123456789RS1
        //  SQL_CUR_123456789RS2
        //  ...
        //
        if (pSrvrStmt->stmtNameLen < (MAX_STMT_NAME_LEN - suffixLen1 - suffixLen2))
        {
            prefixLen          = pSrvrStmt->stmtNameLen;
            strncpy(rsName, pSrvrStmt->stmtName, prefixLen);
        }
        else
        {
            // The name is too Int32, so generate a time tamp name "SQL_CUR_xxx" where xxx is time.
            time_t  t1;
            char    t1Str[21];   // big enough for 64-bits (i.e. 20 digits) + NULL
            int     t1StrLen;
            int     offset1;
            char   *prefix1    = "SQL_CUR_";
            int     prefixLen1 = strlen(prefix1);
            int     chopIndex  = 0;

            // Add prefix
            strncpy(rsName, prefix1, prefixLen1);
            t1 = time(NULL);

            // Add big unique number
            ltoa(t1, t1Str, 10);
            t1StrLen = strlen(t1Str);
            if (t1StrLen > MAX_STMT_NAME_LEN - suffixLen1 - suffixLen2)
                // Too Int32, so figure out how much to shorten it
                chopIndex = t1StrLen - (MAX_STMT_NAME_LEN - suffixLen1 - suffixLen2);

            strncpy(&rsName[prefixLen1], &t1Str[chopIndex], (t1StrLen - chopIndex));
            prefixLen = prefixLen + (t1StrLen - chopIndex);
        }

        strncpy(&rsName[prefixLen], "RS", 2);
        prefixLen = prefixLen + 2;
        memset(&rsName[prefixLen], '\0', 4); // make sure null on end

        while (done == FALSE)
        {
            // Make RS name
            itoa(rsNum, &rsName[prefixLen], 10);

            // Get SRVR_STMT_HDL
            rsSrvrStmt = createSrvrStmt(
                    pSrvrStmt->dialogueId,
                    rsName,
                    (long *)&retcode,
                    NULL,
                    SQLCLI_ODBC_MODULE_VERSION,
                    0,
                    TYPE_UNKNOWN,
                    FALSE,
                    SQL_SP_RESULT_SET,
                    TRUE,
                    0,
                    rsNum,
                    &pSrvrStmt->stmt);

            if (rsSrvrStmt == NULL)
            {
                if (retcode != -8916 /* RS_INDEX_OUT_OF_RANGE */)
                    HANDLE_ERROR(retcode, sqlWarning);

                rsNum = rsNum - 1;
                previousRsSrvrStmt->nextSpjRs = NULL;
                // Either done or warning, so stop processing result sets.
                done = TRUE;
            }
            else
            {
                rsSrvrStmt->isClosed = FALSE;

                // Build the output descriptor for the RS
                retcode = PREPARE_R(rsSrvrStmt);

                if(retcode >= 0)
                {
                    rsSrvrStmt->estRowLength = rsSrvrStmt->outputDescVarBufferLen;
                    retcode = WSQL_EXEC_Exec(&rsSrvrStmt->stmt, &rsSrvrStmt->inputDesc, 0);
                    HANDLE_ERROR(retcode, sqlWarning);
                    previousRsSrvrStmt->nextSpjRs = rsSrvrStmt;
                    rsSrvrStmt->previousSpjRs = previousRsSrvrStmt;
                    previousRsSrvrStmt = rsSrvrStmt;
                    rsSrvrStmt->callStmtHandle = pSrvrStmt;
                    rsNum = rsNum + 1;
                }
                else if (retcode == -8915)
                {
                    // An SPJ can be declared to return say for example 3 Result Sets, but actually
                    // return only 2 Result Sets. In this case, SQL_EXEC_AllocStmtForRS would not
                    // return -8916 when we call it for the 3rd result set. However when we try
                    // to describe it we would get a -8915. This should also be treated as a signal
                    // that there are no more result sets. In this case we should free this statement
                    rsSrvrStmt->freeResourceOpt = SQL_DROP;

                    rc = FREESTATEMENT(rsSrvrStmt);

                    rsNum = rsNum - 1;
                    done = TRUE;
                }
                else
                    HANDLE_ERROR(retcode, sqlWarning);
            }
        }  // end while done == FALSE
        pSrvrStmt->numResultSets = rsNum;
    }  // end if pSrvrStmt->sqlQueryType == SQL_CALL_WITH_RESULT_SETS

    if (sqlWarning)
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
    else
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}  // end EXECUTE2

bool fatalSQLError(Int32 sqlcode)
{
    /*
       Nonfatal SQL errors:

       -8101	This operation is prevented by a check constraint <check constraint name > on table <table name>.
       -8102	The operation is prevented by a unique constraint. (Note that this message applies to the primary key only)
       -8105	The operation is prevented by the check option on view <view name>.
       -8108	Surrogate Key Error.
       -8402	A string overflow occurred during the evaluation of a character expression.
       -8403	The length argument of function SUBSTRING cannot be less than zero or greater than source string length.
       -8404	The trim character argument of function TRIM must be one character in length.
       -8405	The operand of function CONVERTTIMESTAMP is out of range.
       -8409	The escape character argument of a LIKE predicate must be one character in length.
       -8410	An escape character in a LIKE pattern must be followed by another escape character, an underscore, or a percent character.
       -8411	A numeric overflow occurred during an arithmetic computation or data conversion
       -8412	 An input character host variable is missing its null terminator.
       -8413	The string argument contains characters that cannot be converted.
       -8415	The provided DATE, TIME, or TIMESTAMP is not valid and cannot be converted.
       -8416	A datetime expression evaluated to an invalid datetime value.
       -8419	An arithmetic expression attempted a division by zero.
       -8421	NULL cannot be assigned to a NOT NULL column
       -8422	The provided INTERVAL is not valid and cannot be converted.
       -8429	The preceding error actually occurred in function <function-name>
       -8432	A negative value cannot be converted to an unsigned numeric datatype.
       -8690	An invalid character value encountered in TRANSLATE function.
       */
    Int32 static Hsqlcode = -8101;
    Int32 static Lsqlcode = -8690;
    Int32 static Tsqlcode[] = {-8101,-8102,-8105,-8108,-8402,-8403,-8404,-8405,-8409,-8410,-8411,-8412,
        -8413,-8415,-8416,-8419,-8421,-8422,-8429,-8432,-8690};
    if (sqlcode > 0)
        return false;
    if (sqlcode > Hsqlcode || sqlcode < Lsqlcode)
        return true;
    for(int i=0; Tsqlcode[i] >= Lsqlcode; i++)
        if (sqlcode == Tsqlcode[i])
            return false;
    return true;
}

//--------------------------------------------------------------------------
SQLRETURN BuildSQLDesc2withRowsets(
        SQLDESC_ID          *pDesc
        , Int32                 sqlQueryType
        , Int32                 maxRowsetSize
        , bool                &sqlBulkFetchPossible
        , Int32                 numEntries   // #params + 1 for first quad ptr
        , BYTE               *&SQLDesc
        , Int32                &SQLDescLen
        , BYTE               *&varBuffer
        , Int32               &totalMemLen
        , SRVR_DESC_HDL      *&implDesc
        , SQLCLI_QUAD_FIELDS *&inputQuadList
        , SQLCLI_QUAD_FIELDS *&inputQuadList_recover
        )
{

    Int32 FSDataType;   // added this to get NSK file system datatype
    //       to find numeric signed or unsigned.
    Int32 VarAlign;
    Int32 IndAlign;
    Int32 Version = 0;
    Int32 DataType;
    Int32 DateTimeCode;
    Int32 Length;
    Int32 Precision;
    Int32 Scale;
    Int32 Nullable;
    BOOL SignType;
    Int32 ODBCDataType;
    Int32 ODBCPrecision;
    Int32 SQLCharset;
    Int32 ODBCCharset;
    Int32 ColHeadingNmlen;
    Int32 TableNamelen;
    Int32 CatalogNamelen;
    Int32 SchemaNamlen;
    Int32 Headinglen;
    Int32 IntLeadPrec;
    Int32 paramMode;

    Int32 totalDescLength = 0;
    Int32 i, j;
    Int32 tempLen    = 0;
    Int32 retcode    = SQL_SUCCESS;
    BOOL sqlWarning = FALSE;
    char Heading[MAX_ANSI_NAME_LEN];

    Int32  memOffSet = 0;
    BYTE *VarPtr;
    BYTE *IndPtr;
    BYTE *memPtr;

    struct ODBCDescriptors
    {
        Int32 varAlign;
        Int32 indAlign;
        Int32 version;
        Int32 dataType;
        Int32 datetimeCode;
        Int32 maxLen;
        Int32 precision;
        Int32 scale;
        Int32 nullInfo;
        Int32 signType;
        Int32 ODBCDataType;
        Int32 ODBCPrecision;
        Int32 SQLCharset;
        Int32 ODBCCharset;
        Int32 FSDataType;
        Int32 colHeadingNmlen;
        char colHeadingNm[MAX_ANSI_NAME_LEN+1];
        Int32 TableNamelen;
        char TableName[MAX_ANSI_NAME_LEN+1];
        Int32 CatalogNamelen;
        char CatalogName[MAX_ANSI_NAME_LEN+1];
        Int32 SchemaNamlen;
        char SchemaName[MAX_ANSI_NAME_LEN+1];
        Int32 Headinglen;
        char Heading[MAX_ANSI_NAME_LEN+1];
        Int32 intLeadPrec;
        Int32 paramMode;
    };

    typedef struct tagDESC_HDL_LIST
    {
        Int32 DataType;
        Int32 Length;
        Int32 Nullable;
        Int32 VarBuf;
        Int32 IndBuf;
    } DESC_HDL_LIST;

    DESC_HDL_LIST	*SqlDescInfo;
    Int32		 tempDescLen = 0;

    MEMORY_DELETE_ARRAY(implDesc);
    MEMORY_DELETE_ARRAY(inputQuadList);
    MEMORY_DELETE_ARRAY(inputQuadList_recover);


    if (numEntries > 1)
    {
        MEMORY_ALLOC_ARRAY(implDesc, SRVR_DESC_HDL, numEntries - 1);
        if (implDesc == NULL)
        {
            exit(0);
        }

        MEMORY_ALLOC_ARRAY(inputQuadList, SQLCLI_QUAD_FIELDS, numEntries);
        if (inputQuadList == NULL)
        {
            exit(0);
        }
        
        MEMORY_ALLOC_ARRAY(inputQuadList_recover, SQLCLI_QUAD_FIELDS, numEntries);
        if (inputQuadList_recover == NULL)
        {
            exit(0);
        }

        // Setup ind_layout in quad ptr list.
        for (i = 0; i < numEntries; i++)
            inputQuadList[i].ind_layout = 2;  // 2 byte indicator

        MEMORY_ALLOC_ARRAY(SqlDescInfo, DESC_HDL_LIST, numEntries - 1);
        if (SqlDescInfo == NULL)
        {
            exit(0);
        }

        tempDescLen	= sizeof(totalMemLen) + sizeof(numEntries) + (sizeof(ODBCDescriptors) * (numEntries - 1));
        MEMORY_ALLOC_ARRAY(SQLDesc, BYTE, tempDescLen);
        if (SQLDesc == NULL)
        {
            exit(0);
        }

        *(Int32 *)(SQLDesc+totalDescLength) = 0; // Initialize totalMemLen, Since its calculated later
        totalDescLength += sizeof(totalMemLen);

        *(Int32 *)(SQLDesc+totalDescLength) = numEntries - 1;
        totalDescLength += sizeof(numEntries);

    }

    //if (srvrGlobal->drvrVersion.componentId == JDBC_DRVR_COMPONENT && srvrGlobal->drvrVersion.majorVersion == 3 && srvrGlobal->drvrVersion.minorVersion <= 1)
    //	sqlBulkFetchPossible = false;
    //else
    sqlBulkFetchPossible = true;

    for (i = 2, totalMemLen = 0; i <= numEntries; i++)
    {
        // Initialize the desc entry in SQLDESC_ITEM struct
        for (j = 0; j < NO_OF_DESC_ITEMS ; j++)
        {
            gDescItems[j].entry = i;
        }
        gDescItems[10].num_val_or_len = MAX_ANSI_NAME_LEN;
        gDescItems[11].num_val_or_len = MAX_ANSI_NAME_LEN;
        gDescItems[12].num_val_or_len = MAX_ANSI_NAME_LEN;
        gDescItems[13].num_val_or_len = MAX_ANSI_NAME_LEN;
        gDescItems[14].num_val_or_len = MAX_ANSI_NAME_LEN;

        retcode = WSQL_EXEC_GetDescItems2(pDesc,
                NO_OF_DESC_ITEMS,
                (SQLDESC_ITEM *)&gDescItems);
        HANDLE_ERROR(retcode, sqlWarning);

        // This change is made for ADO since it can't handle 'COLUMN HEADING' since
        // driver returns this as SQL_DESC_LABEL, So change it to 'COLUMN NAME'.
        // Temperaryly use Heading to send NULL and in future replace ColumnHeading
        // with Heading.
        Heading[0] = '\0';

        DataType     = gDescItems[0].num_val_or_len;
        Length       = gDescItems[1].num_val_or_len;
        Precision    = gDescItems[2].num_val_or_len;
        Scale        = gDescItems[3].num_val_or_len;
        Nullable     = gDescItems[4].num_val_or_len;
        paramMode    = gDescItems[5].num_val_or_len;
        IntLeadPrec  = gDescItems[6].num_val_or_len;
        DateTimeCode = gDescItems[7].num_val_or_len;
        SQLCharset   = gDescItems[8].num_val_or_len;
        FSDataType   = gDescItems[9].num_val_or_len;
        CatalogNm[gDescItems[10].num_val_or_len]   = '\0';
        SchemaNm[gDescItems[11].num_val_or_len]    = '\0';
        TableNm[gDescItems[12].num_val_or_len]     = '\0';
        ColumnName[gDescItems[13].num_val_or_len]      = '\0';
        ColumnHeading[gDescItems[14].num_val_or_len] = '\0';

        // Added this check since SQL started returning SQLTYPECODE_NUMERIC.
        // Instead of changing on driver side Datatype is changed here to work correctly.
        if (DataType == SQLTYPECODE_NUMERIC || DataType == SQLTYPECODE_NUMERIC_UNSIGNED)
        {
            switch (FSDataType)
            {
                case 130:
                    DataType = SQLTYPECODE_SMALLINT;
                    break;
                case 131:
                    DataType = SQLTYPECODE_SMALLINT_UNSIGNED;
                    break;
                case 132:
                    DataType = SQLTYPECODE_INTEGER;
                    break;
                case 133:
                    DataType = SQLTYPECODE_INTEGER_UNSIGNED;
                    break;
                case 134:
                    DataType = SQLTYPECODE_LARGEINT;
                    break;
                default:
                    break;
            }
        }

        if (sqlBulkFetchPossible)
        {
            if (!Nullable)
            {
                switch (DataType)
                {
                    case SQLTYPECODE_VARCHAR:
                    case SQLTYPECODE_VARCHAR_WITH_LENGTH:
                    case SQLTYPECODE_VARCHAR_LONG:
                    case SQLTYPECODE_DATETIME:
                    case SQLTYPECODE_INTERVAL:
                        sqlBulkFetchPossible = false;
                        break;
                }
            }
            else
                sqlBulkFetchPossible = false;
        }

        retcode = GetODBCValues( DataType
                , DateTimeCode
                , Length
                , Precision
                , ODBCDataType
                , ODBCPrecision
                , SignType, Nullable
                , totalMemLen
                , SQLCharset
                , ODBCCharset
                , IntLeadPrec
                , ColumnHeading);
        HANDLE_ERROR(retcode, sqlWarning);
        if (DataType == SQLTYPECODE_LARGEINT && Precision == 0 && Scale > 0)
            ODBCDataType = SQL_NUMERIC;

        implDesc[i-2].charSet = SQLCharset;
        implDesc[i-2].dataType  = DataType;
        implDesc[i-2].length    = Length;
        implDesc[i-2].paramMode = paramMode;
        implDesc[i-2].FSDataType = FSDataType;

        SqlDescInfo[i-2].DataType = DataType;
        SqlDescInfo[i-2].Length   = Length;

        inputQuadList[i-1].var_layout = Length;

        SqlDescInfo[i-2].Nullable = Nullable;
        SqlDescInfo[i-2].VarBuf   = totalDescLength;
        totalDescLength          += sizeof(VarAlign);
        SqlDescInfo[i-2].IndBuf   = totalDescLength;
        totalDescLength          += sizeof(IndAlign);

        *(Int32 *)(SQLDesc+totalDescLength) = Version;
        totalDescLength += sizeof(Version);

        *(Int32 *)(SQLDesc+totalDescLength) = DataType;
        totalDescLength += sizeof(DataType);

        *(Int32 *)(SQLDesc+totalDescLength) = DateTimeCode;
        totalDescLength += sizeof(DateTimeCode);

        *(Int32 *)(SQLDesc+totalDescLength) = Length;
        totalDescLength += sizeof(Length);

        *(Int32 *)(SQLDesc+totalDescLength) = Precision;
        totalDescLength += sizeof(Precision);

        *(Int32 *)(SQLDesc+totalDescLength) = Scale;
        totalDescLength += sizeof(Scale);

        *(Int32 *)(SQLDesc+totalDescLength) = Nullable;
        totalDescLength += sizeof(Nullable);

        if (SignType)  // This may change if SQL returns sign and since desc will be Int32 return as Int32
            *(Int32 *)(SQLDesc+totalDescLength) = 1;
        else
            *(Int32 *)(SQLDesc+totalDescLength) = 0;
        totalDescLength += sizeof(Int32);

        *(Int32 *)(SQLDesc+totalDescLength) = ODBCDataType;
        totalDescLength += sizeof(ODBCDataType);

        *(Int32 *)(SQLDesc+totalDescLength) = ODBCPrecision;
        totalDescLength += sizeof(ODBCPrecision);

        *(Int32 *)(SQLDesc+totalDescLength) = SQLCharset;
        totalDescLength += sizeof(SQLCharset);

        *(Int32 *)(SQLDesc+totalDescLength) = ODBCCharset;
        totalDescLength += sizeof(ODBCCharset);

        *(Int32 *)(SQLDesc+totalDescLength) = FSDataType;
        totalDescLength += sizeof(FSDataType);

        ColHeadingNmlen = gDescItems[13].num_val_or_len+1; // Null terminator
        *(Int32 *)(SQLDesc+totalDescLength) = ColHeadingNmlen;
        totalDescLength += sizeof(ColHeadingNmlen);
        if (ColHeadingNmlen > 0)
        {
            memcpy(SQLDesc+totalDescLength,ColumnName,ColHeadingNmlen);
            totalDescLength += ColHeadingNmlen;
        }

        TableNamelen = gDescItems[12].num_val_or_len+1;
        *(Int32 *)(SQLDesc+totalDescLength) = TableNamelen;
        totalDescLength += sizeof(TableNamelen);
        if (TableNamelen > 0)
        {
            memcpy(SQLDesc+totalDescLength,TableNm,TableNamelen);
            totalDescLength += TableNamelen;
        }

        CatalogNamelen = gDescItems[10].num_val_or_len+1;
        *(Int32 *)(SQLDesc+totalDescLength) = CatalogNamelen;
        totalDescLength += sizeof(CatalogNamelen);
        if (CatalogNamelen > 0)
        {
            memcpy(SQLDesc+totalDescLength,CatalogNm,CatalogNamelen);
            totalDescLength += CatalogNamelen;
        }

        SchemaNamlen = gDescItems[11].num_val_or_len+1;
        *(Int32 *)(SQLDesc+totalDescLength) = SchemaNamlen;
        totalDescLength += sizeof(SchemaNamlen);
        if (SchemaNamlen > 0)
        {
            memcpy(SQLDesc+totalDescLength,SchemaNm,SchemaNamlen);
            totalDescLength += SchemaNamlen;
        }

        // Headinglen = gDescItems[14].num_val_or_len+1; // Change this when we start supporting strlen(ColumnHeading);
        Headinglen = 1;
        *(Int32 *)(SQLDesc+totalDescLength) = Headinglen;
        totalDescLength += sizeof(Headinglen);
        if (Headinglen > 0)
        {
            //	memcpy(SQLDesc+totalDescLength,ColumnHeading,Headinglen); // Change this when we start supporting strlen(ColumnHeading);
            memcpy(SQLDesc+totalDescLength,Heading,Headinglen);
            totalDescLength += Headinglen;
        }

        *(Int32 *)(SQLDesc+totalDescLength) = IntLeadPrec;
        totalDescLength += sizeof(IntLeadPrec);

        *(Int32 *)(SQLDesc+totalDescLength) = paramMode;
        totalDescLength += sizeof(paramMode);
    }  // end for

    // Adjust totalMemLen to word boundary
    totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3;

    MEMORY_DELETE_ARRAY(varBuffer);
    
    if (sqlBulkFetchPossible && sqlQueryType == SQL_SELECT_NON_UNIQUE)
    {
        MEMORY_ALLOC_ARRAY(varBuffer, BYTE, srvrGlobal->m_FetchBufferSize);
    }
    else
    {
        // KAS This probably needs to be removed if I end up using the input buffer for the
        // quad pointers to point too.
        MEMORY_ALLOC_ARRAY(varBuffer, BYTE, totalMemLen);
    }
    if (varBuffer == NULL)
    {
        // Handle Memory Overflow execption here
        exit(0);
    }

    *(Int32 *)SQLDesc = totalMemLen;

    memPtr = varBuffer ;
    memOffSet = 0;

    for (i = 0 ; i < numEntries - 1 ; i++)
    {
        switch (SqlDescInfo[i].DataType)
        {
            case SQLTYPECODE_CHAR:
            case SQLTYPECODE_VARCHAR:
                VarPtr = memPtr + memOffSet;
                memOffSet += SqlDescInfo[i].Length + 1;
                break;
            case SQLTYPECODE_VARCHAR_WITH_LENGTH:
            case SQLTYPECODE_VARCHAR_LONG:
                if( SqlDescInfo[i].Length > 0x7FFF)
                {
                    memOffSet = ((memOffSet + 4 - 1) >> 2) << 2;
                    VarPtr = memPtr + memOffSet;
                    memOffSet += SqlDescInfo[i].Length + 5;
                }
                else
                {
                    memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
                    VarPtr = memPtr + memOffSet;
                    memOffSet += SqlDescInfo[i].Length + 3;
                }
                // Adjust quad var_layout to even byte boundry
                if (SqlDescInfo[i].Length == ((SqlDescInfo[i].Length >> 1) << 1))
                    inputQuadList[i+1].var_layout = SqlDescInfo[i].Length;
                else
                    inputQuadList[i+1].var_layout = SqlDescInfo[i].Length + 1;
                break;
            case SQLTYPECODE_SMALLINT:
            case SQLTYPECODE_SMALLINT_UNSIGNED:
                memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
                VarPtr = memPtr + memOffSet;
                memOffSet += SqlDescInfo[i].Length;
                break;
            case SQLTYPECODE_INTEGER:
            case SQLTYPECODE_INTEGER_UNSIGNED:
                memOffSet = ((memOffSet + 4 - 1) >> 2) << 2;
                VarPtr = memPtr + memOffSet;
                memOffSet += SqlDescInfo[i].Length;
                break;
            case SQLTYPECODE_LARGEINT:
            case SQLTYPECODE_IEEE_REAL:
            case SQLTYPECODE_IEEE_FLOAT:
            case SQLTYPECODE_IEEE_DOUBLE:
                memOffSet = ((memOffSet + 8 - 1) >> 3) << 3;
                VarPtr = memPtr + memOffSet;
                memOffSet += SqlDescInfo[i].Length;
                break;
            case SQLTYPECODE_DECIMAL_UNSIGNED:
            case SQLTYPECODE_DECIMAL:
            case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
            case SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
            case SQLTYPECODE_INTERVAL:		// Treating as CHAR
            case SQLTYPECODE_DATETIME:
                VarPtr = memPtr + memOffSet;
                memOffSet += SqlDescInfo[i].Length;
                break;
            default:
                memOffSet = ((memOffSet + 8 - 1) >> 3) << 3;
                VarPtr = memPtr + memOffSet;
                memOffSet += SqlDescInfo[i].Length;
                break;
        }
        *(Int32 *)(SQLDesc+SqlDescInfo[i].VarBuf) = (Int32)(VarPtr-memPtr);

        if (SqlDescInfo[i].Nullable)
        {
            memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
            IndPtr = memPtr + memOffSet;
            *(Int32 *)(SQLDesc+SqlDescInfo[i].IndBuf) = (Int32)(IndPtr-memPtr);
            memOffSet += 2 ;
        }
        else
        {
            IndPtr = NULL;
            *(Int32 *)(SQLDesc+SqlDescInfo[i].IndBuf) = (Int32)(-1);
        }

        implDesc[i].varPtr = VarPtr;
        implDesc[i].indPtr = IndPtr;

        if (memOffSet > totalMemLen)
        {
            CLI_DEBUG_RETURN_SQL(PROGRAM_ERROR);
        }
    }

    SQLDescLen = totalDescLength;

    if (SqlDescInfo != NULL)
    {
        MEMORY_DELETE_ARRAY(SqlDescInfo);
        SqlDescInfo = NULL;
    }

    if (sqlBulkFetchPossible && sqlQueryType == SQL_SELECT_NON_UNIQUE)
    {
        Int32 TmpMaxRows = (srvrGlobal->m_FetchBufferSize)/totalMemLen;
        retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWSET_TYPE, 2, 0);
        HANDLE_ERROR(retcode, sqlWarning);

        retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWWISE_ROWSET_PTR, (long)varBuffer, 0);
        HANDLE_ERROR(retcode, sqlWarning);

        retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWWISE_ROWSET_SIZE, TmpMaxRows, 0);
        HANDLE_ERROR(retcode, sqlWarning);

        retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWWISE_ROWSET_ROW_LEN, totalMemLen, 0);
        HANDLE_ERROR(retcode, sqlWarning);
    }

    if (sqlWarning)
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
    else
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);

}  // BuildSQLDesc2withRowsets

// regular execute
SQLRETURN EXECUTE_R(SRVR_STMT_HDL* pSrvrStmt)
{
    Int32 inputRowCnt = pSrvrStmt->inputRowCnt;
    IDL_short sqlStmtType = pSrvrStmt->sqlStmtType;
    const SQLValueList_def *inputValueList = &pSrvrStmt->inputValueList;
    Int64 rowsAffected_cli;
    Int32 *rowsAffected = &pSrvrStmt->rowsAffected;
    Int32 *rowsAffectedHigherBytes = &pSrvrStmt->rowsAffectedHigherBytes;

    Int32 retcode = SQL_SUCCESS;
    Int32 execretcode = SQL_SUCCESS;

    SQLDESC_ID *pDesc;
    SQLSTMT_ID *pStmt;
    SQLSTMT_ID cursorId;
    Int32		paramCount;
    char		*cursorName;
    Int32		len;
    Int32		curRowCnt;
    Int32		curParamNo;
    Int32		curLength;
    BYTE		*varPtr;
    BYTE		*indPtr;
    void			*pBytes;
    SQLValue_def	*SQLValue;
    BOOL			sqlWarning = FALSE;
    SRVR_DESC_HDL	*IPD;
    Int64	t_rowsAffected = 0;

    pSrvrStmt->PerfFetchRetcode = SQL_SUCCESS;
    pSrvrStmt->RowsetFetchRetcode = SQL_SUCCESS;

    // Initialize this to TRUE later in this function depending on sqlStmtType if SQL_SELECT
    // then and retcode is SUCCESS or SUCCESS_WITH_INFO set it to FALSE. Since SQL opens CURSOR
    // after execute in case of SUCCESS or SUCCESS_WITH_INFO and closes only when it reaches
    // NO_DATA_FOUND or CLOSE STATEMENT or COMMIT WORK or ROLLBACK WORK.
    pSrvrStmt->isClosed = TRUE;

    pStmt = &pSrvrStmt->stmt;
    if (pSrvrStmt->stmtType == EXTERNAL_STMT && sqlStmtType == TYPE_SELECT &&
            pSrvrStmt->moduleId.module_name == NULL)
    {
        cursorName = pSrvrStmt->cursorName;
        // If cursor name is not specified, use the stmt name as cursor name
        if (*cursorName == '\0')
            cursorName = pSrvrStmt->stmtName;
        // If cursorName has chg'd from last EXEC or EXECDIRECT cmd
        // or has not yet been set for the first time call SetCursorName
        if ((strcmp(pSrvrStmt->previousCursorName, cursorName) != 0) && *cursorName != '\0')
        {
            cursorId.version        = SQLCLI_ODBC_VERSION;
            cursorId.module         = pStmt->module;
            cursorId.handle         = 0;
            cursorId.charset        = SQLCHARSETSTRING_UTF8;
            cursorId.name_mode      = cursor_name;
            if (*cursorName == '\0')
                cursorId.identifier_len = pSrvrStmt->stmtNameLen;
            else
                cursorId.identifier_len = pSrvrStmt->cursorNameLen;
            cursorId.identifier     = cursorName;
            strcpy(pSrvrStmt->previousCursorName, cursorName);	// keep track of last cursorName
            retcode = WSQL_EXEC_SetCursorName(pStmt, &cursorId);
            HANDLE_ERROR(retcode, sqlWarning);
        }
    }
    pDesc = &pSrvrStmt->inputDesc;
    paramCount = pSrvrStmt->paramCount;

    if (paramCount == 0) // Ignore inputValueList
    {
        if ( pSrvrStmt->stmtType == EXTERNAL_STMT )
        {
            if ( sqlStmtType == TYPE_SELECT  && (pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE || pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE) )
                retcode = WSQL_EXEC_Exec(pStmt, NULL, 0);
            else
                retcode = execretcode = WSQL_EXEC_ExecFetch(pStmt, NULL, 0);
        }
        else
        {
            if ( sqlStmtType == TYPE_SELECT )
                retcode = WSQL_EXEC_Exec(pStmt, NULL, 0);
            else
                retcode = execretcode = WSQL_EXEC_ExecFetch(pStmt, NULL, 0);
        }
        HANDLE_ERROR(retcode, sqlWarning);
        if (WSQL_EXEC_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &rowsAffected_cli, NULL, 0, NULL) < SQL_SUCCESS)
            t_rowsAffected = -1;
        else
            t_rowsAffected += rowsAffected_cli;
    }
    else
    {
        // Else copy from inputValueList to SQL area
        if ((inputRowCnt * paramCount) > (Int32)inputValueList->_length)
            CLI_DEBUG_RETURN_SQL(SQL_NEED_DATA);
        SQLValue_def *p_buffer = (SQLValue_def *)inputValueList->_buffer;
        SRVR_DESC_HDL *p_IPD = pSrvrStmt->IPD;

        void* v_buffer;
        int v_length;
        int v_dataInd;

        for (curRowCnt = 1, curLength = 0; curRowCnt <= inputRowCnt ; curRowCnt++)
        {
            for (curParamNo = 0 ; curParamNo < paramCount ; curParamNo++, curLength++)
            {
                SQLValue = p_buffer + curLength;
                v_buffer = SQLValue->dataValue._buffer;
                v_length = SQLValue->dataValue._length;
                v_dataInd = SQLValue->dataInd;

                IPD = p_IPD+curParamNo;
                indPtr = IPD->indPtr;
                varPtr = IPD->varPtr;

                if (indPtr != NULL)
                    *((short *)indPtr) = v_dataInd == -1? -1: 0;
                else
                    if (v_dataInd == -1)
                        CLI_DEBUG_RETURN_SQL(ODBC_SERVER_ERROR);

                if (v_dataInd != -1)
                    memcpy(varPtr, v_buffer, v_length);
            }
            if ( pSrvrStmt->stmtType == EXTERNAL_STMT )
            {
                if ( sqlStmtType == TYPE_SELECT
                        && (pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE
                            || pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE) )
                    retcode = WSQL_EXEC_Exec(pStmt, pDesc, 0);
                else
                    retcode = execretcode = WSQL_EXEC_ExecFetch(pStmt, pDesc, 0);
            }
            else
            {
                if ( sqlStmtType == TYPE_SELECT )
                    retcode = WSQL_EXEC_Exec(pStmt, pDesc, 0);
                else
                    retcode = execretcode = WSQL_EXEC_ExecFetch(pStmt, pDesc, 0);
            }
            // At present, we return error for this error condition
            // We could think of skipping this row and contine with the next row
            // at a later time
            HANDLE_ERROR(retcode, sqlWarning);
            if (WSQL_EXEC_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &rowsAffected_cli, NULL, 0, NULL) < SQL_SUCCESS)
                t_rowsAffected = -1;
            else
                t_rowsAffected += rowsAffected_cli;
        }
    }
    *rowsAffected = (Int32)(t_rowsAffected << 32 >> 32);
    *rowsAffectedHigherBytes = (Int32)(t_rowsAffected>>32);

    // Initialize this to TRUE later in this function depending on sqlStmtType if SQL_SELECT
    // then and retcode is SUCCESS or SUCCESS_WITH_INFO set it to FALSE. Since SQL opens CURSOR
    // after execute in case of SUCCESS or SUCCESS_WITH_INFO and closes only when it reaches
    // NO_DATA_FOUND or CLOSE STATEMENT or COMMIT WORK or ROLLBACK WORK.
    if ( pSrvrStmt->stmtType == EXTERNAL_STMT )
    {
        if ( sqlStmtType == TYPE_SELECT && (pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE || pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE) )
        {
            pSrvrStmt->isClosed = FALSE;
            pSrvrStmt->bFetchStarted = FALSE;
        }
    }
    else
    {
        if (sqlStmtType == TYPE_SELECT )
        {
            pSrvrStmt->isClosed = FALSE;
            pSrvrStmt->bFetchStarted = FALSE;
        }
    }

    // Added for fix to SQL returning sqlcode=SQL_NO_DATA_FOUND for non-select
    // stmts when no rows get affected 10/03/06
    if (sqlWarning && !(IGNORE_NODATAFOUND(sqlStmtType, execretcode)))
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
    else
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);

} // SRVR::EXECUTE

// Performance Fetch
SQLRETURN FETCHPERF(SRVR_STMT_HDL *pSrvrStmt,
        SQL_DataValue_def *outputDataValue)
{
    SRVRTRACE_ENTER(FILE_INTF+29);

    Int32 maxRowCnt = pSrvrStmt->maxRowCnt;
    Int32 maxRowLen = pSrvrStmt->maxRowLen;
    Int32 *rowsAffected = &pSrvrStmt->rowsAffected;

    Int32 retcode = SQL_SUCCESS;
    Int32 curRowCnt;
    Int32 curColumnNo;
    Int32 varPtr;
    Int64 indPtr;
    void *pBytes;
    Int32 dataType;
    Int32 dataLength;
    Int32 allocLength;
    Int32 srcDataLength;
    short indValue;
    Int32 columnCount;
    Int32 Charset;
    SQLDESC_ID *pDesc;
    BOOL sqlWarning = FALSE;
    SRVR_DESC_HDL *IRD;

    Int32 estLength=0;
    Int32 estRowLength=0;
    Int32 estTotalLength=0;
    Int32 lsize;

    if (pSrvrStmt->PerfFetchRetcode == SQL_NO_DATA_FOUND)
        CLI_DEBUG_RETURN_SQL(SQL_NO_DATA_FOUND);

    pDesc = &pSrvrStmt->outputDesc;
    columnCount = pSrvrStmt->columnCount;
    estTotalLength = pSrvrStmt->estRowLength * maxRowCnt;
    MEMORY_ALLOC_ARRAY(outputDataValue->_buffer, unsigned char, estTotalLength + 1);
    if (outputDataValue->_buffer == NULL)
    {
        /*
        // Handle Memory Overflow execption here
        */
        exit(0);
    }
    memset (outputDataValue->_buffer,0,estTotalLength + 1);

    SQLSTMT_ID*	pStmt = &pSrvrStmt->stmt;
    SRVR_DESC_HDL* IRD_F = pSrvrStmt->IRD;

    for (curRowCnt = 1; curRowCnt <= maxRowCnt ; curRowCnt++)
    {
        retcode = WSQL_EXEC_Fetch(pStmt, pDesc, 0);
        if (retcode != 0)
        {
            if (retcode == 100)
            {
                WSQL_EXEC_CloseStmt(&pSrvrStmt->stmt);
                pSrvrStmt->isClosed = TRUE;
                pSrvrStmt->PerfFetchRetcode = SQL_NO_DATA_FOUND;

                if (curRowCnt == 1)
                {
                    retcode = SQL_NO_DATA_FOUND;
                    goto ret;
                }
                else
                {
                    *rowsAffected = curRowCnt-1;
                    retcode = SQL_SUCCESS;
                    goto ret;
                }
            }
            else
                if (retcode < 0)
                {
                    retcode = SQL_ERROR;
                    goto ret;
                }
                else
                    sqlWarning = TRUE;
        }
        for (curColumnNo = 0; curColumnNo < columnCount ; curColumnNo++)
        {
            SRVR_DESC_HDL* IRD = IRD_F + curColumnNo;
            indPtr = (intptr_t)IRD->indPtr;
            dataType = IRD->dataType;
            dataLength = IRD->length;
            Charset = IRD->charSet;
            pBytes = (void *)(IRD->varPtr);

            if ((indPtr == NULL) || (indPtr != NULL && *((short *)indPtr) != -1))
            {
                indValue = 0;
                switch(dataType)
                {
                    case SQLTYPECODE_VARCHAR_WITH_LENGTH:
                    case SQLTYPECODE_VARCHAR_LONG:
                    case SQLTYPECODE_BITVAR:
                        dataLength = *(USHORT *)pBytes;
                        allocLength = dataLength+3;
                        if (maxRowLen != 0)
                        {
                            allocLength = (allocLength>(UInt32)maxRowLen+3)?(UInt32)maxRowLen+3:allocLength;
                            srcDataLength = *(USHORT *)pBytes;
                            srcDataLength = (srcDataLength>(UInt32)maxRowLen)?(UInt32)maxRowLen:srcDataLength;
                            *(USHORT *)pBytes=srcDataLength;
                        }
                        break;
                    case SQLTYPECODE_BIT:
                        allocLength = dataLength;
                        if (maxRowLen != 0)
                            allocLength = (allocLength>(UInt32)maxRowLen+1)?(UInt32)maxRowLen+1:allocLength;
                        break;
                    case SQLTYPECODE_CHAR:
                    case SQLTYPECODE_VARCHAR:
                        allocLength = dataLength+1;
                        if (maxRowLen != 0)
                            allocLength = (allocLength>(UInt32)maxRowLen+1)?(UInt32)maxRowLen+1:allocLength;
                        break;
                    default:
                        allocLength = dataLength;
                }

                lsize = outputDataValue->_length;
                outputDataValue->_length += (allocLength+1);
                *(outputDataValue->_buffer+lsize) = '\0';		// first byte must be zero
                memcpy(outputDataValue->_buffer+lsize+1, pBytes, allocLength);
                //
                // for varchar/bitvar string is NULL terminated. NULL does not come from SQL (ZO)
                //
                switch(dataType)
                {
                    case SQLTYPECODE_VARCHAR_WITH_LENGTH:
                    case SQLTYPECODE_VARCHAR_LONG:
                    case SQLTYPECODE_BITVAR:
                        *(outputDataValue->_buffer+lsize + 1 + allocLength - 1) = 0;
                        break;
                }
            }
            else
            {
                indValue = -1;
                lsize = outputDataValue->_length;
                outputDataValue->_length += 1;
                *(outputDataValue->_buffer+lsize) = '\1';
            }

        }
    }
    *rowsAffected = curRowCnt >= maxRowCnt ? maxRowCnt : curRowCnt-1;
    if (sqlWarning)
        retcode = SQL_SUCCESS_WITH_INFO;
    else
        retcode = SQL_SUCCESS;
ret:
    if (outputDataValue->_length == 0)
        outputDataValue->_buffer = NULL;
    SRVRTRACE_EXIT(FILE_INTF+29);

    CLI_DEBUG_RETURN_SQL(retcode);
}  // end FETCHPERF

//================================================================================

//-----------------------------------------------------------------------------------------

SQLRETURN FETCH2bulk(SRVR_STMT_HDL *pSrvrStmt)
{
    SRVRTRACE_ENTER(FILE_INTF+29);

    Int32 retcode = SQL_SUCCESS;
    Int32 fetchretcode = SQL_SUCCESS; //2103
    SQLDESC_ID* pDesc = &pSrvrStmt->outputDesc;
    BOOL sqlWarning = FALSE;
    Int32 RowsFetched = 0;

    bool bRWRS = false;
    if (srvrGlobal->drvrVersion.buildId & ROWWISE_ROWSET)
        bRWRS = true;


    //Changes due to cursor issue
    if (pSrvrStmt->maxRowCnt > 0)
    {
        if (pSrvrStmt->outputDescVarBufferLen > 0)
        {
            if( srvrGlobal->m_FetchBufferSize/pSrvrStmt->outputDescVarBufferLen  < pSrvrStmt->maxRowCnt )
            {
                MEMORY_DELETE_ARRAY(pSrvrStmt->outputDescVarBuffer);
                pSrvrStmt->outputDescVarBuffer = NULL;
                MEMORY_ALLOC_ARRAY(pSrvrStmt->outputDescVarBuffer, BYTE, pSrvrStmt->maxRowCnt*pSrvrStmt->outputDescVarBufferLen);
                if (pSrvrStmt->outputDescVarBuffer == NULL)
                {
                    // Handle Memory Overflow exception here
                    exit(0);
                }

                SetIndandVarPtr(&pSrvrStmt->outputDesc
                        , bRWRS
                        , pSrvrStmt->columnCount
                        , pSrvrStmt->outputDescBuffer
                        , pSrvrStmt->outputDescVarBuffer
                        , pSrvrStmt->outputDescVarBufferLen
                        , pSrvrStmt->IRD
                        , pSrvrStmt->SqlDescInfo);


                retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWWISE_ROWSET_PTR, (long)pSrvrStmt->outputDescVarBuffer, 0);
                HANDLE_ERROR(retcode, sqlWarning);
            }
        }
        retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWWISE_ROWSET_SIZE, pSrvrStmt->maxRowCnt, 0);
        HANDLE_ERROR(retcode, sqlWarning);
        //Changes due to cursor issue
    }
    else
        pSrvrStmt->maxRowCnt = srvrGlobal->m_FetchBufferSize/pSrvrStmt->outputDescVarBufferLen;
    // temp fix which is 1 mb. Make sure you change it in BuildSQLDesc2 & fetch2 also.

    Int32 maxRowCnt = pSrvrStmt->maxRowCnt;
    Int32 maxRowLen = pSrvrStmt->maxRowLen;
    Int32 *rowsAffected = &pSrvrStmt->rowsAffected;

    Int32 curRowCnt;

    if (pSrvrStmt->PerfFetchRetcode == SQL_NO_DATA_FOUND)
        CLI_DEBUG_RETURN_SQL(SQL_NO_DATA_FOUND);

    SQLSTMT_ID*	pStmt = &pSrvrStmt->stmt;

    if (pSrvrStmt->bFirstSqlBulkFetch)
    {
        pSrvrStmt->bFirstSqlBulkFetch = false;
    }

    retcode = WSQL_EXEC_Fetch(pStmt, pDesc, 0);
    if (retcode < SQL_SUCCESS)
    {
        fetchretcode = retcode; //2103
        retcode = SQL_ERROR;
        goto ret;
    }
    //else if (retcode > SQL_SUCCESS && retcode != 100)
    //	retcode = SQL_SUCCESS;

    WSQL_EXEC_GetDescItem(pDesc,1,SQLDESC_ROWSET_NUM_PROCESSED,&RowsFetched,0,0,0,0);

ret:
    //2103
    if (fetchretcode == -8007)
        pSrvrStmt->isClosed = TRUE;

    *rowsAffected = RowsFetched;

    if (retcode == 100)
    {
        WSQL_EXEC_CloseStmt(&pSrvrStmt->stmt);
        pSrvrStmt->isClosed = TRUE;
        pSrvrStmt->PerfFetchRetcode = SQL_NO_DATA_FOUND;

        if(pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
        {
            //
            // If all the result sets have been fetched to end, the CALL
            // stmt has to be closed, to allow SQL to reclaim resources
            //
            int numClosedResultSets = 0;
            SRVR_STMT_HDL *callStmt = pSrvrStmt->callStmtHandle;
            SRVR_STMT_HDL *rsStmt = callStmt->nextSpjRs;
            while(rsStmt != NULL)
            {
                if(rsStmt->isClosed == TRUE)
                    numClosedResultSets++;
                rsStmt = rsStmt->nextSpjRs;
            }
            if(numClosedResultSets == callStmt->numResultSets)
            {
                WSQL_EXEC_CloseStmt(&callStmt->stmt);
                callStmt->isClosed = TRUE;
                callStmt->PerfFetchRetcode = SQL_NO_DATA_FOUND;
            }
        } // if(pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
    } // if retcode == 100

    if (retcode == SQL_SUCCESS)
    {
        // Should not happen, but just in case
        if (RowsFetched == 0)
            retcode = SQL_NO_DATA_FOUND;
    }
    else if (retcode == 100)
    {
        if (RowsFetched == 0)
            retcode = SQL_NO_DATA_FOUND;
        else
        {
            pSrvrStmt->PerfFetchRetcode = SQL_NO_DATA_FOUND;
            retcode = SQL_SUCCESS;
        }
    }
    else if(retcode > 0)  // AQR warning case, SQL_NO_DATA_FOUND is already handled in the if-case above
    {
        //
        // Warning - possibly AQR or stream access
        //
        //if (RowsFetched == 0)
        //	retcode = SQL_NO_DATA_FOUND;
        //else
        retcode = SQL_SUCCESS_WITH_INFO;

        Int32 total_conds = 0;
        Int32 curr_cond = 1;
        Int32 sqlcode  = 0;

        WSQL_EXEC_GetDiagnosticsStmtInfo2(NULL, SQLDIAG_NUMBER, (Int64*)&total_conds, NULL, 0, NULL); //total_conds actually 32bit, and SQLCli return 32bit

        while(curr_cond <= total_conds)
        {
            WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_SQLCODE, curr_cond,	&sqlcode, NULL, 0, NULL);
            if(sqlcode == 100)
            {
                WSQL_EXEC_CloseStmt(&pSrvrStmt->stmt);
                pSrvrStmt->isClosed = TRUE;
                pSrvrStmt->PerfFetchRetcode = SQL_NO_DATA_FOUND;

                if(pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
                {
                    //
                    // If all the result sets have been fetched to end, the CALL
                    // stmt has to be closed, to allow SQL to reclaim resources
                    //
                    int numClosedResultSets = 0;
                    SRVR_STMT_HDL *callStmt = pSrvrStmt->callStmtHandle;
                    SRVR_STMT_HDL *rsStmt = callStmt->nextSpjRs;
                    while(rsStmt != NULL)
                    {
                        if(rsStmt->isClosed == TRUE)
                            numClosedResultSets++;
                        rsStmt = rsStmt->nextSpjRs;
                    }
                    if(numClosedResultSets == callStmt->numResultSets)
                    {
                        WSQL_EXEC_CloseStmt(&callStmt->stmt);
                        callStmt->isClosed = TRUE;
                        callStmt->PerfFetchRetcode = SQL_NO_DATA_FOUND;
                    }
                } // if(pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)

            } // if sqlcode == 100
            curr_cond++;
        }
    }

    SRVRTRACE_EXIT(FILE_INTF+29);

    CLI_DEBUG_RETURN_SQL(retcode);
}  // end FETCH2bulk

