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
#include "SCMVersHelp.h"

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

#include "tmglobals.h"
#include "tmtimer.h"
#include "tmthreadeg.h"

#include "tminfo.h"
#include "tmrecov.h"
#include "tmtxbranches.h"

#include "hbasetm.h"

extern void tm_xarm_initialize();
extern void tm_process_msg_from_xarm(CTmTxMessage * pp_msg);
extern int HbaseTM_initialize (bool pp_tracing, bool pv_tm_stats, CTmTimer *pp_tmTimer, short pv_nid);
extern int HbaseTM_initiate_stall(int where);
extern HashMapArray* HbaseTM_process_request_regions_info();


// Version
DEFINE_EXTERN_COMP_DOVERS(tm)
// global data 
CTmTxBranches     gv_RMs;
TM_MAP            gv_sync_map;
int32             gv_system_tx_count = 0;         
TM_Info           gv_tm_info;
SB_Int64_Type     gv_wait_interval = TM_DEFAULT_WAIT_INTERVAL;

CTmThreadExample  *gp_tmExampleThread;


// ---------------------------------------------------------------
// misc helper routines
// ---------------------------------------------------------------
void tm_fill_perf_stats_buffer (Tm_Perf_Stats_Rsp_Type *pp_buffer)
{

    int64 lv_tx_count = gv_tm_info.tx_count();
    int64 lv_abort_count = gv_tm_info.abort_count();
    int64 lv_commit_count = gv_tm_info.commit_count();
    int64 lv_current_tx_count =  gv_tm_info.current_tx_count();
    int32 lv_tm_abort_count =  gv_tm_info.tm_initiated_aborts();
    int32 lv_hung_tx_count = gv_tm_info.tx_hung_count();

    TMTrace (2, ("tm_fill_perf_stats_buffer : tx count : " PFLL ", abort count " PFLL ", commit count " PFLL ", current tx count " PFLL ".\n", lv_tx_count, lv_abort_count, lv_commit_count, lv_current_tx_count));

    pp_buffer->iv_error = 0;
    pp_buffer->iv_tx_count = lv_tx_count;
    pp_buffer->iv_abort_count = lv_abort_count;
    pp_buffer->iv_commit_count = lv_commit_count;
    pp_buffer->iv_tm_initiated_aborts = lv_tm_abort_count;
    pp_buffer->iv_hung_tx_count = lv_hung_tx_count;
    pp_buffer->iv_outstanding_tx_count = lv_current_tx_count;
    pp_buffer->iv_oldest_transid_1 = 0;
    pp_buffer->iv_oldest_transid_2 = 0;
    pp_buffer->iv_oldest_transid_3 = 0;
    pp_buffer->iv_oldest_transid_4 = 0;
}

void tm_fill_sys_status_buffer (Tm_Sys_Status_Rsp_Type *pp_buffer)
{

    int32 lv_up = 0;
    int32 lv_down = 0;
    if(gv_tm_info.state() == TM_STATE_UP) {
        lv_up = 1;
    }
    else if(gv_tm_info.state() == TM_STATE_DOWN ||
            gv_tm_info.state() == TM_STATE_WAITING_RM_OPEN) {
        lv_down = 1;
    }

    int32 lv_recovering = 0;
    if(gv_tm_info.sys_recov_state() != TM_SYS_RECOV_STATE_END) {
        lv_recovering = 1;
    }
    int32 lv_totaltms = 1;
    int32 lv_leadtm = 0;
    if(gv_tm_info.lead_tm()) {
        lv_leadtm = 1;
    }
    
    int32 lv_activetxns = gv_tm_info.num_active_txs();
    // If we're still in recovery we need to add any transactions 
    // still queued to recover.
    if (gv_tm_info.ClusterRecov())
       lv_activetxns += gv_tm_info.ClusterRecov()->txnStateList()->size();

    TMTrace (2, ("tm_fill_sys_status_buffer : up %d, down %d, recovering %d, activetxns %d.\n", lv_up, lv_down, lv_recovering, lv_activetxns));

    pp_buffer->iv_status_system.iv_up = lv_up;
    pp_buffer->iv_status_system.iv_down = lv_down;
    pp_buffer->iv_status_system.iv_recovering = lv_recovering;
    pp_buffer->iv_status_system.iv_totaltms = lv_totaltms;
    pp_buffer->iv_status_system.iv_activetxns = lv_activetxns;
    pp_buffer->iv_status_system.iv_leadtm = lv_leadtm;
}

// ---------------------------------------------------------
// tm_initialize_rsp_hdr
// Purpose : Initialize the header field for a message response.
// Note this should only be used for broadcast syncs now.
// All TM Library responses are through the CTmTxMessage class.
// ---------------------------------------------------------
int tm_initialize_rsp_hdr(short rsp_type, Tm_Rsp_Msg_Type *pp_rsp)
{
   pp_rsp->iv_msg_hdr.dialect_type = DIALECT_TM_SQ;
   pp_rsp->iv_msg_hdr.rr_type.reply_type = (short) (rsp_type + 1);
   pp_rsp->iv_msg_hdr.version.reply_version = TM_SQ_MSG_VERSION_CURRENT;
   pp_rsp->iv_msg_hdr.miv_err.error = FEOK;
   return FEOK;
}

// ----------------------------------------------------------------
// tm_start_auditThread
// Purpose : Start the audit thread.  
// ----------------------------------------------------------------
void tm_start_auditThread()
{
   char lv_name[20];

   TMTrace(2, ("tm_start_auditThread ENTRY\n")); 

   // Instantiate timer object
   sprintf(lv_name, "auditTh");
   CTmAuditObj *lp_auditobj = new CTmAuditObj(auditThread_main, (const char *) &lv_name);
   if (lp_auditobj)
   {
      gv_tm_info.tmAuditObj(lp_auditobj);
      gv_tm_info.initialize_adp();
   }
   else
   {
      tm_log_event(DTM_TMTIMER_FAILED, SQ_LOG_CRIT, "DTM_TMTIMER_FAILED");
      TMTrace(1, ("tm_start_auditThread - Failed to instantiate audit object.\n"));
      abort();
   }

   TMTrace(2, ("tm_start_auditThread EXIT. Timer thread %s(%p) started.\n", 
         lv_name, (void *) lp_auditobj));
} //tm_start_auditThread

// ----------------------------------------------------------------
// tm_start_timerThread
// Purpose : Start the timer thread.  This is used whenever an
// internal timed event occurs.
// ----------------------------------------------------------------
void tm_start_timerThread()
{
   char lv_name[20];

   TMTrace(2, ("tm_start_timerThread ENTRY\n")); 

   // Instantiate timer object
   sprintf(lv_name, "timerTh");
   CTmTimer *lp_timer = new CTmTimer(timerThread_main, -1, (const char *) &lv_name,
                                     gv_tm_info.timerDefaultWaitTime());
   if (lp_timer)
   {
      gv_tm_info.tmTimer(lp_timer);
      gv_startTime = lp_timer->startTime();
      lp_timer->addControlpointEvent(gv_tm_info.cp_interval());
      if (gv_tm_info.lead_tm())
      {
         TMTrace(2, ("tm_start_timerThread lead DTM, adding timer events\n")); 
         lp_timer->addStatsEvent(gv_tm_info.stats_interval());
      }
     lp_timer->addRMRetryEvent(gv_tm_info.RMRetry_interval());
   }
   else
   {
      tm_log_event(DTM_TMTIMER_FAILED, SQ_LOG_CRIT, "DTM_TMTIMER_FAILED");
      TMTrace(1, ("tm_start_timerThread - Failed to instantiate timer object.\n"));
      abort();
   }

   TMTrace(2, ("tm_start_timerThread EXIT. Timer thread %s(%p) started.\n", 
         lv_name, (void *) lp_timer));
} //tm_start_timerThread


// ----------------------------------------------------------------
// tm_start_exampleThread
// Purpose : Start the example thread.
// ----------------------------------------------------------------
void tm_start_exampleThread()
{
   char lv_name[20];

   TMTrace(2, ("tm_start_exampleThread ENTRY\n")); 

   // Instantiate thread object
   sprintf(lv_name, "exampleTh");
   gp_tmExampleThread = new CTmThreadExample(exampleThread_main, -2, (const char *) &lv_name);
   if (!gp_tmExampleThread)
   {
      tm_log_event(DTM_TMTIMER_FAILED, SQ_LOG_CRIT, "DTM_TMTIMER_FAILED");
      TMTrace(1, ("tm_start_exampleThread - Failed to instantiate example thread object\n"));
      abort();
   }

   TMTrace(2, ("tm_start_exampleThread EXIT. Example thread %s(%p) started.\n", 
         lv_name, (void *) gp_tmExampleThread));
} //tm_start_exampleThread


// ---------------------------------------------------------
// tm_send_reply
// Purpose :  send a reply
// Note this should only be used for cp and shutdown
// replies.
// ---------------------------------------------------------
void tm_send_reply(int32 pv_msgid, Tm_Rsp_Msg_Type *pp_rsp)
{
    int lv_len = sizeof(Tm_Rsp_Msg_Type);
    TMTrace( 2, ("tm_send_reply : ENTRY. msgid(%d), reply code(%d), error(%d).\n", 
                    pv_msgid, pp_rsp->iv_msg_hdr.rr_type.reply_type, 
                    pp_rsp->iv_msg_hdr.miv_err.error));

    BMSG_REPLY_(pv_msgid,                // msgid
                NULL,                    // replyctrl
                0,                       // replyctrlsize
                (char *) pp_rsp,         // replydata
                lv_len,                  // replydatasize
                0,                       // errorclass
                NULL);                   // newphandle
}

// ---------------------------------------------------------
// tm_up_check
// Purpose : Check that DTM is up and, if not, reply 
// to the client with an error.
// We need to allow active transasctions to continue to 
// completion when shutting down and when transactions are
// disabled.
// Note that this function will delete pp_msg if there is
// an error, so it must not be used after the call if false
// is returned.
// pv_block    default=true, block client in TM_LIB by returning
//          FESERVICEDISABLED.
//          false - don't block client on reply.
// --------------------------------------------------------
const bool TX_UNBLOCKED=false,
           TX_BLOCKED=true;

bool tm_up_check(CTmTxMessage * pp_msg, bool pv_block=true) 
{
    bool lv_up = false;
    short lv_replyCode = FEOK;

    switch (gv_tm_info.state())
    {
    case TM_STATE_UP:
    case TM_STATE_SHUTTING_DOWN:
    case TM_STATE_TX_DISABLED:
    case TM_STATE_TX_DISABLED_SHUTDOWN_PHASE1:
    case TM_SYS_RECOV_STATE_END:
    case TM_STATE_DRAIN:
       lv_replyCode = FEOK;
       lv_up = true;
       break;
    case TM_STATE_QUIESCE:
       lv_replyCode = (pv_block)?FESERVICEDISABLED:FETMFNOTRUNNING;
       lv_up = false;
       break;
    default:
       lv_replyCode = FETMFNOTRUNNING;
       lv_up = false;
    }
    
    if (!lv_up)
    {
       TMTrace(1, ("tm_up_check EXIT replying %d, up=%d.\n", lv_replyCode, lv_up));
       pp_msg->reply(lv_replyCode);
       delete pp_msg;
    }
    return lv_up;
}

// ---------------------------------------------------------
// tm_notx_check
// Purpose : Check that transaction exists, if not, reply 
// to the client with an error.  
// Note that this function will delete pp_msg if there is
// an error, so it must not be used after the call if false
// is returned
// --------------------------------------------------------
bool tm_notx_check(TM_TX_Info *pp_tx, CTmTxMessage *pp_msg)
{
   if (pp_tx == NULL)
   {
      TMTrace(1, ("tm_notx_check - unable to complete, returning error FENOTRANSID\n"));
      pp_msg->reply(FENOTRANSID);
      delete pp_msg;
      return false;
   }
   return true;
}

// ------------------------------------------------------------
// Process request methods
// ------------------------------------------------------------

// --------------------------------------------------------------
// process_req_abort
// Purpose : process message of type TM_MSG_TYPE_ABORTTRANSACTION
// --------------------------------------------------------------- 
void tm_process_req_abort(CTmTxMessage * pp_msg) 
{
    TM_Txid_Internal * lp_transid = (TM_Txid_Internal *)
                                    &pp_msg->request()->u.iv_abort_trans.iv_transid;
    
    TMTrace(2, ("tm_process_req_abort, ID %d ENTRY\n", lp_transid->iv_seq_num));

    if (!tm_up_check(pp_msg))
        return;
 
    TM_TX_Info *lp_tx = (TM_TX_Info*) gv_tm_info.get_tx(lp_transid);

    if (!tm_notx_check(lp_tx, pp_msg))
       return;

    lp_tx->stats()->txnAbort()->start();

    if (!gv_tm_info.multithreaded()) {
       lp_tx->req_abort(pp_msg);
       gv_tm_info.cleanup(lp_tx);
       delete pp_msg;
    }
    else
       lp_tx->queueToTransaction(lp_transid, pp_msg);

    TMTrace(2, ("tm_process_req_abort EXIT\n"));
}


// ----------------------------------------------------------------
// tm_process_req_registerregion
// Purpose : process message of type TM_MSG_TYPE_REGISTERREGION
// ----------------------------------------------------------------
void tm_process_req_registerregion(CTmTxMessage * pp_msg)
{
    TM_Txid_Internal * lp_transid = (TM_Txid_Internal *)
                                    &pp_msg->request()->u.iv_register_region.iv_transid;
    TM_Transseq_Type * lp_startid = (TM_Transseq_Type *)
                                    &pp_msg->request()->u.iv_register_region.iv_startid;

    TMTrace(2, ("tm_process_req_registerregion ENTRY for Txn ID (%d,%d), startid %ld, msgid %d\n",
                lp_transid->iv_node, lp_transid->iv_seq_num, (long) lp_startid, pp_msg->msgid()));
    TMTrace(3, ("tm_process_req_registerregion for Txn ID (%d,%d), startid %ld, with region %s\n",
                lp_transid->iv_node, lp_transid->iv_seq_num, (long) lp_startid,
                pp_msg->request()->u.iv_register_region.ia_regioninfo2)); 

    TM_TX_Info *lp_tx = (TM_TX_Info*) gv_tm_info.get_tx(lp_transid);

    if (!tm_notx_check(lp_tx, pp_msg))
       return;

    // lp_tx->req_registerRegion(pp_msg);
    if (!gv_tm_info.multithreaded()) {
       lp_tx->req_registerRegion(pp_msg);
       delete pp_msg;
    }
    else{
       lp_tx->queueToTransaction(lp_transid, pp_msg);
       // Protect the reply as we may be trying to reply at the same time in the main thread.
       lp_tx->lock();
       if (lp_tx->transactionBusy() && pp_msg->replyPending())
          pp_msg->reply(FEOK);
       lp_tx->unlock();
    }

    TMTrace(2, ("tm_process_req_registerregion EXIT\n")); 

} // tm_process_req_registerregion

// -----------------------------------------------------------------
// tm_process_req_ddlrequest
// Purpose : process message of type TM_MSG_TYPE_DDLREQUEST
// -----------------------------------------------------------------
void tm_process_req_ddlrequest(CTmTxMessage * pp_msg)
{
    TM_Txid_Internal * lp_transid = (TM_Txid_Internal *)
                                    &pp_msg->request()->u.iv_ddl_request.iv_transid;

    TMTrace(2, ("tm_process_req_ddlrequest ENTRY for Txn ID (%d, %d) ", lp_transid->iv_node, lp_transid->iv_seq_num));

    TM_TX_Info *lp_tx = (TM_TX_Info*) gv_tm_info.get_tx(lp_transid);

    if (!gv_tm_info.multithreaded()) {
       lp_tx->req_ddloperation(pp_msg);
       pp_msg->reply(FEOK);
       delete pp_msg;
    }
    else {
       lp_tx->queueToTransaction(lp_transid, pp_msg);
    }

    TMTrace(2, ("tm_process_req_ddlrequest EXIT for Txn ID"));
}

//-----------------------------------------------------------------
// tm_process_req_requestregioninfo
// Purpose: process message of type TM_MSG_TYPE_REQUESTREGIONINFO
//-----------------------------------------------------------------
void tm_process_req_requestregioninfo(CTmTxMessage * pp_msg)
{
   int64          lv_size = 0;
   void         **lp_tx_list = gv_tm_info.get_all_txs (&lv_size);
   union
   {
       int64 lv_transid_int64;
       TM_Txid_legacy lv_transid;
   } u;

   char tname[2000];
   tname[299] = '\0';
/*
   char ername[50], rname[100], offline[20], regid[200], hostname[200], port[100];
   tname[299] = '\0', ername[49] = '\0', rname[99] = '\0', offline[19] = '\0';
   regid[199]= '\0', hostname[199]='\0', port[99]='\0';
*/

   TMTrace(2, ("tm_process_req_requestregioninfo ENTRY.\n"));
   HashMapArray* map = HbaseTM_process_request_regions_info();
   TMTrace(2, ("tm_process_req_requestregioninfo HashMapArray call has finished.\n"));

   pp_msg->response()->u.iv_hbaseregion_info.iv_count = 0;
   for (int lv_inx = 0; ((lv_inx < TM_MAX_LIST_TRANS) && (lv_inx < lv_size)); lv_inx++)
   {
        TM_TX_Info *lp_current_tx = (TM_TX_Info *)lp_tx_list[lv_inx];
        if (!lp_current_tx)
            continue;
        pp_msg->response()->u.iv_hbaseregion_info.iv_trans[lv_inx].iv_status = lp_current_tx->tx_state();
        u.lv_transid.iv_seq_num = lp_current_tx->seqnum();
        pp_msg->response()->u.iv_hbaseregion_info.iv_trans[lv_inx].iv_transid = u.lv_transid_int64;
        pp_msg->response()->u.iv_hbaseregion_info.iv_trans[lv_inx].iv_nid = lp_current_tx->node();
        pp_msg->response()->u.iv_hbaseregion_info.iv_trans[lv_inx].iv_seqnum = lp_current_tx->seqnum();

        char* res2 = map->getRegionInfo(lp_current_tx->legacyTransid());
        if(strlen(res2) == 0)
            continue;
        strncpy(tname, res2, sizeof(tname) -1);
        strncpy(pp_msg->response()->u.iv_hbaseregion_info.iv_trans[lv_inx].iv_tablename, tname, sizeof(tname)-1);
/*

        char* res3 = map->getEncodedRegionName(lv_inx);
        strncpy(ername, res3, sizeof(ername) -1);
        strncpy(pp_msg->response()->u.iv_hbaseregion_info.iv_trans[lv_inx].iv_enc_regionname, ername, sizeof(ername)-1);

        char* res4 = map->getRegionName(lv_inx);
        strncpy(rname, res4, sizeof(rname) -1);
        strncpy(pp_msg->response()->u.iv_hbaseregion_info.iv_trans[lv_inx].iv_regionname, res4, strlen(res4));

        char* res5 = map->getRegionOfflineStatus(lv_inx);
        strncpy(offline, res5, sizeof(offline) -1);
        strncpy(pp_msg->response()->u.iv_hbaseregion_info.iv_trans[lv_inx].iv_is_offline, offline, sizeof(offline)-1);

        char* res6 = map->getRegionId(lv_inx);
        strncpy(regid, res6, sizeof(regid) -1);
        strncpy(pp_msg->response()->u.iv_hbaseregion_info.iv_trans[lv_inx].iv_region_id, regid, sizeof(regid)-1);

        char* res7 = map->getHostName(lv_inx);
        strncpy(hostname, res7, sizeof(hostname) -1);
        strncpy(pp_msg->response()->u.iv_hbaseregion_info.iv_trans[lv_inx].iv_hostname, hostname, sizeof(hostname)-1);

        char* res8 = map->getPort(lv_inx);
        strncpy(port, res8, sizeof(port) -1);
        strncpy(pp_msg->response()->u.iv_hbaseregion_info.iv_trans[lv_inx].iv_port, port, sizeof(port)-1);
*/

        pp_msg->response()->u.iv_hbaseregion_info.iv_count++;
   }

   if (lp_tx_list)
      delete []lp_tx_list;
   pp_msg->reply(FEOK);
   delete pp_msg;
   delete map;

   TMTrace(2, ("tm_process_req_requestregioninfo EXIT\n"));
}


