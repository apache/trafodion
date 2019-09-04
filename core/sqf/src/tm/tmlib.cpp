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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// seabed includes
#include "seabed/ms.h"
#include "seabed/fs.h"
#include "seabed/trace.h"
#include "seabed/thread.h"
#include "javaobjectinterfacetm.h"

// tm includes  
#include "dtm/tm_util.h"
//#include "tmtx.h"
#include "tmlib.h"
#include "tmlogging.h"

//extern int HbaseTM_initiate_stall(int where);  Shouldn't need these here.
//extern HashMapArray* HbaseTM_process_request_regions_info();


//==== For the JNI call to RMInterface.cleartransaction - begin
#include <iostream>
#include "jni.h"

//
// ===========================================================================
// ===== Class TMLIB
// ===========================================================================
const char *TMLIB::hbasetxclient_classname = "org/trafodion/dtm/HBaseTxClient";
const char *TMLIB::rminterface_classname = "org/apache/hadoop/hbase/client/transactional/RMInterface";

jclass TMLIB::hbasetxclient_class = NULL;
jclass TMLIB::RMInterface_class = NULL;

JavaMethodInit* TMLIB::TMLibJavaMethods_ = NULL;
bool TMLIB::javaMethodsInitialized_ = false;
TM_Mutex *TMLIB::initMutex_ = NULL;
bool TMLIB::enableCleanupRMInterface_ = false;

// ==============================================================
// === HBaseTM return codes
// ==============================================================
enum HBTM_RetCode {
   RET_OK = 0,
   RET_NOTX,
   RET_READONLY,
   RET_ADD_PARAM,
   RET_EXCEPTION,
   RET_HASCONFLICT,
   RET_IOEXCEPTION,
   RET_NOCOMMITEX,
   RET_LAST
};


// global externsTMLIB_ThreadTxn_Object
__thread  TMLIB_ThreadTxn_Object *gp_trans_thr;
TMLIB gv_tmlib;
bool gv_tmlib_initialized = false;

// --------------------------------------------------------------------
// Helper methods
// --------------------------------------------------------------------

// block signal 60 as soon as possible upon starting
#define SQ_LIO_SIGNAL_REQUEST_REPLY (SIGRTMAX - 4)
static int tm_rtsigblock_proc() {
    sigset_t lv_sig_set;
    // Setup signal handling
    sigemptyset(&lv_sig_set);
    sigaddset(&lv_sig_set, SQ_LIO_SIGNAL_REQUEST_REPLY);
    int err = pthread_sigmask(SIG_BLOCK, &lv_sig_set, NULL);
    if (err)
        abort();
    //fprintf(stderr,"Blocked signal %d.\n",SQ_LIO_SIGNAL_REQUEST_REPLY);
    fflush(stderr);
    return 0;
}

//----------------------------------------------------------------------------
// Map HBase-trx error to DTM
//----------------------------------------------------------------------------
short HBasetoTxnError(short pv_HBerr) 
{
   switch (pv_HBerr)
   {
   case RET_OK: return FEOK;
   case RET_NOTX: return FENOTRANSID;
   case RET_READONLY: return FEOK; //Read-only reply is ok
   case RET_ADD_PARAM: return FEBOUNDSERR;
   case RET_EXCEPTION: return FETRANSEXCEPTION;
   case RET_HASCONFLICT: return FEHASCONFLICT;
   case RET_IOEXCEPTION: return FETRANSIOEXCEPTION;
   case RET_NOCOMMITEX: return FEABORTEDTRANSID;
   default: 
      printf("Unknown error %d encountered, returning FETRANSERRUNKNOWN\n.", pv_HBerr);
      return FETRANSERRUNKNOWN;
   }
}


// -------------------------------------------------------------------
// tmlib_trace_enabled
// -- will return if tracing is enabled, and will initialize if not 
//    already done
// -------------------------------------------------------------------
bool tmlib_trace_enabled(int pv_level)
{
   static bool lv_trace_enabled = false;
   static bool  lv_trace_on = false;
   static int  lv_trace_level = 0;
   bool        lv_unique = false;

   if (!lv_trace_enabled)
    {
       ms_getenv_bool("TMLIB_TRACE", &lv_trace_on);

       if (lv_trace_on)
       {
          lv_trace_level = 1; // default
          ms_getenv_int("TMLIB_TRACE_DETAIL", &lv_trace_level); //set the detail
          ms_getenv_bool("TMLIB_TRACE_UNIQUE", &lv_unique);
          const char *lp_file = ms_getenv_str ("TMLIB_TRACE_FILE");
          if (lp_file != NULL)
          {
              char *lp_trace_file = (char *)lp_file;
              trace_init(lp_trace_file, lv_unique, NULL, false);
          }
          else 
              trace_init((char *)"tmlib_trace", lv_unique, NULL, false);
          trace_printf("TMLIB_TRACE : Process Initialize, trace level = %d\n", lv_trace_level);
  
       }
       lv_trace_enabled = true;
   }
   if (lv_trace_level >= pv_level)
      return true;
   return false;
}

// ---------------------------------------------------------------
// tmlib_zero_transid
// Purpose - return if the transid is all zeros
// --------------------------------------------------------------
bool tmlib_zero_transid(TM_Transid_Type *pp_transid)
{
    if (pp_transid)
    {
        if ((pp_transid->id[0] == 0) &&
            (pp_transid->id[1] == 0) &&
            (pp_transid->id[2] == 0) &&
            (pp_transid->id[3] == 0))
            return true;
        else
            return false;
    }

    // null transid
    return true;
}
// ----------------------------------------------------------------
// tmlib_init_req_hdr
// Purpose - Initialize the header field for a TM Library
// message request.
// ----------------------------------------------------------------
int tmlib_init_req_hdr(short req_type, Tm_Req_Msg_Type *pp_req)
{
   pp_req->iv_msg_hdr.dialect_type = DIALECT_TM_SQ;
   pp_req->iv_msg_hdr.rr_type.request_type = req_type;
   pp_req->iv_msg_hdr.version.request_version = 
                              TM_SQ_MSG_VERSION_CURRENT;
   pp_req->iv_msg_hdr.miv_err.minimum_interpretation_version = 
                              TM_SQ_MSG_VERSION_CURRENT;

   return 0;
}

// ------------------------------------------------------------
// set_transid
// Purpose - need this for now to convert from seabed transid
// to ours
// ------------------------------------------------------------
 void tmlib_set_transid_from_ms ( TM_Transid *pp_transid, MS_Mon_Transid_Type pv_transid2)
{
    TM_Transid_Type pv_ext_transid;
    pv_ext_transid.id[0] = pv_transid2.id[0];
    pv_ext_transid.id[1] = pv_transid2.id[1];
    pv_ext_transid.id[2] = pv_transid2.id[2];
    pv_ext_transid.id[3] = pv_transid2.id[3];

    *pp_transid = pv_ext_transid;
}

// ------------------------------------------------------------
// set_transid_startid
// Purpose - need this for now to convert from seabed transid
// to ours
// ------------------------------------------------------------
 void tmlib_set_transid_startid_from_ms ( TM_Transid *pp_transid, MS_Mon_Transid_Type pv_transid2, TM_Transseq_Type *pv_startid, MS_Mon_Transseq_Type pv_startid2)
{
    TM_Transid_Type pv_ext_transid;
    pv_ext_transid.id[0] = pv_transid2.id[0];
    pv_ext_transid.id[1] = pv_transid2.id[1];
    pv_ext_transid.id[2] = pv_transid2.id[2];
    pv_ext_transid.id[3] = pv_transid2.id[3];

    *pp_transid = pv_ext_transid;
    *pv_startid = pv_startid2;
}

void tmlib_set_ms_from_transid(TM_Transid_Type pp_transid, MS_Mon_Transid_Type *pp_transid2)
{
    pp_transid2->id[0] = pp_transid.id[0];
    pp_transid2->id[1] = pp_transid.id[1];
    pp_transid2->id[2] = pp_transid.id[2];
    pp_transid2->id[3] = pp_transid.id[3];
}

void tmlib_set_ms_from_transid_startid(TM_Transid_Type pp_transid, MS_Mon_Transid_Type *pp_transid2, TM_Transseq_Type pv_startid, MS_Mon_Transseq_Type *pv_startid2)
{
    pp_transid2->id[0] = pp_transid.id[0];
    pp_transid2->id[1] = pp_transid.id[1];
    pp_transid2->id[2] = pp_transid.id[2];
    pp_transid2->id[3] = pp_transid.id[3];
    *pv_startid2 = pv_startid;
}

// ------------------------------------------------------------
// tmlib_check_active_tx
// -- check if there is an active transaction
// ------------------------------------------------------------
short tmlib_check_active_tx ( )
{
    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

     if (gp_trans_thr->get_current() == NULL)
     {
         TMlibTrace(("TMLIB_TRACE : tmlib_check_active_tx returning FENOTRANSID\n"), 3);
         return FENOTRANSID;
      }
    
      return FEOK;
}

// -----------------------------------------------------------
// tmlib_check_miss_param
// -- check if the param is valid (i.e. not NULL)
// ----------------------------------------------------------
short tmlib_check_miss_param( void * pp_param)
{
    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // make sure there is space for pp_status
    if (pp_param == NULL)
    {
        TMlibTrace(("TMLIB_TRACE : tmlib_check_miss_param returning FEMISSPARM\n"), 1);
        return FEMISSPARM;
    }
    return FEOK;
}

// ------------------------------------------------------------
// tmlib_check_outstanding_ios
// -- check if there are outstanding_ios
// -----------------------------------------------------------
short tmlib_check_outstanding_ios()
{
    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();
    
    // make sure there is space for pp_status
    if (gp_trans_thr->get_current_ios())
    {
        TMlibTrace(("TMLIB_TRACE : tmlib_check_outstanding_ios returning "
            "FETRANSNOWAITOUT. %d outstanding ios.\n", gp_trans_thr->get_current_ios()), 1);
        return FETRANSNOWAITOUT;
    }
    return FEOK;
}

short tmlib_send_suspend(TM_Transid pv_transid, bool pv_coord_role, int pv_pid)
{
    short           lv_error;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    TMlibTrace(("TMLIB_TRACE : tmlib_send_suspend ENTRY\n"), 2);

    if (!gv_tmlib_initialized)
    {
        gv_tmlib.initialize();
    }

    tmlib_init_req_hdr(TM_MSG_TYPE_SUSPENDTRANSACTION, &lv_req);
    pv_transid.set_external_data_type (&lv_req.u.iv_suspend_trans.iv_transid);
    lv_req.u.iv_suspend_trans.iv_coord_role = pv_coord_role;
    lv_req.u.iv_suspend_trans.iv_pid = pv_pid;
    lv_req.u.iv_suspend_trans.iv_nid = gv_tmlib.iv_my_nid;

    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, pv_transid.get_node());

    TMlibTrace(("TMLIB_TRACE : tmlib_send_suspend (seq num %d) EXIT returning %d\n", 
        pv_transid.get_seq_num(), lv_error), 2);
    if (lv_error)
        return lv_error;

    if (lv_rsp.iv_msg_hdr.miv_err.error)
        return lv_rsp.iv_msg_hdr.miv_err.error;
    
    return FEOK;
}
// ----------------------------------------------------------------
// tmlib_callback
// Purpose - callback registered with Seabed upon startup, used
//           for File System Propagation
// ---------------------------------------------------------------
int tmlib_callback (MS_Mon_Tmlib_Fun_Type pv_fun, 
                    MS_Mon_Transid_Type pv_transid,
                    MS_Mon_Transid_Type *pp_transid_out)

{
    char       la_buf[DTM_STRING_BUF_SIZE];
    int        lv_return = FEOK;
    TM_Transid lv_transid;

   TMlibTrace(("TMLIB_TRACE : tmlib_callback ENTRY with function %d \n",
                     pv_fun), 2);

    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    tmlib_set_transid_from_ms (&lv_transid, pv_transid);

    switch (pv_fun) 
    {
    // called in the client to get the current transaction
    case TMLIB_FUN_GET_TX:
    {
         TMlibTrace(("TMLIB_TRACE : tmlib_callback, FUN_GET_TX\n"), 3);

         // active transaction
         if (gp_trans_thr->get_current() != NULL)
         {
             if (pp_transid_out == NULL)
                 lv_return = FEMISSPARM;
             else
             {
                 gp_trans_thr->increase_current_ios();
                 // otherwise, return the active transaction to propagate
                 tmlib_set_ms_from_transid(
                       gp_trans_thr->get_current()->getTransid()->get_data(), pp_transid_out);
                 TMlibTrace(("TMLIB_TRACE : tmlib_callback, FUN_GET_TX, returning seq num %d\n",
                          gp_trans_thr->get_current()->getTransid()->get_seq_num()), 3); 
             }
         }
         break;
    }

    // called in the server to register a propagated transaction
    case TMLIB_FUN_REG_TX:
    { 
        // if the tx is already active, increase the depth as we
        // cannot delete the transaction before the final reply.
        // in a nowait env, we can have more than 1 request come
        // in for the same tx
        TMlibTrace(("TMLIB_TRACE : tmlib_callback, TMLIB_FUN_REG_TX, seq num %d\n", lv_transid.get_seq_num()), 3);
        if ((gp_trans_thr->get_current()!= NULL) && (gp_trans_thr->get_current()->equal(lv_transid)))
        {
            gp_trans_thr->increase_current_depth();
        }
        else
        {
            // if its not active, add it if need be
            lv_return = gv_tmlib.add_or_update(lv_transid);
         }
        break;
    }

    // called in the server to clear the propagated transaction
    case TMLIB_FUN_CLEAR_TX:
    {
         // make sure we are clearing the proper transaction
         TMlibTrace(("TMLIB_TRACE : tmlib_callback, TMLIB_FUN_CLEAR_TX, seq num %d\n", 
                      lv_transid.get_seq_num()), 3);
      
        // transaction may have been aborted and was cleared out
        if (gp_trans_thr->get_current())
        {
            // If the transaction being cleared doesn't match the current transaction then 
            // we assume that Seabed is well behaved, but that the application has managed
            // to set the current transaction since the reinstate callback!  So we re-do
            // the reinstate, clear it and then set the current transaction back to what
            // it was when we were called.
            if (!(gp_trans_thr->get_current()->equal(lv_transid)))
            {
                sprintf(la_buf, "Warning: File System transaction to be cleared does not "
                        "match current transaction %d, assuming it changed since the reinstate.\n", 
                        gp_trans_thr->get_current()->getTransid()->get_seq_num());
                tm_log_write(DTM_LIB_TRANS_INVALID_ID, SQ_LOG_WARNING, la_buf);
                TMlibTrace(("TMLIB_TRACE : tmlib_callback, TMLIB_FUN_CLEAR_TX, %s\n", 
                      la_buf), 1);

                TM_Transaction *lp_saveTrans = gp_trans_thr->get_current();
                gv_tmlib.reinstate_tx (&lv_transid);
                gv_tmlib.clear_entry(lv_transid, true /*server*/, false);
                gp_trans_thr->set_current(lp_saveTrans);
            }
            else
               gv_tmlib.clear_entry (lv_transid, true /*server*/, false);
        }
        break;
    }

    // called in the client to reinstate a transaction that left the process
    case TMLIB_FUN_REINSTATE_TX:
    {
         TMlibTrace(("TMLIB_TRACE : tmlib_callback, TMLIB_FUN_REINSTATE_TX, seq num %d\n", 
                      lv_transid.get_seq_num()), 3);
        // if we have an active tx, it better be the same!
         if (gp_trans_thr->get_current())
         { 
             if (!(gp_trans_thr->get_current()->equal(lv_transid)))
                lv_return = FEINVTRANSID;
         } 
         else
             gv_tmlib.reinstate_tx (&lv_transid);

        /* We have to allow this for aborts with outstanding I/Os.
         if (!(gp_trans_thr->get_current()))
         {
             sprintf(la_buf, "Transaction reinstatement failed.\n");
             tm_log_write(DTM_LIB_INVALID_TRANS, SQ_LOG_CRIT, la_buf);
             abort();
         } */

         if (gp_trans_thr->get_current())
            gp_trans_thr->decrease_current_ios();
         else
            lv_return = FEINVTRANSID;
         break;
    }

    default:
    {
        sprintf(la_buf, "Seabed software fault - bad input pv_fun %d\n", pv_fun);
        TMlibTrace(("TMLIB_TRACE : tmlib_callback failed %s", la_buf), 1);
        tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, la_buf);
        abort();  // seabed software fault - bad input
        break;
    }
    }

    TMlibTrace(("TMLIB_TRACE : tmlib_callback EXIT with error %d\n",
                     lv_return), 2);

    return lv_return;
}

