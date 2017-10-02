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

#include "SMConnection.h"
#include "ExSMGlobals.h"
#include "ExSMTask.h"
#include "ExSMReadyList.h"
#include "ExSMQueue.h"
#include "ExSMEvent.h"
#include "ExSMShortMessage.h"
#include "ex_exe_stmt_globals.h"
#include "ExpErrorEnums.h"
#include "NAAssert.h"
#include "PortProcessCalls.h"

// -----------------------------------------------------------------------
// Methods for class SMConnection
// -----------------------------------------------------------------------
SMConnection::SMConnection(IpcEnvironment *env,
                           sm_target_t &smTarget,
                           UInt32 numReceiveBuffers,
                           IpcMessageObjSize maxBufSize,
                           ex_tcb *tcb,
                           ExMasterStmtGlobals *masterGlobals,
                           NABoolean isServer)
  : IpcConnection(env,
                  IpcProcessId(),
                  isServer ?
                  eye_SEAMONSTER_CONNECTION_TO_CLIENT : 
                  eye_SEAMONSTER_CONNECTION_TO_SERVER)
  , scheduled_(0)
  , isServer_(isServer)
  , outstandingSMRequests_(0)
  , numReceiveBuffers_(numReceiveBuffers)
  , maxBufSize_(maxBufSize)
  , dataStream_(NULL)
  , cancelStream_(NULL)
  , smTarget_(smTarget)
  , smTask_(NULL)
  , tcb_(tcb)
  , smErrorNumber_(0)
  , ccErrorNumber_(0)
  , masterGlobals_(masterGlobals)
  , sendCount_(0)
  , recvCount_(0)
  , postCount_(0)
  , chunk_buffer_(NULL)
  , chunk_nextOffset_(0)
  , chunk_size_(0)
  , chunk_waitingForAck_(false)
  , intranode_(false)
{
  memset(smErrorFunction_, 0, sizeof(smErrorFunction_));

  ExSMGlobals *smGlobals = ExSMGlobals::GetExSMGlobals();
  assert(smGlobals);

  IpcEnvironment *ipcEnv = getEnvironment();
  assert(ipcEnv);

  // Create the SM target that will be stored in the task
  // 
  // * Server-side: connection and task store the same target
  //
  // * Client-side: the task stores a modified target. The only change
  //   is to target.tag. In the task's tag, we set a bit to indicate
  //   that this task receives replies (as opposed to receiving
  //   requests). This bit is needed for multi-fragment where both
  //   endpoints of a logical connection can be in the same process.
  sm_target_t targetForTask = smTarget_;
  if (!isServer_)
    ExSMTag_SetReplyFlag(&targetForTask.tag);

  // Create the task. We allocate an additional slot in the
  // receive queue for a "larger than expected" reply buffer
  uint32_t queueSize = numReceiveBuffers_ + 1;
  smTask_ = smGlobals->addTask(targetForTask,
                               queueSize,
                               &scheduled_,
                               ipcEnv->getHeap(),
                               tcb_,
                               this);
  assert(smTask_);
  
  EXSM_TRACE(EXSM_TRACE_INIT, "CONN CTOR %s %p task %p", 
             isServer_ ? "S" : "C", this, smTask_);
  EXSM_TRACE(EXSM_TRACE_INIT, "CONN CTOR %s %p max buf %d",
             isServer_ ? "S" : "C", this, (int) maxBufSize_);

  if (masterGlobals)
    masterGlobals->addSMConnection(this);

  ipcEnv->getAllConnections()->incrNumSMConnections();

  // When intranode_ is true, both ends of the connection are on the
  // same seaquest node and there is no limit on the chunk size that
  // can be sent.
  int32_t myNode = smGlobals->getSQNodeNum();
  if (myNode == smTarget_.node)
    intranode_ = true;
}

SMConnection::~SMConnection()
{
  EXSM_TRACE(EXSM_TRACE_INIT,"CONN DTOR %s %p %s", 
             isServer_ ? "S" : "C", this,
             getConnectionStateString(getState()));

  ExSMGlobals *smGlobals = ExSMGlobals::GetExSMGlobals();
  assert(smGlobals);
  
  handleIOErrorForSM();

  ExSMEvent::add(ExSMEvent::SMError, &smTarget_,
                   smErrorNumber_, ccErrorNumber_, 0, (int64_t) this);

  if (smTask_)
  {
    smGlobals->removeTask(smTask_);
    smTask_ = NULL;
  }

  if (masterGlobals_)
    masterGlobals_->removeSMConnection(this);

  IpcMessageBuffer *receiveBuf;
  while (receiveBuf = getNextReceiveQueueEntry())
    receiveBuf->callReceiveCallback(this);

  // Right now we do not have error handling in the SM connection or
  // streams for fatal error scenarios. For example the master might
  // get an IPC error on an ESP control connection and decide to run
  // TCB destructors while the SM connections are expecting
  // arrivals. For now we will simply CLOSE this connection which
  // removes it from the pendingIOs_ list. 
  setState(CLOSED);

  getEnvironment()->getAllConnections()->decrNumSMConnections();
}

