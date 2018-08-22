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

#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/fserr.h"
#include "seabed/pevents.h"
#include "seabed/trace.h"

#include "compq.h"
#include "labelmapsx.h"
#ifndef SQ_PHANDLE_VERIFIER
#include "llmap.h"
#endif
#include "msic.h"
#include "msicctr.h"
#include "mstrace.h"
#include "msx.h"
#ifdef SQ_PHANDLE_VERIFIER
#include "npvmap.h"
#endif
#include "threadtlsx.h"
#include "transport.h"

typedef struct NidPid_Type {
    union {
        struct {
            int iv_nid;
            int iv_pid;
        } i;
        SB_Int64_Type iv_nidpid;
    } u;
} NidPid_Type;

typedef struct NPS_Node {
    SB_LLML_Type            iv_link;
    SB_Trans::Trans_Stream *ip_stream;
} NPS_Node;

typedef void (*Proc_Fsreq)(MS_Md_Type *);
typedef union {
    Proc_Fsreq    ifsreq;
    void         *ipfsreq;
} Proc_Fsreq_Type;

typedef struct RM_Node {
    SB_ML_Type iv_link;
    int        iv_msgid;
} RM_Node;

// forwards
static void thread_key_dtor(void *pp_buf);

#ifdef SQ_PHANDLE_VERIFIER
static SB_Ts_NPVmap        gv_sb_stream_nidpid_map("map-stream-npv");
#else
static SB_Ts_LLmap         gv_sb_stream_nidpid_map("map-stream-nidpid");
#endif
static int                 gv_sb_stream_tls_inx =
                             SB_create_tls_key(thread_key_dtor,
                                               "stream-marker");

#ifdef SQ_PHANDLE_VERIFIER
static NPS_Node *new_NPS_Node(SB_NPV_Type             pv_npv,
#else
static NPS_Node *new_NPS_Node(SB_Int64_Type           pv_nidpid,
#endif
                              SB_Trans::Trans_Stream *pp_stream) {
    NPS_Node *lp_node = new NPS_Node();
#ifdef SQ_PHANDLE_VERIFIER
    lp_node->iv_link.iv_id.npv = pv_npv;
#else
    lp_node->iv_link.iv_id.ll = pv_nidpid;
#endif
    lp_node->ip_stream = pp_stream;
    return lp_node;
}

static RM_Node *new_RM_Node(int pv_reqid, int pv_msgid) {
    RM_Node *lp_node = new RM_Node();
    lp_node->iv_link.iv_id.l = pv_reqid;
    lp_node->iv_msgid = pv_msgid;
    return lp_node;
}

static void thread_key_dtor(void *pp_buf) {
    if (pp_buf != NULL) {
        char *lp_buf = static_cast<char *>(pp_buf);
        delete [] lp_buf;
    }
}

SB_Trans::Trans_Stream *SB_Trans::Trans_Stream::cp_mon_stream = NULL;
SB_Thread::ECM          SB_Trans::Trans_Stream::cv_cap_mutex("mutex-Trans_Stream::cv_cap_mutex");
SB_Thread::ECM          SB_Trans::Trans_Stream::cv_close_mutex("mutex-Trans_Stream::cv_close_mutex");
SB_D_Queue              SB_Trans::Trans_Stream::cv_close_q(QID_STREAM_CLOSE, "q-stream-close");
SB_Thread::ECM          SB_Trans::Trans_Stream::cv_del_mutex("mutex-Trans_Stream::cv_del_mutex");
SB_D_Queue              SB_Trans::Trans_Stream::cv_del_q(QID_STREAM_DEL, "q-stream-del");
bool                    SB_Trans::Trans_Stream::cv_shutdown = false;
SB_Atomic_Int           SB_Trans::Trans_Stream::cv_stream_acc_count;
SB_Atomic_Int           SB_Trans::Trans_Stream::cv_stream_acc_hi_count;
SB_Atomic_Int           SB_Trans::Trans_Stream::cv_stream_con_count;
SB_Atomic_Int           SB_Trans::Trans_Stream::cv_stream_con_hi_count;
SB_Atomic_Int           SB_Trans::Trans_Stream::cv_stream_total_count;
SB_Atomic_Int           SB_Trans::Trans_Stream::cv_stream_total_hi_count;

SB_Trans::Stream_Base::Stream_Base() {
}

SB_Trans::Stream_Base::~Stream_Base() {
}

SB_Trans::Trans_Stream::Trans_Stream(int pv_stream_type)
: iv_aecid_trans_stream(SB_ECID_STREAM_TRANS),
  ip_event_mgr(NULL),
  ip_ms_lim_q(NULL),
  ip_ms_recv_q(NULL),
  iv_generation(0),
  iv_ic(false),
  iv_ms_abandon_callback(NULL),
  iv_ms_comp_callback(NULL),
  iv_ms_lim_callback(NULL),
  iv_ms_oc_callback(NULL),
  iv_msgid_reqid_map("map-trans-stream-msgid-reqid"),
  iv_open_nid(-1),
  iv_open_pid(-1),
#ifdef SQ_PHANDLE_VERIFIER
  iv_open_verif(-1),
#endif
  iv_opened_nid(-1),
  iv_opened_pid(-1),
#ifdef SQ_PHANDLE_VERIFIER
  iv_opened_verif(-1),
#endif
  iv_opened_type(-1),
  iv_post_mon_messages(true),
  iv_rep_map(QID_REP_MAP, "map-rep"),
  iv_reply_piggyback_q(QID_REPLY_PB, "q-reply-pb-md"),
  iv_req_map(QID_REQ_MAP, "map-req"),
  iv_stream_type(pv_stream_type) {
    init("<none>", "<none>", "<none>");
}

SB_Trans::Trans_Stream::Trans_Stream(int                   pv_stream_type,
                                     const char           *pp_name,
                                     const char           *pp_pname,
                                     const char           *pp_prog,
                                     bool                  pv_ic,
                                     SB_Comp_Cb_Type       pv_ms_comp_callback,
                                     SB_Ab_Comp_Cb_Type    pv_ms_abandon_callback,
                                     SB_ILOC_Comp_Cb_Type  pv_ms_oc_callback,
                                     SB_Lim_Cb_Type        pv_ms_lim_callback,
                                     SB_Recv_Queue        *pp_ms_recv_q,
                                     SB_Recv_Queue        *pp_ms_lim_q,
                                     SB_Ms_Tl_Event_Mgr   *pp_event_mgr,
                                     int                   pv_open_nid,
                                     int                   pv_open_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                     SB_Verif_Type         pv_open_verif,
#endif
                                     int                   pv_opened_nid,
                                     int                   pv_opened_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                     SB_Verif_Type         pv_opened_verif,
#endif
                                     int                   pv_opened_type)
: iv_aecid_trans_stream(SB_ECID_STREAM_TRANS),
  ip_event_mgr(pp_event_mgr),
  ip_ms_lim_q(pp_ms_lim_q),
  ip_ms_recv_q(pp_ms_recv_q),
  iv_generation(0),
  iv_ic(pv_ic),
  iv_ms_abandon_callback(pv_ms_abandon_callback),
  iv_ms_comp_callback(pv_ms_comp_callback),
  iv_ms_lim_callback(pv_ms_lim_callback),
  iv_ms_oc_callback(pv_ms_oc_callback),
  iv_msgid_reqid_map("map-trans-stream-msgid-reqid"),
  iv_open_nid(pv_open_nid),
  iv_open_pid(pv_open_pid),
#ifdef SQ_PHANDLE_VERIFIER
  iv_open_verif(pv_open_verif),
#endif
  iv_opened_nid(pv_opened_nid),
  iv_opened_pid(pv_opened_pid),
#ifdef SQ_PHANDLE_VERIFIER
  iv_opened_verif(pv_opened_verif),
#else
#endif
  iv_opened_type(pv_opened_type),
  iv_post_mon_messages(true),
  iv_rep_map(QID_REP_MAP, "map-rep"),
  iv_reply_piggyback_q(QID_REPLY_PB, "q-reply-pb-md"),
  iv_req_map(QID_REQ_MAP, "map-req"),
  iv_stream_type(pv_stream_type) {
    init(pp_name, pp_pname, pp_prog);
}

SB_Trans::Trans_Stream::~Trans_Stream() {
    const char   *WHERE = "Trans_Stream::~Trans_Stream";
    bool          lv_done;
    int           lv_status;
    SB_Imap_Enum *lp_enum;
    RM_Node      *lp_node;
    void         *lp_nodev;

    if (gv_ms_trace)
        trace_where_printf(WHERE, "removing from close-q/del-q stream=%p %s\n",
                           pfp(this), ia_stream_name);

    lv_status = cv_close_mutex.lock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
    cv_close_q.remove_list(&iv_close_link);
    lv_status = cv_close_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0); // sw fault

    lv_status = cv_del_mutex.lock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
    cv_del_q.remove_list(&iv_del_link);
    lv_status = cv_del_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0); // sw fault

    lv_done = false;
    do {
        lp_enum = iv_msgid_reqid_map.keys();
        if (lp_enum->more()) {
            lp_nodev = iv_msgid_reqid_map.remove(lp_enum->next()->iv_id.i);
            lp_node = static_cast<RM_Node *>(lp_nodev);
            delete lp_node;
        } else
            lv_done = true;
        delete lp_enum;
    } while (!lv_done);
    // set the reference count to a negative value
    iv_ref_count.set_val(-0305);
}

//
// Purpose: add msgid to reqid map
//
void SB_Trans::Trans_Stream::add_msgid_to_reqid_map(int pv_reqid,
                                                    int pv_msgid) {
    RM_Node *lp_node = new_RM_Node(pv_reqid, pv_msgid);
    iv_msgid_reqid_map.put(&lp_node->iv_link);
}

//
// Purpose: add md to reply-piggyback
//
void SB_Trans::Trans_Stream::add_reply_piggyback(MS_Md_Type *pp_md) {
    const char *WHERE = "Trans_Stream::add_reply_piggyback";

    iv_reply_piggyback_q.add(&pp_md->iv_link);
    if (gv_ms_trace)
        trace_where_printf(WHERE, "stream=%p %s, adding md (msgid=%d, md=%p), count=%d\n",
                           pfp(this), ia_stream_name,
                           pp_md->iv_link.iv_id.i, pfp(pp_md),
                           iv_reply_piggyback_q.size());
}

//
// Purpose: add accept stream count
// (static)
//
void SB_Trans::Trans_Stream::add_stream_acc_count(int pv_val) {
    const char *WHERE = "Trans_Stream::add_stream_acc_count";
    int         lv_val;

    lv_val = cv_stream_total_count.add_and_fetch(pv_val);
    if (lv_val > cv_stream_total_hi_count.read_val())
        cv_stream_total_hi_count.set_val(lv_val);
    lv_val = cv_stream_acc_count.add_and_fetch(pv_val);
    if (lv_val > cv_stream_acc_hi_count.read_val())
        cv_stream_acc_hi_count.set_val(lv_val);
    if (gv_ms_trace)
        trace_where_printf(WHERE, "stream acc-count=%d, total-count=%d\n",
                           get_stream_acc_count(),
                           get_stream_total_count());
}

//
// Purpose: add connect stream count
// (static)
//
void SB_Trans::Trans_Stream::add_stream_con_count(int pv_val) {
    const char *WHERE = "Trans_Stream::add_stream_con_count";
    int         lv_val;

    lv_val = cv_stream_total_count.add_and_fetch(pv_val);
    if (lv_val > cv_stream_total_hi_count.read_val())
        cv_stream_total_hi_count.set_val(lv_val);
    lv_val = cv_stream_con_count.add_and_fetch(pv_val);
    if (lv_val > cv_stream_con_hi_count.read_val())
        cv_stream_con_hi_count.set_val(lv_val);
    if (gv_ms_trace)
        trace_where_printf(WHERE, "stream con-count=%d, total-count=%d\n",
                           get_stream_con_count(),
                           get_stream_total_count());
    // monitor stream is a connect-stream, but not included in con-count
    SB_util_assert_ige(cv_stream_con_count.read_val(), -1);
}

//
// Purpose: add stream for closing
//
void SB_Trans::Trans_Stream::add_stream_for_close() {
    const char *WHERE = "Trans_Stream::add_stream_for_close";
    int         lv_status;

    if (gv_ms_trace)
        trace_where_printf(WHERE, "adding to close-q stream=%p %s\n",
                           pfp(this), ia_stream_name);
    lv_status = cv_close_mutex.lock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
    cv_close_q.remove_list(&iv_close_link);
    cv_close_q.add_at_front(&iv_close_link);
    lv_status = cv_close_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
}

