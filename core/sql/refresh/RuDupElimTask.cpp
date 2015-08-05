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
* File:         RuDupElimTask.cpp
* Description:  Implementation of class CRUDupElimTask 
*				
*
* Created:      12/29/1999
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuDupElimTask.h"
#include "RuDupElimTaskExecutor.h"

#include "RuMV.h"
#include "RuTbl.h"

#include "RuEmpCheckVector.h"
#include "RuGlobals.h"
#include "RuOptions.h"

//--------------------------------------------------------------------------//
//	Constructor and destructor of CRUDupElimTask
//--------------------------------------------------------------------------//

CRUDupElimTask::CRUDupElimTask(Lng32 id, CRUTbl &table) :
	inherited(id, table),
	isRangeResolv_(FALSE),
	isSingleRowResolv_(FALSE),
	isSkipCrossTypeResolution_(FALSE),
	deLevel_(CRUDeltaDef::NO_DE),
	beginEpoch_(0),
	endEpoch_(0)
{}

CRUDupElimTask::~CRUDupElimTask() {}

//--------------------------------------------------------------------------//
//	CRUDupElimTask::NeedToExecuteDE()
//--------------------------------------------------------------------------//

BOOL CRUDupElimTask::NeedToExecuteDE() const
{
	return (TRUE == isRangeResolv_ || TRUE == isSingleRowResolv_);
}

//--------------------------------------------------------------------------//
//	CRUDupElimTask::CreateExecutorInstance()
//
//	Task executor's creation.
//
//--------------------------------------------------------------------------//

CRUTaskExecutor *CRUDupElimTask::CreateExecutorInstance()
{
	// Setup the internal data structures ...
	SetupDS();

	// Create the executor itself
	CRUTaskExecutor *pTaskEx = new CRUDupElimTaskExecutor(this);

	return pTaskEx;
}

//--------------------------------------------------------------------------//
//	CRUDupElimTask::PullDataFromExecutor()
//
//	Refinement of the base class's method.
//
//--------------------------------------------------------------------------//

void CRUDupElimTask::PullDataFromExecutor()
{
	if (FALSE == WasExecuted())
	{
		return;	//	Nothing to take from the executor
	}

	CRUTbl &tbl = GetTable();

	CRUDupElimTaskExecutor &taskEx = (CRUDupElimTaskExecutor &)GetExecutor();
	
	tbl.SetDELevel(GetDELevel());
	// Copy the DE statistics to the table object
	CRUDeltaStatisticsMap &statMap = tbl.GetStatisticsMap();
	statMap = taskEx.GetStatisticsMap();

	tbl.PropagateDEStatisticsToUsingMVs();
}

//--------------------------------------------------------------------------//
//	CRUDupElimTask::GetDeltaStatisticsSize()
//
//	Estimate the room for the delta statistics in the reply from the 
//	task process (this affects the size of the IPC buffer to allocate).
//
//--------------------------------------------------------------------------//

Lng32 CRUDupElimTask::GetDeltaStatisticsBufSize() const
{
	CRUTbl &tbl = GetTable();

	Int32 nStatEntries = tbl.GetIncrementalInvolvedMVsUsingMe().GetCount();

	Int32 singleStatSize = 
		CRUDeltaStatistics::GetPackedBufferSize(tbl.GetUpdateBitmapSize());
		
	return nStatEntries * singleStatSize;
}

//--------------------------------------------------------------------------//
//	PRIVATE AREA
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimTask::SetupDS()
//
//	CreateExecutorInstance() callee.
//
//	Setup the internal data structures before the executor is created.
//
//--------------------------------------------------------------------------//

