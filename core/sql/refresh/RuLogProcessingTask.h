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
#ifndef _RU_LOG_PROC_TASK_H_
#define _RU_LOG_PROC_TASK_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuLogProcessingTask.h
* Description:  Definition of class CRULogProcessingTask 
*				
*
* Created:      08/29/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "refresh.h"

#include "RuTask.h"
#include "RuTbl.h"

class CRUEmpCheckVector;

//---------------------------------------------------------//
//	CRULogProcessingTask
//	
//	This abstract class is a base class for the TableSync,
//	EmpCheck, DE and Log Cleanup tasks.
//
//	Every Log Processing task is associated with a single
//	CRUTbl object.
//	
//---------------------------------------------------------//

class REFRESH_LIB_CLASS CRULogProcessingTask : public CRUTask {

private:
	typedef CRUTask inherited;

public:
	CRULogProcessingTask(Lng32 id, CRUTbl &table);
	virtual ~CRULogProcessingTask();

	//-----------------------------------//
	// Accessors
	//-----------------------------------//
public:
	CRUTbl &GetTable() const
	{ 
		return table_; 
	}

	virtual BOOL HasObject(TInt64 uid) const
	{
		return table_.GetUID() == uid;
	}

	// Implementation of pure virtual (overriding the default behavior).
	// If my only successor has failed - I am obsolete
	virtual void HandleSuccessorFailure(CRUTask &task);

private:
	CRUTbl &table_;
};

#endif
