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
// MODULE: sqlInterface.cpp
//
// PURPOSE: Implements the Srvr interface to SQL
//
// MODIFICATION: Adds the implementation of resource governing features (5/12/98)
//
//
#include <new>

#include <limits.h>
#include <platform_ndcs.h>
#include <sql.h>
#include <sqlext.h>
#include "odbcCommon.h"
#include "DrvrSrvr.h"
#include "odbc_sv.h"
#include "srvrcommon.h"
#include "srvrkds.h"
#include "sqlinterface.h"
#include "SQLWrapper.h"

#include "CommonDiags.h"  
#include "tdm_odbcSrvrMsg.h"
#include "TransportBase.h"
#include "ResStatisticsStatement.h"

extern Int32 inState;
extern double inEstimatedCost;
extern ResStatisticsStatement *resStatStatement;

void DeallocateAdaptiveSegment(SRVR_STMT_HDL *pSrvrStmt);

#define FETCH2MEMORYSIZE 4194304 // ONE MB

using namespace SRVR; 


extern "C" Int32 getGlobalBufferLength();

extern "C" BYTE* getGlobalBuffer();

extern "C" BYTE* allocGlobalBuffer(Int32 size);

SQLRETURN SRVR::GetODBCValues(Int32 DataType, Int32 DateTimeCode, Int32 &Length, Int32 Precision, 
						Int32 &ODBCDataType, Int32 &ODBCPrecision, BOOL &SignType, 
						Int32 Nullable, Int32 &totalMemLen, Int32 SQLCharset, Int32 &ODBCCharset,
						Int32 IntLeadPrec, char *ColHeading)
{
	SRVRTRACE_ENTER(FILE_INTF+1);

	bool bRWRS = false;
	if (srvrGlobal->drvrVersion.buildId & ROWWISE_ROWSET)
		bRWRS = true;

	ODBCCharset = SQLCHARSETCODE_ISO88591;

	if (bRWRS)
	{
		if (Nullable)
		{
			totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1; 
			totalMemLen += 2 ;
		}
	}

	switch (DataType)
	{
                case SQLTYPECODE_BOOLEAN:
			ODBCPrecision = 1;
			ODBCDataType = SQL_CHAR;
			SignType = FALSE;
			totalMemLen += Length;
			break;
		case SQLTYPECODE_CHAR:
			ODBCPrecision = Length;
			ODBCDataType = SQL_CHAR;
			SignType = FALSE;
			totalMemLen += Length;
			if (!bRWRS)
				totalMemLen += 1;
			ODBCCharset = SQLCharset;
			break;
		case SQLTYPECODE_VARCHAR:
			ODBCPrecision = Length;
			ODBCDataType = SQL_VARCHAR;
			if(srvrGlobal->enableLongVarchar)
			{
			if (Length >= 255)	// we have to fix the data type to comply with ODBC def since SQL does not return this type
				ODBCDataType = SQL_LONGVARCHAR;
			}
			SignType = FALSE;
			totalMemLen += Length;
			if (!bRWRS)
				totalMemLen += 1;
			ODBCCharset = SQLCharset;
			break;
		case SQLTYPECODE_VARCHAR_WITH_LENGTH:
			ODBCPrecision = Length;
			ODBCDataType = SQL_VARCHAR;
			if(srvrGlobal->enableLongVarchar)
			{
			if (Length >= 255)	// we have to fix the data type to comply with ODBC def since SQL does not return this type
				ODBCDataType = SQL_LONGVARCHAR;
			}
			SignType = FALSE;
			// Varchar indicator length can 2 or 4 bytes depending on length of the column
			if (Length > SHRT_MAX)	// 32767
			{
				totalMemLen = ((totalMemLen + 4 - 1) >> 2) << 2; 
				totalMemLen += Length + 4;
			}
			else
			{
				totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1; 
				totalMemLen += Length + 2;
			}

			if (!bRWRS)
				totalMemLen += 1;
			ODBCCharset = SQLCharset;
			break;
		case SQLTYPECODE_VARCHAR_LONG:
			ODBCPrecision = Length;
			ODBCDataType = SQL_LONGVARCHAR;
			SignType = FALSE;
			totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1; 
			totalMemLen += Length + 2;
			if (!bRWRS)
				totalMemLen += 1;
			ODBCCharset = SQLCharset;
			break;

		case SQLTYPECODE_BLOB:
			ODBCPrecision = Length;
			ODBCDataType = TYPE_BLOB;
			SignType = FALSE;
			// Varchar indicator length can 2 or 4 bytes depending on length of the column
			if (Length > SHRT_MAX)	// 32767
			{
				totalMemLen = ((totalMemLen + 4 - 1) >> 2) << 2;
				totalMemLen += Length + 4;
			}
			else
			{
				totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1;
				totalMemLen += Length + 2;
			}

			if (!bRWRS)
				totalMemLen += 1;
			ODBCCharset = SQLCharset;
			break;

		case SQLTYPECODE_CLOB:
			ODBCPrecision = Length;
			ODBCDataType = TYPE_CLOB;
			SignType = FALSE;
			// Varchar indicator length can 2 or 4 bytes depending on length of the column
			if (Length > SHRT_MAX)	// 32767
			{
				totalMemLen = ((totalMemLen + 4 - 1) >> 2) << 2;
				totalMemLen += Length + 4;
			}
			else
			{
				totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1;
				totalMemLen += Length + 2;
			}

			if (!bRWRS)
				totalMemLen += 1;
			ODBCCharset = SQLCharset;
			break;

		case SQLTYPECODE_BINARY:
			ODBCPrecision = Length;
			ODBCDataType = SQL_BINARY;
			SignType = FALSE;
			totalMemLen += Length;
			if (!bRWRS)
				totalMemLen += 1;
			ODBCCharset = SQLCharset;
			break;
		case SQLTYPECODE_VARBINARY:
			ODBCPrecision = Length;
			ODBCDataType = SQL_VARBINARY;
			SignType = FALSE;
			// Varchar indicator length can 2 or 4 bytes depending on length of the column
			if (Length > SHRT_MAX)	// 32767
			{
				totalMemLen = ((totalMemLen + 4 - 1) >> 2) << 2; 
				totalMemLen += Length + 4;
			}
			else
			{
				totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1; 
				totalMemLen += Length + 2;
			}

			if (!bRWRS)
				totalMemLen += 1;
			ODBCCharset = SQLCharset;
			break;

                case SQLTYPECODE_TINYINT:
			ODBCPrecision = 3;
			ODBCDataType = SQL_TINYINT;
			SignType = TRUE;
			totalMemLen += Length;
			break;

                case SQLTYPECODE_TINYINT_UNSIGNED:
			ODBCPrecision = 3;
			ODBCDataType = SQL_TINYINT;
			SignType = FALSE;
			totalMemLen += Length;
			break;

		case SQLTYPECODE_SMALLINT:
			if (Precision == 0)
			{
				ODBCPrecision = 5;
				ODBCDataType = SQL_SMALLINT;
			}
			else
			{
				ODBCPrecision = Precision;
				ODBCDataType = SQL_NUMERIC;
			}
			SignType = TRUE;
			totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1; 
			totalMemLen += Length ;
			break;
		case SQLTYPECODE_SMALLINT_UNSIGNED:
			if (Precision == 0)
			{
				if (srvrGlobal->appVersion.majorVersion == 2)
				{
					ODBCPrecision = 10;
					ODBCDataType = SQL_INTEGER;
					SignType = TRUE;
				}
				else
				{
					if ((srvrGlobal->EnvironmentType & MXO_MSACCESS_1997) || (srvrGlobal->EnvironmentType & MXO_MSACCESS_2000))
					{
						ODBCPrecision = 10;
						ODBCDataType = SQL_INTEGER;
						SignType = TRUE;
					}
					else
					{
						ODBCPrecision = 5;
						ODBCDataType = SQL_SMALLINT;
						SignType = FALSE;
					}
				}
			}
			else
			{
				ODBCPrecision = Precision;
				ODBCDataType = SQL_NUMERIC;
				SignType = FALSE;
			}
			totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1; 
			totalMemLen += Length ;
			break;
		case SQLTYPECODE_INTEGER:
			if (Precision == 0)
			{
				ODBCPrecision = 10;
				ODBCDataType = SQL_INTEGER;
			}
			else
			{
				ODBCPrecision = Precision;
				ODBCDataType = SQL_NUMERIC;
			}
			SignType = TRUE;
			totalMemLen = ((totalMemLen + 4 - 1) >> 2) << 2; 
			totalMemLen += Length ;
			break;
		case SQLTYPECODE_INTEGER_UNSIGNED:
			if (Precision == 0)
			{
				if (srvrGlobal->appVersion.majorVersion == 2)
				{
					ODBCPrecision = 19;
					if (srvrGlobal->EnvironmentType & MXO_BIGINT_NUMERIC)
						ODBCDataType = SQL_NUMERIC;
					else
						ODBCDataType = SQL_BIGINT;
					SignType = TRUE;
				}
				else
				{
					if ((srvrGlobal->EnvironmentType & MXO_MSACCESS_1997) || (srvrGlobal->EnvironmentType & MXO_MSACCESS_2000))
					{
						ODBCPrecision = 19;
						if (srvrGlobal->EnvironmentType & MXO_BIGINT_NUMERIC)
							ODBCDataType = SQL_NUMERIC;
						else
							ODBCDataType = SQL_BIGINT;
						SignType = TRUE;
					}
					else
					{
						ODBCPrecision = 10;
						ODBCDataType = SQL_INTEGER;
						SignType = FALSE;
					}
				}
			}
			else
			{
				ODBCPrecision = Precision;
				ODBCDataType = SQL_NUMERIC;
				SignType = FALSE;
			}
			totalMemLen = ((totalMemLen + 4 - 1) >> 2) << 2; 
			totalMemLen += Length ;
			break;
		case SQLTYPECODE_LARGEINT:
			if (Precision == 0)
			{
				ODBCPrecision = 19;
				if (srvrGlobal->EnvironmentType & MXO_BIGINT_NUMERIC)
					ODBCDataType = SQL_NUMERIC;
				else
					ODBCDataType = SQL_BIGINT;
			}
			else
			{
				ODBCPrecision = Precision;
				ODBCDataType = SQL_NUMERIC;
			}
			SignType = TRUE;
			totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3; 
			totalMemLen += Length ;
			break;		
		case SQLTYPECODE_LARGEINT_UNSIGNED:
			if (Precision == 0)
			{
				ODBCPrecision = 19;
				if (srvrGlobal->EnvironmentType & MXO_BIGINT_NUMERIC)
					ODBCDataType = SQL_NUMERIC;
				else
					ODBCDataType = SQL_BIGINT;
			}
			else
			{
				ODBCPrecision = Precision;
				ODBCDataType = SQL_NUMERIC;
			}
			SignType = FALSE;
			totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3; 
			totalMemLen += Length ;
			break;		
		case SQLTYPECODE_IEEE_REAL:
			ODBCDataType = SQL_REAL;
			ODBCPrecision = 7;
			SignType = TRUE;
			//if (bRWRS)
			//	totalMemLen = ((totalMemLen + 4 - 1) >> 2) << 2; 
			//else
				totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3; 
			totalMemLen += Length ;
			break;
		case SQLTYPECODE_IEEE_FLOAT:
			ODBCDataType = SQL_FLOAT;
			ODBCPrecision = 15;
			SignType = TRUE;
			totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3; 
			totalMemLen += Length ;
			break;
		case SQLTYPECODE_IEEE_DOUBLE:
			ODBCDataType = SQL_DOUBLE;
			ODBCPrecision = 15;
			SignType = TRUE;
			totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3; 
			totalMemLen += Length ;
			break;
		case SQLTYPECODE_DATETIME:
	   		switch (DateTimeCode)
			{
				case SQLDTCODE_DATE:					//1
					ODBCDataType = SQL_DATE;
					ODBCPrecision = 10;
					break;
				case SQLDTCODE_TIME:					//2
					if (!bRWRS) //for RWRS
					{
						ODBCDataType = SQL_TIME;
						if (Precision == 0)
							ODBCPrecision = 8;
						else
						{
							ODBCDataType = SQL_TIMESTAMP;
							ODBCPrecision = 20+Precision;
					
						} 
					}
					else
					{
						ODBCDataType = SQL_TIME;
						ODBCPrecision = Precision;
					}
					break;
				case SQLDTCODE_TIMESTAMP:				//3
					ODBCDataType = SQL_TIMESTAMP;
					if (Precision == 0)
						ODBCPrecision = 19;
					else
						ODBCPrecision = 20+Precision;
					break;
//
// Mapping Non-standard SQL DATETIME types to DATE/TIME/TIMESTAMP
//
				case SQLDTCODE_YEAR:					//4
					ODBCDataType = SQL_DATE;
					ODBCPrecision = 10;
					break;
				case SQLDTCODE_YEAR_TO_MONTH:			//5
					ODBCDataType = SQL_DATE;
					ODBCPrecision = 10;
					break;
				case SQLDTCODE_YEAR_TO_HOUR:			//7
					ODBCDataType = SQL_TIMESTAMP;
					ODBCPrecision = 19;
					break;
				case SQLDTCODE_YEAR_TO_MINUTE:			//8
					ODBCDataType = SQL_TIMESTAMP;
					ODBCPrecision = 19;
					break;
				case SQLDTCODE_MONTH:					//10
					ODBCDataType = SQL_DATE;
					ODBCPrecision = 10;
					break;
				case SQLDTCODE_MONTH_TO_DAY:			//11
					ODBCDataType = SQL_DATE;
					ODBCPrecision = 10;
					break;
				case SQLDTCODE_MONTH_TO_HOUR:			//12
					ODBCDataType = SQL_TIMESTAMP;
					ODBCPrecision = 19;
					break;
				case SQLDTCODE_MONTH_TO_MINUTE:			//13
					ODBCDataType = SQL_TIMESTAMP;
					ODBCPrecision = 19;
					break;
				case SQLDTCODE_MONTH_TO_FRACTION:		//14
					ODBCDataType = SQL_TIMESTAMP;
					if (Precision == 0)
						ODBCPrecision = 19;
					else
						ODBCPrecision = 20+Precision;
					break;
				case SQLDTCODE_DAY:						//15
					ODBCDataType = SQL_DATE;
					ODBCPrecision = 10;
					break;
				case SQLDTCODE_DAY_TO_HOUR:				//16
					ODBCDataType = SQL_TIMESTAMP;
					ODBCPrecision = 19;
					break;
				case SQLDTCODE_DAY_TO_MINUTE:			//17
					ODBCDataType = SQL_TIMESTAMP;
					ODBCPrecision = 19;
					break;
				case SQLDTCODE_DAY_TO_FRACTION:			//18
					ODBCDataType = SQL_TIMESTAMP;
					if (Precision == 0)
						ODBCPrecision = 19;
					else
						ODBCPrecision = 20+Precision;
					break;
				case SQLDTCODE_HOUR:					//19
					ODBCDataType = SQL_TIME;
					ODBCPrecision = 8;
					break;
				case SQLDTCODE_HOUR_TO_MINUTE:			//20
					ODBCDataType = SQL_TIME;
					ODBCPrecision = 8;
					break;
				case SQLDTCODE_MINUTE:					//22
					ODBCDataType = SQL_TIME;
					ODBCPrecision = 8;
					break;
				case SQLDTCODE_MINUTE_TO_FRACTION:		//23
					ODBCDataType = SQL_TIME;
					if (Precision == 0)
						ODBCPrecision = 8;
					else
					{
						ODBCDataType = SQL_TIMESTAMP;
						ODBCPrecision = 20+Precision;
					}
					break;
				case SQLDTCODE_SECOND_TO_FRACTION:		//24
					ODBCDataType = SQL_TIME;
					if (Precision == 0)
						ODBCPrecision = 8;
					else
					{
						ODBCDataType = SQL_TIMESTAMP;
						ODBCPrecision = 20+Precision;
					}
					break;
//				case SQLDTCODE_MONTH_TO_SECOND:			14
//				case SQLDTCODE_DAY_TO_SECOND:			18
//				case SQLDTCODE_HOUR_TO_SECOND:			2
//				case SQLDTCODE_HOUR_TO_FRACTION:		2
//				case SQLDTCODE_MINUTE_TO_SECOND:		23
//				case SQLDTCODE_SECOND:					24
				default:
					ODBCDataType = SQL_TYPE_NULL;
					ODBCPrecision = 0;
					break;
			}
			SignType = FALSE;
			if (!bRWRS)
			   totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3; 
			totalMemLen += Length ;
			break; 
		case SQLTYPECODE_DECIMAL_UNSIGNED:
			ODBCPrecision = Length;
			ODBCDataType = SQL_DECIMAL;
			SignType = FALSE;
			totalMemLen += Length;
			break;
		case SQLTYPECODE_DECIMAL:
			ODBCPrecision = Length;
			ODBCDataType = SQL_DECIMAL;
			SignType = TRUE;
			totalMemLen += Length;
			break; 
		case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // extension
			ODBCDataType = SQL_DOUBLE; // Since there is no corresponding ODBC DataType, Map it as a double
			ODBCPrecision = 15;
			SignType = FALSE;
			totalMemLen += Length;
			break;
		case SQLTYPECODE_DECIMAL_LARGE: // extension
			ODBCDataType = SQL_DOUBLE; // Since there is no corresponding ODBC DataType, Map it as a double
			ODBCPrecision = 15;
			SignType = TRUE;
			totalMemLen += Length;
			break;
		case SQLTYPECODE_INTERVAL:		// Interval will be sent in ANSIVARCHAR format
	   		switch (DateTimeCode)
			{
				case SQLINTCODE_YEAR:
					ODBCDataType = SQL_INTERVAL_YEAR;
					ODBCPrecision = 0;
					break;
				case SQLINTCODE_MONTH:
					ODBCDataType = SQL_INTERVAL_MONTH;
					ODBCPrecision = 0;
					break;
				case SQLINTCODE_DAY:
					ODBCDataType = SQL_INTERVAL_DAY;
					ODBCPrecision = 0;
					break;
				case SQLINTCODE_HOUR:
					ODBCDataType = SQL_INTERVAL_HOUR;
					ODBCPrecision = 0;
					break;
				case SQLINTCODE_MINUTE:
					ODBCDataType = SQL_INTERVAL_MINUTE;
					ODBCPrecision = 0;
					break;
				case SQLINTCODE_SECOND:
					ODBCDataType = SQL_INTERVAL_SECOND;
					ODBCPrecision = Precision;
					break;
				case SQLINTCODE_YEAR_MONTH:
					ODBCDataType = SQL_INTERVAL_YEAR_TO_MONTH;
					ODBCPrecision = 0;
					break;
				case SQLINTCODE_DAY_HOUR:
					ODBCDataType = SQL_INTERVAL_DAY_TO_HOUR;
					ODBCPrecision = 0;
					break;
				case SQLINTCODE_DAY_MINUTE:
					ODBCDataType = SQL_INTERVAL_DAY_TO_MINUTE;
					ODBCPrecision = 0;
					break;
				case SQLINTCODE_DAY_SECOND:
					ODBCDataType = SQL_INTERVAL_DAY_TO_SECOND;
					ODBCPrecision = Precision;
					break;
				case SQLINTCODE_HOUR_MINUTE:
					ODBCDataType = SQL_INTERVAL_HOUR_TO_MINUTE;
					ODBCPrecision = 0;
					break;
				case SQLINTCODE_HOUR_SECOND:
					ODBCDataType = SQL_INTERVAL_HOUR_TO_SECOND;
					ODBCPrecision = Precision;
					break;
				case SQLINTCODE_MINUTE_SECOND:
					ODBCDataType = SQL_INTERVAL_MINUTE_TO_SECOND;
					ODBCPrecision = Precision;
					break;
				default:
					ODBCDataType = SQL_TYPE_NULL;
					ODBCPrecision = 0;
					break;
			}
			SignType = FALSE;
			// Calculate the length based on Precision and IntLeadPrec
			// The max. length is for Day to Fraction(6) 
			// Sign = 1
			// Day = IntLeadPrec + 1 ( 1 for Blank space)
			// Hour = 3 ( 2+1)
			// Minute = 3 (2+1)
			// Seconds = 3 (2+1)
			// Fraction = Precision 
			totalMemLen += Length;
			break; 
		case SQLTYPECODE_NUMERIC:              //2
			ODBCPrecision = Precision;
			ODBCDataType = SQL_NUMERIC;
			SignType = TRUE;
			totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3; 
			totalMemLen += Length ;
			break;
		case SQLTYPECODE_NUMERIC_UNSIGNED:    //-201
			ODBCPrecision = Precision;
			ODBCDataType = SQL_NUMERIC;
			SignType = FALSE;
			totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3; 
			totalMemLen += Length ;
			break;			
		default:
			ODBCDataType = SQL_TYPE_NULL;
			ODBCPrecision = 0;
			SignType = FALSE;
			totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3; 
			totalMemLen += Length ;
			break;
	}

	if (!bRWRS)
	{
		if (Nullable)
		{
			totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1; 
			totalMemLen += 2 ;
		}
	}
	SRVRTRACE_EXIT(FILE_INTF+1);

	return SQL_SUCCESS;
}

SQLRETURN SRVR::SetDataPtr(SQLDESC_ID *pDesc, SQLItemDescList_def *SQLDesc, Int32 &totalMemLen,
							BYTE *&VarBuffer, SRVR_DESC_HDL	*implDesc)
{
	SRVRTRACE_ENTER(FILE_INTF+3);

	SQLItemDesc_def *SQLItemDesc;
	Int32 memOffSet = 0;
	BYTE *VarPtr;
	BYTE *IndPtr;
	BYTE *memPtr;
	UInt32 i;
	Int32 retcode;
	BOOL	sqlWarning = FALSE;

	// Adjust totalMemLen to word boundary
	totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3;
	if (VarBuffer != NULL)
	{
		delete VarBuffer;
	}
	VarBuffer = NULL;
	markNewOperator,VarBuffer = new BYTE[totalMemLen];
	if (VarBuffer == NULL)
	{
		// Handle Memory Overflow execption here
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER,
					srvrGlobal->srvrObjRef, 1, "SetDataPtr");
		exit(0);
	}

	memPtr = VarBuffer ;
	memOffSet = 0;

	for (i = 0 ; i < SQLDesc->_length ; i++)
	{
		SQLItemDesc = (SQLItemDesc_def *)SQLDesc->_buffer + i;

		switch (SQLItemDesc->dataType)
		{
		case SQLTYPECODE_CHAR:
		case SQLTYPECODE_VARCHAR:
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen + 1;
			break;
		case SQLTYPECODE_VARCHAR_WITH_LENGTH:
                case SQLTYPECODE_BLOB:
                case SQLTYPECODE_CLOB:
			if( SQLItemDesc->maxLen > SHRT_MAX )
			{
				memOffSet = ((memOffSet + 4 - 1) >> 2) << 2;
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen + 5;
			}
			else
			{
				memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen + 3;
			}
			break;
		case SQLTYPECODE_VARCHAR_LONG:
			memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen + 3;
			break;
		case SQLTYPECODE_BINARY:
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen + 1;
			break;
		case SQLTYPECODE_VARBINARY:
			memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen + 3;
			break;
                case SQLTYPECODE_BOOLEAN:
                case SQLTYPECODE_TINYINT:
                case SQLTYPECODE_TINYINT_UNSIGNED:
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen ;
			break;
		case SQLTYPECODE_SMALLINT:
			memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen ;
			break;
		case SQLTYPECODE_SMALLINT_UNSIGNED:
			memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen ;
			break;
		case SQLTYPECODE_INTEGER:
			memOffSet = ((memOffSet + 4 - 1) >> 2) << 2;
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen ;
			break;
		case SQLTYPECODE_INTEGER_UNSIGNED:
			memOffSet = ((memOffSet + 4 - 1) >> 2) << 2;
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen ;
			break;
		case SQLTYPECODE_LARGEINT:
		case SQLTYPECODE_LARGEINT_UNSIGNED:
			memOffSet = ((memOffSet + 8 - 1) >> 3) << 3;
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen ;
			break;
		case SQLTYPECODE_IEEE_REAL:
			memOffSet = ((memOffSet + 8 - 1) >> 3) << 3;
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen ;
			break;
		case SQLTYPECODE_IEEE_FLOAT:
		case SQLTYPECODE_IEEE_DOUBLE:
			memOffSet = ((memOffSet + 8 - 1) >> 3) << 3;
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen ;
			break;
		case SQLTYPECODE_DATETIME:
	   		memOffSet = ((memOffSet + 8 - 1) >> 3) << 3;
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen ;
			break; 
		case SQLTYPECODE_DECIMAL_UNSIGNED:
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen;
			break;
		case SQLTYPECODE_DECIMAL:
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen;
			break; 
		case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen;
			break;
		case SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen;
			break;
		case SQLTYPECODE_INTERVAL:		// Treating as CHAR
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen;
			break;
		default:
			memOffSet = ((memOffSet + 8 - 1) >> 3) << 3; 
			VarPtr = memPtr + memOffSet;
			memOffSet += SQLItemDesc->maxLen ;
			break;
		}
		if (SQLItemDesc->nullInfo)
		{
			memOffSet = ((memOffSet + 2 - 1) >> 1) << 1; 
			IndPtr = memPtr + memOffSet;
			memOffSet += 2 ;
		}
		else
			IndPtr = NULL;

		retcode = WSQL_EXEC_SetDescItem(pDesc,
											i+1,
											SQLDESC_VAR_PTR,
											(long)VarPtr,
											0);
		HANDLE_ERROR(retcode, sqlWarning);
		
		retcode = WSQL_EXEC_SetDescItem(pDesc,
										i+1,
										SQLDESC_IND_PTR,
										(long)IndPtr,
										0);
		HANDLE_ERROR(retcode, sqlWarning);

		implDesc[i].varPtr = VarPtr;
		implDesc[i].indPtr = IndPtr;

		// set character set for character based data types;
		if (SQLItemDesc->dataType == SQLTYPECODE_CHAR ||
			SQLItemDesc->dataType == SQLTYPECODE_VARCHAR ||
			SQLItemDesc->dataType == SQLTYPECODE_VARCHAR_LONG ||
			SQLItemDesc->dataType == SQLTYPECODE_VARCHAR_WITH_LENGTH ||
                        SQLItemDesc->dataType == SQLTYPECODE_BLOB ||
                        SQLItemDesc->dataType == SQLTYPECODE_CLOB)
		{
			retcode = WSQL_EXEC_SetDescItem(pDesc,          
                               i+1,                   
                               SQLDESC_CHAR_SET,          
                               SQLItemDesc->ODBCCharset,            
                               NULL);
			HANDLE_ERROR(retcode, sqlWarning);
		}

		if (memOffSet > totalMemLen)
		{
				SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
					1, "memOffSet > totalMemLen in SetDataPtr");
			return PROGRAM_ERROR;
		}
	}
	SRVRTRACE_EXIT(FILE_INTF+3);

	if (sqlWarning)
		return SQL_SUCCESS_WITH_INFO;
	else
		return SQL_SUCCESS;
}
//LCOV_EXCL_START
SQLRETURN SRVR::SetRowsetDataPtr(SQLDESC_ID *pDesc, SQLItemDescList_def *SQLDesc, Int32 sqlStmtType, Int32 maxRowsetSize, 
						   Int32 numEntries, Int32 &indMemLen, Int32 &dataMemLen, BYTE *&VarBuffer)
{
	SRVRTRACE_ENTER(FILE_INTF+4);

	SQLItemDesc_def *SQLItemDesc;
	Int32 quadOffSet = 0;
	Int32 typeOffSet = 0;
	Int32 quadLength = 0;

	short*						TypePtr;
	struct SQLCLI_QUAD_FIELDS	*QuadPtr;

	short*						typeBuffPtr;
	struct SQLCLI_QUAD_FIELDS	*quadBuffPtr;

	UInt32	i;
	short			delta;
	Int32			retcode = SQL_SUCCESS;
	BOOL			sqlWarning = FALSE;

	if (VarBuffer != NULL)
		delete VarBuffer;
	VarBuffer = NULL;

	if (sqlStmtType == TYPE_INSERT_PARAM)
	{
		delta = 1;
		quadLength = sizeof(short) * numEntries + 2 * sizeof(SQLCLI_QUAD_FIELDS) * (numEntries + delta);
	}
	else if (sqlStmtType == TYPE_SELECT)
	{
		delta = 0;
		quadLength = sizeof(short) * numEntries + sizeof(SQLCLI_QUAD_FIELDS) * (numEntries + delta);
	}
	else
		return ODBC_SERVER_ERROR;

	markNewOperator,VarBuffer = new BYTE[quadLength];
	if (VarBuffer == NULL)
	{
		// Handle Memory Overflow execption here
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "SetRowsetDataPtr");
		exit(0);

	}
	memset (VarBuffer,0,sizeof(short) * numEntries + sizeof(SQLCLI_QUAD_FIELDS) * (numEntries + delta));

	typeBuffPtr = (short*)VarBuffer;
	quadBuffPtr = (SQLCLI_QUAD_FIELDS*)(VarBuffer + sizeof(short) * numEntries);
	typeOffSet = 0;
		
	for (i = 0 ; i < SQLDesc->_length ; i++)
	{
		SQLItemDesc = (SQLItemDesc_def *)SQLDesc->_buffer + i;

		TypePtr = typeBuffPtr + i;
		QuadPtr = quadBuffPtr + i + delta;
		
		*TypePtr = SQLItemDesc->dataType;

		QuadPtr->var_layout = SQLItemDesc->maxLen;

		if (SQLItemDesc->nullInfo)
			QuadPtr->ind_layout = sizeof(short);

		// set character set for character based data types;
		if (SQLItemDesc->dataType == SQLTYPECODE_CHAR ||
			SQLItemDesc->dataType == SQLTYPECODE_VARCHAR ||
			SQLItemDesc->dataType == SQLTYPECODE_VARCHAR_LONG ||
			SQLItemDesc->dataType == SQLTYPECODE_VARCHAR_WITH_LENGTH ||
                        SQLItemDesc->dataType == SQLTYPECODE_BLOB ||
                        SQLItemDesc->dataType == SQLTYPECODE_CLOB)
		{
			retcode = WSQL_EXEC_SetDescItem(pDesc,          
							   i+1+ delta,                   
							   SQLDESC_CHAR_SET,          
							   SQLItemDesc->ODBCCharset,            
							   NULL);
			HANDLE_ERROR(retcode, sqlWarning);
		}
	}
	SRVRTRACE_EXIT(FILE_INTF+4);

	return SQL_SUCCESS;
}
//LCOV_EXCL_STOP
SQLRETURN SRVR::AllocAssignValueBuffer(
				bool& bSQLValueListSet,
				SQLItemDescList_def *SQLDesc,  
				SQLValueList_def *SQLValueList, 
				Int32 totalMemLen, 	
				Int32 maxRowCount, 
				BYTE *&VarBuffer)
{
	SRVRTRACE_ENTER(FILE_INTF+5);

	SQLItemDesc_def *SQLItemDesc;
	SQLValue_def *SQLValue;
	Int32 memOffSet = 0;
	BYTE *VarPtr;
	BYTE *IndPtr;
	BYTE *memPtr;
	UInt32 curValueCount, curDescCount;
	Int32 curRowCount;
	Int32 numValues;
	Int32 AllocLength;
	Int32 totalRowMemLen;

	// Allocate SQLValue Array
	if (SQLValueList->_buffer != NULL)
	{
		delete SQLValueList->_buffer;
	}
	if (VarBuffer != NULL)
	{
		delete VarBuffer;
	}
	SQLValueList->_buffer = NULL;
	VarBuffer = NULL;
	numValues = SQLDesc->_length * maxRowCount;
	if (numValues == 0)
	{
		SQLValueList->_buffer = NULL;
		SQLValueList->_length = 0;
		VarBuffer = NULL;
		return SQL_SUCCESS;
	}

	bSQLValueListSet = true;
	markNewOperator,SQLValueList->_buffer = new SQLValue_def[numValues];
	if (SQLValueList->_buffer == NULL)
	{
		// Handle Memory Overflow execption here
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "AllocAssignValueBuffer(1)");
		exit(0);
	}
	SQLValueList->_length = 0;
	

	// Allocate the Value Buffer
	totalRowMemLen = totalMemLen * maxRowCount;
	markNewOperator,VarBuffer = new BYTE[totalRowMemLen];
	if (VarBuffer == NULL)
	{
		// Handle Memory Overflow execption here
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "AllocAssignValueBuffer(2)");
		exit(0);

	}

	memPtr = VarBuffer ;
	memOffSet = 0;
	
	for (curRowCount = 0, curValueCount = 0 ; curRowCount < maxRowCount ; curRowCount++)
	{
		for (curDescCount = 0 ; curDescCount < SQLDesc->_length ; curDescCount++, curValueCount++)
		{
			SQLItemDesc = (SQLItemDesc_def *)SQLDesc->_buffer + curDescCount;
			SQLValue  = (SQLValue_def *)SQLValueList->_buffer + curValueCount;
			switch (SQLItemDesc->dataType)
			{
			case SQLTYPECODE_CHAR:
			case SQLTYPECODE_VARCHAR:
                        case SQLTYPECODE_BINARY:
				VarPtr = memPtr + memOffSet;  
				memOffSet += SQLItemDesc->maxLen + 1;
				AllocLength = SQLItemDesc->maxLen + 1;
				break;
			case SQLTYPECODE_VARCHAR_WITH_LENGTH:
			case SQLTYPECODE_BLOB:
			case SQLTYPECODE_CLOB:
                        case SQLTYPECODE_VARBINARY:
				if( SQLItemDesc->maxLen > SHRT_MAX )
				{
					memOffSet = ((memOffSet + 4 - 1) >> 2) << 2;
					VarPtr = memPtr + memOffSet;
					memOffSet += SQLItemDesc->maxLen + 5;
					AllocLength = SQLItemDesc->maxLen + 5;
				}
				else
				{
					memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
					VarPtr = memPtr + memOffSet;
					memOffSet += SQLItemDesc->maxLen + 3;
					AllocLength = SQLItemDesc->maxLen + 3;
				}
				break;
			case SQLTYPECODE_INTERVAL: 
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen;
				AllocLength = SQLItemDesc->maxLen;
				break;
			case SQLTYPECODE_VARCHAR_LONG:
				memOffSet = ((memOffSet + 2 - 1) >> 1) << 1; 
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen + 3;
				AllocLength = SQLItemDesc->maxLen + 3;
				break;
                        case SQLTYPECODE_BOOLEAN:
                        case SQLTYPECODE_TINYINT:
                        case SQLTYPECODE_TINYINT_UNSIGNED:
				VarPtr = memPtr + memOffSet; 
				memOffSet += SQLItemDesc->maxLen;
				AllocLength = SQLItemDesc->maxLen;
				break;
			case SQLTYPECODE_SMALLINT:
				memOffSet = ((memOffSet + 2 - 1) >> 1) << 1; 
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen;
				AllocLength = SQLItemDesc->maxLen;
				break;
			case SQLTYPECODE_SMALLINT_UNSIGNED:
				memOffSet = ((memOffSet + 2 - 1) >> 1) << 1; 
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen ;
				AllocLength = SQLItemDesc->maxLen;
				break;
			case SQLTYPECODE_INTEGER:
				memOffSet = ((memOffSet + 4 - 1) >> 2) << 2; 
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen ;
				AllocLength = SQLItemDesc->maxLen;
				break;
			case SQLTYPECODE_INTEGER_UNSIGNED:
				memOffSet = ((memOffSet + 4 - 1) >> 2) << 2; 
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen ;
				AllocLength = SQLItemDesc->maxLen;
				break;
			case SQLTYPECODE_LARGEINT:
                        case SQLTYPECODE_LARGEINT_UNSIGNED:
				memOffSet = ((memOffSet + 8 - 1) >> 3) << 3; 
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen ;
				AllocLength = SQLItemDesc->maxLen;
				break;
			case SQLTYPECODE_IEEE_REAL:
				memOffSet = ((memOffSet + 8 - 1) >> 3) << 3; 
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen ;
				AllocLength = SQLItemDesc->maxLen;
				break;
			case SQLTYPECODE_IEEE_FLOAT:
			case SQLTYPECODE_IEEE_DOUBLE:
				memOffSet = ((memOffSet + 8 - 1) >> 3) << 3; 
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen ;
				AllocLength = SQLItemDesc->maxLen;
				break;
			case SQLTYPECODE_DATETIME:
	   			memOffSet = ((memOffSet + 8 - 1) >> 3) << 3; 
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen ;
				AllocLength = SQLItemDesc->maxLen;
				break; 
			case SQLTYPECODE_DECIMAL_UNSIGNED:
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen;
				AllocLength = SQLItemDesc->maxLen;
				break;
			case SQLTYPECODE_DECIMAL:
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen;
				AllocLength = SQLItemDesc->maxLen;
				break; 
			case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen;
				AllocLength = SQLItemDesc->maxLen;
				break;
			case SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen;
				AllocLength = SQLItemDesc->maxLen;
				break;
			default:
				memOffSet = ((memOffSet + 8 - 1) >> 3) << 3; 
				VarPtr = memPtr + memOffSet;
				memOffSet += SQLItemDesc->maxLen ;
				AllocLength = SQLItemDesc->maxLen;
				break;				
			}
			if (SQLItemDesc->nullInfo)
			{
				memOffSet = ((memOffSet + 2 - 1) >> 1) << 1; 
				IndPtr = memPtr + memOffSet;
				memOffSet += 2 ;
			}
			else
				IndPtr = NULL;
			SQLValue->dataValue._buffer = VarPtr;
			// Ignore the indPtr, since it is declared as short already in SQLValue_def
			SQLValue->dataType = SQLItemDesc->dataType;
			SQLValue->dataValue._length = AllocLength;
			SQLValue->dataCharset = SQLItemDesc->ODBCCharset;
		
			if (memOffSet > totalRowMemLen)
			{
					SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
						1, "memOffSet > totalMemLen in AllocAssignValueBuffer");
				return PROGRAM_ERROR;
			}
		}
		// Align it to next word boundary for the next row
		memOffSet = ((memOffSet + 8 - 1) >> 3) << 3; 
	}
	SRVRTRACE_EXIT(FILE_INTF+5);

	return SQL_SUCCESS;
}
//LCOV_EXCL_START
SQLRETURN SRVR::CopyValueList(SQLValueList_def *outValueList, const SQLValueList_def *inValueList)
{
	SRVRTRACE_ENTER(FILE_INTF+6);

	SQLValue_def *inValue;
	SQLValue_def *outValue;
	UInt32 i;

	for (i = 0; i < inValueList->_length ; i++)
	{
		inValue = (SQLValue_def *)inValueList->_buffer + i;
		outValue = (SQLValue_def *)outValueList->_buffer + i;
		
		if (inValue->dataType != outValue->dataType)
		{
			SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
					1, "inValue->dataType != outValue->dataType");
			return PROGRAM_ERROR;
		}
		outValue->dataInd = inValue->dataInd;
		if (inValue->dataInd == 0)
		{
			if (inValue->dataValue._length != outValue->dataValue._length)
			{
				SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
						1, "inValue->dataValue._length != outValue->dataValue._length");
				return PROGRAM_ERROR;	
			}
			memcpy(outValue->dataValue._buffer, inValue->dataValue._buffer, outValue->dataValue._length);
		}
		else
		{
			outValue->dataValue._length = 0;
		}
	}
	outValueList->_length = inValueList->_length;
	SRVRTRACE_EXIT(FILE_INTF+6);

	return SQL_SUCCESS;
}
//LCOV_EXCL_STOP

