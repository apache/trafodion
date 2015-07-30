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
* File:         RuRefreshTask.cpp
* Description:  Implementation of class	CRURefreshTask
*
*
* Created:      12/29/1999
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include <stdio.h>

#include "RuRefreshTask.h"
#include "RuUnAuditRefreshTaskExecutor.h"
#include "RuAuditRefreshTaskExecutor.h"
#include "RuMultiTxnRefreshTaskExecutor.h"
#include "RuEmpCheckVector.h"
#include "RuDeltaDef.h"
#include "RuGlobals.h"
#include "RuOptions.h"
#include "RuTbl.h"

//---------------------------------------------------------------//
//	Constructor and destructor of CRURefreshTask
//---------------------------------------------------------------//

CRURefreshTask::CRURefreshTask(Lng32 id, CRUMV &mv) :
	inherited(id),
	rootMV_(mv),
	mvList_(eItemsArentOwned)
{
	// Somewhat ugly, but CRUMVList can handle only pointers
	mvList_.AddTail(&mv);
}

CRURefreshTask::~CRURefreshTask() {}

//--------------------------------------------------------------------------//
//	PUBLIC METHODS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRURefreshTask::GetTopMV()
//
//	The top MV in a pipeline chain (the single MV if there
//	is no pipelining).
//
//--------------------------------------------------------------------------//

CRUMV &CRURefreshTask::GetTopMV() const
{
	DSListPosition pos = mvList_.GetHeadPosition();
	
	CRUMV *pMV = NULL;
	while (NULL != pos)
	{
		pMV = mvList_.GetNext(pos);
	}

	RUASSERT(NULL != pMV);
	return *pMV;
}

//--------------------------------------------------------------------------//
//	CRURefreshTask::HasObject()
//--------------------------------------------------------------------------//

BOOL CRURefreshTask::HasObject(TInt64 uid) const
{
	DSListPosition pos = mvList_.GetHeadPosition();
	while( NULL != pos )
	{
		CRUMV *pMV = mvList_.GetNext(pos);
		if (pMV->GetUID() == uid)
		{
			return TRUE;
		}
	}

	return FALSE;
}

//--------------------------------------------------------------------------//
//	CRURefreshTask::CreateExecutorInstance()
//
//	Creation of the appropriate task executor: 
//	Audit, NonAudit, MultiTxn.
//
//	Since some executors can leave MVs in inconsistent
//	state, we must block the resource release mechanism:
//	(1) For NonAudit refresh, DDL locks cannot be cancelled
//		for any MV handled by the task.
//	(2) For MultiTxn refresh, the DDL lock cannot be 
//		cancelled only for the root MV, because it can leave
//		context rows in the table's log. All the MVs that the
//		delta is pipelined to always start and finished in
//		a synchronized state with the root MV.
//
//--------------------------------------------------------------------------//

CRUTaskExecutor *CRURefreshTask::CreateExecutorInstance()
{
	// Create the appropriate executor ...
	CRUTaskExecutor *pRefreshEx;

	if (rootMV_.GetAuditType() != CDDObject::eAUDIT)
	{
		SetReleaseDDLLocks(FALSE /*new value*/, FALSE /*all MVs*/);

		pRefreshEx = new CRUUnAuditRefreshTaskExecutor(this);
	}
	else
	{
		if (IsRecompute() || 
                    rootMV_.GetCommitNRows() == 0 ||
                    rootMV_.GetMVStatus() != CDDObject::eINITIALIZED )
		{
			pRefreshEx = new CRUAuditRefreshTaskExecutor(this);
		}
		else
		{
			SetReleaseDDLLocks(FALSE /*new value*/, TRUE /*only root MV*/);
			pRefreshEx = new CRUMultiTxnRefreshTaskExecutor(this);
		}
	}

	return pRefreshEx;
}

//--------------------------------------------------------------//
//	CRURefreshTask::NeedToExecuteInternalRefresh()
//
//  Do we need to execute the INTERNAL REFRESH statement ?
//--------------------------------------------------------------//

BOOL CRURefreshTask::NeedToExecuteInternalRefresh() const
{
	// Case #1. Check whether recompute is required
	if (TRUE == IsRecompute())
	{
		return TRUE;
	}

	// Case #2. Check whether incremental refresh is required.
	if (FALSE == GetDeltaDefList().IsEmpty())
	{
		return TRUE;
	}

	return FALSE;
}

