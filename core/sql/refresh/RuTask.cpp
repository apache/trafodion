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
* File:         RuTask.cpp
* Description:  Implementation of classes CRUTask and CRUTaskList
*				
*
* Created:      12/29/1999
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "dsstring.h"

#include "RuTask.h"
#include "RuTaskExecutor.h"

//--------------------------------------------------------------------------//
//	Class CRUTask
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	Constructor and destructor
//--------------------------------------------------------------------------//

CRUTask::CRUTask(Lng32 taskId) :
	taskId_(taskId),
	pid_(-1),
	pExecutor_(NULL),
	pPredList_(
		new CRUTaskList(eItemsArentOwned)
	),
	pSuccList_(
		new CRUTaskList(eItemsArentOwned)
	),
	cost_(0),
	gain_(0),
	isRunning_(FALSE),
	wasExecuted_(FALSE)
{}

CRUTask::~CRUTask()
{
	pPredList_->RemoveAll();
	delete pPredList_;

	pSuccList_->RemoveAll();
	delete pSuccList_;

	delete pExecutor_;		
}

//--------------------------------------------------------------------------//
//	Accessors
//--------------------------------------------------------------------------//

#ifdef _DEBUG

//--------------------------------------------------------------------------//
//	CRUTask::Dump()
//
//	Called by CRUDependenceGraph::Dump()
//
//	Prints the "standard" task's dump
//--------------------------------------------------------------------------//
void CRUTask::Dump(CDSString &to)
{
	char idStr[10];
	sprintf(idStr,"%d",GetId());

	to += "\nTASK ID = " + CDSString(idStr);
	to += "\n\t" + GetTaskName() + "\n";

	if (0 == pSuccList_->GetCount())
	{
		to += "\tNo tasks depend on me.\n";
	}
	else
	{
		to += "\tTasks that depend on me:\n";

		DSListPosition pos = pSuccList_->GetHeadPosition();
		while (NULL != pos)
		{
			CRUTask *pTask = pSuccList_->GetNext(pos);
			sprintf(idStr,"%d",pTask->GetId());
			to += "\t\tTask id = " + CDSString(idStr) + "\n";
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUTask::DumpGraphNode()
//
//	Called by CRUDependenceGraph::DumpGraph()
//
//	Prints the task's node dump acceptable for the Dotty GUI

//--------------------------------------------------------------------------//
void CRUTask::DumpGraphNode(CDSString &to)
{
	char fromChr[10];
	sprintf(fromChr,"%d",GetId());
	CDSString nodeId(fromChr);

	to += "\t\t" + nodeId;
	to += " [style=filled,";
	to += "label= \"" + nodeId + "." + GetTaskName() + "\"";
	to += ",color=";
	switch (GetType())
	{
		case REFRESH:
			{
				to += "red";
				break;
			}
		case TABLE_SYNC:
			{
				to += "lightgrey";
				break;
			}
		case DUP_ELIM:
			{
				to += "violet";
				break;
			}
		case LOG_CLEANUP:
			{
				to += "yellowgreen";
				break;
			}
		case RC_RELEASE:
			{
				to += "pink";
				break;
			}
		case EMP_CHECK:
			{
				to += "gold";
				break;
			}
		case LOCK_EQUIV_SET:
			{
				to += "yellow";
				break;
			}
		default:
			RUASSERT(FALSE);
	}
	to += "];\n";
}

//--------------------------------------------------------------------------//
//	CRUTask::DumpGraphNode()
//
//	Called by CRUDependenceGraph::DumpGraph()
//
//	Prints the task's edges dump acceptable for the Dotty GUI
//
//--------------------------------------------------------------------------//
void CRUTask::DumpGraphEdges(CDSString &to)
{
	if (0 == pSuccList_->GetCount())
	{
		return;
	}

	char fromChr[10],toChr[10];
	sprintf(fromChr,"%d",GetId());
	
	CDSString fromStr(fromChr);
	fromStr = "\t\t" + fromStr + " -> ";

	DSListPosition pos = pSuccList_->GetHeadPosition();
	while (NULL != pos)
	{
		CRUTask *pTask = pSuccList_->GetNext(pos);
		sprintf(toChr,"%d",pTask->GetId());
		to += fromStr + CDSString(toChr) + ";\n";
	}	
}
#endif

//--------------------------------------------------------------------------//
//	MUTATORS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CONNECTIVITY UPDATES
//
//	Dependencies between tasks (edges in the dependence graph) are always 
//	added and removed in pairs. Technically, pPredList_ in one CRUTask object 
//	and pSuccList_ in the other one are updated symmetrically.
//
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUTask::AddTaskThatIDependOn()
//--------------------------------------------------------------------------//

void CRUTask::AddTaskThatIDependOn(CRUTask* pTask)
{
	this->GetTasksThatIDependOn().AddRefToTask(pTask);
	pTask->GetTasksThatDependOnMe().AddRefToTask(this);
}

//--------------------------------------------------------------------------//
//	CRUTask::RemoveTaskThatIDependOn()
//--------------------------------------------------------------------------//

void CRUTask::RemoveTaskThatIDependOn(CRUTask *pTask)
{
	this->GetTasksThatIDependOn().RemoveRefToTask(pTask);
	pTask->GetTasksThatDependOnMe().RemoveRefToTask(this);
}

//--------------------------------------------------------------------------//
//	CRUTask::AddTaskThatDependsOnMe()
//--------------------------------------------------------------------------//

void CRUTask::AddTaskThatDependsOnMe(CRUTask *pTask)
{
	this->GetTasksThatDependOnMe().AddRefToTask(pTask);
	pTask->GetTasksThatIDependOn().AddRefToTask(this);
}

//--------------------------------------------------------------------------//
//	CRUTask::RemoveTaskThatDependsOnMe()
//--------------------------------------------------------------------------//

void CRUTask::RemoveTaskThatDependsOnMe(CRUTask *pTask)
{
	this->GetTasksThatDependOnMe().RemoveRefToTask(pTask);
	pTask->GetTasksThatIDependOn().RemoveRefToTask(this);
}

//--------------------------------------------------------------------------//
//	CRUTask::RemoveAllTasksThatDependOnMe()
//--------------------------------------------------------------------------//

void CRUTask::RemoveAllTasksThatDependOnMe()
{
	DSListPosition pos = GetTasksThatDependOnMe().GetHeadPosition();

	while (NULL != pos)
	{
		CRUTask *pToTask = GetTasksThatDependOnMe().GetNext(pos);
		pToTask->GetTasksThatIDependOn().RemoveRefToTask(this);
	}

	GetTasksThatDependOnMe().RemoveAll();
}

//--------------------------------------------------------------------------//
//	CRUTask::RemoveAllTasksThatIDependOn()
//--------------------------------------------------------------------------//

void CRUTask::RemoveAllTasksThatIDependOn()
{
	DSListPosition pos = GetTasksThatIDependOn().GetHeadPosition();

	while (NULL != pos)
	{
		CRUTask *pFromTask = GetTasksThatIDependOn().GetNext(pos);
		pFromTask->GetTasksThatDependOnMe().RemoveRefToTask(this);
	}

	GetTasksThatIDependOn().RemoveAll();
}

//--------------------------------------------------------------------------//
//	TASK EXECUTOR CREATION AND DELETION
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUTask::BuildExecutor()
//--------------------------------------------------------------------------//

void CRUTask::BuildExecutor()
{
	RUASSERT(NULL == pExecutor_);

	//	Create the executor's instance and initialize it
	pExecutor_ = CreateExecutorInstance();

	pExecutor_->Init();

	SetExecuted(pExecutor_->HasWork());
}

//--------------------------------------------------------------------------//
//	CRUTask::DeleteExecutor()
//--------------------------------------------------------------------------//

void CRUTask::DeleteExecutor()
{
	RUASSERT(NULL != pExecutor_);
	
	if (0 == GetStatus() 
		&& 
		TRUE == WasExecuted())
	{
		// The last chance to pull some data from the executor...
		PullDataFromExecutor();
	}

	delete pExecutor_;
	
	pExecutor_ = NULL;
}

//--------------------------------------------------------------------------//
//	ERROR PROPAGATION MECHANISM
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUTask::HandlePredecessorFailure()
//
//	CRUFlowController::NotifyEnvironmentOnFailure() callee
//
//	Default behavior in case the predecessor fails:
//	propagate the error to myself.
//
//	IMPORTANT! This behavior can be overridden by the child classes.
//
//--------------------------------------------------------------------------//

void CRUTask::HandlePredecessorFailure(CRUTask &task)
{
	RUASSERT (0 != task.GetStatus());

	// If there is already something wrong, don't touch me
	if (0 != this->GetStatus())
	{
		return;
	}

	// The error's text will be printed only for the Refresh tasks.
	CRUException &ex = GetErrorDesc();
	ex.SetError(IDS_RU_TASK_PREDECESSOR_PROBLEM);
	ex.AddArgument(this->GetTaskName());
	ex.AddArgument(task.GetTaskName());
}

//--------------------------------------------------------------------------//
//	COST AND GAIN COMPUTATION
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
// CRUTask::ComputeGain()
//
// CRUFlowController::Schedule() callee
//
// Gain computation method.
//
// The gain of a task is the total cost of the heaviest 
// directed path in the graph that starts from this task.
//
// In the other words, its cost + the maximum gain 
// of a task that depends on it.
//
// The method assumes that it is being applied from the
// scan through the dependence graph in *reverse* order.
// 
//--------------------------------------------------------------------------//

void CRUTask::ComputeGain()
{
	TInt32 gain=0, maxGain=0;

	// First, update the (local) cost estimate
	ComputeCost();

	// Next, update the (global) gain estimate
	DSListPosition pos = pSuccList_->GetHeadPosition();
	
	// Compute the maximum gain between successor tasks
	while (NULL != pos)
	{
		CRUTask *pSuccTask = pSuccList_->GetNext(pos);
		gain = pSuccTask->GetGain();

		maxGain = (gain > maxGain) ? gain : maxGain;
	}

	// Update the gain
	gain_ = GetCost() + maxGain;
}

//--------------------------------------------------------------------------//
//	CRUTask::ComputeCost()
//
//	The generic local cost computation mechanism.
//
//	A ready task which is *immediate* (i.e., is supposed to take 
//	a very short time) will receive the maximum cost, in order to be 
//	executed *first* between the tasks	in the same priority class.
//	
//	The methods IsImmediate() and GetComputedCost()	are pure virtual.
//
//--------------------------------------------------------------------------//

void CRUTask::ComputeCost()
{
	if (TRUE == IsReady() && TRUE == IsImmediate())
	{
		cost_ = CRUTask::MAX_COST;
	}
	else
	{
		// Cost is computed differently for different task types
		cost_ = GetComputedCost(); 
	}
}

//--------------------------------------------------------------------------//
//	Class CRUTaskList 
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUTaskList::FindTaskPos()
//
//	Lookup the list position by task ID
//--------------------------------------------------------------------------//

DSListPosition CRUTaskList::FindTaskPos(Lng32 taskId)
{
	DSListPosition pos = GetHeadPosition();

	while (NULL != pos)
	{
		DSListPosition prevpos = pos;

		CRUTask *pTask = GetNext(pos);
		if (pTask->GetId() == taskId)
		{
			return prevpos;
		}
	}

	return NULL;
}

//--------------------------------------------------------------------------//
//	CRUTaskList::FindTaskPos()
//
//	Lookup the task by ID
//--------------------------------------------------------------------------//

CRUTask *CRUTaskList::FindTask(Lng32 taskId)
{
	DSListPosition pos = GetHeadPosition();

	while (NULL != pos)
	{
		CRUTask *pTask = GetNext(pos);
		if (pTask->GetId() == taskId) 
		{
			return pTask;
		}
	}

	return NULL;
}

//--------------------------------------------------------------------------//
//	Modifiers
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUTaskList::AddRefToTask()
//--------------------------------------------------------------------------//

void CRUTaskList::AddRefToTask(CRUTask *pTask)
{
	DSListPosition listpos = FindTaskPos(pTask->GetId());
	if (NULL != listpos)
	{
		return;	// The reference already exists
	}

	AddTail(pTask);
}

//--------------------------------------------------------------------------//
//	CRUTaskList::RemoveRefToTask()
//--------------------------------------------------------------------------//

void CRUTaskList::RemoveRefToTask(CRUTask *pTask)
{
	DSListPosition listpos = FindTaskPos(pTask->GetId());
	RUASSERT(NULL != listpos);
	RemoveAt(listpos);	
}
