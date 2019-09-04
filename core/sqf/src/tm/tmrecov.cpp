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
#include "xa.h"
#include "xatmapi.h"
#include "tmrm.h"
#include "tmaudit.h"
#include "tmregistry.h"
#include "tmtxkey.h"
#include "hbasetm.h"

// seabed includes
#include "seabed/ms.h"
#include "seabed/trace.h"
#include "common/sq_common.h"
#include "tmlogging.h"


TM_Recov::TM_Recov(int32 pv_rm_wait_time)
    : iv_total_txs_to_recover(0), iv_max_txs_to_recover(0), iv_rm_wait_time(pv_rm_wait_time),
      ip_tm_info(&gv_tm_info), ip_branches(&gv_RMs), iv_listBuilt(false)
{
}

TM_Recov::~TM_Recov() 
{
    txnStateList()->clear();
    txnList()->clear();
}

int32 TM_Recov::initiate_start_sync()
{
   TMTrace (2, ("TM_Recov::initiate_start_sync ENTRY\n"));
   char    la_auditsvc_ready[2];
   int32   lv_error = FEOK;

   if (!(ip_tm_info->lead_tm()))
   {
        tm_log_event(DTM_RECOV_INVALID_LEAD_TM, SQ_LOG_CRIT, "DTM_RECOV_INVALID_LEAD_TM");
        TMTrace (1, ("TM_Recov::init_start_sync - Only Lead DTM can perform system recovery\n"));
        abort();  // Only the lead TM can perform system recovery.
   }
   
   // Check if the DTM ASE, i.e. $TLOG, is ready for audit service.
   do {
         lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, // type
                    (char *) "CLUSTER",                   // group
                    (char *) "SQ_AUDITSVC_READY",         // key
                    la_auditsvc_ready);                   // key value

      } while ((lv_error == -1) && (atoi(la_auditsvc_ready) == 0));

   if (lv_error)
   {
       TMTrace (1, ("TM_Recov::initiate_start_sync EXIT, error %d\n", lv_error));
       return lv_error;
   }

   ip_tm_info->set_sys_recov_status(TM_SYS_RECOV_STATE_START, ip_tm_info->nid());
   
   // Tell all the other TMs that system recovery has started.
#ifdef SUPPORT_TM_SYNC
   send_sys_recov_start_sync(ip_tm_info->nid());
#endif
   TMTrace (2, ("TM_Recov::initiate_start_sync EXIT.\n"));
   return lv_error;
}


int32 TM_Recov::recover_system()
{
   int32   lv_error = FEOK;
   int32   lv_my_nid = ip_tm_info->nid();
   
   TMTrace (2, ("TM_Recov::recover_system ENTRY.\n"));  

   tm_log_event(DTM_SYSTEM_RECOVERY_INITIATED, SQ_LOG_NOTICE, "DTM_SYSTEM_RECOVERY_INITIATED",-1,-1,lv_my_nid);
   TMTrace (1, ("TM_Recov::recover_system : $TM%d System startup recovery initiated.\n", lv_my_nid));

   // Initialize the transactions to recover registry value
   update_registry_txs_to_recover(0);

   lv_error = scantrail_cleanshutdown();
   if (lv_error == FETMRECOVERY_NOT_NEEDED)  
   {
      // Latest Seaquest shutdown was clean.
      // No recovery is needed.
      ip_tm_info->write_control_point(true);
      ip_tm_info->write_control_point(true, true);
      ip_tm_info->set_sys_recov_status(TM_SYS_RECOV_STATE_END, lv_my_nid);
      //send_sys_recov_end_sync(lv_my_nid);
      lv_error = FEOK;
      return (lv_error);
   }

   // First build the list of transaction states that the TSE care about
   // send xa_recover to each opened RM
   lv_error = send_xa_recover_toall_TSEs();

   if (lv_error != FEOK)
   {
      TMTrace (2, ("TM_Recov::recover_system warning at least one TSE down all records will be added, ERROR: %d\n",lv_error));
      tm_log_event(DTM_RECOVERY_SCAN_ALL1, SQ_LOG_WARNING, "DTM_RECOVERY_SCAN_ALL1", -1, -1, -1, -1, -1, lv_error);
      lv_error = scantrail_bldgtxlist(-1, true);
   }
   else {
      // perform scan-trail on TLOG and flush out the state of the outstanding tx state list
      lv_error = scantrail_bldgtxlist();
   }

   iv_max_txs_to_recover = iv_total_txs_to_recover;
   update_registry_txs_to_recover(iv_max_txs_to_recover);
   tm_log_event(DTM_RECOVERY_TXNS_TO_RECOVER, SQ_LOG_NOTICE, "DTM_RECOVERY_TXNS_TO_RECOVER",
                -1, /*error_code*/ 
                -1, /*rmid*/
                lv_my_nid, /*dtmid*/ 
                -1, /*seq_num*/
                -1, /*msgid*/
                -1, /*xa_error*/
                -1, /*pool_size*/
                -1, /*pool_elems*/
                -1, /*msg_retries*/
                -1, /*pool_high*/
                -1, /*pool_low*/
                -1, /*pool_max*/
                -1,  /*tx_state*/
                iv_max_txs_to_recover /*data*/);
   TMTrace (1, ("TM_Recov::recover_system : $TM%d found %d indoubt transactions to recover.\n", lv_my_nid, iv_max_txs_to_recover));
   if (lv_error == FEOK)
   {
      remove_forgotten_txs();
      queueTxnObjects();
      lv_error = resolve_in_doubt_txs();
   }
   else
   {
     tm_log_event(DTM_RECOVERY_FAILED3, SQ_LOG_CRIT, "DTM_RECOVERY_FAILED3", lv_error);
     TMTrace(1, ("TM_Recov::recover_system : $TM%d Recovery failed with error %d, shutting down Seaquest.\n",
             lv_my_nid, lv_error));
     msg_mon_shutdown(MS_Mon_ShutdownLevel_Abrupt);
     exit(lv_error);
    }

   TMTrace (2, ("TM_Recov::recover_system EXIT, returning %d\n",lv_error));
   return lv_error;
} //recover_system


int32 TM_Recov::recover_dtm_death(int32 pv_dtm)
{
   int32   lv_error = FEOK;

   TMTrace (2, ("TM_Recov::recover_dtm_death ENTRY.\n"));

   if (!(ip_tm_info->lead_tm()))
   {
        tm_log_event(DTM_RECOV_INVALID_LEAD_TM, SQ_LOG_CRIT, "DTM_RECOV_INVALID_LEAD_TM");
        TMTrace (1, ("TM_Recov::recover_dtm_death - Only Lead DTM can perform system recovery\n"));
        abort();  // Only the lead TM can perform system recovery.
   } 
   else
   {
      tm_log_event(DTM_NODE_RECOVERY_INITIATED, SQ_LOG_NOTICE, "DTM_NODE_RECOVERY_INITIATED",
                   -1,-1,ip_tm_info->nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,NULL,pv_dtm);
      TMTrace (1, ("TM_Recov::recover_dtm_death : DTM Recovery initiated for node %d\n", pv_dtm));
   }

   // First build the list of transaction states that the TSE care about
   // send xa_recover to each opened RM
   //lv_error = send_xa_recover_toall_TSEs(pv_dtm);

   // Sending ID for node down.
   TMTrace (2, ("TM_Recov::recover_dtm_death: sending node down message.\n"));
   gv_HbaseTM.nodeDown(pv_dtm);

   if (lv_error != FEOK)
   {
      TMTrace (2, ("TM_Recov::recover_dtm_death warning at least one TSE down all records will be added, ERROR: %d\n",lv_error));
      tm_log_event(DTM_RECOVERY_SCAN_ALL2, SQ_LOG_WARNING, "DTM_RECOVERY_SCAN_ALL2", -1, -1, -1, -1, -1, lv_error);
      lv_error = scantrail_bldgtxlist(pv_dtm, true);
   }
   else {
      // perform scan-trail on TLOG and flush out the state of the outstanding tx state list
      lv_error = scantrail_bldgtxlist(pv_dtm);
   }


   // SEND BUILDLIST_START/END here
   TMTrace(3, ("TM_Recov::recover_dtm_death setting recovery_list_built to TRUE for Node %d\n",pv_dtm));
   ip_tm_info->recovery_list_built (pv_dtm, true);
   listBuilt (true);
   //send_recov_listbuilt_sync (ip_tm_info->nid(), pv_dtm);

   iv_max_txs_to_recover = iv_total_txs_to_recover;
   tm_log_event(DTM_RECOVERY_TXNS_TO_RECOVER, SQ_LOG_NOTICE, "DTM_RECOVERY_TXNS_TO_RECOVER",
                -1, /*error_code*/ 
                -1, /*rmid*/
                pv_dtm, /*dtmid*/ 
                -1, /*seq_num*/
                -1, /*msgid*/
                -1, /*xa_error*/
                -1, /*pool_size*/
                -1, /*pool_elems*/
                -1, /*msg_retries*/
                -1, /*pool_high*/
                -1, /*pool_low*/
                -1, /*pool_max*/
                -1,  /*tx_state*/
                iv_max_txs_to_recover /*data*/);
   TMTrace (1, ("TM_Recov::recover_dtm_death : $TM%d found %d indoubt transactions to recover.\n", pv_dtm, iv_max_txs_to_recover));
   if (lv_error == FEOK)
   {
      remove_forgotten_txs();
      queueTxnObjects(true);
      lv_error = resolve_in_doubt_txs(pv_dtm);
   }

   // Did not find any transactions to recover, so we can finish up processing now
   // If there are recoverying transactions, we will finish when all the transactions are truly finished
   if ((lv_error == FEOK) && (iv_total_txs_to_recover == 0))
   {
      tm_log_event(DTM_RECOVERY_COMPLETED, SQ_LOG_NOTICE, "DTM_RECOVERY_COMPLETED",-1,-1,pv_dtm);
      TMTrace (1, ("TM_Recov::recover_dtm_death : DTM Recovery completed for node %d.\n", pv_dtm));
      update_registry_txs_to_recover(0);
   }


   TMTrace (2, ("TM_Recov::recover_dtm_death EXIT, returning %d\n",lv_error));
   return lv_error;
} //TM_Recov::recover_dtm_death

