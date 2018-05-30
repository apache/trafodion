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
#include "drvrglobal.h"
#include "sqlhandle.h"
#include "sqlconnect.h"
#include "sqlenv.h"
#include "sqlstmt.h"
#include "sqldesc.h"
#include "diagfunctions.h"
#include "DrvrSrvr.h"
#include "cstmt.h"
#include "neofunc.h"

//
#include <math.h>
//
extern "C" 
{
SQLRETURN  SQL_API SQLAllocHandle(SQLSMALLINT HandleType,
				SQLHANDLE InputHandle, 
				SQLHANDLE *OutputHandle)
{
	 return NeoAllocHandle(HandleType, InputHandle, OutputHandle);
}
} // extern "C"


extern "C" 
{
SQLRETURN  SQL_API SQLFreeHandle(SQLSMALLINT HandleType, 
				SQLHANDLE Handle)
{
	return NeoFreeHandle(HandleType, Handle);
}
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
                SQLSMALLINT *TextLength)
{
    return NeoGetDiagRec(HandleType, Handle, RecNumber, (SQLCHAR*)Sqlstate, NativeError, (SQLCHAR*)MessageText, BufferLength, TextLength, false);
}
}


extern "C"
{
SQLRETURN SQL_API SQLGetDiagFieldA(SQLSMALLINT HandleType,
                SQLHANDLE Handle,
                SQLSMALLINT RecNumber,
                SQLSMALLINT DiagIdentifier,
                SQLPOINTER DiagInfo,
                SQLSMALLINT BufferLength,
                SQLSMALLINT *StringLength)
{
    return NeoGetDiagField(HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength, false);
}
}


