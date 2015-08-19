/**********************************************************************
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
********************************************************************/

#ifndef __NEOFUNC_H
#define __NEOFUNC_H

#include "drvrglobal.h"
#include "sqlhandle.h"
#include "sqlconnect.h"
#include "sqlenv.h"
#include "sqlstmt.h"
#include "sqldesc.h"
#include "diagfunctions.h"
#include "DrvrSrvr.h"

SQLRETURN  SQL_API NeoAllocHandle(SQLSMALLINT HandleType,
				SQLHANDLE InputHandle, 
				SQLHANDLE *OutputHandle);

SQLRETURN  SQL_API NeoFreeHandle(SQLSMALLINT HandleType, 
				SQLHANDLE Handle);

SQLRETURN SQL_API NeoGetDiagRec(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber,
				SQLCHAR *Sqlstate,
				SQLINTEGER *NativeError, 
				SQLCHAR *MessageText,
				SQLSMALLINT BufferLength, 
				SQLSMALLINT *TextLength,
				bool isWideCall);

SQLRETURN SQL_API NeoGetDiagField(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber, 
				SQLSMALLINT DiagIdentifier,
				SQLPOINTER DiagInfo, 
				SQLSMALLINT BufferLength,
				SQLSMALLINT *StringLength,
				bool isWideCall);

SQLRETURN SQL_API NeoEndTran(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT CompletionType);

SQLRETURN SQL_API NeoConnect(SQLHDBC ConnectionHandle,
           SQLCHAR *ServerName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *UserName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *Authentication, 
		   SQLSMALLINT NameLength3,
		   bool isWideCall);

SQLRETURN SQL_API NeoDisconnect(SQLHDBC ConnectionHandle);
 
SQLRETURN  SQL_API NeoSetConnectAttr(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength,
		   bool isWideCall);

SQLRETURN  SQL_API NeoGetConnectAttr(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength,
		   bool isWideCall);

SQLRETURN  SQL_API NeoSetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength);

SQLRETURN  SQL_API NeoGetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength);

SQLRETURN  SQL_API NeoSetStmtAttr(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength,
		   bool isWideCall);

SQLRETURN  SQL_API NeoGetStmtAttr(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength,
		   bool isWideCall);

SQLRETURN  SQL_API NeoGetInfo(SQLHDBC ConnectionHandle,
           SQLUSMALLINT InfoType, 
		   SQLPOINTER InfoValuePtr,
           SQLSMALLINT BufferLength,
		   SQLSMALLINT *StringLengthPtr,
		   bool isWideCall);

SQLRETURN  SQL_API NeoSetDescField(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength,
		   bool isWideCall);

SQLRETURN  SQL_API NeoSetDescRec(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber,
		   SQLSMALLINT Type,
           SQLSMALLINT SubType, 
		   SQLLEN Length,
           SQLSMALLINT Precision, 
		   SQLSMALLINT Scale,
           SQLPOINTER Data, 
		   SQLLEN *StringLengthPtr,
           SQLLEN *IndicatorPtr);

SQLRETURN  SQL_API NeoGetDescField(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength,
           SQLINTEGER *StringLengthPtr,
		   bool isWideCall);

SQLRETURN  SQL_API NeoGetDescRec(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLCHAR *Name,
           SQLSMALLINT BufferLength, 
		   SQLSMALLINT *StringLengthPtr,
           SQLSMALLINT *TypePtr, 
		   SQLSMALLINT *SubTypePtr, 
           SQLLEN     *LengthPtr, 
		   SQLSMALLINT *PrecisionPtr, 
           SQLSMALLINT *ScalePtr, 
		   SQLSMALLINT *NullablePtr,
		   bool isWideCall);

SQLRETURN  SQL_API NeoBindCol(SQLHSTMT StatementHandle, 
		   SQLUSMALLINT ColumnNumber, 
		   SQLSMALLINT TargetType, 
		   SQLPOINTER TargetValue, 
		   SQLLEN BufferLength, 
	   	   SQLLEN *StrLen_or_IndPtr);

