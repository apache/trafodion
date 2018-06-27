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

#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include "seabed/pctl.h"
#include "Platform.h"
#include "ExSMGlobals.h"
#include "ExSMTask.h"
#include "ExSMReader.h"
#include "ExSMEvent.h"
#include "ExSMExitHandler.h"
#include "ComDiags.h"
#include "Globals.h"
#include "ex_exe_stmt_globals.h"
#include "ex_ex.h"
#include "ExpErrorEnums.h"
#include "ComRtUtils.h"
#include "ComSqlId.h"
#include "Statement.h"

ExSMGlobals *ExSMGlobals::smGlobals_ = NULL;
bool ExSMGlobals::smInitialized_ = false;

// The SeaMonster ID for executor internal communication will be 1.
// For example, the SHUTDOWN message from main thread to reader
// thread. The IDs assigned to queries will start at 2 and go up.
int64_t ExSMGlobals::exeInternalSMID_ = 1;
int64_t ExSMGlobals::nextAvailableSMID_ = 2;

// Counter for SM fixup replies. See comments in the header file.
uint32_t ExSMGlobals::fixupReplyCount_ = 0;

// *** THIS FUNCTION DEPENDS ON SQL CODE AND SHOULD ONLY BE CALLED
// *** FROM THE MAIN SQL THREAD. DO NOT CALL THIS FUNCTION FROM THE
// *** SM READER THREAD.
void ExSMGlobals::addDiags(const char *functionName,
                           int32_t rc,
                           ExExeStmtGlobals *stmtGlob)
{
  if (stmtGlob)
  {
    ComDiagsArea *diags = stmtGlob->getDiagsArea();
    NAMemory *diagsHeap = stmtGlob->getDefaultHeap();

    if (diags == NULL)
    {
      diags = ComDiagsArea::allocate(diagsHeap);
      stmtGlob->setGlobDiagsArea(diags);
      diags->decrRefCount();
    }

    CliGlobals *cliGlob = stmtGlob->getCliGlobals();
    const char *processName = cliGlob->myProcessNameString();

    // the nsk code determines when the error EXE_SM_FUNCTION_ERROR 
    // will be retired by AQR, for now an nsk code of 10001 and 10012
    // for error EXE_SM_FUNCTION_ERROR will be retried.
    // what codes of nsk are retried are defined in SessionDefaults.cpp file
    *diags << DgSqlCode(-EXE_SM_FUNCTION_ERROR)
           << DgString0(functionName)
           << DgInt0((Lng32) rc)
           << DgInt1((Lng32) getpid())
           << DgString1(processName)
           << DgNskCode((Lng32) 10000 + abs(rc));
  }
}

// To Do: either we need to merge addDiags and this method with a
// different name or if we want to introduce new sql error number for
// reader thread errors then this would change later to report more
// errors than just ERR_NO_SERVICE This method is currently called
// only by ex_root_tdb::build to create ERR_NO_SERVICE error and adds
// the error to stmtGlob
void ExSMGlobals::addReaderThreadError(ExExeStmtGlobals *stmtGlob)
{
  if (stmtGlob)
  {
    ComDiagsArea *diags = stmtGlob->getDiagsArea();
    NAMemory *diagsHeap = stmtGlob->getDefaultHeap();

    if (diags == NULL)
    {
      diags = ComDiagsArea::allocate(diagsHeap);
      stmtGlob->setGlobDiagsArea(diags);
      diags->decrRefCount();
    }

    CliGlobals *cliGlob = stmtGlob->getCliGlobals();
    const char *processName = cliGlob->myProcessNameString();

    *diags << DgSqlCode(-EXE_SM_FUNCTION_ERROR)
           << DgString0(readerThreadSmErrorFunction_)
           << DgInt0((Lng32) readerThreadSmErrorNumber_)
           << DgInt1((Lng32) getpid())
           << DgString1(processName)
           << DgNskCode((Lng32) 10000 + abs(-1));
  }
}


