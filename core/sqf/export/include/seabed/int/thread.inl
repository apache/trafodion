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

#ifndef __SB_INT_THREAD_INL_
#define __SB_INT_THREAD_INL_

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#include <linux/unistd.h> // gettid

#define SB_gettid() syscall(__NR_gettid)

#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
#include "seabed/trace.h"
extern bool gv_sb_trace_pthread;
#endif

SB_INLINE void SB_Thread::Sthr::nsleep(SB_Int64_Type pv_ns) {
    int             lv_err;
    struct timespec lv_tv;

    lv_tv.tv_sec = static_cast<time_t>(pv_ns / SB_NS_PER_SEC);
    lv_tv.tv_nsec = static_cast<long>(pv_ns - lv_tv.tv_sec * SB_NS_PER_SEC);
    lv_err = nanosleep(&lv_tv, NULL);
    if (lv_err)
        SB_util_assert_ieq(errno, EINTR);
}

SB_INLINE long SB_Thread::Sthr::self_id() {
    long lv_tid = SB_gettid();
    return lv_tid;
}

SB_INLINE void SB_Thread::Sthr::sleep(int pv_ms) {
    int             lv_err;
    struct timespec lv_tv;

    lv_tv.tv_sec = pv_ms / SB_MS_PER_SEC;
    lv_tv.tv_nsec = (pv_ms - lv_tv.tv_sec * SB_MS_PER_SEC) * SB_NS_PER_MS; // ms
    lv_err = nanosleep(&lv_tv, NULL);
    if (lv_err)
        SB_util_assert_ieq(errno, EINTR);
}

SB_INLINE void *SB_Thread::Sthr::specific_get(int pv_key) {
    void *lp_data = pthread_getspecific(pv_key);
    //a temp solution for Trafodion to running on CentOS 7
    //Not understanding why lp_data will become 0x01
    if(lp_data == (void *)0x01) return NULL;
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_getspecific(%d) EXIT, ret=%p\n", pv_key, lp_data);
#endif
    return lp_data;
}

SB_INLINE int SB_Thread::Sthr::specific_key_create(int &pr_new_key, Dtor_Function pp_dtor) {
    pthread_key_t lv_key;
    int lv_status = pthread_key_create(&lv_key, pp_dtor);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_key_create(%d) EXIT, ret=%d\n", static_cast<int>(lv_key), lv_status);
#endif
    pr_new_key = lv_key;
    return lv_status;
}

SB_INLINE int SB_Thread::Sthr::specific_key_create2(Dtor_Function pp_dtor) {
    pthread_key_t lv_key;
    int           lv_status;

    lv_status = pthread_key_create(&lv_key, pp_dtor);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_key_create EXIT, status=%d, ret=%d\n", lv_status, static_cast<int>(lv_key));
#endif
    SB_util_assert_ieq(lv_status, 0);
    return lv_key;
}

SB_INLINE int SB_Thread::Sthr::specific_set(int pv_key, const void *pp_data) {
    int lv_status = pthread_setspecific(pv_key, pp_data);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_setspecific(%d,%p) EXIT, ret=%d\n",
                     pv_key, pp_data, lv_status);
#endif
    return lv_status;
}

SB_INLINE SB_Int64_Type SB_Thread::Sthr::time() {
    struct timespec lv_ts;

    clock_gettime(CLOCK_REALTIME, &lv_ts);
    SB_Int64_Type lv_ret =
      static_cast<SB_Int64_Type>(lv_ts.tv_sec) * SB_MS_PER_SEC +
      lv_ts.tv_nsec / SB_NS_PER_MS;
    return lv_ret;
}

SB_INLINE void SB_Thread::Sthr::time(Time_Type *pp_time) {
    struct timespec lv_ts;

    clock_gettime(CLOCK_REALTIME, &lv_ts);
    pp_time->sec = lv_ts.tv_sec;
    pp_time->ms = static_cast<int>(lv_ts.tv_nsec / SB_NS_PER_MS);
}

SB_INLINE SB_Int64_Type SB_Thread::Sthr::time_us() {
    struct timespec lv_ts;

    clock_gettime(CLOCK_REALTIME, &lv_ts);
    SB_Int64_Type lv_ret =
      static_cast<SB_Int64_Type>(lv_ts.tv_sec) * SB_US_PER_SEC +
      lv_ts.tv_nsec / SB_NS_PER_US;
    return lv_ret;
}

SB_INLINE void SB_Thread::Sthr::usleep(int pv_us) {
    int             lv_err;
    struct timespec lv_tv;

    lv_tv.tv_sec = pv_us / SB_US_PER_SEC;
    lv_tv.tv_nsec = (pv_us - lv_tv.tv_sec * SB_US_PER_SEC) * SB_NS_PER_US;
    lv_err = nanosleep(&lv_tv, NULL);
    if (lv_err)
        SB_util_assert_ieq(errno, EINTR);
}

