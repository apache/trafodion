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

//
// special defines:
//

//
// Implement SB_Trans::Loop_Stream
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/fserr.h"
#include "seabed/pevents.h"
#include "seabed/trace.h"

#include "looptrans.h"
#include "msic.h"
#include "mstrace.h"
#include "msx.h"

SB_Trans::Loop_Stream::Loop_Stream(const char           *pp_name,
                                   const char           *pp_pname,
                                   const char           *pp_prog,
                                   bool                  pv_ic,
                                   SB_Comp_Cb_Type       pv_ms_comp_callback,
                                   SB_Ab_Comp_Cb_Type    pv_ms_abandon_callback,
                                   SB_ILOC_Comp_Cb_Type  pv_ms_oc_callback,
                                   SB_Lim_Cb_Type        pv_ms_lim_callback,
                                   SB_Recv_Queue        *pp_ms_recv_q,
                                   SB_Recv_Queue        *pp_ms_lim_q,
                                   SB_Ms_Tl_Event_Mgr   *pp_event_mgr)
: Trans_Stream(STREAM_TYPE_SELF,
               pp_name,
               pp_pname,
               pp_prog,
               pv_ic,
               pv_ms_comp_callback,
               pv_ms_abandon_callback,
               pv_ms_oc_callback,
               pv_ms_lim_callback,
               pp_ms_recv_q,
               pp_ms_lim_q,
               pp_event_mgr,
               -1,                      // open_nid
               -1,                      // open_pid
#ifdef SQ_PHANDLE_VERIFIER
               -1,                      // open_verif
#endif
               -1,                      // opened_nid
               -1,                      // opened_pid
#ifdef SQ_PHANDLE_VERIFIER
               -1,                      // opened_verif
#endif
               0) {                     // opened_type
}

SB_Trans::Loop_Stream::~Loop_Stream() {
}

//
// Purpose: create stream
//
SB_Trans::Loop_Stream *SB_Trans::Loop_Stream::create(const char           *pp_name,
                                                     const char           *pp_pname,
                                                     const char           *pp_prog,
                                                     bool                  pv_ic,
                                                     SB_Comp_Cb_Type       pv_ms_comp_callback,
                                                     SB_Ab_Comp_Cb_Type    pv_ms_abandon_callback,
                                                     SB_ILOC_Comp_Cb_Type  pv_ms_oc_callback,
                                                     SB_Lim_Cb_Type        pv_ms_lim_callback,
                                                     SB_Recv_Queue        *pp_ms_recv_q,
                                                     SB_Recv_Queue        *pp_ms_lim_q,
                                                     SB_Ms_Tl_Event_Mgr   *pp_event_mgr) {
    Loop_Stream *lp_stream;

    lp_stream = new Loop_Stream(pp_name,
                                pp_pname,
                                pp_prog,
                                pv_ic,
                                pv_ms_comp_callback,
                                pv_ms_abandon_callback,
                                pv_ms_oc_callback,
                                pv_ms_lim_callback,
                                pp_ms_recv_q,
                                pp_ms_lim_q,
                                pp_event_mgr);
    return lp_stream;
}

//
// exec-entry points for stream
// for loopback, relay data between the parties.
// NOTE: unlike other streams, this stream
//       uses md's for exchange, bypassing need to create packets.
//       Unfortunately, this means re-use is limited
//
short SB_Trans::Loop_Stream::exec_abandon(MS_Md_Type *pp_md,
                                          int         pv_reqid,
                                          int         pv_can_reqid) {
    const char *WHERE = "Loop_Stream::exec_abandon";

    pp_md->iv_op = MS_OP_ABANDON;
    iv_req_map.put(pp_md);
    pp_md->out.iv_pri = pv_can_reqid;
    process_abandon(WHERE, pv_can_reqid, pv_reqid);
    finish_send(WHERE, WHERE, pp_md, XZFIL_ERR_OK, true);
    return XZFIL_ERR_OK;
}

short SB_Trans::Loop_Stream::exec_abandon_ack(MS_Md_Type *pp_md,
                                              int         pv_reqid,
                                              int         pv_can_ack_reqid) {
    const char *WHERE = "Loop_Stream::exec_abandon_ack";
    MS_Md_Type *lp_md;

    pp_md->iv_op = MS_OP_ABANDON_ACK;
    pv_can_ack_reqid = pv_can_ack_reqid; // touch
    lp_md = Msg_Mgr::map_to_md(pv_reqid, WHERE);
    SB_util_assert_pne(lp_md, NULL); // sw fault
    finish_abandon(lp_md);
    finish_send(WHERE, WHERE, pp_md, XZFIL_ERR_OK, true);
    return XZFIL_ERR_OK;
}

