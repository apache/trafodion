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

#include "tminfo.h"
#include "seabed/trace.h"
#include "tmlogging.h"
#include "tmtimer.h"

extern void HbaseTM_initiate_cp();

// -------------------------------------------------------------
// tmTimer_initiate_cp
// -------------------------------------------------------------
void tmTimer_initiate_cp ()
{
    if (gv_tm_info.state() != TM_STATE_UP && 
        gv_tm_info.state() != TM_STATE_TX_DISABLED &&
        gv_tm_info.state() != TM_STATE_QUIESCE &&
        gv_tm_info.state() != TM_STATE_DRAIN)
    {
        TMTrace (2, ("tmTimer_inititate_cp: EXIT - TM state %d disallows cp.\n",
            gv_tm_info.state()));
        return;
    }
 
   TMTrace(2, ("tmTimer_initiate_cp : ENTRY.\n"));
   
   // This will send control point requests to the other TMs
   if (gv_tm_info.lead_tm()) {
     TMTrace(2, ("tmTimer_initiate_cp : Calling write_control_point \n"));
     gv_tm_info.write_control_point(true);
     gv_tm_info.write_control_point(true, true);
   }

   if (gv_tm_info.use_tlog()) {

       // Initiate HBase TM Control Point
       HbaseTM_initiate_cp();

        // write our records
       gv_tm_info.write_all_trans_state();

       gv_system_tx_count = 0;
   }
   TMTrace(2, ("tmTimer_initiate_cp : EXIT.\n"));
} //tmTimer_initiate_cp


// -------------------------------------------------------------
// tmTimer_stats
// Purpose : wake up at specified interval.
// -------------------------------------------------------------
void tmTimer_stats()
{
   TMTrace(2, ("tmTimer_stats : ENTRY (writing tx_stats - noop in Trafodion).\n"));
}


// -------------------------------------------------------------
// tmTimer_RMRetry
// Purpose : Retry RM opens for any failed RMs.
// -------------------------------------------------------------
void tmTimer_RMRetry()
{
    if (gv_tm_info.state() != TM_STATE_UP && 
        gv_tm_info.state() != TM_STATE_TX_DISABLED &&
        gv_tm_info.state() != TM_STATE_DRAIN)
    {
        TMTrace (2, ("TM_Info::tmTimer_RMRetry: EXIT - TM state %d is not in up or tx "
            "disabled state.\n", gv_tm_info.state()));
        return;
    }
   int32 lv_failedRMs = gv_RMs.num_rm_failed();
   TMTrace(2, ("tmTimer_RMRetry : ENTRY found %d failed RMs.\n", lv_failedRMs));
   if (lv_failedRMs > 0)
      gv_tm_info.CheckFailed_RMs();
   TMTrace(2, ("tmTimer_RMRetry : EXIT.\n"));
}