// ----------------------------------------------------------------
// tm_process_req_GetNextSeqNum
// Purpose : Retrieve the next transaction sequence number
// block.  This is used to implement local transactions
// in Trafodion.
// ----------------------------------------------------------------
void tm_process_req_GetNextSeqNum(CTmTxMessage * pp_msg)
{
    TMTrace(2, ("tm_process_req_GetNextSeqNum ENTRY.\n")); 
    
    gv_tm_info.lock();
    gv_tm_info.tm_new_seqNumBlock(pp_msg->request()->u.iv_GetNextSeqNum.iv_block_size,
                                  &pp_msg->response()->u.iv_GetNextSeqNum.iv_seqNumBlock_start,
                                  &pp_msg->response()->u.iv_GetNextSeqNum.iv_seqNumBlock_count);
    gv_tm_info.unlock();
    pp_msg->reply(FEOK);

    TMTrace(2, ("tm_process_req_GetNextSeqNum EXIT returning Next seqNum start %d, block size %d\n",
            pp_msg->response()->u.iv_GetNextSeqNum.iv_seqNumBlock_start, 
            pp_msg->response()->u.iv_GetNextSeqNum.iv_seqNumBlock_count)); 
    delete pp_msg;
} // tm_process_req_GetNextSeqNum


// ----------------------------------------------------------------
// tm_process_req_begin
// Purpose : process message of type TM_MSG_TYPE_BEGINTRANSACTION
// ----------------------------------------------------------------
void tm_process_req_begin(CTmTxMessage * pp_msg) 
{
    short lv_error = FEOK;
    TMTrace(2, ("tm_process_req_begin ENTRY\n")); 
 
    if ((gv_tm_info.state() != TM_STATE_UP) ||
        (gv_tm_info.sys_recov_state() != TM_SYS_RECOV_STATE_END))
    {
        switch (gv_tm_info.state())
        {
        case TM_STATE_TX_DISABLED:
        case TM_STATE_DRAIN:
            lv_error = FEBEGINTRDISABLED;
            break;
        case TM_STATE_QUIESCE:
            lv_error = FESERVICEDISABLED;
            break;
        default:
            lv_error = FETMFNOTRUNNING;
        }
        TMTrace(1, ("tm_process_req_begin returning error %d.\n", lv_error));
        pp_msg->reply(lv_error);
        delete pp_msg;
        return;
    }

    // Instantiate a new tx object.
    TM_TX_Info *lp_tx = (TM_TX_Info *) gv_tm_info.new_tx(pp_msg->request()->u.iv_begin_trans.iv_nid, 
                                                       pp_msg->request()->u.iv_begin_trans.iv_pid,
                                                       -1, -1,
                                                       (void* (*)(long int)) &TM_TX_Info::constructPoolElement);

    // An error indicates we are handling our maximum number of concurrent
    // transactions.
    if (lp_tx == NULL)
    {
       // Removing this event for now as we keep hitting it and it's just announcing that 
       //   we've reached the maximum transactions allowed per node.
       //tm_log_event(DTM_TX_MAX_EXCEEDED, SQ_LOG_WARNING, "DTM_TX_MAX_EXCEEDED", 
       //             -1, /*error_code*/ 
       //             -1, /*rmid*/
       //             gv_tm_info.nid(), /*dtmid*/ 
       //             -1, /*seq_num*/
       //             -1, /*msgid*/
       //             -1, /*xa_error*/
       //             gv_tm_info.transactionPool()->get_maxPoolSize(), /*pool_size*/
       //             gv_tm_info.transactionPool()->totalElements() /*pool_elems*/);
       TMTrace(1, ("tm_process_req_begin, FETOOMANYTRANSBEGINS\n"));
       pp_msg->reply(FETOOMANYTRANSBEGINS);
       delete pp_msg;
       return;
    }
    lp_tx->lock();

    lp_tx->setAbortTimeout(pp_msg->request()->u.iv_begin_trans.iv_abort_timeout);
    lp_tx->TT_flags(pp_msg->request()->u.iv_begin_trans.iv_transactiontype_bits);

    // Start statistics counters
    lp_tx->stats()->txnTotal()->start();
    lp_tx->stats()->txnBegin()->start();

    //M8 eliminate the association with the transaction as there is
    // nothing more to do now that we don't support xa_start

    lp_tx->req_begin(pp_msg);
    lp_tx->unlock();

    // Since we're not queuing requests, we can delete pp_req here itself.
    delete pp_msg;

    TMTrace(2, ("tm_process_req_begin, ID (%d,%d), creator (%d,%d) EXIT\n", 
            lp_tx->node(), lp_tx->seqnum(), lp_tx->ender_nid(), lp_tx->ender_pid()));
}

// --------------------------------------------------------------
// process_req_doomtx
// Purpose : process message of type TM_MSG_TYPE_DOOMTX
// DOOMTRANSACTION marks the transaction for rollback and
// replies immediately.  Then in the background it drives 
// rollback.
// or ABORTTRANSACTION.  
// --------------------------------------------------------------- 
void tm_process_req_doomtx(CTmTxMessage * pp_msg) 
{
    TM_Txid_Internal * lp_transid = (TM_Txid_Internal *)
                                    &pp_msg->request()->u.iv_abort_trans.iv_transid;
    
   TMTrace(2, ("tm_process_req_doomtx ID %d ENTRY\n", lp_transid->iv_seq_num));

   if (!tm_up_check(pp_msg))
       return;
 
   TM_TX_Info *lp_tx = (TM_TX_Info*) gv_tm_info.get_tx(lp_transid);

   if (!tm_notx_check(lp_tx, pp_msg))
      return; 

   int16 lv_error = lp_tx->doom_txn();
   pp_msg->reply(lv_error);

   delete pp_msg;

   TMTrace(2, ("tm_process_req_doomtx EXIT\n"));
} //process_req_doomtx

// --------------------------------------------------------------
// process_req_TSE_doomtx
// Purpose : process message of type TM_MSG_TYPE_TSE_DOOMTX
// This is different from an application doomtransaction because
// it drives an immediate rollback.  This is necessary because 
// the TSE might be dooming the transaction because we have hit
// an audit threshold and can't allow the transaction to continue.
// --------------------------------------------------------------- 
void tm_process_req_TSE_doomtx(CTmTxMessage * pp_msg) 
{
   TM_Txid_Internal * lp_transid = (TM_Txid_Internal *)
                                    &pp_msg->request()->u.iv_abort_trans.iv_transid;
    
   TMTrace(2, ("tm_process_req_TSE_doomtx, ID %d ENTRY\n", lp_transid->iv_seq_num));

   if (!tm_up_check(pp_msg, TX_UNBLOCKED))
       return;
 
   TM_TX_Info *lp_tx = (TM_TX_Info*) gv_tm_info.get_tx(lp_transid);

   if (!tm_notx_check(lp_tx, pp_msg))
      return;

   if (lp_tx->isAborting())
   {
      TMTrace(1, ("tm_process_req_TSE_doomtx, already doomed.\n"));
      pp_msg->reply(FEOK);
      delete pp_msg;
   }
   else
   {
      int16 lv_error = lp_tx->doom_txn();
      pp_msg->reply(lv_error);
      lp_tx->queueToTransaction(lp_transid, pp_msg);
   }

   TMTrace(2, ("tm_process_req_TSE_doomtx EXIT\n"));
}

// --------------------------------------------------------------
// process_req_wait_tmup
// Purpose : Wait until the TM is up, and only then reply. 
// This can be used by an application to wait for DTM to be ready
// to process transactions.
// --------------------------------------------------------------- 
void tm_process_req_wait_tmup(CTmTxMessage * pp_msg) 
{
   TMTrace(2, ("tm_process_req_wait_tmup ENTRY\n"));

   if ((gv_tm_info.state() == TM_STATE_UP) &&
      (gv_tm_info.sys_recov_state() == TM_SYS_RECOV_STATE_END))
   {
      TMTrace(3, ("tm_process_req_wait_tmup : TM up, replying immediately.\n"));

      pp_msg->reply(FEOK);
      delete pp_msg;
   }
   else
   {
     TMTrace(3, ("tm_process_req_wait_tmup : Adding caller msgid(%d) to TMUP_Wait list.\n",
                      pp_msg->msgid()));
      gv_tm_info.TMUP_wait_list()->push(pp_msg);
   }

   TMTrace(2, ("tm_process_req_wait_tmup EXIT\n"));
} //process_req_wait_tmup

// --------------------------------------------------------------
// Purpose : process message of type TM_MSG_TYPE_ENDRANSACTION
// ---------------------------------------------------------------
void tm_process_req_end(CTmTxMessage * pp_msg) 
{
    TM_Txid_Internal * lp_transid = (TM_Txid_Internal *)
                                    &pp_msg->request()->u.iv_end_trans.iv_transid;
    
   TMTrace(1, ("tm_process_req_end, ID %d ENTRY\n", lp_transid->iv_seq_num));

    if (!tm_up_check(pp_msg))
       return;
 
    TM_TX_Info *lp_tx = (TM_TX_Info*) gv_tm_info.get_tx(lp_transid);

    if (!tm_notx_check(lp_tx, pp_msg))
       return;

    lp_tx->stats()->txnCommit()->start();

    if (!gv_tm_info.multithreaded()) {
       lp_tx->req_end(pp_msg);
       lp_tx->req_forget(pp_msg);
       gv_tm_info.cleanup(lp_tx);
       delete pp_msg;
    }
    else
      lp_tx->queueToTransaction(lp_transid, pp_msg);

   TMTrace(2, ("tm_process_req_end, ID %d EXIT\n", lp_transid->iv_seq_num));
}

// ------------------------------------------------------------------
// tm_process_req_join_trans
// Purpose : process message of type TM_MSG_TYPE_JOINTRANSACTION
// ------------------------------------------------------------------
void tm_process_req_join_trans(CTmTxMessage * pp_msg)
{
    TM_Txid_Internal *lp_transid = (TM_Txid_Internal *)
                                   &pp_msg->request()->u.iv_join_trans.iv_transid;

    TMTrace(2, ("tm_process_req_join_trans, ID %d, ENTRY\n",
                     lp_transid->iv_seq_num));

    if (!tm_up_check(pp_msg, TX_UNBLOCKED))
        return;

    TM_TX_Info *lp_tx = (TM_TX_Info *)gv_tm_info.get_tx(lp_transid);

    if (!tm_notx_check(lp_tx, pp_msg))
        return;
   
    // Call join in-line in main thread
    lp_tx->req_join(pp_msg);
    // Since we don't queue join requests, we can delete pp_req here itself.
    delete pp_msg;

    TMTrace(2, ("tm_process_req_join_trans EXIT\n"));
}


// -----------------------------------------------------------------
// tm_process_req_list
// Purpose : Process a list transactions request.
// ----------------------------------------------------------------
void tm_process_req_list(CTmTxMessage *pp_msg)
{
   int64          lv_size = 0;
   void         **lp_tx_list = gv_tm_info.get_all_txs (&lv_size);
   union
   {
       int64 lv_transid_int64;
       TM_Txid_legacy lv_transid;
   } u;

   TMTrace(2, ("tm_process_req_list ENTRY.\n"));

   pp_msg->response()->u.iv_list_trans.iv_count = 0;
   for (int lv_inx = 0; ((lv_inx < TM_MAX_LIST_TRANS) && (lv_inx < lv_size)); lv_inx++)
   {
        TM_TX_Info *lp_current_tx = (TM_TX_Info *)lp_tx_list[lv_inx];
        if (!lp_current_tx)
            break;
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_status = lp_current_tx->tx_state(); 
        u.lv_transid.iv_seq_num = lp_current_tx->seqnum();
        u.lv_transid.iv_node = lp_current_tx->node();
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_transid = u.lv_transid_int64;
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_nid = lp_current_tx->node();
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_seqnum = lp_current_tx->seqnum();
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_tag = lp_current_tx->tag();
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_owner_nid = lp_current_tx->ender_nid();
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_owner_pid = lp_current_tx->ender_pid();
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_event_count = lp_current_tx->eventQ()->size();
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_pendingRequest_count = lp_current_tx->PendingRequestQ()->size();
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_num_active_partic = lp_current_tx->num_active_partic();
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_num_partic_RMs = lp_current_tx->get_TSEBranchesParticCount();
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_XARM_branch = false; //TODO
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_transactionBusy = lp_current_tx->transactionBusy();
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_mark_for_rollback = lp_current_tx->mark_for_rollback();
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_tm_aborted = (lp_current_tx->tm_aborted()|lp_current_tx->tse_aborted());
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_read_only = lp_current_tx->read_only();
        pp_msg->response()->u.iv_list_trans.iv_trans[lv_inx].iv_recovering = lp_current_tx->recovering();
        pp_msg->response()->u.iv_list_trans.iv_count++;
   }
    
   if (lp_tx_list)
      delete []lp_tx_list;
   pp_msg->reply(FEOK);
   delete pp_msg;
   TMTrace(2, ("tm_process_req_list EXIT.\n"));
} //tm_process_req_list

//----------------------------------------------------------------
// tm_process_req_status_all_transmgmt
// Purpose : Process status of all transactions of type
// TM_MSG_TYPE_STATUSALLTRANSMGT
// ----------------------------------------------------------------

void tm_process_req_status_all_transmgmt(CTmTxMessage *pp_msg)
{
   int64        lv_size = 0;
   void       **lp_tx_list = gv_tm_info.get_all_txs (&lv_size);
   union
   {
        int64 lv_transid_int64;
        TM_Txid_legacy lv_transid;
   } u;

   TMTrace(2, ("tm_process_req_status_all_transmgmt ENTRY.\n"));

   pp_msg->response()->u.iv_status_alltrans.iv_count = 0;
   for (int lv_inx = 0; ((lv_inx < TM_MAX_LIST_TRANS) && (lv_inx < lv_size)); lv_inx++)
   {
       TM_TX_Info *lp_current_tx = (TM_TX_Info *)lp_tx_list[lv_inx];
       if (!lp_current_tx)
             break;
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_status = lp_current_tx->tx_state();
       u.lv_transid.iv_seq_num = lp_current_tx->seqnum();
       u.lv_transid.iv_node = lp_current_tx->node();
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_transid = u.lv_transid_int64;
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_timestamp = lp_current_tx->timestamp();
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_nid = lp_current_tx->node();
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_seqnum = lp_current_tx->seqnum();
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_tag = lp_current_tx->tag();
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_owner_nid = lp_current_tx->ender_nid();
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_owner_pid = lp_current_tx->ender_pid();
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_event_count = lp_current_tx->eventQ()->size();
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_pendingRequest_count = lp_current_tx->PendingRequestQ()->size();
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_num_active_partic = lp_current_tx->num_active_partic();
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_num_partic_RMs = lp_current_tx->get_TSEBranchesParticCount();
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_num_unresolved_RMs = lp_current_tx->get_TSEBranchesUnresolvedCount();
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_XARM_branch = false; //TODO
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_transactionBusy = lp_current_tx->transactionBusy();
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_mark_for_rollback = lp_current_tx->mark_for_rollback();
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_tm_aborted = (lp_current_tx->tm_aborted()|lp_current_tx->tse_aborted());
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_read_only = lp_current_tx->read_only();
       pp_msg->response()->u.iv_status_alltrans.iv_trans[lv_inx].iv_recovering = lp_current_tx->recovering();
       pp_msg->response()->u.iv_status_alltrans.iv_count++;
   }
                                                                                                                  if (lp_tx_list)
        delete []lp_tx_list;
   pp_msg->reply(FEOK);
   delete pp_msg;
   TMTrace(2, ("tm_process_req_status_all_transmgmt EXIT.\n"));

}// tm_process_req_status_all_transmgmt 


// -----------------------------------------------------------------
// tm_process_req_tmstats
// Purpose : process message of type TM_MSG_TYPE_TMSTATS to
// list TM statistics.
// ----------------------------------------------------------------
void tm_process_req_tmstats(CTmTxMessage *pp_msg)
{
   TMTrace(2, ("tm_process_req_tmstats ENTRY.\n"));

   gv_tm_info.stats()->readStats(&pp_msg->response()->u.iv_tmstats.iv_stats);

   if (pp_msg->request()->u.iv_tmstats.iv_reset) {
      gv_tm_info.clearCounts();
      gv_tm_info.stats()->clearCounters();
   }

   pp_msg->reply(FEOK);
   delete pp_msg;
   TMTrace(2, ("tm_process_req_tmstats EXIT.\n"));
} //tm_process_req_tmstats

