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

#include "smap.h"
#include "util.h"


#ifndef USE_SB_INLINE
#include "smap.inl"
#endif

//
// Implement string map
//
SB_Smap::SB_Smap(int pv_buckets, float pv_lf)
: SB_Map_Comm(SB_ECID_MAP_S, "smap", pv_buckets, pv_lf) {
    init();
}

SB_Smap::SB_Smap(const char *pp_name, int pv_buckets, float pv_lf)
: SB_Map_Comm(SB_ECID_MAP_S, "smap", pp_name, pv_buckets, pv_lf) {
    init();
}

SB_Smap::~SB_Smap() {
    SML_Type *lp_item;
    SML_Type *lp_next;
    int       lv_hash;

    if (iv_count) {
        for (lv_hash = 0; lv_hash < iv_buckets; lv_hash++) {
            while (ipp_HT[lv_hash] != NULL) {
                lp_item = ipp_HT[lv_hash];
                lp_next = reinterpret_cast<SML_Type *>(lp_item->iv_link.ip_next);
                delete [] lp_item->ip_key;
                if (!lp_item->iv_use_vvalue)
                    delete [] lp_item->ip_value;
                delete lp_item;
                ipp_HT[lv_hash] = lp_next;
#ifdef USE_SB_MAP_STATS
                ip_stats->chain_del(lv_hash); // destructor
#endif
                iv_count--;
            }
            if (iv_count == 0)
                break;
        }
    }
    delete [] ipp_HT;
}

#ifdef SMAP_CHECK
void SB_Smap::check_integrity() {
    SML_Type *lp_item;
    int       lv_count;
    int       lv_hash;

    lv_count = 0;
    for (lv_hash = 0; lv_hash < iv_buckets; lv_hash++) {
        lp_item = ipp_HT[lv_hash];
        while (lp_item != NULL) {
            lp_item = reinterpret_cast<SML_Type *>(lp_item->iv_link.ip_next);
            lv_count++;
            SB_util_assert_ile(lv_count, iv_count);
        }
    }
    SB_util_assert_ieq(iv_count, lv_count);
}
#endif // SMAP_CHECK

const char *SB_Smap::get(Key_Type pp_key) {
    SML_Type *lp_item;
    int       lv_hash;

    SB_util_assert_cpne(pp_key, NULL); // sw fault
    lv_hash = hash(pp_key, iv_buckets);
    lp_item = ipp_HT[lv_hash];
    while (lp_item != NULL) {
        if (strcmp(pp_key, lp_item->ip_key) == 0)
            return lp_item->ip_value;
        lp_item = reinterpret_cast<SML_Type *>(lp_item->iv_link.ip_next);
    }
    return NULL;
}

void *SB_Smap::getv(Key_Type pp_key) {
    SML_Type *lp_item;
    int       lv_hash;

    SB_util_assert_cpne(pp_key, NULL); // sw fault
    lv_hash = hash(pp_key, iv_buckets);
    lp_item = ipp_HT[lv_hash];
    while (lp_item != NULL) {
        if (strcmp(pp_key, lp_item->ip_key) == 0)
            return lp_item->ip_vvalue;
        lp_item = reinterpret_cast<SML_Type *>(lp_item->iv_link.ip_next);
    }
    return NULL;
}

void SB_Smap::init() {
    int lv_hash;

    iv_buckets = SB_Map_Comm::calc_buckets(iv_buckets);
    set_buckets(iv_buckets);

    ipp_HT = new SML_Type *[iv_buckets + 1];
    for (lv_hash = 0; lv_hash <= iv_buckets; lv_hash++)
        ipp_HT[lv_hash] = NULL;

#ifdef USE_SB_MAP_STATS
    SB_Map_Comm::init(this);
#endif
}

