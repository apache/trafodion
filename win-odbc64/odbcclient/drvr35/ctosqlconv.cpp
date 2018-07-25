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

#include "ctosqlconv.h"
#include <stdio.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include "sqlcli.h"
#include "drvrSrvr.h"
#include "tdm_odbcDrvMsg.h"
#include "DrvrGlobal.h"
#include "nskieee.h"
#include "DiagFunctions.h"
#include "csconvert.h"
#include <errno.h>

extern short convDoItMxcs(char * source, 
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

using namespace ODBC;


unsigned long ODBC::Ascii_To_Bignum_Helper(char *source,
					 long sourceLen,
					 char *target,
					 long targetLength,
					 long targetPrecision,
					 long targetScale,
					 SQLSMALLINT SQLDataType,
					 BOOL *truncation
					)						
							
{
	short targetType;
	SQLRETURN retCode;

	if (SQLDataType == SQLTYPECODE_NUMERIC_UNSIGNED)
		targetType = BIGNUM_UNSIGNED;
	else 
		targetType = BIGNUM_SIGNED;

        retCode = convDoItMxcs(source,sourceLen, 0, 0, 0, (char*)target, targetLength, 
				targetType, targetPrecision, targetScale, 0, truncation);

	if (retCode != 0)
		return IDS_22_003;


	return SQL_SUCCESS;

} // Ascii_To_Bignum_Helper()


unsigned long ODBC::ConvertCToSQL(SQLINTEGER	ODBCAppVersion,
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
							DWORD		translateOption,
							UCHAR		*errorMsg,
							SWORD		errorMsgMax,
							SQLINTEGER	EnvironmentType)
{

	unsigned long retCode = SQL_SUCCESS;
	SQLPOINTER		DataPtr = NULL;
	SQLPOINTER		outDataPtr = targetDataPtr;
	SQLINTEGER		DataLen = DRVR_PENDING;
	short			Offset = 0;	// Used for VARCHAR fields
	SQLINTEGER		OutLen = targetLength;
	short targetType = 0;  //for bignum datatype
	
	
	int			dec;
	int			sign;
	int			tempLen;
	int			tempLen1;
		
	short		i;
	short		datetime_parts[8];
	char		*tempPtr;
	double		dTmp;
	double		dTmp1;
	double		scaleOffset;
	SSHORT		sTmp;
	USHORT		usTmp;
	SLONG		lTmp;
	ULONG		ulTmp;
	SCHAR		tTmp;
	UCHAR		utTmp;
	CHAR		cTmpBuf[256];
	CHAR		cTmpBuf2[256];
	CHAR		cTmpFraction[10];
	__int64		tempVal64;
	__int64		integralPart; 
	__int64		decimalPart;
	__int64		tempScaleVal64;
	unsigned __int64 integralMax;
	unsigned __int64 decimalMax;
	float		fltTmp;
	BOOL		useDouble = TRUE;
	BOOL		negative = FALSE;
	long		decimalDigits;
	long		leadZeros;
	unsigned long ulFraction;
	SQLSMALLINT	cTmpDataType;

	DATE_STRUCT			*dateTmp;
	TIME_STRUCT			*timeTmp;
	TIMESTAMP_STRUCT	*timestampTmp;
	SQL_INTERVAL_STRUCT	*intervalTmp;
	DATE_TYPES			SQLDate;
	TIME_TYPES			SQLTime;
	TIMESTAMP_TYPES		SQLTimestamp;
	DATE_TYPES			*pSQLDate;
	TIME_TYPES			*pSQLTime;
	TIMESTAMP_TYPES		*pSQLTimestamp;
	SQLINTEGER			translateLength;
	SQLSMALLINT			tODBCDataType;
	BOOL				signedInteger = FALSE;
	BOOL				unsignedInteger = FALSE;
	BOOL				dataTruncatedWarning = FALSE;
	int					AdjustedLength = 0;
	char				srcDataLocale[256];

	if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable){
		TraceOut(TR_ODBC_API,"ConvertCToSQL(%d, %d, %#x, %d, %d, %d, %d, %#x, %d, %d, %d, %d, %d, %d, %d, %#x, %d, %d)",
							ODBCAppVersion,
							CDataType,
							srcDataPtr,
							srcLength,
							ODBCDataType,
							SQLDataType,
							SQLDatetimeCode,
							targetDataPtr,
							targetLength,
							targetPrecision,
							targetScale,
							targetUnsigned,
							byteSwap,
							errorMsg,
							errorMsgMax,
							EnvironmentType);
	}
	else
		RESET_TRACE();
/* 
1. Because MS programs do not support BIGINT type, the server has to convert it to NUMERIC:
				ODBCDataType = SQL_NUMERIC;
				ODBCPrecision = 19;
				SignType = TRUE;
	Before conversion we have to change it back to:
				ODBCDataType = SQL_BIGINT;

2. Because ODBC does not support unsigned types for SMALLINT and INTEGER,
	the server has to convert it to:
	a)SQLTYPECODE_SMALLINT_UNSIGNED:
				ODBCPrecision = 10;
				ODBCDataType = SQL_INTEGER;
				SignType = TRUE;
	b)SQLTYPECODE_INTEGER_UNSIGNED:
				ODBCPrecision = 19;
				ODBCDataType = SQL_NUMERIC;
				SignType = TRUE;

	Before conversion we have to change it back to datatype, precision and sign described by SQL:
	a)
				ODBCPrecision = 5;
				ODBCDataType = SQL_SMALLINT;
				SignType = FALSE;
	b)
				ODBCPrecision = 10;
				ODBCDataType = SQL_INTEGER;
				SignType = FALSE;
*/
	tODBCDataType = ODBCDataType;
	if (ODBCDataType == SQL_NUMERIC && SQLDataType == SQLTYPECODE_LARGEINT &&
					targetPrecision == 19 && targetScale==0)
	{
		ODBCDataType = SQL_BIGINT;
	}

	if (ODBCDataType == SQL_INTEGER && SQLDataType == SQLTYPECODE_SMALLINT_UNSIGNED &&
				targetPrecision == 10 && targetScale==0)
	{
		targetPrecision = 5;
		ODBCDataType = SQL_SMALLINT;
		targetUnsigned = true;
	}

	if (ODBCDataType == SQL_NUMERIC && SQLDataType == SQLTYPECODE_INTEGER_UNSIGNED &&
				targetPrecision == 19 && targetScale==0)
	{
		targetPrecision = 10;
		ODBCDataType = SQL_INTEGER;
		targetUnsigned = true;
	}

	if (ODBCDataType == SQL_BIGINT && SQLDataType == SQLTYPECODE_INTEGER_UNSIGNED &&
				targetPrecision == 19 && targetScale==0)
	{
		targetPrecision = 10;
		ODBCDataType = SQL_INTEGER;
		targetUnsigned = true;
	}

	if (CDataType == SQL_C_DEFAULT)
	{
		getCDefault(tODBCDataType, ODBCAppVersion, CDataType);
		if (ODBCAppVersion >= 3 && targetUnsigned)
		{
			switch(CDataType)
			{
				case SQL_C_SHORT:
				case SQL_C_SSHORT:
					CDataType = SQL_C_USHORT;
					break;
				case SQL_C_TINYINT:
				case SQL_C_STINYINT:
					CDataType = SQL_C_UTINYINT;
					break;
				case SQL_C_LONG:
				case SQL_C_SLONG:
					CDataType = SQL_C_ULONG;
					break;
			}
		}
	}

//--------------------------------------------------------------------------------------

	if (errorMsg)
		*errorMsg = '\0';

    if (SQLDataType == SQLTYPECODE_BLOB || SQLDataType == SQLTYPECODE_CLOB)
    {
        SQLINTEGER lob_len;
        if (srcLength == SQL_NTS)
            lob_len = strlen((const char *)srcDataPtr);
        else
            lob_len = srcLength;
        memcpy((char *)targetDataPtr, &lob_len, sizeof(lob_len));
        memcpy((char *)targetDataPtr + 4, (const char *)srcDataPtr, targetLength > srcLength ? srcLength : targetLength);
        return SQL_SUCCESS;
    }
	

	switch (ODBCDataType)
	{
	case SQL_VARCHAR:
	case SQL_LONGVARCHAR:
	case SQL_WVARCHAR:
		if (targetPrecision > SHRT_MAX){
			Offset = sizeof(UINT);
		}
		else{
			Offset = sizeof(USHORT);
		}
	case SQL_CHAR:
		if (SQLDataType == SQLTYPECODE_BOOLEAN)
		{
			switch (CDataType)
			{
			case SQL_C_WCHAR:
				if (srcLength != SQL_NTS)
					srcLength = srcLength / 2;
				if (WCharToUTF8((wchar_t*)srcDataPtr, srcLength, srcDataLocale, sizeof(srcDataLocale), (int*)&translateLength, (char*)errorMsg) != SQL_SUCCESS)
					return IDS_193_DRVTODS_ERROR;
				srcDataPtr = srcDataLocale;
				srcLength = translateLength;
			case SQL_C_CHAR:
			{
				retCode = ConvertCharToNumeric(srcDataPtr, srcLength, dTmp);
				if (retCode != SQL_SUCCESS)
					return retCode;
			}
			break;
			case SQL_C_SHORT:
			case SQL_C_SSHORT:
				dTmp = *(SSHORT *)srcDataPtr;
				break;
			case SQL_C_USHORT:
				dTmp = *(USHORT *)srcDataPtr;
				break;
			case SQL_C_TINYINT:
			case SQL_C_STINYINT:
				dTmp = *(SCHAR *)srcDataPtr;
				break;
			case SQL_C_UTINYINT:
			case SQL_C_BIT:
				dTmp = *(UCHAR *)srcDataPtr;
				break;
			case SQL_C_SLONG:
			case SQL_C_LONG:
				dTmp = *(SLONG *)srcDataPtr;
				break;
			case SQL_C_ULONG:
				dTmp = *(ULONG *)srcDataPtr;
				break;
			case SQL_C_FLOAT:
				dTmp = *(SFLOAT *)srcDataPtr;
				break;
			case SQL_C_DOUBLE:
				dTmp = *(DOUBLE *)srcDataPtr;
				break;
			case SQL_C_BINARY:
				DataPtr = srcDataPtr;
				break;
			case SQL_C_DEFAULT:
				if (ODBCAppVersion >= SQL_OV_ODBC3)
					DataPtr = srcDataPtr;
				else
				{
					retCode = ConvertCharToNumeric(srcDataPtr, srcLength, dTmp);
					if (retCode != SQL_SUCCESS)
						return retCode;
				}
				break;
			case SQL_C_SBIGINT:
				dTmp = *(__int64 *)srcDataPtr;
				break;
			case SQL_C_NUMERIC:
				ConvertCNumericToChar((SQL_NUMERIC_STRUCT*)srcDataPtr, cTmpBuf);
				srcLength = strlen(cTmpBuf);
				retCode = ConvertCharToNumeric((char*)cTmpBuf, srcLength, dTmp);
				if (retCode != SQL_SUCCESS)
					return retCode;
				break;
			default:
				return IDS_07_006;
			}
			if (DataPtr == NULL)
			{
				if (dTmp < 0)
					return IDS_22_003_02;  //negValue in unsigned column
				if (dTmp > 1)
					return IDS_22_003;
				tTmp = (SCHAR)dTmp;
				if (dTmp != tTmp)
					retCode = IDS_01_S07;
				DataPtr = &tTmp;
				DataLen = sizeof(SCHAR);
			}
			break;
		}
	case SQL_WCHAR:
		switch (CDataType)
		{
		case SQL_C_CHAR:
		case SQL_C_DEFAULT:
		case SQL_C_BINARY:
		case SQL_C_WCHAR:
			{
				if (srcLength == SQL_NTS)
				{
					if ((CDataType == SQL_C_CHAR) || (CDataType == SQL_C_BINARY)|| (CDataType == SQL_C_DEFAULT))
						DataLen = strlen((const char *)srcDataPtr);
					else
						DataLen = wcslen((const wchar_t *)srcDataPtr);
				}
				else
				{
					if (CDataType == SQL_C_WCHAR)
						DataLen = srcLength/2;
					else
						DataLen = srcLength;
				}
				if (CDataType == SQL_C_BINARY && DataLen > targetLength-Offset-1)
				{
					DataLen = targetLength-Offset-1;
					return IDS_22_001;
				}
				DataPtr = srcDataPtr;
			}
			break;
		case SQL_C_SHORT:
		case SQL_C_SSHORT:
			sTmp = *(SSHORT *)srcDataPtr;
			_ltoa(sTmp, cTmpBuf, 10);
			break;
		case SQL_C_USHORT:
			usTmp = *(USHORT *)srcDataPtr;
			_ultoa(usTmp, cTmpBuf, 10);
			break;
		case SQL_C_TINYINT:
		case SQL_C_STINYINT:
			sTmp = *(SCHAR *)srcDataPtr;
			_ltoa(sTmp, cTmpBuf, 10);
			break;
		case SQL_C_UTINYINT:
		case SQL_C_BIT:
			usTmp = *(UCHAR *)srcDataPtr;
			_ultoa(usTmp, cTmpBuf, 10);
			break;
		case SQL_C_SLONG:
		case SQL_C_LONG:
			lTmp = *(SLONG *)srcDataPtr;
			_ltoa(lTmp, cTmpBuf, 10);
			break;
		case SQL_C_ULONG:
			ulTmp = *(ULONG *)srcDataPtr;
			_ultoa(ulTmp, cTmpBuf, 10);
			break;
		case SQL_C_FLOAT:
			dTmp = *(float *)srcDataPtr;
//			_gcvt(dTmp, FLT_DIG, cTmpBuf);
			if (!double_to_char (dTmp, FLT_DIG, cTmpBuf, sizeof(cTmpBuf)))
				return IDS_22_001;
			break;
		case SQL_C_DOUBLE:
			dTmp = *(double *)srcDataPtr;
//			_gcvt(dTmp, DBL_DIG, cTmpBuf);
			if (!double_to_char (dTmp, DBL_DIG, cTmpBuf, sizeof(cTmpBuf)))
				return IDS_22_001;
			break;
		case SQL_C_DATE:
		case SQL_C_TYPE_DATE:
			dateTmp = (DATE_STRUCT *)srcDataPtr;
			
			for (i = 0 ; i < 8 ; i++)
				datetime_parts[i] = 0;
			datetime_parts[0] = dateTmp->year;
			datetime_parts[1] = dateTmp->month;
			datetime_parts[2] = dateTmp->day;
			if (!checkDatetimeValue(datetime_parts))
				return IDS_22_008;

			sprintf(cTmpBuf, "%04d-%02d-%02d", dateTmp->year, dateTmp->month, dateTmp->day);
			break;
		case SQL_C_TIME:
		case SQL_C_TYPE_TIME:
			timeTmp = (TIME_STRUCT *)srcDataPtr;

			for (i = 0 ; i < 8 ; i++)
				datetime_parts[i] = 0;
			datetime_parts[0] = 1;
			datetime_parts[1] = 1;
			datetime_parts[2] = 1;
			datetime_parts[3] = timeTmp->hour;
			datetime_parts[4] = timeTmp->minute;
			datetime_parts[5] = timeTmp->second;
			if (!checkDatetimeValue(datetime_parts))
				return IDS_22_008;

			sprintf(cTmpBuf, "%02d:%02d:%02d", timeTmp->hour, timeTmp->minute, timeTmp->second);
			break;
		case SQL_C_TIMESTAMP:
		case SQL_C_TYPE_TIMESTAMP:
			timestampTmp = (TIMESTAMP_STRUCT *)srcDataPtr;
// SQL/MX fraction precision is max 6 digits but ODBC accepts max precision 9 digits 
// conversion from nano to micro fraction of second
			ulFraction = (UDWORD)timestampTmp->fraction;
			ulFraction /= 1000;

			for (i = 0 ; i < 8 ; i++)
				datetime_parts[i] = 0;
			datetime_parts[0] = timestampTmp->year;
			datetime_parts[1] = timestampTmp->month;
			datetime_parts[2] = timestampTmp->day;
			datetime_parts[3] = timestampTmp->hour;
			datetime_parts[4] = timestampTmp->minute;
			datetime_parts[5] = timestampTmp->second;
			datetime_parts[6] = (short)(ulFraction/1000);
			datetime_parts[7] = (short)(ulFraction%1000);
			if (!checkDatetimeValue(datetime_parts))
				return IDS_22_008;

			sprintf(cTmpBuf, "%04d-%02d-%02d %02d:%02d:%02d.%06lu", 
				timestampTmp->year,	timestampTmp->month, 
				timestampTmp->day,	timestampTmp->hour, 
				timestampTmp->minute, timestampTmp->second,
				ulFraction);
			break;
		case SQL_C_NUMERIC:
			ConvertCNumericToChar((SQL_NUMERIC_STRUCT*)srcDataPtr, cTmpBuf); 
			break;
		case SQL_C_SBIGINT:
			sprintf(cTmpBuf, "%I64d", *(__int64*)srcDataPtr);
			break;
		case SQL_C_INTERVAL_MONTH:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				sprintf(cTmpBuf,"-%ld",intervalTmp->intval.year_month.month);
			else
				sprintf(cTmpBuf,"%ld",intervalTmp->intval.year_month.month);
			break;
		case SQL_C_INTERVAL_YEAR:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				sprintf(cTmpBuf,"-%ld",intervalTmp->intval.year_month.year);
			else
				sprintf(cTmpBuf,"%ld",intervalTmp->intval.year_month.year);
			break;
		case SQL_C_INTERVAL_YEAR_TO_MONTH:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				sprintf(cTmpBuf,"-%ld-%ld",intervalTmp->intval.year_month.year, intervalTmp->intval.year_month.month);
			else
				sprintf(cTmpBuf,"%ld-%ld",intervalTmp->intval.year_month.year, intervalTmp->intval.year_month.month);
			break;
		case SQL_C_INTERVAL_DAY:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.day);
			else
				sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.day);
			break;
		case SQL_C_INTERVAL_HOUR:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.hour);
			else
				sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.hour);
			break;
		case SQL_C_INTERVAL_MINUTE:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.minute);
			else
				sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.minute);
			break;
		case SQL_C_INTERVAL_SECOND:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
			{
				if (intervalTmp->intval.day_second.fraction == 0)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.second);
				else
					sprintf(cTmpBuf,"-%ld.%ld",intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
			}
			else
			{
				if (intervalTmp->intval.day_second.fraction == 0)
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.second);
				else
					sprintf(cTmpBuf,"%ld.%ld",intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
			}
			break;
		case SQL_C_INTERVAL_DAY_TO_HOUR:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				sprintf(cTmpBuf,"-%ld %ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour);
			else
				sprintf(cTmpBuf,"%ld %ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour);
			break;
		case SQL_C_INTERVAL_DAY_TO_MINUTE:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				sprintf(cTmpBuf,"-%ld %ld:%ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
			else
				sprintf(cTmpBuf,"%ld %ld:%ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
			break;
		case SQL_C_INTERVAL_DAY_TO_SECOND:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
			{
				if (intervalTmp->intval.day_second.fraction == 0)
					sprintf(cTmpBuf,"-%ld %ld:%ld:%ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
				else
					sprintf(cTmpBuf,"-%ld %ld:%ld:%ld.%ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
			}
			else
			{
				if (intervalTmp->intval.day_second.fraction == 0)
					sprintf(cTmpBuf,"%ld %ld:%ld:%ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
				else
					sprintf(cTmpBuf,"%ld %ld:%ld:%ld.%ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
			}
			break;
		case SQL_C_INTERVAL_HOUR_TO_MINUTE:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				sprintf(cTmpBuf,"-%ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
			else
				sprintf(cTmpBuf,"%ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
			break;
		case SQL_C_INTERVAL_HOUR_TO_SECOND:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
			{
				if (intervalTmp->intval.day_second.fraction == 0)
					sprintf(cTmpBuf,"-%ld:%ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
				else
					sprintf(cTmpBuf,"-%ld:%ld:%ld.%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
			}
			else
			{
				if (intervalTmp->intval.day_second.fraction == 0)
					sprintf(cTmpBuf,"%ld:%ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
				else
					sprintf(cTmpBuf,"%ld:%ld:%ld.%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
			}
			break;
		case SQL_C_INTERVAL_MINUTE_TO_SECOND:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
			{
				if (intervalTmp->intval.day_second.fraction == 0)
					sprintf(cTmpBuf,"-%ld:%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
				else
					sprintf(cTmpBuf,"-%ld:%ld.%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
			}
			else
			{
				if (intervalTmp->intval.day_second.fraction == 0)
					sprintf(cTmpBuf,"%ld:%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
				else
					sprintf(cTmpBuf,"%ld:%ld.%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
			}
			break;
		default:
			return IDS_07_006;
		}
		if (DataPtr == NULL)
		{
			DataPtr = cTmpBuf;
			DataLen = strlen(cTmpBuf);
		}
		if (Offset != 0)
		{
			if (targetPrecision > SHRT_MAX)
				outDataPtr = (unsigned char *)targetDataPtr + sizeof(UINT);
			else
				outDataPtr = (unsigned char *)targetDataPtr + sizeof(USHORT);
		}
		if (targetCharSet == SQLCHARSETCODE_UCS2)
			OutLen = targetLength - Offset -2 ; // Remove for Null Pointer;
		else
			OutLen = targetLength - Offset -1 ; // Remove for Null Pointer
		break;
	case SQL_REAL:
		if(CDataType == SQL_C_NUMERIC){
			ConvertCNumericToChar((SQL_NUMERIC_STRUCT*)srcDataPtr, cTmpBuf);
			srcLength = strlen(cTmpBuf);
			if ((retCode = ConvertCharToNumeric(cTmpBuf, srcLength, dTmp)) != SQL_SUCCESS)
				return retCode;
			if (dTmp < -FLT_MAX || dTmp > FLT_MAX)
				return IDS_22_003;
			fltTmp = (SFLOAT)dTmp;
			DataPtr = &fltTmp;
			DataLen = sizeof(fltTmp);
			break;
		}
	case SQL_TINYINT:
	case SQL_SMALLINT:
	case SQL_INTEGER:
	case SQL_FLOAT:
	case SQL_DOUBLE:
	case SQL_DECIMAL:
		switch (CDataType)
		{
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
				if(ODBCDataType == SQL_REAL || ODBCDataType == SQL_FLOAT ||ODBCDataType == SQL_DOUBLE)
					return IDS_07_006;
			default:
				break;
		}
		signedInteger = FALSE;
		unsignedInteger = FALSE;
		switch (CDataType)
		{
		case SQL_C_WCHAR:
			if (srcLength != SQL_NTS)
				srcLength = srcLength/2;
			// translate from UTF16
			if (WCharToUTF8((wchar_t*)srcDataPtr, srcLength, srcDataLocale, sizeof(srcDataLocale), (int*)&translateLength, (char*)errorMsg) != SQL_SUCCESS)
				return IDS_193_DRVTODS_ERROR;
			srcDataPtr = srcDataLocale;
			srcLength = translateLength;
		case SQL_C_CHAR:
			if (ODBCDataType != SQL_DECIMAL)
			{
				if ((retCode = ConvertCharToNumeric(srcDataPtr, srcLength, dTmp)) != SQL_SUCCESS)
					return retCode;
			}
			else // this is a patch should remove dTmp (double) and change it to tempVal64 (__int64) like in SQL_BIGINT.
			{
				if (srcLength == SQL_NTS)
					tempLen = strlen((const char *)srcDataPtr);
				else
					tempLen = srcLength;

				if( tempLen > sizeof( cTmpBuf ) - 1)
					return IDS_22_003;
				strncpy(cTmpBuf,(char*)srcDataPtr, tempLen);
				cTmpBuf[ tempLen ] = 0;
				if ((retCode = ConvertCharToInt64Num((char*)cTmpBuf, integralPart, 
							decimalPart, negative, leadZeros)) != 0)
				{
					// Return values -1 - Out of Range
					//		 -2 - Illegal numeric value
					if (retCode == -1)
						return IDS_22_003;
					if (retCode == -2)
						return IDS_22_005;
				}
				if(negative && targetUnsigned)
					return IDS_22_003_02;
                getMaxNum(targetPrecision, targetScale, integralMax, decimalMax);
				if ((integralPart < 0) || (integralPart > integralMax))
					return IDS_22_003;
				decimalDigits = 0;
				if (decimalPart > 0)
					decimalDigits = getDigitCount(decimalPart);
				if ((decimalPart > decimalMax) || ((decimalDigits + leadZeros) > targetScale))	
				{	
					retCode = IDS_01_S07; // Since SQL does not give truncation warning, we fake it
					// trim the decimalPart based one the scale
					// the number of digits in the decimal portion needs to be adjusted if it contain
					// leading zero(s) 
					decimalPart=decimalPart/pow((double)10, (int)(getDigitCount(decimalPart) + leadZeros - targetScale));
				}  
				useDouble = FALSE;
			}
			break;
		case SQL_C_SHORT:
		case SQL_C_SSHORT:
			dTmp = *(SSHORT *)srcDataPtr;
			signedInteger = TRUE;
			break;
		case SQL_C_USHORT:
			dTmp = *(USHORT *)srcDataPtr;
			unsignedInteger = TRUE;
			break;
		case SQL_C_TINYINT:
		case SQL_C_STINYINT: 
			dTmp = *(SCHAR *)srcDataPtr;
			signedInteger = TRUE;
			break;
		case SQL_C_UTINYINT:
		case SQL_C_BIT:
			dTmp = *(UCHAR *)srcDataPtr;
			break;
		case SQL_C_SLONG:
		case SQL_C_LONG:
			dTmp = *(SLONG *)srcDataPtr;
			signedInteger = TRUE;
			break;
		case SQL_C_ULONG:
			dTmp = *(ULONG *)srcDataPtr;
			unsignedInteger = TRUE;
			break;
		case SQL_C_SBIGINT:
			if (ODBCDataType != SQL_DECIMAL)
			{
				dTmp = *(__int64 *)srcDataPtr;
			}
			else // this is a patch should remove dTmp (double) and change it to tempVal64 (__int64) like in SQL_BIGINT.
			{
				leadZeros = 0;
				decimalPart = 0;
				integralPart = 0;
				negative = FALSE;
				integralPart = *(__int64 *)srcDataPtr;
				if (integralPart < 0)
				{
					integralPart = -integralPart;
					negative = TRUE;
				}
				useDouble = FALSE;
			}
			signedInteger = TRUE;
			break;
		case SQL_C_FLOAT:
			dTmp = *(SFLOAT *)srcDataPtr;
			break;
		case SQL_C_DOUBLE:
			dTmp = *(DOUBLE *)srcDataPtr;
			break;
		case SQL_C_BINARY:
			DataPtr = srcDataPtr;
			break;
		case SQL_C_DEFAULT:
			DataPtr = srcDataPtr;
			break;
		case SQL_C_NUMERIC:
			ConvertCNumericToChar((SQL_NUMERIC_STRUCT*)srcDataPtr, cTmpBuf);
			
			if ((retCode = ConvertCharToInt64Num((char*)cTmpBuf, integralPart, 
							decimalPart, negative, leadZeros)) != 0)
			{
// Return values -1 - Out of Range
//				 -2 - Illegal numeric value

				if (retCode == -1)
					return IDS_22_003;
				if (retCode == -2)
					return IDS_22_005;
			}
			if(negative && targetUnsigned)
				return IDS_22_003_02;
            getMaxNum(targetPrecision, targetScale, integralMax, decimalMax);
			if ((integralPart < 0) || (integralPart > integralMax)) 
				return IDS_22_003;
			decimalDigits = 0;
			if (decimalPart > 0)
				decimalDigits = getDigitCount(decimalPart);
			if ((decimalPart > decimalMax) || ((decimalDigits + leadZeros) > targetScale))
			{
				retCode = IDS_01_S07;
				// trim the decimalPart based one the scale
				// the number of digits in the decimal portion needs to be adjusted if it contain
				// leading zero(s) 
				decimalPart=decimalPart/pow((double)10, (int)(getDigitCount(decimalPart) + leadZeros - targetScale));
			}
			useDouble = FALSE;
			break;
		case SQL_C_INTERVAL_MONTH:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				dTmp = -(intervalTmp->intval.year_month.month);
			else
				dTmp = intervalTmp->intval.year_month.month;
			break;
		case SQL_C_INTERVAL_YEAR:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				dTmp = -(intervalTmp->intval.year_month.year);
			else
				dTmp = intervalTmp->intval.year_month.year;
			break;
		case SQL_C_INTERVAL_DAY:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				dTmp = -(intervalTmp->intval.day_second.day);
			else
				dTmp = intervalTmp->intval.day_second.day;
			break;
		case SQL_C_INTERVAL_HOUR:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				dTmp = -(intervalTmp->intval.day_second.hour);
			else
				dTmp = intervalTmp->intval.day_second.hour;
			break;
		case SQL_C_INTERVAL_MINUTE:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				dTmp = -(intervalTmp->intval.day_second.minute);
			else
				dTmp = intervalTmp->intval.day_second.minute;
			break;
		case SQL_C_INTERVAL_SECOND:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				dTmp = -(intervalTmp->intval.day_second.second);
			else
				dTmp = intervalTmp->intval.day_second.second;
			break;
		default:
			return IDS_07_006;
		}
		if (DataPtr == NULL)
		{
			if( useDouble )
			{

				switch (ODBCDataType)
				{
				case SQL_TINYINT:
					if (targetUnsigned)
					{
						if (dTmp < 0)
							return IDS_22_003_02;  //negValue in unsigned column
						if (dTmp > UCHAR_MAX)
							return IDS_22_003;
						utTmp = (UCHAR)dTmp;
						if (dTmp != utTmp)
							retCode = IDS_01_S07;
						DataPtr = &utTmp;
						DataLen = sizeof(UCHAR);
					}
					else
					{
						if (unsignedInteger)
						{
							if (dTmp < 0 || dTmp > UCHAR_MAX)
								return IDS_22_003;
							tTmp = (SCHAR)dTmp;
						}
						else
						{
							if (dTmp < SCHAR_MIN || dTmp > SCHAR_MAX)
								return IDS_22_003;
							tTmp = (SCHAR)dTmp;
							if (dTmp != tTmp)
								retCode = IDS_01_S07;
						}
						DataPtr = &tTmp;
						DataLen = sizeof(SCHAR);
					}
					break;
				case SQL_SMALLINT:
					if (targetUnsigned)
					{
						if ( dTmp < 0 )
							return IDS_22_003_02;  //negValue in unsigned column
						if (dTmp > USHRT_MAX)
							return IDS_22_003;
						usTmp = (USHORT)dTmp;
						if  (dTmp != usTmp)
							retCode = IDS_01_S07;
						DataPtr = &usTmp;
						DataLen = sizeof(usTmp);
					}
					else
					{
						if (unsignedInteger)
						{
							if (dTmp < 0 || dTmp > USHRT_MAX)
								return IDS_22_003;
							sTmp = (SHORT)dTmp;
						}
						else
						{
							if (dTmp < SHRT_MIN || dTmp > SHRT_MAX)
								return IDS_22_003;
							sTmp = (SHORT)dTmp;
							if  (dTmp != sTmp)
								retCode = IDS_01_S07;
						}
						DataPtr = &sTmp;
						DataLen = sizeof(sTmp);
					}
					break;
				case SQL_INTEGER:
					if (targetUnsigned)
					{
						if (dTmp < 0)
							return IDS_22_003_02;//negValue in unsigned col error
						if (dTmp > ULONG_MAX)
							return IDS_22_003;
						ulTmp = (ULONG)dTmp;
						if  (dTmp != ulTmp)
							retCode = IDS_01_S07;
						DataPtr = &ulTmp;
						DataLen = sizeof(ulTmp);
					}
					else
					{
						if (unsignedInteger)
						{
							if (dTmp < 0 || dTmp > ULONG_MAX)
								return IDS_22_003;
							lTmp = (LONG)dTmp;
						}
						else
						{
							if (dTmp < LONG_MIN || dTmp > LONG_MAX)
								return IDS_22_003;
							lTmp = (LONG)dTmp;
							if  (dTmp != lTmp)
								retCode = IDS_01_S07;
						}
						DataPtr = &lTmp;
						DataLen = sizeof(lTmp);
					}
					break;
				case SQL_REAL:
					if (dTmp < -FLT_MAX || dTmp > FLT_MAX)
						return IDS_22_003;
					fltTmp = (SFLOAT)dTmp;
					DataPtr = &fltTmp;
					DataLen = sizeof(fltTmp);
					break;
				case SQL_DOUBLE:
				case SQL_FLOAT:
					DataPtr = &dTmp;
					DataLen = sizeof(dTmp);
					break;
				case SQL_DECIMAL:
					if (targetPrecision >= sizeof(cTmpBuf))
						return IDS_22_003;
					dTmp1 = pow((short)10,(short)(targetPrecision-targetScale+1)); 
					if (targetUnsigned)
					{
						if ( dTmp < 0 )
							return IDS_22_003_02;  //negValue in unsigned column
						if (dTmp > dTmp1)
							return IDS_22_003;
						
						tempPtr = _fcvt(dTmp, targetScale, &dec, &sign);
						tempLen = strlen(tempPtr);
						tempLen1 = (short)(targetPrecision-tempLen);

						if (tempLen1 < 0)
							return IDS_22_003;

						memset((void *)cTmpBuf, '0', tempLen1);
						strncpy((char *)(cTmpBuf+tempLen1), tempPtr, tempLen);
					}
					else
					{
						if (dTmp < -dTmp1 || dTmp > dTmp1)
							return IDS_22_003;
					
						tempPtr = _fcvt(dTmp, targetScale, &dec, &sign);
						tempLen = strlen(tempPtr);
						tempLen1 = (short)(targetPrecision-tempLen);

						if (tempLen1 < 0)
							return IDS_22_003;

						memset((void *)cTmpBuf, '0', tempLen1);
						strncpy((char *)(cTmpBuf+tempLen1), tempPtr, tempLen);
						if (sign)
							*cTmpBuf = (UCHAR)(*cTmpBuf | (UCHAR)0x80);
					}
					DataPtr = cTmpBuf;
					DataLen = targetPrecision;
					break;
				default:
					return IDS_07_006;
				}
			}
			else
			{

				if (targetScale)
				{
					for (i = 0,tempVal64 = 1; i < targetScale ;  i++)
						tempVal64 *= 10;
					tempVal64 = tempVal64 * integralPart;
					decimalDigits = 0;
					if (decimalPart > 0) 
						decimalDigits = getDigitCount(decimalPart);
					scaleOffset = 0;
					if (leadZeros < targetScale)
					  scaleOffset = targetScale - decimalDigits - leadZeros;
					if (scaleOffset < 0)
					{
//NUMERIC_VALUE_OUT_OF_RANGE_ERROR
						return IDS_22_003;
					}
					for (i =0, tempScaleVal64 = decimalPart ; i < scaleOffset ; i++)
						tempScaleVal64 *= 10;
					tempVal64 += tempScaleVal64;
				}
				else
				{
//NUMERIC_DATA_TRUNCATED_ERROR
					if (decimalPart != 0)
						retCode = IDS_01_S07;
					tempVal64 = integralPart;
				}
				if (negative)
					tempVal64 = -tempVal64;

				switch( SQLDataType )
				{
				case SQLTYPECODE_TINYINT_UNSIGNED:
					if (tempVal64 < 0)
						return IDS_22_003_02;
					if (tempVal64 > UCHAR_MAX)
						return IDS_22_003;
					utTmp = (UCHAR)tempVal64;
					if (tempVal64 != utTmp)
						retCode = IDS_01_S07;
					DataPtr = &utTmp;
					DataLen = sizeof(UCHAR);
					break;
				case SQLTYPECODE_TINYINT:
					if (tempVal64 < SCHAR_MIN || tempVal64 > SCHAR_MAX)
						return IDS_22_003_02;
					tTmp = (SCHAR)tempVal64;
					if (tempVal64 != tTmp)
						retCode = IDS_01_S07;
					DataPtr = &tTmp;
					DataLen = sizeof(SCHAR);
					break;
				case SQLTYPECODE_SMALLINT_UNSIGNED:
					if (tempVal64 < 0)
						return IDS_22_003_02;
					if ((USHORT)tempVal64 > USHRT_MAX)
						return IDS_22_003;
					usTmp = (USHORT)tempVal64;
					if  (tempVal64 != usTmp)
						retCode = IDS_01_S07;
					DataPtr = &usTmp;
					DataLen = sizeof(USHORT);
					break;
				case SQLTYPECODE_SMALLINT:
					if (tempVal64 < SHRT_MIN || tempVal64 > SHRT_MAX)
						return IDS_22_003;
					sTmp = (SHORT)tempVal64;
					if  (tempVal64 != sTmp)
						retCode = IDS_01_S07;
					DataPtr = &sTmp;
					DataLen = sizeof(sTmp);
					break;
				case SQLTYPECODE_INTEGER_UNSIGNED:
					if (tempVal64 < 0)
					       return IDS_22_003_02;
					if ((ULONG)tempVal64 > ULONG_MAX)
						return IDS_22_003;
					ulTmp = (ULONG)tempVal64;
					if  (tempVal64 != ulTmp)
						retCode = IDS_01_S07;
					DataPtr = &ulTmp;
					DataLen = sizeof(ulTmp);
					break;
				case SQLTYPECODE_INTEGER:
					if (tempVal64 < LONG_MIN || tempVal64 > LONG_MAX)
						return IDS_22_003;
					lTmp = (LONG)tempVal64;
					if  (tempVal64 != lTmp)
						retCode = IDS_01_S07;
					DataPtr = &lTmp;
					DataLen = sizeof(lTmp);
					break;
				case SQLTYPECODE_IEEE_FLOAT:
					if (tempVal64 < -FLT_MAX || tempVal64 > FLT_MAX)
						return IDS_22_003;
					fltTmp = (FLOAT)tempVal64;
					if  (tempVal64 != fltTmp)
						retCode = IDS_01_S07;
					DataPtr = &fltTmp;
					DataLen = sizeof(fltTmp);
					break;
				case SQLTYPECODE_IEEE_DOUBLE:
					if (tempVal64 < -DBL_MAX || tempVal64 > DBL_MAX)
						return IDS_22_003;
					dTmp = (DOUBLE)tempVal64;
					if  (tempVal64 != dTmp)
						retCode = IDS_01_S07;
					DataPtr = &dTmp;
					DataLen = sizeof(dTmp);
					break;
				case SQLTYPECODE_DECIMAL_UNSIGNED:
				case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED:
					if(negative)
						return IDS_22_003_02;
				case SQLTYPECODE_DECIMAL:
				case SQLTYPECODE_DECIMAL_LARGE:
					if(negative)
						tempVal64 = -tempVal64;
					sprintf(cTmpBuf, "%0*I64d", targetPrecision, tempVal64);
					if (negative)
						*cTmpBuf = (UCHAR)(*cTmpBuf | (UCHAR)0x80);
					DataPtr = cTmpBuf;
					DataLen = strlen(cTmpBuf);
					break;
				case SQLTYPECODE_LARGEINT:
				default:
					DataPtr = &tempVal64;
					DataLen = sizeof(tempVal64);
					break;
				}


			}

		}
		else
		{
			switch (ODBCDataType)
			{
			case SQL_TINYINT:
				DataLen = sizeof(SCHAR);
				break;
			case SQL_SMALLINT:
				DataLen = sizeof(SHORT);
				break;
			case SQL_INTEGER:
				DataLen = sizeof(LONG);
				break;
			case SQL_REAL:
				DataLen = sizeof(FLOAT);
				break;
			case SQL_DOUBLE:
			case SQL_FLOAT:
				DataLen = sizeof(DOUBLE);
				break;
			default:
				return IDS_07_006;
			}
		}
		break;
	case SQL_BIGINT:
		switch (CDataType)
		{
		case SQL_C_WCHAR:
			if (srcLength != SQL_NTS)
				srcLength = srcLength/2;
			// translate from UTF16
			if (WCharToUTF8((wchar_t*)srcDataPtr, srcLength, srcDataLocale, sizeof(srcDataLocale), (int*)&translateLength, (char*)errorMsg) != SQL_SUCCESS)
				return IDS_193_DRVTODS_ERROR;
			srcDataPtr = srcDataLocale;
			srcLength = translateLength;
		case SQL_C_CHAR:
			{
				retCode = ConvertCharToInt64(srcDataPtr, srcLength, tempVal64);
				if (retCode != SQL_SUCCESS) 
					return retCode;
			}
			break;
		case SQL_C_SHORT:
		case SQL_C_SSHORT:
			tempVal64 = *(SSHORT *)srcDataPtr;
			break;
		case SQL_C_USHORT:
			tempVal64 = *(USHORT *)srcDataPtr;
			break;
		case SQL_C_TINYINT:
		case SQL_C_STINYINT: 
			tempVal64 = *(SCHAR *)srcDataPtr;
			break;
		case SQL_C_UTINYINT:
		case SQL_C_BIT:
			tempVal64 = *(UCHAR *)srcDataPtr;
			break;
		case SQL_C_SLONG:
		case SQL_C_LONG:
			tempVal64 = *(SLONG *)srcDataPtr;
			break;
		case SQL_C_ULONG:
			tempVal64 = *(ULONG *)srcDataPtr;
			break;
		case SQL_C_FLOAT:
			tempVal64 = *(SFLOAT *)srcDataPtr;
			break;
		case SQL_C_DOUBLE:
			tempVal64 = *(DOUBLE *)srcDataPtr;
			break;
		case SQL_C_BINARY:
			DataPtr = srcDataPtr;
			break;
		case SQL_C_DEFAULT:
			if (ODBCAppVersion >= SQL_OV_ODBC3)
				DataPtr = srcDataPtr;
			else
			{
				retCode = ConvertCharToInt64(srcDataPtr, srcLength, tempVal64);
				if (retCode!= SQL_SUCCESS) 
					return retCode;
			}
			break;
		case SQL_C_SBIGINT:
			tempVal64 = *(__int64 *)srcDataPtr;
			break;
		case SQL_C_NUMERIC:
			ConvertCNumericToChar((SQL_NUMERIC_STRUCT*)srcDataPtr, cTmpBuf);
			srcLength = strlen(cTmpBuf);
			retCode = ConvertCharToInt64((char*)cTmpBuf, srcLength, tempVal64);
			if (retCode != SQL_SUCCESS)
				return retCode;
			break;
		case SQL_C_INTERVAL_MONTH:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				tempVal64 = -(intervalTmp->intval.year_month.month);
			else
				tempVal64 = intervalTmp->intval.year_month.month;
			break;
		case SQL_C_INTERVAL_YEAR:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				tempVal64 = -(intervalTmp->intval.year_month.year);
			else
				tempVal64 = intervalTmp->intval.year_month.year;
			break;
		case SQL_C_INTERVAL_DAY:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				tempVal64 = -(intervalTmp->intval.day_second.day);
			else
				tempVal64 = intervalTmp->intval.day_second.day;
			break;
		case SQL_C_INTERVAL_HOUR:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				tempVal64 = -(intervalTmp->intval.day_second.hour);
			else
				tempVal64 = intervalTmp->intval.day_second.hour;
			break;
		case SQL_C_INTERVAL_MINUTE:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				tempVal64 = -(intervalTmp->intval.day_second.minute);
			else
				tempVal64 = intervalTmp->intval.day_second.minute;
			break;
		case SQL_C_INTERVAL_SECOND:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				tempVal64 = -(intervalTmp->intval.day_second.second);
			else
				tempVal64 = intervalTmp->intval.day_second.second;
			break;
		default:
			return IDS_07_006;
		}
		if (DataPtr == NULL)
			DataPtr = &tempVal64;
		DataLen = sizeof(tempVal64);
		break;
	case SQL_NUMERIC:
		// for R2.3 SP2 release BigNum is only supported for the following data type 
		//     SQL_C_DEFAULT, SQL_C_CHAR, SQL_C_FLOAT, SQL_C_DOUBLE
		// other data types will be supported in future release(?) need to reject them
		if (((SQLDataType == SQLTYPECODE_NUMERIC) && (targetPrecision > 18)) || 
			((SQLDataType == SQLTYPECODE_NUMERIC_UNSIGNED) && (targetPrecision > 9))) 
		{ //Bignum 
			switch (CDataType)
		       	{
				case SQL_C_DEFAULT:
				case SQL_C_CHAR:
				case SQL_C_WCHAR:
				case SQL_C_FLOAT:
				case SQL_C_DOUBLE:
				case SQL_C_NUMERIC:
					break;
				default:
					return IDS_S1_006;	
			}
		}

		switch (CDataType)
		{
		case SQL_C_DEFAULT:
			if (ODBCAppVersion >= SQL_OV_ODBC3)
			{

			}						// Want it fall thru and treat it like SQL_C_CHAR
		case SQL_C_WCHAR:
			if (srcLength != SQL_NTS)
				srcLength = srcLength/2;
			// translate from UTF16
			if (WCharToUTF8((wchar_t*)srcDataPtr, srcLength, srcDataLocale, sizeof(srcDataLocale), (int*)&translateLength, (char*)errorMsg) != SQL_SUCCESS)
				return IDS_193_DRVTODS_ERROR;
			srcDataPtr = srcDataLocale;
			srcLength = translateLength;
		case SQL_C_CHAR:
			if (srcLength == SQL_NTS)
				tempLen = strlen((const char *)srcDataPtr);
			else
				tempLen = srcLength;

			if( tempLen > sizeof( cTmpBuf ) - 1)
				return IDS_22_003;

			strncpy(cTmpBuf,(char*)srcDataPtr, tempLen);
			cTmpBuf[ tempLen ] = '\0';
			
			if( ((SQLDataType == SQLTYPECODE_NUMERIC) && (targetPrecision > 18)) || 
				((SQLDataType == SQLTYPECODE_NUMERIC_UNSIGNED) && (targetPrecision > 9))) //for bignum support
			{ //Bignum 

				retCode = Ascii_To_Bignum_Helper(cTmpBuf,
					tempLen,
					(char*)cTmpBuf2, 
					targetLength, 
					targetPrecision,
					targetScale,
					SQLDataType,
					&dataTruncatedWarning);

				if(retCode != SQL_SUCCESS)
					return retCode;
			
				useDouble = FALSE;
				if (DataPtr == NULL) 
//					DataPtr = targetDataPtr;
					DataPtr = (char*)cTmpBuf2;

				DataLen = targetLength;
			} else {
				if ((retCode = ConvertCharToInt64Num(cTmpBuf, integralPart, 
							decimalPart, negative, leadZeros)) != 0)
				{
					// Return values -1 - Out of Range
					//				 -2 - Illegal numeric 
					if (retCode == -1)
						return IDS_22_003;
					if (retCode == -2)
						return IDS_22_005;
				}
				if(negative && targetUnsigned)
					return IDS_22_003_02;
                getMaxNum(targetPrecision, targetScale, integralMax, decimalMax);
				if ((integralPart < 0) || (integralPart > integralMax))
					return IDS_22_003;
				decimalDigits = 0;
				if (decimalPart > 0)
					decimalDigits = getDigitCount(decimalPart);
				if ((decimalPart > decimalMax) || ((decimalDigits + leadZeros) > targetScale))
				{
					retCode = IDS_01_S07;
					// trim the decimalPart based one the scale
					// the number of digits in the decimal portion needs to be adjusted if it contain
					// leading zero(s) 
					decimalPart=decimalPart/pow((double)10, (int)(getDigitCount(decimalPart) + leadZeros - targetScale));
				} 
				useDouble = FALSE;
			}
			break;
		case SQL_C_NUMERIC:
			ConvertCNumericToChar((SQL_NUMERIC_STRUCT*)srcDataPtr, cTmpBuf); 
			tempLen = strlen(cTmpBuf);
			if (((SQLDataType == SQLTYPECODE_NUMERIC) && (targetPrecision > 18)) || 
				((SQLDataType == SQLTYPECODE_NUMERIC_UNSIGNED) && (targetPrecision > 9))) //for bignum support
			{ //Bignum 

				retCode = Ascii_To_Bignum_Helper(cTmpBuf,
					tempLen,
					(char*)targetDataPtr,
					targetLength, 
					targetPrecision,
					targetScale,
					SQLDataType,
					&dataTruncatedWarning);

				if(retCode != SQL_SUCCESS)
					return retCode;
			
				useDouble = FALSE;
				if (DataPtr == NULL) 
					DataPtr = targetDataPtr;

				DataLen = targetLength;
			} 
			else {
				if ((retCode = ConvertCharToInt64Num(cTmpBuf, integralPart, 
							decimalPart, negative, leadZeros)) != 0)
				{
// Return values -1 - Out of Range
//				 -2 - Illegal numeric value

					if (retCode == -1)
						return IDS_22_003;
					if (retCode == -2)
						return IDS_22_005;
				}
					if(negative && targetUnsigned)
						return IDS_22_003_02;
                getMaxNum(targetPrecision, targetScale, integralMax, decimalMax);
				if ((integralPart < 0) || (integralPart > integralMax)) 
					return IDS_22_003;
				decimalDigits = 0;
				if (decimalPart > 0)
					decimalDigits = getDigitCount(decimalPart);
				if ((decimalPart > decimalMax) || ((decimalDigits + leadZeros) > targetScale))
				{
					retCode = IDS_01_S07;
					// trim the decimalPart based one the scale
					// the number of digits in the decimal portion needs to be adjusted if it contain
					// leading zero(s) 
					decimalPart=decimalPart/pow((double)10, (int)(getDigitCount(decimalPart) + leadZeros - targetScale));
				}
				useDouble = FALSE;
			}
			break;

		case SQL_C_FLOAT:
		case SQL_C_DOUBLE:

			if(CDataType == SQL_C_DOUBLE)
				dTmp = *(DOUBLE *)srcDataPtr;
			else
				dTmp = *(SFLOAT *)srcDataPtr;
			negative = (dTmp < 0)? 1: 0;
			if( ((SQLDataType == SQLTYPECODE_NUMERIC) && (targetPrecision > 18)) || 
				((SQLDataType == SQLTYPECODE_NUMERIC_UNSIGNED) && (targetPrecision > 9)))
			{ //Bignum 

				if(CDataType == SQL_C_DOUBLE)
				{
					if (!double_to_char (dTmp, DBL_DIG, cTmpBuf, sizeof(cTmpBuf)))
						dataTruncatedWarning = TRUE;
				}
				else
				{
					if (!double_to_char (dTmp, FLT_DIG, cTmpBuf, sizeof(cTmpBuf)))
						dataTruncatedWarning = TRUE;
				}


				retCode = Ascii_To_Bignum_Helper(cTmpBuf,
					strlen(cTmpBuf),
					(char*)cTmpBuf2,
					targetLength, 
					targetPrecision,
					targetScale,
					SQLDataType,
					&dataTruncatedWarning);

				if(retCode != SQL_SUCCESS)
					return retCode;
			
				useDouble = FALSE;
				if (DataPtr == NULL) 
					DataPtr = cTmpBuf2;

				DataLen = targetLength;
				memcpy(outDataPtr, DataPtr, DataLen);
				if (byteSwap) 
				{
					if (Datatype_Dependent_Swap((BYTE *)outDataPtr, SQLDataType, targetCharSet, DataLen, IEEE_TO_TANDEM, CDataType) != STATUS_OK)
						return IDS_HY_000;
				}

				if(dataTruncatedWarning)
					return IDS_01_S07;
				else
					return SQL_SUCCESS;

			}
			break;

		case SQL_C_SHORT:
		case SQL_C_SSHORT:
			dTmp = *(SSHORT *)srcDataPtr;
			break;
		case SQL_C_TINYINT:
		case SQL_C_STINYINT:
			dTmp = *(SCHAR *)srcDataPtr;
			break;
		case SQL_C_SLONG:
		case SQL_C_LONG:
			dTmp = *(SLONG *)srcDataPtr;
			break;
		case SQL_C_USHORT:
			dTmp = *(USHORT *)srcDataPtr;
			break;
		case SQL_C_UTINYINT:
		case SQL_C_BIT:
			dTmp = *(UCHAR *)srcDataPtr;
			break;
		case SQL_C_ULONG:
			dTmp = *(ULONG *)srcDataPtr;
			break;
		case SQL_C_BINARY:
			if (srcLength != OutLen)
				return IDS_22_003;
			DataPtr = srcDataPtr;
			DataLen = OutLen;
			break;
		case SQL_C_SBIGINT:
			integralPart = *(__int64*)srcDataPtr;

			negative = (integralPart < 0)? 1: 0;
			integralPart = (integralPart < 0)? -integralPart: integralPart;
			decimalPart = 0;
			leadZeros = 0;
            getMaxNum(targetPrecision, targetScale, integralMax, decimalMax);
			if ( integralPart > integralMax )
				return IDS_22_003;

			useDouble = FALSE;
			break;
		case SQL_C_INTERVAL_MONTH:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				dTmp = -(intervalTmp->intval.year_month.month);
			else
				dTmp = intervalTmp->intval.year_month.month;
			break;
		case SQL_C_INTERVAL_YEAR:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				dTmp = -(intervalTmp->intval.year_month.year);
			else
				dTmp = intervalTmp->intval.year_month.year;
			break;
		case SQL_C_INTERVAL_DAY:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				dTmp = -(intervalTmp->intval.day_second.day);
			else
				dTmp = intervalTmp->intval.day_second.day;
			break;
		case SQL_C_INTERVAL_HOUR:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				dTmp = -(intervalTmp->intval.day_second.hour);
			else
				dTmp = intervalTmp->intval.day_second.hour;
			break;
		case SQL_C_INTERVAL_MINUTE:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				dTmp = -(intervalTmp->intval.day_second.minute);
			else
				dTmp = intervalTmp->intval.day_second.minute;
			break;
		case SQL_C_INTERVAL_SECOND:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			if (intervalTmp->interval_sign == SQL_TRUE)
				dTmp = -(intervalTmp->intval.day_second.second);
			else
				dTmp = intervalTmp->intval.day_second.second;
			break;
		default:
			return IDS_07_006;
		}
		if (DataPtr == NULL)
		{
			if (useDouble)
			{
				if( targetUnsigned && ( dTmp < 0 || negative ))
					return IDS_22_003_02;	//negValue in unsigned column

				dTmp1 = pow((short)10, (short)(targetPrecision-targetScale+1)); 
				if (dTmp < -dTmp1 || dTmp > dTmp1)
					return IDS_22_003;
				scaleOffset = pow(10, targetScale);		// This value always multplied to srcValue
											// since SQL stores it as a implied decimal point
											// 1.0 for NUMERIC (4,2) value is stored as 100
				dTmp *= scaleOffset;
				switch (SQLDataType)
				{
				case SQLTYPECODE_TINYINT_UNSIGNED:
					utTmp = (UCHAR)dTmp;
					DataPtr = &utTmp;
					DataLen = sizeof(UCHAR);
					break;
				case SQLTYPECODE_TINYINT:
					tTmp = (SCHAR)dTmp;
					DataPtr = &tTmp;
					DataLen = sizeof(SCHAR);
					break;
				case SQLTYPECODE_SMALLINT_UNSIGNED:
					usTmp = (USHORT)dTmp;
					DataPtr = &usTmp;
					DataLen = sizeof(usTmp);
					break;
				case SQLTYPECODE_SMALLINT:
					sTmp = (SHORT)dTmp;
					DataPtr = &sTmp;
					DataLen = sizeof(sTmp);
					break;
				case SQLTYPECODE_INTEGER_UNSIGNED:
					ulTmp = (ULONG)dTmp;
					DataPtr = &ulTmp;
					DataLen = sizeof(ulTmp);
					break;
				case SQLTYPECODE_INTEGER:
					lTmp = (LONG)dTmp;
					DataPtr = &lTmp;
					DataLen = sizeof(lTmp);
					break;
				case SQLTYPECODE_LARGEINT:
					tempVal64 = (__int64)dTmp;
					DataPtr = &tempVal64;
					DataLen = sizeof(tempVal64);
					break;
				default:
					return IDS_07_006;
				}
			}
			else
			{
				if( targetUnsigned && negative )
					return IDS_22_003_02;	//negValue in unsigned column
				
				if (targetScale)
				{
					for (i = 0,tempVal64 = 1; i < targetScale ;  i++)
						tempVal64 *= 10;
					tempVal64 = tempVal64 * integralPart;
					decimalDigits = 0;
					if (decimalPart > 0) 
						decimalDigits = getDigitCount(decimalPart);
					scaleOffset = 0;
					if (leadZeros < targetScale)
					  scaleOffset = targetScale - decimalDigits - leadZeros;
					if (scaleOffset < 0)
					{
//NUMERIC_VALUE_OUT_OF_RANGE_ERROR
						return IDS_22_003;
					}

					for (i =0, tempScaleVal64 = decimalPart ; i < scaleOffset ; i++)
						tempScaleVal64 *= 10;
					tempVal64 += tempScaleVal64;
				}
				else
				{
//NUMERIC_DATA_TRUNCATED_ERROR
					if (decimalPart != 0)
						retCode = IDS_01_S07;
					tempVal64 = integralPart;
				}
				if (negative)
					tempVal64 = -tempVal64;

				switch( SQLDataType )
				{
				case SQLTYPECODE_SMALLINT_UNSIGNED:
					if (tempVal64 < 0)
					       return IDS_22_003_02;
					if ((USHORT)tempVal64 > USHRT_MAX)
						return IDS_22_003;
					usTmp = (USHORT)tempVal64;
					if  (tempVal64 != usTmp)
						retCode = IDS_01_S07;
					DataPtr = &usTmp;
					DataLen = sizeof(USHORT);
					break;
				case SQLTYPECODE_SMALLINT:
					if (tempVal64 < SHRT_MIN || tempVal64 > SHRT_MAX)
						return IDS_22_003;
					sTmp = (SHORT)tempVal64;
					if  (tempVal64 != sTmp)
						retCode = IDS_01_S07;
					DataPtr = &sTmp;
					DataLen = sizeof(sTmp);
					break;
				case SQLTYPECODE_INTEGER_UNSIGNED:
					// for 64 bit Solaris/AIX (with XlC cplr), 
					// tempVal64 is a signed LONG LONG, 
					// ULONG_MAX is unsigned LONG of 'FFFFFFF....', 
					// somehow it will evaluate tempVal64 GT ULONG_MAX
					// so cast the tempVal64 to (ULONG)
					if (tempVal64 < 0)
					       return IDS_22_003_02;
					if ((ULONG)tempVal64 > ULONG_MAX)
						return IDS_22_003;
					ulTmp = (ULONG)tempVal64;
					if  (tempVal64 != ulTmp)
						retCode = IDS_01_S07;
					DataPtr = &ulTmp;
					DataLen = sizeof(ulTmp);
					break;
				case SQLTYPECODE_INTEGER:
					if (tempVal64 < LONG_MIN || tempVal64 > LONG_MAX)
						return IDS_22_003;
					lTmp = (LONG)tempVal64;
					if  (tempVal64 != lTmp)
						retCode = IDS_01_S07;
					DataPtr = &lTmp;
					DataLen = sizeof(lTmp);
					break;						
				case SQLTYPECODE_LARGEINT:
				default:
					DataPtr = &tempVal64;
					DataLen = sizeof(tempVal64);
					break;
				}
			}
		}
		break; // End of case for SQL_NUMERIC
	case SQL_DATE:
	case SQL_TYPE_DATE:
		switch (CDataType)
		{
		case SQL_C_WCHAR:
			if (srcLength != SQL_NTS)
				srcLength = srcLength/2;
			// translate from UTF16
			if (WCharToUTF8((wchar_t*)srcDataPtr, srcLength, srcDataLocale, sizeof(srcDataLocale), (int*)&translateLength, (char*)errorMsg) != SQL_SUCCESS)
				return IDS_193_DRVTODS_ERROR;
			srcDataPtr = srcDataLocale;
			srcLength = translateLength;
		case SQL_C_CHAR:
			if (ConvertCharToSQLDate(srcDataPtr, srcLength, ODBCDataType, &SQLDate, targetPrecision) 
					!= SQL_SUCCESS)
				return IDS_22_008;
			break;
		case SQL_C_DATE:
		case SQL_C_TYPE_DATE:
		case SQL_C_DEFAULT:
			dateTmp = (DATE_STRUCT *)srcDataPtr;
						
			for (i = 0 ; i < 8 ; i++)
				datetime_parts[i] = 0;
			datetime_parts[0] = dateTmp->year;
			datetime_parts[1] = dateTmp->month;
			datetime_parts[2] = dateTmp->day;
			if (!checkDatetimeValue(datetime_parts))
				return IDS_22_008;

			SQLDate.year = dateTmp->year;
			SQLDate.month = dateTmp->month;
			SQLDate.day = dateTmp->day;
			break;
		case SQL_C_TIMESTAMP:
		case SQL_C_TYPE_TIMESTAMP:
			timestampTmp = (TIMESTAMP_STRUCT *)srcDataPtr;
// SQL/MX fraction precision is max 6 digits but ODBC accepts max precision 9 digits 
// conversion from nano to micro fraction of second
			ulFraction = (UDWORD)timestampTmp->fraction;
			if (targetPrecision > 0)
			{
				ulFraction /= 1000;
				sprintf(cTmpBuf, "%06lu", ulFraction);
				cTmpBuf[targetPrecision] = 0;
				strcpy(cTmpFraction,cTmpBuf);
			}
			
			for (i = 0 ; i < 8 ; i++)
				datetime_parts[i] = 0;
			datetime_parts[0] = timestampTmp->year;
			datetime_parts[1] = timestampTmp->month;
			datetime_parts[2] = timestampTmp->day;
			datetime_parts[3] = timestampTmp->hour;
			datetime_parts[4] = timestampTmp->minute;
			datetime_parts[5] = timestampTmp->second;
			datetime_parts[6] = (short)(ulFraction/1000);
			datetime_parts[7] = (short)(ulFraction%1000);
			if (!checkDatetimeValue(datetime_parts))
				return IDS_22_008;

			SQLDate.year = timestampTmp->year;
			SQLDate.month = timestampTmp->month;
			SQLDate.day = timestampTmp->day;
/*
			if (timestampTmp->hour != 0 || timestampTmp->minute != 0 || timestampTmp->second != 0 ||
				timestampTmp->fraction != 0)
			{
				if (ODBCAppVersion >= SQL_OV_ODBC3)
					return IDS_22_008;
				else
					retCode = IDS_22_001;
			}
*/
			break;
		case SQL_C_BINARY:
			if (srcLength != targetLength)
				return IDS_22_003;
			DataPtr = srcDataPtr;
			break;
		default:
			return IDS_07_006;
		}
		if (DataPtr == NULL)
			DataPtr = &SQLDate;
		DataLen = targetLength;
		
		if (CDataType != SQL_C_BINARY)
		{
			pSQLDate = (DATE_TYPES*)DataPtr;
			switch (SQLDatetimeCode)
			{
			case SQLDTCODE_YEAR:
				DataLen = sprintf(cTmpBuf, "%04d", pSQLDate->year);
				break;
			case SQLDTCODE_YEAR_TO_MONTH:
				DataLen = sprintf(cTmpBuf, "%04d-%02d", pSQLDate->year,pSQLDate->month);
				break;
			case SQLDTCODE_MONTH:
				DataLen = sprintf(cTmpBuf, "%02d", pSQLDate->month);
				break;
			case SQLDTCODE_MONTH_TO_DAY:
				DataLen = sprintf(cTmpBuf, "%02d-%02d", pSQLDate->month,pSQLDate->day);
				break;
			case SQLDTCODE_DAY:
				DataLen = sprintf(cTmpBuf, "%02d", pSQLDate->day);
				break;
			default:
				DataLen = sprintf(cTmpBuf, "%04d-%02d-%02d", pSQLDate->year,pSQLDate->month,pSQLDate->day);
			}
			DataPtr = cTmpBuf;
			if (DataLen != targetLength)
				return IDS_22_003;
		}
		break; // End of case for SQL_DATE
	case SQL_TIME:
	case SQL_TYPE_TIME:
		switch (CDataType)
		{
		case SQL_C_WCHAR:
			if (srcLength != SQL_NTS)
				srcLength = srcLength/2;
			// translate from UTF16 to
			if (WCharToUTF8((wchar_t*)srcDataPtr, srcLength, srcDataLocale, sizeof(srcDataLocale), (int*)&translateLength, (char*)errorMsg) != SQL_SUCCESS)
				return IDS_193_DRVTODS_ERROR;
			srcDataPtr = srcDataLocale;
			srcLength = translateLength;
		case SQL_C_CHAR:
			if (ConvertCharToSQLDate(srcDataPtr, srcLength, ODBCDataType, &SQLTime, targetPrecision) 
					!= SQL_SUCCESS)
				return IDS_22_008;
			break;
		case SQL_C_TIME:
		case SQL_C_TYPE_TIME:
		case SQL_C_DEFAULT:
			timeTmp = (TIME_STRUCT *)srcDataPtr;
		
			for (i = 0 ; i < 8 ; i++)
				datetime_parts[i] = 0;
			datetime_parts[0] = 1;
			datetime_parts[1] = 1;
			datetime_parts[2] = 1;
			datetime_parts[3] = timeTmp->hour;
			datetime_parts[4] = timeTmp->minute;
			datetime_parts[5] = timeTmp->second;
			if (!checkDatetimeValue(datetime_parts))
				return IDS_22_008;

			SQLTime.hour = timeTmp->hour;
			SQLTime.minute = timeTmp->minute;
			SQLTime.second = timeTmp->second;
			memset(&SQLTime.fraction,0,sizeof(UDWORD));
			break;
		case SQL_C_TIMESTAMP:
		case SQL_C_TYPE_TIMESTAMP:
			timestampTmp = (TIMESTAMP_STRUCT *)srcDataPtr;
// SQL/MX fraction precision is max 6 digits but ODBC accepts max precision 9 digits 
// conversion from nano to micro fraction of second
			ulFraction = (UDWORD)timestampTmp->fraction;
			if (targetPrecision > 0)
			{
				ulFraction /= 1000;
				sprintf(cTmpBuf, "%06lu", ulFraction);
				cTmpBuf[targetPrecision] = 0;
				strcpy(cTmpFraction,cTmpBuf);
			}
			
			for (i = 0 ; i < 8 ; i++)
				datetime_parts[i] = 0;
			datetime_parts[0] = timestampTmp->year;
			datetime_parts[1] = timestampTmp->month;
			datetime_parts[2] = timestampTmp->day;
			datetime_parts[3] = timestampTmp->hour;
			datetime_parts[4] = timestampTmp->minute;
			datetime_parts[5] = timestampTmp->second;
			datetime_parts[6] = (short)(ulFraction/1000);
			datetime_parts[7] = (short)(ulFraction%1000);
			if (!checkDatetimeValue(datetime_parts))
				return IDS_22_008;

			SQLTime.hour = timestampTmp->hour;
			SQLTime.minute = timestampTmp->minute;
			SQLTime.second = timestampTmp->second;
			memcpy(&SQLTime.fraction, &ulFraction, sizeof(UDWORD)); 
			if (targetPrecision == 0 && ulFraction != 0)
			{
				if (ODBCAppVersion >= SQL_OV_ODBC3)
						return IDS_22_008;
				else
					retCode = IDS_01_S07;
			}
			break;
		case SQL_C_BINARY:
			if (srcLength != targetLength)
				return IDS_22_008;
			DataPtr = srcDataPtr;
			break;
		default:
			return IDS_07_006;
			break;
		}
		if (DataPtr == NULL)
			DataPtr = &SQLTime;
		DataLen = targetLength;

		if (CDataType != SQL_C_BINARY)
		{
			pSQLTime = (TIME_TYPES*)DataPtr;
			switch  (SQLDatetimeCode)
			{
			case SQLDTCODE_HOUR:
				DataLen = sprintf(cTmpBuf,"%02d",pSQLTime->hour);
				break;
			case SQLDTCODE_HOUR_TO_MINUTE:
				DataLen = sprintf(cTmpBuf,"%02d:%02d",pSQLTime->hour,pSQLTime->minute);
				break;
			case SQLDTCODE_MINUTE:
				DataLen = sprintf(cTmpBuf,"%02d",pSQLTime->minute);
				break;
			case SQLDTCODE_MINUTE_TO_SECOND:
				if (targetPrecision > 0)
					DataLen = sprintf(cTmpBuf,"%02d:%02d.%s",pSQLTime->minute,pSQLTime->second,cTmpFraction);
				else
					DataLen = sprintf(cTmpBuf,"%02d:%02d",pSQLTime->minute,pSQLTime->second);
				break;
			case SQLDTCODE_SECOND:
				DataLen = sprintf(cTmpBuf,"%02d",pSQLTime->second);
				break;
			default:
				DataLen = sprintf(cTmpBuf,"%02d:%02d:%02d",pSQLTime->hour,pSQLTime->minute,pSQLTime->second);
				break;
			}
			if (DataLen != targetLength)
				return IDS_22_008;
			DataPtr = cTmpBuf;
		}
		break; // End of case for SQL_TIME
	case SQL_TIMESTAMP:
	case SQL_TYPE_TIMESTAMP:
		switch (CDataType)
		{
		case SQL_C_WCHAR:
			if (srcLength != SQL_NTS)
				srcLength = srcLength/2;
			// translate from UTF16
			if (WCharToUTF8((wchar_t*)srcDataPtr, srcLength, srcDataLocale, sizeof(srcDataLocale), (int*)&translateLength, (char*)errorMsg) != SQL_SUCCESS)
				return IDS_193_DRVTODS_ERROR;
			srcDataPtr = srcDataLocale;
			srcLength = translateLength;
		case SQL_C_CHAR:
			if (ConvertCharToSQLDate(srcDataPtr, srcLength, ODBCDataType, &SQLTimestamp, targetPrecision) 
					!= SQL_SUCCESS)
				return IDS_22_008;
			memcpy(&ulFraction, &SQLTimestamp.fraction, sizeof(UDWORD));
			if (targetPrecision > 0)
			{
				sprintf(cTmpFraction, "%0*lu", targetPrecision, ulFraction);
			}
			break;
		case SQL_C_DATE:
		case SQL_C_TYPE_DATE:
			dateTmp = (DATE_STRUCT *)srcDataPtr;
						
			for (i = 0 ; i < 8 ; i++)
				datetime_parts[i] = 0;
			datetime_parts[0] = dateTmp->year;
			datetime_parts[1] = dateTmp->month;
			datetime_parts[2] = dateTmp->day;
			if (!checkDatetimeValue(datetime_parts))
				return IDS_22_008;

			SQLTimestamp.year = dateTmp->year;
			SQLTimestamp.month = dateTmp->month;
			SQLTimestamp.day = dateTmp->day;
			SQLTimestamp.hour = 0;
			SQLTimestamp.minute = 0;
			SQLTimestamp.second = 0;
			memset(&SQLTimestamp.fraction, 0, sizeof(UDWORD));
			ulFraction = 0;
			if (targetPrecision > 0)
			{
				sprintf(cTmpBuf, "%06lu", ulFraction);
				cTmpBuf[targetPrecision] = 0;
				strcpy(cTmpFraction,cTmpBuf);
//				ulFraction = atol(cTmpBuf);
			}
			break;
		case SQL_C_TIME:
		case SQL_C_TYPE_TIME:
			struct tm *newtime;
			time_t long_time;

			time( &long_time );					/* Get time as long integer. */
			newtime = localtime( &long_time );	/* Convert to local time. */
			
			timeTmp = (TIME_STRUCT *)srcDataPtr;
			
			for (i = 0 ; i < 8 ; i++)
				datetime_parts[i] = 0;
			datetime_parts[0] = newtime->tm_year+1900;
			datetime_parts[1] = newtime->tm_mon+1;
			datetime_parts[2] = newtime->tm_mday;
			datetime_parts[3] = timeTmp->hour;
			datetime_parts[4] = timeTmp->minute;
			datetime_parts[5] = timeTmp->second;
			if (!checkDatetimeValue(datetime_parts))
				return IDS_22_008;

			SQLTimestamp.year = newtime->tm_year+1900;
			SQLTimestamp.month = newtime->tm_mon+1;
			SQLTimestamp.day = newtime->tm_mday;
			SQLTimestamp.hour = timeTmp->hour;
			SQLTimestamp.minute = timeTmp->minute;
			SQLTimestamp.second = timeTmp->second;
			memset(&SQLTimestamp.fraction, 0, sizeof(UDWORD));
			ulFraction = 0;
			if (targetPrecision > 0)
			{
				sprintf(cTmpBuf, "%06lu", ulFraction);
				cTmpBuf[targetPrecision] = 0;
				strcpy(cTmpFraction,cTmpBuf);
//				ulFraction = atol(cTmpBuf);
			}
			break;
		case SQL_C_TIMESTAMP:
		case SQL_C_TYPE_TIMESTAMP:
		case SQL_C_DEFAULT:
			timestampTmp = (TIMESTAMP_STRUCT *)srcDataPtr;
// SQL/MX fraction precision is max 6 digits but ODBC accepts max precision 9 digits 
// conversion from nano to fraction of second
			ulFraction = (UDWORD)timestampTmp->fraction;

			if (targetPrecision > 0)
				ulFraction = (ulFraction * pow(10,targetPrecision)) / 1000000000.0;
			else
				ulFraction = 0;
			sprintf(cTmpBuf, "%06u", ulFraction);
            strcpy(cTmpFraction,&cTmpBuf[6 - targetPrecision]);

			for (i = 0 ; i < 8 ; i++)
				datetime_parts[i] = 0;
			datetime_parts[0] = timestampTmp->year;
			datetime_parts[1] = timestampTmp->month;
			datetime_parts[2] = timestampTmp->day;
			datetime_parts[3] = timestampTmp->hour;
			datetime_parts[4] = timestampTmp->minute;
			datetime_parts[5] = timestampTmp->second;
			datetime_parts[6] = (short)(ulFraction/1000);
			datetime_parts[7] = (short)(ulFraction%1000);
			if (!checkDatetimeValue(datetime_parts))
				return IDS_22_008;

			SQLTimestamp.year = timestampTmp->year;
			SQLTimestamp.month = timestampTmp->month;
			SQLTimestamp.day = timestampTmp->day;
			SQLTimestamp.hour = timestampTmp->hour;
			SQLTimestamp.minute = timestampTmp->minute;
			SQLTimestamp.second = timestampTmp->second;
			memset(&SQLTimestamp.fraction, 0, sizeof(UDWORD));
			if (targetPrecision > 0) 
				memcpy(&SQLTimestamp.fraction, &ulFraction, sizeof(SQLUINTEGER));
			break;
		case SQL_C_BINARY:
			if (srcLength != targetLength)
				return IDS_22_003;
			DataPtr = srcDataPtr;
			break;
		default:
			return IDS_07_006;
		}
		if (DataPtr == NULL)
			DataPtr = &SQLTimestamp;
//		DataLen = sizeof(SQLTimestamp)-1; // -1 since it is only 11 bytes that matters
		OutLen = DataLen; // in case user creates a table as datetime year to second, 
				  // SQL/MX returns OutLen as 7 bytes
				  // Non-standard timestamp table year to second.
		if (CDataType != SQL_C_BINARY)
		{
			pSQLTimestamp = (TIMESTAMP_TYPES*)DataPtr;
			switch  (SQLDatetimeCode)
			{
			case SQLDTCODE_TIME:
				if (targetPrecision > 0)
					DataLen = sprintf(cTmpBuf,"%02d:%02d:%02d.%s",
								pSQLTimestamp->hour,pSQLTimestamp->minute,pSQLTimestamp->second,
								cTmpFraction);
				else
					DataLen = sprintf(cTmpBuf,"%02d:%02d:%02d",
								pSQLTimestamp->hour,pSQLTimestamp->minute,pSQLTimestamp->second);
				break;
			case SQLDTCODE_YEAR_TO_HOUR:
				DataLen = sprintf(cTmpBuf,"%04d-%02d-%02d %02d",
								pSQLTimestamp->year,pSQLTimestamp->month,pSQLTimestamp->day,
								pSQLTimestamp->hour);
				break;
			case SQLDTCODE_YEAR_TO_MINUTE:
				DataLen = sprintf(cTmpBuf,"%04d-%02d-%02d %02d:%02d",
								pSQLTimestamp->year,pSQLTimestamp->month,pSQLTimestamp->day,
								pSQLTimestamp->hour,pSQLTimestamp->minute);
				break;
			case SQLDTCODE_MONTH_TO_HOUR:
				DataLen = sprintf(cTmpBuf,"%02d-%02d %02d",
								pSQLTimestamp->month,pSQLTimestamp->day,
								pSQLTimestamp->hour);
				break;
			case SQLDTCODE_MONTH_TO_MINUTE:
				DataLen = sprintf(cTmpBuf,"%02d-%02d %02d:%02d",
								pSQLTimestamp->month,pSQLTimestamp->day,
								pSQLTimestamp->hour,pSQLTimestamp->minute);
				break;
			case SQLDTCODE_MONTH_TO_SECOND:
				if (targetPrecision > 0)
					DataLen = sprintf(cTmpBuf,"%02d-%02d %02d:%02d:%02d.%s",
								pSQLTimestamp->month,pSQLTimestamp->day,
								pSQLTimestamp->hour,pSQLTimestamp->minute,pSQLTimestamp->second,
								cTmpFraction);
				else
					DataLen = sprintf(cTmpBuf,"%02d-%02d %02d:%02d:%02d",
								pSQLTimestamp->month,pSQLTimestamp->day,
								pSQLTimestamp->hour,pSQLTimestamp->minute,pSQLTimestamp->second);
				break;
			case SQLDTCODE_DAY_TO_HOUR:
				DataLen = sprintf(cTmpBuf,"%02d %02d",
								pSQLTimestamp->day,
								pSQLTimestamp->hour);
				break;
			case SQLDTCODE_DAY_TO_MINUTE:
				DataLen = sprintf(cTmpBuf,"%02d %02d:%02d",
								pSQLTimestamp->day,
								pSQLTimestamp->hour,pSQLTimestamp->minute);
				break;
			case SQLDTCODE_DAY_TO_SECOND:
				if (targetPrecision > 0)
					DataLen = sprintf(cTmpBuf,"%02d %02d:%02d:%02d.%s",
								pSQLTimestamp->day,
								pSQLTimestamp->hour,pSQLTimestamp->minute,pSQLTimestamp->second,
								cTmpFraction);
				else
					DataLen = sprintf(cTmpBuf,"%02d %02d:%02d:%02d",
								pSQLTimestamp->day,
								pSQLTimestamp->hour,pSQLTimestamp->minute,pSQLTimestamp->second);
				break;
			case SQLDTCODE_HOUR:
				DataLen = sprintf(cTmpBuf,"%02d",pSQLTimestamp->hour);
				break;
			case SQLDTCODE_HOUR_TO_MINUTE:
				DataLen = sprintf(cTmpBuf,"%02d:%02d",pSQLTimestamp->hour,pSQLTimestamp->minute);
				break;
			case SQLDTCODE_MINUTE:
				DataLen = sprintf(cTmpBuf,"%02d",pSQLTimestamp->minute);
				break;
			case SQLDTCODE_MINUTE_TO_SECOND:
				if (targetPrecision > 0)
					DataLen = sprintf(cTmpBuf,"%02d:%02d.%s",pSQLTimestamp->minute,pSQLTimestamp->second,cTmpFraction);
				else
					DataLen = sprintf(cTmpBuf,"%02d:%02d",pSQLTimestamp->minute,pSQLTimestamp->second);
				break;
			case SQLDTCODE_SECOND:
				if (targetPrecision > 0)
					DataLen = sprintf(cTmpBuf,"%02d.%s",pSQLTimestamp->second,cTmpFraction);
				else
					DataLen = sprintf(cTmpBuf,"%02d",pSQLTimestamp->second);
				break;
			default:
				if (targetPrecision > 0)
					DataLen = sprintf(cTmpBuf,"%04d-%02d-%02d %02d:%02d:%02d.%s",
								pSQLTimestamp->year,pSQLTimestamp->month,pSQLTimestamp->day,
								pSQLTimestamp->hour,pSQLTimestamp->minute,pSQLTimestamp->second,
								cTmpFraction);
				else
					DataLen = sprintf(cTmpBuf,"%04d-%02d-%02d %02d:%02d:%02d",
								pSQLTimestamp->year,pSQLTimestamp->month,pSQLTimestamp->day,
								pSQLTimestamp->hour,pSQLTimestamp->minute,pSQLTimestamp->second);
				break;

			}
			DataPtr = cTmpBuf;
			if (DataLen != targetLength)
				return IDS_22_003;
		}
		break; // End of SQL_TIMESTAMP
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
		cTmpDataType = CDataType;
		if (CDataType == SQL_C_DEFAULT)
		{
			switch (ODBCDataType)
			{
			case SQL_INTERVAL_MONTH:
				cTmpDataType = SQL_C_INTERVAL_MONTH;
				break;
			case SQL_INTERVAL_YEAR:
				cTmpDataType = SQL_C_INTERVAL_YEAR;
				break;
			case SQL_INTERVAL_YEAR_TO_MONTH:
				cTmpDataType = SQL_C_INTERVAL_YEAR_TO_MONTH;
				break;
			case SQL_INTERVAL_DAY:
				cTmpDataType = SQL_C_INTERVAL_DAY;
				break;
			case SQL_INTERVAL_HOUR:
				cTmpDataType = SQL_C_INTERVAL_HOUR;
				break;
			case SQL_INTERVAL_MINUTE:
				cTmpDataType = SQL_C_INTERVAL_MINUTE;
				break;
			case SQL_INTERVAL_SECOND:
				cTmpDataType = SQL_C_INTERVAL_SECOND;
				break;
			case SQL_INTERVAL_DAY_TO_HOUR:
				cTmpDataType = SQL_C_INTERVAL_DAY_TO_HOUR;
				break;
			case SQL_INTERVAL_DAY_TO_MINUTE:
				cTmpDataType = SQL_C_INTERVAL_DAY_TO_MINUTE;
				break;
			case SQL_INTERVAL_DAY_TO_SECOND:
				cTmpDataType = SQL_C_INTERVAL_DAY_TO_SECOND;
				break;
			case SQL_INTERVAL_HOUR_TO_MINUTE:
				cTmpDataType = SQL_C_INTERVAL_HOUR_TO_MINUTE;
				break;
			case SQL_INTERVAL_HOUR_TO_SECOND:
				cTmpDataType = SQL_C_INTERVAL_HOUR_TO_SECOND;
				break;
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				cTmpDataType = SQL_C_INTERVAL_MINUTE_TO_SECOND;
				break;
			default:
				return IDS_07_006;
			}
		}

		switch (cTmpDataType)
		{
		case SQL_C_WCHAR:
			if (srcLength != SQL_NTS)
				srcLength = srcLength/2;
			// translate from UTF16
			if (WCharToUTF8((wchar_t*)srcDataPtr, srcLength, srcDataLocale, sizeof(srcDataLocale), (int*)&translateLength, (char*)errorMsg) != SQL_SUCCESS)
				return IDS_193_DRVTODS_ERROR;
			srcDataPtr = srcDataLocale;
			srcLength = translateLength;
		case SQL_C_CHAR:
			if (srcLength == SQL_NTS)
				DataLen = strlen((const char *)srcDataPtr);
			else
				DataLen = srcLength;
			if (StripIntervalLiterals(srcDataPtr, srcLength, ODBCDataType, cTmpBuf) != SQL_SUCCESS)
				return IDS_22_018;
			break;
		case SQL_C_BINARY:
			if (srcLength == SQL_NTS)
				DataLen = strlen((const char *)srcDataPtr);
			else
				DataLen = srcLength;
			strcpy(cTmpBuf, (const char *)srcDataPtr);
			break;
		case SQL_C_SHORT:
		case SQL_C_SSHORT:
			sTmp = *(SSHORT *)srcDataPtr;
			_ltoa(sTmp, cTmpBuf, 10);
			break;
		case SQL_C_USHORT:
			usTmp = *(USHORT *)srcDataPtr;
			_ultoa(usTmp, cTmpBuf, 10);
			break;
		case SQL_C_TINYINT:
		case SQL_C_STINYINT:
			sTmp = *(SCHAR *)srcDataPtr;
			_ltoa(sTmp, cTmpBuf, 10);
			break;
		case SQL_C_UTINYINT:
		case SQL_C_BIT:
			usTmp = *(UCHAR *)srcDataPtr;
			_ultoa(usTmp, cTmpBuf, 10);
			break;
		case SQL_C_SLONG:
		case SQL_C_LONG:
			lTmp = *(SLONG *)srcDataPtr;
			_ltoa(lTmp, cTmpBuf, 10);
			break;
		case SQL_C_ULONG:
			ulTmp = *(ULONG *)srcDataPtr;
			_ultoa(ulTmp, cTmpBuf, 10);
			break;
		case SQL_C_FLOAT:
			dTmp = *(float *)srcDataPtr;
			if (!double_to_char (dTmp, FLT_DIG, cTmpBuf, sizeof(cTmpBuf)))
				return IDS_22_001;
			break;
		case SQL_C_DOUBLE:
			dTmp = *(double *)srcDataPtr;
			if (!double_to_char (dTmp, DBL_DIG, cTmpBuf, sizeof(cTmpBuf)))
				return IDS_22_001;
			break;
		case SQL_C_NUMERIC:
			ConvertCNumericToChar((SQL_NUMERIC_STRUCT*)srcDataPtr, cTmpBuf);
			break;
		case SQL_C_SBIGINT:
			sprintf(cTmpBuf, "%I64d", *(__int64*)srcDataPtr);
			break;
		case SQL_C_INTERVAL_MONTH:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			switch (ODBCDataType)
			{
			case SQL_INTERVAL_MONTH:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.year_month.month);
				else
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.year_month.month);
				break;
			case SQL_INTERVAL_YEAR_TO_MONTH:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-00-%ld",intervalTmp->intval.year_month.month);
				else
					sprintf(cTmpBuf,"00-%ld",intervalTmp->intval.year_month.month);
				break;
			default:
				return IDS_07_006;
			}
			break;
		case SQL_C_INTERVAL_YEAR:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			switch (ODBCDataType)
			{
			case SQL_INTERVAL_YEAR:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.year_month.year);
				else
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.year_month.year);
				break;
			case SQL_INTERVAL_YEAR_TO_MONTH:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld-00",intervalTmp->intval.year_month.year);
				else
					sprintf(cTmpBuf,"%ld-00",intervalTmp->intval.year_month.year);
				break;
			default:
				return IDS_07_006;
			}
			break;
		case SQL_C_INTERVAL_YEAR_TO_MONTH:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			switch (ODBCDataType)
			{
			case SQL_INTERVAL_YEAR:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.year_month.year);
				else
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.year_month.year);
				break;
			case SQL_INTERVAL_MONTH:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld", intervalTmp->intval.year_month.month);
				else
					sprintf(cTmpBuf,"%ld", intervalTmp->intval.year_month.month);
				break;
			case SQL_INTERVAL_YEAR_TO_MONTH:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld-%ld",intervalTmp->intval.year_month.year, intervalTmp->intval.year_month.month);
				else
					sprintf(cTmpBuf,"%ld-%ld",intervalTmp->intval.year_month.year, intervalTmp->intval.year_month.month);
				break;
			default:
				return IDS_07_006;
			}
			break;
		case SQL_C_INTERVAL_DAY:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			switch (ODBCDataType)
			{
			case SQL_INTERVAL_DAY:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.day);
				else
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.day);
				break;
			case SQL_INTERVAL_DAY_TO_HOUR:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld 00",intervalTmp->intval.day_second.day);
				else
					sprintf(cTmpBuf,"%ld 00",intervalTmp->intval.day_second.day);
				break;
			case SQL_INTERVAL_DAY_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld 00:00",intervalTmp->intval.day_second.day);
				else
					sprintf(cTmpBuf,"%ld 00:00",intervalTmp->intval.day_second.day);
				break;
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld 00:00:00",intervalTmp->intval.day_second.day);
				else
					sprintf(cTmpBuf,"%ld 00:00:00",intervalTmp->intval.day_second.day);
				break;
			default:
				return IDS_07_006;
			}
			break;
		case SQL_C_INTERVAL_HOUR:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			switch (ODBCDataType)
			{
			case SQL_INTERVAL_HOUR:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.hour);
				break;
			case SQL_INTERVAL_DAY_TO_HOUR:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-00 %ld",intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"00 %ld",intervalTmp->intval.day_second.hour);
				break;
			case SQL_INTERVAL_DAY_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-00 %ld:00",intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"00 %ld:00",intervalTmp->intval.day_second.hour);
				break;
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-00 %ld:00:00",intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"00 %ld:00:00",intervalTmp->intval.day_second.hour);
				break;
			case SQL_INTERVAL_HOUR_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld:00",intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"%ld:00",intervalTmp->intval.day_second.hour);
				break;
			case SQL_INTERVAL_HOUR_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld:00:00",intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"%ld:00:00",intervalTmp->intval.day_second.hour);
				break;
			default:
				return IDS_07_006;
			}
			break;
		case SQL_C_INTERVAL_MINUTE:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			switch (ODBCDataType)
			{
			case SQL_INTERVAL_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_DAY_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-00 00:%ld",intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"00 00:%ld",intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-00 00:%ld:00",intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"00 00:%ld:00",intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_HOUR_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-00:%ld",intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"00:%ld",intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_HOUR_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-00:%ld:00",intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"00:%ld:00",intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld:00",intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld:00",intervalTmp->intval.day_second.minute);
				break;
			default:
				return IDS_07_006;
			}
			break;
		case SQL_C_INTERVAL_SECOND:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			switch (ODBCDataType)
			{
			case SQL_INTERVAL_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"-%ld.%ld",intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				else
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"%ld.%ld",intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				break;
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"-00 00:00:%ld",intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"-00 00:00:%ld.%ld",intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				else
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"00 00:00:%ld",intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"00 00:00:%ld.%ld",intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				break;
			case SQL_INTERVAL_HOUR_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"-00:00:%ld",intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"-00:00:%ld.%ld",intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				else
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"00:00:%ld",intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"00:00:%ld.%ld",intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				break;
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"-00:%ld",intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"-00:%ld.%ld",intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				else
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"00:%ld",intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"00:%ld.%ld",intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				break;
			default:
				return IDS_07_006;
			}
			break;
		case SQL_C_INTERVAL_DAY_TO_HOUR:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			switch (ODBCDataType)
			{
			case SQL_INTERVAL_DAY:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.day);
				else
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.day);
				break;
			case SQL_INTERVAL_HOUR:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld", intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"%ld", intervalTmp->intval.day_second.hour);
				break;
			case SQL_INTERVAL_DAY_TO_HOUR:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld %ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"%ld %ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour);
				break;
			case SQL_INTERVAL_DAY_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld %ld:00",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"%ld %ld:00",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour);
				break;
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld %ld:00:00",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"%ld %ld:00:00",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour);
				break;
			case SQL_INTERVAL_HOUR_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld:00", intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"%ld:00", intervalTmp->intval.day_second.hour);
				break;
			case SQL_INTERVAL_HOUR_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld:00:00", intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"%ld:00:00", intervalTmp->intval.day_second.hour);
				break;
			default:
				return IDS_07_006;
			}
			break;
		case SQL_C_INTERVAL_DAY_TO_MINUTE:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			switch (ODBCDataType)
			{
			case SQL_INTERVAL_DAY:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.day);
				else
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.day);
				break;
			case SQL_INTERVAL_HOUR:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"%ld %ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_DAY_TO_HOUR:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld %ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"%ld %ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour);
				break;
			case SQL_INTERVAL_DAY_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld %ld:%ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld %ld:%ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld %ld:%ld:00",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld %ld:%ld:00",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_HOUR_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_HOUR_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld:%ld:00",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld:%ld:00",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld:00",intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld:00",intervalTmp->intval.day_second.minute);
				break;
			default:
				return IDS_07_006;
			}
			break;
		case SQL_C_INTERVAL_DAY_TO_SECOND:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			switch (ODBCDataType)
			{
			case SQL_INTERVAL_DAY:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.day);
				else
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.day);
				break;
			case SQL_INTERVAL_HOUR:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"%ld %ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"-%ld.%ld",intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				else
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"%ld.%ld",intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				break;
			case SQL_INTERVAL_DAY_TO_HOUR:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld %ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"%ld %ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour);
				break;
			case SQL_INTERVAL_DAY_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld %ld:%ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld %ld:%ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"-%ld %ld:%ld:%ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"-%ld %ld:%ld:%ld.%ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				else
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"%ld %ld:%ld:%ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"%ld %ld:%ld:%ld.%ld",intervalTmp->intval.day_second.day,intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				break;
			case SQL_INTERVAL_HOUR_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_HOUR_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"-%ld:%ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"-%ld:%ld:%ld.%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				else
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"%ld:%ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"%ld:%ld:%ld.%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				break;
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"-%ld:%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"-%ld:%ld.%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				else
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"%ld:%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"%ld:%ld.%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				break;
			default:
				return IDS_07_006;
			}
			break;
		case SQL_C_INTERVAL_HOUR_TO_MINUTE:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			switch (ODBCDataType)
			{
			case SQL_INTERVAL_HOUR:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.hour);
				break;
			case SQL_INTERVAL_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_DAY_TO_HOUR:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-00 %ld",intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"00 %ld",intervalTmp->intval.day_second.hour);
				break;
			case SQL_INTERVAL_DAY_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-00 %ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"00 %ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-00 %ld:%ld:00",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"00 %ld:%ld:00",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_HOUR_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_HOUR_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld:%ld:00",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld:%ld:00",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld:00",intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld:00",intervalTmp->intval.day_second.minute);
				break;
			default:
				return IDS_07_006;
			}
			break;
		case SQL_C_INTERVAL_HOUR_TO_SECOND:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			switch (ODBCDataType)
			{
			case SQL_INTERVAL_HOUR:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.hour);
				break;
			case SQL_INTERVAL_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"-%ld.%ld",intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				else
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"%ld.%ld",intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				break;
			case SQL_INTERVAL_DAY_TO_HOUR:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-00 %ld",intervalTmp->intval.day_second.hour);
				else
					sprintf(cTmpBuf,"00 %ld",intervalTmp->intval.day_second.hour);
				break;
			case SQL_INTERVAL_DAY_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-00 %ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"00 %ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-00 %ld:%ld:00",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"00 %ld:%ld:00",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_HOUR_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_HOUR_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"-%ld:%ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"-%ld:%ld:%ld.%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				else
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"%ld:%ld:%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"%ld:%ld:%ld.%ld",intervalTmp->intval.day_second.hour,intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				break;
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"-%ld:%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"-%ld:%ld.%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				else
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"%ld:%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"%ld:%ld.%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				break;
			default:
				return IDS_07_006;
			}
			break;
		case SQL_C_INTERVAL_MINUTE_TO_SECOND:
			intervalTmp = (SQL_INTERVAL_STRUCT *)srcDataPtr;
			switch (ODBCDataType)
			{
			case SQL_INTERVAL_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"-%ld",intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"-%ld.%ld",intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				else
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"%ld",intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"%ld.%ld",intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				break;
			case SQL_INTERVAL_DAY_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-00 00:%ld",intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"00 00:%ld",intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"-00 00:%ld:%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"-00 00:%ld:%ld.%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				else
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"00 00:%ld:%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"00 00:%ld:%ld.%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				break;
			case SQL_INTERVAL_HOUR_TO_MINUTE:
				if (intervalTmp->interval_sign == SQL_TRUE)
					sprintf(cTmpBuf,"-00:%ld",intervalTmp->intval.day_second.minute);
				else
					sprintf(cTmpBuf,"00:%ld",intervalTmp->intval.day_second.minute);
				break;
			case SQL_INTERVAL_HOUR_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"-00:%ld:%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"-00:%ld:%ld.%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				else
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"00:%ld:%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"00:%ld:%ld.%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				break;
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				if (intervalTmp->interval_sign == SQL_TRUE)
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"-%ld:%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"-%ld:%ld.%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				else
				{
					if (intervalTmp->intval.day_second.fraction == 0)
						sprintf(cTmpBuf,"%ld:%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second);
					else
						sprintf(cTmpBuf,"%ld:%ld.%ld",intervalTmp->intval.day_second.minute,intervalTmp->intval.day_second.second,intervalTmp->intval.day_second.fraction);
				}
				break;
			default:
				return IDS_07_006;
			}
			break;
		default:
			return IDS_07_006;
		}
		if (DataPtr == NULL)
		{
			if ((retCode = CheckIntervalOverflow(cTmpBuf, ODBCDataType, targetLength, targetPrecision)) != SQL_SUCCESS)
				return retCode;

			InsertBlank(cTmpBuf, targetLength-strlen(cTmpBuf));
			DataPtr = cTmpBuf;
			DataLen = strlen(cTmpBuf);
		}
		if (Offset != 0)
		{
			if(DataLen>32767){
				*(int *)targetDataPtr = DataLen;
				outDataPtr = (unsigned char *)targetDataPtr + sizeof(int);
			}
			else{
				*(unsigned short *)targetDataPtr = DataLen;
				outDataPtr = (unsigned char *)targetDataPtr + sizeof(USHORT);
			}
		}
		OutLen = DataLen;	// in case user creates a table as datetime year to second, 
							// SQL/MX returns OutLen as 7 bytes
							// Non-standard timestamp table year to second.
		break;
	default:
		return IDS_07_006;
	}
	if ((retCode == SQL_SUCCESS) || (retCode == IDS_01_S07 ))
	{
		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_API, "ODBC::ConvertCToSQL DataPtr \"%s\", DataLen %d", 
					 DataPtr, DataLen);

		if (DataPtr != NULL && DataLen > 0 && 
			(ODBCDataType == SQL_CHAR || ODBCDataType == SQL_VARCHAR || ODBCDataType == SQL_LONGVARCHAR || 
			ODBCDataType == SQL_WCHAR || ODBCDataType == SQL_WVARCHAR) && CDataType != SQL_C_BINARY && SQLDataType != SQLTYPECODE_BOOLEAN)
		{
			SQLRETURN rc = SQL_SUCCESS;
			if (targetCharSet == SQLCHARSETCODE_UCS2)
			{
				// translate to UCS2
				if (CDataType == SQL_C_WCHAR)
				{
					// translate from UCS2 to UCS2 - no translation
					if (DataLen > OutLen/2)
					{
						DataLen = OutLen/2;
						return IDS_22_001;
					}						
					wcsncpy((wchar_t*)outDataPtr, (const wchar_t*)DataPtr, DataLen);
					((wchar_t*)outDataPtr)[DataLen] = L'\0';
				}
				else 
				{
					// translate from DrvrLocale to UCS2
					rc = LocaleToWChar((char*)DataPtr, DataLen, (wchar_t*)outDataPtr, OutLen/2+1,  (int*)&translateLength, (char*)errorMsg);
					if (rc != SQL_SUCCESS)
					{
						if (rc == SQL_SUCCESS_WITH_INFO)
							return IDS_22_001;
						else 
							return IDS_193_DRVTODS_ERROR;
					}
					DataLen = translateLength;
				}
				if (Offset != 0)
				{
					if (targetPrecision > SHRT_MAX)
						*(unsigned int *)targetDataPtr = DataLen * 2;
					else
						*(unsigned short *)targetDataPtr = DataLen * 2;
				}
			}
			else if (translateOption == 0) // source charset and dest charset are the same - no translation
			{
				// source and target charset are the same - no translation needed
				if (DataLen > OutLen)
				{
					DataLen = OutLen;
					return IDS_22_001;
				}
				memcpy(outDataPtr, DataPtr, DataLen);
				if (Offset != 0)
				{
					if (targetPrecision > SHRT_MAX)
						*(unsigned int *)targetDataPtr = DataLen;
					else
						*(unsigned short *)targetDataPtr = DataLen;
				}
			}
			else 
			{
				// translate from UTF16/DriverLocale to SQLCharSet - JC function
				if (! (gDrvrGlobal.fpSQLDriverToDataSource) (translateOption,
							ODBCDataType,
							DataPtr,
							(CDataType == SQL_C_WCHAR) ? DataLen*2 : DataLen,
							outDataPtr,
							OutLen+1,
							&translateLength,
							errorMsg,
							errorMsgMax,
							NULL))
				{
					if (translateLength > OutLen)
					{
						translateLength = OutLen;
						((char*)outDataPtr)[translateLength] = '\0';
						if (errorMsg) *errorMsg = '\0';
						return IDS_22_001;
					}
					else 
						return IDS_193_DRVTODS_ERROR;
				}
				DataLen = translateLength;
				if (Offset != 0)
				{
					if (targetPrecision > SHRT_MAX)
						*(unsigned int *)targetDataPtr = DataLen;
					else
						*(unsigned short *)targetDataPtr = DataLen;
				}

			}

			if (ODBCDataType == SQL_CHAR && SQLDataType != SQLTYPECODE_BOOLEAN)
				memset((unsigned char *)outDataPtr+DataLen, ' ', targetLength-DataLen-Offset-1);
			else if (ODBCDataType == SQL_WCHAR)
			{
				tempLen = (targetLength-DataLen*2-Offset)/2-1;
				for (i=0; i<tempLen; i++)
					memcpy((unsigned char *)outDataPtr+DataLen*2+i*2, L" ",2);
			}
		}
		else
		{
			if (OutLen < DataLen)
				return IDS_22_001;
			memcpy(outDataPtr, DataPtr, DataLen);
			if (Offset != 0)	//When Datalen = 0, length was not stored in buffer
			{
				if (targetPrecision > SHRT_MAX)
					*(unsigned int *)targetDataPtr = DataLen;
				else
					*(unsigned short *)targetDataPtr = DataLen;
			}
		}
		if (byteSwap) 
		{
			if (Datatype_Dependent_Swap((BYTE *)outDataPtr-Offset, SQLDataType, targetCharSet, targetLength, IEEE_TO_TANDEM, SQL_C_BINARY) != STATUS_OK)
				return IDS_HY_000;
		}
