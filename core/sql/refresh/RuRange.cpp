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
/*  -*-C++-*-
******************************************************************************
*
* File:         RuRange.cpp
* Description:  Implementation of class CRURange and the auxiliary classes
*
* Created:      08/01/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "ddobject.h"
#include "RuRange.h"
#include "RuDupElimLogRecord.h"

//--------------------------------------------------------------------------//
//	Constructors
//--------------------------------------------------------------------------//

CRURange::CRURange() :
	origBoundaryPair_(),
	fragmentList_(eItemsAreOwned),
	epoch_(0),
	rangeId_(0),
	isDeleteOfScreenedRecordsRequired_(FALSE),
	isUpdateOfScreenedRecordsRequired_(FALSE),
	// Range analysis state
	pMyFragment_(NULL),
	myPos_(NULL)
{}

//--------------------------------------------------------------------------//
//	CRURange::SetBRRecord()
//--------------------------------------------------------------------------//

void CRURange::SetBRRecord(const CRUIUDLogRecord *pBRRec)
{
	RUASSERT(NULL != pBRRec 
			&& 
			NULL == origBoundaryPair_.GetBRRecord());

	origBoundaryPair_.SetBRRecord(pBRRec);

	epoch_ = pBRRec->GetEpoch();
	rangeId_ = pBRRec->GetSyskey();
}

//--------------------------------------------------------------------------//
//	CRURange::SetERRecord()
//--------------------------------------------------------------------------//

void CRURange::SetERRecord(const CRUIUDLogRecord *pERRec)
{
	RUASSERT(NULL != pERRec);
	origBoundaryPair_.SetERRecord(pERRec);
}

//--------------------------------------------------------------------------//
//	CRURange::PerformCrossTypeDE()
//
//	This method is applied only if the single-row resolution is enforced
//	(and hence, both single-row and range records are scanned).
//
//	A single-row IUD log record is *screened* by a range if:
//	(1) It is *covered* by this range.
//	(2) The range was logged *before* the record.
//
//	If the screening epoch is smaller than the record's one, the record's 
//	@IGNORE mark in the log must be updated the screening range's epoch. 
//	
//	If the screening epoch is equal to the record's one, 
//	the record must be deleted from the log.
//
//	These decisions will be marked in the record's internal flags.
//	They will save work for the single-row resolver, if there is one.
//
//	For example, if the single-row resolver (that works after the range 
//	resolver) decides to delete a record while the range resolver made the
//	same decision, the single-row resolver will not perform the operation.
//
//	The range resolver will also reflect the decisions in its own flags
//	(isDeleteOfScreenedRecordsRequired_ and isUpdateOfScreenedRecordsRequired_)
//	in order to save "blind" delete/update in the IUD log during the flush.
//
//--------------------------------------------------------------------------//

void CRURange::PerformCrossTypeDE(CRUIUDLogRecord *pRec)
{
	RUASSERT(TRUE == pRec->IsSingleRowOp());

	if (FALSE == IsClusteringKeyCovered(pRec)
		||
		pRec->GetSyskey() < GetBRRecord()->GetSyskey())
	{
		return;
	}

	TInt32 ep = pRec->GetEpoch();

	if (ep > epoch_)	
	{
		// The @IGNORE mark must be set to epoch_ ...

		if (epoch_ <= pRec->GetIgnoreMark())
		{
			// However, if it is already epoch_ or more,
			// there is nothing to do!
			return;	
		}

		pRec->SetIgnoreMark(epoch_);
		// Set the flag for the single-row resolver...
		pRec->SetRangeResolvWillUpdateMe();
		// ... and for myself
		isUpdateOfScreenedRecordsRequired_ = TRUE;
	}
	else 
	{
		RUASSERT(ep == epoch_);

		// Set the flag for the single-row resolver...
		pRec->SetRangeResolvWillDeleteMe();
		// ... and for myself
		isDeleteOfScreenedRecordsRequired_ = TRUE;
	}
}

//--------------------------------------------------------------------------//
//	CRURange::IsClusteringKeyCovered()
//
//	The clustering key value CK is *covered* by range R 
//	if it holds that R.<BR-columns> <= CK <= R.<ER-columns>
//	
//	There are two possible cases when the CK is covered.
//	(1) The range is *not* balanced, i.e., the end-range record
//	    matching the begin-range record has not encountered yet.
//
//	(2) The range is balanced, but the end-range value is equal
//		to the CK value.
//
//--------------------------------------------------------------------------//

BOOL CRURange::IsClusteringKeyCovered(const CRUIUDLogRecord *pRec) const
{
	// Case 1 - the range is not balanced yet
	if (NULL == GetERRecord())
	{
		return TRUE;
	}

	// Case 2
	if (TRUE == GetERRecord()->IsClusteringKeyEqualTo(*pRec))
	{
		return TRUE;
	}

	return FALSE;
}

//--------------------------------------------------------------------------//
//	CRURange::PrepareForFlush()
//
//	Initialize the fragment list
//--------------------------------------------------------------------------//

void CRURange::PrepareForFlush()
{
	CRURangeFragment *pFragment = new CRURangeFragment();
	
	pFragment->SetBRRecord(this->GetBRRecord());
	pFragment->SetERRecord(this->GetERRecord());

	fragmentList_.AddTail(pFragment);
}

//--------------------------------------------------------------------------//
//	RANGE ANALYSIS METHODS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRURange::PerformRangeAnalysis()
//
//	Perform the range analysis against the range which has been logged
//	*later* then myself (we call it a *younger* range).
//
//	In the case of overlap between my fragment and the younger range,
//	the following principle guides:
//	- The fragment of (my) range which has been logged earlier must be 
//	  changed (deleted, split, or resized) to resolve the overlap, while 
//	  the range which has been logged later must remain intact.
//
//	This rule is easiest to explain for two ranges that belong to different
//	epocs. The range with the greater epoch number can be observed alone by
//	some MV, but not vice versa. Hence, the range from the smaller epoch 
//	"suffers". 
//
//	The same principle also holds for two overlapping ranges that belong 
//	to the same epoch. This is a requirement of the INTERNAL REFRESH 
//	optimization that filters out the cross-type duplicates through the 
//	Delta Computation Block (rather than through DE).
//
//	The algorithm is organized as a scan through the range's fragment list
//	(which is sorted by the Begin-Range clustering key). Each fragment is 
//	compared to the younger range, and the appropriate DE decision 
//	is performed (always in the younger range's favor).
//
//--------------------------------------------------------------------------//

void CRURange::PerformRangeAnalysis(const CRURange &otherRange)
{
	RUASSERT(GetRangeId() <= otherRange.GetRangeId());

	if (TRUE == fragmentList_.IsEmpty()
		||
		TRUE == otherRange.GetFragmentList().IsEmpty())
	{
		// All the fragments of one of the two ranges are dominated 
		// by "third-party" ranges (and hence removed from the list).
		// Nothing to do in this case.
		return;
	}

	myPos_ = fragmentList_.GetHeadPosition();
	ExposureType brExpType, erExpType;

	while (NULL != myPos_)
	{
		pMyFragment_ = fragmentList_.GetNext(myPos_);

		// Compute whether the begin-range and end-range records 
		// are exposed (i.e., not screened by the other range).
		brExpType = GetExposureType(*(pMyFragment_->GetBRRecord()), otherRange);
		erExpType = GetExposureType(*(pMyFragment_->GetERRecord()), otherRange);
	
		switch (brExpType)
		{
		case SMALLER_EXPOSED:
			{
				PerformRAIfMyBRIsSmallerExposed(otherRange, erExpType);
				break;
			}
		case NOT_EXPOSED:
			{
				PerformRAIfMyBRIsNotExposed(otherRange, erExpType);
				break;
			}
		case GREATER_EXPOSED:
			{
				// This fragment (as well as all the following ones)
				// are not in conflict with the other range
				return;
			}
		default:	RUASSERT(FALSE);
		}
	}
}

//--------------------------------------------------------------------------//
//	CRURange::PerformRAIfMyBRIsSmallerExposed()
//
//	The begin-range value of my current fragment is smaller than
//	the begin-range value of the second range's current fragment.
//
//	The picture is:
//
//  -------------------------------------------------------
//	  ^      (1)        ^       (2)              ^    (3)
//	  |                 |                        |
//	MY BR         OTHER BR                 OTHER ER
//
//	My ER can be located in one of (1), (2) or (3).
//
//	Perform the range analysis in the relevant case.
//
//--------------------------------------------------------------------------//

void CRURange::
PerformRAIfMyBRIsSmallerExposed(const CRURange &otherRange,
								ExposureType erExpType)
{
	switch (erExpType)
	{
		case SMALLER_EXPOSED:
			{
				// Case (1) - No overlap

				//         --------   other
				// ------             my

				break;
			}
		case NOT_EXPOSED:
			{
				// Case (2)

				//	   --------     other  ==>    --------
				//   ------         my          --
			
				CutMyFragmentFromEnd(otherRange);
				break;
			}
		case GREATER_EXPOSED:
			{
				// Case (3)

				//    ---	  other ==>     ---
				//	-------   my          --   --

				SplitMyFragment(otherRange);
				break;
			}
		default:	RUASSERT(FALSE);
	}
}

//--------------------------------------------------------------------------//
//	CRURange::PerformRAIfMyBRIsNotExposed()
//
//	The begin-range value of my current fragment is not exposed (i.e.,
//	is screened by the other range).
//
//	The picture is:
//
//  -------------------------------------------------------
//	  ^                 ^       (1)           ^    (2)
//	  |                 |                     |
//	OTHER BR         MY BR                 OTHER ER
//
//	My ER can be located in one of (1) or (2).
//
//	Perform the range analysis in the relevant case.
//
//--------------------------------------------------------------------------//

void CRURange::
PerformRAIfMyBRIsNotExposed(const CRURange &otherRange,
							ExposureType erExpType)
{
	switch (erExpType)
	{
		case NOT_EXPOSED:
			{
				// Case (1)

				//  -------------   other  ==> ------------- 
				//       ----       my
		
				RemoveMyFragment();
				break;
			}
		case GREATER_EXPOSED:
			{
				// Case (2)

				//	------          other  ==> ------
				//	    -----       my               ---

				CutMyFragmentFromBegin(otherRange);
				break;
			}

		case SMALLER_EXPOSED:	// This cannot happen
		default:	
			{
				RUASSERT(FALSE);
			}
	}
}

//--------------------------------------------------------------------------//
//	CRURange::CutMyFragmentFromBegin()
//
//	Solve the following conflict:
//
//		------          other  ==> ------
//			-----       my               ---
//
//--------------------------------------------------------------------------//

void CRURange::CutMyFragmentFromBegin(const CRURange &otherRange)
{
	pMyFragment_->SetBRRecord(otherRange.GetERRecord());

	// My fragment will become open at the beginning
	pMyFragment_->UnsetTypeBit(CRURangeFragment::IS_BR_INCLUDED);
}

//--------------------------------------------------------------------------//
//	CRURange::CutMyFragmentFromEnd()
//
//	Solve the following conflict:
//
//	   --------     other  ==>    --------
//   ------         my          --
//
//--------------------------------------------------------------------------//

void CRURange::CutMyFragmentFromEnd(const CRURange &otherRange)
{
	pMyFragment_->SetERRecord(otherRange.GetBRRecord());

	// My fragment will become open at the end
	pMyFragment_->UnsetTypeBit(CRURangeFragment::IS_ER_INCLUDED);
}

//--------------------------------------------------------------------------//
//	CRURange::SplitMyFragment()
//
//	Solve the following conflict:
//
//	  ---     other ==>     ---
//	-------   my          --   --
//                        #1   #2
//
//	Following this change, a new fragment (fragment #2 in the picture)
//	will be created an inserted into the fragment list. It will	become 
//	my new current fragment. Fragment #1 will be my old current fragment,
//	cut from the end.
//
//--------------------------------------------------------------------------//

void CRURange::SplitMyFragment(const CRURange &otherRange)
{
	// Create fragment #2
	CRURangeFragment *pNewFragment = new CRURangeFragment();

	pNewFragment->SetBRRecord(otherRange.GetERRecord());
	pNewFragment->SetERRecord(pMyFragment_->GetERRecord());
	// The new fragment is "born" closed from both ends.
	// Set the end-range bit of fragment #2 to be the same
	// as the end range of fragment #1 (open or closed).
	if (0 == (pMyFragment_->GetType() & CRURangeFragment::IS_ER_INCLUDED))
	{
		pNewFragment->UnsetTypeBit(CRURangeFragment::IS_ER_INCLUDED);
	}

	pMyFragment_->SetERRecord(otherRange.GetBRRecord());
	
	//	Fragment #1 will become open at the end. 
	pMyFragment_->UnsetTypeBit(CRURangeFragment::IS_ER_INCLUDED);
	//	Fragment #2 will become open at the beginning.
	pNewFragment->UnsetTypeBit(CRURangeFragment::IS_BR_INCLUDED);

	// Insert the fragment #2 into the list
	if (NULL == myPos_)
	{
		fragmentList_.AddTail(pNewFragment);
	}
	else
	{
		fragmentList_.InsertBefore(myPos_, pNewFragment);
		// Make a step back. The newly-created fragment will become current.
		fragmentList_.GetPrev(myPos_);
	}
}

//--------------------------------------------------------------------------//
//	CRURange::RemoveMyFragment()
//
//	Solve the following conflict:
//
//  -------------   other  ==> ------------- 
//       ----       my
//
//--------------------------------------------------------------------------//

void CRURange::RemoveMyFragment()
{
	if (NULL == myPos_)
	{
		// This is the last node in the list
		fragmentList_.RemoveTail();
	}
	else
	{
		DSListPosition myPrevPos = myPos_;

		// The scan has already skipped my node, make a single step back.
		fragmentList_.GetPrev(myPrevPos);
		fragmentList_.RemoveAt(myPrevPos);
	}
}

//--------------------------------------------------------------------------//
//	CRURange::GetExposureType()
//
//	Check where the record lies with regards to the younger range 
//	(and whether it is screened by it). There are three options:
//
//   <--- SMALLER_EXPOSED    <-- NOT_EXPOSED -->    GREATER_EXPOSED --->
//	------------------------[-------------------]-----------------------
//
//	Note that if the clustering key coincides with one of the range's
//	boundaries, it is always NOT exposed.
//
//	In order to refrain from complex (and datatype-dependent) comparisons 
//	between the data values, the algorithm exploits the fact that the delta 
//	computation query sorts the data by the clustering key. Therefore, 
//	the comparisons are made between the *tags* that reflect the sorting order,
//	rather than the actual values (this is also much faster).
//	
//--------------------------------------------------------------------------//

CRURange::ExposureType CRURange::GetExposureType(const CRUIUDLogRecord &rec, 
												 const CRURange &otherRange)
{
	const CRUIUDLogRecord &otherBRRec = *(otherRange.GetBRRecord());
	const CRUIUDLogRecord &otherERRec = *(otherRange.GetERRecord());

	TInt64 ckTag = rec.GetCKTag();
	TInt64 brCkTag = otherRange.GetBRRecord()->GetCKTag();
	TInt64 erCkTag = otherRange.GetERRecord()->GetCKTag();
	
	RUASSERT(brCkTag <= erCkTag);

	if (ckTag < brCkTag)
	{
		return SMALLER_EXPOSED;
	}
	else
	{
		if (ckTag > erCkTag)
		{
			return GREATER_EXPOSED;
		}
		else
		{
			//	brCkTag <= ckTag <= erCkTag
			return NOT_EXPOSED;
		}
	}
}

// List classes' definition completions
DEFINE_PTRLIST(CRURange);
DEFINE_PTRLIST(CRURangeFragment);
