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
* File:         RefreshTest.cpp
* Description:  Test engine for the refresh utility
*
*
* Created:      01/09/2001
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "RefreshTest.h"

#include "RuException.h"
#include "dmresultset.h"
#include "dsstring.h"
#include "RuSQLComposer.h"

enum //values for specifying link options...
  {MSG_LINK_CBA        = 0200 , //all the addresses are 32 bit
                                //ptrs to CBAs
   MSG_LINK_STOPME     = 0100 , //special handling for self stop
   MSG_LINK_TOCLUSTER  = 040 ,  //CLUSTERLINK replacement
   MSG_LINK_FSDONEQ    = 020 ,  //FileSystem DONE queueing
   MSG_LINK_LDONEQ     = 010 ,  //LDONE queueing
   MSG_LINK_EXREMOTE   = 04 ,   //force "remote request" (EXPAND)
   MSG_LINK_REMID      = 02 ,   //force "remote access"  (EXPAND)
   MSG_LINK_SECURE     = 01
  };  
#include "security/UID.h"
#include "security/dsecure.h"
#include "guardian/dsystypz.h"
#include "security/psecure.h"

/*
	This program must have ready sql tables before execution.
	This is the sql statement that creates those tables

create table RefreshControlTable 
		(group_id				INT UNSIGNED NOT NULL NOT DROPPABLE,
		 process_id				INT UNSIGNED NOT NULL NOT DROPPABLE,
		 ordinal				INT UNSIGNED NOT NULL NOT DROPPABLE,
		 sql_text				CHAR(1000) NOT NULL NOT DROPPABLE,
		 number_of_executions	INT UNSIGNED NOT NULL NOT DROPPABLE,
		 number_of_retries		INT UNSIGNED NOT NULL NOT DROPPABLE,
		 auto_commit			INT UNSIGNED NOT NULL NOT DROPPABLE, -- 1 commit , 0 dont commit
		 primary key (group_id,process_id,ordinal))
		 store by primary key;

create table RefreshOutputTable
		(group_id				INT UNSIGNED NOT NULL NOT DROPPABLE,
		 process_id				INT UNSIGNED NOT NULL NOT DROPPABLE,
		 ordinal				INT UNSIGNED NOT NULL NOT DROPPABLE,
		 try_number				INT UNSIGNED NOT NULL NOT DROPPABLE,
		 line					INT UNSIGNED NOT NULL NOT DROPPABLE,
		 sql_error_code			INT NOT NULL NOT DROPPABLE,
		 sql_error_text			CHAR(1000))
		 store by entry order;

create table RefreshTimeTable
		(group_id				INT UNSIGNED NOT NULL NOT DROPPABLE,
		 process_id				INT UNSIGNED NOT NULL NOT DROPPABLE,
		 after_sync 			INT UNSIGNED NOT NULL NOT DROPPABLE,
		 ts						TIMESTAMP NOT NULL NOT DROPPABLE)
		 store by entry order;


*/

//--------------------------------------------------------------------------//
//	Global Functions
//--------------------------------------------------------------------------//

#define MAX_PRINTABLE_SID_LENGTH 256

void SetUser(const char *argAfterOp)
{
	NTSEC_USER currentUser;
	
	UInt32 buflen = MAX_PRINTABLE_SID_LENGTH;
	char sBuf[MAX_PRINTABLE_SID_LENGTH]; // Buffer for printable SID.
	Int32 status;

	if ( SECURITY_APP_PRIV_() )   // Silently ignore, if not Super.
     {
        currentUser = NTSEC_USER( argAfterOp );
        if ( currentUser == SECURITY_INVALID_UID )
           cerr << "Invalid user specified." << endl << flush;
        else
           SECURITY_PSB_SET_( PSB_EFFECTIVE_UID
                            , &currentUser
                            , (int_16)sizeof(currentUser)
                            );
        status = SECURITY_PSB_GET_( PSB_EFFECTIVE_UID
                   , &currentUser
                   , (int_16)sizeof(currentUser)
                   );
        status = currentUser.textsid(sBuf, &buflen); // Convert to SID.
        status = SQL_EXEC_SetAuthID( sBuf, SQLAUTHID_TYPE_ASCII_SID );

        cerr << "User SID = " << sBuf << endl << flush;
     }
}

