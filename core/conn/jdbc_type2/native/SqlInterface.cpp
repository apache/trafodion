/**************************************************************************
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
// MODULE: sqlInterface.cpp
//
// PURPOSE: Implements the Srvr interface to SQL
//
/*Change Log
 * Methods Changed: ClearDiags
 */
 /*Change Log
 * Methods Changed: Removed setOfCQD
 */

#include <platform_ndcs.h>
#include <sql.h>
#include <sqlext.h>
#include "SrvrCommon.h"
#include "SrvrKds.h"
#include "SqlInterface.h"
#include "CommonDiags.h"
#include "Debug.h"
#include "GlobalInformation.h"
#include <fcntl.h>
//Added for CQDs filter

#if defined(TAG64)
std::map<long,SRVR_STMT_HDL*> tempStmtIdMap;
#endif

#ifndef TODO	// Linux port ToDo
extern long SQLMXStatement_int_FAILED(void);
extern long SQLMXStatement_SUCCESS_NO_INFO(void);
#endif

extern int SPJRS;

#define HANDLE_ERROR(error_code, warning_flag) \
{\
	if (error_code != 0) \
{\
	if (error_code < 0) \
	CLI_DEBUG_RETURN_SQL(SQL_ERROR); \
	warning_flag = TRUE; \
}\
}

#define THREAD_RETURN(pSrvrStmt, return_code)                           \
{                                                                       \
    if (NULL != pSrvrStmt)                                              \
        pSrvrStmt->threadReturnCode = return_code;                      \
    if (NULL != pSrvrStmt && pSrvrStmt->threadReturnCode!=SQL_SUCCESS)  \
        pSrvrStmt->processThreadReturnCode();                           \
    CLI_DEBUG_RETURN_SQL(return_code);                                  \
}

#define HANDLE_THREAD_ERROR(error_code, warning_flag, pSrvrStmt) \
{\
	if (error_code != 0) \
{\
	if (error_code < 0) \
	THREAD_RETURN(pSrvrStmt,SQL_ERROR); \
	warning_flag = TRUE; \
}\
}

long AdjustCharLength(SRVR_STMT_HDL::DESC_TYPE descType, long SQLCharset, long Length)
{
	FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,"AdjustCharLength",("descType=%s, SQLCharset=%s, Length=%ld",
		CliDebugDescTypeStr(descType),
		getCharsetEncoding(SQLCharset),
		Length));

	long ret_length = Length;

	switch (SQLCharset)
	{
		// SBB: The doubling for Kanji and KSC5601 needs to be removed
		// upon incorporation of MXCMP Case 10-040224-9870 into next sut (UCS2 also ?)
	case SQLCHARSETCODE_KANJI:
	case SQLCHARSETCODE_KSC5601:
	case SQLCHARSETCODE_UCS2:
		ret_length *= 2;
		break;
	}

	FUNCTION_RETURN_NUMERIC(ret_length,(NULL));
}

long MapSQLCharsetToJDBC(long SQLCharset)
{
	FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE, "MapSQLCharsetToODBC",("SQLCharset=%s",
		getCharsetEncoding(SQLCharset)));

	long rc = SQLCHARSETCODE_ISO88591;

	//if locale is Japanses switch to SJIS (MBCS)
	if (PRIMARYLANGID(LANGIDFROMLCID(srvrGlobal->clientLCID)) == LANG_JAPANESE)
		rc = SQLCHARSETCODE_SJIS;

	// Unicode support
	switch (SQLCharset)
	{
	case SQLCHARSETCODE_ISO88591:
		rc = SQLCHARSETCODE_ISO88591;
		break;
	case SQLCHARSETCODE_KANJI:
		rc = SQLCHARSETCODE_KANJI;
		break;
	case SQLCHARSETCODE_KSC5601:
		rc = SQLCHARSETCODE_KSC5601;
		break;
	case SQLCHARSETCODE_SJIS:
		rc = SQLCHARSETCODE_SJIS;
		break;
	case SQLCHARSETCODE_UCS2:
		rc = SQLCHARSETCODE_UCS2;
		break;
	}

	FUNCTION_RETURN_NUMERIC(rc,("%s",getCharsetEncoding(rc)));
}

// * ***********************************************************************************
// *
// * This routine is only called by BuildSQLDesc, which is in this same file
// *
// * ***********************************************************************************

