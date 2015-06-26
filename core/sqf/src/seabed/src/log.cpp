//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

#include <stdio.h>
#include <wchar.h>
#include <unistd.h>

#include "common/evl_sqlog_eventnum.h"

#include "seabed/log.h"
#include "seabed/mslimits.h"
#include "seabed/thread.h"
#include "seabed/trace.h"

#include "mstrace.h"
#include "threadtlsx.h"


//
// SB wrappers for logging
//


enum { LOG_BUF_SIZE = LOG_DEFAULT_BUF_SIZE };

// forwards
static void sb_log_key_dtor(void *pp_log_buf);

static bool gv_logging       = true;
int         gv_ms_su_compid  = 0;
char        ga_ms_su_pname[MS_MON_MAX_PROCESS_NAME] = "$<unkwn>";
#ifdef SQ_PHANDLE_VERIFIER
char        ga_ms_su_pname_seq[MS_MON_MAX_PROCESS_NAME] = "$<unkwn>";
#endif
int         gv_ms_su_zid     = -1;
static int  gv_log_tls_inx =
              SB_create_tls_key(sb_log_key_dtor, "log-buf");

static char *sb_log_get_buf() {
    int lv_status;

    char *lp_log_buf =
      static_cast<char *>(SB_Thread::Sthr::specific_get(gv_log_tls_inx));
    if (lp_log_buf == NULL) {
        lp_log_buf = new char[LOG_BUF_SIZE];
        lv_status = SB_Thread::Sthr::specific_set(gv_log_tls_inx, lp_log_buf);
        SB_util_assert_ieq(lv_status, 0);
    }
    return lp_log_buf;
}

static void sb_log_key_dtor(void *pp_log_buf) {
    if (pp_log_buf != NULL) {
        char *lp_log_buf = static_cast<char *>(pp_log_buf);
        delete [] lp_log_buf;
    }
}

SB_Export int SB_log_add_array_token(char   *pp_buf,
                                     int     pv_tk_type,
                                     void   *pp_tk_value,
                                     size_t  pv_count) {
    int lv_err;

    pp_buf = pp_buf;
    pv_tk_type = pv_tk_type;
    pp_tk_value = pp_tk_value;
    pv_count = pv_count;
    lv_err = 0;
    return lv_err;
}

SB_Export int SB_log_add_token(char *pp_buf,
                               int   pv_tk_type,
                               void *pp_tk_value) {
    const char *WHERE = "SB_log_add_token";
    const char *lp_tok_type;
    void       *lp_v;
    int         lv_err;

    if (gv_logging) {
        if (gv_ms_trace_evlog) {
            lp_v = static_cast<void *>(pp_tk_value);
            switch (pv_tk_type) {
            case TY_CHAR:
                lp_tok_type = "char";
                break;
            case TY_UCHAR:
                lp_tok_type = "uchar";
                break;
            case TY_SHORT:
                lp_tok_type = "short";
                break;
            case TY_USHORT:
                lp_tok_type = "ushort";
                break;
            case TY_INT:
                lp_tok_type = "int";
                break;
            case TY_UINT:
                lp_tok_type = "uint";
                break;
            case TY_LONG:
                lp_tok_type = "long";
                break;
            case TY_ULONG:
                lp_tok_type = "ulong";
                break;
            case TY_LONGLONG:
                lp_tok_type = "longlong";
                break;
            case TY_ULONGLONG:
                lp_tok_type = "ulonglong";
                break;
            case TY_ADDRESS:
                lp_tok_type = "address";
                break;
            case TY_FLOAT:
                lp_tok_type = "float";
                break;
            case TY_DOUBLE:
                lp_tok_type = "double";
                break;
            case TY_LDOUBLE:
                lp_tok_type = "ldouble";
                break;
            case TY_STRING:
                lp_tok_type = "string";
                break;
            case TY_WCHAR:
                lp_tok_type = "wchar";
                break;
            case TY_WSTRING:
                lp_tok_type = "wstring";
                break;
            default:
                lp_tok_type = "?";
                break;
            }
            switch (pv_tk_type) {
            case TY_CHAR:
            case TY_UCHAR:
                trace_where_printf(WHERE, "type=%s, value=%c",
                                   lp_tok_type,
                                   *reinterpret_cast<char *>(lp_v));
                break;
            case TY_SHORT:
            case TY_USHORT:
                trace_where_printf(WHERE, "type=%s, value=%dh",
                                   lp_tok_type,
                                   *reinterpret_cast<short *>(lp_v));
                break;
            case TY_INT:
            case TY_UINT:
                trace_where_printf(WHERE, "type=%s, value=%d",
                                   lp_tok_type,
                                   *reinterpret_cast<int *>(lp_v));
                break;
            case TY_LONG:
            case TY_ULONG:
                trace_where_printf(WHERE, "type=%s, value=%ld",
                                   lp_tok_type,
                                   *reinterpret_cast<long *>(lp_v));
                break;
            case TY_LONGLONG:
            case TY_ULONGLONG:
                trace_where_printf(WHERE, "type=%s, value=%lld",
                                   lp_tok_type,
                                   *reinterpret_cast<long long *>(lp_v));
                break;
            case TY_ADDRESS:
                trace_where_printf(WHERE, "type=%s, value=%p",
                                   lp_tok_type,
                                   lp_v);
                break;
            case TY_FLOAT:
                trace_where_printf(WHERE, "type=%s, value=%f",
                                   lp_tok_type,
                                   *reinterpret_cast<float *>(lp_v));
                break;
            case TY_DOUBLE:
                trace_where_printf(WHERE, "type=%s, value=%f",
                                   lp_tok_type,
                                   *reinterpret_cast<double *>(lp_v));
                break;
            case TY_LDOUBLE:
                trace_where_printf(WHERE, "type=%s, value=%Lf",
                                   lp_tok_type,
                                   *reinterpret_cast<long double *>(lp_v));
                break;
            case TY_STRING:
                trace_where_printf(WHERE, "type=%s, value=%s",
                                   lp_tok_type,
                                   reinterpret_cast<char *>(lp_v));
                break;
            case TY_WCHAR:
                trace_where_printf(WHERE, "type=%s, value=%c",
                                   lp_tok_type,
                                   *reinterpret_cast<wchar_t *>(lp_v));
                break;
            case TY_WSTRING:
                trace_where_printf(WHERE, "type=%s, value=%ls",
                                   lp_tok_type,
                                   reinterpret_cast<wchar_t *>(lp_v));
                break;
            default:
                break;
            }
        }
        pp_buf = pp_buf;
        lv_err = 0;
    } else
        lv_err = 0;
    return lv_err;
}