// ----------------------------------------------------------------
// tmlib_callback2
// Purpose - callback registered with Seabed upon startup, used
//           for File System Propagation (transId and StartId)
// ---------------------------------------------------------------
int tmlib_callback2 (MS_Mon_Tmlib_Fun_Type pv_fun,
                    MS_Mon_Transid_Type pv_transid,
                    MS_Mon_Transid_Type *pp_transid_out,
                    MS_Mon_Transseq_Type pv_startid,
                    MS_Mon_Transseq_Type *pp_startid_out)

{
    char       la_buf[DTM_STRING_BUF_SIZE];
    int        lv_return = FEOK;
    TM_Transid lv_transid;
    TM_Transseq_Type lv_startid;

   TMlibTrace(("TMLIB_TRACE : tmlib_callback2 ENTRY with function %d \n",
                     pv_fun), 2);

    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    tmlib_set_transid_startid_from_ms (&lv_transid, pv_transid, &lv_startid, pv_startid);

    switch (pv_fun)
    {
    // called in the client to get the current transaction
    case TMLIB_FUN_GET_TX:
    {
         TMlibTrace(("TMLIB_TRACE : tmlib_callback2, FUN_GET_TX\n"), 3);

         // active transaction
         if (gp_trans_thr->get_current() != NULL)
         {
             if ((pp_transid_out == NULL) || (pp_startid_out == NULL))
                 lv_return = FEMISSPARM;
             else
             {
                 gp_trans_thr->increase_current_ios();
                 // otherwise, return the active transaction to propagate
                 tmlib_set_ms_from_transid_startid(
                       gp_trans_thr->get_current()->getTransid()->get_data(), pp_transid_out,
                       gp_trans_thr->get_startid(), pp_startid_out);
                 TMlibTrace(("TMLIB_TRACE : tmlib_callback2, FUN_GET_TX, returning seq num %d and startid %ld\n",
                          gp_trans_thr->get_current()->getTransid()->get_seq_num(), gp_trans_thr->get_startid()), 3);
             }
         }
         break;
    }

    // called in the server to register a propagated transaction
    case TMLIB_FUN_REG_TX:
    {
        // if the tx is already active, increase the depth as we
        // cannot delete the transaction before the final reply.
        // in a nowait env, we can have more than 1 request come
        // in for the same tx
        TMlibTrace(("TMLIB_TRACE : tmlib_callback2, TMLIB_FUN_REG_TX, seq num %d\n", lv_transid.get_seq_num()), 3);
        if ((gp_trans_thr->get_current()!= NULL) && (gp_trans_thr->get_current()->equal(lv_transid)))
        {
            gp_trans_thr->increase_current_depth();
        }
        else
        {
            // if its not active, add it if need be
            lv_return = gv_tmlib.add_or_update(lv_transid, lv_startid);
         }
        break;
    }

    // called in the server to clear the propagated transaction
    case TMLIB_FUN_CLEAR_TX:
    {
         // make sure we are clearing the proper transaction
         TMlibTrace(("TMLIB_TRACE : tmlib_callback2, TMLIB_FUN_CLEAR_TX, seq num %d\n",
                      lv_transid.get_seq_num()), 3);

        // transaction may have been aborted and was cleared out
        if (gp_trans_thr->get_current())
        {
            // If the transaction being cleared doesn't match the current transaction then
            // we assume that Seabed is well behaved, but that the application has managed
            // to set the current transaction since the reinstate callback!  So we re-do
            // the reinstate, clear it and then set the current transaction back to what
            // it was when we were called.
            if (!(gp_trans_thr->get_current()->equal(lv_transid)))
            {
                sprintf(la_buf, "Warning: File System transaction to be cleared does not "
                        "match current transaction %d, assuming it changed since the reinstate.\n",
                        gp_trans_thr->get_current()->getTransid()->get_seq_num());
                tm_log_write(DTM_LIB_TRANS_INVALID_ID, SQ_LOG_WARNING, la_buf);
                TMlibTrace(("TMLIB_TRACE : tmlib_callback2, TMLIB_FUN_CLEAR_TX, %s\n",
                      la_buf), 1);

                TM_Transaction *lp_saveTrans = gp_trans_thr->get_current();
                TM_Transseq_Type lv_saveStartId = gp_trans_thr->get_startid();
                gv_tmlib.reinstate_tx (&lv_transid);
                gv_tmlib.clear_entry(lv_transid, true /*server*/, false);
                gp_trans_thr->set_current(lp_saveTrans);
                gp_trans_thr->set_startid(lv_saveStartId);
            }
            else
               gv_tmlib.clear_entry (lv_transid, true /*server*/, false);
        }
        break;
    }

    // called in the client to reinstate a transaction that left the process
    case TMLIB_FUN_REINSTATE_TX:
    {
         TMlibTrace(("TMLIB_TRACE : tmlib_callback2, TMLIB_FUN_REINSTATE_TX, seq num %d\n",
                      lv_transid.get_seq_num()), 3);
        // if we have an active tx, it better be the same!
         if (gp_trans_thr->get_current())
         {
             if (!(gp_trans_thr->get_current()->equal(lv_transid)))
                lv_return = FEINVTRANSID;
         }
         else{
             gv_tmlib.reinstate_tx (&lv_transid);
             gp_trans_thr->set_startid(lv_startid);
         }
        /* We have to allow this for aborts with outstanding I/Os.
         if (!(gp_trans_thr->get_current()))
         {
             sprintf(la_buf, "Transaction reinstatement failed.\n");
             tm_log_write(DTM_LIB_INVALID_TRANS, SQ_LOG_CRIT, la_buf);
             abort();
         } */

         if (gp_trans_thr->get_current())
            gp_trans_thr->decrease_current_ios();
         else
            lv_return = FEINVTRANSID;
         break;
    }

    default:
    {
        sprintf(la_buf, "Seabed software fault - bad input pv_fun %d\n", pv_fun);
        TMlibTrace(("TMLIB_TRACE : tmlib_callback2 failed %s", la_buf), 1);
        tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, la_buf);
        abort();  // seabed software fault - bad input
        break;
    }
    }

    TMlibTrace(("TMLIB_TRACE : tmlib_callback2 EXIT with error %d\n",
                     lv_return), 2);

    return lv_return;
}

// TOPL REGISTERTRANSACTION
short REGISTERREGION(long transid, long startid, int pv_port, char *pa_hostname, int pv_hostname_length, long pv_startcode, char *pa_regionInfo, int pv_regionInfo_length)
{
   short lv_error = FEOK;
   TM_Transaction *lp_trans = NULL;
   TM_Transid lv_transid((TM_Native_Type) transid);
   TM_Transseq_Type lv_startid((TM_Transseq_Type) startid);
   // instantiate a gp_trans_thr object for this thread if needed.
   TMlibTrace(("TMLIB_TRACE : REGISTERREGION ENTRY: transid: %ld, txid: (%d,%d), startId: %ld, port: %d, hostname %s, length: %d, startcode: %ld, regionInfo: %s, length: %d.\n",
            transid, lv_transid.get_node(), lv_transid.get_seq_num(), startid, pv_port, pa_hostname, pv_hostname_length, pv_startcode, pa_regionInfo, pv_regionInfo_length), 2);

   if (gp_trans_thr == NULL){
      TMlibTrace(("REGISTERREGION gp_trans_thr is null\n"), 2);
      gp_trans_thr = new TMLIB_ThreadTxn_Object();
      gp_trans_thr->set_startid(lv_startid);
   }

   TM_Transaction *lp_currTrans = gp_trans_thr->get_current();
   // Check if the passed-in transid is known to the thread
   // if so, make that Transid current
   if (lp_currTrans == NULL) {
      lp_currTrans = gp_trans_thr->get_trans (lv_transid.get_native_type());
      if (lp_currTrans != NULL) 
         gp_trans_thr->set_current(lp_currTrans);
   } 
   // Check if the thread's current transid matches the passed-in transid
   // If not, check if the passed-in transid is known to the thread
   // if so, make that Transid current
   else if (! lp_currTrans->equal(lv_transid)) {
      lp_currTrans = gp_trans_thr->get_trans (lv_transid.get_native_type());
      if (lp_currTrans != NULL) 
         gp_trans_thr->set_current(lp_currTrans);
   }
   TM_Transseq_Type lv_savedStartId = gp_trans_thr->get_startid();

   TMlibTrace(("REGISTERREGION lv_savedStartId is %ld.  Startid is %ld \n", (long) lv_savedStartId, startid), 2);
   if (lv_savedStartId != lv_startid){
      TMlibTrace(("REGISTERREGION setting lv_savedStartId to %ld. \n", startid), 2);
      lv_savedStartId = lv_startid;
      gp_trans_thr->set_startid(lv_startid);
   }
   if (lp_currTrans != NULL)
   {
      TMlibTrace(("TMLIB_TRACE : REGISTERREGION using current transid (%d,%d) and startId %ld.\n",
                  lv_transid.get_node(), lv_transid.get_seq_num(),lv_savedStartId ), 1);
      lv_error =  lp_currTrans->register_region(lv_savedStartId, pv_port, pa_hostname, pv_hostname_length, pv_startcode, pa_regionInfo, pv_regionInfo_length);
   }
   // Create a temp TM_transaction object to pass the trans id to REGION SERVER 
   else {
      lp_trans = new TM_Transaction();
      lp_trans->setTransid(lv_transid);
      lp_trans->setTag(gv_tmlib.new_tag());
      TMlibTrace(("TMLIB_TRACE : REGISTERREGION using transid (%d,%d) and startId (%ld) passed to REGISTERREGION.\n",
                     lv_transid.get_node(), lv_transid.get_seq_num(),lv_startid), 1);
     lv_error =  lp_trans->register_region(lv_startid, pv_port, pa_hostname, pv_hostname_length, pv_startcode, pa_regionInfo, pv_regionInfo_length);
      delete lp_trans;
   }

   TMlibTrace(("TMLIB_TRACE : REGISTERREGION EXIT: txid: (%d,%d), returning %d\n",
              lv_transid.get_node(), lv_transid.get_seq_num(), lv_error), 2);
   return lv_error;
} //REGISTERREGION

// -------------------------------------------------------------------
// CREATETABLE
//
// Purpose: send CREATETABLE message to the TM
// Params: pa_tabledesc, pv_tabledesc_length, pv_tblname, transid
// -------------------------------------------------------------------
short CREATETABLE(char *pa_tbldesc, int pv_tbldesc_length, char *pv_tblname, char** pv_keys, int pv_numsplits, int pv_keylen, long transid ,
		char* &pv_err_str, int &pv_err_len)
{
    TM_Transid lv_transid((TM_Native_Type) transid);
    short lv_error = FEOK;

    TMlibTrace(("TMLIB_TRACE : CREATETABLE ENTRY: txid: (%d,%d), tablename: %s, numsplits: %d, keylen %d\n",
       lv_transid.get_node(), lv_transid.get_seq_num(), pv_tblname,  pv_numsplits, pv_keylen), 2);

    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();
    TM_Transaction *lp_trans = gp_trans_thr->get_current();
    lv_error =  lp_trans->create_table(pa_tbldesc, pv_tbldesc_length, 
                pv_tblname, pv_keys, pv_numsplits, pv_keylen,
                pv_err_str, pv_err_len);

    TMlibTrace(("TMLIB_TRACE : CREATETABLE EXIT: txid: (%d,%d), returning %d\n",
       lv_transid.get_node(), lv_transid.get_seq_num(), lv_error), 2);

    return lv_error;
}

// -------------------------------------------------------------------
// REGTRUNCATEONABORT
//
// Purpose: send REGTRUNCATEONABORT message to the TM
// Params: pa_tabledesc, pv_tabledesc_length, pv_tblname, transid
// -------------------------------------------------------------------
short REGTRUNCATEONABORT(char *pv_tblname, int pv_tblname_len, long pv_transid,
                        char* &pv_err_str, int &pv_err_len)
{
    short lv_error = FEOK;
    TM_Transid lv_transid((TM_Native_Type) pv_transid);
    TMlibTrace(("TMLIB_TRACE : REGTRUNCATEONABORT ENTRY: txid: (%d,%d), tablename: %s\n",
       lv_transid.get_node(), lv_transid.get_seq_num(), pv_tblname), 2);

    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();
    TM_Transaction *lp_trans = gp_trans_thr->get_current();
    lv_error = lp_trans->reg_truncateonabort(pv_tblname, pv_tblname_len,
               pv_err_str, pv_err_len);

    TMlibTrace(("TMLIB_TRACE : REGTRUNCATEONABORT EXIT: txid: (%d,%d), tablename: %s, returning %d\n",
       lv_transid.get_node(), lv_transid.get_seq_num(), pv_tblname, lv_error), 2);

    return lv_error;
}


short ALTERTABLE(char *pv_tblname, int pv_tblname_len, char ** pv_tbloptions,
                 int pv_tbloptslen, int pv_tbloptscnt, long pv_transid,
                 char* &pv_err_str, int &pv_err_len)
{
    short lv_error = FEOK;
    TM_Transid lv_transid((TM_Native_Type) pv_transid);
    TMlibTrace(("TMLIB_TRACE : ALTERTABLE ENTRY: txid: (%d,%d), tablename: %s\n",
       lv_transid.get_node(), lv_transid.get_seq_num(), pv_tblname), 2);

    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();
    TM_Transaction *lp_trans = gp_trans_thr->get_current();
    lv_error = lp_trans->alter_table(pv_tblname, pv_tblname_len, pv_tbloptions,
                pv_tbloptslen, pv_tbloptscnt, pv_err_str, pv_err_len);

    TMlibTrace(("TMLIB_TRACE : ALTERTABLE EXIT: txid: (%d,%d), tablename: %s, returning %d\n",
       lv_transid.get_node(), lv_transid.get_seq_num(), pv_tblname, lv_error), 2);

    return lv_error;
}


