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
#include "SQLDesc.h"
#include "CDesc.h"

SQLRETURN  ODBC::SetDescField(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength)
{
	SQLRETURN	rc = SQL_SUCCESS;
	CConnect	*pConnect;
	CDesc	*pDesc;

	pDesc = (CDesc *)DescriptorHandle;
	pConnect=pDesc->getDescConnect();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		rc = pDesc->SetDescField(RecNumber, FieldIdentifier, ValuePtr, BufferLength);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN  ODBC::SetDescRec(SQLHDESC DescriptorHandle,
           SQLSMALLINT	RecNumber,
		   SQLSMALLINT	Type,
           SQLSMALLINT	SubType, 
		   SQLLEN		Length,
           SQLSMALLINT	Precision, 
		   SQLSMALLINT	Scale,
           SQLPOINTER	DataPtr, 
		   SQLLEN		*StringLengthPtr,
           SQLLEN		*IndicatorPtr)
{
	SQLRETURN	rc;
	CConnect	*pConnect;
	CDesc	*pDesc;

	pDesc = (CDesc *)DescriptorHandle;
	pConnect=pDesc->getDescConnect();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		rc = pDesc->SetDescRec(RecNumber, Type, SubType,
				Length, Precision, Scale, DataPtr, StringLengthPtr, IndicatorPtr);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN  ODBC::GetDescField(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength,
           SQLINTEGER *StringLengthPtr)
{
	SQLRETURN	rc;
	CConnect	*pConnect;
	CDesc	*pDesc;

	pDesc = (CDesc *)DescriptorHandle;
	pConnect=pDesc->getDescConnect();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		rc = pDesc->GetDescField(RecNumber, FieldIdentifier, ValuePtr, BufferLength, StringLengthPtr);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}


SQLRETURN  ODBC::GetDescRec(SQLHDESC DescriptorHandle,
           SQLSMALLINT	RecNumber, 
		   SQLWCHAR		*NameW,
           SQLSMALLINT	BufferLength, 
		   SQLSMALLINT	*StringLengthPtr,
           SQLSMALLINT	*TypePtr, 
		   SQLSMALLINT	*SubTypePtr, 
           SQLLEN		*LengthPtr, 
		   SQLSMALLINT	*PrecisionPtr, 
           SQLSMALLINT	*ScalePtr, 
		   SQLSMALLINT	*NullablePtr)
{
	SQLRETURN	rc;
	CConnect	*pConnect;
	CDesc	*pDesc;

	pDesc = (CDesc *)DescriptorHandle;
	pConnect=pDesc->getDescConnect();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		rc = pDesc->GetDescRec(RecNumber, NameW, BufferLength, StringLengthPtr, TypePtr, SubTypePtr,
				LengthPtr, PrecisionPtr, ScalePtr, NullablePtr);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}

SQLRETURN ODBC::CopyDesc(SQLHDESC SourceDescHandle,
		SQLHDESC TargetDescHandle)
{
	SQLRETURN	rc;
	CConnect	*pConnect;
	CDesc	*pDesc;

	pDesc = (CDesc *)TargetDescHandle;
	pConnect=pDesc->getDescConnect();
	EnterCriticalSection(&pConnect->m_CSObject);
	__try{
		rc = pDesc->CopyDesc(SourceDescHandle);
	}
	__finally{
		LeaveCriticalSection(&pConnect->m_CSObject);
	}
	return rc;
}
	
