
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#include "seabed/ms.h"
#include "seabed/trace.h"
#include "tmlogging.h"

#include "tmtx.h"
#include "tmrmhbase.h"
#include "tminfo.h"
#include "hbasetm.h"
#include "tmtxbranches.h"


// ----------------------------------------------------------------
// CTmTxBranches::num_rm_failed
// Purpose : Count and return the number of RMs in failed
// state.  Currently returning 0.
// Periodically the TM will attempt to reopen any failed
// RMs.  This is done by the Timer thread.
// ----------------------------------------------------------------
int32 CTmTxBranches::num_rm_failed(CTmTxBase *pp_txn)
{
   int32 lv_count = 0;

   TMTrace (2, ("CTmTxBranches::num_rm_failed ENTRY.\n"));

   lv_count = iv_TSEBranches.num_rm_failed(pp_txn);
   lv_count += iv_HBASEBranches.num_rm_failed(pp_txn);

   TMTrace (2, ("CTmTxBranches::num_rm_failed EXIT Returned %d failed RMs.\n", lv_count));
   return lv_count;
} //CTmTxBranches::num_rm_failed


// ----------------------------------------------------------------
// CTmTxBranches::num_rm_partic
// Purpose : Returns the number of participating branches.
// ----------------------------------------------------------------
int32 CTmTxBranches::num_rm_partic(CTmTxBase *pp_txn)
{
   int32 lv_count = 0;

   lv_count = iv_TSEBranches.num_rm_partic(pp_txn);
   lv_count += iv_HBASEBranches.num_rm_partic(pp_txn);

   TMTrace (2, ("CTmTxBranches::num_rm_partic EXIT Returned %d participating RMs.\n", lv_count));
   return lv_count;
} //CTmTxBranches::num_rm_partic


// ----------------------------------------------------------------
// CTmTxBranches::num_rms_unresolved
// Purpose : Returns the number of RMs/branches which have the 
// iv_partic flag set but don't have the iv_resolved flag set.
// This is used to identify late TSE checkins (ax_reg requests)
// while processing phase 1 & 2.
// ----------------------------------------------------------------
int32 CTmTxBranches::num_rms_unresolved(CTmTxBase *pp_txn)
{
   int32 lv_count = 0;

   lv_count = iv_TSEBranches.num_rms_unresolved(pp_txn);
   lv_count += iv_HBASEBranches.num_rms_unresolved(pp_txn);

   TMTrace (2, ("CTmTxBranches::num_rms_unresolved EXIT Returned %d unresolved RMs.\n", lv_count));
   return lv_count;
} //CTmTxBranches::num_rms_unresolved


// ----------------------------------------------------------------
// CTmTxBranches::reset_resolved
// Purpose : Reset all branches resolved flags.  This is used to 
// reset the flags after prepare phase to all commit_branches() to
// find any ax_reg requests which arrive during phase 2.
// ----------------------------------------------------------------
void CTmTxBranches::reset_resolved(CTmTxBase *pp_txn)
{
   iv_TSEBranches.reset_resolved(pp_txn);
   iv_HBASEBranches.reset_resolved(pp_txn);

   TMTrace (2, ("CTmTxBranches::reset_resolved EXIT.\n"));
} //CTmTxBranches::reset_resolved


// ----------------------------------------------------------------
// CTmTxBranches::init_rms
// Purpose : Initialize branches
// ----------------------------------------------------------------
void CTmTxBranches::init_rms(CTmTxBase *pp_txn, bool pv_partic)
{
   TMTrace (2, ("CTmTxBranches::init_rms ENTRY Txn ID (%d,%d), partic? %d.\n",
            pp_txn->node(), pp_txn->seqnum(), pv_partic));
   iv_TSEBranches.init_rms(pp_txn, pv_partic);
   iv_HBASEBranches.init_rms(pp_txn, pv_partic);

   TMTrace (2, ("CTmTxBranches::init_rms EXIT.\n"));
} //CTmTxBranches::init_rms



// --------------------------------------------------------------
// Branch stuff below
// --------------------------------------------------------------

