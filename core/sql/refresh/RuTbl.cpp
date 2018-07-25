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
* File:         RuTbl.cpp
* Description:  Implementation of class CRUTbl.
*				
*
* Created:      02/27/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "dsstringlist.h"
#include "ddtable.h"

#include "RuTbl.h"
#include "RuMV.h"

#include "RuGlobals.h"
#include "RuKeyColumn.h"
#include "RuEmpCheckVector.h"
#include "RuSQLComposer.h"

//------------------------------------------------------------------------//
//							LOG-RELATED GLOBALS
//------------------------------------------------------------------------//

// Prefix of the control columns in the log
// (i.e., @EPOCH, @IGNORE etc).
CDSString const CRUTbl::logCrtlColPrefix = "@";

CDSString const CRUTbl::iudNmspPrefix = "IUD_LOG";

CDSString const CRUTbl::rngNmspPrefix = "RANGE_LOG";

//------------------------------------------------------------------------//
//							PUBLIC METHODS
//------------------------------------------------------------------------//

//------------------------------------------------------------------------//
//	Constructor and destructor of CRUTbl
//------------------------------------------------------------------------//

CRUTbl::CRUTbl(CDDTblUsedByMV *pddTblUsedByMV) :
	pddTblUsedByMV_(pddTblUsedByMV),
	pddIUDLogTbl_(NULL),

	// Usage lists
	pMVsUsingMe_(
		new CRUMVList(eItemsArentOwned)
	),
	pOnRequestMVsUsingMe_(
		new CRUMVList(eItemsArentOwned)
	),
	pInvolvedMVsUsingMe_(
		new CRUMVList(eItemsArentOwned)
	),
	pOnRequestInvolvedMVsUsingMe_(
		new CRUMVList(eItemsArentOwned)
	),
	pIncInvolvedMVsUsingMe_(
		new CRUMVList(eItemsArentOwned)
	),

	pMVInterface_(NULL),
	timestamp_(0),

	// Data structures for info exchange
	pKeyColumnList_(new CRUKeyColumnList()),
	pPartitionFileNamesList_(new CDSStringList()),
	pEmpCheckVector_(new CRUEmpCheckVector()),
	pStatMap_(new CRUDeltaStatisticsMap()),
	deLevel_(CRUDeltaDef::NO_DE),
	
	// Flags
	isRPOpenPending_(FALSE),
	isLogRPOpenPending_(FALSE),
	isUsedByIncrementalMJV_(FALSE),
	isDENeeded_(FALSE),
	isLongLockNeeded_(FALSE),
	isEmptyDeltaNeeded_(FALSE)
{}

CRUTbl::~CRUTbl()
{
	// Since the lists do not own the referenced CRUMV objects,
	// only the pointers will be released (not the objects themselves)
	delete pMVsUsingMe_;

	delete pOnRequestMVsUsingMe_;
	delete pInvolvedMVsUsingMe_;
	delete pOnRequestInvolvedMVsUsingMe_;
	delete pIncInvolvedMVsUsingMe_;

	delete pKeyColumnList_;

	delete pPartitionFileNamesList_;

	delete pEmpCheckVector_;

	delete pStatMap_;
}

//------------------------------------------------------------------------//
//		ACCESSORS
//------------------------------------------------------------------------//

//------------------------------------------------------------------------//
//	CRUTbl::IsUsedByOnRequestMV()
//------------------------------------------------------------------------//

BOOL CRUTbl::IsUsedByOnRequestMV() const
{
	return (GetOnRequestInvolvedMVsUsingMe().GetCount() > 0);
}

//------------------------------------------------------------------------//
//	CRUTbl::IsUsedByIncRefreshedMV()
//------------------------------------------------------------------------//

BOOL CRUTbl::IsUsedByIncRefreshedMV() const
{
	return (GetIncrementalInvolvedMVsUsingMe().GetCount() > 0);
}

//------------------------------------------------------------------------//
//	CRUTbl::IsInsertLog()
//
//	A table is insert-only if it has an INSERTLOG or 
//	MANUAL RANGELOG attribute.
//------------------------------------------------------------------------//

