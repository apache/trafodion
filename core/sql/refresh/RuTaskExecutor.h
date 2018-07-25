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
#ifndef _RU_TASK_EXECUTOR_H_
#define _RU_TASK_EXECUTOR_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuTaskExecutor.h
* Description:  Definition of class CRUTaskExecutor.
*
* Created:      8/23/1999
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"
#include "dsplatform.h"
#include "dsstring.h"

#include "RuException.h"
#include "RuGlobals.h"

class CUOFsIpcMessageTranslator;

//--------------------------------------------------------------------------//
// CRUTaskExecutor
//
//   A generic task executor abstract class. 
//
//	 The classes that derive from this class will implement  
//	 the concrete task functionality.
//  
//	 The generic task executor works as a finite-state machine (FSM).
//	 Its execution starts from the EX_START state, and ends in the
//	 EX_COMPLETE state. The intermediate states are up to the 
//	 derived classes. A single execution step (i.e., transition 
//	 between states) is handled by the call to the Work() method,
//	 which should be provided by the derived classes.
//
//	 The Work() method may throw exceptions when errors happen. I.e.,
//	 it does not have to provide its own exception handlers. These
//	 exceptions will be handled at some upper level. 
//
//	 A single task's step can be either executed under a single
//	 transaction, or under no transaction at all. The executor 
//	 object will keep its transactional context from the previous
//	 step. The object will be able to switch back to its saved 
//	 context (which is important because the utility handles 
//	 multiple transactions), and also abort the current transaction
//	 in the case of failure. Task executors employ the UOFS transaction
//	 manager for supporting the nowaited transaction API.
//
//	 For most kinds of tasks, the major part of task execution 
//	 can be transferred to the task process. In this case, there 
//	 will be two copies of the executor object: one on side of the
//	 the main process (arkcmp), and the other on the side of the
//	 task process. 
//	
//	 Execution handlers on the both process's sides will exchange 
//	 the (partially) serialized contexts of the executor objects. 
//	 The communication is a two-way requester-server IPC. The 
//	 serialization/de-serialization methods are:
//	 StoreRequest()/LoadRequest()/StoreReply()/LoadReply().
//
//--------------------------------------------------------------------------//

class CRUTask;
class CDMPreparedStatement;

class REFRESH_LIB_CLASS CRUTaskExecutor {

public:
	// Common finite automata states
	CRUTaskExecutor(CRUTask *pParentTask = NULL);
	virtual ~CRUTaskExecutor();

	//----------------------------------//
	//	Accessors
	//----------------------------------//
public:
	CRUTask *GetParentTask() const
	{ 
		return pParentTask_; 
	}

	Lng32 GetState() const 
	{ 
		return state_; 
	}
	// Get the process ID that I associate with 
	Lng32 GetProcessId() const 
	{ 
		return processId_; 
	}
	// Do I want to execute at all ? (Be determined in the Init() function)
	BOOL HasWork() const
	{
		return hasWork_;
	}

	// We time the execution duration in seconds
	TInt32 GetTimerDuration() const
	{ 
		RUASSERT(startTimer_ <= endTimer_);
		return (TInt32)((endTimer_ - startTimer_) / 1000000); 
	}

public:
	CUOFsIpcMessageTranslator &GetTranslator()
	{
		RUASSERT (NULL != pIpcTranslator_);
		return *pIpcTranslator_;
	}

	//-- FSM states common for all of the task executors
	enum {

		// Dummy common finite automata states
		EX_START	= 1,
		EX_COMPLETE	= 2,

		// States that must execute locally are 100..199
		MAIN_STATES_START = 100,
		// States that can execute remotely are 200+
		REMOTE_STATES_START = 200
	};

	// Can the next step of the task's execution
	// be executed remotely (in a task process)?
	BOOL IsNextStepRemotelyExecutable() const
	{
		return state_ >= REMOTE_STATES_START;
	}

	BOOL IsInTaskProcess() const
	{
		return NULL == pParentTask_;
	}

	//----------------------------------//
	//	Mutators
	//----------------------------------//
public:
	//-- Execution mechanism - delegated to the derived classes

	// The main FSM switch
	virtual void Work() = 0;
	
