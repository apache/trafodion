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
#ifndef EXSM_GLOBALS_H
#define EXSM_GLOBALS_H

// The ExSMGLobals class by the SQL main thread and SM reader
// thread. If a function is guaranteed to only be called by the main
// thread, for example the ExSMGlobals constructor, that function can
// use SQL classes such as NAMemory, ComDiagsArea, etc. If a function
// can possibly be called by the reader thread it should hae no
// dependencies on SQL because our SQL code is generally not
// thread-safe.

#include <sys/types.h>
#include <unistd.h>
#include "ExSMCommon.h"
#include "ExSMTaskList.h"
#include "ExSMReadyList.h"
#include "ExSMTrace.h"
 
class ExSMGlobals;
class ComDiagsArea;
class NAMemory;
class ex_tcb;
class ExExeStmtGlobals;
class SMConnection;

class ExSMGlobals
{
public:

  // Thread states used for initialization and exit handling
  //
  // The normal state progression for the main thread is:
  //   NOT_STARTED -> STARTED -> DONE
  // 
  // The normal state progression for the reader thread is:
  //   NOT_STARTED -> STARTED -> PROCESSING_SHUTDOWN -> DONE
  // 
  // Or if the reader thread encounters a NO SERVICE error from SM:
  //   NOT_STARTED -> STARTED -> TERMINATED_DUE_TO_ERROR
  // 
  enum ThreadState
  {
    // Main thread and reader thread
    NOT_STARTED = 0,
    STARTED,
    DONE,

    // Reader thread only
    PROCESSING_SHUTDOWN,
    TERMINATED_DUE_TO_ERROR
  };

  // Constants used for SM message handling
  enum MsgConstants
  {
    // How long to wait for reader thread to start - in seconds
    READER_STARTUP_WAIT = 120,

    // How long to wait for reader thread to shutdown - in seconds
    READER_SHUTDOWN_WAIT = 120
  };


  // *** THIS FUNCTION DEPENDS ON SQL CODE AND SHOULD ONLY BE CALLED
  // *** FROM THE MAIN SQL THREAD. DO NOT CALL THIS FUNCTION FROM THE
  // *** SM READER THREAD.
  ExSMGlobals(bool isMasterExecutor);

  ExSMGlobals(); // Do not implement

  virtual ~ExSMGlobals();

  static ExSMGlobals *GetExSMGlobals() { return smGlobals_; }

  static bool getSMInitialized() { return smInitialized_; }

  const char *getThreadStateString(ThreadState ts) const;

  // Static member functions to reserve SM IDs. One ID is reserved
  // in each process for executor internal communication. For example,
  // the SHUTDOWN message from the main thread to the reader thread in
  // the same process.
  static int64_t reserveSMID();
  static int64_t getExeInternalSMID() { return exeInternalSMID_; }
              
  // *** THIS FUNCTION DEPENDS ON SQL CODE AND SHOULD ONLY BE CALLED
  // *** FROM THE MAIN SQL THREAD. DO NOT CALL THIS FUNCTION FROM THE
  // *** SM READER THREAD.
  static ExSMGlobals *InitSMGlobals(ExExeStmtGlobals *stmtGlob);

  // *** THIS FUNCTION DEPENDS ON SQL CODE AND SHOULD ONLY BE CALLED
  // *** FROM THE MAIN SQL THREAD. DO NOT CALL THIS FUNCTION FROM THE
  // *** SM READER THREAD.
  ExSMTask *addTask(const sm_target_t &tgt,
                    uint32_t queueSize,
                    int32_t *scheduledAddr,
                    NAMemory *heap,
                    ex_tcb *tcb,
                    SMConnection *smConnection_);
  
  static void addDiags(const char *functionName,
                       int32_t rc,
                       ExExeStmtGlobals *stmtGlob);

  void addReaderThreadError(ExExeStmtGlobals *stmtGlob);

  void removeTask(ExSMTask *t);
  
  bool isMasterExecutor() const { return master_; }

  ExSMTaskList *getSMTaskList() { return &smTaskList_; }

  ExSMReadyList *getReadyList() { return &readyList_; }

  ThreadState getMainThreadState() { return mainThreadState_; }
  void setMainThreadState(ThreadState s) { mainThreadState_ = s; }

  ThreadState getReaderThreadState() const { return readerThreadState_; }

  // Set the reader thread state. Optionally:
  // * acquire and release the reader thread state lock
  // * signal the reader thread state condition variable
  void setReaderThreadState(ThreadState s,
                            bool doLocking = true,
                            bool doSignal = false);

  pid_t getMainThreadPID() { return mainThreadPID_; }
  int32_t getSQNodeNum() { return sqNodeNum_; }
  pthread_t getMainThreadTID() { return mainThreadThreadId_; }
  pthread_t getReaderThreadTID() { return readerThreadThreadId_; }

  pthread_mutex_t *getReaderThreadStateLock()
  { return &readerThreadStateLock_; }

