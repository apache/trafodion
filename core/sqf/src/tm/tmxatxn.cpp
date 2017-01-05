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
#include "rm.h"
#include "tmtx.h"
#include "xatmmsg.h"
#include "xatmlib.h"
#include "seabed/trace.h"
#include "tmlogging.h"
#include "tmxatxn.h"
#include "tminfo.h"
#include "tmxidmap.h"

extern XID_MAP *gp_xarmXIDList;

// -----------------------------------------------------------------
// helper methods
// -----------------------------------------------------------------

// -------------------------------------
// CTmXaTxn Methods
// -------------------------------------

// -------------------------------------
// CTmXaTxn constructor
// Purpose : calls initialize
// -------------------------------------
CTmXaTxn::CTmXaTxn(int32 pv_nid, int64 pv_flags, int32 pv_trace_level, 
                       int32 pv_seq, int32 pv_pid, int32 pv_rm_wait_time)
{
   //EID(EID_TM_TX_INFO);
   // Initialize iv_trace_level and iv_lock_count here because they're used in calls 
   // to lock().
   iv_trace_level = pv_trace_level;
   // Mutex attributes: Recursive = true, ErrorCheck=false
   ip_mutex = new TM_Mutex(true, false);
   iv_ender_nid = pv_nid; //TMs nid is the default for the ender nid.
   ip_timeoutEvent = NULL;
   ip_hung_event = NULL;
   ip_Thread = NULL;
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

   ip_branches = new CTmTxBranches();
   
   // Initialize separately from TM_Info::new_tx as we don't have the creator nid,pid when called by
   // the tmPool constructor.
   // this->initialize(pv_nid, pv_flags, pv_trace_level, pv_seq, pv_pid, pv_rm_wait_time);
}

// -------------------------------------
// CTmXaTxn destructor
// Purpose : 
// -------------------------------------
CTmXaTxn::~CTmXaTxn()
{
   TMTrace (2, ("CTmXaTxn::~CTmXaTxn : ENTRY.\n"));
}


//----------------------------------------------------------------------------
// CTmXaTxn::constructPoolElement
// Purpose : Callback for CTmPool elements.
// This method is called to construct a CTmXaTxn object by CTmPool::newElement.
//----------------------------------------------------------------------------
CTmXaTxn * CTmXaTxn::constructPoolElement(int64 pv_id)
{
   CTmTxKey k(pv_id);

   TMTrace (2, ("CTmXaTxn::constructPoolElement : ENTRY Instantiating new transaction object, ID (%d,%d).\n", 
                k.node(), k.seqnum()));

   CTmXaTxn *lp_tx = new CTmXaTxn(k.node(), 0, gv_tm_info.iv_trace_level, 
                                  k.seqnum(), 0, gv_tm_info.rm_wait_time());
   if (!lp_tx)
   {
      tm_log_event(DTM_LOGIC_ERROR_TX_OBJ, SQ_LOG_CRIT, "DTM_LOGIC_ERROR_TX_OBJ");
      TMTrace (1, ("CTmXaTxn::constructPoolElement :  Failed to instantiate transaction object ID (%d,%d)\n",
               k.node(), k.seqnum()));
      abort();
   }

   TMTrace (2, ("CTmXaTxn::constructPoolElement : EXIT transaction object %p, ID (%d,%d) instantiated.\n", 
                (void *) lp_tx, k.node(), k.seqnum()));
   return lp_tx;
} //CTmXaTxn::constructPoolElement


// ---------------------------------------------------------
// cleanup
// Purpose : Prepare transaction object for reuse.
// This is called cleanPoolElement.
// ---------------------------------------------------------
void CTmXaTxn::cleanup()
{
   TMTrace (2, ("CTmXaTxn::cleanup : ENTRY transaction object %p, ID (%d,%d).\n", 
                (void *) this, node(), seqnum()));
   lock();
   iv_txnType = TM_TX_TYPE_XARM;
   iv_txnObj.ip_xaTxn = this;
   iv_rmid = -1;
   memset(&iv_XID, 0, sizeof(iv_XID));
   iv_tx_state = TM_TX_STATE_NOTX;
   iv_in_use = false;
   
   if (ip_timeoutEvent != NULL)
   {
      gv_xaTM.tmTimer()->cancelEvent(ip_timeoutEvent);
      ip_timeoutEvent = NULL;
   }
   if (num_active_partic())
      cleanup_app_partic();
   unlock();
}


// -------------------------------------
// initialize
// Purpose : Initialize this instance
// -------------------------------------
void CTmXaTxn::initialize(int32 pv_nid, int64 pv_flags, int32 pv_trace_level, 
                            int32 pv_seq, int32 pv_creator_nid, int32 pv_creator_pid, 
                            int32 pv_rm_wait_time)
{
   TMTrace(2, ("CTmXaTxn::initialize : ENTRY ID (%d,%d).\n",pv_nid, pv_seq));

   // transid setup
   lock();
   iv_txnType = TM_TX_TYPE_XARM;
   iv_txnObj.ip_xaTxn = this;
   iv_rmid = -1;
   memset(&iv_XID, 0, sizeof(iv_XID));

   CTmTxBase::initialize(pv_nid, pv_flags, pv_trace_level, pv_seq, pv_creator_nid, 
                         pv_creator_pid, pv_rm_wait_time);
   unlock();
} // initialize



// ---------------------------------------------------------------
// Transactional State Machine and helper
// ---------------------------------------------------------------

TM_TX_STATE CTmXaTxn::state_change_abort_helper(CTmTxMessage * pp_msg)
{
    int32 lv_error = XA_OK;
    TM_TX_STATE lv_nextState = tx_state();

    if (iv_wrote_trans_state == false)
    {
        gv_tm_info.write_trans_state(&iv_transid, TM_TX_STATE_ABORTED, abort_flags(), true);
        iv_wrote_trans_state = true;
    }
    lv_error = branches()->rollback_branches(this, TT_flags(), pp_msg, (iv_tm_aborted | iv_tse_aborted));

    switch (lv_error)
    {
         case XA_OK:
         {
               // Set the state to aborted now to allow SQL to continue
               lv_nextState = TM_TX_STATE_ABORTED;
               break;
          }                
          case XA_HEURHAZ:
          case XA_HEURCOM:
          case XA_HEURRB:
          case XA_HEURMIX:
          {
              // Got heuristic results from some RMs.
              // Send xa_forget to those RMs.
              // We set iv_partic to false for all branches that completed ok.
              lv_nextState = TM_TX_STATE_FORGETTING_HEUR;
              break;
            }
          case XAER_RMFAIL:
             {
               // At least one RM responded XAER_RMFAIL, failed to respond, or we
               // encountered an unrecoverable Seabed error.
               /*bug 1823 1/13/11: Commented out event as this event is written every time
                  we redrive rollback and can cause problems when a TSE is down.
               tm_log_event(DTM_TMTX_TX_HUNGABORTED, SQ_LOG_WARNING, "DTM_TMTX_TX_HUNGABORTED",
                            -1,-1,-1,seqnum());*/
               TMTrace (1, ("CTmXaTxn::state_change_abort_helper - At least one RM failed to "
                            " respond or connection lost. "
                            "Transaction %d in aborted state placed in hungAborted state.\n",
                            seqnum()));
               lv_nextState = TM_TX_STATE_HUNGABORTED;
               gv_tm_info.inc_tx_hung_count();
               //reset_transactionBusy();
               // Set up retry 
               addHungTimerEvent(TM_MSG_TXINTERNAL_REDRIVEROLLBACK);
               break;                      
           }
           default:  // some unexpected error
           {                
               tm_log_event(DTM_TMTX_TX_HUNGABORTED_ERROR, SQ_LOG_WARNING, "DTM_TMTX_TX_HUNGABORTED_ERROR", 
                            lv_error);
               TMTrace (1, ("CTmXaTxn::state_change_abort_helper - Unexpected XA error %d "
                            " returned by xa_rollback for aborted branch. "
                            "Transaction %d placed in hungAborted state.\n",
                            lv_error, seqnum()));
               lv_nextState = TM_TX_STATE_HUNGABORTED;
               gv_tm_info.inc_tx_hung_count();
               //reset_transactionBusy();
               // Set up retry 
               addHungTimerEvent(TM_MSG_TXINTERNAL_REDRIVEROLLBACK);
               break;                      
            }
    } // switch (lv_error)

    return lv_nextState;
} //CTmXaTxn::state_change_abort_helper


