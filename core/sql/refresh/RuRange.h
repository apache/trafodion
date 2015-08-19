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
#ifndef _RU_RANGE_H_
#define _RU_RANGE_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuRange.h
* Description:  Definition of classes CRURange and the auxiliary classes
*
* Created:      07/06/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"
#include "dsptrlist.h"

#include "RuException.h"

class CRUIUDLogRecord;

//--------------------------------------------------------------------------//
//	CRURangeBoundaryPair
//
//	This class stores the *pointers* to a range's boundaries (Begin-range
//	and End-range records).	The actual storage of the boundaries' data 
//	is outside the classes implemented in this file. It is performed by
//	CRUDupElimRangeResolver, which handles (and optimizes) the memory
//	management for the "range" part of the Duplicate Elimination algorithm.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRURangeBoundaryPair {

public:
	CRURangeBoundaryPair() : pBRRec_(NULL), pERRec_(NULL) {}
	virtual ~CRURangeBoundaryPair() {}

public:
	const CRUIUDLogRecord *GetBRRecord() const
	{
		return pBRRec_;
	}
	const CRUIUDLogRecord *GetERRecord() const
	{
		return pERRec_;
	}

public:
	void SetBRRecord(const CRUIUDLogRecord *pBRRec)
	{
		RUASSERT(NULL != pBRRec);
		pBRRec_ = pBRRec;
	}
	void SetERRecord(const CRUIUDLogRecord *pERRec)
	{
		RUASSERT(NULL != pERRec);
		pERRec_ = pERRec;
	}

private:
	//-- Prevent copying
	CRURangeBoundaryPair(const CRURangeBoundaryPair &other);
	CRURangeBoundaryPair &operator = (const CRURangeBoundaryPair &other);

private:
	const CRUIUDLogRecord *pBRRec_;	// Begin-range
	const CRUIUDLogRecord *pERRec_;	// End-range
};

//--------------------------------------------------------------------------//
//	CRURangeFragment
//
//	The range analysis algorithm can split a range into a number of disjoint
//	fragments. Each of these fragments can be either open or closed on both
//	boundaries. All the range's fragments will be finally written to the 
//	range log, each as a separate range. 
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRURangeFragment {

public:
	CRURangeFragment() :
	  bp_(), type_(IS_BR_INCLUDED | IS_ER_INCLUDED) {}

	virtual ~CRURangeFragment() {}

public:
	enum BoundaryBitmap {

		IS_BR_INCLUDED = 0x1,
		IS_ER_INCLUDED = 0x2
	};

	const CRUIUDLogRecord *GetBRRecord() const
	{
		return bp_.GetBRRecord();
	}
	const CRUIUDLogRecord *GetERRecord() const
	{
		return bp_.GetERRecord();
	}

	// The boundaries' bitmap
	Lng32 GetType() const 
	{ 
		return type_; 
	}

public:
	void SetBRRecord(const CRUIUDLogRecord *pBRRec)
	{
		bp_.SetBRRecord(pBRRec);
	}
	void SetERRecord(const CRUIUDLogRecord *pERRec)
	{
		bp_.SetERRecord(pERRec);
	}

	// Mark the lower/upper boundary (or both) as unincluded into the range
	void UnsetTypeBit(unsigned short mask)
	{
		type_ &= ~ mask;
	}

private:
	//-- Prevent copying
	CRURangeFragment(const CRURangeFragment &other);
	CRURangeFragment &operator = (const CRURangeFragment &other);

private:
	CRURangeBoundaryPair bp_;
	Lng32 type_;
};

// Delcare the class CRURangeFragment with this macro
DECLARE_PTRLIST(REFRESH_LIB_CLASS, CRURangeFragment);

