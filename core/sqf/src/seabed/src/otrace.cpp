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

//
// special defines:
//   DISPLAY_RA (return address)
//   DISPLAY_THREAD_IDS
//   DISPLAY_THREAD_NAME
//

#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <linux/unistd.h> // gettid

#include <sys/time.h>

#define gettid() static_cast<pid_t>(syscall(__NR_gettid))

//#define DISPLAY_RA
#define DISPLAY_THREAD_IDS
//#define DISPLAY_THREAD_NAME

#include "seabed/otrace.h"
#ifdef DISPLAY_THREAD_NAME
#include "seabed/thread.h"
#endif

#include "threadtlsx.h"
#include "tracex.h"
#include "util.h"

//
// SB_trace_assert_fun will be called instead of SB_util_assert_fun
//
#define SB_util_assert_fun SB_trace_assert_fun
#define SB_util_assert_fun_ieq SB_trace_assert_fun_ieq
#define SB_util_assert_fun_pne SB_trace_assert_fun_pne


enum { TRACE_MIN_INMEM_SIZE = 4096 };
enum { TRACE_MAX_INMEM_INX = 256 };

// forwards
static pthread_key_t sb_trace_key_init();

static pthread_key_t gv_otrace_tls_inx = sb_trace_key_init();

void SB_trace_assert_fun(const char *pp_exp,
                         const char *pp_file,
                         unsigned    pv_line,
                         const char *pp_fun);
void SB_trace_assert_fun_ieq(const char *pp_exp,
                             int         pv_lhs,
                             int         pv_rhs,
                             const char *pp_file,
                             unsigned    pv_line,
                             const char *pp_fun);
void SB_trace_assert_fun_pne(const char *pp_exp,
                             void       *pp_lhs,
                             void       *pp_rhs,
                             const char *pp_file,
                             unsigned    pv_line,
                             const char *pp_fun);

SB_Trace::SB_Trace() {
    ia_trace_pname[0] = '\0';
    ip_trace_file = NULL;
    ip_trace_file_buf = NULL;
    ip_trace_mem_buf = NULL;
    ip_trace_prefix = const_cast<char *>("trace");
    ip_trace_ts = NULL;
    iv_trace_delta = false;
    iv_trace_lock = true;
    iv_trace_max_size = INT_MAX;
    iv_trace_mem_inx = 0;
    iv_trace_mem_size = 0;
    iv_trace_pid = -1;
    iv_trace_set_mem_size = -1;
}

SB_Trace::~SB_Trace() {
    if (ip_trace_mem_buf != NULL) {
        fprintf(ip_trace_file,"%s", ip_trace_mem_buf);
        delete [] ip_trace_mem_buf;
        ip_trace_mem_buf = NULL;
    }
    if (ip_trace_file != NULL) {
        trace_flush();
        fclose(ip_trace_file);
        ip_trace_file = NULL;
    }
    if (ip_trace_file_buf != NULL)
        delete [] ip_trace_file_buf;
}

//
// Purpose: clear line
//
void SB_Trace::trace_clear_line(char *pp_line) {
    memset(pp_line, ' ', 78);
    pp_line[78] = '\0';
}

//
// Purpose: close
//
void SB_Trace::trace_close() {
    if (ip_trace_file != NULL) {
        trace_flush();
        if ((ip_trace_file != stdout) && (ip_trace_file != stderr))
            fclose(ip_trace_file);
        ip_trace_file = NULL;
    }
}

//
// Purpose: flush
//
void SB_Trace::trace_flush() {
    if (ip_trace_file != NULL) {
        if (ip_trace_mem_buf != NULL) {
            fprintf(ip_trace_file,"%s", ip_trace_mem_buf);
            iv_trace_mem_inx = 0;
        }
        fflush(ip_trace_file);
    }
}

//
// Purpose: return trace fd
//
FILE *SB_Trace::trace_get_fd() {
    return ip_trace_file;
}

//
// Purpose: initialize trace module
//
void SB_Trace::trace_init(char *pp_filename,
                          bool  pv_unique,
                          char *pp_prefix,
                          bool  pv_flush) { // flush ignored
    trace_init3(pp_filename, pv_unique, pp_prefix, pv_flush, 0, false);
}

//
// Purpose: initialize trace module
//
void SB_Trace::trace_init2(char      *pp_filename,
                           bool       pv_unique,
                           char      *pp_prefix,
                           bool       pv_flush, // flush ignored
                           long long  pv_max_size) {
    trace_init3(pp_filename, pv_unique, pp_prefix, pv_flush, pv_max_size, false);
}

