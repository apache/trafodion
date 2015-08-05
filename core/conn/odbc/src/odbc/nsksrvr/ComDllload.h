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
//********************************************************************/
#ifndef COMDLLLOAD_H
#define COMDLLLOAD_H

typedef struct tag_context
{
	void context() { handle = 0; timeout = MGR_TIMEOUT; bzero(&exception_,sizeof(exception_));
				bzero(errorMsg,sizeof(errorMsg)); 
				PROCESSHANDLE_NULLIT_(TPT_REF(WmsHandle));
				requesterType = REQUESTER_INIT;
				bzero(requesterApplication, sizeof(requesterApplication));
				bzero(requesterComputer, sizeof(requesterComputer));
				bzero(requesterName, sizeof(requesterName));
				bzero(requesterRole, sizeof(requesterRole));
				bzero(requesterDBUserName, sizeof(requesterDBUserName));}
	struct qrysrvc_exc_ exception_; //Out
	char errorMsg[500];				//Out
	long handle;					//In
	long timeout;					//In
	TPT_DECL(WmsHandle);
	REQUESTER requesterType;
	char requesterApplication[MAX_APPLICATION_NAME_LEN +1];
	char requesterComputer[MAX_COMPUTER_NAME_LEN + 1];
    char requesterName[USERNAME_LENGTH + 1];
    char requesterRole[MAX_ROLE_LEN+1];
    char requesterDBUserName[MAX_ROLE_LEN+1];
	bitmask_type privMask;

} context,* pcontext;

#define ComDll				"libzqs.so"
#define ComCloseConnection	"NskComCloseConnection"
#define ComSetQeryService	"NskComSetQeryService"
#define ComOpenConnection	"NskComOpenConnection"
//#define ComExecDirect		"NskComExecDirect"
//#define ComFetch			"NskComFetch"
#define ComFastExecDirect	"NskComFastExecDirect"
#define ComFastFetch		"NskComFastFetch"

typedef bool (*NskComSetQeryService) (char*);
typedef long (*NskComOpenConnection) (pcontext);
//typedef void (*NskComExecDirect) (pcontext, const IDL_char *,IDL_string, SQLItemDescList_def*);
//typedef void (*NskComFetch) (pcontext, const IDL_char *,IDL_long,IDL_long *,SQL_DataValue_def *);
typedef void (*NskComCloseConnection) (pcontext);

typedef void (*NskComFastExecDirect) (pcontext, const IDL_char *,IDL_string, IDL_long& SqlQueryType, IDL_long& outputDescLength,BYTE* &outputDesc);
typedef void (*NskComFastFetch) (pcontext, const IDL_char *,IDL_long,IDL_long *,SQL_DataValue_def *);
	
typedef struct tag_session
{
	void qs_session() { hzcomdll = 0; bWmsOpen = false; }
	tag_context context;
	dlHandle hzcomdll;
	bool bWmsOpen;
	NskComSetQeryService pComSetQeryService;
	NskComOpenConnection pComOpenConnection;
//	NskComExecDirect pComExecDirect;
//	NskComFetch pComFetch;
	NskComFastExecDirect pComFastExecDirect;
	NskComFastFetch pComFastFetch;
	NskComCloseConnection pComCloseConnection;
} qs_session;

extern qs_session qs;

#endif
