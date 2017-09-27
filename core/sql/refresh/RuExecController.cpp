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
* File:         RuExecController.cpp
* Description:  Implementation of class CRUExecController
*				
*
* Created:      05/07/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuExecController.h"

#include "RuTaskExecutor.h"
#include "RuGlobals.h"
#include "RuOptions.h"
#include "uofsException.h"

#ifdef NA_LINUX
const char * CRUExecController::taskServerName_ = "tdm_arkutp";
#endif
#ifdef NA_NSK
const char * CRUExecController::taskServerName_ = "mxutp";
#endif

//--------------------------------------------------------------------------//
//	Constructor
//--------------------------------------------------------------------------//

CRUExecController::CRUExecController(BOOL useParallelism) :
	inherited(),
	runningTaskList_(eItemsArentOwned),
        processPool_(CRUExecController::taskServerName_, 
	             CUOFsTaskProcess::MXUTP, 
		     CUOFsTaskProcess::OSS, 
		     TRUE),  // Launch processes on other CPUs
	useParallelism_(useParallelism)
{}

//--------------------------------------------------------------------------//
//	CRUExecController::HandleRequest()
//
//	The main request switch
//--------------------------------------------------------------------------//

void CRUExecController::HandleRequest(CRURuntimeControllerRqst *pRqst)
{
	try 
	{
		switch (pRqst->GetType())
		{
		case CRURuntimeControllerRqst::START_TASK:
			{
				HandleStartTaskRqst(pRqst);
				break;
			}

		case CRURuntimeControllerRqst::FINISH_TASK:
			{
				HandleFinishTaskRqst(pRqst);
				break;
			}

		case CRURuntimeControllerRqst::EXECUTE_TASK_STEP:
			{
				HandleExecuteTaskStepRqst(pRqst);
				break;
			}

		case CRURuntimeControllerRqst::AWAIT_EVENT:
			{
				HandleAwaitEventRqst();
				break;
			}

		default: RUASSERT(FALSE);	// Illegal request
		}
	}
	catch (CDSException &ex)
	{
		HandleRequestFailure(pRqst, ex);
		return;
	}
	catch (...) 
	{
		// Unknown error happened
		CRUException ex; // New exception object
		HandleRequestFailure(pRqst, ex);
		return;
	}
}

//--------------------------------------------------------------------------//
//	INDIVIDUAL REQUEST HANDLERS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUExecController::HandleStartTaskRqst()
//
//	 Start the task's execution
//	 (1) Build the task's executor and integrate the task 
//	     into the controller's data structures.
//	 (2) Ignite the task's execution by posting the first
//		 EXECUTE_TASK_STEP request to myself.
//
//--------------------------------------------------------------------------//

void CRUExecController::HandleStartTaskRqst(CRURuntimeControllerRqst *pRqst)
{
	CRUTask &task = pRqst->GetTask();

	task.SetRunning(TRUE);

	runningTaskList_.AddTail(&task);

	task.BuildExecutor();

	// Initiate the task's execution: post the EXECUTE_TASK_STEP to myself
	this->PostRequest(new CRURuntimeControllerRqst(
		CRURuntimeControllerRqst::EXECUTE_TASK_STEP, &task));
}

//--------------------------------------------------------------------------//
//	CRUExecController::HandleFinishTaskRqst()
//
//	 Finish the task's execution.
//	 (1) Remove the task from the controller's data structures.
//	 (2) Release the task's executor (which can occupy much memory).
//	 (3) Post the request to the flow controller to process the
//		 task's finish.
//
//--------------------------------------------------------------------------//

void CRUExecController::HandleFinishTaskRqst(CRURuntimeControllerRqst *pRqst)
{
	CRUTask &task = pRqst->GetTask();

	task.SetRunning(FALSE);

	DSListPosition pos = runningTaskList_.FindTaskPos(task.GetId());
	RUASSERT(NULL != pos);
	runningTaskList_.RemoveAt(pos);

	// If the task was executed remotely,
	// de-associate the process that handled it from the task
	DeAllocateTaskProcess(task);

	task.DeleteExecutor();

	// Tell the flow controller to process the task's completion
	GetPeerController()->PostRequest(new CRURuntimeControllerRqst(
		CRURuntimeControllerRqst::FINISH_TASK, &task));
}

//--------------------------------------------------------------------------//
//	CRUExecController::HandleExecuteTaskStepRqst()
//
//   Switch between the local and remote execution scenarios. 
//--------------------------------------------------------------------------//

void CRUExecController::HandleExecuteTaskStepRqst(CRURuntimeControllerRqst *pRqst)
{
	CRUTask &task = pRqst->GetTask();

	if (TRUE == useParallelism_
		&&
		TRUE == task.GetExecutor().IsNextStepRemotelyExecutable())
	{
		// The next step will be executed in the task process
		HandleRemoteTaskStepExecution(task);
	}
	else 
	{
		// The next step will be executed in arkcmp
		HandleLocalTaskStepExecution(task);
	}
}