// state_change_commit_helper
// Purpose : Process xa_commit.
TM_TX_STATE CTmXaTxn::state_change_commit_helper (CTmTxMessage * pp_msg, bool pv_read_only)
{
    int32 lv_error = XA_OK;
    TM_TX_STATE lv_nextState = tx_state();

    if (pv_read_only)
    {
         if (pp_msg->replyPending()) 
              pp_msg->reply(lv_error);
         // Don't write committed state record for read-only transactions
         // TM_Info::write_trans_state (&iv_transid, TM_TX_STATE_COMMITTED, abort_flags, true);
         // iv_wrote_trans_state = true;
         TMTrace(3, ("state_change_commit_helper, read only, skipping commit record\n"));
         lv_nextState = TM_TX_STATE_FORGETTING;
         return lv_nextState;
    }

    if (iv_wrote_trans_state == false)
    {
        gv_tm_info.write_trans_state(&iv_transid, TM_TX_STATE_COMMITTED, abort_flags(), true);
        iv_wrote_trans_state = true;
    }
    lv_error = branches()->commit_branches(this, TT_flags(), pp_msg); 
    switch (lv_error)
    {
         case XA_OK:
         {
              lv_nextState = TM_TX_STATE_FORGETTING;
              break;
          }                
          case XA_HEURHAZ:
          case XA_HEURCOM:
          case XA_HEURRB:
          case XA_HEURMIX:
          {
               // Got heuristic results from some RMs.
               // Send xa_forget to those RMs.
               // We set iv_partic to false for all branches that completed
               // ok.
               lv_nextState = TM_TX_STATE_FORGETTING_HEUR;
               TMTrace (1, ("CTmXaTxn::state_change_commit_helper: Commit completed heuristically with XAER %d for "  
                            "transaction ID (%d,%d).\n",
                            lv_error, node(), seqnum()));
               break;
           }
           case XAER_RMFAIL:
           {
               // At least one RM responded XAER_RMFAIL, failed to respond, or we
               // encountered an unrecoverable Seabed error.
               tm_log_event(DTM_TMTX_TX_HUNGCOMMITTED, SQ_LOG_WARNING, "DTM_TMTX_TX_HUNGCOMMITTED",
                            -1,-1,-1,seqnum());
               TMTrace (1, ("CTmXaTxn::state_change_commit_helper - At least one RM failed to "
                            " respond or connection lost. "
                            "Transaction ID (%d,%d) in committed state placed in hungCommitted state.\n",
                            node(), seqnum()));
               lv_nextState = TM_TX_STATE_HUNGCOMMITTED;
               gv_tm_info.inc_tx_hung_count();
               reset_transactionBusy();
               // Set up retry 
               addHungTimerEvent(TM_MSG_TXINTERNAL_REDRIVECOMMIT);
               break;                      
           }
           default:  // some unexpected error
           {                
               tm_log_event(DTM_TMTX_TX_HUNGCOMMITTED_ERROR, SQ_LOG_WARNING, "DTM_TMTX_TX_HUNGCOMMITTED_ERROR", lv_error);
               TMTrace (1, ("CTmXaTxn::state_change_commit_helper - Unexpected XA error %d "
                            " returned by xa_commit for committed branch. "
                            "Transaction %d placed in hungCommitted state.\n",
                            lv_error, seqnum()));
               lv_nextState = TM_TX_STATE_HUNGCOMMITTED;
               gv_tm_info.inc_tx_hung_count();
               reset_transactionBusy();
               // Set up retry 
               addHungTimerEvent(TM_MSG_TXINTERNAL_REDRIVECOMMIT);
               break;                      
            }
      } // switch

      return lv_nextState;
} //state_change_commit_helper


// state_change_prepare_helper
// Purpose : performs the work associated with xa_prepare
// Returns an XA error
TM_TX_STATE CTmXaTxn::state_change_prepare_helper(CTmTxMessage * pp_msg)
{
   TM_TX_STATE lv_nextState = tx_state();
   int32 lv_error = mapErr(branches()->prepare_branches(this, TT_flags(), pp_msg));
   if (iv_mark_for_rollback == true)
   {
      pp_msg->responseError(FEABORTEDTRANSID);
      lv_nextState = TM_TX_STATE_ABORTING;
      iv_tse_aborted = true;
      gv_tm_info.inc_tm_initiated_aborts();
      return lv_nextState;
   }
    
   // need to save error!
   switch (lv_error)
   {
   case XA_OK:
      lv_nextState = TM_TX_STATE_PREPARED;
#ifdef DEBUG_MODE
          bool lv_assert;
          lv_assert = false;
          ms_getenv_bool("TM_TEST_AFTER_PREPARE_ASSERT", &lv_assert);
          if (lv_assert)
          {
                tm_log_event(TM_TEST_XA_OK_ASSERT, SQ_LOG_CRIT, "TM_TEST_XA_OK_ASSERT");
                TMTrace (1, ("CTmXaTxn::state_change_prepare_helper - TM test assert after XA_OK state\n"));
                abort ();
          }          
#endif 
      break;
   case XAER_RMFAIL:
      lv_nextState = state_change_abort_set(pp_msg, XAER_RMFAIL);
      break;
   default:
   // All other errors
      lv_nextState = state_change_abort_set(pp_msg, XAER_RMFAIL);
      break;
   }
   return lv_nextState;   
} // state_change_prepare_helper

TM_TX_STATE CTmXaTxn::state_change_abort_set(CTmTxMessage * pp_msg, short pv_error)
{
   TM_TX_STATE lv_nextState = TM_TX_STATE_ABORTING;

   iv_tm_aborted = true;
   gv_tm_info.inc_tm_initiated_aborts();
   // Reply now as we know the outcome and don't want to hold up the client
   if (pp_msg->replyPending())
      pp_msg->reply(pv_error);

   return lv_nextState;
}


// state_change_forget_helper
// Purpose : Process xa_forget.
TM_TX_STATE CTmXaTxn::state_change_forget_helper (CTmTxMessage * pp_msg)
{
    int32 lv_error = XA_OK;
    TM_TX_STATE lv_nextState = tx_state();

    if (iv_wrote_trans_state == false)
    {
        gv_tm_info.write_trans_state(&iv_transid, TM_TX_STATE_FORGOTTEN, abort_flags(), true);
        iv_wrote_trans_state = true;
    }
    lv_error = branches()->forget_heur_branches(this, TT_flags()); 
    switch (lv_error)
    {
         case XA_OK:
         {
              lv_nextState = TM_TX_STATE_FORGOTTEN_HEUR;
              break;
         }                
         case XAER_RMFAIL:
         {
            // At least one RM responded XAER_RMFAIL, failed to respond, or we
            // encountered an unrecoverable Seabed error.
            tm_log_event(DTM_TMTX_TX_HUNGCOMMITTED, SQ_LOG_WARNING, "DTM_TMTX_TX_HUNGCOMMITTED",
                           -1,-1,-1,seqnum());
            TMTrace (1, ("CTmXaTxn::state_change_forget_helper - At least one RM failed to "
                           " respond or connection lost. "
                           "Transaction ID (%d,%d) in forget state placed in hungCommitted state.\n",
                           node(), seqnum()));
            lv_nextState = TM_TX_STATE_HUNGCOMMITTED;
            gv_tm_info.inc_tx_hung_count();
            reset_transactionBusy();
            // Set up retry 
            addHungTimerEvent(TM_MSG_TXINTERNAL_REDRIVECOMMIT);
            break;                      
         }
         default:  // some unexpected error
         {                
            tm_log_event(DTM_TMTX_TX_HUNGCOMMITTED_ERROR, SQ_LOG_WARNING, "DTM_TMTX_TX_HUNGCOMMITTED_ERROR", lv_error);
            TMTrace (1, ("CTmXaTxn::state_change_forget_helper - Unexpected XA error %d "
                           " returned by xa_forget for forgetting branch. "
                           "Transaction %d placed in hungCommitted state.\n",
                           lv_error, seqnum()));
            lv_nextState = TM_TX_STATE_HUNGCOMMITTED;
            gv_tm_info.inc_tx_hung_count();
            reset_transactionBusy();
            // Set up retry 
            addHungTimerEvent(TM_MSG_TXINTERNAL_REDRIVECOMMIT);
            break;                      
         }
      } // switch

      return lv_nextState;
} //state_change_forget_helper