//----------------------------------------------------------------------------
// TM_Recov::recover_tse_restart
// Purpose : recover a single TSE which has been restarted after being stopped
// or failing, or which is new to the TM.
// Note that TSEs currently don't support lock reinstatement, so we must mark
// them as TSEBranch_RECOVERING so that they can't process new transactions!
// Returns a file error code.
//----------------------------------------------------------------------------
int32 TM_Recov::recover_tse_restart(int32 pv_rmid)
{
   int32   lv_error = FEOK;

   TMTrace (2, ("TM_Recov::recover_tse_restart ENTRY.\n"));

   tm_log_event(DTM_RM_RECOVERY_INITIATED, SQ_LOG_NOTICE, "DTM_RM_RECOVERY_INITIATED",
                -1,pv_rmid,ip_tm_info->nid());
   TMTrace (1, ("TM_Recover_recover_tse_restart : DTM individual TSE Recovery initiated for rmid %d\n", pv_rmid));

    // send xa_recover to each opened RM
   lv_error = send_xa_recover(pv_rmid);
   if (lv_error == FEOK)
   {
      lv_error = resolve_in_doubt_txs(ip_tm_info->nid());
   }

   if (lv_error == FETMRECOVERY_NOT_NEEDED)  
   {
      lv_error = FEOK;
   }

   if (lv_error == FEOK)
   {
      tm_log_event(DTM_RECOVERY_COMPLETED, SQ_LOG_NOTICE, "DTM_TSE_RECOVERY_COMPLETED",-1,-1,pv_rmid);
      TMTrace (1, ("TM_Recov::recover_tse_restart : DTM individual TSE Recovery completed for rmid %d.\n", pv_rmid));
   }

   TMTrace (2, ("TM_Recov::recover_tse_restart EXIT, returning %d\n",lv_error));
   return lv_error;
} //TM_Recov::recover_tse_restart


//----------------------------------------------------------------------------
// TM_Recov::add_txState
// Adds a new transaction state element to the iv_txnStateList.  This is keyed
// on the combination of node and sequence number for the transaction.
// Returns the address of the newly created CTmRmTxRecoveryState object.
//----------------------------------------------------------------------------
CTmRmTxRecoveryState *TM_Recov::add_txState(TM_Txid_Internal *pp_transid, TM_TX_STATE pv_state)
{
   CTmTxKey lv_txKey(pp_transid->iv_node, pp_transid->iv_seq_num);
   CTmRmTxRecoveryState *lp_indoubtTxn = new CTmRmTxRecoveryState(pp_transid, pv_state);
   TMTrace (2, ("TM_Recov::add_txState ENTRY ID (%d,%d), state %d to be added to "
            "iv_txnStateList. %d entries to be recovered.\n",
            pp_transid->iv_node, pp_transid->iv_seq_num, 
            pv_state, iv_total_txs_to_recover+1));


   txnStateList()->put(lv_txKey.id(), lp_indoubtTxn);
   iv_total_txs_to_recover++;
   //Only update with the maximum otherwise we generate too many registry change notices
   //update_registry_txs_to_recover(iv_total_txs_to_recover);

   return lp_indoubtTxn;
} //add_txState


//----------------------------------------------------------------------------
// TM_Recov::get_txState
// Returns the CTmRmTxRecoveryState object for the specified transaction.
// If the transaction is not found, null is returned.
//----------------------------------------------------------------------------
CTmRmTxRecoveryState *TM_Recov::get_txState(TM_Txid_Internal *pp_transid)
{
   TMTrace (2, ("TM_Recov::get_txState ENTRY Txn ID (%d,%d).\n", 
       pp_transid->iv_node, pp_transid->iv_seq_num));

   CTmTxKey lv_key(pp_transid->iv_node, pp_transid->iv_seq_num);
   CTmRmTxRecoveryState *lp_indoubtTxn = (CTmRmTxRecoveryState *) txnStateList()->get(lv_key.id());

    return lp_indoubtTxn;
} //get_txState



//----------------------------------------------------------------------------
// TM_Recov::remove_txState
// Remove the current element from the iv_txnStateList.
// This is not a general purpose route.  It must be called only once an
// element has been found using get_first(), get_next(). It assumes that
// the lock is already held.
//----------------------------------------------------------------------------
void TM_Recov::remove_txState(int64 pv_key)
{
   if (pv_key != 0)
   {
       txnStateList()->erase(pv_key);
   }
   else
   {
       TMTrace(1, ("TM_Recov::remove_txState: WARNING no key value "
               "current entry!\n"));
   }
} //remove_thisElementFrom_TxState


//----------------------------------------------------------------------------
// TM_Recov::new_txinfo
// Create a new TM_TX_Info object and add it to the iv_txnList. This is a
// wrapper for TM_Info::import_tx().
// Returns true if successful, 
//         false if the transaction already exists in the list.
//----------------------------------------------------------------------------
TM_TX_Info * TM_Recov::new_txinfo(TM_Txid_Internal *pp_transid)
{
    TMTrace(2, ("TM_Recov::new_txinfo ENTRY : Txn ID (%d,%d).\n", 
            pp_transid->iv_node, pp_transid->iv_seq_num));

    // See if this transaction already exists
    CTmTxKey lv_txKey(pp_transid->iv_node, pp_transid->iv_seq_num);

    TM_TX_Info *lp_tx_info = (TM_TX_Info*) ip_tm_info->transactionPool()->get(lv_txKey.id());

    if (lp_tx_info == NULL)
    {
        lp_tx_info = (TM_TX_Info *) ip_tm_info->import_tx(pp_transid);
        // If we don't succeed in creating a transaction object it means
        // we've exceeded the pool, but since we increased it to allow
        // for the maximum possible transactions, there's a programming
        // bug
        if (lp_tx_info == NULL) 
        {
            tm_log_event(DTM_RECOV_TOO_MANY_TXNS, SQ_LOG_CRIT, "DTM_RECOV_TOO_MANY_TXNS",
                        -1, /*error_code*/ 
                        -1, /*rmid*/
                        ip_tm_info->nid(), /*dtmid*/ 
                        pp_transid->iv_seq_num, /*seq_num*/
                        -1, /*msgid*/
                        -1, /*xa_error*/
                        ip_tm_info->transactionPool()->get_maxPoolSize(), /*pool_size*/
                        ip_tm_info->transactionPool()->get_inUseList()->size() /*pool_elems*/);
            TMTrace (1, ("TM_Recov::new_txinfo - Exceeded maximum transactions during recovery!\n"));
            abort();
        }
        lp_tx_info->initialize_tx_rms(true /*all RMs participate*/);
        lp_tx_info->recovering(true);

        // Add to recovery list iv_txnList
        add_txinfo(lp_tx_info);
        TMTrace(3, ("TM_Recov::new_txinfo - returning Txn ID (%d,%d), %p.\n",
                lp_tx_info->node(), lp_tx_info->seqnum(), lp_tx_info));
    }
    else
    {
        tm_log_event(DTM_RECOV_TXN_FOUND_RECOV_IGNORED, SQ_LOG_WARNING, 
                     "DTM_RECOV_TXN_FOUND_RECOV_IGNORED",
                     -1,-1,pp_transid->iv_node,pp_transid->iv_seq_num,-1,-1,-1,
                     -1,-1,-1,-1,-1,lp_tx_info->tx_state());
        TMTrace (1, ("TM_Recov::new_txinfo - Txn ID (%d,%d) is already present, tx state %d. "
                 "This instantiation ignored!\n", pp_transid->iv_node, 
                 pp_transid->iv_seq_num, lp_tx_info->tx_state()));
    }

    TMTrace(2, ("TM_Recov::new_txinfo - EXIT.\n"));

    return lp_tx_info;
} //new_txinfo


void TM_Recov::add_txinfo(TM_TX_Info *pp_txinfo)
{
    if (pp_txinfo == NULL)
    {
        TMTrace (1, ("TM_Recov::add_txinfo - Received NULL TM_TX_Info when building "
                     "transaction list for recovery"));
        abort();
    }

   TMTrace (2, ("TM_Recov::add_txinfo ENTRY txn ID (%d,%d) to be added to "
                "iv_txnList. %d entries to be recovered.\n",
                pp_txinfo->node(), pp_txinfo->seqnum(), iv_total_txs_to_recover));


   CTmTxKey lv_txKey(pp_txinfo->transid()->iv_node, pp_txinfo->seqnum());

   pp_txinfo->in_use(true);
   txnList()->put(lv_txKey.id(), pp_txinfo);
}

