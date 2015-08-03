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

#include "seabed/fserr.h"
#include "seabed/timer.h"
#include "seabed/trace.h"

#include "mstrace.h"
#include "timermap.h"
#include "util.h"


#ifndef USE_SB_INLINE
#include "timermap.inl"
#endif

//
// timer-map contains timers.
// this map is intended to be able to handle 1000's of timers.
// efficient put/remove is a requirement.
//
// A timer's 'pop-time' is 'hashed' and timers are linked into ipp_HT.
// Hash collisions are ordered by pop-time.
//
// ipp_HT       +--------+             +--------+ <-item
// +---+        .        .             .        .
// |   |        .        .             .        .
// +---+        +--------+ SB_TML_Type +--------+ <-link
// |   |------->| (next) |------------>| (next) |
// +---+        | (prev) |<------------| (prev) |
// |   |        .        .             .        .
// +---+        .        .             .        .
// |   |        +--------+             +--------+
// +---+        .        .             .        .
// |   |        .        .             .        .
// +---+        +--------+             +--------+
//
// A repeating 'master' timer is used to check if any timers have popped.
// Note that the 'master' timer runs every iv_interval tics',
// so the timer resolution is related to iv_interval.
//

SB_TML_Type::SB_TML_Type() {
    SB_TimerMap::init_link(this);
}

//
// Implement timer-map
//
SB_TimerMap::SB_TimerMap(int pv_interval)
: SB_Map_Comm(SB_ECID_MAP_TIME, "timermap", BUCKETS, 0.0),
  iv_interval(pv_interval) {
    init();
}

SB_TimerMap::SB_TimerMap(const char *pp_name, int pv_interval)
: SB_Map_Comm(SB_ECID_MAP_TIME, "timermap", pp_name, BUCKETS, 0.0),
  iv_interval(pv_interval) {
    init();
}

SB_TimerMap::~SB_TimerMap() {
    const char *WHERE = "SB_TimerMap::~SB_TimerMap";

    if (gv_ms_trace_timermap)
        trace_where_printf(WHERE,
                           "destructor timermap=%p(%s)\n",
                           pfp(this),
                           ia_map_name);
    timer_master_stop();

    removeall();
    delete [] ipp_HT;
    delete ip_mutex;
}

#ifdef TIMERMAP_CHECK
void SB_TimerMap::check_integrity() {
    SB_TML_Type *lp_link;
    SB_TML_Type *lp_prev;
    int          lv_count;
    int          lv_hash;

    lv_count = 0;
    for (lv_hash = 0; lv_hash < BUCKETS; lv_hash++) {
        lp_prev = NULL;
        lp_link = ipp_HT[lv_hash];
        while (lp_link != NULL) {
            SB_util_assert_peq(lp_link->ip_prev, lp_prev);
            lp_prev = lp_link;
            lp_link = lp_link->ip_next;
            lv_count++;
            SB_util_assert_ile(lv_count, iv_count);
        }
    }
    SB_util_assert_ieq(iv_count, lv_count);
}
#endif // TIMERMAP_CHECK

//
// init timer-map
//
void SB_TimerMap::init() {
    const char *WHERE = "SB_TimerMap::SB_TimerMap";
    int         lv_hash;

    if (gv_ms_trace_timermap)
        trace_where_printf(WHERE,
                           "constructor timermap=%p(%s)\n",
                           pfp(this),
                           ia_map_name);
    ip_mutex = new SB_Thread::ECM("SB_TimerMap::ip_mutex");
    iv_last_hash_checked = 0;
    iv_time_last_check.tic_set(0);
    iv_master_running = false;
    iv_tleid = 0;
    ipp_HT = new SB_TML_Type *[BUCKETS + 1];
    for (lv_hash = 0; lv_hash <= BUCKETS; lv_hash++)
        ipp_HT[lv_hash] = NULL;

#ifdef USE_SB_MAP_STATS
    SB_Map_Comm::init(this);
#endif
}

