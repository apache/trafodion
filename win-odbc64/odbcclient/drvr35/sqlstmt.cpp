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
#include "DrvrGlobal.h"
#include "SQLStmt.h"
#include "CStmt.h"
#include "tdm_odbcDrvMsg.h"

SQLRETURN ODBC::SetStmtAttr(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->SetStmtAttr(Attribute, Value, StringLength);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
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
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->GetStmtAttr(Attribute, ValuePtr, BufferLength, StringLengthPtr);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
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
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->BindCol(ColumnNumber, TargetType, TargetValuePtr, BufferLength, StrLen_or_IndPtr);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN ODBC::BindParameter(
			SQLHSTMT StatementHandle,
			SQLUSMALLINT ParameterNumber, 
			SQLSMALLINT InputOutputType,
			SQLSMALLINT ValueType,
			SQLSMALLINT ParameterType, 
			SQLULEN		ColumnSize,
			SQLSMALLINT DecimalDigits,
			SQLPOINTER  ParameterValuePtr,
			SQLLEN		BufferLength,
			SQLLEN		*StrLen_or_IndPtr)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->BindParameter(ParameterNumber, InputOutputType, ValueType, ParameterType, ColumnSize, 
				DecimalDigits, ParameterValuePtr, BufferLength, StrLen_or_IndPtr);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN ODBC::Prepare(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->Prepare(StatementText, TextLength);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN ODBC::ExecDirect(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->ExecDirect(StatementText, TextLength);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN ODBC::getDescRec(SQLHSTMT StatementHandle,
		short odbcAPI,
		SQLUSMALLINT ColumnNumber, 
		SQLWCHAR *ColumnName,
        SQLSMALLINT BufferLength, 
		SQLSMALLINT *NameLengthPtr,
        SQLSMALLINT *DataTypePtr, 
		SQLULEN *ColumnSizePtr,
        SQLSMALLINT *DecimalDigitsPtr,
		SQLSMALLINT *NullablePtr)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->getDescRec(odbcAPI, ColumnNumber, ColumnName, BufferLength, NameLengthPtr,
				DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, NullablePtr);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;

}

SQLRETURN ODBC::getDescSize(SQLHSTMT StatementHandle,
			short odbcAPI,
			SQLSMALLINT *ColumnCountPtr)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->getDescSize(odbcAPI, ColumnCountPtr);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN ODBC::FreeStmt(SQLHSTMT StatementHandle,
		short odbcAPI,
        SQLUSMALLINT Option)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->FreeStmt(odbcAPI, Option);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN ODBC::GetSQLCatalogs(SQLHSTMT StatementHandle,
		   short	odbcAPI,
           SQLWCHAR *CatalogNameW, 
		   SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaNameW, 
		   SQLSMALLINT NameLength2,
           SQLWCHAR *TableNameW, 
		   SQLSMALLINT NameLength3,
		   SQLWCHAR *ColumnNameW, 
		   SQLSMALLINT NameLength4,
           SQLWCHAR *TableTypeW, 
		   SQLSMALLINT NameLength5,
		   SQLUSMALLINT IdentifierType,
		   SQLUSMALLINT Scope,
		   SQLUSMALLINT Nullable,
		   SQLSMALLINT SqlType,
		   SQLUSMALLINT Unique,
		   SQLUSMALLINT Reserved,
		   SQLWCHAR *FKCatalogNameW, 
		   SQLSMALLINT NameLength6,
           SQLWCHAR *FKSchemaNameW, 
		   SQLSMALLINT NameLength7,
           SQLWCHAR *FKTableNameW, 
		   SQLSMALLINT NameLength8)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;
	
	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		rc = pStmt->GetSQLCatalogs(odbcAPI, CatalogNameW, NameLength1, SchemaNameW, 
							NameLength2, TableNameW, NameLength3, ColumnNameW, NameLength4,
							TableTypeW, NameLength5, IdentifierType, Scope, Nullable, SqlType, Unique, Reserved,
							FKCatalogNameW, NameLength6, FKSchemaNameW, NameLength7, FKTableNameW, NameLength8);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN ODBC::GetCursorName(SQLHSTMT StatementHandle,
           SQLWCHAR *CursorName, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLengthPtr)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;
	
	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->GetCursorName(CursorName, BufferLength, NameLengthPtr);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN ODBC::SetCursorName(SQLHSTMT StatementHandle,
           SQLWCHAR *CursorName, 
		   SQLSMALLINT NameLength)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->SetCursorName(CursorName, NameLength);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN ODBC::RowCount(
	   SQLHSTMT StatementHandle, 
	   SQLLEN	*RowCountPtr)
{
	CStmt	*pStmt;
	CConnect	*pConnect;
	INT64 tempRowCnt64;
	
	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->clearError();
		pStmt->setDiagRowCount(0, -1);
		if (RowCountPtr != NULL)
		{
		#if defined(_WIN64)
			*RowCountPtr = pStmt->getRowCount();
		#else
			tempRowCnt64 = pStmt->getRowCount();
			if(tempRowCnt64 > MAXLONG)
			{
				char ErrorBuffer[256];
				sprintf(ErrorBuffer,"Numeric Overflow: the row count %I64d exceeds the maximum value that can be returned by SQLRowCount",tempRowCnt64);
				pStmt->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, ErrorBuffer);
				return SQL_ERROR;
			}
			else
				*RowCountPtr = tempRowCnt64;
		#endif /* _WIN64 */
		} /* RowCountPtr != NULL */
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return SQL_SUCCESS;
} // ODBC::RowCount


SQLRETURN ODBC::Cancel(SQLHSTMT StatementHandle)
{

	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	//If multiple SQLCancel() have been called, after the first one, return here
	if(pStmt->m_CancelCalled)
		return SQL_ERROR;

	// We can sometimes have race conditions attempting to cancel a query right after its been submitted
	// for execution (timing issue with the query registering with the cancel broker etc).
	// to prevent that we introduce a 10 second delay when cancel is called.
	Sleep(10000); 

//	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->Cancel();
	}
	__finally{
//		LeaveCriticalSection(&pConnect->m_CSObject);
	}
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
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->ColAttribute(ColumnNumber, FieldIdentifier, CharacterAttributePtr, BufferLength,
						StringLengthPtr, NumericAttributePtr);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}


