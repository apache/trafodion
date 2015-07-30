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
#ifndef _RU_TASK_H_
#define _RU_TASK_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuTask.h
* Description:  Definition of class CRUTask.
*
* Created:      8/23/1999
* Language:     C++
*
*
******************************************************************************
*/

//--------------------------------------------------------------------------//
// CRUTask
//
//   A generic runtime task abstract class.
//  
//	 This class supports connectivity in the dependence graph. 
//	 The CRUTask object maintains a list of tasks that depend on this
//	 task (pSuccList_) and a list of tasks that this task depends on 
//   (pPredList_), and provides the API to add and remove dependencies 
//   (edges in the graph).
// 
//	 The meaning of an edge A-->B is that task B cannot be executed
//	 before task A.
//
//   CRUTask does not support task execution directly. Rather, 
//   a lightweight task executor (CRUTaskExecutor object) is built. 
//	 The executor will carry the concrete task's context, and can be 
//	 executed  either in the main (arkcmp) process, or in a task process.
//
//   Every class derived from CRUTask will be associated with one or more
//	 objects (which inherit from CRUObject). If any errors happen about the 
//	 task's execution, the error message will be stored in the task object.
//
//	 Tasks can compute their own *costs*, which affect the utility scheduler's
//	 decisions.
//
//	 The task object will hold the process id of the task process 
//	 (in the UOFS process pool) that executes the task.
//
//--------------------------------------------------------------------------//

#include "refresh.h"
#include "dsptrlist.h"

#include "RuException.h"

class CRUTaskExecutor;
class CRUTaskList;

class REFRESH_LIB_CLASS CRUTask {

	//---------------------------------------//
	//	PUBLIC AREA
	//---------------------------------------//
public: 

	enum  Type {

		REFRESH			= 0,
		TABLE_SYNC		= 1,
		DUP_ELIM		= 2,
		LOG_CLEANUP		= 3,
		RC_RELEASE		= 4,
		EMP_CHECK		= 5,
		LOCK_EQUIV_SET	= 6
	};

	// Dummy cost value, if want to set a task's cost to artificially high
	enum 
	{
		MAX_COST = 65536	
	};

public:
	CRUTask(Lng32 taskId);
	virtual ~CRUTask();

	//-----------------------------------//
	// Accessors
	//-----------------------------------//
public:
	// Unique task identifier
	Lng32 GetId() const 
	{ 
		return taskId_; 
	}

	// To be applied after building the executor.
	CRUTaskExecutor &GetExecutor() const
	{
		RUASSERT (NULL != pExecutor_);
		return *pExecutor_;
	}

	BOOL IsRunning() const 
	{ 
		return isRunning_; 
	}
	
	// Did the executor run
	BOOL WasExecuted() const 
	{
		return wasExecuted_;
	}

	// NOT TO CONFUSE WITH COST!
	// The gain is function of the cost of both
	// the task itself and the tasks that depend on it.
	TInt32 GetGain() const 
	{ 
		return gain_; 
	}

public:
	// Framework for error reporting mechanism
	CRUException &GetErrorDesc()
	{ 
		return errorDesc_;
	} 

	Lng32 GetStatus() 
	{ 
		return GetErrorDesc().GetStatus(); 
	}

public:
	//---- Connectivity queries ----//

	// What are the tasks that I depend on?
	CRUTaskList& GetTasksThatIDependOn() const 
	{ 
		return *pPredList_; 
	}

	// What are the tasks that depend on me?
	CRUTaskList& GetTasksThatDependOnMe() const
	{ 
		return *pSuccList_; 
	} 

	// Are there no tasks that I depend on?
	BOOL IsReady() const;

public:
	//-- Pure virtuals - delegated to derived classes

	virtual Type GetType() const = 0;
	
	virtual CDSString GetTaskName() const = 0;

