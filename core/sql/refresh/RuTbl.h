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
#ifndef _RU_TBL_H_
#define _RU_TBL_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RUTbl.h
* Description:  Definition of class CRUTbl
*
* Created:      02/27/2000
* Language:     C++
*
* 
******************************************************************************
*/

#include "refresh.h"
#include "dsplatform.h"

#include "ddTblUsedByMV.h"
#include "RuObject.h"
#include "RuDeltaDef.h"

class CDDTable;
class CDDColumnList;
class CDDKeyColumnList;

class CRUMV;
class CRUMVList;

class CRUKeyColumnList;

class CUOFsFileList;
class CDSStringList;

class CRUEmpCheckVector;

//--------------------------------------------------------------------------//
//	CRUTbl (inherits from CRUObject)
//
//	  CRUTbl is a wrapper class for CDDTblUsedByMV that reflects 
//	  the properties of a table being used by MV in a particular 
//	  invocation of REFRESH. These properties can follow either 
//	  from the metadata (e.g., what is the table's current epoch?),
//	  or from the utility logic (e.g., should this table be read-locked 
//	  throughout this invocation to achieve consistency?)
//
//	  The class provides the functionality of updating the metadata 
//	  and saving it in the SMD and UMD tables, acquiring and releasing 
//	  resources etc.
//	  
//	  The class is an *abstract* class. Several methods depend on the
//	  table's implementation (is it a regular table or an MV itself?).
//	  The concrete implementations are in RuTblImpl[.h/.cpp]
//	
//	  The class maintains the following "core" pointers/objects:
//    (1) The pointer to the core DDOL object, pddTblUsedByMV_.
//
//	  (2) The list of pointers to the CRUMV objects for the involved MVs 
//	      *directly* using the table (that do not specify it as IGNORE 
//        CHANGES), pMVsUsingMe_. This list does not own the referenced 
//        objects, and hence does not deallocate them in its destructor.
//
//	  (3) The pointer to the CRUMV object (MV interface), if the table is
//	      also an involved MV - pMVInterface_.
//
//	  (4) The pointer to the DDOL object for the IUD log table, pddIUDLogTbl_.
//        The operations on the log (i.e., read-protected open) are done
//		  by the utility through the CRUTbl object.
//
//	  There are four auxiliary "using MVs lists" that are maintained. 
//	  All of them are a subset of pMVsUsingMe_ (self-commenting):
// 	  (1) pOnRequestMVsUsingMe_
//	  (2) pInvolvedMVsUsingMe_
//	  (3) pOnRequestInvolvedMVsUsingMe_
//	  (4) pIncInvolvedMVsUsingMe_ - the involved ON REQUEST MVs that will
//		  NOT be recomputed by this invocation of Refresh.
//
//	  Various tasks of the Refresh utility use the class to exchange data
//	  the task's execution has computed in (for example, the DE statistics).
//	  This allows a flexible design of the dependence graph, where various  
//	  tasks need not know the entire graph's connectivity to exchange data.
//	  They just find it at the CRUTbl object.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUTbl  : public CRUObject {

private:
	typedef CRUObject inherited;

public:
	CRUTbl(CDDTblUsedByMV *pddTblUsedByMV);
	virtual ~CRUTbl();

	//--------------------------------//
	//  Accessors
	//--------------------------------//
public:
	// Main usage list
	CRUMVList &GetMVsUsingMe() const 
	{ 
		RUASSERT(NULL != pMVsUsingMe_);
		return *pMVsUsingMe_; 
	}

	// Auxiliary usage lists
	CRUMVList &GetOnRequestMVsUsingMe() const
	{
		RUASSERT(NULL != pOnRequestMVsUsingMe_);
		return *pOnRequestMVsUsingMe_;
	}

	CRUMVList &GetInvolvedMVsUsingMe() const
	{
		RUASSERT(NULL != pInvolvedMVsUsingMe_);
		return *pInvolvedMVsUsingMe_;
	}

	CRUMVList &GetOnRequestInvolvedMVsUsingMe() const
	{
		RUASSERT(NULL != pOnRequestInvolvedMVsUsingMe_);
		return *pOnRequestInvolvedMVsUsingMe_;
	}

	// GetOnRequestInvolvedMVsUsingMe().GetCount() > 0
	BOOL IsUsedByOnRequestMV() const;

	CRUMVList &GetIncrementalInvolvedMVsUsingMe() const
	{
		RUASSERT(NULL != pIncInvolvedMVsUsingMe_);
		return *pIncInvolvedMVsUsingMe_;
	}

	// GetIncrementalInvolvedMVsUsingMe().GetCount() > 0
	BOOL IsUsedByIncRefreshedMV() const;

	BOOL IsInvolvedMV() const
	{
		return (NULL != pMVInterface_);
	}

	CRUMV *GetMVInterface() const
	{
		return pMVInterface_;
	}

