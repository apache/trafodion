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
#include "tmrm.h"
#include "tminfo.h"
#include "xaglob.h"


// ----------------------------------------------------------------
// RM_Info_TSE::RM_Info_TSE 
// Purpose : Default constructor
// ----------------------------------------------------------------
RM_Info_TSE::RM_Info_TSE()
   :iv_high_index_used(0)
{
   TMTrace (2, ("RM_Info_TSE::RM_Info_TSE Default Constructor : ENTRY.\n"));
}


// ----------------------------------------------------------------
// RM_Info_TSE::init
// Purpose : Allocate and initialize a free slot in the ia_TSEBranches array
// for this RM.
// Note the ia_TSEBranches slot will be in an TSEBranch_DOWN state if this call
// completes successfully.
// Returns FEOK if the slot was successfully initialized.
//             FEFILEFULL if no slots are available.
// ----------------------------------------------------------------
int32 RM_Info_TSE::init (int32 pv_pid, int32 pv_nid, char *pp_name, int32 pv_rmid, 
                  bool pv_is_ax_reg, TSEBranch_state pv_state, bool pv_clean_init) 
{
    int lv_idx = 0;

    for (; lv_idx < MAX_OPEN_RMS; lv_idx++)
    {
        // if this slot is the first emtpy slot,
        // grab it
        if (!ia_TSEBranches[lv_idx].in_use())
        {
            ia_TSEBranches[lv_idx].init(pv_pid, pv_nid, pp_name, 
                                    pv_rmid, pv_is_ax_reg, pv_state);
            TMTrace(3, ("RM_Info_TSE::init Found free slot %d : RM pid(%d), nid(%d), rmid(%d), name(%s).\n",
                        lv_idx, pv_pid, pv_nid, pv_rmid, pp_name));

            inc_num_rm_partic();
            if (lv_idx > iv_high_index_used)
               iv_high_index_used = lv_idx;
            return FEOK;
        }
   }

    // no empty slots
    return FEFILEFULL;
} // RM_Info_TSE::init


// ----------------------------------------------------------------
// RM_Info_TSE::reinit
// Purpose : Reinitialize the slot in ia_TSEBranches for this RM.
// pv_state:  This should be set to 
//   TSEBranch_RECOVERING if the TSE crashed or 
//   TSEBranch_FAILOVER if a failover was detected.  
//   TSEBranch_RECOVERING will drive recovery of any indoubt 
//   (hung) transactions.  For failover we don't want to send the 
//   TSE an xa_recover as it already has all the details.  We do,
//   however want to redrive all hung transasctions.
// Returns: 
//   FEOK if successful, 
//   FENOTFOUND if it can't find the rmid was not found.
// ----------------------------------------------------------------
int32 RM_Info_TSE::reinit (int32 pv_pid, int32 pv_nid, char *pp_name, int32 pv_rmid, 
                  bool pv_is_ax_reg, bool pv_clean_init, TSEBranch_state pv_state) 
{
   TMTrace(3, ("RM_Info_TSE::reinit ENTER : RM pid(%d), nid(%d), rmid(%d), name(%s), state(%d).\n",
               pv_pid, pv_nid, pv_rmid, pp_name, pv_state));
   int lv_idx = 0;
   for (; lv_idx < MAX_OPEN_RMS; lv_idx++)
   {
      if (ia_TSEBranches[lv_idx].rmid() == pv_rmid)
      {
         ia_TSEBranches[lv_idx].init(pv_pid, pv_nid, pp_name, 
                                 pv_rmid, pv_is_ax_reg, pv_state, pv_clean_init);
         return FEOK;
      }
   }
   // not found
   return FENOTFOUND;
} // RM_Info_TSE::reinit


void RM_Info_TSE::init_rms(CTmTxBase *pp_txn, bool pv_partic)
{
   TMTrace(2, ("RM_Info_TSE::init_rm ENTRY : Txn ID (%d,%d), partic? %d.\n",
           pp_txn->node(), pp_txn->seqnum() , pv_partic));
   iv_num_rm_partic=iv_high_index_used=0; // reinitialize

   for (int32 lv_idx=0; lv_idx< MAX_OPEN_RMS;lv_idx++)
   {
         if (gv_RMs.TSE()->ia_TSEBranches[lv_idx].in_use())
         {
              memcpy (&ia_TSEBranches[lv_idx], 
                      &gv_RMs.TSE()->ia_TSEBranches[lv_idx],
                      sizeof (RM_Info_TSEBranch));
              memcpy(&ia_TSEBranches[lv_idx].iv_xid.data,
                     pp_txn->transid(), sizeof (TM_Txid_Internal));

               // this is for the takeover case.  we want to make
               // sure we grab ALL rms, not just the non ax_reg ones.
               // Therefore, all up RMS will participate in the phase 1/2
               // for takeovers.
               if (pv_partic)
                   ia_TSEBranches[lv_idx].partic(true);
               else
                   ia_TSEBranches[lv_idx].partic(false);

               if (ia_TSEBranches[lv_idx].partic())
                   iv_num_rm_partic++;

               ia_TSEBranches[lv_idx].resolved(false);
               ia_TSEBranches[lv_idx].enlisted(false);
 
               if (lv_idx > iv_high_index_used)
                   iv_high_index_used = lv_idx;
         }
         else 
         {
            ia_TSEBranches[lv_idx].in_use(false);
            ia_TSEBranches[lv_idx].partic(false);
            ia_TSEBranches[lv_idx].resolved(false);
            ia_TSEBranches[lv_idx].enlisted(false);
         }
   } //for
   TMTrace(2, ("RM_Info_TSE::init_rm EXIT.\n"));
} // RM_Info_TSE::init_rm

void RM_Info_TSE::remove_rm_by_rmid(int32 pv_rmid) 
{
    for (int lv_idx = 0; lv_idx <= iv_high_index_used; lv_idx++)
    {
        if ((ia_TSEBranches[lv_idx].in_use() == true) &&
            (ia_TSEBranches[lv_idx].rmid() == pv_rmid))
            
        {
            ia_TSEBranches[lv_idx].partic(false);
            iv_num_rm_partic--;
            break;
        }
     } 
}

void RM_Info_TSE::remove_rm_by_index (int32 pv_index)
{
   if (ia_TSEBranches[pv_index].in_use() == true)
   {
        ia_TSEBranches[pv_index].partic(false);
        iv_num_rm_partic--;
   }
}

// If we calling this it means the RM sent us an ax_reg, so
// we can assume it is up even if previously marked down.
int RM_Info_TSE::set_partic_and_transid(TM_Txid_Internal pv_transid, int32 pv_rmid )
{
    bool lv_found = false;

    int lv_idx = 0;
    while (lv_idx <= iv_high_index_used && !lv_found)
    {
         // we must find this RM in our initialized array, then set
         // partic to true and set the transid
        if (ia_TSEBranches[lv_idx].rmid() == pv_rmid)
        {
            ia_TSEBranches[lv_idx].add_partic(pv_transid);
            lv_found = true;
            iv_num_rm_partic++;

         ia_TSEBranches[lv_idx].up();
        }
        else
           lv_idx++;
    }

    if (lv_found)
    {
       TMTrace(3, ("RM_Info_TSE::set_partic_and_transid EXIT : Txn ID (%d,%d) participation set for RM %d.\n",
               pv_transid.iv_node, pv_transid.iv_seq_num, pv_rmid));
       return lv_idx;
    }
    else
      // not found, -1 is an indicator that a slot wasn't found
    {
       TMTrace(3, ("RM_Info_TSE::set_partic_and_transid EXIT : Txn ID (%d,%d) participation not set for rmid(%d), "
                   "rm not found in ia_TSEBranches.\n",
                   pv_transid.iv_node, pv_transid.iv_seq_num, pv_rmid));
      return -1;
    }
}


// ----------------------------------------------------------------
// RM_Info_TSE::num_rm_failed
// Purpose : Count and return the number of RMs in TSEBranch_FAILED.
// Periodically the TM will attempt to reopen any failed
// RMs.  This is done by the Timer thread.
// ----------------------------------------------------------------
int32 RM_Info_TSE::num_rm_failed(CTmTxBase *pp_txn)
{
   int32 lv_count = 0;

   TMTrace (2, ("RM_Info_TSE::num_rm_failed ENTRY\n"));

   for (int i=0; i<=iv_high_index_used; i++)
   {
      //TMTrace (3, ("RM_Info_TSE::num_rm_failed index %d, state %d\n", i, ia_TSEBranches[i].state()));
      if (ia_TSEBranches[i].state() != TSEBranch_UP)
         lv_count++;
   }

   TMTrace (2, ("RM_Info_TSE::num_rm_failed EXIT Returned %d failed RMs.\n", lv_count));
   return lv_count;
} //RM_Info_TSE::num_rm_failed