//#endif
	}
	if (dataTruncatedWarning)
		retCode = IDS_01_S07;

	return retCode;
}

unsigned long ODBC::ConvertCharToNumeric(SQLPOINTER srcDataPtr, 
								   SQLINTEGER srcLength, 
								   double &dTmp)
{
	int		tempLen;
	char	cTmpBuf[100];
	char    *errorCharPtr;

	if (srcLength == SQL_NTS )
		tempLen = strlen((const char *)srcDataPtr);
	else
		tempLen = srcLength;
	if (tempLen >= sizeof(cTmpBuf))
		return IDS_22_003;
	strncpy(cTmpBuf, (const char *)srcDataPtr, tempLen);
	cTmpBuf[tempLen] = '\0';
	errno = 0;
	dTmp = strtod(cTmpBuf, &errorCharPtr);
	if (errno == ERANGE || errorCharPtr < (cTmpBuf + tempLen))
		return IDS_22_003;
	return SQL_SUCCESS;
}

unsigned long ODBC::ConvertCharToInt64(SQLPOINTER srcDataPtr, 
								   SQLINTEGER srcLength, 
								   __int64 &tempVal64)
{
	int		tempLen;
	char	cTmpBuf[100];
	bool truncation = false;

	if (srcLength == SQL_NTS )
		tempLen = strlen((const char *)srcDataPtr);
	else
		tempLen = srcLength;
	if (tempLen >= sizeof(cTmpBuf))
		return IDS_22_003;
	strncpy(cTmpBuf, (const char *)srcDataPtr, tempLen);
	cTmpBuf[tempLen] = '\0';
	if (!ctoi64(cTmpBuf, tempVal64, &truncation))
		return IDS_22_003;
	if (truncation)
		return IDS_22_001;

	return SQL_SUCCESS;
}