Int32 SMConnection::numQueuedSendMessages()
{
  return sendQueueEntries();
}

Int32 SMConnection::numQueuedReceiveMessages()
{
  Int32 result = 0;
  if (smTask_)
  {
    ExSMQueue *q = smTask_->getOutQueue();
    if (q)
      result = (Int32) q->getLength();
  }
  return result;
}

int32_t SMConnection::allocateReceiveBuffers()
{
  ExSMQueue *inQueue = smTask_->getInQueue();

  int count = 0;
  int32_t rc = 0;

  while (inQueue->getLength() < numReceiveBuffers_)
  {
    assert(!inQueue->isFull());

    IpcEnvironment *env = getEnvironment();
    NAMemory *heap = (env ? env->getHeap() : NULL);

    IpcMessageBuffer *msgBuf =
      IpcMessageBuffer::allocate(maxBufSize_, NULL, heap, 0);
    assert(msgBuf);
    
    ExSMQueue::Entry &entry = inQueue->getTailEntry();
    entry.setData((void *) msgBuf);
    inQueue->insert();
    count++;

    sm_target_t target = smTarget_;
    if (!isServer_)
      ExSMTag_SetReplyFlag(&target.tag);

    UInt32 maxSMChunkSize = ExSM_GetMaxChunkSize(intranode_);
    UInt32 bufferSize = msgBuf->getBufferLength();

    if (bufferSize > maxSMChunkSize) 
      bufferSize = maxSMChunkSize;

    rc = ExSM_Post(target,
                   bufferSize,
                   msgBuf->data(),
                   smTask_,
                   isServer_);

    if (rc == SM_ERR_NOPEER || rc == SM_ERR_NOSERVICE || rc == SM_ERR_NODE_DOWN)
      return rc;

    exsm_assert_rc(rc, "ExSM_Post");
    postCount_++;
  }

  EXSM_TRACE(EXSM_TRACE_BUFFER,"CONN %p %s alloc recv bufs %d", this,
             isServer_ ? "server" : "client", count);
  return rc;
}

void SMConnection::send(IpcMessageBuffer *msgBuf)
{
  assert(msgBuf);

  EXSM_TRACE(EXSM_TRACE_BUFFER, "CONN %p BEGIN SEND %s state %s", this,
             (isServer_ ? "S" : "C"), getConnectionStateString(getState()));
  EXSM_TRACE(EXSM_TRACE_BUFFER, "CONN %p buf %p len %d ref %d", this,
             msgBuf, (int) msgBuf->getMessageLength(),
             (int) msgBuf->getRefCount());

  if (!isServer_ && getState() == ERROR_STATE)
  {
    // The connection is in an error state so we do not attempt the
    // send. We issue a send callback followed by a receive callback
    // because data streams require both callbacks, even when errors
    // are encountered, to perform their bookkeeping.

    // This call will put the connection on pendingIOs_ even though no
    // I/O is being attempted. The connection will be removed from
    // pendingIOs_ when the receive callback is processed.
    incrOutstandingSMRequests();

    // Perform the send callback
    msgBuf->callSendCallback(this);

    // Perform the recieve callback and release the reference to the
    // message buffer
    msgBuf->callReceiveCallback(this);
    msgBuf->decrRefCount();
  } 
  else
  {
    queueSendMessage(msgBuf);
    while (tryToSendOneChunk())
      ;
  }

  EXSM_TRACE(EXSM_TRACE_BUFFER, "CONN %p END SEND state %s", this,
             getConnectionStateString(getState()));
}

