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

#include <limits.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "util.h"

// the following is based on glibc-2.6

#define SB_TLS_KEY2_SIZE 32

#define SB_TLS_KEY1_SIZE \
  ((PTHREAD_KEYS_MAX + SB_TLS_KEY2_SIZE - 1) \
   / SB_TLS_KEY2_SIZE)

struct SB_TLS_key_data {
    uintptr_t  seq;
    void      *data;
};

#if USE_RH == 6
struct SB_TLS_pthread {
    struct header {
        void  *p1[2];
        void  *self;
        int    p2;
        void  *p3[3];
        long   p4[2];
    } header;

    void         *p5[16];
    void         *p6[2];
    pid_t         p7[2];
#if __WORDSIZE == 32
    void         *p8[3];
#else
    void         *p8[4];
#endif
    void         *p9[2];
    int           p10[2];

    struct SB_TLS_key_data  specific1[SB_TLS_KEY2_SIZE];
    struct SB_TLS_key_data *specific2[SB_TLS_KEY1_SIZE];


  // and on-and-on
};
#else
struct SB_TLS_pthread {
    struct header {
        void  *p1[2];
        void  *self;
        int    p2;
        long   p3[5];
        void  *p4[7];
    } header;

    void         *p5[2];
    pid_t         p6[2];
#if __WORDSIZE == 32
    void         *p7[3];
#else
    void         *p7[4];
#endif
    void         *p8[2];
    int           p9[2];

    struct SB_TLS_key_data  specific1[SB_TLS_KEY2_SIZE];
    struct SB_TLS_key_data *specific2[SB_TLS_KEY1_SIZE];


  // and on-and-on
};
#endif

void SB_thread_print_specific(pthread_key_t pv_key) {
    SB_TLS_key_data       *lp_data;
    void                  *lp_res;
    struct SB_TLS_pthread *lp_self;
    unsigned int           lv_inx1;
    unsigned int           lv_inx2;
    pthread_t              lv_self;

    lv_self = pthread_self();
    lp_self = (struct SB_TLS_pthread *) lv_self;
    SB_util_assert(lp_self->header.self == reinterpret_cast<void *>(lv_self));
    if (pv_key < SB_TLS_KEY2_SIZE) {
        lp_data = &lp_self->specific1[pv_key];
#if __WORDSIZE == 32
        lp_res = reinterpret_cast<void *>(lp_data->seq);
#else
        lp_res = lp_data->data;
#endif
        printf("1st-level, key=%d, data=%p\n", pv_key, lp_res);
    } else if (pv_key >= PTHREAD_KEYS_MAX)
        printf("out-of-bounds key=%d\n", pv_key);
    else {
        lv_inx1 = pv_key / SB_TLS_KEY2_SIZE;
        lv_inx2 = pv_key % SB_TLS_KEY2_SIZE;
#if __WORDSIZE == 32
        lp_data = lp_self->specific2[lv_inx1-1];
#else
        lp_data = lp_self->specific2[lv_inx1];
#endif
        if (lp_data == NULL)
            lp_res = NULL;
        else {
            lp_data = &lp_data[lv_inx2];
            lp_res = lp_data->data;
        }
        printf("2nd-level, key=%d, data=%p\n", pv_key, lp_res);
    }
}

