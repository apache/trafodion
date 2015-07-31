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
********************************************************************/
#ifdef ASYNCIO

#include <asyncIO.h>
#include <diagfunctions.h>
#include <drvrglobal.h>
#include <sql.h>
#include <cstmt.h>
#include <cconnect.h>
#include <errno.h>
#include <assert.h>
#include "odbcas_cl.h"

#if defined (MXHPUXIA)
extern int unaligned_access_count;
extern "C" void allow_unaligned_data_access();
#endif

/*****************************************************************************
 *                                                                           *
 *                  Async_Mutex class members                                *
 *                                                                           *
 ****************************************************************************/

//
//  Async_Mutex class constructor
//
Async_Mutex::Async_Mutex()
{
	int status;
	initialized = false;

	status = pthread_mutexattr_init(&attr);
	if (status != 0)
	{
		#ifdef __DEBUGASYNCIO
		printf("ASYNCIO: Async_Mutex(): pthread_mutexattr_init failed with error %d\n", status);
		assert(0);
		#endif

		return;
	}
#ifdef MXOSS
    status = pthread_mutexattr_setkind_np(&attr, MUTEX_NONRECURSIVE_NP);
#else
	status = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
#endif
	if (status != 0)
	{
		#ifdef __DEBUGASYNCIO
		printf("ASYNCIO: Async_Mutex(): pthread_mutexattr_settype failed with error %d\n", status);
		assert(0);
		#endif

		return;
	}
	status = pthread_mutex_init(&mutex, &attr);
	if (status != 0)
	{
		#ifdef __DEBUGASYNCIO
		printf("ASYNCIO: Async_Mutex(): pthread_mutex_init failed with error %d\n", status);
		assert(0);
		#endif

		return;
	}

	initialized = true;

} // Async_Mutex::Async_Mutex()

//
// Acquires a Lock
//
int Async_Mutex::Lock()
{
	int status;

	status = pthread_mutex_lock(&mutex);

	if (status != 0 && status != EDEADLK)
	{
		#ifdef __DEBUGASYNCIO
		printf("ASYNC: Async_Mutex: pthread_mutex_lock() call failed with error = %d\n",status);
		assert(0);
		#endif

		return status;
	}

	return SQL_SUCCESS;

} // Async_Mutex::lock()

//
// Try to acquire a Lock:
// The method tries to lock the specified mutex. If the mutex is already locked, an error is returned (EBUSY). 
// Otherwise, this method returns with the mutex in the locked state with the calling thread as its owner.
//

int Async_Mutex::TryLock()
{
	int status;

	status = pthread_mutex_trylock(&mutex);

	if (status != 0 && status != EBUSY)
	{
		#ifdef __DEBUGASYNCIO
		printf("ASYNC: Async_Mutex: pthread_mutex_trylock() call failed with error = %d\n",status);
		assert(0);
		#endif

	}

	return status;

} // Async_Mutex::lock()

//
//  Frees the Lock on a mutex
//
int Async_Mutex::UnLock()
{
	int status;

	status = pthread_mutex_unlock(&mutex);

	if (status != 0)
	{
		#ifdef __DEBUGASYNCIO
		printf("ASYNC: Async_Mutex: pthread_mutex_unlock() call failed with error = %d\n",status);
		assert(0);
		#endif

		return status;
	}

	return SQL_SUCCESS;

} // Async_Mutex::lock()

//
// Async_Mutex destructor
//
Async_Mutex::~Async_Mutex()
{
	pthread_mutexattr_destroy(&attr);
	pthread_mutex_destroy(&mutex);

} // Async_Mutex::~Async_Mutex()

/*****************************************************************************
 *                                                                           *
 *                  Async_Condtion class members                             *
 *                                                                           *
 ****************************************************************************/

//
// Async_Condition constructor
//
Async_Condition::Async_Condition()
{
	int status;
	initialized = false;

	condLock = new Async_Mutex();

	if(condLock == NULL || !condLock->IsInitialized())
	{
		#ifdef __DEBUGASYNCIO
		printf("ASYNC: Async_Condition: unable to initialize mutex\n");
		assert(0);
		#endif

		delete condLock;
		condLock = NULL;
		return;
	}

	status = pthread_cond_init(&cond, NULL);

	if (status != 0)
	{
		#ifdef __DEBUGASYNCIO
		printf("ASYNCIO: Async_Condition(): pthread_cond_init() failed with error %d\n", status);
		assert(0);
		#endif


		delete condLock;
		condLock = NULL;
		return;
	}

	initialized = true;

} // Async_Condition::Async_Condition()

