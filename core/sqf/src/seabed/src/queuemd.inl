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

#ifndef __SB_QUEUEMD_INL_
#define __SB_QUEUEMD_INL_

#include "mstrace.h"

// SB_Ts_Md_Queue destructor
SB_INLINE SB_Ts_Md_Queue::~SB_Ts_Md_Queue() {
}

//
// Add item
//
SB_INLINE void SB_Ts_Md_Queue::add(SB_DQL_Type *pp_item) {
    const char *WHERE = "SB_Ts_Md_Queue::add";
    MS_Md_Type *lp_md;

    lp_md = reinterpret_cast<MS_Md_Type *>(pp_item);
    if (gv_ms_trace_params) {
        gettimeofday(&lp_md->out.iv_comp_q_on_tod, NULL);
        trace_where_printf(WHERE, "adding to comp-q qid=%d(%s), msgid=%d, md=%p\n",
                           iv_qid, ia_d_q_name,
                           lp_md->iv_link.iv_id.i, pfp(lp_md));
    }
    SB_Ts_D_Queue::add(pp_item);
}

SB_INLINE void SB_Ts_Md_Queue::add_at_front(SB_DQL_Type *pp_item) {
    SB_Ts_D_Queue::add_at_front(pp_item);
}

SB_INLINE void SB_Ts_Md_Queue::add_list(SB_DQL_Type *pp_prev, SB_DQL_Type *pp_item) {
    SB_Ts_D_Queue::add_list(pp_prev, pp_item);
}

SB_INLINE SB_DQL_Type *SB_Ts_Md_Queue::head() {
    SB_DQL_Type *lp_head;

    lp_head = reinterpret_cast<SB_DQL_Type *>(SB_Ts_D_Queue::head());
    return lp_head;
}

//
// Lock queue (can be called by client)
//
SB_INLINE void SB_Ts_Md_Queue::lock() {
    SB_Ts_D_Queue::lock();
}

//
// Remove item
//
SB_INLINE void *SB_Ts_Md_Queue::remove() {
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
SB_INLINE bool SB_Ts_Md_Queue::remove_list(SB_DQL_Type *pp_item) {
    bool lv_ret;

    lv_ret = SB_Ts_D_Queue::remove_list(pp_item);
    if (lv_ret) {
        if (gv_ms_trace_params)
            remove_account(pp_item);
    }
    return lv_ret;
}

//
// Unlock queue (can be called by client)
//
SB_INLINE void SB_Ts_Md_Queue::unlock() {
    SB_Ts_D_Queue::unlock();
}

// SB_Sig_Md_Queue destructor
SB_INLINE SB_Sig_Md_Queue::~SB_Sig_Md_Queue() {
}

SB_INLINE void SB_Sig_Md_Queue::add(SB_DQL_Type *pp_item) {
    SB_Sig_D_Queue::add(pp_item);
}

SB_INLINE void SB_Sig_Md_Queue::add_at_front(SB_DQL_Type *pp_item) {
    SB_Sig_D_Queue::add_at_front(pp_item);
}

SB_INLINE void SB_Sig_Md_Queue::add_list(SB_DQL_Type *pp_prev, SB_DQL_Type *pp_item) {
    SB_Sig_D_Queue::add_list(pp_prev, pp_item);
}

SB_INLINE void SB_Sig_Md_Queue::lock() {
    SB_Sig_D_Queue::lock();
}

SB_INLINE void SB_Sig_Md_Queue::printself(bool pv_traverse, bool pv_lock) {
    SB_Sig_D_Queue::printself(pv_traverse, pv_lock);
}

SB_INLINE void *SB_Sig_Md_Queue::remove() {
    return SB_Sig_D_Queue::remove();
}

SB_INLINE void SB_Sig_Md_Queue::unlock() {
    SB_Sig_D_Queue::unlock();
}

#endif // !__SB_QUEUEMD_INL_
