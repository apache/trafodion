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
#include "tmthreadeg.h"
#include "tmglob.h"

extern CTmThreadExample  *gp_tmExampleThread;

extern CTmTimerEvent * tm_addTimerEvent(TM_MSG_TYPE pv_type, CTmThread * pp_thread, int pv_delayInterval, int pv_repeatCount);
extern void tm_stopTimerEvent(CTmTimerEvent * pp_timerEvent);


//----------------------------------------------------------------------------
// CTmThreadExample methods
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CTmThreadExample Constructor
// Constructs a CTmThreadExample object.
// Add whatever initialization is required here.
//----------------------------------------------------------------------------
CTmThreadExample::CTmThreadExample(SB_Thread::Sthr::Function pv_fun, int64 pv_num, const char *pp_name)
   :CTmThread(pv_fun, pv_num, pp_name), iv_stopped(false), iv_count(0)
{
   TMTrace(2, ("CTmThreadExample::CTmThreadExample : ENTRY.\n"));
  
   start(); //Start the example thread

   TMTrace(2, ("CTmThreadExample::CTmThreadExample : EXIT.\n"));
} //CTmThreadExample::CTmThreadExample


//----------------------------------------------------------------------------
// CTmThreadExample Destructor
//----------------------------------------------------------------------------
CTmThreadExample::~CTmThreadExample()
{
   TMTrace(2, ("CTmThreadExample::~CTmThreadExample : EXIT\n"));
}


// --------------------------------------------------------------
// CTmThreadExample::eventQ_push
// Purpose - push a new event to the threads event queue.
// These are pushed in FIFO order.
// This method is called by other threads to queue an event to
// this thread.
// --------------------------------------------------------------
void CTmThreadExample::eventQ_push(CTmEvent * pp_event)
{
    CTmEvent *lp_event = (CTmEvent *) pp_event; 

    if (lp_event == NULL)
    {
        tm_log_event(DTM_TMTIMER_BAD_EVENT, SQ_LOG_CRIT, "DTM_TMTIMER_BAD_EVENT");
        TMTrace(1, ("CTmThreadExample::eventQ_push - ThreadExample request to be queued is NULL\n"));
        abort ();
    }

   eventQ()->push(lp_event);

   TMTrace(2, ("CTmThreadExample::eventQ_push : signaling example thread, event %p, "
            "request %d.\n",
            (void *) lp_event,
            lp_event->requestType()));
   
   eventQ_CV()->signal(true /*lock*/); 
} //CTmThreadExample::eventQ_push


// --------------------------------------------------------------
// CTmThreadExample::eventQ_pop
// Purpose - pop an event from the end of the queue.  Events are
// always processed in FIFO order.
// This method is specific to the threads implementation, so this
// serves only as an example.
// --------------------------------------------------------------
CTmEvent * CTmThreadExample::eventQ_pop()
{
   CTmEvent *lp_event = NULL;

   TMTrace(2, ("CTmThreadExample::eventQ_pop ENTRY.\n"));

   // Wait forever for a signal from eventQ_push
   eventQ_CV()->wait(true /*lock*/);

   if (!eventQ()->empty())
   {
      // New event arrived
      lp_event = (CTmEvent *) eventQ()->pop_end();
   }

   TMTrace(2, ("CTmThreadExample::eventQ_pop EXIT : Returning event %p, "
            "request %d.\n",
            (void *) lp_event, 
            ((lp_event)?lp_event->requestType():0)));
   return lp_event;
} // CTmThreadExample::eventQ_pop


//----------------------------------------------------------------------------
// exampleThread_main
// Purpose : Main for example thread
//----------------------------------------------------------------------------
void * exampleThread_main(void *arg)
{
   //char              la_buf[DTM_STRING_BUF_SIZE];
   CTmTimerEvent    *lp_event;
   CTmThreadExample *lp_exampleTh;
   bool              lv_exit = false;

   arg = arg;

   TMTrace(2, ("exampleThread_main : ENTRY.\n"));

   // The method for waiting for the thread object to be present varies depending
   // on the implementation.
   // See CTmTimerMain.cpp and CTmTxThread.cpp for examples.
   // Here we just have a basic wait.

   SB_Thread::Sthr::usleep(100);

   // Now we should be able to set a pointer to the CTmThreadExample object because it exits
   // I've just used a global here for simplicity.
   lp_exampleTh = gp_tmExampleThread;

   if (!lp_exampleTh)
      abort();
   TMTrace(2, ("exampleThread_main : Thread %s(%p) State Up.\n",
      lp_exampleTh->get_name(), (void *) lp_exampleTh));

   // Add a timer event
   lp_exampleTh->ip_tevent = tm_addTimerEvent(TM_MSG_TXTHREAD_INITIALIZE, lp_exampleTh, 100, -1);

   while (!lv_exit)
   {
      lp_event = (CTmTimerEvent *) lp_exampleTh->eventQ_pop();

      if (lp_event)
      {
         TMTrace(3, ("exampleThread_main : event received. iv_count %d, iv_stopped %d\n",
            lp_exampleTh->iv_count, lp_exampleTh->iv_stopped));
         // Expecting the timer event we created.
         if (lp_event != lp_exampleTh->ip_tevent || lp_exampleTh->iv_stopped)
            abort();
         // Allow it to run for 100 signals then stop
         if (lp_exampleTh->iv_count++ == 100)
         {
            TMTrace(3, ("exampleThread_main : Stopping timer event. iv_count %d, iv_stopped %d\n",
               lp_exampleTh->iv_count, lp_exampleTh->iv_stopped));
            tm_stopTimerEvent(lp_exampleTh->ip_tevent);
            lp_exampleTh->iv_stopped = true;
         }
      }
   } //while

   // Thread terminating, delete the event which drove termination
   if (lp_event)
      delete lp_event;

   TMTrace(2, ("exampleThread_main : EXIT.\n"));

   lp_exampleTh->stop();
   return NULL;
} //exampleThread_main