//
// Async_Condition class destructor
//
Async_Condition::~Async_Condition()
{
	delete condLock;
	pthread_cond_destroy(&cond);

} // Async_Condition::~Async_Condition()

//
// Async_Condition class method to wait on a condition variable
//
int Async_Condition::Wait(const struct timespec *timeout)
{
	int status;
	struct timespec abstimeout;

	if((timeout == NULL) || (timeout != NULL && timeout->tv_sec == 0 && timeout->tv_nsec == 0))
		status = pthread_cond_wait(&cond, condLock->GetLock());
	else
	{

#ifdef MXOSS
        pthread_get_expiration_np((struct timespec*)timeout,&abstimeout);
#else
        clock_gettime(CLOCK_REALTIME,&abstimeout);

		if((abstimeout.tv_nsec += timeout->tv_nsec) >= 1000000000L) 
		{
		   abstimeout.tv_nsec -= 1000000000L;
		   abstimeout.tv_sec += 1;
		}
		abstimeout.tv_sec += timeout->tv_sec;
#endif		
		status = pthread_cond_timedwait(&cond, condLock->GetLock(),&abstimeout);
	}

	if(status != 0 && status != ETIMEDOUT)
	{
		#ifdef __DEBUGASYNCIO
		printf("ASYNCIO: Async_Condition(): pthread_cond_wait() failed with error %d\n", status);
		assert(0);
		#endif

	}

	return status;

} // Async_Condition::wait()


//
// Async_Condition class method to signal a condition variable
//
int Async_Condition::Signal()
{
	int status;

	status = pthread_cond_signal(&cond);

	if(status != 0)
	{
		#ifdef __DEBUGASYNCIO
		printf("ASYNCIO: Async_Condition(): pthread_cond_signal() failed with error %d\n", status);
		assert(0);
		#endif

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "Async_Condition: pthread_cond_signal() returned %d",status);
	}

	return status;

} // Async_Condition::Signal()

/*****************************************************************************
 *                                                                           *
 *                  AsyncIO_Thread class members                             *
 *                                                                           *
 ****************************************************************************/

//
// AsyncIO_Thread class destructor
//
AsyncIO_Thread::~AsyncIO_Thread()
{
	int status;

	if(initialized == true)
	{
		#ifdef __DEBUGASYNCIO
		printf("~AsyncIO_Thread(): destroying thread for connection handle %x\n",connection);
		#endif

		status = pthread_cancel(thread);

		if(status != 0)
		{
			#ifdef __DEBUGASYNCIO
			printf("~AsyncIO_Thread(): pthread_cancel failed with error %d\n",status);
			#endif
		}

		#ifdef __DEBUGASYNCIO
		printf("~AsyncIO_Thread(): pthread_join() on thread for connection handle %x\n",connection);
		#endif

		status = pthread_join(thread,NULL);
		if(status != 0)
		{
			#ifdef __DEBUGASYNCIO
			printf("~AsyncIO_Thread(): pthread_join failed with error %d\n",status);
			#endif
		}

		#ifdef __DEBUGASYNCIO
		printf("~AsyncIO_Thread(): pthread_join() completed with status %d for connection handle %x\n",status,connection);
		#endif

		status = pthread_attr_destroy(&attr);
		if(status != 0)
		{
			#ifdef __DEBUGASYNCIO
			printf("~AsyncIO_Thread(): pthread_attr_destroy failed with error %d\n",status);
			#endif
		}

		while(!stmtQueueEmpty())
			stmtQueuePop();
		
		delete MutexLock;
		delete conditionVar;
	}

} // AsyncIO_Thread::~AsyncIO_Thread()

//
// AsyncIO_Thread class constructor
//
AsyncIO_Thread::AsyncIO_Thread(CConnect *connection)
{
#if defined (MXHPUXIA)
        allow_unaligned_data_access();
#endif
	int status;
	initialized = false;
	char buffer[256];

	this->connection = connection;

	status = pthread_attr_init(&attr);

	if (status !=0)
	{
		#ifdef __DEBUGASYNCIO
		printf("ASYNCIO: AsyncIO_Thread(): pthread_attr_init failed with error %d for Connection handle %x\n", status, connection);
		assert(0);
		#endif

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncIO_Thread: pthread_attr_init() returned %d for connection handle %x",status, connection);

		sprintf(buffer," pthread_attr_init() failed with error %d for connection handle %x", status, connection); 
		this->connection->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		return;
	}
#if defined(MXHPUX)
	// the default stack size on hpux is 256k. If compression is ON, this will cause issues since the hash tables used 
	// is around this size.
	pthread_attr_setstacksize(&attr,1048576);
#endif

	MutexLock = new Async_Mutex();
	if(MutexLock == NULL || !MutexLock->IsInitialized())
	{
		delete MutexLock;
		MutexLock = NULL;
		pthread_attr_destroy(&attr);

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncIO_Thread: failed to initialize mutex for connection handle %x",connection);

		sprintf(buffer," failed to initialize mutex for connection handle %x",connection); 
		connection->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		return;
	}

	conditionVar = new Async_Condition();
	if(conditionVar == NULL || !conditionVar->IsInitialized())
	{
		delete MutexLock;
		MutexLock = NULL;
		delete conditionVar;
		conditionVar = NULL;

		pthread_attr_destroy(&attr);

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncIO_Thread: failed to initialize condition variable for connection handle %x",connection);

		sprintf(buffer," failed to initialize condition variable for connection handle %x",connection); 
		connection->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		return;
	}

} // AsyncIO_Thread::AsyncIO_Thread()