// ---------------------------------------------------------------------------
// CTmXaTxn::state_change
// Purpose : This is the Finite State Machine implementation
// for XARM transactions.
// ---------------------------------------------------------------------------
int32 CTmXaTxn::state_change (TX_EVENT pv_event, CTmTxMessage *pp_msg)
{
    bool       lv_continue = true;
    int32      lv_error = XA_OK;

    TMTrace (2, ("CTmXaTxn::state_change ENTRY, XA Txn ID (%d,%d) rmid " PFLL ", "
             "XID %s, current state %d, new event %d\n", 
             node(), seqnum(), rmid(), XIDtoa(xid()), (int) tx_state(), (int) pv_event));
    while (lv_continue)
    {
        switch (pv_event)
        {
        case TX_BEGIN:
         {
            switch (iv_tx_state)
            {
            case TM_TX_STATE_NOTX:
            case TM_TX_STATE_BEGINNING:
               // Not a lot to do here - we assume the TSEs will enlist via ax_reg.
               TM_TEST_PAUSE(iv_tx_state);
               tx_state(TM_TX_STATE_ACTIVE);
               lv_continue = false;
               break;
            default:
               lv_error = XAER_PROTO;
               tm_log_event(TM_TMTX_BRCH_STRT_UKN_STATE, SQ_LOG_CRIT, "TM_TMTX_BRCH_STRT_UKN_STATE",
                            -1,rmid(),node(),seqnum());
               TMTrace(1, ("CTmXaTxn::state_change - Branch event %d invalid for XA Txn ID (%d,%d) in state %d.\n",
                       pv_event, node(), seqnum(), tx_state()));
               lv_continue = false;
            }
            break;
         }
        case TX_SUSPEND: //TODO Implement XA suspend/resume/join
        case TX_JOIN:
        case TX_END:
         {
            switch (iv_tx_state)
            {
            case TM_TX_STATE_ACTIVE:
               TM_TEST_PAUSE(iv_tx_state);
               tx_state(TM_TX_STATE_IDLE);
               lv_continue = false;
               break;
            default:
               lv_error = XAER_PROTO;
               tm_log_event(TM_TMTX_BRCH_STRT_UKN_STATE, SQ_LOG_CRIT, "TM_TMTX_BRCH_STRT_UKN_STATE",
                            -1,rmid(),node(),seqnum());
               TMTrace(1, ("CTmXaTxn::state_change - Branch event %d invalid for XA Txn ID (%d,%d) in state %d.\n",
                       pv_event, node(), seqnum(), tx_state()));
               lv_continue = false;
            }
            break;
         }
        case TX_PREPARE:
         {
            switch (iv_tx_state)
            {
            case TM_TX_STATE_ACTIVE: //Accept for now - shouldn't be legal
            case TM_TX_STATE_IDLE:
               TM_TEST_PAUSE(iv_tx_state);
               // Send out prepares
               tx_state(TM_TX_STATE_PREPARING);
               tx_state(state_change_prepare_helper(pp_msg));
               if (tx_state() == TM_TX_STATE_PREPARED)
                  lv_continue = false;
               break;
            default:
               lv_error = XAER_PROTO;
               tm_log_event(TM_TMTX_BRCH_STRT_UKN_STATE, SQ_LOG_CRIT, "TM_TMTX_BRCH_STRT_UKN_STATE",
                            -1,rmid(),node(),seqnum());
               TMTrace(1, ("CTmXaTxn::state_change - Branch event %d invalid for XA Txn ID (%d,%d) in state %d.\n",
                       pv_event, node(), seqnum(), tx_state()));
               lv_continue = false;
            }
            break;
         }
        case TX_COMMIT:
         {
            switch (iv_tx_state)
            {
            case TM_TX_STATE_PREPARED:
            case TM_TX_STATE_HUNGCOMMITTED:
               TM_TEST_PAUSE(iv_tx_state);
               // Send out commits
               tx_state(TM_TX_STATE_COMMITTING);
               tx_state(state_change_commit_helper(pp_msg));
               if (tx_state() == TM_TX_STATE_FORGETTING ||
                   tx_state() == TM_TX_STATE_FORGETTING_HEUR ||
                   tx_state() == TM_TX_STATE_HUNGCOMMITTED ||
                   tx_state() == TM_TX_STATE_HUNGABORTED)
                  lv_continue = false;
               break;
            default:
               lv_error = XAER_PROTO;
               tm_log_event(TM_TMTX_BRCH_STRT_UKN_STATE, SQ_LOG_CRIT, "TM_TMTX_BRCH_STRT_UKN_STATE",
                            -1,rmid(),node(),seqnum());
               TMTrace(1, ("CTmXaTxn::state_change - Branch event %d invalid for XA Txn ID (%d,%d) in state %d.\n",
                       pv_event, node(), seqnum(), tx_state()));
               lv_continue = false;
            }
            break;
         }
        case TX_ROLLBACK:
         {
            switch (iv_tx_state)
            {
            case TM_TX_STATE_NOTX:
            case TM_TX_STATE_BEGINNING:
            case TM_TX_STATE_ACTIVE:
            case TM_TX_STATE_IDLE:
            case TM_TX_STATE_PREPARING:
            case TM_TX_STATE_PREPARED:
            case TM_TX_STATE_ABORTING:
            case TM_TX_STATE_HUNGABORTED:
               // Send out rollbacks
               tx_state(TM_TX_STATE_ABORTING_PART2);
               tx_state(state_change_abort_helper(pp_msg));
               lv_continue = false;
               break;
            default:
               lv_error = XAER_PROTO;
               tm_log_event(TM_TMTX_BRCH_STRT_UKN_STATE, SQ_LOG_CRIT, "TM_TMTX_BRCH_STRT_UKN_STATE",
                            -1,rmid(),node(),seqnum());
               TMTrace(1, ("CTmXaTxn::state_change - Branch event %d invalid for XA Txn ID (%d,%d) in state %d.\n",
                       pv_event, node(), seqnum(), tx_state()));
               lv_continue = false;
            }
            break;
         }
        case TX_FORGET:
         {
            switch (iv_tx_state)
            {
            case TM_TX_STATE_FORGETTING:
            case TM_TX_STATE_ABORTED:
               // Normal commit path
               pv_event = TX_TERMINATE;
               lv_continue = true;
               break;
            case TM_TX_STATE_COMMITTING:
            case TM_TX_STATE_COMMITTED:
            case TM_TX_STATE_ABORTING:
            case TM_TX_STATE_ABORTING_PART2:
               // Send out forget
               tx_state(TM_TX_STATE_FORGETTING_HEUR);
               tx_state(state_change_forget_helper(pp_msg));
               lv_continue = false;
               break;
            default:
               lv_error = XAER_PROTO;
               tm_log_event(TM_TMTX_BRCH_STRT_UKN_STATE, SQ_LOG_CRIT, "TM_TMTX_BRCH_STRT_UKN_STATE",
                            -1,rmid(),node(),seqnum());
               TMTrace(1, ("CTmXaTxn::state_change - Branch event %d invalid for XA Txn ID (%d,%d) in state %d.\n",
                       pv_event, node(), seqnum(), tx_state()));
               lv_continue = false;
            }
            break;
         }
        case TX_TERMINATE:
         {
            tx_state(TM_TX_STATE_TERMINATING);
            lv_continue = false;
            break;
         }

       // we get this when we simply need to resent the commit
       // at recovery time
        case TX_REDRIVE_COMMIT:
         {
            TM_TEST_PAUSE(iv_tx_state);
            // we've already written the trans state record if were are here
            tx_state(TM_TX_STATE_COMMITTED);  
            pv_event = TX_COMMIT;
            break;
         } // case TX_REDRIVE_COMMIT
        case TX_REDRIVE_ROLLBACK:
         {
            TM_TEST_PAUSE(iv_tx_state);
            // we've already written the trans state record if were are here
            tx_state(TM_TX_STATE_ABORTING_PART2);
            pv_event = TX_ROLLBACK;
            break;
         }
        default :
            tm_log_event(TM_TMTX_INVALID_STATE, SQ_LOG_CRIT, "TM_TMTX_INVALID_STATE",
                         -1,rmid(),node(),seqnum());
            TMTrace (1, ("CTmXaTxn::state_change - tx state %d: "
                         "Invalid transaction state. Transaction seq num %d\n",pv_event, seqnum()));
            abort();

        }; //switch on event
    } // while continue
  
    TM_TEST_PAUSE_NEXT();
    TMTrace (2, ("CTmXaTxn::state_change EXIT : txn ID %d, state %d, returning error %d.\n", 
                 seqnum(), iv_tx_state, lv_error));
    return lv_error;
} // state_change