SQLRETURN GetJDBCValues( SQLItemDesc_def *SQLItemDesc,
						long &totalMemLen,
						char *ColHeading)
{
	FUNCTION_ENTRY("GetJDBCValues",
		("DataType=%s, DateTimeCode=%ld, Length=%ld, Precision=%ld, ODBCDataType=%ld, ODBCPrecision=%ld, SignType=%ld, Nullable=%ld, totalMemLen=%ld, SQLCharset=%ld, ODBCCharset=%ld, IntLeadPrec=%ld",
		CliDebugSqlTypeCode(SQLItemDesc->dataType),
		SQLItemDesc->datetimeCode,
		SQLItemDesc->maxLen,
		SQLItemDesc->precision,
		SQLItemDesc->ODBCDataType,
		SQLItemDesc->ODBCPrecision,
		SQLItemDesc->signType,
		SQLItemDesc->nullInfo,
		totalMemLen,
		SQLItemDesc->SQLCharset,
		SQLItemDesc->ODBCCharset,
		SQLItemDesc->intLeadPrec));

	SQLItemDesc->ODBCCharset = SQLCHARSETCODE_ISO88591;

	long memAlignOffset;
	//long allocSize; 64 bit change
	int allocSize;
	getMemoryAllocInfo(	SQLItemDesc->dataType,
		SQLItemDesc->SQLCharset,
		SQLItemDesc->maxLen,
		SQLItemDesc->vc_ind_length,
		totalMemLen,
		&memAlignOffset,
		&allocSize,
		NULL);

	switch (SQLItemDesc->dataType)
	{
	case SQLTYPECODE_CHAR:
		SQLItemDesc->ODBCPrecision = SQLItemDesc->maxLen;
		SQLItemDesc->ODBCDataType  = SQL_CHAR;
		SQLItemDesc->signType      = FALSE;
		SQLItemDesc->ODBCCharset   = MapSQLCharsetToJDBC(SQLItemDesc->SQLCharset);
		break;
	case SQLTYPECODE_VARCHAR:
	case SQLTYPECODE_VARCHAR_WITH_LENGTH:
		SQLItemDesc->ODBCPrecision = SQLItemDesc->maxLen;
		SQLItemDesc->ODBCDataType  = SQL_VARCHAR;
		SQLItemDesc->signType      = FALSE;
		SQLItemDesc->ODBCCharset   = MapSQLCharsetToJDBC(SQLItemDesc->SQLCharset);
		break;
	case SQLTYPECODE_VARCHAR_LONG:
		SQLItemDesc->ODBCPrecision = SQLItemDesc->maxLen;
		SQLItemDesc->ODBCDataType  = SQL_LONGVARCHAR;
		SQLItemDesc->signType      = FALSE;
		SQLItemDesc->ODBCCharset   = MapSQLCharsetToJDBC(SQLItemDesc->SQLCharset);
		break;
	case SQLTYPECODE_SMALLINT:
		SQLItemDesc->ODBCPrecision = 5;
		SQLItemDesc->ODBCDataType  = SQL_SMALLINT;
		SQLItemDesc->signType      = TRUE;
		break;
	case SQLTYPECODE_SMALLINT_UNSIGNED:
		SQLItemDesc->ODBCPrecision = 5;
		SQLItemDesc->ODBCDataType  = SQL_SMALLINT;
		SQLItemDesc->signType      = FALSE;
		break;
	case SQLTYPECODE_INTEGER:
		SQLItemDesc->ODBCPrecision = 10;
		SQLItemDesc->ODBCDataType  = SQL_INTEGER;
		SQLItemDesc->signType      = TRUE;
		break;
	case SQLTYPECODE_INTEGER_UNSIGNED:
		SQLItemDesc->ODBCPrecision = 10;
		SQLItemDesc->ODBCDataType  = SQL_INTEGER;
		SQLItemDesc->signType      = FALSE;
		break;
	case SQLTYPECODE_LARGEINT:
		SQLItemDesc->ODBCDataType  = SQL_BIGINT;  // The LARGEINT is mapped to Types.BIGINT unless CLOB or BLOB
		SQLItemDesc->ODBCPrecision = 19;          // Make sure that the precision is set correctly
		if (ColHeading[0] != '\0')
		{
			if (strstr(ColHeading, CLOB_HEADING) != NULL)
				SQLItemDesc->ODBCDataType = TYPE_CLOB;
			else if (strstr(ColHeading, BLOB_HEADING) != NULL)
				SQLItemDesc->ODBCDataType = TYPE_BLOB;
		}
		SQLItemDesc->signType = TRUE;
		break;
	case SQLTYPECODE_IEEE_REAL:
	case SQLTYPECODE_TDM_REAL:
		SQLItemDesc->ODBCDataType  = SQL_REAL;
		SQLItemDesc->ODBCPrecision = 7;
		SQLItemDesc->signType      = TRUE;
		break;
	case SQLTYPECODE_IEEE_DOUBLE:
	case SQLTYPECODE_TDM_DOUBLE:
		SQLItemDesc->ODBCDataType  = SQL_DOUBLE;
		SQLItemDesc->ODBCPrecision = 15;
		SQLItemDesc->signType      = TRUE;
		break;
	case SQLTYPECODE_DATETIME:
		switch (SQLItemDesc->datetimeCode)
		{
		case SQLDTCODE_DATE:
			SQLItemDesc->ODBCDataType = SQL_TYPE_DATE;
			break;
		case SQLDTCODE_TIME:
			SQLItemDesc->ODBCDataType = SQL_TYPE_TIME;
			break;
		case SQLDTCODE_TIMESTAMP:
			SQLItemDesc->ODBCDataType = SQL_TYPE_TIMESTAMP;
			break;
		default:
			SQLItemDesc->ODBCDataType = SQL_TYPE_TIMESTAMP;
			break;
		}
		SQLItemDesc->signType      = FALSE;
		SQLItemDesc->ODBCPrecision = SQLItemDesc->precision;
		getMemoryAllocInfo(	SQLItemDesc->dataType,
			SQLItemDesc->SQLCharset,
			SQLItemDesc->maxLen,
			SQLItemDesc->vc_ind_length,
			totalMemLen,
			&memAlignOffset,
			&allocSize,
			NULL);
		break;
	case SQLTYPECODE_NUMERIC:
		SQLItemDesc->ODBCDataType  = SQL_NUMERIC;
		SQLItemDesc->ODBCPrecision = SQLItemDesc->precision;
		SQLItemDesc->signType      = TRUE;
		break;
	case SQLTYPECODE_NUMERIC_UNSIGNED:
		SQLItemDesc->ODBCDataType  = SQL_NUMERIC;
		SQLItemDesc->ODBCPrecision = SQLItemDesc->precision;
		SQLItemDesc->signType      = FALSE;
		break;
	case SQLTYPECODE_DECIMAL_UNSIGNED:
		SQLItemDesc->ODBCPrecision = SQLItemDesc->maxLen;
		SQLItemDesc->ODBCDataType  = SQL_DECIMAL;
		SQLItemDesc->signType      = FALSE;
		break;
	case SQLTYPECODE_DECIMAL:
		SQLItemDesc->ODBCPrecision = SQLItemDesc->maxLen;
		SQLItemDesc->ODBCDataType  = SQL_DECIMAL;
		SQLItemDesc->signType      = TRUE;
		break;
	case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
		SQLItemDesc->ODBCDataType  = SQL_DOUBLE; // Since there is no corresponding ODBC DataType, Map it as a double
		SQLItemDesc->ODBCPrecision = 15;
		SQLItemDesc->signType      = FALSE;
		break;
	case SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
		SQLItemDesc->ODBCDataType  = SQL_DOUBLE; // Since there is no corresponding ODBC DataType, Map it as a double
		SQLItemDesc->ODBCPrecision = 15;
		SQLItemDesc->signType      = TRUE;
		break;
	case SQLTYPECODE_INTERVAL:		// Interval will be sent in ANSIVARCHAR format
		SQLItemDesc->ODBCDataType  = SQL_INTERVAL;
		SQLItemDesc->signType      = FALSE;
		SQLItemDesc->ODBCPrecision = SQLItemDesc->precision;
		// Calculate the length based on Precision and IntLeadPrec
		// The max. length is for Day to Fraction(6)
		// Day = IntLeadPrec + 1 ( 1 for Blank space)
		// Hour = 3 ( 2+1)
		// Minute = 3 (2+1)
		// Seconds = 3 (2+1)
		// Fraction = Precision
		/*SQLItemDesc->maxLen =  SQLItemDesc->intLeadPrec + 1 + 3 + 3 + 3 + SQLItemDesc->precision; //Anitha -- 10-060510-6400 */
		/* Soln No: 10-060510-6400
		* Desc:String overflow no longer occurs during evaluation
		*/
		SQLItemDesc->maxLen =  SQLItemDesc->intLeadPrec + 1 + 1 + 3 + 3 + 3 + SQLItemDesc->precision;

		getMemoryAllocInfo(	SQLItemDesc->dataType,
			SQLItemDesc->SQLCharset,
			SQLItemDesc->maxLen,
			SQLItemDesc->vc_ind_length,
			totalMemLen,
			&memAlignOffset,
			&allocSize,
			NULL);
		break;
	case SQLTYPECODE_CLOB:
		SQLItemDesc->ODBCDataType  = TYPE_CLOB;
		SQLItemDesc->signType      = FALSE;
		SQLItemDesc->ODBCPrecision = 0;
	        break;	
	case SQLTYPECODE_BLOB:
		SQLItemDesc->ODBCDataType  = TYPE_BLOB;
		SQLItemDesc->signType      = FALSE;
		SQLItemDesc->ODBCPrecision = 0;
		break;
	case SQLTYPECODE_BIT:
	case SQLTYPECODE_BITVAR:
	case SQLTYPECODE_IEEE_FLOAT:
	case SQLTYPECODE_FLOAT:
		// SQLTYPECODE_FLOAT is also considered an error, Since Trafodion will never
		// return SQLTYPECODE_FLOAT. It returns either SQLTYPECODE_IEEE_REAL or SQLTYPECODE_IEEE_DOUBLE
		// depending upon the precision for FLOAT fields
	case SQLTYPECODE_BPINT_UNSIGNED:
	default:
		SQLItemDesc->ODBCDataType  = SQL_TYPE_NULL;
		SQLItemDesc->ODBCPrecision = 0;
		SQLItemDesc->signType      = FALSE;
		break;
	}
	totalMemLen += memAlignOffset + allocSize;

	if (SQLItemDesc->nullInfo)
	{
		// 2-byte boundary
		totalMemLen = ((totalMemLen + 2 - 1) >> 1) << 1;
		totalMemLen += sizeof(short) ;
	}
	DEBUG_OUT(DEBUG_LEVEL_ENTRY,("Result - ODBCPrecision=%ld ODBCDataType=%ld SignType=%ld Length=%ld totalMemLen=%ld ODBCCharset=%ld",
		SQLItemDesc->ODBCPrecision,
		SQLItemDesc->ODBCDataType,
		SQLItemDesc->signType,
		SQLItemDesc->maxLen,
		totalMemLen,
		SQLItemDesc->ODBCCharset));
	CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN SET_DATA_PTR(SRVR_STMT_HDL *pSrvrStmt, SRVR_STMT_HDL::DESC_TYPE descType)
{
	FUNCTION_ENTRY("SET_DATA_PTR", ("pSrvrStmt=0x%08x, descType=%s",
		pSrvrStmt,
		CliDebugDescTypeStr(descType)));

	SQLDESC_ID *pDesc = pSrvrStmt->getDesc(descType);
	SQLItemDescList_def *SQLDesc = pSrvrStmt->getDescList(descType);
	long *totalMemLen = pSrvrStmt->getDescBufferLenPtr(descType);
	BYTE **varBuffer = pSrvrStmt->getDescVarBufferPtr(descType);
	int numEntries = pSrvrStmt->getDescEntryCount(descType);
	int rowsetSize = pSrvrStmt->getRowsetSize(descType);
	struct SQLCLI_QUAD_FIELDS *quadField = pSrvrStmt->getQuadField(descType);	// Linux port - ToDo SQLCLI_QUAD_FIELDS struct is different on SQ
	SRVR_DESC_HDL *implDesc = pSrvrStmt->getImplDesc(descType);


	SQLItemDesc_def *SQLItemDesc;
	long memOffset = 0, memAlignOffset,  varLayout;
	int allocSize; //64 change
	BYTE *VarPtr;
	BYTE *IndPtr;
	BYTE *memPtr;
	unsigned long i;
	int retcode;
	BOOL sqlWarning = FALSE;
	long buffer_multiplier;
	long quadOffset = pSrvrStmt->getQuadEntryCount(descType) - numEntries;

	// Adjust totalMemLen to word boundary
	*totalMemLen = ((*totalMemLen + 8 - 1) >> 3) << 3;

	if (rowsetSize) buffer_multiplier = rowsetSize;
	else buffer_multiplier = 1;

	MEMORY_DELETE_ARRAY(*varBuffer);

	DEBUG_ASSERT(*totalMemLen!=0,("totalMemLen==0"));
	MEMORY_ALLOC_ARRAY(*varBuffer,BYTE,(*totalMemLen) * buffer_multiplier);
#ifdef _DEBUG
	memset(*varBuffer,0,(*totalMemLen) * buffer_multiplier);
#endif

	memPtr = *varBuffer ;
	memOffset = 0;
	for (i = 0 ; i < SQLDesc->_length ; i++)
	{
		SQLItemDesc = (SQLItemDesc_def *) SQLDesc->_buffer + i;

		getMemoryAllocInfo(SQLItemDesc->dataType, SQLItemDesc->SQLCharset, SQLItemDesc->maxLen, SQLItemDesc->vc_ind_length, memOffset,
			&memAlignOffset, &allocSize, &varLayout);

		if ((SQLItemDesc->dataType==SQLTYPECODE_INTERVAL) ||
			(SQLItemDesc->dataType==SQLTYPECODE_DATETIME))
		{
			retcode = CLI_SetDescItem(pDesc,
				i+1,
				SQLDESC_TYPE,
				(long)SQLTYPECODE_VARCHAR_WITH_LENGTH,
				NULL);
			HANDLE_ERROR(retcode, sqlWarning);
			retcode = CLI_SetDescItem(pDesc,
				i+1,
				SQLDESC_LENGTH,
				SQLItemDesc->maxLen,
				NULL);
			HANDLE_ERROR(retcode, sqlWarning);
			retcode = CLI_SetDescItem(pDesc,
				i+1,
				SQLDESC_VC_IND_LENGTH,
				(long)sizeof(short),
				NULL);
			HANDLE_ERROR(retcode, sqlWarning);
		}

		VarPtr = memPtr + memOffset + memAlignOffset;

		DEBUG_OUT(DEBUG_LEVEL_DATA,("Desc %d dataType=%s, VarPtr=0x%08x, memAlignOffset=%ld",
			i,
			CliDebugSqlTypeCode(SQLItemDesc->dataType),
			VarPtr,
			memAlignOffset));

		memOffset += memAlignOffset + allocSize * buffer_multiplier;

		if (SQLItemDesc->nullInfo)
		{
			memAlignOffset = (((memOffset + 2 - 1) >> 1) << 1) - memOffset;
			IndPtr = memPtr + memOffset + memAlignOffset;
			DEBUG_OUT(DEBUG_LEVEL_DATA,("IndPtr=0x%08x, memAlignOffset=%ld",
				IndPtr,
				memAlignOffset));
			memOffset += memAlignOffset + sizeof(short) * buffer_multiplier;
		} else IndPtr = NULL;

		if (rowsetSize==0)
		{
			retcode = CLI_SetDescItem(pDesc,
				i+1,
				SQLDESC_VAR_PTR,
				(long)VarPtr,
				NULL);
			HANDLE_ERROR(retcode, sqlWarning);
			retcode = CLI_SetDescItem(pDesc,
				i+1,
				SQLDESC_IND_PTR,
				(long)IndPtr,
				NULL);
			HANDLE_ERROR(retcode, sqlWarning);
		} else {
			quadField[i+quadOffset].var_layout = varLayout;
			quadField[i+quadOffset].var_ptr = (BYTE *) VarPtr;
			quadField[i+quadOffset].ind_ptr = (BYTE *) IndPtr;
			if (IndPtr) quadField[i+quadOffset].ind_layout = sizeof(short);
			else quadField[i+quadOffset].ind_layout = 0;
		}

		implDesc[i].varPtr = VarPtr;
		implDesc[i].indPtr = IndPtr;

		if (memOffset > (*totalMemLen * buffer_multiplier))
		{
			DEBUG_OUT(DEBUG_LEVEL_ENTRY,("memOffset (%ld) > totalMemLen (%ld) * buffer_multiplier(%ld)",
				memOffset,
				*totalMemLen,
				buffer_multiplier));
			CLI_DEBUG_RETURN_SQL(PROGRAM_ERROR);
		}
	}

	if (rowsetSize)
	{
		// Must set rowset entry var_layout to zero for set
		if (descType==SRVR_STMT_HDL::Input) quadField[0].var_layout = 0;
		// Setup descriptors for rowset processing
		int rowset_status;
		retcode = CLI_SETROWSETDESCPOINTERS(
			pDesc,
			rowsetSize,
			&rowset_status,
			1L,
			numEntries,
			quadField);
		HANDLE_ERROR(retcode, sqlWarning);
	}

	if (sqlWarning) CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
	CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN AllocAssignValueBuffer(SQLItemDescList_def *SQLDesc,  SQLValueList_def *SQLValueList,
								 long totalMemLen, 	long maxRowCount, BYTE *&VarBuffer)
{
	FUNCTION_ENTRY("AllocAssignValueBuffer",
		("SQLDesc=0x%08x, SQLValueList=0x%08x, totalMemLen=%ld, maxRowCount=%ld, VarBuffer=0x%08x",
		SQLDesc,
		SQLValueList,
		totalMemLen,
		maxRowCount,
		VarBuffer));


	SQLItemDesc_def *SQLItemDesc;
	SQLValue_def *SQLValue;
	long memOffset = 0;
	BYTE *VarPtr;
	BYTE *IndPtr;
	BYTE *memPtr;
	unsigned long curValueCount, curDescCount;
	long curRowCount;
	long numValues;
	//long AllocLength; 64 change
	int AllocLength;
	long totalRowMemLen;

	// Allocate SQLValue Array
	MEMORY_DELETE_ARRAY(SQLValueList->_buffer);
	MEMORY_DELETE_ARRAY(VarBuffer);
	numValues = SQLDesc->_length * maxRowCount;
	if (numValues == 0)
	{
		SQLValueList->_buffer = NULL;
		SQLValueList->_length = 0;
		VarBuffer = NULL;
		CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
	}
	else
	{
		MEMORY_ALLOC_ARRAY(SQLValueList->_buffer, SQLValue_def, numValues);
		SQLValueList->_length = 0;
	}


	// Allocate the Value Buffer
	totalRowMemLen = totalMemLen * maxRowCount;
	MEMORY_ALLOC_ARRAY(VarBuffer, BYTE, totalRowMemLen);
	memPtr = VarBuffer ;
	memOffset = 0;

	for (curRowCount = 0, curValueCount = 0 ; curRowCount < maxRowCount ; curRowCount++)
	{
		for (curDescCount = 0 ; curDescCount < SQLDesc->_length ; curDescCount++, curValueCount++)
		{
			long memAlignOffset;
			SQLItemDesc = (SQLItemDesc_def *)SQLDesc->_buffer + curDescCount;
			SQLValue  = (SQLValue_def *)SQLValueList->_buffer + curValueCount;

			getMemoryAllocInfo(SQLItemDesc->dataType, SQLItemDesc->SQLCharset, SQLItemDesc->maxLen, SQLItemDesc->vc_ind_length, memOffset,
				&memAlignOffset, &AllocLength, NULL);

			VarPtr = memPtr + memOffset + memAlignOffset;
			memOffset += memAlignOffset + AllocLength;

			if (SQLItemDesc->nullInfo)
			{
				memOffset = ((memOffset + 2 - 1) >> 1) << 1;
				IndPtr = memPtr + memOffset;
				memOffset += sizeof(short) ;
			}
			else
				IndPtr = NULL;

			SQLValue->dataValue._buffer = (unsigned char *) VarPtr;
			// Ignore the indPtr, since it is declared as short already in SQLValue_def
			SQLValue->dataType = SQLItemDesc->dataType;
			SQLValue->dataValue._length = AllocLength;
			SQLValue->dataCharset = SQLItemDesc->ODBCCharset;
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLValue: dataValue._buffer=0x%08x, dataType=%s, dataValue._length=%ld, dataCharset=%s",
				SQLValue->dataValue._buffer,
				CliDebugSqlTypeCode(SQLValue->dataType),
				SQLValue->dataValue._length,
				getCharsetEncoding(SQLValue->dataCharset)));

			if (memOffset > totalRowMemLen)
			{
				//TFDS
				/*				if (srvrEventLogger != NULL)
				{
				srvrEventLogger->SendEventMsg(MSG_PROGRAMMING_ERROR, EVENTLOG_ERROR_TYPE,
				srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
				1, "memOffset > totalMemLen in AllocAssignValueBuffer");
				}
				*/
				DEBUG_OUT(DEBUG_LEVEL_ENTRY,("memOffset > totalRowMemLen"));
				CLI_DEBUG_RETURN_SQL(PROGRAM_ERROR);
			}
		}
		// Align it to next word boundary for the next row
		memOffset = ((memOffset + 8 - 1) >> 3) << 3;
	}
	CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN CopyValueList(SQLValueList_def *outValueList, const SQLValueList_def *inValueList)
{
	FUNCTION_ENTRY("CopyValueList",
		("outValueList=0x%08x, inValueList=0x%08x",
		outValueList,
		inValueList));

	SQLValue_def *inValue;
	SQLValue_def *outValue;
	unsigned long i;

	for (i = 0; i < inValueList->_length ; i++)
	{
		inValue = (SQLValue_def *)inValueList->_buffer + i;
		outValue = (SQLValue_def *)outValueList->_buffer + i;

		if (inValue->dataType != outValue->dataType)
		{
			DEBUG_OUT(DEBUG_LEVEL_ENTRY,("inValue->dataType != outValue->dataType"));
			CLI_DEBUG_RETURN_SQL(PROGRAM_ERROR);
		}
		outValue->dataInd = inValue->dataInd;
		if (inValue->dataInd == 0)
		{
			if (inValue->dataValue._length != outValue->dataValue._length)
			{
				// TFDS
				DEBUG_OUT(DEBUG_LEVEL_ENTRY,("inValue->dataValue._length != outValue->dataValue._length"));
				CLI_DEBUG_RETURN_SQL(PROGRAM_ERROR);
			}
			memcpy(outValue->dataValue._buffer, inValue->dataValue._buffer, outValue->dataValue._length);
		}
		else
		{
			outValue->dataValue._length = 0;
		}
	}
	outValueList->_length = inValueList->_length;
	CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN BuildSQLDesc(SRVR_STMT_HDL*pSrvrStmt, SRVR_STMT_HDL::DESC_TYPE descType)
{
	FUNCTION_ENTRY("BuildSQLDesc", ("pSrvrStmt=0x%08x, pSrvrStmt->stmtName = %s, SQL Statement = %s, descType=%s",
		pSrvrStmt,
		pSrvrStmt->stmtName,
		pSrvrStmt->sqlString.dataValue._buffer,
		CliDebugDescTypeStr(descType)));

	long retcode = SQL_SUCCESS;
	short i;
	short j;
	short k;

	BOOL sqlWarning = FALSE;

	long *totalMemLen = pSrvrStmt->getDescBufferLenPtr(descType);
	long numEntries = pSrvrStmt->getDescEntryCount(descType);
	SQLDESC_ID *pDesc = pSrvrStmt->getDesc(descType);
	SQLItemDescList_def *SQLDesc = pSrvrStmt->getDescList(descType);
	SQLItemDesc_def *SQLItemDesc = (SQLItemDesc_def *)SQLDesc->_buffer + SQLDesc->_length;

	SRVR_DESC_HDL *implDesc = pSrvrStmt->allocImplDesc(descType);

	// The following routine is hard coded for at least 15 items, so make sure it does not change
	DEBUG_ASSERT(NO_OF_DESC_ITEMS>=15,("NO_OF_DESC_ITEMS(%d) is less than 15",NO_OF_DESC_ITEMS));
	*totalMemLen = 0;
	for (i = 0; i < numEntries; i++) {
		SQLItemDesc = (SQLItemDesc_def *)SQLDesc->_buffer + SQLDesc->_length;
		// Initialize the desc entry in SQLDESC_ITEM struct
		for (j = 0; j < NO_OF_DESC_ITEMS ; j++) {
			gDescItems[j].entry = i+1;
		}
		gDescItems[10].num_val_or_len = MAX_ANSI_NAME_LEN+1;
		gDescItems[11].num_val_or_len = MAX_ANSI_NAME_LEN+1;
		gDescItems[12].num_val_or_len = MAX_ANSI_NAME_LEN+1;
		gDescItems[13].num_val_or_len = MAX_ANSI_NAME_LEN+1;
		gDescItems[14].num_val_or_len = MAX_ANSI_NAME_LEN+1;

		retcode = CLI_GetDescItems2(pDesc,
			NO_OF_DESC_ITEMS,
			(SQLDESC_ITEM *)&gDescItems);
		HANDLE_ERROR(retcode, sqlWarning);

		SQLItemDesc->dataType     = gDescItems[0].num_val_or_len;
		SQLItemDesc->maxLen       = gDescItems[1].num_val_or_len;
		SQLItemDesc->precision    = (short)gDescItems[2].num_val_or_len;
		SQLItemDesc->scale        = (short)gDescItems[3].num_val_or_len;
		SQLItemDesc->nullInfo     = (BOOL)gDescItems[4].num_val_or_len;
		SQLItemDesc->paramMode    = gDescItems[5].num_val_or_len;
		SQLItemDesc->intLeadPrec  = gDescItems[6].num_val_or_len;
		SQLItemDesc->datetimeCode = gDescItems[7].num_val_or_len;
		SQLItemDesc->SQLCharset   = gDescItems[8].num_val_or_len;
		SQLItemDesc->fsDataType   = gDescItems[9].num_val_or_len;
		for (k = 10; k < 15; k++) {
			gDescItems[k].string_val[gDescItems[k].num_val_or_len] = '\0';
		}
		SQLItemDesc->vc_ind_length = gDescItems[15].num_val_or_len;

		SQLItemDesc->maxLen = AdjustCharLength(descType, SQLItemDesc->SQLCharset, SQLItemDesc->maxLen);

		GetJDBCValues(	SQLItemDesc, 			// Input
			*totalMemLen,
			gDescItems[14].string_val);

		implDesc[i].charSet         = SQLItemDesc->SQLCharset;
		implDesc[i].dataType        = SQLItemDesc->dataType;
		implDesc[i].length          = SQLItemDesc->maxLen;
		implDesc[i].precision       = SQLItemDesc->ODBCPrecision;
		implDesc[i].scale           = SQLItemDesc->scale;
		implDesc[i].sqlDatetimeCode = SQLItemDesc->datetimeCode;
		implDesc[i].FSDataType      = SQLItemDesc->fsDataType;
		implDesc[i].paramMode       = SQLItemDesc->paramMode;
		implDesc[i].vc_ind_length   = SQLItemDesc->vc_ind_length;

		SQLItemDesc->version = 0;

		strcpy(SQLItemDesc->catalogNm, gDescItems[10].string_val);
		strcpy(SQLItemDesc->schemaNm, gDescItems[11].string_val);
		strcpy(SQLItemDesc->tableNm, gDescItems[12].string_val);
		strcpy(SQLItemDesc->colNm, gDescItems[13].string_val);
		strcpy(SQLItemDesc->colLabel, gDescItems[14].string_val);

		SQLDesc->_length++;

	}

	retcode = SET_DATA_PTR(pSrvrStmt, descType);
	HANDLE_ERROR(retcode, sqlWarning);

	if (sqlWarning) CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
	CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

/* ***********************************************************************************************
* FUNCTION: RSgetRSmax
*
* DESCRIPTION:
* This routine will call SQL_EXEC_GetStmtAttr() to get the maximum number
* of RS's the stmt can rtn. {Note: This value corrsponds to the value of
* "DYNAMIC RESULT SETS" specified during the CREATE PROCEDURE registration.
*
* ARGUMENTS:
*     INPUT/OUTPUT:
*			pSrvrStmt			- A C++ object that contains information about the
*								statement being used.
*     OUTPUT:
*			pSrvrStmt->RSMax	- Max number of SPJRS allowed for this stmt.
* Returns:
*	SQL_SUCCESS or
*    SQL failures (see CLI_GetStmtAttr() retcode failures).
*********************************************************************************************** */
SQLRETURN RSgetRSmax(SRVR_STMT_HDL *pSrvrStmt)
{
	FUNCTION_ENTRY("RSgetRSmax",
		("pSrvrStmt=0x%08x",pSrvrStmt));

	CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

	BOOL			sqlWarning = FALSE;
	SQLSTMT_ID		*pStmt = &pSrvrStmt->stmt;
	int			retcode = SQL_SUCCESS;
	int			max_num_rs = 0;

	pSrvrStmt->RSMax = 0;

	// Get RSMax (max SPJ Result Sets that can be returned for this stmt)
	retcode = CLI_GetStmtAttr((SQLSTMT_ID *)pStmt,			// (IN) SQL statement ID
		SQL_ATTR_MAX_RESULT_SETS,	// (IN) Request query statement attribute (max RS per stmt)
		&max_num_rs,				// (OUT) Place to store Max resultsets
		NULL,						// (OUT) Optional string
		0,							// (IN) Max size of optional string buffer
		NULL );						// (IN) Length of item

	// Handle CLI errors
	if (retcode != SQL_SUCCESS)
	{
		// CLI_ClearDiagnostics(NULL);
		DEBUG_OUT(DEBUG_LEVEL_CLI|DEBUG_LEVEL_STMT,
			("CLI_GetStmtAttr(SQL_ATTR_MAX_RESULT_SETS) FAILED : retcode=%s",
			CliDebugSqlError(retcode)));
		CLI_DEBUG_RETURN_SQL(retcode);
	}

	pSrvrStmt->RSMax = max_num_rs;

	DEBUG_ASSERT((pSrvrStmt->RSMax>0)&&(pSrvrStmt->RSMax<256),
		("Max ResultSets(%ld) is out of range", max_num_rs));
	// Point to first RS if greater than 0
	if(pSrvrStmt->RSMax > 0)
		pSrvrStmt->RSIndex = 1;

	// Also should fail if value is less than one or greater than 255
	DEBUG_OUT(DEBUG_LEVEL_STMT,("RSMax: %d  RSIndex: %d  isSPJRS: %d",
		pSrvrStmt->RSMax, pSrvrStmt->RSIndex, pSrvrStmt->isSPJRS));

	CLI_DEBUG_RETURN_SQL(retcode);
}

/* ***********************************************************************************************
* FUNCTION: ReadRow
*
* DESCRIPTION: Called during int or a fetch.
* This routine will retrieve information from a descriptor entry.
* Length of retrieved items is returned in totalRows.
* Copy items to the SQL output value list
* via the kdsCopyToSQLValueSeq() routine.
*
* ARGUMENTS:
*     INPUT/OUTPUT:
*			pSrvrStmt - A C++ object that contains information about the statement being
*                               used.
*     OUTPUT:
*			totalRows - running total of rows read.
*			rowsRead  - rows read by last fetch
* Returns:
*	SQL_SUCCESS or
*    SQL failures (see CLI_GetDescItem() for retcode failures).
*********************************************************************************************** */
static SQLRETURN ReadRow(SRVR_STMT_HDL *pSrvrStmt,
						 int *totalRows, long *rowsRead)
{
	FUNCTION_ENTRY("ReadRow",
		("pSrvrStmt=0x%08x, totalRows=%ld, rowsRead=0x%08x",
		pSrvrStmt,
		*totalRows,
		rowsRead));

	CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

	long	retcode = SQL_SUCCESS;			// assume success
	//long	allocLength;					// allocation length 64 change
	int		allocLength;					// allocation length
	long	charSet;
	long	columnCount = 0;				// total number of columns
	long	curColumnNo = 0;				// current column number
	long	dataLength;
	long	dataType;
	short	*indPtr;						// Linux port - ToDo: this is declared as Int64 in SQ ???
	short	indValue;
	BYTE	*pBytes;

	SQLDESC_ID		*pOutputDesc;
	SQLValueList_def *pOutputValueList;
	BOOL sqlWarning = FALSE;
	SRVR_DESC_HDL	*IRD;

	pOutputValueList = &pSrvrStmt->outputValueList;
	pOutputDesc = &pSrvrStmt->outputDesc;
	columnCount = pSrvrStmt->columnCount;
	IRD = pSrvrStmt->IRD;

	DEBUG_OUT(DEBUG_LEVEL_DATA,("pOutputValueList=0x%08x",
		pOutputValueList));

	// Assert that outputValueList buffer is not null, if null assert
	DEBUG_ASSERT(pOutputValueList->_buffer!=NULL,("pOutputValueList->_buffer==NULL"));
	// This check may be needed; otherwise, a SIG11 occurs from calling kdsCopyToSqlValueSeq()
	// if the buffer is null.  This has been found to occur when JVM memory is exhausted.
	// if (pOutputValueList->_buffer == NULL) CLI_DEBUG_RETURN_SQL(SQL_NULL_DATA);

	if (pSrvrStmt->fetchRowsetSize > 0)
	{
		// This is a Rowset fetch
		int item_count = 0;
		long row;
		retcode = CLI_GetDescItem(pOutputDesc,
			1L,
			SQLDESC_ROWSET_NUM_PROCESSED,
			&item_count,
			NULL,0,NULL,0);

		if (retcode != SQL_SUCCESS) CLI_DEBUG_RETURN_SQL(retcode);

		// Assert that colCnt equals fetch QuadEntries, if not equal assert
		DEBUG_ASSERT(columnCount==pSrvrStmt->fetchQuadEntries,
			("columnCount(%ld)!=pSrvrStmt->fetchQuadEntries(%ld)",
			columnCount,
			pSrvrStmt->fetchQuadEntries));
		DEBUG_OUT(DEBUG_LEVEL_DATA,("Rowset rowsRead=%ld",item_count));

		if (rowsRead) *rowsRead = item_count;
		if (item_count==0) CLI_DEBUG_RETURN_SQL(retcode);

		for (row=0; row<item_count; row++)
		{
			for (curColumnNo = 0; curColumnNo < columnCount; curColumnNo++)
			{
				dataType = IRD[curColumnNo].dataType;
				indPtr = (short *) pSrvrStmt->fetchQuadField[curColumnNo].ind_ptr;
				if ((indPtr == NULL) || (indPtr[row] != -1)) indValue = 0;
				else indValue = -1;
				charSet = IRD[curColumnNo].charSet;
				dataLength = IRD[curColumnNo].length;
				getMemoryAllocInfo(dataType,
					charSet,
					dataLength,
					IRD[curColumnNo].vc_ind_length,
					0,
					NULL,
					&allocLength,
					NULL);
				pBytes = (BYTE *)(pSrvrStmt->fetchQuadField[curColumnNo].var_ptr);

				if (charSet != SQLCHARSETCODE_ISO88591 && dataType == SQLTYPECODE_VARCHAR_WITH_LENGTH)
					pBytes += row * (allocLength - 1);
				else
					pBytes += row * allocLength;

				DEBUG_OUT(DEBUG_LEVEL_DATA,("RowSet Row %ld Column %ld pBytes=0x%08x item_count=%ld",
					row,
					curColumnNo,
					pBytes,
					item_count));
				MEMORY_DUMP(DEBUG_LEVEL_DATA,pBytes,allocLength);
				kdsCopyToSQLValueSeq(pOutputValueList,
					dataType,
					indValue,
					pBytes,
					allocLength,
					charSet);
			}
		}
		*totalRows += item_count;
	}
	else
	{
		// This is not a Rowset fetch
		*totalRows += 1;
		if (rowsRead) *rowsRead = 1;

		for (curColumnNo = 0; curColumnNo < columnCount; curColumnNo++)
		{
			dataType = IRD[curColumnNo].dataType;
			indPtr = (short *) IRD[curColumnNo].indPtr;
			if ((indPtr == NULL) || (indPtr[0] != -1)) indValue = 0;
			else indValue = -1;
			charSet = IRD[curColumnNo].charSet;
			pBytes = (BYTE *)(IRD[curColumnNo].varPtr);
			dataLength = IRD[curColumnNo].length;

			// Compute memory allocation requirements for descriptor
			getMemoryAllocInfo(dataType,
				charSet,
				dataLength,
				IRD[curColumnNo].vc_ind_length,
				0,
				NULL,
				&allocLength,
				NULL);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("totalRows %ld curColumnNo %ld",*totalRows,curColumnNo));
			MEMORY_DUMP(DEBUG_LEVEL_DATA,pBytes,allocLength);
			kdsCopyToSQLValueSeq(pOutputValueList, dataType, indValue, pBytes, allocLength, charSet);
		}
	}

	DEBUG_OUT(DEBUG_LEVEL_DATA,("totalRows=%ld curColumnNo=%ld pOutputValueList=0x%08x",
		*totalRows, curColumnNo, pOutputValueList));
	CLI_DEBUG_RETURN_SQL((SQLRETURN)retcode);
}

/* ***********************************************************************************************
* FUNCTION: EXECUTE
*
* DESCRIPTION: Evaluate input parameters and execute an SQL statement, make diagnostic
*              information available and, if results are present, position the implicit cursor
*              before the first row of the result set.
*
*			For V2.1+ SQL their are four classes of Trafodion query stmt types:
*			1) Unique_selects (1 row) call ClearExecFetchClose() SQL/CLI (w/ a valid outputDesc).
*			2) Non_unique selects (multiple rows) statement types call Trafodion CLI Exec (not chg'd from V2.0 SQL).
*			3) UID (unique and non_unique) stmt types call SQL/CLI ClearExecFetchClose() (w/ a NULL outputDesc).
*			4) All other statements (SQL_CONTROL, SQL_SET_TRANSACTION, SQL_SET_CATALOG, SQL_SET_SCHEMA)
*			call SQL/CLI ClearExecFetchClose() (w/ a NULL outputDesc).
*
*
* ARGUMENTS:
*     INPUT: pSrvrStmt->inputRowCnt - When rowsets is used, this is the maximum number of rows to be fetched
*                                     when using the ExecFetch CLI call.
*            pSrvrStmt->sqlStmtType - Used with V2.0 SQL to indicate a select and non-select statement type.
*
*     OUTPUT: pSrvrStmt->rowCount - The number of rows affected by the Exec or ClearExec/Fetch/Close.
*
*********************************************************************************************** */
SQLRETURN EXECUTE(SRVR_STMT_HDL* pSrvrStmt)
{
	FUNCTION_ENTRY("EXECUTE",
		("pSrvrStmt=0x%08x",
		pSrvrStmt));

	CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

	SRVR_CONNECT_HDL *pConnect = NULL;
	if(pSrvrStmt->dialogueId == 0) CLI_DEBUG_RETURN_SQL(SQL_ERROR);
	pConnect = (SRVR_CONNECT_HDL*)pSrvrStmt->dialogueId;


	BOOL isSPJRS = false;
	long retcode = SQL_SUCCESS;
	SQLDESC_ID *pInputDesc;
	SQLDESC_ID *pOutputDesc;
	SQLSTMT_ID *pStmt;
	SQLSTMT_ID cursorId;
	long		paramCount;
	char		*cursorName;
	long		len;
	long		curParamNo;
	long		curLength;
	BYTE		*varPtr;
	BYTE		*indPtr;
	void			*pBytes;
	SQLValue_def	*SQLValue;
	BOOL			sqlWarning = FALSE;
	int				rtn = 0;
	pSrvrStmt->isSPJRS = FALSE;
	pStmt = &pSrvrStmt->stmt;
	pSrvrStmt->endOfData = FALSE;		// true=return code 100 from CLI call (no data found)
	if ((pSrvrStmt->stmtType == EXTERNAL_STMT) &&
		(pSrvrStmt->moduleId.module_name == NULL))
	{
		// Conditions to check cursor
		// SQL 2.0 Chk cursor if version==SQL 2.0 AND SQL Stmt Type==TYPE_SELECT.
		// SQL 2.1+ Chk cursor if SQL Stmt query type is a non_unique select.
		if ((pSrvrStmt->getSqlQueryStatementType() == SQL_SELECT_NON_UNIQUE) ||
			((GlobalInformation::getSQLMX_Version() == CLI_VERSION_R2) && (pSrvrStmt->sqlStmtType & TYPE_SELECT)))
		{
			cursorName = pSrvrStmt->cursorName;
			// If cursor name is not specified, use the stmt name as cursor name
			// due to bug in Trafodion, though it should default automatically
			if (*cursorName == '\0')
				cursorName = pSrvrStmt->stmtName;

			DEBUG_OUT(DEBUG_LEVEL_CLI,("cursorName='%s', previousCursorName='%s'",
				cursorName,
				pSrvrStmt->previousCursorName));

			// If cursorName has chg'd from last EXEC or EXECDIRECT cmd
			// or has not yet been set for the first time call SetCursorName
			if ((strcmp(pSrvrStmt->previousCursorName, cursorName) != 0) &&
				(*cursorName != '\0'))
			{
				cursorId.version        = SQLCLI_ODBC_VERSION;
				cursorId.module         = pStmt->module;
				cursorId.handle         = 0;
				cursorId.charset        = SQLCHARSETSTRING_ISO88591;
				cursorId.name_mode      = cursor_name;
				cursorId.identifier_len = strlen(cursorName);
				cursorId.identifier     = cursorName;
				strcpy(pSrvrStmt->previousCursorName, cursorName);	// keep track of last cursorName
				DEBUG_OUT(DEBUG_LEVEL_CLI,("EXECUTE cursorName %s previousCursorName %s",
					cursorName,
					pSrvrStmt->previousCursorName));

				retcode = CLI_SetCursorName(pStmt, &cursorId);
				HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
			}
		}
	}

	pInputDesc  = &pSrvrStmt->inputDesc;		// Input descriptor pointer
	pOutputDesc = &pSrvrStmt->outputDesc;		// Output descriptor pointer. Used in ClearExecFetchClose()
	paramCount  = pSrvrStmt->paramCount;

	if(GlobalInformation::getSQLMX_Version() == CLI_VERSION_R2)
	{
		pSrvrStmt->isClosed = FALSE;		// Assume we are open after EXEC_Exec

		if (pSrvrStmt->sqlStmtType & TYPE_SELECT) retcode = CLI_Exec(pStmt, pInputDesc, 0);
		else retcode = CLI_ExecFetch(pStmt, pInputDesc, 0);
	}
	else				// VER 2.1
	{
		// Should be Closed. If Opened assert
		DEBUG_ASSERT(pSrvrStmt->isClosed, ("Server Statement is Open before execute."));
		pSrvrStmt->isClosed = FALSE;		// Assume we are open after EXEC_Exec
		switch (pSrvrStmt->getSqlQueryStatementType())
		{
		case SQL_SELECT_UNIQUE:
			DEBUG_OUT(DEBUG_LEVEL_CLI,("Unique SELECT statement type."));
			if(pConnect->isSPJRS)
				isSPJRS = true;
			if(!isSPJRS) {
				// ClearExecFetchClose performs exec, fetch, and close
				retcode = CLI_ClearExecFetchClose(pStmt, pInputDesc, pOutputDesc, 0, 0, 0);
				// ClearExecFetchClose() does an implicit close
				pSrvrStmt->isClosed = TRUE;
			}
			else {
				//For SPJRS SELECT UNIQUE
				//Using Exec,fetch separately for SPJRS Configuration to avoid statement that is in the closed state.
				retcode = CLI_Exec(pStmt, pInputDesc, 0);
				if (retcode != 0) CLI_ClearDiagnostics(pStmt);
				else {
					retcode = CLI_Fetch(pStmt, pOutputDesc, 0);
					if (retcode != 0) {
						CLI_ClearDiagnostics(pStmt);
						pSrvrStmt->endOfData = TRUE; //SPJRS
					}
				}
			}
			break;

		case SQL_SELECT_NON_UNIQUE:
			DEBUG_OUT(DEBUG_LEVEL_CLI,("Non-Unique SELECT statement type."));
			retcode = CLI_Exec(pStmt, pInputDesc, 0);
			break;

			// IUD (Insert, Update, Delete) unique and non-unique Statement Types
		case SQL_CONTROL:
		case SQL_SET_TRANSACTION:
		case SQL_SET_CATALOG:
		case SQL_SET_SCHEMA:
			DEBUG_OUT(DEBUG_LEVEL_CLI,("Control or Set Connection Attribute statement type."));
		case SQL_INSERT_UNIQUE:
		case SQL_UPDATE_UNIQUE:
		case SQL_DELETE_UNIQUE:
			// SQL_OTHER (DDLs - create/alter/drop/etc) should behave like IUD non-unique Statement Types
		case SQL_OTHER:
		case SQL_INSERT_NON_UNIQUE:
		case SQL_UPDATE_NON_UNIQUE:
		case SQL_DELETE_NON_UNIQUE:
		default:
			DEBUG_OUT(DEBUG_LEVEL_CLI,("IUD (Non-Unique and Unique) and control statement types."));
			// ClearExecFetchClose performs exec, fetch, and close (implicit close)
			// Output Desc = NULL
			retcode = CLI_ClearExecFetchClose(pStmt, pInputDesc, NULL, 0, 0, 0);

			// ClearExecFetchClose() does an implicit close
			pSrvrStmt->isClosed = TRUE;
			break;
		} // End of stmt type switch
	} // End of VER20 check

	if (retcode != SQL_SUCCESS) 
		pSrvrStmt->isClosed = TRUE;
	// Process the SQL CLI return code
	if (retcode != 0){						// SQL success
		if (retcode == 100) {				// No Data Found
			CLI_ClearDiagnostics(pStmt);
			retcode = 0;
			pSrvrStmt->endOfData = TRUE;	// indicates no data found using ClearExecFetchClose()
		} else if (retcode < 0) {			// SQL Error
			THREAD_RETURN(pSrvrStmt,SQL_ERROR);
		} else {							// > 0, SQL Warning.
			sqlWarning = TRUE;
		}
	}

	HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

	if(GlobalInformation::getSQLMX_Version() == CLI_VERSION_R2)
	{
		// Not a SELECT stmt type
		if ((pSrvrStmt->sqlStmtType & TYPE_SELECT) == 0)
		{
                        Int64 tmpRowCount;
			// Get row count
			retcode =  CLI_GetDiagnosticsStmtInfo2 (pStmt, SQLDIAG_ROW_COUNT,
					&tmpRowCount,
				NULL, 0, NULL);
                        if (retcode == SQL_SUCCESS)
                           pSrvrStmt->rowCount._buffer[0] = (int)tmpRowCount;
                        else
                           pSrvrStmt->rowCount._buffer[0] = 0;
			pSrvrStmt->totalRowCount += pSrvrStmt->rowCount._buffer[0];
			// Batch Binding Size
			if (pSrvrStmt->batchRowsetSize)
			{
				// Currently we only get one value back from SQL for rowsets.
				// We will fill in all values with SUCCESS_NO_INFO and ignore the
				//   one value until SQL updates to return the actual number.
				long row_idx;
#ifndef TODO	// Linux port Todo - Currently hardcoding actual value
				long info_value = SQLMXStatement_SUCCESS_NO_INFO();
#else
				long info_value = -2L;
#endif				
				for (row_idx=0; row_idx<pSrvrStmt->inputRowCnt; row_idx++)
				{
					pSrvrStmt->rowCount._buffer[pSrvrStmt->rowCount._length+row_idx] = info_value;
				}
			}
			if (retcode < 0) sqlWarning = TRUE;
			else pSrvrStmt->rowCount._length += pSrvrStmt->inputRowCnt;
		}
	}
	else {				// VER 2.1 or later

		// Linux port- moving variable declarations here from case blocks because of compile errors
		long rows_read = 0;
		long row_idx;
		long info_value;

		switch (pSrvrStmt->getSqlQueryStatementType())
		{
			// CLI will not return unique stmt types for rowsets
			// Rowset processing will not be a unique command
		case SQL_UPDATE_UNIQUE:
		case SQL_INSERT_UNIQUE:
		case SQL_DELETE_UNIQUE:
			if(pSrvrStmt->endOfData) {		// UID unique cmds chk for end of data
				pSrvrStmt->rowCount._buffer[0] = 0;	// If NO DATA FOUND (100) set to 0 row count
			}
			else {
				pSrvrStmt->rowCount._buffer[0] = 1;	// If DATA FOUND set row count to 1
			}
			pSrvrStmt->rowCount._length = 1;
			pSrvrStmt->totalRowCount += pSrvrStmt->rowCount._buffer[0];
			break;
		case SQL_SELECT_UNIQUE:
			// Read row count and load outputValueList for unique select
			pSrvrStmt->rowsAffected = 0;
			rows_read = 0;
			if(!pSrvrStmt->endOfData)
			{
				retcode = ReadRow(pSrvrStmt, &pSrvrStmt->rowsAffected, &rows_read);
				if (retcode < 0) THREAD_RETURN(pSrvrStmt,retcode);
				if (retcode > 0) sqlWarning = TRUE;
				pSrvrStmt->totalRowCount += rows_read;
			}
			DEBUG_OUT(DEBUG_LEVEL_DATA,("pSrvrStmt->rowsAffected=%ld ; rows_read=%ld", pSrvrStmt->rowsAffected, rows_read));
			break;
		case SQL_CONTROL:
		case SQL_SET_TRANSACTION:
		case SQL_SET_CATALOG:
		case SQL_SET_SCHEMA:
			DEBUG_OUT(DEBUG_LEVEL_CLI,("Control --- Set Connection Attribute statement type."));
		case SQL_SELECT_NON_UNIQUE:			// Row count to be filled in on subsequent fetch operations
			break;
			// SQL_OTHER (DDLs - create/alter/drop/etc) should behave like IUD non-unique Statement Types
		case SQL_OTHER:
		case SQL_INSERT_NON_UNIQUE:
		case SQL_UPDATE_NON_UNIQUE:
		case SQL_DELETE_NON_UNIQUE:
		default:
                        Int64 tmpRowCount;
			// For all other NON-UNIQUE statements get the row count from GetDiag2
			retcode =  CLI_GetDiagnosticsStmtInfo2(	pStmt,
				SQLDIAG_ROW_COUNT,
                                &tmpRowCount, 
				NULL,
				0,
				NULL);
                        if (retcode == 0)
		           pSrvrStmt->rowCount._buffer[0] = (int)tmpRowCount;
                        else
		           pSrvrStmt->rowCount._buffer[0] = 0;
			pSrvrStmt->totalRowCount += pSrvrStmt->rowCount._buffer[0];
			if (pSrvrStmt->batchRowsetSize > 0){
				// Currently we only get one value back from SQL for rowsets.
				// We will fill in all values with SUCCESS_NO_INFO and ignore the
				// one value until SQL updates to return the actual number.
#ifndef TODO	// Linux port Todo - Currently hardcoding actual value				
				info_value = SQLMXStatement_SUCCESS_NO_INFO();
#else				
				info_value = -2L;
#endif
				for (row_idx=0; row_idx<pSrvrStmt->inputRowCnt; row_idx++) {
					pSrvrStmt->rowCount._buffer[pSrvrStmt->rowCount._length+row_idx] = info_value;
				}
			}
			if (retcode < 0) sqlWarning = TRUE;
			else {
				pSrvrStmt->rowCount._length += pSrvrStmt->inputRowCnt;
				DEBUG_OUT(DEBUG_LEVEL_CLI,("pSrvrStmt->rowCount._length=%ld", pSrvrStmt->rowCount._length));
			}
			break;
		} // end of default type
	} // End of Version 2.0 Check

	DEBUG_OUT(DEBUG_LEVEL_ENTRY,( "pSrvrStmt->endOfData=%ld, pSrvrStmt->outputValueList._buffer=0x%08x, pSrvrStmt->outputValueList._length=0x%08x, pSrvrStmt->totalRowCount=%ld",
		pSrvrStmt->endOfData, pSrvrStmt->outputValueList._buffer, pSrvrStmt->outputValueList._length,pSrvrStmt->totalRowCount));

	if (sqlWarning) THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);
	THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}

SQLRETURN FREESTATEMENT(SRVR_STMT_HDL* pSrvrStmt)
{
	FUNCTION_ENTRY("FREESTATEMENT",
		("pSrvrStmt=0x%08x",
		pSrvrStmt));

	CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

	long retcode;
	BOOL sqlWarning = FALSE;

	SQLSTMT_ID *pStmt;
	SQLDESC_ID *pDesc;

	if (pSrvrStmt == NULL)
	{
		DEBUG_OUT(DEBUG_LEVEL_ENTRY,("Server statement not allocated"));
		THREAD_RETURN(pSrvrStmt,SQL_INVALID_HANDLE);
	}


	pStmt = &pSrvrStmt->stmt;

	pSrvrStmt->rowsAffected = -1;
	switch(pSrvrStmt->freeResourceOpt)
	{
	case SQL_DROP:			// Logical Close
		pDesc = &pSrvrStmt->inputDesc;
		pSrvrStmt->freeBuffers(SQLWHAT_INPUT_DESC);

		// Don't dealloc input descriptor if SPJRS (since it has not been allocated)
		if ((pSrvrStmt->inputDescName[0] == '\0') && (!pSrvrStmt->isSPJRS))
		{
			retcode = CLI_DeallocDesc(pDesc);
			if( trace_SQL ) LogDelete("SQL_EXEC_DeallocDesc(pDesc);",(void**)&pDesc,pDesc);
		}
		pDesc = &pSrvrStmt->outputDesc;
		pSrvrStmt->freeBuffers(SQLWHAT_OUTPUT_DESC);
		if (pSrvrStmt->outputDescName[0] == '\0')
		{
			retcode = CLI_DeallocDesc(pDesc);
			if( trace_SQL ) LogDelete("SQL_EXEC_DeallocDesc(pDesc);",(void**)&pDesc,pDesc);
		}
		if (!pSrvrStmt->isClosed)
		{
			retcode = CLI_CloseStmt(pStmt);
			pSrvrStmt->isClosed = TRUE;
		}
		retcode = CLI_ClearDiagnostics(pStmt);
		if (pSrvrStmt->moduleName[0] == '\0')
		{
			retcode = CLI_DeallocStmt(pStmt);
			if( trace_SQL ) LogDelete("SQL_EXEC_DeallocStmt(pStmt);",(void**)&pStmt,pStmt);
		}
		// For drop, always return success, even if there was a warning.
		// This was migrated logic during IDL removal.
		THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
	case SQL_CLOSE:			// Physical or hard close
		if (! pSrvrStmt->isClosed)
		{
			retcode = CLI_CloseStmt(pStmt);
			pSrvrStmt->isClosed = TRUE;
			if ((pSrvrStmt->stmtType == INTERNAL_STMT) && (retcode!=0)) CLI_ClearDiagnostics(pStmt);
			else HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
		}
		break;
	default:
		break;
	}
	if (sqlWarning) THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);
	THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}