//
// print timer-map
//
void SB_TimerMap::printself(bool pv_traverse) {
    char                  la_ts[100];
    SB_TML_Type          *lp_link;
    int                   lv_hash;
    int                   lv_inx;
    SB_Timer::Time_Stamp  lv_now;

    lv_now.format_ts(la_ts);
    lock();
    printf("this=%p, type=%s, name=%s, size=%d, buckets=%d, interval=%d, time=%s\n",
           pfp(this),
           ip_map_type,
           ia_map_name,
           iv_count,
           BUCKETS,
           iv_interval,
           la_ts);
    if (pv_traverse) {
        lv_inx = 0;
        for (lv_hash = 0; lv_hash < BUCKETS; lv_hash++) {
            lp_link = ipp_HT[lv_hash];
            for (;
                 lp_link != NULL;
                 lp_link = lp_link->ip_next) {
                lp_link->iv_pop_time.format_ts(la_ts);
                printf("  inx=%d, Item=%p, Hash=%d, tics=%d, pop-time=%s\n",
                       lv_inx,
                       pfp(lp_link),
                       lv_hash,
                       lp_link->iv_tics,
                       la_ts);
                lv_inx++;
            }
        }
    }
    unlock();
}

//
// put item into timer-map
//
void SB_TimerMap::put(SB_TML_Type                  *pp_link,
                      void                         *pp_item,
                      long                          pv_user_param,
                      SB_TML_Type::Timeout_Cb_Type  pv_to_cb,
                      int                           pv_tics) {
    put_lock(pp_link, pp_item, pv_user_param, pv_to_cb, pv_tics, true);
}

//
// put item into timer-map
//
void SB_TimerMap::put_lock(SB_TML_Type                  *pp_link,
                           void                         *pp_item,
                           long                          pv_user_param,
                           SB_TML_Type::Timeout_Cb_Type  pv_to_cb,
                           int                           pv_tics,
                           bool                          pv_lock) {
    const char   *WHERE = "SB_TimerMap::put";
    char          la_fmt_pop[100];
    int           lv_hash;
    SB_TML_Type  *lp_curr;
    SB_TML_Type  *lp_next;
    SB_TML_Type **lpp_curr;

    SB_util_assert_ieq(pp_link->iv_qid, QID_NONE);
    pp_link->ip_item = pp_item;
    pp_link->iv_qid = QID_TIME_MAP;
    pp_link->iv_running = true;
    if (pv_tics < 0)
        pp_link->iv_tics = 0;
    else
        pp_link->iv_tics = pv_tics;
    pp_link->iv_to_cb = pv_to_cb;
    pp_link->iv_user_param = pv_user_param;
    pp_link->iv_pop_time.tic_set_now_add(pv_tics);
    lv_hash = hash(pp_link->iv_pop_time);
    pp_link->iv_hash = lv_hash;

    if (pv_lock)
        lock();
    lpp_curr = &ipp_HT[lv_hash];
    lp_curr = *lpp_curr;
    if (lp_curr == NULL) {
        // is it the only one in this slot?
        pp_link->ip_next = NULL;
        pp_link->ip_prev = NULL;
        *lpp_curr = pp_link;
    } else if (pp_link->iv_pop_time.ts_lt(lp_curr->iv_pop_time)) {
        // should it go first in this slot?
        pp_link->ip_next = *lpp_curr;
        pp_link->ip_prev = NULL;
        lp_curr->ip_prev = pp_link;
        *lpp_curr = pp_link;
    } else {
        // it goes somewhere after the first
        lp_next = lp_curr->ip_next;

        while ((lp_next != NULL) &&
               (pp_link->iv_pop_time.ts_ge(lp_next->iv_pop_time))) {
            lp_curr = lp_next;
            lp_next = lp_curr->ip_next;
        }

        pp_link->ip_prev = lp_curr;
        pp_link->ip_next = lp_next;
        lp_curr->ip_next = pp_link;
        if (lp_next != NULL)
            lp_next->ip_prev = pp_link;
    }
    iv_count++;
    iv_mod++;

    if (gv_ms_trace_timermap)
        trace_where_printf(WHERE,
                           "put timermap=%p(%s), count=%d, link=%p, item=%p, user-param=%ld, tics=%d, hash=%d, pop-time=%s\n",
                           pfp(this),
                           ia_map_name,
                           iv_count,
                           pfp(pp_link),
                           pfp(pp_link->ip_item),
                           pv_user_param,
                           pv_tics,
                           lv_hash,
                           pp_link->iv_pop_time.format_ts(la_fmt_pop));

    timer_master_start();
#ifdef USE_SB_MAP_STATS
    ip_stats->chain_add(lv_hash); // put
#endif
#ifdef TIMERMAP_CHECK
    check_integrity();
#endif // TIMERMAP_CHECK
    if (pv_lock)
        unlock();
}

