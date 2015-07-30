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
#ifndef _RU_DG_BUILDER_H_
#define _RU_DG_BUILDER_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuDgBuilder.h
* Description:  Definition of class CRUDependenceGraphBuilder
*
* Created:      12/29/1999
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"
#include "dsplatform.h"

#include "RuOptions.h"
#include "RuTask.h"
#include "RuTblEquivSetBuilder.h"
#include "RuMVEquivSetBuilder.h"

class CRUTbl;
class CRUMV;

class CRUCache;
class CRUDependenceGraph;

class CRURefreshTask;
class CRUTableSyncTask;
class CRUDupElimTask;
class CDSException;
class CRUEmpCheckTask;
class CRULockEquivSetTask;
class CRULogCleanupTask;

//--------------------------------------------------------------------------//
// CRUDependenceGraphBuilder
// 
//  The dependence graph builder class.
//
//   Encapsulates the logic of creating tasks (nodes in the dependence graph) 
//	 and dependencies between them (edges in the dependence graph).
//
//	 The graph will include three categories of tasks:
//	 (1) Refresh tasks.
//	 (2) Table tasks (LockEquivSet, Table Sync, DE, Log Cleanup).
//	 (3) Resource Release tasks.
//
//	 A Refresh task (works on one or more MVs) refreshes a single MV 
//	 or a number of MVs, using delta pipelining. 
//
//	 A LockEquivSet task (works on one or more tables) performs a simultaneous
//	 lock of *logs* for tables that belong to the same equivalence set, 
//	 to achieve a consistent database cut.
//
//	 A TableSync task (works on a single table) completes the work 
//	 of LockEquivSet, by performing the epoch increment and/or the 
//	 lock of *table*. Following the two tasks, the table is synchronized 
//	 with its log and with the other tables that it participates in joins with.
//
//	 A EmpCheck task (works on a single table) checks whether the 
//	 delta(s) of the table towards the using MV(s) are empty.
//	
//	 A DE task (works on a single table) performs duplicate elimination 
//	 on the table's log, i.e., maps the ranges from the IUD log to the
//	 range log and resolves the duplicate log records.
//
//	 A Log Cleanup task (works on a single table)
//	 deletes the inapplicable rows from the table's log.
//
//	 A Resource Release task (works on a single MV/table) frees the captured 
//	 object's resources (the DDL lock and/or read-protected open).
//
//	 See the description of the task interconnection rules 
//	 in the class's implementation. 
//
//	 GENERAL COMMENTS:
//
//	 (1) Sometimes, the simplicity of the tasks' interconnection rules 
//	 is traded off for the graph's complexity. For example, patterns like:
//
//	 TaskA -----> TaskC
//	    ^           ^
//	    |           | 
//	 TaskB----------+
//
//	 can appear. The graph's builder will not try to optimize the transitive
//	 closure, because in runtime the extra edges cannot be harmful.
//
//	 (2) There are cases when the graph builder does not invest 100% effort
//	 to identify (yet at the early stage) the tasks which will not be actually
//	 executed, and leaves the tasks themselves to do that. This is also done
//	 to reduce the graph's construction complexity.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUDependenceGraphBuilder {

public:
	CRUDependenceGraphBuilder(
		CRUCache& cache, 
		CRUDependenceGraph& dg);
	
	virtual ~CRUDependenceGraphBuilder() {}

public:
	// Create the initial task set 
	// and establish connectivity between tasks.
	void Build();	

private:
	//-- Prevent copying
	CRUDependenceGraphBuilder(const CRUDependenceGraphBuilder& other);
	CRUDependenceGraphBuilder& operator= (const CRUDependenceGraphBuilder& other);

private:
	//-- Build() callees
	void BuildRefreshTasks();
	void TraverseRefreshTasks();

	void BuildTableTasks(); // LockEquivSet/TableSync/EmpCheck/DE/LogCleanup

	void BuildRcReleaseTasks();

private:
	//-- BuildRefreshTasks() callee
	void InterconnectRefreshTasks();

	//-- BuildTableTasks() callees
	void BuildEquivSetBasedTableTasks();	// LockEquivSet/TableSync
	void BuildSingleTableTasks();	// EmpCheck/DE/LogCleanup
	void ConnectTableTasks();

	void BuildSingleTableTasksForTbl(CRUTbl &tbl);

	void BuildLockEquivSetTask(CRUTblList &tblList);
	void BuildTableSyncTask(CRUTbl &tbl);
	void BuildEmpCheckTask(CRUTbl &tbl);
	void BuildDETask(CRUTbl &tbl);
	void BuildLogCleanupTask(CRUTbl &tbl);

	void ConnectLockEquivSetTask(CRULockEquivSetTask &task);
	void ConnectTableSyncTask(CRUTableSyncTask &task);
	void ConnectDupElimTask(CRUDupElimTask &task);
	void ConnectEmpCheckTask(CRUEmpCheckTask &task);
	void ConnectLogCleanupTask(CRULogCleanupTask &task);
	
	void ConnectEmpCheckTaskIfTblIsInvolvedMV(CRUEmpCheckTask &task);
	void ConnectEmpCheckTaskIfTblIsNotInvolvedMV(CRUEmpCheckTask &task);

	// BuildRcReleaseTasks() callees
	void BuildRcReleaseTasksForMVs();
	void BuildRcReleaseTasksForTables();
	void BuildRcReleaseTask(CRUObject &obj);

	void ConnectRcReleaseTasks();
	void ConnectRefreshTaskToTableRcReleaseTasks(CRURefreshTask &task);

	// General
	enum Dir { FORWARD, BACKWARD };

	void ConnectTaskToRefreshTasks(
		CRUTask &task, 
		CRUMVList &mvList,
		Dir dir = FORWARD);
private:
	void BuildMVEquivSets();

private:
	//-----------------------------------//
	//	Core data members
	//-----------------------------------//

	CRUCache &cache_;
	CRUDependenceGraph &dg_;

	Lng32 nTasks_;
	CRUOptions::LogCleanupType lcType_;

	// Tasks lists
	CRUTaskList refreshTaskList_;
	CRUTaskList tblTasksList_;

	// Equivalence set analyzers
	CRUTblEquivSetBuilder tblEquivSetBuilder_; 
	CRUMVEquivSetBuilder mvEquivSetBuilder_;

};

#endif