//
// Purpose: check recv checksums
//
void SB_Trans::Trans_Stream::checksum_recv_check(Rd_Type     *pp_rd,
                                                 MS_PMH_Type *pp_hdr,
                                                 bool         pv_client) {
    char *lp_ctrl;
    char *lp_data;
    int   lv_chk;
    int   lv_ctrl_len;
    int   lv_data_len;
    int   lv_inx;

    lv_chk = -1;
    pv_client = pv_client; // touch
    lp_ctrl = pp_rd->ip_ctrl;
    lv_ctrl_len = pp_rd->iv_ctrl_len;
    for (lv_inx = 0; lv_inx < lv_ctrl_len; lv_inx++)
        lv_chk += lp_ctrl[lv_inx];
    SB_util_assert_ieq(lv_chk, pp_hdr->iv_chk_ctrl);
    lv_chk = -1;
    lp_data = pp_rd->ip_data;
    lv_data_len = pp_rd->iv_data_len;
    for (lv_inx = 0; lv_inx < lv_data_len; lv_inx++)
        lv_chk += lp_data[lv_inx];
    SB_util_assert_ieq(lv_chk, pp_hdr->iv_chk_data);
}

//
// Purpose: check send checksums
//
void SB_Trans::Trans_Stream::checksum_send_check(MS_Md_Type *pp_md,
                                                 bool        pv_reply) {
    char       *lp_ctrl;
    char       *lp_data;
    int        *lp_hdr;
    char       *lp_rep_beg;
    char       *lp_rep_end;
    MS_SS_Type *lp_s;
    int         lv_chk;
    int         lv_inx;
    int         lv_limit_hdr;
    int         lv_limit_ctrl;
    int         lv_limit_data;

    lp_s = &pp_md->iv_ss;
    lv_chk = -1;
    lp_hdr = reinterpret_cast<int *>(&lp_s->iv_hdr);
    lv_limit_hdr = static_cast<int>(offsetof(MS_PMH_Type, iv_chk_hdr))/4;
    for (lv_inx = 0; lv_inx < lv_limit_hdr; lv_inx++)
        lv_chk += lp_hdr[lv_inx];
    SB_util_assert_ieq(lv_chk, lp_s->iv_hdr.iv_chk_hdr);

    lp_ctrl = reinterpret_cast<char *>(lp_s->ip_req_ctrl);
    lv_limit_ctrl = lp_s->iv_hdr.iv_csize;
    lp_rep_beg = reinterpret_cast<char *>(pp_md->out.ip_reply_ctrl);
    lp_rep_end = lp_rep_beg + lp_s->iv_req_ctrl_size;
    if (pv_reply && (lp_ctrl != NULL)) {
        if (((lp_rep_beg >= lp_ctrl) && // reply overlaps req
            (lp_rep_beg < lp_ctrl + lv_limit_ctrl)) ||
           ((lp_rep_end >= lp_ctrl) && // reply overlaps req
            (lp_rep_end < lp_ctrl + lv_limit_ctrl)))
            lp_ctrl = NULL;
    }

    lp_data = lp_s->ip_req_data;
    lv_limit_data = lp_s->iv_hdr.iv_dsize;
    lp_rep_beg = static_cast<char *>(pp_md->out.ip_reply_data);
    lp_rep_end = lp_rep_beg + pp_md->iv_ss.iv_req_data_size;
    if (pv_reply && (lp_data != NULL)) {
        if (((lp_rep_beg >= lp_data) && // reply overlaps req
             (lp_rep_beg < lp_data + lv_limit_data)) ||
            ((lp_rep_end >= lp_data) && // reply overlaps req
             (lp_rep_end < lp_data + lv_limit_data)))
            lp_data = NULL;
    }

    // check here in case of fault - stack trace will show 'send' problem
    if (lp_ctrl != NULL) {
        lv_chk = -1;
        for (lv_inx = 0; lv_inx < lv_limit_ctrl; lv_inx++)
            lv_chk += lp_ctrl[lv_inx];
        SB_util_assert_ieq(lv_chk, lp_s->iv_hdr.iv_chk_ctrl);
    }

    if (lp_data != NULL) {
        lv_chk = -1;
        for (lv_inx = 0; lv_inx < lv_limit_data; lv_inx++)
            lv_chk += lp_data[lv_inx];
        SB_util_assert_ieq(lv_chk, lp_s->iv_hdr.iv_chk_data);
    }
}

