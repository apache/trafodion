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
* File:         RuLogCleanupTaskExecutor.cpp
* Description:  Implementation of class CRULogCleanupTaskExecutor.
*				
*
* Created:      09/25/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "dmprepstatement.h"
#include "dsstringlist.h"

#include "uofsIpcMessageTranslator.h"

#include "RuLogCleanupTaskExecutor.h"
#include "RuLogCleanupTask.h"
#include "RuGlobals.h"
#include "RuJournal.h"
#include "RuLogCleanupSQLComposer.h"

//--------------------------------------------------------------------------//
//	Constructor
//--------------------------------------------------------------------------//

CRULogCleanupTaskExecutor::CRULogCleanupTaskExecutor(CRUTask *pParentTask) :
	inherited(pParentTask),
	logCleanupTEDynamicContainer_(NUM_OF_SQL_STMT),
	hasRangeLog_(TRUE),
        noOfPartitions_(0)
{}

//--------------------------------------------------------------------------//
//	CRULogCleanupTaskExecutor::Init()
//
//	Initialize data members by analyzing the CleanupTask
//--------------------------------------------------------------------------//

void CRULogCleanupTaskExecutor::Init()
{
	inherited::Init();

	CRUTbl &tbl = GetLogCleanupTask()->GetTable();
	hasRangeLog_ = (CDDObject::eNONE != tbl.GetRangeLogType());
        noOfPartitions_ = tbl.getNumberOfPartitions();

	ComposeMySql();
}

//--------------------------------------------------------------------------//
//	CRULogCleanupTaskExecutor::StoreRequest()
//--------------------------------------------------------------------------//
void CRULogCleanupTaskExecutor::
	StoreRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::StoreRequest(translator);

	logCleanupTEDynamicContainer_.StoreData(translator);
	translator.WriteBlock(&hasRangeLog_,sizeof(BOOL));
	translator.WriteBlock(&noOfPartitions_,sizeof(Int32));

	translator.SetMessageType(
		CUOFsIpcMessageTranslator::RU_LOG_CLEANUP_EXECUTOR);
}

//--------------------------------------------------------------------------//
//	CRULogCleanupTaskExecutor::LoadRequest()
//--------------------------------------------------------------------------//
void CRULogCleanupTaskExecutor::
	LoadRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::LoadRequest(translator);

	logCleanupTEDynamicContainer_.LoadData(translator);
	translator.ReadBlock(&hasRangeLog_,sizeof(BOOL));
	translator.ReadBlock(&noOfPartitions_,sizeof(Int32));
}

//--------------------------------------------------------------------------//
//	CRULogCleanupTaskExecutor::ComposeMySql()
//--------------------------------------------------------------------------//

void CRULogCleanupTaskExecutor::ComposeMySql()
{
	CRULogCleanupSQLComposer myComposer(GetLogCleanupTask());

	myComposer.ComposeIUDLogCleanup(CLEAN_IUD_BASIC);
	logCleanupTEDynamicContainer_.
		SetStatementText(CLEAN_IUD_BASIC, myComposer.GetSQL());

	myComposer.ComposeIUDLogCleanup(CLEAN_IUD_FIRSTN);
	logCleanupTEDynamicContainer_.
		SetStatementText(CLEAN_IUD_FIRSTN, myComposer.GetSQL());

	myComposer.ComposeIUDLogCleanup(CLEAN_IUD_MCOMMIT);
	logCleanupTEDynamicContainer_.
		SetStatementText(CLEAN_IUD_MCOMMIT, myComposer.GetSQL());

	if (TRUE == hasRangeLog_)
	{
		myComposer.ComposeRangeLogCleanup();

		logCleanupTEDynamicContainer_.
			SetStatementText(CLEAN_RANGE,myComposer.GetSQL());
	}

	myComposer.composeGetRowCount();
	logCleanupTEDynamicContainer_.
		SetStatementText(CLEAN_ROWCOUNT, myComposer.GetSQL());
}

//--------------------------------------------------------------------------//
//	CRULogCleanupTaskExecutor::Work()
//
//	Main finite-state machine switch.
//--------------------------------------------------------------------------//

void CRULogCleanupTaskExecutor::Work()
{
	switch(GetState())
	{
		case EX_START:
			{
				RUASSERT(FALSE == IsTransactionOpen());
				Start();
				break;
			}

		case EX_CLEAN:
			{
				RUASSERT(FALSE == IsTransactionOpen());
				Clean();
				break;
			}
		case EX_EPILOGUE:
			{
				RUASSERT(FALSE == IsTransactionOpen());
				Epilogue();
				break;
			}
		default: RUASSERT(FALSE);
	}
}