// -----------------------------------------------------------------
// tm_process_req_attachrm
// Purpose : process message of type TM_MSG_TYPE_ATTACHRM to
// return the status of this TM.
// ----------------------------------------------------------------
void tm_process_req_attachrm(CTmTxMessage *pp_msg)
{
    TMTrace(2, ("tm_process_req_attachrm ENTRY fpr %s.\n", pp_msg->request()->u.iv_attachrm.ia_rmname));
 
    gv_tm_info.addTimerEvent(pp_msg, 0 /*execute now*/);

    pp_msg->reply(FEOK);
    delete pp_msg;

    TMTrace(2, ("tm_process_req_attachrm EXIT.\n"));
}

// -----------------------------------------------------------------
// tm_process_req_statustm
// Purpose : process message of type TM_MSG_TYPE_STATUSTM to
// return the status of this TM.
// ----------------------------------------------------------------
void tm_process_req_statustm(CTmTxMessage *pp_msg)
{
   RM_Info_TSEBranch *lp_rm;
   TMTrace(2, ("tm_process_req_statustm ENTRY.\n"));

   pp_msg->response()->u.iv_statustm.iv_status.iv_node = gv_tm_info.nid();
   pp_msg->response()->u.iv_statustm.iv_status.iv_isLeadTM = gv_tm_info.lead_tm();
   pp_msg->response()->u.iv_statustm.iv_status.iv_state = gv_tm_info.state();
   pp_msg->response()->u.iv_statustm.iv_status.iv_sys_recovery_state = gv_tm_info.sys_recov_state();
   pp_msg->response()->u.iv_statustm.iv_status.iv_shutdown_level = gv_tm_info.shutdown_level();
   pp_msg->response()->u.iv_statustm.iv_status.iv_incarnation_num = gv_tm_info.incarnation_num();

   pp_msg->response()->u.iv_statustm.iv_status.iv_number_active_txns = gv_tm_info.num_active_txs();
   // Pick up any queued indoubt transactions
   if (gv_tm_info.ClusterRecov())
      pp_msg->response()->u.iv_statustm.iv_status.iv_number_active_txns += gv_tm_info.ClusterRecov()->txnStateList()->size();

   pp_msg->response()->u.iv_statustm.iv_status.iv_is_isolated = gv_tm_info.leadTM_isolated();

   if (gv_RMs.TSE()->return_highest_index_used() == 0) {
      lp_rm = gv_RMs.TSE()->return_slot_by_index(0);
      if(lp_rm->in_use()) {
         pp_msg->response()->u.iv_statustm.iv_status.iv_rm_count = 1;
         lp_rm->copyto(&pp_msg->response()->u.iv_statustm.iv_status.ia_rminfo[0]);
      }
      else {
         pp_msg->response()->u.iv_statustm.iv_status.iv_rm_count =  0;
      }
   }
   else {
      pp_msg->response()->u.iv_statustm.iv_status.iv_rm_count = gv_RMs.TSE()->return_highest_index_used() + 1;
      for (int i=0; i<=gv_RMs.TSE()->return_highest_index_used(); i++)
      {
         lp_rm = gv_RMs.TSE()->return_slot_by_index(i);
         lp_rm->copyto(&pp_msg->response()->u.iv_statustm.iv_status.ia_rminfo[i]);
      }
   }

   pp_msg->reply(FEOK);
   delete pp_msg;
   TMTrace(2, ("tm_process_req_statustm EXIT.\n"));
} //tm_process_req_statustm


// -----------------------------------------------------------------
// tm_process_req_status_transmgmt
// Purpose : process message of type TM_MSG_TYPE_STATUS_TRANSMGMT to
// return the status of the transaction.
// ----------------------------------------------------------------
void tm_process_req_status_transmgmt(CTmTxMessage *pp_msg)
{
    TMTrace(2, ("tm_process_req_status_transmgmt ENTRY.\n"));
    TM_Txid_Internal *lp_transid = (TM_Txid_Internal *)
                                   &pp_msg->request()->u.iv_status_transm.iv_transid;
    TM_Transid lv_transid(*lp_transid);                               

    //should already be sent to the correct TM
    TM_TX_Info *lp_tx = (TM_TX_Info *)gv_tm_info.get_tx(lp_transid);
    if(!lp_tx) {
        pp_msg->reply(FENOTRANSID);
        delete pp_msg;
        return;
    }

    TM_Transid lv_fulltransid(*(lp_tx->transid()));                               

    lv_fulltransid.set_external_data_type(&pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_transid);
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_status               = lp_tx->tx_state();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_nid                  = lp_tx->node();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_seqnum               = lp_tx->seqnum();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_incarnation_num      = lv_transid.get_incarnation_num();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_tx_flags             = lv_transid.get_tx_flags();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_tt_flags             = lp_tx->TT_flags();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_owner_nid            = lp_tx->ender_nid();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_owner_pid            = lp_tx->ender_pid();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_event_count          = lp_tx->eventQ()->size();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_pendingRequest_count = lp_tx->PendingRequestQ()->size();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_num_active_partic    = lp_tx->num_active_partic();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_num_partic_RMs       = lp_tx->get_TSEBranchesParticCount();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_XARM_branch          = false;
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_transactionBusy      = lp_tx->transactionBusy();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_mark_for_rollback    = lp_tx->mark_for_rollback();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_tm_aborted           = lp_tx->tm_aborted();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_abort_flags          = lp_tx->abort_flags();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_read_only            = lp_tx->read_only();
    pp_msg->response()->u.iv_status_transm.iv_status_trans.iv_recovering           = lp_tx->recovering();

    pp_msg->reply(FEOK);
    delete pp_msg;
    TMTrace(2, ("tm_process_req_status_transmgmt EXIT.\n"));
} //tm_process_req_status_transmgmt

// -----------------------------------------------------------------
// tm_process_req_status_gettransinfo
// Purpose : process message of type TM_MSG_TYPE_GETTRANSINFO to
// return the trans ID information
// ----------------------------------------------------------------
void tm_process_req_status_gettransinfo(CTmTxMessage *pp_msg)
{
   TMTrace(2, ("tm_process_req_status_gettransinfo ENTRY.\n"));
   TM_Txid_Internal *lp_transid = (TM_Txid_Internal *)
                                   &pp_msg->request()->u.iv_status_transm.iv_transid;
   TM_Transid lv_transid(*lp_transid);
   union
   {
      int64 lv_tt_flags_int64;
      TM_TT_Flags lv_tt_flags;
   } u;
   
   //should already be sent to the correct TM
   TM_TX_Info *lp_tx = (TM_TX_Info *)gv_tm_info.get_tx(lp_transid);
   if(!lp_tx) {
      pp_msg->reply(FENOTRANSID);
      delete pp_msg;
      return;
   }
  
   TM_Transid lv_fulltransid(*(lp_tx->transid()));
   pp_msg->response()->u.iv_gettransinfo.iv_seqnum          = lp_tx->seqnum();
   pp_msg->response()->u.iv_gettransinfo.iv_node             = lp_tx->node();
   pp_msg->response()->u.iv_gettransinfo.iv_incarnation_num  = lv_fulltransid.get_incarnation_num();
   pp_msg->response()->u.iv_gettransinfo.iv_tx_flags         = lv_fulltransid.get_tx_flags();
   u.lv_tt_flags_int64                                       = lp_tx->TT_flags();
   pp_msg->response()->u.iv_gettransinfo.iv_tt_flags         = u.lv_tt_flags;
   pp_msg->response()->u.iv_gettransinfo.iv_version          = lv_fulltransid.get_version();
   pp_msg->response()->u.iv_gettransinfo.iv_checksum         = lv_fulltransid.get_check_sum();
   pp_msg->response()->u.iv_gettransinfo.iv_timestamp        = lv_fulltransid.get_timestamp();
  
   pp_msg->reply(FEOK);
   delete pp_msg;
   TMTrace(2, ("tm_process_req_status_gettransinfo EXIT.\n"));
} //tm_process_req_gettransinfo


// -----------------------------------------------------------------
// tm_process_req_leadtm
// Purpose : process message of type TM_MSG_TYPE_LEADTM to
// return the current Lead TMs nid.
// ----------------------------------------------------------------
void tm_process_req_leadtm(CTmTxMessage *pp_msg)
{
   TMTrace(2, ("tm_process_req_leadtm ENTRY.\n"));

   pp_msg->response()->u.iv_leadtm.iv_node = gv_tm_info.lead_tm_nid();

   pp_msg->reply(FEOK);
   delete pp_msg;
   TMTrace(2, ("tm_process_req_leadtm EXIT.\n"));
} //tm_process_req_leadtm


// -----------------------------------------------------------------
// tm_process_req_enabletrans
// Purpose : process message of type TM_MSG_TYPE_ENABLETRANS to
// enable transaction processing in DTM.
// This can only be executed by the Lead TM.  Non-lead TMs will 
// return FEDEVDOWN.
// ----------------------------------------------------------------
void tm_process_req_enabletrans(CTmTxMessage *pp_msg)
{
   short lv_error = FEOK;
   TMTrace(2, ("tm_process_req_enabletrans ENTRY.\n"));

   if (!gv_tm_info.lead_tm())
      lv_error = FEDEVDOWN;
   else
       switch (gv_tm_info.state())
       {
       case TM_STATE_QUIESCE:
       case TM_STATE_DRAIN:
         lv_error = FEINVALOP;
         break;
       case TM_STATE_TX_DISABLED:
       default:
         // Queue the enabletransaction to the timer thread for execution.
         gv_tm_info.addTimerEvent(pp_msg, 0 /*execute now*/);
       }

   // Reply immediately and leave the enable to run in the background.
   pp_msg->reply(lv_error);
   delete pp_msg;
   TMTrace(2, ("tm_process_req_enabletrans EXIT.\n"));
} //tm_process_req_enabletrans


// -----------------------------------------------------------------
// tm_process_req_disabletrans
// Purpose : process message of type TM_MSG_TYPE_DISABLETRANS to
// disable transaction processing in DTM.
// This can only be executed by the Lead TM.  Non-lead TMs will 
// return FEDEVDOWN.
// ----------------------------------------------------------------
void tm_process_req_disabletrans(CTmTxMessage *pp_msg)
{
   short lv_error = FEOK;
   char  lv_levelStr[20],
        *lp_levelStr = (char *) &lv_levelStr;
   switch (pp_msg->request()->u.iv_disabletrans.iv_shutdown_level)
   {
   case TM_DISABLE_SHUTDOWN_IMMEDIATE:
       strcpy(lp_levelStr, "Immediate");
       break;
   case TM_DISABLE_SHUTDOWN_NORMAL:
       strcpy(lp_levelStr, "Normal");
       break;
   default:
       strcpy(lp_levelStr, "** Invalid **");
   }
   TMTrace(2, ("tm_process_req_disabletrans ENTRY, level %s.\n", lp_levelStr));

   if (!gv_tm_info.lead_tm())
      lv_error = FEDEVDOWN;
   else
      if (gv_tm_info.state() == TM_STATE_UP ||
          gv_tm_info.state() == TM_STATE_TX_DISABLED ||
          (gv_tm_info.state() == TM_STATE_TX_DISABLED_SHUTDOWN_PHASE1 &&
           pp_msg->request()->u.iv_disabletrans.iv_shutdown_level == TM_DISABLE_SHUTDOWN_IMMEDIATE) ||
          gv_tm_info.state() == TM_STATE_DRAIN)
      {
         // For disabletrans normal shutdown, only queue the disabletransaction if all TMs have recovered.
         if (pp_msg->request()->u.iv_disabletrans.iv_shutdown_level != TM_DISABLE_SHUTDOWN_NORMAL ||
             gv_tm_info.all_tms_recovered())
            gv_tm_info.addTimerEvent(pp_msg, 0 /*execute now*/);
         else
            lv_error = FERETRY;
      }
      else
         lv_error = FEINVALOP;

   // Reply immediately and leave the disable to run in the background.
   pp_msg->reply(lv_error);
   delete pp_msg;
   TMTrace(2, ("tm_process_req_disabletrans EXIT, replied with error %d.\n", lv_error));
} //tm_process_req_disabletrans


// -----------------------------------------------------------------
// tm_process_req_draintrans
// Purpose : process message of type TM_MSG_TYPE_DRAINTRANS to
// drain transaction processing in this TM.
// This can be executed in any TM.  It is used to allow transactions
// to complete before a planned node outage.
// Immediate means abort all active transactions and overrides a 
// prior drain
// ----------------------------------------------------------------
void tm_process_req_draintrans(CTmTxMessage *pp_msg)
{
   short lv_error = FEOK;
   TMTrace(2, ("tm_process_req_draintrans ENTRY immediate=%d.\n", 
       pp_msg->request()->u.iv_draintrans.iv_immediate));

   if (gv_tm_info.state() == TM_STATE_UP ||
       gv_tm_info.state() == TM_STATE_DRAIN)
      gv_tm_info.drainTrans(pp_msg);
   else
      lv_error = FEINVALOP;

   // Reply immediately and leave the drain to run in the background.
   pp_msg->reply(lv_error);
   delete pp_msg;
   TMTrace(2, ("tm_process_req_draintrans EXIT error=%d.\n", lv_error));
} //tm_process_req_draintrans


// -----------------------------------------------------------------
// tm_process_req_status
// Purpose : process message of type TM_MSG_TYPE_STATUSTRANSACTION
// ----------------------------------------------------------------
void tm_process_req_status(CTmTxMessage * pp_msg) 
{
    TM_Txid_Internal *lp_transid = (TM_Txid_Internal *)
                                   &pp_msg->request()->u.iv_status_trans.iv_transid;

   TMTrace(2, ("tm_process_req_status, ID %d, ENTRY\n", 
                     lp_transid->iv_seq_num));

    if (!tm_up_check(pp_msg))
       return;

    TM_TX_Info *lp_tx = (TM_TX_Info *)gv_tm_info.get_tx(lp_transid);

    pp_msg->response()->u.iv_status_trans.iv_status = TM_TX_STATE_NOTX;

    if (!tm_notx_check(lp_tx, pp_msg))
       return;

    // Handle status request in main thread to avoid status
    // getting queued behind other requests.
    lp_tx->req_status(pp_msg);
    // Since we don't queue status requests, we can delete pp_msg here itself.
    delete pp_msg;
    TMTrace(2, ("tm_process_req_status EXIT\n"));
}

// ---------------------------------------------------------------
// process_req_suspend_trans
// Purpose : process request of type TM_MSG_TYPE_SUSPENDTRANSACTION
// ---------------------------------------------------------------
void tm_process_req_suspend_trans (CTmTxMessage * pp_msg)
{
    TM_Txid_Internal *lp_transid = (TM_Txid_Internal *)
                                   &pp_msg->request()->u.iv_suspend_trans.iv_transid;

    TMTrace(2, ("tm_process_req_suspend_trans, ID %d, ENTRY\n",
                  lp_transid->iv_seq_num));

    if (!tm_up_check(pp_msg))
        return;
     
    TM_TX_Info *lp_tx = (TM_TX_Info *)gv_tm_info.get_tx(lp_transid);

    if (!tm_notx_check(lp_tx, pp_msg))
        return;

    // Call suspend in-line in main thread
    lp_tx->req_suspend(pp_msg);
    // Since we don't queue suspend requests, we can delete pp_msg here itself.
    delete pp_msg;

    TMTrace(2, ("tm_process_req_suspend_trans EXIT\n"));
}