BOOL CRUTbl::IsInsertLog()
{
	return (TRUE == pddTblUsedByMV_->IsInsertLog() 
			||
			CDDObject::eMANUAL == GetRangeLogType()
			);
}

//------------------------------------------------------------------------//
//	CRUTbl::GetLogShortName()
//
//	Generate the short name of the IUD/range log (not FQ, without namespace).
//------------------------------------------------------------------------//

CDSString CRUTbl::GetLogShortName(const CDSString &nmsp)
{
//	Suffix removed from all the logs.
//	CDSString logNameSuffix("__" + nmsp);
	CDSString logNameSuffix("");
	CDSString name(this->GetName());
	
	CRUSQLComposer::AddSuffixToString(logNameSuffix, name);
	return name;
}

//------------------------------------------------------------------------//
//	CRUTbl::GetIUDLogContentTypeBitmap()
//------------------------------------------------------------------------//

Lng32 CRUTbl::GetIUDLogContentTypeBitmap()
{	
	Lng32 contentType = 0;

	CDDObject::ERangeLogType rlType = GetRangeLogType();

	if (CDDObject::eNONE != rlType)
	{
		contentType |= RANGE;
	}

	if (CDDObject::eMANUAL != rlType)
	{
		contentType |= SINGLE_ROW;
	}

	return contentType;
}

//------------------------------------------------------------------------//
//	CRUTbl::GetUpdateBitmapSize()
//------------------------------------------------------------------------//

Lng32 CRUTbl::GetUpdateBitmapSize()
{
	RUASSERT (NULL != pddIUDLogTbl_);

	CDSString colName("UPDATE_BITMAP");
	colName = CRUSQLComposer::ComposeQuotedColName(
		CRUTbl::logCrtlColPrefix, colName
	);

	CDDColumn *pCol = pddIUDLogTbl_->GetColumn(colName);

	RUASSERT (NULL != pCol);
	return pCol->GetSize();
}

//------------------------------------------------------------------------//
//	CRUTbl::IsDeltaNonEmpty()
//
//	Is the delta empty starting from this epoch?
//------------------------------------------------------------------------//

BOOL CRUTbl::IsDeltaNonEmpty(TInt32 beginEpoch)
{
	return GetEmpCheckVector().IsDeltaNonEmpty(beginEpoch);
}

//------------------------------------------------------------------------//
//	CRUTbl::IsUsedOnlyByMultiTxnMvs()
//
//	Are all the incrementally refreshed MVs on top of this table 
//	multi-transactional (i.e., COMMIT EACH > 0)?
//------------------------------------------------------------------------//

BOOL CRUTbl::IsUsedOnlyByMultiTxnMvs() const
{
	if (TRUE == pIncInvolvedMVsUsingMe_->IsEmpty())
	{
		return FALSE;
	}

	DSListPosition pos = pIncInvolvedMVsUsingMe_->GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = pIncInvolvedMVsUsingMe_->GetNext(pos);
	
		RUASSERT(
			CDDObject::eON_REQUEST == pMV->GetRefreshType()
			&&
			TRUE == pMV->IsInvolved()
		);

		if (0 == pMV->GetCommitNRows())
		{
			return FALSE;
		}
	}

	return TRUE;
}

#ifdef _DEBUG
//------------------------------------------------------------------------//
//	CRUTbl::Dump()
//------------------------------------------------------------------------//
void CRUTbl::Dump(CDSString &to, BOOL isExtended) 
{
	char statusStr[10];

	to += "\nUSED OBJECT " + GetFullName() + " (";
	if (TRUE == IsRegularTable())
	{
		to += "regular table)\n";
	}
	else 
	{
		to += "materialized view)\n";
	}

	sprintf(statusStr, "%d", GetStatus()); 
	to += "Status = ";
	to += statusStr;
	if (0 != GetStatus()) 
		to += "(error)";

	to += "\nMVs using me:\n";
	DSListPosition pos = GetMVsUsingMe().GetHeadPosition();
	while (NULL != pos) 
	{
		CRUMV *pMV = GetMVsUsingMe().GetNext(pos);
		to += "\t" + (pMV->GetFullName()) + "\n";
	}
}
#endif

