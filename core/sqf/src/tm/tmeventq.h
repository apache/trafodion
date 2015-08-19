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

#ifndef TMEVENTQ_H_
#define TMEVENTQ_H_

#include <sys/types.h>
#include <sys/time.h>
#include "seabed/thread.h"
#include "tmlibmsg.h"
#include "tmtxmsg.h"
#include "tmlogging.h"
#include "tmdeque.h"
#include "tmevent.h"

// Forward declarations
class CTmEvent;


// CTmEventQ class definition
// This is the template for TM event queues.
// To use this class, your class should inherit it and implement it's own 
// versions of eventQ_push and eventQ_pop.
class CTmEventQ :public TM_DEQUE
{
private: 
   SB_Thread::CV iv_CV; // Condition variable for controlling the event queue

public:
   CTmEventQ() {}
   ~CTmEventQ() {}

   SB_Thread::CV * eventQ_CV() {return &iv_CV;}
   TM_DEQUE * eventQ() {return (TM_DEQUE *) this;}

   // The following must be implemented by any classes which inherit from CTmEventQ!!
   virtual void eventQ_push(CTmEvent *pp_event);
   virtual CTmEvent * eventQ_pop();
}; //CTmEventQ

#endif //TMEVENTQ_H_