//--------------------------------------------------------------------------//
//	CRURange
//
//	This class represents a single range, originally logged into the 
//	IUD log, throughout the processing by the Duplicate Elimination 
//	algorithm.
//
//	The range's original boundaries will be stored in the *original 
//	boundary pair*. 
//
//	Before the active ranges in the memory are flushed to the disk,
//	the range analysis is performed between them. Following it, a range
//	can be split into a number of fragments. Therefore, the class contains
//	a *fragment list*. Every fragment of the range is disjoint with every 
//	other fragment (of this or other range). Each fragment will become 
//	a separate record in the range log. For efficient identification of 
//	all of the fragments that belong to the same range, they will have
//	the same *range Id*, which is equal to the syskey of the original
//	range's Begin-range record.
//
//	The class handles two types of duplicate elimination:
//	(1) Range analysis (against a range logged later than me).
//	(2) Cross-type DE (which is applicable only in the case the whole
//	    delta is scanned).
//
//	If cross-type DE is performed, the flags whether to delete/update
//	the single-row records screened by the range can be computed precisely.
//	Otherwise, the I/O is blind in most cases (a tradeoff for a smaller scan).
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRURange {

public:
	CRURange();
	virtual ~CRURange() {}

	//-------------------------------//
	//	Accessors
	//-------------------------------//		
public:
	const CRUIUDLogRecord *GetBRRecord() const
	{
		return origBoundaryPair_.GetBRRecord();
	}

	const CRUIUDLogRecord *GetERRecord() const
	{
		return origBoundaryPair_.GetERRecord();
	}

	const CRURangeFragmentList &GetFragmentList() const
	{
		return fragmentList_;
	}

	TInt32 GetEpoch() const
	{
		return epoch_;
	}
	TInt64 GetRangeId() const
	{
		return rangeId_;
	}

	BOOL IsUpdateOfScreenedRecordsRequired() const
	{
		return isUpdateOfScreenedRecordsRequired_;
	}
	BOOL IsDeleteOfScreenedRecordsRequired() const
	{
		return isDeleteOfScreenedRecordsRequired_;
	}

	//-------------------------------//
	//	Mutators
	//-------------------------------//		
public:
	void SetBRRecord(const CRUIUDLogRecord *pBRRec);
	void SetERRecord(const CRUIUDLogRecord *pERRec);

	//-- Duplicates resolution --//

	// Applicable only if the whole delta is scanned
	void PerformCrossTypeDE(CRUIUDLogRecord *pRec);

	// The other range must be "younger" than me
	void PerformRangeAnalysis(const CRURange &other);

	// Initialize the fragment list
	void PrepareForFlush();

private:
	//-- Prevent copying
	CRURange(const CRURange &other);
	CRURange &operator = (const CRURange &other);

private:
	BOOL IsClusteringKeyCovered(const CRUIUDLogRecord *pRec) const;

private:
	// PerformRangeAnalysis() callees 

	// Indicator of where the record lies with regards to the range
	enum ExposureType {
		
		SMALLER_EXPOSED = 0,
		NOT_EXPOSED,
		GREATER_EXPOSED
	};
		
	void PerformRAIfMyBRIsSmallerExposed(
		const CRURange &otherRange,
		ExposureType erExpType);

	void PerformRAIfMyBRIsNotExposed(
		const CRURange &otherRange,
		ExposureType erExpType);

	void CutMyFragmentFromBegin(const CRURange &otherRange);
	void CutMyFragmentFromEnd(const CRURange &otherRange);
	void SplitMyFragment(const CRURange &otherRange);
	void RemoveMyFragment();

	ExposureType GetExposureType(
		const CRUIUDLogRecord &rec, 
		const CRURange &otherRange
	);

private:
	CRURangeBoundaryPair origBoundaryPair_;
	CRURangeFragmentList fragmentList_;

	TInt32 epoch_;
	TInt64 rangeId_;

	// Applicable only if the whole delta is scanned
	BOOL isUpdateOfScreenedRecordsRequired_;
	BOOL isDeleteOfScreenedRecordsRequired_;

	// State variables for the range analysis
	DSListPosition myPos_;
	CRURangeFragment *pMyFragment_;
};

// Delcare the class CRURangeList with this macro
DECLARE_PTRLIST(REFRESH_LIB_CLASS, CRURange);

#endif