// -------------------------------------------------------------
// tmTimer_RecoveryWait
// Purpose : Wait for transaction count to be 0 before continuing
// -------------------------------------------------------------
void tmTimer_RecoveryWait(CTmTimerEvent * pp_event)
{

   int32 lv_nid = pp_event->request()->u.iv_tmrecovery_internal.iv_nid;
   
   if (lv_nid == -1) //Cluster recovery
   {
       int32 lv_activeTxns = gv_tm_info.transactionPool()->get_inUseList()->size();
       int32 lv_queuedTxns = gv_tm_info.ClusterRecov()->txnStateList()->size();

       TMTrace(2, ("tmTimer_RecoveryWait : Cluster ENTRY. Indoubt queued: %d, in progress %d.\n",
               lv_queuedTxns, lv_activeTxns));

       //Check the number of in doubt transactions
       if (lv_activeTxns + lv_queuedTxns > 0 ) 
       {
          int32 lv_availableTxnObjs = 
                MIN(gv_tm_info.transactionPool()->get_maxPoolSize(), gv_tm_info.maxRecoveringTxns())  - lv_activeTxns;
          if (lv_availableTxnObjs > 0)
          {
              gv_tm_info.ClusterRecov()->queueTxnObjects();
              gv_tm_info.ClusterRecov()->resolve_in_doubt_txs(-1/*all tms*/, false/*no delay*/);
          }
          else
              gv_tm_info.addTMRecoveryWait(lv_nid, 1000 /*1 sec*/);
       }
       else 
       {
          gv_tm_info.ClusterRecov()->completeRecovery();
          delete gv_tm_info.ClusterRecov();
          gv_tm_info.ClusterRecov(NULL);
          tm_log_event(DTM_RECOVERY_COMPLETED, SQ_LOG_NOTICE, "DTM_RECOVERY_COMPLETED", 
                       -1, -1, gv_tm_info.nid());
          TMTrace(1, ("System recover : DTM%d System startup recovery completed.\n", gv_tm_info.nid()));
       }
    }
    else 
    {
        int32 lv_queuedTxns = gv_tm_info.NodeRecov(lv_nid)->txnStateList()->size();

        TMTrace(2, ("tmTimer_RecoveryWait : Node ENTRY. Node %d has indoubt queued: %d.\n",
                lv_nid, lv_queuedTxns));

        //Check the number of in doubt transactions
        if (lv_queuedTxns > 0 )
        {
           int32 lv_availableTxnObjs =
                  gv_tm_info.transactionPool()->get_maxPoolSize() - gv_tm_info.num_active_txs();
           if (lv_availableTxnObjs)
           {
               gv_tm_info.NodeRecov(lv_nid)->queueTxnObjects();
               gv_tm_info.NodeRecov(lv_nid)->resolve_in_doubt_txs(lv_nid, 1000 /*1 sec*/);
           }
           else
               gv_tm_info.addTMRecoveryWait(lv_nid, 1000 /*1 sec*/);
        }
        else 
        {
           gv_tm_info.NodeRecov(lv_nid)->update_registry_txs_to_recover(0);
        }

     }

     TMTrace(2, ("tmTimer_RecoveryWait : EXIT.\n"));
}


// -------------------------------------------------------------
// tmTimer_initializeRMs
// Purpose : Initiailze the RMs.  This sends xa_open to all
// TSEs.
// -------------------------------------------------------------
void tmTimer_initializeRMs()
{
   TMTrace(2, ("tmTimer_initializeRMs : ENTRY.\n"));
   
   gv_tm_info.init_and_recover_rms();

   TMTrace(2, ("tmTimer_initializeRMs : EXIT.\n"));
} //tmTimer_initialize_RMs


// -------------------------------------------------------------
// tmTimer_recoverSystem
// Purpose : Runs system recovery under the timer thread.
// -------------------------------------------------------------
void tmTimer_recoverSystem()
{
   TMTrace(2, ("tmTimer_recoverSystem : ENTRY.\n"));
   
   gv_tm_info.ClusterRecov()->recover_system();

   TMTrace(2, ("tmTimer_recoverSystem : EXIT.\n"));
} //tmTimer_initialize_RMs


// -------------------------------------------------------------
// tmTimer_TMRestartRetry
// Purpose : Retry TM Restart for TMs which don't start on the
// first try.  This is only present in Lead TMs, and only when
// a TM is down and we need to retry the open.
// -------------------------------------------------------------
void tmTimer_TMRestartRetry(CTmTimerEvent * pp_event)
{
    int32 lv_error = FEOK;
    int32 lv_nid = pp_event->request()->u.iv_tmrestart_internal.iv_nid;
    TMTrace(2, ("tmTimer_TMRestartRetry : ENTRY Attempting to restart "
            "$TM%d.\n", lv_nid));

    if (gv_tm_info.state() != TM_STATE_UP && 
        gv_tm_info.state() != TM_STATE_TX_DISABLED &&
        gv_tm_info.state() != TM_STATE_DRAIN)
    {
        TMTrace(1, ("TM_Info::tmTimer_TMRestartRetry: EXIT - Too late! "
                "Lead TM not in up or tx disabled state.\n"));
        return;
    }
    lv_error = gv_tm_info.recover_tm(lv_nid);

    TMTrace(2, ("tmTimer_TMRestartRetry : EXIT, error %d for nid %d.\n", 
            lv_error, lv_nid));
}


