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
#ifndef EXSM_QUEUE_H
#define EXSM_QUEUE_H

#include "ExSMCommon.h"

class NAMemory;

//--------------------------------------------------------------------------
// Notes on class ExSMQueue
//--------------------------------------------------------------------------
// ExSMQueue provides a fixed-size, lock-free queue
//
// Most of the code in this class is copied from class ex_queue
//
// There is exactly one reader and one writer
//
// The reader never writes, the writer never reads
// 
// Queue elements are stored in a circular array
// 
// Two indexes head_ and tail_ track the head and tail entries
// 
// head_ and tail_ are allowed to wrap around
// 
// The queue is empty when head_ == tail_
//
// The number of entries is (tail_ - head_). If tail_ happens to
// wraparound before head_, we assume hardware arithmetic still
// reports the correct number of entries. This assumption also exists
// in class ex_queue.
//
// The maximum number of entries that can be stored is (size_ - 1)
//
// If we tried to store size_ elements we would have head_ == tail_
// when the queue is full and this violates the rule that the queue is
// empty when head_ == tail_.
//
// size_ is always a power of 2. This makes modulo arithmetic fast.
//
// The ex_queue protocol must be followed when adding or removing:
// 
//   Before adding, make sure the queue is not full
// 
//   Before removing, make sure the queue is not empty
// 
//   First the caller modifies a queue element and then
//   does the add or remove. The add or remove only does
//   an increment of head_ or tail_
//
// For lock-free thread safety a compiler memory barrier is added to
// the add/remove methods:
//
//   The lock-free protocol requires that the following two steps are
//   never reordered by gcc:
//
//    1. Modification of a queue element
//    2. Increment of the data member tail_ (for insert) or
//       head_ (for remove)
// 
//   Without a compiler memory barrier, steps 1 and 2 could be
//   reordered. The compiler memory barrier is implemented with the
//   following gcc directive:
//
//     asm volatile("" : : : "memory");

class ExSMQueue
{
public:
  class Entry
  {
  public:
    void *getData() { return data_; }
    void setData(void *data) { data_ = data; }

  protected:
    Entry() {}
    virtual ~Entry() {}
    void *data_;
  };
  
  ExSMQueue(uint32_t initialSize, NAMemory *heap);
  
  ExSMQueue(); // Do not implement
  
  // If the queue is used to store pointers, this destructor will not
  // delete the objects pointed to. The user of queue is responsible
  // for deleting objects before calling this destructor.
  virtual ~ExSMQueue();
  
  Entry &getTailEntry() const
  {
    return queue_[tail_ & mask_];
  }
  
  uint32_t getTailIndex() const
  {
    return tail_;
  }

  void insert()
  {
    // We assert that queue is never full when insert() is called. The
    // assert will only fail if the caller does not test isFull()
    // before calling insert().
    exsm_assert(!isFull(), "insert called when queue is full");

    asm volatile("" : : : "memory");
    tail_++;
  }

  Entry &getQueueEntry(uint32_t i) const
  {
    exsm_assert(!isVacant(i), "getQueueEntry called for vacant entry");
    return queue_[i & mask_];
  }

  Entry &getHeadEntry() const
  {
    exsm_assert(!isEmpty(), "getHeadEntry called for empty queue");
    return queue_[head_ & mask_];
  }
  
  uint32_t getHeadIndex() const
  {
    return head_;
  }
  
  bool entryExists(uint32_t i) const
  {
    return (!isVacant(i));
  }

  void removeHead()
  {
    // We assert that queue is never empty when removeHead() is
    // called. The assert will only fail if the caller does not test
    // isEmpty() before calling removeHead().
    exsm_assert(!isEmpty(), "removeHead called for empty queue");

    asm volatile("" : : : "memory");
    head_++;
  }

  uint32_t getSize() const { return size_; }
  uint32_t getLength() const { return tail_ - head_; }
  
  bool isFull() const { return (getLength() == mask_); }
  bool isEmpty() const { return (head_ == tail_); }

  bool isVacant(uint32_t i) const
  {
    // We have two cases to consider
    // * There is no wraparound and head_ <= tail_
    // * tail_ has wrapped around before head_
    if (head_ <= tail_)
      return !((head_ <= i) && (i < tail_));
    else
      return !((head_ <= i) || (i < tail_));  
  }

private:

  uint32_t head_;
  uint32_t tail_;
  uint32_t size_;

  // Using a bit mask is faster than masking with (size_ - 1).
  uint32_t mask_;

  // Pointer to an array of queue entries
  Entry *queue_;

  NAMemory *heap_;
 
}; // class ExSMQueue

#endif // EXSM_QUEUE_H