//--------------------------------------------------------------------------//
//	CRULogCleanupTaskExecutor::Start()
//--------------------------------------------------------------------------//

void CRULogCleanupTaskExecutor::Start()
{
	CDSString msg;
	
	msg = RefreshDiags[17];
	msg += GetLogCleanupTask()->GetTable().GetFullName();
	msg += CDSString("...\n");
	
	CRUGlobals::GetInstance()->GetJournal().LogMessage(msg);
	
	SetState(EX_CLEAN);
}

//--------------------------------------------------------------------------//
//	CRULogCleanupTaskExecutor::Clean()
//
//	Delete the inapplicable data from the log
//--------------------------------------------------------------------------//

void CRULogCleanupTaskExecutor::Clean()
{
  SQL_STATEMENT deleteMethod = decideOnDeleteMethod();

  switch(deleteMethod)
  {
    case CLEAN_IUD_BASIC : 
      CleanLogBasic();
      break;

    case CLEAN_IUD_MCOMMIT :
      CleanLogMultiCommit();
      break;

    case CLEAN_IUD_FIRSTN :
      CleanLogFirstN(CLEAN_IUD_FIRSTN);
      break;
  }

  if (TRUE == hasRangeLog_)
  {
    CleanLogFirstN(CLEAN_RANGE);
  }
  
  SetState(EX_EPILOGUE);
}

//--------------------------------------------------------------------------//
//	CRULogCleanupTaskExecutor::decideOnDeleteMethod()
//
// A heuristic decision on which delete method to use.
// First we use this formula to get the average number of rows er partition:
//        rowCount * SF
//       ---------------
//           P * 100
// Where: rowCount is the number of rows in the IUD log (as an upper bound to 
//                 the number of rows to be deleted).
//        SF       is the safety factor from the defaults table (default is 50%).
//        P        is the number of partitions.
//
// If the number of rows per partition is smaller than the lock escalation 
// limit, we use a simple delete statement.
// Otherwise we use delete with multi commit, unless it is disabled by a CQD.
//--------------------------------------------------------------------------//
CRULogCleanupTaskExecutor::SQL_STATEMENT CRULogCleanupTaskExecutor::decideOnDeleteMethod()
{
  SQL_STATEMENT deleteMethod;

  BeginTransaction();

  CDSString safetyFactorName("MV_LOG_CLEANUP_SAFETY_FACTOR");
  TInt32 safetyFactor = 0;
  CRUCache::FetchSingleDefault( safetyFactorName, safetyFactor);

  CDSString useMultiCommitName("MV_LOG_CLEANUP_USE_MULTI_COMMIT");
  TInt32 useMultiCommit = 1;
  CRUCache::FetchSingleDefault( useMultiCommitName, useMultiCommit);

  if (safetyFactor <= 100)
  {
    // When the safty factor is set to 100 or less, lock escalation is not an issue,
    // so we can use simple delete no matter how many rows are to be deleted.
    deleteMethod = CLEAN_IUD_BASIC;

#ifdef _DEBUG
    CDSString msg("Safety factor set to 100 or less - using simple delete.");
    CRUGlobals::GetInstance()->
		LogDebugMessage(CRUGlobals::DUMP_COMPILED_DYNAMIC_SQL, "", msg, FALSE);
#endif
  }
  else
  {
    Int64 rowCount = getRowCount();
    TInt64 rowsPerPartition = rowCount * safetyFactor / (noOfPartitions_ * 100);

    if (rowsPerPartition < CRULogCleanupSQLComposer::MAX_ROW_TO_DELETE_IN_SINGLE_TXN)
      deleteMethod = CLEAN_IUD_BASIC;
    else if (useMultiCommit == 1)
      deleteMethod = CLEAN_IUD_MCOMMIT;
    else
      deleteMethod = CLEAN_IUD_FIRSTN;

#ifdef _DEBUG
    CDSString msg;
    char buff[200];
    sprintf(buff, "IUD log has " PF64 " rows, and %d partitions. Safety factor is %d. Using ", 
            rowCount, noOfPartitions_, safetyFactor);
    msg = buff; 

    switch(deleteMethod)
    {
      case CLEAN_IUD_BASIC:
        msg += "simple delete.\n";
        break;
      case CLEAN_IUD_FIRSTN:
        msg += "delete First N.\n";
        break;
      case CLEAN_IUD_MCOMMIT:
        msg += "delete with Multi Commit.\n";
        break;
    }
    CRUGlobals::GetInstance()->
		LogDebugMessage(CRUGlobals::DUMP_COMPILED_DYNAMIC_SQL, "", msg, FALSE);
#endif
  }

  CommitTransaction();

  return deleteMethod;
}

