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
* File:         RuDupElimRangeResolv.cpp
* Description:  Implementation of class CRUDupElimRangeResolver 
*
*
* Created:      07/06/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "RuOptions.h"
#include "RuGlobals.h"
#include "RuDupElimRangeResolv.h"
#include "uofsIpcMessageTranslator.h"

#include "RuDupElimLogScanner.h"
#include "RuDupElimGlobals.h"
#include "ComSmallDefs.h"

//--------------------------------------------------------------------------//
//	Constructor and destructor
//--------------------------------------------------------------------------//

CRUDupElimRangeResolver::
CRUDupElimRangeResolver(const CRUDupElimGlobals &globals, 
						CRUSQLDynamicStatementContainer &ctrlStmtContainer) :
	inherited(
		globals, 
		ctrlStmtContainer, 
		CRUDupElimConst::NUM_RNG_RESOLV_STMTS
	),
	isSkipCrossTypeResoultion_(globals.IsSkipCrossTypeResoultion()),
	isSingleRowResolv_(globals.IsSingleRowResolv()),
	lastDEEpoch_(globals.GetLastDEEpoch()),
	endEpoch_(globals.GetEndEpoch()),
	wasPrevDEInvocationCompleted_(globals.WasPrevDEInvocationCompleted()),
	// The main data structures
	rangeCollection_((CDDObject::ERangeLogType) globals.GetRangeLogType()),
	rangeBoundariesList_(eItemsAreOwned),
	// Statistics
	numRngLogInsert_(0),
	numRngLogDelete_(0),
	numIUDLogDelete_(0),
	numIUDLogUpdate_(0)
{}

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::Reset()
//--------------------------------------------------------------------------//

void CRUDupElimRangeResolver::Reset()
{
	// Free the data structure of pointers to the range boundaries 
	rangeCollection_.Reset();

	// Free the range boundaries themselves
	rangeBoundariesList_.RemoveAll();
}

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::PrepareSQL()
//
//	Prepare the IUD statements to be performed on the IUD and the range logs.
//
//	In order to force the optimizer to employ MDAM in the subset 
//	Delete/Update statements, execute the CONTROL QUERY SHAPE
//	statement before the compilation, and execute the *reverse* control
//	statement after it.
//	
//--------------------------------------------------------------------------//

void CRUDupElimRangeResolver::PrepareSQL()
{
	PrepareStatement(CRUDupElimConst::RANGE_LOG_INSERT);

	ExecuteCQSForceMDAM(CRUDupElimResolver::RNG_LOG);
	PrepareStatement(CRUDupElimConst::RANGE_LOG_DELETE);
	ExecuteCQSOff();

	if (FALSE == GetDupElimGlobals().IsSkipCrossTypeResoultion())
	{
	  CRUOptions &options = CRUGlobals::GetInstance()->GetOptions();
	  if (NULL == options.FindDebugOption(CRUGlobals::DE_USE_IGNORE_ALWAYS, ""))
	  {
		PrepareStatement(CRUDupElimConst::IUD_LOG_SUBSET_DELETE);
	  }
	  else
	  {
		PrepareStatement(CRUDupElimConst::IUD_LOG_SUBSET_UPDATE_ALWAYS_IGNORE);
	  }

	  PrepareStatement(CRUDupElimConst::IUD_LOG_SUBSET_UPDATE_IGNORE);
	}
	
	
}

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::Resolve()
//
//	If there further extension of the range collection is impossible, 
//	the conflicts between the ranges must be resolved (range analysis), 
//	and the resulting ranges must be written to the disk.
//
//	The collection cannot be extended anymore in two cases:
//	(1) The clustering key of the current IUD log record is not
//		covered by any range in the collection.
//	(2) The delta is scanned completely.
//
//	If the current record is a Range record, it will be added
//	to the collection (possibly after the flush). 

//	If it is a single-row record, the resolver performs a cross-type DE: 
//	if the record is screened by some range, the DE decision will be marked 
//	both on the record and the screening range. This will save work to the 
//	single-row resolver(if there is one), and will make the range resolver's 
//	IUD operations non-blind.
//
//	The resolver allows to complete the DE phase only on the range boundary
//	(i.e., if *before* processing the new record the range collection became
//	empty). The boundary case is when a new BR record is inserted into the 
//	range collection immediately *after* the flush, but the mechanism  
//	decides to commit. Then, the range collection will be reset, and the 
//	next phase will resume from this last CK value).
//
//--------------------------------------------------------------------------//

