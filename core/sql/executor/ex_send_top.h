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
#ifndef EX_SEND_TOP_H
#define EX_SEND_TOP_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_send_top.h
 * Description:  Send top node (client part of a client-server connection)
 *               
 * Created:      12/6/95
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
#include "ExSMCommon.h"

////////////////////////////////////////////////////////////////////////////
// classes defined in this file
////////////////////////////////////////////////////////////////////////////
class ex_send_top_tdb;
class ex_send_top_tcb;
class ExSendTopMsgStream;
class ExSendTopCancelMessageStream;

////////////////////////////////////////////////////////////////////////////
// Task Definition Block for send top node
////////////////////////////////////////////////////////////////////////////
#include "ComTdbSendTop.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ex_send_top_tdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ex_send_top_tdb
// -----------------------------------------------------------------------
class ex_send_top_tdb : public ComTdbSendTop
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ex_send_top_tdb()
  {}

  virtual ~ex_send_top_tdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

  // ---------------------------------------------------------------------
  // send top nodes are usually built such that many tcbs exist for a
  // single tdb and that each tcb can use a different form of communication
  // ---------------------------------------------------------------------
  ex_tcb *  buildInstance(ExExeStmtGlobals * glob,
                          Lng32 myInstanceNum,
                          Lng32 childInstanceNum);

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
  //    If yes, put them in the ComTdbSendTop instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

const int               NumSendTopTraceElements = 8;

////////////////////////////////////////////////////////////////////////////
// Task control block for send top node
////////////////////////////////////////////////////////////////////////////

class ex_send_top_tcb : public ex_tcb
{
public:

  enum sendStep {NOT_STARTED_, STARTED_, CANCELED_AFTER_SENT_};

  // Constructor
  ex_send_top_tcb(const ex_send_top_tdb &sendTopTdb,
                  ExExeStmtGlobals *glob,
                  const IpcProcessId &sendBottomProcId,
                  Lng32 myInstanceNum,
                  Lng32 childInstanceNum);

  // Destructor
  ~ex_send_top_tcb();  

  inline const ex_send_top_tdb &sendTopTdb() const
                                  { return (const ex_send_top_tdb &)tdb; }

  void freeResources();    // free resources
  void registerSubtasks(); // add extra event for IPC I/O completion
  
  virtual Int32 fixup();

  short work();
  
  ex_queue_pair getParentQueue() const;
  inline ex_queue_pair getParentQueueForSendTop() const { return qParent_; }

  inline Lng32 getMyInstanceNum() const           { return myInstanceNum_; }
  inline Lng32 getChildInstanceNum() const     { return childInstanceNum_; }

  // access predicates in tdb
  inline ex_expr * moveInputValues() const
                                  { return sendTopTdb().moveInputValues_; }

  // this is how the callback procedures tell the ::work method
  // that something is wrong.
  inline void setIpcBroken() 
    { ipcBroken_ = TRUE; }

  inline void tickleSchedulerCancel()
    { ioCancelSubtask_->scheduleAndNoteCompletion(); }

  inline void tickleSchedulerWork(NABoolean noteCompletion = FALSE)
    {
      if (noteCompletion)
	ioSubtask_->scheduleAndNoteCompletion();
      else
	ioSubtask_->schedule();
    }

  virtual Int32 numChildren() const;

  virtual const ex_tcb* getChild(Int32 pos) const;

  virtual ExOperStats *doAllocateStatsEntry(CollHeap *heap,
                                                       ComTdb *tdb);

                  // Tell the server that we lost interest in a request
  short notifyProducerThatWeCanceled();
                  // Stub to processCancel() used by scheduler.
  static ExWorkProcRetcode sCancel(ex_tcb *tcb);
  
  void incReqMsg(Int64 msgBytes);

  ExSendTopMsgStream *getMsgStream() { return msgStream_; }
  ExSendTopCancelMessageStream *getCancelMsgStream() 
                           { return cancelMessageStream_; }

  IpcConnection *getConnection() { return connection_; }

private:

  // check for data in the down queue to send to send bottom
  short checkSend();

  // get a send buffer from the message stream
  TupMsgBuffer *getSendBuffer();

  // pre-send continue requests in anticipation of alot of reply data
  short continueRequest();