// -------------------------------------------------------------------
// DROPTABLE
//
// Purpose: send DROPTABLE message to TM
// Params: pv_tablename, transid
// -------------------------------------------------------------------
short DROPTABLE(char *pv_tblname, int pv_tblname_len, long transid,
                char* &pv_err_str, int &pv_err_len)
{
    short lv_error = FEOK;
    TM_Transid lv_transid((TM_Native_Type) transid);
    TMlibTrace(("TMLIB_TRACE : DROPTABLE ENTRY: txid: (%d,%d), tablename: %s\n",
       lv_transid.get_node(), lv_transid.get_seq_num(), pv_tblname), 2);

    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();
    TM_Transaction *lp_trans = gp_trans_thr->get_current();
    lv_error = lp_trans->drop_table(pv_tblname, pv_tblname_len, pv_err_str,
                                    pv_err_len);

    TMlibTrace(("TMLIB_TRACE : DROPTABLE EXIT: txid: (%d,%d), tablename: %s, returning %d\n",
       lv_transid.get_node(), lv_transid.get_seq_num(), pv_tblname, lv_error), 2);

    return lv_error;
}

short HBASETM_REQUESTREGIONINFO(TM_HBASEREGIONINFO pa_trans[], short *pp_count)
{
    TMlibTrace(("TRY::TEST::: TMLIB_TRACE : REQUESTREGIONINFO entry\n"), 2);

    short           lv_error = FEOK;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    TMlibTrace(("TMLIB_TRACE : REQUESTREGIONINFO entry\n"), 2);
    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    lv_error = tmlib_check_miss_param (pp_count);
    if (lv_error)
        return lv_error;

    tmlib_init_req_hdr(TM_MSG_TYPE_REQUESTREGIONINFO, &lv_req);
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    lv_error = tmlib_check_miss_param (pp_count);
    if (lv_error)
        return lv_error;

    tmlib_init_req_hdr(TM_MSG_TYPE_REQUESTREGIONINFO, &lv_req);
    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, gv_tmlib.iv_my_nid);
    if (lv_error)
    {
        *pp_count = 0;
        TMlibTrace(("TMLIB_TRACE : HBASETM_REQUESTREGIONINFO EXIT with error %d\n", lv_error), 1);
        return lv_error;
    }

    *pp_count = lv_rsp.u.iv_hbaseregion_info.iv_count;
    for(int i=0; i < *pp_count; i++)
    memcpy((void *) &pa_trans[i], &lv_rsp.u.iv_hbaseregion_info.iv_trans[i], (sizeof(TM_HBASEREGIONINFO)));

    lv_error = lv_rsp.iv_msg_hdr.miv_err.error;

    TMlibTrace(("TMLIB_TRACE : REQUESTREGIONINFO exit\n"), 2);

    return lv_error;

}//HBASETM_REQUESTREGIONINFO

// -------------------------------------------------------------------
// ABORTTRANSACTION
//
// Purpose: send ABORTTRANSACTION message to the TM
// Params : none
//------------------------------------------------------------------- 
short ABORTTRANSACTION() 
{
    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    TM_Transaction *lp_trans = gp_trans_thr->get_current();
    if (lp_trans == NULL)
    {
        TMlibTrace(("TMLIB_TRACE : ABORTTRANSACTION returning with error %d\n",
                     FENOTRANSID), 1);
        return FENOTRANSID;
     }
    
    short lv_error =  lp_trans->abort();

     // cleanup for legacy API
     if  ((lv_error == FEINVTRANSID) ||
         (lv_error == FENOTRANSID) ||
         (lv_error == FEOK)  ||
         (lv_error == FEABORTEDTRANSID) ||
         (lv_error == FEENDEDTRANSID))
     {
         // abort removes the tx from the list and deletes the
         // enlistment object.  We simply need to delete the trans 
         gp_trans_thr->set_current(NULL);
         TM_Native_Type lv_native_type_txid = lp_trans->getTransid()->get_native_type();
         gv_tmlib.cleanupTransactionLocal(lv_native_type_txid);
         delete lp_trans; 
     }

     return lv_error;
}


//------------------------------------------------------------------------
// BEGINTRANSACTION
//
// Purpose  : send BEGINTRANSACTION message to TM
// BEGINTRANSACTION calls BEGINTX with a timeout value of 0 to pickup
// the default auto-abort timeout.
// Params   : pp_tag, pointer to a tag (out), defines the tx among
//            others for this process (and for now - all processes)
// ---------------------------------------------------------------------
short BEGINTRANSACTION(int *pp_tag) 
{
   return BEGINTX(pp_tag, 0, TM_TT_NOFLAGS);
}


//------------------------------------------------------------------------
// BEGINTX
//
// Purpose  : send BEGINTRANSACTION message to TM
// Params   : pp_tag, pointer to a tag (out), defines the tx among
//            others for this process (and for now - all processes)
//            pv_timeout contains the optional auto-abort time in seconds.
//                       0 => transaction uses default auto-abort.
//                      -1 => transaction never times out.
//            pv_type_flags contains the transaction type 
//            flags for this transaction.  Supported flags:
//                TM_TT_NOFLAGS - No flags specified.
//                TM_TT_NO_UNDO - Do not undo transaction on rollback.
//                TM_TT_FORCE_CONSISTENCY - Overrides NO_UNDO - The 
//                         audit process will not set NO_UNDO for this transaction.
// ---------------------------------------------------------------------
short BEGINTX(int *pp_tag, int pv_timeout, int64 pv_type_flags) 
{
    TM_Transaction *lp_trans = NULL;
    short lv_error = FEOK;

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    if (tmlib_check_miss_param(pp_tag) != FEOK)
    {
        TMlibTrace(("TMLIB_TRACE : BEGINTX returning with error %d\n",
                     FEMISSPARM), 1);
        return FEMISSPARM;
    }
    if (pv_timeout < -1)
    {
        TMlibTrace(("TMLIB_TRACE : BEGINTX returning with error %d\n",
                     FEBADPARMVALUE), 1);
        return FEBADPARMVALUE;
    }

    lp_trans = new TM_Transaction(pv_timeout, pv_type_flags);

    if (lp_trans == NULL)
    {
        TMlibTrace(("TMLIB_TRACE : BEGINTX returning with error %d\n",
                     FENOBUFSPACE), 1);
        return FENOBUFSPACE;
     }

    lv_error = lp_trans->get_error();
    if (!lv_error)
        *pp_tag = (int) lp_trans->getTag();

    return lv_error;
} //BEGINTX

//--------------------------------------------------------------------
//DEALLOCATE_ERR
//
//Purpose   : Called subsequent to ENDTRANSACTION_ERR
//Params    : none
//--------------------------------------------------------------------
void DEALLOCATE_ERR(char *&errStr)
{
  if(errStr)
  {
    delete errStr;
    errStr = NULL;
  }
}

//--------------------------------------------------------------------
//ENDTRANSACTION
//
//Purpose   : end the current transaction
//Params    : none
//--------------------------------------------------------------------
short ENDTRANSACTION()
{
  char *errStr = NULL;
  int   errlen = 0;
  short lv_error = ENDTRANSACTION_ERR(errStr,errlen);
  DEALLOCATE_ERR(errStr);
  return lv_error;
}
// --------------------------------------------------------------------
// ENDTRANSACTION
//
// Purpose   : end the current transaction
// Params    : none
// --------------------------------------------------------------------
short ENDTRANSACTION_ERR(char *&errStr, int &errlen) 
{
    short lv_error = FEOK;
    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    TM_Transaction *lp_trans = gp_trans_thr->get_current();

    if (lp_trans == NULL)
    {
        TMlibTrace(("TMLIB_TRACE : ENDTRANSACTION returning with error %d\n",
                     FENOTRANSID), 1);
        return FENOTRANSID;
     }

    TMlibTrace(("TMLIB_TRACE : ENDTRANSACTION ENTRY: txid: %d\n", lp_trans->getTransid()->get_seq_num()), 1);
    lv_error =  lp_trans->end(errStr, errlen);
    TMlibTrace(("TMLIB_TRACE : ENDTRANSACTION EXIT: txid: %d\n", lp_trans->getTransid()->get_seq_num()), 1);

     // cleanup for legacy API
     if  ((lv_error == FEINVTRANSID) ||
         (lv_error == FENOTRANSID) ||
         (lv_error == FEOK)  ||
         (lv_error == FEABORTEDTRANSID) ||
         (lv_error == FEENDEDTRANSID) ||
         (lv_error == FELOCKED)  ||
         (lv_error == FEHASCONFLICT))
     {
         // end removes the tx from the list and deletes the
         // enlistment object.  We simply need to delete the trans 
         gp_trans_thr->set_current(NULL);
         TM_Native_Type lv_native_type_txid = lp_trans->getTransid()->get_native_type();
         gv_tmlib.cleanupTransactionLocal(lv_native_type_txid);
         delete lp_trans;
     }

     return lv_error;
}


// --------------------------------------------------------------
// GETTRANSID
//
// Purpose  : send GETTRANSID message to TM
// Params   : pp_transid - pointer to a transid (out)
// --------------------------------------------------------------
short GETTRANSID(short* pp_transid) 
{
    TM_Native_Type *lp_transid_to_return = NULL;
    TM_Transid     *lp_transid = NULL;

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();
    
    short lv_error = tmlib_check_active_tx ();
    if (lv_error)
    {
        TMlibTrace(("TMLIB_TRACE : GETTRANSID returning with error %d\n",
                     lv_error), 2);
        return lv_error;
    }

    lv_error = tmlib_check_miss_param (pp_transid);
    if (lv_error)
    {
        TMlibTrace(("TMLIB_TRACE : GETTRANSID returning with error %d\n",
                     lv_error), 1);
        return lv_error;
    }

    lp_transid_to_return = (TM_Native_Type *)pp_transid;
    lp_transid = gp_trans_thr->get_current()->getTransid();

    if (lp_transid == NULL)
    {
       TMlibTrace(("TMLIB_TRACE : GETTRANSID failed, aborting\n"), 1);
        abort();
    }

    *lp_transid_to_return = lp_transid->get_native_type();    
    return FEOK;
    
}


// --------------------------------------------------------------
// GETTRANSINFO
//
// Purpose  : send GETTRANSID message to TM
// Params   : pp_transid - pointer to a transid (out)
// --------------------------------------------------------------
short GETTRANSINFO(short *pp_transid, int64 *pp_type_flags)
{
    TM_Native_Type *lp_transid_native = (TM_Native_Type *) pp_transid;
    TM_Transid     *lp_transid = NULL;
    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    TM_Transaction *lp_trans = gp_trans_thr->get_current();

    short lv_error = FEOK;

    TMlibTrace(("TMLIB_TRACE : GETTRANSINFO ENTRY\n"), 2);

    lv_error = tmlib_check_active_tx ();
    if (lv_error)
    {
        TMlibTrace(("TMLIB_TRACE : GETTRANSINFO returning with error %d\n",
                     lv_error), 1);
        return lv_error;
    }

    lv_error = tmlib_check_miss_param (pp_transid);
    if (lv_error == FEOK)
       lv_error = tmlib_check_miss_param(pp_type_flags);

    if (lv_error)
    {
        TMlibTrace(("TMLIB_TRACE : GETTRANSINFO returning with error %d\n",
                     lv_error), 1);
        return lv_error;
    }

    if (lp_trans == NULL)
        lv_error = FENOTRANSID;
    else
    {
       lp_transid = lp_trans->getTransid();
       if (lp_transid)
       {
           TM_Native_Type lv_native =  lp_transid->get_native_type();
           memcpy(lp_transid_native, &lv_native, sizeof(TM_Native_Type));
           *pp_type_flags = lp_trans->getTypeFlags();
       }
       else
           lv_error = FENOTRANSID;
    }
 
    TMlibTrace(("TMLIB_TRACE : GETTRANSINFO EXIT, error %d\n", lv_error), 2);
    return FEOK;
}


// -----------------------------------------------------------------
// JOINTRANSACTION
//
// Purpose - join transid specified by pv_transid
// Params  - pv_transid
// ----------------------------------------------------------------
short JOINTRANSACTION(int64 pv_transid)
{
    TMlibTrace(("TMLIB_TRACE : JOINTRANSACTION ENTRY\n"), 2);

    if (pv_transid == 0)
    {
        TMlibTrace(("TMLIB_TRACE : JOINTRANSACTION returning with error %d, empty transid supplied.\n",
                     FEINVTRANSID), 1);
        return FEINVTRANSID;
    }

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    TM_Transaction *lp_trans = NULL;
    short lv_error = FEOK;
    TM_Transid lv_transid;
    lv_transid = pv_transid;
    TM_Transaction *pp_current = gp_trans_thr->get_current();

    if ((pp_current != NULL)
        && (pp_current->getTransid()->get_native_type() == lv_transid.get_native_type()))
    {
        TMlibTrace(("TMLIB_TRACE : JOINTRANSACTION ID (%d,%d) returning with error %d\n",
                    lv_transid.get_node(), lv_transid.get_seq_num(), FEALREADYJOINED), 1);
        return FEALREADYJOINED;
    }

    lp_trans = gp_trans_thr->get_trans (lv_transid.get_native_type());

    if (lp_trans)
    {   /*12/22/2010 Removed restriction on joining a transaction we have already
           joined (implicit or explicit) in a server.
        if (!lp_trans->isEnder())
        {
            TMlibTrace(("TMLIB_TRACE : JOINTRANSACTION ID (%d,%d) returning with error %d\n",
                       lv_transid.get_node(), lv_transid.get_seq_num(), FEALREADYJOINED), 1);
            return FEALREADYJOINED;
        }
        else */
        {
            gp_trans_thr->set_current(lp_trans);  
            gp_trans_thr->set_current_suspended(false); 
        }
            TMlibTrace(("TMLIB_TRACE : JOINTRANSACTION ID (%d,%d) EXIT FEOK\n",
                        lv_transid.get_node(), lv_transid.get_seq_num()), 2);
        return FEOK;
    }
    else
    {
        lp_trans = new TM_Transaction (lv_transid, false); // implicit join and add
        if (lp_trans == NULL)
        {
            TMlibTrace(("TMLIB_TRACE : JOINTRANSACTION ID (%d,%d) returning with error %d\n",
                       lv_transid.get_node(), lv_transid.get_seq_num(), FENOBUFSPACE), 1);
            return FENOBUFSPACE;
        }

        lv_error = lp_trans->get_error();
        if (lv_error)
           delete lp_trans;
        
        TMlibTrace(("TMLIB_TRACE : JOINTRANSACTION ID (%d,%d) EXIT, error %d\n",
                    lv_transid.get_node(), lv_transid.get_seq_num(), lv_error), 2);
        return lv_error;
    }
}

// -----------------------------------------------------------------
// RESUMETRANSACTION
//
// Purpose - resume transaction specified by pv_tag.
// Note that the tag is process local and can not be used in
// RESUMETRANSACTION calls outside the beginner.
// Params  - pv_tag, either a valid tag or 0
// -----------------------------------------------------------------
short RESUMETRANSACTION(int pv_tag)
{
    unsigned int lv_tag = (unsigned int) pv_tag;
    TMlibTrace(("TMLIB_TRACE : RESUMETRANSACTION ENTRY, tag %d\n",
                lv_tag), 2);

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    if ((pv_tag == -1) || (pv_tag == 0))
    {
        gp_trans_thr->set_current(NULL);
        TMlibTrace(("TMLIB_TRACE : RESUMETRANSACTION EXIT\n"), 2);
        return FEOK;
    }

    TM_Transaction *lp_trans = gp_trans_thr->get_trans(lv_tag);
    if (lp_trans)
       gp_trans_thr->set_current(lp_trans);
    else
    {
           TMlibTrace(("TMLIB_TRACE : RESUMETRANSACTION EXIT with error %d\n", 
                        FEINVTRANSID), 2);
       return FEINVTRANSID;
    }

    TMlibTrace(("TMLIB_TRACE : RESUMETRANSACTION EXIT\n"), 2);
    return FEOK;
}

