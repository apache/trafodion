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
#include "sqlstmt.h"
#include "cstmt.h"
#include "mxomsg.h"

using namespace ODBC;

SQLRETURN ODBC::SetStmtAttr(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->SetStmtAttr(Attribute, Value, StringLength);
	return rc;
}

SQLRETURN ODBC::GetStmtAttr(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER ValuePtr,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLengthPtr)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->GetStmtAttr(Attribute, ValuePtr, BufferLength, StringLengthPtr);
	return rc;

}

SQLRETURN ODBC::BindCol(SQLHSTMT StatementHandle, 
		   SQLUSMALLINT ColumnNumber, 
		   SQLSMALLINT TargetType, 
		   SQLPOINTER TargetValuePtr, 
		   SQLLEN BufferLength, 
	   	   SQLLEN *StrLen_or_IndPtr)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->BindCol(ColumnNumber, TargetType, TargetValuePtr, BufferLength, StrLen_or_IndPtr);
	return rc;
}

SQLRETURN ODBC::BindParameter(SQLHSTMT StatementHandle,
			SQLUSMALLINT ParameterNumber, 
			SQLSMALLINT InputOutputType,
			SQLSMALLINT ValueType,
			SQLSMALLINT ParameterType, 
			SQLULEN 	ColumnSize,
			SQLSMALLINT DecimalDigits,
			SQLPOINTER  ParameterValuePtr,
			SQLLEN  	BufferLength,
			SQLLEN     *StrLen_or_IndPtr)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->BindParameter(ParameterNumber, InputOutputType, ValueType, ParameterType, ColumnSize, 
			DecimalDigits, ParameterValuePtr, BufferLength, StrLen_or_IndPtr);
	return rc;
}

SQLRETURN ODBC::WMPrepare(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->Prepare(StatementText, TextLength);
	if (pStmt->m_BT) pStmt->m_WMStmtPrepared = true;
	return rc;
}


SQLRETURN ODBC::Prepare(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength)
{
	SQLRETURN	rc = SQL_SUCCESS;
	SQLRETURN	WMrc = SQL_SUCCESS;
	CStmt		*pStmt;
	CConnect	*pConnect;
	unsigned char* sql = StatementText;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;

  if(gDrvrGlobal.gSpecial_1)
  { 
	pConnect = pStmt->getConnectHandle();
	switch(ParseStmt(pStmt, StatementText))
	{
	case WM_BT:
		FreeStmt(StatementHandle, SQL_API_SQLCLOSECURSOR, SQL_CLOSE);
		ResetRowsetValues(pStmt);
		break;
	case WM_ET:
		if (pStmt->m_valcnt)
			WMrc = ProcessRowset(pStmt);
		break;
	case WM_INBT:
		pStmt->m_WMStmtPrepared = false;
		if (pStmt->m_valcnt)
			WMrc = ProcessRowset(pStmt);
		break;
	case WM_RETURN_SUCCESS:
		pStmt->m_WMStmtPrepared = false;
		return SQL_SUCCESS;
		break;
	case WM_OTHER:
	default:
		break;
	}
		
	if (WMrc == SQL_ERROR)
		return WMrc;
  }// if gDrvrGlobal.gSpecial_1 

	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->Prepare(StatementText, TextLength);

	if (gDrvrGlobal.gSpecial_1 && pStmt->m_BT) pStmt->m_WMStmtPrepared = true;
	return rc;
}

