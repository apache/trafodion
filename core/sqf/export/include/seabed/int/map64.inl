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

#ifndef __SB_INT_MAP64_INL_
#define __SB_INT_MAP64_INL_

// needed by forward
SB_INLINE void SB_Ts_Map64::lock() {
    int lv_status;

    lv_status = iv_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
}

// needed by forward
SB_INLINE void SB_Ts_Map64::unlock() {
    int lv_status;

    lv_status = iv_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

SB_INLINE void SB_Ts_Map64::clear() {
    lock();
    iv_map.clear();
    unlock();
}

SB_INLINE bool SB_Ts_Map64::end() {
    return (iv_iterator == iv_map.end());
}

SB_INLINE void *SB_Ts_Map64::get(SB_Int64_Type pv_key) {
    Iter lv_iter;

    lock();
    lv_iter = iv_map.find(pv_key);
    unlock();

    if (lv_iter == iv_map.end())
        return NULL;
    return lv_iter->second;
}

SB_INLINE void SB_Ts_Map64::get_end() {
    unlock();
}

SB_INLINE void *SB_Ts_Map64::get_first() {
    lock();
    iv_iterator = iv_map.begin();
    if (iv_iterator == iv_map.end())
        return NULL;
    return iv_iterator->second;
}

SB_INLINE void *SB_Ts_Map64::get_next() {
    iv_iterator++;
    if (iv_iterator == iv_map.end())
        return NULL;
    return iv_iterator->second;
}

SB_INLINE void SB_Ts_Map64::put(SB_Int64_Type pv_key, void *pp_data) {
    lock();
    iv_map.insert(std::make_pair(pv_key, pp_data));
    unlock();
}

SB_INLINE void *SB_Ts_Map64::remove(SB_Int64_Type pv_key) {
    void *lp_return = NULL;
    Iter  lv_iter;

    lock();
    lv_iter = iv_map.find(pv_key);
    if (lv_iter == iv_map.end()) {
       unlock();
       return NULL;
    }

    lp_return = lv_iter->second;
    iv_map.erase(pv_key);
    unlock();

    return lp_return;
}

SB_INLINE SB_Int64_Type SB_Ts_Map64::size() {
    return iv_map.size();
}

#endif //!__SB_INT_MAP64_INL_