SQLRETURN PREPARE(SRVR_STMT_HDL* pSrvrStmt)
{
	FUNCTION_ENTRY("PREPARE",
		("pSrvrStmt=0x%08x",
		pSrvrStmt));

	CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

	long retcode;
	SQLRETURN rc;

	SQLSTMT_ID	*pStmt;
	SQLDESC_ID	*pInputDesc;
	SQLDESC_ID	*pOutputDesc;

	long		numEntries;
	char		*pStmtName;
	BOOL		sqlWarning = FALSE;
	BOOL		rgWarning = FALSE;
	int    SqlQueryStatementType;

	pStmt = &pSrvrStmt->stmt;
	pOutputDesc = &pSrvrStmt->outputDesc;
	pInputDesc = &pSrvrStmt->inputDesc;

	if (!pSrvrStmt->isClosed)
	{
		retcode = CLI_CloseStmt(pStmt);
		if (retcode!=0) retcode = CLI_ClearDiagnostics(pStmt);
		pSrvrStmt->isClosed = TRUE;
	}

	if (pSrvrStmt->holdability == HOLD_CURSORS_OVER_COMMIT){
		retcode = CLI_SetStmtAttr(pStmt, SQL_ATTR_CURSOR_HOLDABLE, SQL_HOLDABLE, NULL);
		HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
	}

	SQLDESC_ID	sqlString_desc;

	sqlString_desc.version        = SQLCLI_ODBC_VERSION;
	sqlString_desc.module         = &pSrvrStmt->moduleId;
	sqlString_desc.name_mode      = string_data;
	sqlString_desc.identifier     = (const char *)pSrvrStmt->sqlString.dataValue._buffer;
	sqlString_desc.handle         = 0;
	sqlString_desc.identifier_len = pSrvrStmt->sqlString.dataValue._length;
	sqlString_desc.charset        = SQLCHARSETSTRING_ISO88591;

	retcode = CLI_Prepare(pStmt, &sqlString_desc);

	int rtn;
	
	HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

	pSrvrStmt->estimatedCost = -1;

	retcode = CLI_DescribeStmt(pStmt, pInputDesc, pOutputDesc);
	HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

	retcode = CLI_GetDescEntryCount(pInputDesc, (int *)&pSrvrStmt->paramCount);
	HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

	retcode = CLI_GetDescEntryCount(pOutputDesc, (int *)&pSrvrStmt->columnCount);
	HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

	pSrvrStmt->prepareSetup();

	if (pSrvrStmt->paramCount > 0){
		kdsCreateSQLDescSeq(&pSrvrStmt->inputDescList, pSrvrStmt->paramCount+pSrvrStmt->inputDescParamOffset);
		retcode = BuildSQLDesc(pSrvrStmt, SRVR_STMT_HDL::Input);
		HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
	} else {
		kdsCreateEmptySQLDescSeq(&pSrvrStmt->inputDescList);
	}

	if (pSrvrStmt->columnCount > 0){
		kdsCreateSQLDescSeq(&pSrvrStmt->outputDescList, pSrvrStmt->columnCount);
		retcode = BuildSQLDesc(pSrvrStmt, SRVR_STMT_HDL::Output);
		HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
	} else {
		kdsCreateEmptySQLDescSeq(&pSrvrStmt->outputDescList);
	}

	/* *****************************************************************************
	* The call to CLI_GetStmtAttr to query the statement type was added as a
	* performance enhancement. Previous versions of the Trafodion database will not return
	* a statement type, but will return a 0 which is SQL_OTHER. In the case were
	* SQL_OTHER is returned and JDBC/MX knows what the statement type is, then the
	* JDBC/MX statement type will be used. This will allow the JDBC/MX driver to
	* run with an older version of the Trafodion.
	* ***************************************************************************** */


	DEBUG_OUT(DEBUG_LEVEL_CLI,( "getSQLMX_Version: returned %i", GlobalInformation::getSQLMX_Version()));

	if (GlobalInformation::getSQLMX_Version() == CLI_VERSION_R2 ) {    //If this version of Trafodion is version R2
		if (pSrvrStmt->sqlStmtType != TYPE_UNKNOWN)                    //If this is a SELECT, INVOKE, or SHOWSHAPE
			SqlQueryStatementType = SQL_SELECT_NON_UNIQUE;              //then force an execute with no fetch
		else SqlQueryStatementType = SQL_OTHER;                         //else allow an executeFetch
	}
	else
	{
		retcode = CLI_GetStmtAttr( &pSrvrStmt->stmt,		// (IN) SQL statement ID
			SQL_ATTR_QUERY_TYPE,		// (IN) Request query statement attribute
			(int*)&SqlQueryStatementType,	// (OUT) Place to store query statement type
			NULL,					// (OUT) Optional string
			0,						// (IN) Max size of optional string buffer
			NULL );					// (IN) Length of item


		//If there is an error this statement will return
		HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
	}
	DEBUG_OUT(DEBUG_LEVEL_CLI,("SQL Query Statement Type=%s",
		CliDebugSqlQueryStatementType(SqlQueryStatementType)));
		
		if (SqlQueryStatementType == SQL_EXE_UTIL  &&
            pSrvrStmt->columnCount > 0)
          SqlQueryStatementType = SQL_SELECT_NON_UNIQUE;
          
	pSrvrStmt->setSqlQueryStatementType(SqlQueryStatementType);

	switch (pSrvrStmt->getSqlQueryStatementType())
	{
	case SQL_CALL_NO_RESULT_SETS:
		DEBUG_OUT(DEBUG_LEVEL_CLI|DEBUG_LEVEL_STMT,("Prepare SQL_CALL_NO_RESULT_SETS query type"));
		pSrvrStmt->isSPJRS = false;		// Indicate this is an RS.
		pSrvrStmt->RSIndex = 0;			// Index into RS array
		pSrvrStmt->RSMax = 0;			// No Result Sets to return
		DEBUG_OUT(DEBUG_LEVEL_CLI|DEBUG_LEVEL_STMT,("RSMax: %d  RSIndex: %d  isSPJRS: %d ", pSrvrStmt->RSMax, pSrvrStmt->RSIndex,  pSrvrStmt->isSPJRS));
		break;

	case SQL_CALL_WITH_RESULT_SETS:
		DEBUG_OUT(DEBUG_LEVEL_CLI|DEBUG_LEVEL_STMT,("Prepare SQL_CALL_WITH_RESULT_SETS query type"));
		pSrvrStmt->isSPJRS = true;
		retcode = RSgetRSmax(pSrvrStmt);
		DEBUG_OUT(DEBUG_LEVEL_CLI|DEBUG_LEVEL_STMT,("RSMax: %d  RSIndex: %d  isSPJRS: %d  ", pSrvrStmt->RSMax, pSrvrStmt->RSIndex,  pSrvrStmt->isSPJRS));
		//If there is an error this statement will return
		HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
		break;
	}
	if (sqlWarning) THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);
	THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}

