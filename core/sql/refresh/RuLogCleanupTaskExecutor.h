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
#ifndef _RU_LOG_CLEANUP_TASK_EXECUTOR_H_
#define _RU_LOG_CLEANUP_TASK_EXECUTOR_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuLogCleanupTaskExecutor.h
* Description:  Definition of class CRULogCleanupTaskExecutor.
*
* Created:      05/25/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"

#include "RuTaskExecutor.h"
#include "RuSQLDynamicStatementContainer.h"

class CRULogCleanupTask;

//--------------------------------------------------------------------------//
//	CRULogCleanupTaskExecutor
//
//	Implementation of cleanup of data which has been observed
//	by all of the ON REQUEST MVs on this table.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRULogCleanupTaskExecutor : public CRUTaskExecutor
{
private:
	typedef CRUTaskExecutor inherited;

public:
	CRULogCleanupTaskExecutor(CRUTask *pParentTask = NULL);
	virtual ~CRULogCleanupTaskExecutor() {}

	//----------------------------------//
	//	Accessors
	//----------------------------------//
public:
	CRULogCleanupTask *GetLogCleanupTask() 
	{ 
		return (CRULogCleanupTask *) GetParentTask(); 
	}

	//----------------------------------//
	//	Mutators
	//----------------------------------//
public:
	//-- Implementation of pure virtual functions
	virtual void Work();
	virtual void Init();

public:
	enum SQL_STATEMENT {
		
		CLEAN_IUD_BASIC	  = 0,
		CLEAN_IUD_FIRSTN,
		CLEAN_IUD_MCOMMIT,
		CLEAN_RANGE,
                CLEAN_ROWCOUNT,

		NUM_OF_SQL_STMT // should always be last
	};

	// These functions serialize/de-serialize the executor's context 
	// for the message communication with the remote server process

	// Used in the main process side
	virtual void StoreRequest(CUOFsIpcMessageTranslator &translator);
	virtual void LoadReply(CUOFsIpcMessageTranslator &translator)
	{
		inherited::LoadReply(translator);
	}
	
	// Used in the remote process side
	virtual void LoadRequest(CUOFsIpcMessageTranslator &translator);
	virtual void StoreReply(CUOFsIpcMessageTranslator &translator)
	{
		inherited::StoreReply(translator);
	}

protected:
	enum { SIZE_OF_PACK_BUFFER = 2000 };

	//-- Implementation of pure virtual 
	virtual Lng32 GetIpcBufferSize() const
	{
		return SIZE_OF_PACK_BUFFER; // Initial size 
	}

private:
	//-- Prevent copying
	CRULogCleanupTaskExecutor(const CRULogCleanupTaskExecutor &other);
	CRULogCleanupTaskExecutor &operator = (const CRULogCleanupTaskExecutor &other);

private:
	//Init() callee
	void ComposeMySql();

	//Work() callees
	void Start();
	void Clean();
	void Epilogue();

        SQL_STATEMENT decideOnDeleteMethod();
	void CleanLogBasic();
	void CleanLogFirstN(SQL_STATEMENT statement);
	void CleanLogMultiCommit();
        TInt64 getRowCount();

private:
	enum State { 
		
		EX_EPILOGUE = MAIN_STATES_START, 
		EX_CLEAN = REMOTE_STATES_START 
	};

	CRUSQLDynamicStatementContainer logCleanupTEDynamicContainer_;
	BOOL hasRangeLog_;
        Int32  noOfPartitions_;
};

#endif
