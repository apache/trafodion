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

#ifndef TMRECOV_H_
#define TMRECOV_H_

#include "tmaudit.h"
#include "tmtx.h"
#include "tmtxkey.h"
#include "tmrecovstate.h"
#include "tmtxbranches.h"

typedef enum {
   RECOV_REDRIVE_COMMIT    = 1,
   RECOV_REDRIVE_ROLLBACK  = 2,
   RECOV_FORGET            = 3,
   RECOV_FORGET_HEURISTIC  = 4,
   RECOV_HUNGCOMMITTED     = 5,
   RECOV_HUNGABORTED       = 6
} RECOV_STATE_TYPE;


class TM_Recov
{
private:
   int32        iv_total_txs_to_recover;
   int32        iv_max_txs_to_recover;
   int32        iv_rm_wait_time;
   TM_Info      *ip_tm_info;
   CTmTxBranches *ip_branches;
   // iv_txnStateList is a linked list of indoubt txn states (CTmRmTxRecoveryState 
   // objects).  These are the transaction states for transactions found on the 
   // backward scan.  These will be used to construct TM_TX_Info objects once we
   // have responses to xa_recover from all the TSEs.  This allows us to filter 
   // out forgotten transactions and avoid the memory overhead of full TM_TX_Info
   // objects for forgotten transactions which most of the indoubt txn candidates
   // will be.
   TM_MAP       iv_txnStateList; 
   // iv_txnList is the list of indoubt transactions (TM_TX_Info objects) used to build
   // recovery lists for subordinate RMs (TSEs).
   TM_MAP       iv_txnList;

   // iv_builtlist is to be set after the tx list is built the first time.  In the case
   // of double failures, we'll need to know that so we can send the appropriate flags to the
   // TSEs for xa_recover.
   // This flag is active from the time the list is built until the time the RECOVERY END is sent
   bool         iv_listBuilt;

   CTmRmTxRecoveryState *add_txState(TM_Txid_Internal *pp_transid, TM_TX_STATE pv_state);
   CTmRmTxRecoveryState *get_txState(TM_Txid_Internal *pp_transid);
   void                  remove_txState(int64 pv_key);

   TM_TX_Info * new_txinfo(TM_Txid_Internal *pp_transid);
   void         add_txinfo(TM_TX_Info *pp_txinfo);
   TM_TX_Info * get_txinfo(TM_Txid_Internal *pp_transid);
   void         remove_txinfo(int64 pv_key);

   void         sync_recov_data_write_state(TM_TX_Info *pp_trans, int32 pv_nid, 
                                            int32 pv_pid, TM_TX_STATE pv_state);
   int32        redrive_recov_tx(RECOV_STATE_TYPE pv_state, TM_TX_Info *pp_trans);
   int32        scantrail_cleanshutdown();
   int32        scantrail_bldgtxlist(int32 pv_dead_dtm = -1, bool pv_scan_all = false);
   int32        send_xa_recover(int32 pv_rmid, bool pv_rmfailure = false);
   int32        send_xa_recover_toall_TSEs(int32 pv_dead_dtm = -1);
   int32        resolveTxn(TM_TX_Info * pp_txn);
   void         remove_forgotten_txs();

public:
   TM_Recov(int32 pv_rm_wait_time);
   ~TM_Recov();

   int32 initiate_start_sync();
   int32 recover_system();
   int32 recover_dtm_death(int32 pv_dtm);
   int32 recover_tse_restart(int32 pv_rmid);
   int32 updateTxnsToRecover();
   void  completeRecovery();
   void  listBuilt(bool pv_list);
   void  queueTxnObjects(bool pv_ignoreDupTxns = false);
   int32 resolve_in_doubt_txs(int32 pv_dtm = -1, int32 delay = 0);
   TM_MAP * txnList();
   TM_MAP * txnStateList();
   int32 total_txs_to_recover();
   void  dec_txs_to_recover();
   void update_registry_txs_to_recover(int32 pv_txns);

   void rm_wait_time (int32 pv_rm_wait_time) {iv_rm_wait_time = pv_rm_wait_time;}
   int32 rm_wait_time () {return iv_rm_wait_time;}
};

inline TM_MAP * TM_Recov::txnList()
{
   return &iv_txnList;
}

inline TM_MAP * TM_Recov::txnStateList()
{
   return &iv_txnStateList;
}
      
inline int32 TM_Recov::total_txs_to_recover()
{ 
   return iv_total_txs_to_recover; 
}

inline void TM_Recov::dec_txs_to_recover()
{ 
   iv_total_txs_to_recover--; 
}


#endif
