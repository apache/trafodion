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
#ifndef _RU_MV_H_
#define _RU_MV_H_

/* -*-C++-*-
******************************************************************************
*
* File:         CRUMV.h
* Description:  Definition of class CRUMV
*				The wrapper for CDDMV at the REFRESH utility level
*
* Created:      02/27/2000
* Language:     C++
*
* 
******************************************************************************
*/

#include "refresh.h"
#include "dsplatform.h"
#include "ddMV.h"

#include "RuObject.h"
#include "RuForceOptions.h"

class CDDIndex;
class CDDIndexList;

class CRUTbl;
class CRUTblList;

class CRUIndex;
class CRUIndexList;

struct CRUDeltaDef;
class CRUDeltaDefList;

//--------------------------------------------------------------------------//
//	CRUMV (inherits from CRUObject)
//
//	  CRUMV is a wrapper class for CDDMV (MV class in DDOL)
//	  that reflects the properties of an MV in a particular 
//	  invocation of REFRESH (for example, will the MV be recomputed
//	  in the current invocation of REFRESH?).
// 
//	  The class provides the functionality of updating the metadata 
//	  and saving it in the SMD and UMD tables, acquiring and releasing 
//	  resources etc.
//
//	  The class maintains the following pointers:
//	  (1) The pointer to the DDOL "core" object, pddMV_. 
//	      The destructor should *not* try to deallocate memory 
//		  for the core, because the latter is owned by the 
//		  CDDSchema object and will be freed by it.
//	  (2) A list of pointers to the CRUTbl objects *directly* 
//		  used by the MV, pTablesUsedByMe_ (that are not specified 
//		  as IGNORE CHANGES). This list does not own the referenced 
//		  objects, and hence does not deallocate them in its destructor.
//	  (3) A pointer to the CRUTbl object with the  same UID as the 
//		  MV's one (i.e., the MV's representation as a table), if 
//		  such an object exists in the cache - pTblInterface_.
//	  (4) A list of pointers to the CDDIndex objects for indexes defined 
//		  on the MV. When the REFRESH utility performs purgedata/popindex
//		  operations on indexes, index handling is not transparent for it.  
//		  In this case, the index list is populated.
//
//	 An auxiliary table list, pFullySyncTablesUsedByMe_,
//	 is a subset of pTablesUsedByMe_. It holds the pointers to the
//	 regular tables and ON STATEMENT MVs used by this MV.
//
//	 The MV also maintains a *delta-def* list, where each delta-def object 
//	 corresponds to a single non-empty delta of the MV. This list will be 
//	 exploited in building the syntax of INTERNAL  REFRESH, and for scheduling 
//	 decisions.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUMV : public CRUObject {

private:
	typedef CRUObject inherited;

public:
	CRUMV(CDDMV *pddMV);
	virtual ~CRUMV();

	//--------------------------------//
	//  Accessors
	//--------------------------------//
public:
	CRUTblList &GetTablesUsedByMe() const 
	{ 
		RUASSERT(NULL != pTablesUsedByMe_);
		return *pTablesUsedByMe_; 
	} 

	CRUTblList &GetFullySyncTablesUsedByMe() const
	{
		RUASSERT(NULL != pFullySyncTablesUsedByMe_);
		return *pFullySyncTablesUsedByMe_; 
	}

	// The CRUTbl object that corresponds to this MV
	CRUTbl *GetTblInterface() const
	{
		return pTblInterface_;
	}

	CRUDeltaDefList &GetDeltaDefList() const
	{
		RUASSERT(NULL != pDeltaDefList_);
		return *pDeltaDefList_;
	}

	CRUDeltaDef* GetDeltaDefByUid(TInt64 uid) const;

	CRUTbl* GetUsedTableByName(const CDSString& tblName) const;

	// Kind of refresh and optimizations during it
	enum RefreshPatternMask {

		RECOMPUTE = 0x1,	// Recompute or incremental?
		NODELETE  = 0x2,	// (If recompute) Use NODELETE option?
		PURGEDATA = 0x4,	// (If recompute) Use purgedata?
		POPINDEX  = 0x8		// (If recompute) Populate indexes?
	};

	Lng32 GetRefreshPatternMap() const
	{
		return refreshPatternMap_;
	}

	// Should the MV be recomputed
	BOOL WillBeRecomputed() const
	{
		return (0 != (refreshPatternMap_ & RECOMPUTE));
	}

	//-- Various queries
