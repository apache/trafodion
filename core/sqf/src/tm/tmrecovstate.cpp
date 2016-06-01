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

#include <assert.h>
#include "tmrecov.h"
#include "tminfo.h"
#include "tmrecovstate.h"
#include "tmrm.h"

// seabed includes
#include "seabed/ms.h"
#include "seabed/trace.h"
#include "common/sq_common.h"
#include "tmlogging.h"
#include "tmtxbranches.h"


//----------------------------------------------------------------------------
// CTmRmTxRecoveryState::CTmRmTxRecoveryState constructor
//----------------------------------------------------------------------------
CTmRmTxRecoveryState::CTmRmTxRecoveryState(TM_Txid_Internal *pp_transid, TM_TX_STATE pv_state) 
        :iv_state(pv_state), iv_abort_flags(0), iv_rms_to_recover(0) 
{
    memcpy(&iv_transid, pp_transid, sizeof(iv_transid));
}


//----------------------------------------------------------------------------
// CTmRmTxRecoveryState::add_partic
// Add the rm identified by pv_rmid as a participant in this transaction for
// this CTmRmTxRecoveryState object.
//----------------------------------------------------------------------------
void CTmRmTxRecoveryState::add_partic(int32 pv_rmid)
{
    RM_Info_TSEBranch *lp_rm = (RM_Info_TSEBranch *) iv_rmList.get(pv_rmid);
    if (lp_rm == NULL)
    {
        // Lookup RM
        lp_rm = gv_RMs.TSE()->return_slot(pv_rmid);
        if (lp_rm)
            iv_rmList.put(pv_rmid, lp_rm);
    }
    if (lp_rm != NULL)
    {
        lp_rm->inc_totalBranchesLeftToRecover();
        inc_rms_to_recover();
    }
} //add_partic


//----------------------------------------------------------------------------
// CTmRmTxRecoveryState::remove_partic
// Remove the rm identified by pv_rmid as a participant in this transaction for
// this CTmRmTxRecoveryState object.
//----------------------------------------------------------------------------
void CTmRmTxRecoveryState::remove_partic(int32 pv_rmid)
{
    RM_Info_TSEBranch *lp_rm = (RM_Info_TSEBranch *) iv_rmList.get(pv_rmid);
    if (lp_rm != NULL)
    {
        lp_rm->dec_totalBranchesLeftToRecover();
        dec_rms_to_recover();
    }
} //remove_partic