// ------------------------------------------------------------------
// STATUSTRANSACTION
//
// Purpose : send STATUSTRANSACTION message to TM
// Params  : pp_status  - out param for status, possible values are
//           ACTIVE, PREPARED, COMMITTED, ABORTING, ABORTED, HUNG
// -----------------------------------------------------------------         
short STATUSTRANSACTION(short *pp_status, int64 pv_transid) 
{
    short lv_return = 0;

    TMlibTrace(("TMLIB_TRACE : STATUSTRANSACTION ENTRY\n"), 2);

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();
    
   // we don't know about this tx, which is ok, just send it on to the DTM
   if (pv_transid != 0)
   {
        short           lv_error = FEOK;
        Tm_Req_Msg_Type lv_req;
        Tm_Rsp_Msg_Type lv_rsp;
        TM_Transid      lv_transid ((TM_Native_Type)pv_transid);

        tmlib_init_req_hdr(TM_MSG_TYPE_STATUSTRANSACTION, &lv_req);
        lv_transid.set_external_data_type(&lv_req.u.iv_status_trans.iv_transid);
        lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, lv_transid.get_node());
        if (lv_error)
        {
            TMlibTrace(("TMLIB_TRACE : STATUSTRANSACTION EXIT with error %d\n", lv_error), 1);
            return lv_error;
        }
        *pp_status = lv_rsp.u.iv_status_trans.iv_status;
        lv_error = lv_rsp.iv_msg_hdr.miv_err.error;
        TMlibTrace(("TMLIB_TRACE : STATUSTRANSACTION EXIT with error %d\n", lv_error), 2);
        return lv_error;
   }
   else if (gp_trans_thr->get_current() == NULL)
      lv_return = FENOTRANSID;
   else
      lv_return = gp_trans_thr->get_current()->status(pp_status);

   TMlibTrace(("TMLIB_TRACE : STATUSTRANSACTION EXIT with error %d\n", lv_return), 2);
   return lv_return;
}

// -----------------------------------------------------------------
// LISTTRANSACTION
//
// -----------------------------------------------------------------
short LISTTRANSACTION(TM_LIST_TRANS pa_trans[], short *pp_count, int pv_node)
{
    short           lv_error = FEOK;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    TMlibTrace(("TMLIB_TRACE : LISTTRANSACTION ENTRY\n"), 2);
    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    lv_error = tmlib_check_miss_param (pa_trans);
    if (lv_error)
        return lv_error;

    lv_error = tmlib_check_miss_param (pp_count);
    if (lv_error)
        return lv_error;

    tmlib_init_req_hdr(TM_MSG_TYPE_LISTTRANSACTION, &lv_req);
    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, pv_node);
    if (lv_error)
    {
        *pp_count = 0;
        TMlibTrace(("TMLIB_TRACE : LISTTRANSACTION EXIT with error %d\n", lv_error), 1);
        return lv_error;
    }

    *pp_count = lv_rsp.u.iv_list_trans.iv_count;
    for (int i=0; i<*pp_count; i++)
        memcpy ((void *) &pa_trans[i], (void *) &lv_rsp.u.iv_list_trans.iv_trans[i], sizeof(TM_LIST_TRANS));
    lv_error = lv_rsp.iv_msg_hdr.miv_err.error;
    TMlibTrace(("TMLIB_TRACE : LISTTRANSACTION EXIT with error %d\n", lv_error), 2);
    return lv_error;
}

// -----------------------------------------------------------------
// TMSTATS
//
// -----------------------------------------------------------------
short TMSTATS(int pv_node, TM_TMSTATS *pp_tmstats, bool pv_reset)
{
    short           lv_error = FEOK;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    lv_error = tmlib_check_miss_param (pp_tmstats);
    if (lv_error)
        return lv_error;

    tmlib_init_req_hdr(TM_MSG_TYPE_TMSTATS, &lv_req);
    lv_req.u.iv_tmstats.iv_reset = pv_reset;
    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, pv_node);
    if (!lv_error)
    {
      memcpy (pp_tmstats, &lv_rsp.u.iv_tmstats, sizeof (Tmstats_Rsp_Type));
      lv_error = lv_rsp.iv_msg_hdr.miv_err.error;
    }
    return lv_error;
}


// -----------------------------------------------------------------
// DTM_GETNEXTSEQNUMBLOCK
//  Retrieves the next block of transaction sequence 
//  numbers.
// -----------------------------------------------------------------
short DTM_GETNEXTSEQNUMBLOCK(unsigned int &pp_seqNum_start, unsigned int &pp_seqNum_count)
{
    short lv_error = FEOK;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    tmlib_init_req_hdr(TM_MSG_TYPE_GETNEXTSEQNUMBLOCK, &lv_req);
    lv_req.u.iv_GetNextSeqNum.iv_block_size =  gv_tmlib.seqNum_blockSize();

    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, gv_tmlib.iv_my_nid);
    if (!lv_error)
    {
      pp_seqNum_start = lv_rsp.u.iv_GetNextSeqNum.iv_seqNumBlock_start;
      pp_seqNum_count = lv_rsp.u.iv_GetNextSeqNum.iv_seqNumBlock_count;
      lv_error = lv_rsp.iv_msg_hdr.miv_err.error;
    }
    return lv_error;
}


// -------------------------------------------------------------------
// SUSPENDTRANSACTION
//
// Purpose - Suspend current transaction
// Params  - pp_transid, out param of transid
// ------------------------------------------------------------------
short SUSPENDTRANSACTION(short *pp_transid) 
{
    short lv_error = FEOK;
    TM_Transid lv_transid;

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    TM_Transaction *lp_trans = gp_trans_thr->get_current();
    if (lp_trans == NULL)
    {
        TMlibTrace(("TMLIB_TRACE : SUSPENDTRANSACTION returning error %d\n", FENOTRANSID), 1);
        return FENOTRANSID;
    }

    // they did not join and hence cannot suspend
    if (gp_trans_thr->get_current_propagated() == true)
    {
        TMlibTrace(("TMLIB_TRACE : SUSPENDTRANSACTION returning error %d\n", FETXSUSPENDREJECTED), 1);
        return FETXSUSPENDREJECTED; 
    }

    lv_error = lp_trans->suspend(&lv_transid);
    
    // copy the transid if there was no error
    if (!lv_error)
        memcpy (pp_transid, lv_transid.get_data_address(), sizeof (TM_Native_Type));

    // if there no error, OR if there WAS an error and we are not the ender, then
    // get rid of the context as long as we don't have outstanding I/Os.
    if (((!lv_error) || ((lv_error) && (!lp_trans->isEnder()))) && 
        ((gp_trans_thr->get_current() != NULL) && (gp_trans_thr->get_current_ios() == 0)))
    {
        gp_trans_thr->set_current(NULL);
        if(!lp_trans->isEnder()) {
          TM_Native_Type lv_native_type_txid = lp_trans->getTransid()->get_native_type();
          gv_tmlib.cleanupTransactionLocal(lv_native_type_txid);
          delete lp_trans;
        }
    }
    return lv_error;
}

short TMF_GETTXHANDLE_(short *pp_handle)
{
    TMlibTrace(("TMLIB_TRACE : TMF_GETTXHANDLE_ ENTRY\n"), 2);

    short lv_error = GETTRANSID_EXT((TM_Transid_Type *)pp_handle);

    if (lv_error == FEINVTRANSID)
    {
        TMlibTrace(("TMLIB_TRACE : TMF_GETTXHANDLE_ EXIT with error %d\n", FEINVALIDTXHANDLE), 1);
        return FEINVALIDTXHANDLE;
    }

    TMlibTrace(("TMLIB_TRACE : TMF_GETTXHANDLE_ EXIT with error %d\n", lv_error), 2);
    return lv_error;
}

//----------------------------------------------------------------------------
// TMF_SETTXHANDLE_
// Purpose : Emulates TMF API TMF_SETTXHANDLE_.
// if pp_handle == NULL (zero) or points to a zero value, then this API clears
// the current transaction.
//----------------------------------------------------------------------------
short TMF_SETTXHANDLE_(short *pp_handle)
{
    TM_Transaction *lp_trans_old = NULL;
    short lv_error = FEOK;
    TM_Transid lv_tx;
    TM_Transid_Type *lp_tx_type = NULL;

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    if (pp_handle) {
        TMlibTrace(("TMLIB_TRACE : TMF_SETTXHANDLE_ ENTRY, handle " PFLL "\n", (int64) *pp_handle), 2);
    }
    else {
        TMlibTrace(("TMLIB_TRACE : TMF_SETTXHANDLE_ ENTRY, null handle.\n"), 2);
    }


    if (pp_handle)
    {
       lv_error = tmlib_check_miss_param (pp_handle);
       if (lv_error)
       {
          TMlibTrace(("TMLIB_TRACE : TMF_SETTXHANDLE_ EXIT with error %d\n", lv_error), 1); 
          return lv_error;
       }

       lp_tx_type = (TM_Transid_Type *)pp_handle;
    }

    lp_trans_old = gp_trans_thr->get_current();
    gp_trans_thr->set_current(NULL);  // null transid
    if (pp_handle && !tmlib_zero_transid(lp_tx_type))
    {
        lv_tx = *lp_tx_type;
        if (! gv_tmlib.reinstate_tx (&lv_tx, true))
        {
            gp_trans_thr->set_current(lp_trans_old); 
            TMlibTrace(("TMLIB_TRACE : TMF_SETTXHANDLE_ EXIT with error %d\n", FEINVALIDTXHANDLE), 1); 
            lv_error = FEINVALIDTXHANDLE;
        }
    }

    TMlibTrace(("TMLIB_TRACE :TM_SETTXHANDLE_ EXIT returning %d.\n", lv_error), 2);
    return lv_error;
}

// ------------------------------------------------------------------
// 
// ------------------------------------------------------------------
short TMF_DOOMTRANSACTION_()
{

    TM_Transaction *lp_trans = NULL;
    short lv_error =  FEOK;

    TMlibTrace(("TMLIB_TRACE : TMF_DOOMTRANSACTION_ ENTRY\n"), 2);

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    lp_trans = gp_trans_thr->get_current();
    if (lp_trans == NULL)
    {
        TMlibTrace(("TMLIB_TRACE : TMF_DOOMTRANSACTION_ EXIT with error %d\n", FENOTRANSID), 1); 
        return FENOTRANSID;
    }

    lv_error =  lp_trans->abort(true);

     // this is the expected error, so return FEOK
     if (lv_error == FEABORTEDTRANSID)
     {
        TMlibTrace(("TMLIB_TRACE : TMF_DOOMTRANSACTION_ EXIT with error %d\n", FEOK), 2);
        lv_error = FEOK;
     }
     else
        TMlibTrace(("TMLIB_TRACE : TMF_DOOMTRANSACTION_ EXIT with error %d\n", lv_error), 2);

    return lv_error;
}

// -----------------------------------------------------------------
// DTM_STATUSSYSTEM
// Purpose - Return TM system information
// Returns FEOK if successful
// -----------------------------------------------------------------
short DTM_STATUSSYSTEM(TM_STATUSSYS *pp_status)
{
    short           lv_error = FEOK;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    memset(&lv_rsp, 0, sizeof(lv_rsp));

    TMlibTrace(("TMLIB_TRACE : DTM_STATUSSYS ENTRY"), 2);
    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    lv_error = tmlib_check_miss_param (pp_status);
    if (lv_error)
        return lv_error;

    tmlib_init_req_hdr(TM_MSG_TYPE_CALLSTATUSSYSTEM, &lv_req);

    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, gv_tmlib.iv_my_nid);
    if (lv_error)
    {
        TMlibTrace(("TMLIB_TRACE : DTM_STATUSSYS EXIT with error %d\n", lv_error), 1);
        return lv_error;
    }

    memcpy(pp_status, &lv_rsp,
           (sizeof(TM_STATUSSYS)));

    lv_error = lv_rsp.iv_msg_hdr.miv_err.error;
    TMlibTrace(("TMLIB_TRACE : DTM_STATUSSYS EXIT with error %d\n", lv_error), 2);
    return lv_error;
} // DTM_STATUSSYSTEM

// ------------------------------------------------------------------
// DTM_ATTACHRM
// Purpose : Intruct a TM to attach a newly restarted RM
// ------------------------------------------------------------------
short DTM_ATTACHRM(short pv_node, char *pp_rmname)
{
    short           lv_error = FEOK;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    memset(&lv_rsp, 0, sizeof(lv_rsp));

    TMlibTrace(("TMLIB_TRACE : DTM_ATTACHRM ENTRY, node %d\n", pv_node), 2);
    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    lv_error = tmlib_check_miss_param (pp_rmname);
    if (lv_error)
        return lv_error;

    tmlib_init_req_hdr(TM_MSG_TYPE_ATTACHRM, &lv_req);
    strcpy(lv_req.u.iv_attachrm.ia_rmname, pp_rmname);

    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, pv_node);
    if (lv_error)
    {
        TMlibTrace(("TMLIB_TRACE : DTM_ATTACHRM EXIT with error %d\n", lv_error), 1);
        return lv_error;
    }

    lv_error = lv_rsp.iv_msg_hdr.miv_err.error;
    TMlibTrace(("TMLIB_TRACE : DTM_ATTACHRM EXIT with error %d\n", lv_error), 2);
    return lv_error;
}

// -----------------------------------------------------------------
// DTM_STATUSTM
// Purpose - Return status information for a specific TM.
// Returns FEOK if successful
//           FENOTFOUND if the node specified was not found.
// -----------------------------------------------------------------
short DTM_STATUSTM(short pv_node, TMSTATUS *pp_tmstatus)
{
    short           lv_error = FEOK;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    memset(&lv_rsp, 0, sizeof(lv_rsp));

    TMlibTrace(("TMLIB_TRACE : DTM_STATUSTM ENTRY, node %d\n", pv_node), 2);
    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    lv_error = tmlib_check_miss_param (pp_tmstatus);
    if (lv_error)
        return lv_error;

    tmlib_init_req_hdr(TM_MSG_TYPE_STATUSTM, &lv_req);
    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, pv_node);
    if (lv_error)
    {
        TMlibTrace(("TMLIB_TRACE : DTM_STATUSTM EXIT with error %d\n", lv_error), 1);
        return lv_error;
    }

    memcpy(pp_tmstatus, &lv_rsp.u.iv_statustm.iv_status, 
           (sizeof(lv_rsp.u.iv_statustm.iv_status) + 
           (lv_rsp.u.iv_statustm.iv_status.iv_rm_count * sizeof(RM_INFO))));

    lv_error = lv_rsp.iv_msg_hdr.miv_err.error;
    TMlibTrace(("TMLIB_TRACE : DTM_STATUSTM EXIT with error %d\n", lv_error), 2);
    return lv_error;
} // DTM_STATUSTM