SB_INLINE SB_Int64_Type SB_Thread::Sthr::utime() {
    struct timespec lv_ts;

    clock_gettime(CLOCK_REALTIME, &lv_ts);
    SB_Int64_Type lv_ret =
      static_cast<SB_Int64_Type>(lv_ts.tv_sec) * SB_US_PER_SEC +
      lv_ts.tv_nsec / SB_NS_PER_US;
    return lv_ret;
}

SB_INLINE void SB_Thread::Sthr::utime(Utime_Type *pp_time) {
    struct timespec lv_ts;

    clock_gettime(CLOCK_REALTIME, &lv_ts);
    pp_time->sec = lv_ts.tv_sec;
    pp_time->us = static_cast<int>(lv_ts.tv_nsec / SB_NS_PER_US);
}

SB_INLINE void SB_Thread::Sthr::yield() {
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("sched_yield() ENTRY\n");
#endif
    sched_yield();
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("sched_yield() EXIT\n");
#endif
}

// needed by CV
SB_INLINE int SB_Thread::Mutex::destroy() {
    int lv_status;

    if (iv_destroyed)
        lv_status = 0;
    else {
        lv_status = pthread_mutex_destroy(&iv_mutex);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
        if (gv_sb_trace_pthread)
            trace_printf("pthread_mutex_destroy(%p-%s) EXIT, ret=%d\n",
                         (void *) &iv_mutex, ip_mutex_name, lv_status);
#endif
        if (lv_status == 0)
            iv_destroyed = true;
    }
    return lv_status;
}

// needed by CV
SB_INLINE int SB_Thread::Mutex::lock() {
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("pthread_mutex_lock(%p-%s) ENTRY\n",
                     (void *) &iv_mutex, ip_mutex_name);
  #else
        trace_printf("pthread_mutex_lock(%p-%s) ENTRY\n",
                     (void *) ip_mutex, ip_mutex_name);
  #endif
#endif
#ifdef SB_THREAD_LOCK_STATS
    gv_sb_lock_stats.inc(&gv_sb_lock_stats.iv_mutex_count);
    SB_Thread_Lock_Time lv_lock_start = gv_sb_lock_stats.time_start();
#endif
#ifdef SB_THREAD_LINUX
    int lv_status = pthread_mutex_lock(&iv_mutex);
  #ifdef SB_THREAD_LOCK_STATS
    if (lv_status == 0) {
        ia_lock_bt[0] = __builtin_return_address(0);
        ia_lock_bt[1] = __builtin_return_address(1);
        ia_lock_bt[2] = __builtin_return_address(2);
    }
  #endif
#else
    int lv_status = pthread_mutex_lock(reinterpret_cast<pthread_mutex_t *>(ip_mutex));
#endif
#ifdef SB_THREAD_LOCK_STATS
    iv_lock_start = gv_sb_lock_stats.get_lock_time_set_mutex_get_lock(lv_lock_start);
    iv_last_locker = gettid();
#endif
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("pthread_mutex_lock(%p-%s) EXIT, ret=%d\n",
                     (void *) &iv_mutex, ip_mutex_name, lv_status);
  #else
        trace_printf("pthread_mutex_lock(%p-%s) EXIT, ret=%d\n",
                     (void *) ip_mutex, ip_mutex_name, lv_status);
  #endif
#endif
    return lv_status;
}

// needed by CV
SB_INLINE void SB_Thread::Mutex::setname(const char *pp_name) {
    ip_mutex_name = pp_name;
}

// needed by CV
SB_INLINE int SB_Thread::Mutex::trylock() {
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("pthread_mutex_trylock(%p-%s) ENTRY\n",
                     (void *) &iv_mutex, ip_mutex_name);
  #else
        trace_printf("pthread_mutex_trylock(%p-%s) ENTRY\n",
                     (void *) ip_mutex, ip_mutex_name);
  #endif
#endif
#ifdef SB_THREAD_LINUX
    int lv_status = pthread_mutex_trylock(&iv_mutex);
#else
    int lv_status = pthread_mutex_trylock(reinterpret_cast<pthread_mutex_t *>(ip_mutex));
#endif
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("pthread_mutex_trylock(%p-%s) EXIT, ret=%d\n",
                     (void *) &iv_mutex, ip_mutex_name, lv_status);
  #else
        trace_printf("pthread_mutex_trylock(%p-%s) EXIT, ret=%d\n",
                     (void *) ip_mutex, ip_mutex_name, lv_status);
  #endif
#endif
    return lv_status;
}

