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
// MODULE: SrvrKds.cpp
//
// PURPOSE: Implements the functions to populate the output structures
//     
//
//

#include <platform_ndcs.h>
#include <sql.h>
#include <sqlext.h>
#include "glu.h"
#include "odbcCommon.h"
#include "odbc_sv.h"
#include "sqlcli.h"
#include "DrvrSrvr.h"
#include "srvrfunctions.h"
#include "srvrcommon.h"
#include "tdm_odbcSrvrMsg.h"
#include "CommonDiags.h"
#include "srvrkds.h"

using namespace SRVR;

void SRVR::kdsCreateSQLDescSeq(SQLItemDescList_def *SQLDesc, short numEntries)
{
	SRVRTRACE_ENTER(FILE_KDS+1);
	if (numEntries == 0)
	{
		SQLDesc->_length = 0;
		SQLDesc->_buffer = 0;
	}
	else
	{
		markNewOperator,SQLDesc->_buffer = new SQLItemDesc_def[numEntries];
		if (SQLDesc->_buffer == NULL)
		{
			// Handle Memory Overflow execption here
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
						srvrGlobal->srvrObjRef, 1, "KdsCreateSQLDescSeq");
			exit(0);
		}
		SQLDesc->_length = 0;
	}
	SRVRTRACE_EXIT(FILE_KDS+1);
	return;
}

void SRVR::kdsCreateEmptySQLDescSeq(SQLItemDescList_def *SQLDesc)
{
	SRVRTRACE_ENTER(FILE_KDS+2);
	SQLDesc->_length = 0;
	SQLDesc->_buffer = NULL;
	SRVRTRACE_EXIT(FILE_KDS+2);
}

void SRVR::kdsCopyToSQLDescSeq(SQLItemDescList_def *SQLDesc, 
				  char *DescName,
				  long DataType,  
				  long DateTimeCode, long Length, 
				  long Precision, long Scale,
				  long Nullable, long ODBCDataType, 
				  long ODBCPrecision, BOOL SignType,
				  long SQLCharset, long ODBCCharset,
				  char* TableName,
				  char* CatalogName,
				  char* SchemaName,
				  char* Heading,
				  long intLeadPrec,
				  long paramMode)
{
	SRVRTRACE_ENTER(FILE_KDS+3);
	SQLItemDesc_def *SQLItemDesc;

	SQLItemDesc = (SQLItemDesc_def *)SQLDesc->_buffer + SQLDesc->_length;
	SQLItemDesc->version = 0; 
	SQLItemDesc->dataType = DataType;
	SQLItemDesc->datetimeCode = DateTimeCode;
	SQLItemDesc->maxLen = Length;
	SQLItemDesc->precision = (short)Precision;
	SQLItemDesc->scale = (short)Scale;
	SQLItemDesc->nullInfo = (BOOL)Nullable;
	SQLItemDesc->signType = SignType;

	SQLItemDesc->ODBCDataType = ODBCDataType;
	SQLItemDesc->ODBCPrecision = (short)ODBCPrecision;
	SQLItemDesc->SQLCharset = SQLCharset;
	SQLItemDesc->ODBCCharset = ODBCCharset;

	strncpy(SQLItemDesc->colHeadingNm, DescName, sizeof(SQLItemDesc->colHeadingNm));
	SQLItemDesc->colHeadingNm[sizeof(SQLItemDesc->colHeadingNm)-1] = 0;
	strcpy(SQLItemDesc->TableName, TableName);
	strcpy(SQLItemDesc->CatalogName, CatalogName);
	strcpy(SQLItemDesc->SchemaName, SchemaName);
	strcpy(SQLItemDesc->Heading, Heading);
	SQLItemDesc->intLeadPrec = intLeadPrec;
	SQLItemDesc->paramMode = paramMode;
	SQLDesc->_length++;
	SRVRTRACE_EXIT(FILE_KDS+3);
	return;
}

void SRVR::kdsCreateSQLErrorException(
								bool& bSQLMessageSet,
								odbc_SQLSvc_SQLError *SQLError, 
								long numConditions)
{
	SRVRTRACE_ENTER(FILE_KDS+4);
	if (numConditions == 0)
	{
		SQLError->errorList._buffer = NULL;
		SQLError->errorList._length = 0;
	}
	else
	{
		bSQLMessageSet = true;
		markNewOperator,SQLError->errorList._buffer = new ERROR_DESC_def[numConditions];
		if (SQLError->errorList._buffer == NULL)
		{
			// Handle Memory Overflow execption here
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
						srvrGlobal->srvrObjRef, 1, "KdsCreateSQLErrorException");
			exit(0);
		}
		
		SQLError->errorList._length = 0;		
	}
	SRVRTRACE_EXIT(FILE_KDS+4);
	return;
}

