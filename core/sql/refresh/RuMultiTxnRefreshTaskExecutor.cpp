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
* File:         RuMultiTxnRefreshTaskExecutor.cpp
* Description:  Implementation of class CRUMultiTxnRefreshTaskExecutor.
*				
*
* Created:      08/17/2000
* Language:     C++
* 
*
* 
***************************************************************************
*/

#include "RuMultiTxnRefreshTaskExecutor.h"
#include "RuMultiTxnRefreshSQLComposer.h"
#include "RuTbl.h"
#include "RuGlobals.h"
#include "RuJournal.h"
#include "dsstringlist.h"
#include "dmresultset.h"
#include "uofsfile.h"
#include "uofsIpcMessageTranslator.h"
#include "uofsSystem.h"
#include "RuEmpCheck.h"
#include "RuEmpCheckVector.h"

#include "ddstatements.h"
#include "ComVersionDefs.h"



//--------------------------------------------------------------------------//
//	Constructor and destructor
//--------------------------------------------------------------------------//
CRUMultiTxnRefreshTaskExecutor::
CRUMultiTxnRefreshTaskExecutor(CRURefreshTask *pParentTask) :
	inherited(pParentTask),
	multiTxnRefreshTEDynamicContainer_(NUM_OF_SQL_STMT),
	context_(),
	txnCounter_(0),
	endEpoch_(0),
	beginEpoch_(0),
	catchupEpoch_(0),
	multiTxnTargetEpoch_(0)

{}

