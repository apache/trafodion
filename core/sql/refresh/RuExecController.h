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
#ifndef _RU_EXEC_CONTROLLER_H_
#define _RU_EXEC_CONTROLLER_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuExecController.h
* Description:  Definition of class CRUExecController
*
* Created:      05/07/2000
* Language:     C++
*
*
*
******************************************************************************
*/

#include "refresh.h"

#include "RuRuntimeController.h"
#include "RuTask.h"

#include "uofsProcessPool.h"

class CRUTaskExecutor;

#include "dslist.h"

//--------------------------------------------------------------//
//	CRUExecController
//	
//	This class manages the local and remote execution of tasks
//	scheduled by the flow controller. Upon receiving a request
//	to execute a task, it builds the task executor, and performs
//	the execution in one or more steps, applying the executor 
//	as a finite-state machine.
//
//	A part of execution steps may be performed remotely. To
//	support remote execution, the controller manages a pool
//	of server processes to dispatch the task executors to, and
//	performs the communication between the main process and the
//	servers.
//
//--------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUExecController : public CRURuntimeController {

private:
	typedef CRURuntimeController inherited;

public:
	CRUExecController(BOOL useParallelism);
	virtual ~CRUExecController() {}

public:
	// Close all Task Processes
	void ShutDownTaskProcesses();

protected:
	//-- Pure virtual function implementation
	//-- The actual switch for event handling
	virtual void HandleRequest(CRURuntimeControllerRqst *pRequest);

private:
	//-- Prevent copying
	CRUExecController(const CRUExecController &other);
	CRUExecController &operator = (const CRUExecController &other);

private:
	//-- Individual event handlers

	void HandleStartTaskRqst(CRURuntimeControllerRqst *pRqst);
	void HandleFinishTaskRqst(CRURuntimeControllerRqst *pRqst);

	void HandleExecuteTaskStepRqst(CRURuntimeControllerRqst *pRqst);

	void HandleAwaitEventRqst();

private:
	//-- Allocate/dealloacte a process associated with a task
	void AllocateTaskProcess(CRUTask &task);
	void DeAllocateTaskProcess(CRUTask &task);

	// AllocateTaskProcess() callee
	// Launch the process and send a message with CRUGlobals
	Lng32 InitiateTaskProcess();

private:
	//-- HandleExecuteTaskStepRqst() callees
	void HandleRemoteTaskStepExecution(CRUTask &task);
	void HandleLocalTaskStepExecution(CRUTask &task);

	// Start the remote execution ...
	void ShipWorkToRemoteProcess(CRUTask &task);
	void SerializeTaskExecutor(CRUTaskExecutor &executor);

	// and complete it
	void HandleReturnOfRemoteExecutor(Lng32 pid);
	void HandleRemoteExecutorSuccess(
		CRUTask *pTask,
		CUOFsIpcMessageTranslator &translator);
	void HandleRemoteExecutorFailure(
		CRUTask *pTask,
		Lng32 pid,
		CUOFsIpcMessageTranslator &translator);

	CRUTask *FindRunningTask(Lng32 pid);

	void HandleRequestFailure(
		CRURuntimeControllerRqst *pRqst, 
		CDSException &ex);

	void HandleTaskFailure(CRUTask &task, CDSException &ex);

private:
	// The tasks that are currently being executed
	CRUTaskList runningTaskList_;
	
	CUOFsTaskProcessPool processPool_;
	
	BOOL useParallelism_;

private:
	static const char* taskServerName_;
};

#endif
