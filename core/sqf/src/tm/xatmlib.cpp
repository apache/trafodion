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
#include <unistd.h>
#include <sys/time.h>

#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/trace.h"

#include "dtm/tm_util.h"

#include "dtm/xa.h"
#include "rm.h"
#include "tmrm.h"
#include "xatmglob.h"
#include "tmlibmsg.h"
#include "tmmap.h"
#include "tmdeque.h"
#include "tmlogging.h"
#include "tmtimer.h"
#include "tmpool.h"
#include "xatmmsg.h"
#include "tmregistry.h"
#include "xatmlib.h"
#include "xatmapi.h"

// Externals

// Global functions


// -------------------------------------------------------------------
// XIDtotransid
// Purpose - Extract the transaction id from an XID.
// ** NOTE THIS FUNCTION DEPENDS ON XID AND TRANSID FORMATS! **
// This is only done for branches associated with a TSE where the
// XID.data contains the transid.  The transid is then used
// to maintain a list of Seabed msgids per transaction to allow
// requests to be cancelled when complete_all completes to avoid
// replies left over from previous interactions confusing completions.
// To ensure we only get TSE specific XIDs, the XID.formatID is checked
// to make sure it is correct (FORMAT_ID = 403).
// Any XIDs of different formats will return 0.
// -------------------------------------------------------------------
int64 XIDtotransid(XID *pp_xid)
{
   union u_
   {
      int64 i;
      char c[8];
   } lv_transid;

   if (pp_xid->formatID == FORMAT_ID)
   {
      memcpy(&lv_transid.c, pp_xid->data, 8);
      return lv_transid.i;
   }
   else 
      return 0;
}


// -------------------------------------------------------------------
// XIDtoa
// Purpose - Convert an xid to a character string for tracing.
// Note: Callers to this procedure must be careful to use the returned
//         string before a thread switch occurs as it could cause another 
//         call to XIDtoa and overwrite the string.
// -------------------------------------------------------------------
char * XIDtoa(XID *pp_xid)
{
   static char lv_xid[11+11+11+XIDDATASIZE+1];
   union u_
   {
      int32 i;
      char c[4];
   } lv_seqNum;
   memcpy(&lv_seqNum.c, pp_xid->data, 4);

   sprintf((char *) &lv_xid, "%d:%d:%d:ID %d",
            pp_xid->formatID, 
            pp_xid->gtrid_length, 
            pp_xid->bqual_length, 
            lv_seqNum.i);

   return (char *) &lv_xid;
}


// CxaTM_TM Methods
// TM Default Constructor
CxaTM_TM::CxaTM_TM() 
{
   // Mutex attributes: Recursive = true, ErrorCheck=false
   ip_mutex = new TM_Mutex(true, false);
   iv_tm_stats = false;

   lock();
   iv_initialized = false;
   my_nid(-1);     // Indicates that the node hasn't been set yet.
   iv_traceMask = XATM_TraceOff;

   iv_RMmsgMax = MAX_NUM_RMMESSAGES;
   iv_RMmsgSteadyLow = STEADYSTATE_LOW_RMMESSAGES;
   iv_RMmsgSteadyHigh = STEADYSTATE_HIGH_RMMESSAGES;
   iv_lastMonError = 0;
   iv_RMmsgTotal = 0;
   iv_RMmsgPoolThresholdEventCounter = 0;
   
   // Initialize RMMessagePool
   ip_RMMessagePool = new CTmPool<CxaTM_RMMessage>(gv_xaTM.tm_stats(), MAX_NUM_RMMESSAGES,
                                 STEADYSTATE_LOW_RMMESSAGES, STEADYSTATE_HIGH_RMMESSAGES);
#ifndef XARM_BUILD_
   ip_tmTimer = NULL;
#endif
   iv_next_msgNum = 1;
   unlock();
}

// TM Destructor
CxaTM_TM::~CxaTM_TM() 
{
   delete ip_mutex;
   delete ip_RMMessagePool;
   XATrace(XATM_TraceExit, ("XATM: CxaTM_TM::~CxaTM_TM Exit.\n"));
}

// TM deleteRM
// Remove RM from rmList, cleanup and delete the RM object.
void CxaTM_TM::deleteRM(CxaTM_RM *pp_RM)
{
   XATrace(XATM_TraceDetail, ("XATM: CxaTM_TM::deleteRM ENTRY.\n"));

   lock();
   ia_rmList.remove(pp_RM->getRmid());
   delete pp_RM;
   unlock();
   XATrace(XATM_TraceDetail, ("XATM: CxaTM_TM::deleteRM EXIT.\n"));
}

// TM lock semaphore
void CxaTM_TM::lock()
{
   XATrace(XATM_TraceLock, ("XATM: CxaTM_TM::lock, count %d, owner %ld\n", 
           ip_mutex->lock_count(), ip_mutex->lock_owner()));

   int lv_error = ip_mutex->lock();
   if (lv_error)
   {
      XATrace(XATM_TraceError, ("XATM: CxaTM_TM::lock returned error %d.\n", lv_error));
      abort();
   }
}


// TM new RM
// Lookup the rmList for this rmid.  If found, we assume 
// this is a reopen and return the RM, otherwise 
// instantiate a new CxaTM_RM object and return that.
// Return codes:
//    XA_OK       Success, RM object returned
//    XA_RETRY    Success, RM found in rmList (assume reopen)
//    XAER_RMERR  Failure, rmid already in use
int CxaTM_TM::newRM(int pv_rmid, CxaTM_RM **ppp_RM)
{
   int lv_xaError = XA_OK;
   XATrace(XATM_TraceExit,("XATM: CxaTM_TM::newRM ENTRY rmid (%d)\n", pv_rmid));
   CxaTM_RM * lp_RM = (CxaTM_RM *) ia_rmList.get(pv_rmid);

   // if we found the rmid in the rmList then return XA_RETRY to indicate
   // we are reusing it. Assume it's a re-open.
   if (lp_RM)
   {
      lv_xaError = XA_RETRY;
      *ppp_RM = lp_RM;
   }
   else
   {
      // Instantiate and initialize the RM object
      *ppp_RM = new CxaTM_RM(pv_rmid);

      // Insert RM into the rmList.
      lock();
      ia_rmList.put(pv_rmid, *ppp_RM);
      unlock();
   }

   XATrace((lv_xaError?XATM_TraceExitError:XATM_TraceExit),
           ("XATM: CxaTM_TM::newRM EXIT rmid(%d) returning %s.\n", 
            pv_rmid, XAtoa(lv_xaError)));
   return lv_xaError;
} //CxaTM_TM::newRM


