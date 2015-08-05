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

#ifndef __SB_LMAP_INL_
#define __SB_LMAP_INL_

#include <errno.h>

SB_INLINE bool SB_Lmap::empty() {
    return (iv_count == 0);
}

// hash needed by get
SB_INLINE int SB_Lmap::hash(Key_Type pv_id, int pv_buckets) {
    int lv_hash;

    lv_hash = (static_cast<unsigned int>(pv_id) % pv_buckets);
    return lv_hash;
}

SB_INLINE void *SB_Lmap::get(Key_Type pv_id) {
    SB_LML_Type *lp_item;
    int          lv_hash;

    lv_hash = hash(pv_id, iv_buckets);
    lp_item = ipp_HT[lv_hash];
    while (lp_item != NULL) {
        if (lp_item->iv_id.l == pv_id)
            return lp_item;
        lp_item = lp_item->ip_next;
    }
    return NULL;
}

SB_INLINE SB_Lmap_Enum *SB_Lmap::keys() {
    return new SB_Lmap_Enum(this);
}

SB_INLINE void SB_Lmap::put(SB_LML_Type *pp_item) {
    int lv_hash;

    lv_hash = hash(pp_item->iv_id.l, iv_buckets);
    pp_item->ip_next = static_cast<SB_QL_Type *>(ipp_HT[lv_hash]);
    ipp_HT[lv_hash] = pp_item;
    iv_count++;
    iv_mod++;
#ifdef USE_SB_MAP_STATS
    ip_stats->chain_add(lv_hash); // put
#endif
#ifdef LMAP_CHECK
    check_integrity();
#endif // LMAP_CHECK
    if (iv_count > iv_buckets_threshold)
        SB_Lmap::resize(iv_buckets_resize);
}

SB_INLINE int SB_Lmap::size() {
    return iv_count;
}

SB_INLINE void *SB_Ts_Lmap::get(Key_Type pv_id) {
    void *lp_item;

    lock();
    lp_item = SB_Lmap::get(pv_id);
    unlock();
    return lp_item;
}

SB_INLINE void *SB_Ts_Lmap::get_lock(Key_Type pv_id, bool pv_lock) {
    void *lp_item;

    if (pv_lock)
        lock();
    lp_item = SB_Lmap::get(pv_id);
    if (pv_lock)
        unlock();
    return lp_item;
}

SB_INLINE void SB_Ts_Lmap::lock() {
    int lv_status;

    lv_status = iv_lock.lock();
    SB_util_assert_ieq(lv_status, 0);
}

SB_INLINE void SB_Ts_Lmap::printself(bool pv_traverse) {
    lock();
    SB_Lmap::printself(pv_traverse);
    unlock();
}

SB_INLINE void SB_Ts_Lmap::put(SB_LML_Type *pp_item) {
    lock();
    SB_Lmap::put(pp_item);
    unlock();
}

SB_INLINE void SB_Ts_Lmap::put_lock(SB_LML_Type *pp_item, bool pv_lock) {
    if (pv_lock)
        lock();
    SB_Lmap::put(pp_item);
    if (pv_lock)
        unlock();
}

SB_INLINE void *SB_Ts_Lmap::remove(Key_Type pv_id) {
    void *lp_item;

    lock();
    lp_item = SB_Lmap::remove(pv_id);
    unlock();
    return lp_item;
}

SB_INLINE void *SB_Ts_Lmap::remove_lock(Key_Type pv_id, bool pv_lock) {
    void *lp_item;

    if (pv_lock)
        lock();
    lp_item = SB_Lmap::remove(pv_id);
    if (pv_lock)
        unlock();
    return lp_item;
}

SB_INLINE void SB_Ts_Lmap::resize(int pv_buckets) {
    lock();
    SB_Lmap::resize(pv_buckets);
    unlock();
}

SB_INLINE void SB_Ts_Lmap::setlockname(const char *pp_lockname) {
    iv_lock.setname(pp_lockname);
}

SB_INLINE int SB_Ts_Lmap::trylock() {
    int lv_status;

    lv_status = iv_lock.trylock();
    SB_util_assert((lv_status == 0) || (lv_status == EBUSY));
    return lv_status;
}

SB_INLINE void SB_Ts_Lmap::unlock() {
    int lv_status;

    lv_status = iv_lock.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

SB_INLINE SB_Lmap_Enum::SB_Lmap_Enum(SB_Lmap *pp_map)
: ip_item(pp_map->ipp_HT[0]), ip_map(pp_map), iv_hash(0), iv_inx(0) {
    iv_mod = pp_map->iv_mod;
    iv_count = pp_map->iv_count;
}

SB_INLINE bool SB_Lmap_Enum::more() {
    SB_util_assert_ieq(iv_mod, ip_map->iv_mod); // sw fault
    return (iv_inx < iv_count);
}

#endif // !__SB_LMAP_INL_