public:
	// Pure virtuals - delegated to the derived classes.
	virtual BOOL IsRegularTable() const = 0;

	// If a used object is regular table, it is always initialized
	virtual BOOL IsInitialized() = 0;

	virtual BOOL IsAudited() = 0;

	// Regular tables and ON STATEMENT MVs 
	// are always synchronized with the database
	virtual BOOL IsFullySynchronized() = 0;

	//	A table is insert-only if it has an INSERTLOG or 
	//	MANUAL RANGELOG attribute.
	BOOL IsInsertLog();

	// Get the name of the IUD/range log (not FQ)
	CDSString GetLogShortName(const CDSString &nmsp);

	//-- Access to the data structures
public:
	// Get the STORE BY key column list 
	CRUKeyColumnList &GetKeyColumnList() const
	{
		RUASSERT (NULL != pKeyColumnList_);
		return *pKeyColumnList_;
	}
	
	//-- Get the list of all partition names
	const CDSStringList &GetPartitionFileNamesList() const
	{
		RUASSERT (NULL != pPartitionFileNamesList_);
		return *pPartitionFileNamesList_;
	}

	//-- Expose the statistics gathered by DE
	CRUDeltaStatisticsMap &GetStatisticsMap()
	{
		RUASSERT (NULL != pStatMap_);
		return *pStatMap_;
	}

	// Get the emptiness check's results
	CRUEmpCheckVector &GetEmpCheckVector()
	{
		RUASSERT (NULL != pEmpCheckVector_);
		return *pEmpCheckVector_;
	}

	BOOL IsDeltaNonEmpty(TInt32 fromEpoch);

	CRUDeltaDef::DELevel GetDELevel() const
	{
		return deLevel_;
	}

        Lng32 getNumberOfPartitions();

public:
	// Implementation of pure virtual
	virtual BOOL HoldsResources() const
	{
		return (TRUE == IsDDLLockPending()
				||
				TRUE == IsRPOpenPending()
				||
				TRUE == IsLogRPOpenPending());
	}

	TInt64 GetTimestamp() const
	{
		return timestamp_;
	}

	BOOL IsRPOpenPending() const
	{
		return isRPOpenPending_;
	}

	BOOL IsLogRPOpenPending() const
	{
		return isLogRPOpenPending_;
	}

	// What is the size of the @UPDATE_BITMAP column in the log?
	Lng32 GetUpdateBitmapSize();

public:
	enum IUDLogContentType {

		RANGE	  		= 0x1,	// Range records
		SINGLE_ROW_INSERTS	= 0x2,	// Single-row insert records
		SINGLE_ROW_OTHER	= 0x4,	// Single-row delete and update records
		SINGLE_ROW = SINGLE_ROW_INSERTS | SINGLE_ROW_OTHER // Any Single-row records
	};

	// Which kind of records can the IUD log hold?
	Lng32 GetIUDLogContentTypeBitmap();

	// Should the utility perform DE on this table's log?
	BOOL IsDENeeded() const 
	{
		return isDENeeded_;
	}
	
	BOOL IsUsedByIncrementalMJV() const
	{
		return isUsedByIncrementalMJV_;
	}

	BOOL IsUsedOnlyByMultiTxnMvs() const;

	// Should the table be locked (by read-protected open)
	// throughout the execution?
	BOOL IsLongLockNeeded() const
	{
		return isLongLockNeeded_;
	}

	// Must the table's delta be empty for consistency reasons?
	BOOL IsEmptyDeltaNeeded() const
	{
		return isEmptyDeltaNeeded_;
	}

	BOOL IsIncEpochNeeded() const
	{
		// Epoch increment is required only if there is 
		// an involved ON REQUEST MV that uses the tabke
		// (not necessarily incrementally refreshed).
		return IsUsedByOnRequestMV();
	}

	//-- Accessor wrappers of DDOL methods
