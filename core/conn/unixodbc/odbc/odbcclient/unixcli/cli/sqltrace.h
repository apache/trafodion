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
//
//
#ifndef SQLTRACE_H
#define SQLTRACE_H

#include <odbccommon.h>

#ifdef __LP64__
#define SQLROWSETSIZE   SQLULEN
#define SQLROWOFFSET    SQLINTEGER
//#define SQLROWCOUNT     SQLUINTEGER
//#define SQLSETPOSIROW   SQLUSMALLINT
#endif /* __LP64__ */


typedef RETCODE  (SQL_API *FPTraceSQLAllocHandle) (SQLSMALLINT HandleType,
           SQLHANDLE InputHandle, SQLHANDLE *OutputHandle);

typedef RETCODE  (SQL_API *FPTraceSQLBindCol) (SQLHSTMT StatementHandle, 
		   SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType, 
		   SQLPOINTER TargetValue, SQLLEN BufferLength, 
	   	   SQLLEN *StrLen_or_Ind);

typedef RETCODE  (SQL_API *FPTraceSQLBindParam) (SQLHSTMT StatementHandle,
           SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
           SQLSMALLINT ParameterType, SQLULEN LengthPrecision,
           SQLSMALLINT ParameterScale, SQLPOINTER ParameterValue,
           SQLLEN *StrLen_or_Ind);

typedef RETCODE  (SQL_API *FPTraceSQLCancel) (SQLHSTMT StatementHandle);

typedef RETCODE  (SQL_API *FPTraceSQLCloseCursor) (SQLHSTMT StatementHandle);

typedef RETCODE  (SQL_API *FPTraceSQLColAttribute) (SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLength, SQLPOINTER NumericAttribute);

typedef RETCODE  (SQL_API *FPTraceSQLColumnPrivileges) (SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
           SQLCHAR *TableName, SQLSMALLINT NameLength3,
           SQLCHAR *ColumnName, SQLSMALLINT NameLength4);

typedef RETCODE  (SQL_API *FPTraceSQLColumns) (SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
           SQLCHAR *TableName, SQLSMALLINT NameLength3,
           SQLCHAR *ColumnName, SQLSMALLINT NameLength4);


typedef RETCODE  (SQL_API *FPTraceSQLConnect) (SQLHDBC ConnectionHandle,
           SQLCHAR *ServerName, SQLSMALLINT NameLength1,
           SQLCHAR *UserName, SQLSMALLINT NameLength2,
           SQLCHAR *Authentication, SQLSMALLINT NameLength3);

typedef RETCODE  (SQL_API *FPTraceSQLCopyDesc) (SQLHDESC SourceDescHandle,
           SQLHDESC TargetDescHandle);

typedef RETCODE  (SQL_API *FPTraceSQLDescribeCol)(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, SQLCHAR *ColumnName,
           SQLSMALLINT BufferLength, SQLSMALLINT *NameLength,
           SQLSMALLINT *DataType, SQLULEN *ColumnSize,
           SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable);

typedef RETCODE  (SQL_API *FPTraceSQLDisconnect) (SQLHDBC ConnectionHandle);

typedef RETCODE  (SQL_API *FPTraceSQLEndTran) (SQLSMALLINT HandleType, SQLHANDLE Handle,
           SQLSMALLINT CompletionType);

typedef RETCODE  (SQL_API *FPTraceSQLExecDirect) (SQLHSTMT StatementHandle,
           SQLCHAR *StatementText, SQLINTEGER TextLength);

typedef RETCODE  (SQL_API *FPTraceSQLExecute) (SQLHSTMT StatementHandle);

typedef RETCODE  (SQL_API *FPTraceSQLExtendedFetch) (SQLHSTMT StatementHandle,SQLUSMALLINT FetchOrientation,
			SQLLEN FetchOffset,SQLULEN* RowCountPtr,SQLUSMALLINT* RowStatusArray);

typedef RETCODE  (SQL_API *FPTraceSQLFetch) (SQLHSTMT StatementHandle);