//---------------------------------------------------------------------
// Construct a string used for the trace file name.
//
// The function extracts the following fields from the SQL query ID
// 
//   CPUNUM        - the node number (SQ) where the master is running
//   PIN           - the pid of the master executor
//   EXE_STARTTIME - the start time for the master executor
//
// and concatenates them into a string like this:
//
//          000_019731_212194294688945814
// 
// The return value is the <node>_<pid>_<starttime> string
//
// The string is then used when SM trace files are created to make
// sure we can identify all the traces from the same session.
//
// Currently trace file names look like this:
//
//   <smTrcPrefix>.<nodeNumOfProcess>.<stringAbove>.<processId>.log
// 
// Example:
//
//   exsm.out.000.000_019731_212194294688945814.19731.log
//
// NOTE: the string returned is allocated on the system heap and it is
// the callers responsibility to deallocate it when it is no longer needed by 
// calling global "delete [] <str>"
//
// *** THIS FUNCTION DEPENDS ON SQL CODE AND SHOULD ONLY BE CALLED
// *** FROM THE MAIN SQL THREAD. DO NOT CALL THIS FUNCTION FROM THE
// *** SM READER THREAD.
//---------------------------------------------------------------------
const char *ExSMGlobals::createSessionID(ExExeStmtGlobals *stmtGlob)
{
  ExMasterStmtGlobals *master_glob = stmtGlob->castToExMasterStmtGlobals();
  ExEspStmtGlobals *esp_glob = stmtGlob->castToExEspStmtGlobals();

  char *sqlQueryId = NULL;
  int32_t sqlQueryIdLen = 0;

  if (master_glob != NULL)
  {
    sqlQueryId = master_glob->getStatement()->getUniqueStmtId();
    sqlQueryIdLen = master_glob->getStatement()->getUniqueStmtIdLen();   
  }
  else
  {
    sqlQueryId = esp_glob->getQueryId();
    sqlQueryIdLen = esp_glob->getQueryIdLen();
  }
 
  int32_t rc = 0;
  char *smIDStr = new char[ComSqlId::MAX_QUERY_ID_LEN];
  ex_assert(smIDStr, "Failed to allocate memory for SM session Id String");
  
  // Hack for cursors. Sometimes cursors do not have a query ID.
  if (sqlQueryIdLen == 0 )
  {
    // mkstemp replaces XXXXXX in the argument string with a value
    // that makes the file unique. mkstemp also returns an open file
    // descriptor which we don't need so we close it right away.
    char buf[32] = "Cursor_XXXXXX";
    int fd = mkstemp(buf);
    if (fd != -1)
      close(fd);

    strcpy(smIDStr, buf);
    return smIDStr;
  }
  
  // Extract fields from the query ID:
  //  Node number (CPU num in nv parlor)
  //  PID of master executor 
  //  Master executor start time

  Int64 sqNodeNumber = 0;
  Int64 masterPID = 0;
  Int64 masterStart = 0;

  if (rc == 0)
    rc = ComSqlId::getSqlQueryIdAttr(ComSqlId::SQLQUERYID_CPUNUM,
                                     sqlQueryId, sqlQueryIdLen,
                                     sqNodeNumber, NULL);
  if (rc == 0)
    rc = ComSqlId::getSqlQueryIdAttr(ComSqlId::SQLQUERYID_PIN,
                                     sqlQueryId, sqlQueryIdLen,
                                     masterPID, NULL);
  if (rc == 0)
    rc = ComSqlId::getSqlQueryIdAttr(ComSqlId::SQLQUERYID_EXESTARTTIME,
                                     sqlQueryId, sqlQueryIdLen,
                                     masterStart, NULL);

  if (rc != 0)
  {
    delete [] smIDStr;
    return NULL;
  }

  str_sprintf(smIDStr, "%d_%d_%ld", (Int32) sqNodeNumber, (Int32) masterPID,
              (Int64) masterStart);

  return smIDStr;
}

