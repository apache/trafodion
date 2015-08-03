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

// seabed includes
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/trace.h"
#include "common/sq_common.h"

#include "rm.h"
#include "tmrmtse.h"
#include "tmmap.h"
#include "tmdeque.h"
#include "tmlogging.h"
#include "dtm/xa.h"
#include "dtm/xarm.h"
#include "xatmmsg.h"
#include "xatmlib.h"
#include "tmlibmsg.h"
#include "tmregistry.h"

// Globals
CxaTM_TM gv_xaTM;      // One global xaTM object

#define TSE_RM_NAME "SEAQUEST_RM_TSE"
#define XARM_RM_NAME "SEAQUEST_RM_XARM"

// TSE xa_switch structure
xa_switch_t tse_switch = {
  TSE_RM_NAME,
  TMREGISTER, // Supports association migration, and
              // dynamic registration, but not
              // asynchronous operations.
  1,          // Version 1: Don't send xa_start and xa_end
  tm_xa_open,
  tm_xa_close,
  tm_xa_start,
  tm_xa_end,
  tm_xa_rollback,
  tm_xa_prepare,
  tm_xa_commit,
  tm_xa_recover,
  tm_xa_forget,
  tm_xa_complete
};

xa_switch_t *tm_switch = &tse_switch;

extern int xaTM_initialize (bool pp_tracing, bool pv_tm_stats, CTmTimer *pp_tmTimer);


// -------------------------------------------------------------------
// tm_xa_rmType_TSE
// Purpose - Identify TSE RMs so that a different protocol can be 
// applied.
// -------------------------------------------------------------------
bool tm_xa_rmType_TSE(int pv_rmid)
{
   CxaTM_RM *lp_RM = gv_xaTM.getRM(pv_rmid);

   // Return an error if the RM wasn't found
   if (lp_RM)
   {
      if (strcmp(TSE_RM_NAME, lp_RM->getRMname()) == 0)
         return true;
   }
   else
   {
       XATrace(XATM_TraceError, ("XATM: tm_xa_rmType_TSE : Couldn't find rm object for rmid=%d\n",
              pv_rmid));
   }
   return false;
} //tm_xa_rmType_TSE


// -------------------------------------------------------------------
// tm_XARM_generic_library
// Purpose - True is returned if the library is working as a standard
// X/Open XARM Library.
// -------------------------------------------------------------------
bool tm_XARM_generic_library()
{
   if (strcmp(tm_switch->name, XARM_RM_NAME) == 0)
      return true;

   return false;
} //tm_XARM_generic_library


// ---------------------------------------------------------------------
// tm_XARM_amIaTM
// Check to see whether the library is running inside a
// TM.  If it is, then it is assumed to be connecting to
// a TSE as a subordinate RM.
// todo: This needs to be extended when we support 
// subordinate foreign TM/RMs.
// ---------------------------------------------------------------------
bool tm_XARM_amIaTM()
{
   int lv_process_type;
   msg_mon_get_my_info2(NULL,       // mon node-id
                        NULL,       // mon process-id
                        NULL,       // mon name
                        NULL,       // mon name-len
                        &lv_process_type, // mon process-type
                        NULL,       // mon zone-id
                        NULL,       // os process-id
                        NULL,       // os thread-id
                        NULL);      // component-id
   if (lv_process_type == MS_ProcessType_DTM)
      return true;
   else
      return false;
} // tm_XARM_amIaTM