extern "C"
{
SQLRETURN SQL_API SQLConnectA(SQLHDBC ConnectionHandle,
           SQLCHAR *ServerName,
           SQLSMALLINT NameLength1,
           SQLCHAR *UserName,
           SQLSMALLINT NameLength2,
           SQLCHAR *Authentication,
           SQLSMALLINT NameLength3)
{
    return NeoConnect(ConnectionHandle, (SQLCHAR*)ServerName, NameLength1, (SQLCHAR*)UserName, NameLength2, (SQLCHAR*)Authentication, NameLength3, false);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLSetConnectAttrA(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute,
           SQLPOINTER Value,
           SQLINTEGER StringLength)
{
    return NeoSetConnectAttr(ConnectionHandle, Attribute, Value, StringLength, false);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLGetConnectAttrA(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute,
           SQLPOINTER Value,
           SQLINTEGER BufferLength,
           SQLINTEGER *StringLength)
{
    return NeoGetConnectAttr(ConnectionHandle, Attribute, Value, BufferLength, StringLength, false);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLSetStmtAttrA(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute,
           SQLPOINTER Value,
           SQLINTEGER StringLength)
{
    return NeoSetStmtAttr(StatementHandle, Attribute, Value, StringLength, false);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLGetStmtAttrA(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute,
           SQLPOINTER Value,
           SQLINTEGER BufferLength,
           SQLINTEGER *StringLength)
{
    return NeoGetStmtAttr(StatementHandle, Attribute, Value, BufferLength, StringLength, false);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLGetInfoA(SQLHDBC ConnectionHandle,
           SQLUSMALLINT InfoType,
           SQLPOINTER InfoValuePtr,
           SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLengthPtr)
{
    return NeoGetInfo(ConnectionHandle, InfoType, InfoValuePtr, BufferLength, StringLengthPtr, false);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLSetDescFieldA(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber,
           SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr,
           SQLINTEGER BufferLength)
{
    return NeoSetDescField(DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength, false);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLGetDescFieldA(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber,
           SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr,
           SQLINTEGER BufferLength,
           SQLINTEGER *StringLengthPtr)
{
    return NeoGetDescField(DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength, StringLengthPtr, false);
}
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
           SQLSMALLINT *NullablePtr)
{
    return NeoGetDescRec(DescriptorHandle, RecNumber, Name, BufferLength, StringLengthPtr, TypePtr, SubTypePtr, LengthPtr,
        PrecisionPtr, ScalePtr, NullablePtr, false);
}
}


extern "C"
{
SQLRETURN SQL_API SQLBrowseConnectA(
    SQLHDBC            ConnectionHandle,
    SQLCHAR           *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLCHAR           *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT       *StringLength2Ptr)
{
    return NeoBrowseConnect(ConnectionHandle, InConnectionString, StringLength1, OutConnectionString, BufferLength, StringLength2Ptr, false);
}
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
    SQLUSMALLINT       DriverCompletion)
{
    return NeoDriverConnect(ConnectionHandle, WindowHandle, InConnectionString, StringLength1,
        OutConnectionString, BufferLength, StringLength2Ptr, DriverCompletion, false);
}
}


extern "C"
{
SQLRETURN SQL_API SQLPrepareA(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
           SQLINTEGER TextLength)
{
    return NeoPrepare(StatementHandle, StatementText, TextLength, false);
}
}


extern "C"
{
SQLRETURN SQL_API SQLExecDirectA(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
           SQLINTEGER TextLength)
{
    return NeoExecDirect(StatementHandle, (SQLCHAR*)StatementText, TextLength, false);
}
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
           SQLSMALLINT *NullablePtr)
{
    return NeoDescribeCol(StatementHandle, ColumnNumber, ColumnName, BufferLength, NameLengthPtr, DataTypePtr,
        ColumnSizePtr, DecimalDigitsPtr, NullablePtr, false);
}
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
           SQLSMALLINT NameLength4)
{
    return NeoTables(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3, TableType, NameLength4, false);
}
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
           SQLSMALLINT NameLength4)
{
    return NeoColumns(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3, ColumnName, NameLength4, false);
}
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
           SQLUSMALLINT Nullable)
{
    return NeoSpecialColumns(StatementHandle, IdentifierType, CatalogName, NameLength1, SchemaName, NameLength2, TableName, 
           NameLength3, Scope, Nullable, false);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLGetTypeInfoA(SQLHSTMT StatementHandle,
           SQLSMALLINT DataType)
{
    return NeoGetTypeInfo(StatementHandle, DataType, false);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLPrimaryKeysA(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName,
           SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName,
           SQLSMALLINT NameLength2,
           SQLCHAR *TableName,
           SQLSMALLINT NameLength3)
{
    return NeoPrimaryKeys(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3, false);
}
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
           SQLUSMALLINT Reserved)
{
    return NeoStatistics(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
           Unique, Reserved, false);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLGetCursorNameA(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName,
           SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLengthPtr)
{
    return NeoGetCursorName(StatementHandle, CursorName, BufferLength, NameLengthPtr, false);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLSetCursorNameA(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName,
           SQLSMALLINT NameLength)
{
    return NeoSetCursorName(StatementHandle, CursorName, NameLength, false);
}
}


extern "C"
{
SQLRETURN SQL_API SQLNativeSqlA(
    SQLHDBC            ConnectionHandle,
    SQLCHAR           *InStatementText,
    SQLINTEGER         TextLength1,
    SQLCHAR           *OutStatementText,
    SQLINTEGER         BufferLength,
    SQLINTEGER           *TextLength2Ptr)
{
    return NeoNativeSql(ConnectionHandle, InStatementText, TextLength1, OutStatementText, BufferLength, TextLength2Ptr, false);
}
}


extern "C"
{
SQLRETURN SQL_API SQLColAttributeA(SQLHSTMT StatementHandle,
           SQLSMALLINT ColumnNumber,
           SQLSMALLINT FieldIdentifier,
           SQLPOINTER   CharacterAttributePtr,
           SQLSMALLINT  BufferLength,
           SQLSMALLINT *StringLengthPtr,
           SQLLEN      *NumericAttributePtr)
{
    return NeoColAttribute(StatementHandle, ColumnNumber, FieldIdentifier, CharacterAttributePtr,
           BufferLength, StringLengthPtr, NumericAttributePtr, false);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLProceduresA(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName,
           SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName,
           SQLSMALLINT NameLength2,
           SQLCHAR *ProcName,
           SQLSMALLINT NameLength3)
{
    return NeoProcedures(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, ProcName, NameLength3, false);
}
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
           SQLSMALLINT NameLength4)
{
    return NeoProcedureColumns(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2,
               ProcName, NameLength3, ColumnName, NameLength4, false);
}
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
     SQLSMALLINT NameLength4)
{
    return NeoColumnPrivileges(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2,
            TableName, NameLength3, ColumnName, NameLength4, false);
}
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
     SQLSMALLINT NameLength6)
{
    return NeoForeignKeys(StatementHandle, PKCatalogName, NameLength1, PKSchemaName, NameLength2,
                 PKTableName, NameLength3, FKCatalogName, NameLength4, FKSchemaName, NameLength5, FKTableName, NameLength6, false);
}
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
     SQLSMALLINT NameLength3)
{
    return NeoTablePrivileges(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3, false);
}
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
				SQLSMALLINT *TextLength)
{
	return NeoGetDiagRec(HandleType, Handle, RecNumber, (SQLCHAR*)Sqlstate, NativeError, (SQLCHAR*)MessageText, BufferLength, TextLength,true);
}
}