// CxaTM_TM::setAndGetNid
// Get the node number
// The first time this is called it will retrieve the value from
// the Monitor.
inline int CxaTM_TM::setAndGetNid()
{
   lock();
   if (my_nid() == -1)
   {
      msg_mon_get_process_info(NULL, &iv_my_nid, &iv_my_pid);
   }
   unlock();
   return my_nid();
} //setAndGetNid


// CxaTM_TM::initialize
// Initialize the CxaTM_TM object
// 
int CxaTM_TM::initialize(XATM_TraceMask pv_traceMask, bool pv_tm_stats, CTmTimer *pp_tmTimer)

{
   char la_value[9];
   bool lv_success = false;
   int lv_error = 0;

   setxaTrace(pv_traceMask);
   lock();
   gv_xaTM.setxaTrace(pv_traceMask);
   iv_tm_stats = pv_tm_stats;

   //initialize pool limits
   int32 lv_max_num_RMmsgs=0;
   int32 lv_ss_low_RMmsgs=0;
   int32 lv_ss_high_RMmsgs=0;

   if (pp_tmTimer != NULL) 
   {
      ip_tmTimer = pp_tmTimer;
      gv_startTime = ip_tmTimer->startTime();

      // Configure RMMessagePool
      lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                           (char *) CLUSTER_GROUP, (char *) DTM_MAX_NUM_RMMESSAGES, 
                           la_value);
      lv_max_num_RMmsgs = ((lv_error == 0)?atoi(la_value):-1);
      lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                           (char *) CLUSTER_GROUP, (char *) DTM_STEADYSTATE_LOW_RMMESSAGES, 
                           la_value);
      lv_ss_low_RMmsgs = ((lv_error == 0)?atoi(la_value):-1);
      lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                           (char *) CLUSTER_GROUP, (char *) DTM_STEADYSTATE_HIGH_RMMESSAGES, 
                           la_value);
      lv_ss_high_RMmsgs = ((lv_error == 0)?atoi(la_value):-1);
   }
   else 
   {
      ip_tmTimer = NULL;
      gv_startTime = Ctimeval::now();
      lv_max_num_RMmsgs=1024;
      lv_ss_low_RMmsgs=1;
      lv_ss_high_RMmsgs=1024;
   }

   lv_success = ip_RMMessagePool->setConfig(gv_xaTM.tm_stats(), lv_max_num_RMmsgs, 
                                             lv_ss_low_RMmsgs, lv_ss_high_RMmsgs);
   if (lv_success)
   {
      XATrace (XATM_TraceAPIError, ("RMMessage pool parameters set: "
                  "Max %d, steady state low %d, steady state high %d.\n",
                  lv_max_num_RMmsgs, lv_ss_low_RMmsgs, lv_ss_high_RMmsgs));
   }
   else
   {
      XATrace (XATM_TraceAPIError, ("Attempt to set RMMessage pool parameters failed: "
                  "Max %d, steady state low %d, steady state high %d.\n",
                  lv_max_num_RMmsgs, lv_ss_low_RMmsgs, lv_ss_high_RMmsgs));
   }

   iv_initialized = true;
   unlock();

   XATrace((lv_success?XATM_TraceExit:XATM_TraceExitError),
           ("XATM: CxaTM_TM::initialize EXIT returning %d.\n", lv_error));
   return lv_error;
}

// CxaTM_TM::initializePhandle
// Initialize a phandle to zeros.
inline void CxaTM_TM::initalizePhandle(SB_Phandle_Type *pp_phandle)
{
   memset(pp_phandle, 0, sizeof (SB_Phandle_Type));
}

// CxaTM_TM::setPhandle
// Set a (destination) phandle to a supplied (source) phandle.
inline void CxaTM_TM::setPhandle(SB_Phandle_Type *pp_destPhandle,
                                 SB_Phandle_Type *pp_sourcePhandle)
{
   memcpy(pp_destPhandle, pp_sourcePhandle, sizeof (SB_Phandle_Type));
}

// CxaTM_TM::mapMonErr_To_xaErr
// Map an error code returned by the Monitor into an XA error code.
inline int CxaTM_TM::mapMonErr_To_xaErr(int32 pv_error)
{
   int lv_xaerror = XA_OK;
   if (pv_error)
   {
      gv_xaTM.lastMonError(pv_error);
      lv_xaerror = XAER_PROTO;
   }

   // To do: expand error handling
   return lv_xaerror;
}

// TM unlock semaphore
void CxaTM_TM::unlock()
{
   XATrace(XATM_TraceLock, ("XATM: CxaTM_TM::unlock, count %d, owner %ld\n", 
           ip_mutex->lock_count(), ip_mutex->lock_owner()));

   int lv_error = ip_mutex->unlock();
   if (lv_error)
   {
      XATrace(XATM_TraceError, ("XATM: CxaTM_TM::unlock returned error %d.\n", lv_error));
      abort();
   }
}

// TM xaTrace 
// Returns the value of iv_trace.
bool CxaTM_TM::xaTrace(XATM_TraceMask pv_traceMask)
{
   return ((pv_traceMask & iv_traceMask)? true: false);
}

// TM setxaTrace
// Sets the value of iv_traceMask.
// Note that because this is a mask it is concatenated to the mask unless
// set to 0. Ie:
// If pv_traceMask == 0, set iv_traceMask = 0
// If pv_traceMask > 0, iv_traceMask |= pv_traceMask; (bit-wise OR)
void CxaTM_TM::setxaTrace(XATM_TraceMask pv_traceMask)
{
   XATM_TraceMask iv_OldMask = iv_traceMask;

   lock();
   if (pv_traceMask == XATM_TraceOff)
      iv_traceMask = XATM_TraceOff;
   else
      iv_traceMask = (XATM_TraceMask) (iv_traceMask | pv_traceMask);

   gv_XATM_traceMask = iv_traceMask;
   unlock();

   // Don't use XATrace here as we always want to write the trace record.
   if (iv_OldMask != XATM_TraceOff && pv_traceMask == XATM_TraceOff)
      trace_printf("XATM:Tracing off.\n");
   else
      if (pv_traceMask != XATM_TraceOff)
         trace_printf("XATM: Tracing on, Mask=0x%x.\n", iv_traceMask);
}