//--------------------------------------------------------------------------//
//	CRUExecController::HandleAwaitEventRqst()
//
//	No work to do so far. Wait until some I/O from a task process happens.
//
//	When listening on the communication channel, ignore the transaction 
//	completion messages that TMF sends when the T-file is open.
//
//--------------------------------------------------------------------------//

void CRUExecController::HandleAwaitEventRqst()
{
	Lng32 pid;
	
	for (;;)
	{
		try 
		{
			pid = processPool_.ReceiveFromAnyProcess(-1); // An indefinite wait
			HandleReturnOfRemoteExecutor(pid);

			return;
		}
		catch (CUOFsTaskProcessPool::CUOFsUnknownProcessException &e)
		{
			// If it's the TMF completion message, then try to listen again
			CUOFsTransManager &tm = 
				CRUGlobals::GetInstance()->GetTransactionManager();

			if (tm.GetTMFFileNum() != e.GetFileNum())
			{
				throw e;	
			}
		}
	}
}

//--------------------------------------------------------------------------//
//	TASK STEP EXECUTION
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUExecController::HandleLocalTaskStepExecution()
//	
//	Execute the next step of execution in the main process.
//
//  If the step fails, or the task's execution completes, 
//	post the FINISH_TASK request to myself.
//	Otherwise, post the EXECUTE_TASK_STEP request to myself,
//	in order to initiate the next step.
//--------------------------------------------------------------------------//

void CRUExecController::HandleLocalTaskStepExecution(CRUTask &task)
{
	CRUTaskExecutor &taskEx = task.GetExecutor();
	CRURuntimeControllerRqst *pRqst;

	// The step has been completed successfully.
	// Check whether the task's execution is complete.
	if (CRUTaskExecutor::EX_COMPLETE == taskEx.GetState())
	{
		// Post a request to myself to handle the task's completion
		pRqst = new CRURuntimeControllerRqst(
					CRURuntimeControllerRqst::FINISH_TASK,
					&task
				);
		this->PostRequest(pRqst);
		return;
	}

	// If the task's previous step has left it in an 
	// unfinished transaction - switch back to it
	taskEx.SwitchTransContextBack();

	// The real action happens here: execute one step 
	taskEx.Work();

	// Post a new request to myself to continue the execution.
	pRqst = new CRURuntimeControllerRqst(
				CRURuntimeControllerRqst::EXECUTE_TASK_STEP,
				&task
			);
	this->PostRequest(pRqst);
}

//--------------------------------------------------------------------------//
//	CRUExecController::HandleRemoteTaskStepExecution()
//	
//	(1) Submit the task executor's context to the task process 
//	    that has been associated with it.
//	(2) Post a new scheduling request to the flow controller.
//
//--------------------------------------------------------------------------//

void CRUExecController::HandleRemoteTaskStepExecution(CRUTask &task)
{
	// If the task's previous step has left it 
	// in an unfinished transaction - switch back to it
	task.GetExecutor().SwitchTransContextBack();

	// Serialize the task executor's context
	// and send it to the task process
	ShipWorkToRemoteProcess(task);

	// Ask for a new task to perform
	GetPeerController()->PostRequest(
		new CRURuntimeControllerRqst(CRURuntimeControllerRqst::SCHEDULE)
	);
}

//--------------------------------------------------------------------------//
//	CRUExecController::HandleRequestFailure()
//--------------------------------------------------------------------------//

