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
* File:         RuDupElimSingleRowResolv.cpp
* Description:  Implementation of class CRUDupElimSingleRowResolver
*				
*
* Created:      07/02/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuDupElimSingleRowResolv.h"

#include "RuException.h"
#include "RuDupElimLogScanner.h"
#include "RuDupElimLogRecord.h"
#include "RuDupElimGlobals.h"

#include "uofsIpcMessageTranslator.h"

//--------------------------------------------------------------------------//
//	Constructor 
//--------------------------------------------------------------------------//

CRUDupElimSingleRowResolver::
CRUDupElimSingleRowResolver(const CRUDupElimGlobals &globals, 
							CRUSQLDynamicStatementContainer &ctrlStmtContainer) :
	inherited(
		globals, 
		ctrlStmtContainer,
		CRUDupElimConst::NUM_SINGLE_RESOLV_STMTS
	),
	// Automaton status
	isPureUpdateChain_(TRUE),
	isUpdateRecInChain_(FALSE),
	// Common Update bitmap
	updateBitmap_(globals.GetUpdateBmpSize()),
	// Statistics
	numDeleteRecord_(0),
	numUpdateIgnMark_(0),
	numUpdateBitmap_(0),
	numUpdateOpType_(0)
{}

//--------------------------------------------------------------------------//
//	CRUDupElimSingleRowResolver::Reset()
//--------------------------------------------------------------------------//

void CRUDupElimSingleRowResolver::Reset()
{
	isPureUpdateChain_  = TRUE;
	isUpdateRecInChain_ = FALSE;

	// Set the bitmap to zeroes
	updateBitmap_.Reset();
}

//--------------------------------------------------------------------------//
//	CRUDupElimSingleRowResolver::PrepareSQL()
//
//	Prepare the IUD statements to be performed on the IUD log.
//
//	In order to force the optimizer to employ MDAM in the subset Update 
//	statements, execute the CONTROL QUERY SHAPE statement before the 
//	compilation, and execute the *reverse* control statement after it.
//	
//--------------------------------------------------------------------------//

void CRUDupElimSingleRowResolver::PrepareSQL()
{
	PrepareStatement(CRUDupElimConst::IUD_LOG_SINGLE_DELETE);
	PrepareStatement(CRUDupElimConst::IUD_LOG_SINGLE_UPDATE_IGNORE);

	ExecuteCQSForceMDAM(CRUDupElimResolver::IUD_LOG);
	
	PrepareStatement(CRUDupElimConst::IUD_LOG_UPDATE_BITMAP);
	PrepareStatement(CRUDupElimConst::IUD_LOG_UPDATE_OPTYPE);
	
	ExecuteCQSOff();
}

//--------------------------------------------------------------------------//
//	CRUDupElimSingleRowResolver::Resolve()
//
//	The main conflict resolution handler.
//
//	The class tracks the *duplicate chain* context (a duplicate	chain 
//	is a sequence of log records that reflect the operations applied 
//	to the same clustering key value, in the correct temporal order). 
//	The resolution actions are of two types:
//	(1) In the middle of the chain (handling non-trailing Inserts and
//		not-pivot Deletes, see the design document).
//	(2) At the end of the chain (handling the update bitmaps).
//	
//--------------------------------------------------------------------------//