// ------------------------------------------------------------------
// RM Message object related helper functions:
// ------------------------------------------------------------------
// new_RMmsg
// Purpose : Allocate a new RM message object.
// This could be taken from the freeList or
// instantiate a new CTM_RMMessage object, depending on the
// number of current RM Message objects and configuration of the
// RMMessagePool.
// Returns a pointer to the instantiated message if successful or
//         NULL if it fails to allocate a message object.
// ------------------------------------------------------------------
CxaTM_RMMessage * CxaTM_TM::new_RMmsg()
{
   CxaTM_RMMessage * lp_msg = NULL;
   bool lv_reused = false;

   XATrace(XATM_TraceRMMsg, ("CxaTM_TM::new_RMmsg : ENTRY.\n"));

   lock();
   int64 lv_next_msgNum = gv_xaTM.next_msgNum();
   XATrace(XATM_TraceRMMsg, ("CxaTM_TM::new_RMmsg : Calling CTmPool<CxaTM_RMMessage>::newElement "
         "next index is " PFLL ", poolInUseCount=" PFLL ", freeCount=" PFLL ".\n", 
         lv_next_msgNum, ip_RMMessagePool->get_inUseList()->size(),
         ip_RMMessagePool->get_freeList()->size()));

   lp_msg = ip_RMMessagePool->newElement(lv_next_msgNum, &lv_reused, true);
   //, (void *) &CxaTM_RMMessage::constructPoolElement);

   if (lp_msg == NULL)
   {
      tm_log_event(DTM_XATM_MAX_RM_MSG, SQ_LOG_CRIT,"DTM_XATM_MAX_RM_MSG",
                  -1, /*error_code*/ 
                  -1, /*rmid*/
                  setAndGetNid(), /*dtmid*/ 
                  -1, /*seq_num*/
                  -1, /*msgid*/
                  -1, /*xa_error*/
                  ip_RMMessagePool->get_maxPoolSize(), /*pool_size*/
                  ip_RMMessagePool->totalElements()    /*pool_elems*/);
      XATrace (XATM_TraceError, ("CxaTM_TM::new_RMmsg : RM Message pool max elements hit. "
                                 "RMMessage object not allocated by CTmPool<CxaTM_RMMessage>::newElement. "
                                 "Current pool size=%d, Pool maxSize=%d\n",
                                 ip_RMMessagePool->get_maxPoolSize(), ip_RMMessagePool->totalElements()));
   } 
   else
   {
       if (!lv_reused)
          gv_xaTM.inc_next_msgNum();

      XATrace(XATM_TraceRMMsg, ("CxaTM_TM:new_RMmsg : CTmPool<CxaTM_RMMessage>::newElement returned "
            "reused %d, RMMessage object %p, msgNum " PFLL ", poolInUseCount=" PFLL ", freeCount=" PFLL ".\n", 
            lv_reused, (void *) lp_msg, lp_msg->msgNum(), ip_RMMessagePool->get_inUseList()->size(),
            ip_RMMessagePool->get_freeList()->size()));
   }
   unlock();

   XATrace(XATM_TraceRMMsg, ("CxaTM_TM::new_RMmsg : EXIT returning %p.\n", lp_msg));

   return lp_msg;
} //new_RMmsg


// ------------------------------------------------------------------
// release_RMmsg
// Purpose : Release an RMmsg object.
// Return to the RMMessagePool.
// NOTE: Best practice is for the caller should remove the RM message 
// from the RMs RM message list prior to calling release_RMmsg.
// ------------------------------------------------------------------
void CxaTM_TM::release_RMmsg(CxaTM_RMMessage *pp_msg)
{
   XATrace(XATM_TraceRMMsg, ("CxaTM_TM::release_RMmsg : ENTRY msg object %p" 
                             ", rmid %d, msgNum " PFLL ", msgid %d.\n", 
                             (void *) pp_msg, pp_msg->RM()->getRmid(), 
                             pp_msg->msgNum(), pp_msg->msgid()));

   lock();
   // Check the RM objects message list and remove this message if it's still there
   pp_msg->RM()->msgList()->remove(pp_msg->msgid());

   XATrace(XATM_TraceRMMsg, ("CxaTM_TM::releaseRMmsg : Calling CTmPool<CxaTM_RMMessage>::deleteElement "
          "index " PFLL ", poolInUseCount=" PFLL ", freeCount=" PFLL ".\n", pp_msg->msgNum(), 
          ip_RMMessagePool->get_inUseList()->size(),
          ip_RMMessagePool->get_freeList()->size()));

   ip_RMMessagePool->deleteElement(pp_msg->msgNum());
   unlock();

   XATrace(XATM_TraceRMMsg, ("CxaTM_TM::release_RMmsg : EXIT, poolInUseCount=" PFLL ", freeCount=" PFLL " .\n", 
           ip_RMMessagePool->get_inUseList()->size(),
           ip_RMMessagePool->get_freeList()->size()));
} //CxaTM_TM::release_RMmsg

// ------------------------------------------------------------------
// CxaTM_TM::insert_txnMsgList
// Purpose : Insert the message into the transaction msgList
// Note: this method assumes the XID contains a transid
// ------------------------------------------------------------------
void CxaTM_TM::insert_txnMsgList(XID *pp_xid, CxaTM_RMMessage * pp_msg)
{
   XATrace(XATM_TraceExit, ("CxaTM_TM::insert_txnMsgList : Entry. xid=%s, msgNum=" PFLL ", msgid=%d\n",
      XIDtoa(pp_xid), pp_msg->msgNum(), pp_msg->msgid()));
   lock();
   pp_msg->xid(pp_xid);
   gv_xaTM.txnMsgList()->put(XIDtotransid(pp_xid), pp_msg);
   unlock();
} //CxaTM_TM::insert_txnMsgList


// ------------------------------------------------------------------
// CxaTM_TM::cancelAll
// Purpose : cancel all outstanding requests associated with the  
// transaction pv_transid supplied and remove from the transaction & RM msgLists.
// pv_count is the number of messages outstanding after complete_all.
// pv_numMsgs is the number of messages sent.
// ------------------------------------------------------------------
void CxaTM_TM::cancelAll(int32 pv_count, int32 pv_numMsgs, int64 pv_transid)
{
   int32 lv_index = 0;
   bool lv_locked = false;

   XATrace(XATM_TraceExit, ("CxaTM_TM::cancelAll : ENTRY msgs to be cancelled %d, outstanding msgs %d, "
      "transid " PFLLX "\n", pv_count, pv_numMsgs, pv_transid));
   if (pv_count > 0)
   {
      lock();
      lv_locked = true;
      CxaTM_RMMessage *lp_msg = (CxaTM_RMMessage *) ia_txnMsgList.get_first(pv_transid);
      while (lv_index++ < pv_count && lp_msg != NULL)
      {
         // We check the transid on the RMMessage object to make sure it still applies to
         // this transaction and hasn't been released and reused.
         // Also need to make sure the message is still valid and in the list before we abandon!
         int64 lv_transid = XIDtotransid(lp_msg->xid());
         if (pv_transid == lv_transid && lp_msg->msgid() && lp_msg->RM() 
             && lp_msg->RM()->msgList()->get(lp_msg->msgid()))
         {
#ifndef XARM_BUILD_
            if (lp_msg->EIDState() != EID_STATE_INUSE)
            {
               tm_log_event(DTM_XATM_MSG_INTEGRITY_FAILED1, SQ_LOG_CRIT, "DTM_XATM_MSG_INTEGRITY_FAILED1",
                           -1, lp_msg->RM()->getRmid(), -1, -1, lp_msg->msgid(),
                           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, NULL, gv_xaTM.setAndGetNid());
               XATrace(XATM_TraceError, ("CxaTM_TM::cancelAll: ERROR Msg object EID not in use!\n"));
               abort();
            }
#endif
            XATrace(XATM_TraceDetail, ("CxaTM_TM::cancelAll : ID transid " PFLLX ": cancelling "
              "message: msgNum " PFLL ", msgid %d\n", pv_transid, lp_msg->msgNum(), lp_msg->msgid()));
            lp_msg->abandon();
            //lp_msg->cancel(true /*releaseMsg*/);
         }
         lp_msg = (CxaTM_RMMessage *) ia_txnMsgList.get_next(pv_transid);
      }
      ia_txnMsgList.get_end();
   }
   //Remove all entries for this seqNum.
   int lv_countRemoved = ia_txnMsgList.remove_all(pv_transid); 
   if (lv_locked)
      unlock();
   XATrace(XATM_TraceExit, ("CxaTM_TM::cancelAll : EXIT transid " PFLLX ", removed %d "
      "elements from txnMsgList.\n", pv_transid, lv_countRemoved));
} //CxaTM_TM::cancelAll