// ----------------------------------------------------------------
// RM_Info_TSE::num_rms_partic
// Purpose : Returns the number of RMs/branches which have the 
// iv_partic flag set.
// ----------------------------------------------------------------
int32 RM_Info_TSE::num_rm_partic(CTmTxBase *pp_txn)
{
   int32 lv_count = 0;

   for (int i=0; i<=iv_high_index_used; i++)
   {
      TMTrace (3, ("RM_Info_TSE::num_rms_unresolved index %d, in use %d, partic %d, resolved %d\n", i, 
               ia_TSEBranches[i].in_use(), ia_TSEBranches[i].partic(), ia_TSEBranches[i].resolved()));
      if (ia_TSEBranches[i].partic())
         lv_count++;
   }

   TMTrace (2, ("RM_Info_TSE::num_rm_partic EXIT Returned %d participating RMs.\n", lv_count));
   if (iv_num_rm_partic != lv_count)
      TMTrace(1, ("RM_Info_TSE::num_rm_partic ERROR: Aggregated partic count %d != actual count %d!\n",
              iv_num_rm_partic, lv_count));
   return lv_count;
} //RM_Info_TSE::num_rm_partic


// ----------------------------------------------------------------
// RM_Info_TSE::num_rms_unresolved
// Purpose : Returns the number of RMs/branches which have the 
// iv_partic flag set but don't have the iv_resolved flag set.
// This is used to identify late TSE checkins (ax_reg requests)
// while processing phase 1 & 2.
// ----------------------------------------------------------------
int32 RM_Info_TSE::num_rms_unresolved(CTmTxBase *pp_txn)
{
   int32 lv_count = 0, lv_partic_count = 0;

   TMTrace (2, ("RM_Info_TSE::num_rms_unresolved ENTRY, participants=%d.\n", iv_num_rm_partic));

   branch_lock();   

   for (int i=0; i<=iv_high_index_used; i++)
   {
#ifdef DEBUG_MODE
      TMTrace (3, ("RM_Info_TSE::num_rms_unresolved index %d, rmid %d, in use %d, partic %d, resolved %d\n", i, 
               ia_TSEBranches[i].rmid(), ia_TSEBranches[i].in_use(), ia_TSEBranches[i].partic(), ia_TSEBranches[i].resolved()));
#endif //DEBUG_MODE
      if (ia_TSEBranches[i].partic())
      {
         lv_partic_count++;
         if (!ia_TSEBranches[i].in_use())
            TMTrace (3, ("RM_Info_TSE::num_rms_unresolved ERROR: participant %d is not inuse!\n", i));
      }

      // Don't care here whether the RM is up or not, just whether it participated but is still to be resolved.
      if (ia_TSEBranches[i].partic() && !ia_TSEBranches[i].resolved())
         lv_count++;
   }

   branch_unlock();

   TMTrace (2, ("RM_Info_TSE::num_rms_unresolved EXIT Returned %d unresolved RMs.\n", lv_count));
   if (iv_num_rm_partic != lv_partic_count)
      TMTrace(1, ("RM_Info_TSE::num_rms_unresolved ERROR: Aggregated partic count %d != actual count %d!\n",
              iv_num_rm_partic, lv_partic_count));
   return lv_count;
} //RM_Info_TSE::num_rms_unresolved


// ----------------------------------------------------------------
// RM_Info_TSE::reset_resolved
// Purpose : Reset all branches resolved flags.  This is used to 
// reset the flags after prepare phase to all commit_branches() to
// find any ax_reg requests which arrive during phase 2.
// ----------------------------------------------------------------
void RM_Info_TSE::reset_resolved(CTmTxBase *pp_txn)
{
   TMTrace (2, ("RM_Info_TSE::reset_resolved ENTRY\n"));

   for (int i=0; i<=iv_high_index_used; i++)
   {
      ia_TSEBranches[i].resolved(false);
   }

   TMTrace (2, ("RM_Info_TSE::reset_resolved EXIT.\n"));
} //RM_Info_TSE::reset_resolved


void RM_Info_TSE::fail_rm(int32 pv_rmid)
{
   int lv_idx = 0;
   TMTrace (2, ("RM_Info_TSE::fail_rm by rmid ENTRY, rmid=%d\n", pv_rmid));

    for (; lv_idx <= iv_high_index_used; lv_idx++)
    {
        if (ia_TSEBranches[lv_idx].rmid() == pv_rmid)
            
        {
            ia_TSEBranches[lv_idx].state(TSEBranch_FAILED);
            break;
        }
     } 

   TMTrace (2, ("RM_Info_TSE::fail_rm by rmid EXIT failed rm index %d.\n", lv_idx));
} //RM_Info_TSE::fail_rm


// ----------------------------------------------------------------
// RM_Info_TSE::fail_rm by nid,pid
// Purpose : Mark this rm as TSEBranch_FAILED in ia_TSEBranches
// ----------------------------------------------------------------
void RM_Info_TSE::fail_rm(int32 pv_nid, int32 pv_pid)
{
   int lv_idx = 0;
   TMTrace (2, ("RM_Info_TSE::fail_rm  by nid,pid ENTRY, nid=%d, pid=%d\n", 
            pv_nid, pv_pid));

    for (; lv_idx <= iv_high_index_used; lv_idx++)
    {
        TMTrace (4, ("RM_Info_TSE::fail_rm  by nid,pid index %d, nid=%d, "
                 "pid=%d, rmid=%d, state=%d\n", 
                 lv_idx, ia_TSEBranches[lv_idx].nid(), ia_TSEBranches[lv_idx].pid(), 
                 ia_TSEBranches[lv_idx].rmid(), ia_TSEBranches[lv_idx].state()));
        if ((ia_TSEBranches[lv_idx].nid() == pv_nid) &&
            (ia_TSEBranches[lv_idx].pid() == pv_pid))
            
        {
            ia_TSEBranches[lv_idx].state(TSEBranch_FAILED);
            break;
        }
     } 

   TMTrace (2, ("RM_Info_TSE::fail_rm by nid,pid EXIT failed rm index %d, rmid %d.\n", 
            lv_idx, ia_TSEBranches[lv_idx].rmid()));
} //RM_Info_TSE::fail_rm


RM_Info_TSEBranch *RM_Info_TSE::return_slot(int32 pv_rmid)
{
   int32 lv_midx = 0;

   while (lv_midx < MAX_OPEN_RMS && ia_TSEBranches[lv_midx].rmid() != pv_rmid)
      lv_midx++;
   
   if (lv_midx == MAX_OPEN_RMS)
    {
      TMTrace(2, ("RM_Info_TSE::return_slot EXIT : No entry in ia_TSEBranches found for rmid(%d).\n",
                         pv_rmid));
      return 0;
    }
   else
    {
      TMTrace(4, ("RM_Info_TSE::return_slot EXIT : Slot %d matches rmid(%d).\n",
                         lv_midx, pv_rmid));
      return (RM_Info_TSEBranch *) &ia_TSEBranches[lv_midx];
    }
}


// ----------------------------------------------------------------
// return_slot_index
// Looks up the rmid in ia_TSEBranches and returns the index rather than a
// pointer to the RM_Info_TSEBranch object.
// Returns -1 if it fails to find the RM.
// ----------------------------------------------------------------
int32 RM_Info_TSE::return_slot_index(int32 pv_rmid)
{
   int32 lv_midx = 0;

   while (lv_midx < MAX_OPEN_RMS && ia_TSEBranches[lv_midx].rmid() != pv_rmid)
      lv_midx++;
   
   if (lv_midx == MAX_OPEN_RMS)
    {
      TMTrace(2, ("RM_Info_TSE::return_slot_index EXIT : No entry in ia_TSEBranches found for rmid(%d).\n",
                         pv_rmid));
      return -1;
    }
   else
    {
      TMTrace(4, ("RM_Info_TSE::return_slot_index EXIT : Slot %d matches rmid(%d).\n",
                         lv_midx, pv_rmid));
      return lv_midx;
    }
} //return_slot_index


// ----------------------------------------------------------------
// RM_Info_TSEBranch::return_slot by process name
// Purpose : Lookup an RM based on the process name
// This is used when checking for new TSEs and updating the status
// for restarted TSEs.
// ----------------------------------------------------------------
RM_Info_TSEBranch *RM_Info_TSE::return_slot(char *pp_name)
{
   int32 lv_midx = 0;

   while (lv_midx < MAX_OPEN_RMS && strcmp((char *) ia_TSEBranches[lv_midx].pname(), pp_name)!=0)
      lv_midx++;
   
   if (lv_midx == MAX_OPEN_RMS)
    {
      TMTrace(2, ("RM_Info_TSE::return_slot EXIT : No entry in ia_TSEBranches found for TSE %s.\n",
                         pp_name));
      return NULL;
    }
   else
    {
      TMTrace(4, ("RM_Info_TSE::return_slot EXIT : Slot %d matches TSE %s.\n",
                         lv_midx, pp_name));
      return (RM_Info_TSEBranch *) &ia_TSEBranches[lv_midx];
    }
}


