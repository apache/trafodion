/**********************************************************************
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
#include "sqltocconv.h"
#include <errno.h>
#include <stdio.h>
#ifdef unixcli
#include <float.h>
#include "unix_extra.h"
#endif
#include <limits.h>
#include <time.h>
#include "sqlcli.h"
#include "DrvrSrvr.h"
#include "mxomsg.h"
#include "drvrglobal.h"
#include "nskieee.h"
#include "diagfunctions.h"
#include "csconvert.h"

#define NUM_LEN_MAX  128 + 2
#define MAXCHARLEN  32767  //32k
#define ENDIAN_PRECISION_MAX 39

extern short convDoItMxcs(char * sourne,
            long sourceLen,
            short sourceType,
            long sourcePrecision,
            long sourceScale,
            char * target,
            long targetLen,
            short targetType,
            long targetPrecision,
            long targetScale,
            long flags,
            BOOL *truncation);


unsigned long ODBC::ConvertSQLToC(SQLINTEGER      ODBCAppVersion,
                            DWORD           DataLangId,
                            CDescRec*       srcDescPtr,
                            SQLPOINTER      srcDataPtr,
                            SQLINTEGER      srcLength,
                            SQLSMALLINT     CDataType,
                            SQLPOINTER      targetDataPtr,
                            SQLINTEGER      targetLength,
                            SQLLEN*         targetStrLenPtr,
                            BOOL            byteSwap,
                            CHAR*&          translatedDataPtr,
#ifdef unixcli
                            ICUConverter*   iconv,
#else
                            DWORD           translateOption,
#endif
                            SQLINTEGER*     totalReturnedLength,
                            UCHAR*          errorMsg,
                            SWORD           errorMsgMax,
                            SQLINTEGER      EnvironmentType,
                            BOOL            ColumnwiseData,
                            CHAR*           replacementChar
                            )
{
    unsigned long       retCode     = SQL_SUCCESS;

    double              dTmp        = 0;
    __int64             tempVal64   = 0;
    char                numTmpBuf[NUM_LEN_MAX] = {0};
    char*               cTmpBuf     = NULL;
    SQLINTEGER          tmpLen      = 0;

    if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable){
        TraceOut(TR_ODBC_DEBUG,"ConvertSQLToC(%d, %d, %d, %d, %#x, %d, %d, %d, %d, %d, %#x, %d, %#x, %d, %d, %#x, %d, %d)",
                ODBCAppVersion,
                srcDescPtr->m_SQLDataType,
                srcDescPtr->m_ODBCDataType,
                srcDescPtr->m_SQLDatetimeCode,
                srcDataPtr,
                srcLength,
                srcDescPtr->m_ODBCPrecision,
                srcDescPtr->m_ODBCScale,
                srcDescPtr->m_DescUnsigned,
                CDataType,
                targetDataPtr,
                targetLength,
                targetStrLenPtr,
                byteSwap,
                totalReturnedLength,
                errorMsg,
                errorMsgMax,
                EnvironmentType
                );
    }
    if (errorMsg != NULL)
        *errorMsg = '\0';
    
    if (srcDataPtr == NULL)
        return IDS_HY_000;

    DataLangId = LANG_NEUTRAL;

    if (CDataType == SQL_C_DEFAULT)
        retCode = getCDefault(srcDescPtr->m_ODBCDataType, ODBCAppVersion, srcDescPtr->m_SQLCharset, CDataType);

    if (retCode != SQL_SUCCESS)
        return retCode;

    if (srcDescPtr->m_SQLDataType == SQLTYPECODE_BOOLEAN)
    {
        retCode = ConvFromSQLBool(srcDataPtr, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
        return retCode;
    }

    switch (CDataType)
    {
    case SQL_C_CHAR:
    case SQL_C_WCHAR:
    case SQL_C_BINARY:
        switch (srcDescPtr->m_ODBCDataType)
        {
        case SQL_CHAR:
        case SQL_WCHAR:
        case SQL_VARCHAR:
        case SQL_WVARCHAR:
        case SQL_LONGVARCHAR:
        case SQL_INTERVAL_MONTH:
        case SQL_INTERVAL_YEAR:
        case SQL_INTERVAL_YEAR_TO_MONTH:
        case SQL_INTERVAL_DAY:
        case SQL_INTERVAL_HOUR:
        case SQL_INTERVAL_MINUTE:
        case SQL_INTERVAL_SECOND:
        case SQL_INTERVAL_DAY_TO_HOUR:
        case SQL_INTERVAL_DAY_TO_MINUTE:
        case SQL_INTERVAL_DAY_TO_SECOND:
        case SQL_INTERVAL_HOUR_TO_MINUTE:
        case SQL_INTERVAL_HOUR_TO_SECOND:
        case SQL_INTERVAL_MINUTE_TO_SECOND:
            retCode = ConvSQLCharToChar(srcDataPtr, srcDescPtr, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr, iconv, translatedDataPtr, totalReturnedLength, errorMsg, replacementChar);
            break;

        case SQL_TINYINT:
        case SQL_SMALLINT:
        case SQL_INTEGER:
        case SQL_BIGINT:
        case SQL_REAL:
        case SQL_DOUBLE:
        case SQL_DECIMAL:
            retCode = ConvSQLNumberToChar(srcDataPtr, srcDescPtr, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
            break;

        case SQL_NUMERIC:
            retCode = ConvSQLNumericToChar(srcDataPtr, srcDescPtr, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
            break;

        case SQL_DATE:
        case SQL_TYPE_DATE:
            if (ColumnwiseData)
                retCode = ConvCopyColumnwiseData(srcDataPtr, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
            else
                retCode = ConvSQLDateToChar(srcDataPtr, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
            break;

        case SQL_TIME:
        case SQL_TYPE_TIME:
            if (ColumnwiseData)
                retCode = ConvCopyColumnwiseData(srcDataPtr, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
            else
            retCode = ConvSQLTimeToChar(srcDataPtr, srcDescPtr->m_ODBCPrecision, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
            break;

        case SQL_TIMESTAMP:
        case SQL_TYPE_TIMESTAMP:
            if (ColumnwiseData)
                retCode = ConvCopyColumnwiseData(srcDataPtr, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
            else
                retCode = ConvSQLTimestampToChar(srcDataPtr, srcDescPtr->m_ODBCPrecision, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
            break;

        default:
            return IDS_07_006;
        }
        break;

    case SQL_C_TINYINT:
    case SQL_C_STINYINT:
    case SQL_C_BIT:
    case SQL_C_UTINYINT:
    case SQL_C_SHORT:
    case SQL_C_SSHORT:
    case SQL_C_USHORT:
    case SQL_C_LONG:
    case SQL_C_SLONG:
    case SQL_C_ULONG:
    case SQL_C_SBIGINT:
    case SQL_C_UBIGINT:
    case SQL_C_FLOAT:
    case SQL_C_DOUBLE:
        switch (srcDescPtr->m_ODBCDataType)
        {
        case SQL_CHAR:
        case SQL_WCHAR:
        case SQL_VARCHAR:
        case SQL_WVARCHAR:
        case SQL_LONGVARCHAR:
            retCode = ConvSQLCharToNumber(srcDataPtr, srcDescPtr, srcLength, CDataType, targetDataPtr, targetStrLenPtr, iconv, errorMsg);
            break;

        case SQL_INTERVAL_MONTH:
        case SQL_INTERVAL_YEAR:
        case SQL_INTERVAL_YEAR_TO_MONTH:
        case SQL_INTERVAL_DAY:
        case SQL_INTERVAL_HOUR:
        case SQL_INTERVAL_MINUTE:
        case SQL_INTERVAL_SECOND:
        case SQL_INTERVAL_DAY_TO_HOUR:
        case SQL_INTERVAL_DAY_TO_MINUTE:
        case SQL_INTERVAL_DAY_TO_SECOND:
        case SQL_INTERVAL_HOUR_TO_MINUTE:
        case SQL_INTERVAL_HOUR_TO_SECOND:
        case SQL_INTERVAL_MINUTE_TO_SECOND:
            retCode = ConvSQLIntevalToDouble(srcDataPtr, dTmp);
            if (retCode != SQL_SUCCESS)
                break;
            retCode = ConvDoubleToCNumber(dTmp, CDataType, targetDataPtr, targetStrLenPtr);
            break;
        case SQL_TINYINT:
        case SQL_SMALLINT:
        case SQL_INTEGER:
        case SQL_REAL:
        case SQL_DOUBLE:
        case SQL_DECIMAL:
            retCode = ConvSQLNumberToDouble(srcDataPtr, srcDescPtr, srcLength, dTmp);
            if (retCode != SQL_SUCCESS)
                break;
            retCode = ConvDoubleToCNumber(dTmp, CDataType, targetDataPtr, targetStrLenPtr);
            break;
        case SQL_BIGINT:
            retCode = ConvSQLBigintToNumber(srcDataPtr, srcDescPtr->m_DescUnsigned, CDataType, 0, targetDataPtr, targetStrLenPtr);
            break;

        case SQL_NUMERIC:
            retCode = ConvSQLNumericToNumber(srcDataPtr, srcDescPtr, srcLength, CDataType, targetDataPtr, targetStrLenPtr);
            break;

        default:
            return IDS_07_006;
        }
        break;

    case SQL_C_DATE:
    case SQL_C_TYPE_DATE:
        switch (srcDescPtr->m_ODBCDataType)
        {
        case SQL_CHAR:
        case SQL_WCHAR:
        case SQL_VARCHAR:
        case SQL_WVARCHAR:
        case SQL_LONGVARCHAR:
            retCode = GetCTmpBufFromSQLChar(srcDataPtr, srcLength, srcDescPtr->m_ODBCDataType, srcDescPtr->m_SQLCharset, srcDescPtr->m_SQLMaxLength <= MAXCHARLEN, cTmpBuf, tmpLen, iconv, errorMsg, false);
            if (retCode != SQL_SUCCESS)
                break;
            retCode = ConvSQLCharToDateTime(cTmpBuf, tmpLen, CDataType, targetDataPtr);
            break;
        case SQL_DATE:
        case SQL_TYPE_DATE:
            retCode = ConvSQLDateToDate(srcDataPtr, srcDescPtr->m_SQLDatetimeCode, targetDataPtr, targetStrLenPtr, ColumnwiseData);
            break;
    
        case SQL_TIMESTAMP:
        case SQL_TYPE_TIMESTAMP:
            retCode = ConvSQLTimestampToDate(srcDataPtr, srcDescPtr->m_SQLDatetimeCode, targetDataPtr, targetStrLenPtr, ColumnwiseData);
            break;
        default:
            return IDS_07_006;
        }
        break;
        
    case SQL_C_TIME:
    case SQL_C_TYPE_TIME:
        switch (srcDescPtr->m_ODBCDataType)
        {
        case SQL_CHAR:
        case SQL_WCHAR:
        case SQL_VARCHAR:
        case SQL_WVARCHAR:
        case SQL_LONGVARCHAR:
            retCode = GetCTmpBufFromSQLChar(srcDataPtr, srcLength, srcDescPtr->m_ODBCDataType, srcDescPtr->m_SQLCharset, srcDescPtr->m_SQLMaxLength <= MAXCHARLEN, cTmpBuf, tmpLen, iconv, errorMsg, false);
            if (retCode != SQL_SUCCESS)
                break;
            retCode = ConvSQLCharToDateTime(cTmpBuf, tmpLen, CDataType, targetDataPtr);
            break;
        case SQL_TIME:
        case SQL_TYPE_TIME:
            retCode = ConvSQLTimeToTime(srcDataPtr, srcDescPtr->m_SQLDatetimeCode, targetDataPtr, targetStrLenPtr, ColumnwiseData);
            break;
    
        case SQL_TIMESTAMP:
        case SQL_TYPE_TIMESTAMP:
            retCode = ConvSQLTimestampToTime(srcDataPtr, srcDescPtr->m_SQLDatetimeCode, targetDataPtr, targetStrLenPtr, ColumnwiseData);
            break;
        default:
            return IDS_07_006;
        }
        break;

    case SQL_C_TIMESTAMP:
    case SQL_C_TYPE_TIMESTAMP:
        switch (srcDescPtr->m_ODBCDataType)
        {
        case SQL_CHAR:
        case SQL_WCHAR:
        case SQL_VARCHAR:
        case SQL_WVARCHAR:
        case SQL_LONGVARCHAR:
            retCode = GetCTmpBufFromSQLChar(srcDataPtr, srcLength, srcDescPtr->m_ODBCDataType, srcDescPtr->m_SQLCharset, srcDescPtr->m_SQLMaxLength <= MAXCHARLEN, cTmpBuf, tmpLen, iconv, errorMsg, false);
            if (retCode != SQL_SUCCESS)
                break;
            retCode = ConvSQLCharToDateTime(cTmpBuf, tmpLen, CDataType, targetDataPtr);
            break;

        case SQL_DATE:
        case SQL_TYPE_DATE:
            retCode = ConvSQLDateToTimestamp(srcDataPtr, srcDescPtr->m_SQLDatetimeCode, targetDataPtr, targetStrLenPtr, ColumnwiseData);
            break;

        case SQL_TIME:
        case SQL_TYPE_TIME:
            retCode = ConvSQLTimeToTimestamp(srcDataPtr, srcDescPtr->m_SQLDatetimeCode, targetDataPtr, targetStrLenPtr, ColumnwiseData);
            break;
    
        case SQL_TIMESTAMP:
        case SQL_TYPE_TIMESTAMP:
            retCode = ConvSQLTimestampToTimestamp(srcDataPtr, srcDescPtr, targetDataPtr, targetStrLenPtr, ColumnwiseData);
            break;
        default:
            return IDS_07_006;
        }
        break;

    case SQL_C_NUMERIC:
        switch (srcDescPtr->m_ODBCDataType)
        {
        case SQL_CHAR:
        case SQL_WCHAR:
        case SQL_VARCHAR:
        case SQL_WVARCHAR:
        case SQL_LONGVARCHAR:
            retCode = ConvSQLCharToNumeric(srcDataPtr, srcDescPtr, srcLength, CDataType, targetDataPtr, targetStrLenPtr, iconv, errorMsg);
            break;

        case SQL_TINYINT:
        case SQL_SMALLINT:
        case SQL_INTEGER:
        case SQL_REAL:
        case SQL_DOUBLE:
        case SQL_DECIMAL:
            retCode = ConvSQLNumberToChar(srcDataPtr, srcDescPtr, srcLength, SQL_C_CHAR, numTmpBuf, NUM_LEN_MAX, NULL);
            retCode = ConvertCharToCNumeric((SQL_NUMERIC_STRUCT *)targetDataPtr, numTmpBuf);
            break;
        case SQL_BIGINT:
            retCode = ConvSQLBigintToNumber(srcDataPtr, srcDescPtr->m_DescUnsigned, 0, CDataType, targetDataPtr, targetStrLenPtr);
            break;

        case SQL_NUMERIC:
            retCode = ConvSQLNumericToNumeric(srcDataPtr, srcDescPtr, srcLength, targetDataPtr, targetStrLenPtr);
            break;
        default:
            return IDS_07_006;
        }
        break;

    case SQL_C_INTERVAL_MONTH:
    case SQL_C_INTERVAL_YEAR:
    case SQL_C_INTERVAL_YEAR_TO_MONTH:
    case SQL_C_INTERVAL_DAY:
    case SQL_C_INTERVAL_HOUR:
    case SQL_C_INTERVAL_MINUTE:
    case SQL_C_INTERVAL_SECOND:
    case SQL_C_INTERVAL_DAY_TO_HOUR:
    case SQL_C_INTERVAL_DAY_TO_MINUTE:
    case SQL_C_INTERVAL_DAY_TO_SECOND:
    case SQL_C_INTERVAL_HOUR_TO_MINUTE:
    case SQL_C_INTERVAL_HOUR_TO_SECOND:
    case SQL_C_INTERVAL_MINUTE_TO_SECOND:
        switch (srcDescPtr->m_ODBCDataType)
        {
        case SQL_CHAR:
        case SQL_WCHAR:
        case SQL_VARCHAR:
        case SQL_WVARCHAR:
        case SQL_LONGVARCHAR:
            retCode = GetCTmpBufFromSQLChar(srcDataPtr, srcLength, srcDescPtr->m_ODBCDataType, srcDescPtr->m_SQLCharset, srcDescPtr->m_SQLMaxLength <= MAXCHARLEN, cTmpBuf, tmpLen, iconv, errorMsg, false);
            if (retCode != SQL_SUCCESS)
                break;
            retCode = ConvertSQLCharToInterval(srcDescPtr->m_ODBCDataType, cTmpBuf, tmpLen, CDataType, targetDataPtr);
            break;
        case SQL_INTERVAL_MONTH:
        case SQL_INTERVAL_YEAR:
        case SQL_INTERVAL_YEAR_TO_MONTH:
        case SQL_INTERVAL_DAY:
        case SQL_INTERVAL_HOUR:
        case SQL_INTERVAL_MINUTE:
        case SQL_INTERVAL_SECOND:
        case SQL_INTERVAL_DAY_TO_HOUR:
        case SQL_INTERVAL_DAY_TO_MINUTE:
        case SQL_INTERVAL_DAY_TO_SECOND:
        case SQL_INTERVAL_HOUR_TO_MINUTE:
        case SQL_INTERVAL_HOUR_TO_SECOND:
        case SQL_INTERVAL_MINUTE_TO_SECOND:
            retCode = ConvertSQLCharToInterval(srcDescPtr->m_ODBCDataType, srcDataPtr, srcLength, CDataType, targetDataPtr);
            break;

        case SQL_TINYINT:
        case SQL_SMALLINT:
        case SQL_INTEGER:
        case SQL_REAL:
        case SQL_DOUBLE:
        case SQL_DECIMAL:
            retCode = ConvSQLNumberToDouble(srcDataPtr, srcDescPtr, srcLength, dTmp);
            if (retCode != SQL_SUCCESS)
                break;
            retCode = ConvDoubleToInterval(dTmp, targetDataPtr, targetLength, CDataType, targetStrLenPtr);
            break;

        case SQL_BIGINT:
            retCode = ConvSQLBigintToNumber(srcDataPtr, srcDescPtr->m_DescUnsigned, SQL_C_DOUBLE, 0, &dTmp, targetStrLenPtr);
            if (retCode != SQL_SUCCESS)
                break;
            retCode = ConvDoubleToInterval(dTmp, targetDataPtr, targetLength, CDataType, targetStrLenPtr);
            break;

        case SQL_NUMERIC:
            retCode = ConvSQLNumericToNumber(srcDataPtr, srcDescPtr, srcLength, SQL_C_DOUBLE, &dTmp, targetStrLenPtr);
            if (retCode != SQL_SUCCESS)
                break;
            retCode = ConvDoubleToInterval(dTmp, targetDataPtr, targetLength, CDataType, targetStrLenPtr);
            break;

        default:
            return IDS_07_006;
        }
        break;

    default:
        return IDS_07_006;
    }

    if (cTmpBuf != NULL)
    {
        free(cTmpBuf);
        cTmpBuf = NULL;
    }

    return retCode;
}


unsigned long ODBC::ConvSQLCharToChar(SQLPOINTER srcDataPtr, CDescRec* srcDescPtr, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN* targetStrLenPtr, ICUConverter* iconv, CHAR*& translatedDataPtr, SQLINTEGER* totalReturnedLength, UCHAR* errorMsg, CHAR* replacementChar)
{
    unsigned long   retCode             = SQL_SUCCESS;
    unsigned int    charlength          = 0;
    unsigned int    DataLen             = 0;
    SQLINTEGER      DataLenTruncated    = 0;
    SQLINTEGER      Offset              = 0;
    SQLINTEGER      translateLengthMax  = 0;
    SQLSMALLINT     ODBCDataType        = srcDescPtr->m_ODBCDataType;
    SQLINTEGER      srcCharSet          = srcDescPtr->m_SQLCharset;
    SQLPOINTER      DataPtr             = srcDataPtr;

    if (totalReturnedLength != NULL)
    {
        Offset = *totalReturnedLength;
        *totalReturnedLength = -1;
    }
    else
        Offset = 0;

    switch (srcDescPtr->m_ODBCDataType)
    {
    case SQL_CHAR:
    case SQL_WCHAR:
        charlength = srcLength - 1;
        if (charlength == 0)
        {
            if (targetStrLenPtr != NULL)
                *targetStrLenPtr = 0;
            if (targetLength > 0)
                ((char*)targetDataPtr)[0] = '\0';
            return retCode;
        }
        DataLen = charlength - Offset;
        if (DataLen == 0)
            return SQL_NO_DATA;
        break;
    case SQL_VARCHAR:
    case SQL_WVARCHAR:
    case SQL_LONGVARCHAR:
        if (srcDescPtr->m_SQLMaxLength <= MAXCHARLEN)
        {
            charlength = *(USHORT *)srcDataPtr;
            DataPtr = (char *)srcDataPtr + 2 + Offset;
        }
        else
        {
            charlength = *(unsigned int *)srcDataPtr;
            DataPtr = (char *)srcDataPtr + 4 + Offset;
        }
        if (charlength == 0)
        {
            if (targetStrLenPtr != NULL)
                *targetStrLenPtr = 0;
            if (targetLength > 0)
                ((char*)targetDataPtr)[0] = '\0';
            return retCode;
        }
        DataLen = charlength - Offset;
        if (DataLen == 0)
            return SQL_NO_DATA;
        break;
    case SQL_INTERVAL_MONTH:
    case SQL_INTERVAL_YEAR:
    case SQL_INTERVAL_YEAR_TO_MONTH:
    case SQL_INTERVAL_DAY:
    case SQL_INTERVAL_HOUR:
    case SQL_INTERVAL_MINUTE:
    case SQL_INTERVAL_SECOND:
    case SQL_INTERVAL_DAY_TO_HOUR:
    case SQL_INTERVAL_DAY_TO_MINUTE:
    case SQL_INTERVAL_DAY_TO_SECOND:
    case SQL_INTERVAL_HOUR_TO_MINUTE:
    case SQL_INTERVAL_HOUR_TO_SECOND:
    case SQL_INTERVAL_MINUTE_TO_SECOND:
        DataLen = srcLength;
        if (DataLen >= targetLength)
        {
            DataLenTruncated = srcLength - Offset;
            if (targetLength > 0)
                DataLen = targetLength - 1;
            else
                DataLen = 0;
            return IDS_01_004;
        }
        break;
    default:
        return SQL_ERROR;
    }

    SQLINTEGER transLen = 0;
    if (translatedDataPtr != NULL && totalReturnedLength != NULL && Offset != 0)
    { 
        // data has already translated
        if (CDataType == SQL_C_WCHAR && iconv->isAppUTF16())
        {
            DataLen = charlength * 2 - Offset;
            if (DataLen > targetLength - 2)
            {
                DataLenTruncated = DataLen;
                DataLen = targetLength - 2;
                if (DataLen % 2 == 1) DataLen--;
                retCode = IDS_01_004;
                if (totalReturnedLength != NULL)
                    *totalReturnedLength = DataLen + Offset;
            }
            else
            {
                DataLenTruncated = 0;
            }
            memcpy(targetDataPtr, translatedDataPtr + Offset, DataLen);
        }
        else
        {
            if(CDataType == SQL_C_CHAR)
            {
                if(srcCharSet == SQLCHARSETCODE_UCS2)
                    DataLen = charlength / 2 - Offset;
                else
                    DataLen = charlength  - Offset;
                if (DataLen > targetLength)
                {
                    DataLenTruncated = DataLen;
                    DataLen = targetLength - 1;
                    if (totalReturnedLength != NULL)
                        *totalReturnedLength = DataLen + Offset;
                    retCode = IDS_01_004;
                }
                else
                {
                    DataLenTruncated = 0;
                }
                memcpy(targetDataPtr, translatedDataPtr + Offset, DataLen);
            }
            else
            {
                DataLen = strlen(translatedDataPtr) - Offset;
                if (DataLen >= targetLength)
                {
                    DataLenTruncated = DataLen;
                    iconv->strcpyUTF8((char *)targetDataPtr, (const char *)translatedDataPtr+Offset, targetLength, DataLen);
                    DataLen = strlen((char *)targetDataPtr);
                    if (totalReturnedLength != NULL)
                        *totalReturnedLength = DataLen + Offset;
                    retCode = IDS_01_004;
                }
                else
                {
                    iconv->strcpyUTF8((char*)targetDataPtr, (const char*)translatedDataPtr + Offset, targetLength, DataLen);
                    DataLenTruncated = 0;
                }
            }
        }
    }
    else if ( CDataType == SQL_C_CHAR )
    {
        if (srcCharSet == SQLCHARSETCODE_UCS2)
        {
            // translate from UTF16 to DrvrLocale
            if ((retCode = iconv->WCharToLocale((UChar *)DataPtr, DataLen/2, (char *)targetDataPtr, targetLength,
                &transLen, (char*)errorMsg, replacementChar)) != SQL_ERROR)
            {
                if (retCode == SQL_SUCCESS_WITH_INFO)
                {    
                    // buffer overflow - need to allocate temporary buffer
                    translateLengthMax = DataLen * 4 + 1;
                    if (translatedDataPtr != NULL)
                        delete[] translatedDataPtr;
                    translatedDataPtr = new char[translateLengthMax];
                    memset((char *)translatedDataPtr, '\0', translateLengthMax);
                    retCode = iconv->WCharToLocale((UChar *)DataPtr, DataLen / 2,(char *)translatedDataPtr, translateLengthMax, &transLen, (char *)errorMsg, replacementChar);
                    if (retCode != SQL_SUCCESS)
                        return IDS_190_DSTODRV_ERROR;
                    DataLenTruncated = transLen;
                    DataLen = targetLength - 1;
                    memcpy(targetDataPtr, translatedDataPtr, DataLen);
                    retCode = IDS_01_004;
                    if (totalReturnedLength != NULL)
                        *totalReturnedLength = DataLen + Offset;    
                }
                else
                {
                    DataLen = transLen;
                    DataLenTruncated = 0;
                }
            }
            else // error
            {
                return IDS_190_DSTODRV_ERROR;                    
            }
        }
        else if (srcCharSet == SQLCHARSETCODE_UTF8)
        {
            if ((retCode = iconv->TranslateUTF8(true, (char*)DataPtr, DataLen, (char*)targetDataPtr, targetLength, 
                                            &transLen, (char*)errorMsg)) != SQL_ERROR)
            {
                if (retCode == SQL_SUCCESS_WITH_INFO)
                {    
                    // buffer overflow - need to allocate temporary buffer
                    translateLengthMax = DataLen+1;
                    if (translatedDataPtr != NULL) delete[] translatedDataPtr;
                    translatedDataPtr = new char[translateLengthMax];
                    memset((char*)translatedDataPtr,'\0',translateLengthMax);
                    retCode = iconv->TranslateUTF8(true, (char*)DataPtr, DataLen,(char*)translatedDataPtr, translateLengthMax, &transLen, (char*)errorMsg);
                    if (retCode != SQL_SUCCESS) return IDS_190_DSTODRV_ERROR;
                    DataLenTruncated = transLen;
                    DataLen = targetLength - 1;
                    memcpy(targetDataPtr, translatedDataPtr, DataLen);
                    retCode = IDS_01_004;
                    if (totalReturnedLength != NULL)
                        *totalReturnedLength = DataLen + Offset;    
                }
                else                                                                            
                {
                    DataLen = transLen;
                    DataLenTruncated = 0;
                }
            }
            else // error
            {
                return IDS_190_DSTODRV_ERROR;                    
            }
        }
        else 
        if ((iconv->isIso88591Translation()) && srcCharSet != SQLCHARSETCODE_UCS2 && 
            ((ODBCDataType ==  SQL_CHAR) || (ODBCDataType == SQL_VARCHAR) || (ODBCDataType == SQL_LONGVARCHAR)))
        {
            // translate from Iso88591 to DrvrLocale
            if((retCode = iconv->TranslateISO88591(true, (char *)DataPtr, DataLen, (char *)targetDataPtr,
                        targetLength, &transLen, (char *)errorMsg)) != SQL_ERROR)
            {
                if (retCode == SQL_SUCCESS_WITH_INFO)
                {
                    // buffer overflow - need to allocate temporary buffer
                    translateLengthMax = DataLen*4+1;
                    if (translatedDataPtr != NULL) delete[] translatedDataPtr;
                    translatedDataPtr = new char[translateLengthMax];
                    if((retCode = iconv->TranslateISO88591(true, (char *)DataPtr, DataLen, (char *)translatedDataPtr,
                            translateLengthMax, &transLen, (char *)errorMsg)) == SQL_ERROR)
                        return IDS_190_DSTODRV_ERROR;
                    DataLenTruncated = transLen;
                    DataLen = targetLength - 1;
                    memcpy(targetDataPtr, translatedDataPtr, DataLen);
                    if (errorMsg) errorMsg[0] = '\0'; // reset errorMsg in case of buffer overflow
                    retCode = IDS_01_004;
                    if (totalReturnedLength != NULL)
                        *totalReturnedLength = DataLen + Offset;
                }
                else
                {
                    DataLen = transLen;
                    DataLenTruncated = 0;
                }
            }
            else
            {
                return IDS_190_DSTODRV_ERROR;
            }
        }
        else
        {
            memcpy(targetDataPtr, DataPtr, DataLen);
            if (totalReturnedLength != NULL && retCode == IDS_01_004)
                *totalReturnedLength = DataLen + Offset;
        }
    } //SQL_C_CHAR end
    else if ( CDataType == SQL_C_WCHAR && srcCharSet != SQLCHARSETCODE_UCS2 )
    {
        if (iconv->isAppUTF16() && srcCharSet != SQLCHARSETCODE_UTF8)
        {
            // translate from iso88591 to UTF16
            if ((retCode = iconv->ISO88591ToWChar((char *)DataPtr, DataLen, (UChar *)targetDataPtr,
                targetLength, &transLen, (char *)errorMsg, MB_ERR_INVALID_CHARS, &translateLengthMax)) != SQL_ERROR)    // ALM CR5228. Added translateLengthMax to get therequired length.
            {
                if (retCode == SQL_SUCCESS_WITH_INFO)
                {
                    // buffer overflow - need to allocate temporary buffer
                      translateLengthMax = DataLen * 4 + 1;     // ALM CR5228. Instead off get the length by function ISO88591ToWchar above.
                    if (translatedDataPtr != NULL) delete[] translatedDataPtr;
                    translatedDataPtr = new char[(translateLengthMax + 1) * 2];

                    if((retCode = iconv->ISO88591ToWChar((char *)DataPtr, DataLen, (UChar *)translatedDataPtr,
                        translateLengthMax + 1, &transLen, (char *)errorMsg, MB_ERR_INVALID_CHARS, NULL)) == SQL_ERROR)
                        return IDS_190_DSTODRV_ERROR;
                    DataLenTruncated = transLen * 2;
                    DataLen = targetLength - 2;
                    if(DataLen % 2 == 1) DataLen--; // in case DataLen is an odd number
                    memcpy(targetDataPtr, translatedDataPtr, DataLen);
                    if (errorMsg) errorMsg[0] = '\0'; // reset errorMsg in case of buffer overflow
                    retCode = IDS_01_004;
                    if (totalReturnedLength != NULL)
                        *totalReturnedLength = DataLen + Offset;
                }
                else
                {
                    DataLen = transLen * 2;
                    DataLenTruncated = 0;
                }
            }
            else
            {
                return IDS_190_DSTODRV_ERROR;
            }    
        }
        else if ( (!iconv->isAppUTF16()) && srcCharSet == SQLCHARSETCODE_UTF8)
        {
            if(targetLength > DataLen)
                memcpy(targetDataPtr, DataPtr, DataLen);
            else
            {
                // buffer overflow - need to allocate temporary buffer
                translateLengthMax = DataLen + 1;
                if (translatedDataPtr != NULL)
                    delete[] translatedDataPtr;
                translatedDataPtr = new char[translateLengthMax];
                memcpy((char *)translatedDataPtr, (const char *)DataPtr, DataLen);
                translatedDataPtr[DataLen] = '\0';
                DataLenTruncated = DataLen;
                iconv->strcpyUTF8((char *)targetDataPtr, (const char*)translatedDataPtr, targetLength, DataLen);
                if (errorMsg) errorMsg[0] = '\0'; // reset errorMsg in case of buffer overflow
                DataLen = strlen((char *)targetDataPtr);
                    retCode = IDS_01_004;
                if (totalReturnedLength != NULL)
                    *totalReturnedLength = strlen((char *)targetDataPtr) + Offset;
            }
        }
        else if ( iconv->isAppUTF16() && srcCharSet == SQLCHARSETCODE_UTF8)
        {
            if((retCode = iconv->UTF8ToWChar((char *)DataPtr, DataLen, (UChar *)targetDataPtr, targetLength, &transLen, (char *)errorMsg, MB_ERR_INVALID_CHARS, &translateLengthMax))!=SQL_ERROR)
            {
                if (retCode == SQL_SUCCESS_WITH_INFO)
                {
                    // buffer overflow - need to allocate temporary buffer
                    if (translatedDataPtr != NULL)
                        delete[] translatedDataPtr;
                    translatedDataPtr = new char[(translateLengthMax+1)*2];

                    if((retCode = iconv->UTF8ToWChar((char *)DataPtr, DataLen, (UChar *)translatedDataPtr,
                                            (translateLengthMax + 1) * 2, &transLen, (char *)errorMsg, MB_ERR_INVALID_CHARS, NULL)) == SQL_ERROR)
                        return IDS_190_DSTODRV_ERROR;
                    DataLenTruncated = transLen * 2;
                    DataLen = targetLength - 2;
                    if(DataLen % 2 == 1) DataLen--;
                    memcpy(targetDataPtr, translatedDataPtr, DataLen);
                    if (errorMsg) errorMsg[0] = '\0'; // reset errorMsg in case of buffer overflow
                    retCode = IDS_01_004;
                    if (totalReturnedLength != NULL)
                        *totalReturnedLength = DataLen + Offset;
                }
                else
                {
                    DataLen = transLen * 2;
                    DataLenTruncated = 0;
                }
            }
            else
            {
                return IDS_190_DSTODRV_ERROR;
            }    
        }
        else
        {
            // translate from iso88591 to UTF8
            if ((retCode = iconv->UTF8ToFromISO88591(false, (char *)DataPtr, DataLen, (char *)targetDataPtr,
                            targetLength, &transLen, (char *)errorMsg)) != SQL_ERROR)     
            {
                if (retCode == SQL_SUCCESS_WITH_INFO)
                {
                    // buffer overflow - need to allocate temporary buffer
                    translateLengthMax = DataLen * 4 + 1;
                    if (translatedDataPtr != NULL)
                        delete[] translatedDataPtr;
                    translatedDataPtr = new char[translateLengthMax];

                    if((retCode = iconv->UTF8ToFromISO88591(false, (char *)DataPtr, DataLen, (char *)translatedDataPtr,
                        translateLengthMax, &transLen, (char *)errorMsg)) == SQL_ERROR)
                        return IDS_190_DSTODRV_ERROR;
                    DataLenTruncated = transLen;
                    DataLen = targetLength-1;
                    memcpy(targetDataPtr, translatedDataPtr, DataLen);
                    if (errorMsg) errorMsg[0] = '\0'; // reset errorMsg in case of buffer overflow
                    retCode = IDS_01_004;
                    if (totalReturnedLength != NULL)
                        *totalReturnedLength = DataLen + Offset;
                }
                else
                {
                    DataLen = transLen;
                    DataLenTruncated = 0;
                }
            }
            else
            {
                return IDS_190_DSTODRV_ERROR;
            }                
        }            
    }
    else if( CDataType == SQL_C_WCHAR && srcCharSet == SQLCHARSETCODE_UCS2 && (!iconv->isAppUTF16()))
    {
    //Special case where the data is WChar but the application type is UTF8
        if ((retCode = iconv->WCharToUTF8((UChar *)DataPtr, DataLen/2, (char *)targetDataPtr,
                        targetLength, &transLen, (char *)errorMsg, MB_ERR_INVALID_CHARS, &translateLengthMax)) != SQL_ERROR)
        {
            if (retCode == SQL_SUCCESS_WITH_INFO)
            {
                // buffer overflow - need to allocate temporary buffer
                translateLengthMax = DataLen * 4 + 1;
                if (translatedDataPtr != NULL)
                    delete[] translatedDataPtr;
                translatedDataPtr = new char[translateLengthMax];

                if((retCode = iconv->WCharToUTF8((UChar *)DataPtr, DataLen/2, (char *)translatedDataPtr,
                            translateLengthMax, &transLen, (char *)errorMsg, MB_ERR_INVALID_CHARS, NULL)) == SQL_ERROR)
                    return IDS_190_DSTODRV_ERROR;
                DataLenTruncated = transLen;
                DataLen = targetLength - 1;
                memcpy(targetDataPtr, translatedDataPtr, DataLen);
                if (errorMsg) errorMsg[0] = '\0'; // reset errorMsg in case of buffer overflow
                    retCode = IDS_01_004;
                if (totalReturnedLength != NULL)
                    *totalReturnedLength = DataLen + Offset;
            }
            else
                            {
                DataLen = transLen;
                DataLenTruncated = 0;
            }
        }
        else
        {
            return IDS_190_DSTODRV_ERROR;
        }
    }
    else
    {
        memcpy(targetDataPtr, DataPtr, DataLen);
        if (totalReturnedLength != NULL && retCode == IDS_01_004)
            *totalReturnedLength = DataLen + Offset;
    }

    if (CDataType == SQL_C_CHAR && DataLen > 0)
        *(char *)((const char *)targetDataPtr + DataLen) = '\0';
    if (CDataType == SQL_C_WCHAR && iconv->isAppUTF16() && DataLen > 0)
        *((UChar *)((const UChar *)targetDataPtr + DataLen/2)) = UCharNull;

    if (targetStrLenPtr != NULL)
    {
        if (DataLenTruncated != 0)
            *targetStrLenPtr = DataLenTruncated;
        else
            *targetStrLenPtr = DataLen;
    }


    return retCode;
}


unsigned long ODBC::ConvSQLNumberToChar(SQLPOINTER srcDataPtr, CDescRec* srcDescPtr, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN* targetStrLenPtr)
{
    unsigned long       retCode                 = SQL_SUCCESS;
    __int64             lTmp                    = 0;
    unsigned __int64    ulTmp                   = 0;
    double              dTmp                    = 0;
    SQLINTEGER          DataLen                 = 0;
    SQLINTEGER          DecimalPoint            = 0;
    char                cTmpBuf[NUM_LEN_MAX]    = {0};
    char*               tempPtr                 = NULL;

    switch (srcDescPtr->m_ODBCDataType)
    {
    case SQL_TINYINT:
        if(srcDescPtr->m_DescUnsigned)
        {
            ulTmp = *((UCHAR *) srcDataPtr);
            _ultoa(ulTmp, cTmpBuf, 10);
        }
        else
        {
            lTmp = *((SCHAR *) srcDataPtr);
            _ltoa(lTmp, cTmpBuf, 10);
        }
        DataLen = strlen(cTmpBuf);
        if(DataLen > targetLength)
            return IDS_22_003;
        break;
    case SQL_SMALLINT:
        if (srcDescPtr->m_DescUnsigned)
        {
            ulTmp = *((USHORT *) srcDataPtr);
            _ultoa(ulTmp, cTmpBuf, 10);
        }
        else
        {
            lTmp = *((SSHORT *) srcDataPtr);
            _ltoa(lTmp, cTmpBuf, 10);
        }
        DataLen = strlen(cTmpBuf);
        if (DataLen > targetLength)
            return IDS_22_003;
        break;
    case SQL_INTEGER: 
        if (srcDescPtr->m_DescUnsigned)
        {
            ulTmp = *((ULONG_P *)srcDataPtr);
            _ultoa(ulTmp, cTmpBuf, 10);
        }
        else
        {
            lTmp = *((SLONG_P *) srcDataPtr);
            _ltoa(lTmp, cTmpBuf, 10);
        }
        DataLen = strlen(cTmpBuf);
        if (DataLen > targetLength)
            return IDS_22_003;
        break;
    case SQL_BIGINT:
        if (srcDescPtr->m_DescUnsigned)
            sprintf(cTmpBuf, "%llu", *((unsigned __int64 *)srcDataPtr));
        else
            sprintf(cTmpBuf, "%lld", *((__int64 *)srcDataPtr));
        DataLen = strlen(cTmpBuf);
        if (DataLen > targetLength)
            return IDS_22_003;
        break;
    case SQL_REAL:
    case SQL_DOUBLE:
        if ((srcDescPtr->m_SQLDataType == SQLTYPECODE_DECIMAL_LARGE_UNSIGNED) ||
            (srcDescPtr->m_SQLDataType == SQLTYPECODE_DECIMAL_LARGE))
        {
            if (ConvertSoftDecimalToDouble(srcDescPtr->m_SQLDataType, srcDataPtr, srcLength, srcDescPtr->m_ODBCScale,
                                            dTmp)  != SQL_SUCCESS)
                return IDS_07_006;
            if (!double_to_char (dTmp, DBL_DIG, cTmpBuf, targetLength))
                return IDS_22_001;            
        }
        else
        {
            if (srcDescPtr->m_ODBCDataType == SQL_REAL)
            {
                dTmp = (double)(*(float *)srcDataPtr);
                if (!double_to_char (dTmp, FLT_DIG + 1, cTmpBuf, targetLength))
                    return IDS_22_001;
            }
            else
            {
                dTmp = *(double *)srcDataPtr;
                if (!double_to_char (dTmp, DBL_DIG + 1, cTmpBuf, targetLength))
                    return IDS_22_001;
            }
        }
        DataLen = strlen(cTmpBuf);
        if ((tempPtr = strchr(cTmpBuf, '.')) != NULL)
            DecimalPoint = tempPtr - cTmpBuf;
        else
            DecimalPoint = 0;
        if (DecimalPoint > targetLength)
            return IDS_22_003;
        if (DataLen > targetLength)
        {
            DataLen = targetLength-1;
            retCode = IDS_01_004;
        }
        break;
    case SQL_DECIMAL:
        if (ConvertDecimalToChar(srcDescPtr->m_SQLDataType, srcDataPtr, srcLength, srcDescPtr->m_ODBCScale,
                            cTmpBuf, DecimalPoint) != SQL_SUCCESS)
            return IDS_07_006;
        DataLen = strlen(cTmpBuf);
        if (DecimalPoint > targetLength)
            return IDS_22_003;
        if (DataLen > targetLength)
        {
            DataLen = targetLength - 1;
            retCode = IDS_01_004;
        }
        break;
    }

    if (CDataType == SQL_C_WCHAR)
        swprintf((wchar_t *)targetDataPtr, DataLen * 2, L"%s", cTmpBuf);
    else
        sprintf((char *)targetDataPtr, "%s", cTmpBuf);

    return retCode;
}


unsigned long ODBC::ConvertDecimalToChar(SQLSMALLINT SQLDataType, SQLPOINTER srcDataPtr, SQLINTEGER srcLength, SQLSMALLINT srcScale, char *cTmpBuf, SQLINTEGER &DecimalPoint)
{

    char *destTempPtr;
    short i;    
    BOOL leadZero;
    BOOL leadDecimalPoint = TRUE;
    destTempPtr = cTmpBuf;
    char    *tmpPtr;
    
    switch(SQLDataType)
    {
        case SQLTYPECODE_DECIMAL_UNSIGNED:
            leadZero = TRUE;
            
            for (i = 0; i < (srcLength-srcScale) ; i++) {
                if (!(leadZero && ((char *)srcDataPtr)[i] == '0')) {
                    *destTempPtr++ = ((char *)srcDataPtr)[i];
                    leadZero = FALSE;
                    leadDecimalPoint = FALSE;
                }
            }
            if (srcScale > 0)
            {
                if (gDrvrGlobal.gSpecial_1 == false && leadDecimalPoint)
                    *destTempPtr++ = '0';
                *destTempPtr++ = '.';
                for (i = (short)(srcLength-srcScale) ; i < srcLength ; i++)
                    *destTempPtr++ = ((char *)srcDataPtr)[i];
            }
            *destTempPtr = '\0';
            break;
        case SQLTYPECODE_DECIMAL:
            BYTE valByte;    // Sign Bit + first digit
    
            valByte = (BYTE)(*(BYTE *)srcDataPtr & (BYTE)0x80);
            if (valByte) 
                *destTempPtr++ = '-';
            valByte = (BYTE)(*(BYTE *)srcDataPtr & (BYTE) 0x7F);
            if (valByte != '0')
            {    
                if (srcLength != srcScale)
                {
                    *destTempPtr++ = valByte;
                    leadDecimalPoint = FALSE;
                }
                leadZero = FALSE;
            }
            else
                leadZero = TRUE;
            for (i = 1; i < (srcLength-srcScale) ; i++) {
                if (!(leadZero && ((char *)srcDataPtr)[i] == '0')) {
                    *destTempPtr++ = ((char *)srcDataPtr)[i];
                    leadZero = FALSE;
                    leadDecimalPoint = FALSE;
                }
            }
            if (srcScale > 0)
            {
                if (gDrvrGlobal.gSpecial_1 == false && leadDecimalPoint)
                    *destTempPtr++ = '0';
                *destTempPtr++ = '.';
                if (srcLength == srcScale)
                {
                    *destTempPtr++ = valByte;
                    for (i = 1; i < srcLength ; i++)
                        *destTempPtr++ = ((char *)srcDataPtr)[i];
                }
                else
                {
                    for (i = (short)(srcLength-srcScale) ; i < srcLength ; i++)
                        *destTempPtr++ = ((char *)srcDataPtr)[i];
                }
            }
            *destTempPtr = '\0';
            break;
        default: 
            return SQL_ERROR;
    }
    //Fix for customer's trailing Zero problem. Customer b wants to see trailing 0's
    // eg. 123.000. To avoid regression for customer w, we kept the original logic
    // unchanged using if (gDrvrGlobal.gSpecial_1). For customer w
    // trailing zeros will be deleted eg. 123.0
        if (gDrvrGlobal.gSpecial_1)
    {    
        int x = strcspn(cTmpBuf, ".");
        for (i = strlen(cTmpBuf) - 1; i >= 0; i--)
        {
            if (cTmpBuf[i] == '0' && x<i) {cTmpBuf[i] = 0; continue;}
            if (cTmpBuf[i] == '.' )
            {
                strcat(cTmpBuf,"0");
                break;
            }
            break;
        }
    }
    if (cTmpBuf[0] == 0 ) strcpy(cTmpBuf,"0");
    if ((tmpPtr = strchr(cTmpBuf, '.')) != NULL)
        DecimalPoint = tmpPtr - cTmpBuf;
    else
        DecimalPoint = 0;
    return SQL_SUCCESS;
}


unsigned long ODBC::ConvertSoftDecimalToDouble(SQLSMALLINT SQLDataType, SQLPOINTER srcDataPtr, SQLINTEGER srcLength, SQLSMALLINT srcScale, double &dTmp)
{
    char *stopStr;
    char cTmpBuf[256];
    double dTmp1;
    short i;

    switch(SQLDataType)
    {
        case SQLTYPECODE_DECIMAL_LARGE:
            memcpy(cTmpBuf, (const char *)srcDataPtr, srcLength);
            // Make it as a display string
            for (i = 1; i < srcLength ; cTmpBuf[i++] += '0');
            cTmpBuf[srcLength] =  '\0';
            dTmp = strtod(cTmpBuf,&stopStr);
            dTmp1 = pow(10, srcScale);
            dTmp = dTmp / dTmp1;
            break;
        case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED:
            memcpy(cTmpBuf, (const char *)srcDataPtr, srcLength);
            // Make it as a display string
            for (i = 0; i < srcLength ; cTmpBuf[i++] += '0');
            cTmpBuf[srcLength] =  '\0';
            dTmp = strtod(cTmpBuf,&stopStr);
            dTmp1 = pow(10, srcScale);
            dTmp = dTmp / dTmp1;
            break;
        default:
            return SQL_ERROR;
    }
    return SQL_SUCCESS;
}


unsigned long ODBC::ConvSQLNumberToDouble(SQLPOINTER srcDataPtr, CDescRec* srcDescPtr, SQLINTEGER srcLength, double &dTmp)
{
    unsigned long       retCode = SQL_SUCCESS;
    char                cTmpBuf[NUM_LEN_MAX] = {0};
    __int64             tempVal64 = 0;
    SQLINTEGER          DecimalPoint;

    switch (srcDescPtr->m_ODBCDataType)
    {
        case SQL_TINYINT:
            if (srcDescPtr->m_DescUnsigned)
            {
                dTmp = *((UCHAR *) srcDataPtr);
            }
            else
            {
                dTmp = *((SCHAR *) srcDataPtr);
            }
            break;

        case SQL_SMALLINT:
            if (srcDescPtr->m_DescUnsigned)
            {
                dTmp = *((USHORT *) srcDataPtr);
            }
            else
            {
                dTmp = *((SSHORT *) srcDataPtr);
            }
            break;

        case SQL_INTEGER:
            if (srcDescPtr->m_DescUnsigned)
            {
                dTmp = *((ULONG_P *) srcDataPtr);
            }
            else
            {
                dTmp = *((SLONG_P *) srcDataPtr);
            }
            break;
            
        case SQL_REAL:
            dTmp = *(SFLOAT *) srcDataPtr;
            break;
        case SQL_DOUBLE:
            if ((srcDescPtr->m_SQLDataType == SQLTYPECODE_DECIMAL_LARGE_UNSIGNED) ||
                (srcDescPtr->m_SQLDataType == SQLTYPECODE_DECIMAL_LARGE))
            {
                if (ConvertSoftDecimalToDouble(srcDescPtr->m_SQLDataType, srcDataPtr, srcLength, srcDescPtr->m_ODBCScale, dTmp)  != SQL_SUCCESS)
                    return IDS_07_006;
            }
            else
                dTmp =  *(SDOUBLE *)srcDataPtr;
            break;
        case SQL_DECIMAL:
            if (ConvertDecimalToChar(srcDescPtr->m_SQLDataType, srcDataPtr, srcLength, srcDescPtr->m_ODBCScale, cTmpBuf, DecimalPoint)  != SQL_SUCCESS)
                return IDS_07_006;
            if (!ctoi64(cTmpBuf, tempVal64))
                return IDS_22_003;
            dTmp = strtod(cTmpBuf, NULL);
            if (errno == ERANGE)
                return IDS_22_003;
            break;

        default:
            return IDS_07_006;
    }

    return retCode;
}

unsigned long ODBC::ConvSQLBigintToNumber(SQLPOINTER srcDataPtr, bool unsignedValue, SQLSMALLINT CDataType, SQLSMALLINT Scale, SQLPOINTER targetDataPtr, SQLLEN* targetStrLenPtr)
{
    unsigned long       retCode = SQL_SUCCESS;

    __int64             tempVal64   = 0;
    unsigned __int64    utempVal64  = 0;

    if (unsignedValue)
    {
        utempVal64 = *((unsigned __int64 *) srcDataPtr);
    }
    else
    {
        tempVal64 = *((__int64 *) srcDataPtr);
    }

    switch (CDataType)
    {
        case SQL_C_BIT:
        case SQL_C_TINYINT:
        case SQL_C_STINYINT:
            if (unsignedValue)
            {
                if (utempVal64 / pow(10, Scale) > SCHAR_MAX)
                    return IDS_22_003;
                *((SCHAR *)targetDataPtr) = utempVal64 / pow(10, Scale);
                if (*((SCHAR *)targetDataPtr) != utempVal64 / pow(10, Scale))
                        retCode = IDS_01_S07;
            }
            else
            {
                if (tempVal64 / pow(10, Scale) < SCHAR_MIN || tempVal64 / pow(10, Scale) > SCHAR_MAX)
                    return IDS_22_003;
                *((SCHAR *)targetDataPtr) = tempVal64 / pow(10, Scale);
                if (*((SCHAR *)targetDataPtr) != tempVal64 / pow(10, Scale))
                    retCode = IDS_01_S07;
            }
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(SCHAR); 
            break;
        case SQL_C_UTINYINT:
            if (unsignedValue)
            {
                if (utempVal64 / pow(10, Scale) > UCHAR_MAX)
                    return IDS_22_003;
                *((UCHAR *)targetDataPtr) = utempVal64 / pow(10, Scale);
                if (*((UCHAR *)targetDataPtr) != utempVal64 / pow(10, Scale))
                    retCode = IDS_01_S07;
            }
            else
            {
                if (tempVal64 / pow(10, Scale) < 0 || tempVal64 / pow(10, Scale) > UCHAR_MAX)
                    return IDS_22_003;
                *((UCHAR *)targetDataPtr) = tempVal64 / pow(10, Scale);
                if (*((UCHAR *)targetDataPtr) != tempVal64 / pow(10, Scale))
                    retCode = IDS_01_S07;
            }
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(UCHAR); 
            break;

        case SQL_C_SHORT:
        case SQL_C_SSHORT:
            if (unsignedValue)
            {
                if (utempVal64 / pow(10, Scale) > SHRT_MAX)
                    return IDS_22_003;
                *((SHORT *)targetDataPtr) = utempVal64 / pow(10, Scale);
                if (*((SHORT *)targetDataPtr) != utempVal64 / pow(10, Scale))
                    retCode = IDS_01_S07;
            }
            else
            {
                if (tempVal64 / pow(10, Scale) < SHRT_MIN || tempVal64 / pow(10, Scale) > SHRT_MAX)
                    return IDS_22_003;
                *((SHORT *)targetDataPtr) = tempVal64 / pow(10, Scale);
                if (*((SHORT *)targetDataPtr) != tempVal64 / pow(10, Scale))
                    retCode = IDS_01_S07;
            }
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(SHORT); 
            break;

        case SQL_C_USHORT:
            if (unsignedValue)
            {
                if (utempVal64 / pow(10, Scale) > USHRT_MAX)
                    return IDS_22_003;
                *((USHORT *)targetDataPtr) = utempVal64 / pow(10, Scale);
                if (*((USHORT *)targetDataPtr) != utempVal64 / pow(10, Scale))
                    retCode = IDS_01_S07;
            }
            else
            {
                if (tempVal64 / pow(10, Scale) < 0 || tempVal64 / pow(10, Scale) > USHRT_MAX)
                    return IDS_22_003;
                *((USHORT *)targetDataPtr) = (USHORT)(tempVal64 / pow(10, Scale));
                if (*((USHORT *)targetDataPtr) != (USHORT)(tempVal64 / pow(10, Scale)))
                    retCode = IDS_01_S07;
            }
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(USHORT); 
            break;

        case SQL_C_LONG:
        case SQL_C_SLONG:
            if (unsignedValue)
            {
                if (utempVal64 / pow(10, Scale) > LONG_MAX)
                    return IDS_22_003;
                *((SLONG_P *)targetDataPtr) = utempVal64 / pow(10, Scale);
                if (*((SLONG_P *)targetDataPtr) != utempVal64 / pow(10, Scale))
                    retCode = IDS_01_S07;
            }
            else
            {
                if (tempVal64 / pow(10, Scale) < LONG_MIN || tempVal64 / pow(10, Scale) > LONG_MAX)
                    return IDS_22_003;
                *((SLONG_P *)targetDataPtr) = tempVal64 / pow(10, Scale);
                if (*((SLONG_P *)targetDataPtr) != tempVal64 / pow(10, Scale))
                    retCode = IDS_01_S07;
            }
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(SLONG_P); 
            break;

        case SQL_C_ULONG:
            if (unsignedValue)
            {
                if (utempVal64 / pow(10, Scale) > ULONG_MAX)
                    return IDS_22_003;
                *((ULONG_P *)targetDataPtr) = utempVal64 / pow(10, Scale);
                if (*((ULONG_P *)targetDataPtr) != utempVal64 / pow(10, Scale))
                    retCode = IDS_01_S07;
            }
            else
            {
                if (tempVal64 / pow(10, Scale) < 0 || tempVal64 / pow(10, Scale) > ULONG_MAX)
                    return IDS_22_003;
                *((ULONG_P *)targetDataPtr) = tempVal64 / pow(10, Scale);
                if (*((ULONG_P *)targetDataPtr) != tempVal64 / pow(10, Scale))
                    retCode = IDS_01_S07;
            }
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(ULONG_P); 
            break;

        case SQL_C_SBIGINT:
            if (unsignedValue)
            {
                if (utempVal64 / pow(10, Scale) > LLONG_MAX)
                    return IDS_22_003;
                *((__int64 *)targetDataPtr) = utempVal64 / pow(10, Scale);
                if (*((__int64 *)targetDataPtr) != utempVal64 / pow(10, Scale))
                    retCode = IDS_01_S07;
            }
            else
            {
                if (tempVal64 / pow(10, Scale) < LLONG_MIN || tempVal64 / pow(10, Scale) > LLONG_MAX)
                    return IDS_22_003;
                *((__int64 *)targetDataPtr) = tempVal64 / pow(10, Scale);
                if (*((__int64 *)targetDataPtr) != tempVal64 / pow(10, Scale))
                    retCode = IDS_01_S07;
            }
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(__int64); 
            break;

        case SQL_C_UBIGINT:
            if (unsignedValue)
            {
                if (utempVal64 / pow(10, Scale) > ULLONG_MAX)
                    return IDS_22_003;
                *((unsigned __int64 *)targetDataPtr) = utempVal64 / pow(10, Scale);
                if (*((unsigned __int64 *)targetDataPtr) != utempVal64 / pow(10, Scale))
                    retCode = IDS_01_S07;
            }
            else
            {
                if (tempVal64 / pow(10, Scale) < 0 || tempVal64 / pow(10, Scale) > ULLONG_MAX)
                    return IDS_22_003;
                *((unsigned __int64 *)targetDataPtr) = tempVal64 / pow(10, Scale);
                if (*((unsigned __int64 *)targetDataPtr) != tempVal64 / pow(10, Scale))
                    retCode = IDS_01_S07;
            }
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(unsigned __int64); 
            break;

        case SQL_C_FLOAT:
            if (unsignedValue)
            {
                if (utempVal64 / pow(10, Scale) > FLT_MAX)
                    return IDS_22_003;
                *((FLOAT *)targetDataPtr) = (FLOAT)utempVal64 / pow(10, Scale);
                if (*((FLOAT *)targetDataPtr) != (FLOAT)utempVal64 / pow(10, Scale))
                    retCode = IDS_01_S07;
            }
            else
            {
                if (tempVal64 / pow(10, Scale) < -FLT_MAX || tempVal64 / pow(10, Scale) > FLT_MAX)
                    return IDS_22_003;
                *((FLOAT *)targetDataPtr) = (FLOAT)tempVal64 / pow(10, Scale);
                if (*((FLOAT *)targetDataPtr) != (FLOAT)tempVal64 / pow(10, Scale))
                    retCode = IDS_01_S07;
            }
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(FLOAT); 
            break;

        case SQL_C_DOUBLE:
            *((SQLDOUBLE*)targetDataPtr) = (unsignedValue ?
                                                ((SQLDOUBLE)utempVal64 / pow(10, Scale)): ((SQLDOUBLE)tempVal64 / pow(10, Scale)));
            if (*((SQLDOUBLE*)targetDataPtr) = (unsignedValue ?
                                                ((SQLDOUBLE)utempVal64 / pow(10, Scale)): ((SQLDOUBLE)tempVal64 / pow(10, Scale))))
                retCode = IDS_01_S07;
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(SQLDOUBLE); 
            break;

        default:
            return IDS_07_006;
    }

    return retCode;
}


unsigned long ODBC::ConvSQLNumericToNumber(SQLPOINTER srcDataPtr, CDescRec* srcDescPtr, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLLEN* targetStrLenPtr)
{
    unsigned long       retCode     = SQL_SUCCESS;
    double              dTmp        = 0;
    char                cTmpBuf[NUM_LEN_MAX]    = {0};

    switch (srcDescPtr->m_SQLDataType)
    {
    case SQLTYPECODE_SMALLINT:
        dTmp = *((SHORT *)srcDataPtr);
        if (srcDescPtr->m_ODBCScale > 0)
            dTmp = dTmp / (long)pow(10,srcDescPtr->m_ODBCScale);
        break;

    case SQLTYPECODE_SMALLINT_UNSIGNED:
        dTmp = *((USHORT *)srcDataPtr);
        if (srcDescPtr->m_ODBCScale > 0)
            dTmp = dTmp / (long)pow(10,srcDescPtr->m_ODBCScale);
        break;

    case SQLTYPECODE_INTEGER:
        dTmp = *((SLONG_P *)srcDataPtr);
        if (srcDescPtr->m_ODBCScale > 0)
            dTmp = dTmp / (long)pow(10,srcDescPtr->m_ODBCScale);
        break;

    case SQLTYPECODE_INTEGER_UNSIGNED:
        dTmp = *((ULONG_P *)srcDataPtr);
        if (srcDescPtr->m_ODBCScale > 0)
            dTmp = dTmp / (long)pow(10,srcDescPtr->m_ODBCScale);
        break;

    case SQLTYPECODE_LARGEINT:
        retCode = ConvSQLBigintToNumber(srcDataPtr, false, CDataType, srcDescPtr->m_ODBCScale, targetDataPtr, targetStrLenPtr);
        break;

    case SQLTYPECODE_LARGEINT_UNSIGNED:
        retCode = ConvSQLBigintToNumber(srcDataPtr, true, CDataType, srcDescPtr->m_ODBCScale, targetDataPtr, targetStrLenPtr);
        break;

    case SQLTYPECODE_NUMERIC:
    case SQLTYPECODE_NUMERIC_UNSIGNED:
        retCode = BigNum_To_Ascii_Helper((char*)srcDataPtr, srcLength, srcDescPtr->m_ODBCPrecision, srcDescPtr->m_ODBCScale, cTmpBuf, srcDescPtr->m_SQLDataType);
        if(retCode != SQL_SUCCESS)
            return retCode;
        dTmp = strtod(cTmpBuf, NULL);
        if(errno == ERANGE)
            return IDS_22_003;

        break;

    default:
        return IDS_HY_000;
    }

    if (srcDescPtr->m_SQLDataType != SQLTYPECODE_LARGEINT && srcDescPtr->m_SQLDataType != SQLTYPECODE_LARGEINT_UNSIGNED)
        retCode = ConvDoubleToCNumber(dTmp, CDataType, targetDataPtr, targetStrLenPtr);

    return retCode;
}

unsigned long ODBC::ConvFromSQLBool(SQLPOINTER srcDataPtr, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN* targetStrLenPtr)
{
    unsigned long   retCode  = SQL_SUCCESS;
    double      dTmp     = 0;
    SQLINTEGER  DataLen  = 0;
    rTrim((char *)srcDataPtr);
    DataLen = strlen((char *)srcDataPtr);
    switch (CDataType)
    {
        case SQL_C_CHAR:
            if (DataLen + 1 > targetLength)
                return IDS_22_003;
            if (targetStrLenPtr)
                *targetStrLenPtr = snprintf((char *)targetDataPtr, targetLength, "%s", srcDataPtr);
            break;
            
        case SQL_C_WCHAR:
            if (DataLen * 2 + 1 > targetLength)
                return IDS_22_003;
            if (targetStrLenPtr)
                *targetStrLenPtr = swprintf((wchar_t *)targetDataPtr, targetLength, L"%s", srcDataPtr);
            break;

        case SQL_C_BIT:
        case SQL_C_TINYINT:
        case SQL_C_STINYINT:
        case SQL_C_UTINYINT:
        case SQL_C_SHORT:
        case SQL_C_SSHORT:
        case SQL_C_USHORT:
        case SQL_C_LONG:
        case SQL_C_SLONG:
        case SQL_C_ULONG:
        case SQL_C_SBIGINT:
        case SQL_C_UBIGINT:
        case SQL_C_FLOAT:
        case SQL_C_DOUBLE:
            dTmp = strtod((char*)srcDataPtr, NULL);
            retCode = ConvDoubleToCNumber(dTmp, CDataType, targetDataPtr, targetStrLenPtr);
            break;
        default:
            return IDS_07_006;
    }
    
    return retCode;
}

unsigned long ODBC::ConvSQLCharToNumber(SQLPOINTER srcDataPtr, CDescRec* srcDescPtr, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLLEN* targetStrLenPtr, ICUConverter* iconv, UCHAR *errorMsg)
{
    int             TransStringLength   = 0;
    double          dTmp                = 0;
    unsigned long   retCode             = SQL_SUCCESS;
    char*           cTmpBuf             = NULL;
    SQLINTEGER      tmpLen              = 0;
    char*           error               = NULL;
    __int64         tempVal64           = 0;
    SQLINTEGER      DataLen             = 0;

    retCode = GetCTmpBufFromSQLChar(srcDataPtr, srcLength, srcDescPtr->m_ODBCDataType, srcDescPtr->m_SQLCharset, srcDescPtr->m_SQLMaxLength <= MAXCHARLEN, cTmpBuf, tmpLen, iconv, errorMsg, true);
    if (retCode != SQL_SUCCESS)
    {
        if (cTmpBuf)
        {
            free(cTmpBuf);
            cTmpBuf = NULL;
        }
        return retCode;
    }
    dTmp = strtod(cTmpBuf, &error);
    if (errno == ERANGE)
    {
        if (cTmpBuf)
        {
            free(cTmpBuf);
            cTmpBuf = NULL;
        }
        return IDS_22_018;
    }
    retCode = ConvDoubleToCNumber(dTmp, CDataType, targetDataPtr, targetStrLenPtr);
    if (cTmpBuf)
    {
        free(cTmpBuf);
        cTmpBuf = NULL;
    }
    return retCode;
}


unsigned long ODBC::ConvSQLIntevalToDouble(SQLPOINTER srcDataPtr, double &dTmp)
{
    char*   str                     = NULL;
    char    tempBuf[NUM_LEN_MAX]    = {0};
    int     tempLen                 = 0;
    errno   = 0;

    str = trimInterval((char *)srcDataPtr);
    tempLen = strlen(str);

    if (tempLen > sizeof(tempBuf) - 1)
        return IDS_22_003;

    strncpy(tempBuf, (const char *)str, tempLen);
    dTmp = strtod(tempBuf, NULL);
    if (errno == ERANGE)
        return IDS_22_018;

    return SQL_SUCCESS;
}


unsigned long ODBC::ConvDoubleToCNumber(double dTmp, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLLEN* targetStrLenPtr)
{
    unsigned long       retCode = SQL_SUCCESS;

    switch (CDataType)
    {
        case SQL_C_BIT:
        case SQL_C_TINYINT:
        case SQL_C_STINYINT:
            if (dTmp < SCHAR_MIN || dTmp > SCHAR_MAX)
                return IDS_22_003;
            *((SCHAR *)targetDataPtr) = (SCHAR)dTmp;    
            if (*((SCHAR *)targetDataPtr) != dTmp)
                retCode = IDS_01_S07;
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(SCHAR); 
            break;

        case SQL_C_UTINYINT:
            if (dTmp < 0 || dTmp > UCHAR_MAX)
                return IDS_22_003;
            *((UCHAR *)targetDataPtr) = (UCHAR)dTmp;
            if (*((UCHAR *)targetDataPtr) != dTmp)
                retCode = IDS_01_S07;
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(UCHAR); 
            break;

        case SQL_C_SHORT:
        case SQL_C_SSHORT:
            if (dTmp < SHRT_MIN || dTmp > SHRT_MAX)
                return IDS_22_003;
            *((SHORT *)targetDataPtr) = (SHORT)dTmp;
            if (*((SHORT *)targetDataPtr) != dTmp)
                retCode = IDS_01_S07;
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(SHORT); 
            break;

        case SQL_C_USHORT:
            if (dTmp < 0 || dTmp > USHRT_MAX)
                return IDS_22_003;
            *((USHORT *)targetDataPtr) = (USHORT)dTmp;
            if (*((USHORT *)targetDataPtr) != dTmp)
                retCode = IDS_01_S07;
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(USHORT); 
            break;

        case SQL_C_LONG:
        case SQL_C_SLONG:
            if (dTmp < LONG_MIN || dTmp > LONG_MAX)
                return IDS_22_003;
            *((SLONG_P *)targetDataPtr) = (SLONG_P)dTmp;
            if (*((SLONG_P *)targetDataPtr) != dTmp)
                retCode = IDS_01_S07;
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(SLONG_P); 
            break;

        case SQL_C_ULONG:
            if (dTmp < 0 ||dTmp > ULONG_MAX)
                return IDS_22_003;
            *((ULONG_P *)targetDataPtr) = (ULONG_P)dTmp;
            if (*((ULONG_P *)targetDataPtr) != dTmp)
                retCode = IDS_01_S07;
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(ULONG_P); 
            break;

        case SQL_C_SBIGINT:
            if (dTmp < LLONG_MIN || dTmp > LLONG_MAX)
                return IDS_22_003;
            *((__int64 *)targetDataPtr) = (__int64)dTmp;
            if (*((__int64 *)targetDataPtr) != dTmp)
                retCode = IDS_01_S07;
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(__int64); 
            break;

        case SQL_C_UBIGINT:
            if (dTmp < 0 || dTmp > ULLONG_MAX)
                return IDS_22_003;
            *((unsigned __int64 *)targetDataPtr) = (unsigned __int64)dTmp;
            if (*((unsigned __int64 *)targetDataPtr) != dTmp)
                retCode = IDS_01_S07;
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(unsigned __int64); 
            break;

        case SQL_C_FLOAT:
            if (dTmp < -FLT_MAX || dTmp > FLT_MAX)
                return IDS_22_003;
            *((float *)targetDataPtr) = (float)dTmp;
            if (*((float *)targetDataPtr) != dTmp)
                retCode = IDS_01_S07;
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(FLOAT); 
            break;

        case SQL_C_DOUBLE:
            *((SQLDOUBLE *)targetDataPtr) = dTmp;
            if (*((SQLDOUBLE *)targetDataPtr) != dTmp)
                retCode = IDS_01_S07;
            if (targetStrLenPtr)
                *targetStrLenPtr = sizeof(SQLDOUBLE); 
            break;

        default:
            return IDS_07_006;
    }

    return retCode;
}


unsigned long ODBC::ConvSQLNumericToChar(SQLPOINTER srcDataPtr, CDescRec* srcDescPtr, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN* targetStrLenPtr)
{
    unsigned long   retCode                 = SQL_SUCCESS;
    char            cTmpBuf[NUM_LEN_MAX]    = {0};
    SQLINTEGER      DecimalPoint            = 0;
    SQLLEN          retLen                  = 0;

    if(srcDescPtr->m_SQLDataType == SQLTYPECODE_NUMERIC || srcDescPtr->m_SQLDataType == SQLTYPECODE_NUMERIC_UNSIGNED) //for bignum support
    {
        retCode = BigNum_To_Ascii_Helper((char*)srcDataPtr, srcLength, srcDescPtr->m_ODBCPrecision, srcDescPtr->m_ODBCScale, cTmpBuf, srcDescPtr->m_SQLDataType);
    }
    else
    {
        if ((ConvSQLSoftNumericToChar(srcDescPtr->m_SQLDataType, srcDataPtr, srcDescPtr->m_ODBCScale, cTmpBuf, DecimalPoint)) != SQL_SUCCESS)
            return IDS_07_006;
    }

    if (CDataType == SQL_C_WCHAR)
    {
        if (strlen(cTmpBuf) * 2 + 1 > targetLength)
            return IDS_22_003;
        retLen = swprintf((wchar_t *)targetDataPtr, strlen(cTmpBuf), L"%s", cTmpBuf);
    }
    else
    {
        if (strlen(cTmpBuf) + 1 > targetLength)
            return IDS_22_003;
        retLen = sprintf((char *)targetDataPtr, "%s", cTmpBuf);
    }

    if (targetStrLenPtr)
        *targetStrLenPtr = retLen;

    return retCode;
}


unsigned long ODBC::BigNum_To_Ascii_Helper(char * source,
                    long sourceLen,
                    long sourcePrecision,
                    long sourceScale,
                    char * target,
                    SQLSMALLINT SQLDataType
                    )                        
{
    char *testb, *ctemp2, *ctemp3;
    short sourceType= 0;
    SQLRETURN retcode = -1;
    char  ctemp1[132];
    BOOL truncation = FALSE;
    char *saveptr=NULL;

    if (SQLDataType == SQLTYPECODE_NUMERIC_UNSIGNED)
        sourceType = BIGNUM_UNSIGNED;
    else 
        sourceType = BIGNUM_SIGNED;                        
    
    retcode = convDoItMxcs((char *)source, sourceLen, sourceType, sourcePrecision, sourceScale, target, 1+sourcePrecision+1, 0,0,0,0, &truncation);
            
    if (retcode != 0)
        return IDS_22_003;

    testb = new char[sourcePrecision+4];

    if (gDrvrGlobal.gSpecial_1) 
    {    //need no leading zero
        strcpy(testb,target);
        if((testb[0] == '0')||(testb[0] == '-') && (testb[1] == '0'))  
        {
            ctemp2 = strtok(testb, ".");
            ctemp3= strtok(NULL, "\0");
            if (testb[0] == '0') 
                sprintf(ctemp1,".%s", ctemp3);
            else 
                sprintf(ctemp1,"-.%s", ctemp3);
            strcpy(target, ctemp1);
        }                    
    } 
    else 
    {   //need leading zero
        strcpy(testb,target);
        if(testb[0] == '.') 
        {
            sprintf(ctemp1, "0%s", target);
            strcpy(target, ctemp1);
        } 
        else if((testb[0] == '-') && (testb[1] == '.')) 
        {
            ctemp2 = strtok(testb, ".");
            ctemp3= strtok(NULL, "\0");
            sprintf(ctemp1, "-0.%s", ctemp3);
            strcpy(target, ctemp1);
        }                                        
    }

    delete[] testb;

    return SQL_SUCCESS;

} // BigNum_To_Ascii_Helper()


unsigned long ODBC::ConvSQLSoftNumericToChar(SQLSMALLINT SQLDataType, SQLPOINTER srcDataPtr, SQLSMALLINT srcScale, char *cTmpBuf, SQLINTEGER &DecimalPoint)
{

    long                lTmp        = 0;
    ldiv_t              lDiv;
    __int64             i64Tmp      = 0;
    unsigned __int64    ui64Tmp     = 0;
    char*               tmpPtr      = NULL;

    switch (SQLDataType)
    {
    case SQLTYPECODE_SMALLINT:
        lTmp = *((short *)srcDataPtr);
        if (srcScale > 0)
        {
            lDiv = ldiv(lTmp, (long)pow(10,srcScale));
            if (gDrvrGlobal.gSpecial_1 && lDiv.quot == 0)
            {
                if (lDiv.rem < 0)
                    sprintf(cTmpBuf, "-.%0*ld", srcScale, abs(lDiv.rem));
                else
                    sprintf(cTmpBuf, ".%0*ld", srcScale, abs(lDiv.rem));
            }
            else
            {
                if (lDiv.quot == 0 && lDiv.rem < 0)
                    sprintf(cTmpBuf, "-%ld.%0*ld", lDiv.quot, srcScale, abs(lDiv.rem));
                else
                    sprintf(cTmpBuf, "%ld.%0*ld", lDiv.quot, srcScale, abs(lDiv.rem));
            }
        }
        else
            sprintf(cTmpBuf, "%ld", lTmp);
        break;
    case SQLTYPECODE_SMALLINT_UNSIGNED:
        lTmp = *((unsigned short *)srcDataPtr);
        if (srcScale > 0)
        {
            lDiv = ldiv(lTmp, (long)pow(10,srcScale));
            if (gDrvrGlobal.gSpecial_1 && lDiv.quot == 0)
                sprintf(cTmpBuf, ".%0*ld", srcScale, abs(lDiv.rem));
            else
                sprintf(cTmpBuf, "%ld.%0*ld", lDiv.quot, srcScale, abs(lDiv.rem));
        }
        else
            sprintf(cTmpBuf, "%ld", lTmp);
        break;
    case SQLTYPECODE_INTEGER:
        lTmp = *((IDL_long *)srcDataPtr);
        if (srcScale > 0)
        {
            lDiv = ldiv(lTmp, (long)pow(10,srcScale));
            if (gDrvrGlobal.gSpecial_1 && lDiv.quot == 0)
            {
                if (lDiv.rem < 0)
                    sprintf(cTmpBuf, "-.%0*ld", srcScale, abs(lDiv.rem));
                else
                    sprintf(cTmpBuf, ".%0*ld", srcScale, abs(lDiv.rem));
            }
            else
            {
                if (lDiv.quot == 0 && lDiv.rem < 0)
                    sprintf(cTmpBuf, "-%ld.%0*ld", lDiv.quot, srcScale, abs(lDiv.rem));
                else
                    sprintf(cTmpBuf, "%ld.%0*ld", lDiv.quot, srcScale, abs(lDiv.rem));
            }
        }
        else
            sprintf(cTmpBuf, "%ld", lTmp);
        break;
    case SQLTYPECODE_INTEGER_UNSIGNED:
        i64Tmp = *((ULONG_P *)srcDataPtr);
        if (srcScale > 0)
        {
            __int64 power ;
            short i;
            for (i = 0, power = 1 ; i < srcScale ; power *= 10, i++);   
            __int64 t = i64Tmp / power;
            __int64 rem = i64Tmp - t * power;
            if (rem < 0)
                rem = -rem; // Is there a abs for __int64?

            if (gDrvrGlobal.gSpecial_1 && t==0)
                sprintf(cTmpBuf, ".%0*lld", srcScale, rem);
            else
                sprintf(cTmpBuf, "%lld.%0*lld", t, srcScale, rem);
        }
        else
            sprintf(cTmpBuf, "%lld", i64Tmp);
        break;
    case SQLTYPECODE_LARGEINT:
        i64Tmp = *((__int64 *)srcDataPtr);

        if (srcScale > 0)
        {
            __int64 power ;
            short i;
            for (i = 0, power = 1 ; i < srcScale ; power *= 10, i++);
            __int64 t = i64Tmp / power;
            __int64 rem = i64Tmp - t * power;

            if (gDrvrGlobal.gSpecial_1 && t == 0)
            {
                if (rem < 0)
                    sprintf(cTmpBuf, "-.%0*lld", srcScale, -rem);
                else
                    sprintf(cTmpBuf, ".%0*lld", srcScale, rem);
            }
            else
            {
                if (t == 0 && rem < 0)
                sprintf(cTmpBuf, "-%lld.%0*lld", t, srcScale, -rem);
                else
                {
                    if (rem < 0)
                        rem = -rem; // Is there a abs for __int64?
                    sprintf(cTmpBuf, "%lld.%0*lld", t, srcScale, rem);
                }
            }
        }
        else
            sprintf(cTmpBuf, "%lld", i64Tmp);
        break;
    case SQLTYPECODE_LARGEINT_UNSIGNED:
        ui64Tmp = *((unsigned __int64 *)srcDataPtr);

        if (srcScale > 0)
        {
            unsigned __int64 power ;
            short i;
            for (i = 0, power = 1 ; i < srcScale ; power *= 10, i++);
            unsigned __int64 t = ui64Tmp / power;
            unsigned __int64 rem = ui64Tmp - t * power;

            if (gDrvrGlobal.gSpecial_1 && t == 0)
            {
                if (rem < 0)
                    sprintf(cTmpBuf, "-.%0*llu", srcScale, -rem);
                else
                    sprintf(cTmpBuf, ".%0*llu", srcScale, rem);
            }
            else
            {
                if (t == 0 && rem < 0)
                sprintf(cTmpBuf, "-%llu.%0*llu", t, srcScale, -rem);
                else
                {
                    if (rem < 0)
                        rem = -rem; // Is there a abs for __int64?
                    sprintf(cTmpBuf, "%llu.%0*llu", t, srcScale, rem);
                }
            }
        }
        else
            sprintf(cTmpBuf, "%llu", ui64Tmp);
        break;
    default:
        return SQL_ERROR;
    }
    if ((tmpPtr = strchr(cTmpBuf, '.')) != NULL)
        DecimalPoint = tmpPtr - cTmpBuf;
    else
        DecimalPoint = 0;
    return SQL_SUCCESS;
}


unsigned long ODBC::ConvSQLDateToChar(SQLPOINTER srcDataPtr, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN* targetStrLenPtr)
{
    DATE_TYPES*     DatePtr = (DATE_TYPES*)srcDataPtr;
    SQLLEN          retLen  = 0;
    if (CDataType == SQL_C_CHAR)
    {
        if (targetLength < SQL_DATE_LEN + 1)
            return IDS_22_003;
        retLen = sprintf((char *)targetDataPtr, "%04d-%02d-%02d", DatePtr->year, DatePtr->month, DatePtr->day);
    }
    else
    {
        if (targetLength < SQL_DATE_LEN * 2 + 1)
            return IDS_22_003;
        retLen = swprintf((wchar_t *)targetDataPtr, SQL_DATE_LEN * 2, L"%04d-%02d=%02d", DatePtr->year, DatePtr->month, DatePtr->day);
    }

    if (targetStrLenPtr)
        *targetStrLenPtr = retLen;
    return SQL_SUCCESS;
}


unsigned long ODBC::ConvSQLTimeToChar(SQLPOINTER srcDataPtr, SQLSMALLINT srcPrecision, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN* targetStrLenPtr)
{
    TIME_TYPES*     TimePtr     = (TIME_TYPES*)srcDataPtr;
    SQLUINTEGER     ulFraction  = *(UDWORD_P *)TimePtr->fraction;
    SQLLEN          retLen      = 0;

    if (CDataType == SQL_C_CHAR)
    {
        if (srcPrecision > 0)
        {
            if (targetLength < SQL_TIME_LEN + 1 + srcPrecision + 1)
                return IDS_22_003;
            retLen = sprintf((char *)targetDataPtr, "%02d:%02d:%02d.%0*lu", TimePtr->hour, TimePtr->minute, TimePtr->second, srcPrecision, ulFraction);
        }
        else
        {
            if (targetLength < SQL_TIME_LEN + 1)
                return IDS_22_003;
            retLen = sprintf((char *)targetDataPtr, "%02d:%02d:%02d", TimePtr->hour, TimePtr->minute, TimePtr->second);
        }

    }
    else // SQL_C_WCHAR
    {
        if (srcPrecision > 0)
        {
            if (targetLength < (SQL_TIME_LEN + 1 + srcPrecision) * 2 + 1)
                return IDS_22_003;
            retLen = swprintf((wchar_t *)targetDataPtr, (SQL_TIME_LEN + 1 + srcPrecision) * 2, L"%02d:%02d:%02d.%0*lu", TimePtr->hour, TimePtr->minute, TimePtr->second, srcPrecision, ulFraction);
        }
        else
        {
            if (targetLength < SQL_TIME_LEN * 2 + 1)
                return IDS_22_003;
            retLen = swprintf((wchar_t *)targetDataPtr, SQL_TIME_LEN * 2, L"%02d:%02d:%02d", TimePtr->hour, TimePtr->minute, TimePtr->second);
        }
    }

    if (targetStrLenPtr)
        *targetStrLenPtr = retLen;
    return SQL_SUCCESS;
}


unsigned long ODBC::ConvSQLTimestampToChar(SQLPOINTER srcDataPtr, SQLSMALLINT srcPrecision, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN* targetStrLenPtr)
{
    TIMESTAMP_TYPES*    TimestampPtr    = (TIMESTAMP_TYPES*)srcDataPtr;
    SQLUINTEGER         ulFraction      = *(UDWORD_P *)TimestampPtr->fraction;
    SQLLEN              retLen          = 0;

    if (CDataType == SQL_C_CHAR)
    {
        if (srcPrecision > 0)
        {
            if (targetLength < SQL_TIMESTAMP_LEN + 1 + srcPrecision + 1)
                return IDS_22_003;
            retLen = sprintf((char *)targetDataPtr,
                    "%04d-%02u-%02u %02u:%02u:%02u.%0*u",
                    TimestampPtr->year, TimestampPtr->month, TimestampPtr->day,
                    TimestampPtr->hour, TimestampPtr->minute, TimestampPtr->second,
                    srcPrecision, ulFraction);
        }
        else
        {
            if (targetLength < SQL_TIMESTAMP_LEN + 1)
                return IDS_22_003;
            retLen = sprintf((char *)targetDataPtr,
                    "%04d-%02u-%02u %02u:%02u:%02u",
                    TimestampPtr->year, TimestampPtr->month, TimestampPtr->day,
                    TimestampPtr->hour, TimestampPtr->minute, TimestampPtr->second);
        }

    }
    else // SQL_C_WCHAR
    {
        if (srcPrecision > 0)
        {
            if (targetLength < (SQL_TIMESTAMP_LEN + 1 + srcPrecision) * 2 + 1)
                return IDS_22_003;
            retLen = swprintf((wchar_t *)targetDataPtr,
                    (SQL_TIMESTAMP_LEN + 1 + srcPrecision) * 2,
                    L"%04d-%02u-%02u %02u:%02u:%02u.%0*lu",
                    TimestampPtr->year, TimestampPtr->month, TimestampPtr->day,
                    TimestampPtr->hour, TimestampPtr->minute, TimestampPtr->second,
                    srcPrecision, ulFraction);
        }
        else
        {
            if (targetLength < SQL_TIMESTAMP_LEN * 2 + 1)
                return IDS_22_003;
            retLen = swprintf((wchar_t *)targetDataPtr,
                    SQL_TIMESTAMP_LEN * 2,
                    L"%04d-%02u-%02u %02u:%02u:%02u",
                    TimestampPtr->year, TimestampPtr->month, TimestampPtr->day,
                    TimestampPtr->hour, TimestampPtr->minute, TimestampPtr->second);
        }
    }

    if (targetStrLenPtr)
        *targetStrLenPtr = retLen;
    return SQL_SUCCESS;
}


unsigned long ODBC::ConvCopyColumnwiseData(SQLPOINTER srcDataPtr, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN* targetStrLenPtr)
{
    SQLLEN      retLen      = 0;


    if (CDataType == SQL_C_CHAR)
    {
        if (targetLength < srcLength + 1)
            return IDS_22_003;
        retLen = snprintf((char *)targetDataPtr, srcLength, "%s", srcDataPtr);
    }
    else // SQL_C_WCHAR
    {
        if (targetLength < srcLength * 2 + 1)
            return IDS_22_003;
        retLen = swprintf((wchar_t *)targetDataPtr, srcLength * 2, L"%s", srcDataPtr);
    }

    if (targetStrLenPtr)
        *targetStrLenPtr = retLen;
    return SQL_SUCCESS;
}


unsigned long ODBC::ConvSQLCharToDateTime(SQLPOINTER srcDataPtr,
                        SQLINTEGER    srcLength,
                        SQLSMALLINT CDataType,
                        SQLPOINTER outValue)
{
    char    in_value[50];
    short   datetime_parts[8];
    char    *token;
    short   i;
    SQLUINTEGER fraction_part = 0;
    char    delimiters[3];
    SQLINTEGER   len;
    char    *strPtr;
    char    *saveptr=NULL;

    len = srcLength-1;
    strPtr = (char *)srcDataPtr;
    if (strPtr[ len - 1 ] == ' ')
    {
        strPtr[ len - 1 ] = 0;
        rTrim( strPtr);
        len = strlen(strPtr);
    }

    if (len >= sizeof(in_value))
        len = sizeof(in_value)-1;
    strncpy(in_value, strPtr, len);
    in_value[len] = '\0';
    if (len != (short)strspn(in_value, "1234567890:/.- "))
       return IDS_22_018;
    for (i = 0 ; i < 8 ; i++)
       datetime_parts[i] = 0;
    if (strpbrk(in_value, "/-") == NULL)
    {
        i = 3;
        strcpy(delimiters, ":");

        struct tm *newtime;
        time_t long_time;

        time( &long_time );                    /* Get time as long integer. */
        newtime = localtime( &long_time );    /* Convert to local time. */
        datetime_parts[0] = (short)(newtime->tm_year+1900);
        datetime_parts[1] = (short)(newtime->tm_mon+1);
        datetime_parts[2] = (short)newtime->tm_mday;
    }
    else
    {
        i = 0;
        strcpy(delimiters, "/-");
    }
