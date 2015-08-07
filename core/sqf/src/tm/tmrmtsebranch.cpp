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
#include "tmrmtsebranch.h"

// ----------------------------------------------------------------
// RM_Info_TSEBranch::init
// Purpose : Initialize a TSE branch.
// ----------------------------------------------------------------
void RM_Info_TSEBranch::init(int32 pv_pid, int32 pv_nid, char *pp_name, int32 pv_rmid,
                      bool pv_is_ax_reg, TSEBranch_state pv_state, bool pv_clean_init)
{
    iv_state = pv_state;
    if (pv_clean_init)
    {
         iv_in_use = false;
         iv_partic = false;
         iv_resolved = false;
         iv_enlisted = false;
         iv_rmid = iv_pid = iv_nid = -1;
    }
    else
    {
        iv_in_use = true;
        iv_rmid = pv_rmid;
        iv_pid = pv_pid;
        iv_nid = pv_nid;
        strncpy (ia_name, pp_name, 255);

        if (pv_is_ax_reg)
            iv_partic = false;
        else
            iv_partic = true;

        // XID related fields
        TM_RM_Branch_Data lv_bdata;
        iv_xid.formatID = FORMAT_ID;
        iv_xid.gtrid_length = sizeof (TM_Transid_Type);
        iv_xid.bqual_length = sizeof (TM_RM_Branch_Data);
        lv_bdata.iv_branchtype = 0;
        lv_bdata.iv_branchid = pv_rmid;
        memcpy (&iv_xid.data[iv_xid.gtrid_length],
                &lv_bdata, sizeof (TM_RM_Branch_Data));

    }
}


// ----------------------------------------------------------------
// RM_Info_TSEBranch::close
// Purpose : Put the RM in a closed state.
// Periodically the TM will attempt to reopen any failed
// RMs.
// ----------------------------------------------------------------
void RM_Info_TSEBranch::close()
{
   TMTrace (2, ("RM_Info_TSEBranch::close ENTRY for rmid %d\n", iv_rmid));

   state(TSEBranch_DOWN);
   in_use(false);

   TMTrace (2, ("RM_Info_TSEBranch::close EXIT for rmid=%d.\n", iv_rmid));
} //RM_Info_TSEBranch::close


// ----------------------------------------------------------------
// RM_Info_TSEBranch::fail
// Purpose : Put the RM in a failed state.
// Periodically the TM will attempt to reopen any failed
// RMs.
// ----------------------------------------------------------------
void RM_Info_TSEBranch::fail()
{
   TMTrace (2, ("RM_Info_TSEBranch::fail ENTRY for rmid %d\n", iv_rmid));

   state(TSEBranch_FAILED);
   // Don't reset in_use, it will stop new transactions attempting
   // to use this RM.  Transactions started with static registration
   // MUST enlist ALL TSEs in the transaction.
   //in_use(false);

   TMTrace (2, ("RM_Info_TSEBranch::fail EXIT for rmid=%d.\n", iv_rmid));
} //RM_Info_TSEBranch::fail


// ----------------------------------------------------------------
// RM_Info_TSEBranch::up
// Purpose : Put the RM in a up state.
// Periodically the TM will attempt to reopen any failed
// RMs.
// ----------------------------------------------------------------
void RM_Info_TSEBranch::up()
{
   TMTrace (2, ("RM_Info_TSEBranch::up ENTRY for rmid %d\n", iv_rmid));

   state(TSEBranch_UP);
   in_use(true);

   TMTrace (2, ("RM_Info_TSEBranch::up EXIT for rmid=%d.\n", iv_rmid));
} //RM_Info_TSEBranch::up


