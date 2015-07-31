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
#ifndef _RU_DEPENDENCE_GRAPH_H_
#define _RU_DEPENDENCE_GRAPH_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuDependenceGraph.h
* Description:  Definition of class CRUDependenceGraph.
*
* Created:      8/23/1999
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"
#include "dsplatform.h"
#include "dsmap.h"

#include "RuTask.h"

class CDSString;

//--------------------------------------------------------------------------//
// CRUDependenceGraph
// 
//   The run-time representation of the dependence graph.
//   The tasks are nodes in the graph,
//   and the dependencies between them are edges in the graph.
//
//   CRUDependenceGraph handles a collection of CRUTask objects.
//   Every CRUTask is responsible for handling its connectivity.
//	 CRUDependenceGraph is responsible for deleting the task
//	 objects when the execution is over.
//	 
//	 The user can insert tasks into the graph. A task is called 
//	 *available* if it was inserted into the graph, and can be 
//	 scheduled for execution when ready. A task that completes 
//	 execution or is *doomed* before it - stops being available. 
//	 Newly created tasks are automatically available.
//
//	 A task completion is followed by the *reduction* of the 
//	 dependence graph (i.e., the task is deleted from the graph
//	 together with its adjacent edges. 
//
//	The class also implements the Refresh utility's scheduling 
//	algorithm. The algorithm chooses the next task to execute,
//  guided by the task priority and task cost considerations.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUDependenceGraph {

public:
 	CRUDependenceGraph(TInt32 maxPipelining);
	virtual ~CRUDependenceGraph();
	
	//----------------------------------------------//
	// Accessors
	//----------------------------------------------//
public:
	// Access task by Id
	CRUTask *GetTask(Lng32 taskId);

	// Access task by UID of associated object and type
	CRUTask *GetTask(TInt64 objUid, CRUTask::Type type);

	// Is the dependence graph empty?
	BOOL AreAllTasksComplete() const
	{
		return (TRUE == availableTaskList_.IsEmpty());
	}

        // are all empcheck tasks complete.  this must happen
        // before MaximizePipeliningPotential 
        BOOL AllEmpCheckTasksComplete() const;

public:
	// Get the last scheduled task
	CRUTask *GetScheduledTask() const 
	{ 
		return pScheduledTask_; 
	}

#ifdef _DEBUG
public:
	void Dump(CDSString &to);

	// Print the graph in the Dotty format
	void DumpGraph(CDSString &to);
#endif

	//----------------------------------------------//
	// Mutators
	//----------------------------------------------//
public:
	void InsertTask(CRUTask *pTask);

	//-- Disconnect the task from its environment
	void Reduce(CRUTask *pTask);

public:
	//-- Elect (schedule) the next task to be executed --//
	void Schedule();

private:
	//-- Prevent copying
	CRUDependenceGraph(const CRUDependenceGraph& other);
	CRUDependenceGraph& operator= (const CRUDependenceGraph& other);

private:
	friend class CRUDependenceGraphIterator;

private:
	void RemoveFromAvailableList(CRUTask *pTask);

	//-- Schedule() callees
	void ScheduleDoomedTask();
	void ScheduleNormalTask();

	// Before making the scheduling decision, 
	// Apply the heuristic to compute the gain function.
	void ComputeGainFunction();

private:
	// Consider a single candidate for scheduling.
	void ConsiderTaskForScheduling(CRUTask *pTask);

	// This is a very sensitive enum, DO NOT TOUCH UNNECESSARILY !
	// Different task types have different priorities for scheduling.
	// Between the types with the same priority, the gain decides.

	enum TypePriority
	{
		PR_TABLE_SYNC		= 4,
		PR_RC_RELEASE		= 3,

		PR_EMP_CHECK		= 2,
		
		PR_REFRESH			= 1,
		PR_DUP_ELIM			= 1,
		PR_LOCK_EQUIV_SET	= 1,
		
		PR_LOG_CLEANUP		= 0
	};

	// The mapping between the task type and task priority.
	// See the explanations for the priority allocation in the .cpp file!
	static TypePriority GetTypePriority(CRUTask *pTask);
	
private:
	// Schedule() callee
	// Once the next task is chosen, merge it with the 
	// maximum possible # of successors to perform pipelining.
	void MaximizePipeliningPotential();

	void BuildPipelinedTasksList(CRUTaskList &pplTaskList);
	void MergePipelinedTasks(const CRUTaskList &pplTaskList);

	void InheritTopTaskConnectivity(CRUTask *pNewTopTask);

private:
	//----------------------------------------------//
	// Core data members
	//----------------------------------------------//

	CRUTaskList allTaskList_;

	// List of available tasks (i.e., those that can be executed).
	// The scheduler will only consider these tasks.
	CRUTaskList	availableTaskList_;

	CRUTask *pScheduledTask_;
	TInt32 maxPipelining_;
};

#endif
