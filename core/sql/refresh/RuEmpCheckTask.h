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
#ifndef _RU_EMP_CHECK_TASK_H_
#define _RU_EMP_CHECK_TASK_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuEmpCheckTask.h
* Description:  Definition of class CRUEmpCheckTask.
*
*
* Created:      04/06/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"

#include "RuLogProcessingTask.h"

//--------------------------------------------------------------------------//
//	CRUEmpCheckTask
//	
//	The EmpCheck task (works on a single table) checks whether the delta(s) 
//	of the table towards the using MV(s) are empty. The emptiness check's 
//	results are recorded in the *emptiness check vector*. For each MV on T,
//	the vector's element V[MV.EPOCH[T]] contains two flags (a bitmap):
//	(1) Are there single-row records in the log starting from epoch MV.EPOCH[T]?
//	(2) Are there range records in the log starting from epoch MV.EPOCH[T]?
//
//	The task's executor applies the EmpCheck algorithm as a generic unit.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUEmpCheckTask : public CRULogProcessingTask {

private:
	typedef CRULogProcessingTask inherited;

	//---------------------------------------//
	//	PUBLIC AREA
	//---------------------------------------//
public:
	CRUEmpCheckTask(Lng32 id, CRUTbl &table);
	virtual ~CRUEmpCheckTask();

	//-----------------------------------//
	// Accessors
	//-----------------------------------//
public:
	//-- Implementation of pure virtuals
	virtual CRUTask::Type GetType() const 
	{ 
		return CRUTask::EMP_CHECK; 
	}

	//---------------------------------------//
	//	PRIVATE AND PROTECTED AREA
	//---------------------------------------//
protected:
	
	virtual CDSString GetTaskName() const;

	// Refinement of the parent's method
	virtual void PullDataFromExecutor();

	// Create the concrete task executor
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
	CRUEmpCheckTask(const CRUEmpCheckTask &other);
	CRUEmpCheckTask& operator= (const CRUEmpCheckTask &other);
};

#endif