void CRUDupElimSingleRowResolver::Resolve(CRUDupElimLogScanner &scanner)
{
	// Re-initialize the flag
	SetCanCompletePhase(FALSE);	

	CRUIUDLogRecord *pCurrentRec = scanner.GetCurrentRecord();
	CRUIUDLogRecord *pPrevRec = scanner.GetPrevRecord();

	if (NULL == pPrevRec    && 
	    NULL == pCurrentRec &&
	    TRUE == scanner.IsEntireDeltaScanned() )
	{
	  // This chain was empty.
	  // Nothing to do here.
	  return;
	}

	if (TRUE == scanner.IsEntireDeltaScanned())
	{
		RUASSERT(NULL != pPrevRec);
		EndChain(pPrevRec);
		return;
	}

	RUASSERT(NULL != pCurrentRec);
	if (FALSE == pCurrentRec->IsSingleRowOp() )
	{
		return;
	}

	// Resolve the single-row conflicts ...
	if (NULL != pPrevRec)
	{
		if (TRUE == pCurrentRec->IsClusteringKeyEqualTo(*pPrevRec))
		{
			// Take care of non-trailing Inserts
			if (TRUE == pPrevRec->IsInsert())
			{
				ResolveInsert(pPrevRec);
			}

			// Take care of non-pivot Deletes
			if (TRUE == pCurrentRec->IsDelete())
			{
				ResolveDelete(pCurrentRec, pPrevRec);
			}
		}
		else
		{
			EndChain(pPrevRec);
			
			// Allow the commit now
			SetCanCompletePhase(TRUE);	
		}
	}

	// The record is also a part of UPDATE operation, monitor 
	// its contribution to the chain's common @UPDATE_BITMAP.
	MonitorUpdate(pCurrentRec);
}

#ifdef _DEBUG
//------------------------------------------------------------//
//	CRUDupElimSingleRowResolver::DumpPerformanceStatistics()
//
//	Dump the number of times each IUD statement was performed.
//
//------------------------------------------------------------//

void CRUDupElimSingleRowResolver::
DumpPerformanceStatistics(CDSString &to) const
{
	CDSString msg;
	
	msg = "IUD log single-row delete";
	DumpStmtStatistics(to, msg, numDeleteRecord_);
	msg = "IUD log single-row update @IGNORE";
	DumpStmtStatistics(to, msg, numUpdateIgnMark_);
	msg = "IUD log update @UPDATE_BITMAP";
	DumpStmtStatistics(to, msg, numUpdateBitmap_);
	msg = "IUD log update @OPTYPE";
	DumpStmtStatistics(to, msg, numUpdateOpType_);
}
#endif

//--------------------------------------------------------------------------//
//	PRIVATE AREA	
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimSingleRowResolver::LoadReply()
//--------------------------------------------------------------------------//

void CRUDupElimSingleRowResolver::
LoadReply(CUOFsIpcMessageTranslator &translator)
{
	translator.ReadBlock(&numDeleteRecord_, sizeof(Lng32));
	translator.ReadBlock(&numUpdateIgnMark_, sizeof(Lng32));
	translator.ReadBlock(&numUpdateBitmap_, sizeof(Lng32));
	translator.ReadBlock(&numUpdateOpType_, sizeof(Lng32));
}

//--------------------------------------------------------------------------//
//	CRUDupElimSingleRowResolver::StoreReply()
//--------------------------------------------------------------------------//

void CRUDupElimSingleRowResolver::
StoreReply(CUOFsIpcMessageTranslator &translator)
{
	translator.WriteBlock(&numDeleteRecord_, sizeof(Lng32));
	translator.WriteBlock(&numUpdateIgnMark_, sizeof(Lng32));
	translator.WriteBlock(&numUpdateBitmap_, sizeof(Lng32));
	translator.WriteBlock(&numUpdateOpType_, sizeof(Lng32));
}

//--------------------------------------------------------------------------//
//	DUPLICATE ELIMINATION CONFLICTS RESOLUTION
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimSingleRowResolver::ResolveInsert()
//
//	The INSERT record that is not a trailing one it its chain
//	must be deleted (unless the Range resolver has already 
//	decided so). We can conclude whether the Insert is trailing
//	not before than we read the next record in the same chain
//	(i.e., the current record).
//
//	NOTE. This method is called only if the CURRENT record is 
//	a single-row record. Hence, we are checking for the case:
//	
//	(1) Insert(CK)	<-- prev
//	(2) Delete(CK)	<-- current
//
//	The following case cannot happen if the range is 
//	logged automatically (an Insert immediately before
//	a range with a boundary on the same CK):
//
//	(1) Insert(CK)
//	(2) {Begin/End}Range(CK) <-- prev
//	(3) Delete(CK)           <-- current
//
//	Therefore, the upper case is the only relevant one.
//
//--------------------------------------------------------------------------//

