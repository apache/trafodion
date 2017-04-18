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
// MODULE: SQLMXStatement.cpp
//
#include <platform_ndcs.h>
#ifdef NSK_PLATFORM
	#include <sqlWin.h>
	#include <windows.h>
#else
	#include <sql.h>
#endif
#include <sqlext.h>
#include "JdbcDriverGlobal.h"
#include "CoreCommon.h"
#include "org_apache_trafodion_jdbc_t2_SQLMXStatement.h"
#include "SQLMXCommonFunctions.h"
#include "CSrvrConnect.h"
#include "Debug.h"

JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXStatement_executeDirect
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jint txid, jboolean autoCommit, jint txnMode, 
  jstring stmtLabel, jstring cursorName, jstring sql, jboolean isSelect, jint queryTimeout,
  jint holdability, jobject resultSet, jlong currentStmtId)
{
	FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_STMT,"Java_org_apache_trafodion_jdbc_t2_SQLMXStatement_executeDirect",
		("... dialogueId=0x%08x, txid=%ld, autoCommit=%s, stmtLabel=%s, cursorName=%s, sql=%s, txnMode=%ld, isSelect=%s, holdability=%ld, resultSet=0x%08x",
		dialogueId,
		txid,
		DebugBoolStr(autoCommit),
		DebugJString(jenv,stmtLabel),
		DebugJString(jenv,cursorName),
		DebugJString(jenv,sql),
		txnMode,
		DebugBoolStr(isSelect),
		holdability,
		resultSet));

	ExceptionStruct				exception_;
	long					estimatedCost;
	long					rowsAffected;
	ERROR_DESC_LIST_def			sqlWarning;
	SQLItemDescList_def			outputDesc;
	jint						currentTxid = txid;
	jint						externalTxid = 0;
	long						stmtId;

	const char		*nSql;
	const char		*nStmtLabel;
	const char		*nCursorName;
	jbyteArray		sqlByteArray;
	jboolean		isCopy;
	jsize			len;
	SQLValue_def	sqlString;
	short			txn_status;

	sqlString.dataCharset = 0;
	sqlString.dataInd = 0;
	sqlString.dataType = SQLTYPECODE_VARCHAR;
	
	if (sql)
	{
		if ((sqlByteArray = (jbyteArray)jenv->CallObjectMethod(sql, gJNICache.getBytesMethodId)) 
				== NULL)
		{
			throwSQLException(jenv, INVALID_SQL_STRING_ERROR, NULL, "HY090");
			FUNCTION_RETURN_VOID(("getBytesMethod returned NULL"));
		}
		if ((nSql = (const char *)JNI_GetByteArrayElements(jenv,sqlByteArray, &isCopy)) == NULL)
		{
			throwSQLException(jenv, INVALID_SQL_STRING_ERROR, NULL, "HY090");
			FUNCTION_RETURN_VOID(("GetByteArrayElements returned NULL"));
		}
		len = JNI_GetArrayLength(jenv,sqlByteArray);
		sqlString.dataValue._buffer = (unsigned char *)nSql;
		sqlString.dataValue._length = len;
		DEBUG_OUT(DEBUG_LEVEL_CLI,("sqlString = '%s', length =%ld", nSql, len));
		MEMORY_DUMP(DEBUG_LEVEL_CLI, nSql, len);
	}
	else
	{
		throwSQLException(jenv, INVALID_SQL_STRING_ERROR, NULL, "HY090");
		FUNCTION_RETURN_VOID(("sql is NULL"));
	}

	if (stmtLabel)
		nStmtLabel = JNI_GetStringUTFChars(jenv,stmtLabel, NULL);
	else
	{
		throwSQLException(jenv, INVALID_STMT_LABEL_ERROR, NULL, "HY000");
		FUNCTION_RETURN_VOID(("stmtLabel is NULL"));
	}

	if (cursorName)
		nCursorName = JNI_GetStringUTFChars(jenv,cursorName, NULL);
	else
		nCursorName = NULL;

	if ((txn_status = beginTxnControl(jenv, currentTxid, externalTxid, txnMode, -1) != 0))
	{
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwTransactionException(jenv, txn_status);
		FUNCTION_RETURN_VOID(("beginTxnControl() failed"));
	}

	exception_.u.SQLError.errorList._buffer = NULL;
	odbc_SQLSvc_ExecDirect_sme_(NULL, NULL, 
			&exception_,
			dialogueId,
			nStmtLabel,
			(char *)nCursorName,
			"",					// StmtExplainName
			EXTERNAL_STMT,
			(isSelect ? TYPE_SELECT : TYPE_UNKNOWN),
			&sqlString,
			holdability,
			queryTimeout,
			(long) resultSet,
			&estimatedCost,
			&outputDesc,
			&rowsAffected,
			&sqlWarning,
			&stmtId,
			currentStmtId);

	if (sql)
		JNI_ReleaseByteArrayElements(jenv,sqlByteArray, (jbyte *)nSql, JNI_ABORT);
	if (stmtLabel)
		JNI_ReleaseStringUTFChars(jenv,stmtLabel, nStmtLabel);
	if (cursorName)
		JNI_ReleaseStringUTFChars(jenv,cursorName, nCursorName);

	if ((txn_status = endTxnControl(jenv, currentTxid, txid, autoCommit, 
						exception_.exception_nr, isSelect, txnMode, externalTxid)) != 0)
	{
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwTransactionException(jenv, txn_status);
		FUNCTION_RETURN_VOID(("endTxnControl() Failed"));
	}
	
	switch (exception_.exception_nr)
	{
	case CEE_SUCCESS:
		if (sqlWarning._length != 0)
			setSQLWarning(jenv, jobj, &sqlWarning);
		setExecuteDirectOutputs(jenv, jobj, &outputDesc, rowsAffected, currentTxid, stmtId);
		break;
	case odbc_SQLSvc_ExecDirect_ParamError_exn_:
	    jenv->CallVoidMethod(jobj, gJNICache.setCurrentStmtIdMethodId, stmtId);
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, PROGRAMMING_ERROR, exception_.u.ParamError.ParamDesc, "HY000");
		break;
	case odbc_SQLSvc_ExecDirect_SQLError_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentStmtIdMethodId, stmtId);
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, &exception_.u.SQLError);
		break;
	case odbc_SQLSvc_ExecDirect_SQLQueryCancelled_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentStmtIdMethodId, stmtId);
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, QUERY_CANCELLED_ERROR, NULL, "HY008", 
			exception_.u.SQLQueryCancelled.sqlcode);
		break;
	case odbc_SQLSvc_ExecDirect_SQLInvalidHandle_exn_:
	    jenv->CallVoidMethod(jobj, gJNICache.setCurrentStmtIdMethodId, stmtId);
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000", exception_.u.SQLInvalidHandle.sqlcode);
		break;
	case odbc_SQLSvc_ExecDirect_SQLStillExecuting_exn_:
	case odbc_SQLSvc_ExecDirect_InvalidConnection_exn_:
	case odbc_SQLSvc_ExecDirect_TransactionError_exn_:
	default:
