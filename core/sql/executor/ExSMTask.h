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
#ifndef EXSM_TASK_H
#define EXSM_TASK_H

#include "ExSMCommon.h"

class ExSMTask;
class ExSMTaskList;
class ExSMQueue;
class SMConnection;
class NAMemory;
class ex_tcb;

class ExSMTask
{
public:
  ExSMTask(const sm_target_t &receiver,
           uint32_t queueSize,
           int32_t *scheduledAddr,
           NAMemory *heap,
           ex_tcb *tcb,
           SMConnection *smConnection);
  
  ExSMTask(); // Do not implement

  virtual ~ExSMTask();

  ExSMQueue *getInQueue() { return inQueue_; }

  ExSMQueue *getOutQueue() { return outQueue_; }

  const sm_target_t &getReceiver() const { return receiver_; }

  int32_t *getScheduledAddr() { return scheduledAddr_; }

  // The TCB pointer is for debugging and tracing only and should not
  // be dereferenced
  ex_tcb *getTCB() { return tcb_; }

  void incrReceiveCount() { receiveCount_++; }
  int64_t getReceiveCount() { return receiveCount_; }

  SMConnection *getSMConnection() const { return smConnection_; }

  // The following methods are used for control flow when a message is
  // larger than the expected size. First the sender sends a short
  // message announcing the size of the reply. Then the sender waits
  // for an ack. The receiver posts a receive buffer of sufficient
  // size and sends an ack. Once the ack arrives the large buffer can
  // be sent. No other messages are sent by the sender while waiting
  // for the ack.

  // Methods for receiving a large buffer
  void recvChunk_Enter(void *buffer, uint32_t msgSize, uint32_t chunkSize);
  void recvChunk_Exit();
  void recvChunk_Receive(uint32_t bytesReceived);
  void *recvChunk_GetPrepostAddr() const;
  bool recvChunk_MoreExpected() const;
  void *recvChunk_GetBuffer() const { return recvChunk_buffer_; }
  uint32_t recvChunk_GetMessageSize() const { return recvChunk_msgSize_; }
  uint32_t recvChunk_GetChunkSize() const { return recvChunk_chunkSize_; }

  // Methods for sending a large buffer
  void sendChunk_SetAckArrived(bool b);
  bool sendChunk_GetAckArrived() const { return sendChunk_ackArrived_; }

protected:
  ExSMQueue *inQueue_;
  ExSMQueue *outQueue_;
  sm_target_t receiver_;
  int32_t *scheduledAddr_;
  NAMemory *heap_;

  // The TCB pointer is for debugging and tracing only and should not
  // be dereferenced
  ex_tcb *tcb_;

  int64_t receiveCount_;
  SMConnection *smConnection_;

  // The task list object maintains a table of tasks and requires
  // access to the protected field hashBucketNext_ to manage the hash
  // chains.
  friend class ExSMTaskList;
  ExSMTask *hashBucketNext_;

  // Each task contains pointers for a doubly-linked list called the
  // ready list. The reader thread places tasks on the ready list when
  // messages arrive that need to be processed by the main thread.
  // The interface to add and remove from the ready list is provided
  // by a separate class ExSMReadyList.
  friend class ExSMReadyList;
  ExSMTask *readyListNext_;
  ExSMTask *readyListPrev_;

  // The following data members are for control flow when a message is
  // larger than the expected size. The control flow protocol is
  // explained in comments above, with the control flow methods.
  // 
  // Data members for receiving a large buffer
  void *recvChunk_buffer_;         // Points to an IpcMessageBuffer
  uint32_t recvChunk_msgSize_;     // Size of the complete message
  uint32_t recvChunk_chunkSize_;   // Chunk size
  uint32_t recvChunk_bytesSoFar_;  // Bytes seen so far

  // Data members for sending a large buffer
  bool sendChunk_ackArrived_;        // Did an ack arrive?

}; // class ExSMTask

#endif // EXSM_TASK_H