//
// Creates the running thread with entry point as the function ptr IOfunction
//
int AsyncIO_Thread::Create(PTHR_FUNCTION IOfunction)
{
	int  status;
	char buffer[256];
#ifdef MXOSS
    thread.field1=NULL;
    thread.field2=0;
    thread.field3=0;
#else
	thread = 0;
#endif
	stmtHandle = NULL;

	#ifdef __DEBUGASYNCIO
	printf("ASYNCIO: Create() called for Connection handle %x\n",connection);
	#endif

	if((conditionVar == NULL) || (MutexLock == NULL) ||
		!conditionVar->IsInitialized() || !MutexLock->IsInitialized())
	{
		#ifdef __DEBUGASYNCIO
		printf("ASYNCIO: Create() condition variable and mutexes not initialized for connection %x\n",connection);
		#endif

		return SQL_ERROR;
	}

	status = pthread_create( &thread,      // AsyncIO thread
							 &attr,        // Thread attributes
							 IOfunction,   // Entry point for the IO thread
							 connection);  // Thread arguments
	if (status != 0)
	{
		#ifdef __DEBUGASYNCIO
		printf("ASYNCIO: Create(): pthread_create failed with error %d for Connection handle %x\n", status, connection);
		#endif

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncIO_Thread: pthread_create() returned %d for connection handle %x",status, connection);

		sprintf(buffer," pthread_create() failed with error %d for connection handle %x", status,connection); 
		connection->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);
		return status;
	}

/*
	status = pthread_detach(thread);
	if (status != 0)
	{
		#ifdef __DEBUGASYNCIO
		printf("ASYNCIO: Create(): pthread_detach() failed with error %d for Connection handle %x\n", status, connection);
		#endif

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncIO_Thread: pthread_detach() returned %d for connection handle %x",status,connection);

		sprintf(buffer," pthread_attr_detch() failed with error %d for connection handle %x", status, connection); 
		connection->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		pthread_attr_destroy(&attr);
		delete conditionVar;

		return status;
	}
*/


#ifdef __DEBUGASYNCIO
	printf("ASYNCIO: Create(): completed for Connection handle %x\n",connection);
#endif

	sleep(0);
	sched_yield(); // give the new thread a chance to run
				   // The newly created thread will mark initialization as complete


	return SQL_SUCCESS;

} // AsyncIO_Thread::Create()

//
// Check if the statement queue is empty
//
bool AsyncIO_Thread::stmtQueueEmpty() 
{ 
	bool empty = true;

	if(this->LockMutex() == SQL_SUCCESS)
	{
		empty =  stmtQueue.empty(); 
		this->UnLockMutex();
	}

	return empty;

} // AsyncIO_Thread::stmtQueueEmpty()

//
// Push a statement handle to be processed into the queue
//
int AsyncIO_Thread::stmtQueuePush(CStmt *stmt)
{
	if(this->LockMutex() == SQL_SUCCESS)
	{
		stmtQueue.push_back(stmt);
		this->UnLockMutex();
		return SQL_SUCCESS;
	}

	return SQL_ERROR;

} // AsyncIO_Thread::stmtQueuePop()


//
// Pop a statement handle to be processed into the queue
//
CStmt* AsyncIO_Thread::stmtQueuePop()
{
	CStmt *stmt;

	if(this->LockMutex() == SQL_SUCCESS)
	{
		stmt = stmtQueue.front();
		stmtQueue.pop_front();
		
		this->UnLockMutex();
		return stmt;
	}

	return NULL;

} // AsyncIO_Thread::stmtQueuePop()


