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
#define OUTBUF_MAX		1024  //10-090504-1334
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

#define MAXCHARLEN 32768 //32K

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

// The Qmove2 moves two bytes from an arbitrary (unaligned) location to an aligned one.
// This routine is needed and used where a normal assignment statement would
// cause an alignment exception.  The inputs are pointer to source and destination, respectively.
// The source pointer is not modified.

#ifdef MXHPUX
void Qmove2(char *S, char *D)
{
char *T = S;
	*D++ = *T++;
	*D = *T;
}
#endif

using namespace ODBC;

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
		
		retcode = convDoItMxcs((char *)source, sourceLen, sourceType, sourcePrecision, sourceScale,
					target, 1+sourcePrecision+1, 0,0,0,0, &truncation);
				
		if (retcode != 0)
			return IDS_22_003;
		//return IDS_07_006;	

		testb = new char[sourcePrecision+4];

		if (gDrvrGlobal.gSpecial_1) 
		{	//need no leading zero
			strcpy(testb,target);
			if((testb[0] == '0')||(testb[0] == '-') && (testb[1] == '0'))  
			{
#ifndef unixcli
				ctemp2 = strtok(testb, ".");
				ctemp3= strtok(NULL, "\0");
#else
				ctemp2 = strtok_r(testb, ".",&saveptr);
				ctemp3= strtok_r(NULL, "\0",&saveptr);
#endif
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
#ifndef unixcli
				ctemp2 = strtok(testb, ".");
				ctemp3= strtok(NULL, "\0");
#else
				ctemp2 = strtok_r(testb, ".",&saveptr);
				ctemp3= strtok_r(NULL, "\0",&saveptr);
#endif
				sprintf(ctemp1, "-0.%s", ctemp3);
				strcpy(target, ctemp1);
			}										
		}

		delete[] testb;

	return SQL_SUCCESS;

} // BigNum_To_Ascii_Helper()


