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
* File:         RuRangeCollection.cpp
* Description:  Implementation of class CRURangeCollection
*
* Created:      07/06/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "RuRangeCollection.h"

#include "dmSqlTuple.h"
#include "RuDupElimLogRecord.h"

//--------------------------------------------------------------------------//
//	CRURangeCollection
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	Constructor and destructor
//--------------------------------------------------------------------------//

CRURangeCollection::CRURangeCollection(CDDObject::ERangeLogType rlType) :
	rlType_(rlType),
	rangeList_(eItemsAreOwned),
	pSortedRangeVector_(NULL),
	minEpoch_(0),
	pMaxCKRecord_(NULL),
	balanceCounter_(0)
{}

CRURangeCollection::~CRURangeCollection()
{
	if (NULL != pSortedRangeVector_)
	{
		delete [] pSortedRangeVector_;
	}
}

//--------------------------------------------------------------------------//
//	CRURangeCollection::Reset()
//
//	Release memory and re-initialize the state variables.
//	The collection will live through multiple invocations of Flush().	
//
//--------------------------------------------------------------------------//

void CRURangeCollection::Reset()
{
	rangeList_.RemoveAll();

	if (NULL != pSortedRangeVector_)
	{
		delete [] pSortedRangeVector_;
		pSortedRangeVector_ = NULL;
	}

	balanceCounter_ = 0;
	minEpoch_ = 0;
	pMaxCKRecord_ = NULL;
}

//--------------------------------------------------------------------------//
//	CRURangeCollection::IsClusteringKeyCovered()
//
//	Does *some* range in the collection cover this key's value?
//	(If not, the collection's construction is complete). 
//	Recall that all the ranges in the IUD log are closed.
//
//--------------------------------------------------------------------------//

BOOL CRURangeCollection::
IsClusteringKeyCovered(const CRUIUDLogRecord *pRec) const
{
	if (0 == GetSize())
	{
		return FALSE;
	}

	if (balanceCounter_ > 0)
	{
		return TRUE;	// There is at least one unmatched Begin-range
	}

	// The collection is balanced. 
	// Let's check whether this CK is equal to the maximum CK stored in it.
	RUASSERT(NULL != pMaxCKRecord_);
	return pMaxCKRecord_->IsClusteringKeyEqualTo(*pRec);
}

//--------------------------------------------------------------------------//
//	CRURangeCollection::PerformCrossTypeDE()
//
//	Applicable only if the single-row records are scanned (i.e., 
//	the single-row resolution is enforced).
//
//	Find whether this record must be deleted/updated by DE. If yes,
//	mark both the record's and the screening range's data structures
//	(in the main memory). This will allow to avoid blind I/O.
//
//--------------------------------------------------------------------------//

void CRURangeCollection::PerformCrossTypeDE(CRUIUDLogRecord *pRec)
{
	DSListPosition pos = rangeList_.GetHeadPosition();
	while (NULL != pos)
	{
		CRURange *pRange = rangeList_.GetNext(pos);
		pRange->PerformCrossTypeDE(pRec);
	}
}

//--------------------------------------------------------------------------//
//	CRURangeCollection::InsertRangeBoundary()
//
//	Insert a new Begin-range/End-range record into the collection.
//
//--------------------------------------------------------------------------//

void CRURangeCollection::InsertRangeBoundary(const CRUIUDLogRecord *pRec)
{
	RUASSERT(FALSE == pRec->IsSingleRowOp());

	// The scan order guarantees that the last record has the greatest CK.
	pMaxCKRecord_ = pRec;

	TInt32 ep = pRec->GetEpoch();
	UpdateMinEpoch(ep);

	if (FALSE == pRec->IsBeginRange())
	{
		balanceCounter_--;
		if (balanceCounter_ < 0)
		{
			// Incorrent (unbalanced) logging - should never happen
			CRUException ex;
			ex.SetError(IDS_RU_UNBALANCED_LOGGING);
			throw ex;
		}

		// Find the open range which has a Begin-range record
		// that matches this End-range record 
		LocateMatchForER(pRec);
	}
	else
	{
		balanceCounter_++;

		// Open a new range
		CRURange *pNewRange = new CRURange();
		pNewRange->SetBRRecord(pRec);
		rangeList_.AddTail(pNewRange);
	}
}

//--------------------------------------------------------------------------//
//	CRURangeCollection::PrepareForFlush()
//
//	Before the flush, verify that all the ranges are balanced, 
//	and sort them by the temporal (syskey) order, which will allow
//	I/O optimizations (see CRUDupElimRangeResolver).
//
//--------------------------------------------------------------------------//

void CRURangeCollection::PrepareForFlush()
{
	Lng32 size = GetSize();
	RUASSERT(size > 0);

	VerifyBalance();
	
	typedef CRURange *PCRURange;
	pSortedRangeVector_ = new PCRURange[size];

	DSListPosition pos = rangeList_.GetHeadPosition();
	for (Int32 i=0; NULL != pos; i++)
	{
		CRURange *pRange = rangeList_.GetNext(pos);

		// Initialize the fragment list
		pRange->PrepareForFlush();

		pSortedRangeVector_[i] = pRange;
	}

	// Sort the ranges by the temporal order
	qsort(pSortedRangeVector_, size, sizeof(PCRURange), CompareElem);
}