short SB_Trans::Loop_Stream::exec_close(MS_Md_Type *pp_md,
                                        int         pv_reqid) {
    const char *WHERE = "Loop_Stream::exec_close";
    MS_Md_Type *lp_md;

    pp_md->iv_op = MS_OP_CLOSE;
    iv_req_map.put(pp_md);
    if (gv_ms_trace)
        trace_where_printf(WHERE, "ENTER msgid=%d, md=%p, reqid=%d\n",
                           pp_md->iv_link.iv_id.i, pfp(pp_md), pv_reqid);
    Msg_Mgr::get_md(&lp_md, // exec-close
                    this,
                    NULL,
                    true,   // send
                    NULL,   // fserr
                    WHERE,
                    MD_STATE_MSG_LOW_LOOP_CLOSE);
    SB_util_assert_pne(lp_md, NULL); // TODO: can't get md
    mon_init(lp_md, pp_md);
    lp_md->out.iv_msg_type = MS_PMH_TYPE_CLOSE;
    lp_md->out.iv_recv_ctrl_size = static_cast<int>(strlen(ga_ms_su_pname) + 1);
    lp_md->out.ip_recv_ctrl = static_cast<char *>
      (MS_BUF_MGR_ALLOC(lp_md->out.iv_recv_ctrl_size));
    // TODO: if can't get buffer
    strcpy(lp_md->out.ip_recv_ctrl, ga_ms_su_pname);
    if (iv_ic)
        lp_md->out.iv_opts |= MS_OPTS_MSIC;

    if (gv_ms_trace)
        trace_where_printf(WHERE,
                           "adding (close) md (reqid=%d, msgid=%d, md=%p) to server recv queue\n",
                           lp_md->out.iv_recv_req_id,
                           lp_md->iv_link.iv_id.i, pfp(lp_md));
    iv_ms_oc_callback(lp_md, NULL);

    finish_close(pp_md);
    finish_send(WHERE, WHERE, pp_md, XZFIL_ERR_OK, true);
    return XZFIL_ERR_OK;
}

short SB_Trans::Loop_Stream::exec_close_ack(MS_Md_Type * /*pp_md*/,
                                            int          /*pv_reqid*/) {
    SB_util_abort("NOT implemented"); // TODO: NOT implemented
    return XZFIL_ERR_OK;
}

short SB_Trans::Loop_Stream::exec_conn_ack(MS_Md_Type * /*pp_md*/,
                                           int          /*pv_reqid*/) {
    SB_util_abort("NOT implemented"); // TODO: NOT implemented
    return XZFIL_ERR_OK;
}

short SB_Trans::Loop_Stream::exec_open(MS_Md_Type *pp_md,
                                       int         pv_reqid) {
    const char *WHERE = "Loop_Stream::exec_open";
    MS_Md_Type *lp_md;

    pp_md->iv_op = MS_OP_OPEN;
    iv_req_map.put(pp_md);
    if (gv_ms_trace)
        trace_where_printf(WHERE, "ENTER msgid=%d, md=%p, reqid=%d\n",
                           pp_md->iv_link.iv_id.i, pfp(pp_md), pv_reqid);
    Msg_Mgr::get_md(&lp_md, // exec-open
                    this,
                    NULL,
                    true,   // send
                    NULL,   // fserr
                    WHERE,
                    MD_STATE_MSG_LOW_LOOP_OPEN);
    SB_util_assert_pne(lp_md, NULL); // TODO: can't get md
    mon_init(lp_md, pp_md);
    lp_md->out.iv_msg_type = MS_PMH_TYPE_OPEN;
    lp_md->out.iv_recv_ctrl_size = static_cast<int>(strlen(ga_ms_su_pname) + 1);
    lp_md->out.ip_recv_ctrl = static_cast<char *>
      (MS_BUF_MGR_ALLOC(lp_md->out.iv_recv_ctrl_size));
    // TODO: if can't get buffer
    strcpy(lp_md->out.ip_recv_ctrl, ga_ms_su_pname);
    if (iv_ic)
        lp_md->out.iv_opts |= MS_OPTS_MSIC;

    if (gv_ms_trace)
        trace_where_printf(WHERE,
                           "adding (open) md (reqid=%d, msgid=%d, md=%p) to server recv queue\n",
                           lp_md->out.iv_recv_req_id,
                           lp_md->iv_link.iv_id.i, pfp(lp_md));
    iv_ms_oc_callback(lp_md, NULL);

    finish_open(pp_md);
    finish_send(WHERE, WHERE, pp_md, XZFIL_ERR_OK, true);
    return XZFIL_ERR_OK;
}