SB_Export int SB_log_buf_used(char *pp_buf) {
    pp_buf = pp_buf;
    return 0;
}

SB_Export void SB_log_enable_logging(bool pv_logging) {
    gv_logging = pv_logging;
}

SB_Export int SB_log_init(int pv_comp_id, char *pp_buf, size_t pv_buf_maxlen) {
    int                lv_err;
    pv_comp_id = pv_comp_id;
    pp_buf = pp_buf;
    pv_buf_maxlen = pv_buf_maxlen;
    lv_err = 0;
    return lv_err;
}

SB_Export int SB_log_init_compid(int pv_comp_id) {
    gv_ms_su_compid = pv_comp_id;
    return 0;
}

SB_Export int SB_log_write(posix_sqlog_facility_t  pv_facility,
                           int                     pv_event_type,
                           posix_sqlog_severity_t  pv_severity,
                           char                   *pp_buf) {
    int         lv_err;

    pv_facility = pv_facility;
    pv_event_type = pv_event_type;
    pv_severity = pv_severity;
    pp_buf = pp_buf;
    lv_err = 0;
    return lv_err;
}

SB_Export int SB_log_write_str(int                     pv_comp_id,
                               int                     pv_event_id,
                               posix_sqlog_facility_t  pv_facility,
                               posix_sqlog_severity_t  pv_severity,
                               char                   *pp_str) {
    char la_ebuf[LOG_DEFAULT_BUF_SIZE];

    int lv_err = SB_log_init(pv_comp_id, la_ebuf, sizeof(la_ebuf));
    if (lv_err == 0)
        lv_err = SB_log_add_token(la_ebuf,
                                  TY_STRING,
                                  pp_str);
    if (lv_err == 0)
        lv_err = SB_log_write(pv_facility,
                              pv_event_id,
                              pv_severity,
                              la_ebuf);
    return lv_err;
}

SB_Export int SB_log_ts_add_array_token(int     pv_tk_type,
                                        void   *pp_tk_value,
                                        size_t  pv_count) {
    return SB_log_add_array_token(sb_log_get_buf(),
                                  pv_tk_type,
                                  pp_tk_value,
                                  pv_count);
}

SB_Export int SB_log_ts_add_token(int   pv_tk_type,
                                  void *pp_tk_value) {
    return SB_log_add_token(sb_log_get_buf(), pv_tk_type, pp_tk_value);
}

SB_Export int SB_log_ts_init(int pv_comp_id) {
    return SB_log_init(pv_comp_id, sb_log_get_buf(), LOG_BUF_SIZE);
}

SB_Export int SB_log_ts_write(posix_sqlog_facility_t  pv_facility,
                              int                     pv_event_type,
                              posix_sqlog_severity_t  pv_severity) {
    return SB_log_write(pv_facility,
                        pv_event_type,
                        pv_severity,
                        sb_log_get_buf());
}