// ----------------------------------------------------------------
// RM_Info_TSEBranch::copyto
// Purpose : Copy the contents of this RM_Info_TSEBranch to a RM_INFO
// structure.
// ----------------------------------------------------------------
void RM_Info_TSEBranch::copyto(RM_INFO *pp_output)
{
    memcpy(pp_output->ia_name, ia_name, sizeof(pp_output->ia_name));
    pp_output->iv_state = iv_state;
    pp_output->iv_in_use = iv_in_use;
    pp_output->iv_nid = iv_nid;
    pp_output->iv_rmid = iv_rmid;
    pp_output->iv_partic = iv_partic;
    pp_output->iv_pid = iv_pid;
    pp_output->iv_totalBranchesLeftToRecover = iv_totalBranchesLeftToRecover;
} //RM_Info_TSEBranch::copyto


// ----------------------------------------------------------------
// RM_Info_TSEBranch::dec_totalBranchesLeftToRecover
// Purpose : For recovering RMs this decrements the
// iv_totalBranchesLeftToRecover counter and if it reaches 0,
// changes the RM state to TSEBranch_UP.
// This only needs to be called by commit_branches and rollback_branches
// as transactions can only be in hungCommitted or hungAborted.
// ----------------------------------------------------------------
void RM_Info_TSEBranch::dec_totalBranchesLeftToRecover()
{
   if (iv_state == TSEBranch_RECOVERING &&
       iv_totalBranchesLeftToRecover > 0)
   {
      iv_totalBranchesLeftToRecover--;
      if (iv_totalBranchesLeftToRecover <= 0)
      {
         up();
      }
   }
} // RM_Info_TSEBranch::dec_totalBranchesLeftToRecover


// ----------------------------------------------------------------
// RM_Info_TSEBranch::add_partic
// Purpose : Add this branch as a participant in the transaction.
// If we calling this it means the RM sent us an ax_reg or
// a doomtxn, so we can assume it is up even if previously
// marked down.
// Returns true if the branch was added as a participant
//         false if the branch is already participating
// pp_transid input Transid in which particpation is being requested.
// ----------------------------------------------------------------
bool RM_Info_TSEBranch::add_partic(TM_Txid_Internal pv_transid)
{
    bool lv_particAdded = false;

    if (partic())
    {
       TMTrace(1,("RM_Info_TSEBranch::add_partic WARNING : Txn ID (%d,%d). "
               "RM %d is already participating in the transaction, ignored.\n",
               pv_transid.iv_node, pv_transid.iv_seq_num, rmid()));
    }
    else
    {
       memcpy (&iv_xid.data, &pv_transid, sizeof(TM_Txid_Internal));
       partic(true);
       // If it's particpating, the RM must be up!
       up();
       lv_particAdded = true;
       TMTrace(3, ("RM_Info_TSEBranch::add_partic : Txn ID (%d,%d) participation set for rmid(%d).\n",
                   pv_transid.iv_node, pv_transid.iv_seq_num, rmid()));
    }

    TMTrace(1, ("RM_Info_TSEBranch::add_partic EXIT : Participation set for rmid(%d) returning %d.\n ",
            rmid(), lv_particAdded));
    return lv_particAdded;
} // RM_Info_TSEBranch::add_partic

#ifdef DEBUG_MODE
void RM_Info_TSEBranch::in_use( bool pv_in_use)
            {   TMTrace(3,("RM %s(%d): set in_use old=%d, new=%d.\n",ia_name, iv_rmid, iv_in_use, pv_in_use));
               iv_in_use = pv_in_use;}
void RM_Info_TSEBranch::partic( bool pv_partic )
            {   TMTrace(3,("RM %s(%d): set partic old=%d, new=%d.\n",ia_name, iv_rmid, iv_partic, pv_partic));
               iv_partic = pv_partic;}
void RM_Info_TSEBranch::resolved( bool pv_resolve )
            {   TMTrace(3,("RM %s(%d): set resolved old=%d, new=%d.\n",ia_name, iv_rmid, iv_resolved, pv_resolve));
               iv_resolved = pv_resolve;}
void RM_Info_TSEBranch::enlisted( bool pv_enlist )
            {   TMTrace(3,("RM %s(%d): set enlisted old=%d, new=%d.\n",ia_name, iv_rmid, iv_enlisted, pv_enlist));
                  iv_enlisted = pv_enlist;}
#endif //DEBUG_MODE