//
// remove item from timer-map
//
void *SB_TimerMap::remove(SB_TML_Type *pp_link) {
    void *lp_ret;

    lp_ret = remove_lock(pp_link, true);
    return lp_ret;
}

//
// remove item from timer-map
//
void *SB_TimerMap::remove_lock(SB_TML_Type *pp_link, bool pv_lock) {
    const char *WHERE = "SB_TimerMap::remove";
    char        la_fmt_pop[100];
    int         lv_hash;

    if (!pp_link->iv_running)
        return NULL;

    if (pv_lock)
        lock();
    SB_util_assert_ieq(pp_link->iv_qid, QID_TIME_MAP);
    SB_util_assert_ilt(pp_link->iv_hash, BUCKETS);
    lv_hash = pp_link->iv_hash;

    if (ipp_HT[lv_hash] == pp_link) {
        ipp_HT[lv_hash] = pp_link->ip_next;
        if (pp_link->ip_next != NULL)
            pp_link->ip_next->ip_prev = NULL;
    } else {
        if (pp_link->ip_prev != NULL)
            pp_link->ip_prev->ip_next = pp_link->ip_next;
        if (pp_link->ip_next != NULL)
            pp_link->ip_next->ip_prev = pp_link->ip_prev;
    }
    pp_link->ip_next = NULL;
    pp_link->ip_prev = NULL;
    pp_link->iv_qid_last = pp_link->iv_qid;
    pp_link->iv_qid = QID_NONE;
    pp_link->iv_running = false;
#ifdef USE_SB_MAP_STATS
    ip_stats->chain_del(lv_hash); // remove
#endif
    iv_count--;
    iv_mod++;

    if (gv_ms_trace_timermap)
        trace_where_printf(WHERE,
                           "remove timermap=%p(%s), count=%d, link=%p, item=%p, user-param=%ld, tics=%d, hash=%d, pop-time=%s\n",
                           pfp(this),
                           ia_map_name,
                           iv_count,
                           pfp(pp_link),
                           pfp(pp_link->ip_item),
                           pp_link->iv_user_param,
                           pp_link->iv_tics,
                           lv_hash,
                           pp_link->iv_pop_time.format_ts(la_fmt_pop));

#ifdef TIMERMAP_CHECK
    check_integrity();
#endif // TIMERMAP_CHECK
    if (pv_lock)
        unlock();
    return pp_link->ip_item;
}

//
// remove all items from timer-map
//
void SB_TimerMap::removeall() {
    removeall_del_lock(NULL, true);
}

void SB_TimerMap::removeall_lock(bool pv_lock) {
    removeall_del_lock(NULL, pv_lock);
}

void SB_TimerMap::removeall_del(Removeall_Del_Cb_Type pv_cb) {
    removeall_del_lock(pv_cb, true);
}

void SB_TimerMap::removeall_del_lock(Removeall_Del_Cb_Type pv_cb,
                                     bool                  pv_lock) {
    SB_TML_Type *lp_link;
    int          lv_hash;

    if (pv_lock)
        lock();
    for (lv_hash = 0; lv_hash < BUCKETS; lv_hash++) {
        for (;;) {
            lp_link = ipp_HT[lv_hash];
            if (lp_link == NULL)
                break;
            ipp_HT[lv_hash] = lp_link->ip_next;
#ifdef USE_SB_MAP_STATS
            ip_stats->chain_del(lv_hash); // removeall
#endif
            SB_util_assert_ieq(lp_link->iv_qid, QID_TIME_MAP);
            SB_util_assert_ieq(lp_link->iv_hash, lv_hash);
            lp_link->iv_qid_last = lp_link->iv_qid;
            lp_link->iv_qid = QID_NONE;
            lp_link->ip_next = NULL;
            lp_link->ip_prev = NULL;
            lp_link->iv_running = false;
            if (pv_cb != NULL)
                pv_cb(lp_link->iv_user_param, lp_link->ip_item);
            iv_count--;
            iv_mod++;
        }
    }
    SB_util_assert_ieq(iv_count, 0);
    if (pv_lock)
        unlock();
}

