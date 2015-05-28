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

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/ms.h"
#include "seabed/thread.h"

// Define XATM_LIB here because currently this code is in the XATM Library
#define XATM_LIB

#include "tminfo.h"
#include "seabed/trace.h"
#include "tmlogging.h"
#include "tmtime.h"
#include "tmtimer.h"





//----------------------------------------------------------------------------
// CTmTimerEvent methods
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// CTmTimerList methods
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CTmTimerList::add
// Purpose : add a new event to the timer list.
// The wakeupInterval is a delay in mseconds.
//----------------------------------------------------------------------------
void CTmTimerList::add(CTmTimerEvent *pp_event)
{
   CTmTime lv_time(pp_event->wakeupInterval());

   put(lv_time.get(), pp_event);

   // Avoiding tracing here for efficiency
   TMTIME_TRACE(5, XATM_TraceTimer, ("CTmTimerList::add : EXIT wait time " PFLL ".\n",  lv_time.get()));
} // CTmTimerList::add


//----------------------------------------------------------------------------
// CTmTimerList::getFirst
// Purpose : Get the time of the first entry on the timer list.
// The CTmTime returned is the wakeup time(since EPOC) - TMSTARTTIME.
//----------------------------------------------------------------------------
CTmTime CTmTimerList::getFirst()
{
   CTmTime lv_time(0);
   CTmTimerEvent *lp_event = (CTmTimerEvent *) get_first();
   if (lp_event)
      lv_time.set(curr_key());
   get_end();


   // Avoiding tracing here for efficiency
   if (lp_event)
   {
      TMTIME_TRACE(5, XATM_TraceTimer, ("CTmTimerList::getFirst : EXIT event %p, inputTime %d, returnTime " PFLL ".\n",
         (void *) lp_event, lp_event->wakeupInterval(), lv_time.get()));
   }
   else
   {
      TMTIME_TRACE(5, XATM_TraceTimer, ("CTmTimerList::getFirst : EXIT event list empty, returnTime " PFLL ".\n",
         lv_time.get()));
   }
   return lv_time;
} // CTmTimerList::getFirst


//----------------------------------------------------------------------------
// CTmTimerList::pop
// Purpose : Pop the first element off the timer list.  This also removes it
// from the list.
//----------------------------------------------------------------------------
CTmTimerEvent * CTmTimerList::pop(CTmTime *pp_time)
{
   CTmTimerEvent *lp_event = (CTmTimerEvent *) get_first();
   if (lp_event)
      pp_time->set(curr_key());
   else
      pp_time->set(0);
   int lv_count = count(curr_key());
   get_end();

   // Avoiding tracing here for efficiency
   if (lp_event)
   {
      //7/8/10 Temporary code to catch duplicates
      if (lv_count > 1)
      {
         TMTIME_TRACE(5, XATM_TraceTimer, ("CTmTimerList::pop : DUPLICATE KEY DETECTED. count=%d\n", lv_count));
         CTmTimerEvent * lp_ev = (CTmTimerEvent *) get_first();

         for (int lv_idx=0;lv_idx < lv_count; lv_idx++)
         {
             TMTIME_TRACE(5, XATM_TraceTimer, ("CTmTimerList::pop : DUPLICATE KEY DETECTED. %d: key " PFLL ", cmd %d, ID %d\n", 
                 lv_idx, curr_key(), lp_ev->command(), ((lp_ev->transaction())?lp_ev->transaction()->seqnum():-1)));
             lp_ev = (CTmTimerEvent *) get_next();
         }
         get_end();
      }

      CTmTimerEvent *lp_removedEvent = (CTmTimerEvent *) remove_first((pp_time->get()));
      if (lp_event != lp_removedEvent)
      {
          TMTIME_TRACE(1, XATM_TraceError, ("CTmTimerList::pop : PROGRAMMING ERROR! first event %p doesn't match removed event %p.\n",
             (void *) lp_event, (void *) lp_removedEvent));
          abort();
      }
      TMTIME_TRACE(5, XATM_TraceTimer, ("CTmTimerList::pop : EXIT event %p, inputTime %d, returnTime " PFLL ".\n",
         (void *) lp_event, lp_event->wakeupInterval(), pp_time->get()));
   }
   else
   {
      TMTIME_TRACE(5, XATM_TraceTimer, ("CTmTimerList::pop : EXIT Timer list empty, returnTime " PFLL ".\n",
         pp_time->get()));
   }
   return lp_event;
} // CTmTimerList::pop