public:
	// Implementation of pure virtual
	virtual BOOL HoldsResources() const
	{
		return IsDDLLockPending();
	}

	// Is the CRUTbl object that corresponds to this MV an *involved* table?
	BOOL IsInvolvedTbl() const;

	// Can the MV be maintained based only on the log?
	BOOL IsSelfMaintainable();

	// Is a MIN/MAX, and some used table is not insert-only?
	BOOL IsComplexMinMax();

	// Does the MV observe a non-empty delta of this table?
	BOOL IsDeltaNonEmpty(CRUTbl &tbl);

	BOOL IsSingleDeltaRefresh() const;

	// Pipelining criteria 
	BOOL CanPipelineFromMe(BOOL isRoot, TInt64 &nextCandidateUid);
	BOOL CanPipelineToMe(CRUMV &bottomMV);

	// The current user's privileges 
	enum PrivMask {

		SELECT_PRIV = 0x1,
		INSERT_PRIV = 0x2,
		DELETE_PRIV = 0x4
	};
	
	Lng32 GetPrivMap() const
	{
		return privMap_;
	}

	//-- Accessor wrappers
public:
	virtual TInt64 GetUID() const 
	{ 
		return pddMV_->GetUID(); 
	}
	virtual const CDSString &GetFullName() 
	{ 
		return pddMV_->GetFullName(); 
	}
	virtual const CDSString &GetCatName()
	{
		return pddMV_->GetCatName();
	}
	virtual const CDSString &GetSchName()
	{
		return pddMV_->GetSchName();
	}
	virtual const CDSString &GetName()
	{
		return pddMV_->GetName();
	}

	CDDUIDTripleList &GetAllUsedObjectsUIDs() 
	{
		return pddMV_->GetUsedObjects();
	} 

	CDDObject::EMVRefreshType GetRefreshType() 
	{ 
		return pddMV_->GetRefreshType(); 
	}

	BOOL IsIgnoreChanges(TInt64 usedObjUid) 
	{ 
		return pddMV_->IsIgnoreChanges(usedObjUid);
	}

	BOOL IsUsedObjectAnMV(TInt64 usedObjUid) 
	{ 
		return pddMV_->IsUsedObjectAnMV(usedObjUid);
	}

	BOOL IsUsedObjectUDF(TInt64 usedObjUid) 
	{ 
		return pddMV_->IsUsedObjectUDF(usedObjUid);
	}

	BOOL IsDeclarativeSingleDeltaMV()
	{
		return pddMV_->IsSingleDeltaMV();
	}

	// How many of the directly used objects are MVs?
	short GetNumDirectlyUsedMVs()
	{
		return pddMV_->GetNumDirectlyUsedMVs();
	}

	CDDObject::EMVQueryType GetQueryType() 
	{
		return pddMV_->GetQueryType(); 
	}
	CDDObject::EMVAuditType GetAuditType() 
	{ 
		return pddMV_->GetAuditType(); 
	}

	CDDObject::EMVStatus GetMVStatus() 
	{ 
		return pddMV_->GetMVStatus(); 
	}

	TInt32 GetCommitNRows() 
	{ 
		return pddMV_->GetCommitNRows();
	}

	BOOL HasMinMaxAggregate() 
	{ 
		return pddMV_->HasMinMaxAggregate(); 
	}

	virtual TInt64 GetTimestamp() const 
	{
		return pddMV_->GetRefreshedAtTimestamp(); 
	}

	TInt32 GetEpoch(TInt64 usedObjUid) 
	{ 
		return pddMV_->GetEpoch(usedObjUid); 
	}

	TInt32 GetRecomputeDurationEstimate() 
	{
		return pddMV_->GetRecomputeDurationEstimate();
	}
	TInt32 GetRefreshDurationEstimateWith_2_Deltas()
	{
		return pddMV_->GetRefreshDurationEstimateWith_2_Deltas();
	}
	TInt32 GetRefreshDurationEstimateWith_3_Deltas()
	{
		return pddMV_->GetRefreshDurationEstimateWith_3_Deltas();
	}
	TInt32 GetRefreshDurationEstimateWith_N_Deltas()
	{
		return pddMV_->GetRefreshDurationEstimateWith_N_Deltas();
	}
	TInt32 GetMultiTxnTargetEpoch()
	{
		return pddMV_->GetMultiTxnTargetEpoch();
	}
	TInt32 GetRefreshDurationEstimate(TInt64 usedObjUid)
	{
		return pddMV_->GetRefreshDurationEstimate(usedObjUid);
	}
	TInt32 GetDeltaSizeEstimate(TInt64 usedObjUid)
	{
		return pddMV_->GetDeltaSizeEstimate(usedObjUid);
	}
	
	// Is the MV in the middle of multi-transactional Refresh?
	BOOL IsMultiTxnContext()
	{
		return pddMV_->IsMultiTxnContext();
	}

	const CDDIndexList& GetIndexList() const 
	{ 
		RUASSERT(NULL != pddIndexList_);
		return *pddIndexList_; 
	}

	CDDIndexList& GetIndexList() 
	{ 
		RUASSERT(NULL != pddIndexList_);
		return *pddIndexList_; 
	}

	// Direct invocation of POPINDEX CatApi request (not through DDOL)
	void GetPopIndexCatApiRequestText(CDSString &to, CDDIndex *pddIndex);

	void GetUpdateIndexStatusCatApiRequestText(CDSString &to, 
						BOOL isAvail,
						CDDIndex *pddIndex);


        void GetToggleAuditCatApiRequestText(CDSString &to, 
						BOOL auditFlag,
						CDDIndex *pddIndex);

	// Force options for the internal refresh statement
	const CRUMVForceOptions* GetMVForceOption() const
	{
		return pMvForceOptions_;
	}
	
