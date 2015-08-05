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
#ifndef _RU_LOCK_EQUIV_SET_TASK_H_
#define _RU_LOCK_EQUIV_SET_TASK_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuLockEquivSetTask.h
* Description:  Definition of class CRULockEquivSetTask.
*
*
* Created:      12/06/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"

#include "RuTask.h"
#include "RuTbl.h"
#include "RuException.h"

//--------------------------------------------------------------//
//	CRULockEquivSetTask
//	
//	The purpose of the Lock Equivalence Set task is to execute 
//	synchronization between involved base tables by locking all the 
//  tables or the tables' logs that are members of this lock 
//	equivalence set.
//
//--------------------------------------------------------------//

class REFRESH_LIB_CLASS CRULockEquivSetTask : public CRUTask {

	//---------------------------------------//
	//	PUBLIC AREA
	//---------------------------------------//
public:

	CRULockEquivSetTask(Lng32 id, const CRUTblList &tblList);
	virtual ~CRULockEquivSetTask();

	//-----------------------------------//
	// Accessors
	//-----------------------------------//
public:
	//-- Implementation of pure virtuals
	virtual CRUTask::Type GetType() const 
	{ 
		return CRUTask::LOCK_EQUIV_SET; 
	}

	virtual BOOL HasObject(TInt64 uid) const;

	CRUTblList& GetTableList()
	{
		return tblList_;
	}

	//-----------------------------------//
	// Mutators
	//-----------------------------------//

	//---------------------------------------//
	//	PRIVATE AND PROTECTED AREA
	//---------------------------------------//
protected:
	virtual CDSString GetTaskName() const;

	// If my only successor has failed - I am obsolete
	virtual void HandleSuccessorFailure(CRUTask &task) {}

	// Create the concrete task executor
	virtual CRUTaskExecutor *CreateExecutorInstance();

	virtual TInt32 GetComputedCost() const
	{
		return 0;
	}

	virtual BOOL IsImmediate() const
	{
		return TRUE;
	}

private:
	//-- Prevent copying
	CRULockEquivSetTask(const CRULockEquivSetTask &other);
	CRULockEquivSetTask& operator= (const CRULockEquivSetTask &other);

private:

	CRUTblList tblList_;
};

#endif