extern "C" 
{
SQLRETURN SQL_API SQLGetDiagFieldW(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber, 
				SQLSMALLINT DiagIdentifier,
				SQLPOINTER DiagInfo, 
				SQLSMALLINT BufferLength,
				SQLSMALLINT *StringLength)
{
	return NeoGetDiagField(HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength,true);
}
}


extern "C" 
{
SQLRETURN SQL_API SQLEndTran(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT CompletionType)
{
	return NeoEndTran(HandleType, Handle, CompletionType);
}
}


extern "C" 
{
SQLRETURN SQL_API SQLConnectW(SQLHDBC ConnectionHandle,
           SQLWCHAR *ServerName, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *UserName, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *Authentication, 
		   SQLSMALLINT NameLength3)
{
	return NeoConnect(ConnectionHandle, (SQLCHAR*)ServerName, NameLength1, (SQLCHAR*)UserName, NameLength2, (SQLCHAR*)Authentication, NameLength3,true);
}
}


extern "C"
{
SQLRETURN SQL_API SQLDisconnect(SQLHDBC ConnectionHandle)
{
	return NeoDisconnect(ConnectionHandle);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLSetConnectAttrW(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	return NeoSetConnectAttr(ConnectionHandle, Attribute, Value, StringLength,true);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLGetConnectAttrW(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength)
{
	return NeoGetConnectAttr(ConnectionHandle, Attribute, Value, BufferLength, StringLength,true);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLSetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	return NeoSetEnvAttr(EnvironmentHandle, Attribute, Value, StringLength);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLGetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength)
{
	return NeoGetEnvAttr(EnvironmentHandle, Attribute, Value, BufferLength, StringLength);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLSetStmtAttrW(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	return NeoSetStmtAttr(StatementHandle, Attribute, Value, StringLength,true);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLGetStmtAttrW(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength)
{
	return NeoGetStmtAttr(StatementHandle, Attribute, Value, BufferLength, StringLength,true);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLGetInfoW(SQLHDBC ConnectionHandle,
           SQLUSMALLINT InfoType, 
		   SQLPOINTER InfoValuePtr,
           SQLSMALLINT BufferLength,
		   SQLSMALLINT *StringLengthPtr)
{
	return NeoGetInfo(ConnectionHandle, InfoType, InfoValuePtr, BufferLength, StringLengthPtr,true);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLSetDescFieldW(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength)
{
	return NeoSetDescField(DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength,true);
}
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
           SQLLEN *IndicatorPtr)
{
	return NeoSetDescRec(DescriptorHandle, RecNumber, Type, SubType, Length, Precision, Scale, Data, StringLengthPtr, IndicatorPtr);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLGetDescFieldW(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength,
           SQLINTEGER *StringLengthPtr)
{
	return NeoGetDescField(DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength, StringLengthPtr,true);
}
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
		   SQLSMALLINT *NullablePtr)
{
	return NeoGetDescRec(DescriptorHandle, RecNumber, (SQLCHAR*)Name, BufferLength, StringLengthPtr, TypePtr, SubTypePtr, LengthPtr, 
		PrecisionPtr, ScalePtr, NullablePtr,true);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLBindCol(SQLHSTMT StatementHandle, 
		   SQLUSMALLINT ColumnNumber, 
		   SQLSMALLINT TargetType, 
		   SQLPOINTER TargetValue, 
		   SQLLEN BufferLength, 
	   	   SQLLEN *StrLen_or_IndPtr)
{
	return NeoBindCol(StatementHandle, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_IndPtr);
}
}


extern "C"
{
SQLRETURN SQL_API SQLBindParameter(SQLHSTMT StatementHandle,
			SQLUSMALLINT ParameterNumber, 
			SQLSMALLINT InputOutputType,
			SQLSMALLINT ValueType,
			SQLSMALLINT ParameterType, 
			SQLULEN	ColumnSize,
			SQLSMALLINT DecimalDigits,
			SQLPOINTER  ParameterValuePtr,
			SQLLEN	BufferLength,
			SQLLEN *StrLen_or_IndPtr)		   
{
	return NeoBindParameter(StatementHandle, ParameterNumber, InputOutputType, ValueType, ParameterType, ColumnSize, DecimalDigits,
			ParameterValuePtr, BufferLength, StrLen_or_IndPtr);		   
}
}


extern "C"
{
SQLRETURN SQL_API SQLBrowseConnectW(
    SQLHDBC            ConnectionHandle,
    SQLWCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLWCHAR 		  *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT       *StringLength2Ptr)
{
	return NeoBrowseConnect( ConnectionHandle, (SQLCHAR*)InConnectionString, StringLength1, (SQLCHAR*)OutConnectionString, BufferLength, StringLength2Ptr,true);
}
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
    SQLUSMALLINT       DriverCompletion)
{
	return NeoDriverConnect(ConnectionHandle, WindowHandle, (SQLCHAR*)InConnectionString, StringLength1, 
		(SQLCHAR*)OutConnectionString, BufferLength, StringLength2Ptr, DriverCompletion,true);
}
}


extern "C"
{
SQLRETURN SQL_API SQLPrepareW(SQLHSTMT StatementHandle,
           SQLWCHAR *StatementText,
		   SQLINTEGER TextLength)
{
	return NeoPrepare(StatementHandle, (SQLCHAR*)StatementText, TextLength,true);
}
}


extern "C"
{
SQLRETURN SQL_API SQLExecDirectW(SQLHSTMT StatementHandle,
           SQLWCHAR *StatementText,
		   SQLINTEGER TextLength)
{
	return NeoExecDirect(StatementHandle, (SQLCHAR*)StatementText, TextLength,true);
}
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
		   SQLSMALLINT *NullablePtr)
{
	return NeoDescribeCol(StatementHandle, ColumnNumber, (SQLCHAR*)ColumnName, BufferLength, NameLengthPtr, DataTypePtr, 
		ColumnSizePtr, DecimalDigitsPtr, NullablePtr,true);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLNumResultCols(SQLHSTMT StatementHandle,
           SQLSMALLINT *ColumnCountPtr)
{
	return NeoNumResultCols(StatementHandle, ColumnCountPtr);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLNumParams(SQLHSTMT StatementHandle,
           SQLSMALLINT *ParameterCountPtr)
{
	return NeoNumParams(StatementHandle, ParameterCountPtr);
}
}


extern "C"
{
SQLRETURN SQL_API SQLDescribeParam(
	SQLHSTMT           StatementHandle,
	SQLUSMALLINT       ParameterNumber,
    SQLSMALLINT 	  *DataTypePtr,
    SQLULEN      	  *ParameterSizePtr,
    SQLSMALLINT 	  *DecimalDigitsPtr,
    SQLSMALLINT 	  *NullablePtr)
{
	return NeoDescribeParam( StatementHandle, ParameterNumber, DataTypePtr, ParameterSizePtr, DecimalDigitsPtr, NullablePtr);
}
}


extern "C"
{
SQLRETURN SQL_API SQLFreeStmt(SQLHSTMT StatementHandle,
        SQLUSMALLINT Option)
{
	return NeoFreeStmt(StatementHandle, Option);
}
}


extern "C"
{
SQLRETURN SQL_API SQLCloseCursor(SQLHSTMT StatementHandle)
{
	return NeoCloseCursor(StatementHandle);
}
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
		   SQLSMALLINT NameLength4)
{
	return NeoTables(StatementHandle, (SQLCHAR*)CatalogName, NameLength1, (SQLCHAR*)SchemaName, NameLength2, (SQLCHAR*)TableName, NameLength3, (SQLCHAR*)TableType, NameLength4,true);
}
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
		   SQLSMALLINT NameLength4)
{
	return NeoColumns(StatementHandle, (SQLCHAR*)CatalogName, NameLength1, (SQLCHAR*)SchemaName, NameLength2, (SQLCHAR*)TableName, NameLength3, (SQLCHAR*)ColumnName, NameLength4,true);
}
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
		   SQLUSMALLINT Nullable)
{
	return NeoSpecialColumns(StatementHandle, IdentifierType, (SQLCHAR*)CatalogName, NameLength1, (SQLCHAR*)SchemaName, NameLength2, (SQLCHAR*)TableName, 
		   NameLength3, Scope, Nullable,true);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLGetTypeInfoW(SQLHSTMT StatementHandle,
           SQLSMALLINT DataType)
{
	return NeoGetTypeInfo(StatementHandle, DataType,true);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLPrimaryKeysW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableName, 
		   SQLSMALLINT NameLength3)
{
	return NeoPrimaryKeys(StatementHandle, (SQLCHAR*)CatalogName, NameLength1, (SQLCHAR*)SchemaName, NameLength2, (SQLCHAR*)TableName, NameLength3,true);
}
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
		   SQLUSMALLINT Reserved)
{
	return NeoStatistics(StatementHandle, (SQLCHAR*)CatalogName, NameLength1, (SQLCHAR*)SchemaName, NameLength2, (SQLCHAR*)TableName, NameLength3,
		   Unique, Reserved,true);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLGetCursorNameW(SQLHSTMT StatementHandle,
           SQLWCHAR *CursorName, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLengthPtr)
{
	return NeoGetCursorName(StatementHandle, (SQLCHAR*)CursorName, BufferLength, NameLengthPtr,true);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLSetCursorNameW(SQLHSTMT StatementHandle,
           SQLWCHAR *CursorName, 
		   SQLSMALLINT NameLength)
{
	return NeoSetCursorName(StatementHandle, (SQLCHAR*)CursorName, NameLength,true);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLRowCount(SQLHSTMT StatementHandle, 
	   SQLLEN	*RowCountPtr)
{
	return NeoRowCount(StatementHandle, RowCountPtr);
}
}


extern "C"
{
SQLRETURN SQL_API SQLMoreResults(
    SQLHSTMT           StatementHandle)
{
	return NeoMoreResults(StatementHandle);
}
}


extern "C"
{
SQLRETURN SQL_API SQLNativeSqlW(
    SQLHDBC            ConnectionHandle,
    SQLWCHAR 		  *InStatementText,
    SQLINTEGER         TextLength1,
    SQLWCHAR 		  *OutStatementText,
    SQLINTEGER         BufferLength,
    SQLINTEGER 		  *TextLength2Ptr)
{
	return NeoNativeSql(ConnectionHandle, (SQLCHAR*)InStatementText, TextLength1, (SQLCHAR*)OutStatementText, BufferLength, TextLength2Ptr,true);
}
}


extern "C"
{
SQLRETURN SQL_API SQLCancel(SQLHSTMT StatementHandle)
{
	return NeoCancel(StatementHandle);
}
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
		   SQLLEN *NumericAttributePtr)
{
	return NeoColAttribute(StatementHandle, ColumnNumber, FieldIdentifier, CharacterAttributePtr, 
		   BufferLength, StringLengthPtr, NumericAttributePtr,true);
}
#else
SQLRETURN SQL_API SQLColAttributeW(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttributePtr, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLengthPtr, 
		   SQLPOINTER NumericAttributePtr)
{
	return NeoColAttribute(StatementHandle, ColumnNumber, FieldIdentifier, CharacterAttributePtr, 
		   BufferLength, StringLengthPtr, NumericAttributePtr,true);
}
#endif
}


extern "C"
{
SQLRETURN SQL_API SQLExecute(SQLHSTMT StatementHandle)
{
	return NeoExecute(StatementHandle);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLParamData(SQLHSTMT StatementHandle,
           SQLPOINTER *ValuePtrPtr)
{
	return NeoParamData(StatementHandle, ValuePtrPtr);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLPutData(SQLHSTMT StatementHandle,
           SQLPOINTER DataPtr, 
		   SQLLEN StrLen_or_Ind)
{
	return NeoPutData(StatementHandle, DataPtr, StrLen_or_Ind);
}
}


extern "C"
{
SQLRETURN SQL_API SQLFetch(SQLHSTMT StatementHandle)
{
return 	NeoFetch(StatementHandle);
}
}


extern "C"
{
SQLRETURN SQL_API SQLGetData(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLSMALLINT  TargetType,
           SQLPOINTER   TargetValuePtr, 
		   SQLLEN       BufferLength,
           SQLLEN      *StrLen_or_IndPtr)
{
return NeoGetData(StatementHandle, ColumnNumber, TargetType, TargetValuePtr, BufferLength, StrLen_or_IndPtr);
}
}


extern "C"
{
SQLRETURN SQL_API SQLSetPos(
    SQLHSTMT        StatementHandle,
    SQLSETPOSIROW	RowNumber,
    SQLUSMALLINT    Operation,
    SQLUSMALLINT    LockType)
{
 return NeoSetPos(StatementHandle, RowNumber, Operation, LockType);
}
}


extern "C"
{
SQLRETURN SQL_API SQLCopyDesc(SQLHDESC SourceDescHandle,
		SQLHDESC TargetDescHandle)
{
	return NeoCopyDesc(SourceDescHandle, TargetDescHandle);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLProceduresW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *ProcName, 
		   SQLSMALLINT NameLength3)
{
	return NeoProcedures(StatementHandle, (SQLCHAR*)CatalogName, NameLength1, (SQLCHAR*)SchemaName, NameLength2, (SQLCHAR*)ProcName, NameLength3,true);
}
}


extern "C"
{
SQLRETURN  SQL_API SQLProcedureColumnsW(SQLHSTMT StatementHandle,
           SQLWCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *ProcName, 
		   SQLSMALLINT NameLength3,
           SQLWCHAR *ColumnName, 
		   SQLSMALLINT NameLength4)
{
	return NeoProcedureColumns(StatementHandle, (SQLCHAR*)CatalogName, NameLength1, (SQLCHAR*)SchemaName, NameLength2,
           	(SQLCHAR*)ProcName, NameLength3, (SQLCHAR*)ColumnName, NameLength4,true);
}
}


extern "C"
{
SQLRETURN SQL_API SQLExtendedFetch(SQLHSTMT StatementHandle,
			SQLUSMALLINT FetchOrientation,
			SQLLEN FetchOffset,
			SQLULEN* RowCountPtr,
			SQLUSMALLINT* RowStatusArray)
{
	return NeoExtendedFetch(StatementHandle, FetchOrientation, FetchOffset, RowCountPtr, RowStatusArray);
}
}


extern "C"
{
SQLRETURN SQL_API SQLFetchScroll(
	SQLHSTMT StatementHandle,
    SQLSMALLINT FetchOrientation,
    SQLLEN FetchOffset)
{
	return NeoFetchScroll(StatementHandle, FetchOrientation, FetchOffset);
}
}


extern "C"
{
SQLRETURN SQL_API SQLColumnPrivilegesW(SQLHSTMT StatementHandle,
     SQLWCHAR* CatalogName,
     SQLSMALLINT NameLength1,
     SQLWCHAR* SchemaName,
     SQLSMALLINT NameLength2,
     SQLWCHAR* TableName,
     SQLSMALLINT NameLength3,
     SQLWCHAR* ColumnName,
     SQLSMALLINT NameLength4)
{
	return NeoColumnPrivileges(StatementHandle, (SQLCHAR*)CatalogName, NameLength1, (SQLCHAR*)SchemaName, NameLength2, 
			(SQLCHAR*)TableName, NameLength3, (SQLCHAR*)ColumnName, NameLength4,true);
}
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
     SQLSMALLINT NameLength6)
{
	return NeoForeignKeys(StatementHandle, (SQLCHAR*)PKCatalogName, NameLength1, (SQLCHAR*)PKSchemaName, NameLength2,
     			(SQLCHAR*)PKTableName, NameLength3, (SQLCHAR*)FKCatalogName, NameLength4, (SQLCHAR*)FKSchemaName, NameLength5, (SQLCHAR*)FKTableName, NameLength6,true);
}
}


extern "C"
{
SQLRETURN SQL_API SQLTablePrivilegesW(
	 SQLHSTMT StatementHandle,
     SQLWCHAR *CatalogName,
     SQLSMALLINT NameLength1,
     SQLWCHAR *SchemaName,
     SQLSMALLINT NameLength2,
     SQLWCHAR *TableName,
     SQLSMALLINT NameLength3)
{
	return NeoTablePrivileges(StatementHandle, (SQLCHAR*)CatalogName, NameLength1, (SQLCHAR*)SchemaName, NameLength2, (SQLCHAR*)TableName, NameLength3,true);
}
}


//**************************************************************************
//================== APIs not implemented ==================================
//**************************************************************************

extern "C"
{
SQLRETURN SQL_API SQLBulkOperations(
	SQLHSTMT StatementHandle,
    SQLSMALLINT Operation)
{
	return NeoBulkOperations(StatementHandle, Operation);
}
}