// *** THIS FUNCTION DEPENDS ON SQL CODE AND SHOULD ONLY BE CALLED
// *** FROM THE MAIN SQL THREAD. DO NOT CALL THIS FUNCTION FROM THE
// *** SM READER THREAD.
ExSMGlobals *ExSMGlobals::InitSMGlobals(ExExeStmtGlobals *stmtGlob)
{
  int32_t rc = 0;
  bool isMasterExecutor =
    (stmtGlob->castToExMasterStmtGlobals() ? true : false);

  // The following call populates two output parameters and has no
  // other meaningful side effects. Output values are determined by
  // looking at current trace settings (passed in as arguments) and
  // the values of certain environment variables.

  uint32_t traceLevel = stmtGlob->getSMTraceLevel();
  const char *traceFilePrefix = stmtGlob->getSMTraceFilePrefix();
  uint32_t effectiveTraceLevel = 0;
  const char *effectiveTraceFilePrefix = NULL;
  ExSM_SetTraceInfo(traceLevel,       // IN
                    traceFilePrefix,  // IN
                    &effectiveTraceLevel,       // OUT
                    &effectiveTraceFilePrefix); // OUT

  // Certain initialization steps should only happen once in a given
  // process. Even if SM_init fails and we retry initialization later,
  // we only want these steps to happen once.
  static int firstTime = true;
  if (firstTime)
  {
    // Register an exit handler
    atexit(ExSM_ExitHandler);

    // Register the main thread with Seabed as pin 1
    proc_register_group_pin(0, EXSM_MAIN_THREAD_PIN);

    firstTime = false;
  }

  if (smGlobals_ == NULL)
  {
    // Note: everything in this block should be retryable. That is,
    // the block might execute again later if SM_init is not
    // successful or if the reader thread could not be created for
    // some reason. Make sure nothing is leaked if this block executes
    // again later.

    // Build the ExSMGlobals instance
    smGlobals_ = new ExSMGlobals(isMasterExecutor);
    ex_assert(smGlobals_, "ExSMGlobals allocation failure");

    // When we call SM_init we pass in a seaquest node number. Note
    // that on a workstation we often run seaquest with two virtual
    // nodes. A given seaquest process might be running on logical
    // node 0 or 1, even though all testing is taking place on a
    // single workstation.
    //
    // Two steps are needed to acquire the correct node number
    // * Ask for the "CPU number" from CLI globals. On seaquest the
    //   node number is the same as the CPU number on NV.
    // * To account for virtual nodes, pass that number through a
    //   mapping function, ExSM_GetNodeID(). The mapping is a no-op on
    //   a cluster but on a workstation will take virtual node
    //   settings into account.

    CliGlobals *cliGlobals = stmtGlob->getCliGlobals();
    int32_t cliNodeNum = (int32_t) cliGlobals->myCpu();
    smGlobals_->sqNodeNum_ = ExSM_GetNodeID(cliNodeNum);
    
    // Initialize data members in SM globals for tracing to a file.
    // The session ID string has to be generated before we initialize
    // tracing because the string becomes part of the trace file
    // name. See comments in ExSMGlobals::createSessionID() for more
    // detail on how trace file names are generated.
    smGlobals_->traceEnabled_ = (effectiveTraceLevel != 0);
    smGlobals_->traceLevel_ = effectiveTraceLevel;
    smGlobals_->traceFilePrefix_ = effectiveTraceFilePrefix;
    smGlobals_->sessionID_ = createSessionID(stmtGlob);
    ex_assert(smGlobals_->sessionID_, "Failed to create SM session ID");
    ExSM_SetTraceEnabled(smGlobals_->traceEnabled_, smGlobals_);

    // Create a thread-safe heap and IPC environment. If
    // initialization is not successful, the ExSMGlobals destructor
    // will deallocate these objects.
    NAHeap *smThreadSafeHeap = new (cliGlobals->exCollHeap())
      NAHeap("SeaMonster Threadsafe Heap",
             (NAHeap*)cliGlobals->exCollHeap(),
             (Lng32)32768,
             0 // upperLimit
             );
    smThreadSafeHeap->setThreadSafe();

    smGlobals_->threadSafeHeap_ = smThreadSafeHeap;

    EXSM_TRACE(EXSM_TRACE_INIT|EXSM_TRACE_SM_CALLS,
               "BEGIN %s, pid %d, tid %d",
               isMasterExecutor ? "MASTER" : "ESP",
               getpid(), ExSM_GetThreadID());
    EXSM_TRACE(EXSM_TRACE_INIT|EXSM_TRACE_SM_CALLS,
               "Seaquest node %d, SM node %d",
               (int) cliNodeNum, (int) smGlobals_->sqNodeNum_);
    
    // Initialize the in-memory trace. This function is a no-op if
    // called multiple times.
    ExSMEvent::initMainThread();

    // Call SM_init if it has not been done before
    if (!smInitialized_)
    {
      rc = ExSM_Initialize(smGlobals_, stmtGlob);
      if (rc == 0)
        smInitialized_ = true;
    }

    // If SM initialization failed, deallocate the SM globals object
    // and return NULL. The next call to this function will attemp SM
    // initialization again.
    if (!smInitialized_)
    {
      delete smGlobals_;
      smGlobals_ = NULL;
      return NULL;
    }
    
    // Create the reader thread
    EXSM_TRACE(EXSM_TRACE_MAIN_THR|EXSM_TRACE_INIT, 
               "Creating SM reader thread...");

    void *arg = smGlobals_;
    rc = pthread_create(&smGlobals_->readerThreadThreadId_, // thread struct
                        NULL,                 // default thread attributes
                        ExSM_ReaderFunction,  // thread start routine
                        arg);                 // arg to start routine
    EXSM_TRACE(EXSM_TRACE_MAIN_THR|EXSM_TRACE_INIT, 
               "pthread_create returned %d", rc);

    if (rc != 0)
    {
      ExSMGlobals::addDiags("InitSMGlobals: Error from pthread_create",
                            rc, stmtGlob);
      delete smGlobals_;
      smGlobals_ = NULL;
      return NULL;
    }

    EXSM_TRACE(EXSM_TRACE_MAIN_THR|EXSM_TRACE_INIT, "pthread_t %p",
               (void *) smGlobals_->readerThreadThreadId_);

  } // if (smGlobals_ == NULL)

  else
  {
    // SM globals were successfully created in a previous call to this
    // function. We still need to check if any trace information
    // changed. For example the name of the trace file might have
    // changed via a SET SESSION DEFAULT statement.

    if (smGlobals_->traceLevel_ != effectiveTraceLevel)
      smGlobals_->traceLevel_ = effectiveTraceLevel;

    bool newTracePrefix = false;
    if (strcmp(smGlobals_->traceFilePrefix_, effectiveTraceFilePrefix))
    {
      smGlobals_->traceFilePrefix_ = effectiveTraceFilePrefix;
      newTracePrefix = true;
    }

    bool newTraceEnabled = (effectiveTraceLevel != 0);
    if (newTraceEnabled != smGlobals_->traceEnabled_)
    {
      ExSM_SetTraceEnabled(newTraceEnabled, smGlobals_);
      smGlobals_->traceEnabled_ = newTraceEnabled;
    }

    if (newTracePrefix == true)
    {
      EXSM_TRACE(EXSM_TRACE_INIT, 
                 "BEGIN %s, pid %d, tid %d", 
                 isMasterExecutor ? "MASTER" : "ESP", 
                 getpid(), ExSM_GetThreadID());
    }

  } // if (smGlobals_ == NULL) else ...

  // Wait for reader thread to start
  EXSM_TRACE(EXSM_TRACE_MAIN_THR|EXSM_TRACE_INIT, "BEGIN wait for reader");
  
  struct timespec waitTime;
  clock_gettime(CLOCK_REALTIME, &waitTime);
  waitTime.tv_sec += ExSMGlobals::READER_STARTUP_WAIT;
  
  rc = pthread_mutex_lock(&smGlobals_->readerThreadStateLock_);
  exsm_assert_rc(rc, "pthread_mutex_lock");
  
  rc = 0;
  while (smGlobals_->readerThreadState_ == NOT_STARTED && rc == 0)
  {
    rc = pthread_cond_timedwait(&smGlobals_->readerThreadStateCond_,
                                &smGlobals_->readerThreadStateLock_,
                                &waitTime);
    EXSM_TRACE(EXSM_TRACE_MAIN_THR|EXSM_TRACE_INIT,
               "pthread_cond_timedwait returned %d", rc);

    // Possible errors from pthread_cond_timedwait:
    // * ETIMEDOUT -- reader thread did not initialize successfully in
    //   the specified time interval
    // * Other errors -- probably indicate a problem or a programming
    //   error in the calling thread.
    // 
    // In either case we do not want to proceed
    exsm_assert_rc(rc, "pthread_cond_timedwait");
  }
  
  rc = pthread_mutex_unlock(&smGlobals_->readerThreadStateLock_);
  exsm_assert_rc(rc, "pthread_mutex_unlock");
  
  EXSM_TRACE(EXSM_TRACE_MAIN_THR|EXSM_TRACE_INIT, "END wait for reader");
  
  return smGlobals_;
}

