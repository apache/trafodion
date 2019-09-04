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
// Implement SB_Trans::MPI_Msg
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "seabed/fserr.h"
#include "seabed/trace.h"

#include "cap.h"
#include "env.h"
#include "logaggr.h"
#include "mstrace.h"
#include "msx.h"
#include "qid.h"


//
// statics
//
SB_Atomic_Int                SB_Trans::Msg_Mgr::cv_md_table_count_recv;
SB_Atomic_Int                SB_Trans::Msg_Mgr::cv_md_table_count_send;
SB_Atomic_Int                SB_Trans::Msg_Mgr::cv_md_table_count_total;
int                          SB_Trans::Msg_Mgr::cv_md_table_hi_recv = 0;
int                          SB_Trans::Msg_Mgr::cv_md_table_hi_send = 0;
int                          SB_Trans::Msg_Mgr::cv_md_table_hi_total = 0;
int                          SB_Trans::Msg_Mgr::cv_md_table_max_recv = 255;
int                          SB_Trans::Msg_Mgr::cv_md_table_max_send = 1023;


//
// Purpose: get a free md
//
int SB_Trans::Msg_Mgr::get_md(MS_Md_Type      **ppp_md,
                              void             *pp_stream,
                              SB_Ms_Event_Mgr  *pp_mgr,
                              bool              pv_dir_send,
                              short            *pp_fserr,
                              const char       *pp_where,
                              int               pv_md_state) {
    int   lv_count;
    short lv_fserr;

    lv_fserr = XZFIL_ERR_OK;
    if (pp_fserr != NULL) {
        // only check counts if pp_fserr
        if (pv_dir_send) {
            lv_count = cv_md_table_count_send.read_val();
            if (lv_count >= cv_md_table_max_send) {
                if (gv_ms_trace_md)
                    trace_printf("get_md send-count limit, out-sends=%d, max-sends=%d\n",
                                 lv_count, cv_md_table_max_send);

                lv_fserr = XZFIL_ERR_NOLCB;
            } else if (gv_ms_trace_md & gv_ms_trace_verbose) {
                if (gv_ms_trace_md)
                    trace_printf("get_md send-count ok, out-sends=%d, max-sends=%d\n",
                                 lv_count, cv_md_table_max_send);
            }
        } else {
            // recv
            lv_count = cv_md_table_count_recv.read_val();
            if (lv_count >= cv_md_table_max_recv) {
                if (gv_ms_trace_md)
                    trace_printf("get_md recv-count limit, out-recvs=%d, max-recvs=%d\n",
                                 lv_count, cv_md_table_max_recv);
                lv_fserr = XZFIL_ERR_NOLCB;
            } else if (gv_ms_trace_md & gv_ms_trace_verbose) {
                if (gv_ms_trace_md)
                    trace_printf("get_md recv-count ok, out-recvs=%d, max-recvs=%d\n",
                                 lv_count, cv_md_table_max_recv);
            }
        }
    }
    if (lv_fserr == XZFIL_ERR_OK) {
        cv_md_table.lock();
        size_t lv_msgid = cv_md_table.alloc_entry_lock(false);
        MS_Md_Type *lp_md = cv_md_table.get_entry_lock(lv_msgid, false);
        lp_md->iv_md_state = pv_md_state;
        cv_md_table.unlock();
        reset_md(lp_md, pp_stream, pp_mgr, pp_where);
        lv_count = cv_md_table_count_total.add_and_fetch(1);
        // data-race if multiple threads (may not be completely accurate)
        if (lv_count > cv_md_table_hi_total)
            cv_md_table_hi_total = lv_count;
        lp_md->iv_dir_send = pv_dir_send;
        if (pv_dir_send) {
            lv_count = cv_md_table_count_send.add_and_fetch(1);
            if (lv_count > cv_md_table_hi_send) {
                cv_md_table_hi_send = lv_count;
                gv_ms_cap.iv_md_table_hi_send = lv_count;
                if ((cv_md_table_hi_send % LOG_HIGH) == 0)
                    log_high(pp_where,
                             cv_md_table_hi_send,
                             "send",
                             SB_LOG_AGGR_CAP_MD_SEND_HI_CAP_INC);
            }
        } else {
            lv_count = cv_md_table_count_recv.add_and_fetch(1);
            if (lv_count > cv_md_table_hi_recv) {
                cv_md_table_hi_recv = lv_count;
                gv_ms_cap.iv_md_table_hi_recv = lv_count;
                if ((cv_md_table_hi_recv % LOG_HIGH) == 0)
                    log_high(pp_where,
                             cv_md_table_hi_recv,
                             "recv",
                             SB_LOG_AGGR_CAP_MD_RECV_HI_CAP_INC);
            }
        }
        *ppp_md = lp_md;
        if (gv_ms_trace_md)
            trace_printf("msg get_md msgid=%d, stream=%p, dir-send=%d, where=%s, MD-inuse-count=%d\n",
                         static_cast<int>(lv_msgid), pfp(pp_stream),
                         pv_dir_send, pp_where,
                         cv_md_table_count_total.read_val());
        if (pp_fserr != NULL)
            *pp_fserr = lv_fserr;
        return static_cast<int>(lv_msgid);
    }
    *ppp_md = NULL;
    if (gv_ms_trace_md)
        trace_printf("msg get_md msgid=-1, fserr=%d, where=%s\n",
                     lv_fserr, pp_where);
    if (pp_fserr != NULL)
        *pp_fserr = lv_fserr;
    return -1; // no md available
}

