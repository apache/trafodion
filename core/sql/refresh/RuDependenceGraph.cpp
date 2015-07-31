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
/* -*-C++-*-
******************************************************************************
*
* File:         RuDependenceGraph.cpp
* Description:  Implementation of class CRUDependenceGraph.
*				
* Created:      12/29/1999
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuDependenceGraph.h"
#include "RuDgIterator.h"

#include "RuRefreshTask.h"

//--------------------------------------------------------------------------//
//	Constructor and destructor
//--------------------------------------------------------------------------//

CRUDependenceGraph::CRUDependenceGraph(TInt32 maxPipelining) : 
	allTaskList_(eItemsAreOwned),
	availableTaskList_(eItemsArentOwned),
	maxPipelining_(maxPipelining)
{}

CRUDependenceGraph::~CRUDependenceGraph() 
{
	// Remove pointers to tasks
	availableTaskList_.RemoveAll();

	// Remove the task objects themselves
	allTaskList_.RemoveAll();
}

//--------------------------------------------------------------------------//
//	ACCESSORS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDependenceGraph::GetTask()
//
//	Lookup by task ID
//--------------------------------------------------------------------------//

CRUTask *CRUDependenceGraph::GetTask(Lng32 taskId) 
{
	return availableTaskList_.FindTask(taskId);
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraph::GetTask()
//
//	Lookup by object UID and type
//--------------------------------------------------------------------------//

CRUTask *CRUDependenceGraph::GetTask(TInt64 objUid, CRUTask::Type type)
{
	DSListPosition pos = availableTaskList_.GetHeadPosition();

	while (NULL != pos) 
	{
		CRUTask *pTask = availableTaskList_.GetNext(pos);
	
		if (TRUE == pTask->HasObject(objUid) 
			&&
			type == pTask->GetType()
			)
		{
			return pTask;
		}
	}

	return NULL;
}

#ifdef _DEBUG
//--------------------------------------------------------------------------//
//	CRUDependenceGraph::Dump()
//--------------------------------------------------------------------------//

void CRUDependenceGraph::Dump(CDSString &to)
{
	to += "\n\t\tDEPENDENCE GRAPH DUMP\n";

	CRUTask *pTask;
	CRUTopologicalDGIterator it(*this, CRUTopologicalDGIterator::DIRECT);

	for (; (pTask = it.GetCurrentTask()) != NULL; it.Next())
	{
		pTask->Dump(to);
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraph::DumpGraph()
//
//	Dump the graph in the DOTTY format (for the further viewing 
//	through the GUI).
//
//--------------------------------------------------------------------------//

void CRUDependenceGraph::DumpGraph(CDSString &to)
{
	to += "\n\t\tDEPENDENCE GRAPH DRAWING START\n";

	to += "digraph G { \n\n";

	to += " graph [	fontname = \"Helvetica-Oblique\",page = \"8,10\", margin = \"1.5,1.5\"];\n\n";
	to += "node [style=filled,fontsize=14];\n\n";

	CRUTask *pTask;
	CRUTopologicalDGIterator firstPass(*this, CRUTopologicalDGIterator::DIRECT);

	for (; (pTask = firstPass.GetCurrentTask()) != NULL; firstPass.Next())
	{
		pTask->DumpGraphNode(to);
	}
	
	CRUTopologicalDGIterator secondPass(*this, CRUTopologicalDGIterator::DIRECT);

	for (; (pTask = secondPass.GetCurrentTask()) != NULL; secondPass.Next())
	{
		pTask->DumpGraphEdges(to);
	}

	to += "\n}\n\n\t\tDEPENDENCE GRAPH DRAWING END\n";
}
#endif

//--------------------------------------------------------------------------//
//	MUTATORS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDependenceGraph::InsertTask()
//--------------------------------------------------------------------------//

void CRUDependenceGraph::InsertTask(CRUTask *pTask)
{
	RUASSERT (NULL != pTask);

	allTaskList_.AddTail(pTask);
	availableTaskList_.AddTail(pTask);
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraph::Reduce()
//	
//	Reduce the graph after a task completion:
//	(1) Remove the task from the available list.
//	(2) Disconnect it from its successors.
//	(3) If there are available tasks that the executed task 
//	    depended on (i.e., the task was doomed), disconnect 
//		them too.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraph::Reduce(CRUTask *pTask)
{
	RemoveFromAvailableList(pTask);
	
	pTask->RemoveAllTasksThatDependOnMe();

	if (FALSE == pTask->IsReady())
	{
		RUASSERT(0 != pTask->GetStatus());
		pTask->RemoveAllTasksThatIDependOn();
	}
}

//--------------------------------------------------------------------------//
// CRUDependenceGraph::Schedule()
//
//	Select the next ready task to execute. The scheduling algorithm 
//	is as follows.
//	(1) First, try to schedule a task with error (a doomed task).
//	    NOTE THAT A DOOMED TASK IS NOT NECESSARILY READY (i.e.,
//	    there may be tasks that precede it in the graph).
//
//	(2) If no one found, schedule a READY "normal" task in accordance
//		with the priority scale described in ConsiderTaskForScheduling().
//	    The scheduling pass will be preceded by the gain function 
//	    computation pass.
//
//	(3) If the scheduled task is a Refresh task, try to merge it 
//	    with as many succeeding Refresh tasks as possible,  
//	    in order to maximize the pipelining potential.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraph::Schedule()
{
	pScheduledTask_ = NULL;

	if (TRUE == AreAllTasksComplete())
	{
		return;
	}

	ScheduleDoomedTask();
	if (NULL != pScheduledTask_)
	{
		return;		// A doomed task found
	}

	ScheduleNormalTask();
	if (NULL == pScheduledTask_)
	{
		return;	// All the ready tasks are being executed currently
	}

	if (CRUTask::REFRESH == pScheduledTask_->GetType())
	{
		MaximizePipeliningPotential();
	}
}

//--------------------------------------------------------------------------//
//	PRIVATE AREA
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDependenceGraph::RemoveFromAvailableList()
//--------------------------------------------------------------------------//

void CRUDependenceGraph::RemoveFromAvailableList(CRUTask *pTask)
{
	DSListPosition pos = 
		availableTaskList_.FindTaskPos(pTask->GetId());
	RUASSERT (NULL != pos);

	availableTaskList_.RemoveAt(pos);
}

//--------------------------------------------------------------------------//
//	SCHEDULING
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDependenceGraph::ScheduleDoomedTask()
//
//	Try to schedule some task with error (top priority).
//--------------------------------------------------------------------------//

void CRUDependenceGraph::ScheduleDoomedTask()
{
	CRUTask *pTask;
	CRUTopologicalDGIterator it(*this, CRUTopologicalDGIterator::DIRECT);

	for (; (pTask = it.GetCurrentTask()) != NULL; it.Next())
	{
		if (0 != pTask->GetStatus())
		{
			pScheduledTask_ = pTask;
			return;
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraph::ScheduleNormalTask()
//
//	(1) Compute the gain function's value for every task in the graph.
//	(2) Traverse the graph and select the best candidate for execution.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraph::ScheduleNormalTask()
{
	ComputeGainFunction();

	CRUTopologicalDGIterator it(*this, CRUTopologicalDGIterator::DIRECT);
	CRUTask *pTask;

	for (; (pTask = it.GetCurrentTask()) != NULL; it.Next())
	{
		RUASSERT (0 == pTask->GetStatus());

		// Consider only ready tasks that are not being executed already
		if (FALSE == pTask->IsReady()
			||
			TRUE == pTask->IsRunning())
		{
			continue;
		}
		
		// Examine the task's candidateship ...
		ConsiderTaskForScheduling(pTask);
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraph::ConsiderTaskForScheduling()
//	  
//	 Consider a single candidate for scheduling. 
//	 Apply the following scale of criteria:	 		
//	 (1) First, try to schedule the task of the higher priority class.
//	 (2) Between two tasks of the same priority class, choose the one 
//	     with the greater gain function.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraph::ConsiderTaskForScheduling(CRUTask *pTask)
{

        // don't schedule refresh until all emptiness check tasks are complete
        // in order to maximize pipelining potential
        if( CRUTask::REFRESH == pTask->GetType() 
            &&
            FALSE == AllEmpCheckTasksComplete() )
        {
                return;
        }

	// The first scanned task 
	if (NULL == pScheduledTask_)
	{
		pScheduledTask_ = pTask;
		return;
	}

	TypePriority oldPrio = GetTypePriority(pScheduledTask_);
	TypePriority newPrio = GetTypePriority(pTask);

	if (newPrio < oldPrio)
	{
		return;
	}

	if (newPrio > oldPrio)
	{
		pScheduledTask_ = pTask;
		return;
	}

	// newPrio == oldPrio. The gain will decide
	if (pTask->GetGain() > pScheduledTask_->GetGain())
	{
		pScheduledTask_ = pTask;
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraph::GetTypePriority()
//
//	The mapping between the task's type and the type's priority.
//
//	--------------+----------+---------------------------------------------
//	TASK          | PRIORITY | REASON
//	TYPE          | CLASS    |
//	--------------+----------+---------------------------------------------
//	TABLE_SYNC    |  4       | Releases the log-lock (effectively, table-lock).
//	              |          | For NO LOCKONREFRESH tables, this lock must be
//	              |          | released as soon as possible.
//	--------------+----------+---------------------------------------------
//	RC_RELEASE    |  3       | Releases the resources (DDL locks and RP opens
//	              |          | of locked tables).Once there is no reason 
//	              |          | to hold them - the resources must be released
//	              |          | ASAP.
//	--------------+----------+---------------------------------------------
//	EMP_CHECK	  |	 2       | The emptiness check's results can improve 
//                |          | the delta pipelining decisions' quality.
//	--------------+----------+---------------------------------------------
//	REFRESH       |  1       | Regular priority
//	DUP_ELIM      |  1       |
//	LOCK_EQUIV_SET|  1       |
//	--------------+----------+---------------------------------------------
//	LOG_CLEANUP   |  0       | Lowest priority
//	--------------+----------+---------------------------------------------
//
//--------------------------------------------------------------------------//

CRUDependenceGraph::TypePriority 
CRUDependenceGraph::GetTypePriority(CRUTask *pTask)
{
	CRUDependenceGraph::TypePriority prio = PR_LOG_CLEANUP;

	switch (pTask->GetType())
	{
	case CRUTask::TABLE_SYNC: 
		{
			prio = PR_TABLE_SYNC;	// 4
			break;
		}
	case CRUTask::RC_RELEASE: 
		{
			prio = PR_RC_RELEASE;	// 3
			break;
		}
	case CRUTask::EMP_CHECK:	
		{
			prio = PR_EMP_CHECK;	// 2
			break;
		}
	case CRUTask::REFRESH: 
		{
			prio = PR_REFRESH;		// 1
			break;
		}
	case CRUTask::DUP_ELIM: 
		{
			prio = PR_DUP_ELIM;		// 1
			break;
		}
	case CRUTask::LOCK_EQUIV_SET: 
		{
			prio = PR_LOCK_EQUIV_SET; // 1
			break;
		}	
	case CRUTask::LOG_CLEANUP: 
		{
			prio = PR_LOG_CLEANUP;	// 0
			break;
		}
	default: RUASSERT(FALSE);
	}

	return prio;
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraph::ComputeGainFunction
//
//   Compute the gain function for every task the graph.
//	 The gain function of a task is the total cost of the 
//   heaviest path in the graph that starts from this task.
//
//   The efficient computation of this function requires 
//	 iterating the graph in the *reverse* order.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraph::ComputeGainFunction()
{
	CRUTask *pTask;

	CRUTopologicalDGIterator it(*this, CRUTopologicalDGIterator::REVERSE);
	for (; (pTask = it.GetCurrentTask()) != NULL; it.Next())
	{
		pTask->ComputeGain();
	}
}

//--------------------------------------------------------------------------//
//	PIPELINING
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDependenceGraph::MaximizePipeliningPotential()
//
//	Once the next task to execute is chosen, if this is a
//	Refresh task, the scheduler will attempt to merge it
//	with the maximum possible number of the Refresh tasks 
//	that depend on it, in order to perform delta pipelining
//	and skip excessive write to the intermediate MVs' logs.
//
//	The pipelining decision is limited by the user (through 
//	a system default), by the examined MVs' queries and the
//	relationship between these MVs. 
//
//	The algorithm will proceed in two steps:
//	(1) Build the list of Refresh tasks to pipeline the delta to.
//	(2) Collapse the scheduled tasks and the tasks in the list
//	    into a single "fat" Refresh task, while changing the 
//		graph's connectivity.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraph::MaximizePipeliningPotential()
{
	RUASSERT(CRUTask::REFRESH == pScheduledTask_->GetType());
	CRURefreshTask *pRootTask = (CRURefreshTask *)pScheduledTask_;
	
	CRUTaskList pplTaskList(eItemsArentOwned);
	
	BuildPipelinedTasksList(pplTaskList);

	MergePipelinedTasks(pplTaskList);
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraph::BuildPipelinedTasksList()
//
//	Build the list of Refresh tasks that follow the scheduled
//	task, that can be further collapsed into a single task.
//	For each candidate, check whether the delta can be pipelined
//	from/to it, and whether the user-given limit for pipelining
//	is not exhausted.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraph::BuildPipelinedTasksList(CRUTaskList &pplTaskList)
{
	CRURefreshTask *pTopTask = (CRURefreshTask *)pScheduledTask_;
	CRURefreshTask *pNewTopTask = pTopTask;
	TInt64 nextCandidateUid;

	BOOL isRoot = TRUE;

	for (;;)
	{
		if (pplTaskList.GetCount() + 1 == maxPipelining_)
		{
			// The user-given limit (including the root task) is reached.
			// If the value is 0, the number of tasks is unlimited.
			break;
		}

		CRUMV &topMV = pTopTask->GetRootMV();
		if (FALSE 
			== 
			topMV.CanPipelineFromMe(isRoot, nextCandidateUid/*by ref*/))
		{
			break;
		}
		
		// After the first time, the top task is no more the root
		isRoot = FALSE;	

		pNewTopTask = 
			(CRURefreshTask *)GetTask(nextCandidateUid, CRUTask::REFRESH);

		if (NULL == pNewTopTask)
		{
			break;
		}

		if (FALSE == pNewTopTask->GetRootMV().CanPipelineToMe(topMV))
		{
			break;
		}

		pplTaskList.AddTail(pNewTopTask); // Queue the candidate

		pTopTask = pNewTopTask;	// Move the top task pointer
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraph::MergePipelinedTasks()
//	
//	Collapse the candidates for pipelining into a single task
//	(merge them with the scheduled task), and reorganize the
//	graph's connectivity.
//
//	For each task in the list, the following is performed:
//	(1) Add the task's MV to the scheduled task's MV list.
//	(2) For every task that directly SUCCEEDS it, connect the SCHEDULED 
//	    task to it (i.e., inherit the merged task's connectivity).
//	(3) Drop the merged Refresh tasks from 
//	    the dependence graph's available list.
// 
//	In the next example, suppose that the tasks Refresh(MV1)
//	and Refresh(MV2) are merged.
//
//	BEFORE-STATE:                         
//
//	     +--> TASK1        +--> TASK3
//       |                 |
//     Refresh(MV1) --> Refresh(MV2) --> Refresh(MV3)   
//	     |                 |
//	     +--> TASK2        +--> TASK4
//
//	AFTER-STATE:                         
//
//
//	     +--> TASK3
//	     |
//	     +--> TASK1
//	     |
//   Refresh(MV1,MV2) --> Refresh(MV3)
//	     |
//	     +--> TASK2
//	     |
//	     +--> TASK4
//
//--------------------------------------------------------------------------//

