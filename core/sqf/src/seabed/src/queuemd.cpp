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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mstrace.h"
#include "queuemd.h"

#include "seabed/trace.h"

#ifndef USE_SB_INLINE
#include "queuemd.inl"
#endif

// SB_Ts_Md_Queue constructor
SB_Ts_Md_Queue::SB_Ts_Md_Queue(int         pv_qid,
                               const char *pp_name)
: SB_Ts_D_Queue(pv_qid, pp_name) {
}

//
// Print queue
//
char *SB_Ts_Md_Queue::printbuf(char *pp_buf) {
    MS_Md_Type *lp_md;
    int         lv_id;
    int         lv_inx;

    lock();
    sprintf(pp_buf, "this=%p(%s), qid=%d, size=%d\n", pfp(this),
            ia_d_q_name, iv_qid, iv_count);
    pp_buf += strlen(pp_buf);
    SB_DQL_Type *lp_item = ip_head;
    for (lv_inx = 0; lp_item != NULL; lp_item = lp_item->ip_next) {
        lp_md = reinterpret_cast<MS_Md_Type *>(lp_item);
        lv_id = lp_md->iv_link.iv_id.i;
        sprintf(pp_buf, "  inx=%d, i=%p, p=%p, n=%p, msgid=%d, md=%p\n",
                lv_inx,
                pfp(lp_item),
                pfp(lp_item->ip_prev),
                pfp(lp_item->ip_next),
                lv_id,
                pfp(lp_md));
        pp_buf += strlen(pp_buf);
        lv_inx++;
    }
    unlock();
    *pp_buf = '\0';
    return pp_buf;
}

//
// Print queue
//
void SB_Ts_Md_Queue::printself(bool pv_traverse, bool pv_lock) {
    MS_Md_Type *lp_md;
    int         lv_id;
    int         lv_inx;

    if (pv_lock)
        lock();
    printf("this=%p(%s), qid=%d, size=%d\n", pfp(this),
           ia_d_q_name, iv_qid, iv_count);
    if (pv_traverse) {
        SB_DQL_Type *lp_item = ip_head;
        for (lv_inx = 0; lp_item != NULL; lp_item = lp_item->ip_next) {
            lp_md = reinterpret_cast<MS_Md_Type *>(lp_item);
            lv_id = lp_md->iv_link.iv_id.i;
            printf("  inx=%d, i=%p, p=%p, n=%p, msgid=%d, md=%p\n",
                   lv_inx,
                   pfp(lp_item),
                   pfp(lp_item->ip_prev),
                   pfp(lp_item->ip_next),
                   lv_id,
                   pfp(lp_md));
            lv_inx++;
        }
    }
    if (pv_lock)
        unlock();
}

//
// accounting
//
void SB_Ts_Md_Queue::remove_account(void *pp_item) {
    const char *WHERE = "SB_Ts_Md_Queue::remove_account";
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

// SB_Sig_Md_Queue constructor
SB_Sig_Md_Queue::SB_Sig_Md_Queue(int         pv_qid,
                                 const char *pp_name,
                                 bool        pv_multi_reader)
: SB_Sig_D_Queue(pv_qid, pp_name, pv_multi_reader) {
}

