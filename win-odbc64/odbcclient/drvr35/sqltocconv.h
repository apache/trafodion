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

#include <windows.h>
#include <sql.h>
#include <sqlExt.h>
#include "DrvrGlobal.h"
#include "charsetconv.h"
#include <cdesc.h>

#define ENDIAN_PRECISION_MAX	39
#define	NUM_LEN_MAX				128 + 2

namespace ODBC {

unsigned long ConvertSQLToC(SQLINTEGER	ODBCAppVersion,
							DWORD		DataLangId,
							CDescRec*	srcDescPtr,
							SQLPOINTER	srcDataPtr,
							SQLINTEGER	srcLength,
							SQLSMALLINT	CDataType,
							SQLPOINTER	targetDataPtr,
							SQLINTEGER	targetLength,
							SQLLEN		*targetStrLenPtr,
							BOOL		byteSwap,
							CHAR		*&translatedDataPtr,
							SQLINTEGER	*totalReturnedLength = NULL,
							DWORD		translateOption = 0,
							UCHAR		*errorMsg = NULL,
							SWORD		errorMsgMax = 0,
							SQLINTEGER	EnvironmentType = NSK_BUILD_1,
							BOOL		ColumnwiseData = FALSE,
							CHAR		*replacementChar = NULL);

unsigned long ConvSQLCharToChar(SQLPOINTER srcDataPtr, CDescRec* srcDescPtr, SQLINTEGER srcLength, SQLSMALLINT CDataType,
								SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN *targetStrLenPtr,
								DWORD translateOption, CHAR *&translatedDataPtr, SQLINTEGER* totalReturnedLength,
								UCHAR *errorMsg, SWORD errorMsgMax, CHAR *replacementChar);

unsigned long ConvSQLNumberToChar(SQLPOINTER srcDataPtr,
								CDescRec* srcDescPtr,
								SQLINTEGER srcLength,
								SQLSMALLINT CDataType,
								SQLPOINTER targetDataPtr,
								SQLINTEGER targetLength,
								SQLLEN *targetStrLenPtr);

unsigned long ConvSQLDateToChar(SQLPOINTER srcDataPtr,
								SQLINTEGER srcLength,
								SQLSMALLINT CDataType,
								SQLPOINTER targetDataPtr,
								SQLINTEGER targetLength,
								SQLLEN *targetStrLenPtr);

unsigned long ConvSQLTimeToChar(SQLPOINTER srcDataPtr,
								SQLINTEGER srcPrecision,
								SQLINTEGER srcLength,
								SQLSMALLINT CDataType,
								SQLPOINTER targetDataPtr,
								SQLINTEGER targetLength,
								SQLLEN *targetStrLenPtr);

unsigned long ConvSQLTimestampToChar(SQLPOINTER srcDataPtr,
									SQLINTEGER srcPrecision,
									SQLINTEGER srcLength,
									SQLSMALLINT CDataType,
									SQLPOINTER targetDataPtr,
									SQLINTEGER targetLength,
									SQLLEN *targetStrLenPtr);

unsigned long ConvSQLCharToNumber(SQLPOINTER srcDataPtr,
								CDescRec* srcDescPtr,
								SQLINTEGER srcLength,
								SQLSMALLINT CDataType,
								SQLPOINTER targetDataPtr,
								SQLINTEGER targetLength,
								SQLLEN *targetStrLenPtr);

unsigned long ConvertDoubleToNumber(double dTmp,
									SQLSMALLINT CDataType,
									SQLPOINTER targetDataPtr,
									SQLINTEGER targetLength,
									SQLLEN *targetStrLenPtr);

unsigned long ConvSQLNumberToNumber(SQLPOINTER srcDataPtr,
									CDescRec* srcDescPtr,
									SQLINTEGER srcLength,
									SQLSMALLINT CDataType,
									SQLPOINTER targetDataPtr,
									SQLINTEGER targetLength,
									SQLLEN *targetStrLenPtr);

unsigned long ConvSQLBigintToNumber(SQLPOINTER srcDataPtr,
									SQLSMALLINT srcUnsigned,
									SQLSMALLINT CDataType,
									SQLPOINTER targetDataPtr,
									SQLINTEGER targetLength,
									SQLLEN *targetStrLenPtr);

unsigned long ConvSQLNumericToNumber(SQLPOINTER srcDataPtr,
									CDescRec *srcDescPtr,
									SQLINTEGER srcLength,
									SQLSMALLINT CDataType,
									SQLPOINTER targetDataPtr,
									SQLINTEGER targetLength,
									SQLLEN *targetStrLenPtr);

unsigned long ConvSQLCharToDate(SQLPOINTER srcDataPtr,
								CDescRec *srcDescPtr,
								SQLINTEGER srcLength,
								SQLPOINTER targetDataPtr,
								SQLLEN *targetStrLenPtr);

unsigned long ConvDoubleToInterval(DOUBLE dTmp,
								SQLSMALLINT CDataType,
								SQLPOINTER targetDataPtr);

unsigned long ConvSQLDateToDate(SQLPOINTER srcDataPtr,
								SQLSMALLINT SQLDatetimeCode,
								BOOL ColumnwiseData,
								SQLPOINTER targetDataPtr);

unsigned long ConvSQLTimeToTime(SQLPOINTER srcDataPtr,
								SQLSMALLINT SQLDatetimeCode,
								BOOL ColumnwiseData,
								SQLPOINTER targetDataPtr);

unsigned long ConvSQLTimestampToDateTime(SQLPOINTER srcDataPtr,
										SQLSMALLINT SQLDatetimeCode,
										SQLSMALLINT srcPrecision,
										BOOL ColumnwiseData,
										SQLSMALLINT CDataType,
										SQLPOINTER targetDataPtr);

unsigned long ConvSQLDateToTimestamp(SQLPOINTER srcDataPtr,
									SQLSMALLINT SQLDatetimeCode,
									BOOL ColumnwiseData,
									SQLPOINTER targetDataPtr);

unsigned long ConvSQLTimeToTimestamp(SQLPOINTER srcDataPtr,
									SQLSMALLINT SQLDatetimeCode,
									BOOL ColumnwiseData,
									SQLPOINTER targetDataPtr);

unsigned long ConvSQLCharToNumeric(SQLPOINTER srcDataPtr,
								CDescRec *srcDescPtr,
								SQLINTEGER srcLength,
								SQLPOINTER targetDataPtr);

unsigned long GetCTmpBufFromSQLChar(SQLPOINTER srcDataPtr,
								CDescRec* srcDescPtr,
								SQLINTEGER srcLength,
								bool isShort,
								char *&cTmpBuf,
								SQLINTEGER *tmpLen,
								bool RemoveSpace = true);

SQLRETURN ConvertNumericToChar(SQLSMALLINT SQLDataType,
							SQLPOINTER srcDataPtr,
							SQLSMALLINT srcScale,
							char *cTmpBuf,
							SQLINTEGER &DecimalPoint);

SQLRETURN ConvertDecimalToChar(SQLSMALLINT SQLDataType, SQLPOINTER srcDataPtr, SQLINTEGER srcLength, 
								SQLSMALLINT srcScale, char *cTmpBuf, SQLINTEGER &DecimalPoint);

SQLRETURN ConvertSoftDecimalToDouble(SQLSMALLINT SQLDataType, SQLPOINTER srcDataPtr, SQLINTEGER srcLength, 
								SQLSMALLINT srcScale, double &dTmp);

unsigned long ConvertSQLCharToDouble(SQLPOINTER srcDataPtr, SQLINTEGER srcLength,
									SQLSMALLINT ODBCDataType, double &dTmp);

unsigned long ConvertSQLCharToDateTime(SQLSMALLINT ODBCDataType, 
						SQLPOINTER srcDataPtr,
						SQLINTEGER	srcLength,
						SQLSMALLINT CDataType,
						SQLPOINTER outValue);

unsigned long ConvertCharToCNumeric( SQL_NUMERIC_STRUCT& numericTmp, 
									CHAR* cTmpBuf);

unsigned long ConvertSQLCharToInterval(SQLSMALLINT ODBCDataType, 
						SQLPOINTER srcDataPtr,
						SQLINTEGER	srcLength,
						SQLSMALLINT CDataType,
						SQLPOINTER outValue);

SWORD GetYearFromStr(UCHAR* ptr);
UCHAR GetMonthFromStr(UCHAR* ptr);
UCHAR GetDayFromStr(UCHAR* ptr);
UCHAR GetHourFromStr(UCHAR* ptr);
UCHAR GetMinuteFromStr(UCHAR* ptr);
UCHAR GetSecondFromStr(UCHAR* ptr);
UDWORD GetFractionFromStr(UCHAR* ptr, short precision);
unsigned short ConvToInt(UCHAR* ptr,int len);

unsigned long BigNum_To_Ascii_Helper(char * source,
							 long sourceLen,
							 long sourcePrecision,
							 long sourceScale,
							 char * target,
							 SQLSMALLINT SQLDataType
							);

inline char * getTmpDest(unsigned int size, unsigned int *allocSize)
{
	if ((size > 8192) && (size <= 16384))
	{
		*allocSize = 16384; // 16 * 1024 bytes
		return new char[16384];
	}
	else
	{
		*allocSize = 32768; // 32 * 1024 bytes
		return new char[32768];
	}
}

}

#endif
