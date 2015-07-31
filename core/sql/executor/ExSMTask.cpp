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

#include "ExSMTask.h"
#include "ExSMTaskList.h"
#include "ExSMReadyList.h"
#include "ExSMGlobals.h"
#include "ExSMQueue.h"
#include "Ipc.h"

ExSMTask::ExSMTask(const sm_target_t &receiver,
                   uint32_t queueSize,
                   int32_t *scheduledAddr,
                   NAMemory *heap,
                   ex_tcb *tcb,
                   SMConnection *smConnection)
  : inQueue_(NULL)
  , outQueue_(NULL)
  , receiver_(receiver)
  , scheduledAddr_(scheduledAddr)
  , heap_(heap)
  , tcb_(tcb)
  , receiveCount_(0)
  , smConnection_(smConnection)
  , hashBucketNext_(NULL)
  , readyListNext_(NULL)
  , readyListPrev_(NULL)
  , recvChunk_buffer_(NULL)
  , recvChunk_msgSize_(0)
  , recvChunk_chunkSize_(0)
  , recvChunk_bytesSoFar_(0)
  , sendChunk_ackArrived_(false)
{
  exsm_assert(heap_, "Invalid NAMemory pointer");

  // In the queue constructors we add 1 to the passed in size because
  // the queue class only guarantees storage for (size - 1)
  // entries. The queue class also raises its size to a power of
  // 2. See ExSMQueue.h for details. One effect is, if you want
  // storage for N entries and N is a power of 2 (say 4) the queue
  // will allocate storage for 2N (8) entries which wastes some
  // memory.

  inQueue_ = new ExSMQueue(queueSize + 1, heap);
  exsm_assert(inQueue_, "Failed to allocate input task queue");

  outQueue_ = new ExSMQueue(queueSize + 1, heap);
  exsm_assert(outQueue_, "Failed to allocate output task queue");
}

ExSMTask::~ExSMTask()
{
  if (recvChunk_buffer_)
  {
    // If recvChunk_buffer_ is not NULL, the task was waiting for an
    // arrival that never came. The arrival never made its way to the
    // task output queue. No one else has a pointer to this buffer so
    // the task needs to release the buffer before going away.
    IpcMessageBuffer *msgBuf = (IpcMessageBuffer *) recvChunk_buffer_;
    msgBuf->decrRefCount();
    recvChunk_buffer_ = NULL;
  }

  if (inQueue_)
  {
    while (!inQueue_->isEmpty())
    {
      ExSMQueue::Entry &entry = inQueue_->getHeadEntry();
      IpcMessageBuffer *msgBuf = (IpcMessageBuffer *) entry.getData();
      exsm_assert(msgBuf, "Invalid msgBuf pointer in SM queue entry");
      exsm_assert(msgBuf->getRefCount() == 1,
                  "Ref count should be 1 for buffer in SM task queue");
      msgBuf->decrRefCount();
      inQueue_->removeHead();
    }
  }

  if (outQueue_)
  {
    ExSMTaskList *taskList = ExSMGlobals::GetExSMGlobals()->getSMTaskList();
    bool taskListLocked = false;

    // This task has already been removed from the globals task list
    // by ExSMGlobals::removeTask(). Therefore it is not necessary to
    // obtain the task list lock before emptying the output
    // queue. However, for now, the output queue and completed task
    // queue will be locked as a pair for consistency with other code
    // locations. In other words, in any code path where the output
    // queue can become empty, we lock the task list and remove the
    // task from the completed task queue.
    if (!outQueue_->isEmpty())
    {
      taskList->lock();
      taskListLocked = true;
    }

    while (!outQueue_->isEmpty())
    {
      ExSMQueue::Entry &entry = outQueue_->getHeadEntry();
      IpcMessageBuffer *msgBuf = (IpcMessageBuffer *) entry.getData();
      exsm_assert(msgBuf, "Invalid msgBuf pointer in SM queue entry");
      exsm_assert(msgBuf->getRefCount() == 1,
                  "Ref count should be 1 for buffer in SM task queue");
      msgBuf->decrRefCount();
      outQueue_->removeHead();
    }
  
    if (taskListLocked)
    {
      // NOTE: The SM ready list is accessed by both threads (main and
      // reader) but does not have its own lock. By convention,
      // modifications to the ready list are always performed while
      // holding a lock on the SM task list (a global collection of
      // all SM tasks in this process).
      ExSMReadyList *readyList = ExSMGlobals::GetExSMGlobals()->getReadyList();
      readyList->remove(this);
      taskList->unlock();
    }

  } // if (outQueue_)

  delete inQueue_;
  delete outQueue_;
}

// Methods for RECEIVE chunk protocol
void ExSMTask::recvChunk_Enter(void *buffer, uint32_t msgSize,
                               uint32_t chunkSize)
{
  recvChunk_buffer_ = buffer;
  recvChunk_bytesSoFar_ = 0;
  recvChunk_msgSize_ = msgSize;
  recvChunk_chunkSize_ = chunkSize;
}

void ExSMTask::recvChunk_Exit()
{
  recvChunk_buffer_ = NULL;
  recvChunk_bytesSoFar_ = 0;
  recvChunk_msgSize_ = 0;
  recvChunk_chunkSize_ = 0;
}

void ExSMTask::recvChunk_Receive(uint32_t bytesReceived)
{
  recvChunk_bytesSoFar_ += bytesReceived;
}

void *ExSMTask::recvChunk_GetPrepostAddr() const
{
  void *result = NULL;
  IpcMessageBuffer *msgBuf = (IpcMessageBuffer *) recvChunk_buffer_;
  if (msgBuf)
    result = msgBuf->data(recvChunk_bytesSoFar_);
  return result;
}

bool ExSMTask::recvChunk_MoreExpected() const
{
  return (recvChunk_bytesSoFar_ < recvChunk_msgSize_);
}

// Methods for SEND chunk protocol
void ExSMTask::sendChunk_SetAckArrived(bool b)
{
  bool notify = false;
  if (sendChunk_ackArrived_ == false && b == true)
    notify = true;

  sendChunk_ackArrived_ = b;

  if (notify)
  {
    exsm_assert(scheduledAddr_, "Invalid scheduledAddr_ pointer");
    *scheduledAddr_ = 1;
  }
}