// Try to send one buffer or one chunk if the buffer size is too
// large. Return true if a buffer or chunk is sent.
// 
// Nothing is sent and false is returned if the connection is waiting
// for an ack.
//
// Buffers are too large if either of the following is true:
// * Buffer size exceeds the expected size for this connection
// * Buffer size exceeds the maximum SM chunk size
//
// When we send chunks the protocol is:
//
//   * First send a short message containing the buffer size and
//     the chunk size, and return false
//   * For each chunk
//       * The function will eventually be called again when an ack
//         arrives.
//       * Send a chunk and return true
// 
//   Any time the function is called while waiting for an ack, do
//   nothing and return false.
//
// The function will return true if a send or prepost attempt returns
// the NOPEER error.
// 
bool SMConnection::tryToSendOneChunk()
{
  EXSM_TRACE(EXSM_TRACE_BUFFER,
             "CONN %p TRY wait %d arr %d q_sz %d",
             this, (int) getWaitingForAck(), (int) getAckArrived(),
             (int) sendQueueEntries());

  int32_t rc = 0;

  // These local variables will be used for the SM send
  IpcMessageBuffer *sendBuffer = NULL;
  UInt32 sendOffset = 0;
  UInt32 sendBytes = 0;

  // Keep track of whether we are sending a message in chunks, and
  // whether we have reached the final chunk
  bool sqlChunkMode = false;
  bool finalSQLChunk = false;

  // We want to send different things depending on whether we are in
  // chunk mode. In chunk mode, we want to send the next
  // chunk. Otherwise we want to send the next buffer from the send
  // queue.
  //
  // In chunk mode the WAITING FOR ACK property of the connection will
  // be TRUE. It gets set to TRUE by the enterChunkMode() method and
  // stays TRUE until chunk transmissions are complete.

  if (getWaitingForAck())
  {
    // Note the fact that we are in chunk mode
    sqlChunkMode = true;

    // See if the ACK ARRIVED property is set. The property is stored
    // in the task object. The property is set by the reader thread
    // calling task->sendChunk_SetAckArrived(true). The property is
    // cleared below, inside the getAckArrived() block.
    if (getAckArrived())
    {
      // The connection was waiting for an ack and the ack
      // arrived. The next send can take place.

      // Adjust local variables to drive the send. Since we are
      // already in chunk mode, the data to send comes from the buffer
      // chunk_buffer_ at offset chunk_nextOffset_.
      // 
      // The number of bytes to send will be chunk_size_ unless this
      // is the last chunk and there are not that many bytes remaining
      // in chunk_buffer_.

      exsm_assert(chunk_buffer_, "ACK arrived but no buffer to send");
      UInt32 msgLen = (UInt32) chunk_buffer_->getMessageLength();
      sendBuffer = chunk_buffer_;
      sendOffset = chunk_nextOffset_;
      if (chunk_nextOffset_ + chunk_size_ < msgLen)
      {
        sendBytes = chunk_size_;
      }
      else
      {
        exsm_assert(chunk_nextOffset_ < msgLen,
                    "Chunk offset should not exceed message length");

        sendBytes = (msgLen - chunk_nextOffset_);
        finalSQLChunk = true;
      }

      // Adjust data members in response to the ack arriving
      // 
      // * Clear the ACK ARRIVED flag
      // 
      // * For a non-final chunk, set the WAITING FOR ACK flag because
      //   this connection will require an ack before sending the next
      //   chunk
      // 
      // * For a final chunk, clear the WAITING FOR ACK flag
      // 
      setAckArrived(false, !finalSQLChunk);

      EXSM_TRACE(EXSM_TRACE_BUFFER,
                 "CONN %p ACK ARRIVED buf %p offset %d bytes %d final %d",
                 this, sendBuffer, (int) sendBytes, (int) sendOffset,
                 (int) finalSQLChunk);

    } // ack arrived

    else
    {
      // No ack has arrived so there is nothing to do. The sendBuffer
      // variable is NULL which will be seen by code below and trigger
      // an early return.
    }

  } // waiting for an ack

  else
  {
    // Remove a buffer from the send queue
    sendBuffer = getNextSendQueueEntry();

    if (sendBuffer)
    {
      // maxSMChunkSize will be the largest physical chunk we can
      // transmit over SM. For intranode messages there is no limit so
      // maxSMChunkSize will be the total size of sendBuffer.
      UInt32 msgLen = (UInt32) sendBuffer->getMessageLength();
      UInt32 maxSMChunkSize = ExSM_GetMaxChunkSize(intranode_);
      
      EXSM_TRACE(EXSM_TRACE_BUFFER,
                 "CONN %p dequeue %p len %d",
                 this, sendBuffer, (int) msgLen);

      // Cases to consider after a buffer is removed from the send
      // queue:
      // 
      // (a) The buffer exceeds SM maximum chunk size. It will be
      //     sent in multiple chunks.
      // 
      // (b) The buffer does not exceed SM maximum chunk size but is
      //     larger than the receiving connection's max buffer size. The
      //     buffer can be sent in one chunk but first requires a short
      //     message round-trip interaction so that the receiving end can
      //     allocate a receive buffer of sufficient size.
      // 
      // (c) The buffer does not exceed either limit and can be sent
      //     right away

      if (msgLen <= maxBufSize_ && msgLen <= maxSMChunkSize)
      {
        // Case (c)

        // The buffer can be sent right away. There is nothing to do
        // here except adjust local variables that will drive the
        // send. The send takes place later in this method.
        sendOffset = 0;
        sendBytes = msgLen;
      }
      else
      {
        // Cases (a) and (b) are treated the same

        // The buffer exceeds either the SM maximum size or the
        // receiving connection's maximum size. A short message will
        // be sent containing the buffer size and the chunk size. Data
        // from the buffer will not be sent until an ack arrives.

        // Adjust data members to ENTER chunk mode
        UInt32 sqlChunkSize =
          (msgLen > maxSMChunkSize ? maxSMChunkSize : msgLen);
        enterChunkMode(sendBuffer, sqlChunkSize);

        // Setting sendBuffer to NULL will trigger an early return
        // after the short message is sent
        sendBuffer = NULL;
        
        // We are about to send a short message containing the buffer
        // size and the chosen chunk size. The tag needs the REPLY bit
        // set. The message data region contains:
        // * Message type
        // * Buffer size
        // * SQL chunk size

        sm_target_t targetToSend = smTarget_;
        if (isServer_)
          ExSMTag_SetReplyFlag(&targetToSend.tag);

        ExSMShortMessage m;
        m.setTarget(targetToSend);
        m.setNumValues(3);
        m.setValue(0, (int32_t) ExSMShortMessage::SIZE);
        m.setValue(1, (int32_t) msgLen);
        m.setValue(2, (int32_t) sqlChunkSize);
        m.send();
        if (rc == SM_ERR_NOPEER || rc == SM_ERR_NOSERVICE || rc == SM_ERR_NODE_DOWN)
        {
          if (!isServer_)
          {
            setErrorInfo(-1);
            setState(ERROR_STATE);
            smErrorNumber_ = rc;
            strcpy(smErrorFunction_, "ExSM_SendShortMessage");
          }
          else
            NAExit(0);
        }
        exsm_assert_rc(rc, "ExSM_SendShortMessage");
      } // buffer is too large
    } // if (sendBuffer)
  } // not waiting for an ack

  // There is nothing to do if the connection is waiting on an ack or
  // if there was nothing in the send queue. In either case the
  // sendBuffer variable is NULL.
  if (sendBuffer == NULL)
    return false;

  // On the client side this adds the SM connection to
  // pendingIOs_. On the server side the SM connection is added to
  // pendingIOs_ as soon as the connection is created.
  if (!isServer_)
    incrOutstandingSMRequests();

  // Before sending an SM message we must have pre-allocated receive
  // buffers for all possible replies. Pre-allocated buffers allow the
  // reader thread to fill target buffers without the overhead of
  // allocating space on the fly.
  rc = allocateReceiveBuffers();
  if (rc == SM_ERR_NOPEER || rc == SM_ERR_NOSERVICE || rc == SM_ERR_NODE_DOWN)
  {
    if  (!isServer_)
    {
      smErrorNumber_ = rc;
      strcpy(smErrorFunction_, "ExSM_Post");
      setErrorInfo(-1);
      setState(ERROR_STATE);

      // Issue the send callback with the connection in the ERROR
      // state. Note that smErrorNumber_ and smErrorFunction_ were
      // set by allocateReceiveBuffers().
      sendBuffer->callSendCallback(this);

      sendBuffer->decrRefCount();
      return true;
    }
    else
      NAExit(0);
  }

  // The next step is to send a message

  ExSM_MessageType msgType =
    (isServer_ ? EXSM_MSG_REPLY : EXSM_MSG_REQUEST);
  ExSMGlobals *smGlobals = ExSMGlobals::GetExSMGlobals();

  sm_target_t targetToSend = smTarget_;
  if (sqlChunkMode)
    ExSMTag_SetSQLChunkFlag(&targetToSend.tag);
  if (isServer_)
    ExSMTag_SetReplyFlag(&targetToSend.tag);

  bool messageWasSent = false;
  rc = ExSM_Send(smGlobals,
                 targetToSend,
                 sendBuffer->data(sendOffset),
                 sendBytes,
                 msgType,
                 true,             // isPrepostRequired
                 messageWasSent,   // OUT
                 -1,               // retry forever
                 sendCount_,
                 sendBuffer->data(0));

  if (rc == SM_ERR_NOPEER || rc == SM_ERR_NOSERVICE || rc == SM_ERR_NODE_DOWN)
  {
    if (!isServer_)
    {
      setErrorInfo(-1);
      setState(ERROR_STATE);
      smErrorNumber_ = rc;
      strcpy(smErrorFunction_, "ExSM_Send");
    }
    else
      NAExit(0);
  }
  else if (rc == SM_ERR_QUEUED)
  {
    // Add a reference to the send buffer because SM is holding a
    // pointer to the buffer. This new reference will be released
    // after SM returns the buffer to the reader thread. The reader
    // will place the buffer on a list that the main thread cleans up
    // periodically.
    sendBuffer->incrRefCount();
  }
  else
  {
    assert(messageWasSent && rc == 0);
  }

  sendCount_++;
  EXSM_TRACE(EXSM_TRACE_PROTOCOL,"CONN %p send count %lld", this,
             sendCount_);

  if (sqlChunkMode)
  {
    // Do not attempt to send more chunks if an error was encountered
    if (getState() == ERROR_STATE)
      finalSQLChunk = true;

    if (finalSQLChunk)
    {
      // Adjust data members to EXIT chunk mode
      exitChunkMode();
    }
    else
    {
      // A chunk was sent and it was not the final chunk. Adjust data
      // members to move to the NEXT chunk.
      moveToNextChunk();
    }
  }
  
  // Invoke the send callback if this was not a chunked message or if
  // it was the final chunk
  if (!sqlChunkMode || finalSQLChunk)
  {
    // The lastSentBuffer_ data member is used inside some callback
    // methods, for example to record the size of the message that was
    // just sent in runtime stats.
    lastSentBuffer_ = sendBuffer;
    sendBuffer->callSendCallback(this);
    lastSentBuffer_ = NULL;
    
    // Release the message buffer. The buffer was passed in from a
    // stream but the stream no longer holds a reference. If the
    // buffer was not queued by SM, this connection holds the only
    // reference. If the buffer was queued, an additional reference
    // was added above that the buffer will be released later.
    sendBuffer->decrRefCount();
  }

  return true;
}

