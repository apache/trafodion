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
* File:         RuMV.cpp
* Description:  Implementation of class CRUMV.
*				
*
* Created:      02/27/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "ddobject.h"
#include "ddprivilegemgr.h"
//#include "ddschemaprivilegemgr.h"
#include "ddlock.h"
#include "ddindex.h"

#include "RuMV.h"
#include "RuTbl.h"
#include "RuDeltaDef.h"
#include "RuEmpCheckVector.h"
#include "uosessioninfo.h"

//--------------------------------------------------------------------------//
//	Constructor and destructor of CRUMV
//--------------------------------------------------------------------------//

CRUMV::CRUMV(CDDMV *pddMV) :
		pddMV_(pddMV),
		// Indexes associated with the MV
		pddIndexList_(
			new CDDIndexList(eItemsArentOwned)
		),
		// Usage lists
		pTablesUsedByMe_(
			new CRUTblList(eItemsArentOwned)
		),
		pFullySyncTablesUsedByMe_(
			new CRUTblList(eItemsArentOwned)
		),
		//
		pDeltaDefList_(
			new CRUDeltaDefList()	// the items are owned
		),
		pTblInterface_(NULL),
		refreshPatternMap_(0),
		privMap_(0),
		pMvForceOptions_(NULL)
{}

CRUMV::~CRUMV()
{
	delete pddIndexList_;

	delete pTablesUsedByMe_;

	delete pFullySyncTablesUsedByMe_;

	delete pDeltaDefList_;

	delete pMvForceOptions_;
}

//--------------------------------------------------------------------------//
//	ACCESSORS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUMV::IsInvolvedTbl()
//
//	Is the CRUTbl object that corresponds to this MV an *involved* table?
//
//--------------------------------------------------------------------------//

BOOL CRUMV::IsInvolvedTbl() const
{
	if (NULL == pTblInterface_)
	{
		return FALSE;
	}

	return pTblInterface_->IsInvolved();
}

//--------------------------------------------------------------------------//
//	CRUMV::IsDeltaNonEmpty()
//
//	Does the MV observe a non-empty delta of this table?
//--------------------------------------------------------------------------//

BOOL CRUMV::IsDeltaNonEmpty(CRUTbl &tbl)
{
	TInt32 beginEpoch = GetEpoch(tbl.GetUID());
	
	return tbl.IsDeltaNonEmpty(beginEpoch);
}

//--------------------------------------------------------------------------//
//	CRUMV::IsSelfMaintainable()
//
//	 An MV is considered self-maintainable if two conditions hold:
//	 (1) It is a single-delta MV (i.e, it is based either on a single 
//	     table or on join where only one of the tables does not appear 
//	     in the IGNORE CHANGES clause.    
//	 (2) It is not a complex MIN/MAX (i.e., if it has MIN/MAX aggregate 
//	     functions, the table that is not IGNORE CHANGES should have 
//	     an insert-only log.
//--------------------------------------------------------------------------//

BOOL CRUMV::IsSelfMaintainable()
{
	// Not a single-delta MV 
	if (FALSE == IsDeclarativeSingleDeltaMV())
	{
		return FALSE;
	}

	return (FALSE == IsComplexMinMax());
}

//--------------------------------------------------------------------------//
//	CRUMV::IsComplexMinMax()
//
//	A MIN/MAX MAV is considered *complex* if under certain conditions
//	its refresh can turn to recompute from incremental. This can happen
//	in two cases:
//	 
//	(1) The MV is (declaratively) not single-delta.
//	(2) The only table used by the MV is not insert-only.
//
//--------------------------------------------------------------------------//

BOOL CRUMV::IsComplexMinMax()
{
	if (FALSE == HasMinMaxAggregate())
	{
		return FALSE;
	}

	if (FALSE == IsDeclarativeSingleDeltaMV())
	{
		return TRUE;
	}

	RUASSERT(1 == GetTablesUsedByMe().GetCount());

	if (FALSE == GetTablesUsedByMe().GetAt(0)->IsInsertLog())
	{
		// One of the logs is not insert-only
		return TRUE;
	}

	return FALSE;
}