CRUMultiTxnRefreshTaskExecutor::
	~CRUMultiTxnRefreshTaskExecutor() 
{
};

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::Init()
//
//	Initialize data members by analyzing the RefreshTask
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshTaskExecutor::Init()
{
	inherited::Init();

	RUASSERT(TRUE == HasWork());

	if (FALSE == GetRefreshTask()->NeedToExecuteInternalRefresh())
	{
		return;
	}

	CRUMV &mv = GetRootMV();

	// We only support a single delta multi-txn refresh 
	// so the tblList contain only a single delta
	CRUTblList &tblList = mv.GetTablesUsedByMe();

	RUASSERT (1 == tblList.GetCount());

	CRUTbl *pTbl = tblList.GetAt(0);

	// END_EPOCH ,END_EPOCH will never change during this refresh invocation
	endEpoch_ = pTbl->GetCurrentEpoch() - 1;

	// BEGIN_EPOCH , BEGIN_EPOCH may be changed during this refresh invocation
	beginEpoch_ = mv.GetEpoch(pTbl->GetUID());

	if (TRUE == mv.IsMultiTxnContext())
	{
		// If we are in a catchup mode , multiTxnTargetEpoch_ will not be zero
		multiTxnTargetEpoch_ = GetRootMV().GetMultiTxnTargetEpoch();
	}

	// We must synchronize between the table and the mv, so we need to 
	// lock the table partitions
	// Here we copy the partitions file names in order to allow access to 
	// the files in the remote process when DDOL is not built
	tableLockProtocol_ = new CRUTableLockProtocol();
	tableLockProtocol_->Init(mv.GetTablesUsedByMe(), 
				 GetRefreshTask()->GetDeltaDefList());

	ComposeMySql();
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::UpdateTargetEpoch()
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshTaskExecutor::UpdateTargetEpoch(TInt32 epoch)
{
  // VO, metadata versioning: 
  // Figure out what the real schema version is, rather than using a hard-coded value
  CDSString QueryText = "SELECT VERSION FROM TABLE (VERSION_INFO ('SCHEMA',";
#ifdef NA_NSK 
  QueryText += "'";
#else 
  QueryText += "_UTF8'";
#endif
  QueryText += GetRootMVSchema().ConvertToQuotedString(FALSE) + "'))";

  CDMConnection connection;
  CDMPreparedStatement *pPrepStmt;
  CDMResultSet *pResult;

#ifdef NA_NSK 
  pPrepStmt = connection.PrepareStmtWithCharSet(QueryText, SQLCHARSETCODE_ISO_MAPPING);
#else 
  pPrepStmt = connection.PrepareStmtWithCharSet(QueryText, SQLCHARSETCODE_UTF8);
#endif
  pResult = pPrepStmt->ExecuteQuery();
  pResult->Next();

  // Get the one and only column value
  COM_VERSION schemaVersion = (COM_VERSION) pResult->GetInt(1);
  char schemaVersionAsString [DIGITS_IN_VERSION_NUMBER+1];
  VersionToString (schemaVersion,schemaVersionAsString);


  CRUSQLStaticStatementContainer staticSqlContainer(1);

  staticSqlContainer.SetStatement (UPDATE_CTX,
                                   kStaticStmtArray[kMultiTxnTargetEpochUpdate],
                                   schemaVersion);

  CDMPreparedStatement *pStat =
  staticSqlContainer.GetPreparedStatement(UPDATE_CTX);

#ifdef NA_NSK 
  pStat->SetString(1, GetRootMVSchema() + ".MVS_USED_UMD" );
#else 
  CDSString table1name(GetRootMVSchema() + ".MVS_USED_UMD");
  pStat->SetWString(1, CDSCharInfo::ConvStrFromInfCSToUTF16(table1name) );
#endif
  pStat->SetInt(2, epoch);
  pStat->SetLargeInt(3, GetRootMVUID());
#ifdef NA_NSK 
  pStat->SetString(4, GetRootMVCatalog() + ".HP_DEFINITION_SCHEMA.MVS_USED" );
#else 
  CDSString table4name(GetRootMVCatalog() + ".HP_DEFINITION_SCHEMA.MVS_USED");
  pStat->SetWString(4, CDSCharInfo::ConvStrFromInfCSToUTF16(table4name) );
#endif
  pStat->SetLargeInt(5, GetRootMVUID());

  try
  {
    // Need an exception handler around this update because it is generated as non-retryable.
    // In reality, it is retryable, but we have to handle that ourselves. 
    pStat->ExecuteUpdate(TRUE);
  }
  catch (CDMException &eDm)
  {
    if (eDm.GetErrorCode (0) == -VERSION_PLAN_VERSION_ERROR_NON_RETRYABLE_STATEMENT)
      // A retryable versioning error - re-execute to get the statement recompiled
      pStat->ExecuteUpdate(TRUE);
    else
      throw;
  }

  pStat->Close();
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::ComposeMySql()
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshTaskExecutor::ComposeMySql()
{
	CRUMultiTxnRefreshSQLComposer myComposer(GetRefreshTask());

	myComposer.ComposeRefresh(PHASE_0,DONT_NEED_CATCHUP); 
	
	multiTxnRefreshTEDynamicContainer_.SetStatementText
				(NO_BOUNDS_REFRESH,myComposer.GetSQL());


	myComposer.ComposeRefresh(PHASE_1,DONT_NEED_CATCHUP); 
	
	multiTxnRefreshTEDynamicContainer_.SetStatementText
				(LOWER_BOUND_REFRESH,myComposer.GetSQL());

	myComposer.ComposeRefresh(PHASE_0,NEED_CATCHUP); 
	
	multiTxnRefreshTEDynamicContainer_.SetStatementText
				(UPPER_BOUND_REFRESH,myComposer.GetSQL());

	myComposer.ComposeRefresh(PHASE_1,NEED_CATCHUP); 
	
	multiTxnRefreshTEDynamicContainer_.SetStatementText
				(BOTH_BOUND_REFRESH,myComposer.GetSQL());

	myComposer.ComposeReadContextLog();

	multiTxnRefreshTEDynamicContainer_.SetStatementText
				(READ_CONTEXT_LOG,myComposer.GetSQL());

	if (NULL != GetRefreshTask()->GetRootMV().GetMVForceOption())
	{
		myComposer.ComposeCQSForIRPhase1();

		multiTxnRefreshTEDynamicContainer_.SetStatementText
					(CQS_PHASE1,myComposer.GetSQL());
	}
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::Work()
//
//	Main finite-state machine switch.
// 
//	Implementation of multi-txn fsm macine.See Refresh-Design.doc
//  for detailed explanation
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshTaskExecutor::Work()
{
	switch (GetState())
	{
	case EX_START: // MAIN PROCESS
		{
			// Write the opening message
			Start();
			break;
		}
	case EX_PROLOGUE: // REMOTE PROCESS
		{
			// Reads the context table umd and adds a context record to it
			Prologue();
			break;
		}
	case EX_NO_BOUNDS: // REMOTE PROCESS
		{
			NoBounds();
			break;
		}
	case EX_UPPER_BOUND: // REMOTE PROCESS
		{
			UpperBound();
			break;
		}
	case EX_LOWER_BOUND: // REMOTE PROCESS
		{
			LowerBound();
			break;
		}
	case EX_BOTH_BOUND: // REMOTE PROCESS
		{
			BothBounds();
			break;
		}
	case EX_REMOTE_END: // REMOTE PROCESS
		{
			RemoteEnd();
			break;
		}
	case EX_EPILOGUE: // MAIN PROCESS 
		{
			Epilogue();
			break;
		}
	default: RUASSERT(FALSE);
	}
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::Start()
//
//	Implementation of the START phase see Refresh-Design.doc
//  for detailed explanation
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshTaskExecutor::Start()
{
	RUASSERT(FALSE == IsTransactionOpen());

	LogOpeningMessage();

	StartTimer();

	if (FALSE == GetRefreshTask()->NeedToExecuteInternalRefresh())
	{
		// There is no delta , we only need to update the metadata 
		SetState(EX_EPILOGUE);
	}
	else
	{
		SetState(EX_PROLOGUE);
	}
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::prologue()
//
//	Implementation of the PROLOGUE phase see Refresh-Design.doc
//  for detailed explanation
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshTaskExecutor::Prologue()
{
	RUASSERT(FALSE == IsTransactionOpen());

	TESTPOINT2(CRUGlobals::TESTPOINT120, GetRootMVName());

	BeginTransaction();

	if (FALSE == StartTableLockProtocol() )
	{
		CommitTransaction();
		SetState(EX_REMOTE_END);
		return;
	}

	if (0 < multiTxnTargetEpoch_)
	{
		// There are context rows from previous failures,We should initiate a
		// recovery process by doing internal refresh catchup phases 
		SetState(EX_UPPER_BOUND);
		// BEGIN_EPOCH may changed if we are after previous failures
		beginEpoch_ = multiTxnTargetEpoch_ + 1;

		CDMPreparedStatement *pStat = 
			multiTxnRefreshTEDynamicContainer_.GetPreparedStatement(READ_CONTEXT_LOG);

		context_.ReadRowsFromContextLog(pStat);
	}
	else
	{
		// There are no previous failures
		SetState(EX_NO_BOUNDS);
	}
	
	// Simulating the IR behaviour
	context_.Push(beginEpoch_);
}


//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::NoBounds()
//
//	Implementation of the NO_BOUNDS phase see Refresh-Design.doc
//  for detailed explanation
//
//  Executing Internal refresh statemet phase 0;
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshTaskExecutor::NoBounds()
{
	RUASSERT(TRUE == IsTransactionOpen());

	CDMPreparedStatement *pStat
				= CompileRefresh(NO_BOUNDS_REFRESH,beginEpoch_,endEpoch_);

	// Decouple the prepare and execute to separate transactions.
	// Don't count the prepare transaction.
	CommitTransaction(FALSE);
	BeginTransaction();
	
	LOGTIME("Starting to execute multi-txn refresh \n");
	pStat->ExecuteUpdate(TRUE);
	pStat->Close();

	// We may need more txn to complete
	SetState(EX_LOWER_BOUND);

	// Simulate a system error at this point!
	TESTPOINT_SEVERE2(CRUGlobals::SEVERE_REFRESH_CRASH, GetRootMVName());

	UpdateTargetEpoch(endEpoch_);

	ExecuteShowExplain();

	CommitTransaction();
	
	TESTPOINT2(CRUGlobals::TESTPOINT125, GetRootMVName());
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::LowerBound()
//
//	Implementation of the LOWER_BOUND phase see Refresh-Design.doc
//  for detailed explanation
//
//  Executing Internal refresh statemet phase 1;
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshTaskExecutor::LowerBound()
{
	RUASSERT(FALSE == IsTransactionOpen());

	TESTPOINT2(CRUGlobals::TESTPOINT121, GetRootMVName());

	// Execute Internal refresh statement until we recieve no more 
	// transactions needed code
	DoIRefreshUntilIDone(LOWER_BOUND_REFRESH);

	SetState(EX_REMOTE_END);
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::UpperBound()
//
//	Implementation of the UPPER_BOUND phase see Refresh-Design.doc
//  for detailed explanation
//
//  Executing Internal refresh statemet phase 0 with catchup clause;
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshTaskExecutor::UpperBound()
{
	RUASSERT(TRUE == IsTransactionOpen());

	CDMPreparedStatement *pStat;

	// The second row always contain the begin epoch
	catchupEpoch_ = context_.GetTargetEpochOfSecondRow();

	if (CRUMultiTxnContext::ROW_DOES_NOT_EXIST == catchupEpoch_)
	{
		// This is a special case that is named "Clean Failure".
		// I means that the previous invocation of refresh utility failed 
		// after completed all the refresh process and before making the SMD
		// updates and therfore the context rows are not deleted from the 
		// context table.
		// All we need to do is to apply all records from the previous epoch
		// increment and update the smd table
		SetState(EX_NO_BOUNDS);
		return;
	}
	
	
	pStat = CompileRefresh(UPPER_BOUND_REFRESH,
				beginEpoch_,
				endEpoch_,
				catchupEpoch_);

	// Decouple the prepare and execute to separate transactions.
	// Don't count the prepare transaction.
	CommitTransaction(FALSE);
	BeginTransaction();

	LOGTIME("Starting to execute multi-txn refresh \n");
	pStat->ExecuteUpdate(TRUE);
	pStat->Close();
	
	UpdateTargetEpoch(endEpoch_);

	CommitTransaction();
	
	SetState(EX_BOTH_BOUND);
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::BothBounds()
//
//	Implementation of the BOTH_BOUNDS phase see Refresh-Design.doc
//  for detailed explanation
//
//  Executing Internal refresh statemet phase 1 with catchup clause;
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshTaskExecutor::BothBounds()
{
	RUASSERT(FALSE == IsTransactionOpen());

	
	TESTPOINT2(CRUGlobals::TESTPOINT122, GetRootMVName());

	// If this is the only line in the context , the begin epoch will be taken
	// from this row

	beginEpoch_     = context_.GetTargetEpochOfFirstRow();
	
	RUASSERT (CRUMultiTxnContext::ROW_DOES_NOT_EXIST != beginEpoch_);

	catchupEpoch_ = context_.GetTargetEpochOfSecondRow();

	if (CRUMultiTxnContext::ROW_DOES_NOT_EXIST == catchupEpoch_)
	{
		SetState(EX_LOWER_BOUND);
		return;
	}

	DoIRefreshUntilIDone(BOTH_BOUND_REFRESH);
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::RemoteEnd()
//
//	Implementation of the REMOTE_END phase see Refresh-Design.doc
//  for detailed explanation
//
//  Unlock used table if necessary  
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshTaskExecutor::RemoteEnd()
{
	RUASSERT(FALSE == IsTransactionOpen());

	EndTableLockProtocol();
	SetState(EX_EPILOGUE);
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::Epilogue()
//
//	Implementation of the EPILOGUE phase see Refresh-Design.doc
//  for detailed explanation
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshTaskExecutor::Epilogue()
{
	RUASSERT(FALSE == IsTransactionOpen());

	TESTPOINT2(CRUGlobals::TESTPOINT123, GetRootMVName());

	BeginTransaction();	

	EndTimer();

	FinalMetadataUpdate();
	
	CommitTransaction();
	
	TESTPOINT2(CRUGlobals::TESTPOINT124, GetRootMVName());
	
	LogClosureMessage();
	
	SetState(EX_COMPLETE);
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::DoIRefreshUntilIDone()
//
// Compiles the IRefresh statement and then execute it again until 
// a no more txn needed error is thrown
// This function is called by BothBounds() and LowerBound()
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshTaskExecutor::
	DoIRefreshUntilIDone(SQL_STATEMENT type)
{
	RUASSERT(LOWER_BOUND_REFRESH == type || BOTH_BOUND_REFRESH == type);
	RUASSERT(FALSE == IsTransactionOpen());

	CDMPreparedStatement *pStat;

	BeginTransaction();
	pStat = CompileRefresh(type,
				beginEpoch_,
				endEpoch_,
				catchupEpoch_);
	// Decouple the prepare and execute to separate transactions.
	// Don't count the prepare transaction.
	CommitTransaction(FALSE);

	do 
	{
		BeginTransaction();	

		try
		{
			LOGTIME("Starting to execute multi-txn refresh \n");
			pStat->ExecuteUpdate(TRUE);
			
			pStat->Close();
			
			CommitTransaction();
		}
		catch (CDSException &ex)
		{
			pStat->Close();
			
			// ReThrow the exception if another error besides 
			// NO_MORE_TXN_CODE_ERROR occured
			VerifyMultiTxnError(ex);

			RollbackTransaction();
			
			// End of catchup phase
			context_.Pop();
			
			return;
		}

#ifdef _DEBUG
		// This section is only for the regression test

		if ( EX_LOWER_BOUND == GetState()) 
		{
			TESTPOINT2(CRUGlobals::TESTPOINT126, GetRootMVName());
		}
		else
		{
			TESTPOINT2(CRUGlobals::TESTPOINT127, GetRootMVName());
		}

#endif
	} while (1);
}


//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::VerifyMultiTxnError()
//
// This function verifies that the only error that occured is 
// NO_MORE_TXN_CODE_ERROR otherwise it ReThrow the exception
//--------------------------------------------------------------------------//
void CRUMultiTxnRefreshTaskExecutor::VerifyMultiTxnError(CDSException &ex)
{
	for (Int32 i=0;i<ex.GetNumErrors();i++)
	{
		if (ex.GetErrorCode(i) < 0 && ex.GetErrorCode(i) != NO_MORE_TXN_CODE_ERROR)
		{
			throw ex;
		}
	}
}


//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::CompileRefresh()
//
// This function compiles an Internal refresh statment 
// with the actual params
//--------------------------------------------------------------------------//

CDMPreparedStatement * CRUMultiTxnRefreshTaskExecutor::
	CompileRefresh(SQL_STATEMENT type, 
				   TInt32 beginEpoch, 
				   TInt32 endEpoch,
				   TInt32 catchupEpoch )
{
	CDMPreparedStatement *pStat;
	
	// set the begin epoch
	multiTxnRefreshTEDynamicContainer_.SetCompiledTimeParam
			(type,0,CRUSQLComposer::TInt32ToStr(beginEpoch));
	
	// set the end epoch
	multiTxnRefreshTEDynamicContainer_.SetCompiledTimeParam
			(type,1,CRUSQLComposer::TInt32ToStr(endEpoch));
	
	if (UPPER_BOUND_REFRESH == type || BOTH_BOUND_REFRESH == type )
	{
		// set the catchup epoch
		multiTxnRefreshTEDynamicContainer_.SetCompiledTimeParam
			(type,2,CRUSQLComposer::TInt32ToStr(catchupEpoch));
	}

	LOGTIME("Starting to compile multi-txn refresh \n");

	try
	{
		ApplyIRCompilerDefaults();

		// Compilation Stage
		pStat = multiTxnRefreshTEDynamicContainer_.
						GetPreparedStatement(type);
	
		ResetIRCompilerDefaults();
		
		// Simulate a system error at this point!
		TESTPOINT_SEVERE2(CRUGlobals::SEVERE_PREPARE_CRASH, GetRootMVName());
	}
	catch (CDSException &ex)
	{
		ex.SetError(IDS_RU_IREFRESH_FAILED);	
		ex.AddArgument(multiTxnRefreshTEDynamicContainer_.
									GetLastSQL(type));
		throw ex;	// Re-throw
	}
	catch (...)
	{
		CRUException ex;

		ex.SetError(IDS_RU_IREFRESH_FAILED);	
		ex.AddArgument(multiTxnRefreshTEDynamicContainer_.
									GetLastSQL(type));
		throw ex;	// Re-throw
	}

	return pStat;
}

//--------------------------------------------------------------------------//
// CRUMultiTxnRefreshTaskExecutor::ApplyCQSForInternalRefresh() 
//--------------------------------------------------------------------------//
void CRUMultiTxnRefreshTaskExecutor::ApplyCQSForInternalRefresh()
{
	if (EX_LOWER_BOUND == GetState() || EX_BOTH_BOUND == GetState())
	{
		// Internal refresh phase 1, use a special CQS statement
		CDMPreparedStatement *pStat = multiTxnRefreshTEDynamicContainer_.
						GetPreparedStatement(CQS_PHASE1);

		ExecuteStatement(*pStat, CQS_PHASE1);

		CRUGlobals::GetInstance()->GetJournal().LogMessage(
			multiTxnRefreshTEDynamicContainer_.GetLastSQL(CQS_PHASE1));

		
	}
	else
	{
		// Internal refresh phase 0, use the default CQS statement
		inherited::ApplyCQSForInternalRefresh();
	}
}

//--------------------------------------------------------------------------//
// CRUMultiTxnRefreshTaskExecutor::LogOpeningMessage() 
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshTaskExecutor::LogOpeningMessage() 
{
	CDSString msg;

	GenOpeningMessage(msg);

	msg += RefreshDiags[14] + CDSString("\n");
	
	CRUGlobals::GetInstance()->GetJournal().LogMessage(msg);
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::LogClosureMessage()
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshTaskExecutor::LogClosureMessage()
{
	CDSString msg;

	GenClosureMessage(msg);
	
	if (TRUE == GetRefreshTask()->NeedToExecuteInternalRefresh())
	{
		msg += RefreshDiags[15];
		msg += CRUSQLComposer::TInt32ToStr(txnCounter_);
		msg += RefreshDiags[16] + CDSString("\n");
	}
	
	CRUGlobals::GetInstance()->GetJournal().LogMessage(msg);
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::StoreRequest()
//--------------------------------------------------------------------------//
void CRUMultiTxnRefreshTaskExecutor::
	StoreRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::StoreRequest(translator);
	
	// Handle multi-txn sql dynamic container
	multiTxnRefreshTEDynamicContainer_.StoreData(translator);
		
	// Handle multi-txn data executor members
	translator.WriteBlock(&endEpoch_, sizeof(TInt32));
	translator.WriteBlock(&beginEpoch_, sizeof(TInt32));
	translator.WriteBlock(&catchupEpoch_, sizeof(TInt32));
	translator.WriteBlock(&multiTxnTargetEpoch_, sizeof(TInt32));
	translator.WriteBlock(&txnCounter_, sizeof(short));

	translator.SetMessageType(CUOFsIpcMessageTranslator::
		RU_MULTI_TXN_REFRESH_EXECUTOR);
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshTaskExecutor::StoreReply()
//--------------------------------------------------------------------------//
void CRUMultiTxnRefreshTaskExecutor::
	StoreReply(CUOFsIpcMessageTranslator &translator)
{
	inherited::StoreReply(translator);

	// Handle multi-txn data executor members
	translator.WriteBlock(&endEpoch_, sizeof(TInt32));
	translator.WriteBlock(&beginEpoch_, sizeof(TInt32));
	translator.WriteBlock(&catchupEpoch_, sizeof(TInt32));
	translator.WriteBlock(&multiTxnTargetEpoch_, sizeof(TInt32));
	translator.WriteBlock(&txnCounter_, sizeof(short));

}

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::LoadRequest()
//--------------------------------------------------------------------------//
void CRUMultiTxnRefreshTaskExecutor::
	LoadRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::LoadRequest(translator);

	// Handle multi-txn sql dynamic container
	multiTxnRefreshTEDynamicContainer_.LoadData(translator);

	// Handle multi-txn data executor members
	translator.ReadBlock(&endEpoch_, sizeof(TInt32));
	translator.ReadBlock(&beginEpoch_, sizeof(TInt32));
	translator.ReadBlock(&catchupEpoch_, sizeof(TInt32));
	translator.ReadBlock(&multiTxnTargetEpoch_, sizeof(TInt32));
	translator.ReadBlock(&txnCounter_, sizeof(short));
}

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::LoadReply()
//--------------------------------------------------------------------------//
void CRUMultiTxnRefreshTaskExecutor::
	LoadReply(CUOFsIpcMessageTranslator &translator)
{
	inherited::LoadReply(translator);

	// Handle multi-txn data executor members
	translator.ReadBlock(&endEpoch_, sizeof(TInt32));
	translator.ReadBlock(&beginEpoch_, sizeof(TInt32));
	translator.ReadBlock(&catchupEpoch_, sizeof(TInt32));
	translator.ReadBlock(&multiTxnTargetEpoch_, sizeof(TInt32));
	translator.ReadBlock(&txnCounter_, sizeof(short));
}

