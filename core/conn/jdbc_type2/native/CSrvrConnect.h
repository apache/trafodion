/**************************************************************************
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
// MODULE: CSrvrConnect.h
//

#ifndef _CSRVRCONNECT_DEFINED
#define _CSRVRCONNECT_DEFINED
#include <vector>
#include <list>
#include <set>
#include <string>
#include <platform_ndcs.h>
#include <sql.h>
#include <sqlext.h>
#include "SrvrCommon.h"
//using namespace std;
//Stan's performance improvement changes
#include <ext/hash_map>
namespace __gnu_cxx
{
template <>
struct hash<std::string> {
        size_t operator() (const std::string& x) const {
                return hash<const char*>()(x.c_str());
        // hash<const char*> already exists
         }
    };
}

typedef __gnu_cxx::hash_map<long, SRVR_STMT_HDL*> MapOfSrvrStmt;
typedef __gnu_cxx::hash_map<std::string, SRVR_STMT_HDL*> MapOfInternalSrvrStmt;

class SRVR_CONNECT_HDL
{
public:
	SRVR_CONNECT_HDL();
	~SRVR_CONNECT_HDL();
	SQLRETURN sqlConnect(const char *uid, const char *pwd);
	SQLRETURN sqlClose();
	SQLRETURN switchContext(long *sqlcode);
	void cleanupSQLMessage();
	const ERROR_DESC_LIST_def *getSQLError();

	void addSrvrStmt(SRVR_STMT_HDL *pSrvrStmt,BOOL internalStmt=FALSE);
	void removeSrvrStmt(SRVR_STMT_HDL *pSrvrStmt);
	SRVR_STMT_HDL *createSrvrStmt(const char *stmtLabel,
		long	*sqlcode,
		const char *moduleName,
		long moduleVersion,
		long long moduleTimestamp,
		short	sqlStmtType,
		BOOL	useDefaultDesc,
		BOOL internalStmt = FALSE,
		long stmtId = 0,
		short sqlQueryType = SQL_UNKNOWN,
		Int32  resultSetIndex = 0,
		SQLSTMT_ID* callStmtId = NULL);

	SRVR_STMT_HDL *createSpjrsSrvrStmt(SRVR_STMT_HDL *pSrvrStmt,
		const char *stmtLabel,
		long	*sqlcode,
		const char *moduleName,
		long moduleVersion,
		long long moduleTimestamp,
		short	sqlStmtType,
		long	RSindex,
		const char *RSstmtName,
		BOOL	useDefaultDesc);

	
	SRVR_STMT_HDL *getSrvrStmt(long dialogueId,long stmtId,long	*sqlcode);
	SRVR_STMT_HDL *getInternalSrvrStmt(long dialogueId, const char* stmtLabel, long *sqlcode);

	long getStmtCount()
	{
		return count;
	}
	
	inline void setCurrentStmt(SRVR_STMT_HDL *pSrvrStmt) { pCurrentSrvrStmt = pSrvrStmt;};
public:
	SQLCTX_HANDLE			contextHandle;
	ERROR_DESC_LIST_def		sqlWarning;
	odbc_SQLSvc_SQLError	sqlError;
	BOOL					isClosed;
	BOOL                    isSPJRS;
	
	char				DefaultCatalog[129];
	char				DefaultSchema[129];
	char				CurrentCatalog[129];
	char				CurrentSchema[129];
	
private:
	SRVR_STMT_HDL_LIST		*pSrvrStmtListHead;
	
	MapOfSrvrStmt mapOfSrvrStmt;
	MapOfInternalSrvrStmt mapOfInternalSrvrStmt;

	SRVR_STMT_HDL			*pCurrentSrvrStmt;
	long					count;
	std::set<std::string> setOfLoadedModules;
	SRVR_STMT_HDL *pSrvrStmtInternal;
};

#endif