// -----------------------------------------------------------------
// DTM_STATUSTRANSACTION
// Purpose - Provides status information for a specified transaction
// Returns FEOK if successful
//           FENOTRANSID Trans ID not known to TM
//           FEINVTRANSID Invalid Trans ID
//           FEBADPARMVALUE One of the required parameters is invalid
// -----------------------------------------------------------------
short DTM_STATUSTRANSACTION(int64 pv_transid, TM_STATUS_TRANS *pp_trans)
{
    short           lv_error = FEOK;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    memset(&lv_rsp, 0, sizeof(lv_rsp));

    TMlibTrace(("TMLIB_TRACE : DTM_STATUSTRANSACTION ENTRY, transid " PFLL "\n", pv_transid), 2);
    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();


    if (pv_transid != 0) 
    {
        TM_Transid  lv_transid ((TM_Native_Type)pv_transid);
 
        lv_error = tmlib_check_miss_param (pp_trans);
        if (lv_error){
            TMlibTrace(("TMLIB_TRACE : DTM_STATUSTRANSMGMT EXIT with error %d\n", lv_error), 1);
            return lv_error;
        }
    
        tmlib_init_req_hdr(TM_MSG_TYPE_STATUSTRANSMGMT, &lv_req);
        //setting the response
        lv_transid.set_external_data_type(&lv_req.u.iv_status_transm.iv_transid);
        lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, lv_transid.get_node());
        if (lv_error)
        {
            TMlibTrace(("TMLIB_TRACE : DTM_STATUSTRANSMGMT EXIT with error %d\n", lv_error), 1);
            return lv_error;
        }
    
        memcpy(pp_trans, &lv_rsp.u.iv_status_transm.iv_status_trans, 
               (sizeof(lv_rsp.u.iv_status_transm.iv_status_trans)));  
    
        lv_error = lv_rsp.iv_msg_hdr.miv_err.error;
    }
    else if (gp_trans_thr->get_current() == NULL){
        lv_error = FENOTRANSID;
    }
    else {
        lv_error = FEINVTRANSID;
    }
       
    TMlibTrace(("TMLIB_TRACE : DTM_STATUSTRANSACTION EXIT with error %d\n", lv_error), 2);
    return lv_error;
} // DTM_STATUSTRANSACTION

// -----------------------------------------------------------------
// DTM_STATUSALLTRANS
// Purpose - Provides status information for all transactions
// Returns FEOK if successful
// -----------------------------------------------------------------
short DTM_STATUSALLTRANS(TM_STATUS_ALL_TRANS pa_trans[], short *pp_count, int pv_node)
{
   short           lv_error = FEOK;
   Tm_Req_Msg_Type lv_req;
   Tm_Rsp_Msg_Type lv_rsp;

   TMlibTrace(("TMLIB_TRACE : DTM_STATUSALLTRANS ENTRY\n"),2);
   if(!gv_tmlib_initialized)
      gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

   lv_error = tmlib_check_miss_param (pa_trans);
   if(lv_error)
        return lv_error;

   lv_error = tmlib_check_miss_param (pp_count);
   if(lv_error)
        return lv_error;

   tmlib_init_req_hdr(TM_MSG_TYPE_STATUSALLTRANSMGT, &lv_req);
   lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, pv_node);
   if(lv_error)
   {
      *pp_count = 0;
      TMlibTrace(("TMLIB_TRACE : DTM_STATUSALLTRANS ENTRY%d\n", lv_error),1);
      return lv_error;
   }

    *pp_count = lv_rsp.u.iv_status_alltrans.iv_count;
    for (int i=0; i<*pp_count; i++)
        memcpy((void *) &pa_trans[i], (void *) &lv_rsp.u.iv_status_alltrans.iv_trans[i], sizeof(TM_STATUS_ALL_TRANS));
    lv_error = lv_rsp.iv_msg_hdr.miv_err.error;
    TMlibTrace(("TMLIB_TRACE : DTM_STATUSALLTRANS EXIT with error %d\n", lv_error),2);
    return lv_error;
}

// -----------------------------------------------------------------
// DTM_GETTRANSINFO
// Purpose - Provides full transaction ID information
// Returns FEOK if successful
//           FENOTRANSID Trans ID not known to TM
//           FEINVTRANSID Invalid Trans ID
//           FEBADPARMVALUE One of the required parameters is invalid
// -----------------------------------------------------------------
short DTM_GETTRANSINFO(int64 pv_transid, 
                       int32 *pp_seq_num, 
                       int32 *pp_node,
                       int16 *pp_incarnation_num, 
                       int16 *pp_tx_flags,
                       TM_TT_Flags *pp_tt_flags, 
                       int16 *pp_version,
                       int16 *pp_checksum, 
                       int64 *pp_timestamp)

{
    short           lv_error = FEOK;

    TMlibTrace(("TMLIB_TRACE : DTM_GETTRANSINFO ENTRY, transid " PFLL "\n", pv_transid), 2);
    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();


    if (pv_transid != 0) 
    {
        TM_Transid  lv_transid ((TM_Native_Type)pv_transid);
        TM_Transid_Type lv_transid_type = lv_transid.get_data();

        lv_error = DTM_GETTRANSINFO_EXT(lv_transid_type, 
                                        pp_seq_num, 
                                        pp_node, 
                                        pp_incarnation_num, 
                                        pp_tx_flags, 
                                        pp_tt_flags,
                                        pp_version,
                                        pp_checksum, 
                                        pp_timestamp);
    }
    else if (gp_trans_thr->get_current() == NULL){
        lv_error = FENOTRANSID;
    }
    else {
        lv_error = FEINVTRANSID;
    }
       
    TMlibTrace(("TMLIB_TRACE : DTM_GETTRANSINFO EXIT with error %d\n", lv_error), 2);
    return lv_error;
} // DTM_GETTRANSINFO


// -----------------------------------------------------------------
// DTM_GETTRANSINFO_EXT
// Purpose - Provides full transaction ID information
// Returns FEOK if successful
//           FENOTRANSID Trans ID not known to TM
//           FEINVTRANSID Invalid Trans ID
//           FEBADPARMVALUE One of the required parameters is invalid
// -----------------------------------------------------------------
short DTM_GETTRANSINFO_EXT(TM_Transid_Type pv_transid, 
                           int32 *pp_seq_num, 
                           int32 *pp_node,
                           int16 *pp_incarnation_num, 
                           int16 *pp_tx_flags,
                           TM_TT_Flags *pp_tt_flags, 
                           int16 *pp_version,
                           int16 *pp_checksum, 
                           int64 *pp_timestamp)

{
    short           lv_error = FEOK;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;
    TM_Transid      lv_transid;

    memset(&lv_rsp, 0, sizeof(lv_rsp));

    TMlibTrace(("TMLIB_TRACE : DTM_GETTRANSINFO_EXT ENTRY\n"), 2);
    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    lv_transid = pv_transid;
    if ((lv_transid.get_node()!=0) || (lv_transid.get_seq_num()!=0))
    {
        tmlib_init_req_hdr(TM_MSG_TYPE_GETTRANSINFO, &lv_req);
        //setting the response
        lv_transid.set_external_data_type(&lv_req.u.iv_status_transm.iv_transid);
        lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, lv_transid.get_node());
        if (lv_error)
        {
            TMlibTrace(("TMLIB_TRACE : DTM_GETTRANSINFO_EXT EXIT with error %d\n", lv_error), 1);
            return lv_error;
        }
        
        if(pp_seq_num != NULL) {
            memcpy(pp_seq_num, &lv_rsp.u.iv_gettransinfo.iv_seqnum, 
                (sizeof(lv_rsp.u.iv_gettransinfo.iv_seqnum)));
        }
        if(pp_node != NULL) {
            memcpy(pp_node, &lv_rsp.u.iv_gettransinfo.iv_node, 
                (sizeof(lv_rsp.u.iv_gettransinfo.iv_node)));  
        }
        if(pp_incarnation_num != NULL) {
            memcpy(pp_incarnation_num, &lv_rsp.u.iv_gettransinfo.iv_incarnation_num, 
                (sizeof(lv_rsp.u.iv_gettransinfo.iv_incarnation_num)));  
        }
        if(pp_tx_flags != NULL) {
            memcpy(pp_tx_flags, &lv_rsp.u.iv_gettransinfo.iv_tx_flags, 
                (sizeof(lv_rsp.u.iv_gettransinfo.iv_tx_flags)));  
        }
        if(pp_tt_flags != NULL) {
            memcpy(pp_tt_flags, &lv_rsp.u.iv_gettransinfo.iv_tt_flags, 
                (sizeof(lv_rsp.u.iv_gettransinfo.iv_tt_flags)));  
        }
        if(pp_version != NULL) {
            memcpy(pp_version, &lv_rsp.u.iv_gettransinfo.iv_version, 
                (sizeof(lv_rsp.u.iv_gettransinfo.iv_version)));  
        }
        if(pp_checksum != NULL) {
            memcpy(pp_checksum, &lv_rsp.u.iv_gettransinfo.iv_checksum, 
                (sizeof(lv_rsp.u.iv_gettransinfo.iv_checksum)));  
        }
        if(pp_timestamp != NULL) {
            memcpy(pp_timestamp, &lv_rsp.u.iv_gettransinfo.iv_timestamp, 
                (sizeof(lv_rsp.u.iv_gettransinfo.iv_timestamp))); 
        }

        lv_error = lv_rsp.iv_msg_hdr.miv_err.error;

    }
    else if (gp_trans_thr->get_current() == NULL){
        lv_error = FENOTRANSID;
    }
    else {
        lv_error = FEINVTRANSID;
    }

       
    TMlibTrace(("TMLIB_TRACE : DTM_GETTRANSINFO_EXT EXIT with error %d\n", lv_error), 2);
    return lv_error;
} // DTM_GETTRANSINFO_EXT


// -----------------------------------------------------------------
// DTM_GETTRANSIDSTR(int64 pv_transid, char *pp_transidstr);
// Purpose - Obtain transid information in string format
// Returns FEOK if successful
//           
// -----------------------------------------------------------------
short DTM_GETTRANSIDSTR(int64 pv_transid, char *pp_transidstr)
{
    int32 lv_seq_num, lv_node;
    int16 lv_incarnation_num;
    short lv_error = FEOK;

    TMlibTrace(("TMLIB_TRACE : DTM_GETTRANSIDSTR ENTRY with transid: " PFLL "\n", pv_transid), 2);

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();
    
    lv_error = DTM_GETTRANSINFO(pv_transid, &lv_seq_num, &lv_node, 
                                &lv_incarnation_num, NULL, 
                                NULL, NULL, NULL,
                                NULL);
    if(!lv_error) {
        sprintf(pp_transidstr, "(%d, %d, %d)", lv_node, lv_seq_num, lv_incarnation_num);
    }

    TMlibTrace(("TMLIB_TRACE : DTM_GETTRANSIDSTR EXIT with error %d\n", lv_error), 2);
    return lv_error;
}


// -----------------------------------------------------------------
// DTM_GETTRANSIDSTR(TM_Transid_Type pv_transid, char *pp_transidstr);
// Purpose - Obtain transid information in string format uses input transid
//           format TM_Transid_Type
// Returns FEOK if successful
//           
// -----------------------------------------------------------------
short DTM_GETTRANSIDSTR_EXT(TM_Transid_Type pv_transid, char *pp_transidstr)
{
    short           lv_error = FEOK;
    TM_Transid      lv_transid;
    union {
        TM_Txid_legacy iv_legacy_txid;
        int64          iv_txid;
    } u;

    TMlibTrace(("TMLIB_TRACE : DTM_GETTRANSIDSTR_EXT ENTRY\n"), 2);

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    lv_transid = pv_transid;
    u.iv_legacy_txid.iv_seq_num = lv_transid.get_seq_num();
    u.iv_legacy_txid.iv_node = lv_transid.get_node();

    if (u.iv_txid != 0)
    {
        lv_error = DTM_GETTRANSIDSTR(u.iv_txid, pp_transidstr);
    }
    TMlibTrace(("TMLIB_TRACE : DTM_GETTRANSIDSTR_EXT EXIT with error %d\n", lv_error), 2);
    return lv_error;
}


// -----------------------------------------------------------------
// DTM_ENABLETRANSACTIONS
// Purpose - Request DTM enable transaction processing.
// Returns FEOK if successful
//           
// -----------------------------------------------------------------
short DTM_ENABLETRANSACTIONS()
{
    short           lv_error = FEOK;
    int32            lv_leadTM = -1;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    memset(&lv_rsp, 0, sizeof(lv_rsp));

    TMlibTrace(("TMLIB_TRACE : DTM_ENABLETRANSACTIONS ENTRY.\n"), 2);
    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    // First get the Lead TM node number.
    tmlib_init_req_hdr(TM_MSG_TYPE_LEADTM, &lv_req);
    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, gv_tmlib.iv_my_nid);
    if (lv_error)
    {
        TMlibTrace(("TMLIB_TRACE : DTM_ENABLETRANSACTIONS EXIT - Get LeadTM returned error %d\n", lv_error), 1);
        return lv_error;
    }

    lv_leadTM = lv_rsp.u.iv_leadtm.iv_node;

    // Now we can send the enable txns request to the Lead TM
    tmlib_init_req_hdr(TM_MSG_TYPE_ENABLETRANS, &lv_req);
    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, lv_leadTM);
    if (lv_error)
    {
        TMlibTrace(("TMLIB_TRACE : DTM_ENABLETRANSACTIONS EXIT returned error %d\n", lv_error), 1);
        return lv_error;
    }

    lv_error = lv_rsp.iv_msg_hdr.miv_err.error;
    TMlibTrace(("TMLIB_TRACE : DTM_ENABLETRANSACTIONS EXIT with error %d\n", lv_error), 2);
    return lv_error;
} // DTM_ENABLETRANSACTIONS


// -----------------------------------------------------------------
// DTM_DISABLETRANSACTIONS
// Purpose - Request DTM disable transaction processing.
// pv_shutdown_level is defined in tm.h.  This is the shutdown level
// requested.
// Returns FEOK if successful
//           FEDEVDOWN The TM that enabletrans was sent to was not or
//                   no longer is the Lead TM.
// -----------------------------------------------------------------
short DTM_DISABLETRANSACTIONS(int32 pv_shutdown_level)
{
    short           lv_error = FEOK;
    int32            lv_leadTM = -1;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    memset(&lv_rsp, 0, sizeof(lv_rsp));

    TMlibTrace(("TMLIB_TRACE : DTM_DISABLETRANSACTIONS ENTRY shutdown level %d\n", pv_shutdown_level), 2);
    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    if (pv_shutdown_level != TM_DISABLE_NORMAL &&
        pv_shutdown_level != TM_DISABLE_SHUTDOWN_NORMAL &&
        pv_shutdown_level != TM_DISABLE_SHUTDOWN_IMMEDIATE)
    {
        TMlibTrace(("TMLIB_TRACE : DTM_DISABLETRANSACTIONS EXIT - Bad shutdown level specified %d\n", pv_shutdown_level), 1);
        return FEBADPARMVALUE;
    }
       

    // First get the Lead TM node number.
    tmlib_init_req_hdr(TM_MSG_TYPE_LEADTM, &lv_req);
    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, gv_tmlib.iv_my_nid);
    if (lv_error)
    {
        TMlibTrace(("TMLIB_TRACE : DTM_DISABLETRANSACTIONS EXIT - Get LeadTM returned error %d\n", lv_error), 1);
        return lv_error;
    }

    lv_leadTM = lv_rsp.u.iv_leadtm.iv_node;

    // Now we can send the disable txns request to the Lead TM
    tmlib_init_req_hdr(TM_MSG_TYPE_DISABLETRANS, &lv_req);
    lv_req.u.iv_disabletrans.iv_shutdown_level = pv_shutdown_level;

    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, lv_leadTM);
    if (lv_error)
    {
        TMlibTrace(("TMLIB_TRACE : DTM_DISABLETRANSACTIONS EXIT returned error %d\n", lv_error), 1);
        return lv_error;
    }

    lv_error = lv_rsp.iv_msg_hdr.miv_err.error;
    TMlibTrace(("TMLIB_TRACE : DTM_DISABLETRANSACTIONS EXIT with reply error %d\n", lv_error), 2);
    return lv_error;
} // DTM_DISABLETRANSACTIONS