SQLRETURN SRVR::BuildSQLDesc(SQLDESC_ID *pDesc,
						short numEntries,
						SQLItemDescList_def *SQLDesc,
						BYTE *&varBuffer,
						Int32	&totalMemLen,
						SRVR_DESC_HDL	*&implDesc)
{
	SRVRTRACE_ENTER(FILE_INTF+7);

	Int32 FSDataType; // added this to get file system datatype 
					 // to find numeric signed or unsigned.
	Int32 DataType;
	Int32 DateTimeCode;
	Int32 Length;
	Int32 Nullable;
	Int32 Scale;
	Int32 Precision;
 	char Heading[MAX_ANSI_NAME_LEN];
	Int32 retcode = SQL_SUCCESS;
	short i, j;
	Int32 ODBCPrecision;
	Int32 ODBCDataType;
	Int32 SQLCharset;
	Int32 ODBCCharset;
	BOOL SignType;
	BOOL sqlWarning = FALSE;
	Int32 IntLeadPrec;
	Int32 paramMode;
	
	if (implDesc != NULL)
	{
		delete implDesc;
		implDesc = NULL;
	}

	if (numEntries > 0)
	{
		markNewOperator,implDesc = new SRVR_DESC_HDL[numEntries];
		if (implDesc == NULL)
		{
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "BuildSQLDesc");
			exit(0);
		}
	}

	for (i = 1, totalMemLen = 0; i <= numEntries; i++)
	{
		// Initialize the desc entry in SQLDESC_ITEM struct
		for (j = 0; j < NO_OF_DESC_ITEMS ; j++)
		{
			gDescItems[j].entry = i;
		}
		gDescItems[10].num_val_or_len = MAX_ANSI_NAME_LEN;
		gDescItems[11].num_val_or_len = MAX_ANSI_NAME_LEN;
		gDescItems[12].num_val_or_len = MAX_ANSI_NAME_LEN;
		gDescItems[13].num_val_or_len = MAX_ANSI_NAME_LEN;
		gDescItems[14].num_val_or_len = MAX_ANSI_NAME_LEN;

		retcode = WSQL_EXEC_GetDescItems2(pDesc,
					NO_OF_DESC_ITEMS,
					(SQLDESC_ITEM *)&gDescItems);
		HANDLE_ERROR(retcode, sqlWarning);

		// This change is made for ADO since it can't handle 'COLUMN HEADING' since
		// driver returns this as SQL_DESC_LABEL, So change it to 'COLUMN NAME'.
		// Temperaryly use Heading to send NULL and in future replace ColumnHeading
		// with Heading.
		Heading[0] = '\0';

		DataType = gDescItems[0].num_val_or_len;
		Length = gDescItems[1].num_val_or_len;
		Precision = gDescItems[2].num_val_or_len;
		Scale = gDescItems[3].num_val_or_len;
		Nullable = gDescItems[4].num_val_or_len;
		paramMode = gDescItems[5].num_val_or_len;
		IntLeadPrec = gDescItems[6].num_val_or_len;
		DateTimeCode = gDescItems[7].num_val_or_len;
		SQLCharset = gDescItems[8].num_val_or_len;
		FSDataType = gDescItems[9].num_val_or_len;
		CatalogName[gDescItems[10].num_val_or_len] = '\0';
		SchemaName[gDescItems[11].num_val_or_len] = '\0';
		TableName[gDescItems[12].num_val_or_len] = '\0';
		DescName[gDescItems[13].num_val_or_len] = '\0';
		ColumnHeading[gDescItems[14].num_val_or_len] = '\0';

		// Added this check since SQL started returning SQLTYPECODE_NUMERIC.
		// Instead of changing on driver side Datatype is changed here to work correctly.
		if (DataType == SQLTYPECODE_NUMERIC || DataType == SQLTYPECODE_NUMERIC_UNSIGNED)
		{
			switch (FSDataType)
			{
				case 130:
					DataType = SQLTYPECODE_SMALLINT;
					break;
				case 131:
					DataType = SQLTYPECODE_SMALLINT_UNSIGNED;
					break;
				case 132:
					DataType = SQLTYPECODE_INTEGER;
					break;
				case 133:
					DataType = SQLTYPECODE_INTEGER_UNSIGNED;
					break;
				case 134:
					DataType = SQLTYPECODE_LARGEINT;
					break;	
				case 136:
					DataType = SQLTYPECODE_TINYINT;
					break;
				case 137:
					DataType = SQLTYPECODE_TINYINT_UNSIGNED;
					break;
				case 138:
					DataType = SQLTYPECODE_LARGEINT_UNSIGNED;
					break;	
							
				default:
					break;
			}
		}

		retcode = GetODBCValues(DataType, DateTimeCode, Length, Precision, ODBCDataType, 
					ODBCPrecision, SignType, Nullable, totalMemLen,	SQLCharset, ODBCCharset,
					IntLeadPrec, ColumnHeading);
		HANDLE_ERROR(retcode, sqlWarning);
                if (DataType == SQLTYPECODE_LARGEINT && Precision == 0 && Scale > 0)
                   ODBCDataType = SQL_NUMERIC;

		implDesc[i-1].charSet = SQLCharset;
		implDesc[i-1].dataType = DataType;
		implDesc[i-1].length = Length;
		implDesc[i-1].paramMode = paramMode;
			
		kdsCopyToSQLDescSeq(SQLDesc, DescName, DataType, DateTimeCode, Length, 
				  Precision, Scale, Nullable, ODBCDataType, ODBCPrecision, SignType, 
				  SQLCharset, ODBCCharset, TableName, CatalogName, 
				  SchemaName, Heading, IntLeadPrec, paramMode);

	}
	retcode = SetDataPtr(pDesc, SQLDesc, totalMemLen, varBuffer, implDesc);
	HANDLE_ERROR(retcode, sqlWarning);
	
	SRVRTRACE_EXIT(FILE_INTF+7);

	if (sqlWarning)
		return SQL_SUCCESS_WITH_INFO;
	else
		return SQL_SUCCESS;
}

SQLRETURN SRVR::BuildSQLDesc2(SQLDESC_ID *pDesc,
						Int32 sqlQueryType, 
						Int32 maxRowsetSize,
						bool &sqlBulkFetchPossible,
						Int32 numEntries,
						BYTE *&SQLDesc,
						Int32 &SQLDescLen,
						BYTE *&varBuffer,
						Int32 &totalMemLen,
						SRVR_DESC_HDL *&implDesc,
						DESC_HDL_LISTSTMT *&SqlDescInfo)
{
	SRVRTRACE_ENTER(FILE_INTF+7);

	Int32 FSDataType; // added this to get file system datatype 
					 // to find numeric signed or unsigned.
	Int32 VarAlign;
	Int32 IndAlign;
	Int32 Version = 0;
	Int32 DataType;
	Int32 DateTimeCode;
	Int32 Length;
	Int32 Precision;
	Int32 Scale;
	Int32 Nullable;
	BOOL SignType;
	Int32 ODBCDataType;
	Int32 ODBCPrecision;
	Int32 SQLCharset;
	Int32 ODBCCharset;
	Int32 ColHeadingNmlen;
	Int32 TableNamelen;
	Int32 CatalogNamelen;
	Int32 SchemaNamlen;
	Int32 Headinglen;
	Int32 IntLeadPrec;
	Int32 paramMode;

	Int32 totalDescLength = 0;
	Int32 i, j;
	Int32 tempLen = 0;
	Int32 retcode = SQL_SUCCESS;
	BOOL sqlWarning = FALSE;
 	char Heading[MAX_ANSI_NAME_LEN];

	
	bool bRWRS = false;
	if (srvrGlobal->drvrVersion.buildId & ROWWISE_ROWSET)
		bRWRS = true;
        
	struct ODBCDescriptors
	{
	  Int32 varAlign;
	  Int32 indAlign;
	  Int32 version;
	  Int32 dataType;
	  Int32 datetimeCode;
	  Int32 maxLen;
	  Int32 precision;
	  Int32 scale;
	  Int32 nullInfo;
	  Int32 signType;
	  Int32 ODBCDataType;
	  Int32 ODBCPrecision;
	  Int32 SQLCharset;
	  Int32 ODBCCharset;
	  Int32 colHeadingNmlen;
	  char colHeadingNm[MAX_ANSI_NAME_LEN+1];
	  Int32 TableNamelen;
	  char TableName[MAX_ANSI_NAME_LEN+1];
	  Int32 CatalogNamelen;
	  char CatalogName[MAX_ANSI_NAME_LEN+1];
	  Int32 SchemaNamlen;
	  char SchemaName[MAX_ANSI_NAME_LEN+1];
	  Int32 Headinglen;
	  char Heading[MAX_ANSI_NAME_LEN+1];
	  Int32 intLeadPrec;
	  Int32 paramMode;
	};
	/* Moved to CSrvrStmt.h file
	typedef struct tagDESC_HDL_LIST
	{
		Int32 DataType;
		Int32 Length;
		Int32 Nullable;
		Int32 VarBuf;
		Int32 IndBuf;
	} DESC_HDL_LIST;

	DESC_HDL_LIST	*SqlDescInfo;
	*/
	Int32			tempDescLen = 0;

	if (implDesc != NULL)
	{
		delete implDesc;
		implDesc = NULL;
	}

	if (SqlDescInfo != NULL)
	{
		delete SqlDescInfo;
		SqlDescInfo = NULL;
	}
        
        if (SQLDesc != NULL)
        {
           delete SQLDesc;
           SQLDesc = NULL;
        }

	if (numEntries > 0)
	{

		markNewOperator,implDesc = new SRVR_DESC_HDL[numEntries];
		if (implDesc == NULL)
		{
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "BuildSQLDesc");
			exit(0);
		}

		markNewOperator,SqlDescInfo = new DESC_HDL_LISTSTMT[numEntries];
		if (SqlDescInfo == NULL)
		{
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "BuildSQLDesc2");
			exit(0);
		}

		tempDescLen	= sizeof(totalMemLen) + sizeof(numEntries) + (sizeof(ODBCDescriptors) * numEntries);
		markNewOperator,SQLDesc = new BYTE[tempDescLen];
		if (SQLDesc == NULL)
		{
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "BuildSQLDesc2");
			exit(0);
		}

		*(Int32 *)(SQLDesc+totalDescLength) = 0; // Initialize totalMemLen, Since its calculated later
		totalDescLength += sizeof(totalMemLen);

		*(Int32 *)(SQLDesc+totalDescLength) = numEntries;
		totalDescLength += sizeof(numEntries);

	}

//	if (srvrGlobal->drvrVersion.componentId == JDBC_DRVR_COMPONENT && srvrGlobal->drvrVersion.majorVersion == 3 && srvrGlobal->drvrVersion.minorVersion <= 1)
//		sqlBulkFetchPossible = false;
//	else
//		sqlBulkFetchPossible = true;

	for (i = 1, totalMemLen = 0; i <= numEntries; i++)
	{
		// Initialize the desc entry in SQLDESC_ITEM struct
		for (j = 0; j < NO_OF_DESC_ITEMS ; j++)
		{
			gDescItems[j].entry = i;
		}
		gDescItems[10].num_val_or_len = MAX_ANSI_NAME_LEN;
		gDescItems[11].num_val_or_len = MAX_ANSI_NAME_LEN;
		gDescItems[12].num_val_or_len = MAX_ANSI_NAME_LEN;
		gDescItems[13].num_val_or_len = MAX_ANSI_NAME_LEN;
		gDescItems[14].num_val_or_len = MAX_ANSI_NAME_LEN;

		retcode = WSQL_EXEC_GetDescItems2(pDesc,
					NO_OF_DESC_ITEMS,
					(SQLDESC_ITEM *)&gDescItems);
		HANDLE_ERROR(retcode, sqlWarning);

		// this change is made for ADO since it can't handle 'COLUMN HEADING' since
		// driver returns this as SQL_DESC_LABEL, So change it to 'COLUMN NAME'.
		// Temperaryly use Heading to send NULL and in future replace ColumnHeading
		// with Heading.
		Heading[0] = '\0';

		DataType = gDescItems[0].num_val_or_len;
		Length = gDescItems[1].num_val_or_len;
		Precision = gDescItems[2].num_val_or_len;
		Scale = gDescItems[3].num_val_or_len;
		Nullable = gDescItems[4].num_val_or_len;
		paramMode = gDescItems[5].num_val_or_len;
		IntLeadPrec = gDescItems[6].num_val_or_len;
		DateTimeCode = gDescItems[7].num_val_or_len;
		SQLCharset = gDescItems[8].num_val_or_len;
		FSDataType = gDescItems[9].num_val_or_len;
		CatalogName[gDescItems[10].num_val_or_len] = '\0';
		SchemaName[gDescItems[11].num_val_or_len] = '\0';
		TableName[gDescItems[12].num_val_or_len] = '\0';
		DescName[gDescItems[13].num_val_or_len] = '\0';
		ColumnHeading[gDescItems[14].num_val_or_len] = '\0';

		// Added this check since SQL started returning SQLTYPECODE_NUMERIC.
		// Instead of changing on driver side Datatype is changed here to work correctly.
		if (DataType == SQLTYPECODE_NUMERIC || DataType == SQLTYPECODE_NUMERIC_UNSIGNED)
		{
			switch (FSDataType)
			{
				case 130:
					DataType = SQLTYPECODE_SMALLINT;
					break;
				case 131:
					DataType = SQLTYPECODE_SMALLINT_UNSIGNED;
					break;
				case 132:
					DataType = SQLTYPECODE_INTEGER;
					break;
				case 133:
					DataType = SQLTYPECODE_INTEGER_UNSIGNED;
					break;
				case 134:
					DataType = SQLTYPECODE_LARGEINT;
					break;
				case 136:
					DataType = SQLTYPECODE_TINYINT;
					break;
				case 137:
					DataType = SQLTYPECODE_TINYINT_UNSIGNED;
					break;
				case 138:
					DataType = SQLTYPECODE_LARGEINT_UNSIGNED;
					break;
				
				default:
					break;
			}
		}

//		if (sqlBulkFetchPossible)
//		{
//			if (!Nullable)
//			{
//				switch (DataType)
//				{
//					case SQLTYPECODE_VARCHAR:
//					case SQLTYPECODE_VARCHAR_WITH_LENGTH:
//					case SQLTYPECODE_VARCHAR_LONG:
//					case SQLTYPECODE_DATETIME:
//					case SQLTYPECODE_INTERVAL:
//						sqlBulkFetchPossible = false;
//						break;
//				}
//			}
//			else
//				sqlBulkFetchPossible = false;
//		}

		retcode = GetODBCValues(DataType, DateTimeCode, Length, Precision, ODBCDataType, 
					ODBCPrecision, SignType, Nullable, totalMemLen,	SQLCharset, ODBCCharset,
					IntLeadPrec, ColumnHeading);
		HANDLE_ERROR(retcode, sqlWarning);
		if (DataType == SQLTYPECODE_LARGEINT && Precision == 0 && Scale > 0)
			ODBCDataType = SQL_NUMERIC;

		implDesc[i-1].charSet = SQLCharset;
		implDesc[i-1].dataType = DataType;
		implDesc[i-1].length = Length;
		implDesc[i-1].paramMode = paramMode;

		SqlDescInfo[i-1].DataType = DataType;
		SqlDescInfo[i-1].Length = Length;
		SqlDescInfo[i-1].Nullable = Nullable;
		SqlDescInfo[i-1].VarBuf = totalDescLength;
		totalDescLength += sizeof(VarAlign);
		SqlDescInfo[i-1].IndBuf = totalDescLength;
		totalDescLength += sizeof(IndAlign);

		*(Int32 *)(SQLDesc+totalDescLength) = Version;
		totalDescLength += sizeof(Version);

		*(Int32 *)(SQLDesc+totalDescLength) = DataType;
		totalDescLength += sizeof(DataType);

		*(Int32 *)(SQLDesc+totalDescLength) = DateTimeCode;
		totalDescLength += sizeof(DateTimeCode);
		
		*(Int32 *)(SQLDesc+totalDescLength) = Length;
		totalDescLength += sizeof(Length);

		*(Int32 *)(SQLDesc+totalDescLength) = Precision;
		totalDescLength += sizeof(Precision);

		*(Int32 *)(SQLDesc+totalDescLength) = Scale;
		totalDescLength += sizeof(Scale);

		*(Int32 *)(SQLDesc+totalDescLength) = Nullable;
		totalDescLength += sizeof(Nullable);

		if (SignType)  // This may change if SQL returns sign and since desc will be Int32 return as Int32
			*(Int32 *)(SQLDesc+totalDescLength) = 1;
		else
			*(Int32 *)(SQLDesc+totalDescLength) = 0;
		totalDescLength += sizeof(Int32);

		*(Int32 *)(SQLDesc+totalDescLength) = ODBCDataType;
		totalDescLength += sizeof(ODBCDataType);

		*(Int32 *)(SQLDesc+totalDescLength) = ODBCPrecision;
		totalDescLength += sizeof(ODBCPrecision);

		*(Int32 *)(SQLDesc+totalDescLength) = SQLCharset;
		totalDescLength += sizeof(SQLCharset);
		
		*(Int32 *)(SQLDesc+totalDescLength) = ODBCCharset;
		totalDescLength += sizeof(ODBCCharset);
		
		ColHeadingNmlen = gDescItems[13].num_val_or_len+1; // Null terminator
		*(Int32 *)(SQLDesc+totalDescLength) = ColHeadingNmlen;
		totalDescLength += sizeof(ColHeadingNmlen);
		if (ColHeadingNmlen > 0)
		{
			memcpy(SQLDesc+totalDescLength,DescName,ColHeadingNmlen);
			totalDescLength += ColHeadingNmlen;
		}
		
		TableNamelen = gDescItems[12].num_val_or_len+1;
		*(Int32 *)(SQLDesc+totalDescLength) = TableNamelen;
		totalDescLength += sizeof(TableNamelen);
		if (TableNamelen > 0)
		{
			memcpy(SQLDesc+totalDescLength,TableName,TableNamelen);
			totalDescLength += TableNamelen;
		}

		CatalogNamelen = gDescItems[10].num_val_or_len+1;
		*(Int32 *)(SQLDesc+totalDescLength) = CatalogNamelen;
		totalDescLength += sizeof(CatalogNamelen);
		if (CatalogNamelen > 0)
		{
			memcpy(SQLDesc+totalDescLength,CatalogName,CatalogNamelen);
			totalDescLength += CatalogNamelen;
		}
		
		SchemaNamlen = gDescItems[11].num_val_or_len+1;
		*(Int32 *)(SQLDesc+totalDescLength) = SchemaNamlen;
		totalDescLength += sizeof(SchemaNamlen);
		if (SchemaNamlen > 0)
		{
			memcpy(SQLDesc+totalDescLength,SchemaName,SchemaNamlen);
			totalDescLength += SchemaNamlen;
		}
		
		// Headinglen = gDescItems[14].num_val_or_len+1; // Change this when we start supporting strlen(ColumnHeading);
		Headinglen = 1;
		*(Int32 *)(SQLDesc+totalDescLength) = Headinglen;
		totalDescLength += sizeof(Headinglen);
		if (Headinglen > 0)
		{
		//	memcpy(SQLDesc+totalDescLength,ColumnHeading,Headinglen); // Change this when we start supporting strlen(ColumnHeading);
			memcpy(SQLDesc+totalDescLength,Heading,Headinglen);
			totalDescLength += Headinglen;
		}

		*(Int32 *)(SQLDesc+totalDescLength) = IntLeadPrec;
		totalDescLength += sizeof(IntLeadPrec);

		*(Int32 *)(SQLDesc+totalDescLength) = paramMode;
		totalDescLength += sizeof(paramMode);
	}
	
	if (!bRWRS)
//		sqlBulkFetchPossible = true;
//	else
	{
		// Adjust totalMemLen to word boundary
		totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3;
	}

	if (varBuffer != NULL)
	{
		delete varBuffer;
	}
	varBuffer = NULL;
//	if (sqlBulkFetchPossible && (sqlQueryType == SQL_SELECT_NON_UNIQUE || sqlQueryType == SQL_SP_RESULT_SET))
	if (bRWRS)
		markNewOperator,varBuffer = new BYTE[srvrGlobal->m_FetchBufferSize];
	else
		markNewOperator,varBuffer = new BYTE[totalMemLen];
	if (varBuffer == NULL)
	{
		// Handle Memory Overflow execption here
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "BuildSQLDesc2");
		exit(0);
	}

		//setting the Indicator and Variable pointer
		retcode =  SetIndandVarPtr(pDesc,bRWRS,numEntries,SQLDesc,varBuffer,totalMemLen,
						implDesc,SqlDescInfo);

	SQLDescLen = totalDescLength;
//	if (sqlBulkFetchPossible && (sqlQueryType == SQL_SELECT_NON_UNIQUE || sqlQueryType == SQL_SP_RESULT_SET))
	if (bRWRS)
	{
		Int32 TmpMaxRows = (srvrGlobal->m_FetchBufferSize)/totalMemLen;

		retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWSET_TYPE, 3, 0);

		HANDLE_ERROR(retcode, sqlWarning);

		retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWWISE_ROWSET_PTR, (long)varBuffer, 0);
		HANDLE_ERROR(retcode, sqlWarning);

		retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWWISE_ROWSET_SIZE, TmpMaxRows, 0);
		HANDLE_ERROR(retcode, sqlWarning);

		retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWWISE_ROWSET_ROW_LEN, totalMemLen, 0);
		HANDLE_ERROR(retcode, sqlWarning);
	}
	
	SRVRTRACE_EXIT(FILE_INTF+7);

	if (sqlWarning)
		return SQL_SUCCESS_WITH_INFO;
	else
		return SQL_SUCCESS;
}
//LCOV_EXCL_START
Int32 SRVR::sqlMaxDataLength(SQLSMALLINT SQLDataType, SQLINTEGER SQLMaxLength)
{
	SRVRTRACE_ENTER(FILE_INTF+9);

	Int32 sqlMaxLength = 0;
	switch (SQLDataType)
	{
	case SQLTYPECODE_BLOB:
	case SQLTYPECODE_CLOB:
	case SQLTYPECODE_VARCHAR_WITH_LENGTH:
	case SQLTYPECODE_VARCHAR_LONG:
	case SQLTYPECODE_BITVAR:
		sqlMaxLength = SQLMaxLength + 2;
		sqlMaxLength = ((sqlMaxLength + 2 - 1)>>1)<<1;
		break;
	default:
		sqlMaxLength = SQLMaxLength;
		break;
	}
	SRVRTRACE_EXIT(FILE_INTF+9);

	return sqlMaxLength;
}


Int32 SRVR::dataLength(SQLSMALLINT SQLDataType, SQLINTEGER SQLOctetLength, void* buffer)
{
	SRVRTRACE_ENTER(FILE_INTF+10);

	Int32 allocLength = 0;
	switch (SQLDataType)
	{
	case SQLTYPECODE_BLOB:
	case SQLTYPECODE_CLOB:
	case SQLTYPECODE_VARCHAR_WITH_LENGTH:
	case SQLTYPECODE_VARCHAR_LONG:
	case SQLTYPECODE_BITVAR:
		allocLength = *(USHORT *)buffer + 2;
		break;
	default:
		allocLength = SQLOctetLength;
		break;
	}
	SRVRTRACE_EXIT(FILE_INTF+10);

	return allocLength;
}

// We need to change rowsAffected in pSrvrStmt to __int64
SQLRETURN SRVR::GetRowsAffected(SRVR_STMT_HDL *pSrvrStmt)
{
	SQLDESC_ID *rowCountDesc = NULL;
	SQLMODULE_ID * module = NULL;

	Int32 retcode = SQL_SUCCESS;
	BOOL sqlWarning = FALSE;

	__int64 rowsAffected_ = 0;
	SQLDIAG_STMT_INFO_ITEM_ID sqlItem = SQLDIAG_ROW_COUNT;
	rowCountDesc = new SQLDESC_ID;
	rowCountDesc->version = SQLCLI_ODBC_VERSION;
	rowCountDesc->name_mode = desc_handle;
	module = new SQLMODULE_ID;
	rowCountDesc->module = module;
	module->module_name = 0;
	module->charset = SQLCHARSETSTRING_UTF8;
	module->module_name_len = 0;
	module->version = SQLCLI_ODBC_VERSION;
	rowCountDesc->identifier = 0;
	rowCountDesc->handle = 0;

	// get number of rows affected

	retcode = WSQL_EXEC_AllocDesc (rowCountDesc, 1);
	HANDLE_ERROR(retcode, sqlWarning);

  	retcode = WSQL_EXEC_SetDescItem(rowCountDesc, 1,
					 SQLDESC_TYPE_FS,
					 134, 0);
	HANDLE_ERROR(retcode, sqlWarning);

	retcode = WSQL_EXEC_SetDescItem(rowCountDesc, 1,
					 SQLDESC_VAR_PTR,
					 (long) &rowsAffected_, 0);
	HANDLE_ERROR(retcode, sqlWarning);

	Int32 *temp;
	temp = (Int32 *) &sqlItem;

	retcode = WSQL_EXEC_GetDiagnosticsStmtInfo(temp, rowCountDesc);
	HANDLE_ERROR(retcode, sqlWarning);

	pSrvrStmt->rowsAffected = (Int32)(rowsAffected_ << 32 >> 32);
	pSrvrStmt->rowsAffectedHigherBytes = (Int32)(rowsAffected_ >> 32);

	// free up resources
	WSQL_EXEC_DeallocDesc(rowCountDesc);
	delete ((void*)rowCountDesc->module);
	delete rowCountDesc;

	return SQL_SUCCESS;

} // SRVR::GetRowsAffected()
//LCOV_EXCL_STOP

// regular execute
SQLRETURN SRVR::EXECUTE(SRVR_STMT_HDL* pSrvrStmt)
{ 
	SRVRTRACE_ENTER(FILE_INTF+13);

	Int32 inputRowCnt = pSrvrStmt->inputRowCnt;
	IDL_short sqlStmtType = pSrvrStmt->sqlStmtType;
	const SQLValueList_def *inputValueList = &pSrvrStmt->inputValueList;
	Int64 rowsAffected_cli;
	Int32 *rowsAffected = &pSrvrStmt->rowsAffected;
	Int32 *rowsAffectedHigherBytes = &pSrvrStmt->rowsAffectedHigherBytes;

	Int32 retcode = SQL_SUCCESS;
	Int32 execretcode = SQL_SUCCESS;
	
	SQLDESC_ID *pDesc;
    SQLSTMT_ID *pStmt;
	SQLSTMT_ID cursorId;
	Int32		paramCount;
	char		*cursorName;
	Int32		len;
	Int32		curRowCnt;
	Int32		curParamNo;
	Int32		curLength;
	BYTE		*varPtr;
	BYTE		*indPtr;
	void			*pBytes;
	SQLValue_def	*SQLValue;
	BOOL			sqlWarning = FALSE;
	SRVR_DESC_HDL	*IPD;
	Int64	t_rowsAffected = 0;

	pSrvrStmt->PerfFetchRetcode = SQL_SUCCESS;
	pSrvrStmt->RowsetFetchRetcode = SQL_SUCCESS;

	// Initialize this to TRUE later in this function depending on sqlStmtType if SQL_SELECT
	// then and retcode is SUCCESS or SUCCESS_WITH_INFO set it to FALSE. Since SQL opens CURSOR
	// after execute in case of SUCCESS or SUCCESS_WITH_INFO and closes only when it reaches 
	// NO_DATA_FOUND or CLOSE STATEMENT or COMMIT WORK or ROLLBACK WORK.
	pSrvrStmt->isClosed = TRUE;

	pStmt = &pSrvrStmt->stmt;
	if (pSrvrStmt->stmtType == EXTERNAL_STMT && sqlStmtType == TYPE_SELECT &&
			pSrvrStmt->moduleId.module_name == NULL)
	{
		cursorName = pSrvrStmt->cursorName;
		// If cursor name is not specified, use the stmt name as cursor name
		if (*cursorName == '\0')
			cursorName = pSrvrStmt->stmtName;
		// If cursorName has chg'd from last EXEC or EXECDIRECT cmd
		// or has not yet been set for the first time call SetCursorName
		if ((strcmp(pSrvrStmt->previousCursorName, cursorName) != 0) && *cursorName != '\0')
		{
			cursorId.version        = SQLCLI_ODBC_VERSION;
			cursorId.module         = pStmt->module;
			cursorId.handle         = 0;
			cursorId.charset        = SQLCHARSETSTRING_UTF8;
			cursorId.name_mode      = cursor_name;
			if (*cursorName == '\0')
				cursorId.identifier_len = pSrvrStmt->stmtNameLen;
			else
				cursorId.identifier_len = pSrvrStmt->cursorNameLen;
			cursorId.identifier     = cursorName;
			strcpy(pSrvrStmt->previousCursorName, cursorName);	// keep track of last cursorName
			retcode = WSQL_EXEC_SetCursorName(pStmt, &cursorId);
			HANDLE_ERROR(retcode, sqlWarning);
		}
	}
	pDesc = &pSrvrStmt->inputDesc;
	paramCount = pSrvrStmt->paramCount;
	
	if (paramCount == 0) // Ignore inputValueList
	{
		if ( pSrvrStmt->stmtType == EXTERNAL_STMT )
		{
			if ( sqlStmtType == TYPE_SELECT  && (pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE || pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE) )
					retcode = WSQL_EXEC_Exec(pStmt, NULL, 0);
			else
					retcode = execretcode = WSQL_EXEC_ExecFetch(pStmt, NULL, 0);
		}
		else
		{
			if ( sqlStmtType == TYPE_SELECT )
					retcode = WSQL_EXEC_Exec(pStmt, NULL, 0);
			else
					retcode = execretcode = WSQL_EXEC_ExecFetch(pStmt, NULL, 0);
		}
		HANDLE_ERROR(retcode, sqlWarning);
		if (WSQL_EXEC_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &rowsAffected_cli, NULL, 0, NULL) < SQL_SUCCESS)
			t_rowsAffected = -1;
		else
			t_rowsAffected += rowsAffected_cli;
	}
	else
	{
		// Else copy from inputValueList to SQL area
		if ((inputRowCnt * paramCount) > (Int32)inputValueList->_length)
			return SQL_NEED_DATA;
		SQLValue_def *p_buffer = (SQLValue_def *)inputValueList->_buffer;
		SRVR_DESC_HDL *p_IPD = pSrvrStmt->IPD;

		void* v_buffer;
		int v_length;
		int v_dataInd;

		for (curRowCnt = 1, curLength = 0; curRowCnt <= inputRowCnt ; curRowCnt++)
		{
			for (curParamNo = 0 ; curParamNo < paramCount ; curParamNo++, curLength++)
			{
				SQLValue = p_buffer + curLength;
				v_buffer = SQLValue->dataValue._buffer;
				v_length = SQLValue->dataValue._length;
				v_dataInd = SQLValue->dataInd;

				IPD = p_IPD+curParamNo;
				indPtr = IPD->indPtr;
				varPtr = IPD->varPtr;

				if (indPtr != NULL)
					*((short *)indPtr) = v_dataInd == -1? -1: 0;
				else
					if (v_dataInd == -1)
						return ODBC_SERVER_ERROR;

				if (v_dataInd != -1)
					memcpy(varPtr, v_buffer, v_length);
			}
			if ( pSrvrStmt->stmtType == EXTERNAL_STMT )
			{
				if ( sqlStmtType == TYPE_SELECT  && (pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE || pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE) )
					retcode = WSQL_EXEC_Exec(pStmt, pDesc, 0);
				else
					retcode = execretcode = WSQL_EXEC_ExecFetch(pStmt, pDesc, 0);
			}
			else
			{
				if ( sqlStmtType == TYPE_SELECT )
					retcode = WSQL_EXEC_Exec(pStmt, pDesc, 0);
				else
					retcode = execretcode = WSQL_EXEC_ExecFetch(pStmt, pDesc, 0);
			}
			// At present, we return error for this error condition
			// We could think of skipping this row and contine with the next row
			// at a later time
			HANDLE_ERROR(retcode, sqlWarning);
			if (WSQL_EXEC_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &rowsAffected_cli, NULL, 0, NULL) < SQL_SUCCESS)
				t_rowsAffected = -1;
			else
				t_rowsAffected += rowsAffected_cli;
		}
	}
	*rowsAffected = (Int32)(t_rowsAffected << 32 >> 32);
	*rowsAffectedHigherBytes = (Int32)(t_rowsAffected>>32);
	SRVRTRACE_EXIT(FILE_INTF+13);

	// Initialize this to TRUE later in this function depending on sqlStmtType if SQL_SELECT
	// then and retcode is SUCCESS or SUCCESS_WITH_INFO set it to FALSE. Since SQL opens CURSOR
	// after execute in case of SUCCESS or SUCCESS_WITH_INFO and closes only when it reaches 
	// NO_DATA_FOUND or CLOSE STATEMENT or COMMIT WORK or ROLLBACK WORK.
	if ( pSrvrStmt->stmtType == EXTERNAL_STMT )
	{
		 if ( sqlStmtType == TYPE_SELECT && (pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE || pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE) )
		 {
			pSrvrStmt->isClosed = FALSE;
			pSrvrStmt->bFetchStarted = FALSE;
		 }
	}
	else
	{
		if (sqlStmtType == TYPE_SELECT )
		{
			pSrvrStmt->isClosed = FALSE;
			pSrvrStmt->bFetchStarted = FALSE;
		}
	}

	// Added for fix to SQL returning sqlcode=SQL_NO_DATA_FOUND for non-select
	// stmts when no rows get affected 10/03/06
	if (sqlWarning && !(IGNORE_NODATAFOUND(sqlStmtType, execretcode)))
		return SQL_SUCCESS_WITH_INFO;
	else
		return SQL_SUCCESS;

} // SRVR::EXECUTE