// ---------------------------------------------------------------------
// complete_all
// Purpose - this will loop on LDONE waiting for all requests 
//           identified by input params to complete.  It wll also
//           gather pertinent return data into the output params for 
//           the TM to walk through.
// pv_transid Input: defaults to 0.  Used to verify replies match the
// correct transid.
// Returns the number of outstanding requests which should be 0 if
// all RMs responded.
// ---------------------------------------------------------------------
int32 complete_all(int32 pv_num, 
                   TM_RM_Responses *pa_responses,
                   int pv_rm_wait_time,
                   int64 pv_transid)
{ 
   int32          lv_count = 0;
   char           la_buf[DTM_STRING_BUF_SIZE];
   short          la_results[6];
   short          lv_ret;
   short          lv_ret2;
   BMS_SRE_LDONE  lv_sre;
   int            lv_xaError = XA_OK;
   bool           lv_retryLink = false;
   short          lv_event = 0;
   bool           lv_delete_RM = false;

   if (pv_num >= MAX_OPEN_RMS)
   {
       tm_log_event(DTM_XATM_CNT_EXCD_LMT, SQ_LOG_CRIT, "DTM_XATM_CNT_EXCD_LMT", 
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
                     pv_num, MAX_OPEN_RMS);
       abort();
   }

   XATrace(XATM_TraceXAAPI,("XATM: complete_all ENTRY, sends to complete=%d, RM waitTimeout=%d, transid=" 
           PFLLX ".\n", 
           pv_num, pv_rm_wait_time, pv_transid));

   for (int32 i = 0; i < pv_num; i++)
   {
        pa_responses[i].iv_rmid = 0;
        pa_responses[i].iv_error = XA_OK;
        pa_responses[i].iv_partic = false;
        pa_responses[i].iv_ax_reg = false;
        pa_responses[i].iv_drive_recover = false;
   }

   //we exit out of this while using breaks
   while (true)
   {
      // we've reached our message reply count, break
      if (lv_count >= pv_num)
         break;
 
      // wait for an LDONE wakeup 
      lv_event = XWAIT(LDONE, pv_rm_wait_time);

      // Timeout break and exit.
      if (lv_event == 0)
      {
         // EMS DTM_XATM_COMPLETEALL_FAILED
         sprintf(la_buf, "Timed out waiting for RM reply.  %d RM replies "
                 "outstanding, transaction aborted or hung.\n", 
                 (pv_num - lv_count));
         tm_log_event(DTM_XATM_COMPLETEALL_FAILED, SQ_LOG_WARNING, "DTM_XATM_COMPLETEALL_FAILED",
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
                    (pv_num - lv_count));

         XATrace(XATM_TraceError,("XATM: complete_all %s", la_buf));
         break;
      }

      do 
      {
         // we've reached our message reply count, break
         //if (lv_count >= pv_num)Temp - force listen to exaust all messages
         //   break;

         lv_ret = BMSG_LISTEN_((short *)&lv_sre, 
                              BLISTEN_ALLOW_LDONEM, 0);
         XATrace(XATM_TraceSpecial, ("XATM: complete_all: Count(%d) BMSG_LISTEN_ "
                 "completed with returncode=%d, sre.msgId=%d, sre.linkTag(rmid)=%d.\n",
                 lv_count, lv_ret, lv_sre.sre_msgId, (int)lv_sre.sre_linkTag));

         if (lv_ret == BSRETYPE_LDONE)
         {
            // The sre_linkTag contains the RMID.  Since we only ever have
            // one message outstanding for an RM, finding the RM object finds
            // the message.
            int lv_rmid = (int) lv_sre.sre_linkTag; //intentional truncation on 64 bit patforms.
            //XATrace(XATM_TraceDetail, ("XATM: complete_all rmid=%d, msgid=%d\n", 
            //        lv_rmid, lv_sre.sre_msgId));
            if (lv_count >= pv_num)  //Check for extra replies
            {
               tm_log_event(DTM_XATM_COMPLETEALL_FAILED, SQ_LOG_CRIT,"DTM_XATM_COMPLETEALL_FAILED");
               XATrace(XATM_TraceError, ("XATM: complete_all ERROR. rmid=%d, "
                       "msgid=%d Count %d exceeds %d requests expected.\n",
                       lv_rmid, lv_sre.sre_msgId, lv_count, pv_num));
               abort();
            }


            CxaTM_RM *lp_RM = gv_xaTM.getRM(lv_rmid);
            if (!lp_RM)
            {
               // EMS DTM_XATM_COMPLETEALL_FAILED
               tm_log_event(DTM_XATM_COMPLETEALL_FAILED, SQ_LOG_CRIT,"DTM_XATM_COMPLETEALL_FAILED");
               XATrace(XATM_TraceError,("XATM: complete_all BMSG_LISTEN_ "
                       "returned unknown tag (rmid) %d.\n", lv_rmid));
               abort();
            }


            // Look up message in RMs outstanding messages list
            CxaTM_RMMessage *lp_msg = (CxaTM_RMMessage *) lp_RM->msgList()->get(lv_sre.sre_msgId);
            if (lp_msg == NULL)
            {
               tm_log_event(DTM_XATM_COMPLETEALL_FAILED, SQ_LOG_CRIT,"DTM_XATM_COMPLETEALL_FAILED");
               XATrace(XATM_TraceError, ("XATM: complete_all ERROR. rmid=%d, "
                       "msgid=%d not found in RM outstanding message list, ignored.\n",
                       lv_rmid, lv_sre.sre_msgId));
               abort();
            }
            else
            {
               lp_RM->lock(); //Lock the RM to make sure no one else can change the msgList before we do.
               gv_xaTM.check_msgIntegrity(pv_transid, lp_msg);
               lv_xaError = XA_OK;
               lv_ret2 = BMSG_BREAK_(lp_msg->msgid(), 
                                    la_results,
                                    lp_RM->getRmPhandle());

               XATrace(XATM_TraceSpecial, ("XATM: complete_all BMSG_BREAK_ completed with %d, msgid %d, rmid %d, sendAttempts %d\n",
                       lv_ret2, lp_msg->msgid(), lp_RM->getRmid(), lp_msg->sendAttempts()));

               // If break returns an error we retry the link - resend the message to the 
               // RM. 
               lv_retryLink = lp_msg->checkError(lv_ret2, &lv_xaError, true, pv_transid);

               if (lv_retryLink == false)
               {
                  /////////////////////////////////////////////////////  
                  // deal with responses here                        //
                  /////////////////////////////////////////////////////
                  pa_responses[lv_count].iv_rmid = lp_RM->getRmid();
                  pa_responses[lv_count].iv_error = lv_xaError;
                  int32 lv_type = lp_msg->getRmReplyType();

                  if (lv_xaError)
                  {
                     XATrace(XATM_TraceError,("XATM: complete_all rmid=%d replied "
                           "to message type %d with XA error %s\n",
                           pa_responses[lv_count].iv_rmid, lv_type, 
                           XAtoa(pa_responses[lv_count].iv_error)));
                  }

                  switch (lv_type)
                  {
                     case TM_DP2_SQ_XA_PREPARE_REPLY:
                     {
                        // we don't want to overwrite an error by a non partic
                        if ((lp_msg->getRmReplyPreparePartic()==0) &&
                           (pa_responses[lv_count].iv_error == XA_OK))
                        {
                           pa_responses[lv_count].iv_error = XA_RDONLY;
                           XATrace(XATM_TraceError,("XATM: complete_all rmid=%d replied to xa_prepare "
                                 "indicating non-participation so returning XA_RDONLY\n",
                                 pa_responses[lv_count].iv_rmid));
                        }
                        break;
                     }
                     case TM_DP2_SQ_XA_OPEN_REPLY:
                     {
                        pa_responses[lv_count].iv_ax_reg = lp_msg->getRmReplyOpenAx_Reg();
                        if (lp_msg->getRmReplyOpenOpener())
                           pa_responses[lv_count].iv_drive_recover = true;
                        break; 
                     }
                     case TM_DP2_SQ_XA_CLOSE_REPLY:
                     {
                        // We can delete the RM now the I/O has completed
                        lp_RM->msgList()->remove(lp_msg->msgid());
                        lp_RM = NULL;
                        // defer the delete as we have the RM locked right now.
                        lv_delete_RM = true;
                        break;
                     }
                     default :
                     {
                        break;
                     }
                  }; //switch

                  // Completed one response, clean up 
                  lv_count++;
                  // release the message object
                  if (lp_RM)
                  {
                     gv_xaTM.release_RMmsg(lp_msg);
                     lp_RM->unlock();
                     if (lv_delete_RM)
                     {
                        gv_xaTM.deleteRM(lp_RM);
                        lv_delete_RM = false;
                     }
                  }
               }  ////////////////////////////////////////////////////
                  // End dealing with responses                     //
                  ////////////////////////////////////////////////////
               else
               {
                  // delay, retry link, loop back and wait for another LDONE completion
                  if (lp_RM)
                  {
                     if (lp_msg)
                     {
                        // We reuse the RM message object, so only remove it from the RMs msgList.
                        lp_msg->release_from_RMmsgList();
                        lv_retryLink = lp_msg->retrySend();
                     }
                     lp_RM->unlock();
                  }
                  lv_retryLink = false;
                }
            } // else found message object, call BREAK
         } //if LDONE
         XATrace(XATM_TraceSpecial, ("XATM: complete_all response count=%d, sre.linkTag=%d, error=%d\n",
                 lv_count-1, pa_responses[lv_count-1].iv_rmid, pa_responses[lv_count-1].iv_error));
      } while (lv_ret == BSRETYPE_LDONE);
   } //while (true)

   if (pv_transid)
      gv_xaTM.cancelAll((pv_num - lv_count), pv_num, pv_transid);

   XATrace(XATM_TraceXAAPI,("XATM: complete_all EXIT expected %d replies, received %d.\n", pv_num, lv_count));

   // Return the number of RM replies still outstanding.  This should be 0!
   return (pv_num - lv_count);
} // complete_all


