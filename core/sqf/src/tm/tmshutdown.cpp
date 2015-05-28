// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

#include <stdlib.h>
#include "dtm/tm.h"
#include "tmshutdown.h"
#include "tminfo.h"
#include "dtm/xa.h"
#include "tmrm.h"
#include "tmaudit.h"
#include "tmlogging.h"
#include "tmregistry.h"

// seabed includes
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/trace.h"
#include "common/sq_common.h"


// TM_Shutdown Constructor
TMShutdown::TMShutdown(TM_Info *pp_tm_info, RM_Info_TSEBranch *pp_rm_info)
{
    if ((pp_tm_info == NULL) || (pp_rm_info == NULL))
    {
        tm_log_event(DTM_SHTDWN_INVALID_TM_INFO, SQ_LOG_CRIT, "DTM_SHTDWN_INVALID_TM_INFO");
        TMTrace(1, ("TMShutdown::TMShutdown - Invalid TM or RM info"));
        abort ();
    }

   ip_tm_info = pp_tm_info;
   ip_rm_info = pp_rm_info;
}

// TM_Shutdown Destructor
TMShutdown::~TMShutdown() {}

void TMShutdown::coordinate_shutdown()
{
   int32    lv_tm_error = FEOK;
   int32    lv_rm_error = FEOK;
   bool     lv_clean_shutdown = false;

   TMTrace(2, ("TMShutdown::coordinate_shutdown: ENTRY\n"));
   if (!ip_tm_info->lead_tm())
   {
      lv_rm_error = close_all_rms_for_shutdown(false /*non-leadTM*/, true /*clean*/);

      // For a non-lead TM, all shutdown steps have been
      // performed except replying to the lead TM's inquiry 
      // message for shutdown completion.  Return to the
      // main loop in tm.cpp to wait for the lead TM's message
      // or possible system message of lead TM failure.
      if (lv_rm_error == FEOK)
         ip_tm_info->state(TM_STATE_SHUTDOWN_COMPLETED);
      else
         ip_tm_info->state(TM_STATE_SHUTDOWN_FAILED);
   }
   else
   {  //I'm leadTM
      do
      {
         lv_tm_error = send_shutdown_msg_to_opened_TMs();  
         if (lv_tm_error == FETMSHUTDOWN_NOTREADY)
         {
            XWAIT(0, 4*TM_SHUTDOWN_WAKEUP_INTERVAL);
         }
      } while (lv_tm_error == FETMSHUTDOWN_NOTREADY);

      lv_rm_error = close_all_rms_for_shutdown(true /*leadTM*/, (lv_tm_error == FEOK) ? true : false);

      if ((lv_tm_error == FEOK) && (lv_rm_error == FEOK))
      {
         lv_clean_shutdown = true;
         // force write shutdown audit record
         ip_tm_info->write_shutdown();
         tm_log_event(DTM_SHUTDOWN_CLEAN, SQ_LOG_INFO, "DTM_SHUTDOWN_CLEAN");
      }
      else
      {
         tm_log_event(DTM_SHUTDOWN_DIRTY, SQ_LOG_WARNING, "DTM_SHUTDOWN_DIRTY", lv_tm_error,
              -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,lv_rm_error);
      }

      // Shutting down, do NOT want any interval timers to pop now
      gv_tm_info.tmTimer()->cancelControlpointEvent();
      gv_tm_info.tmTimer()->cancelStatsEvent();
      gv_tm_info.tmTimer()->cancelRMRetryEvent();

      XWAIT(0, 4*TM_SHUTDOWN_WAKEUP_INTERVAL);

      // send SQ_AUDIT_SHUTDOWN message to AMP and wait for reply
      send_shutdown_msg_to_AMP();

      XWAIT(0, 4*TM_SHUTDOWN_WAKEUP_INTERVAL);

      if (lv_clean_shutdown)
      {
         ip_tm_info->state(TM_STATE_SHUTDOWN_COMPLETED);
         gv_tm_info.set_txnsvc_ready(TXNSVC_DOWN);
         TMTrace(1, ("$TM%d shutting down clean.\n",gv_tm_info.nid()));
         msg_mon_process_shutdown();
      }
      else {
         ip_tm_info->state(TM_STATE_SHUTDOWN_FAILED);
         TMTrace(1, ("$TM%d shutdown failed.\n",gv_tm_info.nid()));
      }

      TMTrace(1, ("$TM%d exiting. TM state %d.\n",
              gv_tm_info.nid(), ip_tm_info->state()));
      exit(0);
   }
}

