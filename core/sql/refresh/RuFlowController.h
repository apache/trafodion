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
#ifndef _RU_FLOW_CONTROLLER_H_
#define _RU_FLOW_CONTROLLER_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuFlowController.h
* Description:  Definition of class CRUFlowController
*
* Created:      05/07/2000
* Language:     C++
*
*
*
******************************************************************************
*/

#include "refresh.h"

#include "RuRuntimeController.h"

class CRUDependenceGraph;

//--------------------------------------------------------------------------//
//	CRUFlowController
//	
//	The flow controller FSM is responsible for task scheduling 
//	and processing the results of task execution. 
//
//	The class operates the dependence graph object, which is created 
//	at the utility's prologue stage, and is responsible for task 
//	communication and scheduling
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUFlowController : public CRURuntimeController {

private:
	typedef CRURuntimeController inherited;

public:
	CRUFlowController(
		CRUDependenceGraph &dg, 
		TInt32 maxParallelism
	);
	virtual ~CRUFlowController() {}

public:
	BOOL DidTaskFailuresHappen() const 
	{ 
		return didTaskFailuresHappen_; 
	}

protected:
	//-- Pure virtual function implementation
	//-- The actual switch for event handling
	virtual void HandleRequest(CRURuntimeControllerRqst *pRequest);

private:
	//-- Prevent copying
	CRUFlowController(const CRUFlowController &other);
	CRUFlowController &operator = (const CRUFlowController &other);

private:
	//-- HandleRequest() callees
	void HandleScheduleRqst();
	void HandleFinishTaskRqst(CRURuntimeControllerRqst *pRqst);

	void RouteScheduledTask(CRUTask *pTask);
	static BOOL NeedToReportTaskFailure(CRUTask &task);

	// Notify the tasks that depend on this one about its completion
	void NotifyTaskEnvironmentOnFailure(CRUTask &task);

private:
	CRUDependenceGraph &dg_;
	TInt32 maxParallelism_;
	unsigned short nRunningTasks_;

	BOOL didTaskFailuresHappen_;
};

#endif