#ifndef unixcli
    for (token = strtok(in_value, delimiters) ; token != NULL && i < 6 ;
            token = strtok(NULL, delimiters), i++)
#else
     for (token = strtok_r(in_value, delimiters,&saveptr) ; token != NULL && i < 6 ;
            token = strtok_r(NULL, delimiters,&saveptr), i++)
#endif
    {
       datetime_parts[i] = (short)atoi(token); 
       if (i == 1)
          strcpy(delimiters, ": ");
       else
       if (i == 2)
          strcpy(delimiters, ":");
       else
       if (i == 4)
          strcpy(delimiters, ".");
    }
    if (token != NULL)
    {
        int exponent = 9 - strlen(token);
        fraction_part = (exponent >= 0)? atol(token) * pow((double)10,exponent):atol(token) / pow((double)10,-exponent) ;
        datetime_parts[6] = (short)(fraction_part / 1000);
        datetime_parts[7] = (short)(fraction_part % 1000);
    }
    if (! checkDatetimeValue(datetime_parts))
        return(IDS_22_018);
    switch (CDataType)
    {
    case SQL_C_DATE:
    case SQL_C_TYPE_DATE:
        ((DATE_STRUCT *)outValue)->year=datetime_parts[0];
        ((DATE_STRUCT *)outValue)->month=datetime_parts[1];
        ((DATE_STRUCT *)outValue)->day=datetime_parts[2];
        break;
    case SQL_C_TIME:
    case SQL_C_TYPE_TIME:
        ((TIME_STRUCT *)outValue)->hour=datetime_parts[3];
        ((TIME_STRUCT *)outValue)->minute=datetime_parts[4];
        ((TIME_STRUCT *)outValue)->second=datetime_parts[5];
        break;
    case SQL_C_TIMESTAMP:
    case SQL_C_TYPE_TIMESTAMP:
        ((TIMESTAMP_STRUCT *)outValue)->year=datetime_parts[0];
        ((TIMESTAMP_STRUCT *)outValue)->month=datetime_parts[1];
        ((TIMESTAMP_STRUCT *)outValue)->day=datetime_parts[2];
        ((TIMESTAMP_STRUCT *)outValue)->hour=datetime_parts[3];
        ((TIMESTAMP_STRUCT *)outValue)->minute=datetime_parts[4];
        ((TIMESTAMP_STRUCT *)outValue)->second=datetime_parts[5];
        memcpy(&((TIMESTAMP_STRUCT *)outValue)->fraction, &fraction_part, sizeof(fraction_part));
        break;
    default:
        return IDS_07_006;
    }
    return 0;
} 