// ---------------------------------------------------------------
// xaTM_initialize
// Purpose - Initialize the XATM.
// This function is called by each tm_xa_* to ensure that the 
// initialization has been done.  When this library is being used
// as an XARM interface the method can not be called directly as
// it is in the TM.
// The initialized flag can only be false if the library is
// acting as an XARM.
// ---------------------------------------------------------------
int xaTM_initialize ()
{
   int lv_error = 0;
   char  la_value[9];
   bool lv_tm_stats = false;

   // Only proceed the first time this is called.
   if (gv_xaTM.initialized())
      return lv_error;
   else
   {
      // XARM library only - we are not executing inside the TM
      tm_switch = &tse_switch;
      // Get DTM_TM_STATS
      lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                           (char *) CLUSTER_GROUP, (char *) DTM_TM_STATS, 
                           la_value);
      if (lv_error == 0)
         lv_tm_stats = ((atoi(la_value) == 0)?false:true);

      lv_error = xaTM_initialize(true, lv_tm_stats, NULL);
   }
   return lv_error;
} //xaTM_initialize


// ---------------------------------------------------------------
// xaTM_initialize
// Purpose - Initialize the XA TM.
// ---------------------------------------------------------------
int xaTM_initialize (bool pp_tracing, bool pv_tm_stats, CTmTimer *pp_tmTimer)
{
   int lv_error = 0;
   union {
      XATM_TraceMask lv_traceMask;
      int lv_traceMaskInt;
   } u;

   //initialize trace file
   char * lp_traceString = (char *) ms_getenv_str("XATM_TRACE");
   char * lp_traceStringEnd;
   if (pp_tracing != 0 && lp_traceString)
   {
      lp_traceStringEnd = lp_traceString + strlen(lp_traceString);

      if (lp_traceStringEnd == NULL)
      {
        tm_log_event(DTM_XMAPI_INVALID_STRING_SIZE, SQ_LOG_CRIT, "DTM_XMAPI_INVALID_STRING_SIZE");
        // Make sure the lp_traceStringEnd pointer points to the null terminator.
        abort();
      }

      //Convert hexadecimal string to int
      unsigned long lv_traceMaskul = strtoul(lp_traceString, &lp_traceStringEnd, 16); 
      u.lv_traceMaskInt = (int) lv_traceMaskul;
   }
   else
      u.lv_traceMask = XATM_TraceOff;

   lv_error = gv_xaTM.initialize(u.lv_traceMask, pv_tm_stats, pp_tmTimer);

   // Check whether we are running as a TM or as an XARM library
   if (tm_XARM_amIaTM())
   {
      tm_switch = &tse_switch;      
   }

   XATrace((lv_error?XATM_TraceAPIExitError:XATM_TraceAPIExit),
           ("XATM: xaTM_initialize EXIT returning %d\n", lv_error));
   return lv_error;
}


// ---------------------------------------------------------------
// xaTM_setTrace
// Purpose - Set the XATM trace mask to a new value.
// ---------------------------------------------------------------
void xaTM_setTrace(unsigned long pv_mask)
{
   union {
      XATM_TraceMask lv_traceMask;
      int lv_traceMaskInt;
   } u;

   u.lv_traceMaskInt = (int) pv_mask;
   gv_xaTM.traceMask(u.lv_traceMask);

   XATrace(XATM_TraceAPIExit,("XATM: xaTM_setTrace EXIT : New trace mask %uld\n",
      gv_xaTM.traceMask()));
}

// ---------------------------------------------------------------
// xaTM_RMMessagePool
// Purpose - Retrieve a pointer to the RMMessagePool
// ---------------------------------------------------------------
CTmPool<CxaTM_RMMessage> * xaTM_RMMessagePool ()
{
   return gv_xaTM.RMMessagePool();
}

// ---------------------------------------------------------------
// tm_xa_open
// Purpose - open an RM
// This routine checks the parameters, instantiates a CxaTM_RM 
// object, and invokes its open method which causes the xa_open
// message to be sent to the RM.
// Asynchronous xa_open() is not supported.
// 
// Return Codes:
//    XA_OK          Normal completion.
//    XAER_ASYNC     Async opens not supported yet.
//    XAER_RMFAIL     Error while opening RM.
//    XAER_INVAL     Invalid parameter.
//    XAER_PROTO     xa_open() called against an open RM.
// ---------------------------------------------------------------
int tm_xa_open (char *info, int rmid, int64 flags)
{
   int lv_xaError = XA_OK;
   CxaTM_RM *lp_RM;
  
   // Validate parameters
   if ((!info) || (rmid < 0))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_open failed with XAER_INVAL (invalid parameter)\n"));
      return XAER_INVAL;
   }

   XATrace(XATM_TraceXAAPI,("XATM: xa_open ENTRY info=%s, rmid=%d, flags=0x" PFLLX "\n",
            info, rmid, flags));

   // Currently no flags are supported for TSE xa_open
   if (!tm_XARM_generic_library() && (flags & TMASYNC))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_open failed with XAER_ASYNC\n"));
      return XAER_ASYNC;
   }

   // Instantiate an RM object and open the RM.  
   // There is one RM object per open RM.
   lv_xaError = gv_xaTM.newRM(rmid, &lp_RM);
   
   if (lp_RM == NULL)
   {
        tm_log_event(DTM_XMAPI_INVALID_RM_OBJ, SQ_LOG_CRIT, "DTM_XMAPI_INVALID_RM_OBJ");
        XATrace(XATM_TraceAPIError,("XATM: xa_open failed - Invalid RM object\n"));
        abort(); //Program logic error
   }

   if (lv_xaError < XA_OK)
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_open failed to instantiate new RM with %s\n",
                                  XAtoa(lv_xaError)));
   }
   else
   {
      lv_xaError = lp_RM->open(info, flags);
    
      // Tear down the RM object if there was an error
      if (lv_xaError < XA_OK)
      {
         XATrace(XATM_TraceAPIError,("XATM: xa_open failed to open RM with %s\n",
                                    XAtoa(lv_xaError)));
         gv_xaTM.deleteRM(lp_RM); 
      }
   }

   if (lv_xaError <= XA_OK)
   {
      XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
            ("XATM: xa_open EXIT returning %s.\n", XAtoa(lv_xaError)));
   }
   else
   {
      XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
            ("XATM: xa_open EXIT TMASYNC returning handle %d.\n", lv_xaError));
   }
   return lv_xaError;
} //tm_xa_open


