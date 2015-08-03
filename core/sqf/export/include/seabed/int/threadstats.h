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

#ifndef __SB_INT_THREADSTATS_H_
#define __SB_INT_THREADSTATS_H_

#include <limits.h>
#include <stdio.h>        // printf
#include <string.h>
#include <unistd.h>

#include <linux/unistd.h> // gettid

#include <sys/time.h>     // gettimeofday

#include "seabed/debug.h"

#define SB_gettid() syscall(__NR_gettid)

// uncomment and change 'dp2' to exe to target
//#define SB_THREAD_LOCK_STATS_EXE               "dp2"
// uncomment and change '2000' to number of us to exceed to assert
//#define SB_THREAD_LOCK_STATS_ASSERT_MUTEX_GET  2000
// uncomment and change '2000' to number of us to exceed to assert
//#define SB_THREAD_LOCK_STATS_ASSERT_MUTEX_HOLD 2000
// uncomment and change '2000' to number of us to exceed to assert
//#define SB_THREAD_LOCK_STATS_ASSERT_SPIN_GET   2000
// uncomment and change '2000' to number of us to exceed to assert
//#define SB_THREAD_LOCK_STATS_ASSERT_SPIN_HOLD  2000

typedef long long SB_Thread_Lock_Counter;
typedef long long SB_Thread_Lock_Time;

class SB_Thread_Lock_Stats {
public:
    SB_Thread_Lock_Stats()
    : iv_mutex_count(0), iv_mutex_time_get_lock(0),
      iv_spin_count(0), iv_spin_time_get_lock(0),
      iv_mutex_times_inx_get_lock(0), iv_mutex_times_inx_hold_lock(0),
      iv_spin_times_inx_get_lock(0), iv_spin_times_inx_hold_lock(0) {
    }

    ~SB_Thread_Lock_Stats() {
    }

    SB_Thread_Lock_Counter inc(SB_Thread_Lock_Counter *pp_counter) {
        return __sync_fetch_and_add_8(pp_counter, 1);
    }

    void print(const char *pp_where) {
        int lv_count;
        int lv_elapsed;
        int lv_inx;
        int lv_tid;

        printf("LOCK STATS %s:\n", pp_where);
        printf("  mutex count=%lld, get-lock-time=%lld\n",
               iv_mutex_count, iv_mutex_time_get_lock);
        printf("  spin count=%lld, get-lock-time=%lld\n",
               iv_spin_count, iv_spin_time_get_lock);
        lv_count = iv_mutex_times_inx_get_lock % MAX_COUNT;
        for (lv_inx = 0; lv_inx <= lv_count; lv_inx++) {
            lv_elapsed = static_cast<int>(ia_mutex_times_get_lock[lv_inx]);
            lv_tid = static_cast<int>(ia_mutex_times_get_lock[lv_inx] >> 32);
            printf("  mutex-get-et[%d]=%d, tid=%d\n", lv_inx, lv_elapsed, lv_tid);
        }
        lv_count = iv_mutex_times_inx_hold_lock % MAX_COUNT;
        for (lv_inx = 0; lv_inx <= lv_count; lv_inx++) {
            lv_elapsed = static_cast<int>(ia_mutex_times_hold_lock[lv_inx]);
            lv_tid = static_cast<int>(ia_mutex_times_hold_lock[lv_inx] >> 32);
            printf("  mutex-hold-et[%d]=%d, tid=%d\n", lv_inx, lv_elapsed, lv_tid);
        }
        lv_count = iv_spin_times_inx_get_lock % MAX_COUNT;
        for (lv_inx = 0; lv_inx <= lv_count; lv_inx++) {
            lv_elapsed = static_cast<int>(ia_spin_times_get_lock[lv_inx]);
            lv_tid = static_cast<int>(ia_spin_times_get_lock[lv_inx] >> 32);
            printf("  spin-get-et[%d]=%d, tid=%d\n", lv_inx, lv_elapsed, lv_tid);
        }
        lv_count = iv_spin_times_inx_hold_lock % MAX_COUNT;
        for (lv_inx = 0; lv_inx <= lv_count; lv_inx++) {
            lv_elapsed = static_cast<int>(ia_spin_times_hold_lock[lv_inx]);
            lv_tid = static_cast<int>(ia_spin_times_hold_lock[lv_inx] >> 32);
            printf("  spin-hold-et[%d]=%d, tid=%d\n", lv_inx, lv_elapsed, lv_tid);
        }
        fflush(stdout);
    }

