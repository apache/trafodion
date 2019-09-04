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
// IMPLEMENTATION-NOTES:
//   Use pthread primitives directly
//

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/unistd.h> // gettid

#include "threadtlsx.h"
#include "util.h"

#define gettid() static_cast<pid_t>(syscall(__NR_gettid))

//
// SB_trace_assert_fun will be called instead of SB_util_assert_fun
//
#define SB_util_assert_fun SB_thread_assert_fun

enum { MAX_TLS_KEYS = 100 };

typedef struct SB_TLS_Key_Info_Type {
    const char *ip_desc;
} SB_TLS_Key_Info_Type;
SB_TLS_Key_Info_Type  ga_sb_tls_keys[MAX_TLS_KEYS];
bool                  gv_sb_tls_inited = false;
int                   gv_sb_tls_key_max = MAX_TLS_KEYS;
const char           *gp_sb_tls_key_null = "";
pthread_mutex_t       gv_sb_tls_mutex = PTHREAD_MUTEX_INITIALIZER;

void SB_thread_assert_fun(const char *pp_exp,
                          const char *pp_file,
                          unsigned    pv_line,
                          const char *pp_fun) {
    char  la_buf[512];
    char  la_cmdline[512];
    FILE *lp_file;
    char *lp_s;

    lp_file = fopen("/proc/self/cmdline", "r");
    if (lp_file != NULL) {
        lp_s = fgets(la_cmdline, sizeof(la_cmdline), lp_file);
        fclose(lp_file);
    } else
        lp_s = NULL;
    if (lp_s == NULL)
        lp_s = const_cast<char *>("<unknown>");
    sprintf(la_buf, "%s (%d-%d): %s:%u %s: Assertion '%s' failed.\n",
            lp_s,
            getpid(), gettid(),
            pp_file, pv_line, pp_fun,
            pp_exp);
    fprintf(stderr, "%s", la_buf);
    fflush(stderr);
    abort(); // can't use SB_util_abort
}

void SB_add_tls_key(int pv_key, const char *pp_desc) {
    int lv_inx;
    int lv_status;

    lv_status = pthread_mutex_lock(&gv_sb_tls_mutex);
    SB_util_assert(lv_status == 0);
    if (!gv_sb_tls_inited) {
        gv_sb_tls_inited = true;
        for (lv_inx = 0; lv_inx < MAX_TLS_KEYS; lv_inx++)
            ga_sb_tls_keys[lv_inx].ip_desc = gp_sb_tls_key_null;
    }

    SB_util_assert(pv_key >= 0);
    SB_util_assert(pv_key < MAX_TLS_KEYS);
    ga_sb_tls_keys[pv_key].ip_desc = pp_desc;

    lv_status = pthread_mutex_unlock(&gv_sb_tls_mutex);
    SB_util_assert(lv_status == 0);
}

int SB_create_tls_key(SB_Thread::Sthr::Dtor_Function  pp_dtor,
                      const char                     *pp_desc) {
    pthread_key_t lv_key;
    int           lv_status;

    lv_status = pthread_key_create(&lv_key, pp_dtor);
    SB_util_assert(lv_status == 0);
    SB_add_tls_key(lv_key, pp_desc);

    return lv_key;
}