// SMConnection::receive()
//
// SM connections are only used by buffered streams (subclasses of
// IpcBufferedMsgStream). Buffered streams call connection.receive()
// in the following cases:
//
// Client
// * IpcClientMsgStream::internalActOnSend() calls
//   connection->receive() BEFORE issuing the send callback on the
//   stream
//
// Server
// * IpcServerMsgStream::internalActOnReceive() calls
//   connection->receive() AFTER issuing the receive callback on the
//   stream
// * During the send bottom TCB constructor, connection.receive() is
//   called once to put the connection into an initial receiving state
//
void SMConnection::receive(IpcMessageStreamBase *msgStream)
{
   // if we are on server side adds SM connection to pendingIOs_
   // on the client side SM connection is added to pendingIOs when
   // the first message is sent and the connection state is set to SENDING
  if (isServer_)
    setState(RECEIVING);

  // The call to addReceiveCallback(stream) will add the stream to the
  // connection's recvStreams_ list. It does nothing else.
  // recvStreams_ is used as buffers arrive to match buffers to the
  // correct stream. The matchmaking happens in
  // IpcConnection::getNextReceiveQueueEntry().
  addReceiveCallback(msgStream);

  // Make sure the maximum number of receive buffers are now
  // pre-allocated.  This is needed only for server-side connections
  // because the server-side has to be ready to receive messages
  // before doing any sends. In contract, a client-side connection can
  // allocate receive buffers when a new request is sent to the
  // server.
  if (isServer_)
  {
    EXSM_TRACE(EXSM_TRACE_PROTOCOL,
               "CONN %p receive, allocating buffers", this);
    int32_t rc = allocateReceiveBuffers();
    if (rc == SM_ERR_NOPEER || rc == SM_ERR_NOSERVICE || rc == SM_ERR_NODE_DOWN)
       NAExit(0);
  }
}

