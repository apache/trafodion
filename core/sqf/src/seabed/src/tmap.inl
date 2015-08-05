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

#ifndef __SB_TMAP_INL_
#define __SB_TMAP_INL_

template <class I, class T>
SB_Tmap<I,T>::SB_Tmap(int pv_buckets, float pv_lf)
: SB_Map_Comm(SB_ECID_MAP_T, "tmap", pv_buckets, pv_lf) {
    init();
}

template <class I, class T>
SB_Tmap<I,T>::SB_Tmap(const char *pp_name, int pv_buckets, float pv_lf)
: SB_Map_Comm(SB_ECID_MAP_T, "tmap", pp_name, pv_buckets, pv_lf) {
    init();
}

template <class I, class T>
SB_Tmap<I,T>::~SB_Tmap() {
    delete [] ipp_HT;
}

#ifdef TMAP_CHECK
template <class I, class T>
void SB_Tmap<I,T>::check_integrity() {
    Tmap_Node *lp_node;
    int        lv_count;
    int        lv_hash;

    lv_count = 0;
    for (lv_hash = 0; lv_hash < iv_buckets; lv_hash++) {
        lp_node = ipp_HT[lv_hash];
        while (lp_node != NULL) {
            lp_node = lp_node->ip_next;
            lv_count++;
            SB_util_assert_ile(lv_count, iv_count);
        }
    }
    SB_util_assert_ieq(iv_count, lv_count);
}
#endif // TMAP_CHECK

template <class I, class T>
bool SB_Tmap<I,T>::empty() {
    return (iv_count == 0);
}

// hash needed by get
template <class I, class T>
int SB_Tmap<I,T>::hash(I pv_id, int pv_buckets) {
    int lv_hash;

    lv_hash = (static_cast<unsigned int>(pv_id) % pv_buckets);
    return lv_hash;
}

template <class I, class T>
T *SB_Tmap<I,T>::get(I pv_id) {
    Tmap_Node *lp_node;
    int        lv_hash;

    lv_hash = hash(pv_id, iv_buckets);
    lp_node = ipp_HT[lv_hash];
    while (lp_node != NULL) {
        if (lp_node->iv_id == pv_id)
            return lp_node->ip_item;
        lp_node = lp_node->ip_next;
    }
    return NULL;
}

template <class I, class T>
void SB_Tmap<I,T>::init() {
    int lv_hash;

    iv_buckets = SB_Map_Comm::calc_buckets(iv_buckets);
    set_buckets(iv_buckets);

    ipp_HT = new Tmap_Node *[iv_buckets + 1];
    for (lv_hash = 0; lv_hash <= iv_buckets; lv_hash++)
        ipp_HT[lv_hash] = NULL;

#ifdef USE_SB_MAP_STATS
    SB_Map_Comm::init(this);
#endif
}

template <class I, class T>
SB_Tmap_Enum<I,T> *SB_Tmap<I,T>::keys() {
    return new SB_Tmap_Enum<I,T>(this);
}

template <class I, class T>
void SB_Tmap<I,T>::printself(bool pv_traverse) {
    Tmap_Node *lp_node;
    int        lv_hash;
    int        lv_id;
    int        lv_inx;

    printf("this=%p, type=%s, name=%s, size=%d, buckets=%d\n",
           pfp(this), ip_map_type, ia_map_name, iv_count, iv_buckets);
    if (pv_traverse) {
        lv_inx = 0;
        for (lv_hash = 0; lv_hash < iv_buckets; lv_hash++) {
            lp_node = ipp_HT[lv_hash];
            for (;
                 lp_node != NULL;
                 lp_node = lp_node->ip_next) {
                lv_id = lp_node->iv_id;
                printf("  inx=%d, Item=%p, Hash=%d, id=%d (0x%x)\n",
                       lv_inx, pfp(lp_node), lv_hash, lv_id, lv_id);
                lv_inx++;
            }
        }
    }
}
template <class I, class T>
void SB_Tmap<I,T>::put(I pv_id, T *pp_item) {
    Tmap_Node *lp_node;
    int        lv_hash;

    lv_hash = hash(pv_id, iv_buckets);
    lp_node = new Tmap_Node;
    lp_node->iv_id = pv_id;
    lp_node->ip_next = ipp_HT[lv_hash];
    lp_node->ip_item = pp_item;
    ipp_HT[lv_hash] = lp_node;
    iv_count++;
    iv_mod++;
#ifdef USE_SB_MAP_STATS
    ip_stats->chain_add(lv_hash); // put
#endif
#ifdef TMAP_CHECK
    check_integrity();
#endif // TMAP_CHECK
    if (iv_count > iv_buckets_threshold)
        SB_Tmap::resize(iv_buckets_resize);
}

