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
// encapsulate $RECEIVE
//

#include <stdio.h>
#include <string.h>

#include "seabed/trace.h"

#include "msi.h"
#include "mstrace.h"
#include "qid.h"
#include "recvq.h"

#ifndef USE_SB_INLINE
#include "recvq.inl"
#endif

// SB_Recv_Queue constructor
SB_Recv_Queue::SB_Recv_Queue(const char *pp_name)
: SB_Ts_Md_Queue(QID_RCV, pp_name), iv_aecid_recv_queue(SB_ECID_QUEUE_RECV), iv_pri_q(false) {
}

//
// Print queue
//
char *SB_Recv_Queue::printbuf(char *pp_buf) {
    SB_DQL_Type *lp_item;
    MS_Md_Type  *lp_md;
    int          lv_inx;
    int          lv_msgid;
    int          lv_pri;

    lock();
    sprintf(pp_buf, "this=%p(%s), size=%d\n", pfp(this),
           ia_d_q_name, iv_count);
    pp_buf += strlen(pp_buf);
    lp_item = ip_head;
    for (lv_inx = 0; lp_item != NULL; lv_inx++) {
        lp_md = reinterpret_cast<MS_Md_Type *>(lp_item);
        lv_msgid = lp_md->iv_link.iv_id.i;
        lv_pri = lp_md->out.iv_pri;
        sprintf(pp_buf,
                "  inx=%d, i=%p, p=%p, n=%p, msgid=%d, md=%p, reqid=%d, pri=%d\n",
                lv_inx,
                pfp(lp_item),
                pfp(lp_item->ip_prev),
                pfp(lp_item->ip_next),
                lv_msgid,
                pfp(lp_md),
                lp_md->out.iv_recv_req_id,
                lv_pri);
        pp_buf += strlen(pp_buf);
        lp_item = lp_item->ip_next;
    }
    unlock();
    *pp_buf = '\0';
    return pp_buf;
}

//
// Print queue
//
void SB_Recv_Queue::printself(bool pv_traverse, bool pv_lock) {
    SB_DQL_Type *lp_item;
    MS_Md_Type  *lp_md;
    int          lv_inx;
    int          lv_msgid;
    int          lv_pri;

    if (pv_lock)
        lock();
    printf("this=%p(%s), size=%d\n", pfp(this),
           ia_d_q_name, iv_count);
    if (pv_traverse) {
        lp_item = ip_head;
        for (lv_inx = 0; lp_item != NULL; lv_inx++) {
            lp_md = reinterpret_cast<MS_Md_Type *>(lp_item);
            lv_msgid = lp_md->iv_link.iv_id.i;
            lv_pri = lp_md->out.iv_pri;
            printf("  inx=%d, i=%p, p=%p, n=%p, msgid=%d, md=%p, reqid=%d, pri=%d\n",
                   lv_inx,
                   pfp(lp_item),
                   pfp(lp_item->ip_prev),
                   pfp(lp_item->ip_next),
                   lv_msgid,
                   pfp(lp_md),
                   lp_md->out.iv_recv_req_id,
                   lv_pri);
            lp_item = lp_item->ip_next;
        }
    }
    if (pv_lock)
        unlock();
}

//
// accounting
//
void SB_Recv_Queue::remove_account(void *pp_item) {
    const char *WHERE = "SB_Recv_Queue::remove_account";
    MS_Md_Type *lp_md;
    long long   lv_delta;

    lp_md = static_cast<MS_Md_Type *>(pp_item);
    gettimeofday(&lp_md->out.iv_recv_q_off_tod, NULL);
    lv_delta = (lp_md->out.iv_recv_q_off_tod.tv_sec * SB_US_PER_SEC +
                lp_md->out.iv_recv_q_off_tod.tv_usec) -
               (lp_md->out.iv_recv_q_on_tod.tv_sec * SB_US_PER_SEC +
                lp_md->out.iv_recv_q_on_tod.tv_usec);
    trace_where_printf(WHERE,
                       "recv-q-to-de-queue (qid=%d(%s), msgid=%d, md=%p, md-tag=0x%lx) time=%lld us\n",
                       iv_qid, ia_d_q_name,
                       lp_md->iv_link.iv_id.i, pfp(lp_md),
                       lp_md->iv_tag,
                       lv_delta);
}