  // check for response data from send bottom and put in up queue
  short checkReceive();

  // get a receive buffer from the message stream
  TupMsgBuffer *getReceiveBuffer();

  // open a connection to the corresponding (remote) send bottom node
  short createConnectionToSendBottom(NABoolean nowaitedCompleted = FALSE);

  short createIpcGuardianConnection(NABoolean nowaitedCompleted = FALSE);

  // Process cancel requests. (called when parent cancel its request)
  ExWorkProcRetcode processCancel(); 
  ExWorkProcRetcode processCancelHelper(); 

  // check for reply to a cancel request.
  void checkCancelReply();

  // get an empty send buffer (caller becomes owner) for canceling.
  TupMsgBuffer *getCancelSendBuffer(NABoolean lateCancel);

  // For SeaMonster
  short createSMConnection();

  // Return the max number of outstanding send buffers. By default the
  // value is restricted to 1. The restriction can be lifted by
  // setting CQD GEN_SNDT_RESTRICT_SEND_BUFFERS 'OFF'.
  //
  // The restriction was originally a hard-coded value of 1 in the
  // send top cpp file accompanied by this comment: "Restrict the
  // protocol to 1 send buffer until we have a fix for out of sequence
  // messages."
  UInt32 getNumSendBuffers() const
  {
    if (sendTopTdb().getRestrictSendBuffers())
      return 1;
    return sendTopTdb().getNumSendBuffers();
  }

  UInt32 getNumRecvBuffers() const
  { return sendTopTdb().getNumRecvBuffers(); }

  // state of the send top node with respect to the connection
  enum ExSendTopState
    {
    INVALID,                // Invalid is for the trace buffer only
    NOT_OPENED,             // initial state
    WAITING_FOR_OPEN_COMPLETION, // waiting for nowaited open to complete
    WAITING_FOR_OPEN_REPLY, // waiting for open reply, ExFragInstanceHandle
    CANCELED_BEFORE_OPENED, // waiting for cancel reply, normal open
                            // hasn't happened; can only go back to NOT_OPENED
    OPEN_COMPLETE,          // open reply received, continue sending
    SERVER_SATURATED        // do not send any more data, continue req only
    } sendTopState_;

  static const char *getExSendTopStateString(ExSendTopState s);

  struct ExSendTopStateTrace
  {
    ExSendTopState stState_;
    int lineNum_;
  } stStateTrace_[NumSendTopTraceElements];
  int stTidx_;

  void setStState(ExSendTopState newState, int linenum);

  //  send top node state transition diagram
  //
  //   ---------------                              --------------------------
  //   |             |      getSendBuffer           |                        |
  //   | NOT_OPENED  |------------->>>------------->| WAITING_FOR_OPEN_REPLY |
  //   |             |                              |                        |
  //   ---------------                              --------------------------
  //          |    |                                         |
  //          |    |                                         |      
  //          |    |                                         |      
  //          v    ^                                         |
  //          v    ^                                         |      
  //          v    ^                                         |
  //          |    |                                         |
  //  notify  |    | checkCancelReply                        |
  //  Producer|    |                                         |
  //  We      |    |                                         |
  //  Canceled|    |                                         |
  //          |    |                                         |
  //  -------------------------                              |
  //  |                       |                              | getReceiveBuffer
  //  |CANCELED_BEFORE_OPENED |                              v
  //  |                       |                              v
  //  -------------------------                              v
  //                                                         |
  //                                                         |
  //                                                         |
  //   -------------------                         -------------------
  //   |                 |        Note 1           |                 |
  //   |                 |---------<<<-------------|                 |
  //   |SERVER_SATURATED |                         |  OPEN_COMPLETE  |
  //   |                 |        Note 2           |                 |
  //   |                 |--------->>>-------------|                 |
  //   -------------------                         -------------------
  //
  //   Note 1: getReceiveBuffer, dataHdr->stopSendingData == TRUE
  //
  //   Note 2: getReceiveBuffer, dataHdr->stopSendingData == FALSE
  //
  //

  NABoolean       ipcBroken_;      // callback sets it, work reads it. 

  ex_queue_pair   qParent_;

  atp_struct*     workAtp_;

