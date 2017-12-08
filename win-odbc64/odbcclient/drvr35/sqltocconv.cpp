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
#include "sqltocconv.h"
#include <stdio.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include "sqlcli.h"
#include "drvrSrvr.h"
#include "tdm_odbcDrvMsg.h"
#include "drvrglobal.h"
#include "nskieee.h"
#include "DiagFunctions.h"
#include "csconvert.h" 
#include <errno.h>
#include <string.h>

#define MAXCHARLEN 32768 //32K

// for server2008 when using function pow() then throws STATUS_ILLEGAL_INSTRUCTION
double pow(int base, short power, unsigned long *error)
{
	DWORD dwVersion = 0;
	DWORD dwBuild = 0;

	dwVersion = GetVersion();

	// Get the build number.

	if (dwVersion < 0x80000000)
		dwBuild = (DWORD)(HIWORD(dwVersion));

	double retValue = 1;
	if (dwBuild == 7600)
	{
		for (int i = 0; i < power; i++)
			retValue = retValue * 10;
	}
	else
	{
		errno = 0;
		retValue = pow((double)base, power);
		if (errno == ERANGE || retValue == 0)
			*error = IDS_22_003;
	}

	return retValue;
}

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
						  BOOL *truncation
						  );

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
		{   //need leading zero for NEO
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


// totalReturnedLength is a Input/Output parameter
// *totalReturnedLength = Offset in Input
// srcLength includes NULL for SQL_CHAR Type, hence srcLength is srcLength-1 for SQL_CHAR fields
unsigned long ODBC::ConvertSQLToC(SQLINTEGER	ODBCAppVersion,
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
								SQLINTEGER	*totalReturnedLength,
								DWORD		translateOption,
								UCHAR		*errorMsg,
								SWORD		errorMsgMax,
								SQLINTEGER	EnvironmentType,
								BOOL		ColumnwiseData,
								CHAR		*replacementChar)
{
	unsigned long	retCode = SQL_SUCCESS;
	WORD			LangId;
	char			*cTmpBuf = NULL;
	double			dTmp = 0;
	SQLINTEGER		DataLen = 0;
	SQLSMALLINT		ODBCDataType = srcDescPtr->m_ODBCDataType;

	if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable){
		TraceOut(TR_ODBC_API,"ConvertSQLToC(%d, %d, %d, %d, %#x, %d, %d, %d, %d, %d, %#x, %d, %#x, %d, %#x, %#x, %#x, %d, %d)",
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
							translateOption,
							errorMsg,
							errorMsgMax,
							EnvironmentType
							);
	}
	else
		RESET_TRACE();

	if (ODBCAppVersion == SQL_OV_ODBC2)
	{
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

		if (ODBCDataType == SQL_NUMERIC && srcDescPtr->m_SQLDataType == SQLTYPECODE_LARGEINT &&
			srcDescPtr->m_ODBCPrecision == 19 && srcDescPtr->m_ODBCScale == 0)
		{
			srcDescPtr->m_ODBCDataType = SQL_BIGINT;
		}

		if (ODBCDataType == SQL_INTEGER && srcDescPtr->m_SQLDataType == SQLTYPECODE_SMALLINT_UNSIGNED &&
			srcDescPtr->m_ODBCPrecision == 10 && srcDescPtr->m_ODBCScale == 0)
		{
			srcDescPtr->m_ODBCDataType = SQL_SMALLINT;
		}

		if (ODBCDataType == SQL_NUMERIC && srcDescPtr->m_SQLDataType == SQLTYPECODE_INTEGER_UNSIGNED &&
			srcDescPtr->m_ODBCPrecision == 19 && srcDescPtr->m_ODBCScale == 0)
		{
			srcDescPtr->m_ODBCDataType = SQL_INTEGER;
		}

		if (ODBCDataType == SQL_BIGINT && srcDescPtr->m_SQLDataType == SQLTYPECODE_INTEGER_UNSIGNED &&
			srcDescPtr->m_ODBCPrecision == 19 && srcDescPtr->m_ODBCScale == 0)
		{
			srcDescPtr->m_ODBCDataType = SQL_INTEGER;
		}
	}

	ODBCDataType = srcDescPtr->m_ODBCDataType;

	if (CDataType == SQL_C_DEFAULT)
	{
		retCode = getCDefault(ODBCDataType, ODBCAppVersion, CDataType);
	}

	if (retCode != SQL_SUCCESS)
	{
		return retCode;
	}

//--------------------------------------------------------------------------------------

	if (errorMsg != NULL)
		*errorMsg = '\0';

	if (srcDataPtr == NULL)
		return IDS_HY_000;

	LangId = LANG_NEUTRAL;

	switch (CDataType)
	{
	case SQL_C_CHAR:
	case SQL_C_WCHAR:
		switch (ODBCDataType)
		{
		case SQL_CHAR:
		case SQL_WCHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WVARCHAR:
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
			retCode = ConvSQLCharToChar(srcDataPtr, srcDescPtr, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr, translateOption, translatedDataPtr, totalReturnedLength, errorMsg, errorMsgMax, replacementChar);
			break;

		case SQL_TINYINT:
		case SQL_SMALLINT:
		case SQL_INTEGER: 
		case SQL_BIGINT:
		case SQL_REAL:
		case SQL_DOUBLE:
		case SQL_DECIMAL:
		case SQL_NUMERIC:
			retCode = ConvSQLNumberToChar(srcDataPtr, srcDescPtr, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
			break;

		case SQL_DATE:
		case SQL_TYPE_DATE:
			retCode = ConvSQLDateToChar(srcDataPtr, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
			break;

		case SQL_TIME:
		case SQL_TYPE_TIME:
			retCode = ConvSQLTimeToChar(srcDataPtr, srcDescPtr->m_ODBCPrecision, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
			break;
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
			retCode = ConvSQLTimestampToChar(srcDataPtr, srcDescPtr->m_ODBCPrecision, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
			break;

		default:
			return IDS_07_006;
		}
		break; // End of SQL_C_CHAR

	case SQL_C_BINARY:
		switch (ODBCDataType)
		{
		case SQL_CHAR:
		case SQL_WCHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WVARCHAR:
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
			retCode = ConvSQLCharToChar(srcDataPtr, srcDescPtr, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr, translateOption, translatedDataPtr, totalReturnedLength, errorMsg, errorMsgMax, replacementChar);
			break;

		case SQL_TINYINT:
		case SQL_SMALLINT:
		case SQL_INTEGER:
		case SQL_BIGINT:
		case SQL_REAL:
		case SQL_DOUBLE:
		case SQL_DECIMAL:
		case SQL_NUMERIC:
		case SQL_DATE:
		case SQL_TYPE_DATE:
		case SQL_TIME:
		case SQL_TYPE_TIME:
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
			if (srcLength > targetLength)
				return IDS_22_003;
			memcpy(targetDataPtr, srcDataPtr, srcLength);
			if (targetStrLenPtr)
				*targetStrLenPtr = srcLength;
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
		switch (ODBCDataType)
		{
		case SQL_CHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WCHAR:
		case SQL_WVARCHAR:
		case SQL_WLONGVARCHAR:
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
			retCode = ConvSQLCharToNumber(srcDataPtr, srcDescPtr, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
			break;
		case SQL_TINYINT:
		case SQL_SMALLINT:
		case SQL_INTEGER:
		case SQL_REAL:
		case SQL_DOUBLE:
		case SQL_DECIMAL:
			retCode = ConvSQLNumberToNumber(srcDataPtr, srcDescPtr, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
			break;
		case SQL_BIGINT:
			retCode = ConvSQLBigintToNumber(srcDataPtr, srcDescPtr->m_DescUnsigned, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
			break;
		case SQL_NUMERIC:
			retCode = ConvSQLNumericToNumber(srcDataPtr, srcDescPtr, srcLength, CDataType, targetDataPtr, targetLength, targetStrLenPtr);
			break;

		default:
			return IDS_07_006;
		}
		break; 
	case SQL_C_DATE:
	case SQL_C_TYPE_DATE:
		switch (ODBCDataType)
		{
		case SQL_CHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WCHAR:
		case SQL_WVARCHAR:
			retCode = GetCTmpBufFromSQLChar(srcDataPtr, srcDescPtr, srcLength, srcDescPtr->m_SQLMaxLength <= 32767, cTmpBuf, &DataLen);
			if (retCode == SQL_SUCCESS)
				retCode = ConvertSQLCharToDateTime(ODBCDataType, cTmpBuf, DataLen, SQL_C_DATE, targetDataPtr);
			break;
		case SQL_DATE:
		case SQL_TYPE_DATE:
			retCode = ConvSQLDateToDate(srcDataPtr, srcDescPtr->m_SQLDatetimeCode, ColumnwiseData, targetDataPtr);
			break;
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
			retCode = ConvSQLTimestampToDateTime(srcDataPtr, srcDescPtr->m_SQLDatetimeCode, 0, ColumnwiseData, CDataType, targetDataPtr);
			break;
		default:
			return IDS_07_006;
		}
		break; // End of switch for SQL_C_DATE

	case SQL_C_TIME:
	case SQL_C_TYPE_TIME:
		switch (ODBCDataType)
		{
		case SQL_CHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WCHAR:
		case SQL_WVARCHAR:
			retCode = GetCTmpBufFromSQLChar(srcDataPtr, srcDescPtr, srcLength, srcDescPtr->m_SQLMaxLength <= 32767, cTmpBuf, &DataLen);
			if (retCode == SQL_SUCCESS)
				retCode = ConvertSQLCharToDateTime(ODBCDataType, cTmpBuf, DataLen, SQL_C_TIME, targetDataPtr);
			break;
		case SQL_TIME:
		case SQL_TYPE_TIME:
			retCode = ConvSQLTimeToTime(srcDataPtr, srcDescPtr->m_SQLDatetimeCode, ColumnwiseData, targetDataPtr);
			break;
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
			retCode = ConvSQLTimestampToDateTime(srcDataPtr, srcDescPtr->m_SQLDatetimeCode, 0, ColumnwiseData, CDataType, targetDataPtr);
			break;
		default:
			return IDS_07_006;
		}
		break; // End of switch for SQL_C_TIME

	case SQL_C_TIMESTAMP:
	case SQL_C_TYPE_TIMESTAMP:
		switch (ODBCDataType)
		{
		case SQL_CHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WCHAR:
		case SQL_WVARCHAR:
			retCode = GetCTmpBufFromSQLChar(srcDataPtr, srcDescPtr, srcLength, srcDescPtr->m_SQLMaxLength <= 32767, cTmpBuf, &DataLen, false);
			if (retCode == SQL_SUCCESS)
				retCode = ConvertSQLCharToDateTime(ODBCDataType, cTmpBuf, DataLen, SQL_C_TIMESTAMP, targetDataPtr);
			break;
		case SQL_DATE:
		case SQL_TYPE_DATE:
			retCode = ConvSQLDateToTimestamp(srcDataPtr, srcDescPtr->m_SQLDatetimeCode, ColumnwiseData, targetDataPtr);
			break;
		case SQL_TIME:
		case SQL_TYPE_TIME:
			retCode = ConvSQLTimeToTimestamp(srcDataPtr, srcDescPtr->m_SQLDatetimeCode, ColumnwiseData, targetDataPtr);
			break;
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
			retCode = ConvSQLTimestampToDateTime(srcDataPtr, srcDescPtr->m_SQLDatetimeCode, srcDescPtr->m_ODBCPrecision, ColumnwiseData, CDataType, targetDataPtr);
			break;
		default:
			return IDS_07_006;
		}
		break; // End of switch for SQL_C_TIMESTAMP

	case SQL_C_NUMERIC:
		switch (ODBCDataType)
		{
		case SQL_CHAR:
		case SQL_WCHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WVARCHAR:
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
			retCode = ConvSQLCharToNumeric(srcDataPtr, srcDescPtr, srcLength, targetDataPtr);
			break;
		case SQL_TINYINT:
		case SQL_SMALLINT:
		case SQL_INTEGER:
		case SQL_REAL:
		case SQL_DOUBLE:
		case SQL_BIGINT:
		case SQL_DECIMAL:
		case SQL_NUMERIC:
			cTmpBuf = (char *)malloc(NUM_LEN_MAX);
			memset(cTmpBuf, 0, NUM_LEN_MAX);
			retCode = ConvSQLNumberToChar(srcDataPtr, srcDescPtr, srcLength, CDataType, cTmpBuf, NUM_LEN_MAX, NULL);
			if (retCode == SQL_SUCCESS)
				retCode = ConvertCharToCNumeric(*((SQL_NUMERIC_STRUCT *)srcDataPtr), cTmpBuf);
			*targetStrLenPtr = sizeof(SQL_NUMERIC_STRUCT);
			break;
		default:
			return IDS_07_006;
		}

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
	{
		bool useDouble = true;
		switch (ODBCDataType)
		{
		case SQL_CHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WCHAR:
		case SQL_WVARCHAR:
			retCode = GetCTmpBufFromSQLChar(srcDataPtr, srcDescPtr, srcLength, srcDescPtr->m_SQLMaxLength <= 32767, cTmpBuf, &DataLen, false);
			if (retCode != SQL_SUCCESS)
				break;
			retCode = ConvertSQLCharToInterval(ODBCDataType, cTmpBuf, DataLen, CDataType, targetDataPtr);
			useDouble = false;
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
			retCode = ConvertSQLCharToInterval(ODBCDataType, srcDataPtr, srcLength, CDataType, targetDataPtr);
			useDouble = false;
			break;
		case SQL_TINYINT:
		case SQL_SMALLINT:
		case SQL_INTEGER:
		case SQL_DECIMAL:
			retCode = ConvSQLNumberToNumber(srcDataPtr, srcDescPtr, srcLength, SQL_C_DOUBLE, &dTmp, targetLength, targetStrLenPtr);
			break;
		case SQL_BIGINT:
			retCode = ConvSQLBigintToNumber(srcDataPtr, srcDescPtr->m_DescUnsigned, SQL_C_DOUBLE, &dTmp, targetLength, targetStrLenPtr);
			break;
		case SQL_NUMERIC:
			retCode = ConvSQLNumericToNumber(srcDataPtr, srcDescPtr, srcLength, SQL_C_DOUBLE, &dTmp, targetLength, targetStrLenPtr);
			break;
		default:
			return IDS_07_006;
		}
		if (useDouble)
			retCode = ConvDoubleToInterval(dTmp, CDataType, targetDataPtr);
		*targetStrLenPtr = sizeof(SQL_INTERVAL_STRUCT);
	}
		break; // End of SQL_C_INTERVAL
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
			lTmp = *((long *)srcDataPtr);
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
			i64Tmp = *((ULONG *)srcDataPtr);
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
					sprintf(cTmpBuf, ".%0*I64d", srcScale, rem);
				else
					sprintf(cTmpBuf, "%I64d.%0*I64d", t, srcScale, rem);
			}
			else
				sprintf(cTmpBuf, "%I64d", i64Tmp);
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
				if (gDrvrGlobal.gSpecial_1 && t==0)
				{
					if (rem < 0)
						sprintf(cTmpBuf, "-.%0*I64d", srcScale, -rem);
					else
						sprintf(cTmpBuf, ".%0*I64d", srcScale, rem);
				}
				else
				{
					if (t == 0 && rem < 0)
						sprintf(cTmpBuf, "-%I64d.%0*I64d", t, srcScale, -rem);
					else
					{
						if (rem < 0)
							rem = -rem; // Is there a abs for __int64?
						sprintf(cTmpBuf, "%I64d.%0*I64d", t, srcScale, rem);
					}
				}
			}
			else
				sprintf(cTmpBuf, "%I64d", i64Tmp);
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
	//Fix for customer B trailing Zero problem. B wants to see trailing 0's
	// eg. 123.000. To avoid regression in W, we kept the original logic
	// unchanged for W using if (gDrvrGlobal.gSpecial_1). For W
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

unsigned long ODBC::ConvertSQLCharToDouble(SQLPOINTER srcDataPtr, SQLINTEGER srcLength,
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

unsigned long ODBC::ConvertSQLCharToDateTime(SQLSMALLINT ODBCDataType, 
						SQLPOINTER srcDataPtr,
						SQLINTEGER	srcLength,
						SQLSMALLINT CDataType,
						SQLPOINTER outValue)
{						  
    char    in_value[50];
    short   datetime_parts[8];
    char    *token;
    short   i;
    long    fraction_part = 0;
    char    delimiters[3];
    short	len;
	char	*strPtr;
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

	length = strlen(strcpy( tempPtr, cTmpBuf ));
	if( tempPtr[ length - 1 ] == '.' ) tempPtr[ length - 1 ] = '\0';

	tempSign = (*tempPtr == '-')? 0: 1;

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
    long			fraction_part = 0;
    char			delimiters[3];
    short			len;
	char			*strPtr;
	char			*pdest;

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
	token = strtok(in_value, delimiters );
	while( token != NULL )
	{
		interval_parts[i] = (unsigned long)atol(token);
		token = strtok( NULL, delimiters );
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
UDWORD ODBC::GetFractionFromStr(UCHAR* ptr, short precision)
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

unsigned long ODBC::ConvSQLCharToChar(SQLPOINTER srcDataPtr, CDescRec* srcDescPtr, SQLINTEGER srcLength, SQLSMALLINT CDataType,
								SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN *targetStrLenPtr,
								DWORD translateOption, CHAR *&translatedDataPtr, SQLINTEGER* totalReturnedLength,
								UCHAR *errorMsg, SWORD errorMsgMax, CHAR *replacementChar)
{
	unsigned long	retCode = SQL_SUCCESS;
	SQLINTEGER		Offset = 0;
	int				charlength = 0;
	SQLINTEGER		DataLen = 0;
	SQLINTEGER      DataLenTruncated = 0;
	SQLINTEGER		translateLengthMax = 0;
	SQLINTEGER		translateLength = 0;
	SQLPOINTER		DataPtr = srcDataPtr;
	SQLSMALLINT     ODBCDataType = srcDescPtr->m_ODBCDataType;
	SQLINTEGER      srcCharSet = srcDescPtr->m_SQLCharset;

	bool DefaultCharRequired;
	LPBOOL PtrDefaultCharRequired = (LPBOOL)&DefaultCharRequired;

	if (totalReturnedLength != NULL)
	{
		Offset = *totalReturnedLength;
		*totalReturnedLength = -1;
	}
	else
		Offset = 0;

	switch (ODBCDataType)
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
		break;
	case SQL_VARCHAR:
	case SQL_WVARCHAR:
	case SQL_LONGVARCHAR:
		if (srcDescPtr->m_SQLMaxLength <= MAXCHARLEN){
			charlength = *(USHORT *)srcDataPtr;
			DataPtr = (char *)srcDataPtr + 2 + Offset;
		}
		else{
			charlength = *(int *)srcDataPtr;
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
	}

	if (translatedDataPtr == NULL)
	{
		if (DataLen == 0)
			return SQL_NO_DATA;
		if ((translateOption == 0) && DataLen >= targetLength && srcCharSet != SQLCHARSETCODE_UCS2)
		{
			if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
				TraceOut(TR_ODBC_API, "ConvertSQLToC: \"Data truncated\" OBCDataType %d, srcCharSet %d, DataLen %d, targetLength %d",
				ODBCDataType, srcCharSet, DataLen, targetLength);
			retCode = IDS_01_004;
			DataLenTruncated = DataLen;
			if (targetLength = 1)
				DataLen = targetLength;
			else if (targetLength > 0)
				DataLen = targetLength - 1;
			else
				DataLen = 0;
		}
	}

	if (targetDataPtr != NULL && ((DataPtr != NULL && DataLen > 0) || translatedDataPtr != NULL) && gDrvrGlobal.fpSQLDataSourceToDriver)
	{
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_API, "ODBC::ConvertSQLToC DataPtr \"%s\", DataLen %d, translateOption 0x%08x",
			DataPtr, DataLen, translateOption);

		if (translatedDataPtr != NULL && totalReturnedLength != NULL && Offset != 0)
		{
			// data has already translated
			if (CDataType == SQL_C_WCHAR)
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
					retCode = SQL_SUCCESS;
				}
			}
			else
			{
				if (srcCharSet == SQLCHARSETCODE_UCS2)
					DataLen = charlength / 2 - Offset;
				else
					DataLen = charlength - Offset;
				if (DataLen >= targetLength)
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
					retCode = SQL_SUCCESS;
				}
			}
			memcpy(targetDataPtr, translatedDataPtr + Offset, DataLen);
		}
		else if (CDataType == SQL_C_CHAR)
		{
			if (srcCharSet == SQLCHARSETCODE_UCS2)
			{
				memset((LPSTR)targetDataPtr, '\0', targetLength - 1);
				unsigned int transLen = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)DataPtr, DataLen / 2,
					(LPSTR)targetDataPtr, targetLength - 1, (LPCSTR)replacementChar, PtrDefaultCharRequired);
				if (transLen != 0)
				{
					DataLen = transLen;
					DataLenTruncated = 0;
				}
				else
				{
					switch (GetLastError())
					{
					case ERROR_INSUFFICIENT_BUFFER:
					{
						// buffer overflow - need to allocate temporary buffer
						translateLengthMax = DataLen * 4 + 1;
						if (translatedDataPtr != NULL) delete[] translatedDataPtr;
						translatedDataPtr = new char[translateLengthMax];
						memset((LPSTR)translatedDataPtr, '\0', translateLengthMax);
						transLen = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)DataPtr, DataLen / 2,
							(LPSTR)translatedDataPtr, translateLengthMax - 1, (LPCSTR)replacementChar, PtrDefaultCharRequired);
						if (transLen == 0) return IDS_190_DSTODRV_ERROR;
						DataLenTruncated = transLen;
						DataLen = targetLength - 1;
						memcpy(targetDataPtr, translatedDataPtr, DataLen);
						retCode = IDS_01_004;
						if (totalReturnedLength != NULL)
							*totalReturnedLength = DataLen + Offset;
					}
					break;
					default: //Error
					{
						if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
							TraceOut(TR_ODBC_API, "ODBC::ConvertSQLToC: WideCharToMultiByte: Error: DataPtr \"%s\",	DataLen %d, targetLength %d", DataPtr, DataLen, targetLength);
						return IDS_190_DSTODRV_ERROR;
						break;
					}
					}
				}
			}
			else if (translateOption != 0 && // translate from SQLCharSet to DriverLocale - JC function 
				((ODBCDataType == SQL_CHAR) || (ODBCDataType == SQL_VARCHAR) || (ODBCDataType == SQL_LONGVARCHAR)))
			{
				translateLengthMax = targetLength;
				if (!(gDrvrGlobal.fpSQLDataSourceToDriver) (translateOption,
					ODBCDataType,
					DataPtr,
					DataLen,
					targetDataPtr,
					translateLengthMax,
					&translateLength,
					errorMsg,
					errorMsgMax,
					NULL,
					replacementChar))
				{
					if (translateLength == translateLengthMax || (errorMsg != NULL && strstr((const char*)errorMsg, "overflow") != NULL))
					{
						// buffer overflow - need to allocate big temp buffer
						translateLengthMax = DataLen * 4 + 1;
						if (translatedDataPtr != NULL) delete[] translatedDataPtr;
						translatedDataPtr = new char[translateLengthMax];
						if (!(gDrvrGlobal.fpSQLDataSourceToDriver) (translateOption,
							ODBCDataType,
							DataPtr,
							DataLen,
							translatedDataPtr,
							translateLengthMax,
							&translateLength,
							errorMsg,
							errorMsgMax,
							NULL,
							replacementChar))
							return IDS_190_DSTODRV_ERROR;

						DataLenTruncated = translateLength;
						DataLen = targetLength - 1;
						memcpy(targetDataPtr, translatedDataPtr, DataLen);
						if (errorMsg) errorMsg[0] = '\0'; // reset errorMsg in case of buffer overflow
						retCode = IDS_01_004;
						if (totalReturnedLength != NULL)
							*totalReturnedLength = DataLen + Offset;
					}
					else
					{
						return IDS_190_DSTODRV_ERROR;
					}
				}
				else
				{
					DataLenTruncated = 0;
					DataLen = translateLength;
				}
			}
			else
			{
				memcpy(targetDataPtr, DataPtr, DataLen);
				if (totalReturnedLength != NULL && retCode == IDS_01_004)
					*totalReturnedLength = DataLen + Offset;
			}
		} //SQL_C_CHAR end
		else if (CDataType == SQL_C_WCHAR && translateOption != 0 && srcCharSet == SQLCHARSETCODE_UCS2)
		{
			translateLengthMax = targetLength;
			if (translateLengthMax % 2 == 1) translateLengthMax--;
			if (!(gDrvrGlobal.fpSQLDataSourceToDriver) (translateOption,
				ODBCDataType,
				DataPtr,
				DataLen,
				targetDataPtr,
				translateLengthMax,
				&translateLength,
				errorMsg,
				errorMsgMax,
				NULL,
				replacementChar))
			{
				if (translateLength == translateLengthMax || (errorMsg != NULL && strstr((const char*)errorMsg, "overflow") != NULL))
				{
					// buffer overflow - need to allocate big temp buffer
					translateLengthMax = DataLen * 4 + 1;
					if (translatedDataPtr != NULL) delete[] translatedDataPtr;
					translatedDataPtr = new char[translateLengthMax];
					if (!(gDrvrGlobal.fpSQLDataSourceToDriver) (translateOption,
						ODBCDataType,
						DataPtr,
						DataLen,
						translatedDataPtr,
						translateLengthMax,
						&translateLength,
						errorMsg,
						errorMsgMax,
						NULL,
						replacementChar))
						return IDS_190_DSTODRV_ERROR;

					DataLenTruncated = translateLength;
					DataLen = targetLength - 2;
					if (DataLen % 2 == 1) DataLen--;
					memcpy(targetDataPtr, translatedDataPtr, DataLen);
					if (errorMsg) errorMsg[0] = '\0'; // reset errorMsg in case of buffer overflow
					retCode = IDS_01_004;
					if (totalReturnedLength != NULL)
						*totalReturnedLength = DataLen + Offset;
				}
				else
				{
					return IDS_190_DSTODRV_ERROR;
				}
			}
			else
			{
				DataLenTruncated = 0;
				DataLen = translateLength;
				if (DataLen % 2 == 1) DataLen--;
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
		if (CDataType == SQL_C_WCHAR && DataLen > 0)
			*((wchar_t *)((const wchar_t *)targetDataPtr + DataLen / 2)) = L'\0';
	}
	else
	{
		if (srcCharSet == SQLCHARSETCODE_UCS2)
			((wchar_t*)targetDataPtr)[0] = L'\0';
		else
			((char*)targetDataPtr)[0] = '\0';
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

unsigned long ODBC::ConvSQLNumberToChar(SQLPOINTER srcDataPtr, CDescRec* srcDescPtr, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN *targetStrLenPtr)
{
	unsigned long		retCode	= SQL_SUCCESS;
	__int64				lTmp	= 0;
	unsigned __int64	ulTmp	= 0;
	double				dTmp	= 0;
	SQLINTEGER			DataLen = 0;
	SQLINTEGER			DecimalPoint = 0;
	char				cTmpBuf[NUM_LEN_MAX] = { 0 };
	char				*tempPtr = NULL;

	switch (srcDescPtr->m_ODBCDataType)
	{
	case SQL_TINYINT:
		if (srcDescPtr->m_DescUnsigned)
		{
			ulTmp = *((UCHAR *)srcDataPtr);
			_ultoa(ulTmp, cTmpBuf, 10);
		}
		else
		{
			lTmp = *((SCHAR *)srcDataPtr);
			_ltoa(lTmp, cTmpBuf, 10);
		}
		DataLen = strlen(cTmpBuf);
		if (DataLen > targetLength)
			return IDS_22_003;
		break;
	case SQL_SMALLINT:
		if (srcDescPtr->m_DescUnsigned)
		{
			ulTmp = *((USHORT *)srcDataPtr);
			_ultoa(ulTmp, cTmpBuf, 10);
		}
		else
		{
			lTmp = *((SSHORT *)srcDataPtr);
			_ltoa(lTmp, cTmpBuf, 10);
		}
		DataLen = strlen(cTmpBuf);
		if (DataLen > targetLength)
			return IDS_22_003;
		break;
	case SQL_INTEGER:
		if (srcDescPtr->m_DescUnsigned)
		{
			ulTmp = *((ULONG *)srcDataPtr);
			_ultoa(ulTmp, cTmpBuf, 10);
		}
		else
		{
			lTmp = *((SLONG *)srcDataPtr);
			_ltoa(lTmp, cTmpBuf, 10);
		}
		DataLen = strlen(cTmpBuf);
		if (DataLen > targetLength)
			return IDS_22_003;
		break;
	case SQL_BIGINT:
		if (srcDescPtr->m_DescUnsigned)
			sprintf(cTmpBuf, "%I64u", *((unsigned __int64 *)srcDataPtr));
		else
			sprintf(cTmpBuf, "%I64d", *((__int64 *)srcDataPtr));
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
				dTmp) != SQL_SUCCESS)
				return IDS_07_006;
			if (!double_to_char(dTmp, DBL_DIG, cTmpBuf, targetLength))
				return IDS_22_001;
		}
		else
		{
			if (srcDescPtr->m_ODBCDataType == SQL_REAL) {
				dTmp = (double)(*(float *)srcDataPtr);
				if (!double_to_char(dTmp, FLT_DIG + 1, cTmpBuf, targetLength))
					return IDS_22_001;
			}
			else {
				dTmp = *(double *)srcDataPtr;
				if (!double_to_char(dTmp, DBL_DIG + 1, cTmpBuf, targetLength))
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
			DataLen = targetLength - 1;
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
	case SQL_NUMERIC:
		if ((srcDescPtr->m_SQLDataType == SQLTYPECODE_NUMERIC) ||
			(srcDescPtr->m_SQLDataType == SQLTYPECODE_NUMERIC_UNSIGNED)) //for bignum support
		{
			retCode = BigNum_To_Ascii_Helper((char*)srcDataPtr, srcLength, srcDescPtr->m_ODBCPrecision, srcDescPtr->m_ODBCScale, cTmpBuf, srcDescPtr->m_SQLDataType);
			if (retCode != SQL_SUCCESS)
				return retCode;
		}
		else {
			if ((ConvertNumericToChar(srcDescPtr->m_SQLDataType, srcDataPtr, srcDescPtr->m_ODBCScale, cTmpBuf, DecimalPoint)) != SQL_SUCCESS)
				return IDS_07_006;
			if (DecimalPoint > targetLength)
				return IDS_22_003;
		}
		DataLen = strlen(cTmpBuf);
		if (DataLen  > targetLength)
		{
			DataLen = targetLength - 1;
			retCode = IDS_01_004;
		}
		break;
	}

	if (CDataType == SQL_C_WCHAR)
		swprintf((wchar_t *)targetDataPtr, DataLen, L"%s", cTmpBuf);
	else
		sprintf((char *)targetDataPtr, "%s", cTmpBuf);

	return retCode;
}

unsigned long ODBC::ConvSQLDateToChar(SQLPOINTER srcDataPtr, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN *targetStrLenPtr)
{
	DATE_TYPES *SQLDate = (DATE_TYPES *)srcDataPtr;
	if (targetLength < SQL_DATE_LEN + 1)
		return IDS_22_003;
	if (CDataType == SQL_C_CHAR)
		sprintf((char *)targetDataPtr, "%04d-%02d-%02d", SQLDate->year, SQLDate->month, SQLDate->day);
	else
		swprintf((wchar_t *)targetDataPtr, L"%04d-%02d-%02d", SQLDate->year, SQLDate->month, SQLDate->day);

	if (targetStrLenPtr)
		*targetStrLenPtr = SQL_DATE_LEN;

	return SQL_SUCCESS;
}

unsigned long ODBC::ConvSQLTimeToChar(SQLPOINTER srcDataPtr, SQLINTEGER srcPrecision, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN *targetStrLenPtr)
{
	SQLINTEGER		DataLen;
	unsigned long	ulFraction = 0;
	char sFraction[10] = { 0 };
	char cTmpBuf[SQL_TIME_LEN + 11] = { 0 };
	TIME_TYPES *SQLTime = (TIME_TYPES *)srcDataPtr;

	DataLen = SQL_TIME_LEN + (srcPrecision ? srcPrecision + 1 : 0);
	if (targetLength <= DataLen)
		return IDS_22_003;

	if (srcPrecision > 0)
	{
		ulFraction = 0;
		ulFraction = *(UDWORD*)SQLTime->fraction;
		sprintf(sFraction, "%0*lu", srcPrecision, ulFraction);
		sprintf(cTmpBuf, "%02d:%02d:%02d.%s",
				SQLTime->hour, SQLTime->minute, SQLTime->second, sFraction);
	}
	else
	{
		sprintf(cTmpBuf, "%02d:%02d:%02d",
				SQLTime->hour, SQLTime->minute, SQLTime->second);
	}

	if (CDataType == SQL_C_WCHAR)
		swprintf((wchar_t *)targetDataPtr, L"%s", cTmpBuf);
	else
		sprintf((char *)targetDataPtr, "%s", cTmpBuf);

	if (targetStrLenPtr)
		*targetStrLenPtr = DataLen;

	return SQL_SUCCESS;
}

unsigned long ODBC::ConvSQLTimestampToChar(SQLPOINTER srcDataPtr, SQLINTEGER srcPrecision, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN *targetStrLenPtr)
{
	SQLINTEGER		DataLen;
	unsigned long	ulFraction = 0;
	char sFraction[10] = { 0 };
	char cTmpBuf[SQL_TIMESTAMP_LEN + 11] = { 0 };
	TIMESTAMP_TYPES* SQLTimestamp = (TIMESTAMP_TYPES *)srcDataPtr;

	DataLen = SQL_TIMESTAMP_LEN + (srcPrecision ? srcPrecision + 1 : 0);
	if (targetLength <= DataLen)
		return IDS_22_003;

	if (srcPrecision > 0)
	{
		ulFraction = *(UDWORD*)SQLTimestamp->fraction;
		sprintf(sFraction, "%0*lu", srcPrecision, ulFraction);
		ulFraction = atol(sFraction);
		sprintf(cTmpBuf, "%04d-%02u-%02u %02u:%02u:%02u.%s",
			SQLTimestamp->year, SQLTimestamp->month, SQLTimestamp->day,
			SQLTimestamp->hour, SQLTimestamp->minute, SQLTimestamp->second, sFraction);
	}
	else
	{
		sprintf(cTmpBuf, "%04d-%02u-%02u %02u:%02u:%02u",
			SQLTimestamp->year, SQLTimestamp->month, SQLTimestamp->day,
			SQLTimestamp->hour, SQLTimestamp->minute, SQLTimestamp->second);
	}

	if (CDataType == SQL_C_WCHAR)
		swprintf((wchar_t *)targetDataPtr, L"%s", cTmpBuf);
	else
		sprintf((char *)targetDataPtr, "%s", cTmpBuf);

	if (targetStrLenPtr)
		*targetStrLenPtr = DataLen;

	return SQL_SUCCESS;
}

unsigned long ODBC::ConvSQLCharToNumber(SQLPOINTER srcDataPtr, CDescRec* srcDescPtr, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN *targetStrLenPtr)
{
	unsigned long	retCode		= SQL_SUCCESS;
	__int64			tempVal64	= 0;
	double			dTmp = 0;
	SQLINTEGER		DataLen		= 0;
	int TransStringLength = 0;

	char error[64];
	char			*cTmpBuf	= NULL;
	SQLSMALLINT     ODBCDataType = srcDescPtr->m_ODBCDataType;
	SQLINTEGER      srcCharSet	= srcDescPtr->m_SQLCharset;

	retCode = GetCTmpBufFromSQLChar(srcDataPtr, srcDescPtr, srcLength, srcDescPtr->m_SQLMaxLength <= 32767, cTmpBuf, &DataLen);

	if (retCode == SQL_SUCCESS)
		retCode = ConvertSQLCharToDouble(cTmpBuf, DataLen, ODBCDataType, dTmp);

	if (retCode == SQL_SUCCESS)
		retCode = ConvertDoubleToNumber(dTmp, CDataType, targetDataPtr, targetLength, targetStrLenPtr);

	if (cTmpBuf != NULL)
	{
		free(cTmpBuf);
		cTmpBuf = NULL;
	}

	return retCode;
}

unsigned long ODBC::ConvertDoubleToNumber(double dTmp, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN *targetStrLenPtr)
{
	unsigned long retCode = SQL_SUCCESS;

	CHAR	tTmp	= 0;
	UCHAR	utTmp	= 0;
	SHORT	sTmp	= 0;
	USHORT	usTmp	= 0;
	SLONG	lTmp	= 0;
	ULONG	ulTmp	= 0;
	__int64	tempVal64 = 0;
	unsigned __int64 utempVal64 = 0;
	FLOAT	fltTmp	= 0;

	SQLPOINTER DataPtr = NULL;
	SQLLEN	DataLen = 0;

	switch (CDataType)
	{
	case SQL_C_BIT:
		if (dTmp == 0 || dTmp == 1)
			utTmp = (UCHAR)dTmp;
		else if (dTmp > 0 && dTmp < 2)
		{
			utTmp = (UCHAR)dTmp;
			retCode = IDS_01_S07;
		}
		else
			retCode = IDS_22_003;
		DataPtr = &utTmp;
		DataLen = sizeof(UCHAR);
		break;

	case SQL_C_TINYINT:
	case SQL_C_STINYINT:
		if (dTmp < SCHAR_MIN || dTmp > SCHAR_MAX)
			return IDS_22_003;
		tTmp = (SCHAR)dTmp;
		if (dTmp != tTmp)
			retCode = IDS_01_S07;
		DataPtr = &tTmp;
		DataLen = sizeof(SCHAR);
		break;
	case SQL_C_UTINYINT:
			if (dTmp < 0 || dTmp > UCHAR_MAX)
				return IDS_22_003;
			utTmp = (UCHAR)dTmp;
			if (dTmp != utTmp)
				retCode = IDS_01_S07;
		DataPtr = &utTmp;
		DataLen = sizeof(UCHAR);
		break;

	case SQL_C_SHORT:
	case SQL_C_SSHORT:
		if (dTmp < SHRT_MIN || dTmp > SHRT_MAX)
			return IDS_22_003;
		sTmp = (SHORT)dTmp;
		if (dTmp != sTmp)
			return IDS_01_S07;
		DataPtr = &sTmp;
		DataLen = sizeof(SHORT);
		break;
	case SQL_C_USHORT:
		if (dTmp < 0 || dTmp > USHRT_MAX)
			return IDS_22_003;
		usTmp = (USHORT)dTmp;
		if (dTmp != usTmp)
			return IDS_01_S07;
		DataPtr = &usTmp;
		DataLen = sizeof(USHORT);
		break;

	case SQL_C_LONG:
	case SQL_C_SLONG:
		if (dTmp < LONG_MIN || dTmp > LONG_MAX)
			return IDS_22_003;
		lTmp = (SLONG)dTmp;
		if (dTmp != lTmp)
			retCode = IDS_01_S07;
		DataPtr = &lTmp;
		DataLen = sizeof(SLONG);
		break;
	case SQL_C_ULONG:
		if (dTmp < 0 || dTmp > ULONG_MAX)
			return IDS_22_003;
		ulTmp = (ULONG)dTmp;
		if (dTmp != ulTmp)
			retCode = IDS_01_S07;
		DataPtr = &ulTmp;
		DataLen = sizeof(ULONG);
		break;

	case SQL_C_SBIGINT:
		if (dTmp < LLONG_MIN || dTmp > LLONG_MAX)
			return IDS_22_003;
		tempVal64 = dTmp;
		DataPtr = &tempVal64;
		DataLen = sizeof(__int64);
		break;
	case SQL_C_UBIGINT:
		if (dTmp < 0 || dTmp > ULLONG_MAX)
			return IDS_22_003;
		utempVal64 = dTmp;
		DataPtr = &utempVal64;
		DataLen = sizeof(unsigned __int64);
		break;

	case SQL_C_FLOAT:
		if (dTmp < -FLT_MAX || dTmp > FLT_MAX)
			return IDS_22_003;
		fltTmp = (FLOAT)dTmp;
		DataPtr = &fltTmp;
		DataLen = sizeof(FLOAT);
		break;
	case SQL_C_DOUBLE:
		DataPtr = &dTmp;
		DataLen = sizeof(DOUBLE);
		break;

	default:
		return IDS_07_006;
	}

	memcpy(targetDataPtr, DataPtr, DataLen);

	if (targetStrLenPtr)
		*targetStrLenPtr = DataLen;

	return retCode;
}

unsigned long ODBC::ConvSQLNumberToNumber(SQLPOINTER srcDataPtr, CDescRec* srcDescPtr, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN *targetStrLenPtr)
{
	unsigned long retCode = SQL_SUCCESS;

	double	dTmp = 0;
	char cTmpBuf[NUM_LEN_MAX] = { 0 };
	SQLINTEGER	DecimalPoint = 0;
	__int64		tempVal64 = 0;
	char		*stopStr = NULL;

	SQLSMALLINT srcUnsigned = srcDescPtr->m_DescUnsigned;
	SQLSMALLINT SQLDataType = srcDescPtr->m_SQLDataType;
	SQLSMALLINT srcScale	= srcDescPtr->m_ODBCScale;

	switch (srcDescPtr->m_ODBCDataType)
	{
	case SQL_TINYINT:
		if (srcUnsigned)
		{
			dTmp = *((UCHAR *)srcDataPtr);
		}
		else
		{
			dTmp = *((SCHAR *)srcDataPtr);
		}
		break;
	case SQL_SMALLINT:
		if (srcUnsigned)
		{
			dTmp = *(USHORT *)srcDataPtr;
		}
		else
		{
			dTmp = *(SSHORT *)srcDataPtr;
		}
		break;
	case SQL_INTEGER:
		if (srcUnsigned)
		{
			dTmp = *(ULONG *)srcDataPtr;
		}
		else
		{
			dTmp = *(SLONG *)srcDataPtr;
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
				dTmp) != SQL_SUCCESS)
				return IDS_07_006;
		}
		else
			dTmp = *(SDOUBLE *)srcDataPtr;
		break;
	case SQL_DECIMAL:
		if (ConvertDecimalToChar(SQLDataType, srcDataPtr, srcLength, srcScale,
			cTmpBuf, DecimalPoint) != SQL_SUCCESS)
			return IDS_07_006;
		if (!ctoi64((char*)cTmpBuf, tempVal64))
			return IDS_22_003;
		dTmp = strtod(cTmpBuf, &stopStr);
		break;
	}

	retCode = ConvertDoubleToNumber(dTmp, CDataType, targetDataPtr, targetLength, targetStrLenPtr);

	return retCode;
}

unsigned long ODBC::ConvSQLBigintToNumber(SQLPOINTER srcDataPtr, SQLSMALLINT srcUnsigned, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN *targetStrLenPtr)
{
	unsigned long retCode = SQL_SUCCESS;

	CHAR	tTmp	= 0;
	UCHAR	utTmp	= 0;
	SHORT	sTmp	= 0;
	USHORT	usTmp	= 0;
	SLONG	lTmp	= 0;
	ULONG	ulTmp	= 0;
	__int64	tempVal64 = 0;
	unsigned __int64 utempVal64 = 0;
	FLOAT	fltTmp	= 0;
	DOUBLE	dTmp	= 0;

	SQLPOINTER DataPtr = NULL;
	SQLLEN	DataLen = 0;

	if (srcUnsigned)
	{
		utempVal64 = *(unsigned __int64 *)srcDataPtr;
		switch (CDataType)
		{
		case SQL_C_BIT:
			if (utempVal64 == 0 || utempVal64 == 1)
				utTmp = (UCHAR)utempVal64;
			else if (utempVal64 > 0 && utempVal64 < 2)
			{
				utTmp = (UCHAR)utempVal64;
				retCode = IDS_01_S07;
			}
			else
				retCode = IDS_22_003;
			DataPtr = &utTmp;
			DataLen = sizeof(UCHAR);
			break;

		case SQL_C_TINYINT:
		case SQL_C_STINYINT:
			if (utempVal64 > SCHAR_MAX)
				return IDS_22_003;
			tTmp = (SCHAR)utempVal64;
			if (utempVal64 != tTmp)
				retCode = IDS_01_S07;
			DataPtr = &tTmp;
			DataLen = sizeof(SCHAR);
			break;
		case SQL_C_UTINYINT:
			if (utempVal64 > UCHAR_MAX)
				return IDS_22_003;
			utTmp = (UCHAR)utempVal64;
			if (utempVal64 != utTmp)
				retCode = IDS_01_S07;
			DataPtr = &utTmp;
			DataLen = sizeof(UCHAR);
			break;

		case SQL_C_SHORT:
		case SQL_C_SSHORT:
			if (utempVal64 > SHRT_MAX)
				return IDS_22_003;
			sTmp = (SHORT)utempVal64;
			if (utempVal64 != sTmp)
				return IDS_01_S07;
			DataPtr = &sTmp;
			DataLen = sizeof(SHORT);
			break;
		case SQL_C_USHORT:
			if (utempVal64 > USHRT_MAX)
				return IDS_22_003;
			usTmp = (USHORT)utempVal64;
			if (utempVal64 != usTmp)
				return IDS_01_S07;
			DataPtr = &usTmp;
			DataLen = sizeof(USHORT);
			break;

		case SQL_C_LONG:
		case SQL_C_SLONG:
			if (utempVal64 > LONG_MAX)
				return IDS_22_003;
			lTmp = (SLONG)utempVal64;
			if (utempVal64 != lTmp)
				retCode = IDS_01_S07;
			DataPtr = &lTmp;
			DataLen = sizeof(SLONG);
			break;
		case SQL_C_ULONG:
			if (utempVal64 > ULONG_MAX)
				return IDS_22_003;
			ulTmp = (ULONG)utempVal64;
			if (utempVal64 != ulTmp)
				retCode = IDS_01_S07;
			DataPtr = &ulTmp;
			DataLen = sizeof(ULONG);
			break;

		case SQL_C_SBIGINT:
			if (utempVal64 > LLONG_MAX)
				return IDS_22_003;
			tempVal64 = (__int64)utempVal64;
			DataPtr = &tempVal64;
			DataLen = sizeof(__int64);
			break;
		case SQL_C_UBIGINT:
			DataPtr = &utempVal64;
			DataLen = sizeof(unsigned __int64);
			break;

		case SQL_C_FLOAT:
			fltTmp = (FLOAT)utempVal64;
			if (utempVal64 != fltTmp)
				retCode = IDS_01_S07;
			DataPtr = &fltTmp;
			DataLen = sizeof(FLOAT);
			break;
		case SQL_C_DOUBLE:
			dTmp = (DOUBLE)utempVal64;
			if (utempVal64 != dTmp)
				retCode = IDS_01_S07;
			DataPtr = &dTmp;
			DataLen = sizeof(DOUBLE);
			break;

		default:
			return IDS_07_006;
		}
	}
	else
	{
		tempVal64 = *(__int64 *)srcDataPtr;
		switch (CDataType)
		{
		case SQL_C_BIT:
			if (tempVal64 == 0 || tempVal64 == 1)
				utTmp = (UCHAR)tempVal64;
			else if (tempVal64 > 0 && tempVal64 < 2)
			{
				utTmp = (UCHAR)tempVal64;
				retCode = IDS_01_S07;
			}
			else
				retCode = IDS_22_003;
			DataPtr = &utTmp;
			DataLen = sizeof(UCHAR);
			break;

		case SQL_C_TINYINT:
		case SQL_C_STINYINT:
			if (tempVal64 < SCHAR_MIN || tempVal64 > SCHAR_MAX)
				return IDS_22_003;
			tTmp = (SCHAR)tempVal64;
			if (tempVal64 != tTmp)
				retCode = IDS_01_S07;
			DataPtr = &tTmp;
			DataLen = sizeof(SCHAR);
			break;
		case SQL_C_UTINYINT:
			if (tempVal64 < 0 || tempVal64 > UCHAR_MAX)
				return IDS_22_003;
			utTmp = (UCHAR)tempVal64;
			if (tempVal64 != utTmp)
				retCode = IDS_01_S07;
			DataPtr = &utTmp;
			DataLen = sizeof(UCHAR);
			break;

		case SQL_C_SHORT:
		case SQL_C_SSHORT:
			if (tempVal64 < SHRT_MIN || tempVal64 > SHRT_MAX)
				return IDS_22_003;
			sTmp = (SHORT)tempVal64;
			if (tempVal64 != sTmp)
				return IDS_01_S07;
			DataPtr = &sTmp;
			DataLen = sizeof(SHORT);
			break;
		case SQL_C_USHORT:
			if (tempVal64 < 0 || tempVal64 > USHRT_MAX)
				return IDS_22_003;
			usTmp = (USHORT)tempVal64;
			if (tempVal64 != usTmp)
				return IDS_01_S07;
			DataPtr = &usTmp;
			DataLen = sizeof(USHORT);
			break;

		case SQL_C_LONG:
		case SQL_C_SLONG:
			if (tempVal64 < LONG_MIN || tempVal64 > LONG_MAX)
				return IDS_22_003;
			lTmp = (SLONG)tempVal64;
			if (tempVal64 != lTmp)
				retCode = IDS_01_S07;
			DataPtr = &lTmp;
			DataLen = sizeof(SLONG);
			break;
		case SQL_C_ULONG:
			if (tempVal64 < 0 || tempVal64 > ULONG_MAX)
				return IDS_22_003;
			ulTmp = (ULONG)tempVal64;
			if (tempVal64 != ulTmp)
				retCode = IDS_01_S07;
			DataPtr = &ulTmp;
			DataLen = sizeof(ULONG);
			break;

		case SQL_C_SBIGINT:
			DataPtr = &tempVal64;
			DataLen = sizeof(__int64);
			break;
		case SQL_C_UBIGINT:
			if (tempVal64 < 0 )
				return IDS_22_003;
			utempVal64 = (unsigned __int64)tempVal64;
			DataPtr = &utempVal64;
			DataLen = sizeof(unsigned __int64);
			break;

		case SQL_C_FLOAT:
			fltTmp = (FLOAT)tempVal64;
			if (tempVal64 != fltTmp)
				retCode = IDS_01_S07;
			DataPtr = &fltTmp;
			DataLen = sizeof(FLOAT);
			break;
		case SQL_C_DOUBLE:
			dTmp = (DOUBLE)tempVal64;
			if (tempVal64 != dTmp)
				retCode = IDS_01_S07;
			DataPtr = &dTmp;
			DataLen = sizeof(DOUBLE);
			break;

		default:
			return IDS_07_006;
		}
	}

	memcpy(targetDataPtr, DataPtr, DataLen);

	if (targetStrLenPtr)
		*targetStrLenPtr = DataLen;

	return retCode;
}

unsigned long ODBC::ConvSQLNumericToNumber(SQLPOINTER srcDataPtr, CDescRec *srcDescPtr, SQLINTEGER srcLength, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr, SQLINTEGER targetLength, SQLLEN *targetStrLenPtr)
{
	unsigned long retCode = SQL_SUCCESS;

	DOUBLE	dTmp = 0;
	__int64 tempVal64 = 0;
	unsigned __int64 utempVal64 = 0;
	char *stopStr = NULL;

	int i = 0;
	__int64 power = 0;
	__int64 tempValFrac = 0;
	unsigned __int64 utempValFrac = 0;
	char cTmpBuf[NUM_LEN_MAX] = { 0 };

	SQLSMALLINT SQLDataType		= srcDescPtr->m_SQLDataType;
	SQLSMALLINT srcPrecision	= srcDescPtr->m_ODBCPrecision;
	SQLSMALLINT	srcScale		= srcDescPtr->m_ODBCScale;

	switch (SQLDataType)
	{
	case SQLTYPECODE_TINYINT:
		dTmp = *((SCHAR *)srcDataPtr);
		if (srcScale > 0)
			dTmp = dTmp / (long)pow(10, srcScale);
		break;
	case SQLTYPECODE_TINYINT_UNSIGNED:
		dTmp = *((UCHAR *)srcDataPtr);
		if (srcScale > 0)
			dTmp = dTmp / (long)pow(10, srcScale);
		break;

	case SQLTYPECODE_SMALLINT:
		dTmp = *((SHORT *)srcDataPtr);
		if (srcScale > 0)
			dTmp = dTmp / (long)pow(10, srcScale);
		break;
	case SQLTYPECODE_SMALLINT_UNSIGNED:
		dTmp = *((USHORT *)srcDataPtr);
		if (srcScale > 0)
			dTmp = dTmp / (long)pow(10, srcScale);
		break;

	case SQLTYPECODE_INTEGER:
		dTmp = *((LONG *)srcDataPtr);
		if (srcScale > 0)
			dTmp = dTmp / (long)pow(10, srcScale);
		break;
	case SQLTYPECODE_INTEGER_UNSIGNED:
		dTmp = *((ULONG *)srcDataPtr);
		if (srcScale > 0)
			dTmp = dTmp / (long)pow(10, srcScale);
		break;

	case SQLTYPECODE_LARGEINT:
		tempVal64 = *((__int64 *)srcDataPtr);
		for (i = 0, power = 1; i < srcScale; power *= 10, i++);
		tempValFrac = tempVal64 % power;
		tempVal64 = tempVal64 / power;
		dTmp = ((double)tempValFrac / (double)power);
		dTmp = dTmp + tempVal64;
		break;
	case SQLTYPECODE_LARGEINT_UNSIGNED:
		utempVal64 = *((unsigned __int64 *)srcDataPtr);
		for (i = 0, power = 1; i < srcScale; power *= 10, i++);
		utempValFrac = utempVal64 % power;
		utempVal64 = utempVal64 / power;
		dTmp = ((double)utempValFrac / (double)power);
		dTmp = dTmp + utempVal64;
		break;

	case SQLTYPECODE_NUMERIC:
	case SQLTYPECODE_NUMERIC_UNSIGNED:
		if (((SQLDataType == SQLTYPECODE_NUMERIC) && (srcPrecision > 18)) ||
			((SQLDataType == SQLTYPECODE_NUMERIC_UNSIGNED) && (srcPrecision > 9))) //for bignum support
		{
			retCode = BigNum_To_Ascii_Helper((char*)srcDataPtr, srcLength, srcPrecision, srcScale, cTmpBuf, SQLDataType);
			if (retCode != SQL_SUCCESS)
				return retCode;

		}
		else
			return IDS_07_006;

		dTmp = strtod(cTmpBuf, &stopStr);
		if (errno == ERANGE)
			return IDS_22_003;

		break;

	default:
		return IDS_HY_000;
	}
	
	retCode = ConvertDoubleToNumber(dTmp, CDataType, targetDataPtr, targetLength, targetStrLenPtr);

	return retCode;
}

unsigned long ODBC::GetCTmpBufFromSQLChar(SQLPOINTER srcDataPtr, CDescRec* srcDescPtr, SQLINTEGER srcLength, bool isShort, char *&cTmpBuf, SQLINTEGER *tmpLen, bool RemoveSpace)
{
	int TransStringLength = 0;
	char error[64] = {0};
	SQLLEN DataLen = 0;

	SQLSMALLINT ODBCDataType = srcDescPtr->m_ODBCDataType;
	SQLSMALLINT srcCharSet = srcDescPtr->m_SQLCharset;

	if (ODBCDataType == SQL_CHAR || ODBCDataType == SQL_WCHAR)
	{
		if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to UTF-8
		{
			wchar_t* spaceStart = wcschr((wchar_t*)srcDataPtr, L' ');
			if (spaceStart != NULL && RemoveSpace)
				srcLength = (spaceStart - (wchar_t*)srcDataPtr) + 1;
			cTmpBuf = (char *)malloc(srcLength * 2);
			memset(cTmpBuf, 0, srcLength * 2);
			if (cTmpBuf == NULL) //Avoid a seg-violation
				return IDS_07_003;
			if (WCharToUTF8((wchar_t*)srcDataPtr, srcLength - 1, (char*)cTmpBuf,
				srcLength * 2, &TransStringLength, error) != SQL_SUCCESS)
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
			if (spaceStart != NULL && RemoveSpace)
				srcLength = (spaceStart - (char*)srcDataPtr) + 1;
			cTmpBuf = (char *)malloc(srcLength);
			memset(cTmpBuf, 0, srcLength);
			if (cTmpBuf == NULL) //Avoid a seg-violation
				return IDS_07_003;
			strncpy(cTmpBuf, (char*)srcDataPtr, srcLength - 1);
			cTmpBuf[srcLength - 1] = 0;
		}
	}
	else //SQL_LONG_VARCHAR
	{
		if (isShort)
		{
			DataLen = *(USHORT *)srcDataPtr;
			if (DataLen == 0)
				return SQL_NO_DATA;

			if (srcCharSet == SQLCHARSETCODE_UCS2) //convert from UTF-16 to UTF-8
			{
				cTmpBuf = (char *)malloc(DataLen * 2 + 1);
				memset(cTmpBuf, 0, DataLen * 2 + 1);
				if (cTmpBuf == NULL) //Avoid a seg-violation
					return IDS_07_003;
				if (WCharToUTF8((wchar_t*)srcDataPtr + 1, DataLen / 2, (char*)cTmpBuf,
					DataLen * 2 + 1, &TransStringLength, error) != SQL_SUCCESS)
				{
					//We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
					return IDS_22_003;
				}
				srcLength = TransStringLength + 1;
			}
			else
			{
				cTmpBuf = (char *)malloc(DataLen + 1);
				memset(cTmpBuf, 0, DataLen + 1);
				if (cTmpBuf == NULL) //Avoid a seg-violation
					return IDS_07_003;
				memcpy(cTmpBuf, (char*)srcDataPtr + 2, DataLen);
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
				cTmpBuf = (char *)malloc(DataLen * 2 + 1);
				memset(cTmpBuf, 0, DataLen * 2 + 1);
				if (cTmpBuf == NULL) //Avoid a seg-violation
					return IDS_07_003;
				if (WCharToUTF8((wchar_t*)srcDataPtr + 2, DataLen / 2, (char*)cTmpBuf,
					DataLen * 2 + 1, &TransStringLength, error) != SQL_SUCCESS)
				{
					//We don't want to see a data truncation (SQL_SUCCESS_WITH_INFO) here
					return IDS_22_003;
				}
				srcLength = TransStringLength + 1;
			}
			else
			{
				cTmpBuf = (char *)malloc(DataLen + 1);
				memset(cTmpBuf, 0, DataLen + 1);
				if (cTmpBuf == NULL) //Avoid a seg-violation
					return IDS_07_003;
				memcpy(cTmpBuf, (char*)srcDataPtr + 4, DataLen);
				srcLength = DataLen + 1;
				cTmpBuf[DataLen] = 0;
			}
		}
	}

	if (tmpLen)
		*tmpLen = srcLength;

	return SQL_SUCCESS;
}

unsigned long ODBC::ConvDoubleToInterval(DOUBLE dTmp, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr)
{
	unsigned long retCode = SQL_SUCCESS;

	SQL_INTERVAL_STRUCT intervalTmp;
	memset(&intervalTmp, 0, sizeof(SQL_INTERVAL_STRUCT));
	ULONG ulTmp = 0;

	if (dTmp < 0)
		intervalTmp.interval_sign = 1;
	ulTmp = (ULONG)dTmp;
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

	return retCode;
}

unsigned long ODBC::ConvSQLDateToDate(SQLPOINTER srcDataPtr, SQLSMALLINT SQLDatetimeCode, BOOL ColumnwiseData, SQLPOINTER targetDataPtr)
{
	DATE_TYPES DateTmp = { 0, 0, 0 };
	DATE_TYPES *SQLDate = NULL;
	SQL_DATE_STRUCT *datePtr = (SQL_DATE_STRUCT *)targetDataPtr;

	if (ColumnwiseData) // !RowwiseRowSet
	{
		SQLDate = &DateTmp;
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

	datePtr->year = SQLDate->year;
	datePtr->month = SQLDate->month;
	datePtr->day = SQLDate->day;

	return SQL_SUCCESS;
}

unsigned long ODBC::ConvSQLTimeToTime(SQLPOINTER srcDataPtr, SQLSMALLINT SQLDatetimeCode, BOOL ColumnwiseData, SQLPOINTER targetDataPtr)
{
	TIME_TYPES TimeTmp = {0, 0, 0, 0};
	TIME_TYPES *SQLTime = NULL;
	SQL_TIME_STRUCT *timePtr = (SQL_TIME_STRUCT *)targetDataPtr;

	if (ColumnwiseData) //!RowwiseRowSet 
	{
		SQLTime = &TimeTmp;
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

	timePtr->hour = SQLTime->hour;
	timePtr->minute = SQLTime->minute;
	timePtr->second = SQLTime->second;

	return SQL_SUCCESS;
}

unsigned long ODBC::ConvSQLDateToTimestamp(SQLPOINTER srcDataPtr, SQLSMALLINT SQLDatetimeCode, BOOL ColumnwiseData, SQLPOINTER targetDataPtr)
{
	DATE_TYPES *SQLDate = NULL;
	DATE_TYPES SQLDateTmp = { 0, 0, 0 };
	SQL_TIMESTAMP_STRUCT timestampTmp;

	if (ColumnwiseData) //!RowwiseRowSet
	{
		SQLDate = &SQLDateTmp;
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
	
	memcpy(targetDataPtr, &timestampTmp, sizeof(SQL_TIMESTAMP_STRUCT));

	return SQL_SUCCESS;
}

unsigned long ODBC::ConvSQLTimeToTimestamp(SQLPOINTER srcDataPtr, SQLSMALLINT SQLDatetimeCode, BOOL ColumnwiseData, SQLPOINTER targetDataPtr)
{
	TIME_TYPES *SQLTime = NULL;
	TIME_TYPES SQLTimeTmp = { 0, 0, 0, 0};
	SQL_TIMESTAMP_STRUCT timestampTmp;

	struct tm *newtime;
	time_t long_time;

	if (ColumnwiseData) //!RowwiseRowSet
	{
		SQLTime = &SQLTimeTmp;
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

	time(&long_time);					/* Get time as long integer. */
	newtime = localtime(&long_time);	/* Convert to local time. */
	timestampTmp.year = (short)(newtime->tm_year + 1900);
	timestampTmp.month = (unsigned short)(newtime->tm_mon + 1);
	timestampTmp.day = (unsigned short)newtime->tm_mday;
	timestampTmp.hour = SQLTime->hour;
	timestampTmp.minute = SQLTime->minute;
	timestampTmp.second = SQLTime->second;
	timestampTmp.fraction = 0;

	memcpy(targetDataPtr, &timestampTmp, sizeof(SQL_TIMESTAMP_STRUCT));

	return SQL_SUCCESS;
}

unsigned long ODBC::ConvSQLTimestampToDateTime(SQLPOINTER srcDataPtr, SQLSMALLINT SQLDatetimeCode, SQLSMALLINT srcPrecision, BOOL ColumnwiseData, SQLSMALLINT CDataType, SQLPOINTER targetDataPtr)
{
	unsigned long retCode	= SQL_SUCCESS;

	TIMESTAMP_TYPES SQLTimestampTmp;
	memset(&SQLTimestampTmp, 0, sizeof(TIMESTAMP_TYPES));
	TIMESTAMP_TYPES *SQLTimestamp = &SQLTimestampTmp;
	ULONG	ulFraction		= 0;
	char	sFraction[16]	= { 0 };
	DOUBLE  dTmp			= 0;

	SQL_DATE_STRUCT dateTmp = {0, 0, 0};
	SQL_TIME_STRUCT timeTmp = {0, 0, 0};
	SQL_TIMESTAMP_STRUCT timestampTmp = {0, 0, 0, 0, 0, 0, 0};
	SQLPOINTER DataPtr = NULL;
	SQLINTEGER DataLen = 0;

	if (ColumnwiseData) //!RowwiseRowSet
	{
		SQLTimestamp->year = 0;
		SQLTimestamp->month = 0;
		SQLTimestamp->day = 0;
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
				sprintf(sFraction, "%0*lu000000000", 6, ulFraction);
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
			dTmp = (*(UDWORD*)SQLTimestamp->fraction *  1000000000.0) / pow(10, srcPrecision);
			ulFraction = dTmp;
		}
		else
			ulFraction = 0;
	}

	switch (CDataType)
	{
	case SQL_C_DATE:
	case SQL_C_TYPE_DATE:
		if (SQLTimestamp->hour != 0 || SQLTimestamp->minute != 0 || SQLTimestamp->second != 0 || ulFraction != 0)
			retCode = IDS_01_S07;
		dateTmp.year = SQLTimestamp->year;
		dateTmp.month = SQLTimestamp->month;
		dateTmp.day = SQLTimestamp->day;
		DataPtr = &dateTmp;
		DataLen = sizeof(SQL_DATE_STRUCT);
		break;
	case SQL_C_TIME:
	case SQL_C_TYPE_TIME:
		if (SQLTimestamp->year != 0 || SQLTimestamp->month != 0 || SQLTimestamp->day != 0)
			retCode = IDS_01_S07;
		timeTmp.hour = SQLTimestamp->hour;
		timeTmp.minute = SQLTimestamp->minute;
		timeTmp.second = SQLTimestamp->second;
		DataPtr = &timeTmp;
		DataLen = sizeof(SQL_TIME_STRUCT);
		break;
	case SQL_C_TIMESTAMP:
	case SQL_C_TYPE_TIMESTAMP:
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
		DataPtr = &timestampTmp;
		DataLen = sizeof(SQL_TIMESTAMP_STRUCT);
		break;
	}

	memcpy(targetDataPtr, DataPtr, DataLen);

	return retCode;
}

unsigned long ODBC::ConvSQLCharToNumeric(SQLPOINTER srcDataPtr, CDescRec *srcDescPtr, SQLINTEGER srcLength, SQLPOINTER targetDataPtr)
{
	unsigned long retCode = SQL_SUCCESS;

	char		*cTmpBuf = NULL;

	char*		DataPtr	= NULL;
	SQLLEN		DataLen	= 0;

	SQL_NUMERIC_STRUCT numericTmp;

	SQLSMALLINT ODBCDataType = srcDescPtr->m_ODBCDataType;

	switch (ODBCDataType)
	{
	case SQL_CHAR:
	case SQL_WCHAR:
	case SQL_VARCHAR:
	case SQL_LONGVARCHAR:
	case SQL_WVARCHAR:
		retCode = GetCTmpBufFromSQLChar(srcDataPtr, srcDescPtr, srcLength, srcDescPtr->m_SQLMaxLength <= 32767, cTmpBuf, NULL);
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
	{
		cTmpBuf = (char *)malloc(NUM_LEN_MAX);
		trimInterval((char*)srcDataPtr);
		DataLen = strlen(DataPtr);
		if (sizeof(cTmpBuf) < DataLen)
			return IDS_07_003;
		strncpy(cTmpBuf, DataPtr, DataLen);
		DataPtr = cTmpBuf;
	}
		break;
	}

	if (retCode != SQL_SUCCESS)
		retCode = ConvertCharToCNumeric(numericTmp, cTmpBuf);

	if (cTmpBuf != NULL)
	{
		free(cTmpBuf);
		cTmpBuf = NULL;
	}

	memcpy(targetDataPtr, &numericTmp, sizeof(SQL_NUMERIC_STRUCT));

	return retCode;
}