void CRUExecController::
HandleRequestFailure(CRURuntimeControllerRqst *pRqst, CDSException &ex)
{
	CRURuntimeControllerRqst::Type reqType = pRqst->GetType();

	switch (reqType)
	{
	case CRURuntimeControllerRqst::AWAIT_EVENT:
		{
			// If this is an AWAIT_EVENT request, this is a severe error.
			// The whole utility will fail.
			ex.SetError(IDS_RU_AWAITIO_FAILURE);
			throw ex;
		}
	case CRURuntimeControllerRqst::FINISH_TASK:
		{
			// If this is a FINISH_TASK request, then the error handler
			// has failed (obviously, following a software bug).
			// Do not queue the FINISH_TASK request anew, because
			// this will cause an endless loop. The whole utility will fail.
			throw ex;
		}
	case CRURuntimeControllerRqst::START_TASK:
	case CRURuntimeControllerRqst::EXECUTE_TASK_STEP:
		{
			// Otherwise, the error handling is more focused
			HandleTaskFailure(pRqst->GetTask(), ex);
			break;
		}
	default:
		{
			RUASSERT(FALSE);
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUExecController::HandleTaskFailure()
//	
//	When a task fails:
//	 (1) Copy the error message (described in the 
//	     exception object) to the task object.
//	 (2) If the task was in the middle of transaction,
//	     abort this transaction.
//	 (3) Post a FINISH_TASK request to myself.
//
//--------------------------------------------------------------------------//

void CRUExecController::HandleTaskFailure(CRUTask &task, CDSException &ex)
{
	// Stack a new error
	ex.SetError(IDS_RU_TASK_EX_FAILED);
	ex.AddArgument(task.GetTaskName());

	// Stamp the task as failed
	CDSException &errDesc = task.GetErrorDesc();
	errDesc = ex;	// Copy the error information

	// Release the dangling transaction ...
	task.GetExecutor().RollbackTransaction();

	// Post a request to myself to handle the task's completion
	this->PostRequest(new CRURuntimeControllerRqst(
		CRURuntimeControllerRqst::FINISH_TASK, &task));
}

//--------------------------------------------------------------------------//
//	CRUExecController::ShipWorkToRemoteProcess()
//
//	Serialize the task's context and send it to the process
//	associated with this task.
//--------------------------------------------------------------------------//

void CRUExecController::ShipWorkToRemoteProcess(CRUTask &task)
{
	CRUTaskExecutor &executor = task.GetExecutor();

	if (-1 == executor.GetProcessId())
	{
		// This is the first time that the executor will be shipped.

		// Associate the task with a task process (possibly creating it).
		AllocateTaskProcess(task);

		// The initial buffer allocation
		executor.AllocateBuffer();
	}

	// Pack the IPC message ...
	SerializeTaskExecutor(executor);

	// Send !
	processPool_[executor.GetProcessId()].Send(executor.GetTranslator());
}

//--------------------------------------------------------------------------//
//	CRUExecController::SerializeTaskExecutor()
//
//	Store the request in the UOFS buffer. If there is not 
//	enough memory - try to double the buffer until the 
//	UOFS message size limit is reached.
//
//--------------------------------------------------------------------------//

void CRUExecController::SerializeTaskExecutor(CRUTaskExecutor &executor)
{
	CUOFsIpcMessageTranslator *pTranslator = NULL;

	for (;;)
	{
		pTranslator = &(executor.GetTranslator());

		try 
		{
			// Try to serialize the executor until there is enough room
			pTranslator->StartWrite();
			executor.StoreRequest(*pTranslator);	
			pTranslator->EndWrite();

			break;
		}
		catch (CUOFsBufferOverFlowException &ex)
		{
			if (pTranslator->GetBufferSize() 
				>= 
				CUOFsIpcMessageTranslator::MaxMsgSize)
			{
				// No more room, propagate the exception
				ex.SetError(IDS_RU_REMOTE_EX_IMPOSSIBLE);
				ex.AddArgument(executor.GetParentTask()->GetTaskName());
				throw ex;
			}

			// The buffer can be resized up to the maximum 
			executor.ReAllocateBuffer(2 /*factor*/);
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUExecController::HandleRemoteExecutorCompletion()
//
//	The message has been received from the task process 
//	that was associated with the executor. 
//	Handle the success and failure cases.
//
//--------------------------------------------------------------------------//

void CRUExecController::HandleReturnOfRemoteExecutor(Lng32 pid)
{
	CRUTask *pTask = FindRunningTask(pid);
	RUASSERT(NULL != pTask);
	CRUTaskExecutor &executor = pTask->GetExecutor();

	CUOFsIpcMessageTranslator &translator = executor.GetTranslator();

	if (TRUE == translator.IsSystemMessage()
		// The task process crashed
		||
		CUOFsIpcMessageTranslator::APPLICATION_ERROR 
		== 
		translator.GetMessageType()
		// The task process remained alive but caught an exception
		)
	{
		HandleRemoteExecutorFailure(pTask, pid, translator);
	}
	else
	{
		HandleRemoteExecutorSuccess(pTask, translator);
	}
}

//--------------------------------------------------------------------------//
//	CRUExecController::HandleRemoteExecutorSuccess()
//--------------------------------------------------------------------------//

void CRUExecController::
HandleRemoteExecutorSuccess(CRUTask *pTask,
							CUOFsIpcMessageTranslator &translator)
{
	CRUTaskExecutor &executor = pTask->GetExecutor();

	// De-serialize the executor's context
	translator.StartRead();
	executor.LoadReply(translator);
	translator.EndRead();
	
	// Post a request to myself to continue the task's executor running
	CRURuntimeControllerRqst *pRqst = 
		new CRURuntimeControllerRqst(
						CRURuntimeControllerRqst::EXECUTE_TASK_STEP,
						pTask
						);

	this->PostRequest(pRqst);
}

//--------------------------------------------------------------------------//
//	CRUExecController::HandleRemoteExecutorFailure()
//--------------------------------------------------------------------------//
void CRUExecController::
HandleRemoteExecutorFailure(CRUTask *pTask,
							Lng32 pid,
							CUOFsIpcMessageTranslator &translator)
{
	CRUException ex;

	if (TRUE == translator.IsSystemMessage())
	{
		// The task process has crashed. 
		// No valuable information can be extracted.
		processPool_.HandleSystemMessage(translator, pid);
	}
	else
	{
		// De-serialize the exception object from the message
		translator.StartRead();
		ex.LoadData(translator);
		translator.EndRead();
	}
	
	ex.SetError(IDS_RU_REMOTE_EXECUTION_FAILURE);
	HandleTaskFailure(*pTask, ex);
}

//--------------------------------------------------------------------------//
//	GENERAL-PURPOSE METHODS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUExecController::FindRunningTask()
//
//	Locate a remotely executed task by pid.
//--------------------------------------------------------------------------//

CRUTask *CRUExecController::FindRunningTask(Lng32 pid)
{
	CRUTask *pTask = NULL;
	DSListPosition pos = runningTaskList_.GetHeadPosition();

	while (NULL != pos)
	{
		pTask = runningTaskList_.GetNext(pos);
		if (pTask->GetExecutor().GetProcessId() == pid)
		{
			break;
		}
	}	

	return pTask;
}

//--------------------------------------------------------------------------//
///	CRUExecController::AllocateTaskProcess()
//
//	If at least one step of the task will be executed 
//	in the task process, associate the task with the 
//	process that it will be executed in.
//
//  If the task process does not exist yet - create it.
//
//--------------------------------------------------------------------------//

void CRUExecController::AllocateTaskProcess(CRUTask &task)
{
	RUASSERT(TRUE == useParallelism_);
	
	Lng32 pid = processPool_.GetInactiveTaskProcessPid();
	if (-1 == pid)
	{
		// No inactive process, create a new server ...
		pid = InitiateTaskProcess();
	}

	task.GetExecutor().SetProcessId(pid);

	processPool_[pid].SetBusy(TRUE);
}

//-------------------------------------------------------------------//
//	CRUExecController::InitiateTaskProcess()
//
//	Activate a new arkutp server and return its process id.
//	The handshake protocol includes sending the serialized
//	CRUGlobals object to the server process.
//
//-------------------------------------------------------------------//

Lng32 CRUExecController::InitiateTaskProcess()
{
	Lng32 pid = processPool_.LaunchTaskProcess();
        
	char buffer[CRUOptions::PACK_BUFFER_SIZE];
        CUOFsIpcMessageTranslator translator(buffer, CRUOptions::PACK_BUFFER_SIZE);
        char *parentQid = CRUGlobals::GetInstance()->getParentQid();
        short len;
        if (parentQid != NULL)
          len = (short)strlen(parentQid);
        else
          len = 0;
        // Serialize the utility's command-line options ...
	translator.StartWrite();
        
	// Tell him his pid
	translator.WriteBlock(&pid,sizeof(Lng32));
        translator.WriteBlock(&len,sizeof(short));
        if (len > 0)
          translator.WriteBlock(parentQid, len);

	CRUGlobals::GetInstance()->GetOptions().StoreData(translator);

        translator.SetMessageType(CUOFsIpcMessageTranslator::RU_GLOBALS);

	translator.EndWrite();

	// Send them to the task process ...
	processPool_[pid].Send(translator);

	// And receive the reply
	try 
	{
		processPool_[pid].Receive();
	}
	catch (CUOFsException &e)
	{
		// Something wrong has happened during send
		processPool_[pid].Shutdown(FALSE /* don't send a message */);
		throw e;
	}

	// Verify that the answer was correct
	RUASSERT(
		translator.GetMessageType() == CUOFsIpcMessageTranslator::RU_GLOBALS
	);

	return pid;
}

//-------------------------------------------------------------------//
//	CRUExecController::DeAllocateTaskProcess()
//
//	De-associate the process from the task that it has been executing.
//-------------------------------------------------------------------//

void CRUExecController::DeAllocateTaskProcess(CRUTask &task)
{
	Lng32 pid = task.GetExecutor().GetProcessId();
	
	if (-1 == pid)
	{
		// The task was executed in the main process
		return;
	}
	
	processPool_[pid].SetBusy(FALSE);
}

//-------------------------------------------------------------------//
//	CRUExecController::ShutDownTaskProcesses()
//-------------------------------------------------------------------//

void CRUExecController::ShutDownTaskProcesses()
{
	processPool_.ShutdownAllTaskProcesses();
}