//
// Purpose: initialize trace module
//
void SB_Trace::trace_init3(char      *pp_filename,
                           bool       pv_unique,
                           char      *pp_prefix,
                           bool       pv_flush, // flush ignored
                           long long  pv_max_size,
                           bool       pv_sig_hdlr) {
    char  la_uniquename[PATH_MAX];
    FILE *lp_file;

    if (ip_trace_file != NULL) {
        trace_flush();
        if ((ip_trace_file != stdout) && (ip_trace_file != stderr)) {
            fclose(ip_trace_file);
            ip_trace_file = NULL;
        }
    }
    pv_flush = pv_flush; // no-warn
    iv_trace_max_size = pv_max_size;
    iv_trace_pid = getpid();
    iv_trace_sig_hdlr = pv_sig_hdlr;
    if (ip_trace_file == NULL)
        ip_trace_file = stdout;
    if (pp_prefix != NULL)
        ip_trace_prefix = pp_prefix;
    if (pp_filename != NULL) {
        if (strcmp(pp_filename, "STDOUT") == 0)
            lp_file = stdout;
        else if (strcmp(pp_filename, "STDERR") == 0)
            lp_file = stderr;
        else if (pv_unique) {
            sprintf(la_uniquename, "%s%d", pp_filename, getpid());
            if ((pv_max_size == 0) || (pv_max_size > INT_MAX))
                lp_file = fopen64(la_uniquename, "a");
            else
                lp_file = fopen(la_uniquename, "a");
        } else {
            if ((pv_max_size == 0) || (pv_max_size > INT_MAX))
                lp_file = fopen64(pp_filename, "a");
            else
                lp_file = fopen(pp_filename, "a");
        }
        if (lp_file != NULL) {
            ip_trace_file = lp_file;

        }
    }
    if (ip_trace_file != stderr) {
        if (iv_trace_set_mem_size > 0) {
            trace_set_mem(iv_trace_set_mem_size);
            iv_trace_set_mem_size = -1;
        } else {
            trace_flush();
            setvbuf(ip_trace_file, NULL, _IOLBF, 0);
        }
    }
}

//
// Purpose: lock
//
void SB_Trace::trace_lock() {
    sigset_t *lp_set_old;
    int       lv_status;

    if (iv_trace_sig_hdlr) {
        lp_set_old = static_cast<sigset_t *>(pthread_getspecific(gv_otrace_tls_inx));
        if (lp_set_old == NULL) {
            lp_set_old = new sigset_t;
            lv_status = pthread_setspecific(gv_otrace_tls_inx, lp_set_old);
            SB_util_assert_ieq(lv_status, 0);
        }
        gv_trace_sig.lock(lp_set_old);
    }
    if (iv_trace_lock)
        gv_trace_mutex.lock();
}

//
// Purpose: nolock-printf
//
void SB_Trace::trace_nolock_printf(const char *pp_format, ...) {
    void    *lp_ra;
    va_list  lv_ap;

#ifdef DISPLAY_RA
    lp_ra = __builtin_return_address(0);
#else
    lp_ra = NULL;
#endif // DISPLAY_RA
    va_start(lv_ap, pp_format);
    trace_nolock_vprintf(lp_ra, pp_format, lv_ap);
    va_end(lv_ap);
}

//
// Purpose: nolock-vprintf
//
void SB_Trace::trace_nolock_vprintf(void       *pp_ra,
                                    const char *pp_format,
                                    va_list     pv_ap) {
    if (ip_trace_file == NULL) { // don't bother, if not initialized
        if (iv_trace_assert_no_trace)
            SB_util_assert_pne(ip_trace_file, NULL);
    } else {
        trace_print_time_id();
        trace_print_ra(pp_ra, NULL);
        trace_print_format(pp_format, pv_ap);
    }
}

//
// Purpose: nolock-printf with where
//
void SB_Trace::trace_nolock_where_printf(const char *pp_where,
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
    trace_nolock_where_vprintf(lp_ra, pp_where, pp_format, lv_ap);
    va_end(lv_ap);
}

//
// Purpose: nolock-vprintf with where
//
void SB_Trace::trace_nolock_where_vprintf(void       *pp_ra,
                                          const char *pp_where,
                                          const char *pp_format,
                                          va_list     pv_ap) {
    if (ip_trace_file == NULL) { // don't bother, if not initialized
        if (iv_trace_assert_no_trace)
            SB_util_assert_pne(ip_trace_file, NULL);
    } else {
        trace_print_time_id();
        trace_print_ra(pp_ra, pp_where);
        trace_print_format(pp_format, pv_ap);
    }
}

