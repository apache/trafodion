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
#include <time.h>
#include <sys/time.h>

// General Seaquest includes
//#include "SCMVersHelp.h"

// seabed includes
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/trace.h"
#include "seabed/thread.h"
#include "common/sq_common.h"

// tm includes
#include "rm.h"
#include "tmaudit.h"
// #include "tmmap.h"
#include "tmregistry.h"
#include "tmlogging.h"
#include "tmrecov.h"
#include "tmshutdown.h"
#include "tmpool.h"
#include "tmstats.h"

#include "tmglob.h"
#include "tmtimer.h"
#include "tmthreadeg.h"

#include "tminfo.h"
#include "tmrecov.h"
#include "tmxarmmain.h"
#include "tmxidmap.h"

extern char * XIDtoa(XID *pp_xid);

void tm_xarm_initialize()
{
   gp_xarmRmidList = new TM_MAP();
   gp_xarmXIDList = new XID_MAP();
}

//----------------------------------------------------------------
// tm_process_msg_from_xarm
// ---------------------------------------------------------------
void tm_process_msg_from_xarm(CTmTxMessage * pp_msg)
{
   TMTrace(2, ("tm_process_msg_from_xarm ENTRY, request %d. \n",
      pp_msg->request()->iv_msg_hdr.rr_type.request_type));

   switch (pp_msg->request()->iv_msg_hdr.rr_type.request_type)
   {
   case TM_DP2_SQ_XA_START:
      tm_process_xa_start(pp_msg);
      break;
   case TM_DP2_SQ_XA_END:
      tm_process_xa_end(pp_msg);
      break;
   case TM_DP2_SQ_XA_COMMIT:
      tm_process_xa_commit(pp_msg);
      break;
   case TM_DP2_SQ_XA_PREPARE:
      tm_process_xa_prepare(pp_msg);
      break;
   case TM_DP2_SQ_XA_ROLLBACK:
      tm_process_xa_rollback(pp_msg);
      break;
   case TM_DP2_SQ_XA_OPEN:
      tm_process_xa_open(pp_msg);
      break;
   case TM_DP2_SQ_XA_CLOSE:
      tm_process_xa_close(pp_msg);
      break;
   case TM_DP2_SQ_XA_RECOVER:
      tm_process_xa_recover(pp_msg);
      break;
   case TM_DP2_SQ_XA_FORGET:
      tm_process_xa_forget(pp_msg);
      break;
   case TM_DP2_SQ_XA_COMPLETE:
      tm_process_xa_complete(pp_msg);
      break;
   case TM_DP2_SQ_AX_REG:
      tm_process_ax_reg(pp_msg);
      break;
   case TM_DP2_SQ_AX_UNREG:
      tm_process_ax_unreg(pp_msg);
      break;
   default:
      TMTrace(1, ("tm_process_msg_from_xarm ERROR, bad request code %d.\n", 
         pp_msg->request()->iv_msg_hdr.rr_type.request_type));
      // Reply with error since unknown request type
      pp_msg->reply(XAER_RMERR);                
      abort();
   }

   TMTrace(2, ("tm_process_msg_from_xarm EXIT. \n"));
} // tm_process_msg_from_xarm


// Lookup xarm txn branch in 
CTmXaTxn *tm_lookup_xaTxn(int pv_rmid, XID *pp_xid)
{
   TMTrace(2, ("tm_lookup_xaTxn ENTRY: rmid %d, XID %s.\n", pv_rmid, XIDtoa(pp_xid)));
   CTmXaRm *lp_rm = tm_lookup_xaRM(pv_rmid);
   if (lp_rm == NULL)
      abort();

   CTmXaTxn *lp_xaTxn = lp_rm->getTxn(pp_xid);
   
   if (lp_xaTxn != NULL)
   {
      TMTrace(2, ("tm_lookup_xaTxn EXIT: returned Txn ID (%d,%d).\n", 
              lp_xaTxn->node(), lp_xaTxn->seqnum()));
   }
   else
   {
      TMTrace(2, ("tm_lookup_xaTxn EXIT: Txn not found for XID %s.\n", 
              XIDtoa(pp_xid)));
   }
   return lp_xaTxn;
} //tm_lookup_xaTxn


