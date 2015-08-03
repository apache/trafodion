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
#ifndef _RU_MULTI_TXN_REFRESH_TASK_EXECUTOR_H_
#define _RU_MULTI_TXN_REFRESH_TASK_EXECUTOR_H_

/* -
*-C++-*-
******************************************************************************
*
* File:         RuMultiTxnRefreshTaskExecutor.h
* Description:  Definition of class CRUMultiTxnRefreshTaskExecutor.
*
* Created:      08/14/2000
* Language:     C++
*
*
******************************************************************************
*/

//--------------------------------------------------------------------------//
// This class handle the refresh of a multi transaction audited mv. 
// The class implements a finite state machine where some states may be 
// executed in a different process.
//
// The executor is handled to recover from multiple failures (process crash).
// For each such failure we keep a "water mark" that signs the maximal  
// clustering index (CLI) of that table that we have already read (This is   
// done by the internal refresh in a special mode ,see Internal refresh 
// design document)
//
// The process of recovering from a multiple failure is done by executing 
// intrenal refresh on a boundaries that are set by those water marks.After 
// such execution we can join the two "water marks" to a single water mark.
// We keep on going until there is a single water mark ,so we can just  
// continue from that CLI and process the rest of the table. 
// 
// The executor must also keeps track on the refresh progress for knowing how 
// to  activate the internal refresh command after a failure.
// Therefore the executor will read the context log in the begining of every
// execution. In the next stages the executor will imitate the IR context
// updates in the memory using the class RuMultiTxnContext.
// In this way, the context will have an exact representation of the rows in
// the context table.
//
// The class has the following states :
//
//	1.EX_PROLOGUE	 - Starts a transaction,Reads the context 
//			   from the umd table and insert a new context row
//	2.EX_NO_BOUNDS	 - Execute an internal refresh command with no bounds
//	3.EX_LOWER_BOUND - Execute an internal refresh command with a minimal CLI
//	4.EX_UPPER_BOUND - Execute an internal refresh command with a maximal CLI
//	5.EX_BOTH_BOUND	 - Execute an internal refresh command between two CLI's
//	6.EX_REMOTE_END	 - Unlocks any used tables if needed
//	7.EX_EPILOGUE	 - Update the metadata , delete all context rows from 
//			   the umd table and commits the transaction
//
//--------------------------------------------------------------------------//

#include "RuRefreshTaskExecutor.h"
#include "RuSQLDynamicStatementContainer.h"
#include "RuMultiTxnContext.h"

class CRUTbl;
class CDSStringList;
class CRUEmpCheck;
class CRUEmpCheckVector;