//
// Purpose: print data
//
void SB_Trace::trace_print_data(void *pp_buf, int pv_count, int pv_max_count) {
    enum { MAX_LINE_LEN     = 132 };
    enum { HEX_DIGIT_OFFSET = 10 };
    enum { ASCII_OFFSET     = HEX_DIGIT_OFFSET + 52 };

    char      la_line[MAX_LINE_LEN];
    char     *lp_ascii;
    char     *lp_buf;
    char     *lp_hex;
    long      lv_addr;
    int       lv_byte;
    int       lv_inx;
    int       lv_max;
    sigset_t  lv_set_old;

    if (ip_trace_file == NULL) { // don't bother, if not initialized
        if (iv_trace_assert_no_trace)
            SB_util_assert_pne(ip_trace_file, NULL);
    } else {
        if (iv_trace_sig_hdlr)
            gv_trace_sig.lock(&lv_set_old);
        if (iv_trace_lock)
            gv_trace_mutex.lock();
        if ((pv_max_count > 0) && (pv_count > pv_max_count)) {
            trace_print_fprintf("count=%d exceeds %d, TRUNCATED\n",
                                pv_count, pv_max_count);
            pv_count = pv_max_count; // limit
        }
        lv_addr = 0;
        lp_buf = static_cast<char *>(pp_buf);
        trace_clear_line(la_line);
        while (pv_count > 0) {
            if (pv_count < 16)
                lv_max = pv_count;
            else
                lv_max = 16;
            lp_hex = &la_line[HEX_DIGIT_OFFSET];
            lp_ascii = &la_line[ASCII_OFFSET];
            sprintf(la_line, "%08lx: ", lv_addr);
            for (lv_inx = 0;
                 lv_inx < lv_max;
                 lv_inx++, lp_buf++, lp_hex += 3, lp_ascii++) {
                lv_byte = (*lp_buf) & 0xff;
                sprintf(lp_hex, "%02x ", lv_byte);
                if (isprint(lv_byte))
                    *lp_ascii = static_cast<char>(lv_byte);
                else
                    *lp_ascii = '.';
            }

            *lp_hex = ' ';
            trace_print_fprintf("%s\n", la_line);
            trace_clear_line(la_line);
            lv_addr += lv_max;
            pv_count -= lv_max;
        }
        if (iv_trace_lock)
            gv_trace_mutex.unlock();
        if (iv_trace_sig_hdlr)
            gv_trace_sig.unlock(&lv_set_old);
    }
}

//
// Purpose: print format (either to trace-file or memory)
//
void SB_Trace::trace_print_format(const char *pp_format, va_list pv_ap) {
    int lv_len;

    if (ip_trace_mem_buf == NULL)
        vfprintf(ip_trace_file, pp_format, pv_ap);
    else {
        if ((iv_trace_mem_inx + TRACE_MAX_INMEM_INX) > iv_trace_mem_size)
            iv_trace_mem_inx = 0; // wrap
        lv_len = vsnprintf(&ip_trace_mem_buf[iv_trace_mem_inx],
                           TRACE_MAX_INMEM_INX,
                           pp_format,
                           pv_ap);
        if (lv_len >= TRACE_MAX_INMEM_INX) // too big
            lv_len = TRACE_MAX_INMEM_INX - 1;
        if (lv_len > 0) {
            iv_trace_mem_inx += lv_len;
            ip_trace_mem_buf[iv_trace_mem_inx] = '\0';
        }
    }
}

//
// Purpose: internal fprintf
//
void SB_Trace::trace_print_fprintf(const char *pp_format, ...) {
    va_list lv_ap;

    va_start(lv_ap, pp_format);
    trace_print_format(pp_format, lv_ap);
    va_end(lv_ap);
}

void SB_Trace::trace_print_ra(void *pp_ra, const char *pp_where) {
#ifdef DISPLAY_RA
    if (pp_where == NULL)
        trace_print_fprintf(" ra=%p: ", pp_ra);
    else
        trace_print_fprintf(" ra=%p: %s ", pp_ra, pp_where);
#else
    pp_ra = pp_ra; // touch
    if (pp_where == NULL)
        trace_print_fprintf(": ");
    else
        trace_print_fprintf(": %s ", pp_where);
#endif // DISPLAY_RA
}