void SRVR::kdsCopySQLErrorException(
							odbc_SQLSvc_SQLError *SQLError, 
							char *msg_buf,
							long sqlcode,
							char *sqlState)
{
	SRVRTRACE_ENTER(FILE_KDS+5);
	ERROR_DESC_def *SqlErrorDesc;
	size_t	len;

	len = strlen(msg_buf);
	// Allocate String Buffer
	
	SqlErrorDesc = (ERROR_DESC_def *)SQLError->errorList._buffer + SQLError->errorList._length;
	markNewOperator,SqlErrorDesc->errorText = new char[len+1];
	if (SqlErrorDesc->errorText == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "KdsAddSQLErrorException");
		exit(0);
	}
	strcpy(SqlErrorDesc->errorText, msg_buf);
	SqlErrorDesc->sqlcode = sqlcode;
	strncpy(SqlErrorDesc->sqlstate, sqlState, sizeof(SqlErrorDesc->sqlstate));
	SqlErrorDesc->sqlstate[sizeof(SqlErrorDesc->sqlstate)-1] = 0;
	SqlErrorDesc->errorCodeType = SQLERRWARN;
	SqlErrorDesc->Param1 = NULL;
	SqlErrorDesc->Param2 = NULL;
	SqlErrorDesc->Param3 = NULL;
	SqlErrorDesc->Param4 = NULL;
	SqlErrorDesc->Param5 = NULL;
	SqlErrorDesc->Param6 = NULL;
	SqlErrorDesc->Param7 = NULL;
	SqlErrorDesc->rowId = 0;
	SqlErrorDesc->errorDiagnosticId = 0;
	SqlErrorDesc->operationAbortId = 0;
	SQLError->errorList._length++;

	SRVRTRACE_EXIT(FILE_KDS+5);
	return;
}

void SRVR::kdsCopySQLErrorExceptionAndRowCount(
							odbc_SQLSvc_SQLError *SQLError, 
							char *msg_buf,
							long sqlcode,
							char *sqlState,
							long currentRowCount)
{
	SRVRTRACE_ENTER(FILE_KDS+6);
	ERROR_DESC_def *SqlErrorDesc;
	size_t	len;

	len = strlen(msg_buf);
	// Allocate String Buffer
	
	SqlErrorDesc = (ERROR_DESC_def *)SQLError->errorList._buffer + SQLError->errorList._length;
	markNewOperator,SqlErrorDesc->errorText = new char[len+1];
	if (SqlErrorDesc->errorText == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "KdsAddSQLErrorException");
		exit(0);
	}
	strcpy(SqlErrorDesc->errorText, msg_buf);
	SqlErrorDesc->sqlcode = sqlcode;
	strcpy(SqlErrorDesc->sqlstate, sqlState);
	SqlErrorDesc->errorCodeType = SQLERRWARN;
	SqlErrorDesc->Param1 = NULL;
	SqlErrorDesc->Param2 = NULL;
	SqlErrorDesc->Param3 = NULL;
	SqlErrorDesc->Param4 = NULL;
	SqlErrorDesc->Param5 = NULL;
	SqlErrorDesc->Param6 = NULL;
	SqlErrorDesc->Param7 = NULL;
	SqlErrorDesc->rowId = currentRowCount;
	SqlErrorDesc->errorDiagnosticId = 0;
	SqlErrorDesc->operationAbortId = 1;
	SQLError->errorList._length++;

	SRVRTRACE_EXIT(FILE_KDS+6);
	return;
}

void SRVR::kdsCopyToSQLValueSeq(SQLValueList_def *SQLValueList,
						 IDL_long dataType, 
						 IDL_short indValue, 
						 void *varPtr, 
						 long allocLength,
						 long Charset)
{
	SRVRTRACE_ENTER(FILE_KDS+7);
	SQLValue_def *SQLValue;
	SQLValue = (SQLValue_def *)SQLValueList->_buffer + SQLValueList->_length;
	SQLValue->dataInd = indValue;
	SQLValue->dataType = dataType;
	SQLValue->dataCharset = Charset;
	if (indValue == 0)
	{
		SQLValue->dataValue._length = allocLength;
		memcpy(SQLValue->dataValue._buffer, varPtr, allocLength);

	}
	else
	{
		SQLValue->dataValue._length = 0;
		// Do not initialize _buffer value, since this should always point to var buffer
		// Should not be problem with Krypton, since Krypton uses _length field
		// to determine if this buffer need to be shipped across
		// SQLValue->dataValue._buffer = NULL;
	}
	SQLValueList->_length++;
	SRVRTRACE_EXIT(FILE_KDS+7);
	return;
}

