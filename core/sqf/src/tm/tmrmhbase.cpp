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


// ----------------------------------------------------------------
// RM_Info_HBASE::RM_Info_HBASE 
// Purpose : Default constructor
// ----------------------------------------------------------------
RM_Info_HBASE::RM_Info_HBASE()
{
   TMTrace (2, ("RM_Info_HBASE::RM_Info_HBASE Default Constructor : ENTRY.\n"));
}

// ----------------------------------------------------------------
// RM_Info_HBASE::num_rm_failed
// Purpose : Count and return the number of RMs in failed
// state.  Currently returning 0.
// Periodically the TM will attempt to reopen any failed
// RMs.  This is done by the Timer thread.
// ----------------------------------------------------------------
int32 RM_Info_HBASE::num_rm_failed(CTmTxBase *pp_txn)
{
   int32 lv_count = 0;

   if (pp_txn)
   {
      TMTrace (2, ("RM_Info_HBASE::num_rm_failed ENTRY : Txn ID (%d,%d).\n",
               pp_txn->node(), pp_txn->seqnum()));
      lv_count = gv_HbaseTM.failedRegions(pp_txn->legacyTransid());
   }
   else
   {
      TMTrace(2, ("RM_Info_HBASE::num_rm_failed : ENTRY : Get the total number of failed RMs.\n"));
      lv_count = gv_HbaseTM.failedRegions(0);
   }

   TMTrace (2, ("RM_Info_HBASE::num_rm_failed EXIT Returned %d failed RMs.\n", lv_count));
   return lv_count;
} //RM_Info_HBASE::num_rm_failed


// ----------------------------------------------------------------
// RM_Info_HBASE::num_rm_partic
// Purpose : Returns the number of participating branches.
// ----------------------------------------------------------------
int32 RM_Info_HBASE::num_rm_partic(CTmTxBase *pp_txn)
{
   int32 lv_count = 0;

   if (pp_txn)
   {
      TMTrace (2, ("RM_Info_HBASE::num_rm_partic ENTRY : Txn ID (%d,%d).\n",
               pp_txn->node(), pp_txn->seqnum()));
      lv_count = gv_HbaseTM.participatingRegions(pp_txn->legacyTransid());
   }
   else
   {
      TMTrace(1, ("RM_Info_HBASE::num_rm_partic : Error, Transaction pointer not supplied!.\n"));
      tm_log_event(DTM_RMINFOHBASE_NULLTXNID, SQ_LOG_CRIT, "DTM_RMINFOHBASE_NULLTXNID");
      // TODO Total failed RMs.
      //abort();
   }

   TMTrace (2, ("RM_Info_HBASE::num_rm_partic EXIT Returned %d failed RMs.\n", lv_count));
   return lv_count;
} //RM_Info_HBASE::num_rm_partic


// ----------------------------------------------------------------
// RM_Info_HBASE::init_rms
// Purpose : Initialize branches
// Nothing to do here for now
// ----------------------------------------------------------------
void RM_Info_HBASE::init_rms(CTmTxBase *pp_txn, bool pv_partic)
{
   if (pp_txn)
   {
      TMTrace (2, ("RM_Info_HBASE::init_rms ENTRY : Txn ID (%d,%d), partic flag %d.\n",
               pp_txn->node(), pp_txn->seqnum(), pv_partic));
      // Nothing to do.
   }
   else
   {
      TMTrace(1, ("RM_Info_HBASE::init_rms : Error, Transaction pointer not supplied!.\n"));
      tm_log_event(DTM_RMINFOHBASE_NULLTXNID, SQ_LOG_CRIT, "DTM_RMINFOHBASE_NULLTXNID");
      // TODO Total failed RMs.
      //abort();
   }

   TMTrace (2, ("RM_Info_HBASE::init_rms EXIT.\n"));
} //RM_Info_HBASE::init_rms