SQLRETURN SQL_API NeoBindParameter(SQLHSTMT StatementHandle,
			SQLUSMALLINT ParameterNumber, 
			SQLSMALLINT InputOutputType,
			SQLSMALLINT ValueType,
			SQLSMALLINT ParameterType, 
			SQLULEN  	ColumnSize,
			SQLSMALLINT DecimalDigits,
			SQLPOINTER  ParameterValuePtr,
			SQLLEN	BufferLength,
			SQLLEN *StrLen_or_IndPtr);		   

SQLRETURN SQL_API NeoBrowseConnect(
    SQLHDBC            ConnectionHandle,
    SQLCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLCHAR 		  *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT       *StringLength2Ptr,
    bool               isWideCall);

SQLRETURN SQL_API NeoDriverConnect(SQLHDBC  ConnectionHandle,
    SQLHWND            WindowHandle,
    SQLCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLCHAR           *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT 	  *StringLength2Ptr,
    SQLUSMALLINT       DriverCompletion,
    bool               isWideCall);

SQLRETURN SQL_API NeoPrepare(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength,
		   bool isWideCall);

SQLRETURN SQL_API NeoExecDirect(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength,
		   bool isWideCall);

SQLRETURN  SQL_API NeoDescribeCol(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLCHAR *ColumnName,
           SQLSMALLINT BufferLength, 
		   SQLSMALLINT *NameLengthPtr,
           SQLSMALLINT *DataTypePtr, 
		   SQLULEN *ColumnSizePtr,
           SQLSMALLINT *DecimalDigitsPtr,
		   SQLSMALLINT *NullablePtr,
		   bool isWideCall);

SQLRETURN  SQL_API NeoNumResultCols(SQLHSTMT StatementHandle,
           SQLSMALLINT *ColumnCountPtr);

SQLRETURN  SQL_API NeoNumParams(SQLHSTMT StatementHandle,
           SQLSMALLINT *ParameterCountPtr);

SQLRETURN SQL_API NeoDescribeParam(
	SQLHSTMT           StatementHandle,
	SQLUSMALLINT       ParameterNumber,
    SQLSMALLINT 	  *DataTypePtr,
    SQLULEN      	  *ParameterSizePtr,
    SQLSMALLINT 	  *DecimalDigitsPtr,
    SQLSMALLINT 	  *NullablePtr);

SQLRETURN SQL_API NeoFreeStmt(SQLHSTMT StatementHandle,
        SQLUSMALLINT Option);

SQLRETURN SQL_API NeoCloseCursor(SQLHSTMT StatementHandle);

SQLRETURN  SQL_API NeoTables(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
           SQLCHAR *TableType, 
		   SQLSMALLINT NameLength4,
		   bool isWideCall);

SQLRETURN  SQL_API NeoColumns(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
           SQLCHAR *ColumnName, 
		   SQLSMALLINT NameLength4,
		   bool isWideCall);

SQLRETURN  SQL_API NeoSpecialColumns(SQLHSTMT StatementHandle,
		   SQLUSMALLINT IdentifierType,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   SQLUSMALLINT Scope,
		   SQLUSMALLINT Nullable,
		   bool isWideCall);

SQLRETURN  SQL_API NeoGetTypeInfo(SQLHSTMT StatementHandle,
           SQLSMALLINT DataType,
		   bool isWideCall);

SQLRETURN  SQL_API NeoPrimaryKeys(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   bool isWideCall);

SQLRETURN  SQL_API NeoStatistics(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   SQLUSMALLINT Unique,
		   SQLUSMALLINT Reserved,
		   bool isWideCall);

SQLRETURN  SQL_API NeoGetCursorName(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLengthPtr,
		   bool isWideCall);

SQLRETURN  SQL_API NeoSetCursorName(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, 
		   SQLSMALLINT NameLength,
		   bool isWideCall);
		
SQLRETURN  SQL_API NeoRowCount(SQLHSTMT StatementHandle, 
	   SQLLEN	*RowCountPtr);

SQLRETURN SQL_API NeoMoreResults(
    SQLHSTMT           StatementHandle);