//--------------------------------------------------------------------------//
//	CRULogCleanupTaskExecutor::CleanLogBasic()
//
// No need to loop, but must start a transaction.
//--------------------------------------------------------------------------//

void CRULogCleanupTaskExecutor::CleanLogBasic()
{
  // Compilation Stage
  CDMPreparedStatement *pStat = 
    logCleanupTEDynamicContainer_.GetPreparedStatement(CLEAN_IUD_BASIC);

  BeginTransaction();

  ExecuteStatement(*pStat,
		    IDS_RU_LOG_CLEANUP_FAILED,
		    NULL, /* error argument */
		    TRUE /* Obtain row count */);

  CommitTransaction();

  TESTPOINT(CRUGlobals::TESTPOINT160); 
}

//--------------------------------------------------------------------------//
//	CRULogCleanupTaskExecutor::CleanLogFirstN()
//
// Need both the loop and the transactions.
//--------------------------------------------------------------------------//

void CRULogCleanupTaskExecutor::CleanLogFirstN(SQL_STATEMENT statement)
{
	// Compilation Stage
	CDMPreparedStatement *pStat = 
          logCleanupTEDynamicContainer_.GetPreparedStatement(statement);

	while (TRUE)
	{
		BeginTransaction();

		ExecuteStatement(*pStat,
				 IDS_RU_LOG_CLEANUP_FAILED,
				 NULL, /* error argument */
				 TRUE /* Obtain row count */);

		CommitTransaction();

		TESTPOINT(CRUGlobals::TESTPOINT160); 

		if (pStat->GetRowsAffected() < 
			CRULogCleanupSQLComposer::MAX_ROW_TO_DELETE_IN_SINGLE_TXN)
		{
			break;
		}
	}
}

//--------------------------------------------------------------------------//
// CRULogCleanupTaskExecutor::CleanLogMultiCommit()
//
// No need to loop or handle transactions.
//--------------------------------------------------------------------------//

void CRULogCleanupTaskExecutor::CleanLogMultiCommit()
{
  // Compilation Stage
  CDMPreparedStatement *pStat = 
    logCleanupTEDynamicContainer_.GetPreparedStatement(CLEAN_IUD_MCOMMIT);

  ExecuteStatement(*pStat,
		    IDS_RU_LOG_CLEANUP_FAILED,
		    NULL, /* error argument */
		    TRUE /* Obtain row count */);

  TESTPOINT(CRUGlobals::TESTPOINT160); 
}

//--------------------------------------------------------------------------//
// CRULogCleanupTaskExecutor::getRowCount()
//
// Find out the number of rows in the IUD log table.
//--------------------------------------------------------------------------//

TInt64 CRULogCleanupTaskExecutor::getRowCount()
{
  CDMPreparedStatement *pStat = 
    logCleanupTEDynamicContainer_.GetPreparedStatement(CLEAN_ROWCOUNT);

  ExecuteStatement(*pStat,
		    IDS_RU_LOG_CLEANUP_FAILED,
		    NULL, /* error argument */
		    TRUE, /* Obtain row count */
                    TRUE  /* isQuery */);

  CDMResultSet *resultSet = pStat->GetResultSet();
  resultSet->Next();
  TInt64 rowCount = resultSet->GetLargeInt(1);

  pStat->Close();

  return rowCount;
}

//--------------------------------------------------------------------------//
//	CRULogCleanupTaskExecutor::Epilogue()
//--------------------------------------------------------------------------//

void CRULogCleanupTaskExecutor::Epilogue()
{
	CRULogCleanupTask *pTask = GetLogCleanupTask();
	RUASSERT(NULL != pTask);
	CRUTbl &tbl = pTask->GetTable();

	// Update the T.MIN_EPOCH metadata variable
	BeginTransaction();
	tbl.SetMinLogEpoch(pTask->GetMaxInapplicableEpoch()+1);
	tbl.SaveMetadata();
	CommitTransaction();

	CDSString msg(RefreshDiags[18]);
	msg += GetLogCleanupTask()->GetTable().GetFullName();
	msg += CDSString(".\n");

	CRUGlobals::GetInstance()->GetJournal().LogMessage(msg);
	
	SetState(EX_COMPLETE);
}