void HandleExecuteException(CDSException &ex)
{
	enum { BUFSIZE = 1024 };

	short nerr = ex.GetNumErrors();
	char buffer[BUFSIZE];
	CDSString msg;

	for (Int32 i=0; i<nerr; i++) {
		
		ex.GetErrorMsg(i, buffer, BUFSIZE);
		
		if (buffer[0] != 0)
		{
			// Clear the trailing whitespace
			char *p = buffer + strlen(buffer) - 1;
			for (;buffer != p && isspace((unsigned char)*p); p--, *p=0);  // For VS2003
		}

		msg += buffer + CDSString("\n");
	}
	cout << msg;
}

// Main Params :	1. number of allowed remote processes
//					2. Sql user id
Int32 main(Int32 argc, char *argv[])
{
    Int32 numOfProcesses = 0;
	if (argc != 3)
    {
		cout << "The command line parameters should be :" << endl;
		cout << "1. Number of processes" << endl;
		return -1;
	}
	
	sscanf(argv[1],"%d",&numOfProcesses);

	if (1 > numOfProcesses || 15 < numOfProcesses )
	{
		cout << "Wrong number of remote processes" << endl;
		return -1;
	}
	
	SetUser(argv[2]);

	RefreshTestController controller(numOfProcesses);
	
	try {	
		controller.Init();

		while (RefreshTestController::EX_COMPLETE!= controller.GetState())
		{
			controller.Work();
		}
		cout << "Done" << endl;
	}
	catch (CDSException &ex)
	{
		HandleExecuteException(ex);
	}

	return 0;
}

//--------------------------------------------------------------------------//
//	RefreshTestController::Init()
//--------------------------------------------------------------------------//
void RefreshTestController::Init()
{
	pJournal_ = new CRUJournal("");

	CRUGlobals::Init(options_,
					*(pJournal_),
					transManager_);

	transManager_.OpenTMF();

	for(Int32 i=0;i<numOfProcesses_;i++)
	{
		InitiateTaskProcess();
	}
	
	
	CDSString sqlText;

	sqlText = "	SELECT	group_id,process_id,count(*) as number_of_statements ";
	sqlText +="	FROM CAT1.MVSCHM.RefreshControlTable ";
	sqlText +="	GROUP BY group_id,process_id ";
	sqlText +="	ORDER BY group_id,process_id;\n";
	
	dynamicSQLContainer_.SetStatementText(0,sqlText);

	CDMPreparedStatement *pStat = 
		dynamicSQLContainer_.GetPreparedStatement(0);

	transManager_.BeginTrans();

	CDMResultSet *pResult = pStat->ExecuteQuery();

	const Int32 kGroupId			= 1;
	const Int32 kProcessId		= 2;
	const Int32 kNumOfStatements	= 3;

	BOOL first = TRUE;

	while (pResult->Next()) 
	{

		GroupStatement *pGroup = new GroupStatement();
		
		groupList_.AddTail(pGroup);

		pGroup->groupId_ = pResult->GetInt(kGroupId);
		
		if (first)
		{
			currentGroupId_ = pGroup->groupId_;

			first = FALSE;
		}

		pGroup->processId_ = pResult->GetInt(kProcessId) - 1;
		pGroup->num_of_executions = pResult->GetInt(kNumOfStatements);
	}

	pStat->DeleteResultSet(pResult);
	pStat->Close();
	
	sqlText = "INSERT INTO CAT1.MVSCHM.REFRESHTIMETABLE VALUES(?,?,?,CURRENT)";

	dynamicSQLContainer_.SetStatementText(1,sqlText);

	dynamicSQLContainer_.PrepareStatement(1);

	transManager_.CommitTrans();

	currentPos_ = groupList_.GetHeadPosition();

	if (groupList_.IsEmpty())
	{
		cout << "Control Table is empty\n";
		RUASSERT(FALSE);
	}

	SetState(EX_SEND_FOR_INIT);
}

//--------------------------------------------------------------------------//
//	RefreshTestController::InitiateTaskProcess()
//--------------------------------------------------------------------------//
Int32 RefreshTestController::InitiateTaskProcess()
{
	CRUOptions option;
	
	const Int32 bufferSize = CUOFsIpcMessageTranslator::MaxMsgSize;

	char buffer[bufferSize];

	Lng32 pid = processPool_.LaunchTaskProcess();

	CUOFsIpcMessageTranslator translator(buffer,bufferSize);
	
	// Serialize the utility's command-line options ...
	translator.StartWrite();

	translator.SetMessageType(CUOFsIpcMessageTranslator::RU_GLOBALS);

	translator.EndWrite();

	transManager_.LeaveTransaction();

	// Send them to the task process ...
	processPool_[pid].Send(translator);

	// And receive the reply
	processPool_[pid].Receive();

	RUASSERT(translator.GetMessageType() == 
			 CUOFsIpcMessageTranslator::RU_GLOBALS);
	
	return pid;
}

