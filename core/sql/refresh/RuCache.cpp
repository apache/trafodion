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
* File:         RuCache.cpp
* Description:  Implementation of class CRUCache.
*				
*
* Created:      02/27/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "dsutilitywa.h"

#include "ddschema.h"
#include "ddtable.h"
#include "ddMVGroup.h"
#include "dddefaults.h"
#include "ddattribute.h"

#include "uofsSystem.h"
#include "uofsProcessPool.h"

#include "RuCache.h"
#include "RuException.h"
#include "RuGlobals.h"
#include "RuForceOptionsParser.h"

//--------------------------------------------------------------------------//
//							PUBLIC METHODS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUCache constructor and destructor
//--------------------------------------------------------------------------//

CRUCache::CRUCache() :
	sqlNode_(FALSE), // Do not let DDOL control the txns
	mvList_(),		// Items are owned
	tableList_(),	// Items are owned
	ddlLockHandler_(),
	isCancelOnly_(FALSE),
	isTotalRecompute_(FALSE),
	lcType_(CRUOptions::DONTCARE_LC),
	currentUser_(""),
	maxParallelism_(1),
	maxPipelining_(1)
{}
						
CRUCache::~CRUCache() 
{
	// Normally, all of the DDOL MV/table objects are in
	// the eClosed state at this point. However,
	// if the destructor is called from inside an exception 
	// handler when some DDOL object is left in the eModified 
	// state, the utility should not attempt to save the object
	// to the catalog.
	DSListPosition pos = mvList_.GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = mvList_.GetNext(pos);
		pMV->CancelChanges();
	}	

	pos = tableList_.GetHeadPosition();
	while (NULL != pos)
	{
		CRUTbl *pTbl = tableList_.GetNext(pos);
		pTbl->CancelChanges();
	}	

	// Since sqlNode_ is an object (not a pointer),
	// its destructor is called automatically,
	// therefore disposing the cloud of DD objects.

	// Since mvList_ and tableList_ *own* the referenced objects,
	// there is no need to apply the RemoveAll() method to them.
}

//--------------------------------------------------------------------------//
//	Lookup methods
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUCache::GetMV()
//--------------------------------------------------------------------------//

CRUMV *CRUCache::GetMV(TInt64 objUid) const
{
	DSListPosition pos = mvList_.GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = mvList_.GetNext(pos);
		
		if (pMV->GetUID() == objUid)
		{
			return pMV;
		}
	}
	
	return NULL;
}

//--------------------------------------------------------------------------//
//	CRUCache::GetTable()
//--------------------------------------------------------------------------//

CRUTbl *CRUCache::GetTable(TInt64 objUid) const
{
	DSListPosition pos = tableList_.GetHeadPosition();
	while (NULL != pos)
	{
		CRUTbl *pTbl = tableList_.GetNext(pos);
		
		if (pTbl->GetUID() == objUid)
		{
			return pTbl;
		}
	}

	return NULL;
}

//--------------------------------------------------------------------------//
// CRUCache::Build()
//
//  Populate the MV list and the table (used object) list.
//	The objects are classified as follows:
//
//	(1) Involved MVs - the MVs that should be refreshed by the current 
//	    invocation of the utility.
//	(2) Non-involved MVs - the ON REQUEST MVs that use the involved 
//	    tables, but are not being refreshed themselves (used by the 
//	    log cleanup logic).
//
//	(3) Involved used objects - objects that are directly used 
//	    by the involved MVs.
//	(4) Non-involved used objects - a CRUTbl interface for every 
//	    involved ON REQUEST MV that is not used by the other involved
//		MVs. 
//
//--------------------------------------------------------------------------//

void CRUCache::Build()
{
	InitBuild();

	// Fill the cache by involved MVs
	FetchInvolvedMVsMetadata();

	if (FALSE == isCancelOnly_)
	{
		// Check that there is at least one log to clean,
		// if the user requested log cleanup. 
		CheckLogCleanupApplicability();
	}

	// Fill the cache by involved tables, setup the usage lists
	FetchInvolvedUsedObjectsMetadata();

	// For every involved MV, decide whether it will be recomputed now.
	// Consider to perform popindex/purgedata
	if (FALSE == isCancelOnly_)
	{
		SetupRecomputeProperty();
	}

	// Cancel/Aquire/Replace the DDL locks ...
	ddlLockHandler_.HandleDDLLocks(isCancelOnly_);

	if (TRUE == isCancelOnly_)
	{
		// Short path - the utility was applied with the CANCEL option.
		// The DDL locks were cancelled after fetching the involved objects
		return;
	}

	// Read the MV_REFRESH_MAX_PIPELINING 
	// and MV_REFRESH_MAX_PARALLELISM defaults
	FetchDefaults();

	// Add the non-involved MVs to the cache
	FetchNonInvolvedMVsMetadata();

	if (CRUOptions::DO_ONLY_LC == lcType_)
	{
		// Nothing more is required for DO ONLY LOG CLEANUP
		return;
	}

	// Complete the job for the "real" run!
	FetchNonInvolvedUsedObjectsMetadata();

	// For each involved MV, setup the cross-pointers 
	// between the CRUMV and CRUTbl objects corresponding to it.
	FixupMVTblInterfaces();

	if (FALSE == CRUGlobals::GetInstance()->GetOptions().GetForceFilename().IsEmpty())
	{
		FetchForceData();
	}
}

