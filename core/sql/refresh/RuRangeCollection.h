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
#ifndef _RU_RANGE_COLLECTION_H_
#define _RU_RANGE_COLLECTION_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuRangeCollection.h
* Description:	Definition of classes 
*               CRURangeCollection and CRURangeCollectionIterator
*
* Created:      07/06/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"
#include "dsptrlist.h"
#include "ddobject.h"
#include "RuRange.h"

//--------------------------------------------------------------------------//
//	CRURangeCollection
//
//	This class is used by the Duplicate Elimination algorithm.
//
//	A *range collection* is a data structure that holds the currently 
//	active ranges. 

//	CRUDupElimRangeResolver gradually builds the instance of CRURangeCollection
//	by adding the range boundary records as they arrive from the delta.
//	The construction is complete when all the ranges in the collection 
//	are closed, no more delta records can overlap them.
//
//	Once the construction is over, the resolver will perform 
//	a *range analysis* on the collection, i.e., turn every range in it 
//	into a set of disjoint fragments.
//
//	If the scan includes the single-row records (i.e., the single-row
//	resolution is enforced), *cross-type DE* will be performed between
//	single-row records and ranges that screen them. 
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRURangeCollection {

public:
	CRURangeCollection(CDDObject::ERangeLogType rlType);
	virtual ~CRURangeCollection();

	//-------------------------------//
	//	Accessors
	//-------------------------------//		
public:
	Lng32 GetSize() const
	{
		return const_cast<CRURangeList &>(rangeList_).GetCount();
	}

	TInt32 GetMinEpoch() const
	{
		return minEpoch_;
	}

	// Does *some* range in the collection cover this key's value?
	// (If not, the collection's construction is complete)
	BOOL IsClusteringKeyCovered(const CRUIUDLogRecord *pRec) const;

	//-------------------------------//
	//	Mutators
	//-------------------------------//		
public:
	// Check whether the single-row record must be deleted/updated 
	void PerformCrossTypeDE(CRUIUDLogRecord *pRec);

	// Insert a new record into the collection
	void InsertRangeBoundary(const CRUIUDLogRecord *pRec);

	// Sort the ranges by the syskey order
	void PrepareForFlush();

	// Solve all the overlap conflicts within the collection
	void PerformRangeAnalysis();

	// Cleanup the data structures after flush
	void Reset();

private:
	// Prevent copying
	CRURangeCollection(const CRURangeCollection &other);
	CRURangeCollection &operator = (const CRURangeCollection &other);

private:
	friend class CRURangeCollectionIterator;

private:
	// Find the range that the Begin-range record matches 
	// this End-range record.
	void LocateMatchForER(const CRUIUDLogRecord *pRec);

	void UpdateMinEpoch(TInt32 ep);

	// Verify the balance before flush
	void VerifyBalance();

	// Comparison function for qsort
	static Int32 CompareElem(const void *pEl1, const void *pEl2);

private:
	CDDObject::ERangeLogType rlType_;

	CRURangeList rangeList_;
	CRURange **pSortedRangeVector_;

	// The row with the maximum clustering key so far (the last one)
	const CRUIUDLogRecord *pMaxCKRecord_;

	TInt32 minEpoch_;
	Int32 balanceCounter_;
};

//--------------------------------------------------------------------------//
//	CRURangeCollectionIterator
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRURangeCollectionIterator {

public:
	enum IterDirection {
	
		DIR_FORWARD,
		DIR_BACKWARD
	};

	CRURangeCollectionIterator(
		const CRURangeCollection &coll, 
		IterDirection dir) : 

		pVec_(coll.pSortedRangeVector_), 
		size_(coll.GetSize()),
		dir_(dir)
	{
		RUASSERT(NULL != pVec_);
		i_ = (DIR_FORWARD == dir) ? 0 : size_-1 ;
	}

	virtual ~CRURangeCollectionIterator() {}

public:
	CRURange *GetCurrent() const
	{
		if (i_ < 0 || i_ >= size_)
		{
			return NULL;
		}

		return pVec_[i_];
	}

	void Next()
	{
		if (DIR_FORWARD == dir_) i_++; else i_--;
	}

private:
	CRURange **pVec_;
	Lng32 size_;

	IterDirection dir_;
	Int32 i_;
};

#endif
