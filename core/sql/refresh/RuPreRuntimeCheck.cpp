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
* File:         RuPreRuntimeCheck.cpp
* Description:  Implementation of class CRUPreRuntimeCheck
*
* Created:      10/10/2000
* Language:     C++
*
*
******************************************************************************/

#include "RuPreRuntimeCheck.h"
#include "RuOptions.h"
#include "RuGlobals.h"
#include "RuRefreshTask.h"
#include "RuTbl.h"
#include "uosessioninfo.h"

//--------------------------------------------------------------------------//
//	Constructor
//--------------------------------------------------------------------------//

CRUPreRuntimeCheck::CRUPreRuntimeCheck(CRURefreshTask &refreshTask) :
	refreshTask_(refreshTask),
	mv_(refreshTask_.GetRootMV()),
	mvName_(mv_.GetFullName()),
	errDesc_(refreshTask_.GetErrorDesc())
{}

//--------------------------------------------------------------------------//
//	CRUPreRuntimeCheck::PerformCheck()
//
//	Perform all the pre-runtime check stages
//--------------------------------------------------------------------------//

void CRUPreRuntimeCheck::PerformCheck()
{
	// 1. Check that there are no problems
	// about the DDL locks on the MV(and indexes on it) and/or used objects.
	if (FALSE == CheckDDLLockErrors())
	{
		return;
	}

	// 2. In order to perform the refresh, the user
	// must have the INSERT priviledge for this MV.
	// In order to perform purgedata, the user must have
	// the SELECT privilege for this MV.
	// In order to populate the index, the user must have
	// the SELECT and DELETE privileges for this MV.
	
        if (FALSE == CheckPrivileges())
	{
		return;
	}

	// 3. Check that all the used objects are initialized
	if (FALSE == CheckUsedObjectsInitialization())
	{
		return;
	}

	// 4. The user can recompute an ON REQUEST/ON STATEMENT MV
	// only if it is not based on NO LOCKONREFRESH tables.
	if (FALSE == CheckUsedTablesNoLockOnRefresh())
	{
		return;
	}


	// 5. Check that an ON STATEMENT MV is not re-initialized.
	if (FALSE == CheckOnStatementMVReInit())
	{
		return;
	}

	// 6. Verify that after the refresh 
	// the MV will reflect a correct database snapshot.
	if (FALSE == CheckConsistency())
	{
		return;
	}

	// 7. If the MV is incremental, verify that 
	//    all the used regular tables are audited.
	if (FALSE == CheckUsedTablesAuditing())
	{
		return;
	}

	// 8. Check that if a non-available non-audited MV
	//    is recomputed, the user must specify 
	//    the RECOMPUTE option explicitly.
	CheckNonAvailableMVRecompute();
}

//--------------------------------------------------------------------------//
//	CRUPreRuntimeCheck::CheckDDLLockErrors()
//
//	Check the error objects associated with the MV and the tables used by it. 
//
//	If the error objects carry a non-zero (error) code,
//	this is the consequence of failure to acquire/replace a DDL lock
//	during the cache construction. In this case, the task will fail.
//	
//--------------------------------------------------------------------------//

BOOL CRUPreRuntimeCheck::CheckDDLLockErrors()
{
	if (0 != mv_.GetStatus())
	{
		// Copy the MV's error description
		errDesc_ = mv_.GetErrorDesc();
		return FALSE;
	}

	CRUTblList &tblList = mv_.GetTablesUsedByMe();
	DSListPosition pos = tblList.GetHeadPosition();

	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList.GetNext(pos);
		if (0 != pTbl->GetStatus())
		{
			// Copy the table's error description
			errDesc_ = pTbl->GetErrorDesc();
			return FALSE;
		}
	}

	return TRUE;
}

//--------------------------------------------------------------------------//
//	CRUPreRuntimeCheck::CheckOnStatementMVReInit()
//--------------------------------------------------------------------------//

BOOL CRUPreRuntimeCheck::CheckOnStatementMVReInit()
{
	if (CDDMV::eON_STATEMENT == mv_.GetRefreshType()
		&&
		CDDObject::eINITIALIZED == mv_.GetMVStatus()
		)
	{
		errDesc_.SetError(IDS_RU_ON_STATEMENT_REINIT);
		errDesc_.AddArgument(mvName_);
		return FALSE;
	}	

	return TRUE;
}

//--------------------------------------------------------------------------//
//	CRUPreRuntimeCheck::CheckPrivileges()
//
//	(1) INSERT privilege for refresh in general.
//	(2) SELECT privilege for popindex.
//	(3) DELETE and SELECT privileges for purgedata.
//
//--------------------------------------------------------------------------//