int32 TMShutdown::close_all_rms_for_shutdown(bool pv_leadTM, bool pv_clean)
{
   int32            lv_fatal_error = FEOK;
   TMTrace(2, ("TMShutdown::close_all_rms_for_shutdown: ENTRY\n"));
  
   gv_RMs.shutdown_branches(pv_leadTM, pv_clean);
   ip_tm_info->all_rms_closed(true);

   TMTrace(2, ("TMShutdown::close_all_rms_for_shutdown: EXIT\n"));
   return lv_fatal_error;
}

// -----------------------------------------------------------------
// send_shutdown_msg_to_opened_TMs
// Purpose: This function is invoked only by the lead TM.  Send a 
//          SHUTDOWN_COMPLETE message to each open TM and check their replies.
// -----------------------------------------------------------------
int32 TMShutdown::send_shutdown_msg_to_opened_TMs()  
{
   short          la_results[6];
   int32          lv_error = FEOK;
   static int32   lv_fatal_error = FEOK; // Persists over calls!!
   int32          lv_index = 0; 
   int32          lv_not_ready_error = FEOK;
   int32          lv_num_sent = 0;
   int32          lv_reqLen = 0;
   int32          lv_rspLen = 0;
   int            lv_rsp_rcvd = 0;
   long           lv_ret;
   long           lv_ret2;
   BMS_SRE_LDONE  lv_sre;

   TMTrace(2, ("TMShutdown::send_shutdown_msg_to_opened_TMs: ENTRY\n"));

   Tm_Shutdown_Msg_Info *lp_shutdown_msg_info = new Tm_Shutdown_Msg_Info[MAX_NODES];

   lv_reqLen = sizeof (Tm_Shutdown_Req_Type);
   lv_rspLen = sizeof (Tm_Shutdown_Rsp_Type);

   for (int lv_idx = 0; lv_idx <= ip_tm_info->tms_highest_index_used(); lv_idx++)
   {
      lp_shutdown_msg_info[lv_idx].ip_phandle = ip_tm_info->get_opened_tm_phandle(lv_idx);
      if (lp_shutdown_msg_info[lv_idx].ip_phandle != NULL)
      {
         lp_shutdown_msg_info[lv_idx].iv_tag = lv_idx + 1;
         lp_shutdown_msg_info[lv_idx].iv_req.iv_msg_hdr.rr_type.request_type = 
                                                                 TM_MSG_TYPE_SHUTDOWN_COMPLETE;  
         lp_shutdown_msg_info[lv_idx].iv_req.iv_msg_hdr.version.request_version = 
                                                                 TM_SQ_MSG_VERSION_CURRENT;
         lp_shutdown_msg_info[lv_idx].iv_nid = lv_idx;

         lv_error = ip_tm_info->link(lp_shutdown_msg_info[lv_idx].ip_phandle,   // phandle, 
                                     &lp_shutdown_msg_info[lv_idx].iv_msgid,         // msgid
                                     (char *) &lp_shutdown_msg_info[lv_idx].iv_req,  // reqdata
                                     lv_reqLen,                                      // reqdatasize
                                     (char *) &lp_shutdown_msg_info[lv_idx].iv_rsp,  // replydata
                                     lv_rspLen,                                      // replydatamax
                                     lp_shutdown_msg_info[lv_idx].iv_tag,            // linkertag
                                     TM_TM_LINK_PRIORITY,                            // pri
                                     BMSG_LINK_LDONEQ,                               // linkopts
                                     TM_LINKRETRY_RETRIES);

         if (lv_error != FEOK)
         {
            lv_fatal_error = lv_error;
            tm_log_event(DTM_MSG_TO_DTM_FAILED, SQ_LOG_CRIT, "DTM_MSG_TO_DTM_FAILED", lv_fatal_error);
            TMTrace(1, ("TM_Shutdown::send_shutdown_msg_to_opened_TMs : BMSG_LINK error = %d \n",
                    lv_fatal_error));
            // An error during shutdown, so shutdown abrupt now!
            ip_tm_info->error_shutdown_abrupt(lv_error);
         }
         else
            lv_num_sent++;
      } // ip_phandle != NULL
   } // for

   // LDONE LOOP
   while (lv_rsp_rcvd < lv_num_sent)
   {
      // wait for an LDONE wakeup 
      XWAIT(LDONE, -1);
 
      do {
         // we've reached our message reply count, break
         if (lv_rsp_rcvd >= lv_num_sent)
            break;

         lv_ret = BMSG_LISTEN_((short *)&lv_sre, BLISTEN_ALLOW_LDONEM, 0);

         if (lv_ret == BSRETYPE_LDONE)
         {
            lv_index = -1;
            for (int32 lv_idx2 = 0; lv_idx2 <= ip_tm_info->tms_highest_index_used(); lv_idx2++)
            {
               if (lp_shutdown_msg_info[lv_idx2].iv_tag == lv_sre.sre_linkTag)
               {
                  lv_index = lv_idx2;
                  break;
               }
            } // for

            // We have an LDONE completion other than one of the shutdown replies. Drop it
            // as we're exiting and don't care
            if (lv_index == -1)
            {
                tm_log_event(DTM_SHTDWN_INVALID_LINKTAG, SQ_LOG_WARNING, "DTM_SHTDWN_INVALID_LINKTAG", 
                    FENOTFOUND, lv_sre.sre_linkTag, -1, -1, lv_sre.sre_msgId);
                TMTrace(1, ("send_shutdown_msg_to_opened_TMs - Uncompleted Link. Tag %d, msgid %d\n",
                    (int)lv_sre.sre_linkTag, lv_sre.sre_msgId));
                BMSG_ABANDON_(lv_sre.sre_msgId);
            } 
            else
            {
               lv_ret2 = BMSG_BREAK_(lp_shutdown_msg_info[lv_index].iv_msgid, la_results,
                                     lp_shutdown_msg_info[lv_index].ip_phandle);
               if (lv_ret2 != FEOK)
               {
                  lv_fatal_error = lv_ret;
                  tm_log_event(DTM_MSG_TO_DTM_FAILED, SQ_LOG_CRIT, "DTM_MSG_TO_DTM_FAILED", lv_fatal_error);
                  TMTrace(1, ("TM_Shutdown::send_shutdown_msg_to_opened_TMs : BMSG_BREAK error = %d \n",
                          lv_fatal_error));
               }

               switch (lp_shutdown_msg_info[lv_index].iv_rsp.iv_error)
               {
                  case FEOK:  
                  {
                     ip_tm_info->close_tm(lv_index);
                     break;
                  }
                  case FETMSHUTDOWN_NOTREADY:
                  {
                     lv_not_ready_error = FETMSHUTDOWN_NOTREADY;
                     break;
                  }
                  default:
                  {
                     ip_tm_info->close_tm(lv_index);
                     lv_fatal_error = lp_shutdown_msg_info[lv_index].iv_rsp.iv_error;
                     // Can't return this error as it's not handled by the 
                     // caller - shutdown abrupt now!
                     tm_log_event(DTM_MSG_TO_DTM_FAILED2, SQ_LOG_CRIT, "DTM_MSG_TO_DTM_FAILED2", lv_fatal_error);
                     ip_tm_info->error_shutdown_abrupt(lv_fatal_error);
                     break;
                  }
               } // switch
            lv_rsp_rcvd++;
            } // else found corresponding link
         } // if LDONE
      } while (lv_ret == BSRETYPE_LDONE); 
   } // while (lv_rsp_rcvd < lv_num_sent)

   delete []lp_shutdown_msg_info;

   TMTrace(2, ("TMShutdown::send_shutdown_msg_to_opened_TMs: EXIT\n"));
   if (lv_not_ready_error != FEOK)
      return lv_not_ready_error;
   else if (lv_fatal_error != FEOK)
      return lv_fatal_error;
   else
      return FEOK;
}