	// Must be called before calling to Work()
	// This function must not be overridden and should be only refined 
	virtual void Init() = 0;

public:
	// These functions serialize/de-serialize the executor's context 
	// for the message communication with the remote server process
	
	// Used in the main process side
	inline virtual void StoreRequest(CUOFsIpcMessageTranslator &translator) = 0;
	inline virtual void LoadReply(CUOFsIpcMessageTranslator &translator) = 0;
	
	// Used in the remote process side
	inline virtual void LoadRequest(CUOFsIpcMessageTranslator &translator) = 0;
	inline virtual void StoreReply(CUOFsIpcMessageTranslator &translator) = 0;

public:
	void SetState(Lng32 state) 
	{ 
		state_ = state; 
	}

	// If we want to skip execution this function is called
	void ResetHasWork()
	{
		hasWork_ = FALSE;
		SetState(EX_COMPLETE);
	}

	void SetProcessId(Lng32 pid) 
	{ 
		processId_ = pid; 
	}

	void StartTimer() 
	{ 
		startTimer_ = CRUGlobals::GetCurrentTimestamp(); 
	}
	void EndTimer() 
	{ 
		endTimer_ = CRUGlobals::GetCurrentTimestamp(); 
	}

public:
	// Initial IPC buffer allocation
	void AllocateBuffer();

	// Resize the IPC buffer by the constant factor (if it's not big enough)
	void ReAllocateBuffer(Int32 factor);

	void ExecuteStatement(
			CDMPreparedStatement &stmt,
			Lng32 errorCode,
			const char *errorArgument = NULL,
			BOOL needRowCount = FALSE,
                        BOOL isQuery = FALSE);

public:
	//----------------------------------//
	//	Transactions handling
	//----------------------------------//

	virtual void BeginTransaction();
	virtual void CommitTransaction();
	
	// If the previous step of execution has did not 
	// complete a transaction - switch back to it
	void SwitchTransContextBack(); 

	// A part of error handling.
	// If the executor that failed was in a middle 
	// of a transaction - try to abort this transaction
	// (which might have been already aborted by TMF).
	void RollbackTransaction();

	// Leave the transaction and enter a transaction nil state 
	// (no current transaction)
	void LeaveTransaction();

	// Is there an open transaction 
	BOOL IsTransactionOpen();

	// Get the transaction Id 
	// associated with the next step of the execution
	Lng32 GetTransIdx() const 
	{ 
		return transIdx_; 
	} 

protected:
	// A pure virtual.
	// How much memory does the executor need for request serialization?
	// Each executor will define its own value.
	// If overflow happens, the buffer can be resized (up to the
	// maximum message size).
	virtual Lng32 GetIpcBufferSize() const = 0;

	// ExecuteStatement() callee.
	// Default behavior for dealing with execution error
	virtual void HandleSqlError(CDSException &ex,
								Lng32 errorCode,
								const char *errorArgument = NULL);

	void LoadData(CUOFsIpcMessageTranslator &translator);
	void StoreData(CUOFsIpcMessageTranslator &translator);

private:
	void SetTransIdx(Lng32 transIdx) 
	{ 
		transIdx_ = transIdx; 
	}

	void CreateBufferAndTranslator(Int32 bufsize);

private:
	CRUTask						*pParentTask_;
	BOOL						hasWork_;
	
	// The IPC buffer and its manager
	char						*pIpcBuffer_;
	CUOFsIpcMessageTranslator	*pIpcTranslator_;

	TInt64						startTimer_;
	TInt64						endTimer_;
	
	// Transaction currently associated with the task
	Lng32						transIdx_;		

	// These are the only data members that travles between processes
	// The current execution state
	Lng32						state_;		
	// The process associated with the task
	Lng32						processId_;	
};

void CRUTaskExecutor::StoreRequest(CUOFsIpcMessageTranslator &translator)
{
	StoreData(translator);
}

void CRUTaskExecutor::LoadReply(CUOFsIpcMessageTranslator &translator)
{
	LoadData(translator);
}

// Used in the remote process side
void CRUTaskExecutor::LoadRequest(CUOFsIpcMessageTranslator &translator)
{
	LoadData(translator);
}

void CRUTaskExecutor::StoreReply(CUOFsIpcMessageTranslator &translator)
{
	StoreData(translator);
}

#endif
