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
#ifndef EXSM_TASKLIST_H
#define EXSM_TASKLIST_H

#include <pthread.h>
#include "ExSMCommon.h"

class ExSMTask;
class ExSMTaskList;
class ExSMQueue;

class ExSMTaskList
{
public:
  ExSMTaskList();

  virtual ~ExSMTaskList();

  void lock();
  void unlock();

  void addTask(ExSMTask *t);
  void removeTask(ExSMTask *t);

  uint32_t getNumTasks() { return numTasks_; }

  ExSMTask *findTask(const sm_target_t &target,
                     bool doLock,
                     bool doTrace);

protected:

  ExSMTask **buckets_;
  uint32_t numBuckets_;
  uint32_t numTasks_;
  int64_t numInserts_;
  int64_t numCollisions_;

  pthread_mutex_t taskListMutex_;

}; // class ExSMTaskList

#endif // EXSM_TASKLIST_H
