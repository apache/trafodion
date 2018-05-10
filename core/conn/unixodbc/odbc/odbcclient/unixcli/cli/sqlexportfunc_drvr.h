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
#ifndef SQLEXPORTFUNC_DRVR_H
#define SQLEXPORTFUNC_DRVR_H

#include "drvrglobal.h"
#include "sqlhandle.h"
#include "sqlconnect.h"
#include "sqlenv.h"
#include "sqlstmt.h"
#include "sqldesc.h"
#include "diagfunctions.h"
#include "DrvrSrvr.h"


extern "C"
{
SQLRETURN  SQL_API SQLAllocHandle(SQLSMALLINT HandleType,
				SQLHANDLE InputHandle, 
				SQLHANDLE *OutputHandle);
}


extern "C"
{
SQLRETURN  SQL_API SQLFreeHandle(SQLSMALLINT HandleType, 
				SQLHANDLE Handle);
}


//Ansi Function
extern "C"
{
SQLRETURN SQL_API SQLGetDiagRecA(SQLSMALLINT HandleType,
                SQLHANDLE Handle,
                SQLSMALLINT RecNumber,
                SQLCHAR *Sqlstate,
                SQLINTEGER *NativeError,
                SQLCHAR *MessageText,
                SQLSMALLINT BufferLength,
                SQLSMALLINT *TextLength);
}


extern "C"
{
SQLRETURN SQL_API SQLGetDiagFieldA(SQLSMALLINT HandleType,
                SQLHANDLE Handle,
                SQLSMALLINT RecNumber,
                SQLSMALLINT DiagIdentifier,
                SQLPOINTER DiagInfo,
                SQLSMALLINT BufferLength,
                SQLSMALLINT *StringLength);
}


extern "C"
{
SQLRETURN SQL_API SQLConnectA(SQLHDBC ConnectionHandle,
           SQLCHAR *ServerName,
           SQLSMALLINT NameLength1,
           SQLCHAR *UserName,
           SQLSMALLINT NameLength2,
           SQLCHAR *Authentication,
           SQLSMALLINT NameLength3);
}


extern "C"
{
SQLRETURN  SQL_API SQLSetConnectAttrA(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute,
           SQLPOINTER Value,
           SQLINTEGER StringLength);
}


extern "C"
{
SQLRETURN  SQL_API SQLGetConnectAttrA(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute,
           SQLPOINTER Value,
           SQLINTEGER BufferLength,
           SQLINTEGER *StringLength);
}


extern "C"
{
SQLRETURN  SQL_API SQLSetStmtAttrA(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute,
           SQLPOINTER Value,
           SQLINTEGER StringLength);
}


extern "C"
{
SQLRETURN  SQL_API SQLGetStmtAttrA(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute,
           SQLPOINTER Value,
           SQLINTEGER BufferLength,
           SQLINTEGER *StringLength);
}


extern "C"
{
SQLRETURN  SQL_API SQLGetInfoA(SQLHDBC ConnectionHandle,
           SQLUSMALLINT InfoType,
           SQLPOINTER InfoValuePtr,
           SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLengthPtr);
}


extern "C"
{
SQLRETURN  SQL_API SQLSetDescFieldA(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber,
           SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr,
           SQLINTEGER BufferLength);
}


extern "C"
{
SQLRETURN  SQL_API SQLGetDescFieldA(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber,
           SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr,
           SQLINTEGER BufferLength,
           SQLINTEGER *StringLengthPtr);
}


extern "C"
{
SQLRETURN  SQL_API SQLGetDescRecA(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber,
           SQLCHAR *Name,
           SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLengthPtr,
           SQLSMALLINT *TypePtr,
           SQLSMALLINT *SubTypePtr,
           SQLLEN     *LengthPtr,
           SQLSMALLINT *PrecisionPtr,
           SQLSMALLINT *ScalePtr,
           SQLSMALLINT *NullablePtr);
}


extern "C"
{
SQLRETURN SQL_API SQLBrowseConnectA(
    SQLHDBC            ConnectionHandle,
    SQLCHAR           *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLCHAR           *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT       *StringLength2Ptr);
}


extern "C"
{
SQLRETURN SQL_API SQLDriverConnectA(SQLHDBC  ConnectionHandle,
    SQLHWND            WindowHandle,
    SQLCHAR           *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLCHAR           *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT       *StringLength2Ptr,
    SQLUSMALLINT       DriverCompletion);
}


extern "C"
{
SQLRETURN SQL_API SQLPrepareA(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
           SQLINTEGER TextLength);
}