void SB_Smap::printself(bool pv_traverse) {
    SML_Type *lp_item;
    int       lv_hash;
    int       lv_inx;

    printf("this=%p, type=%s, name=%s, size=%d, buckets=%d\n",
           pfp(this), ip_map_type, ia_map_name, iv_count, iv_buckets);
    if (pv_traverse) {
        lv_inx = 0;
        for (lv_hash = 0; lv_hash < iv_buckets; lv_hash++) {
            lp_item = ipp_HT[lv_hash];
            for (;
                 lp_item != NULL;
                 lp_item = reinterpret_cast<SML_Type *>(lp_item->iv_link.ip_next)) {
                if (lp_item->iv_use_vvalue)
                    printf("  inx=%d, hash=%d, key='%s', value=%p\n",
                           lv_inx,
                           lv_hash,
                           lp_item->ip_key,
                           lp_item->ip_vvalue);
                else
                    printf("  inx=%d, hash=%d, key='%s', value='%s'\n",
                           lv_inx, lv_hash, lp_item->ip_key, lp_item->ip_value);
                lv_inx++;
            }
        }
    }
}

void SB_Smap::put(Key_Type pp_key, const char *pp_value) {
    SML_Type *lp_item;
    int       lv_hash;
    int       lv_len;

    SB_util_assert_cpne(pp_key, NULL); // sw fault
    SB_util_assert_cpne(pp_value, NULL); // sw fault
    SB_Smap::remove(pp_key, NULL);
    lv_hash = hash(pp_key, iv_buckets);
    lp_item = new SML_Type;
    lp_item->iv_link.ip_next = reinterpret_cast<SB_QL_Type *>(ipp_HT[lv_hash]);
    lv_len = static_cast<int>(strlen(pp_key));
    lp_item->ip_key = new char[lv_len+1];
    strcpy(lp_item->ip_key, pp_key);
    lv_len = static_cast<int>(strlen(pp_value));
    lp_item->ip_value = new char[lv_len+1];
    lp_item->ip_vvalue = NULL;
    lp_item->iv_use_vvalue = false;
    strcpy(lp_item->ip_value, pp_value);
    ipp_HT[lv_hash] = lp_item;
    iv_count++;
    iv_mod++;
#ifdef USE_SB_MAP_STATS
    ip_stats->chain_add(lv_hash);  // put
#endif
#ifdef SMAP_CHECK
    check_integrity();
#endif // SMAP_CHECK
    if (iv_count > iv_buckets_threshold)
        SB_Smap::resize(iv_buckets_resize);
}

void SB_Smap::putv(Key_Type pp_key, void *pp_value) {
    SML_Type *lp_item;
    int       lv_hash;
    int       lv_len;

    SB_util_assert_cpne(pp_key, NULL); // sw fault
    SB_util_assert_pne(pp_value, NULL); // sw fault
    SB_Smap::removev(pp_key);
    lv_hash = hash(pp_key, iv_buckets);
    lp_item = new SML_Type;
    lp_item->iv_link.ip_next = reinterpret_cast<SB_QL_Type *>(ipp_HT[lv_hash]);
    lv_len = static_cast<int>(strlen(pp_key));
    lp_item->ip_key = new char[lv_len+1];
    strcpy(lp_item->ip_key, pp_key);
    lp_item->ip_value = NULL;
    lp_item->ip_vvalue = pp_value;
    lp_item->iv_use_vvalue = true;
    ipp_HT[lv_hash] = lp_item;
    iv_count++;
    iv_mod++;
#ifdef USE_SB_MAP_STATS
    ip_stats->chain_add(lv_hash);  // putv
#endif
#ifdef SMAP_CHECK
    check_integrity();
#endif // SMAP_CHECK
    if (iv_count > iv_buckets_threshold)
        SB_Smap::resize(iv_buckets_resize);
}