SQLRETURN SQL_API NeoNativeSql(
    SQLHDBC            ConnectionHandle,
    SQLCHAR 		  *InStatementText,
    SQLINTEGER         TextLength1,
    SQLCHAR 		  *OutStatementText,
    SQLINTEGER         BufferLength,
    SQLINTEGER 		  *TextLength2Ptr,
	bool              isWideCall);

SQLRETURN SQL_API NeoCancel(SQLHSTMT StatementHandle);

SQLRETURN SQL_API NeoColAttribute(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttributePtr, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLengthPtr, 
		   SQLPOINTER NumericAttributePtr,
		   bool isWideCall);

SQLRETURN SQL_API NeoExecute(SQLHSTMT StatementHandle);

SQLRETURN  SQL_API NeoParamData(SQLHSTMT StatementHandle,
           SQLPOINTER *ValuePtrPtr);

SQLRETURN  SQL_API NeoPutData(SQLHSTMT StatementHandle,
           SQLPOINTER DataPtr, 
		   SQLLEN StrLen_or_Ind);

SQLRETURN SQL_API NeoFetch(SQLHSTMT StatementHandle);

SQLRETURN SQL_API NeoGetData(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLSMALLINT TargetType,
           SQLPOINTER TargetValuePtr, 
		   SQLLEN     BufferLength,
           SQLLEN    *StrLen_or_IndPtr);

SQLRETURN SQL_API NeoSetPos(
    SQLHSTMT        StatementHandle,
    SQLSETPOSIROW 	RowNumber,
    SQLUSMALLINT    Operation,
    SQLUSMALLINT    LockType);

SQLRETURN SQL_API NeoCopyDesc(SQLHDESC SourceDescHandle,
		SQLHDESC TargetDescHandle);

//================== APIs not implemented ==========================

SQLRETURN SQL_API NeoColumnPrivileges(SQLHSTMT StatementHandle,
     SQLCHAR* CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR* SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR* TableName,
     SQLSMALLINT NameLength3,
     SQLCHAR* ColumnName,
     SQLSMALLINT NameLength4,
	 bool isWideCall);

SQLRETURN SQL_API NeoForeignKeys(SQLHSTMT StatementHandle,
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
     SQLSMALLINT NameLength6,
	 bool isWideCall);

SQLRETURN SQL_API NeoProcedureColumns(
     SQLHSTMT     StatementHandle,
     SQLCHAR *     CatalogName,
     SQLSMALLINT     NameLength1,
     SQLCHAR *     SchemaName,
     SQLSMALLINT     NameLength2,
     SQLCHAR *     ProcName,
     SQLSMALLINT     NameLength3,
     SQLCHAR *     ColumnName,
     SQLSMALLINT     NameLength4,
	 bool isWideCall);

SQLRETURN SQL_API NeoProcedures(
     SQLHSTMT     StatementHandle,
     SQLCHAR *     CatalogName,
     SQLSMALLINT     NameLength1,
     SQLCHAR *     SchemaName,
     SQLSMALLINT     NameLength2,
     SQLCHAR *     ProcName,
     SQLSMALLINT     NameLength3,
	 bool isWideCall);

SQLRETURN SQL_API NeoTablePrivileges(
	 SQLHSTMT StatementHandle,
     SQLCHAR *CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *TableName,
     SQLSMALLINT NameLength3,
	 bool isWideCall);

SQLRETURN SQL_API NeoExtendedFetch(
	 SQLHSTMT StatementHandle,
     SQLUSMALLINT FetchOrientation,
     SQLLEN FetchOffset,
     SQLULEN *RowCountPtr,
     SQLUSMALLINT *RowStatusArray);

SQLRETURN SQL_API NeoBulkOperations(
	SQLHSTMT StatementHandle,
    SQLSMALLINT Operation);

SQLRETURN SQL_API NeoFetchScroll(
	SQLHSTMT StatementHandle,
    SQLSMALLINT FetchOrientation,
    SQLLEN FetchOffset);



#endif // __NEOFUNC_H
