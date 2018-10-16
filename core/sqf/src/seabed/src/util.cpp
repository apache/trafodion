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

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/unistd.h>

#include "common/evl_sqlog_eventnum.h"

#include "seabed/log.h"
#include "seabed/logalt.h"
#include "seabed/ms.h"
#include "seabed/trace.h"

#include "buf.h"
#include "chk.h"
#include "logaggr.h"
#include "mslog.h"
#include "util.h"


#define gettid() static_cast<pid_t>(syscall(__NR_gettid))

static const char      ga_util_digits[] = "0123456789abcdef";
static pthread_mutex_t gv_util_short_mutex = PTHREAD_MUTEX_INITIALIZER;

// type of assert
typedef enum {
    ASSERT_BOOLEXP,
    ASSERT_INTCMP,
    ASSERT_INTEXP,
    ASSERT_LONGCMP,
    ASSERT_LONGLONGCMP,
    ASSERT_NORMAL,
    ASSERT_PTRCMP,
    ASSERT_SIZETCMP,
    ASSERT_LAST
} Assert_Type;

// save assert info
typedef struct SB_Save_Assert_Type {
    SB_Buf_Line ia_buf;
    SB_Buf_Line ia_cmdline;
    SB_Buf_Line ia_cmdline_parent;
    SB_Buf_Line ia_exp;
    SB_Buf_Line ia_file;
    SB_Buf_Line ia_fun;
    SB_Buf_Line ia_pname;
    int         iv_errno;
    unsigned    iv_line;
    int         iv_pid;
    int         iv_ppid;
    int         iv_tid;
    long long   iv_lhs;
    long long   iv_rhs;
} SB_Save_Assert_Type;

SB_Save_Assert_Type gv_ms_save_assert;
char                gv_ms_save_log[BUFSIZ];

// forward
static void sb_util_assert_fun_com(Assert_Type  pv_assert,
                                   const char  *pp_exp,
                                   long long    pv_lhs,
                                   const char  *pp_op,
                                   long long    pv_rhs,
                                   const char  *pp_file,
                                   unsigned     pv_line,
                                   const char  *pp_fun);
void        sb_util_save_state();
void        sb_util_write_log(char *pp_buf);

