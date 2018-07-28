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

#ifndef DRVRNET_H
#define DRVRNET_H

#include "CHandle.h"

typedef struct CATALOG_API_PARAMS
{
	const IDL_char				*catalogNm;
	const IDL_char				*schemaNm;
	const IDL_char				*tableNm;
	const IDL_char				*tableTypeList;
	const IDL_char				*columnNm;
	IDL_long					columnType;
	IDL_long					rowIdScope;
	IDL_long					nullable;
	IDL_long					uniqueness;
	IDL_long					accuracy;
	IDL_short					sqlType;
	IDL_unsigned_long			metadataId;
	const IDL_char				*fkcatalogNm;
	const IDL_char				*fkschemaNm;
	const IDL_char				*fktableNm;
} CATALOG_API_PARAMS;

typedef struct SRVR_CALL_CONTEXT
{
	SRVR_CALL_CONTEXT(){
		maxRowsetSize=-1;
	};
	~SRVR_CALL_CONTEXT(){
	};
	IDL_short					odbcAPI;
	CHandle					*sqlHandle;	
	IDL_short					pingAttempts;
	IDL_Object				Ping_ObjRef;
	IDL_Object				SQLSvc_ObjRef;
	IDL_Object				ASSvc_ObjRef;
	HANDLE					eventHandle;
	SQLUINTEGER				connectionTimeout;
	SQLUINTEGER				statementTimeout;
	DIALOGUE_ID_def			dialogueId;
	SRVR_STATE				srvrState;
	IDL_long					maxRowsetSize;
	union
	{	
		struct SEND_SQL_COMMAND_PARAMS
		{
			SQLUINTEGER		queryTimeout;
			BOOL			asyncEnable;
			BOOL			holdableCursor;			
			IDL_char		*stmtLabel;
			IDL_char		*stmtName;
			IDL_char		*cursorName;
			IDL_char		*moduleName;
			const IDL_char		*sqlString;
			DWORD			sqlStmtType;
		} sendSQLcommandParams;
		struct EXECUTE_PARAMS
		{
			SQLUINTEGER		queryTimeout;
			BOOL			asyncEnable;
			BOOL			holdableCursor;
			IDL_char		*stmtLabel;
			IDL_char		*cursorName;
			DWORD			sqlStmtType;
			IDL_long		rowCount;
			const SQLValueList_def *inputValueList;
		} executeParams;
		struct FETCH_PARAMS
		{
			SQLUINTEGER				queryTimeout;
			BOOL					asyncEnable;
			IDL_char					*stmtLabel;
			IDL_unsigned_long_long		maxRowCount;
			IDL_unsigned_long_long		maxRowLen;
		} fetchParams;
		struct CLOSE_PARAMS
		{
			IDL_char					*stmtLabel;
			IDL_short					option;
		} closeParams;
		struct GETSQLCATALOGS_PARAMS
		{
			IDL_char					*stmtLabel;
			CATALOG_API_PARAMS		catalogAPIParams;			
		} getSQLCatalogsParams;
		struct CONNECT_PARAMS
		{
			SQLUINTEGER				loginTimeout;
			CONNECTION_CONTEXT_def	*inContext;
			USER_DESC_def			*userDesc;
		} connectParams;
		struct SETCONNECT_PARAMS
		{
			SQLUINTEGER				attribute;
			SQLUINTEGER				valueNum;
			SQLPOINTER				valueStr;
		} setConnectParams;
        struct EXTRACTLOB_PARAMS
        {
            IDL_short               extractType;
            IDL_string              lobHandle;
            IDL_long                lobHandleLen;
            IDL_long                extractLen;
            BYTE *                  extractData;
        } extractLobParams;
        struct UPDATELOB_PARAMS
        {
            IDL_long                updateType;
            IDL_string              lobHandle;
            IDL_long                lobHandleLen;
            IDL_long_long           totalLength;
            IDL_long_long           offset;
            IDL_long_long           pos;
            IDL_long_long           length;
            BYTE *                  data;
        } updateLobParams;
		SQLSMALLINT					completionType;
	} u;

} SRVR_CALL_CONTEXT;

typedef struct _SQLFetchDataValue
{
	IDL_unsigned_long	numberOfElements;
	IDL_unsigned_long	numberOfRows;
	LONG_PTR*	rowAddress;
} SQLFetchDataValue;


extern long stopSrvrAbrupt(SRVR_CALL_CONTEXT *srvrCallContext);
extern UINT __stdcall ThreadControlProc(LPVOID pParam);

extern "C" SQLRETURN ENDTRANSACT(SRVR_CALL_CONTEXT *srvrCallContext);
extern "C" SQLRETURN GETOBJREF(SRVR_CALL_CONTEXT *srvrCallContext);
extern "C" SQLRETURN INITIALIZE_DIALOG(SRVR_CALL_CONTEXT *srvrCallContext);
extern "C" SQLRETURN TERMINATE_DIALOG(SRVR_CALL_CONTEXT *srvrCallContext);
extern "C" SQLRETURN SETCONNECT(SRVR_CALL_CONTEXT *srvrCallContext);
extern "C" SQLRETURN FREESTATEMENT(SRVR_CALL_CONTEXT *srvrCallContext);
extern "C" SQLRETURN SMDCATALOGS(SRVR_CALL_CONTEXT *srvrCallContext);
extern "C" SQLRETURN SQLPREPARE_(SRVR_CALL_CONTEXT *srvrCallContext);
extern "C" SQLRETURN SQLFETCH_(SRVR_CALL_CONTEXT *srvrCallContext);
extern "C" SQLRETURN SQLEXECUTE_(SRVR_CALL_CONTEXT *srvrCallContext);
extern "C" SQLRETURN EXTRACTLOB(SRVR_CALL_CONTEXT *srvrCallContext);
extern "C" SQLRETURN UPDATELOB(SRVR_CALL_CONTEXT *srvrCallContext);

extern "C" SQLRETURN checkNetStmt(SRVR_CALL_CONTEXT *srvrCallContext,CEE_status sts, char* procname);

#endif