// TFDS - These error should not happen		
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentStmtIdMethodId, stmtId);
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000", exception_.exception_nr);
		break;
	}
	FUNCTION_RETURN_VOID((NULL));
}

//venu changed dialogueId and stmtId from int to long for 64 bit
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXStatement_executeRS
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jint txid, jboolean autoCommit, jint txnMode,
  jstring stmtLabel, jstring RSstmtLabel,  jboolean isSelect, jlong stmtId, jint ResultSetIndex, jobject resultSet)
{
	FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_STMT, "Java_org_apache_trafodion_jdbc_t2_SQLMXStatement_executeRS",
	("...dialogueId=0x%08x, txid=0x%08x, stmtLabel=%s, RSstmtLabel=%s, isSelect=%s, stmtId=0x%08x, ResultSetIndex=%ld, resultSet=0x%08x",
        dialogueId,
		txid,
		DebugJString(jenv,stmtLabel),
		DebugJString(jenv,RSstmtLabel),
		DebugBoolStr(isSelect),
		stmtId,
		ResultSetIndex,
		resultSet));

	ExceptionStruct				exception_;
	ERROR_DESC_LIST_def			sqlWarning;
	SQLItemDescList_def			outputDesc;
	jint						currentTxid = txid;
	jint						externalTxid = 0;
	long						RSstmtId;
	const char					*nStmtLabel;
	const char					*nRSStmtLabel;
	jboolean					isCopy;
	short						txn_status;

	if (stmtLabel)
		nStmtLabel = JNI_GetStringUTFChars(jenv,stmtLabel, NULL);
	else
	{
		throwSQLException(jenv, INVALID_STMT_LABEL_ERROR, NULL, "HY000");
		FUNCTION_RETURN_VOID(("stmtLabel is NULL"));
	}
	if (RSstmtLabel)
		nRSStmtLabel = JNI_GetStringUTFChars(jenv,RSstmtLabel, NULL);
	else
	{
		throwSQLException(jenv, INVALID_STMT_LABEL_ERROR, NULL, "HY000");
		FUNCTION_RETURN_VOID(("RSstmtLabel is NULL"));
	}

	if ((txn_status = beginTxnControl(jenv, currentTxid, externalTxid, txnMode, -1) != 0))
	{
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwTransactionException(jenv, txn_status);
		FUNCTION_RETURN_VOID(("beginTxnControl() failed"));
	}

	jdbc_SQLSvc_ExecSPJRS_sme_(NULL, NULL, 
			&exception_,
			dialogueId,
			nStmtLabel,
			nRSStmtLabel,
			EXTERNAL_STMT,
			(isSelect ? TYPE_SELECT : TYPE_UNKNOWN),
			(long) resultSet,
			ResultSetIndex,
			&outputDesc,
			&sqlWarning,
			&RSstmtId,
			stmtId);

	if (stmtLabel)
		JNI_ReleaseStringUTFChars(jenv, stmtLabel, nStmtLabel);

	if (RSstmtLabel)
		JNI_ReleaseStringUTFChars(jenv, RSstmtLabel, nRSStmtLabel);

	if ((txn_status = endTxnControl(jenv, currentTxid, txid, autoCommit, 
						exception_.exception_nr, isSelect, txnMode, externalTxid)) != 0)
	{
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwTransactionException(jenv, txn_status);
		FUNCTION_RETURN_VOID(("endTxnControl() Failed"));
	}
	
	switch (exception_.exception_nr)
	{
	case CEE_SUCCESS:
		if (sqlWarning._length != 0)
			setSQLWarning(jenv, jobj, &sqlWarning);
		setExecuteRSOutputs(jenv, jobj, &outputDesc, currentTxid, RSstmtId, ResultSetIndex);
		break;
	case jdbc_SQLSvc_ExecSPJRS_ParamError_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, PROGRAMMING_ERROR, exception_.u.ParamError.ParamDesc, "HY000");
		break;
	case jdbc_SQLSvc_ExecSPJRS_SQLError_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, &exception_.u.SQLError);
		break;
	case jdbc_SQLSvc_ExecSPJRS_SQLQueryCancelled_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, QUERY_CANCELLED_ERROR, NULL, "HY008", exception_.u.SQLQueryCancelled.sqlcode);
		break;
	case jdbc_SQLSvc_ExecSPJRS_SQLInvalidHandle_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000", exception_.u.SQLInvalidHandle.sqlcode);
		break;
	case jdbc_SQLSvc_ExecSPJRS_SQLStillExecuting_exn_:
	case jdbc_SQLSvc_ExecSPJRS_InvalidConnection_exn_:
	case jdbc_SQLSvc_ExecSPJRS_TransactionError_exn_:
	default:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000", exception_.exception_nr);
		break;
	}
	FUNCTION_RETURN_VOID((NULL));
}

