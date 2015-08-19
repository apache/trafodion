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

#ifndef __SB_UTRACE_INL_
#define __SB_UTRACE_INL_

#include <unistd.h>

#include <linux/unistd.h>

#define sb_utrace_gettid() static_cast<pid_t>(syscall(__NR_gettid))

// constructor
template <class T>
SB_Utrace<T>::SB_Utrace(int pv_max) : iv_inx(0), iv_wrapped(false) {
    unsigned int lv_mask;
    int          lv_max;

    // make sure that max is a multiple of 2
    if (pv_max < 1)
        pv_max = 256;
    // quickly scale mask
    if (pv_max & 0xffff0000)
        lv_mask = 0x80000000;
    else
        lv_mask = 0x8000;
    // find high bit and set max
    while (lv_mask) {
        lv_max = pv_max & lv_mask;
        if (lv_max)
            break;
        lv_mask >>= 1;
    }
    iv_max = lv_max;
    iv_mask = lv_max - 1;
    ip_buf = new T[lv_max];
}

// destructor
template <class T>
SB_Utrace<T>::~SB_Utrace() {
    delete [] ip_buf;
}

template <class T>
T *SB_Utrace<T>::get_entry() {
    int lv_linx;

    lv_linx = __sync_fetch_and_add_4(&iv_inx, 1);
    if (lv_linx >= iv_max) {
        // oops - overflow
        __sync_fetch_and_and_4(&iv_inx, iv_mask);
        lv_linx &= iv_mask;
        iv_wrapped = true;
    }
    return &ip_buf[lv_linx];
}

template <class T>
T *SB_Utrace<T>::get_entry_at(int pv_inx) {
    if ((pv_inx >= 0) && (pv_inx < iv_max))
        return &ip_buf[pv_inx];
    else
        return NULL;
}

template <class T>
int SB_Utrace<T>::get_inx() {
    return iv_inx;
}

template <class T>
int SB_Utrace<T>::get_max() {
    return iv_max;
}

template <class T>
bool SB_Utrace<T>::get_wrapped() {
    return iv_wrapped;
}

template <class T>
void SB_Utrace<T>::print_entries_last(const char *pp_title,
                                      FILE       *pp_f,
                                      PE_Type     pv_pf,
                                      int         pv_cnt) {
    T    *lp_rec;
    int   lv_cntr;
    int   lv_inx;
    int   lv_max;
    int   lv_wcnt;
    int   lv_winx;
    bool  lv_wrap;

    lv_inx = iv_inx;
    lv_max = iv_max;
    lv_wrap = iv_wrapped;
    if (pv_cnt > lv_max)
        pv_cnt = lv_max;
    fprintf(pp_f, "%s - inx=%d, max=%d, wrapped=%d\n",
            pp_title, lv_inx, lv_max, lv_wrap);
    if (lv_wrap) {
        lv_wcnt = pv_cnt - lv_inx;
        lv_winx = lv_max - lv_wcnt;
        if (lv_wcnt < 0)
            lv_cntr = lv_wcnt;
        else {
            lv_cntr = 0;
            pv_cnt -= lv_wcnt;
        }
        while (lv_cntr < lv_wcnt) {
            lp_rec = &ip_buf[lv_winx];
            pv_pf(pp_f, lp_rec, lv_winx);
            lv_winx++;
            lv_cntr++;
        }
    }
    if (lv_inx < pv_cnt)
        pv_cnt = lv_inx;
    lv_inx -= pv_cnt;
    lv_cntr = 0;
    while (lv_cntr < pv_cnt) {
        lp_rec = &ip_buf[lv_inx];
        pv_pf(pp_f, lp_rec, lv_inx);
        lv_inx++;
        lv_cntr++;
    }
    fflush(pp_f);
}
#endif // !__SB_UTRACE_INL_