    void print_bt() {
        enum { MAX_PRINT_COUNT = 50 };
        enum { MAX_PRINT_SIZE = 200 };
        char la_recs[MAX_PRINT_COUNT][MAX_PRINT_SIZE];
        int  lv_count;
        int  lv_inx;

        SB_backtrace2(MAX_PRINT_COUNT,
                      MAX_PRINT_SIZE,
                      &lv_count,
                      (char *) la_recs);
        for (lv_inx = 0; lv_inx < lv_count; lv_inx++)
            printf("%s\n", la_recs[lv_inx]);
    }

#ifdef SB_THREAD_LOCK_STATS_EXE
    bool check_exe() {
        const char  *EXE = SB_THREAD_LOCK_STATS_EXE;
        char         la_self[PATH_MAX];
        char        *lp_p;
        static bool  lv_match = false;
        static bool  lv_readlink = false;
        ssize_t      lv_size;

        if (!lv_readlink) {
            lv_readlink = true;
            lv_size = readlink("/proc/self/exe", la_self, PATH_MAX);
            if (lv_size != -1) {
                la_self[lv_size] = 0;
                lp_p = rindex(la_self, '/');
                if (lp_p != NULL)
                    lp_p++;
                else
                    lp_p = la_self;
                if (strcmp(lp_p, EXE) == 0)
                    lv_match = true;
            }
        }
        return lv_match;
    }
#else
    bool check_exe() {
        return true;
    }
#endif

    SB_Thread_Lock_Time get_lock_time_set_mutex_get_lock(SB_Thread_Lock_Time pv_start) {
        SB_Thread_Lock_Time lv_elapsed;
        int                 lv_slot;
        SB_Thread_Lock_Time lv_stop;
        struct timeval      lv_tod;

        gettimeofday(&lv_tod, NULL);
        lv_stop = static_cast<SB_Thread_Lock_Time>(lv_tod.tv_sec) * 1000000 +
                  lv_tod.tv_usec;
        lv_elapsed = lv_stop - pv_start;
        lv_slot = __sync_fetch_and_add_4(&iv_mutex_times_inx_get_lock, 1);
        lv_slot %= MAX_COUNT;
        ia_mutex_times_get_lock[lv_slot] =
          (static_cast<SB_Thread_Lock_Time>(SB_gettid()) << 32) + lv_elapsed;
#ifdef SB_THREAD_LOCK_STATS_ASSERT_MUTEX_GET
        if (lv_elapsed > SB_THREAD_LOCK_STATS_ASSERT_MUTEX_GET) {
            if (check_exe()) {
                printf("mutex get-lock=%lld\n", lv_elapsed);
                print_bt();
                print("mutex get_lock");
                SB_util_assert(lv_elapsed <= SB_THREAD_LOCK_STATS_ASSERT_MUTEX_GET);
            }
        }
#endif
        __sync_add_and_fetch_8(&iv_mutex_time_get_lock, lv_elapsed);
        return lv_stop;
    }

    void get_lock_time_set_mutex_hold_lock(SB_Thread_Lock_Time pv_start) {
        SB_Thread_Lock_Time lv_elapsed;
        int                 lv_slot;
        SB_Thread_Lock_Time lv_stop;
        struct timeval      lv_tod;

        gettimeofday(&lv_tod, NULL);
        lv_stop = static_cast<SB_Thread_Lock_Time>(lv_tod.tv_sec) * 1000000 +
                  lv_tod.tv_usec;
        lv_elapsed = lv_stop - pv_start;
        lv_slot = __sync_fetch_and_add_4(&iv_mutex_times_inx_hold_lock, 1);
        lv_slot %= MAX_COUNT;
        ia_mutex_times_hold_lock[lv_slot] =
          (static_cast<SB_Thread_Lock_Time>(SB_gettid()) << 32) + lv_elapsed;
#ifdef SB_THREAD_LOCK_STATS_ASSERT_MUTEX_HOLD
        if (lv_elapsed > SB_THREAD_LOCK_STATS_ASSERT_MUTEX_HOLD) {
            if (check_exe()) {
                printf("mutex hold-lock=%lld\n", lv_elapsed);
                print_bt();
                print("mutex hold-lock");
                SB_util_assert(lv_elapsed <= SB_THREAD_LOCK_STATS_ASSERT_MUTEX_HOLD);
            }
        }
#endif
        __sync_add_and_fetch_8(&iv_mutex_time_hold_lock, lv_elapsed);
    }

