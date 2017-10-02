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
* File:         RuDupElimTaskExecutor.cpp
* Description:  Implementation of class CRUDupElimTaskExecutor.
*				
*
* Created:      05/25/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "uofsIpcMessageTranslator.h"

#include "RuDupElimTaskExecutor.h"
#include "RuDupElimTask.h"
#include "RuDupElimSQLComposer.h"
#include "RuDupElimLogScanner.h"
#include "RuDupElimSingleRowResolv.h"
#include "RuDupElimRangeResolv.h"

#include "RuTbl.h"
#include "RuKeyColumn.h"

#include "RuGlobals.h"
#include "RuOptions.h"
#include "RuJournal.h"

//--------------------------------------------------------------------------//
//	PUBLIC AREA
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	Constructor and destructor
//--------------------------------------------------------------------------//

CRUDupElimTaskExecutor::CRUDupElimTaskExecutor(CRUTask *pParentTask) :
	inherited(pParentTask),
	globals_(),
	ctrlStmtContainer_(CRUDupElimConst::NUM_CONTROL_STMTS),
	txnTimeLimit_(0),
	phase_(0),
	// The units
	pLogScanner_(NULL),
	pSingleRowResolver_(NULL),
	pRangeResolver_(NULL)
{}

CRUDupElimTaskExecutor::~CRUDupElimTaskExecutor()
{
	delete pLogScanner_;

	delete pSingleRowResolver_;

	delete pRangeResolver_;
}

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::GetStatisticsMap()
//--------------------------------------------------------------------------//

const CRUDeltaStatisticsMap &CRUDupElimTaskExecutor::
GetStatisticsMap() const
{
	RUASSERT (NULL != pLogScanner_);

	return pLogScanner_->GetStatisticsMap();
}

//--------------------------------------------------------------------------//
//	INITIALIZATION
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::Init()
//
//	Duplicate Elimination always runs under a (number of) transactions, 
//	and demands the log to be audited. This is very important, because 
//	otherwise in the case of failure the log can get out of sync 
//	with the table. The cases that do not comply with this condition 
//	must be blocked by the catalog/pre-runtime check.
//
//	At the Init stage, the following actions are performed:
//	(1) The globals are initialized.
//	(2) The execution units are created.
//	(3) The SQL statements' text is generated, and the units are
//		configured by it.
//	(4) The transaction's timeout is computed.
//
//--------------------------------------------------------------------------//

void CRUDupElimTaskExecutor::Init()
{
	inherited::Init(); 

	CRUDupElimTask *pParentTask = (CRUDupElimTask*) GetParentTask();

	RUASSERT(NULL != pParentTask);

	if (FALSE == pParentTask->NeedToExecuteDE())
	{
		ResetHasWork();	// Nothing to do
		return;
	}

	InitGlobals(pParentTask);
		
	InitUnits(pParentTask);

	InitSQL(pParentTask);

	InitTxnTimeLimit();
}

//--------------------------------------------------------------------------//
//	FINITE-STATE MACHINE
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::Work()
//
//	The main FSM switch.
//--------------------------------------------------------------------------//

void CRUDupElimTaskExecutor::Work()
{
	switch(GetState())
	{
	case EX_START:
		{
			ResetDECompleteFlag();	// Local
			break;
		}
	case EX_PERFORM_PHASE:					
		{
			PerformPhase();			// Remote, can happen mutiple times
			break;
		}
	case EX_EPILOGUE:	// Local
		{
			FinalMetadataUpdate();
			break;
		}
	default: RUASSERT(FALSE);
	};
}

//--------------------------------------------------------------------------//
//	INTER-PROCESS COMMUNICATION
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::GetIpcBufferSize()
//
//	Implementation of pure virtual. Estimate the IPC buffer size. The buffer
//	is allocated *before* the Init() method is applied.
//
//	The requestor/server communication model requires that the same buffer 
//	will serve for both request and reply. In most task executors, 
//	the request data is dominant by far, and the buffer size can be estimated
//	by a (reasonable) constant. However, the DE task executor returns (in the
//	reply) the delta statistics. The reply buffer size depends therefore 
//	on the number of statistics' blocks.
//	
//--------------------------------------------------------------------------//