// ----------------------------------------------------------------
// RM_Info_TSE::return_rmid by nid and pid.
// Purpose : Lookup an RM based on the process nid and pid
// This is used to fill in the rmid for ax_reg and doomtxn requests  
// from TSEs which may not fill it in.
// 
// Return rmid. -1 indicates no entry found for nid,pid.
// ----------------------------------------------------------------
int RM_Info_TSE::return_rmid(int32 pv_nid, int32 pv_pid)
{
   int32 lv_error = FEOK;
   int32 lv_rmid;
   char lv_pname[MS_MON_MAX_PROCESS_PATH], *lp_pname = (char *) &lv_pname;
   
   //TMTrace(1, ("RM_Info_TSE::return_rmid: ENTRY for TSE (%d, %d).\n", pv_nid, pv_pid));
   lv_error = msg_mon_get_process_name(pv_nid, pv_pid, lp_pname);
   //TMTrace(1, ("RM_Info_TSE::return_rmid: msg_mon_get_process_name completed for TSE (%d, %d).\n",\ pv_nid, pv_pid));
   if (lv_error)
    {
      TMTrace(1, ("RM_Info_TSE::return_rmid: Error %d retrieving pname "
                  "for TSE (%d, %d).\n", lv_error, pv_nid, pv_pid));
      return -1;
    }
   else
    {
      RM_Info_TSEBranch *lp_rmInfo = return_slot(lp_pname);
      if (lp_rmInfo)
      {
         lv_rmid = lp_rmInfo->rmid();
         // Update our nid and pid just in case it's changed.
         lp_rmInfo->nid(pv_nid);
         lp_rmInfo->pid(pv_pid);
    }
      else
      {
         TMTrace(1, ("RM_Info_TSE::return_rmid EXIT : No entry in ia_TSEBranches found for TSE %s (%d, %d).\n",
                 lp_pname, pv_nid, pv_pid));
         return -1;
}
   }

   TMTrace(2, ("RM_Info_TSE::return_rmid EXIT : TSE %s (%d, %d) found, returning rmid %d.\n",
               lp_pname, pv_nid, pv_pid, lv_rmid));
   return lv_rmid;
} //RM_Info_TSE::return_rmid


// ----------------------------------------------------------------
// RM_Info_TSEBranch::return_slot_by_index
// Purpose : Returns a pointer to ia_TSEBranches[pp_index].
// This is used to return RM info to the management interfaces.
// ----------------------------------------------------------------
RM_Info_TSEBranch *RM_Info_TSE::return_slot_by_index(int32 pv_index)
{
   return (RM_Info_TSEBranch *) &ia_TSEBranches[pv_index];
}


// ----------------------------------------------------------------
// RM_Info_TSEBranch::setBranchState
// Purpose : Sets the branch state by looking up
// the rmid in the branch list.
// ----------------------------------------------------------------
void RM_Info_TSE::setBranchState(int32 pv_rmid, TSEBranch_state pv_state)
{
   RM_Info_TSEBranch *lp_branch = return_slot(pv_rmid);
   if (lp_branch == NULL)
   {
      TMTrace (1, ("RM_Info_TSE::setBranchState, rmid %d not found in branch list.\n",
               pv_rmid));
      tm_log_event(DTM_TSEBRANCH_RMIDNOTFOUND, SQ_LOG_CRIT, "DTM_TSEBRANCH_RMIDNOTFOUND",
                   FENOTFOUND, pv_rmid);
      abort();
   }
   else
      lp_branch->state(pv_state);
   TMTrace (2, ("RM_Info_TSE::setBranchState EXIT : rmid %d, state set to %d.\n", 
            pv_rmid, pv_state));
} //RM_Info_TSE::setBranchState



// --------------------------------------------------------------
// Branch stuff below
// --------------------------------------------------------------

// ---------------------------------------------------------------
// xa_send
// Purpose : Take in a transaction representation and a function
//           pointer, then walk through the tx's RM array and
//           and send the message to each one.
// While the TM is in quiescing state, we go to sleep here.
// This prevents the TM from sending any further messages to TSEs.
// Note that the TM can be forced out of quiescing state
// ---------------------------------------------------------------
int32 RM_Info_TSE::xa_send (xa_entry *pp_fnc_ptr, RM_Info_TSEBranch *pa_rms, int64 pv_flags)
{
    if (gv_tm_info.state() == TM_STATE_QUIESCE)
    {
       CTmTxKey lv_txn = XIDtotransid(&pa_rms->iv_xid);
       TMTrace(1, ("xa_send : TM Quiescing - Transaction thread for (%d,%d) going to sleep!\n",
               lv_txn.node(), lv_txn.seqnum()));
       while (gv_tm_info.state() == TM_STATE_QUIESCE)
       {
          SB_Thread::Sthr::sleep(30000); // 30 seconds
          if (gv_tm_info.state() != TM_STATE_QUIESCE)
          {
             TMTrace(1, ("xa_send : TM Leaving Quiesced state - Transaction (%d,%d).\n",
                     lv_txn.node(), lv_txn.seqnum()));
          }
       } //while
    }

    int32 lv_error = (*pp_fnc_ptr) (&(pa_rms->iv_xid), pa_rms->rmid(),  pv_flags);
    
    return lv_error;
} //RM_Info_TSE::xa_send


// ------------------------------------------------------------
// rollback_branches
// Purpose : send out an xa_rollback to all partic RMs.
// This now calls rollback_branches_single_pass() in a loop until all 
// participants are resolved (have both the partic and resolved flags
// set on the branch.
// ------------------------------------------------------------
int32 RM_Info_TSE::rollback_branches (CTmTxBase *pp_txn, 
                                      int64 pv_flags,
                                      CTmTxMessage * pp_msg,
                                      bool pv_error_condition)
{
   int32 lv_err = FEOK;
   int32 lv_iterations = 1;

   TMTrace (2, ("rollback_branches, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
                pp_txn->node(), pp_txn->seqnum(), pv_flags));

   lv_err = rollback_branches_single_pass(pp_txn, pv_flags, pp_msg, pv_error_condition);
   TMTrace (3, ("rollback_branches, Txn ID (%d,%d), pass %d, RMs failed=%d, Partic=%d, UnResolved=%d.\n",
                pp_txn->node(), pp_txn->seqnum(), lv_iterations,
                num_rm_failed(pp_txn), num_rm_partic(pp_txn), num_rms_unresolved(pp_txn)));

   while (!gv_tm_info.broadcast_rollbacks() && lv_err == FEOK && num_rms_unresolved(pp_txn))
   {
      lv_iterations++;
      // Once we hit the threshold, report a warning - it shouldn't take so many attempts
      if (lv_iterations > TM_MULTIPASS_REPORT_THRESHOLD)
         tm_log_event(DTM_TMTX_ROLLBACK_BRANCHES_MULTIPASS, SQ_LOG_WARNING, "DTM_TMTX_ROLLBACK_BRANCHES_MULTIPASS",
                      -1,-1,pp_txn->node(),pp_txn->seqnum(),-1,-1,-1,-1,-1,-1,-1,-1,pp_txn->tx_state(),lv_iterations,num_rms_unresolved(pp_txn));
      TMTrace (3, ("rollback_branches, Txn ID (%d,%d) MULTIPASS, pass %d, RMs failed=%d, Partic=%d, UnResolved=%d.\n",
                   pp_txn->node(), pp_txn->seqnum(), lv_iterations,
                   num_rm_failed(pp_txn), num_rm_partic(pp_txn), num_rms_unresolved(pp_txn)));
      if (lv_iterations > TM_MULTIPASS_LOOP_THRESHOLD)
         lv_err = FETOOMANY;
      else
         lv_err = rollback_branches_single_pass(pp_txn, pv_flags, pp_msg, pv_error_condition);
   }

   TMTrace (2, ("rollback_branches, Txn ID (%d,%d), EXIT, total passes %d, UnResolved %d.\n",
                pp_txn->node(), pp_txn->seqnum(), lv_iterations, num_rms_unresolved(pp_txn)));
   return lv_err;
} //rollback_branches