//--------------------------------------------------------------------------
SQLRETURN SRVR::BuildSQLDesc2withRowsets( SQLDESC_ID          *pDesc
					, Int32                 sqlQueryType 
					, Int32                 maxRowsetSize
					, bool                &sqlBulkFetchPossible 
					, Int32                 numEntries   // #params + 1 for first quad ptr
					, BYTE               *&SQLDesc
					, Int32                &SQLDescLen
					, BYTE               *&varBuffer
					, Int32                &totalMemLen
				        , SRVR_DESC_HDL      *&implDesc
                                        , SQLCLI_QUAD_FIELDS *&inputQuadList
					, SQLCLI_QUAD_FIELDS *&inputQuadList_recover
                                        )
{
  SRVRTRACE_ENTER(FILE_INTF+40);

	Int32 FSDataType;   // added this to get NSK file system datatype 
			   //       to find numeric signed or unsigned.
	Int32 VarAlign;
	Int32 IndAlign;
	Int32 Version = 0;
	Int32 DataType;
	Int32 DateTimeCode;
	Int32 Length;
	Int32 Precision;
	Int32 Scale;
	Int32 Nullable;
	BOOL SignType;
	Int32 ODBCDataType;
	Int32 ODBCPrecision;
	Int32 SQLCharset;
	Int32 ODBCCharset;
	Int32 ColHeadingNmlen;
	Int32 TableNamelen;
	Int32 CatalogNamelen;
	Int32 SchemaNamlen;
	Int32 Headinglen;
	Int32 IntLeadPrec;
	Int32 paramMode;

	Int32 totalDescLength = 0;
	Int32 i, j;
	Int32 tempLen    = 0;
	Int32 retcode    = SQL_SUCCESS;
	BOOL sqlWarning = FALSE;
 	char Heading[MAX_ANSI_NAME_LEN];

	Int32  memOffSet = 0;
	BYTE *VarPtr;
	BYTE *IndPtr;
	BYTE *memPtr;
	
	struct ODBCDescriptors
	{
	  Int32 varAlign;
	  Int32 indAlign;
	  Int32 version;
	  Int32 dataType;
	  Int32 datetimeCode;
	  Int32 maxLen;
	  Int32 precision;
	  Int32 scale;
	  Int32 nullInfo;
	  Int32 signType;
	  Int32 ODBCDataType;
	  Int32 ODBCPrecision;
	  Int32 SQLCharset;
	  Int32 ODBCCharset;
	  Int32 colHeadingNmlen;
	  char colHeadingNm[MAX_ANSI_NAME_LEN+1];
	  Int32 TableNamelen;
	  char TableName[MAX_ANSI_NAME_LEN+1];
	  Int32 CatalogNamelen;
	  char CatalogName[MAX_ANSI_NAME_LEN+1];
	  Int32 SchemaNamlen;
	  char SchemaName[MAX_ANSI_NAME_LEN+1];
	  Int32 Headinglen;
	  char Heading[MAX_ANSI_NAME_LEN+1];
	  Int32 intLeadPrec;
	  Int32 paramMode;
	};
	
	typedef struct tagDESC_HDL_LIST
	{
		Int32 DataType;
		Int32 Length;
		Int32 Nullable;
		Int32 VarBuf;
		Int32 IndBuf;
	} DESC_HDL_LIST;

	DESC_HDL_LIST	*SqlDescInfo;
	Int32		 tempDescLen = 0;

	if (implDesc != NULL)
	{
		delete implDesc;
		implDesc = NULL;
	}

	if (inputQuadList != NULL)
	{
		delete inputQuadList;
		inputQuadList = NULL;
	}

	if (inputQuadList_recover != NULL)
	{
		delete inputQuadList_recover;
		inputQuadList_recover = NULL;
	}


	if (numEntries > 1)
	{

		markNewOperator,implDesc = new SRVR_DESC_HDL[numEntries - 1];
		if (implDesc == NULL)
		{
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "BuildSQLDesc");
			exit(0);
		}

		markNewOperator,inputQuadList = new SQLCLI_QUAD_FIELDS[numEntries];
		if (inputQuadList == NULL)
		{
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "BuildSQLDesc");
			exit(0);
		}
                markNewOperator,inputQuadList_recover = new SQLCLI_QUAD_FIELDS[numEntries];
		if (inputQuadList_recover == NULL)
		{
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "BuildSQLDesc");
			exit(0);
		}


		// Setup ind_layout in quad ptr list.
                for (i = 0; i < numEntries; i++)
                  inputQuadList[i].ind_layout = 2;  // 2 byte indicator

		markNewOperator,SqlDescInfo = new DESC_HDL_LIST[numEntries - 1];
		if (SqlDescInfo == NULL)
		{
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "BuildSQLDesc2");
			exit(0);
		}

		tempDescLen	= sizeof(totalMemLen) + sizeof(numEntries) + (sizeof(ODBCDescriptors) * (numEntries - 1));
		markNewOperator,SQLDesc = new BYTE[tempDescLen];
		if (SQLDesc == NULL)
		{
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "BuildSQLDesc2");
			exit(0);
		}

		*(Int32 *)(SQLDesc+totalDescLength) = 0; // Initialize totalMemLen, Since its calculated later
		totalDescLength += sizeof(totalMemLen);

		*(Int32 *)(SQLDesc+totalDescLength) = numEntries - 1;
		totalDescLength += sizeof(numEntries);

	}

	if (srvrGlobal->drvrVersion.componentId == JDBC_DRVR_COMPONENT && srvrGlobal->drvrVersion.majorVersion == 3 && srvrGlobal->drvrVersion.minorVersion <= 1)
		sqlBulkFetchPossible = false;
	else
		sqlBulkFetchPossible = true;

	for (i = 2, totalMemLen = 0; i <= numEntries; i++)
	{
		// Initialize the desc entry in SQLDESC_ITEM struct
		for (j = 0; j < NO_OF_DESC_ITEMS ; j++)
		{
			gDescItems[j].entry = i;
		}
		gDescItems[10].num_val_or_len = MAX_ANSI_NAME_LEN;
		gDescItems[11].num_val_or_len = MAX_ANSI_NAME_LEN;
		gDescItems[12].num_val_or_len = MAX_ANSI_NAME_LEN;
		gDescItems[13].num_val_or_len = MAX_ANSI_NAME_LEN;
		gDescItems[14].num_val_or_len = MAX_ANSI_NAME_LEN;

		retcode = WSQL_EXEC_GetDescItems2(pDesc,
					NO_OF_DESC_ITEMS,
					(SQLDESC_ITEM *)&gDescItems);
		HANDLE_ERROR(retcode, sqlWarning);

		// This change is made for ADO since it can't handle 'COLUMN HEADING' since
		// driver returns this as SQL_DESC_LABEL, So change it to 'COLUMN NAME'.
		// Temperaryly use Heading to send NULL and in future replace ColumnHeading
		// with Heading.
		Heading[0] = '\0';

		DataType     = gDescItems[0].num_val_or_len;
		Length       = gDescItems[1].num_val_or_len;
		Precision    = gDescItems[2].num_val_or_len;
		Scale        = gDescItems[3].num_val_or_len;
		Nullable     = gDescItems[4].num_val_or_len;
		paramMode    = gDescItems[5].num_val_or_len;
		IntLeadPrec  = gDescItems[6].num_val_or_len;
		DateTimeCode = gDescItems[7].num_val_or_len;
		SQLCharset   = gDescItems[8].num_val_or_len;
		FSDataType   = gDescItems[9].num_val_or_len;
		CatalogName[gDescItems[10].num_val_or_len]   = '\0';
		SchemaName[gDescItems[11].num_val_or_len]    = '\0';
		TableName[gDescItems[12].num_val_or_len]     = '\0';
		DescName[gDescItems[13].num_val_or_len]      = '\0';
		ColumnHeading[gDescItems[14].num_val_or_len] = '\0';

		// Added this check since SQL started returning SQLTYPECODE_NUMERIC.
		// Instead of changing on driver side Datatype is changed here to work correctly.
		if (DataType == SQLTYPECODE_NUMERIC || DataType == SQLTYPECODE_NUMERIC_UNSIGNED)
		{
			switch (FSDataType)
			{
				case 130:
					DataType = SQLTYPECODE_SMALLINT;
					break;
				case 131:
					DataType = SQLTYPECODE_SMALLINT_UNSIGNED;
					break;
				case 132:
					DataType = SQLTYPECODE_INTEGER;
					break;
				case 133:
					DataType = SQLTYPECODE_INTEGER_UNSIGNED;
					break;
				case 134:
					DataType = SQLTYPECODE_LARGEINT;
					break;					
				case 136:
					DataType = SQLTYPECODE_TINYINT;
					break;
				case 137:
					DataType = SQLTYPECODE_TINYINT_UNSIGNED;
					break;
				case 138:
					DataType = SQLTYPECODE_LARGEINT_UNSIGNED;
					break;					

				default:
					break;
			}
		}

		if (sqlBulkFetchPossible)
		{
			if (!Nullable)
			{
				switch (DataType)
				{
					case SQLTYPECODE_VARCHAR:
					case SQLTYPECODE_VARCHAR_WITH_LENGTH:
					case SQLTYPECODE_VARCHAR_LONG:
					case SQLTYPECODE_DATETIME:
					case SQLTYPECODE_INTERVAL:
					case SQLTYPECODE_BLOB:
					case SQLTYPECODE_CLOB:
						sqlBulkFetchPossible = false;
						break;
				}
			}
			else
				sqlBulkFetchPossible = false;
		}

		retcode = GetODBCValues( DataType
                                       , DateTimeCode
                                       , Length
                                       , Precision
                                       , ODBCDataType 
				       , ODBCPrecision
                                       , SignType, Nullable
                                       , totalMemLen
                                       , SQLCharset
                                       , ODBCCharset
				       , IntLeadPrec
                                       , ColumnHeading);
		HANDLE_ERROR(retcode, sqlWarning);
		if (DataType == SQLTYPECODE_LARGEINT && Precision == 0 && Scale > 0)
			ODBCDataType = SQL_NUMERIC;

		implDesc[i-2].charSet = SQLCharset;
		implDesc[i-2].dataType  = DataType;
		implDesc[i-2].length    = Length;
		implDesc[i-2].paramMode = paramMode;

		SqlDescInfo[i-2].DataType = DataType;
		SqlDescInfo[i-2].Length   = Length;

                inputQuadList[i-1].var_layout = Length;

		SqlDescInfo[i-2].Nullable = Nullable;
		SqlDescInfo[i-2].VarBuf   = totalDescLength;
		totalDescLength          += sizeof(VarAlign);
		SqlDescInfo[i-2].IndBuf   = totalDescLength;
		totalDescLength          += sizeof(IndAlign);

		*(Int32 *)(SQLDesc+totalDescLength) = Version;
		totalDescLength += sizeof(Version);

		*(Int32 *)(SQLDesc+totalDescLength) = DataType;
		totalDescLength += sizeof(DataType);

		*(Int32 *)(SQLDesc+totalDescLength) = DateTimeCode;
		totalDescLength += sizeof(DateTimeCode);
		
		*(Int32 *)(SQLDesc+totalDescLength) = Length;
		totalDescLength += sizeof(Length);

		*(Int32 *)(SQLDesc+totalDescLength) = Precision;
		totalDescLength += sizeof(Precision);

		*(Int32 *)(SQLDesc+totalDescLength) = Scale;
		totalDescLength += sizeof(Scale);

		*(Int32 *)(SQLDesc+totalDescLength) = Nullable;
		totalDescLength += sizeof(Nullable);

		if (SignType)  // This may change if SQL returns sign and since desc will be Int32 return as Int32
			*(Int32 *)(SQLDesc+totalDescLength) = 1;
		else
			*(Int32 *)(SQLDesc+totalDescLength) = 0;
		totalDescLength += sizeof(Int32);

		*(Int32 *)(SQLDesc+totalDescLength) = ODBCDataType;
		totalDescLength += sizeof(ODBCDataType);

		*(Int32 *)(SQLDesc+totalDescLength) = ODBCPrecision;
		totalDescLength += sizeof(ODBCPrecision);

		*(Int32 *)(SQLDesc+totalDescLength) = SQLCharset;
		totalDescLength += sizeof(SQLCharset);
		
		*(Int32 *)(SQLDesc+totalDescLength) = ODBCCharset;
		totalDescLength += sizeof(ODBCCharset);
		
		ColHeadingNmlen = gDescItems[13].num_val_or_len+1; // Null terminator
		*(Int32 *)(SQLDesc+totalDescLength) = ColHeadingNmlen;
		totalDescLength += sizeof(ColHeadingNmlen);
		if (ColHeadingNmlen > 0)
		{
			memcpy(SQLDesc+totalDescLength,DescName,ColHeadingNmlen);
			totalDescLength += ColHeadingNmlen;
		}
		
		TableNamelen = gDescItems[12].num_val_or_len+1;
		*(Int32 *)(SQLDesc+totalDescLength) = TableNamelen;
		totalDescLength += sizeof(TableNamelen);
		if (TableNamelen > 0)
		{
			memcpy(SQLDesc+totalDescLength,TableName,TableNamelen);
			totalDescLength += TableNamelen;
		}

		CatalogNamelen = gDescItems[10].num_val_or_len+1;
		*(Int32 *)(SQLDesc+totalDescLength) = CatalogNamelen;
		totalDescLength += sizeof(CatalogNamelen);
		if (CatalogNamelen > 0)
		{
			memcpy(SQLDesc+totalDescLength,CatalogName,CatalogNamelen);
			totalDescLength += CatalogNamelen;
		}
		
		SchemaNamlen = gDescItems[11].num_val_or_len+1;
		*(Int32 *)(SQLDesc+totalDescLength) = SchemaNamlen;
		totalDescLength += sizeof(SchemaNamlen);
		if (SchemaNamlen > 0)
		{
			memcpy(SQLDesc+totalDescLength,SchemaName,SchemaNamlen);
			totalDescLength += SchemaNamlen;
		}
		
		// Headinglen = gDescItems[14].num_val_or_len+1; // Change this when we start supporting strlen(ColumnHeading);
		Headinglen = 1;
		*(Int32 *)(SQLDesc+totalDescLength) = Headinglen;
		totalDescLength += sizeof(Headinglen);
		if (Headinglen > 0)
		{
		//	memcpy(SQLDesc+totalDescLength,ColumnHeading,Headinglen); // Change this when we start supporting strlen(ColumnHeading);
			memcpy(SQLDesc+totalDescLength,Heading,Headinglen);
			totalDescLength += Headinglen;
		}

		*(Int32 *)(SQLDesc+totalDescLength) = IntLeadPrec;
		totalDescLength += sizeof(IntLeadPrec);

		*(Int32 *)(SQLDesc+totalDescLength) = paramMode;
		totalDescLength += sizeof(paramMode);
	}  // end for
	
	// Adjust totalMemLen to word boundary
	totalMemLen = ((totalMemLen + 8 - 1) >> 3) << 3; 

	if (varBuffer != NULL)
	{
		delete varBuffer;
	}
	varBuffer = NULL;
	if (sqlBulkFetchPossible && sqlQueryType == SQL_SELECT_NON_UNIQUE)
		markNewOperator,varBuffer = new BYTE[srvrGlobal->m_FetchBufferSize];
	else
                // KAS This probably needs to be removed if I end up using the input buffer for the
	        // quad pointers to point too.
	        markNewOperator,varBuffer = new BYTE[totalMemLen]; 
	if (varBuffer == NULL)
	{
		// Handle Memory Overflow execption here
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "BuildSQLDesc2");
		exit(0);
	}

	*(Int32 *)SQLDesc = totalMemLen;

	memPtr = varBuffer ;
	memOffSet = 0;

	for (i = 0 ; i < numEntries - 1 ; i++)
	{
		switch (SqlDescInfo[i].DataType)
		{
		case SQLTYPECODE_CHAR:
		case SQLTYPECODE_VARCHAR:
                case SQLTYPECODE_BINARY:
			VarPtr = memPtr + memOffSet;					
			memOffSet += SqlDescInfo[i].Length + 1;
			break;
		case SQLTYPECODE_VARCHAR_WITH_LENGTH:
		case SQLTYPECODE_VARCHAR_LONG:
		case SQLTYPECODE_BLOB:
		case SQLTYPECODE_CLOB:
                case SQLTYPECODE_VARBINARY:
			if( SqlDescInfo[i].Length > SHRT_MAX )
			{
				memOffSet = ((memOffSet + 4 - 1) >> 2) << 2;
				VarPtr = memPtr + memOffSet;
				memOffSet += SqlDescInfo[i].Length + 5;
			}
			else
			{
				memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
				VarPtr = memPtr + memOffSet;
				memOffSet += SqlDescInfo[i].Length + 3;
			}
			// Adjust quad var_layout to even byte boundry
			if (SqlDescInfo[i].Length == ((SqlDescInfo[i].Length >> 1) << 1))
				inputQuadList[i+1].var_layout = SqlDescInfo[i].Length;
			else
				inputQuadList[i+1].var_layout = SqlDescInfo[i].Length + 1;
			break;
                case SQLTYPECODE_BOOLEAN:
                case SQLTYPECODE_TINYINT:
                case SQLTYPECODE_TINYINT_UNSIGNED:
			VarPtr = memPtr + memOffSet;
			memOffSet += SqlDescInfo[i].Length;
			break;
		case SQLTYPECODE_SMALLINT:
		case SQLTYPECODE_SMALLINT_UNSIGNED:
			memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
			VarPtr = memPtr + memOffSet;
			memOffSet += SqlDescInfo[i].Length;
			break;
		case SQLTYPECODE_INTEGER:
		case SQLTYPECODE_INTEGER_UNSIGNED:
			memOffSet = ((memOffSet + 4 - 1) >> 2) << 2; 
			VarPtr = memPtr + memOffSet;
			memOffSet += SqlDescInfo[i].Length;
			break;
		case SQLTYPECODE_LARGEINT:
		case SQLTYPECODE_LARGEINT_UNSIGNED:
		case SQLTYPECODE_IEEE_REAL:
		case SQLTYPECODE_IEEE_FLOAT:
		case SQLTYPECODE_IEEE_DOUBLE:
			memOffSet = ((memOffSet + 8 - 1) >> 3) << 3; 
			VarPtr = memPtr + memOffSet;
			memOffSet += SqlDescInfo[i].Length;
			break;
		case SQLTYPECODE_DECIMAL_UNSIGNED:
		case SQLTYPECODE_DECIMAL:
		case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
		case SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
		case SQLTYPECODE_INTERVAL:		// Treating as CHAR
		case SQLTYPECODE_DATETIME:
			VarPtr = memPtr + memOffSet;
			memOffSet += SqlDescInfo[i].Length;
			break;
		default:
			memOffSet = ((memOffSet + 8 - 1) >> 3) << 3; 
			VarPtr = memPtr + memOffSet;
			memOffSet += SqlDescInfo[i].Length;
			break;
		}
		*(Int32 *)(SQLDesc+SqlDescInfo[i].VarBuf) = (Int32)(VarPtr-memPtr);

		if (SqlDescInfo[i].Nullable)
		{
			memOffSet = ((memOffSet + 2 - 1) >> 1) << 1; 
			IndPtr = memPtr + memOffSet;
			*(Int32 *)(SQLDesc+SqlDescInfo[i].IndBuf) = (Int32)(IndPtr-memPtr);
			memOffSet += 2 ;
		}
		else
		{
			IndPtr = NULL;
			*(Int32 *)(SQLDesc+SqlDescInfo[i].IndBuf) = (Int32)(-1);
		}

		implDesc[i].varPtr = VarPtr;
		implDesc[i].indPtr = IndPtr;

		if (memOffSet > totalMemLen)
		{
			SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
					1, "memOffSet > totalMemLen in SetDataPtr");
			return PROGRAM_ERROR;
		}
	}

	SQLDescLen = totalDescLength;

	if (SqlDescInfo != NULL)
	{
		delete [] SqlDescInfo;
		SqlDescInfo = NULL;
	}

	if (sqlBulkFetchPossible && sqlQueryType == SQL_SELECT_NON_UNIQUE)
	{
		Int32 TmpMaxRows = (srvrGlobal->m_FetchBufferSize)/totalMemLen;
		retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWSET_TYPE, 2, 0);
		HANDLE_ERROR(retcode, sqlWarning);

		retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWWISE_ROWSET_PTR, (long)varBuffer, 0);
		HANDLE_ERROR(retcode, sqlWarning);

		retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWWISE_ROWSET_SIZE, TmpMaxRows, 0);
		HANDLE_ERROR(retcode, sqlWarning);

		retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWWISE_ROWSET_ROW_LEN, totalMemLen, 0);
		HANDLE_ERROR(retcode, sqlWarning);
	}
	
	SRVRTRACE_EXIT(FILE_INTF+40);

	if (sqlWarning)
		return SQL_SUCCESS_WITH_INFO;
	else
		return SQL_SUCCESS;

}  // BuildSQLDesc2withRowsets

void SRVR::INSERT_NODE_TO_LIST(SRVR_STMT_HDL* pSrvrStmt, ROWSET_ERROR_NODE* pNode, Int32 rowCount)
{
	SRVRTRACE_ENTER(FILE_INTF+16);

	ROWSET_ERROR_NODE* currNode=NULL, *nextNode=NULL, *prevNode=NULL;

	currNode = pSrvrStmt->rowsetErrorList.firstNode;

	while (currNode != NULL)
	{
		if (currNode->rowNumber < rowCount) // insert after it
		{
			nextNode = currNode->nextNode;
			currNode->nextNode = pNode;
			pNode->nextNode = nextNode;
			break;
		}
		prevNode = currNode;
		currNode = currNode->nextNode;
	}

	if (currNode == NULL)
	{
		if (prevNode == NULL) // list is empty
		{
			pSrvrStmt->rowsetErrorList.firstNode = pNode;
		}
		else				// insert after last element
		{
			prevNode->nextNode = pNode;
		}
		pNode->nextNode = NULL;
	}
	pSrvrStmt->rowsetErrorList.nodeCount++;
	SRVRTRACE_EXIT(FILE_INTF+16);

	return;
} 

SQLRETURN SRVR::RECOVERY_FROM_ROWSET_ERROR2( SRVR_STMT_HDL *pSrvrStmt
		                           , SQLDESC_ID	   *pDesc
		                           , SQLSTMT_ID	   *pStmt
		                           , Int32       inputRowCnt
		                           , Int64      *totalrowsAffected
                                           )
  {
  SRVRTRACE_ENTER(FILE_INTF+41);

  short                     *TypePtr;
  struct SQLCLI_QUAD_FIELDS *QuadPtr_original;
  struct SQLCLI_QUAD_FIELDS *QuadPtr;
  short                     *curTypePtr;
  struct SQLCLI_QUAD_FIELDS *curQuadPtr;

  SQLRETURN            retcode;
  odbc_SQLSvc_SQLError SQLError;
  Int32                 paramCount        = pSrvrStmt->paramCount;
  Int32                 curParamNo;
  Int32                 sqlRowCount;
  Int32                 currentRowCount   = 0;
  Int32                 prevRowCount      = 0;
  Int32                 currentRowsetSize = 0;
  Int64                 rowsAffected      = 0;
  Int32             RowsetSize;
  Int32                 DataType;
  Int32                 DataLen;
  Int32                 sqlMaxLength;
  Int32                 quad_length;
  SQLRETURN execRetCode;	// Added for MODE_SPECIAL_1 behavior

  RowsetSize        = inputRowCnt;
  currentRowsetSize = RowsetSize;
  quad_length       = sizeof(SQLCLI_QUAD_FIELDS) * paramCount;
  QuadPtr_original  = pSrvrStmt->inputQuadList;
  QuadPtr           = pSrvrStmt->inputQuadList_recover;


	Int32 prevErrRow = -1;

  memcpy(QuadPtr, QuadPtr_original, quad_length);

  *totalrowsAffected = -1;

  retcode = GETSQLERROR_AND_ROWCOUNT(pSrvrStmt->bSQLMessageSet, &SQLError,RowsetSize, currentRowCount, &sqlRowCount);
  if (sqlRowCount == -1)
    {
    ADDSQLERROR_TO_LIST( pSrvrStmt, &SQLError, currentRowCount + 1);
    return SQL_ERROR;
    }
  else
    ADDSQLERROR_TO_LIST( pSrvrStmt, &SQLError, currentRowCount + sqlRowCount+1);

  if (sqlRowCount == 0)
    {
  		//for Surrogate key AR
		if (SQLError.errorList._buffer->sqlcode == -8108 )    //if the first row
		{
			prevRowCount = currentRowCount;
			prevErrRow   = currentRowCount;
		}
		//for Surrogate key AR
		else
		{
		prevRowCount = currentRowCount;
		currentRowCount++;
		currentRowsetSize--;
		}

    }
  else
    {
    prevRowCount      = currentRowCount;
    currentRowsetSize = sqlRowCount;
    }

  while( currentRowCount < RowsetSize)
    {
    QuadPtr->var_ptr = &currentRowsetSize;

    for (curParamNo = 1 ; curParamNo < paramCount ; curParamNo++)
      {
      curQuadPtr = QuadPtr + curParamNo;
      DataType   = pSrvrStmt->IPD[curParamNo - 1].dataType;
      DataLen    = curQuadPtr->var_layout;

      switch (DataType)
	{
	case SQLTYPECODE_VARCHAR_WITH_LENGTH:
	case SQLTYPECODE_VARCHAR_LONG:
	case SQLTYPECODE_BITVAR:
	case SQLTYPECODE_BLOB:
	case SQLTYPECODE_CLOB:
             sqlMaxLength = DataLen + 2;
	     sqlMaxLength = ((sqlMaxLength + 2 - 1)>>1)<<1;
	     break;
	default:
	    sqlMaxLength = DataLen;
	    break;
	}  // end switch

      curQuadPtr->var_ptr = (BYTE*)curQuadPtr->var_ptr + (sqlMaxLength * (currentRowCount - prevRowCount));

      if (curQuadPtr->ind_ptr != NULL)
        curQuadPtr->ind_ptr = (BYTE*)curQuadPtr->ind_ptr + ((currentRowCount - prevRowCount) * sizeof(short));
      }  // end for

    retcode = WSQL_EXEC_SETROWSETDESCPOINTERS( pDesc
					     , pSrvrStmt->maxRowsetSize
					     , NULL
					     , 1
					     , paramCount
					     , QuadPtr
                                             );
    if (retcode < SQL_SUCCESS)
      {
      GETSQLERROR_AND_ROWCOUNT(pSrvrStmt->bSQLMessageSet, &SQLError, RowsetSize, currentRowCount, &sqlRowCount);
      ADDSQLERROR_TO_LIST( pSrvrStmt, &SQLError, currentRowCount + 1);
      return SQL_ERROR;
      }
    retcode = execRetCode = WSQL_EXEC_ExecFetch(pStmt, pDesc, 0);
    if (retcode < SQL_SUCCESS)
      {
      GETSQLERROR_AND_ROWCOUNT(pSrvrStmt->bSQLMessageSet, &SQLError, RowsetSize, currentRowCount, &sqlRowCount);
      if (sqlRowCount == -1)
	{
        ADDSQLERROR_TO_LIST( pSrvrStmt, &SQLError, currentRowCount + 1);
	return SQL_ERROR;
	}
      else
	{
	if  (SQLError.errorList._buffer->sqlcode != -8108) 
	ADDSQLERROR_TO_LIST( pSrvrStmt, &SQLError, currentRowCount + sqlRowCount+1);
	}
      if (sqlRowCount == 0)
	  {
		  				//for Surrogate key AR
			   			if ((SQLError.errorList._buffer->sqlcode == -8108 ))
						{ 
							if ( (prevErrRow == currentRowCount) )
							{
								ADDSQLERROR_TO_LIST( pSrvrStmt, &SQLError, currentRowCount+1);
								prevRowCount = currentRowCount;
								prevErrRow   = currentRowCount;
								currentRowCount++ ;
								currentRowsetSize  = RowsetSize - currentRowCount;
							}
							else
							{
								prevRowCount = currentRowCount;
								prevErrRow   = currentRowCount;
								currentRowsetSize  = RowsetSize - currentRowCount;
							}
						}
						else
						{
								prevRowCount = currentRowCount;
								currentRowCount++;
								currentRowsetSize  = RowsetSize - currentRowCount;
						}
	  }

	  else
        {
		prevRowCount = currentRowCount;
		currentRowsetSize = sqlRowCount;
		}
      }  // end if retcode < SQL_SUCCESS
    else
      {
      if (retcode > 0)
         WSQL_EXEC_ClearDiagnostics(NULL);
      if (WSQL_EXEC_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &rowsAffected, NULL, 0, NULL) < SQL_SUCCESS)
			return SQL_ERROR;
      retcode = COMMIT_ROWSET(pSrvrStmt->bSQLMessageSet, &SQLError, currentRowCount);
      if (retcode != SQL_SUCCESS)
		{
			ADDSQLERROR_TO_LIST( pSrvrStmt, &SQLError, currentRowCount + 1);
			return SQL_ERROR;
        }

		// Added for MODE_SPECIAL_1 behavior
		if (srvrGlobal->modeSpecial_1 && pSrvrStmt->sqlStmtType == TYPE_INSERT_PARAM) { 
			if ( *totalrowsAffected == -1)
				*totalrowsAffected = currentRowsetSize;	
			else
				*totalrowsAffected += currentRowsetSize;
		}
		else {
			if ( *totalrowsAffected == -1)
				*totalrowsAffected = rowsAffected;
			else
				*totalrowsAffected += rowsAffected;
		}

	  						//for Surrogate key AR
						if ( (SQLError.errorList._buffer->sqlcode == -8108 ))
							{
								prevRowCount = currentRowCount;
								if( srvrGlobal->modeSpecial_1 && (execRetCode == SQL_SUCCESS || execRetCode == SQL_SUCCESS_WITH_INFO || 
									execRetCode == SQL_NO_DATA_FOUND) )
									rowsAffected = currentRowsetSize;
								if (pSrvrStmt->sqlStmtType == TYPE_UPDATE || pSrvrStmt->sqlStmtType == TYPE_DELETE) // should not touch for update/delete
									rowsAffected = currentRowsetSize;
								currentRowCount += rowsAffected ;
								prevErrRow  = currentRowCount;
								currentRowsetSize  = RowsetSize - currentRowCount;
							}
						else
						//for Surrogate key AR
							{
	            			  prevRowCount = currentRowCount;
								// Added for MODE_SPECIAL_1 behavior
								// In the following conditions the rowsAffected should be the same as 
								// the currentRowsetSize. If all rows are duplicate in the rowset then
								// SQL returns SQL_NO_DATA_FOUND (100)
								if( srvrGlobal->modeSpecial_1 && (execRetCode == SQL_SUCCESS || execRetCode == SQL_SUCCESS_WITH_INFO || 
									execRetCode == SQL_NO_DATA_FOUND) )
									rowsAffected = currentRowsetSize;
								if (pSrvrStmt->sqlStmtType == TYPE_UPDATE || pSrvrStmt->sqlStmtType == TYPE_DELETE) // should not touch for update/delete
									rowsAffected = currentRowsetSize;
							  currentRowCount += rowsAffected + 1;
							  currentRowsetSize  = RowsetSize - currentRowCount;
							} 
	} // end else
    }  // end while
	
  SRVRTRACE_EXIT(FILE_INTF+41);

  //return SQL_SUCCESS_WITH_INFO;
  if ( *totalrowsAffected == -1)  *totalrowsAffected =0;

  return SQL_SUCCESS;

}  // end RECOVERY_FROM_ROWSET_ERROR2

//----------------------------------------------------------------------

void SRVR::COPYSQLERROR_LIST_TO_SRVRSTMT(SRVR_STMT_HDL* pSrvrStmt)
{
	SRVRTRACE_ENTER(FILE_INTF+17);

	Int32 i;
	Int32 totallength = pSrvrStmt->sqlWarning._length;
	ERROR_DESC_def* pErrorDesc;
	ROWSET_ERROR_NODE* currNode, *nextNode;
	if (pSrvrStmt->rowsetErrorList.nodeCount == 0)
		return;
	pSrvrStmt->bSQLMessageSet = true;
	currNode = pSrvrStmt->rowsetErrorList.firstNode;
	for (i = 0; i < pSrvrStmt->rowsetErrorList.nodeCount; i++)
	{
		totallength += currNode->SQLError.errorList._length;
		currNode = currNode->nextNode;
	}
	if (totallength == 0)
		return;
	ERROR_DESC_def* _buffer = new ERROR_DESC_def[totallength];
	if (_buffer == NULL)
	{
		// Handle Memory Overflow execption here
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "COPYSQLERROR_LIST_TO_SRVRSTMT");
		exit(0);
	}
	pErrorDesc = _buffer;
	if (pSrvrStmt->sqlWarning._length > 0)
	{
		memcpy(pErrorDesc, pSrvrStmt->sqlWarning._buffer, pSrvrStmt->sqlWarning._length * sizeof(ERROR_DESC_def));
		pErrorDesc += pSrvrStmt->sqlWarning._length;
		delete pSrvrStmt->sqlWarning._buffer;
	}

	currNode = pSrvrStmt->rowsetErrorList.firstNode;
		
	for (i = 0; i < pSrvrStmt->rowsetErrorList.nodeCount; i++)
	{
		memcpy(pErrorDesc, currNode->SQLError.errorList._buffer, currNode->SQLError.errorList._length * sizeof(ERROR_DESC_def));
		pErrorDesc += currNode->SQLError.errorList._length;
		currNode = currNode->nextNode;
	}
	pSrvrStmt->sqlWarning._length = totallength;
	pSrvrStmt->sqlWarning._buffer = _buffer;

	currNode = pSrvrStmt->rowsetErrorList.firstNode;
	while (currNode != NULL)
	{
		nextNode = currNode->nextNode;
		delete currNode;
		currNode = nextNode;
	}
	pSrvrStmt->rowsetErrorList.nodeCount = 0;
	pSrvrStmt->rowsetErrorList.firstNode = NULL;

	SRVRTRACE_EXIT(FILE_INTF+17);
	return;
}