#ifdef _DEBUG
public:
	virtual void Dump(CDSString &to, BOOL isExtended=FALSE);
#endif

	//--------------------------------//
	//  Mutators
	//--------------------------------//
public:
	// Update the usage lists
	void AddRefToUsedObject(CRUTbl *pTbl);
	
	// Implementation of pure virtual
	virtual void ReleaseResources();

	// Set the reference to the CRUTbl object
	// corresponding to this MV
	void SetTblInterface(CRUTbl *pTbl)
	{
		pTblInterface_ = pTbl;
	}

	// During the cache construction, decide whether the MV 
	// must be recomputed, and consider the purgedata/popindex optimizations
	void SetupRecomputeProperty(BOOL isTotalRecompute);

	void SetRefreshPatternMask(RefreshPatternMask mask)
	{
		refreshPatternMap_ |= mask;
	}

	// Check the Select/Insert/Delete privileges for the current user
	void FetchPrivileges();

	// During the DG construction, propagate the property between the MVs
	void PropagateRecomputeProperty();

	// Set MV.RECOMPUTE_EPOCH <-- MV.CURRENT_EPOCH
	void AdvanceRecomputeEpoch();

	//-- Index population
public:
	// Build the list of all the secondary indexes on me 
	// that are not captured by DDL locks
	void BuildIndexList();

	void PurgeDataWithIndexes();

	//-- Event handlers
public:
	// Update the delta-def list based on the EmpCheck results
	void PropagateEmpCheck(CRUTbl &tbl);
	// Update the delta-def list based on the DE statistics
	void PropagateDEStatistics(CRUTbl &tbl);

	//-- Mutator wrappers