template <class I, class T>
T *SB_Tmap<I,T>::remove(I pv_id) {
    Tmap_Node *lp_node;
    Tmap_Node *lp_prev;
    T         *lp_ret;
    int        lv_hash;

    lv_hash = hash(pv_id, iv_buckets);
    lp_node = ipp_HT[lv_hash];
    lp_prev = NULL;
    while (lp_node != NULL) {
        if (lp_node->iv_id == pv_id) {
            if (lp_prev == NULL)
                ipp_HT[lv_hash] = lp_node->ip_next;
            else
                lp_prev->ip_next = lp_node->ip_next;
#ifdef USE_SB_MAP_STATS
            ip_stats->chain_del(lv_hash); // remove
#endif
            iv_count--;
            iv_mod++;
#ifdef TMAP_CHECK
            check_integrity();
#endif // TMAP_CHECK
            lp_ret = lp_node->ip_item;
            delete lp_node;
            return lp_ret;
        }
        lp_prev = lp_node;
        lp_node = lp_node->ip_next;
    }
    return NULL;
}

template <class I, class T>
void SB_Tmap<I,T>::removeall() {
    Tmap_Node *lp_node;
    int        lv_hash;

    for (lv_hash = 0; lv_hash < iv_buckets; lv_hash++) {
        for (;;) {
            lp_node = ipp_HT[lv_hash];
            if (lp_node == NULL)
                break;
            ipp_HT[lv_hash] = lp_node->ip_next;
#ifdef USE_SB_MAP_STATS
            ip_stats->chain_del(lv_hash); // removeall
#endif
            delete lp_node;
        }
    }
}

template <class I, class T>
void SB_Tmap<I,T>::resize(int pv_buckets) {
    int          lv_bucketsn;
    int          lv_bucketso;
    int          lv_hashn;
    int          lv_hasho;
    Tmap_Node   *lp_node;
    Tmap_Node  **lpp_HTn;
    Tmap_Node  **lpp_HTo;

    // create new set of buckets and swap
    lv_bucketsn = calc_buckets(pv_buckets);
    if (lv_bucketsn <= iv_buckets)
        return; // forget it
    lpp_HTn = new Tmap_Node *[lv_bucketsn + 1];
    for (lv_hashn = 0; lv_hashn <= lv_bucketsn; lv_hashn++)
        lpp_HTn[lv_hashn] = NULL;
    lpp_HTo = ipp_HT;
    ipp_HT = lpp_HTn;
    iv_count = 0;
    lv_bucketso = iv_buckets;
#ifdef USE_SB_MAP_STATS
    ip_stats->resize(lv_bucketsn);
#endif
    set_buckets(lv_bucketsn);

    // copy old buckets to new buckets
    for (lv_hasho = 0; lv_hasho < lv_bucketso; lv_hasho++) {
        for (;;) {
            lp_node = lpp_HTo[lv_hasho];
            if (lp_node == NULL)
                break;
            lpp_HTo[lv_hasho] = lp_node->ip_next;
            lv_hashn = hash(lp_node->iv_id, iv_buckets);
            lp_node->ip_next = ipp_HT[lv_hashn];
            ipp_HT[lv_hashn] = lp_node;
            iv_count++;
            iv_mod++;
#ifdef USE_SB_MAP_STATS
            ip_stats->chain_add(lv_hashn); // put
#endif
        }
    }

    // delete old buckets
    delete [] lpp_HTo;
}
template <class I, class T>
int SB_Tmap<I,T>::size() {
    return iv_count;
}

template <class I, class T>
SB_Tmap_Enum<I,T>::SB_Tmap_Enum(SB_Tmap<I,T> *pp_map)
: ip_map(pp_map),
  ip_node(reinterpret_cast<Tmap_Node *>(pp_map->ipp_HT[0])),
  iv_hash(0), iv_inx(0) {
    iv_mod = pp_map->iv_mod;
    iv_count = pp_map->iv_count;
}

template <class I, class T>
bool SB_Tmap_Enum<I,T>::more() {
    SB_util_assert_ieq(iv_mod, ip_map->iv_mod); // sw fault
    return (iv_inx < iv_count);
}

template <class I, class T>
T *SB_Tmap_Enum<I,T>::next() {
    Tmap_Node *lp_node;
    int        lv_buckets;
    int        lv_hash;

    lp_node = ip_node;
    lv_hash = iv_hash;
    SB_util_assert_ieq(iv_mod, ip_map->iv_mod); // sw fault
    if (iv_inx >= iv_count)
        return NULL;
    lv_buckets = ip_map->iv_buckets;
    for (; lv_hash < lv_buckets; lv_hash++) {
        if (lp_node != NULL) {
            iv_inx++;
            break;
        }
        lp_node = reinterpret_cast<Tmap_Node *>(ip_map->ipp_HT[lv_hash+1]);
    }
    ip_node = lp_node;
    iv_hash = lv_hash;
    if (lp_node != NULL)
        ip_node = lp_node->ip_next;
    return lp_node->ip_item;
}

#endif // !__SB_TMAP_INL_