unsigned long ODBC::ConvSQLDateToDate(SQLPOINTER srcDataPtr, SQLSMALLINT SQLDatetimeCode, SQLPOINTER targetDataPtr, SQLLEN* targetStrLenPtr, BOOL ColumnwiseData)
{
    DATE_STRUCT*    targetDate = (DATE_STRUCT *)targetDataPtr;
    DATE_TYPES      DateTmp;
    memset(&DateTmp, 0, sizeof(DATE_TYPES));
    DATE_TYPES*     srcDate = NULL;

    if (ColumnwiseData)
    {
        srcDate = &DateTmp;
        srcDate->year = 01;
        srcDate->month = 01;
        srcDate->day = 01;
        switch (SQLDatetimeCode)
        {
        case SQLDTCODE_YEAR:
            srcDate->year = GetYearFromStr((UCHAR*)srcDataPtr);
            break;
        case SQLDTCODE_YEAR_TO_MONTH:
            srcDate->year = GetYearFromStr((UCHAR*)srcDataPtr);
            srcDate->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
            break;
        case SQLDTCODE_MONTH:
            srcDate->month = GetMonthFromStr((UCHAR*)srcDataPtr);
            break;
        case SQLDTCODE_MONTH_TO_DAY:
            srcDate->month = GetMonthFromStr((UCHAR*)srcDataPtr);
            srcDate->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
            break;
        case SQLDTCODE_DAY:
            srcDate->day = GetDayFromStr((UCHAR*)srcDataPtr);
            break;
        default:
            srcDate->year = GetYearFromStr((UCHAR*)srcDataPtr);
            srcDate->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
            srcDate->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
            break;
        }
    }
    else
    {
        srcDate = (DATE_TYPES *)srcDataPtr;
    }

    targetDate->year = srcDate->year;
    targetDate->month = srcDate->month;
    targetDate->day = srcDate->day;

    if (targetStrLenPtr)
        *targetStrLenPtr = sizeof(DATE_STRUCT);

    return SQL_SUCCESS;
}


