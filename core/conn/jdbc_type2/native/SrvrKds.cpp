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
// MODULE: SrvrKds.cpp
//
// PURPOSE: Implements the functions to populate the output structures
//

#include <platform_ndcs.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>
#include "sqlcli.h"
//#include "SrvrFunctions.h"	// Linux port - Todo
#include "SrvrCommon.h"
#include "CommonDiags.h"
#include "Debug.h"

/* ************************************************************************
 * Used in SqlInterface.cpp by EXECDIRECT, PREPARE_FROM_MODULE and PREPARE.
 * ************************************************************************ */

bool g_bSqlMessageSet = false;

void kdsCreateSQLDescSeq(SQLItemDescList_def *SQLDesc, short numEntries)
{
	FUNCTION_ENTRY("kdsCreateSQLDescSeq",("SQLDesc=0x%08x, numEntries=%d",
		SQLDesc,
		numEntries));

	if (numEntries == 0)
	{
		DEBUG_ASSERT(SQLDesc->_buffer==NULL,("SQLDesc->_buffer!=NULL (0x%08x",SQLDesc->_buffer));
		SQLDesc->_length = 0;
		SQLDesc->_buffer = 0;
	}
	else
	{
		MEMORY_ALLOC_ARRAY(SQLDesc->_buffer, SQLItemDesc_def, numEntries);
		SQLDesc->_length = 0;
	}
	FUNCTION_RETURN_VOID((NULL));
}

/* ************************************************************************
 * Used in SqlInterface.cpp by EXECDIRECT, PREPARE_FROM_MODULE and PREPARE.
 * ************************************************************************ */

void kdsCreateEmptySQLDescSeq(SQLItemDescList_def *SQLDesc)
{
	FUNCTION_ENTRY("kdsCreateEmptySQLDescSeq",("SQLDesc=0x%08x",
		SQLDesc));
	DEBUG_ASSERT(SQLDesc->_buffer==NULL,("SQLDesc->_buffer!=NULL"));
	SQLDesc->_length = 0;
	SQLDesc->_buffer = NULL;
	FUNCTION_RETURN_VOID((NULL));
}

/* ************************************************************************
 * Used in CSrvrStmt.cpp by ControlProc and Execute.
 * Used in SqlInterface.cpp by GETSQLERROR.
 * Used in SrvrCommon.cpp by do_ExecFetchAppend and do_ExecSMD.
 * Used in SrvrOther.cpp by odbc_SQLSvc_PrepareFromModule_sme.
 * ************************************************************************ */

void kdsCreateSQLErrorException(odbc_SQLSvc_SQLError *SQLError,
								long numConditions, bool &bSQLMessageSet)
{
	FUNCTION_ENTRY("kdsCreateSQLErrorException",("SQLError=0x%08x,numConditions=%ld",SQLError,numConditions));
	DEBUG_ASSERT(SQLError!=NULL,("SQLError is NULL"));
	DEBUG_ASSERT(SQLError->errorList._buffer==NULL,
		("SQLError->errorList._buffer is not NULL (0x%08x)",SQLError->errorList._buffer));

	if (numConditions == 0)
	{
		SQLError->errorList._buffer = NULL;
		SQLError->errorList._length = 0;
	}
	else
	{
		bSQLMessageSet = true;
		MEMORY_ALLOC_ARRAY(SQLError->errorList._buffer, ERROR_DESC_def, numConditions);
		memset(SQLError->errorList._buffer,0,numConditions*sizeof(ERROR_DESC_def));
		SQLError->errorList._length = 0;
	}
	FUNCTION_RETURN_VOID((NULL));
}

/* ************************************************************************
 * Used in CSrvrStmt.cpp by ControlProc and Execute.
 * Used in SqlInterface.cpp by GETSQLERROR.
 * Used in SrvrCommon.cpp by do_ExecFetchAppend and do_ExecSMD.
 * Used in SrvrOther.cpp by odbc_SQLSvc_PrepareFromModule_sme.
 * ************************************************************************ */

void kdsCopySQLErrorException(odbc_SQLSvc_SQLError *SQLError,
							char *msg_buf,
							long sqlcode,
							char *sqlState)
{
	FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ERROR,"kdsCopySQLErrorException",("SQLError=0x%08x,msg_buf='%s',sqlcode=%ld,sqlState='%s'",
		SQLError,
		msg_buf,
		sqlcode,
		sqlState));

	ERROR_DESC_def *SqlErrorDesc;

	SqlErrorDesc = (ERROR_DESC_def *)SQLError->errorList._buffer + SQLError->errorList._length;
	DEBUG_OUT(DEBUG_LEVEL_ERROR,("SqlErrorDesc = SQLError->errorList._buffer[%ld]",SQLError->errorList._length));

	// Save the message
	if (msg_buf)
	{
		// Allocate String Buffer
		MEMORY_ALLOC_ARRAY(SqlErrorDesc->errorText, char, strlen(msg_buf)+1);
		strcpy(SqlErrorDesc->errorText,msg_buf);
	} else SqlErrorDesc->errorText = NULL;
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
	SqlErrorDesc->rowId = 0;
	SqlErrorDesc->errorDiagnosticId = 0;
	SqlErrorDesc->operationAbortId = 0;
	SQLError->errorList._length++;

	FUNCTION_RETURN_VOID((NULL));
}


/* ************************************************************************
 * Used in SqlInterface.cpp by FETCH.
 * ************************************************************************ */

void kdsCopyToSQLValueSeq(SQLValueList_def *SQLValueList,
						 long dataType,
						 short indValue,
						 void *varPtr,
						 long allocLength,
						 long Charset)
{
	FUNCTION_ENTRY("kdsCopyToSQLValueSeq",("SQLValueList=0x%08x, dataType=%s, indValue=%d, varPtr=0x%08x, allocLength=%ld, Charset=%s",
		SQLValueList,
		CliDebugSqlTypeCode(dataType),
		indValue,
		varPtr,
		allocLength,
		getCharsetEncoding(Charset)));
	SQLValue_def *SQLValue = (SQLValue_def *)SQLValueList->_buffer + SQLValueList->_length;
	DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLValueList->_buffer[%ld]=0x%08x",
		SQLValueList->_length,
		SQLValue));
	SQLValue->dataInd = indValue;
	SQLValue->dataType = dataType;
	SQLValue->dataCharset = Charset;
	if (indValue == 0)
	{
		SQLValue->dataValue._length = allocLength;
		memcpy(SQLValue->dataValue._buffer, varPtr, allocLength);
		DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLValue->dataValue: _buffer=0x%08x _length=%ld",
			SQLValue->dataValue._buffer,
			SQLValue->dataValue._length));
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
	FUNCTION_RETURN_VOID((NULL));
}

void kdsCopySQLErrorExceptionAndRowCount(
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
	if (msg_buf)
	{
        MEMORY_ALLOC_ARRAY(SqlErrorDesc->errorText, char, strlen(msg_buf)+1);
        strcpy(SqlErrorDesc->errorText, msg_buf);
    }
    else
        SqlErrorDesc->errorText = NULL;
	
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

