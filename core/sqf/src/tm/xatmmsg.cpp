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
#include "seabed/trace.h"
#include "seabed/thread.h"

#include "dtm/tm_util.h"

#include "dtm/xa.h"
#include "rm.h"
#include "tmrm.h"
#include "tmmap.h"
#include "tmdeque.h"
#include "tmlogging.h"
#include "xatmmsg.h"
#include "xatmlib.h"
#include "xatmapi.h"
#include "tminfo.h"


//------------------------------------------------------------------------
// Standard constructor
//------------------------------------------------------------------------
CxaTM_RMMessage::CxaTM_RMMessage(int64 pv_msgNum) 
    :iv_msgNum(pv_msgNum)
{
   // Mutex attributes: Recursive = true, ErrorCheck=false
   ip_mutex = new TM_Mutex(true, false);

   lock();
   ip_req = new RM_Req_Msg_Type();
   ip_rsp = new RM_Rsp_Msg_Type();
   //EID(EID_CxaTM_RMMessage);
   clean();
   unlock();
}

//------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------
CxaTM_RMMessage::~CxaTM_RMMessage()
{
   lock();
   delete ip_req;
   delete ip_rsp;
   unlock();

   delete ip_mutex;
}
   

//----------------------------------------------------------------------------
// CxaTM_RMMessage::constructPoolElement
// Purpose : Callback for CTmPool elements.
// This method is called to construct a CxaTM_RMMessage object by CTmPool::newElement.
//----------------------------------------------------------------------------
CxaTM_RMMessage * CxaTM_RMMessage::constructPoolElement(int64 pv_msgNum)
{

   XATrace (XATM_TraceRMMsg, ("CxaTM_RMMessage::constructPoolElement : ENTRY Instantiating new RMMessage object, msgNum " PFLL ".\n", 
                pv_msgNum));

   CxaTM_RMMessage *lp_msg = new CxaTM_RMMessage(pv_msgNum);
   if (!lp_msg)
   {
      tm_log_event(DTM_LOGIC_ERROR_RM_OBJ, SQ_LOG_CRIT, "DTM_LOGIC_ERROR_RM_OBJ");
      XATrace (XATM_TraceError, ("CxaTM_RMMessage::constructPoolElement : Failed to instantiate "
                                 " RMMessage object.\n"));
      abort();
   }

   XATrace (XATM_TraceRMMsg, ("CxaTM_RMMessage::constructPoolElement : EXIT RMMessage object %p, msgNum " PFLL " instantiated.\n", 
                (void *) lp_msg, pv_msgNum));
   return lp_msg;
} //CxaTM_RMMessage::constructPoolElement


//----------------------------------------------------------------------------
// CxaTM_RMMessage::cleanPoolElement
// Purpose : Callback for CTmPool elements.
// This method is called to clean a CxaTM_RMMessage object when CTmPool::newElement
// allocated it from the freeList.
// Returns the msgNum which is reused to add the thread object to the 
// CTmPool<CxaTM_RMMessage> inUseList.
//----------------------------------------------------------------------------
int64 CxaTM_RMMessage::cleanPoolElement()
{
   lock();
   clean();
   unlock();
   return msgNum();
} //CxaTM_RMMessage::cleanPoolElement


//------------------------------------------------------------------------
// clean
// Purpose : Clean the message object to make it ready for reuse.
// Caller must lock.
//------------------------------------------------------------------------
void CxaTM_RMMessage::clean()
{
   iv_msgid = 0;
   threadId(-1);
   iv_sendAttempts = 0;
   iv_sleepTime = XATM_MSG_INIT_SLEEPTIME;
   memset(&iv_xid, 0, sizeof(iv_xid));
   memset(ip_req, 0, sizeof(RM_Req_Msg_Type));
   memset(ip_rsp, 0, sizeof(RM_Rsp_Msg_Type));
} //CxaTM_RMMessage::clean