// needed by CV
SB_INLINE int SB_Thread::Mutex::unlock() {
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("pthread_mutex_unlock(%p-%s) ENTER\n",
                     (void *) &iv_mutex, ip_mutex_name);
  #else
        trace_printf("pthread_mutex_unlock(%p-%s) ENTER\n",
                     (void *) ip_mutex, ip_mutex_name);
  #endif
#endif
#ifdef SB_THREAD_LOCK_STATS
    if (dynamic_cast<CV *>(this) == NULL)
        gv_sb_lock_stats.get_lock_time_set_mutex_hold_lock(iv_lock_start);
    ia_lock_bt[0] = NULL;
#endif
#ifdef SB_THREAD_LINUX
    int lv_status = pthread_mutex_unlock(&iv_mutex);
#else
    int lv_status = pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t *>(ip_mutex));
#endif
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("pthread_mutex_unlock(%p-%s) EXIT, ret=%d\n",
                     (void *) &iv_mutex, ip_mutex_name, lv_status);
  #else
        trace_printf("pthread_mutex_unlock(%p-%s) EXIT, ret=%d\n",
                     (void *) ip_mutex, ip_mutex_name, lv_status);
  #endif
#endif
    return lv_status;
}

SB_INLINE int SB_Thread::Errorcheck_Mutex::lock() {
    int lv_status;

    lv_status = Mutex::lock();
    if (lv_status == 0)
        iv_locked = true;
    return lv_status;
}

SB_INLINE bool SB_Thread::Errorcheck_Mutex::locked() {
    return iv_locked;
}

SB_INLINE int SB_Thread::Errorcheck_Mutex::trylock() {
    int lv_status;

    lv_status = Mutex::trylock();
    if (lv_status == 0)
        iv_locked = true;
    return lv_status;
}

SB_INLINE int SB_Thread::Errorcheck_Mutex::unlock() {
    int lv_status;

    lv_status = Mutex::unlock();
    if (lv_status == 0)
        iv_locked = false;
    return lv_status;
}

SB_INLINE int SB_Thread::CV::broadcast() {
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("pthread_cond_broadcast(%p-%s) ENTER\n",
                     (void *) &iv_cv, ip_mutex_name);
  #else
        trace_printf("pthread_cond_broadcast(%p-%s) ENTER\n",
                     (void *) ip_cv, ip_mutex_name);
  #endif
#endif
#ifdef SB_THREAD_LINUX
    int lv_status = pthread_cond_broadcast(&iv_cv);
#else
    int lv_status = pthread_cond_broadcast(reinterpret_cast<pthread_cond_t *>(ip_cv));
#endif
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("pthread_cond_broadcast(%p-%s) EXIT, ret=%d\n",
                     (void *) &iv_cv, ip_mutex_name, lv_status);
  #else
        trace_printf("pthread_cond_broadcast(%p-%s) EXIT, ret=%d\n",
                     (void *) ip_cv, ip_mutex_name, lv_status);
  #endif
#endif

    return lv_status;
}

SB_INLINE int SB_Thread::CV::broadcast(bool pv_lock) {
    int lv_ret;
    int lv_status;

#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("CV::broadcast(%p-%s) ENTER\n",
                     (void *) &iv_cv, ip_mutex_name);
  #else
        trace_printf("CV::broadcast(%p-%s) ENTER\n",
                     (void *) ip_cv, ip_mutex_name);
  #endif
#endif
    if (pv_lock) {
        lv_status = lock();
        SB_util_assert_ieq(lv_status, 0);
        lv_ret = broadcast(); // locked
        lv_status = unlock();
        SB_util_assert_ieq(lv_status, 0);
    } else
        lv_ret = broadcast(); // no lock necessary
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("CV::broadcast(%p-%s) EXIT, ret=%d\n",
                     (void *) &iv_cv, ip_mutex_name, lv_ret);
  #else
        trace_printf("CV::broadcast(%p-%s) EXIT, ret=%d\n",
                     (void *) ip_cv, ip_mutex_name, lv_ret);
  #endif
#endif
    return lv_ret;
}

SB_INLINE int SB_Thread::CV::destroy() {
    int lv_status;

    if (iv_destroyed)
        lv_status = 0;
    else {
        do {
            lv_status = Mutex::destroy();
            if (lv_status == EBUSY)
                usleep(100);
        } while (lv_status);

        lv_status = pthread_cond_destroy(&iv_cv);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
        if (gv_sb_trace_pthread)
            trace_printf("pthread_cond_destroy(%p-%s) EXIT, ret=%d\n",
                         (void *) &iv_cv, ip_mutex_name, lv_status);
#endif
        if (lv_status == 0) 
            iv_destroyed = true;
    }
    return lv_status;
}

