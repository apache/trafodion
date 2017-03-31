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
// MODULE: SQLMXDatabaseMetaData.cpp
//
// Note:
//  Each specific Java DatabaseMetaData method passes the appropriate 
//  catalog API Type and parameters to getSQLCatalogsInfo() via the 
//  wrapper functions in this file. 
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
#include "SQLMXCommonFunctions.h"
#include "org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData.h"
#include "Debug.h"
#define SQL_API_JDBC                    9999
#define SQL_API_SQLTABLES_JDBC          SQL_API_SQLTABLES + SQL_API_JDBC

JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getCatalogs
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jint txid, 
  jboolean autoCommit, jint txnMode, jstring catalogPattern)

{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getCatalogs",("..."));

	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId, txid, autoCommit, txnMode,
		SQL_API_SQLTABLES_JDBC, catalogPattern,
		NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, NULL, NULL, NULL);

	FUNCTION_RETURN_PTR(rc, (NULL));
}


JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getSchemas
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jint txid, 
  jboolean autoCommit, jint txnMode, jstring schemaPattern)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getSchemas",("server=%s, dialogueId=0x%08x, txid=0x%08x, autoCommit=%s, txnMode=%ld, schemaPattern=%s",
		DebugJString(jenv,server),
		dialogueId,
		txid,
        DebugBoolStr(autoCommit),
		txnMode,
		DebugJString(jenv,schemaPattern)));

	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId,  txid, autoCommit, txnMode, 
		SQL_API_SQLTABLES_JDBC, NULL, schemaPattern,
		NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, NULL, NULL, NULL);
	
	FUNCTION_RETURN_PTR(rc, (NULL));
}

JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getTables
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,  jint txid, 
  jboolean autoCommit, jint txnMode, jstring catalog, 
		jstring schemaPattern, jstring tableNamePattern, jstring tableType)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getTables",("..."));

	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId, txid, autoCommit, txnMode, 
		SQL_API_SQLTABLES, catalog, schemaPattern, 
		tableNamePattern, tableType, NULL, 0, 0, 0, 0, 0, 0, 1, NULL, NULL, NULL);

	FUNCTION_RETURN_PTR(rc, (NULL));
}

JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getTableTypes
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,  jint txid, 
  jboolean autoCommit, jint txnMode)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getTableTypes",("..."));

	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId,  txid, autoCommit, txnMode, 
		SQL_API_SQLTABLES, NULL, NULL, 
		NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, NULL, NULL, NULL);

	FUNCTION_RETURN_PTR(rc, (NULL));
}
JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getColumns
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,  jint txid, 
  jboolean autoCommit, jint txnMode, jstring catalog, jstring schemaPattern, 
  jstring tableNamePattern, jstring columnNamePattern)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getColumns",("..."));
	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId,  txid, autoCommit, txnMode, 
		SQL_API_SQLCOLUMNS, catalog, schemaPattern, 
		tableNamePattern, NULL, columnNamePattern, 0, 0, 0, 0, 0, 0, 1, NULL, NULL, NULL);

	FUNCTION_RETURN_PTR(rc, (NULL));
}

JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getPrimaryKeys
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,  jint txid, 
  jboolean autoCommit, jint txnMode, jstring catalog, jstring schema, 
		jstring table)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getPrimaryKeys",("..."));

	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId,  txid, autoCommit, txnMode, 
		SQL_API_SQLPRIMARYKEYS, catalog, schema, 
		table, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, NULL, NULL, NULL);

	FUNCTION_RETURN_PTR(rc, (NULL));
}

JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getIndexInfo
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,  jint txid, 
  jboolean autoCommit, jint txnMode, jstring catalog, jstring schema, 
		jstring table, jboolean unique, jboolean approximate)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getIndexInfo",("..."));
	long	uniqueness = SQL_INDEX_ALL;
	
	if (unique)
		uniqueness = SQL_INDEX_UNIQUE;
	
	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId,  txid, autoCommit, txnMode, 
		SQL_API_SQLSTATISTICS, catalog, schema, 
		table, NULL, NULL, 0, 0, 0, uniqueness, 0, 0, 1, NULL, NULL, NULL);

	FUNCTION_RETURN_PTR(rc, (NULL));
}

JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getTypeInfo
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,  jint txid, 
  jboolean autoCommit, jint txnMode)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getTypeInfo",("..."));
	
	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId,  txid, autoCommit, txnMode, 
		SQL_API_SQLGETTYPEINFO, NULL, NULL, 
		NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, NULL, NULL, NULL);

	FUNCTION_RETURN_PTR(rc, (NULL));
}


JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getBestRowIdentifier
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,  jint txid, 
  jboolean autoCommit, jint txnMode, jstring catalog, jstring schema, 
		jstring table, jint scope, jboolean nullable)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getBestRowIdentifier",("..."));

	long nullableODBC;

	if (nullable)
		nullableODBC = SQL_NULLABLE;
	else
		nullableODBC = SQL_NO_NULLS;

	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId,  txid, autoCommit, txnMode, 
		SQL_API_SQLSPECIALCOLUMNS, catalog, schema, 
		table, NULL, NULL, SQL_BEST_ROWID, scope, nullableODBC, 0, 0, 0, 1, NULL, NULL, NULL);

	FUNCTION_RETURN_PTR(rc, (NULL));
}

// getTablePrivileges added for V31 release 
JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getTablePrivileges
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,  jint txid, 
  jboolean autoCommit, jint txnMode, jstring catalog, jstring schema, 
		jstring table)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getTablePrivileges",("..."));
	
	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId,  txid, autoCommit, txnMode, 
		SQL_API_SQLTABLEPRIVILEGES, catalog, schema, 
		table, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, NULL, NULL, NULL);

	FUNCTION_RETURN_PTR(rc, (NULL));
}

// getColumnPrivileges added for V31 release 
JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getColumnPrivileges
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jint txid, 
  jboolean autoCommit, jint txnMode, jstring catalog, 
		jstring schemaPattern, jstring table, jstring columnNamePattern)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getColumnPrivileges",("..."));
	
	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId, txid, autoCommit, txnMode, 
		SQL_API_SQLCOLUMNPRIVILEGES, catalog, schemaPattern, 
		table, NULL, columnNamePattern, 0, 0, 0, 0, 0, 0, 1, NULL, NULL, NULL);

	FUNCTION_RETURN_PTR(rc, (NULL));
}
 
JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getExportedKeys
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,  jint txid, 
  jboolean autoCommit, jint txnMode, jstring catalog, jstring schema, 
		jstring table)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getExportedKeys",("..."));
	
	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId,  txid, autoCommit, txnMode, 
		SQL_API_SQLFOREIGNKEYS, catalog, schema, 
		table, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, NULL, NULL, NULL);

	FUNCTION_RETURN_PTR(rc, (NULL));
}

JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getImportedKeys
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,  jint txid, 
  jboolean autoCommit, jint txnMode, jstring fkcatalog, jstring fkschema, 
		jstring fktable)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getImportedKeys",("..."));
	
	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId,  txid, autoCommit, txnMode, 
		SQL_API_SQLFOREIGNKEYS, NULL, NULL, 
		NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, fkcatalog, fkschema, fktable);

	FUNCTION_RETURN_PTR(rc, (NULL));
}

JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getCrossReference
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,  jint txid, 
  jboolean autoCommit, jint txnMode, jstring catalog, jstring schema, jstring table, jstring fkcatalog, jstring fkschema, 
		jstring fktable)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getCrossReference",("..."));
	
	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId,  txid, autoCommit, txnMode, 
		SQL_API_SQLFOREIGNKEYS, catalog, schema, 
		table, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, fkcatalog, fkschema, fktable);

	FUNCTION_RETURN_PTR(rc, (NULL));
}

JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getVersionColumns
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,  jint txid, 
  jboolean autoCommit, jint txnMode, jstring catalog, jstring schema, 
		jstring table)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getVersionColumns",("..."));
	
	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId,  txid, autoCommit, txnMode, 
		SQL_API_SQLSPECIALCOLUMNS, catalog, schema, 
		table, NULL, NULL, SQL_ROWVER, 0, 0, 0, 0, 0, 1, NULL, NULL, NULL);

	FUNCTION_RETURN_PTR(rc, (NULL));
}

JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getProcedures
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jint txid, 
  jboolean autoCommit, jint txnMode, jstring catalog, 
		jstring schemaPattern, jstring procNamePattern)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getProcedures",("..."));
	
	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId, txid, autoCommit, txnMode,
		SQL_API_SQLPROCEDURES, catalog, schemaPattern, 
		procNamePattern, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, NULL, NULL, NULL);

	FUNCTION_RETURN_PTR(rc, (NULL));
}


JNIEXPORT jobject JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getProcedureColumns
  (JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, jint txid, 
  jboolean autoCommit, jint txnMode, jstring catalog, 
		jstring schemaPattern, jstring procNamePattern, jstring columnNamePattern)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_SQLMXDatabaseMetaData_getProcedureColumns",("..."));
	
	jobject rc = getSQLCatalogsInfo(jenv, jobj, server, dialogueId, txid, autoCommit, txnMode, 
		SQL_API_SQLPROCEDURECOLUMNS, catalog, schemaPattern, 
		procNamePattern, NULL, columnNamePattern, 0, 0, 0, 0, 0, 0, 1, NULL, NULL, NULL);

	FUNCTION_RETURN_PTR(rc, (NULL));
}
