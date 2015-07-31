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
// special defines:
//   DISPLAY_RA
//   DISPLAY_THREAD_IDS
//
#include <stdio.h>
#include <stdlib.h>

#include "seabed/otrace.h"
//#define DISPLAY_RA
#define DISPLAY_THREAD_IDS
#ifdef DISPLAY_THREAD_IDS
#  include "seabed/thread.h"
#endif
#include "seabed/trace.h"

#include "tracex.h"


//
// Global wrapper for SB_Trace
//

static SB_Trace    gv_trace;
static bool        gv_trace_sig_hdlr = false;
SB_Trace_Mutex     gv_trace_mutex; // share with otrace
SB_Trace_Sig_Block gv_trace_sig;   // share with otrace


//
// Purpose: close
//
SB_Export void trace_close() {
    return gv_trace.trace_close();
}

//
// Purpose: flush
//
SB_Export void trace_flush() {
    return gv_trace.trace_flush();
}

//
// Purpose: return trace fd
//
SB_Export FILE *trace_get_fd() {
    return gv_trace.trace_get_fd();
}

//
// Purpose: initialize trace module
//
SB_Export void trace_init(char *pp_filename,
                          bool  pv_unique,
                          char *pp_prefix,
                          bool  pv_flush) {
    gv_trace.trace_init(pp_filename, pv_unique, pp_prefix, pv_flush);
}

//
// Purpose: initialize trace module
//
SB_Export void trace_init2(char      *pp_filename,
                           bool       pv_unique,
                           char      *pp_prefix,
                           bool       pv_flush,
                           long long  pv_max_size) {
    gv_trace.trace_init2(pp_filename,
                         pv_unique,
                         pp_prefix,
                         pv_flush,
                         pv_max_size);
}

//
// Purpose: initialize trace module
//
SB_Export void trace_init3(char      *pp_filename,
                           bool       pv_unique,
                           char      *pp_prefix,
                           bool       pv_flush,
                           long long  pv_max_size,
                           bool       pv_sig_hdlr) {
    gv_trace_sig_hdlr = pv_sig_hdlr;
    gv_trace.trace_init3(pp_filename,
                         pv_unique,
                         pp_prefix,
                         pv_flush,
                         pv_max_size,
                         pv_sig_hdlr);
}

//
// Purpose: lock
//
SB_Export void trace_lock() {
    gv_trace.trace_lock();
}

//
// Purpose: nolock-printf
//
SB_Export void trace_nolock_printf(const char *pp_format, ...) {
    void    *lp_ra;
    va_list  lv_ap;

#ifdef DISPLAY_RA
    lp_ra = __builtin_return_address(0);
#else
    lp_ra = NULL;
#endif // DISPLAY_RA
    va_start(lv_ap, pp_format);
    gv_trace.trace_nolock_vprintf(lp_ra, pp_format, lv_ap);
    va_end(lv_ap);
}

//
// Purpose: nolock-vprintf
//
SB_Export void trace_nolock_vprintf(const char *pp_format,
                                    va_list     pv_ap) {
    void *lp_ra;

#ifdef DISPLAY_RA
    lp_ra = __builtin_return_address(0);
#else
    lp_ra = NULL;
#endif // DISPLAY_RA
    gv_trace.trace_nolock_vprintf(lp_ra, pp_format, pv_ap);
}

//
// Purpose: nolock-printf with where
//
SB_Export void trace_nolock_where_printf(const char *pp_where,
                                         const char *pp_format,
                                         ...) {
    void    *lp_ra;
    va_list  lv_ap;

#ifdef DISPLAY_RA
    lp_ra = __builtin_return_address(0);
#else
    lp_ra = NULL;
#endif // DISPLAY_RA
    va_start(lv_ap, pp_format);
    gv_trace.trace_nolock_where_vprintf(lp_ra, pp_where, pp_format, lv_ap);
    va_end(lv_ap);
}

//
// Purpose: nolock-vprintf with where
//
SB_Export void trace_nolock_where_vprintf(const char *pp_where,
                                          const char *pp_format,
                                          va_list     pv_ap) {
    void *lp_ra;

#ifdef DISPLAY_RA
    lp_ra = __builtin_return_address(0);
#else
    lp_ra = NULL;
#endif // DISPLAY_RA
    gv_trace.trace_nolock_where_vprintf(lp_ra, pp_where, pp_format, pv_ap);
}

//
// Purpose: print data
//
SB_Export void trace_print_data(void *pp_buf, int pv_count, int pv_max_count) {
    gv_trace.trace_print_data(pp_buf, pv_count, pv_max_count);
}

//
// Purpose: printf
//
SB_Export void trace_printf(const char *pp_format, ...) {
    void    *lp_ra;
    va_list  lv_ap;

#ifdef DISPLAY_RA
    lp_ra = __builtin_return_address(0);
#else
    lp_ra = NULL;
#endif // DISPLAY_RA
    va_start(lv_ap, pp_format);
    gv_trace.trace_vprintf(lp_ra, pp_format, lv_ap);
    va_end(lv_ap);
}

//
// Purpose: set assert-if-no-trace
//
SB_Export void trace_set_assert_no_trace(bool pv_assert_no_trace) {
    gv_trace.trace_set_assert_no_trace(pv_assert_no_trace);
}

//
// Purpose: set delta trace
//
SB_Export void trace_set_delta(bool pv_delta) {
    gv_trace.trace_set_delta(pv_delta);
}

//
// Purpose: set in-memory trace
//
SB_Export void trace_set_inmem(int pv_size) {
    gv_trace.trace_set_inmem(pv_size);
}

//
// Purpose: set lock
//
SB_Export void trace_set_lock(bool pv_lock) {
    gv_trace.trace_set_lock(pv_lock);
}

//
// Purpose: set memory trace
//
SB_Export void trace_set_mem(int pv_size) {
    gv_trace.trace_set_mem(pv_size);
}

//
// Purpose: set name
//
SB_Export void trace_set_pname(const char *pp_pname) {
    gv_trace.trace_set_pname(pp_pname);
}


//
// Purpose: unlock
//
SB_Export void trace_unlock() {
    gv_trace.trace_unlock();
}

//
// Purpose: vprintf
//
SB_Export void trace_vprintf(const char *pp_format,
                             va_list     pv_ap) {
    void *lp_ra;

#ifdef DISPLAY_RA
    lp_ra = __builtin_return_address(0);
#else
    lp_ra = NULL;
#endif // DISPLAY_RA
    gv_trace.trace_vprintf(lp_ra, pp_format, pv_ap);
}

//
// Purpose: printf with where
//
SB_Export void trace_where_printf(const char *pp_where,
                                  const char *pp_format,
                                  ...) {
    void    *lp_ra;
    va_list  lv_ap;

#ifdef DISPLAY_RA
    lp_ra = __builtin_return_address(0);
#else
    lp_ra = NULL;
#endif // DISPLAY_RA
    va_start(lv_ap, pp_format);
    gv_trace.trace_where_vprintf(lp_ra, pp_where, pp_format, lv_ap);
    va_end(lv_ap);
}

//
// Purpose: vprintf with where
//
SB_Export void trace_where_vprintf(const char *pp_where,
                                   const char *pp_format,
                                   va_list     pv_ap) {
    void *lp_ra;

#ifdef DISPLAY_RA
    lp_ra = __builtin_return_address(0);
#else
    lp_ra = NULL;
#endif // DISPLAY_RA
    gv_trace.trace_where_vprintf(lp_ra, pp_where, pp_format, pv_ap);
}