WaitReturnStatus SMConnection::wait(IpcTimeout timeout, 
                                    UInt32 *eventConsumed, 
                                    IpcAwaitiox *ipcAwaitiox)
{
  WaitReturnStatus waitReturnStatus;

  // Temporary code till Gil's changes are merged to send a message to master 
  // through control connection
  ExSMGlobals *smGlobals = ExSMGlobals::GetExSMGlobals();
  if (smGlobals->getReaderThreadState() == ExSMGlobals::TERMINATED_DUE_TO_ERROR)
  {
    if (!isServer_)
    {
      if (getState() != ERROR_STATE)
      {
        EXSM_TRACE(EXSM_TRACE_PROTOCOL,"CONN %p Reader thread terminated",
                   this);

        smErrorNumber_ = smGlobals->getReaderThreadSmErrorNumber();
        strcpy(smErrorFunction_, smGlobals->getReaderThreadSmErrorFunction());
        setErrorInfo(-1);
        setState(ERROR_STATE);
      }
    }
    else
    {
      EXSM_TRACE(EXSM_TRACE_PROTOCOL,
                 "CONN %p Reader thread terminated, exiting", this);
      NAExit(0);
    }
  }

  // $$$$ SEAMONSTER PROJECT JUNE 2013
  // We should be able to remove the scheduled_ data member now that
  // we have a ready list for tasks with arrivals, and only call
  // connection.wait() for a task that is on the ready list.
  if (scheduled_)
    waitReturnStatus = workOnArrivals();
  else
    waitReturnStatus = WAIT_OK;
  return waitReturnStatus;
}