//------------------------------------------------------------------------//
//		MUTATORS
//------------------------------------------------------------------------//

//------------------------------------------------------------------------//
//	CRUTbl::AddRefToUsingMV()
//
//	CALLED BY: CRUCache
//
//	Update the usage lists
//------------------------------------------------------------------------//

void CRUTbl::AddRefToUsingMV(CRUMV *pMV)
{
	// Update the main list
	pMVsUsingMe_->AddTail(pMV);

	// Update the auxiliary lists
	BOOL isOnRequest = (CDDObject::eON_REQUEST == pMV->GetRefreshType());
	BOOL isInvolved = pMV->IsInvolved();

	if (TRUE == isOnRequest)
	{
		pOnRequestMVsUsingMe_->AddTail(pMV);
	}

	if (TRUE == isInvolved)
	{
		pInvolvedMVsUsingMe_->AddTail(pMV);
	}
	
	if (TRUE == isOnRequest && TRUE == isInvolved)
	{
		pOnRequestInvolvedMVsUsingMe_->AddTail(pMV);
	}
}

//------------------------------------------------------------------------//
//	CRUTbl::ReleaseResources()
//
//	CALLED BY CRURcReleaseTaskExecutor
//
//	Release the DDL lock + read-protected open(s), if exist.
//	
//------------------------------------------------------------------------//

void CRUTbl::ReleaseResources()
{
	if (TRUE == IsDDLLockPending())
	{
		if (TRUE == CanReleaseDDLLock())
		{
			ReleaseDDLLock();	// This is generally the case
		}
		else
		{
			// The table is a non-involved NON-AUDITED/MULTI-TXN MV,
			// which carried the DDL lock from some previous
			// invocation of Refresh. 
			// Take care not to drop it accidentally!
			RUASSERT(FALSE == IsFullySynchronized());
		}
	}

	// The log's read-protected open (if there was one)
	// should have been released by the TableSync task.
	// However, if TableSync has crashed, I must do the job.
	if (TRUE == IsLogRPOpenPending())
	{
		ReleaseLogReadProtectedOpen();
	}

	if (TRUE == IsRPOpenPending())
	{
		ReleaseReadProtectedOpen();
	}
}

//------------------------------------------------------------------------//
//	CRUTbl::BuildListOfIncrementalInvolvedMVsUsingMe()
//
//	CALLER: CRUDependenceGraphBuilder
//
//	ASSUMPTION: This method is called after the MV's RECOMPUTE attribute
//	propagation through the graph.
//
//------------------------------------------------------------------------//

void CRUTbl::BuildListOfIncrementalInvolvedMVsUsingMe()
{
	// Verify that the list is not initialized already
	RUASSERT(TRUE == pIncInvolvedMVsUsingMe_->IsEmpty());

	DSListPosition pos = pOnRequestInvolvedMVsUsingMe_->GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = pOnRequestInvolvedMVsUsingMe_->GetNext(pos);
		RUASSERT(CDDObject::eON_REQUEST == pMV->GetRefreshType()
				 &&
				 TRUE == pMV->IsInvolved()
		);

		if (TRUE == pMV->WillBeRecomputed()) 
		{	
			continue;
		}

		pIncInvolvedMVsUsingMe_->AddTail(pMV);

		CDDObject::EMVQueryType qt = pMV->GetQueryType();
		if (CDDObject::eMJV == qt
			||
			CDDObject::eOTHER == qt	// Is this MJV on single table?
			)
		{
			isUsedByIncrementalMJV_ = TRUE;
		}
	}
}

//------------------------------------------------------------------------//
//	CRUTbl::PropagateEmpCheckToUsingMVs()
//
//	CALLED BY: {CRUEmpCheckTask/CRURefreshTask}::PullDataFromExecutor()
//
//	When the emptiness check's vector becomes final,
//	trigger the update of the using MVs' delta-def lists.
//
//------------------------------------------------------------------------//