SQLRETURN ODBC::Execute(SQLHSTMT StatementHandle)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->Execute();
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN ODBC::ParamData(SQLHSTMT StatementHandle,
		SQLPOINTER *ValuePtrPtr)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->ParamData(ValuePtrPtr);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;

}

SQLRETURN ODBC::PutData(SQLHSTMT StatementHandle,
           SQLPOINTER DataPtr, 
		   SQLLEN StrLen_or_Ind)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->PutData(DataPtr, StrLen_or_Ind);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN ODBC::Fetch(SQLHSTMT StatementHandle)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->Fetch();
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
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
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->ExtendedFetch(FetchOrientation,FetchOffset,RowCountPtr,RowStatusArray);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN ODBC::FetchScroll(SQLHSTMT StatementHandle,
			SQLUSMALLINT FetchOrientation,
			SQLINTEGER FetchOffset)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->FetchScroll(FetchOrientation,FetchOffset);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
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
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->GetData(ColumnNumber, TargetType, TargetValuePtr, BufferLength, StrLen_or_IndPtr);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN ODBC::SetPos(SQLHSTMT StatementHandle,
    SQLUSMALLINT	RowNumber,
    SQLUSMALLINT    Operation,
    SQLUSMALLINT    LockType)
{
	SQLRETURN	rc;
	CStmt		*pStmt;
	CConnect	*pConnect;

	pStmt = (CStmt *)StatementHandle;
	pConnect = pStmt->getConnectHandle();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		pStmt->setDiagRowCount(0, -1);
		rc = pStmt->SetPos(RowNumber, Operation, LockType);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}
