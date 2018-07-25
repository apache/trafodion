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
* File:         RuTaskServerExecController.h
* Description:  Definition of class CRUTaskServerExecController 
*
* Created:      09/17/2000
* Language:     C++
*
*
*
******************************************************************************
*/
#include "refresh.h"
#include "RuException.h"
#include "RuTaskServerExecControler.h"
#include "uofsIpcMessageTranslator.h"
#include "RuAuditRefreshTaskExecutor.h"
#include "RuMultiTxnRefreshTaskExecutor.h"
#include "RuUnAuditRefreshTaskExecutor.h"
#include "RuDupElimTaskExecutor.h"
#include "RuLogCleanupTaskExecutor.h"
#include "RuTableSyncTaskExecutor.h"
#include "RuEmpCheckTaskExecutor.h"
#include "RuGlobals.h"
#include "RuJournal.h"

#ifdef _DEBUG
#include "RuTestTaskExecutor.h"	
#include <fstream>
#if !defined(NA_NSK)
#include <process.h>
#endif
#endif

CRUTaskServerExecController* CRUTaskServerExecController::pInstance_ = NULL;

#ifdef _DEBUG
#if defined(NA_WINNT)
static void DisplayDebugBox(Int32 num)
{
  Int32 pid = _getpid();
  char stmp[256];
  char title[10];
  _snprintf(title, sizeof(title), "MXUTP %d", num);
  _snprintf(stmp, sizeof(stmp), "Process Launched %d", pid);
  MessageBox(NULL, stmp , title, MB_OK|MB_ICONINFORMATION);
}
#endif
#endif

// This class is not used at all.

//--------------------------------------------------------------------------//
//	CRUTaskServerExecController constructors & destructors
//--------------------------------------------------------------------------//

CRUTaskServerExecController::CRUTaskServerExecController() : 
	pTranslator_(NULL), // Passed an owned by tdm_arkutp main proc.
	pTaskExecutor_(NULL),
	pJournal_(NULL)
#ifdef _DEBUG
	, logfile_(NULL)
	, isLogging_(FALSE)
#endif
        , parentQid_(NULL)
{}

CRUTaskServerExecController::~CRUTaskServerExecController() 
{
	delete pJournal_;
	delete pTaskExecutor_;

#ifdef _DEBUG
	if (isLogging_)
	{
	  delete logfile_;
	}
#endif
        if (parentQid_)
          delete parentQid_;
}

//--------------------------------------------------------------------------//
//	CRUTaskServerExecController::Init()
//--------------------------------------------------------------------------//

void CRUTaskServerExecController::Init(CUOFsIpcMessageTranslator &translator,
									   CUOFsTransManager &transManager_)
{
      	if (NULL == pInstance_)
	{
		pInstance_ =  new CRUTaskServerExecController();
	}

	// Find out which UTP process I am.
	Lng32 pid;
        short len;
	translator.ReadBlock(&pid,sizeof(Lng32));
        translator.ReadBlock(&len,sizeof(short));
        if (len != 0)
        {
          pInstance_->parentQid_ = new char[len+1];
          translator.ReadBlock(pInstance_->parentQid_, len);
          pInstance_->parentQid_[len] = '\0';
        }
        else
          pInstance_->parentQid_ = NULL;
#ifdef _DEBUG
	if (getenv("SQL_UTP_LOG") != NULL)
	{
	  pInstance_->isLogging_ = TRUE;
	  pInstance_->createLogFile(pid);
	}
  	if (getenv("DEBUG_MXUTP_SERVER") != NULL) 
	{
//	  _asm { int 3 };
	  #ifdef NA_WINNT
	    DisplayDebugBox(pid);
	  #else
	    NADebug();
	  #endif
	}
#endif
        CRUGlobals::setParentQidAtSession(pInstance_->parentQid_);
	pInstance_->options_.LoadData(translator);
	CDSString filename = pInstance_->options_.GetOutputFilename();

        if( ! filename.IsEmpty() )
        {          
          if( -1 != filename.Find('.') )
          {
            // try to break up the filename into name and ext
            CDSStringList *nameList = filename.Tokenize('.');
          
            // the name is broken into two parts 
            RUASSERT( 2 == nameList->GetCount() );

            // and the pid is added before the ext
            filename = *((CDSString*)nameList->GetAt(0));
            filename += pid;
            filename += ".";
            filename += *((CDSString*)nameList->GetAt(1));
          }
          else
          {
            // if there is no ext given, just add the pid
            // to the end of the filename
            filename += pid;
          }                             
        }

        pInstance_->options_.SetOutputFilename(filename);
        pInstance_->pJournal_ = new CRUJournal(filename);
        pInstance_->pJournal_->Open();
        
	
	CRUGlobals::Init(pInstance_->options_,
					*(pInstance_->pJournal_),
					transManager_);
	// We send here an empty message ,just for completing the two-way 
	// communication.
	translator.StartWrite();
	translator.SetMessageType(CUOFsIpcMessageTranslator::RU_GLOBALS);
	translator.EndWrite();

	// Crash the remote process 
	TESTPOINT_SEVERE(CRUGlobals::REMOTE_CRASH_IN_REMOTE_CONTROLLER);
}