unsigned long ODBC::ConvertCharWithDecimalToInt64(SQLPOINTER srcDataPtr, 
								   SQLINTEGER srcLength, 
								   __int64 &integralPart,
								   __int64 &decimalPart)
{
	int		tempLen;
	int		integralLen;
	int		decimalLen;
	char	cTmpBuf[100];
	char	*integral;
	char	*decimal;
	bool truncation = false;

	if (srcLength == SQL_NTS )
		tempLen = strlen((const char *)srcDataPtr);
	else
		tempLen = srcLength;
	if (tempLen >= sizeof(cTmpBuf))
		return IDS_22_003;
	strncpy(cTmpBuf, (const char *)srcDataPtr, tempLen);
	cTmpBuf[tempLen] = '\0';
	integral = cTmpBuf;
	if ((decimal = strchr(integral, '.')) != NULL)
	{
		integralLen = decimal - integral;
		*decimal++ = '\0';
		decimalLen = tempLen - integralLen -1;
	}
	if (!ctoi64(integral, integralPart, &truncation))
		return IDS_22_003;
	if (decimal != NULL)
	{
		if (!ctoi64(decimal, decimalPart, &truncation))
			return IDS_22_003;
	}
	else
		decimalPart = 0;
	if (truncation)
		    return IDS_22_001;
	return SQL_SUCCESS;
}