//------------------------------------------------------------------------
// initialize
// Purpose : initialize or reinitialize the message object.
//------------------------------------------------------------------------
void CxaTM_RMMessage::initialize(CxaTM_RM *pp_RM, 
                                 TM_DP2_SQ_MSG_TYPE pv_req_type, 
                                 int32 pv_reqLen, int32 pv_rspLen)
{
   lock();
   clean();
   threadId(SB_Thread::Sthr::self_id());
   ip_RM = pp_RM;
   iv_reqLen = pv_reqLen;
   iv_rspLen = pv_rspLen;

   ip_req->iv_msg_hdr.dialect_type = DIALECT_TM_DP2_SQ;
   ip_req->iv_msg_hdr.rr_type.request_type = (short) pv_req_type;
   ip_req->iv_msg_hdr.version.request_version = TM_SQ_MSG_VERSION_CURRENT;
   ip_req->iv_msg_hdr.miv_err.minimum_interpretation_version = TM_SQ_MSG_VERSION_CURRENT;
   unlock();
}

//---------------------------------------------------------------------
// getRmReplyOpenAx_Reg
// Get the ax_reg allowed flag from the last reply returned by the RM.
// --------------------------------------------------------------------
bool CxaTM_RMMessage::getRmReplyOpenAx_Reg()
{
   bool lv_return = false;
   lock();
   if (getRmReplyType() == TM_DP2_SQ_XA_OPEN_REPLY)
      lv_return = ip_rsp->u.iv_open.iv_ax_reg;
   else
   {
      XATrace(XATM_TraceError,
              ("XATM: CxaTM_RMMessage::getRmReplyOpenAx_Reg rmid=%d. Invalid reply type %d "
               "to return xa_open reply values.\n",
               ip_RM->getRmid(), getRmReplyType()));
   }
   unlock();
   return lv_return;
}

// --------------------------------------------------------------------
// CxaTM_RMMessage::getRmReplyOpenOpener
// Get the Opener from the last reply returned by the RM.
// --------------------------------------------------------------------
int32 CxaTM_RMMessage::getRmReplyOpenOpener()
{
   bool lv_return = 0;
   lock();
   if (getRmReplyType() == TM_DP2_SQ_XA_OPEN_REPLY)
      lv_return = ip_rsp->u.iv_open.iv_opener;
   else
   {
      XATrace(XATM_TraceError,
              ("XATM: CxaTM_RMMessage::getRmReplyOpenOpener rmid=%d. Invalid reply type %d "
               "to return xa_open reply values.\n",
               ip_RM->getRmid(), getRmReplyType()));
   }
   unlock();
   return lv_return;
}

// --------------------------------------------------------------------
// CxaTM_RMMessage::getRmReplyPreparePartic
// Get the prepare partic flag from the last reply returned by the RM.
// --------------------------------------------------------------------
bool CxaTM_RMMessage::getRmReplyPreparePartic()
{
   bool lv_return = false;
   lock();
   if (getRmReplyType() == TM_DP2_SQ_XA_PREPARE_REPLY)
      lv_return = ip_rsp->u.iv_prepare.iv_partic;
   else
   {
      XATrace(XATM_TraceError,
              ("XATM: CxaTM_RMMessage::getRmReplyPreparePartic rmid=%d. Invalid reply type %d "
               "to return xa_prepare reply values.\n",
               ip_RM->getRmid(), getRmReplyType()));
   }
   unlock();
   return lv_return;
}

// --------------------------------------------------------------------
// CxaTM_RMMessage::getRmReplyType
// Get the type of the last reply returned by the RM.
// --------------------------------------------------------------------
int32 CxaTM_RMMessage::getRmReplyType()
{
   return ip_rsp->iv_msg_hdr.rr_type.reply_type;
}