//venu changed dialogueId and stmtId from int to long for 64 bit
JNIEXPORT jint JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXStatement_close
  (JNIEnv *jenv, jclass jclass, jstring server, jlong dialogueId, 
		jlong stmtId, jboolean dropStmt)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXStatement_close",
		("... dialogueId=0x%08x, stmtId=%ld, dropStmt=%s",
		dialogueId,
		stmtId,
		DebugBoolStr(dropStmt)));

	ExceptionStruct				exception_;
	long					rowsAffected;
	ERROR_DESC_LIST_def			sqlWarning;
	jint						retcode = -1;		// 0 - Success, -1 = failure 
															// 1= Cursor Not Found
	ERROR_DESC_def				*error_desc_def;

	odbc_SQLSvc_Close_sme_(NULL, NULL, &exception_,
					dialogueId,
					stmtId,
					(dropStmt ? SQL_DROP : SQL_CLOSE),
					&rowsAffected,
					&sqlWarning);
	
	// Ignore setting warning since the input is jclass and not jobject
	switch (exception_.exception_nr)
	{
	case CEE_SUCCESS:
		retcode = 0;
		break;
	case odbc_SQLSvc_Close_SQLError_exn_:
		error_desc_def =((ERROR_DESC_LIST_def *)(&exception_.u.SQLError.errorList))->_buffer;
		if (error_desc_def->sqlcode == -8811)		// Close a non-existant cursor)
			retcode = 1;
		break;
	case odbc_SQLSvc_Close_ParamError_exn_:
		throwSQLException(jenv, PROGRAMMING_ERROR, exception_.u.ParamError.ParamDesc, "HY000");
		break;
	case odbc_SQLSvc_Close_InvalidConnection_exn_:
	case odbc_SQLSvc_Close_TransactionError_exn_:
	default:
	// TFDS - This exceptions should not happen
		throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000", exception_.exception_nr);
		break;
	}
	FUNCTION_RETURN_NUMERIC(retcode,(NULL));
}
/* RFE: Batch update improvements
 * executeBatch() now contains an additional argument: contBatchOnError
 */
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXStatement_executeBatch
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jint txid, jboolean autoCommit, jint txnMode,  
  jstring stmtLabel, jstring cursorName, jobjectArray sqlCommands, jboolean isSelect, jint queryTimeout, jboolean contBatchOnError, jlong currentStmtId)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXStatement_executeBatch",
		("... dialogueId=0x%08x, txid=%ld, autoCommit=%s, stmtLabel=%s, cursorName=%s,txnMode=%ld, isSelect=%s,contBatchOnError=%s ...",
		dialogueId,
		txid,
		DebugBoolStr(autoCommit),
		DebugJString(jenv,stmtLabel),
		DebugJString(jenv,cursorName),
		txnMode,
		DebugBoolStr(isSelect),
		DebugBoolStr(contBatchOnError)));

	ExceptionStruct				exception_;
	long						estimatedCost;
	long						rowsAffected;
	ERROR_DESC_LIST_def			sqlWarning;
	SQLItemDescList_def			outputDesc;
	jint						currentTxid = txid;
	jint						externalTxid = 0;
	jsize						noOfCommands;
	jstring						sql;
	jint						i;

	const char		*nSql;
	const char		*nStmtLabel;
	const char		*nCursorName = NULL;
	jbyteArray		sqlByteArray;
	jboolean		isCopy;
	long			stmtId;
	jsize			len;
	SQLValue_def	sqlString;
	short			txn_status;	
	// RFE: Batch update improvements
	jthrowable		queuedException = NULL;
	jthrowable		exceptionHead = NULL;
	bool			isSuspended = false;

	exception_.exception_nr = CEE_SUCCESS;

	if (stmtLabel)
		nStmtLabel = JNI_GetStringUTFChars(jenv,stmtLabel, NULL);
	else
	{
		throwSQLException(jenv, INVALID_STMT_LABEL_ERROR, NULL, "HY000");
		FUNCTION_RETURN_VOID(("stmtLabel is NULL"));
	}

	if (cursorName) nCursorName = JNI_GetStringUTFChars(jenv,cursorName, NULL);
	
	if ((txn_status = beginTxnControl(jenv, currentTxid, externalTxid, txnMode, -1) != 0))
	{
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwTransactionException(jenv, txn_status);
		FUNCTION_RETURN_VOID(("beginTxnControl() failed"));
	}

	sqlString.dataCharset = 0;
	sqlString.dataInd = 0;
	sqlString.dataType = SQLTYPECODE_VARCHAR;
	
	noOfCommands = JNI_GetArrayLength(jenv,sqlCommands);

	for (i = 0; i < noOfCommands ; i++)
	{
		/* RFE: Batch update improvements
		 * Resume the transaction if it was earlier suspended, by checking the
		 * variable isSuspended. This is reset to false after the transaction is resumed.
		 */
		if(isSuspended)
		{
			if ((txn_status = beginTxnControl(jenv, currentTxid, externalTxid, txnMode, -1) != 0))
			{
				jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
				throwTransactionException(jenv, txn_status);
				FUNCTION_RETURN_VOID(("beginTxnControl() failed"));
			}
			isSuspended = false;
		}
		sql = (jstring) JNI_GetObjectArrayElement(jenv,sqlCommands, i);
		if (sql)
		{
			if ((sqlByteArray = (jbyteArray)jenv->CallObjectMethod(sql, gJNICache.getBytesMethodId))
					== NULL)
			{
				throwSQLException(jenv, INVALID_SQL_STRING_ERROR, NULL, "HY090");
				FUNCTION_RETURN_VOID(("getBytesMethod() returned NULL"));
			}
			if ((nSql = (const char *)JNI_GetByteArrayElements(jenv,sqlByteArray, &isCopy)) == NULL)
			{
				throwSQLException(jenv, INVALID_SQL_STRING_ERROR, NULL, "HY090");
				FUNCTION_RETURN_VOID(("GetByteArrayElements() returned NULL"));
			}
			len = JNI_GetArrayLength(jenv,sqlByteArray);
			sqlString.dataValue._buffer = (unsigned char *)nSql;
			sqlString.dataValue._length = len;
		}
		else
		{
			throwSQLException(jenv, INVALID_SQL_STRING_ERROR, NULL, "HY090");
			FUNCTION_RETURN_VOID(("sql is NULL"));
		}

		odbc_SQLSvc_ExecDirect_sme_(NULL, NULL, 
				&exception_,
				dialogueId,
				nStmtLabel,
				(char *)nCursorName,
				"",					// StmtExplainName
				EXTERNAL_STMT,
				(isSelect ? TYPE_SELECT : TYPE_UNKNOWN),
				&sqlString,
				CLOSE_CURSORS_AT_COMMIT,
				queryTimeout,
				NULL,
				&estimatedCost,
				&outputDesc,
				&rowsAffected,
				&sqlWarning,
				&stmtId
				,currentStmtId);

		if (sql)
			JNI_ReleaseByteArrayElements(jenv,sqlByteArray, (jbyte *)nSql, JNI_ABORT);
		
		/* RFE: Batch update improvements
		 * Perform pre-function exit processing only if contBatchOnError is not set to true
		 */		
		if (exception_.exception_nr != CEE_SUCCESS)
		{
			//RFE: Batch update improvements
			if(!contBatchOnError)
			{
				if (stmtLabel)
					JNI_ReleaseStringUTFChars(jenv,stmtLabel, nStmtLabel);
				if (cursorName)
					JNI_ReleaseStringUTFChars(jenv,cursorName, nCursorName);
			}

			// Commit the transaction so all good statements are processed.			
			txn_status = endTxnControl(jenv, currentTxid, txid, autoCommit, 
									   CEE_SUCCESS, isSelect, txnMode, externalTxid);						
			jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
			if (txn_status != 0)
			{
				throwTransactionException(jenv, txn_status);
				DEBUG_OUT(DEBUG_LEVEL_ENTRY|DEBUG_LEVEL_TXN,("endTxnControl() Failed after ExecDirect failure"));
			}
			//RFE: Batch update improvements
			if(contBatchOnError)
				isSuspended = true;

		}

		switch (exception_.exception_nr)
		{
		case CEE_SUCCESS:
			if (sqlWarning._length != 0)
				setSQLWarning(jenv, jobj, &sqlWarning);
			jenv->CallVoidMethod(jobj, gJNICache.execDirectBatchOutputsMethodId, i, rowsAffected, 
				currentTxid);
			break;
		case odbc_SQLSvc_ExecDirect_ParamError_exn_:
		    jenv->CallVoidMethod(jobj, gJNICache.setCurrentStmtIdMethodId, stmtId);
			throwSQLException(jenv, PROGRAMMING_ERROR, exception_.u.ParamError.ParamDesc, "HY000");
			break;
		case odbc_SQLSvc_ExecDirect_SQLError_exn_:
		    jenv->CallVoidMethod(jobj, gJNICache.setCurrentStmtIdMethodId, stmtId);
			throwSQLException(jenv, &exception_.u.SQLError);
			break;
		case odbc_SQLSvc_ExecDirect_SQLQueryCancelled_exn_:
		 	jenv->CallVoidMethod(jobj, gJNICache.setCurrentStmtIdMethodId, stmtId);
			throwSQLException(jenv, QUERY_CANCELLED_ERROR, NULL, "HY008", 
				exception_.u.SQLQueryCancelled.sqlcode);
			break;
		case odbc_SQLSvc_ExecDirect_SQLInvalidHandle_exn_:
			jenv->CallVoidMethod(jobj, gJNICache.setCurrentStmtIdMethodId, stmtId);
			throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000", exception_.u.SQLInvalidHandle.sqlcode);
			break;
		case odbc_SQLSvc_ExecDirect_SQLStillExecuting_exn_:
		case odbc_SQLSvc_ExecDirect_InvalidConnection_exn_:
		case odbc_SQLSvc_ExecDirect_TransactionError_exn_:
		default:
			// TFDS - These error should not happen		
			 jenv->CallVoidMethod(jobj, gJNICache.setCurrentStmtIdMethodId, stmtId);
			throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000", exception_.exception_nr);
			break;
		}
		/* RFE: Batch update improvements
		 * Return to caller on error and if contBatchOnError is not true.
		 */
		if (exception_.exception_nr != CEE_SUCCESS && !contBatchOnError)
			FUNCTION_RETURN_VOID(("exception_.exception_nr(%ld) is not CEE_SUCCESS",exception_.exception_nr));
	
		/* RFE: Batch update improvements
		 * Queue up the exceptions. 
		 */
		if(jenv->ExceptionOccurred())
		{
			queuedException = jenv->ExceptionOccurred();
			if(exceptionHead)
				jenv->CallVoidMethod(exceptionHead, gJNICache.setNextExceptionMethodId,
									queuedException);				
			else
				exceptionHead = queuedException;					
		}

	}//end of for

	if (stmtLabel)
		JNI_ReleaseStringUTFChars(jenv,stmtLabel, nStmtLabel);

	if (cursorName)
		JNI_ReleaseStringUTFChars(jenv,cursorName, nCursorName);

	/* RFE: Batch update improvements
	 * If contBatchOnError is true, CEE_SUCCESS is always passed instead of
	 * exception_.exception_nr, so that endTxnControl suspends the transaction.	 
	 */	
	txn_status = endTxnControl(jenv, currentTxid, txid, autoCommit, 
							   (contBatchOnError ? CEE_SUCCESS:exception_.exception_nr), isSelect, txnMode, externalTxid);
	jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
	if (txn_status != 0)
	{
		throwTransactionException(jenv, txn_status);
		FUNCTION_RETURN_VOID(("endTxnControl() failed"));
	}
	/* RFE: Batch update improvements
	 * Throw the queued exception if any */
	if(exceptionHead)
		jenv->Throw(exceptionHead);
	FUNCTION_RETURN_VOID((NULL));
}
		
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXStatement_closeUsingLabel
  (JNIEnv *jenv, jclass jclass, jstring server, jlong dialogueId, 
		jstring stmtLabel, jboolean dropStmt)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXStatement_closeUsingLabel",("... stmtLabel=%s, dialogueId=0x%08x, dropStmt=%s",
		DebugJString(jenv,stmtLabel),
		dialogueId,
		DebugBoolStr(dropStmt)));

	ExceptionStruct				exception_;
	long						rowsAffected;
	ERROR_DESC_LIST_def			sqlWarning;
	ERROR_DESC_def				*error_desc_def;
	const char					*nStmtLabel;

	if (stmtLabel)
		nStmtLabel = JNI_GetStringUTFChars(jenv,stmtLabel, NULL);
	else
	{
		throwSQLException(jenv, INVALID_STMT_LABEL_ERROR, NULL, "HY000");
		FUNCTION_RETURN_VOID(("stmtLabel is NULL"));
	}

	odbc_SQLSvc_CloseUsingLabel_sme_(NULL, NULL, &exception_,
					dialogueId,
					(char *)nStmtLabel,
					(dropStmt ? SQL_DROP : SQL_CLOSE),
					&rowsAffected,
					&sqlWarning);
	
	if (stmtLabel)
		JNI_ReleaseStringUTFChars(jenv,stmtLabel, nStmtLabel);
	// Ignore setting warning since the input is jclass and not jobject
	switch (exception_.exception_nr)
	{
	case CEE_SUCCESS:
		break;
	case odbc_SQLSvc_Close_SQLError_exn_:
		break;
	case odbc_SQLSvc_Close_ParamError_exn_:
		throwSQLException(jenv, PROGRAMMING_ERROR, exception_.u.ParamError.ParamDesc, "HY000");
		break;
	case odbc_SQLSvc_Close_InvalidConnection_exn_:
	case odbc_SQLSvc_Close_TransactionError_exn_:
	default:
	// TFDS - This exceptions should not happen
		throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000", exception_.exception_nr);
		break;
	}
	FUNCTION_RETURN_VOID((NULL));
}

