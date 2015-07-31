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
#ifndef _RU_REFRESH_TASK_EXECUTOR_H_
#define _RU_REFRESH_TASK_EXECUTOR_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuRefreshTaskExecutor.h
* Description:  Definition of class CRURefreshTaskExecutor.
*
*				
* Created:      06/04/1999
* Language:     C++
*
*
******************************************************************************
*/

//--------------------------------------------------------------------------//
//	This is an abstract class ,that is the root of all refresh executors.
//	The class contain common data that is used for executing the internal 
//	refersh command, and implements most of the metadata updates that are 
//	executed in the end of the mv refresh process.
//
//   This class updates the follwing metadata :
//	1. Refresh duration - used for later optimization by the schedular 
//	2. Refresh at timestamp - sets the time of the last refresh
//	3. Epoch Vector - The current position in the log consumption for 
//	   every table
//	4. If the mv is also an involved table we increase the current epoch by 1
//
//--------------------------------------------------------------------------//

#include "refresh.h"
#include "RuEmpCheck.h"
#include "RuTaskExecutor.h"
#include "RuRefreshTask.h"
#include "RuSQLDynamicStatementContainer.h"
#include "RuForceOptions.h"

class CRUMV;
class CRUTableLockProtocol;

class REFRESH_LIB_CLASS CRURefreshTaskExecutor : public CRUTaskExecutor
{

private:
	typedef CRUTaskExecutor inherited;

	//----------------------------------//
	//	Public Members
	//----------------------------------//	
public:
	CRURefreshTaskExecutor(CRUTask *pParentTask = NULL);
	virtual ~CRURefreshTaskExecutor();

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
	CRURefreshTask* GetRefreshTask() 
	{ 
		return (CRURefreshTask *)GetParentTask(); 
	}

	const CDSString& GetRootMVName() const 
	{ 
		return rootMVName_; 
	}

	TInt64 GetRootMVUID() const 
	{ 
		return rootMVUID_; 
	}
	
	const CDSString& GetRootMVSchema() const 
	{ 
		return rootMVSchema_; 
	}
	
	const CDSString& GetRootMVCatalog() const 
	{ 
		return rootMVCatalog_; 
	}
	
	CDDObject::EMVRefreshType GetRootMVType() const
	{
		return rootMVType_;
	}

	virtual BOOL IsRecompute() const 
	{ 
		return isRecompute_; 
	}

	//----------------------------------//
	//	Protected Members
	//----------------------------------//	
protected:
	virtual void LogOpeningMessage() = 0;
	
	virtual void LogClosureMessage() = 0;

protected:

	// The method implements the core of the SMD/UMD table updates, 
	// but the child classes must further refine it
	virtual void FinalMetadataUpdate() = 0;

	// Update the refresh MV current epoch when it is also
	// used by invloved on request mvs
	void IncrementTopMvCurrentEpoch();

protected:
	// This function prepare the common message for the 
	// various refresh executors
	void GenOpeningMessage(CDSString &msg);
	void GenClosureMessage(CDSString &msg);

protected:
	CRUMV &GetRootMV() 
	{
		return GetRefreshTask()->GetRootMV();
	}

	void SetRecompute(BOOL flag) 
	{ 
		isRecompute_ = flag; 
	}

	enum { SIZE_OF_PACK_BUFFER = 2000 };

	//-- Implementation of pure virtual 
	virtual Lng32 GetIpcBufferSize() const
	{
		return SIZE_OF_PACK_BUFFER; // Initial size 
	}

	Int32 GetForceFlags() { return forceFlags_; }

protected:
	
	enum FORCE_FLAGS {	NO_FORCE = 0, 
						FORCE_CQS = 1, 
						FORCE_MV_MDAM = 2,
						FORCE_TABLE_MDAM = 4,
						FORCE_USER_CQS = 8,
						FORCE_SHOW_EXPLAIN = 16
	};

	void ApplyIRCompilerDefaults();
	void ResetIRCompilerDefaults();
	virtual void ApplyCQSForInternalRefresh();
	void ApplyUserCQSForInternalRefresh();
	void ExecuteShowExplain();
	void SetExplainGeneration();
	void SetAllowOfflineAccess();

	// Returns FALSE if the main process lost its locks.
	NABoolean StartTableLockProtocol();
	void EndTableLockProtocol();

	//----------------------------------//
	//	Private Members
	//----------------------------------//	
private:
	//-- Prevent copying
	CRURefreshTaskExecutor(const CRURefreshTaskExecutor &other);
	CRURefreshTaskExecutor &operator = (const CRURefreshTaskExecutor &other);

private:
	// For the root mv only we need to update the current epoch vector
	void UpdateRootMVCurrentEpochVector();

	// The refresh duration and delta size heuristics
	void UpdateRootMVStatistics(TInt32 currDuration);
	
	// For all other mv's that where refreshed (pipeline)
	// The statistics also needs to be updated
	void UpdateNonRootMVStatistics(CRUMV *pCurrMV, 
								   CRUMV *pPrevMV,
								   TInt32 currDuration);
private:
	// For a single delta refresh we use special calculations
	// for the statistics
	void UpdateSingleDeltaStatistics(CRUDeltaDef *pDdef, 
									 TInt32 currDuration);

	// returns the timestamp of the refreshed mv. 
	// If the mv has a defined timestamp (the used tables where 
	// syncronized ) we take the minimum ts otherwise we take the 
	// current time
	TInt64 CalculateTimeStamp();
private:
	// CalculateTimeStamp() callee
	static TInt32 EvaluateHeuristic(TInt32 curr, TInt32 prev);