typedef RETCODE  (SQL_API *FPTraceSQLFetchScroll) (SQLHSTMT StatementHandle,SQLUSMALLINT FetchOrientation,
			SQLINTEGER FetchOffset);

typedef RETCODE  (SQL_API *FPTraceSQLForeignKeys) (SQLHSTMT StatementHandle,
           SQLCHAR *PKCatalogName, SQLSMALLINT NameLength1,
           SQLCHAR *PKSchemaName, SQLSMALLINT NameLength2,
           SQLCHAR *PKTableName, SQLSMALLINT NameLength3,
           SQLCHAR *FKCatalogName, SQLSMALLINT NameLength4,
           SQLCHAR *FKSchemaName, SQLSMALLINT NameLength5,
           SQLCHAR *FKTableName, SQLSMALLINT NameLength6);

typedef RETCODE  (SQL_API *FPTraceSQLFreeHandle) (SQLSMALLINT HandleType, SQLHANDLE Handle);

typedef RETCODE  (SQL_API *FPTraceSQLFreeStmt) (SQLHSTMT StatementHandle,
           SQLUSMALLINT Option);

typedef RETCODE  (SQL_API *FPTraceSQLGetConnectAttr) (SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, SQLPOINTER Value,
           SQLINTEGER BufferLength, SQLINTEGER *StringLength);

typedef RETCODE  (SQL_API *FPTraceSQLGetConnectOption) (SQLHDBC ConnectionHandle,
           SQLUSMALLINT Option, SQLPOINTER Value);

typedef RETCODE  (SQL_API *FPTraceSQLGetCursorName) (SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLength);

typedef RETCODE  (SQL_API *FPTraceSQLGetData) (SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
           SQLPOINTER TargetValue, SQLLEN BufferLength,
           SQLLEN *StrLen_or_Ind);

typedef RETCODE  (SQL_API *FPTraceSQLGetDescField) (SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
           SQLPOINTER Value, SQLINTEGER BufferLength,
           SQLINTEGER *StringLength);

typedef RETCODE  (SQL_API *FPTraceSQLGetDescRec) (SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, SQLCHAR *Name,
           SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
           SQLSMALLINT *Type, SQLSMALLINT *SubType, 
           SQLLEN     *Length, SQLSMALLINT *Precision, 
           SQLSMALLINT *Scale, SQLSMALLINT *Nullable);

typedef RETCODE  (SQL_API *FPTraceSQLGetDiagField) (SQLSMALLINT HandleType, SQLHANDLE Handle,
           SQLSMALLINT RecNumber, SQLSMALLINT DiagIdentifier,
           SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLength);

typedef RETCODE  (SQL_API *FPTraceSQLGetDiagRec) (SQLSMALLINT HandleType, SQLHANDLE Handle,
           SQLSMALLINT RecNumber, SQLCHAR *Sqlstate,
           SQLINTEGER *NativeError, SQLCHAR *MessageText,
           SQLSMALLINT BufferLength, SQLSMALLINT *TextLength);

typedef RETCODE  (SQL_API *FPTraceSQLGetEnvAttr) (SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, SQLPOINTER Value,
           SQLINTEGER BufferLength, SQLINTEGER *StringLength);

typedef RETCODE  (SQL_API *FPTraceSQLGetInfo) (SQLHDBC ConnectionHandle,
           SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
           SQLSMALLINT BufferLength, SQLSMALLINT *StringLength);

typedef RETCODE  (SQL_API *FPTraceSQLGetStmtAttr) (SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, SQLPOINTER Value,
           SQLINTEGER BufferLength, SQLINTEGER *StringLength);

typedef RETCODE  (SQL_API *FPTraceSQLGetTypeInfo) (SQLHSTMT StatementHandle,
           SQLSMALLINT DataType);

typedef RETCODE  (SQL_API *FPTraceSQLNumResultCols) (SQLHSTMT StatementHandle,
           SQLSMALLINT *ColumnCount);

typedef RETCODE  (SQL_API *FPTraceSQLParamData) (SQLHSTMT StatementHandle,
           SQLPOINTER *Value);

typedef RETCODE  (SQL_API *FPTraceSQLPrepare) (SQLHSTMT StatementHandle,
           SQLCHAR *StatementText, SQLINTEGER TextLength);

