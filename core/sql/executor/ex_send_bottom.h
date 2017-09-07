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
#ifndef EX_SEND_BOTTOM_H
#define EX_SEND_BOTTOM_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_send_bottom.h
 * Description:  Send bottom node (server part of a point to point
 *               connection)
 *               
 * Created:      1/1/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "Ipc.h"
#include "FragDir.h"
#include "Ex_esp_msg.h"
#include "ex_split_bottom.h"
#include "ExSMCommon.h"

////////////////////////////////////////////////////////////////////////////
// forward declarations
////////////////////////////////////////////////////////////////////////////
class ExSendBottomRouteMessageStream;
class ExSendBottomWorkMessageStream;
class ExSendBottomCancelMessageStream;
class ExEspFragInstanceDir;

////////////////////////////////////////////////////////////////////////////
// Task Definition Block for send top node
////////////////////////////////////////////////////////////////////////////
#include "ComTdbSendBottom.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ex_send_bottom_tdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ex_send_bottom_tdb
// -----------------------------------------------------------------------
class ex_send_bottom_tdb : public ComTdbSendBottom
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ex_send_bottom_tdb()
  {}

  virtual ~ex_send_bottom_tdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

  // allocate one node to communicate with a particular parent instance
  virtual ex_send_bottom_tcb * buildInstance(ExExeStmtGlobals * glob,
                                     ExEspFragInstanceDir *espInstanceDir,
                                     const ExFragKey &myKey,
                                     const ExFragKey &parentKey,
                                     ExFragInstanceHandle myHandle,
                                     Lng32 parentInstanceNum,
                                     NABoolean isLocal);

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
  //    If yes, put them in the ComTdbSendBottom instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


////////////////////////////////////////////////////////////////////////////
// Task control block for send bottom node
////////////////////////////////////////////////////////////////////////////
class ex_send_bottom_tcb : public ex_tcb
{

  friend class ExSendBottomRouteMessageStream;
  friend class ExSendBottomWorkMessageStream;

public:

  // Constructor
  ex_send_bottom_tcb(const ex_send_bottom_tdb & sendBottomTdb,
                     ExExeStmtGlobals *glob,
                     ExEspFragInstanceDir *espInstanceDir,
                     const ExFragKey &myKey,
                     const ExFragKey &parentKey,
                     ExFragInstanceHandle myHandle,
                     Lng32 parentInstanceNum);

  ~ex_send_bottom_tcb();

  inline const ex_send_bottom_tdb & sendBottomTdb() const
                                { return (const ex_send_bottom_tdb &)tdb; }
  
  void freeResources();  // free resources
  void registerSubtasks(); // queues are used in a non-standard way
  
  virtual ExWorkProcRetcode work();
  ExWorkProcRetcode cancel();
  
  ex_queue_pair getParentQueue() const;
  inline ex_queue_pair getParentQueueForSendBottom() const { return qSplit_; }
  inline Lng32 getParentInstanceNum() const   { return parentInstanceNum_; }
  inline ExFragInstanceHandle getMyHandle() const     { return myHandle_; }
  inline ExEspFragInstanceDir * getEspFragInstanceDir() const
                                                { return espInstanceDir_; }

  ExFragId getMyFragId() const { return myFragId_; }

  // Stub to processCancel() used by scheduler.
  static ExWorkProcRetcode sCancel(ex_tcb *tcb)
                   { return ((ex_send_bottom_tcb *) tcb)->cancel(); }

  // access predicates in tdb
  inline ex_expr * moveOutputValues() const
                              { return sendBottomTdb().moveOutputValues_; }

  void tickleSchedulerCancel()           { ioCancelSubtask_->schedule(); }
  void tickleScheduler()                       { ioSubtask_->schedule(); }

  virtual Int32 numChildren() const;
  virtual const ex_tcb* getChild(Int32 pos) const;

  void routeMsg(IpcMessageStream& msgStream);
    
  void setClient(IpcConnection* connection);

  IpcConnection * getClient();

  virtual ExOperStats *doAllocateStatsEntry(CollHeap *heap,
                                                       ComTdb *tdb);

  void setExtractConsumerFlag(NABoolean b) { isExtractConsumer_ = b; }
  NABoolean getExtractConsumerFlag() const { return isExtractConsumer_; }
  void incReplyMsg(Int64 msgBytes);

  IpcConnection *getConnection() { return connection_; }

private:

  // check for request messages from send top and put data in queue
  short checkRequest();

  // get a request buffer from the message stream
  TupMsgBuffer* getRequestBuffer();