static SQLRETURN NoDataFound(SRVR_STMT_HDL *pSrvrStmt, long curRowNo, int curRowCount, int *rowsAffected)
{
	FUNCTION_ENTRY("NoDataFound",("pSrvrStmt=0x%08x, curRowNo=%ld, curRowCount=%ld, rowsAffected=0x%08x",
		pSrvrStmt,
		curRowNo,
		curRowCount,
		rowsAffected));

	if (curRowNo == 1)
	{
		CLI_ClearDiagnostics(&pSrvrStmt->stmt);
		CLI_DEBUG_RETURN_SQL(SQL_NO_DATA_FOUND);
	}
	*rowsAffected = curRowCount;
	DEBUG_OUT(DEBUG_LEVEL_ENTRY,("rowsAffected=%ld",*rowsAffected));
	CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
}

SQLRETURN FETCH(SRVR_STMT_HDL *pSrvrStmt)
{
	FUNCTION_ENTRY("FETCH",
		("pSrvrStmt=0x%08x",
		pSrvrStmt));

	long retcode=SQL_SUCCESS;
	int curRowCount = 0;
	long curRowNo;
	long curColumnNo;
	BYTE *varPtr;
	short *indPtr;
	BYTE *pBytes;
	long dataType;
	long dataLength;
	long allocLength;
	short indValue;
	long columnCount;
	long charSet;
	long fetchSize;

	SQLDESC_ID *pDesc;
	BOOL sqlWarning = FALSE;

	pDesc = &pSrvrStmt->outputDesc;
	columnCount = pSrvrStmt->columnCount;

	if (pSrvrStmt->fetchRowsetSize == 0) {
		fetchSize = 1;
	}
	else {
		fetchSize = pSrvrStmt->fetchRowsetSize;
	}

	curRowNo = 1;
	while (curRowNo <= pSrvrStmt->maxRowCnt)
	{
		DEBUG_OUT(DEBUG_LEVEL_STMT,("***Anitha ---- >pSrvrStmt->isClosed=%ld", pSrvrStmt->isClosed));

		/// For Modius
		CLI_ClearDiagnostics(&pSrvrStmt->stmt);
		retcode = CLI_Fetch(&pSrvrStmt->stmt, pDesc, 0);

		int rtn;
		if (retcode != 0)
		{                  //Check for a bad return code
			if (retcode == 100)
			{
				THREAD_RETURN(pSrvrStmt,NoDataFound(pSrvrStmt,curRowNo,curRowCount,&pSrvrStmt->rowsAffected));
			}
			else if (retcode < 0)
			{
				THREAD_RETURN(pSrvrStmt,SQL_ERROR);
			}
			else
			{
				sqlWarning = TRUE;
			}
		}

		// Read row count and load the output value list
		long rows_read = 0;
		retcode = ReadRow(pSrvrStmt, &curRowCount, &rows_read);
		if (retcode < 0) THREAD_RETURN(pSrvrStmt,retcode);
		if (retcode > 0) sqlWarning = TRUE;

		curRowNo += rows_read;
	}
	pSrvrStmt->rowsAffected = curRowCount > pSrvrStmt->maxRowCnt ? pSrvrStmt->maxRowCnt : curRowCount;
	DEBUG_OUT(DEBUG_LEVEL_ENTRY,("pSrvrStmt->rowsAffected=%ld curRowCount=%ld pSrvrStmt->maxRowCnt=%ld",
		pSrvrStmt->rowsAffected, curRowCount, pSrvrStmt->maxRowCnt));

	if (sqlWarning) THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);
	THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}