SQLRETURN ODBC::ExecDirect(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength)
{
	SQLRETURN	rc = SQL_SUCCESS;
	SQLRETURN	WMrc = SQL_SUCCESS;
	
	CStmt		*pStmt;

	CConnect	*pConnect;
	unsigned char* sql = StatementText;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	
   if(gDrvrGlobal.gSpecial_1)
   {
	  switch(ParseStmt(pStmt, StatementText))
	  {
	  case WM_BT:
		FreeStmt(StatementHandle, SQL_API_SQLCLOSECURSOR, SQL_CLOSE);
		ResetRowsetValues(pStmt);
		break;
	  case WM_ET:
		if (pStmt->m_valcnt)
			WMrc = ProcessRowset(pStmt);
		break;
	  case WM_INBT:
		if (pStmt->m_valcnt)
			WMrc = ProcessRowset(pStmt);
		break;
	  case WM_RETURN_SUCCESS:
		return SQL_SUCCESS;
		break;
	  case WM_OTHER:
	  default:
		break;
	  }
	  if (WMrc == SQL_ERROR)
		return WMrc;
   } // if gDrvrGlobal.gSpecial_1

	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->ExecDirect(StatementText, TextLength);

    if(gDrvrGlobal.gSpecial_1)
	{
	   if (pStmt->m_token == WM_BT)
	   {
	      if (rc == SQL_SUCCESS)
			pStmt->m_BT = true;
	   }
	   else if (pStmt->m_token == WM_BT)
	   {
	      if (rc == SQL_SUCCESS)
			pStmt->m_BT = false;
	   }
	} // if gDrvrGlobal.gSpecial_1

	return rc;
}
SQLRETURN ODBC::getDescRec(SQLHSTMT StatementHandle,
		short odbcAPI,
		SQLUSMALLINT ColumnNumber, 
		SQLCHAR *ColumnName,
        SQLSMALLINT BufferLength, 
		SQLSMALLINT *NameLengthPtr,
        SQLSMALLINT *DataTypePtr, 
		SQLULEN *ColumnSizePtr,
        SQLSMALLINT *DecimalDigitsPtr,
		SQLSMALLINT *NullablePtr)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->getDescRec(odbcAPI, ColumnNumber, ColumnName, BufferLength, NameLengthPtr,
			DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, NullablePtr);
	return rc;

}

SQLRETURN ODBC::getDescSize(SQLHSTMT StatementHandle,
			short odbcAPI,
			SQLSMALLINT *ColumnCountPtr)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	if(pStmt->getClearDiagRec())
		pStmt->setDiagRowCount(0, -1);
	rc = pStmt->getDescSize(odbcAPI, ColumnCountPtr);
	return rc;
}

SQLRETURN ODBC::FreeStmt(SQLHSTMT StatementHandle,
		short odbcAPI,
        SQLUSMALLINT Option)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->FreeStmt(odbcAPI, Option);
	return rc;
}

SQLRETURN ODBC::GetSQLCatalogs(SQLHSTMT StatementHandle,
		   short	odbcAPI,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   SQLCHAR *ColumnName, 
		   SQLSMALLINT NameLength4,
           SQLCHAR *TableType, 
		   SQLSMALLINT NameLength5,
		   SQLUSMALLINT IdentifierType,
		   SQLUSMALLINT Scope,
		   SQLUSMALLINT Nullable,
		   SQLSMALLINT SqlType,
		   SQLUSMALLINT Unique,
		   SQLUSMALLINT Reserved,
		   SQLCHAR *FKCatalogName, 
		   SQLSMALLINT NameLength6,
           SQLCHAR *FKSchemaName, 
		   SQLSMALLINT NameLength7,
           SQLCHAR *FKTableName, 
		   SQLSMALLINT NameLength8)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	
	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->GetSQLCatalogs(odbcAPI, CatalogName, NameLength1, SchemaName, 
						NameLength2, TableName, NameLength3, ColumnName, NameLength4,
						TableType, NameLength5, IdentifierType, Scope, Nullable, SqlType, Unique, Reserved,
						FKCatalogName, NameLength6, FKSchemaName, NameLength7, FKTableName, NameLength8);
	return rc;
}

SQLRETURN ODBC::GetCursorName(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLengthPtr)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	
	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->GetCursorName(CursorName, BufferLength, NameLengthPtr);
	return rc;
}

SQLRETURN ODBC::SetCursorName(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, 
		   SQLSMALLINT NameLength)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->SetCursorName(CursorName, NameLength);
	return rc;
}

