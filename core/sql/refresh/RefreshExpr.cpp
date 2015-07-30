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
/* -*-C++-*-
******************************************************************************
*
* File:         RefreshExpr.cpp
* Description:  Execution engine of the REFRESH utility.
*
*
* Created:      01/09/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/
#include <string.h>

#include "RefreshExpr.h"

#include "RuJournal.h"
#include "RuCache.h"
#include "RuDependenceGraph.h"
#include "RuGlobals.h"
#include "RuDgBuilder.h"

#include "RuFlowController.h"
#include "RuExecController.h"

//--------------------------------------------------------------------------//
//	Constructor and destructor
//--------------------------------------------------------------------------//

CRefreshExpr::CRefreshExpr() :
	pJournal_(NULL),
	pCache_(NULL),
	pDepGraph_(NULL),
	pTransManager_(NULL),
	pFlowController_(NULL),
	pExecController_(NULL)
{}

CRefreshExpr::~CRefreshExpr()
{
	// The Done() method that has been applied before
	// (either normally or through exception handler)
	// has already freed all the memory.
	RUASSERT (TRUE == IsMemoryClean());
}

//--------------------------------------------------------------------------//
//	CRefreshExpr::Execute() 
//
//	The utility's execution engine.
//--------------------------------------------------------------------------//

void CRefreshExpr::Execute() 
{
	try 
	{
		Init();

		if (FALSE == options_.IsCancel())
		{
			// Full path - the utility was applied 
			// without the CANCEL option.
			// The DDL locks have been processed 
			// during the cache construction.
			Run();
		}
	}
	catch (CDSException &ex)
	{
		HandleExecuteException(ex);
	}
	catch (...)
	{
		CRUException ex; // A new exception object
		ex.SetError(IDS_RU_UNEXPECTED);
		HandleExecuteException(ex);
	}

	// Avoid recursive call through exception handler
	Done();
}

//--------------------------------------------------------------------------//
//	CRefreshExpr::HandleExecuteException()
//--------------------------------------------------------------------------//

void CRefreshExpr::HandleExecuteException(CDSException &ex)
{
	if (NULL != pJournal_)
	{
		pJournal_->LogError(ex);
	}

	Done();

	throw ex;	// Re-throw
}

//--------------------------------------------------------------------------//
//	CRefreshExpr::Init()
//
//	The prologue stage. 
//
//	Initialize the internal objects (the main part is to build the cache 
//	based on the catalog data,	and the dependence graph based on the cache).
//
//--------------------------------------------------------------------------//

void CRefreshExpr::Init()
{
	// Open the output file
	InitJournal();

	// Initiate the trans manager and open TFILE
	InitTransactions();

	if (TRUE == pTransManager_->IsTransactionOpen())
	{
		// Error for calling REFRESH within a user defined transaction. 
		CRUException ex;
		ex.SetError(IDS_RU_USER_TXN);
		throw ex;
	}

	CRUGlobals::Init(options_, *pJournal_, *pTransManager_);

	InitCache();

	if (TRUE == options_.IsCancel())
	{
		return;		// Short execution path
	}

	InitDependenceGraph();

	InitControllers();
}

//--------------------------------------------------------------------------//
//	CRefreshExpr::Run()
//
//	The runtime stage. 
//
//	As long as one of the controllers has work to do - 
//	apply this controller. 
//
//	When the work is over, if the Exec controller has registered
//	any task failures - throw an exception (to be further logged).
//
//--------------------------------------------------------------------------//

void CRefreshExpr::Run()
{
	for (;;)
	{
		if (TRUE == pFlowController_->HasWork())
		{
			pFlowController_->Work();
			continue;
		}

		if (TRUE == pExecController_->HasWork())
		{
			pExecController_->Work();
			continue;
		}

		// Neither of the controllers has work to do
		break;
	}

	if (TRUE == pFlowController_->DidTaskFailuresHappen())
	{
		CompleteExecutionWithError();
	}
}

//--------------------------------------------------------------------------//
//	CRefreshExpr::Done()
//
//	The epilogue stage
//--------------------------------------------------------------------------//

void CRefreshExpr::Done()
{
	DoneTaskProcesses();

	DoneJournal();

	DoneTransManager();

	DoneMemory();
}

//--------------------------------------------------------------------------//
//	INTERNAL OBJECT INITIALIZATIONS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRefreshExpr::InitJournal()
//
//	Open the output file and log the opening message
//--------------------------------------------------------------------------//

void CRefreshExpr::InitJournal()
{
	pJournal_ = new CRUJournal(options_.GetOutputFilename());
	pJournal_->Open();
}

//--------------------------------------------------------------------------//
//	CRefreshExpr::InitTransactions()
//
//	Initiate the trans manager and open the TFILE (for multiple
//	transactions' management).
//
//--------------------------------------------------------------------------//

void CRefreshExpr::InitTransactions()
{
	if (pTransManager_ != NULL)
		return;

	pTransManager_ = new CUOFsTransManager();

#ifndef NA_LINUX
	pTransManager_->OpenTMF();
#endif
}

//--------------------------------------------------------------------------//
//	CRefreshExpr::InitCache()
//
//	Read the catalog data and populate the cache.
//
//	The whole operation will be done in a single transaction, in order to 
//	achieve consistency in reading the metadata and populating the DDL locks.
//
//--------------------------------------------------------------------------//

void CRefreshExpr::InitCache()
{
	if (pCache_ != NULL)
		return;

	RUASSERT(FALSE == pTransManager_->IsTransactionOpen());
	LOGTIME("Starting to build the Cache \n");
	
	pCache_ = new CRUCache();	

	try
	{
		pTransManager_->BeginTrans();
		pCache_->Build();

		RUASSERT(TRUE == pTransManager_->IsTransactionOpen());

		TESTPOINT(CRUGlobals::TESTPOINT170);
		pTransManager_->CommitTrans();
		TESTPOINT(CRUGlobals::TESTPOINT171);
	}
	catch (CDSException &ex)
	{
		HandleCatalogAccessException(ex);
	}
	catch (...)
	{
		CRUException ex;	// New exception object
		ex.SetError(IDS_RU_UNEXPECTED);
		HandleCatalogAccessException(ex);
	} 

	// If the CANCEL option is used, but some of the DDL locks 
	// could not be cancelled, the user must see an error message.
	if (TRUE == options_.IsCancel()
		&&
		TRUE == pCache_->DidDDLLockErrorsHappen())
	{
		CompleteExecutionWithError();
	}
}

//--------------------------------------------------------------------------//
//	CRefreshExpr::InitDependenceGraph()
//
//	Build the dependence graph based on the cache.
//
//	During the graph's construction, pre-runtime checks are performed.
//	One of them is checking the user's privileges to refresh the MV(s).
//	Currently, this is implemented as a direct read through DDOL, which
//	requires a transaction. The utility will control this (read-only) 
//	transaction's boundaries.
//
//--------------------------------------------------------------------------//

void CRefreshExpr::InitDependenceGraph()
{
	RUASSERT(FALSE == pTransManager_->IsTransactionOpen());
	LOGTIME("Starting to build the Dependence Graph \n");

	pDepGraph_ = new CRUDependenceGraph(pCache_->GetMaxPipelining());

	try 
	{	
		CRUDependenceGraphBuilder dgBuilder(*pCache_, *pDepGraph_);
		dgBuilder.Build();
	}
	catch (CDSException &ex)
	{
		HandleCatalogAccessException(ex);
	}
	catch (...)
	{
		CRUException ex;	// New exception object
		ex.SetError(IDS_RU_UNEXPECTED);
		HandleCatalogAccessException(ex);
	} 

	// No uncontrolled catalog access must have happened ...
	RUASSERT(FALSE == pTransManager_->IsTransactionOpen());
	
	// Debug only: dump the data structures and exit
	// if the user has used the option "debug 1".
#ifdef _DEBUG
	try
	{
		TESTPOINT(CRUGlobals::DUMP_DS);
	}
	catch (CRUException &ex)
	{
		DumpDSAfterInit();
		throw ex;
	}
#endif
}

//--------------------------------------------------------------------------//
//	CRefreshExpr::HandleCatalogAccessException()
//--------------------------------------------------------------------------//

void CRefreshExpr::HandleCatalogAccessException(CDSException &ex)
{
	try 
	{
		pTransManager_->AbortTrans();
	}
	catch (...)
	{
		// Do nothing if TMF has aborted the transaction by itself
	}

	// Stack a new error message
	ex.SetError(IDS_RU_CATALOG_ACCESS_FAILED);

	throw ex;	// Propagate exception
}

//--------------------------------------------------------------------------//
//	CRefreshExpr::InitControllers()
//
//	Create the flow controller and the exec controller FSMs.
//	Queue the SCHEDULE request to the flow controller, 
//	in order to bootstrap the runtime part's execution.
//
//--------------------------------------------------------------------------//

void CRefreshExpr::InitControllers()
{
	LOGTIME("Starting to build the Controllers \n");


	TInt32 maxParallelism = pCache_->GetMaxParallelism();
#ifdef _DEBUG
	if (NULL != getenv("REFRESH_NO_PARALLELISM"))
	{
		maxParallelism = 1;
	}
#endif
	BOOL useParallelism = (maxParallelism > 1);

	pFlowController_ = new CRUFlowController(*pDepGraph_, maxParallelism);
	pExecController_ = new CRUExecController(useParallelism);

	pFlowController_->SetPeerController(pExecController_);
	pExecController_->SetPeerController(pFlowController_);

	// Bootstrap the runtime part
	pFlowController_->PostRequest(
		new CRURuntimeControllerRqst(CRURuntimeControllerRqst::SCHEDULE)
	);
}

//--------------------------------------------------------------------------//
//	CRefreshExpr::CompleteExecutionWithError()
//	
//	If severe errors have happened during the execution, complete the 
//	command's execution with error (by throwing an exception that will be 
//	caught at the stored procedure's level).
//
//--------------------------------------------------------------------------//

void CRefreshExpr::CompleteExecutionWithError()
{
	// Let the upper-level error handler log the message
	CRUException ex;

	ex.SetError(IDS_RU_FAILURES_HAPPENED);
    
        // if the user specified the OUTFILE option instruct the user
        // to inspect the value of this option in case or errors otherwise
        // instruct the user to inspect EMS
        CDSString messageLocation = options_.GetOutputFilename();
        if (messageLocation.IsEmpty())
#ifdef NA_LINUX
           messageLocation = CDSString ("the event logs");
#else
           messageLocation = CDSString ("EMS");
#endif
        ex.AddArgument(messageLocation);

	throw ex;
}

#ifdef _DEBUG
//--------------------------------------------------------------------------//
//	CRefreshExpr::DumpDSAfterInit()
//
//	Dump the data structures after the initialization.
//--------------------------------------------------------------------------//

void CRefreshExpr::DumpDSAfterInit()
{
	CDSString errstr;

	options_.Dump(errstr);
	pCache_->Dump(errstr);
	pDepGraph_->DumpGraph(errstr);
	pJournal_->LogMessage(errstr);
}
#endif

//--------------------------------------------------------------------------//
//	DESTRUCTION METHODS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRefreshExpr::DoneJournal()
//--------------------------------------------------------------------------//

void CRefreshExpr::DoneJournal()
{
	if (NULL != pJournal_)
	{
		pJournal_->Close();
	}
}

//--------------------------------------------------------------------------//
//	CRefreshExpr::DoneTransManager()
//--------------------------------------------------------------------------//

void CRefreshExpr::DoneTransManager()
{
	if (NULL != pTransManager_)
	{
		pTransManager_->CloseTMF();
	}
}

//--------------------------------------------------------------------------//
//	CRefreshExpr::DoneMemory()
//
//	Release the dynamically allocated objects
//--------------------------------------------------------------------------//

void CRefreshExpr::DoneMemory()
{
	CRUGlobals::Done();

	delete pJournal_;
	pJournal_ = NULL;

	delete pCache_;
	pCache_ = NULL;

	delete pDepGraph_;
	pDepGraph_ = NULL;

	delete pFlowController_;
	pFlowController_ = NULL;

	delete pExecController_;
	pExecController_ = NULL;

	delete pTransManager_;
	pTransManager_ = NULL;

}

//--------------------------------------------------------------------------//
//	CRefreshExpr::DoneTaskProcesses()
//
//	Shutdown all the spawned task processes
//--------------------------------------------------------------------------//

void CRefreshExpr::DoneTaskProcesses()
{
	try 
	{
		if (NULL != pExecController_)
		{
			pExecController_->ShutDownTaskProcesses();
		}
	}
	catch(...)
	{
		// Ignore errors at shutdown
	}
}

//--------------------------------------------------------------------------//
//	CRefreshExpr::IsMemoryClean()
//
//	Verify that the internal objects are freed.
//--------------------------------------------------------------------------//

BOOL CRefreshExpr::IsMemoryClean() const
{
	return (
		NULL == pJournal_
		&&
		NULL == pCache_
		&&
		NULL == pDepGraph_
		&&
		NULL == pFlowController_
		&&
		NULL == pExecController_
		&&
		NULL == pTransManager_
	);
}