SQLRETURN ODBC::ConvertCharToSQLDate(SQLPOINTER srcDataPtr, SQLINTEGER srcLength, SWORD ODBCDataType,
							   SQLPOINTER targetPtr, SQLSMALLINT targetPrecision)
{						  
    char      in_value[50];
    short datetime_parts[8];
    char     *token;
    short     i;
    unsigned long fraction_part=0;
    char      delimiters[3];
	int		tempLen;
	char	sFraction[10];
	char* pDate = (char*)srcDataPtr;
	bool	bWMFormat = false;

	if (targetPrecision > 9)
		return SQL_ERROR;

	if (srcLength == SQL_NTS )
		tempLen = strlen((const char *)srcDataPtr);
	else
		tempLen = srcLength;
	if (tempLen > sizeof(in_value)-1)
		return SQL_ERROR;

	if (gDrvrGlobal.gSpecial_1 && tempLen == (short)strspn(pDate, "1234567890"))
	{
		if (tempLen < 8 && tempLen > 5)
		{
			char sTmp[7];
			int century;
			int year;
			int month;
			int day;

			memset(sTmp, '0', sizeof(sTmp));
			strncpy(sTmp+ sizeof(sTmp) - tempLen, pDate, tempLen);

			century = (19 + sTmp[0] - '0');
			year = century * 100 + (sTmp[1] - '0') * 10 + (sTmp[2] - '0');
			month = (sTmp[3] - '0') * 10 + (sTmp[4] - '0');
			day = (sTmp[5] - '0') * 10 + (sTmp[6] - '0');

			sprintf(in_value,"%d/%02d/%02d",year,month,day);
			tempLen = strlen(in_value);
			bWMFormat = true;
		}
	}

	if (bWMFormat == false)
	{
	strncpy(in_value, (const char *)srcDataPtr, tempLen);
	in_value[tempLen] = '\0';
	}

// check if date, time, timestamp is in escape sequence format
// if yes, check format and convert {,},d,t,ts,' to blanks

    if (tempLen != (short)strspn(in_value, "1234567890:/.- "))
	{
		if (tempLen != (short)strspn(in_value, "1234567890:/.-{}dts\' "))
			return SQL_ERROR;

		int nIndex = 0;
		int nIteration = 0;
		char cLastDel = 0;
		char cChar;

		while( TRUE )
		{
			while( in_value[ nIndex ] == ' ' ) nIndex++;
			cChar = in_value[ nIndex ];

			if( cChar == 0 )
			{
				if( cLastDel != '}' || nIteration != 5 ) return(SQL_ERROR);
				else break;
			}

			if( cChar == '{' || cChar == 'd' || cChar == 't' || 
				cChar == '\'' || cChar == '}')
			{

				switch ( nIteration++ )
				{
				case 0:
					if( cChar != '{' ) return(SQL_ERROR);
					break;
				case 1:
					if( cChar != 'd' && cChar != 't' ) return(SQL_ERROR);
					if( cChar == 't' && in_value[ nIndex + 1] == 's' )
						in_value[ nIndex + 1 ] = ' ';
					break;
				case 2:
					if( cChar != '\'' ) return(SQL_ERROR);
					break;
				case 3:
					if( cChar != '\'' ) return(SQL_ERROR);
					break;
				case 4:
					if( cChar != '}' ) return(SQL_ERROR);
					break;
				default:
					return(SQL_ERROR);
					break;
				}

				cLastDel = cChar;
				in_value[ nIndex ] = ' ';
			}
			nIndex++;
		}
	}

	for (i = 0 ; i < 8 ; i++)
       datetime_parts[i] = 0;
	if (strpbrk(in_value, "/-") == NULL)
	{
	    if ((ODBCDataType == SQL_DATE) || (ODBCDataType == SQL_TIMESTAMP)) 
		  return SQL_ERROR; //not a valid date literal, return error : JoyJ
		i = 3;
		strcpy(delimiters, ":");

		struct tm *newtime;
		time_t long_time;

		time( &long_time );					/* Get time as long integer. */
		newtime = localtime( &long_time );	/* Convert to local time. */
		datetime_parts[0] = (short)(newtime->tm_year+1900);
		datetime_parts[1] = (short)(newtime->tm_mon+1);
		datetime_parts[2] = (short)newtime->tm_mday;
	}
	else
	{
		i = 0;
		strcpy(delimiters, "/-");
	}
    for (token = strtok(in_value, delimiters) ; token != NULL && i < 6 ;
            token = strtok(NULL, delimiters), i++)
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
		rTrim(token);
		memset(sFraction, 0, sizeof(sFraction));
		memset(sFraction, '0', 9);
		memcpy(sFraction, token, strlen(token));
		sFraction[targetPrecision] = 0;
		fraction_part = atol(sFraction);

		datetime_parts[6] = (short)(fraction_part / 1000);
		datetime_parts[7] = (short)(fraction_part % 1000);
    }
	if (! checkDatetimeValue(datetime_parts))
		return SQL_ERROR;
	switch (ODBCDataType) {
	case SQL_DATE:
		{
			DATE_TYPES *dateTmp = (DATE_TYPES *)targetPtr;
			dateTmp->year = datetime_parts[0];
			dateTmp->month = (unsigned char)datetime_parts[1];
			dateTmp->day = (unsigned char)datetime_parts[2];
		}
		break;
	case SQL_TIME:
		{
			TIME_TYPES *timeTmp = (TIME_TYPES *)targetPtr;
			timeTmp->hour=(unsigned char)datetime_parts[3];
			timeTmp->minute=(unsigned char)datetime_parts[4];
			timeTmp->second=(unsigned char)datetime_parts[5];
		}
		break;
	case SQL_TIMESTAMP:
		{ 
			TIMESTAMP_TYPES *timeStampTmp = (TIMESTAMP_TYPES *)targetPtr;

			timeStampTmp->year=datetime_parts[0];
			timeStampTmp->month=(unsigned char)datetime_parts[1];
			timeStampTmp->day=(unsigned char)datetime_parts[2];
			timeStampTmp->hour=(unsigned char)datetime_parts[3];
			timeStampTmp->minute=(unsigned char)datetime_parts[4];
			timeStampTmp->second=(unsigned char)datetime_parts[5];
			if (targetPrecision > 0) 
				memcpy(&timeStampTmp->fraction, &fraction_part, sizeof(UDWORD));
			else
				memset(&timeStampTmp->fraction, 0, sizeof(UDWORD));

		}
		break;
	default:
		return SQL_ERROR;
    }
    return SQL_SUCCESS;
}

