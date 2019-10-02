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
// MODULE: SQLMXCallableStatement.cpp
//
/*Change Log
 * Solution Number:10-091103-5969
 * Methods Changed: Java_org_apache_trafodion_jdbc_t2_SQLMXCallableStatement_prepareCall(JNIEnv *, jobject, jstring, jint, jint, jboolean, jint, jstring, jstring, jint, jint, jint)
 * Changed By: gargesh
 */
#include <platform_ndcs.h>
#ifdef NSK_PLATFORM
#include <sqlWin.h>
#else
#include <sql.h>
#endif
#include <sqlext.h>
#include "CoreCommon.h"
#include "JdbcDriverGlobal.h"
#include "org_apache_trafodion_jdbc_t2_SQLMXCallableStatement.h"
#include "SQLMXCommonFunctions.h"
#include "CSrvrStmt.h"
#include "SrvrCommon.h"
#include "Debug.h"

JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXCallableStatement_prepareCall
(JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,
 jint txid, jboolean autoCommit, jint txnMode, jstring stmtLabel, jstring sql,
 jint queryTimeout, jint holdability, jint fetchSize)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXCallableStatement_prepareCall",("... fetchSize=%ld",
		fetchSize));

	long					estimatedCost;
	long					inputParamOffset;
	ERROR_DESC_LIST_def			sqlWarning;
	SQLItemDescList_def			outputDesc;
	SQLItemDescList_def			inputDesc;
	jint						currentTxid = txid;
	jint						externalTxid = 0;
	jbyteArray					sqlByteArray;
	jboolean					isCopy;
	long						stmtId;
	jsize						len;
	SQLValue_def				sqlString;
	const char					*nSql = NULL;
	const char					*nStmtLabel = NULL;
	short						txn_status;

	ExceptionStruct exception_;
	CLEAR_EXCEPTION(exception_);

	sqlString.dataCharset = 0;
	sqlString.dataInd = 0;
	sqlString.dataType = SQLTYPECODE_VARCHAR;

	if (sql)
	{
		if ((sqlByteArray = (jbyteArray)jenv->CallObjectMethod(sql, gJNICache.getBytesMethodId))
			== NULL)
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
		//Start Soln. No.: 10-091103-5969
		sqlString.dataValue._buffer = new unsigned char [len+1];
		memset(sqlString.dataValue._buffer,'\0',len+1);
		strncpy((char *)sqlString.dataValue._buffer,(const char *)nSql,len);
		//sqlString.dataValue._buffer = (unsigned char *)nSql;
		//End Soln. No.: 10-091103-5969
		sqlString.dataValue._length = len;
	}
	else
	{
		throwSQLException(jenv, INVALID_SQL_STRING_ERROR, NULL, "HY090");
		FUNCTION_RETURN_VOID(("SQL string is Null"));
	}

	if (stmtLabel)
		nStmtLabel = JNI_GetStringUTFChars(jenv,stmtLabel, NULL);
	else
	{
		throwSQLException(jenv, INVALID_STMT_LABEL_ERROR, NULL, "HY000");
		FUNCTION_RETURN_VOID(("stmtLabel is Null"));
	}

	if ((txn_status = beginTxnControl(jenv, currentTxid, externalTxid, txnMode, -1)) != 0)
	{
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwTransactionException(jenv, txn_status);
		FUNCTION_RETURN_VOID(("beginTxnControl() failed"));
	}

	odbc_SQLSvc_Prepare_sme_(NULL, NULL,
		&exception_,
		dialogueId,
		nStmtLabel,
		"",					// StmtExplainName
		EXTERNAL_STMT,
		&sqlString,
		holdability,
		TYPE_CALL,
		0,
		fetchSize,
		queryTimeout,
		&estimatedCost,
		&inputDesc,
		&outputDesc,
		&sqlWarning,
		&stmtId,
		&inputParamOffset);

	if (sql)
	{
		JNI_ReleaseByteArrayElements(jenv,sqlByteArray, (jbyte *)nSql, JNI_ABORT);
	}

	if (stmtLabel)
		JNI_ReleaseStringUTFChars(jenv,stmtLabel, nStmtLabel);

	if ((txn_status = endTxnControl(jenv, currentTxid, txid, autoCommit, CEE_SUCCESS, FALSE, txnMode, externalTxid)) != 0)
	{
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwTransactionException(jenv, txn_status);
		FUNCTION_RETURN_VOID(("endTxnControl() Failed"));
	}

	switch (exception_.exception_nr)
	{
	case CEE_SUCCESS:
		outputDesc._length = 0;
		outputDesc._buffer = 0;
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
	case odbc_SQLSvc_Prepare_InvalidConnection_exn_:
	case odbc_SQLSvc_Prepare_SQLStillExecuting_exn_:
	case odbc_SQLSvc_Prepare_TransactionError_exn_:
	default:
		// TFDS - These exceptions should not happen
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000", exception_.exception_nr);
		break;
	}
	FUNCTION_RETURN_VOID((NULL));
}
//venu changed from int to long for 64 bit
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXCallableStatement_executeCall
(JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,
 jint txid, jboolean autoCommit, jint txnMode, jlong stmtId,
 jint paramCount, jobject paramValues, jint queryTimeout, jstring iso88591Encoding)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXCallableStatement_executeCall",("..."));

	SQLValueList_def				outputSqlValueList;
	ERROR_DESC_LIST_def				sqlWarning;
	jint							currentTxid = txid;
	jint							externalTxid = 0;
	short							returnResultSet;
	long							sqlcode;
	short							txn_status;

	SQLValueList_def				inputSqlValueList;
	CLEAR_LIST(inputSqlValueList);
	ExceptionStruct	exception_;
	CLEAR_EXCEPTION(exception_);

	SRVR_STMT_HDL	*pSrvrStmt;

	if ((pSrvrStmt = getSrvrStmt(dialogueId, stmtId, &sqlcode)) == NULL)
	{
		throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000", sqlcode);
		FUNCTION_RETURN_VOID(("getSrvrStmt() Failed"));
	}
	CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt);
	inputSqlValueList._buffer = NULL;
	inputSqlValueList._length = 0;

	if (fillInSQLValues(jenv, jobj, pSrvrStmt,
		0, 1, paramCount, paramValues, iso88591Encoding))
		FUNCTION_RETURN_VOID(("fillInSQLValues() Failed"));

	if ((txn_status = beginTxnControl(jenv, currentTxid, externalTxid, txnMode, -1)) != 0)
	{
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwTransactionException(jenv, txn_status);
		FUNCTION_RETURN_VOID(("beginTxnControl() failed"));
	}

	odbc_SQLSvc_ExecuteCall_sme_(NULL, NULL,
		&exception_,
		dialogueId,
		stmtId,
		&inputSqlValueList,
		FALSE,
		queryTimeout,
		&outputSqlValueList,
		&returnResultSet,
		&sqlWarning);

	if ((txn_status = endTxnControl(jenv, currentTxid, txid, autoCommit, exception_.exception_nr,
		pSrvrStmt->isSPJRS, txnMode, externalTxid)) != 0)
	{
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwTransactionException(jenv, txn_status);
		DEBUG_OUT(DEBUG_LEVEL_ENTRY,("endTxnControl() Failed"));
	}

	switch (exception_.exception_nr)
	{
	case CEE_SUCCESS:

		setExecuteCallOutputs(jenv, jobj, pSrvrStmt, returnResultSet, currentTxid);
		DEBUG_OUT(DEBUG_LEVEL_STMT,("RSMax: %d  RSIndex: %d  isSPJResultSet: %d",
			pSrvrStmt->RSMax, pSrvrStmt->RSIndex, pSrvrStmt->isSPJRS));

		if (sqlWarning._length > 0)
			setSQLWarning(jenv, jobj, &sqlWarning);
		break;
	case odbc_SQLSvc_ExecuteCall_SQLQueryCancelled_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, QUERY_CANCELLED_ERROR, NULL, "HY008",
			exception_.u.SQLQueryCancelled.sqlcode);
		break;
	case odbc_SQLSvc_ExecuteCall_SQLError_exn_:
	case odbc_SQLSvc_ExecuteCall_SQLRetryCompile_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, &exception_.u.SQLError);
		break;
	case odbc_SQLSvc_ExecuteCall_ParamError_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, PROGRAMMING_ERROR, exception_.u.ParamError.ParamDesc, "HY000");
		break;
	case odbc_SQLSvc_ExecuteCall_SQLInvalidHandle_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000", exception_.u.SQLInvalidHandle.sqlcode);
		break;
	case odbc_SQLSvc_ExecuteCall_SQLStillExecuting_exn_:
	case odbc_SQLSvc_ExecuteCall_InvalidConnection_exn_:
	case odbc_SQLSvc_ExecuteCall_TransactionError_exn_:
	case odbc_SQLSvc_ExecuteCall_SQLNeedData_exn_:
	default:
		// TFDS - These exceptions should not happen
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidStmtMethodId, currentTxid);
		throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000", exception_.exception_nr);
		break;
	}
	FUNCTION_RETURN_VOID((NULL));
}