void SRVR::ADDSQLERROR_TO_LIST(
		SRVR_STMT_HDL* pSrvrStmt,
		odbc_SQLSvc_SQLError *SQLError, 
		Int32 rowCount)
{
	SRVRTRACE_ENTER(FILE_INTF+18);

	ERROR_DESC_def *errorDesc;
	UInt32 i;

	ROWSET_ERROR_NODE* pNode = new ROWSET_ERROR_NODE();
	if (pNode == NULL)
	{
		// Handle Memory Overflow execption here
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "ADDSQLERROR_TO_LIST");
		exit(0);
	}

	pNode->rowNumber = rowCount;
	pNode->SQLError.errorList._length = SQLError->errorList._length;
	pNode->SQLError.errorList._buffer = SQLError->errorList._buffer;

	INSERT_NODE_TO_LIST(pSrvrStmt, pNode, rowCount);
	SRVRTRACE_EXIT(FILE_INTF+18);

	return;
}


SQLRETURN SRVR::COMMIT_ROWSET(bool& bSQLMessageSet, odbc_SQLSvc_SQLError* SQLError, Int32 currentRowCount)
{
	SRVRTRACE_ENTER(FILE_INTF+19);

	SQLRETURN retcode;
	SQLValueList_def inValueList;
	inValueList._buffer = NULL;
	inValueList._length = 0;

	SRVR_STMT_HDL	*CmwSrvrStmt = getSrvrStmt("STMT_COMMIT_1", FALSE);
	retcode = CmwSrvrStmt->Execute(NULL,1,TYPE_UNKNOWN,&inValueList,SQL_ASYNC_ENABLE_OFF,0);
	if (retcode == SQL_ERROR)
	{
		ERROR_DESC_def *error_desc_def = CmwSrvrStmt->sqlError.errorList._buffer;
		if (CmwSrvrStmt->sqlError.errorList._length != 0 )
		{
			if(error_desc_def->sqlcode != -8605 )
			{
				kdsCreateSQLErrorException(bSQLMessageSet,SQLError, 1);
				kdsCopySQLErrorExceptionAndRowCount(SQLError, error_desc_def->errorText, error_desc_def->sqlcode, error_desc_def->sqlstate, currentRowCount+1);
			}
			else
				retcode = SQL_SUCCESS;
		}
		else
		{
			kdsCreateSQLErrorException(bSQLMessageSet,SQLError, 1);
			kdsCopySQLErrorExceptionAndRowCount(SQLError, "Internal Error: From Commit Rowsets ", retcode, "", currentRowCount+1);
		}
	} 
	else if (retcode != SQL_SUCCESS)
	{
		kdsCreateSQLErrorException(bSQLMessageSet,SQLError, 1);
		kdsCopySQLErrorExceptionAndRowCount(SQLError, "Internal Error: From Commit Rowsets ", retcode, "", currentRowCount+1);
	}
	SRVRTRACE_EXIT(FILE_INTF+19);

	return retcode;
}

bool fatalSQLError(Int32 sqlcode)
{
/*
Nonfatal SQL errors:
 
-8101	This operation is prevented by a check constraint <check constraint name > on table <table name>.	   
-8102	The operation is prevented by a unique constraint. (Note that this message applies to the primary key only)	   
-8105	The operation is prevented by the check option on view <view name>.	   
-8108	Surrogate Key Error.	   
-8402	A string overflow occurred during the evaluation of a character expression.	   
-8403	The length argument of function SUBSTRING cannot be less than zero or greater than source string length.	   
-8404	The trim character argument of function TRIM must be one character in length.	   
-8405	The operand of function CONVERTTIMESTAMP is out of range.	   
-8409	The escape character argument of a LIKE predicate must be one character in length.	   
-8410	An escape character in a LIKE pattern must be followed by another escape character, an underscore, or a percent character.	   
-8411	A numeric overflow occurred during an arithmetic computation or data conversion	   
-8412	 An input character host variable is missing its null terminator.	   
-8413	The string argument contains characters that cannot be converted.	   
-8415	The provided DATE, TIME, or TIMESTAMP is not valid and cannot be converted.	   
-8416	A datetime expression evaluated to an invalid datetime value.	   
-8419	An arithmetic expression attempted a division by zero.	   
-8421	NULL cannot be assigned to a NOT NULL column	   
-8422	The provided INTERVAL is not valid and cannot be converted.	   
-8429	The preceding error actually occurred in function <function-name>	   
-8432	A negative value cannot be converted to an unsigned numeric datatype.	   
-8690	An invalid character value encountered in TRANSLATE function.
*/
	Int32 static Hsqlcode = -8101;
	Int32 static Lsqlcode = -8690;
	Int32 static Tsqlcode[] = {-8101,-8102,-8105,-8108,-8402,-8403,-8404,-8405,-8409,-8410,-8411,-8412,
		-8413,-8415,-8416,-8419,-8421,-8422,-8429,-8432,-8690};
	if (sqlcode > 0)
		return false;
	if (sqlcode > Hsqlcode || sqlcode < Lsqlcode)
		return true;
	for(int i=0; Tsqlcode[i] >= Lsqlcode; i++)
		if (sqlcode == Tsqlcode[i])
			return false;
	return true;
}

SQLRETURN SRVR::GETSQLERROR_AND_ROWCOUNT(
		bool& bSQLMessageSet, 
		odbc_SQLSvc_SQLError *SQLError,
		Int32 RowsetSize,
		Int32 currentRowCount,
		Int32* errorRowCount)
{
	SRVRTRACE_ENTER(FILE_INTF+20);

	Int32 retcode;
	Int32 total_conds = 0;
	char *msg_buf;
	Int32 buf_len;
	Int32 sqlcode  = 0;
	char sqlState[6];
	Int32 curr_cond = 1;
	Int32 msg_buf_len;
	ERROR_DESC_def *error_desc_def;
	Int32 sqlRowCount = -1;
	*errorRowCount = -1;
	char  strNow[TIMEBUFSIZE + 1];
		
	retcode =  WSQL_EXEC_GetDiagnosticsStmtInfo2(NULL,
											SQLDIAG_NUMBER,
																(Int64*)&total_conds, NULL, 0, NULL);
    if (total_conds == 0)
	{
		kdsCreateSQLErrorException(bSQLMessageSet,SQLError, 1);
		kdsCopySQLErrorExceptionAndRowCount(SQLError, "Internal Error: From GetStmtDiagnostics ", retcode, "", currentRowCount);
		return SQL_SUCCESS;
	}

	kdsCreateSQLErrorException(bSQLMessageSet,SQLError, total_conds);
	while (curr_cond <= total_conds)
	{
		retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_SQLCODE, curr_cond,
			&sqlcode, NULL, 0, NULL);
		if (retcode >= SQL_SUCCESS)
		{
			if (sqlcode == 100)
			{
				// We are not copying the Warning message if the error code is 100
				// It is ok, though we have allocated more SQLError, but length is incremented 
				// only when SQLError is copied
				curr_cond++;
				continue;
			}
			retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_MSG_LEN, curr_cond,
				&msg_buf_len, NULL, 0, NULL);
		}
		if (retcode >= SQL_SUCCESS)
		{
			msg_buf_len = (msg_buf_len+1)*4;
			msg_buf_len += TIMEBUFSIZE;
			msg_buf = new char[msg_buf_len];
			buf_len = 0;
			retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_MSG_TEXT, curr_cond,
				NULL, msg_buf, msg_buf_len, &buf_len);
		}
		if (retcode >= SQL_SUCCESS)
		{
			msg_buf[buf_len] = '\0';
			//Get the timetsamp
			time_t  now = time(NULL);
			bzero(strNow, sizeof(strNow) );
			strftime(strNow, sizeof(strNow), " [%Y-%m-%d %H:%M:%S]", localtime(&now));
			strcat(msg_buf, strNow);
			buf_len = 0;
			retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_RET_SQLSTATE, curr_cond,
				NULL, sqlState, sizeof(sqlState), &buf_len);
		}			
		if (retcode >= SQL_SUCCESS)
		{
			retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_ROW_NUMBER, curr_cond,
				&sqlRowCount, NULL, 0, NULL);
		}
		if (retcode >= SQL_SUCCESS)
			sqlState[5] = '\0';
		else
		{
			kdsCopySQLErrorExceptionAndRowCount(SQLError, "Internal Error : From GetCondDiagnostics ", retcode, "", currentRowCount);
			delete msg_buf;
			break;
		}
		kdsCopySQLErrorException(SQLError, msg_buf, sqlcode, sqlState);
		delete msg_buf;
		if (fatalSQLError(sqlcode))
		{
			*errorRowCount = -1;
			WSQL_EXEC_ClearDiagnostics(NULL);
			return SQL_ERROR;
		}
		if (sqlRowCount < 0 || sqlRowCount > RowsetSize)
			sqlRowCount = -1;
		if (sqlRowCount != -1) *errorRowCount = sqlRowCount;
		curr_cond++;
	}
	if (*errorRowCount != -1)
		currentRowCount += *errorRowCount;

	if (total_conds)
		bSQLMessageSet = true;

	curr_cond = 0;
	while(curr_cond < total_conds)
	{
		error_desc_def = SQLError->errorList._buffer + curr_cond;
		error_desc_def->rowId = currentRowCount + 1;
		curr_cond++;
	}

	WSQL_EXEC_ClearDiagnostics(NULL);
	SRVRTRACE_EXIT(FILE_INTF+20);

	return SQL_SUCCESS;
}

SQLRETURN SRVR::FREESTATEMENT(SRVR_STMT_HDL* pSrvrStmt)
{
	SRVRTRACE_ENTER(FILE_INTF+21);

	IDL_enum freeResourceOpt = pSrvrStmt->freeResourceOpt;
	Int32 *rowsAffected = &pSrvrStmt->rowsAffected;

	Int32 retcode;
	BOOL sqlWarning = FALSE;
	SRVR_STMT_HDL* pTmpSrvrStmt;
	SRVR_STMT_HDL* pTmp2SrvrStmt;
	bool done = false;
	
	SQLSTMT_ID *pStmt;
	SQLDESC_ID *pDesc;

	if(pSrvrStmt == NULL)
		return SQL_INVALID_HANDLE;

	pStmt = &pSrvrStmt->stmt;
	
	*rowsAffected = -1;
	switch( freeResourceOpt )
	{
	case SQL_DROP:
		// If we are de-allocating the SPJ CALL stmt then we
		// should also de-alloc any RS stmts associated with it.
		pTmpSrvrStmt = pSrvrStmt;
		pTmp2SrvrStmt = NULL;
		done = false;

		if (pSrvrStmt->sqlQueryType == SQL_CALL_WITH_RESULT_SETS && 
				pSrvrStmt->nextSpjRs != NULL)
			pSrvrStmt = pSrvrStmt->nextSpjRs;

		while( !done )
		{
			pStmt = &pSrvrStmt->stmt;
			pDesc = &pSrvrStmt->inputDesc;
			pSrvrStmt->freeBuffers(SQLWHAT_INPUT_DESC);
			if (pSrvrStmt->inputDescName[0] == '\0')
			{
				retcode = WSQL_EXEC_DeallocDesc(pDesc);
			}
			pDesc = &pSrvrStmt->outputDesc;
			pSrvrStmt->freeBuffers(SQLWHAT_OUTPUT_DESC);
			if (pSrvrStmt->outputDescName[0] == '\0')
			{
				retcode = WSQL_EXEC_DeallocDesc(pDesc);
			}
			if (! pSrvrStmt->isClosed)
			{
				retcode = WSQL_EXEC_CloseStmt(pStmt);
				pSrvrStmt->isClosed = TRUE;
			}
			if (pSrvrStmt->moduleName[0] == '\0')
			{
				retcode = WSQL_EXEC_DeallocStmt(pStmt);
			}	

			// If the RS stmt is passed to the method then only that stmt should be deleted
			if (pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET && pSrvrStmt != pTmpSrvrStmt)
			{	
				pTmp2SrvrStmt = pSrvrStmt;				
				if (pSrvrStmt->nextSpjRs == NULL)
				{
					pSrvrStmt = pTmpSrvrStmt;
					pSrvrStmt->nextSpjRs = NULL;
				}
				else
					pSrvrStmt = pSrvrStmt->nextSpjRs;

				removeSrvrStmt(pTmp2SrvrStmt);			
			}
			else
				done = true;
		}		
		DeallocateAdaptiveSegment(pSrvrStmt);
		removeSrvrStmt(pSrvrStmt);			
		listAllocatedMemory("FREESTATEMENT(SQL_DROP)");
		break;
	case SQL_CLOSE:
		if (! pSrvrStmt->isClosed)
		{
			retcode = WSQL_EXEC_CloseStmt(pStmt);
			if (pSrvrStmt->stmtType != INTERNAL_STMT)
			{
				if (retcode == -8811)
				{
					retcode = SQL_SUCCESS;
				}
				HANDLE_ERROR(retcode, sqlWarning);
			}
			pSrvrStmt->isClosed = TRUE;
		}
		break;
	case SQL_UNBIND:
		pDesc = &pSrvrStmt->outputDesc;
		pSrvrStmt->freeBuffers(SQLWHAT_OUTPUT_DESC);
		if (pSrvrStmt->outputDescName[0] == '\0')
		{
			retcode = WSQL_EXEC_DeallocDesc(pDesc);
		}
		break;
	case SQL_RESET_PARAMS:
		pDesc = &pSrvrStmt->inputDesc;
		pSrvrStmt->freeBuffers(SQLWHAT_INPUT_DESC);
		if (pSrvrStmt->inputDescName[0] == '\0')
		{
			retcode = WSQL_EXEC_DeallocDesc(pDesc);
		}
		break;
	default:
		break;
	}  
	SRVRTRACE_EXIT(FILE_INTF+21);

	if (sqlWarning)
		return SQL_SUCCESS_WITH_INFO;
	else
		return SQL_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function: RESOURCEGOV 
//
// Purpose: This function performs RG checking to stop or log resource-consuming queries.
//
// Scenario:
//          1. Perform RG checking only if (a). External statement, and (b). RG policies exist.
//          2. For the OLTP Beta2 release, we only handle the estimated cost case.
//          3. The RG policies are displayed in a descent order, and that's the order
//             we check RG policies. 
//          4. If the estimated cost exceeds the cost limit, we stop the query if the action
//             is "STOP". The function returns SQL_ERROR immediately, and we don't check the rest of RG policies if any.
//          5. If the estimated cost exceeds the cost limit, we log the query if the action
//             is "LOG". The function returns SQL_SUCCESS immediately, and we don't check the rest of RG policies if any.
//          6. If the estimated cost exceeds the cost limit, we log the query if the action
//             is "LOG_WITH_INFO". The function returns SQL_SUCCESS_WITH_INFO immediately, 
//			   and we don't check the rest of RG policies if any.
//
//
// Output: (SQLRETURN)SQL_ERROR if the estimated cost exceeds the cost limit, and if the action is "STOP".
//         (SQLRETURN)SQL_SUCCESS for "LOG"
//         (SQLRETURN)SQL_SUCCESS_WITH_INFO for "LOG_WITH_INFO"
//           
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SQLRETURN SRVR::RESOURCEGOV(SRVR_STMT_HDL* pSrvrStmt, 				   
				  char *pSqlStr,
				  double estimated_cost)
{
	/* Obsolete Code */
	int retcode = SQL_SUCCESS;  // Initialization
	return (SQLRETURN)retcode;  // for other cases
}

// Regular prepare
SQLRETURN SRVR::PREPARE(SRVR_STMT_HDL* pSrvrStmt)
{
	SRVRTRACE_ENTER(FILE_INTF+23);

	char *pSqlStr = pSrvrStmt->sqlString;
	SQLItemDescList_def *inputSQLDesc = &pSrvrStmt->inputDescList;
	SQLItemDescList_def *outputSQLDesc = &pSrvrStmt->outputDescList;
	RES_HIT_DESC_def	  *rgPolicyHit = &pSrvrStmt->rgPolicyHit;

	Int32 retcode;
	
	SQLSTMT_ID	*pStmt;
	SQLDESC_ID	*pInputDesc;
	SQLDESC_ID	*pOutputDesc;

	char        *pStmtName;
	BOOL		sqlWarning = FALSE;
	BOOL		rgWarning = FALSE;
	Int32		tempmaxRowsetSize = 0;

	pSrvrStmt->PerfFetchRetcode = SQL_SUCCESS;
	pSrvrStmt->RowsetFetchRetcode = SQL_SUCCESS;

	pStmt = &pSrvrStmt->stmt;
	pOutputDesc = &pSrvrStmt->outputDesc;
	pInputDesc = &pSrvrStmt->inputDesc;

	SQLDESC_ID	sqlString_desc;
	sqlString_desc.version = SQLCLI_ODBC_VERSION;
	sqlString_desc.module = &pSrvrStmt->moduleId;
	sqlString_desc.name_mode = string_data;;
	sqlString_desc.identifier     = (const char *) pSqlStr;
	sqlString_desc.handle = 0;
	sqlString_desc.identifier_len = pSrvrStmt->sqlStringLen;
	sqlString_desc.charset = SQLCHARSETSTRING_UTF8;

	// We have to do the following CLI to set rowsize to ZERO, Since we switch between rowset to non-rowsets 
	retcode = WSQL_EXEC_SetStmtAttr(pStmt,SQL_ATTR_INPUT_ARRAY_MAXSIZE,tempmaxRowsetSize, NULL);
	HANDLE_ERROR(retcode, sqlWarning);

	pSrvrStmt->m_aggPriority = 0;
	pSrvrStmt->sqlUniqueQueryIDLen = MAX_QUERY_NAME_LEN;
	pSrvrStmt->sqlUniqueQueryID[0] = '\0';
	retcode = WSQL_EXEC_Prepare2(pStmt, &sqlString_desc, NULL, NULL, NULL, &pSrvrStmt->cost_info, &pSrvrStmt->comp_stats_info, 
	                                     pSrvrStmt->sqlUniqueQueryID, &pSrvrStmt->sqlUniqueQueryIDLen);
	pSrvrStmt->sqlUniqueQueryID[pSrvrStmt->sqlUniqueQueryIDLen] = '\0';
	pSrvrStmt->m_bNewQueryId = true;

	HANDLE_ERROR(retcode, sqlWarning);
		
	if (pSrvrStmt->stmtType == EXTERNAL_STMT && srvrGlobal->srvrType == CORE_SRVR && (srvrGlobal->resGovernOn || (srvrGlobal->resourceStatistics & STMTSTAT_PREPARE)))
	{
		retcode = RESOURCEGOV(pSrvrStmt, pSqlStr, pSrvrStmt->cost_info.totalTime); // Need to fix function param from Int32 to double
		switch (retcode)
		{
		case SQL_SUCCESS:
			break;
		case ODBC_RG_WARNING:
			rgWarning = TRUE;
			break;
		case ODBC_RG_ERROR:
			return (SQLRETURN)retcode;
		default:
			break;
		}
	}
	retcode = WSQL_EXEC_DescribeStmt(pStmt, pInputDesc, pOutputDesc);
	HANDLE_ERROR(retcode, sqlWarning);
		
	retcode = WSQL_EXEC_GetDescEntryCount(pInputDesc, &pSrvrStmt->paramCount);
	HANDLE_ERROR(retcode, sqlWarning);

	retcode = WSQL_EXEC_GetDescEntryCount(pOutputDesc, &pSrvrStmt->columnCount);
	HANDLE_ERROR(retcode, sqlWarning);


	if (pSrvrStmt->paramCount > 0)
	{
		kdsCreateSQLDescSeq(inputSQLDesc, pSrvrStmt->paramCount);
		retcode = BuildSQLDesc(pInputDesc, pSrvrStmt->paramCount, inputSQLDesc, 
				pSrvrStmt->inputDescVarBuffer, pSrvrStmt->inputDescVarBufferLen,
				pSrvrStmt->IPD);
		HANDLE_ERROR(retcode, sqlWarning);
	}
	else
		kdsCreateEmptySQLDescSeq(inputSQLDesc);

	Int32 estRowLength=0;
	if (pSrvrStmt->columnCount > 0)
	{
		kdsCreateSQLDescSeq(outputSQLDesc, pSrvrStmt->columnCount);
		retcode = BuildSQLDesc(pOutputDesc, pSrvrStmt->columnCount, outputSQLDesc,
			pSrvrStmt->outputDescVarBuffer, pSrvrStmt->outputDescVarBufferLen,
			pSrvrStmt->IRD);
		HANDLE_ERROR(retcode, sqlWarning);

		SRVR_DESC_HDL *IRD = pSrvrStmt->IRD;

		Int32 estLength=0;
		int columnCount = pSrvrStmt->columnCount;

		for (int curColumnNo = 0; curColumnNo < columnCount ; curColumnNo++)
		{
			IRD = pSrvrStmt->IRD;
			estLength = getAllocLength(IRD[curColumnNo].dataType, IRD[curColumnNo].length);
			estLength += 1;
			estRowLength += estLength;
		}
	}
	else
		kdsCreateEmptySQLDescSeq(outputSQLDesc);
	pSrvrStmt->estRowLength = estRowLength;

	SRVRTRACE_EXIT(FILE_INTF+23);

	if (rgWarning)
		return ODBC_RG_WARNING;
	if (sqlWarning)
		return SQL_SUCCESS_WITH_INFO;
	else
		return SQL_SUCCESS;
}	

// SQL Module prepare (catalog APIs)
SQLRETURN SRVR::PREPARE_FROM_MODULE(SRVR_STMT_HDL* pSrvrStmt, 
						 SQLItemDescList_def *inputSQLDesc,
						 SQLItemDescList_def *outputSQLDesc)
{
	SRVRTRACE_ENTER(FILE_INTF+24);

	Int32 retcode = SQL_SUCCESS;
	
	SQLSTMT_ID	*pStmt;
	SQLDESC_ID	*pInputDesc;
	SQLDESC_ID	*pOutputDesc;

	char        *pStmtName;
	BOOL		sqlWarning = FALSE;
	
	pStmt = &pSrvrStmt->stmt;
	pInputDesc = &pSrvrStmt->inputDesc;
	pOutputDesc = &pSrvrStmt->outputDesc;

	retcode = WSQL_EXEC_AddModule(&pSrvrStmt->moduleId);
	// Ignore error -8825 - Module already loaded
	if (retcode == -8825)
		retcode = SQL_SUCCESS;

	HANDLE_ERROR(retcode, sqlWarning);

	retcode = WSQL_EXEC_DescribeStmt(pStmt, pInputDesc, pOutputDesc);
	HANDLE_ERROR(retcode, sqlWarning);
	
	retcode = WSQL_EXEC_GetDescEntryCount(pInputDesc, &pSrvrStmt->paramCount);
	HANDLE_ERROR(retcode, sqlWarning);

	retcode = WSQL_EXEC_GetDescEntryCount(pOutputDesc, &pSrvrStmt->columnCount);
	HANDLE_ERROR(retcode, sqlWarning);

	UInt32 tmpBuildID = srvrGlobal->drvrVersion.buildId; // should go way once we support rowwise rowsets
	srvrGlobal->drvrVersion.buildId = 0;
	if (pSrvrStmt->paramCount > 0)
	{
		kdsCreateSQLDescSeq(inputSQLDesc, pSrvrStmt->paramCount);
		retcode = BuildSQLDesc(pInputDesc, pSrvrStmt->paramCount, inputSQLDesc, 
								pSrvrStmt->inputDescVarBuffer, pSrvrStmt->inputDescVarBufferLen, pSrvrStmt->IPD);
		HANDLE_ERROR(retcode, sqlWarning);
	}
	else
		kdsCreateEmptySQLDescSeq(inputSQLDesc);

	Int32 estRowLength=0;
	if (pSrvrStmt->columnCount > 0)
	{
		kdsCreateSQLDescSeq(outputSQLDesc, pSrvrStmt->columnCount);
		retcode = BuildSQLDesc(pOutputDesc, pSrvrStmt->columnCount, outputSQLDesc,
								pSrvrStmt->outputDescVarBuffer, pSrvrStmt->outputDescVarBufferLen, pSrvrStmt->IRD);
		HANDLE_ERROR(retcode, sqlWarning);

		SRVR_DESC_HDL *IRD = pSrvrStmt->IRD;

		Int32 estLength=0;
		int columnCount = pSrvrStmt->columnCount;

		for (int curColumnNo = 0; curColumnNo < columnCount ; curColumnNo++)
		{
			IRD = pSrvrStmt->IRD;
			estLength = getAllocLength(IRD[curColumnNo].dataType, IRD[curColumnNo].length);
			estLength += 1;
			estRowLength += estLength;
		}
	}
	else
		kdsCreateEmptySQLDescSeq(outputSQLDesc);
	pSrvrStmt->estRowLength = estRowLength;
	SRVRTRACE_EXIT(FILE_INTF+24);
	srvrGlobal->drvrVersion.buildId = tmpBuildID; // should go way once we support rowwise rowsets

	if(srvrGlobal->bSpjEnableProxy)
	{
		Int32	requiredStringLen = 0;

		if(pSrvrStmt->SpjProxySyntaxString == NULL)
		{
			markNewOperator,pSrvrStmt->SpjProxySyntaxString = new char[2000];
			if (pSrvrStmt->SpjProxySyntaxString == NULL)
			{
				// Handle Memory Overflow execption here
				SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
							srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
							srvrGlobal->srvrObjRef, 1, "PREPARE2");
				exit(0);
			}
			pSrvrStmt->SpjProxySyntaxStringLen = 2000;
		}
			
		retcode = SRVR::WSQL_EXEC_GetStmtAttr(&pSrvrStmt->stmt,SQL_ATTR_RS_PROXY_SYNTAX,NULL, pSrvrStmt->SpjProxySyntaxString, 2000, &requiredStringLen);

		// In case the provided buffer is not sufficient for the proxy syntax or the pointer is NULL SQL returns -8889

		if( (retcode == -8889) || requiredStringLen + 1 > pSrvrStmt->SpjProxySyntaxStringLen) // we need to null terminate the string
		{
			delete[] pSrvrStmt->SpjProxySyntaxString;
				markNewOperator,pSrvrStmt->SpjProxySyntaxString = new char[requiredStringLen + 1];
			if (pSrvrStmt->SpjProxySyntaxString == NULL)
			{
				// Handle Memory Overflow execption here
				SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
							srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
							srvrGlobal->srvrObjRef, 1, "PREPARE2");
				exit(0);
			}
			pSrvrStmt->SpjProxySyntaxStringLen = requiredStringLen + 1;
			retcode = SRVR::WSQL_EXEC_GetStmtAttr(&pSrvrStmt->stmt,SQL_ATTR_RS_PROXY_SYNTAX,NULL, pSrvrStmt->SpjProxySyntaxString, requiredStringLen, &requiredStringLen);
			HANDLE_ERROR(retcode, sqlWarning);
		}

		pSrvrStmt->SpjProxySyntaxString[requiredStringLen] = '\0';
	}
	else
	{
		if(pSrvrStmt->SpjProxySyntaxString != NULL)
			delete[] pSrvrStmt->SpjProxySyntaxString;
		pSrvrStmt->SpjProxySyntaxString = NULL;
		pSrvrStmt->SpjProxySyntaxStringLen = 0;
	}

	if (sqlWarning)
		return SQL_SUCCESS_WITH_INFO;
	else
		return SQL_SUCCESS;
}

// Regular fetch
SQLRETURN SRVR::FETCH(SRVR_STMT_HDL *pSrvrStmt)
{
	SRVRTRACE_ENTER(FILE_INTF+26);

	Int32 maxRowCnt = pSrvrStmt->maxRowCnt; 
	Int32 maxRowLen = pSrvrStmt->maxRowLen;
	Int32 *rowsAffected = &pSrvrStmt->rowsAffected; 
	SQLValueList_def *outputValueList = &pSrvrStmt->outputValueList;

	Int32 retcode = SQL_SUCCESS;
	Int32 curRowCnt;
	Int32 curColumnNo;
	Int32 varPtr;
	Int64 indPtr;
	void *pBytes;
	Int32 dataType;
	Int32 dataLength;
	Int32 allocLength;
	Int32 srcDataLength;
	short indValue;
	Int32 columnCount;
	Int32 Charset;
	SQLDESC_ID *pDesc;
	BOOL sqlWarning = FALSE;
	SRVR_DESC_HDL		*IRD;

	pDesc = &pSrvrStmt->outputDesc;  
	columnCount = pSrvrStmt->columnCount;

	for (curRowCnt = 1; curRowCnt <= maxRowCnt ; curRowCnt++)
	{
		retcode = WSQL_EXEC_Fetch(&pSrvrStmt->stmt, pDesc, 0);
		if (retcode != 0)
		{
			if (retcode == 100)
			{
				WSQL_EXEC_CloseStmt(&pSrvrStmt->stmt);
				pSrvrStmt->isClosed = TRUE;
				if (curRowCnt == 1)
				{
					return SQL_NO_DATA_FOUND;
				}
				else
				{
					*rowsAffected = curRowCnt-1;
					return SQL_SUCCESS;
				}
			}
			else
			if (retcode < 0)
				return SQL_ERROR;
			else
				sqlWarning = TRUE;
		}
		for (curColumnNo = 0; curColumnNo < columnCount ; curColumnNo++)
		{
			IRD = pSrvrStmt->IRD;
			dataType = IRD[curColumnNo].dataType;
			indPtr = (intptr_t)IRD[curColumnNo].indPtr;
			if ((indPtr == NULL) || (indPtr != NULL && *((short *)indPtr) != -1))
				indValue = 0;
			else
				indValue = -1;
			Charset = IRD[curColumnNo].charSet;
			pBytes = (void *)(IRD[curColumnNo].varPtr);
			dataLength = IRD[curColumnNo].length;
			allocLength = getAllocLength(dataType, dataLength);
			if (maxRowLen != 0)
			{
				switch(dataType)
				{
					case SQLTYPECODE_VARCHAR_WITH_LENGTH:
					case SQLTYPECODE_VARCHAR_LONG:
					case SQLTYPECODE_BITVAR:
					case SQLTYPECODE_BLOB:
					case SQLTYPECODE_CLOB:
                                case SQLTYPECODE_VARBINARY:
						allocLength = (allocLength>(UInt32)maxRowLen+3)?(UInt32)maxRowLen+3:allocLength;
						srcDataLength = *(USHORT *)pBytes;
						srcDataLength = (srcDataLength>(UInt32)maxRowLen)?(UInt32)maxRowLen:srcDataLength;
						*(USHORT *)pBytes=srcDataLength;
						break;
                                case SQLTYPECODE_CHAR:
                                case SQLTYPECODE_BIT:
                                case SQLTYPECODE_VARCHAR:
                                case SQLTYPECODE_BINARY:
                                  allocLength = (allocLength>(UInt32)maxRowLen+1)?(UInt32)maxRowLen+1:allocLength;
						break;
				}
			}
			kdsCopyToSQLValueSeq(outputValueList, dataType, indValue, pBytes, allocLength, Charset);    
		}
	}
	*rowsAffected = curRowCnt >= maxRowCnt ? maxRowCnt : curRowCnt-1; 
	SRVRTRACE_EXIT(FILE_INTF+26);

	if (sqlWarning)
		return SQL_SUCCESS_WITH_INFO;
	else
		return SQL_SUCCESS;
}

// Performance Fetch
SQLRETURN SRVR::FETCHPERF(SRVR_STMT_HDL *pSrvrStmt, 
				SQL_DataValue_def *outputDataValue)
{
	SRVRTRACE_ENTER(FILE_INTF+29);

	Int32 maxRowCnt = pSrvrStmt->maxRowCnt; 
	Int32 maxRowLen = pSrvrStmt->maxRowLen;
	Int32 *rowsAffected = &pSrvrStmt->rowsAffected; 

	Int32 retcode = SQL_SUCCESS;
	Int32 curRowCnt;
	Int32 curColumnNo;
	Int32 varPtr;
	Int64 indPtr;
	void *pBytes;
	Int32 dataType;
	Int32 dataLength;
	Int32 allocLength;
	Int32 srcDataLength;
	short indValue;
	Int32 columnCount;
	Int32 Charset;
	SQLDESC_ID *pDesc;
	BOOL sqlWarning = FALSE;
	SRVR_DESC_HDL *IRD;

	Int32 estLength=0;
	Int32 estRowLength=0;
	Int32 estTotalLength=0;
	Int32 lsize;

	if (pSrvrStmt->PerfFetchRetcode == SQL_NO_DATA_FOUND)
		return SQL_NO_DATA_FOUND;

	pDesc = &pSrvrStmt->outputDesc;  
	columnCount = pSrvrStmt->columnCount;
	estTotalLength = pSrvrStmt->estRowLength * maxRowCnt;
	outputDataValue->_buffer = allocGlobalBuffer(estTotalLength + 1);
	if (outputDataValue->_buffer == NULL)
	{
		// Handle Memory Overflow execption here
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "_buffer:FETCHPERF");
		exit(0);
	}
	memset (outputDataValue->_buffer,0,estTotalLength + 1);

	SQLSTMT_ID*	pStmt = &pSrvrStmt->stmt;
	SRVR_DESC_HDL* IRD_F = pSrvrStmt->IRD;

	for (curRowCnt = 1; curRowCnt <= maxRowCnt ; curRowCnt++)
	{
		retcode = WSQL_EXEC_Fetch(pStmt, pDesc, 0);
		if (retcode != 0)
		{
			if (retcode == 100)
			{
				WSQL_EXEC_CloseStmt(&pSrvrStmt->stmt);
				pSrvrStmt->isClosed = TRUE;
				pSrvrStmt->PerfFetchRetcode = SQL_NO_DATA_FOUND;

				if (curRowCnt == 1)
				{
					retcode = SQL_NO_DATA_FOUND;
					goto ret;
				}
				else
				{
					*rowsAffected = curRowCnt-1;
					retcode = SQL_SUCCESS;
					goto ret;
				}
			}
			else
			if (retcode < 0)
			{
				retcode = SQL_ERROR;
				goto ret;
			}
			else
				sqlWarning = TRUE;
		}
		for (curColumnNo = 0; curColumnNo < columnCount ; curColumnNo++)
		{
			SRVR_DESC_HDL* IRD = IRD_F + curColumnNo;
			indPtr = (intptr_t)IRD->indPtr;
			dataType = IRD->dataType;
			dataLength = IRD->length;
			Charset = IRD->charSet;
			pBytes = (void *)(IRD->varPtr);

			if ((indPtr == NULL) || (indPtr != NULL && *((short *)indPtr) != -1))
			{
				indValue = 0;
				switch(dataType)
				{
					case SQLTYPECODE_VARCHAR_WITH_LENGTH:
					case SQLTYPECODE_VARCHAR_LONG:
					case SQLTYPECODE_BITVAR:
					case SQLTYPECODE_BLOB:
					case SQLTYPECODE_CLOB:
                                        case SQLTYPECODE_VARBINARY:
						dataLength = *(USHORT *)pBytes;
						allocLength = dataLength+3;
						if (maxRowLen != 0)
						{
							allocLength = (allocLength>(UInt32)maxRowLen+3)?(UInt32)maxRowLen+3:allocLength;
							srcDataLength = *(USHORT *)pBytes;
							srcDataLength = (srcDataLength>(UInt32)maxRowLen)?(UInt32)maxRowLen:srcDataLength;
							*(USHORT *)pBytes=srcDataLength;
						}
						break;
					case SQLTYPECODE_BIT:
						allocLength = dataLength;	
						if (maxRowLen != 0)
							allocLength = (allocLength>(UInt32)maxRowLen+1)?(UInt32)maxRowLen+1:allocLength;
						break;
					case SQLTYPECODE_CHAR:
					case SQLTYPECODE_VARCHAR:
                                        case SQLTYPECODE_BINARY:
						allocLength = dataLength+1;	
						if (maxRowLen != 0)
							allocLength = (allocLength>(UInt32)maxRowLen+1)?(UInt32)maxRowLen+1:allocLength;
						break;
					default:
						allocLength = dataLength;
				}

				lsize = outputDataValue->_length;
				outputDataValue->_length += (allocLength+1);
				*(outputDataValue->_buffer+lsize) = '\0';		// first byte must be zero
				memcpy(outputDataValue->_buffer+lsize+1, pBytes, allocLength);
				//
				// for varchar/bitvar string is NULL terminated. NULL does not come from SQL (ZO)
				//
				switch(dataType)
				{
					case SQLTYPECODE_VARCHAR_WITH_LENGTH:
					case SQLTYPECODE_VARCHAR_LONG:
					case SQLTYPECODE_BITVAR:
					case SQLTYPECODE_BLOB:
					case SQLTYPECODE_CLOB:
                                        case SQLTYPECODE_VARBINARY:
						*(outputDataValue->_buffer+lsize + 1 + allocLength - 1) = 0;
						break;
				}
			}
			else
			{
				indValue = -1;
				lsize = outputDataValue->_length;
				outputDataValue->_length += 1;
				*(outputDataValue->_buffer+lsize) = '\1';
			}

		}
	}
	*rowsAffected = curRowCnt >= maxRowCnt ? maxRowCnt : curRowCnt-1; 
	if (sqlWarning)
		retcode = SQL_SUCCESS_WITH_INFO;
	else
		retcode = SQL_SUCCESS;