#ifdef _DEBUG
//--------------------------------------------------------------------------//
//	CRUCache::Dump()
//--------------------------------------------------------------------------//

void CRUCache::Dump(CDSString &to, BOOL isExtended) const
{
	to += "\n\t\tCACHE DUMP\n";

	DSListPosition pos = mvList_.GetHeadPosition();
	while (NULL != pos) 
	{
		CRUMV *pMV = mvList_.GetNext(pos);
		pMV->Dump(to, isExtended);
	}

	pos = tableList_.GetHeadPosition();
	while (NULL != pos) 
	{
		CRUTbl *pTbl = tableList_.GetNext(pos);
		pTbl->Dump(to, isExtended);
	}
}
#endif

//--------------------------------------------------------------------------//
//						PRIVATE METHODS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUCache::InitBuild()
//--------------------------------------------------------------------------//

void CRUCache::InitBuild()
{
	CRUGlobals *pGlobals = CRUGlobals::GetInstance(); 
	CRUOptions &options = pGlobals->GetOptions();

	// Should this be the machine name when the code is ported?
	sqlNode_.Open("NSK");

	lcType_ = options.GetLogCleanupType();
	isCancelOnly_ = options.IsCancel();
	isTotalRecompute_ = options.IsRecompute();

	if (FALSE == isCancelOnly_)
	{
          currentUser_ = CRUGlobals::GetCurrentUser();
	}
}

//--------------------------------------------------------------------------//
//	CRUCache::FetchInvolvedMVsMetadata()
//
//	Retrieve the metadata about all the involved MVs. Construct the wrapper 
//	CRUMV objects (one for each MV) and place them to the list of MVs.
//
//	Split by cases (single-MV refresh, cascaded refresh, MV group refresh).
//--------------------------------------------------------------------------//

void CRUCache::FetchInvolvedMVsMetadata()
{
	CRUOptions &options = CRUGlobals::GetInstance()->GetOptions();

	// Retrieve the schema that hosts the MV/MV group
	CDDSchema *pddSch = 
		GetDDSchemaByName(options.GetCatalogName(), 
		                  options.GetSchemaName());

	CDSString objName = options.GetObjectName();
	CRUOptions::InvocType invocType = options.GetInvocType();

	switch (invocType) {

	case CRUOptions::SINGLE_MV:
		{
			CDDMV *pddMV = GetDDMVByName(pddSch, objName);
			FetchSingleInvolvedMV(pddMV);
			break;
		}

	case CRUOptions::CASCADE:
		{
			FetchCascadedMVs(pddSch, objName);
			break;
		}

		
	case CRUOptions::MV_GROUP:
		{
			FetchMVsInGroup(pddSch, objName);
			break;
		}
		
	default: RUASSERT(FALSE);
	}
}

//--------------------------------------------------------------------------//
//	CRUCache::FetchCascadedMVs()
//
//	 Fetch the metadata for all the involved MVs if REFRESH is run 
//	 with CASCADE option. The involved MVs are all the ON REQUEST 
//	 or RECOMPUTE MVs used by the "root" MV directly or indirectly.
//--------------------------------------------------------------------------//

void CRUCache::FetchCascadedMVs(CDDSchema *pddSch, const CDSString& mvName)
{
	CDDMV *pddRootMV = GetDDMVByName(pddSch, mvName);

	if (CDDMV::eON_STATEMENT == pddRootMV->GetRefreshType())
	{
		// The CASCADE option is not allowed for ON STATEMENT MVs
		CRUException e;
		e.SetError(IDS_RU_ILLEGAL_CASCADE);
		e.AddArgument(pddRootMV->GetFullName());
		throw e;
	}

	// Start the recursive seacrh
	RecursiveFetchCascadedMVs(pddRootMV);
}

//--------------------------------------------------------------------------//
//	CRUCache::RecursiveFetchCascadedMVs()
//
//	Fetch the expanded tree of MV objects starting from a "root" MV.
//	Algorithm: 
//		Fetch the ON REQUEST/RECOMPUTED MVs directly used by the root MV,
//		and continue drilling recursively.
//--------------------------------------------------------------------------//