//======================================================================
void ODBC::getMaxNum(	long precision,
				long scale,
				unsigned __int64 &integralMax,
				unsigned __int64 &decimalMax)
{
	int i;		
	decimalMax = 0;
	integralMax = 0;

	for(i=0; i < precision - scale; i++)
		integralMax= integralMax * 10 + 9;

	for(i=0; i < scale; i++)
		decimalMax = decimalMax * 10 + 9;

}

// 
// This routine will attempt to convert a value into integralPart, decimalPart, negative, leadZeros
//
// for example 
//   -2.145  will have integralPart = 2, decimalPart = 145, negative = TRUE; leadZeros = 0 (no leading zero)
//   6.09811 will have integralPart = 6, decimalPart = 9811(not 09811), negative = FALSE, leadZeros = 1 (1 leading zero)
short ODBC::ConvertCharToInt64Num(const char *srcDataPtr, 
				   __int64 &integralPart,
				   __int64 &decimalPart,
				   BOOL	&negative,
				   long &leadZeros)
{
	// Return values -1 - Out of Range
	//		 -2 - Illegal numeric value
	//		  0 - success
				
	char		*tempPtr,*tempPtr1,*tempPtr2;
	int             tempLen = 0;
	double		dTmp;
	char		*errorCharPtr;
	BOOL		decimalFound = false;
	BOOL		countZeros = true;

	leadZeros = 0;
	decimalPart = 0;
	integralPart = 0;
	negative = FALSE;

	if (srcDataPtr == NULL)
		return -2;
	tempPtr = (char *)srcDataPtr;
	trim(tempPtr);
	if( strspn( tempPtr, "+-0123456789.eE" ) == strlen( tempPtr) )
	{
		if( strspn( tempPtr, "+-0123456789." ) != strlen( tempPtr) )
		{
			char sTmp[20];
			errno = 0;
			dTmp = strtod(srcDataPtr, &errorCharPtr);
			if (errno == ERANGE || errorCharPtr < (srcDataPtr + strlen(srcDataPtr)))
				return -1;
			sprintf(sTmp,"%lf",dTmp);
			strcpy(tempPtr,sTmp);
			rSup(tempPtr);
		}
	}
	else
		return -2;

	switch (*tempPtr)
	{
	case '+':
		tempPtr++;
		break;
	case '-':
		negative = TRUE;
		tempPtr++;
		break;
	default:
		break;
	}

	tempPtr1 = strchr( tempPtr, '.' );
	if( tempPtr1 == NULL )
	{
		if( strlen(tempPtr) > 33 )
			return -1;
	}
	else
	{
		if( tempPtr1 - tempPtr > 33 )
			return -1;
	}
	
	while (*tempPtr != '\0')
	{
		if (*tempPtr == '.')
		{
			decimalFound = TRUE;
			break;
		}
		if (*tempPtr >= '0' && *tempPtr <= '9') 
		{
			integralPart= integralPart * 10 + (*tempPtr - '0');
			++tempPtr;
		}
		else
			return -2; 
	}

	if (decimalFound)
	{
	  	//  to get rid of trailing zeros
		tempLen  = strlen(tempPtr);
		if ((tempPtr2 = (char*)malloc(tempLen + 1)) == NULL)
			return -1;
		strcpy(tempPtr2, tempPtr);
		tempPtr2[tempLen] = '\0';

		for (int i= tempLen - 1; i >=0; i--)
		{
			if (tempPtr2[i] == '0')
				tempPtr2[i] = '\0';
			else
				break;
		}
		tempPtr = tempPtr2;

		while (*++tempPtr != '\0') 
		{
			if (*tempPtr >= '0' && *tempPtr <= '9') 
			{
				decimalPart= decimalPart * 10 + (*tempPtr - '0');
				if(*tempPtr != '0' )
					countZeros = false;

				if(*tempPtr == '0' && countZeros ) 
					leadZeros++;
			}
			else
			{
				if (tempPtr2 != NULL)
				      free(tempPtr2);

				return -1;
			}
		}

		if (tempPtr2 != NULL)
			free(tempPtr2);
	}

	return 0;
}