//--------------------------------------------------------------------------//
//	RefreshTestController::Work()
//--------------------------------------------------------------------------//
void RefreshTestController::Work()
{
	switch (GetState())
	{
	case EX_SEND_FOR_INIT: 
		{
			GroupSendForInitialization();
			
			SetState(EX_WAIT_FOR_ALL_RETURN_FROM_COMPILATION);
			break;
		}
	case EX_WAIT_FOR_ALL_RETURN_FROM_COMPILATION:
		{
			WaitForAll();

			SetState(EX_SEND_FOR_EXECUTE);
			break;
		}
	case EX_SEND_FOR_EXECUTE: 
		{
			SendSyncToAllExecutors();

			SetState(EX_WAIT_FOR_ALL_RETURN_FROM_EXECUTION);
			break;
		}	
	case EX_WAIT_FOR_ALL_RETURN_FROM_EXECUTION: 
		{
			WaitForAll();
			
			if (!endFlag_)
			{
				SetState(EX_SEND_FOR_INIT);
			}
			else
			{
				processPool_.ShutdownAllTaskProcesses();
				SetState(EX_COMPLETE);
			}
			break;
		}	

	default: RUASSERT(FALSE);
	}
}


//--------------------------------------------------------------------------//
//	RefreshTestController::GroupSendForInitialization()
//--------------------------------------------------------------------------//
void RefreshTestController::GroupSendForInitialization()
{

	executorsList_.RemoveAll();

	activeProcesses_ = 0;
	
	while (NULL != currentPos_)
	{
		DSListPosition tempPos_ = currentPos_;

		GroupStatement *pGroup = groupList_.GetNext(currentPos_);

		if (currentGroupId_ != pGroup->groupId_)
		{
			// End of group
			currentGroupId_ = pGroup->groupId_;
			currentPos_ = tempPos_;
			return;
		}

		activeProcesses_++;
		
		if (pGroup->processId_ >= numOfProcesses_ )
		{
			cout << "Group: " << pGroup->groupId_ << " sends to unavailable process:" << pGroup->processId_ + 1<< endl;
			exit(-1);
		}

		cout << "Sending group for compilation: " ;
		cout << pGroup->groupId_ << " Process : " << pGroup->processId_ + 1<< endl;

		SendForInitialization(pGroup->groupId_,
							  pGroup->processId_,
							  pGroup->num_of_executions);
	} 
	endFlag_ = TRUE;
}

//--------------------------------------------------------------------------//
//	RefreshTestController::SendForInitialization()
//--------------------------------------------------------------------------//
void RefreshTestController::SendForInitialization(Int32 groupId,	
												  Int32 processId,
												  Int32 numOfStmt)
{
	CRUTestTaskExecutor *pExecutor = new CRUTestTaskExecutor();
	
	pExecutor->Init();

	pExecutor->SetGroupId(groupId);

	pExecutor->SetProcessId(processId);

	pExecutor->SetNumberOfStatements(numOfStmt);

	executorsList_.AddTail(pExecutor);
	
	// The initial buffer allocation
	pExecutor->AllocateBuffer();

	SendExecutor(*pExecutor);
}

//--------------------------------------------------------------------------//
//	RefreshTestController::SendExecutor()
//--------------------------------------------------------------------------//
void RefreshTestController::SendExecutor(CRUTaskExecutor &executor)
{
	CUOFsIpcMessageTranslator *pTranslator = NULL;

	for (Int32 i=2;;i*2)
	{
		pTranslator = &(executor.GetTranslator());

		try {
			// Serialize the executor
			pTranslator->StartWrite();
			executor.StoreRequest(*pTranslator);	
			pTranslator->EndWrite();
			break;
		}
		catch (CUOFsBufferOverFlowException)
		{
			// The buffer can be resized till the max message size 
			executor.ReAllocateBuffer(2 /*factor*/);
		}
	}
	transManager_.LeaveTransaction();
	processPool_[executor.GetProcessId()].Send(*pTranslator);
}

