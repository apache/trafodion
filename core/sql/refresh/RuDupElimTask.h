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
#ifndef _RU_DUP_ELIM_TASK_H_
#define _RU_DUP_ELIM_TASK_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuDupElimTask.h
* Description:  Definition of class CRUDupElimTask 
*
* Created:      8/23/1999
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"

#include "RuLogProcessingTask.h"
#include "RuDeltaDef.h"

//--------------------------------------------------------------------------//
// CRUDupElimTask
//
//	 The Duplicate Elimination (DE) task class. 
//
//	 The task's executor performs the duplicate elimination 
//	 algorithm on a single table-log. 
//
//	 The purpose of the duplicate elimination algorithm is to resolve 
//	 all the logged operations on each table row (which is identified 
//	 by a unique clustering key) into a correct minimum sequence. 
//
//	 Along with resolving redundancies, duplicate elimination has two 
//	 more key purposes. The first one is to extract new ranges from 
//	 the IUD log and move them to the range log. 
//	 The second one is to collect statistics about the log. This is done
//   in order to boost the performance of INTERNAL REFRESH. 
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUDupElimTask : public CRULogProcessingTask {

private:
	typedef CRULogProcessingTask inherited;

	//---------------------------------------//
	//	PUBLIC AREA
	//---------------------------------------//
public:
	CRUDupElimTask(Lng32 id, CRUTbl &table);
	virtual ~CRUDupElimTask();

public:
	//-- Implementation of pure virtual functions
	virtual CRUTask::Type GetType() const 
	{ 
		return CRUTask::DUP_ELIM; 
	}
	
public:
	TInt32 GetBeginEpoch() const
	{ 
		return beginEpoch_; 
	}
	TInt32 GetEndEpoch() const
	{ 
		return endEpoch_; 
	}

	BOOL IsRangeResolv() const
	{
		return isRangeResolv_;
	}
	BOOL IsSingleRowResolv() const
	{
		return isSingleRowResolv_;
	}

	CRUDeltaDef::DELevel GetDELevel() const
	{
		return deLevel_;
	}

	BOOL IsSkipCrossTypeResoultion() const
	{
		return isSkipCrossTypeResolution_;
	}

	// How much space will be required by statistics data in the IPC buffer?
	Lng32 GetDeltaStatisticsBufSize() const;

	// Can Duplicate Elimination be skipped?
	BOOL NeedToExecuteDE() const;

	//---------------------------------------//
	//	PRIVATE AND PROTECTED AREA
	//---------------------------------------//

protected:
	//-- Implementation of pure virtual functions
	virtual CDSString GetTaskName() const
	{
		return "DE(" + GetTable().GetFullName() +")"; 
	}

	virtual CRUTaskExecutor *CreateExecutorInstance();

	virtual void PullDataFromExecutor();

	virtual TInt32 GetComputedCost() const
	{
		return 0;
	}

	virtual BOOL IsImmediate() const
	{
		return (FALSE == NeedToExecuteDE());
	}

private:
	//-- Prevent copying
	CRUDupElimTask(const CRUDupElimTask &other);
	CRUDupElimTask& operator= (const CRUDupElimTask &other);

	//	Setup the internal data structures before the executor is created
	void SetupDS();
	
	BOOL IsSingleRowResolutionEnforced() const;
	BOOL IsCrossTypeResolutionEnforced() const;

private:
	// Do we need to apply range/single-row resolution?
	BOOL isRangeResolv_;
	BOOL isSingleRowResolv_;
	BOOL isSkipCrossTypeResolution_;

	// Epoch boundaries for the duplicate elimination scan
	TInt32 beginEpoch_;
	TInt32 endEpoch_;

	CRUDeltaDef::DELevel deLevel_;
};

#endif