void CRUDependenceGraph::MergePipelinedTasks(const CRUTaskList &pplTaskList)
{
	if (TRUE == pplTaskList.IsEmpty())
	{
		return;
	}

	CRURefreshTask *pTopTask = (CRURefreshTask*) pScheduledTask_;
	CRURefreshTask *pNewTopTask;

	CRUMVList &mvList = pTopTask->GetMVList();

	DSListPosition pos = pplTaskList.GetHeadPosition();
	while (NULL != pos)
	{
		// The next task to merge ...
		pNewTopTask = (CRURefreshTask*)(pplTaskList.GetNext(pos));
		CRUMV &topMV = pNewTopTask->GetRootMV();
		mvList.AddTail(&topMV);

		// Inherit the connectivity of the newly merged task
		InheritTopTaskConnectivity(pNewTopTask);
		RemoveFromAvailableList(pNewTopTask);

		pTopTask = pNewTopTask;
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraph::InheritTopTaskConnectivity()
//
//	Before the next pipelined Refresh task stops being available,
//	inherit its connectivity to the scheduled Refresh task.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraph::InheritTopTaskConnectivity(CRUTask *pNewTopTask)
{
	CRUTaskList &toTaskList = pNewTopTask->GetTasksThatDependOnMe();
	DSListPosition pos = toTaskList.GetHeadPosition();

	pScheduledTask_->RemoveTaskThatDependsOnMe(pNewTopTask);
	while (NULL != pos)
	{
		CRUTask *pTask = toTaskList.GetNext(pos);

		pNewTopTask->RemoveTaskThatDependsOnMe(pTask);
		pScheduledTask_->AddTaskThatDependsOnMe(pTask);
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraph::AllEmpCheckTasksComplete()
//	  
//	 Check all the available tasks for an EmpCheck task
//
//--------------------------------------------------------------------------//

BOOL CRUDependenceGraph::AllEmpCheckTasksComplete() const
{
        DSListPosition pos = availableTaskList_.GetHeadPosition();

	while (NULL != pos) 
	{
		CRUTask *pTask = availableTaskList_.GetNext(pos);

                if( CRUTask::EMP_CHECK == pTask->GetType() )
                {
                  return FALSE;
                }
        }
        return TRUE;
}