// -------------------------------------------------------
// tm_xa_close
// Purpose -  close an RM, them remove from the rm map
// ------------------------------------------------------
int tm_xa_close (char *info, int rmid, int64 flags)
{
   int lv_xaError = XA_OK;
   
   XATrace(XATM_TraceXAAPI,("XATM: xa_close ENTRY info=%s, rmid=%d, flags=0x" PFLLX "\n",
            info, rmid, flags));

   // At shutdown time, info is used by the lead TM to send the "SHUTDOWN" string to the RMs
   if (rmid < 0)
   {
       XATrace(XATM_TraceAPIError,("XATM: xa_close failed with XAER_INVAL (rmid < 0)\n"));
      return XAER_INVAL;
   }

   // Currently no flags are supported for xa_close
   if (!tm_XARM_generic_library() && (flags & TMASYNC))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_close failed with XAER_ASYNC\n"));
      return XAER_ASYNC;
   }

   CxaTM_RM *lp_RM = gv_xaTM.getRM(rmid);

   if (!lp_RM)
   {
      lv_xaError = XAER_RMFAIL;
      XATrace(XATM_TraceAPIError,("XATM: xa_close failed with XAER_RMFAIL\n"));
   }
   else
   {
      lv_xaError = lp_RM->close(info, flags);
      // delete the RM once we receive the reply from the RM.
   }

   if (lv_xaError <= XA_OK)
   {
      XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
            ("XATM: xa_close EXIT returning %s.\n", XAtoa(lv_xaError)));
   }
   else
   {
      XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
            ("XATM: xa_close EXIT TMASYNC returning handle %d.\n", lv_xaError));
   }
   return lv_xaError;
} //tm_xa_close


// -----------------------------------------------------
// tm_xa_start
// Purpose - send an xa_start message
// -----------------------------------------------------
int tm_xa_start (XID *xid, int rmid, int64 flags)
{
   int lv_xaError = XA_OK;

   XATrace(XATM_TraceXAAPI,("XATM: xa_start ENTRY rmid=%d, flags=0x" PFLLX ", xid=%s\n",
           rmid, flags, XIDtoa(xid)));

   // Validate xid and rmid
   if ((!xid) || (rmid < 0))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_start failed with XAER_INVAL (invalid parameter)\n"));
      return XAER_INVAL;
   }

   // Validate flags
   if (!tm_XARM_generic_library() && (flags & TMASYNC))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_start failed with XAER_ASYNC\n"));
      return XAER_ASYNC;
   }
   if (tm_XARM_generic_library() && flags && 
       ((flags & TMNOWAIT) ||
       (!(flags & TMJOIN) && !(flags & TMRESUME))))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_start failed with XAER_INVAL (invalid flags)\n"));
      return XAER_INVAL;
   }

   CxaTM_RM *lp_RM = gv_xaTM.getRM(rmid);

   // Return an error if the RM wasn't found
   if (!lp_RM)
      lv_xaError = XAER_RMFAIL;
   else
      lv_xaError = lp_RM->start(xid, flags);

   if (lv_xaError <= XA_OK)
   {
      XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
            ("XATM: xa_start EXIT returning %s.\n", XAtoa(lv_xaError)));
   }
   else
   {
      XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
            ("XATM: xa_start EXIT TMASYNC returning handle %d.\n", lv_xaError));
   }
   return lv_xaError;
} //tm_xa_start


// ------------------------------------------------------
// tm_xa_end
// Purpose - send xa_end message
// ------------------------------------------------------
int tm_xa_end (XID *xid, int rmid, int64 flags)
{
   int lv_xaError = XA_OK;

   XATrace(XATM_TraceXAAPI,("XATM: xa_end ENTRY rmid=%d, flags=0x" PFLLX ", xid=%s\n",
           rmid, flags, XIDtoa(xid)));

   // validate xid and rmid
   if ((!xid) || (rmid < 0))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_end failed with XAER_INVAL (invalid parameter)\n"));
      return XAER_INVAL;
   }

   // validate flags
   if (!tm_XARM_generic_library() && (flags & TMASYNC))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_end failed with XAER_ASYNC\n"));
      return XAER_ASYNC;
   }
   if (tm_XARM_generic_library() && flags && 
       (!(flags & TMSUSPEND) && !(flags & TMMIGRATE) &&
        !(flags & TMSUCCESS) &&  !(flags & TMFAIL)))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_end failed with XAER_INVAL (invalid flags)\n"));
      return XAER_INVAL;
   }

   CxaTM_RM *lp_RM = gv_xaTM.getRM(rmid);

   // Return an error if the RM wasn't found
   if (!lp_RM)
      lv_xaError = XAER_RMFAIL;
   else
      lv_xaError = lp_RM->end(xid, flags);
   
   XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
           ("XATM: xa_end EXIT returning %s.\n", XAtoa(lv_xaError)));

   if (lv_xaError <= XA_OK)
   {
      XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
            ("XATM: xa_end EXIT returning %s.\n", XAtoa(lv_xaError)));
   }
   else
   {
      XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
            ("XATM: xa_end EXIT TMASYNC returning handle %d.\n", lv_xaError));
   }
   return lv_xaError;
} //tm_xa_end


// -----------------------------------------------------
// tm_xa_prepare
// Purpose - send xa_prepare message
// ----------------------------------------------------
int tm_xa_prepare (XID *xid, int rmid, int64 flags)
{
   int lv_xaError = XA_OK;

   XATrace(XATM_TraceXAAPI,("XATM: xa_prepare ENTRY rmid=%d, flags=0x" PFLLX ", xid=%s\n",
           rmid, flags, XIDtoa(xid)));

   // validate xid and rmid
   if ((!xid) || (rmid < 0))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_prepare failed with XAER_INVAL (invalid parameter)\n"));
      return XAER_INVAL;
   }

   // validate flags
   if (!tm_XARM_generic_library() && (flags & TMASYNC))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_prepare failed with XAER_ASYNC\n"));
      return XAER_ASYNC;
   }

   CxaTM_RM *lp_RM = gv_xaTM.getRM(rmid);

   // Return an error if the RM wasn't found
   if (!lp_RM)
   {
      lv_xaError = XAER_RMFAIL;
      XATrace(XATM_TraceAPIError,("XATM: xa_prepare failed with XAER_RMFAIL\n"));
   }
   else
      lv_xaError = lp_RM->prepare(xid, flags);
   
   if (lv_xaError <= XA_OK)
   {
      XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
            ("XATM: xa_prepare EXIT returning %s.\n", XAtoa(lv_xaError)));
   }
   else
   {
      XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
            ("XATM: xa_prepare EXIT TMASYNC returning handle %d.\n", lv_xaError));
   }
   return lv_xaError;
} //tm_xa_prepare

