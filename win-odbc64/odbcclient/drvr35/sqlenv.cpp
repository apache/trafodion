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
#include "SQLEnv.h"
#include "CEnv.h"

SQLRETURN  ODBC::SetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute,
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	SQLRETURN	rc;
	CEnv		*pEnv;

	pEnv = (CEnv *)EnvironmentHandle;
	EnterCriticalSection(&pEnv->m_CSObject);
	__try{
		pEnv->clearError();
		rc = pEnv->SetEnvAttr(Attribute, Value, StringLength);
	}
	__finally{
		LeaveCriticalSection(&pEnv->m_CSObject);
	}
	return rc;
}

SQLRETURN  ODBC::GetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER ValuePtr,
           SQLINTEGER BufferLength, 
		   SQLINTEGER *StringLengthPtr)
{
	SQLRETURN	rc;
	CEnv		*pEnv;

	pEnv = (CEnv *)EnvironmentHandle;
	EnterCriticalSection(&pEnv->m_CSObject);
	__try{
		pEnv->clearError();
		rc = pEnv->GetEnvAttr(Attribute, ValuePtr, BufferLength, StringLengthPtr);
	}
	__finally{
		LeaveCriticalSection(&pEnv->m_CSObject);
	}
	return rc;
}