unsigned long ODBC::ConvSQLTimestampToDate(SQLPOINTER srcDataPtr, SQLSMALLINT SQLDatetimeCode, SQLPOINTER targetDataPtr, SQLLEN* targetStrLenPtr, BOOL ColumnwiseData)
{
    unsigned long           retCode = SQL_SUCCESS;
    DATE_STRUCT*            targetDate = (DATE_STRUCT *)targetDataPtr;
    TIMESTAMP_TYPES         TimeStampTmp;
    memset(&TimeStampTmp, 0,sizeof(TIMESTAMP_TYPES));
    TIMESTAMP_TYPES*        srcTimeStamp = NULL;
    if (ColumnwiseData) // !RowwiseRowSet
    {
        srcTimeStamp = &TimeStampTmp;
        srcTimeStamp->year = 01;
        srcTimeStamp->month = 01;
        srcTimeStamp->day = 01;
        srcTimeStamp->hour = 0;
        srcTimeStamp->minute = 0;
        srcTimeStamp->second = 0;
        switch (SQLDatetimeCode)
        {
        case SQLDTCODE_TIME:
            srcTimeStamp->hour = GetHourFromStr((UCHAR*)srcDataPtr);
            srcTimeStamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
            srcTimeStamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 6);
        case SQLDTCODE_YEAR_TO_HOUR:
            srcTimeStamp->year = GetYearFromStr((UCHAR*)srcDataPtr);
            srcTimeStamp->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
            srcTimeStamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
            srcTimeStamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
            break;
        case SQLDTCODE_YEAR_TO_MINUTE:
            srcTimeStamp->year = GetYearFromStr((UCHAR*)srcDataPtr);
            srcTimeStamp->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
            srcTimeStamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
            srcTimeStamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
            srcTimeStamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 14);
            break;
        case SQLDTCODE_MONTH_TO_HOUR:
            srcTimeStamp->month = GetMonthFromStr((UCHAR*)srcDataPtr);
            srcTimeStamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
            srcTimeStamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
            break;
        case SQLDTCODE_MONTH_TO_MINUTE:
            srcTimeStamp->month = GetMonthFromStr((UCHAR*)srcDataPtr);
            srcTimeStamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
            srcTimeStamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
            srcTimeStamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 9);
            break;
        case SQLDTCODE_MONTH_TO_SECOND:
            srcTimeStamp->month = GetMonthFromStr((UCHAR*)srcDataPtr);
            srcTimeStamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
            srcTimeStamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
            srcTimeStamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 9);
            srcTimeStamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 12);
            break;
        case SQLDTCODE_DAY_TO_HOUR:
            srcTimeStamp->day = GetDayFromStr((UCHAR*)srcDataPtr);
            srcTimeStamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
            break;
        case SQLDTCODE_DAY_TO_MINUTE:
            srcTimeStamp->day = GetDayFromStr((UCHAR*)srcDataPtr);
            srcTimeStamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
            srcTimeStamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 6);
            break;
        case SQLDTCODE_DAY_TO_SECOND:
            srcTimeStamp->day = GetDayFromStr((UCHAR*)srcDataPtr);
            srcTimeStamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
            srcTimeStamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 6);
            srcTimeStamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 9);
            break;
        case SQLDTCODE_MINUTE_TO_SECOND:
            srcTimeStamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
            srcTimeStamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 3);
            break;
        case SQLDTCODE_SECOND:
            srcTimeStamp->second = GetSecondFromStr((UCHAR*)srcDataPtr);
        break;
        default:
            srcTimeStamp->year = GetYearFromStr((UCHAR*)srcDataPtr);
            srcTimeStamp->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
            srcTimeStamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
            srcTimeStamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
            srcTimeStamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 14);
            srcTimeStamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 17);
            break;
        }
    }
    else
    {
        srcTimeStamp = (TIMESTAMP_TYPES *)srcDataPtr;
    }

    if (srcTimeStamp->hour != 0 || srcTimeStamp->minute != 0 || srcTimeStamp->second != 0)
        retCode = IDS_01_S07;

    targetDate->year = srcTimeStamp->year;
    targetDate->month = srcTimeStamp->month;
    targetDate->day = srcTimeStamp->day;

    if (targetStrLenPtr)
        *targetStrLenPtr = sizeof(DATE_STRUCT);

    return SQL_SUCCESS;
}