//
// Purpose: close nidpid streams
// (static)
//
void SB_Trans::Trans_Stream::close_nidpid_streams(bool pv_lock) {
    const char  *WHERE = "Trans_Stream::close_nidpid_streams";
#ifdef SQ_PHANDLE_VERIFIER
    SB_NPV_Type *lp_npv;
#else
    NidPid_Type *lp_nidpid;
#endif
    int          lv_status;

    while (gv_sb_stream_nidpid_map.size()) {
        // need to do this because enum is not safe from remove
        if (pv_lock)
            map_nidpid_lock();
        else {
            do {
                lv_status = map_nidpid_trylock();
                if (lv_status == EBUSY)
                    usleep(100);
            } while (lv_status);
            SB_util_assert_ieq(lv_status, 0); // sw fault
        }

#ifdef SQ_PHANDLE_VERIFIER
        SB_NPVmap_Enum *lp_enum = gv_sb_stream_nidpid_map.keys();
#else
        SB_LLmap_Enum *lp_enum = gv_sb_stream_nidpid_map.keys();
#endif
        enum { MAX_STREAMS = 10 };
#ifdef SQ_PHANDLE_VERIFIER
        SB_NPV_Type   la_npv[MAX_STREAMS];
#else
        SB_Int64_Type la_nidpids[MAX_STREAMS];
#endif
        int lv_max;
        for (lv_max = 0;
             lp_enum->more() && (lv_max < MAX_STREAMS);
             lv_max++) {
#ifdef SQ_PHANDLE_VERIFIER
            la_npv[lv_max] = lp_enum->next()->iv_id.npv;
#else
            la_nidpids[lv_max] = lp_enum->next()->iv_id.ll;
#endif
        }
        map_nidpid_unlock();
        for (int lv_inx = 0; lv_inx < lv_max; lv_inx++) {
            NPS_Node *lp_node =
#ifdef SQ_PHANDLE_VERIFIER
              static_cast<NPS_Node *>(gv_sb_stream_nidpid_map.remove_lock(la_npv[lv_inx], true));
#else
              static_cast<NPS_Node *>(gv_sb_stream_nidpid_map.remove_lock(la_nidpids[lv_inx], true));
#endif
            if (lp_node != NULL) {
#ifdef SQ_PHANDLE_VERIFIER
                lp_npv = reinterpret_cast<SB_NPV_Type *>(&lp_node->iv_link.iv_id);
#else
                lp_nidpid = reinterpret_cast<NidPid_Type *>(&lp_node->iv_link.iv_id);
#endif
                if (gv_ms_trace || (gv_ms_trace_xx & MS_TRACE_XX_NIDPID))
#ifdef SQ_PHANDLE_VERIFIER
                    trace_where_printf(WHERE, "removed/closing p-id=%d/%d/" PFVY ", stream=%p %s\n",
#else
                    trace_where_printf(WHERE, "removed/closing p-id=%d/%d, stream=%p %s\n",
#endif
#ifdef SQ_PHANDLE_VERIFIER
                                       lp_npv->iv_nid,
                                       lp_npv->iv_pid,
                                       lp_npv->iv_verif,
#else
                                       lp_nidpid->u.i.iv_nid,
                                       lp_nidpid->u.i.iv_pid,
#endif
                                       pfp(lp_node->ip_stream),
                                       lp_node->ip_stream->ia_stream_name);
                close_stream(WHERE, lp_node->ip_stream, true, false, false);
                delete lp_node;
            }
        }
        delete lp_enum;
    }
}

//
// Purpose: close stream with optional lock
// (static)
//
void SB_Trans::Trans_Stream::close_stream(const char   *pp_where,
                                          Trans_Stream *pp_stream,
                                          bool          pv_local,
                                          bool          pv_lock,
                                          bool          pv_sem) {
    const char *WHERE = "Trans_Stream::close_stream";
    int         lv_md_ref_count;
    int         lv_ref_count;
    int         lv_status;

    if (gv_ms_trace)
        trace_where_printf(WHERE, "%s ENTER\n", pp_where);
    pp_stream->close_this(pv_local, pv_lock, pv_sem);
    lv_ref_count = pp_stream->ref_get();
    lv_md_ref_count = pp_stream->md_ref_get();
    if (lv_md_ref_count == 0) {
        if (lv_ref_count == 0)  {
            if (gv_ms_trace_ref)
                trace_where_printf(WHERE,
                                   "Deleting stream=%s, stream-ref=%d\n",
                                   pp_stream->get_name(),
                                   lv_ref_count);
            delete pp_stream;
        } else {
            if (gv_ms_trace_ref)
                trace_where_printf(WHERE,
                                   "Not deleting stream=%s, stream-ref=%d\n",
                                   pp_stream->get_name(),
                                   lv_ref_count);
        }
    } else {
        if (gv_ms_trace_ref)
            trace_where_printf(WHERE,
                               "Not deleting stream=%s, adding to del-q, md-ref=%d\n",
                               pp_stream->get_name(),
                               lv_md_ref_count);
        lv_status = cv_del_mutex.lock();
        SB_util_assert_ieq(lv_status, 0); // sw fault
        cv_del_q.remove_list(&pp_stream->iv_del_link);
        cv_del_q.add_at_front(&pp_stream->iv_del_link);
        lv_status = cv_del_mutex.unlock();
        SB_util_assert_ieq(lv_status, 0); // sw fault
    }
}

//
// Purpose: delete streams
//
void SB_Trans::Trans_Stream::delete_streams(bool pv_ref_zero) {
    const char   *WHERE = "Trans_Stream::delete_streams";
    char         *lp_dql;
    Trans_Stream *lp_stream;
    char         *lp_streamc;
    SB_D_Queue    lv_del_q(QID_STREAM_DEL_TEMP, "q-stream-del-temp");
    int           lv_off;
    int           lv_ref;
    int           lv_stream_ref;

    // offsetof the hard way
    lp_stream = NULL;
    lv_off = reinterpret_cast<char *>(&lp_stream->iv_del_link) -
             reinterpret_cast<char *>(lp_stream);
    while (!cv_del_q.empty()) {
        // lp_dql will contain address of iv_del_link, so
        // lp_stream will have to be calculated
        lp_dql = static_cast<char *>(cv_del_q.remove());
        lp_streamc = lp_dql - lv_off;
        lp_stream = reinterpret_cast<Trans_Stream *>(lp_streamc);
        // ref_get to trace reference count before deleting
        lv_ref = lp_stream->md_ref_get();
        lv_stream_ref = lp_stream->ref_get();
        if (pv_ref_zero) {
            if (lv_ref == 0) {
                if (lv_stream_ref == 0) {
                    if (gv_ms_trace_ref)
                        trace_where_printf(WHERE,
                                           "Deleting stream=%s, stream-ref=%d\n",
                                           lp_stream->get_name(),
                                           lv_stream_ref);
                    delete lp_stream;
                } else {
                    if (gv_ms_trace_ref)
                        trace_where_printf(WHERE,
                                           "Not deleting stream=%s, stream-ref=%d\n",
                                           lp_stream->get_name(),
                                           lv_stream_ref);
                    lv_del_q.add(&lp_stream->iv_del_link);
                }
            } else {
                if (gv_ms_trace_ref)
                    trace_where_printf(WHERE,
                                       "Not deleting stream=%s, adding to del-q, md-ref=%d\n",
                                       lp_stream->get_name(),
                                       lv_ref);
                lv_del_q.add(&lp_stream->iv_del_link);
            }
        } else {
            if (lv_stream_ref == 0) {
                if (gv_ms_trace_ref)
                    trace_where_printf(WHERE,
                                       "Deleting stream=%s, stream-ref=%d\n",
                                       lp_stream->get_name(),
                                       lv_stream_ref);
                delete lp_stream;
            } else {
                if (gv_ms_trace_ref)
                    trace_where_printf(WHERE,
                                       "Not deleting stream=%s, stream-ref=%d\n",
                                       lp_stream->get_name(),
                                       lv_stream_ref);
                lv_del_q.add(&lp_stream->iv_del_link);
            }
        }
    }
    if (pv_ref_zero) {
        while (!lv_del_q.empty()) {
            lp_dql = static_cast<char *>(lv_del_q.remove());
            lp_streamc = lp_dql - lv_off;
            lp_stream = reinterpret_cast<Trans_Stream *>(lp_streamc);
            cv_del_q.add(&lp_stream->iv_del_link);
        }
    }
}

//
// Purpose: finish client abandon
//
void SB_Trans::Trans_Stream::finish_abandon(MS_Md_Type *pp_md) {
    const char *WHERE = "Trans_Stream::finish_abandon";
    int         lv_msgid;

    if (gv_ms_trace || gv_ms_trace_abandon)
        trace_where_printf(WHERE,
                           "stream=%p %s, processing abandon md (msgid=%d, md=%p, pri=%d)\n",
                           pfp(this), ia_stream_name,
                           pp_md->iv_link.iv_id.i, pfp(pp_md),
                           pp_md->out.iv_pri);

    // remove (abandon) md from request list
    iv_req_map.remove(pp_md->iv_link.iv_id.i);

    // remove (original) md from request list and put
    lv_msgid = pp_md->out.iv_pri;
    if (iv_req_map.remove(lv_msgid) != NULL)
        Msg_Mgr::put_md(lv_msgid, "abandon, no server reply");

    // tell client it's done
    pp_md->iv_md_state = MD_STATE_ABANDON_FIN;
    // needs to be done last
    pp_md->iv_reply_done = true;
    pp_md->iv_cv.signal(true); // need lock
}

//
// Purpose: finish client close
//
void SB_Trans::Trans_Stream::finish_close(MS_Md_Type *pp_md) {
    const char *WHERE = "Trans_Stream::finish_close";

    if (gv_ms_trace)
        trace_where_printf(WHERE,
                           "stream=%p %s, processing close md (msgid=%d, md=%p), setting reply-done=1\n",
                           pfp(this), ia_stream_name,
                           pp_md->iv_link.iv_id.i, pfp(pp_md));

    // remove (close) md from request list
    iv_req_map.remove(pp_md->iv_link.iv_id.i);

    // tell client it's done
    pp_md->iv_md_state = MD_STATE_CLOSE_FIN;
    // needs to be done last
    pp_md->iv_reply_done = true;
    pp_md->iv_cv.signal(true); // need lock
}

//
// Purpose: finish client conn
//
void SB_Trans::Trans_Stream::finish_conn(MS_Md_Type *pp_md) {
    const char      *WHERE = "Trans_Stream::finish_conn";
    Ms_Sm_Conn_Type *lp_conn;

    if (gv_ms_trace)
        trace_where_printf(WHERE,
                           "stream=%p %s, processing conn md (msgid=%d, md=%p), setting reply-done=1\n",
                           pfp(this), ia_stream_name,
                           pp_md->iv_link.iv_id.i, pfp(pp_md));

    // remove (neg) md from request list
    iv_req_map.remove(pp_md->iv_link.iv_id.i);

    lp_conn = reinterpret_cast<Ms_Sm_Conn_Type *>(pp_md->out.ip_reply_ctrl);
    set_pname(lp_conn->ia_pname);
    set_prog(lp_conn->ia_prog);

    // tell client it's done
    pp_md->iv_md_state = MD_STATE_CONN_FIN;
    // needs to be done last
    pp_md->iv_reply_done = true;
    pp_md->iv_cv.signal(true); // need lock
}

//
// Purpose: finish client open
//
void SB_Trans::Trans_Stream::finish_open(MS_Md_Type *pp_md) {
    const char *WHERE = "Trans_Stream::finish_open";

    if (gv_ms_trace)
        trace_where_printf(WHERE,
                           "stream=%p %s, processing open md (msgid=%d, md=%p), setting reply-done=1\n",
                           pfp(this), ia_stream_name,
                           pp_md->iv_link.iv_id.i, pfp(pp_md));

    // remove (open) md from request list
    iv_req_map.remove(pp_md->iv_link.iv_id.i);

    // tell client it's done
    pp_md->iv_md_state = MD_STATE_OPEN_FIN;
    // needs to be done last
    pp_md->iv_reply_done = true;
    pp_md->iv_cv.signal(true); // need lock
}

//
// Purpose: finish client read
//
void SB_Trans::Trans_Stream::finish_recv(Rd_Type     *pp_rd,
                                         MS_PMH_Type *pp_hdr,
                                         bool         pv_client) {
    const char *WHERE = "Trans_Stream::finish_recv";

    if (pv_client) {
        //
        // client processing
        //
        if (gv_ms_assert_chk) // finish_recv (check server reply)
            checksum_recv_check(pp_rd, pp_hdr, pv_client);

        // need to copy info from rd to original md
        MS_Md_Type *lp_md = Msg_Mgr::map_to_md(pp_hdr->iv_reqid, WHERE);
        SB_util_assert_pne(lp_md, NULL); // sw fault
        if (gv_ms_trace &&
            ((pp_hdr->iv_type == MS_PMH_TYPE_REPLY) ||
             (pp_hdr->iv_type == MS_PMH_TYPE_REPLY_NACK)))
            trace_where_printf(WHERE,
                               "stream=%p %s, adding md (reqid=%d, msgid=%d, md=%p, fserr=%d) to client recv done, %s\n",
                               pfp(this), ia_stream_name,
                               pp_hdr->iv_reqid,
                               lp_md->iv_link.iv_id.i,
                               pfp(lp_md),
                               pp_hdr->iv_fserr,
                               lp_md->out.iv_opts & MS_OPTS_FSDONE ? "fs" : "ms");
        lp_md->out.iv_mon_msg = false;
        lp_md->out.iv_pri = pp_hdr->iv_pri;
        if (gv_ms_msg_timestamp) {
            gettimeofday(&lp_md->iv_ts_msg_cli_rcvd, NULL);
            lp_md->iv_ts_msg_srv_reply.tv_sec = pp_hdr->iv_ts1.iv_sec;
            lp_md->iv_ts_msg_srv_reply.tv_usec = pp_hdr->iv_ts1.iv_usec;
            lp_md->iv_ts_msg_srv_rcvd.tv_sec = pp_hdr->iv_ts2.iv_sec;
            lp_md->iv_ts_msg_srv_rcvd.tv_usec = pp_hdr->iv_ts2.iv_usec;
            lp_md->iv_ts_msg_srv_listen.tv_sec = pp_hdr->iv_ts3.iv_sec;
            lp_md->iv_ts_msg_srv_listen.tv_usec = pp_hdr->iv_ts3.iv_usec;
        }

        if (!(gv_ms_buf_options & MS_BUF_OPTION_COPY)) {
            pp_rd->ip_ctrl = NULL;
            pp_rd->ip_data = NULL;
        } else {
            // copy ctrl
            lp_md->out.iv_reply_ctrl_count = pp_rd->iv_ctrl_len;
            int lv_rc;
            recv_copy(WHERE,
                      &pp_rd->ip_ctrl,
                      pp_rd->iv_ctrl_len,
                      reinterpret_cast<char *>(lp_md->out.ip_reply_ctrl),
                      lp_md->out.iv_reply_ctrl_max,
                      &lv_rc,
                      true);

            // copy data
            lp_md->out.iv_reply_data_count = pp_rd->iv_data_len;
            recv_copy(WHERE,
                      &pp_rd->ip_data,
                      pp_rd->iv_data_len,
                      lp_md->out.ip_reply_data,
                      lp_md->out.iv_reply_data_max,
                      &lv_rc,
                      true);
        }

        switch (pp_hdr->iv_type) {
        case MS_PMH_TYPE_ABANDON_ACK:
            comp_sends(); // try to complete sends
            finish_abandon(lp_md);
            break;
        case MS_PMH_TYPE_CLOSE_ACK:
            comp_sends(); // try to complete sends
            finish_close(lp_md);
            break;
        case MS_PMH_TYPE_CONN_ACK:
            comp_sends(); // try to complete sends
            finish_conn(lp_md);
            break;
        case MS_PMH_TYPE_OPEN_ACK:
            comp_sends(); // try to complete sends
            finish_open(lp_md);
            break;
        case MS_PMH_TYPE_REPLY_NACK:
            finish_reply(lp_md,
                         XZFIL_ERR_NOLCB,
                         false,  // softerr!
                         true);
            break;
        default:
            finish_reply(lp_md,
                         pp_hdr->iv_fserr,
                         false,
                         true);
            break;
        }
    } else if (pp_hdr->iv_type == MS_PMH_TYPE_ABANDON) {
        //
        // server processing, but abandon
        //
        process_abandon(WHERE, pp_hdr->iv_pri, pp_hdr->iv_reqid);
    } else if (pp_hdr->iv_type == MS_PMH_TYPE_CLOSE) {
        //
        // server processing, but close
        //
        if (pp_hdr->iv_opts & MS_OPTS_MSIC) {
            if (gv_ms_trace)
                trace_where_printf(WHERE, "stream=%p %s, IC stream, interceptor close, reqid=%d\n",
                                   pfp(this), ia_stream_name,
                                   pp_hdr->iv_reqid);
        } else {
            MS_Md_Type *lp_md;
            Msg_Mgr::get_md(&lp_md, // finish_recv (close)
                            this,
                            NULL,
                            false,  // recv
                            NULL,   // fserr
                            "Trans_Stream::finish_recv (close)",
                            MD_STATE_RCVD_CLOSE);
            SB_util_assert_pne(lp_md, NULL); // TODO: can't get md
            add_msgid_to_reqid_map(pp_hdr->iv_reqid, lp_md->iv_link.iv_id.i);

            if (gv_ms_trace)
                trace_where_printf(WHERE,
                                   "stream=%p %s, adding (close-inline) md (reqid=%d, msgid=%d, md=%p) to server recv queue\n",
                                   pfp(this), ia_stream_name,
                                   pp_hdr->iv_reqid,
                                   lp_md->iv_link.iv_id.i, pfp(lp_md));
            set_basic_md(pp_rd, pp_hdr, lp_md, true);
            lp_md->out.iv_pri = -1;
            lp_md->out.iv_recv_mpi_source_rank = -1;
            lp_md->out.iv_recv_mpi_tag = -1;
            lp_md->out.iv_recv_ctrl_max = 0;
            lp_md->out.iv_recv_data_max = 0;
            iv_ms_oc_callback(lp_md, NULL);
            pp_rd->ip_ctrl = NULL;
        }

        //
        // Send close-ack
        //
        MS_Md_Type *lp_close_ack_md;
        Msg_Mgr::get_md(&lp_close_ack_md, // finish_recv (close-ack)
                        this,
                        NULL,
                        true,             // send
                        NULL,             // fserr
                        "Trans_Stream::finish_recv (close ack)",
                        MD_STATE_MSG_LOW_CLOSE_ACK);
        SB_util_assert_pne(lp_close_ack_md, NULL); // TODO: can't get md
        lp_close_ack_md->iv_inline = true;
        send_close_ack(lp_close_ack_md, pp_hdr->iv_reqid);
    } else if (pp_hdr->iv_type == MS_PMH_TYPE_OPEN) {
        //
        // server processing, but open
        //
        if (pp_hdr->iv_opts & MS_OPTS_MSIC) {
            if (gv_ms_trace)
                trace_where_printf(WHERE, "stream=%p %s, IC stream, interceptor open, reqid=%d\n",
                                   pfp(this), ia_stream_name,
                                   pp_hdr->iv_reqid);
        } else {
            MS_Md_Type *lp_md;
            Msg_Mgr::get_md(&lp_md, // finish_recv (open)
                            this,
                            NULL,
                            false,  // recv
                            NULL,   // fserr
                            "Trans_Stream::finish_recv (open)",
                            MD_STATE_RCVD_OPEN);
            SB_util_assert_pne(lp_md, NULL); // TODO: can't get md
            add_msgid_to_reqid_map(pp_hdr->iv_reqid, lp_md->iv_link.iv_id.i);

            if (gv_ms_trace)
                trace_where_printf(WHERE,
                                   "stream=%p %s, adding (open-inline) md (reqid=%d, msgid=%d, md=%p) to server recv queue\n",
                                   pfp(this), ia_stream_name,
                                   pp_hdr->iv_reqid,
                                   lp_md->iv_link.iv_id.i, pfp(lp_md));
            set_basic_md(pp_rd, pp_hdr, lp_md, true);
            lp_md->out.iv_pri = -1;
            lp_md->out.iv_recv_mpi_source_rank = -1;
            lp_md->out.iv_recv_mpi_tag = -1;
            lp_md->out.ip_recv_ctrl = pp_rd->ip_ctrl;
            lp_md->out.iv_recv_ctrl_size = pp_rd->iv_ctrl_len;
            lp_md->out.ip_recv_data = pp_rd->ip_data;
            lp_md->out.iv_recv_data_size = pp_rd->iv_data_len;
            lp_md->out.iv_recv_ctrl_max = 0;
            lp_md->out.iv_recv_data_max = 0;
            iv_ms_oc_callback(lp_md, NULL);
            pp_rd->ip_ctrl = NULL;
        }

        //
        // Send open-ack
        //
        MS_Md_Type *lp_open_ack_md;
        Msg_Mgr::get_md(&lp_open_ack_md, // finish_recv (open-ack)
                        this,
                        NULL,
                        true,            // send
                        NULL,            // fserr
                        "Trans_Stream::finish_recv (open ack)",
                        MD_STATE_MSG_LOW_OPEN_ACK);
        SB_util_assert_pne(lp_open_ack_md, NULL); // TODO: can't get md
        lp_open_ack_md->iv_inline = true;
        send_open_ack(lp_open_ack_md, pp_hdr->iv_reqid);
    } else if (pp_hdr->iv_type == MS_PMH_TYPE_CONN) {
        Ms_Sm_Conn_Type *lp_conn =
          reinterpret_cast<Ms_Sm_Conn_Type *>(pp_hdr->ia_data);
        set_pname(lp_conn->ia_pname);
        set_prog(lp_conn->ia_prog);

        //
        // Send conn-ack
        //
        MS_Md_Type *lp_conn_ack_md;
        Msg_Mgr::get_md(&lp_conn_ack_md, // finish_recv (conn-ack)
                        this,
                        NULL,
                        true,             // send
                        NULL,             // fserr
                        "Trans_Stream::finish_recv (conn ack)",
                        MD_STATE_MSG_LOW_CONN_ACK);
        SB_util_assert_pne(lp_conn_ack_md, NULL); // TODO: can't get md
        lp_conn_ack_md->iv_inline = true;
        send_conn_ack(lp_conn_ack_md, pp_hdr->iv_reqid);
    } else {
        //
        // server processing, NOT abandon
        //

        if (pp_hdr->iv_opts) {
            if (pp_hdr->iv_opts & MS_PMH_OPT_FSREQ)
                finish_recv_server_fsreq(pp_rd, pp_hdr);
            else if (pp_hdr->iv_opts & MS_PMH_OPT_MSIC)
                finish_recv_server_msinterceptor(pp_rd, pp_hdr);
            else
                finish_recv_server_normal(pp_rd, pp_hdr);
        } else
            finish_recv_server_normal(pp_rd, pp_hdr);

    }
}

//
// An fsreq needs to be processed outside of $RECEIVE
//
void SB_Trans::Trans_Stream::finish_recv_server_fsreq(Rd_Type     *pp_rd,
                                                      MS_PMH_Type *pp_hdr) {
    const char             *WHERE = "Trans_Stream::finish_recv_server_fsreq";
    static void            *lp_handle;
    MS_Md_Type             *lp_md;
    MS_Md_Type             *lp_reply_nack_md;
    short                   lv_fserr;
    static Proc_Fsreq_Type  lv_fsreq;
    static bool             lv_inited = false;

    Msg_Mgr::get_md(&lp_md,    // finish_recv_server_fsreq
                    this,
                    NULL,
                    false,     // recv
                    &lv_fserr, // fserr
                    WHERE,
                    MD_STATE_RCVD_FSREQ);
    if (lp_md == NULL) {
        //
        // Send recv-nack
        //
        Msg_Mgr::get_md(&lp_reply_nack_md, // finish_recv_server_fsreq (reply_nack)
                        this,
                        NULL,
                        true,             // send
                        NULL,             // fserr
                        "Trans_Stream::finish_recv_server_fsreq (reply nack)",
                        MD_STATE_MSG_LOW_REPLY_NACK);
        SB_util_assert_pne(lp_reply_nack_md, NULL); // TODO: can't get md
        lp_reply_nack_md->iv_inline = true;
        send_reply_nack(lp_reply_nack_md, pp_hdr->iv_reqid);
    } else {
        // don't add reqid to map - can't be cancelled
        if (gv_ms_trace)
            trace_where_printf(WHERE,
                               "stream=%p %s, processing fsreq md (reqid=%d, msgid=%d, md=%p)\n",
                               pfp(this), ia_stream_name,
                               pp_hdr->iv_reqid,
                               lp_md->iv_link.iv_id.i, pfp(lp_md));

        set_basic_md(pp_rd, pp_hdr, lp_md, false);
        if (gv_ms_assert_chk) // finish_recv_server_fsreq (check client req)
            checksum_recv_check(pp_rd, pp_hdr, false);


        // we own the buffers until they're copied
        pp_rd->ip_ctrl = NULL;
        pp_rd->ip_data = NULL;

        if (!lv_inited) {
            lv_inited = true;
            lp_handle = dlopen("libsbfs.so", RTLD_NOW);
            if (lp_handle != NULL)
                lv_fsreq.ipfsreq = dlsym(lp_handle, "fs_int_process_fsreq");
            else
                lv_fsreq.ipfsreq = NULL;
        }
        if (lv_fsreq.ipfsreq != NULL)
            lv_fsreq.ifsreq(lp_md);
        else
            SB_util_assert_pne(lv_fsreq.ipfsreq, NULL); // sw fault
    }
}

//
// A msinterceptor needs to be processed outside of $RECEIVE
//
void SB_Trans::Trans_Stream::finish_recv_server_msinterceptor(Rd_Type     *pp_rd,
                                                              MS_PMH_Type *pp_hdr) {
    const char *WHERE = "Trans_Stream::finish_recv_server_msinterceptor";
    char        la_err[200];
    MS_Md_Type *lp_md;
    MS_Md_Type *lp_reply_nack_md;
    short       lv_fserr;
    bool        lv_reply;

    Msg_Mgr::get_md(&lp_md,    // finish_recv_server_msinterceptor
                    this,
                    NULL,
                    false,     // recv
                    &lv_fserr, // fserr
                    WHERE,
                    MD_STATE_RCVD_IC);
    if (lp_md == NULL) {
        //
        // Send recv-nack
        //
        Msg_Mgr::get_md(&lp_reply_nack_md, // finish_recv_server_msinterceptor (reply_nack)
                        this,
                        NULL,
                        true,             // send
                        NULL,             // fserr
                        "Trans_Stream::finish_recv_server_msinterceptor (reply nack)",
                        MD_STATE_MSG_LOW_REPLY_NACK);
        SB_util_assert_pne(lp_reply_nack_md, NULL); // TODO: can't get md
        lp_reply_nack_md->iv_inline = true;
        send_reply_nack(lp_reply_nack_md, pp_hdr->iv_reqid);
    } else {
        // don't add reqid to map - can't be cancelled
        set_basic_md(pp_rd, pp_hdr, lp_md, false);
        if (gv_ms_assert_chk) // finish_recv_server_msinterceptor (check client req)
            checksum_recv_check(pp_rd, pp_hdr, false);

        // we own the buffers until they're copied
        pp_rd->ip_ctrl = NULL;
        pp_rd->ip_data = NULL;

        ms_interceptor(WHERE,
                       lp_md,
                       pp_hdr->iv_reqid,
                       &lv_reply,
                       &lv_fserr,
                       la_err);
        if (lv_reply) {
            if (gv_ms_trace_ic)
                trace_where_printf(WHERE, "EXIT stream=%p %s, %s, replying with err=%d\n",
                                   pfp(this), ia_stream_name,
                                   la_err, lv_fserr);
            exec_reply(lp_md,
                       0,                                  // src
                       lp_md->out.iv_recv_mpi_source_rank, // dest
                       lp_md->out.iv_recv_req_id,          // reqid
                       NULL,                               // req_ctrl
                       0,                                  // req_ctrl_size
                       NULL,                               // req_data
                       0,                                  // req_data_size
                       lv_fserr);                          // fserr
            wait_rep_done(lp_md);
            free_reply_md(lp_md);
        } else {
            if (gv_ms_trace_ic)
                trace_where_printf(WHERE, "EXIT stream=%p %s, reply by interceptor\n",
                                   pfp(this), ia_stream_name);
        }
    }
}

void SB_Trans::Trans_Stream::finish_recv_server_normal(Rd_Type     *pp_rd,
                                                       MS_PMH_Type *pp_hdr) {
    const char *WHERE = "Trans_Stream::finish_recv_server_normal";
    MS_Md_Type *lp_md;
    MS_Md_Type *lp_reply_nack_md;
    short       lv_fserr;

    // fill out md for delivery
    Msg_Mgr::get_md(&lp_md,    // finish_recv_server_normal
                    this,
                    NULL,
                    false,     // recv
                    &lv_fserr, // fserr
                    WHERE,
                    MD_STATE_RCVD_MSREQ);
    if (lp_md == NULL) {
        //
        // Send recv-nack
        //
        Msg_Mgr::get_md(&lp_reply_nack_md, // finish_recv_server_normal (reply_nack)
                        this,
                        NULL,
                        true,             // send
                        NULL,             // fserr
                        "Trans_Stream::finish_recv_server_normal (reply nack)",
                        MD_STATE_MSG_LOW_REPLY_NACK);
        SB_util_assert_pne(lp_reply_nack_md, NULL); // TODO: can't get md
        lp_reply_nack_md->iv_inline = true;
        send_reply_nack(lp_reply_nack_md, pp_hdr->iv_reqid);
    } else {
        if (gv_ms_msg_timestamp) {
            lp_md->iv_ts_msg_cli_link.tv_sec = pp_hdr->iv_ts1.iv_sec;
            lp_md->iv_ts_msg_cli_link.tv_usec = pp_hdr->iv_ts1.iv_usec;
            gettimeofday(&lp_md->iv_ts_msg_srv_rcvd, NULL);
        }
        add_msgid_to_reqid_map(pp_hdr->iv_reqid, lp_md->iv_link.iv_id.i);

        if (gv_ms_trace)
            trace_where_printf(WHERE,
                               "stream=%p %s, adding md (reqid=%d, msgid=%d, md=%p) to server recv queue\n",
                               pfp(this), ia_stream_name,
                               pp_hdr->iv_reqid,
                               lp_md->iv_link.iv_id.i, pfp(lp_md));

        set_basic_md(pp_rd, pp_hdr, lp_md, false);
        if (gv_ms_assert_chk) // finish_recv_server_normal (check client req)
            checksum_recv_check(pp_rd, pp_hdr, false);


        // we own the buffers until they're copied
        pp_rd->ip_ctrl = NULL;
        pp_rd->ip_data = NULL;

        gv_ms_ic_ctr.ctr_bump_msgs_rcvd();

        // add it to recv q
        if ((iv_ms_lim_callback != NULL) &&
            iv_ms_lim_callback(lp_md->out.iv_ptype, // non-mon-msg
                               reinterpret_cast<short *>(lp_md->out.ip_recv_ctrl),
                               lp_md->out.iv_recv_ctrl_size,
                               lp_md->out.ip_recv_data,
                               lp_md->out.iv_recv_data_size)) {
            ip_ms_lim_q->add(&lp_md->iv_link);
#ifdef USE_EVENT_REG
            ip_event_mgr->set_event_reg(INTR);
#else
            ip_event_mgr->set_event_all(INTR);
#endif
        } else {
            ip_ms_recv_q->add(&lp_md->iv_link);
#ifdef USE_EVENT_REG
            ip_event_mgr->set_event_reg(LREQ);
#else
            ip_event_mgr->set_event_all(LREQ);
#endif
        }
    }
}

//
// Purpose: finish client reply
//
void SB_Trans::Trans_Stream::finish_reply(MS_Md_Type *pp_md,
                                          short       pv_fserr,
                                          bool        pv_harderr,
                                          bool        pv_req_map_lock) {
    finish_reply_static(pp_md,
                        pv_fserr,
                        pv_harderr,
                        iv_generation,
                        &iv_req_map,
                        pv_req_map_lock,
                        (iv_stream_type == STREAM_TYPE_SELF),
                        iv_ms_comp_callback);
}

//
// Purpose: finish client reply
// (static)
//
void SB_Trans::Trans_Stream::finish_reply_static(MS_Md_Type      *pp_md,
                                                 short            pv_fserr,
                                                 bool             pv_harderr,
                                                 int              pv_generation,
                                                 SB_Ts_Md_Map    *pp_req_map,
                                                 bool             pv_req_map_lock,
                                                 bool             pv_self,
                                                 SB_Comp_Cb_Type  pv_ms_comp_callback) {
    const char *WHERE = "Trans_Stream::finish_reply";
    bool        lv_set_event;

    if (gv_ms_trace)
        trace_where_printf(WHERE,
                           "adding md (msgid=%d, md=%p, tag=%ld) to client recv done, opts=%d, fserr=%d, gen=%d\n",
                           pp_md->iv_link.iv_id.i, pfp(pp_md),
                           pp_md->iv_tag, pp_md->out.iv_opts, pv_fserr,
                           pv_generation);
    // send or recv can set fserr - don't overwrite
    if (pp_md->out.iv_fserr == XZFIL_ERR_OK) {
        pp_md->out.iv_fserr = pv_fserr;
        pp_md->out.iv_fserr_hard = pv_harderr;
        pp_md->out.iv_fserr_generation = pv_generation;
    }

    // remove md from request list
    if (pp_req_map != NULL)
        pp_req_map->remove_lock(pp_md->iv_link.iv_id.i, pv_req_map_lock);

    // tell client it's done
    pp_md->iv_md_state = MD_STATE_WR_FIN;
    if (pp_md->iv_abandoned) {
        Msg_Mgr::put_md_link(pp_md, "reply done, abandoned");
    } else if (pp_md->iv_inline) {
        pp_md->iv_reply_done = true;
        pp_md->iv_cv.signal(true); // need lock
    } else {
        if ((pv_fserr == XZFIL_ERR_OK) &&
            (!pv_self) &&
            gv_ms_assert_chk && gv_ms_assert_chk_send) { // finish_reply (check client req send)
            // check that buffer still *checks* after send
            checksum_send_check(pp_md, true);
        }

        gv_ms_ic_ctr.ctr_bump_msgs_sent();
        pp_md->ip_mgr->change_replies_done(1, 0);
        lv_set_event = true;
        if (pp_md->out.iv_opts & MS_OPTS_FSDONE)
{
            pp_md->ip_fs_comp_q->add(&pp_md->iv_link);
}
        else if (pp_md->out.iv_opts & MS_OPTS_LDONE) {
            lv_set_event = false;
            pp_md->out.iv_ldone = true;
            pv_ms_comp_callback(pp_md);
        }
        if (lv_set_event) {
            if (gv_ms_trace_params) {
                gettimeofday(&pp_md->out.iv_comp_q_on_tod, NULL);
                trace_where_printf(WHERE, "setting done, msgid=%d, md=%p, fserr=%d\n",
                                   pp_md->iv_link.iv_id.i, pfp(pp_md), pv_fserr);
            }
            pp_md->ip_mgr->set_event(LDONE, &pp_md->iv_reply_done);
        }
        // needs to be done last
        pp_md->iv_cv.signal(true); // need lock
    }
}

//
// Purpose: finish send
//
void SB_Trans::Trans_Stream::finish_send(const char *pp_where,
                                         const char *pp_id,
                                         MS_Md_Type *pp_md,
                                         short       pv_fserr,
                                         bool        pv_harderr) {
    const char *WHERE = "Trans_Stream::finish_send";
    int         lv_status;

    if (gv_ms_trace) {
        char la_where[100];
        sprintf(la_where, "%s %s", WHERE, pp_where);
        trace_where_printf(la_where, "stream=%p %s, finish send %s msgid=%d, md=%p, op=%d, fserr=%d\n",
                           pfp(this), ia_stream_name,
                           pp_id, pp_md->iv_link.iv_id.i, pfp(pp_md),
                           pp_md->iv_op, pv_fserr);
    }
    switch (pp_md->iv_op) {
    case MS_OP_REPLY:
        pp_md->iv_send_done = true;
        pp_md->out.iv_fserr = pv_fserr;
        pp_md->out.iv_fserr_hard = pv_harderr;
        pp_md->out.iv_fserr_generation = iv_generation;
        pp_md->iv_md_state = MD_STATE_REPLY_SEND_FIN;


        // tell client it's done
        pp_md->iv_reply_done = true;
        pp_md->iv_cv.signal(true); // need lock
        break;

    case MS_OP_REPLY_NACK:
        // all done
        Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "finish-send REPLY-NACK");
        break;

    case MS_OP_REPLY_NW:
        pp_md->iv_send_done = true;
        pp_md->out.iv_fserr = pv_fserr;
        pp_md->out.iv_fserr_hard = pv_harderr;
        pp_md->out.iv_fserr_generation = iv_generation;
        pp_md->iv_md_state = MD_STATE_REPLY_SEND_FIN;


        // all done
        free_reply_md(pp_md);
        break;

    case MS_OP_WR:
        lv_status = pp_md->iv_sl.lock();
        SB_util_assert_ieq(lv_status, 0);
        if (pp_md->iv_free_md) {
            lv_status = pp_md->iv_sl.unlock();
            SB_util_assert_ieq(lv_status, 0);
            Msg_Mgr::put_md(pp_md->iv_msgid, "finish-send WR");
        } else {
            // if reply done - don't overwrite
            if (!pp_md->iv_reply_done) {
                pp_md->out.iv_fserr = pv_fserr;
                pp_md->out.iv_fserr_hard = pv_harderr;
                pp_md->out.iv_fserr_generation = iv_generation;
            }
            pp_md->iv_md_state = MD_STATE_WR_SEND_FIN;
            pp_md->iv_send_done = true;
            lv_status = pp_md->iv_sl.unlock();
            SB_util_assert_ieq(lv_status, 0);
        }
        break;

    case MS_OP_FLUSH:
        pp_md->iv_send_done = true;
        pp_md->out.iv_fserr = pv_fserr;
        pp_md->out.iv_fserr_hard = pv_harderr;
        pp_md->out.iv_fserr_generation = iv_generation;
        Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "finish-send FLUSH");
        break;

    case MS_OP_ABANDON:
        lv_status = pp_md->iv_sl.lock();
        SB_util_assert_ieq(lv_status, 0);
        if (pp_md->iv_free_md) {
            lv_status = pp_md->iv_sl.unlock();
            SB_util_assert_ieq(lv_status, 0);
            Msg_Mgr::put_md(pp_md->iv_msgid, "finish-send ABANDON");
        } else {
            // if abandon-ack done - don't overwrite
            pp_md->iv_md_state = MD_STATE_ABANDON_FIN;
            pp_md->iv_send_done = true;
            lv_status = pp_md->iv_sl.unlock();
            SB_util_assert_ieq(lv_status, 0);
        }
        break;

    case MS_OP_ABANDON_ACK:
        // all done
        Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "finish-send ABANDON-ACK");
        break;

    case MS_OP_CLOSE:
        pp_md->iv_send_done = true;
        if (gv_ms_trace)
            trace_where_printf(WHERE, "setting send-done=%d for msgid=%d, md=%p\n",
                               pp_md->iv_send_done,
                               pp_md->iv_link.iv_id.i, pfp(pp_md));
        pp_md->iv_cv2.signal(true); // need lock
        // keep md
        break;

    case MS_OP_CLOSE_ACK:
        // all done
        Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "finish-send CLOSE-ACK");
        break;

    case MS_OP_OPEN:
        // keep md
        break;

    case MS_OP_OPEN_ACK:
        // all done
        Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "finish-send OPEN-ACK");
        break;

    default:
        SB_util_abort("invalid pp_md->iv_op"); // sw fault
    }
}