// Instantiate a new XARM txn.
CTmXaTxn *tm_new_xaTxn(int pv_rmid, XID *pp_xid, int pv_nid, int pv_pid)
{
   CTmXaTxn * lp_xaTxn = NULL;
   TMTrace(2, ("tm_new_xaTxn ENTRY: rmid %d, XID %s from (%d,%d).\n", 
      pv_rmid, XIDtoa(pp_xid), pv_nid, pv_pid));

   // Instantiate a new tx object.
   lp_xaTxn = (CTmXaTxn *) gv_tm_info.new_tx(pv_nid, pv_pid,
                                             -1, -1, // node,seqnum assigned by function
                                             (void* (*)(long int)) &CTmXaTxn::constructPoolElement);
   if (lp_xaTxn) 
   {
      TMTrace(2, ("tm_new_xaTxn EXIT: Txn ID (%d,%d), Txn obj %p used.\n", 
         lp_xaTxn->node(), lp_xaTxn->seqnum(), lp_xaTxn));

      // Add into XID list
      gp_xarmXIDList->put(pp_xid, lp_xaTxn);
   }
   else 
   {
      TMTrace(2, ("tm_new_xaTxn EXIT: Could not instantiate new txn object.\n"));
   }
   return lp_xaTxn;
} // tm_new_xaTxn


// Lookup an xarm superior RM
CTmXaRm *tm_lookup_xaRM(int pv_rmid)
{
   CTmXaRm *lp_rm = (CTmXaRm *) gp_xarmRmidList->get(pv_rmid);
   return lp_rm;
} // tm_lookup_xaRM


// Add an XARM superior RM
bool add_xaRM(CTmXaRm *pp_xarm)
{
   int lv_rmid = pp_xarm->rmid();
   gp_xarmRmidList->put(lv_rmid, pp_xarm);
   return true; //prototype TODO
} // add_xaRM


void delete_xaRM(CTmXaRm *pp_xarm)
{
   int lv_rmid = pp_xarm->rmid();
   gp_xarmRmidList->remove(lv_rmid);
   delete [] pp_xarm;
} // delete_xaRM



