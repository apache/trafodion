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

#include <unistd.h>

#include "seabed/int/assert.h"

#include "mapstats.h"

#ifdef USE_SB_MAP_STATS
FILE           *SB_Map_Stats::cp_file = NULL;
SB_Thread::MSL *SB_Map_Stats::cp_lock = NULL;
#endif

#ifndef USE_SB_INLINE
#include "mapstats.inl"
#endif

#ifdef USE_SB_MAP_STATS

// constructor
SB_Map_Stats::SB_Map_Stats(void       *pp_map,
                           const char *pp_map_type,
                           const char *pp_map_name,
                           int         pv_buckets)
: ip_bucket_len(NULL), ip_bucket_max(NULL),
  ip_map(pp_map), ip_map_type(pp_map_type),
  ip_map_name(pp_map_name), iv_buckets(pv_buckets), iv_count(0) {
    resize(pv_buckets);
}

// destructor
SB_Map_Stats::~SB_Map_Stats() {
    if (ip_bucket_len != NULL)
        delete [] ip_bucket_len;
    if (ip_bucket_max != NULL)
        delete [] ip_bucket_max;
}

// print statistics report
void SB_Map_Stats::print_stats() {
    char la_file[100];
    int  lv_hash;
    int  lv_max;
    int  lv_max_hash;
    int  lv_status;

    lv_max = 0;
    lv_max_hash = 0;
    for (lv_hash = 0; lv_hash < iv_buckets; lv_hash++) {
        if (ip_bucket_max[lv_hash] > lv_max) {
            lv_max = ip_bucket_max[lv_hash];
            lv_max_hash = lv_hash;
        }
    }

    // this is a memory leak - but
    // static construction doesn't work
    if (cp_lock == NULL)
        cp_lock = new SB_Thread::MSL();
    lv_status = cp_lock->lock();
    SB_util_assert_ieq(lv_status, 0);
    if (cp_file == NULL) {
        sprintf(la_file, "zsbmap.%d", getpid());
        cp_file = fopen(la_file, "a");
        SB_util_assert_pne(cp_file, NULL);
    }
    fprintf(cp_file, "map stats - map=%p, type=%s, name=%s, max-chain=%d, hash=%d, count-max=%d, buckets=%d\n",
            ip_map, ip_map_type, ip_map_name,
            lv_max, lv_max_hash, iv_count_max, iv_buckets);
    fflush(cp_file);
    lv_status = cp_lock->unlock();
    SB_util_assert_ieq(lv_status, 0);
}

// resize statistics structures
void SB_Map_Stats::resize(int pv_buckets) {
    int lv_hash;

    if (ip_bucket_len != NULL)
        delete [] ip_bucket_len;
    if (ip_bucket_max != NULL)
        delete [] ip_bucket_max;

    iv_buckets = pv_buckets;
    ip_bucket_len = new int[pv_buckets];
    ip_bucket_max = new int[pv_buckets];
    for (lv_hash = 0; lv_hash < pv_buckets; lv_hash++) {
        ip_bucket_len[lv_hash] = 0; // int
        ip_bucket_max[lv_hash] = 0; // int
    }
    iv_count_max = 0;
}

#endif
