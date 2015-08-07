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
* File:         RuDgBuilder.cpp
* Description:  Implementation of class CRUDependenceGraphBuilder
*
* Created:      12/29/1999
* Language:     C++
*
*
******************************************************************************
*/

#include "RuDgBuilder.h"
#include "RuDgIterator.h"
#include "RuDependenceGraph.h"

#include "RuCache.h"
#include "RuGlobals.h"
#include "RuRefreshTask.h"
#include "RuDupElimTask.h"
#include "RuTableSyncTask.h"
#include "RuEmpCheckTask.h"
#include "RuLogCleanupTask.h"
#include "RuRcReleaseTask.h"
#include "RuLockEquivSetTask.h"
#include "RuPreRuntimeCheck.h"

//--------------------------------------------------------------------------//
//	PUBLIC METHODS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	Constructor and destructor
//--------------------------------------------------------------------------//

CRUDependenceGraphBuilder::
CRUDependenceGraphBuilder(CRUCache& cache, 
						  CRUDependenceGraph& dg) :
	cache_(cache),
	dg_(dg),
	nTasks_(0),
	refreshTaskList_(eItemsArentOwned),
	tblTasksList_(eItemsArentOwned)
{
	CRUOptions &options = CRUGlobals::GetInstance()->GetOptions();
	lcType_ = options.GetLogCleanupType();
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::Build()
//
//	The graph building method.
//
//	The Build() algorithm consists of four stages:
//	 (1) At the first stage, only the Refresh tasks are created.
//	     The dependencies between them are established,
//	     in a way that reflects hierarchy between the MVs.
//
//   (2) At the second stage, the LockEquivSet, EmpCheck, TableSync, 
//	     Duplicate Elimination and LogCleanup tasks are created, 
//		 if necessary, and are integrated into the dependence graph.
//	     
//	 (3) At the third stage, the RcRelease tasks are created
//		 and integrated into the dependence graph.
// 
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::Build()
{
	// Stage 1
	if (CRUOptions::DO_ONLY_LC != lcType_)
	{
		BuildRefreshTasks();
		TraverseRefreshTasks();
	}	

	// Stage 2
	// LockEquivSet/TableSync/EmpCheck/DE/LogCleanup
	BuildTableTasks();

	// Stage 3
	if (CRUOptions::DO_ONLY_LC != lcType_)
	{
		BuildRcReleaseTasks();
	}
}

//--------------------------------------------------------------------------//
//	PRIVATE METHODS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	BUILDING AND CONNECTING THE Refresh TASKS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//  CRUDependenceGraphBuilder::BuildRefreshTasks()
//
//	Build() callee - the first stage of building the graph.
//
//	Create the Refresh tasks and dependencies between them.
//	A Refresh task corresponds to every involved MV, the 
//	dependencies reflect the MV hierarchy.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::BuildRefreshTasks()
{
	RUASSERT (CRUOptions::DO_ONLY_LC != lcType_);

	CRUTask *pTask;
	CRUMVList &mvList = cache_.GetMVList();

	// Pass one: build the tasks
	DSListPosition pos = mvList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = mvList.GetNext(pos);

		// Skip the non-involved MVs
		if (FALSE == pMV->IsInvolved())
		{
			continue;
		}

		// Create a new task 
		pTask = new CRURefreshTask(++nTasks_, *pMV);
		dg_.InsertTask(pTask);

		// Store the pointer to the task
		refreshTaskList_.AddTail(pTask);
	}

	// Pass two: connect the tasks
	InterconnectRefreshTasks();
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::InterconnectRefreshTasks()
//
//	BuildRefreshTasks() callee.
//
//	Build a set of edges emerging from a single Refresh(MV) task
//	(to the set of Refresh(MVx) tasks where each MVx uses MV).
//
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::InterconnectRefreshTasks()
{
	DSListPosition pos = refreshTaskList_.GetHeadPosition();
	while (NULL != pos)
	{
		CRUTask *pTask = refreshTaskList_.GetNext(pos);
		
		CRUMV &rootMV = ((CRURefreshTask *)pTask)->GetRootMV();
		if (FALSE == rootMV.IsInvolvedTbl())
		{
			continue;
		}

		CRUMVList &mvList = 
			rootMV.GetTblInterface()->GetInvolvedMVsUsingMe();

		ConnectTaskToRefreshTasks(*pTask, mvList);
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::ConnectTaskToRefreshTasks()
//
//	Connect the task to the Refresh tasks associated with the 
//	MVs in the list. The Dir parameter is the edges' direction
//	(the default is FROM this task TO the other Refresh tasks).
//
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::
ConnectTaskToRefreshTasks(CRUTask &task, 
						  CRUMVList &mvList,
						  CRUDependenceGraphBuilder::Dir dir)
{
	RUASSERT(FALSE == mvList.IsEmpty());

	DSListPosition pos = mvList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = mvList.GetNext(pos);
		
		CRUTask *pTargetTask = dg_.GetTask(pMV->GetUID(), CRUTask::REFRESH);
		RUASSERT(NULL != pTargetTask);

		if (FORWARD == dir)
		{
			task.AddTaskThatDependsOnMe(pTargetTask);	
		}
		else
		{
			task.AddTaskThatIDependOn(pTargetTask);
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::TraverseRefreshTasks()
//
//	Build() callee (stage 1).
//
//	Traverse through the Refresh tasks (the graph's skeleton).
//	For each Refresh task:
//
//	(1) Setup the associated MV object:
//		- Propagate the recompute property from the used MVs to this one:
//		  the recompute of a used MV implies the recompute of this MV.
//		  If the MV will not be recomputed, build a delta-def list for it.
//
//	(2) Perform the pre-runtime checks for this MV (privileges, 
//	    consistency etc.)
//	
//	IMPORTANT: The operations performed on the tasks suppose that the graph
//	is traversed in the direct topological order.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::TraverseRefreshTasks()
{
	CRUTask *pTask;

	CRUTopologicalDGIterator it(dg_, CRUTopologicalDGIterator::DIRECT);

	for (; (pTask = it.GetCurrentTask()) != NULL; it.Next())
	{
		// The topological traversal is done
		// when there are only REFRESH tasks in the graph
		RUASSERT(CRUTask::REFRESH == pTask->GetType());
		CRURefreshTask *pRefreshTask = (CRURefreshTask *)pTask;

		// Action 1
		pRefreshTask->GetRootMV().PropagateRecomputeProperty();

		// Action 2
		CRUPreRuntimeCheck tester(*pRefreshTask);
		tester.PerformCheck();
	}
}

//--------------------------------------------------------------------------//
//	BUILDING THE TABLE TASKS: 
//	LockEquivSet, TableSync, EmpCheck, DE
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//  CRUDependenceGraphBuilder::BuildTableTasks()
//
//	Build() callee - the second stage of building the graph.
//
//	Create the Lock Equivalence Set, Table Sync, Duplicate 
//	Elimination(DE), Emptiness Check and Log Cleanup tasks, 
//	and integrate them into the graph. These tasks relate to 
//	involved tables that are being used by the involved 
//	ON REQUEST/RECOMPUTED MVs.
//
//  None of these tasks is created if the utility operates 
//	on a (single) ON STATEMENT MV.
//
//	The construction is performed in two stages:
//	(1) The tasks are built and stored in a list.
//	(2) The list is traversed, and the dependencies are established
//		for every created task.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::BuildTableTasks()
{
	CRUMVList &mvList = cache_.GetMVList();
	if (0 == mvList.GetHeadPosition())
	  return;

	// Do not create table tasks if an ON STATEMENT MV is refreshed
	if (CDDObject::eON_STATEMENT 
		== 
		// Recall the ON STATEMENT MVs cannot be refreshed in a group
		cache_.GetMVList().GetAt(0)->GetRefreshType())
	{
		return;
	}

	// Stage 1
	if (CRUOptions::DO_ONLY_LC != lcType_)
	{
		BuildEquivSetBasedTableTasks();		// LockEquivSet	
	}

	BuildSingleTableTasks();	// EmpCheck/TableSync/DE/LogCleanup

	// Prepare stage 2
	BuildMVEquivSets();

	// Stage 2
	ConnectTableTasks();		
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::BuildEquivSetBasedTableTasks()
//	
//	Perform the equivalence set analysis, and partition the involved
//	tables (which are not the involved MVs) into disjoint equivalence
//	sets. 
//
//	For every set, there is a single LockEquivSet task.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::BuildEquivSetBasedTableTasks()
{
	CRUMVList &mvList = cache_.GetMVList();

	// Initialize the equivalence set analyzer ...
	DSListPosition pos = mvList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = mvList.GetNext(pos);
		if (FALSE == pMV->IsInvolved())
		{
			continue;
		}

		tblEquivSetBuilder_.AddMV(pMV);
	}

	// Perform the equivalence set analysis...
	tblEquivSetBuilder_.Run();

	// And create the tasks, equivalence set-based
	for (Int32 i=0; i<tblEquivSetBuilder_.GetNumOfSets(); i++)
	{
		CRUTblList &tblList = tblEquivSetBuilder_.GetSet(i);

		// The LockEquivSet task handles a number of tables
		BuildLockEquivSetTask(tblList);
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::BuildMVEquivSets()
//	
//	Partition the ON REQUEST MVs to equivalence sets.
//	This is a basis for the correct wiring of the EmpCheck tasks in the graph.
//
//	MV equivalence sets are connected components in the hierarchy 
//	between the involved MVs.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::BuildMVEquivSets()
{
	CRUMVList &mvList = cache_.GetMVList();

	// Initialize the equivalence set analyzer ...
	DSListPosition pos = mvList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = mvList.GetNext(pos);
		if (FALSE == pMV->IsInvolved())
		{
			continue;
		}

		if (CDDObject::eON_REQUEST == pMV->GetRefreshType())
		{
			mvEquivSetBuilder_.AddMV(pMV);
		}
	}

	// Perform the equivalence set analysis...
	mvEquivSetBuilder_.Run();

#ifdef _DEBUG
	CRUOptions &options = CRUGlobals::GetInstance()->GetOptions();
	if (NULL != options.FindDebugOption(CRUGlobals::DUMP_DS, ""))
	{
		mvEquivSetBuilder_.DumpSets();
	}
#endif
}

//--------------------------------------------------------------------------//
//  CRUDependenceGraphBuilder::BuildSingleTableTasks()
//
//	Create the TableSync/EmpCheck/DE/LogCleanup tasks. 
//
//	Each task handles a single table object. Tasks of these types are created 
//	only for involved tables (i.e., tables used by involved MVs). 
//
//	ASSUMPTION: applied after the Recompute attribute is propagated
//	through the Refresh tasks.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::BuildSingleTableTasks()
{
	// A single pass over all CRUTbl objects in the cache
	CRUTblList &tblList = cache_.GetTableList();
	DSListPosition pos = tblList.GetHeadPosition();

	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList.GetNext(pos);

		if (FALSE == pTbl->IsInvolved())
		{
			continue;
		}

		RUASSERT (pTbl->GetInvolvedMVsUsingMe().GetCount() > 0);

		// Now that we know for every MV whether it will be
		// refreshed incrementally or not - build the list
		pTbl->BuildListOfIncrementalInvolvedMVsUsingMe();

		BuildSingleTableTasksForTbl(*pTbl);
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::BuildSingleTableTasksForTbl()
//
//	Build the TableSync/EmpCheck/DE/LogCleanup tasks associated
//	with a *single* table (which is used by an involved MV).
//
//	Conditions for creating the tasks (provided that the
//	Log Cleanup mode is appropriate):
//	(1) TableSync task - the table is NOT an involved MV
//	    (if it is, epoch increment is performed in the Refresh task).
//	(2) Emptiness Check task - the table is used by an incrementally
//		refreshed ON REQUEST MV.
//	(3) Duplicate Elimination task - the table is used by an incrementally
//		refreshed ON REQUEST MV + the conditions in CRUTbl::IsDENeeded().
//	(4) Log Cleanup task - the table is used by an incrementally
//		refreshed ON REQUEST MV, and WITH LOG CLEANUP was requested.
//	
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::BuildSingleTableTasksForTbl(CRUTbl &tbl)
{	
	if (FALSE == tbl.IsInvolvedMV()
		&&
		CRUOptions::DO_ONLY_LC != lcType_)
	{
		BuildTableSyncTask(tbl);	// (1)
	}

	// Pre-conditions for EmpCheck and DE tasks
	if (TRUE == tbl.IsUsedByIncRefreshedMV()
		&&
		CRUOptions::DO_ONLY_LC != lcType_)
	{
		BuildEmpCheckTask(tbl);	// (2) 

		// Compute whether we need duplicate elimination ...
		tbl.CheckIfDENeeded();

		// And if yes, create the DE task
		if (TRUE == tbl.IsDENeeded())
		{
			BuildDETask(tbl);	// (3)
		}
	}

	if (TRUE == tbl.IsUsedByOnRequestMV()
		&&
		CRUOptions::WITHOUT_LC != lcType_)
	{
		BuildLogCleanupTask(tbl);	// (4)	
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::ConnectTableTasks()
//
//	BuildTableTasks() callee.
//	
//	The newly-created table tasks are stored in tblTasksList_.
//	Traverse the list and connect each task by its own rules.
// 
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::ConnectTableTasks()
{
	DSListPosition pos = tblTasksList_.GetHeadPosition();
	while (NULL != pos)
	{
		CRUTask *pTask = tblTasksList_.GetNext(pos);
		switch (pTask->GetType())
		{
			case CRUTask::TABLE_SYNC:
				{
					ConnectTableSyncTask((CRUTableSyncTask &) *pTask);
					break;
				}
			
			case CRUTask::DUP_ELIM:
				{
					ConnectDupElimTask((CRUDupElimTask &) *pTask);
					break;
				}
			
			case CRUTask::EMP_CHECK:
				{
					ConnectEmpCheckTask((CRUEmpCheckTask &) *pTask);
					break;
				}
			
			case CRUTask::LOCK_EQUIV_SET:
				{
					ConnectLockEquivSetTask((CRULockEquivSetTask &) *pTask);
					break;
				}
			
			case CRUTask::LOG_CLEANUP:
				{
					ConnectLogCleanupTask((CRULogCleanupTask &) *pTask);
					break;
				}

			case CRUTask::RC_RELEASE:
			case CRUTask::REFRESH:
			default:
			{
				RUASSERT(FALSE);	// All irrelevant
			}
		}
	}
}

//--------------------------------------------------------------------------//
//	BUILDING AND CONNECTING THE LockEquivSet TASK
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::BuildLockEquivSetTask()
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::BuildLockEquivSetTask(CRUTblList &tblList)
{
	CRUTask *pTask = new CRULockEquivSetTask(++nTasks_, tblList);
	dg_.InsertTask(pTask);
	tblTasksList_.AddTail(pTask);
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::ConnectLockEquivSetTask()
//
//	For every table T handled by LockEquivSet task, there is a
//	corresponding TableSync(T) task, that depends on the LockEquivSet task.
//
//	LockEquivSet (T1, T2, T3) -+-------------> TableSync(T1)
//	                           |
//	                           +-------------> TableSync(T2)
//	                           |
//	                           +-------------> TableSync(T3)
//
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::
ConnectLockEquivSetTask(CRULockEquivSetTask &task)
{
	CRUTblList &tblList = task.GetTableList();

	DSListPosition pos = tblList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList.GetNext(pos);
		
		CRUTask *pSuccTask = dg_.GetTask(pTbl->GetUID(), CRUTask::TABLE_SYNC);
		RUASSERT(NULL != pSuccTask);

		task.AddTaskThatDependsOnMe(pSuccTask);
	}	
}

//--------------------------------------------------------------------------//
//	BUILDING AND CONNECTING THE TableSync TASK
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//  CRUDependenceGraphBuilder::BuildTableSyncTask()
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::BuildTableSyncTask(CRUTbl &tbl)
{
	CRUTask *pTask = new CRUTableSyncTask(++nTasks_, tbl);
	dg_.InsertTask(pTask);
	tblTasksList_.AddTail(pTask);
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::ConnectTableSyncTask()
//
//	Establish the TableSync's task's connectivity. 
// 
//	(1) The TableSync task will be connected to the Refresh tasks 
//		for ALL the involved MVs using the table.
//	(2) If the table is used by am incrementally refreshed MV
//		(hence, there is an EmpCheck task for it), there is also
//		an edge TableSync(T) --> EmpCheck(T)
//		
//		EmpCheck(T1) --> Refresh(MV2) 
//		    ^             ^
//		    |			  |
//		TableSync(T1) ----+
//	        |
//	        +---------> Refresh(MV1, recompute)
//
//	This is an example of a suboptimal transitive closure.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::ConnectTableSyncTask(CRUTableSyncTask &task)
{
	CRUTbl &tbl = task.GetTable();

	if (TRUE == tbl.IsUsedByIncRefreshedMV())
	{
		CRUTask *pSuccTask = dg_.GetTask(tbl.GetUID(), CRUTask::EMP_CHECK);

		// TableSync(T) --> EmpCheck(T)
		RUASSERT (NULL != pSuccTask);
		task.AddTaskThatDependsOnMe(pSuccTask);
	}
	else
	{
		// If there is no emptiness check, there is also no DE
		RUASSERT(FALSE == tbl.IsDENeeded());
	}

	ConnectTaskToRefreshTasks(task, tbl.GetInvolvedMVsUsingMe());
}

//--------------------------------------------------------------------------//
//	BUILDING AND CONNECTING THE EmpCheck TASK
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//  CRUDependenceGraphBuilder::BuildEmpCheckTask()
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::BuildEmpCheckTask(CRUTbl &tbl)
{
	CRUTask *pTask = new CRUEmpCheckTask(++nTasks_, tbl);
	dg_.InsertTask(pTask);
	tblTasksList_.AddTail(pTask);
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::ConnectEmpCheckTask()
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::ConnectEmpCheckTask(CRUEmpCheckTask &task)
{
	if (TRUE == task.GetTable().IsInvolvedMV())
	{
		ConnectEmpCheckTaskIfTblIsInvolvedMV(task);
	}
	else
	{
		ConnectEmpCheckTaskIfTblIsNotInvolvedMV(task);
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::ConnectEmpCheckTaskIfTblIsInvolvedMV()
//
//	If the table is also an involved MV, then connect it *BEFORE* each
//	Refresh(MVi) task for every MVi that can pipeline the data to this MV. 
//	
//	BEFORE-STATE
//	  	
//		Refresh(MV1) ---> Refresh(MV2) ---> Refresh(MV3)
//
//	AFTER-STATE
//
//		Refresh(MV1) ---> Refresh(MV2) ---> Refresh(MV3)
//		 ^	 ^
//	     |   |
//	     |   +----- EmpCheck(MV1)
//		 |
//		 +--------- EmpCheck(MV2)
//	
//	This connection is the requirement of the pipelining mechanism,
//	because the results of MV1's and MV2's emptiness check must be ready
//	BEFORE scheduling Refresh(MV1), in order to decide which REFRESH tasks
//	can be collapsed for pipelining.
//
//	In the example, MV1, MV2 and MV3 form an *MV equivalence set* (do not
//	confuse with the *table equivalence set*). MV1 is the equivalence set's
//	*root*. The connection rule is:
//
//	(1) If the MV is an equiv set's root, connect EmpCheck(MV) directly 
//		to Refresh(MV).
//	(2) Otherwise, connect EmpCheck(MV) to all of the roots of its 
//		equivalence set.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::
ConnectEmpCheckTaskIfTblIsInvolvedMV(CRUEmpCheckTask &task)
{
	TInt64 uid = task.GetTable().GetUID();

	CRUMVList &mvList = mvEquivSetBuilder_.GetEquivSetsRootsByMVUID(uid);
	
	// Check whether the MV is the MV equiv set's root itself ...
	BOOL isRoot = FALSE;
	
	DSListPosition pos = mvList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = mvList.GetNext(pos);

		if (pMV->GetUID() == uid)
		{
			isRoot = TRUE;
			break;
		}
	}

	// Connect the EmpCheck task ...
	CRUTask *pSuccTask;

	if (TRUE == isRoot)
	{
		// EmpCheck(MV) --> REFRESH(MV), MV is the equiv set's root itself
		pSuccTask = dg_.GetTask(uid, CRUTask::REFRESH);
		RUASSERT(NULL != pSuccTask);

		task.AddTaskThatDependsOnMe(pSuccTask);
	}
	else
	{
		pos = mvList.GetHeadPosition();
		while (NULL != pos)
		{
			CRUMV *pMV = mvList.GetNext(pos);

			// EmpCheck(MV) --> REFRESH(MVx), MVx is the equiv's set root
			pSuccTask = dg_.GetTask(pMV->GetUID(), CRUTask::REFRESH);
			RUASSERT(NULL != pSuccTask);

			task.AddTaskThatDependsOnMe(pSuccTask);
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::ConnectEmpCheckTaskIfTblIsNotInvolvedMV()
//
//	If the table is NOT an involved MV (it can be either a regular table
//	or an MV itself), the emptiness check must be performed BEFORE the
//	refresh of any MV that uses the table.
//
//	If no DE must be performed on the table, the EmpCheck(T) task will be
//	connected directly to the Refresh(MVx) tasks for the MVs that use the
//	table:
//
//	                  +-----------> Refresh(MV1)
//	                  |
//	EmpCheck(T) ------+-----------> Refresh(MV2)
//
//
//	If DE must be performed, the EmpCheck(T) task will be
//	connected  to the Refresh(MVx) tasks for the MVs through the DE task.
//
//	                            +-----------> Refresh(MV1)
//	                            |
//	EmpCheck(T) ---> DE(T) -----+-----------> Refresh(MV2)
//
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::
ConnectEmpCheckTaskIfTblIsNotInvolvedMV(CRUEmpCheckTask &task)
{
	CRUTbl &tbl = task.GetTable();

	if (TRUE == tbl.IsDENeeded())
	{
		// Connection through the DE task
		TInt64 uid = task.GetTable().GetUID();
		CRUTask *pSuccTask = dg_.GetTask(uid, CRUTask::DUP_ELIM);
		
		RUASSERT(NULL != pSuccTask);
		task.AddTaskThatDependsOnMe(pSuccTask);
	}
	else
	{
		// Direct connection to the Refresh tasks
		ConnectTaskToRefreshTasks(
			task, 
			tbl.GetIncrementalInvolvedMVsUsingMe()
		);
	}
}

//--------------------------------------------------------------------------//
//	BUILDING AND CONNECTING THE DE TASK
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//  CRUDependenceGraphBuilder::BuildDETask()
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::BuildDETask(CRUTbl &tbl)
{
	CRUTask *pTask = new CRUDupElimTask(++nTasks_, tbl);
	dg_.InsertTask(pTask);
	tblTasksList_.AddTail(pTask);
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::ConnectDupElimTask()
//
//	Establish the DE task's connectivity. There are two cases.
// 
//	(1) If the table is an MV that is refreshed in this invocation
//	    (and hence, has a corresponding Refresh task):
//
//	BEFORE-STATE
//
//		REFRESH(MV1) -----> REFRESH(MV2)
//
//	AFTER-STATE
//					     
//		REFRESH(MV1) --> REFRESH(MV2)
//			|                ^
//          |                |
//          +-----------> DE(MV1)		
//
//	(2) Otherwise, the DE task will have only outgoing edges:
//		
//		 DE(T) ---> REFRESH(MV1)
//
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::ConnectDupElimTask(CRUDupElimTask &task)
{
	CRUTbl &tbl = task.GetTable();

	// (1) Check whether the DE must be performed on an involved MV
	if (TRUE == tbl.IsInvolvedMV())
	{
		// The DE task is performed after the Refresh task
		// Add the edge REFRESH -> DE
		CRUTask *pPredTask = dg_.GetTask(tbl.GetUID(), CRUTask::REFRESH);

		RUASSERT (NULL != pPredTask);
		task.AddTaskThatIDependOn(pPredTask);
	}

	// (2) Connect the DE task to all of its customers
	//     (incremental Refresh tasks)
	ConnectTaskToRefreshTasks(task, tbl.GetIncrementalInvolvedMVsUsingMe());
}

//--------------------------------------------------------------------------//
//	BUILDING AND CONNECTING THE LogCleanup TASK
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//  CRUDependenceGraphBuilder::BuildLogCleanupTask()
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::BuildLogCleanupTask(CRUTbl &tbl)
{
	CRUTask *pTask = new CRULogCleanupTask(++nTasks_, tbl);
	dg_.InsertTask(pTask);
	tblTasksList_.AddTail(pTask);
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::ConnectLogCleanupTask()
//
//	Add the edge Refresh(MV) --> LogCleanup(T) for every involved 
//	ON REQUEST MV that uses T. The MV is NOT NECESSARILY refreshed
//	incrementally (log cleanup cannot happen before all the MVs' epoch
//	vectors are advanced).
//
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::ConnectLogCleanupTask(CRULogCleanupTask &task)
{
	if (CRUOptions::DO_ONLY_LC == lcType_)
	{
		return;	// There are only Log Cleanup tasks in the graph
	}

	CRUTbl &tbl = task.GetTable();
	CRUMVList &mvList = tbl.GetOnRequestInvolvedMVsUsingMe();

	ConnectTaskToRefreshTasks(task, mvList, BACKWARD /* !!! */);
}

//--------------------------------------------------------------------------//
//	BUILDING AND CONNECTING THE RcRELEASE TASKS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::BuildRcReleaseTasks()
//
//	Build the Resource Release tasks, in 3 steps.
//	(1) Create tasks that correspond to the involved MVs
//	(2) Create tasks that correspond to the involved tables
//	    that are not involved MVs.
//	(3) Connect the newly-created tasks to the other
//		tasks in the graph.
//
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::BuildRcReleaseTasks()
{
	//RUASSERT(FALSE == cache_.GetMVList().IsEmpty());
	
	BuildRcReleaseTasksForMVs();

	BuildRcReleaseTasksForTables();

	ConnectRcReleaseTasks();
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::BuildRcReleaseTasksForMVs()
//
//	Create the Resource Release tasks that correspond to the involved MVs.
//	
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::BuildRcReleaseTasksForMVs()
{
	CRUMVList &mvList = cache_.GetMVList();

	DSListPosition pos = mvList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = mvList.GetNext(pos);

		// Skip the non-involved MVs
		if (FALSE == pMV->IsInvolved())
		{
			continue;
		}

		BuildRcReleaseTask(*pMV);
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::BuildRcReleaseTasksForTables()
//
//	Create the tasks that correspond to the involved tables 
//	that are NOT involved MVs (for those, they are already
//	created).
//
//	If the DDL lock on the table must NOT be cancelled after
//	the execution of Refresh is complete (i.e., the table is a
//	multi-transactional MV that is not synchronized on epoch
//	boundary), the RcRelease task will not be created. Therefore,
//	for these objects, the DDL lock will be *replaced* at the
//	cache construction stage, but will finally NOT be removed.
//	
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::BuildRcReleaseTasksForTables()
{
	CRUTblList &tableList = cache_.GetTableList();

	DSListPosition pos = tableList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUTbl *pTbl = tableList.GetNext(pos);

		if (FALSE == pTbl->IsInvolved())
		{
			continue;
		}

		if (TRUE == pTbl->IsInvolvedMV())
		{
			// If the table is also an involved MV,
			// there is already an RcRelease task for it
			RUASSERT (NULL != dg_.GetTask(pTbl->GetUID(), CRUTask::RC_RELEASE));
			continue;
		}

		BuildRcReleaseTask(*pTbl);
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::BuildRcReleaseTask()
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::BuildRcReleaseTask(CRUObject &obj)
{ 
	CRUTask *pTask = new CRURcReleaseTask(++nTasks_, obj);
	dg_.InsertTask(pTask);
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::ConnectRcReleaseTasks()
//
//	Establish the connectivity between the RcRelease tasks
//	and the rest of the graph. The edges are of two types:
//	(1) Refresh(MV) --> RcRelease(MV), where MV is an involved MV.
//	(2) Refresh(MV) --> RcRelease(T), where T is a table used by MV.
//	
//	EXAMPLE
//		MV2 uses MV1 that uses T, and both MV1 and MV2 are involved.
//
//	The graph fragment will look like:
//
//		RcRelease(MV2)
//			^
//			|
//			|
//		Refresh(MV2) ---------> RcRelease(MV1)
//			^                        ^
//			|                        |
//		Refresh(MV1)--------------------------> RcRelease(T)
//
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::ConnectRcReleaseTasks()
{
	DSListPosition pos = refreshTaskList_.GetHeadPosition();
	while (NULL != pos)
	{
		CRURefreshTask *pRefreshTask  = 
			(CRURefreshTask *)(refreshTaskList_.GetNext(pos));

		CRUTask *pRcReleaseTask = 
			dg_.GetTask(pRefreshTask->GetRootMV().GetUID(), 
			            CRUTask::RC_RELEASE);
		
		RUASSERT(NULL != pRcReleaseTask);

		// Refresh(MV) --> RcRelease(MV)
		pRefreshTask->AddTaskThatDependsOnMe(pRcReleaseTask);

		// Build the Refresh(MV) --> RcRelease(T) tasks,
		// for each table T used by MV
		ConnectRefreshTaskToTableRcReleaseTasks(*pRefreshTask);
	}
}

//--------------------------------------------------------------------------//
//	CRUDependenceGraphBuilder::ConnectRefreshTaskToTableRcReleaseTasks()
//
//	Build the Refresh(MV) --> RcRelease(T) tasks, for each table T used by MV.
//
//	Condition: there must be an RcRelease task associated with T.	
//	For example, if a DDL lock on T is non-cancellable, no RcRelease task
//	will be created for T.
//	
//--------------------------------------------------------------------------//

void CRUDependenceGraphBuilder::
ConnectRefreshTaskToTableRcReleaseTasks(CRURefreshTask &task)
{
	CRUMV &mv = task.GetRootMV();

	CRUTblList &tblList = mv.GetTablesUsedByMe();

	DSListPosition pos = tblList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList.GetNext(pos);

		CRUTask *pRcReleaseTask = 
			dg_.GetTask(pTbl->GetUID(), CRUTask::RC_RELEASE);

		if (NULL == pRcReleaseTask)
		{
			// No RcRelease task corresponding to this table 
			continue;
		}

		// Refresh(MV) --> RcRelease(T)
		task.AddTaskThatDependsOnMe(pRcReleaseTask);
	}
}