	// Does the task operate on an object with a given UID?
	virtual BOOL HasObject(TInt64 uid) const = 0;

#ifdef _DEBUG
public:
	void Dump(CDSString &to);
	void DumpGraphNode(CDSString &to);
	void DumpGraphEdges(CDSString &to);
#endif

	//-----------------------------------//
	// Mutators
	//-----------------------------------//
public:
	void SetRunning(BOOL val) 
	{ 
		isRunning_ = val; 
	}

	void SetExecuted(BOOL val) 
	{ 
		wasExecuted_ = val; 
	}

	// Build the executor object.
	void BuildExecutor();

	// After the execution, the executor can be released
	void DeleteExecutor();

	//-- Heuristics computation
	// The gain computation heuristic is the same for every type
	void ComputeGain();

public:
	//---- Connectivity updates ----//
	
	void AddTaskThatIDependOn(CRUTask *pTask);
	void RemoveTaskThatIDependOn(CRUTask *pTask);

	void AddTaskThatDependsOnMe(CRUTask *pTask);
	void RemoveTaskThatDependsOnMe(CRUTask *pTask);

	void RemoveAllTasksThatDependOnMe();
	void RemoveAllTasksThatIDependOn();

public:
	//-- Predecessor/Successor task failure handlers
	// BOTH ARE VIRTUAL AND CAN BE OVERRIDDEN

	// Default behavior when the predecessor fails - 
	// propagate the error to myself
	virtual void HandlePredecessorFailure(CRUTask &task);

	// Default behavior when the successor fails - nothing
	virtual void HandleSuccessorFailure(CRUTask &task) {}

	//---------------------------------------//
	//	PROTECTED AND PRIVATE AREA
	//---------------------------------------//

protected:
	// The task's cost. If the task is not immediate,
	// this is the estimate of its running time.

	virtual TInt32 GetComputedCost() const = 0;

	// Is this a very short task 
	// (to be executed in the arkcmp process) ?
	virtual BOOL IsImmediate() const = 0;

protected:
	// BuildExecutor() callee
	// Create the concrete executor's object.
	// Delegated to the child classes
	virtual CRUTaskExecutor *CreateExecutorInstance() = 0;

	// DeleteExecutor() callee
	// Before the executor is deleted - pull from it whatever is required.
	// The default behavior is DO NOTHING - can be overridden.
	virtual void PullDataFromExecutor() {}

private:
	// Generic cost computation mechanism
	void ComputeCost();

	TInt32 GetCost() const 
	{ 
		return cost_; 
	}

	//---------------------------------------//
	//	Core data members
	//---------------------------------------//
private:
	Lng32 taskId_;

	CRUException errorDesc_;

	// Connectivity lists
	CRUTaskList *pPredList_;
	CRUTaskList *pSuccList_;

	// The task's executor
	CRUTaskExecutor *pExecutor_;

	BOOL isRunning_;
	BOOL wasExecuted_;

	TInt32 cost_;
	TInt32 gain_;

	// Process id (in the process pool) associated with the task
	short  pid_;
};

//--------------------------------------------------------------------------//
// CRUTaskList
//
//	 List of pointers to task objects.
//	 Supports searching by task Id and smart update.
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUTaskList : public CDSPtrList<CRUTask> {
	
private:
	typedef CDSPtrList<CRUTask> inherited;

public:
	CRUTaskList(EOwnership ownership = eItemsAreOwned)
		: inherited(ownership) {} 
	~CRUTaskList() {}

	//-----------------------------------//
	// Accessors
	//-----------------------------------//
public:
	CRUTask *FindTask(Lng32 taskId);
	DSListPosition FindTaskPos(Lng32 taskId);

	//-----------------------------------//
	// Mutators
	//-----------------------------------//
public:
	void AddRefToTask(CRUTask *pTask);
	void RemoveRefToTask(CRUTask *pTask);
};

//--------------------------------------------------------------------------//
//	The inliners
//--------------------------------------------------------------------------//

inline BOOL CRUTask::IsReady() const
{
	return (0 == GetTasksThatIDependOn().GetCount());
}

#endif