//--------------------------------------------------------------------------//
//	CRUMV::IsSingleDeltaRefresh()
//--------------------------------------------------------------------------//

BOOL CRUMV::IsSingleDeltaRefresh() const
{
	return (1 == GetDeltaDefList().GetCount());
}

//--------------------------------------------------------------------------//
//	PIPELINING CRITERIA
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUMV::CanPipelineFromMe()
//
//	CRUScheduler::MaximizePipeliningPotential() callee
//
//	Can delta be pipelined FROM this MV upstream?
//
//	The conditions for pipelining are:
//	(1) The MV "theoretically" pipelineable, i.e., it does not 
//		perform recompute, and the MV's	query allows pipelining.
//	(2) If the root MV's query has a MIN/MAX aggregate, 
//		the MV must not be a complex MIN/MAX (i.e., it cannot revert
//		to recompute in the runtime)
//	(3) The MV is used by *exactly* one MV, which is also
//		an involved MV.
//
//	If the return value is TRUE, the using MV's uid is returned
//	through the nextCandidateUid parameter.
//
//--------------------------------------------------------------------------//

BOOL CRUMV::CanPipelineFromMe(BOOL isRoot, TInt64 &nextCandidateUid) 
{
	if (TRUE == isRoot)
	{
		// An optimization.
		// If this MV does not start the pipelining chain
		// (not the root MV), IsPipelineable() was already
		// called previously, when checking whether delta can
		// be pipelined TO this MV. We would not have been 
		// here unless that check had returned TRUE.
		if (FALSE == IsPipelineable())
		{
			return FALSE;	// (1) 
		}

		if (TRUE == IsComplexMinMax())
		{
			return FALSE;	// (2)
		}

	}

	if (FALSE == IsInvolvedTbl())
	{
		return FALSE;	// No involved MV is using me
	}

	// Check that there is exactly one ON REQUEST MV using me
	// (which is also involved into the current execution).
	RUASSERT(NULL != pTblInterface_);
	
	CRUMVList &usingMVsList = pTblInterface_->GetOnRequestMVsUsingMe();

	if (usingMVsList.GetCount() != 1)
	{
		return FALSE;
	}

	nextCandidateUid = usingMVsList.GetAt(0)->GetUID();
	return TRUE;
}

//--------------------------------------------------------------------------//
//	CRUMV::CanPipelineToMe()
//	
//	CURScheduler::MaximizePipeliningPotential() callee
//
//	Can delta be pipelined TO this MV from downstream?
//
//	The conditions for pipelining are:
//	(1) The MV "theoretically" pipelineable, i.e., it does not 
//		perform recompute, and the MV's	query allows pipelining.
//	(2) The audit types of this task's MV and the downstream
//	    task's MV (bottom MV) are compatible.
//	(3) The MV does not have a MIN/MAX aggregate.
//	(4) The bottom MV's delta is the only one observed by the MV,
//	    AND the preliminary emptiness check (performed by the 
//	    bottom MV's EmpCheck task) showed that this only delta 
//		is also empty BEFORE the bottom MV is refreshed.
//
//--------------------------------------------------------------------------//

BOOL CRUMV::CanPipelineToMe(CRUMV &bottomMV) 
{
	if (FALSE == IsPipelineable()	// (1)
		||
		FALSE == IsCompatibleForPipelining(bottomMV) // (2)
		||
		TRUE == HasMinMaxAggregate()	// (3)
		) 
	{
		return FALSE;
	}

	// Start checking condition (4) ...
	if (FALSE == IsSingleDeltaRefresh())
	{
		// There is more than a single delta-def. When can this happen?
		// Recall that delta-defs can be removed only by the 
		// preceding EmpCheck tasks' completion handlers.
		// Therefore, either the used regular table(s) have non-empty
		// delta(s), or the MV is based on more than one involved MV.
		// In both cases, pipelining is impossible.
		return FALSE;
	}

	// Check the result of the bottom MV's preliminary emptiness check
	// (performed by the bottom MV's EmpCheck task).
	CRUTbl *pBottomMVTbl = bottomMV.GetTblInterface();
	if (TRUE == IsDeltaNonEmpty(*pBottomMVTbl))
	{
		// The delta is non-empty. Pipelining impossible.
		return FALSE;
	}

	return TRUE; 	// Go!
}

