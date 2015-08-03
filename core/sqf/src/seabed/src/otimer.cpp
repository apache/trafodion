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

#include "seabed/otimer.h"
#include "seabed/trace.h"

#include "util.h"

SB_Timer::Timer::Slot_Type  SB_Timer::Timer::cv_last_slot_checked = 0;
SB_Timer::Timer            *SB_Timer::Timer::ca_slots[Timer::MAX_SLOTS] = { NULL };
SB_Thread::Mutex             SB_Timer::Timer::cv_mutex;
bool                         SB_Timer::Timer::cv_trace_enabled = false;

SB_Timer::Time_Stamp::Time_Stamp() {
    timeval lv_t;

    gettimeofday(&lv_t, NULL);
    iv_tics = (static_cast<Tics>(lv_t.tv_sec) * TICS_PER_SEC) +
              (lv_t.tv_usec / US_PER_TIC);
}

SB_Timer::Time_Stamp::Time_Stamp(Time_Stamp &pr_ts) {
    iv_tics = pr_ts.iv_tics;
}

SB_Timer::Time_Stamp::~Time_Stamp() {
}

const char *SB_Timer::Time_Stamp::format_ts(char *pp_buf) {
    struct tm *lp_tx;
    int        lv_ms;
    timeval    lv_t;
    struct tm  lv_tx;

    lv_t.tv_sec = static_cast<time_t>(iv_tics / TICS_PER_SEC);
    lv_t.tv_usec =
      static_cast<suseconds_t>((iv_tics - lv_t.tv_sec * TICS_PER_SEC) * US_PER_TIC);
    lp_tx = localtime_r(&lv_t.tv_sec, &lv_tx);
    lv_ms = static_cast<int>(lv_t.tv_usec) / 1000;

    // don't need to calculate us, as it will always be 0 due to tics
    sprintf(pp_buf, PF64 " (%02d:%02d:%02d.%03d.000)",
            iv_tics,
            lp_tx->tm_hour, lp_tx->tm_min, lp_tx->tm_sec, lv_ms);
    return pp_buf;
}

SB_Timer::Tics SB_Timer::Time_Stamp::tic_add(Tics pv_tics) {
    return iv_tics + pv_tics;
}

SB_Timer::Tics SB_Timer::Time_Stamp::tic_get() {
    return iv_tics;
}

void SB_Timer::Time_Stamp::tic_set(Tics pv_tics) {
    iv_tics = pv_tics;
}

void SB_Timer::Time_Stamp::tic_set_now_add(Tics pv_tics) {
    Time_Stamp lv_now;

    iv_tics = lv_now.iv_tics + pv_tics;
}

bool SB_Timer::Time_Stamp::ts_eq(Time_Stamp &pr_ts) {
    bool lv_ret;

    lv_ret = (iv_tics == pr_ts.iv_tics);
    return lv_ret;
}

bool SB_Timer::Time_Stamp::ts_le(Time_Stamp &pr_ts) {
    bool lv_ret;

    lv_ret = (iv_tics < pr_ts.iv_tics);
    return lv_ret;
}

bool SB_Timer::Time_Stamp::ts_lt(Time_Stamp &pr_ts) {
    bool lv_ret;

    lv_ret = (iv_tics < pr_ts.iv_tics);
    return lv_ret;
}

bool SB_Timer::Time_Stamp::ts_ge(Time_Stamp &pr_ts) {
    bool lv_ret;

    lv_ret = (iv_tics >= pr_ts.iv_tics);
    return lv_ret;
}

bool SB_Timer::Time_Stamp::ts_gt(Time_Stamp &pr_ts) {
    bool lv_ret;

    lv_ret = (iv_tics > pr_ts.iv_tics);
    return lv_ret;
}

bool SB_Timer::Time_Stamp::ts_ne(Time_Stamp &pr_ts) {
    bool lv_ret;

    lv_ret = (iv_tics != pr_ts.iv_tics);
    return lv_ret;
}

void SB_Timer::Time_Stamp::ts_set(Time_Stamp &pr_ts) {
    iv_tics = pr_ts.iv_tics;
}