long ODBC::getDigitCount(__int64 value)
{
	int i;
    static __int64 decValue[] = {0,
                               9,
                               99,
                               999,
                               9999,
                               99999,
                               999999,
                               9999999,
                               99999999,
                               999999999,
                               9999999999,
                               99999999999,
                               999999999999,
                               9999999999999,
                               99999999999999,
                               999999999999999,
                               9999999999999999,
                               99999999999999999,
                               999999999999999999};

    for (i = 4; i <= 16; i += 4)
      if (value <= decValue[i]) {
	if (value <= decValue[i-3])
	  return(i-3);
	if (value <= decValue[i-2])
	  return(i-2);
	if (value <= decValue[i-1])
	  return(i-1);
	else return i;
      }
    if (value <= decValue[17])
      return 17;
    if (value <= decValue[18])
      return 18;
    return 19;
}

//
// the conversion from little endian mode to the character string
//

SQLRETURN ODBC::ConvertCNumericToChar( SQL_NUMERIC_STRUCT* numericPtr, char* cTmpBuf)
{

	union
	{
		unsigned __int64 intTest[2];
		SQLCHAR tmpVal[SQL_MAX_NUMERIC_LEN];
	} tmpUnion;

	char localBuf[50];
	char* tempPtr = localBuf;
	char* outBuf = localBuf;

	BOOLEAN bStart = false;
	short offset = SQL_MAX_NUMERIC_LEN - 1;

	memset( localBuf, 0, sizeof(localBuf));
	memcpy( tmpUnion.tmpVal, numericPtr->val, SQL_MAX_NUMERIC_LEN );

	//NOTE: The ODBC 3.0 spec required drivers to return the sign as 
	//1 for positive numbers and 2 for negative number. This was changed in the
	//ODBC 3.5 spec to return 0 for negative instead of 2.
	if( numericPtr->sign == 2 || numericPtr->sign == 0) 
	{
		strcpy( tempPtr, "-" );
		tempPtr++;
	}

	if( tmpUnion.intTest[1] == 0 )
	{
		_ui64toa( tmpUnion.intTest[0], tempPtr, 10 );
	}
	else
	{

		while( tmpUnion.intTest[0] != 0 || tmpUnion.intTest[1] != 0)
		{
			int i,a=0,b=0,current,calc=0;

			for( i = offset; i >= 0 ; i--)
			{
				if( tmpUnion.tmpVal[ i ] == 0 && !bStart)
				{
					offset--;
					continue;
				}
				else
					bStart = true;

				current = tmpUnion.tmpVal[ i ];
				calc = calc * 256 + current;
				a = calc % 10;
				b = calc / 10;

				tmpUnion.tmpVal[ i ] =  b;
				calc = a;
			}
			*tempPtr++ = a + '0';
		}

		tempPtr--;

		while (tempPtr >= outBuf )
		{
			char cTmp;

			cTmp = *outBuf;
			*(outBuf++) = *tempPtr;
			*(tempPtr--) = cTmp;
		}

	}

	memset(cTmpBuf, 0, numericPtr->precision + 1);
	strncpy(cTmpBuf, localBuf, strlen(localBuf) - numericPtr->scale );
	if( numericPtr->scale != 0 ){
		strcat(cTmpBuf,".");
		strcat(cTmpBuf, localBuf + strlen(localBuf) - numericPtr->scale );
	}

	return SQL_SUCCESS;
}

