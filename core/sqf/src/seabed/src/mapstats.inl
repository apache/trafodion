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

#ifndef __SB_MAPSTATS_INL_
#define __SB_MAPSTATS_INL_

#ifdef USE_SB_MAP_STATS

SB_INLINE int SB_Map_Stats::chain_add(int pv_hash) {
    int lv_len;
    int lv_max;

    iv_count++;
    lv_len = ++ip_bucket_len[pv_hash];
    lv_max = ip_bucket_max[pv_hash];
    if (lv_len > lv_max) {
        ip_bucket_max[pv_hash] = lv_len;
        lv_max = lv_len;
    }
    if (iv_count > iv_count_max)
        iv_count_max = iv_count;
    return lv_max;
}

SB_INLINE void SB_Map_Stats::chain_del(int pv_hash) {
    iv_count--;
    ip_bucket_len[pv_hash]--;
}

#endif

#endif // !__SB_MAPSTATS_INL_