// ---------------------------------------------------
// tm_xa_rollback
// Purpose -  send xa_rollback message
// ---------------------------------------------------
int tm_xa_rollback (XID *xid, int rmid, int64 flags)
{
   int lv_xaError = XA_OK;

   XATrace(XATM_TraceXAAPI,("XATM: xa_rollback ENTRY rmid=%d, flags=0x" PFLLX ", xid=%s\n",
           rmid, flags, XIDtoa(xid)));

   // validate xid and rmid
   if ((!xid) || (rmid < 0))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_rollback failed with XAER_INVAL (invalid parameter)\n"));
      return XAER_INVAL;
   }

   // validate flags
   if (!tm_XARM_generic_library() && (flags & TMASYNC))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_rollback failed with XAER_ASYNC\n"));
      return XAER_ASYNC;
   }

   CxaTM_RM *lp_RM = gv_xaTM.getRM(rmid);

   // Return an error if the RM wasn't found
   if (!lp_RM)
   {
      lv_xaError = XAER_RMFAIL;
      XATrace(XATM_TraceAPIError,("XATM: xa_rollback failed with XAER_RMFAIL\n"));
   }
   else
      lv_xaError = lp_RM->rollback(xid, flags);
   
   if (lv_xaError <= XA_OK)
   {
      XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
            ("XATM: xa_rollback EXIT returning %s.\n", XAtoa(lv_xaError)));
   }
   else
   {
      XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
            ("XATM: xa_rollback EXIT TMASYNC returning handle %d.\n", lv_xaError));
   }
   return lv_xaError;
} //tm_xa_rollback


// -----------------------------------------------
// tm_xa_commit
// Purpose - send xa_commit message
// -----------------------------------------------
int tm_xa_commit (XID *xid, int rmid, int64 flags)
{
   int lv_xaError = XA_OK;

   XATrace(XATM_TraceXAAPI,("XATM: xa_commit ENTRY rmid=%d, flags=0x" PFLLX ", xid=%s\n",
           rmid, flags, XIDtoa(xid)));

   // validate xid and rmid
   if ((!xid) || (rmid < 0))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_commit failed with XAER_INVAL (invalid parameter)\n"));
      return XAER_INVAL;
   }

   // validate flags
   if (!tm_XARM_generic_library() && (flags & TMASYNC))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_commit failed with XAER_ASYNC\n"));
      return XAER_ASYNC;
   }

   CxaTM_RM *lp_RM = gv_xaTM.getRM(rmid);

   // Return an error if the RM wasn't found
   if (!lp_RM)
   {
      lv_xaError = XAER_RMFAIL;
      XATrace(XATM_TraceAPIError,("XATM: xa_commit failed with XAER_RMFAIL\n"));
   }
   else
      lv_xaError = lp_RM->commit(xid, flags);
   
   if (lv_xaError <= XA_OK)
   {
      XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
            ("XATM: xa_commit EXIT returning %s.\n", XAtoa(lv_xaError)));
   }
   else
   {
      XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
            ("XATM: xa_commit EXIT TMASYNC returning handle %d.\n", lv_xaError));
   }
   return lv_xaError;
} //tm_xa_commit


// ----------------------------------------------
// tm_xa_forget 
// Purpose - send xa_forget message
// ----------------------------------------------
int tm_xa_forget (XID *xid, int rmid, int64 flags)
{
   int lv_xaError = XA_OK;

   XATrace(XATM_TraceXAAPI,("XATM: xa_forget ENTRY rmid=%d, flags=0x" PFLLX ", xid=%s\n",
           rmid, flags, XIDtoa(xid)));

   // validate xid and rmid
   if ((!xid) || (rmid < 0))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_forget failed with XAER_INVAL (invalid parameter)\n"));
      return XAER_INVAL;
   }

   // validate flags
   if (!tm_XARM_generic_library() && (flags & TMASYNC))
   {
      XATrace(XATM_TraceAPIError,("XATM: xa_forget failed with XAER_ASYNC\n"));
      return XAER_ASYNC;
   }

   CxaTM_RM *lp_RM = gv_xaTM.getRM(rmid);

   // Return an error if the RM wasn't found
   if (!lp_RM)
   {
      lv_xaError = XAER_RMFAIL;
      XATrace(XATM_TraceAPIError,("XATM: xa_forget failed with XAER_RMFAIL\n"));
   }
   else
      lv_xaError = lp_RM->forget(xid, flags);
   
   if (lv_xaError <= XA_OK)
   {
      XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
            ("XATM: xa_forget EXIT returning %s.\n", XAtoa(lv_xaError)));
   }
   else
   {
      XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
            ("XATM: xa_forget EXIT TMASYNC returning handle %d.\n", lv_xaError));
   }
   return lv_xaError;
}


// ----------------------------------------------------------
// tm_xa_recover
// Purpose - send xa_recovery message
// This is currently a dummy routine, left in to fill the 
// xa_switch structure out correctly.
// -----------------------------------------------------------
int tm_xa_recover (XID *xids, int64 count, int rmid, int64 flags)
{
   //int lv_xaError = XA_OK;

   XATrace(XATM_TraceXAAPI,("XATM: tm_xa_recover ENTRY UNEXPECTED! rmid=%d, flags=0x" PFLLX 
           ", count=" PFLL ".\n", rmid, flags, count));

   return XAER_INVAL;
} //tm_xa_recover

