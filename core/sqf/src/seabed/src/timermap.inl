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

#ifndef __SB_TIMERMAP_INL_
#define __SB_TIMERMAP_INL_

#include <errno.h>

#include "qid.h"

SB_INLINE bool SB_TimerMap::empty() {
    return (iv_count == 0);
}

// hash needed by get
SB_INLINE int SB_TimerMap::hash(SB_Timer::Time_Stamp &pr_ts) {
    int lv_hash;

    lv_hash =
      static_cast<Hash_Type>((pr_ts.tic_get() / TICS_PER_SLOT) % BUCKETS);
    return lv_hash;
}

SB_INLINE void *SB_TimerMap::get(SB_TML_Type *pp_link) {
    return get_lock(pp_link, true);
}

SB_INLINE void *SB_TimerMap::get_lock(SB_TML_Type *pp_link, bool pv_lock) {
    void *lp_ret;

    if (pv_lock)
        lock();
    if (!pp_link->iv_running)
        lp_ret = NULL;
    else {
        SB_util_assert_ieq(pp_link->iv_qid, QID_TIME_MAP);
        lp_ret = pp_link->ip_item;
    }
    if (pv_lock)
        unlock();
    return lp_ret;
}

SB_INLINE void SB_TimerMap::init_link(SB_TML_Type *pp_link) {
    memset(pp_link, 0, sizeof(SB_TML_Type));
    pp_link->iv_qid = QID_NONE;
}

SB_INLINE SB_TimerMap_Enum *SB_TimerMap::keys() {
    return new SB_TimerMap_Enum(this);
}

SB_INLINE void SB_TimerMap::lock() {
    int lv_status;

    lv_status = ip_mutex->lock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
}

SB_INLINE int SB_TimerMap::size() {
    return iv_count;
}

SB_INLINE void SB_TimerMap::unlock() {
    int lv_status;

    lv_status = ip_mutex->unlock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
}

SB_INLINE SB_TimerMap_Enum::SB_TimerMap_Enum(SB_TimerMap *pp_map)
: ip_item(pp_map->ipp_HT[0]), ip_map(pp_map), iv_hash(0), iv_inx(0) {
    iv_mod = pp_map->iv_mod;
    iv_count = pp_map->iv_count;
}

SB_INLINE bool SB_TimerMap_Enum::more() {
    SB_util_assert_ieq(iv_mod, ip_map->iv_mod); // sw fault
    return (iv_inx < iv_count);
}

#endif // !__SB_TIMERMAP_INL_