extern "C"
{
SQLRETURN SQL_API SQLExecDirectA(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
           SQLINTEGER TextLength);
}


extern "C"
{
SQLRETURN  SQL_API SQLDescribeColA(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber,
           SQLCHAR *ColumnName,
           SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLengthPtr,
           SQLSMALLINT *DataTypePtr,
           SQLULEN *ColumnSizePtr,
           SQLSMALLINT *DecimalDigitsPtr,
           SQLSMALLINT *NullablePtr);
}


extern "C"
{
SQLRETURN  SQL_API SQLTablesA(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName,
           SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName,
           SQLSMALLINT NameLength2,
           SQLCHAR *TableName,
           SQLSMALLINT NameLength3,
           SQLCHAR *TableType,
           SQLSMALLINT NameLength4);
}


extern "C"
{
SQLRETURN  SQL_API SQLColumnsA(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName,
           SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName,
           SQLSMALLINT NameLength2,
           SQLCHAR *TableName,
           SQLSMALLINT NameLength3,
           SQLCHAR *ColumnName,
           SQLSMALLINT NameLength4);
}


extern "C"
{
SQLRETURN  SQL_API SQLSpecialColumnsA(SQLHSTMT StatementHandle,
           SQLUSMALLINT IdentifierType,
           SQLCHAR *CatalogName,
           SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName,
           SQLSMALLINT NameLength2,
           SQLCHAR *TableName,
           SQLSMALLINT NameLength3,
           SQLUSMALLINT Scope,
           SQLUSMALLINT Nullable);
}


extern "C"
{
SQLRETURN  SQL_API SQLGetTypeInfoA(SQLHSTMT StatementHandle,
           SQLSMALLINT DataType);
}


extern "C"
{
SQLRETURN  SQL_API SQLPrimaryKeysA(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName,
           SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName,
           SQLSMALLINT NameLength2,
           SQLCHAR *TableName,
           SQLSMALLINT NameLength3);
}


extern "C"
{
SQLRETURN  SQL_API SQLStatisticsA(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName,
           SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName,
           SQLSMALLINT NameLength2,
           SQLCHAR *TableName,
           SQLSMALLINT NameLength3,
           SQLUSMALLINT Unique,
           SQLUSMALLINT Reserved);
}


extern "C"
{
SQLRETURN  SQL_API SQLGetCursorNameA(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName,
           SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLengthPtr);
}


extern "C"
{
SQLRETURN  SQL_API SQLSetCursorNameA(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName,
           SQLSMALLINT NameLength);
}


extern "C"
{
SQLRETURN SQL_API SQLNativeSqlA(
    SQLHDBC            ConnectionHandle,
    SQLCHAR           *InStatementText,
    SQLINTEGER         TextLength1,
    SQLCHAR           *OutStatementText,
    SQLINTEGER         BufferLength,
    SQLINTEGER           *TextLength2Ptr);
}


extern "C"
{
SQLRETURN SQL_API SQLColAttributeA(SQLHSTMT StatementHandle,
           SQLSMALLINT ColumnNumber,
           SQLSMALLINT FieldIdentifier,
           SQLPOINTER   CharacterAttributePtr,
           SQLSMALLINT  BufferLength,
           SQLSMALLINT *StringLengthPtr,
           SQLLEN      *NumericAttributePtr);
}


extern "C"
{
SQLRETURN  SQL_API SQLProceduresA(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName,
           SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName,
           SQLSMALLINT NameLength2,
           SQLCHAR *ProcName,
           SQLSMALLINT NameLength3);
}


extern "C"
{
SQLRETURN  SQL_API SQLProcedureColumnsA(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName,
           SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName,
           SQLSMALLINT NameLength2,
           SQLCHAR *ProcName,
           SQLSMALLINT NameLength3,
           SQLCHAR *ColumnName,
           SQLSMALLINT NameLength4);
}


extern "C"
{
SQLRETURN SQL_API SQLColumnPrivilegesA(SQLHSTMT StatementHandle,
     SQLCHAR* CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR* SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR* TableName,
     SQLSMALLINT NameLength3,
     SQLCHAR* ColumnName,
     SQLSMALLINT NameLength4);
}


extern "C"
{
SQLRETURN SQL_API SQLForeignKeysA(SQLHSTMT StatementHandle,
     SQLCHAR *PKCatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *PKSchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *PKTableName,
     SQLSMALLINT NameLength3,
     SQLCHAR *FKCatalogName,
     SQLSMALLINT NameLength4,
     SQLCHAR *FKSchemaName,
     SQLSMALLINT NameLength5,
     SQLCHAR *FKTableName,
     SQLSMALLINT NameLength6);
}


