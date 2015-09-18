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
#include "tdm_odbcDrvMsg.h"
#include "CEnv.h"
#include "DrvrGlobal.h"
#include "CConnect.h"

// Implements the member functions of CEnv

CEnv::CEnv(SQLHANDLE InputHandle) : CHandle(SQL_HANDLE_ENV, InputHandle)
{
	m_ODBCVersion = SQL_OV_ODBC3;
}

SQLRETURN CEnv::SetEnvAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength)
{
	
	SQLRETURN rc = SQL_SUCCESS;

	switch (Attribute)
	{
	case SQL_ATTR_CONNECTION_POOLING:			// Implemented by DM
		break;
	case SQL_ATTR_CP_MATCH:						// Implemented by DM
		break;
	case SQL_ATTR_ODBC_VERSION:
		m_ODBCVersion = (long)ValuePtr;
		break;
	case SQL_ATTR_OUTPUT_NTS:					// Implemented by DM
		break;									
	}
	return rc;
}

SQLRETURN CEnv::GetEnvAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER BufferLength,
								   SQLINTEGER *StringLengthPtr)
{
	
	SQLRETURN rc = SQL_SUCCESS;
	RETURN_VALUE_STRUCT		retValue;
	
	retValue.dataType = DRVR_PENDING;
	retValue.u.strPtr = NULL;

	switch (Attribute)
	{
	case SQL_ATTR_CONNECTION_POOLING:			// Implemented by DM
		break;
	case SQL_ATTR_CP_MATCH:						// Implemented by DM
		break;
	case SQL_ATTR_ODBC_VERSION:
		retValue.u.s32Value = m_ODBCVersion;
		retValue.dataType = SQL_IS_INTEGER;
		break;
	case SQL_ATTR_OUTPUT_NTS:					// Implemented by DM
		break;									
	}
	if (rc == SQL_SUCCESS)
		rc = returnAttrValue(TRUE, this, &retValue, ValuePtr, BufferLength, StringLengthPtr);
	return rc;
}

SQLRETURN CEnv::AllocHandle(SQLSMALLINT HandleType,SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
	SQLRETURN rc = SQL_SUCCESS;

	CConnect* pConnect = new CConnect(InputHandle);
	if (pConnect != NULL)
	{
		if ((rc = pConnect->initialize()) == SQL_SUCCESS)
			*OutputHandle = pConnect;
		else
		{
			delete pConnect;
			rc = SQL_ERROR;
		}
	}
	else
	{
		setDiagRec(DRIVER_ERROR, IDS_HY_001);
		rc = SQL_ERROR;
	}
	return rc;
}