BOOL CRUPreRuntimeCheck::CheckPrivileges()
{
	// Allow super.super and super.services to perform REFRESH
	// IsServices checks for both
        CUOSessionInfo sessionInfo (TRUE, FALSE, FALSE);
        if (sessionInfo.BelongsToServicesRole())
		return TRUE;

	Lng32 priv = mv_.GetPrivMap();
	BOOL isInsertPriv = (0 != (priv & CRUMV::INSERT_PRIV));
	BOOL isSelectPriv = (0 != (priv & CRUMV::SELECT_PRIV));
	BOOL isDeletePriv = (0 != (priv & CRUMV::DELETE_PRIV));

	if (FALSE == isInsertPriv)
	{
		errDesc_.SetError(IDS_RU_NO_REFRESH_PRIVILGE);
		errDesc_.AddArgument(mvName_);
		return FALSE;
	}

	Lng32 refreshPattern = mv_.GetRefreshPatternMap();
	BOOL isPurgedata = (0 != (refreshPattern & CRUMV::PURGEDATA));
	BOOL isPopindex = (0 != (refreshPattern & CRUMV::POPINDEX));

	if (TRUE == isPurgedata || 
		CDDObject::eNON_INITIALIZED == mv_.GetMVStatus() ||
		CDDObject::eAUDIT != mv_.GetAuditType())
	{
		if (FALSE == isSelectPriv || FALSE == isDeletePriv)
		{
			// We need select and delete privs in 3 cases :
			//	1. A purgedata must be performed after a failure
			//  2. The MV is not initialize and initialization is needed.
			//     If the initialization process will fail then the purge data 
			//     is a must. In order to avoid entering the usr into a dead end
			//     we require at the begining of the process all the might needed 
			//     privileges
			//  3. A No audit on refresh MV is refreshed. In this case any failure 
			//     in the refresh process will demand that in the following refresh 
			//     a purgedata will be executed and the mv will be recomputed
			errDesc_.SetError(IDS_RU_NO_PURGEDATA_PRIVILEGE);
			errDesc_.AddArgument(mvName_);
			return FALSE;
		}
	}

	if (TRUE == isPopindex
		&&
		FALSE == isSelectPriv)
	{
		// Index population is a must, not an optimization
		RUASSERT(CDDObject::eNON_INITIALIZED == mv_.GetMVStatus());

		errDesc_.SetError(IDS_RU_NO_POPINDEX_PRIVILEGE);
		errDesc_.AddArgument(mvName_);
		return FALSE;
	}

	return TRUE;
}

//--------------------------------------------------------------------------//
//	CRUPreRuntimeCheck::CheckUsedObjectsInitialization()
//--------------------------------------------------------------------------//

BOOL CRUPreRuntimeCheck::CheckUsedObjectsInitialization()
{
	CRUTblList &tblList = mv_.GetTablesUsedByMe();
	DSListPosition pos = tblList.GetHeadPosition();

	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList.GetNext(pos);
		
		if (FALSE == pTbl->IsInitialized()
			&&
			FALSE == pTbl->IsInvolvedMV())
		{
			// If a used object is regular table, it is always initialized.
			// The check is only relevant for a table's implementation by an MV.
			errDesc_.SetError(IDS_RU_USED_MV_UNINITIALIZED);
			errDesc_.AddArgument(mvName_);
			
			return FALSE;
		}
	}

	return TRUE;
}

//--------------------------------------------------------------------------//
//	CRUPreRuntimeCheck::CheckUsedTablesNoLockOnRefresh()
//
//	The user can recompute an ON REQUEST/ON STATEMENT MV
//	only if it is NOT based on NO LOCKONREFRESH tables.
//
//	This limitation follows from the need to lock the table
//	during the incremental MV's recompute (see CRUTbl::IsLongLockNeeded()),
//	which contradicts the NO LOCKONREFRESH attribute.
//
//--------------------------------------------------------------------------//

BOOL CRUPreRuntimeCheck::CheckUsedTablesNoLockOnRefresh()
{
	if (FALSE == mv_.WillBeRecomputed()
		||
		CDDObject::eRECOMPUTE == mv_.GetRefreshType())
	{
		return TRUE;
	}

	CRUTblList &tblList = mv_.GetTablesUsedByMe();
	DSListPosition pos = tblList.GetHeadPosition();

	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList.GetNext(pos);
		if (TRUE == pTbl->IsNoLockOnRefresh() )
		{
			errDesc_.SetError(IDS_RU_RECOMPUTE_NOLOCKONREFRESH);
			errDesc_.AddArgument(mvName_);
			return FALSE;
		}
	}

	return TRUE;
}