// ------------------------------------------------------------------
// CxaTM_TM::check_msgIntegrity
// Purpose : Lookup pp_msg in the txnMsgList.  This is used to
// match replies to requests to check the transaction thread is 
// receiving the right messages.
// NOTE: This routine asserts that the sending thread also completes 
// requests to subordinate RM.
// ------------------------------------------------------------------
bool CxaTM_TM::check_msgIntegrity(int64 pv_transid, CxaTM_RMMessage * pp_msg)
{
   bool lv_found = false;
   long int lv_my_threadId = SB_Thread::Sthr::self_id();

   XATrace(XATM_TraceExit, ("CxaTM_TM::check_msgIntegrity : ENTRY transid " PFLLX ", msgid %d\n", 
           pv_transid, pp_msg->msgid()));
   // Ignore if the transid wasn't supplied
   if (pv_transid == 0)
       return true;

   lock();
   CxaTM_RMMessage *lp_txnMsgList_msg = (CxaTM_RMMessage *) ia_txnMsgList.get_first(pv_transid);
   while (!lv_found && lp_txnMsgList_msg != NULL)
   {
        if (pp_msg == lp_txnMsgList_msg)
        {
           lv_found = true;
           // Double check thread
           if (lp_txnMsgList_msg->threadId() != lv_my_threadId)
           {
              tm_log_event(DTM_XATM_MSG_INTEGRITY_FAILED2, SQ_LOG_CRIT, "DTM_XATM_MSG_INTEGRITY_FAILED2",
                           -1, pp_msg->RM()->getRmid(), -1, -1, pp_msg->msgid(),
                           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, NULL, gv_xaTM.setAndGetNid());
              XATrace(XATM_TraceError, ("CxaTM_TM::check_msgIntegrity: ERROR "
                      "completion thread %ld does not match sender %ld! ID "
                      "transid " PFLLX ", msgid %d\n", 
                      lv_my_threadId, lp_txnMsgList_msg->threadId(), 
                      pv_transid, pp_msg->msgid()));
              abort();
           }
        }
        else
           lp_txnMsgList_msg = (CxaTM_RMMessage *) ia_txnMsgList.get_next(pv_transid);
   }
   ia_txnMsgList.get_end();
   unlock();

   if (!lv_found)
   {
      tm_log_event(DTM_XATM_MSG_INTEGRITY_FAILED3, SQ_LOG_CRIT, "DTM_XATM_MSG_INTEGRITY_FAILED3",
                   -1, pp_msg->RM()->getRmid(), -1, -1, pp_msg->msgid(),
                   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, NULL, gv_xaTM.setAndGetNid());
      XATrace(XATM_TraceError, ("CxaTM_TM::check_msgIntegrity: ERROR "
              "message returned by Seabed not found in txnMsgList! ID "
              "transid " PFLLX ", msgid %d\n", 
              pv_transid, pp_msg->msgid()));
      abort();
   }
   XATrace(XATM_TraceExit, ("CxaTM_TM::check_msgIntegrity : EXIT transid " PFLLX ", msgid %d "
      "returning %d.\n", pv_transid, pp_msg->msgid(), lv_found));
   return lv_found;
} //CxaTM_TM::check_msgIntegrity


//------------------------------
// CxaTM_RM Methods
//------------------------------

// Utility methods
// RM Default Constructor
CxaTM_RM::CxaTM_RM()
{
   // Mutex attributes: Recursive = true, ErrorCheck=false
   ip_mutex = new TM_Mutex(true, false);

   lock();
   iv_rmid = -1;

   // Set the flag to say we haven't completed an xa_recover call
   iv_recoverEnd = false;

   iv_maxSendRetries = MAX_TSE_SEND_RETRIES;
   
   memset(iv_xarmName, 0, RMNAMESZ);
   memset(ia_xarmInfo, 0, MAXINFOSIZE);
   memset(&iv_phandle, 0, sizeof(SB_Phandle_Type));
   memset(iv_tmName, 0, MAXPROCESSNAME); 
   xarmFlags(TMNOFLAGS);
   iv_totalRetries = 0;
   unlock();
}

// RM Constructor
// This should always be used when instantiating an RM object.
CxaTM_RM::CxaTM_RM(int pv_rmid)
{
   // Mutex attributes: Recursive = true, ErrorCheck=false
   ip_mutex = new TM_Mutex(true, false);

   lock();
   iv_rmid = pv_rmid;

   // Set the flag to say we haven't completed an xa_recover call
   iv_recoverEnd = false;

   iv_maxSendRetries = MAX_TSE_SEND_RETRIES;
   
   memset(iv_xarmName, 0, RMNAMESZ);
   memset(ia_xarmInfo, 0, MAXINFOSIZE);
   memset(iv_tmName, 0, MAXPROCESSNAME);
   memset(&iv_phandle, 0, sizeof(SB_Phandle_Type));
   xarmFlags(TMNOFLAGS);
   iv_totalRetries = 0;
   unlock();
}

// CxaTM_RM Destructor
CxaTM_RM::~CxaTM_RM()
{
   lock();
   // Close the RM process now.
   msg_mon_close_process(getRmPhandle());  

   // Cleanup any messages in the RMs Outstanding Messages list.
   cleanup_msgList();
   unlock();

   delete ip_mutex;
   XATrace(XATM_TraceExit, ("XATM: CxaTM_RM::~CxaTM_RM EXIT.\n"));
}