// totalReturnedLength is a Input/Output parameter
// *totalReturnedLength = Offset in Input
// srcLength includes NULL for SQL_CHAR Type, hence srcLength is srcLength-1 for SQL_CHAR fields
unsigned long ODBC::ConvertSQLToC(SQLINTEGER	ODBCAppVersion,
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
							SQLLEN 	*targetStrLenPtr,
							BOOL		byteSwap,
							CHAR		*&translatedDataPtr,
							ICUConverter* iconv,
							SQLINTEGER	*totalReturnedLength,
//							DWORD		translateOption, 
							UCHAR		*errorMsg,
							SWORD		errorMsgMax,
							SQLINTEGER	EnvironmentType,
							BOOL		ColumnwiseData,
//							DWOD		localeTranslation, 
							CHAR		*replacementChar)
{
	SYSTEMTIME			systemtimeTmp;
	
	unsigned long retCode = SQL_SUCCESS;
	SQLPOINTER		DataPtr;
	SQLINTEGER		DataLen;
	SQLINTEGER		DataLen1;
	SQLINTEGER		Offset;
	SQLINTEGER		translateLengthMax;
	SQLINTEGER		translateLength = 0;
	SQLINTEGER		DataLenTruncated = 0;

	char		*stopStr;
	short		i;
	char		*tempPtr;
	double		dTmp;
	SSHORT		sTmp;
	USHORT		usTmp;
	SLONG_P		lTmp;
	ULONG_P		ulTmp;
	CHAR		cTmpBuf[OUTBUF_MAX];  //10-090504-1334
	CHAR		cTmpBuf1[30];
	USHORT		tmpUShort;
	SSHORT		tmpShort;
	__int64		tempVal64;
	__int64		power;
	__int64		tempValFrac;
#ifndef unixcli
#ifndef NSK_PLATFORM
	unsigned __int64 utempVal64;
	unsigned __int64 utempValFrac;
#endif
#else
	__int64 utempVal64;
	__int64 utempValFrac;
#endif
	SQLINTEGER	DecimalPoint;
	SQLINTEGER	Sign;
	float		fltTmp;
	SCHAR		tTmp;
	UCHAR		utTmp;
	BOOL		NullTerminate = FALSE;
	BOOL		NullTerminateW = FALSE;
	BOOL		LocalizeNumericString = FALSE;
	WORD		LangId;
	SQLUINTEGER ulFraction;
	char sFraction[10] = {'\0'};

	BOOL		useDouble = TRUE;

	DATE_STRUCT			dateTmp;
	TIME_STRUCT			timeTmp;
	TIMESTAMP_STRUCT	timestampTmp;
	SQL_INTERVAL_STRUCT	intervalTmp;
	DATE_TYPES			*SQLDate;
	TIME_TYPES			*SQLTime;
	TIMESTAMP_TYPES		*SQLTimestamp;
	SQL_NUMERIC_STRUCT	numericTmp;

	DATE_TYPES			SQLDateTmp;
	TIME_TYPES			SQLTimeTmp;
	TIMESTAMP_TYPES		SQLTimestampTmp;
	SQLSMALLINT			tODBCDataType;
	BOOL				signedInteger = FALSE;
	BOOL				unsignedInteger = FALSE;
	short sourceType= 0;
	short retcode = -1;
	bool WCharData = false;
	char Dest[MAXCHARLEN];
	short srcSz;
	int rc;
	bool DefaultCharRequired;
	LPBOOL PtrDefaultCharRequired = (LPBOOL)&DefaultCharRequired;	
	DWORD lastError;
	bool isshort;
	int short_len;
	int int_len;
	int charlength=0;

	if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable){
		TraceOut(TR_ODBC_DEBUG,"ConvertSQLToC(%d, %d, %d, %d, %#x, %d, %d, %d, %d, %d, %#x, %d, %#x, %d, %d, %#x, %d, %d)",
							ODBCAppVersion,
							SQLDataType,
							ODBCDataType,
							SQLDatetimeCode,
							srcDataPtr,
							srcLength,
							srcPrecision,
							srcScale,
							srcUnsigned,
							CDataType,
							targetDataPtr,
							targetLength,
							targetStrLenPtr,
							byteSwap,
							totalReturnedLength,
//							translateOption,
							errorMsg,
							errorMsgMax,
							EnvironmentType
							);
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
	isshort=srcMaxLength<=32767;

	tODBCDataType = ODBCDataType;
	if (ODBCDataType == SQL_NUMERIC && SQLDataType == SQLTYPECODE_LARGEINT &&
					srcPrecision == 19 && srcScale==0)
	{
		ODBCDataType = SQL_BIGINT;
	}

	if (ODBCDataType == SQL_INTEGER && SQLDataType == SQLTYPECODE_SMALLINT_UNSIGNED &&
				srcPrecision == 10 && srcScale==0)
	{
		srcPrecision = 5;
		ODBCDataType = SQL_SMALLINT;
		tODBCDataType = ODBCDataType;
		srcUnsigned = true;
	}

	if (ODBCDataType == SQL_NUMERIC && SQLDataType == SQLTYPECODE_INTEGER_UNSIGNED &&
				srcPrecision == 19 && srcScale==0)
	{
		srcPrecision = 10;
		ODBCDataType = SQL_INTEGER;
		srcUnsigned = true;
	}

	if (ODBCDataType == SQL_BIGINT && SQLDataType == SQLTYPECODE_INTEGER_UNSIGNED &&
				srcPrecision == 19 && srcScale==0)
	{
		srcPrecision = 10;
		ODBCDataType = SQL_INTEGER;
		srcUnsigned = true;
	}

    if (ODBCDataType == SQL_BIGINT && SQLDataType == SQLTYPECODE_LARGEINT_UNSIGNED &&
                srcPrecision == 19 && srcScale==0)
    {
        srcUnsigned = true;
    }

	if (CDataType == SQL_C_DEFAULT)
	{
		getCDefault(tODBCDataType, ODBCAppVersion, srcCharSet, CDataType);
		if (ODBCAppVersion >= 3 && srcUnsigned)
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


	if (errorMsg != NULL)
		*errorMsg = '\0';

	if (srcDataPtr == NULL)
		return IDS_HY_000;

//	if (charsetSupport)
		LangId = LANG_NEUTRAL;
//	else
//		LangId = LANGIDFROMLCID(DataLangId);

	if (totalReturnedLength != NULL)
	{
		Offset = *totalReturnedLength;
		*totalReturnedLength = -1;
	}
	else
		Offset = 0;

//	if (byteSwap)
//	{
//		if (Datatype_Dependent_Swap((BYTE *)srcDataPtr,SQLDataType, srcLength, TANDEM_TO_IEEE) != STATUS_OK)
//			return IDS_HY_000;
//	}
	switch (CDataType)
	{
	case SQL_C_CHAR:
		switch (ODBCDataType)
		{
		case SQL_CHAR:
            if(SQLDataType == SQLTYPECODE_BOOLEAN)
            {
                tTmp = *((SCHAR *) srcDataPtr);
                _ltoa(tTmp, cTmpBuf, 10);
                DataLen = strlen(cTmpBuf);
                if(DataLen > targetLength)
                    return IDS_22_003;
                DataPtr = cTmpBuf;
                break;
            }
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
			if (translatedDataPtr == NULL)
			{
				DataLen = charlength - Offset;		
				if (DataLen == 0)
					return SQL_NO_DATA;
				DataPtr = (char *)srcDataPtr + Offset;
					
				if (!(iconv->isIso88591Translation()) && DataLen >= targetLength && srcCharSet != SQLCHARSETCODE_UCS2 && srcCharSet != SQLCHARSETCODE_UTF8)
				{
					if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
						TraceOut(TR_ODBC_DEBUG, "ConvertSQLToC: \"Data truncated\" OBCDataType %d, srcCharSet %d, DataLen %d, targetLength %d",
								 ODBCDataType, srcCharSet, DataLen, targetLength);
					retCode = IDS_01_004;
					DataLenTruncated = DataLen;
					if (targetLength > 0) 
						DataLen = targetLength-1;
					else
						DataLen = 0;
				}
			}
//			if (totalReturnedLength != NULL)
//				*totalReturnedLength = DataLen + Offset;
			break;
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WVARCHAR:
			if(isshort){
				short_len=*(USHORT *)srcDataPtr;
				charlength=short_len;
				if (short_len == 0)
				{	
					if (targetStrLenPtr != NULL)
						*targetStrLenPtr = 0;
					if (targetLength > 0)
						((char*)targetDataPtr)[0] = '\0';
					return retCode;
				}
			}
			else{
				int_len=*(int *)srcDataPtr;
				charlength=int_len;
				if (int_len == 0)
				{	
					if (targetStrLenPtr != NULL)
						*targetStrLenPtr = 0;
					if (targetLength > 0)
						((char*)targetDataPtr)[0] = '\0';
					return retCode;
				}
			}
			if (translatedDataPtr == NULL)
			{
				if(isshort){
					DataLen = short_len - Offset;
					DataPtr = (char *)srcDataPtr + 2 + Offset;
				}
				else{
					DataLen = int_len - Offset;
					DataPtr = (char *)srcDataPtr + 4 + Offset;
				}
				if (DataLen == 0)
					return SQL_NO_DATA;	
				if (!(iconv->isIso88591Translation()) && DataLen >= targetLength && srcCharSet != SQLCHARSETCODE_UCS2 && srcCharSet != SQLCHARSETCODE_UTF8)
				{
					if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
						TraceOut(TR_ODBC_DEBUG, "ConvertSQLToC: \"Data truncated\" OBCDataType %d, srcCharSet %d, DataLen %d, targetLength %d",
								 ODBCDataType, srcCharSet, DataLen, targetLength);
					retCode = IDS_01_004;
					DataLenTruncated = DataLen;
					if (targetLength > 0) 
						DataLen = targetLength-1;
					else
						DataLen = 0;
				}
			}
//			if (totalReturnedLength != NULL )
//				*totalReturnedLength = DataLen + Offset;
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
			DataPtr = (char *)srcDataPtr + Offset;
//
// SQL does not return null terminating character - we have to use octet length
//
//			DataLen = strlen((char*)DataPtr);
			DataLen = srcLength;
			if (DataLen >= targetLength)
			{
				retCode = IDS_01_004;
				DataLenTruncated = srcLength - Offset;
				if (targetLength > 0)
					DataLen = targetLength - 1;
				else
					DataLen = 0;
			}
//			if (totalReturnedLength != NULL)
//				*totalReturnedLength = DataLen + Offset;
			break;
        case SQL_TINYINT:
            if(srcUnsigned)
            {
                utTmp = *((UCHAR *) srcDataPtr);
                _ltoa(utTmp, cTmpBuf, 10);
            }
            else
            {
                tTmp = *((SCHAR *) srcDataPtr);
                _ltoa(tTmp, cTmpBuf, 10);
            }
            DataLen = strlen(cTmpBuf);
            if(DataLen > targetLength)
                return IDS_22_003;
            DataPtr = cTmpBuf;
            LocalizeNumericString = TRUE;
            break;
		case SQL_SMALLINT:
			if (srcUnsigned)
				lTmp = *((USHORT *) srcDataPtr);
			else
				lTmp = *((SSHORT *) srcDataPtr);
			_ltoa(lTmp, cTmpBuf, 10);
			DataLen = strlen(cTmpBuf);
			if (DataLen > targetLength)
				return IDS_22_003;
			DataPtr = cTmpBuf;
			LocalizeNumericString = TRUE;
			break;
		case SQL_INTEGER: 
			if (srcUnsigned)
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
			DataPtr = cTmpBuf;
			LocalizeNumericString = TRUE;
			break;
		case SQL_BIGINT:
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
            if (srcUnsigned)
                sprintf( cTmpBuf, "%lu", *((unsigned __int64 *)srcDataPtr));
            else
                sprintf( cTmpBuf, "%ld", *((__int64 *)srcDataPtr));
#else
            if (srcUnsigned)
                sprintf( cTmpBuf, "%llu", *((unsigned __int64 *)srcDataPtr));
            else
                sprintf( cTmpBuf, "%lld", *((__int64 *)srcDataPtr));
#endif
			DataLen = strlen(cTmpBuf);
			if (DataLen > targetLength)
				return IDS_22_003;
			DataPtr = cTmpBuf;
			LocalizeNumericString = TRUE;
			break;
		case SQL_NUMERIC:
			if( ((SQLDataType == SQLTYPECODE_NUMERIC) && (srcPrecision > 18)) || 
				((SQLDataType == SQLTYPECODE_NUMERIC_UNSIGNED) && (srcPrecision > 9))) //for bignum support
			{
				retCode = BigNum_To_Ascii_Helper((char*)srcDataPtr,srcLength,srcPrecision,srcScale,cTmpBuf,SQLDataType);
				if(retCode != SQL_SUCCESS)
					return retCode;
			} else {
				if ((ConvertNumericToChar(SQLDataType, srcDataPtr, srcScale, cTmpBuf, DecimalPoint)) != SQL_SUCCESS)
					return IDS_07_006;
				if (DecimalPoint > targetLength)
					return IDS_22_003;
			}
			DataLen = strlen(cTmpBuf);
			if (DataLen  > targetLength)
			{
				DataLen = targetLength-1;
				retCode = IDS_01_004;
			}
			DataPtr = cTmpBuf;
			LocalizeNumericString = TRUE;
			break;
		case SQL_DECIMAL:
			if (ConvertDecimalToChar(SQLDataType, srcDataPtr, srcLength, srcScale, 
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
			DataPtr = cTmpBuf;
			LocalizeNumericString = TRUE;
			break;
		case SQL_REAL:
		case SQL_DOUBLE:
			if ((SQLDataType == SQLTYPECODE_DECIMAL_LARGE_UNSIGNED) ||
				(SQLDataType == SQLTYPECODE_DECIMAL_LARGE))
			{
				if (ConvertSoftDecimalToDouble(SQLDataType, srcDataPtr, srcLength, srcScale, 
							dTmp)  != SQL_SUCCESS)
					return IDS_07_006;
//				_gcvt(dTmp, DBL_DIG, cTmpBuf);
				if (!double_to_char (dTmp, DBL_DIG, cTmpBuf, targetLength))
					return IDS_22_001;			
			}
			else
			{
				if (ODBCDataType == SQL_REAL) {
					dTmp = (double)(*(float *)srcDataPtr);
//					_gcvt(dTmp, FLT_DIG + 1, cTmpBuf);
					if (!double_to_char (dTmp, FLT_DIG + 1, cTmpBuf, targetLength))
						return IDS_22_001;
				}
				else {
					dTmp = *(double *)srcDataPtr;
//					_gcvt(dTmp, DBL_DIG, cTmpBuf);
					if (!double_to_char (dTmp, DBL_DIG, cTmpBuf, targetLength))
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
			DataPtr = cTmpBuf;
			LocalizeNumericString = TRUE;
			break;
		case SQL_DATE:
		case SQL_TYPE_DATE:
			if (!ColumnwiseData) //RowwiseRowSet
			{
				SQLDate = &SQLDateTmp;
				memset(SQLDate,0,sizeof(DATE_TYPES));
				DataPtr = cTmpBuf;
				SQLDate = (DATE_TYPES *)srcDataPtr;
				DataLen = sprintf((char*)cTmpBuf, "%04d-%02d-%02d", SQLDate->year, SQLDate->month, SQLDate->day);
			       	break;
				if (targetLength <= DataLen )
					return IDS_22_003;
			}
		case SQL_TIME:
		case SQL_TYPE_TIME:
			if (!ColumnwiseData) //RowwiseRowSet
			{
				SQLTime = &SQLTimeTmp;
				memset(SQLTime,0,sizeof(TIME_TYPES));
				DataPtr = cTmpBuf;
				SQLTime = (TIME_TYPES *)srcDataPtr;
				if (srcPrecision > 0 )
				{
					ulFraction = 0;	
					ulFraction = *(UDWORD_P*)SQLTime->fraction;
					sprintf(sFraction, "%0*lu", srcPrecision, ulFraction);
					DataLen = sprintf((char*)cTmpBuf, "%02d:%02d:%02d.%s", 
						SQLTime->hour, SQLTime->minute, SQLTime->second, sFraction);
				}
				else
					DataLen = sprintf((char*)cTmpBuf, "%02d:%02d:%02d", 
				SQLTime->hour, SQLTime->minute, SQLTime->second);
				break;
				if( targetLength <= DataLen )
					return IDS_22_003;
			}
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
			if (ColumnwiseData) // !RowwiseRowSet
			{
				DataLen = srcLength;
				DataPtr = srcDataPtr;
			}
			else
			{
				ulFraction = 0;
				DataPtr = cTmpBuf;
				SQLTimestamp = &SQLTimestampTmp;
				memset (SQLTimestamp, 0, sizeof(TIMESTAMP_TYPES));
				SQLTimestamp = (TIMESTAMP_TYPES *)srcDataPtr;
				if (srcPrecision > 0 )
				{
					ulFraction = *(UDWORD_P*)SQLTimestamp->fraction;
					sprintf(sFraction, "%0*lu", srcPrecision, ulFraction);
										ulFraction = atol(sFraction);
					DataLen = sprintf(cTmpBuf, "%04d-%02u-%02u %02u:%02u:%02u.%s",
							SQLTimestamp->year, SQLTimestamp->month, SQLTimestamp->day,
							SQLTimestamp->hour, SQLTimestamp->minute, SQLTimestamp->second,
							sFraction);
				}
       			else
					DataLen = sprintf(cTmpBuf, "%04d-%02u-%02u %02u:%02u:%02u",
							SQLTimestamp->year, SQLTimestamp->month, SQLTimestamp->day,
							SQLTimestamp->hour, SQLTimestamp->minute, SQLTimestamp->second);

			}
			if (targetLength > DataLen)
				DataLen = DataLen;
			else if (targetLength >= 20)
			{
				DataLen = targetLength-1;
				retCode = IDS_01_S07;
			}
			else
				return IDS_22_003;
				
			break;
		default:
			return IDS_07_006;
		}
		NullTerminate = TRUE;
		break; // End of SQL_C_CHAR
	case SQL_C_WCHAR :
		{ //JJ
		switch (ODBCDataType)
		{
		 case SQL_CHAR:
            if(SQLDataType == SQLTYPECODE_BOOLEAN)
            {
                tTmp = *((SCHAR *) srcDataPtr);
                _ltoa(tTmp, cTmpBuf, 10);
                DataLen = strlen(cTmpBuf);
                if(DataLen > targetLength)
                    return IDS_22_003;
                DataPtr = cTmpBuf;
                break;
            }
		 case SQL_WCHAR:
			charlength = srcLength-1;
			if (charlength == 0)
			{
				if (targetStrLenPtr != NULL)
					*targetStrLenPtr = 0;
				if (targetLength > 0)
					((char*)targetDataPtr)[0] = '\0';
				return retCode;
			}
			if (translatedDataPtr == NULL)
			{
				DataLen = charlength - Offset;		
				if (DataLen == 0)
					return SQL_NO_DATA;
				if (srcCharSet == SQLCHARSETCODE_UCS2)
				{
					if (targetLength > 0 && DataLen > targetLength-2)
					{
						if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
							TraceOut(TR_ODBC_DEBUG, "ConvertSQLToC: \"Data truncated\" OBCDataType %d, srcCharSet %d, DataLen %d, targetLength %d",
							 ODBCDataType, srcCharSet, DataLen, targetLength);
						retCode = IDS_01_004;
//						if(!targetLength%2)
//							DataLen = targetLength -2;
						DataLenTruncated = DataLen;
						if(iconv->m_AppUnicodeType == APP_UNICODE_TYPE_UTF16)
						{
							DataLen = (targetLength%2)? targetLength - 3 : targetLength - 2 ;
						}
					}
					WCharData = true; // no translation needed incase app is utf16
				}
				DataPtr = (char *)srcDataPtr + Offset;
			}
//			if (totalReturnedLength != NULL)
//				*totalReturnedLength = DataLen + Offset;
			break;
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WVARCHAR:
			if(isshort){
				short_len=*(USHORT *)srcDataPtr;
				charlength=short_len;
				if (short_len == 0)
				{	
					if (targetStrLenPtr != NULL)
						*targetStrLenPtr = 0;
					if (targetLength > 0)
						((char*)targetDataPtr)[0] = '\0';
					return retCode;
				}
			}
			else{
				int_len=*(int *)srcDataPtr;
				charlength=int_len;
				if (int_len == 0)
				{
					if (targetStrLenPtr != NULL)
						*targetStrLenPtr = 0;
					if (targetLength > 0)
						((char*)targetDataPtr)[0] = '\0';
					return retCode;
				}
			}

			if (translatedDataPtr == NULL)
			{
				if(isshort)
					DataLen = short_len - Offset;			
				else
					DataLen = int_len - Offset;			
				if (DataLen == 0)
					return SQL_NO_DATA;
				if (srcCharSet == SQLCHARSETCODE_UCS2)
				{
					if (targetLength > 0 && DataLen > targetLength-2)
					{
						//10-071122-9072 
						if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
							TraceOut(TR_ODBC_DEBUG, "ConvertSQLToC: \"Data truncated\" OBCDataType %d, srcCharSet %d, DataLen %d, targetLength %d",
									 ODBCDataType, srcCharSet, DataLen, targetLength);
						retCode = IDS_01_004;
						DataLenTruncated = DataLen;
						if(iconv->m_AppUnicodeType == APP_UNICODE_TYPE_UTF16)
						{
							DataLen = (targetLength%2)? targetLength - 3 : targetLength - 2 ;
						}
					}
					WCharData = true; // No translation needed, if app is utf16
				}
				if(isshort)
					DataPtr = (char *)srcDataPtr + 2 + Offset;
				else
					DataPtr = (char *)srcDataPtr + 4 + Offset;
			}
//			if (totalReturnedLength != NULL )
//				*totalReturnedLength = DataLen + Offset;
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
			DataPtr = (char *)srcDataPtr + Offset;
//
// SQL does not return null terminating character - we have to use octet length
//
//			DataLen = strlen((char*)DataPtr);
			DataLen = srcLength;
			if (targetLength > 0 && DataLen >= targetLength)
			{
				retCode = IDS_01_004;
				DataLenTruncated = srcLength - Offset;
				if (targetLength > 0)
					DataLen = targetLength - 1;
				else
					DataLen = 0;
			}
//			if (totalReturnedLength != NULL)
//				*totalReturnedLength = DataLen + Offset;
			break;
        case SQL_TINYINT:
            if(srcUnsigned)
            {
                utTmp = *((UCHAR *) srcDataPtr);
                _ultoa(utTmp, cTmpBuf, 10);
            }
            else
            {
                tTmp = *((SCHAR *) srcDataPtr);
                _ltoa(tTmp, cTmpBuf, 10);
            }
            DataLen = strlen(cTmpBuf);
            if(DataLen > targetLength)
                return IDS_22_003;
            DataPtr = cTmpBuf;
            LocalizeNumericString = TRUE;
            break;
		case SQL_SMALLINT:
			if (srcUnsigned)
				lTmp = *((USHORT *) srcDataPtr);
			else
				lTmp = *((SSHORT *) srcDataPtr);
			_ltoa(lTmp, cTmpBuf, 10);
			DataLen = strlen(cTmpBuf);
			if (DataLen > targetLength)
				return IDS_22_003;
			DataPtr = cTmpBuf;
			LocalizeNumericString = TRUE;
			break;
		case SQL_INTEGER: 
			if (srcUnsigned)
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
			DataPtr = cTmpBuf;
			LocalizeNumericString = TRUE;
			break;
		case SQL_BIGINT:
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
			sprintf( cTmpBuf, "%Ld", *((__int64 *)srcDataPtr));
#else
			sprintf( cTmpBuf, "%lld", *((__int64 *)srcDataPtr));
#endif
			DataLen = strlen(cTmpBuf);
			if (DataLen > targetLength)
				return IDS_22_003;
			DataPtr = cTmpBuf;
			LocalizeNumericString = TRUE;
			break;
		case SQL_NUMERIC:
			if( ((SQLDataType == SQLTYPECODE_NUMERIC) && (srcPrecision > 18)) || 
				((SQLDataType == SQLTYPECODE_NUMERIC_UNSIGNED) && (srcPrecision > 9))) //for bignum support
			{
				retCode = BigNum_To_Ascii_Helper((char*)srcDataPtr,srcLength,srcPrecision,srcScale,cTmpBuf,SQLDataType);
				if(retCode != SQL_SUCCESS)
					return retCode;
			} else {
				if ((ConvertNumericToChar(SQLDataType, srcDataPtr, srcScale, cTmpBuf, DecimalPoint)) != SQL_SUCCESS)
					return IDS_07_006;
				if (DecimalPoint > targetLength)
					return IDS_22_003;
			}
			DataLen = strlen(cTmpBuf);
			if (DataLen  > targetLength)
			{
				DataLen = targetLength-1;
				retCode = IDS_01_004;
			}
			DataPtr = cTmpBuf;
			LocalizeNumericString = TRUE;			
			break;
		case SQL_DECIMAL:
			if (ConvertDecimalToChar(SQLDataType, srcDataPtr, srcLength, srcScale, 
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
			DataPtr = cTmpBuf;
			LocalizeNumericString = TRUE;
			break;
		case SQL_REAL:
		case SQL_DOUBLE:
			if ((SQLDataType == SQLTYPECODE_DECIMAL_LARGE_UNSIGNED) ||
				(SQLDataType == SQLTYPECODE_DECIMAL_LARGE))
			{
				if (ConvertSoftDecimalToDouble(SQLDataType, srcDataPtr, srcLength, srcScale, 
							dTmp)  != SQL_SUCCESS)
					return IDS_07_006;
//				_gcvt(dTmp, DBL_DIG, cTmpBuf);
				if (!double_to_char (dTmp, DBL_DIG, cTmpBuf, targetLength))
					return IDS_22_001;
			
			}
			else
			{
				if (ODBCDataType == SQL_REAL) {
					dTmp = (double)(*(float *)srcDataPtr);
//					_gcvt(dTmp, FLT_DIG + 1, cTmpBuf);
					if (!double_to_char (dTmp, FLT_DIG + 1, cTmpBuf, targetLength))
						return IDS_22_001;
				}
				else {
					dTmp = *(double *)srcDataPtr;
//					_gcvt(dTmp, DBL_DIG, cTmpBuf);
					if (!double_to_char (dTmp, DBL_DIG, cTmpBuf, targetLength))
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
			DataPtr = cTmpBuf;
			LocalizeNumericString = TRUE;
			break;
		case SQL_DATE:
		case SQL_TYPE_DATE:
			if (!ColumnwiseData) //RowwiseRowSet
			{
				SQLDate = &SQLDateTmp;
				memset(SQLDate,0,sizeof(DATE_TYPES));
				DataPtr = cTmpBuf;
				SQLDate = (DATE_TYPES *)srcDataPtr;
				DataLen = sprintf((char*)cTmpBuf, "%04d-%02d-%02d", SQLDate->year, SQLDate->month, SQLDate->day);
			       	break;
				if (targetLength <= DataLen )
					return IDS_22_003;
			}
		case SQL_TIME:
		case SQL_TYPE_TIME:
			if (!ColumnwiseData) //RowwiseRowSet
			{
				SQLTime = &SQLTimeTmp;
				memset(SQLTime,0,sizeof(TIME_TYPES));
				DataPtr = cTmpBuf;
				SQLTime = (TIME_TYPES *)srcDataPtr;
				if (srcPrecision > 0 )
				{
					ulFraction = 0;	
					ulFraction = *(UDWORD_P*)SQLTime->fraction;
					sprintf(sFraction, "%0*lu", srcPrecision, ulFraction);
					DataLen = sprintf((char*)cTmpBuf, "%02d:%02d:%02d.%s", 
						SQLTime->hour, SQLTime->minute, SQLTime->second, sFraction);
				}
				else
					DataLen = sprintf((char*)cTmpBuf, "%02d:%02d:%02d", 
				SQLTime->hour, SQLTime->minute, SQLTime->second);
				break;
				if( targetLength <= DataLen )
					return IDS_22_003;
			}
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
			if (ColumnwiseData) //!RowwiseRowSet
			{
				DataLen = srcLength;
				DataPtr = srcDataPtr;
			}
			else
			{
				ulFraction = 0;
				DataPtr = cTmpBuf;
				SQLTimestamp = &SQLTimestampTmp;
				memset (SQLTimestamp, 0, sizeof(TIMESTAMP_TYPES));
				SQLTimestamp = (TIMESTAMP_TYPES *)srcDataPtr;
				if (srcPrecision > 0 )
				{
					ulFraction = *(UDWORD_P*)SQLTimestamp->fraction;
					sprintf(sFraction, "%0*lu", srcPrecision, ulFraction);
					ulFraction = atol(sFraction);
					DataLen = sprintf(cTmpBuf, "%04d-%02u-%02u %02u:%02u:%02u.%s",
							SQLTimestamp->year, SQLTimestamp->month, SQLTimestamp->day,
							SQLTimestamp->hour, SQLTimestamp->minute, SQLTimestamp->second,
							sFraction);
				}
         		else
					DataLen = sprintf(cTmpBuf, "%04d-%02u-%02u %02u:%02u:%02u",
							SQLTimestamp->year, SQLTimestamp->month, SQLTimestamp->day,
							SQLTimestamp->hour, SQLTimestamp->minute, SQLTimestamp->second);
				
			}
			if (targetLength > DataLen)
				DataLen = DataLen;
			else if (targetLength >= 20)
			{
				DataLen = targetLength-1;
				retCode = IDS_01_S07;
			}
			else
				return IDS_22_003;
				
			break;
		default:
			return IDS_07_006;
		}
		if(iconv->isAppUTF16())
			NullTerminateW = TRUE;
		else
			NullTerminate = TRUE; //UTF8 buffer
		break; // End of SQL_C_WCHAR
		}//JJ		
	case SQL_C_SHORT:
	case SQL_C_SSHORT:
	case SQL_C_DOUBLE:
	case SQL_C_FLOAT:
	case SQL_C_USHORT:
	case SQL_C_TINYINT:
	case SQL_C_STINYINT:
	case SQL_C_UTINYINT:
	case SQL_C_LONG:
	case SQL_C_SLONG:
	case SQL_C_ULONG:
	case SQL_C_SBIGINT:
	case SQL_C_BIT:
		switch (ODBCDataType)
		{
		case SQL_CHAR:
            if(SQLDataType == SQLTYPECODE_BOOLEAN)
            {
                dTmp = *((SCHAR *) srcDataPtr);
                break;
            }
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WCHAR:
		case SQL_WVARCHAR:
			{
				int TransStringLength = 0;
				char error[64];
				if (ODBCDataType == SQL_CHAR || ODBCDataType == SQL_WCHAR)
				{
					if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to UTF-8
					{
						UChar* spaceStart = u_strchr((UChar*)srcDataPtr, L' ');
						if (spaceStart != NULL)
							srcLength = (spaceStart - (UChar*)srcDataPtr) + 1;
						if(iconv->WCharToUTF8((UChar*)srcDataPtr, srcLength-1, (char*)cTmpBuf,
							sizeof(cTmpBuf), &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
						{
							//	((CHandle*)ConnectionHandle)->setWcharConvError(error); 
							return IDS_22_003;
						}
						//srcLength = TransStringLength + 1;
						srcLength = strlen(cTmpBuf) + 1;
					}
					else
					{
					//remove spaces if any
						char* spaceStart = strchr((char*)srcDataPtr, ' ');
						if (spaceStart != NULL)
							srcLength = (spaceStart - (char*)srcDataPtr) + 1;
						if (srcLength <= sizeof (cTmpBuf)) //Avoid a seg-violation
						{
							strncpy(cTmpBuf, (char*)srcDataPtr, srcLength - 1);
							cTmpBuf[srcLength - 1] = 0;
						}
						else
							return IDS_22_003;
					}
					if (!ctoi64(cTmpBuf, tempVal64 ))
						return IDS_22_003;
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
							if(iconv->WCharToUTF8((UChar*)srcDataPtr+1, DataLen/2, (char*)cTmpBuf,
								(sizeof(cTmpBuf)) , &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
							{
							//We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
								return IDS_22_003;
							}
							srcLength = TransStringLength + 1;
						}
						else
						{
							if((DataLen + 1) <= sizeof(cTmpBuf))
							{
								memcpy(cTmpBuf, (char*)srcDataPtr+2, DataLen);
								srcLength = DataLen + 1;
								cTmpBuf[DataLen] = 0;
							}
							else
								return IDS_22_003;
						}
						if (!ctoi64(cTmpBuf, tempVal64 ))
							return IDS_22_003;
					}
					else
					{
						DataLen = *(UINT *)srcDataPtr;
						if (DataLen == 0)
						return SQL_NO_DATA;
						if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to UTF-8
						{
							if(iconv->WCharToUTF8((UChar*)srcDataPtr+2, DataLen/2, (char*)cTmpBuf,
								(sizeof(cTmpBuf)) , &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
							{
							//We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
								return IDS_22_003;
							}
							srcLength = TransStringLength + 1;
						}
						else
						{
							if((DataLen + 1) <= sizeof(cTmpBuf))
							{
								memcpy(cTmpBuf, (char*)srcDataPtr+4, DataLen);
								srcLength = DataLen + 1;
								cTmpBuf[DataLen] = 0;
							}
							else
								return IDS_22_003;
						}
						if (!ctoi64(cTmpBuf, tempVal64 ))
							return IDS_22_003;
					}
				}
			}
			if ((retCode = ConvertSQLCharToNumeric(cTmpBuf, srcLength, ODBCDataType, dTmp)) != SQL_SUCCESS)
				return retCode;
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
			if ((retCode = ConvertSQLCharToNumeric(srcDataPtr, srcLength, ODBCDataType, dTmp)) != SQL_SUCCESS)
				return retCode;
			break;
        case SQL_TINYINT:
            if (srcUnsigned)
            {
                dTmp = *((UCHAR *) srcDataPtr);
                unsignedInteger = TRUE;
            }
            else
            {
                dTmp = *((SCHAR *) srcDataPtr);
                signedInteger = TRUE;
            }
            break;
		case SQL_SMALLINT:
			if (srcUnsigned)
			{
                #if defined(MXSUNSPARC)
                // on Solaris for release code there is
                // some optimization issue which is causing 
                // the dTmp = *(USHORT*)srcDataPtr to
                // cause a segv
                usTmp = *(USHORT*)srcDataPtr;
                dTmp = usTmp; 
                unsignedInteger = TRUE;
                #else 
				dTmp = *(USHORT *)srcDataPtr;
				unsignedInteger = TRUE;
				#endif
			}
			else
			{
				dTmp = *(SSHORT *)srcDataPtr;
				signedInteger = TRUE;
			}
			break;
		case SQL_INTEGER:
			if (srcUnsigned)
			{
				dTmp = *(ULONG_P *)srcDataPtr;
				unsignedInteger = TRUE;
			}
			else
			{
				dTmp = *(SLONG_P *)srcDataPtr;
				signedInteger = TRUE;
			}
			break;
		case SQL_REAL:
			dTmp = *(SFLOAT *)srcDataPtr;
			break;
		case SQL_DOUBLE:
			if ((SQLDataType == SQLTYPECODE_DECIMAL_LARGE_UNSIGNED) ||
				(SQLDataType == SQLTYPECODE_DECIMAL_LARGE))
			{
				if (ConvertSoftDecimalToDouble(SQLDataType, srcDataPtr, srcLength, srcScale,
							dTmp)  != SQL_SUCCESS)
					return IDS_07_006;
			}
			else
				dTmp =  *(SDOUBLE *)srcDataPtr;
			break;
		case SQL_BIGINT:
			tempVal64 = *((__int64 *)srcDataPtr);
			if (tempVal64 < -DBL_MAX || tempVal64 > DBL_MAX)
				return IDS_22_003;
			dTmp = tempVal64;
            if (srcUnsigned)
                unsignedInteger = TRUE;
            else
                signedInteger = TRUE;
			break;
		case SQL_NUMERIC:
			switch (SQLDataType)
			{
			case SQLTYPECODE_SMALLINT:
#ifdef MXHPUX
				Qmove2((char *)srcDataPtr, (char *)&sTmp);
				dTmp = sTmp;
#elif defined(MXSUNSPARC)
              			tmpShort = *(SHORT*)srcDataPtr;
		                dTmp = tmpShort;
#else	
				dTmp = *((SHORT *)srcDataPtr);
#endif
				if (srcScale > 0)
					dTmp = dTmp / (long)pow(10,srcScale);
				break;
			case SQLTYPECODE_SMALLINT_UNSIGNED:
#ifdef MXHPUX
				Qmove2((char *)srcDataPtr, (char *)&usTmp);
				dTmp = usTmp;
#elif defined(MXSUNSPARC)
              			tmpUShort = *(USHORT*)srcDataPtr;
		                dTmp = tmpUShort;
#else
				dTmp = *((USHORT *)srcDataPtr);
#endif
				if (srcScale > 0)
					dTmp = dTmp / (long)pow(10,srcScale);
				break;
			case SQLTYPECODE_INTEGER:
				dTmp = *((SLONG_P *)srcDataPtr);
				if (srcScale > 0)
					dTmp = dTmp / (long)pow(10,srcScale);
				break;
			case SQLTYPECODE_INTEGER_UNSIGNED:
				dTmp = *((ULONG_P *)srcDataPtr);
				if (srcScale > 0)
					dTmp = dTmp / (long)pow(10,srcScale);
				break;
			case SQLTYPECODE_LARGEINT:
				tempVal64 = *((__int64 *)srcDataPtr);
				for (i = 0, power = 1 ; i < srcScale ; power *= 10, i++);
				tempValFrac = tempVal64 % power;
	    		tempVal64 = tempVal64 / power;
				dTmp = ((double)tempValFrac/(double)power);
				dTmp = dTmp + tempVal64;
				break;

			case SQLTYPECODE_NUMERIC:
			case SQLTYPECODE_NUMERIC_UNSIGNED:
				if( ((SQLDataType == SQLTYPECODE_NUMERIC) && (srcPrecision > 18)) || 
					((SQLDataType == SQLTYPECODE_NUMERIC_UNSIGNED) && (srcPrecision > 9))) //for bignum support
				{
					retCode = BigNum_To_Ascii_Helper((char*)srcDataPtr,srcLength,srcPrecision,srcScale,cTmpBuf,SQLDataType);
					if(retCode != SQL_SUCCESS)
						return retCode;

				} else 
					return IDS_07_006;

				dTmp = strtod(cTmpBuf, &stopStr);
				if(errno == ERANGE)
					return IDS_22_003;

				break;

			default:
				return IDS_HY_000;
			}
			break;
		case SQL_DECIMAL:
			if (ConvertDecimalToChar(SQLDataType, srcDataPtr, srcLength, srcScale,
						cTmpBuf, DecimalPoint) != SQL_SUCCESS)
				return IDS_07_006;
			if (!ctoi64((char*)cTmpBuf, tempVal64 ))
				return IDS_22_003;
			dTmp = strtod(cTmpBuf, &stopStr);
			break;
		default:
			return IDS_07_006;
		}
		switch (CDataType)
		{
		case SQL_C_SHORT:
		case SQL_C_SSHORT:
			if (unsignedInteger)
			{
				if (dTmp > SHRT_MAX)
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
			break;
		case SQL_C_USHORT:
			if (signedInteger)
			{
				if (dTmp < SHRT_MIN || dTmp > SHRT_MAX)
					return IDS_22_003;
				usTmp = (USHORT)dTmp;
			}
			else
			{
				if (dTmp < 0 || dTmp > USHRT_MAX)
					return IDS_22_003;
				usTmp = (USHORT)dTmp;
				if  (dTmp != usTmp)
					retCode = IDS_01_S07;
			}
			DataPtr = &usTmp;
			DataLen = sizeof(usTmp);
			break;
		case SQL_C_TINYINT:
		case SQL_C_STINYINT:
			if (unsignedInteger)
			{
				if (dTmp > SCHAR_MAX)
					return IDS_22_003;
				tTmp = (SCHAR)dTmp;
			}
			else
			{
				if (dTmp < SCHAR_MIN || dTmp > SCHAR_MAX)
					return IDS_22_003;
				tTmp = (SCHAR)dTmp;
				if  (dTmp != tTmp)
					retCode = IDS_01_S07;
			}
			DataPtr = &tTmp;
			DataLen = sizeof(tTmp);
			break;
		case SQL_C_UTINYINT:
			if (signedInteger)
			{
				if (dTmp < SCHAR_MIN || dTmp > SCHAR_MAX)
					return IDS_22_003;
				utTmp = (UCHAR)dTmp;
			}
			else
			{
				if (dTmp < 0 || dTmp > UCHAR_MAX)
					return IDS_22_003;
				utTmp = (UCHAR)dTmp;
				if  (dTmp != utTmp)
					retCode = IDS_01_S07;
			}
			DataPtr = &utTmp;
			DataLen = sizeof(utTmp);
			break;
		case SQL_C_LONG:
		case SQL_C_SLONG:
			if (unsignedInteger)
			{
				if (dTmp > LONG_MAX)
					return IDS_22_003;
				lTmp = (SLONG_P)dTmp;
			}
			else
			{
				if (dTmp < LONG_MIN || dTmp > LONG_MAX)
					return IDS_22_003;
				lTmp = (SLONG_P)dTmp;
				if  (dTmp != lTmp)
					retCode = IDS_01_S07;
			}
			DataPtr = &lTmp;
			DataLen = sizeof(lTmp);
			break;
		case SQL_C_ULONG:
			if (signedInteger)
			{
				if (dTmp < LONG_MIN || dTmp > LONG_MAX)
					return IDS_22_003;
				ulTmp = (ULONG_P)dTmp;
			}
			else
			{
				if( dTmp < 0 || dTmp > ULONG_MAX )
					return IDS_22_003;
				ulTmp = (ULONG)dTmp;
				if (dTmp != ulTmp)
					retCode = IDS_01_S07;
			}
			DataPtr = &ulTmp;
			DataLen = sizeof(ulTmp);
			break;
		case SQL_C_SBIGINT:
			if (ODBCDataType != SQL_BIGINT && 
				ODBCDataType != SQL_DECIMAL &&
				ODBCDataType != SQL_CHAR &&
				ODBCDataType != SQL_VARCHAR &&
				ODBCDataType != SQL_LONGVARCHAR && 
				ODBCDataType != SQL_WCHAR &&
				ODBCDataType != SQL_WVARCHAR ||
                SQLDataType == SQLTYPECODE_BOOLEAN)
				tempVal64 = dTmp;
			DataPtr = &tempVal64;
			DataLen = sizeof(tempVal64);
			break;
		case SQL_C_BIT:
			if(dTmp == 0 || dTmp == 1)
				utTmp = (UCHAR)dTmp;
			else
			if (dTmp > 0 && dTmp < 2)
			{
				utTmp = (UCHAR)dTmp;
				retCode = IDS_01_S07;
			}
			else
				retCode = IDS_22_003;
			DataPtr = &utTmp;
			DataLen = sizeof(utTmp);
			break;
		case SQL_C_DOUBLE:
			DataPtr = &dTmp;
			DataLen = sizeof(dTmp);
			break;
		case SQL_C_FLOAT:
			if (dTmp < -FLT_MAX || dTmp > FLT_MAX)
				return IDS_22_003;
			fltTmp = (FLOAT)dTmp;
			DataPtr = &fltTmp;
			DataLen = sizeof(fltTmp);
			break;
		default:
			return IDS_07_006;
		}
		break;
	case SQL_C_DATE:
	case SQL_C_TYPE_DATE:
		DataPtr = &dateTmp;
		DataLen = sizeof(DATE_STRUCT);
		switch (ODBCDataType)
		{
		case SQL_CHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WCHAR:
		case SQL_WVARCHAR:
			{
				int TransStringLength = 0;
				char error[64];
				if (ODBCDataType == SQL_CHAR || ODBCDataType == SQL_WCHAR)
				{
					if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to UTF-8
					{
						UChar* spaceStart = u_strchr((UChar*)srcDataPtr, L' ');
						if (spaceStart != NULL)
							srcLength = (spaceStart - (UChar*)srcDataPtr) + 1;
						if(iconv->WCharToUTF8((UChar*)srcDataPtr, srcLength-1, (char*)cTmpBuf,
							sizeof(cTmpBuf), &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
						{
						//	((CHandle*)ConnectionHandle)->setWcharConvError(error); 
							return IDS_22_003;
						}
						//srcLength = TransStringLength + 1;
						srcLength = strlen(cTmpBuf) + 1;
					}
					else
					{
					//remove spaces if any
						char* spaceStart = strchr((char*)srcDataPtr, ' ');
						if (spaceStart != NULL)
							srcLength = (spaceStart - (char*)srcDataPtr) + 1;
						if (srcLength <= sizeof (cTmpBuf)) //Avoid a seg-violation
						{
							strncpy(cTmpBuf, (char*)srcDataPtr, srcLength - 1);
							cTmpBuf[srcLength - 1] = 0;
						}
						else
							return IDS_22_003;
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
							if(iconv->WCharToUTF8((UChar*)srcDataPtr+1, DataLen/2, (char*)cTmpBuf,
								(sizeof(cTmpBuf)) , &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
							{
							//We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
								return IDS_22_003;
							}
							srcLength = TransStringLength + 1;
                            DataLen = sizeof(DATE_STRUCT);
						}
						else
						{
							if((DataLen + 1) <= sizeof(cTmpBuf))
							{
								memcpy(cTmpBuf, (char*)srcDataPtr+2, DataLen);
								srcLength = DataLen + 1;
								cTmpBuf[DataLen] = 0;
							}
							else
								return IDS_22_003;
						}
					}
					else
					{
						DataLen = *(UINT *)srcDataPtr;
						if (DataLen == 0)
						return SQL_NO_DATA;
						if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to UTF-8
						{
							if(iconv->WCharToUTF8((UChar*)srcDataPtr+2, DataLen/2, (char*)cTmpBuf,
								(sizeof(cTmpBuf)) , &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
							{
							//We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
								return IDS_22_003;
							}
							srcLength = TransStringLength + 1;
                            DataLen = sizeof(DATE_STRUCT);
						}
						else
						{
							if((DataLen + 1) <= sizeof(cTmpBuf))
							{
								memcpy(cTmpBuf, (char*)srcDataPtr+4, DataLen);
								srcLength = DataLen + 1;
								cTmpBuf[DataLen] = 0;
							}
							else
								return IDS_22_003;
						}
					}
                    DataLen = sizeof(DATE_STRUCT);
				}
			}
			if ((retCode = ConvertSQLCharToDate(ODBCDataType, cTmpBuf, srcLength, SQL_C_DATE, 
					(SQLPOINTER)&dateTmp)) != 0)
				return retCode;
			break;
		case SQL_DATE:
		case SQL_TYPE_DATE:
			SQLDate = &SQLDateTmp;
			if (ColumnwiseData) // !RowwiseRowSet
			{
				SQLDate->year = 01;
				SQLDate->month = 01;
				SQLDate->day = 01;
				switch (SQLDatetimeCode)
				{
				case SQLDTCODE_YEAR:
					SQLDate->year = GetYearFromStr((UCHAR*)srcDataPtr);
					break;
				case SQLDTCODE_YEAR_TO_MONTH:
					SQLDate->year = GetYearFromStr((UCHAR*)srcDataPtr);
					SQLDate->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
					break;
				case SQLDTCODE_MONTH:
					SQLDate->month = GetMonthFromStr((UCHAR*)srcDataPtr);
					break;
				case SQLDTCODE_MONTH_TO_DAY:
					SQLDate->month = GetMonthFromStr((UCHAR*)srcDataPtr);
					SQLDate->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
					break;
				case SQLDTCODE_DAY:
					SQLDate->day = GetDayFromStr((UCHAR*)srcDataPtr);
					break;
				default:
					SQLDate->year = GetYearFromStr((UCHAR*)srcDataPtr);
					SQLDate->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
					SQLDate->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
					break;
				}
			}
			else
			{
				SQLDate = (DATE_TYPES *)srcDataPtr;
			}
			dateTmp.year = SQLDate->year;
			dateTmp.month = SQLDate->month;
			dateTmp.day = SQLDate->day;
			break;
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
			SQLTimestamp = &SQLTimestampTmp;
			if (ColumnwiseData) // !RowwiseRowSet
			{
				SQLTimestamp->year = 01;
				SQLTimestamp->month = 01;
				SQLTimestamp->day = 01;
				SQLTimestamp->hour = 0;
				SQLTimestamp->minute = 0;
				SQLTimestamp->second = 0;
				switch (SQLDatetimeCode)
				{
				case SQLDTCODE_TIME:
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 6);
					if (srcPrecision > 0)
						ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 9, srcPrecision);
					break;
				case SQLDTCODE_YEAR_TO_HOUR:
					SQLTimestamp->year = GetYearFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
					break;
				case SQLDTCODE_YEAR_TO_MINUTE:
					SQLTimestamp->year = GetYearFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 14);
					break;
				case SQLDTCODE_MONTH_TO_HOUR:
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
					break;
				case SQLDTCODE_MONTH_TO_MINUTE:
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 9);
					break;
				case SQLDTCODE_MONTH_TO_SECOND:
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 9);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 12);
					break;
				case SQLDTCODE_DAY_TO_HOUR:
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
					break;
				case SQLDTCODE_DAY_TO_MINUTE:
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 6);
					break;
				case SQLDTCODE_DAY_TO_SECOND:
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 6);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 9);
					break;
				case SQLDTCODE_MINUTE_TO_SECOND:
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 3);
					break;
				case SQLDTCODE_SECOND:
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr);
				break;
				default:
					SQLTimestamp->year = GetYearFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 14);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 17);
					break;
				}
			}
			else
			{
				SQLTimestamp = (TIMESTAMP_TYPES *)srcDataPtr;
			}
			if (SQLTimestamp->hour != 0 || SQLTimestamp->minute != 0 || SQLTimestamp->second != 0)
				retCode = IDS_01_S07;
			dateTmp.year = SQLTimestamp->year;
			dateTmp.month = SQLTimestamp->month;
			dateTmp.day = SQLTimestamp->day;
			break;
		default:
			return IDS_07_006;
		}
		break; // End of switch for SQL_C_DATE
	case SQL_C_TIME:
	case SQL_C_TYPE_TIME:
		DataPtr = &timeTmp;
		DataLen = sizeof(TIME_STRUCT);
		switch (ODBCDataType)
		{
		case SQL_CHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WCHAR:
		case SQL_WVARCHAR:
			{
				int TransStringLength = 0;
				char error[64];
				if (ODBCDataType == SQL_CHAR || ODBCDataType == SQL_WCHAR)
				{
					if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to UTF-8
					{
						UChar* spaceStart = u_strchr((UChar*)srcDataPtr, L' ');
						if (spaceStart != NULL)
							srcLength = (spaceStart - (UChar*)srcDataPtr) + 1;
						if(iconv->WCharToUTF8((UChar*)srcDataPtr, srcLength-1, (char*)cTmpBuf,
							sizeof(cTmpBuf), &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
						{
						//We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
							return IDS_22_003;
						}
						//srcLength = TransStringLength + 1;
						srcLength = strlen((char*)cTmpBuf) + 1;
					}
					else
					{
					//remove spaces if any
						char* spaceStart = strchr((char*)srcDataPtr, ' ');
						if (spaceStart != NULL)
							srcLength = (spaceStart - (char*)srcDataPtr) + 1;
						if (srcLength <= sizeof (cTmpBuf)) //Avoid a seg-violation
						{
							strncpy(cTmpBuf, (char*)srcDataPtr, srcLength - 1);
							cTmpBuf[srcLength - 1] = 0;
						}
						else
							return IDS_22_003;
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
							if(iconv->WCharToUTF8((UChar*)srcDataPtr+1, DataLen/2, (char*)cTmpBuf,
								(sizeof(cTmpBuf)) , &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
							{
							//We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
								return IDS_22_003;
							}
							srcLength = TransStringLength + 1;
						}
						else
						{
							if((DataLen + 1) <= sizeof(cTmpBuf))
							{
								memcpy(cTmpBuf, (char*)srcDataPtr+2, DataLen);
								srcLength = DataLen + 1;
								cTmpBuf[DataLen] = 0;
							}
							else
								return IDS_22_003;
						}
					}
					else
					{
						DataLen = *(UINT *)srcDataPtr;
						if (DataLen == 0)
						return SQL_NO_DATA;
						if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to UTF-8
						{
							if(iconv->WCharToUTF8((UChar*)srcDataPtr+2, DataLen/2, (char*)cTmpBuf,
								(sizeof(cTmpBuf)) , &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
							{
							//We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
								return IDS_22_003;
							}
							srcLength = TransStringLength + 1;
						}
						else
						{
							if((DataLen + 1) <= sizeof(cTmpBuf))
							{
								memcpy(cTmpBuf, (char*)srcDataPtr+4, DataLen);
								srcLength = DataLen + 1;
								cTmpBuf[DataLen] = 0;
							}
							else
								return IDS_22_003;
						}
					}
                    DataLen = sizeof(TIME_STRUCT);
				}
			}
			if ((retCode = ConvertSQLCharToDate(ODBCDataType, cTmpBuf, srcLength, SQL_C_TIME, 
					(SQLPOINTER)&timeTmp)) != 0)
				return retCode;
			break;
		case SQL_TIME:
		case SQL_TYPE_TIME:
			SQLTime = &SQLTimeTmp;
			if (ColumnwiseData) // !RowwiseRowSet
			{
				SQLTime->hour = 0;
				SQLTime->minute = 0;
				SQLTime->second = 0;
				switch (SQLDatetimeCode)
				{
				case SQLDTCODE_HOUR:
					SQLTime->hour = GetHourFromStr((UCHAR*)srcDataPtr);
					break;
				case SQLDTCODE_HOUR_TO_MINUTE:
					SQLTime->hour = GetHourFromStr((UCHAR*)srcDataPtr);
					SQLTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
					break;
				case SQLDTCODE_MINUTE:
					SQLTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
					break;
				case SQLDTCODE_MINUTE_TO_SECOND:
					SQLTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
					SQLTime->second = GetSecondFromStr((UCHAR*)srcDataPtr + 3);
					break;
				case SQLDTCODE_SECOND:
					SQLTime->second = GetSecondFromStr((UCHAR*)srcDataPtr);
					break;
				default:
					SQLTime->hour = GetHourFromStr((UCHAR*)srcDataPtr);
					SQLTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
					SQLTime->second = GetSecondFromStr((UCHAR*)srcDataPtr + 6);
				}
			}
			else
			{
				SQLTime = (TIME_TYPES *)srcDataPtr;
			}
			timeTmp.hour = SQLTime->hour;
			timeTmp.minute = SQLTime->minute;
			timeTmp.second = SQLTime->second;
			break;
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
			SQLTimestamp = &SQLTimestampTmp;
			if (ColumnwiseData) // !RowwiseRowSet
			{
				SQLTimestamp->year = 01;
				SQLTimestamp->month = 01;
				SQLTimestamp->day = 01;
				SQLTimestamp->hour = 0;
				SQLTimestamp->minute = 0;
				SQLTimestamp->second = 0;
				switch (SQLDatetimeCode)
				{
				case SQLDTCODE_YEAR_TO_HOUR:
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
					break;
				case SQLDTCODE_YEAR_TO_MINUTE:
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 14);
					break;
				case SQLDTCODE_MONTH_TO_HOUR:
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
					break;
				case SQLDTCODE_MONTH_TO_MINUTE:
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 9);
					break;
				case SQLDTCODE_MONTH_TO_SECOND:
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 9);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 12);
					break;
				case SQLDTCODE_DAY_TO_HOUR:
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
					break;
				case SQLDTCODE_DAY_TO_MINUTE:
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 6);
					break;
				case SQLDTCODE_DAY_TO_SECOND:
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 6);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 9);
					break;
				case SQLDTCODE_TIME:
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 6);
					break;
				case SQLDTCODE_MINUTE_TO_SECOND:
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 3);
					break;
				case SQLDTCODE_SECOND:
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr);
					break;
				default:
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 14);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 17);
					break;
				}
			}
			else
			{
				SQLTimestamp = (TIMESTAMP_TYPES *)srcDataPtr;
			}
			timeTmp.hour = SQLTimestamp->hour;
			timeTmp.minute = SQLTimestamp->minute;
			timeTmp.second = SQLTimestamp->second;
			break;
		default:
			return IDS_07_006;
		}
		break; // End of switch for SQL_C_TIME
	case SQL_C_TIMESTAMP:
	case SQL_C_TYPE_TIMESTAMP:
		DataPtr = &timestampTmp;
		DataLen = sizeof(TIMESTAMP_STRUCT);
		switch (ODBCDataType)
		{
		case SQL_CHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WCHAR:
		case SQL_WVARCHAR:
			{
				int TransStringLength = 0;
				char error[64];
				if (ODBCDataType == SQL_CHAR || ODBCDataType == SQL_WCHAR)
				{
					if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to UTF-8
					{
						if (sizeof(cTmpBuf) < srcLength/2) //Avoid a seg-violation
							return IDS_22_003;
						if(iconv->WCharToUTF8((UChar*)srcDataPtr, srcLength-1, (char*)cTmpBuf,
							sizeof(cTmpBuf), &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
						{
							// This should never happen!
							return IDS_22_003;
						}
						srcLength = strlen(cTmpBuf) + 1 ; //TransStringLength + 1;
					}
					else
					{
						if (srcLength <= sizeof (cTmpBuf)) //Avoid a seg-violation
						{
							strncpy(cTmpBuf, (char*)srcDataPtr, srcLength - 1);
							cTmpBuf[srcLength - 1] = 0;
						}
						else
							return IDS_22_003;
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
							if(iconv->WCharToUTF8((UChar*)srcDataPtr+1, DataLen/2, (char*)cTmpBuf,
								(sizeof(cTmpBuf)) , &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
							{
							//We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
								return IDS_22_003;
							}
							srcLength = TransStringLength + 1;
                            DataLen = sizeof(TIMESTAMP_STRUCT);
						}
						else
						{
							if((DataLen + 1) <= sizeof(cTmpBuf))
							{
								memcpy(cTmpBuf, (char*)srcDataPtr+2, DataLen);
								srcLength = DataLen + 1;
								cTmpBuf[DataLen] = 0;
							}
							else
								return IDS_22_003;
						}
					}
					else
					{
						DataLen = *(UINT *)srcDataPtr;
						if (DataLen == 0)
						return SQL_NO_DATA;
						if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to UTF-8
						{
							if(iconv->WCharToUTF8((UChar*)srcDataPtr+2, DataLen/2, (char*)cTmpBuf,
								(sizeof(cTmpBuf)) , &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
							{
							//We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
								return IDS_22_003;
							}
							srcLength = TransStringLength + 1;
                            DataLen = sizeof(TIMESTAMP_STRUCT);
						}
						else
						{
							if((DataLen + 1) <= sizeof(cTmpBuf))
							{
								memcpy(cTmpBuf, (char*)srcDataPtr+4, DataLen);
								srcLength = DataLen + 1;
								cTmpBuf[DataLen] = 0;
							}
							else
								return IDS_22_003;
						}
					}
                    DataLen = sizeof(TIMESTAMP_STRUCT);
				}
			}
			if ((retCode = ConvertSQLCharToDate(ODBCDataType, cTmpBuf, srcLength, SQL_C_TIMESTAMP, 
					(SQLPOINTER)&timestampTmp)) != 0)
				return retCode;
			break;
		case SQL_DATE:
		case SQL_TYPE_DATE:
			SQLDate = &SQLDateTmp;
			if (ColumnwiseData) // ! RowwiseRowSet
			{
				SQLDate->year = 01;
				SQLDate->month = 01;
				SQLDate->day = 01;
				switch (SQLDatetimeCode)
				{
				case SQLDTCODE_YEAR:
					SQLDate->year = GetYearFromStr((UCHAR *)srcDataPtr);
					break;
				case SQLDTCODE_YEAR_TO_MONTH:
					SQLDate->year = GetYearFromStr((UCHAR *)srcDataPtr);
					SQLDate->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
					break;
				case SQLDTCODE_MONTH:
					SQLDate->month = GetMonthFromStr((UCHAR*)srcDataPtr);
					break;
				case SQLDTCODE_MONTH_TO_DAY:
					SQLDate->month = GetMonthFromStr((UCHAR*)srcDataPtr);
					SQLDate->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
					break;
				case SQLDTCODE_DAY:
					SQLDate->day = GetDayFromStr((UCHAR *)srcDataPtr);
					break;
				default:
					SQLDate->year = GetYearFromStr((UCHAR *)srcDataPtr);
					SQLDate->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
					SQLDate->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
					break;
				}
			}
			else
			{
				SQLDate = (DATE_TYPES *)srcDataPtr;
			}
			timestampTmp.year = SQLDate->year;
			timestampTmp.month = SQLDate->month;
			timestampTmp.day = SQLDate->day;
			timestampTmp.hour = 0;
			timestampTmp.minute = 0;
			timestampTmp.second = 0;
			timestampTmp.fraction = 0;
			break;
		case SQL_TIME:
		case SQL_TYPE_TIME:
			struct tm *newtime;
			time_t long_time;
			SQLTime = &SQLTimeTmp;
			if (ColumnwiseData) // !RowwiseRowSet
			{
				SQLTime->hour = 0;
				SQLTime->minute = 0;
				SQLTime->second = 0;
				switch (SQLDatetimeCode)
				{
				case SQLDTCODE_HOUR:
					SQLTime->hour = GetHourFromStr((UCHAR*)srcDataPtr);
					break;
				case SQLDTCODE_HOUR_TO_MINUTE:
					SQLTime->hour = GetHourFromStr((UCHAR*)srcDataPtr);
					SQLTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
					break;
				case SQLDTCODE_MINUTE:
					SQLTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
					break;
				case SQLDTCODE_MINUTE_TO_SECOND:
					SQLTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
					SQLTime->second = GetSecondFromStr((UCHAR*)srcDataPtr + 3);
					break;
				case SQLDTCODE_SECOND:
					SQLTime->second = GetSecondFromStr((UCHAR*)srcDataPtr);
					break;
				default:
					SQLTime->hour = GetHourFromStr((UCHAR*)srcDataPtr);
					SQLTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
					SQLTime->second = GetSecondFromStr((UCHAR*)srcDataPtr + 6);
				}
			}
			else
			{
				SQLTime = (TIME_TYPES *)srcDataPtr;
			}
			time( &long_time );					/* Get time as long integer. */
			newtime = localtime( &long_time );	/* Convert to local time. */
			timestampTmp.year = (short)(newtime->tm_year+1900);
			timestampTmp.month =(unsigned short)(newtime->tm_mon+1);
			timestampTmp.day = (unsigned short)newtime->tm_mday;
			timestampTmp.hour = SQLTime->hour;
			timestampTmp.minute = SQLTime->minute;
			timestampTmp.second = SQLTime->second;
			timestampTmp.fraction = 0;
			break;
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
			SQLTimestamp = &SQLTimestampTmp;
			if (ColumnwiseData)  // !RowwiseRowSet
			{
				SQLTimestamp->year = 01;
				SQLTimestamp->month = 01;
				SQLTimestamp->day = 01;
				SQLTimestamp->hour = 0;
				SQLTimestamp->minute = 0;
				SQLTimestamp->second = 0;
				ulFraction = 0;
				switch (SQLDatetimeCode)
				{
				case SQLDTCODE_TIME:
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 6);
					if (srcPrecision > 0)
						ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 9, srcPrecision);
					break;
				case SQLDTCODE_YEAR_TO_HOUR:
					SQLTimestamp->year = GetYearFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
					break;
				case SQLDTCODE_YEAR_TO_MINUTE:
					SQLTimestamp->year = GetYearFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 14);
					break;
				case SQLDTCODE_MONTH_TO_HOUR:
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);;
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);;
					break;
				case SQLDTCODE_MONTH_TO_MINUTE:
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 9);
					break;
				case SQLDTCODE_MONTH_TO_SECOND:
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 9);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 12);
					if (srcPrecision > 0)
						ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 15, srcPrecision);
					break;
				case SQLDTCODE_DAY_TO_HOUR:
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
					break;
				case SQLDTCODE_DAY_TO_MINUTE:
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 6);
					break;
				case SQLDTCODE_DAY_TO_SECOND:
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 6);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 9);
					if (srcPrecision > 0)
						ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 12, srcPrecision);
					break;
				case SQLDTCODE_MINUTE_TO_SECOND:
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 3);
					if (srcPrecision > 0)
						ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 6, srcPrecision);
					break;
				case SQLDTCODE_SECOND:
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr);
					if (srcPrecision > 0)
						ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 3, srcPrecision);
					break;
				default:
					SQLTimestamp->year = GetYearFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 14);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 17);
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
				SQLTimestamp = (TIMESTAMP_TYPES *)srcDataPtr;
				if (srcPrecision > 0)
				{
					// SQL returns fraction of a second which has to be converted to nano seconds
					dTmp = (*(UDWORD_P*)SQLTimestamp->fraction *  1000000000.0) / pow(10,srcPrecision);
					ulFraction = dTmp;

				}
				else
					ulFraction = 0; 
			}
			timestampTmp.year = SQLTimestamp->year;
			timestampTmp.month = SQLTimestamp->month;
			timestampTmp.day = SQLTimestamp->day;
			timestampTmp.hour = SQLTimestamp->hour;
			timestampTmp.minute = SQLTimestamp->minute;
			timestampTmp.second = SQLTimestamp->second;
			if (srcPrecision > 0)
			{
				timestampTmp.fraction = ulFraction;
			}
			else
				timestampTmp.fraction = 0;
			break;
		default:
			return IDS_07_006;
		}
		break; // End of switch for SQL_C_TIMESTAMP

	case SQL_C_BINARY:
		switch (ODBCDataType)
		{
		case SQL_CHAR:
		case SQL_WCHAR:
			if (srcLength-1 == 0)
			{
				if (targetStrLenPtr != NULL)
					*targetStrLenPtr = 0;
				((char*)targetDataPtr)[0] = '\0';
				return retCode;
			}
			DataLen = srcLength -1 - Offset;
			if (DataLen == 0)
				return SQL_NO_DATA;
			if (DataLen >= targetLength)
			{
				retCode = IDS_01_004;
				DataLenTruncated = srcLength - Offset-1;
				DataLen = targetLength-1;
			}
			DataPtr = (char *)srcDataPtr + Offset;
//			if (totalReturnedLength != NULL)
//				*totalReturnedLength = DataLen + Offset;
			break;
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WVARCHAR:
			if(isshort){
				short_len=*(USHORT *)srcDataPtr;
				charlength=short_len;
				if (short_len == 0){	
					if (targetStrLenPtr != NULL)
						*targetStrLenPtr = 0;
					((char*)targetDataPtr)[0] = '\0';
					return retCode;
				}
				DataLen = short_len - Offset;
			}
			else{
				int_len=*(int *)srcDataPtr;
				charlength=int_len;
				if(int_len == 0){
					if (targetStrLenPtr != NULL)
						*targetStrLenPtr = 0;
					((char*)targetDataPtr)[0] = '\0';
					return retCode;
				}
				DataLen = int_len - Offset;
			}

			if (DataLen == 0)
				return SQL_NO_DATA;
			if (DataLen >= targetLength)
			{
				retCode = IDS_01_004;
				if(isshort)
					DataLenTruncated = short_len - Offset;
				else
					DataLenTruncated = int_len - Offset;
				DataLen = targetLength-1;
			}
			if(isshort)
				DataPtr = (char *)srcDataPtr + 2 + Offset;
			else
				DataPtr = (char *)srcDataPtr + 4 + Offset;
//			if (totalReturnedLength != NULL)
//				*totalReturnedLength = DataLen + Offset;
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
			DataPtr = (char *)srcDataPtr + Offset;
			DataLen = strlen((char*)DataPtr);
			if (DataLen >= targetLength)
			{
				retCode = IDS_01_004;
				DataLenTruncated = srcLength-Offset;
				if (targetLength > 0)
					DataLen = targetLength-1;
				else
					DataLen = 0;
			}
//			if (totalReturnedLength != NULL)
//				*totalReturnedLength = DataLen + Offset;
			break;
        case SQL_TINYINT:
		case SQL_SMALLINT:
		case SQL_INTEGER:
		case SQL_BIGINT:
		case SQL_REAL:
		case SQL_FLOAT:
		case SQL_NUMERIC:
		case SQL_DECIMAL:
		case SQL_DOUBLE:
		case SQL_DATE:
		case SQL_TYPE_DATE:
		case SQL_TIME:
		case SQL_TYPE_TIME:
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
			DataPtr = srcDataPtr;
			DataLen = srcLength;
			if (DataLen > targetLength)
				return IDS_22_003;
			break;
		default:
			return IDS_07_006;
		}
		NullTerminate = TRUE;
		break; // End of switch for SQL_C_BINARY 
	case SQL_C_DEFAULT:
		switch (ODBCDataType)
		{
		case SQL_CHAR:
            if(SQLDataType == SQLTYPECODE_BOOLEAN)
            {
                tTmp = *((SCHAR *) srcDataPtr);
                DataPtr = &tTmp;
                DataLen = sizeof(SCHAR);
                break;
            }
		case SQL_WCHAR:
			charlength = srcLength-1;
			if (charlength == 0)
			{
				if (targetStrLenPtr != NULL)
					*targetStrLenPtr = 0;
				((char*)targetDataPtr)[0] = '\0';
				return retCode;
			}
			DataLen = charlength - Offset;
			if (DataLen == 0)
				return SQL_NO_DATA;
			if (DataLen >= targetLength)
			{
				retCode = IDS_01_004;
				DataLenTruncated = charlength - Offset;
				DataLen = targetLength-1;
			}
			DataPtr = (char *)srcDataPtr + Offset;
//			if (totalReturnedLength != NULL)
//				*totalReturnedLength = DataLen + Offset;
			NullTerminate = TRUE;
			break;
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WVARCHAR:
			if(isshort){
				short_len=*(USHORT *)srcDataPtr;
				charlength=short_len;
				if (short_len == 0)
				{	
					if (targetStrLenPtr != NULL)
						*targetStrLenPtr = 0;
					((char*)targetDataPtr)[0] = '\0';
					return retCode;
				}
				DataLen = short_len - Offset;
			}
			else{
				int_len=*(int *)srcDataPtr;
				charlength=int_len;
				if (int_len == 0)
				{
					if (targetStrLenPtr != NULL)
						*targetStrLenPtr = 0;
					((char*)targetDataPtr)[0] = '\0';
					return retCode;
				}
				DataLen = int_len - Offset;
			}
			if (DataLen == 0)
				return SQL_NO_DATA;
			if (DataLen >= targetLength)
			{
				retCode = IDS_01_004;
				if(isshort)
					DataLenTruncated = short_len - Offset;
				else
					DataLenTruncated = int_len - Offset;
				DataLen = targetLength-1;
			}
			if(isshort)
				DataPtr = (char *)srcDataPtr + 2 + Offset;
			else
				DataPtr = (char *)srcDataPtr + 4 + Offset;
//			if (totalReturnedLength != NULL)
//				*totalReturnedLength = DataLen + Offset;
			NullTerminate = TRUE;
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
			if ((retCode = ConvertSQLCharToInterval(ODBCDataType, srcDataPtr, srcLength, CDataType, 
					(SQLPOINTER)&intervalTmp)) != 0)
				if (retCode != IDS_01_S07)
					return retCode;
			DataPtr = &intervalTmp;
			DataLen = sizeof(SQL_INTERVAL_STRUCT);
			break;
        case SQL_TINYINT:
            tTmp = *((SCHAR *) srcDataPtr);
            DataPtr = &tTmp;
            DataLen = sizeof(SCHAR);
            break;
		case SQL_SMALLINT:
			if (srcUnsigned)
			{
				usTmp = *((USHORT *)srcDataPtr);
				DataPtr = &usTmp;
				DataLen = sizeof(USHORT);
			}
			else
			{
				sTmp = *(SHORT *)srcDataPtr;
				DataPtr = &sTmp;
				DataLen = sizeof(SSHORT);
			}
			break;
		case SQL_INTEGER:
			if (srcUnsigned)
			{
				ulTmp = *(ULONG_P *)srcDataPtr;
				DataPtr = &ulTmp;
				DataLen = sizeof(ULONG_P);
			}
			else
			{
				lTmp = *(SLONG_P *)srcDataPtr;
				DataPtr = &lTmp;
				DataLen = sizeof(SLONG_P);
			}
			break;
		case SQL_BIGINT:
		case SQL_NUMERIC:
			if ((ConvertNumericToChar(SQLDataType, srcDataPtr, srcScale, cTmpBuf, DecimalPoint)) != SQL_SUCCESS)
				return IDS_07_006;
			DataLen = strlen(cTmpBuf);
			if (DecimalPoint > targetLength)
				return IDS_22_003;
			if (DataLen > targetLength)
			{
				DataLen = targetLength-1;
				retCode = IDS_01_004;
			}
			DataPtr = cTmpBuf;
			NullTerminate = TRUE;
			break;
		case SQL_DECIMAL:
			if (ConvertDecimalToChar(SQLDataType, srcDataPtr, srcLength, srcScale, 
						cTmpBuf, DecimalPoint) != SQL_SUCCESS)
				return IDS_07_006;
			DataLen = strlen(cTmpBuf);
			if (DecimalPoint > targetLength)
				return IDS_22_003;
			if (DataLen > targetLength)
			{
				DataLen = targetLength-1;
				retCode = IDS_01_004;
			}
			DataPtr = cTmpBuf;
			NullTerminate = TRUE;
			break;
		case SQL_REAL:
			fltTmp = *(SFLOAT *)srcDataPtr;
			DataPtr = &fltTmp;
			DataLen = sizeof(SFLOAT);
			break;
		case SQL_DOUBLE:
			if ((SQLDataType == SQLTYPECODE_DECIMAL_LARGE_UNSIGNED) ||
				(SQLDataType == SQLTYPECODE_DECIMAL_LARGE))
			{
				if (ConvertSoftDecimalToDouble(SQLDataType, srcDataPtr, srcLength, srcScale, 
							dTmp)  != SQL_SUCCESS)
					return IDS_07_006;
			}
			else
				dTmp = *((SDOUBLE FAR *) srcDataPtr);
			DataPtr = &dTmp;
			DataLen = sizeof(DOUBLE);
			break;
		case SQL_DATE:
		case SQL_TYPE_DATE:
			SQLDate = &SQLDateTmp;
			if (ColumnwiseData) // !RowwiseRowSet
			{
				SQLDate->year = 01;
				SQLDate->month = 01;
				SQLDate->day = 01;
				switch (SQLDatetimeCode)
				{
				case SQLDTCODE_YEAR:
					SQLDate->year = GetYearFromStr((UCHAR *)srcDataPtr);
					break;
				case SQLDTCODE_YEAR_TO_MONTH:
					SQLDate->year = GetYearFromStr((UCHAR *)srcDataPtr);
					SQLDate->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
					break;
				case SQLDTCODE_MONTH:
					SQLDate->month = GetMonthFromStr((UCHAR*)srcDataPtr);
					break;
				case SQLDTCODE_MONTH_TO_DAY:
					SQLDate->month = GetMonthFromStr((UCHAR*)srcDataPtr);
					SQLDate->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
					break;
				case SQLDTCODE_DAY:
					SQLDate->day = GetDayFromStr((UCHAR*)srcDataPtr);
					break;
				default:
					SQLDate->year = GetYearFromStr((UCHAR *)srcDataPtr);
					SQLDate->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
					SQLDate->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
					break;
				}
			}
			else
			{
				SQLDate = (DATE_TYPES *)srcDataPtr;
			}
			dateTmp.year = SQLDate->year;
			dateTmp.month = SQLDate->month;
			dateTmp.day = SQLDate->day;
			DataPtr = &dateTmp;
			DataLen = sizeof(DATE_STRUCT);
			break;
		case SQL_TIME:
		case SQL_TYPE_TIME:
			SQLTime = &SQLTimeTmp;
			if (ColumnwiseData) // !RowwiseRowSet
			{
				SQLTime->hour = 0;
				SQLTime->minute = 0;
				SQLTime->second = 0;
				switch (SQLDatetimeCode)
				{
				case SQLDTCODE_HOUR:
					SQLTime->hour = GetHourFromStr((UCHAR*)srcDataPtr);
					break;
				case SQLDTCODE_HOUR_TO_MINUTE:
					SQLTime->hour = GetHourFromStr((UCHAR*)srcDataPtr);
					SQLTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
					break;
				case SQLDTCODE_MINUTE:
					SQLTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
					break;
				case SQLDTCODE_MINUTE_TO_SECOND:
					SQLTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
					SQLTime->second = GetSecondFromStr((UCHAR*)srcDataPtr + 3);
					break;
				case SQLDTCODE_SECOND:
					SQLTime->second = *((UCHAR*)srcDataPtr);
					break;
				default:
					SQLTime->hour = GetHourFromStr((UCHAR*)srcDataPtr);
					SQLTime->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
					SQLTime->second = GetSecondFromStr((UCHAR*)srcDataPtr + 6);
				}
			}
			else
			{
				SQLTime = (TIME_TYPES *)srcDataPtr;
			}
			timeTmp.hour = SQLTime->hour;
			timeTmp.minute = SQLTime->minute;
			timeTmp.second = SQLTime->second;
			DataPtr = &timeTmp;
			DataLen = sizeof(TIME_STRUCT);
			break;
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
			SQLTimestamp = &SQLTimestampTmp;
			if (ColumnwiseData) // !RowwiseRowSet
			{
				SQLTimestamp->year = 01;
				SQLTimestamp->month = 01;
				SQLTimestamp->day = 01;
				SQLTimestamp->hour = 0;
				SQLTimestamp->minute = 0;
				SQLTimestamp->second = 0;
				ulFraction = 0;
				switch (SQLDatetimeCode)
				{
				case SQLDTCODE_TIME:
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 6);
					if (srcPrecision > 0)
						ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 9, srcPrecision);
					break;
				case SQLDTCODE_YEAR_TO_HOUR:
					SQLTimestamp->year = GetYearFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
					break;
				case SQLDTCODE_YEAR_TO_MINUTE:
					SQLTimestamp->year = GetYearFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 14);
					break;
				case SQLDTCODE_MONTH_TO_HOUR:
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
					break;
				case SQLDTCODE_MONTH_TO_MINUTE:
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 9);
					break;
				case SQLDTCODE_MONTH_TO_SECOND:
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 6);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 9);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 12);
					if (srcPrecision > 0)
						ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 15, srcPrecision);
					break;
				case SQLDTCODE_DAY_TO_HOUR:
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
					break;
				case SQLDTCODE_DAY_TO_MINUTE:
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 6);
					break;
				case SQLDTCODE_DAY_TO_SECOND:
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 3);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 6);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 9);
					if (srcPrecision > 0)
						ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 12, srcPrecision);
					break;
				case SQLDTCODE_MINUTE_TO_SECOND:
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 3);
					if (srcPrecision > 0)
						ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 6, srcPrecision);
					break;
				case SQLDTCODE_SECOND:
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr);
					if (srcPrecision > 0)
						ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 3, srcPrecision);
					break;
				default:
					SQLTimestamp->year = GetYearFromStr((UCHAR*)srcDataPtr);
					SQLTimestamp->month = GetMonthFromStr((UCHAR*)srcDataPtr + 5);
					SQLTimestamp->day = GetDayFromStr((UCHAR*)srcDataPtr + 8);
					SQLTimestamp->hour = GetHourFromStr((UCHAR*)srcDataPtr + 11);
					SQLTimestamp->minute = GetMinuteFromStr((UCHAR*)srcDataPtr + 14);
					SQLTimestamp->second = GetSecondFromStr((UCHAR*)srcDataPtr + 17);
					if (srcPrecision > 0)
						ulFraction = GetFractionFromStr((UCHAR*)srcDataPtr + 20, srcPrecision);
					break;
				}
			}
			else
			{
				SQLTimestamp = (TIMESTAMP_TYPES *)srcDataPtr;
				if (srcPrecision > 0)
				{
					// SQL returns fraction of a second which has to be converted to nano seconds
					dTmp = (*(UDWORD_P*)SQLTimestamp->fraction *  1000000000.0) / pow(10,srcPrecision);
					ulFraction = dTmp;

				}
				else
					ulFraction = 0;
			}
			timestampTmp.year = SQLTimestamp->year;
			timestampTmp.month = SQLTimestamp->month;
			timestampTmp.day = SQLTimestamp->day;
			timestampTmp.hour = SQLTimestamp->hour;
			timestampTmp.minute = SQLTimestamp->minute;
			timestampTmp.second = SQLTimestamp->second;
			timestampTmp.fraction = ulFraction;

			DataPtr = &timestampTmp;
			DataLen = sizeof(TIMESTAMP_STRUCT);
			break;
		default:
			return IDS_07_006;
		}
		break;  // End of switch for SQL_C_DEFAULT

	case SQL_C_NUMERIC:

		memset(cTmpBuf,0,sizeof(cTmpBuf));

		switch (ODBCDataType)
		{
		case SQL_CHAR:
		case SQL_WCHAR:
			DataLen = srcLength - 1;
			if (DataLen == 0)
				return SQL_NO_DATA;
			char error[64];
			if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to UTF-8
			{
				int TransStringLength = 0;
				UChar* spaceStart = u_strchr((UChar*)srcDataPtr, L' ');
				if (spaceStart != NULL)
					srcLength = (spaceStart - (UChar*)srcDataPtr) + 1;
				if(iconv->WCharToUTF8((UChar*)srcDataPtr, srcLength-1, (char*)cTmpBuf,
					sizeof(cTmpBuf), &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
				{
				//	((CHandle*)ConnectionHandle)->setWcharConvError(error); 
					return IDS_07_003;
				}
				srcLength = strlen((char*)cTmpBuf) + 1;
			}
			else
			{
			//remove spaces if any
				char* spaceStart = strchr((char*)srcDataPtr, ' ');
				if (spaceStart != NULL)
					srcLength = (spaceStart - (char*)srcDataPtr) + 1;
				if (srcLength <= sizeof (cTmpBuf)) //Avoid a seg-violation
				{
					strncpy(cTmpBuf, (char*)srcDataPtr, srcLength - 1);
					cTmpBuf[srcLength - 1] = 0;
				}
				else
					return IDS_07_003;
			}
			useDouble = FALSE;
			break;
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WVARCHAR:
			if(isshort)
			{
				DataLen = *(USHORT *)srcDataPtr;
				if (DataLen == 0)
				return SQL_NO_DATA;
				if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to UTF-8
				{
					int TransStringLength = 0;
					if(iconv->WCharToUTF8((UChar*)srcDataPtr+1, DataLen/2, (char*)cTmpBuf,
						(sizeof(cTmpBuf)) , &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
					{
					//We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
						return IDS_07_003;
					}
					srcLength = TransStringLength + 1;
				}
				else
				{
					if((DataLen + 1) <= sizeof(cTmpBuf))
					{
						memcpy(cTmpBuf, (char*)srcDataPtr+2, DataLen);
						srcLength = DataLen + 1;
						cTmpBuf[DataLen] = 0;
					}
					else
						return IDS_07_003;
				}
			}
			else
			{
				DataLen = *(UINT *)srcDataPtr;
				if (DataLen == 0)
				return SQL_NO_DATA;
				if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to UTF-8
				{

					int TransStringLength = 0;
					if(iconv->WCharToUTF8((UChar*)srcDataPtr+2, DataLen/2, (char*)cTmpBuf,
						(sizeof(cTmpBuf)) , &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
					{
					//We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
						return IDS_07_003;
					}
					srcLength = TransStringLength + 1;
				}
				else
				{
					if((DataLen + 1) <= sizeof(cTmpBuf))
					{
						memcpy(cTmpBuf, (char*)srcDataPtr+4, DataLen);
						srcLength = DataLen + 1;
						cTmpBuf[DataLen] = 0;
					}
					else
						return IDS_07_003;
				}
			}
			useDouble = FALSE;
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
			tempPtr = trimInterval((char*)srcDataPtr);
			DataLen = strlen(tempPtr);
			if (sizeof(cTmpBuf) < DataLen )
				return IDS_07_003;
			strncpy( cTmpBuf, tempPtr, DataLen );
			useDouble = FALSE;
			break;
        case SQL_TINYINT:
            if(srcUnsigned)
            {
                utTmp = *((UCHAR *) srcDataPtr);
                _ultoa(utTmp,cTmpBuf,10);
            }
            else
            {
                tTmp = *((SCHAR *) srcDataPtr);
                _ltoa(tTmp,cTmpBuf,10);
            }
            useDouble = FALSE;
            break;
		case SQL_SMALLINT:
			if (srcUnsigned)
			{
				ulTmp = *(USHORT *)srcDataPtr;
				_ultoa( ulTmp, cTmpBuf, 10 );
			}
			else
			{
				lTmp = *(SSHORT *)srcDataPtr;
				_ltoa( lTmp, cTmpBuf, 10 );
			}
			useDouble = FALSE;
			break;
		case SQL_INTEGER:
			if (srcUnsigned)
			{
				ulTmp = *(ULONG_P *)srcDataPtr;
				_ultoa( ulTmp, cTmpBuf, 10 );
			}
			else
			{
				lTmp = *(SLONG_P *)srcDataPtr;
				_ltoa( lTmp, cTmpBuf, 10 );
			}
			useDouble = FALSE;
			break;
		case SQL_REAL:
			dTmp = *(SFLOAT *)srcDataPtr;  
			break;
		case SQL_DOUBLE:
			if ((SQLDataType == SQLTYPECODE_DECIMAL_LARGE_UNSIGNED) ||
				(SQLDataType == SQLTYPECODE_DECIMAL_LARGE))
			{
				int j;
				cTmpBuf[0] = ((unsigned char* )srcDataPtr)[0];
			// Make it as a display string
				for (j = 1, i = 1; i < srcLength ; i++, j++)
				{
					if( srcLength - i == srcScale )
						cTmpBuf[j] = '.';
					cTmpBuf[j] = '0' + ((unsigned char* )srcDataPtr)[i];
				}
				cTmpBuf[j] =  '\0';
				useDouble = FALSE;
			}
			else
				dTmp =  *(SDOUBLE *)srcDataPtr;
			break;
		case SQL_BIGINT:
			tempVal64 = *((__int64 *)srcDataPtr);
			if (tempVal64 < -DBL_MAX || tempVal64 > DBL_MAX)
				return IDS_22_003;
			_i64toa( tempVal64, cTmpBuf, 10);
			useDouble = FALSE;
			break;
		case SQL_NUMERIC:
			switch (SQLDataType)
			{
			case SQLTYPECODE_SMALLINT:
				tempVal64 = *(SHORT *)srcDataPtr;

				if (srcScale > 0)
				{
					for (i = 0, power = 1 ; i < srcScale ; power *= 10, i++);   
					tempValFrac = tempVal64 % power;
	    			tempVal64 = tempVal64 / power;
					if (tempVal64 == 0 && tempVal64 < 0)
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
						sprintf(cTmpBuf, "-%Ld.%0*Ld", tempVal64, srcScale, -tempValFrac);
#else
						sprintf(cTmpBuf, "-%lld.%0*lld", tempVal64, srcScale, -tempValFrac);
#endif
					else
					{
						if (tempValFrac < 0)
							tempValFrac = -tempValFrac;
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
						sprintf(cTmpBuf, "%Ld.%0*Ld", tempVal64, srcScale, tempValFrac);
#else
						sprintf(cTmpBuf, "%lld.%0*lld", tempVal64, srcScale, tempValFrac);
#endif
					}
				}
				else
					_i64toa( tempVal64, cTmpBuf, 10);
				break;
			case SQLTYPECODE_SMALLINT_UNSIGNED:
				utempVal64 = *(USHORT *)srcDataPtr;

				if (srcScale > 0)
				{
					for (i = 0, power = 1 ; i < srcScale ; power *= 10, i++);   
					utempValFrac = utempVal64 % power;
	    			utempVal64 = utempVal64 / power;
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
					sprintf(cTmpBuf, "%Ld.%0*%Ld", utempVal64, srcScale, utempValFrac);
#else
					sprintf(cTmpBuf, "%lld.%0*%lld", utempVal64, srcScale, utempValFrac);
#endif
				}
				else
					_ui64toa( utempVal64, cTmpBuf, 10);
				break;
			case SQLTYPECODE_INTEGER:
				tempVal64 = *(SLONG_P *)srcDataPtr;

				if (srcScale > 0)
				{
					for (i = 0, power = 1 ; i < srcScale ; power *= 10, i++);   
					tempValFrac = tempVal64 % power;
	    			tempVal64 = tempVal64 / power;
					if (tempVal64 == 0 && tempVal64 < 0)
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
						sprintf(cTmpBuf, "-%Ld.%0*Ld", tempVal64, srcScale, -tempValFrac);
#else
						sprintf(cTmpBuf, "-%lld.%0*lld", tempVal64, srcScale, -tempValFrac);
#endif
					else
					{
						if (tempValFrac < 0)
							tempValFrac = -tempValFrac;
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
						sprintf(cTmpBuf, "%Ld.%0*Ld", tempVal64, srcScale, tempValFrac);
#else
						sprintf(cTmpBuf, "%lld.%0*lld", tempVal64, srcScale, tempValFrac);
#endif
					}
				}
				else
					_i64toa( tempVal64, cTmpBuf, 10);
				break;
			case SQLTYPECODE_INTEGER_UNSIGNED:
				utempVal64 = *(ULONG_P *)srcDataPtr;

				if (srcScale > 0)
				{
					for (i = 0, power = 1 ; i < srcScale ; power *= 10, i++);   
					utempValFrac = utempVal64 % power;
	    			utempVal64 = utempVal64 / power;
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
					sprintf(cTmpBuf, "%Ld.%0*Ld", utempVal64, srcScale, utempValFrac);
#else
					sprintf(cTmpBuf, "%lld.%0*lld", utempVal64, srcScale, utempValFrac);
#endif
				}
				else
					_ui64toa( utempVal64, cTmpBuf, 10);
				break;
			case SQLTYPECODE_LARGEINT:
				tempVal64 = *((__int64 *)srcDataPtr);

				if (srcScale > 0)
				{
					for (i = 0, power = 1 ; i < srcScale ; power *= 10, i++);   
					tempValFrac = tempVal64 % power;
	    			tempVal64 = tempVal64 / power;
					if (tempVal64 == 0 && tempVal64 < 0)
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
						sprintf(cTmpBuf, "-%Ld.%0*Ld", tempVal64, srcScale, -tempValFrac);
#else
						sprintf(cTmpBuf, "-%lld.%0*lld", tempVal64, srcScale, -tempValFrac);
#endif
					else
					{
						if (tempValFrac < 0)
							tempValFrac = -tempValFrac;
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
						sprintf(cTmpBuf, "%Ld.%0*Ld", tempVal64, srcScale, tempValFrac);
#else
						sprintf(cTmpBuf, "%lld.%0*lld", tempVal64, srcScale, tempValFrac);
#endif
					}
				}
				else
					_i64toa( tempVal64, cTmpBuf, 10);
				break;
			default:
				return IDS_HY_000;
			}
			useDouble = FALSE;
			break;
		case SQL_DECIMAL: 
			if (ConvertDecimalToChar(SQLDataType, srcDataPtr, srcLength, srcScale, 
						cTmpBuf, DecimalPoint) != SQL_SUCCESS)
				return IDS_07_006;
			useDouble = FALSE;
			break;
		default:
			return IDS_07_006;
		}

		if(useDouble)
		{
			tempPtr = fcvt( dTmp, srcScale, (int*)&DecimalPoint, (int*)&Sign);
			if(strlen(tempPtr) > sizeof(cTmpBuf) - 3 || DecimalPoint + srcScale > ENDIAN_PRECISION_MAX)
				return IDS_22_003;
			memset(cTmpBuf, 0, sizeof(cTmpBuf));
			stopStr = cTmpBuf;
			*stopStr++ = (Sign == 0)?'+':'-';
			strncpy(stopStr, tempPtr, strlen(tempPtr) - srcScale);
			strcat(stopStr,".");
			strcat(stopStr, tempPtr + strlen(tempPtr) - srcScale);

		}

		if ((retCode = ConvertCharToCNumeric( numericTmp, cTmpBuf)) != SQL_SUCCESS)
			return retCode;

		DataPtr = &numericTmp;
		DataLen = sizeof(SQL_NUMERIC_STRUCT);

		break; // end of SQL_C_NUMERIC
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
		intervalTmp.interval_sign = 0;
		switch (ODBCDataType)
		{
		case SQL_CHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WCHAR:
		case SQL_WVARCHAR:
			{
				int TransStringLength = 0;
				char error[64];
				if (ODBCDataType == SQL_CHAR || ODBCDataType == SQL_WCHAR)
				{
					if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to locale
					{
						if (sizeof (cTmpBuf) <= srcLength/2) //Avoid a seg-violation
							return IDS_07_003;
						if(iconv->WCharToUTF8((UChar*)srcDataPtr, srcLength-1, (char*)cTmpBuf,
							sizeof(cTmpBuf), &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
						{
						//	((CHandle*)ConnectionHandle)->setWcharConvError(error); 
							return IDS_07_003;
						}
						srcLength = strlen(cTmpBuf) + 1;//TransStringLength + 1;
					}
					else
					{
						if (srcLength <= sizeof (cTmpBuf)) //Avoid a seg-violation
						{
							strncpy(cTmpBuf, (char*)srcDataPtr, srcLength - 1);
							cTmpBuf[srcLength - 1] = 0;
						}
						else
							return IDS_07_003;
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
							if(iconv->WCharToUTF8((UChar*)srcDataPtr+1, DataLen/2, (char*)cTmpBuf,
								(sizeof(cTmpBuf)) , &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
							{
							//We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
								return IDS_07_003;
							}
							srcLength = TransStringLength + 1;
						}
						else
						{
							if((DataLen + 1) <= sizeof(cTmpBuf))
							{
								memcpy(cTmpBuf, (char*)srcDataPtr+2, DataLen);
								srcLength = DataLen + 1;
								cTmpBuf[DataLen] = 0;
							}
							else
								return IDS_07_003;
						}
					}
					else
					{
						DataLen = *(UINT *)srcDataPtr;
						if (DataLen == 0)
						return SQL_NO_DATA;
						if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to UTF-8
						{
							if(iconv->WCharToUTF8((UChar*)srcDataPtr+2, DataLen/2, (char*)cTmpBuf,
								(sizeof(cTmpBuf)) , &TransStringLength, (char*)errorMsg) != SQL_SUCCESS)
							{
							//We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
								return IDS_07_003;
							}
							srcLength = TransStringLength + 1;
						}
						else
						{
							if((DataLen + 1) <= sizeof(cTmpBuf))
							{
								memcpy(cTmpBuf, (char*)srcDataPtr+4, DataLen);
								srcLength = DataLen + 1;
								cTmpBuf[DataLen] = 0;
							}
							else
								return IDS_07_003;
						}
					}
				}
				if ((retCode = ConvertSQLCharToInterval(ODBCDataType, cTmpBuf, srcLength, CDataType, 
					(SQLPOINTER)&intervalTmp)) != 0)
				if (retCode != IDS_01_S07)
					return retCode;
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
			if ((retCode = ConvertSQLCharToInterval(ODBCDataType, srcDataPtr, srcLength, CDataType, 
					(SQLPOINTER)&intervalTmp)) != 0)
				if (retCode != IDS_01_S07)
					return retCode;
			break;
		case SQL_SMALLINT:
			if (srcUnsigned)
				dTmp = *(USHORT *)srcDataPtr;
			else
				dTmp = *(SSHORT *)srcDataPtr;
			break;
		case SQL_INTEGER:
			if (srcUnsigned)
				dTmp = *(ULONG_P *)srcDataPtr;
			else
				dTmp = *(SLONG_P *)srcDataPtr;
			break;
		case SQL_BIGINT:
			tempVal64 = *((__int64 *)srcDataPtr);
			if (tempVal64 < -DBL_MAX || tempVal64 > DBL_MAX)
				return IDS_22_003;
			dTmp = tempVal64;
			break;
		case SQL_NUMERIC:
			switch (SQLDataType)
			{
			case SQLTYPECODE_SMALLINT:
				dTmp = *((SHORT *)srcDataPtr);
				if (srcScale > 0)
					dTmp = dTmp / (long)pow(10,srcScale);
				break;
			case SQLTYPECODE_SMALLINT_UNSIGNED:
				dTmp = *((USHORT *)srcDataPtr);
				if (srcScale > 0)
					dTmp = dTmp / (long)pow(10,srcScale);
				break;
			case SQLTYPECODE_INTEGER:
				dTmp = *((SLONG_P *)srcDataPtr);
				if (srcScale > 0)
					dTmp = dTmp / (long)pow(10,srcScale);
				break;
			case SQLTYPECODE_INTEGER_UNSIGNED:
				dTmp = *((ULONG_P *)srcDataPtr);
				if (srcScale > 0)
					dTmp = dTmp / (long)pow(10,srcScale);
				break;
			case SQLTYPECODE_LARGEINT:
				tempVal64 = *((__int64 *)srcDataPtr);
				for (i = 0, power = 1 ; i < srcScale ; power *= 10, i++);   
				tempValFrac = tempVal64 % power;
	    		tempVal64 = tempVal64 / power;
				dTmp = ((double)tempValFrac/(double)power);
				dTmp = dTmp + tempVal64;
				break;
			default:
				return IDS_HY_000;
			}
			break;
		case SQL_DECIMAL: 
			if (ConvertDecimalToChar(SQLDataType, srcDataPtr, srcLength, srcScale, 
						cTmpBuf, DecimalPoint) != SQL_SUCCESS)
				return IDS_07_006;
			dTmp = strtod(cTmpBuf, &stopStr);
			break;
		default:
			return IDS_07_006;
		}
		if((ODBCDataType == SQL_SMALLINT) || (ODBCDataType == SQL_INTEGER) || (ODBCDataType == SQL_BIGINT) || (ODBCDataType == SQL_NUMERIC) || (ODBCDataType == SQL_DECIMAL))
		{
			if (dTmp < 0)
				intervalTmp.interval_sign = 1;
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
		}
		DataPtr = &intervalTmp;
		DataLen = sizeof(SQL_INTERVAL_STRUCT);
		break; // End of SQL_C_INTERVAL
	default:
		return IDS_07_006;
	}

	if (LangId != LANG_NEUTRAL)
	{
		if (LocalizeNumericString)
		{
			// If there is any error, the original buffer is used
			if ((DataLen1 = GetNumberFormat(DataLangId,	// locale for which string is to be formatted
							0,							// bit flag that controls the function's operation
							(char *)DataPtr,					// pointer to input number string
							NULL,						// pointer to a formatting information structure
							cTmpBuf1,					// pointer to output buffer
							sizeof(cTmpBuf1))) != 0)	// size of output buffer
			{	
				DataLen = DataLen1-1;					// Returned DataLen includes NULL terminator
				DataPtr = cTmpBuf1;						
			}											
		}
	}
	if (targetDataPtr != NULL && ((DataPtr != NULL && DataLen > 0) || translatedDataPtr != NULL))
	{
		//10-080124-0030 
		//10-090611-2268 seg fault in GetData with DEBUG trace on
		// if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			// TraceOut(TR_ODBC_DEBUG, "ODBC::ConvertSQLToC DataPtr \"%s\", DataLen %d, translateOption 0x%08x", 
				 // DataPtr, DataLen, translateOption);
		//if (fpSQLDataSourceToDriver && translateOption != 0 && (CDataType == SQL_C_CHAR || CDataType == SQL_C_DEFAULT))

		SQLRETURN rc = SQL_SUCCESS;
		SQLINTEGER transLen = 0;
		
		if (translatedDataPtr != NULL && totalReturnedLength != NULL && Offset != 0)
		{ 
			// data has already translated
			if (CDataType == SQL_C_WCHAR && iconv->isAppUTF16())
			{
				DataLen = charlength*2-Offset;
				if (DataLen > targetLength-2)
				{
					DataLenTruncated = DataLen;
					DataLen = targetLength-2;
					if (DataLen % 2 == 1) DataLen--;
					retCode = IDS_01_004;
					if (totalReturnedLength != NULL)
						*totalReturnedLength = DataLen + Offset;
				}
				else
				{
					DataLenTruncated = 0;
					retCode = SQL_SUCCESS;
				}
				memcpy(targetDataPtr, translatedDataPtr+Offset, DataLen);
			}
			else
			{
				if(CDataType == SQL_C_CHAR)
				{
			                if(srcCharSet == SQLCHARSETCODE_UCS2)
					    DataLen = charlength / 2 - Offset;
					else
					    DataLen = charlength  - Offset;
					if (DataLen >= targetLength)
					{
						DataLenTruncated = DataLen;
						DataLen = targetLength-1;
						if (totalReturnedLength != NULL)
							*totalReturnedLength = DataLen+Offset;
						retCode = IDS_01_004;
					}
					else
					{
						DataLenTruncated = 0;
						retCode = SQL_SUCCESS;
					}
					memcpy(targetDataPtr, translatedDataPtr+Offset, DataLen);
				}
				else
				{
                                       DataLen = strlen(translatedDataPtr)-Offset;
                                        if (DataLen >= targetLength)
                                        {
                                                DataLenTruncated = DataLen;
						iconv->strcpyUTF8((char*)targetDataPtr, (const char*)translatedDataPtr+Offset, targetLength, DataLen);
                                                DataLen = strlen((char*)targetDataPtr);
                                                if (totalReturnedLength != NULL)
                                                        *totalReturnedLength = DataLen+Offset;
                                                retCode = IDS_01_004;
                                        }
                                        else
                                        {
						iconv->strcpyUTF8((char*)targetDataPtr, (const char*)translatedDataPtr+Offset, targetLength, DataLen);
                                                DataLenTruncated = 0;
                                                retCode = SQL_SUCCESS;
                                        }
				}
			}
		}
		else if ((CDataType == SQL_C_CHAR) && (CDataType != SQL_C_BINARY))
		{
			//if (!iconv->isAppUTF16() && srcCharSet == SQLCHARSETCODE_UCS2)
			if (srcCharSet == SQLCHARSETCODE_UCS2)
			{
				// translate from UTF16 to DrvrLocale
				if ((rc = iconv->WCharToLocale((UChar*)DataPtr, DataLen/2, (char*)targetDataPtr, targetLength, 
												&transLen, (char*)errorMsg, replacementChar)) != SQL_ERROR)
				{
					if (rc == SQL_SUCCESS_WITH_INFO)
					{	
						// buffer overflow - need to allocate temporary buffer
						translateLengthMax = DataLen*4+1;
						if (translatedDataPtr != NULL) delete[] translatedDataPtr;
						translatedDataPtr = new char[translateLengthMax];
						memset((char*)translatedDataPtr,'\0',translateLengthMax);
						rc = iconv->WCharToLocale((UChar*)DataPtr, DataLen/2,(char*)translatedDataPtr, translateLengthMax, &transLen, (char*)errorMsg, replacementChar);
						if (rc != SQL_SUCCESS) return IDS_190_DSTODRV_ERROR;
						DataLenTruncated = transLen;
						DataLen = targetLength-1;
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
					if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
						TraceOut(TR_ODBC_DEBUG, "ODBC::ConvertSQLToC: WCharToLocale: Error: DataPtr \"%s\",	DataLen %d, targetLength %d", DataPtr, DataLen, targetLength);
					return IDS_190_DSTODRV_ERROR;					
				}
			}
		//	else if(iconv->isAppUTF16() && srcCharSet == SQLCHARSETCODE_UCS2)
		//	{
		//		// If the App Type is UTF16, and source data character set encoding is UCS2
		//		// Do not translate the data, directly copy as UChar* string (using u_strcpy()).
		//		u_strcpy((UChar*)targetDataPtr, (const UChar*)DataPtr);
		//		if (totalReturnedLength != NULL && retCode == IDS_01_004)
		//			*totalReturnedLength = DataLen + Offset;
		//	}
			else if (srcCharSet == SQLCHARSETCODE_UTF8)
			{
				if ((rc = iconv->TranslateUTF8(true, (char*)DataPtr, DataLen, (char*)targetDataPtr, targetLength, 
												&transLen, (char*)errorMsg)) != SQL_ERROR)
				{
					if (rc == SQL_SUCCESS_WITH_INFO)
					{	
						// buffer overflow - need to allocate temporary buffer
						translateLengthMax = DataLen+1;
						if (translatedDataPtr != NULL) delete[] translatedDataPtr;
						translatedDataPtr = new char[translateLengthMax];
						memset((char*)translatedDataPtr,'\0',translateLengthMax);
						rc = iconv->TranslateUTF8(true, (char*)DataPtr, DataLen,(char*)translatedDataPtr, translateLengthMax, &transLen, (char*)errorMsg);
						if (rc != SQL_SUCCESS) return IDS_190_DSTODRV_ERROR;
						DataLenTruncated = transLen;
						DataLen = targetLength-1;
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
					if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
						TraceOut(TR_ODBC_DEBUG, "ODBC::ConvertSQLToC: WCharToLocale: Error: DataPtr \"%s\",	DataLen %d, targetLength %d", DataPtr, DataLen, targetLength);
					return IDS_190_DSTODRV_ERROR;					
				}
			}
			else 
			if ((iconv->isIso88591Translation()) && srcCharSet != SQLCHARSETCODE_UCS2 && 
				((ODBCDataType ==  SQL_CHAR) || (ODBCDataType == SQL_VARCHAR) || (ODBCDataType == SQL_LONGVARCHAR)))
			{
				// translate from Iso88591 to DrvrLocale
				if((rc = iconv->TranslateISO88591(true, (char *)DataPtr, DataLen, (char *)targetDataPtr,
							targetLength, &transLen, (char *)errorMsg)) != SQL_ERROR)
				{
					if (rc == SQL_SUCCESS_WITH_INFO)
					{
						// buffer overflow - need to allocate temporary buffer
						translateLengthMax = DataLen*4+1;
						if (translatedDataPtr != NULL) delete[] translatedDataPtr;
						translatedDataPtr = new char[translateLengthMax];
						if((rc = iconv->TranslateISO88591(true, (char *)DataPtr, DataLen, (char *)translatedDataPtr,
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
					if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
						TraceOut(TR_ODBC_DEBUG, "ODBC::ConvertSQLToC: TranslateISO88591: Error: %s DataPtr \"%s\", DataLen %d, targetLength %d",
																			errorMsg, DataPtr, DataLen, targetLength);
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
		else if ( CDataType == SQL_C_WCHAR && CDataType != SQL_C_BINARY && ! WCharData )
		{
			if (iconv->isAppUTF16() && srcCharSet != SQLCHARSETCODE_UTF8)
			{
				// translate from iso88591 to UTF16
				if ((rc = iconv->ISO88591ToWChar((char*)DataPtr, DataLen, (UChar*)targetDataPtr,
								targetLength, &transLen, (char*)errorMsg, MB_ERR_INVALID_CHARS, &translateLengthMax)) != SQL_ERROR)	// ALM CR5228. Added translateLengthMax to get therequired length.
				{
					if (rc == SQL_SUCCESS_WITH_INFO)
					{
						// buffer overflow - need to allocate temporary buffer
//						translateLengthMax = DataLen*4+1;	// ALM CR5228. Instead off get the length by function ISO88591ToWchar above.
						if (translatedDataPtr != NULL) delete[] translatedDataPtr;
						translatedDataPtr = new char[(translateLengthMax+1)*2];
	
						if((rc = iconv->ISO88591ToWChar((char *)DataPtr, DataLen, (UChar *)translatedDataPtr,
							translateLengthMax+1, &transLen, (char *)errorMsg, MB_ERR_INVALID_CHARS,NULL)) == SQL_ERROR)
							return IDS_190_DSTODRV_ERROR;
						DataLenTruncated = transLen*2;
						DataLen = targetLength-2;
						if(DataLen%2 == 1) DataLen--; // in case DataLen is an odd number
						memcpy(targetDataPtr, translatedDataPtr, DataLen);
						if (errorMsg) errorMsg[0] = '\0'; // reset errorMsg in case of buffer overflow
						retCode = IDS_01_004;
						if (totalReturnedLength != NULL)
							*totalReturnedLength = DataLen + Offset;
					}
					else
					{
						DataLen = transLen*2;
						DataLenTruncated = 0;
					}
				}
				else
				{
					if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
						TraceOut(TR_ODBC_DEBUG, "ODBC::ConvertSQLToC: ISO88591ToWChar: Error: %s DataPtr \"%s\", DataLen %d, targetLength %d",
																		errorMsg, DataPtr, DataLen, targetLength);
					return IDS_190_DSTODRV_ERROR;
				}	
			}
			else
			if ( !iconv->isAppUTF16() && srcCharSet == SQLCHARSETCODE_UTF8)
			{
				if(targetLength > DataLen)
					memcpy(targetDataPtr, DataPtr, DataLen);
				else
				{
                                                // buffer overflow - need to allocate temporary buffer
                                                translateLengthMax = DataLen+1;
                                                if (translatedDataPtr != NULL) delete[] translatedDataPtr;
                                                translatedDataPtr = new char[translateLengthMax];
						memcpy((char*)translatedDataPtr, (const char*)DataPtr, DataLen);
						translatedDataPtr[DataLen] = '\0';
                                                DataLenTruncated = DataLen;
						iconv->strcpyUTF8((char*)targetDataPtr, (const char*)translatedDataPtr, targetLength, DataLen);
                                                if (errorMsg) errorMsg[0] = '\0'; // reset errorMsg in case of buffer overflow
						DataLen = strlen((char*)targetDataPtr);
                                                retCode = IDS_01_004;
                                                if (totalReturnedLength != NULL)
                                                        *totalReturnedLength = strlen((char*)targetDataPtr) + Offset;
				}
			}
			else
			if ( iconv->isAppUTF16() && srcCharSet == SQLCHARSETCODE_UTF8)
			{
				if((rc = iconv->UTF8ToWChar((char*)DataPtr,DataLen,(UChar*)targetDataPtr,targetLength,&transLen,(char *)errorMsg,MB_ERR_INVALID_CHARS,&translateLengthMax))!=SQL_ERROR)
				{
					if (rc == SQL_SUCCESS_WITH_INFO)
					{
						// buffer overflow - need to allocate temporary buffer
						if (translatedDataPtr != NULL) delete[] translatedDataPtr;
						translatedDataPtr = new char[(translateLengthMax+1)*2];
	
						if((rc = iconv->UTF8ToWChar((char *)DataPtr, DataLen, (UChar *)translatedDataPtr,
															 (translateLengthMax+1)*2, &transLen, (char *)errorMsg,MB_ERR_INVALID_CHARS,NULL)) == SQL_ERROR)
							return IDS_190_DSTODRV_ERROR;
						DataLenTruncated = transLen*2;
						DataLen = targetLength-2;
						if(DataLen%2 == 1) DataLen--;
						memcpy(targetDataPtr, translatedDataPtr, DataLen);
						if (errorMsg) errorMsg[0] = '\0'; // reset errorMsg in case of buffer overflow
						retCode = IDS_01_004;
						if (totalReturnedLength != NULL)
							*totalReturnedLength = DataLen + Offset;
					}
					else
					{
						DataLen = transLen*2;
						DataLenTruncated = 0;
					}
				}
				else
				{
					if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
						TraceOut(TR_ODBC_DEBUG, "ODBC::ConvertSQLToC: UTF8ToWChar: Error: %s DataPtr \"%s\", DataLen %d, targetLength %d",
																		errorMsg, DataPtr, DataLen, targetLength);
					return IDS_190_DSTODRV_ERROR;
				}	
			}
			else
			{
				// translate from iso88591 to UTF8
				if ((rc = iconv->UTF8ToFromISO88591(false,(char*)DataPtr, DataLen, (char*)targetDataPtr,
								targetLength, &transLen, (char*)errorMsg)) != SQL_ERROR) 	
				{
					if (rc == SQL_SUCCESS_WITH_INFO)
					{
						// buffer overflow - need to allocate temporary buffer
						translateLengthMax = DataLen*4+1;
						if (translatedDataPtr != NULL) delete[] translatedDataPtr;
						translatedDataPtr = new char[translateLengthMax];
	
						if((rc = iconv->UTF8ToFromISO88591(false, (char *)DataPtr, DataLen, (char *)translatedDataPtr,
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
					if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
						TraceOut(TR_ODBC_DEBUG, "ODBC::ConvertSQLToC:  Error \"%s\", DataPtr \"%s\", DataLen %d, targetLength %d",
																	errorMsg, DataPtr, DataLen, targetLength);
					return IDS_190_DSTODRV_ERROR;
				}				
			}			
		}
		else if( CDataType == SQL_C_WCHAR && CDataType != SQL_C_BINARY &&  WCharData && !iconv->isAppUTF16())
		{
		//Special case where the data is WChar but the application type is UTF8
			if ((rc = iconv->WCharToUTF8((UChar*)DataPtr, DataLen/2, (char*)targetDataPtr,
							targetLength, &transLen, (char*)errorMsg, MB_ERR_INVALID_CHARS, &translateLengthMax)) != SQL_ERROR)
			{
				if (rc == SQL_SUCCESS_WITH_INFO)
				{
					// buffer overflow - need to allocate temporary buffer
					translateLengthMax = DataLen*4+1;
					if (translatedDataPtr != NULL) delete[] translatedDataPtr;
						translatedDataPtr = new char[translateLengthMax];

					if((rc = iconv->WCharToUTF8((UChar *)DataPtr, DataLen/2, (char *)translatedDataPtr,
								translateLengthMax, &transLen, (char *)errorMsg, MB_ERR_INVALID_CHARS, NULL)) == SQL_ERROR)
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
				if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
				TraceOut(TR_ODBC_DEBUG, "ODBC::ConvertSQLToC: WCharToUTF8: Error: %s DataPtr \"%s\", DataLen %d, targetLength %d", errorMsg, DataPtr, DataLen, targetLength);
				return IDS_190_DSTODRV_ERROR;
			}
		}
		else
		{
			memcpy(targetDataPtr, DataPtr, DataLen);
			if (totalReturnedLength != NULL && retCode == IDS_01_004)
				*totalReturnedLength = DataLen + Offset;
		}

		if (NullTerminate && DataLen > 0)
			*(char *)((const char *)targetDataPtr + DataLen) = '\0';
		if (NullTerminateW && DataLen > 0)
			*((UChar *)((const UChar *)targetDataPtr + DataLen/2)) = UCharNull;
	}
	else
	{
		if(WCharData && (iconv->isAppUTF16()))
			((UChar*)targetDataPtr)[0] = UCharNull;
		else
			((char*)targetDataPtr)[0] = '\0';
		*targetStrLenPtr = 0 ;
	}
	if (targetStrLenPtr != NULL)
	{
		if (DataLenTruncated != 0) 
			*targetStrLenPtr = DataLenTruncated;
		else
			*targetStrLenPtr = DataLen;
	}
	return retCode;
}


SQLRETURN ODBC::ConvertNumericToChar(SQLSMALLINT SQLDataType, SQLPOINTER srcDataPtr, SQLSMALLINT srcScale, 
			char *cTmpBuf, SQLINTEGER &DecimalPoint)
{
	
	long lTmp;
	ldiv_t lDiv;
	__int64 i64Tmp;
	char *tmpPtr;

	switch (SQLDataType) {
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
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
					sprintf(cTmpBuf, ".%0*Ld", srcScale, rem);
#else
					sprintf(cTmpBuf, ".%0*lld", srcScale, rem);
#endif
				else
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
					sprintf(cTmpBuf, "%Ld.%0*Ld", t, srcScale, rem);
#else
					sprintf(cTmpBuf, "%lld.%0*lld", t, srcScale, rem);
#endif
			}
			else
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
				sprintf(cTmpBuf, "%Ld", i64Tmp);
#else
				sprintf(cTmpBuf, "%lld", i64Tmp);
#endif
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
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
						sprintf(cTmpBuf, "-.%0*Ld", srcScale, -rem);
#else
						sprintf(cTmpBuf, "-.%0*lld", srcScale, -rem);
#endif
					else
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
						sprintf(cTmpBuf, ".%0*Ld", srcScale, rem);
#else
						sprintf(cTmpBuf, ".%0*lld", srcScale, rem);
#endif
				}
				else
				{
					if (t == 0 && rem < 0)
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
					sprintf(cTmpBuf, "-%Ld.%0*Ld", t, srcScale, -rem);
#else
					sprintf(cTmpBuf, "-%lld.%0*lld", t, srcScale, -rem);
#endif
					else
					{
						if (rem < 0)
							rem = -rem; // Is there a abs for __int64?
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
						sprintf(cTmpBuf, "%Ld.%0*Ld", t, srcScale, rem);
#else
						sprintf(cTmpBuf, "%lld.%0*lld", t, srcScale, rem);
#endif


					}
				}
			}
			else
#if !defined MXHPUX && !defined MXOSS && !defined MXAIX && !defined MXSUNSPARC
				sprintf(cTmpBuf, "%Ld", i64Tmp);
#else
				sprintf(cTmpBuf, "%lld", i64Tmp);
#endif
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

SQLRETURN ODBC::ConvertDecimalToChar(SQLSMALLINT SQLDataType, SQLPOINTER srcDataPtr, SQLINTEGER srcLength,
								SQLSMALLINT srcScale, char *cTmpBuf, SQLINTEGER &DecimalPoint)
{

	char *destTempPtr;
	short i;	
	BOOL leadZero;
	BOOL leadDecimalPoint = TRUE;
	destTempPtr = cTmpBuf;
	char	*tmpPtr;
	
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
			BYTE valByte;	// Sign Bit + first digit
	
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


SQLRETURN ODBC::ConvertSoftDecimalToDouble(SQLSMALLINT SQLDataType, SQLPOINTER srcDataPtr, SQLINTEGER srcLength, 
			SQLSMALLINT srcScale, double &dTmp)
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

unsigned long ODBC::ConvertSQLCharToNumeric(SQLPOINTER srcDataPtr, SQLINTEGER srcLength,
									SQLSMALLINT ODBCDataType, double &dTmp)
{
	SQLINTEGER	tempLen;
	char	cTmpBuf[100];
	char	*str;
	char    *errorCharPtr;

	
	switch (ODBCDataType) {
	case SQL_CHAR:
	case SQL_WCHAR:
	case SQL_VARCHAR:
	case SQL_LONGVARCHAR:
	case SQL_WVARCHAR:
		str = (char *)srcDataPtr;
		tempLen = srcLength-1;
		if (str[ tempLen - 1 ] == ' ')
		{
			str[ tempLen - 1 ] = 0;
			rTrim( str);
			tempLen = strlen(str);
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
		str = trimInterval((char *)srcDataPtr);
		tempLen = strlen(str);
		break;
	default:
		return IDS_07_006;
	}
	if (tempLen > sizeof(cTmpBuf)-1)
		return IDS_22_003;
	strncpy(cTmpBuf, (const char *)str, tempLen);
	cTmpBuf[tempLen] = '\0';
	tempLen = (short)strlen(rTrim(cTmpBuf));
	dTmp = strtod(cTmpBuf, &errorCharPtr);
	if (errno == ERANGE || errorCharPtr < (cTmpBuf + tempLen))
		return IDS_22_018;
	return SQL_SUCCESS;
}

unsigned long ODBC::ConvertSQLCharToDate(SQLSMALLINT ODBCDataType, 
						SQLPOINTER srcDataPtr,
						SQLINTEGER	srcLength,
						SQLSMALLINT CDataType,
						SQLPOINTER outValue)
{						  
    char    in_value[50];
    short   datetime_parts[8];
    char    *token;
    short   i;
    SQLUINTEGER fraction_part = 0;
    char    delimiters[3];
    short	len;
	char	*strPtr;
	char *saveptr=NULL;
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
	default:
		return IDS_07_006;
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

//
// the conversion from the char to little endian mode 
//

unsigned long ODBC::ConvertCharToCNumeric( SQL_NUMERIC_STRUCT& numericTmp, CHAR* cTmpBuf)
{
	unsigned char localBuf[101];
	char* tempPtr = (char*)localBuf,*tempPtr1;
	int i,j,a,b,current,calc,length;

	SQLCHAR tempPrecision;
	SQLCHAR tempScale;
	SQLCHAR tempSign;
	SQLCHAR tmpVal[101];

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

	numericTmp.sign = tempSign;
	numericTmp.precision = tempPrecision;
	numericTmp.scale = tempScale;
	memcpy( numericTmp.val, tmpVal, SQL_MAX_NUMERIC_LEN);

	return SQL_SUCCESS;
}
//
// the conversion from the char to numeric big endian mode 
//

unsigned long ODBC::ConvertCharToBigEndianCNumeric( SQL_NUMERIC_STRUCT& numericTmp, CHAR* cTmpBuf)
{
	unsigned long retcode = ConvertCharToCNumeric( numericTmp, cTmpBuf);
	if (retcode == SQL_SUCCESS)
		byte_swap((BYTE *)numericTmp.val, SQL_MAX_NUMERIC_LEN);

	return retcode;
}

//
// the conversion from the char to bigint big endian mode  
//

unsigned long ODBC::ConvertCharToBigEndianCBigint( void* bigintTmp, CHAR* cTmpBuf)
{
	SQL_NUMERIC_STRUCT numericTmp;
	unsigned long retcode = ConvertCharToCNumeric( numericTmp, cTmpBuf);
	if (retcode == SQL_SUCCESS)
	{
		byte_swap((BYTE *)numericTmp.val, SQL_MAX_NUMERIC_LEN);
		memcpy(bigintTmp, numericTmp.val, SQL_MAX_NUMERIC_LEN);
	}

	return retcode;
}

unsigned long ODBC::ConvertSQLCharToInterval(SQLSMALLINT ODBCDataType, 
						SQLPOINTER srcDataPtr,
						SQLINTEGER	srcLength,
						SQLSMALLINT CDataType,
						SQLPOINTER outValue)
{						  
    char			in_value[128];
	char			temp_value[128];
    unsigned long	interval_parts[5];
	short			sign = 0;
    char			*token;
    short			i;
    SQLUINTEGER  	fraction_part = 0;
    char			delimiters[6];
    short			len;
	char			*strPtr;
	char			*pdest;
	char *saveptr=NULL;

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