// ----------------------------------------------------------------
// RM_Info_HBASE::num_rms_unresolved
// Purpose : Returns the number of RMs/branches which have the 
// iv_partic flag set but don't have the iv_resolved flag set.
// This is used to identify late TSE checkins (ax_reg requests)
// while processing phase 1 & 2.
// ----------------------------------------------------------------
int32 RM_Info_HBASE::num_rms_unresolved(CTmTxBase *pp_txn)
{
   int32 lv_count = 0;

   if (pp_txn)
   {
      TMTrace (2, ("RM_Info_HBASE::num_rms_unresolved ENTRY : Txn ID (%d,%d).\n",
               pp_txn->node(), pp_txn->seqnum()));
      lv_count = gv_HbaseTM.unresolvedRegions(pp_txn->legacyTransid());
   }
   else
   {
      TMTrace(1, ("RM_Info_HBASE::num_rms_unresolved : Error, Transaction pointer not supplied!.\n"));
      tm_log_event(DTM_RMINFOHBASE_NULLTXNID, SQ_LOG_CRIT, "DTM_RMINFOHBASE_NULLTXNID");
      // TODO Total failed RMs.
      //abort();
   }

   TMTrace (2, ("RM_Info_HBASE::num_rms_unresolved EXIT Returned %d failed RMs.\n", lv_count));
   return lv_count;
} //RM_Info_HBASE::num_rms_unresolved


// ----------------------------------------------------------------
// RM_Info_HBASE::reset_resolved
// Purpose : Reset all branches resolved flags.  This is used to 
// reset the flags after prepare phase to all commit_branches() to
// find any ax_reg requests which arrive during phase 2.
// ----------------------------------------------------------------
void RM_Info_HBASE::reset_resolved(CTmTxBase *pp_txn)
{
   TMTrace (2, ("RM_Info_HBASE::reset_resolved ENTRY\n"));

   //Nothing to do

   TMTrace (2, ("RM_Info_HBASE::reset_resolved EXIT.\n"));
} //RM_Info_HBASE::reset_resolved


// --------------------------------------------------------------
// Branch stuff below
// --------------------------------------------------------------

// ---------------------------------------------------------------------------
// rollback_branches
// Purpose : Pass the rollback request to HBase TM Library
// ---------------------------------------------------------------------------
int32 RM_Info_HBASE::rollback_branches (CTmTxBase *pp_txn,
                                        int64 pv_flags,
                                        CTmTxMessage * pp_msg,
                                        bool pv_error_condition)
{
   TMTrace (2, ("RM_Info_HBASE::rollback_branches, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
                pp_txn->node(), pp_txn->seqnum(), pv_flags));

   short lv_err = gv_HbaseTM.abortTransaction(pp_txn->legacyTransid());

   TMTrace (2, ("RM_Info_HBASE::rollback_branches, Txn ID (%d,%d), EXIT, UnResolved branches %d.\n",
                pp_txn->node(), pp_txn->seqnum(), num_rms_unresolved(pp_txn)));
   return lv_err;
} //rollback_branches


// ---------------------------------------------------------------------------
// commit_branches
// Purpose : Send out commit (phase 2) to HBase TM Library.
// ---------------------------------------------------------------------------
int32 RM_Info_HBASE::commit_branches (CTmTxBase *pp_txn,
                                      int64 pv_flags, CTmTxMessage * pp_msg)
{
   TMTrace (2, ("RM_Info_HBASE::commit_branches, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
                pp_txn->node(), pp_txn->seqnum(), pv_flags));

   short lv_err = gv_HbaseTM.doCommit(pp_txn->legacyTransid());

   TMTrace (2, ("RM_Info_HBASE::commit_branches, Txn ID (%d,%d), EXIT, UnResolved branches %d, error %d.\n",
                pp_txn->node(), pp_txn->seqnum(), num_rms_unresolved(pp_txn), lv_err));
   return lv_err;
} // commit_branches


// ---------------------------------------------------------------------------
// completeRequest_branches
// Purpose : Wait for Phase 2 commit or rollback to complete
// ---------------------------------------------------------------------------
int32 RM_Info_HBASE::completeRequest_branches (CTmTxBase *pp_txn)
{
   TMTrace (2, ("RM_Info_HBASE::completeRequest_branches, Txn ID (%d,%d), ENTRY\n",
                pp_txn->node(), pp_txn->seqnum()));

   short lv_err = gv_HbaseTM.completeRequest(pp_txn->legacyTransid());

   TMTrace (2, ("RM_Info_HBASE::completeRequest_branches, Txn ID (%d,%d), EXIT.\n",
                pp_txn->node(), pp_txn->seqnum()));
   return lv_err;
} // completeRequest_branches