extern "C"
{
SQLRETURN SQL_API SQLTablePrivilegesA(
     SQLHSTMT StatementHandle,
     SQLCHAR *CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *TableName,
     SQLSMALLINT NameLength3);
}


//Unicode Function
extern "C"
{
SQLRETURN SQL_API SQLGetDiagRecW(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber,
				SQLWCHAR *Sqlstate,
				SQLINTEGER *NativeError, 
				SQLWCHAR *MessageText,
				SQLSMALLINT BufferLength, 
				SQLSMALLINT *TextLength);
}


extern "C"
{
SQLRETURN SQL_API SQLGetDiagFieldW(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber, 
				SQLSMALLINT DiagIdentifier,
				SQLPOINTER DiagInfo, 
				SQLSMALLINT BufferLength,
				SQLSMALLINT *StringLength);
}


extern "C"
{
SQLRETURN SQL_API SQLEndTran(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT CompletionType);
}


extern "C"
{
SQLRETURN SQL_API SQLConnectW(SQLHDBC ConnectionHandle,
           SQLWCHAR *ServerName, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *UserName, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *Authentication, 
		   SQLSMALLINT NameLength3);
}


extern "C"
{
SQLRETURN SQL_API SQLDisconnect(SQLHDBC ConnectionHandle);
}


extern "C"
{
SQLRETURN  SQL_API SQLSetConnectAttrW(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength);
}


extern "C"
{
SQLRETURN  SQL_API SQLGetConnectAttrW(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength);
}


extern "C"
{
SQLRETURN  SQL_API SQLSetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength);
}


extern "C"
{
SQLRETURN  SQL_API SQLGetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength);
}


extern "C"
{
SQLRETURN  SQL_API SQLSetStmtAttrW(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength);
}


extern "C"
{
SQLRETURN  SQL_API SQLGetStmtAttrW(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength);
}


extern "C"
{
SQLRETURN  SQL_API SQLGetInfoW(SQLHDBC ConnectionHandle,
           SQLUSMALLINT InfoType, 
		   SQLPOINTER InfoValuePtr,
           SQLSMALLINT BufferLength,
		   SQLSMALLINT *StringLengthPtr);
}


extern "C"
{
SQLRETURN  SQL_API SQLSetDescFieldW(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength);
}


extern "C"
{
SQLRETURN  SQL_API SQLSetDescRec(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber,
		   SQLSMALLINT Type,
           SQLSMALLINT SubType, 
		   SQLLEN Length,
           SQLSMALLINT Precision, 
		   SQLSMALLINT Scale,
           SQLPOINTER Data, 
		   SQLLEN *StringLengthPtr,
           SQLLEN *IndicatorPtr);
}


extern "C"
{
SQLRETURN  SQL_API SQLGetDescFieldW(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength,
           SQLINTEGER *StringLengthPtr);
}


extern "C"
{
SQLRETURN  SQL_API SQLGetDescRecW(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber,
		   SQLWCHAR *Name,
           SQLSMALLINT BufferLength, 
		   SQLSMALLINT *StringLengthPtr,
           SQLSMALLINT *TypePtr, 
		   SQLSMALLINT *SubTypePtr, 
           SQLLEN     *LengthPtr, 
		   SQLSMALLINT *PrecisionPtr, 
           SQLSMALLINT *ScalePtr, 
		   SQLSMALLINT *NullablePtr);
}


extern "C"
{
SQLRETURN  SQL_API SQLBindCol(SQLHSTMT StatementHandle, 
		   SQLUSMALLINT ColumnNumber, 
		   SQLSMALLINT TargetType, 
		   SQLPOINTER TargetValue, 
		   SQLLEN BufferLength, 
	   	   SQLLEN *StrLen_or_IndPtr);
}


extern "C"
{
SQLRETURN SQL_API SQLBindParameter(SQLHSTMT StatementHandle,
			SQLUSMALLINT ParameterNumber, 
			SQLSMALLINT InputOutputType,
			SQLSMALLINT ValueType,
			SQLSMALLINT ParameterType, 
			SQLUINTEGER	ColumnSize,
			SQLSMALLINT DecimalDigits,
			SQLPOINTER  ParameterValuePtr,
			SQLINTEGER	BufferLength,
			SQLLEN *StrLen_or_IndPtr);		   
}


extern "C"
{
SQLRETURN SQL_API SQLBrowseConnectW(
    SQLHDBC            ConnectionHandle,
    SQLWCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLWCHAR 		  *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT       *StringLength2Ptr);
}


