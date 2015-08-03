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
#ifndef SQLSTMT_H
#define SQLSTMT_H

#include <windows.h>
#include <sql.h>
#include "drvrnet.h"

namespace ODBC {

extern SQLRETURN SetStmtAttr(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength);

extern SQLRETURN GetStmtAttr(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER ValuePtr,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLengthPtr);

extern SQLRETURN BindCol(SQLHSTMT StatementHandle, 
		   SQLUSMALLINT ColumnNumber, 
		   SQLSMALLINT TargetType, 
		   SQLPOINTER TargetValue, 
		   SQLLEN BufferLength, 
	   	   SQLLEN *StrLen_or_Ind);

extern SQLRETURN BindParameter(SQLHSTMT StatementHandle,
			SQLUSMALLINT ParameterNumber, 
			SQLSMALLINT InputOutputType,
			SQLSMALLINT ValueType,
			SQLSMALLINT ParameterType, 
			SQLULEN	ColumnSize,
			SQLSMALLINT DecimalDigits,
			SQLPOINTER  ParameterValuePtr,
			SQLLEN   	BufferLength,
			SQLLEN      *StrLen_or_IndPtr);

extern SQLRETURN Prepare(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength);

extern SQLRETURN WMPrepare(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength);

extern SQLRETURN ExecDirect(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength);

extern SQLRETURN getDescRec(SQLHSTMT StatementHandle,
		short odbcAPI,
		SQLUSMALLINT ColumnNumber,
		SQLCHAR *ColumnName,
        SQLSMALLINT BufferLength, 
		SQLSMALLINT *NameLengthPtr,
        SQLSMALLINT *DataTypePtr, 
		SQLULEN *ColumnSizePtr,
        SQLSMALLINT *DecimalDigitsPtr,
		SQLSMALLINT *NullablePtr);

extern SQLRETURN getDescSize(SQLHSTMT StatementHandle,
        short odbcAPI,
		SQLSMALLINT *ColumnCountPtr);

extern SQLRETURN FreeStmt(SQLHSTMT StatementHandle,
		short odbcAPI,
        SQLUSMALLINT Option);

extern SQLRETURN GetSQLCatalogs(SQLHSTMT StatementHandle,
		   short odbcAPI,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
           SQLCHAR *ColumnName = NULL, 
		   SQLSMALLINT NameLength4 = SQL_NTS,
           SQLCHAR *TableType = NULL, 
		   SQLSMALLINT NameLength5 = SQL_NTS,
		   SQLUSMALLINT IdentifierType =0,
		   SQLUSMALLINT Scope = 0,
		   SQLUSMALLINT Nullable = 0,
		   SQLSMALLINT SqlType = 0,
		   SQLUSMALLINT Unique = 0,
		   SQLUSMALLINT Reserved = 0,
           SQLCHAR *FKCatalogName = NULL, 
		   SQLSMALLINT NameLength6 = SQL_NTS,
           SQLCHAR *FKSchemaName = NULL, 
		   SQLSMALLINT NameLength7 = SQL_NTS,
           SQLCHAR *FKTableName = NULL, 
		   SQLSMALLINT NameLength8 = SQL_NTS);

extern SQLRETURN GetCursorName(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLengthPtr);

extern SQLRETURN SetCursorName(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, 
		   SQLSMALLINT NameLength);

extern SQLRETURN RowCount(SQLHSTMT StatementHandle, 
	   SQLLEN	*RowCountPtr);

extern SQLRETURN Cancel(SQLHSTMT StatementHandle);

extern SQLRETURN ColAttribute(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttributePtr, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLengthPtr, 
		   SQLPOINTER NumericAttributePtr);

extern SQLRETURN Execute(SQLHSTMT StatementHandle);

extern SQLRETURN WMExecute(SQLHSTMT StatementHandle);

extern SQLRETURN ParamData(SQLHSTMT StatementHandle,
		SQLPOINTER *ValuePtrPtr);

extern SQLRETURN PutData(SQLHSTMT StatementHandle,
           SQLPOINTER DataPtr, 
		   SQLINTEGER StrLen_or_Ind);

extern SQLRETURN Fetch(SQLHSTMT StatementHandle);

extern SQLRETURN FetchScroll(SQLHSTMT StatementHandle,
			SQLUSMALLINT FetchOrientation,
			SQLINTEGER FetchOffset);

extern SQLRETURN ExtendedFetch(SQLHSTMT StatementHandle,
			SQLUSMALLINT FetchOrientation,
			SQLLEN FetchOffset,
			SQLULEN* RowCountPtr,
			SQLUSMALLINT* RowStatusArray);

extern SQLRETURN GetData(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLSMALLINT TargetType,
           SQLPOINTER TargetValuePtr, 
		   SQLLEN     BufferLength,
           SQLLEN    *StrLen_or_IndPtr);

extern SQLRETURN SetPos(SQLHSTMT StatementHandle,
    SQLUSMALLINT	RowNumber,
    SQLUSMALLINT    Operation,
    SQLUSMALLINT    LockType);

}

#endif
