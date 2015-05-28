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

#ifndef TMSHUTDOWN_H_
#define TMSHUTDOWN_H_

#include "tmtx.h"

#define TM_SHUTDOWN_WAKEUP_INTERVAL    25  // in 10 milliseconds units      

typedef struct _tmshutdown_h_as_0 {
   TPT_PTR( ip_phandle);
   int32 iv_tag;
   int32 iv_nid;
   int32 iv_msgid;
   Tm_Shutdown_Req_Type iv_req;
   Tm_Shutdown_Rsp_Type iv_rsp;
} Tm_Shutdown_Msg_Info;

class TMShutdown
{
   public:
      TMShutdown(TM_Info *pp_tm_info, RM_Info_TSEBranch *pp_rm_info);

      ~TMShutdown();
      
      void coordinate_shutdown();
       
   private:
      TM_Info              *ip_tm_info;
      RM_Info_TSEBranch    *ip_rm_info;

      void        drain_all_txs();
      void        wait_for_all_txs_to_end();
      void        abort_all_active_txs();
      int32       close_all_rms_for_shutdown(bool pv_leadTM, bool pv_clean);
      int32       send_shutdown_msg_to_opened_TMs();  
      bool        send_shutdown_msg_to_AMP();
      void        stop_all_other_TMs();
      void        stop_all_TSEs();
      void        stop_AMP();
};

#endif