Lng32 CRUDupElimTaskExecutor::GetIpcBufferSize() const
{
	Lng32 requestSize = SIZE_OF_REQUEST_PACK_BUFFER;

	Lng32 replySize = 
		((CRUDupElimTask*) GetParentTask())->GetDeltaStatisticsBufSize()
		+
		FIXED_PART_OF_REPLY_PACK_BUFFER;

	return (requestSize > replySize) ? requestSize : replySize;
}

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::LoadRequest()
//
//	REMOTE after receiving MAIN. Read the DE globals and control statements,
//  build the units and load their SQL text.
//
//--------------------------------------------------------------------------//
void CRUDupElimTaskExecutor::
LoadRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::LoadRequest(translator);

	BOOL flag;

	translator.ReadBlock(&txnTimeLimit_, sizeof(TInt32));

	globals_.LoadData(translator);
	ctrlStmtContainer_.LoadData(translator);

	// Build the units and load their SQL text
	RUASSERT(NULL == pLogScanner_);
	pLogScanner_ = new CRUDupElimLogScanner(globals_, ctrlStmtContainer_);

	pLogScanner_->LoadRequest(translator);
	
	RUASSERT(NULL == pSingleRowResolver_);
	translator.ReadBlock(&flag, sizeof(BOOL));
	if (TRUE == flag)
	{
		pSingleRowResolver_ = 
			new CRUDupElimSingleRowResolver(globals_, ctrlStmtContainer_);
		pSingleRowResolver_->LoadRequest(translator);
	}

	RUASSERT(NULL == pRangeResolver_);
	translator.ReadBlock(&flag, sizeof(BOOL));
	if (TRUE == flag)
	{
		pRangeResolver_ = 
			new CRUDupElimRangeResolver(globals_, ctrlStmtContainer_);
		pRangeResolver_->LoadRequest(translator);
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::StoreReply()
//
//	REMOTE before sending to MAIN, pack the statistics 
//--------------------------------------------------------------------------//
void CRUDupElimTaskExecutor::
StoreReply(CUOFsIpcMessageTranslator &translator)
{
	inherited::StoreReply(translator);

	BOOL flag;

	translator.WriteBlock(&phase_, sizeof(Int32));

	pLogScanner_->StoreReply(translator);
	
	if (NULL != pSingleRowResolver_)
	{
		flag = TRUE;
		translator.WriteBlock(&flag, sizeof(BOOL));
		pSingleRowResolver_->StoreReply(translator);
	}
	else
	{
		flag = FALSE;
		translator.WriteBlock(&flag, sizeof(BOOL));
	}

	if (NULL != pRangeResolver_)
	{
		flag = TRUE;
		translator.WriteBlock(&flag, sizeof(BOOL));
		pRangeResolver_->StoreReply(translator);
	}
	else
	{
		flag = FALSE;
		translator.WriteBlock(&flag, sizeof(BOOL));
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::LoadReply()
//
//	MAIN after receiving from REMOTE, unpack the statistics 
//--------------------------------------------------------------------------//
void CRUDupElimTaskExecutor::
LoadReply(CUOFsIpcMessageTranslator &translator)
{
	inherited::LoadReply(translator);
	
	BOOL flag;

	translator.ReadBlock(&phase_, sizeof(Int32));
	pLogScanner_->LoadReply(translator);
	
	translator.ReadBlock(&flag, sizeof(BOOL));
	if (TRUE == flag)
	{
		RUASSERT(NULL != pSingleRowResolver_);
		pSingleRowResolver_->LoadReply(translator);
	}
	
	translator.ReadBlock(&flag, sizeof(BOOL));
	if (TRUE == flag)
	{
		RUASSERT(NULL != pRangeResolver_);
		pRangeResolver_->LoadReply(translator);
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::StoreRequest()
//
//	MAIN before sending to REMOTE, pack the DE globals and the SQL text.
//
//--------------------------------------------------------------------------//
void CRUDupElimTaskExecutor::
StoreRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::StoreRequest(translator);

	BOOL flag;

	translator.WriteBlock(&txnTimeLimit_, sizeof(TInt32));

	globals_.StoreData(translator);
	ctrlStmtContainer_.StoreData(translator);

	pLogScanner_->StoreRequest(translator);
	
	if (NULL != pSingleRowResolver_)
	{
		flag = TRUE;
		translator.WriteBlock(&flag, sizeof(BOOL));
		pSingleRowResolver_->StoreRequest(translator);
	}
	else
	{
		flag = FALSE;
		translator.WriteBlock(&flag, sizeof(BOOL));
	}

	if (NULL != pRangeResolver_)
	{
		flag = TRUE;
		translator.WriteBlock(&flag, sizeof(BOOL));
		pRangeResolver_->StoreRequest(translator);
	}
	else
	{
		flag = FALSE;
		translator.WriteBlock(&flag, sizeof(BOOL));
	}
	
	translator.SetMessageType(CUOFsIpcMessageTranslator::RU_DE_EXECUTOR);
}

//--------------------------------------------------------------------------//
//	PRIVATE AREA
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	INITIALIZATION
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::InitGlobals()
//
//	Initialize the globals object (for the future usage by the units).
//--------------------------------------------------------------------------//

void CRUDupElimTaskExecutor::InitGlobals(CRUDupElimTask *pParentTask)
{
	CRUTbl &tbl = pParentTask->GetTable();

	BOOL isRangeResolv = pParentTask->IsRangeResolv();
	BOOL isSingleRowResolv = pParentTask->IsSingleRowResolv();

	RUASSERT(TRUE == isRangeResolv || TRUE == isSingleRowResolv);
	// Otherwise, the execution can be skipped

	// The @IGNORE and @UPDATE_BITMAP columns is extracted from the log
	// only if there is single-row resolution.

	Lng32 nCtrlColumns = (TRUE == isSingleRowResolv) ?
		CRUDupElimConst::NUM_IUD_LOG_CONTROL_COLS_EXTEND
		:
		CRUDupElimConst::NUM_IUD_LOG_CONTROL_COLS_BASIC;

	Lng32 updateBmpSize = (TRUE == isSingleRowResolv) ? 
		tbl.GetUpdateBitmapSize() : 0;

	globals_.Init(
		isRangeResolv,
		isSingleRowResolv,
		updateBmpSize,
		nCtrlColumns,
		tbl.GetKeyColumnList().GetCount(),
		tbl.GetLastDupElimEpoch(),
		pParentTask->GetBeginEpoch(),
		pParentTask->GetEndEpoch(),
		tbl.GetRangeLogType(),
		tbl.IsLastDEComplete(),
		pParentTask->IsSkipCrossTypeResoultion()
	);
}

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::InitUnits()
//
//	Create all the necessary units.
//--------------------------------------------------------------------------//

void CRUDupElimTaskExecutor::InitUnits(CRUDupElimTask *pParentTask)
{
	CRUTbl &tbl = pParentTask->GetTable();
	CRUEmpCheckVector &vec = tbl.GetEmpCheckVector();
	TInt32 beginEpoch = globals_.GetBeginEpoch();

	pLogScanner_ = new CRUDupElimLogScanner(globals_, ctrlStmtContainer_);
	pLogScanner_->SetEmpCheckVector(vec);

	if (TRUE == globals_.IsRangeResolv())
	{
		pRangeResolver_ = 
			new CRUDupElimRangeResolver(globals_, ctrlStmtContainer_);
	}

	// Initialize the single-row resolver only if single-row resolution 
	// is enforced and there are single-row records in the log
	if (TRUE == globals_.IsSingleRowResolv())
	{
		pSingleRowResolver_ = 
			new CRUDupElimSingleRowResolver(globals_, ctrlStmtContainer_);
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::InitSQL()
//
//	Generate the SQL text for the query (to compute the log's delta) 
//	and the IUD statements (to update the IUD and the range logs). 
//	Store the SQL in the task executor's units.
//
//--------------------------------------------------------------------------//

void CRUDupElimTaskExecutor::InitSQL(CRUDupElimTask *pParentTask)
{
	Int32 i;
	CRUDupElimSQLComposer comp(pParentTask, globals_);

	for (i=0; i<CRUDupElimConst::NUM_CONTROL_STMTS; i++)
	{
		comp.ComposeControlText(i);
		ctrlStmtContainer_.SetStatementText(i, comp.GetSQL());
	}

	for (i=0; i<CRUDupElimConst::NUM_QUERY_STMTS; i++)
	{
		comp.ComposeQueryText(i);
		pLogScanner_->SetStatementText(i, comp.GetSQL());
	}

	if (NULL != pSingleRowResolver_)
	{
		for (i=0; i<CRUDupElimConst::NUM_SINGLE_RESOLV_STMTS; i++)
		{
			comp.ComposeSingleRowResolvText(i);
			pSingleRowResolver_->SetStatementText(i, comp.GetSQL());
		}
	}

	if (NULL != pRangeResolver_)
	{
		for (i=0; i<CRUDupElimConst::NUM_RNG_RESOLV_STMTS; i++)
		{
			comp.ComposeRangeResolvText(i);
			pRangeResolver_->SetStatementText(i, comp.GetSQL());
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::InitTxnTimeLimit()
//
//	Compute the maximum duration of a single DE transaction 
//	(1/8 of the system's AUTOABORT timeout value).
//
//--------------------------------------------------------------------------//

void CRUDupElimTaskExecutor::InitTxnTimeLimit()
{
	// TEMPORARY CODE! MUST BE REPLACED WHEN THE AUTOABORT
	// IS SUPPORTED IN UOFS! THIS IS THE DEFAULT AUTOABORT VALUE (2 HOURS).
	txnTimeLimit_ = 2/*hours*/ * 3600/*seconds in hour*/ / 8/*fraction*/;

#ifdef _DEBUG
	CRUOptions &options = CRUGlobals::GetInstance()->GetOptions();
	if (NULL 
		!= 
		options.FindDebugOption(CRUGlobals::SHRINK_DE_TXN_TIMELIMIT, ""))
	{
		// Override the default timeout value
		txnTimeLimit_ = 0;
	}
#endif
}

//--------------------------------------------------------------------------//
//	FINITE-STATE MACHINE
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::PerformPhase()
//
//	Duplicate elimination is designed to be multi-phase. Each phase executes 
//	in a single transaction. 
//	
//	A single phase of DE scans a subset of the delta, ordered by the 
//	clustering key value, and eliminates the duplicates within this subset.
//	The next phase starts from the greatest CK value reached by the previous
//	one. The current phase is finished (and committed) if the whole delta is 
//	scanned, or the transaction's running time exceeds a certain fraction 
//	of the AUTOABORT system limit.
//
//	Typically, the DE's execution must not take more than one transaction.
//
//--------------------------------------------------------------------------//

void CRUDupElimTaskExecutor::PerformPhase()
{
	RUASSERT(GetState() == EX_PERFORM_PHASE && FALSE == IsTransactionOpen());

	if (phase_ <= 1)
	{
		PrepareSQL();
	}

	BeginTransaction();
	InitPhase();

	do 
	{
		// Read the next record from the delta
		pLogScanner_->Fetch();

		// Apply the resolvers
		Resolve();
	}
	while (FALSE == CanCompletePhase());

	pLogScanner_->EndScan();
	CommitTransaction();

	phase_++;

#ifdef _DEBUG
	char buf[10];
	sprintf(buf, "%d", phase_);

	// Simulate a breakdown upon phase completion
	TESTPOINT2(CRUGlobals::TESTPOINT151, CDSString(buf));
#endif

	if (TRUE == pLogScanner_->IsEntireDeltaScanned())
	{
		SetState(EX_EPILOGUE);
	}
	else
	{
		SetState(EX_PERFORM_PHASE);
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::ResetDECompleteFlag()
//	
//	Set the metadata variable: T.IS_LAST_DE_COMPLETE <-- 'N'
//
//	If Duplicate Elimination fails in the middle after committing a part
//	of its transactions, its next invocation (that starts with 
//	T.IS_LAST_DE_COMPLETE = 'N') must take this fact into account.
//
//	In this context, the range resolver cannot know for sure which ranges
//	have been written to the range log in the "new" epochs (i.e., those
//	greater than T.LAST_DE_EPOCH). See the code of CRURangeResolver for
//	details/
//
//--------------------------------------------------------------------------//

void CRUDupElimTaskExecutor::ResetDECompleteFlag()
{
	RUASSERT(GetState() == EX_START && FALSE == IsTransactionOpen());

	CRUDupElimTask *pTask =	(CRUDupElimTask *)GetParentTask();
	RUASSERT (NULL != pTask);
	CRUTbl &tbl = pTask->GetTable();

	BeginTransaction();
	tbl.SetLastDEComplete(FALSE);
	tbl.SaveMetadata();
	CommitTransaction();

	SetState(EX_PERFORM_PHASE);
	
	TESTPOINT(CRUGlobals::TESTPOINT150);
}

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::FinalMetadataUpdate()
//	
//	Set the metadata variables:
//		T.LAST_DE_EPOCH <-- The greatest scanned epoch
//		T.IS_LAST_DE_COMPLETE <-- 'Y'
//
//	T.LAST_DE_EPOCH is the last epoch in which all the clustering key values
//	have been processed by the DE algorithm.
//
//--------------------------------------------------------------------------//

void CRUDupElimTaskExecutor::FinalMetadataUpdate()
{
	RUASSERT(
		GetState() == EX_EPILOGUE 
		&& 
		FALSE == IsTransactionOpen()
	);

	CRUDupElimTask *pTask =	(CRUDupElimTask *)GetParentTask();
	RUASSERT (NULL != pTask);

	CRUTbl &tbl = pTask->GetTable();

	BeginTransaction();
	tbl.SetLastDEComplete(TRUE);

	// We only update the last DE epoch if there is a continuity in the DE.
	if (pTask->GetBeginEpoch() <= globals_.GetLastDEEpoch() + 1)
	{
		tbl.SetLastDupElimEpoch(pTask->GetEndEpoch());
	}

	tbl.SaveMetadata();
	CommitTransaction();	

	TESTPOINT(CRUGlobals::TESTPOINT152);

#ifdef _DEBUG
	DumpPerformanceStatistics();
#endif

	SetState(EX_COMPLETE);
}

//--------------------------------------------------------------------------//
//	EXECUTION CORE
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::Resolve()
//
//	Resolve() applies the single-row and range resolvers, respectively, 
//	to perform the duplicate elimination decisions and the corresponding 
//	write operations to the IUD and the range logs.
//	
//--------------------------------------------------------------------------//

void CRUDupElimTaskExecutor::Resolve()
{
	if (NULL != pRangeResolver_)
	{
		pRangeResolver_->Resolve(*pLogScanner_);
	}

	if (NULL != pSingleRowResolver_)
	{
		// The order is important! The range resolver must work first,
		// to save work to the single-row resolver.
		pSingleRowResolver_->Resolve(*pLogScanner_);
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::InitPhase()
//
//	Reset the units' state (in the phase greater than 0), 
//	and start performing the query.
//
//--------------------------------------------------------------------------//

void CRUDupElimTaskExecutor::InitPhase()
{
	// Start measuring the transaction's time
	StartTimer();

	if (phase_ > 0)
	{
		pLogScanner_->Reset();

		if (NULL != pRangeResolver_)
		{
			pRangeResolver_->Reset();
		}

		if (NULL != pSingleRowResolver_)
		{
			pSingleRowResolver_->Reset();
		}
	}

	// Start executing the query ...
	pLogScanner_->StartScan(phase_);
}

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::PrepareSQL()
//
//	Prepare the SQL statements required for the next phase.
//	
//	The log scanner will not compile the second query (the one with the 
//	lower bound) until it reaches the second phase.
//
//	For the resolvers, however, compiling the IUD statemnts in the lazy 
//	fashion is not applicable. The statement compilation happens in a 
//	different transaction. However, due to the CLI limitations, switching
//	to a different transaction in the middle of scan corrupts the scan's
//	context. This is why all the IUD statements must be prepared in advance
//	(before the first transaction).
//
//	All the compilations are under the CONTROL TABLE * MDAM ON setting, 
//	which allows the MDAM optimization of both IUD and SELECT statements.
//
//--------------------------------------------------------------------------//

void CRUDupElimTaskExecutor::PrepareSQL()
{
	//	CONTROL TABLE * MDAM 'ON'
	CDMPreparedStatement *pStmt = ctrlStmtContainer_.GetPreparedStatement(
		CRUDupElimConst::FORCE_MDAM_CT
	);
	pStmt->ExecuteUpdate(TRUE/* special syntax*/);
	pStmt->Close();

	switch (phase_)
	{
	case 0:
		{
			pLogScanner_->PrepareStatement(CRUDupElimConst::PHASE_0_QUERY);

			if (NULL != pSingleRowResolver_)
			{
				pSingleRowResolver_->PrepareSQL();
			}

			if (NULL != pRangeResolver_)
			{
				pRangeResolver_->PrepareSQL();
			}

			break;
		}
	case 1:
		{
			pLogScanner_->PrepareStatement(CRUDupElimConst::PHASE_1_QUERY);
			break;
		}

	default:	RUASSERT(FALSE);	// Not supposed to be called
	}

	//	CONTROL TABLE * RESET
	pStmt = ctrlStmtContainer_.GetPreparedStatement(
		CRUDupElimConst::RESET_MDAM_CT
	);
	pStmt->ExecuteUpdate(TRUE/* special syntax*/);
	pStmt->Close();
}

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::CanCompletePhase()
//	
//	The task executor decides whether to finish the current phase (and 
//	commit the transaction) in collaboration with the execution units.
//	The phase is over when one of the two condition holds:
//	(1) End of input - the entire delta is scanned (the common case).
//	(2) Intermediate cut:
//		(2.1) Both resolver agree to complete the phase.
//		(2.2) The execution has taken enough time to justify the commit.
//	
//	Note that the order between the checks 2.1 and 2.2 is important,
//	because the current time check is expensive. That's why it must not
//	be done too often. 
//	
//--------------------------------------------------------------------------//

BOOL CRUDupElimTaskExecutor::CanCompletePhase()
{
	if (TRUE == pLogScanner_->IsEntireDeltaScanned())
	{
		return TRUE;	// Case #1
	}

	BOOL rangeResolverOK=TRUE, singleRowResolverOK=TRUE;

	if (NULL != pRangeResolver_)
	{
		rangeResolverOK = pRangeResolver_->CanCompletePhase();
	}

	if (NULL != pSingleRowResolver_)
	{
		singleRowResolverOK = pSingleRowResolver_->CanCompletePhase();
		
		if (NULL != pRangeResolver_
			&&
			TRUE == pRangeResolver_->IsRangeCollectionEmpty())
		{
			// If the range resolver's collection is empty,
			// it actually does not care. The phase can be completed
			// if the single-row resolver wants to.
			rangeResolverOK = TRUE;
		}
	}

	if (FALSE == rangeResolverOK || FALSE == singleRowResolverOK)
	{
		return FALSE;	// Condition #2.1 violated
	}

	// It's okay with the resolvers. Measure the current timestamp.
	EndTimer();	

	// Condition #2.2
	// The ">=" relation is very important for simulating short 
	// transactions (when the timer duration can take less than 1 second).
	return (GetTimerDuration() >= txnTimeLimit_) ? TRUE : FALSE;
}

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExecutor::DumpPerformanceStatistics()
//
//	Print the number of IUD statements performed by the task.
//--------------------------------------------------------------------------//

#ifdef _DEBUG
void CRUDupElimTaskExecutor::DumpPerformanceStatistics()
{
	CRUOptions &options = CRUGlobals::GetInstance()->GetOptions();
	if (NULL != options.FindDebugOption(CRUGlobals::DUMP_DE_STATISTICS, ""))
	{
		char txnStr[10];
		sprintf(txnStr, "%d", phase_);

		CDSString str(
			"Task " + GetParentTask()->GetTaskName() + 
			" has been completed in "+CDSString(txnStr)+
			" transaction(s).\nIUD statements performed by the task:\n");
			
		if (NULL != pSingleRowResolver_)
		{
			pSingleRowResolver_->DumpPerformanceStatistics(str);
		}

		if (NULL != pRangeResolver_)
		{
			pRangeResolver_->DumpPerformanceStatistics(str);
		}	

		CRUJournal &journal = CRUGlobals::GetInstance()->GetJournal();
		journal.LogMessage(str);		
	}
}
#endif
