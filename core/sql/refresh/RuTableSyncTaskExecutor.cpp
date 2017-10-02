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
* File:         RuTableSyncTaskExecutor.cpp
* Description:  Implementation of class CRUTableSyncTaskExecutor.
*				
*
* Created:      12/06/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuTableSyncTaskExecutor.h"
#include "RuTableSyncTask.h"

#include "RuTbl.h"
#include "uofsIpcMessageTranslator.h"
#include "RuOptions.h"

//--------------------------------------------------------------------------//
//	Constructor and destructor
//--------------------------------------------------------------------------//

CRUTableSyncTaskExecutor::CRUTableSyncTaskExecutor(CRUTask *pParentTask) :
	inherited(pParentTask),
	syncTableDynamicContainer_(NUM_OF_SQL_STMT),
	tableName_("")
{}

CRUTableSyncTaskExecutor::~CRUTableSyncTaskExecutor()
{}

//--------------------------------------------------------------------------//
//	CRUTableSyncTaskExecutor::Init()
//	Initialize the table sync executor
//--------------------------------------------------------------------------//

void CRUTableSyncTaskExecutor::Init()
{
	inherited::Init();

	
	CRUTableSyncTask *pParentTask = (CRUTableSyncTask *)GetParentTask();

	RUASSERT(NULL != pParentTask);

	CRUTbl &tbl = pParentTask->GetTable();

	if (FALSE == tbl.IsIncEpochNeeded())
	{
		ResetHasWork();
		return;
	}

	tableName_ = tbl.GetFullName();
	
	// Decides whether the table needs a long lock,and set the table attribute
	// prepare the sql text and update the ddol cache object
	PrepareCatApi();

	SetState(EX_INC_EPOCH);
}

//--------------------------------------------------------------------------//
//	CRUTableSyncTaskExecutor::PrepareCatApi()
//
//  This functions updates the ddol cache object ,but avoids any
//  catalog opreation by clearing the object's modify flag.
//  The procedure also retrievs the cat api text that is needed for 
//  increasing the epoch  
//--------------------------------------------------------------------------//

void CRUTableSyncTaskExecutor::PrepareCatApi()
{
	CRUTableSyncTask *pParentTask = (CRUTableSyncTask *)GetParentTask();

	RUASSERT(NULL != pParentTask);

	CRUTbl &tbl = pParentTask->GetTable();

	tbl.IncrementCurrentEpoch(FALSE);
	
	CDSString catApiText = tbl.GetIncEpochCatApiText();

	syncTableDynamicContainer_.SetStatementText
				(INC_EPOCH,catApiText);
}

//--------------------------------------------------------------------------//
//	CRUTableSyncTaskExecutor::StoreRequest()
//--------------------------------------------------------------------------//

void CRUTableSyncTaskExecutor::
	StoreRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::StoreRequest(translator);

	Int32 stringSize = tableName_.GetLength() + 1;
	translator.WriteBlock(&stringSize, sizeof(Int32));
	translator.WriteBlock(tableName_.c_string(), stringSize);;
	
	syncTableDynamicContainer_.StoreData(translator);

	translator.SetMessageType(CUOFsIpcMessageTranslator::
							  RU_TABLE_SYNC_EXECUTOR);
}

//--------------------------------------------------------------------------//
//	CRUTableSyncTaskExecutor::LoadRequest()
//--------------------------------------------------------------------------//

void CRUTableSyncTaskExecutor::
	LoadRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::LoadRequest(translator);

	Int32 stringSize;
	Int32 const maxStringSize = CUOFsIpcMessageTranslator::MaxMsgSize;
	char buffer[maxStringSize];

	translator.ReadBlock(&stringSize, sizeof(Int32));
	
	RUASSERT(maxStringSize > stringSize);

	translator.ReadBlock(buffer, stringSize);
	
	tableName_ = CDSString(buffer);

	syncTableDynamicContainer_.LoadData(translator);
}

//--------------------------------------------------------------------------//
//	CRUTableSyncTaskExecutor::Work()
//
//	Perform all the actions that the task was configured for.
//
//--------------------------------------------------------------------------//

void CRUTableSyncTaskExecutor::Work()
{
	switch (GetState())
	{
	case EX_INC_EPOCH: // REMOTE PROCESS
		{
			IncEpoch();
			break;
		}
	case EX_EPILOGUE: // MAIN PROCESS
		{
			Epilogue();
			break;
		}
	default: RUASSERT(FALSE);
	}
}
	
//--------------------------------------------------------------------------//
//	CRUTableSyncTaskExecutor::IncEpoch()
//	
//	Work() callee.
//
//  Executes the cat api statement "INC EPOCH" on the table
//--------------------------------------------------------------------------//

void CRUTableSyncTaskExecutor::IncEpoch()
{
	TESTPOINT2(CRUGlobals::TESTPOINT140, tableName_);

	// Crash the remote process 
	TESTPOINT_SEVERE2(CRUGlobals::REMOTE_CRASH_IN_TABLE_SYNC, tableName_);

	CDMPreparedStatement *pStat = syncTableDynamicContainer_.
									GetPreparedStatement(INC_EPOCH);

	BeginTransaction();

	ExecuteStatement(*pStat,IDS_RU_EPOCH_INC_FAILED, tableName_.c_string());

	CommitTransaction();

	TESTPOINT2(CRUGlobals::TESTPOINT141, tableName_);

	SetState(EX_EPILOGUE);
}

//--------------------------------------------------------------------------//
//	CRUTableSyncTaskExecutor::Epilogue()
//	
// Unlock the log lock and set a lock on the table if it is needed.The
// function also save the syncronization timestamp in the table object
//--------------------------------------------------------------------------//

void CRUTableSyncTaskExecutor::Epilogue()
{
	CRUTableSyncTask *pParentTask = (CRUTableSyncTask *)GetParentTask();

	RUASSERT(NULL != pParentTask);

	CRUTbl &tbl = pParentTask->GetTable();

	if (TRUE == tbl.IsIncEpochNeeded())
	{
		if (TRUE == tbl.IsLongLockNeeded())
		{
			BeginTransaction();
			tbl.ExecuteReadProtectedOpen();
			CommitTransaction();
		}

		if (TRUE == tbl.IsLogRPOpenPending())
		{
			// The above condition is false only in case of 
			// a non involved incremental mv.We do not need to lock the logs 
			// because we rely on the ddl locks
			tbl.ReleaseLogReadProtectedOpen();
		}
	}
	SetState(EX_COMPLETE);

}


