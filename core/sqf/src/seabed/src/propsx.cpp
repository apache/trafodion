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

#include <linux/unistd.h>

// include all support files/functions for SB_Props
#include "props.h"
#include "smap.h"
#ifdef USE_SB_MAP_STATS
#include "mapstats.h"
#endif

#define gettid() static_cast<pid_t>(syscall(__NR_gettid))

void SB_util_assert_fun(const char *pp_exp,
                        const char *pp_file,
                        unsigned    pv_line,
                        const char *pp_fun) {
    printf("(%d-%d): %s:%u %s: Assertion '%s' failed.\n",
           getpid(), gettid(),
           pp_file, pv_line, pp_fun,
           pp_exp);
}

void SB_util_assert_fun_cpne(const char *pp_exp,
                             const void *pp_lhs,
                             const void *pp_rhs,
                             const char *pp_file,
                             unsigned    pv_line,
                             const char *pp_fun) {
    printf("(%d-%d): %s:%u %s: Assertion '%s' '(%p != %p)' failed.\n",
           getpid(), gettid(),
           pp_file, pv_line, pp_fun,
           pp_exp, pp_lhs, pp_rhs);
}

void SB_util_assert_fun_ieq(const char *pp_exp,
                            int         pv_lhs,
                            int         pv_rhs,
                            const char *pp_file,
                            unsigned    pv_line,
                            const char *pp_fun) {
    printf("(%d-%d): %s:%u %s: Assertion '%s' '(%d == %d)' failed.\n",
           getpid(), gettid(),
           pp_file, pv_line, pp_fun,
           pp_exp, pv_lhs, pv_rhs);
}

void SB_util_assert_fun_pne(const char *pp_exp,
                            void       *pp_lhs,
                            void       *pp_rhs,
                            const char *pp_file,
                            unsigned    pv_line,
                            const char *pp_fun) {
    printf("(%d-%d): %s:%u %s: Assertion '%s' '(%p != %p)' failed.\n",
           getpid(), gettid(),
           pp_file, pv_line, pp_fun,
           pp_exp, pp_lhs, pp_rhs);
}

#include "mapcom.cpp"
#include "props.cpp"
#include "smap.cpp"

#ifdef USE_SB_MAP_STATS
#include "mapstats.cpp"
SB_Thread::Mutex::Mutex() {
    pthread_mutex_t *lp_mutex = &iv_mutex;
    int lv_status = pthread_mutex_init(lp_mutex, NULL);
    SB_util_assert_ieq(lv_status, 0); // sw fault
}
SB_Thread::Mutex::~Mutex() {
    int lv_status = destroy();
    SB_util_assert_ieq(lv_status, 0); // sw fault
}
int SB_Thread::Mutex::destroy() {
    int lv_status = pthread_mutex_destroy(&iv_mutex);
    SB_util_assert_ieq(lv_status, 0); // sw fault
    return 0;
}
int SB_Thread::Mutex::lock() {
    int lv_status = pthread_mutex_lock(&iv_mutex);
    SB_util_assert_ieq(lv_status, 0); // sw fault
    return 0;
}
int SB_Thread::Mutex::trylock() {
    int lv_status = pthread_mutex_trylock(&iv_mutex);
    return lv_status;
}
int SB_Thread::Mutex::unlock() {
    int lv_status = pthread_mutex_unlock(&iv_mutex);
    SB_util_assert_ieq(lv_status, 0); // sw fault
    return 0;
}
#endif