SB_Timer::Tics SB_Timer::Time_Stamp::ts_sub(Time_Stamp &pr_ts) {
    Tics lv_ret;

    lv_ret = (iv_tics - pr_ts.iv_tics);
    return lv_ret;
}

SB_Timer::Timer::Timer() {
    SB_util_assert_bt(false);
}

SB_Timer::Timer::Timer(TH   *pp_th,
                       long  pv_user_param,
                       Tics  pv_interval,
                       bool  pv_start) {
    const char *WHERE = "Timer::Timer";

    SB_util_assert_pne(pp_th, NULL);

    ip_th = pp_th;
    iv_user_param = pv_user_param;

    if (pv_interval > ((MAX_SLOTS-1) * TICS_PER_SLOT))
        // Don't allow interval that would cause a wrap-around of
        // the timer slots array.
        pv_interval = (MAX_SLOTS-1) * TICS_PER_SLOT;
    else if (pv_interval < 0)
        pv_interval = 0;

    iv_interval = pv_interval;

    ip_next = NULL;
    iv_running = false;

    if (cv_trace_enabled)
        trace_where_printf(WHERE,
                           "timer=%p, user-param=%ld, interval=" PF64 ", start=%d\n",
                           pfp(this),
                           iv_user_param,
                           iv_interval,
                           pv_start);

    if (pv_start)
        start();
}

SB_Timer::Timer::~Timer() {
    const char *WHERE = "Timer::~Timer";

    cancel_int(true);
    if (cv_trace_enabled)
        trace_where_printf(WHERE,
                           "timer=%p, user-param=%ld, interval=" PF64 "\n",
                           pfp(this),
                           iv_user_param,
                           iv_interval);
}

void SB_Timer::Timer::cancel() {
    cancel_int(true);
}

void SB_Timer::Timer::cancel_int(bool pv_lock) {
    const char *WHERE = "Timer::cancel";
    char        la_fmt_pop[100];
    Timer      *lp_curr;
    Timer      *lp_next;
    Slot_Type   lv_slot;
    int         lv_status;

    if (!iv_running)
        return;
    lv_slot = hash(iv_pop_time);

    if (cv_trace_enabled)
        trace_where_printf(WHERE,
                           "timer=%p, user-param=%ld, pop time=%s, slot=%d\n",
                           pfp(this),
                           iv_user_param,
                           format_pop_time(la_fmt_pop),
                           lv_slot);

    if (pv_lock) {
        lv_status = cv_mutex.lock();
        SB_util_assert_ieq(lv_status, 0); // sw fault
    }

    lp_curr = ca_slots[lv_slot];
    SB_util_assert_pne(lp_curr, NULL);

    if (lp_curr == this) {
        ca_slots[lv_slot] = ip_next;

        ip_next = NULL;
        iv_running = false;

        if (pv_lock) {
            lv_status = cv_mutex.unlock();
            SB_util_assert_ieq(lv_status, 0); // sw fault
        }
        return;
    }

    while (lp_curr != NULL) {
        lp_next = lp_curr->ip_next;
        if (lp_next == this) {
            lp_curr->ip_next = ip_next;

            ip_next = NULL;
            iv_running = false;

            if (pv_lock) {
                lv_status = cv_mutex.unlock();
                SB_util_assert_ieq(lv_status, 0); // sw fault
            }
            return;
        }

        lp_curr = lp_next;
    }

    //
    // shouldn't get here if timer is running.
    //
    SB_util_assert_bt(false);
}