TM_TX_Info * TM_Recov::get_txinfo(TM_Txid_Internal *pp_transid)
{
   TMTrace (2, ("TM_Recov::get_txInfo ENTRY txn ID (%d,%d).\n", 
       pp_transid->iv_node, pp_transid->iv_seq_num));

   CTmTxKey k(pp_transid->iv_node, pp_transid->iv_seq_num);
   return (TM_TX_Info *) txnList()->get(k.id());
}


//----------------------------------------------------------------------------
// TM_Recov::remove_txinfo
// Remove the element at key from the iv_txnList.
// This is not a general purpose route.  It must be called only once an
// element has been found using get_first(), get_next(). It assumed that the 
// lock is held by the caller.
//----------------------------------------------------------------------------
void TM_Recov::remove_txinfo(int64 pv_key)
{
   if (pv_key != 0)
   {
      txnList()->erase(pv_key);
      // Count is now decremented when the actual transaction has finished, not when we have queued transaction thread to run
      //iv_total_txs_to_recover--;
      //update_registry_txs_to_recover(iv_total_txs_to_recover);
      TMTrace(3, ("TM_Recov::remove_txinfo: txns to recover %d.\n", 
              iv_total_txs_to_recover));
   }
   else
   {
      TMTrace(1, ("TM_Recov::remove_txinfo: WARNING no key value.\n"));
   }
} //remove_txinfo


int32 TM_Recov::redrive_recov_tx(RECOV_STATE_TYPE pv_recov_state, TM_TX_Info *pp_trans)
{
   int32             lv_error = TM_NO_ERR;   
   RECOV_STATE_TYPE  lv_recov_state = pv_recov_state;
  
   TMTrace(2, ("TM_Recov::redrive_recov_tx: ENTRY txn ID (%d,%d), txnState = %d, recovState = %d, flags = " PFLLX "\n", 
                     pp_trans->node(), pp_trans->seqnum(), pp_trans->tx_state(), pv_recov_state, pp_trans->TT_flags()));
   
   switch (lv_recov_state)
   {
        case RECOV_REDRIVE_COMMIT:
        {
            pp_trans->recover_tx(TM_MSG_TXINTERNAL_REDRIVECOMMIT);
            break;
        }
        case RECOV_REDRIVE_ROLLBACK:
        {
            pp_trans->recover_tx(TM_MSG_TXINTERNAL_REDRIVEROLLBACK);
            break;
        }
        default:
        {
            // unknown recov state, issue EMS
            TMTrace (2, ("TM_Recov::redrive_recov_tx: PROGRAMMING ERROR Invalid recovery state %d detected.\n",
                        lv_recov_state));
            tm_log_event(DTM_RECOV_BAD_STATE, SQ_LOG_CRIT, "DTM_RECOV_BAD_STATE");
            abort();
        }
   } // switch

   TMTrace (2, ("TM_Recov::redrive_recov_tx: EXIT\n"));
   return lv_error;
} //RM_Recov::redrive_recov_tx

int32 TM_Recov::scantrail_cleanshutdown()
{
   Audit_Header            *lp_audit_header;

   int32 lv_error = FETMRECOVERY_NOT_NEEDED;
  
   // For now, the audit reading functions are not returning errors.  They raise
   // exceptions and abort, causing the TM to abend and the node to go down.
   // Later on, these utility functions can return errors and let the caller
   // process to decide, according to the seriousness of the errors, whether to
   // abend or continue after issuing some events.  For now this function
   // always returns FEOK in Errcode

    
   TMTrace (2, ("TM_Recov::scantrail_cleandshutdown ENTRY.\n"));
   ip_tm_info->start_backwards_scan();

   Addr lp_audit_rec = ip_tm_info->read_audit_rec();
   lp_audit_header = (Audit_Header *) lp_audit_rec;


   // If the first audit record read in the backward scan is the
   // TM_Shutdown record, then the latest DTM shutdown is a clean shutdown
   // (all txs have been committed or aborted and all DTM and RM audits 
   // have been flushed.  Hence, no recovery is needed.
   
   if ((lp_audit_rec != NULL) && (lp_audit_header->iv_type == TM_Shutdown) &&
       (ip_tm_info->SysRecovMode() == CLEAN_SHUTDOWN_OPTIMIZE))
   {
      lv_error = FETMRECOVERY_NOT_NEEDED; 
      TMTrace (2, ("TM_Recov::scantrail_cleanshutdown, recovery not needed.\n"));
   } 

   ip_tm_info->release_audit_rec();
   ip_tm_info->end_backwards_scan();

   TMTrace (2, ("TM_Recov::scantrail_cleandshutdown EXIT.\n"));

   return lv_error;
}