void CRUDupElimRangeResolver::Resolve(CRUDupElimLogScanner &scanner)
{
	SetCanCompletePhase(FALSE);	// Re-initialize the flag

	if (TRUE == scanner.IsEntireDeltaScanned())
	{
		// End of input. There will be no other chance
		// to perform the duplicate analysis.
		Flush();
		return;
	}

	CRUIUDLogRecord *pCurrentRec = scanner.GetCurrentRecord();
	RUASSERT(NULL != pCurrentRec);

	BOOL isCovered = rangeCollection_.IsClusteringKeyCovered(pCurrentRec);
	if (FALSE == isCovered)
	{
		if (FALSE == IsRangeCollectionEmpty())
		{
			// We are about to flush a non-empty collection.
			// Allow the commit (although a new range might be started next!)
			SetCanCompletePhase(TRUE);
		}

		// The range collection cannot be extended anymore,
		// time to perform the duplicate analysis 
		// and write the data to the disk.
		Flush();
	}

	// Process the new record ...
	if (TRUE == pCurrentRec->IsSingleRowOp())
	{
		if (TRUE == isCovered)
		{
			rangeCollection_.PerformCrossTypeDE(pCurrentRec);
		}
	}
	else
	{
		InsertRangeBoundary(pCurrentRec);
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::DumpPerformanceStatistics()
//
//	Dump the number of times each IUD statement was performed.
//
//--------------------------------------------------------------------------//

#ifdef _DEBUG
void CRUDupElimRangeResolver::
DumpPerformanceStatistics(CDSString &to) const
{
	CDSString msg;
	
	msg = "Range log insert";
	DumpStmtStatistics(to, msg, numRngLogInsert_);
	msg = "Range log delete";
	DumpStmtStatistics(to, msg, numRngLogDelete_);
	msg = "IUD log subset delete";
	DumpStmtStatistics(to, msg, numIUDLogDelete_);
	msg = "IUD log subset update @IGNORE";
	DumpStmtStatistics(to, msg, numIUDLogUpdate_);
}
#endif


//--------------------------------------------------------------------------//
//		PRIVATE AREA
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::LoadReply()
//--------------------------------------------------------------------------//

void CRUDupElimRangeResolver::
LoadReply(CUOFsIpcMessageTranslator &translator)
{
	translator.ReadBlock(&numRngLogDelete_, sizeof(Lng32));
	translator.ReadBlock(&numRngLogInsert_, sizeof(Lng32));
	translator.ReadBlock(&numIUDLogDelete_, sizeof(Lng32));
	translator.ReadBlock(&numIUDLogUpdate_, sizeof(Lng32));
}

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::StoreReply()
//--------------------------------------------------------------------------//

void CRUDupElimRangeResolver::
StoreReply(CUOFsIpcMessageTranslator &translator)
{
	translator.WriteBlock(&numRngLogDelete_, sizeof(Lng32));
	translator.WriteBlock(&numRngLogInsert_, sizeof(Lng32));
	translator.WriteBlock(&numIUDLogDelete_, sizeof(Lng32));
	translator.WriteBlock(&numIUDLogUpdate_, sizeof(Lng32));
}

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::InsertRangeBoundary()
//
//	The current record is a range boundary record.
//	Therefore, it must be inserted into the ranges' data structure.	
//
//	However, we cannot just store the pointer into the scanner's input buffer, 
//	because the data in it is overwritten cyclically. Therefore, we store a
//	*copy* of the record in the auxiliary list that will be later used 
//	for the garbage collection.
//	
//--------------------------------------------------------------------------//

void CRUDupElimRangeResolver::InsertRangeBoundary(CRUIUDLogRecord *pRec)
{
	CRUIUDLogRecord *pRecCopy = new CRUIUDLogRecord(*pRec);

	// Store the pointer for the futher garbage collection
	rangeBoundariesList_.AddTail(pRecCopy);

	// Update the ranges' data structure
	rangeCollection_.InsertRangeBoundary(pRecCopy);
}

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::Flush()
//	
//	Translate the Duplicate Elimination decisions to disk I/O:
//	(1) Subset Update/Delete statements are performed on the IUD
//		log, in order to resolve cross-type duplicates.
//	(2) The range analysis is performed, and all the resulting
//		ranges are written to the range log.
//
//	After the flush is performed, memory (the range collection 
//	+ the range records' storage) can be released. Also, the flag 
//	is raised to signal the task executor that the flush is performed, 
//	and the transaction can be now committed.
//
//--------------------------------------------------------------------------//

void CRUDupElimRangeResolver::Flush()
{
	if (0 == rangeCollection_.GetSize())
	{
		return;
	}

	rangeCollection_.PrepareForFlush();

	// Step 1 ...
	if (FALSE == isSkipCrossTypeResoultion_)
	{
		FlushCrossTypeDuplicatesToIUDLog();
	}

	// Step 2 ...
	FlushRangesToRngLog();

	// Cleanup the data structures and free the memory
	Reset();
}

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::FlushCrossTypeDuplicatesToIUDLog()
//
//	Traverse the range collection to eliminate the single-row records 
//	which are screened by the ranges in the collection. The elimination
//	is expressed in terms of subset Update/Delete operations applied to
//	the IUD log:
//	(1) The screened records in the same epoch as the range will be deleted.
//	(2) For those in the greater epochs, the @IGNORE mark will be set 
//	    to the range's epoch.
//
//	The collection will be traversed twice: first in the direct order of epochs
//	to perform Delete, next in the *reverse* order of epochs, to perform Update.
//
//	The reverse order of epochs in Update is a performance optimization. Suppose 
//	two overlapping ranges and two single-row records form the following picture:
//
//	Epoch 1001	-----------
//
//	Epoch 1002    ^		(must receive @Ignore = 1001)
//	              |
//	Epoch 1003         --------
//
//  Epoch 1004           ^      (must receive @Ignore = 1003)
//	                     |
//
//  During the construction, the resolver has left the needToUpdate flag on both 
//	ranges. Since the single-row record in Epoch 1004 is screened by both ranges, 
//	we want	to prevent the update of its @Ignore mark first to 1001, 
//	and next to 1003.
//
//--------------------------------------------------------------------------//

void CRUDupElimRangeResolver::FlushCrossTypeDuplicatesToIUDLog()
{
	RUASSERT(FALSE == isSkipCrossTypeResoultion_);

	TraverseRngCollectionAndPerformRqst(
		DELETE_DUPLICATE_IUDLOG,
		CRURangeCollectionIterator::DIR_FORWARD
	);

	TraverseRngCollectionAndPerformRqst(
		UPDATE_DUPLICATE_IUDLOG,
		CRURangeCollectionIterator::DIR_BACKWARD
	);
}


//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::FlushRangesToRngLog()
//
//
//  1.If the range collection contains only a single range record and the 
//  range record is in the range log then it must be the same range record 
//  thus we can avoid deleting the range and rewriting it back. 
//  The following argument explain why this must be the same record.
//  Let's assume that the range record in the log, denoted saved-record, 
//  is different from the record in memory.The saved record 
//  must had conflicts with other range records in the past since 
//	the saved-record is different from the original range record.Moreover 
//  the saved-record was not the last range in its collection since the last record 
//  in every collection remains intact.
//  This makes a contradiction since our collection contains only one record 
//  and it must contain all records that are later than this range record including 
//  the above conflicting range records. 
//  In conclusion, the saved and in memory record must be the same.
//
//	2.If the epochs of some ranges in the collection are smaller than the last 
//	DE epoch, then fragments of ranges in these sets might be in the range log. 
//	The range analysis that involves some "brand new" ranges can produce 
//	a different picture of fragments. Hence, all the old fragments must be
//	deleted BEFORE the range analysis.
//
//--------------------------------------------------------------------------//

void CRUDupElimRangeResolver::FlushRangesToRngLog()
{
	// if the range collection contains only a single range record and the
	// range record is in the range log then do not perform IO
	if (IS_IN_RNG_LOG == GetRangeMappingStatus(rangeCollection_.GetMinEpoch()) 
	    && 1 == rangeCollection_.GetSize() )
	{
		return;
	}

	// Fragments of some ranges in the collection may be in the range log.
	// Conflicts may be encountered, delete the prev-image from disk.
	TraverseRngCollectionAndPerformRqst(
		DELETE_PREV_IMAGE_RNGLOG,
		CRURangeCollectionIterator::DIR_FORWARD
	);

	if (1 < rangeCollection_.GetSize())
	{
		// The collection contains multiple overlapping ranges. Hence, 
		// range analysis must be performed before flushing the collection 
		// to the range log.
		// Turn the ranges into the collection to disjoint fragment sets
		rangeCollection_.PerformRangeAnalysis();
	}
		
	// ... and write them to the range log!
	TraverseRngCollectionAndPerformRqst(
		INSERT_POST_IMAGE_RNGLOG,
		CRURangeCollectionIterator::DIR_FORWARD
	);
}

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::GetRangeMappingStatus()
//	
//	What can we say about whether the previous invocation(s) of DE have
//	succeeded to write this range to the range log?
//	
//	If the range's epoch is less or equal to T.LAST_DE_EPOCH, then
//	it is doubtlessly in the range log because LAST_DE_EPOCH is advanced
//	only when the DE completes successfully (and hence, all the ranges 
//	in the smaller or equal epochs are mapped to the range log).
//	Otherwise we cannot be sure that the range record is either in the 
//      range log or not since it maybe that the last DE has failed after 
//      commiting a phase and the range record was written to the log.
//
//--------------------------------------------------------------------------//

CRUDupElimRangeResolver::RangeMappingStatus 
CRUDupElimRangeResolver::GetRangeMappingStatus(TInt32 epoch)
{
	if (epoch <= lastDEEpoch_)
	{
		return IS_IN_RNG_LOG;
	}

	return MAYBE_IN_RNG_LOG;
}

//--------------------------------------------------------------------------//
//	DUPLICATE ELIMINATION DECISIONS' EXECUTION
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::TraverseRngCollectionAndPerformRqst()
//
//	Traverse the range collection and perform the request.
//
//--------------------------------------------------------------------------//

void CRUDupElimRangeResolver::
TraverseRngCollectionAndPerformRqst(RequestType req, 
									CRURangeCollectionIterator::IterDirection dir)
{
	CRURange *pRange;
	CRURangeCollectionIterator it(rangeCollection_, dir);

	for (; NULL != (pRange = it.GetCurrent()); it.Next())
	{
		CRURange &rng = *pRange;

		switch (req)
		{
		case INSERT_POST_IMAGE_RNGLOG:
			{
				ExecuteRngLogInsert(rng);
				break;
			}
		case DELETE_PREV_IMAGE_RNGLOG:
			{
				ExecuteRngLogDelete(rng);
				break;
			}
		case DELETE_DUPLICATE_IUDLOG:
			{
				CRUOptions &options = CRUGlobals::GetInstance()->GetOptions();
				if (FALSE == CanAvoidIUDLogDelete(rng))
				{
				  if (NULL == options.FindDebugOption(CRUGlobals::DE_USE_IGNORE_ALWAYS, ""))
				  {
				    ExecuteIUDLogDelete(rng);
				  }
				  else
				  {
				    // Instead of delete records , use ignore always method
				    ExecuteIUDLogAlwaysIgnore(rng);
				  }
				}
				break;
			}
		case UPDATE_DUPLICATE_IUDLOG:
			{
				if (FALSE == CanAvoidIUDLogUpdate(rng))
				{
					ExecuteIUDLogUpdate(rng);
				}
				break;
			}
		default: RUASSERT(FALSE);
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::ExecuteRngLogInsert()
//
//	Insert all the range's fragments into the range log.
//
//	INSERT INTO <T-range-log>
//	(@EPOCH, @RANGE_ID, @RANGE_TYPE, 
//	 <@BR_-columns>, 
//	 <@ER_-columns>)
//	VALUES (?, ..., ?,  -- control column values
//			?, ..., ?,  -- begin-range CK values
//			?, ..., ?)	-- end-range CK values
//
//--------------------------------------------------------------------------//

void CRUDupElimRangeResolver::ExecuteRngLogInsert(const CRURange &rng)
{
	TInt32 epoch = rng.GetEpoch();
	TInt64 rangeId = rng.GetRangeId();

	CDMPreparedStatement *pStmt = 
		GetPreparedStatement(CRUDupElimConst::RANGE_LOG_INSERT);

	const CRURangeFragmentList &fragList = rng.GetFragmentList();

	DSListPosition pos = fragList.GetHeadPosition();
	while (NULL != pos)
	{
		CRURangeFragment *pFrag = fragList.GetNext(pos);

		const CRUIUDLogRecord *pLBRec = pFrag->GetBRRecord();
		const CRUIUDLogRecord *pUBRec = pFrag->GetERRecord();
		
		pStmt->SetInt(1, epoch);		// @EPOCH
		pStmt->SetLargeInt(2, rangeId); // @RANGE_ID
		pStmt->SetInt(3, pFrag->GetType());	 // @RANGE_TYPE

		// The @BR_ values
		pLBRec->CopyCKTupleValuesToParams(*pStmt, 4);

		// The @ER_ values
		pUBRec->CopyCKTupleValuesToParams(*pStmt, 4+pLBRec->GetCKLength());

		pStmt->ExecuteUpdate();
		pStmt->Close();

		numRngLogInsert_++;
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::ExecuteRngLogDelete()
//
//	Delete all the previously existing fragments of this range
//	from the range log:
//
//	DELETE FROM <T-range-log>
//	WHERE 
//	@EPOCH = ? 
//	AND
//	@RANGE_ID = ?
//	AND
//  (@BR_col1, @BR_col2, ...) >= (?, ?, ..) DIRECTEDBY (ASC/DESC, ...)
//  --                           BR-boundary
//	AND
//  (@ER_col1, @ER_col2, ...) <= (?, ?, ..) DIRECTEDBY (...)
//  --                           ER-boundary
//	AND
//  (@ER_col1, @ER_col2, ...) >= (?, ?, ..) DIRECTEDBY (...)
//  --                           BR-boundary
//
//	Note that the number of rows deleted by this statement can eventually 
//	be zero, because the deletion is based on the picture of range collection 
//	built in the main memory *before* the range analysis. It can happen 
//	that there was no pre-image on disk that matches the search predicate 
//	(for example, because in the previous invocation of DE this in-memory range
//	was fully dominated	by a younger range, and hence was not logged).
//
//--------------------------------------------------------------------------//

void CRUDupElimRangeResolver::ExecuteRngLogDelete(const CRURange &rng)
{
	const CRUIUDLogRecord *pLBRec = rng.GetBRRecord();
	const CRUIUDLogRecord *pUBRec = rng.GetERRecord();
	Lng32 ckLen = pLBRec->GetCKLength();

	CDMPreparedStatement *pStmt = 
		GetPreparedStatement(CRUDupElimConst::RANGE_LOG_DELETE);

	pStmt->SetInt(1, rng.GetEpoch());	// @EPOCH
	pStmt->SetLargeInt(2, rng.GetRangeId()); // @RANGE_ID

	// The @BR_ values 
	pLBRec->CopyCKTupleValuesToParams(*pStmt, 3);

	// The @ER_ values - first clause
	pUBRec->CopyCKTupleValuesToParams(*pStmt, 3+ckLen);

	// The @ER_ values - second clause
	pLBRec->CopyCKTupleValuesToParams(*pStmt, 3+2*ckLen);

	pStmt->ExecuteUpdate();
	pStmt->Close();

	numRngLogDelete_++;
}

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::ExecuteIUDLogDelete()
//
//	Delete the screened records in the same epoch (from the IUD log):
//
//	DELETE FROM <T-IUD-log>
//	WHERE
//	@EPOCH = ?	-- my epoch
//	AND
//  (col1, col2, ...) >= (?, ?, ..) DIRECTEDBY (ASC/DESC, ...)
//	AND
//  (col1, col2, ...) <= (?, ?, ..) DIRECTEDBY (...)
//	AND
//	@TS > ?	-- my syskey
//
//--------------------------------------------------------------------------//

void CRUDupElimRangeResolver::ExecuteIUDLogDelete(const CRURange &rng)
{
	const CRUIUDLogRecord *pLBRec = rng.GetBRRecord();
	const CRUIUDLogRecord *pUBRec = rng.GetERRecord();

	TInt32 epoch = pLBRec->GetEpoch();
	Lng32 ckLen = pLBRec->GetCKLength();

	CDMPreparedStatement *pStmt = 
		GetPreparedStatement(CRUDupElimConst::IUD_LOG_SUBSET_DELETE);

	pStmt->SetInt(1, epoch); // @EPOCH

	// The lower CK bound
	pLBRec->CopyCKTupleValuesToParams(*pStmt, 2);

	// The upper CK bound
	pUBRec->CopyCKTupleValuesToParams(*pStmt, 2+ckLen);

	// The range's syskey (defined by the begin-range record)
	pStmt->SetLargeInt(2+2*ckLen, pLBRec->GetSyskey());

	pStmt->ExecuteUpdate();
	pStmt->Close();

	numIUDLogDelete_++;
}

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::ExecuteIUDLogAlwaysIgnore()
//
//  This function sets the ignore mark of records to infinite. 
//  Theses records are always ignored by the internal refresh and thus
//  they can be assumed as deleted records.
//
//  The records are actually deleted in the cleanup operation.
//
//  This function can be used when the update operation is much faster than
//  the delete operation. 
//
//  This option is can be triggerd by using debug option
//--------------------------------------------------------------------------//

void CRUDupElimRangeResolver::ExecuteIUDLogAlwaysIgnore(const CRURange &rng)
{
	const CRUIUDLogRecord *pLBRec = rng.GetBRRecord();
	const CRUIUDLogRecord *pUBRec = rng.GetERRecord();

	TInt32 epoch = pLBRec->GetEpoch();
	Lng32 ckLen = pLBRec->GetCKLength();

	CDMPreparedStatement *pStmt = 
		GetPreparedStatement(CRUDupElimConst::IUD_LOG_SUBSET_UPDATE_ALWAYS_IGNORE);

	TInt32 ignoreEpoch = MAX_COMSINT32;

	pStmt->SetInt(1, ignoreEpoch);	// @IGNORE = ?

	pStmt->SetInt(2, epoch); // @EPOCH = ?

	// The lower CK bound
	pLBRec->CopyCKTupleValuesToParams(*pStmt, 3);

	// The upper CK bound
	pUBRec->CopyCKTupleValuesToParams(*pStmt, 3+ckLen);

	// The range's syskey (defined by the begin-range record)
	pStmt->SetLargeInt(3+2*ckLen, pLBRec->GetSyskey());

	pStmt->ExecuteUpdate();

	pStmt->Close();

	numIUDLogUpdate_++;
}

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::CanAvoidIUDLogDelete()
//--------------------------------------------------------------------------//

BOOL CRUDupElimRangeResolver::CanAvoidIUDLogDelete(const CRURange &rng)
{
	if (TRUE == isSingleRowResolv_)
	{
		// We work non-blind. Use the flags maintained by the analysis.
		return (FALSE == rng.IsDeleteOfScreenedRecordsRequired());
	}
	else
	{
		return FALSE;
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::ExecuteIUDLogUpdate()
//
//	Update the @IGNORE mark of the screened records in the greater epochs 
//	(in the IUD log):
//
//	UPDATE <T-IUD-log>
//	SET @IGNORE=?	-- my epoch
//	WHERE 
//	@EPOCH = ?	-- my epoch
//	AND
//  (col1, col2, ...) >= (?, ?, ..) DIRECTEDBY (ASC/DESC, ...)
//	AND
//  (col1, col2, ...) <= (?, ?, ..) DIRECTEDBY (...)
//	AND
//	@IGNORE < ?	-- my epoch
//
//--------------------------------------------------------------------------//

void CRUDupElimRangeResolver::ExecuteIUDLogUpdate(const CRURange &rng)
{
	const CRUIUDLogRecord *pLBRec = rng.GetBRRecord();
	const CRUIUDLogRecord *pUBRec = rng.GetERRecord();

	TInt32 epoch = pLBRec->GetEpoch();
	Lng32 ckLen = pLBRec->GetCKLength();

	CDMPreparedStatement *pStmt = 
		GetPreparedStatement(CRUDupElimConst::IUD_LOG_SUBSET_UPDATE_IGNORE);

	for (TInt32 cEpoch = epoch + 1;cEpoch <= endEpoch_;cEpoch++)
	{
		pStmt->SetInt(1, epoch);		// @IGNORE = ?

		pStmt->SetInt(2, cEpoch);		// @EPOCH = ?

		// The lower CK bound
		pLBRec->CopyCKTupleValuesToParams(*pStmt, 3);

		// The upper CK bound
		pUBRec->CopyCKTupleValuesToParams(*pStmt, 3+ckLen);

		// @IGNORE < ?	
		pStmt->SetInt(3+2*ckLen, epoch);

		pStmt->ExecuteUpdate();
		pStmt->Close();
	}

	numIUDLogUpdate_++;
}

//--------------------------------------------------------------------------//
//	CRUDupElimRangeResolver::CanAvoidIUDLogUpdate()
//--------------------------------------------------------------------------//

BOOL CRUDupElimRangeResolver::CanAvoidIUDLogUpdate(const CRURange &rng)
{
	if (TRUE == isSingleRowResolv_)
	{
		// We work non-blind ...
		return (FALSE == rng.IsUpdateOfScreenedRecordsRequired());
	}
	else
	{
		if (endEpoch_ == rng.GetEpoch())
		{
			// The range belongs to the last scanned epoch.
			// The @IGNORE mark can be set only in the records 
			// that belong to greater epochs.
			return TRUE;
		}

		return FALSE;
	}
}
