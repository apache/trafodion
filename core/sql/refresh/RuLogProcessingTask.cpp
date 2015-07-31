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
* File:         RuLogProcessingTask.cpp
* Description:  Implementation of class CRULogProcessingTask 
*				
*
* Created:      08/29/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuLogProcessingTask.h"
#include "RuException.h"

//--------------------------------------------------------------------------//
//	Constructor
//--------------------------------------------------------------------------//

CRULogProcessingTask::CRULogProcessingTask(Lng32 id, CRUTbl &table) :
	inherited(id), 
	table_(table)
{}

CRULogProcessingTask::~CRULogProcessingTask() {}

//--------------------------------------------------------------------------//
//	CRULogProcessingTask::HandleSuccessorFailure()
//
//	If this is the last task that depends on me -
//	I am obsolete, and must not be executed.
//--------------------------------------------------------------------------//

void CRULogProcessingTask::HandleSuccessorFailure(CRUTask &taask)
{
	if (1 == GetTasksThatDependOnMe().GetCount())
	{
		CRUException &ex = GetErrorDesc();
		ex.SetError(IDS_RU_OBSOLETE_PROBLEM);
		ex.AddArgument(GetTaskName());
	}
}