// ---------------------------------------------------------------------------
// rollback_branches
// Purpose : Pass the rollback request to HBase TM Library
// ---------------------------------------------------------------------------
int32 CTmTxBranches::rollback_branches (CTmTxBase *pp_txn,
                                        int64 pv_flags,
                                        CTmTxMessage * pp_msg,
                                        bool pv_error_condition)
{
   int32 lv_err = FEOK, lv_err1 = RET_OK;

   TMTrace (2, ("CTmTxBranches::rollback_branches, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
                pp_txn->node(), pp_txn->seqnum(), pv_flags));

   iv_HBASEBranches.rollback_branches(pp_txn, pv_flags, pp_msg, pv_error_condition);
   iv_TSEBranches.rollback_branches(pp_txn, pv_flags, pp_msg, pv_error_condition);

   // For situations where the only branches hare in HBase, we can reply to the application which called abort now and
   // then wait for the aborts to complete.  If we had other branches, then we may have already replied!
   if (pp_msg->replyPending())
      pp_msg->reply();
   // Now wait for the HBase library to complete it's work.
   lv_err1 = completeRequest_branches(pp_txn);
   lv_err = (lv_err1!=RET_OK)?XAER_RMERR:XA_OK;


   TMTrace (2, ("CTmTxBranches::rollback_branches, Txn ID (%d,%d), EXIT, "
            "UnResolved branches %d, return error %d, HBase client error %d.\n",
            pp_txn->node(), pp_txn->seqnum(), num_rms_unresolved(pp_txn), lv_err, lv_err1));
   return lv_err;
} //rollback_branches


// ---------------------------------------------------------------------------
// commit_branches
// Purpose : Send out commit (phase 2) to HBase TM Library.
// ---------------------------------------------------------------------------
int32 CTmTxBranches::commit_branches (CTmTxBase *pp_txn,
                                      int64 pv_flags, CTmTxMessage * pp_msg)
{
   int32 lv_err = XA_OK, lv_err1 = RET_OK, lv_err2 = XA_OK;

   TMTrace (2, ("CTmTxBranches::commit_branches, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
                pp_txn->node(), pp_txn->seqnum(), pv_flags));

   lv_err1 = iv_HBASEBranches.commit_branches(pp_txn, pv_flags, pp_msg);
   lv_err2 = iv_TSEBranches.commit_branches(pp_txn, pv_flags, pp_msg);

   lv_err = (lv_err1!=RET_OK)?XAER_RMERR:((lv_err2!=XA_OK)?lv_err2:XA_OK);

   TMTrace (2, ("CTmTxBranches::commit_branches, Txn ID (%d,%d), EXIT, HBase branch returned %d, "
            "TSE Branch returned %d, Composite result %d. UnResolved branches %d.\n",
            pp_txn->node(), pp_txn->seqnum(), lv_err1, lv_err2, lv_err, num_rms_unresolved(pp_txn)));
   return lv_err;
} // commit_branches


// ---------------------------------------------------------------------------
// completeRequest_branches
// Purpose : Wait for Phase 2 to complete in the HBase TM Library.
// This isn't required for other branch types at this time because
// they reply early directly within the branch code.
// ---------------------------------------------------------------------------
int32 CTmTxBranches::completeRequest_branches (CTmTxBase *pp_txn)
{
   int32 lv_err = XA_OK, lv_err1 = RET_OK;

   TMTrace (2, ("CTmTxBranches::completeRequest_branches, Txn ID (%d,%d), ENTRY\n",
                pp_txn->node(), pp_txn->seqnum()));

   lv_err1 = iv_HBASEBranches.completeRequest_branches(pp_txn);
   
   lv_err = (lv_err1!=RET_OK)?XAER_RMERR:XA_OK;

   TMTrace (2, ("CTmTxBranches::completeRequest_branches, Txn ID (%d,%d), EXIT, HBase branch returned %d.\n",
            pp_txn->node(), pp_txn->seqnum(), lv_err1));
   return lv_err;
} // completeRequest_branches


// ---------------------------------------------------------------------------
// end_branches
// Purpose - Doesn't really do anything for HBase TM Library
// ---------------------------------------------------------------------------
int32 CTmTxBranches::end_branches (CTmTxBase *pp_txn,
                                   int64 pv_flags)
{
   int32 lv_err = FEOK;

   TMTrace (2, ("CTmTxBranches::end_branches, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
                pp_txn->node(), pp_txn->seqnum(), pv_flags));

   iv_HBASEBranches.end_branches(pp_txn, pv_flags);
   lv_err = iv_TSEBranches.end_branches(pp_txn, pv_flags);

   TMTrace (2, ("CTmTxBranches::end_branches, Txn ID (%d,%d), EXIT, UnResolved branches %d.\n",
                pp_txn->node(), pp_txn->seqnum(), num_rms_unresolved(pp_txn)));
   return lv_err;
} //end_branches