// ------------------------------------------------------------
// rollback_branches_single_pass 
// Purpose : send out an xa_rollback to all partic RMs.
// An XAER_RMFAIL error will cause the RM to go into an TSEBranch_FAILED
// state.  The TM periodically attempts to re-open failed RMs.
// pv_flags contains the XA flag values.  For TSE RMs this includes
// transaction type bits.
// ------------------------------------------------------------
int32 RM_Info_TSE::rollback_branches_single_pass (CTmTxBase *pp_txn,
                                                  int64 pv_flags,
                                                  CTmTxMessage * pp_msg,
                                                  bool pv_error_condition)
{

//printf("\n pv_error_condition : %d", pv_error_condition);
   TM_RM_Responses    la_resp[MAX_OPEN_RMS];
   RM_Info_TSEBranch *la_rms = NULL;
   int32              lv_count = 0;
   int32              lv_BRerror = XA_OK;
   int32              lv_TRerror = XA_OK;
   RM_Info_TSEBranch *lp_branch; // Used to match rmid from responses to branches.

   TMTrace (2, ("rollback_branches_single_pass, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
            pp_txn->node(), pp_txn->seqnum(), pv_flags));

   pp_txn->stats()->xa_rollback()->start();
   // Locking the txn object for the entire routine.
   // Nobody else should be touching it anyway!
   branch_lock();
   la_rms = return_rms();

   pp_txn->stats()->RMSend()->start();
   for (int32 lv_idx = 0; lv_idx <= return_highest_index_used(); lv_idx++)
   {
      // If DTM_BROADCAST_ROLLBACKS is set then force all up RMs to participate.
      if ((la_rms[lv_idx].partic() && la_rms[lv_idx].in_use()) ||
          (gv_tm_info.broadcast_rollbacks() && la_rms[lv_idx].in_use()))
      {
         lv_BRerror = xa_send(&((*tm_switch).xa_rollback_entry), &la_rms[lv_idx], pv_flags);

         if (lv_BRerror == XA_OK || lv_BRerror == XAER_NOTA)
         {
            // Successful send to TSE RM
            lv_count++;
            la_rms[lv_idx].resolved(true);
         }
         else
         {
            TMTrace (1, ("rollback_branches_single_pass ERROR %s returned by XATM Library "
                            "for rmid=%d. Presumed aborted.\n",
                            XAtoa(lv_BRerror), la_rms[lv_idx].rmid()));

            switch (lv_BRerror)
            {
            case XAER_INVAL:
               tm_log_event(DTM_TMTX_PROG_ERROR, SQ_LOG_CRIT, "DTM_TMTX_PROG_ERROR");
               // Programming error!
               abort ();
               break;
            case XAER_RMFAIL:
               // RM hard down
               fail_rm(la_rms[lv_idx].rmid());
               break;
            default:
               // Return the error to the transaction state machine
               // It will decide what to do.
               break;
            } //switch
            lv_TRerror = lv_BRerror;
         } //else error
      }
   } //for each rm
            

   // if this is a normal ABORTTRANSACTION, then we want to reply
   // back to the APP now.
   // Always reply FEOK to ABORTTRANSACTION unless told otherwise,
   // we should have already replied if there was an error in earlier 
   // processing.  Now that we're in phase 2, the rollback should succeed.
   /*
   if (pp_msg->replyPending())
   {
      if (!pv_error_condition)
         pp_msg->reply(FEOK);
      else
         pp_msg->reply();
   }
   */
   if (!pv_error_condition)  
       pp_msg->responseError(FEOK); 
 
   int32 lv_repliesOutstanding = complete_all(lv_count, la_resp, pp_txn->rm_wait_time(), pp_txn->legacyTransid());
   pp_txn->stats()->RMSend_stop();

   if (lv_repliesOutstanding > 0)
      lv_TRerror = XAER_RMFAIL;
   else
      // Check the responses for errors
      for (int32 lv_idx = 0; lv_idx < lv_count; lv_idx++)
      {
         lv_BRerror = la_resp[lv_idx].iv_error;

         lp_branch = return_slot(la_resp[lv_idx].iv_rmid);
           
         // We had better find a matching RM or there's something very wrong!
         if (lp_branch == NULL)
         {
            tm_log_event(DTM_TMTX_GETRM_FAIL, SQ_LOG_CRIT, "DTM_TMTX_GETRM_FAIL",
                         lv_BRerror,la_resp[lv_idx].iv_rmid);
            TMTrace (1, ("RM_Info_TSE::rollback_branches_single_pass: Get "
                     "resource manager rmid %d failed with error %d\n",
                     la_resp[lv_idx].iv_rmid, lv_BRerror));
            abort();
         }

         // Error specific processing:
         // Only write a trace record for actual errors
         if (pp_txn->trace_level() && lv_BRerror < XA_OK)
            trace_printf("rollback_branches_single_pass ERROR %s returned by RM, rmid=%d.\n",
                        XAtoa(lv_BRerror), 
                        lp_branch->rmid());

         switch (lv_BRerror)
         {
         case XA_OK:
            la_rms[lv_idx].resolved(true);
            if (gv_tm_info.broadcast_rollbacks() && !lp_branch->partic())
            {
               tm_log_event(DTM_PARTIC_MISSING, SQ_LOG_CRIT, "DTM_PARTIC_MISSING",
                  -1,lp_branch->rmid(),pp_txn->node(),pp_txn->seqnum(),
                  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,lp_branch->pname());
               TMTrace(1, ("RM_Info_TSE::rollback_branches_single_pass: Txn ID (%d,%d) participant rmid %d (%s) found "
                  "that was not in the transaction branch list!\n", pp_txn->node(), pp_txn->seqnum(), 
                  lp_branch->rmid(), lp_branch->pname()));
               if (gv_tm_info.broadcast_rollbacks() == TM_BROADCAST_ROLLBACKS_DEBUG)
                  abort();
            }
            la_rms[lv_idx].resolved(true);
            break;
         case XAER_NOTA:
            remove_rm_by_rmid(lp_branch->rmid());
            // Intentional drop through
         case XA_RBROLLBACK:
         case XA_RBCOMMFAIL:
         case XA_RBDEADLOCK:
         case XA_RBOTHER:
         case XA_RBPROTO:
         case XA_RBTIMEOUT:
         case XA_RBTRANSIENT:
            la_rms[lv_idx].resolved(true);
            break;
         case XAER_RMFAIL:
            {
            // RM hard down
            fail_rm(lp_branch->rmid());
            // We don't want to override a heuristic here or we won't drive
            // forget.
            if (!lv_TRerror)
               lv_TRerror = lv_BRerror;
            break;
            }
         case XA_HEURHAZ:
         case XA_HEURCOM:
         case XA_HEURRB:
         case XA_HEURMIX:
            {
            // Heuristic completion
            la_rms[lv_idx].resolved(true);
            tm_log_event(DTM_ROLLBACK_HEURISTIC, SQ_LOG_WARNING, "DTM_ROLLBACK_HEURISTIC",
                           lp_branch->rmid(),pp_txn->node(),pp_txn->seqnum(),-1,-1,-1,-1,-1,-1,-1,-1,-1-1,
                           pp_txn->tx_state(),-1,-1,-1,XAtoa(lv_BRerror));
            TMTrace (1, ("rollback_branches_single_pass received heuristic completion %s, Rollback completed heuristically for "
                           "Txn ID (%d,%d), rmid %d\n",
                           XAtoa(lv_BRerror), 
                           pp_txn->node(),pp_txn->seqnum(),
                           lp_branch->rmid()));

            lv_TRerror = lv_BRerror;
            break;
            }
         default:
            // For all other errors
            la_rms[lv_idx].resolved(true);
            lv_TRerror = lv_BRerror;
            break;
         } //switch

         // If there wasn't an error then decrement the totalBranchesLeftToRecover
         // for this RM.  Once this reaches 0 the RM will be declared TSEBranch_UP and 
         // allow new transactions to begin.
         if (lv_BRerror >= 0)
         {
            RM_Info_TSEBranch * lp_glob_rm = gv_RMs.TSE()->return_slot(la_resp[lv_idx].iv_rmid);
            // We had better find a matching RM or there's something very very wrong!
            if (lp_glob_rm == NULL)
            {
               tm_log_event(DTM_TMTX_GETRM_FAIL2, SQ_LOG_CRIT, "DTM_TMTX_GETRM_FAIL2",
                              lv_BRerror,la_resp[lv_idx].iv_rmid);
               TMTrace (1, ("RM_Info_TSE::rollback_branches_single_pass: Get resource manager %d failed 2.\n",
                        la_resp[lv_idx].iv_rmid));
               abort();
            }
            lp_glob_rm->dec_totalBranchesLeftToRecover();
         }
      } //for all completions
   branch_unlock();
   pp_txn->stats()->xa_rollback()->stop();

   TMTrace (2, ("rollback_branches_single_pass, Txn ID (%d,%d), EXIT returning %s, partic=%d, unresolved=%d.\n",
                   pp_txn->node(), pp_txn->seqnum(), 
                   XAtoa(lv_TRerror), 
                   num_rm_partic(pp_txn),num_rms_unresolved(pp_txn)));

   return lv_TRerror;
} //rollback_branches_single_pass