ret:
	if (outputDataValue->_length == 0)
		outputDataValue->_buffer = NULL;
	SRVRTRACE_EXIT(FILE_INTF+29);

	return retcode;
}  // end FETCHPERF

//LCOV_EXCL_START
//-------------------------------------------------------------------------
// FETCH2
SQLRETURN SRVR::FETCH2( SRVR_STMT_HDL   *pSrvrStmt
		      , Int32        *outValuesFormat
                      , Int32        *outValuesLength
                      , BYTE           *&outValues
                      )
  {
  SRVRTRACE_ENTER(FILE_INTF+42);

  Int32                       retcode      = SQL_SUCCESS;
  Int32                   maxRowCnt    = pSrvrStmt->maxRowCnt;
  Int32                  *rowsAffected = &pSrvrStmt->rowsAffected; 
  Int32                       columnCount  = pSrvrStmt->columnCount;
  SQLDESC_ID	            *pDesc        = &pSrvrStmt->outputDesc;  
  BYTE                      *buffPtr      = NULL;
  BYTE                      *dataStart    = NULL;
  BYTE                      *base         = NULL;
  int                        outputBufLen = 0; 
  struct SQLCLI_QUAD_FIELDS *quadPtrs     = NULL;
  Int32                       rowsFetched  = 0;
  SRVR_DESC_HDL	            *IRD          = NULL;

  // Note, We are going to guess that there are no warnings or errors, so the
  // message is the length of the Header plus the length of the message
  // with no errors or warnings.
  Int32            sizeofFetch2Reply = sizeof(HEADER) + FETCH2_REPLY_ESTIMATED_LENGTH; 

  // maxRowCnt must be at least 1
  if (maxRowCnt < 1)
    maxRowCnt = 1;

  *outValuesFormat = COLUMNWISE_ROWSETS;

  if (&pSrvrStmt->stmt == NULL)
    return SQL_INVALID_HANDLE;
  if (pSrvrStmt->RowsetFetchRetcode == SQL_NO_DATA_FOUND)
    return SQL_NO_DATA_FOUND;

  //
  // We will use rowsets if the maxRowCnt > 1 or if we are allready
  // using rowsets.
  //
  bool useRowsets = false;

  if (maxRowCnt > 1 || pSrvrStmt->outputQuadList != NULL)
    useRowsets = true;
  else
    buffPtr =  pSrvrStmt->IRD[0].varPtr;

  if (useRowsets == true)
    {
    //
    // We will try to be clever here and allocate a global buffer to hold
    // the entire reply message. If it turns out later that we can not put
    // the entire reply message in this buffer (i.e. there are warnings
    // or errors), then we will take the performance hit and re-allocate
    // a bigger buffer and move data appropriately. In theory, this 
    // logic should save time when we are fetching lots of rows per rowset,
    // and the number of rows per fetch is the same size.
    *outValuesLength = pSrvrStmt->outputDescVarBufferLen * maxRowCnt;
    outputBufLen     = sizeofFetch2Reply + *outValuesLength;

    //
    // Try to reuse same fetch buffer and same quad pointers if possible.
    //
    bool allocBuffer   = false;
    bool allocQuadPtrs = false;

    if (   pSrvrStmt->outputBuffer == NULL
        || pSrvrStmt->outputBuffer != getGlobalBuffer()
        || pSrvrStmt->outputBufferLength > getGlobalBufferLength()
       )
      allocBuffer = true;

    if (   allocBuffer == true
        || pSrvrStmt->outputBufferLength != outputBufLen
       )
      allocQuadPtrs = true;

    if (allocBuffer == true)
      buffPtr = allocGlobalBuffer(outputBufLen);
    else
      buffPtr = pSrvrStmt->outputBuffer;
  
    pSrvrStmt->outputBuffer       = buffPtr;
    pSrvrStmt->outputBufferLength = outputBufLen;

    if (buffPtr == NULL )
      {
      // Handle No Memory execption here
        SendEventMsg( MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE
				     , srvrGlobal->nskProcessInfo.processId
                                     , ODBCMX_SERVER 
				     , srvrGlobal->srvrObjRef
                                     , 1
				     , "FETCH2");
        exit(0);
      }

    //
    // Build the quad list if necessary
    //
    if (pSrvrStmt->outputQuadList == NULL) 
      {
      // This is the first fetch, so allocate the quad list or the situation requires a new quad list.
      markNewOperator,pSrvrStmt->outputQuadList = new SQLCLI_QUAD_FIELDS[columnCount];
      if (pSrvrStmt->outputQuadList == NULL)
        {          
          SendEventMsg( MSG_MEMORY_ALLOCATION_ERROR
                                       , EVENTLOG_ERROR_TYPE		
		                       , srvrGlobal->nskProcessInfo.processId
                                       , ODBCMX_SERVER 
			  	       , srvrGlobal->srvrObjRef
                                       , 1
	  			       , "FETCH2");
          exit(0);
        }
      }  // if (pSrvrStmt->outputQuadList == NULL) 

    if (allocQuadPtrs == true)
      {
      // KAS - I'm commenting this out for now. It turns out that the transport logic does not use the 
      // buffer address passed in. Rather it used the buffer associated with pnode. Until I can figure
      // out how to handle this, I'll just copy the data.
      // KAS - Also note that a single non-rowset fetch will not work. That is, we are fetching
      // into the pre-allocated buffer, and that buffer has no space for the leading Fetch reply 
      // message. This situation must be handled.
      //dataStart = buffPtr + sizeofFetch2Reply;
      dataStart = buffPtr;  // KAS - temp (see previous line).
      IRD       = pSrvrStmt->IRD;
      base      = IRD[0].varPtr;

      for (int i = 0; i < columnCount; i++)
        {
        pSrvrStmt->outputQuadList[i].var_layout = IRD[i].length;
        if (  (IRD[i].dataType == SQLTYPECODE_VARCHAR_WITH_LENGTH || IRD[i].dataType == SQLTYPECODE_VARCHAR_LONG || IRD[i].dataType == SQLTYPECODE_BLOB || IRD[i].dataType == SQLTYPECODE_CLOB)
	    && IRD[i].length % 2 == 1
           )
          // Need to adjust varchars to even number 
          pSrvrStmt->outputQuadList[i].var_layout = pSrvrStmt->outputQuadList[i].var_layout + 1;
        pSrvrStmt->outputQuadList[i].var_ptr = (dataStart) + ((IRD[i].varPtr - base) * maxRowCnt);
        if (IRD[i].indPtr != 0)
          {
          pSrvrStmt->outputQuadList[i].ind_layout = sizeof(short);
          pSrvrStmt->outputQuadList[i].ind_ptr = (dataStart + ((IRD[i].indPtr - base) * maxRowCnt));
          }
        else
          {
          pSrvrStmt->outputQuadList[i].ind_layout = 0;
          pSrvrStmt->outputQuadList[i].ind_ptr    = NULL;
          }
        }  // end for

      SQLCLI_QUAD_FIELDS *quadList = pSrvrStmt->outputQuadList;

      retcode = WSQL_EXEC_SETROWSETDESCPOINTERS( pDesc
	    				       , maxRowCnt
					       , NULL
					       , 1
					       , columnCount
					       , quadList);
      if (retcode < SQL_SUCCESS)
        {
        retcode = SQL_ERROR;
        }
      }  // end if allocQuadPtrs == true
    }  // end if (useRowsets == true)

  retcode = WSQL_EXEC_Fetch(&pSrvrStmt->stmt, pDesc, 0);
  if (retcode < SQL_SUCCESS)
    {
    retcode = SQL_ERROR;
    }
  else if (retcode > SQL_SUCCESS && retcode != 100)
    retcode = SQL_SUCCESS;

  if (useRowsets == true)
    WSQL_EXEC_GetDescItem(pDesc,1,SQLDESC_ROWSET_NUM_PROCESSED,&rowsFetched,0,0,0,0);
  else
    if (retcode == SQL_SUCCESS)
      {
      rowsFetched = 1;
      *outValuesLength = pSrvrStmt->outputDescVarBufferLen;
      }

  if (retcode == 100)
    {
    WSQL_EXEC_CloseStmt(&pSrvrStmt->stmt);
    pSrvrStmt->isClosed = TRUE;
    pSrvrStmt->RowsetFetchRetcode = SQL_NO_DATA_FOUND;
    }

  if (retcode == SQL_SUCCESS)
    {
    *rowsAffected = rowsFetched;
	if (rowsFetched == 0)
		retcode = SQL_NO_DATA_FOUND;
    }
  else if (retcode == 100)
    {
    if (rowsFetched == 1 || rowsFetched == 0)
      retcode = SQL_NO_DATA_FOUND;
    else
      {
      pSrvrStmt->RowsetFetchRetcode = SQL_NO_DATA_FOUND;
      pSrvrStmt->rowsAffected = rowsFetched;
      retcode = SQL_SUCCESS;
      }
    }  // end else if retcode == 100

  outValues = buffPtr;

  SRVRTRACE_EXIT(FILE_INTF+42);
  return retcode;
  }  // end SRVR::FETCH2
//LCOV_EXCL_STOP

SQLRETURN SRVR::GETSQLERROR(bool& bSQLMessageSet, 
					odbc_SQLSvc_SQLError *SQLError)
{
	SRVRTRACE_ENTER(FILE_INTF+31);


	Int32 retcode;
	Int32 total_conds = 0;
	char *msg_buf;
	Int32 buf_len;
	Int32 sqlcode  = 0;
	char sqlState[6];
	Int32 curr_cond = 1;
	Int32 msg_buf_len;
	Int32 sqlRowCount;
	char  strNow[TIMEBUFSIZE + 1];
		
	retcode =  WSQL_EXEC_GetDiagnosticsStmtInfo2(NULL,
											SQLDIAG_NUMBER,
																(Int64*)&total_conds, NULL, 0, NULL);
    if (total_conds == 0)
	{
		kdsCreateSQLErrorException(bSQLMessageSet,SQLError, 1);
		kdsCopySQLErrorException(SQLError, "No error message in SQL diagnostics area, but sqlcode is non-zero", retcode, "");
	}
	else
	{
		kdsCreateSQLErrorException(bSQLMessageSet, SQLError, total_conds);
		while (curr_cond <= total_conds)
		{
			retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_SQLCODE, curr_cond,
				&sqlcode, NULL, 0, NULL);
			if (retcode >= SQL_SUCCESS)
			{
				if (sqlcode == 100)
				{
					// We are not copying the Warning message if the error code is 100
					// It is ok, though we have allocated more SQLError, but length is incremented 
					// only when SQLError is copied
					curr_cond++;
					continue;
				}
				retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_MSG_LEN, curr_cond,
					&msg_buf_len, NULL, 0, NULL);
			}
			if (retcode >= SQL_SUCCESS)
			{
				msg_buf_len = (msg_buf_len+1)*4 + TIMEBUFSIZE;
				msg_buf = new char[msg_buf_len];
				buf_len = 0;
				retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_MSG_TEXT, curr_cond,
					NULL, msg_buf, msg_buf_len, &buf_len);
			}
			if (retcode >= SQL_SUCCESS)
			{
				msg_buf[buf_len] = '\0';
				//Get the timetsamp
				time_t  now = time(NULL);
				bzero(strNow, sizeof(strNow) );
				strftime(strNow, sizeof(strNow), " [%Y-%m-%d %H:%M:%S]", localtime(&now));
				strcat(msg_buf, strNow);
				buf_len = 0;
				retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_RET_SQLSTATE, curr_cond,
					NULL, sqlState, sizeof(sqlState), &buf_len);
			}			
			if (retcode >= SQL_SUCCESS)
				sqlState[5] = '\0';
			else
			{
				kdsCopySQLErrorException(SQLError, "Internal Error : From GetCondDiagnostics",
					retcode, "");
				delete msg_buf;
				break;
			}
			sqlRowCount = -1;
			retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_ROW_NUMBER, curr_cond,
					&sqlRowCount, NULL, 0, NULL);

			if (retcode < SQL_SUCCESS)
				sqlRowCount = -1;
			else if (sqlRowCount != -1)
				sqlRowCount++;

			kdsCopySQLErrorExceptionAndRowCount(SQLError, msg_buf, sqlcode, sqlState, sqlRowCount);
			delete msg_buf;
			curr_cond++;
		}
		
		WSQL_EXEC_ClearDiagnostics(NULL);
	}
	SRVRTRACE_EXIT(FILE_INTF+31);

	return SQL_SUCCESS;
}

SQLRETURN SRVR::GETSQLWARNINGORERROR2(SRVR_STMT_HDL* pSrvrStmt)
{
	SRVRTRACE_ENTER(FILE_INTF+31);

	Int32 retcode;
	Int32 total_conds = 0;
	char *msg_buf=NULL;
	Int32 buf_len;
	Int32 sqlcode  = 0;
	char sqlState[6];
	Int32 curr_cond = 1;
	Int32 skipped_conds = 0;
	Int32 msg_buf_len = 0;
	Int32 msg_total_len = 0,Tot_Alloc_Buffer_len = 0;
	Int32 rowId = 0; // use this for rowset recovery.
	char  strNow[TIMEBUFSIZE + 1];
	
	pSrvrStmt->sqlWarningOrErrorLength = 0;
	retcode =  WSQL_EXEC_GetDiagnosticsStmtInfo2(NULL, SQLDIAG_NUMBER, (Int64*)&total_conds, NULL, 0, NULL);

	// Calculate Total Buffer to allocate 
	while(curr_cond <= total_conds)
	{
		retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_SQLCODE, curr_cond,	&sqlcode, NULL, 0, NULL);

		// We are not copying the Warning message if the error code is 100
		// An Error Code -8916 or -8915 is how the UDR server signals that there are no more result sets
		// There is no need to pass this error to the client
		if(sqlcode != -8916 && sqlcode != -8915 && sqlcode != 100)
		{
			Tot_Alloc_Buffer_len += sizeof(total_conds) + sizeof(rowId) + sizeof(sqlcode);
			Tot_Alloc_Buffer_len += sizeof(sqlState) + sizeof(msg_buf_len);
			// From R2.3 onwards, the above call returns the number of "characters"
			// in the error message text. Incase of multibyte characters, one character
			// can have upto 4 bytes each.
			retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_MSG_LEN, curr_cond,
			&msg_buf_len, NULL, 0, NULL);

 			Tot_Alloc_Buffer_len += (msg_buf_len + 1)*4 + TIMEBUFSIZE; //*4 To take care of multi-byte characters
													//TIMEBUFSIZE for timestamp					
		}
		else
		{
			skipped_conds++;
		}

		curr_cond++;
	}
  	
	
	markNewOperator,pSrvrStmt->sqlWarningOrError = new BYTE[Tot_Alloc_Buffer_len];
		
	if (pSrvrStmt->sqlWarningOrError == NULL)
	{
		// Handle Memory Overflow execption here
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "GETSQLWARNINGORERROR2");
		exit(0);
	}

	*(Int32 *)(pSrvrStmt->sqlWarningOrError+msg_total_len) = total_conds - skipped_conds;
	msg_total_len += sizeof(total_conds);


	curr_cond = 1;
	while (curr_cond <= total_conds)
	{
		// Need to get the sqlcode ahead, so that we can skip diagnostics that we are not interested in
		WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_SQLCODE, curr_cond,	&sqlcode, NULL, 0, NULL);
		if(sqlcode == -8915 || sqlcode == -8916 || sqlcode == 100)
		{
			curr_cond++;
			continue;
		}

		retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_ROW_NUMBER, curr_cond, &rowId, NULL, 0, NULL);
		if (retcode < SQL_SUCCESS) 
			rowId = -1;
		else if(rowId != -1)
			rowId++;  //indexing starts at 0
		*(Int32 *)(pSrvrStmt->sqlWarningOrError+msg_total_len) = rowId;
		msg_total_len += sizeof(rowId);

		*(Int32 *)(pSrvrStmt->sqlWarningOrError+msg_total_len) = sqlcode;
		msg_total_len += sizeof(sqlcode);

		if (retcode >= SQL_SUCCESS)
		{
			msg_buf_len = 0;
			retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_MSG_LEN, curr_cond,
				&msg_buf_len, NULL, 0, NULL);
			msg_buf_len += 1; // Null terminator
		}
		if (retcode >= SQL_SUCCESS )
		{
			// From R2.3 onwards, the above call returns the number of "characters"
			// in the error message text. Incase of multibyte characters, one character
			// can have upto 4 bytes each. From R2.3 onwards, only for ISO88591 
			// configuration msg_buf_len and buf_len (see below) will be the same.
			// Incase the error message has multi-byte characters, we need to be sure
			// of providing enough buffer to hold the error message
			msg_buf_len = msg_buf_len*4;	//max, for multibyte characters
			msg_buf = new char[msg_buf_len]; 
			buf_len = 0;
			retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_MSG_TEXT, curr_cond,
				NULL, msg_buf, msg_buf_len, &buf_len);
			//Note that the buf_len always has the correct number of returned bytes.
			msg_buf[buf_len] = '\0';
			msg_buf_len = buf_len +1;
			//Get the timetsamp
			time_t  now = time(NULL);
			bzero(strNow, sizeof(strNow) );
			strftime(strNow, sizeof(strNow), " [%Y-%m-%d %H:%M:%S]", localtime(&now));
			strcat(msg_buf, strNow);
			msg_buf_len += TIMEBUFSIZE;
			*(Int32*)(pSrvrStmt->sqlWarningOrError+msg_total_len) = msg_buf_len;
			msg_total_len += sizeof(msg_buf_len);
			memcpy(pSrvrStmt->sqlWarningOrError+msg_total_len, msg_buf, msg_buf_len);
			msg_total_len += msg_buf_len;
		}
      else
          return retcode;

		if (retcode >= SQL_SUCCESS)
		{
			buf_len = 0;
			sqlState[0] = '\0';
			retcode = WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_RET_SQLSTATE, curr_cond,
				NULL, sqlState, sizeof(sqlState), &buf_len);
			sqlState[5] = '\0';
			memcpy(pSrvrStmt->sqlWarningOrError+msg_total_len, sqlState, sizeof(sqlState));
			msg_total_len += sizeof(sqlState);
		}			
		
		// Fix for CR 6447. We'll add 2034 error to sqlErrorExit array only if on a file system error 31
		if(sqlcode == -2034)
		{
			if( strstr((const char *)msg_buf, (const char *)"error 31" ) != NULL && errorIndex < 8 )
			{
				sqlErrorExit[errorIndex++] = sqlcode;
			}
		}
		
		if (msg_buf != NULL) delete [] msg_buf;
		curr_cond++;
	}
	
	WSQL_EXEC_ClearDiagnostics(NULL);
	pSrvrStmt->sqlWarningOrErrorLength = msg_total_len;
	SRVRTRACE_EXIT(FILE_INTF+31);

	retcode = SQL_SUCCESS;
ret:
	return retcode;
}  // end GETSQLWARNINGORERROR2


//---------------------------------------------------------------
SQLRETURN SRVR::GETSQLWARNINGORERROR2forRowsets(SRVR_STMT_HDL* pSrvrStmt)
  {
  SRVRTRACE_ENTER(FILE_INTF+43);

  Int32  retcode;
  Int32  total_conds          = 0;
  char *msg_buf              = NULL;
  Int32  buf_len;
  Int32  sqlcode              = 0;
  char  sqlState[6];
  Int32  curr_cond            = 1;
  Int32  msg_buf_len          = 0;
  Int32  msg_total_len        = 0;
  Int32  Tot_Alloc_Buffer_len = 0;
  Int32  rowId                = 0;
	
  pSrvrStmt->sqlWarningOrErrorLength = 0;
  total_conds = pSrvrStmt->sqlWarning._length;

  retcode = 0; //WSQL_EXEC_GetDiagnosticsStmtInfo2(NULL, SQLDIAG_NUMBER, &total_conds, NULL, 0, NULL);

  // Calculate Total Buffer to allocate 
  while(curr_cond <= total_conds)
    {

    if (sqlcode != 100)
      {
	  Tot_Alloc_Buffer_len += sizeof(total_conds) + sizeof(rowId) + sizeof(sqlcode);
      sqlcode               = pSrvrStmt->sqlWarning._buffer[curr_cond - 1].sqlcode;
      Tot_Alloc_Buffer_len += sizeof(sqlState) + sizeof(msg_buf_len);
      msg_buf_len           = strlen(pSrvrStmt->sqlWarning._buffer[curr_cond - 1].errorText);
      Tot_Alloc_Buffer_len += msg_buf_len + 1;
      }
    curr_cond++;
    }
  	
  curr_cond = 1;
	
  markNewOperator,pSrvrStmt->sqlWarningOrError = new BYTE[Tot_Alloc_Buffer_len];
		
  if (pSrvrStmt->sqlWarningOrError == NULL)
    {
    // Handle Memory Overflow execption here
     SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "GETSQLWARNINGORERROR2");
     exit(0);
    }

  *(Int32 *)(pSrvrStmt->sqlWarningOrError+msg_total_len) = total_conds;
  msg_total_len += sizeof(total_conds);

  while (curr_cond <= total_conds)
    {
    if (retcode >= SQL_SUCCESS)
      {
      if (sqlcode == 100)
        {
        // We are not copying the Warning message if the error code is 100
        // It is ok, though we have allocated more SQLError, but length is incremented 
        // only when SQLError is copied
        curr_cond++;
        continue;
        }
      rowId = pSrvrStmt->sqlWarning._buffer[curr_cond - 1].rowId;
      *(Int32 *)(pSrvrStmt->sqlWarningOrError+msg_total_len) = rowId;
      msg_total_len += sizeof(rowId);
      sqlcode = pSrvrStmt->sqlWarning._buffer[curr_cond - 1].sqlcode;    
      *(Int32 *)(pSrvrStmt->sqlWarningOrError+msg_total_len) = sqlcode;
      msg_total_len += sizeof(sqlcode);
      msg_buf_len  = 0;
      msg_buf_len  = strlen(pSrvrStmt->sqlWarning._buffer[curr_cond - 1].errorText);
      msg_buf_len += 1; // Null terminator
      }
    if (retcode >= SQL_SUCCESS )
      {
      msg_buf = new char[msg_buf_len];
      buf_len = 0;
      memcpy(msg_buf, pSrvrStmt->sqlWarning._buffer[curr_cond - 1].errorText, msg_buf_len);
      msg_buf[msg_buf_len-1] = '\0';
      *(Int32*)(pSrvrStmt->sqlWarningOrError+msg_total_len) = msg_buf_len;
      msg_total_len += sizeof(msg_buf_len);
      memcpy(pSrvrStmt->sqlWarningOrError+msg_total_len, msg_buf, msg_buf_len);
      msg_total_len += msg_buf_len;
      }
    else
      return retcode;

    if (retcode >= SQL_SUCCESS)
      {
      buf_len = 0;
      sqlState[0] = '\0';
      memcpy(sqlState, pSrvrStmt->sqlWarning._buffer[curr_cond - 1].sqlstate, 5);
      sqlState[5] = '\0';
      memcpy(pSrvrStmt->sqlWarningOrError+msg_total_len, sqlState, sizeof(sqlState));
      msg_total_len += sizeof(sqlState);
      }			
      
		// Fix for CR 6447. We'll add 2034 error to sqlErrorExit array only if on a file system error 31
		if(sqlcode == -2034)
		{
			if( strstr((const char *)msg_buf, (const char *)"error 31" ) != NULL && errorIndex < 8 )
			{
				sqlErrorExit[errorIndex++] = sqlcode;
			}
		}
      
    if (msg_buf != NULL) delete [] msg_buf;
      curr_cond++;
    }  // end while

    pSrvrStmt->sqlWarningOrErrorLength = msg_total_len;
    SRVRTRACE_EXIT(FILE_INTF+43);

    retcode = SQL_SUCCESS;
ret:
    return retcode;
}  // end GETSQLWARNINGORERROR2forRowsets

//------------------------------------------------------------------------------


// Regular execdirect
SQLRETURN SRVR::EXECDIRECT(SRVR_STMT_HDL* pSrvrStmt)

{
	SRVRTRACE_ENTER(FILE_INTF+32);

	char *pSqlStr = pSrvrStmt->sqlString;
	IDL_short sqlStmtType = pSrvrStmt->sqlStmtType;
	SQLItemDescList_def *outputSQLDesc = &pSrvrStmt->outputDescList;
	Int64 rowsAffected_cli;
	Int32		*rowsAffected = &pSrvrStmt->rowsAffected;
	RES_HIT_DESC_def *rgPolicyHit = &pSrvrStmt->rgPolicyHit;

	Int32 retcode = SQL_SUCCESS;
	Int32 execretcode = SQL_SUCCESS;
	
	SQLSTMT_ID	*pStmt;
	SQLDESC_ID	*pInputDesc;
	SQLDESC_ID	*pOutputDesc;
	SQLSTMT_ID  cursorId;

	char        *pStmtName;
	char		*cursorName;
	char		*tempSqlString = NULL;
	size_t		len;
	BOOL		sqlWarning = FALSE;
	BOOL		rgWarning = FALSE;
	
	pSrvrStmt->PerfFetchRetcode = SQL_SUCCESS;
	pSrvrStmt->RowsetFetchRetcode = SQL_SUCCESS;

	pStmt = &pSrvrStmt->stmt;
	pInputDesc = &pSrvrStmt->inputDesc;
	pOutputDesc = &pSrvrStmt->outputDesc;

	SQLDESC_ID	sqlString_desc;
	sqlString_desc.version = SQLCLI_ODBC_VERSION;
	sqlString_desc.module = &pSrvrStmt->moduleId;
	sqlString_desc.name_mode = string_data;;
	sqlString_desc.identifier     = (const char *) pSqlStr;
	sqlString_desc.handle = 0;
	sqlString_desc.identifier_len = pSrvrStmt->sqlStringLen;
	sqlString_desc.charset = SQLCHARSETSTRING_UTF8;

	// Initialize this to TRUE later in this function depending on sqlStmtType if SQL_SELECT
	// then and retcode is SUCCESS or SUCCESS_WITH_INFO set it to FALSE. Since SQL opens CURSOR
	// after execute in case of SUCCESS or SUCCESS_WITH_INFO and closes only when it reaches 
	// NO_DATA_FOUND or CLOSE STATEMENT or COMMIT WORK or ROLLBACK WORK.
	pSrvrStmt->isClosed = TRUE;

	pSrvrStmt->sqlUniqueQueryIDLen = 0;
	pSrvrStmt->sqlUniqueQueryID[0] = '\0';

	if (pSrvrStmt->stmtType == EXTERNAL_STMT && srvrGlobal->srvrType == CORE_SRVR)
	{
		if (strnicmp(pSrvrStmt->sqlString, "INFOSTATS", 9) == 0)
		{
			tempSqlString = new char[strlen(pSrvrStmt->sqlString) + 1];
			if (tempSqlString == NULL)
			{
				SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "tempSqlString");
				exit(0);
			}
			strcpy(tempSqlString,pSrvrStmt->sqlString);
			retcode = doInfoStats(pSrvrStmt);
			if (retcode != SQL_SUCCESS) 
			{
				if ((resStatStatement != NULL) && (pSrvrStmt->inState != STMTSTAT_NONE))
					/*resStatStatement->start(pSrvrStmt->inState, 
								pSrvrStmt->sqlQueryType, 
								pSrvrStmt->stmtName, 
								pSrvrStmt->sqlUniqueQueryID, 
								pSrvrStmt->cost_info,
								pSrvrStmt->comp_stats_info,
								inEstimatedCost, 
								&pSrvrStmt->m_need_21036_end_msg,
								tempSqlString);*/
					resStatStatement->start(pSrvrStmt->inState, 
								pSrvrStmt->sqlQueryType, 
								pSrvrStmt->stmtName, 
								pSrvrStmt, 
								inEstimatedCost, 
								&pSrvrStmt->m_need_21036_end_msg,
								tempSqlString);
				delete [] tempSqlString;
				return retcode;
			}
			pSrvrStmt->m_bSkipWouldLikeToExecute = true;
			sqlStmtType = pSrvrStmt->sqlStmtType;
			sqlString_desc.identifier = (const char *) pSrvrStmt->sqlString;
			sqlString_desc.identifier_len = pSrvrStmt->sqlStringLen;
		}

		if (srvrGlobal->resGovernOn || sqlStmtType == TYPE_SELECT)
		{
			pSrvrStmt->m_aggPriority = 0;
			pSrvrStmt->sqlUniqueQueryIDLen = MAX_QUERY_NAME_LEN;
			retcode = WSQL_EXEC_Prepare2(pStmt, &sqlString_desc, NULL, NULL, NULL, &pSrvrStmt->cost_info, &pSrvrStmt->comp_stats_info,
			                                     pSrvrStmt->sqlUniqueQueryID, &pSrvrStmt->sqlUniqueQueryIDLen);
			pSrvrStmt->sqlUniqueQueryID[pSrvrStmt->sqlUniqueQueryIDLen] = '\0';
			pSrvrStmt->m_bNewQueryId = true;
		
			if ((resStatStatement != NULL) && (pSrvrStmt->inState != STMTSTAT_NONE))
				/*resStatStatement->start(pSrvrStmt->inState, 
							pSrvrStmt->sqlQueryType, 
							pSrvrStmt->stmtName, 
							pSrvrStmt->sqlUniqueQueryID, 
							pSrvrStmt->cost_info,
							pSrvrStmt->comp_stats_info,
							inEstimatedCost, 
							&pSrvrStmt->m_need_21036_end_msg,
							tempSqlString);*/
				resStatStatement->start(pSrvrStmt->inState, 
							pSrvrStmt->sqlQueryType, 
							pSrvrStmt->stmtName, 
							pSrvrStmt, 
							inEstimatedCost, 
							&pSrvrStmt->m_need_21036_end_msg,
							tempSqlString);
			if (tempSqlString != NULL) delete [] tempSqlString;
			HANDLE_ERROR(retcode, sqlWarning);
			if (srvrGlobal->resGovernOn || (srvrGlobal->resourceStatistics & STMTSTAT_EXECDIRECT))
			{
				retcode = RESOURCEGOV(pSrvrStmt, pSqlStr, pSrvrStmt->cost_info.totalTime);
				switch (retcode)
				{
				case SQL_SUCCESS:
					break;
				case ODBC_RG_WARNING:
					rgWarning = TRUE;
					break;
				case ODBC_RG_ERROR:
					return (SQLRETURN)retcode;
				default:
					break;
				}
			}
		}
		else
		{
			retcode = WSQL_EXEC_SetStmtAttr(pStmt,SQL_ATTR_UNIQUE_STMT_ID,0,NULL);
			HANDLE_ERROR(retcode, sqlWarning);
			retcode = WSQL_EXEC_GetStmtAttr(pStmt, SQL_ATTR_UNIQUE_STMT_ID , 0, 
			                                       pSrvrStmt->sqlUniqueQueryID, MAX_QUERY_NAME_LEN, &pSrvrStmt->sqlUniqueQueryIDLen);
			pSrvrStmt->sqlUniqueQueryID[pSrvrStmt->sqlUniqueQueryIDLen] = '\0';
			pSrvrStmt->m_bNewQueryId = true;

			HANDLE_ERROR(retcode, sqlWarning);
			if ((resStatStatement != NULL) && (pSrvrStmt->inState != STMTSTAT_NONE))
				/* resStatStatement->start(pSrvrStmt->inState, 
							pSrvrStmt->sqlQueryType, 
							pSrvrStmt->stmtName, 
							pSrvrStmt->sqlUniqueQueryID, 
							pSrvrStmt->cost_info,
							pSrvrStmt->comp_stats_info,
							inEstimatedCost, 
							&pSrvrStmt->m_need_21036_end_msg,
							tempSqlString);*/
				resStatStatement->start(pSrvrStmt->inState, 
							pSrvrStmt->sqlQueryType, 
							pSrvrStmt->stmtName, 
							pSrvrStmt, 
							inEstimatedCost, 
							&pSrvrStmt->m_need_21036_end_msg,
							tempSqlString);
			if (tempSqlString != NULL) delete [] tempSqlString;
			retcode = execretcode = WSQL_EXEC_ExecDirect(pStmt, &sqlString_desc, 0, 0);
			HANDLE_ERROR(retcode, sqlWarning);
			pSrvrStmt->cost_info.totalTime = -1;
		}
	}
	else
	{
		if (sqlStmtType == TYPE_SELECT)
		{
			pSrvrStmt->m_aggPriority = 0;
			pSrvrStmt->sqlUniqueQueryIDLen = MAX_QUERY_NAME_LEN;
			retcode = WSQL_EXEC_Prepare2(pStmt, &sqlString_desc, NULL, NULL, NULL, &pSrvrStmt->cost_info, &pSrvrStmt->comp_stats_info,
			                                     pSrvrStmt->sqlUniqueQueryID, &pSrvrStmt->sqlUniqueQueryIDLen);
			pSrvrStmt->sqlUniqueQueryID[pSrvrStmt->sqlUniqueQueryIDLen] = '\0';
			pSrvrStmt->m_bNewQueryId = true;

			HANDLE_ERROR(retcode, sqlWarning);
		}
		else
		{
			retcode = WSQL_EXEC_SetStmtAttr(pStmt,SQL_ATTR_UNIQUE_STMT_ID,0,NULL);
			HANDLE_ERROR(retcode, sqlWarning);
			retcode = WSQL_EXEC_GetStmtAttr(pStmt, SQL_ATTR_UNIQUE_STMT_ID , 0, 
			                                       pSrvrStmt->sqlUniqueQueryID, MAX_QUERY_NAME_LEN, &pSrvrStmt->sqlUniqueQueryIDLen);
			pSrvrStmt->sqlUniqueQueryID[pSrvrStmt->sqlUniqueQueryIDLen] = '\0';	
			pSrvrStmt->m_bNewQueryId = true;

			HANDLE_ERROR(retcode, sqlWarning);
			retcode = execretcode = WSQL_EXEC_ExecDirect(pStmt, &sqlString_desc, 0, 0);
			HANDLE_ERROR(retcode, sqlWarning);
			pSrvrStmt->cost_info.totalTime = -1;
		}
	}
	if (sqlStmtType == TYPE_SELECT)
	{
		retcode = WSQL_EXEC_DescribeStmt(pStmt, (SQLDESC_ID *)NULL, pOutputDesc);
		HANDLE_ERROR(retcode, sqlWarning);

		retcode = WSQL_EXEC_GetDescEntryCount(pOutputDesc, &pSrvrStmt->columnCount);
		HANDLE_ERROR(retcode, sqlWarning);

		if (pSrvrStmt->columnCount > 0)
		{
			if (outputSQLDesc != NULL)
			{
				UInt32 tmpBuildID = srvrGlobal->drvrVersion.buildId; // should go way once we support rowwise rowsets
				srvrGlobal->drvrVersion.buildId = 0;

				kdsCreateSQLDescSeq(outputSQLDesc, pSrvrStmt->columnCount);
				retcode = BuildSQLDesc(pOutputDesc, pSrvrStmt->columnCount, outputSQLDesc,
				pSrvrStmt->outputDescVarBuffer, pSrvrStmt->outputDescVarBufferLen,pSrvrStmt->IRD);

				srvrGlobal->drvrVersion.buildId = tmpBuildID; // should go way once we support rowwise rowsets

				HANDLE_ERROR(retcode, sqlWarning);
				Int32 estLength=0;
				Int32 estRowLength = 0;
				int columnCount = pSrvrStmt->columnCount;
				SRVR_DESC_HDL *IRD = pSrvrStmt->IRD;

				for (int curColumnNo = 0; curColumnNo < columnCount ; curColumnNo++)
				{
					estLength = getAllocLength(IRD[curColumnNo].dataType, IRD[curColumnNo].length);
					estLength += 1;
					estRowLength += estLength;
				}
				pSrvrStmt->estRowLength = estRowLength;
			}
			else
				return ODBC_SERVER_ERROR;
		}
		else
		{
			if (outputSQLDesc != NULL)
				kdsCreateEmptySQLDescSeq(outputSQLDesc);
		}
	}
	else
	{
			pSrvrStmt->columnCount = 0;
			kdsCreateEmptySQLDescSeq(outputSQLDesc);
	}
	if (pSrvrStmt->stmtType == EXTERNAL_STMT && srvrGlobal->srvrType == CORE_SRVR)
	{
			// Child query visibility
			retcode = WSQL_EXEC_GetStmtAttr(pStmt, SQL_ATTR_SUBQUERY_TYPE, &pSrvrStmt->sqlSubQueryType, NULL, 0, NULL);
			retcode = WSQL_EXEC_GetStmtAttr(pStmt, SQL_ATTR_QUERY_TYPE, &pSrvrStmt->sqlQueryType, NULL, 0, NULL);
			// Added the below to treat the new SQL_EXE_UTIL type as SQL_SELECT_NON_UNIQUE 
			// in MXOSRVR to minimize code changes and to send the new query type to WMS.
			pSrvrStmt->sqlNewQueryType = pSrvrStmt->sqlQueryType;
			if( pSrvrStmt->sqlQueryType == SQL_EXE_UTIL )
				pSrvrStmt->sqlQueryType = SQL_SELECT_NON_UNIQUE;
			HANDLE_ERROR(retcode, sqlWarning);

		if (sqlStmtType == TYPE_SELECT)
		{
			cursorName = pSrvrStmt->cursorName;
			// If cursor name is not specified, use the stmt name as cursor name
			if (*cursorName == '\0')
				cursorName = pSrvrStmt->stmtName;
			// If cursorName has chg'd from last EXEC or EXECDIRECT cmd
			// or has not yet been set for the first time call SetCursorName
			if ((strcmp(pSrvrStmt->previousCursorName, cursorName) != 0) && *cursorName != '\0')
			{
				cursorId.version        = SQLCLI_ODBC_VERSION;
				cursorId.module         = pStmt->module;
				cursorId.handle         = 0;
				cursorId.charset        = SQLCHARSETSTRING_UTF8;
				cursorId.name_mode      = cursor_name;
				if (*cursorName == '\0')
					cursorId.identifier_len = pSrvrStmt->stmtNameLen;
				else
					cursorId.identifier_len = pSrvrStmt->cursorNameLen;
				cursorId.identifier     = cursorName;
				strcpy(pSrvrStmt->previousCursorName, cursorName);	// keep track of last cursorName
				retcode = WSQL_EXEC_SetCursorName(pStmt, &cursorId);
				HANDLE_ERROR(retcode, sqlWarning);
			}
			if (pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE || pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE)
			{
			retcode = WSQL_EXEC_Exec(pStmt, NULL, 0);
			HANDLE_ERROR(retcode, sqlWarning);
			}
			else
			{
			retcode = execretcode = WSQL_EXEC_ExecFetch(pStmt, NULL, 0);
			HANDLE_ERROR(retcode, sqlWarning);
			}
		}
		else
		if (srvrGlobal->resGovernOn)
		{
			retcode = execretcode = WSQL_EXEC_ExecFetch(pStmt, NULL, 0);
			HANDLE_ERROR(retcode, sqlWarning);
		}

	}
	else
	{
		if (sqlStmtType == TYPE_SELECT)
		{
			retcode = WSQL_EXEC_Exec(pStmt, NULL, 0);
			HANDLE_ERROR(retcode, sqlWarning);
		}
	}
	
	if (retcode >= SQL_SUCCESS || rgWarning)
	{
		if (WSQL_EXEC_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &rowsAffected_cli, NULL, 0, NULL) < SQL_SUCCESS)
		{
			*rowsAffected = -1;
		}
		else
		{
			*rowsAffected=(Int32)rowsAffected_cli;
		}
	}
	SRVRTRACE_EXIT(FILE_INTF+32);

	// Initialize this to TRUE later in this function depending on sqlStmtType if SQL_SELECT
	// then and retcode is SUCCESS or SUCCESS_WITH_INFO set it to FALSE. Since SQL opens CURSOR
	// after execute in case of SUCCESS or SUCCESS_WITH_INFO and closes only when it reaches 
	// NO_DATA_FOUND or CLOSE STATEMENT or COMMIT WORK or ROLLBACK WORK.
	if ( pSrvrStmt->stmtType == EXTERNAL_STMT ) 
	{
		if ( sqlStmtType == TYPE_SELECT  && (pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE || pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE) )
		{ 
		pSrvrStmt->isClosed = FALSE;
		pSrvrStmt->bFetchStarted = FALSE;
		}
	}
	else
	{
		if (sqlStmtType == TYPE_SELECT) 
		{
			pSrvrStmt->isClosed = FALSE;
			pSrvrStmt->bFetchStarted = FALSE;
		}
	}

	if (rgWarning)
		return ODBC_RG_WARNING;

	// Added for fix to SQL returning sqlcode=SQL_NO_DATA_FOUND for non-select
	// stmts when no rows get affected - 10/03/06
	if (sqlWarning && !(IGNORE_NODATAFOUND(sqlStmtType, execretcode)))
		return SQL_SUCCESS_WITH_INFO;
	else
		return SQL_SUCCESS;
}

