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
#ifndef _RU_DUP_ELIM_TASK_EXECUTOR_H_
#define _RU_DUP_ELIM_TASK_EXECUTOR_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuDupElimTaskExecutor.h
* Description:  Definition of class CRUDupElimTaskExecutor.
*
* Created:      05/25/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"

#include "RuTaskExecutor.h"
#include "RuDupElimGlobals.h"
#include "RuSQLDynamicStatementContainer.h"

class CRUDupElimTask;
class CRUDupElimLogScanner;
class CRUDupElimSingleRowResolver;
class CRUDupElimRangeResolver;

class CRUDeltaStatisticsMap;
class CRUTbl;

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor
//	
//	This class implements the Duplicate Elimination algorithm.
//
//	Its main purpose is to coordinate between three main units 
//	(instances of classes CRUDupElimLogScanner,
//	CRUDupElimSingleRowResolver, and CRUDupElimRangeResolver).
//  
//	The log scanner performs the reading from the IUD log, and
//	interfaces with the other two units to expose the data read
//	from the log.
//
//	The single row resolver makes the duplicate elimination 
//	decisions concerning the *single-row* records in the IUD log,
//	and translates them into the update/delete operations applied 
//	to these records.
//
//	The range resolver makes the duplicate elimination decisions 
//	concerning the *range* records in the IUD log (range analysis).
//	Upon resolving the conflicts between the ranges, it writes them
//	to the *range* log.
//
//	The task executor class generates the SQL text for the dynamic SQL 
//	statements applied by the three units (through an instance of the 
//	CRUDupElimSQLComposer class), and performs the flow of control.
//
//	Duplicate elimination is a multi-phase algorithm. Every phase runs
//	in a separate transaction, where it scans an (ordered) interval of data
//	in the IUD log. The task executor manages the transaction framing, 
//	in coordination with the resolvers.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUDupElimTaskExecutor : public CRUTaskExecutor
{
private:
	typedef CRUTaskExecutor inherited;

public:
	enum {

		// Intermediate FSM states 

		// Remote execution
		EX_PERFORM_PHASE = REMOTE_STATES_START,	
		// Local execution
		EX_EPILOGUE = MAIN_STATES_START 
	};

public:
	CRUDupElimTaskExecutor(CRUTask *pParentTask=NULL);
	virtual ~CRUDupElimTaskExecutor();

	//----------------------------------//
	//	Accessors
	//----------------------------------//
public:
	// Expose the statistics gathered by DE
	const CRUDeltaStatisticsMap &GetStatisticsMap() const;

	//----------------------------------//
	//	Mutators
	//----------------------------------//
public:
	//-- Implementation of pure virtuals

	//-- Initialize the executor at the main process's side
	virtual void Init();

	//-- Single execution step.
	virtual void Work();

public:
	// These functions serialize/de-serialize the executor's context 
	// for the message communication with the remote server process
	
	// Used in the main process side
	virtual void StoreRequest(CUOFsIpcMessageTranslator &translator);
	virtual void LoadReply(CUOFsIpcMessageTranslator &translator);
	
	// Used in the remote process side
	virtual void LoadRequest(CUOFsIpcMessageTranslator &translator);
	virtual void StoreReply(CUOFsIpcMessageTranslator &translator);

protected:
	// Initial estimate
	enum { 

		SIZE_OF_REQUEST_PACK_BUFFER = 10000,
		FIXED_PART_OF_REPLY_PACK_BUFFER = 1000
	};

	//-- Implementation of pure virtual
	virtual Lng32 GetIpcBufferSize() const;

private:
	//-- Prevent copying
	CRUDupElimTaskExecutor(const CRUDupElimTaskExecutor &other);
	CRUDupElimTaskExecutor &operator = (const CRUDupElimTaskExecutor &other);

private:
	//-- Init() callees
	void InitGlobals(CRUDupElimTask *pParentTask);
	void InitUnits(CRUDupElimTask *pParentTask);
	void InitSQL(CRUDupElimTask *pParentTask);
	void InitTxnTimeLimit();

	//-- The duplicate elimination's engine
	void PerformPhase();

	//-- PerformPhase() callees
	void PrepareSQL();
	void InitPhase();
	void Resolve();
	BOOL CanCompletePhase();

	//-- Write metadata to MVS_TABLE_INFO_UMD
	void ResetDECompleteFlag();
	void FinalMetadataUpdate(); 

#ifdef _DEBUG
	// Print the number of performed statements to the output file
	void DumpPerformanceStatistics();
#endif

private:
	//----------------------------------//
	//	Executor units
	//----------------------------------//
	CRUDupElimLogScanner *pLogScanner_;
	CRUDupElimSingleRowResolver *pSingleRowResolver_;
	CRUDupElimRangeResolver *pRangeResolver_;

	TInt32 txnTimeLimit_;
	Int32 phase_;
	CRUDupElimGlobals globals_;

	// CONTROL QUERY SHAPE/CONTROL TABLE statements
	CRUSQLDynamicStatementContainer ctrlStmtContainer_;
};

#endif