// -----------------------------------------------------------------
// tm_process_req_broadcast
// Purpose - process a broadcast for sync data
// ----------------------------------------------------------------
void tm_process_req_broadcast (BMS_SRE *pp_sre,
         Tm_Broadcast_Req_Type *pp_req, Tm_Broadcast_Rsp_Type *pp_rsp)
{
    TMTrace(2, ("tm_process_req_broadcast for node %d ENTRY\n",
                   pp_req->iv_node));

    ushort lv_len = sizeof(Tm_Broadcast_Rsp_Type);

    gv_tm_info.unpack_sync_buffer (pp_req, pp_req->iv_node);
    if (pp_req->iv_state_up)  // last one, can be considered up
    {
        gv_tm_info.can_takeover(true);
        gv_tm_info.tm_up(); // up for processing
    }
    
    XMSG_REPLY_(pp_sre->sre_msgId,       // msgid
                NULL,                    // replyctrl
 
               0,                       // replyctrlsize
                (char *) pp_rsp,         // replydata
                lv_len,                  // replydatasize
                0,                       // errorclass
                NULL);                   // newphandle
    TMTrace(2, ("tm_process_req_broadcast EXIT\n")); 
}
// --------------------------------------------------------------------
// ax_* methods
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// tm_process_req_ax_reg
// Purpose : process message of type TM_MSG_TYPE_AX_REG
// --------------------------------------------------------------------
void tm_process_req_ax_reg (CTmTxMessage * pp_msg) 
{
   short lv_error = FEOK;
   int lv_ptype = -1;
   int lv_nid = -1;
   int lv_pid = -1;
   int lv_seq_num = 0;
   int32 lv_rmid = pp_msg->request()->u.iv_ax_reg.iv_rmid;
   TM_Txid_Internal *lp_transid = (TM_Txid_Internal *)
                                 &pp_msg->request()->u.iv_ax_reg.iv_txid;

   TMTrace(2, ("tm_process_req_ax_reg, ID (%d,%d), ENTRY msgid %d\n",
                   lp_transid->iv_node, lp_transid->iv_seq_num, pp_msg->msgid()));

   // Removed check here because ax_reg needs to work during system recovery in M6.
   //if (!tm_up_check(pp_msg))
   //    return; 

   TM_TX_Info *lp_tx = (TM_TX_Info *)gv_tm_info.get_tx(lp_transid);

   // sent to the wrong TM or this tx never existed or has been forgotten.
   if (lp_tx == NULL)
   {
      pp_msg->response()->u.iv_ax_reg.iv_TM_incarnation_num = gv_tm_info.incarnation_num();
      pp_msg->response()->u.iv_ax_reg.iv_LeadTM_nid = gv_tm_info.lead_tm_nid();


      if (pp_msg->request()->u.iv_ax_reg.iv_flags & TM_TT_NO_UNDO)
         lv_error = FEWRONGID;
      else
         lv_error = FEINVTRANSID;

      TMTrace(3, ("tm_process_req_ax_reg, ID (%d,%d) from RM %d not found in transactionPool - "
              "redirecting TSE to Lead TM,  error %d.\n",
              lp_transid->iv_node, lp_transid->iv_seq_num, lv_rmid, lv_error));
      //tm_log_event(DTM_TM_NO_TRANS, SQ_LOG_WARNING, "DTM_TM_NO_TRANS",
      //             lv_error,lv_rmid,lp_transid->iv_node,lp_transid->iv_seq_num);

      pp_msg->reply(lv_error);
      delete pp_msg;
      return;
   }
   lp_tx->stats()->ax_reg()->start();

   // The TSE doesn't always know its rmid, so we can't rely on that.
   // Instead we lookup the RM in out list.
   if (lv_rmid == -1 || lv_rmid == 0)
   {
      lv_error = BMSG_GETREQINFO_(MSGINFO_PTYPE, pp_msg->msgid(), &lv_ptype);
      if (!lv_error && lv_ptype == MS_ProcessType_TSE)
      {
         lv_error = BMSG_GETREQINFO_(MSGINFO_NID, pp_msg->msgid(), &lv_nid);
         if (!lv_error)
            lv_error = BMSG_GETREQINFO_(MSGINFO_PID, pp_msg->msgid(), &lv_pid);
         if (lv_error)
         {
            TMTrace(1, ("tm_process_req_ax_reg, Error %d retrieving nid "
                    "(%d) and pid (%d) for TSE.  ax_reg ignored.\n",
                    lv_error, lv_nid, lv_pid));
            tm_log_event(DTM_AX_REG_NID_PID_BAD, SQ_LOG_CRIT, "DTM_AX_REG_NID_PID_BAD", 
                lv_error, lv_rmid, -1, -1, pp_msg->msgid(), -1, -1, -1, -1, -1, -1, 
                -1, -1, -1, lv_pid, -1, NULL, lv_nid);
            pp_msg->reply(FENOTFOUND);
            delete pp_msg;
            return;
         }

         lv_rmid = gv_RMs.TSE()->return_rmid(lv_nid, lv_pid);

         if (lv_rmid == -1)
          {
            TMTrace(1, ("tm_process_req_ax_reg, RM not found in RM list. "
                    "ax_reg ignored.\n"));
            tm_log_event(DTM_AX_REG_NOTFOUND, SQ_LOG_CRIT, "DTM_AX_REG_NOTFOUND", 
                -1, lv_rmid, -1, -1, pp_msg->msgid(), -1, -1, -1, -1, -1, -1, 
                -1, -1, -1, lv_pid, -1, NULL, lv_nid);
            pp_msg->reply(FENOTFOUND);
            delete pp_msg;
            return;
         }
         else  
            TMTrace(3, ("tm_process_req_ax_reg, TSE ax_reg for rmid %d, TSE (%d, %d).\n",
                    lv_rmid, lv_nid, lv_pid));
      }
      else // Not TSE or error
      {
         if (!lv_error)
            lv_error = BMSG_GETREQINFO_(MSGINFO_NID, pp_msg->msgid(), &lv_nid);
         if (!lv_error)
            lv_error = BMSG_GETREQINFO_(MSGINFO_PID, pp_msg->msgid(), &lv_pid);
         if (lv_error)
          {
            TMTrace(1, ("tm_process_req_ax_reg, Error %d retrieving PTYPE (%d), "
                    "nid (%d) or pid (%d). ax_reg ignored!\n",
                    lv_error, lv_ptype, lv_nid, lv_pid));
            tm_log_event(DTM_AX_REG_PTYPE_BAD, SQ_LOG_CRIT, "DTM_AX_REG_PTYPE_BAD", 
                lv_error,-1,-1,-1,pp_msg->msgid(),-1,-1,-1,-1,-1,-1,-1,-1,lv_pid,
                lv_ptype,-1,NULL,lv_nid);
            pp_msg->reply(FENOTFOUND);
            delete pp_msg;
            return;
         }
         else // Not an error - ax_reg from XARM library and should contain the rmid.
              // but not yet implemented!
         {
            TMTrace(1, ("tm_process_req_ax_reg, Received unexpected ax_reg from non-TSE"
                    " process (%d, %d), PTYPE %d assuming this was an XARM request!?, ignored!\n",
                    lv_nid, lv_pid, lv_ptype));
            tm_log_event(DTM_AX_REG_XARM_NOTSUPPORTED, SQ_LOG_CRIT, "DTM_AX_REG_XARM_NOTSUPPORTED", 
                -1,pp_msg->request()->u.iv_ax_reg.iv_rmid,-1,pp_msg->msgid(),
                -1,-1,-1,-1,-1,-1,-1,-1,lv_pid,lv_ptype,0,lv_nid);
            pp_msg->reply(FENOTFOUND);
            delete pp_msg;
            return;
  }
      }
   }

   // Save the rmid back in the message
   pp_msg->request()->u.iv_ax_reg.iv_rmid = lv_rmid;
   // Call directly in the main thread to improve performance.
   //lp_tx->queueToTransaction(lp_transid, pp_msg);
   lp_tx->req_ax_reg(pp_msg);
   lv_seq_num = lp_transid->iv_seq_num;
   delete pp_msg;
   lp_tx->stats()->ax_reg()->stop();


   TMTrace(2, ("tm_process_req_ax_reg, ID %d, EXIT\n", lv_seq_num));
} //tm_process_req_ax_reg


// --------------------------------------------------------------------
// tm_process_req_ax_unreg
// Purpose : process message of type TM_MSG_TYPE_AX_UNREG
// --------------------------------------------------------------------
void tm_process_req_ax_unreg (CTmTxMessage * pp_msg) 
{
 
  TMTrace(2, ("tm_process_req_ax_unreg ENTRY\n"));

  // sorry, not implemented right now! 
  pp_msg->reply(FEOK);
  delete pp_msg;

  TMTrace(2, ("tm_process_req_ax_unreg EXIT\n"));
}

// ------------------------------------------------------------------
// callback methods and processing downline from callbacks
// ------------------------------------------------------------------

// ------------------------------------------------------------------
// tm_sync_cb
// Purpose : this method is registered with seabed and is used when
//           a sync is received (Phase 1)
// ------------------------------------------------------------------
int32 tm_sync_cb (void *pp_data, int32 pv_len , int32 pv_handle)
{
    Tm_Sync_Header  *lp_hdr = (Tm_Sync_Header*)pp_data;
    Tm_Sync_Data    *lp_sync_data = new Tm_Sync_Data;
    Tm_Sync_Data    *lp_data = (Tm_Sync_Data *)pp_data;

    pv_len = pv_len; // intel compiler warning 869

    if (pp_data == NULL)
    {
        tm_log_event(DTM_SYNC_INVALID_DATA, SQ_LOG_CRIT, "DTM_SYNC_INVALID_DATA");
        TMTrace(1, ("tm_sync_cb : data is invalid\n"));
        abort ();
    }

    TMTrace(2, ("tm_sync_cb ENTRY : type %d\n", lp_hdr->iv_type));
       
    // allow duplicates per Charles
    Tm_Sync_Data *lp_existing_data = (Tm_Sync_Data *)gv_sync_map.get(pv_handle);
    if (lp_existing_data != NULL)
    {
        delete lp_sync_data;
        return 0;
    }
    
    switch (lp_hdr->iv_type)
    {
        case TM_BEGIN_SYNC:
        case TM_END_SYNC:
        case TM_FORGET_SYNC:
        {
#ifdef DEBUG_MODE
            bool lv_test = false;
            ms_getenv_bool("TM_TEST_SINGLE_FORCE_ABORT", &lv_test);
            if (lv_test)
            {
            //   sprintf(la_buf, "TM Test: Force Abort\n");
            //   tm_log_write(DTM_TM_TEST_FORCE_ABORT, SQ_LOG_CRIT, la_buf);
             //  TMTrace(1, ("tm_sync_cb - %s", la_buf));
               abort ();
            }
#endif
            if (lp_data->u.iv_tx_data.iv_pid <= 0)
            {
               tm_log_event (DTM_SYNC_INVALID_PID, SQ_LOG_CRIT, "DTM_SYNC_INVALID_PID",
                     -1, /*error_code*/ 
                    -1, /*rmid*/
                    -1, /*dtmid*/ 
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
                    lp_data->u.iv_tx_data.iv_pid); /*data */

               TMTrace(1, ("tm_sync_cb - Invalid sync PID: %d\n", lp_data->u.iv_tx_data.iv_pid));
               abort ();
            }
            if (lp_data->u.iv_tx_data.iv_transid.id[0] <= 0)
            {
               tm_log_event (DTM_SYNC_INVALID_TRANSID, SQ_LOG_CRIT, "DTM_SYNC_INVALID_TRANSID",
                    -1, /*error_code*/ 
                    -1, /*rmid*/
                    -1, /*dtmid*/ 
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
                    -lp_data->u.iv_tx_data.iv_transid.id[0]);/*data2 */
               TMTrace(1, ("tm_sync_cb - Invalid sync Trans ID: " PFLL "\n", 
                           lp_data->u.iv_tx_data.iv_transid.id[0]));
               abort ();
            }
            break;
        }
        case TM_UP:
        {
            break; 
        }
        case TM_STATE_RESYNC:
        {
             TMTrace(3, ("tm_sync_cb - TM_STATE_RESYNC received \n"));
             // nothing to validate since these are booleans and the node
             // being recovered could be a -1
             break;
        }
        case TM_RECOVERY_START:
        case TM_RECOVERY_END:
        {
            if ((lp_data->u.iv_to_data.iv_my_node < 0) ||
                 (lp_data->u.iv_to_data.iv_my_node > MAX_NODES) ||
                 (lp_data->u.iv_to_data.iv_down_node < 0) ||
                 (lp_data->u.iv_to_data.iv_down_node > MAX_NODES))
            {
                tm_log_event(DTM_TM_NODE_OUTSIDE_RANGE, SQ_LOG_CRIT, "DTM_TM_NODE_OUTSIDE_RANGE");
                TMTrace(1, ("tm_sync_cb - Received RECOVERY sync with node out of range.\n"));
                abort ();
            }


#ifdef DEBUG_MODE
           bool lv_assert = false;
           ms_getenv_bool("TM_TEST_AFTER_REC_START_SYNC_ASSERT", &lv_assert);
           if (lv_assert == true)
           {
              // sprintf(la_buf, "TM Test: Rec start sync assert\n");
            //   tm_log_write(DTM_TM_TEST_REC_START_SYNC, SQ_LOG_CRIT, la_buf);
              // TMTrace(1, ("tm_sync_cb - %s", la_buf));
               abort ();
           }
           
#endif 
             break;
        }
        case TM_LISTBUILT_SYNC:
        {
            if ((lp_data->u.iv_list_built.iv_down_node < 0) ||
                 (lp_data->u.iv_list_built.iv_down_node > MAX_NODES))
            {
                tm_log_event(DTM_TM_NODE_OUTSIDE_RANGE, SQ_LOG_CRIT, "DTM_TM_NODE_OUTSIDE_RANGE");
                TMTrace(1, ("tm_sync_cb - Received TM_LISTBUILT_SYNC sync with node out of range\n"));
                abort ();
            }
            TMTrace(1, ("tm_sync_cb - received TM_LISTBUILT_SYNC, verification successful\n"));
            break; 
        }
        case TM_PROCESS_RESTART:
            break;
        case TM_SYS_RECOV_START_SYNC:
        case TM_SYS_RECOV_END_SYNC:
        {
            if ((lp_data->u.iv_sys_recov_data.iv_sys_recov_state > TM_SYS_RECOV_STATE_END) ||
                 (lp_data->u.iv_sys_recov_data.iv_sys_recov_lead_tm_node < 0) ||
                 (lp_data->u.iv_sys_recov_data.iv_sys_recov_lead_tm_node > MAX_NODES))
            {
                tm_log_event(DTM_TM_NODE_OUTSIDE_RANGE, SQ_LOG_CRIT, "DTM_TM_NODE_OUTSIDE_RANGE");
                TMTrace(1, ("tm_sync_cb - Received RECOVERY sync with node out of range\n"));
                abort ();
            }

            break;
        }
        default:
        {
            tm_log_event(DTM_TM_UNKNOWN_SYNC_TYPE, SQ_LOG_CRIT, "DTM_TM_UNKNOWN_SYNC_TYPE");
            TMTrace(1, ("tm_sync_cb - Unknown sync header type received\n"));
            abort ();
            break;
        }
    }; 

    memcpy (lp_sync_data, lp_data, sizeof (Tm_Sync_Data));
    gv_sync_map.put(pv_handle, lp_sync_data);

    TMTrace(2, ("tm_sync_cb EXIT : type %d\n", lp_hdr->iv_type));

    return 0;
}


void tm_recipient_sync_commit (Tm_Sync_Data *pp_sync_data)
{
    TMTrace(2, ("tm_recipient_sync_commit : ENTRY, type %d\n",
                                    pp_sync_data->iv_hdr.iv_type));
    switch (pp_sync_data->iv_hdr.iv_type)
    {
        case TM_BEGIN_SYNC:
        {
            gv_tm_info.add_sync_data(pp_sync_data->iv_hdr.iv_nid, 
                                     &pp_sync_data->u.iv_tx_data);
            gv_system_tx_count++;
            break;  
        }
        case TM_END_SYNC:
        {

            Tm_Tx_Sync_Data *lp_data = gv_tm_info.get_sync_data(
                                       &pp_sync_data->u.iv_tx_data);
            // Add sync data to the sync data list if
            // it isn't already in the list for the sending node.
            if (lp_data == NULL)
               gv_tm_info.add_sync_data(pp_sync_data->iv_hdr.iv_nid, 
                                      &pp_sync_data->u.iv_tx_data);
            else
               lp_data->iv_state = pp_sync_data->u.iv_tx_data.iv_state;

            break;
        }
        case TM_FORGET_SYNC:
        {
            gv_tm_info.remove_sync_data(&pp_sync_data->u.iv_tx_data);
            break;
        }
        case TM_STATE_RESYNC:
        {

            TMTrace(3, ("tm_recipient_sync_commit - TM_STATE_RESYNC sync received.\n"));
            gv_tm_info.node_being_recovered(pp_sync_data->u.iv_state_resync.iv_index,
                                            pp_sync_data->u.iv_state_resync.iv_node_being_recovered);
            gv_tm_info.down_without_sync( pp_sync_data->u.iv_state_resync.iv_index,
                                            pp_sync_data->u.iv_state_resync.iv_down_without_sync);
            gv_tm_info.recovery_list_built( pp_sync_data->u.iv_state_resync.iv_index,
                                            pp_sync_data->u.iv_state_resync.iv_list_built);
            break;
        }
        case TM_RECOVERY_START:
        {
            tm_log_event(DTM_TM_START_NODE_RECOVERY, SQ_LOG_INFO, "DTM_TM_START_NODE_RECOVERY");
            TMTrace(1, ("tm_recipient_sync_commit - RECOVERY START sync received.\n"));
            // The lead TM can not receive this sync. Issue an event and shutdown the cluster
            if (gv_tm_info.lead_tm())
            {
               tm_log_event(DTM_LEAD_TM_TM_SYNC_UNEXPECTED, SQ_LOG_CRIT, "DTM_LEAD_TM_TM_SYNC_UNEXPECTED", FEDUP);
               TMTrace(1, ("tm_recipient_sync_recipient : Error TM_RECOVERY_START sync received by Lead TM.\n"));
               gv_tm_info.error_shutdown_abrupt(FEDUP);
            }
            if (pp_sync_data->u.iv_to_data.iv_down_node == -1)
            {
                tm_log_event(DTM_TM_START_NODE_RECOVERY, SQ_LOG_CRIT, "DTM_TM_START_NODE_RECOVERY");
                TMTrace(1, ("tm_recipient_sync_commit - Invalid node id received for a RECOVERY START sync\n"));
                abort ();
            }
               
            gv_tm_info.node_being_recovered (
                       pp_sync_data->u.iv_to_data.iv_down_node,
                       pp_sync_data->u.iv_to_data.iv_my_node);
            gv_tm_info.down_without_sync(pp_sync_data->u.iv_to_data.iv_down_node, false);
            TMTrace(3, ("tm_recipient_sync_commit - setting down_without_sync to FALSE for node %d\n",
                          pp_sync_data->u.iv_to_data.iv_down_node));
            gv_tm_info.schedule_init_and_recover_rms();
            break;
        }
        case TM_RECOVERY_END:
        {
            tm_log_event(DTM_TM_END_NODE_RECOVERY, SQ_LOG_INFO, "DTM_TM_END_NODE_RECOVERY");
            TMTrace(1, ("tm_recipient_sync_commit - RECOVERY END sync received.\n"));
            if (pp_sync_data->u.iv_to_data.iv_down_node == -1)
            {
                tm_log_event(DTM_TM_END_NODE_RECOVERY, SQ_LOG_CRIT, "DTM_TM_END_NODE_RECOVERY");
                TMTrace(1, ("tm_recipient_sync_commit - Invalid node id received for a RECOVERY END sync.\n"));
                abort ();
            }
                
            //reset
            TMTrace(3, ("tm_recipient_sync_commit setting recovery_list_built to FALSE for Node " 
                         " %d\n",pp_sync_data->u.iv_to_data.iv_down_node));
            // reset list built flag for recovery
            gv_tm_info.recovery_list_built (pp_sync_data->u.iv_to_data.iv_down_node, false);
            gv_tm_info.node_being_recovered (pp_sync_data->u.iv_to_data.iv_down_node, -1);
            gv_tm_info.set_sys_recov_status(TM_SYS_RECOV_STATE_END,
                        pp_sync_data->u.iv_to_data.iv_my_node); //my node sb the lead tm
            gv_tm_info.tm_up();
            break;
        }
        case TM_LISTBUILT_SYNC:
        {
           tm_log_event(DTM_TM_LISTBUILT_SYNC, SQ_LOG_INFO, "DTM_TM_LISTBUILT_SYNC");
            TMTrace(3, ("tm_recipient_sync_commit (TM_LISTBUILT_SYNC) setting recovery_list_built " 
                        " to TRUE for Node %d\n",pp_sync_data->u.iv_list_built.iv_down_node));
            gv_tm_info.recovery_list_built (pp_sync_data->u.iv_list_built.iv_down_node, true);
            break;
        }
        case TM_PROCESS_RESTART:
           tm_log_event(DTM_TM_PROCESS_RESTART_SYNC, SQ_LOG_INFO, "DTM_TM_PROCESS_RESTART_SYNC");
           TMTrace(1, ("tm_recipient_sync_commit - process restart sync received.\n"));

           gv_tm_info.restarting_tm(pp_sync_data->u.iv_proc_restart_data.iv_proc_restart_node);
           gv_tm_info.schedule_init_and_recover_rms();
           break;
        case TM_SYS_RECOV_START_SYNC:
        case TM_SYS_RECOV_END_SYNC:
        {
            gv_tm_info.set_sys_recov_status(pp_sync_data->u.iv_sys_recov_data.iv_sys_recov_state,
                        pp_sync_data->u.iv_sys_recov_data.iv_sys_recov_lead_tm_node);
            break;
        }       
        case TM_UP:
        {
            // this is received upon startup after recovery, so its a fresh system
            gv_tm_info.can_takeover(true);
            gv_tm_info.tm_up();
            break;
        }
        default:
        {
            tm_log_event(DTM_TM_UNKNOWN_SYNC_TYPE, SQ_LOG_CRIT, "DTM_TM_UNKNOWN_SYNC_TYPE");
            TMTrace(1, ("tm_recipient_sync_commit : invalid data\n"));

            abort ();
            break;
        }
    };
    TMTrace(2, ("tm_recipient_sync_commit EXIT \n"));
}