    SB_Thread_Lock_Time get_lock_time_set_spin_get_lock(SB_Thread_Lock_Time pv_start) {
        SB_Thread_Lock_Time lv_elapsed;
        int                 lv_slot;
        SB_Thread_Lock_Time lv_stop;
        struct timeval      lv_tod;

        gettimeofday(&lv_tod, NULL);
        lv_stop = static_cast<SB_Thread_Lock_Time>(lv_tod.tv_sec) * 1000000 +
                  lv_tod.tv_usec;
        lv_elapsed = lv_stop - pv_start;
        lv_slot = __sync_fetch_and_add_4(&iv_spin_times_inx_get_lock, 1);
        lv_slot %= MAX_COUNT;
        ia_spin_times_get_lock[lv_slot] =
          (static_cast<SB_Thread_Lock_Time>(SB_gettid()) << 32) + lv_elapsed;
#ifdef SB_THREAD_LOCK_STATS_ASSERT_SPIN_GET
        if (lv_elapsed > SB_THREAD_LOCK_STATS_ASSERT_SPIN_GET) {
            if (check_exe()) {
                printf("spin get-lock=%lld\n", lv_elapsed);
                print_bt("spin get-lock");
                print();
                SB_util_assert(lv_elapsed <= SB_THREAD_LOCK_STATS_ASSERT_SPIN_GET);
            }
        }
#endif
        __sync_add_and_fetch_8(&iv_spin_time_get_lock, lv_elapsed);
        return lv_stop;
    }

    void get_lock_time_set_spin_hold_lock(SB_Thread_Lock_Time pv_start) {
        SB_Thread_Lock_Time lv_elapsed;
        int                 lv_slot;
        SB_Thread_Lock_Time lv_stop;
        struct timeval      lv_tod;

        gettimeofday(&lv_tod, NULL);
        lv_stop = static_cast<SB_Thread_Lock_Time>(lv_tod.tv_sec) * 1000000 +
                  lv_tod.tv_usec;
        lv_elapsed = lv_stop - pv_start;
        lv_slot = __sync_fetch_and_add_4(&iv_spin_times_inx_hold_lock, 1);
        lv_slot %= MAX_COUNT;
        ia_spin_times_hold_lock[lv_slot] =
          (static_cast<SB_Thread_Lock_Time>(SB_gettid()) << 32) + lv_elapsed;
#ifdef SB_THREAD_LOCK_STATS_ASSERT_SPIN_HOLD
        if (lv_elapsed > SB_THREAD_LOCK_STATS_ASSERT_SPIN_HOLD) {
            if (check_exe()) {
                printf("spin hold-lock=%lld\n", lv_elapsed);
                print_bt("spin hold-lock");
                print();
                SB_util_assert(lv_elapsed <= SB_THREAD_LOCK_STATS_ASSERT_SPIN_HOLD);
            }
        }
#endif
        __sync_add_and_fetch_8(&iv_spin_time_hold_lock, lv_elapsed);
    }

    SB_Thread_Lock_Time time_start() {
        SB_Thread_Lock_Time lv_ret;
        struct timeval      lv_tod;

        gettimeofday(&lv_tod, NULL);
        lv_ret = static_cast<SB_Thread_Lock_Time>(lv_tod.tv_sec) * 1000000 +
                 lv_tod.tv_usec;
        return lv_ret;
    }

public:
    SB_Thread_Lock_Counter iv_mutex_count;
    SB_Thread_Lock_Time    iv_mutex_time_get_lock;
    SB_Thread_Lock_Time    iv_mutex_time_hold_lock;
    SB_Thread_Lock_Counter iv_spin_count;
    SB_Thread_Lock_Time    iv_spin_time_get_lock;
    SB_Thread_Lock_Time    iv_spin_time_hold_lock;

private:
    enum { MAX_COUNT = 20000 };
    SB_Thread_Lock_Time    ia_mutex_times_get_lock[MAX_COUNT];
    SB_Thread_Lock_Time    ia_mutex_times_hold_lock[MAX_COUNT];
    int                    iv_mutex_times_inx_get_lock;
    int                    iv_mutex_times_inx_hold_lock;
    SB_Thread_Lock_Time    ia_spin_times_get_lock[MAX_COUNT];
    SB_Thread_Lock_Time    ia_spin_times_hold_lock[MAX_COUNT];
    int                    iv_spin_times_inx_get_lock;
    int                    iv_spin_times_inx_hold_lock;
};

extern SB_Thread_Lock_Stats gv_sb_lock_stats;

#endif // !__SB_INT_THREADSTATS_H_