void CRUTbl::PropagateEmpCheckToUsingMVs()
{
	RUASSERT(TRUE == GetEmpCheckVector().IsFinal());

	DSListPosition pos = pIncInvolvedMVsUsingMe_->GetHeadPosition();
	while (NULL != pos) 
	{
		CRUMV *pMV = pIncInvolvedMVsUsingMe_->GetNext(pos);

		pMV->PropagateEmpCheck(*this);
	}	
}

//------------------------------------------------------------------------//
//	CRUTbl::PropagateRecomputeToUsingMVs()
//
//	CALLED BY: CRURefreshTask::PullDataFromExecutor()
//
//	If the table is itself an MV that has been just recomputed -
//	propagate this attribute to the using MVs
//	
//------------------------------------------------------------------------//

void CRUTbl::PropagateRecomputeToUsingMVs()
{
	RUASSERT(TRUE == IsInvolvedMV()
			 &&
			 TRUE == GetMVInterface()->WillBeRecomputed());

	DSListPosition pos = pIncInvolvedMVsUsingMe_->GetHeadPosition();
	while (NULL != pos) 
	{
		CRUMV *pMV = pIncInvolvedMVsUsingMe_->GetNext(pos);
		pMV->SetRefreshPatternMask(CRUMV::RECOMPUTE);
	}	
}

//---------------------------------------------------------//
//	CRUTbl::PropagateDEStatisticsToUsingMVs()
//
//	CRURefreshExector::Init() callee. 
//	
//	Adopt the statistics fetched by the DE task.
//---------------------------------------------------------//

void CRUTbl::PropagateDEStatisticsToUsingMVs()
{
	DSListPosition pos = pIncInvolvedMVsUsingMe_->GetHeadPosition();
	while (NULL != pos) 
	{
		CRUMV *pMV = pIncInvolvedMVsUsingMe_->GetNext(pos);

		pMV->PropagateDEStatistics(*this);
	}	
}

//------------------------------------------------------------------------//
//	REQUIREMENTS CHECKS
//------------------------------------------------------------------------//

//------------------------------------------------------------------------//
//	CRUTbl::CheckIfLongLockNeeded()
//
//	CALLED BY: CRUTableSyncExecutor
//
//	Compute whether the the table must be locked (by read-protected open)
//	as long as the MVs directly using it are refreshed, to achieve 
//	consistency between the table and the MVs. The table must be locked
//	in 3 cases:
//	(1) The table has an AUTOMATIC/MIXED RANGELOG attribute.
//	(2) Some ON REQUEST MV using the table is not self-maintainable.
//	(3) Some ON REQUEST MV using the table will be recomputed.
//
//	If the table must be locked because it is involved into an indirect
//	join through RECOMPUTED MVs, the flag will be set by the 
//	EquivSetBuilder.
//
//------------------------------------------------------------------------//

void CRUTbl::CheckIfLongLockNeeded()
{
	if (TRUE == IsLongLockNeeded())
	{
		// Someone else (the equivalence set analyzer)
		// has already set the flag.
		return;	
	}

	CDDObject::ERangeLogType rlType = GetRangeLogType();

	if (CDDObject::eAUTOMATIC == rlType	// Case #1
		||
		CDDObject::eMIXED == rlType)
	{
		SetLongLockIsNeeded();
		return;
	}

	DSListPosition pos = pOnRequestInvolvedMVsUsingMe_->GetHeadPosition();
	while (NULL != pos) 
	{
		CRUMV *pMV = pOnRequestInvolvedMVsUsingMe_->GetNext(pos);
		
		if (FALSE == pMV->IsSelfMaintainable() // Case #2
			||
			TRUE  == pMV->WillBeRecomputed() // Case #3
 		   )
		{
			SetLongLockIsNeeded();
			return;
		}
	}
}

//------------------------------------------------------------------------//
//	CRUTbl::CheckIfDENeeded()
//
//	CALLED BY: CRUDependenceGraphBuilder
//	
//	Decide whether duplicate elimination must be done on the log 
//	of this table (and hence, whether the DE task must be created).
// 
//	This should happen if:
//	(1) There is at least one MV on this table that is being 
//		refreshed incrementally, 
//	AND
//	(2) There is no single-delta restriction which requires 
//	    the delta to be empty
//	AND
//	(3) Either the table has a range log (3.1), OR one using 
//		incremental MV is an MJV (3.2)
//
//------------------------------------------------------------------------//