int32 TM_Recov::scantrail_bldgtxlist(int32 pv_dead_dtm, bool pv_scan_all)
{
   Audit_Header            *lp_audit_header;
   Audit_Transaction_State *lp_audit_tx_state_rec;
   TM_Txid_Internal        *lp_transid = NULL;
   CTmRmTxRecoveryState    *lp_txState = NULL;

   int32 lv_error = FEOK;
   bool  lv_first_control_pt_reached = false;
   int32 lv_forgotten_txs = 0;
   bool  lv_new_tx = false;
   bool  lv_second_control_pt_reached = false;

   // For now, the audit reading functions are not returning errors.  They raise
   // exceptions and abort, causing the TM to abend and the node to go down.
   // Later on, these utility functions can return errors and let the caller
   // process to decide, according to the seriousness of the errors, whether to
   // abend or continue after issuing some events.  For now this function
   // always returns FEOK in Errcode

    
   TMTrace (2, ("TM_Recov::scantrail_bldgtxlist ENTRY for DTM %d.\n", pv_dead_dtm));
   ip_tm_info->start_backwards_scan();

   Addr lp_audit_rec = ip_tm_info->read_audit_rec();
   lp_audit_header = (Audit_Header *) lp_audit_rec;


   // If the first audit record read in the backward scan is the
   // TM_Shutdown record, then the latest DTM shutdown is a clean shutdown
   // (all txs have been committed or aborted and all DTM and RM audits 
   // have been flushed.  Hence, no recovery is needed.
   
   if ((lp_audit_rec != NULL) && (lp_audit_header->iv_type == TM_Shutdown) &&
       (ip_tm_info->SysRecovMode() == CLEAN_SHUTDOWN_OPTIMIZE))
   {
      lv_error = FETMRECOVERY_NOT_NEEDED; 
      TMTrace (2, ("TM_Recov::scantrail_bldgtxlist EXIT, recovery not needed.\n"));
      ip_tm_info->release_audit_rec();
      ip_tm_info->end_backwards_scan();
      return lv_error;
   } 
    
   while ((lp_audit_rec != NULL) && !lv_second_control_pt_reached)
   {
      switch (lp_audit_header->iv_type)
      {
         case TM_Control_Point:
         {
            if (!lv_first_control_pt_reached)
            {
               lv_first_control_pt_reached = true;
            }
            else
            {
               lv_second_control_pt_reached = true;
            }
            break;
         }
         case TM_Transaction_State:
         {
            lp_audit_tx_state_rec = (Audit_Transaction_State *)lp_audit_rec;
            lp_transid = (TM_Txid_Internal *) &lp_audit_header->iv_transid;

            if ((pv_dead_dtm != -1) && (lp_transid->iv_node != pv_dead_dtm))
               break;
 
            // Try to find this tx in the recovery tx state list first.
            // We have already gathered all the txs that are known by the TSEs (state will be ABORTED)
            // so we will not instantiate any new transations during the scantrails, we are just 
            // determining the final state (committed or aborted).
            // Exception: When not all the TSEs are up, all indoubt transactions found in TLOG must
            // be included as we don't know which TSEs participated in them.

            lp_txState = get_txState(lp_transid);
            if (lp_txState == NULL)
            {
               if(pv_scan_all) 
               {
                  lp_txState = add_txState(lp_transid, TM_TX_STATE_NOTX);
                  lv_new_tx = true;
               }
               else 
               {  // pv_scan_all not set (all TSEs up), so we can ignore this transaction.
                  TMTrace (4, ("TM_Recov::scantrail_bldgtxlist - Transaction not returned "
                           "by a TSE and ignored.\n"));
               }
            }
            else
               lv_new_tx = false;  
            
            // If we found or created an entry in the txStateList, determine outcome
            if (lp_txState)
            {
               switch (lp_audit_tx_state_rec->iv_state)
               {
                  case Forgotten_Trans_State:
                  {
                     if (lv_new_tx)
                     {                                          
                        //lp_txState->inc_rms_to_recover();
                        lp_txState->abort_flags(lp_audit_tx_state_rec->iv_abort_flags);
                        lp_txState->state(TM_TX_STATE_FORGOTTEN);
                        // We'll ignore any additional state records for this tx in the
                        // backward scan.
                        lv_forgotten_txs++;
                     }
                     else
                     {
                         // Consistency check
                         if (lp_txState->state() != TM_TX_STATE_FORGOTTEN)
                         {
                           //tm_log_event(DTM_RECOV_NOT_FORGOT_STATE, SQ_LOG_CRIT, "DTM_RECOV_NOT_FORGOT_STATE");
                           TMTrace (1, ("TM_Recov::scantrail_bldgtxlist - Transaction not in FORGOTTEN "
                                        "state (during system recovery)\n"));
                           lp_txState->abort_flags(lp_audit_tx_state_rec->iv_abort_flags);
                           lp_txState->state(TM_TX_STATE_FORGOTTEN);
                           lv_forgotten_txs++;
                           // For now allow this inconsistency since database is ok, plan to fix in M6
                           //if (ip_tm_info->overrideAuditInconsistencyDuringRecovery() == false) 
                              //abort();
                         }                      

                     }
                     break;
                  }
                  case Active_Trans_State:
                  {
                     if (lv_new_tx)
                     {                                    
                        lp_txState->state(TM_TX_STATE_ACTIVE);
                     }
                     // Else, this tx already exists in the tx list.
                     // Just ignore this audit record and read another one
                     break;
                  }
                  case HungCommitted_Trans_State:
                  case Committed_Trans_State:
                  {
                     if (lv_new_tx)
                     {                        
                        //lp_txState->inc_rms_to_recover();
                        lp_txState->state(TM_TX_STATE_COMMITTED);
                     }
                     else
                     {
                         if ((lp_txState->state() != TM_TX_STATE_COMMITTED) &&
                             (lp_txState->state() != TM_TX_STATE_FORGOTTEN))
                         {
                           //tm_log_event(DTM_RECOV_INVALID_OLD_CMFG_STATE, SQ_LOG_CRIT,
                           //             "DTM_RECOV_INVALID_OLD_CMFG_STATE");
                           TMTrace (1, ("TM_Recov::scantrail_bldgtxlist - Transaction not in COMMITTED or "
                                            "FORGOTTEN state (during system recovery)\n"));
                           lp_txState->state(TM_TX_STATE_COMMITTED);
                           //if (ip_tm_info->overrideAuditInconsistencyDuringRecovery() == false) 
                           //   abort();
                         }

                     }
                     break;               
                  }
                  case Aborting_Trans_State:
                  {
                     if (lv_new_tx)
                     {
                        //lp_txState->inc_rm_to_recover();
                        lp_txState->abort_flags(lp_audit_tx_state_rec->iv_abort_flags);
                        lp_txState->state(TM_TX_STATE_ABORTING); 
                     }
                     else
                     {
                         if ((lp_txState->state() != TM_TX_STATE_ABORTING) &&
                             (lp_txState->state() != TM_TX_STATE_ABORTING_PART2) &&
                             (lp_txState->state() != TM_TX_STATE_ABORTED))
                         {
                               tm_log_event(DTM_RECOV_INVALID_OLD_ABRT_STATE, SQ_LOG_CRIT, 
                                            "DTM_RECOV_INVALID_OLD_ABRT_STATE");
                               TMTrace (1, ("TM_Recov::scantrail_bldgtxlist - Transaction not in ABORTED or ABORTING"
                                                "state (during system recovery)\n"));
                               if (ip_tm_info->overrideAuditInconsistencyDuringRecovery() == false) 
                                  abort();

                         }
                     }
                     break;
                  }
                  case HungAborted_Trans_State:
                  case Aborted_Trans_State:
                  {
                     if (lv_new_tx)
                     {
                        //lp_txState->inc_rms_to_recover();
                        lp_txState->abort_flags(lp_audit_tx_state_rec->iv_abort_flags);
                        lp_txState->state(TM_TX_STATE_ABORTED);
                     }
                     else
                     {
                         if ((lp_txState->state() != TM_TX_STATE_ABORTING) &&
                             (lp_txState->state() != TM_TX_STATE_ABORTING_PART2) &&
                             (lp_txState->state() != TM_TX_STATE_ABORTED))
                         {
                           tm_log_event(DTM_RECOV_INVALID_OLD_ABRT_STATE, SQ_LOG_CRIT,
                                        "DTM_RECOV_INVALID_OLD_ABRT_STATE");
                           TMTrace (1, ("TM_Recov::scantrail_bldgtxlist - Transaction not in ABORTED or "
                                           "ABORTING state (during system recovery)\n"));
                           if (ip_tm_info->overrideAuditInconsistencyDuringRecovery() == false) 
                              abort();
                         }
                     }
                     break;                  
                  }
                  default:
                  {
                    // unknown tx state audit record
                     break;
                  }
               }  // switch iv_state
               break;            
            }  // case TM_Transaction_State
            default:
            {
               break;
            }
         }  // switch iv_type
      } // if txState entry exists

      ip_tm_info->release_audit_rec();            
      lp_audit_rec = ip_tm_info->read_audit_rec();
      lp_audit_header = (Audit_Header *) lp_audit_rec;
   }  // while
   
   ip_tm_info->end_backwards_scan();
 
   TMTrace (2, ("TM_Recov::scantrail_bldgtxlist EXIT, %d txns to be recovered.\n", iv_total_txs_to_recover));
   return lv_error;  // FEOK     
}