//--------------------------------------------------------------------------//
//	CRUTaskServerExecController::LoadExecutor()
//
//  This function creates the proper refresh executor by the message type
//--------------------------------------------------------------------------//

void CRUTaskServerExecController::
	LoadExecutor(CUOFsIpcMessageTranslator &translator)
{
	try 
	{
		pTranslator_ = &translator;

		switch (translator.GetMessageType())
		{
			case CUOFsIpcMessageTranslator::RU_AUDIT_REFRESH_EXECUTOR:
				{
					RUASSERT(NULL == pTaskExecutor_);	
					pTaskExecutor_ = new CRUAuditRefreshTaskExecutor();
				}
				break;
			case CUOFsIpcMessageTranslator::RU_UNAUDIT_REFRESH_EXECUTOR:
				{
					RUASSERT(NULL == pTaskExecutor_);	
					pTaskExecutor_ = new CRUUnAuditRefreshTaskExecutor();
				}
				break;
			case CUOFsIpcMessageTranslator::RU_MULTI_TXN_REFRESH_EXECUTOR:
				{
					RUASSERT(NULL == pTaskExecutor_);	
					pTaskExecutor_ = new CRUMultiTxnRefreshTaskExecutor();
				}
				break;
			case CUOFsIpcMessageTranslator::RU_DE_EXECUTOR:
				{
					RUASSERT(NULL == pTaskExecutor_);	
					pTaskExecutor_ = new CRUDupElimTaskExecutor();
				}
				break;
			case CUOFsIpcMessageTranslator::RU_LOG_CLEANUP_EXECUTOR:
				{
					RUASSERT(NULL == pTaskExecutor_);	
					pTaskExecutor_ = new CRULogCleanupTaskExecutor();
				}
				break;
			case CUOFsIpcMessageTranslator::RU_TABLE_SYNC_EXECUTOR:
				{
					RUASSERT(NULL == pTaskExecutor_);	
					pTaskExecutor_ = new CRUTableSyncTaskExecutor();
				}
				break;
			case CUOFsIpcMessageTranslator::RU_EMP_CHECK_EXECUTOR:
				{
					RUASSERT(NULL == pTaskExecutor_);	
					pTaskExecutor_ = new CRUEmpCheckTaskExecutor();	
				}
				break;
			case CUOFsIpcMessageTranslator::RU_TEST_SYNC:
				{
					RUASSERT(NULL != pTaskExecutor_);	
				}
				break;
			case CUOFsIpcMessageTranslator::RU_TEST_EXECUTOR:
			#ifdef _DEBUG
				{
					RUASSERT(NULL == pTaskExecutor_);	
					pTaskExecutor_ = new CRUTestTaskExecutor();	
				}
				break;
			#endif
			default: RUASSERT(FALSE);

		}
		
		pTaskExecutor_->LoadRequest(translator);
	}
	catch (CDSException &ex)
	{
		HandleExecutorFailure(ex);
	}
	catch (...) 
	{
		// Unknown error happened
		CRUException ex; // New exception object
		ex.SetError(IDS_RU_UNEXPECTED);
		HandleExecutorFailure(ex);
	}
}