  // remember my own instance number and the instance number of the
  // child ESP that I'm talking to
  Lng32            myInstanceNum_;
  Lng32            childInstanceNum_;

  queue_index nextToSendDown_; // next down queue index to send to server
  
  TupMsgBuffer* currentSendBuffer_;     // send sql buffer being built
  TupMsgBuffer* currentReceiveBuffer_;  // receive sql buffer being processed

  Lng32 sendBufferSize_;    // size of send sql buffer
  Lng32 receiveBufferSize_; // size of receive sql buffer
  
  // subtasks to be executed when an I/O completes
  ExSubtask* ioCancelSubtask_;
  ExSubtask* ioSubtask_;

  // the connection to the ESP containing the send bottom node
  IpcConnection*      connection_;

  // A message stream used to assemble and disassemble messages
  ExSendTopMsgStream* msgStream_;

  // A message stream to allow canceling regardless of state
  // of the other message stream.  
  ExSendTopCancelMessageStream *cancelMessageStream_;
  CollIndex mySendTopTcbIndex_; // for cancel processing

  // a quick way to identify the child fragment in the child ESP
  ExFragInstanceHandle  childFragHandle_;

  // process id of our communication partner
  IpcProcessId bottomProcId_;

  // to help locate canceled messages for send bottom.
  ULng32 currentBufferNumber_;               

  sm_target_t smTarget_;
};


// -----------------------------------------------------------------------
// The message stream used by the send top node to send cancel messages
// for specific ex_queue msgs to the send bottom node via an Ipc connection
// -----------------------------------------------------------------------

class ExSendTopCancelMessageStream : public IpcClientMsgStream
{
public:

  // Constructors
  // construct a message stream associated with a particular send top node
  ExSendTopCancelMessageStream(
                ExExeStmtGlobals *glob,
                Lng32 sendBufferLimit,
                Lng32 inUseBufferLimit,
                IpcMessageObjSize bufferSize,
                ex_send_top_tcb *sendTopTcb);

private:
    // Make a private copy Ctor and leave it undefined, to prevent inadvertant
    // copying.
  ExSendTopCancelMessageStream( ExSendTopCancelMessageStream &t );

private:

  // Operators: 

    // Make a private assignment operator and leave it undefined, to 
    // prevent inadvertant copying.
  ExSendTopCancelMessageStream &operator =(ExSendTopCancelMessageStream &t);

public:
  // Public methods, in alphabetical order.
  virtual void actOnReceive(IpcConnection *connection);
  virtual void actOnSend(IpcConnection *connection);
  void setLateCancel()                             { lateCancel_ = TRUE; }

private:

  // a pointer back to the send top node
  ex_send_top_tcb *sendTopTcb_;
  NABoolean lateCancel_;
};

// -----------------------------------------------------------------------
// The message stream used by the send top node to exchange data with
// the send bottom node via an Ipc connection
// -----------------------------------------------------------------------

class ExSendTopMsgStream : public IpcClientMsgStream
{
public:

  // constructor
  ExSendTopMsgStream(ExExeStmtGlobals* glob,
                     Lng32 sendBufferLimit,
                     Lng32 inUseBufferLimit,
                     IpcMessageObjSize bufferSize,
                     ex_send_top_tcb* sendTopTcb);

  // method called upon send complete
  virtual void actOnSend(IpcConnection* connection);
 
  // method called upon receive complete
  virtual void actOnReceive(IpcConnection* connection);

  ex_send_top_tcb *getSendTopTcb() { return sendTopTcb_; }

private:

  // a pointer back to the send top node
  ex_send_top_tcb* sendTopTcb_;
};


// -----------------------------------------------------------------------
// Private state: ex_send_top_private_state
// -----------------------------------------------------------------------

class ex_send_top_private_state : public ex_tcb_private_state
{
friend class ex_send_top_tcb;

public:	
  ex_send_top_private_state(const ex_send_top_tcb *tcb); //constructor
  
  ~ex_send_top_private_state();	// destructor

  ex_tcb_private_state *allocate_new(const ex_tcb *tcb);

private:  
  ex_send_top_tcb::sendStep step_; // step in processing this parent row
  ULng32 bufferNumber_;
  Int64 matchCount_;
};

#endif /* EX_SEND_TOP_H */
