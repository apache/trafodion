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
#include "cdesc.h"

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
							DWORD		translateOption = 0,
#endif
							UCHAR		*errorMsg = NULL,
							SWORD		errorMsgMax = 0,
							SQLINTEGER	EnvironmentType = NSK_BUILD_1);

unsigned long ConvToSQLBool(SQLPOINTER srcDataPtr,
							SQLINTEGER srcLength,
							SQLSMALLINT CDataType,
							SQLPOINTER targetDataPtr);

unsigned long ConvertToCharTypes(SQLINTEGER ODBCAppVersion,
								SQLSMALLINT CDataType,
								SQLPOINTER srcDataPtr,
								SQLINTEGER srcLength,
								CDescRec* targetDescPtr,
								BOOL byteSwap,
#ifdef unixcli
								ICUConverter* iconv,
#else
								DWORD translateOption,
#endif
								SQLPOINTER targetDataPtr,
								UCHAR *errorMsg,
								SWORD errorMsgMax);

unsigned long ConvertCharset(SQLPOINTER DataPtr,
							SQLINTEGER& DataLen,
							CDescRec* targetDescPtr,
							SQLSMALLINT CDataType,
#ifdef unixcli
							ICUConverter* iconv,
#else
							DWORD translateOption,
#endif
							SQLPOINTER targetDataPtr,
							SQLPOINTER& outDataPtr,
							SQLINTEGER OutLen,
							short Offset,
							UCHAR* errorMsg,
							SWORD errorMsgMax);

unsigned long  ConvertToNumberSimple(SQLSMALLINT CDataType,
									SQLPOINTER srcDataPtr,
									SQLINTEGER srcLength,
									CDescRec* targetDescPtr,
#ifdef unixcli
									ICUConverter* iconv,
#else
									DWORD translateOption,
#endif
									SQLPOINTER targetDataPtr,
									UCHAR* errorMsg);

unsigned long  ConvertToBigint(SQLINTEGER ODBCAppVersion,
								SQLSMALLINT CDataType,
								SQLPOINTER srcDataPtr,
								SQLINTEGER srcLength,
								CDescRec* targetDescPtr,
#ifdef unixcli
								ICUConverter* iconv,
#else
								DWORD translateOption,
#endif
								SQLPOINTER targetDataPtr,
								UCHAR* errorMsg);

unsigned long  ConvertToNumeric(SQLINTEGER ODBCAppVersion,
								SQLSMALLINT CDataType,
								SQLPOINTER srcDataPtr,
								SQLINTEGER srcLength,
								CDescRec* targetDescPtr,
								BOOL byteSwap,
#ifdef unixcli
								ICUConverter* iconv,
#else
								DWORD translateOption,
#endif
								SQLPOINTER targetDataPtr,
								UCHAR* errorMsg);

unsigned long MemcpyToNumeric(SQLPOINTER DataPtr,
							SQLINTEGER& DataLen,
							CDescRec* targetDescPtr,
							SQLSMALLINT CDataType,
							BOOL useDouble,
							double dTmp,
							BOOL negative,
							unsigned __int64 decimalPart,
							unsigned __int64 integralPart,
							long leadZeros,
#ifdef unixcli
							ICUConverter* iconv,
#else
							DWORD translateOption,
#endif
							SQLPOINTER& outDataPtr,
							unsigned long retTmp);

unsigned long ConvertToDateType(SQLINTEGER ODBCAppVersion,
								SQLSMALLINT CDataType,
								SQLPOINTER srcDataPtr,
								SQLINTEGER srcLength,
								CDescRec* targetDescPtr,
#ifdef unixcli
								ICUConverter* iconv,
#else
								DWORD translateOption,
#endif
								SQLPOINTER targetDataPtr,
								UCHAR *errorMsg);

unsigned long ConvertToTimeType(SQLINTEGER ODBCAppVersion,
								SQLSMALLINT CDataType,
								SQLPOINTER srcDataPtr,
								SQLINTEGER srcLength,
								CDescRec* targetDescPtr,
#ifdef unixcli
								ICUConverter* iconv,
#else
								DWORD translateOption,
#endif
								SQLPOINTER targetDataPtr,
								UCHAR *errorMsg);

unsigned long ConvertToTimeStampType(SQLINTEGER	ODBCAppVersion,
									SQLSMALLINT CDataType,
									SQLPOINTER srcDataPtr,
									SQLINTEGER srcLength,
									CDescRec* targetDescPtr,
#ifdef unixcli
									ICUConverter* iconv,
#else
									DWORD translateOption,
#endif
									SQLPOINTER targetDataPtr,
									UCHAR *errorMsg);

unsigned long  ConvertToTimeIntervalType(SQLINTEGER ODBCAppVersion,
										SQLSMALLINT	CDataType,
										SQLPOINTER srcDataPtr,
										SQLINTEGER srcLength,
										CDescRec* targetDescPtr,
#ifdef unixcli
										ICUConverter* iconv,
#else
										DWORD translateOption,
#endif
										SQLPOINTER targetDataPtr,
										UCHAR *errorMsg);

unsigned long ConvertCharToNumeric(SQLPOINTER srcDataPtr, 
								   SQLINTEGER srcLength, 
								   double &dTmp);

unsigned long ConvertCharToInt64(SQLPOINTER srcDataPtr, 
								   SQLINTEGER srcLength, 
								   __int64 &tempVal64);

unsigned long ConvertCharToUnsignedInt64(SQLPOINTER srcDataPtr,
									SQLINTEGER srcLength,
									unsigned __int64 &utempVal64);

short ConvertCharToInt64Num(const char *srcDataPtr, 
						   unsigned __int64 &integralPart,
						   unsigned __int64 &decimalPart,
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