//
// Purpose: finish all in-transit writereads
//
void SB_Trans::Trans_Stream::finish_writereads(short pv_fserr) {
    const char *WHERE = "Trans_Stream::finish_writereads";
    int         lv_status;

    if (gv_ms_trace)
        trace_where_printf(WHERE, "stream=%p %s, finish fserr=%d\n",
                           pfp(this), ia_stream_name, pv_fserr);

    lv_status = iv_error_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    iv_req_map.lock();
    while (iv_req_map.size()) {
        SB_Imap_Enum *lp_enum = iv_req_map.keys();
        // cache MAX_MDS and then remove them
        // need to do this because enum is not safe from remove
        enum { MAX_IDS = 10 };
        int la_ids[MAX_IDS];
        int lv_max;
        for (lv_max = 0;
             lp_enum->more() && (lv_max < MAX_IDS);
             lv_max++)
            la_ids[lv_max] = lp_enum->next()->iv_id.i;
        for (int lv_inx = 0; lv_inx < lv_max; lv_inx++) {
            MS_Md_Type *lp_md =
              static_cast<MS_Md_Type *>(iv_req_map.remove_lock(la_ids[lv_inx], false));
            if (lp_md != NULL) {
                lp_md->iv_reply_done_temp = lp_md->iv_reply_done ? 0x0101 : 0x0100;
                if (lp_md->iv_inline) {
                    if (gv_ms_trace)
                        trace_where_printf(WHERE, "stream=%p %s, finish inline msgid=%d, md=%p, fserr=%d, setting reply-done=1\n",
                                           pfp(this), ia_stream_name,
                                           lp_md->iv_link.iv_id.i,
                                           pfp(lp_md),
                                           pv_fserr);
                    lp_md->iv_reply_done = true;
                    lp_md->iv_cv.signal(true); // need lock
                } else {
                    if (!lp_md->iv_reply_done) {
                        if (gv_ms_trace)
                            trace_where_printf(WHERE, "stream=%p %s, finish reply msgid=%d, md=%p, fserr=%d\n",
                                               pfp(this), ia_stream_name,
                                               lp_md->iv_link.iv_id.i,
                                               pfp(lp_md),
                                               pv_fserr);
                        finish_reply(lp_md, pv_fserr, true, false);
                    }
                }
            }
        }
        delete lp_enum;
    }
    iv_req_map.unlock();
    lv_status = iv_error_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

//
// Purpose: is generation of stream's generation?
//
bool SB_Trans::Trans_Stream::error_of_generation(int pv_generation) {
    const char *WHERE = "Trans_Stream::error_of_generation";
    bool        lv_ret;
    int         lv_status;

    lv_status = iv_generation_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    if (pv_generation == iv_generation) {
        iv_generation++;
        if (gv_ms_trace)
            trace_where_printf(WHERE, "stream=%p %s, change generation, gen=%d\n",
                               pfp(this), ia_stream_name, iv_generation);
        lv_ret = true;
    } else
        lv_ret = false;
    lv_status = iv_generation_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);
    return lv_ret;
}