//
// Cancel on a statement
//
int AsyncIO_Thread::Cancel(CStmt *stmt)
{
	char buffer[256];
	#ifdef __DEBUGASYNCIO
	printf("ASYNCIO: Cancel(): called for statement handle %x, connection handle %x failed\n", stmt, connection);
	#endif

	if(this->LockMutex() == SQL_SUCCESS)
	{
		if(stmtHandle == stmt)
		{
			// This statement is currently executing, stop the MXOSRVR
			#ifdef __DEBUGASYNCIO
			printf("ASYNCIO: Cancel(): sendStopServer() for statement handle %x, connection handle %x\n", stmt, connection);
			#endif
			odbcas_ASSvc_StopSrvr_exc_ stopSrvrException;
			stopSrvrException.exception_nr=SQL_ERROR;
			stmtHandle->sendStopServer(&stopSrvrException);
			stmtHandle = NULL;
			stmt->setAsyncCancelled(TRUE);
			this->UnLockMutex();
			if(stopSrvrException.exception_nr==SQL_ERROR)
			{
				stmt->setDiagRec(DRIVER_ERROR, IDS_08_005, 0, "Network I/O failure");
			}
			else if(stopSrvrException.exception_nr==SQL_SUCCESS)
			{
				return SQL_SUCCESS;
			}
			else if(stopSrvrException.exception_nr==odbcas_ASSvc_StopSrvr_ASNotAvailable_exn_)
			{
				stmt->setDiagRec(DRIVER_ERROR, IDS_HY_018, 0, stopSrvrException.u.ASNotAvailable.ErrorText);
			}
			else
			{
				stmt->setDiagRec(DRIVER_ERROR, IDS_HY_018, 0, stopSrvrException.u.ASParamError.ErrorText);
			}
			return SQL_ERROR;
		}

		// Check if this statement handle is queued up waiting for the connection to be free
		for(int i = 0; i < stmtQueue.size(); i++)
		{
			if(stmtQueue.at(i) == stmt)
			{
				#ifdef __DEBUGASYNCIO
				printf("ASYNCIO: Cancel(): removing statement handle %x from queue for connection handle %x\n", stmt, connection);
				#endif

				stmtQueue.erase(stmtQueue.begin() +i);
				this->UnLockMutex();
				stmt->setAsyncCancelled(TRUE);
				stmt->setStmtExecState(STMT_EXECUTION_NONE);
				return SQL_SUCCESS;
			}
		}
	}

	#ifdef __DEBUGASYNCIO
	printf("ASYNCIO: Cancel(): for statement handle %x for connection handle %x failed\n", stmt, connection);
	#endif

	if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
		TraceOut(TR_ODBC_ERROR, "ASYNCIO: Cancel() failed for statement handle %x, connection handle %x",stmt,connection);

	sprintf(buffer," Cancel() failed for statement %x connection handle %x", stmt, connection); 
	stmt->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

	this->UnLockMutex();

	return SQL_ERROR;
} // AsyncIO_Thread::Cancel()


//
// This function is the entry point for the Async IO thread (one per connection)
//    - Waits to be signalled
//    - If there are enqueued work items (statement handles), then process them till they're all done
//    - Wait to be signalled again
//