//--------------------------------------------------------------------------//
//	CRURangeCollection::PerformRangeAnalysis()
//
//	This method is called by CRUDupElimRangeResolver if the
//	collection has more than one range in it (and hence,
//	there are at least two overlapping ranges).
//
//	In order to solve the conflicts, every range should be analysed 
//	with respect to all the ranges that have been logged later
//	(i.e., those that it's inferior to).
//
//	ASSUMPTION: The list of ranges is ordered by SYSKEY before the call.
//
//--------------------------------------------------------------------------//

void CRURangeCollection::PerformRangeAnalysis()
{
	Lng32 size = GetSize();

	for (Int32 oldIndex=0; oldIndex<size; oldIndex++)
	{
		CRURange *pOlderRng = pSortedRangeVector_[oldIndex];
		
		for (Int32 youngIndex = oldIndex+1; youngIndex<size; youngIndex++)
		{
			CRURange *pYoungerRng = pSortedRangeVector_[youngIndex];
			pOlderRng->PerformRangeAnalysis(*pYoungerRng);
		}
	}
}

//--------------------------------------------------------------------------//
//	PRIVATE AREA
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRURangeCollection::LocateMatchForER()
//
//	Find the begin-range record that matches this end-range record, 
//	and update the appropriate boundary pair.
//
//	Algorithm: search for the greatest value of BR-syskey 
//	which is *not* greater than this record's syskey. This is based 
//	on an *assumption* that for an originally logged (BR,ER) pair, 
//	the timestamps are close enough (i.e., there is no other BR record 
//	that has a syskey between theirs). 
//	
//	Do not perform this algorithm for manually range-logged tables. 
//	For such tables, no cross-type duplicates exist. Besides, the matching 
//	algorithm relies on comparison between syskeys, which is correct only 
//	within the same partition's boundaries. 
//
//	For hash-partitioned tables, manually-logged ranges can span partitions, 
//	and the relationship between BR-syskey and ER-syskey is not defined. 
//	In this case, we always match the end-range record to the *some* 
//	open range in the list. This ensures that the match is always found, and 
//	preserves the user's semantics if the ranges are either disjoint or touching:
// 
//	--------             or    ----------
//	          ------                     --------
//
//	However, if the user manually logs overlapping ranges (which is forbidden,
//	by the external), the semantics will not be preserved.
//
//--------------------------------------------------------------------------//

void CRURangeCollection::LocateMatchForER(const CRUIUDLogRecord *pRec)
{
	CRURange *pMatchingRange = NULL;

	if (CDDObject::eMANUAL == rlType_)
	{
		DSListPosition pos = rangeList_.GetHeadPosition();
		while (NULL != pos)
		{
			CRURange *pRange = rangeList_.GetNext(pos);
			if (NULL == pRange->GetERRecord())
			{
				pMatchingRange = pRange;
				break;
			}
		}
	}
	else
	{
		TInt64 erSyskey = pRec->GetSyskey();

		DSListPosition pos = rangeList_.GetHeadPosition();
		TInt64 matchingSyskey = 0;

		while (NULL != pos)
		{
			CRURange *pRange = rangeList_.GetNext(pos);
			
			if (NULL != pRange->GetERRecord())
			{
				continue;	// This one is already matched
			}

			TInt64 brSyskey = pRange->GetBRRecord()->GetSyskey();
			if (brSyskey > erSyskey) 
			{
				continue;	// This cannot match for sure
			}

			if (NULL == pMatchingRange
				||
				brSyskey > matchingSyskey)
			{
				pMatchingRange = pRange;
				matchingSyskey = brSyskey;
			}
		}
	}

	if (NULL == pMatchingRange)
	{
		// Incorrent (unbalanced) logging - should never happen
		CRUException ex;
		ex.SetError(IDS_RU_UNBALANCED_LOGGING);
		throw ex;
	}

	pMatchingRange->SetERRecord(pRec);
}

//--------------------------------------------------------------------------//
//	CRURangeCollection::UpdateMinEpoch()
//--------------------------------------------------------------------------//

void CRURangeCollection::UpdateMinEpoch(TInt32 ep)
{
	if (0 == minEpoch_)
	{
		minEpoch_ = ep;
	}	
	else
	{
		minEpoch_ = (ep < minEpoch_) ? ep : minEpoch_;
	}
}

//--------------------------------------------------------------------------//
//	CRURangeCollection::VerifyBalance()
//
//	Called by CRURangeResolver::Flush()
//
//	Verify that the number of Begin-range records is equal to the number
//	of the End-range records. If this condition holds, the InsertRangeBoundary()
//	method guarantees that boundaries exist (ar not null) for every range 
//	in the collection.
//
//--------------------------------------------------------------------------//

void CRURangeCollection::VerifyBalance()
{
	if (balanceCounter_ != 0)
	{
		// Incorrent (unbalanced) logging - should never happen
		CRUException ex;
		ex.SetError(IDS_RU_UNBALANCED_LOGGING);
		throw ex;
	}
}

//--------------------------------------------------------------------------//
//	CRURangeCollection::CompareElem()
//
//	The function serves for the quicksort criteria.
//--------------------------------------------------------------------------//

Int32 CRURangeCollection::CompareElem(const void *pEl1, const void *pEl2)
{
	CRURange *pRng1 = *(CRURange **)pEl1;
	CRURange *pRng2 = *(CRURange **)pEl2;

	return (pRng1->GetRangeId() < pRng2->GetRangeId()) ? -1 : 1;
}
