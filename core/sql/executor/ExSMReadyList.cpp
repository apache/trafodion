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

#include "ExSMReadyList.h"
#include "ExSMTask.h"
#include "ExSMTrace.h"

// NOTE: The SM ready list is accessed by both threads (main and
// reader) but does not have its own lock. By convention,
// modifications to the ready list are always performed while holding
// a lock on the SM task list (a global collection of all SM tasks in
// this process).

ExSMReadyList::ExSMReadyList()
  : head_(NULL)
{
}

ExSMReadyList::~ExSMReadyList()
{
}


void ExSMReadyList::add(ExSMTask *t)
{
  EXSM_TRACE(EXSM_TRACE_THR_ALL, "READY LIST ADD %p", t);

  if (!head_)
  {
    // The list is currently empty. Set the head, next, and prev
    // pointers to all point to the new task.
    t->readyListNext_ = t;
    t->readyListPrev_ = t;
    head_ = t;
  }
  else
  {
    // Add the new task to the end of the list
    // * t->next will point to the current head
    // * t->prev will point to the current tail
    // * head->prev will point to the new task
    // * tail->next will point to the new task
    ExSMTask *tail = head_->readyListPrev_;
    exsm_assert(tail, "Ready list tail pointer should not be NULL");
    t->readyListNext_ = head_;
    t->readyListPrev_ = tail;
    head_->readyListPrev_ = t;
    tail->readyListNext_ = t;
  }
}

void ExSMReadyList::remove(ExSMTask *t)
{
  EXSM_TRACE(EXSM_TRACE_THR_ALL, "READY LIST REMOVE %p", t);

  // Get pointers to the next and prev neighbors
  ExSMTask *next = t->readyListNext_;
  ExSMTask *prev = t->readyListPrev_;

  exsm_assert(head_, "Ready list should not be NULL");
  exsm_assert(next && prev, "Ready list neighbors should not be NULL");

  // Clear pointers in the task being removed
  t->readyListNext_ = NULL;
  t->readyListPrev_ = NULL;

  if (next == t)
  {
    // If the task was its own neighbor, that means it was the only
    // element in the list. The list now becomes empty.
    head_ = NULL;
  }
  else
  {
    // Update pointers in the neighbors
    next->readyListPrev_ = prev;
    prev->readyListNext_ = next;

    // If the task was the first element, head_ will now point to the
    // next neighbor
    if (head_ == t)
      head_ = next;
  }
}

ExSMTask *ExSMReadyList::getFirst()
{
  return head_;
}