// ------------------------------------------------------------
// commit_branches
// Purpose : send out an xa_commit to all partic RMs.
// the iv_resolved flag is set here, but we don't make use of it
// during commit.
// An XAER_RMFAIL error will cause the RM to go into an TSEBranch_FAILED
// state.  The TM periodically attempts to re-open failed RMs.
// pv_flags contains the XA flag values.  For TSE RMs this includes
// transaction type bits.
// ------------------------------------------------------------
int32 RM_Info_TSE::commit_branches (CTmTxBase *pp_txn, int64 pv_flags, CTmTxMessage * pp_msg)
{
   TM_RM_Responses la_resp[MAX_OPEN_RMS];
   RM_Info_TSEBranch *la_rms = NULL;
   int32           lv_count = 0;
   int32           lv_BRerror = XA_OK;
   int32           lv_TRerror = XA_OK;

   TMTrace (2, ("commit_branches ENTRY, Txn ID (%d,%d), msgid %d, flags " PFLL "\n",
            pp_txn->node(), pp_txn->seqnum(), pp_msg->msgid(), pv_flags));

   pp_txn->stats()->xa_commit()->start();
   // Locking the txn object for the entire routine.
   // Nobody else should be touching it anyway!
   branch_lock();
   la_rms = return_rms();
   pp_txn->stats()->RMSend()->start();
   for (int32 lv_idx = 0; lv_idx <= return_highest_index_used(); lv_idx++)
   {
      if (la_rms[lv_idx].partic() && la_rms[lv_idx].in_use())
      {
         lv_BRerror = xa_send(&((*tm_switch).xa_commit_entry), &la_rms[lv_idx], pv_flags);

         if (lv_BRerror == XA_OK)
            lv_count++;
         else
         {
            TMTrace (1, ("commit_branches ERROR %s returned by XATM Library "
                            "for rmid=%d.\n",
                            XAtoa(lv_BRerror), la_rms[lv_idx].rmid()));

            switch (lv_BRerror)
            {
            case XAER_INVAL:
                tm_log_event(DTM_TMTX_PROG_ERROR, SQ_LOG_CRIT, "DTM_TMTX_PROG_ERROR");
               // Programming error!
               abort ();
               break;
            case XAER_RMFAIL:
               // RM hard down
               fail_rm(la_rms[lv_idx].rmid());
               break;
            default:
               // Return the error to the transaction state machine
               // It will decide what to do.
               break;
            } //switch
            lv_TRerror = lv_BRerror;
         } //else error
      }
   } //for each rm

   // Set response code
   switch (lv_TRerror)
   {
      case XA_OK:
         pp_msg->responseError(FEOK);
         break;
      case XAER_RMFAIL:
      default:
         pp_msg->responseError(FEDEVICEDOWNFORTMF);
         break;
   }

   // We must call complete_all to process any
   // outstanding rm completions even if we had errors
   // on the send.
   int32 lv_repliesOutstanding = complete_all(lv_count, la_resp, pp_txn->rm_wait_time(), pp_txn->legacyTransid());
   pp_txn->stats()->RMSend_stop();

   if (lv_repliesOutstanding > 0)
      lv_TRerror = XAER_RMFAIL;
   else
      // Check the responses for errors
      for (int32 lv_idx = 0; lv_idx < lv_count; lv_idx++)
      {
         lv_BRerror = la_resp[lv_idx].iv_error;
         RM_Info_TSEBranch * lp_rm = return_slot(la_resp[lv_idx].iv_rmid);
         if (lp_rm == NULL)
         {
            tm_log_event(DTM_TMTX_GETRM_FAIL, SQ_LOG_CRIT, "DTM_TMTX_GETRM_FAIL");
            // We had better find a matching RM or there's something very wrong!
            TMTrace (1, ("RM_Info_TSE::commit_branches - Get resource manager failed\n"));
            abort();

         }
         // Error specific processing:
         // Only write a trace record for actual errors
         if (pp_txn->trace_level() && lv_BRerror < XA_OK)
            trace_printf("commit_branches ERROR %s returned by RM, rmid=%d.\n",
                         XAtoa(lv_BRerror), 
                         lp_rm->rmid());

         switch (lv_BRerror)
         {
         case XA_OK:
            {
            TMTrace (4, ("RM_Info_TSE::commit_branches - commit_branches received "
                         "XA_OK response to xa_commit. Txn ID (%d,%d), rmid %d.\n",
                         pp_txn->node(), pp_txn->seqnum(), lp_rm->rmid()));
            break;
            }
         case XAER_NOTA:
            {
            tm_log_event(DTM_TMTX_TX_FORGET_BAD_RESP, SQ_LOG_WARNING, "DTM_TMTX_TX_FORGET_BAD_RESP",-1,
                         lp_rm->rmid(),pp_txn->node(),pp_txn->seqnum());
            TMTrace (1, ("RM_Info_TSE::commit_branches - commit_branches received "
                         " XAER_NOTA response to xa_commit. "
                         "Assuming RM has already processed commit and forgotten. "
                         "Txn ID (%d,%d), rmid %d.\n",
                         pp_txn->node(), pp_txn->seqnum(), lp_rm->rmid()));
            lv_TRerror = XA_OK;
            break;
            }

         case XA_RBROLLBACK:
         case XA_RBCOMMFAIL:
         case XA_RBDEADLOCK:
         case XA_RBOTHER:
         case XA_RBPROTO:
         case XA_RBTIMEOUT:
         case XA_RBTRANSIENT:
            // Transaction was unexpectedly aborted by RM
            {
            tm_log_event(DTM_TMTX_TX_ROLLBACKONCOMMIT, SQ_LOG_ERR, "DTM_TMTX_TX_ROLLBACKONCOMMIT",
                           -1,lp_rm->rmid(),pp_txn->node(),pp_txn->seqnum(),-1,lv_BRerror,-1,-1,-1,
                           -1,-1,-1,-1,-1,-1,-1,XAtoa(lv_BRerror));
            TMTrace (1, ("commit_branches received rollback notification %s, completing "
                           "heuristically, Txn ID (%d,%d), rmid %d\n",
                           XAtoa(lv_BRerror), 
                           pp_txn->node(), pp_txn->seqnum(),
                           lp_rm->rmid()));
            lv_TRerror = XA_HEURRB;
            // Intentionally drop through to heuristic processing
            }
         case XA_HEURHAZ:
         case XA_HEURCOM:
         case XA_HEURRB:
         case XA_HEURMIX:
            {
            // Heuristic completion: Log it.
            // EMS DTM_COMMIT_HEURISTIC
            tm_log_event(DTM_COMMIT_HEURISTIC, SQ_LOG_WARNING, "DTM_COMMIT_HEURISTIC",
                           lp_rm->rmid(),pp_txn->node(),pp_txn->seqnum(),-1,-1,-1,-1,-1,-1,-1,-1,-1-1,
                           pp_txn->tx_state(),-1,-1,-1,XAtoa(lv_BRerror));
            TMTrace (1, ("commit_branches received heuristic completion %s, "
                           "Txn ID (%d,%d), rmid %d\n",
                           XAtoa(lv_BRerror), 
                           pp_txn->node(),pp_txn->seqnum(),
                           lp_rm->rmid()));

            lp_rm->resolved(true);
            lv_TRerror = lv_BRerror;
            break;
            }
         case XAER_RMFAIL:
            // RM hard down
            fail_rm(lp_rm->rmid());
            lv_TRerror = lv_BRerror;
            break;
         default:
            // For all other errors
            lp_rm->resolved(true);
            lv_TRerror = lv_BRerror;

            // EMS DTM_COMMIT_INCONSISTENT
            tm_log_event(DTM_COMMIT_INCONSISTENT, SQ_LOG_WARNING, "DTM_COMMIT_INCONSISTENT",-1,
                           lp_rm->rmid(),pp_txn->node(),pp_txn->seqnum());
            break;
         } //switch

         // If there wasn't an error then decrement the totalBranchesLeftToRecover
         // for this RM.  Once this reaches 0 the RM will be declared TSEBranch_UP and 
         // allow new transactions to begin.
         if (lv_BRerror >= 0)
         {
            RM_Info_TSEBranch * lp_glob_rm = gv_RMs.TSE()->return_slot(la_resp[lv_idx].iv_rmid);
            lp_glob_rm->dec_totalBranchesLeftToRecover();
         }
      } //for all completions
   branch_unlock();
   pp_txn->stats()->xa_commit()->stop();

   TMTrace (2, ("commit_branches, Txn ID (%d,%d), EXIT returning %s, partic=%d, unresolved=%d.\n",
                   pp_txn->node(),pp_txn->seqnum(), 
                   XAtoa(lv_TRerror), 
                   num_rm_partic(pp_txn), num_rms_unresolved(pp_txn)));

   return lv_TRerror;
} // commit_branches