extern "C"
void* AsyncIOThread(void *pParam)
{
	int status;
	int returnCode;
	CConnect *connectObject = (CConnect*)pParam;
	CStmt *statementObject = NULL;

	#ifdef __DEBUGASYNCIO
	printf("AsyncIOThread: ENTER for Connection handle %x, threadID = %d\n",connectObject,pthread_self());
	#endif

	// Lock the mutex associated with the condition variable -
	// the Wait on the condition will atomically release it
	status = connectObject->m_async_IOthread->LockCondition();

	if (status != 0)
	{
		#ifdef __DEBUGASYNCIO
		printf("AsyncIOThread: LockCondition() failed with error = %d\n",status);
		assert(0);
		#endif

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncIO_Thread: LockCondition() returned %d for connection handle %x",status,connectObject);

		pthread_exit(NULL);
	}

	// Once we're here we can say that initialization is done
	connectObject->m_async_IOthread->SetInitialized(true);

	//
	// Loop forever waiting for the next IO
	//
	while(1)
	{
		//
		// Wait for Condition to be signalled
		//
		#ifdef __DEBUGASYNCIO
		printf("AsyncIOThread: Connection handle %x, Thread %d waiting for condition\n",connectObject,pthread_self());
		#endif

		status = connectObject->m_async_IOthread->WaitCondition();

		if (status != 0)
		{
			#ifdef __DEBUGASYNCIO
			printf("AsyncIOThread: Wait() failed with error = %d\n",status);
			assert(0);
			#endif
			if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
				TraceOut(TR_ODBC_ERROR, "AsyncIO_Thread: WaitCondition() returned %d for connection handle %x",status,connectObject);

			continue;
		}

		#ifdef __DEBUGASYNCIO
		printf("AsyncIOThread: Connection %x signalled\n",connectObject);
		#endif

		while(!connectObject->m_async_IOthread->stmtQueueEmpty())
		{
			//
			// Process all the Queued up Statement handles
			// 

			statementObject = connectObject->m_async_IOthread->stmtQueuePop();

			if(statementObject == NULL)
			{
				// Should not happen since we check if the queue is empty or not before coming here
				// but just in case ...
				#ifdef __DEBUGASYNCIO
				printf("AsyncIOThread: NULL statement object returned from queue for connection %x\n",connectObject);
				assert(0);
				#endif

				if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
					TraceOut(TR_ODBC_ERROR, "AsyncIO_Thread: WaitCondition() returned %d for connection handle %x",status,connectObject);

				continue; // get the next statement handle from the queue
			}


			//connectObject->setSrvrCallContext(statementObject->getSrvrCallContext());
			connectObject->m_async_IOthread->setStatementHandle(statementObject);

			#ifdef __DEBUGASYNCIO
			printf("AsyncIOThread: Processing Statement %x on Connection %x\n",statementObject,connectObject);
			#endif


			returnCode = ThreadControlProc(statementObject->getSrvrCallContext());

			#ifdef __DEBUGASYNCIO
			printf("AsyncIOThread: ThreadControlProc() exited with return code %d for Connection %x, statement %x\n",returnCode,connectObject,connectObject->m_async_IOthread->getStatementHandle());
			#endif

			//
			// Mark the Statement State as 'Execution Done' and set the return code for the statement object
			statementObject->setAsyncOperationReturnCode(returnCode);
			statementObject->setStmtExecState(STMT_EXECUTION_DONE);

#ifdef DBSELECT
			if(gDrvrGlobal.gdbSelect != NULL && gDrvrGlobal.gdbSelect->IsEnabled())
			{
				status = gDrvrGlobal.gdbSelect->LockMutex();

				//
				// Update the status array for the Connections/Statement handles which have completed if there is space for it in the status array
				// Note: If the application does not call dbSelect periodically, the status array could fill up and some notifications could be lost
				//

				if(gDrvrGlobal.gdbSelect->GetNumIOCompleted() + 1 <= gDrvrGlobal.gdbSelect->GetQueueLen())
				{
					gDrvrGlobal.gdbSelect->AddCompletion((SQLHANDLE)connectObject,(SQLHANDLE)statementObject);

				}
				else
				{
					#ifdef __DEBUGASYNCIO
					printf("AsyncIOThread: dbSelect Array full. Cannot record the completion status for %sn",connectObject->getSrvrObjRef());
					#endif

					//
					// Log Error Message
				}
				status = gDrvrGlobal.gdbSelect->UnLockMutex();

				//
				// Notify the application if its blocked on a dbSelect

				if(gDrvrGlobal.gdbSelect->IsWaiting())
				{
					#ifdef __DEBUGASYNCIO
					printf("AsyncIOThread: application waiting on dbSelect\n");
					#endif

					// application has called dbSelect to get notified

					status = gDrvrGlobal.gdbSelect->LockCondition();
					status = gDrvrGlobal.gdbSelect->SignalCondition();
					status = gDrvrGlobal.gdbSelect->UnLockCondition();

					#ifdef __DEBUGASYNCIO
					printf("AsyncIOThread: dbSelect signalled\n");
					#endif
				}
			}
#endif /* DBSELECT */
			
			// Reset the statement handle associated with this async thread to NULL
			connectObject->m_async_IOthread->setStatementHandle(NULL);

		} // while !connectObject->m_async_IOthread->stmtQueueEmpty

	} // while(1)

	return(0);

} // AsyncIOThread()