int SB_Trans::Msg_Mgr::get_md_count_recv() {
    return cv_md_table_count_recv.read_val();
}

int SB_Trans::Msg_Mgr::get_md_count_send() {
    return cv_md_table_count_send.read_val() - 1; // sub 1 for rsvd-md
}

int SB_Trans::Msg_Mgr::get_md_count_total() {
    return cv_md_table_count_total.read_val();
}

int SB_Trans::Msg_Mgr::get_md_hi_recv() {
    return cv_md_table_hi_recv;
}

int SB_Trans::Msg_Mgr::get_md_hi_send() {
    return cv_md_table_hi_send;
}

int SB_Trans::Msg_Mgr::get_md_hi_total() {
    return cv_md_table_hi_total;
}

int SB_Trans::Msg_Mgr::get_md_max_recv() {
    return cv_md_table_max_recv;
}

int SB_Trans::Msg_Mgr::get_md_max_send() {
    return cv_md_table_max_send - 1;  // sub 1 for rsvd-md
}

int SB_Trans::Msg_Mgr::get_md_max_total() {
    size_t lv_cap;

    lv_cap = cv_md_table.get_cap();
    return static_cast<int>(lv_cap);
}

//
// Purpose: init table
//
int SB_Trans::Msg_Mgr::init() {
    int         lv_max_mds;
    size_t      lv_msgid;
    MS_Md_Type *lp_md;

    lv_msgid = cv_md_table.alloc_entry(); // forget about this one
    // but initialize for md-scanner
    lp_md = cv_md_table.get_entry(lv_msgid);
    lp_md->ip_stream = NULL;
    lp_md->ip_where = "rsvd";
    lp_md->iv_md_state = MD_STATE_ZERO_MD;
    lp_md->iv_ss.iv_slot_hdr = 0;
    lp_md->iv_ss.iv_slot_ctrl = 0;
    lp_md->iv_ss.iv_slot_data = 0;
    lv_max_mds = 0;
    ms_getenv_int(gp_ms_env_max_cap_mds, &lv_max_mds);
    if (lv_max_mds > 0)
        cv_md_table.set_cap_max(lv_max_mds);
    return 1;
}

//
// Purpose: log new high-mark
//
void SB_Trans::Msg_Mgr::log_high(const char *pp_where,
                                 int         pv_high,
                                 const char *pp_str,
                                 int         pv_cap_type) {
    sb_log_aggr_cap_log(pp_where, pv_cap_type, pv_high);
    if (gv_ms_trace || gv_ms_trace_md)
        trace_where_printf(pp_where,
                           "MD-%s-hi increased hi=%d\n", pp_str, pv_high);
}

//
// Purpose: map msgid to msg
//
MS_Md_Type *SB_Trans::Msg_Mgr::map_to_md(int pv_msgid, const char *pp_where) {
    MS_Md_Type *lp_md;
    size_t      lv_cap;

    lv_cap = cv_md_table.get_cap();
    if ((pv_msgid > 0) && (static_cast<size_t>(pv_msgid) < lv_cap)) {
        lp_md = cv_md_table.get_entry(pv_msgid);
        if (lp_md->iv_inuse) {
            if (pp_where != NULL)
                lp_md->ip_where = pp_where;
            return lp_md;
        }
    }
    return NULL;
}

//
// Purpose: print msg
//
void SB_Trans::Msg_Mgr::print_md(MS_Md_Type *pp_md) {
    trace_printf("msg PRINT...\n");
    if (pp_md == NULL)
        trace_printf("  is null\n");
    else if (!pp_md->iv_inuse)
        trace_printf("  is not in use\n");
    else {
        trace_printf("  msg.op=%d, state=%d\n",
                     pp_md->iv_op, pp_md->iv_md_state);
        trace_printf("  msg.csize=%d, dsize=%d, source=%d, tag=%d\n",
                     pp_md->out.iv_recv_ctrl_size,
                     pp_md->out.iv_recv_data_size,
                     pp_md->out.iv_recv_mpi_source_rank,
                     pp_md->out.iv_recv_mpi_tag);
        trace_printf("  msg.repc=%p, repmax=%d, repc=%p, repmax=%d\n",
                     pfp(pp_md->out.ip_reply_ctrl),
                     pp_md->out.iv_reply_ctrl_max,
                     pp_md->out.ip_reply_data, pp_md->out.iv_reply_data_max);
    }
}