void SRVR::kdsCopyToSQLDataSeq(SQL_DataValue_def *outputDataValue,
							 IDL_long dataType, 
							 IDL_short indValue, 
							 void *varPtr, 
							 long allocLength,
							 long Charset)
{
	SRVRTRACE_ENTER(FILE_KDS+8);
	long size;

	size = outputDataValue->_length;
	
	if (indValue == 0)
	{
		outputDataValue->_length += (allocLength+1);
		memcpy(outputDataValue->_buffer+size, "\0", 1);
		memcpy(outputDataValue->_buffer+size+1, varPtr, allocLength);

	}
	else
	{
		outputDataValue->_length += 1;
		memcpy(outputDataValue->_buffer+size, "\1", 1);
	}
	SRVRTRACE_EXIT(FILE_KDS+8);
}

long SRVR::kdsCopyToSMDSQLValueSeq(SQLValueList_def *SQLValueList,
							 IDL_long dataType, 
							 IDL_short indValue, 
							 char *dataValue, 
							 long allocLength,
							 long Charset)
{
	SRVRTRACE_ENTER(FILE_KDS+9);
	SQLValue_def *SQLValue;
	short length;
	int iTmp;
	__int64 int64Tmp;
	short sTmp;
	double dTmp;
	
	SQLValue = (SQLValue_def *)SQLValueList->_buffer + SQLValueList->_length;
	SQLValue->dataInd = indValue;
	SQLValue->dataType = dataType;
	SQLValue->dataCharset = Charset;
	if (indValue == 0)
	{
		SQLValue->dataValue._length = allocLength;
		switch (dataType)
		{
			case SQLTYPECODE_CHAR:
			// case SQLTYPECODE_CHAR_UP: // need to comment it out since sqlci.h doesn't support anymore.
				length = strlen(dataValue);
				if (length > (allocLength-1))
					return DATA_ERROR;
				memcpy(SQLValue->dataValue._buffer, dataValue, length);
				memset((SQLValue->dataValue._buffer+length), ' ', allocLength-length);
				SQLValue->dataValue._buffer[allocLength-1] = '\0';
				break;
			case SQLTYPECODE_VARCHAR:
				length = strlen(dataValue);
				if (length > (allocLength-1))
					return DATA_ERROR;
				memcpy(SQLValue->dataValue._buffer, dataValue, length);
				SQLValue->dataValue._buffer[length] = '\0';
				break;
			case SQLTYPECODE_VARCHAR_WITH_LENGTH:
			case SQLTYPECODE_INTERVAL:
			case SQLTYPECODE_BLOB:
			case SQLTYPECODE_CLOB:
			// case SQLTYPECODE_VARCHAR_UP: //             = -1201 // need to comment it out since sqlci.h doesn't support anymore.
				length = strlen(dataValue);
				if (length > allocLength-(1+2)) // 2 Bytes for Length
					return DATA_ERROR;
				memcpy(SQLValue->dataValue._buffer, (void *)&length, sizeof(length));
				memcpy(SQLValue->dataValue._buffer+sizeof(length), dataValue, length);
				SQLValue->dataValue._buffer[length+2] = '\0';
				break;
			case SQLTYPECODE_SMALLINT: //5
				sTmp = (short) atoi(dataValue);
				*((SWORD *) SQLValue->dataValue._buffer)= (SWORD)sTmp;
				break;
			case SQLTYPECODE_INTEGER: // 4
				iTmp = atoi(dataValue);
				*((SDWORD *) SQLValue->dataValue._buffer)= (SDWORD)iTmp;
				break;
			case SQLTYPECODE_LARGEINT: // -402
				int64Tmp = _atoi64(dataValue);
				*((__int64 *) SQLValue->dataValue._buffer)= int64Tmp;
				break;
			case SQLTYPECODE_DATETIME: // 9
				{
					TIMESTAMP_TYPES *timstamp = (TIMESTAMP_TYPES *)SQLValue->dataValue._buffer;
					char    *token;
					char	*saveptr;
					UDWORD fraction;
					long   fraction_part;
					short datetime_parts[8];
					short i;
					
					for(i=0;i<8;i++)
						datetime_parts[i] = 0;

					token = strtok_r(dataValue, " :-.", &saveptr) ;
					
					for (i =0; token != NULL && i < 6 ; i++)
					{
					   datetime_parts[i] = (short)atoi(token);
					   token = strtok_r(NULL, " :-.", &saveptr);
					}
					if (token != NULL)
					{
					   fraction_part = atol(token);
					   datetime_parts[6] = (short)(fraction_part / 1000);
					   datetime_parts[7] = (short)(fraction_part % 1000);
					}
					timstamp->year=datetime_parts[0];
					timstamp->month=(unsigned char)datetime_parts[1];
					timstamp->day=(unsigned char)datetime_parts[2];
					timstamp->hour=(unsigned char)datetime_parts[3];
					timstamp->minute=(unsigned char)datetime_parts[4];
					timstamp->second=(unsigned char)datetime_parts[5];
					fraction=(UDWORD)(datetime_parts[6])*1000+datetime_parts[7];
					memcpy((void *)((TIMESTAMP_TYPES *)SQLValue->dataValue._buffer)->fraction, 
															&fraction, sizeof(UDWORD));
				}
				break;
			default:
				return DATA_TYPE_ERROR;
		}
	}
	else
		SQLValue->dataValue._length = 0;
	SQLValueList->_length++;
	SRVRTRACE_EXIT(FILE_KDS+9);
	return 0;
}