// --------------------------------------------------------------
// schedule_abort
// This executes under the main thread, not under the transaction
// thread.  If the transaction has already reached Phase 2, an
// error is returned. Otherwise a rollback will take place.
// If the transaction thread is not already processing an 
// ENDTRANSACTION or ABORTTRANSACTION, then an abort event will
// be queued to the transaction thread.  If the transaction thread
// is already processing a request, the transaction is marked for 
// rollback and the already executing transaction thread will 
// take care of the rollback.
// Return codes:
//    FEOK              Successful.
//    FEENDEDTRANSID    Transaction is already in Phase 2.
// --------------------------------------------------------------
int32 CTmXaTxn::schedule_abort()
{
   int32 lv_error = FEOK;

   TMTrace (2, ("CTmXaTxn::schedule_abort ENTRY, Txn ID (%d,%d), state %d.\n",
                  node(), seqnum(), iv_tx_state));

   if ((tx_state() != TM_TX_STATE_NOTX) && // beginning
      (tx_state() != TM_TX_STATE_BEGINNING) &&   // beginning part 2
      (tx_state() != TM_TX_STATE_ACTIVE) &&   // active
      (tx_state() != TM_TX_STATE_PREPARING)) // about to commit
      lv_error = FEENDEDTRANSID;
   else
   {
      internal_abortTrans(true);
   }

   TMTrace (2, ("CTmXaTxn::schedule_abort, EXIT, ID %d, error %d, state %d, %s.\n",
                   seqnum(), lv_error, tx_state(), ((iv_mark_for_rollback)?"rollback deferred":"rolled back")));

   return lv_error;
} //schedule_abort


// -------------------------------------------------------------
// schedule_eventQ
// -------------------------------------------------------------
void CTmXaTxn::schedule_eventQ()
{
   short lv_request_type = TM_MSG_TYPE_NULL;

   TMTrace (2, ("CTmXaTxn::schedule_eventQ ENTER ID %d, state %d.\n",
                   iv_tag, iv_tx_state));

   switch (iv_tx_state)
   {
      case TM_TX_STATE_BEGINNING:
      {
         lv_request_type = TM_MSG_TXINTERNAL_BEGINCOMPLETE;
         break;
      }
      case TM_TX_STATE_COMMITTED:
      {
         lv_request_type = TM_MSG_TXINTERNAL_ENDCOMPLETE;
         break;
      }
      case TM_TX_STATE_ABORTED:
      {
         lv_request_type = TM_MSG_TXINTERNAL_ABORTCOMPLETE;
         break;
      }
      case TM_TX_STATE_FORGOTTEN:
      {
         lv_request_type = TM_MSG_TXINTERNAL_ENDFORGET;
         break;
      }
      default:
      {
         tm_log_event(TM_TMTX_INVALID_STATE, SQ_LOG_CRIT, "TM_TMTX_INVALID_STATE",
                      -1,-1,-1,seqnum());
         TMTrace (1, ("CTmXaTxn::schedule_eventQ - tx state %d: "
                      "Invalid transaction state. Transaction seq num %d\n",iv_tx_state, seqnum()));
         abort();
      }
   };

   // We use the saved request here to drive the event.
   // This is because this is a continuance of the existing
   // request and we want this to be used for any reply as
   // it contains the msgid of the orignal request (eg
   // ENDTRANSACTION request).
   if (!ip_currRequest)
   {
      tm_log_event(DTM_TMTX_INVALID_POINTER, SQ_LOG_CRIT, "DTM_TMTX_INVALID_POINTER",
                    -1,-1,-1,seqnum());
      TMTrace (1, ("CTmXaTxn::schedule_eventQ - PROGRAMMING BUG!: "
                   " ip_currRequest was null when we received a sync "
                   "completion for transaction %d.\n", seqnum()));
      abort();
   }
   else
   {
      ip_currRequest->requestType(lv_request_type);
      eventQ_push(ip_currRequest);
   }

   TMTrace (2, ("CTmXaTxn::schedule_eventQ EXIT ID %d, request_type %d.\n",
                   iv_tag, lv_request_type));
}