//
// cancel timer
//
void SB_TimerMap::timer_map_cancel(SB_TML_Type *pp_link) {
    const char *WHERE = "SB_TimerMap::timer_map_cancel";
    char        la_fmt_pop[100];

    if (!pp_link->iv_running)
        return;

    if (gv_ms_trace_timermap)
        trace_where_printf(WHERE,
                           "cancel timermap=%p(%s), link=%p, item=%p, user-param=%ld, tics=%d, hash=%d, pop-time=%s\n",
                           pfp(this),
                           ia_map_name,
                           pfp(pp_link),
                           pfp(pp_link->ip_item),
                           pp_link->iv_user_param,
                           pp_link->iv_tics,
                           pp_link->iv_hash,
                           pp_link->iv_pop_time.format_ts(la_fmt_pop));

    remove_lock(pp_link, false);
}

//
// check if any timers have timed out
//
void SB_TimerMap::timer_map_check() {
    const char  *WHERE = "SB_TimerMap::timer_map_check";
    char                  la_fmt_pop[100];
    SB_TML_Type          *lp_curr;
    SB_TML_Type          *lp_next;
    SB_Timer::Time_Stamp  lv_now;
    Hash_Type             lv_now_hash;
    Hash_Type             lv_hash;
    bool                  lv_pop;

    lv_pop = false;
    lp_next = NULL;
    lv_now_hash = hash(lv_now);

    iv_time_last_check.ts_set(lv_now);
    for (lv_hash = iv_last_hash_checked;
         lv_hash != (lv_now_hash + 1) % BUCKETS;
         lv_hash = (lv_hash + 1) % BUCKETS) {
        lp_curr = ipp_HT[lv_hash];
        while (lp_curr != NULL) {
            //
            // we're done if the timer pops later than now.
            //
            if (lp_curr->iv_pop_time.ts_gt(lv_now)) {
                if (gv_ms_trace_timermap) {
                    if (!lv_pop)
                        trace_where_printf(WHERE,
                                           "no timers ready timermap=%p(%s) - later - count=%d\n",
                                           pfp(this),
                                           ia_map_name,
                                           iv_count);
                }
                return;
            }

            lp_next = lp_curr->ip_next;
            timer_map_cancel(lp_curr);

            lv_pop = true;
            if (gv_ms_trace_timermap)
                trace_where_printf(WHERE,
                                   "callback timermap=%p(%s), link=%p, item=%p, user-param=%ld, tics=%d, hash=%d, pop-time=%s\n",
                                   pfp(this),
                                   ia_map_name,
                                   pfp(lp_curr),
                                   pfp(lp_curr->ip_item),
                                   lp_curr->iv_user_param,
                                   lp_curr->iv_tics,
                                   lv_hash,
                                   lp_curr->iv_pop_time.format_ts(la_fmt_pop));

            lp_curr->iv_to_cb(lp_curr->iv_user_param, lp_curr->ip_item);

            lp_curr = lp_next;
        }

        if (lv_hash != lv_now_hash) {
            // only increment if not at "now".  A new timer might be
            // set in the current time slot before "now" goes to the next
            // slot so we want to be sure and check it again.
            iv_last_hash_checked = (iv_last_hash_checked + 1) % BUCKETS;
        }
    }
    if (gv_ms_trace_timermap) {
        if (lv_pop)
            trace_where_printf(WHERE,
                               "timers running timermap=%p(%s) - count=%d\n",
                               pfp(this),
                               ia_map_name,
                               iv_count);
        else
            trace_where_printf(WHERE,
                               "no timers ready timermap=%p(%s) - count=%d\n",
                               pfp(this),
                               ia_map_name,
                               iv_count);
    }
}