//--------------------------------------------------------------------------//
//	CRUPreRuntimeCheck::CheckConsistency()
//
//  - ON REQUEST and RECOMPUTE MVs that are based on 
//	  more than one MV cannot be refreshed standalone.
//  - RECOMPUTED MV based on some other MV cannot have 
//    any other object beneath it.
//  - ON REQUEST MV based on some other MV must observe 
//    empty table-deltas (this check can be done only in 
//	  runtime, after the EmpCheck task).
//
//--------------------------------------------------------------------------//

BOOL CRUPreRuntimeCheck::CheckConsistency()
{
	if (CRUOptions::SINGLE_MV != 
		CRUGlobals::GetInstance()->GetOptions().GetInvocType())
	{
		return TRUE;	// Check irrelevant for group refresh
	}

	Int32 nNonFullySyncTablesUsedByMe = 
		mv_.GetTablesUsedByMe().GetCount() 
		-
		mv_.GetFullySyncTablesUsedByMe().GetCount();

	switch (nNonFullySyncTablesUsedByMe)
	{
	case 0:	
		{
			// The MV is based only on regular tables.
			// No consistency check is required whatsoever.
			return TRUE;
		}
	
	case 1:
		{
			return CheckConsistencyIfSingleMVUsed();
		}

	default:	// More than one directly used MV
		{
			// ON REQUEST and RECOMPUTE MVs that are based on more 
			// than one MV non-ON-STATEMENT MV cannot be 
			// refreshed standalone (in a single-MV refresh)
			errDesc_.SetError(IDS_RU_INCONSISTENT_JOIN);
			errDesc_.AddArgument(mvName_);
			return FALSE;
		}
	}

	// Just make the compiler happy
	return TRUE;
}

//--------------------------------------------------------------------------//
//	CRUPreRuntimeCheck::CheckConsistencyIfSingleMVUsed()
//--------------------------------------------------------------------------//

BOOL CRUPreRuntimeCheck::CheckConsistencyIfSingleMVUsed()
{
	// If the MV is single-delta (no base tables, or all of 
	// them are IGNORE CHANGES), no additional checks are required.
	if (TRUE == mv_.IsDeclarativeSingleDeltaMV()) 
	{
		return TRUE;
	}

	// If the MV is recomputed (for whatever reason),
	// there is no emptiness check - no way to check consistency
	if (TRUE == mv_.WillBeRecomputed())
	{
		errDesc_.SetError(IDS_RU_INCONSISTENT_JOIN);
		errDesc_.AddArgument(mvName_);
		return FALSE;
	}

	// The decision is delayed to runtime,
	// The Refresh task executor must verify 
	// that the regular tables' deltas are empty for this MV.
	CRUTblList &tblList = mv_.GetFullySyncTablesUsedByMe();
	DSListPosition pos = tblList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList.GetNext(pos);
		pTbl->SetEmptyDeltaNeeded();
	}

	return TRUE;
}

//--------------------------------------------------------------------------//
//	CRUPreRuntimeCheck::CheckUsedTablesAuditing()
//
//	An incremental MV cannot be refreshed as long as at least 
//	one REGULAR table used by it is non-audited.
//
//--------------------------------------------------------------------------//

BOOL CRUPreRuntimeCheck::CheckUsedTablesAuditing()
{
	if (CDDObject::eRECOMPUTE == mv_.GetRefreshType())
	{
		return TRUE;
	}

	CRUTblList &tblList = mv_.GetTablesUsedByMe();
	DSListPosition pos = tblList.GetHeadPosition();

	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList.GetNext(pos);

		if (TRUE == pTbl->IsRegularTable()
			&&
			FALSE == pTbl->IsAudited())
		{
			errDesc_.SetError(IDS_RU_TABLE_UNAUDITED);
			errDesc_.AddArgument(mv_.GetFullName());
			errDesc_.AddArgument(pTbl->GetFullName());
			return FALSE;
		}
	}

	return TRUE;
}

//--------------------------------------------------------------------------//
//	CRUPreRuntimeCheck::CheckNonAvailableMVRecompute()
//
//	If a non-available non-audited MV is recomputed, 
//	the user must specify the RECOMPUTE option explicitly.
//
//--------------------------------------------------------------------------//

BOOL CRUPreRuntimeCheck::CheckNonAvailableMVRecompute()
{
	if (CDDMV::eUNAVAILABLE == mv_.GetMVStatus())
	{
		RUASSERT(TRUE == mv_.WillBeRecomputed());

		if (FALSE == 
			CRUGlobals::GetInstance()->GetOptions().IsRecompute())
		{
			errDesc_.SetError(IDS_RU_UNAVAILABLE_RECOMPUTE);
			errDesc_.AddArgument(mvName_);	
			return FALSE;
		}
	}

	return TRUE;
}