void CRUCache::RecursiveFetchCascadedMVs(CDDMV *pddRootMV)
{
	// Setup the wrapper object for the MV
	FetchSingleInvolvedMV(pddRootMV);

	CDDUIDTripleList &uList = pddRootMV->GetUsedObjects();

	DSListPosition pos = uList.GetHeadPosition();
	while (NULL != pos) 
	{
		CDDUIDTriple& uidt = uList.GetNext(pos);

		// Skip the regular tables 
		if (FALSE == pddRootMV->IsUsedObjectAnMV(uidt.objUID))
		{
			continue;
		}

		CDDMV *pddMV = GetDDMVByUID(uidt);

		// An ON STATEMENT MV is a regular table 
		// for the purpose of cascade. Stop drilling inside.
		if (CDDObject::eON_STATEMENT == pddMV->GetRefreshType())
		{
			continue;
		}

		// Since the same object cannot be used twice by the same MV,
		// the used MV should not have been referenced before
		// (and placed in the MV list).
		RUASSERT (NULL != GetMV(pddRootMV->GetUID()));

		RecursiveFetchCascadedMVs(pddMV);		
	}
}

//--------------------------------------------------------------------------//
//	CRUCache::FetchMVsInGroup()
//
//	 Fetch the metadata for all the MVs in the MVGROUP
//--------------------------------------------------------------------------//

void CRUCache::FetchMVsInGroup(CDDSchema *pddSch, const CDSString& mvgName)
{
	CDDMVGroup *pddMVG = GetDDMVGroupByName(pddSch, mvgName);

	if (TRUE == pddMVG->IsEmpty()) 
	{
		// The source MVGROUP is empty
		CRUException e;
		e.SetError(IDS_RU_MVGROUP_EMPTY);
		e.AddArgument(pddMVG->GetFullName());
		throw e;
	}

	CDDUIDTripleList& uidList = pddMVG->GetAllMVs();
	DSListPosition pos = uidList.GetHeadPosition();

	while (NULL != pos) 
	{
		// Retrieve the DD object
		CDDUIDTriple uidt = uidList.GetNext(pos);

		CDDMV *pddMV = GetDDMVByUID(uidt);

		// Setup the CRUMV wrapper object
		FetchSingleInvolvedMV(pddMV);
	}
}

//--------------------------------------------------------------------------//
//	CRUCache::FetchSingleInvolvedMV()
//
//	Least common denominator for all the paths to MV metadata.
//
//	(1) Allocate a single CRUMV object and put it to the global list.
//	(2) Fetch the Refresh-related metadata.
//	(3) If we are in the CANCEL mode - done. Otherwise, check the user's
//		privileges for this MV.
//	
//--------------------------------------------------------------------------//

void CRUCache::FetchSingleInvolvedMV(CDDMV *pddMV)
{
	CRUMV *pMV = new CRUMV(pddMV);

	pMV->SetInvolved();	

	// Read the REFRESH-specific metadata 
	pMV->FetchMetadata();

	// Take care not to cancel the dangling DDL lock accidentally.
	// For example, if the MV is multi-transactional, and has a 
	// DDL lock following the previous Refresh failure - it is 
	// forbidden to remove the DDL lock as long as the MV is not
	// refreshed successfully.
	pMV->SetReleaseDDLLock(pddMV->CanCancelDDLLock());

	// Ignore User Maintainable MVs.
	if (CDDObject::eBY_USER == pMV->GetRefreshType())
	{
	  delete pMV;
	  return;
	}

	mvList_.AddTail(pMV);

	// Register the MV for the further DDL lock handling
	ddlLockHandler_.AddObject(pMV);

	if (TRUE == isCancelOnly_)
	{
		return;
	}

	// Retrieve whether the user has Insert/Select/Delete privilege for this MV
	pMV->FetchPrivileges();
}

//--------------------------------------------------------------------------//
//  CRUCache::FetchInvolvedUsedObjectsMetadata()
//
//	Fetch the metadata for all the objects used by the involved MVs.
//	For each used object, create a CRUTbl wrapper and place it to the 
//	list of used objects (aka tables).
//
//	Establish the cross-references of type "tables used by me" and "MVs 
//	using me" between the individual CRUMV and CRUTbl objects. 
//	The cross-references are implemented as lists of PHYSICAL pointers,
//	rather that LOGICAL ones (UID triples) at the DDOL level.
//
//--------------------------------------------------------------------------//

void CRUCache::FetchInvolvedUsedObjectsMetadata()
{
	DSListPosition pos = mvList_.GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = mvList_.GetNext(pos);
		FetchUsedObjectsMetadataForSingleMV(pMV);
	}
}