// ---------------------------------------------------------------
// tm_xa_recover_send
// Purpose - Send an xa_recover request to a specific RM.
// node defaults to -1, system recovery.
// Returns:
//    XA_OK       Successful
//    XA_INVAL    Invalid parameter
//    XA_RMFAIL   Couldn't find the RM in the RM object list.
//    others      Returned by CxaTM_RM::recover_send.
// -----------------------------------------------------------------
int tm_xa_recover_send(int pv_rmid, int64 pv_count, int64 pv_flags, 
                       int pv_index = 0, int pv_node=-1, bool pv_dead_tm=true)
{
   int lv_xaError = XA_OK;

   XATrace(XATM_TraceXAAPI,("XATM: tm_xa_recover_send ENTRY rmid=%d, flags=0x" PFLLX 
           ", count=" PFLL ", node=%d\n", pv_rmid, pv_flags, pv_count, pv_node));

   // validate parameters
   if ((pv_rmid < 0) || (pv_count <= 0) || (pv_node >= MAX_NODES))
   {
      XATrace(XATM_TraceAPIError,("XATM: tm_xa_recover_send failed with XAER_INVAL "
                                  "(invalid parameter)\n"));
      return XAER_INVAL;
   }

   // validate flags
   if (!(pv_flags & TMSTARTRSCAN) && !(pv_flags & TMENDRSCAN) && 
       (pv_flags != TMNOFLAGS) && (pv_flags != TMRESENDRSCAN) &&
       (pv_flags != TMRMFAILRSCAN))

   {
      XATrace(XATM_TraceAPIError,("XATM: tm_xa_recover_send failed with XAER_INVAL "
                                  "(invalid flag specified)\n"));
      return XAER_INVAL;
   }

   CxaTM_RM *lp_RM = gv_xaTM.getRM(pv_rmid);

   // Return an error if the RM wasn't found
   if (!lp_RM)
      return XAER_RMFAIL;

   //Lock the RM to make sure only one xa_recover can be sent to an RM at a time.
   //It will be unlocked in tm_xa_recover_waitReply.
   lp_RM->lock();
   lv_xaError = lp_RM->recoverSend(pv_count, pv_flags, pv_node, pv_dead_tm, pv_index);
   
   XATrace(XATM_TraceAPIExitError,
            ("XATM: tm_xa_recover_send EXIT returning error %s.\n", 
            XAtoa(lv_xaError)));
   return lv_xaError;
} //tm_xa_recover_send


