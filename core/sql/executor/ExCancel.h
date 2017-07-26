// **********************************************************************
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
// File:         ExCancel.h
// Description:  Class declaration for ExCancelTdb and ExCancelTcb.
//
// Created:      Oct 15, 2009
// **********************************************************************
#ifndef EX_CANCEL_H
#define EX_CANCEL_H

#include "ComTdbCancel.h"
#include "QueueIndex.h"
#include "Ipc.h"
#include "rts_msg.h"
#include "ssmpipc.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExCancelTdb;
class ExCancelTcb;
class CancelMsgStream;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExCancelTdb -- Task Definition Block
// -----------------------------------------------------------------------
class ExCancelTdb : public ComTdbCancel
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  NA_EIDPROC ExCancelTdb()
  {}

  NA_EIDPROC virtual ~ExCancelTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  NA_EIDPROC virtual ex_tcb *build(ex_globals *globals);

private:
  // ---------------------------------------------------------------------
  // !!!!!!! IMPORTANT -- NO DATA MEMBERS ALLOWED IN EXECUTOR TDB !!!!!!!!
  // *********************************************************************
  // The Executor TDB's are only used for the sole purpose of providing a
  // way to supplement the Compiler TDB's (in comexe) with methods whose
  // implementation depends on Executor objects. This is done so as to
  // decouple the Compiler from linking in Executor objects unnecessarily.
  //
  // When a Compiler generated TDB arrives at the Executor, the same data
  // image is "cast" as an Executor TDB after unpacking. Therefore, it is
  // a requirement that a Compiler TDB has the same object layout as its
  // corresponding Executor TDB. As a result of this, all Executor TDB's
  // must have absolutely NO data members, but only member functions. So,
  // if you reach here with an intention to add data members to a TDB, ask
  // yourself two questions:
  //
  // 1. Are those data members Compiler-generated?
  //    If yes, put them in the appropriate ComTdb subclass instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//
// Task control block
//
class ExCancelTcb : public ex_tcb
{

public:
  // Constructor
  ExCancelTcb(const ExCancelTdb & cancel_tdb, ex_globals *glob);
  
  ~ExCancelTcb();  
  
  void registerSubtasks(); // add extra event for IPC I/O completion

  void freeResources();  // free resources
  
  ExWorkProcRetcode work();

  ex_queue_pair getParentQueue() const { return qparent_;}

  virtual Int32 numChildren() const { return 0; }   

  virtual const ex_tcb* getChild(Int32 /*pos*/) const { return NULL; }

  inline void tickleSchedulerWork(NABoolean noteCompletion = FALSE)
    {
      if (noteCompletion)
	ioSubtask_->scheduleAndNoteCompletion();
      else
	ioSubtask_->schedule();
    }

private:
  /////////////////////////////////////////////////////
  // Private methods.
  /////////////////////////////////////////////////////

  inline ExCancelTdb & cancelTdb() const 
      { return (ExCancelTdb &) tdb; }

  void reportError(ComDiagsArea *da, 
                   bool addCondition = false, Lng32 SQLCode = 0,
                   char *nodeName = NULL, short cpu = -1);

  /////////////////////////////////////////////////////
  // Private data.
  /////////////////////////////////////////////////////

  ex_queue_pair  qparent_;

  enum CancelStep 
  {
    NOT_STARTED, 
    SEND_MESSAGE,
    GET_REPLY, 
    DONE
  };

  CancelStep step_;
  ExSubtask *ioSubtask_;
  IpcServer * cbServer_;
  CancelMsgStream *cancelStream_;
  char nodeName_[32];
  short cpu_;
  int pid_;
  bool retryQidNotActive_;  
  Int32 retryCount_;
};

// -----------------------------------------------------------------------
// The message stream used by ExCancelTcb to let the control broker (MXSSMP)
// know that a query must be cancelled.
// -----------------------------------------------------------------------
class CancelMsgStream : public IpcMessageStream
{
public:

  // constructor
  CancelMsgStream(IpcEnvironment *env, 
                  ExCancelTcb *cancelTcb,
                  ExSsmpManager *ssmpManager)

      : IpcMessageStream(env,
                         IPC_MSG_SSMP_REQUEST,
                         CurrSsmpRequestMessageVersion,
#ifndef USE_SB_NEW_RI
                         RTS_STATS_MSG_BUF_SIZE, 
#else
                         env->getGuaMaxMsgIOSize(),
#endif
                         TRUE)
      , cancelTcb_(cancelTcb)
      , ssmpManager_(ssmpManager)
  {
  }
  
  // method called upon send complete
  virtual void actOnSend(IpcConnection *conn);
  virtual void actOnSendAllComplete();
 
  // method called upon receive complete
  virtual void actOnReceive(IpcConnection *conn);
  virtual void actOnReceiveAllComplete();

  void delinkConnection(IpcConnection *conn);

private:
  ExCancelTcb *cancelTcb_;
  ExSsmpManager *ssmpManager_;
};


#endif  // EX_CANCEL_H