//venu changed dialogueId and stmtId from int to long for 64 bit
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXStatement_cancel
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jlong stmtId)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXStatement_cancel",("..."));

	ExceptionStruct				exception_;
	ERROR_DESC_LIST_def			sqlWarning;

	odbc_SQLSvc_CancelStatement_sme_(NULL, NULL, &exception_,
					dialogueId,
					stmtId,
					&sqlWarning);
	
	// Ignore setting warning since the input is jclass and not jobject
	switch (exception_.exception_nr)
	{
	case CEE_SUCCESS:
		break;
	case odbc_SQLSvc_CancelStatement_SQLError_exn_:
		throwSQLException(jenv, &exception_.u.SQLError);
		break;
	case odbc_SQLSvc_CancelStatement_ParamError_exn_:
		throwSQLException(jenv, PROGRAMMING_ERROR, exception_.u.ParamError.ParamDesc, "HY000");
		break;
	case odbc_SQLSvc_CancelStatement_SQLInvalidHandle_exn_:
		throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000", exception_.u.SQLInvalidHandle.sqlcode);
		break;
	case odbc_SQLSvc_CancelStatement_InvalidConnection_exn_:
	default:
	// TFDS - This exceptions should not happen
		throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000", exception_.exception_nr);
		break;
	}
	FUNCTION_RETURN_VOID((NULL));
}