typedef RETCODE  (SQL_API *FPTraceSQLPutData) (SQLHSTMT StatementHandle,
           SQLPOINTER Data, SQLLEN StrLen_or_Ind);

typedef RETCODE  (SQL_API *FPTraceSQLRowCount) (SQLHSTMT StatementHandle, 
	   SQLLEN* RowCount);

typedef RETCODE  (SQL_API *FPTraceSQLSetConnectAttr) (SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, SQLPOINTER Value,
           SQLINTEGER StringLength);

typedef RETCODE  (SQL_API *FPTraceSQLSetConnectOption) (SQLHDBC ConnectionHandle,
           SQLUSMALLINT Option, SQLULEN Value);

typedef RETCODE  (SQL_API *FPTraceSQLSetCursorName) (SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, SQLSMALLINT NameLength);

typedef RETCODE  (SQL_API *FPTraceSQLSetDescField) (SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
           SQLPOINTER Value, SQLINTEGER BufferLength);

typedef RETCODE  (SQL_API *FPTraceSQLSetDescRec) (SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, SQLSMALLINT Type,
           SQLSMALLINT SubType, SQLLEN Length,
           SQLSMALLINT Precision, SQLSMALLINT Scale,
           SQLPOINTER Data, SQLLEN *StringLength,
           SQLLEN *Indicator);

typedef RETCODE  (SQL_API *FPTraceSQLSetEnvAttr) (SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, SQLPOINTER Value,
           SQLINTEGER StringLength);

typedef RETCODE  (SQL_API *FPTraceSQLSetStmtAttr) (SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, SQLPOINTER Value,
           SQLINTEGER StringLength);

typedef RETCODE  (SQL_API *FPTraceSQLSpecialColumns) (SQLHSTMT StatementHandle,
           SQLUSMALLINT IdentifierType, SQLCHAR *CatalogName,
           SQLSMALLINT NameLength1, SQLCHAR *SchemaName,
           SQLSMALLINT NameLength2, SQLCHAR *TableName,
           SQLSMALLINT NameLength3, SQLUSMALLINT Scope,
           SQLUSMALLINT Nullable);

typedef RETCODE  (SQL_API *FPTraceSQLStatistics) (SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
           SQLCHAR *TableName, SQLSMALLINT NameLength3,
           SQLUSMALLINT Unique, SQLUSMALLINT Reserved);

typedef RETCODE  (SQL_API *FPTraceSQLTablePrivileges) (SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
           SQLCHAR *TableName, SQLSMALLINT NameLength3);

typedef RETCODE  (SQL_API *FPTraceSQLTables) (SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
           SQLCHAR *TableName, SQLSMALLINT NameLength3,
           SQLCHAR *TableType, SQLSMALLINT NameLength4);

typedef RETCODE (SQL_API *FPTraceSQLBrowseConnect) (
    SQLHDBC            hdbc,
    SQLCHAR 		  *szConnStrIn,
    SQLSMALLINT        cbConnStrIn,
    SQLCHAR 		  *szConnStrOut,
    SQLSMALLINT        cbConnStrOutMax,
    SQLSMALLINT       *pcbConnStrOut);

typedef RETCODE	(SQL_API *FPTraceSQLBulkOperations) (
	SQLHSTMT			StatementHandle,
	SQLSMALLINT			Operation);

typedef RETCODE (SQL_API *FPTraceSQLColAttributes) (
    SQLHSTMT           hstmt,
    SQLUSMALLINT       icol,
    SQLUSMALLINT       fDescType,
    SQLPOINTER         rgbDesc,
    SQLSMALLINT        cbDescMax,
    SQLSMALLINT 	  *pcbDesc,
    SQLLEN 		      * pfDesc);

typedef RETCODE (SQL_API *FPTraceSQLColumnPrivileges) (
    SQLHSTMT           hstmt,
    SQLCHAR 		  *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLCHAR 		  *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLCHAR 		  *szTableName,
    SQLSMALLINT        cbTableName,
    SQLCHAR 		  *szColumnName,
    SQLSMALLINT        cbColumnName);

