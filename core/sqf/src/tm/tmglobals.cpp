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

// tmglobals is included in XATM Library
#define XATM_LIB

#include "tmglobals.h"

#ifdef XATM_LIB
// XATM Library definitions
#define TMTIMER gv_xaTM.tmTimer()
#define TRACE(level, mask, a) XATrace(mask, a)

#else
// TM definitions
#define TMTIMER gv_tm_info.tmTimer()
#define TRACE(level, mask, a) TMTrace(level, a)
#endif //XATM_LIB


// ----------------------------------------------------------------------------
// tm_addTimerEvent
// Purpose : Wrapper to simplify the addition of timer events to the timer 
// threads event queue for events with no associated transaction.  
// Events will always be queued back against the calling thread objects event 
// queue.
// pv_type is a message type
// pv_delayInterval is the interval the timer thread will wait before
// posting the event back to the transaction in msec.
// pv_repeatCount is the number of times to repeat this event.  Each time the 
// pv_delayInterval occurs, the timer thread will post this event back to the 
// thread.  A value of -1 indicates repeat forever.
// ----------------------------------------------------------------------------
CTmTimerEvent * tm_addTimerEvent(TM_MSG_TYPE pv_type, CTmThread * pp_thread, int pv_delayInterval, int pv_repeatCount)
{
   TRACE (2, XATM_TraceExit, ("tm_addTimerEvent : ENTRY, msg type %d, thread %p, delay %d, repeat %d\n", 
      pv_type, (void *) pp_thread, pv_delayInterval, pv_repeatCount));

   CTmTimerEvent *lp_timerEvent = new CTmTimerEvent(pv_type, pp_thread, pv_delayInterval, pv_repeatCount);
   TMTIMER->eventQ_push((CTmEvent *) lp_timerEvent);

   return lp_timerEvent;
} // tm_addTimerEvent


// ----------------------------------------------------------------------------
// tm_stopTimerEvent 
// Purpose : Wrapper to simplify/illustrate stopping of a timer event.
// ----------------------------------------------------------------------------
void tm_stopTimerEvent(CTmTimerEvent * pp_timerEvent)
{
   TMTIMER->cancelEvent(pp_timerEvent);
} // tm_stopTimerEvent