//--------------------------------------------------------------------------//
//	CRURefreshTask::GetTaskName()
//--------------------------------------------------------------------------//

CDSString CRURefreshTask::GetTaskName() const
{
	CDSString taskName;

	taskName += (TRUE == IsRecompute()) ? "REC(" : "REF(";
	DumpNamesOfAllMVs(taskName);
	taskName += ")";

	return taskName;
}

//--------------------------------------------------------------------------//
//	CRURefreshTask::DumpNamesOfAllMVs()
//--------------------------------------------------------------------------//
					 
void CRURefreshTask::DumpNamesOfAllMVs(CDSString &to) const
{
	DSListPosition pos = mvList_.GetHeadPosition();
	for (BOOL first = TRUE; NULL != pos; first=FALSE)
	{
		CRUMV *pMV = mvList_.GetNext(pos);
		if (FALSE == first)
		{
			to += ", ";
		}
		to += pMV->GetFullName();
	}
}

//--------------------------------------------------------------------------//
//	PROTECTED METHODS (implementation of pure virtuals)
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRURefreshTask::GetComputedCost()
//
//	Cost computation (implementation of a pure virtual 
//	method).
//
//	The Refresh task's cost is the estimate for the task's
//	execution time.
//	
//  The computation is based on the metadata that is saved 
//	in the UMD tables, and updated each time the MV is 
//	refreshed. 
//
//	If the MV observes a single non-empty T-delta, the cost 
//	estimate is with respect to T. E.g., if MV is an MJV on 
//	two tables T1 and T2, the estimate of refresh time can  
//	be different if only T1-delta or only T2-delta is non-empty.
//
//	If the MV observes more than one delta, the granularity
//	of estimation is coarser (a single number, regardless of
//	which T-deltas are observed).
//
//--------------------------------------------------------------------------//