//----------------------------------------------------------------------------
// TM_Recov::send_xa_recover
// Purpose : send xa_recover message to the TSE specified by pv_rmid.
// Must have previously called xa_open to establish a connection to the TSE.
// This variant is used by any TM (not just the Lead TM) to retrieve a list of
// transaction branches the TSE considers indoubt following a TSE failure, 
// TSE stop and restart, or starting of a new TSE after Seaquest is up
// and running.
// Returns: 
//    FEOK if successful.
//    FEBADREPLY if the send fails or the reply contains an error.
//----------------------------------------------------------------------------
int32 TM_Recov::send_xa_recover(int32 pv_rmid, bool pv_rmfailure)
{
   char              la_buf[DTM_STRING_BUF_SIZE];
   RMID              lv_rmid;
   XID              *lp_xids;
   TM_Txid_Internal *lp_transid;
   TM_TX_Info       *lp_tx_info;

   int64 lv_count;
   bool  lv_end = false;
   int   lv_index=0;
   int64 lv_maxCount;
   int   lv_xa_result;
   int32 lv_returnError = FEOK;
   int32 lv_int_error = FEOK;
   int32 lv_retry_count = 0;
   uint64 lv_flags;

   // Since this gets called for a TSE restart, we won't have a case where (with this method)
   // we would need to set this to TMRESENDRSCAN.  If this is called as the result of a failed
   // RM, we need to send the TMRMFAILRSCAN so the TSEs know what to do.

//   if (!pv_rmfailure)
      lv_flags = TMSTARTRSCAN;
//   else
//      lv_flags = TMRMFAILRSCAN;  reviewers : this is comments out “just in case”.  We don’t want to crash any TSEs with someone playing around with this on accident



   //startScan: True if we have not received the first full buffer from the RM.
   //           False if we have received a buffer and are retrieving the next buffer.
   //           rmid is the index.  Initially true.
   bool lv_startScan[MAX_OPEN_RMS]; 
   
   TMTrace (2, ("TM_Recov::send_xa_recover: ENTRY rmid %d, flags " PFLLX ", index %d.\n", 
                pv_rmid, lv_flags, lv_index));

  // Grab the mutex to make sure we only send one xa_recover to the TSEs at a time
   ip_tm_info->recovery_lock();

   lv_maxCount = MIN(MAX_RECOVER_XIDS, ip_tm_info->max_trans());
   lp_xids = new XID[lv_maxCount];
   for (int i=0;i<MAX_OPEN_RMS;i++)
       lv_startScan[i] = true;

   // Send the initial xa_recover message to the RM.
   lv_xa_result = tm_xa_recover_send(pv_rmid, lv_maxCount, 
                                     lv_flags, lv_index, ip_tm_info->nid(), false);
   if (lv_xa_result != XA_OK)
   {
      sprintf(la_buf, "xa_recover RM %d returned XA error %d. Unable to "
              "continue recovery.\n", pv_rmid, lv_xa_result);
      tm_log_event(DTM_XATM_RECOVER_SEND_FAILED, SQ_LOG_CRIT, "DTM_XATM_RECOVER_SEND_FAILED",-1,
                   pv_rmid,-1,-1,-1,lv_xa_result);
      TMTrace (1, ("TM_Recov::send_xa_recover: %s",la_buf));
      // Can't continue!
      delete [] lp_xids;
      ip_tm_info->recovery_unlock();
      return FEBADREPLY;
   }

   // While we have outstanding xa_recover requests, wait for and process 
   // the replies.
   while (lv_end == false && lv_returnError == FEOK)
   {
      lv_count = lv_maxCount;
      memset(lp_xids, 0, sizeof(XID)*lv_count);
      lv_xa_result = tm_xa_recover_waitReply((int *)&lv_rmid.iv_rmid, lp_xids, &lv_count, 
                                             &lv_end, &lv_index, rm_wait_time(), &lv_int_error);
      if ((lv_xa_result == XA_RETRY) && (lv_int_error != FEOK))
      {
         if (lv_retry_count++ >= TM_LINKRETRY_RETRIES)
         {
             lv_xa_result = XAER_RMFAIL;
             TMTrace(1,("TM_Recov::send_xa_recover_toall_TSEs: Failing rmid %d "
                    "because of too many errors (%d).\n", lv_rmid.iv_rmid, lv_xa_result));
         }
      }

      switch (lv_xa_result)
      {
      case XA_OK:
          lv_startScan[lv_rmid.s.iv_num] = false;
          // Intentional drop through
      case XA_RETRY:
       {
         // If there are more XIDs still to be received send another xa_recover.
         if (lv_xa_result == XA_RETRY || lv_end == false)
         {
            // If the last xa_recover completed without lv_end set, then there are more XIDs to get
            if (lv_startScan[lv_rmid.s.iv_num] == true)
               lv_flags = TMSTARTRSCAN;
            else
               lv_flags = TMNOFLAGS;

            // Send a new xa_recover request
            TMTrace (3, ("TM_Recov::send_xa_recover (2): sending xa_recover with rmid %d, flags " PFLLX " and "
                         " index %d.\n", pv_rmid, lv_flags, lv_index));
            lv_xa_result = tm_xa_recover_send(pv_rmid, lv_maxCount, 
                                              lv_flags, lv_index, ip_tm_info->nid(), false);
            if (lv_xa_result != XA_OK)
            {
               sprintf(la_buf, "xa_recover RM %d returned XA error %d. Unable to "
                     "continue recovery(2).\n", lv_rmid.iv_rmid, lv_xa_result);
               tm_log_event(DTM_XATM_RECOVER_SEND2_FAILED, SQ_LOG_CRIT, "DTM_XATM_RECOVER_SEND2_FAILED",-1,
                         lv_rmid.iv_rmid,-1,-1,-1,lv_xa_result);
               TMTrace (1, ("TM_Recov::send_xa_recover: %s",la_buf));
               // Can't continue!
               delete [] lp_xids;
               ip_tm_info->recovery_unlock();
               return FEBADREPLY;
            }
         }

         // Process returned XIDs from this reply:
         for (int32 j = 0; j < lv_count; j++)
         {
            lp_transid  = (TM_Txid_Internal *) &lp_xids[j].data;
            if (lp_transid->iv_seq_num != 0)
            {
               TMTrace (3, ("TM_Recov::send_xa_recover: Warning XID 0 ignored: XID entry, %d "
                            "node %d, ID %d.\n",
                            j, lp_transid->iv_node, lp_transid->iv_seq_num));
            }
            else
            {
               TMTrace (3, ("TM_Recov::send_xa_recover: Recovering XID entry, %d "
                            "node %d, ID %d.\n",
                            j, lp_transid->iv_node, lp_transid->iv_seq_num));
               lp_tx_info = get_txinfo(lp_transid);
            
               if (lp_tx_info != NULL)
               {
                   TMTrace (3, ("TM_Recov::send_xa_recover: Txn object found for XID entry, %d "
                                  "node %d, rmid %d, ID %d.\n",
                                  j, lp_transid->iv_node, pv_rmid, lp_transid->iv_seq_num));
                  // Set the RM's 'partic' flag to true
                  lp_tx_info->safe_initialize_slot(pv_rmid);
                  lp_tx_info->inc_prepared_rms();
                  lp_tx_info->recovering(true);
               }
               else
               {
                  // Can't find the tx in the recover tx list.  We must not have
                  // generated a trans state record for the transaction yet when
                  // the environment failed (i.e. no control point processing).
                  // So, need to add the tx to the list and send xa_rollback to
                  // the RM, to drive abort processing.
                     TMTrace (3, ("TM_Recov::send_xa_recover: Txn object not found for XID entry, %d "
                                  "node %d, rmid %d, ID %d.\n",
                                  j, lp_transid->iv_node, pv_rmid, lp_transid->iv_seq_num));
                     lp_tx_info = new_txinfo(lp_transid);
                     lp_tx_info->tx_state(TM_TX_STATE_ABORTED);
                     lp_tx_info->safe_initialize_slot(pv_rmid);
                     lp_tx_info->inc_prepared_rms();
               }
            } //process XID
         } //for each XID returned
         break;
       }
      default:
       {
         sprintf(la_buf, "xa_recover wait for reply failed for RM %d with "
                 "error %d.\n", pv_rmid, lv_xa_result);
         tm_log_event(DTM_XATM_RECOVER_WAIT_FAILED, SQ_LOG_CRIT, "DTM_XATM_RECOVER_WAIT_FAILED",-1,
                         pv_rmid,-1,-1,-1,lv_xa_result);
         TMTrace(1, ("TM_Recov::send_xa_recover: %s",la_buf));
         lv_returnError = FEBADREPLY;
       }
      } //switch
   } //while more XIDs to be received

   delete [] lp_xids;

   // Release the mutex to make sure we only send one xa_recover to the TSEs at a time
   ip_tm_info->recovery_unlock();

   TMTrace (2, ("TM_Recov::send_xa_recover: EXIT returning error %d\n",
                    lv_returnError));
   return lv_returnError;
} //send_xa_recover


