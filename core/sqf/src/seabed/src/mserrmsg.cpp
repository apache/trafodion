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

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/trace.h"

#include "seabed/int/assert.h"

#include "buf.h"
#include "mserrmsg.h"
#include "mstrace.h"
#include "msvars.h"
#include "util.h"
#include "utracex.h"

typedef struct MSG_ERROR_TEMPLATE {
    short dialect_type;
    short reply_type;
    short reply_version;
    short error;
} MSG_ERROR_TEMPLATE;
enum { MSG_ERROR_SIMPLE_REPLY_SIZE = sizeof(MSG_ERROR_TEMPLATE) };
enum { DIALECT_MSG_ERROR           = 1 }; // NSK: DIALECT_MSG_ERROR
enum { MSG_ERROR_SIMPLE_REPLY      = 3 }; // NSK: MSG_ERROR_SIMPLE_REPLY
enum { MSG_ERROR_VERSION_D00       = 1 }; // NSK: MSG_ERROR_VERSION_D00

//
// Purpose: ms break error
//
void ms_err_break_err(const char *pp_where,
                      short       pv_fserr,
                      MS_Md_Type *pp_md,
                      short      *pp_results) {
    SB_Buf_Line         la_log_buf;
    MSG_ERROR_TEMPLATE *lp_error;
    MS_Result_Type     *lp_results;
    MS_Result_Raw_Type *lp_results_raw;

    if (pp_md == NULL) {
        sprintf(la_log_buf, "%s: no MD", pp_where);
        SB_util_fatal(la_log_buf, true); // cannot determine reply-ctrl
    }
    lp_error =
      reinterpret_cast<MSG_ERROR_TEMPLATE *>(pp_md->out.ip_reply_ctrl);
    lp_error->dialect_type = DIALECT_MSG_ERROR;
    lp_error->reply_type = MSG_ERROR_SIMPLE_REPLY;
    lp_error->reply_version = MSG_ERROR_VERSION_D00;
    lp_error->error = pv_fserr;

    lp_results = reinterpret_cast<MS_Result_Type *>(pp_results);
    lp_results_raw = reinterpret_cast<MS_Result_Raw_Type *>(pp_results);
    lp_results->rr_ctrlsize = sizeof(MSG_ERROR_TEMPLATE);
    lp_results->rr_datasize = 0;
    lp_results_raw->rrerr = 0;
    lp_results->rrerr_errorb = 1;
    lp_results->rrerr_mserrb = 1;
    if (gv_ms_trace_errors)
        trace_where_printf(pp_where, "setting repC, repC=%p, max=%d, dtype=%d, rtype=%d(MSG_ERROR_SIMPLE_REPLY), rvers=%d, fserr=%d\n",
                           pfp(pp_md->out.ip_reply_ctrl),
                           pp_md->out.iv_reply_ctrl_max,
                           lp_error->dialect_type,
                           lp_error->reply_type,
                           lp_error->reply_version,
                           lp_error->error);
}

//
// Purpose: return an ms error
//
short ms_err_rtn(short pv_fserr) {
    if (pv_fserr != XZFIL_ERR_OK) {
        SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MS_EXIT, pv_fserr);
        if (gv_ms_trace_errors)
            trace_printf("setting ms ret=%d\n", pv_fserr);
        if (gv_ms_assert_error)
            SB_util_assert_ieq(pv_fserr, XZFIL_ERR_OK); // sw fault
    }
    return pv_fserr;
}

//
// Purpose: fatal error
//
short ms_err_rtn_fatal(const char *pp_msg, short pv_fserr, bool pv_stderr) {
    if (gv_ms_trace_errors)
        trace_printf("setting ms ret=%d\n", pv_fserr);
    SB_util_fatal(pp_msg, pv_stderr);
    return pv_fserr;
}

//
// Purpose: trace msg and return an ms error
//
short ms_err_rtn_msg(const char *pp_where,
                     const char *pp_msg,
                     short       pv_fserr) {
    if (pv_fserr != XZFIL_ERR_OK) {
        SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_MS_EXIT, pv_fserr);
        if (gv_ms_trace_errors)
            trace_where_printf(pp_where, "setting ms (%s) ret=%d\n",
                               pp_msg, pv_fserr);
        if (gv_ms_assert_error)
            SB_util_assert_ieq(pv_fserr, XZFIL_ERR_OK); // sw fault
    }
    if (gv_ms_trace_params)
        trace_where_printf(pp_where, "%s\n", pp_msg);
    return ms_err_rtn(pv_fserr);
}

//
// Purpose: like ms_err_rtn_msg, but the error is fatal
//
short ms_err_rtn_msg_fatal(const char *pp_where,
                           const char *pp_msg,
                           short       pv_fserr,
                           bool        pv_stderr) {
    SB_Buf_Line la_msg;

    if (gv_ms_trace_errors)
        trace_where_printf(pp_where, "setting ms (%s) ret=%d\n",
                           pp_msg, pv_fserr);
    sprintf(la_msg, "%s, fserr=%d", pp_msg, pv_fserr);
    return ms_err_rtn_fatal(la_msg, pv_fserr, pv_stderr);
}

//
// Purpose: trace msg and return an ms error
//
short ms_err_rtn_msg_noassert(const char *pp_where,
                              const char *pp_msg,
                              short       pv_fserr) {
    if (pv_fserr != XZFIL_ERR_OK) {
        if (gv_ms_trace_errors)
            trace_where_printf(pp_where, "setting ms (%s) ret=%d\n",
                               pp_msg, pv_fserr);
    }
    if (gv_ms_trace_params)
        trace_where_printf(pp_where, "%s\n", pp_msg);
    return ms_err_rtn_noassert(pv_fserr);
}

//
// Purpose: return an ms error
//
short ms_err_rtn_noassert(short pv_fserr) {
    if (pv_fserr != XZFIL_ERR_OK) {
        if (gv_ms_trace_errors)
            trace_printf("setting ms ret=%d\n", pv_fserr);
    }
    return pv_fserr;
}

//
// Purpose: fatal error
//
short ms_err_sock_rtn_msg_fatal(const char *pp_where,
                                const char *pp_msg,
                                int         /* pv_err */) {
    short lv_fserr = 1; // TODO
    return ms_err_rtn_msg_fatal(pp_where, pp_msg, lv_fserr, true);
}