extern "C"
{
SQLRETURN SQL_API SQLDriverConnectW(SQLHDBC  ConnectionHandle,
    SQLHWND            WindowHandle,
    SQLWCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLWCHAR           *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT 	  *StringLength2Ptr,
    SQLUSMALLINT       DriverCompletion);
}


extern "C"
{
SQLRETURN SQL_API SQLPrepareW(SQLHSTMT StatementHandle,
           SQLWCHAR *StatementText,
		   SQLINTEGER TextLength);
}


extern "C"
{
SQLRETURN SQL_API SQLExecDirectW(SQLHSTMT StatementHandle,
           SQLWCHAR *StatementText,
		   SQLINTEGER TextLength);
}


extern "C"
{
SQLRETURN  SQL_API SQLDescribeColW(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLWCHAR *ColumnName,
           SQLSMALLINT BufferLength, 
		   SQLSMALLINT *NameLengthPtr,
           SQLSMALLINT *DataTypePtr, 
		   SQLULEN *ColumnSizePtr,
           SQLSMALLINT *DecimalDigitsPtr,
		   SQLSMALLINT *NullablePtr);
}


extern "C"
{
SQLRETURN  SQL_API SQLNumResultCols(SQLHSTMT StatementHandle,
           SQLSMALLINT *ColumnCountPtr);
}


extern "C"
{
SQLRETURN  SQL_API SQLNumParams(SQLHSTMT StatementHandle,
           SQLSMALLINT *ParameterCountPtr);
}


extern "C"
{
SQLRETURN SQL_API SQLDescribeParam(
	SQLHSTMT           StatementHandle,
	SQLUSMALLINT       ParameterNumber,
    SQLSMALLINT 	  *DataTypePtr,
    SQLULEN      	  *ParameterSizePtr,
    SQLSMALLINT 	  *DecimalDigitsPtr,
    SQLSMALLINT 	  *NullablePtr);
}


extern "C"
{
SQLRETURN SQL_API SQLFreeStmt(SQLHSTMT StatementHandle,
        SQLUSMALLINT Option);
}


extern "C"
{
SQLRETURN SQL_API SQLCloseCursor(SQLHSTMT StatementHandle);
}


extern "C"
{
SQLRETURN  SQL_API SQLTablesW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableName, 
		   SQLSMALLINT NameLength3,
           SQLWCHAR *TableType, 
		   SQLSMALLINT NameLength4);
}


extern "C"
{
SQLRETURN  SQL_API SQLColumnsW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableName, 
		   SQLSMALLINT NameLength3,
           SQLWCHAR *ColumnName, 
		   SQLSMALLINT NameLength4);
}


extern "C"
{
SQLRETURN  SQL_API SQLSpecialColumnsW(SQLHSTMT StatementHandle,
		   SQLUSMALLINT IdentifierType,
           SQLWCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   SQLUSMALLINT Scope,
		   SQLUSMALLINT Nullable);
}


extern "C"
{
SQLRETURN  SQL_API SQLGetTypeInfoW(SQLHSTMT StatementHandle,
           SQLSMALLINT DataType);
}


extern "C"
{
SQLRETURN  SQL_API SQLPrimaryKeysW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableName, 
		   SQLSMALLINT NameLength3);
}


extern "C"
{
SQLRETURN  SQL_API SQLStatisticsW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   SQLUSMALLINT Unique,
		   SQLUSMALLINT Reserved);
}


extern "C"
{
SQLRETURN  SQL_API SQLGetCursorNameW(SQLHSTMT StatementHandle,
           SQLWCHAR *CursorName, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLengthPtr);
}


extern "C"
{
SQLRETURN  SQL_API SQLSetCursorNameW(SQLHSTMT StatementHandle,
           SQLWCHAR *CursorName, 
		   SQLSMALLINT NameLength);
}


extern "C"
{
SQLRETURN  SQL_API SQLRowCount(SQLHSTMT StatementHandle, 
	   SQLLEN	*RowCountPtr);
}


extern "C"
{
SQLRETURN SQL_API SQLMoreResults(
    SQLHSTMT           StatementHandle);
}


extern "C"
{
SQLRETURN SQL_API SQLNativeSqlW(
    SQLHDBC            ConnectionHandle,
    SQLWCHAR 		  *InStatementText,
    SQLINTEGER         TextLength1,
    SQLWCHAR 		  *OutStatementText,
    SQLINTEGER         BufferLength,
    SQLINTEGER 		  *TextLength2Ptr);
}