// ---------------------------------------------------------------
// tm_xa_recover_waitReply
// Purpose - Wait for the next xa_recover reply to arrive.
// All parameters are retrieved from the xa_recover reply.
// pp_rmid Output
//         Rmid for the RM sending this xa_recover reply. Initialized
//         to -1 to avoid confusion over whether we set it.
// pp_xids Output
//         Points to a buffer allocated by the caller.  
//         tm_xa_recover_waitReply copies the XIDs from the message
//         into this buffer and then returns the message object to
//         the pool.
// pp_count Input/Output.
//         Contains the maximum number of XIDs allocated in pp_xids
//         on input.  On output it contains the actual number of 
//         XIDs returned in pp_xids.
// pp_end  Output
//         True indicates this is the last buffer to be returned.
//         False indicates there are more XIDs to come and xa_recover
//         must be re-sent.
//
// pp_index Output
//         Index to be sent to the TSE on the next xa_recover (if
//         applicable)
// pv_rm_wait_time input
//         Time in 10 millisecond intervals that we will wait for a 
//         LDONE completion from the RM.
//
// pp_int_error output
//         Seabed error
// -----------------------------------------------------------------
int tm_xa_recover_waitReply(int *pp_rmid, XID *pp_xids, int64 *pp_count, 
                            bool *pp_end, int *pp_index, int pv_rm_wait_time,
                            int *pp_int_error)
{
   char la_buf[DTM_STRING_BUF_SIZE];
   static short lv_ret = BSRETYPE_NOWORK;  //Persists across calls!
   short  lv_ret2;
   int lv_xaError = XA_OK;
   short  la_results[6];
   int64 lv_count = 0;
   BMS_SRE_LDONE lv_sre;
   short lv_event = 0;
   int32 lv_type = -1;
   bool  lv_retryLink = false;

   if (!pp_rmid || !pp_xids || !pp_count || pp_count <= 0 || !pp_end || !pp_index || !pp_int_error)
   {
      XATrace(XATM_TraceAPIError,("XATM: tm_xa_recover_waitReply failed with XAER_INVAL "
                                  "(invalid parameter or bad address)\n"));
      return XAER_INVAL;
   }

   if(pp_rmid != NULL) {
      *pp_rmid = -1; //Initialize
      XATrace(XATM_TraceXAAPI,("XATM: tm_xa_recover_waitReply ENTRY rmid %d, count " 
           PFLL ", end %d, RM wait time %d.\n", 
           *pp_rmid, *pp_count, *pp_end, pv_rm_wait_time));
   }

   // Initialize return values
   *pp_rmid=0;
   *pp_end=false;
   *pp_index=0;
   *pp_int_error = FEOK;

   do 
   {
      lv_retryLink = false;

      // Wait for the next LDONE completion
      // We don't want to wait if we haven't processed all the LDONE 
      // completions since the last wait.
      if (lv_ret != BSRETYPE_LDONE)
      {
         lv_event = XWAIT(LDONE, pv_rm_wait_time);

         // Timeout break and exit.
         if (lv_event == 0)
         {
            // EMS DTM_XATM_COMPLETEALL_FAILED
            sprintf(la_buf, "Timed out after %d msec waiting for RM reply, "
                    "returning XAER_RMFAIL.\n", pv_rm_wait_time);
            tm_log_event(DTM_XATM_XA_RECOVER_TIMEOUT, SQ_LOG_WARNING, 
                         "DTM_XATM_XA_RECOVER_TIMEOUT", FETIMEDOUT,
                         -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,pv_rm_wait_time);
            XATrace(XATM_TraceError,("XATM: tm_xa_recover_waitReply %s", la_buf));
            *pp_count = 0;
            lv_xaError = XAER_RMFAIL;
            break;
         }
      }

      lv_ret = BMSG_LISTEN_((short *)&lv_sre, BLISTEN_ALLOW_LDONEM, 0);

      if (lv_ret == BSRETYPE_LDONE)
      {
         *pp_rmid = lv_sre.sre_linkTag;

         CxaTM_RM *lp_RM = gv_xaTM.getRM(*pp_rmid);
         
         // Check the RM is valid
         if (!lp_RM)
         {
            // EMS DTM_XATM_RECOVER_FAILED_UNEXP_TAG
            tm_log_event(DTM_XATM_RECOVER_FAILED_UNEXP_TAG, SQ_LOG_CRIT, "DTM_XATM_RECOVER_FAILED_UNEXP_TAG");
            XATrace(XATM_TraceError,("XATM: tm_xa_recover_waitReply ERROR Listen "
                                     "completed for unexpected tag (rmid), returning "
                                     "XAER_RMFAIL.\n"));
            abort(); //Critical fault - fast fail
         }

         // Find the associated message object
         CxaTM_RMMessage *lp_msg = (CxaTM_RMMessage *) lp_RM->msgList()->get(lv_sre.sre_msgId);
         if (lp_msg == NULL)
         {  
             tm_log_event(DTM_XATMLIB_INVALID_RM_MSG, SQ_LOG_CRIT, "DTM_XATMLIB_INVALID_RM_MSG");
             XATrace(XATM_TraceError, ("XATM: tm_xa_recover_waitReply ERROR. rmid=%d, "
                     "msgid=%d not found in RM outstanding message list.\n",
                     *pp_rmid, lv_sre.sre_msgId));
            abort(); //Critical fault - fast fail
         }
         else
         {
            lv_ret2 = BMSG_BREAK_(lp_msg->msgid(), la_results, lp_RM->getRmPhandle());
            *pp_int_error = lv_ret2;

            XATrace(XATM_TraceSpecial, ("XATM: tm_xa_recover_waitReply BMSG_BREAK_ completed "
                    "with %d, msgid %d, rmid %d, sendAttempts %d\n",
                    lv_ret2, lp_msg->msgid(), lp_RM->getRmid(), lp_msg->sendAttempts()));

            // If break returns an error we retry the link - resend the message to the RM.
            lv_retryLink = lp_msg->checkError(lv_ret2, &lv_xaError, false /*no soft retry*/);

            if (lv_retryLink == false)
            {
               lv_type = lp_msg->getRmReplyType();

               // Check the reply
               if (lv_xaError || lv_type != TM_DP2_SQ_XA_RECOVER_REPLY)
               {
                  XATrace(XATM_TraceError,("XATM: tm_xa_recover_waitReply ERROR rmid=%d replied "
                          "to message type %d with XA error %s\n",
                          *pp_rmid, lv_type, XAtoa(lv_xaError)));
                  // Only report errors, not warnings
                  if (lv_xaError < XA_OK) 
                     tm_log_event(DTM_XATM_RECOVER_FAILED_UNEXP_REPLY, SQ_LOG_WARNING, 
                                  "DTM_XATM_RECOVER_FAILED_UNEXP_REPLY",lv_xaError,*pp_rmid,-1,-1,
                                  -1,lv_xaError,-1,-1,-1,-1,-1,-1,-1,lv_type);
                }
                else
                {  /////////////////////////////////////////////////////  
                   // deal with valid responses here                  //
                   /////////////////////////////////////////////////////
                   *pp_end = lp_msg->Rsp()->u.iv_recover.iv_end;
                   *pp_index = lp_msg->Rsp()->u.iv_recover.iv_recovery_index;
                   lv_count = (int64) lp_msg->Rsp()->u.iv_recover.iv_count;
                   // Check that the RM didn't return more XIDs than we allocated
                   // buffer space for.
                   if (lv_count <= *pp_count)
                      *pp_count = lv_count; // return count
                   else
                   {
                      tm_log_event(DTM_XATM_RECOVER_FAILED_PROG_ERROR, SQ_LOG_CRIT, 
                                   "DTM_XATM_RECOVER_FAILED_PROG_ERROR",-1,*pp_rmid);
                      XATrace(XATM_TraceError,("XATM: tm_xa_recover_waitReply PROGRAMMING ERROR: RM %d returned "
                                              " too many XIDs!\n", *pp_rmid));
                      abort();
                   }

                   memcpy(pp_xids, (XID *) &(lp_msg->Rsp()->u.iv_recover.iv_xid), (lv_count*sizeof(XID)));
                }  ////////////////////////////////////////////////////
                   // End dealing with responses                     //
                   ////////////////////////////////////////////////////

                // Now we can clean up the message object
                if (lp_RM)
                {
                    gv_xaTM.release_RMmsg(lp_msg);
                    lp_RM->unlock();
                }
            } // Don't retry, so we have a reply or error to process
            else
            {
                // delay, retry link, loop back and wait for another LDONE completion
                if (lp_RM)
                {
                   if (lp_msg)
                   {
                      // We reuse the RM message object, so only remove it from the RMs msgList.
                      lp_msg->release_from_RMmsgList();
                      lv_retryLink = lp_msg->retrySend();

                     // Initialize return values back since we don't have a reply to process and we will return XA_OK 
                     XATrace(XATM_TraceError,("XATM: tm_xa_recover_waitReply Warning resetting return values since had to retry send.\n"));
                     *pp_count=0;
                     *pp_end=false;
                     *pp_index=0;
                     *pp_int_error = FEOK;

                   }
                   lp_RM->unlock();
                }
                lv_retryLink = false;   
            } //retry link
         } // msg found
      } // LDONE
      else // not LDONE
      {
         XATrace(XATM_TraceSpecial,("XATM: tm_xa_recover_waitReply : Listen completed with "
               "return code: %d, waiting for next LDONE.\n", lv_ret));
      }
   } while ((lv_ret != BSRETYPE_LDONE && lv_xaError == XA_OK) || lv_retryLink == true);

   XATrace(XATM_TraceExit,("XATM: tm_xa_recover_waitReply EXIT rmid=%d, count=" PFLL
           " end=%d, index=%d, returning error %s.\n", *pp_rmid, *pp_count, *pp_end, 
           *pp_index, XAtoa(lv_xaError)));
   return lv_xaError;
} //tm_xa_recover_waitReply


// -----------------------------------------------------------
// tm_xa_complete
// Purpose - send xa_complete message
// ----------------------------------------------------------
int tm_xa_complete (int *handle, int *ret, int rmid, int64 flags)
{
   int lv_xaError = XA_OK;
   // intel compiler warnings 869
   ret = ret;
   handle = handle;

   XATrace(XATM_TraceExit,("XATM: xa_complete ENTRY rmid=%d, flags=0x" PFLLX "\n",
           rmid, flags));

   XATrace(XATM_TraceAPIError,("XATM: xa_complete currently not supported!\n"));
   // TODO - implement routine
   XATrace((lv_xaError?XATM_TraceAPIExitError:XATM_TraceAPIExit),
           ("XATM: xa_complete EXIT returning %s.\n", XAtoa(lv_xaError)));
   return lv_xaError;
} //tm_xa_complete