// ---------------------------------------------------------------------------
// RM_Info_HBASE::end_branches
// Purpose - Doesn't really do anything for HBase TM Library
// ---------------------------------------------------------------------------
int32 RM_Info_HBASE::end_branches (CTmTxBase *pp_txn, int64 pv_flags)
{
   short lv_err = FEOK;

   TMTrace (2, ("RM_Info_HBASE::end_branches, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
                pp_txn->node(), pp_txn->seqnum(), pv_flags));

   // Nothing to do here!

   TMTrace (2, ("RM_Info_HBASE::end_branches, Txn ID (%d,%d), EXIT, UnResolved branches %d.\n",
                pp_txn->node(), pp_txn->seqnum(), num_rms_unresolved(pp_txn)));
   return lv_err;
} //RM_Info_HBASE::end_branches

// ---------------------------------------------------------------
// forget_heur_branches
// Purpose : Heuristic forget
// --------------------------------------------------------------
int32 RM_Info_HBASE::forget_heur_branches (CTmTxBase *pp_txn, int64 pv_flags)
{
    int32 lv_error = FEOK;

    TMTrace (2, ("RM_Info_HBASE::forget_heur_branches ENTRY : ID (%d,%d), flags " PFLL "\n",
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
            TMTrace (1, ("RM_Info_HBASE::forget_heur_branches - Invalid branch state\n"));
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
int32 RM_Info_HBASE::forget_branches (CTmTxBase *pp_txn, int64 pv_flags)
{
   int32 lv_err = FEOK;

   TMTrace (2, ("RM_Info_HBASE::forget_branches, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
                pp_txn->node(), pp_txn->seqnum(), pv_flags));

   // Nothing to do here!

   TMTrace (2, ("RM_Info_HBASE::forget_branches, Txn ID (%d,%d), EXIT, UnResolved branches %d.\n",
                pp_txn->node(), pp_txn->seqnum(), num_rms_unresolved(pp_txn)));
   return lv_err;
} //forget_branches


// ------------------------------------------------------------
// prepare_branches
// Purpose : Send prepare to HBase TM Library
// ------------------------------------------------------------
int32 RM_Info_HBASE::prepare_branches (CTmTxBase *pp_txn, int64 pv_flags, CTmTxMessage * pp_msg)
{
   TMTrace (2, ("RM_Info_HBASE::prepare_branches, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
                pp_txn->node(), pp_txn->seqnum(), pv_flags));

   pp_msg->response()->u.iv_end_trans.iv_err_str_len = 
     sizeof(pp_msg->response()->u.iv_end_trans.iv_err_str);
   
   short lv_err = gv_HbaseTM.prepareCommit(pp_txn->legacyTransid(),
                             pp_msg->response()->u.iv_end_trans.iv_err_str,
                             pp_msg->response()->u.iv_end_trans.iv_err_str_len);

   TMTrace (2, ("RM_Info_HBASE::prepare_branches, Txn ID (%d,%d), EXIT, Error %d, UnResolved branches %d.\n",
                pp_txn->node(), pp_txn->seqnum(), lv_err, num_rms_unresolved(pp_txn)));
   return lv_err;
} //RM_Info_HBASE::prepare_branches

//------------------------------------------------------------------------------
// start_branches
// Purpose - Call  beginTransaction against the HBase TM
// Library.
//------------------------------------------------------------------------------
int32 RM_Info_HBASE::start_branches (CTmTxBase *pp_txn,  int64 pv_flags, CTmTxMessage * pp_msg)
{   
   int32 lv_err = FEOK;
   int64 lv_transid = pp_txn->legacyTransid();
   int64 lv_transidIn = lv_transid;


   TMTrace (2, ("RM_Info_HBASE::start_branches, Txn ID (%d,%d), ENTRY, flags " PFLL "\n",
                pp_txn->node(), pp_txn->seqnum(), pv_flags));

    lv_err = gv_HbaseTM.beginTransaction(&lv_transid);

   if ((lv_transid != lv_transidIn) || (lv_err != 0))
   {
      if(lv_err != 0) {
         TMTrace (1, ("RM_Info_HBASE::start_branches, Txn ID (%d,%d), Error returned from HBase beginTransaction",
                  pp_txn->node(), pp_txn->seqnum()));
      }
      else { 
         TMTrace (1, ("RM_Info_HBASE::start_branches, Txn ID (%d,%d), Error returned transid " PFLLX 
                  " != entered transid " PFLLX ".\n",
                  pp_txn->node(), pp_txn->seqnum(), lv_transid, lv_transidIn));
      }
      tm_log_event(DTM_HBASE_BEGIN_TXN_ERROR, SQ_LOG_CRIT, "DTM_HBASE_BEGIN_TXN_ERROR",
                   -1,-1,pp_txn->node(),pp_txn->seqnum(),-1,-1,-1,-1,-1,-1,-1,-1,-1,lv_transidIn,lv_transid);
      abort();
   }

   TMTrace (2, ("RM_Info_HBASE::start_branches, Txn ID (%d,%d), EXIT returning %d, UnResolved branches %d.\n",
                pp_txn->node(), pp_txn->seqnum(), lv_err, num_rms_unresolved(pp_txn)));
   return lv_err;
} // start_branches


//------------------------------------------------------------------------------
// registerRegion
// Purpose - Call  register a region  for this transaction.
//------------------------------------------------------------------------------
int32 RM_Info_HBASE::registerRegion (CTmTxBase *pp_txn,  int64 pv_flags, CTmTxMessage * pp_msg)
{
   int32 lv_err = FEOK;
   int64 lv_transid = pp_txn->legacyTransid();
   int64 lv_startid = pp_msg->request()->u.iv_register_region.iv_startid;

   TMTrace (2, ("RM_Info_HBASE::registerRegion ENTRY : Txn ID (%d,%d), ENTRY, startId: %ld, flags " PFLL ", region %s.\n",
                pp_txn->node(), pp_txn->seqnum(), pv_flags, lv_startid,
                pp_msg->request()->u.iv_register_region.ia_regioninfo2));

    lv_err = gv_HbaseTM.registerRegion(lv_transid,
                   pp_msg->request()->u.iv_register_region.iv_startid,
                   pp_msg->request()->u.iv_register_region.iv_port,
                   pp_msg->request()->u.iv_register_region.ia_hostname,
                   pp_msg->request()->u.iv_register_region.iv_hostname_length,
                   pp_msg->request()->u.iv_register_region.iv_startcode,
                   pp_msg->request()->u.iv_register_region.ia_regioninfo2,
                   pp_msg->request()->u.iv_register_region.iv_regioninfo_length
                   );

   TMTrace (2, ("RM_Info_HBASE::registerRegion EXIT : Txn ID (%d,%d), returning %d.\n",
                pp_txn->node(), pp_txn->seqnum(), lv_err));
   return lv_err;
} // registerRegion

//------------------------------------------------------------------------------
// hb_ddl_operation
// Purpose: Call hb_ddl_operation for this transaction
// ------------------------------------------------------------------------------
int32 RM_Info_HBASE::hb_ddl_operation(CTmTxBase *pp_txn, int64 pv_flags, CTmTxMessage * pp_msg)
{
   int32 lv_err = FEOK;
   int64 lv_transid = pp_txn->legacyTransid();

   char *buffer_tbldesc = new char[pp_msg->request()->u.iv_ddl_request.ddlreq_len];
   int pv_tbldesclen;
   int pv_numsplits;
   int pv_keylen;

   int len;
   int len_aligned;
   int index;
   char *ddlbuffer;
   char **buffer_keys;

   char **buffer_opts;
   int pv_numtblopts;
   int pv_tbloptslen;

   TMTrace (2, ("RM_Info_HBASE::hb_ddl_operation ENTRY\n"));

   pp_msg->response()->u.iv_ddl_response.iv_err_str_len = 
  	 sizeof(pp_msg->response()->u.iv_ddl_response.iv_err_str);
   
   switch(pp_msg->request()->u.iv_ddl_request.ddlreq_type)
   {
      case TM_DDL_CREATE:
         len = sizeof(Tm_Req_Msg_Type);
         len_aligned = 8*((len + 7)/8);

         pv_numsplits = pp_msg->request()->u.iv_ddl_request.crt_numsplits;
         pv_keylen = pp_msg->request()->u.iv_ddl_request.crt_keylen;

         pv_tbldesclen = pp_msg->request()->u.iv_ddl_request.ddlreq_len;
         memcpy(buffer_tbldesc, pp_msg->request()->u.iv_ddl_request.ddlreq, pv_tbldesclen);

         ddlbuffer = pp_msg->getBuffer();

         if(ddlbuffer == NULL) {
            buffer_keys = NULL;
            lv_err = gv_HbaseTM.createTable(lv_transid,
                         buffer_tbldesc,
                         pv_tbldesclen,
                         NULL,
                         0,
                         0,
                         pp_msg->response()->u.iv_ddl_response.iv_err_str,
                         pp_msg->response()->u.iv_ddl_response.iv_err_str_len);
         }
         else {
            buffer_keys = new char *[pv_numsplits];

            index = len_aligned;
            for(int i=0; i<pp_msg->request()->u.iv_ddl_request.crt_numsplits ; i++)
            {
               buffer_keys[i] = new char[pp_msg->request()->u.iv_ddl_request.crt_keylen];
               memcpy(buffer_keys[i],(char*)(ddlbuffer)+index , pv_keylen);
               index = index + pv_keylen;
             }
             lv_err = gv_HbaseTM.createTable(lv_transid,
                         buffer_tbldesc,
                         pv_tbldesclen,
                         buffer_keys,
                         pv_numsplits,
                         pv_keylen,
                         pp_msg->response()->u.iv_ddl_response.iv_err_str,
                         pp_msg->response()->u.iv_ddl_response.iv_err_str_len);
         }

         if(ddlbuffer!=NULL) {
            for(int i=0; i<pp_msg->request()->u.iv_ddl_request.crt_numsplits ; i++)
               delete buffer_keys[i];
            delete[] buffer_keys;
         }
         break;
      case TM_DDL_DROP:
         lv_err = gv_HbaseTM.dropTable(lv_transid,
                         pp_msg->request()->u.iv_ddl_request.ddlreq,
                         pp_msg->request()->u.iv_ddl_request.ddlreq_len,
                         pp_msg->response()->u.iv_ddl_response.iv_err_str,
                         pp_msg->response()->u.iv_ddl_response.iv_err_str_len);
         break;
      case TM_DDL_TRUNCATE:
         lv_err = gv_HbaseTM.regTruncateOnAbort(lv_transid,
                         pp_msg->request()->u.iv_ddl_request.ddlreq,
                         pp_msg->request()->u.iv_ddl_request.ddlreq_len,
                         pp_msg->response()->u.iv_ddl_response.iv_err_str,
                         pp_msg->response()->u.iv_ddl_response.iv_err_str_len);
      case TM_DDL_ALTER:
        
         len = sizeof(Tm_Req_Msg_Type);
         len_aligned = 8*((len + 7)/8);

         pv_numtblopts = pp_msg->request()->u.iv_ddl_request.alt_numopts;
         pv_tbloptslen = pp_msg->request()->u.iv_ddl_request.alt_optslen;

         ddlbuffer = pp_msg->getBuffer();

         buffer_opts = new char *[pv_numtblopts];

         index = len_aligned;
         for(int i=0; i<pp_msg->request()->u.iv_ddl_request.alt_numopts; i++)
         {
            buffer_opts[i] = new char[pp_msg->request()->u.iv_ddl_request.alt_optslen];
            memcpy(buffer_opts[i],(char*)(ddlbuffer)+index , pv_tbloptslen);
            index = index + pv_tbloptslen;
         }

         lv_err = gv_HbaseTM.alterTable(lv_transid,
                         pp_msg->request()->u.iv_ddl_request.ddlreq,
                         pp_msg->request()->u.iv_ddl_request.ddlreq_len,
                         buffer_opts,
                         pv_numtblopts,
                         pv_tbloptslen,
                         pp_msg->response()->u.iv_ddl_response.iv_err_str,
                         pp_msg->response()->u.iv_ddl_response.iv_err_str_len);

         if(ddlbuffer!=NULL) {
            for(int i=0; i<pp_msg->request()->u.iv_ddl_request.alt_numopts; i++)
               delete buffer_opts[i];
            delete[] buffer_opts;
         }

         break;
      default:
         TMTrace (1, ("RM_Info_HBASE::hb_ddl_operation : Invalid ddl operation\n"));
         break;
   }
   TMTrace (2, ("RM_Info_HBASE::hb_ddl_operation EXIT : Txn ID (%d,%d), returning %d.\n",
                pp_txn->node(), pp_txn->seqnum(), lv_err));

   return lv_err;

} //hb_ddl_operation


//------------------------------------------------------------------------------
// shutdown_branches
// Purpose - shutdown HBASE branches
// Library.
//------------------------------------------------------------------------------
int32 RM_Info_HBASE::shutdown_branches (bool pv_leadTM, bool pv_clean)
{   
   TMTrace (2, ("RM_Info_HBASE::shutdown_branches ENTRY Lead TM %d, clean? %d.\n",
            pv_leadTM, pv_clean));

   gv_HbaseTM.shutdown();

   TMTrace (2, ("RM_Info_HBASE::shutdown_branches EXIT.\n"));
   return FEOK;
} // shutdown_branches
