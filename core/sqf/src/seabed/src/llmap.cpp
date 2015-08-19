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

#include <stdio.h>
#include <string.h>

#include "llmap.h"
#include "util.h"


#ifndef USE_SB_INLINE
#include "llmap.inl"
#endif

//
// Implement long-long map
//
SB_LLmap::SB_LLmap(int pv_buckets, float pv_lf)
: SB_Map_Comm(SB_ECID_MAP_LL, "llmap", pv_buckets, pv_lf) {
    init();
}

SB_LLmap::SB_LLmap(const char *pp_name, int pv_buckets, float pv_lf)
: SB_Map_Comm(SB_ECID_MAP_LL, "llmap", pp_name, pv_buckets, pv_lf) {
    init();
}

SB_LLmap::~SB_LLmap() {
    delete [] ipp_HT;
}

#ifdef LLMAP_CHECK
void SB_LLmap::check_integrity() {
    SB_LLML_Type *lp_item;
    int           lv_count;
    int           lv_hash;

    lv_count = 0;
    for (lv_hash = 0; lv_hash < iv_buckets; lv_hash++) {
        lp_item = ipp_HT[lv_hash];
        while (lp_item != NULL) {
            lp_item = lp_item->ip_next;
            lv_count++;
            SB_util_assert_ile(lv_count, iv_count);
        }
    }
    SB_util_assert_ieq(iv_count, lv_count);
}
#endif // LLMAP_CHECK

void SB_LLmap::init() {
    int lv_hash;

    iv_buckets = SB_Map_Comm::calc_buckets(iv_buckets);
    set_buckets(iv_buckets);

    ipp_HT = new SB_LLML_Type *[iv_buckets + 1];
    for (lv_hash = 0; lv_hash <= iv_buckets; lv_hash++)
        ipp_HT[lv_hash] = NULL;

#ifdef USE_SB_MAP_STATS
    SB_Map_Comm::init(this);
#endif
}

void SB_LLmap::printself(bool pv_traverse) {
    SB_LLML_Type *lp_item;
    int           lv_hash;
    Key_Type      lv_id;
    int           lv_inx;

    printf("this=%p, type=llmap, name=%s, size=%d, buckets=%d\n",
           pfp(this), ia_map_name, iv_count, iv_buckets);
    if (pv_traverse) {
        lv_inx = 0;
        for (lv_hash = 0; lv_hash < iv_buckets; lv_hash++) {
            lp_item = ipp_HT[lv_hash];
            for (;
                 lp_item != NULL;
                 lp_item = lp_item->ip_next) {
                lv_id = lp_item->iv_id.ll;
                printf("  inx=%d, Item=%p, Hash=%d, id=" PF64 " (0x" PF64X ")\n",
                       lv_inx, pfp(lp_item), lv_hash, lv_id, lv_id);
                lv_inx++;
            }
        }
    }
}

void *SB_LLmap::remove(Key_Type pv_id) {
    SB_LLML_Type *lp_item;
    SB_LLML_Type *lp_prev;
    int           lv_hash;

    lv_hash = hash(pv_id, iv_buckets);
    lp_item = ipp_HT[lv_hash];
    lp_prev = NULL;
    while (lp_item != NULL) {
        if (lp_item->iv_id.ll == pv_id) {
            if (lp_prev == NULL)
                ipp_HT[lv_hash] = lp_item->ip_next;
            else
                lp_prev->ip_next = lp_item->ip_next;
#ifdef USE_SB_MAP_STATS
            ip_stats->chain_del(lv_hash); // remove
#endif
            iv_count--;
            iv_mod++;
#ifdef LLMAP_CHECK
            check_integrity();
#endif // LLMAP_CHECK
            return lp_item;
        }
        lp_prev = lp_item;
        lp_item = lp_item->ip_next;
    }
    return NULL;
}

void SB_LLmap::removeall() {
    SB_LLML_Type *lp_item;
    int           lv_hash;

    for (lv_hash = 0; lv_hash < iv_buckets; lv_hash++) {
        for (;;) {
            lp_item = ipp_HT[lv_hash];
            if (lp_item == NULL)
                break;
            ipp_HT[lv_hash] = lp_item->ip_next;
#ifdef USE_SB_MAP_STATS
            ip_stats->chain_del(lv_hash); // removeall
#endif
            delete lp_item;
        }
    }
}

void SB_LLmap::resize(int pv_buckets) {
    int            lv_bucketsn;
    int            lv_bucketso;
    int            lv_hashn;
    int            lv_hasho;
    SB_LLML_Type  *lp_item;
    SB_LLML_Type **lpp_HTn;
    SB_LLML_Type **lpp_HTo;

    // create new set of buckets and swap
    lv_bucketsn = calc_buckets(pv_buckets);
    if (lv_bucketsn <= iv_buckets)
        return; // forget it
    lpp_HTn = new SB_LLML_Type *[lv_bucketsn + 1];
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
            lp_item = lpp_HTo[lv_hasho];
            if (lp_item == NULL)
                break;
            lpp_HTo[lv_hasho] = lp_item->ip_next;
            lv_hashn = hash(lp_item->iv_id.ll, iv_buckets);
            lp_item->ip_next = ipp_HT[lv_hashn];
            ipp_HT[lv_hashn] = lp_item;
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

SB_LLML_Type *SB_LLmap_Enum::next() {
    SB_LLML_Type *lp_item;
    int           lv_buckets;
    int           lv_hash;

    lp_item = ip_item;
    lv_hash = iv_hash;
    SB_util_assert_ieq(iv_mod, ip_map->iv_mod); // sw fault
    if (iv_inx >= iv_count)
        return NULL;
    lv_buckets = ip_map->iv_buckets;
    for (; lv_hash < lv_buckets; lv_hash++) {
        if (lp_item != NULL) {
            iv_inx++;
            break;
        }
        lp_item = ip_map->ipp_HT[lv_hash+1];
    }
    ip_item = lp_item;
    iv_hash = lv_hash;
    if (lp_item != NULL)
        ip_item = lp_item->ip_next;
    return lp_item;
}