SQLRETURN ODBC::StripIntervalLiterals(SQLPOINTER srcDataPtr, SQLINTEGER srcLength, SWORD ODBCDataType, char* cTmpBuf)
{						  

    char      in_value[128];
    char      save_value[128];
    char	*token;
    short	i = 0;
    char	delimiters[] = " {}\t\n";
    char	validValue[] = "1234567890:-. ";
	int		tempLen;
	BOOL	signSet = FALSE;
	int		ch = '\'';
	char	*pdest;
	int		bresult, eresult;
	
	if (srcLength == SQL_NTS )
		tempLen = strlen((const char *)srcDataPtr);
	else
		tempLen = srcLength;
	strncpy(in_value, (const char *)srcDataPtr, tempLen);
	in_value[tempLen] = '\0';

	if(in_value[0] != '{' && in_value[tempLen-1] != '}') // No Interval Literal just the value
	{
		i = strspn(in_value, validValue);
		if (i < strlen(in_value)) return SQL_ERROR;
		strcpy(cTmpBuf, in_value);
	}
	else
	{
		pdest = strchr( in_value, ch );
		if( pdest == NULL ) return SQL_ERROR;
		bresult = pdest - in_value + 1;
		/* Search backward. */
		pdest = strrchr( in_value, ch );
		if( pdest == NULL ) return SQL_ERROR;
		eresult = pdest - in_value + 1;

		if (bresult == eresult) return SQL_ERROR;
		strncpy(save_value, in_value+bresult, eresult-bresult);
		save_value[eresult-bresult-1] = '\0';
		_strnset(in_value+bresult-1,' ',eresult-bresult+1);

		token = strtok( in_value, delimiters );
		while( token != NULL )
		{
			switch(i)
			{
			case 0:
				if (_stricmp(token, "INTERVAL") != 0) return SQL_ERROR;
				break;
			case 1:
				switch (ODBCDataType)
				{
				case SQL_INTERVAL_MONTH:
					if (_strnicmp(token, "MONTH",5) != 0) return SQL_ERROR;
					break;
				case SQL_INTERVAL_YEAR:
				case SQL_INTERVAL_YEAR_TO_MONTH:
					if (_strnicmp(token, "YEAR",4) != 0) return SQL_ERROR;
					break;
				case SQL_INTERVAL_DAY:
				case SQL_INTERVAL_DAY_TO_HOUR:
				case SQL_INTERVAL_DAY_TO_MINUTE:
				case SQL_INTERVAL_DAY_TO_SECOND:
					if (_strnicmp(token, "DAY",3) != 0) return SQL_ERROR;
					break;
				case SQL_INTERVAL_HOUR:
				case SQL_INTERVAL_HOUR_TO_MINUTE:
				case SQL_INTERVAL_HOUR_TO_SECOND:
					if (_strnicmp(token, "HOUR",4) != 0) return SQL_ERROR;
					break;
				case SQL_INTERVAL_MINUTE:
				case SQL_INTERVAL_MINUTE_TO_SECOND:
					if (_strnicmp(token, "MINUTE",6) != 0) return SQL_ERROR;
					break;
				case SQL_INTERVAL_SECOND:
					if (_strnicmp(token, "SECOND",5) != 0) return SQL_ERROR;
					break;
				default:
					return SQL_ERROR;
				}
				break;
			case 2:
				if (_stricmp(token, "TO") != 0) return SQL_ERROR;
				break;
			case 3:
				switch (ODBCDataType)
				{
				case SQL_INTERVAL_YEAR_TO_MONTH:
					if (_strnicmp(token, "MONTH",5) != 0) return SQL_ERROR;
					break;
				case SQL_INTERVAL_DAY_TO_HOUR:
					if (_strnicmp(token, "HOUR",4) != 0) return SQL_ERROR;
					break;
				case SQL_INTERVAL_DAY_TO_MINUTE:
				case SQL_INTERVAL_HOUR_TO_MINUTE:
					if (_strnicmp(token, "MINUTE",6) != 0) return SQL_ERROR;
					break;
				case SQL_INTERVAL_DAY_TO_SECOND:
				case SQL_INTERVAL_HOUR_TO_SECOND:
				case SQL_INTERVAL_MINUTE_TO_SECOND:
					if (_strnicmp(token, "SECOND",5) != 0) return SQL_ERROR;
					break;
				default:
					return SQL_ERROR;
				}
				break;
			default:
				return SQL_ERROR;
			}
			// Get next token:
			token = strtok( NULL, delimiters );
			if( token != NULL )
			{
				if (i == 0 && ((_stricmp(token, "-") == 0) || (_stricmp(token, "+") == 0)))
				{
					strcpy(cTmpBuf,token);
					signSet = TRUE;
					token = strtok( NULL, delimiters );
				}
			}
			i++;

		}
		if(signSet)
			strcat(cTmpBuf,save_value);
		else
			strcpy(cTmpBuf,save_value);
	}
	return SQL_SUCCESS;
}