//
// Purpose: synchronize error handling
//
void SB_Trans::Trans_Stream::error_sync() {
    int lv_status;

    lv_status = iv_error_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    lv_status = iv_error_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

//
// Purpose: free reply
//
void SB_Trans::Trans_Stream::free_reply_md(MS_Md_Type *pp_md) {
    if (pp_md->out.iv_recv_req_id > 0)
        remove_msgid_from_reqid_map(pp_md->out.iv_recv_req_id, true);
    if (pp_md->iv_abandoned)
        iv_ms_abandon_callback(pp_md, false);
    Msg_Mgr::put_md(pp_md->iv_link.iv_id.i, "free-reply-md");
}

//
// Purpose: get hdr type
//
void SB_Trans::Trans_Stream::get_hdr_type(int pv_hdr_type, char *pp_hdr_type) {
    switch (pv_hdr_type) {
    case MS_PMH_TYPE_WR:
        strcpy(pp_hdr_type, "wr");
        break;
    case MS_PMH_TYPE_REPLY:
        strcpy(pp_hdr_type, "rep");
        break;
    case MS_PMH_TYPE_ABANDON:
        strcpy(pp_hdr_type, "ab");
        break;
    case MS_PMH_TYPE_ABANDON_ACK:
        strcpy(pp_hdr_type, "ab-ack");
        break;
    case MS_PMH_TYPE_CLOSE:
        strcpy(pp_hdr_type, "cl");
        break;
    case MS_PMH_TYPE_CLOSE_ACK:
        strcpy(pp_hdr_type, "cl-ack");
        break;
    case MS_PMH_TYPE_OPEN:
        strcpy(pp_hdr_type, "op");
        break;
    case MS_PMH_TYPE_OPEN_ACK:
        strcpy(pp_hdr_type, "op-ack");
        break;
    case MS_PMH_TYPE_REPLY_NACK:
        strcpy(pp_hdr_type, "rep-nack");
        break;
    case MS_PMH_TYPE_CONN:
        strcpy(pp_hdr_type, "neg");
        break;
    case MS_PMH_TYPE_CONN_ACK:
        strcpy(pp_hdr_type, "conn-ack");
        break;
    default:
        strcpy(pp_hdr_type, "<unknown>");
        break;
    }
}

//
// Purpose: get idle-timer-link
//
SB_TML_Type *SB_Trans::Trans_Stream::get_idle_timer_link() {
    return &iv_idle_timer_link;
}

//
// Purpose: get monitor stream
// (static)
//
SB_Trans::Trans_Stream *SB_Trans::Trans_Stream::get_mon_stream() {
    return cp_mon_stream;
}

//
// Purpose: get stream name
//
char *SB_Trans::Trans_Stream::get_name() {
    return ia_stream_name;
}

//
// Purpose: get oid
//
int SB_Trans::Trans_Stream::get_oid() {
    return iv_oid;
}

//
// Purpose: get open nid
//
int SB_Trans::Trans_Stream::get_open_nid() {
    return iv_open_nid;
}

//
// Purpose: get open pid
//
int SB_Trans::Trans_Stream::get_open_pid() {
    return iv_open_pid;
}

//
// Purpose: get remote nid
//
int SB_Trans::Trans_Stream::get_remote_nid() {
    return iv_remote_nid;
}

//
// Purpose: get remote pid
//
int SB_Trans::Trans_Stream::get_remote_pid() {
    return iv_remote_pid;
}

#ifdef SQ_PHANDLE_VERIFIER
//
// Purpose: get remote pid
//
SB_Verif_Type SB_Trans::Trans_Stream::get_remote_verif() {
    return iv_remote_verif;
}
#endif

//
// Purpose: get shutdown
//
bool SB_Trans::Trans_Stream::get_shutdown() {
    return cv_shutdown;
}

//
// Purpose: get accept stream count
// (static)
//
int SB_Trans::Trans_Stream::get_stream_acc_count() {
    return cv_stream_acc_count.read_val();
}

//
// Purpose: get accept stream hi-count
// (static)
//
int SB_Trans::Trans_Stream::get_stream_acc_hi_count() {
    return cv_stream_acc_hi_count.read_val();
}

//
// Purpose: get connect stream count
// (static)
//
int SB_Trans::Trans_Stream::get_stream_con_count() {
    return cv_stream_con_count.read_val();
}

//
// Purpose: get connect stream hi-count
// (static)
//
int SB_Trans::Trans_Stream::get_stream_con_hi_count() {
    return cv_stream_con_hi_count.read_val();
}
//
// Purpose: get total stream count
// (static)
//
int SB_Trans::Trans_Stream::get_stream_total_count() {
    return cv_stream_total_count.read_val();
}

//
// Purpose: get total stream hi-count
// (static)
//
int SB_Trans::Trans_Stream::get_stream_total_hi_count() {
    return cv_stream_total_hi_count.read_val();
}

//
// Purpose: get thread-marker
//
bool SB_Trans::Trans_Stream::get_thread_marker() {
    char *lp_buf =
      static_cast<char *>(SB_Thread::Sthr::specific_get(gv_sb_stream_tls_inx));
    if (lp_buf == NULL)
        return false;
    return *lp_buf;
}

void SB_Trans::Trans_Stream::init(const char *pp_name,
                                  const char *pp_pname,
                                  const char *pp_prog) {
    if (strcmp(pp_name, "monitor") == 0)
        set_mon_stream(this);
    strcpy(ia_stream_name, pp_name);
    strcpy(ia_pname, pp_pname);
    strcpy(ia_prog, pp_prog);
    md_ref_set(0);
    ref_set(0);
    sb_queue_dql_init(&iv_close_link);
    sb_queue_dql_init(&iv_del_link);
    iv_error_mutex.setname("mutex-Trans_Stream::iv_error_mutex");
    iv_generation_mutex.setname("mutex-Trans_Stream::iv_generation_mutex");
    iv_post_mutex.setname("mutex-Trans_Stream::iv_post_mutex");
}

//
// Purpose: is self?
//
bool SB_Trans::Trans_Stream::is_self() {
    return (iv_stream_type == STREAM_TYPE_SELF);
}

//
// Purpose: add stream to nid/pid map
//
#ifdef SQ_PHANDLE_VERIFIER
void SB_Trans::Trans_Stream::map_nidpid_add_stream(int pv_nid, int pv_pid, SB_Verif_Type pv_verif, bool pv_lock) {
#else
void SB_Trans::Trans_Stream::map_nidpid_add_stream(int pv_nid, int pv_pid, bool pv_lock) {
#endif
    const char  *WHERE = "Trans_Stream::map_nidpid_add_stream";
#ifdef SQ_PHANDLE_VERIFIER
    SB_NPV_Type  lv_npv;
#else
    NidPid_Type  lv_nidpid;
#endif

    if (gv_ms_trace || (gv_ms_trace_xx & MS_TRACE_XX_NIDPID))
#ifdef SQ_PHANDLE_VERIFIER
        trace_where_printf(WHERE, "ENTER stream=%p %s, p-id=%d/%d/" PFVY "\n",
#else
        trace_where_printf(WHERE, "ENTER stream=%p %s, p-id=%d/%d\n",
#endif
                           pfp(this), ia_stream_name,
#ifdef SQ_PHANDLE_VERIFIER
                           pv_nid, pv_pid, pv_verif);
#else
                           pv_nid, pv_pid);
#endif
#ifdef SQ_PHANDLE_VERIFIER
    lv_npv.iv_nid = pv_nid;
    lv_npv.iv_pid = pv_pid;
    lv_npv.iv_verif = pv_verif;
    NPS_Node *lp_node = new_NPS_Node(lv_npv, this);
#else
    lv_nidpid.u.i.iv_nid = pv_nid;
    lv_nidpid.u.i.iv_pid = pv_pid;
    NPS_Node *lp_node = new_NPS_Node(lv_nidpid.u.iv_nidpid, this);
#endif
    gv_sb_stream_nidpid_map.put_lock(&lp_node->iv_link, pv_lock);
}

//
// Purpose: map nid/pid to stream
// (static)
//
#ifdef SQ_PHANDLE_VERIFIER
SB_Trans::Trans_Stream *SB_Trans::Trans_Stream::map_nidpid_key_to_stream(SB_NPV_Type pv_npv,
                                                                         bool         pv_lock) {

    return map_nidpid_to_stream(pv_npv.iv_nid,
                                pv_npv.iv_pid,
                                pv_npv.iv_verif,
                                pv_lock);
}
#else
SB_Trans::Trans_Stream *SB_Trans::Trans_Stream::map_nidpid_key_to_stream(SB_Int64_Type pv_nidpid,
                                                                         bool          pv_lock) {
    NidPid_Type lv_nidpid;

    lv_nidpid.u.iv_nidpid = pv_nidpid;
    return map_nidpid_to_stream(lv_nidpid.u.i.iv_nid, lv_nidpid.u.i.iv_pid, pv_lock);
}
#endif

#ifdef SQ_PHANDLE_VERIFIER
SB_NPVmap_Enum *SB_Trans::Trans_Stream::map_nidpid_keys() {
#else
SB_LLmap_Enum *SB_Trans::Trans_Stream::map_nidpid_keys() {
#endif
#ifdef SQ_PHANDLE_VERIFIER
    SB_NPVmap_Enum *lp_enum;
#else
    SB_LLmap_Enum *lp_enum;
#endif

    lp_enum = gv_sb_stream_nidpid_map.keys();
    return lp_enum;
}

void SB_Trans::Trans_Stream::map_nidpid_lock() {
    gv_sb_stream_nidpid_map.lock();
}

int SB_Trans::Trans_Stream::map_nidpid_trylock() {
    return gv_sb_stream_nidpid_map.trylock();
}

//
// Purpose: remove stream from nid/pid map
//
bool SB_Trans::Trans_Stream::map_nidpid_remove(bool pv_lock) {
    const char  *WHERE = "Trans_Stream::map_nidpid_remove";
#ifdef SQ_PHANDLE_VERIFIER
    SB_NPV_Type  lv_npv;
#else
    NidPid_Type  lv_nidpid;
#endif
    bool         lv_ret;

    if (iv_open_nid >= 0) {
        if (gv_ms_trace || (gv_ms_trace_xx & MS_TRACE_XX_NIDPID))
#ifdef SQ_PHANDLE_VERIFIER
            trace_where_printf(WHERE, "ENTER stream=%p %s, p-id=%d/%d/" PFVY "\n",
#else
            trace_where_printf(WHERE, "ENTER stream=%p %s, p-id=%d/%d\n",
#endif
                               pfp(this), ia_stream_name,
#ifdef SQ_PHANDLE_VERIFIER
                               iv_open_nid, iv_open_pid, iv_open_verif);
#else
                               iv_open_nid, iv_open_pid);
#endif
#ifdef SQ_PHANDLE_VERIFIER
        lv_npv.iv_nid = iv_open_nid;
        lv_npv.iv_pid = iv_open_pid;
        lv_npv.iv_verif = iv_open_verif;
        NPS_Node *lp_node =
          static_cast<NPS_Node *>(gv_sb_stream_nidpid_map.remove_lock(lv_npv, pv_lock));
#else
        lv_nidpid.u.i.iv_nid = iv_open_nid;
        lv_nidpid.u.i.iv_pid = iv_open_pid;
        NPS_Node *lp_node =
          static_cast<NPS_Node *>(gv_sb_stream_nidpid_map.remove_lock(lv_nidpid.u.iv_nidpid, pv_lock));
#endif
        if (lp_node == NULL) {
            lv_ret = false;
            if (gv_ms_trace || (gv_ms_trace_xx & MS_TRACE_XX_NIDPID))
#ifdef SQ_PHANDLE_VERIFIER
                trace_where_printf(WHERE, "no stream removed, p-id=%d/%d/" PFVY "\n",
#else
                trace_where_printf(WHERE, "no stream removed, p-id=%d/%d\n",
#endif
#ifdef SQ_PHANDLE_VERIFIER
                                   iv_open_nid, iv_open_pid, iv_open_verif);
#else
                                   iv_open_nid, iv_open_pid);
#endif
        } else {
            lv_ret = true;
            if (gv_ms_trace || (gv_ms_trace_xx & MS_TRACE_XX_NIDPID))
#ifdef SQ_PHANDLE_VERIFIER
                trace_where_printf(WHERE, "removed actual stream=%p %s, p-id=%d/%d/" PFVY "\n",
#else
                trace_where_printf(WHERE, "removed actual stream=%p %s, p-id=%d/%d\n",
#endif
                                   pfp(lp_node->ip_stream),
                                   lp_node->ip_stream->ia_stream_name,
#ifdef SQ_PHANDLE_VERIFIER
                                   iv_open_nid, iv_open_pid, iv_open_verif);
#else
                                   iv_open_nid, iv_open_pid);
#endif
            delete lp_node;
        }
    } else
        lv_ret = false;
    return lv_ret;
}

//
// Purpose: remove stream from nid/pid map
//
void SB_Trans::Trans_Stream::map_nidpid_remove_stream(const char *pp_where,
                                                      int         pv_nid,
                                                      int         pv_pid
#ifdef SQ_PHANDLE_VERIFIER
                                                     ,SB_Verif_Type  pv_verif
#endif
                                                     ) {
    const char  *WHERE = "Trans_Stream::map_nidpid_remove_stream";
    char         la_where[100];
#ifdef SQ_PHANDLE_VERIFIER
    SB_NPV_Type  lv_npv;
#else
    NidPid_Type  lv_nidpid;
#endif

    if (gv_ms_trace || (gv_ms_trace_xx & MS_TRACE_XX_NIDPID))
        sprintf(la_where, "%s(%s)", WHERE, pp_where);
    if (gv_ms_trace || (gv_ms_trace_xx & MS_TRACE_XX_NIDPID))
#ifdef SQ_PHANDLE_VERIFIER
        trace_where_printf(la_where, "ENTER stream=%p %s, p-id=%d/%d/" PFVY "\n",
#else
        trace_where_printf(la_where, "ENTER stream=%p %s, p-id=%d/%d\n",
#endif
                           pfp(this), ia_stream_name,
#ifdef SQ_PHANDLE_VERIFIER
                           pv_nid, pv_pid, pv_verif);
#else
                           pv_nid, pv_pid);
#endif
#ifdef SQ_PHANDLE_VERIFIER
    lv_npv.iv_nid = pv_nid;
    lv_npv.iv_pid = pv_pid;
    lv_npv.iv_verif = pv_verif;
    NPS_Node *lp_node =
      static_cast<NPS_Node *>(gv_sb_stream_nidpid_map.remove(lv_npv));
#else
    lv_nidpid.u.i.iv_nid = pv_nid;
    lv_nidpid.u.i.iv_pid = pv_pid;
    NPS_Node *lp_node =
      static_cast<NPS_Node *>(gv_sb_stream_nidpid_map.remove(lv_nidpid.u.iv_nidpid));
#endif
    if (lp_node == NULL) {
        if (gv_ms_trace || (gv_ms_trace_xx & MS_TRACE_XX_NIDPID))
#ifdef SQ_PHANDLE_VERIFIER
            trace_where_printf(la_where, "no stream removed, p-id=%d/%d/" PFVY "\n",
#else
            trace_where_printf(la_where, "no stream removed, p-id=%d/%d\n",
#endif
#ifdef SQ_PHANDLE_VERIFIER
                               pv_nid, pv_pid, pv_verif);
#else
                               pv_nid, pv_pid);
#endif
    } else {
        if (gv_ms_trace || (gv_ms_trace_xx & MS_TRACE_XX_NIDPID))
#ifdef SQ_PHANDLE_VERIFIER
            trace_where_printf(la_where, "removed actual stream=%p %s, p-id=%d/%d/" PFVY "\n",
#else
            trace_where_printf(la_where, "removed actual stream=%p %s, p-id=%d/%d\n",
#endif
                               pfp(lp_node->ip_stream),
                               lp_node->ip_stream->ia_stream_name,
#ifdef SQ_PHANDLE_VERIFIER
                               pv_nid, pv_pid, pv_verif);
#else
                               pv_nid, pv_pid);
#endif
        delete lp_node;
    }
}

int SB_Trans::Trans_Stream::map_nidpid_size() {
    int lv_size;

    lv_size = gv_sb_stream_nidpid_map.size();
    return lv_size;
}

//
// Purpose: map nid/pid to stream
// (static)
//
SB_Trans::Trans_Stream *SB_Trans::Trans_Stream::map_nidpid_to_stream(int           pv_nid,
                                                                     int           pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                                                     SB_Verif_Type pv_verif,
#endif
                                                                     bool          pv_lock) {
    const char *WHERE = "Trans_Stream::map_nidpid_to_stream";
    NPS_Node   *lp_node;
#ifdef SQ_PHANDLE_VERIFIER
    SB_NPV_Type lv_npv;
#else
    NidPid_Type lv_nidpid;
#endif

#ifdef SQ_PHANDLE_VERIFIER
    lv_npv.iv_nid = pv_nid;
    lv_npv.iv_pid = pv_pid;
    lv_npv.iv_verif = pv_verif;
    lp_node =
      static_cast<NPS_Node *>(gv_sb_stream_nidpid_map.get_lock(lv_npv,
                                                               pv_lock));
#else
    lv_nidpid.u.i.iv_nid = pv_nid;
    lv_nidpid.u.i.iv_pid = pv_pid;
    lp_node =
      static_cast<NPS_Node *>(gv_sb_stream_nidpid_map.get_lock(lv_nidpid.u.iv_nidpid,
                                                               pv_lock));
#endif
    if (lp_node == NULL) {
        if (gv_ms_trace)
            trace_where_printf(WHERE, "no stream for p-id=%d/%d\n",
                               pv_nid, pv_pid);
        return NULL;
    }
    Trans_Stream *lp_stream = lp_node->ip_stream;
    return lp_stream;
}

void SB_Trans::Trans_Stream::map_nidpid_unlock() {
    gv_sb_stream_nidpid_map.unlock();
}

//
// Purpose: dec md-ref count
//
int SB_Trans::Trans_Stream::md_ref_dec() {
    const char *WHERE = "Trans_Stream::md_ref_dec";
    int         lv_ref_count;

    lv_ref_count = iv_md_ref_count.sub_and_fetch(1);
    if (gv_ms_trace_ref)
        trace_where_printf(WHERE, "ENTER stream=%p %s, md-ref=%d\n",
                           pfp(this), ia_stream_name, lv_ref_count);
    return lv_ref_count;
}

//
// Purpose: get md-ref count
//
int SB_Trans::Trans_Stream::md_ref_get() {
    const char *WHERE = "Trans_Stream::md_ref_get";
    int         lv_ref_count;

    lv_ref_count = iv_md_ref_count.read_val();
    if (gv_ms_trace_ref)
        trace_where_printf(WHERE, "ENTER stream=%p %s, md-ref=%d\n",
                           pfp(this), ia_stream_name, lv_ref_count);
    return lv_ref_count;
}

//
// Purpose: inc md-ref count
//
void SB_Trans::Trans_Stream::md_ref_inc() {
    const char *WHERE = "Trans_Stream::md_ref_inc";
    int         lv_ref_count;

    lv_ref_count = iv_md_ref_count.add_and_fetch(1);
    if (gv_ms_trace_ref)
        trace_where_printf(WHERE, "ENTER stream=%p %s, md-ref=%d\n",
                           pfp(this), ia_stream_name, lv_ref_count);
}

//
// Purpose: set md-ref count
//
void SB_Trans::Trans_Stream::md_ref_set(int pv_val) {
    const char *WHERE = "Trans_Stream::md_ref_set";

    iv_md_ref_count.set_val(pv_val);
    if (gv_ms_trace_ref)
        trace_where_printf(WHERE, "ENTER stream=%p %s, md-ref=%d\n",
                           pfp(this), ia_stream_name, pv_val);
}

bool SB_Trans::Trans_Stream::post_mon_messages_get() {
    return iv_post_mon_messages;
}

void SB_Trans::Trans_Stream::post_mon_messages_set(bool pv_post_mon_messages) {
    iv_post_mon_messages = pv_post_mon_messages;
}

void SB_Trans::Trans_Stream::post_mon_mutex_lock() {
    int lv_status;

    lv_status = iv_post_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
}

void SB_Trans::Trans_Stream::post_mon_mutex_unlock() {
    int lv_status;

    lv_status = iv_post_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

void SB_Trans::Trans_Stream::process_abandon(const char *pp_where,
                                             int         pv_abandon_reqid,
                                             int         pv_req_reqid) {
    const char *WHERE = "Trans_Stream::process_abandon";
    MS_Md_Type *lp_md;
    RM_Node    *lp_node;
    bool        lv_reply_piggyback;
    int         lv_status;

    if (gv_ms_trace || gv_ms_trace_abandon)
        trace_where_printf(pp_where,
                           "stream=%p %s, received abandon (reqid=%d, target reqid=%d)\n",
                           pfp(this), ia_stream_name,
                           pv_req_reqid, pv_abandon_reqid);
    lv_reply_piggyback = false;
    iv_msgid_reqid_map.lock();
    lp_node = static_cast<RM_Node *>(iv_msgid_reqid_map.get_lock(pv_abandon_reqid, false));
    if (lp_node != NULL) {
        lp_md = Msg_Mgr::map_to_md(lp_node->iv_msgid, pp_where);
        lv_status = lp_md->iv_abandon_mutex.lock();
        SB_util_assert_ieq(lv_status, 0);
        if (remove_recv_q(lp_md)) {
            // msg was on receive-queue, remove it
            if (gv_ms_trace || gv_ms_trace_abandon)
                trace_where_printf(pp_where,
                                   "stream=%p %s, removed reqid from receive queue (reqid=%d)\n",
                                   pfp(this), ia_stream_name,
                                   pv_abandon_reqid);
            remove_msgid_from_reqid_map(lp_md->out.iv_recv_req_id, false);
            MS_BUF_MGR_FREE(lp_md->out.ip_recv_ctrl);
            lp_md->out.ip_recv_ctrl = NULL;
            MS_BUF_MGR_FREE(lp_md->out.ip_recv_data);
            lp_md->out.ip_recv_data = NULL;
            lv_status = lp_md->iv_abandon_mutex.unlock();
            SB_util_assert_ieq(lv_status, 0);
            Msg_Mgr::put_md(lp_md->iv_msgid, "server abandoned receive-q");
            lp_md = NULL;
        } else if (lp_md->iv_op == MS_OP_REPLY) {
            if (gv_ms_trace || gv_ms_trace_abandon)
                trace_where_printf(pp_where,
                                   "stream=%p %s, reply already started, ignoring cancel msgid=%d\n",
                                   pfp(this), ia_stream_name,
                                   lp_md->iv_msgid);
            lv_reply_piggyback = true;
        } else {
            lp_md->iv_abandoned = true;
            // msg is alive somewhere,
            if (gv_ms_trace || gv_ms_trace_abandon)
                trace_where_printf(pp_where,
                                   "stream=%p %s, msg not on receive queue, cancelling msgid=%d\n",
                                   pfp(this), ia_stream_name,
                                   lp_md->iv_msgid);
            iv_rep_map.remove(lp_md->iv_link.iv_id.i); // can't be on two lists
            iv_ms_abandon_callback(lp_md, true);
#ifdef USE_EVENT_REG
            ip_event_mgr->set_event_reg(LCAN);
#else
            ip_event_mgr->set_event_all(LCAN);
#endif
        }
        if (lp_md != NULL) {
            lv_status = lp_md->iv_abandon_mutex.unlock();
            SB_util_assert_ieq(lv_status, 0);
        }
    } else
        lp_md = NULL;
    iv_msgid_reqid_map.unlock();

    //
    // Send abandon-ack
    //
    MS_Md_Type *lp_can_ack_md;
    Msg_Mgr::get_md(&lp_can_ack_md, // process_abandon (cancel-ack)
                    this,
                    NULL,
                    true,           // send
                    NULL,           // fserr
                    WHERE,
                    MD_STATE_MSG_LOW_CAN_ACK);
    SB_util_assert_pne(lp_can_ack_md, NULL); // TODO: can't get md
    if (lv_reply_piggyback) {
        // if it is possible to piggyback ack to reply, then do so
        lv_status = iv_reply_piggyback_mutex.lock();
        SB_util_assert_ieq(lv_status, 0);
        if (lp_md->iv_md_state == MD_STATE_REPLY_SENDING) {
            lp_can_ack_md->iv_op = MS_OP_ABANDON_ACK;
            lp_can_ack_md->iv_aa_reqid = pv_req_reqid;
            lp_can_ack_md->iv_aa_can_ack_reqid = pv_abandon_reqid;
            add_reply_piggyback(lp_can_ack_md);
            lv_status = iv_reply_piggyback_mutex.unlock();
            SB_util_assert_ieq(lv_status, 0);
        } else {
            lv_status = iv_reply_piggyback_mutex.unlock();
            SB_util_assert_ieq(lv_status, 0);
            send_abandon_ack(lp_can_ack_md, pv_req_reqid, pv_abandon_reqid);
        }
    } else
        send_abandon_ack(lp_can_ack_md, pv_req_reqid, pv_abandon_reqid);
}

//
// (static)
//
void SB_Trans::Trans_Stream::recv_copy(const char        *pp_where,
                                       char             **ppp_source,
                                       int                pv_source_count,
                                       char              *pp_dest,
                                       int                pv_dest_count,
                                       int               *pp_rc,
                                       bool               pv_source_del) {
    char *lp_source = *ppp_source;
    int   lv_len;

    if (pv_source_count > 0)
        lv_len = sbmin(pv_dest_count, pv_source_count);
    else
        lv_len = pv_dest_count;
    if (gv_ms_trace)
        trace_where_printf(pp_where,
                           "(copy) src=%p, scnt=%d, dest=%p, dcnt=%d, len=%d\n",
                           lp_source, pv_source_count, pp_dest,
                           pv_dest_count, lv_len);
    if (pv_source_count == 0)
        *pp_rc = 0;
    else {
        memcpy(pp_dest, lp_source, lv_len);
        *pp_rc = pv_source_count;
        if (pv_source_del) {
            MS_BUF_MGR_FREE(lp_source);
            *ppp_source = NULL;
        }
    }
}

//
// Purpose: dec stream-ref count
//
int SB_Trans::Trans_Stream::ref_dec(const char *pp_where, void *pp_referee) {
    const char    *WHERE = "Trans_Stream::ref_dec";
    Trans_Stream **lpp_referee;
    int            lv_ref_count;

    if (pp_referee != NULL) {
        lpp_referee = static_cast<Trans_Stream **>(pp_referee);
        *lpp_referee = NULL;
    }
    lv_ref_count = iv_ref_count.sub_and_fetch(1);
    if (gv_ms_trace_ref)
        trace_where_printf(WHERE, "ENTER stream=%p %s, stream-ref=%d, referee=%p, where=%s\n",
                           pfp(this),
                           ia_stream_name,
                           lv_ref_count,
                           pp_referee,
                           pp_where);
    return lv_ref_count;
}

//
// Purpose: get stream-ref count
//
int SB_Trans::Trans_Stream::ref_get() {
    const char *WHERE = "Trans_Stream::ref_get";
    int         lv_ref_count;

    lv_ref_count = iv_ref_count.read_val();
    if (gv_ms_trace_ref)
        trace_where_printf(WHERE, "ENTER stream=%p %s, stream-ref=%d\n",
                           pfp(this), ia_stream_name, lv_ref_count);
    return lv_ref_count;
}

//
// Purpose: inc stream-ref count
//
int SB_Trans::Trans_Stream::ref_inc(const char *pp_where, void *pp_referee) {
    const char    *WHERE = "Trans_Stream::ref_inc";
    Trans_Stream **lpp_referee;
    int            lv_ref_count;

    lv_ref_count = iv_ref_count.add_and_fetch(1);
    if (pp_referee != NULL) {
        lpp_referee = static_cast<Trans_Stream **>(pp_referee);
        *lpp_referee = this;
    }
    if (gv_ms_trace_ref)
        trace_where_printf(WHERE, "ENTER stream=%p %s, stream-ref=%d, referee=%p, where=%s\n",
                           pfp(this),
                           ia_stream_name,
                           lv_ref_count,
                           pp_referee,
                           pp_where);
    return lv_ref_count;
}

//
// Purpose: set stream-ref count
//
void SB_Trans::Trans_Stream::ref_set(int pv_val) {
    const char *WHERE = "Trans_Stream::ref_set";

    iv_ref_count.set_val(pv_val);
    if (gv_ms_trace_ref)
        trace_where_printf(WHERE, "ENTER stream=%p %s, stream-ref=%d\n",
    pfp(this), ia_stream_name, pv_val);
}

//
// Purpose: remove md from completion-q
//
bool SB_Trans::Trans_Stream::remove_comp_q(MS_Md_Type *pp_md) {
    return remove_comp_q_static(pp_md);
}

//
// Purpose: remove md from completion-q
//
bool SB_Trans::Trans_Stream::remove_comp_q_static(MS_Md_Type *pp_md) {
    SB_Comp_Queue *lp_q;
    bool           lv_ret;

    if (pp_md->out.iv_opts & MS_OPTS_FSDONE)
        lp_q = pp_md->ip_fs_comp_q;
    else if (pp_md->out.iv_opts & MS_OPTS_LDONE)
        lp_q = pp_md->ip_comp_q;
    else
        lp_q = NULL;
    if (lp_q != NULL)
        lv_ret = lp_q->remove_list(&pp_md->iv_link);
    else
        lv_ret = false;
    return lv_ret;
}

//
// Purpose: remove msgid from reqid map
//
void SB_Trans::Trans_Stream::remove_msgid_from_reqid_map(int  pv_reqid,
                                                         bool pv_lock) {
    RM_Node *lp_node =
      static_cast<RM_Node *>(iv_msgid_reqid_map.remove_lock(pv_reqid, pv_lock));
    delete lp_node;
}

//
// Purpose: remove md from rcv-q
//
bool SB_Trans::Trans_Stream::remove_recv_q(MS_Md_Type *pp_md) {
    bool lv_ret;

    lv_ret = ip_ms_recv_q->remove_list(&pp_md->iv_link);
    return lv_ret;
}

//
// Purpose: remove md from reply-piggyback
//
MS_Md_Type *SB_Trans::Trans_Stream::remove_reply_piggyback() {
    const char *WHERE = "Trans_Stream::remove_reply_piggyback";
    MS_Md_Type *lp_md;

    lp_md = static_cast<MS_Md_Type *>(iv_reply_piggyback_q.remove());
    if (gv_ms_trace && (lp_md != NULL))
        trace_where_printf(WHERE, "stream=%p %s, removed md (msgid=%d, md=%p), count=%d\n",
                           pfp(this), ia_stream_name,
                           lp_md->iv_link.iv_id.i,
                           pfp(lp_md),
                           iv_reply_piggyback_q.size());
    return lp_md;
}

//
// Purpose: send abandon-ack
//
void SB_Trans::Trans_Stream::send_abandon_ack(MS_Md_Type *pp_md,
                                              int         pv_reqid,
                                              int         pv_can_ack_reqid) {
    exec_abandon_ack(pp_md, pv_reqid, pv_can_ack_reqid);
}

//
// Purpose: send close-ack
//
void SB_Trans::Trans_Stream::send_close_ack(MS_Md_Type *pp_md, int pv_reqid) {
    exec_close_ack(pp_md, pv_reqid);
}

//
// Purpose: send close-ind
//
void SB_Trans::Trans_Stream::send_close_ind() {
    const char *WHERE = "Trans_Stream::send_close_ind";
    MS_Md_Type *lp_md;

    if (iv_ic) {
        if (gv_ms_trace || gv_ms_trace_mon)
            trace_where_printf(WHERE,
                               "stream=%p %s, IC stream, close indication ignored\n",
                               pfp(this), ia_stream_name);
    } else {
        Msg_Mgr::get_md(&lp_md, // send_close_ind
                        NULL,
                        NULL,
                        false,  // recv
                        NULL,   // fserr
                        WHERE,
                        MD_STATE_RCVD_MON_CLOSE);
        SB_util_assert_pne(lp_md, NULL); // TODO: can't get md

        lp_md->iv_tag = -1;
        lp_md->out.iv_mon_msg = true;
        lp_md->out.iv_ldone = false;
        lp_md->out.iv_fserr = XZFIL_ERR_OK;
        lp_md->out.iv_nid = iv_remote_nid;
        lp_md->out.iv_pid = iv_remote_pid;
#ifdef SQ_PHANDLE_VERIFIER
        lp_md->out.iv_verif = iv_remote_verif;
#endif
        lp_md->out.iv_msg_type = MS_PMH_TYPE_CLOSE;
        lp_md->out.iv_ptype = iv_opened_type;
        lp_md->out.iv_recv_req_id = -1;
        lp_md->out.iv_pri = -1;
        lp_md->out.iv_recv_mpi_source_rank = -2; // mark aborted
        lp_md->out.iv_recv_mpi_tag = -1;
        lp_md->out.iv_recv_ctrl_max = 0;
        lp_md->out.iv_recv_data_max = 0;
        lp_md->out.ip_recv_data = NULL;

        if (gv_ms_trace || gv_ms_trace_mon)
            trace_where_printf(WHERE,
                               "stream=%p %s, adding (close-ind) md (reqid=%d, msgid=%d, md=%p, name=%s) to server recv queue\n",
                               pfp(this), ia_stream_name,
                               lp_md->out.iv_recv_req_id,
                               lp_md->iv_link.iv_id.i, pfp(lp_md),
                               ia_pname);

        lp_md->out.iv_recv_ctrl_size = static_cast<int>(strlen(ia_pname) + 1);
        lp_md->out.ip_recv_ctrl = static_cast<char *>(MS_BUF_MGR_ALLOC(lp_md->out.iv_recv_ctrl_size));
        // TODO: if can't get buffer
        strcpy(reinterpret_cast<char *>(lp_md->out.ip_recv_ctrl), ia_pname);
        iv_ms_oc_callback(lp_md, this);
    }
}

//
// Purpose: send conn-ack
//
void SB_Trans::Trans_Stream::send_conn_ack(MS_Md_Type *pp_md, int pv_reqid) {
    exec_conn_ack(pp_md, pv_reqid);
}

//
// Purpose: send open-ack
//
void SB_Trans::Trans_Stream::send_open_ack(MS_Md_Type *pp_md, int pv_reqid) {
    exec_open_ack(pp_md, pv_reqid);
}

//
// Purpose: send reply-nack
//
void SB_Trans::Trans_Stream::send_reply_nack(MS_Md_Type *pp_md, int pv_reqid) {
    exec_reply_nack(pp_md, pv_reqid);
}

//
// Purpose: setup md
//
void SB_Trans::Trans_Stream::set_basic_md(Rd_Type     *pp_rd,
                                          MS_PMH_Type *pp_hdr,
                                          MS_Md_Type  *pp_md,
                                          bool         pv_mon_msg) {
    int lv_opts;

#if 0
    bool lv_reply = pp_md->out.iv_reply;
    int lv_reply_ctrl_max = pp_md->out.iv_reply_ctrl_max;
    int lv_reply_data_max = pp_md->out.iv_reply_data_max;
    short *lp_reply_ctrl = pp_md->out.ip_reply_ctrl;
    char *lp_reply_data = pp_md->out.ip_reply_data;
    memset(&pp_md->out, 0, sizeof(pp_md->out));
    pp_md->out.iv_reply = lv_reply;
    pp_md->out.iv_reply_ctrl_max = lv_reply_ctrl_max;
    pp_md->out.iv_reply_data_max = lv_reply_data_max;
    pp_md->out.ip_reply_ctrl = lp_reply_ctrl;
    pp_md->out.ip_reply_data = lp_reply_data;
#endif
    pp_md->iv_tag = -1;
    pp_md->out.iv_mon_msg = pv_mon_msg;
    pp_md->out.iv_ldone = false;
    pp_md->out.iv_fserr = XZFIL_ERR_OK;
    pp_md->out.iv_nid = iv_remote_nid;
#ifdef SQ_PHANDLE_VERIFIER
    pp_md->out.iv_verif = iv_remote_verif;
#endif
    lv_opts = 0;
    if (pp_hdr->iv_opts) {
        if (pp_hdr->iv_opts & MS_PMH_OPT_FSREQ)
            lv_opts |= MS_OPTS_FSREQ;
        if (pp_hdr->iv_opts & MS_PMH_OPT_MSIC)
            lv_opts |= MS_OPTS_MSIC;
    }
    pp_md->out.iv_opts = lv_opts;
    pp_md->out.iv_pid = iv_remote_pid;
    pp_md->out.iv_msg_type = pp_hdr->iv_type;
    pp_md->out.iv_ptype = pp_hdr->iv_ptype;
    pp_md->out.iv_recv_req_id = pp_hdr->iv_reqid;
    pp_md->out.iv_pri = pp_hdr->iv_pri;
    pp_md->out.iv_recv_mpi_source_rank = pp_rd->iv_mpi_source_rank;
    pp_md->out.iv_recv_mpi_tag = pp_rd->iv_mpi_tag;
    pp_md->out.iv_recv_seq1 = pp_hdr->iv_seq1;
    pp_md->out.iv_recv_seq2 = pp_hdr->iv_seq2;
    pp_md->out.ip_recv_ctrl = pp_rd->ip_ctrl;
    pp_md->out.iv_recv_ctrl_size = pp_rd->iv_ctrl_len;
    pp_md->out.ip_recv_data = pp_rd->ip_data;
    pp_md->out.iv_recv_data_size = pp_rd->iv_data_len;
    pp_md->out.iv_recv_ctrl_max = pp_hdr->iv_cmaxsize;
    pp_md->out.iv_recv_data_max = pp_hdr->iv_dmaxsize;
}

void SB_Trans::Trans_Stream::set_mon_stream(Trans_Stream *pp_stream) {
    cp_mon_stream = pp_stream;
}

void SB_Trans::Trans_Stream::set_pname(const char *pp_pname) {
    strcpy(ia_pname, pp_pname);
    set_stream_name();
}

void SB_Trans::Trans_Stream::set_prog(const char *pp_prog) {
    strcpy(ia_prog, pp_prog);
    set_stream_name();
}

void SB_Trans::Trans_Stream::set_stream_name() {
    char          *lp_p;
    int            lv_nid;
    int            lv_pid;
#ifdef SQ_PHANDLE_VERIFIER
    SB_Verif_Type  lv_verif;
#endif

    if (iv_open_nid >= 0) {
        lv_nid = iv_open_nid;
        lv_pid = iv_open_pid;
#ifdef SQ_PHANDLE_VERIFIER
        lv_verif = iv_open_verif;
#endif
    } else {
        lv_nid = iv_opened_nid;
        lv_pid = iv_opened_pid;
#ifdef SQ_PHANDLE_VERIFIER
        lv_verif = iv_opened_verif;
#endif
    }
    lp_p = strchr(ia_stream_name, '=');
    if (lp_p == NULL)
#ifdef SQ_PHANDLE_VERIFIER
        sprintf(ia_stream_name, "p-id=%d/%d/" PFVY " (%s-%s)",
#else
        sprintf(ia_stream_name, "p-id=%d/%d (%s-%s)",
#endif
#ifdef SQ_PHANDLE_VERIFIER
                lv_nid, lv_pid, lv_verif, ia_pname, ia_prog);
#else
                lv_nid, lv_pid, ia_pname, ia_prog);
#endif
    else
#ifdef SQ_PHANDLE_VERIFIER
        sprintf(&lp_p[1], "%d/%d/" PFVY " (%s-%s)",
#else
        sprintf(&lp_p[1], "%d/%d (%s-%s)",
#endif
#ifdef SQ_PHANDLE_VERIFIER
                lv_nid, lv_pid, lv_verif, ia_pname, ia_prog);
#else
                lv_nid, lv_pid, ia_pname, ia_prog);
#endif
}

//
// Purpose: set thread-marker (stop recursion)
//
void SB_Trans::Trans_Stream::set_thread_marker(bool pv_marker) {
    char *lp_buf;
    int   lv_status;

    lp_buf =
      static_cast<char *>(SB_Thread::Sthr::specific_get(gv_sb_stream_tls_inx));
    if (lp_buf == NULL) {
        lp_buf = new char[1];
        lv_status =
          SB_Thread::Sthr::specific_set(gv_sb_stream_tls_inx, lp_buf);
        SB_util_assert_ieq(lv_status, 0);
    }
    *lp_buf = pv_marker;
}

//
// Purpose: set shutdown
//
void SB_Trans::Trans_Stream::set_shutdown(bool pv_shutdown) {
    cv_shutdown = pv_shutdown;
}

//
// Purpose: trace close/del q's
//
void SB_Trans::Trans_Stream::trace_close_del_q(const char *pp_where) {
    const char   *WHERE = "Trans_Stream::trace_close_del_q";
    char         *lp_cll;
    Trans_Stream *lp_stream;
    char         *lp_streamc;
    int           lv_off;
    int           lv_status;

    lv_status = cv_close_mutex.lock();
    SB_util_assert_ieq(lv_status, 0); // sw fault

    // offsetof the hard way
    lp_stream = NULL;
    lv_off = reinterpret_cast<char *>(&lp_stream->iv_close_link) -
             reinterpret_cast<char *>(lp_stream);
    lp_cll = reinterpret_cast<char *>(cv_close_q.head());
    if (lp_cll == NULL) {
        trace_where_printf(WHERE, "where=%s, close-q is empty\n", pp_where);
    } else {
        while (lp_cll != NULL) {
            // lp_cll will contain address of iv_close_link, so
            // lp_stream will have to be calculated
            lp_streamc = lp_cll - lv_off;
            lp_stream = reinterpret_cast<Trans_Stream *>(lp_streamc);
            trace_where_printf(WHERE, "where=%s, close-q, stream=%p %s\n",
                               pp_where,
                               pfp(lp_stream),
                               lp_stream->ia_stream_name);

            lp_cll = reinterpret_cast<char *>(lp_stream->iv_close_link.ip_next);
        }
    }

    lv_status = cv_close_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
}

//
// Purpose: wait until rep md is done
//
void SB_Trans::Trans_Stream::wait_rep_done(MS_Md_Type *pp_md) {
    const char *WHERE = "Trans_Stream::wait_rep_done";
    int         lv_status;

    if (gv_ms_trace)
        trace_where_printf(WHERE, "ENTER stream=%p %s, msgid=%d, md=%p\n",
                           pfp(this), ia_stream_name,
                           pp_md->iv_link.iv_id.i, pfp(pp_md));
    if (!pp_md->iv_reply_done) {
        lv_status = pp_md->iv_cv.wait(true); // need lock
        SB_util_assert_ieq(lv_status, 0);
    }
    iv_rep_map.remove(pp_md->iv_link.iv_id.i);
    if (gv_ms_trace)
        trace_where_printf(WHERE, "EXIT stream=%p %s, msgid=%d, md=%p, reply-done=1\n",
                           pfp(this), ia_stream_name,
                           pp_md->iv_link.iv_id.i, pfp(pp_md));
}

//
// Purpose: wait until req md is done
//
int SB_Trans::Trans_Stream::wait_req_done(MS_Md_Type *pp_md) {
    return wait_req_done_static(pp_md);
}

//
// Purpose: wait until req md is done
// (static)
//
int SB_Trans::Trans_Stream::wait_req_done_static(MS_Md_Type *pp_md) {
    int lv_status;

    if (!pp_md->iv_reply_done) {
        lv_status = pp_md->iv_cv.wait(true); // need lock
        SB_util_assert_ieq(lv_status, 0);
        if (!pp_md->iv_reply_done) {
            // spurious wakeup, wait some more
            lv_status = pp_md->iv_cv.wait(true); // need lock
            SB_util_assert_ieq(lv_status, 0);
        }
        SB_util_assert_bt(pp_md->iv_reply_done);
    }
    return XZFIL_ERR_OK;
}

const char *sb_print_stream_md_state(int pv_state) {
    const char *lp_label =
      SB_get_label(&gv_sb_stream_md_state_label_map, pv_state);
    printf("%s", lp_label);
    return lp_label;
}

char *sb_stream_get_name(MS_Md_Type *pp_md) {
    char                  *lp_name;
    SB_Trans::Stream_Base *lp_stream;

    lp_stream = static_cast<SB_Trans::Stream_Base *>(pp_md->ip_stream);
    if (lp_stream == NULL)
        lp_name = NULL;
    else
        lp_name = lp_stream->get_name();
    return lp_name;
}

