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
* File:         RuEmpCheckTask.cpp
* Description:  Implementation of class CRUEmpCheckTask
*				
*
* Created:      12/29/1999
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuEmpCheckTask.h"
#include "RuEmpCheckTaskExecutor.h"
#include "RuEmpCheckVector.h"
#include "RuTbl.h"

//---------------------------------------------------------------//
//	Constructor and destructor of CRUEmpCheckTask
//---------------------------------------------------------------//

CRUEmpCheckTask::CRUEmpCheckTask(Lng32 id, CRUTbl &table) :
	inherited(id, table)
{}

CRUEmpCheckTask::~CRUEmpCheckTask() 
{}

//--------------------------------------------------------------------------//
//	CRUEmpCheckTask::GetTaskName() 
//--------------------------------------------------------------------------//

CDSString CRUEmpCheckTask::GetTaskName() const
{
	CDSString name("EC(");
	name += GetTable().GetFullName() + " ) ";
	
	return name;
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckTask::CreateExecutorInstance()
//
//	Task executor's creation
//--------------------------------------------------------------------------//

CRUTaskExecutor *CRUEmpCheckTask::CreateExecutorInstance()
{
	// Setup the data structure ...
	GetTable().BuildEmpCheckVector();

	CRUTaskExecutor *pTaskEx = new CRUEmpCheckTaskExecutor(this);

	return pTaskEx;
}

//--------------------------------------------------------------------------//
//	CRUEmpCheckTask::PullDataFromExecutor()
//
//	(1) Copy the updated emptiness check vector 
//		from the executor to the table object.
//	(2) If the check's results are final (i.e.,
//	    the table is not an involved MV), broadcast
//		the check's results to the using MVs.
//	
//--------------------------------------------------------------------------//

void CRUEmpCheckTask::PullDataFromExecutor()
{
	inherited::PullDataFromExecutor();

	CRUEmpCheckTaskExecutor &taskEx = 
		(CRUEmpCheckTaskExecutor &)GetExecutor();

	CRUTbl &tbl = GetTable();

	// Copy the emptiness check vector back to the table object
	CRUEmpCheckVector &empCheckVec = tbl.GetEmpCheckVector();
	empCheckVec = taskEx.GetEmpCheckVector();

	if (FALSE == tbl.IsInvolvedMV()) 
	{
		empCheckVec.SetFinal();
		// Update the using MVs' delta-def lists
		tbl.PropagateEmpCheckToUsingMVs();
	}
}