	//	If the task must perform a runtime consistency check,
	//	this means that the MV which is based on a single MV
	//	and one or more tables is refreshed standalone. In this
	//	context, the deltas of all the regular tables must be 
	//	empty.
	void CheckSingleDeltaRestriction();

private:
	enum SQL_STATEMENT {MV_MDAM_STAT = 0,
						CQS_STAT,
						CQS_CUT_STAT,
						SHOW_EXPLAIN,
						USER_CQS_FORCE,
						RESET_MDAM_STAT,
						FIRST_TBL_STAT
	};



	// Init() callee
	void ComposeMySql();

private:	
	// ComposeMySql() callee
	void ComposeForceStatements();

private:
	// ComposeForceStatements() callee
	void ComposeControlTableStmtForUsedTables();
	void ComposeControlTableStmtForUsedTable(CRUTbl &tbl,
											 Int32 &stmtIndex,
											 CRUForceOptions::MdamOptions mdamOpt);
	void ComposeCQSStatements();
	void ComposeMDAMStatements();

	CRUForceOptions::MdamOptions GetDefaultMdamOptForTable(CRUTbl &tbl);

	CRUTableForceOptions* GetForceOptionForTable(const CDSString &name);

	void VerifyForceTblListCorrectness();

protected:
	CRUTableLockProtocol		*tableLockProtocol_;

private:
	CDSString			 rootMVName_;
	CDDObject::EMVRefreshType	 rootMVType_;
	TInt64				 rootMVUID_;
	CDSString			 rootMVSchema_;
	CDSString			 rootMVCatalog_;
	BOOL				 isRecompute_;
	TInt32				 numOfStmtInContainer_;
	CRUSQLDynamicStatementContainer *pRefreshTEDynamicContainer_;
	Int32				 forceFlags_;

};


//--------------------------------------------------------------------------//
// When the used table of the mv has a automatic range log or a mixed range 
// log the refresh process is using both the log and the table,so in order to 
// maintain consistency we need to lock the table in addition to the epoch 
// increment.Usually it is done by the main process prior to the execution of 
// the refresh execution,however in multi-txn refresh due to the fact that we
// are commiting the transactions in the remote process we cannot know if the
// main process have failed (and therefor has lost his locks on the table) 
// before we commit.In order to solve this we take a second share lock by the
// remote process and immediately after we check if there are new records 
// in the log after the epoch increment.If there are such records it means 
// that the main must have failed and we should abort ,otherwise we keep the 
// locks until the refresh process is done and the executor returns to the
// main for updating the SMD tables
//--------------------------------------------------------------------------//

class CRUEmpCheck;

class REFRESH_LIB_CLASS CRUSingleTableLockProtocol {

public:
  CRUSingleTableLockProtocol();
  virtual ~CRUSingleTableLockProtocol();

public:
  void Init(CRUTbl *pTbl, TInt32 EndEpoch);

  BOOL IsEnsureUsedTableLockContinuityNeeded() const
  {
    return NULL != pUsedTblFileNameList_;
  }

  // called by work() in the remote process
  BOOL EnsureUsedTableLockContinuity();

  void StoreData(CUOFsIpcMessageTranslator &translator);
  void LoadData(CUOFsIpcMessageTranslator &translator);

  void UnLockUsedTablePartitions();

  void StoreDataFileNameList(CUOFsIpcMessageTranslator &translator);
  void LoadDataFileNameList(CUOFsIpcMessageTranslator &translator);

  // These functions are called when the underlined table has an 
  // automatic or mixed range log attribute and therefor must be locked

  // called by init() in the main process
  void CopyPartitionFileNames(CRUTbl *pTbl);
	
  // The emptiness check for the locking protocol as explained above is done 
  // by the empCheck class (a reusable componenet). 
  // called by init() in the main process
  void BuildEmpCheck(CRUTbl *pTbl);

  // EnsureUsedTableLockContinuity() callee
  void LockUsedTablePartitions();
	
  BOOL AnyNewRecordsSinceEpochIncrement();

private:
  //-- Prevent copying
  CRUSingleTableLockProtocol(const CRUSingleTableLockProtocol &other);
  CRUSingleTableLockProtocol &operator = (const CRUSingleTableLockProtocol &other);

private:
	
  CDSStringList	  *pUsedTblFileNameList_;

  CUOFsFileList	  *pPartitionFileList_;

  // Emptiness check is done for completing the lock protocol
  CRUEmpCheck	  *pEmpCheck_;

  TInt32	   endEpoch_;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUTableLockProtocol {

public:
  CRUTableLockProtocol() :
      LocksList_(NULL),
      NothingToLock_(TRUE)
  {
  }

  virtual ~CRUTableLockProtocol()
  {
    if (NULL != LocksList_)
    delete LocksList_;
  }

public:
  void Init(CRUTblList &tblList, CRUDeltaDefList &DeltaDefList);

  BOOL IsEnsureUsedTableLockContinuityNeeded() const;

  // called by work() in the remote process
  BOOL EnsureUsedTableLockContinuity();

  void StoreData(CUOFsIpcMessageTranslator &translator);
  void LoadData(CUOFsIpcMessageTranslator &translator);

  void UnLockUsedTablePartitions();

private:
  //-- Prevent copying
  CRUTableLockProtocol(const CRUTableLockProtocol &other);
  CRUTableLockProtocol &operator = (const CRUTableLockProtocol &other);

private:
  typedef CDSPtrList<CRUSingleTableLockProtocol> LocksList;

  LocksList                  *LocksList_;
  BOOL                        NothingToLock_;
};

#endif