void SB_Smap::remove(Key_Type pp_key, char *pp_value) {
    SML_Type *lp_item;
    SML_Type *lp_prev;
    int       lv_hash;

    SB_util_assert_cpne(pp_key, NULL); // sw fault
    lv_hash = hash(pp_key, iv_buckets);
    lp_item = ipp_HT[lv_hash];
    lp_prev = NULL;
    while (lp_item != NULL) {
        if (strcmp(pp_key, lp_item->ip_key) == 0) {
            if (lp_prev == NULL)
                ipp_HT[lv_hash] = reinterpret_cast<SML_Type *>(lp_item->iv_link.ip_next);
            else
                lp_prev->iv_link.ip_next = lp_item->iv_link.ip_next;
#ifdef USE_SB_MAP_STATS
            ip_stats->chain_del(lv_hash); // remove
#endif
            iv_count--;
            iv_mod++;
#ifdef SMAP_CHECK
            check_integrity();
#endif // SMAP_CHECK
            if (pp_value != NULL)
                strcpy(pp_value, lp_item->ip_value);
            delete [] lp_item->ip_key;
            if (!lp_item->iv_use_vvalue)
                delete [] lp_item->ip_value;
            delete lp_item;
            break;
        }
        lp_prev = lp_item;
        lp_item = reinterpret_cast<SML_Type *>(lp_item->iv_link.ip_next);
    }
}

void *SB_Smap::removev(Key_Type pp_key) {
    SML_Type *lp_item;
    SML_Type *lp_prev;
    void     *lp_ret;
    int       lv_hash;

    SB_util_assert_cpne(pp_key, NULL); // sw fault
    lv_hash = hash(pp_key, iv_buckets);
    lp_item = ipp_HT[lv_hash];
    lp_prev = NULL;
    while (lp_item != NULL) {
        if (strcmp(pp_key, lp_item->ip_key) == 0) {
            if (lp_prev == NULL)
                ipp_HT[lv_hash] = reinterpret_cast<SML_Type *>(lp_item->iv_link.ip_next);
            else
                lp_prev->iv_link.ip_next = lp_item->iv_link.ip_next;
#ifdef USE_SB_MAP_STATS
            ip_stats->chain_del(lv_hash); // removev
#endif
            iv_count--;
            iv_mod++;
#ifdef SMAP_CHECK
            check_integrity();
#endif // SMAP_CHECK
            lp_ret = lp_item->ip_vvalue;
            delete [] lp_item->ip_key;
            if (!lp_item->iv_use_vvalue)
                delete [] lp_item->ip_value;
            delete lp_item;
            return lp_ret;
        }
        lp_prev = lp_item;
        lp_item = reinterpret_cast<SML_Type *>(lp_item->iv_link.ip_next);
    }
    return NULL;
}

void SB_Smap::resize(int pv_buckets) {
    int        lv_bucketsn;
    int        lv_bucketso;
    int        lv_hashn;
    int        lv_hasho;
    SML_Type  *lp_item;
    SML_Type **lpp_HTn;
    SML_Type **lpp_HTo;

    // create new set of buckets and swap
    lv_bucketsn = calc_buckets(pv_buckets);
    if (lv_bucketsn <= iv_buckets)
        return; // forget it
    lpp_HTn = new SML_Type *[lv_bucketsn + 1];
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
            lpp_HTo[lv_hasho] = reinterpret_cast<SML_Type *>(lp_item->iv_link.ip_next);
            // do in-place insert - don't call put/putv
            lv_hashn = hash(lp_item->ip_key, iv_buckets);
            lp_item->iv_link.ip_next = reinterpret_cast<SB_QL_Type *>(ipp_HT[lv_hashn]);
            ipp_HT[lv_hashn] = lp_item;
            iv_count++;
            iv_mod++;
#ifdef USE_SB_MAP_STATS
            ip_stats->chain_add(lv_hashn);  // put
#endif
        }
    }

    // delete old buckets
    delete [] lpp_HTo;
}

SB_Smap_Enum::SB_Smap_Enum(SB_Smap *pp_map)
: ip_item(pp_map->ipp_HT[0]), ip_map(pp_map), iv_hash(0), iv_inx(0) {
    iv_mod = pp_map->iv_mod;
    iv_count = pp_map->iv_count;
}

char *SB_Smap_Enum::next() {
    SB_Smap::SML_Type *lp_item;
    int                lv_buckets;
    int                lv_hash;

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
    if (lp_item != NULL) {
        ip_item = reinterpret_cast<SB_Smap::SML_Type *>(lp_item->iv_link.ip_next);
        return lp_item->ip_key;
    }
    return NULL;
}