unsigned long ODBC::ConvSQLTimeToTime(SQLPOINTER srcDataPtr, SQLSMALLINT SQLDatetimeCode, SQLPOINTER targetDataPtr, SQLLEN* targetStrLenPtr, BOOL ColumnwiseData)
{
    TIME_STRUCT*            targetTime = (TIME_STRUCT *)targetDataPtr;
    TIME_TYPES              TimeTmp;
    memset(&TimeTmp, 0,sizeof(TIME_TYPES));
    TIME_TYPES*        srcTime = NULL;
    if (ColumnwiseData) // !RowwiseRowSet
    {
        srcTime = &TimeTmp;
        srcTime->hour = 0;
        srcTime->minute = 0;
        srcTime->second = 0;
        switch (SQLDatetimeCode)
        {
        case SQLDTCODE_HOUR:
            srcTime->hour = GetHourFromStr((UCHAR*)srcDataPtr);
            break;
        case SQLDTCODE_HOUR_TO_MINUTE:
            srcTime->hour = GetHourFromStr((UCHAR*)srcDataPtr);
            srcTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
            break;
        case SQLDTCODE_MINUTE:
            srcTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
            break;
        case SQLDTCODE_MINUTE_TO_SECOND:
            srcTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
            srcTime->second = GetSecondFromStr((UCHAR*)srcDataPtr + 3);
            break;
        case SQLDTCODE_SECOND:
            srcTime->second = GetSecondFromStr((UCHAR*)srcDataPtr);
            break;
        default:
            srcTime->hour = GetHourFromStr((UCHAR*)srcDataPtr);
            srcTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
            srcTime->second = GetSecondFromStr((UCHAR*)srcDataPtr + 6);
        }
    }
    else
    {
        srcTime = (TIME_TYPES *)srcDataPtr;
    }

    targetTime->hour = srcTime->hour;
    targetTime->minute = srcTime->minute;
    targetTime->second = srcTime->second;

    if (targetStrLenPtr)
        *targetStrLenPtr = sizeof(TIME_STRUCT);

    return SQL_SUCCESS;
}