typedef RETCODE (SQL_API *FPTraceSQLDescribeParam) (
    SQLHSTMT           hstmt,
    SQLUSMALLINT       ipar,
    SQLSMALLINT 	  *pfSqlType,
    SQLULEN      	  *pcbParamDef,
    SQLSMALLINT 	  *pibScale,
    SQLSMALLINT 	  *pfNullable);

typedef RETCODE (SQL_API *FPTraceSQLForeignKeys) (
    SQLHSTMT           hstmt,
    SQLCHAR 		  *szPkCatalogName,
    SQLSMALLINT        cbPkCatalogName,
    SQLCHAR 		  *szPkSchemaName,
    SQLSMALLINT        cbPkSchemaName,
    SQLCHAR 		  *szPkTableName,
    SQLSMALLINT        cbPkTableName,
    SQLCHAR 		  *szFkCatalogName,
    SQLSMALLINT        cbFkCatalogName,
    SQLCHAR 		  *szFkSchemaName,
    SQLSMALLINT        cbFkSchemaName,
    SQLCHAR 		  *szFkTableName,
    SQLSMALLINT        cbFkTableName);

typedef RETCODE (SQL_API *FPTraceSQLMoreResults) (
    SQLHSTMT           hstmt);

typedef RETCODE (SQL_API *FPTraceSQLNativeSql) (
    SQLHDBC            hdbc,
    SQLCHAR 		  *szSqlStrIn,
    SQLINTEGER         cbSqlStrIn,
    SQLCHAR 		  *szSqlStr,
    SQLINTEGER         cbSqlStrMax,
    SQLINTEGER 		  *pcbSqlStr);

typedef RETCODE (SQL_API *FPTraceSQLNumParams) (
    SQLHSTMT           hstmt,
    SQLSMALLINT 	  *pcpar);

typedef RETCODE (SQL_API *FPTraceSQLParamOptions) (
    SQLHSTMT           hstmt,
    SQLULEN		       crow,
    SQLULEN       	  *pirow);

typedef RETCODE (SQL_API *FPTraceSQLPrimaryKeys) (
    SQLHSTMT           hstmt,
    SQLCHAR 		  *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLCHAR 		  *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLCHAR 		  *szTableName,
    SQLSMALLINT        cbTableName);

typedef RETCODE (SQL_API *FPTraceSQLProcedureColumns) (
    SQLHSTMT           hstmt,
    SQLCHAR 		  *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLCHAR 		  *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLCHAR 		  *szProcName,
    SQLSMALLINT        cbProcName,
    SQLCHAR 		  *szColumnName,
    SQLSMALLINT        cbColumnName);

typedef RETCODE (SQL_API *FPTraceSQLProcedures) (
    SQLHSTMT           hstmt,
    SQLCHAR 		  *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLCHAR 		  *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLCHAR 		  *szProcName,
    SQLSMALLINT        cbProcName);

typedef RETCODE (SQL_API *FPTraceSQLSetPos) (
    SQLHSTMT           hstmt,
    SQLSETPOSIROW      irow,
    SQLUSMALLINT       fOption,
    SQLUSMALLINT       fLock);

typedef RETCODE (SQL_API *FPTraceSQLTablePrivileges) (
    SQLHSTMT           hstmt,
    SQLCHAR 		  *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLCHAR 		  *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLCHAR 		  *szTableName,
    SQLSMALLINT        cbTableName);

typedef RETCODE (SQL_API *FPTraceSQLBindParameter) (
    SQLHSTMT           hstmt,
    SQLUSMALLINT       ipar,
    SQLSMALLINT        fParamType,
    SQLSMALLINT        fCType,
    SQLSMALLINT        fSqlType,
    SQLULEN            cbColDef,
    SQLSMALLINT        ibScale,
    SQLPOINTER         rgbValue,
    SQLLEN             cbValueMax,
    SQLLEN     		   *pcbValue);

