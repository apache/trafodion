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
#ifndef _RU_DUPELIM_RANGE_RESOLV_H_
#define _RU_DUPELIM_RANGE_RESOLV_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuDupElimRangeResolv.h
* Description:  Definition of class CRUDupElimRangeResolver 
*
*
* Created:      07/06/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "RuDupElimTaskExUnit.h"
#include "RuDupElimConst.h"
#include "RuRangeCollection.h"
#include "RuDupElimLogRecord.h"

class CRUDupElimLogScanner;
class CRUEmpCheckVector;

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver
//	
//	This class resolves two types of duplicate conflicts:
//	(1) Between the overlapping and subsumed ranges (in other 
//		words, performs the range analysis). 
//	(2) Between the single-row and range- records (cross-type
//		duplicates). 
//
//	A range is called *active* if it can conflict with log records 
//	than are encountered further during the scan. The resolver 
//	maintains a data structure of active ranges (called *range 
//	collection*) in the main memory. Every range in the collection
//	can be marked whether it screens some single-row records in the IUD 
//	log (cross-type duplicates).
//
//	When the collection cannot be extended any further, its contents 
//	are *flushed* to disk, in the following order:
//	(1) Subset Update/Delete statements are performed on the IUD
//		log, in order to resolve cross-type duplicates. If the single-row
//	    conflict resolution is enforced (and hence, the single-row records
//	    are scanned), the resolver can know which data is screened, and
//	    save I/O. Otherwise, the I/O is generally blind (i.e., there are
//	    statements that might update/delete no data). Therefore, the cost
//	    of a massive scan is traded off for the cost of blind I/O, which
//	    was shown to be less expensive. 
//
//	(2) The range analysis is performed, and its results 
//		are written to the range log. The algorithm maintains 
//		the range log incrementally, i.e., ranges that are already 
//		there (since the previous DE) and should not be re-written 
//		(due to new conflicts) remain intact, therefore saving I/O.
//	
//  The range collection holds *pointers* to the actual range boundary records. 
//	The records themselves are stored in a separate list. They are deleted once
//	the range analysis is performed, and the data is flushed to the range log.
//	
//	The range resolver allows to complete the DE phase (and commit the txn)
//	only on the range boundary. Thus, the commit can be done either when
//	the range collection is empty, or immediately after the flush.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS 
CRUDupElimRangeResolver : public CRUDupElimResolver {

private:
	typedef CRUDupElimResolver inherited;

public:
	CRUDupElimRangeResolver(
		const CRUDupElimGlobals &globals,
		CRUSQLDynamicStatementContainer &ctrlStmtContainer
	);

	virtual ~CRUDupElimRangeResolver() {}

	// Implementation of pure virtuals
public:
	//------------------------------------//
	//	Accessors
	//------------------------------------//
	BOOL IsRangeCollectionEmpty() const
	{
		return (0 == rangeCollection_.GetSize());
	}

#ifdef _DEBUG
	virtual void DumpPerformanceStatistics(CDSString &to) const;
#endif


public:
	//------------------------------------//
	//	Mutators
	//------------------------------------//
	virtual void Reset();

	//-- Resolve the next duplicate conflict (including the write to disk).
	virtual void Resolve(CRUDupElimLogScanner &scanner);

	// Prepare all the SQL statements in the container (together)
	virtual void PrepareSQL();

	// IPC pack/unpack
	virtual void LoadReply(CUOFsIpcMessageTranslator &translator);
	virtual void StoreReply(CUOFsIpcMessageTranslator &translator);



	//---------------------------------------//
	//	PRIVATE AREA
	//---------------------------------------//
private:
	//  Resolve() callees

	// Store the pointer to the new range record
	void InsertRangeBoundary(CRUIUDLogRecord *pRec);

	// Write all the ranges in the collection to the disk,
	// with conflicts resolved + release the memory.
	void Flush(); 

	// Flush() callees
	void FlushCrossTypeDuplicatesToIUDLog();
	void FlushRangesToRngLog();

	enum RequestType {

		INSERT_POST_IMAGE_RNGLOG,
		DELETE_PREV_IMAGE_RNGLOG,

		DELETE_DUPLICATE_IUDLOG,
		UPDATE_DUPLICATE_IUDLOG,
	};

	void TraverseRngCollectionAndPerformRqst(
		RequestType req,
		CRURangeCollectionIterator::IterDirection dir
	);

	// What can we say about whether the range is in the range log
	enum RangeMappingStatus {

		IS_IN_RNG_LOG,
		MAYBE_IN_RNG_LOG
	};

	RangeMappingStatus GetRangeMappingStatus(TInt32 epoch);

private:
	// Low-level I/O

	// Insert a single record to the range log
	void ExecuteRngLogInsert(const CRURange &rng);

	// Delete all the fragments of the range from the range log
	void ExecuteRngLogDelete(const CRURange &rng);
	
	// Delete the cross-type duplicates from the IUD log
	void ExecuteIUDLogDelete(const CRURange &rng);
	BOOL CanAvoidIUDLogDelete(const CRURange &rng);
	// Always ignore these records by setting the 
	// ignore mark to infinite
	void ExecuteIUDLogAlwaysIgnore(const CRURange &rng);

	// Mark the cross-type duplicates in the IUD log
	void ExecuteIUDLogUpdate(const CRURange &rng);
	BOOL CanAvoidIUDLogUpdate(const CRURange &rng);

private:
	// The core data structute
	CRURangeCollection rangeCollection_;

	// The actual storage of the range records from the IUD log
	CRUIUDLogRecordList rangeBoundariesList_;

	// The cached globals
	TInt32 lastDEEpoch_;
	TInt32 endEpoch_;
	BOOL isSingleRowResolv_;
	BOOL isSkipCrossTypeResoultion_;
	BOOL wasPrevDEInvocationCompleted_;

	// Statistics for the number of performed IUD statements
	Lng32 numRngLogInsert_;
	Lng32 numRngLogDelete_;
	Lng32 numIUDLogDelete_;
	Lng32 numIUDLogUpdate_;
};

#endif
