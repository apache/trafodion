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
/**************************************************************************
**************************************************************************/
//
//
#ifndef CENV_H
#define CENV_H

#include "chandle.h"

class CEnv : public CHandle {

public:
	CEnv(SQLHANDLE InputHandle);
	~CEnv();
	SQLRETURN SetEnvAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength);
	SQLRETURN GetEnvAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER BufferLength,
								   SQLINTEGER *StringLengthPtr);
	SQLRETURN AllocHandle(SQLSMALLINT HandleType,SQLHANDLE InputHandle, SQLHANDLE *OutputHandle);
	inline SQLINTEGER getODBCAppVersion() { return m_ODBCVersion; };
public:
	// Collection of Connect Handles in this Env
	CHANDLECOLLECT	m_ConnectCollect;
private:
	SQLINTEGER	m_ODBCVersion;	// Application ODBC Version

};

#endif
