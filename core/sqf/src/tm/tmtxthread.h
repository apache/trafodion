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

#ifndef TMTXTHREAD_H_
#define TMTXTHREAD_H_

#include "seabed/thread.h"
#include "tmlibmsg.h"
#include "tmdeque.h"
#include "tmtimer.h"
//#include "tmtx.h"
#include "tmthread.h"
#include "tmpoolelement.h"
#include "tmpool.h"

// EID must be exactly 9 characters
const char EID_CTxThread[] = {"CTxThread"}; 

class TM_TX_Info;

// CTxThread class definition
// Each CTxThread object corresponds to a transaction thread.
// CTxThread objects are maintained in a pool and allocated to active
// transactions as needed.  Normally they stay associated with the same 
// transaction for the life of that transaction, but resource shortages
// and long running transactions can cause the thread to be dissociated,
// and a new thread associated when needed.

class CTxThread :public CTmThread, 
                 public virtual CTmPoolElement
                 
{
private:
   TM_TX_Info *ip_txn;
   long int iv_id;
   TM_TX_TH_STATE iv_state;

public:
   CTxThread(SB_Thread::Sthr::Function pv_fun, int64 pv_num, const char *pp_name);
   ~CTxThread();
   void reset();

   // Callbacks for ThreadPool
   static CTxThread *constructPoolElement(int64 pv_threadNum);
   int64 cleanPoolElement();

   void associate(TM_TX_Info *pp_txn);
   bool disassociate();
   long int get_id() {return iv_id;}
   void set_id(long int pv_id) {iv_id = pv_id;}


   // Event queue management methods
   void eventQ_push(CTmEvent *pp_event);
   void eventQ_push_top(CTmEvent *pp_event);
   CTmEvent *eventQ_pop();

   //Get/Set methods:
   TM_TX_Info * transaction() {return ip_txn;}
   TM_TX_TH_STATE state() {return iv_state;}
   void state(TM_TX_TH_STATE pv_state)
   {
      lock();
      iv_state = pv_state;
      unlock();
   }
};


// thread main line is not a method against the object, I couldn't get
// it to work that way.
void * txThread_main(void *arg);
#endif //TMTXTHREAD_H_