typedef RETCODE (SQL_API *FPTraceSQLDriverConnect) (
    SQLHDBC            hdbc,
    SQLHWND            hwnd,
    SQLCHAR 		  *szConnStrIn,
    SQLSMALLINT        cbConnStrIn,
    SQLCHAR           *szConnStrOut,
    SQLSMALLINT        cbConnStrOutMax,
    SQLSMALLINT 	  *pcbConnStrOut,
    SQLUSMALLINT       fDriverCompletion);

typedef VOID (SQL_API *FPTraceReturn) (RETCODE, RETCODE);

typedef VOID (SQL_API *FPTraceError) (RETCODE, RETCODE);

typedef DWORD* (WINAPI* FPTraceProcessEntry)(void);

typedef void (WINAPI* FPTraceDebugOut)(unsigned int,char*, char*, int);
typedef void (WINAPI* FPTraceTransportIn)(int, char*, void*, char*, long, long);
typedef void (WINAPI* FPTraceTransportOut)(int, char*, void*, char*, long, long);

typedef void (WINAPI* FPTracePrintMarker)(void);

typedef void (WINAPI* FPTraceFirstEntry)(char*);

//=====================================================================

extern "C" RETCODE  SQL_API TraceSQLAllocHandle (SQLSMALLINT HandleType,
           SQLHANDLE InputHandle, SQLHANDLE *OutputHandle);

extern "C" RETCODE  SQL_API TraceSQLBindCol (SQLHSTMT StatementHandle, 
		   SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType, 
		   SQLPOINTER TargetValue, SQLLEN BufferLength, 
	   	   SQLLEN *StrLen_or_Ind);

extern "C" RETCODE  SQL_API TraceSQLBindParam (SQLHSTMT StatementHandle,
           SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
           SQLSMALLINT ParameterType, SQLULEN LengthPrecision,
           SQLSMALLINT ParameterScale, SQLPOINTER ParameterValue,
           SQLLEN *StrLen_or_Ind);

extern "C" RETCODE  SQL_API TraceSQLCancel (SQLHSTMT StatementHandle);

extern "C" RETCODE  SQL_API TraceSQLCloseCursor (SQLHSTMT StatementHandle);

extern "C" RETCODE  SQL_API TraceSQLColAttribute (SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLength, SQLPOINTER NumericAttribute);

extern "C" RETCODE  SQL_API TraceSQLColumns(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
           SQLCHAR *TableName, SQLSMALLINT NameLength3,
           SQLCHAR *ColumnName, SQLSMALLINT NameLength4);


extern "C" RETCODE  SQL_API TraceSQLConnect(SQLHDBC ConnectionHandle,
           SQLCHAR *ServerName, SQLSMALLINT NameLength1,
           SQLCHAR *UserName, SQLSMALLINT NameLength2,
           SQLCHAR *Authentication, SQLSMALLINT NameLength3);

extern "C" RETCODE  SQL_API TraceSQLCopyDesc(SQLHDESC SourceDescHandle,
           SQLHDESC TargetDescHandle);

extern "C" RETCODE  SQL_API TraceSQLDescribeCol(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, SQLCHAR *ColumnName,
           SQLSMALLINT BufferLength, SQLSMALLINT *NameLength,
           SQLSMALLINT *DataType, SQLULEN *ColumnSize,
           SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable);

extern "C" RETCODE  SQL_API TraceSQLDisconnect(SQLHDBC ConnectionHandle);

extern "C" RETCODE  SQL_API TraceSQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle,
           SQLSMALLINT CompletionType);

extern "C" RETCODE  SQL_API TraceSQLExecDirect(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText, SQLINTEGER TextLength);

extern "C" RETCODE  SQL_API TraceSQLExecute(SQLHSTMT StatementHandle);

extern "C" RETCODE  SQL_API TraceSQLExtendedFetch(SQLHSTMT StatementHandle,SQLUSMALLINT FetchOrientation,
			SQLLEN FetchOffset,SQLULEN* RowCountPtr,SQLUSMALLINT* RowStatusArray);

extern "C" RETCODE  SQL_API TraceSQLFetch(SQLHSTMT StatementHandle);

