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
#ifndef _RU_RC_RELEASE_TASK_H_
#define _RU_RC_RELEASE_TASK_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuRcReleaseTask.h
* Description:  Definition of class	CRURcReleaseTask
*
* Created:      10/18/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"

#include "RuTask.h"
#include "RuObject.h"

//--------------------------------------------------------------------------//
//	CRURcReleaseTask
//
//	This task implements the release of resources 
//	captured at the previous stages of the Refresh
//	utility's execution: DDL lock + (optionally) 
//	read-protected open.
//
//	The task operates on a single object (MV or table).
//	It is always performed in the main process.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRURcReleaseTask : public CRUTask {

private:
	typedef CRUTask inherited;

public:
	CRURcReleaseTask(Lng32 id, CRUObject &obj);
	~CRURcReleaseTask() {}

	//-----------------------------------//
	// Accessors
	//-----------------------------------//
public:
	CRUObject &GetObject() const
	{
		return obj_;
	}

	//-- Implementation of pure virtuals
	virtual CRUTask::Type GetType() const 
	{ 
		return CRUTask::RC_RELEASE; 
	}

	virtual BOOL HasObject(TInt64 uid) const
	{
		return (obj_.GetUID() == uid);
	}

	// Override the default behavior:
	// I do not react automatically on the predecessor's failure
	virtual void HandlePredecessorFailure(CRUTask &task) {}

	//---------------------------------------//
	//	PRIVATE AND PROTECTED AREA
	//---------------------------------------//
protected:
	//-- Implementation of pure virtuals

	virtual CDSString GetTaskName() const
	{
		return "RR(" + obj_.GetFullName() +")"; 
	}

	virtual CRUTaskExecutor *CreateExecutorInstance();

	virtual TInt32 GetComputedCost() const
	{
		return 0;
	}

	virtual BOOL IsImmediate() const
	{
		return FALSE;
	}

private:
	//-- Prevent copying
	CRURcReleaseTask(const CRURcReleaseTask &other);
	CRURcReleaseTask& operator= (const CRURcReleaseTask &other);

private:
	CRUObject &obj_;	// The object to release
};

#endif
