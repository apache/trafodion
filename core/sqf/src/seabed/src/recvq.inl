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

#include "mstrace.h"
#include "msutil.h"
#include "streamutil.h"

//
// Add item
//
SB_INLINE void SB_Recv_Queue::add(SB_DQL_Type *pp_item) {
    const char   *WHERE = "SB_Recv_Queue::add";
    MS_Md_Type   *lp_list_md;
    MS_Md_Type   *lp_md;
    Mon_Msg_Type *lp_mon_msg;
    const char   *lp_mon_text1;
    const char   *lp_mon_text2;
    char         *lp_name;
    int           lv_pri;

    lp_md = reinterpret_cast<MS_Md_Type *>(pp_item);
    // Raise the priority, this is different from NSK.
    if (iv_pri_q && (lp_md->out.iv_pri == 0))
        lp_md->out.iv_pri = 10;
    lv_pri = lp_md->out.iv_pri;
    if (gv_ms_trace_params) {
        gettimeofday(&lp_md->out.iv_recv_q_on_tod, NULL);
        lp_name = sb_stream_get_name(lp_md);
        if (lp_md->out.iv_mon_msg) {
            lp_mon_msg =
              reinterpret_cast<Mon_Msg_Type *>(lp_md->out.ip_recv_data);
            lp_mon_text1 = ", mon-msg-";
            lp_mon_text2 = msg_util_get_msg_type(lp_mon_msg->type);
        } else {
            lp_mon_text1 = "";
            lp_mon_text2 = "";
        }
        trace_where_printf(WHERE, "adding to recv-q qid=%d(%s), from=%s, reqid=%d, msgid=%d, md=%p, pri=%d%s%s\n",
                           iv_qid, ia_d_q_name,
                           lp_name, lp_md->out.iv_recv_req_id,
                           lp_md->iv_link.iv_id.i, pfp(lp_md), lv_pri,
                           lp_mon_text1, lp_mon_text2);
    }
    if (iv_pri_q) {
        lock();
        if (ip_head == NULL)
            SB_D_Queue::add(&lp_md->iv_link);
        else {
            lp_list_md = reinterpret_cast<MS_Md_Type *>(ip_tail);
            while ((lp_list_md != reinterpret_cast<MS_Md_Type *>(ip_head)) &&
                   (lp_list_md->out.iv_pri >= 0) &&
                   (lv_pri > lp_list_md->out.iv_pri)) {
                // step over a queued request, priority is not upped, this is different from NSK
                lp_list_md = reinterpret_cast<MS_Md_Type *>(lp_list_md->iv_link.ip_prev);
            }
            // check if should be at head
            if ((lp_list_md == reinterpret_cast<MS_Md_Type *>(ip_head)) &&
                (lp_list_md->out.iv_pri >= 0) &&
                (lv_pri > lp_list_md->out.iv_pri)) {
                // step over a queued request, priority is not upped, this is different from NSK
                SB_D_Queue::add_at_front(&lp_md->iv_link);
            } else
                SB_D_Queue::add_list(&lp_list_md->iv_link, &lp_md->iv_link);
        }
        unlock();
    } else
        SB_Ts_D_Queue::add(pp_item);
}

//
// Add item at head
//
SB_INLINE void SB_Recv_Queue::add_at_front(SB_DQL_Type *pp_item) {
    const char   *WHERE = "SB_Recv_Queue::add_at_front";
    MS_Md_Type   *lp_md;
    Mon_Msg_Type *lp_mon_msg;
    const char   *lp_mon_text1;
    const char   *lp_mon_text2;
    char         *lp_name;
    int           lv_pri;

    lp_md = reinterpret_cast<MS_Md_Type *>(pp_item);
    lv_pri = lp_md->out.iv_pri;
    if (gv_ms_trace_params) {
        gettimeofday(&lp_md->out.iv_recv_q_on_tod, NULL);
        lp_name = sb_stream_get_name(lp_md);
        if (lp_md->out.iv_mon_msg) {
            lp_mon_msg =
              reinterpret_cast<Mon_Msg_Type *>(lp_md->out.ip_recv_data);
            lp_mon_text1 = ", mon-msg-";
            lp_mon_text2 = msg_util_get_msg_type(lp_mon_msg->type);
        } else {
            lp_mon_text1 = "";
            lp_mon_text2 = "";
        }
        trace_where_printf(WHERE, "adding to front recv-q qid=%d(%s), from=%s, reqid=%d, msgid=%d, md=%p, pri=%d%s%s\n",
                           iv_qid, ia_d_q_name,
                           lp_name, lp_md->out.iv_recv_req_id,
                           lp_md->iv_link.iv_id.i, pfp(lp_md), lv_pri,
                           lp_mon_text1, lp_mon_text2);
    }
    SB_Ts_D_Queue::add_at_front(pp_item);
}

//
// Lock queue (can be called by client)
//
SB_INLINE void SB_Recv_Queue::lock() {
    SB_Ts_D_Queue::lock();
}

//
// Remove item
//
SB_INLINE void *SB_Recv_Queue::remove() {
    void *lp_ret;

    lp_ret = SB_Ts_D_Queue::remove();
    if (lp_ret != NULL) {
        if (gv_ms_trace_params)
            remove_account(lp_ret);
    }
    return lp_ret;
}

//
// Remove from list
//
SB_INLINE bool SB_Recv_Queue::remove_list(SB_DQL_Type *pp_item) {
    bool lv_ret;

    lv_ret = SB_Ts_D_Queue::remove_list(pp_item);
    if (lv_ret) {
        if (gv_ms_trace_params)
            remove_account(pp_item);
    }
    return lv_ret;
}

SB_INLINE void SB_Recv_Queue::set_priority_queue(bool pv_pri_q) {
    iv_pri_q = pv_pri_q;
}

//
// Unlock queue (can be called by client)
//
SB_INLINE void SB_Recv_Queue::unlock() {
    SB_Ts_D_Queue::unlock();
}