// --------------------------------------------------------------------
// tm_get_leader_info
// Purpose : Get the new tm leader. 
// --------------------------------------------------------------------
void tm_get_leader_info()
{
    // Nothing to do here if we are already the Lead TM.
    if (gv_tm_info.lead_tm() == true)
         return;

    int32 lv_leader_nid, lv_leader_pid;
    char la_leader_name[BUFSIZ]; 
    int32 lv_old_leader_nid = gv_tm_info.lead_tm_nid();

    int lv_leader_error = msg_mon_tm_leader_set(&lv_leader_nid, 
                                            &lv_leader_pid, la_leader_name);
    // ignore error as it simply indicates that we are not the leader.
    if (lv_leader_error)
    {
        TMTrace(3, ("tm_get_leader_info : Error %d returned by "
                "msg_mon_tm_leader_set - $TM%d is not the Lead. Error ignored.\n", 
                lv_leader_error, gv_tm_info.nid()));
    }
    gv_tm_info.lead_tm_nid(lv_leader_nid);

    if (lv_leader_nid != lv_old_leader_nid)
    {
         tm_log_event (DTM_TM_LEADTM_SET, SQ_LOG_INFO , "DTM_TM_LEADTM_SET", 
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
                    lv_old_leader_nid, /*data */
                    -1, /*data1*/
                    -1,/*data2 */
                    NULL, /*string2*/
                    lv_leader_nid /*node*/);
         TMTrace(3, ("tm_get_leader_info : Node %d is new Lead DTM.\n", lv_leader_nid));

         if (lv_leader_nid == gv_tm_info.nid())
         {
            // modify the wait interval now for this lead dtm
            gv_wait_interval = LEAD_DTM_WAKEUP_INTERVAL/10;
            gv_tm_info.lead_tm(true);
            gv_tm_info.lead_tm_takeover(true);
            gv_tm_info.open_other_tms();
            // Add a Checkpoint event to drive cp processing
//            gv_tm_info.tmTimer()->cancelControlpointEvent();
//            gv_tm_info.tmTimer()->addControlpointEvent(gv_tm_info.cp_interval());
            // Add a stats event
            gv_tm_info.tmTimer()->cancelStatsEvent();
            gv_tm_info.tmTimer()->addStatsEvent(gv_tm_info.stats_interval());
         }

    }
    else
    {
         TMTrace(3, ("tm_get_leader_info : Lead DTM did not change. Node %d is still Lead DTM.\n", 
                 lv_leader_nid));
    }

} //tm_get_leader_info

#ifdef SUPPORT_TM_SYNC
//---------------------------------------------------------------------
// tm_originating_sync_commit
// Purpose - helper method to process the phase2 sync from the 
//           originating TM
// --------------------------------------------------------------------
void tm_originating_sync_commit (int32 pv_tag)
{
    CTmTxBase *lp_tx             = NULL;
    Tm_Sync_Type_Transid *lp_data = gv_tm_info.get_sync_otag(pv_tag);
    CTmTxMessage *lp_msg;

    // assert (lp_data != NULL);
    if (lp_data == NULL)
    {
       TMTrace(1, ("tm_originating_sync_commit : ERROR tag %d not found in sync tags, sync ignored.\n",
                     pv_tag));
        return;
    }

    TMTrace(2, ("tm_originating_sync_commit ENTRY, ID %d, tag %d, type %d.\n",
                     lp_data->u.iv_seqnum, pv_tag, lp_data->iv_sync_type));

    switch(lp_data->iv_sync_type)
    {
        case TM_END_SYNC:
        case TM_BEGIN_SYNC:
        {
            lp_tx = (CTmTxBase *) gv_tm_info.get_tx(lp_data->u.iv_node_to_takeover, lp_data->u.iv_seqnum);

            if (lp_tx == NULL)
            {
                tm_log_event(DTM_TM_INVALID_TRANSACTION, SQ_LOG_CRIT, "DTM_TM_INVALID_TRANSACTION");
                TMTrace(1, ("tm_originating_sync_commit : END/BEGIN SYNC - Unable to find "
                            " transaction during a phase 2 sync\n"));
                abort ();
            }
            
            TMTrace(3, ("tm_originating_sync_commit : END/BEGIN SYNC "
                              "for ID %d, Tx.SeqNum %d, Tx.Tag %d, tag %d, type %d.\n",
                              lp_data->u.iv_seqnum, lp_tx->seqnum(), lp_tx->tag(), pv_tag, lp_data->iv_sync_type));
            lp_tx->schedule_eventQ(); 
            break;
         }
         case TM_FORGET_SYNC:
         {
            // we are done, queue endforget event against txn thread.
            lp_tx = (CTmTxBase *) gv_tm_info.get_tx(lp_data->u.iv_node_to_takeover, lp_data->u.iv_seqnum);

            // If the transaction object doesn't exist we assume that it's already been
            // cleaned up.  This can happen, for example, when a begin or end sync completion
            // arrives after we've already issued the forget sync.  TM_TX_Info::schedule_eventQ
            // will drive forget processing because the transaction is in forgotten state.
            // In this case the best thing we can do is simply through away the forget sync
            // completion here.
            if (lp_tx == NULL)
            {
                tm_log_event(DTM_TM_INVALID_TRANSACTION, SQ_LOG_CRIT, "DTM_TM_INVALID_TRANSACTION");
                TMTrace(1, ("tm_originating_sync_commit : FORGET_SYNC - WARNING "
                            "Unable to find transaction for a phase 2 "
                            "forget sync completion.  Completion assumed out of order and ignored!\n"));
                abort ();
            }            
            else
            {
               TMTrace(3, ("tm_originating_sync_commit : FORGET_SYNC "
                              "for ID %d, Tx.SeqNum %d, Tx.Tag %d, tag %d, type %d.\n",
                              lp_data->u.iv_seqnum, lp_tx->seqnum(), lp_tx->tag(), pv_tag, lp_data->iv_sync_type));

               lp_msg = lp_tx->currRequest();
               if (lp_msg)
               {
                  lp_msg->requestType(TM_MSG_TXINTERNAL_ENDFORGET);
                  lp_tx->eventQ_push(lp_msg);
               }
               else
               {
                  tm_log_event(DTM_TM_INVALID_TRANSACTION, SQ_LOG_CRIT, "DTM_TM_INVALID_TRANSACTION");
                  TMTrace(1, ("tm_originating_sync_commit : FORGET_SYNC - Forget Sync phase 2 for transaction "
                         "%d but request "
                        "has already completed!  Forget ignored.\n", lp_tx->seqnum()));
               }            
            }
            break;
          }
          case TM_STATE_RESYNC:
          {
              TMTrace(1, ("tm_originating_sync_commit, TM_STATE_RESYNC received, no-op\n"));
              break;
          }
          case TM_RECOVERY_START:
          {       
#ifdef DEBUG_MODE
             bool lv_verify = false;
             ms_getenv_bool("TM_VERIFY", &lv_verify);
             if (lv_verify)
             {
                if (gv_tm_info.iv_trace_level)
                {
                   if (gv_tm_info.tm_test_verify(lp_data->u.iv_node_to_takeover))
                       trace_printf("tm_verify after takeover successful\n");
                   else
                       trace_printf("tm_verify after takeover ERROR\n");
                }
             }
#endif
             break;
          }
          case TM_RECOVERY_END:
          {   
               break;
          }
          case TM_LISTBUILT_SYNC:
          {
               // We don't need to do anything in the originating TM since we've already 
               // recorded the appropriate flags.
               TMTrace(3, ("tm_originating_sync_commit : received TM_LISTBUILT_SYNC, no-op.\n"));
               break;
          }
          case TM_UP:
          {
              // set registry entry to indicate that transaction service is ready.
              gv_tm_info.set_txnsvc_ready(TXNSVC_UP);
              break;
              
           }
          case TM_PROCESS_RESTART:
              break;
           case TM_SYS_RECOV_START_SYNC:
           {
               // software fault
               if (!gv_tm_info.ClusterRecov())
               {
                    tm_log_event(DTM_RECOVERY_FAILED2, SQ_LOG_CRIT, "DTM_RECOVERY_FAILED2",
                                  -1,-1,gv_tm_info.nid());
                    abort(); // this is a software fault that doesn't warrant taking
                             // down the cluster
               }

               // System recovery now runs in the timer thread to keep the main thread from blocking.
               gv_tm_info.schedule_recover_system();      
               break;
           }
           case TM_SYS_RECOV_END_SYNC:
           {
               // Lead TM: Send out TM_UP sync and then set TM_UP to allow new transactions to be processed.
               send_state_up_sync(gv_tm_info.nid());
               gv_tm_info.tm_up();
               gv_tm_info.can_takeover(true);
               break;
           }
           default:
           {
               // TODO
               break;
            } 
       };
    
    TMTrace(2, ("tm_originating_sync_commit EXIT, TxnId %d, sync type %d\n",
                     lp_data->u.iv_seqnum, lp_data->iv_sync_type));
    gv_tm_info.remove_sync_otag(pv_tag);
}

void tm_originating_sync_abort(int32 pv_tag)
{
     CTmTxBase *lp_tx             = NULL;
     Tm_Sync_Type_Transid *lp_data = gv_tm_info.get_sync_otag(pv_tag);


     // we need to allow this to not be here as the monitor can choose the abort...
     if (lp_data == NULL)
     {
          TMTrace(1, ("tm_originating_sync_abort : NULL data for tag %d\n", pv_tag));
     }
     else
     {
         TMTrace(2, ("tm_originating_sync_abort ENTRY, tag=%d, type=%d\n", pv_tag, lp_data->iv_sync_type));

         if (lp_data->iv_num_tries >=3 )
         {
              tm_log_event(DTM_TM_EXCEEDED_SYNC_ABORT_TRIES, SQ_LOG_CRIT, "DTM_TM_EXCEEDED_SYNC_ABORT_TRIES");
              TMTrace(1, ("tm_originating_sync_abort : max number of retries, exiting.\n"));

              abort (); // retry 3 times
         }
         
         lp_data->iv_num_tries++;

         switch(lp_data->iv_sync_type)
         {
             case TM_BEGIN_SYNC:
             case TM_END_SYNC:
             case TM_FORGET_SYNC:
             {
                 lp_tx = (CTmTxBase *) gv_tm_info.get_tx(lp_data->u.iv_node_to_takeover, lp_data->u.iv_seqnum);

                 if (lp_tx == NULL)
                 {
                     tm_log_event(DTM_TM_INVALID_TRANSACTION, SQ_LOG_CRIT, "DTM_TM_INVALID_TRANSACTION");
                     TMTrace(1, ("tm_originating_sync_abort - Unable to find transaction "
                                 "during a phase 2 sync.\n"));
                     abort ();
                 }
                 
                 lp_tx->schedule_redrive_sync();
                 break;
             }
             case TM_UP:
            {
                  send_state_up_sync(gv_tm_info.nid()); 
                  break;
               
            }
            case TM_STATE_RESYNC:
            {
                TMTrace(3, ("tm_originating_sync_abort - TM_STATE_RESYNC sync received.\n"));
                send_state_resync (gv_tm_info.nid(),
                                       gv_tm_info.down_without_sync(lp_data->u.iv_node_to_takeover),
                                       gv_tm_info.node_being_recovered(lp_data->u.iv_node_to_takeover),
                                       gv_tm_info.recovery_list_built(lp_data->u.iv_node_to_takeover), 
                                       lp_data->u.iv_node_to_takeover);
                break;
            }
             case TM_RECOVERY_START:
             {
                 send_takeover_tm_sync (TM_RECOVERY_START, gv_tm_info.nid(), 
                                        lp_data->u.iv_node_to_takeover);
                 break;
             } 
             case TM_RECOVERY_END:
             {
                 send_takeover_tm_sync (TM_RECOVERY_END, gv_tm_info.nid(), 
                                       lp_data->u.iv_node_to_takeover);                         
                 break;
             } 
             case TM_LISTBUILT_SYNC:
             {
                 send_recov_listbuilt_sync (gv_tm_info.nid(), lp_data->u.iv_node_to_takeover);
                 break;

             }
             case TM_PROCESS_RESTART:
                 break;
              case TM_SYS_RECOV_START_SYNC:
              {
                  send_sys_recov_start_sync(gv_tm_info.nid()); 
                  break;
              }
              case TM_SYS_RECOV_END_SYNC:
              {
                  send_sys_recov_end_sync(gv_tm_info.nid()); 
                  break;
              }
              default:
              {
                 tm_log_event(DTM_TM_UKN_SYNC_TYPE, SQ_LOG_WARNING, "DTM_TM_UKN_SYNC_TYPE");
                 break;
              } 
         };
         // do not remove from table as we will retry
     }
     TMTrace(2, ("tm_originating_sync_abort EXIT\n"));
}
#endif

// ---------------------------------------------------------------------------
// tm_process_node_down_msg
// Purpose : Process a down msg from the monitor (virtual nodes). For real
// clusters, process a DTM process death in the case of a logical node failure
// ---------------------------------------------------------------------------
void tm_process_node_down_msg(int32 pv_nid)
{
    gv_tm_info.close_tm(pv_nid); 
    TMTrace(2, ("tm_process_node_down_msg ENTRY, nid %d\n", pv_nid));
    tm_log_event(DTM_NODEDOWN, SQ_LOG_INFO, "DTM_NODEDOWN", 
        -1,-1,gv_tm_info.nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        NULL,pv_nid);

    if (!gv_tm_info.lead_tm())
    {
        tm_get_leader_info();
        if (gv_tm_info.lead_tm() == false)
        {
            TMTrace(2, ("tm_process_node_down_msg EXIT - %d is not the lead TM.\n", pv_nid));
            return;
        }
    }

    if ((gv_tm_info.sys_recov_state() != TM_SYS_RECOV_STATE_END) && 
        (gv_tm_info.sys_recov_lead_tm_nid() == pv_nid))
        // If this is system startup time and system recovery has not yet ended
        // and the down node is the previous lead TM node, this new Lead TM needs
        // to perform system recovery again.  There are no new outstanding 
        // transactions to take over from the down node at this stage since 
        // transaction has now yet been enabled.  
    {
        gv_tm_info.ClusterRecov(new TM_Recov(gv_tm_info.rm_wait_time()));
        gv_tm_info.ClusterRecov()->initiate_start_sync();
    }
    else 
    {       
        if (gv_tm_info.can_takeover())
        {
            // take over phase 1 is now called by TM_Info:restart_tm to make sure
            // it happens after the tm process starts.
            //tm_process_take_over_phase1 (pv_nid);
            // lets start with a clean slate and write a control
            // point after a takeover just in case the lead went
            // down, and for an otherwise fresh start
            if ((gv_tm_info.state() == TM_STATE_SHUTTING_DOWN) ||
                (gv_tm_info.state() == TM_STATE_SHUTDOWN_COMPLETED))
            {
               if (gv_tm_info.num_active_txs() <= 0)
               {
                  // redrive the shutdown operation
                  TMShutdown *lp_Shutdown = new TMShutdown(&gv_tm_info, gv_RMs.TSE()->return_rms());
                  gv_tm_info.shutdown_coordination_started(true);
                  lp_Shutdown->coordinate_shutdown();
                  delete lp_Shutdown;
                  // This must be the lead TM.  After the shutdown, set the registry
                  // entry to indicate that transaction service has stoppped
                  gv_tm_info.set_txnsvc_ready(TXNSVC_DOWN);
               }
            }
        }
    }
    // Since the node is down, the TM is closed
    gv_tm_info.close_tm(pv_nid);
    TMTrace(2, ("tm_process_node_down_msg EXIT nid %d\n", pv_nid));
} //tm_process_node_down_msg


// -----------------------------------------------------------------
// tm_process_node_quiesce_msg
// Purpose : process a quiesce node notice from the Monitor.
// This can be received by any TM.  The TM suspends transaction
// processing but will still process Management requests and TSE
// replies to outstanding requests.  The Monitor will kill this
// TM process once TSEs have completed control pointing.
// pv_stop is only set to true for TM testing.
// ----------------------------------------------------------------
void tm_process_node_quiesce_msg(CTmTxMessage *pp_msg=NULL)
{
   short lv_error = FEOK;
   static int32 lv_lastTMState = gv_tm_info.state();
   bool lv_stop = (pp_msg)?pp_msg->request()->u.iv_quiesce.iv_stop:false;

   TMTrace(2, ("tm_process_node_quiesce_msg ENTRY, stop=%d, current TM State %d.\n",
           lv_stop, lv_lastTMState));
    tm_log_event(DTM_NODEQUIESCE, SQ_LOG_INFO, "DTM_NODEQUIESCE", 
        -1,-1,gv_tm_info.nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,gv_tm_info.state(),lv_stop);

   if (lv_stop)
   {
      if (gv_tm_info.state() != TM_STATE_QUIESCE)
      {
          TMTrace(1, ("tm_process_node_quiesce_msg - Must quiesce first!!\n"));
      }
      else
          gv_tm_info.state(lv_lastTMState);
   }
   else
      gv_tm_info.state(TM_STATE_QUIESCE);

   tm_log_event(DTM_TM_QUIESCED, SQ_LOG_WARNING, "DTM_TM_QUIESCED",
                -1, -1, gv_tm_info.nid());
   TMTrace(1, ("TM %d quiescing.\n", gv_tm_info.nid()));
   if (pp_msg != NULL)
   {
      pp_msg->reply(lv_error);
      delete pp_msg;
   }
   TMTrace(2, ("tm_process_node_quiesce_msg EXIT.\n"));
} //tm_process_req_quiesce