void SB_Timer::Timer::check_timers() {
    const char  *WHERE = "Timer::check_timers";
    char         la_fmt_pop[100];
    Timer       *lp_curr;
    Timer       *lp_next;
    Time_Stamp   lv_now;
    Slot_Type    lv_now_slot;
    Slot_Type    lv_slot;
    int          lv_status;

    lp_next = NULL;
    lv_now_slot = hash(lv_now);

    lv_status = cv_mutex.lock();
    SB_util_assert_ieq(lv_status, 0); // sw fault

    for (lv_slot = cv_last_slot_checked;
         lv_slot != (lv_now_slot + 1) % MAX_SLOTS;
         lv_slot = (lv_slot + 1) % MAX_SLOTS) {
        lp_curr = ca_slots[lv_slot];
        while (lp_curr != NULL) {
            //
            // we're done if the timer pops later than now.
            //
            if (lp_curr->iv_pop_time.ts_gt(lv_now)) {
                lv_status = cv_mutex.unlock();
                SB_util_assert_ieq(lv_status, 0); // sw fault
                if (cv_trace_enabled)
                    trace_where_printf(WHERE, "no timers ready\n");
                return;
            }

            lp_next = lp_curr->ip_next;
            lp_curr->cancel_int(false);

            if (cv_trace_enabled)
                trace_where_printf(WHERE,
                                   "timer=%p, user-param=%ld, pop time=%s\n",
                                   pfp(lp_curr),
                                   lp_curr->iv_user_param,
                                   lp_curr->format_pop_time(la_fmt_pop));

            lp_curr->ip_th->handle_timeout(lp_curr);

            lp_curr = lp_next;
        }

        if (lv_slot != lv_now_slot) {
            // Only increment if not at "now".  A new timer might be
            // set in the current time slot before "now" goes to the next
            // slot so we want to be sure and check it again.
            cv_last_slot_checked = (cv_last_slot_checked + 1) % MAX_SLOTS;
        }
    }

    lv_status = cv_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
}

const char *SB_Timer::Timer::format_timer(char *pp_buf) {
    char la_buf[100];

    format_pop_time(la_buf);
    sprintf(pp_buf, "timer=%p, user-param=%ld, interval=" PF64 ", pop-time=%s",
            pfp(this), iv_user_param, iv_interval, la_buf);
    return pp_buf;
}

const char *SB_Timer::Timer::format_pop_time(char *pp_buf) {
    iv_pop_time.format_ts(pp_buf);
    return pp_buf;
}

SB_Timer::Tics SB_Timer::Timer::get_wait_time() {
    const char   *WHERE = "Timer::get_wait_time";
    Timer        *lp_curr;
    static bool   lv_1st_time = true;
    Slot_Type     lv_default_slot;
    Tics          lv_result;
    Slot_Type     lv_slot;
    int           lv_status;

    if (lv_1st_time) {
        lv_1st_time = false;
        lv_result = Time_Stamp::TICS_PER_SEC;
        if (cv_trace_enabled)
            trace_where_printf(WHERE, "result=" PF64 "\n", lv_result);
        return lv_result;
    }

    lv_status = cv_mutex.lock();
    SB_util_assert_ieq(lv_status, 0); // sw fault

    lv_result = DEFAULT_WAIT_TICS;
    lv_default_slot =
      (cv_last_slot_checked + DEFAULT_SLOT_COUNT) % MAX_SLOTS;

    //
    // check for any timers ready to pop between the last slot checked
    // and the slot of the default wake time.
    //
    for (lv_slot = cv_last_slot_checked;
         lv_slot != lv_default_slot;
         lv_slot = (lv_slot + 1) % MAX_SLOTS) {
        lp_curr = ca_slots[lv_slot];
        if (lp_curr != NULL) {
            Time_Stamp lv_now;
            lv_result = lp_curr->iv_pop_time.ts_sub(lv_now);
            if (lv_result < 0)
                lv_result = 0;

            if (cv_trace_enabled)
                trace_where_printf(WHERE, "result=" PF64 "\n", lv_result);

            lv_status = cv_mutex.unlock();
            SB_util_assert_ieq(lv_status, 0); // sw fault
            return lv_result;
        }
    }

    if (cv_trace_enabled)
        trace_where_printf(WHERE, "result=" PF64 " (default)\n", lv_result);

    lv_status = cv_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
    return lv_result;
}

