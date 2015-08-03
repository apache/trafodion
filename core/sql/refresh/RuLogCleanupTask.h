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
#ifndef _RU_LOG_CLEANUP_TASK_H_
#define _RU_LOG_CLEANUP_TASK_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuLogCleanupTask.h
* Description:  Definition of class	CRULogCleanupTask
*
* Created:      12/29/1999
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"

#include "RuTbl.h"

#include "RuLogProcessingTask.h"
#include "RuTaskExecutor.h"

// ----------------------------------------------------------------- //
// CRULogCleanupTask
//
//	 The Log Cleanup task class. Supports cleanup of inapplicable 
//	 epochs from a single table-log.
//
// ----------------------------------------------------------------- //

class REFRESH_LIB_CLASS CRULogCleanupTask : public CRULogProcessingTask {

private:
	typedef CRULogProcessingTask inherited;

	//---------------------------------------//
	//	PUBLIC AREA
	//---------------------------------------//
public:
	CRULogCleanupTask(Lng32 id, CRUTbl& table);
	virtual ~CRULogCleanupTask() {}

	//-----------------------------------//
	// Accessors
	//-----------------------------------//

public:
	TInt32 GetMaxInapplicableEpoch() const
	{
		return maxInapplicableEpoch_;
	}

	//-- Implementation of pure virtual functions
	virtual CRUTask::Type GetType() const 
	{ 
		return CRUTask::LOG_CLEANUP; 
	}

	//---------------------------------------//
	//	PRIVATE AND PROTECTED AREA
	//---------------------------------------//
protected:
	//-- Implementation of pure virtuals
	virtual CDSString GetTaskName() const
	{
		return "LC(" + GetTable().GetFullName() +")"; 
	}

	virtual TInt32 GetComputedCost() const
	{
		return 0;
	}

	virtual BOOL IsImmediate() const
	{
		return FALSE;
	}

	virtual CRUTaskExecutor *CreateExecutorInstance();

private:
	//-- Prevent copying
	CRULogCleanupTask(const CRULogCleanupTask &other);
	CRULogCleanupTask& operator= (const CRULogCleanupTask &other);

private:
	void ComputeMaxInapplicableEpoch();

private:
	TInt32 maxInapplicableEpoch_;
};

#endif