//----------------------------------------------------------------------------
// CTmTimer methods
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CTmTimer Constructor
// Constructs a CTmTimer object.
//----------------------------------------------------------------------------
CTmTimer::CTmTimer(SB_Thread::Sthr::Function pv_fun, int64 pv_num, const char *pp_name,
                   int64 pv_defaultWaitTime)
   :CTmThread(pv_fun, pv_num, pp_name), iv_state(TmTimer_Up)
{
   TMTIME_TRACE(2, XATM_TraceTimerExit, ("CTmTimer::CTmTimer : ENTRY.\n"));

   iv_startTime = Ctimeval::now();
   defaultWaitTime(pv_defaultWaitTime);
   iv_last_cp_written = 0;
   ip_cp_event = NULL;
   ip_stats_event = NULL;
   ip_RMRetry_event = NULL;
   ip_ShutdownP1_event = NULL;
   start();
   TMTIME_TRACE(2, XATM_TraceTimerExit, ("CTmTimer::CTmTimer : EXIT.\n"));
} //CTmTimer::CTmTimer


//----------------------------------------------------------------------------
// CTmTimer Destructor
//----------------------------------------------------------------------------
CTmTimer::~CTmTimer()
{
   TMTIME_TRACE(2, XATM_TraceTimerExit, ("CTmTimer::~CTmTimer : ENTRY\n"));

   //stop(); //Stop the thread now
}


// --------------------------------------------------------------
// CTmTimer::eventQ_push
// Purpose - push a new event to the timer event queue.
// These are pushed in FIFO order.
// --------------------------------------------------------------
void CTmTimer::eventQ_push(CTmEvent * pp_event)
{
    char        la_buf[DTM_STRING_BUF_SIZE];
    CTmTimerEvent *lp_tevent = (CTmTimerEvent *) pp_event; 

    if (lp_tevent == NULL)
    {
        sprintf(la_buf, "Timer request to be queued is NULL.\n");
        tm_log_event(DTM_TMTIMER_BAD_EVENT, SQ_LOG_CRIT, "DTM_TMTIMER_BAD_EVENT");
        TMTIME_TRACE (1, XATM_TraceTimerExitError,("CTmTimer::eventQ_push - %s", la_buf));
        abort ();
    }

   eventQ()->push(lp_tevent);

   if (lp_tevent->transaction())
   {
      TMTIME_TRACE (3, XATM_TraceTimerExit, ("CTmTimer::eventQ_push : signaling timer thread, event %p, "
               "cmd %d, trans obj %p, request %d, wakeup interval %d, repeat %d.\n",
               (void *) lp_tevent, lp_tevent->command(), 
               (void *) lp_tevent->transaction(), lp_tevent->requestType(),
               lp_tevent->wakeupInterval(),
               lp_tevent->wakeupRepeat()));
   }
   else
   {
      TMTIME_TRACE (3, XATM_TraceTimerExit, ("CTmTimer::eventQ_push : signaling timer thread, event %p, "
               "cmd %d, request %d, wakeup interval %d, repeat %d.\n",
               (void *) lp_tevent, lp_tevent->command(), 
               lp_tevent->requestType(),
               lp_tevent->wakeupInterval(),
               lp_tevent->wakeupRepeat()));
   }
   
   eventQ_CV()->signal(true /*lock*/); 
} //CTmTimer::eventQ_push


