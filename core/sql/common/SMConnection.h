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
 *****************************************************************************
 *
 * File:         SMConnection.h
 * Description:  Connection class for SeaMonster messaging
 * Created:      September 2011
 *
 *****************************************************************************
 */
#ifndef SM_CONNECTION_H
#define SM_CONNECTION_H

#include "Ipc.h"
#include "sm.h"

// Forward declarations
class ExSMTask;
class ex_tcb;
class ExMasterStmtGlobals;

// Classes defined in this file
class SMConnection;

class SMConnection : public IpcConnection
{
public:
  SMConnection(IpcEnvironment *env,
               sm_target_t &smTarget,
               UInt32 numReceiveBuffers,
               IpcMessageObjSize maxBufSize,
               ex_tcb *tcb,
               ExMasterStmtGlobals *masterGlobals,
               NABoolean isServer = FALSE);
  
  virtual ~SMConnection();

  // Virtual interface from IpcConnection parent class
  virtual IpcConnection *castToSMConnection() { return this; }
  virtual bool isServerSide() { return (isServer_ == TRUE); }
  virtual void receive(IpcMessageStreamBase *msg);
  virtual void send(IpcMessageBuffer *msgBuf);
  virtual WaitReturnStatus wait(IpcTimeout timeout, 
                                UInt32 *eventConsumed = NULL, 
                                IpcAwaitiox *ipcAwaitiox = NULL);

  // Pure virtual functions from the IpcConnection parent class
  // * numQueuedSendMessages -- length of the send queue
  // * numQueuedReceiveMessages -- length of the task output queue
  virtual Int32 numQueuedSendMessages();
  virtual Int32 numQueuedReceiveMessages();

  // A wrapper around the parent class setState() method that also
  // writes to the ExSM trace file
  virtual void setState(IpcConnectionState s);

  // This connection class holds two stream pointers, one for data and
  // one for cancel. The user of the connection can set the pointers
  // after the connection constructor by calling these set methods.
  void setDataStream(IpcMessageStreamBase *s) { dataStream_ = s; }
  void setCancelStream(IpcMessageStreamBase *s) { cancelStream_ = s; }

  // Return the SM target structure
  const sm_target_t &getSMTarget() const { return smTarget_; }

  // Return the number of receive buffers. This value is passed to the
  // connection constructor and does not change during the life of the
  // connection.
  UInt32 getNumReceiveBuffers() const { return numReceiveBuffers_; }

  // Methods to manage a count of outstanding requests. The increment
  // and decrement methods are public to make them callable from IPC
  // infrastructure code paths such as receive callbacks.
  UInt32 getOutstandingSMRequests() const { return outstandingSMRequests_; }
  void incrOutstandingSMRequests();
  void decrOutstandingSMRequests();

  // Fill the diagnostics area with error information from this connection
  virtual void populateDiagsArea(ComDiagsArea *&d, CollHeap *diagsHeap);

  // A public method to force the connection into an error state.
  // ExMasterEspMessage::actOnErrorConnection calls this function
  // after an error is encountered on an ESP control connection and we
  // want to propagate the error to SM connections.
  void reportControlConnectionError(GuaErrorNumber err);

  // Method to stop the other end if something unexpected is received.
  void dumpAndStopOtherEnd(bool doDump, bool doStop) const;

private:

  // Allocates empty IpcMessageBuffers off of the NAHeap and 
  // inserts in the in queue of the SM task for the reader thread
  int32_t allocateReceiveBuffers();

  // The scheduled_ field is set to 1 by the SM reader thread every
  // time an arrival is seen
  Int32 scheduled_;

  // This flag is not provided by the base class because for regular
  // IPC the client and server use different classes and client versus
  // server behavior is differentiated by that, but for SM the
  // client and server use the same IpcConnection subclass and we need
  // a flag to differentiate between client and server.
  NABoolean isServer_;

  // A client-side counter of outstanding requests. The counter is
  // incremented for each data, continue, or cancel request. The
  // counter is decremented when the final reply buffer is seen for a
  // given request.
  UInt32 outstandingSMRequests_;

  // Number of pre-allocated receive buffers
  UInt32 numReceiveBuffers_;

  // used when pre allocating receive buffers that are inserted
  // into the inQueue of the SM task for the reader thread
  IpcMessageObjSize maxBufSize_;

  IpcMessageStreamBase *dataStream_;
  IpcMessageStreamBase *cancelStream_;

  // The other end of the SMConnection that receives the
  // messages from this SMConnection
  sm_target_t smTarget_;

  // SM task that contains the queue pair to send and receive
  // IpcMessageBuffer's to and from the reader thread.
  ExSMTask *smTask_;

  // The TCB pointer is only used for debugging and tracing. It is
  // never dereferenced.
  ex_tcb *tcb_;
  
  // The next two members are used for SM error handling
  // and reporting:
  // 1) The error number is the value returned by either: a) an SM API,
  //    or b) an ExSM API that calls an SM API.
  // 2) The error function is either: a) an SM API, or b) an ExSM
  //    ExSM API that calls am SM API.
  Int32 smErrorNumber_;
  char smErrorFunction_[32];

  // Each SM connection keeps a pointer to master executor statement
  // globals. In an ESP the pointer is NULL. The pointer allows the
  // connection constructor/destructor to add/remove the connection in
  // a collection of SM connections owned by statement globals.
  ExMasterStmtGlobals *masterGlobals_;

  Int64 sendCount_;
  Int64 recvCount_;
  Int64 postCount_;

  // The following members and methods are used for control flow when
  // a message either cannot be sent in a single chunk (because the
  // size exceeds SM max buffer size), or the message size exceeds
  // the receiving end's max size.
  //
  // See comments in the cpp file at the beginning of method
  // tryToSendOneChunk() for details on the protocol.
  IpcMessageBuffer *chunk_buffer_; // The IpcMessageBuffer being sent
  UInt32 chunk_nextOffset_;        // The next offset to send
  UInt32 chunk_size_;              // Chunk size for this buffer
  bool chunk_waitingForAck_;       // Is the connection waiting for an ack

  // Is the other end of this connection running on the same Seaquest
  // node
  bool intranode_;

  // Was a control connection error reported? A value of zero means no
  // control connection error has been reported. A non-zero value is
  // the Guardian error number that was reported on the control
  // connection.
  Int32 ccErrorNumber_;

  // Process arrivals. First try to send buffers if an ack recently
  // arrived.
  WaitReturnStatus workOnArrivals();

  // All data members related to chunk mode need to be kept in
  // sync. These methods perform the updates for entering chunk mode,
  // moving to the next chunk, and exiting chunk mode.
  void enterChunkMode(IpcMessageBuffer *sendBuffer, UInt32 chunkSize);
  void moveToNextChunk();
  void exitChunkMode();

  // These methods provide access to attributes related to chunking:
  // * WAITING FOR ACK -- is the connection waiting for an ack?
  // * ACK ARRIVED -- has an ack arrived but is not yet processed?
  // 
  // Note: ACK ARRIVED is stored in the task object
  bool getWaitingForAck() const { return chunk_waitingForAck_; }
  bool getAckArrived();
  void setAckArrived(bool arrived, bool waitForAnother);

  // Try to send one buffer or send a chunk of the buffer if the
  // buffer size is too large. See comments in the cpp file for more
  // detail. The method returns false if no data could be sent because
  // the connection is waiting for an ack.
  bool tryToSendOneChunk();

  void handleIOErrorForSM();

};

#endif // SM_CONNECTION_H
