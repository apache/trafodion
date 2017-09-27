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
* File:         RuEmpCheckTaskExecutor.cpp
* Description:  Implementation of class CRUEmpCheckTaskExecutor.
*				
*
* Created:      04/06/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuEmpCheckTaskExecutor.h"
#include "RuEmpCheckTask.h"

#include "RuEmpCheck.h"
#include "RuTbl.h"
#include "uofsIpcMessageTranslator.h"
#include "RuOptions.h"

//--------------------------------------------------------------------------//
//	Constructor and destructor
//--------------------------------------------------------------------------//

CRUEmpCheckTaskExecutor::CRUEmpCheckTaskExecutor(CRUTask *pParentTask) :
	inherited(pParentTask),
	pEmpCheck_(NULL)
{}

CRUEmpCheckTaskExecutor::~CRUEmpCheckTaskExecutor()
{
	delete pEmpCheck_;
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckTaskExecutor::GetEmpCheckVector()
//--------------------------------------------------------------------------//

const CRUEmpCheckVector &
CRUEmpCheckTaskExecutor::GetEmpCheckVector() const
{
	RUASSERT(NULL != pEmpCheck_);

	return pEmpCheck_->GetVector();
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckTaskExecutor::Init()
//	
//	Initialize the emptiness check unit
//--------------------------------------------------------------------------//

void CRUEmpCheckTaskExecutor::Init()
{
	inherited::Init();
	
	CRUEmpCheckTask *pParentTask = (CRUEmpCheckTask *)GetParentTask();
	
	RUASSERT(NULL != pParentTask);

	CRUTbl &tbl = pParentTask->GetTable();

	pEmpCheck_ = new CRUEmpCheck(tbl.GetEmpCheckVector());

	// We are interested only in the the records that are logged 
	// BEFORE the epoch increment, so we place an upper bound
	TInt32 upperBound;
	
	if (TRUE == tbl.IsInvolvedMV())
	{
		// The check happens before the epoch increment
		upperBound = tbl.GetCurrentEpoch();
	}
	else
	{	
		// The check happens after the epoch increment
		upperBound = tbl.GetCurrentEpoch()-1;
	}

	pEmpCheck_->ComposeSQL(tbl, upperBound);

	SetState(EX_CHECK);
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckTaskExecutor::LoadRequest()
//--------------------------------------------------------------------------//
void CRUEmpCheckTaskExecutor::
LoadRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::LoadRequest(translator);

	RUASSERT(NULL == pEmpCheck_);

	pEmpCheck_ = new CRUEmpCheck();
	
	pEmpCheck_->LoadData(translator);
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckTaskExecutor::LoadReply()
//--------------------------------------------------------------------------//
void CRUEmpCheckTaskExecutor::
LoadReply(CUOFsIpcMessageTranslator &translator)
{
	inherited::LoadReply(translator);

	RUASSERT(NULL != pEmpCheck_);

	pEmpCheck_->LoadData(translator);
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckTaskExecutor::StoreRequest()
//--------------------------------------------------------------------------//
void CRUEmpCheckTaskExecutor::
StoreRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::StoreRequest(translator);

	pEmpCheck_->StoreData(translator);

	translator.SetMessageType(CUOFsIpcMessageTranslator::
							  RU_EMP_CHECK_EXECUTOR);
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckTaskExecutor::StoreReply()
//--------------------------------------------------------------------------//
void CRUEmpCheckTaskExecutor::
StoreReply(CUOFsIpcMessageTranslator &translator)
{
	inherited::StoreReply(translator);

	pEmpCheck_->StoreData(translator);
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckTaskExecutor::Work()
//
//	Perform the emptiness check algorithm.
//
//--------------------------------------------------------------------------//

void CRUEmpCheckTaskExecutor::Work()
{
	RUASSERT(EX_CHECK == GetState());

	PerformEmptinessCheck();
	
	SetState(EX_COMPLETE);
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckTaskExecutor::PerformEmptinessCheck()
//	
//	Work() callee.
//	
//	Iterate through the values of MV.EPOCH[T] of using MVs,
//	in the descending order, and perform the delta emptiness
//	check (by selecting the first (occasional) record from
//	the T-delta for the MV. Once a non-empty delta is encoun-
//	tered, it always holds that the MVs that observe the log
//	from earlier epochs also see a non-empty delta, and the
//	search can be stopped.
//	
//--------------------------------------------------------------------------//

void CRUEmpCheckTaskExecutor::
PerformEmptinessCheck()
{
	pEmpCheck_->PrepareSQL();

	BeginTransaction();
	pEmpCheck_->PerformCheck();		
	CommitTransaction(); // Although no write operations were performed...
}
