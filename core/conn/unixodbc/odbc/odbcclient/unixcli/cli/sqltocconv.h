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
//
//
#ifndef SQLTOCCONV_H
#define SQLTOCCONV_H

#ifdef unixcli
#include "unix_extra.h"
#endif
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include "drvrglobal.h"
#include "charsetconv.h"
#include "cdesc.h"

namespace ODBC {

unsigned long ConvertSQLToC(SQLINTEGER      ODBCAppVersion,
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
                            DWORD           translateOption = 0,
#endif
                            SQLINTEGER*     totalReturnedLength = NULL,//offset in Input
                            UCHAR*          errorMsg = NULL,
                            SWORD           errorMsgMax = 0,
                            SQLINTEGER      EnvironmentType = NSK_BUILD_1,
                            BOOL            ColumnwiseData = FALSE,//catalog api set TRUE
                            CHAR*           replacementChar = NULL
                            );


unsigned long ConvSQLNumberToChar(SQLPOINTER srcDataPtr,
                                CDescRec* srcDescPtr,
                                SQLINTEGER srcLength,
                                SQLSMALLINT CDataType,
                                SQLPOINTER targetDataPtr,
                                SQLINTEGER targetLength,
                                SQLLEN* targetStrLenPtr);

unsigned long ConvSQLCharToChar(SQLPOINTER srcDataPtr,
                                CDescRec* srcDescPtr,
                                SQLINTEGER srcLength,
                                SQLSMALLINT CDataType,
                                SQLPOINTER targetDataPtr,
                                SQLINTEGER targetLength,
                                SQLLEN* targetStrLenPtr,
                                ICUConverter* iconv,
                                CHAR*& translatedDataPtr,
                                SQLINTEGER* totalReturnedLength,
                                UCHAR* errorMsg,
                                CHAR* replacementChar);

unsigned long ConvertDecimalToChar(SQLSMALLINT SQLDataType,
                                SQLPOINTER srcDataPtr,
                                SQLINTEGER srcLength,
                                SQLSMALLINT srcScale,
                                char *cTmpBuf,
                                SQLINTEGER &DecimalPoint);

unsigned long ConvertSoftDecimalToDouble(SQLSMALLINT SQLDataType,
                                        SQLPOINTER srcDataPtr,
                                        SQLINTEGER srcLength,
                                        SQLSMALLINT srcScale,
                                        double &dTmp);

unsigned long ConvSQLNumberToDouble(SQLPOINTER srcDataPtr,
                                    CDescRec* srcDescPtr,
                                    SQLINTEGER srcLength,
                                    double &dTmp);

unsigned long ConvSQLBigintToNumber(SQLPOINTER srcDataPtr,
                                    bool unsignedValue,
                                    SQLSMALLINT CDataType,
                                    SQLSMALLINT Scale,
                                    SQLPOINTER targetDataPtr,
                                    SQLLEN* targetStrLenPtr);

unsigned long ConvSQLNumericToNumber(SQLPOINTER srcDataPtr,
                                    CDescRec* srcDescPtr,
                                    SQLINTEGER srcLength,
                                    SQLSMALLINT CDataType,
                                    SQLPOINTER targetDataPtr,
                                    SQLLEN* targetStrLenPtr);

unsigned long ConvFromSQLBool(SQLPOINTER srcDataPtr,
                            SQLSMALLINT CDataType,
                            SQLPOINTER targetDataPtr,
                            SQLINTEGER targetLength,
                            SQLLEN* targetStrLenPtr);

unsigned long ConvSQLCharToNumber(SQLPOINTER srcDataPtr,
                                CDescRec* srcDescPt,
                                SQLINTEGER srcLength,
                                SQLSMALLINT CDataType,
                                SQLPOINTER targetDataPtr,
                                SQLLEN* targetStrLenPtr,
                                ICUConverter* iconv,
                                UCHAR *errorMsg = NULL);

unsigned long ConvSQLIntevalToDouble(SQLPOINTER srcDataPtr,
                                    double &dTmp);

unsigned long ConvDoubleToCNumber(double dTmp,
                                SQLSMALLINT CDataType,
                                SQLPOINTER targetDataPtr,
                                SQLLEN* targetStrLenPtr);


unsigned long ConvSQLNumericToChar(SQLPOINTER srcDataPtr,
                                CDescRec* srcDescPtr,
                                SQLINTEGER srcLength,
                                SQLSMALLINT CDataType,
                                SQLPOINTER targetDataPtr,
                                SQLINTEGER targetLength,
                                SQLLEN* targetStrLenPtr);

unsigned long BigNum_To_Ascii_Helper(char * source,
                    long sourceLen,
                    long sourcePrecision,
                    long sourceScale,
                    char * target,
                    SQLSMALLINT SQLDataType
                    );

unsigned long ConvSQLSoftNumericToChar(SQLSMALLINT SQLDataType,
                                    SQLPOINTER srcDataPtr,
                                    SQLSMALLINT srcScale,
                                    char *cTmpBuf,
                                    SQLINTEGER &DecimalPoint);

unsigned long ConvSQLDateToChar(SQLPOINTER srcDataPtr,
                                SQLINTEGER srcLength,
                                SQLSMALLINT CDataType,
                                SQLPOINTER targetDataPtr,
                                SQLINTEGER targetLength,
                                SQLLEN* targetStrLenPtr);

unsigned long ConvSQLTimeToChar(SQLPOINTER srcDataPtr,
                                SQLSMALLINT srcPrecision,
                                SQLINTEGER srcLength,
                                SQLSMALLINT CDataType,
                                SQLPOINTER targetDataPtr,
                                SQLINTEGER targetLength,
                                SQLLEN* targetStrLenPtr);

unsigned long ConvSQLTimestampToChar(SQLPOINTER srcDataPtr,
                                    SQLSMALLINT srcPrecision,
                                    SQLINTEGER srcLength,
                                    SQLSMALLINT CDataType,
                                    SQLPOINTER targetDataPtr,
                                    SQLINTEGER targetLength,
                                    SQLLEN* targetStrLenPtr);

unsigned long ConvCopyColumnwiseData(SQLPOINTER srcDataPtr,
                                    SQLINTEGER srcLength,
                                    SQLSMALLINT CDataType,
                                    SQLPOINTER targetDataPtr,
                                    SQLINTEGER targetLength,
                                    SQLLEN* targetStrLenPtr);

unsigned long ConvSQLCharToDateTime(SQLPOINTER srcDataPtr,
                        SQLINTEGER    srcLength,
                        SQLSMALLINT CDataType,
                        SQLPOINTER outValue);

unsigned long ConvSQLDateToDate(SQLPOINTER srcDataPtr,
                                SQLSMALLINT SQLDatatimeCode,
                                SQLPOINTER targetDataPtr,
                                SQLLEN* targetStrLenPtr,
                                BOOL ColumnwiseData);

unsigned long ConvSQLTimestampToDate(SQLPOINTER srcDataPtr,
                                    SQLSMALLINT SQLDatatimeCode,
                                    SQLPOINTER targetDataPtr,
                                    SQLLEN* targetStrLenPtr,
                                    BOOL ColumnwiseData);

unsigned long ConvSQLTimeToTime(SQLPOINTER srcDataPtr,
                                SQLSMALLINT SQLDatatimeCode,
                                SQLPOINTER targetDataPtr,
                                SQLLEN* targetStrLenPtr,
                                BOOL ColumnwiseData);

unsigned long ConvSQLTimestampToTime(SQLPOINTER srcDataPtr,
                                    SQLSMALLINT SQLDatatimeCode,
                                    SQLPOINTER targetDataPtr,
                                    SQLLEN* targetStrLenPtr,
                                    BOOL ColumnwiseData);

unsigned long ConvSQLDateToTimestamp(SQLPOINTER srcDataPtr,
                                    SQLSMALLINT SQLDatatimeCode,
                                    SQLPOINTER targetDataPtr,
                                    SQLLEN* targetStrLenPtr,
                                    BOOL ColumnwiseData);

unsigned long ConvSQLTimeToTimestamp(SQLPOINTER srcDataPtr,
                                    SQLSMALLINT SQLDatatimeCode,
                                    SQLPOINTER targetDataPtr,
                                    SQLLEN* targetStrLenPtr,
                                    BOOL ColumnwiseData);

unsigned long ConvSQLTimestampToTimestamp(SQLPOINTER srcDataPtr,
                                        CDescRec* srcDescPtr,
                                        SQLPOINTER targetDataPtr,
                                        SQLLEN* targetStrLenPtr,
                                        BOOL ColumnwiseData);

unsigned long ConvSQLCharToNumeric(SQLPOINTER srcDataPtr,
                                CDescRec* srcDescPtr,
                                SQLINTEGER srcLength,
                                SQLSMALLINT CDataType,
                                SQLPOINTER targetDataPtr,
                                SQLLEN* targetStrLenPtr,
                                ICUConverter* iconv,
                                UCHAR* errorMsg);

unsigned long ConvertCharToCNumeric(SQL_NUMERIC_STRUCT* numericTmp,
                                    CHAR* cTmpBuf);

unsigned long ConvSQLNumericToNumeric(SQLPOINTER srcDataPtr,
                                    CDescRec* srcDescPtr,
                                    SQLINTEGER srcLength,
                                    SQLPOINTER targetDataPtr,
                                    SQLLEN* targetStrLenPtr);

unsigned long ConvertSQLCharToInterval(SQLSMALLINT ODBCDataType, 
                        SQLPOINTER srcDataPtr,
                        SQLINTEGER    srcLength,
                        SQLSMALLINT CDataType,
                        SQLPOINTER outValue);

unsigned long  ConvDoubleToInterval(double dTmp,
                                    SQLPOINTER targetDataPtr,
                                    SQLINTEGER targetLength,
                                    SQLSMALLINT CDataType,
                                    SQLLEN* targetStrLenPtr);

unsigned long GetCTmpBufFromSQLChar(SQLPOINTER srcDataPtr,
                                    SQLINTEGER srcLength,
                                    SQLSMALLINT ODBCDataType,
                                    SQLINTEGER srcCharSet,
                                    bool isshort,
                                    char*& cTmpBuf,
                                    SQLINTEGER &tmpLen,
                                    ICUConverter* iconv,
                                    UCHAR* errorMsg,
                                    bool RemoveSpace);

unsigned short ConvToInt(UCHAR* ptr, int len);

SWORD GetYearFromStr(UCHAR* ptr);
UCHAR GetMonthFromStr(UCHAR* ptr);
UCHAR GetDayFromStr(UCHAR* ptr);
UCHAR GetHourFromStr(UCHAR* ptr);
UCHAR GetMinuteFromStr(UCHAR* ptr);
UCHAR GetSecondFromStr(UCHAR* ptr);
UDWORD_P GetFractionFromStr(UCHAR* ptr, short precision);

}


#endif