extern "C" RETCODE  SQL_API TraceSQLFetchScroll(SQLHSTMT StatementHandle,
           SQLSMALLINT FetchOrientation, SQLROWOFFSET FetchOffset);

extern "C" RETCODE  SQL_API TraceSQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle);

extern "C" RETCODE  SQL_API TraceSQLFreeStmt(SQLHSTMT StatementHandle,
           SQLUSMALLINT Option);

extern "C" RETCODE  SQL_API TraceSQLGetConnectAttr(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, SQLPOINTER Value,
           SQLINTEGER BufferLength, SQLINTEGER *StringLength);

extern "C" RETCODE  SQL_API TraceSQLGetConnectOption(SQLHDBC ConnectionHandle,
           SQLUSMALLINT Option, SQLPOINTER Value);

extern "C" RETCODE  SQL_API TraceSQLGetCursorName(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLength);

extern "C" RETCODE  SQL_API TraceSQLGetData(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
           SQLPOINTER TargetValue, SQLLEN BufferLength,
           SQLLEN *StrLen_or_Ind);

extern "C" RETCODE  SQL_API TraceSQLGetDescField(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
           SQLPOINTER Value, SQLINTEGER BufferLength,
           SQLINTEGER *StringLength);

extern "C" RETCODE  SQL_API TraceSQLGetDescRec(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, SQLCHAR *Name,
           SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
           SQLSMALLINT *Type, SQLSMALLINT *SubType, 
           SQLLEN     *Length, SQLSMALLINT *Precision, 
           SQLSMALLINT *Scale, SQLSMALLINT *Nullable);

extern "C" RETCODE  SQL_API TraceSQLGetDiagField(SQLSMALLINT HandleType, SQLHANDLE Handle,
           SQLSMALLINT RecNumber, SQLSMALLINT DiagIdentifier,
           SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLength);

extern "C" RETCODE  SQL_API TraceSQLGetDiagRec(SQLSMALLINT HandleType, SQLHANDLE Handle,
           SQLSMALLINT RecNumber, SQLCHAR *Sqlstate,
           SQLINTEGER *NativeError, SQLCHAR *MessageText,
           SQLSMALLINT BufferLength, SQLSMALLINT *TextLength);

extern "C" RETCODE  SQL_API TraceSQLGetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, SQLPOINTER Value,
           SQLINTEGER BufferLength, SQLINTEGER *StringLength);

extern "C" RETCODE  SQL_API TraceSQLGetInfo(SQLHDBC ConnectionHandle,
           SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
           SQLSMALLINT BufferLength, SQLSMALLINT *StringLength);

extern "C" RETCODE  SQL_API TraceSQLGetStmtAttr(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, SQLPOINTER Value,
           SQLINTEGER BufferLength, SQLINTEGER *StringLength);

extern "C" RETCODE  SQL_API TraceSQLGetTypeInfo(SQLHSTMT StatementHandle,
           SQLSMALLINT DataType);

extern "C" RETCODE  SQL_API TraceSQLNumResultCols(SQLHSTMT StatementHandle,
           SQLSMALLINT *ColumnCount);

extern "C" RETCODE  SQL_API TraceSQLParamData(SQLHSTMT StatementHandle,
           SQLPOINTER *Value);

extern "C" RETCODE  SQL_API TraceSQLPrepare(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText, SQLINTEGER TextLength);

extern "C" RETCODE  SQL_API TraceSQLPutData(SQLHSTMT StatementHandle,
           SQLPOINTER Data, SQLLEN StrLen_or_Ind);

extern "C" RETCODE  SQL_API TraceSQLRowCount(SQLHSTMT StatementHandle, 
	   SQLLEN* RowCount);

extern "C" RETCODE  SQL_API TraceSQLSetConnectAttr(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, SQLPOINTER Value,
           SQLINTEGER StringLength);

extern "C" RETCODE  SQL_API TraceSQLSetConnectOption(SQLHDBC ConnectionHandle,
           SQLUSMALLINT Option, SQLULEN Value);

extern "C" RETCODE  SQL_API TraceSQLSetCursorName(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, SQLSMALLINT NameLength);