WaitReturnStatus SMConnection::workOnArrivals()
{
  EXSM_TRACE(EXSM_TRACE_PROTOCOL,"CONN %p BEGIN WORKONARRIVALS", this);
  
  while (getWaitingForAck() && getAckArrived())
    while (tryToSendOneChunk())
      ;
  
  scheduled_ = 0;
  ExSMQueue *outQueue = smTask_->getOutQueue();
  
  // Each iteration of the while loop will do the following
  // * Remove an incoming buffer from the SM task queue
  // * On the client side, give the buffer a message stream pointer
  //   (data or cancel)
  // * Add the buffer to the connection's receive queue
  // * On the server side, set the connection state to ESTABLISHED
  //
  // Following the while loop, a message stream receive callback is
  // issued for each buffer on the connection's receive queue
  
  ExSMTaskList *smTaskList = ExSMGlobals::GetExSMGlobals()->getSMTaskList();
  ExSMReadyList *readyList = ExSMGlobals::GetExSMGlobals()->getReadyList();

  while (!outQueue->isEmpty())
  {
    ExSMQueue::Entry &entry = outQueue->getHeadEntry();
    IpcMessageBuffer *msgBuf = (IpcMessageBuffer *) entry.getData();
    bool locked = false;

    // We are about to remove a buffer from the task output queue
    // 
    // If the task output queue is about to become empty (current
    // length is 1 and we are about to remove an entry), perform the
    // following steps:
    // 
    // (a) Before removing the buffer, acquire the task list lock.
    //     This prevents the reader thread from putting the task back
    //     on the completed task queue.
    // 
    // (b) After removing the buffer, if the output queue is still
    //     empty, remove the task from the completed task queue and
    //     release the task list lock.

    // Acquire the task list lock
    if (outQueue->getLength() == 1)
    {
      smTaskList->lock();
      locked = true;
    }

    // Remove a buffer from the task output queue
    outQueue->removeHead();
    recvCount_++;

    // Remove the SM task from the ready list and release the lock. A
    // task is added to the ready list by the reader thread when a
    // buffer arrives for a task and that task currently has an empty
    // output queue.
    // 
    // NOTE: The SM ready list is accessed by both threads (main and
    // reader) but does not have its own lock. By convention,
    // modifications to the ready list are always performed while
    // holding a lock on the SM task list (a global collection of all
    // SM tasks in this process).
    if (outQueue->isEmpty())
    {
      assert(locked);
      readyList->remove(smTask_);
    }

    if (locked)
      smTaskList->unlock();

    // The next few lines look into the message and extract the type
    // of the sending message stream. The stream type is used for
    // tracing and also used later to differentiate data versus cancel
    // messages.
    InternalMsgHdrInfoStruct *infoStruct = (InternalMsgHdrInfoStruct *)
      msgBuf->data(0);
    ESPMessageTypeEnum streamType =
      (ESPMessageTypeEnum) infoStruct->getType();
    
    EXSM_TRACE(EXSM_TRACE_PROTOCOL,
               "CONN %p recv %p len %d tag %d %s", this, msgBuf,
               (int) msgBuf->getMessageLength(),
               (int) msgBuf->getReplyTag(),
               getESPStreamTypeString(streamType));
    if (!isServer_)
      EXSM_TRACE(EXSM_TRACE_PROTOCOL,
                 "CONN %p lbatch %d seq curr %d incoming %d", this,
                 (int) infoStruct->getSMLastInBatch(),
                 (int) getReplySeqNum(), (int) infoStruct->getSeqNum());

    // After a control connection error was reported we discard any
    // late-arriving data buffers. This is because the method
    // reportControlConnectionError() already issued receive callbacks
    // for all pending arrivals and we do not want callbacks issued
    // twice for the same arrival.
    if (ccErrorNumber_ != 0)
    {
      msgBuf->decrRefCount();

      // Return to the top of the "WHILE OUTQ IS NOT EMPTY" loop
      continue;
    }
    
    if (!isServer_)
    {
      // For client-side only. Add a pointer to the appropriate stream
      // to the message buffer object.
      //
      // Note: we do not call decrOutstandingSMRequests() for a DATA
      // reply as we do for a CANCEL reply. This is because data
      // streams use a batch protocol and we do not want the decrement
      // for every buffer. Only the last buffer in a batch should
      // trigger the decrement. The stream receive callback (issued
      // later in this method) takes care of decrementing the counter
      // once a complete batch is seen. This happens inside
      // IpcClientMsgStream::internalActOnReceive().
      switch (streamType)
      {
        case IPC_MSG_SQLESP_DATA_REPLY:
        {
          msgBuf->addCallback(dataStream_);
          break;
        }
        case IPC_MSG_SQLESP_CANCEL_REPLY:
        {
          msgBuf->addCallback(cancelStream_);
          decrOutstandingSMRequests();
          break;
        }
        default:
          assert(0);

      } // switch (streamType)
    } // if (!isServer_)
    
    // To allow this buffer to pass through an IpcClientMsgStream or
    // IpcServerMsgStream it needs a valid reply tag. The tag is only
    // checked for being valid, the actual value currently has no
    // meaning for SM.
    short replyTag = 1;
    msgBuf->setReplyTag(replyTag);
    EXSM_TRACE(EXSM_TRACE_PROTOCOL,"CONN %p set reply tag %d", 
               this, (int) replyTag);      
    
    queueReceiveMessage(msgBuf);
    
    // right for server, as the server sets it to SENDING
    // again before receive call is done.
    // But for client the the conn should not removed 
    // from the pendingIOs_ after the first receive but
    // should be set to ESTABLISHED after all the expected
    // number of replies are recieved or EOD is received.
    if (isServer_)
      setState(ESTABLISHED);

  } // while out queue is not empty
  
  // NOTE: getNextReceiveQueueEntry() has side effects and is not
  // guaranteed to return a buffer just because the queue is
  // non-empty. To fully understand the de-queueing step, you should
  // read getNextReceiveQueueEntry(). The logic there includes:
  // * special cases for server-side and the ERROR state
  // * matching of reply sequence numbers
  // * if the incoming buffer has a stream pointer, matching of that
  //   pointer with an element of recvStreams_
  // * removal from receiveQueue_ and recvStreams_
  // * updating the buffer's stream pointer
  // * increment of replySeqNum_
  
  IpcMessageBuffer *receiveBuf;
  while ((receiveBuf = getNextReceiveQueueEntry()))
    receiveBuf->callReceiveCallback(this);
  
  EXSM_TRACE(EXSM_TRACE_PROTOCOL,"CONN %p END WORKONARRIVALS", this);
  return WAIT_OK;
}