//
// Helper routine to start Async Execution on a statement handle
//
extern "C"
SQLRETURN StartAsyncOperation(CConnect *ConnectionObject, CStmt *StatementObject)
{
		int	 status;
		char buffer[256];

		#ifdef __DEBUGASYNCIO
		printf("StartAsyncOperation: for Statement handle %x, and Connection handle %x\n",StatementObject,ConnectionObject);
		#endif

		#ifdef __DEBUGASYNCIO
		printf("StartAsyncOperation: Queing Statement %x\n",StatementObject);
		#endif
		//
		// Queue work item
		ConnectionObject->m_async_IOthread->stmtQueuePush(StatementObject);
		StatementObject->setStmtExecState(STMT_EXECUTION_EXECUTING);

		#ifdef __DEBUGASYNCIO
		printf("StartAsyncOperation: Statement %x, Try Acquiring lock for Connection %x\n",StatementObject,ConnectionObject);
		#endif


		status = ConnectionObject->m_async_IOthread->TryLockCondition();

		if(status != 0 && status != EBUSY)
		{
			#ifdef __DEBUGASYNCIO
			printf("StartAsyncOperation: Statement %x, Connection Object %x, Lock() failed with error %d\n",
				StatementObject, ConnectionObject,status);
			assert(0);
			#endif

			sprintf(buffer," Start asynchronous operation failed for statement handle %x because Lock() failed with %d for connection handle %x",
				StatementObject,status,ConnectionObject); 
			StatementObject->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);
			return SQL_ERROR;
		}

		// We've already que'ed up the work for the thread, so we can return here
		if(status == EBUSY)
		{
			#ifdef __DEBUGASYNCIO
			printf("StartAsyncOperation: Statement %x, Lock for Connection %x busy\n",StatementObject,ConnectionObject);
			#endif

			return SQL_SUCCESS;
		}

		// We've got the lock, so the thread is waiting for a signal

		#ifdef __DEBUGASYNCIO
		printf("StartAsyncOperation: Statement %x, Acquired lock for Connection %x, Signalling ..\n",StatementObject,ConnectionObject);
		#endif

		status = ConnectionObject->m_async_IOthread->SignalCondition();
		if (status != 0)
		{
			#ifdef __DEBUGASYNCIO
			printf("StartAsyncOperation: Statement %x, Connection %x, Signal() failed with error %d\n",StatementObject,ConnectionObject,status);
			assert(0);
			#endif
			sprintf(buffer," Start asynchronous operation failed for statement handle %x because Signal() failed with %d for connection handle %x",
				StatementObject,status,ConnectionObject); 
			StatementObject->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

			return SQL_ERROR;
		}

		#ifdef __DEBUGASYNCIO
		printf("StartAsyncOperation: Statement %x, Releasing lock for connection %x\n",StatementObject,ConnectionObject);
		#endif

		status = ConnectionObject->m_async_IOthread->UnLockCondition();
		if(status != 0)
		{
			#ifdef __DEBUGASYNCIO
			printf("StartAsyncOperation: Statement %x, Connection %x - Unlock() failed with error %d\n",StatementObject,ConnectionObject,status);
			assert(0);
			#endif
			sprintf(buffer," Start asynchronous operation failed for statement handle %x because pthread_mutex_unlock failed with %d for connection handle %x",
				StatementObject,status,ConnectionObject); 
			StatementObject->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);
			return SQL_ERROR;
		}

		#ifdef __DEBUGASYNCIO
		printf("StartAsyncOperation complete for Statement %x, Connection %x\n",StatementObject,ConnectionObject);
		#endif

		// Allow the thread waiting on the condition a chance to grab the condition lock
		sleep(0);
		sched_yield();

		return SQL_SUCCESS;

} /* StartAsyncOperation() */

/*****************************************************************************
 *                                                                           *
 *                  AsyncSelect class members                                *
 *                                                                           *
 ****************************************************************************/

#ifdef DBSELECT

//
// Sets the completion queue length.
//
int AsyncSelect::SetQueueLen(int Len)
{
	char buffer[256];

	if(Len < 1)
	{
		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncSelect: invalid queue length specified");

		sprintf(buffer," dbSelect() invalid queue length specified for environment handle %x", envHandle); 
		envHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		return SQL_ERROR;
	}

	if(Len != this->queueLen)
	{
		if(connHandles != NULL)
			delete[] connHandles;
		if(stmtHandles != NULL)
			delete[] stmtHandles;

		connHandles = NULL;
		stmtHandles = NULL;
	}

	if(connHandles == NULL)
		connHandles = new SQLHANDLE[queueLen];

	if(stmtHandles == NULL)
		stmtHandles = new SQLHANDLE[queueLen];

	if(connHandles == NULL || stmtHandles == NULL)
	{
		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncSelect: unable to allocate memory for the statement completion queues");

		sprintf(buffer," dbSelect() failed to initialize select queue for environment handle %x", envHandle); 
		envHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		if(connHandles != NULL)
			delete[] connHandles;

		if(stmtHandles != NULL)
			delete[] stmtHandles;

		delete conditionVar;
		delete MutexLock;

		stmtHandles = NULL;
		connHandles = NULL;
		conditionVar = NULL;
		MutexLock = NULL;

		return SQL_ERROR;
	}

	this->queueLen = Len;
	return SQL_SUCCESS;

} // AsyncSelect::SetQueueLen()

