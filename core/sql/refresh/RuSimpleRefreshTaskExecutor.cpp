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
* File:         RuSimpleRefreshTaskExecutor.cpp
* Description:  Implementation of class CRUSimpleRefreshTaskExecutor.
*				
*
* Created:      08/14/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuSimpleRefreshTaskExecutor.h"
#include "RuSimpleRefreshSQLComposer.h"

#include "RuMV.h"
#include "RuTbl.h"
#include "RuGlobals.h"
#include "RuJournal.h"
#include "uofsIpcMessageTranslator.h"

//--------------------------------------------------------------------------//
//	Constructor and destructor
//--------------------------------------------------------------------------//

CRUSimpleRefreshTaskExecutor::
CRUSimpleRefreshTaskExecutor(CRURefreshTask *pParentTask) :
	inherited(pParentTask),
	isSingleDeltaRefresh_(TRUE),
	simpleRefreshTEDynamicContainer_(NUM_OF_SQL_STMT),
	numOfPhases_(0)
{}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::StoreRequest()
//--------------------------------------------------------------------------//
void CRUSimpleRefreshTaskExecutor::
	StoreRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::StoreRequest(translator);

	// Handle isSingleDeltaRefresh_ data member
	translator.WriteBlock(&isSingleDeltaRefresh_,sizeof(BOOL));
	
	// Handle refresh executor sql dynamic container
	simpleRefreshTEDynamicContainer_.StoreData(translator);
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::StoreReply()
//--------------------------------------------------------------------------//
void CRUSimpleRefreshTaskExecutor::
	StoreReply(CUOFsIpcMessageTranslator &translator)
{
	inherited::StoreReply(translator);

	// Handle numOfPhases_ data member
	translator.WriteBlock(&numOfPhases_,sizeof(short));
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::LoadRequest()
//--------------------------------------------------------------------------//
void CRUSimpleRefreshTaskExecutor::
	LoadRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::LoadRequest(translator);

	// Handle isSingleDeltaRefresh_ data member
	translator.ReadBlock(&isSingleDeltaRefresh_,sizeof(BOOL));

	// Handle refresh executor sql dynamic container
	simpleRefreshTEDynamicContainer_.LoadData(translator);
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::LoadReply()
//--------------------------------------------------------------------------//
void CRUSimpleRefreshTaskExecutor::
	LoadReply(CUOFsIpcMessageTranslator &translator)
{
	inherited::LoadReply(translator);

	// Handle numOfPhases_ data member
	translator.ReadBlock(&numOfPhases_,sizeof(short));
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::Init()
//
//	Initialize data members by analyzing the RefreshTask
//--------------------------------------------------------------------------//

void CRUSimpleRefreshTaskExecutor::Init()
{
	inherited::Init();

	RUASSERT(TRUE == HasWork()); 

	if (FALSE == GetRefreshTask()->NeedToExecuteInternalRefresh())
	{
		// This is an incremental refresh that does not have any delta
		// We do nothing here and the proper treatment is done in the 
		// derived classes
		return;
	}

	CRURefreshTask *pRefreshTask = GetRefreshTask();

	isSingleDeltaRefresh_ = pRefreshTask->IsSingleDeltaRefresh();

	ComposeMySql();
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::ComposeMySql()
//--------------------------------------------------------------------------//

void CRUSimpleRefreshTaskExecutor::ComposeMySql()
{
	CRUSimpleRefreshSQLComposer myComposer(GetRefreshTask());
	
	BOOL isRecompute = IsRecompute();

	if (FALSE == isRecompute) 
	{
		// THE INTERNAL REFRESH (incremental) statement
		myComposer.ComposeRefresh();
		simpleRefreshTEDynamicContainer_.SetStatementText(
			INTERNAL_REFRESH,myComposer.GetSQL());
	}

	// Always compose the INTERNAL REFRESH RECOMPUTE syntax
	// (because we might revert to it during the MIN/MAX refresh)
	if (0 != (GetRootMV().GetRefreshPatternMap() & CRUMV::NODELETE))
	{
		RUASSERT(TRUE == isRecompute);
		myComposer.ComposeRecompute(CRUSimpleRefreshSQLComposer::REC_NODELETE);
	}
	else
	{
		// We must delete all data from the MV prior to the recomputation
		myComposer.ComposeRecompute(CRUSimpleRefreshSQLComposer::REC_DELETE);
	}
		
	simpleRefreshTEDynamicContainer_.SetStatementText(
		RECOMPUTE,myComposer.GetSQL());


	// The LOCK TABLE ... statement 
	myComposer.ComposeLock(GetRootMVName(),TRUE);
	simpleRefreshTEDynamicContainer_.SetStatementText(
		RU_LOCK_TABLE,myComposer.GetSQL());
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::HandleSqlError()
//
// Overriding the defualt behavior
//
// If we receive MIN_MAX_RECOMPUTATION_NEEDED sql code,
// it means that we need to perform a full recomputation of this MV 
// (an MV with min/max on non insert only tables may require this method).
//
//--------------------------------------------------------------------------//
void CRUSimpleRefreshTaskExecutor::HandleSqlError(CDSException &ex,
												  Lng32 errorCode,
												  const char *errorArgument)
{
	if (ex.GetErrorCode(0) == MIN_MAX_RECOMPUTATION_NEEDED)
	{
		throw NeedRecomputeException();
	}

	inherited::HandleSqlError(ex,errorCode,errorArgument);
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::PrepareRecomputeMV()
//
//	The base for recomputing an MV
//--------------------------------------------------------------------------//

CDMPreparedStatement *CRUSimpleRefreshTaskExecutor::PrepareRecomputeMV()
{
	ApplyIRCompilerDefaults();

	CDMPreparedStatement *pStat = 
	  simpleRefreshTEDynamicContainer_.GetPreparedStatement(RECOMPUTE);
	 
	ResetIRCompilerDefaults();

	ExecuteShowExplain();

	return pStat;
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::ExecuteRecomputeMV()
//
//	The base for recomputing an MV
//--------------------------------------------------------------------------//

void CRUSimpleRefreshTaskExecutor::ExecuteRecomputeMV(CDMPreparedStatement *pStat)
{
	ExecuteStatement(*pStat,IDS_RU_IREFRESH_FAILED);

	SetRecompute(TRUE);
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::PrepareSingleDeltaRefresh()
//
//	Compile a single-delta INTERNAL REFRESH
//	(Requires only a single phase).
//--------------------------------------------------------------------------//

CDMPreparedStatement *CRUSimpleRefreshTaskExecutor::PrepareSingleDeltaRefresh()
{
	LOGTIME("Starting to compile single-delta refresh \n");

	CDMPreparedStatement *pStat = 
	  simpleRefreshTEDynamicContainer_.GetPreparedStatement(INTERNAL_REFRESH);

	return pStat;
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::ExecuteSingleDeltaRefresh()
//
//	Execute a single-delta INTERNAL REFRESH
//	(Requires only a single phase).
//--------------------------------------------------------------------------//

void CRUSimpleRefreshTaskExecutor::ExecuteSingleDeltaRefresh(CDMPreparedStatement *pStat)
{
	LOGTIME("Starting to execute single-delta refresh \n");

	ExecuteStatement(*pStat,IDS_RU_IREFRESH_FAILED);
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::PrepareMultiDeltasRefresh()
//
//	Compile all phases of multi-delta INTERNAL REFRESH.
//
//--------------------------------------------------------------------------//

CRUSimpleRefreshTaskExecutor::StmtList *CRUSimpleRefreshTaskExecutor::PrepareMultiDeltasRefresh()
{
	// Compilation Stage
	LOGTIME("Starting to compile multi-delta refresh \n");

	CRUSimpleRefreshTaskExecutor::StmtList *stmts = new StmtList();

#ifdef NA_LINUX
        Int32 nid = 0;
        // get the node id of this process
        if (XZFIL_ERR_OK != msg_mon_get_my_info(&nid, NULL, NULL, 0, NULL, NULL, NULL, NULL))
        {
             CRUException ex;
             ex.SetError(IDS_RU_UNEXPECTED);
             ex.AddArgument("Call to <msg_mon_get_my_info> to get node id failed.");
             throw ex;      
        }
#endif

	// Get the statement before compiling it.
	CDMPreparedStatement *pStat = NULL;
	BOOL needMorePhases = TRUE;
	while (TRUE == needMorePhases)
	{
           needMorePhases = CompileMultiDeltasIRefresh(pStat);
           stmts->AddTail(pStat);
           numOfPhases_++;


#ifdef NA_LINUX
           // check the RMS shared memory after every CHECK_RMS_EACH_N_STMT statements
           if (numOfPhases_%CHECK_RMS_EACH_N_STMT == 0)
           {
             Int32 heapUsed=0;
             Int32 heapTotal=0;
         
             pStat->getRmsHeapStats(nid, heapUsed, heapTotal);
         
             // round up the percentage number
             float percentOfRMSHeapUsed = (heapUsed*100.0/heapTotal)+0.5;
             // raise an error an bail out if we already used more than the allowed RMS shared memory
             if (percentOfRMSHeapUsed > PERCENT_OF_RMS_SMEM_USED)
             {
                CRUException ex;
                ex.SetError(IDS_RU_EXHAUSTED_RMS_SHARED_MEM);
                ex.AddArgument((Int32)(percentOfRMSHeapUsed));
                ex.AddArgument(numOfPhases_);
                throw ex;      
             }
           }
#endif

	}

	return stmts;
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::ExecuteMultiDeltasRefresh()
//
//	Execute all phases of multi-delta INTERNAL REFRESH.
//
//--------------------------------------------------------------------------//

void CRUSimpleRefreshTaskExecutor::ExecuteMultiDeltasRefresh(StmtList *stmts)
{
	// Execution stage
	LOGTIME("Starting to execute multi-delta refresh \n");

	CDMPreparedStatement *pStat = NULL;

	DSListPosition pos = stmts->GetHeadPosition();
	while (pos != NULL)
	{
		pStat = stmts->GetNext(pos);
		ExecuteStatement(*pStat,IDS_RU_IREFRESH_FAILED);
	}

	// This is probably a memory leak, but this delete causes a crash
	// in the destructor when deleting the statement container.
	//delete stmts;
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::CompileMultiDeltasIRefresh()
//
// If we recieve MIN_MAX_RECOMPUTATION_NEEDED sql code ,it means that we 
// need to make a full recomputation of this mv ( mv that has min/max on non 
// insert only tables may require this method).
// Another case is MULTIDELTA_RECOMPUTATION_NEEDED, which means its a multi-
// delta MJV that is just too big, so instead of risking an optimizer crash,
// we just recompute (which will probably be faster than executing such a 
// huge tree anyway).  
//
// When more phases are needed we recieve a warning code after the
// compilation was done
//--------------------------------------------------------------------------//

BOOL CRUSimpleRefreshTaskExecutor::
	 CompileMultiDeltasIRefresh(CDMPreparedStatement *&pStat)
{
	simpleRefreshTEDynamicContainer_.SetCompiledTimeParam
		(INTERNAL_REFRESH,0,CRUSQLComposer::TInt32ToStr(numOfPhases_));

	try {
	  // The FALSE is to avoid deleting used statements.
	  // They will be deleted later in the list.
		pStat = simpleRefreshTEDynamicContainer_.
						GetPreparedStatement(INTERNAL_REFRESH, FALSE);
	}
	catch (CDSException &ex)
	{
		if (ex.GetErrorCode(0) == MIN_MAX_RECOMPUTATION_NEEDED   ||
		    ex.GetErrorCode(0) == MULTIDELTA_RECOMPUTATION_NEEDED )
		{
			// In case we need a multi phase refresh and we have min-max
			// columns we just do a recomputation instead
			throw NeedRecomputeException();
		}
		
		ex.SetError(IDS_RU_IREFRESH_FAILED);
		ex.AddArgument(simpleRefreshTEDynamicContainer_.
							GetLastSQL(INTERNAL_REFRESH));
		throw ex;	// Re-throw
	}

	if (TRUE == pStat->HasWarning() )
	{
		return HasWarningCode(pStat->GetWarningException(), MORE_PHASE_CODE);
	}
	else
	{
		return FALSE;
	}
}


//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::HasWarningCode()
//
//--------------------------------------------------------------------------//

BOOL CRUSimpleRefreshTaskExecutor::HasWarningCode(CDSException ex,Lng32 errorCode) const
{
	for (Int32 i=0;i<ex.GetNumErrors();i++)
	{
		if (ex.GetErrorCode(i) == errorCode)
		{
			return TRUE;
		}
	}
	return FALSE;
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::FinalMetadataUpdate()
//
//	Refine the parent class's metadata update for a task
//	that performed a recompute.
//
//	(1) After a successful refresh, the MV's status 
//	    must become INITIALIZED.
//	(2) If the MV is ON REQUEST, update the value of
//		MV.RECOMPUTE_EPOCH.
//
//--------------------------------------------------------------------------//

void CRUSimpleRefreshTaskExecutor::FinalMetadataUpdate()
{
	inherited::FinalMetadataUpdate();

	CRURefreshTask *pParentTask = GetRefreshTask();

	if (FALSE == IsRecompute())
	{
		// The recompute can be either expected (by the task)
		// or unexpected (the refresh started as incremented,
		// but finished by recompute). In both cases, check
		// the executor, rather than the parent task.
		return;
	}

	CRUMV &rootMV = GetRootMV();
	if (CDDObject::eINITIALIZED != rootMV.GetMVStatus())
	{
		RUASSERT(CDDObject::eNON_INITIALIZED == rootMV.GetMVStatus() ||
			     CDDObject::eUNAVAILABLE == rootMV.GetMVStatus());

		rootMV.SetMVStatus(CDDObject::eINITIALIZED);
		rootMV.SaveMetadata();
	}

	if (CDDObject::eON_REQUEST == rootMV.GetRefreshType())
	{
		CRUTbl *pTbl = rootMV.GetTblInterface();
		RUASSERT(NULL != pTbl);

		// Set MV.RECOMPUTE_EPOCH <-- MV.CURRENT_EPOCH
		if (TRUE == rootMV.IsInvolvedTbl())
		{
			// If the mv is also an involved mv, the epoch was already 
			// incremented by inherited::FinalMetadataUpdate()
			pTbl->SetRecomputeEpoch(pTbl->GetCurrentEpoch() - 1);
		}
		else
		{
			pTbl->SetRecomputeEpoch(pTbl->GetCurrentEpoch());
		}
		pTbl->SaveMetadata();
	}
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::LogOpeningMessage()
//--------------------------------------------------------------------------//

void CRUSimpleRefreshTaskExecutor::LogOpeningMessage() 
{
	CDSString msg;

	GenOpeningMessage(msg);

	if (TRUE == IsRecompute())
	{
		msg += RefreshDiags[10];
	}
	
	msg += RefreshDiags[11] + CDSString("...\n");
	
	CRUGlobals::GetInstance()->GetJournal().LogMessage(msg);
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::LockMV()
//
// Execute a lock table sql statement on the mv
// We don't need to implement here an unlock statement because except for 
// unaudited mv the lock will be removed in the end of the transaction by 
// itself
//--------------------------------------------------------------------------//

void CRUSimpleRefreshTaskExecutor::LockMV()
{
	CDMPreparedStatement *pStat = simpleRefreshTEDynamicContainer_.
						GetPreparedStatement(RU_LOCK_TABLE);

	ExecuteStatement(*pStat,IDS_RU_LOCK_TABLE_FAILED);
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshTaskExecutor::LogClosureMessage()
//--------------------------------------------------------------------------//
	
void CRUSimpleRefreshTaskExecutor::LogClosureMessage()
{
  CDSString msg;

  GenClosureMessage(msg);  // MV ... has been refreshed
  
  if (TRUE == IsRecompute())
  {
    msg += RefreshDiags[10]; // (by recompute)
  }
  else
  {
    // Phases == 0 means MV was up to date.
    if (GetNumOfPhases() > 0)
    {
      if (IsSingleDeltaRefresh())
      {
	msg += RefreshDiags[19]; // from a single delta
      }
      else
      {
	Lng32 deltas = GetRefreshTask()->GetDeltaDefList().GetCount();

	// from ... deltas
	msg += RefreshDiags[20]; 
	msg += CRUSQLComposer::TInt32ToStr(deltas); 
	msg += RefreshDiags[21];

	if (GetNumOfPhases() > 1)
	{
	  // (in ... phases)
	  msg += RefreshDiags[12]; 
	  msg += CRUSQLComposer::TInt32ToStr(GetNumOfPhases()); 
	  msg += RefreshDiags[13];
	}
      }
    }
  }

  if (TRUE == GetRefreshTask()->NeedToExecuteInternalRefresh())
  {
    // in a single transaction
    msg += RefreshDiags[11];
  }

  msg += CDSString(".\n");
  
  CRUGlobals::GetInstance()->GetJournal().LogMessage(msg);
}
