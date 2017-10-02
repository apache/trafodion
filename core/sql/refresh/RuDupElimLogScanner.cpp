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
* File:         RuDupElimLogScanner.cpp
* Description:  Implementation of class CRUDupElimLogScanner
*				
*
* Created:      07/02/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuDupElimLogScanner.h"

#include "dmresultset.h"

#include "RuDeltaDef.h"
#include "RuDupElimGlobals.h"

//--------------------------------------------------------------------------//
//	Constructor and destructor
//--------------------------------------------------------------------------//

CRUDupElimLogScanner::
CRUDupElimLogScanner(const CRUDupElimGlobals &globals, 
					 CRUSQLDynamicStatementContainer &ctrlStmtContainer) :
	inherited(
		globals, 
		ctrlStmtContainer,
		CRUDupElimConst::NUM_QUERY_STMTS),

	// State variables
	tupleDesc_(globals.GetNumCKColumns()),
	ckStartColumn_(0),
	pCurrentStmt_(NULL),
	pResultSet_(NULL),
	inputBufIndex_(0),
	pCurrentRec_(NULL),
	pPrevRec_(NULL),
	pLowerBoundRec_(NULL),
	isEntireDeltaScanned_(FALSE),
	ckTag_(0),
	// Statistics
	statMap_(),
	empCheckVector_()
{
	for (Int32 i =0; i<IB_SIZE; i++)
	{
		inputBuf_[i] = NULL;
	}
}

