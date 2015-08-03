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
#ifndef _RU_EMP_CHECK_VECTOR_H_
#define _RU_EMP_CHECK_VECTOR_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuEmpCheckVector.h
* Description:  Definition of classes 
*				CRUEmpCheckVector and CRUEmpCheckVecIterator
*				
*
* Created:      08/28/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "refresh.h"
#include "dsplatform.h"
#include "dsmap.h"

#include "RuTbl.h"

class CUOFsIpcMessageTranslator;

//--------------------------------------------------------------------------//
//	CRUEmpCheckVector
//	
//	The emptiness check vector is a class that stores the results 
//	of delta emptiness checks. A delta emptiness check tests whether 
//	the table's log is	non-empty with regards to a specific ON REQUEST MV 
//	(which observes the log from a certain epoch).
//
//	The vector consists of the (epoch, checkBitmap) pairs, ordered by the 
//	epoch number. There are two bits in the check bitmap:
//	(1) Are there single-row records in the log from this epoch on?
//	(2) Are there range records in the log from this epoch on?
//
//	In order to build the vector, every "interested" MV registers its
//	begin-epoch with the vector (using the AddEpoch() method). 
//	When the registration is over, the vector will be sorted.
//
//	The vector is *final* if it is known for sure that the deltas 
//	will change no more during the utility's execution.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUEmpCheckVector {

private:
	friend class CRUEmpCheckVecIterator;

public:
	CRUEmpCheckVector();
	CRUEmpCheckVector(const CRUEmpCheckVector &other);

	virtual ~CRUEmpCheckVector();

public:
	// A single element of the vector
	struct Elem {
		
		Elem() : epoch_(0), checkBitmap_(0) {}

		TInt32 epoch_;
		Lng32 checkBitmap_;		
	};

	//-----------------------------------//
	// Accessors
	//-----------------------------------//
public:
	BOOL IsValid() const
	{
		return (0 != size_);
	}

	// Is this a final check or a delta can still appear 
	BOOL IsFinal() const
	{
		return isFinal_;
	}

	// Is the delta empty with regards to a certain MV 
	// which observes it from a certain epoch?
	BOOL IsDeltaNonEmpty(
		TInt32 epoch, 
		Lng32 checkMask = 
			CRUTbl::SINGLE_ROW 
			| 
			CRUTbl::RANGE
	) const;

	// Is the delta insert only with regards to a certain MV 
	// which observes it from a certain epoch?
	BOOL IsDeltaInsertOnly(TInt32 epoch) const;

	TInt32 GetMinEpoch() const
	{
		RUASSERT(NULL != pVec_);
		return pVec_[0].epoch_;
	}

	//-----------------------------------//
	// Mutators
	//-----------------------------------//
public:
	CRUEmpCheckVector &operator = (const CRUEmpCheckVector& other);

	void AddEpochForCheck(TInt32 epoch)
	{
		epMap_[epoch] = 1;
	}

	// Build the vector based on the map of begin-epochs
	// (prepared by the client task)
	void Build();

	// Mark that the delta contains records of this type from this epoch on
	void SetDeltaNonEmpty(TInt32 epoch, CRUTbl::IUDLogContentType ct);

	void SetAllDeltasNonEmpty();

	void SetFinal() 
	{
		isFinal_ = TRUE;
	}

	// IPC pack/unpack
	void LoadData(CUOFsIpcMessageTranslator &translator);
	void StoreData(CUOFsIpcMessageTranslator &translator);

private:
	typedef CDSLongMap<Lng32> EpochMap;
	static Int32 CompareElem(const void *pEl1, const void *pEl2);

private:
	Elem *pVec_;
	Lng32 size_;
	
	BOOL isFinal_;

	EpochMap epMap_;
};

//--------------------------------------------------------------------------//
//	CRUEmpCheckVecIterator
//
//	Traverse the vector's elements, in the decsending epoch  order. 
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUEmpCheckVecIterator {

public:
	CRUEmpCheckVecIterator(const CRUEmpCheckVector& vec) :
	  vec_(vec), i_(vec.size_-1) {}

	virtual ~CRUEmpCheckVecIterator() {}

public:
	void Next()
	{
		if (i_ >= 0) i_--;
	}

	CRUEmpCheckVector::Elem *GetCurrentElem() const
	{
		return (i_ >= 0) ? &(vec_.pVec_[i_]) : NULL;
	}

private:
	const CRUEmpCheckVector &vec_;
	Int32 i_;
};

#endif