  // check for reply data in the queue and send reply message to send top
  short checkReply();

  // get a reply buffer from the message stream
  void getReplyBuffer();

  // get a request buffer from the message stream
  TupMsgBuffer* getCancelRequestBuffer();

  ExWorkProcRetcode replyCancel();

  // notify ESP main of our requests
  void start();
  void finish();
  void startCancel();
  void finishCancel();

  ex_queue_pair        qSplit_;

  atp_struct           *workAtp_;

  queue_index          nextToSend_; // next down queue index to send to req.

  ExFragId             myFragId_;

  // the fragment instance handle that is assigned to this node's instance
  ExFragInstanceHandle myHandle_;

  // remember the instance number of the parent process that I'm talking to
  Lng32                 parentInstanceNum_;

  ExEspFragInstanceDir *espInstanceDir_;

  // subtasks to be executed when an I/O completes
  ExSubtask            *ioSubtask_;
  ExSubtask            *ioCancelSubtask_;

  SET(queue_index)     cancelOnSight_;

  TupMsgBuffer *currentRequestBuffer_;  // send_top request being processed
  TupMsgBuffer *currentReplyBuffer_;    // reply to send_top being built

  Lng32 requestBufferSize_; // size of receive sql buffer
  Lng32 replyBufferSize_;   // size of send sql buffer

  ULng32 currentBufferNumber_;
  NABoolean cancelReplyPending_;
  NABoolean lateCancel_;

  NABoolean isActive_; // does this node have a request from a send top?

  ExSendBottomRouteMessageStream *routeMsgStream_;
  ExSendBottomWorkMessageStream *workMsgStream_;
  ExSendBottomCancelMessageStream *cancelMsgStream_;

  NABoolean isExtractConsumer_;
  NABoolean isExtractWorkDone_;
  IpcConnection *connection_;

  // pool that will only have the defragmentation buffer and no other sql buffers
  sql_buffer_pool *defragPool_;
  tupp_descriptor * defragTd_;

  sm_target_t smTarget_;
};

////////////////////////////////////////////////////////////////////////////
// Message stream for Ipc send bottom node.  Does routing only.
////////////////////////////////////////////////////////////////////////////
class ExSendBottomRouteMessageStream : public IpcServerMsgStream
{
public:

  // constructor
  ExSendBottomRouteMessageStream(ExExeStmtGlobals *glob,
                                ex_send_bottom_tcb *sendBottomTcb);

  // callbacks, they handle administrative work when an I/O operation
  // completes
  virtual void actOnSend(IpcConnection *connection);
  virtual void actOnReceive(IpcConnection *connection);

private:

  // a pointer back to the send bottom node
  ex_send_bottom_tcb *sendBottomTcb_;
  NABoolean sendBottomActive_;
};

////////////////////////////////////////////////////////////////////////////
// Message stream for Ipc send bottom node.  Work messages are routed here.
////////////////////////////////////////////////////////////////////////////
class ExSendBottomWorkMessageStream : public IpcServerMsgStream
{
public:

  // constructor
  ExSendBottomWorkMessageStream(ExExeStmtGlobals *glob,
                                Lng32 sendBufferLimit,
                                Lng32 inUseBufferLimit,
                                IpcMessageObjSize bufferSize,
                                ex_send_bottom_tcb *sendBottomTcb);

  // callbacks, they handle administrative work when an I/O operation
  // completes
  virtual void actOnSend(IpcConnection *connection);
  virtual void actOnReceive(IpcConnection *connection);
  ex_send_bottom_tcb *getSendBottomTcb() { return sendBottomTcb_; }

private:

  // a pointer back to the send bottom node
  ex_send_bottom_tcb *sendBottomTcb_;
  NABoolean sendBottomActive_;
};
////////////////////////////////////////////////////////////////////////////
// Message stream for Ipc send bottom node's cancel messages.
////////////////////////////////////////////////////////////////////////////
class ExSendBottomCancelMessageStream : public IpcServerMsgStream
{
public:

  // construct a message stream associated with a particular send bottom node
  ExSendBottomCancelMessageStream(
                                  ExExeStmtGlobals *glob,
                                  ex_send_bottom_tcb *sendBottomTcb);
  
  // callbacks, they handle administrative work when an I/O operation
  // completes
  virtual void actOnSend(IpcConnection *connection);
  virtual void actOnReceive(IpcConnection *connection);

private:

  // a pointer back to the send bottom node
  ex_send_bottom_tcb *sendBottomTcb_;
};

#endif /* EX_SEND_BOTTOM_H */