//--------------------------------------------------------------------------//
//	CRUCache::FetchUsedObjectsMetadataForSingleMV()
//
//	Fetch the metadata for all the objects used by a single MV.
//	
//	Use the "global" list of CRUTbl wrappers as a cache. Create a wrapper
//	once a used object is referenced first.
//
//	Establish the cross-references of type "tables used by me" and "MVs 
//	using me" between the using and used objects. 
//--------------------------------------------------------------------------//

void CRUCache::FetchUsedObjectsMetadataForSingleMV(CRUMV *pMV)
{
	CDDUIDTripleList &uList = pMV->GetAllUsedObjectsUIDs();

	DSListPosition pos = uList.GetHeadPosition();
	while (NULL != pos) 
	{
		CDDUIDTriple& uidt = uList.GetNext(pos);
		TInt64 objUid = uidt.objUID;

		// Tables that are IGNORE CHANGES and UDFs do not interest us
		// for the purpose of REFRESH
		if (TRUE == pMV->IsIgnoreChanges(objUid) ||
		    TRUE == pMV->IsUsedObjectUDF(objUid)) 
		{
			continue;
		}

		// Fetch the pointer to the used table's wrapper object
		// (create the wrapper if the table is referenced first)
		CRUTbl *pTbl = GetTable(objUid);
		if (NULL == pTbl)
		{
			pTbl = FetchSingleUsedObject(pMV, uidt);
		}
		
		// Add the pointer to the table to the MV's list of dependencies.
		pMV->AddRefToUsedObject(pTbl);
		
		// Add the pointer to the MV to the table's list of dependencies.
		pTbl->AddRefToUsingMV(pMV);

		// Register the used object for the DDL lock handling ...
		SetupUsedObject(pTbl);
	}
}

//--------------------------------------------------------------------------//
//	CRUCache::FetchSingleUsedObject()
//	
//	Create a wrapper for a single used object that is referenced first,
//	and add it to the list of used tables.
//
//	Split by cases: the object can be an MV or a regular table.
//--------------------------------------------------------------------------//

CRUTbl *CRUCache::FetchSingleUsedObject(CRUMV *pUsingMV, 
										const CDDUIDTriple &uidt)
{
	CRUTbl *pTbl;
	BOOL isMV = pUsingMV->IsUsedObjectAnMV(uidt.objUID);

	if (FALSE == isMV) 
	{
		CDDTable *pddTable = GetDDTableByUID(uidt);
		pTbl = new CRURegularTbl(pddTable);
	}
	else 
	{
		CDDMV *pddMV = GetDDMVByUID(uidt);
		pTbl = new CRUMVTbl(pddMV);

		// Read the REFRESH-specific metadata
		pddMV->FetchMVRefreshMetadata();

		// A table based on MV will always inherit the MV's timestamp
		pTbl->SetTimestamp(pddMV->GetRefreshedAtTimestamp());

		// Inherit the MV's feature.
		// If a table is NOT an involved MV, take care not to remove 
		// its dangling DDL lock (if any) accidentally.
		pTbl->SetReleaseDDLLock(pddMV->CanCancelDDLLock());
	}

	pTbl->SetInvolved();
	tableList_.AddTail(pTbl);

	return pTbl;
}

//--------------------------------------------------------------------------//
//	CRUCache::SetupUsedObject()
//
//	(1) Register the used object for DDL lock handling.
//
//	(2) For the first ON REQUEST MV using the table:
//	    - Fetch the epoch metadata from the MVS_TABLE_INFO_UMD table.
//	    - Fixup its pointer to the IUD log table's object in DDOL.
//
//--------------------------------------------------------------------------//

void CRUCache::SetupUsedObject(CRUTbl *pTbl)
{
	// If the table is also an involved MV, the DDL lock is already handled
	if (NULL == GetMV(pTbl->GetUID()))
	{
		// Register the table for the further DDL lock handling
		ddlLockHandler_.AddObject(pTbl);
	}
	
	if (TRUE == isCancelOnly_)
	{
		return;	// No more processing is required
	}

	if (1 == pTbl->GetOnRequestMVsUsingMe().GetCount())
	{
		// Fetch the epochs' metadata
		pTbl->FetchMetadata();

		// Fixup the pointer to the IUD log table
		FixupLogTblReference(pTbl);
	}
}

//--------------------------------------------------------------------------//
//	CRUCache::FixupLogTblReference()
//
//	Fixup the pointer to the IUD log object from the table
//--------------------------------------------------------------------------//

void CRUCache::FixupLogTblReference(CRUTbl *pTbl)
{
	const CDSString &catName = pTbl->GetCatName();
	const CDSString &schName = pTbl->GetSchName();
	CDDSchema *pddSch = GetDDSchemaByName(catName, schName);

	// Compose the IUD log table's name
	CDSString logName(pTbl->GetLogShortName(CRUTbl::iudNmspPrefix));
		
	CDDTable *pddLogTable = GetDDIUDLogTableByName(pddSch, logName);

	// Dummy fetch, just force DDOL to fetch the data
	pddLogTable->GetColumnList();

	pTbl->FixupIUDLogTbl(pddLogTable);
}