SQLRETURN ODBC::RowCount(SQLHSTMT StatementHandle, 
	   SQLLEN	*RowCountPtr)
{
	CStmt	*pStmt;
	INT64 tempRowCnt64;
	
	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->clearError();
	pStmt->setDiagRowCount(0, -1);
	if (RowCountPtr != NULL)
	{
		#ifdef __LP64__
		*RowCountPtr = pStmt->getRowCount();
		#else
			tempRowCnt64 = pStmt->getRowCount();
			if(tempRowCnt64 > LONG_MAX)
			{
				char ErrorBuffer[256];
				#if defined (MXLINUX) || defined(MXIA64LINUX)
				sprintf(ErrorBuffer,"Numeric Overflow: the row count %Ld exceeds the maximum value that can be returned by SQLRowCount",tempRowCnt64);
				#else
				sprintf(ErrorBuffer,"Numeric Overflow: the row count %lld exceeds the maximum value that can be returned by SQLRowCount",tempRowCnt64);
				#endif
				pStmt->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, ErrorBuffer);
				return SQL_ERROR;
			}
			else
				*RowCountPtr = tempRowCnt64;
		#endif /* __LP64__ */
	} // RowCountPtr != NULL

	return SQL_SUCCESS;
}

SQLRETURN ODBC::Cancel(SQLHSTMT StatementHandle)
{

	SQLRETURN	rc;
	CStmt		*pStmt;

	// We can sometimes have race conditions attempting to cancel a query right after its been submitted
	// for execution (timing issue with the query registering with the cancel broker etc).
	// to prevent that we introduce a 10 second delay when cancel is called.
	sleep(10); 

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->Cancel();
	return rc;
}


SQLRETURN ODBC::ColAttribute(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttributePtr, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLengthPtr, 
		   SQLPOINTER NumericAttributePtr)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->ColAttribute(ColumnNumber, FieldIdentifier, CharacterAttributePtr, BufferLength,
					StringLengthPtr, NumericAttributePtr);
	return rc;
}

SQLRETURN ODBC::WMExecute(SQLHSTMT StatementHandle)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->Execute();
	return rc;
}

SQLRETURN ODBC::Execute(SQLHSTMT StatementHandle)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;

	if (gDrvrGlobal.gSpecial_1 && pStmt->m_BT && pStmt->m_WMStmtPrepared == false)
		return SQL_SUCCESS;

	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->Execute();
	
   if(gDrvrGlobal.gSpecial_1)
   {
	  if (pStmt->m_token == WM_BT)
	  {
		if (rc == SQL_SUCCESS)
			pStmt->m_BT = true;
	  }
	  else if (pStmt->m_token == WM_BT)
	  {
		if (rc == SQL_SUCCESS)
			pStmt->m_BT = false;
	  }
   } // if gDrvrGlobal.gSpecial_1
	
	return rc;
}

SQLRETURN ODBC::ParamData(SQLHSTMT StatementHandle,
		SQLPOINTER *ValuePtrPtr)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->ParamData(ValuePtrPtr);
	return rc;

}

SQLRETURN ODBC::PutData(SQLHSTMT StatementHandle,
           SQLPOINTER DataPtr, 
		   SQLINTEGER StrLen_or_Ind)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->PutData(DataPtr, StrLen_or_Ind);
	return rc;
}

SQLRETURN ODBC::Fetch(SQLHSTMT StatementHandle)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->Fetch();
	return rc;
}

SQLRETURN ODBC::FetchScroll(SQLHSTMT StatementHandle,
			SQLUSMALLINT FetchOrientation,
			SQLINTEGER FetchOffset)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->FetchScroll(FetchOrientation, FetchOffset);
	return rc;
}

SQLRETURN ODBC::ExtendedFetch(SQLHSTMT StatementHandle,
			SQLUSMALLINT FetchOrientation,
			SQLLEN FetchOffset,
			SQLULEN* RowCountPtr,
			SQLUSMALLINT* RowStatusArray)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->ExtendedFetch(FetchOrientation,FetchOffset,RowCountPtr,RowStatusArray);
	return rc;
}

SQLRETURN ODBC::GetData(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLSMALLINT TargetType,
           SQLPOINTER TargetValuePtr, 
		   SQLLEN BufferLength,
           SQLLEN *StrLen_or_IndPtr)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->GetData(ColumnNumber, TargetType, TargetValuePtr, BufferLength, StrLen_or_IndPtr);
	return rc;
}

