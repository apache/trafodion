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
* File:         RuLockEquivSetTask.cpp
* Description:  Implementation of class CRULockEquivSetTask
*				
*
* Created:      12/06/1999
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuLockEquivSetTask.h"
#include "RuLockEquivSetTaskExecutor.h"
#include "RuRefreshTask.h"

//-------------------------------------------------------------------------//
//	Constructor and destructor of CRULockEquivSetTask
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
// The constructor will rearrange the table list and will put the 
// tables with the attribute no lock on refresh in the last places
// in the list.This is done in order to lock those tables last 
// and this way we will shorten the time of the lock
//--------------------------------------------------------------------------//

CRULockEquivSetTask::CRULockEquivSetTask(Lng32 id,const CRUTblList &tblList) :
	CRUTask(id),
	tblList_(eItemsArentOwned)
{
	DSListPosition pos = tblList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList.GetNext(pos);
		if (TRUE == pTbl->IsNoLockOnRefresh())
		{
			tblList_.AddTail(pTbl);
		}
		else
		{
			tblList_.AddHead(pTbl);
		}
	}
}

CRULockEquivSetTask::~CRULockEquivSetTask() 
{}

//--------------------------------------------------------------------------//
//	CRULockEquivSetTask::GetTaskName() 
//--------------------------------------------------------------------------//

CDSString CRULockEquivSetTask::GetTaskName() const
{
	CDSString name("LOCK SET(");

	DSListPosition pos = tblList_.GetHeadPosition();
	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList_.GetNext(pos);
		name += pTbl->GetFullName(); 
		if (NULL != pos) 
		{
			name += ",";
		}
	}
	name += ")";
	return name;
}

//--------------------------------------------------------------------------//
//	CRULockEquivSetTask::CreateExecutorInstance()
//
//	Task executor's creation
//--------------------------------------------------------------------------//

CRUTaskExecutor *CRULockEquivSetTask::CreateExecutorInstance()
{
	CRUTaskExecutor *pTaskEx = new CRULockEquivSetTaskExecutor(this);

	return pTaskEx;
}


//--------------------------------------------------------------------------//
//	CRURefreshTask::HasObject()
//--------------------------------------------------------------------------//

BOOL CRULockEquivSetTask::HasObject(TInt64 uid) const
{
	DSListPosition pos = tblList_.GetHeadPosition();
	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList_.GetNext(pos);
		if (pTbl->GetUID() == uid)
		{
			return TRUE;
		}
	}
	return FALSE;
}