// --------------------------------------------------------------
// CTmTimer::eventQ_pop
// Purpose - pop an event from the end of the queue.  Events are
// always processed in FIFO order.
// This method calculates the time until the next timer pops and
// uses that in a timed CV.wait() call to drive timer pop.
// --------------------------------------------------------------
CTmEvent * CTmTimer::eventQ_pop()
{
   CTmTimerEvent *lp_tevent = NULL;
   CTmTime lv_time;
   Ctimeval lv_waitTime;
   bool lv_wait = true;

   TMTIME_TRACE (2, XATM_TraceTimerExit, ("CTmTimer::eventQ_pop ENTRY.\n"));

   lv_wait = calculateWait(&lv_waitTime);

   // If another thread has signaled us, there's an event in the input queue
   // so don't even bother to wait, just grab and process it.
   TMTIME_TRACE (5, XATM_TraceTimer, ("CTmTimer::eventQ_pop wait eventQ()->empty() "
      "= %d, wait %d, wait time %ld:%ld\n", eventQ()->empty(), lv_wait,
      (long) lv_waitTime.tv_sec, (long) lv_waitTime.tv_usec));
   if (eventQ()->empty())
   {
      if (lv_wait)
         eventQ_CV()->wait(true /*lock*/, lv_waitTime.tv_sec, lv_waitTime.tv_usec);
      else
         eventQ_CV()->wait(true /*lock*/); //Wait forever
   }
   else
   {
      // Clear the CV so it doesn't trigger the next time pop is called.
      TMTIME_TRACE (5, XATM_TraceTimer, ("CTmTimer::eventQ_pop reseting CV.\n"));
      eventQ_CV()->reset_flag();
   }

   if (!eventQ()->empty())
   {
      // New event arrived
      TMTIME_TRACE(5, XATM_TraceTimer, ("CTmTimer::eventQ_pop new event\n"));
      lp_tevent = (CTmTimerEvent *) eventQ()->pop_end();
   }
   else
   {
      // Timer pop
      TMTIME_TRACE(5, XATM_TraceTimer, ("CTmTimer::eventQ_pop timer pop\n"));
      lp_tevent = timerList()->pop(&lv_time);
   }

   TMTIME_TRACE (2, XATM_TraceTimerExit, ("CTmTimer::eventQ_pop EXIT : Returning event %p, "
            "cmd %d, trans obj %p, request %d.\n",
            (void *) lp_tevent, 
            ((lp_tevent)?lp_tevent->command():0), 
            (void *) ((lp_tevent)?lp_tevent->transaction():0), 
            ((lp_tevent)?lp_tevent->requestType():0)));
   return lp_tevent;
} // CTmTimer::eventQ_pop


// --------------------------------------------------------------
// CTmTimer::calculateWait
// Purpose : Calculate how long eventQ_pop should wait for a CV.
// This is driven either by the first event on the timerList.
// --------------------------------------------------------------
bool CTmTimer::calculateWait(Ctimeval *pp_waitTime)
{
   bool lv_wait = true;
   CTmTime lv_time;
   Ctimeval lv_waitTime;

   lv_time = timerList()->getFirst();
   if (lv_time.get() == 0)
   {
      *pp_waitTime = iv_defaultWaitTime;
      if (pp_waitTime->tv_sec == 0 && pp_waitTime->tv_usec == 0)
         lv_wait = false;
   }
   else
      *pp_waitTime = lv_time.left_tv();

   TMTIME_TRACE (5, XATM_TraceTimer, ("CTmTimer::calculateWait : returning %d, "
      "wait time %ld:%ld\n", lv_wait,
      (long) pp_waitTime->tv_sec, (long) pp_waitTime->tv_usec));

   return lv_wait;
} // CTmTimer::calculateWait


// --------------------------------------------------------------
// CTmTimer::cancelEvent
// Purpose : Push a cancel event to the tmTimer event queue.
// --------------------------------------------------------------
void CTmTimer::cancelEvent(CTmTimerEvent *pp_tevent)
{
   TMTIME_TRACE (2, XATM_TraceTimerExit, ("CTmTimer::cancelEvent : ENTRY, timer event %p\n", 
      (void *) pp_tevent));

   pp_tevent->command(TmTimerCmd_Cancelled);
} //CTmTimer::cancelEvent