// --------------------------------------------------------------------
// CxaTM_RMMessage::lock
// Lock the RM message object semaphore
// Now using recursive semaphores
// --------------------------------------------------------------------
void CxaTM_RMMessage::lock()
{
   XATrace(XATM_TraceLock, ("XATM: CxaTM_RMMessage::lock, count %d, owner %ld\n", 
           ip_mutex->lock_count(), ip_mutex->lock_owner()));

   int lv_error = ip_mutex->lock();
   if (lv_error)
   {
      XATrace(XATM_TraceError, ("XATM: CxaTM_RMMessage::lock returned error %d.\n", lv_error));
      abort();
   }
}

// --------------------------------------------------------------------
// CxaTM_RMMessage::unlock
// Unlock the RM message object semaphore
// --------------------------------------------------------------------
void CxaTM_RMMessage::unlock()
{
   XATrace(XATM_TraceLock, ("XATM: CxaTM_RMMessage::unlock, count %d, owner %ld\n", 
           ip_mutex->lock_count(), ip_mutex->lock_owner()));

   int lv_error = ip_mutex->unlock();
   if (lv_error)
   {
      XATrace(XATM_TraceError, ("XATM: CxaTM_RMMessage::unlock returned error %d.\n", lv_error));
      abort();
   }
}



// --------------------------------------------------------------
// CxaTM_RMMessage::send_rm
// Purpose - send the message to the RM
// Caller may lock.
// --------------------------------------------------------------
int CxaTM_RMMessage::send_rm() 
{
   short lv_ret = -1;
   int   lv_xaError = XA_OK;

   XATrace(XATM_TraceExit,
           ("XATM: CxaTM_RMMessage::send_rm ENTRY. msgid=%d, retries=%d/%d.\n", 
            iv_msgid, sendAttempts(), RM()->maxSendRetries()));
   lock();
   // Make sure that this CxaTM_RMMessage object doesn't already have a link outstanding
   if (msgid())
   {
      tm_log_event(DTM_XATM_SENDTORM_FAILED, SQ_LOG_CRIT, "DTM_XATM_SENDTORM_FAILED",
                   -1, RM()->getRmid(), -1, -1, msgid());
      XATrace(XATM_TraceError, ("XATM: CxaTM_RMMessage::send_rm : XATM Message object "
                                "already has a link outstanding, msgid %d\n", msgid()));
      abort();
   }

   if (iv_reqLen > RM_MAX_DATA || iv_rspLen > RM_MAX_DATA)
   {
      // EMS DTM_XATM_SENDTORM_FAILED
      tm_log_event(DTM_XATM_SENDTORM_FAILED2, SQ_LOG_CRIT, "DTM_XATM_SENDTORM_FAILED2",
                   -1,RM()->getRmid(),-1,-1,-1,XAER_RMFAIL,-1,-1,-1,-1,-1,-1,-1,
                   RM_MAX_DATA,iv_reqLen,iv_rspLen);
      XATrace(XATM_TraceError, ("XATM: CxaTM_RMMessage::send_rm : "
                                " Request or reply buffer too large, returning XAER_RMFAIL.\n"));
      lv_xaError = XAER_RMFAIL;
   }
   else
   {
      inc_sendAttempts();
      lv_ret = link();
   }

   if (lv_ret)
   {
      // EMS DTM_XATM_SENDTORM_FAILED
      tm_log_event(DTM_XATM_SENDTORM_FAILED, SQ_LOG_CRIT, "DTM_XATM_SENDTORM_FAILED", lv_ret, ip_RM->getRmid());
      lv_xaError = XAER_RMFAIL;
   }
   unlock();

   XATrace((lv_xaError?XATM_TraceExitError:XATM_TraceExit),
           ("XATM: CxaTM_RMMessage::send_rm EXIT returning %s, BMSG_LINK_ returned %d, msgid=%d, retries=%d/%d.\n", 
            XAtoa(lv_xaError), lv_ret, iv_msgid, sendAttempts(), RM()->maxSendRetries()));
   return lv_xaError;
} //CxaTM_RMMessage::send_rm