CRUDupElimLogScanner::~CRUDupElimLogScanner()
{
	for (Int32 i=0; i<IB_SIZE; i++)
	{
		if (NULL != inputBuf_[i])
		{
			delete inputBuf_[i];
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimLogScanner::Reset()
//
//	Reset the state variables before the next phase.
//--------------------------------------------------------------------------//

void CRUDupElimLogScanner::Reset()
{
	RUASSERT(FALSE == isEntireDeltaScanned_);

	// Cache the clustering key value to start from ...
	pLowerBoundRec_ = pCurrentRec_;

	pCurrentRec_ = NULL;
	pPrevRec_ = NULL;

	inputBufIndex_ = 0;
	ckTag_ = 0;
}

//--------------------------------------------------------------------------//
//	CRUDupElimLogScanner::StartScan()
//
//	Start executing the query and allocate the resources
//	
//	SELECT <T_CK columns, control columns> FROM <T-IUD-log> 
//	WHERE 
//	(
//	@EPOCH = {+/-} begin_epoch 
//	{AND (col1, col2, ...) >= (?, ?, ..) DIRECTEDBY (ASC/DESC, ...)}
//	)
//	UNION ALL
//	SELECT <T_CK columns, control columns> FROM <T-IUD-log> 
//	WHERE 
//	(
//	@EPOCH = {+/-} begin_epoch+1
//	{AND (col1, col2, ...) >= (?, ?, ..) DIRECTEDBY (ASC/DESC, ...)}
//	)
//	...
//	UNION ALL
//	SELECT <T_CK columns, control columns> FROM <T-IUD-log> 
//	WHERE 
//	(
//	@EPOCH = {+/-} end_epoch 
//	{AND (col1, col2, ...) >= (?, ?, ..) DIRECTEDBY (ASC/DESC, ...)}
//	)
//
//	ORDER BY <T_CK columns>, @TS
//	FOR BROWSE ACCESS;
//
//--------------------------------------------------------------------------//

void CRUDupElimLogScanner::StartScan(Int32 phase)
{
	RUASSERT(NULL == pCurrentStmt_ && NULL == pResultSet_);

	if (0 == phase)
	{
		pCurrentStmt_ = GetPreparedStatement(CRUDupElimConst::PHASE_0_QUERY);
	}
	else
	{
		RUASSERT(phase > 0);

		// The same statement for each phase starting from 1
		pCurrentStmt_ = GetPreparedStatement(CRUDupElimConst::PHASE_1_QUERY);

		// Set the parameters for the lower bound ...
		CopyLowerBoundParams();
	}

	// Start the execution ...
	pCurrentStmt_->ExecuteQuery();
	pResultSet_ = pCurrentStmt_->GetResultSet();

	if (0 == phase)
	{
		// Allocate the buffer space and setup the output descriptors
		SetupScan();
	}
	else
	{
		// Nothing. Both queries have the same result set's structure.
		// Hence, we need not re-allocate the buffers.
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimLogScanner::Fetch()
//	
//	Retrieve the next row from the delta, if there is one,
//	and fetch its data into the next position in the cyclic buffer. 
//
//	Update the record's tags (for future use by the resolvers)
//	and the delta statistics.
//
//--------------------------------------------------------------------------//

void CRUDupElimLogScanner::Fetch()
{
	// Start the cyclic shift ...
	pPrevRec_ = pCurrentRec_;

	if (TRUE == pResultSet_->Next())
	{
		// End the cyclic shift ...
		pCurrentRec_ = inputBuf_[inputBufIndex_];
		// ... and retrieve the new data 
		pCurrentRec_->Build(*pResultSet_, ckStartColumn_);

		// Move the pointer inside the buffer
		inputBufIndex_ = (inputBufIndex_+1) % IB_SIZE;

		if (NULL == pPrevRec_
			||
			pCurrentRec_->GetCKTuple() != pPrevRec_->GetCKTuple())
		{
			// A new distinct CK value has been encountered
			ckTag_++;
		}
		pCurrentRec_->SetCKTag(ckTag_);

		UpdateStatistics();
	}
	else
	{
		// End of input reached !
		isEntireDeltaScanned_ = TRUE;

		pCurrentStmt_->Close();
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimLogScanner::EndScan()
//--------------------------------------------------------------------------//

void CRUDupElimLogScanner::EndScan()
{
	RUASSERT(
		NULL != pCurrentStmt_
		&&
		NULL != pResultSet_);

	pCurrentStmt_->Close();

	pCurrentStmt_ = NULL;
	pResultSet_ = NULL;
}

//--------------------------------------------------------------------------//
//	PRIVATE AREA
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimLogScanner::SetupScan()
//	
//	When the *first* scan is started, initialize the tuple descriptors
//	and allocate the input buffer space
//
//--------------------------------------------------------------------------//

void CRUDupElimLogScanner::SetupScan()
{
	const CRUDupElimGlobals &globals = GetDupElimGlobals();
	Int32 i;

	// Which index do the CK columns start from in the tuple?
	// Count from 1 + control columns + syskey
	ckStartColumn_ = globals.GetNumCtrlColumns()+2;

	// Retrieve the tuple's descriptors ...
	Lng32 len = tupleDesc_.GetLength();
	for (i=0; i < len; i++)
	{
		Int32 colIndex = i + ckStartColumn_;
		tupleDesc_.GetItemDesc(i).Build(*pResultSet_, colIndex);
	}

	// Allocate space for input buffer 
	Int32 updateBmpSize = globals.GetUpdateBmpSize();
	for (i =0; i<IB_SIZE; i++)
	{
		inputBuf_[i] = new CRUIUDLogRecord(tupleDesc_, updateBmpSize);
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimLogScanner::UpdateStatistics()
//	
//	Update the statistics for each MV that will observe 
//	the current log record, i.e., each MV that has
//
//		MV.EPOCH[T] >= this-record-epoch
//
//	For this, iterate through all the epochs in the emptiness
//	check vector, and pick the relevant structures in the hash.
//
//--------------------------------------------------------------------------//

void CRUDupElimLogScanner::UpdateStatistics()
{
	CRUEmpCheckVector::Elem *pElem;
	CRUEmpCheckVecIterator it(empCheckVector_);

	for (;NULL != (pElem = it.GetCurrentElem()); it.Next())
	{
		TInt32 ep = pElem->epoch_;

		if (pCurrentRec_->GetEpoch() < ep)
		{
			// This MV has observed me already
			continue;
		}

		// The statistics structure in the hash
		CRUDeltaStatistics &stat = statMap_[ep];

		if (FALSE == pCurrentRec_->IsSingleRowOp())
		{
			if (FALSE == pCurrentRec_->IsBeginRange())
			{
				UpdateRangeStatistics(stat);
			}
		}
		else
		{
			UpdateSingleRowStatistics(stat);
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimLogScanner::UpdateRangeStatistics()
//
//	Update the statistics entry from a range record.
//	
///	Summarize the number of ranges and the number of rows covered by them.
//	If at least one range record without a range size estimate is
//	encountered (i.e., @RANGE_SIZE = -1), then the whole statistics record
//	will remain without the estimate (no matter what other records are 
//	encountered).
//
//--------------------------------------------------------------------------//

void CRUDupElimLogScanner::UpdateRangeStatistics(CRUDeltaStatistics &stat)
{
	stat.IncrementCounter(stat.nRanges_);
	
	if (CRUDeltaStatistics::RANGE_SIZE_UNKNOWN == stat.nRangeCoveredRows_)
	{
		// We have already seen one range without an estimate
		return;
	}

	TInt32 rangeSize = pCurrentRec_->GetRangeSize();
	if (CRUDeltaStatistics::RANGE_SIZE_UNKNOWN == rangeSize)
	{
		stat.nRangeCoveredRows_ = CRUDeltaStatistics::RANGE_SIZE_UNKNOWN;
	}
	else
	{
		stat.IncrementCounter(stat.nRangeCoveredRows_, rangeSize);
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimLogScanner::UpdateSingleRowStatistics()
//
//	Update the statistics entry from a single-row record
//--------------------------------------------------------------------------//

void CRUDupElimLogScanner::UpdateSingleRowStatistics(CRUDeltaStatistics &stat)
{
	RUASSERT (TRUE == GetDupElimGlobals().IsSingleRowResolv());

	if (FALSE == pCurrentRec_->IsPartOfUpdate())
	{
		if (TRUE == pCurrentRec_->IsInsert())
		{
			stat.IncrementCounter(stat.nInsertedRows_);
		}
		else
		{
			stat.IncrementCounter(stat.nDeletedRows_);
		}
	}
	else
	{
		stat.IncrementCounter(stat.nUpdatedRows_);

		// Compute the superposition of update bitmaps
		CRUUpdateBitmap bmp(*(pCurrentRec_->GetUpdateBitmap()));

		if (NULL == stat.pUpdateBitmap_)
		{
			stat.pUpdateBitmap_ = new CRUUpdateBitmap(bmp);
		}
		else
		{
			*(stat.pUpdateBitmap_) |= bmp;
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimLogScanner::CopyLowerBoundParams()
//
//	Set the lower-bound parameters to the phase 1 query statement.
//	The lower bound is the maximum clustering key value reached by the 
//	previous phase. 
//
//	The query statement is composed of the variable number of blocks.
//	The lower bound tuple values must be passed as parameters to every 
//	block of the query.
//
//--------------------------------------------------------------------------//

void CRUDupElimLogScanner::CopyLowerBoundParams()
{
	RUASSERT(NULL != pLowerBoundRec_);
	
	Int32 tupleDescLen = tupleDesc_.GetLength();
	
	// Compute the number of the query's parameters
	Lng32 nParams = pCurrentStmt_->GetParamCount();

	// There is an integral number of blocks
	RUASSERT(nParams > 0 && 0 == nParams % tupleDescLen);

	Int32 nBlocks = nParams/tupleDescLen;
	
	for (Int32 i=0; i<nBlocks; i++)
	{
		pLowerBoundRec_->
			CopyCKTupleValuesToParams(*pCurrentStmt_, 1+i*tupleDescLen);
	}
}