SQLRETURN GETSQLERROR(SRVR_STMT_HDL *pSrvrStmt,
					  odbc_SQLSvc_SQLError *SQLError)
{
	FUNCTION_ENTRY("GETSQLERROR",
		("pSrvrStmt=0x%08x, SQLError=0x%08x",
		pSrvrStmt,
		SQLError));

	long retcode;
	int total_conds = 0;
	int buf_len;
	int sqlcode  = 0;
	char sqlState[6];
	int curr_cond = 1;

	retcode =  CLI_GetDiagnosticsStmtInfo2(NULL,
		SQLDIAG_NUMBER,
		&total_conds, NULL, 0, NULL);

	if (total_conds == 0)
	{
		kdsCreateSQLErrorException(SQLError, 1);
		kdsCopySQLErrorException(SQLError, "No error message in Trafodion diagnostics area, but sqlcode is non-zero", retcode, "");
		CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
	}

	kdsCreateSQLErrorException(SQLError, total_conds);
	while (curr_cond <= total_conds)
	{
		char *msg_buf=NULL;
		int msg_buf_len;
		retcode = CLI_GetDiagnosticsCondInfo2(SQLDIAG_SQLCODE, curr_cond,
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

			retcode = CLI_GetDiagnosticsCondInfo2(SQLDIAG_MSG_LEN, curr_cond,
				&msg_buf_len, NULL, 0, NULL);
		}
		if (retcode >= SQL_SUCCESS)
		{
			MEMORY_ALLOC_ARRAY(msg_buf, char, msg_buf_len+1);
			msg_buf[msg_buf_len] = 0;
			buf_len = 0;
			// By passing the msg_buf_len to the following SQL call,
			// the returned buf_len will be equal to msg_buf_len w/o a
			// null terminator. If a value greater than msg_buf_len is passed,
			// msg_buf will be null terminated by SQL.
			retcode = CLI_GetDiagnosticsCondInfo2(SQLDIAG_MSG_TEXT, curr_cond,
				NULL, msg_buf, msg_buf_len, &buf_len);
			DEBUG_ASSERT(msg_buf[msg_buf_len]==0,("Memory corruption detected during error message handling"));
		}
		if (retcode >= SQL_SUCCESS)
		{
			// Found that the returned CLI message length can be incorrect.  If the
			//   size returned is too small, we will retry once with a bigger buffer and hope
			//   it works.
			if (buf_len>msg_buf_len)
			{
				DEBUG_OUT(DEBUG_LEVEL_CLI,("CLI Message length changed.  Retrying."));
				MEMORY_DELETE_ARRAY(msg_buf);
				msg_buf_len = buf_len;
				MEMORY_ALLOC_ARRAY(msg_buf, char, msg_buf_len+1);
				msg_buf[msg_buf_len] = 0;
				buf_len = 0;
				retcode = CLI_GetDiagnosticsCondInfo2(SQLDIAG_MSG_TEXT, curr_cond,
					NULL, msg_buf, msg_buf_len, &buf_len);
				DEBUG_ASSERT(msg_buf[msg_buf_len]==0,("Memory corruption detected during error message retry handling"));
				// Happened again.  Just use the short buffer.
				if (buf_len>msg_buf_len) buf_len = msg_buf_len;
			}
			// Null terminate msg_buf since it is not yet null terminated.
			msg_buf[buf_len] = '\0';
			DEBUG_OUT(DEBUG_LEVEL_CLI,("msg_buf='%s'",msg_buf));
			buf_len = 0;
			retcode = CLI_GetDiagnosticsCondInfo2(SQLDIAG_RET_SQLSTATE, curr_cond,
				NULL, sqlState, sizeof(sqlState), &buf_len);
		}
		if (retcode < SQL_SUCCESS)
		{
			kdsCopySQLErrorException(SQLError, "Internal Error : From CLI_GetDiagnosticsCondInfo2",
				retcode, "");
			MEMORY_DELETE_ARRAY(msg_buf);
			break;
		}
		sqlState[5] = '\0';
		kdsCopySQLErrorException(SQLError, msg_buf, sqlcode, sqlState);
		MEMORY_DELETE_ARRAY(msg_buf);
		curr_cond++;
	}

	CLI_ClearDiagnostics(NULL);
	CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN EXECDIRECT(SRVR_STMT_HDL* pSrvrStmt)
{
	FUNCTION_ENTRY("EXECDIRECT",
		("pSrvrStmt=0x%08x",
		pSrvrStmt));

	CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

	long retcode = SQL_SUCCESS;

	SQLSTMT_ID	*pStmt;
	SQLDESC_ID	*pInputDesc;
	SQLDESC_ID	*pOutputDesc;
	SQLSTMT_ID  cursorId;

	long		numEntries;
	char		*pStmtName;
	char		*cursorName;
	size_t		len;
	BOOL		sqlWarning = FALSE;

	pStmt = &pSrvrStmt->stmt;
	pInputDesc = &pSrvrStmt->inputDesc;
	pOutputDesc = &pSrvrStmt->outputDesc;

	SQLDESC_ID	sqlString_desc;

	sqlString_desc.version        = SQLCLI_ODBC_VERSION;
	sqlString_desc.module         = &pSrvrStmt->moduleId;
	sqlString_desc.name_mode      = string_data;
	sqlString_desc.identifier     = (const char *) pSrvrStmt->sqlString.dataValue._buffer;
	sqlString_desc.handle         = 0;
	sqlString_desc.identifier_len = pSrvrStmt->sqlString.dataValue._length;
	sqlString_desc.charset        = SQLCHARSETSTRING_ISO88591;

	DEBUG_ASSERT(pSrvrStmt->isClosed, ("Server Statement is Open before execute."));
	pSrvrStmt->isClosed = TRUE;

	if (pSrvrStmt->sqlStmtType & TYPE_SELECT)
	{
		retcode = CLI_Prepare(pStmt, &sqlString_desc);
		HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
		pSrvrStmt->estimatedCost = -1;
	}
	else
	{
		retcode = CLI_ExecDirect(pStmt, &sqlString_desc, 0, 0);
		HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
		pSrvrStmt->isClosed = FALSE;
		pSrvrStmt->estimatedCost = -1;
	}

	if (pSrvrStmt->sqlStmtType & TYPE_SELECT)
	{
		// Retrieving/loading OutputDesc values
		retcode = CLI_DescribeStmt(pStmt, (SQLDESC_ID *)NULL, pOutputDesc);
		HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

		retcode = CLI_GetDescEntryCount(pOutputDesc, (int*)&pSrvrStmt->columnCount);
		HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

		if (pSrvrStmt->columnCount > 0)
		{
			kdsCreateSQLDescSeq(&pSrvrStmt->outputDescList, pSrvrStmt->columnCount);
			retcode = BuildSQLDesc(pSrvrStmt, SRVR_STMT_HDL::Output);
			HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
		}
		else
		{
			kdsCreateEmptySQLDescSeq(&pSrvrStmt->outputDescList);
		}
	}
	else
	{
		pSrvrStmt->columnCount = 0;
		kdsCreateEmptySQLDescSeq(&pSrvrStmt->outputDescList);
	}
	if (pSrvrStmt->stmtType == EXTERNAL_STMT)
	{
		if (pSrvrStmt->sqlStmtType & TYPE_SELECT)
		{
			cursorName = pSrvrStmt->cursorName;
			// If cursor name is not specified, use the stmt name as cursor name
			if (*cursorName == '\0') {
				cursorName = pSrvrStmt->stmtName;
			}

			DEBUG_OUT(DEBUG_LEVEL_CLI,("cursorName %s previousCursorName %s",
				cursorName,
				pSrvrStmt->previousCursorName));

			// If cursorName has chg'd from last EXEC or EXECDIRECT cmd
			// or has not yet been set for the first time call SetCursorName
			if ((strcmp(pSrvrStmt->previousCursorName, cursorName) != 0) && *cursorName != '\0')
			{
				cursorId.version        = SQLCLI_ODBC_VERSION;
				cursorId.module         = pStmt->module;
				cursorId.handle         = 0;
				cursorId.charset        = SQLCHARSETSTRING_ISO88591;
				cursorId.name_mode      = cursor_name;
				cursorId.identifier_len = strlen(cursorName);
				cursorId.identifier     = cursorName;
				strcpy(pSrvrStmt->previousCursorName, cursorName);		// keep track of last cursor name used
				DEBUG_OUT(DEBUG_LEVEL_CLI,("cursorName %s previousCursorName %s",
					cursorName,
					pSrvrStmt->previousCursorName));

				retcode = CLI_SetCursorName(pStmt, &cursorId);
				HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
			}
			retcode = CLI_Exec(pStmt, NULL, 0);
			HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
			pSrvrStmt->isClosed = FALSE;
		}
	}

	if (retcode >= SQL_SUCCESS)
	{
		// If NOT a select type
		if ((pSrvrStmt->sqlStmtType & TYPE_SELECT)==0)
		{
                        Int64 tmpRowCount;
			retcode =  CLI_GetDiagnosticsStmtInfo2(pStmt, SQLDIAG_ROW_COUNT, &tmpRowCount,
				NULL, 0, NULL);
			if (retcode < 0)
			{
				sqlWarning = TRUE;
				pSrvrStmt->rowsAffected = -1;
			}
                        else
                                pSrvrStmt->rowsAffected = (int)tmpRowCount;
		}
		else
			pSrvrStmt->rowsAffected = -1;
	}

	if (sqlWarning) THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);
	THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}