void SB_Trace::trace_print_time_id() {
    char la_tl[40];

    trace_printf_time(la_tl);
#ifdef DISPLAY_THREAD_IDS
    #ifdef DISPLAY_THREAD_NAME
    char *lp_name = SB_Thread::Sthr::self_name();
    if (lp_name == NULL)
        lp_name = const_cast<char *>("main");
    trace_print_fprintf("%s %s(%s%ld-%8s)",
                        la_tl, ip_trace_prefix,
                        ia_trace_pname, gettid(), lp_name);
    #else
    trace_print_fprintf("%s %s(%s%d)",
                        la_tl, ip_trace_prefix,
                        ia_trace_pname, gettid());
    #endif // DISPLAY_THREAD_NAME
#else
    trace_print_fprintf("%s %s(%s%d)",
                        la_tl, ip_trace_prefix, ia_trace_pname, iv_trace_pid);
#endif // DISPLAY_THREAD_IDS
}

//
// Purpose: printf
//
void SB_Trace::trace_printf(const char *pp_format, ...) {
    void    *lp_ra;
    va_list  lv_ap;

#ifdef DISPLAY_RA
    lp_ra = __builtin_return_address(0);
#else
    lp_ra = NULL;
#endif // DISPLAY_RA
    va_start(lv_ap, pp_format);
    trace_vprintf(lp_ra, pp_format, lv_ap);
    va_end(lv_ap);
}

//
// Purpose: print time
//
void SB_Trace::trace_printf_time(char *pa_tl) {
    struct tm      *lp_tx;
    int             lv_ms;
    long            lv_run1;
    long            lv_run2;
    struct timeval  lv_t;
    struct tm       lv_tx;
    double          lv_time;
    int             lv_us;

    if (ip_trace_ts != NULL) {
        lv_time = ip_trace_ts();
        lp_tx = &lv_tx;
        lv_run1 = static_cast<long>(lv_time); // sec
        lv_t.tv_sec = lv_run1;
        lv_t.tv_usec = static_cast<int>((lv_time - static_cast<double>(lv_run1)) * SB_US_PER_SEC);
        lv_run2 = lv_run1 / 60; // min
        lv_tx.tm_sec = static_cast<int>(lv_run1 - (lv_run2 * 60));
        lv_run1 = lv_run2 / 60; // hrs
        lv_tx.tm_min = static_cast<int>(lv_run2 - (lv_run1 * 60));
        lv_run2 = lv_run1 / 24; // days
        lv_tx.tm_hour = static_cast<int>(lv_run1 - (lv_run2 * 24));
    } else {
        gettimeofday(&lv_t, NULL);
        lp_tx = localtime_r(&lv_t.tv_sec, &lv_tx);
    }
    lv_ms = static_cast<int>(lv_t.tv_usec) / 1000;
    lv_us = static_cast<int>(lv_t.tv_usec) - lv_ms * 1000;
    if (iv_trace_delta) {
        char la_delta[20];
        static long lv_last_sec = -1;
        static long lv_last_us = -1;
        if (lv_last_sec < 0)
            strcpy(la_delta, "           ");
        else if (lv_t.tv_sec < lv_last_sec)
            strcpy(la_delta, "(>--------)");
        else {
            long lv_delta_us = lv_t.tv_usec - lv_last_us;
            lv_delta_us += (lv_t.tv_sec - lv_last_sec) * SB_US_PER_SEC;
            sprintf(la_delta, "(>%8ld)", lv_delta_us);
        }
        lv_last_sec = lv_t.tv_sec;
        lv_last_us = lv_t.tv_usec;
        sprintf(pa_tl, "%02d:%02d:%02d.%03d.%03d %s",
                lp_tx->tm_hour, lp_tx->tm_min, lp_tx->tm_sec, lv_ms, lv_us,
                la_delta);
    } else
        sprintf(pa_tl, "%02d:%02d:%02d.%03d.%03d",
                lp_tx->tm_hour, lp_tx->tm_min, lp_tx->tm_sec, lv_ms, lv_us);
}

//
// Purpose: set assert-if-no-trace
//
void SB_Trace::trace_set_assert_no_trace(bool pv_assert_no_trace) {
    iv_trace_assert_no_trace = pv_assert_no_trace;
}

//
// Purpose: set delta trace
//
void SB_Trace::trace_set_delta(bool pv_delta) {
    iv_trace_delta = pv_delta;
}