// -----------------------------------------------------------------
// DTM_DRAINTRANSACTIONS
// Purpose - Request a TM specified by pv_node to disable new 
// transactions and allow active transactions to complete or abort 
// if pv_immediate is set.  pv_immediate defaults to false.
// Returns FEOK if successful
//         FEDEVDOWN The TM specified by pv_node is not up.
// -----------------------------------------------------------------
short DTM_DRAINTRANSACTIONS(int32 pv_node, bool pv_immediate=false)
{
    short           lv_error = FEOK;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    memset(&lv_rsp, 0, sizeof(lv_rsp));

    TMlibTrace(("TMLIB_TRACE : DTM_DRAINTRANSACTIONS ENTRY node %d, immediate=%d.\n", pv_node, pv_immediate), 2);
    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    tmlib_init_req_hdr(TM_MSG_TYPE_DRAINTRANS, &lv_req);
    lv_req.u.iv_draintrans.iv_immediate = pv_immediate;

    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, pv_node);
    if (lv_error)
    {
        TMlibTrace(("TMLIB_TRACE : DTM_DRAINTRANSACTIONS EXIT returned error %d\n", lv_error), 1);
        return lv_error;
    }

    lv_error = lv_rsp.iv_msg_hdr.miv_err.error;
    TMlibTrace(("TMLIB_TRACE : DTM_DRAINTRANSACTIONS EXIT with reply error %d\n", lv_error), 2);
    return lv_error;
} // DTM_DRAINTRANSACTIONS


// -----------------------------------------------------------------
// DTM_QUIESCE
// Purpose - Send a Quiesce request to the TM specified by pv_node.
// This is for internal testing of NodeQuiesce only!!!!
// Returns FEOK if successful
//         FEDEVDOWN The TM specified by pv_node is not up.
// -----------------------------------------------------------------
short DTM_QUIESCE(int32 pv_node)
{
    short           lv_error = FEOK;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    memset(&lv_rsp, 0, sizeof(lv_rsp));

    TMlibTrace(("TMLIB_TRACE : DTM_QUIESCE ENTRY node %d.\n", pv_node), 2);
    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    tmlib_init_req_hdr(TM_MSG_TYPE_QUIESCE, &lv_req);
    lv_req.u.iv_quiesce.iv_stop = false;

    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, pv_node);
    if (lv_error)
    {
        TMlibTrace(("TMLIB_TRACE : DTM_QUIESCE EXIT returned error %d\n", lv_error), 1);
        return lv_error;
    }

    lv_error = lv_rsp.iv_msg_hdr.miv_err.error;
    TMlibTrace(("TMLIB_TRACE : DTM_QUIESCE EXIT with reply error %d\n", lv_error), 2);
    return lv_error;
} // DTM_QUIESCE


// -----------------------------------------------------------------
// DTM_UNQUIESCE
// Purpose - Send a Un-Quiesce request to the TM specified by pv_node.
// This is for internal testing of NodeQuiesce only!!!!
// Returns FEOK if successful
//         FEDEVDOWN The TM specified by pv_node is not up.
// -----------------------------------------------------------------
short DTM_UNQUIESCE(int32 pv_node)
{
    short           lv_error = FEOK;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    memset(&lv_rsp, 0, sizeof(lv_rsp));

    TMlibTrace(("TMLIB_TRACE : DTM_QUIESCE ENTRY node %d.\n", pv_node), 2);
    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    tmlib_init_req_hdr(TM_MSG_TYPE_QUIESCE, &lv_req);
    lv_req.u.iv_quiesce.iv_stop = true;

    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, pv_node);
    if (lv_error)
    {
        TMlibTrace(("TMLIB_TRACE : DTM_QUIESCE EXIT returned error %d\n", lv_error), 1);
        return lv_error;
    }

    lv_error = lv_rsp.iv_msg_hdr.miv_err.error;
    TMlibTrace(("TMLIB_TRACE : DTM_QUIESCE EXIT with reply error %d\n", lv_error), 2);
    return lv_error;
} // DTM_QUIESCE


// ------------------------------------------------------------------
// EXTENDED API - same as their counterparts
// ------------------------------------------------------------------
short GETTRANSID_EXT (TM_Transid_Type *pp_transid)
{
    TMlibTrace(("TMLIB_TRACE : GETTRANSID_EXT ENTRY\n"), 2);

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    if (gp_trans_thr->get_current() == NULL)
    {
        TMlibTrace(("TMLIB_TRACE : GETTRANSID_EXT EXIT with error %d\n", FENOTRANSID), 2);
        return FENOTRANSID;
    }

    TM_Transid *lp_transid = gp_trans_thr->get_current()->getTransid();
    if (lp_transid)
        memcpy (pp_transid, lp_transid->get_data_address(), sizeof (TM_Transid_Type));
    else
    {
        TMlibTrace(("TMLIB_TRACE : GETTRANSID_EXT EXIT with error %d\n", FENOTRANSID), 2);
        return FENOTRANSID;
    }
 
    TMlibTrace(("TMLIB_TRACE : GETTRANSID_EXT EXIT\n"), 2);
    return FEOK;
}


short GETTRANSINFO_EXT (TM_Transid_Type *pp_transid, int64 *pp_type_flags)
{
    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    TM_Transaction *lp_trans = gp_trans_thr->get_current();
    TM_Transid *lp_transid = NULL;
    short lv_error = FEOK;

    TMlibTrace(("TMLIB_TRACE : GETTRANSINFO_EXT ENTRY\n"), 2);

    lv_error = tmlib_check_active_tx ();
    if (lv_error)
    {
        TMlibTrace(("TMLIB_TRACE : GETTRANSINFO_EXT returning with error %d\n",
                     lv_error), 1);
        return lv_error;
    }

    lv_error = tmlib_check_miss_param(pp_transid);
    if (lv_error == FEOK)
       lv_error = tmlib_check_miss_param(pp_type_flags);

    if (lv_error != FEOK)
    {
        TMlibTrace(("TMLIB_TRACE : GETTRANSINFO_EXT returning with error %d\n",
                     lv_error), 1);
        return lv_error;
    }

    if (lp_trans == NULL)
        lv_error = FENOTRANSID;
    else
    {
       lp_transid = lp_trans->getTransid();
       if (lp_transid)
       {
           memcpy (pp_transid, lp_transid->get_data_address(), sizeof (TM_Transid_Type));
           *pp_type_flags = lp_trans->getTypeFlags();
       }
       else
           lv_error = FENOTRANSID;
    }
 
    TMlibTrace(("TMLIB_TRACE : GETTRANSINFO_EXT EXIT, error %d\n", lv_error), 2);
    return FEOK;
} //GETTRANSINFO_EXT


short SUSPENDTRANSACTION_EXT (TM_Transid_Type *pp_transid)
{
    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    TM_Transaction *lp_trans = gp_trans_thr->get_current();
    TM_Transid lv_transid;
    short lv_error = FEOK;

    TMlibTrace(("TMLIB_TRACE : SUSPENDTRANSACTION_EXT ENTRY\n"), 2);

    if (lp_trans == NULL)
    {
        TMlibTrace(("TMLIB_TRACE : SUSPENDTRANSACTION_EXT EXIT with error %d\n", FENOTRANSID), 1);
        return FENOTRANSID;
    }

    // they did not join and hence cannot suspend
    if (gp_trans_thr->get_current_propagated() == true)
    {
        TMlibTrace(("TMLIB_TRACE : SUSPENDTRANSACTION_EXT EXIT with error %d\n", 
                     FETXSUSPENDREJECTED), 1);
        return FETXSUSPENDREJECTED; 
    }

    lv_error = lp_trans->suspend(&lv_transid);
    if (!lv_error)
    {
        memcpy (pp_transid, lv_transid.get_data_address(), sizeof (TM_Transid_Type));
        gp_trans_thr->set_current(NULL);
        if(!lp_trans->isEnder())
           delete lp_trans;
    } 

    TMlibTrace(("TMLIB_TRACE : SUSPENDTRANSACTION_EXT EXIT with error %d\n", lv_error), 2);
    return lv_error;
}

short JOINTRANSACTION_EXT (TM_Transid_Type *pp_transid)
{
    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    TM_Transaction *lp_trans = NULL;
    short lv_error = FEOK;
    TM_Transid lv_transid;

    if (pp_transid == NULL)
        return FEINVTRANSID;

    TMlibTrace(("TMLIB_TRACE : JOINTRANSACTION_EXT ENTRY\n"), 2);
    lv_transid = *pp_transid;

    if ((gp_trans_thr->get_current() != NULL)
        && (gp_trans_thr->get_current()->equal(lv_transid)))
    {
        TMlibTrace(("TMLIB_TRACE : JOINTRANSACTION_EXT EXIT with error %d\n", FEALREADYJOINED), 1);
        return FEALREADYJOINED;
    }

    lp_trans = gp_trans_thr->get_trans (lv_transid.get_native_type());

    if (lp_trans)
    { 
        if (!lp_trans->isEnder())
        {
            TMlibTrace(("TMLIB_TRACE : JOINTRANSACTION_EXT EXIT with error %d\n", FEALREADYJOINED), 1);
            return FEALREADYJOINED;
        }
        else
        {
            gp_trans_thr->set_current(lp_trans);  // beginner, no need to increment anything
            gp_trans_thr->set_current_suspended(false);
        } 
        TMlibTrace(("TMLIB_TRACE : JOINTRANSACTION_EXT EXIT with error %d\n", FEOK), 2);
        return FEOK;
    }
    else
    {
       lp_trans = new TM_Transaction (lv_transid, false); // implicit join and add
       if (lp_trans == NULL)
       {
           TMlibTrace(("TMLIB_TRACE : JOINTRANSACTION_EXT EXIT with error %d\n", FENOBUFSPACE), 1);
           return FENOBUFSPACE;
       }

       lv_error = lp_trans->get_error();
       if (lv_error)
           delete lp_trans;
       TMlibTrace(("TMLIB_TRACE : JOINTRANSACTION_EXT EXIT with error %d\n", lv_error), 2);
       return lv_error;
    }
}

// ---------------------------------------------------------------
// TEST_TX_COUNT
//
// Purpose - internal testing
// ---------------------------------------------------------------
short TEST_TX_COUNT()
{
    short           lv_error;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    TMlibTrace(("TMLIB_TRACE : TEST_TC_COUNT ENTRY \n"), 2);

    if (!gv_tmlib_initialized)
    {
        gv_tmlib.initialize();
    }

    // instantiate a gp_trans_thr object for this thread if needed.
    if (gp_trans_thr == NULL)
       gp_trans_thr = new TMLIB_ThreadTxn_Object();

    tmlib_init_req_hdr(TM_MSG_TYPE_TEST_TX_COUNT, &lv_req);
    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, gv_tmlib.iv_my_nid);
    if (lv_error)
        return lv_error;

    int lv_count = lv_rsp.u.iv_count.iv_count;
 
    TMlibTrace(("TMLIB_TRACE : TEST_TX_COUNT is %d, for node %d EXIT\n", 
                      lv_count, gv_tmlib.iv_my_nid), 2); 
    return (short) lv_count;

}

//------------------------------------------------------------------------
// TMWAIT
//
// Purpose  : Wait for the TM to start and be ready to process begintransactions
// Params   : none.
// ---------------------------------------------------------------------
int16 TMWAIT()
{
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;
    int16 lv_error = FEOK;

    TMlibTrace(("TMLIB_TRACE : TMWAIT ENTRY\n"), 2);

    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();
    
    if (!gv_tmlib.open_tm(gv_tmlib.iv_my_nid))
    {
         TMlibTrace(("TMLIB_TRACE : TMWAIT returning FETMFNOTRUNNING\n"), 1);
         return FETMFNOTRUNNING;
    }

    tmlib_init_req_hdr(TM_MSG_TYPE_WAIT_TMUP, &lv_req);
    lv_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, gv_tmlib.iv_my_nid);

    TMlibTrace(("TMLIB_TRACE : TMWAIT EXIT, feerror=%d, error=%d\n",
                lv_error, lv_rsp.iv_msg_hdr.miv_err.error), 2);

    if (lv_error != FEOK)
        return lv_error;
    else
        return lv_rsp.iv_msg_hdr.miv_err.error;
}

//------------------------------------------------------------------------
// TMCLIENTEXIT
//
// Purpose  : To close all the TM opens from the clients before exiting 
// Params   : none.
// ---------------------------------------------------------------------
int16 TMCLIENTEXIT()
{
   int16 lv_error = FEOK;
   lv_error = gv_tmlib.close_tm();
   return lv_error;
}


// -------------------------------------------------------------------
// TMLIB methods
// -------------------------------------------------------------------

// ------------------------------------------------------------------
// TMLIB
// Purpose - Register callback with seabed!
// Also need to get any configuration values.
// ------------------------------------------------------------------
TMLIB::TMLIB() : JavaObjectInterfaceTM()
{
    tm_rtsigblock_proc();  
    iv_initialized = false;
    initMutex_ = new TM_Mutex(true, false);
//    msg_mon_trans_register_tmlib (tmlib_callback);
    msg_mon_trans_register_tmlib2 (tmlib_callback2);

    for (int lv_idx = 0; lv_idx < MAX_NODES; lv_idx++)
    {
        memset( &ia_tm_phandle[lv_idx].iv_phandle, 0, sizeof(SB_Phandle_Type));
        ia_tm_phandle[lv_idx].iv_open = 0;
        ia_tm_phandle[lv_idx].iv_pid = 0;
        ia_is_enlisted[lv_idx] = false;
    } 

    iv_next_nid = 0;
    iv_next_tag = 1;
    iv_my_nid = iv_my_pid = iv_node_count = iv_tm_pid = 0;
    iv_txn_distribute = DIST_NOT_SET;

    localBegin(false);
    ms_getenv_bool("DTM_LOCAL_TRANSACTIONS", &iv_localBegin);
    //if (localBegin())
    //   printf("!! Using local transactions. !!\n");

    seqNum_blockSize(1000);
    ms_getenv_int("DTM_LOCAL_BLOCKSIZE", &iv_seqNum_blockSize);

    enableCleanupRMInterface(true);
    ms_getenv_bool("TMLIB_ENABLE_CLEANUP", &enableCleanupRMInterface_);

    ip_seqNum = new CtmSeqNum();
} //TMLIB::TMLIB