// ---------------------------------------------------------------------
// complete_one
// Purpose - This function is called by xa_* APIs to perform a waited
// completion for the method when the library is acting as a standard
// XARM implementation. This is similar to complete_all, but we wait
// for one reply from the associated TM.
// Parameters:
//    rmid is the identifier for the RM with an outstanding request.
//    handle is the handle for completion of a no-waited operation. 
//           0 indicates any completion for the RM will be returned.
// Returns the XA return code indicating the outcome of the xa_* request.
// If the rmid or handle don't match those passed to complete_one then
// an error is returned.
// ---------------------------------------------------------------------
int complete_one(int pv_rmid, int *pp_handle)
{ 
   short          la_results[6];
   long           lv_ret;
   long           lv_ret2;
   BMS_SRE_LDONE  lv_sre;
   char           la_buf[DTM_STRING_BUF_SIZE];
   int            lv_xaError = XA_OK;
   bool           lv_retryLink = false;
   short          lv_event = 0;

   XATrace(XATM_TraceXAAPI,("XATM: complete_one ENTRY, rmid(%d), handle(%d).\n", 
         pv_rmid, *pp_handle));

   CxaTM_RM *lp_RM = gv_xaTM.getRM(pv_rmid);
   if (!lp_RM)
   {
      // EMS DTM_XATM_COMPLETEONE_FAILED
      sprintf(la_buf, "rmid %d was not found.\n", pv_rmid);
      tm_log_write(DTM_XATM_COMPLETEONE_FAILED, SQ_LOG_CRIT, la_buf);
      XATrace(XATM_TraceError,("XATM: complete_one %s", la_buf));
      abort();
   }

   // Check that there is at least one message outstanding.
   if (lp_RM->msgList()->size() == 0)
   {
      // EMS DTM_XATM_COMPLETEONE_FAILED
      sprintf(la_buf, "rmid %d has no outstanding messages.\n", pv_rmid);
      tm_log_write(DTM_XATM_COMPLETEONE_FAILED, SQ_LOG_CRIT, la_buf);
      XATrace(XATM_TraceError,("XATM: complete_one %s", la_buf));
      abort();
   }

   // Wait forever for a completion
   lv_event = XWAIT(LDONE, -1);

   // It's possible for us to have multiple outstanding requests
   // to the same RM (XARM, so DTM TM process) complete.
   do
   {
      lv_ret = BMSG_LISTEN_((short *) &lv_sre, BLISTEN_ALLOW_LDONEM, 0);

      if (lv_ret == BSRETYPE_LDONE)
      {
         // The sre_linkTag contains the RMID.
         int lv_rmid = (int) lv_sre.sre_linkTag; //intentional truncation on 64 bit patforms.
         XATrace(XATM_TraceDetail, ("XATM: complete_one rmid=%d, msgid=%d\n", 
                 lv_rmid, lv_sre.sre_msgId));

         //TODO: need to wait for the reply associated with the
         // specified pp_handle (msgid) to arrive, saving any other
         // completions.
         if ((lv_rmid != pv_rmid) || 
             (*pp_handle != 0 && lv_sre.sre_msgId != *pp_handle))
         {
            // EMS DTM_XATM_COMPLETEONE_FAILED
            sprintf(la_buf, "Listen completed for unknown tag.  This "
                     "should match the rmid and handle passed in. Expected "
                     "rmid %d, handle %d, but Listen completed for rmid %d, "
                     "handle %d.\n",
                     pv_rmid, *pp_handle, lv_rmid, lv_sre.sre_msgId);
            tm_log_write(DTM_XATM_COMPLETEONE_FAILED, SQ_LOG_CRIT, la_buf);
            XATrace(XATM_TraceError,("XATM: complete_one: %s\n", la_buf));
            abort();
         }

         // Look up message in RMs outstanding messages list
         CxaTM_RMMessage *lp_msg = (CxaTM_RMMessage *) lp_RM->msgList()->get(lv_sre.sre_msgId);
         if (lp_msg == NULL)
         {
            // EMS DTM_XATM_COMPLETEONE_FAILED
            sprintf(la_buf, "ERROR. rmid=%d, msgid=%d not found in RM "
                    "outstanding message list.\n",
                    lv_rmid, lv_sre.sre_msgId);
            tm_log_write(DTM_XATM_COMPLETEONE_FAILED, SQ_LOG_CRIT, la_buf);
            XATrace(XATM_TraceError,("XATM: complete_one: %s\n", la_buf));
            abort();
         }
         else
         {
            lv_xaError = XA_OK;
            lp_RM->lock(); //Lock the RM to make sure no one else can change the msgList before we do.

               lv_ret2 = BMSG_BREAK_(lp_msg->msgid(), 
                                 la_results,
                                 lp_RM->getRmPhandle());

            XATrace(XATM_TraceSpecial, ("XATM: complete_one BMSG_BREAK_ completed with %ld, msgid %d, rmid %d, sendAttempts %d\n",
                     lv_ret2, lp_msg->msgid(), lp_RM->getRmid(), lp_msg->sendAttempts()));

            // If break returns an error we retry the link - resend the message to the 
            // RM.
            lv_retryLink = lp_msg->checkError(lv_ret2, &lv_xaError);

            if (lv_retryLink == false)
            {
               /////////////////////////////////////////////////////  
               // deal with response here                         //
               /////////////////////////////////////////////////////
               int32 lv_type = lp_msg->getRmReplyType();
                  
               if (lv_xaError != XA_OK)
               {
                  XATrace(XATM_TraceError,("XATM: complete_one rmid=%d replied "
                        "to message type %d with XA error %s\n",
                        lv_rmid, lv_type, 
                        XAtoa(lv_xaError)));
               }

               // release the message object
               if (lp_RM != NULL)
               {
                  gv_xaTM.release_RMmsg(lp_msg);
                  lp_RM->unlock();
               }
            }  ////////////////////////////////////////////////////
               // End dealing with response                      //
               ////////////////////////////////////////////////////
            else
            {
                // delay, retry link, loop back and wait for another LDONE completion
                if (lp_RM)
                {
                   if (lp_msg)
                   {
                      // We reuse the RM message object, so only remove it from the RMs msgList.
                      lp_msg->release_from_RMmsgList();
                      lv_retryLink = lp_msg->retrySend();
                   }
                   lp_RM->unlock();
                }
                lv_retryLink = false;
            }
         } // else found message object, call BREAK
      } //if LDONE
      else
      {
         XATrace(XATM_TraceError, ("XATM: complete_one ERROR: Listen didn't "
                  "return LDONE! rmid=%d, Listen returned=%ld\n",
                  pv_rmid, lv_ret));
      }
   } while (lv_ret == BSRETYPE_LDONE);

   XATrace(XATM_TraceXAAPI,("XATM: complete_one EXIT rmid %d, handle %d, returning %s.\n",
           pv_rmid, *pp_handle, XAtoa(lv_xaError)));

   return lv_xaError;
} // complete_one