//
// Purpose: abort
//
SB_Export void SB_util_abort(const char *pp_msg) {
    SB_Buf_Line  la_buf;
    char        *lp_pname;
#ifdef USE_SB_SP_LOG
    int          lv_status;
#endif

    sb_util_save_state();
    if (strlen(ga_ms_su_pname) == 0)
        lp_pname = const_cast<char *>("$<unkwn>");
    else
        lp_pname = ga_ms_su_pname;
        sprintf(la_buf, "Fatal error (%s-%d-%d): %s\n",
                lp_pname, getpid(), gettid(), pp_msg);
        fprintf(stderr, la_buf);
    fflush(stderr);
    sb_log_aggr_cap_log_flush();
#ifdef USE_SB_SP_LOG
    lv_status = SB_log_write_str(SQEVL_SEABED, // USE_SB_SP_LOG
                                 SB_EVENT_ID,
                                 SQ_LOG_SEAQUEST,
                                 SQ_LOG_CRIT,
                                 la_buf);
    CHK_STATUSIGNORE(lv_status);
#endif
    sb_util_write_log(la_buf);
    trace_flush();
    abort(); // SB_util_abort
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun(const char *pp_exp,
                                  const char *pp_file,
                                  unsigned    pv_line,
                                  const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_NORMAL,
                           pp_exp,
                           0,
                           "",
                           0,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
static void sb_util_assert_fun_com(Assert_Type  pv_assert,
                                   const char  *pp_exp,
                                   long long    pv_lhs,
                                   const char  *pp_op,
                                   long long    pv_rhs,
                                   const char  *pp_file,
                                   unsigned     pv_line,
                                   const char  *pp_fun) {
    SB_Buf_Line  la_cmdline;
    char        *lp_pname;
    pid_t        lv_pid;
#ifdef USE_SB_SP_LOG
    int          lv_status;
#endif
    pid_t        lv_tid;

    sb_util_save_state();
    char *lp_s = SB_util_get_cmdline(0,
                                     false, // args
                                     la_cmdline,
                                     sizeof(la_cmdline));
    if (lp_s == NULL)
        lp_s = const_cast<char *>("<unknown>");
    if (strlen(ga_ms_su_pname) == 0)
        lp_pname = const_cast<char *>("$<unkwn>");
    else
        lp_pname = ga_ms_su_pname;
    lv_pid = getpid();
    lv_tid = gettid();
    gv_ms_save_assert.iv_errno = errno;
    gv_ms_save_assert.iv_line = pv_line;
    gv_ms_save_assert.iv_lhs = pv_lhs;
    gv_ms_save_assert.iv_rhs = pv_rhs;
    strncpy(gv_ms_save_assert.ia_exp,
            pp_exp,
            sizeof(gv_ms_save_assert.ia_exp));
    strncpy(gv_ms_save_assert.ia_file,
            pp_file,
            sizeof(gv_ms_save_assert.ia_file));
    strncpy(gv_ms_save_assert.ia_fun,
            pp_fun,
            sizeof(gv_ms_save_assert.ia_fun));
    switch (pv_assert) {
    case ASSERT_BOOLEXP:
        sprintf(gv_ms_save_assert.ia_buf, "%s (%s-%d-%d): %s:%u %s: Assertion '%s' '(%s %lld)' failed.\n",
                lp_s,
                lp_pname, lv_pid, lv_tid,
                pp_file, pv_line, pp_fun,
                pp_exp, pp_op, pv_lhs);
        break;
    case ASSERT_INTCMP:
        sprintf(gv_ms_save_assert.ia_buf, "%s (%s-%d-%d): %s:%u %s: Assertion '%s' '(%lld %s %lld)' failed.\n",
                lp_s,
                lp_pname, lv_pid, lv_tid,
                pp_file, pv_line, pp_fun,
                pp_exp, pv_lhs, pp_op, pv_rhs);
        break;
    case ASSERT_INTEXP:
        sprintf(gv_ms_save_assert.ia_buf, "%s (%s-%d-%d): %s:%u %s: Assertion '%s' '(%s %lld)' failed.\n",
                lp_s,
                lp_pname, lv_pid, lv_tid,
                pp_file, pv_line, pp_fun,
                pp_exp, pp_op, pv_lhs);
        break;
    case ASSERT_LONGCMP:
        sprintf(gv_ms_save_assert.ia_buf, "%s (%s-%d-%d): %s:%u %s: Assertion '%s' '(%lld %s %lld)' failed.\n",
                lp_s,
                lp_pname, lv_pid, lv_tid,
                pp_file, pv_line, pp_fun,
                pp_exp, pv_lhs, pp_op, pv_rhs);
        break;
    case ASSERT_LONGLONGCMP:
        sprintf(gv_ms_save_assert.ia_buf, "%s (%s-%d-%d): %s:%u %s: Assertion '%s' '(%lld %s %lld)' failed.\n",
                lp_s,
                lp_pname, lv_pid, lv_tid,
                pp_file, pv_line, pp_fun,
                pp_exp, pv_lhs, pp_op, pv_rhs);
        break;
    case ASSERT_NORMAL:
        sprintf(gv_ms_save_assert.ia_buf, "%s (%s-%d-%d): %s:%u %s: Assertion '%s' failed.\n",
                lp_s,
                lp_pname, lv_pid, lv_tid,
                pp_file, pv_line, pp_fun,
                pp_exp);
        break;
    case ASSERT_PTRCMP:
        sprintf(gv_ms_save_assert.ia_buf, "%s (%s-%d-%d): %s:%u %s: Assertion '%s' '(%lld %s %lld)' failed.\n",
                lp_s,
                lp_pname, lv_pid, lv_tid,
                pp_file, pv_line, pp_fun,
                pp_exp, pv_lhs, pp_op, pv_rhs);
        break;
    case ASSERT_SIZETCMP:
        sprintf(gv_ms_save_assert.ia_buf, "%s (%s-%d-%d): %s:%u %s: Assertion '%s' '(%lld %s %lld)' failed.\n",
                lp_s,
                lp_pname, lv_pid, lv_tid,
                pp_file, pv_line, pp_fun,
                pp_exp, pv_lhs, pp_op, pv_rhs);
        break;
    default:
        abort(); // sw fault
        break;
    }

    fprintf(stderr, gv_ms_save_assert.ia_buf);
    fflush(stderr);
    sb_log_aggr_cap_log_flush();
#ifdef USE_SB_SP_LOG
    lv_status = SB_log_write_str(SQEVL_SEABED, // USE_SB_SP_LOG
                                 SB_EVENT_ID,
                                 SQ_LOG_SEAQUEST,
                                 SQ_LOG_CRIT,
                                 gv_ms_save_assert.ia_buf);
    CHK_STATUSIGNORE(lv_status);
#endif
    sb_util_write_log(gv_ms_save_assert.ia_buf);
    trace_flush();
    abort(); // sb_util_assert_fun_com
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_bf(const char *pp_exp,
                                     bool        pv_exp,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_BOOLEXP,
                           pp_exp,
                           pv_exp,
                           "!",
                           0,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_bt(const char *pp_exp,
                                     bool        pv_exp,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_BOOLEXP,
                           pp_exp,
                           pv_exp,
                           "",
                           0,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_cpeq(const char *pp_exp,
                                       const void *pp_lhs,
                                       const void *pp_rhs,
                                       const char *pp_file,
                                       unsigned    pv_line,
                                       const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_PTRCMP,
                           pp_exp,
                           reinterpret_cast<long>(pp_lhs),
                           "==",
                           reinterpret_cast<long>(pp_rhs),
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_cpne(const char *pp_exp,
                                       const void *pp_lhs,
                                       const void *pp_rhs,
                                       const char *pp_file,
                                       unsigned    pv_line,
                                       const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_PTRCMP,
                           pp_exp,
                           reinterpret_cast<long>(pp_lhs),
                           "!=",
                           reinterpret_cast<long>(pp_rhs),
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_ieq(const char *pp_exp,
                                      int         pv_lhs,
                                      int         pv_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_INTCMP,
                           pp_exp,
                           pv_lhs,
                           "==",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_if(const char *pp_exp,
                                     int         pv_exp,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_INTEXP,
                           pp_exp,
                           pv_exp,
                           "!",
                           0,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_ige(const char *pp_exp,
                                      int         pv_lhs,
                                      int         pv_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_INTCMP,
                           pp_exp,
                           pv_lhs,
                           ">=",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_igt(const char *pp_exp,
                                      int         pv_lhs,
                                      int         pv_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_INTCMP,
                           pp_exp,
                           pv_lhs,
                           ">",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_ile(const char *pp_exp,
                                      int         pv_lhs,
                                      int         pv_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_INTCMP,
                           pp_exp,
                           pv_lhs,
                           "<=",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_ilt(const char *pp_exp,
                                      int         pv_lhs,
                                      int         pv_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_INTCMP,
                           pp_exp,
                           pv_lhs,
                           "<",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_ine(const char *pp_exp,
                                      int         pv_lhs,
                                      int         pv_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_INTCMP,
                           pp_exp,
                           pv_lhs,
                           "!=",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_it(const char *pp_exp,
                                     int         pv_exp,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_INTEXP,
                           pp_exp,
                           pv_exp,
                           "",
                           0,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_leq(const char *pp_exp,
                                      long        pv_lhs,
                                      long        pv_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_LONGCMP,
                           pp_exp,
                           pv_lhs,
                           "==",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_lge(const char *pp_exp,
                                      long        pv_lhs,
                                      long        pv_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_LONGCMP,
                           pp_exp,
                           pv_lhs,
                           ">=",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_lgt(const char *pp_exp,
                                      long        pv_lhs,
                                      long        pv_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_LONGCMP,
                           pp_exp,
                           pv_lhs,
                           ">",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_lle(const char *pp_exp,
                                      long        pv_lhs,
                                      long        pv_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_LONGCMP,
                           pp_exp,
                           pv_lhs,
                           "<=",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_llt(const char *pp_exp,
                                      long        pv_lhs,
                                      long        pv_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_LONGCMP,
                           pp_exp,
                           pv_lhs,
                           "<",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_lne(const char *pp_exp,
                                      long        pv_lhs,
                                      long        pv_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_LONGCMP,
                           pp_exp,
                           pv_lhs,
                           "!=",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_lleq(const char *pp_exp,
                                       long long   pv_lhs,
                                       long long   pv_rhs,
                                       const char *pp_file,
                                       unsigned    pv_line,
                                       const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_LONGLONGCMP,
                           pp_exp,
                           pv_lhs,
                           "==",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_llge(const char *pp_exp,
                                       long long   pv_lhs,
                                       long long   pv_rhs,
                                       const char *pp_file,
                                       unsigned    pv_line,
                                       const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_LONGLONGCMP,
                           pp_exp,
                           pv_lhs,
                           ">=",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_llgt(const char *pp_exp,
                                       long long   pv_lhs,
                                       long long   pv_rhs,
                                       const char *pp_file,
                                       unsigned    pv_line,
                                       const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_LONGLONGCMP,
                           pp_exp,
                           pv_lhs,
                           ">",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_llle(const char *pp_exp,
                                       long long   pv_lhs,
                                       long long   pv_rhs,
                                       const char *pp_file,
                                       unsigned    pv_line,
                                       const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_LONGLONGCMP,
                           pp_exp,
                           pv_lhs,
                           "<=",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_lllt(const char *pp_exp,
                                       long long   pv_lhs,
                                       long long   pv_rhs,
                                       const char *pp_file,
                                       unsigned    pv_line,
                                       const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_LONGLONGCMP,
                           pp_exp,
                           pv_lhs,
                           "<",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_llne(const char *pp_exp,
                                       long long   pv_lhs,
                                       long long   pv_rhs,
                                       const char *pp_file,
                                       unsigned    pv_line,
                                       const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_LONGLONGCMP,
                           pp_exp,
                           pv_lhs,
                           "!=",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_steq(const char *pp_exp,
                                       size_t      pv_lhs,
                                       size_t      pv_rhs,
                                       const char *pp_file,
                                       unsigned    pv_line,
                                       const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_SIZETCMP,
                           pp_exp,
                           pv_lhs,
                           "==",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_stge(const char *pp_exp,
                                       size_t      pv_lhs,
                                       size_t      pv_rhs,
                                       const char *pp_file,
                                       unsigned    pv_line,
                                       const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_SIZETCMP,
                           pp_exp,
                           pv_lhs,
                           ">=",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_stgt(const char *pp_exp,
                                       size_t      pv_lhs,
                                       size_t      pv_rhs,
                                       const char *pp_file,
                                       unsigned    pv_line,
                                       const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_SIZETCMP,
                           pp_exp,
                           pv_lhs,
                           ">",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_stle(const char *pp_exp,
                                       size_t      pv_lhs,
                                       size_t      pv_rhs,
                                       const char *pp_file,
                                       unsigned    pv_line,
                                       const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_SIZETCMP,
                           pp_exp,
                           pv_lhs,
                           "<=",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_stlt(const char *pp_exp,
                                       size_t      pv_lhs,
                                       size_t      pv_rhs,
                                       const char *pp_file,
                                       unsigned    pv_line,
                                       const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_SIZETCMP,
                           pp_exp,
                           pv_lhs,
                           "<",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_stne(const char *pp_exp,
                                       size_t      pv_lhs,
                                       size_t      pv_rhs,
                                       const char *pp_file,
                                       unsigned    pv_line,
                                       const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_SIZETCMP,
                           pp_exp,
                           pv_lhs,
                           "!=",
                           pv_rhs,
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_peq(const char *pp_exp,
                                      void       *pp_lhs,
                                      void       *pp_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_PTRCMP,
                           pp_exp,
                           reinterpret_cast<long>(pp_lhs),
                           "==",
                           reinterpret_cast<long>(pp_rhs),
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_pgt(const char *pp_exp,
                                      void       *pp_lhs,
                                      void       *pp_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_PTRCMP,
                           pp_exp,
                           reinterpret_cast<long>(pp_lhs),
                           ">",
                           reinterpret_cast<long>(pp_rhs),
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_pge(const char *pp_exp,
                                      void       *pp_lhs,
                                      void       *pp_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_PTRCMP,
                           pp_exp,
                           reinterpret_cast<long>(pp_lhs),
                           ">=",
                           reinterpret_cast<long>(pp_rhs),
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_plt(const char *pp_exp,
                                      void       *pp_lhs,
                                      void       *pp_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_PTRCMP,
                           pp_exp,
                           reinterpret_cast<long>(pp_lhs),
                           "<",
                           reinterpret_cast<long>(pp_rhs),
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_ple(const char *pp_exp,
                                      void       *pp_lhs,
                                      void       *pp_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_PTRCMP,
                           pp_exp,
                           reinterpret_cast<long>(pp_lhs),
                           "<=",
                           reinterpret_cast<long>(pp_rhs),
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: assert
//
SB_Export void SB_util_assert_fun_pne(const char *pp_exp,
                                      void       *pp_lhs,
                                      void       *pp_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun) {
    sb_util_assert_fun_com(ASSERT_PTRCMP,
                           pp_exp,
                           reinterpret_cast<long>(pp_lhs),
                           "!=",
                           reinterpret_cast<long>(pp_rhs),
                           pp_file,
                           pv_line,
                           pp_fun);
}

//
// Purpose: fatal error
//
SB_Export void SB_util_fatal(const char *pp_msg,
                             bool        pv_stderr) SB_THROWS_FATAL {
    SB_Buf_Line  la_buf;
    char        *lp_pname;
#ifdef USE_SB_SP_LOG
    int          lv_status;
#endif

    sb_util_save_state();
    if (strlen(ga_ms_su_pname) == 0)
        lp_pname = const_cast<char *>("$<unkwn>");
    else
        lp_pname = ga_ms_su_pname;
    if (pv_stderr) {
        sprintf(la_buf, "Fatal error (%s-%d-%d): %s\n",
                lp_pname, getpid(), gettid(), pp_msg);
        fprintf(stderr, la_buf);
    }
    fflush(stderr);
    sb_log_aggr_cap_log_flush();
#ifdef USE_SB_SP_LOG
    lv_status = SB_log_write_str(SQEVL_SEABED, // USE_SB_SP_LOG
                                 SB_EVENT_ID,
                                 SQ_LOG_SEAQUEST,
                                 SQ_LOG_CRIT,
                                 la_buf);
    CHK_STATUSIGNORE(lv_status);
#endif
    sb_util_write_log(la_buf);
    trace_flush();
    SB_THROW_FATAL(pp_msg);
}

//
// Purpose: get name in a consistently case insensitive manner
//
SB_Export void SB_util_get_case_insensitive_name(char *pp_inname,
                                                 char *pp_outname) {
    int lv_inx;
    int lv_len;

    lv_len = static_cast<int>(strlen(pp_inname));
    SB_util_assert_ilt(lv_len, MS_MON_MAX_PROCESS_NAME);

    for (lv_inx = 0; lv_inx < lv_len; lv_inx++)
        pp_outname[lv_inx] = static_cast<char>(toupper(pp_inname[lv_inx]));

    pp_outname[lv_len] = '\0';
}

//
// Purpose: get cmdline
//
SB_Export char *SB_util_get_cmdline(int   pv_pid,
                                    bool  pv_args,
                                    char *pp_cmdline,
                                    int   pv_len) {
    SB_Buf_Line  la_line;
    char        *lp_buf;
    char        *lp_end;
    FILE        *lp_file;
    char        *lp_p;
    size_t       lv_ret;

    if (pv_pid == 0)
        strcpy(la_line, "/proc/self/cmdline");
    else
        sprintf(la_line, "/proc/%d/cmdline", pv_pid);
    lp_file = fopen(la_line, "r");
    if (lp_file != NULL) {
        if (pv_args) {
            lv_ret = fread(pp_cmdline, 1, pv_len, lp_file);
            lp_buf = pp_cmdline;
            if (lv_ret == static_cast<size_t>(pv_len))
                lv_ret--; // don't overflow
            lp_end = &lp_buf[lv_ret];
            lp_end[0] = '\0';
            if ((lv_ret > 0) && (lp_end[-1] == 0))
                lp_end[-1] = '\0';
            while (lp_buf < lp_end) {
                lp_p = strchr(lp_buf, 0);
                if ((lp_p != NULL) && (lp_p < lp_end)) {
                    if (&lp_p[1] == lp_end)
                        lp_p[0] = '\0';
                    else
                        lp_p[0] = ' ';
                    lp_buf = &lp_p[1];
                } else
                    lp_buf = lp_end;
            }
            lp_p = pp_cmdline;
        } else
            lp_p = fgets(pp_cmdline, pv_len, lp_file);
        fclose(lp_file);
        return lp_p;
    }
    return NULL;
}

//
// Purpose: get exe name
//
SB_Export void SB_util_get_exe(char *pp_exe, int pv_exe_max, bool pv_base) {
    char  la_exe[PATH_MAX];
    int   lv_ret;
    char *lp_p;

    if (pv_base) {
        lv_ret = static_cast<int>(readlink("/proc/self/exe", la_exe, PATH_MAX));
        if (lv_ret == -1)
            la_exe[0] = '\0';
        else
            la_exe[lv_ret] = '\0';
        lp_p = rindex(la_exe, '/');
        if (lp_p == NULL)
            strcpy(pp_exe, la_exe);
        else
            strcpy(pp_exe, &lp_p[1]);
    } else {
        lv_ret = static_cast<int>(readlink("/proc/self/exe",
                                           pp_exe, pv_exe_max));
        if (lv_ret == -1)
            pp_exe[0] = '\0';
        else
            pp_exe[lv_ret] = '\0';
    }
}

//
// Purpose: int-to-ascii (of a number [base])
//
SB_Export char *SB_util_itoa_int(char         *pp_str,
                                 unsigned int  pv_num,
                                 int           pv_base) {
    char         *lp_str;
    unsigned int  lv_num;
    int           lv_width;

    lv_width = 1;
    switch (pv_base) {
    case 10:
        if (static_cast<int>(pv_num) < 0) {
            *pp_str = '-';
            pv_num = static_cast<unsigned int>(0 - static_cast<int>(pv_num));
            lv_width++;
        }
        lv_num = pv_num;
        // Using literals causes better code generation
        while (lv_num /= 10)
            lv_width++;
        lp_str = pp_str + lv_width;
        do {
            *--lp_str = ga_util_digits[pv_num % 10];
        } while (pv_num /= 10);
        break;

    case 16:
        lv_num = pv_num;
        // Using literals causes better code generation
        while (lv_num /= 16)
            lv_width++;
        lp_str = pp_str + lv_width;
        do {
            *--lp_str = ga_util_digits[pv_num % 16];
        } while (pv_num /= 16);
        break;

    default:
        lv_num = pv_num;
        while (lv_num /= pv_base)
            lv_width++;
        lp_str = pp_str + lv_width;
        do {
            *--lp_str = ga_util_digits[pv_num % pv_base];
        } while (pv_num /= pv_base);
        break;
    }

    return pp_str + lv_width;
}

//
// Purpose: int-to-ascii (of a pointer)
//
SB_Export char *SB_util_itoa_ptr(char *pp_str, void *pp_ptr) {
    char          *lp_str;
    unsigned long  lv_num;
    int            lv_width;

    if (pp_ptr == NULL) {
        lv_width = 5;
        lp_str = pp_str;
        *lp_str++ = '(';
        *lp_str++ = 'n';
        *lp_str++ = 'i';
        *lp_str++ = 'l';
        *lp_str++ = ')';
    } else {
        lv_width = 3;
        pp_str[0] = '0';
        pp_str[1] = 'x';
        lv_num = reinterpret_cast<unsigned long>(pp_ptr);
        // Using literals causes better code generation
        while (lv_num /= 16)
            lv_width++;
        lp_str = pp_str + lv_width;
        lv_num = reinterpret_cast<unsigned long>(pp_ptr);
        do {
            *--lp_str = ga_util_digits[lv_num % 16];
        } while (lv_num /= 16);
    }

    return pp_str + lv_width;
}

//
// Purpose: save some state
//
void sb_util_save_state() {
    char  *lp_pname;
    pid_t  lv_pid;
    pid_t  lv_ppid;
    pid_t  lv_tid;

    lv_pid = getpid();
    lv_ppid = getppid();
    lv_tid = gettid();
    gv_ms_save_assert.iv_pid = lv_pid;
    gv_ms_save_assert.iv_ppid = lv_ppid;
    gv_ms_save_assert.iv_tid = lv_tid;
    if (strlen(ga_ms_su_pname) == 0)
        lp_pname = const_cast<char *>("$<unkwn>");
    else
        lp_pname = ga_ms_su_pname;
    SB_util_get_cmdline(0,
                        true, // args
                        gv_ms_save_assert.ia_cmdline,
                        sizeof(gv_ms_save_assert.ia_cmdline));
    SB_util_get_cmdline(lv_ppid,
                        true, // args
                        gv_ms_save_assert.ia_cmdline_parent,
                        sizeof(gv_ms_save_assert.ia_cmdline));
    strncpy(gv_ms_save_assert.ia_pname,
            lp_pname,
            sizeof(gv_ms_save_assert.ia_pname));
}

//
// Purpose: set log info
//
SB_Export void  SB_util_set_log(int pv_compid, int pv_zid) {
    gv_ms_su_compid = pv_compid;
    gv_ms_su_zid = pv_zid;
}

//
// Purpose: set process-name
//
SB_Export void SB_util_set_pname(const char *pp_pname) {
    strcpy(ga_ms_su_pname, pp_pname);
}

//
// Purpose: set 'short' lock
//
SB_Export void SB_util_short_lock() {
    int lv_status;

    lv_status = pthread_mutex_lock(&gv_util_short_mutex);
    SB_util_assert_ieq(lv_status, 0);
}

//
// Purpose: clear 'short' lock
//
SB_Export void SB_util_short_unlock() {
    int lv_status;

    lv_status = pthread_mutex_unlock(&gv_util_short_mutex);
    SB_util_assert_ieq(lv_status, 0);
}

//
// Purpose: write to log
//
void sb_util_write_log(char *pp_buf) {
    char            la_log_file_dir[PATH_MAX];
    char           *lp_log_file_dir;
    char           *lp_root;

    strncpy(gv_ms_save_log, pp_buf, sizeof(gv_ms_save_log) - 1);
    gv_ms_save_log[sizeof(gv_ms_save_log) - 1] = '\0';
    lp_root = getenv("TRAF_LOG");
    if (lp_root == NULL)
        lp_log_file_dir = NULL;
    else {
        sprintf(la_log_file_dir, "%s", lp_root);
        lp_log_file_dir = la_log_file_dir;
    }
    SBX_log_write(SBX_LOG_TYPE_LOGFILE |        // log_type
                  SBX_LOG_TYPE_LOGFILE_PSTACK |
                  SBX_LOG_TYPE_SYSLOG,
                  lp_log_file_dir,              // log_file_dir
                  "zsb",                        // log_file_prefix
                  SQEVL_SEABED,                 // comp_id
                  SB_EVENT_ID,                  // event_id
                  SQ_LOG_SEAQUEST,              // facility
                  SQ_LOG_CRIT,                  // severity
                  ga_ms_su_pname,               // name
                  NULL,                         // msg_prefix
                  pp_buf,                       // msg
                  NULL,                         // snmptrap_cmd
                  NULL,                         // msg_snmptrap
                  gv_ms_save_log,               // msg_ret
                  sizeof(gv_ms_save_log));      // msg_ret size
}
