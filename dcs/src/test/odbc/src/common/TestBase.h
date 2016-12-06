/*************************************************************************
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
**************************************************************************/
#ifndef ODBC_TEST_BASE_H
#define ODBC_TEST_BASE_H

#include "Global.h"

class CTestBase
{
protected:
	char * m_chDsn;
	char * m_chUID;
	char * m_chPwd;
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
	SQLHWND hWnd;

public:
	CTestBase(const char * chDsn, const char * chUID, const char * chPwd);
	virtual bool Run();
	virtual ~CTestBase();

private:
	CTestBase();
protected:
	virtual bool Connect();
	virtual bool Prepare() = 0;
	virtual bool TestGo() = 0;
	virtual void CleanUp() = 0;
	virtual void Disconnect();
	virtual void LogDiagnostics(const char *sqlFunction, SQLRETURN rc);
};

#endif // !ODBC_TEST_BASE_H