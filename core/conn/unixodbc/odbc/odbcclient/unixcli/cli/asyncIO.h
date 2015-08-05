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

#ifndef __ASYNCIO_THREAD__
#define __ASYNCIO_THREAD__

#ifdef ASYNCIO
#ifdef MXOSS
#include <spthread.h>
#else
#include <pthread.h>
#endif
#include <sql.h>
#include <deque>
class CStmt;
class CConnect;
class CEnv;
// This function is the entry point for the Async IO thread (one per connection)
//
extern "C"  {
	typedef void* PTHR_FUNCTION(void*);
	void* AsyncIOThread(void *pParam);
}

/*
 * Wrapper class for pthread_mutex_t
 */
class Async_Mutex
{
public:
	Async_Mutex();
	~Async_Mutex();
	int	Lock(void);
	int TryLock(void); // Tries to lock the specified mutex, returns a EBUSY if
					   // it is already locked. Otherwise acquires the lock
	int	UnLock(void);
	inline pthread_mutex_t* GetLock() { return &mutex; }
	inline bool IsInitialized() { return initialized; }

private:
	bool initialized;
	pthread_mutex_t	 mutex;
	pthread_mutexattr_t attr;
};

/*
 * Wrapper class for pthread_cond_t
 */
class Async_Condition
{
public:

	Async_Condition();
	~Async_Condition();
	int Wait(const struct timespec *timeout = NULL);	// Wait on a Condition Variable
	int Signal(); // Signal the Condition Variable
	inline int Lock() { return condLock->Lock(); }
	inline int TryLock() { 
		// Tries to lock the mutex associated with the Condition Variable.
		// Retuns EBUSY or acquires the lock
		return condLock->TryLock(); 
	} 
	inline int UnLock() { return condLock->UnLock(); }
	inline bool IsInitialized() { return initialized; }

private:
	Async_Mutex	   *condLock;
	pthread_cond_t	cond;
	bool	initialized;
};

/*
 * Async IO thread class associated with every Connection Object
 */
class AsyncIO_Thread 
{
public: 
	AsyncIO_Thread(CConnect *connection);
	~AsyncIO_Thread();

	// Creates the running thread with entry point IOfunction
	int Create(PTHR_FUNCTION IOfunction);

	inline int	LockMutex()   { return MutexLock->Lock(); }
	inline int	UnLockMutex() { return MutexLock->UnLock(); }

	inline int	LockCondition() { return conditionVar->Lock(); }
	// Attempt to lock the Condition Variable - returns EBUSY if its already locked
	inline int	TryLockCondition() { return conditionVar->TryLock(); }
	inline int	UnLockCondition() { return conditionVar->UnLock(); }

	// Signal the Condition Variable
	inline int	SignalCondition() { return conditionVar->Signal(); }

	// Wait for the Condition Variable to be Signalled
	inline int	WaitCondition() { return conditionVar->Wait(); }

	inline bool	IsInitialized() { return initialized; }
	inline void SetInitialized(bool initialized) { this->initialized = initialized; }

	bool stmtQueueEmpty(); // returns true if we have no stmtHandles to process
	int  stmtQueuePush(CStmt* stmt); // push a new stmtHandle to the queue
	CStmt* stmtQueuePop();  // pop a stmtHandle from the queue


	inline void	setStatementHandle(CStmt *stmtHandle) { 
		// sets the statement handle that the async thread is currently processing
		this->stmtHandle = stmtHandle; 
	}

	inline CStmt*	getStatementHandle() { 
		// returns the statement handle that the async thread is currently processing
		return stmtHandle; 
	}

	int Cancel(CStmt *stmt); // sends a StopServer request if the stmt is currently executing
							 // -or- removes it from the stmtQueue if its waiting to be executed


private:
	pthread_t		thread;			// Async IO thread
	pthread_attr_t	attr;			// Async IO thread attributes
	bool			initialized;	// Async IO thread initialization complete
	CStmt			*stmtHandle;	// statement handle currently being processed by this async thread
	CConnect		*connection;	// Connection object associated with the Async thread
	Async_Mutex		*MutexLock;		// Mutex lock for updating the statement queue list
	Async_Condition *conditionVar;	// Condition Variable associated with this Async thread
	std::deque<CStmt *> stmtQueue;  // Statement handles que'ed up for execution
};

//
//Helper routine to start Async Execution on a statement handle
//
extern "C" SQLRETURN StartAsyncOperation(CConnect *ConnectionObject, CStmt *StatementObject);


#ifdef DBSELECT
//
//   This class implements the dbSelect functionality for the native driver
//   (we should not enable the define DBSELECT for the main ODBC driver)
//
class AsyncSelect
{
public:

	AsyncSelect(CEnv *env,int queueLen);
	~AsyncSelect();

	// check if initialization is complete
	inline bool	IsInitialized() { return initialized; }
	
	// set/check if dbSelect functionality is enabled
	inline bool	IsEnabled() { return enabled; }
	inline void SetEnabled(bool enabled) { this->enabled = enabled; }

	// check if client is waiting on a dbSelect
	inline bool IsWaiting() { return waiting; }

	// get/set the queue length
	int SetQueueLen(int Len);
	inline int GetQueueLen() { return queueLen; }

	// get the number of IO that have been completed in between calls to dbSelect
	inline int	GetNumIOCompleted() { return numIOCompleted; }

	// add a connection + statement handle pair to the list of completed IOs
	void AddCompletion(SQLHANDLE connectionHandle,SQLHANDLE stmtHandle);

	// Lock/Unlock the mutex to update some of the shared members
	inline int	LockMutex()   { return MutexLock->Lock(); }
	inline int	UnLockMutex() { return MutexLock->UnLock(); }

	// Lock/Unlock/Signal the condition variables
	inline int	LockCondition() { return conditionVar->Lock(); }
	inline int	UnLockCondition() { return conditionVar->UnLock(); }
	inline int	SignalCondition() { return conditionVar->Signal(); }

	SQLRETURN dbSelect( int ArraySize,
		SQLHANDLE *Connhandles,	SQLHANDLE *Stmthandles,
		const struct timespec *timeout);

private:
	Async_Mutex		*MutexLock;		// Mutex lock for updating enabled/numIOCompleted/connHandles/stmtHandles
	Async_Condition *conditionVar;	// Condition Variable to signal client waiting on a dbSelect() call
	bool			initialized;	// Async Select initialized
	bool			enabled;		// Async Select enabled
	bool			waiting;		// Is client is waiting on a dbSelect call ?
	SQLINTEGER		queueLen;		// completion queue length
	SQLHANDLE		*connHandles;   // List of completed connection handles
	SQLHANDLE		*stmtHandles;   // List of completed statement handles
	int				numIOCompleted; // Number of IO's completed in between calls to dbSelect
	CEnv			*envHandle;     // CEnv handle
};
#endif


#endif /* ASYNCIO */

#endif /* __ASYNCIO_THREAD__ */