//----------------------------------------------------------------------------
// TM_Recov::send_xa_recover_toall_TSEs
// Purpose : send xa_recover messages to all TSEs.
// If pv_dead_dtm == -1, this is a standard system recovery at startup.
// If pv_dead_dtm == node_id then this recovery resulted from a TM process death.
// Note this function is only called in the Lead TM.
// Returns: 
//    FEOK if successful.
//    FEBADREPLY if the send fails or the reply contains an error.
//----------------------------------------------------------------------------
int32 TM_Recov::send_xa_recover_toall_TSEs(int32 pv_dead_dtm)
{
   char              la_buf[DTM_STRING_BUF_SIZE];
   XID              *lp_xids;
   TM_Txid_Internal *lp_transid;
   CTmRmTxRecoveryState *lp_txState;

   int64 lv_count;
   bool  lv_end = true;
   int   lv_index=0;
   int64 lv_maxCount;
   int   lv_xa_result;
   int32 lv_rmid;
   int   lv_rms_inRecovery = 0;
   int32 lv_returnError = FEOK;
   int32 lv_int_error = FEOK;
   int   lv_idx;

   // The main purpose of this struct is to track the flag values for
   // the RMs, but we store other information for debugging.
   struct 
   {
      int32 iv_rmid;
      int32 iv_count;
      uint64 iv_flags;
      int iv_outcome;
      bool iv_end;
      int32 iv_retry_count;
   } lv_RM[MAX_OPEN_RMS]; 
   
   TMTrace (2, ("TM_Recov::send_xa_recover_toall_TSEs: ENTRY, tm %d\n", pv_dead_dtm));

   // Grab the mutex to make sure we only send one xa_recover to the TSEs at a time
   ip_tm_info->recovery_lock();

   lv_maxCount = MIN(MAX_RECOVER_XIDS, ip_tm_info->max_trans());
   lp_xids = new XID[lv_maxCount];
   RM_Info_TSEBranch *lp_branches = ip_branches->TSE()->return_rms();

   // Send the initial xa_recover message to each RM.
   // We keep track of the number of outstanding xa_recover requests
   // using lv_rms_inRecovery
   for (int32 i = 0; i < MAX_OPEN_RMS; i++)
   {
      // Only send to RMs that are in use.
      if (lp_branches[i].in_use())
      {
         lv_RM[i].iv_rmid = lp_branches[i].rmid();
         
         if (iv_listBuilt == false)
             lv_RM[i].iv_flags = TMSTARTRSCAN;
         else
             lv_RM[i].iv_flags = TMRESENDRSCAN;
 
         lv_RM[i].iv_retry_count = 0;
         lv_RM[i].iv_count = 0;
         lv_RM[i].iv_outcome = XA_OK;
         lv_RM[i].iv_end = false;

         TMTrace (3, ("TM_Recov::send_xa_recover_toall_TSEs: sending xa_recover with rmid %d, "
                      " flags " PFLLX " and index %d.\n", lv_RM[i].iv_rmid, lv_RM[i].iv_flags,
                      lv_index));
         lv_xa_result = tm_xa_recover_send(lv_RM[i].iv_rmid, lv_maxCount, 
                                           lv_RM[i].iv_flags, lv_index, pv_dead_dtm, true);
         if (lv_xa_result == XA_OK)
            lv_rms_inRecovery++;
         else
         {
            sprintf(la_buf, "xa_recover RM %d returned XA error %d. Unable to "
                    "continue recovery.\n", lv_RM[i].iv_rmid, lv_xa_result);
            tm_log_event(DTM_XATM_RECOVER_SENDALL_FAILED, SQ_LOG_CRIT, "DTM_XATM_RECOVER_SENDALL_FAILED",-1,
                         lv_RM[i].iv_rmid,-1,-1,-1,lv_xa_result);
            TMTrace (1, ("TM_Recov::send_xa_recover_toall_TSEs: %s",la_buf));
            // Can't continue!
            delete [] lp_xids;
            ip_tm_info->recovery_unlock();
            return FEBADREPLY;
         }
      }
      else
          lv_RM[i].iv_rmid = -1; // Unused
   } //for

   TMTrace (3, ("TM_Recov::send_xa_recover_toall_TSEs: Participating RMs %d\n",
                   lv_rms_inRecovery));

   // While we have outstanding xa_recover requests, wait for and process 
   // the replies.
   while (lv_rms_inRecovery)
   {
      lv_idx = -1;
      lv_count = lv_maxCount;
      memset(lp_xids, 0, sizeof(XID)*lv_count);
      lv_xa_result = tm_xa_recover_waitReply(&lv_rmid, lp_xids, &lv_count, 
                                             &lv_end, &lv_index, rm_wait_time(), &lv_int_error);

      // XA errors are negative, warnings positive
      if (lv_xa_result >= XA_OK)
      {
         // Validate RM entry.  tm_xa_recover_waitReply always returns an rmid.
         lv_idx = gv_RMs.TSE()->return_slot_index(lv_rmid);
         if (lv_idx < 0 || lv_idx >= MAX_OPEN_RMS || lv_RM[lv_idx].iv_rmid == -1)
         {
            tm_log_event(DTM_TMRECOV_BAD_RMID, SQ_LOG_CRIT, "DTM_TMRECOV_BAD_RMID",
                  -1,lv_rmid,ip_tm_info->nid(),-1,-1,lv_xa_result,-1,-1,-1,-1,
                  -1,-1,-1,lv_idx,-1,-1,NULL,pv_dead_dtm);
            TMTrace(1,("TM_Recov::send_xa_recover_toall_TSEs: Bad rmid %d received "
                    "on xa_recover reply. gv_RMs.TSE()->return_slot_index() lookup resulted in slot %d, "
                    "XA error %d.\n", lv_rmid, lv_idx, lv_xa_result));
            abort();
         }
         if ((lv_xa_result == XA_RETRY) && (lv_int_error != FEOK))
         {
            if (lv_RM[lv_idx].iv_retry_count++ >= TM_LINKRETRY_RETRIES)
            {
                lv_xa_result = XAER_RMFAIL;
                TMTrace(1,("TM_Recov::send_xa_recover_toall_TSEs: Failing rmid %d "
                    "because of too many errors (%d).\n", lv_rmid, lv_xa_result));
            }
         }
      }

      switch (lv_xa_result)
      {
      case XA_OK:
         lv_RM[lv_idx].iv_flags = TMNOFLAGS;
         lv_RM[lv_idx].iv_count += lv_count;
         // Intentional drop through
      case XA_RETRY:
       {
         lv_RM[lv_idx].iv_outcome = lv_xa_result;
         if (lv_xa_result == XA_RETRY) {
             lv_count = 0;
         }

         // Decrement the counter if an RM has finished, otherwise send another xa_recover.
         if (lv_end == true)
         {
            lv_RM[lv_idx].iv_end = true;
            if (lv_xa_result == XA_OK)
               lv_rms_inRecovery--;
         }
         else
         {
            // If the last xa_recover completed without lv_end set, then there are more XIDs to get.
            // Send a new xa_recover request
            TMTrace (3, ("TM_Recov::send_xa_recover_toall_TSEs (2): sending xa_recover with rmid %d, "
                         " flags " PFLLX "and index %d.\n", lv_rmid, lv_RM[lv_idx].iv_flags, lv_index));
            lv_xa_result = tm_xa_recover_send(lv_rmid, lv_maxCount, 
                                              lv_RM[lv_idx].iv_flags, lv_index, pv_dead_dtm);
            if (lv_xa_result != XA_OK)
            {
               sprintf(la_buf, "xa_recover RM %d returned XA error %d. Unable to "
                     "continue recovery.\n", lv_rmid, lv_xa_result);
               tm_log_event(DTM_XATM_RECOVER_SENDALL2_FAILED, SQ_LOG_CRIT, "DTM_XATM_RECOVER_SENDALL2_FAILED",-1,
                         lv_rmid,-1,-1,-1,lv_xa_result);
               TMTrace (1, ("TM_Recov::send_xa_recover_toall_TSEs: %s",la_buf));
               // Can't continue!
               delete [] lp_xids;
               ip_tm_info->recovery_unlock();
               return FEBADREPLY;
            }
         } //else resend
         // Process returned XIDs from this reply:
         for (int32 j = 0; j < lv_count; j++)
         {
            lp_transid  = (TM_Txid_Internal *) &lp_xids[j].data;
            TMTrace (3, ("TM_Recov::send_xa_recover_toall_TSEs: RM %d Recovering XID entry, %d "
                         "txn ID (%d,%d).\n", lv_rmid, j, lp_transid->iv_node, 
                         lp_transid->iv_seq_num));
            lp_txState = get_txState(lp_transid);
            
            if (lp_txState != NULL)
            {
                TMTrace (3, ("TM_Recov::send_xa_recover_toall_TSEs: RM %d Txn State object found for XID entry, %d "
                    "Txn ID (%d,%d), #partic " PFLL ".\n",
                    lv_rmid, j, lp_transid->iv_node, lp_transid->iv_seq_num, (lp_txState->rmList()->size()+1)));
               // When we receive a request for the outcome of a transaction the TM has forgotten, we
               // need to try to get the right outcome back to the TSE.
               if (lp_txState->state() == TM_TX_STATE_FORGOTTEN ||
                   lp_txState->state() == TM_TX_STATE_FORGOTTEN_HEUR)
               {
                   if (lp_txState->abort_flags())
                       lp_txState->state(TM_TX_STATE_ABORTED);
                   else
                       lp_txState->state(TM_TX_STATE_COMMITTED);
               }
               lp_txState->add_partic(lv_rmid);
            }
            else
            {
               // Can't find the tx in the iv_txnStateList.  We must not have
               // generated a trans state record for the transaction yet when
               // the environment failed (i.e. no control point processing).
               // So, need to add the tx to the list and send xa_rollback to
               // the RM, to drive abort processing (presumed abort protocol).
               TMTrace (3, ("TM_Recov::send_xa_recover_toall_TSEs: RM %d Txn object not found for XID entry, %d "
                            "ID (%d,%d), sending xa_rollback.\n",
                            lv_rmid, j, lp_transid->iv_node, lp_transid->iv_seq_num));
               lp_txState = add_txState(lp_transid, TM_TX_STATE_ABORTED);
               lp_txState->add_partic(lv_rmid);
            }
         } //for each XID returned
         break;
      }
      case XAER_RMFAIL:
         // Mark the RM as down and continue 
         sprintf(la_buf, "xa_recover wait for reply failed for RM 0x%x. XAER_RMFAIL  "
                 "error %d.\n", lv_rmid, lv_xa_result);
         tm_log_event(DTM_XATM_RECOVER_SENDALL_WAIT_XAER_RMFAIL, SQ_LOG_WARNING, 
                      "DTM_XATM_RECOVER_SENDALL_WAIT_XAER_RMFAIL",-1,
                      lv_rmid,-1,-1,-1,lv_xa_result);
         TMTrace(1, ("TM_Recov::send_xa_recover_toall_TSEs: %s",la_buf));
         gv_RMs.TSE()->fail_rm(lv_rmid);
         lv_returnError = FEBADREPLY;
         lv_rms_inRecovery--;
         break;
      default:
       {
         sprintf(la_buf, "xa_recover wait for reply failed for RM %d with "
                 "error %d.\n", lv_rmid, lv_xa_result);
         tm_log_event(DTM_XATM_RECOVER_SENDALL_WAIT_FAILED, SQ_LOG_CRIT, 
                      "DTM_XATM_RECOVER_SENDALL_WAIT_FAILED",-1,
                      lv_rmid,-1,-1,-1,lv_xa_result);
         TMTrace(1, ("TM_Recov::send_xa_recover_toall_TSEs: %s",la_buf));
         lv_returnError = FEBADREPLY;
         lv_rms_inRecovery--;
       }
      } //switch
   } //while there are rms still to reply

   delete [] lp_xids;

   //Finished with the xa_recover so we can release the lock
   ip_tm_info->recovery_unlock();

   TMTrace (2, ("TM_Recov::send_xa_recover_toall_TSEs: EXIT returning error %d\n",
                    lv_returnError));
   return lv_returnError;
} //send_xa_recover_toall_TSEs


//----------------------------------------------------------------------------
// TM_Recov::resolve_in_doubt_txs
// Purpose : Resolve all transactions in the indoubt txnList for the specified
// dtm process.
// If an error occurs while sending xa_commit or xa_rollback requests to the 
// RMs, we mark the transaction object as needing to retry by changing it's 
// tx_state to HUNGCOMMITTED or HUNGABORTED but don't return an error.  This
// way we can process the entire txnList even if we can't contact all RMs.
//----------------------------------------------------------------------------
int32 TM_Recov::resolve_in_doubt_txs(int32 pv_dtm, int_32 delay)
{
   TM_TX_Info * lp_txn = NULL;
   int64        lv_last_key = 0;

   // The total number of transactions to recover for this node has already been set when we created the Transaction State List
   //iv_total_txs_to_recover = ip_tm_info->num_active_txs() + txnStateList()->size();
   //iv_total_txs_to_recover = ip_tm_info->transactionPool()->get_inUseList()->size() + txnStateList()->size();

   TMTrace(2, ("TM_Recov::resolve_in_doubt_txs ENTRY - %d txns to be recovered. "
               "Queued: " PFLL ", in progress: " PFLL ".\n",
               iv_total_txs_to_recover, txnStateList()->size(), 
               ip_tm_info->transactionPool()->get_inUseList()->size()));
   tm_log_event(DTM_RECOV_INDOUBT_TXNS, SQ_LOG_INFO, "DTM_RECOV_INDOUBT_TXNS", 
                -1,-1,pv_dtm,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,iv_total_txs_to_recover);
        
    lp_txn = (TM_TX_Info *) txnList()->get_first();

    while (lp_txn)
    {
        resolveTxn(lp_txn);

        lv_last_key = txnList()->curr_key();
        lp_txn = (TM_TX_Info *) txnList()->get_next();
        remove_txinfo(lv_last_key);
    } // while more txns for node

    txnList()->get_end();

    //We always start a TMRecoveryWait timer, delay is how long to wait
    //for Cluster recovery (nid = -1) it will reschedule waiting for all transactions to finish
    //for Node recovery it will just make sure any pending TransactionState objects get processed 
    ip_tm_info->addTMRecoveryWait(pv_dtm, delay);
   
    // Update the registry value
    updateTxnsToRecover();
    TMTrace(2, ("TM_Recov::resolve_in_doubt_txs EXIT - %d txns to be recovered. "
               "Queued: " PFLL ", in progress: " PFLL ".\n",
               iv_total_txs_to_recover, txnStateList()->size(), 
               ip_tm_info->transactionPool()->get_inUseList()->size()));
    return 0;
} //TM_Recov::resolve_in_doubt_txs


