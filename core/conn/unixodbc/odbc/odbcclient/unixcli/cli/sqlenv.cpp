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
#include "sqlenv.h"
#include "cenv.h"

SQLRETURN ODBC::SetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute,
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	SQLRETURN	rc;
	CEnv		*pEnv;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_ENV, EnvironmentHandle))
		return SQL_INVALID_HANDLE;
	pEnv = (CEnv *)EnvironmentHandle;
	pEnv->clearError();
	rc = pEnv->SetEnvAttr(Attribute, Value, StringLength);
	return rc;
}

SQLRETURN ODBC::GetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER ValuePtr,
           SQLINTEGER BufferLength, 
		   SQLINTEGER *StringLengthPtr)
{
	SQLRETURN	rc;
	CEnv		*pEnv;

	if (! gDrvrGlobal.gHandle.validateHandle(SQL_HANDLE_ENV, EnvironmentHandle))
		return SQL_INVALID_HANDLE;
	pEnv = (CEnv *)EnvironmentHandle;
	pEnv->clearError();
	rc = pEnv->GetEnvAttr(Attribute, ValuePtr, BufferLength, StringLengthPtr);
	return rc;
}
