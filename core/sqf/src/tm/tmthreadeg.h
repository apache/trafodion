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

#ifndef TMTHREADEG_H_
#define TMTHREADEG_H_

#include <sys/types.h>
#include <sys/time.h>
#include "tmmmap.h"
#include "seabed/thread.h"
#include "tmlibmsg.h"
#include "tmtxmsg.h"
#include "tmlogging.h"
#include "tmevent.h"
#include "tmeventq.h"
#include "tmthread.h"
#include "tmtime.h"


// CTmThreadExample class definition
// This is a sample thread.
// This example assumes it will be built in the main TM, not in a library.
class CTmThreadExample :public CTmThread
{
public:
   bool iv_stopped;
   int iv_count;
   CTmTimerEvent *ip_tevent;

   CTmThreadExample(SB_Thread::Sthr::Function pv_fun, int64 pv_num, const char *pp_name);
   ~CTmThreadExample();

   // Event queue management methods inherited from CTmThread
   void eventQ_push(CTmEvent *pp_event);
   CTmEvent * eventQ_pop();
}; //CTmThreadExample



// Example thread main line is not a method against the object.
extern void * exampleThread_main(void *arg);

#endif //TMTHREADEG_H_