SQLRETURN EXECUTESPJRS(SRVR_STMT_HDL *pSrvrStmt)
{
	FUNCTION_ENTRY("EXECUTESPJRS",
		("pSrvrStmt=0x%08x",
		pSrvrStmt));

	CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

	long retcode = SQL_SUCCESS;

	SQLDESC_ID		*pOutputDesc;
	SQLSTMT_ID		*pStmt;
	SQLSTMT_ID		cursorId;
	long			numEntries;
	char			*pStmtName;
	char			*cursorName;
	size_t			len;
	BOOL			sqlWarning = FALSE;
	int				rtn = 0;
	SQLRETURN		rc;

	// For debug purposes only
	//long int		SqlQueryStatementType;

	pStmt = &pSrvrStmt->stmt;
	pOutputDesc = &pSrvrStmt->outputDesc;

	DEBUG_OUT(DEBUG_LEVEL_STMT,("pStmt=0x%08x, pOutputDesc=0x%08x, RSIndex=%ld, isClosed=0x%08x",
		pStmt,
		pOutputDesc,
		pSrvrStmt->RSIndex,
		pSrvrStmt->isClosed));

	// Retrieving/loading OutputDesc values
	retcode = CLI_DescribeStmt(pStmt, (SQLDESC_ID *)NULL, pOutputDesc);

	DEBUG_OUT(DEBUG_LEVEL_CLI, ("EXECUTESPJRS : retcode = %s",
		CliDebugSqlError(retcode)));

	// Trying to open a non-existent SPJRS
	if(retcode == RS_DOES_NOT_EXIST)
	{
		pSrvrStmt->isClosed = TRUE;
		kdsCreateEmptySQLDescSeq(&pSrvrStmt->outputDescList);
		THREAD_RETURN(pSrvrStmt,SQL_RS_DOES_NOT_EXIST);
	}
	HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

	retcode = CLI_GetDescEntryCount(pOutputDesc,(int*) &pSrvrStmt->columnCount);
	HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);

	if (pSrvrStmt->columnCount > 0)
	{
		kdsCreateSQLDescSeq(&pSrvrStmt->outputDescList, pSrvrStmt->columnCount);
		retcode = BuildSQLDesc(pSrvrStmt, SRVR_STMT_HDL::Output);
		HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
	}
	else
	{
		kdsCreateEmptySQLDescSeq(&pSrvrStmt->outputDescList);
	}

	cursorName = pSrvrStmt->cursorName;
	// If cursor name is not specified, use the stmt name as cursor name
	if (*cursorName == '\0') {
		cursorName = pSrvrStmt->stmtName;
	}

	DEBUG_OUT(DEBUG_LEVEL_CLI|DEBUG_LEVEL_STMT,("cursorName %s : previousCursorName %s",
		cursorName, pSrvrStmt->previousCursorName));

	// If cursorName has chg'd from last EXEC or EXECDIRECT cmd
	// or has not yet been set for the first time call SetCursorName
	if ((strcmp(pSrvrStmt->previousCursorName, cursorName) != 0) && *cursorName != '\0')
	{
		cursorId.version        = SQLCLI_ODBC_VERSION;
		cursorId.module         = pStmt->module;
		cursorId.handle         = 0;
		cursorId.charset        = SQLCHARSETSTRING_ISO88591;
		cursorId.name_mode      = cursor_name;
		cursorId.identifier_len = strlen(cursorName);
		cursorId.identifier     = cursorName;
		strcpy(pSrvrStmt->previousCursorName, cursorName);
		DEBUG_OUT(DEBUG_LEVEL_CLI|DEBUG_LEVEL_STMT,
			("Calling CLI_SetCursorName - cursorName %s : previousCursorName %s",
			cursorName,
			pSrvrStmt->previousCursorName));

		retcode = CLI_SetCursorName(pStmt, &cursorId);
		HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
	}

	DEBUG_ASSERT(pSrvrStmt->isClosed, ("Server RS Statement is Open before execute."));
	retcode = CLI_Exec(pStmt, NULL, 0);
	DEBUG_OUT(DEBUG_LEVEL_STMT,("EXECUTESPJRS  CLI_EXEC  retcode: %ld.", retcode));
	HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
	pSrvrStmt->isClosed = FALSE;

	if (sqlWarning) THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);
	THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}