class REFRESH_LIB_CLASS CRUMultiTxnRefreshTaskExecutor : 
		public CRURefreshTaskExecutor {

private:
	typedef CRURefreshTaskExecutor inherited;

	//----------------------------------//
	//	Public Members
	//----------------------------------//	
public:
	CRUMultiTxnRefreshTaskExecutor(CRURefreshTask *pParentTask = NULL);
	virtual ~CRUMultiTxnRefreshTaskExecutor();

public:
	//-- Implementation of pure virtuals	
	virtual void Work();
	
	virtual void Init();
	
public:
	// These functions serialize/de-serialize the executor's context 
	// for the message communication with the remote server process
	
	// Used in the main process side
	virtual void StoreRequest(CUOFsIpcMessageTranslator &translator);
	virtual void LoadReply(CUOFsIpcMessageTranslator &translator);
	
	// Used in the remote process side
	virtual void LoadRequest(CUOFsIpcMessageTranslator &translator);
	virtual void StoreReply(CUOFsIpcMessageTranslator &translator);


	//----------------------------------//
	//	Protected Members
	//----------------------------------//	

protected:
	virtual void LogOpeningMessage();
	
	virtual void LogClosureMessage();


	//-- Refinement of a pure virtual
	virtual void FinalMetadataUpdate()
	{
		inherited::FinalMetadataUpdate();
	}

	// We override this function for the purpose of counting 
	// the number of Txns that we have already completed
	virtual void CommitTransaction(BOOL countIt=TRUE)
	{
		inherited::CommitTransaction();
		if (countIt)
		{
		      txnCounter_++;
		}
	}

	// Update the MULTI_TXN_TARGET_COLUMN in MVS_UMD table
	void UpdateTargetEpoch(TInt32 epoch);

	//----------------------------------//
	//	Private Members
	//----------------------------------//	

private:
	
	enum SQL_STATEMENT {  NO_BOUNDS_REFRESH =0,   // PHASE 0 
			      LOWER_BOUND_REFRESH ,   // PHASE 1
			      UPPER_BOUND_REFRESH ,   // PHASE 0 CATCHUP
			      BOTH_BOUND_REFRESH  ,   // PHASE 1 CATCHUP
			      READ_CONTEXT_LOG,	      // read context log table
			      CQS_PHASE1,	      // a CQS statment for IR phase 1
			      NUM_OF_SQL_STMT	      // should always be last
	};
	
	enum SQL_STATIC_STATMENTS { UPDATE_CTX = 0 // update target epoch in MVS_UMD
	};

	enum PHASES {	PHASE_0 = 0,
			PHASE_1 = 1,
			NEED_CATCHUP = TRUE,
			DONT_NEED_CATCHUP = FALSE
		    };
	// error code recieved from Internal Refresh when no more TXN are needed
	enum { NO_MORE_TXN_CODE_ERROR = -12316};

private:
	
	// Init()  callee function
	void ComposeMySql();

private:

	// Work() callee functions
	void Start();
	void Prologue();
	void Epilogue();
	void NoBounds();
	void LowerBound();
	void UpperBound();
	void BothBounds();
	void RemoteEnd();
	
private:	
	
	// Compile the refresh statement returns TRUE when the refresh ends
	CDMPreparedStatement * CompileRefresh(SQL_STATEMENT type, 
					      TInt32 beginEpoch , 
					      TInt32 endEpoch   ,
					      TInt32 catchupEpoch=0);

	// Execute internal refresh until 
	// a no more txn needed error is thrown
	// This function is called by BothBounds() and LowerBound()
	void DoIRefreshUntilIDone(SQL_STATEMENT type);
	
	// This function verifies that the only error that occured is 
	// NO_MORE_TXN_CODE_ERROR otherwise it ReThrow the exception
	void VerifyMultiTxnError(CDSException &ex);

	enum MoreTxnStatus {  
		NO_MORE_TXN_NEEDED = FALSE,
		MORE_TXN_NEEDED = TRUE
	};
	
	// overriding CRURefreshTaskExecutor::ApplyCQSForInternalRefresh()
	void ApplyCQSForInternalRefresh();


private:

	enum	STATES {   
		// Local States
		EX_EPILOGUE = MAIN_STATES_START ,
		// Remote States
		EX_PROLOGUE = REMOTE_STATES_START ,
		EX_NO_BOUNDS,
		EX_LOWER_BOUND,
		EX_UPPER_BOUND,
		EX_BOTH_BOUND,
		EX_REMOTE_END
	};

private:

	//-- Prevent copying
	CRUMultiTxnRefreshTaskExecutor(const CRUMultiTxnRefreshTaskExecutor &other);
	CRUMultiTxnRefreshTaskExecutor &
						operator = (const CRUMultiTxnRefreshTaskExecutor &other);

private:
	
	CRUSQLDynamicStatementContainer multiTxnRefreshTEDynamicContainer_;

	// The following data is common for all Work() states
	
	TInt32							endEpoch_;
	TInt32							beginEpoch_;
	TInt32							catchupEpoch_;
	// If we are in a catchup mode , multiTxnTargetEpoch_ will not be zero
	TInt32							multiTxnTargetEpoch_;

	// The number of transactions we already completed
	short							txnCounter_;

	// Stack with context lines from the UMD table
	CRUMultiTxnContext				context_;
	// Emptiness check is done for completing the lock protocol
};

#endif