// *** THIS FUNCTION DEPENDS ON SQL CODE AND SHOULD ONLY BE CALLED
// *** FROM THE MAIN SQL THREAD. DO NOT CALL THIS FUNCTION FROM THE
// *** SM READER THREAD.
ExSMGlobals::ExSMGlobals(bool isMasterExecutor)
  : master_(isMasterExecutor),
    traceEnabled_(false),
    traceLevel_(0),
    traceFilePrefix_(NULL),
    sessionID_(NULL),
    smTaskList_(),
    readyList_(),
    mainThreadPID_(getpid()),
    sqNodeNum_(0),
    sendRequestCount_(0),
    sendReplyCount_(0),
    sendShortCount_(0),
    recvRequestCount_(0),
    recvReplyCount_(0),
    recvShortCount_(0),
    mainThreadThreadId_(pthread_self()),
    mainThreadState_(STARTED),
    readerThreadState_(NOT_STARTED),
    threadSafeHeap_(NULL)
{
  int rc = 0;

  // Initialize the lock and condition variable for reader thread
  // state changes
  rc = pthread_mutex_init(&readerThreadStateLock_, NULL);
  exsm_assert_rc(rc, "pthread_mutex_init");

  rc = pthread_cond_init(&readerThreadStateCond_, NULL);
  exsm_assert_rc(rc, "pthread_cond_init");
}