// ---------------------------------------------------------------
// forget_heur_branches
// Purpose : Heuristic forget
// --------------------------------------------------------------
int32 CTmTxBranches::forget_heur_branches (CTmTxBase *pp_txn,
                                           int64 pv_flags)
{
    int32 lv_error = FEOK;

    TMTrace (2, ("CTmTxBranches::forget_heur_branches ENTRY : ID (%d,%d), flags " PFLL "\n",
             pp_txn->node(), pp_txn->seqnum(), pv_flags));
    tm_log_event(DTM_TMTX_FORGET_HEURISTIC, SQ_LOG_WARNING, "DTM_TMTX_FORGET_HEURISTIC",
                 -1,-1,pp_txn->node(), pp_txn->seqnum());
    gv_tm_info.write_trans_state (pp_txn->transid(), TM_TX_STATE_FORGOTTEN_HEUR, pp_txn->abort_flags(), false);
    pp_txn->wrote_trans_state(true);
    if (pp_txn->tx_state() != TM_TX_STATE_FORGOTTEN_HEUR)
    {
       lv_error = forget_branches (pp_txn, pv_flags);
       switch (lv_error)
       {
         case XA_OK:
            pp_txn->tx_state(TM_TX_STATE_FORGOTTEN_HEUR);
            break;
         case XAER_RMFAIL:
            //TODO recovery case, what to do here?
         abort();
         default:
            tm_log_event(DTM_TMTX_INVALID_BRANCH, SQ_LOG_CRIT, "DTM_TMTX_INVALID_BRANCH");
            TMTrace (1, ("CTmTxBranches::forget_heur_branches - Invalid branch state\n"));
            abort (); 
         break;
       }
    }
    return lv_error;
} // forget_heur_branches

// ---------------------------------------------------------------
// forget_branches
// Purpose : all RMs have responded, so forget this tx.
// Not passed to HBase TM Library.
// --------------------------------------------------------------
int32 CTmTxBranches::forget_branches (CTmTxBase *pp_txn,
                                      int64 pv_flags)
{
   int32 lv_err = FEOK;

   TMTrace (2, ("CTmTxBranches::forget_branches, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
                pp_txn->node(), pp_txn->seqnum(), pv_flags));

   iv_TSEBranches.forget_branches(pp_txn, pv_flags);
   iv_HBASEBranches.forget_branches(pp_txn, pv_flags);

   TMTrace (2, ("CTmTxBranches::forget_branches, Txn ID (%d,%d), EXIT, UnResolved branches %d.\n",
                pp_txn->node(), pp_txn->seqnum(), num_rms_unresolved(pp_txn)));
   return lv_err;
} //forget_branches


// ------------------------------------------------------------
// prepare_branches
// Purpose : Send prepare to HBase TM Library
// Only return read-only if all subordinate branch 
// types return read-only.  Other than that, we
// always return an error response from non-HBase
// branches to maintain compatibility with
// older code.
//                               HBase branches return
//                               Ok    ReadOnly    Error1
//                            +----------------------------------
// RM            Ok       | Ok      Ok            Error1
// branches    RO       | Ok      RO            Error1
// return        Error2  | Error2  Error2      Error2
// ------------------------------------------------------------
int32 CTmTxBranches::prepare_branches (CTmTxBase *pp_txn,
                                       int64 pv_flags,
                                       CTmTxMessage *pp_msg)
{
   int32 lv_err = XA_OK, lv_err1 = RET_OK, lv_err2 = XA_OK;

   TMTrace (2, ("CTmTxBranches::prepare_branches, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
                pp_txn->node(), pp_txn->seqnum(), pv_flags));

   lv_err1 = iv_HBASEBranches.prepare_branches(pp_txn, pv_flags, pp_msg);
   lv_err2 = iv_TSEBranches.prepare_branches(pp_txn, pv_flags, pp_msg);

   // The transaction is only read-only if both branch types return read-only.
   if (lv_err1 == RET_READONLY && 
       (lv_err2 == XA_RDONLY || lv_err2 == XAER_NOTA))
     //  Right now we return XA_OK here and the transaction state
     // machine works out whether there are any participants left
     // in the transaction and decides whether the transaction
     // is read-only on that basis.
     // lv_err = XA_RDONLY;
     lv_err = XA_OK;
   else
   {
      // If either of the branch types returned read-only then clear because both
      // must be read-only.
      if (lv_err1 == RET_READONLY)
         lv_err1 = RET_OK;
      if (lv_err2 == XA_RDONLY || lv_err2 == XAER_NOTA)
         lv_err2 = XA_OK;
      //lv_err = (lv_err1!=RET_OK)?XAER_RMERR:((lv_err2!=XA_OK)?lv_err2:XA_OK);
      lv_err = (lv_err2!=RET_OK)?XAER_RMERR:((lv_err1!=XA_OK)?lv_err1:XA_OK);
   }

   // If an error occurred then make sure we drive rollback
   if (lv_err != XA_OK && lv_err != XA_RDONLY)
      pp_txn->mark_for_rollback(true);

   TMTrace (2, ("CTmTxBranches::prepare_branches, Txn ID (%d,%d), EXIT, HBase branch returned %d, "
            "TSE Branch returned %d. Result %d, UnResolved branches %d.\n",
            pp_txn->node(), pp_txn->seqnum(), lv_err1, lv_err2, lv_err, num_rms_unresolved(pp_txn)));
   return lv_err;
} //prepare_branches


