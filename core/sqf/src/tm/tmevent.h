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

#ifndef TMEVENT_H_
#define TMEVENT_H_

#include <sys/types.h>
#include <sys/time.h>
#include "seabed/thread.h"
#include "tmlibmsg.h"
#include "tmtxmsg.h"
#include "tmlogging.h"
//#include "tmthread.h"


// Forward declaration
class CTmThread;


// CTmEvent class definition
// TM Event class template.
class CTmEvent :public CTmTxMessage
{
private:
   // Thread which will execute this event, or the thread
   // for which this event is eventually destined.
   CTmThread *ip_thread; 

public:
   CTmEvent(short pv_reqType, CTmThread *pp_thread) 
      :CTmTxMessage(pv_reqType), ip_thread(pp_thread)
   {
   }
   CTmEvent(short pv_reqType) 
      :CTmTxMessage(pv_reqType), ip_thread(NULL)
   {}
   CTmEvent(Tm_Req_Msg_Type * pp_req) 
      :CTmTxMessage(pp_req), ip_thread(NULL)
   {}
   CTmEvent(CTmTxMessage * pp_msg) 
      :CTmTxMessage(pp_msg), ip_thread(NULL)
   {}
   ~CTmEvent() {}

   CTmThread *thread() {return ip_thread;}
   CTmTxMessage *msg() {return (CTmTxMessage *) this;}
}; //CTmEvent

#endif //TMEVENT_H_