#ifdef _DEBUG
//--------------------------------------------------------------------------//
//	CRUMV::Dump()
//--------------------------------------------------------------------------//

void CRUMV::Dump(CDSString &to, BOOL isExtended) 
{
	char statusStr[10];

	to += "\nMATERIALIZED VIEW " + GetFullName() + " (";
	if (FALSE == IsInvolved()) 
		to += "not ";
	to += "involved into REFRESH)\n";

	sprintf(statusStr, "%d", GetStatus()); 
	to += "Status = ";
	to += statusStr;
	if (0 != GetStatus()) 
		to += "(error)";

	to += "\nTables used by me:\n";
	DSListPosition pos = GetTablesUsedByMe().GetHeadPosition();
	while (NULL != pos) 
	{
		CRUTbl *pTbl = GetTablesUsedByMe().GetNext(pos);
		to +=  ("\t" + (pTbl->GetFullName()) + "\n");
	}
}
#endif

//--------------------------------------------------------------------------//
//	MUTATORS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
// CRUMV::AddRefToUsedObject()
//
//	CALLED BY CRUCache::Build()
//
//	Update the usage lists
//--------------------------------------------------------------------------//

void CRUMV::AddRefToUsedObject(CRUTbl *pTbl)
{
	pTablesUsedByMe_->AddTail(pTbl);

	if (TRUE == pTbl->IsFullySynchronized())
	{
		pFullySyncTablesUsedByMe_->AddTail(pTbl);
	}
}

//--------------------------------------------------------------------------//
// CRUMV::CancelChanges()
//
//	CALLED BY CRUCache destructor
//
//	If changes were made to the object MV, but the user does not want 
//	to save them to the catalog - cancel the changes.
//
//--------------------------------------------------------------------------//

void CRUMV::CancelChanges()
{
	pddMV_->CancelChanges();

	DSListPosition pos = pddIndexList_->GetHeadPosition();
	while (NULL != pos)
	{
		CDDIndex *pddIndex = pddIndexList_->GetNext(pos);
		pddIndex->CancelChanges();
	}
}

//--------------------------------------------------------------------------//
// CRUMV::ReleaseResources()
//
// CALLED BY CRURcReleaseTaskExecutor
//
//--------------------------------------------------------------------------//

void CRUMV::ReleaseResources()
{
	if (TRUE == CanReleaseDDLLock())
	{
		ReleaseDDLLock();
	}
	else
	{
		// I cannot drop the DDL lock => the refresh did not pass successfully.
		// I am called from a task executor => I can throw exceptions!
		CRUException ex;
		ex.SetError(IDS_RU_DDLOCK_RELEASE_IMPOSSIBLE);
		ex.AddArgument(GetFullName());

		throw ex;
	}
}

//--------------------------------------------------------------------------//
//	CRUMV::FetchPrivileges()
//
//	CALLED BY CRUCache::Build()
//
//	Check whether the current user has Select/Insert/Delete privileges 
//	for this MV. The Insert privilege is required for the user to perform 
//	refresh. The Delete and Select privileges are required for performing
//	Purgedata from the MV and populating indexes on it.
//
//	Currently, is implemented as a direct read from the SMD tables.
//
//	THE CORRECT IMPLEMENTATION, HOWEVER, REQUIRES TURNING THE CHECK
//	INTO AN EXECUTOR FILE-OPEN. 
//
//--------------------------------------------------------------------------//

void CRUMV::FetchPrivileges()
{
	if (TRUE == FetchPrivilege(CDDObject::eSelect))
	{
		privMap_ |= SELECT_PRIV;
	}

	if (TRUE == FetchPrivilege(CDDObject::eInsert))
	{
		privMap_ |= INSERT_PRIV;
	}

	if (TRUE == FetchPrivilege(CDDObject::eDelete))
	{
		privMap_ |= DELETE_PRIV;
	}
}