// Returns true if shutdown message was successfully sent to the AMP
//         false if there was a problem.
bool TMShutdown::send_shutdown_msg_to_AMP()
{
   char           lv_amp_pname[MS_MON_MAX_PROCESS_NAME];
   int32          lv_error;
   int32          lv_msgid;
   int32          lv_oid;
   TPT_DECL      (lv_phandle);
   short          la_results[6];
   long           lv_ret;
   BMS_SRE_LDONE  lv_sre;
   int32          lv_retries = 0;

   TMTrace(2, ("TMShutdown::send_shutdown_msg_to_AMP: ENTRY\n"));
   Tm_Shutdown_Req_Type *lp_req = new Tm_Shutdown_Req_Type();
   Tm_Shutdown_Rsp_Type *lp_rsp = new Tm_Shutdown_Rsp_Type();

   lp_req->iv_msg_hdr.dialect_type = DIALECT_TM_AMP_SQ;
   lp_req->iv_msg_hdr.rr_type.request_type = TM_MSG_TYPE_SHUTDOWN_AUDIT;
   lp_req->iv_msg_hdr.version.request_version = TM_SQ_MSG_VERSION_CURRENT;
   lp_rsp->iv_msg_hdr.rr_type.reply_type = TM_MSG_TYPE_SHUTDOWN_AUDIT_REPLY;

   memset(lv_amp_pname, 0, MS_MON_MAX_PROCESS_NAME);
   lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                  (char *) CLUSTER_GROUP, (char *) AUDIT_MGMT_PROC, lv_amp_pname);
           
   // If the AUDIT_MGMT_PROC registry value is not found, spit out an error but 
   // continue.
   if (lv_error != 0)
   {
      tm_log_event(DTM_CANNOT_OPEN_AMP, SQ_LOG_CRIT, 
                 "DTM_AUDIT_MGMT_PROC_REG_VALUE_NOT_FOUND");
      TMTrace(1, ("TM_Shutdown::send_shutdown_msg_to_AMP : Cannot open AMP. "
                 "AUDIT_MGMT_PROC registry value not found.\n"));
      delete lp_req;
      delete lp_rsp;
      return false;
   }

   lv_error = msg_mon_open_process(lv_amp_pname, &lv_phandle, &lv_oid);
   if (lv_error != FEOK)
   {
      tm_log_event(DTM_CANNOT_OPEN_AMP, SQ_LOG_CRIT, "DTM_CANNOT_OPEN_AMP");
      TMTrace(1, ("TM_Shutdown::send_shutdown_msg_to_AMP : cannot open AMP %s\n",
                lv_amp_pname));
      delete lp_req;
      delete lp_rsp;
      return false;
   }

   do
   {
      lv_error = ip_tm_info->link(&lv_phandle,                 // phandle
                                  &lv_msgid,                   // msgid
                                  (char *) lp_req,             // reqdata
                                  sizeof (Tm_Shutdown_Req_Type),  // reqdatasize
                                  (char *) lp_rsp,             // replydata
                                  sizeof (Tm_Shutdown_Rsp_Type), // replydatamax
                                  0,                           // linkertag
                                  TSE_LINK_PRIORITY,           // pri
                                  BMSG_LINK_LDONEQ,            // linkopts
                                  TM_LINKRETRY_RETRIES);

      if (lv_error != FEOK)
      {
         TMTrace(3, ("TM_Shutdown::send_shutdown_msg_to_AMP : BMSG_LINK error = %d, AMP %s, retries %d.\n",
                 lv_error, lv_amp_pname, lv_retries));
      }
      else
      {
         XWAIT(LDONE, -1);

         lv_ret = BMSG_LISTEN_((short *)&lv_sre, BLISTEN_ALLOW_LDONEM, 0);
         if (lv_ret == BSRETYPE_LDONE)
         {
            lv_error = BMSG_BREAK_(lv_msgid, la_results, &lv_phandle);
            if (lv_error != FEOK)
            {
               TMTrace(3, ("TM_Shutdown::send_shutdown_msg_to_AMP: BMSG_BREAK error = %d, AMP %s, retries %d.\n",
                       lv_error, lv_amp_pname, lv_retries));
            }
         }
         else
         {
            TMTrace(3, ("TM_Shutdown::send_shutdown_msg_to_AMP: BMSG_LISTEN completed with unexpected "
                    " return code = %ld, AMP %s, retries %d.\n", lv_ret, lv_amp_pname, lv_retries));
         }
      }
      lv_retries++;
      if (lv_error != FEOK && lv_retries <= TM_LINKRETRY_RETRIES)
         SB_Thread::Sthr::sleep(TM_LINKRETRY_PAUSE); // in msec

   } while (lv_error != FEOK && lv_retries <= TM_LINKRETRY_RETRIES);
      
   if (lv_error != FEOK)
   {
      tm_log_event(DTM_MSG_TO_AMP_FAILED, SQ_LOG_CRIT, "DTM_MSG_TO_AMP_FAILED", lv_error, 
                   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,1,-1,-1,lv_amp_pname);
      TMTrace(1, ("TM_Shutdown::send_shutdown_msg_to_AMP : Error = %d, AMP %s.\n",
              lv_error, lv_amp_pname));
   }

   delete lp_rsp;
   delete lp_req;
   TMTrace(2, ("TMShutdown::send_shutdown_msg_to_AMP: EXIT\n"));

   if (lv_error)
      return false;
   else
      return true;
}