SQLRETURN GETSQLWARNING(SRVR_STMT_HDL *pSrvrStmt,
						ERROR_DESC_LIST_def *sqlWarning)
{
	// This function is needed since SQLError uses odbc_SQLSvc_SQLError * instead of ERROR_DESC_LIST_def *
	// Hence this function wraps around GETSQLERROR
	FUNCTION_ENTRY("GETSQLWARNING",
		("pSrvrStmt=0x%08x, sqlWarning=0x%08x",
		pSrvrStmt,
		sqlWarning));

	odbc_SQLSvc_SQLError SQLError;
	CLEAR_ERROR(SQLError);

	long retcode = GETSQLERROR(pSrvrStmt, &SQLError);
	sqlWarning->_length = SQLError.errorList._length;
	sqlWarning->_buffer = SQLError.errorList._buffer;
	CLI_DEBUG_RETURN_SQL((SQLRETURN)retcode);
}


SQLRETURN CANCEL(SRVR_STMT_HDL *pSrvrStmt)
{
	FUNCTION_ENTRY("CANCEL",
		("pSrvrStmt=0x%08x",
		pSrvrStmt));

	THREAD_RETURN(pSrvrStmt,CLI_Cancel(&pSrvrStmt->stmt));
}


SQLRETURN CLEARDIAGNOSTICS(SRVR_STMT_HDL *pSrvrStmt)
{
	FUNCTION_ENTRY("CLEARDIAGNOSTICS",
		("pSrvrStmt=0x%08x",
		pSrvrStmt));

	long retcode = CLI_ClearDiagnostics(&pSrvrStmt->stmt);
	CLI_DEBUG_RETURN_SQL((SQLRETURN)retcode);
}

SQLRETURN ALLOCSQLMXHDLS(SRVR_STMT_HDL* pSrvrStmt)
{
	FUNCTION_ENTRY("ALLOCSQLMXHDLS", ("pSrvrStmt=0x%08x",
		pSrvrStmt));
#if defined(TAG64)
    int _ptr32* tempStmtId;
#endif
	long retcode = SQL_SUCCESS;
	SQLSTMT_ID	*pStmt = &pSrvrStmt->stmt;
	SQLDESC_ID	*pInputDesc;
	SQLDESC_ID	*pOutputDesc;
	SQLMODULE_ID *pModule = &pSrvrStmt->moduleId;
	BOOL		sqlWarning;

	pStmt->version = SQLCLI_ODBC_VERSION;
	pStmt->module = pModule;
	pStmt->handle = 0;
	pStmt->charset = SQLCHARSETSTRING_ISO88591;
	
	if (pSrvrStmt->stmtName[0] != '\0')
	{
		pStmt->name_mode = stmt_name;
		pStmt->identifier_len = strlen(pSrvrStmt->stmtName);
		pStmt->identifier = pSrvrStmt->stmtName;
	}
	else
	{
		pStmt->name_mode = stmt_handle;
		pStmt->identifier_len = 0;
		pStmt->identifier = NULL;
	}
	pStmt->tag = 0;
	if (pModule->module_name == NULL)
	{
		retcode = CLI_AllocStmt(pStmt,(SQLSTMT_ID *)NULL);
		if (retcode < 0)
		{
			CLI_ClearDiagnostics(NULL);
			CLI_DEBUG_RETURN_SQL(retcode);
		}
	}


	if (!pSrvrStmt->useDefaultDesc)
	{
		pInputDesc = &pSrvrStmt->inputDesc;
		pInputDesc->version = SQLCLI_ODBC_VERSION;
		pInputDesc->handle = 0;
		pInputDesc->charset = SQLCHARSETSTRING_ISO88591;
		pInputDesc->name_mode = desc_handle;
		pInputDesc->identifier_len = 0;
		pInputDesc->identifier = NULL;
		pInputDesc->module = pModule;
		retcode = CLI_AllocDesc(pInputDesc, (SQLDESC_ID *)NULL);
		if (retcode < 0)
		{
			CLI_ClearDiagnostics(NULL);
			CLI_DEBUG_RETURN_SQL(retcode);
		}
		pOutputDesc = &pSrvrStmt->outputDesc;
		pOutputDesc->version = SQLCLI_ODBC_VERSION;
		pOutputDesc->handle = 0;
		pOutputDesc->charset = SQLCHARSETSTRING_ISO88591;
		pOutputDesc->name_mode = desc_handle;
		pOutputDesc->identifier_len = 0;
		pOutputDesc->identifier = NULL;
		pOutputDesc->module = pModule;
		retcode = CLI_AllocDesc(pOutputDesc, (SQLDESC_ID *)NULL);
		if (retcode < 0)
		{
			CLI_ClearDiagnostics(NULL);
			CLI_DEBUG_RETURN_SQL(retcode);
		}
	}
	else
	{
		pInputDesc = &pSrvrStmt->inputDesc;
		pInputDesc->version = SQLCLI_ODBC_VERSION;
		pInputDesc->handle = 0;
		pInputDesc->name_mode = desc_name;
		pInputDesc->identifier_len = MAX_DESC_NAME_LEN;
		pInputDesc->identifier = pSrvrStmt->inputDescName;
		pInputDesc->module = pModule;
		retcode = CLI_ResDescName(pInputDesc, pStmt, SQLWHAT_INPUT_DESC);
		if (retcode == -8803)
		{
			pInputDesc->identifier_len = 0;
			CLI_ClearDiagnostics(NULL);
			retcode = 0;
		}
		if (retcode < 0)
		{
			CLI_ClearDiagnostics(NULL);
			CLI_DEBUG_RETURN_SQL(retcode);
		}
		pSrvrStmt->inputDescName[pInputDesc->identifier_len] = '\0';

		pOutputDesc = &pSrvrStmt->outputDesc;
		pOutputDesc->version = SQLCLI_ODBC_VERSION;
		pOutputDesc->handle = 0;
		pOutputDesc->charset = SQLCHARSETSTRING_ISO88591;
		pOutputDesc->name_mode = desc_name;
		pOutputDesc->identifier_len = MAX_DESC_NAME_LEN;
		pOutputDesc->identifier = pSrvrStmt->outputDescName;
		pOutputDesc->module = pModule;
		retcode = CLI_ResDescName(pOutputDesc, pStmt, SQLWHAT_OUTPUT_DESC);
		if (retcode == -8803)
		{
			pOutputDesc->identifier_len = 0;
			retcode = CLI_ClearDiagnostics(NULL);
			retcode = 0;
		}
		if (retcode < 0)
		{
			CLI_ClearDiagnostics(NULL);
			CLI_DEBUG_RETURN_SQL(retcode);
		}
		pSrvrStmt->outputDescName[pOutputDesc->identifier_len] = '\0';
	}

	// Set the input and output Desc to be Wide Descriptors
	if (!pSrvrStmt->useDefaultDesc)
	{
		DEBUG_OUT(DEBUG_LEVEL_CLI,("Non-Default descriptor."));
		//R321: passing 1 instead of 0 for CLI_SetDescItem 
		retcode = CLI_SetDescItem(pInputDesc, 1, SQLDESC_DESCRIPTOR_TYPE, DESCRIPTOR_TYPE_WIDE, NULL);
		if (retcode < 0)
		{
			CLI_ClearDiagnostics(NULL);
			CLI_DEBUG_RETURN_SQL(retcode);
		}
		//R321: passing 1 instead of 0 for CLI_SetDescItem
		retcode = CLI_SetDescItem(pOutputDesc, 1, SQLDESC_DESCRIPTOR_TYPE, DESCRIPTOR_TYPE_WIDE, NULL);
		if (retcode < 0)
		{
			CLI_ClearDiagnostics(NULL);
			CLI_DEBUG_RETURN_SQL(retcode);
		}
	}
	else
	{
		if (pSrvrStmt->inputDescName[0] != '\0')
		{
			DEBUG_OUT(DEBUG_LEVEL_CLI,("Default descriptor. Input Descriptor Name=%s",
				pSrvrStmt->inputDescName));
			//R321: passing 1 instead of 0 for CLI_SetDescItem
			retcode = CLI_SetDescItem(pInputDesc, 1, SQLDESC_DESCRIPTOR_TYPE, DESCRIPTOR_TYPE_WIDE, NULL);
			if (retcode < 0)
			{
				CLI_ClearDiagnostics(NULL);
				CLI_DEBUG_RETURN_SQL(retcode);
			}
		}
		if (pSrvrStmt->outputDescName[0] != '\0')
		{
			DEBUG_OUT(DEBUG_LEVEL_CLI,("Default descriptor. Output Descriptor Name=%s",
				pSrvrStmt->outputDescName));
			//R321: passing 1 instead of 0 for CLI_SetDescItem
			retcode = CLI_SetDescItem(pOutputDesc, 1, SQLDESC_DESCRIPTOR_TYPE, DESCRIPTOR_TYPE_WIDE, NULL);
			if (retcode < 0)
			{
				CLI_ClearDiagnostics(NULL);
				CLI_DEBUG_RETURN_SQL(retcode);
			}
		}
	}
	CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}


