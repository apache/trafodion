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
// MODULE: SQLMXResultSet.cpp
//

#include <platform_ndcs.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#ifndef NSK_PLATFORM
	#include <float.h>
#endif
#include <limits.h>
#ifdef NSK_PLATFORM
	#include <sqlWin.h>
	#include <windows.h>		
#else
	#include <sql.h>
#endif
#include <sqlext.h>
#include "JdbcDriverGlobal.h"
#include "CoreCommon.h"
#include "SQLMXCommonFunctions.h"
#include "org_apache_trafodion_jdbc_t2_SQLMXResultSet.h"
#include "org_apache_trafodion_jdbc_t2_SQLMXStatement.h"
#include "CSrvrStmt.h"
#include "SrvrCommon.h"
#include "Debug.h"

JNIEXPORT jboolean JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXResultSet_fetchN
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jint txid, jint txnMode,
		jlong stmtId, 
		jint maxRowCnt, jint queryTimeout, jint holdability)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXResultSet_fetchN",("server=%s, dialogueId=0x%08x, txid=0x%08x, txnMode=%ld, stmtId=0x%08x, maxRowCnt=%ld, queryTimeout=%ld, holdability=%ld",
		DebugJString(jenv,server),
		dialogueId,
		txid,
		txnMode,
		stmtId,
		maxRowCnt,
		queryTimeout,
		holdability));

	ExceptionStruct			exception_;
	SQLValueList_def		outputValueList;
	long					rowsAffected;
	BOOL					retCode = FALSE;
	ERROR_DESC_LIST_def		sqlWarning;
	jint					currentTxid = txid;
	jint					externalTxid = 0;
	const char				*nStmtLabel;
	
	SRVR_STMT_HDL	*pSrvrStmt;
	long			sqlcode;
	short			txn_status;

	if ((pSrvrStmt = getSrvrStmt(dialogueId, stmtId, &sqlcode)) == NULL)
	{
		throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000", sqlcode);
		FUNCTION_RETURN_NUMERIC(false,("getSrvrStmt() returned NULL"));
	}

	if ((txn_status = beginTxnControl(jenv, currentTxid, externalTxid, txnMode, holdability)) != 0)
	{
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidRSMethodId, currentTxid);
		throwTransactionException(jenv, txn_status);
		FUNCTION_RETURN_NUMERIC(false,("beginTxnControl() failed"));
	}

	odbc_SQLSvc_FetchN_sme_(NULL, NULL, &exception_,
			dialogueId, stmtId, maxRowCnt, FALSE, queryTimeout, &rowsAffected, &outputValueList,
			&sqlWarning);

	if ((txn_status = endTxnControl(jenv, currentTxid, 0, FALSE, 
		(exception_.exception_nr == odbc_SQLSvc_FetchN_SQLNoDataFound_exn_ ? CEE_SUCCESS : 
			exception_.exception_nr), TRUE, txnMode, externalTxid)) != 0)
	{
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidRSMethodId, currentTxid);
		throwTransactionException(jenv, txn_status);
		FUNCTION_RETURN_NUMERIC(false,("endTxnControl() Failed"));
	}
	switch (exception_.exception_nr)
	{
	case CEE_SUCCESS:
		if (sqlWarning._length != 0)
			setSQLWarning(jenv, jobj, &sqlWarning);

		setFetchOutputs(jenv, jobj, pSrvrStmt, &outputValueList, rowsAffected, 
			rowsAffected < maxRowCnt ? TRUE : FALSE, TRUE, currentTxid);

		retCode = TRUE;
		break;
	case odbc_SQLSvc_FetchN_SQLNoDataFound_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidRSMethodId, currentTxid);
		break;
	case odbc_SQLSvc_FetchN_SQLQueryCancelled_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidRSMethodId, currentTxid);
		throwSQLException(jenv, QUERY_CANCELLED_ERROR, NULL, "HY008", 
			exception_.u.SQLQueryCancelled.sqlcode);
		break;
	case odbc_SQLSvc_FetchN_SQLError_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidRSMethodId, currentTxid);
		throwSQLException(jenv, &exception_.u.SQLError);
		break;
	case odbc_SQLSvc_FetchN_ParamError_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidRSMethodId, currentTxid);
		throwSQLException(jenv, PROGRAMMING_ERROR, exception_.u.ParamError.ParamDesc, "HY000");
		break;
	case odbc_SQLSvc_FetchN_SQLInvalidHandle_exn_:
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidRSMethodId, currentTxid);
		throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000", exception_.u.SQLInvalidHandle.sqlcode);
		break;
	case odbc_SQLSvc_FetchN_SQLStillExecuting_exn_:
	case odbc_SQLSvc_FetchN_InvalidConnection_exn_:
	case odbc_SQLSvc_FetchN_TransactionError_exn_:
	default:
