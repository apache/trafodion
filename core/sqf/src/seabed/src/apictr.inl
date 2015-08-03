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

inline SB_API::Ctr::Ctr(Ctr_Type pv_ctr_type) : iv_ctr_type(pv_ctr_type) {
    clock_gettime(CLOCK_REALTIME, &iv_ts_start);
}

inline SB_API::Ctr::~Ctr() {
    clock_gettime(CLOCK_REALTIME, &iv_ts_stop);
    gv_sb_api_ctr_mgr.acct(this);
}

inline void SB_API::Ctr_Mgr::acct(Ctr *pp_ctr) {
    Stat_Type     *lp_stat;
    SB_Int64_Type  lv_delta;
    int            lv_status;

    if (!iv_ready)
        return;
    lv_delta =
      static_cast<SB_Int64_Type>(pp_ctr->iv_ts_stop.tv_sec) * SB_NS_PER_SEC -
      static_cast<SB_Int64_Type>(pp_ctr->iv_ts_start.tv_sec) * SB_NS_PER_SEC +
      pp_ctr->iv_ts_stop.tv_nsec -
      pp_ctr->iv_ts_start.tv_nsec;
    lp_stat = &ia_stats[pp_ctr->iv_ctr_type];

    lv_status = lp_stat->iv_sl.lock();
    SB_util_assert_ieq(lv_status, 0);

    // do accounting
    lp_stat->iv_count++;
    lp_stat->iv_time_total += lv_delta;
    if (lv_delta < lp_stat->iv_time_min)
        lp_stat->iv_time_min = lv_delta;
    if (lv_delta > lp_stat->iv_time_max)
        lp_stat->iv_time_max = lv_delta;

    lv_status = lp_stat->iv_sl.unlock();
    SB_util_assert_ieq(lv_status, 0);
}