// --------------------------------------------------------------
// process_eventQ
// Purpose - process all events on the event queue for this 
// transaction.
// For multithreaded TMs this is the main processing loop for
// the transaction.
// --------------------------------------------------------------
void CTmXaTxn::process_eventQ()
{
   bool lv_exit = false;
   CTmEvent *lp_event = NULL;

   TMTrace (2, ("CTmXaTxn::process_eventQ : ENTRY ID %d, thread %s.\n",
                   iv_tag, ((gv_tm_info.multithreaded())?ip_Thread->get_name():"SINGLE THREAD")));

   // Pop/wait for a transaction event
   CTmTxMessage *lp_msg = eventQ_pop();
   lp_msg->validate();

   while (!lv_exit && lp_msg->request())
   {
      switch (lp_msg->requestType())
      {
      case TM_DP2_SQ_XA_START:
         req_xa_notImplemented(lp_msg);
         break;
      case TM_DP2_SQ_XA_END:
         lv_exit = req_xa_end(lp_msg);
         break;
      case TM_DP2_SQ_XA_COMMIT:
         lv_exit = req_xa_commit(lp_msg);
         break;
      case TM_DP2_SQ_XA_PREPARE:
        lv_exit =  req_xa_prepare(lp_msg);
         break;
      case TM_DP2_SQ_XA_ROLLBACK:
         lv_exit = req_xa_rollback(lp_msg);
         break;
      case TM_DP2_SQ_XA_OPEN:
         req_xa_notImplemented(lp_msg);
         break;
      case TM_DP2_SQ_XA_CLOSE:
         req_xa_notImplemented(lp_msg);
         break;
      case TM_DP2_SQ_XA_RECOVER:
         req_xa_notImplemented(lp_msg);
         break;
      case TM_DP2_SQ_XA_FORGET:
         lv_exit = req_xa_forget(lp_msg);
         break;
      case TM_DP2_SQ_XA_COMPLETE:
         req_xa_notImplemented(lp_msg);
         break;
      case TM_DP2_SQ_AX_REG:
         req_xa_notImplemented(lp_msg);
         break;
      case TM_DP2_SQ_AX_UNREG:
         req_xa_notImplemented(lp_msg);
         break;

      case TM_MSG_TYPE_BEGINTRANSACTION:
         TMTrace (1, ("CTmXaTxn::process_eventQ : ERROR - BEGINTRANSACTION for XARM subordinate branch.\n"));
         req_xa_notImplemented(lp_msg);
         break;
      case TM_MSG_TYPE_ENDTRANSACTION:
         lv_exit = req_end(lp_msg);
         break;
      case TM_MSG_TYPE_ABORTTRANSACTION:
      case TM_MSG_TYPE_TSE_DOOMTX:
      case TM_MSG_TYPE_DOOMTX:
         lv_exit = req_abort(lp_msg);
         break;
      case TM_MSG_TYPE_STATUSTRANSACTION:
         // These aren't queued, so we should never hit this
         lv_exit = req_status(lp_msg);
         break;
      case TM_MSG_TYPE_AX_REG:
         lv_exit = req_ax_reg(lp_msg);
         break;
      case TM_MSG_TYPE_JOINTRANSACTION:
         // These aren't queued, so we should never hit this
         lv_exit = req_join(lp_msg);
         break;
      case TM_MSG_TYPE_SUSPENDTRANSACTION:
         // These aren't queued, so we should never hit this
         lv_exit = req_suspend(lp_msg);
         break;
      case TM_MSG_TXINTERNAL_ROLLBACK:
      {
         lv_exit = rollback_txn(lp_msg);
         if ((gv_tm_info.mode() == TM_NONSYNC_MODE) && (!lv_exit))
         {
             if (transactionBusy())
                req_abort_complete(lp_msg);
             lv_exit = req_forget(lp_msg);  
         }
         break;
      }
      case TM_MSG_TXINTERNAL_REDRIVEROLLBACK:
         lv_exit = redriverollback_txn(lp_msg);
         break;
      case TM_MSG_TXINTERNAL_REDRIVECOMMIT:
         lv_exit = redrivecommit_txn(lp_msg);
         break;
      case TM_MSG_TXINTERNAL_ABORTCOMPLETE:
         lv_exit = req_abort_complete(lp_msg);
         break;      
      case TM_MSG_TXINTERNAL_ENDCOMPLETE:
         lv_exit = req_end_complete(lp_msg);
         break;
      case TM_MSG_TXINTERNAL_ENDFORGET:
         lv_exit = req_forget(lp_msg);
         break;
      case TM_MSG_TXINTERNAL_BEGINCOMPLETE:
         req_xa_notImplemented(lp_msg);
         break;
      case TM_MSG_TXTHREAD_TERMINATE:
         // Need to re-queue the Terminate to the thread event queue
         lp_event = new CTmEvent(lp_msg);
         ip_Thread->eventQ_push(lp_event);
         // Intentional fall through.
      case TM_MSG_TXTHREAD_RELEASE:
         lv_exit = true;
         break;
      case TM_MSG_TXINTERNAL_REDRIVESYNC:
      default:
         // EMS message here, DTM_INVALID_MESSAGE_TYPE
         tm_log_event(DTM_INVALID_MESSAGE_TYPE, SQ_LOG_CRIT, "DTM_INVALID_MESSAGE_TYPE",
                    -1, /*error_code*/ 
                    -1, /*rmid*/
                    gv_tm_info.nid(), /*dtmid*/ 
                    -1, /*seq_num*/
                    -1, /*msgid*/
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
                    -1, /*node*/
                    -1, /*msgid2*/
                    -1, /*offset*/
                     lp_msg->requestType());
                
         //tm_log_event(DTM_INVALID_MESSAGE_TYPE, SQ_LOG_CRIT , "DTM_INVALID_MESSAGE_TYPE");
         TMTrace (1, ("CTmXaTxn::process_eventQ - DTM%d txThread received UNKNOWN message type : %d\n",
                 gv_tm_info.nid(), lp_msg->requestType()));
         abort (); // bogus type
      } // switch

      lock();
      delete lp_msg;
      unlock();
      // Multithreaded only:
      // Worker threads release the transaction after every request
      // unless there are more requests to process for this transaction.
      if (gv_tm_info.threadModel() == worker && iv_eventQ.empty())
      {  
         lv_exit = true;
         /* Not currently using the release event
         CTmEvent *lp_event = new CTmEvent(TM_MSG_TXTHREAD_RELEASE, ip_Thread);
         ip_Thread->eventQ_push(lp_event); */
      }
      if (!lv_exit)
      {
         // We treat thread events with higher priority, so first 
         // check to see if there are any.  If not, then check (wait) for
         // a transaction event.
         if (ip_Thread->eventQ()->empty())
         {
            TMTrace (3, ("CTmXaTxn::process_eventQ : Thread %s thread event queue empty, "
                         "checking transaction Q.\n", ip_Thread->get_name()));

            lp_msg = eventQ_pop();
         }
         else
         {
            TMTrace (3, ("CTmXaTxn::process_eventQ : Thread %s thread event detected, exiting proc.\n",
                           ip_Thread->get_name()));
            // Drop out of transaction processing loop and
            // handle the thread event
            lv_exit = true;
         }
      }
   } //while more events to process

   TMTrace (2, ("CTmXaTxn::process_eventQ : EXIT teminateThread=%d.\n",
            lv_exit));
} //process_eventQ


// --------------------------------------------------------------
// DTM Event Specific Methods:
// --------------------------------------------------------------
// --------------------------------------------------------------
// Purpose - Txn Thread specific processing for BEGINTRANSACTION
// NOT SUPPORTED FOR XARM TRANSACTIONS.
// --------------------------------------------------------------
bool CTmXaTxn::req_begin(CTmTxMessage * pp_msg)
{
   //short lv_error = FEOK;
   bool lv_terminate = false;

   TMTrace (2, ("CTmXaTxn::req_begin : Not implemented!\n"));
   abort();

   return lv_terminate;
} //req_end


// --------------------------------------------------------------
// Purpose - Txn Thread specific processing for ENDTRANSACTION.
// --------------------------------------------------------------
bool CTmXaTxn::req_end(CTmTxMessage * pp_msg)
{
   //short lv_error = FEOK;
   bool lv_terminate = false;

   TMTrace (2, ("CTmXaTxn::req_end : Not implemented!\n"));
   abort();

   return lv_terminate;
} //req_end


// --------------------------------------------------------------
// req_abort
// Purpose - Txn Thread specific processing for ABORTTRANSACTION
// and for TSE_DOOMTX.
// --------------------------------------------------------------
bool CTmXaTxn::req_abort(CTmTxMessage * pp_msg)
{   
   bool lv_terminateThread = false;
   //int32 lv_error = FEOK;
   int32 lv_nid = pp_msg->request()->u.iv_abort_trans.iv_nid,
         lv_pid = pp_msg->request()->u.iv_abort_trans.iv_pid;
    //int32 lv_rmid = gv_RMs.TSE()->return_rmid(lv_nid, lv_pid);

   TMTrace (2, ("CTmXaTxn::req_abort : ENTRY Txn ID (%d,%d), originating process (nid, pid): "
            "(%d, %d).\n", node(), seqnum(), lv_nid, lv_pid));

   

   TMTrace (2, ("CTmXaTxn::req_abort : ID %d, Not Implemented!\n", seqnum()));
   abort();
   return lv_terminateThread;
} //req_abort


// --------------------------------------------------------------
// req_abort_complete
// Purpose - Txn Thread specific processing for ABORTTRANSACTION.
//            phase 2 (the real work).
// --------------------------------------------------------------
bool CTmXaTxn::req_abort_complete(CTmTxMessage * pp_msg)
{
   TMTrace (2, ("CTmXaTxn::req_abort_complete : Txn ID (%d,%d), ENTRY\n", node(), seqnum()));
   pp_msg->validate();


   if (tx_state() != TM_TX_STATE_FORGETTING &&
       tx_state() != TM_TX_STATE_FORGOTTEN)
      state_change(TX_ROLLBACK, pp_msg);
   else
      TMTrace (3, ("CTmXaTxn:req_abort_complete : tx state is FORGETTING or "
                   "FORGOTTEN so nothing to do here, exiting.\n"));

   if (tx_state() != TM_TX_STATE_HUNGABORTED)
      stats()->txnAbort()->stop();

   pp_msg->validate();

   // If we haven't replied yet, do so now - before we forget the outcome!
   if (pp_msg->replyPending())
       pp_msg->reply(); // reply with whatever error has been set

   TMTrace (2, ("CTmXaTxn::req_abort_complete : ID %d, EXIT\n", seqnum()));

   return false; //Don't terminate transaction, still need to do forget

}