void tm_abort_all_transactions(bool pv_shutdown)
{
    TMTrace(2, ("tm_abort_all_transactions ENTRY with shutdown=%d.\n", pv_shutdown));

    gv_tm_info.abort_all_active_txns();

    if (!pv_shutdown)
       gv_tm_info.state(TM_STATE_UP);

    TMTrace(2, ("tm_abort_all_transactions EXIT\n"));
}
// ----------------------------------------------------------
// tm_process_registry_change
// Purpose - determine if a DTM key was changed and if we need 
//           to take action 
// -----------------------------------------------------------
void tm_process_registry_change(MS_Mon_Change_def *pp_change )
{
    int32 lv_value; 
    char lv_regKeyText[1024];
    char *lp_regKeyText = (char *) &lv_regKeyText;

    sprintf(lp_regKeyText, "%s:%s=%s", pp_change->group, 
            pp_change->key, pp_change->value);
    TMTrace(1, ("tm_process_registry_change Registry Change notice key %s.\n", lp_regKeyText));
    //tm_log_event(DTM_REGCHANGE_NOTICE, SQ_LOG_INFO, "DTM_REGCHANGE_NOTICE",
    //            -1,-1,gv_tm_info.nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,lp_regKeyText);

    if (strcmp(pp_change->key, DTM_STALL_PHASE_2) == 0)
    {
       lv_value = atoi (pp_change->value);
       if (lv_value >= 0)
       {
          gv_tm_info.stall_phase_2(lv_value);
          HbaseTM_initiate_stall(lv_value);
       }
    }
    else if (strcmp(pp_change->key, DTM_RM_WAIT_TIME) == 0)
    {
       lv_value = atoi (pp_change->value);
       if (lv_value > 0)
       {
          lv_value *= 100; //  100 (secs to 10 msecs)
          gv_tm_info.rm_wait_time(lv_value);
       }
    }
    else if (strcmp(pp_change->key, DTM_TM_TRACE) == 0)
    {
        lv_value = atoi (pp_change->value);
        if (lv_value >= 0)
            gv_tm_info.set_trace(lv_value/*detail*/);
    }
    else if (strcmp(pp_change->key, DTM_TRANS_HUNG_RETRY_INTERVAL) == 0)
    {
        lv_value = atoi (pp_change->value);
        if (lv_value > 0)
          gv_tm_info.trans_hung_retry_interval(lv_value);
    }
    else if (strcmp(pp_change->key, DTM_XATM_TRACE) == 0)
    {
        if (strcmp(pp_change->value,"") != 0)
            gv_tm_info.set_xa_trace(pp_change->value);
    }
    else if (strcmp(pp_change->key, DTM_TIMERTHREAD_WAIT) == 0)
    {
        lv_value = atoi (pp_change->value);
        if (lv_value > 0 || lv_value == -1)
          gv_tm_info.timerDefaultWaitTime(lv_value);
        gv_tm_info.tmTimer()->defaultWaitTime(gv_tm_info.timerDefaultWaitTime());
    }
    // Note that with pool configuration parameters you must set/alter them in a 
    // specific order if they overlap because they are parsed separately.
    // Increasing values: Set max, then ss_high, then ss_low.
    // Decreasing values: Set ss_low, then ss_high, then max.
    else if (strcmp(pp_change->key, DTM_TM_STATS) == 0)
    {
        lv_value = atoi (pp_change->value);
        bool lv_tm_stats = ((lv_value == 0)?false:true);
        gv_tm_info.stats()->initialize(lv_tm_stats, gv_tm_info.stats()->collectInterval());
        gv_tm_info.threadPool()->setConfig(lv_tm_stats);
        // Add other pools here
    }
    // Configure thread pool
    else if (strcmp(pp_change->key, DTM_MAX_NUM_THREADS) == 0)
    {
        lv_value = atoi (pp_change->value);
        if (lv_value >= 1)
           gv_tm_info.threadPool()->setConfig(gv_tm_info.tm_stats(), lv_value);
    }
    else if (strcmp(pp_change->key, DTM_STEADYSTATE_LOW_THREADS) == 0)
    {
        lv_value = atoi (pp_change->value);
        if (lv_value >= 0)
           gv_tm_info.threadPool()->setConfig(gv_tm_info.tm_stats(), -1, lv_value);
    }
    else if (strcmp(pp_change->key, DTM_STEADYSTATE_HIGH_THREADS) == 0)
    {
        lv_value = atoi (pp_change->value);
        if (lv_value >= 0)
           gv_tm_info.threadPool()->setConfig(gv_tm_info.tm_stats(), -1, -1, lv_value);
    }
    // Configure transaction pool
    else if (strcmp(pp_change->key, DTM_MAX_NUM_TRANS) == 0)
    {
        lv_value = atoi (pp_change->value);
        if (lv_value >= 1)
           gv_tm_info.transactionPool()->setConfig(gv_tm_info.tm_stats(), lv_value);
    }
    else if (strcmp(pp_change->key, DTM_STEADYSTATE_LOW_TRANS) == 0)
    {
        lv_value = atoi (pp_change->value);
        if (lv_value >= 0)
           gv_tm_info.transactionPool()->setConfig(gv_tm_info.tm_stats(), -1, lv_value);
    }
    else if (strcmp(pp_change->key, DTM_STEADYSTATE_HIGH_TRANS) == 0)
    {
        lv_value = atoi (pp_change->value);
        if (lv_value >= 0)
           gv_tm_info.transactionPool()->setConfig(gv_tm_info.tm_stats(), -1, -1, lv_value);
    }
    else if (strcmp(pp_change->key, DTM_CP_INTERVAL) == 0)
    {
        lv_value = atoi (pp_change->value);
        if (lv_value > 0)
         lv_value *= 60000; // 60 (mins to secs) * 1000 (secs to msecs)
        if (lv_value >= 0 && lv_value != gv_tm_info.cp_interval())
        {
          // Cancel the TmTimer control point event and re-add with the
          // new interval.
          gv_tm_info.cp_interval(lv_value);
          gv_tm_info.tmTimer()->cancelControlpointEvent();
          gv_tm_info.tmTimer()->addControlpointEvent(lv_value);
        }
    }
    else if (strcmp(pp_change->key, DTM_STATS_INTERVAL) == 0)
    {
        lv_value = atoi (pp_change->value);
        if (lv_value > 0)
         lv_value *= 60000; // 60 (mins to secs) * 1000 (secs to msecs)
        if (lv_value >= 0 && lv_value != gv_tm_info.stats_interval())
        {
            // Cancel the TmTimer stats event and re-add with the
            // new interval.
            gv_tm_info.stats_interval(lv_value);
            gv_tm_info.tmTimer()->cancelStatsEvent();
            gv_tm_info.tmTimer()->addStatsEvent(lv_value);
        }
    }
    else if (strcmp(pp_change->key, DTM_TM_RMRETRY_INTERVAL) == 0)
    {
        lv_value = atoi (pp_change->value);
        if (lv_value > 0)
         lv_value *= 60000; // 60 (mins to secs) * 1000 (secs to msecs)
        if (lv_value >= 0 && lv_value != gv_tm_info.RMRetry_interval())
        {
            // Cancel the TmTimer stats event and re-add with the
            // new interval.
            gv_tm_info.RMRetry_interval(lv_value);
            gv_tm_info.tmTimer()->cancelRMRetryEvent();
            gv_tm_info.tmTimer()->addRMRetryEvent(lv_value);
        }
    }
    else if (strcmp(pp_change->key, DTM_TX_ABORT_TIMEOUT) == 0)
    {
       lv_value = atoi (pp_change->value);
       if (lv_value != -1 && lv_value <= 0)
          gv_tm_info.timeout(TX_ABORT_TIMEOUT); //Default
       else
          gv_tm_info.timeout(lv_value);
    }
    else if (strcmp(pp_change->key, DTM_TEST_PAUSE_STATE) == 0)
    {
       lv_value = atoi (pp_change->value);
       if (lv_value < TM_TX_STATE_NOTX || lv_value > TM_TX_STATE_LAST)
       {
          if (lv_value == -2)
          {
             TMTrace(1,("DTM_TEST_PAUSE_STATE set to %d, type %d = random!\n",  
                     lv_value, gv_pause_state_type));
             srand(time(NULL));
             gv_pause_state_type = TX_PAUSE_STATE_TYPE_RANDOM;
             gv_pause_state = rand() % TM_TX_STATE_LAST; //starting point
          }
          else
          {
             TMTrace(1,("DTM_TEST_PAUSE_STATE set to default (-1) because %d not a value state, type %d.\n", 
                     lv_value, TM_TX_STATE_NOTX));
             gv_pause_state = -1; //Default
          }
       }
       else
       {
          TMTrace(1,("DTM_TEST_PAUSE_STATE set to %d, type %d.\n",  lv_value, gv_pause_state_type));
          gv_pause_state = lv_value;
       }
    }
    else if (strcmp(pp_change->key, DTM_RM_PARTIC) == 0)
    {
       lv_value = atoi (pp_change->value);
       gv_tm_info.RMPartic(lv_value);
       TMTrace (1, ("DTM_RM_PARTIC set to %d.\n", gv_tm_info.RMPartic()));
    }
    else if (strcmp(pp_change->key, DTM_TM_TS_MODE) == 0)
    {
       lv_value = atoi (pp_change->value);
       gv_tm_info.TSMode((TS_MODE) lv_value);
       TMTrace (1, ("DTM_TM_TS_MODE set to %d.\n", gv_tm_info.TSMode()));
    }
    else if (strcmp(pp_change->key, DTM_TM_SHUTDOWNABRUPTNOW) == 0)
    {
       lv_value = atoi (pp_change->value);
       if (lv_value == 1)
       {
          TMTrace (1, ("DTM_TM_SHUTDOWNABRUPTNOW set, calling shutdown, abrupt. Use for testing only!!\n"));
          tm_log_event(DTM_ERROR_SHUTDOWN_DEBUG, SQ_LOG_INFO, "DTM_ERROR_SHUTDOWN_DEBUG");
          msg_mon_shutdown(MS_Mon_ShutdownLevel_Abrupt);
       }
    }
    else if (strcmp(pp_change->key, DTM_BROADCAST_ROLLBACKS) == 0)
    {
        bool lv_changed = false;
        TM_BROADCAST_ROLLBACKS lv_broadcast_rollbacks = (TM_BROADCAST_ROLLBACKS) atoi (pp_change->value);
        switch (lv_broadcast_rollbacks)
        {
        case TM_BROADCAST_ROLLBACKS_NO:
        case TM_BROADCAST_ROLLBACKS_YES:
        case TM_BROADCAST_ROLLBACKS_DEBUG:
           gv_tm_info.broadcast_rollbacks(lv_broadcast_rollbacks);
           lv_changed = true;
           break;
        }
        if (lv_changed)
        {
           tm_log_event (DTM_BROADCAST_ROLLBACKS_INFO, SQ_LOG_INFO,"DTM_BROADCAST_ROLLBACKS_INFO",
                         -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,gv_tm_info.broadcast_rollbacks()); 
           TMTrace(1, ("DTM_BROADCAST_ROLLBACKS changed to %d.\n", gv_tm_info.broadcast_rollbacks()));
        }
    }

}