extern "C" RETCODE  SQL_API TraceSQLSetDescField(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
           SQLPOINTER Value, SQLINTEGER BufferLength);

extern "C" RETCODE  SQL_API TraceSQLSetDescRec(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, SQLSMALLINT Type,
           SQLSMALLINT SubType, SQLLEN Length,
           SQLSMALLINT Precision, SQLSMALLINT Scale,
           SQLPOINTER Data, SQLLEN *StringLength,
           SQLLEN *Indicator);

extern "C" RETCODE  SQL_API TraceSQLSetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, SQLPOINTER Value,
           SQLINTEGER StringLength);

extern "C" RETCODE  SQL_API TraceSQLSetStmtAttr(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, SQLPOINTER Value,
           SQLINTEGER StringLength);

extern "C" RETCODE  SQL_API TraceSQLSpecialColumns(SQLHSTMT StatementHandle,
           SQLUSMALLINT IdentifierType, SQLCHAR *CatalogName,
           SQLSMALLINT NameLength1, SQLCHAR *SchemaName,
           SQLSMALLINT NameLength2, SQLCHAR *TableName,
           SQLSMALLINT NameLength3, SQLUSMALLINT Scope,
           SQLUSMALLINT Nullable);

extern "C" RETCODE  SQL_API TraceSQLStatistics(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
           SQLCHAR *TableName, SQLSMALLINT NameLength3,
           SQLUSMALLINT Unique, SQLUSMALLINT Reserved);

extern "C" RETCODE  SQL_API TraceSQLTables(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
           SQLCHAR *TableName, SQLSMALLINT NameLength3,
           SQLCHAR *TableType, SQLSMALLINT NameLength4);

extern "C" RETCODE SQL_API TraceTdmSQLBrowseConnect(
    SQLHDBC            hdbc,
    SQLCHAR 		  *szConnStrIn,
    SQLSMALLINT        cbConnStrIn,
    SQLCHAR 		  *szConnStrOut,
    SQLSMALLINT        cbConnStrOutMax,
    SQLSMALLINT       *pcbConnStrOut);

extern "C" RETCODE	SQL_API TraceSQLBulkOperations(
	SQLHSTMT			StatementHandle,
	SQLSMALLINT			Operation);

extern "C" RETCODE SQL_API TraceSQLColAttributes(
    SQLHSTMT           hstmt,
    SQLUSMALLINT       icol,
    SQLUSMALLINT       fDescType,
    SQLPOINTER         rgbDesc,
    SQLSMALLINT        cbDescMax,
    SQLSMALLINT 	  *pcbDesc,
    SQLLEN 		      * pfDesc);

extern "C" RETCODE SQL_API TraceSQLColumnPrivileges(
    SQLHSTMT           hstmt,
    SQLCHAR 		  *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLCHAR 		  *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLCHAR 		  *szTableName,
    SQLSMALLINT        cbTableName,
    SQLCHAR 		  *szColumnName,
    SQLSMALLINT        cbColumnName);

extern "C" RETCODE SQL_API TraceSQLDescribeParam(
    SQLHSTMT           hstmt,
    SQLUSMALLINT       ipar,
    SQLSMALLINT 	  *pfSqlType,
    SQLULEN      	  *pcbParamDef,
    SQLSMALLINT 	  *pibScale,
    SQLSMALLINT 	  *pfNullable);

extern "C" RETCODE SQL_API TraceSQLForeignKeys(
    SQLHSTMT           hstmt,
    SQLCHAR 		  *szPkCatalogName,
    SQLSMALLINT        cbPkCatalogName,
    SQLCHAR 		  *szPkSchemaName,
    SQLSMALLINT        cbPkSchemaName,
    SQLCHAR 		  *szPkTableName,
    SQLSMALLINT        cbPkTableName,
    SQLCHAR 		  *szFkCatalogName,
    SQLSMALLINT        cbFkCatalogName,
    SQLCHAR 		  *szFkSchemaName,
    SQLSMALLINT        cbFkSchemaName,
    SQLCHAR 		  *szFkTableName,
    SQLSMALLINT        cbFkTableName);

extern "C" RETCODE SQL_API TraceSQLMoreResults(
    SQLHSTMT           hstmt);