// --------------------------------------------------------------
// CxaTM_RMMessage::abandon
// Purpose - Abandon this Seabed request.
// This is very similar to cancel(), except that the RMMessage
// object is not deleted.  This allows it to be reused.
// --------------------------------------------------------------
short CxaTM_RMMessage::abandon() 
{
   XATrace(XATM_TraceExit,
           ("XATM: CxaTM_RMMessage::cancel ENTRY msgid=%d.\n", iv_msgid));
   lock();
   short lv_ret = BMSG_ABANDON_(iv_msgid);

   // Check the RM objects message list and remove this message if it's still there
   ip_RM->msgList()->remove(iv_msgid);

   unlock();
   XATrace((lv_ret?XATM_TraceExitError:XATM_TraceExit),
           ("XATM: CxaTM_RMMessage::abandon EXIT BMSG_ABANDON_ returned %d, msgid=%d.\n",
            lv_ret, iv_msgid));
   return lv_ret;
} //CxaTM_RMMessage::abandon


// --------------------------------------------------------------
// CxaTM_RMMessage::cancel
// Purpose - Cancel this Seabed request.  This removes the message
// from the message list and returns the RMMessage object to the pool.
// Set pv_releaseMsg to true if you want to release the msg object
// from the RMmsg list. Default is false
// --------------------------------------------------------------
short CxaTM_RMMessage::cancel(bool pv_releaseMsg) 
{
   XATrace(XATM_TraceExit,
           ("XATM: CxaTM_RMMessage::cancel ENTRY msgid=%d.\n", iv_msgid));
   lock();
   short lv_ret = BMSG_ABANDON_(iv_msgid);

   // Release the message from msgList if it's present
   if (pv_releaseMsg && ip_RM->msgList()->get(iv_msgid) != NULL)
      gv_xaTM.release_RMmsg(this);

   unlock();
   XATrace((lv_ret?XATM_TraceExitError:XATM_TraceExit),
           ("XATM: CxaTM_RMMessage::cancel EXIT BMSG_ABANDON_ returned %d, msgid=%d.\n",
            lv_ret, iv_msgid));
   return lv_ret;
} //CxaTM_RMMessage::cancel


// --------------------------------------------------------------
// CxaTM_RMMessage::retrySend
// Purpose - Retries a send to an RM.  Note that we delay before
// each retry at the end of complete_all.
// Returns:
//    true - retry will be successful
//    false - retry will be unsuccessful. This could be because 
//            we got an error on the send or we've exceeded the retries.
// pp_xaError output:
//    XA_OK if the retry was successful, or 
//    XAER_RMFAIL if retries exceeded.
//    XAER_*   Any error returned by RM()->send_rm().
// --------------------------------------------------------------
bool CxaTM_RMMessage::retrySend() 
{
   bool lv_success = false;
   int lv_xaError = XA_OK;

   lock();
   if (sendAttempts() < RM()->maxSendRetries())
   {
      // For the first retry, do not sleep
      if (sendAttempts() > 1)
       SB_Thread::Sthr::sleep(iv_sleepTime);
      //iv_sleepTime *= 2; linear for now
      lv_xaError = RM()->send_rm(this);
      if (lv_xaError == XA_OK)
         lv_success = true;
   }

   unlock();
   return lv_success;
} //CxaTM_RMMessage::retrySend

// --------------------------------------------------------------
// CxaTM_RMMessage::release_from_RMmsgList
// Purpose: Release this meesage from the RMs msgList.
// --------------------------------------------------------------
void CxaTM_RMMessage::release_from_RMmsgList()
{
    // Remove the message from the RMs message list but reuse it for the retry
    RM()->msgList()->remove(msgid());
    msgid(0);
} //CxaTM_RMMessage::release_from_RMmsgList


