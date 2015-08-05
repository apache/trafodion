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
#ifndef _RU_DG_ITERATOR_H_
#define _RU_DG_ITERATOR_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuDgIterator.h
* Description:  Definition of class CRUDependenceGraphIterator.
*
*
* Created:      8/23/1999
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"

#include "RuTask.h"
#include "dsmap.h"

class CRUDependenceGraph;

//--------------------------------------------------------------------------//
// CRUDependenceGraphIterator
//
//	This is an abstract class for iterators on the dependence graph.
//
//  A general framework for the iteration:
//
//	  CRUDependenceGraphIterator it(depGraph);
//	  for (it.Build(); NULL != it.GetCurrentTask; it.Next())
//	  {
//	    ...
//	  }
//
//  The subclasses of CRUDependenceGraphIterator will use some temporaray 
//	data structure that holds the current state during the traversal. 
//	This data structure is ruined during the iteration and can be rebuilt 
//	using the Rebuild() method to start the traversal anew.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUDependenceGraphIterator {

public:
	CRUDependenceGraphIterator(CRUDependenceGraph &dg);
	virtual ~CRUDependenceGraphIterator() {};

	//----------------------------------------------//
	// Accessors
	//----------------------------------------------//
public:
	CRUTask* GetCurrentTask() 
	{	
		return pCurrentTask_;
	}

	//----------------------------------------------//
	// Mutators
	//----------------------------------------------//
public:
	// Iteration step
	virtual void Next() = 0;

public:
	// Initialization and release of the internal data structures
	virtual void Build()	= 0;
	virtual void Destroy()	= 0;

	void Rebuild() 
	{ 
		Destroy(); 
		Build(); 
	}

protected:
	CRUTaskList &GetAllTasksList() const 
	{ 
		return allTasksList_; 
	}
	void SetCurrentTask(CRUTask* pTask) 
	{ 
		pCurrentTask_ = pTask; 
	}

private:
	// Reference to the graph's available task list
	CRUTaskList &allTasksList_;

	CRUTask *pCurrentTask_;
};


// -----------------------------------------------------------------------//
// CRUTopologicalDGIterator
//
//  A topological iterator for class Dependence Graph (DG).
//  
//  Tasks in the dependence graph are executed in a topological order 
//	of traversal. This iterator supports iterating through the tasks 
//	in direct and reverse topological order.
//
//  The class DOES NOT SUPPORT connectivity changes (i.e., adding to/
//	removing from usage lists) during traversal.
//
//  It contains a hash table that holds *links* to the available tasks
//  (i.e., those that can be scheduled). A link contains a pointer to 
//	the task and the *reference count*, i.e., the number of non-traversed
//	tasks that have (direct or reverse) dependencies from this task.
//  
//	A task that has reference count of 0 is called a *ready* task.
//	Once a task becomes ready, it is placed to the *ready list*.
//	The next candidate for traversal is chosen from there.
//
// ------------------------------------------------------------------------ //

class REFRESH_LIB_CLASS CRUTopologicalDGIterator : 
                        public CRUDependenceGraphIterator {

private:
	typedef CRUDependenceGraphIterator inherited;

public:
	enum IterationDir { DIRECT = 0, REVERSE = 1 }; 

public:
	CRUTopologicalDGIterator(
		CRUDependenceGraph &dg, 
		IterationDir dir);
	virtual ~CRUTopologicalDGIterator();

public:
	//-- Implementation of pure virtual methods
	virtual void Next();

	virtual void Build();
	virtual void Destroy();

private:
	// The task's state in the iterator's context.
	// When the reference count is 0, the task is ready.
	struct TaskLink {

		CRUTask *pTask_;
		Lng32 refCount_; 
	};

	typedef CDSLongMap<TaskLink> TaskLinkMap;	

	enum { HASH_SIZE = 101 };

	void UpdateIterator(CRUTaskList &taskList);

private:
	IterationDir dir_;
	
	// All the candidates to be traversed next
	CRUTaskList readyTasksList_;

	// A hash table of links to all the tasks 
	TaskLinkMap allTasksLinkMap_;
};

#endif