//--------------------------------------------------------------------------//
//	RefreshTestController::SendExecutor()
//--------------------------------------------------------------------------//
void RefreshTestController::SendSyncToAllExecutors()
{
	DSListPosition pos = executorsList_.GetHeadPosition();

	while (NULL != pos)
	{
		CRUTaskExecutor *pExecutor = executorsList_.GetNext(pos);

		CUOFsIpcMessageTranslator *pTranslator = &(pExecutor->GetTranslator());

		// Just advance the executor state to the next state 
		// (must be a remote state)
		pExecutor->Work();

		// Serialize the executor
		pTranslator->StartWrite();
		pExecutor->StoreRequest(*pTranslator);	
		pTranslator->EndWrite();

		pTranslator->SetMessageType(CUOFsIpcMessageTranslator::
								RU_TEST_SYNC);

		processPool_[pExecutor->GetProcessId()].Send(*pTranslator);

		cout << "Sending for execution: Group : " << ((CRUTestTaskExecutor*) pExecutor )->GetGroupId() << " Process : " << pExecutor->GetProcessId() + 1 << endl;

	}		
}

//--------------------------------------------------------------------------//
//	RefreshTestController::SendExecutor()
//--------------------------------------------------------------------------//
void RefreshTestController::WaitForAll()
{
	Int32 return_counter = 0;

	while (return_counter < activeProcesses_)
	{
		Lng32 pid = processPool_.ReceiveFromAnyProcess(-1);// An indefinite wait
		
		return_counter++;

		HandleReturnOfExecutor(pid);
	}
}

//----------------------------------------------------------------//
//	RefreshTestController::HandleReturnOfExecutor()
//
//	The message with the executor's context has been received
//	from the task process. De-serialize the context to continue
//	the execution.
//
//----------------------------------------------------------------//

void RefreshTestController::HandleReturnOfExecutor(Lng32 pid)
{
	CRUTaskExecutor *pExecutor  = FindRunningExecutor(pid);
	
	RUASSERT(NULL != pExecutor);

	CUOFsIpcMessageTranslator &translator = pExecutor->GetTranslator();

	if ( CUOFsIpcMessageTranslator::APPLICATION_ERROR == 
		 translator.GetMessageType() )
	{
		CRUException ex;
				
		// De-serialize the exception object from the message
		translator.StartRead();
		ex.LoadData(translator);
		translator.EndRead();

		ex.SetError(IDS_RU_REMOTE_EXECUTION_FAILURE);
		
		cout << "Remote Exception recieved ,Details are :" << endl;
		Int32  num  = ex.GetNumErrors();
	    for (Int32 i = 0; i < num; i++)
	    {
			const Int32 len = ERROR_BUFF_SIZE;   // buffer length
			char  buffer[len];     // exception message buffer

			Lng32 code = ex.GetErrorCode(i);
			ex.GetErrorMsg(i, buffer, len);
			cout << buffer << endl << endl;
	    }

		exit(-1);
	}

	// Normal completion of remote execution

	// De-serialize the executor's context
	translator.StartRead();
	pExecutor->LoadReply(translator);
	translator.EndRead();

	SaveTime(((CRUTestTaskExecutor*) pExecutor )->GetGroupId(),pid);

	cout << "Group : " << ((CRUTestTaskExecutor*) pExecutor )->GetGroupId() << " Process : " << pid + 1 ;
	cout << " is back in time " << CRUSQLComposer::TInt64ToStr(CRUGlobals::GetCurrentTimestamp()) << endl;
}


//--------------------------------------------------------------------------//
//	RefreshTestController::FindRunningExecutor()
//--------------------------------------------------------------------------//
CRUTaskExecutor *RefreshTestController::FindRunningExecutor(Lng32 pid)
{
	DSListPosition pos = executorsList_.GetHeadPosition();

	while (NULL != pos)
	{
		CRUTaskExecutor *pExecutor = executorsList_.GetNext(pos);
		if (pExecutor->GetProcessId() == pid)
			return pExecutor;
	}
	return NULL;
}


//--------------------------------------------------------------------------//
//	RefreshTestController::SaveTime()
//--------------------------------------------------------------------------//
void RefreshTestController::SaveTime(Int32 groupId,	
									 Int32 processId)
{
	CDMPreparedStatement *pStmt = 
		dynamicSQLContainer_.GetPreparedStatement(1);
	
	Int32 afterSyncInt = GetState() == EX_WAIT_FOR_ALL_RETURN_FROM_COMPILATION ? 0 : 1;
	pStmt->SetInt(1,groupId);
	pStmt->SetInt(2,processId + 1);
	pStmt->SetInt(3,afterSyncInt);

	transManager_.BeginTrans();

	pStmt->ExecuteUpdate();

	transManager_.CommitTrans();
}