//------------------------------------------------------------------------
// cleanup_msgList
// Purpose : Release all messages on the RMs outstanding message list.
//------------------------------------------------------------------------
void CxaTM_RM::cleanup_msgList()
{
   CxaTM_RMMessage *lp_cur_msg;
   CxaTM_RMMessage *lp_next_msg;
   XATrace(XATM_TraceExit, ("XATM: CxaTM_RM::cleanup_msgList ENTRY.\n"));

   lock();
   lp_cur_msg = (CxaTM_RMMessage *) iv_msgList.get_first();

   while (lp_cur_msg != NULL)
   {
      lp_next_msg = (CxaTM_RMMessage *) iv_msgList.get_next();
      gv_xaTM.release_RMmsg(lp_cur_msg);
      lp_cur_msg = lp_next_msg;
   }
   iv_msgList.get_end();
   unlock();

   XATrace(XATM_TraceExit, ("XATM: CxaTM_RM::cleanup_msgList EXIT.\n"));
}

int CxaTM_RM::getRmid()
{
   return iv_rmid;
}


//------------------------------------------------------------------------
// inc_totalRetries
// Purpose : Increment the totalRetries.
// Returns true if totalRetries is a multiple of TM_RM_DOWN_LOGEVENT_INTERVAL.
//------------------------------------------------------------------------
bool CxaTM_RM::inc_totalRetries()
{
    bool lv_rtn = false;
    lock();
    iv_totalRetries++;
    if (iv_totalRetries % TM_RM_DOWN_LOGEVENT_INTERVAL == 0)
        lv_rtn = true;
    unlock();
    return lv_rtn;
}


// CxaTM_RM::lock 
// Lock the RM semaphore
// Now using recursive semaphores
void CxaTM_RM::lock()
{
   XATrace(XATM_TraceLock, ("XATM: CxaTM_RM::lock rmid %d: count %d,  owner %ld acquirer %ld\n", 
           iv_rmid, ip_mutex->lock_count(), ip_mutex->lock_owner(), SB_Thread::Sthr::self_id()));

   int lv_error = ip_mutex->lock();
   if (lv_error)
   {
      XATrace(XATM_TraceError, ("XATM: CxaTM_RM::lock returned error %d.\n", lv_error));
      abort();
   }
   else
   {
       XATrace(XATM_TraceLock, ("XATM: CxaTM_RM::lock rmid %d: acquired.\n", iv_rmid));
   }
}


// --------------------------------------------------------------
// CxaTM_RM::send_rm
// Purpose - send a message to the RM
// Caller must lock CxaTM_RM object.
// --------------------------------------------------------------
int CxaTM_RM::send_rm(CxaTM_RMMessage * pp_msg) 
{
   XATrace(XATM_TraceExit,("XATM: CxaTM_RM::send_rm ENTRY.\n"));

   lock();
   int lv_error = pp_msg->send_rm();

   if (!lv_error)
   {
      // Add message to RMs outstanding RM message list.
      iv_msgList.put(pp_msg->msgid(), pp_msg);
      XATrace(XATM_TraceDetail, ("XATM: CxaTM_RM::send_rm added message(%d) to "
               "RM outstanding message list.\n", pp_msg->msgid()));
   }
   unlock();

   return lv_error;
} //CxaTM_RM::send_rm


// --------------------------------------------------------------------
// CxaTM_RM::getRmPhandle
// Get the phandle of RM.
// --------------------------------------------------------------------
SB_Phandle_Type * CxaTM_RM::getRmPhandle()
{
   return (SB_Phandle_Type *) &iv_phandle;
}

// --------------------------------------------------------------------
// CxaTM_RM::recoverSend
// Send an xa_recover message to the RM
// --------------------------------------------------------------------
int CxaTM_RM::recoverSend(int64 pv_count, int64 pv_flags, int pv_node, bool pv_dead_node, int pv_index)
{
   int lv_xaError = XA_OK;
   XATrace(XATM_TraceExit,("XATM: CxaTM_RM::recoverSend ENTRY.\n"));
   lock();
   CxaTM_RMMessage * lp_msg = gv_xaTM.new_RMmsg();
   if (lp_msg == NULL)
   {
       lv_xaError = XAER_RMFAIL;
   }
   else
   {
       lp_msg->initialize(this, TM_DP2_SQ_XA_RECOVER, 
                          RM_MsgSize(lp_msg->Req()->u.iv_recover), 
                          RM_MsgSize(lp_msg->Rsp()->u.iv_recover));
       lp_msg->Req()->u.iv_recover.iv_rmid = getRmid();
       lp_msg->Req()->u.iv_recover.iv_flags = pv_flags;
       lp_msg->Req()->u.iv_recover.iv_count = MIN(pv_count, MAX_RECOVER_XIDS);
       lp_msg->Req()->u.iv_recover.iv_recovery_index = pv_index;
       if (pv_node != -1)
       {
           lp_msg->Req()->u.iv_recover.iv_dtm_death = pv_dead_node;
           lp_msg->Req()->u.iv_recover.iv_dtm_node = pv_node;
       }
       else
       {
           lp_msg->Req()->u.iv_recover.iv_dtm_death = false;
           lp_msg->Req()->u.iv_recover.iv_dtm_node = -1;
       }
 
       lv_xaError = send_rm(lp_msg);
   }

   // release the message object back to the pool if there was an error
   if (lv_xaError && lp_msg)
      gv_xaTM.release_RMmsg(lp_msg);
   unlock();

   XATrace((lv_xaError?XATM_TraceExitError:XATM_TraceExit),
           ("XATM: CxaTM_RM::recoverSend EXIT returning %s.\n", 
            XAtoa(lv_xaError)));
   return lv_xaError;
} //recoverSend


// --------------------------------------------------------------------
// CxaTM_RM::setRmPhandle
// Set the phandle of RM specified by pp_name.
// --------------------------------------------------------------------
int CxaTM_RM::setRmPhandle(char *pp_name)
{
   SB_Phandle_Type la_rm_phandle;
   int32           lv_error = 0;
   int32           lv_oid;
   int             lv_xaError = XA_OK;

  CxaTM_TM::initalizePhandle((SB_Phandle_Type *) &la_rm_phandle);

   lv_error = msg_mon_open_process(pp_name,
                                   &la_rm_phandle,
                                   &lv_oid);

   if (lv_error)
      lv_xaError = CxaTM_TM::mapMonErr_To_xaErr(lv_error);

   if (lv_xaError == XA_OK)
   {
      lock();
      CxaTM_TM::setPhandle((SB_Phandle_Type *) &iv_phandle, (SB_Phandle_Type *) &la_rm_phandle);
      unlock();
   }
   else
      XATrace(XATM_TraceError,
              ("XATM: CxaTM_RM::setRmPhandle returning %s. msg_mon_open_process returned error %d.\n",
               XAtoa(lv_xaError),
               lv_error));

   return lv_xaError;
} //CxaTM_RM::setRmPhandle

