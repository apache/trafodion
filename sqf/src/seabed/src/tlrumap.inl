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

#ifndef __SB_TLRUMAP_INL_
#define __SB_TLRUMAP_INL_

#include <string.h>

template <class I, class T>
SB_TlruMap<I,T>::SB_TlruMap(int              pv_cap,
                            const char      *pp_name,
                            Evicted_Cb_Type  pv_cb)
: SB_Map_Comm(SB_ECID_LRU_T, "tlru", 0, 1.0),
  ip_head(NULL), ip_tail(NULL),
  iv_cap(pv_cap), iv_count(0),
  iv_evicted_cb(pv_cb),
  iv_map(pp_name, pv_cap) {
    if (pp_name == NULL)
        ia_map_name[0] = '\0';
    else {
        ia_map_name[sizeof(ia_map_name) - 1] = 0;
        strncpy(ia_map_name, pp_name, sizeof(ia_map_name) - 1);
    }
}

template <class I, class T>
SB_TlruMap<I,T>::~SB_TlruMap() {
}

template <class I, class T>
bool SB_TlruMap<I,T>::empty() {
    return iv_map.empty();
}

template <class I, class T>
T *SB_TlruMap<I,T>::get(I pv_id) {
    Tmap_Node *lp_node;
    T         *lp_ret;

    lp_node = static_cast<Tmap_Node *>(iv_map.get(pv_id));
    if (lp_node == NULL)
        lp_ret = NULL;
    else {
        lp_ret = lp_node->ip_item;
        if (ip_head != lp_node) {
            if (ip_tail == lp_node)
                ip_tail = lp_node->ip_prev;
            // unlink
            if (lp_node->ip_prev != NULL)
                lp_node->ip_prev->ip_next = lp_node->ip_next;
            if (lp_node->ip_next != NULL)
                lp_node->ip_next->ip_prev = lp_node->ip_prev;
            // put at head
            lp_node->ip_next = ip_head;
            ip_head = lp_node;
        }
    }
    return lp_ret;
}

template <class I, class T>
T *SB_TlruMap<I,T>::get_noage(I pv_id) {
    Tmap_Node *lp_node;
    T         *lp_ret;

    lp_node = static_cast<Tmap_Node *>(iv_map.get(pv_id));
    if (lp_node == NULL)
        lp_ret = NULL;
    else
        lp_ret = lp_node->ip_item;
    return lp_ret;
}

template <class I, class T>
SB_TlruMap_Enum<I,T> *SB_TlruMap<I,T>::keys() {
    return new SB_TlruMap_Enum<I,T>(this);
}

template <class I, class T>
void SB_TlruMap<I,T>::printself(bool pv_traverse) {
    Tmap_Node *lp_node;
    int        lv_inx;

    printf("this=%p, type=tlru, name=%s, size=%d, cap=%d\n",
           pfp(this), ia_map_name, iv_count, iv_cap);
    if (pv_traverse) {
        printf("head=%p, tail=%p\n", pfp(ip_head), pfp(ip_tail));
        lp_node = ip_head;
        for (lv_inx = 0; lv_inx < iv_count; lv_inx++) {
            printf("node[%d]=%p, p=%p, n=%p, id=" PF64 ", v=%p\n",
                   lv_inx, pfp(lp_node),
                   pfp(lp_node->ip_prev), pfp(lp_node->ip_next),
                   lp_node->iv_link.iv_id.ll, pfp(lp_node->ip_item));
            lp_node = lp_node->ip_next;
        }
    }
    iv_map.printself(pv_traverse);
}

template <class I, class T>
void SB_TlruMap<I,T>::put(I pv_id, T *pp_item) {
    Tmap_Node *lp_node;
    I          lv_id;

    if (iv_count >= iv_cap) {
        // remove from tail and map
        lp_node = ip_tail;
        ip_tail = lp_node->ip_prev;
        ip_tail->ip_next = NULL;
        if (iv_evicted_cb != NULL) {
            lv_id = static_cast<I>(lp_node->iv_link.iv_id.ll);
            iv_evicted_cb(lv_id, lp_node->ip_item);
        }
        iv_map.remove(lp_node->iv_link.iv_id.ll);
        delete lp_node;
        iv_count--;
    }

    // add to head and map
    lp_node = new Tmap_Node;
    if (ip_head != NULL)
        ip_head->ip_prev = lp_node;
    if (ip_tail == NULL)
        ip_tail = lp_node;
    lp_node->iv_link.iv_id.ll = pv_id;
    lp_node->ip_item = pp_item;
    lp_node->ip_prev = NULL;
    lp_node->ip_next = ip_head;
    ip_head = lp_node;
    iv_count++;
    iv_map.put(&lp_node->iv_link);
}

template <class I, class T>
T *SB_TlruMap<I,T>::remove(I pv_id) {
    Tmap_Node *lp_node;
    T         *lp_ret;

    lp_node = static_cast<Tmap_Node *>(iv_map.remove(pv_id));
    if (lp_node == NULL)
        lp_ret = NULL;
    else {
        if (ip_head == lp_node)
            ip_head = lp_node->ip_next;
        if (ip_tail == lp_node)
            ip_tail = lp_node->ip_prev;
        // unlink
        if (lp_node->ip_prev != NULL)
            lp_node->ip_prev->ip_next = lp_node->ip_next;
        if (lp_node->ip_next != NULL)
            lp_node->ip_next->ip_prev = lp_node->ip_prev;
        lp_ret = lp_node->ip_item;
        delete  lp_node;
        iv_count--;
    }
    return lp_ret;
}

template <class I, class T>
void SB_TlruMap<I,T>::removeall() {
    Tmap_Node *lp_node;

    while (ip_head != NULL) {
        lp_node = ip_head;
        ip_head = lp_node->ip_next;
        iv_map.remove(lp_node->iv_link.iv_id.ll);
        iv_count--;
        delete lp_node;
    }
    ip_tail = NULL;
}

template <class I, class T>
void SB_TlruMap<I,T>::resize(int pv_cap) {
    iv_cap = pv_cap;
    iv_map.resize(pv_cap);
}

template <class I, class T>
int SB_TlruMap<I,T>::size() {
    return iv_map.size();
}

template <class I, class T>
SB_TlruMap_Enum<I,T>::SB_TlruMap_Enum(SB_TlruMap<I,T> *pp_map)
: ip_map(pp_map), iv_inx(0) {
    ip_node = reinterpret_cast<Tmap_Node *>(pp_map->ip_head);
    iv_count = pp_map->iv_count;
    iv_mod = pp_map->iv_mod;
}

template <class I, class T>
bool SB_TlruMap_Enum<I,T>::more() {
    SB_util_assert_ieq(iv_mod, ip_map->iv_mod); // sw fault
    return (iv_inx < iv_count);
}

template <class I, class T>
T *SB_TlruMap_Enum<I,T>::next() {
    T *lp_ret;

    SB_util_assert_ieq(iv_mod, ip_map->iv_mod); // sw fault
    if (iv_inx >= iv_count)
        return NULL;
    lp_ret = ip_node->ip_item;
    ip_node = ip_node->ip_next;
    iv_inx++;
    return lp_ret;
}

#endif // !__SB_TLRUMAP_INL_
