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
#include "sqldesc.h"
#include "cdesc.h"

SQLRETURN ODBC::SetDescField(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength)
{
	SQLRETURN	rc = SQL_SUCCESS;
	CDesc	*pDesc;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_DESC, DescriptorHandle))
		return SQL_INVALID_HANDLE;
	pDesc = (CDesc *)DescriptorHandle;
	rc = pDesc->SetDescField(RecNumber, FieldIdentifier, ValuePtr, BufferLength);
	return rc;
}

SQLRETURN ODBC::SetDescRec(SQLHDESC DescriptorHandle,
           SQLSMALLINT	RecNumber,
		   SQLSMALLINT	Type,
           SQLSMALLINT	SubType, 
		   SQLLEN	Length,
           SQLSMALLINT	Precision, 
		   SQLSMALLINT	Scale,
           SQLPOINTER	DataPtr, 
		   SQLLEN	*StringLengthPtr,
           SQLLEN	*IndicatorPtr)
{
	SQLRETURN	rc;
	CDesc	*pDesc;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_DESC, DescriptorHandle))
		return SQL_INVALID_HANDLE;
	pDesc = (CDesc *)DescriptorHandle;
	rc = pDesc->SetDescRec(RecNumber, Type, SubType,
				Length, Precision, Scale, DataPtr, StringLengthPtr, IndicatorPtr);
	return rc;
}

SQLRETURN ODBC::GetDescField(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength,
           SQLINTEGER *StringLengthPtr)
{
	SQLRETURN	rc;
	CDesc	*pDesc;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_DESC, DescriptorHandle))
		return SQL_INVALID_HANDLE;
	pDesc = (CDesc *)DescriptorHandle;
	rc = pDesc->GetDescField(RecNumber, FieldIdentifier, ValuePtr, BufferLength, StringLengthPtr);
	return rc;
}


SQLRETURN ODBC::GetDescRec(SQLHDESC DescriptorHandle,
           SQLSMALLINT	RecNumber, 
		   SQLCHAR		*Name,
           SQLSMALLINT	BufferLength, 
		   SQLSMALLINT	*StringLengthPtr,
           SQLSMALLINT	*TypePtr, 
		   SQLSMALLINT	*SubTypePtr, 
           SQLLEN  	    *LengthPtr, 
		   SQLSMALLINT	*PrecisionPtr, 
           SQLSMALLINT	*ScalePtr, 
		   SQLSMALLINT	*NullablePtr)
{
	SQLRETURN	rc;
	CDesc	*pDesc;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_DESC, DescriptorHandle))
		return SQL_INVALID_HANDLE;
	pDesc = (CDesc *)DescriptorHandle;
	rc = pDesc->GetDescRec(RecNumber, Name, BufferLength, StringLengthPtr, TypePtr, SubTypePtr,
				LengthPtr, PrecisionPtr, ScalePtr, NullablePtr);
	return rc;
}

SQLRETURN ODBC::CopyDesc(SQLHDESC SourceDescHandle,
		SQLHDESC TargetDescHandle)
{
	SQLRETURN	rc;
	CDesc	*pDesc;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_DESC, SourceDescHandle))
		return SQL_INVALID_HANDLE;
	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_DESC, TargetDescHandle))
		return SQL_INVALID_HANDLE;

	pDesc = (CDesc *)TargetDescHandle;
	rc = pDesc->CopyDesc(SourceDescHandle);
	return rc;
}