// tm_process_monitor_msg
// Purpose - when a monitor message is received, this is called
// ------------------------------------------------------------ 
void tm_process_monitor_msg(BMS_SRE *pp_sre, char *pp_buf)
{
    CTmTxBase *lp_tx     = NULL;
    MS_Mon_Msg  lv_msg;

    if (pp_buf == NULL)
    {
        tm_log_event(DTM_INVALID_PROC_MON_MSG, SQ_LOG_CRIT, "DTM_INVALID_PROC_MON_MSG");
        TMTrace(1, ("tm_process_monitor_msg ENTER, data null, exiting \n"));

        abort ();
    }
    
    memcpy (&lv_msg, pp_buf, sizeof (MS_Mon_Msg));

    TMTrace(2, ("tm_process_monitor_msg ENTRY, type=%d\n", lv_msg.type));

    if (lv_msg.type != MS_MsgType_NodeQuiesce)
       // Delay reply for quiesce processing.
       // At this point we will not reply with an error so get the reply
       // out of the way so we can do some real processing
       XMSG_REPLY_(pp_sre->sre_msgId,       /*msgid*/
                   NULL,           /*replyctrl*/
                   0,      /*replyctrlsize*/
                   NULL,           /*replydata*/
                   0,          /*replydatasize*/
                   0,              /*errorclass*/
                   NULL);          /*newphandle*/
    
    switch (lv_msg.type) 
    {
    case MS_MsgType_Change:
    {
         tm_process_registry_change(&lv_msg.u.change);
         break;
    }
    case MS_MsgType_Shutdown:
    {
         // If the TM is already shutting down, we don't want to change the state back to TM_STATE_SHUTTING_DOWN
         if (gv_tm_info.state_shutdown())
         {         
             TMTrace(1, ("tm_process_monitor_msg Shutdown notice, level %d.  Duplicate notice ignored!\n",
                lv_msg.u.shutdown.level));
             tm_log_event(DTM_DUPLICATE_SHUTDOWN_NOTICE, SQ_LOG_CRIT, "DTM_DUPLICATE_SHUTDOWN_NOTICE",
                FEDUP,-1,gv_tm_info.nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,lv_msg.u.shutdown.level);
         }
         else
         {
             TMTrace(1, ("tm_process_monitor_msg Shutdown notice, level %d.\n", 
                 lv_msg.u.shutdown.level));
             tm_log_event(DTM_SHUTDOWN_NOTICE, SQ_LOG_INFO, "DTM_SHUTDOWN_NOTICE",
                 -1,-1,gv_tm_info.nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,lv_msg.u.shutdown.level);

             gv_tm_info.state(TM_STATE_SHUTTING_DOWN);
             gv_tm_info.shutdown_level(lv_msg.u.shutdown.level);

             if (lv_msg.u.shutdown.level == MS_Mon_ShutdownLevel_Immediate)
                tm_abort_all_transactions(true);
         }

         // if the shutdown mode is MS_Mon_ShutdownLevel_Normal, go back to the main loop
         // to service user's request for committing or aborting the outstanding txs.
         // If the shutdown mode is MS_Mon_ShutdownLevel_Abrupt, go back to the main loop
         // and wait for the monitor to kill all the TMs.-
         break;
    }  // MS_MsgType_Shutdown
    case MS_MsgType_NodeDown:
    {
         TMTrace(3, ("tm_process_monitor_msg NodeDown notice for nid %d\n", lv_msg.u.down.nid));

         // Appoint new Lead TM if necessary.
         tm_get_leader_info();

         if (gv_tm_info.lead_tm() == false)
         {
            gv_tm_info.down_without_sync(lv_msg.u.death.nid, true);
            TMTrace(3, ("tm_process_monitor_msg - setting down_without_sync to TRUE for node %d\n",
                        lv_msg.u.death.nid))
         }
         // Process the death notice for the logical node which died
         // We may already have processed a node down message, depending on the Seaquest
         // environment - configurations with spares don't send node down.
         if (gv_tm_info.tm_is_up(lv_msg.u.death.nid))
            tm_process_node_down_msg(lv_msg.u.death.nid);

         // If we're the lead TM, attempt to recover the TM.
         if (gv_tm_info.lead_tm() == true)
            gv_tm_info.addTMRestartRetry(lv_msg.u.death.nid, 0);
         break;
    }
    case MS_MsgType_NodeUp:
    {
        TMTrace(1, ("tm_process_monitor_msg NodeUp notice for nid %d\n", lv_msg.u.up.nid));
        tm_log_event(DTM_NODEUP, SQ_LOG_INFO, "DTM_NODEUP", 
            -1,-1,gv_tm_info.nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
           NULL,lv_msg.u.up.nid);
        if (gv_tm_info.lead_tm()) {
          gv_tm_info.open_restarted_tm(lv_msg.u.up.nid);
        }
        break;
    }
#if 0
    case MS_MsgType_NodePrepare:
    {
        TMTrace(1, ("tm_process_monitor_msg NodePrepare notice for nid %d\n", lv_msg.u.prepare.nid));
        tm_log_event(DTM_NODEPREPARE, SQ_LOG_INFO, "DTM_NODEPREPARE", 
            -1,-1,gv_tm_info.nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
           NULL,lv_msg.u.prepare.nid);
        if (gv_tm_info.lead_tm()) {
           gv_tm_info.restart_tm_process(lv_msg.u.prepare.nid);
        }
        break;
    }
    case MS_MsgType_TmRestarted:
    {
        TMTrace(1, ("tm_process_monitor_msg TMRestarted notice for nid %d\n", lv_msg.u.tmrestarted.nid));

        // Appoint new Lead TM if necessary.
         tm_get_leader_info();
         if (gv_tm_info.lead_tm() == false)
         {
            gv_tm_info.down_without_sync(lv_msg.u.tmrestarted.nid, true);
            TMTrace(3, ("tm_process_monitor_msg - setting down_without_sync to TRUE for node %d\n",
                        lv_msg.u.tmrestarted.nid))
         }
        	
        tm_log_event(DTM_TMRESTARTED, SQ_LOG_INFO, "DTM_TMRESTARTED", 
            -1,-1,gv_tm_info.nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
           NULL,lv_msg.u.tmrestarted.nid);
        if (gv_tm_info.lead_tm()) {
           gv_tm_info.open_restarted_tm(lv_msg.u.tmrestarted.nid);
        }
        break;
    }
#endif
    case MS_MsgType_ProcessDeath:
    {
        TMTrace(3, ("tm_process_monitor_msg Process Death notice for %s\n", 
                    lv_msg.u.death.process_name));

         switch (lv_msg.u.death.type)
         {
             case MS_ProcessType_TSE:
             {
                tm_log_event(DTM_PROCDEATH_TSE, SQ_LOG_INFO, "DTM_PROCDEATH_TSE", 
                    -1,-1,gv_tm_info.nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,lv_msg.u.death.pid,-1,-1,
                    lv_msg.u.death.process_name,lv_msg.u.death.nid);

                TMTrace(1, ("tm_process_monitor_msg death notice for TSE %s (%d, %d).\n", 
                            lv_msg.u.death.process_name, lv_msg.u.death.nid, lv_msg.u.death.pid));

                // Check to see if the TSE is still alive - this will indicate a 
                // failover rather than a crash/stop.
                int lv_nid;
                int lv_pid;
                int lv_ret = msg_mon_get_process_info ((char *) &lv_msg.u.death.process_name, 
                                                       &lv_nid, &lv_pid);

                // Mark TSE as failed in RM list
                if (lv_ret != FEOK || lv_pid == -1)
                {
                    tm_log_event(DTM_TSE_FAILURE_DETECTED, SQ_LOG_WARNING, "DTM_TSE_FAILURE_DETECTED", 
                        lv_ret,-1,gv_tm_info.nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,lv_msg.u.death.pid,-1,-1,
                        lv_msg.u.death.process_name,lv_msg.u.death.nid);
                    TMTrace(1, ("tm_process_monitor_msg Failure detected for TSE %s (%d, %d).\n", 
                            lv_msg.u.death.process_name, lv_msg.u.death.nid, lv_msg.u.death.pid));

                    gv_RMs.TSE()->fail_rm(lv_msg.u.death.nid, lv_msg.u.death.pid);
                }
                else
                {
                    // Ignore failovers, they're transparent apart from an error
                    // 201 for any outstanding I/Os.
                    RM_Info_TSEBranch * lp_RM;
                    for (int lv_inx=0; 
                         lv_inx < gv_RMs.TSE()->return_highest_index_used(); 
                         lv_inx++)
                    {
                        lp_RM = gv_RMs.TSE()->return_slot_by_index(lv_inx);
                        if (lp_RM && 
                            !strcmp(lp_RM->pname(), 
                                    (char *) &lv_msg.u.death.process_name))
                        {
                            lp_RM->nid(lv_nid);
                            lp_RM->pid(lv_pid);
                        }
                    }
                    tm_log_event(DTM_TSE_FAILOVER_DETECTED, SQ_LOG_INFO, "DTM_TSE_FAILOVER_DETECTED", 
                        -1,-1,gv_tm_info.nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,lv_pid,-1,-1,
                        lv_msg.u.death.process_name,lv_nid);
                    TMTrace(1, ("tm_process_monitor_msg failover detected for TSE %s. New primary is (%d, %d).\n", 
                            lv_msg.u.death.process_name, lv_nid, lv_pid));
                }
                break;
             }
             case MS_ProcessType_ASE:
             {
                  // Don't care unless its the TLOG.  TODO
                tm_log_event(DTM_PROCDEATH_ASE, SQ_LOG_INFO, "DTM_PROCDEATH_ASE", 
                    -1,-1,gv_tm_info.nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,lv_msg.u.death.pid,-1,-1,
                    lv_msg.u.death.process_name,lv_msg.u.death.nid);

                TMTrace(1, ("tm_process_monitor_msg death notice for ASE %s (%d, %d).\n",
                    lv_msg.u.death.process_name, lv_msg.u.death.nid, lv_msg.u.death.pid));

                break;
             }
             case MS_ProcessType_DTM:
             {
                tm_log_event(DTM_PROCDEATH_DTM, SQ_LOG_INFO, "DTM_PROCDEATH_DTM", 
                    -1,-1,gv_tm_info.nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                    lv_msg.u.death.process_name);
                TMTrace(1, ("tm_process_monitor_msg death notice for DTM%d\n", lv_msg.u.death.nid));
                break;
             }
             // most likely application death.  If not, then the tx will come back NULL
             // and we'll just return
             default :
             {
                 TMTrace(1, ("tm_process_monitor_msg death notice for process type %d\n",lv_msg.u.death.type ));

                  TM_Txid_Internal  *lp_transid  = (TM_Txid_Internal *)&lv_msg.u.death.transid;
                  lp_tx = (CTmTxBase *) gv_tm_info.get_tx(lp_transid);
                  if (lp_tx != NULL)
                       lp_tx->schedule_abort();
                  // this is a regular process death not associated with a transid.  Find them....
                  else 
                  {
                      TMTrace(3, ("tm_process_monitor_msg death notice for pid %d on nid %d\n",
                                        lv_msg.u.death.pid, lv_msg.u.death.nid ));

                      int64 lv_count = 0;
                      int64 lv_size = 0;
                      void **lp_tx_list = gv_tm_info.get_all_txs (&lv_size);
                      if (!lp_tx_list)
                         break;

                      TM_TX_Info *lp_current_tx = (TM_TX_Info *)lp_tx_list[lv_count];
                      while ((lv_count < lv_size) && lp_current_tx) 
                      {
                           if (lp_current_tx->is_app_partic(lv_msg.u.death.pid, lv_msg.u.death.nid))
                           {
                               TMTrace(3, ("tm_process_monitor_msg aborting seq num %d\n",
                                        lp_current_tx->seqnum() ));
                               lp_current_tx->remove_app_partic(lv_msg.u.death.pid, lv_msg.u.death.nid);
                               lp_current_tx->schedule_abort();
                           }
                           lv_count++;
                           if (lv_count < lv_size)
                              lp_current_tx = (TM_TX_Info*)lp_tx_list[lv_count];
                      }
                      if (lp_tx_list)
                          delete []lp_tx_list;
                  } 
                break;
             }
          }
       break;
    }
    case MS_MsgType_NodeQuiesce:
    {
         TMTrace(3, ("tm_process_monitor_msg NodeQuiesce notice.\n"));
         tm_process_node_quiesce_msg();
         XMSG_REPLY_(pp_sre->sre_msgId,       /*msgid*/
                     NULL,           /*replyctrl*/
                     0,      /*replyctrlsize*/
                     NULL,           /*replydata*/
                     0,          /*replydatasize*/
                     0,              /*errorclass*/
                     NULL);          /*newphandle*/
         break;
    }
#ifdef SUPPORT_TM_SYNC
    case MS_MsgType_TmSyncAbort:
    {
        // There can be many monitor replies, so circle through them all
        for (int lv_count = 0; lv_count < lv_msg.u.tmsync.count; lv_count++)
        {
            // We use sync handles for receiving DTMs and sync tags for originating DTMs.  Right now this
            // is required because the tm_sync_cb() is passed the handle.
            Tm_Sync_Data *lp_sync_data = (Tm_Sync_Data *)gv_sync_map.get(lv_msg.u.tmsync.handle[lv_count]);
         
            // originating DTM
            if (lp_sync_data == NULL)
            {
               tm_originating_sync_abort (lv_msg.u.tmsync.orig_tag[lv_count]);
            }
            // recipient DTM
            else 
            {
               gv_sync_map.remove(lv_msg.u.tmsync.handle[lv_count]);
               delete lp_sync_data;
            }
        }
        break; // case MS_MsgType_TmSyncAbort
    } 
    case MS_MsgType_TmSyncCommit:
    {
        // There can be many monitor replies, so circle through them all
        for (int lv_count = 0; lv_count < lv_msg.u.tmsync.count; lv_count++)
        {
            // We use sync handles for receiving DTMs and sync tags for originating DTMs.  Right now this
            // is required because the tm_sync_cb() is passed the handle.
            Tm_Sync_Data *lp_sync_data = (Tm_Sync_Data *)gv_sync_map.get(lv_msg.u.tmsync.handle[lv_count]);
      
            // originating DTM
            if (lp_sync_data == NULL)
            {
                tm_originating_sync_commit(lv_msg.u.tmsync.orig_tag[lv_count]);
            }
            // receipient DTM
            else
            {
                tm_recipient_sync_commit(lp_sync_data);
                gv_sync_map.remove(lv_msg.u.tmsync.handle[lv_count]);
                delete lp_sync_data;
            }
        }
    break;
    }
#endif
    case MS_MsgType_Event:
//    case MS_MsgType_UnsolicitedMessage:
    default:
    {
         break;
    }
    };
   
    TMTrace(2, ("tm_process_monitor_msg EXIT\n"));
}

// -----------------------------------------------------------------------
// tm_process_msg
// Purpose - process messages incoming to the TM
// -----------------------------------------------------------------------
void tm_process_msg(BMS_SRE *pp_sre) 
{
    short                  lv_ret;
    char                   la_send_buffer[4096];
    char                   la_recv_buffer[sizeof(Tm_Req_Msg_Type)];
    char                  *la_recv_buffer_ddl = NULL;
    Tm_Broadcast_Req_Type *lp_br_req;
    Tm_Broadcast_Rsp_Type *lp_br_rsp; 
    Tm_Perf_Stats_Req_Type *lp_ps_req;
    Tm_Perf_Stats_Rsp_Type *lp_ps_rsp; 
    //Tm_Sys_Status_Req_Type *lp_ss_req;
    Tm_Sys_Status_Rsp_Type *lp_ss_rsp;
    Tm_RolloverCP_Req_Type *lp_rc_req;
    Tm_RolloverCP_Rsp_Type *lp_rc_rsp;
    Tm_Control_Point_Req_Type *lp_cp_req;
    MESSAGE_HEADER_SQ     *lp_msg_hdr;
    CTmTxMessage          *lp_msg;

    static bool           sv_schedule_init_and_recover_rms_called = false;

    TMTrace(2, ("tm_process_msg ENTRY\n"));

    if((unsigned)(pp_sre->sre_reqDataSize) > (sizeof(Tm_Req_Msg_Type))){
       la_recv_buffer_ddl = new char[pp_sre->sre_reqDataSize];

    lv_ret = BMSG_READDATA_(pp_sre->sre_msgId,           // msgid
                            la_recv_buffer_ddl,          // reqdata
                            pp_sre->sre_reqDataSize);    // bytecount

    }else{
    lv_ret = BMSG_READDATA_(pp_sre->sre_msgId,           // msgid
                            la_recv_buffer,              // reqdata
                            pp_sre->sre_reqDataSize);    // bytecount
    }

    if (lv_ret != 0)
    {
       // a return value of 1 means the message has been abandoned by the sender.
       if (lv_ret == 1)
       {
          tm_log_event(DTM_TM_READ_MSG_FAIL, SQ_LOG_WARNING, "DTM_TM_READ_MSG_FAIL", lv_ret);
          TMTrace(1, ("tm_process_msg : BMSG_READDATA_ failed with error %d. Message ignored!\n", lv_ret));
          return;
       }
       else
       {
          tm_log_event(DTM_TM_READ_MSG_FAIL, SQ_LOG_CRIT, "DTM_TM_READ_MSG_FAIL", lv_ret);
          TMTrace(1, ("tm_process_msg : BMSG_READDATA_ failed with error %d\n", lv_ret));
           abort();
       }   
    }
    
    if (pp_sre->sre_flags & XSRE_MON) 
    {
        tm_process_monitor_msg(pp_sre, la_recv_buffer);
        return;
    }

    lp_msg_hdr = (MESSAGE_HEADER_SQ *)&la_recv_buffer;

    TMTrace(3, ("tm_process_msg : tm %d, type %d, msgid %d\n",
                    gv_tm_info.nid(), lp_msg_hdr->rr_type.request_type, pp_sre->sre_msgId));

    // Test the message version and make sure not too low OR too high
    if ((lp_msg_hdr->version.request_version < TM_SQ_MSG_VERSION_MINIMUM) ||
        (lp_msg_hdr->version.request_version > TM_SQ_MSG_VERSION_CURRENT))
    {
        tm_log_event(DTM_TM_MSG_VERSION_INVALID, SQ_LOG_CRIT, "DTM_TM_MSG_VERSION_INVALID");
        TMTrace(1, ("tm_process_msg : Old message received. Minimum supported=%d, "
                       "Received message version=%d\n",
                       TM_SQ_MSG_VERSION_MINIMUM,
                       lp_msg_hdr->version.request_version));
        // Reply with error since illegal version
        XMSG_REPLY_(pp_sre->sre_msgId,   // msgid
                NULL,                    // replyctrl 
                0,                       // replyctrlsize
                NULL,                    // replydata
                0,                       // replydatasize
                FEINCOMPATIBLEVERSION,   // errorclass
                NULL);                   // newphandle
        return;
    }

    switch (lp_msg_hdr->rr_type.request_type)
    {
   case TM_MSG_TYPE_BROADCAST:
    {
         lp_br_req = (Tm_Broadcast_Req_Type *) la_recv_buffer;
         lp_br_rsp = (Tm_Broadcast_Rsp_Type *) la_send_buffer;
         tm_initialize_rsp_hdr(lp_br_req->iv_msg_hdr.rr_type.request_type,
                               (Tm_Rsp_Msg_Type *) lp_br_rsp);
         tm_process_req_broadcast (pp_sre, lp_br_req, lp_br_rsp); 
         TMTrace(2, ("tm_process_msg EXIT\n"));
         return; 
    }
    case TM_MSG_TYPE_TMPERFSTATS:
    {
         lp_ps_req = (Tm_Perf_Stats_Req_Type *) la_recv_buffer;
         lp_ps_rsp = (Tm_Perf_Stats_Rsp_Type *) la_send_buffer;
         if (gv_tm_info.lead_tm())
         {
            // We ignore unexpected Perf Stats request because they can happen shortly after
            // a lead TM migration.
            TMTrace(1, ("tm_process_msg : Warning ignoring Performance Statistics request received by Lead TM from nid %d\n",
                    lp_ps_req->iv_sending_tm_nid));
            tm_log_event(DTM_TM_UNEXPECTED_PS_RECEIVED, SQ_LOG_WARNING, "DTM_TM_UNEXPECTED_PS_RECEIVED",
                         -1, -1, gv_tm_info.nid(), -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
                         -1, "Lead TM received Performance Statistics request", lp_ps_req->iv_sending_tm_nid);
         }
         tm_fill_perf_stats_buffer(lp_ps_rsp);
         ushort lv_len = sizeof(Tm_Perf_Stats_Rsp_Type);
         XMSG_REPLY_(pp_sre->sre_msgId,       // msgid
                NULL,                    // replyctrl 
                0,                       // replyctrlsize
                (char *) lp_ps_rsp,         // replydata
                lv_len,                  // replydatasize
                0,                       // errorclass
                NULL);                   // newphandle

         TMTrace(2, ("tm_process_msg EXIT\n")); 
         return; 
    }
    case TM_MSG_TYPE_CALLSTATUSSYSTEM:
    {
         //lp_ss_req = (Tm_Sys_Status_Req_Type *) la_recv_buffer;
         lp_ss_rsp = (Tm_Sys_Status_Rsp_Type *) la_send_buffer;

         TM_STATUSSYS *lp_system_status =  new TM_STATUSSYS();
         
         gv_tm_info.send_system_status(&lp_ss_rsp->iv_status_system);
         ushort lv_len = sizeof(Tm_Sys_Status_Rsp_Type);
         XMSG_REPLY_(pp_sre->sre_msgId,       // msgid
                NULL,                    // replyctrl 
                0,                       // replyctrlsize
                (char *) lp_ss_rsp,         // replydata
                lv_len,                  // replydatasize
                0,                       // errorclass
                NULL);                   // newphandle
         delete lp_system_status;

         TMTrace(2, ("tm_process_msg EXIT\n")); 
         return; 
    }
    case TM_MSG_TYPE_STATUSSYSTEM:
    {
         //lp_ss_req = (Tm_Sys_Status_Req_Type *) la_recv_buffer;
         lp_ss_rsp = (Tm_Sys_Status_Rsp_Type *) la_send_buffer;

         tm_fill_sys_status_buffer(lp_ss_rsp);
         ushort lv_len = sizeof(Tm_Perf_Stats_Rsp_Type);
         XMSG_REPLY_(pp_sre->sre_msgId,       // msgid
                NULL,                    // replyctrl 
                0,                       // replyctrlsize
                (char *) lp_ss_rsp,         // replydata
                lv_len,                  // replydatasize
                0,                       // errorclass
                NULL);                   // newphandle

         TMTrace(2, ("tm_process_msg EXIT\n")); 
         return; 
    }
    case TM_MSG_TYPE_ROLLOVER_CP:
    {
         lp_rc_req = (Tm_RolloverCP_Req_Type *) la_recv_buffer;
         lp_rc_rsp = (Tm_RolloverCP_Rsp_Type *) la_send_buffer;
         int64 lv_sequence_no = lp_rc_req->iv_sequence_no;

         TMTrace(2, ("tm_control_point_rollover nid: %d, position: %ld\n", lp_rc_req->iv_nid, lv_sequence_no)); 
     
         // May write more than one control point if lv_sequence_no == 1 and iv_audit_seqno != 1
         if((lv_sequence_no > gv_tm_info.audit_seqno()) || ((lv_sequence_no == 1) && (gv_tm_info.audit_seqno() !=1))) {
            gv_tm_info.audit_seqno(lv_sequence_no);
            gv_tm_info.addControlPointEvent();
         }

         ushort lv_len = sizeof(Tm_RolloverCP_Rsp_Type);
         XMSG_REPLY_(pp_sre->sre_msgId,  // msgid
                NULL,                    // replyctrl 
                0,                       // replyctrlsize
                (char *) lp_rc_rsp,      // replydata
                lv_len,                  // replydatasize
                0,                       // errorclass
                NULL);                   // newphandle

         TMTrace(2, ("tm_process_msg EXIT\n")); 
         return; 
    }
    case TM_MSG_TYPE_CP:
    {
          lp_cp_req = (Tm_Control_Point_Req_Type *) la_recv_buffer;
          TMTrace(3, ("tm_process_msg : Control Point from Lead TM nid %d, type %d, startup %d.\n",
                      lp_cp_req->iv_sending_tm_nid, lp_cp_req->iv_type, lp_cp_req->iv_startup));
          if (gv_tm_info.lead_tm())
          {
             // We ignore these unexpected control points because they can happen shortly after
             // a lead TM migration.
             TMTrace(1, ("tm_process_msg : Control Point request received by Lead TM from nid %d\n",
                     lp_cp_req->iv_sending_tm_nid));
             tm_log_event(DTM_TM_UNEXPECTED_CP_RECEIVED, SQ_LOG_WARNING, "DTM_TM_UNEXPECTED_CP_RECEIVED",
                          -1, -1, gv_tm_info.nid(), -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
                          -1, "Lead TM received Control Point request", lp_cp_req->iv_sending_tm_nid);
          }
          else
          {
            if (! sv_schedule_init_and_recover_rms_called)
             {
                 TMTrace(3, ("tm_process_msg : Control Point request (first) from the Lead TM nid %d.\n",
                         lp_cp_req->iv_sending_tm_nid));
                 gv_tm_info.schedule_init_and_recover_rms();
                 sv_schedule_init_and_recover_rms_called = true;
             }
             else
             {
                 TMTrace(3, ("tm_process_msg : Control Point request (subsequent) from the Lead TM nid %d.\n",
                         lp_cp_req->iv_sending_tm_nid));
                 gv_system_tx_count = 0;
                 gv_tm_info.write_all_trans_state();
             }
          }
          Tm_Control_Point_Rsp_Type *lp_rsp2 = 
                        (Tm_Control_Point_Rsp_Type *) la_send_buffer;
          lp_rsp2->iv_error = 0;
          lp_rsp2->iv_msg_hdr.rr_type.reply_type = 
                       (short) (lp_msg_hdr->rr_type.request_type + 1);
          lp_rsp2->iv_msg_hdr.miv_err.error = 0;
          tm_send_reply(pp_sre->sre_msgId,  (Tm_Rsp_Msg_Type *)lp_rsp2);
          return;
    }
    case TM_MSG_TYPE_SHUTDOWN_COMPLETE:
    {
         TMTrace(3, ("tm_process_msg SHUTDOWN_COMPLETE message received.\n"));
         // This is the shutdown_complete inquiry from the lead TM.  Reply if all
         // active txs have been aborted or commited and all RMs are closed.  
         // If the lead TM fails during the Seaquest Shutdown coordination,
         // a new lead TM can take over and resend this inquiry message.
         // That's ok. Just reply to the message again.
         Tm_Shutdown_Rsp_Type *lp_rsp_shutdown = 
                               (Tm_Shutdown_Rsp_Type *) la_send_buffer;

         bool lv_shutdown = false; 
         lp_rsp_shutdown->iv_msg_hdr.rr_type.reply_type = 
                      (short) (lp_msg_hdr->rr_type.request_type + 1);

         switch (gv_tm_info.state())
         {
            case TM_STATE_WAITING_RM_OPEN:
            case TM_STATE_UP:
            case TM_STATE_SHUTTING_DOWN:
            case TM_STATE_TX_DISABLED:
            case TM_STATE_TX_DISABLED_SHUTDOWN_PHASE1:
            case TM_STATE_QUIESCE:
            case TM_STATE_DRAIN:
            {
               lp_rsp_shutdown->iv_error = FETMSHUTDOWN_NOTREADY;
               break;
            }
            case TM_STATE_SHUTDOWN_COMPLETED:
            case TM_STATE_DOWN:
            {
                TMTrace(3, ("tm_process_msg shutdown complete.\n"));
               lp_rsp_shutdown->iv_error = FEOK;
               lv_shutdown = true;
               break;
            }
            default:
            {
                TMTrace(3, ("tm_process_msg shutdown dirty.\n"));
               lp_rsp_shutdown->iv_error = FETMSHUTDOWN_FATAL_ERR;
               lv_shutdown = true;
               break;
            }
         }
         tm_send_reply(pp_sre->sre_msgId, (Tm_Rsp_Msg_Type *) lp_rsp_shutdown);

         if (lv_shutdown)
         {
              TMTrace(1, ("SHUTDOWN : Non Lead DTM%d shutting down, TM state %d.\n", gv_tm_info.nid(), gv_tm_info.state())); 
              msg_mon_process_shutdown();
              TMTrace(1, ("$TM%d exiting. TM state %d.\n",
                      gv_tm_info.nid(), gv_tm_info.state()));
            exit(0);
         }

         return;
    }
    default:
        break;
    }// switch

    // Allocate a message object.  It will be deleted by the
    // TM_TX_Info::process_eventQ method once the request
    // has been processed.

    if( la_recv_buffer_ddl!=NULL)
       lp_msg = new CTmTxMessage((Tm_Req_Msg_Type *) la_recv_buffer_ddl, pp_sre->sre_msgId, la_recv_buffer_ddl);
    else 
       lp_msg = new CTmTxMessage((Tm_Req_Msg_Type *) &la_recv_buffer, pp_sre->sre_msgId, NULL);

    if (lp_msg_hdr->dialect_type == DIALECT_TM_DP2_SQ)
    {
       tm_process_msg_from_xarm(lp_msg);
       TMTrace(2, ("tm_process_msg EXIT. XARM Request detected.\n"));
       return;
    }

   switch (lp_msg->requestType()) 
    {
    case TM_MSG_TYPE_BEGINTRANSACTION:
        tm_process_req_begin(lp_msg);
        break;
    case TM_MSG_TYPE_ENDTRANSACTION:
        tm_process_req_end(lp_msg);
        break;
    case TM_MSG_TYPE_ABORTTRANSACTION:
        tm_process_req_abort(lp_msg);
        break;
    case TM_MSG_TYPE_STATUSTRANSACTION:
        tm_process_req_status (lp_msg);
        break;
    case TM_MSG_TYPE_LISTTRANSACTION:
        tm_process_req_list (lp_msg);
        break;
    case TM_MSG_TYPE_TMSTATS:
        tm_process_req_tmstats (lp_msg);
        break;
    case TM_MSG_TYPE_STATUSTM:
        tm_process_req_statustm (lp_msg);
        break;
    case TM_MSG_TYPE_ATTACHRM:
        tm_process_req_attachrm (lp_msg);
        break;
    case TM_MSG_TYPE_STATUSTRANSMGMT:
        tm_process_req_status_transmgmt(lp_msg);
        break;
    case TM_MSG_TYPE_STATUSALLTRANSMGT:
        tm_process_req_status_all_transmgmt(lp_msg);
        break;
    case TM_MSG_TYPE_GETTRANSINFO:
        tm_process_req_status_gettransinfo(lp_msg);
        break;
    case TM_MSG_TYPE_LEADTM:
        tm_process_req_leadtm (lp_msg);
        break;
    case TM_MSG_TYPE_ENABLETRANS:
        tm_process_req_enabletrans (lp_msg);
        break;
    case TM_MSG_TYPE_DISABLETRANS:
        tm_process_req_disabletrans (lp_msg);
        break;
    case TM_MSG_TYPE_DRAINTRANS:
        tm_process_req_draintrans (lp_msg);
        break;
    case TM_MSG_TYPE_QUIESCE:
        tm_process_node_quiesce_msg(lp_msg);
        break;
    case (TM_MSG_TYPE_ENABLETRANS + TM_TM_MSG_OFFSET):
        // Non-lead TM enableTrans arriving from lead TM
        gv_tm_info.enableTrans(lp_msg);
        break;
    case (TM_MSG_TYPE_DISABLETRANS + TM_TM_MSG_OFFSET):
        // Non-lead TM disableTrans arriving from lead TM
        gv_tm_info.disableTrans(lp_msg);
        break;
    case (TM_MSG_TXINTERNAL_SHUTDOWNP1_WAIT + TM_TM_MSG_OFFSET):
        // Non-lead TM ShutdownPhase1Wait arriving from lead TM
        gv_tm_info.disableTrans(lp_msg);
        break;
    case TM_MSG_TYPE_AX_REG:
        tm_process_req_ax_reg (lp_msg);
        break;
    case TM_MSG_TYPE_JOINTRANSACTION:
        tm_process_req_join_trans (lp_msg);
        break;
    case TM_MSG_TYPE_SUSPENDTRANSACTION:
        tm_process_req_suspend_trans (lp_msg);
        break;
    case TM_MSG_TYPE_AX_UNREG:
        tm_process_req_ax_unreg (lp_msg);
        break;
    case TM_MSG_TYPE_TEST_TX_COUNT:
        lp_msg->response()->u.iv_count.iv_count = gv_tm_info.num_active_txs();
        lp_msg->reply();
        delete lp_msg;
        break;
    case TM_MSG_TYPE_DOOMTX:
        tm_process_req_doomtx(lp_msg);
        break;
    case TM_MSG_TYPE_TSE_DOOMTX:
        tm_process_req_TSE_doomtx(lp_msg);
        break;
    case TM_MSG_TYPE_WAIT_TMUP:
        tm_process_req_wait_tmup(lp_msg);
        break;
   case TM_MSG_TYPE_REGISTERREGION:
        tm_process_req_registerregion(lp_msg);
        break;
   case TM_MSG_TYPE_DDLREQUEST:
        tm_process_req_ddlrequest(lp_msg);
        break;
   case TM_MSG_TYPE_REQUESTREGIONINFO:
        tm_process_req_requestregioninfo(lp_msg);
        break;
   case TM_MSG_TYPE_GETNEXTSEQNUMBLOCK:
        tm_process_req_GetNextSeqNum(lp_msg);
        break;
   default:

        // EMS message here, DTM_INVALID_MESSAGE_TYPE
        tm_log_event(DTM_INVALID_MESSAGE_TYPE2, SQ_LOG_CRIT , "DTM_INVALID_MESSAGE_TYPE2",
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
                    lp_msg->requestType()); /*data */

        TMTrace(1, ("tm_process_msg - TM%d received UNKNOWN message type : %d\n",
               gv_tm_info.nid(), lp_msg->requestType()));
        // Reply with error since unknown request type
        XMSG_REPLY_(pp_sre->sre_msgId,   // msgid
                NULL,                    // replyctrl 
                0,                       // replyctrlsize
                NULL,                    // replydata
                0,                       // replydatasize
                FEINVALOP,               // errorclass
                NULL);                   // newphandle                
        return;
    }
    TMTrace(2, ("tm_process_msg EXIT\n"));
}