SB_Timer::Timer::Slot_Type SB_Timer::Timer::hash(Time_Stamp &pr_slot_time) {
    Slot_Type lv_result;

    lv_result =
      static_cast<Slot_Type>((pr_slot_time.iv_tics / TICS_PER_SLOT) % MAX_SLOTS);

    return lv_result;
}

void SB_Timer::Timer::init() {
    Slot_Type lv_slot;
    int       lv_status;

    lv_status = cv_mutex.lock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
    for (lv_slot = 0; lv_slot < MAX_SLOTS; lv_slot++)
        ca_slots[lv_slot] = NULL;
    lv_status = cv_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
}

void SB_Timer::Timer::print_timers(Print_Timer_Cb pv_cb) {
    Timer      *lp_curr;
    Slot_Type   lv_slot;
    int         lv_status;

    lv_status = cv_mutex.lock();
    SB_util_assert_ieq(lv_status, 0); // sw fault

    for (lv_slot = 0;
         lv_slot < MAX_SLOTS;
         lv_slot++) {
        lp_curr = ca_slots[lv_slot];
        while (lp_curr != NULL) {
            pv_cb(*lp_curr);
            lp_curr = lp_curr->ip_next;
        }
    }

    lv_status = cv_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0); // sw fault

}

void SB_Timer::Timer::set_interval(Tics pv_interval, bool pv_start) {
    const char *WHERE = "Timer::set_interval";

    if (pv_interval > ((MAX_SLOTS-1) * TICS_PER_SLOT))
        // Don't allow interval that would cause a wrap-around of
        // the timer slots array.
        pv_interval = (MAX_SLOTS-1) * TICS_PER_SLOT;
    else if (pv_interval < 0)
        pv_interval = 0;

    iv_interval = pv_interval;

    if (cv_trace_enabled)
        trace_where_printf(WHERE,
                           "timer=%p, user-param=%ld, interval=" PF64 ", start=%d\n",
                           pfp(this),
                           iv_user_param,
                           iv_interval,
                           pv_start);

    if (pv_start)
        start();
}

void SB_Timer::Timer::set_param(long pv_user_param) {
    const char *WHERE = "Timer::set_param";

    if (cv_trace_enabled)
        trace_where_printf(WHERE,
                           "timer=%p, user-param=%ld\n",
                           pfp(this), pv_user_param);
    iv_user_param = pv_user_param;
}

void SB_Timer::Timer::start() {
    const char *WHERE = "Timer::start";
    char        la_fmt_pop[100];
    Timer      *lp_curr;
    Timer      *lp_next;
    const char *lp_trace_type;
    Slot_Type   lv_slot;
    int         lv_status;

    if (iv_running) {
        lp_trace_type = "(re)start";
        cancel_int(true);
    } else
        lp_trace_type = "start";

    iv_pop_time.tic_set_now_add(iv_interval);
    lv_slot = hash(iv_pop_time);

    if (cv_trace_enabled)
        trace_where_printf(WHERE,
                          "%s, timer=%p, user-param=%ld, interval=" PF64 ", pop time=%s, slot=%d\n",
                          lp_trace_type,
                          pfp(this),
                          iv_user_param,
                          iv_interval,
                          format_pop_time(la_fmt_pop), lv_slot);

    lv_status = cv_mutex.lock();
    SB_util_assert_ieq(lv_status, 0); // sw fault

    lp_curr = ca_slots[lv_slot];

    // is it the only one in this slot?
    if (lp_curr == NULL) {
        ca_slots[lv_slot] = this;
        ip_next = NULL;
    } else if (iv_pop_time.ts_lt(lp_curr->iv_pop_time)) {
        // should it go first in this slot?
        ca_slots[lv_slot] = this;
        ip_next = lp_curr;
    } else {
        // it goes somewhere after the first
        lp_next = lp_curr->ip_next;

        while ((lp_next != NULL) &&
               (iv_pop_time.ts_ge(lp_next->iv_pop_time))) {
            lp_curr = lp_next;
            lp_next = lp_curr->ip_next;
        }

        lp_curr->ip_next = this;
        ip_next = lp_next;
    }

    iv_running = true;

    lv_status = cv_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
}