void ODBC::InsertBlank(char *str, short numBlank)
{
	char temp[256];
	long length=0;

	memset(temp, ' ', numBlank);
	length = numBlank + strlen(str);
	length = length > 255 ? 255 : length;
	memcpy(&temp[numBlank], str, length-numBlank);
	temp[length] = '\0';
	strcpy(str, temp);
	
	return;
}

unsigned long ODBC::CheckIntervalOverflow(char *intervalValue, SWORD ODBCDataType, SQLINTEGER targetLength, SQLSMALLINT secPrecision)
{
	char	*token;
	short	i = 0;
	short   j = 0;
	char	in_value[128 + 1];
	char	delimiters[] = " :.-";
	char	sep[5]={0,0,0,0,0};
	SQLINTEGER leadingPrecision; 
	unsigned __int64 integralMax;
	unsigned __int64 decimalMax;
	unsigned __int64 tempVal64;
	unsigned int tempVal;

	// already exceeds the target length
	if (strlen(intervalValue) > targetLength) return IDS_22_015;

	if (intervalValue[0] == '-') 
		strcpy(in_value, intervalValue+1);
	else
		strcpy(in_value, intervalValue);

	leadingPrecision = GetIntervalLeadingPrecision(ODBCDataType, targetLength, secPrecision);

	j = strcspn(in_value, delimiters);
	sep[i] = in_value[j];
	token = strtok(in_value, delimiters);
	while( token != NULL )
	{
		switch(i)
		{
		case 0:
			getMaxNum(leadingPrecision, 0, integralMax, decimalMax);
			tempVal64 = _atoi64(token);
			if (tempVal64 > integralMax) return IDS_22_015;
			break;

		case 1:
			switch (ODBCDataType)
			{	
			case SQL_INTERVAL_SECOND:
				if (secPrecision == 0) return IDS_22_015;
				if (sep[i-1] != '.') return IDS_22_018;
				getMaxNum(secPrecision, 0, integralMax, decimalMax);
				tempVal64 = _atoi64(token);
				if (tempVal64 > integralMax) return IDS_22_015;
				break;
			case SQL_INTERVAL_YEAR_TO_MONTH:
				if (sep[i-1] != '-') return IDS_22_018;
				// month value must be between 0 and 11, inclusive
				tempVal = atoi(token);
				if (tempVal < 0 || tempVal > 11) return IDS_22_015;
				break;
			case SQL_INTERVAL_DAY_TO_HOUR:
			case SQL_INTERVAL_DAY_TO_MINUTE:
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (sep[i-1] != ' ') return IDS_22_018;
				// hour value must be between 0 and 23, inclusive
				tempVal = atoi(token);
				if (tempVal < 0 || tempVal > 23) return IDS_22_015;
				break;
			case SQL_INTERVAL_HOUR_TO_MINUTE:
			case SQL_INTERVAL_HOUR_TO_SECOND:
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				if (sep[i-1] != ':') return IDS_22_018;
				// minute or second value must be between 0 and 59, inclusive
				tempVal = atoi(token);
				if (tempVal < 0 || tempVal > 59) return IDS_22_015;
				break;
			default:
				return IDS_22_018;
			}
			break;

		case 2:
			switch (ODBCDataType)
			{	
			case SQL_INTERVAL_DAY_TO_MINUTE:
			case SQL_INTERVAL_DAY_TO_SECOND:
			case SQL_INTERVAL_HOUR_TO_SECOND:
				if (sep[i-1] != ':') return IDS_22_018;
				// minute or second value must be between 0 and 59, inclusive
				tempVal = atoi(token);
				if (tempVal < 0 || tempVal > 59) return IDS_22_015;
				break;
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				if (secPrecision == 0) return IDS_22_015;
				if (sep[i-1] != '.') return IDS_22_018;
				getMaxNum(secPrecision, 0, integralMax, decimalMax);
				tempVal64 = _atoi64(token);
				if (tempVal64 > integralMax) return IDS_22_015;		
				break;
			default:
				return IDS_22_018;
			}
			break;

		case 3:
			switch (ODBCDataType)
			{
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (sep[i-1] != ':') return IDS_22_018;
				// second value must be between 0 and 59, inclusive
				tempVal = atoi(token);
				if (tempVal < 0 || tempVal > 59) return IDS_22_015;
				break;	
			case SQL_INTERVAL_HOUR_TO_SECOND:
				if (secPrecision == 0) return IDS_22_015;
				if (sep[i-1] != '.') return IDS_22_018;
				getMaxNum(secPrecision, 0, integralMax, decimalMax);
				tempVal64 = _atoi64(token);
				if (tempVal64 > integralMax) return IDS_22_015;		
				break;
			default:
				return IDS_22_018;
			}
			break;
		
		case 4:
			switch (ODBCDataType)
			{
			case SQL_INTERVAL_DAY_TO_SECOND:
				if (secPrecision == 0) return IDS_22_015;
				if (sep[i-1] != '.') return IDS_22_018;
				getMaxNum(secPrecision, 0, integralMax, decimalMax);
				tempVal64 = _atoi64(token);
				if (tempVal64 > integralMax) return IDS_22_015;		
				break;
			default:
				return IDS_22_018;
			}
			break;

		default:
			return IDS_22_018;
		}
		// Get next token:
		i++;
		j += strcspn(in_value+j+1, delimiters)+1;
		if (j > 128)
			return IDS_22_018;
		sep[i] = in_value[j];
		token = strtok(NULL, delimiters);
	}

	return SQL_SUCCESS;
}

SQLINTEGER ODBC::GetIntervalLeadingPrecision(SWORD ODBCDataType, SQLINTEGER targetLength, SQLSMALLINT secPrecision)
{
	SQLINTEGER leadingPrecision = 0;
	switch (ODBCDataType)
	{
	case SQL_INTERVAL_SECOND:
		if (secPrecision == 0)
			leadingPrecision = targetLength-1;
		else
			leadingPrecision = targetLength-secPrecision-2;
		break;
	case SQL_INTERVAL_DAY_TO_SECOND:
		if (secPrecision == 0)
			leadingPrecision = targetLength-10;
		else
			leadingPrecision = targetLength-secPrecision-11;
		break;
	case SQL_INTERVAL_HOUR_TO_SECOND:
		if (secPrecision == 0)
			leadingPrecision = targetLength-7;
		else
			leadingPrecision = targetLength-secPrecision-8;
		break;
	case SQL_INTERVAL_MINUTE_TO_SECOND:
		if (secPrecision == 0)
			leadingPrecision = targetLength-4;
		else
			leadingPrecision = targetLength-secPrecision-5;
		break;
	case SQL_INTERVAL_YEAR:
	case SQL_INTERVAL_MONTH:
	case SQL_INTERVAL_DAY:
	case SQL_INTERVAL_HOUR:
	case SQL_INTERVAL_MINUTE:
		leadingPrecision = targetLength-1;
		break;
	case SQL_INTERVAL_YEAR_TO_MONTH:
	case SQL_INTERVAL_DAY_TO_HOUR:
	case SQL_INTERVAL_HOUR_TO_MINUTE:
		leadingPrecision = targetLength-4;
		break;
	case SQL_INTERVAL_DAY_TO_MINUTE:
		leadingPrecision = targetLength-7;
		break;
	default:
		break;
	}

	return leadingPrecision;
}