//
// Purpose: put a md
//
void SB_Trans::Msg_Mgr::put_md(int pv_msgid, const char *pp_why) {
    MS_Md_Type   *lp_md;
    Trans_Stream *lp_stream;
    size_t        lv_cap;
    bool          lv_dir_send;
    int           lv_inuse_count;

    lv_cap = cv_md_table.get_cap();
    if ((pv_msgid > 0) && (static_cast<size_t>(pv_msgid) < lv_cap)) {
        lp_md = cv_md_table.get_entry(pv_msgid);
        if (lp_md->iv_inuse) {
            if (lp_md->ip_stream != NULL) {
                lp_stream = static_cast<Trans_Stream *>(lp_md->ip_stream);
                lp_stream->md_ref_dec();
            }
            lv_dir_send = lp_md->iv_dir_send;
            if (lv_dir_send)
                cv_md_table_count_send.sub_val(1);
            else
                cv_md_table_count_recv.sub_val(1);
            lv_inuse_count = cv_md_table_count_total.sub_and_fetch(1);
            lp_md->iv_inuse = false;
            cv_md_table.free_entry(pv_msgid);
            if (gv_ms_trace_md)
                trace_printf("msg put_md msgid=%d, dir-send=%d %s, MD-inuse-count=%d\n",
                             pv_msgid, lv_dir_send, pp_why, lv_inuse_count);
        } else
            SB_util_abort("!lp_md->iv_inuse"); // sw fault
    } else
        SB_util_abort("invalid pv_msgid"); // sw fault
}

//
// Purpose: put a md (linker check)
//
void SB_Trans::Msg_Mgr::put_md_link(MS_Md_Type *pp_md, const char *pp_why) {
    int lv_msgid;
    int lv_status;

    lv_status = pp_md->iv_sl.lock();
    SB_util_assert_ieq(lv_status, 0);
    if (pp_md->iv_send_done) {
        lv_status = pp_md->iv_sl.unlock();
        SB_util_assert_ieq(lv_status, 0);
        put_md(pp_md->iv_msgid, pp_why);
    } else {
        lv_msgid = pp_md->iv_msgid;
        pp_md->iv_free_md = true;
        lv_status = pp_md->iv_sl.unlock();
        SB_util_assert_ieq(lv_status, 0);
        if (gv_ms_trace_md)
            trace_printf("msg put_md_link NOT-put(send-not-done) msgid=%d, %s\n",
                         lv_msgid, pp_why);
    }
}

//
// Purpose: reset md
//
void SB_Trans::Msg_Mgr::reset_md(MS_Md_Type      *pp_md,
                                 void            *pp_stream,
                                 SB_Ms_Event_Mgr *pp_mgr,
                                 const char      *pp_where) {
    Trans_Stream *lp_stream;

#ifdef DEBUG
    // set invalid fields
    pp_md->iv_md_state = -1;
    memset(&pp_md->out, -1, sizeof(pp_md->out));
    memset(&pp_md->iv_ss, -1, sizeof(pp_md->iv_ss));
#endif
    if (pp_stream != NULL) {
        lp_stream = static_cast<Trans_Stream *>(pp_stream);
        lp_stream->md_ref_inc();
    }
    pp_md->ip_stream = pp_stream;
    pp_md->ip_mgr = pp_mgr;
    pp_md->iv_abandon_mutex.setname("mutex-md.iv_abandon_mutex");
    pp_md->iv_cv.reset_flag();
    pp_md->iv_cv.setname("cv-md.iv_cv");
    pp_md->iv_cv2.reset_flag();
    pp_md->iv_cv2.setname("cv-md.iv_cv2");
    pp_md->iv_sl.setname("sl-md.iv_sl");
    pp_md->iv_link.iv_qid = QID_NONE;
    pp_md->iv_op = static_cast<MS_Md_Op_Type>(0);
    pp_md->iv_break_done = false;
    pp_md->iv_reply_done = false;
    pp_md->iv_reply_done_temp = 0;
    pp_md->iv_send_done = false;
    pp_md->iv_abandoned = false;
    pp_md->iv_self = false;
    pp_md->iv_inline = false;
    pp_md->iv_free_md = false;
    pp_md->out.iv_fserr = XZFIL_ERR_OK;
    pp_md->out.iv_ldone = false;
    pp_md->out.iv_mon_msg = false;
    pp_md->out.iv_opts = 0;
    pp_md->out.ip_recv_ctrl = NULL;
    pp_md->out.ip_recv_data = NULL;
    pp_md->out.iv_reply = true;
    pp_md->out.iv_requeue = false;
    pp_md->ip_where = pp_where;
    if (gv_ms_msg_timestamp) {
        pp_md->iv_ts_msg_cli_break.tv_sec = 0;
        pp_md->iv_ts_msg_cli_break.tv_usec = 0;
        pp_md->iv_ts_msg_cli_link.tv_sec = 0;
        pp_md->iv_ts_msg_cli_link.tv_usec = 0;
        pp_md->iv_ts_msg_cli_rcvd.tv_sec = 0;
        pp_md->iv_ts_msg_cli_rcvd.tv_usec = 0;
        pp_md->iv_ts_msg_srv_listen.tv_sec = 0;
        pp_md->iv_ts_msg_srv_listen.tv_usec = 0;
        pp_md->iv_ts_msg_srv_rcvd.tv_sec = 0;
        pp_md->iv_ts_msg_srv_rcvd.tv_usec = 0;
        pp_md->iv_ts_msg_srv_reply.tv_sec = 0;
        pp_md->iv_ts_msg_srv_reply.tv_usec = 0;
    }
}