TInt32 CRURefreshTask::GetComputedCost() const
{
#ifdef _DEBUG
	CRUOptions::DebugOption *pDO = 
		CRUGlobals::GetInstance()->
		GetOptions().FindDebugOption(CRUGlobals::IGNORE_STATISTICS, "");

	if (NULL != pDO)
	{
		return 0;	
	}
#endif

	if (TRUE == IsRecompute())
	{
		return GetRootMV().GetRecomputeDurationEstimate();
	}

	switch (GetDeltaDefList().GetCount())
	{
	case 0:	
		{
			return 0;	
		}
	case 1:
		{
			return GetSingleDeltaCost();
		}
	case 2:
		{
			return GetRootMV().GetRefreshDurationEstimateWith_2_Deltas();
		}
	case 3:
		{
			return GetRootMV().GetRefreshDurationEstimateWith_3_Deltas();
		}
	default:
		{
			return GetRootMV().GetRefreshDurationEstimateWith_N_Deltas();
		}
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshTask::PullDataFromExecutor()
//--------------------------------------------------------------------------//

void CRURefreshTask::PullDataFromExecutor()
{
	RUASSERT(0 == GetStatus());

	// Everything is fine, permit to release DDL locks
	// from the MVs refreshed in this task
	SetReleaseDDLLocks(TRUE /*new value*/, FALSE /*all MVs*/);

	if (FALSE == rootMV_.IsInvolvedTbl())
	{
		// No MV uses me, no one to propagate the info to
		return;	
	}

	CRURefreshTaskExecutor &taskEx = 
		(CRURefreshTaskExecutor &)GetExecutor();

	if (FALSE == taskEx.IsRecompute())
	{
		PullDataFromExecutorAfterIncRefresh(taskEx);
	}
	else
	{
		PullDataFromExecutorAfterRecompute(taskEx);
	}
}

//--------------------------------------------------------------------------//
//	PRIVATE METHODS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRURefreshTask::PullDataFromExecutorAfterIncRefresh()
//
//	PullDataFromExecutor() callee.
//
//	After a (non-empty) incremental refresh, update the 
//	emptiness check vector: all the MVs using me observe
//	a non-empty delta.
//
//--------------------------------------------------------------------------//

void CRURefreshTask::
PullDataFromExecutorAfterIncRefresh(CRURefreshTaskExecutor &taskEx)
{
	CRUTbl *pTbl = rootMV_.GetTblInterface();
	RUASSERT(NULL != pTbl);

	CRUEmpCheckVector &empCheckVector = pTbl->GetEmpCheckVector();

	if (TRUE == NeedToExecuteInternalRefresh())
	{
		empCheckVector.SetAllDeltasNonEmpty();
	}

	// After the task is complete, it is known for sure
	// whether the MV's delta is empty or not.
	empCheckVector.SetFinal();

	pTbl->PropagateEmpCheckToUsingMVs();
}

//--------------------------------------------------------------------------//
//	CRURefreshTask::PullDataFromExecutorAfterRecompute()
//
//	PullDataFromExecutor() callee.
//
//	After a recompute, propagate the Recompute property
//	to all the using MVs. This is a complementary mechanism
//	to propagating the property during the graph construction.
//	
//	The early propagation of the property is not sufficient,
//	because there can be unexpected recomputations 
//	of MIN/MAX MVs. However, it is required for a more accurate 
//	cost heuristics computation.
//
//--------------------------------------------------------------------------//

void CRURefreshTask::
PullDataFromExecutorAfterRecompute(CRURefreshTaskExecutor &taskEx)
{
	if (FALSE == rootMV_.WillBeRecomputed())
	{
		// Unexpected recompute
		RUASSERT(TRUE == rootMV_.IsComplexMinMax());
		rootMV_.SetRefreshPatternMask(CRUMV::RECOMPUTE);
	}

	rootMV_.GetTblInterface()->PropagateRecomputeToUsingMVs();
}

//--------------------------------------------------------------------------//
//	CRURefreshTask::ComputeSingleDeltaCost()
//	
//	GetComputedCost() callee
//
//	Compute the task's cost in the case when there is 
//	only a single non-empty delta. 
//
//  The cost estimate depends on whether duplicate elimination 
//	on T-log was done or not. If it was (hence, the statistics
//	were collected), the cost estimate is 
//
//	   PrevCost *  (DeltaSize / PrevDeltaSize)
//
//  where PrevCost is the previous cost estimate for this 
//	non-empty delta, and DeltaSize and PrevDeltaSize are the 
//	T-delta sizes in the current and previous invocations,
//	respectively. This heuristic is a rough estimate that the
//	duration of MV refresh is proportional to the delta size.
// 
//	If the delta statistics are non-available, the cost 
//	estimate is set to PrevCost.
//
//--------------------------------------------------------------------------//

TInt32 CRURefreshTask::GetSingleDeltaCost() const
{
	CRUDeltaDef *pDdef = GetDeltaDefList().GetAt(0);
	RUASSERT(NULL != pDdef);

	TInt32 prevCost = 
		rootMV_.GetRefreshDurationEstimate(pDdef->tblUid_);

	CRUDeltaStatistics *pStat = pDdef->pStat_;
	if (NULL == pStat)
	{
		return prevCost;
	}
	else 
	{
		TInt32 prevDeltaSize = 0;
                TInt32 deltaSize = 0;
                TInt32 dummy;
                try {
			prevDeltaSize = rootMV_.GetDeltaSizeEstimate(pDdef->tblUid_);
                }
                catch (...)
                {
                        dummy = 1;
                }

                try {
		        deltaSize = pStat->GetDeltaSize();
                }
                catch (...)
                {
                        dummy = 2;
                }

                try {
		        if (0 == prevDeltaSize)
		        {
			        return prevCost;
		        }
		        else
		        {
			        return prevCost * deltaSize / prevDeltaSize;
		        }
                }
                catch (...)
                {
                        dummy = 3;
                }
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshTask::SetReleaseDDLLocks()
//
//	CreateExecutorInstance() and PullDataFromExecutor() callee.
//	Block/unblock the possiblility to release DDL locks
//	on the MVs handled by the task.
//	
//--------------------------------------------------------------------------//

void CRURefreshTask::SetReleaseDDLLocks(BOOL flag, BOOL isRootMVOnly)
{
	DSListPosition pos = GetMVList().GetHeadPosition();

	while (NULL != pos)
	{
		CRUMV *pMV = GetMVList().GetNext(pos);

		pMV->SetReleaseDDLLock(flag);

		if (TRUE == pMV->IsInvolvedTbl())
		{
			// Actually, we need this only for consistency.
			// There is no RcRelease task for the associated table's object.
			pMV->GetTblInterface()->SetReleaseDDLLock(flag);
		}

		if (TRUE == isRootMVOnly)
		{
			break;
		}
	}
}