//--------------------------------------------------------------------------//
//	CRUTaskServerExecController::Run()
//--------------------------------------------------------------------------//

void CRUTaskServerExecController::Run()
{
	try 
	{
#ifdef _DEBUG
		if (isLogging_)
		{
			*logfile_ << "Start working on request." << endl;
		}
#endif
		while (TRUE == pTaskExecutor_->IsNextStepRemotelyExecutable())
		{
			// The real action happens here: execute one step 
			pTaskExecutor_->Work();
		}

#ifdef _DEBUG
		// This section is for the refresh test only and is used
		// for syncronizing the tests
		if (CUOFsIpcMessageTranslator::RU_TEST_EXECUTOR ==
			pTranslator_->GetMessageType())
		{
			if (CRUTaskExecutor::EX_COMPLETE != 
				pTaskExecutor_->GetState())
			{
				// Write the return message without
				// deleting the executor
				pTranslator_->StartWrite();
				pTaskExecutor_->StoreReply(*pTranslator_);
				pTranslator_->EndWrite();
				pTranslator_->SetMessageType(
					CUOFsIpcMessageTranslator::RU_TEST_SYNC);
				return;
			}
		}

#endif
		// Write the return message and delete the executor
		pTranslator_->StartWrite();
		pTaskExecutor_->StoreReply(*pTranslator_);
		pTranslator_->EndWrite();

		delete pTaskExecutor_;
		pTaskExecutor_ = NULL;
#ifdef _DEBUG
		if (isLogging_)
		{
			*logfile_ << "Finished work." << endl;
		}
#endif

		return;
	}
	catch (CDSException &ex)
	{
		HandleExecutorFailure(ex);
		return;
	}
	catch (...) 
	{
		// Unknown error happened
		CRUException ex; // New exception object
		ex.SetError(IDS_RU_UNEXPECTED);
		HandleExecutorFailure(ex);
		return;
	}
}

//--------------------------------------------------------------------------//
//	CRUTaskServerExecController::HandleExecutorFailure()
//--------------------------------------------------------------------------//
void CRUTaskServerExecController::HandleExecutorFailure(CDSException &ex)
{
	// First lets rollback the transaction and clear the memory
	if (NULL != pTaskExecutor_)
	{
#ifdef _DEBUG
		if (isLogging_)
		{
			logError(ex);
		}
#endif
		pTaskExecutor_->RollbackTransaction();
		delete pTaskExecutor_;
		pTaskExecutor_ = NULL;
	}

#ifdef _DEBUG
#ifdef NA_NSK
	pJournal_->LogError(ex);
#endif
#endif		
	// Then we should send back the exception object

	pTranslator_->StartWrite();
	
	pTranslator_->SetMessageType(CUOFsIpcMessageTranslator::APPLICATION_ERROR);
	
	CRUException ruEx(ex);

	ruEx.StoreData(*pTranslator_);

	pTranslator_->EndWrite();

	throw CRURemoteExecutorFailureException();
}

#ifdef _DEBUG
void CRUTaskServerExecController::createLogFile(Lng32 pid)
{
	char streamName[30];
	sprintf(streamName, "refresh.utp%d", pid);
	logfile_ = new ofstream(streamName, ios::app);
#if !defined(NA_LINUX) && defined(NA_ASSIGNABLE_IOSTREAMS) 
	cout = *logfile_; // direct standard output to the file
#else
	cout.rdbuf(logfile_->rdbuf());
#endif
	*logfile_ << "Init." << endl;
}

void CRUTaskServerExecController::logError(CDSException &ex)
{
	enum { BUFSIZE = 4096 };

	Int32 nerr = ex.GetNumErrors();
	char buffer[BUFSIZE];
	CDSString msg;

	for (Int32 i=0; i<nerr; i++) 
	{
		ex.GetErrorMsg(i, buffer, BUFSIZE);

		if (buffer[0] != 0)
		{
			// Clear the trailing whitespace
			char *p = buffer + strlen(buffer) - 1;
			for (;buffer != p && isspace((unsigned char)*p); p--, *p=0); // For VS2003
		}

		msg += buffer + CDSString("\n");
	}
	*logfile_ << msg << endl;
	*logfile_ << "Abnormal termination!" << endl;
}

#endif

