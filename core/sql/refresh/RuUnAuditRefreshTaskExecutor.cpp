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
* File:         RuUnAuditRefreshTaskExecutor.cpp
* Description:  Implementation of class CRUUnAuditRefreshTaskExecutor.
*				
*
*
* Created:      08/14/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuUnAuditRefreshTaskExecutor.h"
#include "RuSimpleRefreshSQLComposer.h"
#include "RuGlobals.h"
#include "RuTbl.h"
#include "uofsIpcMessageTranslator.h"
#include "ddindex.h"

//--------------------------------------------------------------------------//
//	Constructor and destructor
//--------------------------------------------------------------------------//

CRUUnAuditRefreshTaskExecutor::
CRUUnAuditRefreshTaskExecutor(CRURefreshTask *pParentTask) :
	inherited(pParentTask),
	unAuditRefreshTEDynamicContainer_(NUM_OF_SQL_STMT),
	pUnAuditPopIndexdynamicContainer_(NULL),
	pUnAuditAvailableIndeXdynamicContainer_(NULL),
	pUnAuditUnavailableIndeXdynamicContainer_(NULL),
	pLockTablesTEDynamicContainer_(NULL),
	isPurgedata_(FALSE),
	isPopindex_(FALSE),
	numOfIndexes_(0)
{}	

CRUUnAuditRefreshTaskExecutor::~CRUUnAuditRefreshTaskExecutor() 
{
	delete pLockTablesTEDynamicContainer_;
	delete pUnAuditPopIndexdynamicContainer_;
	delete pUnAuditAvailableIndeXdynamicContainer_;
	delete pUnAuditUnavailableIndeXdynamicContainer_;
}
	