ExSMGlobals::~ExSMGlobals()
{
  if (sessionID_)
    delete [] sessionID_;

  if (threadSafeHeap_)
    delete threadSafeHeap_;
}

// Method to convert a ThreadState to a String - used for tracing
const char *ExSMGlobals::getThreadStateString(ThreadState ts) const
{
  switch (ts)
  {
    case NOT_STARTED: { return "NOT_STARTED"; }
    case STARTED: { return "STARTED"; }
    case DONE: { return "DONE"; }
    case PROCESSING_SHUTDOWN: { return "PROCESSING_SHUTDOWN"; }
    case TERMINATED_DUE_TO_ERROR: { return "TERMINATED_DUE_TO_ERROR"; }
    default: return ComRtGetUnknownString((Int32) ts);
  }
}

// *** THIS FUNCTION DEPENDS ON SQL CODE AND SHOULD ONLY BE CALLED
// *** FROM THE MAIN SQL THREAD. DO NOT CALL THIS FUNCTION FROM THE
// *** SM READER THREAD.
ExSMTask *ExSMGlobals::addTask(const sm_target_t &tgt,
                               uint32_t queueSize,
                               int32_t *scheduledAddr,
                               NAMemory *heap,
                               ex_tcb *tcb,
                               SMConnection *smConnection)
{
  ExSMTask *result = new ExSMTask(tgt, queueSize, scheduledAddr,
                                  heap, tcb, smConnection);
  ex_assert(result, "ExSMTask allocation failure");
  smTaskList_.addTask(result);

  EXSM_TRACE(EXSM_TRACE_PROTOCOL, "TASK ADD %p, tgt %d:%d:%" PRId64 ":%d:0x%c",
             result, (int) tgt.node, (int) tgt.pid, tgt.id,
             (int) ExSMTag_GetTagWithoutQualifier(tgt.tag),
             (char) ExSMTag_GetQualifierDisplay(tgt.tag));
  EXSM_TRACE(EXSM_TRACE_PROTOCOL, "TASK  queueSize %d, scheduledAddr %p",
             (int) queueSize, scheduledAddr);
  EXSM_TRACE(EXSM_TRACE_PROTOCOL, "TASK  tcb %p", tcb);

  return result;
}

void ExSMGlobals::removeTask(ExSMTask *t)
{
  EXSM_TRACE(EXSM_TRACE_PROTOCOL, "TASK REMOVE %p", t);
  smTaskList_.removeTask(t);
  delete t;
}

int64_t ExSMGlobals::reserveSMID()
{
  // The ID counter is 64 bits and we don't worry about wrapping around
  return nextAvailableSMID_++;
}

void ExSMGlobals::setTraceEnabled(bool b)
{
  if (!b)
  {
    // regress/executor/TEST123 expects this line of output when
    // tracing goes from ON to OFF
    EXSM_TRACE(EXSM_TRACE_MAIN_THR, "Trace disabled");
  }

  EXSM_TRACE_ENABLED = traceEnabled_ = b;
}

void ExSMGlobals::setReaderThreadState(ThreadState s,
                                       bool doLocking,
                                       bool doSignal)
{
  int rc = 0;

  if (doLocking)
  {
    rc = pthread_mutex_lock(&readerThreadStateLock_);
    exsm_assert_rc(rc, "pthread_mutex_lock");
  }

  EXSM_TRACE(EXSM_TRACE_RDR_THR, "Reader thread state %s -> %s",
             getThreadStateString(readerThreadState_),
             getThreadStateString(s));

  readerThreadState_ = s;

  if (doSignal)
  {
    rc = pthread_cond_signal(&readerThreadStateCond_);
    exsm_assert_rc(rc, "pthread_cond_signal");
  }

  if (doLocking)
  {
    rc = pthread_mutex_unlock(&readerThreadStateLock_);
    exsm_assert_rc(rc, "pthread_mutex_unlock");
  }
}

void ExSMGlobals::handleReaderThreadError(int32_t rc, 
                                          const char *function, 
                                          sm_handle_t dataHandle, 
                                          ExSMTaskList *smTaskList)
{
   if (smTaskList)
     smTaskList->unlock();

   readerThreadState_ = ExSMGlobals::TERMINATED_DUE_TO_ERROR;
   readerThreadSmErrorNumber_ = rc;
   strcpy(readerThreadSmErrorFunction_, function);
   if (dataHandle)
     rc = SM_get_done(0, dataHandle);
   XAWAKE(EXSM_MAIN_THREAD_PIN, LRABBIT);
   ExSMEvent::add(ExSMEvent::Exit);
}