//----------------------------------------------------------------------------
// TM_Recov::updateTxnsToRecover
// Purpose : Returns the number of indoubt transactions left to recover and 
// updates the registry value periodically if it has changed.
// Note that the total number of transactions to recover is the number of 
// active transactions + number of txState entries.  This only works for system recovery
// Returns indoubt transaction count
//----------------------------------------------------------------------------
int32 TM_Recov::updateTxnsToRecover()
{
   static int lv_counter = 0;
   static int32 lv_lastIndoubtTxns = 0;
   static int32 lv_maxIndoubtTxns = 0;
   int32 lv_activeIndoubtTxns = ip_tm_info->transactionPool()->get_inUseList()->size() +
                                iv_txnStateList.size();
   TMTrace (2, ("TM_Recov::updateTxnsToRecover ENTRY There are %d txns in progress now, "
            "previous call %d txns.\n",
            lv_activeIndoubtTxns, lv_lastIndoubtTxns));

   if(lv_activeIndoubtTxns > 0) 
   {
      lv_counter++;
      if (lv_activeIndoubtTxns > lv_maxIndoubtTxns)
         lv_maxIndoubtTxns = lv_activeIndoubtTxns;

      if ((lv_lastIndoubtTxns != lv_activeIndoubtTxns) || ((lv_counter % 10) == 0)) 
      {
         update_registry_txs_to_recover(lv_activeIndoubtTxns);
         tm_log_event(DTM_RECOVERY_TXNS_TO_RECOVER, SQ_LOG_NOTICE, "DTM_RECOVERY_TXNS_TO_RECOVER",
                      -1,-1,ip_tm_info->nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,lv_activeIndoubtTxns);
      }
      lv_lastIndoubtTxns = lv_activeIndoubtTxns;
   }
   else
   {
      tm_log_event(DTM_RECOVERY_TXNS_TO_RECOVER, SQ_LOG_NOTICE, "DTM_RECOVERY_TXNS_TO_RECOVER",
                   -1,-1,ip_tm_info->nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,lv_maxIndoubtTxns);
      lv_activeIndoubtTxns = 0;
   }

   TMTrace (2, ("TM_Recov::updateTxnsToRecover EXIT %d indoubt txns left to resolve out of %d.\n",
            lv_activeIndoubtTxns, lv_maxIndoubtTxns));
   return lv_activeIndoubtTxns;
} //TM_Recov::updateTxnsToRecover


//----------------------------------------------------------------------------
// TM_Recov::completeRecovery
// Purpose : Complete recovery by:
// 1. set registry value to 0
// 2. write 2 control points
// 3. tell all non-Lead TMs that recovery is complete
//----------------------------------------------------------------------------
void TM_Recov::completeRecovery()
{
   TMTrace (2, ("TM_Recov::completeRecovery EXIT\n"));
   update_registry_txs_to_recover(0);
   ip_tm_info->write_control_point(true);
   ip_tm_info->write_control_point(true, true);
   ip_tm_info->set_sys_recov_status(TM_SYS_RECOV_STATE_END, ip_tm_info->nid());
   //send_sys_recov_end_sync(ip_tm_info->nid());

   TMTrace (2, ("TM_Recov::completeRecovery EXIT\n"));
} //TM_Recov::completeRecovery


//----------------------------------------------------------------------------
// TM_Recov::update_registry_txs_to_recover
// Purpose : Sets the registry value DTM_RECOVERING_TX_COUNT to the number
// of transactions still to be recovered specified by pv_total.  This is 
// updated each time it changes and is used to report the number of 
// transactions being recovered at startup time.
//----------------------------------------------------------------------------
void TM_Recov::update_registry_txs_to_recover(int32 pv_total)
{
    char la_txns[9];
    int32 lv_error = FEOK;

    sprintf(la_txns, "%d", pv_total);
    lv_error = tm_reg_set(MS_Mon_ConfigType_Cluster, (char *) CLUSTER_GROUP, 
                          (char *) DTM_RECOVERING_TX_COUNT, la_txns);

    if (lv_error != FEOK)
    {
        tm_log_event(DTM_TM_REGISTRY_SET_ERROR, SQ_LOG_CRIT, "DTM_TM_REGISTRY_SET_ERROR", 
            lv_error);
        TMTrace( 1, ("TM_Recov::update_registry_txs_to_recover : Failed to write "
            "value into the registry. DTM_RECOVERING_TX_COUNT: Txns to recover %d  Error %d\n", 
            pv_total, lv_error));
        abort(); 
    }

    TMTrace(1, ("TM_Recov::update_registry_txs_to_recover : New value %d.\n",
            pv_total));
} //TM_Recov::update_registry_txs_to_recover

   
//----------------------------------------------------------------------------
// TM_Recov::remove_forgotten_txs
// Remove forgotten transactions with no associated RM from the iv_txnStateList.
// This will be performed after we reconcile the list of indoubt transactions
// against the subordinate RMs to ensure we inform any RMs which still have
// an indoubt entry for a forgotten transaction.
// This used to occur after the backwards scan and before xa_recover phases
// If a TSE reports a transaction in the xa_recover reply which we have listed
// as forgotten, deleting these before xa_recover results in the TM driving a
// rollback for a forgotten transaction (presumed abort protocol), one which 
// has previously been committed!
//----------------------------------------------------------------------------
void TM_Recov::remove_forgotten_txs()
{
    TMTrace(2, ("TM_Recov::remove_forgotten_txs : ENTRY\n"));
    int32 lv_forgottenTxns_removed = 0;
    int32 lv_forgottenTxns_kept = 0;
    int32 lv_indoubtTxns = 0;
    int64 lv_id;
    bool lv_remove = false;
    CTmTxKey lv_key(0);
    CTmRmTxRecoveryState *lp_txState = (CTmRmTxRecoveryState *) iv_txnStateList.get_first();
    
    while (lp_txState != NULL)
    {
        lv_remove = false;
        lv_id = iv_txnStateList.curr_key();
        lv_key.set(lv_id);
        lv_indoubtTxns++;
        if (lp_txState->state() == TM_TX_STATE_FORGOTTEN &&
            lp_txState->rms_to_recover() == 0)
        {
            lv_remove = true;
            lv_forgottenTxns_removed++;
        }
        else
            if (lp_txState->state() == TM_TX_STATE_FORGOTTEN)
                lv_forgottenTxns_kept++;

        lp_txState = (CTmRmTxRecoveryState *) iv_txnStateList.get_next();
        if (lv_remove)
        {
            remove_txState(lv_id);
            iv_total_txs_to_recover--;
            TMTrace(3, ("TM_Recov::remove_forgotten_txs : removed txn ID (%d,%d), txns "
                    "to recover %d.\n", lv_key.node(), lv_key.seqnum(), iv_total_txs_to_recover));
        }
    }
    iv_txnStateList.get_end();

    TMTrace(2, ("TM_Recov::remove_forgotten_txs : EXIT  Removed %d and kept %d "
                "forgotten of %d indoubt txns.\n", 
                lv_forgottenTxns_removed, lv_forgottenTxns_kept, lv_indoubtTxns));
} //remove_forgotten_txs