short SB_Trans::Loop_Stream::exec_open_ack(MS_Md_Type * /*pp_md*/,
                                           int          /*pv_reqid*/) {
    SB_util_abort("NOT implemented"); // TODO: NOT implemented
    return XZFIL_ERR_OK;
}

short SB_Trans::Loop_Stream::exec_reply(MS_Md_Type *pp_md,
                                        int         pv_src,
                                        int         pv_dest,
                                        int         pv_reqid,
                                        short      *pp_req_ctrl,
                                        int         pv_req_ctrl_size,
                                        char       *pp_req_data,
                                        int         pv_req_data_size,
                                        short       pv_fserr) {
    const char *WHERE = "Loop_Stream::exec_reply";
    MS_Md_Type *lp_md;
    int         lv_size;

    pp_md->iv_op = MS_OP_REPLY;
    pv_src = pv_src; // touch
    pv_dest = pv_dest; // touch
    // copy data from 'remote'
    if (pp_md->iv_abandoned)
        lp_md = NULL;
    else if (pp_md->iv_self)
        lp_md = NULL;
    else {
        lp_md = Msg_Mgr::map_to_md(pv_reqid, WHERE);
        lv_size = pv_req_ctrl_size;
        lp_md->out.iv_reply_ctrl_count = lv_size;
        int lv_rc;
        recv_copy(WHERE,
                  reinterpret_cast<char **>(&pp_req_ctrl),
                  lv_size,
                  reinterpret_cast<char *>(lp_md->out.ip_reply_ctrl),
                  lp_md->out.iv_reply_ctrl_max,
                  &lv_rc,
                  false);
        lv_size = pv_req_data_size;
        lp_md->out.iv_reply_data_count = lv_size;
        recv_copy(WHERE,
                  &pp_req_data,
                  lv_size,
                  static_cast<char *>(lp_md->out.ip_reply_data),
                  lp_md->out.iv_reply_data_max,
                  &lv_rc,
                  false);
    }

    // complete reply
    finish_send(WHERE, WHERE, pp_md, XZFIL_ERR_OK, true);
    if (lp_md != NULL)
        finish_reply(lp_md, pv_fserr, true, true);
    return XZFIL_ERR_OK;
}

short SB_Trans::Loop_Stream::exec_reply_nack(MS_Md_Type * /*pp_md*/,
                                             int          /*pv_reqid*/) {
    SB_util_abort("NOT implemented"); // TODO: NOT implemented
    return XZFIL_ERR_OK;
}

short SB_Trans::Loop_Stream::exec_reply_nw(MS_Md_Type *pp_md,
                                           int         pv_src,
                                           int         pv_dest,
                                           int         pv_reqid,
                                           short      *pp_req_ctrl,
                                           int         pv_req_ctrl_size,
                                           char       *pp_req_data,
                                           int         pv_req_data_size) {
    return exec_reply(pp_md,
                      pv_src,
                      pv_dest,
                      pv_reqid,
                      pp_req_ctrl,
                      pv_req_ctrl_size,
                      pp_req_data,
                      pv_req_data_size,
                      XZFIL_ERR_OK);
}