extern "C" RETCODE SQL_API TraceSQLNativeSql(
    SQLHDBC            hdbc,
    SQLCHAR 		  *szSqlStrIn,
    SQLINTEGER         cbSqlStrIn,
    SQLCHAR 		  *szSqlStr,
    SQLINTEGER         cbSqlStrMax,
    SQLINTEGER 		  *pcbSqlStr);

extern "C" RETCODE SQL_API TraceSQLNumParams(
    SQLHSTMT           hstmt,
    SQLSMALLINT 	  *pcpar);

extern "C" RETCODE SQL_API TraceSQLParamOptions(
    SQLHSTMT           hstmt,
    SQLROWSETSIZE      crow,
    SQLROWSETSIZE  	  *pirow);

extern "C" RETCODE SQL_API TraceSQLPrimaryKeys(
    SQLHSTMT           hstmt,
    SQLCHAR 		  *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLCHAR 		  *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLCHAR 		  *szTableName,
    SQLSMALLINT        cbTableName);

extern "C" RETCODE SQL_API TraceSQLProcedureColumns(
    SQLHSTMT           hstmt,
    SQLCHAR 		  *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLCHAR 		  *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLCHAR 		  *szProcName,
    SQLSMALLINT        cbProcName,
    SQLCHAR 		  *szColumnName,
    SQLSMALLINT        cbColumnName);

extern "C" RETCODE SQL_API TraceSQLProcedures(
    SQLHSTMT           hstmt,
    SQLCHAR 		  *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLCHAR 		  *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLCHAR 		  *szProcName,
    SQLSMALLINT        cbProcName);

extern "C" RETCODE SQL_API TraceSQLSetPos(
    SQLHSTMT           hstmt,
    SQLSETPOSIROW      irow,
    SQLUSMALLINT       fOption,
    SQLUSMALLINT       fLock);

extern "C" RETCODE SQL_API TraceSQLTablePrivileges(
    SQLHSTMT           hstmt,
    SQLCHAR 		  *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLCHAR 		  *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLCHAR 		  *szTableName,
    SQLSMALLINT        cbTableName);

extern "C" RETCODE SQL_API TraceSQLBindParameter(
    SQLHSTMT           hstmt,
    SQLUSMALLINT       ipar,
    SQLSMALLINT        fParamType,
    SQLSMALLINT        fCType,
    SQLSMALLINT        fSqlType,
    SQLULEN            cbColDef,
    SQLSMALLINT        ibScale,
    SQLPOINTER         rgbValue,
    SQLLEN             cbValueMax,
    SQLLEN     		   *pcbValue);

extern "C" RETCODE SQL_API TraceTdmSQLDriverConnect (
    SQLHDBC            hdbc,
    SQLHWND            hwnd,
    SQLCHAR 		  *szConnStrIn,
    SQLSMALLINT        cbConnStrIn,
    SQLCHAR           *szConnStrOut,
    SQLSMALLINT        cbConnStrOutMax,
    SQLSMALLINT 	  *pcbConnStrOut,
    SQLUSMALLINT       fDriverCompletion);

extern "C" VOID SQL_API TraceReturn_P (RETCODE, RETCODE);

extern "C" VOID SQL_API TraceError (RETCODE, RETCODE);

extern "C" DWORD* WINAPI TraceProcessEntry(void);

extern "C" void WINAPI TraceDebugOut(unsigned int,char*, char*, int);

extern "C" void WINAPI TracePrintMarker(void);

extern "C" void WINAPI TraceFirstEntry(char*);

extern VERSION_def* ASVersion;
extern VERSION_def* SrvrVersion;
extern VERSION_def* SqlVersion;

extern "C" void WINAPI TraceTransportIn(int, char*, void*, char*, long, long);
extern "C" void WINAPI TraceTransportOut(int, char*, void*, char*, long, long);

#ifdef __LP64__
#undef SQLROWSETSIZE
#undef SQLROWOFFSET
//#undef SQLROWCOUNT
//#undef SQLSETPOSIROW
#endif /* __LP64__ */

#endif