// CxaTM_RM::unlock 
// Unlock the RM semaphore
void CxaTM_RM::unlock()
{
   XATrace(XATM_TraceLock, ("XATM: CxaTM_RM::unlock rmid %d: count %d, owner %ld\n", 
           iv_rmid, ip_mutex->lock_count(), ip_mutex->lock_owner()));

   int lv_error = ip_mutex->unlock();
   if (lv_error)
   {
      XATrace(XATM_TraceError, ("XATM: CxaTM_RM::unlock returned error %d.\n", lv_error));
      abort();
   }
}


//-------------------------------------------------------------------
// CxaTM_RM::validateRMname
// Purpose : Extract and validate the RM name from xa_info passed
// in a standard XARM xa_open call.
// Checks the pp_info is formatted correctly and
// extracts the rm name which is returned in 
// pp_rmName.
// Format must be "RM=<name>, FLAGS=<flags>"
//-------------------------------------------------------------------
short CxaTM_RM::validateRMname(const char * pp_info, char * pp_rmName)
{
   char *tmp_p = (char *) pp_info;
   char *eq_char = NULL;
   char var[MAXVARNAME];
   char value[RMNAMESZ];
   int64 my_flags = -1;
   char la_buf[DTM_STRING_BUF_SIZE];

   while (tmp_p != NULL)
   {
      //make sure it is of the form a=b
      if ((eq_char = strchr(tmp_p,'=')) == NULL)
      {
         XATrace(XATM_TraceError, ("XATM: CxaTM_RM::validateRMname: malformed "
                  "openinfo string %s. Returning XAER_INVAL",tmp_p));
         return XAER_INVAL;
      }
      int len = eq_char - tmp_p;
      strncpy(var,tmp_p,MAXVARNAME);
      var[len] = '\0';
      strncpy(value, eq_char + 1, RMNAMESZ - 1);

      if (!strcmp(var,"RM"))
      {
         /*
         * Save the RM name.  Make sure that it is less than
         * 31 characters in length and should contain only
         * valid characters: [a-zA-Z0-9$_-^@&]+.  Last character
         * must be null.
         */
         strncpy(pp_rmName,value,RMNAMESZ);
         XATrace(XATM_TraceDetail, ("XATM: CxaTM_RM::validateRMname: xa_open: RM = %s\n",
                  pp_rmName));
      }
      else if (!strcmp(var,"FLAGS"))
      {
         // No flags supported at this stage
         my_flags = atoi(value);
         /* Not sure what values we will permit at this stage.
         if (my_flags & ~(ENABLE_AUDIT|ENABLE_STATS|ENABLE_ASYNC_IMPORT)) 
         {
            XATrace(XATM_TraceError, ("XATM: CxaTM_RM::validateRMname: xa_open: "
                     "Unknown flags (0x%08X) specified.\n",
                     my_flags & ~(ENABLE_AUDIT|ENABLE_STATS|ENABLE_ASYNC_IMPORT)));
            return XAER_INVAL;
         } */
         XATrace(XATM_TraceDetail, ("XATM: CxaTM_RM::validateRMname: xa_open: FLAGS = 0x" PFLLX "\n",
                  my_flags));
      }
      else
      {
         sprintf(la_buf, "XATM: xa_open: malformed openinfo string %s.\n", tmp_p);
         tm_log_write(DTM_XATM_XA_OPEN_FAILED, SQ_LOG_WARNING, la_buf);
         XATrace (XATM_TraceError, ("XATM: CxaTM_RM::validateRMname: %s", la_buf));
         return XAER_INVAL;
      }
      tmp_p = strtok(NULL,":");
   } // while

   xarmName(pp_rmName);
   xarmFlags(my_flags);

   return XA_OK;
} // CxaTM_RM::validateRMname


// xa Implementation methods
//-------------------------------------------------------------------
// CxaTM_RM::close
// Send the xa_close message and close the RM.
//-------------------------------------------------------------------
int CxaTM_RM::close(char *info, int64 pv_flags)
{
   int lv_xaError = XA_OK;

   lock();
   CxaTM_RMMessage * lp_msg = gv_xaTM.new_RMmsg();

   if (lp_msg == NULL)
   {
       lv_xaError = XAER_RMFAIL;
   }
   else
   {
       lp_msg->initialize(this, TM_DP2_SQ_XA_CLOSE, 
                          RM_MsgSize(lp_msg->Req()->u.iv_close), 
                          RM_MsgSize(lp_msg->Rsp()->u.iv_close));
       lp_msg->Req()->u.iv_close.iv_rmid = getRmid();
       sprintf(lp_msg->Req()->u.iv_close.iv_info, info);
       lp_msg->Req()->u.iv_close.iv_flags = pv_flags;
       lp_msg->Req()->u.iv_start.iv_nid = gv_xaTM.my_nid();
       lp_msg->Req()->u.iv_start.iv_pid = gv_xaTM.my_pid();

       lv_xaError = send_rm(lp_msg);
   }

   // release the message object back to the pool if there was an error
   if (lv_xaError && lp_msg)
      gv_xaTM.release_RMmsg(lp_msg);
   unlock();

   if (lp_msg && (pv_flags & TMASYNC) && (lv_xaError == XA_OK))
      return lp_msg->msgid();
   else
      return lv_xaError;
} // CxaTM_RM::close

//-------------------------------------------------------------------
// CxaTM_RM::commit
// Send xa_commit message to RM.
//-------------------------------------------------------------------
int CxaTM_RM::commit(XID *pp_xid, int64 pv_flags)
{
   int lv_xaError = XA_OK;

   lock();
   CxaTM_RMMessage * lp_msg = gv_xaTM.new_RMmsg();

   if (lp_msg == NULL)
   {
       lv_xaError = XAER_RMFAIL;
   }
   else
   {
       lp_msg->initialize(this, TM_DP2_SQ_XA_COMMIT, 
                          RM_MsgSize(lp_msg->Req()->u.iv_commit), 
                          RM_MsgSize(lp_msg->Rsp()->u.iv_commit));
       lp_msg->Req()->u.iv_commit.iv_xid = *pp_xid;
       lp_msg->Req()->u.iv_commit.iv_rmid = getRmid();
       lp_msg->Req()->u.iv_commit.iv_flags = pv_flags;
       lp_msg->Req()->u.iv_start.iv_nid = gv_xaTM.my_nid();
       lp_msg->Req()->u.iv_start.iv_pid = gv_xaTM.my_pid();

       lv_xaError = send_rm(lp_msg);
   }

   // release the message object back to the pool if there was an error
   if (lp_msg)
   {
       if (lv_xaError)
          gv_xaTM.release_RMmsg(lp_msg);
       else
          gv_xaTM.insert_txnMsgList(pp_xid, lp_msg);
   }
   unlock();

   if (lp_msg && (pv_flags & TMASYNC) && (lv_xaError == XA_OK))
      return lp_msg->msgid();
   else
      return lv_xaError;
} //CxaTM_RM::commit