// XARM message processing functions
void tm_process_xa_start(CTmTxMessage * pp_msg)
{
   int32 lv_error = XA_OK;
   TMTrace(2, ("tm_process_xa_start ENTRY: Received from (%d,%d), rm %d, xid %s.\n", 
      pp_msg->request()->u.iv_start.iv_nid,
      pp_msg->request()->u.iv_start.iv_pid,
      pp_msg->request()->u.iv_start.iv_rmid, 
      XIDtoa(&pp_msg->request()->u.iv_start.iv_xid)));
 
   // Check that the TM is in a state that allows it to start new transactions
   if ((gv_tm_info.state() != TM_STATE_UP) ||
       (gv_tm_info.sys_recov_state() != TM_SYS_RECOV_STATE_END))
   {
        switch (gv_tm_info.state())
        {
        case TM_STATE_TX_DISABLED:
        case TM_STATE_DRAIN:
            lv_error = XAER_RMERR;
            break;
        case TM_STATE_QUIESCE:
            lv_error = XAER_RMERR;
            break;
        default:
            lv_error = XAER_RMERR;
        }
        TMTrace(1, ("tm_process_xa_start returning error %d, TM state %d.\n", 
           lv_error, gv_tm_info.state()));
        pp_msg->reply(lv_error);
        delete pp_msg;
        return;
    }

    // First check to see if a txn branch already exists
    CTmXaTxn *lp_tx = tm_lookup_xaTxn(pp_msg->request()->u.iv_start.iv_rmid,
                                     &pp_msg->request()->u.iv_start.iv_xid);
    
    switch (pp_msg->request()->u.iv_start.iv_flags)
    {
    case TM_JOIN:
       if (!lp_tx)
         pp_msg->reply(XAER_NOTA);
       else
         lp_tx->req_xa_join(pp_msg); //join replies
       break;
    case TM_RESUME:
       if (!lp_tx)
         pp_msg->reply(XAER_NOTA);
       else
         lp_tx->req_xa_resume(pp_msg); //resume replies
       break;
    case TMASYNC:
    case TMNOWAIT:
        TMTrace(1, ("tm_process_xa_start unsupported flag " PFLL ".\n", 
           pp_msg->request()->u.iv_start.iv_flags));
        pp_msg->reply(XAER_INVAL);
        break;
    default:
       // Instantiate a new CTmXaTxn to represent the XARM subordinate branch object.
       if (lp_tx)
       {

          pp_msg->reply(XAER_DUPID);
          break;
       }
       else
          lp_tx = tm_new_xaTxn(pp_msg->request()->u.iv_start.iv_rmid, 
                               &pp_msg->request()->u.iv_start.iv_xid,
                               pp_msg->request()->u.iv_start.iv_nid,
                               pp_msg->request()->u.iv_start.iv_pid);

       // An error indicates we are handling our maximum number of concurrent
       // transactions.
       if (lp_tx == NULL)
       {
          TMTrace(1, ("tm_process_xa_start, FETOOMANYTRANSBEGINS\n"));
          pp_msg->reply(XAER_RMERR);
       }
       else
       {
          TMTrace(1, ("tm_process_xa_start, Beginning new XA Txn ID (%d,%d).\n",
                  lp_tx->node(), lp_tx->seqnum()));
          lp_tx->req_xa_start(pp_msg);
       }

       lp_tx->lock();

       // Start statistics counters
       //lp_tx->stats()->txnTotal()->start();
       //lp_tx->stats()->txnBegin()->start();


       // Do we need to enlist TSEs for xa_start, or can we wait for the ax_reg
       // requests to arrive?  Assuming ax_reg for now.
       pp_msg->reply(XA_OK);
       lp_tx->unlock();
    } //switch flags

    TMTrace(2, ("tm_process_xa_start, ID (%d,%d), creator (%d,%d) EXIT\n", 
            lp_tx->node(), lp_tx->seqnum(), lp_tx->ender_nid(), lp_tx->ender_pid()));
   
   delete pp_msg;
   TMTrace(2, ("tm_process_xa_start EXIT.\n"));
} // tm_process_xa_start


// --------------------------------------------------------------
// tm_process_xa_prepare
// Purpose - Txn  specific processing for xa_prepare.
// xa_prepare initiates phase 1.
// --------------------------------------------------------------
void tm_process_xa_prepare(CTmTxMessage * pp_msg)
{
   CTmXaTxn *lp_tx = tm_lookup_xaTxn(pp_msg->request()->u.iv_prepare.iv_rmid,
                                     &pp_msg->request()->u.iv_prepare.iv_xid);
   if (lp_tx) {
      TMTrace(2, ("tm_process_xa_prepare ENTRY: Received from (%d,%d) found Txn ID (%d,%d) for rm %d, xid %s.\n",
         pp_msg->request()->u.iv_prepare.iv_nid,
         pp_msg->request()->u.iv_prepare.iv_pid,
         lp_tx->node(), lp_tx->seqnum(),
         pp_msg->request()->u.iv_prepare.iv_rmid, 
         XIDtoa(&pp_msg->request()->u.iv_prepare.iv_xid)));
      // Ignoring flags for now (prototype)
      TM_Txid_Internal * lp_transid = (TM_Txid_Internal *) lp_tx->transid();
      lp_tx->queueToTransaction(lp_transid, pp_msg);
   }
   else
   {
      TMTrace(1, ("tm_process_xa_prepare ENTRY Txn NOT FOUND for rm %d, xid %s.\n", 
         pp_msg->request()->u.iv_prepare.iv_rmid, 
         XIDtoa(&pp_msg->request()->u.iv_prepare.iv_xid)));
      pp_msg->reply(XAER_NOTA);
   }

   TMTrace(2, ("tm_process_xa_prepare EXIT.\n"));
} // tm_process_xa_prepare