// --------------------------------------------------------------
// req_end_complete
// Purpose - Txn Thread specific processing for ENDTRANSACTION.
//            phase 2 (the real work).
// --------------------------------------------------------------
bool CTmXaTxn::req_end_complete(CTmTxMessage * pp_msg)
{
   TMTrace (2, ("CTmXaTxn::req_end_complete : ID %d, ENTRY\n", seqnum()));

   if (isAborting())
   {
      pp_msg->responseError(FEABORTEDTRANSID);
      state_change(TX_ROLLBACK, pp_msg);
   }
   else
      state_change(TX_COMMIT, pp_msg);

   if (tx_state() != TM_TX_STATE_HUNGCOMMITTED)
      stats()->txnCommit()->stop();

   TMTrace (2, ("CTmXaTxn::req_end_complete : ID %d, EXIT\n", seqnum()));

   return false; //Don't terminate transaction, still need to do forget
}


// --------------------------------------------------------------
// rollback_txn
// Purpose - Txn Thread specific processing for abort processing.
// --------------------------------------------------------------
bool CTmXaTxn::rollback_txn(CTmTxMessage * pp_msg)
{

   TMTrace (2, ("CTmXaTxn::rollback_txn : ID %d, ENTRY\n", 
                    seqnum()));
   pp_msg->validate();

   if ((tx_state() != TM_TX_STATE_BEGINNING) &&
       (tx_state() != TM_TX_STATE_ACTIVE) &&
       (tx_state() != TM_TX_STATE_ABORTING) &&
       (tx_state() != TM_TX_STATE_ABORTING_PART2) &&
       (tx_state() != TM_TX_STATE_ABORTED) &&
       (tx_state() != TM_TX_STATE_HUNGABORTED) &&
       (tx_state() != TM_TX_STATE_FORGETTING) &&
       (tx_state() != TM_TX_STATE_FORGOTTEN))
   {
         // EMS Message DTM_ABORT_FAILED
         tm_log_event(DTM_ABORT_FAILED, SQ_LOG_WARNING, "DTM_ABORT_FAILED",
                    -1, /*error_code*/ 
                    -1, /*rmid*/
                    gv_tm_info.nid(), /*dtmid*/ 
                    seqnum(), /*seq_num*/
                    -1, /*msgid*/
                    -1, /*xa_error*/
                    -1, /*pool_size*/
                    -1, /*pool_elems*/
                    -1, /*msg_retries*/
                    -1, /*pool_high*/
                    -1, /*pool_low*/
                    -1, /*pool_max*/
                     tx_state()); /*tx_state*/

         TMTrace (1, ("CTmXaTxn::rollback_txn : DTM%d txThread could not abort "
                      " transaction %d, state %d incorrect.\n",
                      gv_tm_info.nid(), seqnum(), (short) tx_state()));

         // If this rollback was caused by a call to ABORTTRANSACTION, reply now
         if (pp_msg->replyPending())
         {
            if (isAborting())
               pp_msg->reply(FEABORTEDTRANSID);
            else
               pp_msg->reply(FEENDEDTRANSID);
         }

         bool lv_txnBusy = reset_transactionBusy();
         if (!lv_txnBusy)
         {
            inc_cleanup();
            return false; // Allow thread to process outstanding requests
         }
         else
            return true; // Terminate txn thread
   }

   // we don't care about joiners here.  The TX has to go, so we don't even check

   state_change(TX_ROLLBACK, pp_msg);

   TMTrace (2, ("CTmXaTxn::rollback_txn, EXIT\n"));
   
   return false; //Don't terminate txn thread, still need to process the
                 //TM_MSG_TXINTERNAL_ABORTCOMPLETE.
} //rollback_txn


// --------------------------------------------------------------
// redriverollback_txn
// Purpose - Txn Thread specific processing to redrive a rollback.
// --------------------------------------------------------------
bool CTmXaTxn::redriverollback_txn(CTmTxMessage * pp_msg)
{
   bool lv_terminate = false;

   TMTrace (2, ("CTmXaTxn::redriverollback_txn : ID %d, ENTRY\n", seqnum()));

   if (iv_tx_state == TM_TX_STATE_HUNGABORTED)
      gv_tm_info.dec_tx_hung_count();

   // If we have a hung event timer then remove it now.
   if (ip_hung_event)
   {
      gv_xaTM.tmTimer()->cancelEvent(ip_hung_event);
      ip_hung_event = NULL;
   }

   // We've already driven phase 1, redrive phase 2.
   state_change(TX_REDRIVE_ROLLBACK, pp_msg);

   
   if (tx_state() != TM_TX_STATE_HUNGABORTED)
      stats()->txnAbort()->stop();

   if (gv_tm_info.mode() == TM_NONSYNC_MODE)
      lv_terminate = req_forget(pp_msg);

   TMTrace (2, ("CTmXaTxn::redriverollback_txn, EXIT returning %s\n", (lv_terminate)?"TRUE":"FALSE"));
   return lv_terminate;
} //redriverollback_txn


// --------------------------------------------------------------
// redrivecommit_txn
// Purpose - Txn Thread specific processing to redrive a commit.
// --------------------------------------------------------------
bool CTmXaTxn::redrivecommit_txn(CTmTxMessage * pp_msg)
{
   bool lv_terminate = false;

   TMTrace (2, ("CTmXaTxn::redrivecommit_txn : ID %d, ENTRY, current txn state %d\n", 
      seqnum(), tx_state()));

   if (iv_tx_state == TM_TX_STATE_HUNGCOMMITTED)
      gv_tm_info.dec_tx_hung_count();
   
   // If we have a hung event timer then remove it now.
   if (ip_hung_event)
   {
      gv_xaTM.tmTimer()->cancelEvent(ip_hung_event);
      ip_hung_event = NULL;
   }

   // We've already driven phase 1, redrive phase 2.
   state_change(TX_REDRIVE_COMMIT, pp_msg);

   if (tx_state() != TM_TX_STATE_HUNGCOMMITTED)
      stats()->txnCommit()->stop();

   if (gv_tm_info.mode() == TM_NONSYNC_MODE)
      lv_terminate = req_forget(pp_msg);

   TMTrace (2, ("CTmXaTxn::redrivecommit_txn, EXIT returning %s\n", (lv_terminate)?"TRUE":"FALSE"));
   return lv_terminate;
} //redrivecommit_txn


// --------------------------------------------------------------
// req_forget
// Purpose - Txn Thread specific processing for ENDTRANSACTION,
//           ABORTTRANSACTION and DOOMTRANSACTION.
//           Forget the transaction.
// --------------------------------------------------------------
bool CTmXaTxn::req_forget(CTmTxMessage * pp_msg)
{
   bool lv_terminate = true; // Normal processing is for forget to 
                             // drive thread disassociation for 
                             // transaction threads.
   TMTrace (2, ("CTmXaTxn::req_forget : ID %d, ENTRY\n", seqnum()));

   // If the transaction is in a hung state, we don't want to forget it.
   // We will wait for the commit/rollback to be retried, or for operator
   // intervention.
   if (tx_state() == TM_TX_STATE_HUNGCOMMITTED ||
       tx_state() == TM_TX_STATE_HUNGABORTED)
   {
      TMTrace (1, ("CTmXaTxn::req_forget : Ignoring transaction ID %d in hungAborted state.\n",
               seqnum()));
      lv_terminate = false; // Don't cleanup transaction object yet
   }
   else
   {
       if (tx_state() == TM_TX_STATE_FORGETTING_HEUR ||
           tx_state() == TM_TX_STATE_FORGOTTEN_HEUR)
       {
          TMTrace (1, ("CTmXaTxn::req_forget : Transaction ID %d in heuristicForgetting state, sending xa_forget.\n",
                   seqnum()));
          state_change(TX_FORGET, pp_msg);
       }


       gv_tm_info.remove_tx_from_oldest_list(this);
       // Gather up statistics
       stats()->txnTotal()->stop();
       gv_tm_info.stats()->addTxnCounters(stats());
   }

   // We want to reset the transactionBusy flag and cleanup the pendingQ
   // even if this is a hung transaction.
   bool lv_txnBusy = reset_transactionBusy(true /*cleanup pendingQ*/);

   // Cleanup now if we are terminating the transaction and we've finished
   // processing transaction requests.
   if (lv_terminate && !lv_txnBusy)
   {
      inc_cleanup();
   }
   else
      TMTrace (3, ("CTmXaTxn::req_forget : ID %d, terminate=%d, txnBusy=%d, "
               "pending requests, not cleaning up transaction.\n", 
               seqnum(), lv_terminate, lv_txnBusy));


   TMTrace (2, ("CTmXaTxn::req_forget : ID %d, txnBusy %d, terminate %d EXIT\n", 
      seqnum(), lv_txnBusy, lv_terminate));
   return lv_terminate; //Finished with the transaction, disassociate and cleanup
} // req_forget


