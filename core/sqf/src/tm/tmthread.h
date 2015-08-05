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

#ifndef TMTHREAD_H_
#define TMTHREAD_H_

#include <sys/types.h>
#include <sys/time.h>
#include "tmmutex.h"
#include "tmlibmsg.h"
#include "tmeventq.h"


// Forward declaration
class CTmTimerEvent;

// CTmThread class definition
// This is the template for all thread classes with the TM.
// Each thread class must implement it's own versions of CTmEventQ::eventQ_push
// and CTmEventQ::eventQ_pop.
class CTmThread :public SB_Thread::Thread, public CTmEventQ
{
private: 
   int64 iv_threadNum;
   TM_Mutex iv_mutex; // Semaphore to serialize updates to the object.

public:
   CTmThread(SB_Thread::Sthr::Function pv_fun, int64 pv_num, const char *pp_name);
   ~CTmThread();

   void lock();
   void unlock();
   int64 threadNum() {return iv_threadNum;}
}; //CTmThread

#endif //TMTHREAD_H_
