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
#ifndef _RU_TEST_TASK_EXECUTOR_H_
#define _RU_TEST_TASK_EXECUTOR_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuTestTaskExecutor.cpp
* Description:  Definition of class CRUTestTaskExecutor
*
*
* Created:      01/09/2001
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuTaskExecutor.h"
#include "RuSQLDynamicStatementContainer.h"


class REFRESH_LIB_CLASS CRUTestTaskExecutor : public CRUTaskExecutor
{
public:
	enum { SQL_TEXT_MAX_SIZE = 1000 };

private:
	typedef CRUTaskExecutor inherited;

public:
	CRUTestTaskExecutor(CRUTask *pParentTask = NULL) 
		:
		pDynamicSQLContainer_(NULL),
		pNumberOfExecutions_(NULL),
		pNumberOfRetries_(NULL),
		pNumberOfFailures_(NULL),
		pAutoCommit_(NULL),
		errorDynamicSQLContainer_(1)

	{}

	virtual ~CRUTestTaskExecutor()
	{
		delete[] pNumberOfExecutions_;
		delete[] pNumberOfRetries_;
		delete[] pNumberOfFailures_;
		delete[] pAutoCommit_;
		delete pDynamicSQLContainer_;
	}

public:
	
	virtual void Init() 
	{
		SetState(EX_READ_GROUP);
	}
	
	virtual void Work();

	void SetNumberOfStatements(Int32 numberOfStatements)
	{
		numberOfStatements_ = numberOfStatements;
	}

	void SetGroupId(Int32 groupId)
	{
		groupId_ = groupId;
	}

	Int32 GetGroupId()
	{
		return groupId_;
	}

	virtual Lng32 GetIpcBufferSize() const
	{
		return 2000;
	}

public:
	// These functions serialize/de-serialize the executor's context 
	// for the message communication with the remote server process
	
	// Used in the main process side
	virtual void StoreRequest(CUOFsIpcMessageTranslator &translator)
	{
		inherited::StoreRequest(translator);
		StoreData(translator);
	}

	virtual void LoadReply(CUOFsIpcMessageTranslator &translator)
	{
		inherited::LoadReply(translator);
		LoadData(translator);
	}
	
	// Used in the remote process side
	virtual void LoadRequest(CUOFsIpcMessageTranslator &translator)
	{
		inherited::LoadRequest(translator);
		LoadData(translator);
	}

	virtual void StoreReply(CUOFsIpcMessageTranslator &translator)
	{
		inherited::StoreReply(translator);
		StoreData(translator);
	}

public:
	// fsm's state definitions
	enum STATES { EX_AFTER_COMPILATION_BEFORE_EXECUTION = MAIN_STATES_START  ,
				  EX_READ_GROUP = REMOTE_STATES_START ,
				  EX_COMPILE_ALL,
				  EX_EXECUTE
	};
private:
	void ReadSqlStatement();
	void ExecuteAllStatements();
	void ExecuteStatement(Int32 i);
	void HandleError( Int32 groupId ,
					  Int32 processId,
					  Int32 ordinal,
					  CDMException &e);

	void StoreData(CUOFsIpcMessageTranslator &translator);
	void LoadData(CUOFsIpcMessageTranslator &translator);

private:

	//-- Prevent copying
	CRUTestTaskExecutor(const CRUTestTaskExecutor &other);
	CRUTestTaskExecutor &operator = (const CRUTestTaskExecutor &other);

private:

	Int32 numberOfStatements_;
	Int32 groupId_;

private:
	CRUSQLDynamicStatementContainer *pDynamicSQLContainer_;
	Int32 *pNumberOfExecutions_;
	Int32 *pNumberOfRetries_;
	Int32 *pNumberOfFailures_;
	Int32 *pAutoCommit_;
	CRUSQLDynamicStatementContainer errorDynamicSQLContainer_;
};


#endif