SB_INLINE int SB_Thread::CV::lock() {
    return Mutex::lock();
}

SB_INLINE void SB_Thread::CV::reset_flag() {
    iv_flag = false;
}

SB_INLINE int SB_Thread::CV::signal() {
    iv_flag = true;

#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("pthread_cond_signal(%p-%s) ENTER\n",
                     (void *) &iv_cv, ip_mutex_name);
  #else
        trace_printf("pthread_cond_signal(%p-%s) ENTER\n",
                     (void *) ip_cv, ip_mutex_name);
  #endif
#endif
#ifdef SB_THREAD_LINUX
    int lv_status = pthread_cond_signal(&iv_cv);
#else
    int lv_status = pthread_cond_signal(reinterpret_cast<pthread_cond_t *>(ip_cv));
#endif
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("pthread_cond_signal(%p-%s) EXIT, ret=%d\n",
                     (void *) &iv_cv, ip_mutex_name, lv_status);
  #else
        trace_printf("pthread_cond_signal(%p-%s) EXIT, ret=%d\n",
                     (void *) ip_cv, ip_mutex_name, lv_status);
  #endif
#endif

    return lv_status;
}

SB_INLINE int SB_Thread::CV::signal(bool pv_lock) {
    int lv_ret;
    int lv_status;

#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("CV::signal(%p-%s) ENTER\n",
                     (void *) &iv_cv, ip_mutex_name);
  #else
        trace_printf("CV::signal(%p-%s) ENTER\n",
                     (void *) ip_cv, ip_mutex_name);
  #endif
#endif
    if (pv_lock) {
        lv_status = lock();
        SB_util_assert_ieq(lv_status, 0);
        lv_ret = signal(); // locked
        lv_status = unlock();
        SB_util_assert_ieq(lv_status, 0);
    } else
        lv_ret = signal(); // no lock necessary
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("CV::signal(%p-%s) EXIT, ret=%d\n",
                     (void *) &iv_cv, ip_mutex_name, lv_ret);
  #else
        trace_printf("CV::signal(%p-%s) EXIT, ret=%d\n",
                     (void *) ip_cv, ip_mutex_name, lv_ret);
  #endif
#endif
    return lv_ret;
}

SB_INLINE bool SB_Thread::CV::signaled() {
    return iv_flag;
}

SB_INLINE int SB_Thread::CV::trylock() {
    return Mutex::trylock();
}

SB_INLINE int SB_Thread::CV::unlock() {
    return Mutex::unlock();
}

SB_INLINE int SB_Thread::CV::wait() {
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("CV:::wait ENTRY cv=%p\n", (void *) &iv_cv);
  #else
        trace_printf("CV:::wait ENTRY cv=%p\n", (void *) ip_cv);
  #endif
#endif
    if (iv_flag) {
        iv_flag = false;
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
        if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
            trace_printf("CV:::wait EXIT cv=%p\n", (void *) &iv_cv);
  #else
            trace_printf("CV:::wait EXIT cv=%p\n", (void *) ip_cv);
  #endif
#endif
        return 0;
    }

#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("pthread_cond_wait(%p,%p) ENTRY\n",
                     (void *) &iv_cv, (void *) &iv_mutex);
  #else
        trace_printf("pthread_cond_wait(%p,%p) ENTRY\n",
                     (void *) ip_cv, (void *) ip_mutex);
  #endif
#endif
#ifdef SB_THREAD_LINUX
    int lv_status = pthread_cond_wait(&iv_cv, &iv_mutex);
#else
    int lv_status = pthread_cond_wait(reinterpret_cast<pthread_cond_t *>(ip_cv),
                                      reinterpret_cast<pthread_mutex_t *>(ip_mutex));
#endif
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("pthread_cond_wait(%p,%p) EXIT, ret=%d\n",
                     (void *) &iv_cv, (void *) &iv_mutex, lv_status);
  #else
        trace_printf("pthread_cond_wait(%p,%p) EXIT, ret=%d\n",
                     (void *) ip_cv, (void *) ip_mutex, lv_status);
  #endif
#endif

    iv_flag = false;
    return lv_status;
}

SB_INLINE int SB_Thread::CV::wait(bool pv_lock) {
    int lv_ret;
    int lv_status;

#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("CV:::wait ENTRY cv=%p\n", (void *) &iv_cv);
  #else
        trace_printf("CV:::wait ENTRY cv=%p\n", (void *) ip_cv);
  #endif
#endif
    if (pv_lock) {
        lv_status = lock();
        SB_util_assert_ieq(lv_status, 0);
        lv_ret = wait();
        lv_status = unlock();
        SB_util_assert_ieq(lv_status, 0);
    } else
        lv_ret = wait();
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("CV:::wait EXIT cv=%p\n", (void *) &iv_cv);
  #else
        trace_printf("CV:::wait EXIT cv=%p\n", (void *) ip_cv);
  #endif
#endif
    return lv_ret;
}