extern "C"
{
SQLRETURN SQL_API SQLCancel(SQLHSTMT StatementHandle);
}


extern "C"
{
#if defined(_WIN64) || defined(__LP64__)
SQLRETURN SQL_API SQLColAttributeW(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttributePtr, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLengthPtr, 
		   SQLLEN *NumericAttributePtr);
#else
SQLRETURN SQL_API SQLColAttributeW(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttributePtr, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLengthPtr, 
		   SQLPOINTER NumericAttributePtr);
#endif
}


extern "C"
{
SQLRETURN SQL_API SQLExecute(SQLHSTMT StatementHandle);
}


extern "C"
{
SQLRETURN  SQL_API SQLParamData(SQLHSTMT StatementHandle,
           SQLPOINTER *ValuePtrPtr);
}


extern "C"
{
SQLRETURN  SQL_API SQLPutData(SQLHSTMT StatementHandle,
           SQLPOINTER DataPtr, 
		   SQLLEN StrLen_or_Ind);
}


extern "C"
{
SQLRETURN SQL_API SQLFetch(SQLHSTMT StatementHandle);
}


extern "C"
{
SQLRETURN SQL_API SQLGetData(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLSMALLINT TargetType,
           SQLPOINTER TargetValuePtr, 
		   SQLLEN BufferLength,
           SQLLEN *StrLen_or_IndPtr);
}


extern "C"
{
SQLRETURN SQL_API SQLSetPos(
    SQLHSTMT        StatementHandle,
    SQLSETPOSIROW	RowNumber,
    SQLUSMALLINT    Operation,
    SQLUSMALLINT    LockType);
}


extern "C"
{
SQLRETURN SQL_API SQLCopyDesc(SQLHDESC SourceDescHandle,
		SQLHDESC TargetDescHandle);
}


extern "C"
{
SQLRETURN SQL_API SQLColumnPrivilegesW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
     SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
     SQLSMALLINT NameLength2,
     SQLWCHAR* TableName,
     SQLSMALLINT NameLength3,
           SQLWCHAR *ColumnName, 
     SQLSMALLINT NameLength4);
}


extern "C"
{
SQLRETURN SQL_API SQLForeignKeysW(SQLHSTMT StatementHandle,
     SQLWCHAR *PKCatalogName,
     SQLSMALLINT NameLength1,
     SQLWCHAR *PKSchemaName,
     SQLSMALLINT NameLength2,
     SQLWCHAR *PKTableName,
     SQLSMALLINT NameLength3,
     SQLWCHAR *FKCatalogName,
     SQLSMALLINT NameLength4,
     SQLWCHAR *FKSchemaName,
     SQLSMALLINT NameLength5,
     SQLWCHAR *FKTableName,
     SQLSMALLINT NameLength6);
}


extern "C"
{
SQLRETURN SQL_API SQLProcedureColumnsW(
     SQLHSTMT     StatementHandle,
           SQLWCHAR *CatalogName, 
     SQLSMALLINT     NameLength1,
           SQLWCHAR *SchemaName, 
     SQLSMALLINT     NameLength2,
           SQLWCHAR *ProcName, 
     SQLSMALLINT     NameLength3,
           SQLWCHAR *ColumnName, 
     SQLSMALLINT     NameLength4);
}


extern "C"
{
SQLRETURN SQL_API SQLProceduresW(
     SQLHSTMT     StatementHandle,
           SQLWCHAR *CatalogName, 
     SQLSMALLINT     NameLength1,
           SQLWCHAR *SchemaName, 
     SQLSMALLINT     NameLength2,
           SQLWCHAR *ProcName, 
     SQLSMALLINT     NameLength3);
}


extern "C"
{
SQLRETURN SQL_API SQLTablePrivilegesW(
	 SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
     SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
     SQLSMALLINT NameLength2,
     SQLWCHAR* TableName,
     SQLSMALLINT NameLength3);
}


extern "C"
{
SQLRETURN SQL_API SQLExtendedFetch(
	 SQLHSTMT StatementHandle,
     SQLUSMALLINT FetchOrientation,
     SQLLEN FetchOffset,
     SQLULEN *RowCountPtr,
     SQLUSMALLINT *RowStatusArray);
}


extern "C"
{
SQLRETURN SQL_API SQLBulkOperations(
	SQLHSTMT StatementHandle,
    SQLSMALLINT Operation);
}


extern "C"
{
SQLRETURN SQL_API SQLFetchScroll(
	SQLHSTMT StatementHandle,
    SQLSMALLINT FetchOrientation,
    SQLLEN FetchOffset);
}

#endif


