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
**********************************************************************/
/* -*-C++-*-
****************************************************************************
*
* File:         NAAbort.cpp (previously part of /common/BaseTypes.cpp)
* RCS:          $Id: NAAbort.cpp,v 1.1 1998/06/29 03:50:55  Exp $
* Description:
*
* Created:      5/6/98
* Modified:     $ $Date: 1998/06/29 03:50:55 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
*
*
****************************************************************************
*/

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <pthread.h>

#include "CompException.h"
#include "StmtCompilationMode.h"

extern Int32 writeStackTrace(char *s, int bufLen);

// Mutex to serialize termination via NAExit or an assertion failure
// via abort_botch_abend in the main executor thread with an assertion
// failure in the SeaMonster reader thread
//
// 1. Main executor thread initializes the mutex.
// 2. NAAssertMutexExTid is the thread id of the main executor thread which,
//    can be different than the pid, as in the case of mxosrvr.
// 3. NAExit exits or aborts while holding the mutex, and asert_botch_abend
//    aborts while holding it.
//
pthread_mutex_t NAAssertMutex;
int NAAssertMutexThreadCount = 0; // Count of executor threads
int NAAssertMutexExTid = 0; // Thread id of the main executor thread
int NAAssertMutexCreate()
{
  int rc;
  pthread_mutexattr_t attr;
  if (NAAssertMutexThreadCount++ == 0)
  {
  // The recursive attribute allows the mutex to be locked recursively
  // within a thread.  This avoids a hang if NAExit locks the mutex
  // and an exit handler subsequently asserts and calls
  // assert_botch_abend. At the present time, only the SeaMonster exit
  // handler can call assert_botch_abend, but other exit handlers
  // might be capable of calling it in the future.
    rc = pthread_mutexattr_init(&attr);
    if (rc)
    {
      if (IdentifyMyself::GetMyName() == I_AM_EMBEDDED_SQL_COMPILER)
        AssertException("", __FILE__, __LINE__).throwException();
      else
        abort();
    }
    rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (rc)
    {    
      if (IdentifyMyself::GetMyName() == I_AM_EMBEDDED_SQL_COMPILER)
        AssertException("", __FILE__, __LINE__).throwException();
      else
        abort();
    }
    rc = pthread_mutex_init(&NAAssertMutex, &attr); // Create mutex on call from executor thread
    if (rc)
    {
      if (IdentifyMyself::GetMyName() == I_AM_EMBEDDED_SQL_COMPILER)
        AssertException("", __FILE__, __LINE__).throwException();
      else
        abort();
    }
    NAAssertMutexExTid = (int) syscall(__NR_gettid);
  }
  return NAAssertMutexThreadCount;
}
int NAAssertMutexLock()
{
  int rc;
  rc = pthread_mutex_lock(&NAAssertMutex);
  if (rc)
  {
    if (IdentifyMyself::GetMyName() == I_AM_EMBEDDED_SQL_COMPILER)
      AssertException("", __FILE__, __LINE__).throwException();
    else
      abort();
  }
  return rc;
}

#include "Platform.h"

extern short getRTSSemaphore();     // Functions implemented in SqlStats.cpp
extern void releaseRTSSemaphore();

#include "BaseTypes.h" // for declaration of NAAbort()
#include "NAAssert.h"  // for declaration of NAAssert()
#include "NAError.h"   // for ARKCMP_EXCEPTION_EPILOGUE()

#include <setjmp.h>

#include <iostream>
#include "ExceptionCallBack.h"
#include "NAInternalError.h"

ExceptionCallBack * NAInternalError::pExceptionCallBack_ = NULL;

ExceptionCallBack * NAInternalError::getExceptionCallBack()
{ return pExceptionCallBack_; }

void NAInternalError::registerExceptionCallBack(ExceptionCallBack *p)
{ pExceptionCallBack_ = p; }

void NAInternalError::unRegisterExceptionCallBack()
{ pExceptionCallBack_ = NULL; }

void NAInternalError::throwFatalException(const char *msg,
					  const char *file,
					  UInt32 line)
{
  if(pExceptionCallBack_ != NULL)
  {
    char stackTrace[STACK_TRACE_SIZE];
    writeStackTrace(stackTrace, sizeof(stackTrace));
    pExceptionCallBack_->throwFatalException(msg, file, line, stackTrace);
  }
}

