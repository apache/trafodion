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
// encapsulate completion-queue
//
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <linux/unistd.h>

#include "seabed/trace.h"

#include "compq.h"
#include "msi.h"
#include "mstrace.h"
#include "qid.h"

#ifndef USE_SB_INLINE
#include "compq.inl"
#endif

#define gettid() static_cast<pid_t>(syscall(__NR_gettid))

// SB_Comp_Queue constructor
SB_Comp_Queue::SB_Comp_Queue(int         pv_qid,
                             const char *pp_name)
: SB_Ts_Md_Queue(pv_qid, pp_name) {
    iv_tid = gettid();
}

//
// Print queue
//
char *SB_Comp_Queue::printbuf(char *pp_buf) {
    MS_Md_Type *lp_md;
    int         lv_inx;

    lock();
    sprintf(pp_buf, "this=%p(%s), qid=%d, size=%d, tid=%d\n", pfp(this),
           ia_d_q_name, iv_qid, iv_count, iv_tid);
    pp_buf += strlen(pp_buf);
    SB_DQL_Type *lp_item = ip_head;
    for (lv_inx = 0; lp_item != NULL; lv_inx++) {
        lp_md = reinterpret_cast<MS_Md_Type *>(lp_item);
        sprintf(pp_buf, "  inx=%d, p=%p, n=%p, msgid=%d, md=%p\n",
                lv_inx,
                pfp(lp_item->ip_prev),
                pfp(lp_item->ip_next),
                lp_md->iv_link.iv_id.i,
                pfp(lp_md));
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
void SB_Comp_Queue::printself(bool pv_traverse, bool pv_lock) {
    MS_Md_Type *lp_md;
    int         lv_inx;

    if (pv_lock)
        lock();
    printf("this=%p(%s), qid=%d, size=%d\n", pfp(this),
           ia_d_q_name, iv_qid, iv_count);
    if (pv_traverse) {
        SB_DQL_Type *lp_item = ip_head;
        for (lv_inx = 0; lp_item != NULL; lv_inx++) {
            lp_md = reinterpret_cast<MS_Md_Type *>(lp_item);
            printf("  inx=%d, p=%p, n=%p, msgid=%d, md=%p\n",
                   lv_inx,
                   pfp(lp_item->ip_prev),
                   pfp(lp_item->ip_next),
                   lp_md->iv_link.iv_id.i,
                   pfp(lp_md));
            lp_item = lp_item->ip_next;
        }
    }
    if (pv_lock)
        unlock();
}

//
// accounting
//
void SB_Comp_Queue::remove_account(void *pp_item) {
    const char *WHERE = "SB_Comp_Queue::remove_account";
    MS_Md_Type *lp_md;
    long long   lv_delta;

    lp_md = static_cast<MS_Md_Type *>(pp_item);
    gettimeofday(&lp_md->out.iv_comp_q_off_tod, NULL);
    lv_delta = (lp_md->out.iv_comp_q_off_tod.tv_sec * SB_US_PER_SEC +
                lp_md->out.iv_comp_q_off_tod.tv_usec) -
               (lp_md->out.iv_comp_q_on_tod.tv_sec * SB_US_PER_SEC +
                lp_md->out.iv_comp_q_on_tod.tv_usec);
    trace_where_printf(WHERE,
                       "comp-q-to-de-queue qid=%d(%s), (msgid=%d, md=%p, md-tag=0x%lx) time=%lld us\n",
                       iv_qid, ia_d_q_name,
                       lp_md->iv_link.iv_id.i, pfp(lp_md),
                       lp_md->iv_tag,
                       lv_delta);
}