void CRUTbl::CheckIfDENeeded()
{
	isDENeeded_ = FALSE;

	if (FALSE == IsUsedByIncRefreshedMV()	// Condition #1
		||
		TRUE == IsEmptyDeltaNeeded()		// Condition #2
		)
	{
		return;
	}

	if (CDDObject::eNONE != GetRangeLogType()	// Condition #3.1
		||
		TRUE == IsUsedByIncrementalMJV()		// Condition #3.2
		)
	{
		isDENeeded_ = TRUE;
	}
}

//------------------------------------------------------------------------//
//	CRUTbl::BuildEmpCheckVector()
//
//	CALLED BY: CRUEmpCheckTask
//
//	Build the emptiness check vector, in the following way.
//	For each incrementally refreshed MV on the table, 
//	add MV.EPOCH[T] to the vector.
//
//------------------------------------------------------------------------//

void CRUTbl::BuildEmpCheckVector()
{
	// Verify that the vector is non-initialized
	RUASSERT(FALSE == pEmpCheckVector_->IsValid());

	// CRUEmpCheckTask is created only if there is
	// at least one incrementally refreshed MV.
	RUASSERT(FALSE == pIncInvolvedMVsUsingMe_->IsEmpty());

	TInt64 tblUid = GetUID();
	
	DSListPosition lpos = pIncInvolvedMVsUsingMe_->GetHeadPosition();
	while (NULL != lpos)
	{
		CRUMV *pMV = pIncInvolvedMVsUsingMe_->GetNext(lpos);	
		RUASSERT (TRUE == pMV->IsInvolved() 
				  && 
				  FALSE == pMV->WillBeRecomputed());

		// Register the epoch in the vector ...
		pEmpCheckVector_->AddEpochForCheck(pMV->GetEpoch(tblUid));
	}	

	pEmpCheckVector_->Build();
}

//------------------------------------------------------------------------//
//	CRUTbl::BuildPartitionFileNamesList()
//
//	Build the list of partitions names
//------------------------------------------------------------------------//

void CRUTbl::BuildPartitionFileNamesList()
{
	if (FALSE == pPartitionFileNamesList_->IsEmpty())
	{
		// The list is already initialized 
		return;
	}

	CUOFsFileList *pddPartitionFileList = GetPartitionFileList();

	RUASSERT(NULL != pddPartitionFileList);

	DSListPosition pos = pddPartitionFileList->GetHeadPosition();
	while (NULL != pos)
	{
		CUOFsFile *pFile = pddPartitionFileList->GetNext(pos);
		
		CDSString *pName = new CDSString(pFile->GetFileName());

		pPartitionFileNamesList_->AddTail(pName);
	}
}

//------------------------------------------------------------------------//
//	CRUTbl::getNumberOfPartitions()
//
//	Return the number of partitions of this table.
//------------------------------------------------------------------------//

Lng32 CRUTbl::getNumberOfPartitions()
{
  return GetPartitionFileList()->GetCount();
}

//------------------------------------------------------------------------//
//	CRUTbl::BuildKeyColumnList()
//
//	Build the list of key columns (for the DE purposes).
//	The CRUKeyColumn object combines the properties
//	of CDDColumn (name, type string) and of CDDKeyColumn(sort order).
//
//------------------------------------------------------------------------//

void CRUTbl::BuildKeyColumnList()
{
	// Verify that the list is non-initialized
	if (FALSE == pKeyColumnList_->IsEmpty())
	{
		return;
	}

	CDDKeyColumnList *pddKeyColList = GetDDKeyColumnList();
	CDDColumnList *pddColList =	GetDDColumnList();

	DSListPosition pos = pddKeyColList->GetHeadPosition();
	while (NULL != pos)
	{
		CDDKeyColumn *pddKeyCol = pddKeyColList->GetNext(pos);

		CDDColumn *pddCol = pddColList->Find(pddKeyCol->GetName());
		RUASSERT(NULL != pddCol);
		
		CRUKeyColumn *pKeyCol = new CRUKeyColumn(pddCol, pddKeyCol);
		pKeyColumnList_->AddTail(pKeyCol);
	}
}

