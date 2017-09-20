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
#ifndef _RU_UNAUDIT_REFRESH_TASK_EXECUTOR_H_
#define _RU_UNAUDIT_REFRESH_TASK_EXECUTOR_H_

/* -
*-C++-*-
******************************************************************************
*
* File:         RuUnAuditRefreshTaskExecutor.h
* Description:  Definition of class CRUUnAuditRefreshTaskExecutor.
*
*
* Created:      08/14/2000
* Language:     C++
*
*
******************************************************************************
*/

//--------------------------------------------------------------------------//
//
//	CRUUnAuditRefreshTaskExecutor
//
//	This class handle the refresh of a NO AUDIT/NO AUDITONREFRESH MV,
//	and the initialization (through recompute) of an AUDITED MV whose
//	status is NON_INITIALIZED/UNAVAILABLE.
//
//	The class implements a finite state machine where some states 
//	may be executed in a different process. The task executor supports
//	the refresh of either a single MV, or a list of pipelined MVs.
//
//	The process of refresh is performed outside the transaction boundaries,
//	to avoid the problems of autoabort and lock escalation.
//	NOTE. CURRENTLY, THE REFRESH IS DONE IN A TRANSACTION BECAUSE OF AN
//	EXTERNAL DEPENDENCY ON DP2: THE TRANSACTION IS OPENED AUTOMATICALLY
//	BECAUSE OF THE READ-COMMITTED SELECT FROM THE AUDITED TABLES.
//
//	The MV(s) remain(s) non-available throughout the process of refresh,
//	because the algorithm sometimes performs non-recoverable actions.
//	The MV (and its indexes) are returned to the available state at the 
//	end of the execution.
// 
//	The process of recompute can be optimized in two ways:
//	(1) The use of purgedata from MV (instead of the DELETE part of 
//		INTERNAL REFRESH), if there was previously some data in the MV,
//		and the user has a DELETE privilege.
//	(2) The indexes are disabled during recompute (to avoid IM), and 
//		populated separately (without transaction) when the recompute is over.
//
//	When initializing an ON STATEMENT MV, the task executor employs 
//	sophisticated lock handling to ensure that at the end of refresh
//	the MV is consistent with the tables, and incremental maintenance
//	can start immediately.
//
//--------------------------------------------------------------------------//

#include "RuSimpleRefreshTaskExecutor.h"

class REFRESH_LIB_CLASS CRUUnAuditRefreshTaskExecutor : 
		public CRUSimpleRefreshTaskExecutor
{
private:
	typedef CRUSimpleRefreshTaskExecutor inherited;

public:
	CRUUnAuditRefreshTaskExecutor(CRURefreshTask *pParentTask = NULL);
	virtual ~CRUUnAuditRefreshTaskExecutor();

public:
	//-- Implementation of pure virtuals	
	virtual void Work();
	virtual void Init();

public:
	// These functions serialize/de-serialize the executor's context 
	// for the message communication with the remote server process

	// Used at the main process side
	virtual void StoreRequest(CUOFsIpcMessageTranslator &translator);
	virtual void LoadReply(CUOFsIpcMessageTranslator &translator)
	{
		inherited::LoadReply(translator);
	}
	
	// Used at the remote process side
	virtual void LoadRequest(CUOFsIpcMessageTranslator &translator);
	virtual void StoreReply(CUOFsIpcMessageTranslator &translator)
	{
		inherited::StoreReply(translator);
	}

	//----------------------------------//
	//	Private Members
	//----------------------------------//	
private:
	enum STATES { 
		// Local states
		EX_PROLOGUE = MAIN_STATES_START,
		EX_EPILOGUE,
		EX_PURGE_DATA,
		// Remote states
		EX_REFRESH = REMOTE_STATES_START,
		EX_RECOMPUTE,
		EX_POPINDEX
	};

	// Work() callees
	void Start();
	void Prologue();
	void Refresh();
	void PurgeData();
	void Recompute();
	void PopulateIndexes();
	void Epilogue();

	void ComposeMySql();
	void ComposeIndexesSql();

	// First txn: set the MV (+ optionally its indexes) non-available
	void SetObjectsUnavailable();
	// Last txn: set the MV (+ optionally its indexes) back available
	void ResetObjectsAvailable();

	void UnLockMV();

	// Special handling of the ON STATEMENT MV's initialization
	void PrologueHandleOnStatementMV();
	void EpilogueHandleOnStatementMV();

	void ExecuteIndexStatmenents(CRUSQLDynamicStatementContainer &container, 
								 Lng32 errorCode);
private:
	enum SQL_STATEMENT { 
		
		RU_UNLOCK_TABLE = 0,
		NUM_OF_SQL_STMT // should always be last
	};

	// Statements specific for this task executor
	CRUSQLDynamicStatementContainer unAuditRefreshTEDynamicContainer_;
	CRUSQLDynamicStatementContainer *pUnAuditPopIndexdynamicContainer_;
	CRUSQLDynamicStatementContainer *pUnAuditAvailableIndeXdynamicContainer_;
	CRUSQLDynamicStatementContainer *pUnAuditUnavailableIndeXdynamicContainer_;
	TInt32 numOfIndexes_;
	
	// The container of LOCK TABLE statements
	CRUSQLDynamicStatementContainer *pLockTablesTEDynamicContainer_;

	BOOL isPurgedata_;	
	BOOL isPopindex_;
};

#endif