// This function is written to execute internal statments which doesn't have any parameters
// and no output columns using WSQL_EXEC_ExecDirect call.
// We need to clean up the statement handle and descriptors and hence this function treats 
// them as if they are external statments
// This function will always log the error in event log

SQLRETURN SRVR::EXECDIRECT(char *pSqlStr, BOOL WriteError)
{
	SRVRTRACE_ENTER(FILE_INTF+33);

	SRVR_STMT_HDL	*srvrStmt;
	Int32			retcode;


	srvrStmt = getSrvrStmt("STMT_INTER_1", TRUE);
	if (srvrStmt == NULL)
         return ODBC_SERVER_ERROR;

	retcode = srvrStmt->ExecDirect(NULL, pSqlStr, INTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);
	
	if (retcode != SQL_SUCCESS)
	{
		// Need to get the SQL Error and log them as events
	}

	// RemoveSrvrStmt in Drop Option, since this srvrStmt will not be found in the GlobalList
	srvrStmt->Close(SQL_DROP);

	SRVRTRACE_EXIT(FILE_INTF+33);

	return (SQLRETURN)retcode;
}	

SQLRETURN SRVR::GETSQLWARNING(bool& bSQLMessageSet, 
					ERROR_DESC_LIST_def *sqlWarning)
{
	SRVRTRACE_ENTER(FILE_INTF+35);

	Int32 retcode;

	// This function is needed since SQLError uses odbc_SQLSvc_SQLError * instead of ERROR_DESC_LIST_def *
	// Hence this function wraps around GETSQLERROR
	odbc_SQLSvc_SQLError SQLError;

	retcode = GETSQLERROR(bSQLMessageSet, &SQLError);
	sqlWarning->_length = SQLError.errorList._length;
	sqlWarning->_buffer = SQLError.errorList._buffer;
	SRVRTRACE_EXIT(FILE_INTF+35);

	return (SQLRETURN)retcode;
}

SQLRETURN SRVR::CANCEL(SRVR_STMT_HDL *pSrvrStmt)
{
	SRVRTRACE_ENTER(FILE_INTF+36);

	Int32 retcode = SQL_SUCCESS;
	short i;
	
	for (i = 0; i < 5; i++)
	{
			retcode = WSQL_EXEC_Cancel(&pSrvrStmt->stmt);
			if (retcode == -8847 || retcode == 8847) // Cancel retry
				Sleep(50);
			else
			{
				// Any error 8848 (Cancel NOT possible will be sent up
				break;
			}
	}
	SRVRTRACE_EXIT(FILE_INTF+36);

	return (SQLRETURN)retcode;
}

SQLRETURN SRVR::ALLOCSQLMXHDLS(SRVR_STMT_HDL* pSrvrStmt)
{
	SRVRTRACE_ENTER(FILE_INTF+37);

	Int32 retcode = SQL_SUCCESS;
	SQLSTMT_ID	*pStmt = &pSrvrStmt->stmt;
	SQLDESC_ID	*pInputDesc;
	SQLDESC_ID	*pOutputDesc;
	SQLMODULE_ID *pModule = &pSrvrStmt->moduleId;
	BOOL		sqlWarning;

	pStmt->version = SQLCLI_ODBC_VERSION;
	pStmt->module = pModule;
	pStmt->handle = 0;
	pStmt->charset = SQLCHARSETSTRING_UTF8;
	if (pSrvrStmt->stmtName[0] != '\0')
	{
		pStmt->name_mode = stmt_name;
		pStmt->identifier_len = pSrvrStmt->stmtNameLen;
		pStmt->identifier = pSrvrStmt->stmtName;
	}
	else
	{
		pStmt->name_mode = stmt_handle;
		pStmt->identifier_len = 0;
		pStmt->identifier = NULL;
	}

	if (pModule->module_name == NULL)
	{
	        if (pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
		  {
                  retcode = WSQL_EXEC_AllocStmtForRS(pSrvrStmt->callStmtId, pSrvrStmt->resultSetIndex, pStmt);
                  HANDLE_ERROR2(retcode, sqlWarning);
		  }
                else
		  {
		  retcode = WSQL_EXEC_AllocStmt(pStmt,(SQLSTMT_ID *)NULL);
		  HANDLE_ERROR(retcode, sqlWarning);
		  }
	}
	
	pInputDesc = &pSrvrStmt->inputDesc;
	pInputDesc->version = SQLCLI_ODBC_VERSION;
	pInputDesc->handle = 0;
	pInputDesc->charset = SQLCHARSETSTRING_UTF8;
	if (pSrvrStmt->inputDescName[0] != '\0')
	{
		pInputDesc->name_mode = desc_name;
		pInputDesc->identifier_len = strlen(pSrvrStmt->inputDescName);
		pInputDesc->identifier = pSrvrStmt->inputDescName;
		pInputDesc->module = pModule;
	}
	else
	{
		pInputDesc->name_mode = desc_handle;
		pInputDesc->identifier_len = 0;
		pInputDesc->identifier = NULL;
		pInputDesc->module = &nullModule;
		retcode = WSQL_EXEC_AllocDesc(pInputDesc, (SQLDESC_ID *)NULL);
		HANDLE_ERROR(retcode, sqlWarning);
	}
	
	pOutputDesc = &pSrvrStmt->outputDesc;
	pOutputDesc->version = SQLCLI_ODBC_VERSION;
	pOutputDesc->handle = 0;
	pOutputDesc->charset = SQLCHARSETSTRING_UTF8;
	if (pSrvrStmt->outputDescName[0] != '\0')
	{
		pOutputDesc->name_mode = desc_name;
		pOutputDesc->identifier_len = strlen(pSrvrStmt->outputDescName);
		pOutputDesc->identifier = pSrvrStmt->outputDescName;
		pOutputDesc->module = pModule;
	}
	else
	{
		pOutputDesc->name_mode = desc_handle;
		pOutputDesc->identifier_len = 0;
		pOutputDesc->identifier = NULL;
		pOutputDesc->module = &nullModule;
		retcode = WSQL_EXEC_AllocDesc(pOutputDesc, (SQLDESC_ID *)NULL);
		HANDLE_ERROR(retcode, sqlWarning);
	}

	retcode = WSQL_EXEC_SetDescItem(pInputDesc, 0, SQLDESC_DESCRIPTOR_TYPE, DESCRIPTOR_TYPE_WIDE, NULL);
	HANDLE_ERROR(retcode, sqlWarning);
	retcode = WSQL_EXEC_SetDescItem(pOutputDesc, 0, SQLDESC_DESCRIPTOR_TYPE, DESCRIPTOR_TYPE_WIDE, NULL);
	HANDLE_ERROR(retcode, sqlWarning);

	SRVRTRACE_EXIT(FILE_INTF+37);

	return retcode;
}
//LCOV_EXCL_START
SQLRETURN SRVR::EXECUTECALL(SRVR_STMT_HDL* pSrvrStmt)
{
	SRVRTRACE_ENTER(FILE_INTF+38);

	Int32 retcode = SQL_SUCCESS;

	const SQLValueList_def *inputValueList = &pSrvrStmt->inputValueList;
	SQLValueList_def *outputValueList = &pSrvrStmt->outputValueList;

	SQLDESC_ID *pDesc;
    SQLSTMT_ID *pStmt;
	Int32		paramCount;
	Int32		len;
	Int32		curParamNo;
	Int32		curLength;
	BYTE		*varPtr;
	BYTE		*indPtr;
	void		*pBytes;
	SQLValue_def *SQLValue;
	Int32		curColumnNo;
	Int32		dataType;
	Int32		dataLength;
	Int32		allocLength;
	short		indValue;
	Int32		columnCount;
	Int32		Charset;
	BOOL		sqlWarning = FALSE;
	SRVR_DESC_HDL	*IPD;
	SRVR_DESC_HDL	*IRD;
	
	pStmt = &pSrvrStmt->stmt;
	
	if (pStmt == NULL)		
		return SQL_INVALID_HANDLE;
		
	pDesc = &pSrvrStmt->inputDesc;
	paramCount = pSrvrStmt->paramCount;

	// Initialize this to TRUE later in this function depending on sqlStmtType if SQL_SELECT
	// then and retcode is SUCCESS or SUCCESS_WITH_INFO set it to FALSE. Since SQL opens CURSOR
	// after execute in case of SUCCESS or SUCCESS_WITH_INFO and closes only when it reaches 
	// NO_DATA_FOUND or CLOSE STATEMENT or COMMIT WORK or ROLLBACK WORK.
	// Since this is TYPE_CALL statement and in case of output we only get 0 or one rows and the 
	// statement is closed since we use SQL_EXEC_FetchClose call.
	pSrvrStmt->isClosed = TRUE;

	if (paramCount == 0) // Ignore inputValueList
	{
		retcode = WSQL_EXEC_Exec(pStmt, NULL, 0);
		HANDLE_ERROR(retcode, sqlWarning);
		
    }
	else
	{
		// Else copy from inputValueList to SQL area
		if ( paramCount > (Int32)inputValueList->_length)
			return SQL_NEED_DATA;
		IPD = pSrvrStmt->IPD;
		for (curParamNo = 0 ; curParamNo < paramCount ; curParamNo++ )
		{
			SQLValue = (SQLValue_def *)inputValueList->_buffer + curParamNo;
			indPtr = IPD[curParamNo].indPtr;
			varPtr = IPD[curParamNo].varPtr;
			if (indPtr != NULL)
			{
				if (SQLValue->dataInd == -1)
					*((short *)indPtr) = -1;
				else
					*((short *)indPtr) = 0;
			}
			else
				if (SQLValue->dataInd == -1)
					return ODBC_SERVER_ERROR;
			if (SQLValue->dataInd != -1)
			{
				// Assuming that the client will send the data with the right length
				// Hence no checking done.
				pBytes = (void *)varPtr;
				memcpy(pBytes, (const void *)SQLValue->dataValue._buffer, SQLValue->dataValue._length);
			}
		}
		retcode = WSQL_EXEC_Exec(pStmt, pDesc, 0);
		// At present, we return error for this error condition
		// We could think of skipping this row and contine with the next row
		// at a later time
		HANDLE_ERROR(retcode, sqlWarning);
	}

	pDesc = &pSrvrStmt->outputDesc;  
	columnCount = pSrvrStmt->columnCount;

	if (columnCount > 0)
	{
		if (pDesc == NULL) // May be we should return internal Error
			return SQL_INVALID_HANDLE;
		retcode = WSQL_EXEC_FetchClose(pStmt, pDesc, 0);
		HANDLE_ERROR(retcode, sqlWarning);
		if (retcode == 100)
		{
			return SQL_NO_DATA_FOUND;
		}
		IRD = pSrvrStmt->IRD;
		for (curColumnNo = 0; curColumnNo < columnCount ; curColumnNo++)
		{
			dataType = IRD[curColumnNo].dataType;
			indPtr = IRD[curColumnNo].indPtr;
			
			if (IRD[curColumnNo].paramMode != PARAMETER_MODE_IN && ((indPtr == NULL) || (indPtr != NULL && *((short *)indPtr) != -1)))
				indValue = 0;
			else
				indValue = -1;
			Charset = IRD[curColumnNo].charSet;
			pBytes = (void *)(IRD[curColumnNo].varPtr);
			dataLength = IRD[curColumnNo].length;
			allocLength = getAllocLength(dataType, dataLength);
			kdsCopyToSQLValueSeq(outputValueList, dataType, indValue, pBytes, allocLength, Charset);    
		}
	}
	else
	{
		retcode = WSQL_EXEC_FetchClose(pStmt, NULL, 0);
		HANDLE_ERROR(retcode, sqlWarning);
	}
	SRVRTRACE_EXIT(FILE_INTF+38);

	if (sqlWarning)
		return SQL_SUCCESS_WITH_INFO;
	else
		return SQL_SUCCESS;
}
//LCOV_EXCL_STOP
SQLRETURN SRVR::PREPARE2(SRVR_STMT_HDL* pSrvrStmt,bool isFromExecDirect)
{ 
	SRVRTRACE_ENTER(FILE_INTF+23);

	Int32 retcode;
	UInt32 flags = 0;
	if(isFromExecDirect)
		flags = flags | PREPARE_STANDALONE_QUERY;

	if (strnicmp(pSrvrStmt->sqlString, "INFOSTATS", 9) == 0)
	{
		retcode = doInfoStats(pSrvrStmt);
		if (retcode != SQL_SUCCESS) 
			return retcode;
		pSrvrStmt->m_bSkipWouldLikeToExecute = true;
	}

	char *pSqlStr = pSrvrStmt->sqlString;
	RES_HIT_DESC_def	  *rgPolicyHit = &pSrvrStmt->rgPolicyHit;

	SQLSTMT_ID	*pStmt;
	SQLDESC_ID	*pInputDesc;
	SQLDESC_ID	*pOutputDesc;

	char        *pStmtName;
	BOOL		sqlWarning = FALSE;
	BOOL		rgWarning = FALSE;
	BOOL		shapeWarning = FALSE;
	Int32		tempmaxRowsetSize = 0;
	Int32 		holdableCursor = pSrvrStmt->holdableCursor;

	pSrvrStmt->PerfFetchRetcode = SQL_SUCCESS;
	pSrvrStmt->RowsetFetchRetcode = SQL_SUCCESS;

	pStmt = &pSrvrStmt->stmt;
	pOutputDesc = &pSrvrStmt->outputDesc;
	pInputDesc = &pSrvrStmt->inputDesc;

	SQLDESC_ID	sqlString_desc;
	sqlString_desc.version = SQLCLI_ODBC_VERSION;
	sqlString_desc.module = &pSrvrStmt->moduleId;
	sqlString_desc.name_mode = string_data;
	sqlString_desc.identifier     = (const char *) pSqlStr;
	sqlString_desc.handle = 0;
	sqlString_desc.identifier_len = pSrvrStmt->sqlStringLen;
	sqlString_desc.charset = SQLCHARSETSTRING_UTF8;
	UInt32 tmpBuildID = 0;


	if (srvrGlobal->drvrVersion.buildId & ROWWISE_ROWSET)
		retcode = WSQL_EXEC_SetDescItem(pOutputDesc, 0, SQLDESC_ROWSET_TYPE, 3, 0);
	else
		retcode = WSQL_EXEC_SetDescItem(pOutputDesc, 0, SQLDESC_ROWSET_TYPE, 0, 0);
	HANDLE_ERROR(retcode, sqlWarning);

	// We have to do the following CLI to set rowsize to ZERO, Since we switch between rowset to non-rowsets 
	retcode = WSQL_EXEC_SetStmtAttr(pStmt,SQL_ATTR_INPUT_ARRAY_MAXSIZE,tempmaxRowsetSize, NULL);
	HANDLE_ERROR(retcode, sqlWarning);

	// need to set HOLDABLE cursor only when it differs from current holdable cursor
	if (holdableCursor != pSrvrStmt->current_holdableCursor) {
		retcode =  WSQL_EXEC_SetStmtAttr(pStmt,SQL_ATTR_CURSOR_HOLDABLE,holdableCursor, NULL);
		pSrvrStmt->current_holdableCursor = holdableCursor;
		HANDLE_ERROR(retcode, sqlWarning);
	}
	
     if (pSrvrStmt->sqlQueryType != SQL_SP_RESULT_SET)
	 { 
	  	// Result sets are already prepared.
    	pSrvrStmt->m_aggPriority = 0;
		pSrvrStmt->sqlUniqueQueryIDLen = MAX_QUERY_NAME_LEN;
		pSrvrStmt->sqlUniqueQueryID[0] = '\0';
		retcode = WSQL_EXEC_Prepare2(pStmt, &sqlString_desc, NULL, NULL, NULL, &pSrvrStmt->cost_info, &pSrvrStmt->comp_stats_info, 
		                                     pSrvrStmt->sqlUniqueQueryID, &pSrvrStmt->sqlUniqueQueryIDLen, flags);
		pSrvrStmt->sqlUniqueQueryID[pSrvrStmt->sqlUniqueQueryIDLen] = '\0';
		pSrvrStmt->m_bNewQueryId = true;

		if ((srvrGlobal->isShapeLoaded == true) && (retcode == -2103 || retcode == -2104 || retcode == -2105))
		{
			GETSQLWARNINGORERROR2(pSrvrStmt);
			shapeWarning = TRUE;
			EXECDIRECT("CONTROL QUERY SHAPE CUT");
			pSrvrStmt->m_aggPriority = 0;
			pSrvrStmt->sqlUniqueQueryIDLen = MAX_QUERY_NAME_LEN;
			pSrvrStmt->sqlUniqueQueryID[0] = '\0';
			retcode = WSQL_EXEC_Prepare2(pStmt, &sqlString_desc, NULL, NULL, NULL, &pSrvrStmt->cost_info, &pSrvrStmt->comp_stats_info,
			                                     pSrvrStmt->sqlUniqueQueryID, &pSrvrStmt->sqlUniqueQueryIDLen, flags);
			pSrvrStmt->sqlUniqueQueryID[pSrvrStmt->sqlUniqueQueryIDLen] = '\0';
			pSrvrStmt->m_bNewQueryId = true;

		}
		HANDLE_ERROR(retcode, sqlWarning);

	 }
		
	if (pSrvrStmt->stmtType == EXTERNAL_STMT && srvrGlobal->srvrType == CORE_SRVR && (srvrGlobal->resGovernOn || (srvrGlobal->resourceStatistics & STMTSTAT_PREPARE)))
	{
		retcode = RESOURCEGOV(pSrvrStmt, pSqlStr, pSrvrStmt->cost_info.totalTime);
		switch (retcode)
		{
		case SQL_SUCCESS:
			break;
		case ODBC_RG_WARNING:
			rgWarning = TRUE;
			break;
		case ODBC_RG_ERROR:
			return (SQLRETURN)retcode;
		default:
			break;
		}
	}
	retcode = WSQL_EXEC_DescribeStmt(pStmt, pInputDesc, pOutputDesc);

	if (pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
	{
		// An SPJ can be declared to return say for example 3 Result Sets, but actually
		// return only 2 Result Sets. In this case, SQL_EXEC_AllocStmtForRS would not
		// return -8916 when we call it for the 3rd result set. However when we try
		// to describe it we would get a -8915. This should also be treated as a signal
		// that there are no more result sets. HANDLE_ERROR2 would return this value
		// to the previous stack frame where we will handle this condition
		HANDLE_ERROR2(retcode, sqlWarning);
	}
	else
	{
		HANDLE_ERROR(retcode, sqlWarning);
	}
		
	retcode = WSQL_EXEC_GetDescEntryCount(pInputDesc, &pSrvrStmt->paramCount);
	HANDLE_ERROR(retcode, sqlWarning);

	retcode = WSQL_EXEC_GetDescEntryCount(pOutputDesc, &pSrvrStmt->columnCount);
	HANDLE_ERROR(retcode, sqlWarning);

	// Child query visibility
	retcode = WSQL_EXEC_GetStmtAttr(pStmt, SQL_ATTR_SUBQUERY_TYPE, &pSrvrStmt->sqlSubQueryType, NULL, 0, NULL);

	retcode = WSQL_EXEC_GetStmtAttr(pStmt, SQL_ATTR_QUERY_TYPE, &pSrvrStmt->sqlQueryType, NULL, 0, NULL);
	// Added the below to treat the new SQL_EXE_UTIL type as SQL_SELECT_NON_UNIQUE 
	// in MXOSRVR to minimize code changes and to send the new query type to WMS.
	pSrvrStmt->sqlNewQueryType = pSrvrStmt->sqlQueryType;

	if( pSrvrStmt->sqlQueryType == SQL_EXE_UTIL )
	{
		if(pSrvrStmt->columnCount > 0)
			pSrvrStmt->sqlQueryType = SQL_SELECT_NON_UNIQUE;
		else
			pSrvrStmt->sqlQueryType = SQL_OTHER;

	}


	HANDLE_ERROR(retcode, sqlWarning);

	//if (pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
	//{
		if(srvrGlobal->bSpjEnableProxy)
		{
			Int32	requiredStringLen = 0;

			if(pSrvrStmt->SpjProxySyntaxString == NULL)
			{
				markNewOperator,pSrvrStmt->SpjProxySyntaxString = new char[2000];
				if (pSrvrStmt->SpjProxySyntaxString == NULL)
				{
					// Handle Memory Overflow execption here
					SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
								srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
								srvrGlobal->srvrObjRef, 1, "PREPARE2");
					exit(0);
				}
				pSrvrStmt->SpjProxySyntaxStringLen = 2000;
			}
			
			retcode = SRVR::WSQL_EXEC_GetStmtAttr(&pSrvrStmt->stmt,SQL_ATTR_RS_PROXY_SYNTAX,NULL, pSrvrStmt->SpjProxySyntaxString, 2000, &requiredStringLen);

			// In case the provided buffer is not sufficient for the proxy syntax or the pointer is NULL SQL returns -8889

			if( (retcode == -8889) || requiredStringLen + 1 > pSrvrStmt->SpjProxySyntaxStringLen) // we need to null terminate the string
			{
				delete[] pSrvrStmt->SpjProxySyntaxString;

				markNewOperator,pSrvrStmt->SpjProxySyntaxString = new char[requiredStringLen + 1];

				if (pSrvrStmt->SpjProxySyntaxString == NULL)
				{
					// Handle Memory Overflow execption here
					SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
								srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
								srvrGlobal->srvrObjRef, 1, "PREPARE2");
					exit(0);
				}
				pSrvrStmt->SpjProxySyntaxStringLen = requiredStringLen + 1;
				retcode = SRVR::WSQL_EXEC_GetStmtAttr(&pSrvrStmt->stmt,SQL_ATTR_RS_PROXY_SYNTAX,NULL, pSrvrStmt->SpjProxySyntaxString, requiredStringLen, &requiredStringLen);
				HANDLE_ERROR(retcode, sqlWarning);
			}

			pSrvrStmt->SpjProxySyntaxString[requiredStringLen] = '\0';
		}
		else
		{
			if(pSrvrStmt->SpjProxySyntaxString != NULL)
				delete[] pSrvrStmt->SpjProxySyntaxString;
			pSrvrStmt->SpjProxySyntaxString = NULL;
			pSrvrStmt->SpjProxySyntaxStringLen = 0;
		}
	//} // pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET


	if (pSrvrStmt->paramCount > 0)
	{
		tmpBuildID = srvrGlobal->drvrVersion.buildId; // should go way once we support rowwise rowsets
		srvrGlobal->drvrVersion.buildId = 0;
		retcode = BuildSQLDesc2(pInputDesc, -9999, 0, pSrvrStmt->sqlBulkFetchPossible, pSrvrStmt->paramCount,
				pSrvrStmt->inputDescBuffer, pSrvrStmt->inputDescBufferLength,
				pSrvrStmt->inputDescVarBuffer, pSrvrStmt->inputDescVarBufferLen, pSrvrStmt->IPD,pSrvrStmt->SqlDescInfo);
		srvrGlobal->drvrVersion.buildId = tmpBuildID; // should go way once we support rowwise rowsets
		HANDLE_ERROR(retcode, sqlWarning);
	}

	Int32 estRowLength=0;
	if (pSrvrStmt->columnCount > 0)
	{
		retcode = BuildSQLDesc2(pOutputDesc, pSrvrStmt->sqlQueryType,  pSrvrStmt->maxRowsetSize, pSrvrStmt->sqlBulkFetchPossible,
				pSrvrStmt->columnCount, pSrvrStmt->outputDescBuffer, pSrvrStmt->outputDescBufferLength,
				pSrvrStmt->outputDescVarBuffer, pSrvrStmt->outputDescVarBufferLen, pSrvrStmt->IRD,pSrvrStmt->SqlDescInfo);
		HANDLE_ERROR(retcode, sqlWarning);

		if (srvrGlobal->drvrVersion.buildId & ROWWISE_ROWSET)
		{
			estRowLength = pSrvrStmt->outputDescVarBufferLen;
			pSrvrStmt->bFirstSqlBulkFetch = true;
		}
		else
		{
			int columnCount = pSrvrStmt->columnCount;
			Int32 estLength;
			SRVR_DESC_HDL *IRD = pSrvrStmt->IRD;

			for (int curColumnNo = 0; curColumnNo < columnCount ; curColumnNo++)
			{
				IRD = pSrvrStmt->IRD;
				estLength = getAllocLength(IRD[curColumnNo].dataType, IRD[curColumnNo].length);
				estLength += 1;
				estRowLength += estLength;
			}
		}
	}
	pSrvrStmt->estRowLength = estRowLength;

	SRVRTRACE_EXIT(FILE_INTF+23);

	if (rgWarning)
		return ODBC_RG_WARNING;
	if (shapeWarning)
		return SQL_SHAPE_WARNING;
	if (sqlWarning)
		return SQL_SUCCESS_WITH_INFO;
	else
		return SQL_SUCCESS;
}