// --------------------------------------------------------------
// CTmTimer::addControlpointEvent
// Purpose : Add a Controlpoint Event to drive control pointing.
// Only the Lead TM must make this call!
// pv_cp_interval is the interval between control points in msecs.
// --------------------------------------------------------------
void CTmTimer::addControlpointEvent(int32 pv_cp_interval)
{
   TMTIME_TRACE (2, XATM_TraceTimerExit, ("CTmTimer::addControlpointEvent : ENTRY, cp interval %d\n",
            pv_cp_interval));

   // Add CP Timer event.
   // This is always processed by the timer thread, so no need to specify thread or tranasction.
   // Repeat it forever.
   ip_cp_event = new CTmTimerEvent(TM_MSG_TXINTERNAL_CONTROLPOINT, NULL, 
                                   pv_cp_interval, -1);
   eventQ_push((CTmEvent *) ip_cp_event);
} //CTmTimer::addControlpointEvent


// --------------------------------------------------------------
// CTmTimer::cancelControlpointEvent
// Purpose : Cancel the Controlpoint event.  This should only be
// called in the Lead TM and only when changing the control point
// interval.
// --------------------------------------------------------------
void CTmTimer::cancelControlpointEvent()
{
   TMTIME_TRACE (2, XATM_TraceTimerExit, ("CTmTimer::cancelControlpointEvent : ENTRY, timer event %p\n", 
      (void *) ip_cp_event));

   if (ip_cp_event != NULL)
      cancelEvent(ip_cp_event);
} //CTmTimer::cancelControlpointEvent


// --------------------------------------------------------------
// CTmTimer::addStatsEvent
// Purpose : Add a Stats Event to drive statistics gathering
// pv_interval is the interval between event executions in msecs.
// --------------------------------------------------------------
void CTmTimer::addStatsEvent(int32 pv_interval)
{
   TMTIME_TRACE (2, XATM_TraceTimerExit, ("CTmTimer::StatsEvent : ENTRY, interval %d\n",
            pv_interval));

   // Add Timer event.
   // This is always processed by the timer thread, so no need to specify thread or tranasction.
   // Repeat it forever.
   ip_stats_event = new CTmTimerEvent(TM_MSG_TXINTERNAL_STATS, NULL, 
                                      pv_interval, -1);
   eventQ_push((CTmEvent *) ip_stats_event);
} //CTmTimer::addStatsEvent


// --------------------------------------------------------------
// CTmTimer::cancelStatsEvent
// Purpose : Cancel the Stats Event
// --------------------------------------------------------------
void CTmTimer::cancelStatsEvent()
{
   TMTIME_TRACE (2, XATM_TraceTimerExit, ("CTmTimer::cancelStatsEvent : ENTRY, timer event %p\n", 
      (void *) ip_stats_event));

   if (ip_stats_event != NULL)
      cancelEvent(ip_stats_event);
} //CTmTimer::cancelStatsEvent




// --------------------------------------------------------------
// CTmTimer::addRMRetryEvent
// Purpose : Add a RMRetry Event to drive ???.
// pv_interval is the interval between event executions in msecs.
// --------------------------------------------------------------
void CTmTimer::addRMRetryEvent(int32 pv_interval)
{
   TMTIME_TRACE (2, XATM_TraceTimerExit, ("CTmTimer::RMRetryEvent : ENTRY, interval %d\n",
            pv_interval));

   // Add Timer event.
   // This is always processed by the timer thread, so no need to specify thread or tranasction.
   // Repeat it forever.
   ip_RMRetry_event = new CTmTimerEvent(TM_MSG_TXINTERNAL_RMRETRY, NULL, 
                                      pv_interval, -1);
   eventQ_push((CTmEvent *) ip_RMRetry_event);
} //CTmTimer::addRMRetryEvent


// --------------------------------------------------------------
// CTmTimer::cancelRMRetryEvent
// Purpose : Cancel the RMRetry Event
// --------------------------------------------------------------
void CTmTimer::cancelRMRetryEvent()
{
   TMTIME_TRACE (2, XATM_TraceTimerExit, ("CTmTimer::cancelRMRetryEvent : ENTRY, timer event %p\n", 
      (void *) ip_RMRetry_event));

   if (ip_RMRetry_event != NULL)
      cancelEvent(ip_RMRetry_event);
} //CTmTimer::cancelRMRetryEvent
