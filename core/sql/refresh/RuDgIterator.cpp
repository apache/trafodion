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
* File:         RuDgIterator.cpp
* Description:  Implementation of classes CRUDependenceGraphIterator
*				and CRUTopologicalDGIterator.
*
* Created:      12/29/1999
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuDgIterator.h"
#include "RuDependenceGraph.h"

//--------------------------------------------------------------------------//
//	Constructors and destructor
//--------------------------------------------------------------------------//

CRUDependenceGraphIterator::CRUDependenceGraphIterator(CRUDependenceGraph &dg)
 : allTasksList_(dg.availableTaskList_),
   pCurrentTask_(NULL)
{}

CRUTopologicalDGIterator::
CRUTopologicalDGIterator(CRUDependenceGraph &dg, 
						 IterationDir dir) :
	inherited(dg),
	dir_(dir),
	readyTasksList_(eItemsArentOwned),
	allTasksLinkMap_(HASH_SIZE)
{
	if (FALSE == GetAllTasksList().IsEmpty())
	{
		Build();
	}
}

CRUTopologicalDGIterator::~CRUTopologicalDGIterator()
{
	if (FALSE == GetAllTasksList().IsEmpty())
	{
		Destroy();
	}
}

//--------------------------------------------------------------------------//
//	CRUTopologicalDGIterator::Build()
//
//	Topological iterator: construction 
//--------------------------------------------------------------------------//

void CRUTopologicalDGIterator::Build()
{
	TaskLink link;

	// Fill the data structures by references to available tasks
	DSListPosition pos = GetAllTasksList().GetHeadPosition();
	while (NULL != pos)
	{
		CRUTask *pTask = GetAllTasksList().GetNext(pos);
		link.pTask_ = pTask;

		// The tasks that need to be traversed before me
		CRUTaskList &refTaskList =
			(DIRECT == dir_) ? 
				pTask->GetTasksThatIDependOn() :
				pTask->GetTasksThatDependOnMe();
			
		link.refCount_ = refTaskList.GetCount();
		
		// Place the link into the hash
		allTasksLinkMap_[pTask->GetId()] = link;

		// If this is a ready task, 
		// place a reference to it into the ready list
		if (0 == link.refCount_)
		{
			readyTasksList_.AddHead(link.pTask_);
		}
	}

	// My guess is that this is a circular graph
	RUASSERT(FALSE == readyTasksList_.IsEmpty()); 

	SetCurrentTask(readyTasksList_.RemoveHead());
}

//--------------------------------------------------------------------------//
//  CRUTopologicalDGIterator::Destroy()
//
//	Topological iterator: destruction 
//--------------------------------------------------------------------------//

void CRUTopologicalDGIterator::Destroy()
{
	readyTasksList_.RemoveAll();
	allTasksLinkMap_.RemoveAll();
}

//--------------------------------------------------------------------------//
//	CRUTopologicalDGIterator::Next()
//
//	Topological iterator: single step
//--------------------------------------------------------------------------//

void CRUTopologicalDGIterator::Next()
{
	RUASSERT(FALSE == allTasksLinkMap_.IsEmpty());

	CRUTask *pTask = GetCurrentTask();
	if (NULL == pTask)
	{
		return; // all the tasks have been already traversed
	}

	// The tasks that need to be traversed after me
	CRUTaskList &refTaskList = 
		(DIRECT == dir_) ? 
			pTask->GetTasksThatDependOnMe() :
			pTask->GetTasksThatIDependOn();

	// For all of these tasks, decrecase the reference count
	// Update the "ready" list, if any of the tasks have become ready.
	UpdateIterator(refTaskList);

	// Select a new current task
	if (TRUE == readyTasksList_.IsEmpty())
	{
		SetCurrentTask(NULL); // The last task has just been traversed
	}
	else
	{
		// Retrieve a new task from the "ready" list
		SetCurrentTask(readyTasksList_.RemoveHead());
	}
}

//--------------------------------------------------------------------------//
//	CRUTopologicalDGIterator::UpdateIterator()
//	
//	For all the tasks that depend on the currently traversed one,
//	decrease the reference count. 
//
//	Put those that have a 0 RC to the "ready" list (candidates for 
//	traversal).
//
//--------------------------------------------------------------------------//

void CRUTopologicalDGIterator::UpdateIterator(CRUTaskList &taskList)
{
	TaskLink link = {NULL, 0};

	DSListPosition pos = taskList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUTask *pTask = taskList.GetNext(pos);

		// Locate the link in the link map
		BOOL ret = allTasksLinkMap_.Lookup(pTask->GetId(), link);
		RUASSERT(TRUE == ret);

		// One reference less to the task
		link.refCount_--;

		if (0 == link.refCount_)
		{
			// One more task has no dependencies, place it to the ready list
			readyTasksList_.AddHead(link.pTask_);
		}
		else 
		{
			// Update the link's reference count in the hash
			allTasksLinkMap_[pTask->GetId()] = link;
		}
	}
}
