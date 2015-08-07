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

#ifndef __SB_SMAP_INL_
#define __SB_SMAP_INL_

SB_INLINE bool SB_Smap::empty() {
    return (iv_count == 0);
}

SB_INLINE int SB_Smap::hash(Key_Type pp_key, int pv_buckets) {
    int lv_hash;
    int lv_inx;
    int lv_len;

    lv_len = static_cast<int>(strlen(pp_key));
    lv_hash = lv_len;
    for (lv_inx = 0; lv_inx < lv_len; lv_inx++)
        lv_hash = 31 * lv_hash + pp_key[lv_inx];
    lv_hash = (static_cast<unsigned int>(lv_hash) % pv_buckets);
    return lv_hash;
}

SB_INLINE SB_Smap_Enum *SB_Smap::keys() {
    return new SB_Smap_Enum(this);
}

SB_INLINE bool SB_Smap_Enum::more() {
    SB_util_assert_ieq(iv_mod, ip_map->iv_mod); // sw fault
    return (iv_inx < iv_count);
}

SB_INLINE int SB_Smap::size() {
    return iv_count;
}

#endif // !__SB_SMAP_INL_