//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::Init()
//
//	Initialize data members by analyzing the RefreshTask
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::Init()
{
	inherited::Init();

	RUASSERT(TRUE == HasWork());

	if (FALSE == GetRefreshTask()->NeedToExecuteInternalRefresh())
	{
		return;
	}

	Lng32 refreshPattern = GetRootMV().GetRefreshPatternMap();
	isPurgedata_ = (0 != (refreshPattern & CRUMV::PURGEDATA));
	isPopindex_ = (0 != (refreshPattern & CRUMV::POPINDEX));

	// Compose the INTERNAL REFRESH 
	// + (optionally) the PopIndex CatApi request statements
	// + (optionally) the LOCK TABLE statements for ON STATEMENT MV
	ComposeMySql();
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::Work()
//
//	Main finite-state machine switch.
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::Work()
{
	switch (GetState())
	{
	case EX_START: // MAIN PROCESS
		{
			RUASSERT(FALSE == IsTransactionOpen());
			Start();
			break;
		}
	case EX_PROLOGUE: // MAIN PROCESS
		{
			RUASSERT(FALSE == IsTransactionOpen());
			// Turn the MV (+ optionally its indexes) non-available
			Prologue();
			break;
		}
	case EX_PURGE_DATA: // MAIN PROCESS
		{
			RUASSERT(FALSE == IsTransactionOpen());
			// Execute purgedata from the MV (with indexes)
			PurgeData();			
			break;
		}
	case EX_REFRESH: // REMOTE PROCESS
		{
			RUASSERT(FALSE == IsTransactionOpen());
			// Execute the incremental INTERNAL REFRESH
			Refresh();
			break;
		}
	case EX_RECOMPUTE: // REMOTE PROCESS
		{
			// Execute INTERNAL REFRESH RECOMPUTE
			Recompute();
			break;
		}
	case EX_POPINDEX:	// REMOTE PROCESS
		{
			RUASSERT(FALSE == IsTransactionOpen());
			PopulateIndexes();
			break;
		}
	case EX_EPILOGUE:	// MAIN PROCESS
		{
			RUASSERT(FALSE == IsTransactionOpen());
			// Turn the MV (+ optionally indexes) back to available
			// Perform the final metadata update
			Epilogue();
			break;
		}
	default: RUASSERT(FALSE);
	}
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::StoreRequest()
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::
	StoreRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::StoreRequest(translator);

	translator.WriteBlock(&isPurgedata_, sizeof(BOOL));
	translator.WriteBlock(&isPopindex_, sizeof(BOOL));

	// Store the executor-specific SQL container
	unAuditRefreshTEDynamicContainer_.StoreData(translator);

	translator.WriteBlock(&numOfIndexes_, sizeof(TInt32));

	if (0 < numOfIndexes_)
	{
		pUnAuditPopIndexdynamicContainer_->StoreData(translator);
		pUnAuditAvailableIndeXdynamicContainer_->StoreData(translator);
		pUnAuditUnavailableIndeXdynamicContainer_->StoreData(translator);
	}

	translator.SetMessageType(
		CUOFsIpcMessageTranslator::RU_UNAUDIT_REFRESH_EXECUTOR
	);
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::LoadRequest()
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::
	LoadRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::LoadRequest(translator);

	translator.ReadBlock(&isPurgedata_, sizeof(BOOL));
	translator.ReadBlock(&isPopindex_, sizeof(BOOL));

	// Store the executor-specific SQL container
	unAuditRefreshTEDynamicContainer_.LoadData(translator);

	translator.ReadBlock(&numOfIndexes_, sizeof(TInt32));

	if (0 < numOfIndexes_)
	{
		pUnAuditPopIndexdynamicContainer_ = 
			new CRUSQLDynamicStatementContainer(numOfIndexes_);
		pUnAuditAvailableIndeXdynamicContainer_ =
			new CRUSQLDynamicStatementContainer(numOfIndexes_);
		pUnAuditUnavailableIndeXdynamicContainer_ = 
			new CRUSQLDynamicStatementContainer(numOfIndexes_);

		pUnAuditPopIndexdynamicContainer_->LoadData(translator);
		pUnAuditAvailableIndeXdynamicContainer_->LoadData(translator);
		pUnAuditUnavailableIndeXdynamicContainer_->LoadData(translator);
	}
}

//--------------------------------------------------------------------------//
//	PRIVATE AREA
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::Start()
//
//	Implementation of EX_START state
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::Start()
{
	LogOpeningMessage();

	StartTimer();
	
	if (FALSE == GetRefreshTask()->NeedToExecuteInternalRefresh())
	{
		SetState(EX_EPILOGUE);
	}
	else
	{
		SetState(EX_PROLOGUE);
	}
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::Prologue()
//
//	Implementation of EX_PROLOGUE state
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::Prologue()
{
	TESTPOINT2(CRUGlobals::TESTPOINT101, GetRootMVName());

	BeginTransaction();
	if (CDDObject::eON_STATEMENT == GetRootMVType())
	{
		PrologueHandleOnStatementMV();
	}
	// Set the MV non-available etc ...
	SetObjectsUnavailable();
	CommitTransaction();

	TESTPOINT2(CRUGlobals::TESTPOINT102, GetRootMVName());

	if (TRUE == isPurgedata_) 
	{
		SetState(EX_PURGE_DATA);
	}
	else if (TRUE == IsRecompute())
	{
		SetState(EX_RECOMPUTE);
	}
	else
	{
		SetState(EX_REFRESH);
	}
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::Refresh()
//
//	Implementation of EX_REFRESH state
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::Refresh()
{
	try 
	{
		// ?????
		// Temporary - Should be removed when read commited from unaudited table is implemented
		BeginTransaction(); 

		ApplyIRCompilerDefaults();

		if(TRUE == IsSingleDeltaRefresh())
		{
			CDMPreparedStatement *pStat = PrepareSingleDeltaRefresh();

			// ?????
			// Temporary - Should be removed when read commited from unaudited table is implemented
			CommitTransaction();
			BeginTransaction();

			ExecuteSingleDeltaRefresh(pStat);
		}
		else
		{
			StmtList *stmts = PrepareMultiDeltasRefresh();

			// ?????
			// Temporary - Should be removed when read commited from unaudited table is implemented
			CommitTransaction();
			BeginTransaction();

			ExecuteMultiDeltasRefresh(stmts);
		}

		ResetIRCompilerDefaults();

		ExecuteShowExplain();

		// ?????
		// Temporary - Should be removed when read commited from unaudited table is implemented
		CommitTransaction();

		TESTPOINT2(CRUGlobals::TESTPOINT106, GetRootMVName());
		
		SetState(EX_EPILOGUE);
	}
	catch (CRUSimpleRefreshTaskExecutor::NeedRecomputeException)
	{
		SetState(EX_RECOMPUTE);
		SetRecompute(TRUE);
	}
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::PurgeData()
//
//	Implementation of EX_PURGE_DATA state
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::PurgeData()
{
#ifdef _DEBUG
	CDSString msg(
		"\nPurging the data from materialized view " 
		+ 
		GetRootMVName());
	
	if (TRUE == isPopindex_)
	{
		msg += " and its secondary indexes";
	}

	msg += "...\n";
	CRUGlobals::GetInstance()->
		LogDebugMessage(CRUGlobals::DUMP_PURGEDATA,"",msg);
#endif

	GetRootMV().PurgeDataWithIndexes();

	TESTPOINT2(CRUGlobals::TESTPOINT103, GetRootMVName());

	SetState(EX_RECOMPUTE);
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::Recompute()
//
//	Implementation of EX_RECOMPUTE state
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::Recompute()
{
	// ?????
	// Temporary - Should be removed when read commited from unaudited table is implemented
	if (FALSE == IsTransactionOpen())
	{
		BeginTransaction(); 
	}
	
	CDMPreparedStatement *pStat = PrepareRecomputeMV();

	// ?????
	// Temporary - Should be removed when read commited from unaudited table is implemented
	CommitTransaction();
	BeginTransaction();

	ExecuteRecomputeMV(pStat);

	// ?????
	// Temporary - Should be removed when read commited from unaudited table is implemented
	CommitTransaction();

	TESTPOINT2(CRUGlobals::TESTPOINT104, GetRootMVName());

	if (TRUE == isPopindex_)
	{
		SetState(EX_POPINDEX);
	}
	else
	{
		SetState(EX_EPILOGUE);
	}
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::PopulateIndexes()
//
//	Apply the pre-composed CatApi request code to populate the indexes
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::PopulateIndexes()
{
	RUASSERT(TRUE == isPopindex_);

	if (0 == numOfIndexes_)
	{
		return;
	}

	// ?????
	// Temporary - Should be removed when read commited from unaudited table is implemented
	BeginTransaction();

#ifdef _DEBUG
	CDSString msg(
		"\nPopulating the secondary indexes of materialized view " 
		+ GetRootMVName() + "...\n");

	CRUGlobals::GetInstance()->
		LogDebugMessage(CRUGlobals::DUMP_POPINDEX,"",msg);
#endif

	ExecuteIndexStatmenents(*pUnAuditPopIndexdynamicContainer_, IDS_RU_POPINDEX_FAILED);

	// ?????
	// Temporary - Should be removed when read commited from unaudited table is implemented
	CommitTransaction();

	TESTPOINT2(CRUGlobals::TESTPOINT105, GetRootMVName());

	SetState(EX_EPILOGUE);
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::Epilogue()
//
//	Implementation of EX_EPILOGUE state
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::Epilogue()
{
	EndTimer();

	BeginTransaction();

	if (TRUE == GetRefreshTask()->NeedToExecuteInternalRefresh())
	{
		if (CDDObject::eON_STATEMENT == GetRootMVType())
		{
			EpilogueHandleOnStatementMV();
		}
		
		// Set the MV(s)/indexes back available
		ResetObjectsAvailable();

	}

	FinalMetadataUpdate();
	
	CommitTransaction();

	TESTPOINT2(CRUGlobals::TESTPOINT107, GetRootMVName());
	
	LogClosureMessage();

	SetState(EX_COMPLETE);
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::ComposeMySql()
//
//	Compose the SQL statements specific to this task executor
//	
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::ComposeMySql()
{
	CRURefreshTask *pTask = GetRefreshTask();
	CRUMV &mv = pTask->GetRootMV();

	CRUSimpleRefreshSQLComposer myComposer(pTask);

	// UNLOCK TABLE statement
	myComposer.ComposeUnLock(GetRootMVName());
	
	unAuditRefreshTEDynamicContainer_.SetStatementText
				(RU_UNLOCK_TABLE, myComposer.GetSQL());

	// POPINDEX CatApi request
	if (TRUE == isPopindex_)
	{
		numOfIndexes_ = mv.GetIndexList().GetCount();
		
		if (0 < numOfIndexes_)
		{
			ComposeIndexesSql();
		}
	}

	// Compose the LOCK TABLE sql statements for locking all tables 
	// in the on statement MV initialization 
	if (CDDObject::eON_STATEMENT == GetRootMVType())
	{	
		CRUTblList &tblList = mv.GetTablesUsedByMe();
		
		DSListPosition pos = tblList.GetHeadPosition();
		
		pLockTablesTEDynamicContainer_ = 
			new CRUSQLDynamicStatementContainer((short)tblList.GetCount());
		
		Int32 i=0;
		
		while (NULL != pos)
		{
			CRUTbl *pTbl = tblList.GetNext(pos);
			myComposer.ComposeLock(pTbl->GetFullName(), FALSE /*shared*/);
			pLockTablesTEDynamicContainer_->SetStatementText
				(i,myComposer.GetSQL());
			i++;
		}
	}
}
//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::ComposeIndexesSql()
//
//--------------------------------------------------------------------------//
void CRUUnAuditRefreshTaskExecutor::ComposeIndexesSql()
{
	
	const CDDIndexList &indexList = GetRootMV().GetIndexList();

        if( NULL == pUnAuditPopIndexdynamicContainer_ )
        {
    	  pUnAuditPopIndexdynamicContainer_ = 
		new CRUSQLDynamicStatementContainer(numOfIndexes_);
        }

        if( NULL == pUnAuditAvailableIndeXdynamicContainer_ )
        {
	  pUnAuditAvailableIndeXdynamicContainer_ =
		new CRUSQLDynamicStatementContainer(numOfIndexes_);
        }

        if( NULL == pUnAuditUnavailableIndeXdynamicContainer_ )
        {
	  pUnAuditUnavailableIndeXdynamicContainer_ = 
		new CRUSQLDynamicStatementContainer(numOfIndexes_);
        }

	DSListPosition pos = indexList.GetHeadPosition();
	
	for (Int32 i=0;NULL != pos;i++)
	{
		CDDIndex *pddIndex = indexList.GetNext(pos);
	
		CDSString popIdxRqst;
		GetRootMV().GetPopIndexCatApiRequestText(popIdxRqst, pddIndex);
		pUnAuditPopIndexdynamicContainer_->SetStatementText(i, popIdxRqst);

		CDSString availablepopIdxRqst;
		GetRootMV().GetUpdateIndexStatusCatApiRequestText(availablepopIdxRqst, 
														  TRUE, /*available*/
														  pddIndex);
		pUnAuditAvailableIndeXdynamicContainer_->SetStatementText(i, availablepopIdxRqst);

		CDSString unavailableIdxRqst; 
		GetRootMV().GetUpdateIndexStatusCatApiRequestText(unavailableIdxRqst, 
														  FALSE, /*unavailable*/
														  pddIndex);
		pUnAuditUnavailableIndeXdynamicContainer_->SetStatementText(i, unavailableIdxRqst);
	}
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::ExecuteIndexStatmenents()
//
// Execute all the index related statments in the container
//--------------------------------------------------------------------------//
void CRUUnAuditRefreshTaskExecutor::ExecuteIndexStatmenents(
											CRUSQLDynamicStatementContainer &container, 
											Lng32 errorCode)
{
	short numStmt = container.GetNumOfStmt();

	for (short i=0;i<numStmt;i++)
	{
		CDMPreparedStatement *pStmt = container.GetPreparedStatement(i);
	
		ExecuteStatement(*pStmt, errorCode);
	}
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::SetObjectsUnavailable()
//
//	Marking the MVs/indexes as non-available (called by Prologue()):
//	(1) If POPINDEX will be performed, turn the indexes to non-available
//	(2) For all MVs:
//	 (2.1) MV.MV_Status <-- UNAVAILABLE 
//	 (2.2) If MV.AUDIT != NOAUDIT then MV_TABLE.AUDIT <-- 'N'
//  
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::SetObjectsUnavailable()
{
	if (TRUE == isPopindex_ && 0 < numOfIndexes_)
	{
		// Turn all indexes to unavailable state
		ExecuteIndexStatmenents(*pUnAuditUnavailableIndeXdynamicContainer_,
								IDS_RU_INDEXSTATUS_FAILED);
	} 

	CRUMVList &mvList = GetRefreshTask()->GetMVList();

	DSListPosition pos = mvList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = mvList.GetNext(pos);
		if (CDDObject::eNO_AUDIT_ON_REFRESH == pMV->GetAuditType()
			// || 
			// CDDObject::eAUDIT == pMV->GetAuditType()
			// DO NOT TURN THE AUDITED MV'S TABLE TO NON-AUDITED
			// UNTIL THE BUG WITH TURNING THE AUDIT ATTRIBUTE IS FIXED !!!
			// Right now, we assume that the MV's table is initially
			// non-audited if this task executor is picked up.
			)
		{
		  // Alter table audit uses "ALTER TABLE" syntax
		  // and it cannot be performed if there is a DDL lock.
		  // Due to the transaction protection , for any other transaction
		  // the ddl locks will preserve continuity.
		  pMV->ReleaseDDLLock();
		  pMV->SetMVTableAudit(FALSE);
		  pMV->SaveMetadata();
		  pMV->CreateDDLLock();
		  
		}
		
		pMV->SetMVStatus(CDDObject::eUNAVAILABLE);
		pMV->SaveMetadata();
	} 

        // Since the mv ddl lock was released and recreated, the popindex
        // sql statements need to be recomposed
        if( TRUE == isPopindex_ && 0 < numOfIndexes_ )
        {
        	ComposeIndexesSql();
        }
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::ResetObjectsAvailable()
//
//	Restoring things to the normal status (called by Epilogue())
//	(1) For all MVs:
//	 (1.1) MV.MV_Status <-- INITIALIZED
//	 (1.2) If MV.AUDIT != NOAUDIT then MV_TABLE.AUDIT <-- 'Y'
//  
//	(2) If POPINDEX will be performed, turn the indexes back to available
//
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::ResetObjectsAvailable()
{
	CRUMVList &mvList = GetRefreshTask()->GetMVList();
	DSListPosition pos = mvList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = mvList.GetNext(pos);

		if (CDDObject::eNO_AUDIT != pMV->GetAuditType())
		{
			// Alter table audit uses "ALTER TABLE" syntax
			// and it cannot be performed if there is a DDL lock.
			// Due to the transaction protection , for any other transaction
			// the ddl locks will preserve continuity.
			pMV->ReleaseDDLLock();
			pMV->SetMVTableAudit(TRUE);
			pMV->SaveMetadata();
			pMV->CreateDDLLock();
		}

		RUASSERT(CDDObject::eUNAVAILABLE == pMV->GetMVStatus());

		pMV->SetMVStatus(CDDObject::eINITIALIZED);

		pMV->SaveMetadata();
	}

	if (TRUE == isPopindex_ && 0 < numOfIndexes_)
	{
		// must recompose since DDL locks were released/created        
		ComposeIndexesSql();

		// Turn all indexes to available state
		ExecuteIndexStatmenents(*pUnAuditAvailableIndeXdynamicContainer_,
								IDS_RU_INDEXSTATUS_FAILED);
	}
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::PrologueHandleOnStatementMV()
//
//	Tables used by the ON STATEMENT MV must remain locked throughout the
//	process of refresh, until the MV's status becomes INITIALIZED. 
//
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::PrologueHandleOnStatementMV()
{
	CRUTblList &tblList = GetRootMV().GetTablesUsedByMe();
	DSListPosition pos = tblList.GetHeadPosition();
		
	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList.GetNext(pos);

		pTbl->ExecuteReadProtectedOpen();
	}
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::EpilogueHandleOnStatementMV()
//
//	The ON STATEMENT MV's initialization and index population are complete.
//	Now, its status must become INITIALIZED. From now on, the incremental 
//	maintenance of the MV must start - every IUD statement on the table 
//	must update the MV too. In the other words, the statements on the used
//	tables must be recompiled and inline the MV's update mechanism.
//
//	In order to achieve this behavior, the CatApi request that changes the 
//	status of an ON STATEMENT MV to INITIALIZED - also touches the timestamps
//	of all the tables used by this MV. The request must ignore the DDL locks 
//	on the MV and all the used tables. However, the implementation of the 
//	CatApi request mechanism does not allow to ignore more than a single DDL 
//	lock. This is why the DDL locks on the MV and the used tables are dropped
//	by the Refresh task executor, rather than the RcRelease task executor
//	(the normal scenario).
//
//	The CatApi request is a catalog operation. Therefore, it requires an
//	exclusive lock on the used tables. However, this cannot happen until 
//	the non-transactional RP open on the table is released. On the other
//	hand, we cannot just release the RP open before turning the MV to 
//	initialized, because then IUD operations can trickle data into the table
//	without updating the MV. The solution is to perform a transactional
//	LOCK TABLE command that will *overlap* the original RP open (thus ensuring
//	the lock continuity), and to release the RP open immediately afterwards.
//	The transactional lock will not collide with the DDL operation, and will 
//	expire together with the transaction.
//
//	The operations' timing is as follows:
//	
//	--+------...---+-------------+-----------------+---------
//	  ^            ^             ^                 ^
//	  |            |             |                 |
//   RP Open ...  LOCK TABLE   Release RP Open   Commit txn
//
//    1 lock       2 locks       1 lock            0 locks
//
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::EpilogueHandleOnStatementMV()
{
	CRUMV &mv = GetRootMV();

	CRUTblList &tblList = mv.GetTablesUsedByMe();
	DSListPosition pos = tblList.GetHeadPosition();

	Int32 i=0;

	while (NULL != pos)
	{
		CRUTbl *pTbl = tblList.GetNext(pos);
	
		CDMPreparedStatement *pStat = 
			pLockTablesTEDynamicContainer_->GetPreparedStatement(i);

		// Perform the LOCK TABLE statement on the used table
		// (the lock overlaps the original RP open, therefore
		// guaranteeing the lock continuity).
		ExecuteStatement(*pStat,IDS_RU_IREFRESH_FAILED);

		// Release the DDL lock AND the read-protected open on the used table.
		pTbl->ReleaseResources();
	}
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::UnLockMV()
//
//	CURRENTLY NOT USED
//--------------------------------------------------------------------------//

void CRUUnAuditRefreshTaskExecutor::UnLockMV()
{
	CDMPreparedStatement *pStmt = unAuditRefreshTEDynamicContainer_.
						GetPreparedStatement(RU_UNLOCK_TABLE);
	
	ExecuteStatement(*pStmt, IDS_RU_UNLOCK_TABLE_FAILED);
}