SB_INLINE int SB_Thread::CV::wait(int pv_sec, int pv_us) {
    struct timespec lv_ts;

#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("CV:::wait ENTRY cv=%p, sec=%d, us=%d\n",
                     (void *) &iv_cv, pv_sec, pv_us);
  #else
        trace_printf("CV:::wait ENTRY cv=%p, sec=%d, us=%d\n",
                     (void *) ip_cv, pv_sec, pv_us);
  #endif
#endif
    if (iv_flag) {
        iv_flag = false;
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
        if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
            trace_printf("CV:::wait EXIT cv=%p\n", (void *) &iv_cv);
  #else
            trace_printf("CV:::wait EXIT cv=%p\n", (void *) ip_cv);
  #endif
#endif
        return 0;
    }

#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    struct timespec lv_ts_old;
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("pthread_cond_timedwait(%p,%p,%d.%d) ENTRY\n",
                     (void *) &iv_cv, (void *) &iv_mutex, pv_sec, pv_us);
  #else
        trace_printf("pthread_cond_timedwait(%p,%p,%d.%d) ENTRY\n",
                     (void *) ip_cv, (void *) ip_mutex, pv_sec, pv_us);
  #endif
    clock_gettime(CLOCK_REALTIME, &lv_ts_old);
#endif
    clock_gettime(CLOCK_REALTIME, &lv_ts);
    lv_ts.tv_sec += pv_sec;
    lv_ts.tv_nsec += (pv_us * SB_NS_PER_US);
    if (lv_ts.tv_nsec >= SB_NS_PER_SEC) {
        lv_ts.tv_nsec -= SB_NS_PER_SEC;
        lv_ts.tv_sec++;
    }
#ifdef SB_THREAD_LINUX
    int lv_status = pthread_cond_timedwait(&iv_cv, &iv_mutex, &lv_ts);
#else
    int lv_status = pthread_cond_timedwait(reinterpret_cast<pthread_cond_t *>(ip_cv),
                                           reinterpret_cast<pthread_mutex_t *>(ip_mutex),
                                           &lv_ts);
#endif
    SB_util_assert_ine(lv_status, EINVAL);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread) {
        long long lv_to_act;
        struct timespec lv_ts_new;
        clock_gettime(CLOCK_REALTIME, &lv_ts_new);
        lv_to_act = (long long) (lv_ts_new.tv_sec - lv_ts_old.tv_sec) *
                      SB_NS_PER_SEC +
                    (long long) (lv_ts_new.tv_nsec - lv_ts_old.tv_nsec);
        lv_ts_new.tv_sec = static_cast<__time_t>(lv_to_act / SB_NS_PER_SEC);
        lv_ts_new.tv_nsec =
          static_cast<long>(lv_to_act - (long long) lv_ts_new.tv_sec * SB_NS_PER_SEC);
        lv_ts_new.tv_nsec /= SB_NS_PER_US;
  #ifdef SB_THREAD_LINUX
        trace_printf("pthread_cond_timedwait(%p,%p,%d.%d) to=%ld.%06ld EXIT, ret=%d\n",
                     (void *) &iv_cv, (void *) &iv_mutex, pv_sec, pv_us,
                     lv_ts_new.tv_sec, lv_ts_new.tv_nsec, lv_status);
  #else
        trace_printf("pthread_cond_timedwait(%p,%p,%d.%d) to=%ld.%06ld EXIT, ret=%d\n",
                     (void *) ip_cv, (void *) ip_mutex, pv_sec, pv_us,
                     lv_ts_new.tv_sec, lv_ts_new.tv_nsec, lv_status);
  #endif
    }
#endif

    iv_flag = false;
    return lv_status;
}

SB_INLINE int SB_Thread::CV::wait(bool pv_lock, int pv_sec, int pv_us) {
    int lv_ret;
    int lv_status;

#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("CV:::wait ENTRY cv=%p, lock=%d, sec=%d, us=%d\n",
                     (void *) &iv_cv, pv_lock, pv_sec, pv_us);
  #else
        trace_printf("CV:::wait ENTRY cv=%p, lock=%d, sec=%d, us=%d\n",
                     (void *) ip_cv, pv_lock, pv_sec, pv_us);
  #endif