//-------------------------------------------------------------------
// CxaTM_RM::end
// Send an xa_end message to RM.
//-------------------------------------------------------------------
int CxaTM_RM::end(XID *pp_xid, int64 pv_flags)
{
   int lv_xaError = XA_OK;

   lock();
   CxaTM_RMMessage * lp_msg = gv_xaTM.new_RMmsg();

   if (lp_msg == NULL)
   {
       lv_xaError = XAER_RMFAIL;
   }
   else
   {
       lp_msg->initialize(this, TM_DP2_SQ_XA_END, 
                          RM_MsgSize(lp_msg->Req()->u.iv_end), 
                          RM_MsgSize(lp_msg->Rsp()->u.iv_end));
       lp_msg->Req()->u.iv_end.iv_xid = *pp_xid;
       lp_msg->Req()->u.iv_end.iv_rmid = getRmid();
       lp_msg->Req()->u.iv_end.iv_flags = pv_flags;
       lp_msg->Req()->u.iv_start.iv_nid = gv_xaTM.my_nid();
       lp_msg->Req()->u.iv_start.iv_pid = gv_xaTM.my_pid();

       lv_xaError = send_rm(lp_msg);
   }

   // release the message object back to the pool if there was an error
   if (lp_msg)
   {
       if (lv_xaError)
          gv_xaTM.release_RMmsg(lp_msg);
       else
          gv_xaTM.insert_txnMsgList(pp_xid, lp_msg);
   }
   unlock();

   if (lp_msg && (pv_flags & TMASYNC) && (lv_xaError == XA_OK))
      return lp_msg->msgid();
   else
      return lv_xaError;
} // CxaTM_RM::end

//-------------------------------------------------------------------
// CxaTM_RM::forget
// Send an xa_forget message to RM.
//-------------------------------------------------------------------
int CxaTM_RM::forget(XID *pp_xid, int64 pv_flags)
{
   int lv_xaError = XA_OK;
   
   lock();
   CxaTM_RMMessage * lp_msg = gv_xaTM.new_RMmsg();

   if (lp_msg == NULL)
   {
       lv_xaError = XAER_RMFAIL;
   }
   else
   {
       lp_msg->initialize(this, TM_DP2_SQ_XA_FORGET, 
                          RM_MsgSize(lp_msg->Req()->u.iv_forget), 
                          RM_MsgSize(lp_msg->Rsp()->u.iv_forget));
       lp_msg->Req()->u.iv_forget.iv_xid = *pp_xid;
       lp_msg->Req()->u.iv_forget.iv_rmid = getRmid();
       lp_msg->Req()->u.iv_forget.iv_flags = pv_flags;
       lp_msg->Req()->u.iv_start.iv_nid = gv_xaTM.my_nid();
       lp_msg->Req()->u.iv_start.iv_pid = gv_xaTM.my_pid();

       lv_xaError = send_rm(lp_msg);
   }

   // release the message object back to the pool if there was an error
   if (lp_msg)
   {
       if (lv_xaError)
          gv_xaTM.release_RMmsg(lp_msg);
       else
          gv_xaTM.insert_txnMsgList(pp_xid, lp_msg);
   }
   unlock();

   if (lp_msg && (pv_flags & TMASYNC) && (lv_xaError == XA_OK))
      return lp_msg->msgid();
   else
      return lv_xaError;
} // CxaTM_RM::forget

//-------------------------------------------------------------------
// CxaTM_RM::open
// Open the RM.
// Send xa_open message to RM.
//-------------------------------------------------------------------
int CxaTM_RM::open(char *pp_info, int64 pv_flags)
{
   int lv_xaError = XA_OK;
   xarmFlags(pv_flags);
   char lv_xarmName[RMNAMESZ];
   RM_Open_struct *lp_info = NULL;
   CxaTM_RMMessage *lp_msg = NULL;

   lock();

   if (tm_XARM_generic_library())
   {
      // for standard XARM, this will contain the RM name in the form:
      // "RM=<rm name>"  We want to extract the <rm name> and save it.
      lv_xaError = validateRMname(pp_info, (char *) &lv_xarmName);
      xarmInfo(pp_info);
   
      // For now we always go to the TM in our node.
      sprintf (iv_tmName, "$tm%d", gv_xaTM.setAndGetNid());

      lv_xaError = setRmPhandle(iv_tmName);
      XATrace(XATM_TraceExit,("XATM: CxaTM_RM::open XARM xa_open ENTRY. "
            "RM type %s, rmid %d (%s), associated TM %s, XA error %d.\n",
            xarmName(), getRmid(), xarmInfo(), iv_tmName, lv_xaError));
      char lv_generic_name[MAXPROCESSNAME + RMNAMESZ + MAXINFOSIZE + 10];
      sprintf((char *) &lv_generic_name, "TM_%s:RM_%s:%s", iv_tmName, xarmName(), xarmInfo());
       tm_log_event(DTM_XATM_OPEN_GENERIC, SQ_LOG_INFO, "DTM_XATM_OPEN_GENERIC", 
          -1,getRmid(),gv_xaTM.setAndGetNid(),-1,-1,lv_xaError,-1,-1,-1,-1,
          -1,-1,-1,-1,-1,-1,(char *) &lv_generic_name);
   }
   else
   {
      lp_info = (RM_Open_struct *) pp_info;
      lv_xaError = setRmPhandle((char *) &lp_info->process_name);
      xarmName(tm_switch->name); // For DTM - T SE XA interface we use the generic RM name for now TODO
      xarmInfo(pp_info);
      XATrace(XATM_TraceError,("XATM: CxaTM_RM::open TSE ENTRY nid %d, rmid %d (%s) XA error %d.\n",
            gv_xaTM.setAndGetNid(), getRmid(), getRMnameDetail(), lv_xaError));
      tm_log_event(DTM_XATM_OPEN_TSE, SQ_LOG_INFO, "DTM_XATM_OPEN_TSE", 
          -1,getRmid(),gv_xaTM.setAndGetNid(),-1,-1,lv_xaError,-1,-1,-1,-1,
          -1,-1,-1,-1,-1,-1,getRMnameDetail());
   }
   if (lv_xaError == XA_OK)
   {
      // Send xa_open message
      lp_msg = gv_xaTM.new_RMmsg();

      if (lp_msg == NULL)
      {
         lv_xaError = XAER_RMFAIL;
      }
      else
      {
         lp_msg->initialize(this, TM_DP2_SQ_XA_OPEN, 
                           RM_MsgSize(lp_msg->Req()->u.iv_open), 
                           RM_MsgSize(lp_msg->Rsp()->u.iv_open));
         lp_msg->Req()->u.iv_open.iv_tm_nid = gv_xaTM.setAndGetNid();
         lp_msg->Req()->u.iv_open.iv_incarnation_num = (lp_info)?lp_info->incarnation_num:0;
         lp_msg->Req()->u.iv_open.iv_seq_num_minimum = (lp_info)?lp_info->seq_num_block_start:0;
         lp_msg->Req()->u.iv_open.iv_rmid = getRmid();
         strcpy(lp_msg->Req()->u.iv_open.iv_info, pp_info);
         lp_msg->Req()->u.iv_start.iv_nid = gv_xaTM.my_nid();
         lp_msg->Req()->u.iv_start.iv_pid = gv_xaTM.my_pid();
         lp_msg->Rsp()->u.iv_open.iv_ax_reg = 1;

         lv_xaError = send_rm(lp_msg);
      }

      // release the message object back to the pool if there was an error
      if (lv_xaError && lp_msg)
         gv_xaTM.release_RMmsg(lp_msg);
   }

   unlock();

   XATrace((lv_xaError?XATM_TraceExitError:XATM_TraceExit),
           ("XATM: CxaTM_RM::open EXIT returning %s, nid(%d), rmid(%d).\n", 
            XAtoa(lv_xaError), gv_xaTM.setAndGetNid(),
            getRmid()));

   if (lp_msg && (pv_flags & TMASYNC) && (lv_xaError == XA_OK))
      return lp_msg->msgid();
   else
      return lv_xaError;
} //CxaTM_RM::open