SQLRETURN ODBC::SetPos(SQLHSTMT StatementHandle,
    SQLUSMALLINT	RowNumber,
    SQLUSMALLINT    Operation,
    SQLUSMALLINT    LockType)
{
	SQLRETURN	rc;
	CStmt		*pStmt;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_STMT, StatementHandle))
		return SQL_INVALID_HANDLE;
	pStmt = (CStmt *)StatementHandle;
	pStmt->setDiagRowCount(0, -1);
	rc = pStmt->SetPos(RowNumber, Operation, LockType);
	return rc;
}

void ResetRowsetValues(CStmt *pStmt)
{
	VALUES::iterator pv;

	for(pv = pStmt->m_values.begin(); pv != pStmt->m_values.end(); pv++)
	{
		delete ((char*)(*pv));
	}
	pStmt->m_vlen.clear();
	pStmt->m_values.clear();
	pStmt->m_valcnt = 0;
}

void GetRowsetValues(CStmt *pStmt, unsigned char* sql_i, unsigned char* sql_ie)
{
	bool quote = false;
	int len;
	int valcnt = 0;
	char* value;
	char* pvals=NULL;
	char* pvale=NULL;
	char* sql = (char*)sql_ie;

	while (*sql != '(' ) sql++;
	sql++;
	while (*sql == ' ' || *sql == '\t') sql++;
	while (*sql != ')')
	{
		if (*sql == '\'')
		{
			quote = true;
			pvals = pvale = sql + 1;
			while(*pvale != '\'') pvale++;
		}
		else
		{
			quote = false;
			pvals = pvale = sql;
			while (*pvale != ',' && *pvale != ' ' && *pvale != '\t' && *pvale != ')') pvale++;
		}
		len = pvale - pvals;
		value = new char[ len + 1];
		memset(value,0,len+1);
		strncpy(value, pvals, len);
		pStmt->m_values.push_back(value);

		if (pStmt->m_valcnt == 0)
			pStmt->m_vlen.push_back(len);
		else if (pStmt->m_vlen[valcnt] < len)
			pStmt->m_vlen[valcnt] = len;
		
		valcnt++;
		sql = pvale;
		if (quote) sql++;
		while (*sql == ',' || *sql == ' ' || *sql == '\t') sql++;
	}
	if (pStmt->m_valcnt == 0)
	{
		char sqlStmt[100] = {0};
		pStmt->m_valcnt = valcnt;
		strncpy(sqlStmt,(const char*)sql_i, sql_ie - sql_i);
		pStmt->m_sqlTable = sqlStmt;

		strcpy((char*)sqlStmt,"INSERT INTO ");
		strncat((char*)sqlStmt, (const char*)sql_i, sql_ie - sql_i);
		strcat((char*)sqlStmt," VALUES (?");
		for (int i = 1; i < valcnt; i++)
			strcat((char*)sqlStmt,",?");
		strcat((char*)sqlStmt,")");
		pStmt->m_sqlStmt = sqlStmt;
	}

}