public:
	virtual TInt64 GetUID() const 
	{ 
		return pddTblUsedByMV_->GetUID(); 
	}

	BOOL IsNoLockOnRefresh()
	{
		return pddTblUsedByMV_->IsNoLockOnRefresh();
	}

	CDDObject::ERangeLogType GetRangeLogType()
	{
		return pddTblUsedByMV_->GetRangeLogType();
	}

	CDDUIDTripleList &GetUIDsOfAllMVsUsingMe() 
	{
		return pddTblUsedByMV_->GetMVsUsingMe();
	} 

	TInt32 GetCurrentEpoch() 
	{ 
		return pddTblUsedByMV_->GetCurrentEpoch(); 
	}

	TInt32 GetLastDupElimEpoch() 
	{ 
		return pddTblUsedByMV_->GetLastDupElimEpoch(); 
	} 

	TInt32 GetRecomputeEpoch() 
	{ 
		return pddTblUsedByMV_->GetRecomputeEpoch();
	}

	BOOL IsLastDEComplete()
	{
		return pddTblUsedByMV_->IsLastDEComplete();
	}

	// For direct epoch increment (not through DDOL)
	CDSString GetIncEpochCatApiText()
	{
		return pddTblUsedByMV_->GetIncEpochCatApiText();
	}

#ifdef _DEBUG
public:
	virtual void Dump(CDSString &to, BOOL isExtended=FALSE);
#endif

	//--------------------------------//
	//  Log-related globals
	//--------------------------------//
public:
	// Prefix to all the control columns in the log
	static const CDSString logCrtlColPrefix;

	// IUD log namespace prefix
	static const CDSString iudNmspPrefix;

	// Range log namespace prefix
	static const CDSString rngNmspPrefix;

	CDSString GetIUDLogFullName(BOOL useNmsp=TRUE)
	{
		return GetLogFullName(CRUTbl::iudNmspPrefix, useNmsp);
	}

	CDSString GetRangeLogFullName(BOOL useNmsp=TRUE)
	{
		return GetLogFullName(CRUTbl::rngNmspPrefix, useNmsp);
	}

	//--------------------------------//
	//  Mutators
	//--------------------------------//

	//--- Connectivity updates
public:
	//-- Reference management: update the usage lists
	void AddRefToUsingMV(CRUMV *pMV);

	void SetMVInterface(CRUMV *pMV)
	{
		pMVInterface_ = pMV;
	}

	// Store the pointer to the IUD log table
	void FixupIUDLogTbl(CDDTable *pddIUDLogTbl)
	{
		pddIUDLogTbl_ = pddIUDLogTbl;
	}

	// Build the list when you know for sure
	// which MVs are going to be incrementally refreshed
	void BuildListOfIncrementalInvolvedMVsUsingMe();

public:
	//-- Implementation of pure virtual
	virtual void ReleaseResources();

public:
	//-- Build the list of key columns (for the DE purposes)
	void BuildKeyColumnList();

	//-- Build the list of all partition names
	void BuildPartitionFileNamesList();

	//-- Build the emptiness check vector
	void BuildEmpCheckVector();

	void CheckIfLongLockNeeded();
	void SetLongLockIsNeeded()
	{
		isLongLockNeeded_ = TRUE;
	}

	void CheckIfDENeeded();

	virtual void SetTimestamp(TInt64 ts)
	{ 
		timestamp_ = ts; 
	}
	void SetEmptyDeltaNeeded()
	{
		isEmptyDeltaNeeded_ = TRUE;
	}

	void SetDELevel(CRUDeltaDef::DELevel deLevel)
	{
		deLevel_ = deLevel;
	}

public:
	//-- Event propagation to the using MVs
	void PropagateEmpCheckToUsingMVs();
	void PropagateRecomputeToUsingMVs();
	void PropagateDEStatisticsToUsingMVs();