// --------------------------------------------------------------
// CxaTM_RMMessage::can_we_retrySend
// Purpose - Determines whether we can retry the send to an RM.  
// Returns:
//    true  - retry the send to an RM
//    false - do not retry the send. This could be because we got an 
//            error on the send or we've exceeded the retries.
// pp_xaError output:
//    XA_OK if the retry was successful, or 
//    XAER_RMFAIL if retries exceeded.
//    XAER_*   Any error returned by RM()->send_rm().
// --------------------------------------------------------------
bool CxaTM_RMMessage::can_we_retrySend(int *pp_xaError) 
{
   bool lv_success = false;

   XATrace(XATM_TraceExit,
           ("XATM: CxaTM_RMMessage::can_we_retrySend ENTRY RM %d, XA error=%d.\n",
            RM()->getRmid(), *pp_xaError));


   lock();
   if (sendAttempts() >= RM()->maxSendRetries())
   {
      XATrace(XATM_TraceError,("XATM: CxaTM_RMMessage::can_we_retrySend rmid=%d, "
              "msgid=%d, sendAttempts=%d. Retries exceeded.\n",
              RM()->getRmid(), msgid(), sendAttempts()));
      *pp_xaError = XAER_RMFAIL;
   }
   else
      if (*pp_xaError == XA_RETRY)
         lv_success = true;

   unlock();

   XATrace(XATM_TraceExit,
           ("XATM: CxaTM_RMMessage::can_we_retrySend EXIT RM %d, XA error=%d, returning %d.\n",
            RM()->getRmid(), *pp_xaError, lv_success));
   return lv_success;
} //CxaTM_RMMessage::can_we_retrySend


