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

//
// Implement common-map
//

#ifndef __SB_MAPCOM_H_
#define __SB_MAPCOM_H_

#include "seabed/int/opts.h"

#include "ecid.h"
#include "mapstats.h"

class SB_Map_Comm {
private:
    SB_Ecid_Type iv_aecid_map_com; // should be first instance

protected:
    SB_Map_Comm(SB_Ecid_Data_Type  pv_ecid,
                const char        *pp_map_type,
                int                pv_buckets,
                float              pv_lf);
    SB_Map_Comm(SB_Ecid_Data_Type  pv_ecid,
                const char        *pp_map_type,
                const char        *pp_name,
                int                pv_buckets,
                float              pv_lf);
    virtual ~SB_Map_Comm();

    int           calc_buckets(int pv_buckets);
    void          init(void *pp_map);
    void          set_buckets(int pv_buckets);

    char          ia_map_name[20];
#ifdef USE_SB_MAP_STATS
    SB_Map_Stats *ip_stats;
#endif
    const char   *ip_map_type;
    int           iv_buckets;
    int           iv_buckets_resize;
    int           iv_buckets_resize_inx;
    int           iv_buckets_threshold;
    int           iv_count;
    float         iv_lf;
    int           iv_mod;
};

#endif // !__SB_MAPCOM_H_