// --------------------------------------------------------------
// doom_txn
// This executes under the main thread, not under the transaction
// thread.  If the transaction has already reached Phase 2, an
// error is returned. Otherwise a the transaction is marked as
// rollback required (iv_mark_for_rollback = true).  This will
// cause a rollback to occur once the ABORTTRANSACTION or
// ENDTRANSACTION arrives for the transaction.
// Return codes:
//    FEOK              Successful.
//    FEENDEDTRANSID    Transaction is already in Phase 2.
// --------------------------------------------------------------
short CTmXaTxn::doom_txn()
{
   short lv_error = FEOK;

   TMTrace (2, ("CTmXaTxn::doom_txn ENTRY, ID %d, state %d.\n",
                  seqnum(), iv_tx_state));

   if ((tx_state() != TM_TX_STATE_NOTX) && // beginning
      (tx_state() != TM_TX_STATE_BEGINNING) &&   // beginning part 2
      (tx_state() != TM_TX_STATE_ACTIVE) &&   // active
      (tx_state() != TM_TX_STATE_PREPARING)) // about to commit
      lv_error = FENOTRANSID;
   else
   {
      iv_mark_for_rollback = true;
      iv_appl_aborted = true;
   }

    TMTrace (2, ("CTmXaTxn::doom_txn, EXIT, ID %d, error %d, state %d, %s.\n",
                   seqnum(), lv_error, tx_state(), ((iv_mark_for_rollback)?"rollback deferred":"rolled back")));

   return lv_error;
} //doom_txn


// ---------------------------------------------------------------------------
// CTmXaTxn::req_xa_join
// Purpose: Support the TMJOIN flag in xa_start
// ---------------------------------------------------------------------------
bool CTmXaTxn::req_xa_join(CTmTxMessage *pp_msg)
{
   bool lv_terminateThread = false;
   int32 lv_rmid = pp_msg->request()->u.iv_start.iv_rmid;
   XID *lp_xid = &pp_msg->request()->u.iv_start.iv_xid;
   int64 lv_flags = pp_msg->request()->u.iv_start.iv_flags;

   TMTrace(2, ("CTmXaTxn::req_xa_join ENTRY, XA Txn ID (%d,%d), state %d, rmid %d, XID %s, flags " PFLL ".\n",
           node(), seqnum(), iv_tx_state, lv_rmid, XIDtoa(lp_xid), lv_flags));

   // Call join in-line in main thread
   req_join(pp_msg);

   TMTrace(2, ("CTmXaTxn::req_xa_join EXIT.\n"));
   return lv_terminateThread;
} // req_xa_join


// ---------------------------------------------------------------------------
// CTmXaTxn::req_xa_resume
// Purpose: Support the TMRESUME flag in xa_start
// ---------------------------------------------------------------------------
bool CTmXaTxn::req_xa_resume(CTmTxMessage *pp_msg)
{
   bool lv_terminateThread = false;
   int32 lv_rmid = pp_msg->request()->u.iv_start.iv_rmid;
   XID *lp_xid = &pp_msg->request()->u.iv_start.iv_xid;
   int64 lv_flags = pp_msg->request()->u.iv_start.iv_flags;

   TMTrace(2, ("CTmXaTxn::req_xa_resume ENTRY, XA Txn ID (%d,%d), rmid %d, XID %s, flags " PFLL ",state %d.\n",
           node(), seqnum(), lv_rmid, XIDtoa(lp_xid), lv_flags, iv_tx_state));

   // Call join in-line in main thread
   req_join(pp_msg); //TODO Treated as identical to join for prototype

   TMTrace(2, ("CTmXaTxn::req_xa_resume EXIT.\n"));
   return lv_terminateThread;
} // req_xa_resume


// ---------------------------------------------------------------------------
// CTmXaTxn::req_xa_end
// Purpose: Support xa_end
// ---------------------------------------------------------------------------
bool CTmXaTxn::req_xa_end(CTmTxMessage *pp_msg)
{
   bool lv_terminateThread = false;
   int32 lv_rmid = pp_msg->request()->u.iv_end.iv_rmid;
   XID *lp_xid = &pp_msg->request()->u.iv_end.iv_xid;
   int64 lv_flags = pp_msg->request()->u.iv_end.iv_flags;

   TMTrace(2, ("CTmXaTxn::req_xa_end ENTRY, XA Txn ID (%d,%d), state %d, rmid %d, XID %s, flags " PFLL ".\n",
           node(), seqnum(), iv_tx_state, lv_rmid,  XIDtoa(lp_xid), lv_flags));

   switch (lv_flags)
   {
   case TMMIGRATE:
      req_xa_notImplemented(pp_msg);
      break;
   case TMSUSPEND:
      req_suspend(pp_msg);
      break;
   case TMNOFLAGS:
   case TMSUCCESS:
      pp_msg->reply(state_change(TX_END, pp_msg));
      break;
   case TMFAIL:
      mark_for_rollback();
      pp_msg->reply(state_change(TX_END, pp_msg));
      break;
   case TMASYNC:
      req_xa_notImplemented(pp_msg);
      break;
   default:
      TMTrace(2, ("CTmXaTxn::req_xa_end flag unknown.\n"));
      req_xa_notImplemented(pp_msg);
   }

   TMTrace(2, ("CTmXaTxn::req_xa_end EXIT.\n"));
   return lv_terminateThread;
} // req_xa_end


// ---------------------------------------------------------------------------
// CTmXaTxn::req_xa_prepare
// Purpose: Support xa_prepare
// ---------------------------------------------------------------------------
bool CTmXaTxn::req_xa_prepare(CTmTxMessage *pp_msg)
{
   bool lv_terminateThread = false;
   int32 lv_rmid = pp_msg->request()->u.iv_prepare.iv_rmid;
   XID *lp_xid = &pp_msg->request()->u.iv_prepare.iv_xid;
   int64 lv_flags = pp_msg->request()->u.iv_prepare.iv_flags;

   TMTrace(2, ("CTmXaTxn::req_xa_prepare ENTRY, Txn ID (%d,%d), state %d, rmid %d, XID %s, flags " PFLL ".\n",
           node(), seqnum(), iv_tx_state, lv_rmid, XIDtoa(lp_xid), lv_flags));

   state_change(TX_PREPARE, pp_msg);
   pp_msg->reply(XA_OK);
   lv_terminateThread = releaseTxnObj(false);

   TMTrace(2, ("CTmXaTxn::req_xa_prepare EXIT.\n"));
   return lv_terminateThread;
} // req_xa_prepare