//---------------------------------------------------------------------------
// PREPARE2 Rowset prepare
SQLRETURN SRVR::PREPARE2withRowsets(SRVR_STMT_HDL* pSrvrStmt)
{

	SRVRTRACE_ENTER(FILE_INTF+44);

	SQLItemDescList_def *inputSQLDesc = &pSrvrStmt->inputDescList;

	char              *pSqlStr     = pSrvrStmt->sqlString;
	RES_HIT_DESC_def  *rgPolicyHit = &pSrvrStmt->rgPolicyHit;

	Int32               retcode;

	SQLSTMT_ID	*pStmt;
	SQLDESC_ID	*pInputDesc;
	SQLDESC_ID	*pOutputDesc;

	char		*pStmtName;
	BOOL		sqlWarning  = FALSE;
	BOOL		rgWarning   = FALSE;
	BOOL		shapeWarning = FALSE;
	Int32		tempmaxRowsetSize = 0;
	UInt32 tmpBuildID = 0;
	Int32		holdableCursor = pSrvrStmt->holdableCursor;

	pSrvrStmt->PerfFetchRetcode   = SQL_SUCCESS;
	pSrvrStmt->RowsetFetchRetcode = SQL_SUCCESS;

	pStmt = &pSrvrStmt->stmt;
	pOutputDesc = &pSrvrStmt->outputDesc;
	pInputDesc = &pSrvrStmt->inputDesc;

	SQLDESC_ID         sqlString_desc;
	sqlString_desc.version        = SQLCLI_ODBC_VERSION;
	sqlString_desc.module         = &pSrvrStmt->moduleId;
	sqlString_desc.name_mode      = string_data;
	sqlString_desc.identifier     = (const char *) pSqlStr;
	sqlString_desc.handle         = 0;
	sqlString_desc.identifier_len = pSrvrStmt->sqlStringLen;
	sqlString_desc.charset = SQLCHARSETSTRING_UTF8;

	retcode = WSQL_EXEC_SetDescItem(pOutputDesc, 0, SQLDESC_ROWSET_TYPE, 0, 0);
	HANDLE_ERROR(retcode, sqlWarning);

	if (pSrvrStmt->maxRowsetSize > 1 
/*
            && (   pSrvrStmt->sqlStmtType == TYPE_INSERT_PARAM
	        || pSrvrStmt->sqlStmtType == TYPE_UPDATE
	        || pSrvrStmt->sqlStmtType == TYPE_DELETE
		)
*/
	   )
		retcode = WSQL_EXEC_SetStmtAttr(pStmt,SQL_ATTR_INPUT_ARRAY_MAXSIZE,pSrvrStmt->maxRowsetSize, NULL);
	else
  	  // We have to do the following CLI to set rowsize to ZERO, 
	  // since we switch between rowset to non-rowsets 
	  retcode = WSQL_EXEC_SetStmtAttr(pStmt,SQL_ATTR_INPUT_ARRAY_MAXSIZE,tempmaxRowsetSize, NULL);

	HANDLE_ERROR(retcode, sqlWarning);

	// need to set HOLDABLE cursor only when it differs from current holdable cursor
	if (holdableCursor != pSrvrStmt->current_holdableCursor) {
		retcode =  WSQL_EXEC_SetStmtAttr(pStmt,SQL_ATTR_CURSOR_HOLDABLE,holdableCursor, NULL);
		pSrvrStmt->current_holdableCursor = holdableCursor;
		HANDLE_ERROR(retcode, sqlWarning);
	}	// need to set HOLDABLE cursor
	
	pSrvrStmt->NA_supported = true;		// NOT ATOMIC ROWSET RECOVERY initialization
	pSrvrStmt->m_aggPriority = 0;
	pSrvrStmt->sqlUniqueQueryIDLen = MAX_QUERY_NAME_LEN;
	pSrvrStmt->sqlUniqueQueryID[0] = '\0';
    retcode = WSQL_EXEC_Prepare2(pStmt, &sqlString_desc, NULL, NULL, NULL, &pSrvrStmt->cost_info, &pSrvrStmt->comp_stats_info, 
	                                     pSrvrStmt->sqlUniqueQueryID, &pSrvrStmt->sqlUniqueQueryIDLen);
 	pSrvrStmt->sqlUniqueQueryID[pSrvrStmt->sqlUniqueQueryIDLen] = '\0';
	pSrvrStmt->m_bNewQueryId = true;

	if (retcode != 30032) // ignore warning about recompile
	  HANDLE_ERROR(retcode, sqlWarning);
		
	if ((srvrGlobal->isShapeLoaded == true) && (retcode == -2103 || retcode == -2104 || retcode == -2105))
	{
		GETSQLWARNINGORERROR2(pSrvrStmt);
		shapeWarning = TRUE;
		EXECDIRECT("CONTROL QUERY SHAPE CUT");
		pSrvrStmt->m_aggPriority = 0;
		pSrvrStmt->sqlUniqueQueryIDLen = MAX_QUERY_NAME_LEN;
		pSrvrStmt->sqlUniqueQueryID[0] = '\0';
		retcode = WSQL_EXEC_Prepare2(pStmt, &sqlString_desc, NULL, NULL, NULL, &pSrvrStmt->cost_info, &pSrvrStmt->comp_stats_info, 
		                                     pSrvrStmt->sqlUniqueQueryID, &pSrvrStmt->sqlUniqueQueryIDLen);
		pSrvrStmt->sqlUniqueQueryID[pSrvrStmt->sqlUniqueQueryIDLen] = '\0';
		pSrvrStmt->m_bNewQueryId = true;

		HANDLE_ERROR(retcode, sqlWarning);
	}
	if (retcode == 30026 || retcode == 30028 || retcode == 30033 || retcode == 30034 || retcode == 30029 || pSrvrStmt->sqlStmtType == TYPE_UPDATE || pSrvrStmt->sqlStmtType == TYPE_DELETE)	// Based on SQL's input disabling the 30027 check and make it RFE in Executor. Enable it back once Executor supports it.
		pSrvrStmt->NA_supported = false; // NOT ATOMIC ROWSET RECOVERY is not supported for this SQL

	if (pSrvrStmt->stmtType == EXTERNAL_STMT && srvrGlobal->srvrType == CORE_SRVR && (srvrGlobal->resGovernOn || (srvrGlobal->resourceStatistics & STMTSTAT_PREPARE)))
	{
		retcode = RESOURCEGOV(pSrvrStmt, pSqlStr, pSrvrStmt->cost_info.totalTime);
		switch (retcode)
		{
		case SQL_SUCCESS:
			break;
		case ODBC_RG_WARNING:
			rgWarning = TRUE;
			break;
		case ODBC_RG_ERROR:
			return (SQLRETURN)retcode;
		default:
			break;
		}
	}
	retcode = WSQL_EXEC_DescribeStmt(pStmt, pInputDesc, pOutputDesc);
        if (retcode != 30032) // ignore warning about recompile
	  HANDLE_ERROR(retcode, sqlWarning);
		
	retcode = WSQL_EXEC_GetDescEntryCount(pInputDesc, &pSrvrStmt->paramCount);
        if (retcode != 30032) // ignore warning about recompile
	  HANDLE_ERROR(retcode, sqlWarning);

	retcode = WSQL_EXEC_GetDescEntryCount(pOutputDesc, &pSrvrStmt->columnCount);
        if (retcode != 30032) // ignore warning about recompile
	  HANDLE_ERROR(retcode, sqlWarning);

    // Child query visibility
    retcode = WSQL_EXEC_GetStmtAttr(pStmt, SQL_ATTR_SUBQUERY_TYPE, &pSrvrStmt->sqlSubQueryType, NULL, 0, NULL);

	retcode = WSQL_EXEC_GetStmtAttr(pStmt, SQL_ATTR_QUERY_TYPE, &pSrvrStmt->sqlQueryType, NULL, 0, NULL);

	// Added the below to treat the new SQL_EXE_UTIL type as SQL_SELECT_NON_UNIQUE 
	// in MXOSRVR to minimize code changes and to send the new query type to WMS.
	pSrvrStmt->sqlNewQueryType = pSrvrStmt->sqlQueryType;
	if( pSrvrStmt->sqlQueryType == SQL_EXE_UTIL )
	{
		if(pSrvrStmt->columnCount > 0)
			pSrvrStmt->sqlQueryType = SQL_SELECT_NON_UNIQUE;
		else
			pSrvrStmt->sqlQueryType = SQL_OTHER;
	}


	HANDLE_ERROR(retcode, sqlWarning);
	if (pSrvrStmt->paramCount > 0)
	{
		tmpBuildID = srvrGlobal->drvrVersion.buildId; // should go way once we support rowwise rowsets
		srvrGlobal->drvrVersion.buildId = 0;

		kdsCreateSQLDescSeq(inputSQLDesc, pSrvrStmt->paramCount);
		retcode = BuildSQLDesc2withRowsets( pInputDesc
                                                  , -9999
                                                  , 0
                                                  , pSrvrStmt->sqlBulkFetchPossible
                                                  , pSrvrStmt->paramCount
                                                  , pSrvrStmt->inputDescBuffer
                                                  , pSrvrStmt->inputDescBufferLength
                                                  , pSrvrStmt->inputDescVarBuffer
                                                  , pSrvrStmt->inputDescVarBufferLen
                                                  , pSrvrStmt->IPD
                                                  , pSrvrStmt->inputQuadList
												  , pSrvrStmt->inputQuadList_recover);

        srvrGlobal->drvrVersion.buildId = tmpBuildID; // should go way once we support rowwise rowsets);

		HANDLE_ERROR(retcode, sqlWarning);
	}

	Int32 estRowLength = 0;
	if (pSrvrStmt->columnCount > 0)
	{
		retcode = BuildSQLDesc2(pOutputDesc, pSrvrStmt->sqlQueryType,  pSrvrStmt->maxRowsetSize, pSrvrStmt->sqlBulkFetchPossible,
				pSrvrStmt->columnCount, pSrvrStmt->outputDescBuffer, pSrvrStmt->outputDescBufferLength,
				pSrvrStmt->outputDescVarBuffer, pSrvrStmt->outputDescVarBufferLen, pSrvrStmt->IRD,pSrvrStmt->SqlDescInfo);
		HANDLE_ERROR(retcode, sqlWarning);

		if (pSrvrStmt->sqlBulkFetchPossible && pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE)
		{
			estRowLength = pSrvrStmt->outputDescVarBufferLen;
			pSrvrStmt->bFirstSqlBulkFetch = true;
		}
		else
		{
			int columnCount = pSrvrStmt->columnCount;
			Int32 estLength;
			SRVR_DESC_HDL *IRD = pSrvrStmt->IRD;

			for (int curColumnNo = 0; curColumnNo < columnCount ; curColumnNo++)
			{
				IRD = pSrvrStmt->IRD;
				estLength = getAllocLength(IRD[curColumnNo].dataType, IRD[curColumnNo].length);
				estLength += 1;
				estRowLength += estLength;
			}
		}
	}
	pSrvrStmt->estRowLength = estRowLength;
        pSrvrStmt->preparedWithRowsets = TRUE;

	SRVRTRACE_EXIT(FILE_INTF+44);

	if (rgWarning)
		return ODBC_RG_WARNING;
	if (sqlWarning)
		return SQL_SUCCESS_WITH_INFO;
	else
		return SQL_SUCCESS;

}	// end PREPARE2withRowsets
//---------------------------------------------------------------------------


SQLRETURN SRVR::EXECUTE2(SRVR_STMT_HDL* pSrvrStmt)
{ 
	SRVRTRACE_ENTER(FILE_INTF+38);

	Int32 inputRowCnt = pSrvrStmt->inputRowCnt;
	IDL_short sqlStmtType = pSrvrStmt->sqlStmtType;
	Int64 rowsAffected_cli;
	Int32 *rowsAffected = &pSrvrStmt->rowsAffected;
	Int32 *rowsAffectedHigherBytes = &pSrvrStmt->rowsAffectedHigherBytes;


	Int32 retcode = SQL_SUCCESS;
	
	SQLDESC_ID *pInputDesc;
	SQLDESC_ID *pOutputDesc;
    SQLSTMT_ID *pStmt;
	SQLSTMT_ID cursorId;
	Int32		paramCount;
	char		*cursorName;
	Int32		len;
	Int32		curRowCnt;
	Int32		curParamNo;
	Int32		curLength;
	BYTE		*varPtr;
	BYTE		*indPtr;
	void			*pBytes;
	SQLValue_def	*SQLValue;
	BOOL			sqlWarning = FALSE;
	SRVR_DESC_HDL	*IPD;
	
	Int32		raretcode	= SQL_SUCCESS;

	pSrvrStmt->PerfFetchRetcode = SQL_SUCCESS;
	pSrvrStmt->RowsetFetchRetcode = SQL_SUCCESS;

	pStmt = &pSrvrStmt->stmt;

	// Initialize this to TRUE later in this function depending on sqlStmtType if SQL_SELECT
	// then and retcode is SUCCESS or SUCCESS_WITH_INFO set it to FALSE. Since SQL opens CURSOR
	// after execute in case of SUCCESS or SUCCESS_WITH_INFO and closes only when it reaches 
	// NO_DATA_FOUND or CLOSE STATEMENT or COMMIT WORK or ROLLBACK WORK.
      pSrvrStmt->isClosed = TRUE;

	if (pStmt == NULL)		
		return SQL_INVALID_HANDLE;

 	//
	// Handle statements.
	//
	if (    pSrvrStmt->stmtType == EXTERNAL_STMT
	    &&  (pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE || pSrvrStmt->sqlStmtType == TYPE_CALL)
	    &&  pSrvrStmt->moduleId.module_name == NULL)
	{
		cursorName = pSrvrStmt->cursorName;
		// If cursor name is not specified, use the stmt name as cursor name
		if (*cursorName == '\0')
			cursorName = pSrvrStmt->stmtName;
		// If cursorName has chg'd from last EXEC or EXECDIRECT cmd
		// or has not yet been set for the first time call SetCursorName
		if ((strcmp(pSrvrStmt->previousCursorName, cursorName) != 0) && *cursorName != '\0')
		{
			cursorId.version        = SQLCLI_ODBC_VERSION;
			cursorId.module         = pStmt->module;
			cursorId.handle         = 0;
			cursorId.charset        = SQLCHARSETSTRING_UTF8;
			cursorId.name_mode      = cursor_name;
			if (*cursorName == '\0')
				cursorId.identifier_len = pSrvrStmt->stmtNameLen;
			else
				cursorId.identifier_len = pSrvrStmt->cursorNameLen;
			cursorId.identifier     = cursorName;
			strcpy(pSrvrStmt->previousCursorName, cursorName);	// keep track of last cursorName
			retcode = WSQL_EXEC_SetCursorName(pStmt, &cursorId);
			HANDLE_ERROR(retcode, sqlWarning);
		}
	}
	pInputDesc = &pSrvrStmt->inputDesc;
	paramCount = pSrvrStmt->paramCount;
	
	if (  pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE
           || pSrvrStmt->sqlStmtType  == TYPE_CALL)
		pOutputDesc = &pSrvrStmt->outputDesc;
	else
		pOutputDesc = NULL;

	switch (pSrvrStmt->sqlQueryType)
	{
		case SQL_SELECT_UNIQUE:
		case SQL_CALL_NO_RESULT_SETS:
			retcode = WSQL_EXEC_SetDescItem(&pSrvrStmt->outputDesc, 0, SQLDESC_ROWWISE_ROWSET_SIZE, 1, 0);
			if(retcode >= 0)
				retcode = WSQL_EXEC_ClearExecFetchClose(pStmt, pInputDesc, pOutputDesc, 0, NULL, NULL);
			if(retcode == SQL_NO_DATA_FOUND) //SQL does NOT clear diagnostic records, so MXOSRVR clears it
				WSQL_EXEC_ClearDiagnostics(&pSrvrStmt->stmt);
			break;
		case SQL_SELECT_NON_UNIQUE:
			retcode = WSQL_EXEC_Exec(pStmt, pInputDesc, 0);
			break;
		case SQL_OTHER:
		case SQL_UNKNOWN:
		case SQL_CAT_UTIL:		// Added new types. Previously showing up as SQL_OTHER.
			// client side statement typing should not influence decisions on the server
			// removed to resolve issues with LOCK and PURGEDATA statements not being fetched/closed
			//if (sqlStmtType == TYPE_SELECT)  
				//retcode = WSQL_EXEC_Exec(pStmt, pInputDesc, 0);
			//else
			retcode = WSQL_EXEC_ClearExecFetchClose(pStmt, pInputDesc, pOutputDesc, 0, NULL, NULL);
			if(retcode == SQL_NO_DATA_FOUND) //SQL does NOT clear diagnostic records, so MXOSRVR clears it
				WSQL_EXEC_ClearDiagnostics(&pSrvrStmt->stmt);
			break;
		 case SQL_CALL_WITH_RESULT_SETS:
			WSQL_EXEC_CloseStmt(pStmt); // Added this code since for resultsets, The fetches till end of data will only close resultsets not the call statement.
			retcode = WSQL_EXEC_Exec(pStmt, pInputDesc, 0);
			if (retcode >= 0)
				retcode = WSQL_EXEC_SetDescItem(&pSrvrStmt->outputDesc, 0, SQLDESC_ROWWISE_ROWSET_SIZE, 1, 0);
				if (retcode >= 0)
					retcode = WSQL_EXEC_Fetch(&pSrvrStmt->stmt, &pSrvrStmt->outputDesc, 0);
			break;
		case SQL_RWRS_SPECIAL_INSERT:
				retcode = WSQL_EXEC_ExecFetch(pStmt, pInputDesc, 0);
			break;
		default:
			retcode = WSQL_EXEC_ClearExecFetchClose(pStmt, pInputDesc, pOutputDesc, 0, NULL, NULL);
			if (retcode == -8812)  
			{
				WSQL_EXEC_CloseStmt(pStmt);
				retcode = WSQL_EXEC_ClearExecFetchClose(pStmt, pInputDesc, pOutputDesc, 0, NULL, NULL);
			}
			if(retcode == SQL_NO_DATA_FOUND) //SQL does NOT clear diagnostic records, so MXOSRVR clears it
				WSQL_EXEC_ClearDiagnostics(&pSrvrStmt->stmt);
		 break;
	} // end switch

	if (retcode != SQL_NO_DATA_FOUND)
		HANDLE_ERROR(retcode, sqlWarning);

	rowsAffected_cli=0;
	*rowsAffected = 0;
	*rowsAffectedHigherBytes = 0;

	// 
	// this is to handle the case where AQR warnings are returned for these query types and its a SQL_NO_DATA condition
	// In this case, a AQR warning would be returned instead of SQL_NO_DATA. So would have to check the diagnostics
	// to see if a sql_no_data is retuned in the diagnostics to set the rowsAffected value
	// Using SQL_EXEC_GetDiagnosticsStmtInfo2 to get the rows affected does not work for these query types
	//

	if( pSrvrStmt->sqlQueryType == SQL_INSERT_UNIQUE || 
		pSrvrStmt->sqlQueryType == SQL_UPDATE_UNIQUE || 
		pSrvrStmt->sqlQueryType == SQL_DELETE_UNIQUE || 
		pSrvrStmt->sqlQueryType == SQL_SELECT_UNIQUE ||
		pSrvrStmt->sqlQueryType == SQL_CALL_NO_RESULT_SETS || 
		pSrvrStmt->sqlQueryType == SQL_CALL_WITH_RESULT_SETS)
	{
		*rowsAffected = 1;

		if(retcode == SQL_NO_DATA_FOUND)
			*rowsAffected = 0;
		else if(retcode > 0 ) // AQR warning (SQL_NO_DATA_FOUND already handle in the if-case above
		{
			Int32 total_conds = 0;
			Int32 curr_cond = 1;
			Int32 sqlcode  = 0;


			WSQL_EXEC_GetDiagnosticsStmtInfo2(NULL, SQLDIAG_NUMBER, (Int64*)&total_conds, NULL, 0, NULL);

			while(curr_cond <= total_conds)
			{
				WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_SQLCODE, curr_cond,	&sqlcode, NULL, 0, NULL);
				if(sqlcode == 100)
				{
				   *rowsAffected = 0;
				   break;
				}
				curr_cond++;
			}
		}
	}


	switch (pSrvrStmt->sqlQueryType)
	{
		case SQL_INSERT_UNIQUE:
		case SQL_UPDATE_UNIQUE:
		case SQL_DELETE_UNIQUE:
		  if(retcode == SQL_NO_DATA_FOUND)
			  retcode = SQL_SUCCESS;
		  // rowsAffected is already taken care of
		  break;
		case SQL_SELECT_UNIQUE:
		case SQL_CALL_NO_RESULT_SETS:
		case SQL_CALL_WITH_RESULT_SETS:
		  // rowsAffected is already taken care of
		  break;
		case SQL_OTHER:
		case SQL_UNKNOWN:
		case SQL_CAT_UTIL:		// Added new types. Previously showing up as SQL_OTHER.
			raretcode = WSQL_EXEC_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &rowsAffected_cli, NULL, 0, NULL);
			if (raretcode < SQL_SUCCESS)
			{
				*rowsAffected = -1;
				*rowsAffectedHigherBytes = -1;
			}
			else if (raretcode == 8411)
			{
				if (GetRowsAffected(pSrvrStmt) < SQL_SUCCESS) // Propagate rowsAffected as int64 once interface is changed
				{
					*rowsAffected = -1;
					*rowsAffectedHigherBytes = -1;
				}
			}
			else
			{
				*rowsAffected=(Int32)(rowsAffected_cli << 32 >> 32);
				*rowsAffectedHigherBytes=(Int32)(rowsAffected_cli>>32);
			}
		  break;
		case SQL_SELECT_NON_UNIQUE:
		  break;
		case SQL_INSERT_NON_UNIQUE:
 		case SQL_DELETE_NON_UNIQUE:
		case SQL_UPDATE_NON_UNIQUE:
		default:
		  if (retcode != SQL_NO_DATA_FOUND)
		  {
			raretcode = WSQL_EXEC_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &rowsAffected_cli, NULL, 0, NULL);
			if (raretcode < SQL_SUCCESS)
			{
				*rowsAffected = -1;
				*rowsAffectedHigherBytes = -1;
			}
			else if (raretcode == 8411)
			{
				if (GetRowsAffected(pSrvrStmt) < SQL_SUCCESS) // Propagate rowsAffected as int64 once interface is changed
				{
					*rowsAffected = -1;
					*rowsAffectedHigherBytes = -1;
				}
			}
			else
			{
				*rowsAffected=(Int32)(rowsAffected_cli << 32 >> 32);
				*rowsAffectedHigherBytes=(Int32)(rowsAffected_cli>>32);
			}
		  }
		  else
			retcode = SQL_SUCCESS;
		  break;
	}

	SRVRTRACE_EXIT(FILE_INTF+38);
	if (retcode == SQL_NO_DATA_FOUND)
		return SQL_NO_DATA_FOUND;

	// Initialize this to TRUE later in this function depending on sqlStmtType if SQL_SELECT
	// then and retcode is SUCCESS or SUCCESS_WITH_INFO set it to FALSE. Since SQL opens CURSOR
	// after execute in case of SUCCESS or SUCCESS_WITH_INFO and closes only when it reaches 
	// NO_DATA_FOUND or CLOSE STATEMENT or COMMIT WORK or ROLLBACK WORK.
	
	if ( (pSrvrStmt->sqlQueryType == SQL_SELECT_NON_UNIQUE) || 
		 (pSrvrStmt->sqlQueryType == SQL_CALL_WITH_RESULT_SETS))
	{
	     pSrvrStmt->isClosed = FALSE;
		 pSrvrStmt->bFetchStarted = FALSE;
    }

	// 
	// Process any SPJ RS 
	//
	if (pSrvrStmt->sqlQueryType == SQL_CALL_WITH_RESULT_SETS)
	{
		Int32            rsError            = 0;
		int             rsNum              = 1;
        char            rsName[MAX_STMT_NAME_LEN+1];
        SRVR_STMT_HDL  *rsSrvrStmt         = NULL;
        SRVR_STMT_HDL  *previousRsSrvrStmt = pSrvrStmt;  // point it to the CALL statement to make things work
        BOOL            done               = FALSE;
        int             prefixLen;
        SQLRETURN       rc;
        char           *suffix1    = "RS";
        int             suffixLen1 = strlen(suffix1);
        char           *suffix2    = "000";
        int             suffixLen2 = strlen(suffix2);
            
	  // Make prefix for Result Set names.
	  // For ease of debugging, try to prefix the cursor name with the call statement name.
	  // If the call statement name is too Int32, then generate a cursor name.
	  // The final form of the names should be something like:
	  //
	  //  SQL_CUR_123456789RS1
	  //  SQL_CUR_123456789RS2
	  //  ...
	  //
		if (pSrvrStmt->stmtNameLen < (MAX_STMT_NAME_LEN - suffixLen1 - suffixLen2))
	    {
            prefixLen          = pSrvrStmt->stmtNameLen;
            strncpy(rsName, pSrvrStmt->stmtName, prefixLen);
	    }
		else
	    {
			// The name is too Int32, so generate a time tamp name "SQL_CUR_xxx" where xxx is time.
			time_t  t1;
            char    t1Str[21];   // big enough for 64-bits (i.e. 20 digits) + NULL
            int     t1StrLen;           
            int     offset1;
            char   *prefix1    = "SQL_CUR_";
            int     prefixLen1 = strlen(prefix1);
            int     chopIndex  = 0;
            
			// Add prefix
            strncpy(rsName, prefix1, prefixLen1);
            t1 = time(NULL);

            // Add big unique number
            ltoa(t1, t1Str, 10);
			t1StrLen = strlen(t1Str);
            if (t1StrLen > MAX_STMT_NAME_LEN - suffixLen1 - suffixLen2)
			// Too Int32, so figure out how much to shorten it
			chopIndex = t1StrLen - (MAX_STMT_NAME_LEN - suffixLen1 - suffixLen2);
            
            strncpy(&rsName[prefixLen1], &t1Str[chopIndex], (t1StrLen - chopIndex));
			prefixLen = prefixLen + (t1StrLen - chopIndex);
	    }

		strncpy(&rsName[prefixLen], "RS", 2);
		prefixLen = prefixLen + 2;
        memset(&rsName[prefixLen], '\0', 4); // make sure null on end
         
		while (done == FALSE)
	    {
            // Make RS name
            itoa(rsNum, &rsName[prefixLen], 10);

            // Get SRVR_STMT_HDL
            rsSrvrStmt = getSrvrStmt( rsName
                                    , TRUE
		    						, NULL
		 							, SQLCLI_ODBC_MODULE_VERSION
									, 0
									, NULL
									, NULL
									, TYPE_UNKNOWN
                                    , SQL_SP_RESULT_SET
									, rsNum
									,&pSrvrStmt->stmt
									,&retcode 
                                    );
            if (rsSrvrStmt == NULL)
			{
				if (retcode != -8916 /* RS_INDEX_OUT_OF_RANGE */)
					HANDLE_ERROR(retcode, sqlWarning);

				rsNum = rsNum - 1;
				previousRsSrvrStmt->nextSpjRs = NULL;
				// Either done or warning, so stop processing result sets.
				done = TRUE;
			}
            else
			{
              rsSrvrStmt->isClosed = FALSE;

  			  // Build the output descriptor for the RS
			  retcode = PREPARE2(rsSrvrStmt);

			  if(retcode >= 0)
			  {
				  rsSrvrStmt->estRowLength = rsSrvrStmt->outputDescVarBufferLen;
				  retcode = WSQL_EXEC_Exec(&rsSrvrStmt->stmt, &rsSrvrStmt->inputDesc, 0);
				  HANDLE_ERROR(retcode, sqlWarning);
				  previousRsSrvrStmt->nextSpjRs = rsSrvrStmt;
				  rsSrvrStmt->previousSpjRs = previousRsSrvrStmt; 
				  previousRsSrvrStmt = rsSrvrStmt;
				  rsSrvrStmt->callStmtHandle = pSrvrStmt;
				  rsNum = rsNum + 1;
			  }
			  else if (retcode == -8915)
			  {
				// An SPJ can be declared to return say for example 3 Result Sets, but actually
				// return only 2 Result Sets. In this case, SQL_EXEC_AllocStmtForRS would not
				// return -8916 when we call it for the 3rd result set. However when we try
				// to describe it we would get a -8915. This should also be treated as a signal
				// that there are no more result sets. In this case we should free this statement
			   	 rsSrvrStmt->freeResourceOpt = SQL_DROP;

				 if(rsSrvrStmt->SpjProxySyntaxString != NULL)
				    delete [] rsSrvrStmt->SpjProxySyntaxString;

				 rsSrvrStmt->SpjProxySyntaxString = NULL;
				 rsSrvrStmt->SpjProxySyntaxStringLen = 0;
			     rc = FREESTATEMENT(rsSrvrStmt);

				 rsNum = rsNum - 1;
				 done = TRUE;
			  }
			  else
				  HANDLE_ERROR(retcode, sqlWarning);
			}
		}  // end while done == FALSE
          pSrvrStmt->numResultSets = rsNum;
    }  // end if pSrvrStmt->sqlQueryType == SQL_CALL_WITH_RESULT_SETS

	if (sqlWarning)
		return SQL_SUCCESS_WITH_INFO;
	else
		return SQL_SUCCESS;
}  // end EXECUTE2


//-----------------------------------------------------------------------------------------


// Rowset execute
SQLRETURN SRVR::EXECUTE2withRowsets(SRVR_STMT_HDL* pSrvrStmt)
{
	SRVRTRACE_ENTER(FILE_INTF+45);

	Int32                 inputRowCnt    =  pSrvrStmt->inputRowCnt;
	IDL_short                sqlStmtType    =  pSrvrStmt->sqlStmtType;
	const SQL_DataValue_def *inputDataValue = &pSrvrStmt->inputDataValue;
	Int64  rowsAffected_cli;
	Int32                *rowsAffected  = &pSrvrStmt->rowsAffected;
	Int32				*rowsAffectedHigherBytes = &pSrvrStmt->rowsAffectedHigherBytes;

	Int32		maxRowsetSize;
	Int32		retcode        = SQL_SUCCESS;
	Int32		execretcode	= SQL_SUCCESS;

	Int32		raretcode	= SQL_SUCCESS;

	BYTE		          *indBuffPtr  = NULL;
	BYTE		          *dataBuffPtr = NULL;
	short	 	          *typeBuffPtr;
	struct SQLCLI_QUAD_FIELDS *quadBuffPtr;

	short		          *TypePtr;
	struct SQLCLI_QUAD_FIELDS *QuadPtr;

	short		          *curTypePtr;
	struct SQLCLI_QUAD_FIELDS *curQuadPtr;

	SQLDESC_ID     *pDesc;
        SQLSTMT_ID     *pStmt;
	Int32		paramCount;
	char	       *cursorName;
	Int32		curRowInRowset;
	Int32		curParamNo;
	Int32		curLength;
	Int32		varPtr;
	Int32		indPtr;
	Int32		DataType;
	Int32		DataLen;
	Int32		offset_var_layout,offset_ind_layout;
	Int32		len,datalen,indlen;
	Int32		sqlMaxLength;

	short		SQLDataInd     = 0;
	short		SQLDataLength  = 0;
	void*		SQLDataValue;

	SQLSTMT_ID	cursorId;
	BOOL		sqlWarning     = FALSE;
	Int32		lendata;
        Int32            lenind;

	pSrvrStmt->PerfFetchRetcode = SQL_SUCCESS;
	pSrvrStmt->RowsetFetchRetcode = SQL_SUCCESS;

	if (&pSrvrStmt->stmt == NULL)		
		return SQL_INVALID_HANDLE;

	maxRowsetSize = pSrvrStmt->maxRowsetSize;

	// Initialize this to TRUE later in this function depending on sqlStmtType if SQL_SELECT
	// then and retcode is SUCCESS or SUCCESS_WITH_INFO set it to FALSE. Since SQL opens CURSOR
	// after execute in case of SUCCESS or SUCCESS_WITH_INFO and closes only when it reaches 
	// NO_DATA_FOUND or CLOSE STATEMENT or COMMIT WORK or ROLLBACK WORK.
	pSrvrStmt->isClosed = TRUE;

	pStmt = &pSrvrStmt->stmt;
	if (pSrvrStmt->stmtType == EXTERNAL_STMT && sqlStmtType == TYPE_SELECT &&
			pSrvrStmt->moduleId.module_name == NULL)
	{
		cursorName = pSrvrStmt->cursorName;
		// If cursor name is not specified, use the stmt name as cursor name
		if (*cursorName == '\0')
			cursorName = pSrvrStmt->stmtName;
		// If cursorName has chg'd from last EXEC or EXECDIRECT cmd
		// or has not yet been set for the first time call SetCursorName
		if ((strcmp(pSrvrStmt->previousCursorName, cursorName) != 0) && *cursorName != '\0')
		{
			cursorId.version        = SQLCLI_ODBC_VERSION;
			cursorId.module         = pStmt->module;
			cursorId.handle         = 0;
			cursorId.charset        = SQLCHARSETSTRING_UTF8;
			cursorId.name_mode      = cursor_name;
			if (*cursorName == '\0')
				cursorId.identifier_len = pSrvrStmt->stmtNameLen;
			else
				cursorId.identifier_len = pSrvrStmt->cursorNameLen;
			cursorId.identifier     = cursorName;
			strcpy(pSrvrStmt->previousCursorName, cursorName);	// keep track of last cursorName
			retcode = WSQL_EXEC_SetCursorName(pStmt, &cursorId);
			HANDLE_ERROR(retcode, sqlWarning);
		}

	}
	pDesc = &pSrvrStmt->inputDesc;
	paramCount = pSrvrStmt->paramCount;

        if (paramCount > 0)
	  {
          BYTE *base = pSrvrStmt->IPD[0].varPtr;

          pSrvrStmt->inputQuadList[0].var_layout = 0;
          pSrvrStmt->inputQuadList[0].var_ptr    = &inputRowCnt; // KAS this is a bug. Need to use type returned in prepare.
          pSrvrStmt->inputQuadList[0].ind_layout = 0;
          pSrvrStmt->inputQuadList[0].ind_ptr    = NULL;

          for (int i = 1; i < paramCount; i++)
	    {
	    pSrvrStmt->inputQuadList[i].var_ptr = pSrvrStmt->transportBuffer + ((pSrvrStmt->IPD[i-1].varPtr - base) *  inputRowCnt);
            if (pSrvrStmt->IPD[i-1].indPtr != 0)
  	      pSrvrStmt->inputQuadList[i].ind_ptr = pSrvrStmt->transportBuffer + ((pSrvrStmt->IPD[i-1].indPtr - base) *  inputRowCnt);
            else
	      {
	      pSrvrStmt->inputQuadList[i].ind_layout = 0;
              pSrvrStmt->inputQuadList[i].ind_ptr    = NULL;
	      }
	    }  // end for

          SQLCLI_QUAD_FIELDS *quadList = pSrvrStmt->inputQuadList;

  	  retcode = WSQL_EXEC_SETROWSETDESCPOINTERS( pDesc,
						     maxRowsetSize,
						     NULL,
						     1,
						     paramCount,
						     quadList);
	  if (retcode < SQL_SUCCESS)
		goto ret;
	  }  // end if (paramCount > 0)

	retcode = execretcode = WSQL_EXEC_ExecFetch(pStmt, pDesc, 0);

	pSrvrStmt->numErrRows = 0;

	rowsAffected_cli=0;
	*rowsAffected = 0;
	*rowsAffectedHigherBytes = 0;
	if (retcode == 30022 || retcode == 30035) // some rows are inserted
	{
		raretcode = WSQL_EXEC_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &rowsAffected_cli, NULL, 0, NULL);
		if (raretcode < SQL_SUCCESS)
		{
			*rowsAffected = -1;
			*rowsAffectedHigherBytes = -1;
		}
		else if (raretcode == 8411)
		{
			if (GetRowsAffected(pSrvrStmt) < SQL_SUCCESS)
			{
				*rowsAffected = -1;
				*rowsAffectedHigherBytes = -1;
			}
		}
		else
		{
			*rowsAffected=(Int32)(rowsAffected_cli << 32 >> 32);
			*rowsAffectedHigherBytes=(Int32)(rowsAffected_cli>>32);
		}

		// get information about inserted rows
		GETNOTATOMICROWSET2(pSrvrStmt->bSQLMessageSet, &pSrvrStmt->sqlWarning, pSrvrStmt);
	}
	else if (retcode < SQL_SUCCESS)
	{
		if (    (   sqlStmtType == TYPE_INSERT_PARAM
                         || sqlStmtType == TYPE_UPDATE
                         || sqlStmtType == TYPE_DELETE
		        )
                    &&  (srvrGlobal->EnvironmentType & MXO_ROWSET_ERROR_RECOVERY) 
                    &&  pSrvrStmt->NA_supported   == false
                    &&  srvrGlobal->bAutoCommitOn == true
                    )
		{
			retcode = RECOVERY_FROM_ROWSET_ERROR2(
					pSrvrStmt,
					pDesc,
					pStmt,
					inputRowCnt,
					&rowsAffected_cli);
			COPYSQLERROR_LIST_TO_SRVRSTMT(pSrvrStmt);
			if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
                           retcode = ROWSET_SQL_ERROR;
		}
		else
			retcode = SQL_ERROR;

		*rowsAffected=(Int32)(rowsAffected_cli << 32 >> 32);
		*rowsAffectedHigherBytes=(Int32)(rowsAffected_cli>>32);		
		goto ret;
	}
	else
	{
		HANDLE_ERROR(retcode, sqlWarning);
		raretcode = WSQL_EXEC_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &rowsAffected_cli, NULL, 0, NULL);
		if (raretcode < SQL_SUCCESS)
		{
			*rowsAffected = -1;
			*rowsAffectedHigherBytes = -1;
		}
		else if (raretcode == 8411)
		{
			if (GetRowsAffected(pSrvrStmt) < SQL_SUCCESS)
			{
				*rowsAffected = -1;
				*rowsAffectedHigherBytes = -1;
			}
		}
		else
		{
			*rowsAffected=(Int32)(rowsAffected_cli << 32 >> 32);
			*rowsAffectedHigherBytes=(Int32)(rowsAffected_cli>>32);
		}

	}

	// Added for MODE_SPECIAL_1 behavior
	if (srvrGlobal->modeSpecial_1 && pSrvrStmt->sqlStmtType == TYPE_INSERT_PARAM)
	{
		// In case where we have all the rows in the rowset failing with error -8102 (dup key) then 
		// SQL is returning us code 100 without any diagnostics. So, we will treat this case as
		// a successesful insert.
		if( retcode == 100 ) {
			sqlWarning = false;
			retcode = SQL_SUCCESS;
		}

		if( *rowsAffected >= 0 && *rowsAffected < inputRowCnt && retcode != -1 )
		{
			if( pSrvrStmt->numErrRows > 0 )	// There are errors in diags
				*rowsAffected = inputRowCnt - pSrvrStmt->numErrRows;
			else
				*rowsAffected = inputRowCnt;
		}
	}

ret:

	SRVRTRACE_EXIT(FILE_INTF+45);

	if (retcode < SQL_SUCCESS)
			return retcode;

	// Initialize this to TRUE later in this function depending on sqlStmtType if SQL_SELECT
	// then and retcode is SUCCESS or SUCCESS_WITH_INFO set it to FALSE. Since SQL opens CURSOR
	// after execute in case of SUCCESS or SUCCESS_WITH_INFO and closes only when it reaches 
	// NO_DATA_FOUND or CLOSE STATEMENT or COMMIT WORK or ROLLBACK WORK.
	if (sqlStmtType == TYPE_SELECT)
	      pSrvrStmt->isClosed = FALSE;

	// Added for fix to SQL returning sqlcode=SQL_NO_DATA_FOUND for non-select
	// stmts when no rows get affected - 10/03/06
	if ((sqlWarning || retcode == SQL_SUCCESS_WITH_INFO)
			&& !(IGNORE_NODATAFOUND(sqlStmtType, execretcode)))
		return SQL_SUCCESS_WITH_INFO;
	else
		return SQL_SUCCESS;
}  // end EXECUTE2withRowsets


//-----------------------------------------------------------------------------------------

