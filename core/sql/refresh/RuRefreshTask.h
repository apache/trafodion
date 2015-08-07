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
#ifndef _RU_REFRESH_TASK_H_
#define _RU_REFRESH_TASK_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuRefreshTask.h
* Description:  Definition of class	CRURefreshTask
*
* Created:      8/23/1999
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"

#include "RuMV.h"

#include "RuTask.h"
#include "RuTaskExecutor.h"

class CRUTbl;
struct CRUDeltaDef;

class CRURefreshTaskExecutor;

// ----------------------------------------------------------------- //
// CRURefreshTask
//
//	 The Refresh task class. Supports incremental refresh of a  
//	 single or multiple MVs (with delta pipelining), or re-computation
//	 of a single MV.
// 
//	 Initially, a Refresh task is intended to refresh a single MV. 
//	 However, it can be merged with one or more Refresh tasks, if the 
//	 scheduler will decide to employ delta pipelining. If two tasks are
//   merged, the connectivity between them is merged too, self loops
//	 eliminated.
//
//	 The execution of a Refresh task is based on applying the
//   INTERNAL REFRESH command. The task object will generate the SQL,
//	 but will not run the task. Instead, it will clone an executor
//	 that will be able to execute the task either locally or remotely.
//
// ----------------------------------------------------------------- //

class REFRESH_LIB_CLASS CRURefreshTask : public CRUTask {

private:
	typedef CRUTask inherited;

	//---------------------------------------//
	//	PUBLIC AREA
	//---------------------------------------//
public:
	CRURefreshTask(Lng32 id, CRUMV &mv);
	virtual ~CRURefreshTask();

	//-----------------------------------//
	// Accessors
	//-----------------------------------//
public:
	// The "original" MV that was associated with the task
	// (if pipelining is used, it streams the delta to the other MVs).
	CRUMV &GetRootMV() const
	{
		return rootMV_;
	}

	// The Refresh task can handle more than a single MV,
	// if delta pipelining is applied.
	CRUMVList &GetMVList() 
	{ 
		return mvList_; 
	}

	// The topmost MV in the pipelining chain
	// (the original MV if there is no pipelining).
	CRUMV &GetTopMV() const;

	CRUDeltaDefList &GetDeltaDefList() const
	{ 
		return GetRootMV().GetDeltaDefList();
	}

	BOOL IsSingleDeltaRefresh() const
	{
		return GetRootMV().IsSingleDeltaRefresh();
	}

public:
	//-- Implementation of pure virtual functions
	virtual CRUTask::Type GetType() const 
	{ 
		return CRUTask::REFRESH; 
	}

	virtual BOOL HasObject(TInt64 uid) const;
	
public:
	// Do we need to execute Internal Refresh statement?
	BOOL NeedToExecuteInternalRefresh() const;

	// Will the MV be refreshed by recompute?
	BOOL IsRecompute() const 
	{ 
		return GetRootMV().WillBeRecomputed();
	}

public:
	void DumpNamesOfAllMVs(CDSString& to) const;

	//---------------------------------------//
	//	PROTECTED AND PRIVATE AREA
	//---------------------------------------//
protected:
	//-- Implementation of pure virtuals

	virtual CDSString GetTaskName() const;

	virtual TInt32 GetComputedCost() const;

	// Is this a very short task 
	// (to be executed in the arkcmp process) ?
	virtual BOOL IsImmediate() const
	{
		return (FALSE == NeedToExecuteInternalRefresh());
	}

	// Create the concrete task executor
	virtual CRUTaskExecutor *CreateExecutorInstance();

	// Refinement of the parent's method
	virtual void PullDataFromExecutor();

private:
	//-- Prevent copying
	CRURefreshTask(const CRURefreshTask& other);
	CRURefreshTask& operator= (const CRURefreshTask& other);

private:
	// PullDataFromExecutor() callees
	void PullDataFromExecutorAfterIncRefresh(CRURefreshTaskExecutor &taskEx);
	void PullDataFromExecutorAfterRecompute(CRURefreshTaskExecutor &taskEx);

	// ComputeCost() callee.
	// A heuristic to compute the cost if there is a single delta.
	TInt32 GetSingleDeltaCost() const;


	// CreateExecutorInstance() and PullDataFromExecutor() callee
	// Set the DDL locks release flags in the root MV/all MVs.
	void SetReleaseDDLLocks(BOOL flag, BOOL isRootMVOnly);

private:
	//-----------------------------------//
	// Core data members
	//-----------------------------------//
	
	CRUMV &rootMV_;
	CRUMVList mvList_;
};

#endif
