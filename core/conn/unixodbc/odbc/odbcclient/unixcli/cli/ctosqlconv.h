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
********************************************************************/
/**************************************************************************
**************************************************************************/
//
//
#ifndef _CTOSQLCONV_DEFINED
#define _CTOSQLCONV_DEFINED
#ifdef unixcli
#include "unix_extra.h"
#endif
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include "drvrglobal.h"
#include "charsetconv.h"
#include "cdesc.h"

// interval datatypes
#define REC_MIN_INTERVAL        195
#define REC_INT_YEAR            195
#define REC_INT_MONTH           196
#define REC_INT_YEAR_MONTH      197
#define REC_INT_DAY             198
#define REC_INT_HOUR            199
#define REC_INT_DAY_HOUR        200
#define REC_INT_MINUTE          201
#define REC_INT_HOUR_MINUTE     202
#define REC_INT_DAY_MINUTE      203
#define REC_INT_SECOND          204
#define REC_INT_MINUTE_SECOND   205
#define REC_INT_HOUR_SECOND     206
#define REC_INT_DAY_SECOND      207
#define REC_MAX_INTERVAL        208
#define TMPLEN                  256
#define CHARTMPLEN              311

namespace ODBC {

unsigned long ConvertCToSQL(SQLINTEGER	ODBCAppVersion,
							SQLSMALLINT	CDataType,
							SQLPOINTER	srcDataPtr,
							SQLINTEGER	srcLength,
							SQLPOINTER	targetDataPtr,
                            CDescRec    *targetDescPtr,
							BOOL		byteSwap,
#ifdef unixcli
							ICUConverter* iconv,
#else
							FPSQLDriverToDataSource fpSQLDriverToDataSource = NULL,
							DWORD		translateOption = 0,
#endif
							UCHAR		*errorMsg = NULL,
							SWORD		errorMsgMax = 0,
							SQLINTEGER	EnvironmentType = NSK_BUILD_1,
							BOOL		RWRSFormat = 0,
							SQLINTEGER datetimeIntervalPrecision = 0);


unsigned long convToSQLBool(SQLPOINTER srcDataPtr, SQLINTEGER    srcLength,SQLSMALLINT CDataType, SQLPOINTER targetDataPtr);

unsigned long   MemcpyToNumeric(SQLPOINTER  DataPtr,
            SQLINTEGER &    DataLen,
            CDescRec*       targetDescPtr,
            SQLSMALLINT     CDataType,
            BOOL            useDouble,
            double          dTmp,
            BOOL            negative,
            __int64         decimalPart,
            __int64         integralPart,
            long            leadZeros,
            ICUConverter*   iconv,
            SQLPOINTER &    outDataPtr,
            unsigned long   retTmp);


unsigned long  ConvertToNumeric(SQLINTEGER      ODBCAppVersion,
        SQLSMALLINT   CDataType,
        SQLPOINTER    srcDataPtr,
        SQLINTEGER    srcLength,
        CDescRec*     targetDescPtr,
        ICUConverter* iconv,
        SQLPOINTER    targetDataPtr, 
        UCHAR         *errorMsg);


unsigned long  ConvertToNumberSimple(SQLSMALLINT   CDataType,
                    SQLPOINTER    srcDataPtr,
                    SQLINTEGER    srcLength,
                    CDescRec*     targetDescPtr,
                    ICUConverter* iconv,
                    SQLPOINTER    targetDataPtr, 
                    UCHAR         *errorMsg);



unsigned long   MemcpyToNumber(SQLPOINTER  DataPtr,
            SQLINTEGER &    DataLen,
            CDescRec*       targetDescPtr,
            SQLSMALLINT     CDataType,
            BOOL            useDouble,
            double          dTmp,
            BOOL            negative,
            __int64         decimalPart,
            __int64         integralPart,
            long            leadZeros,
            BOOL            signedInteger,
            ICUConverter*   iconv,
            SQLPOINTER &    outDataPtr,
            unsigned long   retTmp);



unsigned long  ConvertToCharTypes(SQLINTEGER      ODBCAppVersion,
                          SQLSMALLINT   CDataType,
                          SQLPOINTER    srcDataPtr,
                          SQLINTEGER    srcLength,
                          CDescRec*     targetDescPtr,
                          ICUConverter* iconv,
                          SQLPOINTER    targetDataPtr, 
                          UCHAR         *errorMsg);

unsigned long    ConvertCharset(SQLPOINTER  DataPtr,
            SQLINTEGER &    DataLen,
            CDescRec*       targetDescPtr,
            SQLSMALLINT     CDataType,
            ICUConverter*   iconv,
            SQLPOINTER      targetDataPtr,
            SQLPOINTER &    outDataPtr, 
            SQLINTEGER      OutLen ,
            short           Offset,
            UCHAR           *errorMsg);


unsigned long  ConvertToBigint(SQLINTEGER	ODBCAppVersion,
        SQLSMALLINT   CDataType,
        SQLPOINTER    srcDataPtr,
        SQLINTEGER    srcLength,
        CDescRec*     targetDescPtr,
        ICUConverter* iconv,
        SQLPOINTER    targetDataPtr,
        UCHAR         *errorMsg);

unsigned long  ConvertToDateType(SQLINTEGER	   ODBCAppVersion,
        SQLSMALLINT   CDataType,
        SQLPOINTER    srcDataPtr,
        SQLINTEGER    srcLength,
        CDescRec*     targetDescPtr,
        ICUConverter* iconv,
        SQLPOINTER    targetDataPtr,
        BOOL          RWRSFormat,
        UCHAR         *errorMsg);

unsigned long  ConvertToTimeType(SQLINTEGER	   ODBCAppVersion,
        SQLSMALLINT   CDataType,
        SQLPOINTER    srcDataPtr,
        SQLINTEGER    srcLength,
        CDescRec*     targetDescPtr,
        ICUConverter* iconv,
        SQLPOINTER    targetDataPtr,
        BOOL          RWRSFormat,
        UCHAR         *errorMsg);

unsigned long  ConvertToTimeStampType(SQLINTEGER	   ODBCAppVersion,
        SQLSMALLINT   CDataType,
        SQLPOINTER    srcDataPtr,
        SQLINTEGER    srcLength,
        CDescRec*     targetDescPtr,
        ICUConverter* iconv,
        SQLPOINTER    targetDataPtr,
        BOOL          RWRSFormat,
        UCHAR         *errorMsg);

unsigned long  IntervalDataTypeTransform(SQLSMALLINT  & cTmpDataType,SQLSMALLINT  CDataType,SQLSMALLINT ODBCDataType);

unsigned long  IntervalBufferHelper(SQLSMALLINT ODBCDataType,SQLSMALLINT CDataType,SQLPOINTER    srcDataPtr,CHAR * cTmpBuf);


void FitToSQLIntervalDayToSecond(SQLSMALLINT CDataType,CHAR * cTmpBuf, SQL_INTERVAL_STRUCT *intervalTmp);


void FitToSQLIntervalHourToSecond(SQLSMALLINT CDataType,CHAR * cTmpBuf, SQL_INTERVAL_STRUCT *intervalTmp);
unsigned long  ConvertToTimeIntervalType(SQLINTEGER     ODBCAppVersion,
        SQLSMALLINT   CDataType,
        SQLPOINTER    srcDataPtr,
        SQLINTEGER    srcLength,
        CDescRec*     targetDescPtr,
        ICUConverter* iconv,
        SQLPOINTER    targetDataPtr, 
        BOOL          RWRSFormat,
        UCHAR         *errorMsg,
        SQLINTEGER  datetimeIntervalPrecision);



unsigned long ConvertCharToNumeric(SQLPOINTER srcDataPtr, 
								   SQLINTEGER srcLength, 
								   double &dTmp);

unsigned long ConvertCharToInt64(SQLPOINTER srcDataPtr, 
								   SQLINTEGER srcLength, 
								   __int64 &tempVal64);
unsigned long ConvertCharToUnsignedInt64(SQLPOINTER srcDataPtr, 
								   SQLINTEGER srcLength, 
								   unsigned __int64 &uTempVal64);


short ConvertCharToInt64Num(const char *srcDataPtr, 
						   __int64 &integralPart,
						   __int64 &decimalPart,
						   BOOL	&negative,
						   long &decimalLength);

void getMaxNum(long integral,
				long decimal,
				unsigned __int64 &integralMax,
				unsigned __int64 &decimalMax);

unsigned long ConvertCharWithDecimalToInt64(SQLPOINTER srcDataPtr, 
								   SQLINTEGER srcLength, 
								   __int64 &integralPart,
								   __int64 &decimalPart);

SQLRETURN ConvertCharToSQLDate(SQLPOINTER srcDataPtr, SQLINTEGER srcLength, SWORD ODBCDataType,
							   SQLPOINTER targetPtr, SQLSMALLINT targetPrecision);

long getDigitCount(__int64 value);

SQLRETURN ConvertCNumericToChar( SQL_NUMERIC_STRUCT* numericPtr, char* cTmpBuf);

SQLRETURN StripIntervalLiterals(SQLPOINTER srcDataPtr, SQLINTEGER srcLength, SWORD ODBCDataType, char* cTmpBuf);

void InsertBlank(char *str, short numBlank);

unsigned long CheckIntervalOverflow(char *intervalValue, SWORD ODBCDataType, SQLINTEGER targetLength, SQLSMALLINT secPrecision);

SQLINTEGER GetIntervalLeadingPrecision(SWORD ODBCDataType, SQLINTEGER targetLength, SQLSMALLINT secPrecision);

unsigned long Ascii_To_Bignum_Helper(char *source,
							 long sourceLen,
							 char *target,
							 long targetLength,
							 long targetPrecision,
							 long targetScale,
							 SQLSMALLINT SQLDataType,
							 BOOL *truncation
							);

unsigned long Ascii_To_Interval_Helper(char *source,
                                         long sourceLen,
                                         char *target,
                                         long targetLength,
                                         long targetPrecision,
                                         long targetScale,
                                         SQLSMALLINT SQLDataType,
					 BOOL *truncation
                                         );

}

#endif