//--------------------------------------------------------------------------//
//	CRUCache::SetupRecomputeProperty()
//
//	For each involved MV, determine whether the MV must be recomputed, 
//	and consider purgedata/popindex as part of recompute
//
//--------------------------------------------------------------------------//

void CRUCache::SetupRecomputeProperty()
{
	DSListPosition pos = mvList_.GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = mvList_.GetNext(pos);
		pMV->SetupRecomputeProperty(isTotalRecompute_);
	}
}

//--------------------------------------------------------------------------//
//	CRUCache::FetchNonInvolvedMVsMetadata()
//
//	Fetch the epoch metadata for ON REQUEST MVs that are using the involved 
//	tables, but are not refreshed in this invocation of the utility.
//
//  This metadata is required for Log Cleanup and Delta Pipelining 
//	decisions.
//
//--------------------------------------------------------------------------//

void CRUCache::FetchNonInvolvedMVsMetadata()
{
	DSListPosition tpos = tableList_.GetHeadPosition();
	while (NULL != tpos) 
	{
		CRUTbl *pTbl = tableList_.GetNext(tpos);
		
		// Retrieve the table's private dependency list
		CDDUIDTripleList &privTblList = pTbl->GetUIDsOfAllMVsUsingMe();
			
		DSListPosition mvpos = privTblList.GetHeadPosition();
		while (NULL != mvpos)
		{
			CDDUIDTriple &uidt = privTblList.GetNext(mvpos);

			CRUMV *pMV = GetMV(uidt.objUID); 
			if (NULL == pMV)
			{
				// The MV is non-involved, fetch the DD object
				CDDMV *pddMV = GetDDMVByUID(uidt);

				// If the MV is not ON REQUEST, skip it
				if (CDDObject::eON_REQUEST != pddMV->GetRefreshType())
				{
					continue;
				}

				pMV = FetchSingleNonInvolvedMV(pddMV);
			}

			if (FALSE == pMV->IsInvolved())
			{
				// If the MV is involved, the pointer exists already
				pTbl->AddRefToUsingMV(pMV);
			}
		}
	}	
}

//--------------------------------------------------------------------------//
//	CRUCache::FetchSingleNonInvolvedMV()
//
//	Fetch a newly-referenced non-involved ON REQUEST MV.
//--------------------------------------------------------------------------//

CRUMV *CRUCache::FetchSingleNonInvolvedMV(CDDMV *pddMV)
{
	// Create a new wrapper object for a non-involved MV,
	// leave its reference list empty
	CRUMV *pMV = new CRUMV(pddMV);
			
	mvList_.AddTail(pMV);			
			
	// Fetch the epoch data
	pMV->FetchMetadata();

	return pMV;
}

//--------------------------------------------------------------------------//
//	CRUCache::FetchNonInvolvedUsedObjectsMetadata()
//
//	Create a CRUTbl object for each involved ON REQUEST MV that 
//	does not have a corresponding CRUTbl object in the table list 
//	(i.e., is not used by the other involved MVs). 
//
//-------------------------------------------------------------------------//