//--------------------------------------------------------------------------//
//	CRUMV::BuildIndexList()
//
//	CALLED BY CRUUnAuditRefreshTaskExecutor
//
//	If the MV's indexes must undergo purgedata/popindex, 
//	the DDOL index objects must be retrieved.
//
//--------------------------------------------------------------------------//

void CRUMV::BuildIndexList()
{
	RUASSERT(TRUE == pddIndexList_->IsEmpty());
	pddMV_->FetchIndexUIDList();

	if (FALSE == pddMV_->HasIndexes())
	{
		return;
	}

	CDDIndexList *pAllIndexList = pddMV_->GetParent()->GetIndexList();
	DSListPosition pos = pAllIndexList->GetHeadPosition();

	while (NULL != pos)
	{
		CDDIndex *pddIndex = pAllIndexList->GetNext(pos);

		if (TRUE == pddMV_->HasIndex(pddIndex->GetUID() /* No I/O here !*/))
		{
			// I/O only for the relevant indexes !!!
			CDDMV *pddMV = pddIndex->GetMV();
			
			RUASSERT(
				pddMV_ == pddMV 
				&& 
				TRUE == pddIndex->GetDDLLockList()->IsEmpty());

			pddIndexList_->AddTail(pddIndex);
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUMV::GetPopIndexCatApiRequestText()
//
//	CALLED BY CRUUnAuditRefreshTaskExecutor
//
//	Generate the text of CatApi request for index population.
//	This is required for populating indexes in the task process 
//	(there is no DDOL there).
//
//--------------------------------------------------------------------------//

void CRUMV::GetPopIndexCatApiRequestText(CDSString &to, CDDIndex *pddIndex)
{
	RUASSERT(FALSE == pddIndexList_->IsEmpty());
	to = "";

	CDDLockList *pddLockList = GetDDLLockList();
	RUASSERT(FALSE == pddLockList->IsEmpty());

	CDSString ddlLockName = 
		GetCatName() + "." + GetSchName() + "." 
		+ pddLockList->GetAt(0)->GetName();

	CatApiRequest rqst(POPULATE_INDEX);
	rqst.addConstParam(pddIndex->GetFullName().c_string());

	// There is no DDL lock on the index
	RUASSERT(0 == pddIndex->GetDDLLockList()->GetCount());
	rqst.addConstParam("");

	// The name of the DDL lock on the MV
	rqst.addConstParam(ddlLockName);

	// The CatApi request must set the system default
	// to allow reading data from the MV, although it is non-available
	rqst.addParam(TRUE);

	char *cliRequest = new char[rqst.getTextLength()+1];
	rqst.getText(cliRequest);
	
	to += cliRequest;
	delete [] cliRequest;
}

//--------------------------------------------------------------------------//
//	CRUMV::GetUpdateIndexStatusCatApiRequestText()
//
//	CALLED BY CRUUnAuditRefreshTaskExecutor
//
//	Generate the text of CatApi request for turning the index status
//	to available/unavailable.
//
//--------------------------------------------------------------------------//

void CRUMV::GetUpdateIndexStatusCatApiRequestText(CDSString &to, 
												  BOOL isAvail,
												  CDDIndex *pddIndex)
{
	RUASSERT(FALSE == pddIndexList_->IsEmpty());
	to = "";

	CDDLockList *pddLockList = GetDDLLockList();
	RUASSERT(FALSE == pddLockList->IsEmpty());

	CDSString ddlLockName = 
		GetCatName() + "." + GetSchName() + "." 
		+ pddLockList->GetAt(0)->GetName();

	CatApiRequest rqst(UPDATE_INDEX_STATUS);
	rqst.addConstParam(pddIndex->GetFullName().c_string());

    if (TRUE == isAvail)
	{
		rqst.addParam(API_INDEX_AVAILABLE);
	}
    else
	{
		rqst.addParam(API_INDEX_NOT_AVAILABLE);
	}

	// There is no DDL lock on the index
	RUASSERT(0 == pddIndex->GetDDLLockList()->GetCount());
	rqst.addConstParam("");

	// The name of the DDL lock on the MV
	rqst.addConstParam(ddlLockName);

	char *cliRequest = new char[rqst.getTextLength()+1];
	rqst.getText(cliRequest);
	
	to += cliRequest;
	delete [] cliRequest;
}

//--------------------------------------------------------------------------//
//	CRUMV::GetToggleAuditCatApiRequestText()
//
//	CALLED BY CRUAuditRefreshTaskExecutor
//
//	Generate the text of CatApi request for turning the index audit 
//      flag on/off
//
//--------------------------------------------------------------------------//

void CRUMV::GetToggleAuditCatApiRequestText(CDSString &to, 
						BOOL auditFlag,
						CDDIndex *pddIndex)
{
	RUASSERT(FALSE == pddIndexList_->IsEmpty());
	to = "";

        // CatApi request to toggle audit attribute for object
	CatApiRequest rqst(TOGGLE_OBJ_AUDIT);

        // the object name
	rqst.addConstParam(pddIndex->GetFullName());

        // the object type
        rqst.addConstParam(API_INDEX);

        // the audit flag
        rqst.addParam(auditFlag);
      
        // There is no DDL lock on the index
	RUASSERT(0 == pddIndex->GetDDLLockList()->GetCount());
	rqst.addConstParam("");                     

	char *cliRequest = new char[rqst.getTextLength()+1];
	rqst.getText(cliRequest);
	
	to += cliRequest;
	delete [] cliRequest;
}


//--------------------------------------------------------------------------//
//	CRUMV::PurgeDataWithIndexes()
//
//	CALLED BY CRUUnAuditRefreshTaskExecutor
//
//--------------------------------------------------------------------------//

void CRUMV::PurgeDataWithIndexes()
{
	pddMV_->OpenPartitions(CUOFsFile::eExclusive, CUOFsFile::eReadWrite, TRUE, TRUE);
	pddMV_->PurgeDataPartitions();
	pddMV_->ClosePartitions();

	DSListPosition pos = pddIndexList_->GetHeadPosition();
	while (NULL != pos)
	{
		CDDIndex *pddIndex = pddIndexList_->GetNext(pos);

		pddIndex->OpenPartitions(CUOFsFile::eExclusive, CUOFsFile::eReadWrite, TRUE, TRUE);
		pddIndex->PurgeDataPartitions();
		pddIndex->ClosePartitions();
	}
}

//--------------------------------------------------------------------------//
//	CRUMV::AdvanceRecomputeEpoch()
//
//	Set MV.RECOMPUTE_EPOCH <-- MV.CURRENT_EPOCH
//	(let the MVs on me know that they should be recomputed too).
//
//	The update is performed through the CDDTblUsedByMV object 
//	referenced by the CDDMV object (hence, both should be saved).
//
//--------------------------------------------------------------------------//

void CRUMV::AdvanceRecomputeEpoch()
{
	CDDTblUsedByMV *pddTblUsedByMV = pddMV_->GetTblUsedByMV();

	TInt32 newRecompEpoch = pddTblUsedByMV->GetCurrentEpoch();
	pddTblUsedByMV->SetRecomputeEpoch(newRecompEpoch);
}

//--------------------------------------------------------------------------//
//	CRUMV::SetupRecomputeProperty()
//
//	CALLED BY CRUCache::Build()
//
//	Make an initial analysis whether the MV will be recomputed.  
//	Recompute will happen in two cases:
//	(1) The user has applied the utility with the RECOMPUTE option.
//	(2) The MV requires recompute based on its own properties.
//
//--------------------------------------------------------------------------//

void CRUMV::SetupRecomputeProperty(BOOL isTotalRecompute)
{
	if (TRUE == isTotalRecompute		// Case #1
		||
		TRUE == DoIDemandRecompute()	// Case #2
		)	
	{
		SetRefreshPatternMask(RECOMPUTE);
		
		// Consider using purgedata/popindex
		SetupPurgedataAndPopindex();
	}
}

//--------------------------------------------------------------------------//
//	CRUMV::PropagateRecomputeProperty()
//
//	CALLED BY CRUDependenceGraphBuilder::Build()
//
//	Now that the dependence graph's hierarchy is built, the RECOMPUTE
//	property can be propagated between the involved MVs.
//	
//--------------------------------------------------------------------------//

void CRUMV::PropagateRecomputeProperty()
{
	DSListPosition pos = GetTablesUsedByMe().GetHeadPosition();
	while (NULL != pos) 
	{
		CRUTbl *pTbl = GetTablesUsedByMe().GetNext(pos);

		if (TRUE == pTbl->IsInvolvedMV()
			&&
			TRUE == pTbl->GetMVInterface()->WillBeRecomputed()
			)
		{
			SetRefreshPatternMask(RECOMPUTE);
			break;
		}
	}

	// Now we are sure that the task executor will try 
	// to refresh the MV incrementally. 
	if (FALSE == WillBeRecomputed())
	{
		BuildDeltaDefList();
	}
}

//--------------------------------------------------------------------------//
//	CRUMV::PropagateEmpCheck()
//	
//	If the emptiness check shows that the delta of a certain used table 
//	is empty, drop the table from the delta-def list.
//
//	Otherwise, set the emptiness flags for the relevant logs in the delta-def.
//
//--------------------------------------------------------------------------//

void CRUMV::PropagateEmpCheck(CRUTbl &tbl)
{
	CRUEmpCheckVector &ecVector = tbl.GetEmpCheckVector();
	RUASSERT(TRUE == ecVector.IsFinal());

	TInt32 beginEpoch = GetEpoch(tbl.GetUID());
	
	BOOL isIUDLogNonEmpty = 
		ecVector.IsDeltaNonEmpty(beginEpoch, CRUTbl::SINGLE_ROW);

	BOOL isRangeLogNonEmpty = 
		ecVector.IsDeltaNonEmpty(beginEpoch, CRUTbl::RANGE);

	BOOL isIUDLogInsertOnly = 
		ecVector.IsDeltaInsertOnly(beginEpoch);

	if (FALSE == isIUDLogNonEmpty
		&&
		FALSE == isRangeLogNonEmpty)
	{
		pDeltaDefList_->RemoveByUID(tbl.GetUID());
	}
	else
	{
		CRUDeltaDef *pDdef = pDeltaDefList_->FindByUID(tbl.GetUID());

		pDdef->isIUDLogNonEmpty_   = isIUDLogNonEmpty;
		pDdef->isRangeLogNonEmpty_ = isRangeLogNonEmpty;
		pDdef->isIUDLogInsertOnly_ = isIUDLogInsertOnly;
	}	
}

//--------------------------------------------------------------------------//
//	CRUMV::PropagateDEStatistics()
//
//	Integrate the statistics collected by the DE task into the delta-def list. 
//	The DE task's statistics are stored in a hash table (map), an entry per 
//	begin-epoch of each MV using the table.
//
//	The MV must pick the hash entry relevant for itself, and store it in its 
//	own	delta-def strucure.
//
//--------------------------------------------------------------------------//

void CRUMV::PropagateDEStatistics(CRUTbl &tbl)
{
	CRUDeltaDef *pDdef = pDeltaDefList_->FindByUID(tbl.GetUID());

	if (NULL == pDdef)
	{
		// No delta-def => it has been removed 
		// at the emptiness check's completion. 
		// Hence, the DE statistics are irrelevant for me.
		return;
	}

	pDdef->deLevel_ = tbl.GetDELevel();

	// This is the only time to update the statistics ...
	RUASSERT(NULL == pDdef->pStat_);
	CRUDeltaStatistics stat;

	// Extract the value ...
	TInt32 ep = GetEpoch(tbl.GetUID());
	BOOL ret = tbl.GetStatisticsMap().Lookup(ep, stat);

	if (TRUE == ret)
	{
		// There are some statistics for this MV ...
		pDdef->pStat_ = new CRUDeltaStatistics(stat);
	}
}

//--------------------------------------------------------------------------//
//	PRIVATE AREA
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUMV::FetchPrivilege()
//
//	Check whether the user has a specific Select/Insert/Delete privilege
//--------------------------------------------------------------------------//

BOOL CRUMV::FetchPrivilege(CDDObject::EPrivilegeType privType)
{
	// Allow super.super and super.services to perform REFRESH
	// IsServices checks for both
	CUOSessionInfo sessionInfo (TRUE, FALSE, FALSE);
        if (IsSuperID() || sessionInfo.BelongsToSuperRole())
          return TRUE;

	if (sessionInfo.BelongsToServicesRole()) 
		return TRUE;

        CDDPrivilegeMgr *pPrivMgr = pddMV_->GetPrivilege();

        BOOL hasPriv = pPrivMgr->CheckUserPrivilege(privType);
        return hasPriv;
}

//--------------------------------------------------------------------------//
//	CRUMV::BuildDeltaDefList()
//
//	Setup() callee
//
//	Until there is a check which T-deltas are empty, it is assumed 
//	that every used table that is not IGNORE CHANGES - has a non-empty delta.
//
//--------------------------------------------------------------------------//

void CRUMV::BuildDeltaDefList()
{
	DSListPosition pos = GetTablesUsedByMe().GetHeadPosition();

	while (NULL != pos)
	{
		CRUTbl *pTbl = GetTablesUsedByMe().GetNext(pos);

		CRUDeltaDef *pDdef = new CRUDeltaDef(pTbl);

		pDdef->fromEpoch_ = GetEpoch(pTbl->GetUID());

		// The list will become the owner of the new object,
		// and will be responsible to dispose it.
		pDeltaDefList_->AddTail(pDdef);		
	}
}

//--------------------------------------------------------------------------//
//	CRUMV::IsPipelineable()
//
//	Do this MV's query properties allow pipelining the delta 
//	to/from it in the INTERNAL REFRESH command?
//
//	The rules are as follows:
//	(1) The MV will not be recomputed.
//	(2) The MV must be a MAV (on single table or on join).
//
//--------------------------------------------------------------------------//

BOOL CRUMV::IsPipelineable()
{
	if (TRUE == WillBeRecomputed())
	{
		return FALSE;	// (1)
	}

	CDDObject::EMVQueryType qt = GetQueryType();
	if (CDDObject::eMAV_ON_ST != qt && CDDObject::eMAV_ON_JOIN != qt)
	{
		return FALSE;	// (2)
	}

	return TRUE;
}

//--------------------------------------------------------------------------//
//	CRUMV::IsCompatibleForPipelining()
//
//	Are the two MVs compatible for pipelining the delta between them?
//	The rules are as follows:
//	(1) If the top MV (me) is audited, so should the bottom one be.
//	(2) If the top MV is multi-transactional, so should the bottom 
//	    one be, AND
//	    (2.1) The top MV's COMMIT EACH ... ROWS value must be greater
//		      than the bottom MV's one. (In the other words, it must
//		      be capable to accept the largest possible delta generated	      
//		      by the bottom MV).
//	(3) If the top MV is NO AUDIT/NO AUDITONREFRESH, the bottom MV 
//		should be one of these two types.
//
//--------------------------------------------------------------------------//

BOOL CRUMV::IsCompatibleForPipelining(CRUMV &bottomMV)
{
	CDDObject::EMVAuditType otherAT = bottomMV.GetAuditType();

	switch (GetAuditType())
	{
	case CDDObject::eAUDIT:
		{
			if (CDDObject::eAUDIT != otherAT)
			{
				return FALSE;	// (1)
			}
			
			// Both MVs are audited
			TInt32 nRows = GetCommitNRows();
			TInt32 otherNRows = bottomMV.GetCommitNRows();
			if (0 == nRows)
			{
				// Both MVs must be single-transactional
				return (0 == otherNRows);	// (2)
			}
			else
			{
				// Both MVs must be multi-transactional
				return (0 < otherNRows && otherNRows <= nRows);	// (2.1)
			}
		}

	case CDDObject::eNO_AUDIT:
	case CDDObject::eNO_AUDIT_ON_REFRESH:
		{
			return (CDDObject::eNO_AUDIT == otherAT		// (3)
					||
					CDDObject::eNO_AUDIT_ON_REFRESH == otherAT
					);
		}
	

	default: RUASSERT(FALSE);
	}

	// Just make the compiler happy
	return FALSE;
}

//--------------------------------------------------------------------------//
// CRUMV::DoIDemandRecompute()
//
//	The MV demands recompute in one of the following cases:
//
//  (1) The MV is RECOMPUTED or ON STATEMENT.
//	(2) The MV's status is not INITIALIZED.
//	(3) The MV is based on an ON REQUEST MV1, and MV1 has been recomputed 
//		since MV was refreshed last, i.e., MV.EPOCH[MV1] <= MV.RECOMPUTE_EPOCH.
//
//--------------------------------------------------------------------------//

BOOL CRUMV::DoIDemandRecompute()
{
	CDDObject::EMVRefreshType rt = GetRefreshType();
	if (CDDObject::eRECOMPUTE == rt || CDDObject::eON_STATEMENT == rt)
	{
		return TRUE;	// Case #1
	}
		 
	if (CDDObject::eINITIALIZED != GetMVStatus())
	{
		return TRUE;	// Case #2
	}

	DSListPosition pos = GetTablesUsedByMe().GetHeadPosition();
	while (NULL != pos) 
	{
		CRUTbl *pTbl = GetTablesUsedByMe().GetNext(pos);

		if (TRUE == pTbl->IsFullySynchronized()) 
		{
			// The used table is regular or ON STATEMENT MV
			continue;
		}
		
		TInt64 tblUid = pTbl->GetUID();
		if (GetEpoch(tblUid) <= pTbl->GetRecomputeEpoch())
		{
			return TRUE;	// Case #3
		}
	}

	return FALSE;
}

//--------------------------------------------------------------------------//
//	CRUMV::GetDeltaDefByUid
//
//--------------------------------------------------------------------------//
CRUDeltaDef* CRUMV::GetDeltaDefByUid(TInt64 uid) const
{
	CRUDeltaDefList &deltaDefList = GetDeltaDefList();
	DSListPosition pos = deltaDefList.GetHeadPosition();

	while (NULL != pos)
	{
		CRUDeltaDef *pDdef = deltaDefList.GetNext(pos);
		if (pDdef->tblUid_ == uid)
		{
			return pDdef;
		}
	}
	return NULL;
}

//--------------------------------------------------------------------------//
//	CRUMV::GetUsedTableByName
//
//--------------------------------------------------------------------------//
CRUTbl* CRUMV::GetUsedTableByName(const CDSString &tblName) const
{
	CRUTblList &tblList = GetTablesUsedByMe();
	
	DSListPosition pos = tblList.GetHeadPosition();

	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList.GetNext(pos);
		if (pTbl->GetFullName() == tblName)
		{
			return pTbl;
		}
	}
	return NULL;
}


//--------------------------------------------------------------------------//
//	CRUMV::SetupPurgedataAndPopindex
//
//	In the case of recompute, consider the Purgedata/Popindex operations.
//--------------------------------------------------------------------------//

void CRUMV::SetupPurgedataAndPopindex()
{
	RUASSERT(TRUE == WillBeRecomputed());
	
	CDDObject::EMVStatus status = GetMVStatus();

	BOOL isDeletePriv = (0 != (privMap_ & DELETE_PRIV));

	BOOL isPurgedata = FALSE;
	
	if (TRUE == MustPerformPurgedata(isDeletePriv))
	{
		isPurgedata = TRUE;
		SetRefreshPatternMask(PURGEDATA);
	}

	// INTERNAL REFRESH RECOMPUTE NODELETE will be performed in two cases:
	// (1) After purgedata
	// (2) If the MV is non-initialized
	if (TRUE == isPurgedata
		 ||
		 CDDObject::eNON_INITIALIZED == status)
	{
		SetRefreshPatternMask(NODELETE);

		// After RECOMPUTE NODELETE, indexes must be populated (if any)
		BuildIndexList();
	
		if (pddIndexList_->GetCount() > 0)
		{
			SetRefreshPatternMask(POPINDEX);
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUMV::MustPerformPurgedata()
// 
//	Purgedata will be performed if:
//	1. We have delete privs
//
//--------------------------------------------------------------------------//

BOOL CRUMV::MustPerformPurgedata(BOOL isDeletePriv)
{
        // if don't have delete privs, can't do purgedata
	return ( isDeletePriv );
}

// Define the CRUMVList through this macro

DEFINE_PTRLIST(CRUMV)