#endif
    if (pv_lock) {
        lv_status = lock();
        SB_util_assert_ieq(lv_status, 0);
        lv_ret = wait(pv_sec, pv_us);
        lv_status = unlock();
        SB_util_assert_ieq(lv_status, 0);
    } else
        lv_ret = wait(pv_sec, pv_us);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
  #ifdef SB_THREAD_LINUX
        trace_printf("CV:::wait EXIT cv=%p\n", (void *) &iv_cv);
  #else
        trace_printf("CV:::wait EXIT cv=%p\n", (void *) ip_cv);
  #endif
#endif
    return lv_ret;
}

#ifdef SB_THREAD_LINUX
SB_INLINE int SB_Thread::RWL::readlock() {
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_rwlock_rdlock(%p-%s) ENTRY\n",
                     (void *) &iv_rwl, ip_rwl_name);
#endif
    int lv_status = pthread_rwlock_rdlock(&iv_rwl);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_rwlock_rdlock(%p-%s) EXIT, ret=%d\n",
                     (void *) &iv_rwl, ip_rwl_name, lv_status);
#endif
    return lv_status;
}
#else
#error "SB_Thread::RWL::readlock() not implemented for platform"
#endif

SB_INLINE void SB_Thread::RWL::setname(const char *pp_name) {
    ip_rwl_name = pp_name;
}

#ifdef SB_THREAD_LINUX
SB_INLINE int SB_Thread::RWL::tryreadlock() {
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_rwlock_tryrdlock(%p-%s) ENTRY\n",
                     (void *) &iv_rwl, ip_rwl_name);
#endif
    int lv_status = pthread_rwlock_tryrdlock(&iv_rwl);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_rwlock_tryrdlock(%p-%s) EXIT, ret=%d\n",
                 (void *) &iv_rwl, ip_rwl_name, lv_status);
#endif
    return lv_status;
}
#else
#error "SB_Thread::RWL::tryreadlock() not implemented for platform"
#endif

#ifdef SB_THREAD_LINUX
SB_INLINE int SB_Thread::RWL::trywritelock() {
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_rwlock_trywrlock(%p-%s) ENTRY\n",
                     (void *) &iv_rwl, ip_rwl_name);
#endif
    int lv_status = pthread_rwlock_trywrlock(&iv_rwl);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_rwlock_trywrlock(%p-%s) EXIT, ret=%d\n",
                 (void *) &iv_rwl, ip_rwl_name, lv_status);
#endif
    return lv_status;
}
#else
#error "SB_Thread::RWL::trywritelock() not implemented for platform"
#endif

#ifdef SB_THREAD_LINUX
SB_INLINE int SB_Thread::RWL::writelock() {
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_rwlock_wrlock(%p-%s) ENTRY\n",
                     (void *) &iv_rwl, ip_rwl_name);
#endif
    int lv_status = pthread_rwlock_wrlock(&iv_rwl);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_rwlock_wrlock(%p-%s) EXIT, ret=%d\n",
                     (void *) &iv_rwl, ip_rwl_name, lv_status);
#endif
    return lv_status;
}
#else
#error "SB_Thread::RWL::writelock() not implemented for platform"
#endif

#ifdef SB_THREAD_LINUX
SB_INLINE int SB_Thread::RWL::unlock() {
    int lv_status;

    lv_status = pthread_rwlock_unlock(&iv_rwl);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_rwlock_unlock(%p-%s) EXIT, ret=%d\n",
                     (void *) &iv_rwl, ip_rwl_name, lv_status);
#endif
    return lv_status;
}
#else
#error "SB_Thread::RWL::unlock() not implemented for platform"
#endif

#ifdef SB_THREAD_LINUX
SB_INLINE int SB_Thread::SL::lock() {
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_spin_lock(%p-%s) ENTRY\n", 
                     (void *) &iv_sl, ip_sl_name);
#endif
#ifdef SB_THREAD_LOCK_STATS
    gv_sb_lock_stats.inc(&gv_sb_lock_stats.iv_spin_count);
    SB_Thread_Lock_Time lv_lock_start = gv_sb_lock_stats.time_start();
#endif
    int lv_status = pthread_spin_lock(&iv_sl);
#ifdef SB_THREAD_LOCK_STATS
    iv_lock_start = gv_sb_lock_stats.get_lock_time_set_spin_get_lock(lv_lock_start);
    iv_last_locker = gettid();
#endif
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_spin_lock(%p-%s) EXIT, ret=%d\n",
                     (void *) &iv_sl, ip_sl_name, lv_status);
#endif
    return lv_status;
}
#else
SB_INLINE int SB_Thread::SL::lock() {
    pthread_spinlock_t *lp_sl = static_cast<pthread_spinlock_t *>(ip_sl);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_spin_lock(%p-%s) ENTRY\n",
                     (void *) lp_sl, ip_sl_name);
