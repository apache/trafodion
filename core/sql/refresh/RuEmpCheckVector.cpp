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
* File:         RuEmpCheckVector.cpp
* Description:  Implementation of class	CRUEmpCheckVector
*				
*
* Created:      08/28/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuEmpCheckVector.h"
#include "uofsIpcMessageTranslator.h"

//--------------------------------------------------------------------------//
//	Constructors and destructor
//--------------------------------------------------------------------------//

CRUEmpCheckVector::CRUEmpCheckVector()	:
	pVec_(NULL), 
	size_(0), 
	isFinal_(FALSE)
{}

CRUEmpCheckVector::CRUEmpCheckVector(const CRUEmpCheckVector &other) :
	size_(other.size_),
	isFinal_(other.isFinal_)
{
	if (0 == size_)
	{
		return;
	}

	pVec_ = new Elem[size_];

	for (Int32 i=0; i<size_; i++)
	{
		pVec_[i] = other.pVec_[i];	// Default copy
	}
}

CRUEmpCheckVector::~CRUEmpCheckVector()
{
	delete[] pVec_;
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckVector::LoadData()
//--------------------------------------------------------------------------//

void CRUEmpCheckVector::LoadData(CUOFsIpcMessageTranslator &translator)
{
	// Handle size_ data member
	translator.ReadBlock(&size_, sizeof(Lng32));

	// Handle final_ data member
	translator.ReadBlock(&isFinal_,sizeof(BOOL));

	if (NULL == pVec_)
	{
		pVec_ = new Elem[size_];
	}

	// Handle pVec_ data member
	for (Int32 i=0; i<size_; i++)
	{
		translator.ReadBlock(&(pVec_[i].epoch_),sizeof(TInt32));
		translator.ReadBlock(&(pVec_[i].checkBitmap_), sizeof(Lng32));
	}
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckVector::StoreData()
//--------------------------------------------------------------------------//

void CRUEmpCheckVector::StoreData(CUOFsIpcMessageTranslator &translator)
{
	// Handle size_ data member
	translator.WriteBlock(&size_, sizeof(Lng32));

	// Handle final_ data member
	translator.WriteBlock(&isFinal_,sizeof(BOOL));
	
	// Handle pVec_ data member
	for (Int32 i=0; i<size_; i++)
	{
		translator.WriteBlock(&(pVec_[i].epoch_),sizeof(TInt32));
		translator.WriteBlock(&(pVec_[i].checkBitmap_), sizeof(Lng32));
	}
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckVector::operator =
//--------------------------------------------------------------------------//

CRUEmpCheckVector &CRUEmpCheckVector::
operator = (const CRUEmpCheckVector& other)
{
	if (this == &other)
	{
		return *this;
	}

	if (NULL != pVec_)
	{
		RUASSERT(size_ == other.size_);
	}
	else
	{
		size_ = other.size_;
		pVec_ = new Elem[size_];
	}

	for (Int32 i=0; i<size_; i++)
	{
		pVec_[i] = other.pVec_[i];	// Default copy
	}

	return *this;
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckVector::IsDeltaNonEmpty()
//
//	Does the delta contain the certain type of record starting from
//	this epoch?
//
//	Let's refer to the corresponding bit in the bitmap as to a flag.
//	The flags in the vector should always be non-ascending, i.e., 
//	this picture can happen:
//
//	EPOCH	200		202		205
//	FLAG	TRUE	TRUE	FALSE
//
//	but this one cannot:
//
//	EPOCH	200		202		205
//	FLAG	FALSE	FALSE	TRUE
//
//	Hence, the delta is not empty if there is an element *el* 
//	in the vector for which:
//	(1) el.epoch >= epoch && (2) el.flag == TRUE
//		
//	For the epoch value which is different from MV.EPOCH[T] for any 
//	MV on T, the returned value is equal to the one for the closest
//	smaller epoch in the vector.
//
//--------------------------------------------------------------------------//

BOOL CRUEmpCheckVector::IsDeltaNonEmpty(TInt32 epoch, Lng32 checkMask) const
{
	RUASSERT(NULL != pVec_);

	for (Int32 i=size_-1; i>=0; i--)
	{
		Elem &el = pVec_[i];

		if (0 != (checkMask & el.checkBitmap_))
		{
			return TRUE;	// Found it!
		}

		if (el.epoch_ <= epoch)
		{
			break; 	// No point for more checks.
		}
	}

	return FALSE;
}

//--------------------------------------------------------------------------//
// Return FALSE if there are any single row deletes or updates in the delta.
//--------------------------------------------------------------------------//
BOOL CRUEmpCheckVector::IsDeltaInsertOnly(TInt32 epoch) const
{
	RUASSERT(NULL != pVec_);

	// Look for deleted or updates
	for (Int32 i=size_-1; i>=0; i--)
	{
		Elem &el = pVec_[i];

		if (0 != (CRUTbl::SINGLE_ROW_OTHER & el.checkBitmap_))
		{
			return FALSE;	// Found it!
		}

		if (el.epoch_ <= epoch)
		{
			break; 	// No point for more checks.
		}
	}

	return TRUE;
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckVector::Build()
//
//	Build the vector (with all the flags initialized to FALSE) 
//	based on the client incremental Refresh tasks. Given the map of 
//	begin-epochs, sort the vector by the value of begin-epoch.
//
//--------------------------------------------------------------------------//

void CRUEmpCheckVector::Build()
{
	size_ = epMap_.GetCount();
	if (0 == size_)
	{
		return;
	}
	
	pVec_ = new Elem[size_];

	CDSMapPosition<Lng32> mpos;
	Lng32 i, ep, val;

	for (i=0, epMap_.GetStartPosition(mpos); TRUE == mpos.IsValid(); i++)
	{
		epMap_.GetNextAssoc(mpos, ep, val);
		pVec_[i].epoch_ = ep;
	}
	RUASSERT(i == size_);

	qsort(pVec_, size_, sizeof(Elem), CompareElem);
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckVector::SetDeltaNonEmpty()
//
//	Mark that the log contains records of certain type starting
//	from this epoch (and hence, from all the epochs smaller than it).
//
//--------------------------------------------------------------------------//

void CRUEmpCheckVector::
SetDeltaNonEmpty(TInt32 epoch, CRUTbl::IUDLogContentType ct)
{
	for (Int32 i=0; i<size_; i++)
	{
		Elem &el = pVec_[i];

		el.checkBitmap_ |= ct;	// Raise the bit

		if (epoch == el.epoch_)
		{
			return;
		}
	}

	// The target epoch was never encountered - error!
	RUASSERT(FALSE);
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckVector::SetAllDeltasNonEmpty()
//
//	Called by: CRUSimpleRefreshTaskExecutor::FinalMetadataUpdate()
//
//	After an ON REQUEST MV is refreshed *incrementally*, we suppose
//	that it has written something to the log. Hence, its delta
//	is non-empty for each using ON REQUEST MV.
//
//--------------------------------------------------------------------------//

void CRUEmpCheckVector::SetAllDeltasNonEmpty()
{
	for (Int32 i=0; i<size_; i++)
	{
		pVec_[i].checkBitmap_ |= CRUTbl::SINGLE_ROW;
	}
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckVector::CompareElem()
//
//	The function serves for the quicksort criteria.
//--------------------------------------------------------------------------//

Int32 CRUEmpCheckVector::CompareElem(const void *pEl1, const void *pEl2)
{
	return (((CRUEmpCheckVector::Elem *)pEl1)->epoch_ 
			<
			((CRUEmpCheckVector::Elem *)pEl2)->epoch_ 
			) ? -1 : 1;
}