// --------------------------------------------------------------
// tm_process_xa_end
// Purpose - Txn  specific processing for xa_end.
// xa_end dissociates the client/thread from the 
// XA transaction in XA terms.
// This is called directly in the main thread
// as no work is done here.
// --------------------------------------------------------------
void tm_process_xa_end(CTmTxMessage * pp_msg)
{
   CTmXaTxn *lp_tx = tm_lookup_xaTxn(pp_msg->request()->u.iv_end.iv_rmid,
                                     &pp_msg->request()->u.iv_end.iv_xid);
   if (lp_tx)
   {
      TMTrace(2, ("tm_process_xa_end ENTRY: Received from (%d,%d) found XA Txn ID (%d,%d) for rm %d, xid %s.\n",
         pp_msg->request()->u.iv_end.iv_nid,
         pp_msg->request()->u.iv_end.iv_pid,
         lp_tx->node(), lp_tx->seqnum(),
         pp_msg->request()->u.iv_end.iv_rmid, 
         XIDtoa(&pp_msg->request()->u.iv_end.iv_xid)));
      lp_tx->req_xa_end(pp_msg);
   }
   else
   {
      TMTrace(1, ("tm_process_xa_end ENTRY XA Txn NOT FOUND for rm %d, xid %s.\n", 
         pp_msg->request()->u.iv_end.iv_rmid, 
         XIDtoa(&pp_msg->request()->u.iv_end.iv_xid)));
      pp_msg->reply(XAER_NOTA);
   }

   TMTrace(2, ("tm_process_xa_end EXIT.\n"));
} // tm_process_xa_end


// --------------------------------------------------------------
// tm_process_xa_commit
// Purpose - Txn  specific processing for xa_commit.
// xa_commit processes a transaction to phase 2.
// --------------------------------------------------------------
void tm_process_xa_commit(CTmTxMessage * pp_msg)
{
   CTmXaTxn *lp_tx = tm_lookup_xaTxn(pp_msg->request()->u.iv_commit.iv_rmid,
                                     &pp_msg->request()->u.iv_commit.iv_xid);
   if (lp_tx) 
   {
      TMTrace(2, ("tm_process_xa_commit ENTRY: Received from (%d,%d) found txn ID (%d,%d) rm %d, xid %s.\n", 
         pp_msg->request()->u.iv_commit.iv_nid,
         pp_msg->request()->u.iv_commit.iv_pid,
         lp_tx->node(), lp_tx->seqnum(),
         pp_msg->request()->u.iv_commit.iv_rmid, 
         XIDtoa(&pp_msg->request()->u.iv_commit.iv_xid)));
      // Ignoring flags for now (prototype)
      // check TM_ONEPHASE
      TM_Txid_Internal * lp_transid = (TM_Txid_Internal *) lp_tx->transid();
      lp_tx->queueToTransaction(lp_transid, pp_msg);
   }
   else
   {
      TMTrace(1, ("tm_process_xa_commit ENTRY Txn NOT FOUND for rm %d, xid %s.\n", 
         pp_msg->request()->u.iv_commit.iv_rmid, 
         XIDtoa(&pp_msg->request()->u.iv_commit.iv_xid)));
      pp_msg->reply(XAER_NOTA);
   }

   TMTrace(2, ("tm_process_xa_commit EXIT.\n"));
} // tm_process_xa_commit


