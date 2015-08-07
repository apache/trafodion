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


#include <limits.h>
#include <string.h>

#include "mapcom.h"

enum { SB_MAP_MIN_SIZE =    64 };
enum { SB_MAP_MAX_SIZE = 65521 };

// primes
static const int ga_sb_map_size[] = {
       61, //      1 - 2^^0
       61, //      2 - 2^^1
       61, //      4 - 2^^2
       61, //      8 - 2^^3
       61, //     16 - 2^^4
       61, //     32 - 2^^5
       61, //     64 - 2^^6
      113, //    128 - 2^^7
      251, //    256 - 2^^8
      509, //    512 - 2^^9
     1021, //   1024 - 2^^10
     2039, //   2048 - 2^^11
     4093, //   4096 - 2^^12
     8179, //   8192 - 2^^13
    16381, //  16384 - 2^^14
    32749, //  32768 - 2^^15
    65521, //  65536 - 2^^16
    65521  // 131072 - 2^^17
};


SB_Map_Comm::SB_Map_Comm(SB_Ecid_Data_Type  pv_ecid,
                         const char        *pp_map_type,
                         int                pv_buckets,
                         float              pv_lf)
: iv_aecid_map_com(pv_ecid),
  ip_map_type(pp_map_type),
  iv_buckets(pv_buckets),
  iv_count(0),
  iv_lf(pv_lf),
  iv_mod(0) {
    ia_map_name[0] = '\0';
}

SB_Map_Comm::SB_Map_Comm(SB_Ecid_Data_Type  pv_ecid,
                         const char        *pp_map_type,
                         const char        *pp_name,
                         int                pv_buckets,
                         float              pv_lf)
: iv_aecid_map_com(pv_ecid),
  ip_map_type(pp_map_type),
  iv_buckets(pv_buckets),
  iv_count(0),
  iv_lf(pv_lf),
  iv_mod(0) {
    if (pp_name == NULL)
        ia_map_name[0] = '\0';
    else {
        ia_map_name[sizeof(ia_map_name) - 1] = '\0';
        strncpy(ia_map_name, pp_name, sizeof(ia_map_name) - 1);
    }
}

SB_Map_Comm::~SB_Map_Comm() {
#ifdef USE_SB_MAP_STATS
    ip_stats->print_stats();
    delete ip_stats;
#endif
}

// return a prime bucket size
int SB_Map_Comm::calc_buckets(int pv_buckets) {
    int lv_b2;
    int lv_b2_inx;
    int lv_buckets;

    lv_buckets = pv_buckets;
    if (lv_buckets < SB_MAP_MIN_SIZE)
        lv_buckets = SB_MAP_MIN_SIZE;
    else if (lv_buckets > SB_MAP_MAX_SIZE)
        lv_buckets = SB_MAP_MAX_SIZE;
    lv_b2_inx = 0;
    lv_b2 = 1;
    while (lv_b2 < lv_buckets) {
        lv_b2_inx++;
        lv_b2 *= 2;
    }
    lv_buckets = ga_sb_map_size[lv_b2_inx];
    return lv_buckets;
}

#ifdef USE_SB_MAP_STATS
void SB_Map_Comm::init(void *pp_map) {
    ip_stats = new SB_Map_Stats(pp_map, ip_map_type, ia_map_name, iv_buckets);
}
#endif

void SB_Map_Comm::set_buckets(int pv_buckets) {
    int lv_b2;
    int lv_b2_inx;

    lv_b2_inx = 0;
    lv_b2 = 1;
    while (lv_b2 < pv_buckets) {
        lv_b2_inx++;
        lv_b2 *= 2;
    }
    iv_buckets = pv_buckets;
    iv_buckets_resize_inx = lv_b2_inx + 1;
    iv_buckets_resize = ga_sb_map_size[iv_buckets_resize_inx];
    if ((iv_buckets_resize == iv_buckets) &&
        (iv_buckets_resize >= SB_MAP_MAX_SIZE))
        iv_buckets_threshold = INT_MAX; // stop growing
    else
        iv_buckets_threshold =
          static_cast<int>(iv_lf * static_cast<float>(iv_buckets));
}

