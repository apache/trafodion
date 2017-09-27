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
* File:         RuRefreshTaskExecutor.cpp
* Description:  Implementation of class CRURefreshTaskExecutor.
*				
*
* Created:      04/06/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "dsstringlist.h"
#include "uofsIpcMessageTranslator.h"
#include "uofsSystem.h"
#include "RuRefreshTaskExecutor.h"
#include "RuRefreshTask.h"
#include "RuRefreshSQLComposer.h"
#include "RuMV.h"
#include "RuTbl.h"
#include "RuDeltaDef.h"
#include "RuForceOptions.h"
#include "RuJournal.h"
#include "RuKeyColumn.h"
#include "dmconnection.h"
//--------------------------------------------------------------------------//
//	Constructor
//--------------------------------------------------------------------------//

CRURefreshTaskExecutor::CRURefreshTaskExecutor(CRUTask *pParentTask) :
	inherited(pParentTask),
	rootMVName_(""),
	rootMVSchema_(""),
	rootMVCatalog_(""),
	rootMVUID_(0),
	rootMVType_(CDDObject::eRECOMPUTE),
	isRecompute_(FALSE),
	numOfStmtInContainer_(0),
	pRefreshTEDynamicContainer_(NULL),
	forceFlags_(NO_FORCE),
	tableLockProtocol_(NULL)
{}