// --------------------------------------------------------------
// CxaTM_RM::checkError
// Check the error returned by BMSG_BREAK_ and determine whether
// to retry the LINK.
// If break returns an error we retry the link - resend the 
// message to the RM.
// We don't need to cancel/abandon the message. If Seabed 
// returned an error it already abandoned it.
// pv_breakError - return code from BMSG_BREAK_.
// pp_xaError will contain the XA error code on return.
// pv_softRetry - true (default) Will retry true (retry link) if an
//                     XA_RETRY occurs.  This happens if a 
//                     FEPATHDOWN, FENOLCB, or an XA_RETRY occur.
//                false Will return retry = false if one of these
//                     errors occurs. This allows the XA_RETRY to
//                     be passed up to the next level for xa_recover.
// pv_transid only used for event messages (error reporting).
// return code
//    true = retry link.
//    false = Don't retry. Retries exceeded or error is not 
//            retryable.
// --------------------------------------------------------------
bool CxaTM_RMMessage::checkError(short pv_breakError, int *pp_xaError, 
                                 bool pv_softRetry, int64 pv_transid)
{
   bool lv_retryLink = false;
   *pp_xaError = XA_OK;
   char la_buf[DTM_STRING_BUF_SIZE];
   CTmTxKey lv_txn = (CTmTxKey) pv_transid;

   XATrace(XATM_TraceExit,
           ("XATM: CxaTM_RMMessage::checkError ENTRY txn ID (%d,%d) msgid=%d, breakError=%d, "
            "rmid=%d, softRetry=%d.\n", lv_txn.node(), lv_txn.seqnum(), iv_msgid, pv_breakError, 
            RM()->getRmid(), pv_softRetry));

   lock();
   // For XARM clients connecting to a DTM TM, we always retry the error.
   // DTM TMs don't run as process pairs, but we are attempting to send to
   // a specific node which might go down and switch to a spare. Also the
   // TSE might switch to it's backup.
   // If it's a retryable error (FEPATHDOWN (201) or FEOWNERSHIP (200)) or
   // FENOLCB (30) from a TSE then we'll retry. 
   if ((tm_XARM_generic_library() && pv_breakError) || 
       pv_breakError == FEPATHDOWN ||
       pv_breakError == FEOWNERSHIP ||
      pv_breakError == FENOLCB )
   {
      // Retry sending the request to the RM.  This is used to deal with failovers.
      if (ip_RM->inc_totalRetries())
      {
         sprintf(la_buf, "Retrying send to RM. BMSG_BREAK_ failed for Txn ID (%d,%d) with "
                 "error %d, rmid=%d, msgid=%d, retries=%d/%d\n", 
                 lv_txn.node(), lv_txn.seqnum(), pv_breakError, RM()->getRmid(), msgid(), 
                 sendAttempts(), RM()->maxSendRetries());

         tm_log_event(DTM_TM_INFO_MSGBRK_FAIL3, SQ_LOG_WARNING,
                      "DTM_TM_INFO_MSGBRK_FAIL3", pv_breakError, RM()->getRmid(),lv_txn.node(),lv_txn.seqnum(),
                      msgid(),-1,-1,-1, sendAttempts(),-1,-1,-1,-1,-1,-1,-1,RM()->getRMnameDetail());
         XATrace(XATM_TraceError,("XATM: CxaTM_RMMessage::checkError : %s", la_buf));
      }
      if ((pv_breakError == FEPATHDOWN) || (pv_breakError == FENOLCB))          
         *pp_xaError = XA_RETRY;

      lv_retryLink = can_we_retrySend(pp_xaError);
   }
   else
   {
      if (pv_breakError == FEOK)
      {
         *pp_xaError = getRmError();
         if (*pp_xaError != XA_OK)
         {
            XATrace(XATM_TraceDetail,("XATM: CxaTM_RMMessage::checkError : "
                    "XA error %s encountered.\n", XAtoa(*pp_xaError)));
            // If the RM responds XA_RETRY, we retry as for a break error, but allow
            // this to retry forever.
            if (*pp_xaError == XA_RETRY)
            {
               lv_retryLink = can_we_retrySend(pp_xaError);
               clear_sendAttempts();
            }
         }
      }
      else
      {
         sprintf(la_buf, "RM connection error. txn ID (%d,%d) BMSG_BREAK_ failed with "
                 "error %d, rmid=%d, msgid=%d, retries=%d/%d\n", 
                 lv_txn.node(), lv_txn.seqnum(), pv_breakError, RM()->getRmid(), msgid(), 
                 sendAttempts(), RM()->maxSendRetries());

         tm_log_event(DTM_TM_INFO_MSGBRK_FAIL4, SQ_LOG_WARNING,
                      "DTM_TM_INFO_MSGBRK_FAIL4",pv_breakError, RM()->getRmid(), 
                      lv_txn.node(),lv_txn.seqnum(),msgid(),-1,-1,-1, sendAttempts(),
                      -1,-1,-1,-1,-1,-1,-1,RM()->getRMnameDetail());
         XATrace(XATM_TraceError,("XATM: CxaTM_RMMessage::checkError : %s", la_buf));
         *pp_xaError = XAER_RMFAIL;
      }
   }
   unlock();

   if (!pv_softRetry && *pp_xaError == XA_RETRY)
      lv_retryLink = false;

   XATrace(XATM_TraceExit,
           ("XATM: CxaTM_RMMessage::checkError EXIT txn ID (%d,%d) msgid=%d, returning "
            "retryLink=%d, xaError=%s.\n",
            lv_txn.node(), lv_txn.seqnum(), msgid(), lv_retryLink, XAtoa(*pp_xaError)));
   return lv_retryLink;
} //CxaTM_RMMessage::checkError


// ---------------------------------------------------------------------------
// CxaTM_RMMessage::link
// Purpose : Call to BMSG_LINK_.
// This function will retry any retriable errors such as FENOLCB (30).
// Parameters:
//  pv_maxretries - maximum retries for retriable errors.
//  Returns error from BMSG_LINK_ call.
// ---------------------------------------------------------------------------
short CxaTM_RMMessage::link(int32 pv_maxretries)

