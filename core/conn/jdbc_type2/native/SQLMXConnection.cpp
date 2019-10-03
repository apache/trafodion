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
// MODULE: SQLMXConnection.cpp
//
#include <platform_ndcs.h>
#include <sql.h>
#include <sqlext.h>
#include "JdbcDriverGlobal.h"
#include "org_apache_trafodion_jdbc_t2_SQLMXConnection.h"
#include "SQLMXCommonFunctions.h"
#include "CoreCommon.h"
#include "SrvrCommon.h"
#include "SrvrOthers.h"
#include "CSrvrConnect.h"
#include "Debug.h"
#include "GlobalInformation.h"
#include "sqlcli.h"

void setConnectAttr(JNIEnv *jenv, jobject jobj, jstring server, long dialogueId,
        short attribute, SQLUINTEGER valueNum, const char *valuePtr) {
    FUNCTION_ENTRY("setConnectAttr",
            ("jenv=0x%08x, server=%s, dialogueId=0x%08x, attribute=%d, valueNum=%ld, valuePtr=0x%08x", jenv, DebugJString(
                    jenv, server), dialogueId, attribute, valueNum, valuePtr));

    ExceptionStruct exception_;
    ERROR_DESC_LIST_def sqlWarning;
    long sts;
    const char *nServer = NULL;

    odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL, &exception_, dialogueId,
            attribute, valueNum, (char *) valuePtr, &sqlWarning);
    switch (exception_.exception_nr) {
    case CEE_SUCCESS:
        break;
    case odbc_SQLSvc_SetConnectionOption_SQLError_exn_:
        throwSQLException(jenv, &exception_.u.SQLError);
        break;
    default:
        throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000",
                exception_.exception_nr);
        break;
    }
    FUNCTION_RETURN_VOID((NULL));
}

JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_setCatalog
(JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jstring catalog)
{
    FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_setCatalog",("jenv=0x%08x, server=%s, dialogueId=0x%08x, catalog=%s",
                    jenv,
                    DebugJString(jenv,server),
                    dialogueId,
                    DebugJString(jenv,catalog)));

    jthrowable sqlException;

    const char *nCatalog = NULL;
    if (catalog)
    nCatalog = JNI_GetStringUTFChars(jenv,catalog, NULL);

    setConnectAttr(jenv, jobj, server, dialogueId, SQL_ATTR_CURRENT_CATALOG, 0, nCatalog);

    if (catalog)
    JNI_ReleaseStringUTFChars(jenv,catalog, nCatalog);

    FUNCTION_RETURN_VOID((NULL));
}

JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_setAutoCommit
(JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jboolean autoCommit)
{
    FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_setAutoCommit",("jenv=0x%08x, server=%s, dialogueId=0x%08x, autoCommit=%d",
                    jenv,
                    DebugJString(jenv,server),
                    dialogueId,
                    autoCommit));

    setConnectAttr(jenv, jobj, server, dialogueId, SQL_AUTOCOMMIT, autoCommit, NULL);

    FUNCTION_RETURN_VOID((NULL));
}

//spjrs
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_setIsSpjRSFlag
(JNIEnv *jenv, jobject jobj, jlong dialogueId, jboolean isSpjrsOn)
{
    SRVR_CONNECT_HDL *pConnect;
    if(dialogueId != 0) {
        pConnect = (SRVR_CONNECT_HDL *)dialogueId;
        pConnect->isSPJRS = isSpjrsOn;
    }
}

JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_setTransactionIsolation
(JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jint transactionIsolation)
{
    FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_setTransactionIsolation",("jenv=0x%08x, server=%s, dialogueId=0x%08x, transactionIsolation=%ld",
                    jenv,
                    DebugJString(jenv,server),
                    dialogueId,
                    transactionIsolation));

    ExceptionStruct exception_;
    SQLItemDescList_def outputDesc;
    ERROR_DESC_LIST_def sqlWarning;
    SQLValueList_def outputValueList;
    long rowsAffected;
    char catStmtLabel[50];
    long stmtId;

    // Null out outputValueList before we pass it down
    CLEAR_LIST(outputValueList);
    odbc_SQLSvc_GetSQLCatalogs_sme_(NULL, NULL,
            &exception_,
            dialogueId,
            SQL_TXN_ISOLATION,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            0,
            0,
            0,
            0,
            0,
            transactionIsolation,
            0,
            catStmtLabel,
            &outputDesc,
            &sqlWarning,
            &rowsAffected,
            &outputValueList,
            &stmtId,
            NULL,
            NULL,
            NULL);
    switch (exception_.exception_nr)
    {
        case CEE_SUCCESS:
        if (sqlWarning._length > 0)
        setSQLWarning(jenv, jobj, &sqlWarning);
        break;
        case odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_:
        throwSQLException(jenv, MODULE_ERROR, exception_.u.ParamError.ParamDesc, "HY000");
        break;
        case odbc_SQLSvc_GetSQLCatalogs_SQLError_exn_:
        throwSQLException(jenv, &exception_.u.SQLError);
        break;
        case odbc_SQLSvc_GetSQLCatalogs_SQLInvalidHandle_exn_:
        throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000", exception_.u.SQLInvalidHandle.sqlcode);
        break;
        case odbc_SQLSvc_GetSQLCatalogs_InvalidConnection_exn_:
        default:
        // TFDS - These exceptions should not happen
        throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000", exception_.exception_nr);
        break;
    }

    FUNCTION_RETURN_VOID((NULL));
}

