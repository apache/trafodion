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

#include <sys/time.h>

#include "common/evl_sqlog_eventnum.h"

#include "seabed/log.h"
#include "seabed/thread.h"
#include "seabed/trace.h"

#include "chk.h"
#include "labelmapsx.h"
#include "logaggr.h"
#include "mstrace.h"

//
// An aggregate-log-record.
//
typedef struct {
    struct timeval iv_tv;
    int            iv_cap_type;
    int            iv_cap_size;
} SB_Log_Aggr_Cap_Rec_Type;

enum { SB_LOG_AGGR_CAP_MAX_RECS = 500 };

//
// Cache of log-records.
//
SB_Log_Aggr_Cap_Rec_Type gv_sb_log_aggr_cap_rec[SB_LOG_AGGR_CAP_MAX_RECS];
int                      gv_sb_log_aggr_cap_rec_cnt = 0;
SB_Thread::Mutex         gv_sb_log_aggr_cap_mutex("mutex-gv_sb_log_aggr_cap_mutex");

// forwards
static void        sb_log_aggr_cap_log_aggr();
static const char *sb_log_aggr_cap_log_get_cap_type(int pv_cap_type);
static void        sb_log_aggr_cap_log_aggr_write(char *pp_lbuf);

//
// seabed callers log a capacity-change here
//
// Capacity changes are logged into the aggregate-log-buffer.
// If the aggregate-log-buffer overflows, the aggregate-log-buffer is logged
//
void sb_log_aggr_cap_log(const char *pp_where,
                         int         pv_cap_type,
                         int         pv_cap_size) {
    const char               *lp_fmt;
    SB_Log_Aggr_Cap_Rec_Type *lp_rec;
    int                       lv_status;

    if (gv_ms_trace_evlog) {
        lp_fmt = sb_log_aggr_cap_log_get_cap_type(pv_cap_type);
        trace_where_printf(pp_where, lp_fmt, pv_cap_size);
    }
    lv_status = gv_sb_log_aggr_cap_mutex.lock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
    lp_rec = &gv_sb_log_aggr_cap_rec[gv_sb_log_aggr_cap_rec_cnt];
    gv_sb_log_aggr_cap_rec_cnt++;
    gettimeofday(&lp_rec->iv_tv, NULL);
    lp_rec->iv_cap_type = pv_cap_type;
    lp_rec->iv_cap_size = pv_cap_size;
    if (gv_sb_log_aggr_cap_rec_cnt >= SB_LOG_AGGR_CAP_MAX_RECS) {
        sb_log_aggr_cap_log_aggr();
        gv_sb_log_aggr_cap_rec_cnt = 0;
    }
    lv_status = gv_sb_log_aggr_cap_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
}

//
// Scan aggregate-log-buffer and create text for each.
// If output-text overflows, log output-text.
//
static void sb_log_aggr_cap_log_aggr() {
    // can't use entire length
    enum                    { MAX_LOG = POSIX_SQLOG_ENTRY_MAXLEN - 50 };
    enum                    { MAX_LBUF = MAX_LOG - 100 };
    char                      la_lbuf[MAX_LOG];
    char                      la_rec[100];
    const char               *lp_fmt;
    const char               *lp_hdr = "capacity aggregate\n";
    SB_Log_Aggr_Cap_Rec_Type *lp_reci;
    struct tm                *lp_tx;
    int                       lv_empty_size;
    int                       lv_len;
    int                       lv_reci_inx;
    int                       lv_status;
    struct timeval            lv_tv;
    struct tm                 lv_tx;

    lv_status = SB_log_init(SQEVL_SEABED, la_lbuf, MAX_LOG);
    if (lv_status)
        return;
    lv_empty_size = SB_log_buf_used(la_lbuf);
    for (lv_reci_inx = 0;
         lv_reci_inx < gv_sb_log_aggr_cap_rec_cnt;
         lv_reci_inx++) {
        if (SB_log_buf_used(la_lbuf) == lv_empty_size) {
            gettimeofday(&lv_tv, NULL);
            lp_tx = localtime_r(&lv_tv.tv_sec, &lv_tx);
            lv_len = sprintf(la_rec, "%02d:%02d:%02d.%06d: %s",
                             lp_tx->tm_hour,
                             lp_tx->tm_min,
                             lp_tx->tm_sec,
                             static_cast<int>(lv_tv.tv_usec),
                             lp_hdr);
            lv_status = SB_log_add_token(la_lbuf, TY_STRING, la_rec);
            CHK_STATUSIGNORE(lv_status);
        }
        lp_reci = &gv_sb_log_aggr_cap_rec[lv_reci_inx];
        lp_fmt = sb_log_aggr_cap_log_get_cap_type(lp_reci->iv_cap_type);
        lp_tx = localtime_r(&lp_reci->iv_tv.tv_sec, &lv_tx);
        lv_len = sprintf(la_rec, "%02d:%02d:%02d.%06d: ",
                         lp_tx->tm_hour,
                         lp_tx->tm_min,
                         lp_tx->tm_sec,
                         static_cast<int>(lp_reci->iv_tv.tv_usec));
        sprintf(&la_rec[lv_len], lp_fmt, lp_reci->iv_cap_size);
        lv_status = SB_log_add_token(la_lbuf, TY_STRING, la_rec);
        CHK_STATUSIGNORE(lv_status);
        if (SB_log_buf_used(la_lbuf) > MAX_LBUF) {
            sb_log_aggr_cap_log_aggr_write(la_lbuf);
            lv_status = SB_log_init(SQEVL_SEABED, la_lbuf, MAX_LOG);
            if (lv_status)
                return;
        }
    }
    if (SB_log_buf_used(la_lbuf) > lv_empty_size)
        sb_log_aggr_cap_log_aggr_write(la_lbuf);
}

const char *sb_log_aggr_cap_log_get_cap_type(int pv_cap_type) {
    const char *lp_label = SB_get_label(&gv_sb_log_aggr_cap_label_map,
                                        pv_cap_type);
    return lp_label;

}

//
// Do the actual log.
//
static void sb_log_aggr_cap_log_aggr_write(char *pp_lbuf) {
#ifdef USE_SB_SP_LOG
    int lv_status;

    lv_status = SB_log_write(SQ_LOG_SEAQUEST, // USE_SB_SP_LOG
                             SB_EVENT_ID,
                             SQ_LOG_INFO,
                             pp_lbuf);
    CHK_STATUSIGNORE(lv_status);
#else
    pp_lbuf = pp_lbuf; // touch
#endif
}

//
// seabed callers flush the aggregate-log.
//
// If here are any records in the aggregate-log-buffer,
// the aggregate-log-buffer is logged.
//
void sb_log_aggr_cap_log_flush() {
    int lv_status;

    lv_status = gv_sb_log_aggr_cap_mutex.lock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
    if (gv_sb_log_aggr_cap_rec_cnt > 0) {
        sb_log_aggr_cap_log_aggr();
        gv_sb_log_aggr_cap_rec_cnt = 0;
    }
    lv_status = gv_sb_log_aggr_cap_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
}