{
    short lv_ret = 0;
    int32 lv_retries = 0;
    bool lv_exit = false;
    TM_Txid_Internal *lp_transid = (TM_Txid_Internal *) &xid()->data[0];

    XATrace(XATM_TraceExit, ("CxaTM_RMMessage::link ENTRY : ID (%d,%d), linker "
            "tag(rmid) %d.\n", lp_transid->iv_node, lp_transid->iv_seq_num, 
            ip_RM->getRmid()));

    do {
         lv_ret = BMSG_LINK_(ip_RM->getRmPhandle(),     // phandle, 
                           &iv_msgid,                   // msgid
                           NULL,                        // reqctrl
                           0,                           // reqctrlsize
                           NULL,                        // replyctrl
                           0,                           // replyctrlmax
                           (char *) Req(),              // reqdata
                           ReqLen(),                    // reqdatasize
                           (char *) Rsp(),              // replydata
                           RspLen(),                    // replydatamax
                           ip_RM->getRmid(),            // linkertag
                           TSE_LINK_PRIORITY,           // pri
                           0,                           // xmitclass
                           BMSG_LINK_LDONEQ);           // linkopts

       lv_retries++;
       if (lv_ret == FENOLCB && 
          //((pv_maxretries == -1 && (lv_retries % TM_LINKRETRY_RETRIES == 0)) ||
          (pv_maxretries == -1 ||
           (pv_maxretries > 0 && (lv_retries <= pv_maxretries))))
       {  // Message Descriptor depletion.  This means we ran out of MDs.
          // This is retriable, and we want to slow down the TM to allow
          // some of the outstanding requests to complete.
          XATrace(XATM_TraceError, ("CxaTM_RMMessage::link BMSG_LINK_ error %d, "
                       "linker tag(rmid) %d, retires %d/%d - Pausing thread for "
                       "%dms before retrying.\n", lv_ret, ip_RM->getRmid(), 
                       lv_retries, pv_maxretries, TM_LINKRETRY_PAUSE));
          tm_log_event(DTM_TM_LINK_PAUSED, SQ_LOG_WARNING, "DTM_TM_LINK_PAUSED", 
                   lv_ret, -1, lp_transid->iv_node, lp_transid->iv_seq_num, -1, -1, -1, -1, lv_retries, 
                   -1, -1, -1, -1, TM_LINKRETRY_PAUSE /*pause in ms*/, ip_RM->getRmid());
          SB_Thread::Sthr::sleep(TM_LINKRETRY_PAUSE); // in msec
       }
       if (lv_ret != FENOLCB)
          lv_exit = true;
       else
          if (pv_maxretries > 0 && lv_retries >= pv_maxretries)
             lv_exit = true;

    } while (!lv_exit);

    if (lv_ret)
    {
       XATrace(XATM_TraceExit, ("CxaTM_RMMessage::link EXIT : returning error %d.\n", lv_ret));
    }
    else
    {
       XATrace(XATM_TraceExit, ("CxaTM_RMMessage::link EXIT : returning msgid %d.\n", iv_msgid));
    }
    return lv_ret;
} //CxaTM_RMMessage::link

// DEBUG_MODE versions of functions
#ifdef DEBUG_MODE

void CxaTM_RMMessage::inc_sendAttempts()
{
   iv_sendAttempts++;
   XATrace(XATM_TraceExit, ("CxaTM_RMMessage::inc_sendAttempts EXIT : new sendAttempts=%d\n", iv_sendAttempts));
}

void CxaTM_RMMessage::clear_sendAttempts()
{
   XATrace(XATM_TraceExit, ("CxaTM_RMMessage::clear_sendAttempts ENTRY : sendAttempts=%d\n", iv_sendAttempts));
   iv_sendAttempts = 0;
}
#endif