//
// master timer callback
//
void SB_TimerMap::timer_master_cb(int   pv_tleid,
                                  int   pv_toval,
                                  short pv_parm1,
                                  long  pv_parm2) {
    SB_TimerMap *lp_map;

    pv_tleid = pv_tleid; // touch
    pv_toval = pv_toval; // touch
    SB_util_assert_ieq(pv_parm1, 1);
    lp_map = reinterpret_cast<SB_TimerMap *>(pv_parm2);
    lp_map->timer_master_check();
}

//
// master timer callback - check timers
//
void SB_TimerMap::timer_master_check() {
    lock();
    iv_master_running = false;
    timer_map_check();
    timer_master_start();
    unlock();
}

//
// start master timer
//
void SB_TimerMap::timer_master_start() {
    const char *WHERE = "SB_TimerMap::timer_master_start";
    int         lv_ferr;

    if (!iv_master_running) {
        if (gv_ms_trace_timermap)
            trace_where_printf(WHERE,
                               "start master-timer timermap=%p(%s), interval=%d\n",
                               pfp(this),
                               ia_map_name,
                               iv_interval);
        lv_ferr = timer_start_cb(iv_interval,                    // toval
                                 1,                              // parm1
                                 reinterpret_cast<long>(this),   // parm2
                                 &iv_tleid,                      // tleid
                                 timer_master_cb);               // callback
        SB_util_assert_ieq(lv_ferr, XZFIL_ERR_OK); // sw fault
        iv_master_running = true;
    }
}

//
// stop master timer
//
void SB_TimerMap::timer_master_stop() {
    const char *WHERE = "SB_TimerMap::timer_master_stop";
    int         lv_ferr;

    lock();
    if (iv_master_running) {
        if (gv_ms_trace_timermap)
            trace_where_printf(WHERE,
                               "stop master-timer timermap=%p(%s)\n",
                               pfp(this),
                               ia_map_name);
        lv_ferr = timer_cancel(iv_tleid);
        SB_util_assert_ieq(lv_ferr, XZFIL_ERR_OK); // sw fault
        iv_master_running = false;
    }
    unlock();
}

//
// walk timer-map
//
void SB_TimerMap::walk(Walk_Cb_Type pv_cb) {
    walk_lock(pv_cb, true);
}

//
// walk timer-map
//
void SB_TimerMap::walk_lock(Walk_Cb_Type pv_cb, bool pv_lock) {
    SB_TML_Type *lp_link;
    SB_TML_Type *lp_prev;
    int          lv_count;
    int          lv_hash;

    if (pv_lock)
        lock();
    lv_count = 0;
    for (lv_hash = 0; lv_hash < BUCKETS; lv_hash++) {
        lp_prev = NULL;
        lp_link = ipp_HT[lv_hash];
        while (lp_link != NULL) {
            SB_util_assert_peq(lp_link->ip_prev, lp_prev);
            if (pv_cb != NULL)
                pv_cb(lp_link->iv_user_param, lp_link->ip_item);
            lp_prev = lp_link;
            lp_link = lp_link->ip_next;
            lv_count++;
            SB_util_assert_ile(lv_count, iv_count);
        }
    }
    SB_util_assert_ieq(iv_count, lv_count);
    if (pv_lock)
        unlock();
}

SB_TML_Type *SB_TimerMap_Enum::next() {
    SB_TML_Type *lp_link;
    int          lv_hash;

    lp_link = ip_item;
    lv_hash = iv_hash;
    SB_util_assert_ieq(iv_mod, ip_map->iv_mod); // sw fault
    if (iv_inx >= iv_count)
        return NULL;
    for (; lv_hash < SB_TimerMap::BUCKETS; lv_hash++) {
        if (lp_link != NULL) {
            iv_inx++;
            break;
        }
        lp_link = ip_map->ipp_HT[lv_hash+1];
    }
    ip_item = lp_link;
    iv_hash = lv_hash;
    if (lp_link != NULL)
        ip_item = lp_link->ip_next;
    return lp_link;
}