AsyncSelect::AsyncSelect(CEnv *env, int Len)
{

	char buffer[256];
	initialized = false;
	enabled = false;
	waiting = false;
	queueLen = 1000;
	numIOCompleted = 0;

	envHandle = env;

	if(Len < 1)
	{
		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncSelect: invalid queue length specified");

		sprintf(buffer," dbSelect() invalid queue length specified for environment handle %x", envHandle); 
		envHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		return;
	}


	MutexLock = new Async_Mutex();
	if(MutexLock == NULL || !MutexLock->IsInitialized())
	{
		if(MutexLock != NULL)
			delete MutexLock;

		MutexLock = NULL;

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncSelect: failed to initialize mutex");

		sprintf(buffer," dbSelect() failed to initialize mutex for environment handle %x", envHandle); 
		envHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		return;
	}

	conditionVar = new Async_Condition();
	if(conditionVar == NULL || !conditionVar->IsInitialized())
	{
		if(conditionVar != NULL)
			delete conditionVar;
		conditionVar = NULL;
		delete MutexLock;
		MutexLock = NULL;

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncSelect: failed to initialize condition variable");

		sprintf(buffer," dbSelect() failed to initialize condition variables for environment handle %x", envHandle); 
		envHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		return;
	}

	connHandles = new SQLHANDLE[Len];
	stmtHandles = new SQLHANDLE[Len];

	if(connHandles == NULL || stmtHandles == NULL)
	{
		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncSelect: unable to allocate memory for the statement completion queues");

		sprintf(buffer," dbSelect() failed to initialize select queue for environment handle %x", envHandle); 
		envHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		if(connHandles != NULL)
			delete[] connHandles;

		if(stmtHandles != NULL)
			delete[] stmtHandles;
		delete conditionVar;
		delete MutexLock;

		stmtHandles = NULL;
		connHandles = NULL;
		conditionVar = NULL;
		MutexLock = NULL;

		return;
	}


	this->queueLen = Len;
	initialized = true;

} // AsyncSelect::AsyncSelect()

AsyncSelect::~AsyncSelect()
{
	if(connHandles != NULL)
		delete[] connHandles;

	if(stmtHandles != NULL)
		delete[] stmtHandles;

	if(conditionVar != NULL)
		delete conditionVar;

	if(MutexLock != NULL)
		delete MutexLock;

} // AsyncSelect::~AsyncSelect()

void AsyncSelect::AddCompletion(SQLHANDLE connectionHandle,SQLHANDLE stmtHandle)
{

	connHandles[numIOCompleted] = connectionHandle;
	stmtHandles[numIOCompleted] = stmtHandle;
	numIOCompleted++;

} // AsyncSelect::AddCompletion()

