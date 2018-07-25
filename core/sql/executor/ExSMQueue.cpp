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
#include "Platform.h"
#include "ExSMQueue.h"
#include "NAMemory.h"

ExSMQueue::ExSMQueue(uint32_t initialSize, NAMemory *heap)
  : head_(0),
    tail_(0),
    size_(initialSize),
    mask_(0),
    queue_(NULL),
    heap_(heap)
{
  // We want to raise size_ to the next highest power of 2. This
  // allows for fast modulo arithmetic when we index the circular
  // array of queue entries. Two steps are required:
  // * N = the number of significant bits in size_
  // * a bit-shift to compute (2 ** N)

  uint32_t significantBits = 1;     
  uint32_t s = size_ - 1;
  while (s > 1)
  {
    significantBits++;
    s = s >> 1;
  }
  exsm_assert(significantBits < 32, "Too many bits in size_ variable");
  size_ = (1 << significantBits);
  
  exsm_assert(heap_, "Invalid NAMemory pointer");
  uint32_t numBytes = size_ * sizeof(Entry);
  queue_ = (Entry *) heap_->allocateMemory(numBytes);
  exsm_assert(queue_, "Failed to allocate queue entries");
  memset(queue_, 0, numBytes);

  head_ = UINT32_MAX - 1;
  tail_ = UINT32_MAX - 1;
  mask_ = size_ - 1;
}

ExSMQueue::~ExSMQueue()
{
  // If the queue is used to store pointers, this destructor will not
  // delete the objects pointed to. The user of queue is responsible
  // for deleting objects before calling this destructor.

  // As of November 2011 the only user of this class is ExSMTask and
  // the ExSMTask destructor takes care of deleting buffers before
  // calling this queue destructor

  if (queue_)
    heap_->deallocateMemory(queue_);
}
