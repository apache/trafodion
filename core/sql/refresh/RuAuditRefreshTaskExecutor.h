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
#ifndef _RU_AUDIT_REFRESH_TASK_EXECUTOR_H_
#define _RU_AUDIT_REFRESH_TASK_EXECUTOR_H_

/* -
*-C++-*-
******************************************************************************
*
* File:         RuAuditRefreshTaskExecutor.h
* Description:  Definition of class CRUAuditRefreshTaskExecutor.
*
* Created:      08/14/2000
* Language:     C++
*
*
******************************************************************************
*/
#include "RuSimpleRefreshTaskExecutor.h"

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor
//
// This class handle the refresh of an audited mv.The class implements a 
// finite state machine where some states may be executed in a different 
// process.
//
// The executor first execute internal refresh statement which may be executed 
// in a remote process and then updates the mv's metadata which must be done 
// in the main process.
// The executor handles the following types of refresh types :
//	1. Incremental refresh in a single transaction of an audited mv.
//	2. Recomputation of a single transaction audited mv 
//	3  Recomputation of a multi transaction audited mv
//	4. Incremental refresh of multiple syncronized mv's by using 
//	   pipeline optimization
// 
// When the executor needs to update the mv's metadata it calls to his 
// ancestor executor to execute his updates.(in case of this executor, all 
// updates are done in the ancestor funtion
// see CRUSimpleRefreshTaskExecutor::FinalMetadataUpdate()).
//
// The class has the following states :
//
//	1. EX_PROLOGUE - Sets objects unavailable
//      2. EX_REMOTE_START - Starts the tablelock protocol on remote 
//                               process and starts a transaction
//	3. EX_RECOMPUTE - Execute an internal refresh recompute statement 
//	4. EX_REFRESH - Execute an internal refresh statement 
//	5. EX_EPILOGUE - Updates the metadata and Commits the transation
//
//--------------------------------------------------------------------------//
class REFRESH_LIB_CLASS CRUAuditRefreshTaskExecutor : 
		public CRUSimpleRefreshTaskExecutor
{
private:
	typedef CRUSimpleRefreshTaskExecutor inherited;

	//----------------------------------//
	//	Public Members
	//----------------------------------//
public:
	CRUAuditRefreshTaskExecutor(CRURefreshTask *pParentTask = NULL);
	virtual ~CRUAuditRefreshTaskExecutor();

public:
	//-- Implementation of pure virtuals	
	virtual void Work();
	virtual void Init();

public:
	// These functions serialize/de-serialize the executor's context 
	// for the message communication with the remote server process
	
	// Used in the main process side
	virtual void StoreRequest(CUOFsIpcMessageTranslator &translator);
	virtual void LoadReply(CUOFsIpcMessageTranslator &translator)
	{
		inherited::LoadReply(translator);
	}
	
	// Used in the remote process side
	virtual void LoadRequest(CUOFsIpcMessageTranslator &translator);
	virtual void StoreReply(CUOFsIpcMessageTranslator &translator)
	{
		inherited::StoreReply(translator);
	}

	//----------------------------------//
	//	Private Members
	//----------------------------------//	
private:
	//-- Prevent copying
	CRUAuditRefreshTaskExecutor(const CRUAuditRefreshTaskExecutor &other);
	CRUAuditRefreshTaskExecutor &operator = (const CRUAuditRefreshTaskExecutor &other);

private:
	void ComposeMySql();
        void ComposeIndexesSql();

        void SetObjectsUnavailable();
        void ResetObjectsAvailable();
        void ToggleIndicesAudit(BOOL flag);

	//Work() callees
	void Start();
	void Prologue();
        void RemoteStart();
        void PurgeData();
	void Recompute();
	void Refresh();
        void PopulateIndexes();
	void RemoteEnd();
	void Epilogue();

        // Special handling of the ON STATEMENT MV's initialization
	void PrologueHandleOnStatementMV();
	void EpilogueHandleOnStatementMV();

	void ExecuteIndexStatmenents(CRUSQLDynamicStatementContainer &container, 
								 Lng32 errorCode);

private:
	// The FSM's state definitions
	enum STATES { 

		EX_EPILOGUE = MAIN_STATES_START,                
		EX_PROLOGUE,
                EX_PURGE_DATA,
                EX_REMOTE_START = REMOTE_STATES_START,
		EX_REFRESH, 
		EX_REMOTE_END ,
		EX_RECOMPUTE,
                EX_POPINDEX                
	};

private:
	// Statements specific for audited refresh task executor
	enum SQL_STATEMENT { 
		
		DELETE_MULT_TXN_CTX_TBL = 0,
		NUM_OF_SQL_STMT // should always be last
	};

	CRUSQLDynamicStatementContainer auditRefreshTEDynamicContainer_;
	CRUSQLDynamicStatementContainer *pAuditPopIndexdynamicContainer_;
	CRUSQLDynamicStatementContainer *pAuditAvailableIndeXdynamicContainer_;
	CRUSQLDynamicStatementContainer *pAuditUnavailableIndeXdynamicContainer_;
        CRUSQLDynamicStatementContainer *pAuditToggleOnIndexdynamicContainer_;
        CRUSQLDynamicStatementContainer *pAuditToggleOffIndexdynamicContainer_;
	TInt32 numOfIndexes_;

        // The container of LOCK TABLE statements
	CRUSQLDynamicStatementContainer *pLockTablesTEDynamicContainer_;

	// If we do a recomputation to a multi-txn mv we should delete 
	// its context rows from the context umd table 
	BOOL isDeleteMultiTxnContext_;

        BOOL isPurgedata_;
        BOOL isPopindex_;        
};

#endif