JNIEXPORT jlong JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_connect(
        JNIEnv *jenv, jobject jobj, jstring server, jstring uid, jstring pwd) {
    FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_connect",
            ("jenv=0x%08x, server=%s, uid=%ld, pwd=%s", jenv, DebugJString(jenv,
                    server), uid, DebugJString(jenv, pwd)));

    SRVR_CONNECT_HDL *jdbcConnect = NULL;
    const char *nUid;
    const char *nPwd;

    SQLRETURN rc;

    // Initialize gDescItems array
    initSqlCore(0, NULL);

    if (uid)
        nUid = JNI_GetStringUTFChars(jenv, uid, NULL);
    else
        nUid = NULL;

    if (pwd)
        nPwd = JNI_GetStringUTFChars(jenv, pwd, NULL);
    else
        nPwd = NULL;

    MEMORY_ALLOC_OBJ(jdbcConnect, SRVR_CONNECT_HDL());
    rc = jdbcConnect->sqlConnect(nUid, nPwd);
    if (uid)
        JNI_ReleaseStringUTFChars(jenv, uid, nUid);
    if (pwd)
        JNI_ReleaseStringUTFChars(jenv, pwd, nPwd);
    switch (rc) {
    case SQL_SUCCESS:
        break;
    case SQL_SUCCESS_WITH_INFO:
        setSQLWarning(jenv, jobj, &jdbcConnect->sqlWarning);
        jdbcConnect->cleanupSQLMessage();
        break;
    default:
        throwSQLException(jenv, jdbcConnect->getSQLError());
        jdbcConnect->cleanupSQLMessage();
        MEMORY_DELETE_OBJ(jdbcConnect);
        FUNCTION_RETURN_NUMERIC(0,
                ("jdbcConnect->sqlConnect() returned %s", CliDebugSqlError(rc)));
    }
    FUNCTION_RETURN_NUMERIC((jlong) jdbcConnect,
            ("jdbcConnect=0x%08x", jdbcConnect));
}

JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_close
(JNIEnv *jenv, jclass jcls, jstring server, jlong dialogueId)
{
    FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_close",("jenv=0x%08x, server=%s, dialogueId=0x%08x",
                    jenv,
                    DebugJString(jenv,server),
                    dialogueId));

    SRVR_CONNECT_HDL *jdbcConnect;
    SQLRETURN rc;
    
    ExceptionStruct setConnectException;
    ERROR_DESC_LIST_def sqlWarning;
    odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
                 &setConnectException,
                 dialogueId,
                 END_SESSION,
                 0,
                 NULL,
                 &sqlWarning);
    if (setConnectException.exception_nr != CEE_SUCCESS)
    {
        throwSetConnectionException(jenv, &setConnectException);
        FUNCTION_RETURN_VOID(("END_SESSION - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                        CliDebugSqlError(setConnectException.exception_nr)));
    }
    
    jdbcConnect = (SRVR_CONNECT_HDL *)dialogueId;
    rc = jdbcConnect->sqlClose();
    switch (rc)
    {
        case SQL_SUCCESS:
        break;
        case SQL_SUCCESS_WITH_INFO:
        //setSQLWarning(jenv, jcls, &jdbcConnect->sqlWarning);
        jdbcConnect->cleanupSQLMessage();
        break;
        default:
        throwSQLException(jenv, jdbcConnect->getSQLError());
        jdbcConnect->cleanupSQLMessage();
        break;
    }
    MEMORY_DELETE_OBJ(jdbcConnect);
    FUNCTION_RETURN_VOID((NULL));
}

JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_commit
(JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jint txid)
{
    FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_commit",
            ("jenv=0x%08x, server=%s, dialogueId=0x%08x, txid=0x%08x",
                    jenv,
                    DebugJString(jenv,server),
                    dialogueId,
                    txid));

    short status;

    status = resumeTransaction(txid);
    DEBUG_OUT(DEBUG_LEVEL_TXN,("resumeTransaction() returned %d", status));

    switch (status)
    {
        // Note: The following error codes do not require an abortTransaction(); however, all others do
        case 31:// Unable to obtain file-system buffer space
        case 36:// Unable to lock physical memory; not enough memory available
        case 78:// Invalid txn state
        break;
        default:
        status = endTransaction();
        DEBUG_OUT(DEBUG_LEVEL_TXN,("endTransaction() returned %d", status));
    }
    if (status != 0)
    {
        throwTransactionException(jenv, status);
        FUNCTION_RETURN_VOID(("status(%d) is non-zero",status));
    }
    FUNCTION_RETURN_VOID((NULL));
}

JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_rollback
(JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jint txid)
{
    FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_rollback",("jenv=0x%08x, server=%s, dialogueId=0x%08x, txid=0x%08x",
                    jenv,
                    DebugJString(jenv,server),
                    dialogueId,
                    txid));

    short status;

    status = resumeTransaction(txid);
    DEBUG_OUT(DEBUG_LEVEL_TXN,("resumeTransaction(%d) returned %d", txid, status));

    switch (status)
    {
        // Note: The following error codes do not require an abortTransaction(); however, all others do
        case 31:// Unable to obtain file-system buffer space
        case 36:// Unable to lock physical memory; not enough memory available
        case 78:// Invalid txn state
        break;
        default:
        status = abortTransaction();
        DEBUG_OUT(DEBUG_LEVEL_TXN,("abortTransaction() returned %d", status));
    }
    if (status != 0)
    {
        throwTransactionException(jenv, status);
        FUNCTION_RETURN_VOID(("status(%d) is non-zero",status));
    }
    FUNCTION_RETURN_VOID((NULL));
}

JNIEXPORT jint JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_beginTransaction(
        JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId) {
    FUNCTION_ENTRY(
            "Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_beginTransaction",
            ("jenv=0x%08x, server=%s, dialogueId=0x%08x", jenv, DebugJString(
                    jenv, server), dialogueId));

    short status = 1;
    short txid[4];
    jint currentTxid;

#ifdef NSK_PLATFORM	// Linux port - ToDo txn related
    status = GETTRANSID((short *)&txid);
#endif
    switch (status) {
    case 0:
        throwSQLException(jenv, INCONSISTENT_TRANSACTION_ERROR, NULL, "25000",
                0);
        FUNCTION_RETURN_NUMERIC(0, ("GETTRANSID() returned 0"));
    case 75:
        // Begin and Suspend the transaction immediately
        status = beginTransaction((long *) &currentTxid);
        if (status == 0)
            status = resumeTransaction(0);
        break;
    default:
        break;
    }
    if (status != 0) {
        throwTransactionException(jenv, status);
        FUNCTION_RETURN_NUMERIC(0, ("status(%d) is non-zero", status));
    }
    FUNCTION_RETURN_NUMERIC(currentTxid, ("currentTxid=0x%08x", currentTxid));
}

JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_connectInit
(JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jstring catalog,
        jstring schema, jboolean isReadOnly, jboolean autoCommit, jint transactionIsolation,
        jint loginTimeout, jint queryTimeout, jboolean blnDoomUsrTxn,
        jint statisticsIntervalTime, jint statisticsLimitTime, jstring statisticsType, jstring programStatisticsEnabled, jstring statisticsSqlPlanEnabled)
{

    FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_connectInit",("..."));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  jenv=0x%08x, server=%s, dialogueId=0x%08x",
                    jenv,
                    DebugJString(jenv,server),
                    dialogueId));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  catalog=%s",
                    DebugJString(jenv,catalog)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  schema=%s",
                    DebugJString(jenv,schema)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  isReadOnly=%d, transactionIsolation=%ld",
                    isReadOnly,
                    transactionIsolation));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  loginTimeout=%ld, queryTimeout=%ld",
                    loginTimeout,
                    queryTimeout));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  statisticsIntervalTime=%ld, statisticsLimitTime=%ld, statisticsType=%s, programStatisticsEnabled=%s, statisticsSqlPlanEnabled=%s",
                    statisticsIntervalTime,
                    statisticsLimitTime,
                    DebugJString(jenv,statisticsType),
                    DebugJString(jenv,programStatisticsEnabled),
                    DebugJString(jenv,statisticsSqlPlanEnabled)
            ));

    const char *nCatalog;
    const char *nSchema;
    jthrowable exception;

    // PUBLISHING
    const char *nStatisticsType;
    const char *nProgramStatisticsEnabled;
    const char *nStatisticsSqlPlanEnabled;

    jclass jcls = JNI_GetObjectClass(jenv,jobj);

    ExceptionStruct setConnectException;
    ERROR_DESC_LIST_def sqlWarning;

    // Debug checking if sqlStmtType enum's (in sqlcli.h) do not match those
    // defined in SQLMXConnection.java.  This is added as a safety means to detect
    // if they get out of sync with each other and exit. Furthermore, this will
    // get detected only during new development efforts and will not occur in the field.
    DEBUG_ASSERT(org_apache_trafodion_jdbc_t2_SQLMXConnection_SQL_CONTROL == SQL_CONTROL, ("Mismatch on SQL_CONTROL enum sql stmt type"));
    DEBUG_ASSERT(org_apache_trafodion_jdbc_t2_SQLMXConnection_SQL_SET_TRANSACTION == SQL_SET_TRANSACTION, ("Mismatch on SQL_SET_TRANSACTION enum sql stmt type"));
    DEBUG_ASSERT(org_apache_trafodion_jdbc_t2_SQLMXConnection_SQL_SET_CATALOG == SQL_SET_CATALOG, ("Mismatch on SQL_SET_CATALOG enum sql stmt type"));
    DEBUG_ASSERT(org_apache_trafodion_jdbc_t2_SQLMXConnection_SQL_SET_SCHEMA == SQL_SET_SCHEMA, ("Mismatch on SQL_SET_SCHEMA enum sql stmt type"));

#ifdef _DEBUG
    if((org_apache_trafodion_jdbc_t2_SQLMXConnection_SQL_CONTROL != SQL_CONTROL) ||
            (org_apache_trafodion_jdbc_t2_SQLMXConnection_SQL_SET_TRANSACTION != SQL_SET_TRANSACTION ) ||
            (org_apache_trafodion_jdbc_t2_SQLMXConnection_SQL_SET_CATALOG != SQL_SET_CATALOG) ||
            (org_apache_trafodion_jdbc_t2_SQLMXConnection_SQL_SET_SCHEMA != SQL_SET_SCHEMA))
    {
        printf("The SQLATTR_QUERY_TYPE enum's do not match to values defined in SQLMXConnection.\n");
        exit(1);
    }