SQLRETURN ALLOCSQLMXHDLS_SPJRS(SRVR_STMT_HDL *pSrvrStmt, SQLSTMT_ID *callpStmt, const char *RSstmtLabel)
{
	FUNCTION_ENTRY("ALLOCSQLMXHDLS_SPJRS", ("pSrvrStmt=0x%08x, callpStmt=0x%08x, RSstmtLabel=%s",
		pSrvrStmt,
		callpStmt,
		RSstmtLabel));
#if defined(TAG64)
	int _ptr32* tempStmtId;
#endif
	long retcode = SQL_SUCCESS;
	SQLSTMT_ID	*pStmt = &pSrvrStmt->stmt;
	SQLDESC_ID	*pOutputDesc;
	SQLMODULE_ID *pModule = &pSrvrStmt->moduleId;
	BOOL		sqlWarning;

	pStmt->version = SQLCLI_ODBC_VERSION;
	pStmt->module = pModule;
	pStmt->handle = 0;
	pStmt->charset = SQLCHARSETSTRING_ISO88591;
	if (pSrvrStmt->stmtName[0] != '\0')
	{
		pStmt->name_mode = stmt_name;
		pStmt->identifier_len = strlen(pSrvrStmt->stmtName);
		pStmt->identifier = pSrvrStmt->stmtName;
	}
	else
	{
		pStmt->name_mode = stmt_handle;
		pStmt->identifier_len = 0;
		pStmt->identifier = NULL;
	}
	DEBUG_OUT(DEBUG_LEVEL_STMT,("***pStmt->name_mode=%ld", pStmt->name_mode));
	DEBUG_OUT(DEBUG_LEVEL_STMT,("***pStmt->identifier_len=%ld", pStmt->identifier_len));
	DEBUG_OUT(DEBUG_LEVEL_STMT,("***pStmt->identifier=%s", pStmt->identifier));

	pStmt->tag = 0;
	if (pModule->module_name == NULL)
	{
		DEBUG_OUT(DEBUG_LEVEL_STMT,("***pModule->module_name == NULL  Call AllocStmtForRs()"));
		CLI_AllocStmtForRS(callpStmt,
			pSrvrStmt->RSIndex,
			pStmt);
		if (retcode < 0)
		{
			CLI_ClearDiagnostics(NULL);
			CLI_DEBUG_RETURN_SQL(retcode);
		}
	}

	if (pSrvrStmt->useDefaultDesc)
	{
		pOutputDesc = &pSrvrStmt->outputDesc;
		pOutputDesc->version = SQLCLI_ODBC_VERSION;
		pOutputDesc->handle = 0;
		pOutputDesc->charset = SQLCHARSETSTRING_ISO88591;
		pOutputDesc->name_mode = desc_name;
		pOutputDesc->identifier_len = MAX_DESC_NAME_LEN;
		pOutputDesc->identifier = pSrvrStmt->outputDescName;
		pOutputDesc->module = pModule;
		retcode = CLI_ResDescName(pOutputDesc, pStmt, SQLWHAT_OUTPUT_DESC);
		if (retcode == -8803)
		{
			pOutputDesc->identifier_len = 0;
			retcode = CLI_ClearDiagnostics(NULL);
			retcode = 0;
		}
		if (retcode < 0)
		{
			CLI_ClearDiagnostics(NULL);
			CLI_DEBUG_RETURN_SQL(retcode);
		}
		pSrvrStmt->outputDescName[pOutputDesc->identifier_len] = '\0';
	}
	else
	{
		pOutputDesc = &pSrvrStmt->outputDesc;
		pOutputDesc->version = SQLCLI_ODBC_VERSION;
		pOutputDesc->handle = 0;
		pOutputDesc->charset = SQLCHARSETSTRING_ISO88591;
		pOutputDesc->name_mode = desc_handle;
		pOutputDesc->identifier_len = 0;
		pOutputDesc->identifier = NULL;
		pOutputDesc->module = pModule;
		retcode = CLI_AllocDesc(pOutputDesc, (SQLDESC_ID *)NULL);
		if (retcode < 0)
		{
			CLI_ClearDiagnostics(NULL);
			CLI_DEBUG_RETURN_SQL(retcode);
		}
	}

	// Set the output Desc to be Wide Descriptors
	if (pSrvrStmt->useDefaultDesc)
	{
		if (pSrvrStmt->outputDescName[0] != '\0')
		{
			DEBUG_OUT(DEBUG_LEVEL_CLI,("Default descriptor. Output Descriptor Name=%s",
				pSrvrStmt->outputDescName));
			//R321: passing 1 instead of 0 for CLI_SetDescItem
			retcode = CLI_SetDescItem(pOutputDesc, 1, SQLDESC_DESCRIPTOR_TYPE, DESCRIPTOR_TYPE_WIDE, NULL);
			if (retcode < 0)
			{
				CLI_ClearDiagnostics(NULL);
				CLI_DEBUG_RETURN_SQL(retcode);
			}
		}
	}
	else
	{
		DEBUG_OUT(DEBUG_LEVEL_CLI,("Non-Default descriptor."));
		//R321: passing 1 instead of 0 for CLI_SetDescItem
		retcode = CLI_SetDescItem(pOutputDesc, 1, SQLDESC_DESCRIPTOR_TYPE, DESCRIPTOR_TYPE_WIDE, NULL);
		if (retcode < 0)
		{
			CLI_ClearDiagnostics(NULL);
			CLI_DEBUG_RETURN_SQL(retcode);
		}
	}
	CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN EXECUTECALL(SRVR_STMT_HDL *pSrvrStmt)
{
	FUNCTION_ENTRY("EXECUTECALL",
		("pSrvrStmt=0x%08x",
		pSrvrStmt));

	CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);

	int 			rtn;
	long			columnCount;
	long			paramCount;
	long			retcode = SQL_SUCCESS;

	BOOL			sqlWarning = FALSE;
	SQLDESC_ID 		*pDesc;
	SQLDESC_ID		*pDescParam;
	SQLDESC_ID 		*pDescValue;
	SQLSTMT_ID 		*pStmt;
	SQLRETURN 		rc;

	pStmt      = &pSrvrStmt->stmt;

	DEBUG_OUT(DEBUG_LEVEL_STMT,("EXECUTECALL : isClosed = %ld", pSrvrStmt->isClosed));
	if (!pSrvrStmt->isClosed)
	{
		retcode = CLI_CloseStmt(pStmt);
		if (retcode!=0) retcode = CLI_ClearDiagnostics(pStmt);
		pSrvrStmt->isClosed = TRUE;
	}

	pDesc      = &pSrvrStmt->inputDesc;
	paramCount = pSrvrStmt->paramCount;

	if (paramCount > 0){
		pDescValue = pDesc;
	}
	else {
		pDescValue = NULL;
	}

	DEBUG_ASSERT(pSrvrStmt->isClosed, ("Server Statement is Open before int."));
	retcode = CLI_Exec(pStmt, pDescValue, 0);
	DEBUG_OUT(DEBUG_LEVEL_STMT,("intCALL  CLI_EXEC  retcode: %ld.", retcode));
	HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
	pSrvrStmt->isClosed = FALSE;

	pDesc       = &pSrvrStmt->outputDesc;
	columnCount = pSrvrStmt->columnCount;

	if (columnCount > 0) {
		pDescParam = pDesc;
	}
	else {
		pDescParam = NULL;
	}

	DEBUG_OUT(DEBUG_LEVEL_STMT,("***Anitha ---- >pSrvrStmt->isClosed=%ld", pSrvrStmt->isClosed));

	retcode = CLI_Fetch(pStmt, pDescParam, 0);

	// Return if the fetch failed
	HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);


	// SPJRS - Only close when not SPJRS and not already closed
	if ((!pSrvrStmt->isSPJRS) && (!pSrvrStmt->isClosed))
	{
		retcode = CLI_CloseStmt(pStmt);
		// Set the close flag so that another close is not tried
		pSrvrStmt->isClosed = TRUE;
		HANDLE_THREAD_ERROR(retcode, sqlWarning, pSrvrStmt);
	}

	if (sqlWarning) THREAD_RETURN(pSrvrStmt,SQL_SUCCESS_WITH_INFO);
	THREAD_RETURN(pSrvrStmt,SQL_SUCCESS);
}


SQLRETURN CONNECT(SRVR_CONNECT_HDL *pSrvrConnect)
{
	FUNCTION_ENTRY("CONNECT",
		("pSrvrConnect=0x%08x",
		pSrvrConnect));

	long retcode;
	BOOL sqlWarning = FALSE;
	retcode = CLI_CreateContext(&pSrvrConnect->contextHandle, NULL, 0);
	HANDLE_ERROR(retcode, sqlWarning);
	retcode = CLI_SwitchContext(pSrvrConnect->contextHandle, NULL);
	HANDLE_ERROR(retcode, sqlWarning);
	if (sqlWarning) CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
	CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN DISCONNECT(SRVR_CONNECT_HDL *pSrvrConnect)
{
	FUNCTION_ENTRY("DISCONNECT",
		("pSrvrConnect=0x%08x",
		pSrvrConnect));

	long retcode;

	// Hack.
	// Executor starts an internal transaction to clean up volatile schemas
	// when a context is deleted. But it does not clean up properly when this
	// is called from the T2 driver. This is because transactions from the T2
	// interface are no-waited. TMF expects AWAITIOX to be called after the
	// ENDTRANSACTION to clean up the TFILE entry. However, Executor does not
	// make any AWAITIOX calls. So this affects the "active" transaction for
	// the process - pthreads thinks it is working on a user transaction when
	// it is actually working on the committed Executor transaction. This causes
	// the calling thread to hang.
	//
	// Designing a proper fix for this will take some time. For now, we hack
	// around this by resuming the user transaction after the Executor returns
	// from the DeleteContext call.
	//Start Soln. No.: 10-110830-9447
	short txHandle[10];

	long txBeginTag;


	retcode = CLI_DeleteContext(pSrvrConnect->contextHandle);

	// Resume the transaction before handling any errors from the disconnect.
	resumeTransaction(txBeginTag);
	//End Soln. No.: 10-110830-9447

	if (retcode==0) CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
	CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
}

SQLRETURN SWITCHCONTEXT(SRVR_CONNECT_HDL *pSrvrConnect, long *sqlcode)
{
	FUNCTION_ENTRY("SWITCHCONTEXT",
		("pSrvrConnect=0x%08x, sqlcode=(out)",
		pSrvrConnect));
	long retcode;
	BOOL sqlWarning = FALSE;

	retcode = CLI_SwitchContext(pSrvrConnect->contextHandle, NULL);
	if (sqlcode != NULL)
		*sqlcode = retcode;
	HANDLE_ERROR(retcode, sqlWarning);
	if (sqlWarning) CLI_DEBUG_RETURN_SQL(SQL_SUCCESS_WITH_INFO);
	CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
}

SQLRETURN GETSQLCODE(SRVR_STMT_HDL *pSrvrStmt)
{
	FUNCTION_ENTRY("GETSQLCODE",("pSrvrStmt=0x%08x",pSrvrStmt));
	long retcode;
	int sqlcode  = 0;

	retcode = CLI_GetDiagnosticsCondInfo2(SQLDIAG_SQLCODE,
		1, &sqlcode, NULL, 0, NULL);
	if (retcode >= SQL_SUCCESS)
		CLI_DEBUG_RETURN_SQL(sqlcode);
	CLI_DEBUG_RETURN_SQL(retcode);
}

SQLRETURN COMMIT_ROWSET(long dialogueId, bool& bSQLMessageSet, odbc_SQLSvc_SQLError* SQLError, Int32 currentRowCount)
{
    SQLRETURN retcode;
    long      sqlcode;
    SQLValueList_def inValueList;
    inValueList._buffer = NULL;
    inValueList._length = 0;

    SRVR_STMT_HDL *CmwSrvrStmt = getInternalSrvrStmt(dialogueId, "STMT_COMMIT_1", &sqlcode);
    /* Should process the error here if CmwSrvrStmt is NULL */
    if(!CmwSrvrStmt || sqlcode == SQL_INVALID_HANDLE)
    {
        kdsCreateSQLErrorException(SQLError, 1, bSQLMessageSet);
        kdsCopySQLErrorExceptionAndRowCount(SQLError,
                "Internal Error: From Commit Rowsets, getInternalSrvrStmt() failed to get the prepared \"STMT_COMMIT_1\" statement",
                sqlcode,
                "HY000",
                currentRowCount+1);
    }

    // This should be changed to use the rowsets Execute() function once the code is checked in
    retcode = CmwSrvrStmt->Execute(NULL,1,TYPE_UNKNOWN,&inValueList,SQL_ASYNC_ENABLE_OFF,0, /* */NULL);
    if (retcode == SQL_ERROR)
    {
        ERROR_DESC_def *error_desc_def = CmwSrvrStmt->sqlError.errorList._buffer;
        if (CmwSrvrStmt->sqlError.errorList._length != 0 )
        {
            if(error_desc_def->sqlcode != -8605 )
            {
                kdsCreateSQLErrorException(SQLError, 1, bSQLMessageSet);
                kdsCopySQLErrorExceptionAndRowCount(SQLError, error_desc_def->errorText, error_desc_def->sqlcode, error_desc_def->sqlstate, currentRowCount+1);
            }
            else
                retcode = SQL_SUCCESS;
        }
        else
        {
            kdsCreateSQLErrorException(SQLError, 1, bSQLMessageSet);
            kdsCopySQLErrorExceptionAndRowCount(SQLError, "Internal Error: From Commit Rowsets ", retcode, "", currentRowCount+1);
        }
    }
    else if (retcode != SQL_SUCCESS)
    {
        kdsCreateSQLErrorException(SQLError, 1, bSQLMessageSet);
        kdsCopySQLErrorExceptionAndRowCount(SQLError, "Internal Error: From Commit Rowsets ", retcode, "", currentRowCount+1);
    }

    return retcode;
}