// ------------------------------------------------------------
// end_branches
// Purpose - send out xa_end to RMs to end this transactional
//           association.  Happens during END or ABORT.
// An XAER_RMFAIL error will cause the RM to go into an TSEBranch_FAILED
// state.  The TM periodically attempts to re-open failed RMs.
// pv_flags contains the XA flag values.  For TSE RMs this includes
// transaction type bits.
// ------------------------------------------------------------
int32 RM_Info_TSE::end_branches (CTmTxBase *pp_txn, int64 pv_flags)
{
   RM_Info_TSEBranch *la_rms = NULL;
   TM_RM_Responses la_resp[MAX_OPEN_RMS];
   int32           lv_count = 0;
   int32           lv_BRerror = XA_OK;
   int32           lv_TRerror = XA_OK;

   TMTrace (2, ("end_branches, Txn ID (%d,%d), ENTRY flags " PFLL "\n",
            pp_txn->node(), pp_txn->seqnum(), pv_flags));

   pp_txn->stats()->xa_end()->start();
   // Locking the txn object for the entire routine.
   // Nobody else should be touching it anyway!
   branch_lock();
   la_rms = return_rms();
   pp_txn->stats()->RMSend()->start();
   for (int32 lv_idx = 0; lv_idx <= return_highest_index_used(); lv_idx++)
   {
      // Check RM type: For TSE RMs we never send xa_end
      if (la_rms[lv_idx].partic() && la_rms[lv_idx].in_use() && 
          !tm_xa_rmType_TSE(la_rms[lv_idx].rmid()))
      {
         lv_BRerror = xa_send(&((*tm_switch).xa_end_entry), &la_rms[lv_idx], pv_flags);
         if (lv_BRerror == XA_OK)
            lv_count++;
         else
         {
            TMTrace (1, ("end_branches ERROR %s returned by XATM Library "
                            "for rmid=%d, transaction will abort.\n",
                            XAtoa(lv_BRerror), la_rms[lv_idx].rmid()));

            switch (lv_BRerror)
            {
            case XAER_INVAL:
                tm_log_event(DTM_TMTX_PROG_ERROR, SQ_LOG_CRIT, "DTM_TMTX_PROG_ERROR");
               // Programming error!
               abort ();
               break;
            case XAER_RMFAIL:
               // RM hard down
               fail_rm(la_rms[lv_idx].rmid());
               la_rms[lv_idx].partic(false);
               break;
            default:
               // For all other errors, drive abort in transaction state machine
               break;
            } //switch
            lv_TRerror = lv_BRerror;
         } //if error
      }
   } //for each rm

   // We must call complete_all to process any
   // outstanding rm completions even if we had errors
   // on the send.
   int32 lv_repliesOutstanding = complete_all(lv_count, la_resp, 
                                              pp_txn->rm_wait_time(), pp_txn->legacyTransid());
   pp_txn->stats()->RMSend_stop();

   if (lv_repliesOutstanding > 0)
      lv_TRerror = XAER_RMFAIL;
   else
      // go through errors
      for (int32 lv_idx = 0; lv_idx < lv_count; lv_idx++)
      {
         lv_BRerror = la_resp[lv_idx].iv_error;
         if (lv_BRerror != XA_OK)
         {
            RM_Info_TSEBranch * lp_rm = return_slot(la_resp[lv_idx].iv_rmid);
            // We had better find a matching RM or there's something very wrong!
            if (lp_rm == NULL)
            {
               tm_log_event(DTM_TMTX_GETRM_FAIL, SQ_LOG_CRIT, "DTM_TMTX_GETRM_FAIL");
               TMTrace (1, ("end_branches ERROR %s returned by RM, rmid=%d, TM aborting, RM NOT FOUND!\n",
                              XAtoa(lv_BRerror), 
                              la_resp[lv_idx].iv_rmid));
               abort();
            }
            else
               TMTrace (1, ("end_branches ERROR %s returned by RM, rmid=%d, transaction aborted.\n",
                           XAtoa(lv_BRerror), 
                           lp_rm->rmid()));

            // Error specific processing:
            // Ignore rollbacks, they're handled by the transaction state 
            // machine (ie the caller).
            if (lv_BRerror <= XA_RBBASE)
            {
               switch (lv_BRerror)
               {
               case XAER_RMFAIL:
                  // RM hard down
                  fail_rm(lp_rm->rmid());
                  // ia_TSEBranches.remove_rm_by_rmid(lp_rm->rmid());
                  break;
               default:
                  // For all other errors, drive an abort
                  // Don't turn the participate flag off because we want to call
                  // xa_rollback() for all branches, even this one.
                  break;
               } //switch
            }
            lv_TRerror = lv_BRerror;
         } //if error
      } //for each completion
   branch_unlock();
   pp_txn->stats()->xa_end()->stop();

   TMTrace (2, ("end_branches, Txn ID (%d,%d), EXIT, returning %s\n",
            pp_txn->node(), pp_txn->seqnum(), XAtoa(lv_TRerror)));

   return lv_TRerror;
}

// ---------------------------------------------------------------
// forget_heur_branches
// Purpose : helper method for heuristic forget
// If the state has already reached forgotten_heuristic, we don't
// send out xa_forget to the RMs.
// The abort_flags become important when we're trying to recover
// after a crash, where we may need to know the outcome of a 
// forgotten transaction if it appears in the xa_recover list
// from a subordinate RM.
// --------------------------------------------------------------
int32 RM_Info_TSE::forget_heur_branches (CTmTxBase *pp_txn, int64 pv_flags)
{
    int32 lv_error = FEOK;

    TMTrace (2, ("RM_Info_TSE::forget_heur_branches ENTRY : Txn ID (%d,%d), flags " PFLL "\n",
             pp_txn->node(), pp_txn->seqnum(), pv_flags));
    tm_log_event(DTM_TMTX_FORGET_HEURISTIC, SQ_LOG_WARNING, "DTM_TMTX_FORGET_HEURISTIC",
                 -1,-1,pp_txn->node(),pp_txn->seqnum());
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
            TMTrace (1, ("RM_Info_TSE::forget_heur_branches - Invalid branch state\n"));
            abort (); 
         break;
       }
    }
    return lv_error;
}

// ---------------------------------------------------------------
// forget_branches
// Purpose : all RMs have responded, so forget this tx.
// An XAER_RMFAIL error will cause the RM to go into an TSEBranch_FAILED
// state.  The TM periodically attempts to re-open failed RMs.
// pv_flags contains the XA flag values.  For TSE RMs this includes
// transaction type bits.
// --------------------------------------------------------------
int32 RM_Info_TSE::forget_branches (CTmTxBase *pp_txn, int64 pv_flags)
{
   RM_Info_TSEBranch *la_rms = NULL;
   TM_RM_Responses la_resp[MAX_OPEN_RMS];
   int32           lv_count = 0;
   int32           lv_BRerror = 0;
   int32           lv_TRerror = 0;

   TMTrace (2, ("forget_branches, Txn ID (%d,%d), ENTRY flags " PFLL "\n",
            pp_txn->node(), pp_txn->seqnum(), pv_flags));

   // Locking the txn object for the entire routine.
   // Nobody else should be touching it anyway!
   branch_lock();
   la_rms = return_rms();
   for (int32 lv_idx = 0; lv_idx <= return_highest_index_used(); lv_idx++)
   {
      if (la_rms[lv_idx].partic() && la_rms[lv_idx].in_use())
      {
         lv_BRerror = xa_send(&((*tm_switch).xa_forget_entry),  &la_rms[lv_idx], pv_flags);

         if (lv_BRerror == XA_OK)
            lv_count++;
         else
         {
            TMTrace (2, ("forget_branch ERROR %s returned by XATM Library "
                            "for rmid=%d.\n",
                            XAtoa(lv_BRerror), la_rms[lv_idx].rmid()));

            switch (lv_BRerror)
            {
            case XAER_INVAL:
                 tm_log_event(DTM_TMTX_PROG_ERROR, SQ_LOG_CRIT, "DTM_TMTX_PROG_ERROR");
               // Programming error!
               abort (); 
               break;
            case XAER_RMFAIL:
               // RM hard down
               fail_rm(la_rms[lv_idx].rmid());
               //TODO: recovery
               break;
            default:
               // Ignore other errors
               break;
            } //switch
            lv_TRerror = lv_BRerror;
         } //else error
      }
   } //for each rm


   int32 lv_repliesOutstanding = complete_all(lv_count, la_resp, pp_txn->rm_wait_time(), pp_txn->legacyTransid());

   if (lv_repliesOutstanding > 0)
      lv_TRerror = XAER_RMFAIL;
   else
      // we need to check out the prepare responses
      for (int32 lv_idx = 0; lv_idx < lv_count; lv_idx++)
      {
         lv_BRerror = la_resp[lv_idx].iv_error;
         if (lv_BRerror != XA_OK)
         {
            RM_Info_TSEBranch * lp_rm = return_slot(la_resp[lv_idx].iv_rmid);

            if (lp_rm == NULL)
            {
               tm_log_event(DTM_TMTX_GETRM_FAIL, SQ_LOG_CRIT, "DTM_TMTX_GETRM_FAIL");
               // We had better find a matching RM or there's something very wrong!
               TMTrace (1, ("RM_Info_TSE::forget_branches - Get resource manager failed\n"));
               abort();
            }

            // Error specific processing:
            // Only write a trace record for actual errors
            if (pp_txn->trace_level() && lv_BRerror < XA_OK)
               trace_printf("forget_branches ERROR %s returned by RM, rmid=%d, transaction aborted.\n",
                           XAtoa(lv_BRerror), 
                           lp_rm->rmid());

            switch (lv_BRerror)
            {
            case XAER_NOTA:
               lv_TRerror = lv_BRerror;
               break;
            case XAER_RMFAIL:
               // RM hard down
               fail_rm(lp_rm->rmid());
               //TODO: recovery
               lv_TRerror = lv_BRerror;
               break;
            default:
               // For all other errors, drive an abort
               lv_TRerror = lv_BRerror;
               break;
            } //switch
         } //if error
      } //for all completions
   branch_unlock();

   TMTrace (2, ("forget_branches, Txn ID (%d,%d), EXIT\n",
            pp_txn->node(), pp_txn->seqnum()));

   return lv_TRerror;
}


