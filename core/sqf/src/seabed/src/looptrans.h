//------------------------------------------------------------------
//
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

//
// Encapsulate transport helpers
//
#ifndef __SB_LOOPTRANS_H_
#define __SB_LOOPTRANS_H_

#include "seabed/thread.h"

#include "msi.h"
#include "transport.h"

namespace SB_Trans {
    //
    // Loopback Stream engine
    //
    class Loop_Stream : public Trans_Stream {
    public:
        static Loop_Stream *create(const char           *pp_name,
                                   const char           *pp_pname,
                                   const char           *pp_prog,
                                   bool                  pv_ic,
                                   SB_Comp_Cb_Type       pv_ms_comp_callback,
                                   SB_Ab_Comp_Cb_Type    pv_ms_abandon_callback,
                                   SB_ILOC_Comp_Cb_Type  pv_ms_oc_callback,
                                   SB_Lim_Cb_Type        pv_ms_lim_callback,
                                   SB_Recv_Queue        *pp_ms_recv_q,
                                   SB_Recv_Queue        *pp_ms_lim_q,
                                   SB_Ms_Tl_Event_Mgr   *pp_event_mgr);

        // execute functions
        virtual short      exec_abandon(MS_Md_Type *pp_md,
                                        int         pv_reqid,
                                        int         pv_can_reqid);
        virtual short      exec_abandon_ack(MS_Md_Type *pp_md,
                                            int         pv_reqid,
                                            int         pv_can_ack_reqid);
        virtual short      exec_close(MS_Md_Type *pp_md,
                                      int         pv_reqid);
        virtual short      exec_close_ack(MS_Md_Type *pp_md,
                                          int         pv_reqid);
        virtual short      exec_conn_ack(MS_Md_Type *pp_md,
                                         int         pv_reqid);
        virtual short      exec_open(MS_Md_Type *pp_md,
                                     int         pv_reqid);
        virtual short      exec_open_ack(MS_Md_Type *pp_md,
                                         int         pv_reqid);
        virtual short      exec_reply(MS_Md_Type *pp_md,
                                      int         pv_src,
                                      int         pv_dest,
                                      int         pv_reqid,
                                      short      *pp_req_ctrl,
                                      int         pv_req_ctrl_size,
                                      char       *pp_req_data,
                                      int         pv_req_data_size,
                                      short       pv_fserr);
        virtual short      exec_reply_nack(MS_Md_Type *pp_md,
                                           int         pv_reqid);
        virtual short      exec_reply_nw(MS_Md_Type *pp_md,
                                         int         pv_src,
                                         int         pv_dest,
                                         int         pv_reqid,
                                         short      *pp_req_ctrl,
                                         int         pv_req_ctrl_size,
                                         char       *pp_req_data,
                                         int         pv_req_data_size);
        virtual short      exec_wr(MS_Md_Type *pp_md,
                                   int         pv_src,
                                   int         pv_dest,
                                   int         pv_reqid,
                                   int         pv_pri,
                                   short      *pp_req_ctrl,
                                   int         pv_req_ctrl_size,
                                   char       *pp_req_data,
                                   int         pv_req_data_size,
                                   short      *pp_rep_ctrl,
                                   int         pv_rep_max_ctrl_size,
                                   char       *pp_rep_data,
                                   int         pv_rep_max_data_size,
                                   int         pv_opts);
        virtual int        start_stream();
        virtual void       stop_completions();

    protected:
        Loop_Stream(const char           *pp_name,
                    const char           *pp_pname,
                    const char           *pp_prog,
                    bool                  pv_ic,
                    SB_Comp_Cb_Type       pv_ms_comp_callback,
                    SB_Ab_Comp_Cb_Type    pv_ms_abandon_callback,
                    SB_ILOC_Comp_Cb_Type  pv_ms_oc_callback,
                    SB_Lim_Cb_Type        pv_ms_lim_callback,
                    SB_Recv_Queue        *pp_ms_recv_q,
                    SB_Recv_Queue        *pp_ms_lim_q,
                    SB_Ms_Tl_Event_Mgr   *pp_event_mgr);
        ~Loop_Stream();
        virtual void       close_this(bool pv_local, bool pv_lock, bool pv_sem);
        virtual void       comp_sends();

    private:
        void mon_init(MS_Md_Type *pp_dest_md, MS_Md_Type *pp_src_md);
    };

}

#endif // !__SB_LOOPTRANS_H_
