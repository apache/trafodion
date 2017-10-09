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

#include "ExSMTaskList.h"
#include "ExSMTask.h"
#include "ExSMTrace.h"

const int EXSM_DEFAULT_NUM_BUCKETS = 513;

// Compute a 32-bit hash value from an sm_target structure. The hash
// values are used inside the ExSMTaskList class, to break the global
// collection of tasks into a set of hash buckets.
uint32_t hash(const sm_target_t &t, uint32_t numBuckets)
{
  // We can return early if the number of buckets is 1
  if (numBuckets == 1)
    return 0;

  const uint32_t R = 31;
  int64_t result = t.node;
  result = (result * R) + t.pid;
  result = (result * R) + t.id;
  result = (result * R) + t.tag;
  return (uint32_t) (result % numBuckets);
}

ExSMTaskList::ExSMTaskList()
  : buckets_(NULL),
    numBuckets_(EXSM_DEFAULT_NUM_BUCKETS),
    numTasks_(0),
    numInserts_(0),
    numCollisions_(0)
{
  char *e = getenv("EXSM_NUM_BUCKETS");
  if (e && e[0])
  {
    int temp = atoi(e);
    if (temp > 0)
      numBuckets_ = (uint32_t) temp;
  }

  buckets_ = new ExSMTask *[numBuckets_];
  memset(buckets_, 0, sizeof(ExSMTask *) * numBuckets_);
  EXSM_TRACE(EXSM_TRACE_INIT,
             "TASK LIST CTOR %p buckets %u", this, numBuckets_);

  int rc = pthread_mutex_init(&taskListMutex_, NULL);
  exsm_assert_rc(rc, "pthread_mutex_init");
}

ExSMTaskList::~ExSMTaskList()
{
  delete [] buckets_;
  EXSM_TRACE(EXSM_TRACE_INIT,
             "TASK LIST DTOR %p inserts %ld collisions %ld",
             this, numInserts_, numCollisions_);

  int rc = pthread_mutex_destroy(&taskListMutex_);
  exsm_assert_rc(rc, "pthread_mutex_destroy");
}

void ExSMTaskList::lock()
{
  int rc = pthread_mutex_lock(&taskListMutex_);
  exsm_assert_rc(rc, "pthread_mutex_lock");
}

void ExSMTaskList::unlock()
{
  int rc = pthread_mutex_unlock(&taskListMutex_);
  exsm_assert_rc(rc, "pthread_mutex_unlock");
}

void ExSMTaskList::addTask(ExSMTask *t)
{
  exsm_assert(t, "addTask called with NULL task");

  lock();

  uint32_t hashval = hash(t->getReceiver(), numBuckets_);
  ExSMTask *&head = buckets_[hashval];
  if (head != NULL)
    numCollisions_++;

  t->hashBucketNext_ = head;
  head = t;
  numTasks_++;
  numInserts_++;
  
  unlock();
}

void ExSMTaskList::removeTask(ExSMTask *t)
{
  exsm_assert(t, "removeTask called with NULL task");

  lock();

  uint32_t hashval = hash(t->getReceiver(), numBuckets_);
  ExSMTask *&head = buckets_[hashval];
  bool found = false;
  ExSMTask *previous = NULL;
  ExSMTask *current = head;
  
  while (current)
  {
    if (t == current)
    {
      if (previous)
        previous->hashBucketNext_ = current->hashBucketNext_;
      else
        head = current->hashBucketNext_;
      
      found = true;
      numTasks_--;
      
      break;
    }
    
    previous = current;
    current = current->hashBucketNext_;
  }

  unlock();

  exsm_assert(found, "removeTask failed to find the task");
}

ExSMTask *ExSMTaskList::findTask(const sm_target_t &target,
                                 bool doLock,
                                 bool doTrace)
{
  if (doLock)
    lock();
  
  uint32_t hashval = hash(target, numBuckets_);
  ExSMTask *&head = buckets_[hashval];
  ExSMTask *result = NULL;
  ExSMTask *current = head;

  while (current)
  {
    const sm_target_t &receiver = current->getReceiver();

    if (doTrace)
      EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT,
                 "FINDTASK "
                 "%d:%d:%" PRId64 ":%d:0x%c %d:%d:%" PRId64 ":%d:0x%c", 
                 (int) target.node, (int) target.pid, target.id,
                 (int) ExSMTag_GetTagWithoutQualifier(target.tag),
                 (char) ExSMTag_GetQualifierDisplay(target.tag),
                 (int) receiver.node, (int) receiver.pid, receiver.id,
                 (int) ExSMTag_GetTagWithoutQualifier(receiver.tag),
                 (char) ExSMTag_GetQualifierDisplay(receiver.tag));
    
    if (ExSM_TargetsEqual(receiver, target))
    {
      result = current;
      break;
    }
    current = current->hashBucketNext_;
  }

  if (doLock)
    unlock();
  
  return result;
}