// ------------------------------------------------------------
// prepare_branches
// Purpose : send out an xa_prepare to all partic RMs.
// This now calls prepare_branches_single_pass() in a loop until all 
// participants are prepared (have both the partic and resolved flags
// set on the branch.
// ------------------------------------------------------------
int32 RM_Info_TSE::prepare_branches (CTmTxBase *pp_txn, int64 pv_flags, CTmTxMessage * pp_msg)
{
   int32 lv_err = FEOK;
   int32 lv_iterations = 1;

   TMTrace (2, ("prepare_branches, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
            pp_txn->node(), pp_txn->seqnum(), pv_flags));

   lv_err = prepare_branches_single_pass(pp_txn, pv_flags);
   TMTrace (3, ("prepare_branches, Txn ID (%d,%d), pass %d, RMs failed=%d, Partic=%d, UnResolved=%d.\n",
                pp_txn->node(), pp_txn->seqnum(), lv_iterations,
                num_rm_failed(pp_txn), num_rm_partic(pp_txn), num_rms_unresolved(pp_txn)));

   while (lv_err == FEOK && num_rms_unresolved(pp_txn))
   {
      lv_iterations++;
      // Once we hit the threshold, report a warning - it shouldn't take so many attempts
      if (lv_iterations > TM_MULTIPASS_REPORT_THRESHOLD)
         tm_log_event(DTM_TMTX_PREPARE_BRANCHES_MULTIPASS, SQ_LOG_WARNING, "DTM_TMTX_PREPARE_BRANCHES_MULTIPASS",
                      -1,-1,pp_txn->node(),pp_txn->seqnum(),-1,-1,-1,-1,-1,-1,-1,-1,pp_txn->tx_state(),lv_iterations,
                      num_rms_unresolved(pp_txn));
      TMTrace (3, ("prepare_branches, Txn ID (%d,%d) MULTIPASS, pass %d, RMs failed=%d, Partic=%d, UnResolved=%d.\n",
                   pp_txn->node(), pp_txn->seqnum(), lv_iterations,
                   num_rm_failed(pp_txn), num_rm_partic(pp_txn), num_rms_unresolved(pp_txn)));
      if (lv_iterations > TM_MULTIPASS_LOOP_THRESHOLD)
         lv_err = FETOOMANY;
      else
         lv_err = prepare_branches_single_pass(pp_txn, pv_flags);
   }

   // Reset the resolved bits to pick up any ax_reg requests which arrive during commit.
   reset_resolved(pp_txn);

   TMTrace (2, ("prepare_branches, Txn ID (%d,%d), EXIT, total passes %d, UnResolved %d.\n",
                pp_txn->node(), pp_txn->seqnum(), lv_iterations, num_rms_unresolved(pp_txn)));
   return lv_err;
} //prepare_branches


