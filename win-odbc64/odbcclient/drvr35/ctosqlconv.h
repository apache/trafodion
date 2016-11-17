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

#include <windows.h>
#include <sql.h>
#include <sqlExt.h>
#include "DrvrGlobal.h"
#include "charsetconv.h"

namespace ODBC {

unsigned long ConvertCToSQL(SQLINTEGER	ODBCAppVersion,
							SQLSMALLINT	CDataType,
							SQLPOINTER	srcDataPtr,
							SQLINTEGER	srcLength,
							SQLSMALLINT	ODBCDataType,
							SQLSMALLINT	SQLDataType,
							SQLSMALLINT	SQLDatetimeCode,
							SQLPOINTER	targetDataPtr,
							SQLINTEGER	targetLength,
							SQLINTEGER	targetPrecision,
							SQLSMALLINT	targetScale,
							SQLSMALLINT targetUnsigned,
							SQLINTEGER	targetCharSet,
							BOOL		byteSwap,
							DWORD		translateOption=0,
							UCHAR		*errorMsg = NULL,
							SWORD		errorMsgMax = 0,
							SQLINTEGER	EnvironmentType = NSK_BUILD_1);

unsigned long ConvertCharToNumeric(SQLPOINTER srcDataPtr, 
								   SQLINTEGER srcLength, 
								   double &dTmp);

unsigned long ConvertCharToInt64(SQLPOINTER srcDataPtr, 
								   SQLINTEGER srcLength, 
								   __int64 &tempVal64);

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

}

#endif
