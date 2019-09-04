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
// tmtxbase is the base class for transaction objects.  This includes
// both transactions originating in and owned by DTM and XARM 
// subordinate transaction branches.
// TM_TX_Info and CTmXaTxn are both derived from this class.

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/ms.h"
#include "rm.h"
#include "xatmmsg.h"
#include "xatmlib.h"
#include "seabed/trace.h"
#include "tmlogging.h"
#include "tmtxbase.h"
#include "tminfo.h"

// -------------------------------------
// CTmTxBase Methods
// -------------------------------------

// -------------------------------------
// CTmTxBase constructor
// Purpose : calls initialize
// -------------------------------------
CTmTxBase::CTmTxBase(int32 pv_nid, int64 pv_flags, int32 pv_trace_level, 
                       int32 pv_seq, int32 pv_pid, int32 pv_rm_wait_time)
                       //: CTmPoolElement()
{
   //EID(EID_CTmTxBase);
   // Initialize iv_trace_level and iv_lock_count here because they're used in calls 
   // to lock().
   iv_trace_level = pv_trace_level;
   // Mutex attributes: Recursive = true, ErrorCheck=false
   ip_mutex = new TM_Mutex(true, false);
   iv_ender_nid = pv_nid; //TMs nid is the default for the ender nid.
   ip_timeoutEvent = NULL;
   ip_hung_event = NULL;
   ip_Thread = NULL;
   iv_threadPending = false;
   ip_Qlock_owner = NULL;
   ip_currRequest = NULL;

   memset(&iv_transid, 0, sizeof(TM_Txid_Internal));
   iv_tag = 0;
   iv_tx_state = TM_TX_STATE_NOTX;
   iv_ender_pid = 0;
   iv_num_active_partic.set_val(0);
   iv_prepared_rms = 0;
   iv_cleanup_sem = 0;
   iv_rm_wait_time = 0;
   iv_in_use = false;
   iv_incremented = false;
   iv_mark_for_rollback = false;
   iv_tm_aborted = false;
   iv_tse_aborted = false;
   iv_appl_aborted = false;
   iv_heur_aborted = false;
   iv_read_only = false;
   iv_use_ext_transid = false;
   iv_written_in_cp = false;
   iv_wrote_trans_state = false;
   iv_recovering = false;
   iv_transactionBusy = false;
   iv_timeout = 0;
   ip_branches = NULL;
   
   // Initialize separately from TM_Info::new_tx as we don't have the creator nid,pid when called by
   // the tmPool constructor.
   // this->initialize(pv_nid, pv_flags, pv_trace_level, pv_seq, pv_pid, pv_rm_wait_time);
}

// -------------------------------------
// CTmTxBase destructor
// Purpose : 
// -------------------------------------
CTmTxBase::~CTmTxBase()
{
   TMTrace (2, ("CTmTxBase::~CTmTxBase : ENTRY.\n"));
}


//----------------------------------------------------------------------------
// CTmTxBase::constructPoolElement
// Purpose : Callback for CTmPool elements.
// This method must be called by the implementation class!
// Because the function is static we must work out here
// What the derived class is and call it's constructPoolElement
// function directly.
//----------------------------------------------------------------------------
CTmTxBase * CTmTxBase::constructPoolElement(int64 pv_id)
{
   CTmTxBase *lp_txn = NULL;
   CTmTxKey k(pv_id);
   TMTrace (2, ("CTmTxBase::constructPoolElement ENTRY: Constructing transaction "
            "pool element for Txn ID (%d,%d).\n", 
            k.node(), k.seqnum()));
   abort();
   /*
   switch (iv_txnType)
   {
      case TM_TX_TYPE_DTM:
         lp_txn = (CTmTxBase *) ::TM_TX_Info.constructPoolElement(pv_id);
         break;
      case TM_TX_TYPE_XARM:
         lp_txn = (CTmTxBase *) ::CTmXaTxn.constructPoolElement(pv_id);
         break;
      default:
         TMTrace (2, ("CTmTxBase::constructPoolElement ENTRY: Constructing transaction "
                  "pool element of type %d for Txn ID (%d,%d).\n", 
                  iv_txnType, k.node(), k.seqnum()));
         abort();
   } */

   TMTrace (2, ("CTmTxBase::constructPoolElement EXIT: Transaction "
            "pool element %p constructed.\n", lp_txn));
   return lp_txn;
} //constructPoolElement


//----------------------------------------------------------------------------
// CTmTxBase::cleanPoolElement
// Purpose : Callback for CTmPool elements.
// This method is called to clean a CTmTxBase object when CTmPool::newElement
// allocated it from the freeList.
// Returns the threadNum which is reused to add the thread object to the 
// CTmPool<CTmTxBase> inUseList.
//----------------------------------------------------------------------------
int64 CTmTxBase::cleanPoolElement()
{
   CTmTxKey k(node(), seqnum());
   cleanup();
   return k.id();
} //CTmTxBase::cleanPoolElement


// ---------------------------------------------------------
// cleanup
// ---------------------------------------------------------
void CTmTxBase::cleanup()
{
   TMTrace(2,("CTmTxBase::cleanup BASE CLASS!\n"));
   abort();
}


// ---------------------------------------------------------
// get_rm
// Purpose :  Get the address of the ia_TSEBranches entry for the
// specified rmid.  
// Note that this is a private function, and protected by
// the caller.
// ---------------------------------------------------------
RM_Info_TSEBranch * CTmTxBase::get_rm(int32 pv_rmid)
{
   return branches()->TSE()->return_slot(pv_rmid);
}

// -------------------------------------
// initialize
// Purpose : Initialize this instance
// -------------------------------------
void CTmTxBase::initialize(int32 pv_nid, int64 pv_flags, int32 pv_trace_level, 
                            int32 pv_seq, int32 pv_creator_nid, int32 pv_creator_pid, 
                            int32 pv_rm_wait_time)
{
   TMTrace(2, ("CTmTxBase::initialize : ENTRY ID (%d,%d).\n",pv_nid, pv_seq));

   // transid setup
   lock();
   iv_txnType = TM_TX_TYPE_DTM; // default to standard DTM owned txn
   iv_incremented = false;
   iv_transid.iv_seq_num = pv_seq;
   ip_Thread = NULL;
   iv_threadPending = false;
   iv_transid.iv_tx_flags = (short)pv_flags; //HERE
   iv_transid.iv_node = pv_nid;
   iv_transid.iv_version = 1;
   iv_transid.iv_check_sum = 403;
   iv_transid.iv_incarnation_num = gv_tm_info.incarnation_num(); 
   iv_transid.iv_tt_flags.Application = 0;
   iv_transid.iv_tt_flags.Reserved[0] = 0;
   iv_transid.iv_tt_flags.Reserved[1] = 0;
   iv_transid.iv_tt_flags.Predefined = 0;
   iv_transid.iv_timestamp = SB_Thread::Sthr::time();

   // misc setup
   iv_timeout = gv_tm_info.timeout(); //default timeout
   ip_timeoutEvent = NULL;
   iv_tm_aborted = iv_tse_aborted = iv_appl_aborted = iv_heur_aborted = iv_written_in_cp = false;
   iv_wrote_trans_state = iv_mark_for_rollback = false;
   iv_recovering = false;
   iv_read_only = false;
   iv_in_use = true;
   iv_prepared_rms = 0;
   iv_trace_level = pv_trace_level;
   iv_tx_state = TM_TX_STATE_NOTX;
   iv_tag = pv_seq;
   iv_num_active_partic.set_val(1);
   iv_ender_nid = pv_creator_nid;
   iv_ender_pid = pv_creator_pid;
   ip_currRequest = NULL;
   iv_transactionBusy = false;
   iv_cleanup_sem = 0;
   iv_rm_wait_time = pv_rm_wait_time;
   ip_hung_event = NULL;
   add_app_partic(iv_ender_pid, iv_ender_nid);

   iv_stats.initialize(gv_tm_info.stats()->collectStats(), 
                       gv_tm_info.stats()->collectInterval());
   iv_stats.clearCounters();

   unlock();

}

// -----------------------------------------------------------------
// safe_initialize_slot
// purpose : this method provides a safe version of initialize_slot, to
// be called by external users. It acquires the lock, calls initialize_slot,
// releases the lock, and returns the value to the caller. 
// -----------------------------------------------------------------
int CTmTxBase::safe_initialize_slot (int32 pv_rmid)
{
   int lv_idx = 0;
   lock();
   lv_idx = initialize_slot (pv_rmid);
   unlock();
   return lv_idx;
}
 
// -----------------------------------------------------------------
// initialize_slot
// purpose : this method is used to set an RM's partic flag to true
//           so that it can partic in the transaction.
// Currently only called by register_branches (ax_reg), so we assume
// that the RM is up and set it's flags appropriately
// register_branches or other caller MUST have already acquired the lock.
// -----------------------------------------------------------------
int CTmTxBase::initialize_slot (int32 pv_rmid)
{

    return branches()->TSE()->set_partic_and_transid(iv_transid, pv_rmid);

}