//-------------------------------------------------------------------//
//	READ-PROTECTED OPENS OF THE TABLE AND THE LOG: THE BASIS
//-------------------------------------------------------------------//

//-------------------------------------------------------------------//
//	CRUTbl::ExecuteReadProtectedOpen()
//
//	The actual action will be performed by the child classes.
//-------------------------------------------------------------------//

void CRUTbl::ExecuteReadProtectedOpen()
{
	RUASSERT(FALSE == isRPOpenPending_);

#ifdef _DEBUG
	CDSString msg("Locking table " + GetFullName() + "\n");
	
	CRUGlobals::GetInstance()->
		LogDebugMessage(CRUGlobals::DUMP_LOCKS, "", msg, TRUE);
#endif

	isRPOpenPending_ = TRUE;
}

//-------------------------------------------------------------------//
//	CRUTbl::ReleaseReadProtectedOpen()
//
//	The actual action will be performed by the child classes.
//-------------------------------------------------------------------//

void CRUTbl::ReleaseReadProtectedOpen()
{
	RUASSERT(TRUE == isRPOpenPending_);

#ifdef _DEBUG
	CDSString msg("UnLocking table " + GetFullName() + "\n");
	
	CRUGlobals::GetInstance()->
		LogDebugMessage(CRUGlobals::DUMP_LOCKS, "", msg, TRUE);
#endif

	isRPOpenPending_ = FALSE;
}

//-------------------------------------------------------------------//
//	CRUTbl::ExecuteLogReadProtectedOpen() 
//-------------------------------------------------------------------//

void CRUTbl::ExecuteLogReadProtectedOpen() 
{
	RUASSERT(NULL != pddIUDLogTbl_ && FALSE == isLogRPOpenPending_);

#ifdef _DEBUG
	CDSString msg("Locking log " + GetFullName() + "\n");
	
	CRUGlobals::GetInstance()->
		LogDebugMessage(CRUGlobals::DUMP_LOCKS, "", msg, TRUE);
#endif

#ifdef NA_LINUX
	pddIUDLogTbl_->
		OpenPartitions(CUOFsFile::eProtected, CUOFsFile::eReadOnly, 
                      TRUE, FALSE);//default clearVSN; do not clear transactions
#else

	pddIUDLogTbl_->
		OpenPartitions(CUOFsFile::eProtected, CUOFsFile::eReadOnly, TRUE, TRUE, TRUE);
#endif //NA_LINUX

	isLogRPOpenPending_ = TRUE;
}

//-------------------------------------------------------------------//
//	CRUTbl::ReleaseLogReadProtectedOpen()
//-------------------------------------------------------------------//

void CRUTbl::ReleaseLogReadProtectedOpen()
{
	RUASSERT(NULL != pddIUDLogTbl_ && TRUE == isLogRPOpenPending_);

#ifdef _DEBUG
	CDSString msg("UnLocking log " + GetFullName() + "\n");
	
	CRUGlobals::GetInstance()->
		LogDebugMessage(CRUGlobals::DUMP_LOCKS, "", msg, TRUE);
#endif

	pddIUDLogTbl_->ClosePartitions();
	
	isLogRPOpenPending_ = FALSE;
}

//------------------------------------------------------------------------//
//							PRIVATE METHODS
//------------------------------------------------------------------------//

//------------------------------------------------------------------------//
//	CRUTbl::GetLogFullName()
//
//	Generate the full name of the IUD/range log (with/without the namespace).
//------------------------------------------------------------------------//

CDSString CRUTbl::GetLogFullName(const CDSString &nmsp, BOOL useNmsp)
{
	CDSString logName("");
	
	if (TRUE == useNmsp)
	{
		logName += "TABLE(" + nmsp + "_TABLE ";
	}

	logName += 
		GetCatName() + "." + GetSchName() + "." 
		+ GetLogShortName(nmsp);

	if (TRUE == useNmsp)
	{
		logName += ")";
	}

	return logName;
}

// Define the CRUTblList through this macro
DEFINE_PTRLIST(CRUTbl)