void SRVR::kdsCreateSQLWarningException(
				bool& bSQLMessageSet,
				ERROR_DESC_LIST_def *SQLWarning, 
				long numConditions)
{
	SRVRTRACE_ENTER(FILE_KDS+10);
	if (numConditions == 0)
	{
		SQLWarning->_buffer = NULL;
		SQLWarning->_length = 0;
	}
	else
	{
		bSQLMessageSet = true;
		markNewOperator,SQLWarning->_buffer = new ERROR_DESC_def[numConditions];
		if (SQLWarning->_buffer == NULL)
		{
			// Handle Memory Overflow execption here
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
						srvrGlobal->srvrObjRef, 1, "KdsCreateSQLWarningException");
			exit(0);
		}
		
		SQLWarning->_length = 0;		
	}
	SRVRTRACE_EXIT(FILE_KDS+10);
	return;
}

void SRVR::CopyRGMsg(
					ERROR_DESC_def *SqlErrorDesc, 
					RES_HIT_DESC_def *rgPolicyHit)
{
	SRVRTRACE_ENTER(FILE_KDS+11);

	size_t	len;
	const size_t len_longlong = 64;

	SqlErrorDesc->errorText = NULL;
	SqlErrorDesc->sqlcode = 0;
	strcpy(SqlErrorDesc->sqlstate, " ");
	SqlErrorDesc->rowId = 0;
	SqlErrorDesc->errorDiagnosticId = 0;
	SqlErrorDesc->operationAbortId = 0;

	len = strlen(rgPolicyHit->AttrNm);
	markNewOperator,SqlErrorDesc->Param1 = new char[len+1];	
	if (SqlErrorDesc->Param1 == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "CopyRGMsg");
		exit(0);
	}
	strcpy(SqlErrorDesc->Param1, rgPolicyHit->AttrNm);

	len = len_longlong;
	markNewOperator,SqlErrorDesc->Param2 = new char[len+1];
	if (SqlErrorDesc->Param2 == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "CopyRGMsg");
		exit(0);
	}
	
	sprintf(SqlErrorDesc->Param2, "%Ld", rgPolicyHit->Limit);

	len = strlen(rgPolicyHit->Action);
	markNewOperator,SqlErrorDesc->Param3 = new char[len+1];
	if (SqlErrorDesc->Param3 == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "CopyRGMsg");
		exit(0);
	}

	strcpy(SqlErrorDesc->Param3, rgPolicyHit->Action);

	len = len_longlong;
	markNewOperator,SqlErrorDesc->Param4 = new char[len+1];
	if (SqlErrorDesc->Param4 == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "CopyRGMsg");
		exit(0);
	}

	sprintf(SqlErrorDesc->Param4, "%Ld", rgPolicyHit->ActualValue);
	
	SqlErrorDesc->errorCodeType = ESTIMATEDCOSTRGERRWARN;
	SqlErrorDesc->Param5 = NULL;
	SqlErrorDesc->Param6 = NULL;
	SqlErrorDesc->Param7 = NULL;
	SRVRTRACE_EXIT(FILE_KDS+11);
}

void SRVR::kdsCopyRGWarningException(
		ERROR_DESC_LIST_def *SQLWarning, 
		RES_HIT_DESC_def *rgPolicyHit)
{
	SRVRTRACE_ENTER(FILE_KDS+12);
	ERROR_DESC_def *SqlErrorDesc;		
	
	SqlErrorDesc = (ERROR_DESC_def *)SQLWarning->_buffer + SQLWarning->_length;	
	CopyRGMsg(SqlErrorDesc, rgPolicyHit);
	SQLWarning->_length++;

	SRVRTRACE_EXIT(FILE_KDS+12);
	return;
}

void SRVR::kdsCopyRGErrorException(
		odbc_SQLSvc_SQLError *SQLError, 
		RES_HIT_DESC_def *rgPolicyHit)
{
	SRVRTRACE_ENTER(FILE_KDS+13);
	ERROR_DESC_def *SqlErrorDesc;
	
	SqlErrorDesc = (ERROR_DESC_def *)SQLError->errorList._buffer + SQLError->errorList._length;
	CopyRGMsg(SqlErrorDesc, rgPolicyHit);
	SQLError->errorList._length++;

	SRVRTRACE_EXIT(FILE_KDS+13);
	return;
}
