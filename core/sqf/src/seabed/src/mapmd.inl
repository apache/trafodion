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

#ifndef __SB_MAPMD_INL_
#define __SB_MAPMD_INL_

// SB_Md_Map destructor
SB_INLINE SB_Md_Map::~SB_Md_Map() {
}

SB_INLINE bool SB_Md_Map::empty() {
    return SB_Imap::empty();
}

SB_INLINE void *SB_Md_Map::get(int pv_id) {
    return SB_Imap::get(pv_id);
}

SB_INLINE SB_Imap_Enum *SB_Md_Map::keys() {
    return SB_Imap::keys();
}

SB_INLINE void SB_Md_Map::printself(bool pv_traverse) {
    return SB_Imap::printself(pv_traverse);
}

SB_INLINE void SB_Md_Map::put(MS_Md_Type *pp_md) {
    SB_util_assert_ieq(pp_md->iv_link.iv_qid, QID_NONE);
    pp_md->iv_link.iv_qid = iv_qid;
    SB_Imap::put(reinterpret_cast<SB_ML_Type *>(&pp_md->iv_link));
}

SB_INLINE void *SB_Md_Map::remove(int pv_id) {
    MS_Md_Type *lp_md;
    void       *lp_ret;

    lp_ret = SB_Imap::remove(pv_id);
    if (lp_ret != NULL) {
        lp_md = static_cast<MS_Md_Type *>(lp_ret);
        lp_md->iv_link.iv_qid_last = lp_md->iv_link.iv_qid;
        lp_md->iv_link.iv_qid = QID_NONE;
    }
    return lp_ret;
}

SB_INLINE void SB_Md_Map::removeall() {
    SB_Imap::removeall();
}

SB_INLINE int SB_Md_Map::size() {
    return SB_Imap::size();
}

// SB_Ts_Md_Map destructor
SB_INLINE SB_Ts_Md_Map::~SB_Ts_Md_Map() {
}

SB_INLINE bool SB_Ts_Md_Map::empty() {
    return SB_Ts_Imap::empty();
}

SB_INLINE void *SB_Ts_Md_Map::get(int pv_id) {
    return SB_Ts_Imap::get(pv_id);
}

SB_INLINE SB_Imap_Enum *SB_Ts_Md_Map::keys() {
    return SB_Ts_Imap::keys();
}

SB_INLINE void SB_Ts_Md_Map::printself(bool pv_traverse) {
    return SB_Ts_Imap::printself(pv_traverse);
}

SB_INLINE void SB_Ts_Md_Map::put(MS_Md_Type *pp_md) {
    SB_util_assert_ieq(pp_md->iv_link.iv_qid, QID_NONE);
    pp_md->iv_link.iv_qid = iv_qid;
    SB_Ts_Imap::put(reinterpret_cast<SB_ML_Type *>(&pp_md->iv_link));
}

SB_INLINE void *SB_Ts_Md_Map::remove(int pv_id) {
    MS_Md_Type *lp_md;
    void       *lp_ret;

    lp_ret = SB_Ts_Imap::remove(pv_id);
    if (lp_ret != NULL) {
        lp_md = static_cast<MS_Md_Type *>(lp_ret);
        lp_md->iv_link.iv_qid_last = lp_md->iv_link.iv_qid;
        lp_md->iv_link.iv_qid = QID_NONE;
    }
    return lp_ret;
}

SB_INLINE void *SB_Ts_Md_Map::remove_lock(int pv_id, bool pv_lock) {
    MS_Md_Type *lp_md;
    void       *lp_ret;

    lp_ret = SB_Ts_Imap::remove_lock(pv_id, pv_lock);
    if (lp_ret != NULL) {
        lp_md = static_cast<MS_Md_Type *>(lp_ret);
        lp_md->iv_link.iv_qid_last = lp_md->iv_link.iv_qid;
        lp_md->iv_link.iv_qid = QID_NONE;
    }
    return lp_ret;
}

SB_INLINE void SB_Ts_Md_Map::removeall() {
    SB_Ts_Imap::removeall();
}

SB_INLINE int SB_Ts_Md_Map::size() {
    return SB_Ts_Imap::size();
}

#endif // !__SB_MAPMD_INL_