// -----------------------------------------------------------------
// add_or_update
// Purpose - get a transaction into our system.
// The new transaction is the current transaction after the call.
// If the TM Library already has this transaction in it's list of 
// active transactions, then we increase the depth after making it
// current.  This can happen when a server receives multiple
// awaitiox completions for the same transaction without replying
// to the first awaitiox (receive depth > 1 on NSK).
// ----------------------------------------------------------------- 
short TMLIB::add_or_update (TM_Transid pv_transid, bool pv_can_end, 
                            int pv_tag)
{
     int lv_new_tx = false;

     TMlibTrace(("TMLIB_TRACE : TMLIB::add_or_update ENTRY\n"), 2);
     // if the tx doesn't exist yet here, create a new one
     TM_Transaction *lp_trans = gp_trans_thr->get_trans(pv_transid.get_native_type());

    pv_can_end = pv_can_end; //Intel compiler warning 869
    pv_tag = pv_tag; //Intel compiler warning 869

     if (!lp_trans)
     {
         TMlibTrace(("TMLIB_TRACE : TMLIB::add_or_update - adding new transaction " PFLL "\n",
                      pv_transid.get_native_type()), 3);

         lv_new_tx = true;
         lp_trans = new TM_Transaction(pv_transid, true /*fs server*/);
         if (lp_trans == NULL)
         {
             TMlibTrace(("TMLIB_TRACE : TMLIB::add_or_update EXIT with error %d\n", FENOBUFSPACE), 1);
             return FENOBUFSPACE;
         }

         short lv_error = lp_trans->get_error();
         if (lv_error)
         {
             TMlibTrace(("TMLIB_TRACE : TMLIB::add_or_update - new transaction failed with error %d\n",
                          lp_trans->get_error()), 1);
             delete lp_trans;
             lp_trans = NULL;
             TMlibTrace(("TMLIB_TRACE : TMLIB::add_or_update EXIT with error %d\n", lv_error), 2);
             return lv_error;
         }
     }
     else
     {
        TMlibTrace(("TMLIB_TRACE : TMLIB::add_or_update - found existing transaction " PFLL "\n",
                     pv_transid.get_native_type()), 3);
     }

     if (lp_trans)
     {
         gp_trans_thr->set_current(lp_trans);
         if (lv_new_tx)
             gp_trans_thr->set_current_propagated(true);   
         else
             gp_trans_thr->increase_current_depth();
     }
     TMlibTrace(("TMLIB_TRACE : TMLIB::add_or_update EXIT\n"), 2);
     return FEOK;
}

// -----------------------------------------------------------------
// add_or_update
// Purpose - get a transaction and startid into our system.
// The new transaction is the current transaction after the call.
// If the TM Library already has this transaction in it's list of
// active transactions, then we increase the depth after making it
// current.  This can happen when a server receives multiple
// awaitiox completions for the same transaction without replying
// to the first awaitiox (receive depth > 1 on NSK).
// -----------------------------------------------------------------
short TMLIB::add_or_update (TM_Transid pv_transid, TM_Transseq_Type pv_startid,
                            bool pv_can_end, int pv_tag)
{
     int lv_new_tx = false;

     TMlibTrace(("TMLIB_TRACE : TMLIB::add_or_update ENTRY\n"), 2);
     // if the tx doesn't exist yet here, create a new one
     TM_Transaction *lp_trans = gp_trans_thr->get_trans(pv_transid.get_native_type());

    pv_can_end = pv_can_end; //Intel compiler warning 869
    pv_tag = pv_tag; //Intel compiler warning 869

     if (!lp_trans)
     {
         TMlibTrace(("TMLIB_TRACE : TMLIB::add_or_update - adding new transaction " PFLL "\n",
                      pv_transid.get_native_type()), 3);

         lv_new_tx = true;
         lp_trans = new TM_Transaction(pv_transid, true /*fs server*/);
         if (lp_trans == NULL)
         {
             TMlibTrace(("TMLIB_TRACE : TMLIB::add_or_update EXIT with error %d\n", FENOBUFSPACE), 1);
             return FENOBUFSPACE;
         }

         short lv_error = lp_trans->get_error();
         if (lv_error)
         {
             TMlibTrace(("TMLIB_TRACE : TMLIB::add_or_update - new transaction failed with error %d\n",
                          lp_trans->get_error()), 1);
             delete lp_trans;
             lp_trans = NULL;
             TMlibTrace(("TMLIB_TRACE : TMLIB::add_or_update EXIT with error %d\n", lv_error), 2);
             return lv_error;
         }
     }
     else
     {
        TMlibTrace(("TMLIB_TRACE : TMLIB::add_or_update - found existing transaction " PFLL "\n",
                     pv_transid.get_native_type()), 3);
     }

     if (lp_trans)
     {
         gp_trans_thr->set_current(lp_trans);
         if (lv_new_tx){
             gp_trans_thr->set_startid(pv_startid);
             TMlibTrace(("TMLIB_TRACE : TMLIB::add_or_update - setting startid %ld for transaction " PFLL "\n",
                     gp_trans_thr->get_startid(), pv_transid.get_native_type()), 3);
             gp_trans_thr->set_current_propagated(true);
         }
         else
             gp_trans_thr->increase_current_depth();
     }
     TMlibTrace(("TMLIB_TRACE : TMLIB::add_or_update EXIT\n"), 2);
     return FEOK;
}

// ------------------------------------------------------------------------
// clear_entry
// Purpose :  clear an entry out of our system and suspend if instructed to
// ------------------------------------------------------------------------
bool TMLIB::clear_entry (TM_Transid pv_transid, bool pv_server, 
                          /*bool pv_suspend,*/ bool pv_force)
{
    bool lv_done = false;
    int  lv_depth = 0;
    int  lv_node = pv_transid.get_node();

    TMlibTrace(("TMLIB_TRACE : TMLIB::clear_entry ENTRY\n"), 2);

    // either decrease depth or remove, pv_force is only set on tx propagation
     if (!pv_force)
     {
          lv_depth = gp_trans_thr->decrease_current_depth();
          TMlibTrace(("TMLIB_TRACE : TMLIB::clear_entry - decreased depth to %d\n", lv_depth), 3);
          // server side
          if (lv_depth <= 0)
          {
            TMlibTrace(("TMLIB_TRACE : TMLIB::clear_entry - delisting from node %d\n", lv_node), 3);

            // we ARE a server and our count is now zero.  Get rid of it
            if (pv_server)
                 gp_trans_thr->delete_current(); // only do for server

             lv_done = true;
           }
     }
     else { 
         TM_Transaction *lp_trans = gp_trans_thr->get_trans (pv_transid.get_native_type());
         // we don't ever want to delist the owner
         if ((lp_trans) && (!lp_trans->isEnder()))
         {
             TMlibTrace(("TMLIB_TRACE : TMLIB::clear_entry - delisting from node %d\n", lv_node), 3);
             lv_done = true;

         } else
         {
             gp_trans_thr->set_startid(-1);
             gp_trans_thr->set_current(NULL);
         }
    }

    if ((lv_done) && (pv_force)){
        gp_trans_thr->set_startid(-1);
        gp_trans_thr->set_current(NULL);
    }
    TMlibTrace(("TMLIB_TRACE : TMLIB::add_or_update EXIT\n"), 2);
    return lv_done;

}


// -----------------------------------------------------------------
// reinstate_tx
// Purpose : get a transaction and return results
// -----------------------------------------------------------------
bool TMLIB::reinstate_tx(TM_Transid *pv_transid, bool pv_settx)
{
     TMlibTrace(("TMLIB_TRACE : TMLIB::reinstate_tx\n"), 2);
     TM_Transaction *lp_trans = gp_trans_thr->get_trans (pv_transid->get_native_type());

     if (lp_trans)
     {
         gp_trans_thr->set_current(lp_trans);
         // special case for beginner
         if (pv_settx)
         {
             TMLIB_EnlistedTxn_Object* lp_enlisted = gp_trans_thr->get_enlisted(pv_transid->get_native_type());
             if ((lp_enlisted) && (lp_enlisted->suspended_tx() == true))
                 return false;
         }
        
          return true;
     }
     return false;
}

// --------------------------------------------------------------------
// table get methods
// --------------------------------------------------------------------

bool TMLIB::phandle_get(TPT_PTR(pp_phandle), int pv_node)
{
    bool lv_open = false;
    lv_open = ia_tm_phandle[pv_node].iv_open;
    if (lv_open)
        *pp_phandle = ia_tm_phandle[pv_node].iv_phandle;
    return lv_open;
}

// -----------------------------------------------------------------------
// table set methods
// ----------------------------------------------------------------------

void TMLIB::phandle_set (TPT_PTR(pp_phandle), int pv_node)
{

    ia_tm_phandle[pv_node].iv_phandle = *pp_phandle;
    ia_tm_phandle[pv_node].iv_open = true;
    
    //call decompose to get out the nid/pid
    XPROCESSHANDLE_DECOMPOSE_(pp_phandle, 
                                          NULL, // node - already know it
                                          &ia_tm_phandle[pv_node].iv_pid, // pid
                                          NULL, // don't care
                                          NULL, // don't care
                                          0, // don't care
                                          NULL, // don't care
                                          NULL, // don't care
                                          0, // don't care
                                          NULL, // don't care
                                          NULL); //sdon't care

    TMlibTrace(("TMLIB_TRACE : phandle_set, received tm pid of %d for node %d\n", 
        ia_tm_phandle[pv_node].iv_pid, pv_node), 3);
}

void TMLIB::initialize()
{
   initMutex_->lock();
   if (iv_initialized) {
      initMutex_->unlock();
      return;
   }
   msg_mon_get_process_info(NULL, &iv_my_nid,
                                    &iv_my_pid);

    open_tm(iv_my_nid, true);

    TMlibTrace(("TMLIB_TRACE : TMLIB::initialize : my nid,pid (%d, %d)\n",
               iv_my_nid, iv_my_pid ), 1);

    //TODO: switch the following call to msg_mon_get_node_info2 when available.
    // This call has been changed so that the node count includes spare nodes, so 
    // will give the wrong value for iv_node_count.
    msg_mon_get_node_info(&iv_node_count, MAX_NODES, NULL);
    iv_initialized = true;
    // We don't use gv_tmlib_initialized but set it here just to keep things aligned.
    gv_tmlib_initialized = true;
    initMutex_->unlock();
}

// -------------------------------------------------------------------
// TMLIB::initJNI
// Initialize JNI interface 
// Only used on demand - if you do this in TMLIB::initialize
// it gets called when it may not be used and conflicts with udrserv.
// -------------------------------------------------------------------
int TMLIB::initJNI()
{
    int lv_err = 0;

    if ((lv_err = initJNIEnv()) != 0)
       return lv_err;
    if (isInitialized())
       return 0;
    _tlp_jenv->PopLocalFrame(NULL);
    if (javaMethodsInitialized_)
       return JavaObjectInterfaceTM::init((char *)hbasetxclient_classname, hbasetxclient_class, TMLibJavaMethods_, JM_LAST_HBASETXCLIENT, javaMethodsInitialized_);
    else
    {
       initMutex_->lock();
       if (javaMethodsInitialized_) {
          initMutex_->unlock();
          return JavaObjectInterfaceTM::init((char *)hbasetxclient_classname, hbasetxclient_class, TMLibJavaMethods_, JM_LAST_HBASETXCLIENT, javaMethodsInitialized_);
       }
       short lv_result = setupJNI();
       if (lv_result) {
          fprintf(stderr, "setupJNI returned error %d in TMLIB::initJNI. Exiting.\n", lv_result);
          fflush(stderr);
          abort();
       }
       initMutex_->unlock();
    }
    if (localBegin()) {
        lv_err = initConnection(iv_my_nid);
        if (lv_err)
        {
            TMlibTrace(("TMLIB_TRACE : TMLIB::initJNI: initConnection failed with error %d.\n", lv_err), 1);
            printf("TMLIB::initConnection failed with error %d.\n", lv_err);
            //tm_log_event(DTM_HBASE_INIT_FAILED, SQ_LOG_CRIT, "DTM_HBASE_INIT_FAILED", lv_error);
            abort();
        }
        else
            TMlibTrace(("TMLIB_TRACE : TMLIB::initJNI: initConnection succeeded.\n"), 1);
    }
    return 0;
} //initJNI


// -------------------------------------------------------------------
// open_tm
// Purpose : open a TM on the given node
// -------------------------------------------------------------------
bool TMLIB::open_tm(int pv_node, bool pv_startup) 
{
    char            lv_buffer[8];
    int             lv_error = 0;
    TM_Transid_Type lv_null_transid;  
    int             lv_oid;
    TPT_DECL       (lv_phandle);
    int             lv_retry = 0;


    // this mutex is for starting up and threads racing to 
    // open the tm.  This will stay locked and hang all transactional 
    // threads up until the tm is open. 
       // strategy, we may need to redo this part a bit
    if (!iv_initialized && !pv_startup)
        initialize();
    
    // get the phandle
    if (phandle_get(&lv_phandle, pv_node) == true)
     {
        TMlibTrace(("TMLIB_TRACE : open_tm, TM for node %d already open\n", 
                          pv_node), 3);
        return true;
     }

    TMlibTrace(("TMLIB_TRACE : open_tm (node %d) ENTRY\n", pv_node), 2);

    sprintf (lv_buffer, "$tm%d", pv_node);

    while (lv_retry < 10)
    {
        lv_error = msg_mon_open_process(lv_buffer,
                                        &lv_phandle,       
                                        &lv_oid);

        if (!lv_error)
        {
            // set phandle under mutex
            phandle_set(&lv_phandle, pv_node);
 
            lv_null_transid.id[0] = lv_null_transid.id[1] = 
            lv_null_transid.id[2] = lv_null_transid.id[3] = 0;
            msg_mon_trans_enlist (pv_node, ia_tm_phandle[pv_node].iv_pid,
                               lv_null_transid);
            
            TMlibTrace(("TMLIB_TRACE : open_tm EXIT, successfully opened TM (%d,%d)\n",
                             pv_node, ia_tm_phandle[pv_node].iv_pid), 2);
            return true;
        }
        else
        {
            TMlibTrace(("TMLIB_TRACE : open_tm failed to open tm %s, attempt %d, error %d.\n",
                        lv_buffer, lv_retry, lv_error), 2);
        }
        lv_retry++;
    }
  
    TMlibTrace(("TMLIB_TRACE : open_tm EXIT, failed to open TM for node %d, no more attempts\n",
                             pv_node), 1);
    // could not open tm
    return false;
}

