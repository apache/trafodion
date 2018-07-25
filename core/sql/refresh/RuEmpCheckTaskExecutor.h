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
#ifndef _RU_EMP_CHECK_TASK_EXECUTOR_H_
#define _RU_EMP_CHECK_TASK_EXECUTOR_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuEmpCheckTaskExecutor.h
* Description:  Definition of class CRUEmpCheckTaskExecutor.
*
* Created:      04/06/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"

#include "RuTaskExecutor.h"

class CRUEmpCheckTask;
class CRUEmpCheck;
class CRUEmpCheckVector;
class CUOFsIpcMessageTranslator;

//--------------------------------------------------------------------------//
//	CRUEmpCheckTaskExecutor
//	
//	Performs the EmpCheck protocol.
//
//	Also, computes and exports the current database timestamp
//	and the emptiness check vector, which will be further
//	forwarded to the client Refresh tasks.
//	
//	The whole execution is performed in a single step.
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUEmpCheckTaskExecutor : public CRUTaskExecutor
{
private:
	typedef CRUTaskExecutor inherited;

public:
	CRUEmpCheckTaskExecutor(CRUTask *pParentTask = NULL);
	virtual ~CRUEmpCheckTaskExecutor();

	//----------------------------------//
	//	Accessors
	//----------------------------------//
public:
	const CRUEmpCheckVector &GetEmpCheckVector() const;

public:
	// These functions serialize/de-serialize the executor's context 
	// for the message communication with the remote server process
	
	// Used in the main process side
	virtual void StoreRequest(CUOFsIpcMessageTranslator &translator);
	virtual void LoadReply(CUOFsIpcMessageTranslator &translator);
	
	// Used in the remote process side
	virtual void LoadRequest(CUOFsIpcMessageTranslator &translator);
	virtual void StoreReply(CUOFsIpcMessageTranslator &translator);

	//----------------------------------//
	//	Mutators
	//----------------------------------//
public:
	//-- Single execution step.
	//-- Implementation of pure virtual functions
	virtual void Work();
	virtual void Init();

protected:
	enum { SIZE_OF_PACK_BUFFER = 1000 };

	//-- Implementation of pure virtual 
	virtual Lng32 GetIpcBufferSize() const
	{
		return SIZE_OF_PACK_BUFFER; // Initial size 
	}

private:
	enum STATES {  EX_CHECK = REMOTE_STATES_START };

private:
	//-- Prevent copying
	CRUEmpCheckTaskExecutor(const CRUEmpCheckTaskExecutor &other);
	CRUEmpCheckTaskExecutor &operator = (const CRUEmpCheckTaskExecutor &other);

private:
	//-- Work() callees
	void PerformEmptinessCheck();

private:
	CRUEmpCheck *pEmpCheck_;
};

#endif