// --------------------------------------------------------------
// tm_process_xa_forget
// Purpose - Txn  specific processing for xa_forget.
// xa_forget forgets a heuristically completed txn.
// --------------------------------------------------------------
void tm_process_xa_forget(CTmTxMessage * pp_msg)
{
   CTmXaTxn *lp_tx = tm_lookup_xaTxn(pp_msg->request()->u.iv_commit.iv_rmid,
                                     &pp_msg->request()->u.iv_commit.iv_xid);
   if (lp_tx) 
   {
      TMTrace(2, ("tm_process_xa_forget ENTRY: Received from (%d,%d) found txn ID (%d,%d) rm %d, xid %s.\n", 
         pp_msg->request()->u.iv_forget.iv_nid,
         pp_msg->request()->u.iv_forget.iv_pid,
         lp_tx->node(), lp_tx->seqnum(),
         pp_msg->request()->u.iv_forget.iv_rmid, 
         XIDtoa(&pp_msg->request()->u.iv_forget.iv_xid)));
      // Ignoring flags for now (prototype)
      TM_Txid_Internal * lp_transid = (TM_Txid_Internal *) lp_tx->transid();
      lp_tx->queueToTransaction(lp_transid, pp_msg);
   }
   else
   {
      TMTrace(1, ("tm_process_xa_forget ENTRY Txn NOT FOUND for rm %d, xid %s.\n", 
         pp_msg->request()->u.iv_forget.iv_rmid, 
         XIDtoa(&pp_msg->request()->u.iv_forget.iv_xid)));
      pp_msg->reply(XAER_NOTA);
   }

   TMTrace(2, ("tm_process_xa_forget EXIT.\n"));
} // tm_process_xa_forget


// --------------------------------------------------------------
// tm_process_xa_rollback
// Purpose - Txn  specific processing for xa_rollback.
// --------------------------------------------------------------
void tm_process_xa_rollback(CTmTxMessage * pp_msg)
{
   CTmXaTxn *lp_tx = tm_lookup_xaTxn(pp_msg->request()->u.iv_rollback.iv_rmid,
                                     &pp_msg->request()->u.iv_rollback.iv_xid);
   if (lp_tx)
   {
      TMTrace(2, ("tm_process_xa_rollback ENTRY: Received from (%d,%d) found Txn ID (%d,%d) for rm %d, xid %s.\n", 
         pp_msg->request()->u.iv_rollback.iv_nid,
         pp_msg->request()->u.iv_rollback.iv_pid,
         lp_tx->node(), lp_tx->seqnum(),
         pp_msg->request()->u.iv_rollback.iv_rmid, 
         XIDtoa(&pp_msg->request()->u.iv_rollback.iv_xid)));
      // Ignoring flags for now (prototype)
      TM_Txid_Internal * lp_transid = (TM_Txid_Internal *) lp_tx->transid();
      lp_tx->queueToTransaction(lp_transid, pp_msg);
   }
   else
   {
      TMTrace(1, ("tm_process_xa_rollback ENTRY Txn NOT FOUND for rm %d, xid %s.\n", 
         pp_msg->request()->u.iv_rollback.iv_rmid, 
         XIDtoa(&pp_msg->request()->u.iv_rollback.iv_xid)));
      pp_msg->reply(XAER_NOTA);
   }

   TMTrace(2, ("tm_process_xa_rollback EXIT.\n"));
} // tm_process_xa_rollback