unsigned long ODBC::ConvSQLTimestampToTime(SQLPOINTER srcDataPtr, SQLSMALLINT SQLDatetimeCode, SQLPOINTER targetDataPtr, SQLLEN* targetStrLenPtr, BOOL ColumnwiseData)
{
    TIME_STRUCT*            targetTime = (TIME_STRUCT *)targetDataPtr;
    TIMESTAMP_TYPES              TimestampTmp;
    memset(&TimestampTmp, 0,sizeof(TIMESTAMP_TYPES));
    TIMESTAMP_TYPES*        srcTimestamp = NULL;
    if (ColumnwiseData) // !RowwiseRowSet
    {
        srcTimestamp = &TimestampTmp;
        srcTimestamp->year = 01;
        srcTimestamp->month = 01;
        srcTimestamp->day = 01;
        srcTimestamp->hour = 0;
        srcTimestamp->minute = 0;
        srcTimestamp->second = 0;
        switch (SQLDatetimeCode)
        {
        case SQLDTCODE_YEAR_TO_HOUR:
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
            break;
        case SQLDTCODE_YEAR_TO_MINUTE:
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
            srcTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 14);
            break;
        case SQLDTCODE_MONTH_TO_HOUR:
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
            break;
        case SQLDTCODE_MONTH_TO_MINUTE:
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
            srcTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 9);
            break;
        case SQLDTCODE_MONTH_TO_SECOND:
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
            srcTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 9);
            srcTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 12);
            break;
        case SQLDTCODE_DAY_TO_HOUR:
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
            break;
        case SQLDTCODE_DAY_TO_MINUTE:
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
            srcTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 6);
            break;
        case SQLDTCODE_DAY_TO_SECOND:
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
            srcTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 6);
            srcTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 9);
            break;
        case SQLDTCODE_TIME:
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr);
            srcTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
            srcTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 6);
            break;
        case SQLDTCODE_MINUTE_TO_SECOND:
            srcTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
            srcTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 3);
            break;
        case SQLDTCODE_SECOND:
            srcTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr);
            break;
        default:
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
            srcTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 14);
            srcTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 17);
            break;
        }
    }
    else
    {
        srcTimestamp = (TIMESTAMP_TYPES *)srcDataPtr;
    }
    targetTime->hour = srcTimestamp->hour;
    targetTime->minute = srcTimestamp->minute;
    targetTime->second = srcTimestamp->second;

    if (targetStrLenPtr)
        *targetStrLenPtr = sizeof(TIME_STRUCT);

    return SQL_SUCCESS;
}


unsigned long ODBC::ConvSQLDateToTimestamp(SQLPOINTER srcDataPtr, SQLSMALLINT SQLDatetimeCode, SQLPOINTER targetDataPtr, SQLLEN* targetStrLenPtr, BOOL ColumnwiseData)
{
    TIMESTAMP_STRUCT*            targetTimestamp = (TIMESTAMP_STRUCT *)targetDataPtr;
    DATE_TYPES              DateTmp;
    memset(&DateTmp, 0,sizeof(DATE_TYPES));
    DATE_TYPES*        srcDate = NULL;
    if (ColumnwiseData) // ! RowwiseRowSet
    {
        srcDate = &DateTmp;
        srcDate->year = 01;
        srcDate->month = 01;
        srcDate->day = 01;
        switch (SQLDatetimeCode)
        {
        case SQLDTCODE_YEAR:
            srcDate->year = GetYearFromStr((UCHAR *)srcDataPtr);
            break;
        case SQLDTCODE_YEAR_TO_MONTH:
            srcDate->year = GetYearFromStr((UCHAR *)srcDataPtr);
            srcDate->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
            break;
        case SQLDTCODE_MONTH:
            srcDate->month = GetMonthFromStr((UCHAR*)srcDataPtr);
            break;
        case SQLDTCODE_MONTH_TO_DAY:
            srcDate->month = GetMonthFromStr((UCHAR*)srcDataPtr);
            srcDate->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
            break;
        case SQLDTCODE_DAY:
            srcDate->day = GetDayFromStr((UCHAR *)srcDataPtr);
            break;
        default:
            srcDate->year = GetYearFromStr((UCHAR *)srcDataPtr);
            srcDate->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
            srcDate->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
            break;
        }
    }
    else
    {
        srcDate = (DATE_TYPES *)srcDataPtr;
    }
    targetTimestamp->year = srcDate->year;
    targetTimestamp->month = srcDate->month;
    targetTimestamp->day = srcDate->day;
    targetTimestamp->hour = 0;
    targetTimestamp->minute = 0;
    targetTimestamp->second = 0;
    targetTimestamp->fraction = 0;

    if (targetStrLenPtr)
        *targetStrLenPtr = sizeof(TIMESTAMP_STRUCT);

    return SQL_SUCCESS;
}


unsigned long ODBC::ConvSQLTimeToTimestamp(SQLPOINTER srcDataPtr, SQLSMALLINT SQLDatetimeCode, SQLPOINTER targetDataPtr, SQLLEN* targetStrLenPtr, BOOL ColumnwiseData)
{
    TIMESTAMP_STRUCT*            targetTimestamp = (TIMESTAMP_STRUCT *)targetDataPtr;
    TIME_TYPES              TimeTmp;
    memset(&TimeTmp, 0,sizeof(TIME_TYPES));
    TIME_TYPES*        srcTime = NULL;
    struct tm *newtime;
    time_t long_time;
    if (ColumnwiseData) // !RowwiseRowSet
    {
        srcTime = &TimeTmp;
        srcTime->hour = 0;
        srcTime->minute = 0;
        srcTime->second = 0;
        switch (SQLDatetimeCode)
        {
        case SQLDTCODE_HOUR:
            srcTime->hour = GetHourFromStr((UCHAR*)srcDataPtr);
            break;
        case SQLDTCODE_HOUR_TO_MINUTE:
            srcTime->hour = GetHourFromStr((UCHAR*)srcDataPtr);
            srcTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
            break;
        case SQLDTCODE_MINUTE:
            srcTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
            break;
        case SQLDTCODE_MINUTE_TO_SECOND:
            srcTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
            srcTime->second = GetSecondFromStr((UCHAR*)srcDataPtr + 3);
            break;
        case SQLDTCODE_SECOND:
            srcTime->second = GetSecondFromStr((UCHAR*)srcDataPtr);
            break;
        default:
            srcTime->hour = GetHourFromStr((UCHAR*)srcDataPtr);
            srcTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
            srcTime->second = GetSecondFromStr((UCHAR*)srcDataPtr + 6);
        }
    }
    else
    {
        srcTime = (TIME_TYPES *)srcDataPtr;
    }
    time( &long_time );                     /* Get time as long integer. */
    newtime = localtime( &long_time );      /* Convert to local time. */
    targetTimestamp->year = (short)(newtime->tm_year+1900);
    targetTimestamp->month =(unsigned short)(newtime->tm_mon+1);
    targetTimestamp->day = (unsigned short)newtime->tm_mday;
    targetTimestamp->hour = srcTime->hour;
    targetTimestamp->minute = srcTime->minute;
    targetTimestamp->second = srcTime->second;
    targetTimestamp->fraction = 0;

    if (targetStrLenPtr)
        *targetStrLenPtr = sizeof(TIMESTAMP_STRUCT);

    return SQL_SUCCESS;
}