//----------------------------------------------------------------------------
// TM_Recov::queueTxnObjects
// Create the TM_TX_Info objects to represent the indoubt transactions still on
// the iv_txnStateList.  They are then removed from this list and added to the 
// iv_txnList.
// Duplicate entries are not allowed and when we reach the maximum size of the
// transactionPool, we exit.
// pv_ignoreDupTxns defaults to false.
//----------------------------------------------------------------------------
void TM_Recov::queueTxnObjects(bool pv_ignoreDupTxns)
{
    TM_TX_Info * lp_txn = NULL;
    int32 lv_moved = 0;
    RM_Info_TSEBranch * lp_rm = NULL;
    int64 lv_id = 0;
    bool lv_maxRecoveringTxns_hit = false,
         lv_duplicate = false;

    TMTrace(2, ("TM_Recov::queueTxnObjects : ENTRY  " PFLL " active txns and "
                PFLL " indoubt txns.\n", 
                ip_tm_info->transactionPool()->get_inUseList()->size(),
                iv_txnStateList.size()));
    CTmRmTxRecoveryState *lp_txState = (CTmRmTxRecoveryState *) iv_txnStateList.get_first();

    // While we have free transaction objects and there are still entries on the txStateList,
    // instantiate new txn objects.
    while (!lv_maxRecoveringTxns_hit && lp_txState != NULL)
    {
        lv_id = iv_txnStateList.curr_key();
        // Check that the transaction doesn't already exist.  This can happen when we are
        // retrying recovery for a node failure.
        lp_txn = (TM_TX_Info *) ip_tm_info->transactionPool()->get(lv_id);
        if (!lp_txn)
        {
           if ((txnList()->size() >= ip_tm_info->maxRecoveringTxns()) ||
               (ip_tm_info->transactionPool()->get_inUseList()->size() 
                            >= ip_tm_info->transactionPool()->get_maxPoolSize()))
           {
               lv_maxRecoveringTxns_hit = true;
               TMTrace(3, ("TM_Recov::queueTxnObjects : At maximum recovering transactions. "
                       "txnList size=" PFLL  ", active transactions=" PFLL ".\n", 
                       txnList()->size(), ip_tm_info->transactionPool()->get_inUseList()->size()));
           }
           else
           {
               lp_txn = new_txinfo(lp_txState->transid());
               lp_txn->tx_state(lp_txState->state());
               lp_txn->abort_flags(lp_txState->abort_flags());
               //TODO Workaround for CR ???? - Not all RMs participating in the transaction
               // are marked as participating in the txn object and we end up with ophan branches
               // in the TSEs which are never resolved. This only happens for DTM death recoveries. 
               // This workaround forces all RMs to participate in all txns when pv_ignoreDupTxns
               // is true (only true for recovery from DTM failure) and the flag iv_AllRmParticRecov  
               // value (comes from registry or env variable DTM_ALL_RM_PARTIC_RECOV variable).  The
               // current default is still TRUE since we are verifying it had been fixed first

               // I've also added
               // additional information to trace records to help us identify where we're loosing
               // the participants (can only be tested by changing this code back).
               //lp_txn->initialize_tx_rms(false /*no rms participate unless they responded*/);

               if ( pv_ignoreDupTxns && gv_tm_info.AllRmParticRecov() )
               {
                   lp_txn->initialize_tx_rms(true );
               }
               else
               {
                   lp_txn->initialize_tx_rms(false);
               }

               lp_txn->recovering(true);
               lp_rm = (RM_Info_TSEBranch *) lp_txState->rmList()->get_first();
               //TODO one day: We could optimize here by writing a forgotten trans state record
               // when there are no participating RMs and not instantiating a txn object.
               while (lp_rm)
               {
                   lp_rm = lp_txn->get_rm(lp_rm->rmid());
                   lp_rm->partic(true);
                   lp_txn->inc_prepared_rms();

                   lp_rm = (RM_Info_TSEBranch *) lp_txState->rmList()->get_next();
               }
               lp_txState->rmList()->get_end();
           }
        }
        else
        {
           if (!pv_ignoreDupTxns)
           {
               TMTrace(1, ("TM_Recov::queueTxnObjects : Attempted to instantiate a duplicate "
                   "transaction object for (%d,%d).\n", lp_txn->node(), lp_txn->seqnum()));
               tm_log_event(DTM_RECOV_DUP_TXN, SQ_LOG_CRIT, "DTM_RECOV_DUP_TXN", -1, -1, 
                   lp_txn->node(), lp_txn->seqnum());
               abort();
           }
           lv_duplicate = true;
        }
        if (!lv_maxRecoveringTxns_hit)
        {
           // Duplicates allowed, ignore and get next element from list.
           if (!lv_duplicate)
              lv_moved++;
           lp_txState = (CTmRmTxRecoveryState *) iv_txnStateList.get_next();
           remove_txState(lv_id);
        }
    } //while 
    iv_txnStateList.get_end();

    // Update registry value now to reflect new total.
    // update_registry_txs_to_recover(iv_total_txs_to_recover);

    TMTrace(2, ("TM_Recov::queueTxnObjects : EXIT  %d indoubt txns "
            "instantiated from txnStateList. Active Txns " PFLL ", queued "
            "txnState objs " PFLL ".\n", lv_moved, 
            ip_tm_info->transactionPool()->get_inUseList()->size(), 
            iv_txnStateList.size()));
} //queueTxnObjects


//----------------------------------------------------------------------------
// TM_Recov::resolveTxn
// Purpose : Resolve a single transaction.
// pp_txn: Input  - Transaction object pointer for transaction to be resolved.
// Returns error.
//----------------------------------------------------------------------------
int32 TM_Recov::resolveTxn(TM_TX_Info * pp_txn)
{
   int32        lv_error = 0;

   TMTrace(2, ("TM_Recov::resolveTxn ENTRY - Recovering indoubt txn (%d,%d), "
               "with %d prepared RMs.\n",
               pp_txn->node(), pp_txn->seqnum(), pp_txn->prepared_rms()));
   tm_log_event(DTM_RECOV_RESOLVING_TXN, SQ_LOG_INFO, "DTM_RECOV_RESOLVING_TXN", 
                -1,-1,pp_txn->node(), pp_txn->seqnum(),-1,-1,-1,-1,-1,-1,-1,-1,
                -1,pp_txn->prepared_rms(),-1,-1,NULL,ip_tm_info->nid());

   if (pp_txn->in_use())
   {
      switch (pp_txn->tx_state())
      {
      case TM_TX_STATE_HUNGCOMMITTED:
        // Hung transactions are stuck in phase 2 and already committed, 
        // we have only to inform the RMs of the outcome.
        // Note that this can only be the case for existing TM_TX_Info
        // objects in a TM, and not for new ones created by scan trails.
        // scantrail_bldgtxlist always sets hung committed transactions
        // to committed state.
        pp_txn->tx_state(TM_TX_STATE_COMMITTED);
        // Intentional drop-through.
      case TM_TX_STATE_FORGOTTEN:
      case TM_TX_STATE_COMMITTED:
      {
        if (pp_txn->prepared_rms() <= 0)
        {
           // No RMs have included this xid in their prepared tx list.
           // They must have all committed this tx.  So, just write a
           // 'forgotten' audit record.
           pp_txn->tx_state(TM_TX_STATE_FORGETTING);
        }
        lv_error = redrive_recov_tx(RECOV_REDRIVE_COMMIT, pp_txn);
        break;
      } // TM_TX_STATE_COMMITTED
      case TM_TX_STATE_HUNGABORTED:
        // Hung transactions are stuck in phase 2 and already rolledback, 
        // we have only to inform the RMs of the outcome.
        pp_txn->tx_state(TM_TX_STATE_ABORTING_PART2);
        // Intentional drop-through.
      case TM_TX_STATE_ABORTING:
      case TM_TX_STATE_ABORTING_PART2:
      case TM_TX_STATE_ABORTED:
      {
        if (pp_txn->prepared_rms() <= 0)
        {
            // No RMs have included this xid in their in-doubt tx list.
            // The involved RMs must have ulilaterally aborted this tx.
            pp_txn->tx_state(TM_TX_STATE_ABORTED);
        }
        // Some RMs may have included this xid in their prepared tx list.
        // Looks like some work was done by this transaction but we
        // had not yet generated any trans state records for it in
        // the TLOG (i.e. the crash happened after the transaction
        // began and before a control point was generated.
        // Drive a rollback to the RMs.
        lv_error = redrive_recov_tx(RECOV_REDRIVE_ROLLBACK, pp_txn);
        break;
      } // TM_TX_STATE_ABORTING, TM_TX_STATE_ABORTED
      case TM_TX_STATE_ACTIVE:
      {
        if (pp_txn->prepared_rms() <= 0)
        {
           // All involved RMs must have unilaterally aborted the tx.  
           lv_error = redrive_recov_tx(RECOV_REDRIVE_ROLLBACK, pp_txn);
        }
        else
        {
           if (pp_txn->prepared_rms() == MAX_OPEN_RMS)
           {
           // All RMs have prepared this tx, we can proceed with commit.
           // MAX_OPEN_RMS is used to be conservative.
           lv_error = redrive_recov_tx(RECOV_REDRIVE_COMMIT, pp_txn);
           }
           else
           {
           // Only some RMs have prepared this tx.  Other RMs might have ulilaterally
           // aborted this tx.  Proceed with rollback.
           lv_error = redrive_recov_tx(RECOV_REDRIVE_ROLLBACK, pp_txn);
           }
        } // else
        break;
      } // TM_TX_STATE_ACTIVE
      default:
        break;
      } // switch
   } //if in_use

   TMTrace(2, ("TM_Recov::resolveTxn EXIT, error %d.\n", lv_error));
   return 0;
} //TM_Recov::resolveTxn

//----------------------------------------------------------------------------
// TM_Recov::listBuilt
// Purpose : Set the iv_listBuilt flag
// pv_listBuilt: Input  - value for list.
// Returns void.
//----------------------------------------------------------------------------
void TM_Recov::listBuilt(bool pv_listBuilt)
{
    TMTrace(2, ("TM_Recov::listBuilt ENTRY, with list value %d.\n", pv_listBuilt));
    iv_listBuilt = pv_listBuilt;
    TMTrace(2, ("TM_Recov::listBuilt EXIT\n"));
}