SQLRETURN  AsyncSelect::dbSelect( int ArraySize, 
								   SQLHANDLE *Connhandles,
								   SQLHANDLE *Stmthandles,	
								   const struct timespec *timeout)
{
	int status;
	int NumCompleted = 0;
	char buffer[256];


	#ifdef __DEBUGASYNCIO
	printf("dbSelect: Enter\n");
	#endif

	if(gDrvrGlobal.gdbSelect == NULL || !gDrvrGlobal.gdbSelect->IsEnabled())
	{
		#ifdef __DEBUGASYNCIO
		printf("dbSelect: dbSelect is not enabled\n");
		#endif

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncSelect: dbSelect is not enabled");

		sprintf(buffer," dbSelect() is not enabled "); 
		envHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		return SQL_ERROR;
	}

	if(ArraySize != gDrvrGlobal.gdbSelect->GetQueueLen())
	{
		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncSelect: dbSelect() array size param different from what was set");

		sprintf(buffer," dbSelect() array size param different from what was set for env %x", envHandle); 
		envHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		return SQL_ERROR;
	}

	memset(Connhandles,NULL,sizeof(Connhandles) * ArraySize);
	memset(Stmthandles,NULL,sizeof(Stmthandles) * ArraySize);

	//
	// Check if there has been some IO which has already been completed in the time it took the application to call dbSelect
	// 

	status = MutexLock->Lock();

	if(status != SQL_SUCCESS)
	{
		#ifdef __DEBUGASYNCIO
		printf("dbSelect: mutex lock failed with an error %d\n",status);
		#endif

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncSelect: dbSelect() mutex lock returned error %d", status);

		sprintf(buffer," dbSelect() mutex lock failed with an error %d", status); 
		envHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		return SQL_ERROR;
	}

	if(this->numIOCompleted > 0)
	{
		waiting = false;
		memcpy(Connhandles,this->connHandles,sizeof(Connhandles) * ArraySize);
		memcpy(Stmthandles,this->stmtHandles,sizeof(Connhandles) * ArraySize);
		NumCompleted = this->numIOCompleted;

		//
		// reset for the next dbSelect
		numIOCompleted = 0;
		memset(this->connHandles,NULL,sizeof(connHandles) * ArraySize);
		memset(this->stmtHandles,NULL,sizeof(stmtHandles) * ArraySize);

		#ifdef __DEBUGASYNCIO
		int i = 0;
		while(Connhandles[i] != NULL)
		{
			printf("dbSelect: Connection %x, Statement %x completed\n",Connhandles[i],Stmthandles[i]);
			i++;
		}
		#endif
	}
	else
		waiting = true;

	status = MutexLock->UnLock();

	if(status != SQL_SUCCESS)
	{
		#ifdef __DEBUGASYNCIO
		printf("dbSelect: mutex unlock failed with an error %d\n",status);
		#endif

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncSelect: dbSelect() mutex unlock returned error %d", status);

		sprintf(buffer," dbSelect() mutex unlock failed with an error %d", status); 
		envHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		return SQL_ERROR;
	}

	if(NumCompleted > 0)
		return NumCompleted;

	//
	// No Statement handles are ready yet, wait till we either timeout or some of them get completed
	// Wait for the condition to be signalled
	// 

    status = conditionVar->Lock();
	if(status != SQL_SUCCESS)
	{
		#ifdef __DEBUGASYNCIO
		printf("dbSelect: condition variable lock failed with an error %d\n",status);
		#endif

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncSelect: dbSelect() condition variable lock returned error %d", status);

		sprintf(buffer," dbSelect() condition variable lock failed with an error %d", status); 
		envHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		return SQL_ERROR;
	}


	#ifdef __DEBUGASYNCIO
	printf("dbSelect: waiting for condition\n");
	#endif

	if(timeout->tv_sec == 0 && timeout->tv_nsec == 0)
	{
		while(status == 0 && numIOCompleted == 0)
			status = conditionVar->Wait();
	}
	else
		status = conditionVar->Wait(timeout);

	if(conditionVar->UnLock() != SQL_SUCCESS)
	{
		#ifdef __DEBUGASYNCIO
		printf("dbSelect: condition variable unlock failed with an error %d\n",status);
		#endif

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncSelect: dbSelect() condition variable unlock returned error %d", status);

		sprintf(buffer," dbSelect() condition variable unlock failed with an error %d", status); 
		envHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		return SQL_ERROR;
	}

	if(status == ETIMEDOUT)
	{
		#ifdef __DEBUGASYNCIO
		printf("dbSelect: condition wait timed out\n");
		#endif
		return 0;
	}

	if (status != 0)
	{
		#ifdef __DEBUGASYNCIO
		printf("dbSelect: condition wait failed with an error %d at line %d\n",status,__LINE__);
		#endif

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncSelect: dbSelect() wait for condition returned error %d", status);

		sprintf(buffer," dbSelect() wait for condition returned error %d", status); 
		envHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		return SQL_ERROR;
	}

	status = MutexLock->Lock();
	if(status != SQL_SUCCESS)
	{
		#ifdef __DEBUGASYNCIO
		printf("dbSelect: mutex lock failed with an error %d\n",status);
		#endif

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncSelect: dbSelect() mutex lock returned error %d", status);

		sprintf(buffer," dbSelect() mutex lock failed with an error %d", status); 
		envHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		return SQL_ERROR;
	}

	memcpy(Connhandles,connHandles,sizeof(Connhandles) * ArraySize);
	memcpy(Stmthandles,stmtHandles,sizeof(Connhandles) * ArraySize);
	NumCompleted = numIOCompleted;

	//
	// reset for the next dbSelect
	numIOCompleted = 0;
	memset(connHandles,NULL,sizeof(connHandles) * ArraySize);
	memset(stmtHandles,NULL,sizeof(stmtHandles) * ArraySize);

	#ifdef __DEBUGASYNCIO
	int i = 0;
	while(Connhandles[i] != NULL)
	{
		printf("dbSelect: Connection %x, Statement %x completed\n",Connhandles[i],Stmthandles[i]);
		i++;
	}
	#endif

	status = MutexLock->UnLock();

	if(status != SQL_SUCCESS)
	{
		#ifdef __DEBUGASYNCIO
		printf("dbSelect: mutex unlock failed with an error %d\n",status);
		#endif

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			TraceOut(TR_ODBC_ERROR, "AsyncSelect: dbSelect() mutex unlock returned error %d", status);

		sprintf(buffer," dbSelect() mutex unlock failed with an error %d", status); 
		envHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);

		return SQL_ERROR;
	}


	return NumCompleted;

} // AsyncSelect::dbSelectA

#endif /* ifdef DBSELECT */


#endif /* ASYNCIO */