// ---------------------------------------------------------------
// tm_shutdown_helper
// Purpose - 
// ----------------------------------------------------------------
void tm_shutdown_helper ()
{
    TMTrace(2, ("tm_shutdown_helper ENTRY, num of active transactions:%d\n",gv_tm_info.num_active_txs()));

    if (gv_tm_info.num_active_txs() <= 0)
    {
        TMShutdown *lp_Shutdown = new TMShutdown(&gv_tm_info, gv_RMs.TSE()->return_rms());
        gv_tm_info.shutdown_coordination_started(true);
        lp_Shutdown->coordinate_shutdown();
        delete lp_Shutdown;
        if (gv_tm_info.lead_tm())
            gv_tm_info.set_txnsvc_ready(TXNSVC_DOWN);

    }
    else
    {
        // wait 1/4 of a second, can be fine-tuned later
        XWAIT(0, TM_SHUTDOWN_WAKEUP_INTERVAL);
    }
   TMTrace(2, ("tm_shutdown_helper EXIT\n"));
}

//
// The 'TM Ready' message (to the monitor) is generated 
// by a non Lead DTM when it is started.
//
// The monitor generates the 'Node UP' message when it 
// receives the 'TM Ready' message. 
//
// The 'Node UP' message causes the lead DTM to (re)connect
// with this DTM.
//
bool generate_tm_ready_if_necessary()
{
 
  if (gv_tm_info.lead_tm()) {
    TMTrace(2, ("generate_tm_ready_if_necessary, returning as I am the lead().\n"));
    return false;
  }
 
  TMTrace(2, ("generate_tm_ready_if_necessary - going to call:msg_mon_tm_ready\n"));
  msg_mon_tm_ready();
  TMTrace(2, ("generate_tm_ready_if_necessary - back from call:msg_mon_tm_ready\n"));
 
  return true;
 
}
// ---------------------------------------------------------------
// tm_main_initialize
// Purpose - call all initialization routines
// --------------------------------------------------------------
void tm_main_initialize()
{
    char            la_leader_name[BUFSIZ];
    char            la_event_data[MS_MON_MAX_SYNC_DATA];
    int32           lv_event_len;
    int32           lv_leader_nid;
    int32           lv_leader_pid;

    // initialize and get TM leader information
    gv_tm_info.initialize();
    tm_xarm_initialize();

    tm_log_event(DTM_TM_PROCESS_STARTUP, SQ_LOG_INFO, "DTM_TM_PROCESS_STARTUP", 
                 -1,-1,gv_tm_info.nid());

    /*lv_leader_error =*/ msg_mon_tm_leader_set(&lv_leader_nid,
                      &lv_leader_pid, la_leader_name);
    gv_tm_info.lead_tm_nid(lv_leader_nid);

    TMTrace(1, ("tm_main_initialize - lead dtm node id:%d\n", lv_leader_nid)); 
    if (lv_leader_nid < 0  || lv_leader_nid >= MAX_NODES)
    {
        tm_log_event(DTM_TM_LEADTM_BAD, SQ_LOG_CRIT, "DTM_TM_LEADTM_BAD",
                     -1, -1, gv_tm_info.nid(), -1, -1, -1, -1, -1, -1, -1, 
                     -1, -1, -1, -1, -1, -1, NULL, lv_leader_nid);
        TMTrace(1, ("tm_main_initialize - bad lead dtm node id:%d\n", lv_leader_nid)); 
        abort();
    }
    if (lv_leader_nid == gv_tm_info.nid())
    {
        tm_log_event (DTM_TM_LEADTM_SET, SQ_LOG_INFO , "DTM_TM_LEADTM_SET", 
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
                    NULL, /*string2*/
                    gv_tm_info.nid() /*node*/);
        gv_tm_info.lead_tm(true);
        
        //This must be system startup time. Wait for the events
        // before performing system recovery.
        //The AM and TSE events will be implemented in the future.
        //msg_mon_event_wait (AM_TLOG_FIXUP_COMPLETED_EVENT_ID, &lv_event_len, la_event_data);
        //msg_mon_event_wait (TSE_START_EVENT_ID, &lv_event_len, la_event_data);
        if (gv_tm_info.incarnation_num() == 0) {
          TMTrace(2, ("tm_main_initialize - waiting for the start event\n")); 
          msg_mon_event_wait (DTM_START_EVENT_ID, &lv_event_len, la_event_data);
          TMTrace(2, ("tm_main_initialize - got the start event\n")); 
        }
    }

     //Start timer thread
     TMTrace(1, ("tm_main_initialize, Starting timer Thread.\n")); 
     tm_start_timerThread();

     //Start example thread
     //tm_start_exampleThread();

     tm_start_auditThread();

     // Initialize the XA TM Library
     xaTM_initialize(gv_tm_info.iv_trace_level, gv_tm_info.tm_stats(), gv_tm_info.tmTimer());

     // Initialize the HBase TM Library
     HbaseTM_initialize(gv_tm_info.iv_trace_level, gv_tm_info.tm_stats(), gv_tm_info.tmTimer(), gv_tm_info.nid());

     if (gv_tm_info.lead_tm())
     {
        TMTrace(1, ("tm_main_initialize, I am Lead TM, $TM%d.\n", 
                     gv_tm_info.nid())); 
     }
    // open all tms before recovery, will return if not the lead
    gv_tm_info.open_other_tms();

    TMTrace(1, ("main : Lead DTM is on node %d\n", lv_leader_nid));

    if (gv_tm_info.lead_tm())
    {
       // init_and_recover_rms() will invoke system recovery if this
       // the Lead TM.
       gv_tm_info.schedule_init_and_recover_rms();
       gv_wait_interval = LEAD_DTM_WAKEUP_INTERVAL/10;
    }

    TMTrace(1, ("main : initialize complete, pid : %d, nid %d, cp interval %d\n", 
                     gv_tm_info.pid(), gv_tm_info.nid(), (gv_tm_info.cp_interval()/60000))); 

} 


// ----------------------------------------------------------------
// main method
// ----------------------------------------------------------------
int main(int argc, char *argv[]) 
{
    int16           lv_ret;
    int32           lv_my_nid;
    int32           lv_my_pid;
    BMS_SRE         lv_sre;

    CALL_COMP_DOVERS(tm, argc, argv);

    const int l_size = 20;
    int l_idx = 0;
    typedef struct l_element_
    {
        int16 lv_ret;
        BMS_SRE lv_sre;
    } l_element;

    l_element l_array[l_size];


    // get our pid info and initialize
    msg_init(&argc, &argv);

    // get our pid info and initialize
    msg_mon_get_my_info2(&lv_my_nid, // mon node-id
                         &lv_my_pid, // mon process-id
                         NULL,       // mon name
                         0,       // mon name-len
                         NULL,       // mon process-type
                         NULL,       // mon zone-id
                         NULL,       // os process-id
                         NULL,       // os thread-id
                         NULL);      // component-id
    gv_tm_info.nid (lv_my_nid);
    gv_tm_info.pid (lv_my_pid);
#ifdef MULTITHREADED_TM
    XWAIT(0, -2);
#endif

    msg_mon_process_startup(true); // server?
    msg_debug_hook ("tm.hook", "tm.hook");
    tm_init_logging();
//    msg_mon_tmsync_register(tm_sync_cb);
    msg_mon_enable_mon_messages (1);
    msg_enable_priority_queue();
    // allow the DTM to use all the message descriptors
    XCONTROLMESSAGESYSTEM(XCTLMSGSYS_SETRECVLIMIT,XMAX_SETTABLE_RECVLIMIT);  
    XCONTROLMESSAGESYSTEM(XCTLMSGSYS_SETSENDLIMIT,SEABED_MAX_SETTABLE_SENDLIMIT_TM);
    tm_main_initialize();

    TMTrace(1, ("thread_main - back from tm_main_initialize.\n"));

    // Added this block when removing the TM sync mechanism
    gv_tm_info.set_txnsvc_ready(TXNSVC_UP);
    gv_tm_info.can_takeover(true);

    bool lv_tm_ready_generated = generate_tm_ready_if_necessary();

    if (! gv_tm_info.lead_tm() && !lv_tm_ready_generated) {
      gv_tm_info.schedule_init_and_recover_rms();
    }

    for(;;) 
    {
        int lv_msg_count = 0;

        if ((gv_tm_info.state_shutdown()) &&
           (!gv_tm_info.shutdown_coordination_started()))
        {
            tm_shutdown_helper();
        }

        XWAIT(LREQ, (int)gv_wait_interval); // 10 ms units
        do 
        {
            lv_ret = BMSG_LISTEN_((short *) &lv_sre, // sre
                                 BLISTEN_ALLOW_IREQM, 
                                 0);                   // listenertag
            l_array[l_idx].lv_ret = lv_ret;
            memcpy((void *) &l_array[l_idx].lv_sre, (void *) &lv_sre, sizeof(lv_sre));
            if (l_idx >= l_size-1)
                l_idx = 0;
            else
                l_idx++;

            if (lv_ret != BSRETYPE_NOWORK)
                 tm_process_msg(&lv_sre);

            // come up for air to allow control point processing if need be
            if (lv_msg_count++ > 100)
                 break;

        } while (lv_ret != BSRETYPE_NOWORK);
    }
}






