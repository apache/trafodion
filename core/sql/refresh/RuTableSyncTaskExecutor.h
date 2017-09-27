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
#ifndef _RU_TABLE_SYNC_TASK_EXECUTOR_H_
#define _RU_TABLE_SYNC_TASK_EXECUTOR_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuTableSyncTaskExecutor.h
* Description:  Definition of class CRUTableSyncTaskExecutor.
*
* Created:      12/06/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"

#include "RuTaskExecutor.h"
#include "RuSQLDynamicStatementContainer.h"

class CRUTbl;
class CRUTableSyncTask;
class CUOFsIpcMessageTranslator;

//--------------------------------------------------------------------------//
//	CRUTableSyncTaskExecutor
//	
//	The executor is responsible for the following tasks :
//
//	1.	Increment the epoch of a table that needs it (when ever the log may 
//      be consumed in this refresh invocation),this stage may be executed in 
//		the remote process
//	2.  Lock the table in case a long lock is needed (long lock is a lock 
//      that remains until the last mv that required that lock has been 
//		refreshed)
//	3.  Save the syncronization timestamp in the table object for further 
//		use	
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUTableSyncTaskExecutor : public CRUTaskExecutor
{
private:
	typedef CRUTaskExecutor inherited;

	//----------------------------------//
	//	Public Members
	//----------------------------------//

public:
	CRUTableSyncTaskExecutor(CRUTask *pParentTask = NULL);
	virtual ~CRUTableSyncTaskExecutor();

public:
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

public:
	//-- Implementation of pure virtual functions
	virtual void Work();
	virtual void Init();

	//----------------------------------//
	//	Protected Members
	//----------------------------------//	

protected:
	enum { SIZE_OF_PACK_BUFFER = 1000 };

	//-- Implementation of pure virtual
	virtual Lng32 GetIpcBufferSize() const
	{
		return SIZE_OF_PACK_BUFFER;
	}

	//----------------------------------//
	//	Private Members
	//----------------------------------//	

	//Init() callee
private:
	//  This functions updates the ddol cache object ,but avoids any
	//  catalog opreation by clearing the object's modify flag.
	//  The procedure also retrievs the cat api text that is needed for 
	//  increasing the epoch  
	void PrepareCatApi();

	//Work() callee
private:
	void IncEpoch();
	void Epilogue();


private:
	//-- Prevent copying
	CRUTableSyncTaskExecutor(const CRUTableSyncTaskExecutor &other);
	CRUTableSyncTaskExecutor &operator = (const CRUTableSyncTaskExecutor &other);

private:

	enum STATES { EX_EPILOGUE = MAIN_STATES_START ,
				  EX_INC_EPOCH = REMOTE_STATES_START };

private:
	
	enum SQL_STATEMENT { INC_EPOCH = 0, 
						 NUM_OF_SQL_STMT // should always be last
	};

	CRUSQLDynamicStatementContainer syncTableDynamicContainer_;

	CDSString tableName_;
};

#endif