//
// Purpose: set in-memory trace
//
void SB_Trace::trace_set_inmem(int pv_size) {
    if (pv_size < TRACE_MIN_INMEM_SIZE)
        pv_size = TRACE_MIN_INMEM_SIZE;
    trace_flush();
    if (ip_trace_mem_buf != NULL)
        delete [] ip_trace_mem_buf;
    iv_trace_mem_size = pv_size;
    ip_trace_mem_buf = new char[pv_size];
    ip_trace_mem_buf[0] = '\0';
    iv_trace_mem_inx = 0;
}

//
// Purpose: set memory trace
//
void SB_Trace::trace_set_mem(int pv_size) {
    trace_flush();
    if (ip_trace_file_buf != NULL)
        delete [] ip_trace_file_buf;
    ip_trace_file_buf = new char[pv_size];
    if (ip_trace_file != NULL)
        setvbuf(ip_trace_file, ip_trace_file_buf, _IOFBF, pv_size);
    else
        iv_trace_set_mem_size = pv_size;
}

//
// Purpose: set lock
//
void SB_Trace::trace_set_lock(bool pv_lock) {
    iv_trace_lock = pv_lock;
}

//
// Purpose: set name
//
void SB_Trace::trace_set_pname(const char *pp_pname) {
    int lv_lim = static_cast<int>(sizeof(ia_trace_pname) - 2);
    ia_trace_pname[lv_lim] = '\0';
    strncpy(ia_trace_pname, pp_pname, lv_lim);
    strcat(ia_trace_pname,  "-");
}


//
// Purpose: unlock
//
void SB_Trace::trace_unlock() {
    sigset_t *lp_set_old;

    if (iv_trace_lock)
        gv_trace_mutex.unlock();
    if (iv_trace_sig_hdlr) {
        lp_set_old =
          static_cast<sigset_t *>(pthread_getspecific(gv_otrace_tls_inx));
        SB_util_assert_pne(lp_set_old, NULL);
        gv_trace_sig.unlock(lp_set_old);
    }
}

//
// Purpose: vprintf
//
void SB_Trace::trace_vprintf(void       *pp_ra,
                             const char *pp_format,
                             va_list     pv_ap) {
    int       lv_err;
    fpos64_t  lv_pos;
    sigset_t  lv_set_old;

    if (ip_trace_file == NULL) { // don't bother, if not initialized
        if (iv_trace_assert_no_trace)
            SB_util_assert_pne(ip_trace_file, NULL);
    } else {
        if (iv_trace_sig_hdlr)
            gv_trace_sig.lock(&lv_set_old);
        if (iv_trace_lock)
            gv_trace_mutex.lock();
        if (iv_trace_max_size && (iv_trace_max_size != INT_MAX)) {
            lv_err = fgetpos64(ip_trace_file, &lv_pos);
            if (!lv_err && (lv_pos.__pos > iv_trace_max_size))
                abort(); // can't use SB_util_abort
        }
        trace_print_time_id();
        trace_print_ra(pp_ra, NULL);
        trace_print_format(pp_format, pv_ap);
        if (iv_trace_lock)
            gv_trace_mutex.unlock();
        if (iv_trace_sig_hdlr)
            gv_trace_sig.unlock(&lv_set_old);
    }
}

//
// Purpose: printf with where
//
void SB_Trace::trace_where_printf(const char *pp_where,
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
    trace_where_vprintf(lp_ra, pp_where, pp_format, lv_ap);
    va_end(lv_ap);
}

//
// Purpose: vprintf with where
//
void SB_Trace::trace_where_vprintf(void       *pp_ra,
                                   const char *pp_where,
                                   const char *pp_format,
                                   va_list     pv_ap) {
    int       lv_err;
    fpos64_t  lv_pos;
    sigset_t  lv_set_old;

    if (ip_trace_file == NULL) { // don't bother, if not initialized
        if (iv_trace_assert_no_trace)
            SB_util_assert_pne(ip_trace_file, NULL);
    } else {
        if (iv_trace_sig_hdlr)
            gv_trace_sig.lock(&lv_set_old);
        if (iv_trace_lock)
            gv_trace_mutex.lock();
        if (iv_trace_max_size && (iv_trace_max_size != INT_MAX)) {
            lv_err = fgetpos64(ip_trace_file, &lv_pos);
            if (!lv_err && (lv_pos.__pos > iv_trace_max_size))
                abort(); // can't use SB_util_abort
        }
        trace_print_time_id();
        trace_print_ra(pp_ra, pp_where);
        trace_print_format(pp_format, pv_ap);
        if (iv_trace_lock)
            gv_trace_mutex.unlock();
        if (iv_trace_sig_hdlr)
            gv_trace_sig.unlock(&lv_set_old);
    }
}