#endif
#ifdef SB_THREAD_LOCK_STATS
    gv_sb_lock_stats.inc(&gv_sb_lock_stats.iv_spin_count);
    SB_Thread_Lock_Time lv_lock_start = gv_sb_lock_stats.time_start();
#endif
    int lv_status = pthread_spin_lock(lp_sl);
#ifdef SB_THREAD_LOCK_STATS
    iv_lock_start = gv_sb_lock_stats.get_lock_time_set_spin_get_lock(lv_lock_start);
    iv_last_locker = gettid();
#endif
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_spin_lock(%p-%s) EXIT, ret=%d\n",
                     (void *) lp_sl, lv_status);
#endif
    return lv_status;
}
#endif

SB_INLINE void SB_Thread::SL::setname(const char *pp_name) {
    ip_sl_name = pp_name;
}

#ifdef SB_THREAD_LINUX
SB_INLINE int SB_Thread::SL::unlock() {
#ifdef SB_THREAD_LOCK_STATS
    gv_sb_lock_stats.get_lock_time_set_spin_hold_lock(iv_lock_start);
#endif
    int lv_status = pthread_spin_unlock(&iv_sl);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_spin_unlock(%p-%s) EXIT, ret=%d\n",
                     (void *) &iv_sl, ip_sl_name, lv_status);
#endif
    return lv_status;
}
#else
SB_INLINE int SB_Thread::SL::unlock() {
    pthread_spinlock_t *lp_sl = static_cast<pthread_spinlock_t *>(ip_sl);
#ifdef SB_THREAD_LOCK_STATS
    gv_sb_lock_stats.get_lock_time_set_spin_hold_lock(iv_lock_start);
#endif
    int lv_status = pthread_spin_unlock(lp_sl);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_spin_unlock(%p-%s) EXIT, ret=%d\n",
                     (void *) lp_sl, lv_status);
#endif
    return lv_status;
}
#endif

#ifdef SB_THREAD_LINUX
SB_INLINE int SB_Thread::SL::trylock() {
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_spin_trylock(%p-%s) ENTRY\n",
                     (void *) &iv_sl, ip_sl_name);
#endif
    int lv_status = pthread_spin_trylock(&iv_sl);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_spin_trylock(%p-%s) EXIT, ret=%d\n",
                 (void *) &iv_sl, ip_sl_name, lv_status);
#endif
    return lv_status;
}
#else
SB_INLINE int SB_Thread::SL::trylock() {
    pthread_spinlock_t *lp_sl = static_cast<pthread_spinlock_t *>(ip_sl);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_spin_trylock(%p-%s) ENTRY\n", (void *) lp_sl);
#endif
    int lv_status = pthread_spin_trylock(lp_sl);
#ifdef SB_THREAD_PRINT_PTHREAD_CALLS
    if (gv_sb_trace_pthread)
        trace_printf("pthread_spin_trylock(%p-%s) EXIT, ret=%d\n",
                 (void *) lp_sl, lv_status);
#endif
    return lv_status;
}
#endif

SB_INLINE int SB_Thread::Usem::init(bool pv_pshared, unsigned int pv_value) {
#ifndef SB_THREAD_LINUX
    sem_t *lp_sem;
#endif
    int    lv_pshared;
    int    lv_status;

#ifdef SB_THREAD_LINUX
    lv_pshared = pv_pshared ? 1 : 0;
    lv_status = sem_init(&iv_sem, lv_pshared, pv_value);
    SB_util_assert_ieq(lv_status, 0);
    iv_inited = true;
#else
    if (ip_sem == NULL) {
        lv_pshared = pv_pshared ? 1 : 0;
        lp_sem = new sem_t;
        lv_status = sem_init(lp_sem, lv_pshared, pv_value);
        SB_util_assert_ieq(lv_status, 0);
        ip_sem = reinterpret_cast<Thr_Gen_Ptr>(lp_sem);
    } else
        lv_status = 0;
#endif
    return lv_status;
}

SB_INLINE int SB_Thread::Usem::post() {
#ifdef SB_THREAD_LINUX
    int lv_status = sem_post(&iv_sem);
#else
    sem_t *lp_sem = reinterpret_cast<sem_t *>(ip_sem);
    int lv_status = sem_post(lp_sem);
#endif
    SB_util_assert_ieq(lv_status, 0);
    return lv_status;
}

SB_INLINE int SB_Thread::Usem::trywait() {
#ifdef SB_THREAD_LINUX
    int lv_status = sem_trywait(&iv_sem);
#else
    sem_t *lp_sem = reinterpret_cast<sem_t *>(ip_sem);
    int lv_status = sem_trywait(lp_sem);
#endif
    return lv_status;
}

