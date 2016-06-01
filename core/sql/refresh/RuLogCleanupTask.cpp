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
* File:         RuLogCleanupTask.cpp
* Description:  Implementation of class	CRULogCleanupTask
*
*
* Created:      12/29/1999
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuLogCleanupTask.h"
#include "RuLogCleanupTaskExecutor.h"

#include "RuMV.h"

//---------------------------------------------------------------//
//	Constructor 
//---------------------------------------------------------------//

CRULogCleanupTask::CRULogCleanupTask(Lng32 id, CRUTbl &table) :
	inherited(id,table),
	maxInapplicableEpoch_(0)
{}

//---------------------------------------------------------//
//	CRULogCleanupTask::CreateExecutorInstance()
//
//	Task executor's instance creation.
//
//---------------------------------------------------------//

CRUTaskExecutor *CRULogCleanupTask::CreateExecutorInstance()
{
	ComputeMaxInapplicableEpoch();

	CRUTaskExecutor *pTaskEx = new CRULogCleanupTaskExecutor(this);

	return pTaskEx;
}

//--------------------------------------------------------------------------//
//	CRULogCleanupSQLComposer::ComputeMaxInapplicableEpoch()
//
//	Compute the minimal value of MV.EPOCH[T] between the ON REQUEST
//	MVs that use T. 
//
//	All the log records that have the ABSOLUTE epoch value between this value 
//	and 100 (the first 100 epochs in the log are reserved for special purposes)
//	must be deleted.
//
//--------------------------------------------------------------------------//

void CRULogCleanupTask::ComputeMaxInapplicableEpoch()
{
	const TInt64 myUID = GetTable().GetUID();

	CRUMVList &mvList = GetTable().GetOnRequestMVsUsingMe();

	DSListPosition pos = mvList.GetHeadPosition();

	CRUMV *pCurrMV = mvList.GetNext(pos);
	maxInapplicableEpoch_ = pCurrMV->GetEpoch(myUID);

	while (NULL != pos)
	{
		pCurrMV = mvList.GetNext(pos);

                // not interested in the epoch if its in ignore 
                // changes list
                if (FALSE == pCurrMV->IsIgnoreChanges(myUID)) 
		{
		  TInt32 ep = pCurrMV->GetEpoch(myUID);

		  maxInapplicableEpoch_ = 
			(ep < maxInapplicableEpoch_) ? ep : maxInapplicableEpoch_;
                }
	}

	// One less than the minimal MV.EPOCH[T]
	maxInapplicableEpoch_--;
}