// ---------------------------------------------------------------
// prepare_branches_single_pass
// Purpose - send out an xa_prepare.  Happens during END or ABORT
// An XAER_RMFAIL error will cause the RM to go into an TSEBranch_FAILED
// state.  The TM periodically attempts to re-open failed RMs.
// pv_flags contains the XA flag values.  For TSE RMs this includes
// transaction type bits.
// ---------------------------------------------------------------
int32 RM_Info_TSE::prepare_branches_single_pass (CTmTxBase *pp_txn, int64 pv_flags)
{
   RM_Info_TSEBranch     *la_rms = NULL;
   TM_RM_Responses la_resp[MAX_OPEN_RMS];
   int32           lv_count = 0;
   int32           lv_BRerror = XA_OK;
   int32           lv_TRerror = XA_OK;

    TMTrace (2, ("prepare_branches_single_pass, Txn ID (%d,%d), ENTRY flags " PFLL "\n",
                   pp_txn->node(), pp_txn->seqnum(), pv_flags));

   pp_txn->stats()->xa_prepare()->start();
   // Locking the txn object for the entire routine.
   // Nobody else should be touching it anyway!
   branch_lock();
   la_rms = return_rms();
   pp_txn->stats()->RMSend()->start();
   for (int32 lv_idx = 0; lv_idx <= return_highest_index_used(); lv_idx++)
   {
      if (la_rms[lv_idx].partic() && la_rms[lv_idx].in_use())
      {
         lv_BRerror = xa_send(&((*tm_switch).xa_prepare_entry), &la_rms[lv_idx], pv_flags);

         if (lv_BRerror == XA_OK)
            lv_count++;
         else
         {
            TMTrace (1, ("prepare_branch ERROR %s returned by XATM Library "
                            "for rmid=%d, transaction will abort.\n",
                            XAtoa(lv_BRerror), la_rms[lv_idx].rmid()));

            switch (lv_BRerror)
            {
            case XAER_INVAL:
                tm_log_event(DTM_TMTX_PROG_ERROR, SQ_LOG_CRIT, "DTM_TMTX_PROG_ERROR");
               // Programming error!
               abort();
               break;
            case XAER_RMFAIL:
               // RM hard down
               fail_rm(la_rms[lv_idx].rmid());
               // ia_TSEBranches.remove_rm_by_index (lv_idx); Still need to tell TSE to rollback
               break;
            default:
               // For all other errors, drive abort
               break;
            } //switch
            lv_TRerror = lv_BRerror;
         } //else error
      }
   } //for each rm

   // We must call complete_all to process any
   // outstanding rm completions even if we had errors
   // on the send.
   int32 lv_repliesOutstanding = complete_all(lv_count, la_resp, pp_txn->rm_wait_time(), pp_txn->legacyTransid());
   pp_txn->stats()->RMSend_stop();

   if (lv_repliesOutstanding > 0)
      lv_TRerror = XAER_RMFAIL;
   else
      // we need to check out the prepare responses
      // Assume read-only unless we get a response to indicate work was done by a TSE
      // on behalf of this transaction.
      pp_txn->read_only(true);

      for (int32 lv_idx = 0; lv_idx < lv_count; lv_idx++)
         {
         lv_BRerror = la_resp[lv_idx].iv_error;
         RM_Info_TSEBranch * lp_rm = return_slot(la_resp[lv_idx].iv_rmid);

         if (lp_rm == NULL)
         {
            tm_log_event(DTM_TMTX_GETRM_FAIL, SQ_LOG_CRIT, "DTM_TMTX_GETRM_FAIL");
            // We had better find a matching RM or there's something very wrong!
            TMTrace (2, ("RM_Info_TSE::prepare_branches_single_pass - Get resource manager failed\n"));
            abort();
         }

         // Error specific processing:
         // Only write a trace record for actual errors
         if (pp_txn->trace_level() && lv_BRerror != XA_OK)
            trace_printf("prepare_branch ERROR %s returned by RM, rmid=%d.\n",
                        XAtoa(lv_BRerror), 
                        lp_rm->rmid());

         // Check for a rollback
         if (lv_BRerror >= XA_RBBASE && lv_BRerror <= XA_RBEND)
         {
            lp_rm->resolved(true);
            lv_TRerror = lv_BRerror;
         }
         else
            switch (lv_BRerror)
            {
            case XA_OK:
                // At least one TSE did work for this transaction
                // Check that the transaction was not marked read-only
                // We also mark the transaction for rollback here to ensure the updates don't
                // occur.
                lp_rm->resolved(true);
                if ((pp_txn->TT_flags() & TM_TT_READ_ONLY) == TM_TT_READ_ONLY)
                {
                    tm_log_event(DTM_TT_READONLY_BROKEN, SQ_LOG_WARNING, "DTM_TT_READONLY_BROKEN",
                                 -1,lp_rm->rmid(),pp_txn->node(),pp_txn->seqnum(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                                 -1,-1,lp_rm->pname());
                    TMTrace(2, ("RM_Info_TSE::prepare_branches_single_pass - LOGIC ERROR Read-only transaction type "
                            "was specified on Txn ID (%d,%d) by TSE RM %d which updated data!\n",
                            pp_txn->node(), pp_txn->seqnum(), lp_rm->rmid()));
                    pp_txn->mark_for_rollback(true);
                }
                pp_txn->read_only(false);
                pp_txn->stats()->inc_RMParticCount();
                break;
            case XA_RDONLY:
            case XAER_NOTA:
                lp_rm->resolved(true);
                pp_txn->stats()->inc_RMNonParticCount();
                remove_rm_by_rmid(lp_rm->rmid());
                break;
            case XAER_RMFAIL:
                // RM hard down
                fail_rm(lp_rm->rmid());
                // ia_TSEBranches.remove_rm_by_rmid(lp_rm->rmid());
                lv_TRerror = lv_BRerror;
                break;
            default:
                // For all other errors, drive an abort
                lv_TRerror = lv_BRerror;
                lp_rm->resolved(true);
                break;
            } //switch

         if (pp_txn->trace_level() && lv_TRerror != XA_OK)
            trace_printf("prepare_branches_single_pass, Txn ID (%d,%d), aborting\n",
                         pp_txn->node(),pp_txn->seqnum());
      } //for all completions
   branch_unlock();
   pp_txn->stats()->xa_prepare()->stop();

   TMTrace (2, ("prepare_branches_single_pass, Txn ID (%d,%d), EXIT voting: %s, error=%d, partic=%d, unresolved=%d.\n",
                   pp_txn->node(), pp_txn->seqnum(),
                   (lv_TRerror?"Rollback.":"Commit."),
                   lv_TRerror, num_rm_partic(pp_txn), num_rms_unresolved(pp_txn)));

   return lv_TRerror;
} //prepare_branches_single_pass


//------------------------------------------------------------------------------
// start_branches
// Purpose - send xa_start to all TSE RMs
// An XAER_RMFAIL error will cause the RM to go into an TSEBranch_FAILED
// state.  The TM periodically attempts to re-open failed RMs.
// pv_flags contains the XA flag values.  For TSE RMs this includes
// transaction type bits.
//------------------------------------------------------------------------------
int32 RM_Info_TSE::start_branches (CTmTxBase *pp_txn, int64 pv_flags, CTmTxMessage * pp_msg)
{   
   RM_Info_TSEBranch *la_rms = NULL;
   TM_RM_Responses la_resp[MAX_OPEN_RMS];
   int32           lv_count = 0;
   int32           lv_BRerror = XA_OK;
   // lv_TRerror contains the most recent XA error returned by the RM
   // This means we can loose some!
   int32           lv_TRerror = XA_OK;
   short           lv_replyError = FEOK;
   bool            lv_retry = false;
   RM_Info_TSEBranch *lp_branch; // Used to match rmid from responses to branches.
   
   TMTrace (2, ("RM_Info_TSE::start_branches ENTER flags " PFLL "\n", pv_flags));
   
   pp_txn->stats()->xa_start()->start();
   // Locking the txn object for the entire routine.
   // Nobody else should be touching it anyway!
   branch_lock();
   la_rms = return_rms();
   pp_txn->stats()->RMSend()->start();
   for (int32 lv_idx = 0; lv_idx <= return_highest_index_used(); lv_idx++)
   {
      // Can't assume the transaction RM list is the same as the global one!
      RM_Info_TSEBranch *lp_globRMInfo = gv_RMs.TSE()->return_slot(la_rms[lv_idx].rmid());
      if ((lp_globRMInfo->state() != TSEBranch_UP) &&
          la_rms[lv_idx].partic() && la_rms[lv_idx].in_use() &&
          !la_rms[lv_idx].enlisted())
      {
         TMTrace (1, ("RM_Info_TSE::start_branch rejecting BEGINTRANSACTION because RM %d is in %d state.\n",
                      la_rms[lv_idx].rmid(), lp_globRMInfo->state()));
         // ia_TSEBranches.remove_rm_by_index (lv_idx);  All TSEs participate 
         lv_replyError = FEBEGINTRDISABLED;
         lv_TRerror = XAER_RMFAIL;
      }

      // Determine whether we are to send xa_starts
      if (lv_TRerror == XA_OK && gv_tm_info.TSE_xa_start() &&
          la_rms[lv_idx].partic() && la_rms[lv_idx].in_use())
      {
         lv_BRerror = xa_send(&((*tm_switch).xa_start_entry), &la_rms[lv_idx], pv_flags);
         if (lv_BRerror == XA_OK)
            lv_count++;
         else
         {
            TMTrace (1, ("RM_Info_TSE::start_branch ERROR %s returned by XATM Library "
                           "for rmid=%d, transaction will abort.\n",
                           XAtoa(lv_BRerror), la_rms[lv_idx].rmid()));

            lv_TRerror = lv_BRerror;
            switch (lv_BRerror)
            {
            case XAER_INVAL:
               tm_log_event(DTM_TMTX_PROG_ERROR, SQ_LOG_CRIT, "DTM_TMTX_PROG_ERROR");
               // Programming error!
               abort ();
               break;
            case XAER_RMFAIL:
               // RM hard down
               fail_rm(la_rms[lv_idx].rmid());
               // ia_TSEBranches.remove_rm_by_index (lv_idx); Must send rollback
               lv_replyError = FEDEVICEDOWNFORTMF;
               break;
            default:
               // For all other errors, set the rm as non-participating
               // ia_TSEBranches.remove_rm_by_index (lv_idx); Never forget a TSE!
                lv_replyError = FEDEVICEDOWNFORTMF;
               break;
            } //switch
         } //else error
      } //if 
   } //for each rm

   // respond back to the app to allow processing to continue
   pp_msg->reply(lv_replyError);

   // Currently failing to send xa_start to a TSE causes the begintransaction to fail.
   // When this happens, we still need to complete any outstanding xa_starts and drive 
   // rollback, but we want to make sure we don't forget that we failed.
   int32 lv_repliesOutstanding = complete_all(lv_count, la_resp, pp_txn->rm_wait_time(), pp_txn->legacyTransid());
   pp_txn->stats()->RMSend_stop();

   // We process any completions even if we had an error on any of the sends.  The error 
   // returned by start_branches is always the last one encountered.
   if (lv_repliesOutstanding > 0)
      lv_TRerror = XAER_RMFAIL;
   else
      // if there are errors for the xa_start, record them as non partic
      for (int32 lv_idx = 0; lv_idx < lv_count; lv_idx++)
      {
         lp_branch = return_slot(la_resp[lv_idx].iv_rmid);
         if (lp_branch == NULL)
         {
            tm_log_event(DTM_TMTX_GETRM_FAIL, SQ_LOG_CRIT, "DTM_TMTX_GETRM_FAIL",
                        -1,la_resp[lv_idx].iv_rmid);
            // We had better find a matching RM or there's something very wrong!
            TMTrace (1, ("RM_Info_TSE::start_branches - Get resource manager %d failed\n", 
                     la_resp[lv_idx].iv_rmid));
            abort ();
         }

         lv_BRerror = la_resp[lv_idx].iv_error;
         if (lv_BRerror == XA_OK)
         {
           lp_branch->enlisted(true);
         }
         else // error
         {
            // Error specific processing:
            TMTrace (1, ("RM_Info_TSE::start_branch ERROR %s returned by RM, rmid=%d, transaction will abort.\n",
                           XAtoa(lv_BRerror), lp_branch->rmid()));

            lv_TRerror = lv_BRerror;
            switch (lv_BRerror)
            {
            case XA_RETRY:
                lp_branch->enlisted(false);
                lv_retry = true;
                break;
            case XAER_RMFAIL:
               // RM hard down
               fail_rm(lp_branch->rmid());
               // ia_TSEBranches.remove_rm_by_rmid (lp_rm->rmid());
               break;
            default:
               // For all other errors, set the rm as non-participating
               // ia_TSEBranches.remove_rm_by_rmid (lp_rm->rmid());
               break;
            } //switch
         } //else error
      } //for each completion
   branch_unlock();
   pp_txn->stats()->xa_start()->stop();

   if (lv_retry)
       lv_TRerror = XA_RETRY;

   TMTrace (2, ("RM_Info_TSE::start_branches EXIT returning %s\n", 
                   XAtoa(lv_TRerror)));

   return lv_TRerror;
} // start_branches


//------------------------------------------------------------------------------
// registerRegion
// Purpose - Not implemented for TSE branches
//------------------------------------------------------------------------------
int32 RM_Info_TSE::registerRegion (CTmTxBase *pp_txn, int64 pv_flags, CTmTxMessage * pp_msg)
{   
   TMTrace (2, ("RM_Info_TSE::registerRegion **NOT SUPPORTED FOR TSE BRANCHES!\n"));
   return FEBADERR;
} // registerRegion


//------------------------------------------------------------------------------
// shutdown_branches
// Purpose - shutdown TSE branches
// Library.
//------------------------------------------------------------------------------
int32 RM_Info_TSE::shutdown_branches (bool pv_leadTM, bool pv_clean)
{   
   TM_RM_Responses  la_resp[MAX_OPEN_RMS];
   char             la_xa_close_info[MAX_RM_CLOSE_INFO_LEN];

   int32    lv_error = FEOK;
   int32    lv_fatal_error = FEOK;
   int32    lv_msg_count = 0;

   TMTrace (2, ("shutdown_branches ENTRY Lead TM %d, clean? %d.\n",
            pv_leadTM, pv_clean));

   memset((char *) &la_xa_close_info, 0, MAX_RM_CLOSE_INFO_LEN);

   if (pv_leadTM)
   {
      if (pv_clean)
         sprintf(la_xa_close_info, XA_CLOSE_SHUTDOWN_CLEAN);
      else
         sprintf(la_xa_close_info, XA_CLOSE_SHUTDOWN_DIRTY);
   }

   for (int lv_idx = 0; lv_idx < MAX_OPEN_RMS; lv_idx++)
   {
      if (ia_TSEBranches[lv_idx].in_use())
      {
         lv_error = (*tm_switch).xa_close_entry((char *) &la_xa_close_info, 
                       ia_TSEBranches[lv_idx].rmid(), TMNOFLAGS);

         if (lv_error != FEOK)
         {
            TMTrace(1, ("RM_Info_TSE::shutdown_branches ERROR %d returned by xa_close() for rmid=%d.\n",
                            lv_error, ia_TSEBranches[lv_idx].rmid()));

            lv_fatal_error = lv_error;
         }
         else
         {
            lv_msg_count++;
         }
      }
   } // for

   int32 lv_repliesOutstanding = complete_all(lv_msg_count, la_resp, MAX_RM_WAIT_TIME);
   if (lv_repliesOutstanding > 0)
   {
      TMTrace(1, ("RM_Info_TSE::shutdown_branches ERROR: %d RMs failed to reply.\n",
              lv_repliesOutstanding));
      lv_fatal_error = XAER_RMFAIL;
   }
   else
      for (int lv_idx = 0; lv_idx < lv_msg_count; lv_idx++)
      {
         if (la_resp[lv_idx].iv_error != XA_OK)
         {
            TMTrace(1, ("RM_Info_TSE::shutdown_branches ERROR %d returned by xa_close() for rmid=%d.\n",
                           la_resp[lv_idx].iv_error, la_resp[lv_idx].iv_rmid));

            lv_fatal_error = la_resp[lv_idx].iv_error;
         }
      }

   TMTrace(2, ("RM_Info_TSE::shutdown_branches EXIT, error %d.\n", lv_fatal_error));
   return lv_fatal_error;
} // shutdown_branches