void SB_Trans::Msg_Mgr::set_md_max_recv(int pv_count) {
    cv_md_table_max_recv = pv_count;

    if (gv_ms_trace_md)
        trace_printf("msg set_md_max_recv count=%d\n", pv_count);
}

void SB_Trans::Msg_Mgr::set_md_max_send(int pv_count) {
    cv_md_table_max_send = pv_count + 1;  // add 1 for rsvd-md

    if (gv_ms_trace_md)
        trace_printf("msg set_md_max_send count=%d\n", pv_count);
}

//
// Purpose: set stream
//
void SB_Trans::Msg_Mgr::set_stream(MS_Md_Type *pp_md,
                                   void       *pp_stream,
                                   const char *pp_where) {
    SB_Trans::Trans_Stream *lp_stream;

    lp_stream = static_cast<SB_Trans::Trans_Stream *>(pp_stream);
    pp_md->ip_stream = lp_stream;
    if (pp_where != NULL)
        pp_md->ip_where = pp_where;
    if (lp_stream != NULL)
        lp_stream->md_ref_inc();
}

//
// Purpose: check empty (for testing)
//
void SB_Trans::Msg_Mgr::test_check_empty() {
    size_t lv_cap;
    size_t lv_msgid;

    lv_cap = cv_md_table.get_cap();
    for (lv_msgid = 1; lv_msgid < lv_cap; lv_msgid++)
        SB_util_assert_if(cv_md_table.get_entry(lv_msgid)->iv_inuse);
}

//
// Purpose: set md count (for testing)
//
void SB_Trans::Msg_Mgr::test_set_md_count(int pv_count) {
    MS_Md_Type *lp_md;
    size_t      lv_cap;
    int         lv_count = 0;
    size_t      lv_msgid;

    cv_md_table.lock();
    lv_cap = cv_md_table.get_cap();
    for (lv_msgid = 1; lv_msgid < lv_cap; lv_msgid++) {
        lp_md = cv_md_table.get_entry_lock(lv_msgid, false);
        if (!lp_md->iv_inuse) {
            lv_count++;
            if (lv_count > pv_count)
                lp_md->iv_inuse = true;
        }
    }
    cv_md_table.unlock();
}

int SB_Trans::Msg_Mgr::trace_inuse_md_count() {
    MS_Md_Type *lp_md;
    size_t      lv_cap;
    int         lv_count;
    size_t      lv_msgid;

    lv_count = 0;
    cv_md_table.lock();
    lv_cap = cv_md_table.get_cap();
    for (lv_msgid = 1; lv_msgid < lv_cap; lv_msgid++) {
        lp_md = cv_md_table.get_entry_lock(lv_msgid, false);
        if (lp_md->iv_inuse)
            lv_count++;
    }
    cv_md_table.unlock();
    return lv_count;
}

void SB_Trans::Msg_Mgr::trace_inuse_mds() {
    MS_Md_Type *lp_md;
    int         lv_cap;
    int         lv_msgid;

    cv_md_table.lock();
    lv_cap = static_cast<int>(cv_md_table.get_cap());
    trace_printf("md statistics count=%d, max=%d, hi=%d\n",
                 cv_md_table_count_total.read_val(),
                 lv_cap,
                 cv_md_table_hi_total);
    for (lv_msgid = 1; lv_msgid < lv_cap; lv_msgid++) {
        lp_md = cv_md_table.get_entry_lock(lv_msgid, false);
        if (lp_md->iv_inuse) {
            trace_printf("inuse-md msgid=%d\n", lv_msgid);
        }
    }
    cv_md_table.unlock();
}