  pthread_cond_t *getReaderThreadStateCond()
  { return &readerThreadStateCond_; }

  uint32_t getSendRequestCount() const { return sendRequestCount_; }
  uint32_t incrSendRequestCount() { return ++sendRequestCount_; }

  uint32_t getSendReplyCount() const { return sendReplyCount_; }
  uint32_t incrSendReplyCount() { return ++sendReplyCount_; }

  uint32_t getSendShortCount() const { return sendShortCount_; }
  uint32_t incrSendShortCount() { return ++sendShortCount_; }

  uint32_t getRecvRequestCount() const { return recvRequestCount_; }
  uint32_t incrRecvRequestCount() { return ++recvRequestCount_; }

  uint32_t getRecvReplyCount() const { return recvReplyCount_; }
  uint32_t incrRecvReplyCount() { return ++recvReplyCount_; }

  uint32_t getRecvShortCount() const { return recvShortCount_; }
  uint32_t incrRecvShortCount() { return ++recvShortCount_; }

  bool getTraceEnabled() const { return traceEnabled_; }
  void setTraceEnabled(bool b);
  void setTraceLevel(uint32_t lvl) { traceLevel_ = lvl; }
  uint32_t getTraceLevel() const { return traceLevel_; }
  const char * getTraceFilePrefix() const { return traceFilePrefix_; }
  void setTraceFilePrefix(const char *pref) { traceFilePrefix_ = pref; }
  const char *getSessionID() const { return sessionID_; }
  NAMemory *getThreadSafeHeap() { return threadSafeHeap_; }

  // Counter for SM fixup replies. See comments below with the data
  // member.
  static uint32_t getFixupReplyCount() { return fixupReplyCount_; }
  static void incrFixupReplyCount() { fixupReplyCount_++; }
  static void initFixupReplyCount() { fixupReplyCount_ = 0; }

  const Int32 getReaderThreadSmErrorNumber() { return readerThreadSmErrorNumber_; }
  void setReaderThreadSmErrorNumber(Int32 error) { readerThreadSmErrorNumber_ = error; }
  const char * getReaderThreadSmErrorFunction() { return readerThreadSmErrorFunction_; }
  void setReaderThreadSmErrorFunction(const char* function) 
                        { strcpy(readerThreadSmErrorFunction_, function); }

  void handleReaderThreadError(int32_t rc,
                               const char *function,
                               sm_handle_t dataHandle,
                               ExSMTaskList *smTaskList = NULL);

protected:

  // Create a session ID string that becomes part of the output file
  // name when tracing is enabled
  static const char *createSessionID(ExExeStmtGlobals *stmtGlob);

  static bool smInitialized_;
  static ExSMGlobals *smGlobals_;

  bool master_;
  bool traceEnabled_;
  uint32_t  traceLevel_;
  const char *traceFilePrefix_;
  const char *sessionID_;
  ExSMTaskList smTaskList_;
  ExSMReadyList readyList_;
  pid_t mainThreadPID_;
  int32_t sqNodeNum_;

  uint32_t sendRequestCount_;
  uint32_t sendReplyCount_;
  uint32_t sendShortCount_;
  uint32_t recvRequestCount_;
  uint32_t recvReplyCount_;
  uint32_t recvShortCount_;

  // A global increasing counter is used to store the next available
  // SeaMonster ID. In the master executor every new query is given a new
  // ID.
  // 
  // We reserve one ID for communication from the main thread to
  // the reader thread in the same process. The main thread calls SM
  // to initialize this ID right after a successful call to
  // SM_init. The reader thread cancels this ID after receiving a
  // SHUTDOWN message from the main thread.
  // 
  // The ID for executor internal communication will be 1
  //
  // The IDs assigned to queries will start at 2 and go up
  static int64_t nextAvailableSMID_;
  static int64_t exeInternalSMID_;

  // Main thread: thread ID and state
  pthread_t mainThreadThreadId_;
  ThreadState mainThreadState_;               

  // Reader thread: thread ID and state
  pthread_t readerThreadThreadId_;
  ThreadState readerThreadState_;
  Int32 readerThreadSmErrorNumber_;
  char readerThreadSmErrorFunction_[32];


  // A lock and condition variable for accessing reader thread state
  pthread_mutex_t readerThreadStateLock_;
  pthread_cond_t readerThreadStateCond_;

  // The thread-safe heap gives the reader thread the ability to
  // allocate IPC message buffers. Sometimes those buffers are
  // deallocated by the main thread and access to the heap will be
  // serialized by a mutex.
  NAMemory *threadSafeHeap_;

  // During download of a new query to ESPs if the query uses SM, we
  // expect ESPs to reply two ways. They reply on the Seabed control
  // connection and with a short message over SM. This global
  // variable counts the SM replies.
  //
  // Note: we are assuming that in the master only one download is in
  // progress at any given time.
  static uint32_t fixupReplyCount_;
  
}; // class ExSMGlobals

#endif // EXSM_GLOBALS_H
