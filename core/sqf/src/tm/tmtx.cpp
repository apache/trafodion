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
#include "xatmmsg.h"
#include "xatmlib.h"
#include "seabed/trace.h"
#include "tmlogging.h"
#include "tmtx.h"
#include "tminfo.h"
#include "xaglob.h"

// -----------------------------------------------------------------
// helper methods
// -----------------------------------------------------------------

// -------------------------------------
// TM_TX_Info Methods
// -------------------------------------

// -------------------------------------
// TM_TX_Info constructor
// Purpose : calls initialize
// -------------------------------------
TM_TX_Info::TM_TX_Info(int32 pv_nid, int64 pv_flags, int32 pv_trace_level, 
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
/*
// -------------------------------------
// TM_TX_Info constructor
// Purpose : calls initialize
// -------------------------------------
TM_TX_Info::TM_TX_Info(int32 pv_nid, int64 pv_flags, int32 pv_trace_level, 
                       int32 pv_seq, int32 pv_pid, int32 pv_rm_wait_time)
   //:CTmTxBase(pv_nid, pv_flags, pv_trace_level, pv_seq, pv_pid, pv_rm_wait_time)
{

   EID(EID_TM_TX_INFO);
   iv_tx_state = TM_TX_STATE_NOTX;
   
   // Initialize separately from TM_Info::new_tx as we don't have the creator nid,pid when called by
   // the tmPool constructor.
   // this->initialize(pv_nid, pv_flags, pv_trace_level, pv_seq, pv_pid, pv_rm_wait_time);
} */

// -------------------------------------
// TM_TX_Info destructor
// Purpose : 
// -------------------------------------
TM_TX_Info::~TM_TX_Info()
{
   TMTrace (2, ("TM_TX_Info::~TM_TX_Info : ENTRY.\n"));
}


//----------------------------------------------------------------------------
// TM_TX_Info::constructPoolElement
// Purpose : Callback for CTmPool elements.
// This method is called to construct a TM_TX_Info object by CTmPool::newElement.
//----------------------------------------------------------------------------
TM_TX_Info * TM_TX_Info::constructPoolElement(int64 pv_id)
{
   CTmTxKey k(pv_id);

   TMTrace (2, ("TM_TX_Info::constructPoolElement : ENTRY Instantiating new transaction object, ID (%d,%d).\n", 
                k.node(), k.seqnum()));

   TM_TX_Info *lp_tx = new TM_TX_Info(k.node(), 0, gv_tm_info.iv_trace_level, 
                                      k.seqnum(), 0, gv_tm_info.rm_wait_time());
   if (!lp_tx)
   {
      tm_log_event(DTM_LOGIC_ERROR_TX_OBJ, SQ_LOG_CRIT, "DTM_LOGIC_ERROR_TX_OBJ");
      TMTrace (1, ("TM_TX_Info::constructPoolElement :  Failed to instantiate transaction object ID (%d,%d)\n",
               k.node(), k.seqnum()));
      abort();
   }

   TMTrace (2, ("TM_TX_Info::constructPoolElement : EXIT transaction object %p, ID (%d,%d) instantiated.\n", 
                (void *) lp_tx, k.node(), k.seqnum()));
   return lp_tx;
}


// ---------------------------------------------------------
// cleanup
// Purpose : Prepare transaction object for reuse.
// This is called cleanPoolElement.
// ---------------------------------------------------------
void TM_TX_Info::cleanup()
{
   TMTrace (2, ("TM_TX_Info::cleanup : ENTRY transaction object %p, ID (%d,%d).\n", 
                (void *) this, node(), seqnum()));
   lock();
   iv_txnType = TM_TX_TYPE_DTM;
   iv_txnObj.ip_Txn = this;
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
void TM_TX_Info::initialize(int32 pv_nid, int64 pv_flags, int32 pv_trace_level, 
                            int32 pv_seq, int32 pv_creator_nid, int32 pv_creator_pid, 
                            int32 pv_rm_wait_time)
{
   TMTrace(2, ("TM_TX_Info::initialize : ENTRY ID (%d,%d).\n",pv_nid, pv_seq));

   // transid setup
   lock();
   iv_txnType = TM_TX_TYPE_DTM;
   iv_txnObj.ip_Txn = this;

   CTmTxBase::initialize(pv_nid, pv_flags, pv_trace_level, pv_seq, pv_creator_nid, 
                         pv_creator_pid, pv_rm_wait_time);
   unlock();
} // initialize



// ---------------------------------------------------------------
// Transactional State Machine and helper
// ---------------------------------------------------------------

bool TM_TX_Info::state_change_abort_helper(CTmTxMessage * pp_msg)
{
    bool lv_continue = true;
    int32 lv_error = XA_OK;

    if (iv_wrote_trans_state  == false)
    {
       // No reason to write the aborted trans state record (presumed abort protocol).
       //gv_tm_info.write_trans_state (&iv_transid, TM_TX_STATE_ABORTED, abort_flags(), false);
       iv_wrote_trans_state = true;
    }
    lv_error = branches()->rollback_branches (this, TT_flags(), pp_msg, (iv_tm_aborted | iv_tse_aborted));

    switch (lv_error)
    {
         case XA_OK:
         {
               // Set the state to aborted now to allow SQL to continue
               iv_tx_state = TM_TX_STATE_ABORTED;
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
              iv_tx_state = TM_TX_STATE_FORGETTING_HEUR;
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
               TMTrace (1, ("TM_TX_Info::state_change_abort_helper - At least one RM failed to "
                            " respond or connection lost. "
                            "Transaction %d in aborted state placed in hungAborted state.\n",
                            seqnum()));
               iv_tx_state = TM_TX_STATE_HUNGABORTED;
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
               TMTrace (1, ("TM_TX_Info::state_change_abort_helper - Unexpected XA error %d "
                            " returned by xa_rollback for aborted branch. "
                            "Transaction %d placed in hungAborted state.\n",
                            lv_error, seqnum()));
               iv_tx_state = TM_TX_STATE_HUNGABORTED;
               gv_tm_info.inc_tx_hung_count();
               //reset_transactionBusy();
               // Set up retry 
               addHungTimerEvent(TM_MSG_TXINTERNAL_REDRIVEROLLBACK);
               break;                      
            }
    } // switch (lv_error)

    return lv_continue;
      
} //TM_TX_Info::state_change_abort_helper


bool TM_TX_Info::state_change_commit_helper (CTmTxMessage * pp_msg, bool pv_read_only)
{
    int32 lv_error = FEOK;

    if (pv_read_only)
    {
         if (pp_msg->replyPending()) 
              pp_msg->reply(FEOK);
         // Don't write committed state record for read-only transactions
         // TM_Info::write_trans_state (&iv_transid, TM_TX_STATE_COMMITTED, abort_flags, true);
         // iv_wrote_trans_state = true;
         TMTrace(3,  ("state_change_commit_helper, read only, skipping commit record\n"));
         iv_tx_state = TM_TX_STATE_FORGETTING;
         return true;
    }

    if (iv_wrote_trans_state == false)
    {
        gv_tm_info.write_trans_state (&iv_transid, TM_TX_STATE_COMMITTED, abort_flags(), true);
        iv_wrote_trans_state = true;
    }
    lv_error = branches()->commit_branches (this, TT_flags(), pp_msg); 
    switch (lv_error)
    {
         case XA_OK:
         {
              iv_tx_state = TM_TX_STATE_FORGETTING;
              lv_error = branches()->completeRequest_branches(this);
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
               iv_tx_state = TM_TX_STATE_FORGETTING_HEUR;
               TMTrace (1, ("TM_TX_Info::state_change_commit_helper: Commit completed heuristically with XAER %d for "  
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
               TMTrace (1, ("TM_TX_Info::state_change_commit_helper - At least one RM failed to "
                            " respond or connection lost. "
                            "Transaction ID (%d,%d) in committed state placed in hungCommitted state.\n",
                            node(), seqnum()));
               iv_tx_state = TM_TX_STATE_HUNGCOMMITTED;
               gv_tm_info.inc_tx_hung_count();
               reset_transactionBusy();
               // Set up retry 
               addHungTimerEvent(TM_MSG_TXINTERNAL_REDRIVECOMMIT);
               break;                      
           }
           default:  // some unexpected error
           {                
               tm_log_event(DTM_TMTX_TX_HUNGCOMMITTED_ERROR, SQ_LOG_WARNING, "DTM_TMTX_TX_HUNGCOMMITTED_ERROR", lv_error);
               TMTrace (1, ("TM_TX_Info::state_change_commit_helper - Unexpected XA error %d "
                            " returned by xa_commit for committed branch. "
                            "Transaction %d placed in hungCommitted state.\n",
                            lv_error, seqnum()));
               iv_tx_state = TM_TX_STATE_HUNGCOMMITTED;
               gv_tm_info.inc_tx_hung_count();
               reset_transactionBusy();
               // Set up retry 
               addHungTimerEvent(TM_MSG_TXINTERNAL_REDRIVECOMMIT);
               break;                      
            }
      } // switch

      return true;
}

bool TM_TX_Info::state_change_prepare_helper(CTmTxMessage * pp_msg)
{
   bool lv_continue = true;
   int32 lv_error = branches()->prepare_branches (this, TT_flags(), pp_msg);
   if (iv_mark_for_rollback == true)
   {
      short lv_replyErr = FEOK;
      switch (lv_error)
      {
      case COMMIT_CONFLICT:
         lv_replyErr = FEHASCONFLICT;
         tm_log_event(TM_HBASE_COMMIT_CONFLICT, SQ_LOG_INFO, "TM_HBASE_COMMIT_CONFLICT", 
                      lv_error,-1,node(),seqnum(),-1,lv_replyErr);
         break;
      default: 
         lv_replyErr = FEABORTEDTRANSID;
         tm_log_event(TM_HBASE_PREPARE_FAIL, SQ_LOG_INFO, "TM_HBASE_PREPARE_FAIL", 
                      lv_error,-1,node(),seqnum(),-1,lv_replyErr);
         break;
      }
      pp_msg->responseError(lv_replyErr);
      iv_tx_state = TM_TX_STATE_ABORTING;
      lv_continue = false;
      iv_tse_aborted = true;
      gv_tm_info.inc_tm_initiated_aborts();
      return lv_continue;
   }
    
   // need to save error!
   switch (lv_error)
   {
   //  Shouldn't see read-only right now, but the case
   // is here in case it changes at some point.
   case XA_RDONLY:
   case XA_OK:
      iv_tx_state = TM_TX_STATE_COMMITTING;
#ifdef DEBUG_MODE
          bool lv_assert;
          lv_assert = false;
          ms_getenv_bool("TM_TEST_AFTER_PREPARE_ASSERT", &lv_assert);
          if (lv_assert)
          {
                tm_log_event(TM_TEST_XA_OK_ASSERT, SQ_LOG_CRIT, "TM_TEST_XA_OK_ASSERT");
                TMTrace (1, ("TM_TX_Info::state_change_prepare_helper - TM test assert after XA_OK state\n"));
                abort ();
          }          
#endif 
      break;
   case XAER_RMFAIL:
      state_change_abort_set(pp_msg, FEDEVICEDOWNFORTMF);
      lv_continue = false;
      break;
   case COMMIT_CONFLICT:
      state_change_abort_set(pp_msg, COMMIT_CONFLICT);
      lv_continue = false;
   default:
   // All other errors
      state_change_abort_set (pp_msg, FEABORTEDTRANSID);
      lv_continue = false;
      break;
   }
   return lv_continue;   
} // state_change_prepare_helper

void TM_TX_Info::state_change_abort_set(CTmTxMessage * pp_msg, short pv_error)
{
   iv_tx_state = TM_TX_STATE_ABORTING;
   iv_tm_aborted = true;
   gv_tm_info.inc_tm_initiated_aborts();
   // Reply now as we know the outcome and don't want to hold up the client
   if (pp_msg->replyPending())
      pp_msg->reply(pv_error);
}

int32 TM_TX_Info::state_change (TX_EVENT pv_event, 
                               int32 pv_nid, int32 pv_pid, 
                               CTmTxMessage * pp_msg)
{
    bool       lv_continue = true;
    bool       lv_continue2 = true;
    int32      lv_error = XA_OK;
    TX_EVENT   lv_event = pv_event;

    TMTrace (2, ("TM_TX_Info::state_change ENTRY, txn ID %d, current state %d, new event %d\n", 
                     seqnum(), iv_tx_state, lv_event));
    while (lv_continue)
    {
        switch (lv_event)
        {
        case TX_BEGIN:
            {
                switch (iv_tx_state)
                {
                case TM_TX_STATE_NOTX:
                {
                    do {
                       lv_error = branches()->start_branches (this, TT_flags(), pp_msg);
                    } while (lv_error == XA_RETRY);
                    if (lv_error)
                    {
                        // Drive rollback
                        state_change_abort_set(pp_msg, FEABORTEDTRANSID);
                        lv_continue = false;
                        //lv_event = TX_ROLLBACK;
                    }
                    else
                    {
                        if (gv_tm_info.mode() == TM_SYNC_MODE)
                            sync_write (pv_nid, pv_pid, TM_TX_STATE_BEGINNING);
                         else
                              iv_tx_state = TM_TX_STATE_BEGINNING;  // BEGIN_COMPLETE lv_event
                         lv_continue = false;
                     }
                     break;
                }
                case TM_TX_STATE_BEGINNING:
                {
                     if (gv_tm_info.mode() == TM_SYNC_MODE)
                         sync_write (pv_nid, pv_pid, TM_TX_STATE_BEGINNING);
                     else
                         iv_tx_state = TM_TX_STATE_BEGINNING;
                     lv_continue = false;
                     break;
                }
                default:
                {
                    tm_log_event(TM_TMTX_BRCH_STRT_UKN_STATE, SQ_LOG_CRIT, "TM_TMTX_BRCH_STRT_UKN_STATE");
                    TMTrace (1, ("TM_TX_Info::state_change - Branch start in unknown state\n"));
                    abort();
                }
                };
            break;
            }
        case TX_BEGIN_COMPLETE:
           {
                switch (iv_tx_state)
                {
                case TM_TX_STATE_BEGINNING:
                    tx_state(TM_TX_STATE_ACTIVE);
                    // Left as place holder in just in case it has a purpose if run_mode == TM_NONSYNC_MODE
                    //reset_transactionBusy();
                    lv_continue = false;
                    TM_TEST_PAUSE(iv_tx_state);
                    break;
                default:
                    if (isAborting())
                        lv_event = TX_ROLLBACK;
                    else
                    {
                        tm_log_event(TM_TMTX_COMP_NOT_BEG, SQ_LOG_CRIT, "TM_TMTX_COMP_NOT_BEG");
                        TMTrace (1, ("CTmTx_Transaction::state_change - tx state %d: Transaction in invalid "
                                      " state for EVENT TX_BEGIN_COMPLETE.\n", iv_tx_state));
                        abort ();
                    }
                } //switch tx_state

                break;
           }
        case TX_COMMIT:
            {
                if ((iv_tx_state != TM_TX_STATE_ACTIVE) &&
                    (iv_tx_state != TM_TX_STATE_COMMITTED) &&  
                    (iv_tx_state != TM_TX_STATE_FORGOTTEN) &&
                    (iv_tx_state != TM_TX_STATE_COMMITTING) &&
                    (iv_tx_state != TM_TX_STATE_FORGETTING) &&
                    (iv_tx_state != TM_TX_STATE_HUNGCOMMITTED) &&
                    (iv_tx_state != TM_TX_STATE_FORGETTING) &&
                    (iv_tx_state != TM_TX_STATE_FORGOTTEN) &&
                    (iv_tx_state != TM_TX_STATE_FORGETTING_HEUR))
                {
                         tm_log_event(TM_TMTX_COMMIT_NO_OTHERS, SQ_LOG_CRIT, "TM_TMTX_COMMIT_NO_OTHERS");
                        TMTrace (1, ("TM_TX_Info::state_change - tx state %d: "
                            " Transaction in invalid state.  It must be one of ACTIVE,"
                            " COMMITTED, COMMITTING, FORGETTING, FORGOTTEN, or HUNGCOMMITTED.\n",iv_tx_state));
                        abort ();
                }
                lv_continue2 = true;

               while (lv_continue2)
               {
                    switch (iv_tx_state)
                    {
                    case  TM_TX_STATE_ACTIVE:
                    {
                        TM_TEST_PAUSE(iv_tx_state);
                        //Don't call end_branches if txn is already marked for
                        //rollback.
                        if (iv_mark_for_rollback == false)
                           lv_error = branches()->end_branches (this, TT_flags());
                        if ((pv_event == TX_COMMIT && lv_error == XA_OK) &&
                            (iv_mark_for_rollback == false))
                           iv_tx_state = TM_TX_STATE_PREPARING;
                        else 
                        {
                           // record we drove the abort
                           state_change_abort_set(pp_msg, FEABORTEDTRANSID);
                           lv_continue2 = false;
                           if (gv_tm_info.mode() == TM_SYNC_MODE)
                           {
                              //drive to next state for sync mode, to enable
                              //rollback to continue.
                              lv_event = TX_ROLLBACK;
                           }
                           else
                           {
                              //get out of the state machine for non-sync mode.
                              //the abort will be driven by req_end_complete.
                              lv_continue = false;
                           }
                        }
                        break;
                    }
                    case TM_TX_STATE_PREPARING:
                    {
                        TM_TEST_PAUSE(iv_tx_state);
                        lv_continue2 = state_change_prepare_helper(pp_msg);
                        if (!lv_continue2)
                        {
                           if (gv_tm_info.mode() == TM_SYNC_MODE)
                           {
                              //drive to next state for sync mode, to enable
                              //rollback to continue.
                              tx_state(TM_TX_STATE_ABORTING);
                              lv_event = TX_ROLLBACK;
                           }
                           else
                           {
                              //get out of the state machine for non-sync mode.
                              //the abort will be driven by req_end_complete.
                              lv_continue = false;
                           }
                        }
                        else 
                        {
                           int32 lv_partic = branches()->num_rm_partic(this);
                           TMTrace (2, ("TM_TX_Info::state_change, num partic %d\n", lv_partic));
                           if (lv_partic == 0)
                           {
                              tx_state(TM_TX_STATE_COMMITTED);
                              iv_read_only = true;
                           } 
                           else
                              iv_read_only = false;
                        }                        
                         break;
                    }
                    case TM_TX_STATE_COMMITTING:
                    {
                        TM_TEST_PAUSE(iv_tx_state);
                        if (gv_tm_info.mode() == TM_SYNC_MODE)
                            sync_write (pv_nid, pv_pid, TM_TX_STATE_COMMITTED);
                        else
                            tx_state(TM_TX_STATE_COMMITTED);
                        lv_continue = lv_continue2 = true; 
                        break;
                     }
                     case TM_TX_STATE_COMMITTED:
                     {
                          TM_TEST_PAUSE(iv_tx_state);
                          state_change_commit_helper(pp_msg, iv_read_only);
                          break;
                     }
                     case TM_TX_STATE_FORGETTING_HEUR:
                     case TM_TX_STATE_FORGOTTEN_HEUR:
                     {
                          branches()->forget_heur_branches(this, TT_flags());
                          tx_state(TM_TX_STATE_FORGOTTEN);
                          break;
                     }
                     case TM_TX_STATE_FORGETTING:
                     {
                        TM_TEST_PAUSE(iv_tx_state);
                        if (!iv_read_only)
                        {
                            gv_tm_info.write_trans_state (&iv_transid, TM_TX_STATE_FORGOTTEN,
                                                          abort_flags(), false);
                            iv_wrote_trans_state = true;
                        }
                        else
                            TMTrace(3, ("TM_TX_INFO::state_change, read only, skipping forget record\n"));
                        tx_state(TM_TX_STATE_FORGOTTEN);
                        break;
                     }
                     case TM_TX_STATE_FORGOTTEN:
                     {
                        TM_TEST_PAUSE(iv_tx_state);
                        if (gv_tm_info.mode() == TM_SYNC_MODE)
                            sync_write (pv_nid, pv_pid, TM_TX_STATE_FORGOTTEN);
                        else
                            tx_state(TM_TX_STATE_FORGOTTEN);

                        lv_continue = lv_continue2 = false; 
                        break;
                     }
                     case TM_TX_STATE_HUNGCOMMITTED:
                     {
                        TM_TEST_PAUSE(tx_state());
                        lv_continue = lv_continue2 = false;
                        break;
                     }
                     default:
                     {
                          tm_log_event(TM_TMTX_INVALID_STATE, SQ_LOG_CRIT, "TM_TMTX_INVALID_STATE");
                          TMTrace (1, ("TM_TX_Info::state_change - tx state %d: Invalid transaction state."
                                       " Txn ID (%d,%d)\n",iv_tx_state, node(), seqnum()));
                        // invalid, abort?
                        abort ();
                     } // default

                     }; // switch
               } // while
               break;
            } // switch TX_COMMIT
        case TX_ROLLBACK:
            {
                
                if ((iv_tx_state != TM_TX_STATE_BEGINNING) &&
                    (iv_tx_state != TM_TX_STATE_ACTIVE) &&
                    (iv_tx_state != TM_TX_STATE_ABORTING) &&
                    (iv_tx_state != TM_TX_STATE_ABORTING_PART2) &&
                    (iv_tx_state != TM_TX_STATE_ABORTED) &&
                    (iv_tx_state != TM_TX_STATE_HUNGABORTED) &&
                    (iv_tx_state != TM_TX_STATE_FORGETTING) &&
                    (iv_tx_state != TM_TX_STATE_FORGOTTEN) &&
                    (iv_tx_state != TM_TX_STATE_FORGETTING_HEUR))
                {
                    tm_log_event(TM_TMTX_RLL_BACK_NO_OTHERS, SQ_LOG_CRIT, "TM_TMTX_RLL_BACK_NO_OTHERS");
                    TMTrace (1, ("TM_TX_Info::state_change - tx state %d: "
                                 "Transaction in invalid state.  It must be BEGINNING, ACTIVE, "
                                  "ABORTING, ABORTED, or HUNGABORTED.\n",iv_tx_state));
                    abort ();

                }

                lv_continue2 = true;
                while (lv_continue2)
                {
                    switch (iv_tx_state)
                    {
                    case  TM_TX_STATE_BEGINNING:
                    case  TM_TX_STATE_ACTIVE:
                    {
                        TM_TEST_PAUSE(iv_tx_state);
                        lv_error = branches()->end_branches (this, TT_flags());
                        iv_tx_state = TM_TX_STATE_ABORTING;
                        break;
                    }
                    case TM_TX_STATE_ABORTING:
                    {
                        TM_TEST_PAUSE(iv_tx_state);
                        if (gv_tm_info.mode() == TM_SYNC_MODE)
                        {
                             sync_write (pv_nid, pv_pid, TM_TX_STATE_ABORTING_PART2);
                             lv_continue = lv_continue2 = false;
                        }
                        else
                            iv_tx_state = TM_TX_STATE_ABORTING_PART2;
                        break;
                    }
                    case TM_TX_STATE_ABORTING_PART2:
                    {
                        // This is used to avoid telling SQL aborted before lock release
                        TM_TEST_PAUSE(iv_tx_state);
                        lv_continue = lv_continue2 = state_change_abort_helper (pp_msg);
                       break;
                    }
                    case TM_TX_STATE_ABORTED:
                    {
                        TM_TEST_PAUSE(iv_tx_state);
                        iv_tx_state = TM_TX_STATE_FORGETTING;
                       break;
                     }
                     case TM_TX_STATE_FORGETTING_HEUR:
                     case TM_TX_STATE_FORGOTTEN_HEUR:
                     {
                        TM_TEST_PAUSE(iv_tx_state);
                        branches()->forget_heur_branches(this, TT_flags());
                        iv_tx_state = TM_TX_STATE_FORGOTTEN;
                        break;
                     }
                     case TM_TX_STATE_FORGETTING:
                     {
                        TM_TEST_PAUSE(iv_tx_state);
                        iv_tx_state = TM_TX_STATE_FORGOTTEN;
                        break;
                     }
                     case TM_TX_STATE_FORGOTTEN:
                     {
                        TM_TEST_PAUSE(iv_tx_state);
                        // No need to write 'forgotten' trans state record after abort.
                        if (gv_tm_info.mode() == TM_SYNC_MODE)
                            sync_write (pv_nid, pv_pid, TM_TX_STATE_FORGOTTEN);
                        else
                            iv_tx_state = TM_TX_STATE_FORGOTTEN;
                        lv_continue = lv_continue2 = false;
                        break;
                     }
                     case TM_TX_STATE_HUNGABORTED:
                     {
                        TM_TEST_PAUSE(iv_tx_state);
                        lv_continue = lv_continue2 = false;
                        break;
                     }

                    default:
                       {
                        tm_log_event(TM_TMTX_INVALID_STATE, SQ_LOG_CRIT,
                                     "TM_TMTX_INVALID_STATE",-1,-1,-1,seqnum());
                        TMTrace (1, ("TM_TX_Info::state_change - tx state %d: "
                                     "Invalid transaction state. Transaction seq num %d\n",
                                     iv_tx_state, seqnum()));         
                        abort ();
                       }

                    }; // swtich (iv_tx_state)
                } // while
                break;
            } // case TX_ROLLBACK
       case TX_FORGET:
            {
                switch (iv_tx_state)
                {
                     case TM_TX_STATE_FORGETTING_HEUR:
                     {
                        TM_TEST_PAUSE(iv_tx_state);
                        branches()->forget_heur_branches(this, TT_flags());
                        break;
                     }
                     case TM_TX_STATE_FORGOTTEN_HEUR:
                     case TM_TX_STATE_FORGETTING:
                     {
                        TM_TEST_PAUSE(iv_tx_state);
                        if (gv_tm_info.mode() == TM_SYNC_MODE)
                            sync_write (pv_nid, pv_pid, TM_TX_STATE_FORGOTTEN);
                        else
                            iv_tx_state = TM_TX_STATE_FORGOTTEN;
                        break;
                     }
                     case TM_TX_STATE_FORGOTTEN:
                     {
                        TM_TEST_PAUSE(iv_tx_state);
                        lv_continue = lv_continue2 = false;
                        break;
                     }
                     default:
                     {
                        tm_log_event(TM_TMTX_INVALID_FGR_STATE, SQ_LOG_CRIT, "TM_TMTX_INVALID_FGR_STATE");
                        TMTrace (1, ("TM_TX_Info::state_change - tx state %d: "
                                  "Transaction in invalid state.  It must be in FORGOTTEN state.\n",iv_tx_state));
                        abort ();
                     }
                } // switch tx_state
                break;
            }
       // we get this when we simly need to resent the commit
       // at recovery time
        case TX_REDRIVE_COMMIT:
            {
                TM_TEST_PAUSE(iv_tx_state);
                // we've already written the trans state record if were are here
                iv_tx_state = TM_TX_STATE_COMMITTED;  
                lv_event = TX_COMMIT;
                break;
            } // case TX_REDRIVE_COMMIT
        case TX_REDRIVE_ROLLBACK:
            {
                TM_TEST_PAUSE(iv_tx_state);
                // we've already written the trans state record if were are here
                iv_tx_state = TM_TX_STATE_ABORTING_PART2;
                lv_event = TX_ROLLBACK;
                break;
            }
        default :
            tm_log_event(TM_TMTX_INVALID_STATE, SQ_LOG_CRIT, "TM_TMTX_INVALID_STATE",
                         -1,-1,-1,seqnum());
            TMTrace (1, ("TM_TX_Info::state_change - tx state %d: "
                         "Invalid transaction state. Transaction seq num %d\n",lv_event, seqnum()));
            abort();

        };
    }
  
    TM_TEST_PAUSE_NEXT();
    TMTrace (2, ("TM_TX_Info::state_change EXIT : txn ID %d, state %d\n", 
                 seqnum(), iv_tx_state));
    return lv_error;
}


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
int32 TM_TX_Info::schedule_abort()
{
   int32 lv_error = FEOK;

   TMTrace (2, ("TM_TX_Info::schedule_abort ENTRY, Txn ID (%d,%d), state %d.\n",
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

   tm_log_event(DTM_PROCDEATH_ANY, SQ_LOG_NOTICE, "DTM_PROCDEATH_ANY", -1, -1, -1, seqnum(), -1, 
                lv_error, -1, -1, -1, -1, -1, -1, iv_tx_state, -1, -1, -1, "", node());

   TMTrace (2, ("TM_TX_Info::schedule_abort, EXIT, ID %d, error %d, state %d, %s.\n",
                   seqnum(), lv_error, tx_state(), ((iv_mark_for_rollback)?"rollback deferred":"rolled back")));

   return lv_error;
} //schedule_abort


// --------------------------------------------------------------
// req_ddloperation
// Purpose - Txn Thread specific processing for ddl operations
// --------------------------------------------------------------
bool TM_TX_Info::req_ddloperation(CTmTxMessage *pp_msg)
{
   TMTrace (2, ("TM_TX_Info::req_ddloperation : ID (%d,%d) ENTRY.\n",
      node(), seqnum()));

   short lv_error = branches()->ddlOperation(this, 0, pp_msg);

   lock();
   if (pp_msg->replyPending())
      pp_msg->reply(lv_error);
   unlock();

   reset_transactionBusy();

   TMTrace (2, ("TM_TX_Info::req_ddloperation : EXIT error %d, Txn ID (%d,%d).\n",
      lv_error, node(), seqnum()));

   return true;
}


// -------------------------------------------------------------
// schedule_eventQ
// -------------------------------------------------------------
void TM_TX_Info::schedule_eventQ()
{
   short lv_request_type = TM_MSG_TYPE_NULL;

   TMTrace (2, ("TM_TX_Info::schedule_eventQ ENTER ID %d, state %d.\n",
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
         TMTrace (1, ("TM_TX_Info::schedule_eventQ - tx state %d: "
                      "Invalid transaction state. Transaction seq num %d\n",iv_tx_state, seqnum()));
         abort();
      }
   };

   // We use the saved request here to drive the event.
   // This is because this is a continuance of the existing
   // request and we want this to be used for any reply as
   // it contains the msgid of the original request (eg
   // ENDTRANSACTION request).
   if (!ip_currRequest)
   {
      tm_log_event(DTM_TMTX_INVALID_POINTER, SQ_LOG_CRIT, "DTM_TMTX_INVALID_POINTER",
                    -1,-1,-1,seqnum());
      TMTrace (1, ("TM_TX_Info::schedule_eventQ - PROGRAMMING BUG!: "
                   " ip_currRequest was null when we received a sync "
                   "completion for transaction %d.\n", seqnum()));
      abort();
   }
   else
   {
      ip_currRequest->requestType(lv_request_type);
      eventQ_push(ip_currRequest);
   }

   TMTrace (2, ("TM_TX_Info::schedule_eventQ EXIT ID %d, request_type %d.\n",
                   iv_tag, lv_request_type));
}


// --------------------------------------------------------------
// process_eventQ
// Purpose - process all events on the event queue for this 
// transaction.
// For multithreaded TMs this is the main processing loop for
// the transaction.
// --------------------------------------------------------------
void TM_TX_Info::process_eventQ()
{
   bool lv_exit = false;
   CTmEvent *lp_event = NULL;

   TMTrace (2, ("TM_TX_Info::process_eventQ : ENTRY ID %d, thread %s.\n",
                   iv_tag, ((gv_tm_info.multithreaded())?ip_Thread->get_name():"SINGLE THREAD")));

   // Pop/wait for a transaction event
   CTmTxMessage *lp_msg = eventQ_pop();
   lp_msg->validate();

   while (!lv_exit && lp_msg->request())
   {
      switch (lp_msg->requestType())
      {
      case TM_MSG_TYPE_BEGINTRANSACTION:
         lv_exit = req_begin(lp_msg);
         break;
      case TM_MSG_TYPE_ENDTRANSACTION:
         lv_exit = req_end(lp_msg);
//	 delete lp_msg;
         break;
      case TM_MSG_TYPE_ABORTTRANSACTION:
      case TM_MSG_TYPE_TSE_DOOMTX:
      case TM_MSG_TYPE_DOOMTX:
         lv_exit = req_abort(lp_msg);
//	 delete lp_msg;
         break;
      case TM_MSG_TYPE_STATUSTRANSACTION:
         // These aren't queued, so we should never hit this
         lv_exit = req_status(lp_msg);
         break;
      case TM_MSG_TYPE_AX_REG:
         lv_exit = req_ax_reg(lp_msg);
         break;
      case TM_MSG_TYPE_REGISTERREGION:
         lv_exit = req_registerRegion(lp_msg);
/*	 if (! lp_msg->replyPending()) {
	   delete lp_msg;
	 } */
         break;
      case TM_MSG_TYPE_DDLREQUEST:
         lv_exit = req_ddloperation(lp_msg);
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
         lv_exit = req_begin_complete(lp_msg);
         break;
      case TM_MSG_TXINTERNAL_REDRIVESYNC:
         lv_exit = redrive_sync(lp_msg);
         break;
      case TM_MSG_TXTHREAD_TERMINATE:
         // Need to re-queue the Terminate to the thread event queue
         lp_event = new CTmEvent(lp_msg);
         ip_Thread->eventQ_push(lp_event);
         // Intentional fall through.
      case TM_MSG_TXTHREAD_RELEASE:
         lv_exit = true;
         break;
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
         TMTrace (1, ("TM_TX_Info::process_eventQ - DTM%d txThread received UNKNOWN message type : %d\n",
                 gv_tm_info.nid(), lp_msg->requestType()));
         abort (); // bogus type
      } // switch

      // Protect message as registerRegion could try to reply from the main thread.
      bool detectDoubleDelete = true;
      lock();
      if (lp_msg->validate(detectDoubleDelete))
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
            TMTrace (3, ("TM_TX_Info::process_eventQ : Thread %s thread event queue empty, "
                         "checking transaction Q.\n", ip_Thread->get_name()));

            lp_msg = eventQ_pop();
         }
         else
         {
            TMTrace (3, ("TM_TX_Info::process_eventQ : Thread %s thread event detected, exiting proc.\n",
                           ip_Thread->get_name()));
            // Drop out of transaction processing loop and
            // handle the thread event
            lv_exit = true;
         }
      }
   } //while more events to process

   TMTrace (2, ("TM_TX_Info::process_eventQ : EXIT teminateThread=%d.\n",
            lv_exit));
} //process_eventQ


// --------------------------------------------------------------
// Event Specific Methods:
// --------------------------------------------------------------
// req_begin
// Purpose - Txn Thread specific processing for BEGINTRANSACTION.
// Note that this is now called waited in the main thread and 
// therefore must not block!!!
// --------------------------------------------------------------
bool TM_TX_Info::req_begin(CTmTxMessage * pp_msg)
{
   bool  lv_exit = false;
   short lv_error = FEOK;

   TMTrace (2, ("TM_TX_Info::req_begin : ENTRY\n"));

   if (gv_tm_info.TSMode() >= TS_BEGINONLY)
      iv_beginTime = Ctimeval::now();
   
   gv_tm_info.inc_begin_count();
   gv_tm_info.inc_tx_count();
   memcpy (&pp_msg->response()->u.iv_begin_trans.iv_transid, transid(), 
            TM_TRANSID_BYTE_SIZE);

   pp_msg->response()->u.iv_begin_trans.iv_tag = iv_tag;

   // now change state, add this to our active tx list and send sync data
   initialize_tx_rms(gv_tm_info.RMPartic());

   state_change(TX_BEGIN, gv_tm_info.nid(), gv_tm_info.pid(), pp_msg);
   if (pp_msg->responseError() == FEOK)
   {
      if (gv_tm_info.mode() == TM_NONSYNC_MODE)
         req_begin_complete(pp_msg);
   }
   else
   {
      // Save the error for display in exit trace message.
      lv_error = pp_msg->responseError();
      // We may already have responded, in which case we don't want to do so again
      if (pp_msg->replyPending())
      {
         // if we had an error, abort processing will have been initiated for
         // this transaction.  We need to reply back to inform the application 
         // of the outcome, but don't release the transaction object yet, we
         // still need to process the abort completion.
         pp_msg->reply();
      }

      // Queue abort as we now execute in the main thread and don't want to wait.
      schedule_abort();
   }
  
   TMTrace (2, ("TM_TX_Info::req_begin : EXIT, error=%d\n", lv_error));

   return lv_exit;
} //req_begin

// --------------------------------------------------------------
// 

// Purpose - Txn Thread specific processing for ENDTRANSACTION.
// --------------------------------------------------------------
bool TM_TX_Info::req_end(CTmTxMessage * pp_msg)
{
   short lv_error = FEOK;
   bool lv_terminate = false;

   TMTrace (2, ("TM_TX_Info::req_end : ENTRY\n"));

   if (iv_ender_pid != pp_msg->request()->u.iv_end_trans.iv_pid)   
   {
        // Drive rollback
        state_change_abort_set(pp_msg, FEABORTEDTRANSID);
        state_change(TX_ROLLBACK, gv_tm_info.nid(), gv_tm_info.pid(), pp_msg);
        //TODO: We should probably do the abort_complete etc only for
        // non-sync mode.
        req_abort_complete(pp_msg);
        lv_terminate = req_forget(pp_msg);
        TMTrace (1, ("TM_TX_Info::req_end : ID (%d%d), pid (%d,%d) not ender (%d,%d).\n",
                 node(), seqnum(), pp_msg->request()->u.iv_end_trans.iv_nid, 
                 pp_msg->request()->u.iv_end_trans.iv_pid, ender_nid(), ender_pid()));

        // don't need to do inc_cleanup or reset_transactionBusy 
        // as that gets done in req_forget
        return lv_terminate;
   }

   if ((tx_state() !=  TM_TX_STATE_ACTIVE) &&
       (tx_state() != TM_TX_STATE_COMMITTED))
   {
      if (isAborting())
         lv_error = FEABORTEDTRANSID;
      else
         lv_error = FEENDEDTRANSID;

      TMTrace (1,  ("TM_TX_Info::req_end, ID %d, unable to complete txState %d, returning error %d.\n",
                       seqnum(), tx_state(), lv_error));
      pp_msg->reply(lv_error);
      lv_terminate = req_forget(pp_msg);
      return lv_terminate;
   }

   // if there are outstanding joiners, don't let this go through 
   // Don't increment the cleanup counter here as this isn't a fatal error.
   if (num_active_partic() > 1)
   {
      TMTrace (1, ("TM_TX_Info::req_end :, ID (%d,%d), has %d active joiners.\n",
                       node(), seqnum(), num_active_partic()));
      pp_msg->reply(FEJOINSOUTSTANDING);
      reset_transactionBusy();
      return false;
   } 

   gv_tm_info.inc_commit_count();

   // change state
   state_change (TX_COMMIT, gv_tm_info.nid(), gv_tm_info.pid(), pp_msg);

   if (gv_tm_info.mode() == TM_NONSYNC_MODE)
   {
      req_end_complete(pp_msg);
      lv_terminate = req_forget(pp_msg);
   }
   else
   {
      // for SYNC mode - we still don't need this - so its ok to remove it now
      remove_app_partic(pp_msg->request()->u.iv_end_trans.iv_pid, 
                        pp_msg->request()->u.iv_end_trans.iv_nid);
   }
 
   TMTrace (2, ("TM_TX_Info::req_end : EXIT\n"));

   // FOR SYNC MODE Always return false to allow the transaction to continue once the
   // TM_MSG_TX_INTERNAL_ENDCOMPLETE is received.  This is determined by the value
   // of lv_terminate, which will only be true if mode is TM_NONSYNC_MODE and
   // req_forget decided the transaction is finished and the object should be cleaned up.
   return lv_terminate;

} //req_end


// --------------------------------------------------------------
// req_abort
// Purpose - Txn Thread specific processing for ABORTTRANSACTION
// and for TSE_DOOMTX.
// --------------------------------------------------------------
bool TM_TX_Info::req_abort(CTmTxMessage * pp_msg)
{   
   bool lv_terminateThread = false;
   int32 lv_error = FEOK;
   int32 lv_nid = pp_msg->request()->u.iv_abort_trans.iv_nid,
         lv_pid = pp_msg->request()->u.iv_abort_trans.iv_pid;
    int32 lv_rmid;

   TMTrace (2, ("TM_TX_Info::req_abort : ENTRY Txn ID (%d,%d), originating process (nid, pid): "
            "(%d, %d).\n", node(), seqnum(), lv_nid, lv_pid));

   pp_msg->validate();
   gv_tm_info.inc_abort_count();
   if (pp_msg->requestType() == TM_MSG_TYPE_TSE_DOOMTX)
   {
     
      lv_rmid = gv_RMs.TSE()->return_rmid(lv_nid, lv_pid);
      if (lv_rmid == -1)
      {
         TMTrace(1, ("TM_TX_Info::req_abort : Error %d retrieving pname "
                 "for TSE (%d, %d). Returning FENOTFOUND(11) to DOOMTRANSACTION.\n",
                 lv_error, lv_nid, lv_pid));
         tm_log_event(DTM_DOOMTXN_NID_PID_BAD, SQ_LOG_CRIT, "DTM_DOOMTXN_NID_PID_BAD", 
                      lv_error, -1, -1, -1, pp_msg->msgid(), -1, -1, -1, -1, -1, -1, 
                      -1, -1, -1, lv_pid, -1, NULL, lv_nid);
         pp_msg->reply(FENOTFOUND);
         delete pp_msg;
         return lv_terminateThread;
      }

      // EMS message here, DTM_TSE_DOOMEDTXN
      tm_log_event(DTM_TSE_DOOMEDTXN, SQ_LOG_WARNING, "DTM_TSE_DOOMEDTXN",
                   -1, lv_rmid, node(), seqnum(),-1,-1,-1,-1,
                   -1,-1,-1,-1,tx_state(),
                   pp_msg->request()->u.iv_abort_trans.iv_nid, 
                   pp_msg->request()->u.iv_abort_trans.iv_pid); 

      TMTrace (1, ("TM_TX_Info::req_abort : $DTM%d received TSE DOOMTRANSACTION for Txn ID "
               "(%d,%d), Tx state %d, from TSE rmid %d, process (%d, %d).\n",
               gv_tm_info.nid(), node(), seqnum(), tx_state(), lv_rmid,
               pp_msg->request()->u.iv_abort_trans.iv_nid, 
               pp_msg->request()->u.iv_abort_trans.iv_pid));
      CTmTxBase::register_branch(lv_rmid, NULL); // Make sure this RM is included as a participant
      iv_tse_aborted = true;
   }
   else
      iv_appl_aborted = true;
  
   // if this DOOM came from the TSE, it is ok, it will just ignore it if it can't find it
   remove_app_partic(pp_msg->request()->u.iv_abort_trans.iv_pid, 
                     pp_msg->request()->u.iv_abort_trans.iv_nid);

   // Modify request into a Rollback request.
   pp_msg->requestType(TM_MSG_TXINTERNAL_ROLLBACK);
   pp_msg->request()->u.iv_rollback_internal.iv_takeover_or_shutdown = false;

   pp_msg->validate();
   lv_terminateThread = rollback_txn(pp_msg);

   if ((gv_tm_info.mode() == TM_NONSYNC_MODE) && (!lv_terminateThread))
   {
       // Make sure that rollback_txn didn't reset_transactionBusy().  
       // If it did, then we no longer have a pp_msg object and have replied.
       if (transactionBusy())
          req_abort_complete(pp_msg);
       else {
          if (tx_state() != TM_TX_STATE_HUNGABORTED)
             stats()->txnAbort()->stop();
       }
       lv_terminateThread = req_forget();
   }

   TMTrace (2, ("TM_TX_Info::req_abort : ID %d, EXIT\n", seqnum()));

   return lv_terminateThread;
} //req_abort

// --------------------------------------------------------------
// rollback_txn
// Purpose - Txn Thread specific processing for abort processing.
// --------------------------------------------------------------
bool TM_TX_Info::rollback_txn(CTmTxMessage * pp_msg)
{

   TMTrace (2, ("TM_TX_Info::rollback_txn : ID %d, ENTRY\n", 
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

         TMTrace (1, ("TM_TX_Info::rollback_txn : DTM%d txThread could not abort "
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

   state_change(TX_ROLLBACK, gv_tm_info.nid(), gv_tm_info.pid(), pp_msg);

   TMTrace (2, ("TM_TX_Info::rollback_txn, EXIT\n"));
   
   return false; //Don't terminate txn thread, still need to process the
                 //TM_MSG_TXINTERNAL_ABORTCOMPLETE.
} //rollback_txn

// --------------------------------------------------------------
// redriverollback_txn
// Purpose - Txn Thread specific processing to redrive a rollback.
// --------------------------------------------------------------
bool TM_TX_Info::redriverollback_txn(CTmTxMessage * pp_msg)
{
   bool lv_terminate = false;

   TMTrace (2, ("TM_TX_Info::redriverollback_txn : ID %d, ENTRY\n", seqnum()));

   if (iv_tx_state == TM_TX_STATE_HUNGABORTED)
      gv_tm_info.dec_tx_hung_count();

   // If we have a hung event timer then remove it now.
   if (ip_hung_event)
   {
      gv_xaTM.tmTimer()->cancelEvent(ip_hung_event);
      ip_hung_event = NULL;
   }

   // We've already driven phase 1, redrive phase 2.
   state_change(TX_REDRIVE_ROLLBACK, gv_tm_info.nid(), gv_tm_info.pid(), pp_msg);

   
   if (tx_state() != TM_TX_STATE_HUNGABORTED)
      stats()->txnAbort()->stop();

   if (gv_tm_info.mode() == TM_NONSYNC_MODE)
      lv_terminate = req_forget(pp_msg);

   TMTrace (2, ("TM_TX_Info::redriverollback_txn, EXIT returning %s\n", (lv_terminate)?"TRUE":"FALSE"));
   return lv_terminate;
} //redriverollback_txn

// --------------------------------------------------------------
// redrivecommit_txn
// Purpose - Txn Thread specific processing to redrive a commit.
// --------------------------------------------------------------
bool TM_TX_Info::redrivecommit_txn(CTmTxMessage * pp_msg)
{
   bool lv_terminate = false;

   TMTrace (2, ("TM_TX_Info::redrivecommit_txn : ID %d, ENTRY, current txn state %d\n", 
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
   state_change(TX_REDRIVE_COMMIT, gv_tm_info.nid(), gv_tm_info.pid(), pp_msg);

   if (tx_state() != TM_TX_STATE_HUNGCOMMITTED)
      stats()->txnCommit()->stop();

   if (gv_tm_info.mode() == TM_NONSYNC_MODE)
      lv_terminate = req_forget(pp_msg);

   TMTrace (2, ("TM_TX_Info::redrivecommit_txn, EXIT returning %s\n", (lv_terminate)?"TRUE":"FALSE"));
   return lv_terminate;
} //redrivecommit_txn

// ----------------------------------------------------------------
// internal_abortTrans
// Purpose - Queue a rollback request for this transaction.
// This is currently only used during takeover, shutdown and
// process death.
// ----------------------------------------------------------------
int32 TM_TX_Info::internal_abortTrans(bool pv_takeover_or_shutdown)
{ 
   CTmTxMessage * lp_msg = new CTmTxMessage(TM_MSG_TXINTERNAL_ROLLBACK);

   lp_msg->request()->u.iv_rollback_internal.iv_takeover_or_shutdown = pv_takeover_or_shutdown;

   queueToTransaction(&iv_transid, lp_msg);

   gv_tm_info.inc_abort_count();
   gv_tm_info.inc_tm_initiated_aborts();

   TMTrace (2, ("TM_TX_Info::internal_abortTrans : EXIT\n"));
    
   return FEOK;
} //internal_abortTrans


// ----------------------------------------------------------------
// redrive_rollback
// Purpose - Queue a redrive rollback request for this transaction.
// This is currently only used during a takeover.
// ----------------------------------------------------------------
int32 TM_TX_Info::redrive_rollback()
{
   CTmTxMessage * lp_msg = new CTmTxMessage(TM_MSG_TXINTERNAL_REDRIVEROLLBACK);

   queueToTransaction(&iv_transid, lp_msg);

    TMTrace (2, ("TM_TX_Info::redrive_rollback : EXIT\n"));
    
   return FEOK;
} //redrive_rollback

// ----------------------------------------------------------------
// redrive_commit
// Purpose - Queue a redrive commit request for this transaction.
// This is currently only used during a takeover.
// ----------------------------------------------------------------
int32 TM_TX_Info::redrive_commit()
{
   CTmTxMessage * lp_msg = new CTmTxMessage(TM_MSG_TXINTERNAL_REDRIVECOMMIT);

   queueToTransaction(&iv_transid, lp_msg);

   TMTrace (2, ("TM_TX_Info::redrive_commit : EXIT\n"));
    
   return FEOK;
} //redrive_commit

// --------------------------------------------------------------
// req_abort_complete
// Purpose - Txn Thread specific processing for ABORTTRANSACTION.
//            phase 2 (the real work).
// --------------------------------------------------------------
bool TM_TX_Info::req_abort_complete(CTmTxMessage * pp_msg)
{
   TMTrace (2, ("TM_TX_Info::req_abort_complete : ID %d, ENTRY\n", seqnum()));
   pp_msg->validate();


   if (tx_state() != TM_TX_STATE_FORGETTING &&
       tx_state() != TM_TX_STATE_FORGOTTEN)
      state_change(TX_ROLLBACK, gv_tm_info.nid(), gv_tm_info.pid(), pp_msg);
   else
      TMTrace (3, ("TM_TX_Info:req_abort_complete : tx state is FORGETTING or "
                   "FORGOTTEN so nothing to do here, exiting.\n"));

   if (tx_state() != TM_TX_STATE_HUNGABORTED)
      stats()->txnAbort()->stop();

   pp_msg->validate();

   // If we haven't replied yet, do so now - before we forget the outcome!
   if (pp_msg->replyPending())
       pp_msg->reply(); // reply with whatever error has been set

   TMTrace (2, ("TM_TX_Info::req_abort_complete : ID %d, EXIT\n", seqnum()));

   return false; //Don't terminate transaction, still need to do forget

}

// --------------------------------------------------------------
// req_end_complete
// Purpose - Txn Thread specific processing for ENDTRANSACTION.
//            phase 2 (the real work).
// --------------------------------------------------------------
bool TM_TX_Info::req_end_complete(CTmTxMessage * pp_msg)
{
   TMTrace (2, ("TM_TX_Info::req_end_complete : ID %d, ENTRY\n", seqnum()));


   if (isAborting())
   {
      if((pp_msg->responseError() != FELOCKED) && (pp_msg->responseError() != FEHASCONFLICT))
         pp_msg->responseError(FEABORTEDTRANSID);
      state_change(TX_ROLLBACK, gv_tm_info.nid(), gv_tm_info.pid(), pp_msg);
   }
   else
      state_change(TX_COMMIT, gv_tm_info.nid(), gv_tm_info.pid(), pp_msg);

   if (tx_state() != TM_TX_STATE_HUNGCOMMITTED)
      stats()->txnCommit()->stop();

   if (pp_msg->replyPending())  
      pp_msg->reply(FEOK);  


   TMTrace (2, ("TM_TX_Info::req_end_complete : ID %d, EXIT\n", seqnum()));

   return false; //Don't terminate transaction, still need to do forget
}

// --------------------------------------------------------------
// req_forget
// Purpose - Txn Thread specific processing for ENDTRANSACTION,
//           ABORTTRANSACTION and DOOMTRANSACTION.
//           Forget the transaction.
// --------------------------------------------------------------
bool TM_TX_Info::req_forget(CTmTxMessage * pp_msg)
{
   bool lv_terminate = true; // Normal processing is for forget to 
                             // drive thread disassociation for 
                             // transaction threads.
   TMTrace (2, ("TM_TX_Info::req_forget : ID %d, ENTRY\n", seqnum()));

   // If the transaction is in a hung state, we don't want to forget it.
   // We will wait for the commit/rollback to be retried, or for operator
   // intervention.
   if (tx_state() == TM_TX_STATE_HUNGCOMMITTED ||
       tx_state() == TM_TX_STATE_HUNGABORTED)
   {
      TMTrace (1, ("TM_TX_Info::req_forget : Ignoring transaction ID %d in hungAborted state.\n",
               seqnum()));
      lv_terminate = false; // Don't cleanup transaction object yet
   }
   else
   {
       if (tx_state() == TM_TX_STATE_FORGETTING_HEUR ||
           tx_state() == TM_TX_STATE_FORGOTTEN_HEUR)
       {
          TMTrace (1, ("TM_TX_Info::req_forget : Transaction ID %d in heuristicForgetting state, sending xa_forget.\n",
                   seqnum()));
          state_change(TX_FORGET, gv_tm_info.nid(), gv_tm_info.pid(), pp_msg);
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
      TMTrace (3, ("TM_TX_Info::req_forget : ID %d, terminate=%d, txnBusy=%d, "
               "pending requests, not cleaning up transaction.\n", 
               seqnum(), lv_terminate, lv_txnBusy));


   TMTrace (2, ("TM_TX_Info::req_forget : ID %d, txnBusy %d, terminate %d EXIT\n", 
      seqnum(), lv_txnBusy, lv_terminate));
   return lv_terminate; //Finished with the transaction, disassociate and cleanup
} // req_forget


bool TM_TX_Info::req_begin_complete(CTmTxMessage * pp_msg)
{
    TMTrace (2, ("TM_TX_Info::req_begin_complete : ID %d, ENTRY, error=%d\n", 
                    seqnum(), pp_msg->responseError()));

#ifdef DEBUG_MODE
   // This delay is used to test correct API error handling when a begintransaction
   // is still in progress.
   bool lv_test = false;
   ms_getenv_bool("TM_TEST_BEGIN_COMPLETE_DELAY", &lv_test);
   if( lv_test)
   {
       TMTrace (1, ("TM_TX_Info::req_begin_complete : TM_TEST_BEGIN_COMPLETE_DELAY"
                      " testpoint detected, delaying 3 seconds.\n"));
      ip_Thread->sleep(1000*3); //3 sec
   }
#endif

    state_change(TX_BEGIN_COMPLETE, gv_tm_info.nid(), gv_tm_info.pid(), pp_msg);
    reset_transactionBusy();
    stats()->txnBegin()->stop();

    TMTrace (2, ("TM_TX_Info::req_begin_complete : ID %d, EXIT\n", seqnum()));

    return false; //Leave transaction thread alive
}


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
short TM_TX_Info::doom_txn()
{
   short lv_error = FEOK;

   TMTrace (2, ("TM_TX_Info::doom_txn ENTRY, ID %d, state %d.\n",
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

    TMTrace (2, ("TM_TX_Info::doom_txn, EXIT, ID %d, error %d, state %d, %s.\n",
                   seqnum(), lv_error, tx_state(), ((iv_mark_for_rollback)?"rollback deferred":"rolled back")));

   return lv_error;
} //doom_txn


bool TM_TX_Info::redrive_sync(CTmTxMessage * pp_msg)
{
    TMTrace (2, ("TM_TX_Info::schedule_redrive_sync, state %d, ENTER.\n",
                  iv_tx_state));

    switch (iv_tx_state)
    {
        case TM_TX_STATE_BEGINNING:
        {
            state_change(TX_BEGIN, gv_tm_info.nid(), gv_tm_info.pid(), pp_msg);
            break;
        }
        case TM_TX_STATE_COMMITTED:
        {
             iv_tx_state = TM_TX_STATE_COMMITTING;
             state_change(TX_COMMIT, gv_tm_info.nid(), gv_tm_info.pid(), pp_msg);
             break;
        }
        case TM_TX_STATE_ABORTED:
        case TM_TX_STATE_ABORTING_PART2:
        {
             iv_tx_state = TM_TX_STATE_ABORTING;
             state_change(TX_ROLLBACK, gv_tm_info.nid(), gv_tm_info.pid(), pp_msg);
             break;
        }
        case TM_TX_STATE_FORGOTTEN:
             tx_state(TM_TX_STATE_FORGETTING);
             // Intentional fall through.
        case TM_TX_STATE_FORGETTING_HEUR:
        {
             state_change(TX_FORGET, gv_tm_info.nid(), gv_tm_info.pid(), pp_msg);
             break;
        }
        default:
        {
            tm_log_event(TM_TMTX_INVALID_STATE, SQ_LOG_CRIT, "TM_TMTX_INVALID_STATE",
                    -1, /*error_code*/ 
                    -1, /*rmid*/
                    -1, /*dtmid*/ 
                    seqnum()); /*seq_num*/

            TMTrace (1, ("TM_TX_Info::redrive_sync - tx state %d: Invalid transaction state. "
                         "Transaction seq num %d\n", iv_tx_state,seqnum()));
            abort();
        }
    };

    TMTrace (2, ("TM_TX_Info::schedule_redrive_sync EXIT.\n"));

    return false; // Don't dispose of transaction and thread objects
} //redrive_sync