void CRUDupElimTask::SetupDS()
{
	CRUTbl &tbl = GetTable();
	CRUEmpCheckVector &ecVector = tbl.GetEmpCheckVector();

	// The sweep scans the delta up to the current epoch (not including).
	endEpoch_ = tbl.GetCurrentEpoch() - 1;
	// begin-epoch = MIN(MV.EPOCH[T]), for MV on T
	beginEpoch_ = ecVector.GetMinEpoch();

	// Range resolution must be performed if there are ranges in the delta
	if (TRUE == ecVector.IsDeltaNonEmpty(
			GetBeginEpoch(), 
			CRUTbl::RANGE)
		)
	{
		isRangeResolv_ = TRUE; 
	}

	BOOL isSingleDeltaNonEmpty = ecVector.IsDeltaNonEmpty(
		GetBeginEpoch(), 
		CRUTbl::SINGLE_ROW
	);

	// Single-row resolution must be performed if:
	// (1) The table is used by an MJV
	// (2) There are single-row records in the delta
	// (3) The table is not INSERTLOG.
	if (TRUE == IsSingleRowResolutionEnforced()
		&&
		TRUE == isSingleDeltaNonEmpty
		&&
		FALSE == tbl.IsInsertLog())
	{
		isSingleRowResolv_ = TRUE; 
	}

	BOOL isCrossTypeResolutionEnforced = IsCrossTypeResolutionEnforced();

	//	Cross-type duplicates must NOT be resolved in two cases:
	//	(1) The table is used by multi-txn MVs only (hence, 
	//	    the resolution is not enforced).
	//	(2) There are no single-row records in the delta.
	isSkipCrossTypeResolution_ = 
		(FALSE == isCrossTypeResolutionEnforced
		 ||
		 FALSE == isSingleDeltaNonEmpty);

	// Compute the DE level
	if (TRUE == IsSingleRowResolutionEnforced())
	{
		deLevel_ = CRUDeltaDef::SINGLE_2_SINGLE;
	}
	else if (TRUE == isCrossTypeResolutionEnforced)
	{
		deLevel_ = CRUDeltaDef::RANGE_2_SINGLE;
	}
	else
	{
		deLevel_ = CRUDeltaDef::RANGE_2_RANGE;
	}

	// Build the list of the clustering key columns (for the SQL composer)
	tbl.BuildKeyColumnList();
}

//--------------------------------------------------------------------------//
//	CRUDupElimTask::IsSingleRowResolutionEnforced()
//
//	The full-scale conflict resolution (including resolving
//	the duplicated between single-row log records) will be
//	enforced in two cases:
//	(1) The table is used by an incrementally refreshed MJV
//		(because the Internal Refresh of MJV requires no 
//		single-row duplicates).
//	(2) "Debug 55" option appears in the command. This option
//		is used for testing only, in the DEBUG mode.
//	
//--------------------------------------------------------------------------//

BOOL CRUDupElimTask::IsSingleRowResolutionEnforced() const
{
	BOOL isSingleRowResolutionEnforced = 
		GetTable().IsUsedByIncrementalMJV();

#ifdef _DEBUG
	CRUOptions &options = CRUGlobals::GetInstance()->GetOptions();
	if (NULL != options.FindDebugOption(CRUGlobals::ENFORCE_FULL_DE, ""))
	{
		isSingleRowResolutionEnforced = TRUE;
	}
#endif

	return isSingleRowResolutionEnforced;
}

//--------------------------------------------------------------------------//
//	CRUDupElimTask::IsCrossTypeResolutionEnforced()
//
//	The cross-type conflicts (i.e., range vs single-row) must NOT be 
//	resolved if all the incrementally refreshed MVs on this table are multi-
//	transactional (hence, the INTERNAL REFRESH will filter the cross-type 
//	duplicates by itself).
//
//--------------------------------------------------------------------------//

BOOL CRUDupElimTask::IsCrossTypeResolutionEnforced() const
{
#ifdef _DEBUG
	// The REFRESH_WITH_DE environment variable allows 
	// the user to enforce the cross-type duplicate elimination always
	const char *env = getenv("REFRESH_WITH_DE");
	if (NULL != env)
	{
		return TRUE;
	}
#endif

	if (TRUE == GetTable().IsUsedOnlyByMultiTxnMvs())
	{
		return FALSE;
	}

	return TRUE;
}