unsigned long ODBC::ConvSQLTimestampToTimestamp(SQLPOINTER srcDataPtr, CDescRec* srcDescPtr, SQLPOINTER targetDataPtr, SQLLEN* targetStrLenPtr, BOOL ColumnwiseData)
{
    char                    sFraction[10] = {0};
    SQLUINTEGER             ulFraction = 0;
    double                  dTmp = 0;
    TIMESTAMP_STRUCT*            targetTimestamp = (TIMESTAMP_STRUCT *)targetDataPtr;
    TIMESTAMP_TYPES              TimestampTmp;
    memset(&TimestampTmp, 0,sizeof(TIMESTAMP_TYPES));
    TIMESTAMP_TYPES*        srcTimestamp = NULL;
    SQLSMALLINT srcPrecision = srcDescPtr->m_ODBCPrecision;
    if (ColumnwiseData)  // !RowwiseRowSet
    {
        srcTimestamp = &TimestampTmp;
        srcTimestamp->year = 01;
        srcTimestamp->month = 01;
        srcTimestamp->day = 01;
        srcTimestamp->hour = 0;
        srcTimestamp->minute = 0;
        srcTimestamp->second = 0;
        ulFraction = 0;
        switch (srcDescPtr->m_SQLDatetimeCode)
        {
        case SQLDTCODE_TIME:
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr);
            srcTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
            srcTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 6);
            if (srcPrecision > 0)
                ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 9, srcPrecision);
            break;
        case SQLDTCODE_YEAR_TO_HOUR:
            srcTimestamp->year = GetYearFromStr((UCHAR*)srcDataPtr);
            srcTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
            srcTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
            break;
        case SQLDTCODE_YEAR_TO_MINUTE:
            srcTimestamp->year = GetYearFromStr((UCHAR*)srcDataPtr);
            srcTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
            srcTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
            srcTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 14);
            break;
        case SQLDTCODE_MONTH_TO_HOUR:
            srcTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr);
            srcTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);;
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);;
            break;
        case SQLDTCODE_MONTH_TO_MINUTE:
            srcTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr);
            srcTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
            srcTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 9);
            break;
        case SQLDTCODE_MONTH_TO_SECOND:
            srcTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr);
            srcTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
            srcTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 9);
            srcTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 12);
            if (srcPrecision > 0)
                ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 15, srcPrecision);
            break;
        case SQLDTCODE_DAY_TO_HOUR:
            srcTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr);
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
            break;
        case SQLDTCODE_DAY_TO_MINUTE:
            srcTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr);
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
            srcTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 6);
            break;
        case SQLDTCODE_DAY_TO_SECOND:
            srcTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr);
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
            srcTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 6);
            srcTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 9);
            if (srcPrecision > 0)
                ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 12, srcPrecision);
            break;
        case SQLDTCODE_MINUTE_TO_SECOND:
            srcTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
            srcTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 3);
            if (srcPrecision > 0)
                ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 6, srcPrecision);
            break;
        case SQLDTCODE_SECOND:
            srcTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr);
            if (srcPrecision > 0)
                ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 3, srcPrecision);
            break;
        default:
            srcTimestamp->year = GetYearFromStr((UCHAR*)srcDataPtr);
            srcTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
            srcTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
            srcTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
            srcTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 14);
            srcTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 17);
            if (srcPrecision > 6)
            {
                ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 20, srcPrecision);
                sprintf(sFraction , "%0*lu000000000", 6 , ulFraction );
                sFraction[9] = 0;
                ulFraction = atol(sFraction);
            }
            else
            {
                if (srcPrecision > 0)
                    ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 20, srcPrecision);
            }
            break;
        }
    }
    else
    {
        srcTimestamp = (TIMESTAMP_TYPES *)srcDataPtr;
        if (srcPrecision > 0)
        {
            // SQL returns fraction of a second which has to be converted to nano seconds
            dTmp = (*(UDWORD_P*)srcTimestamp->fraction *  1000000000.0) / pow(10,srcPrecision);
            ulFraction = dTmp;

        }
        else
            ulFraction = 0; 
    }
    targetTimestamp->year = srcTimestamp->year;
    targetTimestamp->month = srcTimestamp->month;
    targetTimestamp->day = srcTimestamp->day;
    targetTimestamp->hour = srcTimestamp->hour;
    targetTimestamp->minute = srcTimestamp->minute;
    targetTimestamp->second = srcTimestamp->second;
    if (srcPrecision > 0)
    {
        targetTimestamp->fraction = ulFraction;
    }
    else
        targetTimestamp->fraction = 0;

    if (targetStrLenPtr)
        *targetStrLenPtr = sizeof(TIMESTAMP_STRUCT);

    return SQL_SUCCESS;
}


unsigned long ODBC::ConvSQLCharToNumeric(SQLPOINTER srcDataPtr, CDescRec* srcDescPtr, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLLEN* targetStrLenPtr, ICUConverter* iconv, UCHAR* errorMsg)
{
    int             TransStringLength   = 0;
    unsigned long   retCode             = SQL_SUCCESS;
    char*           cTmpBuf             = NULL;
    SQLINTEGER      tmpLen              = 0;
    char*           error               = NULL;
    __int64         tempVal64           = 0;
    SQLINTEGER      DataLen             = 0;
    retCode = GetCTmpBufFromSQLChar(srcDataPtr, srcLength, srcDescPtr->m_ODBCDataType, srcDescPtr->m_SQLCharset, srcDescPtr->m_SQLMaxLength <= MAXCHARLEN, cTmpBuf, tmpLen, iconv, errorMsg, true);
    if (retCode != SQL_SUCCESS)
    {
        if(cTmpBuf)
        {
            free(cTmpBuf);
            cTmpBuf = NULL;
        }
        return retCode;
    }

    retCode = ConvertCharToCNumeric((SQL_NUMERIC_STRUCT *)targetDataPtr, cTmpBuf);

    if(cTmpBuf)
    {
        free(cTmpBuf);
        cTmpBuf = NULL;
    }

    if (targetStrLenPtr)
        *targetStrLenPtr = sizeof(SQL_NUMERIC_STRUCT);

    return retCode;
}


unsigned long ODBC::ConvertCharToCNumeric(SQL_NUMERIC_STRUCT* numericTmp, CHAR* cTmpBuf)
{
    unsigned char localBuf[NUM_LEN_MAX];
    char* tempPtr = (char*)localBuf,*tempPtr1;
    int i,j,a,b,current,calc,length;

    SQLCHAR tempPrecision;
    SQLCHAR tempScale;
    SQLCHAR tempSign;
    SQLCHAR tmpVal[NUM_LEN_MAX];

    if(strlen(rTrim(cTmpBuf)) > sizeof(tmpVal))
        return IDS_22_003;

    memset( tmpVal, 0, sizeof(tmpVal));
    memset( localBuf, 0, sizeof(localBuf));

    length = strlen(strncpy( tempPtr, cTmpBuf, sizeof(localBuf)-1 ));
    if( tempPtr[ length - 1 ] == '.' ) tempPtr[ length - 1 ] = '\0';

    tempSign = (*tempPtr == '-')? 2: 1;

    if( *tempPtr == '+' || *tempPtr == '-' ) tempPtr++;

    if((tempPtr1 = strchr( tempPtr, '.' )) == NULL )
    {
        tempPrecision = strlen(tempPtr);
        tempScale = 0;
    }
    else
    {
        tempPrecision = strlen(tempPtr) - 1;
        tempScale = strlen(tempPtr1) - 1;
    }

    if( tempPrecision > ENDIAN_PRECISION_MAX )
        return IDS_22_003;

    for( length = 0, tempPtr1 = (char*)localBuf ;*tempPtr != 0; tempPtr++ )
    {
        if(*tempPtr == '.') continue;
        *tempPtr1++ = *tempPtr - '0';
        length++;
    }
    memset( tempPtr1, 0, sizeof(localBuf) - length );

    for( j=0; j < 2 * sizeof(tmpVal); j++)
    {
        a=b=calc=0;

        for( i=0; i < length ; i++)
        {
            current = localBuf[i];
            calc = calc * 10 + current;
            a = calc % 16;
            b = calc / 16;

            localBuf[i] =  b;
            calc = a;
        }
        switch( j % 2 )
        {
        case 0:
            tmpVal[j / 2 ] = a;
            break;
        case 1:
            tmpVal[j / 2 ] |= a<<4;
            break;
        }
    }

    for( i= sizeof(tmpVal) - 1; i > SQL_MAX_NUMERIC_LEN - 1; i--)
        if(tmpVal[i] != 0)
            return IDS_22_003;

    numericTmp->sign = tempSign;
    numericTmp->precision = tempPrecision;
    numericTmp->scale = tempScale;
    memcpy( numericTmp->val, tmpVal, SQL_MAX_NUMERIC_LEN);

    return SQL_SUCCESS;
}


unsigned long ODBC::ConvSQLNumericToNumeric(SQLPOINTER srcDataPtr, CDescRec* srcDescPtr, SQLINTEGER srcLength, SQLPOINTER targetDataPtr, SQLLEN* targetStrLenPtr)
{
    unsigned long   retCode = SQL_SUCCESS;
    char            cTmpBuf[NUM_LEN_MAX]    = {0};
    SQLLEN          cTmpBufRetLen           = 0;
    retCode = ConvSQLNumericToChar( srcDataPtr, srcDescPtr, srcLength, SQL_C_CHAR, cTmpBuf, NUM_LEN_MAX, &cTmpBufRetLen);
    if (retCode != SQL_SUCCESS)
        return retCode;
    retCode = ConvertCharToCNumeric((SQL_NUMERIC_STRUCT *)targetDataPtr, cTmpBuf);

    if (targetStrLenPtr)
        *targetStrLenPtr = sizeof(SQL_NUMERIC_STRUCT);

    return retCode;
}