void CRUCache::FetchNonInvolvedUsedObjectsMetadata()
{
	DSListPosition pos = mvList_.GetHeadPosition();

	while (NULL != pos)
	{
		CRUMV *pMV = mvList_.GetNext(pos);
		if (FALSE == pMV->IsInvolved()
			||
			CDDObject::eON_REQUEST != pMV->GetRefreshType())
		{
			continue;
		}

		if (NULL == GetTable(pMV->GetUID())) 
		{
			CDDSchema *pddSch = 
				GetDDSchemaByName(pMV->GetCatName(), pMV->GetSchName());

			CDDMV *pddMV = GetDDMVByName(pddSch, pMV->GetName());
			CRUTbl *pTbl = new CRUMVTbl(pddMV);
			
			pTbl->FetchMetadata();

			tableList_.AddTail(pTbl);
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUCache::CheckLogCleanupApplicability()
//
//	Check the applicability of the {WITH | DO ONLY} LOG CLEANUP option.
//
//	If there is no involved ON REQUEST MV, then the involved tables 
//	mignt not have logs, and the option is not legal.
//
//  The method also resolves the LOG CLEANUP option, if it does not appear
//	in the command: if there is no involved ON REQUEST MV, it is resolved
//	to WITHOUT LOG CLEANUP, otherwise to WITH LOG CLEANUP.
//
//--------------------------------------------------------------------------//

void CRUCache::CheckLogCleanupApplicability()
{
	switch (lcType_)
	{
	case CRUOptions::WITHOUT_LC:
		break;	// Nothing to check

	case CRUOptions::DONTCARE_LC:
		{
			lcType_ = (TRUE == IsOnRequestMVInvolved()) ?
				CRUOptions::WITH_LC :
				CRUOptions::WITHOUT_LC;

			CRUOptions &options = CRUGlobals::GetInstance()->GetOptions();
			options.SetLogCleanupType(lcType_);

			break;
		}

	case CRUOptions::WITH_LC:
	case CRUOptions::DO_ONLY_LC:
		{
			if (FALSE == IsOnRequestMVInvolved())
			{
				// No candidate for log cleanup
				CRUException ex;

				ex.SetError(IDS_RU_EMPTY_CLEANUP_LIST);
				throw ex;
			}
			break;
		}

	default: RUASSERT(FALSE);
	}
}

//--------------------------------------------------------------------------//
//	CRUCache::IsOnRequestMVInvolved()
//--------------------------------------------------------------------------//

BOOL CRUCache::IsOnRequestMVInvolved()
{
	DSListPosition pos = mvList_.GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = mvList_.GetNext(pos);

		if (TRUE == pMV->IsInvolved()
			&&
			CDDMV::eON_REQUEST == pMV->GetRefreshType()
			)
		{
			return TRUE;
		}
	}
	
	return FALSE;
}

//--------------------------------------------------------------------------//
//	CRUCache::FetchDefaults()
//
//  Read the MV_REFRESH_MAX_PIPELINING and MV_REFRESH_MAX_PARALLELISM 
//	defaults from the SYSTEM_DEFAULTS table.
//
//--------------------------------------------------------------------------//

void CRUCache::FetchDefaults()
{
	CDSString maxPiplining("MV_REFRESH_MAX_PIPELINING");
	FetchSingleDefault(
		maxPiplining,
		maxPipelining_);

	CDSString maxParallelism("MV_REFRESH_MAX_PARALLELISM");
	FetchSingleDefault(
		maxParallelism,
		maxParallelism_);

	if (0 == maxParallelism_
		||
		maxParallelism_ > CUOFsTaskProcessPool::MaxProcesses)
	{
		// We have a limited capacity for parallelism
		maxParallelism_ = CUOFsTaskProcessPool::MaxProcesses;
	}
#ifdef NA_LINUX
	// temporary workaround until we get tdm_arkutp to work
	// maxPipelining_  = 1;
	maxParallelism_ = 1;
#endif
}

//--------------------------------------------------------------------------//
//	CRUCache::FetchSingleDefault()
//
//	The default value of each default is 1.
//--------------------------------------------------------------------------//

void CRUCache::FetchSingleDefault( CDSString &name,
				   TInt32 &to)
{
	to = 1;

        CDDControlQueryDefault cqd(name);

        CDSString stringVal = cqd.RetrieveCQDValue();

        // this should not happen since the cqd is defined in the code but just in case
        if (stringVal.IsEmpty())
        {
             return;
        }

	const char *attrVal = stringVal.c_string();
        
	TInt32 numericVal;

	if (0 == sscanf(attrVal, "%d", &numericVal)	// Invalid value
		||
		numericVal < 0	// negative value
		)
	{
		return;
	}

	to = numericVal;
}

//--------------------------------------------------------------------------//
//	CRUCache::FixupMVTblInterfaces()
//	
//	For each involved MV in the cache such that there is also 
//	a table object in the cache for the same UID (for example,
//	if the MV is used by another involved MV): set the pointer
//	from the MV object to the table object, and back.
//
//--------------------------------------------------------------------------//

void CRUCache::FixupMVTblInterfaces()
{
	DSListPosition pos = mvList_.GetHeadPosition();
	
	while (NULL != pos)
	{
		CRUMV *pMV = mvList_.GetNext(pos);

		if (FALSE == pMV->IsInvolved())
		{
			continue;	// Skip the non-involved MVs
		}

		CRUTbl *pTbl = GetTable(pMV->GetUID());
		if (NULL == pTbl)
		{
			continue; // No table object in the cache, skip
		}

		// Setup pointers MV <---> table
		pMV->SetTblInterface(pTbl);

		pTbl->SetMVInterface(pMV);
	}
}

//--------------------------------------------------------------------------//
//	CRUCache::FetchForceData()
//	
//--------------------------------------------------------------------------//
void CRUCache::FetchForceData()
{
	CRUForceOptionsParser forceParser;
	
	forceParser.SetFile(
		CRUGlobals::GetInstance()->GetOptions().GetForceFilename());

	forceParser.Parse();

	CRUForceOptions& forceOption = forceParser.GetForceOptions();

	// Place the force data on the involved mv's.
	CRUMVList &mvList = GetMVList();
	DSListPosition pos = mvList.GetHeadPosition();
	
	while (NULL != pos)
	{
		CRUMV *pMV = mvList.GetNext(pos);
		if (FALSE == pMV->IsInvolved())
		{
			continue;
		}

		UpdateForceOptionForMV(forceOption, pMV);
	}
}

//--------------------------------------------------------------------------//
//	CRUCache::UpdateForceOptionForMV()
//--------------------------------------------------------------------------//
void CRUCache::UpdateForceOptionForMV(CRUForceOptions& forceOptions,
									  CRUMV *pMV)
{
	CRUMVForceOptionsList& mvForceOptionsList =
		forceOptions.GetMVForceOptionsList();

	DSListPosition prevpos = NULL;
	DSListPosition pos = mvForceOptionsList.GetHeadPosition();
	while (NULL != pos)
	{
		prevpos = pos;
		CRUMVForceOptions *pMVForceOptions = mvForceOptionsList.GetNext(pos);
		if (pMV->GetFullName() == pMVForceOptions->GetMVName())
		{
			pMV->SetMVForceOption(pMVForceOptions);
			if (NULL == prevpos)
			{
				mvForceOptionsList.RemoveHead();
			}
			else
			{
				mvForceOptionsList.RemoveAt(prevpos);
			}
			return;
		}
	}
}

//--------------------------------------------------------------------------//
//	Various DD objects retrieval
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUCache::GetDDSchemaByName()
//  Retrieving the schema by catalog + schema name
//--------------------------------------------------------------------------//

CDDSchema *CRUCache::GetDDSchemaByName(const CDSString& catName,
									   const CDSString& schName)
{
  // check for DEFAULT_SCHEMA_ACCESS_ONLY
  // get default catalog & schema
  CRUOptions &options = CRUGlobals::GetInstance()->GetOptions();  	  
  CDSString curCatSch = catName + "." + schName;
  if (CDDControlQueryDefault::ViolateDefaultSchemaAccessOnly(
        options.GetDefaultSchema(), curCatSch))
  {
    CRUException e;
    e.SetError(IDS_RU_SCHEMA_ACCESS_VIOLATION);
    throw e;
  }

  CDDCatalog *pddCat = sqlNode_.GetCatalog(catName);
	if (NULL == pddCat) 
	{
		// Source catalog does not exist
		CRUException e;
						
		e.SetError(IDS_RU_INVALID_CATALOG_NAME);
		e.AddArgument(catName.c_string());
		throw e;
	}
	
	CDDSchema *pddSch = pddCat->GetSchema(schName, TRUE);

	if (NULL == pddSch) 
	{
		// Source schema does not exist
		CRUException e;
						
		e.SetError(IDS_RU_INVALID_SCHEMA_NAME);
		CDSString fname = catName + "." + schName;
		e.AddArgument(fname.c_string());
		throw e;	
	}

	return pddSch;
}

//--------------------------------------------------------------------------//
//	CRUCache::GetDDSchemaByUID()
//  Retrieving the schema by catalog + schema UID
//--------------------------------------------------------------------------//

CDDSchema *CRUCache::GetDDSchemaByUID(TInt64 catUid, TInt64 schUid)
{
	CDDCatalog *pddCat = sqlNode_.GetCatalog(catUid);
	if (NULL == pddCat) 
	{
		// Catalog does not exist, metadata inconsistency
		CRUException e;	
		e.SetError(IDS_RU_INCONSISTENT_CAT);
		throw e;
	}
	
	CDDSchema *pddSch = pddCat->GetSchema(schUid);
	if (NULL == pddSch) 
	{
		// Schema does not exist, metadata inconsistency
		CRUException e;
		e.SetError(IDS_RU_INCONSISTENT_CAT);
		throw e;	
	}

	return pddSch;
}

//--------------------------------------------------------------------------//
//	CRUCache::GetDDMVByName()
//--------------------------------------------------------------------------//

CDDMV *CRUCache::GetDDMVByName(CDDSchema *pddSch, const CDSString& mvName)
{
	CDDMV *pddMV = pddSch->GetMV(mvName);

	if (NULL == pddMV) // try public schema if necessary
  {
    CDDSchema *pddSchP = GetPublicSchema();
    if (pddSchP)
    {
      pddMV = pddSchP->GetMV(mvName);
      if (pddMV)
      { // if can be found in the public schema
        // update the catalog and schema
        CRUOptions &options = CRUGlobals::GetInstance()->GetOptions();  	  
        options.SetCatalogName(pddSchP->GetCatName());
        options.SetSchemaName(pddSchP->GetName());
      }
    }
  }

	if (NULL == pddMV)
	{
		// Source MV does not exist
		CRUException e;

		e.SetError(IDS_RU_INVALID_MV_NAME);
		CDSString fname = pddSch->GetFullName() + "." + mvName;
		e.AddArgument(fname.c_string());
		throw e;	
	}

	return pddMV;
}

//--------------------------------------------------------------------------//
//	CRUCache::GetDDIUDLogTableByName()
//--------------------------------------------------------------------------//

CDDTable *CRUCache::GetDDIUDLogTableByName(CDDSchema *pddSch, 
									const CDSString& logName)
{
	CDDTable *pddTable = pddSch->GetIUDLogTable(logName);

	if (NULL == pddTable) // try public schema if necessary
  {
    CDDSchema *pddSchP = GetPublicSchema();
    if (pddSchP)
    {
      pddTable = pddSch->GetIUDLogTable(logName);
      if (pddTable)
      { // if can be found in the public schema
        // update the catalog and schema
        CRUOptions &options = CRUGlobals::GetInstance()->GetOptions();  	  
        options.SetCatalogName(pddSchP->GetCatName());
        options.SetSchemaName(pddSchP->GetName());
      }
    }
  }

	if (NULL == pddTable)
	{
		// Table does not exist, metadata inconsistency
		CRUException e;
		e.SetError(IDS_RU_INCONSISTENT_CAT);
		throw e;
	}

	return pddTable;
}

//--------------------------------------------------------------------------//
//	CRUCache::GetDDMVGroupByName()
//--------------------------------------------------------------------------//

CDDMVGroup *CRUCache::GetDDMVGroupByName(CDDSchema *pddSch, const CDSString& mvgName)
{
	CDDMVGroup *pddMVG = pddSch->GetMVGroup(mvgName);

	if (NULL == pddMVG) // try public schema if necessary
  {
    CDDSchema *pddSchP = GetPublicSchema();
    if (pddSchP)
    {
      pddMVG = pddSchP->GetMVGroup(mvgName);
      if (pddMVG)
      { // if can be found in the public schema
        // update the catalog and schema
        CRUOptions &options = CRUGlobals::GetInstance()->GetOptions();  	  
        options.SetCatalogName(pddSchP->GetCatName());
        options.SetSchemaName(pddSchP->GetName());
      }
    }
  }

	if (NULL == pddMVG)
	{
		// Source MV group does not exist
		CRUException e;
		
		e.SetError(IDS_RU_INVALID_MVGROUP_NAME);
		CDSString fname = pddSch->GetFullName() + "." + mvgName;
		e.AddArgument(fname.c_string());
		throw e;	
	}

	return pddMVG;
}

//--------------------------------------------------------------------------//
//	CRUCache::GetDDMVByUID()
//--------------------------------------------------------------------------//

CDDMV *CRUCache::GetDDMVByUID(const CDDUIDTriple &uidt)
{
	CDDSchema *pddSch = GetDDSchemaByUID(uidt.catUID, uidt.schUID);
	CDDMV *pddMV = pddSch->GetMV(uidt.objUID);

	if (NULL == pddMV)
	{
		// MV does not exist, metadata inconsistency
		CRUException e;
		e.SetError(IDS_RU_INCONSISTENT_CAT);
		throw e;
	}

	return pddMV;
}

//--------------------------------------------------------------------------//
//	CRUCache::GetDDTableByUID()
//--------------------------------------------------------------------------//

CDDTable *CRUCache::GetDDTableByUID(const CDDUIDTriple &uidt)
{
	CDDSchema *pddSch = GetDDSchemaByUID(uidt.catUID, uidt.schUID);
	CDDTable *pddTbl = pddSch->GetTable(uidt.objUID);

	if (NULL == pddTbl)
	{
		// Table does not exist, metadata inconsistency
		CRUException e;
		e.SetError(IDS_RU_INCONSISTENT_CAT);
		throw e;
	}

	return pddTbl;
}

// for public schema
// return a CDDSchema pointer if need to check public schema
CDDSchema *CRUCache::GetPublicSchema()
{
  CRUOptions &options = CRUGlobals::GetInstance()->GetOptions();  	  
  if (FALSE == options.IsQualified()) // only when the object is not qualified 
    return NULL;

  CDSString pubCat;
  CDSString pubSch;
  Int32 retPubSch = CDDControlQueryDefault::RetrievePublicSchema( 
    options.GetDefaultSchema(), pubCat, pubSch); 
  
  if (retPubSch == 0)
    return NULL;

  CDDCatalog *pddCat = sqlNode_.GetCatalog(pubCat);

  if (NULL == pddCat) 
    return NULL;

  // retPubSch must be 1 or 2 here
  CDDSchema *pddSchp = pddCat->GetSchema(pubSch, TRUE);
  return pddSchp;
}
