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
// MODULE: SQLMXPreparedStatement.cpp
//

#include <platform_ndcs.h>
#include <sql.h>
#include <sqlext.h>
#include "CoreCommon.h"
#include "JdbcDriverGlobal.h"
#include "org_apache_trafodion_jdbc_t2_SQLMXPreparedStatement.h"
#include "SQLMXCommonFunctions.h"
#ifdef _FASTPATH
#include "CSrvrStmt.h"
#include "SrvrCommon.h"
#endif
#include "Debug.h"


JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXPreparedStatement_prepare
(JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,
 jint txid, jboolean autoCommit, jstring stmtLabel, jstring sql, jboolean isSelect,
 jint queryTimeout, jint holdability, jint jbatchSize, jint jfetchSize)
{
		int batchSize = jbatchSize;
		int fetchSize = jfetchSize;
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXPreparedStatement_prepare",("..., isSelect=%d, batchSize=%ld, fetchSize=%ld, ...",
		isSelect,
		batchSize,
		fetchSize));

	long					estimatedCost;
	long					inputParamOffset;
	ERROR_DESC_LIST_def			sqlWarning;
	SQLItemDescList_def			outputDesc;
	SQLItemDescList_def			inputDesc;
	jint						currentTxid = txid;
	jbyteArray					sqlByteArray;
	jboolean					isCopy;
	jsize						len;
	SQLValue_def				sqlString;

	const char					*nSql = NULL;
	const char					*nStmtLabel = NULL;
	long						stmtId;

	ExceptionStruct	exception_;
	CLEAR_EXCEPTION(exception_);

	sqlString.dataCharset = 0;
	sqlString.dataInd = 0;
	sqlString.dataType = SQLTYPECODE_VARCHAR;

	if (sql)
	{
		if ((sqlByteArray = (jbyteArray)jenv->CallObjectMethod(sql, gJNICache.getBytesMethodId)) == NULL)
		{
			throwSQLException(jenv, INVALID_SQL_STRING_ERROR, NULL, "HY090");
			FUNCTION_RETURN_VOID(("CallObjectMethod() Failed"));
		}
		if ((nSql = (const char *)JNI_GetByteArrayElements(jenv,sqlByteArray, &isCopy)) == NULL)
		{
			throwSQLException(jenv, INVALID_SQL_STRING_ERROR, NULL, "HY090");
			FUNCTION_RETURN_VOID(("GetByteArrayElements() Failed"));
		}
		len = JNI_GetArrayLength(jenv,sqlByteArray);
		//Start Soln. No.: 10-091103-5969 --- commented this fix
		//sqlString.dataValue._buffer = new unsigned char [len+1];
		//memset(sqlString.dataValue._buffer,'\0',len+1);
		//strncpy(sqlString.dataValue._buffer,nSql,len);
		sqlString.dataValue._buffer = (unsigned char *)nSql;
		//End Soln. No.: 10-091103-5969
		sqlString.dataValue._length = len;
	}
	else
	{
		throwSQLException(jenv, INVALID_SQL_STRING_ERROR, NULL, "HY090");
		FUNCTION_RETURN_VOID(("Null SQL string"));
	}

	if (stmtLabel)
		nStmtLabel = JNI_GetStringUTFChars(jenv,stmtLabel, NULL);
	else
	{
		throwSQLException(jenv, INVALID_STMT_LABEL_ERROR, NULL, "HY000");
		FUNCTION_RETURN_VOID(("Null Statement Label"));
	}

	// Note: WLI V31 RFE, see Sol: 10-040311-4065 -
	// Resolve an 'Invalid Transaction state' error when the App server
	// switches to external transaction mode without committing the
	// transaction. Prior to this RFE prepares where done within a
	// transaction started by either JDBC/MX or the application.

	odbc_SQLSvc_Prepare_sme_(NULL, NULL,
		&exception_,
		dialogueId,
		nStmtLabel,
		"",					// StmtExplainName
		EXTERNAL_STMT,
		&sqlString,
		holdability,
		(isSelect ? TYPE_SELECT : TYPE_UNKNOWN),
		batchSize,
		fetchSize,
		queryTimeout,
		&estimatedCost,
		&inputDesc,
		&outputDesc,
		&sqlWarning,
		&stmtId,
		&inputParamOffset);

	if (sql)
		JNI_ReleaseByteArrayElements(jenv,sqlByteArray, (jbyte *)nSql, JNI_ABORT);
	if (stmtLabel)
		JNI_ReleaseStringUTFChars(jenv,stmtLabel, nStmtLabel);

	switch (exception_.exception_nr)
	{
	case CEE_SUCCESS:
		setPrepareOutputs(jenv, jobj, &inputDesc, &outputDesc, currentTxid, stmtId, inputParamOffset);
		if (sqlWarning._length > 0)
			setSQLWarning(jenv, jobj, &sqlWarning);
		break;
	case odbc_SQLSvc_Prepare_SQLQueryCancelled_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, QUERY_CANCELLED_ERROR, NULL, "HY008",
			exception_.u.SQLQueryCancelled.sqlcode);
		break;
	case odbc_SQLSvc_Prepare_SQLError_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, &exception_.u.SQLError);
		break;
	case odbc_SQLSvc_Prepare_ParamError_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, PROGRAMMING_ERROR, exception_.u.ParamError.ParamDesc, "HY000");
		break;
	case odbc_SQLSvc_Prepare_SQLInvalidHandle_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000", exception_.u.SQLInvalidHandle.sqlcode);
		break;
	case odbc_SQLSvc_Prepare_SQLStillExecuting_exn_:
	case odbc_SQLSvc_Prepare_InvalidConnection_exn_:
	case odbc_SQLSvc_Prepare_TransactionError_exn_:
	default:
		// TFDS - These exceptions should not happen
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000", exception_.exception_nr);
		break;
	}
	FUNCTION_RETURN_VOID((NULL));
}

