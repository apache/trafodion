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
* File:         RuAuditRefreshTaskExecutor.cpp
* Description:  Implementation of class CRUSimpleRefreshTaskExecutor.
*				
*
* Created:      08/14/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuAuditRefreshTaskExecutor.h"
#include "RuSimpleRefreshSQLComposer.h"
#include "RuTbl.h"
#include "RuGlobals.h"
#include "uofsIpcMessageTranslator.h"
#include "ddindex.h"

//--------------------------------------------------------------------------//
//	Constructor and destructor
//--------------------------------------------------------------------------//

CRUAuditRefreshTaskExecutor::
CRUAuditRefreshTaskExecutor(CRURefreshTask *pParentTask) :
	inherited(pParentTask),
	auditRefreshTEDynamicContainer_(NUM_OF_SQL_STMT),
	isDeleteMultiTxnContext_(FALSE),
        pAuditPopIndexdynamicContainer_(NULL),
	pAuditAvailableIndeXdynamicContainer_(NULL),
	pAuditUnavailableIndeXdynamicContainer_(NULL),
        pAuditToggleOnIndexdynamicContainer_(NULL),
        pAuditToggleOffIndexdynamicContainer_(NULL),
	pLockTablesTEDynamicContainer_(NULL),
	isPurgedata_(FALSE),
	isPopindex_(FALSE),
	numOfIndexes_(0)
{}