#endif

    if (catalog)
    nCatalog = JNI_GetStringUTFChars(jenv,catalog, NULL);
    if (schema)
    nSchema = JNI_GetStringUTFChars(jenv,schema, NULL);

    odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
            &setConnectException,
            dialogueId,
            SET_JDBC_PROCESS,
            0,
            NULL,
            &sqlWarning);

    if (setConnectException.exception_nr != CEE_SUCCESS)
    {
        throwSetConnectionException(jenv, &setConnectException);
        FUNCTION_RETURN_VOID(("SET_JDBC_PROCESS - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                        CliDebugSqlError(setConnectException.exception_nr)));
    }

#if 0 /* NOT NEEDED with improvements to Native Expressions code */
    // new code begin: to disable native code for multi-threading
    odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
            &setConnectException,
            dialogueId,
            CQD_PCODE_OFF,
            0,
            NULL,
            &sqlWarning);
    if (setConnectException.exception_nr != CEE_SUCCESS)
    {
        throwSetConnectionException(jenv, &setConnectException);
        FUNCTION_RETURN_VOID(("CQD_PCODE_OFF - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                        CliDebugSqlError(setConnectException.exception_nr)));
    }
    // new code end
#endif /* NOT NEEDED with improvements to Native Expressions code */

    if (catalog)
    {
        odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
                &setConnectException,
                dialogueId,
                SET_CATALOG,
                0,
                (char *)nCatalog,
                &sqlWarning);
        JNI_ReleaseStringUTFChars(jenv,catalog, nCatalog);

        if (setConnectException.exception_nr != CEE_SUCCESS)
        {
            throwSetConnectionException(jenv, &setConnectException);
            FUNCTION_RETURN_VOID(("SET_CATALOG - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                            CliDebugSqlError(setConnectException.exception_nr)));
        }
    }
    else
    {
        strcpy(srvrGlobal->CurrentCatalog, "SEABASE");
    }

    if (schema)
    {

        odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
                &setConnectException,
                dialogueId,
                SET_SCHEMA,
                0,
                (char *)nSchema,
                &sqlWarning);
        JNI_ReleaseStringUTFChars(jenv,schema, nSchema);

        if (setConnectException.exception_nr != CEE_SUCCESS)
        {
            throwSetConnectionException(jenv, &setConnectException);
            FUNCTION_RETURN_VOID(("SET_SCHEMA - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                            CliDebugSqlError(setConnectException.exception_nr)));
        }
    }
    else
    {
        strcpy(srvrGlobal->CurrentSchema, "SEABASE");

    }

    odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
                 &setConnectException,
                 dialogueId,
                 BEGIN_SESSION,
                 0,
                 NULL,
                 &sqlWarning);
    if (setConnectException.exception_nr != CEE_SUCCESS)
    {
        throwSetConnectionException(jenv, &setConnectException);
        FUNCTION_RETURN_VOID(("BEGIN_SESSION - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                        CliDebugSqlError(setConnectException.exception_nr)));
    }

    if(blnDoomUsrTxn)
    {

        odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
                &setConnectException,
                dialogueId,
                CQD_DOOM_USER_TXN,
                0,
                NULL,
                &sqlWarning);
        if (setConnectException.exception_nr != CEE_SUCCESS)
        {
            throwSetConnectionException(jenv, &setConnectException);
            FUNCTION_RETURN_VOID(("CQD_DOOM_USER_TXN - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                            CliDebugSqlError(setConnectException.exception_nr)));
        }
    }

    odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
            &setConnectException,
            dialogueId,
            SQL_TXN_ISOLATION,
            transactionIsolation,
            NULL,
            &sqlWarning
    );

    if (setConnectException.exception_nr != CEE_SUCCESS)
    {
        throwSetConnectionException(jenv, &setConnectException);
        FUNCTION_RETURN_VOID(("SQL_TXN_ISOLATION - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                        CliDebugSqlError(setConnectException.exception_nr)));
    }

    odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
            &setConnectException,
            dialogueId,
            SQL_AUTOCOMMIT,
            autoCommit,
            NULL,
            &sqlWarning
    );

    if (setConnectException.exception_nr != CEE_SUCCESS)
    {
        throwSetConnectionException(jenv, &setConnectException);
        FUNCTION_RETURN_VOID(("SQL_AUTOCOMMIT - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                        CliDebugSqlError(setConnectException.exception_nr)));
    }

//    printf("Native statisticsIntervalTime :%ld\n", statisticsIntervalTime);
//    printf("Native statisticsLimitTime :%ld\n", statisticsLimitTime);

    if (statisticsType) {
        nStatisticsType = JNI_GetStringUTFChars(jenv,statisticsType, NULL);
//        printf("Native statisticsType :%s\n", nStatisticsType);
        JNI_ReleaseStringUTFChars(jenv,statisticsType, nStatisticsType);
    }
    if (programStatisticsEnabled) {
        nProgramStatisticsEnabled = JNI_GetStringUTFChars(jenv,programStatisticsEnabled, NULL);
//        printf("Native programStatisticsEnabled :%s\n", nProgramStatisticsEnabled);
        JNI_ReleaseStringUTFChars(jenv,programStatisticsEnabled, nProgramStatisticsEnabled);
    }
    if (statisticsSqlPlanEnabled) {
        nStatisticsSqlPlanEnabled = JNI_GetStringUTFChars(jenv,statisticsSqlPlanEnabled, NULL);
//        printf("Native statisticsSqlPlanEnabled :%s\n", nStatisticsSqlPlanEnabled);
        JNI_ReleaseStringUTFChars(jenv,statisticsSqlPlanEnabled, nStatisticsSqlPlanEnabled);
    }

    SRVR_STMT_HDL *RbwSrvrStmt = NULL;
    SRVR_STMT_HDL *CmwSrvrStmt = NULL;
    const char    *rbwStmtStr  = "ROLLBACK WORK";
    const char    *cmwStmtStr  = "COMMIT WORK";
    int           stmtLen;
    long          sqlcode;

    SQLValue_def inputValue;
    SQLRETURN retCode;

    inputValue.dataCharset = 0;
    inputValue.dataInd = 0;
    inputValue.dataType = SQLTYPECODE_VARCHAR;
    inputValue.dataValue._length = 0;
    MEMORY_ALLOC_ARRAY(inputValue.dataValue._buffer, unsigned char, MAX_INTERNAL_STMT_LEN);

    if((RbwSrvrStmt = createSrvrStmt(
                    dialogueId,
                    "STMT_ROLLBACK_1",
                    &sqlcode,
                    NULL,
                    SQLCLI_ODBC_MODULE_VERSION,
                    0,
                    TYPE_UNKNOWN,
                    FALSE,
                    SQL_UNKNOWN,
                    TRUE,
                    0)) == NULL)
    {
        setConnectException.exception_nr = odbc_SQLSvc_Prepare_SQLInvalidHandle_exn_;
        setConnectException.u.SQLInvalidHandle.sqlcode = sqlcode;
        FUNCTION_RETURN_VOID(("createSrvrStmt() Failed"));
    }

    stmtLen = strlen(rbwStmtStr);
    strncpy((char*)inputValue.dataValue._buffer, rbwStmtStr, stmtLen);
    inputValue.dataValue._length = stmtLen;
    retCode = RbwSrvrStmt->Prepare(&inputValue, INTERNAL_STMT, CLOSE_CURSORS_AT_COMMIT, 0);
    if(retCode == SQL_ERROR)
    {
        setConnectException.exception_nr = odbc_SQLSvc_Prepare_SQLError_exn_;
        //setConnectException.u.SQLError = ;
        FUNCTION_RETURN_VOID(("Prepare ROLLBACK WORK Transaction Statement Failed"));
    }

    if((CmwSrvrStmt = createSrvrStmt(
                    dialogueId,
                    "STMT_COMMIT_1",
                    &sqlcode,
                    NULL,
                    SQLCLI_ODBC_MODULE_VERSION,
                    0,
                    TYPE_UNKNOWN,
                    FALSE,
                    SQL_UNKNOWN,
                    TRUE,
                    0)) == NULL)
    {
        setConnectException.exception_nr = odbc_SQLSvc_Prepare_SQLInvalidHandle_exn_;
        setConnectException.u.SQLInvalidHandle.sqlcode = sqlcode;
        FUNCTION_RETURN_VOID(("createSrvrStmt() Failed"));
    }

    stmtLen = strlen(cmwStmtStr);
    strncpy((char*)inputValue.dataValue._buffer, cmwStmtStr, stmtLen);
    inputValue.dataValue._length = stmtLen;
    retCode = CmwSrvrStmt->Prepare(&inputValue, INTERNAL_STMT, CLOSE_CURSORS_AT_COMMIT, 0);
    if(retCode == SQL_ERROR)
    {
        setConnectException.exception_nr = odbc_SQLSvc_Prepare_SQLError_exn_;
        //setConnectException.u.SQLError = ;
        FUNCTION_RETURN_VOID(("Prepare COMMIT WORK Transaction Statement Failed"));
    }

    // Assign the module Information
    nullModule.version = SQLCLI_ODBC_MODULE_VERSION;
    nullModule.module_name = NULL;
    nullModule.module_name_len = 0;
    nullModule.charset = "ISO88591";
    nullModule.creation_timestamp = 0;

    srvrGlobal->m_FetchBufferSize = MAX_FETCH_BUFFER_SIZE; //Defaulting set the fetch buffer size to 512K
    srvrGlobal->fetchAhead = false;
    srvrGlobal->enableLongVarchar = false;
    srvrGlobal->boolFlgforInitialization = 1;
    srvrGlobal->maxRowsFetched = 0;

    // temporary set buildId for non delay error mode for all connection.
    // should be changed once we use InterfaceConnection in java layer.
    srvrGlobal->drvrVersion.buildId = STREAMING_MODE | ROWWISE_ROWSET | CHARSET | PASSWORD_SECURITY; 
    
    FUNCTION_RETURN_VOID((NULL));
}

JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_connectReuse
(JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jint conReuseBitMap, jstring catalog,
        jstring schema, jint transactionIsolation)
{
    FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_connectReuse",("..."));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  jenv=0x%08x, server=%s, dialogueId=0x%08x",
                    jenv,
                    DebugJString(jenv,server),
                    dialogueId));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  conReuseBitMap=%ld",
                    conReuseBitMap));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  catalog=%s",
                    DebugJString(jenv,catalog)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  schema=%s",
                    DebugJString(jenv,schema)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  transactionIsolation=%ld",
                    transactionIsolation));

    const char *nCatalog;
    const char *nSchema;
    jthrowable exception;

    jclass jcls = JNI_GetObjectClass(jenv,jobj);

    ExceptionStruct setConnectException;
    ERROR_DESC_LIST_def sqlWarning;

    if (catalog)
    nCatalog = JNI_GetStringUTFChars(jenv,catalog, NULL);
    if (schema)
    nSchema = JNI_GetStringUTFChars(jenv,schema, NULL);

    // Need to reset all if any CONTROL cmds were issued
    if (conReuseBitMap & org_apache_trafodion_jdbc_t2_SQLMXConnection_SQL_CONTROL_FLAG)
    {
        odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
                &setConnectException,
                dialogueId,
                RESET_DEFAULTS,
                0,
                NULL,
                &sqlWarning);

        if (setConnectException.exception_nr != CEE_SUCCESS)
        {
            throwSetConnectionException(jenv, &setConnectException);
            FUNCTION_RETURN_VOID(("RESET_DEFAULTS - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                            CliDebugSqlError(setConnectException.exception_nr)));
        }

        odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
                &setConnectException,
                dialogueId,
                SET_JDBC_PROCESS,
                0,
                NULL,
                &sqlWarning);
        if (setConnectException.exception_nr != CEE_SUCCESS)
        {
            throwSetConnectionException(jenv, &setConnectException);
            FUNCTION_RETURN_VOID(("SET_JDBC_PROCESS - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                            CliDebugSqlError(setConnectException.exception_nr)));
        }

        // new code begin: to disable native code for multi-threading
        odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
                &setConnectException,
                dialogueId,
                CQD_PCODE_OFF,
                0,
                NULL,
                &sqlWarning);
        if (setConnectException.exception_nr != CEE_SUCCESS)
        {
            throwSetConnectionException(jenv, &setConnectException);
            FUNCTION_RETURN_VOID(("CQD_PCODE_OFF - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                            CliDebugSqlError(setConnectException.exception_nr)));
        }
        // new code end

        if (catalog)
        {
            odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
                    &setConnectException,
                    dialogueId,
                    SET_CATALOG,
                    0,
                    (char *)nCatalog,
                    &sqlWarning);
            JNI_ReleaseStringUTFChars(jenv,catalog, nCatalog);

            if (setConnectException.exception_nr != CEE_SUCCESS)
            {
                throwSetConnectionException(jenv, &setConnectException);
                FUNCTION_RETURN_VOID(("SET_CATALOG - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                                CliDebugSqlError(setConnectException.exception_nr)));
            }
        }

        if (schema)
        {
            odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
                    &setConnectException,
                    dialogueId,
                    SET_SCHEMA,
                    0,
                    (char *)nSchema,
                    &sqlWarning );
            JNI_ReleaseStringUTFChars(jenv,schema, nSchema);

            if (setConnectException.exception_nr != CEE_SUCCESS)
            {
                throwSetConnectionException(jenv, &setConnectException);
                FUNCTION_RETURN_VOID(("SET_SCHEMA - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                                CliDebugSqlError(setConnectException.exception_nr)));
            }
        }

        odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
                &setConnectException,
                dialogueId,
                SQL_TXN_ISOLATION,
                transactionIsolation,
                NULL,
                &sqlWarning
        );

        if (setConnectException.exception_nr != CEE_SUCCESS)
        {
            throwSetConnectionException(jenv, &setConnectException);
            FUNCTION_RETURN_VOID(("SQL_TXN_ISOLATION - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                            CliDebugSqlError(setConnectException.exception_nr)));
        }
    }
    else // Check what other connection unique attributes have changed
    {
        // Upate catalog if "set catalog" bit is set
        if (conReuseBitMap & org_apache_trafodion_jdbc_t2_SQLMXConnection_SQL_SET_CATALOG_FLAG)
        {
            // Set catalog or clear catalog (if null)
            if (catalog)
            {
                odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
                        &setConnectException,
                        dialogueId,
                        SET_CATALOG,
                        0,
                        (char *)nCatalog,
                        &sqlWarning);

                JNI_ReleaseStringUTFChars(jenv,catalog, nCatalog);

                if (setConnectException.exception_nr != CEE_SUCCESS)
                {
                    throwSetConnectionException(jenv, &setConnectException);
                    FUNCTION_RETURN_VOID(("SET_CATALOG - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                                    CliDebugSqlError(setConnectException.exception_nr)));
                }
            }
            else // clear catalog
            {
                odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
                        &setConnectException,
                        dialogueId,
                        CLEAR_CATALOG,
                        0,
                        NULL,
                        &sqlWarning);

                if (setConnectException.exception_nr != CEE_SUCCESS)
                {
                    throwSetConnectionException(jenv, &setConnectException);
                    FUNCTION_RETURN_VOID(("CLEAR_CATALOG - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                                    CliDebugSqlError(setConnectException.exception_nr)));
                }
            }
        }
        // Upate schema if "set schema" bit is set
        if (conReuseBitMap & org_apache_trafodion_jdbc_t2_SQLMXConnection_SQL_SET_SCHEMA_FLAG)
        {
            // Set schema or clear schema (if null)
            if (schema)
            {
                odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
                        &setConnectException,
                        dialogueId,
                        SET_SCHEMA,
                        0,
                        (char *)nSchema,
                        &sqlWarning);

                JNI_ReleaseStringUTFChars(jenv,catalog, nSchema);

                if (setConnectException.exception_nr != CEE_SUCCESS)
                {
                    throwSetConnectionException(jenv, &setConnectException);
                    FUNCTION_RETURN_VOID(("SET_SCHEMA - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                                    CliDebugSqlError(setConnectException.exception_nr)));
                }
            }
            else // clear schema
            {
                odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
                        &setConnectException,
                        dialogueId,
                        CLEAR_SCHEMA,
                        0,
                        NULL,
                        &sqlWarning);

                if (setConnectException.exception_nr != CEE_SUCCESS)
                {
                    throwSetConnectionException(jenv, &setConnectException);
                    FUNCTION_RETURN_VOID(("CLEAR_SCHEMA - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                                    CliDebugSqlError(setConnectException.exception_nr)));
                }
            }
        }
        // Upate transaction isolation
        if (conReuseBitMap & org_apache_trafodion_jdbc_t2_SQLMXConnection_SQL_SET_TRANSACTION_FLAG)
        {
            odbc_SQLSvc_SetConnectionOption_sme_(NULL, NULL,
                    &setConnectException,
                    dialogueId,
                    SQL_TXN_ISOLATION,
                    transactionIsolation,
                    NULL,
                    &sqlWarning
            );

            if (setConnectException.exception_nr != CEE_SUCCESS)
            {
                throwSetConnectionException(jenv, &setConnectException);
                FUNCTION_RETURN_VOID(("SQL_TXN_ISOLATION - setConnectException.exception_nr(%s) is not CEE_SUCCESS",
                                CliDebugSqlError(setConnectException.exception_nr)));
            }
        }
    }
    FUNCTION_RETURN_VOID((NULL));
}

JNIEXPORT jstring JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_getCharsetEncoding(
        JNIEnv *jenv, jclass jcls, jstring server, jlong dialogueId,
        jint charset, jstring charsetOverride) {
    jstring rc;

    FUNCTION_ENTRY(
            "Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_getCharsetEncoding",
            ("jenv=0x%08x, server=%s, dialogueId=0x%08x, charset=%ld, charsetOverride=%s", jenv, DebugJString(
                    jenv, server), dialogueId, charset, DebugJString(jenv,
                    charsetOverride)));

    // Duplicating logic for current code.  Will probably change.
    if (charset == SQLCHARSETCODE_UNKNOWN)
        FUNCTION_RETURN_PTR(NULL, ("SQLCHARSETCODE_UNKNOWN returning null"));
    rc = getCharsetEncodingJava(jenv, charset, charsetOverride);
    //if ((rc == NULL) || (charset == SQLCHARSETCODE_SJIS))
    if (charset == SQLCHARSETCODE_SJIS) {
        throwSQLException(jenv, UNSUPPORTED_ENCODING_ERROR, NULL, "HY000");
        FUNCTION_RETURN_PTR(NULL, ("Unsupported charset"));
    }
    FUNCTION_RETURN_PTR(rc, ("%s", DebugJString(jenv, rc)));
}

JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_setCharsetEncodingOverride
(JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jint charset, jstring encodingOverride)
{
    FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXConnection_setCharsetEncodingOverride",
            ("jenv=0x%08x, server=%s, dialogueId=0x%08x, charset=%ld, encodingOverride=%s",
                    jenv,
                    DebugJString(jenv,server),
                    dialogueId,
                    charset,
                    DebugJString(jenv,encodingOverride)));

    for (int idx=0; idx<gJNICache.totalCharsets; idx++)
    if (charset == gJNICache.charsetInfo[idx].charset)
    {
        const char *nEncodingOverride = JNI_GetStringUTFChars(jenv,encodingOverride, NULL);
        if (strcmp(nEncodingOverride,defaultEncodingOption) == 0)
        {
            gJNICache.charsetInfo[idx].useDefaultEncoding = TRUE;
        }
        else
        {
            gJNICache.charsetInfo[idx].useDefaultEncoding = FALSE;
        }
        JNI_ReleaseStringUTFChars(jenv,encodingOverride, nEncodingOverride);
    }
    FUNCTION_RETURN_VOID((NULL));
}