CRURefreshTaskExecutor::~CRURefreshTaskExecutor() 
{
	delete pRefreshTEDynamicContainer_;

	if (NULL != tableLockProtocol_)
	  delete tableLockProtocol_;
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::StoreRequest()
//--------------------------------------------------------------------------//
void CRURefreshTaskExecutor::
	StoreRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::StoreRequest(translator);

	translator.WriteBlock(&isRecompute_,sizeof(BOOL));

	Int32 stringSize = rootMVName_.GetLength() + 1;
	translator.WriteBlock(&stringSize, sizeof(Int32));
#pragma nowarn(1506)   // warning elimination 
	translator.WriteBlock(rootMVName_.c_string(), stringSize);
#pragma warn(1506)  // warning elimination 

	stringSize = rootMVSchema_.GetLength() + 1;
	translator.WriteBlock(&stringSize, sizeof(Int32));
#pragma nowarn(1506)   // warning elimination 
	translator.WriteBlock(rootMVSchema_.c_string(), stringSize);
#pragma warn(1506)  // warning elimination 

	stringSize = rootMVCatalog_.GetLength() + 1;
	translator.WriteBlock(&stringSize, sizeof(Int32));
#pragma nowarn(1506)   // warning elimination 
	translator.WriteBlock(rootMVCatalog_.c_string(), stringSize);
#pragma warn(1506)  // warning elimination 

	translator.WriteBlock(&rootMVUID_, sizeof(TInt64));

	translator.WriteBlock(&rootMVType_, sizeof(Int32));

	translator.WriteBlock(&forceFlags_, sizeof(TInt32));

	if (NO_FORCE != forceFlags_)
	{
		translator.WriteBlock(&numOfStmtInContainer_, sizeof(TInt32));
		
		// Handle refresh executor sql dynamic container
		pRefreshTEDynamicContainer_->StoreData(translator);
	}

	// Handle lock table dynamic container
	short lockTable;
	if (NULL != tableLockProtocol_ )
	{
		lockTable = 1;
		translator.WriteBlock(&lockTable, sizeof(short));
		
		tableLockProtocol_->StoreData(translator);
	}
	else
	{
		lockTable = 0;
		translator.WriteBlock(&lockTable, sizeof(short));
	}

}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::StoreReply()
//--------------------------------------------------------------------------//
void CRURefreshTaskExecutor::
	StoreReply(CUOFsIpcMessageTranslator &translator)
{
	inherited::StoreReply(translator);

	translator.WriteBlock(&isRecompute_,sizeof(BOOL));
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::LoadRequest()
//--------------------------------------------------------------------------//
void CRURefreshTaskExecutor::
	LoadRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::LoadRequest(translator);

	translator.ReadBlock(&isRecompute_,sizeof(BOOL));

	Int32 stringSize;
	char buffer[CUOFsIpcMessageTranslator::MaxMsgSize];

	translator.ReadBlock(&stringSize, sizeof(Int32));
#pragma nowarn(1506)   // warning elimination 
	translator.ReadBlock(buffer, stringSize);
#pragma warn(1506)  // warning elimination 

	rootMVName_ = CDSString(buffer);

	translator.ReadBlock(&stringSize, sizeof(Int32));
#pragma nowarn(1506)   // warning elimination 
	translator.ReadBlock(buffer, stringSize);
#pragma warn(1506)  // warning elimination 

	rootMVSchema_ = CDSString(buffer);

	translator.ReadBlock(&stringSize, sizeof(Int32));
#pragma nowarn(1506)   // warning elimination 
	translator.ReadBlock(buffer, stringSize);
#pragma warn(1506)  // warning elimination 

	rootMVCatalog_ = CDSString(buffer);
	
	translator.ReadBlock(&rootMVUID_, sizeof(TInt64));
	
	translator.ReadBlock(&rootMVType_, sizeof(Int32));

	translator.ReadBlock(&forceFlags_, sizeof(TInt32));

	if (NO_FORCE != forceFlags_)
	{
		translator.ReadBlock(&numOfStmtInContainer_, sizeof(TInt32));
		
		pRefreshTEDynamicContainer_ = 
#pragma nowarn(1506)   // warning elimination 
			new CRUSQLDynamicStatementContainer(numOfStmtInContainer_);		
#pragma warn(1506)  // warning elimination 
		// Handle refresh executor sql dynamic container
		pRefreshTEDynamicContainer_->LoadData(translator);
	}
	
	// Handle lock table dynamic container
	short lockTable;
	translator.ReadBlock(&lockTable, sizeof(short));
	
	if (1 == lockTable)
	{
	  tableLockProtocol_ = new CRUTableLockProtocol();
	  tableLockProtocol_->LoadData(translator);
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::LoadReply()
//--------------------------------------------------------------------------//
void CRURefreshTaskExecutor::
	LoadReply(CUOFsIpcMessageTranslator &translator)
{
	inherited::LoadReply(translator);

	translator.ReadBlock(&isRecompute_,sizeof(BOOL));
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::Init()
//
// Task executor initializiation. It may be that the executor
// does not have any work to do and will skip execution
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::Init()
{
	inherited::Init(); 

	CRUMV &rootMV = GetRefreshTask()->GetRootMV();

	SetRecompute(rootMV.WillBeRecomputed());

	rootMVName_   = rootMV.GetFullName();
	rootMVSchema_ = rootMV.GetCatName() + "." + rootMV.GetSchName();
	rootMVCatalog_ = rootMV.GetCatName();

	rootMVType_   = rootMV.GetRefreshType();
	rootMVUID_	  = rootMV.GetUID();

	// Runtime consistency check.
	// Check requirement that the regular table's deltas 
	// are empty (if they must be).
	CheckSingleDeltaRestriction();

	ComposeMySql();
} 

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::ComposeMySql()
//
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::ComposeMySql()
{
	CRURefreshSQLComposer myComposer(GetRefreshTask());

	CRUMV &rootMV = GetRefreshTask()->GetRootMV();

	numOfStmtInContainer_ = rootMV.GetTablesUsedByMe().GetCount() + FIRST_TBL_STAT;

	pRefreshTEDynamicContainer_ = 
#pragma nowarn(1506)   // warning elimination 
		new CRUSQLDynamicStatementContainer(numOfStmtInContainer_);
#pragma warn(1506)  // warning elimination 

	ComposeForceStatements();
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::ComposeForceStatements()
//
// The Refresh SQL container contains ONLY Control SQL statement.
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::ComposeForceStatements()
{
	CRUMV &rootMV = GetRefreshTask()->GetRootMV();
	const CRUMVForceOptions* pForceOption = rootMV.GetMVForceOption();
	CRURefreshSQLComposer myComposer(GetRefreshTask());

	// Deal with explain data
	if (NULL != pForceOption &&
		CRUForceOptions::EXPLAIN_ON == pForceOption->GetExplainOption())
	{
		forceFlags_ = forceFlags_ | FORCE_SHOW_EXPLAIN; 
		myComposer.ComposeShowExplain();
		pRefreshTEDynamicContainer_->SetStatementText(SHOW_EXPLAIN, myComposer.GetSQL());
	}

	ComposeCQSStatements();
	
	ComposeMDAMStatements();
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::ComposeMDAMStatements()
//
//--------------------------------------------------------------------------//
void CRURefreshTaskExecutor::ComposeMDAMStatements()
{
	BOOL hasActiveCQS = FALSE;
	
	CRURefreshSQLComposer myComposer(GetRefreshTask());
	
	CRUMV &rootMV = GetRefreshTask()->GetRootMV();
	const CRUMVForceOptions* pForceOption = rootMV.GetMVForceOption();

	// Even if there are no force options, we still force mdam for tables
	// with more than two columns.
	ComposeControlTableStmtForUsedTables();

	// MV base tables force options
	if (NULL != pForceOption &&
		CRUForceOptions::MDAM_NO_FORCE != pForceOption->GetMDAMoption())
	{
		// Compose CONTROL TABLE mv_name MDAM option
		myComposer.ComposeCntrlTableMDAMText(pForceOption->GetMDAMoption(), 
											 &rootMV.GetFullName());
		pRefreshTEDynamicContainer_->SetStatementText(MV_MDAM_STAT, myComposer.GetSQL());
					
		forceFlags_ |= FORCE_MV_MDAM;
	}

	// Compose reset MDAMs
	if (forceFlags_ & FORCE_MV_MDAM || 	forceFlags_ & FORCE_TABLE_MDAM)
	{
		// Compose CONTROL TABLE * MDAM 'RESET'
		myComposer.ComposeResetCntrlTableMDAMText();
		pRefreshTEDynamicContainer_->SetStatementText(RESET_MDAM_STAT, myComposer.GetSQL());
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::ComposeCQSStatements()
//
//--------------------------------------------------------------------------//
void CRURefreshTaskExecutor::ComposeCQSStatements()
{
	BOOL hasActiveCQS = FALSE;
	
	CRURefreshSQLComposer myComposer(GetRefreshTask());
	
	CRUMV &rootMV = GetRefreshTask()->GetRootMV();
	const CRUMVForceOptions* pForceOption = rootMV.GetMVForceOption();

	if (NULL == pForceOption)
	{
		return;
	}

	// MV force options
	if ((CRUForceOptions::GB_NO_FORCE != pForceOption->GetGroupByoption() ||
	    CRUForceOptions::JOIN_NO_FORCE !=   pForceOption->GetJoinoption()))
	{
		// Compose Control query shape for internal refresh stmt
		myComposer.ComposeControlQueryShape();
		pRefreshTEDynamicContainer_->SetStatementText(CQS_STAT, myComposer.GetSQL());
		forceFlags_ = forceFlags_ | FORCE_CQS;
		hasActiveCQS = TRUE;
	}

	// Deal with a specific user CQS
	if (FALSE == pForceOption->GetCQSStatment().IsEmpty())
	{
		pRefreshTEDynamicContainer_->SetStatementText
			(USER_CQS_FORCE, pForceOption->GetCQSStatment());
		forceFlags_ = forceFlags_ | FORCE_USER_CQS;
		hasActiveCQS = TRUE;
	}

	if (TRUE == hasActiveCQS)
	{
		myComposer.ComposeControlQueryShapeCut();
		pRefreshTEDynamicContainer_->SetStatementText(CQS_CUT_STAT, myComposer.GetSQL());
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::ComposeControlTableStmtForUsedTables()
//
// Compose the Control MDAM option for all base tables for forced tables
//--------------------------------------------------------------------------//
void CRURefreshTaskExecutor::ComposeControlTableStmtForUsedTables() 
{
	CRUMV &rootMV = GetRefreshTask()->GetRootMV();

	const CRUMVForceOptions* pForceOption = rootMV.GetMVForceOption();

	CRUForceOptions::MdamOptions mdamOpt = CRUForceOptions::MDAM_NO_FORCE;

	Int32 stmtIndex = FIRST_TBL_STAT;

	if (NULL != pForceOption)
	{
		VerifyForceTblListCorrectness();

		if (pForceOption->IsTableStarUsed())
		{
			mdamOpt = pForceOption->GetTableStarOption();
		}
	}

	CRUTblList &tblList = rootMV.GetTablesUsedByMe();
	
	DSListPosition pos = tblList.GetHeadPosition();

	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList.GetNext(pos);
		RUASSERT(NULL != pTbl);
		ComposeControlTableStmtForUsedTable(*pTbl,stmtIndex,mdamOpt);
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::ComposeControlTableStmtForUsedTables()
//
// Compose the Control MDAM option for a single table for forced tables
//
// For table with more then two columns and non empty range log the default 
// is to force MDAM
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::ComposeControlTableStmtForUsedTable(CRUTbl &tbl,
																 Int32 &stmtIndex,
																 CRUForceOptions::MdamOptions mdamOpt)
{
   	CRURefreshSQLComposer myComposer(GetRefreshTask());
	

	if (CRUForceOptions::MDAM_NO_FORCE == mdamOpt)
	{
		CRUTableForceOptions *forceOpt = GetForceOptionForTable(tbl.GetFullName());
		
		if (NULL == forceOpt)
		{
			mdamOpt = GetDefaultMdamOptForTable(tbl);
		}
		else
		{
			mdamOpt = forceOpt->GetMdamOptions();
		}
	}

	if (CRUForceOptions::MDAM_NO_FORCE != mdamOpt)
	{
		// Compose CONTROL TABLE table_name MDAM option
		myComposer.ComposeCntrlTableMDAMText(mdamOpt, &(tbl.GetFullName()));
		pRefreshTEDynamicContainer_->SetStatementText
#pragma nowarn(1506)   // warning elimination 
			(stmtIndex++, myComposer.GetSQL());
#pragma warn(1506)  // warning elimination 
		
		forceFlags_ |= FORCE_TABLE_MDAM;
	}

}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::GetDefaultMdamOptForTable()
//
// If the table has more then two key columns and the range log is not empty
// then mdam must be forced on the table unless the user forced otherwise
//--------------------------------------------------------------------------//
CRUForceOptions::MdamOptions CRURefreshTaskExecutor::GetDefaultMdamOptForTable(CRUTbl &tbl)
{
	Int32 numTableKeyCol = tbl.GetKeyColumnList().GetCount();

	if (numTableKeyCol < 2)
	{
		return CRUForceOptions::MDAM_NO_FORCE;
	}

	if (FALSE == GetRefreshTask()->IsSingleDeltaRefresh())
	{
		return CRUForceOptions::MDAM_NO_FORCE;
	}

	const CRUDeltaDef *pDdef = GetRootMV().GetDeltaDefByUid(tbl.GetUID());
	
	if (NULL != pDdef && TRUE == pDdef->isRangeLogNonEmpty_)
	{
		return CRUForceOptions::MDAM_ON;
	}

	return CRUForceOptions::MDAM_NO_FORCE;
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::GetForceOptionForTable()
//--------------------------------------------------------------------------//

CRUTableForceOptions* CRURefreshTaskExecutor::GetForceOptionForTable(const CDSString &name) 
{
	CRUMV &rootMV = GetRefreshTask()->GetRootMV();

	const CRUMVForceOptions* pForceOption = rootMV.GetMVForceOption();

	if (NULL == pForceOption)
	{
		return NULL;
	}

	const CRUTableForceOptionsList &tblList = pForceOption->GetTableForceList();

	DSListPosition pos = tblList.GetHeadPosition();

	while (NULL != pos) 
	{
		CRUTableForceOptions* forceOpt = tblList.GetNext(pos);
		if (name == forceOpt->GetFullName())
		{
			return forceOpt;
		}
	}
	return NULL;
}


//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::VerifyForceTblListCorrectness()
//
// Check that the user did not refered to a un-existing base table
//--------------------------------------------------------------------------//
void CRURefreshTaskExecutor::VerifyForceTblListCorrectness()
{
	const CRUMVForceOptions &forceOption =
		*GetRefreshTask()->GetRootMV().GetMVForceOption();

	const CRUTableForceOptionsList &tblList = forceOption.GetTableForceList();

	DSListPosition pos = tblList.GetHeadPosition();

	while (NULL != pos) 
	{
		CRUTableForceOptions* forceOpt = tblList.GetNext(pos);
		
		CRUTbl *pTbl = GetRefreshTask()->GetRootMV().GetUsedTableByName(forceOpt->GetFullName());

		if(NULL == pTbl)
		{
			CRUException ex;
			ex.SetError(IDS_RU_FORCE_FILE_TABLE_NOT_EXISTS);
			ex.AddArgument(GetRefreshTask()->GetRootMV().GetFullName());
			ex.AddArgument(forceOpt->GetFullName());
			throw ex;
		}
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::FinalMetadataUpdate()
//
//  After the refresh is complete, update the metadata 
//	of the refreshed MV(s) in the SMD and UMD tables.
//
//	(1) If the root MV is an ON REQUEST MV, promote its 
//		epoch vector with regards to the underlying tables.
//	(2) For every MV handled by the task:
//	    (2.1) Update the MV's REFRESHED_AT timestamp.
//	    (2.2) Update the MV's statistics (different for 
//		      the root and non-root MV's).
//
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::FinalMetadataUpdate()
{
	CRURefreshTask *pParentTask = GetRefreshTask();
	RUASSERT (NULL != pParentTask);


	TInt32 currDuration = 0;
        TInt64 ts = CalculateTimeStamp();

	if (TRUE == pParentTask->NeedToExecuteInternalRefresh())
	{
		currDuration = GetTimerDuration();
	}
	
	CRUMV *pPrevMV, *pCurrMV = NULL;
	CRUMVList &mvList = pParentTask->GetMVList();
	DSListPosition pos = mvList.GetHeadPosition();

	while (NULL != pos)
	{
		pPrevMV = pCurrMV;
		pCurrMV = mvList.GetNext(pos);

		// Update the MV's REFRESHED_AT timestamp
		pCurrMV->SetTimestamp(ts);

                // publish the refresh event to the PUBLISH table
           NABoolean isRecomputePublish = (IsRecompute() == TRUE);
           pCurrMV->PublishMVRefresh(isRecomputePublish);
   
	   if (CDDObject::eON_STATEMENT != pCurrMV->GetRefreshType())
           {
		if (NULL == pPrevMV)
		{
			// This is the Root MV (the first in the list)
			if (CDDObject::eON_REQUEST == pCurrMV->GetRefreshType())
			{
				// For every table T used by MV, 
				// set MV.EPOCH[T]<--T.CURRENT_EPOCH.
				UpdateRootMVCurrentEpochVector();
			}
			
			UpdateRootMVStatistics(currDuration);
		}
		else
		{
			UpdateNonRootMVStatistics(pCurrMV, pPrevMV, currDuration);
		}
	   }
		
		pCurrMV->SaveMetadata();
	}

        if (CDDObject::eON_STATEMENT != GetRootMV().GetRefreshType())
        {
	IncrementTopMvCurrentEpoch();
        }
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::IncrementTblCurrentEpoch()
//
//
//	If the top MV in the task (i.e., the one that no other
//	MV in the task uses it) is also an involved table (i.e.,
//	there is another MV in the other task that uses it), then:
//	(1) Increment the MV's current epoch.
//	(2) Update the table's timestamp (for the further propagation).
//
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::IncrementTopMvCurrentEpoch()
{
	CRUMV &mv = GetRefreshTask()->GetTopMV();

	if (TRUE == mv.IsInvolvedTbl())
	{
		CRUTbl *pTbl = mv.GetTblInterface();
		
		pTbl->IncrementCurrentEpoch(TRUE /* through DDOL */);
		pTbl->SaveMetadata();
		
		pTbl->SetTimestamp(mv.GetTimestamp());
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::UpdateRootMVCurrentEpochVector()
//
//	For every table T used by the task's root MV, 
//	set MV.EPOCH[T] <-- T.CURRENT_EPOCH
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::UpdateRootMVCurrentEpochVector()
{
	CRUMV &rootMV = GetRootMV();
	CRUTblList &tblList = rootMV.GetTablesUsedByMe();

	DSListPosition pos = tblList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList.GetNext(pos);
		rootMV.SetEpoch(pTbl->GetUID(), pTbl->GetCurrentEpoch());
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::UpdateRootMVStatistics()
//
//	Update the statistics for the root MV's refresh duration 
//	and (optionally) the delta size.
//
//	In the case the MV was recomputed, or refreshed incrementally
//	with more than one delta, only the total duration is updated. 
//	In the case a single-delta refresh happened, both the duration
//	*with regards to this delta*, and the delta size are updated.
//
//	For each type of update, the new value of the heuristic 
//	is computed as a weighted average between the old heuristic 
//	and the new measurement.
//
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::UpdateRootMVStatistics(TInt32 currDuration)
{
	CRURefreshTask &task = *GetRefreshTask();
	if (FALSE == task.NeedToExecuteInternalRefresh())
	{
		return;
	}

	CRUDeltaDefList &deltaDefList = task.GetDeltaDefList();

	TInt32 prevDuration, nextDuration;
	CRUMV &rootMV = GetRootMV();

	if (TRUE == IsRecompute())
	{
		prevDuration = rootMV.GetRecomputeDurationEstimate();
		nextDuration = EvaluateHeuristic(currDuration, prevDuration);
		rootMV.SetRecomputeDurationEstimate(nextDuration);
		return;
	}

	switch(deltaDefList.GetCount())
	{
		case 0:
		{
			// This cannot happen 
			// (NeedToExecuteInternalRefresh() must have returned FALSE).
			RUASSERT(FALSE);
		}
		case 1:
		{
			CRUDeltaDef *pDdef = deltaDefList.GetAt(0);
			// A special case: update the refresh duration,
			// and (optionally) the delta size.
			UpdateSingleDeltaStatistics(pDdef, currDuration);
			break;
		}
		case 2:
		{
			prevDuration = rootMV.GetRefreshDurationEstimateWith_2_Deltas();
			nextDuration = EvaluateHeuristic(currDuration, prevDuration);
			rootMV.SetRefreshDurationEstimateWith_2_Deltas(nextDuration);
			break;
		}
		case 3:
		{
			prevDuration = rootMV.GetRefreshDurationEstimateWith_3_Deltas();
			nextDuration = EvaluateHeuristic(currDuration, prevDuration);
			rootMV.SetRefreshDurationEstimateWith_3_Deltas(nextDuration);
			break;
		}
		default:
		{
			prevDuration = rootMV.GetRefreshDurationEstimateWith_N_Deltas();
			nextDuration = EvaluateHeuristic(currDuration, prevDuration);
			rootMV.SetRefreshDurationEstimateWith_N_Deltas(nextDuration);
			break;
		}
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::UpdateSingleDeltaStatistics()
//	
//	Update the heuristics for refresh duration for the *specific*
//	table's delta and the delta size (if Duplicate Elimination 
//	was done before, and hence, the delta size is known).
//	
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::
UpdateSingleDeltaStatistics(CRUDeltaDef *pDdef, TInt32 currDuration)
{
	CRUMV &rootMV = GetRootMV();

	TInt32 prevDuration, nextDuration;
	prevDuration = rootMV.GetRefreshDurationEstimate(pDdef->tblUid_);
	
	nextDuration = EvaluateHeuristic(currDuration, prevDuration);
	rootMV.SetRefreshDurationEstimate(pDdef->tblUid_, nextDuration);

	// Appling the delta size computation heuristic
	
	// If there are no statistics, 0 is the default for the delta size
	TInt32 nextDelta = 0;

	if (NULL != pDdef->pStat_)
	{
		// There are statistics, hence the delta size is known
		TInt32 currDelta = pDdef->pStat_->GetDeltaSize();
		TInt32 prevDelta = rootMV.GetDeltaSizeEstimate(pDdef->tblUid_);
		nextDelta = EvaluateHeuristic(currDelta, prevDelta);
	}
	
	rootMV.SetDeltaSizeEstimate(pDdef->tblUid_, nextDelta);
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::UpdateNonRootMVStatistics()
//
//	A non-root MV receives its delta pipelined from the MV
//	previous MV in the list. Hence, the statistic is with regards
//	to the previous MV's (single) delta.
//
//	There is no estimate on the delta size that is pipelined 
//	between the MVs, hence the heuristic is set to 0.
//
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::
UpdateNonRootMVStatistics(CRUMV *pCurrMV, 
						  CRUMV *pPrevMV,
						  TInt32 currDuration)
{
	if (FALSE == GetRefreshTask()->NeedToExecuteInternalRefresh())
	{
		return;
	}

	TInt64 tblUid = pPrevMV->GetUID();

	TInt32 prevDuration = 
		pCurrMV->GetRefreshDurationEstimate(tblUid);

	TInt32 nextDuration = 
		EvaluateHeuristic(currDuration, prevDuration);

	pCurrMV->SetRefreshDurationEstimate(tblUid, prevDuration);
	pCurrMV->SetDeltaSizeEstimate(tblUid, 0);
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::EvaluateHeuristic()
//
//	The heuristic is an exponential backoff function that gives a 
//	part of weight to the old value and the new measurement:
//	
//		new = a*old + b*measure, where a+b=1
//
//--------------------------------------------------------------------------//

TInt32 CRURefreshTaskExecutor::EvaluateHeuristic(TInt32 curr, TInt32 prev)
{
	const double prevFactor = 0.8;
	const double currFactor = 0.2;
	
	if ( 0 == prev )
	{
		return curr;
	}
	
	return static_cast<TInt32> ( prevFactor*prev + currFactor*curr );
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::CalculateTimeStamp()
//
//	All the MVs that the task executor takes care of 
//	will receive the same timestamp. This must be the timestamp
//	of the database cut that all the tables under the MV represent. 
//
//	The cases are as follows:
//	(1) All the root MV's expanded tree is being refreshed.
//		(1.1) The root MV is based only on regular tables/ON STATEMENT MVs
//	          (i.e., fully synchronized tables). If there is more than
//		      one such table, they will belong to the same equivalence
//		      set, and will have the same timestamp. Hence, the MV will
//		      inherit their timestamp.
//		(1.2) The MV is also based on involved MVs. All the MVs used by
//		      it inherit inductively the equivalence set's common timestamp.
//		      Hence, the MV will inherit the same timestamp.
//	(2) The root MV is based on non-involved MVs.
//		(2.1) If there is more than one such used MV - inconsistency.
//		(2.2) Otherwise, the root MV will inherit the external MV's
//		      timestamp. The timestamp will be smaller that all of the
//			  other used table's timestamps (current time measured in this
//		      invocation of the utility).
//	
//	In all the cases, the MV's timestamp is the minimum between the
//	used objects' timestamps.
//
//--------------------------------------------------------------------------//

TInt64 CRURefreshTaskExecutor::CalculateTimeStamp()
{
	CRUTblList &tblList = GetRootMV().GetTablesUsedByMe();
	DSListPosition pos = tblList.GetHeadPosition();

	CRUTbl *pTbl = tblList.GetNext(pos);
	TInt64 minTS = pTbl->GetTimestamp();

	while (NULL != pos)
	{
		pTbl = tblList.GetNext(pos);
		TInt64 nextTS = pTbl->GetTimestamp();

		minTS = (nextTS < minTS) ? nextTS : minTS;
	}

	if (0 == minTS)
	{
		// If the timestamp has not been executed until now,
		// this is time to compute it.
		// RUASSERT(CDDObject::eRECOMPUTE == GetRootMV().GetRefreshType());
		minTS = CRUGlobals::GetCurrentTimestamp();
	}

	return minTS;
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::GenOpeningMessage()
//
//	CALLED BY: LogOpeningMessage() method
//
//	Generate the message about successful refresh.
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::GenOpeningMessage(CDSString &msg) 
{
	CRURefreshTask &task = *GetRefreshTask();

	if (task.GetMVList().GetCount() > 1)
	{
		msg += RefreshDiags[2];
		task.DumpNamesOfAllMVs(msg);
		msg += RefreshDiags[4];
	}
	else
	{
		msg += RefreshDiags[3];
		task.DumpNamesOfAllMVs(msg);
		msg += RefreshDiags[5];
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshTask::GenClosureMessage()
//
//	CALLED BY: LogOpeningMessage() method
//
//	Generate the message about successful refresh.
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::GenClosureMessage(CDSString &msg) 
{
	CRURefreshTask &task = *GetRefreshTask();

	if (task.GetMVList().GetCount() > 1)
	{
		if (FALSE == GetRefreshTask()->NeedToExecuteInternalRefresh())
		{
			msg += RefreshDiags[2];
			task.DumpNamesOfAllMVs(msg);
			msg += RefreshDiags[6];
		}
		else
		{
			msg += RefreshDiags[2];
			task.DumpNamesOfAllMVs(msg);
			msg += RefreshDiags[7];
		}
	}
	else
	{
		if (FALSE == GetRefreshTask()->NeedToExecuteInternalRefresh())
		{
			msg += RefreshDiags[3];
			task.DumpNamesOfAllMVs(msg);
			msg += RefreshDiags[8];
		}
		else
		{
			msg += RefreshDiags[3];
			task.DumpNamesOfAllMVs(msg);
			msg += RefreshDiags[9];
		}
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::CheckSingleDeltaRestriction()
// 
//	Init() callee
//
//	If the task must perform a runtime consistency check,
//	this means that the MV which is based on a single MV
//	and one or more tables is refreshed standalone. In this
//	context, the deltas of all the fully-synchronized used
//	objects (not ON REQUEST MVs) must be empty.
//
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::CheckSingleDeltaRestriction()
{
	CRUMV &rootMV = GetRootMV();
	CRUTblList &tblList = rootMV.GetFullySyncTablesUsedByMe();

	DSListPosition pos = tblList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList.GetNext(pos);
		RUASSERT (TRUE == pTbl->IsFullySynchronized());

		if (TRUE == pTbl->IsEmptyDeltaNeeded()
			&&
			TRUE == rootMV.IsDeltaNonEmpty(*pTbl))
		{
			CRUException errDesc;
			errDesc.SetError(IDS_RU_INCONSISTENT_JOIN);
			errDesc.AddArgument(rootMVName_);
			throw errDesc;
		}
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::ApplyIRCompilerDefaults()
//
// Apply the force options for the internal refresh statement 
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::ApplyIRCompilerDefaults()
{
	// Always turn on the explain generation for IR commands
	SetExplainGeneration();
	SetAllowOfflineAccess();

	if (NO_FORCE == forceFlags_)
	{
		return;
	}
	

	RUASSERT(NULL != pRefreshTEDynamicContainer_);

	CRUGlobals::GetInstance()->GetJournal().LogMessage(
		"USING THE FOLLOWING CONTROL STATMENTS :\n");


	if (FORCE_CQS & forceFlags_)
	{
		ApplyCQSForInternalRefresh();
	}

	if (FORCE_USER_CQS & forceFlags_)
	{
		ApplyUserCQSForInternalRefresh();
	}

	if (FORCE_MV_MDAM & forceFlags_)
	{
		CDMPreparedStatement *pStat = pRefreshTEDynamicContainer_->
							GetPreparedStatement(MV_MDAM_STAT);

		ExecuteStatement(*pStat, MV_MDAM_STAT);

		CRUGlobals::GetInstance()->GetJournal().LogMessage(
			pRefreshTEDynamicContainer_->GetLastSQL(MV_MDAM_STAT));
	}

	if (FORCE_TABLE_MDAM & forceFlags_)
	{
		for (Int32 i=FIRST_TBL_STAT;i<pRefreshTEDynamicContainer_->GetNumOfStmt();i++)
		{
			CDMPreparedStatement *pStat = pRefreshTEDynamicContainer_->
#pragma nowarn(1506)   // warning elimination 
								GetPreparedStatement(i);
#pragma warn(1506)  // warning elimination 

			if (NULL == pStat)
			{
				// No more control statements for table
				break;
			}

			ExecuteStatement(*pStat, i);

			CRUGlobals::GetInstance()->GetJournal().LogMessage(
#pragma nowarn(1506)   // warning elimination 
				pRefreshTEDynamicContainer_->GetLastSQL(i));
#pragma warn(1506)  // warning elimination 
		}
 	}
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::SetExplainGeneration()
// 
// Execute Control query default GENERATE_EXPLAIN 'ON'
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::SetExplainGeneration()
{
	CDSString stmt("control query default GENERATE_EXPLAIN 'ON'");
	CDMConnection conn;
	conn.SetAllowSpecialSyntax(TRUE);
    CDMStatement  *pStmt = conn.CreateStatement();
 	pStmt->ExecuteUpdate(stmt, TRUE);
	conn.DeleteStatement(pStmt);
}


//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::SetAllowOfflineAccess()
// 
// Execute Control query default CAT_PERMIT_OFFLINE_ACCESS 'ON'
// 
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::SetAllowOfflineAccess()
{
	CDSString stmt("control query default CAT_PERMIT_OFFLINE_ACCESS 'ON'");
	CDMConnection conn;
	conn.SetAllowSpecialSyntax(TRUE);
        CDMStatement  *pStmt = conn.CreateStatement();
 	pStmt->ExecuteUpdate(stmt, TRUE);
	conn.DeleteStatement(pStmt);
}



//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::ResetIRCompilerDefaults()
// 
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::ResetIRCompilerDefaults()
{
	if ((FORCE_CQS & forceFlags_) || (FORCE_USER_CQS & forceFlags_))
	{
		CDMPreparedStatement *pStat = pRefreshTEDynamicContainer_->
							GetPreparedStatement(CQS_CUT_STAT);

		ExecuteStatement(*pStat, CQS_CUT_STAT);
	}
	
	if (forceFlags_ & FORCE_MV_MDAM || 
		forceFlags_ & FORCE_TABLE_MDAM)
	{
		CDMPreparedStatement *pStat = 
			pRefreshTEDynamicContainer_->GetPreparedStatement(RESET_MDAM_STAT);

		ExecuteStatement(*pStat,RESET_MDAM_STAT);
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::ApplyCQSForInternalRefresh()
// 
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::ApplyCQSForInternalRefresh()
{
	CDMPreparedStatement *pStat = pRefreshTEDynamicContainer_->
						GetPreparedStatement(CQS_STAT);

	ExecuteStatement(*pStat, CQS_STAT);

	CRUGlobals::GetInstance()->GetJournal().LogMessage(
		pRefreshTEDynamicContainer_->GetLastSQL(CQS_STAT));
}

//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::ApplyUserCQSForInternalRefresh()
// 
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::ApplyUserCQSForInternalRefresh()
{
	CDMPreparedStatement *pStat = pRefreshTEDynamicContainer_->
						GetPreparedStatement(USER_CQS_FORCE);

	ExecuteStatement(*pStat, USER_CQS_FORCE);

	CRUGlobals::GetInstance()->GetJournal().LogMessage(
		pRefreshTEDynamicContainer_->GetLastSQL(USER_CQS_FORCE));
}


//--------------------------------------------------------------------------//
//	CRURefreshTaskExecutor::ExecuteShowExplain()
// 
//--------------------------------------------------------------------------//

void CRURefreshTaskExecutor::ExecuteShowExplain()
{
	if (FALSE == (FORCE_SHOW_EXPLAIN & forceFlags_))
	{
		return;
	}
	
	CDMPreparedStatement *pStat = pRefreshTEDynamicContainer_->
						GetPreparedStatement(SHOW_EXPLAIN);

	ExecuteStatement(*pStat, SHOW_EXPLAIN);

	CRUGlobals::GetInstance()->GetJournal().LogMessage(
		pRefreshTEDynamicContainer_->GetLastSQL(SHOW_EXPLAIN));
}


//--------------------------------------------------------------------------//
// Called from the PROLOGUE state for remotely executed tasks.
// Returns FALSE if the main process lost its locks.
//--------------------------------------------------------------------------//
NABoolean CRURefreshTaskExecutor::StartTableLockProtocol()
{
  RUASSERT(NULL != tableLockProtocol_);

  if ( TRUE == tableLockProtocol_->IsEnsureUsedTableLockContinuityNeeded() && 
       TRUE == IsInTaskProcess())
  {
    if (FALSE == tableLockProtocol_->EnsureUsedTableLockContinuity() )
    {
      // The main process lost his locks , so it must
      // have fallen. We must exit with out doing anything
      #ifdef _DEBUG
	cout << "The main process lost his locks , so it must have died.\n";
      #endif

      return FALSE;
    }	
  }

  return TRUE;
}

//--------------------------------------------------------------------------//
// Called from the REMOTE_END state for remotely executed tasks.
//--------------------------------------------------------------------------//
void CRURefreshTaskExecutor::EndTableLockProtocol()
{
  RUASSERT(NULL != tableLockProtocol_);

  if ( TRUE == tableLockProtocol_->IsEnsureUsedTableLockContinuityNeeded() &&
       TRUE == IsInTaskProcess())
  {
    tableLockProtocol_->UnLockUsedTablePartitions();
  }
}

/*
******************************************************************************
*** Class CRUTableLockProtocol 
******************************************************************************
*/

//--------------------------------------------------------------------------//
//	CRUTableLockProtocol::Init()
//
//--------------------------------------------------------------------------//
void CRUTableLockProtocol::Init(CRUTblList &tblList, 
				CRUDeltaDefList &DeltaDefList)
{
  CRUDeltaDef *pDdef    = NULL;
  CRUTbl      *curTable = NULL;

  // For each table in the used table list, find the 
  // corresponding delta def.
  DSListPosition tPos = tblList.GetHeadPosition();
  while (tPos != NULL)
  {
    curTable = tblList.GetNext(tPos);

    DSListPosition dPos = DeltaDefList.GetHeadPosition();
    while (dPos != NULL)
    {
      pDdef = DeltaDefList.GetNext(dPos);

      // Match each table to its delta def by table UID.
      if (curTable->GetUID() == pDdef->tblUid_)
      { 
	// We have a match!
        // We need this lock only if the range log has any data in it.
	if (TRUE == pDdef->isRangeLogNonEmpty_)
	{
	  if (LocksList_ == NULL)
	  {
	    // This is the first match we have found - alloc the list.
	    LocksList_ = new LocksList();
	    NothingToLock_ = FALSE;
	  }

	  // Alloc a new lock object, init it and add it to the list.
	  CRUSingleTableLockProtocol *thisLock = new CRUSingleTableLockProtocol();
	  thisLock->Init(curTable, pDdef->toEpoch_);
	  LocksList_->AddTail(thisLock);
	}

	break; // Done with this table.

      } // if Matching table UID

    } // Loop on DeltaDefList

  } // Loop on tblList

}

//--------------------------------------------------------------------------//
//	CRUTableLockProtocol::StoreData()
//
//--------------------------------------------------------------------------//
void CRUTableLockProtocol::StoreData(CUOFsIpcMessageTranslator &translator)
{
  // Store the NothingToLock_ flag
  translator.WriteBlock(&NothingToLock_, sizeof(BOOL));
  if (NothingToLock_)
    return;

  // Store the number of elements in the lock list
  Lng32 count = LocksList_->GetCount();
  translator.WriteBlock(&count, sizeof(Lng32));

  // Store the lock elements.
  CRUSingleTableLockProtocol *thisLock = NULL;
  DSListPosition pos = LocksList_->GetHeadPosition();
  while (pos != NULL)
  {
    thisLock = LocksList_->GetNext(pos);
    thisLock->StoreData(translator);
  } 

}

//--------------------------------------------------------------------------//
//	CRUTableLockProtocol::LoadData()
//
//--------------------------------------------------------------------------//
void CRUTableLockProtocol::LoadData(CUOFsIpcMessageTranslator &translator)
{
  // Load the NothingToLock_ flag
  translator.ReadBlock(&NothingToLock_, sizeof(BOOL));
  if (NothingToLock_)
    return;

  // Load the number of elements in the lock list
  Lng32 count;
  translator.ReadBlock(&count, sizeof(Lng32));

  // Load the lock elements.
  LocksList_ = new LocksList();
  for (Lng32 i=0; i<count; i++)
  {
    // Alloc a new lock object, init it and add it to the list.
    CRUSingleTableLockProtocol *thisLock = new CRUSingleTableLockProtocol();
    thisLock->LoadData(translator);
    LocksList_->AddTail(thisLock);

  }

}

//--------------------------------------------------------------------------//
//	CRUTableLockProtocol::IsEnsureUsedTableLockContinuityNeeded()
//
//--------------------------------------------------------------------------//

BOOL CRUTableLockProtocol::IsEnsureUsedTableLockContinuityNeeded() const
{
  if (NothingToLock_)
    return FALSE;

  // Iterate on the lock elements.
  CRUSingleTableLockProtocol *thisLock = NULL;
  DSListPosition pos = LocksList_->GetHeadPosition();
  while (pos != NULL)
  {
    thisLock = LocksList_->GetNext(pos);
    if (thisLock->IsEnsureUsedTableLockContinuityNeeded())
      return TRUE;
  } 

  return FALSE;
}

//--------------------------------------------------------------------------//
//	CRUTableLockProtocol::EnsureUsedTableLockContinuity()
//
// called by work() in the remote process
//
// The function returns TRUE is all is well.
// It returns FALSE if it detected new rows in the log file, which means
// the main process has died.
//--------------------------------------------------------------------------//

BOOL CRUTableLockProtocol::EnsureUsedTableLockContinuity()
{
  if (NothingToLock_)
    return TRUE;

  // Iterate on the lock elements.
  CRUSingleTableLockProtocol *thisLock = NULL;
  DSListPosition pos = LocksList_->GetHeadPosition();
  while (pos != NULL)
  {
    thisLock = LocksList_->GetNext(pos);
    if (thisLock->EnsureUsedTableLockContinuity() == FALSE)
      return FALSE;
  }

  return TRUE;
}

//--------------------------------------------------------------------------//
//	CRUTableLockProtocol::UnLockUsedTablePartitions()
//
//--------------------------------------------------------------------------//

void CRUTableLockProtocol::UnLockUsedTablePartitions()
{
  if (NothingToLock_)
    return;

  // Iterate on the lock elements.
  CRUSingleTableLockProtocol *thisLock = NULL;
  DSListPosition pos = LocksList_->GetHeadPosition();
  while (pos != NULL)
  {
    thisLock = LocksList_->GetNext(pos);
    thisLock->UnLockUsedTablePartitions();
  }
}

/*
******************************************************************************
*** Class CRUSingleTableLockProtocol 
******************************************************************************
*/

CRUSingleTableLockProtocol::CRUSingleTableLockProtocol() :
  pUsedTblFileNameList_(NULL),
  pPartitionFileList_(NULL),
  pEmpCheck_(NULL),
  endEpoch_(0)
{
}

CRUSingleTableLockProtocol::~CRUSingleTableLockProtocol()
{
  delete pUsedTblFileNameList_;
  delete pPartitionFileList_;
  delete pEmpCheck_;
}

//--------------------------------------------------------------------------//
//	CRUSingleTableLockProtocol::Init()
//
//--------------------------------------------------------------------------//
void CRUSingleTableLockProtocol::Init(CRUTbl *pTbl, 
				      TInt32 EndEpoch)
{
  endEpoch_ = EndEpoch;
  pUsedTblFileNameList_ = new CDSStringList();
  CopyPartitionFileNames(pTbl);
  BuildEmpCheck(pTbl);
}

//--------------------------------------------------------------------------//
//	CRUSingleTableLockProtocol::CopyPartitionFileNames()
//
//--------------------------------------------------------------------------//
void CRUSingleTableLockProtocol::CopyPartitionFileNames(CRUTbl *pTbl)
{
  pTbl->BuildPartitionFileNamesList();

  const CDSStringList &sourceList = pTbl->GetPartitionFileNamesList();

  DSListPosition pos = sourceList.GetHeadPosition();

  while (NULL != pos)
  {
    CDSString *pName = new CDSString(*(sourceList.GetNext(pos)));

    pUsedTblFileNameList_->AddTail(pName);
  }
}

//--------------------------------------------------------------------------//
//	CRUSingleTableLockProtocol::EnsureUsedTableLockContinuity()
//
// This function preforms the actions that are needed to ensure the lock
// continuity on the used table.The reason for this actions are explained 
// in this class description.
//
// The function returns TRUE is all is well.
// It returns FALSE if it detected new rows in the log file, which means
// the main process has died.
//--------------------------------------------------------------------------//
BOOL CRUSingleTableLockProtocol::EnsureUsedTableLockContinuity()
{
  LockUsedTablePartitions();

  if ( TRUE == AnyNewRecordsSinceEpochIncrement() )
  {
    UnLockUsedTablePartitions();
		
    return FALSE;
  }
  return TRUE;
}

//--------------------------------------------------------------------------//
//	CRUSingleTableLockProtocol::BuildEmpCheck()
//--------------------------------------------------------------------------//
void CRUSingleTableLockProtocol::BuildEmpCheck(CRUTbl *pTbl)
{
  CRUEmpCheckVector empVector;

  empVector.AddEpochForCheck(endEpoch_ + 1);
  empVector.Build();

  pEmpCheck_ = new CRUEmpCheck(empVector);
  pEmpCheck_->ComposeSQL(*pTbl);
}

//--------------------------------------------------------------------------//
//	CRUSingleTableLockProtocol::AnyNewRecordsSinceEpochIncrement()
//--------------------------------------------------------------------------//
BOOL CRUSingleTableLockProtocol::AnyNewRecordsSinceEpochIncrement()
{
  pEmpCheck_->PerformCheck();

  BOOL ret = pEmpCheck_->GetVector().IsDeltaNonEmpty(endEpoch_ + 1);

  return ret;
}

//--------------------------------------------------------------------------//
//	CRUSingleTableLockProtocol::LockUsedTablePartitions()
//--------------------------------------------------------------------------//
void CRUSingleTableLockProtocol::LockUsedTablePartitions()
{
  pPartitionFileList_ = new CUOFsFileList();
  DSListPosition pos = pUsedTblFileNameList_->GetHeadPosition();

  while (NULL != pos)
  {
    CDSString &pName = *pUsedTblFileNameList_->GetNext(pos);
    CUOFsFile *file = new CUOFsFile();
		
    file->FileOpen(pName,CUOFsFile::eProtected,CUOFsFile::eReadOnly);
		
    pPartitionFileList_->AddTail(file);
  }
}

//--------------------------------------------------------------------------//
//	CRUSingleTableLockProtocol::UnLockUsedTablePartitions()
//--------------------------------------------------------------------------//
void CRUSingleTableLockProtocol::UnLockUsedTablePartitions()
{
  DSListPosition pos = pPartitionFileList_->GetHeadPosition();

  while (NULL != pos)
  {
    CUOFsFile *file = pPartitionFileList_->GetNext(pos);
		
    file->FileClose();
  }
}

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
void CRUSingleTableLockProtocol::StoreData(CUOFsIpcMessageTranslator &translator)
{
  StoreDataFileNameList(translator);
  pEmpCheck_->StoreData(translator);

  translator.WriteBlock(&endEpoch_, sizeof(TInt32));
}

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
void CRUSingleTableLockProtocol::LoadData(CUOFsIpcMessageTranslator &translator)
{
  LoadDataFileNameList(translator);

  pEmpCheck_ = new CRUEmpCheck();
  pEmpCheck_->LoadData(translator);

  translator.ReadBlock(&endEpoch_,sizeof(TInt32));
}

//--------------------------------------------------------------------------//
//	CRUSingleTableLockProtocol::StoreDataFileNameList()
//--------------------------------------------------------------------------//
void CRUSingleTableLockProtocol::StoreDataFileNameList(CUOFsIpcMessageTranslator &translator)
{
  Int32 size = pUsedTblFileNameList_->GetCount();
  translator.WriteBlock(&size,sizeof(Int32));

  DSListPosition pos = pUsedTblFileNameList_->GetHeadPosition();
  while (NULL != pos)
  {
    Int32 stringSize;

    CDSString &str = *(pUsedTblFileNameList_->GetNext(pos));
		
    stringSize = str.GetLength() + 1;
    translator.WriteBlock(&stringSize, sizeof(Int32));

#pragma nowarn(1506)   // warning elimination 
    translator.WriteBlock(str.c_string(), stringSize);
#pragma warn(1506)  // warning elimination 
  }
}

//--------------------------------------------------------------------------//
//	CRUSingleTableLockProtocol::LoadDataFileNameList()
//--------------------------------------------------------------------------//
void CRUSingleTableLockProtocol::
  LoadDataFileNameList(CUOFsIpcMessageTranslator &translator)
{
  Int32 size;
  translator.ReadBlock(&size,sizeof(Int32));

  pUsedTblFileNameList_ = new CDSStringList();

  for (Int32 i=0;i<size;i++)
  {
    Int32 stringSize;
    const Int32 bufSize = CUOFsSystem::IpcMaxGuardianPathNameLength;
    char buf[bufSize];
		
    translator.ReadBlock(&stringSize, sizeof(Int32));
		
    RUASSERT(bufSize > stringSize);

#pragma nowarn(1506)   // warning elimination 
    translator.ReadBlock(buf, stringSize);
#pragma warn(1506)  // warning elimination 
		
    CDSString *objName = new CDSString(buf);

    pUsedTblFileNameList_->AddTail(objName);
  }
}
