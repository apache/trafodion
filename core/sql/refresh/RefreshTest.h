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
//
**********************************************************************/
#ifndef _REFRESH_TEST_H_
#define _REFRESH_TEST_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RefreshTest.h
* Description:  Test engine for the refresh utility
*
*
* Created:      01/09/2001
* Language:     C++
* 
*
* 
******************************************************************************
*/


#include "uofsProcessPool.h"
#include "RuSQLDynamicStatementContainer.h"
#include "RuTestTaskExecutor.h"
#include "RuOptions.h"
#include "RuGlobals.h"
#include "RuJournal.h"
#include "dsptrlist.h"
#include "dmresultset.h"
#include "uofsTransaction.h"

struct GroupStatement;

class RefreshTestController {
public:
	enum {	EX_SEND_FOR_INIT,
			EX_WAIT_FOR_ALL_RETURN_FROM_COMPILATION,
			EX_SEND_FOR_EXECUTE,
			EX_WAIT_FOR_ALL_RETURN_FROM_EXECUTION,
			EX_COMPLETE
	};

public:
	
	RefreshTestController(Int32 numOfProcesses) :
	  numOfProcesses_(numOfProcesses),
	  activeProcesses_(0),
	  processPool_("tdm_arkutp.exe"),
	  dynamicSQLContainer_(2),
	  currentGroupId_(0),
	  executorsList_(CDSPtrList<CRUTestTaskExecutor>::eItemsAreOwned),
	  groupList_(CDSPtrList<GroupStatement>::eItemsAreOwned),
	  state_(EX_SEND_FOR_INIT),
	  endFlag_(FALSE),
	  pJournal_(NULL)
	  {}

	virtual ~RefreshTestController() 
	{}

public:
	Int32 GetState() { return state_; }
	void SetState(Int32 state) { state_ = state; }

public:
	void Init();
	void Work();

private:
	Int32 InitiateTaskProcess();
	void GroupSendForInitialization();
	void SendForInitialization(Int32 groupId,Int32 processId,Int32 numOfStmt);
	void SendExecutor(CRUTaskExecutor &executor);
	void WaitForAll();
	void SendSyncToAllExecutors();
	void HandleReturnOfExecutor(Lng32 pid);
	CRUTaskExecutor *FindRunningExecutor(Lng32 pid);
	void SaveTime(Int32 groupId,	
			 	  Int32 processId);
private:
	Int32 state_;
	Int32 numOfProcesses_;
	Int32 activeProcesses_;
	Int32 currentGroupId_;
	BOOL endFlag_;

	CUOFsTaskProcessPool processPool_;
	CRUSQLDynamicStatementContainer dynamicSQLContainer_;

	CDSPtrList<CRUTestTaskExecutor> executorsList_;

	CDSPtrList<GroupStatement> groupList_;

	DSListPosition currentPos_;

	CRUOptions options_;

	//-- Output file , the journal can be initialized only after we 
	// receive the globals message that contains the output filename
	CRUJournal *pJournal_;
	CUOFsTransManager transManager_;
	
};

struct GroupStatement {
	Int32 groupId_;
	Int32 processId_;
	Int32 num_of_executions;
};


#endif
