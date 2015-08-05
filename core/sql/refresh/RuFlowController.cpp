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
* File:         RuFlowController.cpp
* Description:  Implementation of class CRUFlowController
*				
*
* Created:      05/07/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuFlowController.h"

#include "RuGlobals.h"
#include "RuJournal.h"
#include "RuDependenceGraph.h"
#include "RuRefreshTask.h"

//--------------------------------------------------------------------------//
//	Constructor
//--------------------------------------------------------------------------//

CRUFlowController::
CRUFlowController(CRUDependenceGraph &dg, 
				  TInt32 maxParallelism) :
	inherited(),
	dg_(dg),
	maxParallelism_(maxParallelism),
	nRunningTasks_(0),
	didTaskFailuresHappen_(FALSE)
{}

//--------------------------------------------------------------------------//
//	CRUFlowController::HandleRequest()
//
//	The main request switch
//--------------------------------------------------------------------------//

void CRUFlowController::HandleRequest(CRURuntimeControllerRqst *pRqst)
{
	switch (pRqst->GetType())
	{
	case CRURuntimeControllerRqst::SCHEDULE:
		{
			HandleScheduleRqst();
			break;
		}
	case CRURuntimeControllerRqst::FINISH_TASK:
		{
			HandleFinishTaskRqst(pRqst);
			break;
		}
		
	default: RUASSERT(FALSE);	// illegal request
	}
}

//--------------------------------------------------------------------------//
//	TASK SCHEDULING
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUFlowController::HandleScheduleRqst()
//
//	 Process the SCHEDULE request.
//
//	 The first request is originated by the utility, 
//	 in order to ignite the execution. 
//
//	 The following ones can be initiated either by the flow  
//	 controller itself (upon task completion), or by the peer  
//	 controller (when there is room for additional parallelism).
//
//--------------------------------------------------------------------------//

void CRUFlowController::HandleScheduleRqst()
{
	CRUTask *pScheduledTask = NULL;

	RUASSERT(nRunningTasks_ <= maxParallelism_);

	if (nRunningTasks_ != maxParallelism_)
	{
		dg_.Schedule();		// Find a ready task to run

		pScheduledTask = dg_.GetScheduledTask();
		if (NULL != pScheduledTask)
		{
			// Initiate the scheduled task's execution ...
			nRunningTasks_++;
			RouteScheduledTask(pScheduledTask);
			return;
		}
	}

	// Cannot schedule a new task (for whatever reason).
	// If some tasks are still being executed, await their completion.
	if (nRunningTasks_ > 0)
	{
		GetPeerController()->PostRequest(
			new CRURuntimeControllerRqst(CRURuntimeControllerRqst::AWAIT_EVENT)
		);

		return;
	}

	// All the tasks complete. Do not post new requests.
	// Execution over!
	RUASSERT(TRUE == dg_.AreAllTasksComplete());
}

//--------------------------------------------------------------------------//
//	CRUFlowController::RouteScheduledTask()
//
//	If the scheduled task is doomed, jump directly to the completion
//	handler. Otherwise, submit it to the peer controller for execution.
//
//--------------------------------------------------------------------------//

void CRUFlowController::RouteScheduledTask(CRUTask *pScheduledTask)
{
	CRURuntimeControllerRqst *pRqst = NULL;

	if (0 != pScheduledTask->GetStatus())
	{
		// Something is wrong with this task, don't pass it to the peer
		pRqst = new CRURuntimeControllerRqst(
			CRURuntimeControllerRqst::FINISH_TASK,
			pScheduledTask
		);

		this->PostRequest(pRqst);
	}
	else
	{
		// Post the new request for task execution to the peer
		pRqst = new CRURuntimeControllerRqst(
			CRURuntimeControllerRqst::START_TASK,
			pScheduledTask
		);

		GetPeerController()->PostRequest(pRqst);
	}
}

//--------------------------------------------------------------------------//
//	TASK COMPLETION
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUFlowController::HandleFinishTaskRqst()
//
//	 Process the FINISH_TASK request.
//
//	 This request is generated by the peer controller upon task completion, 
//	 or by myself if a doomed task is scheduled.
//
//--------------------------------------------------------------------------//

void CRUFlowController::HandleFinishTaskRqst(CRURuntimeControllerRqst *pRqst)
{
	nRunningTasks_--;

	CRUTask &task = pRqst->GetTask();

	if (0 != task.GetStatus())
	{
		if (TRUE == NeedToReportTaskFailure(task))
		{
			// Raise the flag only if some failures were reported.
			didTaskFailuresHappen_ = TRUE;

			CRUJournal &journal = CRUGlobals::GetInstance()->GetJournal();
			journal.LogError(task.GetErrorDesc());
		}

		// Propagate the error to the neighbors
		NotifyTaskEnvironmentOnFailure(task);
	}

	// Remove the task from the graph, together with the adjacent edges
	dg_.Reduce(&task);

	// Initiate the next scheduling event to keep things going
	this->PostRequest(
		new CRURuntimeControllerRqst(CRURuntimeControllerRqst::SCHEDULE)
	);
}

//--------------------------------------------------------------------------//
//	CRUFlowController::NotifyTaskEnvironmentOnFailure()
//
//	When the task completes the execution with failure
//	(for whatever reason), it must notify all its successors
//	and predecessors about this fact.
//
//--------------------------------------------------------------------------//

void CRUFlowController::NotifyTaskEnvironmentOnFailure(CRUTask &task)
{
	RUASSERT (0 != task.GetStatus());

	DSListPosition pos;

	CRUTaskList &succList = task.GetTasksThatDependOnMe();
	CRUTaskList &predList = task.GetTasksThatIDependOn();

	// Notify the successors 
	pos = succList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUTask *pSuccTask = succList.GetNext(pos);
		
		// No task can complete while the successor is being 
		RUASSERT(FALSE == pSuccTask->IsRunning());

		pSuccTask->HandlePredecessorFailure(task);
	}

	// Notify the predecessors, if any
	pos = predList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUTask *pPredTask = predList.GetNext(pos);

		if (FALSE == pPredTask->IsRunning())
		{
			// Backward error propagation. 
			// The predecessor may be running 
			// (I received the error from the other predecessor)
			pPredTask->HandleSuccessorFailure(task);
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUFlowController::NeedToReportTaskFailure()
//	
//	Report about two kinds of failed tasks:
//	(1) Refresh tasks (which contain the failed MVs' names).
//	(2) Other tasks that have caused errors by themselves (i.e., do not
//	    relay the other tasks' errors).
//
//--------------------------------------------------------------------------//

BOOL CRUFlowController::NeedToReportTaskFailure(CRUTask &task)
{
	Lng32 status = task.GetStatus();
	RUASSERT(0 != status);

	if (CRUTask::REFRESH == task.GetType())
	{
		return TRUE;
	}

	status = -status;	// SQL errors are negative

	if (IDS_RU_TASK_PREDECESSOR_PROBLEM == status
		||
		IDS_RU_OBSOLETE_PROBLEM == status)
	{
		return FALSE;
	}

	return TRUE;
}