void CRUDupElimSingleRowResolver::
ResolveInsert(CRUIUDLogRecord *pPrevRec)
{
	if (TRUE == pPrevRec->WillRangeResolvDeleteMe())
	{
		// Nothing to do. The range resolver will perform the deletion.
		return;	
	}
	
	ExecuteDeleteRecord(pPrevRec);	
}

//--------------------------------------------------------------------------//
//	CRUDupElimSingleRowResolver::ResolveDelete()
//
//	Process the single-row DELETE record.
//
//	(1) If this is not a the first DELETE in the duplicate chain
//		in this record's epoch - delete it.
//
//	(2) Otherwise, the DELETE (we call it pivot) should receive 
//	    an ignore mark equal to the epoch of the preceding 
//		INSERT/DELETE record in the chain, unless the Range 
//		resolver has assigned it a greater value.
//
//	This method considers the following case:
//	
//	(1) ???(CK)     <-- prev (optype does not matter)
//	(2) Delete(CK)  <-- current
//
//	The duplicate resolution rules are correct both in the 
//	case when the previous record is a {Begin/End}Range and
//	in the case when it is an Insert. In the first case, the
//	Range resolver has already marked its decisions on the
//	current record.
//
//--------------------------------------------------------------------------//

void CRUDupElimSingleRowResolver::
ResolveDelete(CRUIUDLogRecord *pCurrentRec, CRUIUDLogRecord *pPrevRec)
{
	if (TRUE == pCurrentRec->WillRangeResolvDeleteMe())
	{
		// Nothing to do. The range resolver will perform the deletion.
		return;	
	}

	TInt32 prevEp = pPrevRec->GetEpoch();

	if (pCurrentRec->GetEpoch() == prevEp)
	{
		// Not a pivot
		ExecuteDeleteRecord(pCurrentRec);
		return;
	}

	if (prevEp > pCurrentRec->GetIgnoreMark())
	{
		pCurrentRec->SetIgnoreMark(prevEp);
		ExecuteUpdateIgnMark(pCurrentRec);
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimSingleRowResolver::MonitorUpdate()
//	
//	Process a single-row INSERT or DELETE record which was logged 
//	as a part of an Update operation (i.e., the @OPERATION_TYPE column 
//	has the IS_PART_OF_UPDATE bit on).
//
//	Single-row vs single-row duplicate elimination (which is actually
//	applied by the single-row resolver) can cause that the INTERNAL REFRESH
//	folliwing it will apply only one record in the Update (Delete + Insert) 
//	pair in the IUD log. For example, consider the chain D1, I2, D3, I4 
//	where D1 and I2 are part of update #1, D3 and I4 are part of update #2.
//	After DE, INTERNAL REFRESH will apply only D1 and I4. If D1 and I4 have
//	different bitmaps, the result of INTERNAL REFRESH can be incorrect.
//
//	Note that these problems cannot appear as part of the cross-type DE
//	(range vs single-row) that is performed by the range resolver, because
//	in cross-type DE the Update records "survive" or "die" in pairs (i.e.,
//	INTERNAL REFRESH applies either both or none of them.
//
//	In order to prevent INTERNAL REFRESH from confusion, all the DE "survivors"
//	must either inherit the superposition of the update bitmaps in the chain, 
//	or must be unmarked as a part of update. This requires monitoring of the 
//	common bitmap while scanning the chain, and, potentially a subset Update 
//	(applied to the whole chain). 
//
//	The method monitors the following statistics for the whole chain:
//	(1) Is there at least one Update record in the chain
//		that was not deleted by DE?
//	(2) Are all the records in the chain a part of update
//		(we call this chain a *pure update* chain).
//
//	If the first condition is false at the end of chain, there is
//	no need to address the issues that are specific to update bitmaps.	
//
//	As long as the second condition is true, the algorithm keeps maintaining 
//	the common update bitmap for the whole duplicate chain, which is 
//	a superposition of individual records' bitmaps. If the chain is not pure 
//	update, the resolver will unset the IS_PART_OF_UPDATE bit in all the 
//	records when the chain is finished (and hence, the bitmaps
//	will turn meaningless).
//
//--------------------------------------------------------------------------//

void CRUDupElimSingleRowResolver::
MonitorUpdate(CRUIUDLogRecord *pCurrentRec)
{
	if (FALSE == pCurrentRec->IsPartOfUpdate())
	{
		isPureUpdateChain_ = FALSE;
		return;
	}

	// Only if some Update record in chain will survive,
	// it makes sense to perform a "collective" change.
	// Hence, count only the non-deleted records.
	if (FALSE == pCurrentRec->WillRangeResolvDeleteMe())
	{
		isUpdateRecInChain_ = TRUE;
	}

	// Keep maintaining the bitmap only inside a pure update chain
	if (FALSE == isPureUpdateChain_)
	{
		return;
	}

	const CRUUpdateBitmap *pBmp = pCurrentRec->GetUpdateBitmap();

	if (TRUE == updateBitmap_.IsNull())
	{
		// This is the first update in this chain.
		// It *initializes* the bitmap, but doesn't *change* it. 
		updateBitmap_ = *pBmp;
	}
	else
	{
		updateBitmap_ |= *pBmp;
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimSingleRowResolver::EndChain()
//
//  (1) Apply the changes to all the Update records in the chain 
//		(including the write to disk), if required
//  (2) Reset all the automaton's state variables.
//
//--------------------------------------------------------------------------//

void CRUDupElimSingleRowResolver::EndChain(CRUIUDLogRecord *pLastRecInChain)
{
	RUASSERT(NULL != pLastRecInChain);

	if (TRUE == isUpdateRecInChain_)
	{
		// Perform a uniform change 
		// to all the Update records in the chain
		HandleUpdateBitmapAtEndOfChain(pLastRecInChain);
	}

	Reset();
}

//--------------------------------------------------------------------------//
//	CRUDupElimSingleRowResolver::HandleUpdateBitmapAtEndOfChain()
//
//	EndChain() callee.
//
//	When the duplicate chain is finished, *and* there was at
//	least one Update record in the chain that was not deleted
//	by DE:
//	(1) If the whole chain was update-only:
//		- If *not* all the bitmaps in the chain are equal -
//		  update all of them to their superposition (OR).
//	(2) Otherwise, unset the IS_PART_OF_UPDATE bit in the 
//		@OPERATION_TYPE column of all the Update records in
//		the chain (then, INTERNAL REFRESH will not treat these
//		records as updates).
//
//--------------------------------------------------------------------------//

void CRUDupElimSingleRowResolver::
HandleUpdateBitmapAtEndOfChain(CRUIUDLogRecord *pLastRecInChain)
{
	if (TRUE == isPureUpdateChain_)
	{
		if (TRUE == updateBitmap_.WasChanged())
		{
			ExecuteUpdateBitmapInChain(pLastRecInChain);
		}
	}
	else
	{
		ExecuteUpdateOpTypeInChain(pLastRecInChain);
	}
}

//--------------------------------------------------------------------------//
//	DUPLICATE ELIMINATION DECISIONS' EXECUTION
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimSingleRowResolver::ExecuteDeleteRecord()
//
//	DELETE FROM <T-IUD-log>
//	WHERE 
//	@EPOCH=?
//	AND
//	(<T-CK-col1 = ?> AND ... <T-CK-colN = ?>)
//	AND
//	@TS=?
//
//--------------------------------------------------------------------------//

void CRUDupElimSingleRowResolver::
ExecuteDeleteRecord(CRUIUDLogRecord *pRec)
{
	CDMPreparedStatement *pStmt	= 
		GetPreparedStatement(CRUDupElimConst::IUD_LOG_SINGLE_DELETE);
	RUASSERT (NULL != pStmt);

	// Search predicate
	pStmt->SetInt(1, pRec->GetEpoch());
	pRec->CopyCKTupleValuesToParams(*pStmt,2);

	Int32 syskeyPos = 2 + pRec->GetCKLength(); 
	pStmt->SetLargeInt(syskeyPos, pRec->GetSyskey());

	pStmt->ExecuteUpdate();
	// RUASSERT(1 == pStmt->GetRowsAffected());
	pStmt->Close();

	numDeleteRecord_++;
}

//--------------------------------------------------------------------------//
//	CRUDupElimSingleRowResolver::ExecuteUpdateIgnMark()
//
//	UPDATE <T-IUD-log>
//	SET @IGNORE=?
//	WHERE 
//	@EPOCH=?
//	AND
//	(<T-CK-col1 = ?> AND ... <T-CK-colN = ?>)
//	AND
//	@TS=?
//
//--------------------------------------------------------------------------//

void CRUDupElimSingleRowResolver::
ExecuteUpdateIgnMark(CRUIUDLogRecord *pRec)
{
	CDMPreparedStatement *pStmt	= 
		GetPreparedStatement(CRUDupElimConst::IUD_LOG_SINGLE_UPDATE_IGNORE);
	RUASSERT (NULL != pStmt);

	// The new value
	pStmt->SetInt(1, pRec->GetIgnoreMark());

	// Search predicate
	pStmt->SetInt(2, pRec->GetEpoch());
	pRec->CopyCKTupleValuesToParams(*pStmt, 3);

	Int32 syskeyPos = 3 + pRec->GetCKLength();
	pStmt->SetLargeInt(syskeyPos, pRec->GetSyskey());

	pStmt->ExecuteUpdate();
	// RUASSERT(1 == pStmt->GetRowsAffected());
	pStmt->Close();

	numUpdateIgnMark_++;
}

//--------------------------------------------------------------------------//
//	CRUDupElimSingleRowResolver::ExecuteUpdateBitmapInChain()
//
//	UPDATE <T-IUD-log>
//	SET @UPDATE_BITMAP = ?
//	WHERE
//	(@EPOCH BETWEEN begin-epoch and end-epoch)
//	AND
//	(<T-CK-col1 = ?> AND ... <T-CK-colN = ?>)
//	AND
//	(@OPERATION_TYPE = 2 OR @OPERATION_TYPE = 3)
//
//--------------------------------------------------------------------------//

void CRUDupElimSingleRowResolver::
ExecuteUpdateBitmapInChain(CRUIUDLogRecord *pLastRecInChain)
{
	CDMPreparedStatement *pStmt	= 
		GetPreparedStatement(CRUDupElimConst::IUD_LOG_UPDATE_BITMAP);
	RUASSERT (NULL != pStmt);

	// The common bitmap value	
	pStmt->SetString(1, updateBitmap_.GetBuffer(), updateBitmap_.GetSize());
	// Search predicate
	pLastRecInChain->CopyCKTupleValuesToParams(*pStmt, 2);

	pStmt->ExecuteUpdate();
	// RUASSERT(0 != pStmt->GetRowsAffected());

	pStmt->Close();

	numUpdateBitmap_++;
}

//--------------------------------------------------------------------------//
//	CRUDupElimSingleRowResolver::ExecuteUpdateOpTypeInChain()
//
//	UPDATE <T-IUD-log>
//	SET @OPERATION_TYPE = @OPERATION_TYPE - 2
//	WHERE
//	(@EPOCH BETWEEN begin-epoch and end-epoch)
//	AND
//	(<T-CK-col1 = ?> AND ... <T-CK-colN = ?>)
//	AND
//	(@OPERATION_TYPE = 2 OR @OPERATION_TYPE = 3)
//
//--------------------------------------------------------------------------//

void CRUDupElimSingleRowResolver::
ExecuteUpdateOpTypeInChain(CRUIUDLogRecord *pLastRecInChain)
{
	CDMPreparedStatement *pStmt	= 
		GetPreparedStatement(CRUDupElimConst::IUD_LOG_UPDATE_OPTYPE);
	RUASSERT (NULL != pStmt);

	// Search predicate
	pLastRecInChain->CopyCKTupleValuesToParams(*pStmt, 1);

	pStmt->ExecuteUpdate();
	// RUASSERT(0 != pStmt->GetRowsAffected());
	pStmt->Close();

	numUpdateOpType_++;
}
