// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

#ifndef TMTIMER_H_
#define TMTIMER_H_

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

enum TmTimer_State
{
   TmTimer_Down   = 0,
   TmTimer_Up     = 1
};

enum TmTimer_Command
{
   TmTimerCmd_Queue     = 101,   // Queue a timer event
   TmTimerCmd_Timer     = 102,   // Timer event (on timerList).
   TmTimerCmd_Stop      = 103,   // Stop timer Thread
   TmTimerCmd_Cancelled = 104    // A cancelled timer event
};

#define TmTimerRepeat_None     0
#define TmTimerRepeat_Forever -1

// Forward declarations
class CTmTxBase;


// CTmTimerEvent class definition
// Timer events are posted to the timer objects event queue to initiate a 
// timer.
class CTmTimerEvent :public CTmEvent
{
private:
   TmTimer_Command iv_cmd;
   CTmTxBase *ip_transaction;
   int iv_wakeupInterval; // in msecs
   int iv_wakeupRepeat;

public:
   CTmTimerEvent(short pv_reqType, CTmThread *pp_thread, CTmTxBase *pp_transaction,int pv_wakeupInterval) 
      :CTmEvent(pv_reqType, pp_thread), iv_cmd(TmTimerCmd_Queue),
       ip_transaction(pp_transaction), 
       iv_wakeupInterval(pv_wakeupInterval), iv_wakeupRepeat(TmTimerRepeat_None)
   {}
   CTmTimerEvent(short pv_reqType, CTmThread *pp_thread, int pv_wakeupInterval, int pv_wakeupRepeat) 
      :CTmEvent(pv_reqType, pp_thread), iv_cmd(TmTimerCmd_Queue),
       ip_transaction(NULL),
       iv_wakeupInterval(pv_wakeupInterval), iv_wakeupRepeat(pv_wakeupRepeat)
   {}
   CTmTimerEvent(TmTimer_Command pv_cmd, CTmThread *pp_thread)
      :CTmEvent(TM_MSG_TYPE_NULL, pp_thread), iv_cmd(pv_cmd),
       ip_transaction(NULL), iv_wakeupInterval(0),
       iv_wakeupRepeat(TmTimerRepeat_None)
   {}
   // Constructors for timer events originating in main thread.
   CTmTimerEvent(CTmTxMessage *pp_msg, int pv_wakeupInterval, int pv_wakeupRepeat = TmTimerRepeat_None)
      :CTmEvent(pp_msg), iv_cmd(TmTimerCmd_Queue),
       ip_transaction(NULL), iv_wakeupInterval(pv_wakeupInterval),
       iv_wakeupRepeat(pv_wakeupRepeat)
   {} 
   CTmTimerEvent(short pv_reqType, int pv_wakeupInterval, int pv_wakeupRepeat = TmTimerRepeat_None)
      :CTmEvent(pv_reqType), iv_cmd(TmTimerCmd_Queue),
       ip_transaction(NULL), iv_wakeupInterval(pv_wakeupInterval),
       iv_wakeupRepeat(pv_wakeupRepeat)
   {}  
   ~CTmTimerEvent() {}

   TmTimer_Command command() {return iv_cmd;}
   void command(TmTimer_Command pv_cmd) {iv_cmd = pv_cmd;}
   CTmTxBase *transaction() {return ip_transaction;}
   int wakeupInterval() {return iv_wakeupInterval;}
   int wakeupRepeat() {return iv_wakeupRepeat;}
   void dec_wakeupRepeat() 
   {
      if (iv_wakeupRepeat > 0)
         iv_wakeupRepeat--;
   }
}; //CTmTimerEvent


// CTmTimerList class definition
// The timer list contains all oustanding timers 
class CTmTimerList :public TM_MMAP
{
public:
   // Timer list management methods
   void add(CTmTimerEvent *pp_event);
   CTmTime getFirst();
   CTmTimerEvent * pop(CTmTime *pp_time);

   CTmTimerList() {}
   ~CTmTimerList() {}
}; //CTmTimerList


// CTmTimer class definition
// There should only be a single TM timer object instantiated.  This runs
// in it's own thread and handles timer requests for all threads in the TM.
class CTmTimer :public CTmThread
{
private: 
   TmTimer_State iv_state;
   Ctimeval iv_startTime;      // Time the TM started

   // Time to wait in eventQ_pop if time list empty.
   // 0 = wait forever
   Ctimeval iv_defaultWaitTime; 

   // Control Point processing
   time_t iv_last_cp_written; //Just for debugging.
   CTmTimerEvent *ip_cp_event; //Saved so it can be changed.
   CTmTimerEvent *ip_stats_event; //Saved so it can be changed.
   CTmTimerEvent *ip_RMRetry_event; //Saved so it can be changed.
   CTmTimerEvent *ip_ShutdownP1_event;

   // List of outstanding timer events
   CTmTimerList iv_timerList;

public:
   CTmTimer(SB_Thread::Sthr::Function pv_fun, int64 pv_num, const char *pp_name,
            int64 pv_defaultWaitTime);
   ~CTmTimer();

   TmTimer_State state() {return iv_state;}
   void state(TmTimer_State pv_state) {iv_state = pv_state;}
   CTmTimerList *timerList() {return &iv_timerList;}
   CTmTimerEvent *ShutdownP1_event() {return ip_ShutdownP1_event;}
   void ShutdownP1_event(CTmTimerEvent *pp_event) 
         {ip_ShutdownP1_event = pp_event;}

   Ctimeval startTime() {return iv_startTime;}
   Ctimeval defaultWaitTime() {return iv_defaultWaitTime;}
   void defaultWaitTime(int64 pv_msec) 
   {
      // -1 = wait forever which we set to 0 here.
      if (pv_msec == -1)
         iv_defaultWaitTime = CTmTime::msectotimeval(0);
      else
         iv_defaultWaitTime = CTmTime::msectotimeval(pv_msec);
   }
   time_t last_cp_written() {return iv_last_cp_written;}
   void last_cp_written(time_t pv_last_cp_written)
   { iv_last_cp_written = pv_last_cp_written; }

   bool calculateWait(Ctimeval *pp_waitTime);

   // Event queue management methods inherited from CTmThread
   void eventQ_push(CTmEvent *pp_event);
   CTmEvent * eventQ_pop();
   void cancelEvent(CTmTimerEvent *pp_tevent);
   void addControlpointEvent(int32 pv_cp_interval);
   void cancelControlpointEvent();
   void addStatsEvent(int32 pv_interval);
   void cancelStatsEvent();
   void addRMRetryEvent(int32 pv_interval);
   void cancelRMRetryEvent();
}; //CTmTimer



// Timer thread main line is not a method against the object.
extern void * timerThread_main(void *arg);

#endif //TMTIMER_H_