// TFDS
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidRSMethodId, currentTxid);
		throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000", exception_.exception_nr);
		break;
	}
	FUNCTION_RETURN_NUMERIC(retCode,(DebugBoolStr(retCode)));
}

//venu changed dialogueId and stmtId from int to long for 64 bit

JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXResultSet_close
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jint txid, jboolean autoCommit, jint txnMode,
	jlong stmtId, jboolean dropStmt)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXResultSet_close",("server=%s, dialogueId=0x%08x, txid=0x%08x, autoCommit=%s, txnMode=%ld, stmtId=0x%08x, dropStmt=%s",
		DebugJString(jenv,server),
		dialogueId,
		txid,
		DebugBoolStr(autoCommit),
		txnMode,
        	stmtId,
		DebugBoolStr(dropStmt)));

	jint						currentTxid = txid;
	jint						externalTxid = 0;
	jboolean					selectStmt = FALSE;
	ExceptionStruct				exception_;
	long						rowsAffected;
	ERROR_DESC_LIST_def			sqlWarning;
	short						txn_status;
	
	ERROR_DESC_def				*error_desc_def;
	// Don't bother resuming the transaction, if it is already zero. Try and close the cursor
	if (currentTxid != 0)
	{
		if ((txn_status = beginTxnControl(jenv, currentTxid, externalTxid, txnMode, CLOSE_CURSORS_AT_COMMIT)) != 0 )
		{
			jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidRSMethodId, currentTxid);
			throwTransactionException(jenv, txn_status);
			FUNCTION_RETURN_VOID(("beginTxnControl() failed"));
		}
	}
	
	odbc_SQLSvc_Close_sme_(NULL, NULL, &exception_,
					dialogueId,
					stmtId,
					(dropStmt ? SQL_DROP : SQL_CLOSE),
					&rowsAffected,
					&sqlWarning);
	
	if (currentTxid != 0)
	{
		if ((txn_status = endTxnControl(jenv, currentTxid, 0, autoCommit, 0, selectStmt, txnMode, externalTxid)) != 0)
		{
			jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidRSMethodId, currentTxid);
			throwTransactionException(jenv, txn_status);
			FUNCTION_RETURN_VOID(("endTxnControl() failed"));
		}
		jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidRSMethodId, currentTxid);
	}	
	
	switch (exception_.exception_nr)
	{
	case CEE_SUCCESS:
		// Not updating rowsAffected, since it is not useful in JDBC
		if (sqlWarning._length != 0)
			setSQLWarning(jenv, jobj, &sqlWarning);
		selectStmt = FALSE;
		break;
	case odbc_SQLSvc_Close_SQLError_exn_:
		error_desc_def =((ERROR_DESC_LIST_def *)(&exception_.u.SQLError.errorList))->_buffer;
		if (error_desc_def->sqlcode == -8811)		// Close a non-existant cursor and hence don't end 
													// the transaction by treating it like Select stmt
			selectStmt = TRUE;	
		// sql Error in case of close could be treated as SQLWarning
		setSQLWarning(jenv, jobj, &exception_.u.SQLError.errorList);
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
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXResultSet_getResultSetInfo
  (JNIEnv *jenv, jobject jobj, jlong dialogueId, jlong stmtId, jobject RSInfo)
{
	FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_STMT,"Java_org_apache_trafodion_jdbc_t2_SQLMXResultSet_getResultSetInfo",
		("dialogueId=0x%08x, stmtId=0x%08x, RSInfo=0x%08x",
		dialogueId,
		stmtId,
		RSInfo));

	SQLCTX_HANDLE	ctxHandle;
	SRVR_STMT_HDL	*pSrvrStmt;
	long			sqlcode;

	// Get the Statement ID
	if ((pSrvrStmt = getSrvrStmt(dialogueId, stmtId, &sqlcode)) == NULL)
	{
		/*
		* Start Solution No. 10-100412-9447
		*/
		// Set the Field ID 
		//Start Solution No.: 10-091103-5969
		DEBUG_OUT(DEBUG_LEVEL_STMT,("getResultSetInfo() &pSrvrStmt->stmt: 0x%08x", 0));
		//End Solution No.: 10-091103-5969
		JNI_SetIntField(jenv, RSInfo, gJNICache.SPJRS_stmtIDFieldID, (jint)(0));
		// Set the CTXHandle, this is same as the dialogId.
		DEBUG_OUT(DEBUG_LEVEL_STMT,("getResultSetInfo() ctxHandle: 0x%08x", dialogueId));
		JNI_SetIntField(jenv, RSInfo, gJNICache.SPJRS_ctxHandleFieldID, (jint)(dialogueId));
		// Set the CLI statement closure status to the java variable
		DEBUG_OUT(DEBUG_LEVEL_STMT,("getResultSetInfo() pSrvrStmt->isClosed: 0x%08x", JNI_TRUE)); //10-060831-8723
		JNI_SetBooleanField(jenv, RSInfo, gJNICache.SPJRS_stmtClosedFieldID, (jint)(JNI_TRUE));  //10-060831-8723
		/*
		* End Solution No. 10-100412-9447
		*/
		// Variable initialized to zero in calling routine, so 
		// if getting pSrvrStmt fails, just return w/o setting
		DEBUG_OUT(DEBUG_LEVEL_STMT,("getResultSetInfo() getSrvrStmt() failed"));
		FUNCTION_RETURN_VOID((NULL));
	} 
	
	// Set the Field ID 
	DEBUG_OUT(DEBUG_LEVEL_STMT,("getResultSetInfo() &pSrvrStmt->stmt: 0x%08x", &(pSrvrStmt->stmt)));
	JNI_SetIntField(jenv, RSInfo, gJNICache.SPJRS_stmtIDFieldID, (jlong)&(pSrvrStmt->stmt));

	// Get the Current Context Handle
	ctxHandle = pSrvrStmt->getContext();
	if (ctxHandle == 0)		// getContext() failed
	{
		// if ctxHandle = 0, call failed to obtain current context, rtn w/o setting
		DEBUG_OUT(DEBUG_LEVEL_STMT,("getResultSetInfo() ctxHandle failed ctxHandle: 0x%08x", ctxHandle));
		FUNCTION_RETURN_VOID((NULL));
	} 
	DEBUG_OUT(DEBUG_LEVEL_STMT,("getResultSetInfo() ctxHandle: 0x%08x", ctxHandle));
	JNI_SetIntField(jenv, RSInfo, gJNICache.SPJRS_ctxHandleFieldID, (jint)(ctxHandle));

	// Set the CLI statement closure status to the java variable
	DEBUG_OUT(DEBUG_LEVEL_STMT,("getResultSetInfo() pSrvrStmt->isClosed: 0x%08x", pSrvrStmt->isClosed)); //10-060831-8723
	JNI_SetBooleanField(jenv, RSInfo, gJNICache.SPJRS_stmtClosedFieldID, (jint)(pSrvrStmt->isClosed));  //10-060831-8723
	FUNCTION_RETURN_VOID((NULL));
}