public:
	// Fetch the metadata about epochs, statistics etc.
	// (not of general use in DDOL).
	virtual void FetchMetadata()
	{
		pddMV_->FetchMVRefreshMetadata();
	}

	// Flush the metadata changes to the SMD and UMD tables
	virtual void SaveMetadata()
	{
		pddMV_->Save();
	}

	// Cancel saving the metadata
	virtual void CancelChanges();

	void SetMVStatus(CDDObject::EMVStatus status) 
	{ 
		pddMV_->SetMVStatus(status); 
	}

        // publish a row to the Query Rewrite Publish table to
        // indicate that this MV has been refreshed. If the
        // refresh is with recompute the argument isRecompute
        // should be set
        void PublishMVRefresh ( BOOL isRecompute)
        {
                pddMV_->PublishMVRefresh (isRecompute);
        }

	void SetTimestamp(TInt64 ts)
	{ 
		pddMV_->SetRefreshedAtTimestamp(ts);
	}

	void SetEpoch(TInt64 usedObjUid, TInt32 ep)
	{
		pddMV_->SetEpoch(usedObjUid, ep);
	}

	void SetMVTableAudit(BOOL audit)
	{
		pddMV_->SetMVTableAudit(audit);
	}

	void SetRecomputeDurationEstimate(TInt32 time) 
	{ 
		pddMV_->SetRecomputeDurationEstimate(time);
	}
	void SetRefreshDurationEstimateWith_2_Deltas(TInt32 time)
	{
		pddMV_->SetRefreshDurationEstimateWith_2_Deltas(time);
	}
	void SetRefreshDurationEstimateWith_3_Deltas(TInt32 time)
	{
		pddMV_->SetRefreshDurationEstimateWith_3_Deltas(time);
	}
	void SetRefreshDurationEstimateWith_N_Deltas(TInt32 time)
	{
		pddMV_->SetRefreshDurationEstimateWith_N_Deltas(time);
	}

	void SetMultiTxnTargetEpoch(TInt32 epoch)
	{
		pddMV_->SetMultiTxnTargetEpoch(epoch);
	}

	void SetRefreshDurationEstimate(TInt64 usedObjUid, TInt32 time)
	{
		pddMV_->SetRefreshDurationEstimate(usedObjUid, time);
	}
	void SetDeltaSizeEstimate(TInt64 usedObjUid, TInt32 time)
	{
		pddMV_->SetDeltaSizeEstimate(usedObjUid, time);
	}

	void SetMVForceOption(CRUMVForceOptions *pOpt)
	{
		pMvForceOptions_ = pOpt;
	}

	//-------------------------------------------//
	//	PROTECTED AREA
	//-------------------------------------------//
protected:
	virtual CDDLockList *GetDDLLockList()
	{
		return pddMV_->GetDDLLockList();
	}

	virtual CDDLock *AddDDLLock(CDSString lockName, CDDObject::EOperationType op,
 					 const CDDDetailTextList &details, CDSLocationList *locationList,
 					 Int32 status)
	{
		return pddMV_->AddDDLLock(lockName, op, details, locationList, status);
	}

	virtual void DropDDLLock(const CDSString &name)
	{
		pddMV_->DropDDLLock(name);
	}

	//-------------------------------------------//
	//	PRIVATE AREA
	//-------------------------------------------//
private:
	//-- Prevent copying
	CRUMV(const CRUMV& other);
	CRUMV& operator = (const CRUMV& other);

	void BuildDeltaDefList();

	//-- SetupRecomputeProperty() callee
	BOOL DoIDemandRecompute();

	// Do the MV's properties allow to pipeline to/from it?
	BOOL IsPipelineable();

	// Does the MV's audit type allow pipelining from the second MV?
	BOOL IsCompatibleForPipelining(CRUMV &fromMV); 

	// Check a single privilege
	BOOL FetchPrivilege(CDDObject::EPrivilegeType privType);

	// Consider the purgedata/popindex optimizations
	void SetupPurgedataAndPopindex();

	BOOL MustPerformPurgedata(BOOL isDeletePriv);

	//-------------------------------------------//
	//	PRIVATE AREA
	//-------------------------------------------//
private:
	// The DD core object
	CDDMV	*pddMV_;

	// The list of pointers to used objects.
	// NOTE: THE LIST DOES NOT OWN THE REFERENCED OBJECTS
	//       (and therefore, does not deallocate them).
	CRUTblList *pTablesUsedByMe_;

	CRUTblList *pFullySyncTablesUsedByMe_;

	// The CRUTbl object that corresponds to this MV
	CRUTbl *pTblInterface_;

	// The MV's indexes (for purgedata and popindex purposes)
	CDDIndexList *pddIndexList_;

	// The delta-def list for INTERNAL REFRESH syntax and scheduling
	CRUDeltaDefList *pDeltaDefList_;

	Lng32 privMap_;	// Privileges for the current user
	Lng32 refreshPatternMap_; // Incremental/recompute? Optimizations?
	
	CRUMVForceOptions *pMvForceOptions_;

};

// Declare the class CRUMVList through this macro
DECLARE_PTRLIST(REFRESH_LIB_CLASS, CRUMV)

#endif