public:
	//-- Pure virtuals - to be refined by the derived classes
	//-- Read-protected opens for achieving consistency 
	virtual void ExecuteReadProtectedOpen() = 0;
	virtual void ReleaseReadProtectedOpen() = 0;

	void ExecuteLogReadProtectedOpen();
	void ReleaseLogReadProtectedOpen();

	//-- Mutator wrappers
public:
	// Fetch the metadata about epochs (not of general use in DDOL).
	virtual void FetchMetadata()
	{
		pddTblUsedByMV_->FetchMVRefreshMetadata();
	}

	// Flush the metadata changes to the SMD and UMD tables
	virtual void SaveMetadata()
	{
		pddTblUsedByMV_->Save();
	}

	// Cancel saving the metadata
	virtual void CancelChanges()
	{
		pddTblUsedByMV_->CancelChanges();
	}

	void IncrementCurrentEpoch(BOOL changeObjectState = TRUE)
	{
		pddTblUsedByMV_->IncrementCurrentEpoch(changeObjectState);
	}
	void SetLastDupElimEpoch(TInt32 ep) 
	{ 
		pddTblUsedByMV_->SetLastDupElimEpoch(ep); 
	}
	void SetRecomputeEpoch(TInt32 ep) 
	{ 
		pddTblUsedByMV_->SetRecomputeEpoch(ep); 
	}
	void SetMinLogEpoch(TInt32 ep)
	{
		pddTblUsedByMV_->SetMinLogEpoch(ep);
	}
	void SetLastDEComplete(BOOL flag)
	{
		pddTblUsedByMV_->SetLastDEComplete(flag);
	}

	//-------------------------------------------//
	//	PROTECTED AREA
	//-------------------------------------------//
protected:
	//-- Pure virtuals
	//-- Retrieve the column and key column lists from DDOL
	virtual CDDColumnList *GetDDColumnList() = 0;
	virtual CDDKeyColumnList *GetDDKeyColumnList() = 0;
	virtual CUOFsFileList *GetPartitionFileList() = 0; 

	//-------------------------------------------//
	//	PRIVATE AREA
	//-------------------------------------------//
private:
	//-- Connectivity into DDOL and inside the cache

	// Common part for regular table and MV that is used by MVs.
	CDDTblUsedByMV *pddTblUsedByMV_;
	
	// Pointer to the IUD log for this table
	CDDTable *pddIUDLogTbl_;

	// Pointer to the MV object, if the table is also an involved MV.
	CRUMV *pMVInterface_;

	// The list of pointers to objects (MVs) using this object.
	// NOTE: THE LIST DOES NOT OWN THE REFERENCED OBJECTS
	//       (and therefore, does not deallocate them).
	CRUMVList *pMVsUsingMe_;

	// Auxiliary pointer lists
	CRUMVList *pOnRequestMVsUsingMe_;
	CRUMVList *pInvolvedMVsUsingMe_;
	CRUMVList *pOnRequestInvolvedMVsUsingMe_;
	CRUMVList *pIncInvolvedMVsUsingMe_;

private:
	//-- Data structures used by various tasks
	CRUKeyColumnList *pKeyColumnList_;

	CDSStringList *pPartitionFileNamesList_;

	// The emptiness check result vector
	CRUEmpCheckVector *pEmpCheckVector_;

	// The Duplicate Elimination level
	CRUDeltaDef::DELevel deLevel_;

	// A hash table with statistical data about the log.
	// A distinct record in it corresponds to each non-
	// empty epoch in the log.
	CRUDeltaStatisticsMap *pStatMap_;

	TInt64 timestamp_;

private:
	//-- Status flags

	// Is the table/log open in read-protected mode?
	BOOL isRPOpenPending_;
	BOOL isLogRPOpenPending_;

	BOOL isUsedByIncrementalMJV_;

	//-- Requirements flags
	BOOL isDENeeded_;
	BOOL isLongLockNeeded_;
	BOOL isEmptyDeltaNeeded_;

private:
	// Return the fully-qualified log name,
	// with/without the namespace
	CDSString GetLogFullName(const CDSString &nmsp, BOOL useNmsp);
};

// Declare the class CRUTblList through this macro
DECLARE_PTRLIST(REFRESH_LIB_CLASS, CRUTbl)

#endif