unsigned long ODBC::ConvertSQLCharToInterval(SQLSMALLINT ODBCDataType, 
                        SQLPOINTER srcDataPtr,
                        SQLINTEGER    srcLength,
                        SQLSMALLINT CDataType,
                        SQLPOINTER outValue)
{
    unsigned long   retCode = SQL_SUCCESS;
    char            in_value[128];
    char            temp_value[128];
    unsigned long   interval_parts[5];
    short           sign = 0;
    char*           token;
    short           i;
    SQLUINTEGER     fraction_part = 0;
    char            delimiters[6];
    short           len;
    char*           strPtr;
    char*           pdest;
    char*           saveptr =   NULL;

    switch (ODBCDataType)
    {
    case SQL_CHAR:
    case SQL_WCHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
    case SQL_WVARCHAR:
        len = srcLength-1;
        strPtr = (char *)srcDataPtr;
        if (strPtr[ len - 1 ] == ' ')
        {
            strPtr[ len - 1 ] = 0;
            rTrim( strPtr);
            len = strlen(strPtr);
        }
        break;
    case SQL_INTERVAL_MONTH:
    case SQL_INTERVAL_YEAR:
    case SQL_INTERVAL_YEAR_TO_MONTH:
    case SQL_INTERVAL_DAY:
    case SQL_INTERVAL_HOUR:
    case SQL_INTERVAL_MINUTE:
    case SQL_INTERVAL_SECOND:
    case SQL_INTERVAL_DAY_TO_HOUR:
    case SQL_INTERVAL_DAY_TO_MINUTE:
    case SQL_INTERVAL_DAY_TO_SECOND:
    case SQL_INTERVAL_HOUR_TO_MINUTE:
    case SQL_INTERVAL_HOUR_TO_SECOND:
    case SQL_INTERVAL_MINUTE_TO_SECOND:
        strPtr = trimInterval((char *)srcDataPtr, srcLength);
        len = strlen(strPtr);
        break;
    default:
        return IDS_07_006;
    }


    if (len >= sizeof(in_value))
        len = sizeof(in_value)-1;
    strncpy(in_value, strPtr, len);
    in_value[len] = '\0';
    if (len != (short)strspn(in_value, "1234567890:.- "))
       return IDS_22_018;
    for (i = 0 ; i < 5 ; i++)
       interval_parts[i] = 0;

    ((SQL_INTERVAL_STRUCT *)outValue)->interval_sign = 0;
    ((SQL_INTERVAL_STRUCT *)outValue)->intval.year_month.year = 0;
    ((SQL_INTERVAL_STRUCT *)outValue)->intval.year_month.month = 0;
    ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = 0;
    ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = 0;
    ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = 0;
    ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = 0;
    ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = 0;

    strcpy(temp_value,in_value);
    trim(temp_value);

    
    if (temp_value[0] == '-')
    {
        ((SQL_INTERVAL_STRUCT *)outValue)->interval_sign = 1;
        pdest = strchr(in_value,'-');
        strcpy(in_value,pdest+1);
    }

    delimiters[0] = '\0';
    if (strchr(in_value,'-') != NULL)
        strcat(delimiters, "-");
    if (strchr(in_value,':') != NULL)
        strcat(delimiters, ":");
    if (strchr(in_value,'.') != NULL)
        strcat(delimiters, ".");
    if (strchr(in_value,' ') != NULL)
        strcat(delimiters, " ");

    i = 0;
#ifndef unixcli
    token = strtok(in_value, delimiters );
#else
    token = strtok_r(in_value, delimiters,&saveptr );
#endif
    while( token != NULL )
    {
        interval_parts[i] = (unsigned long)atol(token);
#ifndef unixcli
        token = strtok( NULL, delimiters );
#else
        token = strtok_r( NULL, delimiters, &saveptr );
#endif
        i++;
    }
    if (i > 5)
       return IDS_22_018;

    switch (ODBCDataType)
    {
    case SQL_INTERVAL_YEAR:
        if (i > 1)
           return IDS_22_018;
        switch (CDataType)
        {
        case SQL_C_INTERVAL_YEAR:
        case SQL_C_DEFAULT:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_YEAR;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.year_month.year = interval_parts[0];
            break;
        case SQL_C_INTERVAL_YEAR_TO_MONTH:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_YEAR_TO_MONTH;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.year_month.year = interval_parts[0];
            break;
        default:
            return IDS_07_006;
        }
        break;
    case SQL_INTERVAL_MONTH:
        if (i > 1)
           return IDS_22_018;
        switch (CDataType)
        {
        case SQL_C_INTERVAL_MONTH:
        case SQL_C_DEFAULT:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MONTH;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.year_month.month = interval_parts[0];
            break;
        case SQL_C_INTERVAL_YEAR_TO_MONTH:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_YEAR_TO_MONTH;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.year_month.month = interval_parts[0];
            break;
        default:
            return IDS_07_006;
        }
        break;
    case SQL_INTERVAL_YEAR_TO_MONTH:
        if (i > 2)
           return IDS_22_018;
        switch (CDataType)
        {
        case SQL_C_INTERVAL_YEAR:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_YEAR;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.year_month.year = interval_parts[0];
            if (interval_parts[1] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_MONTH:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MONTH;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.year_month.month = interval_parts[1];
            if (interval_parts[1] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_YEAR_TO_MONTH:
        case SQL_C_DEFAULT:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_YEAR_TO_MONTH;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.year_month.year = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.year_month.month = interval_parts[1];
            break;
        default:
            return IDS_07_006;
        }
        break;
    case SQL_INTERVAL_DAY:
        if (i > 1)
           return IDS_22_018;
        switch (CDataType)
        {
        case SQL_C_INTERVAL_DAY:
        case SQL_C_DEFAULT:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
            break;
        case SQL_C_INTERVAL_DAY_TO_HOUR:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_HOUR;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
            break;
        case SQL_C_INTERVAL_DAY_TO_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
            break;
        case SQL_C_INTERVAL_DAY_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
            break;
        default:
            return IDS_07_006;
        }
        break;
    case SQL_INTERVAL_HOUR:
        if (i > 1)
           return IDS_22_018;
        switch (CDataType)
        {
        case SQL_C_INTERVAL_HOUR:
        case SQL_C_DEFAULT:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            break;
        case SQL_C_INTERVAL_HOUR_TO_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            break;
        case SQL_C_INTERVAL_HOUR_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            break;
        case SQL_C_INTERVAL_DAY_TO_HOUR:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_HOUR;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            break;
        case SQL_C_INTERVAL_DAY_TO_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            break;
        case SQL_C_INTERVAL_DAY_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            break;
        default:
            return IDS_07_006;
        }
        break;
    case SQL_INTERVAL_MINUTE:
        if (i > 1)
           return IDS_22_018;
        switch (CDataType)
        {
        case SQL_C_INTERVAL_MINUTE:
        case SQL_C_DEFAULT:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[0];
            break;
        case SQL_C_INTERVAL_DAY_TO_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[0];
            break;
        case SQL_C_INTERVAL_DAY_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[0];
            break;
        case SQL_C_INTERVAL_HOUR_TO_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[0];
            break;
        case SQL_C_INTERVAL_HOUR_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[0];
            break;
        case SQL_C_INTERVAL_MINUTE_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[0];
            break;
        default:
            return IDS_07_006;
        }
        break;
    case SQL_INTERVAL_SECOND:
        if (i > 2)
           return IDS_22_018;
        switch (CDataType)
        {
        case SQL_C_INTERVAL_SECOND:
        case SQL_C_DEFAULT:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[1];
            break;
        case SQL_C_INTERVAL_DAY_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[1];
            break;
        case SQL_C_INTERVAL_HOUR_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[1];
            break;
        case SQL_C_INTERVAL_MINUTE_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[1];
            break;
        default:
            return IDS_07_006;
        }
        break;
    case SQL_INTERVAL_DAY_TO_HOUR:
        if (i > 2)
           return IDS_22_018;
        switch (CDataType)
        {
        case SQL_C_INTERVAL_DAY:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
            if (interval_parts[1] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_HOUR:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            if (interval_parts[0] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_DAY_TO_HOUR:
        case SQL_C_DEFAULT:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_HOUR;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            break;
        case SQL_C_INTERVAL_DAY_TO_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            break;
        case SQL_C_INTERVAL_DAY_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            break;
        case SQL_C_INTERVAL_HOUR_TO_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            if (interval_parts[0] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_HOUR_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            if (interval_parts[0] != 0)
                return IDS_01_S07;
            break;
        default:
            return IDS_07_006;
        }
        break;
    case SQL_INTERVAL_DAY_TO_MINUTE:
        if (i > 3)
           return IDS_22_018;
        switch (CDataType)
        {
        case SQL_C_INTERVAL_DAY:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
            if (interval_parts[1] != 0 || interval_parts[2] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_HOUR:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            if (interval_parts[0] != 0 || interval_parts[2] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[2];
            if (interval_parts[0] != 0 || interval_parts[1] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_DAY_TO_HOUR:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_HOUR;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            if (interval_parts[2] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_DAY_TO_MINUTE:
        case SQL_C_DEFAULT:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[2];
            break;
        case SQL_C_INTERVAL_DAY_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[2];
            break;
        case SQL_C_INTERVAL_HOUR_TO_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[2];
            if (interval_parts[0] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_HOUR_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[2];
            if (interval_parts[0] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_MINUTE_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[2];
            if (interval_parts[0] != 0 || interval_parts[1] != 0)
                return IDS_01_S07;
            break;
        default:
            return IDS_07_006;
        }
        break;
    case SQL_INTERVAL_DAY_TO_SECOND:
        if (i > 5)
           return IDS_22_018;
        switch (CDataType)
        {
        case SQL_C_INTERVAL_DAY:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
            if (interval_parts[1] != 0 || interval_parts[2] != 0 || interval_parts[3] != 0 || interval_parts[4] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_HOUR:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            if (interval_parts[0] != 0 || interval_parts[2] != 0 || interval_parts[3] != 0 || interval_parts[4] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[2];
            if (interval_parts[0] != 0 || interval_parts[1] != 0 || interval_parts[3] != 0 || interval_parts[4] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[3];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[4];
            if (interval_parts[0] != 0 || interval_parts[1] != 0 || interval_parts[2] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_DAY_TO_HOUR:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_HOUR;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            if (interval_parts[2] != 0 || interval_parts[3] != 0 || interval_parts[4] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_DAY_TO_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[2];
            if (interval_parts[3] != 0 || interval_parts[4] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_DAY_TO_SECOND:
        case SQL_C_DEFAULT:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[2];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[3];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[4];
            break;
        case SQL_C_INTERVAL_HOUR_TO_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[2];
            if (interval_parts[0] != 0 || interval_parts[3] != 0 || interval_parts[4] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_HOUR_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[2];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[3];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[4];
            if (interval_parts[0] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_MINUTE_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[2];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[3];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[4];
            if (interval_parts[0] != 0 || interval_parts[1] != 0)
                return IDS_01_S07;
            break;
        default:
            return IDS_07_006;
        }
        break;
    case SQL_INTERVAL_HOUR_TO_MINUTE:
        if (i > 2)
           return IDS_22_018;
        switch (CDataType)
        {
        case SQL_C_INTERVAL_HOUR:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            if (interval_parts[1] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
            if (interval_parts[0] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_DAY_TO_HOUR:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_HOUR;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            if (interval_parts[1] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_DAY_TO_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[1];
            break;
        case SQL_C_INTERVAL_DAY_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[1];
            break;
        case SQL_C_INTERVAL_HOUR_TO_MINUTE:
        case SQL_C_DEFAULT:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[1];
            break;
        case SQL_C_INTERVAL_HOUR_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[1];
            break;
        case SQL_C_INTERVAL_MINUTE_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[1];
            if (interval_parts[0] != 0)
                return IDS_01_S07;
            break;
        default:
            return IDS_07_006;
        }
        break;
    case SQL_INTERVAL_HOUR_TO_SECOND:
        if (i > 4)
           return IDS_22_018;
        switch (CDataType)
        {
        case SQL_C_INTERVAL_HOUR:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            if (interval_parts[1] != 0 || interval_parts[2] != 0 || interval_parts[3] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[1];
            if (interval_parts[0] != 0 || interval_parts[2] != 0 || interval_parts[3] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[2];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[3];
            if (interval_parts[0] != 0 || interval_parts[1] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_DAY_TO_HOUR:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_HOUR;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            if (interval_parts[1] != 0 || interval_parts[2] != 0 || interval_parts[3] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_DAY_TO_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[1];
            if (interval_parts[2] != 0 || interval_parts[3] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_DAY_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[1];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[2];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[3];
            break;
        case SQL_C_INTERVAL_HOUR_TO_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[1];
            if (interval_parts[2] != 0 || interval_parts[3] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_HOUR_TO_SECOND:
        case SQL_C_DEFAULT:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[1];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[2];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[3];
            break;
        case SQL_C_INTERVAL_MINUTE_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[1];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[2];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[3];
            if (interval_parts[0] != 0)
                return IDS_01_S07;
            break;
        default:
            return IDS_07_006;
        }
        break;
    case SQL_INTERVAL_MINUTE_TO_SECOND:
        if (i > 3)
           return IDS_22_018;
        switch (CDataType)
        {
        case SQL_C_INTERVAL_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[0];
            if (interval_parts[1] != 0 || interval_parts[2] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[1];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[2];
            if (interval_parts[0] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_DAY_TO_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[0];
            if (interval_parts[1] != 0 || interval_parts[2] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_DAY_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[1];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[2];
            break;
        case SQL_C_INTERVAL_HOUR_TO_MINUTE:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_MINUTE;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[0];
            if (interval_parts[1] != 0 || interval_parts[2] != 0)
                return IDS_01_S07;
            break;
        case SQL_C_INTERVAL_HOUR_TO_SECOND:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[1];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[2];
            break;
        case SQL_C_INTERVAL_MINUTE_TO_SECOND:
        case SQL_C_DEFAULT:
            ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE_TO_SECOND;
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[0];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[1];
            ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[2];
            break;
        default:
            return IDS_07_006;
        }
        break;
    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
    case SQL_WCHAR:
    case SQL_WVARCHAR:
        switch (i)
        {
        case 1:
            switch (CDataType)
            {
            case SQL_C_INTERVAL_YEAR:
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_YEAR;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.year_month.year = interval_parts[0];
                break;
            case SQL_C_INTERVAL_MONTH:
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MONTH;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.year_month.month = interval_parts[0];
                break;
            case SQL_C_INTERVAL_DAY:
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
                break;
            case SQL_C_INTERVAL_HOUR:
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
                break;
            case SQL_C_INTERVAL_MINUTE:
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[0];
                break;
            case SQL_C_INTERVAL_SECOND:
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[0];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = 0;
                break;
            default:
                return IDS_07_006;
            }
            break;
        case 2:
            switch (CDataType)
            {
            case SQL_C_INTERVAL_YEAR_TO_MONTH:
                if(strcmp(delimiters,"-") != 0)
                   return IDS_22_018;
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_YEAR_TO_MONTH;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.year_month.year = interval_parts[0];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.year_month.month = interval_parts[1];
                break;
            case SQL_C_INTERVAL_SECOND:
                if(strcmp(delimiters,".") != 0)
                   return IDS_22_018;
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_SECOND;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[0];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[1];
                break;
            case SQL_C_INTERVAL_DAY_TO_HOUR:
                if(strcmp(delimiters," ") != 0)
                   return IDS_22_018;
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_HOUR;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
                break;
            case SQL_C_INTERVAL_HOUR_TO_MINUTE:
                if(strcmp(delimiters,":") != 0)
                   return IDS_22_018;
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_MINUTE;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[1];
                break;
            case SQL_C_INTERVAL_MINUTE_TO_SECOND:
                if(strcmp(delimiters,":") != 0)
                   return IDS_22_018;
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE_TO_SECOND;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[0];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[1];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = 0;
                break;
            default:
                return IDS_07_006;
            }
            break;
        case 3:
            switch (CDataType)
            {
            case SQL_C_INTERVAL_DAY_TO_MINUTE:
                if(strcmp(delimiters,": ") != 0)
                   return IDS_22_018;
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_MINUTE;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[2];
                break;
            case SQL_C_INTERVAL_HOUR_TO_SECOND:
                if(strcmp(delimiters,":") != 0)
                   return IDS_22_018;
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_SECOND;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[1];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[2];
                break;
            case SQL_C_INTERVAL_MINUTE_TO_SECOND:
                if(strcmp(delimiters,":.") != 0)
                   return IDS_22_018;
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_MINUTE_TO_SECOND;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[0];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[1];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[2];
                break;
            default:
                return IDS_07_006;
            }
            break;
        case 4:
            switch (CDataType)
            {
            case SQL_C_INTERVAL_DAY_TO_SECOND:
                if(strcmp(delimiters,": ") != 0)
                   return IDS_22_018;
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_SECOND;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[2];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[3];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = 0;
                break;
            case SQL_C_INTERVAL_HOUR_TO_SECOND:
                if(strcmp(delimiters,":.") != 0)
                   return IDS_22_018;
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_HOUR_TO_SECOND;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[0];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[1];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[2];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[3];
                break;
            default:
                return IDS_07_006;
            }
            break;
        case 5:
            switch (CDataType)
            {
            case SQL_C_INTERVAL_DAY_TO_SECOND:
                if(strcmp(delimiters,":. ") != 0)
                   return IDS_22_018;
                ((SQL_INTERVAL_STRUCT *)outValue)->interval_type = SQL_IS_DAY_TO_SECOND;
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.day = interval_parts[0];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.hour = interval_parts[1];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.minute = interval_parts[2];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.second = interval_parts[3];
                ((SQL_INTERVAL_STRUCT *)outValue)->intval.day_second.fraction = interval_parts[4];
                break;
            default:
                return IDS_07_006;
            }
            break;
        default:
            return IDS_07_006;
        }
        break;
    default:
        return IDS_07_006;
    }
    return 0;
}


unsigned long  ODBC::ConvDoubleToInterval(double dTmp, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLSMALLINT CDataType, SQLLEN* targetStrLenPtr)
{
    SQL_INTERVAL_STRUCT intervalTmp;
    ULONG_P             ulTmp   = 0;
    unsigned long       retCode = SQL_SUCCESS;

    if (targetLength < sizeof(SQL_INTERVAL_STRUCT))
        return IDS_22_003;

    if (dTmp < 0)
        intervalTmp.interval_sign = 1;
    else
        intervalTmp.interval_sign = 0;
    ulTmp = (ULONG_P)dTmp;
    if (dTmp != ulTmp)
        retCode = IDS_01_S07;

    switch (CDataType)
    {
    case SQL_C_INTERVAL_MONTH:
        intervalTmp.interval_type = SQL_IS_MONTH;
        intervalTmp.intval.year_month.month = ulTmp;
        intervalTmp.intval.year_month.year = 0;
        break;
    case SQL_C_INTERVAL_YEAR:
        intervalTmp.interval_type = SQL_IS_YEAR;
        intervalTmp.intval.year_month.month = 0;
        intervalTmp.intval.year_month.year = ulTmp;
        break;
    case SQL_C_INTERVAL_DAY:
        intervalTmp.interval_type = SQL_IS_DAY;
        intervalTmp.intval.day_second.day = ulTmp;
        intervalTmp.intval.day_second.hour = 0;
        intervalTmp.intval.day_second.minute = 0;
        intervalTmp.intval.day_second.second = 0;
        intervalTmp.intval.day_second.fraction = 0;
        break;
    case SQL_C_INTERVAL_HOUR:
        intervalTmp.interval_type = SQL_IS_HOUR;
        intervalTmp.intval.day_second.day = 0;
        intervalTmp.intval.day_second.hour = ulTmp;
        intervalTmp.intval.day_second.minute = 0;
        intervalTmp.intval.day_second.second = 0;
        intervalTmp.intval.day_second.fraction = 0;
        break;
    case SQL_C_INTERVAL_MINUTE:
        intervalTmp.interval_type = SQL_IS_MINUTE;
        intervalTmp.intval.day_second.day = 0;
        intervalTmp.intval.day_second.hour = 0;
        intervalTmp.intval.day_second.minute = ulTmp;
        intervalTmp.intval.day_second.second = 0;
        intervalTmp.intval.day_second.fraction = 0;
        break;
    case SQL_C_INTERVAL_SECOND:
        intervalTmp.interval_type = SQL_IS_SECOND;
        intervalTmp.intval.day_second.day = 0;
        intervalTmp.intval.day_second.hour = 0;
        intervalTmp.intval.day_second.minute = 0;
        intervalTmp.intval.day_second.second = ulTmp;
        intervalTmp.intval.day_second.fraction = 0;
        break;
    default:
        return IDS_07_006;
    }

    memcpy(targetDataPtr, &intervalTmp, sizeof(SQL_INTERVAL_STRUCT));

    if (targetStrLenPtr)
        *targetStrLenPtr = sizeof(SQL_INTERVAL_STRUCT);
    return retCode;
}

unsigned long ODBC::GetCTmpBufFromSQLChar(SQLPOINTER srcDataPtr, SQLINTEGER srcLength, SQLSMALLINT ODBCDataType, SQLINTEGER srcCharSet, bool isshort, char*& cTmpBuf, SQLINTEGER &tmpLen, ICUConverter* iconv, UCHAR* errorMsg, bool RemoveSpace)
{
    int         TransStringLength   = 0;
    char        error[64]           = {0};
    SQLINTEGER  DataLen             = 0;

    if (ODBCDataType == SQL_CHAR || ODBCDataType == SQL_WCHAR)
    {
        if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to locale
        {
            UChar* spaceStart = u_strchr((UChar *)srcDataPtr, L' ');
            if (spaceStart != NULL && RemoveSpace)
                srcLength = (spaceStart - (UChar *)srcDataPtr) + 1;
            cTmpBuf = (char *)malloc(srcLength + 1);
            memset(cTmpBuf, 0 , srcLength);
            if (cTmpBuf == NULL) //Avoid a seg-violation
                return IDS_07_003;
            if(iconv->WCharToUTF8((UChar*)srcDataPtr, srcLength-1, (char*)cTmpBuf,
                srcLength + 1, &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
            {
                return IDS_07_003;
            }
            srcLength = strlen(cTmpBuf) + 1;//TransStringLength + 1;
        }
        else
        {
            char* spaceStart = strchr((char *)srcDataPtr, ' ');
            if (spaceStart != NULL && RemoveSpace)
                srcLength = (spaceStart - (char *)srcDataPtr) + 1;
            cTmpBuf = (char *)malloc(srcLength + 1);
            memset(cTmpBuf, 0, srcLength + 1);
            if (cTmpBuf == NULL) //Avoid a seg-violation
                return IDS_07_003;
            strncpy(cTmpBuf, (char *)srcDataPtr, srcLength);
            cTmpBuf[srcLength - 1] = 0;
        }
    }
    else //SQL_LONG_VARCHAR
    {
        if(isshort)
        {
            DataLen = *(USHORT *)srcDataPtr;
            if (DataLen == 0)
                return SQL_NO_DATA;
            if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to UTF-8
            {
                if (DataLen % 2 == 0)
                    srcLength = DataLen / 2 + 1;
                else
                    srcLength = DataLen / 2 + 2;
                cTmpBuf = (char *)malloc(srcLength);
                memset(cTmpBuf, 0 ,DataLen/2 + 1);
                if (cTmpBuf == NULL) //Avoid a seg-violation
                    return IDS_07_003;
                if(iconv->WCharToUTF8((UChar*)srcDataPtr+1, DataLen/2, (char*)cTmpBuf,
                    srcLength, &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
                {
                //We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
                    return IDS_07_003;
                }
                srcLength = TransStringLength + 1;
            }
            else
            {
                cTmpBuf = (char *)malloc(DataLen + 1);
                memset(cTmpBuf, 0, DataLen + 1);
                if (cTmpBuf == NULL) //Avoid a seg-violation
                    return IDS_07_003;
                memcpy(cTmpBuf, (char*)srcDataPtr+2, DataLen);
                srcLength = DataLen + 1;
                cTmpBuf[DataLen] = 0;
            }
        }
        else
        {
            DataLen = *(UINT *)srcDataPtr;
            if (DataLen == 0)
            return SQL_NO_DATA;
            if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to UTF-8
            {
                if (DataLen % 2 == 0)
                    srcLength = DataLen / 2 + 1;
                else
                    srcLength = DataLen / 2 + 2;
                cTmpBuf = (char *)malloc(srcLength);
                memset(cTmpBuf, 0 ,DataLen/2 + 1);
                if (cTmpBuf == NULL) //Avoid a seg-violation
                    return IDS_07_003;
                if(iconv->WCharToUTF8((UChar*)srcDataPtr+2, DataLen/2, (char*)cTmpBuf,
                    DataLen/2 + 1, &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
                {
                //We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
                    return IDS_07_003;
                }
                srcLength = TransStringLength + 1;
            }
            else
            {
                cTmpBuf = (char *)malloc(DataLen + 1);
                memset(cTmpBuf, 0, DataLen + 1);
                if (cTmpBuf == NULL) //Avoid a seg-violation
                    return IDS_07_003;
                memcpy(cTmpBuf, (char*)srcDataPtr+4, DataLen);
                srcLength = DataLen + 1;
                cTmpBuf[DataLen] = 0;
            }
        }
    }

    tmpLen = srcLength;

    return SQL_SUCCESS;
}


unsigned short ODBC::ConvToInt(UCHAR* ptr,int len)
{
    char buf[10];
    if (len > sizeof(buf) || len < 1)
        return 0;
    memset(buf,0,10);
    memcpy(buf,ptr,len);
    return atoi(buf);
}


SWORD ODBC::GetYearFromStr(UCHAR* ptr)
{
    return ConvToInt(ptr,4);
}
UCHAR ODBC::GetMonthFromStr(UCHAR* ptr)
{
    return ConvToInt(ptr,2);
}
UCHAR ODBC::GetDayFromStr(UCHAR* ptr)
{
    return ConvToInt(ptr,2);
}
UCHAR ODBC::GetHourFromStr(UCHAR* ptr)
{
    return ConvToInt(ptr,2);
}
UCHAR ODBC::GetMinuteFromStr(UCHAR* ptr)
{
    return ConvToInt(ptr,2);
}
UCHAR ODBC::GetSecondFromStr(UCHAR* ptr)
{
    return ConvToInt(ptr,2);
}
UDWORD_P ODBC::GetFractionFromStr(UCHAR* ptr, short precision)
{
    char* lptr = (char*)ptr;
    int length;
    char buf[10];
    if (precision < 1)
        return 0;
    rTrim(lptr);
    length = strlen(lptr) > precision? precision: strlen(lptr);
    memset(buf, '0', sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;
    memcpy(buf, lptr, length);
    return atol(buf);

}