void SMConnection::setState(IpcConnectionState s)
{
  IpcConnectionState oldState = getState();
  if (oldState != s)
    EXSM_TRACE(EXSM_TRACE_PROTOCOL,"CONN %p %s -> %s", this,
               getConnectionStateString(oldState),
               getConnectionStateString(s));
  IpcConnection::setState(s);
}

void SMConnection::incrOutstandingSMRequests()
{
  outstandingSMRequests_++;
  EXSM_TRACE(EXSM_TRACE_PROTOCOL,"CONN %p outstanding reqs %d", this,
             (int) outstandingSMRequests_);
  if (outstandingSMRequests_ == 1)
    setState(SENDING);
}

void SMConnection::decrOutstandingSMRequests()
{
  assert(outstandingSMRequests_ > 0);
  outstandingSMRequests_--;
  EXSM_TRACE(EXSM_TRACE_PROTOCOL,"CONN %p outstanding reqs %d", this,
             (int) outstandingSMRequests_);

  if (outstandingSMRequests_ == 0)
    setState(ESTABLISHED);
}

// SMConnection::reportControlConnectionError()
//
// ExMasterEspMessage::actOnErrorConnection calls this function after
// an error is encountered on an ESP control connection and we want to
// propagate the error to SM connections.
//
// The method is only called in the master executor and is therefore a
// no-op for server-side connections. The method is also a no-op if
// the connection already encountered errors.
void SMConnection::reportControlConnectionError(GuaErrorNumber error)
{
  if (isServer_)
    return;

  // When this function is called for the first time, store the error
  // number in a field specific to control connection errors
  if (ccErrorNumber_ == 0)
  {
    ccErrorNumber_ = error;
  
    ExSMEvent::add(ExSMEvent::ControlConnectionError, &smTarget_,
                   ccErrorNumber_, smErrorNumber_, 0, (int64_t) this);
    EXSM_TRACE(EXSM_TRACE_PROTOCOL,"CONN %p reportControlConnectionError %d",
               this, (int) error);

  }

  // Also store the error in the general-purpose field smErrorNumber_
  if (smErrorNumber_ == 0)
    smErrorNumber_ = error;
  
  setErrorInfo(-1);
  setState(ERROR_STATE);

  handleIOErrorForSM();

  // The code above that processes pending responses may have set the
  // state of the connection to ESTABLISHED as a way to remove the
  // connection from the global pendingIOs_ list. It would nice to
  // avoid the transition to ESTABLISHED but for now that is not being
  // attempted. Instead we simply make sure that the connection is in
  // an ERROR state by the time this method returns.
  setState(ERROR_STATE);
}

void SMConnection::handleIOErrorForSM()
{
  
  IpcMessageBuffer *sendBuf;
  // might need to decrement the ref count of the chunk mode buffer, 
  // otherwise it might be a memory leak
  while (sendBuf = removeNextSendBuffer())
    sendBuf->decrRefCount();


  IpcMessageBuffer *receiveBuf;
  while (receiveBuf = removeNextReceiveBuffer())
    getEnvironment()->getAllConnections()->bumpCompletionCount();

  removeReceiveStreams();

  if (dataStream_)
  {
    IpcClientMsgStream *dataStream = (IpcClientMsgStream *)dataStream_;

    for (Lng32 i=0; i < dataStream->numOfResponsesPending(); i++)
    {
      decrOutstandingSMRequests();
      dataStream->setSMBatchIsComplete(TRUE);
      dataStream->internalActOnReceive(NULL, this);
    }
  }

  if (cancelStream_)
  {
    IpcClientMsgStream *cancelStream = (IpcClientMsgStream *)cancelStream_;

    for (Lng32 i=0; i < cancelStream->numOfResponsesPending(); i++)
    {
      decrOutstandingSMRequests();
      cancelStream->internalActOnReceive(NULL, this);
    }
  }

}