//------------------------------------------------------------------------------
// start_branches
// Purpose - Handles start request for all branches
// Library.
//------------------------------------------------------------------------------
int32 CTmTxBranches::start_branches (CTmTxBase *pp_txn, int64 pv_flags, CTmTxMessage * pp_msg)
{   
   int32 lv_err = FEOK;

   TMTrace (2, ("CTmTxBranches::start_branches, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
                pp_txn->node(), pp_txn->seqnum(), pv_flags));

   iv_TSEBranches.start_branches(pp_txn, pv_flags, pp_msg);
   iv_HBASEBranches.start_branches(pp_txn, pv_flags, pp_msg);

   TMTrace (2, ("CTmTxBranches::start_branches, Txn ID (%d,%d), EXIT, UnResolved branches %d.\n",
                pp_txn->node(), pp_txn->seqnum(), num_rms_unresolved(pp_txn)));
   return lv_err;
} // start_branches


//------------------------------------------------------------------------------
// registerRegion
// Purpose - Handles start request for all branches
// Library.
//------------------------------------------------------------------------------
int32 CTmTxBranches::registerRegion (CTmTxBase *pp_txn, int64 pv_flags, CTmTxMessage * pp_msg)
{   
   int32 lv_err = FEOK;

   TMTrace (2, ("CTmTxBranches::registerRegion, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
                pp_txn->node(), pp_txn->seqnum(), pv_flags));

   //iv_TSEBranches.registerRegion(pp_txn, pv_flags, pp_msg);
   lv_err = iv_HBASEBranches.registerRegion(pp_txn, pv_flags, pp_msg);

   TMTrace (2, ("CTmTxBranches::registerRegion, Txn ID (%d,%d), EXIT, UnResolved branches %d.\n",
                pp_txn->node(), pp_txn->seqnum(), num_rms_unresolved(pp_txn)));
   return lv_err;
} // registerRegion


//------------------------------------------------------------------------------
// ddlOperation
// Purpose - Handles ddlOperation requests 
// -----------------------------------------------------------------------------
int32 CTmTxBranches::ddlOperation(CTmTxBase *pp_txn, int64 pv_flags, CTmTxMessage * pp_msg)
{
   int32 lv_err = FEOK;

   TMTrace (2, ("CTmTxBranches::ddlOperation, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
                pp_txn->node(), pp_txn->seqnum(), pv_flags));

   lv_err = iv_HBASEBranches.hb_ddl_operation(pp_txn, pv_flags, pp_msg);

   TMTrace (2, ("CTmTxBranches::ddlOperation, Txn ID (%d,%d), EXIT, UnResolved branches %d.\n",
                pp_txn->node(), pp_txn->seqnum(), num_rms_unresolved(pp_txn)));

   return lv_err;
}


//------------------------------------------------------------------------------
// shutdown_branches
// Purpose - shutdown all branches.
//------------------------------------------------------------------------------
int32 CTmTxBranches::shutdown_branches (bool pv_leadTM, bool pv_clean)
{   
   int32 lv_error = FEOK, lv_err1 = RET_OK, lv_err2 = XA_OK;
   TMTrace (2, ("CTmTxBranches::shutdown_branches, ENTRY, lead TM %d, clean? %d.\n",
            pv_leadTM, pv_clean));

   lv_err1 = TSE()->shutdown_branches(pv_leadTM, pv_clean);
   lv_err2 = HBASE()->shutdown_branches(pv_leadTM, pv_clean);

   lv_error = (lv_err1!=RET_OK)?lv_err1:((lv_err2!=XA_OK)?XAER_RMERR:FEOK);
   TMTrace (2, ("CTmTxBranches::shutdown_branches  EXIT with error %d.\n", lv_error));
   return lv_error;
} // shutdown_branches