//venu changed dialogueId and stmtId from int to long for 64 bit
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXStatement_resetFetchSize
  (JNIEnv *, jobject, jlong dialogueId, jlong stmtId, jint fetchSize)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXStatement_resetFetchSize",
		("... dialogueId=0x%08x, , stmtId=%ld, fetchSize=%ld",
		dialogueId,
		stmtId,
		fetchSize));

	SRVR_STMT_HDL::resetFetchSize(dialogueId, stmtId, fetchSize);
	FUNCTION_RETURN_VOID((NULL));
}

long SQLMXStatement_EXECUTE_FAILED(void)
{
	FUNCTION_ENTRY("SQLMXStatement_EXECUTE_FAILED",(NULL));
	FUNCTION_RETURN_NUMERIC(org_apache_trafodion_jdbc_t2_SQLMXStatement_JNI_EXECUTE_FAILED,
		("org_apache_trafodion_jdbc_t2_SQLMXStatement_JNI_EXECUTE_FAILED"));
}

long SQLMXStatement_SUCCESS_NO_INFO(void)
{
	FUNCTION_ENTRY("SQLMXStatement_SUCCESS_NO_INFO",(NULL));
	FUNCTION_RETURN_NUMERIC(org_apache_trafodion_jdbc_t2_SQLMXStatement_JNI_SUCCESS_NO_INFO,
		("org_apache_trafodion_jdbc_t2_SQLMXStatement_JNI_SUCCESS_NO_INFO"));
}