//
// SB_trace_assert_fun will be called instead of SB_util_assert_fun
//
void SB_trace_assert_fun(const char *pp_exp,
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
    fprintf(stderr,"%s", la_buf);
    fflush(stderr);
    abort(); // can't use SB_util_abort
}

//
// SB_trace_assert_fun_ieq will be called instead of SB_util_assert_fun_ieq
//
void SB_trace_assert_fun_ieq(const char *pp_exp,
                             int         pv_lhs,
                             int         pv_rhs,
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
    sprintf(la_buf, "%s (%d-%d): %s:%u %s: Assertion '%s' '(%d == %d)' failed.\n",
            lp_s,
            getpid(), gettid(),
            pp_file, pv_line, pp_fun,
            pp_exp, pv_lhs, pv_rhs);
    fprintf(stderr,"%s", la_buf);
    fflush(stderr);
    abort(); // can't use SB_util_abort
}

//
// SB_trace_assert_fun_pne will be called instead of SB_util_assert_fun_pne
//
void SB_trace_assert_fun_pne(const char *pp_exp,
                             void       *pp_lhs,
                             void       *pp_rhs,
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
    sprintf(la_buf, "%s (%d-%d): %s:%u %s: Assertion '%s' '(%p != %p)' failed.\n",
            lp_s,
            getpid(), gettid(),
            pp_file, pv_line, pp_fun,
            pp_exp, pp_lhs, pp_rhs);
    fprintf(stderr, "%s",la_buf);
    fflush(stderr);
    abort(); // can't use SB_util_abort
}

//
// Add private mutex impl here so that regular mutex's can be traced.
//
SB_Trace_Mutex::SB_Trace_Mutex() {
    int lv_status = pthread_mutex_init(&iv_mutex, NULL);
    SB_util_assert_ieq(lv_status, 0); // sw fault
    lv_status = lv_status; // touch (in case assert disabled)
}

SB_Trace_Mutex::~SB_Trace_Mutex() {
    int lv_status = pthread_mutex_destroy(&iv_mutex);
    SB_util_assert_ieq(lv_status, 0); // sw fault
    lv_status = lv_status; // touch (in case assert disabled)
}

void SB_Trace_Mutex::lock() {
    int lv_status = pthread_mutex_lock(&iv_mutex);
    SB_util_assert_ieq(lv_status, 0); // sw fault
    lv_status = lv_status; // touch (in case assert disabled)
}

void SB_Trace_Mutex::unlock() {
    int lv_status = pthread_mutex_unlock(&iv_mutex);
    SB_util_assert_ieq(lv_status, 0); // sw fault
    lv_status = lv_status; // touch (in case assert disabled)
}

SB_Trace_Sig_Block::SB_Trace_Sig_Block() {
    int lv_err = sigfillset(&iv_set_all);
    SB_util_assert_ieq(lv_err, 0); // sw fault
    lv_err = lv_err; // touch (in case assert disabled)
}

SB_Trace_Sig_Block::~SB_Trace_Sig_Block() {
}

void SB_Trace_Sig_Block::lock(sigset_t *pp_set_old) {
    int lv_err = pthread_sigmask(SIG_BLOCK, &iv_set_all, pp_set_old);
    SB_util_assert_ieq(lv_err, 0); // sw fault
    lv_err = lv_err; // touch (in case assert disabled)
}

void SB_Trace_Sig_Block::unlock(sigset_t *pp_set_old) {
    int lv_err = pthread_sigmask(SIG_SETMASK, pp_set_old, NULL);
    SB_util_assert_ieq(lv_err, 0); // sw fault
    lv_err = lv_err; // touch (in case assert disabled)
}

static void sb_trace_key_dtor(void *pp_set_old) {
    sigset_t *lp_set_old;

    if (pp_set_old != NULL) {
        lp_set_old = static_cast<sigset_t *>(pp_set_old);
        delete lp_set_old;
    }
}

static pthread_key_t sb_trace_key_init() {
    pthread_key_t lv_key;

    lv_key = SB_create_tls_key(sb_trace_key_dtor, "otrace_sigset");
    return lv_key;
}

#undef SB_util_assert_fun

