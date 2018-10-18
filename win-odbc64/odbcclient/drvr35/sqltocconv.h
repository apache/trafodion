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
#include "cconnect.h"

#define ENDIAN_PRECISION_MAX	39

namespace ODBC {

unsigned long ConvertSQLToC(CConnect *ConnectHandle,
                            SQLHANDLE InputHandle,
                            SQLINTEGER	ODBCAppVersion,
							DWORD		DataLangId,
							SQLSMALLINT	SQLDataType,
							SQLSMALLINT	ODBCDataType,
							SQLSMALLINT SQLDatetimeCode,
							SQLPOINTER	srcDataPtr,
							SQLINTEGER	srcLength,
							SQLSMALLINT	srcPrecision,
							SQLSMALLINT	srcScale,
							SQLSMALLINT srcUnsigned,
							SQLINTEGER	srcCharSet,
							SQLINTEGER	srcMaxLength,
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

SQLRETURN ConvertNumericToChar(SQLSMALLINT SQLDataType, SQLPOINTER srcDataPtr, SQLSMALLINT srcScale, 
							   char *cTmpBuf, SQLINTEGER &DecimalPoint);

SQLRETURN ConvertDecimalToChar(SQLSMALLINT SQLDataType, SQLPOINTER srcDataPtr, SQLINTEGER srcLength, 
								SQLSMALLINT srcScale, char *cTmpBuf, SQLINTEGER &DecimalPoint);

SQLRETURN ConvertSoftDecimalToDouble(SQLSMALLINT SQLDataType, SQLPOINTER srcDataPtr, SQLINTEGER srcLength, 
								SQLSMALLINT srcScale, double &dTmp);

unsigned long ConvertSQLCharToNumeric(SQLPOINTER srcDataPtr, SQLINTEGER srcLength,
									SQLSMALLINT ODBCDataType, double &dTmp);

unsigned long ConvertSQLCharToDate(SQLSMALLINT ODBCDataType, 
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
