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
#ifndef _RU_REQUEST_H_
#define _RU_REQUEST_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuRequest.h
* Description:  Definition of classes CRURuntimeControllerRqst and CRURuntimeControllerRqstList
*
* Created:      05/09/2000
* Language:     C++
*
* 
******************************************************************************
*/

#include "refresh.h"
#include "RuException.h"

class CRUTask;

//--------------------------------------------------------------------------//
//	CRURuntimeControllerRqst
//
//	A base class for requests to be exchanged by
//	the REFRESH utility's runtime controllers (see
//	the header file RuRuntimeController.h for details).
//	
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRURuntimeControllerRqst {

public:
	// Types of requests
	enum Type {

		AWAIT_EVENT			= 0,

		SCHEDULE			= 1,
		
		START_TASK			= 2,
		FINISH_TASK			= 3,

		EXECUTE_TASK_STEP	= 4
	};

public:
	CRURuntimeControllerRqst(Type type, CRUTask *pTask=NULL) : 
	  type_(type), pTask_(pTask) {}

	virtual ~CRURuntimeControllerRqst() {}

public:
	Type GetType() const
	{ 
		return type_; 
	}

	CRUTask &GetTask() const
	{
		RUASSERT(NULL != pTask_);
		return *pTask_;
	}

private:
	Type type_;
	CRUTask *pTask_;
};

#endif