SB_INLINE int SB_Thread::Usem::value(int *pp_val) {
#ifdef SB_THREAD_LINUX
    int lv_status = sem_getvalue(&iv_sem, pp_val);
#else
    sem_t *lp_sem = reinterpret_cast<sem_t *>(ip_sem);
    int lv_status = sem_getvalue(lp_sem, pp_val);
#endif
    SB_util_assert_ieq(lv_status, 0);
    return lv_status;
}

SB_INLINE int SB_Thread::Usem::wait() {
    int lv_status;

    do {
#ifdef SB_THREAD_LINUX
        lv_status = sem_wait(&iv_sem);
#else
        sem_t *lp_sem = reinterpret_cast<sem_t *>(ip_sem);
        lv_status = sem_wait(lp_sem);
#endif
    } while (lv_status == EINTR);
    return lv_status;
}

SB_INLINE int SB_Thread::Usem::wait_timed(int pv_sec, int pv_us) {
#ifndef SB_THREAD_LINUX
    sem_t           *lp_sem;
#endif
    int              lv_status;
    struct timespec  lv_ts;

#ifndef SB_THREAD_LINUX
    lp_sem = reinterpret_cast<sem_t *>(ip_sem);
#endif
    clock_gettime(CLOCK_REALTIME, &lv_ts);
    lv_ts.tv_sec += pv_sec;
    lv_ts.tv_nsec += (pv_us * SB_NS_PER_US);
    if (lv_ts.tv_nsec >= SB_NS_PER_SEC) {
        lv_ts.tv_nsec -= SB_NS_PER_SEC;
        lv_ts.tv_sec++;
    }
#ifdef SB_THREAD_LINUX
    lv_status = sem_timedwait(&iv_sem, &lv_ts);
#else
    lv_status = sem_timedwait(lp_sem, &lv_ts);
#endif
    return lv_status;
}

SB_INLINE int SB_Thread::Nsem::post() {
#ifdef SB_THREAD_LINUX
    int lv_status = sem_post(ip_sem);
#else
    sem_t *lp_sem = reinterpret_cast<sem_t *>(ip_sem);
    int lv_status = sem_post(lp_sem);
#endif
    return lv_status;
}

SB_INLINE int SB_Thread::Nsem::remove(const char *pp_name) {
    int lv_status = sem_unlink(pp_name);
    return lv_status;
}

SB_INLINE int SB_Thread::Nsem::trywait() {
#ifdef SB_THREAD_LINUX
    int lv_status = sem_trywait(ip_sem);
#else
    sem_t *lp_sem = reinterpret_cast<sem_t *>(ip_sem);
    int lv_status = sem_trywait(lp_sem);
#endif
    return lv_status;
}

SB_INLINE int SB_Thread::Nsem::value(int *pp_val) {
#ifdef SB_THREAD_LINUX
    int lv_status = sem_getvalue(ip_sem, pp_val);
#else
    sem_t *lp_sem = reinterpret_cast<sem_t *>(ip_sem);
    int lv_status = sem_getvalue(lp_sem, pp_val);
#endif
    SB_util_assert_ieq(lv_status, 0);
    return lv_status;
}

SB_INLINE int SB_Thread::Nsem::wait() {
    int lv_status;

    do {
#ifdef SB_THREAD_LINUX
        lv_status = sem_wait(ip_sem);
#else
        sem_t *lp_sem = reinterpret_cast<sem_t *>(ip_sem);
        lv_status = sem_wait(lp_sem);
#endif
    } while (lv_status == EINTR);
    return lv_status;
}

SB_INLINE int SB_Thread::Nsem::wait_timed(int pv_sec, int pv_us) {
#ifndef SB_THREAD_LINUX
    sem_t           *lp_sem;
#endif
    int              lv_status;
    struct timespec  lv_ts;

#ifndef SB_THREAD_LINUX
    lp_sem = reinterpret_cast<sem_t *>(ip_sem);
#endif
    clock_gettime(CLOCK_REALTIME, &lv_ts);
    lv_ts.tv_sec += pv_sec;
    lv_ts.tv_nsec += (pv_us * SB_NS_PER_US);
    if (lv_ts.tv_nsec >= SB_NS_PER_SEC) {
        lv_ts.tv_nsec -= SB_NS_PER_SEC;
        lv_ts.tv_sec++;
    }
#ifdef SB_THREAD_LINUX
    lv_status = sem_timedwait(ip_sem, &lv_ts);
#else
    lv_status = sem_timedwait(lp_sem, &lv_ts);
#endif
    return lv_status;
}

#endif //!__SB_INT_THREAD_INL_