void tm_process_xa_open(CTmTxMessage * pp_msg)
{
   CTmXaRm *lp_xarm = NULL;
   lp_xarm = tm_lookup_xaRM(pp_msg->request()->u.iv_open.iv_rmid);
   if (lp_xarm)
   {
      TMTrace(1, ("tm_process_xa_open ENTRY: Received from (%d,%d) found RM "
         "for rmid %d, openInfo %s, flags " PFLL ", reusing XARM object %p.\n", 
         pp_msg->request()->u.iv_open.iv_nid,
         pp_msg->request()->u.iv_open.iv_pid,
         pp_msg->request()->u.iv_open.iv_rmid, 
         pp_msg->request()->u.iv_open.iv_info,
         pp_msg->request()->u.iv_open.iv_flags,
         (void *) lp_xarm));
   }
   else
   {
      TMTrace(3, ("tm_process_xa_open ENTRY: Received from (%d,%d), rmid %d "
         "is not in use, openInfo %s, flags "PFLL".\n", 
         pp_msg->request()->u.iv_open.iv_nid,
         pp_msg->request()->u.iv_open.iv_pid,
         pp_msg->request()->u.iv_open.iv_rmid, 
         pp_msg->request()->u.iv_open.iv_info,
         pp_msg->request()->u.iv_open.iv_flags));
      lp_xarm = new CTmXaRm(pp_msg->request()->u.iv_open.iv_rmid, 
                            (char *) &pp_msg->request()->u.iv_open.iv_info,
                            pp_msg->request()->u.iv_open.iv_flags);
      bool lv_success = add_xaRM(lp_xarm);
      if (!lv_success)
         abort();
   }

   pp_msg->reply(XA_OK);

   TMTrace(2, ("tm_process_xa_open EXIT.\n"));
} // tm_process_xa_open


void tm_process_xa_close(CTmTxMessage * pp_msg)
{
   CTmXaRm *lp_xarm = NULL;
   lp_xarm = tm_lookup_xaRM(pp_msg->request()->u.iv_close.iv_rmid);
   if (lp_xarm)
   {
      TMTrace(2, ("tm_process_xa_close ENTRY found rmid %d, info %s, flags " PFLL ".\n",
            pp_msg->request()->u.iv_close.iv_rmid, 
            pp_msg->request()->u.iv_close.iv_info,
            pp_msg->request()->u.iv_close.iv_flags));
   }
   else
   {
      TMTrace(1, ("tm_process_xa_end ENTRY RM NOT FOUND for rmid %d, openInfo %s, flags "PFLL".\n", 
         pp_msg->request()->u.iv_open.iv_rmid, 
         pp_msg->request()->u.iv_open.iv_info,
         pp_msg->request()->u.iv_open.iv_flags));
      abort();
   }

   delete_xaRM(lp_xarm);

   pp_msg->reply(XA_OK);
   TMTrace(2, ("tm_process_xa_close EXIT.\n"));
} // tm_process_xa_close


void tm_process_xa_recover(CTmTxMessage * pp_msg)
{
   RM_Recover_Rsp_Type lv_recoverReply;
   TMTrace(2, ("tm_process_xa_recover ENTRY -- NOT IMPLEMENTED YET --.\n"));
   
   //TODO
   lv_recoverReply.iv_end = true;
   lv_recoverReply.iv_count = 0;
   //lv_recoverReply.iv_recovery_index = 0; not found in RM_Recover_Req_Type??
   memset(&lv_recoverReply.iv_xid, NULL, sizeof(lv_recoverReply.iv_xid));

   // Need to handle TMSTATRSCAN, TMENDRSCAN
   pp_msg->reply(XA_OK);
   TMTrace(2, ("tm_process_xa_recover EXIT.\n"));
}


void tm_process_xa_complete(CTmTxMessage * pp_msg)
{
   TMTrace(2, ("tm_process_xa_complete ENTRY -- NOT IMPLEMENTED YET --.\n"));
   
   // Flags TMMULTIPLE & TMNOWAIT
   pp_msg->reply(XA_OK);
   TMTrace(2, ("tm_process_xa_complete EXIT.\n"));
}


void tm_process_ax_reg(CTmTxMessage * pp_msg)
{
   TMTrace(2, ("tm_process_ax_reg ENTRY -- NOT IMPLEMENTED YET --.\n"));
   
   pp_msg->reply(XA_OK);
   TMTrace(2, ("tm_process_ax_reg EXIT.\n"));
}


void tm_process_ax_unreg(CTmTxMessage * pp_msg)
{
   TMTrace(2, ("tm_process_ax_unreg ENTRY -- NOT IMPLEMENTED YET --.\n"));
   
   pp_msg->reply(XA_OK);
   TMTrace(2, ("tm_process_ax_unreg EXIT.\n"));
}

