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
#ifndef _RU_RUNTIME_CONTROLLER_H_
#define _RU_RUNTIME_CONTROLLER_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuRuntimeController.h
* Description:  Definition of class CRURuntimeController
*
* Created:      05/07/2000
* Language:     C++
*
*
*
******************************************************************************
*/

#include "refresh.h"

#include "RuRequest.h"
#include "RuException.h"

//--------------------------------------------------------------//
//	CRURuntimeController
//
//	 The runtime part of the REFRESH utility's execution
//	 is managed by two finite-state machines that are called
//	 *runtime controllers*.
//
//	 The first FSM is a *flow controller*, which is responsible
//	 for task scheduling and processing the results of task 
//	 execution.
//	
//	 The second FSM is an *exec controller*, which is responsible
//	 for task execution (local or remote). 
//
//	 Each of the two machines communicates with its *peer* by
//	 posting a single *request* to it. A machine can also
//	 post a request to itself, in order to simplify the logic.
//	 There is always at most one outstanding request in the system
//	 at any given moment.
// 
//--------------------------------------------------------------//

class REFRESH_LIB_CLASS CRURuntimeController {

public:
	CRURuntimeController() : 
		pPeerController_(NULL),
		pRequest_(NULL)
	{}

	virtual ~CRURuntimeController() {}

public:
	// The controller to communicate with
	void SetPeerController(CRURuntimeController *pPeerController)
	{
		pPeerController_ = pPeerController;
	}

public:
	BOOL HasWork() const
	{
		return (NULL != pRequest_);
	}
	
	void PostRequest(CRURuntimeControllerRqst *pNewRqst)
	{
		RUASSERT((NULL == pRequest_) && (NULL != pNewRqst));
		pRequest_ = pNewRqst;
	}

	//-- Single step of execution
	void Work();

protected:
	//-- The actual switch for event handling
	//-- Delegated to derived classes
	virtual void HandleRequest(CRURuntimeControllerRqst *pRequest) = 0;

	CRURuntimeController *GetPeerController() 
	{ 
		return pPeerController_; 
	}

private:
	CRURuntimeController *pPeerController_;
	CRURuntimeControllerRqst *pRequest_;
};

//--------------------------------------------------------------//
//	The inliners
//--------------------------------------------------------------//

inline void CRURuntimeController::Work()
{
	RUASSERT( 
		TRUE == this->HasWork() 
		&&
		FALSE == GetPeerController()->HasWork()
		);

	// Cache the pointer, but dequeue the request
	// (do not return to the same request after a possible failure!)
	CRURuntimeControllerRqst *pRqst = pRequest_;
	pRequest_ = NULL;

	HandleRequest(pRqst);
	
	delete pRqst;
}

#endif
