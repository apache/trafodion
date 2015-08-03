/*************************************************************************
*
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
//
// MODULE: srvrJdbcConnect.h
//
#ifndef _SRVRJDBCCONNECT_DEFINED
#define _SRVRJDBCCONNECT_DEFINED
#include <windows.h>
#ifdef NSK_PLATFORM
	#include <sqlWin.h>
#else
	#include <sql.h>
#endif
#include <sqlExt.h>

class SRVR_CONNECT_HDL
{
	public:
		SRVR_CONNECT_HDL();
		SQLRETURN sqlConnect(const char *uid, const char *pwd);
		void cleanupSQLMessage();
		const ERROR_DESC_LIST_def *getSQLError();

	private:
		SQLCTX_HANDLE contextHandle;
		ERROR_DESC_LIST_def		sqlWarning;
		ERROR_DESC_LIST_def		sqlError;
	
};

#endif
