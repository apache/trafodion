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

#include "msi.h"
#include "mstrace.h"

//
// Add item
//
SB_INLINE void SB_Comp_Queue::add(SB_DQL_Type *pp_item) {
    const char *WHERE = "SB_Comp_Queue::add";
    MS_Md_Type *lp_md;

    lp_md = reinterpret_cast<MS_Md_Type *>(pp_item);
    if (gv_ms_trace_params) {
        gettimeofday(&lp_md->out.iv_comp_q_on_tod, NULL);
        trace_where_printf(WHERE, "adding to comp-q qid=%d(%s), msgid=%d, md=%p, md-tag=0x%lx, fserr=%d\n",
                           iv_qid, ia_d_q_name,
                           lp_md->iv_link.iv_id.i, pfp(lp_md),
                           lp_md->iv_tag,
                           lp_md->out.iv_fserr);
    }
    SB_util_assert_ieq(lp_md->iv_link.iv_qid, QID_NONE);
    SB_Ts_D_Queue::add(pp_item);
}

//
// Remove item
//
SB_INLINE void *SB_Comp_Queue::remove() {
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
SB_INLINE bool SB_Comp_Queue::remove_list(SB_DQL_Type *pp_item) {
    bool lv_ret;

    lv_ret = SB_Ts_D_Queue::remove_list(pp_item);
    if (lv_ret) {
        if (gv_ms_trace_params)
            remove_account(pp_item);
    }
    return lv_ret;
}

//
// Remove from list
//
SB_INLINE bool SB_Comp_Queue::remove_list_lock(SB_DQL_Type *pp_item,
                                               bool         pv_lock) {
    bool lv_ret;

    if (pv_lock)
        lv_ret = SB_Ts_D_Queue::remove_list(pp_item);
    else
        lv_ret = SB_D_Queue::remove_list(pp_item);
    if (lv_ret) {
        if (gv_ms_trace_params)
            remove_account(pp_item);
    }
    return lv_ret;
}