void NAInternalError::throwAssertException(const char *cond,
					   const char *file,
					   UInt32 line)
{
  if(pExceptionCallBack_ != NULL)
  {
    char stackTrace[STACK_TRACE_SIZE];
    writeStackTrace(stackTrace, sizeof(stackTrace));
    pExceptionCallBack_->throwAssertException(cond, file, line, stackTrace);
  }
}




#include "AbortCallBack.h"

static AbortCallBack * pAbortCallBack = 0;
void registerAbortCallBack(AbortCallBack *pACB)
{
  pAbortCallBack = pACB;
}

extern THREAD_P jmp_buf* ExportJmpBufPtr; 

#include "logmxevent.h"


static void do_endprocessing(short exitAbend)
{
  // ##I think this is a tdm_sqlexport.dll bug, wherein Buf here in the dll
  // is uninitialized (all zeroes), and the Buf we REALLY want to longjmp
  // with is out there in arkcmp.exe.  A duplicate globals or a link-export bug.
  //
  // This kludge-around will RETURN if the first half (arbitrary choice)
  // of Buf is ALL ZEROs (uninit), instead of attempting to longjmp
  // (which results in arkcmp crashing).
  //
  const char *bbb = (char *)ExportJmpBufPtr;
  Int32 i = (Int32)sizeof(jmp_buf)/2 + 1;
  for (; i--; )
    if (bbb[i]) break;
  if (i < 0) return;
  longjmp(*ExportJmpBufPtr, NAASSERT_FAILURE);
}

void NAAbort(const char * filename, Int32 lineno, const char * msg)
{
  ARKCMP_EXCEPTION_EPILOGUE("NAAbort");
  if(pAbortCallBack)
    pAbortCallBack->doCallBack(msg, filename, lineno);
  NAInternalError::throwFatalException(msg, filename, lineno);
  assert_botch_abend(filename, lineno, msg);
}


void NAAssert(const char* condition, const char * file_, Int32 line_)
{
   ARKCMP_EXCEPTION_EPILOGUE("NAAssert");
   NAInternalError::throwAssertException(condition, file_, line_);
   assert_botch_abend(file_, line_, condition);
}

void assert_botch_no_abend( const char *f, Int32 l, const char * m)
{


}

void assert_botch_abend( const char *f, Int32 l, const char * m, const char *c)
{
  NAAssertMutexLock(); // Assure "termination synchronization"
  int pid = (int) getpid();

  // get thread id for SeaMonster reader thread or other future
  // executor threads
  int tid = (int) syscall(__NR_gettid);

  Lng32 *tidPtr = tid == pid ? NULL : (Lng32*)&tid;
  if (tid == NAAssertMutexExTid)
    releaseRTSSemaphore(); // Semaphore is released on by main executor thread
  // log the message to the event log
  SQLMXLoggingArea::logSQLMXAssertionFailureEvent(f, l, m, c, tidPtr); // Any executor thread can log a failure

  // Log the message to stderr. On Linux stderr is mapped to a file
  // under $TRAF_LOG and our output will be prefixed with a
  // timestamp and process ID.
  cerr << "Executor assertion failure in file " << f
       << " on line " << l << '\n';
  cerr << "Message: " << m << endl << flush;

#ifdef _DEBUG
  // If debug code, log the message again to the terminal
  char timeString[40] = "";
  time_t tm = time(0);
  ctime_r(&tm, timeString);
  printf("*** EXECUTOR ASSERTION FAILURE\n");
  printf("*** Time: %s", timeString);
  printf("*** Process: %d\n", pid);
  if (tid != pid)
    printf("*** Thread ID: %d\n", tid);
  printf("*** File: %s\n", f);
  printf("*** Line: %d\n", (int) l);
  if (c)
    printf("*** Condition: %s\n", c);
  printf("*** Message: %s\n", m);
  fflush(stdout);
#endif // _DEBUG

  // Cleanup the shared segment since the registered atexit function
  // will not be called
  char *envvar;
  envvar = getenv("SQL_LOOP_ON_ASSERT");
  if (envvar && *envvar == '1')
  {
    while (*envvar == '1')
          sleep(10);
  }
#ifndef _DEBUG
  char *abortOnError = getenv("ABORT_ON_ERROR");
  NAString fileName(f);
  if (abortOnError != NULL)
     abort();
  else if(((int)fileName.index("/optimizer/")) > -1 )
  {
    //throw exception if ABORT is called from optimizer dir
    AssertException(m, f, l).throwException();
  }
  else
#endif
    abort();
}