// ---------------------------------------------------------------------------
// CTmXaTxn::req_xa_commit
// Purpose: Support xa_commit
// ---------------------------------------------------------------------------
bool CTmXaTxn::req_xa_commit(CTmTxMessage *pp_msg)
{
   bool lv_terminateThread = false;
   int32 lv_rmid = pp_msg->request()->u.iv_commit.iv_rmid;
   XID *lp_xid = &pp_msg->request()->u.iv_commit.iv_xid;
   int64 lv_flags = pp_msg->request()->u.iv_commit.iv_flags;

   TMTrace(2, ("CTmXaTxn::req_xa_commit ENTRY, Txn ID (%d,%d), state %d, rmid %d, XID %s, flags " PFLL ".\n",
           node(), seqnum(), iv_tx_state, lv_rmid, XIDtoa(lp_xid), lv_flags));

   state_change(TX_COMMIT, pp_msg);
   pp_msg->reply(XA_OK);

   switch (tx_state())
   {
   case TM_TX_STATE_FORGETTING:
   case TM_TX_STATE_COMMITTED:
      state_change(TX_FORGET, pp_msg);
      gp_xarmXIDList->remove(xid());
      break;
   case TM_TX_STATE_FORGETTING_HEUR: //Need an xa_forget to cleanup
   default:
      ;
   }
      
   lv_terminateThread = releaseTxnObj(true);

   TMTrace(2, ("CTmXaTxn::req_xa_commit EXIT.\n"));
   return lv_terminateThread;
} // req_xa_commit


// ---------------------------------------------------------------------------
// CTmXaTxn::req_xa_rollback
// Purpose: Support xa_rollback
// ---------------------------------------------------------------------------
bool CTmXaTxn::req_xa_rollback(CTmTxMessage *pp_msg)
{
   bool lv_terminateThread = true;
   int32 lv_rmid = pp_msg->request()->u.iv_rollback.iv_rmid;
   XID *lp_xid = &pp_msg->request()->u.iv_rollback.iv_xid;
   int64 lv_flags = pp_msg->request()->u.iv_rollback.iv_flags;

   TMTrace(2, ("CTmXaTxn::req_xa_rollback ENTRY, Txn ID (%d,%d), state %d, rmid %d, XID %s, flags " PFLL ".\n",
           node(), seqnum(), iv_tx_state, lv_rmid, XIDtoa(lp_xid), lv_flags));

   state_change(TX_ROLLBACK, pp_msg);
   pp_msg->reply(XA_OK);

   switch (tx_state())
   {
   case TM_TX_STATE_ABORTED:
      state_change(TX_FORGET, pp_msg);
      gp_xarmXIDList->remove(xid());
      break;
   case TM_TX_STATE_FORGETTING_HEUR: //Need an xa_forget to cleanup
   default:
      ;
   }
      
   lv_terminateThread = releaseTxnObj(true);

   TMTrace(2, ("CTmXaTxn::req_xa_rollback EXIT.\n"));
   return lv_terminateThread;
} // req_xa_rollback


// ---------------------------------------------------------------------------
// CTmXaTxn::req_xa_forget
// Purpose: Support xa_forget
// ---------------------------------------------------------------------------
bool CTmXaTxn::req_xa_forget(CTmTxMessage *pp_msg)
{
   bool lv_terminateThread = false;
   int32 lv_rmid = pp_msg->request()->u.iv_forget.iv_rmid;
   XID *lp_xid = &pp_msg->request()->u.iv_forget.iv_xid;
   int64 lv_flags = pp_msg->request()->u.iv_forget.iv_flags;

   TMTrace(2, ("CTmXaTxn::req_xa_forget ENTRY, Txn ID (%d,%d), state %d, rmid %d, XID %s, flags " PFLL ".\n",
           node(), seqnum(), iv_tx_state, lv_rmid, XIDtoa(lp_xid), lv_flags));

   state_change(TX_FORGET, pp_msg);
   pp_msg->reply(XA_OK);

   switch (tx_state())
   {
   case TM_TX_STATE_FORGOTTEN_HEUR:
      gp_xarmXIDList->remove(xid());
      break;
   default:
      ;
   }
      
   lv_terminateThread = releaseTxnObj(true);

   TMTrace(2, ("CTmXaTxn::req_xa_forget EXIT.\n"));
   return lv_terminateThread;
} // req_xa_forget


// --------------------------------------------------------------
// req_xa_start
// Purpose - Txn  specific processing for xa_start.
// xa_start with no flags is like a BEGINTRANSACTION
// --------------------------------------------------------------
bool CTmXaTxn::req_xa_start(CTmTxMessage * pp_msg)
{
   rmid(pp_msg->request()->u.iv_start.iv_rmid);
   xid(&pp_msg->request()->u.iv_start.iv_xid);

   TMTrace (2, ("CTmXaTxn::req_xa_start ENTRY: XA Txn ID (%d,%d), tx state %d, rmid " PFLL ", XID %s\n",
            node(), seqnum(), tx_state(), rmid(), XIDtoa(xid())));
   add_app_partic(pp_msg->request()->u.iv_start.iv_nid, pp_msg->request()->u.iv_start.iv_pid);
   state_change(TX_BEGIN, pp_msg);

   pp_msg->reply(XA_OK);

   return false;
} //req_xa_start


// ---------------------------------------------------------------------------
// CTmXaTxn::req_xa_notImplemented
// Purpose: Methods not implemented.
// ---------------------------------------------------------------------------
void CTmXaTxn::req_xa_notImplemented(CTmTxMessage *pp_msg)
{
   TMTrace(2, ("Dialect %d, request type %d: This method is not implemented!!\n", 
      pp_msg->request()->iv_msg_hdr.dialect_type, pp_msg->requestType()));
   abort();
} // req_xa_notImplemented


// ---------------------------------------------------------------------------
// CTmXaTxn::mapErr
// Purpose: Maps TM error codes to XA ones.
// ---------------------------------------------------------------------------
int CTmXaTxn::mapErr(short pv_tmError)
{
   int lv_xaError = XA_OK;

   switch (pv_tmError)
   {
      case FEOK:
         lv_xaError = XA_OK;
         break;
      case FEENDEDTRANSID:
      case FETXNOTSUSPENDED:
         lv_xaError = XAER_RMERR;
         break;
      case FEINVTRANSID :
         lv_xaError = XAER_DUPID;
         break;
      case FEABORTEDTRANSID:
         lv_xaError = XAER_RMERR;
         break;
      default:
         lv_xaError = XAER_RMFAIL;
   }
   return lv_xaError;
} // mapErr


//----------------------------------------------------------------------------
// CTmXaTxn::releaseTxnObj
// Purpose: cleanup and release the transaction object.
//----------------------------------------------------------------------------
bool CTmXaTxn::releaseTxnObj(bool pv_terminate)
{
   bool lv_terminate = pv_terminate;
   TMTrace (1, ("CTmXaTxn::releaseTxnObj ENTRY: XA Txn ID (%d,%d), txn state %d, terminate? %d.\n", 
            node(), seqnum(), tx_state(), lv_terminate));

   // We want to reset the transactionBusy flag and cleanup the pendingQ
   // even if this is a hung transaction.
   bool lv_txnBusy = reset_transactionBusy(true /*cleanup pendingQ*/);

   // Cleanup now if we are terminating the transaction and we've finished
   // processing transaction requests.
   if (lv_terminate && !lv_txnBusy)
   {
      inc_cleanup();
   }
   else
      TMTrace (3, ("CTmXaTxn::releaseTxnObj : XA Txn ID (%d,%d), terminate=%d, txnBusy=%d, "
               "pending requests, not cleaning up transaction.\n", 
               node(), seqnum(), lv_terminate, lv_txnBusy));

   TMTrace (1, ("CTmXaTxn::releaseTxnObj EXIT: XA Txn ID (%d,%d), txn state %d, terminate? %d.\n", 
            node(), seqnum(), tx_state(), lv_terminate));
   return lv_terminate;
} //releaseTxnObj

//----------------------------------------------------------------------------
// CTmXaTxn::xid_eq
// Purpose: Compare an XID to the XID associated with 
// this XA transaction.
//----------------------------------------------------------------------------
bool CTmXaTxn::xid_eq(XID *pp_xid)
{
   if (pp_xid->formatID == xid()->formatID &&
       pp_xid->bqual_length == xid()->bqual_length &&
       pp_xid->gtrid_length == xid()->gtrid_length &&
       !strcmp(pp_xid->data, xid()->data))
      return true;
   else
      return false;
} //xid_eq