CRUAuditRefreshTaskExecutor::~CRUAuditRefreshTaskExecutor() 
{
	delete pLockTablesTEDynamicContainer_;

        if( 0 < numOfIndexes_ )
        {
	  delete pAuditPopIndexdynamicContainer_;
	  delete pAuditAvailableIndeXdynamicContainer_;
	  delete pAuditUnavailableIndeXdynamicContainer_;        
          delete pAuditToggleOnIndexdynamicContainer_ ;        
          delete pAuditToggleOffIndexdynamicContainer_ ;
        }        
}

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::StoreRequest()
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::
	StoreRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::StoreRequest(translator);

        translator.WriteBlock(&isPurgedata_, sizeof(BOOL));
	translator.WriteBlock(&isPopindex_, sizeof(BOOL));

	// Handle audit refresh executor sql dynamic container
	auditRefreshTEDynamicContainer_.StoreData(translator);
	
	// Handle isDeleteMultiTxnContext_ data member
	translator.WriteBlock(&isDeleteMultiTxnContext_,sizeof(BOOL));

        translator.WriteBlock(&numOfIndexes_, sizeof(TInt32));

	if (0 < numOfIndexes_)
	{
		pAuditPopIndexdynamicContainer_->StoreData(translator);
		pAuditAvailableIndeXdynamicContainer_->StoreData(translator);
		pAuditUnavailableIndeXdynamicContainer_->StoreData(translator);
                pAuditToggleOnIndexdynamicContainer_->StoreData(translator);
                pAuditToggleOffIndexdynamicContainer_->StoreData(translator);
	}

	translator.SetMessageType(CUOFsIpcMessageTranslator::
						RU_AUDIT_REFRESH_EXECUTOR);
}

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::LoadRequest()
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::
	LoadRequest(CUOFsIpcMessageTranslator &translator)
{
	inherited::LoadRequest(translator);

        translator.ReadBlock(&isPurgedata_, sizeof(BOOL));
	translator.ReadBlock(&isPopindex_, sizeof(BOOL));

	// Handle refresh executor sql dynamic container
	auditRefreshTEDynamicContainer_.LoadData(translator);
	
	// Handle isDeleteMultiTxnContext_ data member
	translator.ReadBlock(&isDeleteMultiTxnContext_,sizeof(BOOL));

        translator.ReadBlock(&numOfIndexes_, sizeof(TInt32));

	if (0 < numOfIndexes_)
	{
		pAuditPopIndexdynamicContainer_ =  
			new CRUSQLDynamicStatementContainer(numOfIndexes_);

		pAuditAvailableIndeXdynamicContainer_ =
			new CRUSQLDynamicStatementContainer(numOfIndexes_);

                pAuditUnavailableIndeXdynamicContainer_ = 
			new CRUSQLDynamicStatementContainer(numOfIndexes_);

		pAuditToggleOnIndexdynamicContainer_ = 
			new CRUSQLDynamicStatementContainer(numOfIndexes_);

		pAuditToggleOffIndexdynamicContainer_ = 
			new CRUSQLDynamicStatementContainer(numOfIndexes_);


		pAuditPopIndexdynamicContainer_->LoadData(translator);
		pAuditAvailableIndeXdynamicContainer_->LoadData(translator);
		pAuditUnavailableIndeXdynamicContainer_->LoadData(translator);
                pAuditToggleOnIndexdynamicContainer_->LoadData(translator);
                pAuditToggleOffIndexdynamicContainer_->LoadData(translator);
	}
}

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::Init()
//
//	Initialize data members by analyzing the RefreshTask
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::Init()
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

	CRUMV &mv = GetRootMV();
	if (mv.GetCommitNRows() != 0 && 
		TRUE == GetRootMV().IsMultiTxnContext())
	{
		RUASSERT(TRUE == GetRefreshTask()->IsRecompute());

		// We need to drop multi-txn context table
		// becuase we are recomputing a multi-txn mv
		isDeleteMultiTxnContext_ = TRUE;
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
//	CRUAuditRefreshTaskExecutor::ComposeMySql()
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::ComposeMySql()
{
	CRUSimpleRefreshSQLComposer myComposer(GetRefreshTask());
	CRUMV &rootMV = GetRootMV();        	

	if (TRUE == isDeleteMultiTxnContext_)
	{
		myComposer.ComposeDeleteContextLogTable();

		auditRefreshTEDynamicContainer_.SetStatementText
			(DELETE_MULT_TXN_CTX_TBL,myComposer.GetSQL());
	}

        // POPINDEX CatApi request
	if (TRUE == isPopindex_)
	{
		numOfIndexes_ = rootMV.GetIndexList().GetCount();
		              
		if (0 < numOfIndexes_)
		{
			ComposeIndexesSql();
		}
	}

	// Compose the LOCK TABLE sql statements for locking all tables 
	// in the on statement MV initialization 
	if (CDDObject::eON_STATEMENT == GetRootMVType())
	{	
		CRUTblList &tblList = rootMV.GetTablesUsedByMe();
		
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
//	CRUAuditRefreshTaskExecutor::Work()
//
//	Main finite-state machine switch.
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::Work()
{
	switch (GetState())
	{
	case EX_START: // MAIN PROCESS
		{
			// This state only logs the opening message
			Start();
			break;
		}	
	case EX_PROLOGUE: // MAIN PROCESS
		{
			// This state just starts a transaction  
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
        case EX_REMOTE_START:
                {
                        // this state starts up the table lock protocol
                        // and starts a transaction                 
                        RemoteStart();
                        break;
                }
	case EX_RECOMPUTE: // REMOTE PROCESS
		{
			// This state execute an internal refresh recompute statement 
			Recompute();
			break;
		}
	case EX_REFRESH: // REMOTE PROCESS
		{
			// This state execute an internal refresh statement 
			// It may be a multi delta refresh or a single delta refresh
			Refresh();
			break;
		}
        case EX_POPINDEX:	// REMOTE PROCESS
		{
			RUASSERT(FALSE == IsTransactionOpen());
			PopulateIndexes();
			break;
		}        
	case EX_REMOTE_END: // REMOTE PROCESS
		{
			RemoteEnd();
			break;
		}
	case EX_EPILOGUE: // MAIN PROCESS
		{
			// Update all meta-data concerning the mv and used tables objects
			Epilogue();
			break;
		}
	default: RUASSERT(FALSE);
	}
}

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::Start()
//
//	Implementation of EX_START state
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::Start()
{
	RUASSERT(FALSE == IsTransactionOpen());

	LogOpeningMessage();

	StartTimer();

	TESTPOINT2(CRUGlobals::TESTPOINT130, GetRootMVName())

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
//	CRUAuditRefreshTaskExecutor::Prologue()
//
//	Implementation of EX_PROLOGUE state
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::Prologue()
{
	RUASSERT(FALSE == IsTransactionOpen());	    

        BeginTransaction();
        if (CDDObject::eON_STATEMENT == GetRootMVType())
	{
		PrologueHandleOnStatementMV();
	}
			
	SetObjectsUnavailable();		        	      
        CommitTransaction();

        TESTPOINT2(CRUGlobals::TESTPOINT102, GetRootMVName());

	if (TRUE == isPurgedata_) 
	{	
		// purgedata is done in main process           
		SetState(EX_PURGE_DATA);
	}
	else 
	{
		// refresh, recompute, and popindex done in remote process
		SetState(EX_REMOTE_START);
	}	
}
//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::RemoteStart()
//
//	Implementation of EX_REMOTE_START state
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::RemoteStart()
{
	BeginTransaction();

	if (FALSE == StartTableLockProtocol() )
	{		
		SetState(EX_REMOTE_END);
		return;
	}  

        if (TRUE == IsRecompute())
	{
		SetState(EX_RECOMPUTE);
	}
	else
	{
		SetState(EX_REFRESH);
	}
}

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::SetObjectsUnavailable()
//
//	The initial refresh sets the MVTable audit flag ON and the
//      MVStatus to eUNAVAILABLE.  The epilogue will set it to initialized.
//
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::SetObjectsUnavailable()
{
	if (TRUE == isPopindex_ && 0 < numOfIndexes_)
	{
		// Turn all indexes to unavailable state
		ExecuteIndexStatmenents(*pAuditUnavailableIndeXdynamicContainer_,
					IDS_RU_INDEXSTATUS_FAILED);
	} 

        // first turn the audit flag ON for the MV table
        // and set the MV to unavailable
	CRUMVList &mvList = GetRefreshTask()->GetMVList();
	DSListPosition pos = mvList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = mvList.GetNext(pos);

		// Alter table audit uses "ALTER TABLE" syntax
		// and it cannot be performed if there is a DDL lock.
		// Due to the transaction protection , for any other transaction
		// the ddl locks will preserve continuity.
		pMV->ReleaseDDLLock();
		pMV->SetMVTableAudit(TRUE);               
		pMV->SaveMetadata();
		pMV->CreateDDLLock();
		
		// if hasn't been set to initialized, initialize it
                if( CDDObject::eINITIALIZED != pMV->GetMVStatus() )
                {
			pMV->SetMVStatus(CDDObject::eUNAVAILABLE);
			pMV->SaveMetadata();                
                }
	}        

        // Since the mv ddl lock was released and recreated, the popindex
        // sql statements need to be recomposed
        if( TRUE == isPopindex_ && 0 < numOfIndexes_ )
        {
        	ComposeIndexesSql();
        }
}

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::ToggleIndicesAudit()
//
//	Toggle the audit flag for all indices
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::ToggleIndicesAudit(BOOL flag)
{
        RUASSERT(TRUE == isPopindex_);

        if( 0 == numOfIndexes_ )
        {
              return;
        }

        if( TRUE == flag )
        {      
              ExecuteIndexStatmenents(*pAuditToggleOnIndexdynamicContainer_, 
                                        IDS_RU_AUDITTOGGLE_FAILED); 
        }
        else
        {
              ExecuteIndexStatmenents(*pAuditToggleOffIndexdynamicContainer_,
                                        IDS_RU_AUDITTOGGLE_FAILED); 
        }                         
}

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::ResetObjectsAvailable()
//
//	Reset objects to available and set the MV to initialized
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::ResetObjectsAvailable()
{
        CRUMVList &mvList = GetRefreshTask()->GetMVList();
	DSListPosition pos = mvList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUMV *pMV = mvList.GetNext(pos);
                BOOL  mvUpdated = FALSE;
		
		// if hasn't been set to initialized, initialize it
                if( CDDObject::eINITIALIZED != pMV->GetMVStatus() )
                {
			pMV->SetMVStatus(CDDObject::eINITIALIZED);
                        mvUpdated = TRUE;
                }

                if (mvUpdated)
                {
			pMV->SaveMetadata();
                }

	}        

        if (TRUE == isPopindex_ && 0 < numOfIndexes_)
	{                
		// Turn all indexes to available state
		ExecuteIndexStatmenents(*pAuditAvailableIndeXdynamicContainer_,
					IDS_RU_INDEXSTATUS_FAILED);
	}	
}

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::Recompute()
//
//	Implementation of EX_RECOMPUTE state
//
// Prior to executing the internal refresh recompute statement we take 
// for optimization reasons an exclusive lock on the mv
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::Recompute()
{
	// TEMPORARY: Commented out until the privilege-skipping lock 
	// is implemented 
	
	// LockMV();

	if (FALSE == IsTransactionOpen())
	{
		BeginTransaction(); 
	}

	// Decouple prepare and execute to separate transactions.
	CDMPreparedStatement *pStat = PrepareRecomputeMV();
	CommitTransaction();
	BeginTransaction();
	ExecuteRecomputeMV(pStat);

	if (TRUE == isDeleteMultiTxnContext_)
	{
		// Delete all rows from the context log table 
		auditRefreshTEDynamicContainer_.
			DirectExecStatement(DELETE_MULT_TXN_CTX_TBL);
	}

        if (TRUE == isPopindex_)
	{
		CommitTransaction();
		SetState(EX_POPINDEX);
	}
        else
        {
		SetState(EX_REMOTE_END);
        }
}

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::Refresh()
//
//	Implementation of EX_REFRESH state
//
//  Execute an internal refresh statement (incremental refresh)
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::Refresh()
{
	RUASSERT(TRUE == IsTransactionOpen());

	// Simulate a system error at this point!
	TESTPOINT_SEVERE(CRUGlobals::SEVERE_REFRESH_CRASH);

	try 
	{		
		ApplyIRCompilerDefaults();

		if(TRUE == IsSingleDeltaRefresh())
		{
			CDMPreparedStatement *pStat = PrepareSingleDeltaRefresh();

			CommitTransaction();
			BeginTransaction();

			ExecuteSingleDeltaRefresh(pStat);
		}
		else
		{
			StmtList *stmts = PrepareMultiDeltasRefresh();

			CommitTransaction();
			BeginTransaction();

			ExecuteMultiDeltasRefresh(stmts);
		}

		ResetIRCompilerDefaults();

		ExecuteShowExplain();

		SetState(EX_REMOTE_END);
	}
	catch (CRUSimpleRefreshTaskExecutor::NeedRecomputeException)
	{
		// If a min/max value was deleted an exception will be thrown and
		// then we need to recompute the mv
		SetState(EX_RECOMPUTE);
		SetRecompute(TRUE);
	}
}

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::PopulateIndexes()
//
//	Apply the pre-composed CatApi request code to populate the indexes
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::PopulateIndexes()
{
	RUASSERT(TRUE == isPopindex_);

	if (0 == numOfIndexes_)
	{
		return;
	}

        // toggle audit flag OFF for indices to do the
        // side tree insert
        BeginTransaction();
        ToggleIndicesAudit(FALSE);        
        CommitTransaction();
        

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

	ExecuteIndexStatmenents(*pAuditPopIndexdynamicContainer_, IDS_RU_POPINDEX_FAILED);
        
        CommitTransaction();

	TESTPOINT2(CRUGlobals::TESTPOINT105, GetRootMVName());

        // toggle the audit flag for the indices back on
        BeginTransaction();
        ToggleIndicesAudit(TRUE);
        // transaction gets committed in EX_REMOTE_END...

	SetState(EX_REMOTE_END);
}

//--------------------------------------------------------------------------//
//	CRUUnAuditRefreshTaskExecutor::PurgeData()
//
//	Implementation of EX_PURGE_DATA state
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::PurgeData()
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

        // now start the remote states and do a recompute
	SetState(EX_REMOTE_START);
}

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::ComposeIndexesSql()
//
//--------------------------------------------------------------------------//
void CRUAuditRefreshTaskExecutor::ComposeIndexesSql()
{
	
	const CDDIndexList &indexList = GetRootMV().GetIndexList();

	if( NULL == pAuditPopIndexdynamicContainer_ )
          pAuditPopIndexdynamicContainer_ = 
		new CRUSQLDynamicStatementContainer(numOfIndexes_);
 
        if( NULL == pAuditAvailableIndeXdynamicContainer_ )
	  pAuditAvailableIndeXdynamicContainer_ =
		new CRUSQLDynamicStatementContainer(numOfIndexes_);

        if( NULL == pAuditUnavailableIndeXdynamicContainer_ )
	  pAuditUnavailableIndeXdynamicContainer_ = 
		new CRUSQLDynamicStatementContainer(numOfIndexes_);

        if( NULL == pAuditToggleOnIndexdynamicContainer_ )
          pAuditToggleOnIndexdynamicContainer_ =
		new CRUSQLDynamicStatementContainer(numOfIndexes_);

        if( NULL == pAuditToggleOffIndexdynamicContainer_ )
          pAuditToggleOffIndexdynamicContainer_ =
		new CRUSQLDynamicStatementContainer(numOfIndexes_);

	DSListPosition pos = indexList.GetHeadPosition();
	
	for (Int32 i=0;NULL != pos;i++)
	{
		CDDIndex *pddIndex = indexList.GetNext(pos);
	
		CDSString popIdxRqst;
		GetRootMV().GetPopIndexCatApiRequestText(popIdxRqst, pddIndex);

		pAuditPopIndexdynamicContainer_->SetStatementText(i, popIdxRqst);

		CDSString availablepopIdxRqst;
		GetRootMV().GetUpdateIndexStatusCatApiRequestText(availablepopIdxRqst, 
								TRUE, /*available*/
								pddIndex);

		pAuditAvailableIndeXdynamicContainer_->SetStatementText(i, availablepopIdxRqst);

		CDSString unavailableIdxRqst; 
		GetRootMV().GetUpdateIndexStatusCatApiRequestText(unavailableIdxRqst, 
								FALSE, /*unavailable*/
								pddIndex);

		pAuditUnavailableIndeXdynamicContainer_->SetStatementText(i, unavailableIdxRqst);

                CDSString auditOffIdxRqst;
                GetRootMV().GetToggleAuditCatApiRequestText(auditOffIdxRqst, 
								FALSE, /* audit OFF */
								pddIndex);

                pAuditToggleOffIndexdynamicContainer_->SetStatementText(i,auditOffIdxRqst);

                CDSString auditOnIdxRqst;
                GetRootMV().GetToggleAuditCatApiRequestText(auditOnIdxRqst, 
								TRUE, /* audit ON */
								pddIndex);

                pAuditToggleOnIndexdynamicContainer_->SetStatementText(i,auditOnIdxRqst);

	}
}

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::ExecuteIndexStatmenents()
//
// Execute all the index related statments in the container
//--------------------------------------------------------------------------//
void CRUAuditRefreshTaskExecutor::ExecuteIndexStatmenents(CRUSQLDynamicStatementContainer &container, 
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
//	CRUAuditRefreshTaskExecutor::RemoteEnd()
//
//	Implementation of the REMOTE_END phase see Refresh-Design.doc
//  for detailed explanation
//
//  Unlock used table if necessary  
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::RemoteEnd()
{
	RUASSERT(TRUE == IsTransactionOpen());

	EndTableLockProtocol();
	CommitTransaction();
	SetState(EX_EPILOGUE);
}

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::Epilogue()
//
//	Implementation of EX_Epilogue state
//
//  Update the MV's metadata
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::Epilogue()
{
	RUASSERT(FALSE == IsTransactionOpen());

        EndTimer();

        BeginTransaction();

	// if this is the first refresh, the MV status can be
        // changed to initialized
	if( TRUE == GetRefreshTask()->NeedToExecuteInternalRefresh() )
	{
        	if (CDDObject::eON_STATEMENT == GetRootMVType())
		{
			EpilogueHandleOnStatementMV();
		}		
		ResetObjectsAvailable();	
	}		

	FinalMetadataUpdate();

	CommitTransaction();

	TESTPOINT2(CRUGlobals::TESTPOINT131, GetRootMVName());
	
	LogClosureMessage();

	SetState(EX_COMPLETE);
}

//--------------------------------------------------------------------------//
//	CRUAuditRefreshTaskExecutor::PrologueHandleOnStatementMV()
//
//	Tables used by the ON STATEMENT MV must remain locked throughout the
//	process of refresh, until the MV's status becomes INITIALIZED. 
//
//--------------------------------------------------------------------------//

void CRUAuditRefreshTaskExecutor::PrologueHandleOnStatementMV()
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
//	CRUAuditRefreshTaskExecutor::EpilogueHandleOnStatementMV()
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

void CRUAuditRefreshTaskExecutor::EpilogueHandleOnStatementMV()
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