/* RFE: Batch update improvements
* execute() now contains an additional argument: contBatchOnError
* venu changed dialogueId and stmtId from int to long
*/
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXPreparedStatement_execute
(JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,
 jint txid, jboolean autoCommit,  jint txnMode, jlong stmtId,
 jstring cursorName, jboolean isSelect, jint paramRowCount, jint paramCount,
 jobject paramValues, jint queryTimeout, jboolean isAnyLob, jstring iso88591Encoding,
 jobject resultSet, jboolean contBatchOnError)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXPreparedStatement_execute",
		("...txid=%ld, autoCommit=%s, txnMode=%ld, isSelect=%s, isAnyLob=%s, iso88591Encoding=%s resultSet=0x%08x, contBatchOnError=%s",
		txid,
		DebugBoolStr(autoCommit),
		txnMode,
		DebugBoolStr(isSelect),
		DebugBoolStr(isAnyLob),
		DebugJString(jenv, iso88591Encoding),
		resultSet,
		DebugBoolStr(contBatchOnError)));

	ERROR_DESC_LIST_def	sqlWarning;
	jint				paramRowNumber;
	jint				currentTxid = txid;
	jint				externalTxid = 0;
	const char			*nCursorName = NULL;
	long				sqlcode;
	SRVR_STMT_HDL		*pSrvrStmt;
	//jthrowable			queuedException = NULL;
	jthrowable          exceptionHead = NULL;// RFE: Batch update improvements

	CLEAR_WARNING(sqlWarning);

	if ((pSrvrStmt = getSrvrStmt(dialogueId, stmtId, &sqlcode)) == NULL)
	{
		throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000", sqlcode);
		FUNCTION_RETURN_VOID(("getSrvrStmt() Failed"));
	}

	if (paramRowCount==0)
	{
		// No rows to process.  Return nothing.
		setExecuteOutputs(jenv, jobj, pSrvrStmt, NULL, 0, currentTxid);
		FUNCTION_RETURN_VOID(("paramRowCount==0"));
	}

	SQLValueList_def	outSqlValueList;		// Ouput SQL Value list
	CLEAR_LIST(outSqlValueList);
	SQLValueList_def	inSqlValueList;			// Input SQL Value list
	CLEAR_LIST(inSqlValueList);
	ROWS_COUNT_LIST_def			rowCount;
	CLEAR_LIST(rowCount);
	ExceptionStruct	exception_;
	CLEAR_EXCEPTION(exception_);

	if (cursorName)
		nCursorName = JNI_GetStringUTFChars(jenv,cursorName, NULL);

	// Save the result set for the returned row data
	pSrvrStmt->resultSetObject = resultSet;

	MEMORY_ALLOC_ARRAY(rowCount._buffer,int,paramRowCount);
	memset(rowCount._buffer,0,paramRowCount*sizeof(int));
	rowCount._length = 0;
	pSrvrStmt->totalRowCount = 0;

	// If execute call fails, will be set to failure.
	// All other failures are handled by success processing.
	exception_.exception_nr = CEE_SUCCESS;

	jint errorRow = 0;
	jint rowsExecuted = 0;
	long totalRows;
	short txn_status;

	/* RFE: Batch update improvements
	* If contBatchOnError is true, execution of batch commands continues even on errors
	*/
	// Try to resume or start the transaction
	//10-091005-5100
	if ((txn_status = beginTxnControl(jenv, currentTxid, externalTxid, txnMode, -1)) != 0)
	{
		DEBUG_OUT(DEBUG_LEVEL_ENTRY,("beginTxnControl() Failed"));
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwTransactionException(jenv, txn_status);
		// Queue up the exception to be rethrown later
		//queuedException = jenv->ExceptionOccurred();
		if(exceptionHead)
			jenv->CallVoidMethod(exceptionHead, gJNICache.setNextExceptionMethodId,
			jenv->ExceptionOccurred());
		else
			exceptionHead = jenv->ExceptionOccurred();
		jenv->ExceptionClear();
		//break;
		for(int nforX = 0; nforX < paramRowCount; ++nforX)
		{
			rowCount._buffer[nforX] = SQLMXStatement_EXECUTE_FAILED();
			rowCount._length++;
		}
	}
	//10-091005-5100
	for (paramRowNumber = 0;
		paramRowNumber < paramRowCount && txn_status == 0;
		paramRowNumber += rowsExecuted)
	{


		//else
		//{
		// Compute how many rows are left to execute
		totalRows = paramRowCount - paramRowNumber;

		// Setup batch binding
		pSrvrStmt->batchSetup(totalRows);

		// If batch binding is active, block execute the batch size number of statements,
		//    otherwise execute each statement separately.
		if (pSrvrStmt->batchRowsetSize) 
		{
			totalRows = pSrvrStmt->batchRowsetSize;
		}
		else
		{
			totalRows = 1;
		}

		// Fill in the parameter data
		errorRow = fillInSQLValues(jenv, jobj, pSrvrStmt,
			paramRowNumber, totalRows,
			paramCount, paramValues, iso88591Encoding);
		if (errorRow)
		{
			/* RFE: Batch update improvements
			* Queue up the parameter errors if any and
			* set the corresponding statement as EXECUTE_FAILED.
			*/
			if(jenv->ExceptionOccurred())
			{
				if(exceptionHead)
					jenv->CallVoidMethod(exceptionHead, gJNICache.setNextExceptionMethodId,
					jenv->ExceptionOccurred());
				else
					exceptionHead = jenv->ExceptionOccurred();
				jenv->ExceptionClear();

				if(contBatchOnError)
				{
					rowCount._buffer[paramRowNumber] = SQLMXStatement_EXECUTE_FAILED();
					rowCount._length++;
				}
			}

			totalRows = errorRow - 1;

			if (totalRows)
			{
				// Error was not in first row so we need to execute the previous commands.
				pSrvrStmt->batchSetup(totalRows);
				fillInSQLValues(jenv, jobj, pSrvrStmt,
					paramRowNumber, totalRows,
					paramCount, paramValues, iso88591Encoding);
			}
		}

		if (totalRows)
		{
			// Try to execute the statements
			odbc_SQLSvc_ExecuteN_sme_(NULL, NULL,
				&exception_,
				dialogueId,
				stmtId,
				(char *)nCursorName,
				(isSelect ? TYPE_SELECT : TYPE_UNKNOWN),
				totalRows,
				&inSqlValueList,
				FALSE,
				queryTimeout,
				&outSqlValueList,
				&sqlWarning);

			DEBUG_OUT(DEBUG_LEVEL_DATA,("outSqlValueList._buffer=0x%08x, outSqlValueList._length=0x%08x",
				outSqlValueList._buffer,
				outSqlValueList._length));

			if (pSrvrStmt->rowCount._length)
			{
				// Row count information returned.  Append to existing row count information.
				rowsExecuted = pSrvrStmt->rowCount._length;
				rowCount._length += pSrvrStmt->rowCount._length;
				DEBUG_ASSERT(rowCount._length<=paramRowCount,
					("rowCount._length(%ld)>paramRowCount(%ld)", rowCount._length, paramRowCount));
				memcpy(rowCount._buffer+paramRowNumber,
					pSrvrStmt->rowCount._buffer,
					sizeof(rowCount._buffer[0]) * pSrvrStmt->rowCount._length);
			}
			else if(contBatchOnError && paramRowCount > 2)
			{

				rowsExecuted = totalRows;
			}
			else 
			{
				rowsExecuted = 1;
			}
		}
		//}
		// Process the execute result.
		switch (exception_.exception_nr)
		{
		case CEE_SUCCESS:
			if (sqlWarning._length > 0) setSQLWarning(jenv, jobj, &sqlWarning);
			/* RFE: Batch update improvements
			* Any queued exception will now be thrown in the end.
			*/
			break;
		case odbc_SQLSvc_ExecuteN_SQLNoDataFound_exn_:
			jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidRSMethodId, currentTxid);
			break;
		case odbc_SQLSvc_ExecuteN_SQLQueryCancelled_exn_:
			jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
			throwSQLException(jenv, QUERY_CANCELLED_ERROR, NULL, "HY008",
				exception_.u.SQLQueryCancelled.sqlcode);
			break;
		case odbc_SQLSvc_ExecuteN_SQLError_exn_:
		case odbc_SQLSvc_ExecuteN_SQLRetryCompile_exn_:
			jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
			throwSQLException(jenv, &exception_.u.SQLError);
			break;
		case odbc_SQLSvc_ExecuteN_ParamError_exn_:
			jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
			throwSQLException(jenv, PROGRAMMING_ERROR, exception_.u.ParamError.ParamDesc, "HY000");
			break;
		case odbc_SQLSvc_ExecuteN_SQLInvalidHandle_exn_:
			jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
			throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000",
				exception_.u.SQLInvalidHandle.sqlcode);
			break;
		case odbc_SQLSvc_ExecuteN_SQLStillExecuting_exn_:
		case odbc_SQLSvc_ExecuteN_InvalidConnection_exn_:
		case odbc_SQLSvc_ExecuteN_TransactionError_exn_:
		case odbc_SQLSvc_ExecuteN_SQLNeedData_exn_:
		default:
			// TFDS - These exceptions should not happen
			jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
			throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000", exception_.exception_nr);
			break;
		}

		if(jenv->ExceptionOccurred())
		{
			if(exceptionHead)
				jenv->CallVoidMethod(exceptionHead, gJNICache.setNextExceptionMethodId,
				jenv->ExceptionOccurred());
			else
				exceptionHead = jenv->ExceptionOccurred();

			jenv->ExceptionClear();

			if(totalRows > 2)
			{
				for(int nforX = 0; nforX < totalRows; ++nforX)
				{
					rowCount._buffer[paramRowNumber+nforX] = SQLMXStatement_EXECUTE_FAILED();
					rowCount._length++;
				}
			}
			else
			{
				rowCount._buffer[paramRowNumber] = SQLMXStatement_EXECUTE_FAILED();
				rowCount._length++;
			}
		}
		/* RFE: Batch update improvements
		* If contBatchOnError is not enabled and if any exceptions have
		* occured, stop processing the batch.
		*/
		if (!contBatchOnError && (exceptionHead || (exception_.exception_nr!=CEE_SUCCESS)))
		{
			break;
		}
		if(errorRow)
		{
			rowsExecuted = errorRow;
		}
	}//end of for

	/* RFE: Batch update improvements
	* If contBatchOnError is true, CEE_SUCCESS is always passed instead of
	* exception_.exception_nr, so that endTxnControl suspends the transaction.
	*/
	//10-091005-5100
	if ((txn_status = endTxnControl(jenv, currentTxid, txid, autoCommit,
		(exception_.exception_nr), isSelect, txnMode, externalTxid)) != 0)
	{
		DEBUG_OUT(DEBUG_LEVEL_ENTRY,("endTxnControl() Failed"));
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwTransactionException(jenv, txn_status);
		// Queue up the exception to be rethrown later
		//queuedException = jenv->ExceptionOccurred();
		if(exceptionHead)
			jenv->CallVoidMethod(exceptionHead, gJNICache.setNextExceptionMethodId,
			jenv->ExceptionOccurred());
		else
			exceptionHead = jenv->ExceptionOccurred();

		jenv->ExceptionClear();
		//break;
	}

	// Free up the cursor name
	if (cursorName)
		JNI_ReleaseStringUTFChars(jenv,cursorName, nCursorName);

	// If there was a parameter error, return null row counts.
	// Note: This is not how ODBC handles errors.  This handling many need to be changed
	//	   in the future to match ODBC (return a row count array with error rows).  The
	//	   current JDBC implementation is designed to make the change to the ODBC implemenation
	//	   easier if required.

	/* RFE: Batch update improvements
	* Now returns the row count array with error rows.
	*/
	setExecuteOutputs(jenv, jobj, pSrvrStmt, &rowCount, pSrvrStmt->totalRowCount, currentTxid);

	/* RFE: Batch update improvements
	* Throw the queued exceptions if any
	*/
	if(exceptionHead)
		jenv->Throw(exceptionHead);

	MEMORY_DELETE(rowCount._buffer);
	FUNCTION_RETURN_VOID((NULL));
}

JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXPreparedStatement_resetFetchSize
  (JNIEnv *, jobject, jlong dialogueId, jlong stmtId, jint fetchSize)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXPreparedStatement_resetFetchSize",
		("... dialogueId=0x%08x, , stmtId=%ld, fetchSize=%ld",
		dialogueId,
		stmtId,
		fetchSize));

	SRVR_STMT_HDL::resetFetchSize(dialogueId, stmtId, fetchSize);
	FUNCTION_RETURN_VOID((NULL));
}
