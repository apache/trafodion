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
* File:         RuRcReleaseTaskExecutor.cpp
* Description:  Implementation of class	CRURcReleaseTaskExecutor
*
* Created:      10/18/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "RuRcReleaseTask.h"
#include "RuObject.h"
#include "RuRcReleaseTaskExecutor.h"

//------------------------------------------------------------------//
//	Constructor
//------------------------------------------------------------------//

CRURcReleaseTaskExecutor::CRURcReleaseTaskExecutor(CRUTask *pParentTask) :
	inherited(pParentTask)
{}

//------------------------------------------------------------------//
//	CRURcReleaseTaskExecutor::Init()
//------------------------------------------------------------------//

void CRURcReleaseTaskExecutor::Init()
{
	CRURcReleaseTask *pParentTask = (CRURcReleaseTask *)GetParentTask();
	CRUObject &obj = pParentTask->GetObject();

	if (FALSE == obj.HoldsResources())
	{
		ResetHasWork();
	}
}

//------------------------------------------------------------------//
//	CRURcReleaseTaskExecutor::Work()
//
//	The main finite-state machine switch
//------------------------------------------------------------------//

void CRURcReleaseTaskExecutor::Work()
{
	CRURcReleaseTask *pParentTask = (CRURcReleaseTask *)GetParentTask();
	RUASSERT(NULL != pParentTask);
	
	BeginTransaction();

	CRUObject &obj = pParentTask->GetObject();
	RUASSERT(TRUE == obj.HoldsResources());

	obj.ReleaseResources();
	
	// Simulate the crash, and check the atomicity of SaveMetadata() !
	TESTPOINT(CRUGlobals::TESTPOINT112);

	CommitTransaction();

	SetState(EX_COMPLETE);
}