SQLRETURN ProcessRowset(CStmt *pStmt)
{
	SQLHSTMT SH = (SQLHSTMT)pStmt;
	SQLRETURN rc = SQL_SUCCESS;

	VALUES::iterator pv;
	char* val;
	int index = 0;
	int row = 0;
	int slen;
	int i;

	SQLLEN** LenOrIndArray = NULL;
	char** ParArray = NULL;

	int valcnt = pStmt->m_valcnt;
	int rowset = pStmt->m_values.size() / valcnt;

	rc = FreeStmt(SH, SQL_API_SQLCLOSECURSOR, SQL_CLOSE);
	if ((rc = SetStmtAttr(SH, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0)) == SQL_ERROR)
	{
		ResetRowsetValues(pStmt);
		goto bailout;
	}
	if ((rc = SetStmtAttr(SH, SQL_ATTR_PARAMSET_SIZE, (void *)rowset, 0)) == SQL_ERROR)
	{
		ResetRowsetValues(pStmt);
		goto bailout;
	}
	if ((rc = SetStmtAttr(SH, SQL_ATTR_PARAM_STATUS_PTR, NULL, 0)) == SQL_ERROR)
	{
		ResetRowsetValues(pStmt);
		goto bailout;
	}
	if ((rc = SetStmtAttr(SH, SQL_ATTR_PARAMS_PROCESSED_PTR, NULL, 0)) == SQL_ERROR)
	{
		ResetRowsetValues(pStmt);
		goto bailout;
	}
    if ((rc = WMPrepare (SH, (SQLCHAR*) pStmt->m_sqlStmt.c_str(), SQL_NTS)) == SQL_ERROR)
	{
		ResetRowsetValues(pStmt);
		goto bailout;
	}

	LenOrIndArray = (SQLLEN**)new char[valcnt * sizeof(long)];
	ParArray = (char**)new char[valcnt * sizeof(long)];

	for (i=0; i< valcnt; i++)
	{
		slen = pStmt->m_vlen[i] + 1;
		LenOrIndArray[i] = (SQLLEN*)new char[rowset * sizeof(long)];
		ParArray[i] = new char[rowset * slen];
		rc = ODBC::BindParameter(SH, i+1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,slen,0, ParArray[i], slen, LenOrIndArray[i]);
	}

	for(pv = pStmt->m_values.begin(); pv != pStmt->m_values.end(); pv++)
	{
		for (index=0; index< valcnt; index++)
		{
			slen = pStmt->m_vlen[index] + 1;
			val = ((char*)(*pv));
			LenOrIndArray[index][row] = SQL_NTS;
			strcpy(&ParArray[index][slen * row],val);
			delete val;
			if (index != valcnt -1)
				pv++;
		}
		row++;
	}
	pStmt->m_vlen.clear();
	pStmt->m_values.clear();
	pStmt->m_valcnt = 0;

	rc = WMExecute(SH);

bailout:
	if (LenOrIndArray) 
	{
		for (i=0; i< valcnt; i++)
			if (LenOrIndArray[i])
				delete LenOrIndArray[i];
		delete LenOrIndArray;
	}
	if (ParArray != NULL)
	{
		for (i=0; i< valcnt; i++)
			if (ParArray[i])
				delete ParArray[i];
		delete ParArray;
	}

	return rc;
}

WMTOKEN ParseStmt(CStmt* pStmt, SQLCHAR *StatementText) 
{
	unsigned char* sql = StatementText;

	pStmt->m_token = WM_INIT;

	while (*sql == 0x0A || *sql == 0x0D || *sql == ' ' || *sql == '\t') sql++;
	if (strnicmp((const char*)sql,"BT;",3) == 0)
	{
		pStmt->m_token = WM_BT;
		return WM_BT;
	}
	else if (strnicmp((const char*)sql,"ET;",3) == 0)
	{
		pStmt->m_token = WM_ET;
		return WM_ET;
	}
	else if (pStmt->m_BT)
	{
		if (strnicmp((const char*)sql,"INSERT",6) == 0)
		{
			sql += 6;
			while (*sql == '\n' || *sql == ' ' || *sql == '\t') sql++;
			if (strnicmp((const char*)sql,"INTO",4) == 0)
			{
				unsigned char* sql_e;
				unsigned char* sql_ee;
				unsigned char* sql_tmp;
				sql += 4;
				while (*sql == ' ' || *sql == '\t') sql++;
				sql_e = sql;
				while (*sql_e != ' ' && *sql_e != '\t') sql_e++;
				sql_tmp = sql_e;
				while (*sql_tmp == ' ' || *sql_tmp == '\t') sql_tmp++;
				if (*sql_tmp == '(')
				{
					while (*sql_tmp != ')') sql_tmp++;
					sql_tmp++;
					sql_ee = sql_tmp;
					sql_e = sql_tmp;
				}
				else
					sql_ee = sql_e;
				while (*sql_ee == ' ' || *sql_ee == '\t') sql_ee++;
				if (strnicmp((const char*)sql_ee,"VALUES",6) == 0)
				{
					if (pStmt->m_valcnt == 0 || strnicmp((const char*)sql,pStmt->m_sqlTable.c_str(),sql_e - sql) == 0)
					{
						GetRowsetValues(pStmt, sql, sql_e);
						return WM_RETURN_SUCCESS;
					}
				}
			}
		}
		return WM_INBT;
	}
	return WM_OTHER;
}