short SB_Trans::Loop_Stream::exec_wr(MS_Md_Type *pp_md,
                                     int         pv_src,
                                     int         pv_dest,
                                     int         pv_reqid,
                                     int         pv_pri,
                                     short      *pp_req_ctrl,
                                     int         pv_req_ctrl_size,
                                     char       *pp_req_data,
                                     int         pv_req_data_size,
                                     short      *pp_rep_ctrl,
                                     int         pv_rep_max_ctrl_size,
                                     char       *pp_rep_data,
                                     int         pv_rep_max_data_size,
                                     int         pv_opts) {
    const char *WHERE = "Loop_Stream::exec_wr";
    char        la_err[200];
    MS_Md_Type *lp_md;
    MS_SS_Type *lp_s = &pp_md->iv_ss;
    short       lv_fserr;
    bool        lv_reply;
    int         lv_size;

    pv_src = pv_src; // touch
    pv_dest = pv_dest; // touch
    pv_reqid = pv_reqid; // touch
    pp_md->iv_op = MS_OP_WR;
    // copy data into 'remote' md
    iv_req_map.put(pp_md);
    pp_md->out.iv_fserr = XZFIL_ERR_OK;
    pp_md->out.ip_reply_ctrl = pp_rep_ctrl;
    pp_md->out.iv_reply_ctrl_max = pv_rep_max_ctrl_size;
    pp_md->out.ip_reply_data = pp_rep_data;
    pp_md->out.iv_reply_data_max = pv_rep_max_data_size;
    lp_s->ip_req_ctrl = pp_req_ctrl;
    lp_s->ip_req_data = pp_req_data;
    Msg_Mgr::get_md(&lp_md, // exec-wr
                    this,
                    NULL,
                    true,   // send
                    NULL,   // fserr
                    WHERE,
                    MD_STATE_MSG_LOW_LOOP_WR);
    SB_util_assert_pne(lp_md, NULL); // TODO: can't get md
    lv_size = pv_req_ctrl_size;
    lp_md->out.iv_recv_ctrl_size = lv_size;
    if (lv_size > 0) {
        lp_md->out.ip_recv_ctrl =
          static_cast<char *>(MS_BUF_MGR_ALLOC(lv_size));
        memcpy(lp_md->out.ip_recv_ctrl,
               pp_req_ctrl,
               lv_size);
    }
    lv_size = pv_req_data_size;
    lp_md->out.iv_recv_data_size = lv_size;
    if (lv_size > 0) {
        lp_md->out.ip_recv_data =
          static_cast<char *>(MS_BUF_MGR_ALLOC(lv_size));
        memcpy(lp_md->out.ip_recv_data,
               pp_req_data,
               lv_size);
    }
    lp_md->out.iv_recv_req_id = pp_md->iv_msgid;
    lp_md->out.iv_recv_ctrl_max = pp_md->out.iv_reply_ctrl_max;
    lp_md->out.iv_recv_data_max = pp_md->out.iv_reply_data_max;
    // set some md vars
    lp_md->iv_op = pp_md->iv_op;
    lp_md->out.iv_mon_msg = false;
    lp_md->out.iv_nid = gv_ms_su_nid;
    lp_md->out.iv_opts = pv_opts;
    lp_md->out.iv_pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
    lp_md->out.iv_verif = gv_ms_su_verif;
#endif
    lp_md->out.iv_ptype = gv_ms_su_ptype;
    lp_md->out.iv_pri = pv_pri;

    if (pv_opts & MS_OPTS_MSIC) {
        ms_interceptor(WHERE,
                       lp_md,
                       pv_reqid,
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
        }
    } else {
        add_msgid_to_reqid_map(lp_md->out.iv_recv_req_id,
                               lp_md->iv_link.iv_id.i);
        if (gv_ms_trace)
            trace_where_printf(WHERE,
                               "adding md (reqid=%d, msgid=%d, md=%p) to server recv queue\n",
                               lp_md->out.iv_recv_req_id,
                               lp_md->iv_link.iv_id.i, pfp(lp_md));
        // add it to recv q
        ip_ms_recv_q->add(&lp_md->iv_link);
#ifdef USE_EVENT_REG
        ip_event_mgr->set_event_reg(LREQ);
#else
        ip_event_mgr->set_event_all(LREQ);
#endif
    }

    finish_send(WHERE, WHERE, pp_md, XZFIL_ERR_OK, true);
    return XZFIL_ERR_OK;
}

void SB_Trans::Loop_Stream::close_this(bool pv_local,
                                       bool pv_lock,
                                       bool pv_sem) {
    pv_local = pv_local; // touch
    pv_lock  = pv_lock;  // touch
    pv_sem  = pv_sem;  // touch
}

void SB_Trans::Loop_Stream::comp_sends() {
}

void SB_Trans::Loop_Stream::mon_init(MS_Md_Type *pp_dest_md,
                                     MS_Md_Type *pp_src_md) {
    int lv_req_id;

    lv_req_id = pp_src_md->iv_msgid;
    pp_dest_md->out.iv_recv_req_id = lv_req_id;
    pp_dest_md->iv_tag = -1;
    pp_dest_md->ip_mgr = NULL;
    pp_dest_md->out.iv_mon_msg = true;
    pp_dest_md->out.iv_ldone = false;
    pp_dest_md->out.iv_nid = gv_ms_su_nid;
    pp_dest_md->out.iv_pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
    pp_dest_md->out.iv_verif = gv_ms_su_verif;
#endif
    pp_dest_md->out.iv_recv_req_id = 0;
    pp_dest_md->out.iv_pri = -1;
    pp_dest_md->out.iv_recv_mpi_source_rank = -1;
    pp_dest_md->out.iv_recv_mpi_tag = -1;
    pp_dest_md->out.iv_recv_ctrl_max = 0;
    pp_dest_md->out.iv_recv_data_max = 0;
}

int SB_Trans::Loop_Stream::start_stream() {
    return XZFIL_ERR_OK;
}

void SB_Trans::Loop_Stream::stop_completions() {
}