//-------------------------------------------------------------------
// CxaTM_RM::prepare
// Send xa_prepare message to RM.
//-------------------------------------------------------------------
int CxaTM_RM::prepare(XID *pp_xid, int64 pv_flags)
{
   int lv_xaError = XA_OK;

   lock();
   CxaTM_RMMessage * lp_msg = gv_xaTM.new_RMmsg();

   if (lp_msg == NULL)
   {
       lv_xaError = XAER_RMFAIL;
   }
   else
   {
       lp_msg->initialize(this, TM_DP2_SQ_XA_PREPARE, 
                          RM_MsgSize(lp_msg->Req()->u.iv_prepare), 
                          RM_MsgSize(lp_msg->Rsp()->u.iv_prepare));
       lp_msg->Req()->u.iv_prepare.iv_xid = *pp_xid;
       lp_msg->Req()->u.iv_prepare.iv_rmid = getRmid();
       lp_msg->Req()->u.iv_prepare.iv_flags = pv_flags;
       lp_msg->Req()->u.iv_start.iv_nid = gv_xaTM.my_nid();
       lp_msg->Req()->u.iv_start.iv_pid = gv_xaTM.my_pid();
       lv_xaError = send_rm(lp_msg);
   }

   // release the message object back to the pool if there was an error
   if (lp_msg)
   {
       if (lv_xaError)
          gv_xaTM.release_RMmsg(lp_msg);
       else
          gv_xaTM.insert_txnMsgList(pp_xid, lp_msg);
   }
   unlock();

   if (lp_msg && (pv_flags & TMASYNC) && (lv_xaError == XA_OK))
      return lp_msg->msgid();
   else
      return lv_xaError;
} //CxaTM_RM::prepare

//-------------------------------------------------------------------
// CxaTM_RM::start
// Send xa_start message to RM.
//-------------------------------------------------------------------
int CxaTM_RM::start(XID *pp_xid, int64 pv_flags)
{
   int lv_xaError = XA_OK;

   lock();
   CxaTM_RMMessage * lp_msg = gv_xaTM.new_RMmsg();

   if (lp_msg == NULL)
   {
       lv_xaError = XAER_RMFAIL;
   }
   else
   {
       lp_msg->initialize(this, TM_DP2_SQ_XA_START, 
                          RM_MsgSize(lp_msg->Req()->u.iv_start), 
                          RM_MsgSize(lp_msg->Rsp()->u.iv_start));
       lp_msg->Req()->u.iv_start.iv_xid = *pp_xid;
       lp_msg->Req()->u.iv_start.iv_rmid = getRmid();
       lp_msg->Req()->u.iv_start.iv_flags = pv_flags;
       lp_msg->Req()->u.iv_start.iv_nid = gv_xaTM.my_nid();
       lp_msg->Req()->u.iv_start.iv_pid = gv_xaTM.my_pid();
       lv_xaError = send_rm(lp_msg);
   }

   // release the message object back to the pool if there was an error
   if (lp_msg)
   {
       if (lv_xaError)
          gv_xaTM.release_RMmsg(lp_msg);
       else
          gv_xaTM.insert_txnMsgList(pp_xid, lp_msg);
   }
   unlock();

   if (lp_msg && (pv_flags & TMASYNC) && (lv_xaError == XA_OK))
      return lp_msg->msgid();
   else
      return lv_xaError;
} //CxaTM_RM::start

//-------------------------------------------------------------------
// CxaTM_RM::rollback
// Send xa_rollback message to RM.
//-------------------------------------------------------------------
int CxaTM_RM::rollback(XID *pp_xid, int64 pv_flags)
{
   int lv_xaError = XA_OK;

   lock();
   CxaTM_RMMessage * lp_msg = gv_xaTM.new_RMmsg();

   if (lp_msg == NULL)
   {
       lv_xaError = XAER_RMFAIL;
   }
   else
   {
       lp_msg->initialize(this, TM_DP2_SQ_XA_ROLLBACK, 
                          RM_MsgSize(lp_msg->Req()->u.iv_rollback), 
                          RM_MsgSize(lp_msg->Rsp()->u.iv_rollback));
       lp_msg->Req()->u.iv_rollback.iv_xid = *pp_xid;
       lp_msg->Req()->u.iv_rollback.iv_rmid = getRmid();
       lp_msg->Req()->u.iv_rollback.iv_flags = pv_flags;
       lp_msg->Req()->u.iv_start.iv_nid = gv_xaTM.my_nid();
       lp_msg->Req()->u.iv_start.iv_pid = gv_xaTM.my_pid();
       lv_xaError = send_rm(lp_msg);
   }

   // release the message object back to the pool if there was an error
   if (lp_msg)
   {
       if (lv_xaError)
          gv_xaTM.release_RMmsg(lp_msg);
       else
          gv_xaTM.insert_txnMsgList(pp_xid, lp_msg);
   }
   unlock();

   if (lp_msg && (pv_flags & TMASYNC) && (lv_xaError == XA_OK))
      return lp_msg->msgid();
   else
      return lv_xaError;
} //CxaTM_RM::rollback



