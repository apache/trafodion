//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

#ifndef __SB_LLRUMAP_INL_
#define __SB_LLRUMAP_INL_

SB_INLINE bool SB_LlruMap::empty() {
    return SB_Lmap::empty();
}

SB_INLINE void *SB_LlruMap::get(Key_Type pv_id) {
    Llru_Node *lp_node;
    void      *lp_ret;

    lp_node = static_cast<Llru_Node *>(SB_Lmap::get(pv_id));
    if (lp_node == NULL)
        lp_ret = NULL;
    else {
        lp_ret = lp_node->ip_item;
        if (ip_lru_head != lp_node) {
            if (ip_lru_tail == lp_node)
                ip_lru_tail = lp_node->ip_prev;
            // unlink
            if (lp_node->ip_prev != NULL)
                lp_node->ip_prev->ip_next = lp_node->ip_next;
            if (lp_node->ip_next != NULL)
                lp_node->ip_next->ip_prev = lp_node->ip_prev;
            // put at head
            lp_node->ip_next = ip_lru_head;
            ip_lru_head = lp_node;
        }
    }
    return lp_ret;
}

SB_INLINE void *SB_LlruMap::get_noage(Key_Type pv_id) {
    Llru_Node *lp_node;
    void      *lp_ret;

    lp_node = static_cast<Llru_Node *>(SB_Lmap::get(pv_id));
    if (lp_node == NULL)
        lp_ret = NULL;
    else
        lp_ret = lp_node->ip_item;
    return lp_ret;
}

SB_INLINE SB_Lmap_Enum *SB_LlruMap::keys() {
    return new SB_LlruMap_Enum(this);
}

SB_INLINE void SB_LlruMap::printself(bool pv_traverse) {
    Llru_Node *lp_node;
    int        lv_inx;

    printf("this=%p, type=llru, name=%s, size=%d, cap=%d\n",
           pfp(this), ia_map_name, iv_lru_count, iv_lru_cap);
    if (pv_traverse) {
        printf("head=%p, tail=%p\n", pfp(ip_lru_head), pfp(ip_lru_tail));
        lp_node = ip_lru_head;
        for (lv_inx = 0; lv_inx < iv_lru_count; lv_inx++) {
            printf("node[%d]=%p, p=%p, n=%p, id=%ld, v=%p\n",
                   lv_inx, pfp(lp_node),
                   pfp(lp_node->ip_prev), pfp(lp_node->ip_next),
                   lp_node->iv_link.iv_id.l, lp_node->ip_item);
            lp_node = lp_node->ip_next;
        }
    }
    SB_Lmap::printself(pv_traverse);
}

SB_INLINE void SB_LlruMap::put(Key_Type pv_id, void *pp_item) {
    Llru_Node *lp_node;

    if (iv_lru_count >= iv_lru_cap) {
        // remove from tail and map
        lp_node = ip_lru_tail;
        ip_lru_tail = lp_node->ip_prev;
        ip_lru_tail->ip_next = NULL;
        if (iv_evicted_cb != NULL)
            iv_evicted_cb(lp_node->iv_link.iv_id.l, lp_node->ip_item);
        SB_Lmap::remove(lp_node->iv_link.iv_id.l);
        delete lp_node;
        iv_lru_count--;
    }

    // add to head and map
    lp_node = new Llru_Node;
    if (ip_lru_head != NULL)
        ip_lru_head->ip_prev = lp_node;
    if (ip_lru_tail == NULL)
        ip_lru_tail = lp_node;
    lp_node->iv_link.iv_id.l = pv_id;
    lp_node->ip_item = pp_item;
    lp_node->ip_prev = NULL;
    lp_node->ip_next = ip_lru_head;
    ip_lru_head = lp_node;
    iv_lru_count++;
    SB_Lmap::put(&lp_node->iv_link);
}

SB_INLINE void *SB_LlruMap::remove(Key_Type pv_id) {
    Llru_Node *lp_node;
    void      *lp_ret;

    lp_node = static_cast<Llru_Node *>(SB_Lmap::remove(pv_id));
    if (lp_node == NULL)
        lp_ret = NULL;
    else {
        if (ip_lru_head == lp_node)
            ip_lru_head = lp_node->ip_next;
        if (ip_lru_tail == lp_node)
            ip_lru_tail = lp_node->ip_prev;
        // unlink
        if (lp_node->ip_prev != NULL)
            lp_node->ip_prev->ip_next = lp_node->ip_next;
        if (lp_node->ip_next != NULL)
            lp_node->ip_next->ip_prev = lp_node->ip_prev;
        lp_ret = lp_node->ip_item;
        delete  lp_node;
        iv_lru_count--;
    }
    return lp_ret;
}

SB_INLINE void SB_LlruMap::removeall() {
    Llru_Node *lp_node;

    while (ip_lru_head != NULL) {
        lp_node = ip_lru_head;
        ip_lru_head = lp_node->ip_next;
        SB_Lmap::remove(lp_node->iv_link.iv_id.l);
        iv_lru_count--;
        delete lp_node;
    }
    ip_lru_tail = NULL;
}

SB_INLINE void SB_LlruMap::resize(int pv_cap) {
    iv_lru_cap = pv_cap;
    SB_Lmap::resize(pv_cap);
}

SB_INLINE int SB_LlruMap::size() {
    return SB_Lmap::size();
}

SB_INLINE SB_LlruMap_Enum::SB_LlruMap_Enum(SB_LlruMap *pp_map)
: ip_map(pp_map), iv_inx(0) {
    ip_node = pp_map->ip_lru_head;
    iv_count = pp_map->iv_count;
    iv_mod = pp_map->iv_mod;
}

SB_INLINE bool SB_LlruMap_Enum::more() {
    SB_util_assert_ieq(iv_mod, ip_map->iv_mod); // sw fault
    return (iv_inx < iv_count);
}

SB_INLINE SB_LML_Type *SB_LlruMap_Enum::next() {
    SB_LML_Type *lp_ret;

    SB_util_assert_ieq(iv_mod, ip_map->iv_mod); // sw fault
    if (iv_inx >= iv_count)
        return NULL;
    lp_ret = static_cast<SB_ML_Type *>(ip_node->ip_item);
    ip_node = ip_node->ip_next;
    iv_inx++;
    return lp_ret;
}

#endif // !__SB_LLRUMAP_INL_