// ----------------------------------------------------------------
// initialize_tx_rms
// Purpose : initialize the group of standard RMs for any
//           given transaction.  This is done at the start
//           of every transaction
// ----------------------------------------------------------------
void CTmTxBase::initialize_tx_rms(bool pv_partic_true)
{
   lock();
   if (branches() == NULL)
   {
      TMTrace(1, ("CTmTxBase::initialize_tx_rms ERROR : ip_branches not instantiated!\n"));
      tm_log_event(DTM_TXN_INIT_RMS_NO_BRANCH_OBJ, SQ_LOG_CRIT, "DTM_TXN_INIT_RMS_NO_BRANCH_OBJ",
                   -1,-1,node(),seqnum());
      abort();
   }
   branches()->init_rms(this, pv_partic_true);
   unlock();
}


// --------------------------------------------------------------
// sync_write
// Purpose - prepare and send sync for state transition
// -------------------------------------------------------------

void CTmTxBase::sync_write (int32 pv_nid,
                int32 pv_pid, TM_TX_STATE pv_state)
{
    TM_SYNC_TYPE lv_type;

    TMTrace (2, ("CTmTxBase::sync_write : ENTRY. TxnId %d, Current state %d, new state %d.\n",
                   iv_tag, iv_tx_state, pv_state));

    iv_tx_state = pv_state;
    
    switch (pv_state)
    {
    case TM_TX_STATE_BEGINNING:
    {
         lv_type = TM_BEGIN_SYNC;
         break;
    }
    case TM_TX_STATE_COMMITTED:
    case TM_TX_STATE_ABORTING:
    case TM_TX_STATE_ABORTING_PART2:
    case TM_TX_STATE_ABORTED:
        {
            lv_type = TM_END_SYNC;
            break;
        }
    case TM_TX_STATE_FORGOTTEN:
        {
            lv_type = TM_FORGET_SYNC;
            break;
        }
    default:
    {
            return; // nothing to do for now
    }
    }; //switch

#ifdef SUPPORT_TM_SYNC
    init_and_send_tx_sync_data( lv_type, pv_state, 
                            (TM_Transid_Type*)&iv_transid, 
                             pv_nid, pv_pid);
#endif

    TMTrace (2, ("CTmTxBase::sync_write : EXIT. TxnId %d, type %d.\n",
                   iv_tag, lv_type));
}

// --------------------------------------------------------------
// eventQ
// Purpose - retrieve the iv_eventQ to allow direct deque method
// calls against it.
// --------------------------------------------------------------
TM_DEQUE * CTmTxBase::eventQ()
{
   return &iv_eventQ;
}


// --------------------------------------------------------------
// eventQ_push
// Purpose - push a new event to the transactions event queue.
// These are pushed in FIFO order.
// pv_threadAssociated was added to ensure that when a thread
//   is associated with a new transaction, we don't get a race
//   condition with the transaction being picked up by a 
//   disassociating thread before the newly instantiated/allocated
//   thread gets a chance to process the THREAD_INITIATE event.
// --------------------------------------------------------------
void CTmTxBase::eventQ_push(CTmTxMessage * pp_msg, bool pv_threadAssociated)
{
    CTxThread * lp_thread = NULL;

    TMTrace(3, ("CTmTxBase::eventQ_push Entry: Txn ID (%d,%d), msg %d, thdAssoc %d, Thread Obj %s(%p) tid %ld, ThreadPending %d, Txn Obj %p\n",
      node(), seqnum(), (pp_msg)?pp_msg->msgid():-1, pv_threadAssociated,
      (ip_Thread)?ip_Thread->get_name():"MAIN", (void *) ip_Thread,
      (ip_Thread)?ip_Thread->get_id():-1,
      threadPending(), (void *) this));

    if (pp_msg == NULL)
    {
        tm_log_event(TM_TMTX_INVALID_MSG_TYPE, SQ_LOG_CRIT, "TM_TMTX_INVALID_MSG_TYPE");
        TMTrace (1, ("CTmTxBase::eventQ_push - Request to be queued is NULL.\n"));
        abort ();
    }

   // for worker threads we need to grab a new thread for every event, unless
   // the transaction object already has an associated thread
   if (!pv_threadAssociated && !ip_Thread && !threadPending())
   {
       TMTrace (3, ("CTmTxBase::eventQ_push : Instantiating new thread to service request.\n"));
       threadPending(true);
       lp_thread = gv_tm_info.new_thread(this);
       if (lp_thread != NULL)
          pv_threadAssociated = true;
       else
          threadPending(false);
   }

   iv_eventQ.push(pp_msg);

   if(ip_Thread || threadPending())
   {
      TMTrace (3, ("CTmTxBase::eventQ_push : signalling Txn ID (%d,%d) thread %s, tid %ld, event %d.\n",
                   node(), seqnum(), 
                   (ip_Thread)?ip_Thread->get_name():"MAIN", 
                   (ip_Thread)?ip_Thread->get_id():-1, pp_msg->requestType()));
      iv_CV.signal(true /*lock*/); 
   }
   else
   {
      if (pv_threadAssociated)
      {
         TMTrace (3, ("CTmTxBase::eventQ_push :  "
                        "Txn ID (%d,%d), event %d queued but thread not signalled.\n",
                        node(), seqnum(), pp_msg->requestType()));
      }
      else
      {
         TMTrace (3, ("CTmTxBase::eventQ_push : no thread associated with "
                        "ID %d, event %d queued and txn added to disassociated Q.\n",
                        seqnum(), pp_msg->requestType()));
         // Put this transaction object on the txdisassociated queue.
         // This allows an idle thread to pick up the work.
         // TODO: Should check here to make sure that the transaction object 
         // isn't already on the queue or we could end up with duplicates.
         gv_tm_info.txdisassociatedQ()->push(this);
      }
   }
}


// --------------------------------------------------------------
// eventQ_push_top
// Purpose - push an event to the top of the queue.  
// This is used for TM internal messages where they must be 
// processed before anything else like an Initialize.
// This has no effect if the TM is singlethreaded.
// --------------------------------------------------------------
void CTmTxBase::eventQ_push_top(CTmTxMessage * pp_msg)
{
    TMTrace(3, ("CTmTxBase::eventQ_push_top : Thread Obj %p(%s), lpid %ld, lname %s, Txn Obj %p, ID %d\n",
      (void *) ip_Thread, (ip_Thread)?ip_Thread->get_name():"MAIN", 
      SB_Thread::Sthr::self_id(), SB_Thread::Sthr::self_name(),
      (void *) this, seqnum()));

    if (pp_msg == NULL)
    {
        tm_log_event(TM_TMTX_INVALID_MSG_TYPE, SQ_LOG_CRIT, "TM_TMTX_INVALID_MSG_TYPE");
        TMTrace (1, ("CTmTxBase::eventQ_push_top - Request to be queued is NULL.\n"));
        abort ();
    }

   iv_eventQ.push_back(pp_msg);

   if(ip_Thread)
   {
      TMTrace (3, ("CTmTxBase::eventQ_push_top : signaling transaction ID %d thread %s, event %d.\n",
                        iv_tag, ip_Thread->get_name(), pp_msg->requestType()));
      iv_CV.signal(true /*lock*/);
   }
   else
      TMTrace (3, ("CTmTxBase::eventQ_push_top : no thread associated with "
                        "transaction ID %d, event %d queued.\n",
                        iv_tag, pp_msg->requestType()));
}