SQLRETURN SRVR::FETCH2bulk(SRVR_STMT_HDL *pSrvrStmt)
{
	SRVRTRACE_ENTER(FILE_INTF+29);

	Int32 retcode = SQL_SUCCESS;
	Int32 fetchretcode = SQL_SUCCESS; //2103
	SQLDESC_ID* pDesc = &pSrvrStmt->outputDesc;  
	BOOL sqlWarning = FALSE;
	Int32 RowsFetched = 0;

	bool bRWRS = false;
	if (srvrGlobal->drvrVersion.buildId & ROWWISE_ROWSET)
		bRWRS = true;

	//Changes due to cursor issue
	if (pSrvrStmt->maxRowCnt > 0)
	{
		if (pSrvrStmt->outputDescVarBufferLen > 0)
		{
			if( srvrGlobal->m_FetchBufferSize/pSrvrStmt->outputDescVarBufferLen  < pSrvrStmt->maxRowCnt ) 
			{
				if (pSrvrStmt->outputDescVarBuffer != NULL)
					delete pSrvrStmt->outputDescVarBuffer;
				pSrvrStmt->outputDescVarBuffer = NULL;
				markNewOperator,pSrvrStmt->outputDescVarBuffer = new BYTE[pSrvrStmt->maxRowCnt*pSrvrStmt->outputDescVarBufferLen];
				if (pSrvrStmt->outputDescVarBuffer == NULL)
				{
					// Handle Memory Overflow exception here
					SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
								srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
								srvrGlobal->srvrObjRef, 1, "_buffer:FETCH2bulk");
					exit(0);
				}
				SetIndandVarPtr(&pSrvrStmt->outputDesc,bRWRS,pSrvrStmt->columnCount,pSrvrStmt->outputDescBuffer,pSrvrStmt->outputDescVarBuffer,
				pSrvrStmt->outputDescVarBufferLen,pSrvrStmt->IRD,pSrvrStmt->SqlDescInfo);				
								

				retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWWISE_ROWSET_PTR, (long)pSrvrStmt->outputDescVarBuffer, 0);
				HANDLE_ERROR(retcode, sqlWarning);
			}
		}
		retcode = WSQL_EXEC_SetDescItem(pDesc, 0, SQLDESC_ROWWISE_ROWSET_SIZE, pSrvrStmt->maxRowCnt, 0);
		HANDLE_ERROR(retcode, sqlWarning);
	//Changes due to cursor issue
	}
	else
	pSrvrStmt->maxRowCnt = srvrGlobal->m_FetchBufferSize/pSrvrStmt->outputDescVarBufferLen;	// temp fix which is 1 mb. Make sure
																			// you change it in BuildSQLDesc2 & fetch2 also.
	Int32 maxRowCnt = pSrvrStmt->maxRowCnt; 
	Int32 maxRowLen = pSrvrStmt->maxRowLen;
	Int32 *rowsAffected = &pSrvrStmt->rowsAffected; 

	Int32 curRowCnt;

	if (pSrvrStmt->PerfFetchRetcode == SQL_NO_DATA_FOUND)
		return SQL_NO_DATA_FOUND;

	
	SQLSTMT_ID*	pStmt = &pSrvrStmt->stmt;
	
	if (pSrvrStmt->bFirstSqlBulkFetch)
	{
		pSrvrStmt->bFirstSqlBulkFetch = false;
	}

	retcode = WSQL_EXEC_Fetch(pStmt, pDesc, 0);
	if (retcode < SQL_SUCCESS)
	{
		fetchretcode = retcode; //2103
		retcode = SQL_ERROR;
		goto ret;
	}
	//else if (retcode > SQL_SUCCESS && retcode != 100)
	//	retcode = SQL_SUCCESS;

	WSQL_EXEC_GetDescItem(pDesc,1,SQLDESC_ROWSET_NUM_PROCESSED,&RowsFetched,0,0,0,0);


ret:

	//2103
	if (fetchretcode == -8007)
		pSrvrStmt->isClosed = TRUE;

	*rowsAffected = RowsFetched;

	if (retcode == 100)
	{
		WSQL_EXEC_CloseStmt(&pSrvrStmt->stmt);
		pSrvrStmt->isClosed = TRUE;
		pSrvrStmt->PerfFetchRetcode = SQL_NO_DATA_FOUND;

        if(pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
		{
		   // 
		   // If all the result sets have been fetched to end, the CALL
		   // stmt has to be closed, to allow SQL to reclaim resources
		   //
		   int numClosedResultSets = 0;
		   SRVR_STMT_HDL *callStmt = pSrvrStmt->callStmtHandle;
		   SRVR_STMT_HDL *rsStmt = callStmt->nextSpjRs;
		   while(rsStmt != NULL)
		   {
		      if(rsStmt->isClosed == TRUE)
			     numClosedResultSets++;
			  rsStmt = rsStmt->nextSpjRs;
		   }
		   if(numClosedResultSets == callStmt->numResultSets)
		   {
		      WSQL_EXEC_CloseStmt(&callStmt->stmt);
			  callStmt->isClosed = TRUE;
		      callStmt->PerfFetchRetcode = SQL_NO_DATA_FOUND;
		   }
		} // if(pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
	} // if retcode == 100

	if (retcode == SQL_SUCCESS)
	{
		// Should not happen, but just in case
		if (RowsFetched == 0)
			retcode = SQL_NO_DATA_FOUND;
	}
	else if (retcode == 100)
	{
		if (RowsFetched == 0)
			retcode = SQL_NO_DATA_FOUND;
		else
		{
			pSrvrStmt->PerfFetchRetcode = SQL_NO_DATA_FOUND;
			retcode = SQL_SUCCESS;
		}
	}
	else if(retcode > 0)  // AQR warning case, SQL_NO_DATA_FOUND is already handled in the if-case above
	{
		//
		// Warning - possibly AQR or stream access
		//
		//if (RowsFetched == 0)
		//	retcode = SQL_NO_DATA_FOUND;
		//else
		retcode = SQL_SUCCESS_WITH_INFO;

		Int32 total_conds = 0;
		Int32 curr_cond = 1;
		Int32 sqlcode  = 0;

		WSQL_EXEC_GetDiagnosticsStmtInfo2(NULL, SQLDIAG_NUMBER, (Int64*)&total_conds, NULL, 0, NULL); //total_conds actually 32bit, and SQLCli return 32bit

		while(curr_cond <= total_conds)
		{
			WSQL_EXEC_GetDiagnosticsCondInfo2(SQLDIAG_SQLCODE, curr_cond,	&sqlcode, NULL, 0, NULL);
			if(sqlcode == 100)
			{
				WSQL_EXEC_CloseStmt(&pSrvrStmt->stmt);
				pSrvrStmt->isClosed = TRUE;
				pSrvrStmt->PerfFetchRetcode = SQL_NO_DATA_FOUND;

		        if(pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
				{
				   // 
				   // If all the result sets have been fetched to end, the CALL
				   // stmt has to be closed, to allow SQL to reclaim resources
				   //
				   int numClosedResultSets = 0;
				   SRVR_STMT_HDL *callStmt = pSrvrStmt->callStmtHandle;
				   SRVR_STMT_HDL *rsStmt = callStmt->nextSpjRs;
				   while(rsStmt != NULL)
				   {
				      if(rsStmt->isClosed == TRUE)
					     numClosedResultSets++;
					  rsStmt = rsStmt->nextSpjRs;
				   }
				   if(numClosedResultSets == callStmt->numResultSets)
				   {
				      WSQL_EXEC_CloseStmt(&callStmt->stmt);
					  callStmt->isClosed = TRUE;
				      callStmt->PerfFetchRetcode = SQL_NO_DATA_FOUND;
				   }
				} // if(pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)

			} // if sqlcode == 100
			curr_cond++;
		}

	}

	SRVRTRACE_EXIT(FILE_INTF+29);

	return retcode;
}  // end FETCH2bulk

// Performance Fetch for Catalog APIs
SQLRETURN SRVR::FETCHCATALOGPERF(SRVR_STMT_HDL *pSrvrStmt, 
				Int32 maxRowCnt, 
				Int32 maxRowLen,
				Int32 *rowsAffected, 
				SQL_DataValue_def *outputDataValue)
{
	SRVRTRACE_ENTER(FILE_INTF+39);

	Int32 retcode = SQL_SUCCESS;
	Int32 curRowCnt = 1;
	Int32 curColumnNo;
	Int32 varPtr;
	Int64 indPtr;
	void *pBytes;
	Int32 dataType;
	Int32 dataLength;
	Int32 allocLength;
	Int32 srcDataLength;
	short indValue;
	Int32 columnCount;
	Int32 Charset;
	SQLDESC_ID *pDesc;
	BOOL sqlWarning = FALSE;
	SRVR_DESC_HDL		*IRD;
	SQL_DataValue_def tempOutputDataValue;

	Int32 estLength=0;
	Int32 estRowLength=0;
	Int32 initestTotalLength=0;
	Int32 increstTotalLength=0;
	Int32 size;

	pDesc = &pSrvrStmt->outputDesc;  
	columnCount = pSrvrStmt->columnCount;

	for (curColumnNo = 0; curColumnNo < columnCount ; curColumnNo++)
	{
		IRD = pSrvrStmt->IRD;
		estLength = getAllocLength(IRD[curColumnNo].dataType, IRD[curColumnNo].length);
		estLength += 1;
		estRowLength += estLength;
	}
	initestTotalLength = estRowLength * (maxRowCnt <= 0 ? 100 : maxRowCnt);
 	if (outputDataValue->_length == 0)
	{
		increstTotalLength = initestTotalLength;
		markNewOperator,outputDataValue->_buffer = new BYTE[initestTotalLength + 1];
		if (outputDataValue->_buffer == NULL)
		{
			// Handle Memory Overflow execption here
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
						srvrGlobal->srvrObjRef, 1, "_buffer:FETCHPERF");
			exit(0);
		}
		memset(outputDataValue->_buffer,0,initestTotalLength + 1);
	}
	else
	{
		increstTotalLength = ((outputDataValue->_length / initestTotalLength) + 1) * initestTotalLength;
	}

	while (retcode != 100)
	{
		retcode = WSQL_EXEC_Fetch(&pSrvrStmt->stmt, pDesc, 0);
		if (retcode != 0)
		{
			if (retcode == 100)
			{
				if (curRowCnt == 1)
				{
					retcode = SQL_NO_DATA_FOUND;
					goto ret;
				}
				else
				{
					*rowsAffected += curRowCnt-1;
					retcode = SQL_SUCCESS;
					goto ret;
				}
			}
			else
			if (retcode < 0)
			{
				retcode = SQL_ERROR;
				goto ret;
			}
			else
				sqlWarning = TRUE;
		}
		for (curColumnNo = 0; curColumnNo < columnCount ; curColumnNo++)
		{
			IRD = pSrvrStmt->IRD;
			indPtr = (intptr_t)IRD[curColumnNo].indPtr;
			dataType = IRD[curColumnNo].dataType;
			dataLength = IRD[curColumnNo].length;
			Charset = IRD[curColumnNo].charSet;
			pBytes = (void *)(IRD[curColumnNo].varPtr);

			if ((indPtr == NULL) || (indPtr != NULL && *((short *)indPtr) != -1))
			{
				indValue = 0;
				switch(dataType)
				{
					case SQLTYPECODE_VARCHAR_WITH_LENGTH:
					case SQLTYPECODE_VARCHAR_LONG:
					case SQLTYPECODE_BITVAR:
					case SQLTYPECODE_BLOB:
					case SQLTYPECODE_CLOB:
                                        case SQLTYPECODE_VARBINARY:
						dataLength = *(USHORT *)pBytes;

						allocLength = dataLength + 3;
						if (maxRowLen != 0)
						{
							allocLength = (allocLength>(UInt32)maxRowLen+3)?(UInt32)maxRowLen+3:allocLength;
							srcDataLength = *(USHORT *)pBytes;
							srcDataLength = (srcDataLength>(UInt32)maxRowLen)?(UInt32)maxRowLen:srcDataLength;
							*(USHORT *)pBytes=srcDataLength;
						}
						break;
					case SQLTYPECODE_CHAR:
					case SQLTYPECODE_BIT:
					case SQLTYPECODE_VARCHAR:
                                        case SQLTYPECODE_BINARY:
						allocLength = dataLength + 1;
						if (maxRowLen != 0)
							allocLength = (allocLength>(UInt32)maxRowLen+1)?(UInt32)maxRowLen+1:allocLength;
						break;
					default:
						allocLength = dataLength;
				}
				size = outputDataValue->_length;
				outputDataValue->_length += (allocLength+1);
				*(outputDataValue->_buffer+size) = '\0';
				memcpy(outputDataValue->_buffer+size+1, pBytes, allocLength);
			}
			else
			{
				size = outputDataValue->_length;
				indValue = -1;
				outputDataValue->_length += 1;
				*(outputDataValue->_buffer+size) = '\1';
			}
		}
		curRowCnt++;
		if (increstTotalLength - outputDataValue->_length < estRowLength)
		{
			if (outputDataValue->_buffer != NULL)
			{
				memset(&tempOutputDataValue, NULL, sizeof(tempOutputDataValue));

				markNewOperator,tempOutputDataValue._buffer = new BYTE[increstTotalLength + 1];
				if (tempOutputDataValue._buffer == NULL)
				{
					SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
							srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
							srvrGlobal->srvrObjRef, 1, "FETCHCATALOGPERF");
					exit(0);
				}
				memset(tempOutputDataValue._buffer,0,increstTotalLength + 1);
				tempOutputDataValue._length = outputDataValue->_length;
				memcpy(tempOutputDataValue._buffer, outputDataValue->_buffer, outputDataValue->_length);
				delete outputDataValue->_buffer;
				outputDataValue->_length = 0;
				outputDataValue->_buffer = NULL;

				increstTotalLength = increstTotalLength + initestTotalLength;

				markNewOperator,outputDataValue->_buffer = new BYTE[increstTotalLength + 1];
				outputDataValue->_length = tempOutputDataValue._length;
				memset(outputDataValue->_buffer,0,increstTotalLength + 1);
				memcpy(outputDataValue->_buffer, tempOutputDataValue._buffer, tempOutputDataValue._length);
				delete tempOutputDataValue._buffer;
				tempOutputDataValue._length = 0;
				tempOutputDataValue._buffer = NULL;
			}
			else
			{
				// Handle Memory Overflow execption here
				SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
							srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
							srvrGlobal->srvrObjRef, 1, "_buffer:FETCHPERF");
				exit(0);
			}

		}
	}
	*rowsAffected += curRowCnt;
	SRVRTRACE_EXIT(FILE_INTF+39);

	if (sqlWarning)
		retcode = SQL_SUCCESS_WITH_INFO;
	else
		retcode = SQL_SUCCESS;
ret:
	return retcode;
}
//========================NOT ATOMIC ROWSET==================================

extern "C" SQLRETURN GETNOTATOMICROWSET2(bool& bSQLMessageSet, ERROR_DESC_LIST_def *sqlWarning, SRVR_STMT_HDL* pSrvrStmt)
{
#define MAX_SQLSTATE_LEN 6
#define MAX_MSG_TEXT_LEN 350

	SQLDIAG_COND_INFO_ITEM_VALUE* pCondInfoItems = NULL;
	Int32* pSqlcode = NULL;
	Int32* pRowNumber = NULL;
	Int32* pSqlstateLen = NULL;
	Int32* pMessageTextLen = NULL;
	char* pSqlstate = NULL;
	char* pMessageText = NULL;
	UInt32 gbuf_len;
	char* gbuf_ptr = NULL;

	odbc_SQLSvc_SQLError SSQLError;
	odbc_SQLSvc_SQLError* SQLError = &SSQLError;

	Int32 retcode = SQL_SUCCESS;
	Int32 total_conds = 0;

	Int32 sqlcode  = 0;
	char sqlState[MAX_SQLSTATE_LEN+1];
	Int32 row_number;
	char MessageText[MAX_MSG_TEXT_LEN+1];

	Int32 no_of_cond_items;
	int i;

	SQLDESC_ID	*pDesc;
	SQLSTMT_ID	*pStmt;
	if (pSrvrStmt != NULL )
	{
		pDesc = &pSrvrStmt->inputDesc;
		pStmt = &pSrvrStmt->stmt;
	}

	retcode =  WSQL_EXEC_GetDiagnosticsStmtInfo2(NULL,
											SQLDIAG_NUMBER,
																(Int64*)&total_conds, NULL, 0, NULL);
    if (total_conds == 0)
	{
		kdsCreateSQLErrorException(bSQLMessageSet,SQLError, 1);
		kdsCopySQLErrorException(SQLError, "No error message in SQL diagnostics area, but sqlcode is non-zero", retcode, "");
	}
	else
	{
// we need following ids: SQLCODE, SQLSTATE, ROW_NUMBER, MESSAGE TEXT

		kdsCreateSQLErrorException(bSQLMessageSet, SQLError, total_conds);

		no_of_cond_items = 4 * total_conds;

		gbuf_len = no_of_cond_items * sizeof(SQLDIAG_COND_INFO_ITEM_VALUE) + 
					total_conds * sizeof(Int32) +
					total_conds * sizeof(Int32) +
					total_conds * sizeof(Int32) +
					total_conds * sizeof(Int32) +
					total_conds * (MAX_SQLSTATE_LEN + 1) + 
					total_conds * (MAX_MSG_TEXT_LEN + 1);

		gbuf_ptr = new char[gbuf_len];
		if (gbuf_ptr == NULL)
		{
			kdsCopySQLErrorException(SQLError, "No Memory Error : From GETNOTATOMICROWSET2",	999, "");
			retcode = 999;
			goto bailout;
		}

		pCondInfoItems = (SQLDIAG_COND_INFO_ITEM_VALUE *)gbuf_ptr;

		pSqlcode = (Int32*)(pCondInfoItems + no_of_cond_items);
		pRowNumber = pSqlcode + total_conds;
		pSqlstateLen = pRowNumber + total_conds;
		pMessageTextLen = pSqlstateLen + total_conds;
		pSqlstate = (char*)(pMessageTextLen + total_conds);
		pMessageText = pSqlstate + total_conds * (MAX_SQLSTATE_LEN + 1);
		
		for (i=0; i < total_conds; i++)
		{
			pCondInfoItems[4*i+0].item_id_and_cond_number.item_id = SQLDIAG_SQLCODE;
			pCondInfoItems[4*i+1].item_id_and_cond_number.item_id = SQLDIAG_RET_SQLSTATE;
			pCondInfoItems[4*i+2].item_id_and_cond_number.item_id = SQLDIAG_ROW_NUMBER;
			pCondInfoItems[4*i+3].item_id_and_cond_number.item_id = SQLDIAG_MSG_TEXT;
			//
			pCondInfoItems[4*i+0].item_id_and_cond_number.cond_number_desc_entry = i+1;
			pCondInfoItems[4*i+1].item_id_and_cond_number.cond_number_desc_entry = i+1;
			pCondInfoItems[4*i+2].item_id_and_cond_number.cond_number_desc_entry = i+1;
			pCondInfoItems[4*i+3].item_id_and_cond_number.cond_number_desc_entry = i+1;
			//SQLCODE
			pCondInfoItems[4*i+0].string_val = 0;
			pCondInfoItems[4*i+0].num_val_or_len = pSqlcode + i;
			//SQLSTATE
			pCondInfoItems[4*i+1].string_val = pSqlstate + i * (MAX_SQLSTATE_LEN+1);
			pSqlstateLen[i] = MAX_SQLSTATE_LEN;
			pCondInfoItems[4*i+1].num_val_or_len = &pSqlstateLen[i];
			//ROW_NUMBER
			pCondInfoItems[4*i+2].string_val = 0;
			pCondInfoItems[4*i+2].num_val_or_len = pRowNumber + i;
			//MESSAGE TEXT
			pCondInfoItems[4*i+3].string_val = pMessageText + i * (MAX_MSG_TEXT_LEN+1);
			pMessageTextLen[i] = MAX_MSG_TEXT_LEN;
			pCondInfoItems[4*i+3].num_val_or_len = &pMessageTextLen[i];
		}
		retcode = SQL_EXEC_GetDiagnosticsCondInfo3(no_of_cond_items, pCondInfoItems);

		if (retcode < SQL_SUCCESS)
		{
			kdsCopySQLErrorException(SQLError, "Internal Error : From GetDiagnosticsCondInfo3",	retcode, "");
			goto bailout;
		}

		sqlState[MAX_SQLSTATE_LEN]=0;
		MessageText[MAX_MSG_TEXT_LEN]=0;

		// Added for MODE_SPECIAL_1 behavior
		bool doErr;
		if (srvrGlobal->modeSpecial_1)
		{
			pSrvrStmt->numErrRows = 0;
			doErr = false;

			markNewOperator,pSrvrStmt->errRowsArray = new BYTE[ pSrvrStmt->maxRowsetSize ];

			if( !pSrvrStmt->errRowsArray ) {
				SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
							srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
							srvrGlobal->srvrObjRef, 1, "GETNOTATOMICROWSET2");
				exit(0);
			}
			memset( pSrvrStmt->errRowsArray, 0x0, pSrvrStmt->maxRowsetSize );
		}

		for (i=0; i < total_conds; i++)
		{
			sqlcode = (Int32)*(pCondInfoItems[4*i+0].num_val_or_len);
			strcpy(sqlState,pCondInfoItems[4*i+1].string_val);
			row_number = (Int32)*(pCondInfoItems[4*i+2].num_val_or_len);
			strcpy(MessageText,pCondInfoItems[4*i+3].string_val);
			
			if (srvrGlobal->modeSpecial_1)
				doErr = true;
			
			//for NAR surrogate key feature
			if (sqlcode == -8108 && pSrvrStmt != NULL)
			{
				retcode = RECOVERY_FOR_SURROGATE_ERROR2(pSrvrStmt,pStmt,pDesc,row_number);
				if (retcode != SQL_SUCCESS)
							kdsCopySQLErrorExceptionAndRowCount(
							SQLError, 
							MessageText,
							sqlcode,
							sqlState,
							row_number + 1);
				else
				if (srvrGlobal->modeSpecial_1 && retcode == SQL_SUCCESS)
					doErr = false;
			}
			else 
			//for NAR surrogate key feature
			
			kdsCopySQLErrorExceptionAndRowCount(
							SQLError, 
							MessageText,
							sqlcode,
							sqlState,
							row_number + 1);

			// Added for MODE_SPECIAL_1 behavior
			// Increment pSrvrStmt->numErrRows only for the first occurrence of the
			// current row_number. The row_number in the SQL diags area is 0-based.
			if( srvrGlobal->modeSpecial_1 && doErr && row_number >= 0 ) 
			{
				if( pSrvrStmt->errRowsArray[ row_number ] == 0x0 ) {
					pSrvrStmt->errRowsArray[ row_number ] = 0x1;
					pSrvrStmt->numErrRows++;
				}
			}

		}
	}
bailout:
	WSQL_EXEC_ClearDiagnostics(NULL);
	sqlWarning->_length = SQLError->errorList._length;
	sqlWarning->_buffer = SQLError->errorList._buffer;
	if (gbuf_ptr != NULL)
		delete gbuf_ptr;
	return retcode;
}
//================================================================================

BYTE *pGlobalBuffer = NULL;
Int32 GlobalBufferLen = 0;

extern "C" Int32 getGlobalBufferLength()
  {
  return GlobalBufferLen;
  }  // end getGlobalBufferLength

//---------------------------------------------------------------

extern "C" BYTE* getGlobalBuffer()
  {
  return pGlobalBuffer;
  }  // end getGlobalBufferLength

//---------------------------------------------------------------

extern "C" BYTE* allocGlobalBuffer(Int32 size)
{
	SRVRTRACE_ENTER(FILE_INTF+27);

	if (size > GlobalBufferLen)
	{
		if (pGlobalBuffer != NULL)
		{
			delete pGlobalBuffer;
			pGlobalBuffer = NULL;
			GlobalBufferLen = 0;
		}
		if ((pGlobalBuffer = new (std::nothrow)BYTE[size]) != NULL)
			GlobalBufferLen = size;
	}
	SRVRTRACE_EXIT(FILE_INTF+27);
	return pGlobalBuffer;
}

extern "C" void releaseGlobalBuffer()
{
	SRVRTRACE_ENTER(FILE_INTF+28);
	if (pGlobalBuffer != NULL)
	{
		delete pGlobalBuffer;
		pGlobalBuffer = NULL;
		GlobalBufferLen = 0;
	}
	SRVRTRACE_EXIT(FILE_INTF+28);
	return;
}
//LCOV_EXCL_START
// Should be only called from EXECUTE2withRowsets thru GETNOTATOMICROWSET2 functions
SQLRETURN SRVR::RECOVERY_FOR_SURROGATE_ERROR2(SRVR_STMT_HDL* pSrvrStmt,		
		SQLSTMT_ID	*pStmt,
		SQLDESC_ID	*pDesc,
		Int32 currRowcnt)

{

	short *TypePtr;
	struct SQLCLI_QUAD_FIELDS *QuadPtr_original;
	struct SQLCLI_QUAD_FIELDS *QuadPtr;
	short *curTypePtr;     
	struct SQLCLI_QUAD_FIELDS *curQuadPtr;

	SQLRETURN retcode;
	odbc_SQLSvc_SQLError SQLError;
	Int32 paramCount,curParamNo;
	Int32 sqlRowCount = 0;
	
	Int32 currentRowsetSize = 0;
	Int32 DataType;
	Int32 DataLen;
	Int32 sqlMaxLength;
	Int32 currentRowCount = 0;
	Int32 RowsetSize;
	
	paramCount = pSrvrStmt->paramCount;
	currentRowCount = currRowcnt;
	currentRowsetSize = RowsetSize = 1;
	
//	TypePtr = (short*)pSrvrStmt->inputDescVarBuffer;
//	Int32 quad_length = sizeof(SQLCLI_QUAD_FIELDS) * (paramCount + 1);
//	QuadPtr_original = (struct SQLCLI_QUAD_FIELDS *)(pSrvrStmt->inputDescVarBuffer + sizeof(short) * paramCount);
//	QuadPtr = QuadPtr_original + paramCount + 1;

	Int32 quad_length       = sizeof(SQLCLI_QUAD_FIELDS) * paramCount;
	QuadPtr_original  = pSrvrStmt->inputQuadList;
	QuadPtr           = pSrvrStmt->inputQuadList_recover;

	memcpy(QuadPtr, QuadPtr_original, quad_length);


	retcode = GETSQLERROR_AND_ROWCOUNT(pSrvrStmt->bSQLMessageSet,&SQLError,1,currentRowCount,&sqlRowCount);	

	QuadPtr->var_ptr = (void*)&currentRowsetSize;
			
    for (curParamNo = 1 ; curParamNo < paramCount ; curParamNo++)
      {
      curQuadPtr = QuadPtr + curParamNo;
      DataType   = pSrvrStmt->IPD[curParamNo - 1].dataType;
      DataLen    = curQuadPtr->var_layout;

      switch (DataType)
	{
	case SQLTYPECODE_VARCHAR_WITH_LENGTH:
	case SQLTYPECODE_VARCHAR_LONG:
	case SQLTYPECODE_BITVAR:
	case SQLTYPECODE_BLOB:
	case SQLTYPECODE_CLOB:
             sqlMaxLength = DataLen + 2;
	     sqlMaxLength = ((sqlMaxLength + 2 - 1)>>1)<<1;
	     break;
	default:
	    sqlMaxLength = DataLen;
	    break;
	}  // end switch

      curQuadPtr->var_ptr = (BYTE*)curQuadPtr->var_ptr + (sqlMaxLength * (currentRowCount));

      if (curQuadPtr->ind_ptr != NULL)
        curQuadPtr->ind_ptr = (BYTE*)curQuadPtr->ind_ptr + (currentRowCount * sizeof(short));
      }  // end for

    retcode = WSQL_EXEC_SETROWSETDESCPOINTERS( pDesc
					     , 1
					     , NULL
					     , 1
					     , paramCount
					     , QuadPtr
                                             );
    if (retcode < SQL_SUCCESS)
      {
      GETSQLERROR_AND_ROWCOUNT(pSrvrStmt->bSQLMessageSet, &SQLError, 1 , currentRowCount, &sqlRowCount);
      ADDSQLERROR_TO_LIST( pSrvrStmt, &SQLError, currentRowCount + 1);
      return SQL_ERROR;
      }
    retcode = WSQL_EXEC_ExecFetch(pStmt, pDesc, 0);

	if (retcode == SQL_SUCCESS) pSrvrStmt->rowsAffected++ ;

	// Added for fix to SQL returning sqlcode=SQL_NO_DATA_FOUND for non-select
	// stmts when no rows get affected - 10/03/06
	if (retcode == SQL_NO_DATA_FOUND)
		retcode = SQL_SUCCESS;

	return retcode;

}
//for Surrogate key NAR
//LCOV_EXCL_STOP

//for setting in the indicator and Varpointers.

SQLRETURN SRVR::SetIndandVarPtr(SQLDESC_ID *pDesc,
						bool &bRWRS, 
						Int32 numEntries,
						BYTE *&SQLDesc,
						BYTE *&varBuffer,
						Int32 &totalMemLen,
						SRVR_DESC_HDL *&implDesc,
						DESC_HDL_LISTSTMT *&SqlDescInfo)
{

  	BYTE *IndPtr;
	BYTE *memPtr;
	BYTE *VarPtr;

	Int32 retcode = SQL_SUCCESS;
	BOOL sqlWarning = FALSE;


	Int32 memOffSet = 0;
	int i;

	memPtr = varBuffer ;
	memOffSet = 0;

	*(Int32 *)SQLDesc = totalMemLen;
  
	for (i = 0 ; i < numEntries ; i++)
	{
		if (bRWRS)
		{
			if (SqlDescInfo[i].Nullable)
			{
				memOffSet = ((memOffSet + 2 - 1) >> 1) << 1; 
				IndPtr = memPtr + memOffSet;
				*(Int32 *)(SQLDesc+SqlDescInfo[i].IndBuf) = (Int32)(IndPtr-memPtr);
				memOffSet += 2 ;
			}
			else
			{
				IndPtr = NULL;
				*(Int32 *)(SQLDesc+SqlDescInfo[i].IndBuf) = (Int32)(-1);
			}
		}
		switch (SqlDescInfo[i].DataType)
		{
		case SQLTYPECODE_CHAR:
		case SQLTYPECODE_VARCHAR:
                case SQLTYPECODE_BINARY:
			VarPtr = memPtr + memOffSet;					
			memOffSet += SqlDescInfo[i].Length;
			if (!bRWRS)
				memOffSet += 1;
			break;
		case SQLTYPECODE_VARCHAR_WITH_LENGTH:
		case SQLTYPECODE_BLOB:
		case SQLTYPECODE_CLOB:
                case SQLTYPECODE_VARBINARY:
			if( SqlDescInfo[i].Length > SHRT_MAX )
			{
				memOffSet = ((memOffSet + 4 - 1) >> 2) << 2; 
				VarPtr = memPtr + memOffSet;
				memOffSet += SqlDescInfo[i].Length + 4;
			}
			else
			{
				memOffSet = ((memOffSet + 2 - 1) >> 1) << 1; 
				VarPtr = memPtr + memOffSet;
				memOffSet += SqlDescInfo[i].Length + 2;
			}
			if (!bRWRS)
				memOffSet += 1;
			break;
		case SQLTYPECODE_VARCHAR_LONG:
			memOffSet = ((memOffSet + 2 - 1) >> 1) << 1; 
			VarPtr = memPtr + memOffSet;
			memOffSet += SqlDescInfo[i].Length + 2;
			if (!bRWRS)
				memOffSet += 1;
			break;
                case SQLTYPECODE_BOOLEAN:
                case SQLTYPECODE_TINYINT:
                case SQLTYPECODE_TINYINT_UNSIGNED:
			VarPtr = memPtr + memOffSet;
			memOffSet += SqlDescInfo[i].Length;
			break;
		case SQLTYPECODE_SMALLINT:
		case SQLTYPECODE_SMALLINT_UNSIGNED:
			memOffSet = ((memOffSet + 2 - 1) >> 1) << 1; 
			VarPtr = memPtr + memOffSet;
			memOffSet += SqlDescInfo[i].Length;
			break;
		case SQLTYPECODE_INTEGER:
		case SQLTYPECODE_INTEGER_UNSIGNED:
		//case SQLTYPECODE_IEEE_REAL:
			memOffSet = ((memOffSet + 4 - 1) >> 2) << 2; 
			VarPtr = memPtr + memOffSet;
			memOffSet += SqlDescInfo[i].Length;
			break;
		case SQLTYPECODE_LARGEINT:
		case SQLTYPECODE_LARGEINT_UNSIGNED:
		case SQLTYPECODE_IEEE_REAL:
		case SQLTYPECODE_IEEE_FLOAT:
		case SQLTYPECODE_IEEE_DOUBLE:
			memOffSet = ((memOffSet + 8 - 1) >> 3) << 3; 
			VarPtr = memPtr + memOffSet;
			memOffSet += SqlDescInfo[i].Length;
			break;
		case SQLTYPECODE_DECIMAL_UNSIGNED:
		case SQLTYPECODE_DECIMAL:
		case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
		case SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
		case SQLTYPECODE_INTERVAL:		// Treating as CHAR
		case SQLTYPECODE_DATETIME:
			VarPtr = memPtr + memOffSet;
			memOffSet += SqlDescInfo[i].Length;
			break;
		default:
			memOffSet = ((memOffSet + 8 - 1) >> 3) << 3; 
			VarPtr = memPtr + memOffSet;
			memOffSet += SqlDescInfo[i].Length;
			break;
		}
		*(Int32 *)(SQLDesc+SqlDescInfo[i].VarBuf) = (Int32)(VarPtr-memPtr);

		if (!bRWRS)
		{
			if (SqlDescInfo[i].Nullable)
			{
				memOffSet = ((memOffSet + 2 - 1) >> 1) << 1; 
				IndPtr = memPtr + memOffSet;
				*(Int32 *)(SQLDesc+SqlDescInfo[i].IndBuf) = (Int32)(IndPtr-memPtr);
				memOffSet += 2 ;
			}
			else
			{
				IndPtr = NULL;
				*(Int32 *)(SQLDesc+SqlDescInfo[i].IndBuf) = (Int32)(-1);
			}
		}

		retcode = WSQL_EXEC_SetDescItem(pDesc,
											i+1,
											SQLDESC_VAR_PTR,
											(long)VarPtr,
											0);
		HANDLE_ERROR(retcode, sqlWarning);
		
		retcode = WSQL_EXEC_SetDescItem(pDesc,
										i+1,
										SQLDESC_IND_PTR,
										(long)IndPtr,
										0);
		HANDLE_ERROR(retcode, sqlWarning);

		implDesc[i].varPtr = VarPtr;
		implDesc[i].indPtr = IndPtr;

		if (memOffSet > totalMemLen)
		{
			SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
					1, "memOffSet > totalMemLen in SetIndandVarPtr");
			return PROGRAM_ERROR;
		}
	
	}
}
//for setting in the indicator and Varpointers.

SQLRETURN SRVR::REALLOCSQLMXHDLS(SRVR_STMT_HDL* pSrvrStmt)
{
	Int32 retcode = SQL_SUCCESS;
	SQLSTMT_ID	*pStmt = &pSrvrStmt->stmt;
	SQLMODULE_ID *pModule = &pSrvrStmt->moduleId;
	BOOL		sqlWarning;

	if (! pSrvrStmt->isClosed)
	{
		retcode = WSQL_EXEC_CloseStmt(pStmt);
		pSrvrStmt->isClosed = TRUE;
	}
	if (pSrvrStmt->moduleName[0] == '\0')
	{
		retcode = WSQL_EXEC_DeallocStmt(pStmt);
	}

	pStmt->version = SQLCLI_ODBC_VERSION;
	pStmt->module = pModule;
	pStmt->handle = 0;
	pStmt->charset = SQLCHARSETSTRING_UTF8;
	if (pSrvrStmt->stmtName[0] != '\0')
	{
		pStmt->name_mode = stmt_name;
		pStmt->identifier_len = pSrvrStmt->stmtNameLen;
		pStmt->identifier = pSrvrStmt->stmtName;
	}
	else
	{
		pStmt->name_mode = stmt_handle;
		pStmt->identifier_len = 0;
		pStmt->identifier = NULL;
	}

	if (pModule->module_name == NULL)
	{
	        if (pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
		  {
                  retcode = WSQL_EXEC_AllocStmtForRS(pSrvrStmt->callStmtId, pSrvrStmt->resultSetIndex, pStmt);
                  HANDLE_ERROR2(retcode, sqlWarning);
		  }
                else
		  {
		  retcode = WSQL_EXEC_AllocStmt(pStmt,(SQLSTMT_ID *)NULL);
		  HANDLE_ERROR(retcode, sqlWarning);
		  }
	}
	
	return retcode;
}
//
//====================================================
//