//----------------------------------------------------------------------------
// timerThread_main
// Purpose : Main for timer thread
//----------------------------------------------------------------------------
void * timerThread_main(void *arg)
{
   CTmTimerEvent   *lp_event;
   CTmTimer        *lp_timerTh;
   bool             lv_exit = false;
   bool                lv_deleteEvent = false;

   arg = arg;
   CTmTxMessage *lp_msg;

   TMTrace(2, ("timerThread_main : ENTRY.\n"));

   while (gv_tm_info.tmTimer() == NULL || gv_tm_info.tmTimer()->state() != TmTimer_Up || gv_tm_info.tmAuditObj() == NULL)
   {
      SB_Thread::Sthr::usleep(10);
   }
   // Now we can set a pointer to the CTmTimer object because it exits
   lp_timerTh = gv_tm_info.tmTimer();

   TMTrace(2, ("timerThread_main : Thread %s(%p) State Up.\n",
      lp_timerTh->get_name(), (void *) lp_timerTh));

   while (!lv_exit)
   {
      lp_event = (CTmTimerEvent *) lp_timerTh->eventQ_pop();

      if (lp_event)
      {
         switch (lp_event->command())
         {
         case TmTimerCmd_Queue:
         {
            // Queue request to timer list
            TMTrace(3, ("timerThread_main : TmTimerCmd_Queue: Queue timer event. "
               "Txn ID %d, Wakeup interval %d, repeat %d, Request %d\n",
               ((lp_event->transaction())?lp_event->transaction()->seqnum():0),
               lp_event->wakeupInterval(), lp_event->wakeupRepeat(),
               lp_event->requestType()));
            lp_event->command(TmTimerCmd_Timer); //Change to timer event
            lp_timerTh->timerList()->add(lp_event);
            break;
         } //case TmTimerCmd_Queue Incoming request

         case TmTimerCmd_Timer:
         {
            // A timer popped, queue request to the transactions event queue
            TMTrace(3, ("timerThread_main : TmTimerCmd_Timer: Timer pop. "
               "Txn ID %d, Wakeup interval %d, repeat %d, Request %d, msgid %d\n",
               ((lp_event->transaction())?lp_event->transaction()->seqnum():0),
               lp_event->wakeupInterval(), lp_event->wakeupRepeat(),
               lp_event->requestType(), lp_event->msg()->msgid()));
            lp_timerTh->lock();

            switch (lp_event->requestType())
            {
            case TM_MSG_TXINTERNAL_CONTROLPOINT:
               // Control Point (must be Lead TM).
               lp_timerTh->last_cp_written(SB_Thread::Sthr::time());
               tmTimer_initiate_cp();
               break;
            case TM_MSG_TXINTERNAL_STATS:
               tmTimer_stats();
               break;
            case TM_MSG_TXINTERNAL_RMRETRY:
               break;
            case TM_MSG_TXINTERNAL_TMRESTART_RETRY:
               // If this is the first and only event or the last of several - then service it.  If 
               // it is not the last one (of duplicate events), then ignore it. 
               if (gv_tm_info.get_restartTimerEvent(lp_event->request()->u.iv_tmrestart_internal.iv_nid)
                        == lp_event)
               {
                   gv_tm_info.reset_restartTimerEvent (lp_event->request()->u.iv_tmrestart_internal.iv_nid);
                   tmTimer_TMRestartRetry(lp_event);
               }
               lv_deleteEvent = true;
               break;
            case TM_MSG_TXINTERNAL_SHUTDOWNP1_WAIT:
               lp_msg = (CTmTxMessage *) lp_event;
               gv_tm_info.ShutdownPhase1Wait(lp_msg);
               break;
            case TM_MSG_TYPE_ATTACHRM:
               lp_msg = new CTmTxMessage(lp_event->request());
               gv_tm_info.attachRm(lp_msg);
               lv_deleteEvent = true;
               delete lp_msg;
               break;
            case TM_MSG_TYPE_ENABLETRANS:
               lp_msg = new CTmTxMessage(lp_event->request());
               gv_tm_info.enableTrans(lp_msg);
               lv_deleteEvent = true;
               delete lp_msg;
               break;
            case TM_MSG_TYPE_DISABLETRANS:
               lp_msg = new CTmTxMessage(lp_event->request());
               gv_tm_info.disableTrans(lp_msg);
               lv_deleteEvent = true;
               delete lp_msg;
               break;
            case TM_MSG_TXINTERNAL_RECOVERY_WAIT:
               tmTimer_RecoveryWait(lp_event);
               lv_deleteEvent = true;
               break;
            case TM_MSG_TXINTERNAL_INITIALIZE_RMS:
               tmTimer_initializeRMs();
               lv_deleteEvent = true;
               break;
            case TM_MSG_TXINTERNAL_SYSTEM_RECOVERY:
               tmTimer_recoverSystem();
               lv_deleteEvent = true;
               break;
            default:
            {
                if (lp_event->transaction())
                {
                   // Notify transaction object
                   CTmTxMessage *lp_msg = new CTmTxMessage(lp_event->request());
                   lp_event->transaction()->queueToTransaction(lp_event->transaction()->transid(), 
                                                               lp_msg);
                   lv_deleteEvent = true;
                }
                else if (lp_event->thread())
                   // Notify non-transactional thread
                   lp_event->thread()->eventQ_push((CTmEvent *) lp_event);
                else
                {
                   TMTrace(1, ("timerThread_main : Timer Thread %p. No transaction or "
                            "thread pointer for event %p, Txn ptr %p, cmd %d, reqType %d, Wakeup interval %d, repeat %d.\n",
                            (void *) lp_timerTh, (void *) lp_event, (void *) lp_event->transaction(), lp_event->command(), 
                            lp_event->requestType(), lp_event->wakeupInterval(), lp_event->wakeupRepeat()));
                   tm_log_event (DTM_TMTIMER_BAD_EVENT2, SQ_LOG_WARNING, "DTM_TMTIMER_BAD_EVENT2",
                       lp_event->request()->iv_msg_hdr.miv_err.error, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                       lp_event->requestType(), lp_event->request()->iv_msg_hdr.dialect_type,
                       -1, NULL, -1, lp_event->wakeupInterval(), lp_event->wakeupRepeat());
                   // abort(); Don't want to abort, just discard event.
                   lv_deleteEvent = true;
                }
            } //default
            } //switch 

            if (lp_event->wakeupRepeat() != 0 && lv_deleteEvent == false)
            {
               lp_event->dec_wakeupRepeat();
               lp_timerTh->timerList()->add(lp_event);
            }
            lp_timerTh->unlock();
            break;
         } //case TmTimerCmd_Timer timer pop

         case TmTimerCmd_Stop:
         {
            lp_timerTh->lock();
            lv_exit = true;
            TMTrace(1, ("timerThread_main : Stop thread received.\n"));
            lv_deleteEvent = true;
            lp_timerTh->state(TmTimer_Down);
            // Unlock on exit
            break;
         } //case TmTimerCmd_Stop the timer thread

         case TmTimerCmd_Cancelled:
         {
            TMTrace(1, ("timerThread_main : Cancelled timer event %p popped.\n",
               (void *) lp_event));
            lv_deleteEvent = true;
            break;
         } //case TmTimerCmd_Cancelled

         default:
         {
            // EMS DTM_TXTHREAD_BAD_EVENT
            TMTrace(1, ("timerThread_main: Timer Thread (0x%p) main received an "
                "unexpected event %p, reqType %d, cmd %d, terminating.\n", 
                (void *) lp_timerTh, (void *) lp_event, lp_event->requestType(), lp_event->command()));
            tm_log_event(DTM_TMTIMER_UNEXP_EVENT, SQ_LOG_CRIT, "DTM_TMTIMER_UNEXP_EVENT",
                         -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,lp_event->requestType(),lp_event->command());
            abort();
         }
         } //switch
         if (lv_deleteEvent)
         {
            TMTrace(2, ("timerThread_main : thread_deleted p_event (0x%p), cmd %d, type %d.\n", 
                    (void *) lp_event, lp_event->command(), lp_event->requestType()));
            delete lp_event;
            lv_deleteEvent = false;
         }
      } // if lp_event
   } //while

   TMTrace(2, ("timerThread_main : EXIT.\n"));

   lp_timerTh->unlock();
   return NULL;
} //timerThread_main
