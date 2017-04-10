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

#pragma once
#include "../../common/Global.h"
#include "../../common/TestBase.h"
#define LARGECOL
class CLargeVarcharCol :public CTestBase
{
public:
	CLargeVarcharCol(const char * chDsn, const char * chUID, const char * chPwd);
	~CLargeVarcharCol();
private:
	CLargeVarcharCol();

private:
	bool Prepare();
	bool TestGo();
	void CleanUp();

	bool InsertCharToVarcharCol(int schema);
	bool InsertWCharToVarcharCol(int schema);
	bool InsertCharToLongVarcharCol(int schema);
	bool InsertWCharToLongVarcharCol(int schema);
	bool VarcharToULong(int schema);
	bool VarcharToDate(int schema);
	bool VarcharToDouble(int schema);
	bool VarcharToTime(int schema);
	bool VarcharToTimestamp(int schema);
	bool VarcharToInterval(int schema);
};