// SMConnection::populateDiagsArea()
//
// This method is a pure virtual function in the IpcConnection parent
// class.
//
// The method gets called by stream objects when a send or receive
// callback is issued and the connection has entered an ERROR
// state. The stream caller provides a diags area in which error
// conditions are written.
//
// The method is a no-op if the connection is not in an ERROR state.
//
void SMConnection::populateDiagsArea(ComDiagsArea *&diags,
                                     CollHeap *diagsHeap)
{
  if (smErrorNumber_ == GuaOK)
    return;
 
  // Cases to consider
  //
  // a. An SM NOPEER error was encountered. This means the process at
  // the other end of an SM connection went away unexpectedly.
  // Generate a descriptive error.
  //
  // b. Otherwise all other errors are assumed to be Guardian errors
  // on an ESP control connection. This assumption might also change
  // in the future. For this case do not generate new diagnostics if
  // the diags area already contains the error.
 
  // Case (a)
  if (smErrorNumber_ == SM_ERR_NOPEER  ||
      smErrorNumber_ == SM_ERR_NOSERVICE  ||
      smErrorNumber_ == SM_ERR_NODE_DOWN)
  {
    IpcAllocateDiagsArea(diags, diagsHeap);
    
    char processName[12];
    sprintf(processName, "%d,%d", smTarget_.node, smTarget_.pid);

    *diags << DgSqlCode(-EXE_SM_FUNCTION_ERROR)
           << DgString0(smErrorFunction_)
           << DgInt0((Lng32) smErrorNumber_)
           << DgInt1((Lng32) smTarget_.pid)
           << DgString1(processName)
           << DgNskCode((Lng32) 10000 + abs(smErrorNumber_));
  }

  // Case (b)
  else
  {
    if (diags && diags->contains(-EXE_SM_CONTROL_CONN_ERROR))
      return;
    
    IpcAllocateDiagsArea(diags, diagsHeap);
    
    char targetBuf[128];
    sprintf(targetBuf, "%d:%d:%" PRId64 ":%d:0x%c",
            (int) smTarget_.node, (int) smTarget_.pid, smTarget_.id,
            (int) ExSMTag_GetTagWithoutQualifier(smTarget_.tag),
            (char) ExSMTag_GetQualifierDisplay(smTarget_.tag));

    *diags << DgSqlCode(-EXE_SM_CONTROL_CONN_ERROR)
           << DgString1(targetBuf)
           << DgInt0(smErrorNumber_);

    getEnvironment()->getMyOwnProcessId(IPC_DOM_GUA_PHANDLE).
      addProcIdToDiagsArea(*diags,0);
  }
}

// Several data members must be maintained and kept in sync as the
// connection begins chunk mode, moves to the next chunk, and ends
// chunk mode. The following methods encapsuate the ENTER, NEXT, and
// EXIT operations.
void SMConnection::enterChunkMode(IpcMessageBuffer *sendBuffer,
                                  UInt32 sqlChunkSize)
{
  chunk_buffer_ = sendBuffer;
  chunk_nextOffset_ = 0;
  chunk_size_ = sqlChunkSize;
  setAckArrived(false,  // Clear the ACK ARRIVED flag
                true);  // Set the WAITING FOR ACK flag
  
  EXSM_TRACE(EXSM_TRACE_BUFFER, "CONN %p ENTER CHUNK buf %p sz %d", this,
             sendBuffer, (int) sqlChunkSize);
}

void SMConnection::moveToNextChunk()
{
  chunk_nextOffset_ += chunk_size_;

  EXSM_TRACE(EXSM_TRACE_BUFFER, "CONN %p NEXT CHUNK offset %d", this,
             (int) chunk_nextOffset_);
}

void SMConnection::exitChunkMode()
{
  chunk_buffer_ = NULL;
  chunk_nextOffset_ = 0;
  chunk_size_ = 0;
  setAckArrived(false,  // Clear the ACK ARRIVED flag
                false); // Clear the WAITING FOR ACK flag

  EXSM_TRACE(EXSM_TRACE_BUFFER, "CONN %p EXIT CHUNK", this);
}

// The following two methods manage the ACK ARRIVED attribute of this
// connection. The attribute is actually stored in the task object
// which makes the attribute visible and writeable by the reader
// thread.
void SMConnection::setAckArrived(bool arrived, bool waitForAnother)
{
  // Set the ACK ARRIVED flag in the task object
  if (smTask_)
    smTask_->sendChunk_SetAckArrived(arrived);

  // Set the WAITING FOR ACK flag in the connection
  chunk_waitingForAck_ = waitForAnother;
}

bool SMConnection::getAckArrived()
{
  bool result = false;
  if (smTask_)
    result = smTask_->sendChunk_GetAckArrived();
  return result;
}

void SMConnection::dumpAndStopOtherEnd(bool doDump, bool doStop) const
{
  char coreFile[1024];
  char processName[MS_MON_MAX_PROCESS_NAME+1];
  char seqName[PhandleStringLen];
  if (XZFIL_ERR_OK == msg_mon_get_process_name2(smTarget_.node,
                                                smTarget_.pid,
                                                smTarget_.verifier,
                                                processName))
  {

    sprintf(seqName, "%s:%d", processName, smTarget_.verifier);

     if (doDump)
       msg_mon_dump_process_name(NULL, seqName, coreFile);

     if (doStop)
       msg_mon_stop_process_name(seqName);
  }
}
