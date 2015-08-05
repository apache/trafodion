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
* File:         RuSQLSimpleRefreshComposer.cpp
* Description:  Implementation of class RuSQLSimpleRefreshComposer
*				
*
* Created:      08/13/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuSimpleRefreshSQLComposer.h"
#include "RuTbl.h"

//--------------------------------------------------------------------------//
//	Constructor
//--------------------------------------------------------------------------//

CRUSimpleRefreshSQLComposer::
CRUSimpleRefreshSQLComposer(CRURefreshTask *pTask) 
	: CRURefreshSQLComposer(pTask) 
{}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshSQLComposer::ComposeRefresh() 
//--------------------------------------------------------------------------//

void CRUSimpleRefreshSQLComposer::ComposeRefresh()
{
	StartInternalRefresh();

	AddDeltaDefListClause();
	
	if (GetRefreshTask().GetMVList().GetCount() > 1)
	{
		AddPipeLineClause();
	}
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshSQLComposer::ComposeRecompute() 
//--------------------------------------------------------------------------//

void CRUSimpleRefreshSQLComposer::ComposeRecompute(NoDeleteClause flag)
{
	StartInternalRefresh();

	if (REC_DELETE == flag)
	{
		sql_ += " RECOMPUTE ";
	}
	else
	{
		sql_ += " RECOMPUTE NODELETE ";
	}
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshSQLComposer::ComposeLockMV()
// 
//  This function construct an sql statement that locks the MV
//--------------------------------------------------------------------------//

void CRUSimpleRefreshSQLComposer::ComposeLock(const CDSString& name, 
											  BOOL exclusive)
{
	sql_ = "LOCK TABLE " + name;

	if (TRUE == exclusive)
	{
		sql_ += " IN EXCLUSIVE MODE ";
	}
	else
	{
		sql_ += " IN SHARE MODE ";
	}
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshSQLComposer::ComposeUnLockMV()
// 
//  This function construct an sql statement that locks the MV
//--------------------------------------------------------------------------//

void CRUSimpleRefreshSQLComposer::ComposeUnLock(const CDSString& name)
{
	sql_ = "UNLOCK TABLE " + name;
}

//--------------------------------------------------------------------------//
//	CRUSimpleRefreshSQLComposer::AddDeltaDefListClause()
//
//	Generate the delta-def clauses syntax for each non-empty T-delta 
//	observed by the MV.
//
//--------------------------------------------------------------------------//

void CRUSimpleRefreshSQLComposer::AddDeltaDefListClause()
{
	RUASSERT(GetRefreshTask().GetDeltaDefList().GetCount() > 0);

	sql_ += "\n FROM ";
	sql_ += (TRUE == GetRefreshTask().IsSingleDeltaRefresh()) ? 
		"SINGLEDELTA " : "MULTIDELTA ";

	DSListPosition pos = GetRefreshTask().GetDeltaDefList().GetHeadPosition();

	for (;;)
	{
		CRUDeltaDef *pDdef = GetRefreshTask().GetDeltaDefList().GetNext(pos);

		CDSString fromEpoch(TInt32ToStr(pDdef->fromEpoch_)) ;
		CDSString toEpoch(TInt32ToStr(pDdef->toEpoch_));

		AddDeltaDefClause(pDdef,fromEpoch,toEpoch);

		if (NULL == pos)
		{
			break;	// The last clause
		}
		else
		{
			sql_ += ", ";
		}
	}	
	if (FALSE == GetRefreshTask().IsSingleDeltaRefresh())
	{
		AddPhaseParam();
	}
}
