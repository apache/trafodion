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
#ifndef _RU_SIMPLE_REFRESH_TASK_EXECUTOR_H_
#define _RU_SIMPLE_REFRESH_TASK_EXECUTOR_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuSimpleRefreshTaskExecutor.h
* Description:  Definition of class CRUSimpleRefreshTaskExecutor.
*
* Created:      08/14/2000
* Language:     C++
*
*
******************************************************************************
*/

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor
//
//	This is an abstract class, that is the the parent of the audited and 
//  unaudited executors.
//
//	The class provides the framework for performing the refresh of MV 
//	in a single transaction (or without transaction):
//	(1) Single-delta or multi-delta incremental refresh
//	(2) Recompute
//
//  This class updates the follwing metadata :
//	1. Recompute epoch for the MV when upon recomputation
//	2. MV status when upon initialization.
//
//--------------------------------------------------------------------------//

#include "RuRefreshTaskExecutor.h"
#include "RuSQLDynamicStatementContainer.h"

class REFRESH_LIB_CLASS CRUSimpleRefreshTaskExecutor : 
						public CRURefreshTaskExecutor
{
	//----------------------------------//
	//	Public Members
	//----------------------------------//
private:
	typedef CRURefreshTaskExecutor inherited;

public:
	
	CRUSimpleRefreshTaskExecutor(CRURefreshTask *pParentTask = NULL);
	virtual ~CRUSimpleRefreshTaskExecutor() {}

public:
	// The function has implemetation here,but it must be refined 
	// (alpha refinement) in the derived classes 
	virtual void Init() = 0;

public:
	// These functions serialize/de-serialize the executor's context 
	// for the message communication with the remote server process
	
	// Used in the main process side
	virtual void StoreRequest(CUOFsIpcMessageTranslator &translator);
	virtual void LoadReply(CUOFsIpcMessageTranslator &translator);
	
	// Used in the remote process side
	virtual void LoadRequest(CUOFsIpcMessageTranslator &translator);
	virtual void StoreReply(CUOFsIpcMessageTranslator &translator);

public:
	BOOL IsSingleDeltaRefresh() const 
	{ 
		return isSingleDeltaRefresh_; 
	}

	//----------------------------------//
	//	Protected Members
	//----------------------------------//	
protected:
	typedef CDSPtrList<CDMPreparedStatement> StmtList;

	// Execute a LOCK TABLE statement on the mv
	virtual void LockMV();

	// Execute an INTERNAL REFRESH RECOMPUTE statement
	// May be refined by the child classes
	virtual CDMPreparedStatement *PrepareRecomputeMV();
	virtual void ExecuteRecomputeMV(CDMPreparedStatement *stmt);

	// The method implements the core of the SMD/UMD table updates, 
	// but the child classes must further refine it
	//-- Refinement of a pure virtual
	virtual void FinalMetadataUpdate();

protected:
	// This exception is thrown whenever a run time mv 
	// recomputation is needed
	class NeedRecomputeException : public CRUException {};

protected:
	virtual void LogOpeningMessage();
	
	virtual void LogClosureMessage();

	// Overriding the defualt behavior
	virtual void HandleSqlError(CDSException &ex,
				    Lng32 errorCode,
				    const char *errorArgument=NULL);

	CDMPreparedStatement *PrepareSingleDeltaRefresh();
	void ExecuteSingleDeltaRefresh(CDMPreparedStatement *stmt);

	StmtList *PrepareMultiDeltasRefresh();
	void ExecuteMultiDeltasRefresh(StmtList *stmts);
	
	BOOL CompileMultiDeltasIRefresh(CDMPreparedStatement *&pStat);

	void IncrementNumOfPhases()
	{
	  numOfPhases_++;
	}

	//----------------------------------//
	//	Private Members
	//----------------------------------//	
private:
	// Init() callee
	void ComposeMySql();

	// The number of intenal refresh phases we already executed
	short GetNumOfPhases() const 
	{ 
		return numOfPhases_; 
	}

	BOOL HasWarningCode(CDSException ex,Lng32 errorCode) const;

private:
	//-- Prevent copying
	CRUSimpleRefreshTaskExecutor(const CRUSimpleRefreshTaskExecutor &other);
	CRUSimpleRefreshTaskExecutor &
		operator = (const CRUSimpleRefreshTaskExecutor &other);

private:
	enum SQL_STATEMENT {
		
		INTERNAL_REFRESH = 0,
		RECOMPUTE		 = 1,
		RU_LOCK_TABLE		 = 2,

		NUM_OF_SQL_STMT // should always be last
	};
	
	// The Internal refresh compilation will return this code
	// when more phases are needed in case of a multi_delta refresh
	enum {   MORE_PHASE_CODE                 =  12304
	       , MIN_MAX_RECOMPUTATION_NEEDED    = -12308 
	       , MULTIDELTA_RECOMPUTATION_NEEDED = -12319 
	};

#ifdef NA_LINUX
        // CHECK_RMS_EACH_N_STMT: How often should multi-delta refresh check 
        //      for the current size of the RMS shared memory 
        // PERCENT_OF_RMS_SMEM_USED: percentage of RMS shared memory in this
        //      node that has already been used above which we can no longer
        //      prepare any statements and should raise an error
        enum { CHECK_RMS_EACH_N_STMT             = 10
               ,PERCENT_OF_RMS_SMEM_USED         = 90 
        };
#endif

	CRUSQLDynamicStatementContainer simpleRefreshTEDynamicContainer_;

private:
	// These data members carries information nessesary for the 
	// work of the executor and must be transfered with the executor
	BOOL isSingleDeltaRefresh_;

	short numOfPhases_;
};

#endif