// --------------------------------------------------------------
// eventQ_pop
// Purpose - pop an event from the end of the queue.  Events are
// always processed in FIFO order.
// pp_msg must be allocated prior to calling eventQ_pop.  calling
// the CTmTxMessage::reply method against it will delete the request
// so no need to clean this up.
// pv_timeout is only implemented as:
// TX_EVENTQ_WAITFOREVER (-1): wait for ever, the default, or
// TX_EVENTQ_NOWAIT (-2): don't wait.
// --------------------------------------------------------------
CTmTxMessage * CTmTxBase::eventQ_pop(int pv_timeout)
{
   char la_buf[1024];
   CTmTxMessage * lp_msg = NULL;
   CTmTxBase *lp_txn = this;
   long lv_thrd = SB_Thread::Sthr::self_id();
   long lv_txnThrd = ip_Thread->get_id();

   TMTrace (2, ("CTmTxBase::eventQ_pop ENTRY timeout(%d) Thread %ld.\n", pv_timeout, lv_thrd));

   if (!ip_Thread || ip_Thread->get_id() != SB_Thread::Sthr::self_id() || lv_thrd != SB_Thread::Sthr::self_id() || lv_txnThrd != lv_thrd)
   {
      sprintf(la_buf, "TxnThread %p, %s(%ld), Executing Thread %s(%ld), Txn Obj %p", 
                        (void *) ip_Thread, 
                        ((ip_Thread)?ip_Thread->get_name():"MAIN"), 
                        ((ip_Thread)?ip_Thread->get_id():-1),
                        SB_Thread::Sthr::self_name(), SB_Thread::Sthr::self_id(), 
                        (void *) this);
      tm_log_event(DTM_LOGIC_ERROR_THR_MISMATCH, SQ_LOG_CRIT, "DTM_LOGIC_ERROR_THR_MISMATCH",
                   -1,-1,node(),seqnum(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,la_buf);
      TMTrace (1, ("CTmTxBase::eventQ_pop : Txn ID (%d,%d) Thread Mismatch! %s\n", node(), seqnum(), la_buf));
      fflush(stdout);
      abort();
   }

   if (pv_timeout == TX_EVENTQ_NOWAIT && iv_eventQ.empty())
   {
      TMTrace (2, ("CTmTxBase::eventQ_pop EXIT nowait and queue is empty.\n"));
      return NULL;
   }

   while (iv_eventQ.empty())
   {
      TMTrace (3, ("CTmTxBase::eventQ_pop transaction ID %d thread %s event Q "
                   "empty, waiting for signal.\n",
                    iv_tag, ((ip_Thread)?ip_Thread->get_name():"NO THREAD")));
      iv_CV.wait(true /*lock*/);
   }
   
   lp_msg = (CTmTxMessage *) iv_eventQ.pop_end();

   // If we've somehow picked up an event for a different thread abort now!
   if (lp_txn != this || ip_Thread->get_id() != SB_Thread::Sthr::self_id() || lv_thrd != SB_Thread::Sthr::self_id())
   {
      sprintf(la_buf, "(2) TxnThread %p, %s(%ld), Executing Thread %s(%ld), Txn Obj %p", 
                        (void *) ip_Thread, 
                        ((ip_Thread)?ip_Thread->get_name():"MAIN"), 
                        ((ip_Thread)?ip_Thread->get_id():-1),
                        SB_Thread::Sthr::self_name(), SB_Thread::Sthr::self_id(), 
                        (void *) this);
      tm_log_event(DTM_LOGIC_ERROR_THR_MISMATCH, SQ_LOG_CRIT, "DTM_LOGIC_ERROR_THR_MISMATCH",
                   -1,-1,node(),seqnum(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,la_buf);
      TMTrace (1, ("CTmTxBase::eventQ_pop : Txn ID (%d,%d) Thread Mismatch! %s\n", node(), seqnum(), la_buf));
      fflush(stdout);
      abort();
   }

   TMTrace (2, ("CTmTxBase::eventQ_pop EXIT transaction ID %d thread %s, "
                   "msgid %d returning event %d.\n",
                   iv_tag, ((ip_Thread)?ip_Thread->get_name():"NO THREAD"),
                   ((lp_msg)?lp_msg->msgid():0),
                   ((lp_msg)?lp_msg->requestType():0)));
   
   return lp_msg;
}


// --------------------------------------------------------------
// Event Specific Methods:
// --------------------------------------------------------------
// req_begin
// --------------------------------------------------------------
bool CTmTxBase::req_begin(CTmTxMessage * pp_msg)
{
   bool  lv_exit = false;
   TMTrace (2, ("CTmTxBase::req_begin : BASE CLASS!\n"));
   abort();
   return lv_exit;
} //req_begin

// --------------------------------------------------------------
//  req_end
// --------------------------------------------------------------
bool CTmTxBase::req_end(CTmTxMessage * pp_msg)
{
   bool lv_terminate = false;
   TMTrace (2, ("CTmTxBase::req_end : BASE CLASS!\n"));
   abort();
   return lv_terminate;
} //req_end


// --------------------------------------------------------------
// req_abort
// --------------------------------------------------------------
bool CTmTxBase::req_abort(CTmTxMessage * pp_msg)
{   
   bool lv_terminateThread = false;
   TMTrace (2, ("CTmTxBase::req_abort : BASE CLASS! Txn ID (%d,%d).\n", node(), seqnum()));
   abort();
   return lv_terminateThread;
} //req_abort

// --------------------------------------------------------------
// rollback_txn
// --------------------------------------------------------------
bool CTmTxBase::rollback_txn(CTmTxMessage * pp_msg)
{
   TMTrace (2, ("CTmTxBase::rollback_txn : ID %d, BASE CLASS!\n", 
                    seqnum()));
   abort();
   return false; //Don't terminate txn thread, still need to process the
                 //TM_MSG_TXINTERNAL_ABORTCOMPLETE.
} //rollback_txn


// --------------------------------------------------------------
// redrivecommit_txn
// --------------------------------------------------------------
bool CTmTxBase::redrivecommit_txn(CTmTxMessage * pp_msg)
{
   bool lv_terminate = false;
   TMTrace (2, ("CTmTxBase::redrivecommit_txn BASE CLASS!: ID %d, current txn state %d\n", 
      seqnum(), tx_state()));
   abort();
   return lv_terminate;
} //redrivecommit_txn

// --------------------------------------------------------------
// req_abort_complete
// --------------------------------------------------------------
bool CTmTxBase::req_abort_complete(CTmTxMessage * pp_msg)
{
   TMTrace (2, ("CTmTxBase::req_abort_complete BASE CLASS!: ID %d\n", seqnum()));
   abort();
   return false; //Don't terminate transaction, still need to do forget
}

// --------------------------------------------------------------
// req_end_complete
// --------------------------------------------------------------
bool CTmTxBase::req_end_complete(CTmTxMessage * pp_msg)
{
   TMTrace (2, ("CTmTxBase::req_end_complete BASE CLASS!: ID %d\n", seqnum()));
   abort();
   return false; //Don't terminate transaction, still need to do forget
}

// --------------------------------------------------------------
// req_forget
// --------------------------------------------------------------
bool CTmTxBase::req_forget(CTmTxMessage * pp_msg)
{
   bool lv_terminate = true; // Normal processing is for forget to 
                             // drive thread disassociation for 
                             // transaction threads.
   TMTrace (2, ("CTmTxBase::req_forget BASE CLASS!: ID %d\n", seqnum()));
   abort();
   return lv_terminate; //Finished with the transaction, disassociate and cleanup
} // req_forget

// --------------------------------------------------------------
// req_begin_complete
// --------------------------------------------------------------
bool CTmTxBase::req_begin_complete(CTmTxMessage * pp_msg)
{
    TMTrace (2, ("CTmTxBase::req_begin_complete BASE CLASS!: ID %d, error=%d\n", 
                    seqnum(), pp_msg->responseError()));
   abort();
   return false; //Leave transaction thread alive
}

// --------------------------------------------------------------
// Event Specific Methods implemented in base class
// --------------------------------------------------------------
// req_join
// Purpose - Txn specific processing for JOINTRANSACTION.
// --------------------------------------------------------------
bool CTmTxBase::req_join(CTmTxMessage * pp_msg)
{
   TMTrace(2, ("CTmTxBase::req_join : ENTRY: XA Txn ID (%d,%d).\n",
           node(), seqnum()));

   if (tx_state() != TM_TX_STATE_NOTX &&
       tx_state() != TM_TX_STATE_BEGINNING &&
       tx_state() != TM_TX_STATE_ACTIVE)
      {
         TMTrace (1, ("CTmTxBase::req_join : FEENDEDTRANSID\n"));
         pp_msg->responseError(mapErr(FEENDEDTRANSID));
      }
      else
      {
         // Transaction is either BEGINNING or ACTIVE, so can be joined. 
         // Note that a transaction in NOTX state has not yet reached BEGINNING.
         TMTrace (3, ("CTmTxBase::req_join : active participants %d\n",
                        num_active_partic()));
         pp_msg->responseError(FEOK);
         if (pp_msg->request()->u.iv_join_trans.iv_coord_role == true)
         {
            if (iv_ender_pid == -1)
            {
               iv_ender_pid = pp_msg->request()->u.iv_join_trans.iv_pid;
               inc_active_partic();
               add_app_partic(iv_ender_pid, pp_msg->request()->u.iv_join_trans.iv_nid);
            }
            else
               pp_msg->responseError(mapErr(FETXNOTSUSPENDED));
         } 
         else
         {
            inc_active_partic();
            add_app_partic(pp_msg->request()->u.iv_join_trans.iv_pid, 
                           pp_msg->request()->u.iv_join_trans.iv_nid);
         }
      }


   pp_msg->response()->u.iv_join_trans.iv_tm_pid = gv_tm_info.nid();
   pp_msg->reply(mapErr(FEOK));

   TMTrace (2, ("CTmTxBase::req_join : EXIT, replying error %d\n", 
            pp_msg->responseError()));

   // Return code doesn't matter, we run from the main thread
   return false;
} //req_join

// --------------------------------------------------------------
// req_status
// Purpose - Txn specific processing for STATUSTRANSACTION.
// We return a status of Aborted if the transaction is in the
// process of aborting because a TMF_DOOMTRANSACTION_ will
// leave the transaction marked for rollback once an ENDTRANSACTION
// or ABORTTRANSACTION is received.  SQL use STATUSTRANSACTION
// to determine whether the transaction was doomed.
// --------------------------------------------------------------
bool CTmTxBase::req_status(CTmTxMessage * pp_msg)
{
   TMTrace (2, ("CTmTxBase::req_status : ENTRY\n"));

   // For transactions that are beginning we want to report them as active
   if (tx_state() == TM_TX_STATE_BEGINNING || 
       tx_state() == TM_TX_STATE_NOTX)
      pp_msg->response()->u.iv_status_trans.iv_status = TM_TX_STATE_ACTIVE;
   else
   {
      if (isAborting() && tx_state() != TM_TX_STATE_ABORTING &&
          tx_state() != TM_TX_STATE_ABORTING_PART2 &&
          tx_state() != TM_TX_STATE_ACTIVE)
         pp_msg->response()->u.iv_status_trans.iv_status = TM_TX_STATE_ABORTED;
      else
         pp_msg->response()->u.iv_status_trans.iv_status = (short) tx_state();
   }

   pp_msg->reply(mapErr(FEOK));

   TMTrace (2, ("CTmTxBase::req_status : EXIT, state %d\n", tx_state()));

   // Return code doesn't matter, we run from the main thread
   return false;
} //req_status

// --------------------------------------------------------------
// req_suspend
// Purpose - Txn specific processing for SUSPENDTRANSACTION.
// --------------------------------------------------------------
bool CTmTxBase::req_suspend(CTmTxMessage * pp_msg)
{
   TMTrace (2, ("CTmTxBase::req_suspend : ENTRY\n"));

   if (tx_state() == TM_TX_STATE_NOTX)
   {
      TMTrace (1, ("CTmTxBase::req_suspend : FENOTRANSID\n"));
      pp_msg->responseError(mapErr(FENOTRANSID));
   }
   else
   {      
      if (remove_app_partic(pp_msg->request()->u.iv_suspend_trans.iv_pid, 
                            pp_msg->request()->u.iv_suspend_trans.iv_nid))
      {
          dec_active_partic();

          TMTrace (3, ("CTmTxBase::req_suspend : active participants %d\n", 
                     num_active_partic()));
          if ((pp_msg->request()->u.iv_suspend_trans.iv_coord_role == false)
               && (pp_msg->request()->u.iv_suspend_trans.iv_pid == iv_ender_pid))
               iv_ender_pid = -1; // someone better call join with coord = true
          pp_msg->responseError(mapErr(FEOK));
      }
      else 
      {
          TMTrace (3, ("CTmTxBase::req_suspend : not an active participant.\n"));
          pp_msg->responseError(mapErr(FENOTRANSID));
      }
   }
   pp_msg->reply();

   TMTrace (2, ("CTmTxBase::req_suspend : EXIT\n"));

   // Return code doesn't matter, we run from the main thread
   return false;
} //req_suspend

// --------------------------------------------------------------
// req_ax_reg
// Purpose - Txn Thread specific processing for ax_reg.
// --------------------------------------------------------------
bool CTmTxBase::req_ax_reg(CTmTxMessage * pp_msg)
{
   TMTrace (2, ("CTmTxBase::req_ax_reg : ID (%d,%d) ENTRY for rmid %d\n",
            node(), seqnum(), pp_msg->request()->u.iv_ax_reg.iv_rmid));

   short lv_error = register_branch(pp_msg->request()->u.iv_ax_reg.iv_rmid, pp_msg);

   pp_msg->response()->u.iv_ax_reg.iv_TM_incarnation_num = gv_tm_info.incarnation_num();
   pp_msg->response()->u.iv_ax_reg.iv_LeadTM_nid = gv_tm_info.lead_tm_nid();
   pp_msg->reply(mapErr(lv_error));

   // Don't reset here - we're now called in the main thread rather than queued to the 
   // transaction thread
   // We're finished so allow other work to commence
   //reset_transactionBusy();

   TMTrace (2, ("CTmTxBase::req_ax_reg : ID %d, EXIT\n", seqnum()));
   // ax_reg never causes the thread to terminate
   return false;
} //req_ax_reg

// --------------------------------------------------------------
// req_registerRegion
// Purpose - Txn Thread specific processing for registerRegion.
// This is only received in SeaTrans for HBase-trx participants.
// --------------------------------------------------------------
bool CTmTxBase::req_registerRegion(CTmTxMessage * pp_msg)
{
   TMTrace (2, ("CTmTxBase::req_registerRegion : ID (%d,%d) ENTRY.\n",
            node(), seqnum()));

   short lv_error = branches()->registerRegion(this, 0, pp_msg);

   // Reply here if we haven't yet done so.
   lock();
   if (pp_msg->replyPending()) 
      pp_msg->reply(lv_error);
   unlock();

   reset_transactionBusy();

   TMTrace (2, ("CTmTxBase::req_registerRegion : EXIT error %d, Txn ID (%d,%d).\n", 
               lv_error, node(), seqnum()));

   return false; //Never terminate the thread if there's no more work to do
} //req_registerRegion


//------------------------------------------------------------------------------
// register_branch
// Purpose - Process received ax_reg from TSE
// If pp_msg == NULL, we are registering a branch internally (eg because we
// just received a doomtxn from a TSE which was not participating.
// Returns error code.
//-----------------------------------------------------------------------------
short CTmTxBase::register_branch(int32 pv_rmid, CTmTxMessage * pp_msg = NULL)
{
   short lv_error = FEOK;
   bool lv_txnActive = false,
        lv_TTflagsChanged = false,
        lv_IownLock = false,
        lv_partic = false;
   RM_Info_TSEBranch *lp_rm_branch = NULL; 
   TMTrace (2, ("CTmTxBase::register_branch ENTER, Txn ID (%d,%d), rmid %d, Tx state %d.\n",
            node(), seqnum(), pv_rmid, tx_state()));

   // The transaction object may have the lock, but we
   // still want to accept the ax_reg here because failure 
   // to do so could hang the TSE which won't respond to 
   // requests like xa_rollback until it gets the reply to
   // ax_reg!
   branches()->branch_lock();
   if (ip_mutex->lock_count() == 0)
   {
      lv_IownLock = true;
      lock();
   }

   // Get the branch
   // This had better work or our pv_rmid is bad.
   lp_rm_branch = branches()->TSE()->return_slot(pv_rmid);
   if (lp_rm_branch == 0)
   {
      tm_log_event(DTM_TMTX_INVALID_RMID, SQ_LOG_WARNING, "DTM_TMTX_INVALID_RMID",
                   FEINVTRANSID,pv_rmid,node(),seqnum());
      TMTrace(1, ("CTmTxBase::register_branch - Unable to find RM %d to "
              "register branch for txn ID (%d,%d), transaction rejected with FEINVTRANSID(75).\n",
              pv_rmid, node(), seqnum()));
      // Reject the register request
      return FEINVTRANSID;
   }

   // Allow branches to participate in transactions in the following states:
   if (iv_tx_state == TM_TX_STATE_ACTIVE ||
       iv_tx_state == TM_TX_STATE_IDLE ||
       iv_tx_state == TM_TX_STATE_ABORTING ||
       iv_tx_state == TM_TX_STATE_COMMITTING ||
       iv_tx_state == TM_TX_STATE_PREPARING ||
       iv_tx_state == TM_TX_STATE_PREPARED ||
       iv_tx_state == TM_TX_STATE_HUNGABORTED ||
       iv_tx_state == TM_TX_STATE_ABORTING_PART2)
   {
      lv_txnActive = true;
      lv_partic = lp_rm_branch->add_partic(iv_transid);
      if (lv_partic)
      {
         if (iv_tx_state != TM_TX_STATE_ACTIVE &&
             iv_tx_state != TM_TX_STATE_IDLE) {
            TMTrace(1, ("CTmTxBase::register_branch - adding late participant rmid %d to Txn ID (%d,%d), Tx state %d.\n",
                    pv_rmid, node(), seqnum(), tx_state()));
         }
         else {
            TMTrace(1, ("CTmTxBase::register_branch - adding participant rmid %d to Txn ID (%d,%d), Tx state %d.\n",
                    pv_rmid, node(), seqnum(), tx_state()));
         }
         // Issue a warning if this was a doomtxn
         if (!pp_msg)
         {
            tm_log_event(DTM_TMTX_DOOMTXN_ADDED_PARTIC, SQ_LOG_WARNING, "DTM_TMTX_DOOMTXN_ADDED_PARTIC",
                        -1, pv_rmid, node(), seqnum(),-1,-1,-1,-1,-1,-1,-1,-1,tx_state());
            TMTrace(3, ("CTmTxBase::register_branch - RM %d already participating in Txn "
                    "ID (%d,%d), state %d.\n",
                    pv_rmid, node(), seqnum(), tx_state()));
         }
         branches()->TSE()->inc_num_rm_partic();
      }
      else //add_partic() returns false if the branch is already participating.
      {
         // If we're already resolved this branch, return an error
         lv_error = FEINVTRANSID;
         // Only log an event if this register request came from an ax_reg. Doomtxn
         // doesn't pass in a message pointer.  
         if (pp_msg)
            tm_log_event(DTM_TMTX_ALREADY_PARTIC, SQ_LOG_WARNING, "DTM_TMTX_ALREADY_PARTIC",
                         lv_error, pv_rmid, node(), seqnum(),-1,-1,-1,-1,-1,-1,-1,-1,tx_state());
         TMTrace(3, ("CTmTxBase::register_branch - RM %d already participating in Txn "
                 "ID (%d,%d), state %d, returning error %d.\n",
                 pv_rmid, node(), seqnum(), tx_state(), lv_error));
      }
      TMTrace(3, ("CTmTxBase::register_branch - RM %d ID (%d,%d) participating: %d, resolved: %d\n",
                  pv_rmid, node(), seqnum(),lv_partic, lp_rm_branch->resolved() ));

   }
   if (pp_msg)
   {
      memcpy(&pp_msg->response()->u.iv_ax_reg.iv_xid, &lp_rm_branch->iv_xid, sizeof(XID));
      // We need to add in any flags set by the TSE that we don't know about.
      // If the TSE set new flags then we need to write a trans state record
      // now so that we know the new flag settings in the case of a failure.
      int64 lv_TTflags = TT_flags() | pp_msg->request()->u.iv_ax_reg.iv_flags;
      if (lv_TTflags != TT_flags())
      {
         lv_TTflagsChanged = true;
         TMTrace(3, ("CTmTxBase::register_branch : Txn ID (%d,%d) Detected "
                     "different TT flags from TSE RM %d, TM "
                     PFLLX ", TSE " PFLLX ", combined " PFLLX ".\n",
                     node(), seqnum(), pv_rmid, TT_flags(), 
                     pp_msg->request()->u.iv_ax_reg.iv_flags, lv_TTflags));
         char lv_TTflagsStr[10], *lp_TTflagsStr = (char *) &lv_TTflagsStr;
         sprintf(lp_TTflagsStr, "0x" PFLLX, lv_TTflags);
         tm_log_event(DTM_TMTX_REG_TT_FLAGS_CHANGED, SQ_LOG_INFO, "DTM_TMTX_REG_TT_FLAGS_CHANGED",
                      -1,pv_rmid,node(),seqnum(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,lp_TTflagsStr);
         // Record that a NO UNDO occured.
         if (TT_flags() & TM_TT_NO_UNDO)
         {
            tm_log_event(DTM_NOUNDO, SQ_LOG_WARNING, "DTM_NOUNDO", 
                         -1,pv_rmid,node(),seqnum(),-1,-1,-1,-1,-1,-1,-1,-1,tx_state());
            TMTrace(1, ("CTmTxBase::register_branch : WARNING - Txn ID (%d,%d), state %d - RM %d set "
                    "NO UNDO.  This will leave the database in an inconsistent state.\n",
                    node(), seqnum(), tx_state(), pv_rmid));
         }
         TT_flags(lv_TTflags);
         gv_tm_info.write_trans_state (&iv_transid, iv_tx_state, abort_flags(), false);
      }
      pp_msg->response()->u.iv_ax_reg.iv_flags = TT_flags();
   }
   if (!lv_txnActive && !lv_TTflagsChanged)
   {
      tm_log_event(DTM_TMTX_REG_ERROR_TOOLATE, SQ_LOG_WARNING, "DTM_TMTX_REG_ERROR_TOOLATE",
                   -1,pv_rmid,node(),seqnum(),-1,-1,-1,-1,-1,-1,-1,-1,tx_state(),
                   lp_rm_branch->resolved(),lp_rm_branch->partic());
      TMTrace(1, ("CTmTxBase::register_branch - ERROR RM %d is too late to participate in "
              "Txn ID (%d,%d), state %d, partic %d, resolved %d.\n",
              pv_rmid, node(), seqnum(), tx_state(),
              lp_rm_branch->partic(), lp_rm_branch->resolved()));
      lv_error = FEINVTRANSID;
    }

   if (pp_msg)
      pp_msg->responseError(lv_error);

   branches()->branch_unlock();

   if (lv_IownLock)
      unlock();

   TMTrace(2, ("register_branch EXIT, error %d\n", lv_error));
   return lv_error;
} // CTmTxBase::register_branch


// ----------------------------------------------------------------
// internal_abortTrans
// Purpose - Queue a rollback request for this transaction.
// This is currently only used during takeover, shutdown and
// process death.
// ----------------------------------------------------------------
int32 CTmTxBase::internal_abortTrans(bool pv_takeover_or_shutdown)
{ 
   CTmTxMessage * lp_msg = new CTmTxMessage(TM_MSG_TXINTERNAL_ROLLBACK);

   lp_msg->request()->u.iv_rollback_internal.iv_takeover_or_shutdown = pv_takeover_or_shutdown;

   queueToTransaction(&iv_transid, lp_msg);

   TMTrace (2, ("CTmTxBase::internal_abortTrans : EXIT\n"));
    
   return FEOK;
} //internal_abortTrans


// ----------------------------------------------------------------
// redrive_rollback
// Purpose - Queue a redrive rollback request for this transaction.
// This is currently only used during a takeover.
// ----------------------------------------------------------------
int32 CTmTxBase::redrive_rollback()
{
   CTmTxMessage * lp_msg = new CTmTxMessage(TM_MSG_TXINTERNAL_REDRIVEROLLBACK);

   queueToTransaction(&iv_transid, lp_msg);

    TMTrace (2, ("CTmTxBase::redrive_rollback : EXIT\n"));
    
   return FEOK;
} //redrive_rollback

// ----------------------------------------------------------------
// redrive_commit
// Purpose - Queue a redrive commit request for this transaction.
// This is currently only used during a takeover.
// ----------------------------------------------------------------
int32 CTmTxBase::redrive_commit()
{
   CTmTxMessage * lp_msg = new CTmTxMessage(TM_MSG_TXINTERNAL_REDRIVECOMMIT);

   queueToTransaction(&iv_transid, lp_msg);

   TMTrace (2, ("CTmTxBase::redrive_commit : EXIT\n"));
    
   return FEOK;
} //redrive_commit


void CTmTxBase::schedule_redrive_sync()
{

   CTmTxMessage * lp_msg = new CTmTxMessage(TM_MSG_TXINTERNAL_REDRIVESYNC);
   eventQ_push(lp_msg);
}


//-----------------------------------------------------------------------------
// decrement & test cleanup semaphore
// Only return true of the semaphore is 1 to make sure
// that only one caller will do the actual cleanup.
// The idea is that every thread working on a specific transaction calls
// inc_cleanup() when they want the txn object cleaned up on exit.
// The cleanup routine executes when the thread disassociates, calls
// dec_cleanup() which will only return true when the iv_cleanup_sem count
// is 1, so only one thread will cleanup the txn object.
// Note that we lock the Qmutex when incrementing the cleanup
// semaphore to ensure no one trys to write to the eventQ or PendingRequestQ
// while we're setting cleanup.
//-----------------------------------------------------------------------------
bool CTmTxBase::dec_cleanup() 
{
   bool cleanup = false;
   
   lock();
   if (iv_cleanup_sem == 1)
      cleanup = true;
   if (iv_cleanup_sem > 0)
      iv_cleanup_sem--;
   unlock();

   TMTrace (2, ("CTmTxBase::dec_cleanup: semaphore = %d, returning %d\n", iv_cleanup_sem, cleanup));
   return cleanup;
}
// increment cleanup semaphore
void CTmTxBase::inc_cleanup()
{
   lock();
   Qlock();
   iv_cleanup_sem++;
   Qunlock();
   unlock();

   TMTrace (2, ("CTmTxBase::inc_cleanup: semaphore = %d\n", iv_cleanup_sem));
}

//-----------------------------------------------------------------------------
// set_transactionBusy
// The following 2 methods allow setting and resetting the
// iv_transactionBusy within the same lock to ensure
// it's consistent and another thread doesn't interfere.
// Qlock() must be called before calling set_transactionBusy and Qunlock()
// after any outstanding request is queued to eventQ or PendingRequestQ.
// The return value indicates whether set_transactionBusy succeeded in setting
// the value.
// We also set the iv_currRequest here.  It may be needed later to handle
// a request which spans waits.
// Returns true if iv_transactionBusy==false, ie if the transaction object 
//              is available.
//         false if iv_transactionBusy==true - the transaction object is busy.
//-----------------------------------------------------------------------------
bool CTmTxBase::set_transactionBusy(CTmTxMessage * pp_msg)
{
   bool lv_return = false;

   TMTrace (2, ("CTmTxBase::set_transactionBusy : ENTRY ID (%d,%d), iv_transactionBusy=%d.\n",
                   node(), seqnum(), iv_transactionBusy));

   if (iv_transactionBusy == false)
   {
      iv_transactionBusy = true;
      lv_return = true;
      // Save the request, it will be needed to drive events after a wakeup for
      // multi-part requests like ENDTRANSACTION.
      currRequest(pp_msg);
   }
   else
   {
         TMTrace (3, ("CTmTxBase::set_transactionBusy : WARNING: Possible "
                      "programming bug. iv_transactionBusy already set on call "
                      "to set_transactionBusy.\n"));
         if (thread() == NULL && currRequest() == NULL)
         {
            TMTrace (3, ("CTmTxBase::set_transactionBusy : BUG! Transaction "
                      "ID (%d,%d) busy but has no associated thread and current request!\n",
                      node(), seqnum()));
            abort();
         }
   }

    TMTrace (2, ("CTmTxBase::set_transactionBusy : EXIT ID (%d,%d), returning "
                   "transactionBusy=%d.\n", node(), seqnum(), lv_return));

   return lv_return;
} //set_transactionBusy


//-----------------------------------------------------------------------------
// reset_transactionBusy
// Resets the transactionBusy flag.  This method also checks the
// PendingRequest queue and pops the top entry if there is one.
// Note that we also delete the iv_currRequest message object here,
// so callers had better have replied to the client prior to calling
// reset_transactionBusy!
// This routine locks the Qmutex to serialize changes to the transactionBusy
// flag and writes/reads to the PendingRequestQ.  It does not lock the transaction
// object.  To ensure we don't mess up, 
// pv_cleanupPending input.
//    True if cleaning up the transaction object.  This happens in forget().
//    In this case we cleanup the PendingRequestQ, replying to any that have
//    outstanding replies.
// Returns true if iv_transactionBusy==true, ie if the transaction object 
//              was busy.
//         false if iv_transactionBusy==false - the transaction object was free.
//-----------------------------------------------------------------------------
bool CTmTxBase::reset_transactionBusy(bool pv_cleanupPending)
{
   CTmTxMessage *lp_msg = NULL;
   CTmTxMessage *lp_firstMsg = NULL;

   TMTrace (2, ("CTmTxBase::reset_transactionBusy : ENTRY, ID (%d,%d), "
            "cleanupPending=%d, iv_transactionBusy=%d.\n",
            node(), seqnum(), pv_cleanupPending, iv_transactionBusy));

   Qlock();
   if (iv_transactionBusy)
   {
      // Clean up the old message, we've finished with it
      delete_currRequest();

      // Check the PendingRequest queue
      lp_msg = lp_firstMsg = (CTmTxMessage *) iv_PendingRequestQ.pop_end();
      while (lp_msg)
      {
         TMTrace (3, ("CTmTxBase::reset_transactionBusy : API "
                      "request popped off PendingRequestQ for msgid(%d).\n", 
                      lp_msg->msgid()));
         // If the cleanupPending flag was set then reply
         // to all outstanding requests.
         if (pv_cleanupPending)
         {
             if (lp_msg->replyPending())
             {
               if (isAborting())
                  lp_msg->reply(mapErr(FEABORTEDTRANSID));
               else
                  lp_msg->reply(mapErr(FEENDEDTRANSID));
             }
            delete lp_msg;
            lp_msg = (CTmTxMessage *) iv_PendingRequestQ.pop_end();
         }
         else
         {
            eventQ_push(lp_msg);
            // Make this new request the current one for the transaction
            currRequest(lp_msg);
            lp_msg = NULL; // Break out of while
         }
      } //while

       // Reset the transactionBusy flag now if there's nothing more to process
     if (currRequest() == NULL)
      {
         iv_transactionBusy = false;
         TMTrace (3, ("CTmTxBase::reset_transactionBusy: No requests outstanding, "
                         "iv_transactionBusy=false.\n"));
      }
   }
   Qunlock();

   TMTrace (2, ("CTmTxBase::reset_transactionBusy : EXIT ID (%d,%d), returning "
                "transactionBusy=%d.\n", node(), seqnum(), iv_transactionBusy));

   return iv_transactionBusy;
} //reset_transactionBusy


// ---------------------------------------------------------
// txn_object_good
// Purpose : Check that the transaction object is allocated 
// to this transaction.  There is a window in which the
// transaction might have been cleaned up and the transaction 
// object returned to the free list.
// 
// Note that this function will delete pp_msg if there is
// an error, so it must not be used after the call if false
// is returned
// --------------------------------------------------------
bool CTmTxBase::txn_object_good(TM_Txid_Internal *pp_transid, CTmTxMessage * pp_msg) 
{
    if (!cleaning_up() && iv_in_use && iv_transid.iv_seq_num == pp_transid->iv_seq_num)
    {
       TMTrace(2, ("CTmTxBase::txn_object_good txn object is good :-)\n"));
       return true;
    }
    else
    {
      TMTrace(1, ("CTmTxBase::txn_object_good txn %d is old! Replying "
              "FEINVTRANSID to late request %d & deleting msg object.\n",
              pp_transid->iv_seq_num, pp_msg->requestType()));
      pp_msg->reply(mapErr(FEINVTRANSID));
      delete pp_msg;
      return false;
    }
}


// --------------------------------------------------------------
// CTmTxBase::reply_to_queuedRequests
// Purpose : reply to all requests queued on the iv_PendingRequestQ
// with the error provided.
// --------------------------------------------------------------

void CTmTxBase::reply_to_queuedRequests(short pv_error)
{
   CTmTxMessage * lp_msg;

   TMTrace (2, ("CTmTxBase::reply_to_queuedRequests ENTRY\n"));

   lock();
   // Reply to any events queued against the transaction
   lp_msg = (CTmTxMessage *) eventQ_pop(TX_EVENTQ_NOWAIT);
   while (lp_msg)
   {
      if (lp_msg->replyPending())
      {
          TMTrace (3, ("CTmTxBase::reply_to_queuedRequests replying to queued "
                         "transaction event %d, msgid %d\n",
                         lp_msg->requestType(), lp_msg->msgid()));
         lp_msg->reply(mapErr(pv_error));
      }
      else
          TMTrace (3, ("CTmTxBase::reply_to_queuedRequests queued transaction "
                         "event %d, msgid %d ignored, no reply pending.\n",
                         lp_msg->requestType(), lp_msg->msgid()));
      
      // If this message matches the current Request then clear it as we've just
      // processed the message
      if (lp_msg == ip_currRequest)
         ip_currRequest = 0;
      delete lp_msg;
      lp_msg = (CTmTxMessage *) eventQ_pop(TX_EVENTQ_NOWAIT);
   }

   // Reply to pending requests for this transaction.  These can't be handled
   // because we're in the process of forgetting the transaction.
   lp_msg = (CTmTxMessage *) iv_PendingRequestQ.pop_end();
   while (lp_msg)
   {
       TMTrace (3, ("CTmTxBase::reply_to_queuedRequests replying to request %d\n",
                      lp_msg->requestType()));
      
      lp_msg->reply(mapErr(pv_error));
      delete lp_msg;
      lp_msg = (CTmTxMessage *) iv_PendingRequestQ.pop_end();
   }

   // Check to see if we have a request queued that needs to be replied to.
   if (ip_currRequest && ip_currRequest->replyPending())
   {
       TMTrace (3, ("CTmTxBase::reply_to_queuedRequests replying to queued event %d thread %s, txn tag %d.\n",
                      ip_currRequest->requestType(),
                      ((ip_Thread)?ip_Thread->get_name():"NO THREAD"),
                      iv_tag));

      ip_currRequest->reply(mapErr(pv_error));
      delete_currRequest();
   }
   unlock();

   TMTrace (2, ("CTmTxBase::reply_to_queuedRequests EXIT\n"));
} //reply_to_queuedRequests


// ---------------------------------------------------------------------------
// CTmTxBase::add_app_partic
// Purpose : add the process to the list of processes participating in this 
// transaction.
// pv_pid pid of participating process
// pv_nid nid of participating process - not of this TM!!!
// Returns true if successfully added.
//         false indicates we didn't get the memory for the pid, or it's
//               already in the list!
// ---------------------------------------------------------------------------
bool CTmTxBase::add_app_partic (int32 pv_pid, int32 pv_nid)
{
    CTmTxKey lv_key(pv_nid, pv_pid);

    CTmTxKey *lp_key = (CTmTxKey *) iv_app_partic_list.get(lv_key.id());
    if (lp_key == NULL)
    {
       lp_key = new CTmTxKey(lv_key.id());
       if (lp_key)
       {
          iv_app_partic_list.put(lv_key.id(), lp_key);
          TMTrace (3, ("CTmTxBase::add_app_partic added process (%d,%d) : %p\n",
                   pv_nid, pv_pid, (void *)lp_key));
          return true;
       }
       else
       {
          TMTrace(1, ("CTmTxBase::add_app_partic (%d,%d) no memory to add int32 pid!, aborting.\n",
                  pv_nid, pv_pid));
          tm_log_event(DTM_TM_ADD_APP_PARTIC_NOMEM, SQ_LOG_CRIT, "DTM_TM_ADD_APP_PARTIC_NOMEM",
                       -1,-1,pv_nid,pv_pid);
          abort();
       }
    }
    else
    {
       TMTrace (1, ("CTmTxBase::add_app_partic Warning: nid %d, pid %d found in "
                "app_partic_list - ignored.\n", pv_nid, pv_pid));
    }
    return false;
} //add_app_partic


bool CTmTxBase::remove_app_partic (int32 pv_pid, int32 pv_nid)
{
    CTmTxKey lv_key(pv_nid, pv_pid);

    CTmTxKey *lp_key = (CTmTxKey *) iv_app_partic_list.remove(lv_key.id());
    if (lp_key)
    {
        TMTrace (3, ("CTmTxBase::remove_app_partic removing process (%d,%d) : %p\n", 
                 pv_nid, pv_pid, (void *)lp_key));
        delete lp_key;
        return true;
    }
    return false;
}

// erase_app_partic is similar to remove_app_partic but can be called by
// code which reads through the map using get_first, ...
// Pass in the map element pointer.  Note that in this case the contents
// of the map pointer is the key.
bool CTmTxBase::erase_app_partic(CTmTxKey *pp_key)
{
    if (pp_key->id())
    {
        TMTrace (3, ("CTmTxBase::erase_app_partic removing process (%d,%d) : %p\n", 
                 pp_key->node(), pp_key->pid(), (void *) pp_key));
       iv_app_partic_list.erase(pp_key->id());
       delete pp_key;
        return true;
    }
    return false;
}

bool CTmTxBase::is_app_partic (int32 pv_pid, int32 pv_nid)
{
    CTmTxKey lv_key(pv_nid, pv_pid);

    CTmTxKey *lp_key = (CTmTxKey *)iv_app_partic_list.get(lv_key.id());
    if (lp_key)
        return true;
    else
        return false;
}

void CTmTxBase::cleanup_app_partic()
{
   // go through our particpation list and make sure there is no
   // outstanding particpants (this is fine as it could have been
   // a unilateral abort or shutdown).  Just get rid of it.
   int32 lv_count = 0;
   CTmTxKey *lp_key = NULL;
   CTmTxKey *lp_last_key = NULL;
        
   lp_key = (CTmTxKey *) iv_app_partic_list.get_first();
   while (lv_count < num_active_partic() && lp_key)
   {            
      lv_count++;
      lp_last_key = lp_key;
      lp_key = (CTmTxKey *) iv_app_partic_list.get_next();
      erase_app_partic(lp_last_key);
   }
   iv_app_partic_list.get_end();

   if (lv_count != num_active_partic())
      TMTrace(1, ("CTmTxBase::cleanup_app_partic Warning: actual participants "
             "%d does not match stored count %d\n", 
             lv_count, num_active_partic()));
   num_active_partic();
}


//----------------------------------------------------------------------------
// PendingRequestQ_push
// Purpose : Push a message onto the iv_PendingRequestQ.  This method also
// checks whether the CTmTxBase object is associated with a thread.  If it
// is not, then we add the transaction to the txdisassociatedQ to make sure
// it is woken when a thread becomes available.
//----------------------------------------------------------------------------
void CTmTxBase::PendingRequestQ_push(CTmTxMessage *pp_msg)
{
   TMTrace (2, ("CTmTxBase::PendingRequestQ_push ENTRY. ID %d, msgid %d.\n", 
                   tag(), pp_msg->msgid()));

   PendingRequestQ()->push(pp_msg);


   // If there is no thread associated here we must add the txn object to the txdisassociatedQ
   // to ensure it's picked up when a thread becomes available.
   if (thread() == NULL)
   {
      gv_tm_info.txdisassociatedQ()->push(this);
      if (iv_trace_level >= 3) 
         trace_printf ("CTmTxBase::PendingRequestQ_push ID %d, msgid %d. Txn object "
                        "added to txdisassociatedQ. " PFLL " entries on txdisassociatedQ.\n",
                        tag(), pp_msg->msgid(), gv_tm_info.txdisassociatedQ()->size());
   }

    TMTrace (2, ("CTmTxBase::PendingRequestQ_push EXIT. ID %d, msgid %d. "
                    "" PFLL " entries on PendingRequest queue.\n",
                    tag(), pp_msg->msgid(), PendingRequestQ()->size()));
} //CTmTxBase::PendingRequestQ_push



// ----------------------------------------------------------------------------
// CTmTxBase::queueToTransaction
// Purpose : Queues the request against the transaction object.
// Check that we aren't already processing a request which alters the transaction state
// (Begin, Abort, or End).  If we are, then put this request on the PendingRequestQ.
// If the transaction is not processing a state altering request, then queue the request
// directly to the transactions event queue.
// Returns true if it successfully queued the message against the transaction 
// object.  Otherwise it returns false.
// ----------------------------------------------------------------------------
bool CTmTxBase::queueToTransaction(TM_Txid_Internal *pp_transid, CTmTxMessage *pp_msg)
{
   bool lv_success = false;

   TMTrace (2, ("CTmTxBase::queueToTransaction : ENTRY\n"));
  
   // There is not need to lock the transaction object here since we're only writing to the
   // event queue.

   Qlock();
   if (txn_object_good(pp_transid, pp_msg))
   {  
      TMTrace (3, ("CTmTxBase::queueToTransaction txn ID (%d,%d), "
               "Request %d queued directly to txn object event queue for msgid(%d), entries in eventQ %ld.\n",
               node(), seqnum(), pp_msg->requestType(), pp_msg->msgid(), iv_eventQ.size()));
      eventQ_push(pp_msg, (ip_Thread?true:false));
      lv_success = true;
   }
   Qunlock();

   TMTrace (2, ("CTmTxBase::queueToTransaction : EXIT returning %d, entries in eventQ %ld\n", 
            lv_success, iv_eventQ.size()));

   return lv_success;
} // CTmTxBase::queueToTransaction


// ----------------------------------------------------------------------------
// CTmTxBase::currRequest
// Purpose : set the current request.  
// ----------------------------------------------------------------------------
void CTmTxBase::currRequest(CTmTxMessage * pp_msg) 
{
   if (ip_currRequest != NULL)
   {
      tm_log_event(DTM_TMTX_INVALID_STATE_CURR_REQ, SQ_LOG_CRIT, "DTM_TMTX_INVALID_STATE_CURR_REQ",
                    -1, /*error_code*/ 
                    -1, /*rmid*/
                    -1, /*dtmid*/ 
                    iv_transid.iv_seq_num, /*seq_num*/
                    ip_currRequest->msgid(), /*msgid*/
                    -1, /*xa_error*/
                    -1, /*pool_size*/
                    -1, /*pool_elems*/
                    -1, /*msg_retries*/
                    -1, /*pool_high*/
                    -1, /*pool_low*/
                    -1, /*pool_max*/
                    -1, /*tx_state*/
                    -1, /*data */
                    -1, /*data1*/
                    -1,/*data2 */
                    NULL,/*string1*/   
                    -1, /* node =*/
                    pp_msg->msgid() /*msgid2*/
);
      TMTrace (1, ("CTmTxBase::currRequest - Failed to set currRequest for Transaction ID %d, current "
         "msgid %d, new msgid %d\n", 
         iv_transid.iv_seq_num, ip_currRequest->msgid(), pp_msg->msgid()));
      abort();
   }

   ip_currRequest = pp_msg;
} // CTmTxBase::currRequest


// ----------------------------------------------------------------------------
// CTmTxBase::addTimerEvent
// Purpose : Wrapper to simplify the addition of timer events to the timer 
// threads event queue by transaction.  Events will always be queued back 
// against the calling transaction objects event queue.
// pv_type is a message type
// pv_delayInterval is the interval the timer thread will wait before
// posting the event back to the transaction in msec.
// ----------------------------------------------------------------------------
CTmTimerEvent * CTmTxBase::addTimerEvent(TM_MSG_TYPE pv_type, int pv_delayInterval)
{
   TMTrace (2, ("CTmTxBase::addTimerEvent : ENTRY, msg type %d, delay %d\n", 
      pv_type, pv_delayInterval));

   CTmTimerEvent *lp_timerEvent = new CTmTimerEvent(pv_type, thread(), this, pv_delayInterval);

   gv_tm_info.tmTimer()->eventQ_push((CTmEvent *) lp_timerEvent);

   return lp_timerEvent;
} // CTmTxBase::addTimerEvent


// ----------------------------------------------------------------------------
// CTmTxBase::setAbortTimeout
// Purpose : Set the abort timeout for this transaction.  This involves
// creating a new timer event for the timeout.
// pv_timeout is the timeout in seconds.  
//    -1 = wait forever.
//    0  = use default (gv_tm_info.timeout())
// ----------------------------------------------------------------------------
void CTmTxBase::setAbortTimeout(int32 pv_timeout)
{
   int32 lv_timeout;
   int32 lv_msec;

   if (pv_timeout != 0)
      lv_timeout = pv_timeout;
   else
      lv_timeout = gv_tm_info.timeout();
   lv_msec = (lv_timeout==-1)?-1:(lv_timeout*1000);
   TMTrace (2, ("CTmTxBase::setAbortTimeout : ENTRY, timeout %d\n", 
      lv_timeout));

   if (ip_timeoutEvent != NULL)
      gv_xaTM.tmTimer()->cancelEvent(ip_timeoutEvent);

   if (lv_timeout != -1)
      ip_timeoutEvent = addTimerEvent(TM_MSG_TXINTERNAL_ROLLBACK, lv_msec);
} // CTmTxBase::setAbortTimeout


// ----------------------------------------------------------------------------
// CTmTxBase::addHungTimerEvent
// Purpose : Add a hung timer event for this transaction.  Only one of these
// is allowed per transaction at any time.
// pv_type should be TM_MSG_TXINTERNAL_REDRIVEROLLBACK or TM_MSG_TXINTERNAL_REDRIVECOMMIT.
// ----------------------------------------------------------------------------
void CTmTxBase::addHungTimerEvent(TM_MSG_TYPE pv_type)
{
   TMTrace (2, ("CTmTxBase::addHungTimerEvent : ENTRY ID %d, msg type %d\n", 
            seqnum(), pv_type));
   if (ip_hung_event == 0)
      ip_hung_event = addTimerEvent(pv_type, 
                                    gv_tm_info.trans_hung_retry_interval());
} //addHungTimerEvent


// ----------------------------------------------------------------------------
// CTmTxBase::hung_redrive
// Redrive a hung transaction.
// This routine walks through the global RM info table
// ----------------------------------------------------------------------------
void CTmTxBase::hung_redrive()
{
   //RM_Info_TSEBranch *lp_rmInfo;   // Txn copy of RM info.

   TMTrace (2, ("CTmTxBase::hung_redrive : ENTRY ID %d\n", seqnum()));
   TM_MSG_TYPE lv_type;
   switch (tx_state())
   {
   case TM_TX_STATE_HUNGABORTED:
      lv_type = TM_MSG_TXINTERNAL_REDRIVEROLLBACK;
      break;
   case TM_TX_STATE_HUNGCOMMITTED:
      lv_type = TM_MSG_TXINTERNAL_REDRIVECOMMIT;
      break;
   default:
      // Ignore, not hung.
      return;
   } // switch state

   /*Commented out as I don't think we mark the RMs as failed over or recovering
      in the ia_TSEBranches table.
   for (int lv_index = 0; ((lv_index <= ia_TSEBranches.return_highest_index_used()) && 
                            (lv_index < lv_count)); lv_index++)
   {
      lp_rmInfo = ia_TSEBranches.get_slot(lv_index);
      if (lp_rmInfo->state() == TSEBranch_FAILOVER)
      {
         lp_rmInfo->state(TSEBranch_UP);
         // Set the RM's 'partic' flag to true
         safe_initialize_slot(lp_rmInfo->rmid());
         inc_prepared_rms();
      }
   } // for each rm
   */

   CTmTxMessage * lp_msg = new CTmTxMessage(lv_type);
   queueToTransaction(&iv_transid, lp_msg);

   TMTrace (2, ("CTmTxBase::hung_redrive : EXIT\n"));
} //CTmTxBase::hung_redrive


// ------------------------------------------------------------
// CTmTxBase::associate_tx
// Instantiate a transaction thread if required and associate
// it with this transaction.
// Queue the specified event to this transaction object.
// Returns true if a thread has been associated with this 
//                transaction.
//           false if no thread was associated.
// ------------------------------------------------------------
bool CTmTxBase::associate_tx(CTmTxMessage *pp_msg)
{
    TMTrace (2, ("CTmTxBase::associate_tx : ENTRY Txn ID (%d,%d) request %d.\n",
             node(), seqnum(), pp_msg->requestType()));

    // Transaction threads instantiate a thread object
    // and associate it with the transaction for its whole life.
    bool lv_threadAssociated = false;
    if (gv_tm_info.threadModel() == transaction)
    {
       TMTrace(3, ("CTmTxBase::associate_tx : Instantiating new thread.\n"));
       CTxThread * lp_thread = gv_tm_info.new_thread(this);
       if (lp_thread != NULL)
          lv_threadAssociated = true;
    }

    TMTrace(2, ("CTmTxBase::associate_tx : EXIT returning threadAssociated %d.\n",
            lv_threadAssociated));
    return lv_threadAssociated;
} //associate_tx


// ------------------------------------------------------------
// CTmTxBase::recover_tx
// This is used during recovery to instantiate and associate
// a thread with this transaction, and queue a request against
// it.
// ------------------------------------------------------------
void CTmTxBase::recover_tx(TM_MSG_TYPE pv_type)
{
    TMTrace (2, ("CTmTxBase::recover_tx : ENTRY Txn ID (%d,%d) request %d.\n",
             node(), seqnum(), pv_type));
   CTmTxMessage * lp_msg = new CTmTxMessage(pv_type);
   bool lv_threadAssociated = associate_tx(lp_msg);

   stats()->txnTotal()->start();
   // Set the appropriate state before queuing the request
   switch (pv_type)
   {
       case TM_MSG_TXINTERNAL_REDRIVEROLLBACK:
           stats()->txnAbort()->start();
           tx_state(TM_TX_STATE_ABORTING);
           break;
       case TM_MSG_TXINTERNAL_REDRIVECOMMIT:
           stats()->txnCommit()->start();
           tx_state(TM_TX_STATE_COMMITTING);
           break;
       default:
           // Should never get here
           TMTrace(1, ("CTmTxBase::recover_tx : PROGRAMMING ERROR!\n"));
           tm_log_event(DTM_RECOV_BAD_TYPE, SQ_LOG_CRIT, "DTM_RECOV_BAD_TYPE");
           abort();
   }

   // Setting the transactionBusy must succeed here because no one 
   // else knows about the transaction yet!
   Qlock();
   bool lv_txnFree = set_transactionBusy(lp_msg); 
   Qunlock(); //No need to wait here, we will only queue against eventQ
   if (!lv_txnFree)
   {
        TMTrace(1, ("CTmTxBase::recover_tx : ERROR transaction collision - txn ID (%d,%d) busy!\n",
                    node(), seqnum()));
        tm_log_event(DTM_RECOV_TRANSACTION_COLLISION, SQ_LOG_CRIT, "DTM_RECOV_TRANSACTION_COLLISION",
                     -1, -1, node(), seqnum());
        abort();
   }
   // Queue the begin against the transaction object
   eventQ_push(lp_msg, lv_threadAssociated);

   TMTrace(2, ("CTmTxBase::recover_tx : EXIT.\n"));
} //recover_tx


// --------------------------------------------------------------
// process_eventQ
// --------------------------------------------------------------
void CTmTxBase::process_eventQ()
{
   TMTrace (2, ("CTmTxBase::process_eventQ : BASE CLASS! ID %d, thread %s.\n",
                   iv_tag, ((gv_tm_info.multithreaded())?ip_Thread->get_name():"SINGLE THREAD")));
   abort();
}


// -------------------------------------------------------------
// schedule_eventQ
// -------------------------------------------------------------
void CTmTxBase::schedule_eventQ()
{
   TMTrace (2, ("CTmTxBase::schedule_eventQ BASE CLASS! ID %d, state %d.\n",
                   iv_tag, iv_tx_state));
   abort();
}


// --------------------------------------------------------------
// schedule_abort
// --------------------------------------------------------------
int32 CTmTxBase::schedule_abort()
{
   int32 lv_error = FEOK;
   TMTrace (2, ("CTmTxBase::schedule_abort BASE CLASS!, Txn ID (%d,%d), state %d.\n",
                  node(), seqnum(), iv_tx_state));
   abort();
   return lv_error;
}


// --------------------------------------------------------------
// doom_txn
// --------------------------------------------------------------
short CTmTxBase::doom_txn()
{
   short lv_error = FEOK;
   TMTrace (2, ("CTmTxBase::doom_txn BASE CLASS!, ID %d, state %d.\n",
                  seqnum(), iv_tx_state));
   abort();
   return lv_error;
}


// --------------------------------------------------------------
// redriverollback_txn
// --------------------------------------------------------------
bool CTmTxBase::redriverollback_txn(CTmTxMessage * pp_msg)
{
   bool lv_terminate = false;
   TMTrace (2, ("CTmTxBase::redriverollback_txn BASE CLASS!: ID %d.\n", seqnum()));
   abort();
   return lv_terminate;
}