// ---------------------------------------------------------
// send_tm
// Purpose - send message to the given TM
// ---------------------------------------------------------
short TMLIB::send_tm(Tm_Req_Msg_Type *pp_req, Tm_Rsp_Msg_Type *pp_rsp, 
                    int pv_node) 
{
    ushort    lv_req_len = sizeof (Tm_Req_Msg_Type);
    int       lv_rsp_len = sizeof (Tm_Rsp_Msg_Type);
    int       lv_msgid;
    TPT_DECL( lv_phandle);
    short     la_results[6];
    short     lv_ret = FEOK;
    int32     lv_linkRetries = 0;
    int32     lv_breakRetries = 0;
    const int32 lc_maxLinkRetries = 100;
    const int32 lc_linkPause = 3000; // 3 second
    const int32 lc_maxBreakRetries = 60;
    const int32 lc_breakPause = 3000; // 3 second

    TMlibTrace(("TMLIB_TRACE : send_tm (node %d) ENTRY\n", pv_node), 2);

 retry_on_fepathdown:
    if (!gv_tmlib.open_tm(pv_node))
    {
         TMlibTrace(("TMLIB_TRACE : returning FETMFNOTRUNNING\n"), 1);
         return FETMFNOTRUNNING;
    }

    // get phandle and (FOR NOW) abort.  if we get into this
    // method, we should have already opened and stored the tm
    gv_tmlib.phandle_get(&lv_phandle, pv_node);

    do {
       lv_ret = BMSG_LINK_(&lv_phandle,                  // phandle
                        &lv_msgid,                   // msgid
                        NULL,                        // reqctrl
                        0,                           // reqctrlsize
                        NULL,                        // replyctrl
                        0,                           // replyctrlmax
                        (char *) pp_req,             // reqdata
                        lv_req_len,                  // reqdatasize
                        (char *) pp_rsp,             // replydata
                        lv_rsp_len,                  // replydatamax
                        0,                           // linkertag
                        TMLIB_LINK_PRIORITY,         // pri
                        0,                           // xmitclass
                        0);                          // linkopts
       lv_linkRetries++;
       if ((lv_ret == FENOLCB) && (lv_linkRetries <= lc_maxLinkRetries) &&
         (lv_linkRetries > 1))
          SB_Thread::Sthr::sleep(lc_linkPause); // in msec
    } while (lv_ret == FENOLCB && ++lv_linkRetries <= lc_maxLinkRetries);


    if (lv_ret)
    {
       TMlibTrace(("TMLIB_TRACE : send_tm , BMSG_LINK error is %d\n", lv_ret), 3);
    }
    else
    {
       lv_ret = BMSG_BREAK_(lv_msgid, la_results, &lv_phandle);
       if (lv_ret)
       {
          TMlibTrace(("TMLIB_TRACE : send_tm , BMSG_BREAK error is %d\n", lv_ret), 3);
       }
    }

    switch (lv_ret)
    {
    case FEPATHDOWN:
       lv_breakRetries++;
       if (lv_breakRetries <= lc_maxBreakRetries) {
         SB_Thread::Sthr::sleep(lc_breakPause); // in msec
         TMlibTrace(("TMLIB_TRACE : send_tm , retry after BMSG_BREAK error: FEPATHDOWN\n"), 3);
         goto retry_on_fepathdown;
       }
       lv_ret = FETMFNOTRUNNING;
       break;
    case FESERVICEDISABLED:
        // This is returned during fail-safe by the TM to indicate that the node
        // is about to go down.  No further transactional requests will be processed
        // by this node and any DTM clients should migrate to another node.
        // In M5 HA SPR we will simply hang here to block the client.
        SB_Thread::Sthr::sleep(-1); // in msec - forever!
    }
 
    TMlibTrace(("TMLIB_TRACE : send_tm EXIT returning error %d.\n", lv_ret), 2);
 
    return lv_ret;
}

// ---------------------------------------------------------
// send_tm_link
// Purpose - send message to the given TM
// ---------------------------------------------------------
short TMLIB::send_tm_link(char *pp_req, int buffer_size, Tm_Rsp_Msg_Type *pp_rsp,
                    int pv_node)
{
    //ushort    lv_req_len = sizeof (Tm_Req_Msg_Type);
    int       lv_rsp_len = sizeof (Tm_Rsp_Msg_Type);
    int       lv_msgid;
    TPT_DECL( lv_phandle);
    short     la_results[6];
    short     lv_ret = FEOK;
    int32     lv_linkRetries = 0;
    const int32 lc_maxLinkRetries = 100;
    const int32 lc_linkPause = 3000; // 3 second

    TMlibTrace(("TMLIB_TRACE : send_tm (node %d) ENTRY\n", pv_node), 2);

    if (!gv_tmlib.open_tm(pv_node))
    {
         TMlibTrace(("TMLIB_TRACE : returning FETMFNOTRUNNING\n"), 1);
         return FETMFNOTRUNNING;
    }

    // get phandle and (FOR NOW) abort.  if we get into this
    // method, we should have already opened and stored the tm
    gv_tmlib.phandle_get(&lv_phandle, pv_node);
    do {
       lv_ret = BMSG_LINK_(&lv_phandle,                  // phandle
                        &lv_msgid,                   // msgid
                        NULL,                        // reqctrl
                        0,                           // reqctrlsize
                        NULL,                        // replyctrl
                        0,                           // replyctrlmax
                        pp_req,                      // reqdata
                        buffer_size,                 // reqdatasize
                        (char *) pp_rsp,             // replydata
                        lv_rsp_len,                  // replydatamax
                        0,                           // linkertag
                        TMLIB_LINK_PRIORITY,         // pri
                        0,                           // xmitclass
                        0);                          // linkopts
       lv_linkRetries++;
       if ((lv_ret == FENOLCB) && (lv_linkRetries <= lc_maxLinkRetries) &&
         (lv_linkRetries > 1))
          SB_Thread::Sthr::sleep(lc_linkPause); // in msec
    } while (lv_ret == FENOLCB && ++lv_linkRetries <= lc_maxLinkRetries);


    if (lv_ret)
    {
       TMlibTrace(("TMLIB_TRACE : send_tm , BMSG_LINK error is %d\n", lv_ret), 3);
    }
    else
    {
       lv_ret = BMSG_BREAK_(lv_msgid, la_results, &lv_phandle);
       if (lv_ret)
       {
          TMlibTrace(("TMLIB_TRACE : send_tm , BMSG_BREAK error is %d\n", lv_ret), 3);
       }
    }

    switch (lv_ret)
    {
    case FEPATHDOWN:
       lv_ret = FETMFNOTRUNNING;
       break;
    case FESERVICEDISABLED:
        SB_Thread::Sthr::sleep(-1); // in msec - forever!
    }

    TMlibTrace(("TMLIB_TRACE : send_tm EXIT returning error %d.\n", lv_ret), 2);

    return lv_ret;
}


// ---------------------------------------------------------
// beginner_nid
// Purpose - return the nid of the TM process to be used
// to begin the next transaction.
// If iv_txn_distribute is NODE_LOCAL_BEGINS then this is
//    always the node of the calling process.
// However, if it is CLUSTER_WIDE_BEGINS then the TMLib
//    allocated the next nid in the cluster.
// ---------------------------------------------------------
int TMLIB::beginner_nid()
{
    int lv_nid = gv_tmlib.iv_my_nid;
    
    // First time through work out the txn distribution algorithm.
    if (iv_txn_distribute == DIST_NOT_SET)
    {
        int lv_txn_dist = 0;
        ms_getenv_int("TMLIB_TXN_DISTRIBUTION", &lv_txn_dist);
        if (lv_txn_dist > 0)
            iv_txn_distribute = (TRANSACTION_DISTRIBUTION) lv_txn_dist;
        else
            iv_txn_distribute = DEFAULT_TRANSACTION_DISTRIBUTION;
        TMlibTrace(("TMLIB_TRACE : TMLIB::beginner_nid : TMLIB_TXN_DISTRIBUTION set to %d\n",
               iv_txn_distribute), 2);
   }

    if (gv_tmlib.iv_txn_distribute == CLUSTER_WIDE_BEGINS)
    {
        lv_nid = gv_tmlib.iv_next_nid;
        // distribution algorightm is a simple round-robin for now
        gv_tmlib.iv_next_nid++;
        if (gv_tmlib.iv_next_nid >= gv_tmlib.iv_node_count)
            gv_tmlib.iv_next_nid = 0;
    }

    TMlibTrace(("TMLIB_TRACE : TMLIB::beginner_nid : Beginning transaction on node %d\n",
               lv_nid), 2);
    return lv_nid;
} //beginner_nid

// ---------------------------------------------------------
// new_tag
// Purpose - allocate a new tag for a begintransaction call.
// ---------------------------------------------------------
unsigned int TMLIB::new_tag() 
{
    if (iv_next_tag == 0U || iv_next_tag == MAX_TXN_TAGS)
        iv_next_tag = 1U;
    unsigned int lv_tag = iv_next_tag++;
    TMlibTrace(("TMLIB_TRACE : TMLIB::new_tag : Allocating tag %d, next tag %d.\n", 
        lv_tag, iv_next_tag), 3);
    return lv_tag;
}


short TMLIB::setupJNI()
{
   jclass lv_javaClass;
   TMLibJavaMethods_ = new JavaMethodInit[JM_LAST];
   TMLibJavaMethods_[JM_CTOR                  ].jm_name      = "<init>";
   TMLibJavaMethods_[JM_CTOR                  ].jm_signature = "()V";
   TMLibJavaMethods_[JM_INIT1                 ].jm_name      = "init";
   TMLibJavaMethods_[JM_INIT1                 ].jm_signature = "(S)Z";
   TMLibJavaMethods_[JM_ABORT                 ].jm_name      = "abortTransaction";
   TMLibJavaMethods_[JM_ABORT                 ].jm_signature = "(J)S";
   TMLibJavaMethods_[JM_TRYCOMMIT             ].jm_name      = "tryCommit";
   TMLibJavaMethods_[JM_TRYCOMMIT             ].jm_signature = "(J)S";
   TMLibJavaMethods_[JM_CLEARTRANSACTIONSTATES].jm_name      = "clearTransactionStates";
   TMLibJavaMethods_[JM_CLEARTRANSACTIONSTATES].jm_signature = "(J)V";

   for (int i = 0 ; i < JM_LAST ; i++)
       TMLibJavaMethods_[i].methodID = NULL;
   
   short ret = JavaObjectInterfaceTM::init((char *)hbasetxclient_classname, hbasetxclient_class, 
                                           TMLibJavaMethods_, JM_LAST_HBASETXCLIENT, false);
   if (ret == JOI_OK) {
      if (enableCleanupRMInterface()) {
         // Setup call to RMInterface.clearTransactionStates
	 lv_javaClass = _tlp_jenv->FindClass(rminterface_classname); 
	 if (lv_javaClass != NULL) {
            RMInterface_class = (jclass)_tlp_jenv->NewGlobalRef(lv_javaClass);
            TMLibJavaMethods_[JM_CLEARTRANSACTIONSTATES].methodID =
                     _tlp_jenv->GetStaticMethodID(RMInterface_class,
                                                  TMLibJavaMethods_[JM_CLEARTRANSACTIONSTATES].jm_name.data(),
                                                  TMLibJavaMethods_[JM_CLEARTRANSACTIONSTATES].jm_signature.data());
         }
         else {
            fprintf(stderr,"FindClass for class name %s failed. Aborting.\n",rminterface_classname);
            fflush(stderr);
            abort();
         }
      }
   }
   else {
      fprintf(stderr,"JavaObjectInterfaceTM::init returned error %d. Aborting.\n",ret);
      fflush(stderr);
      abort();
   }
   return ret;
} //setupJNI


///////////////////////////////////////////////
//                 JNI Methods                              //
///////////////////////////////////////////////
short TMLIB::initConnection(short pv_nid)
{
  jshort   jdtmid = pv_nid;
  //sleep(30);
  _tlp_jenv->CallBooleanMethod(javaObj_, TMLibJavaMethods_[JM_INIT1].methodID, jdtmid);
  if (getExceptionDetails(NULL)) {
     tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"TMLIB::initConnection()", (char *)_tlp_error_msg->c_str(), -1);
     return RET_EXCEPTION;
  }
  // Ignore result and return JOI_OK
  return JOI_OK;
}


void TMLIB::cleanupTransactionLocal(long transactionID)
{
  if (enableCleanupRMInterface() == false)
     return;
  initJNI();
  jlong   jlv_transid = transactionID;
  _tlp_jenv->CallStaticVoidMethod(RMInterface_class, TMLibJavaMethods_[JM_CLEARTRANSACTIONSTATES].methodID, jlv_transid);
  if (getExceptionDetails(NULL)) {
     tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"TMLIB::cleanupTransactionLocal()", (char *)_tlp_error_msg->c_str(), -1);
     _tlp_jenv->PopLocalFrame(NULL);
  }
  _tlp_jenv->PopLocalFrame(NULL);
  return;
} //cleanupTransactionLocal


short TMLIB::endTransactionLocal(long transactionID)
{
  jlong   jlv_transid = transactionID;
  initJNI();
  jshort jresult = _tlp_jenv->CallShortMethod(javaObj_, TMLibJavaMethods_[JM_TRYCOMMIT].methodID, jlv_transid);
  if (getExceptionDetails(NULL)) {
     tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"TMLIB::endTransaction()", (char *)_tlp_error_msg->c_str(), -1);
     _tlp_jenv->PopLocalFrame(NULL);
     return RET_EXCEPTION;
  }
  _tlp_jenv->PopLocalFrame(NULL);
  //  RET_NOTX means the transaction wasn't found by the HBase client code (trx).  This is ok here, it
  //  simply means the transaction hasn't been seen by the HBase client code, so no work was done on it.
  if (jresult == RET_NOTX)
  {
     // printf("TMLIB::endTransactionLocal returning RET_NOTX(1) - empty txn.\n");
     return RET_OK;
  } 
  return jresult;
} //endTransactionLocal


short TMLIB::abortTransactionLocal(long transactionID)
{
  jlong   jlv_transid = transactionID;
  initJNI();
  jshort jresult = _tlp_jenv->CallShortMethod(javaObj_, TMLibJavaMethods_[JM_ABORT].methodID, jlv_transid);
  if (getExceptionDetails(NULL)) {
     tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"TMLIB::abortTransaction()", (char *)_tlp_error_msg->c_str(), -1);
     _tlp_jenv->PopLocalFrame(NULL);
     return RET_EXCEPTION;
  }
  _tlp_jenv->PopLocalFrame(NULL);
  //  RET_NOTX means the transaction wasn't found by the HBase client code (trx).  This is ok here, it
  //  simply means the transaction hasn't been seen by the HBase client code, so no work was done on it.
  if (jresult == RET_NOTX)
  {
    return RET_OK;
  } 

  return jresult;
} //abortTransactionLocal

bool TMLIB::close_tm() 
{
   TPT_DECL       (lv_phandle);
   if (!gv_tmlib_initialized)
      return true;
   for (int i = 0; i < iv_node_count; i++) {
      if (phandle_get(&lv_phandle, i) == true)
         msg_mon_close_process(&lv_phandle);
   }
   return true;
}

//----------------------------------------------------------------------------
// DTM_LOCALTRANSACTION
// Purpose: Returns true if local transactions are
// supported, otherwise false.
// Also returns the current transid
//----------------------------------------------------------------------------
bool DTM_LOCALTRANSACTION(int32 *pp_node, int32 *pp_seqnum)
{

   // instantiate a gp_trans_thr object for this thread if needed.
   if (gp_trans_thr == NULL)
      gp_trans_thr = new TMLIB_ThreadTxn_Object();

   if (!gv_tmlib_initialized)
      gv_tmlib.initialize();

   bool lv_local = gv_tmlib.localBegin();
   TM_Transid     *lp_transid = NULL;

   // instantiate a gp_trans_thr object for this thread if needed.
   if (gp_trans_thr == NULL)
      gp_trans_thr = new TMLIB_ThreadTxn_Object();
    
   short lv_error = tmlib_check_active_tx();
   if (lv_error) {
      TMlibTrace(("TMLIB_TRACE : DTM_LOCALTRANSACTION returning with error %d\n",
                  lv_error), 2);
      *pp_node = *pp_seqnum = -1;
      return false;
   }
   lp_transid = gp_trans_thr->get_current()->getTransid();

   if (lp_transid == NULL) {
      TMlibTrace(("TMLIB_TRACE : DTM_LOCALTRANSACTION failed, aborting\n"), 1);
      abort();
   }
   *pp_node = lp_transid->get_node();
   *pp_seqnum = lp_transid->get_seq_num();
   return lv_local;
}


